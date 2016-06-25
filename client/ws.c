#include <string.h>
#include <assert.h>

#include <nopoll.h>
#undef strlen //nopoll does that

#include "ws.h"

#define READBUFSZ (1024)

// #define CAT_(a,b) a b
// #define CAT(a,b) CAT_(a,b)
#define CAT6_(a,b,c,d,e,f) a b c d e f
#define CAT6(a,b,c,d,e,f) CAT6_(a,b,c,d,e,f)
#define DEBUG_HEADER_(func,line,format) CAT6(func,"(",#line,"): ",format,"\n")
#define DEBUG_HEADER(func,line,format) DEBUG_HEADER_(func,line,format)
#define DEBUG(str) fprintf(stderr,DEBUG_HEADER(__FILE__,__LINE__,str))
#define DEBUGF(format,...) fprintf(stderr,DEBUG_HEADER(__FILE__,__LINE__,format),__VA_ARGS__)

struct ws_context{
	noPollCtx *npctx;
};

struct ws_conn{
	ws_context *ctx;
	noPollConn *npconn;
	char *readbuf;
	int readbuflen;
	bool open;
};


ws_context* ws_init(void){
	noPollCtx *npctx=nopoll_ctx_new();
	if(npctx==NULL)return NULL;
	ws_context *ctx=malloc(sizeof(ws_context));
	if(ctx==NULL)return NULL;
	ctx->npctx=npctx;
	return ctx;
}

void ws_destroy(ws_context *ctx){
	assert(ctx);
	nopoll_ctx_unref(ctx->npctx);
}

ws_conn* ws_connect(ws_context *ctx,const char *hostname,const char *port,const char *path){
	assert(ctx);
	noPollConn *npconn=nopoll_conn_new(ctx->npctx,hostname,port,NULL,path,NULL,NULL);
	if(!nopoll_conn_is_ok(npconn)){
		nopoll_conn_unref(npconn);
		return NULL;
	}
	if(!nopoll_conn_wait_until_connection_ready(npconn,5)){
		nopoll_conn_unref(npconn);
		return NULL;
	}
	ws_conn *conn=malloc(sizeof(ws_conn));
	if(conn==NULL){
		nopoll_conn_unref(npconn);
		return NULL;
	}
	conn->ctx=ctx;
	conn->readbuf=malloc(READBUFSZ);
	if(conn->readbuf==NULL){
		nopoll_conn_unref(npconn);
		free(conn);
		return NULL;
	}
	conn->readbuflen=0;
	conn->open=true;
	conn->npconn=npconn;
	return conn;
}

void ws_close(ws_conn *conn){
	assert(conn);
	if(conn->npconn)nopoll_conn_unref(conn->npconn);
	conn->npconn=NULL;
	if(conn->readbuf)free(conn->readbuf);
	conn->readbuf=NULL;
	conn->open=false;
}


bool ws_isopen(ws_conn *conn){
	return conn&&conn->open;
}


void ws_write(ws_conn *conn,const char *data,int length){
	assert(ws_isopen(conn));
	if(ws_write_noblock(conn,data,length))return;
	do nopoll_sleep(10000); //10ms
	while(!ws_flush(conn));
}

void ws_writestr(ws_conn *conn,const char *str){
	assert(ws_isopen(conn));
	ws_write(conn,str,strlen(str));
}

bool ws_write_noblock(ws_conn *conn,const char *data,int length){
	assert(ws_isopen(conn));
	return nopoll_conn_send_text(conn->npconn,data,length)==length;
}

bool ws_writestr_noblock(ws_conn *conn,const char *str){
	assert(ws_isopen(conn));
	return ws_write_noblock(conn,str,strlen(str));
}

bool ws_flush(ws_conn *conn){
	assert(ws_isopen(conn));
	return nopoll_conn_complete_pending_write(conn->npconn)==0;
}


int ws_selectsock(ws_conn *conn){
	assert(ws_isopen(conn));
	return nopoll_conn_socket(conn->npconn);
}

//-1: error; 0: peer closed; >0: bytes read
//blocks till at least one byte is received; mimics behaviour of recv(2)
static int read_halfblock(noPollConn *npconn,char *buf,int length){
	assert(npconn);
	int nrecv;
	while((nrecv=nopoll_conn_read(npconn,buf,length,false,0))==-1){
		if(!nopoll_conn_is_ok(npconn))return 0; //socket closed
		nopoll_sleep(10000); //10ms
	}
	return nrecv;
}

char* ws_readlinex(ws_conn *conn,char delim){
	assert(ws_isopen(conn));
	int bufsz=READBUFSZ;
	char *buf=malloc(bufsz);
	int buflen=0;
	if(!buf)return NULL;

	if(conn->readbuflen>0){
		char *p=(char*)memchr(conn->readbuf,delim,conn->readbuflen);
		if(p!=NULL){
			int idx=p-conn->readbuf;
			memcpy(buf,conn->readbuf,idx);
			if(idx<conn->readbuflen-1){
				memmove(conn->readbuf,conn->readbuf+idx+1,conn->readbuflen-idx-1);
			}
			conn->readbuflen-=idx+1;
			buf[idx]='\0';
			return buf;
		}

		memcpy(buf,conn->readbuf,conn->readbuflen);
		buflen=conn->readbuflen;
		conn->readbuflen=0;
	}

	while(true){
		if(bufsz-buflen<READBUFSZ/2){
			bufsz+=READBUFSZ;
			char *newbuf=realloc(buf,bufsz);
			if(newbuf==NULL){
				free(buf);
				return NULL;
			}
			buf=newbuf;
		}
		int nrecv=read_halfblock(conn->npconn,buf+buflen,bufsz-buflen);
		if(nrecv==0)conn->open=false;
		if(nrecv<=0)return NULL;

		char *p=(char*)memchr(buf+buflen,delim,nrecv);
		if(p!=NULL){
			int idx=p-buf;
			buf[idx]='\0';
			if(idx<buflen+nrecv-1){
				conn->readbuflen=buflen+nrecv-idx-1;
				memcpy(conn->readbuf,p+1,conn->readbuflen);
			}
			return buf;
		}
	}
}

char* ws_readline(ws_conn *conn){
	assert(ws_isopen(conn));
	return ws_readlinex(conn,'\n');
}


void ws_ping(ws_conn *conn){
	assert(ws_isopen(conn));
	nopoll_conn_send_ping(conn->npconn);
}
