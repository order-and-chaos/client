#define _GNU_SOURCE  // asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <assert.h>

#include "msg.h"
#include "util.h"

typedef void (*Value)(ws_conn *conn,const Message*);

typedef struct Chunk{
	int key;
	Value value;
	struct Chunk *next;
} Chunk;

#define HASHMAPSZ (11) //enlarge for larger applications
Chunk* cbhashmap[HASHMAPSZ]={NULL}; //hash map of linked lists

int max_id=1;

static int genid(void){
	return max_id++;
}

static int hashvalue(int key){
	//stub implementation, but should actually work fine for low-frequency messaging
	return key%HASHMAPSZ;
}

static void hash_insert(int key,Value value){
	int hv=hashvalue(key);
	Chunk *ch=malloc(sizeof(Chunk));
	assert(ch);
	ch->key=key;
	ch->value=value;
	ch->next=cbhashmap[hv];
	cbhashmap[hv]=ch;
}

//NULL if nonexistent
static Chunk* hash_find(int key){
	Chunk *ch=cbhashmap[hashvalue(key)];
	while(ch!=NULL&&ch->key!=key){
		ch=ch->next;
	}
	return ch;
}

//returns whether item existed
static bool hash_delete(int key){
	int hv=hashvalue(key);
	Chunk *ch=cbhashmap[hv];
	Chunk *prev=NULL;
	while(ch!=NULL&&ch->key!=key){
		prev=ch;
		ch=ch->next;
	}
	if(ch==NULL)return false;
	if(prev==NULL)cbhashmap[hv]=ch->next;
	else prev->next=ch->next;
	free(ch);
	return true;
}

static void free_message(Message *msg){
	if(msg){
		if(msg->typ)free(msg->typ);
		if(msg->args){
			for(int i=0;i<msg->nargs;i++)if(msg->args[i])free(msg->args[i]);
			free(msg->args);
		}
		free(msg);
	}
}


static Jsonnode* message_to_json(Message msg) {
	Jsonnode *res = json_make_object();

	json_object_add_key(&res->objval, "id", json_make_num(msg.id));
	json_object_add_key(&res->objval, "type", json_make_str(msg.typ));

	Jsonnode *arr = json_make_array();
	for (int i = 0; i < msg.nargs; i++) {
		json_array_add_item(&arr->arrval, json_make_str(msg.args[i]));
	}
	json_object_add_key(&res->objval, "args", arr);

	json_free(arr);
	return res;
}

static Message* message_from_json(Jsonnode *node) {
	Message *res = malloc(sizeof(Message));

	Jsonnode *idnode = json_object_get_item(&node->objval, "id");
	Jsonnode *typnode = json_object_get_item(&node->objval, "type");
	Jsonnode *argsnode = json_object_get_item(&node->objval, "args");
	if (!idnode || !typnode || !argsnode) {
		if(idnode) json_free(idnode);
		if(typnode) json_free(typnode);
		if(argsnode) json_free(argsnode);
		return NULL;
	}

	res->id = idnode->numval;
	json_free(idnode);

	res->typ = astrcpy(typnode->strval);
	json_free(typnode);

	Jsonarray args = argsnode->arrval;
	res->nargs = args.length;
	res->args = malloc(res->nargs * sizeof(char*));
	for (int i = 0; i < args.length; i++) {
		res->args[i] = astrcpy(args.elems[i]->strval);
	}
	json_free(argsnode);

	return res;
}

static Message* parse_message(const char *line){
	Jsonnode *node = json_parse(line, strlen(line));
	Message *res = message_from_json(node);
	json_free(node);
	return res;
}

bool msg_send_x(int id,ws_conn *conn,const char *typ,void (*cb)(ws_conn *conn,const Message*),int nargs,va_list ap){
	assert(typ);
	assert(nargs>=0);

	Message msg; //should not be freed with msg_destroy()
	msg.id = id;
	msg.typ = (char*)typ;
	msg.nargs = nargs;

	msg.args = malloc(nargs * sizeof(char*));
	for (int i = 0; i < nargs; i++) {
		msg.args[i] = va_arg(ap, char*);
	}

	Jsonnode *node = message_to_json(msg);
	free(msg.args);
	char *res = json_stringify(node);
	json_free(node);

	fprintf(stderr, "Sending: '%s'\n",res);

	// add an \n after the message
	size_t len = strlen(res);
	res = realloc(res, (len+1) * sizeof(char));
	res[len  ] = '\n';
	res[len+1] = '\0';

	ws_writestr(conn,res);
	free(res);

	if(cb){
		hash_insert(id,cb);
	}
	return true;
}

bool msg_send(ws_conn *conn,const char *typ,void (*cb)(ws_conn *conn,const Message*),int nargs,...){
	va_list ap;
	va_start(ap,nargs);
	bool ret=msg_send_x(genid(),conn,typ,cb,nargs,ap);
	va_end(ap);
	return ret;
}

bool msg_reply(int id,ws_conn *conn,const char *typ,void (*cb)(ws_conn *conn,const Message*),int nargs,...){
	va_list ap;
	va_start(ap,nargs);
	bool ret=msg_send_x(id,conn,typ,cb,nargs,ap);
	va_end(ap);
	return ret;
}

void msg_runloop(
		ws_conn *conn,
		struct timeval* (*populate_fdsets)(fd_set*,fd_set*),
		void (*msghandler)(ws_conn *conn,const Message*),
		void (*fdhandler)(ws_conn *conn,const fd_set*,const fd_set*),
		void (*timeouthandler)(ws_conn *conn)){

	assert(populate_fdsets);
	assert(msghandler);

	int connsock=ws_selectsock(conn);
	while(true){
		if(!ws_isopen(conn))break;
		fd_set rdset,wrset;
		int ret;
		while(true){
			FD_ZERO(&rdset);
			FD_ZERO(&wrset);
			struct timeval *timeout=populate_fdsets(&rdset,&wrset);
			FD_SET(connsock,&rdset);
			ret=select(FD_SETSIZE,&rdset,&wrset,NULL,timeout); //TODO: improve nfds estimate
			if(timeout)free(timeout);
			if(ret==-1){
				if(errno==EINTR)continue;
				perror("select");
				exit(1);
			}
			break;
		}
		if(ret==0){
			if(timeouthandler)timeouthandler(conn);
			else fprintf(stderr,"No timeout handler while timeout was given!\n");
			continue;
		}

		if(FD_ISSET(connsock,&rdset)){
			char *line=ws_readline(conn);
			if(line==NULL){ //error or closed
				return;
			}
			Message *msg=parse_message(line);
			if(msg){
				int id=msg->id;
				Chunk *ch=hash_find(id);
				if(ch==NULL)msghandler(conn,msg);
				else {
					ch->value(conn,msg);
					hash_delete(id);
				}
				free(msg);
			} else {
				fprintf(stderr,"Could not parse message: '%s'\n",line);
			}
		}

		if(ret>1||(ret==1&&!FD_ISSET(connsock,&rdset))){
			if(fdhandler)fdhandler(conn,&rdset,&wrset);
			else fprintf(stderr,"No fd handler while extra fd's were given!\n");
		}
	}
}
