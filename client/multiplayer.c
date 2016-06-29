#define _GNU_SOURCE  // asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "multiplayer.h"
#include "termio.h"
#include "widget.h"
#include "ws.h"
#include "msg.h"
#include "util.h"
#include "../ai.h"

#define SERVERHOST "localhost"
#define SERVERPORT "1337"


//state shared between callbacks
typedef struct Multiplayerstate{
	ws_conn *conn;
	char *nickname;
	Board *board;
	char *roomid;
	Logwidget *lgw;
	Menuwidget *mw;
} Multiplayerstate;

static Multiplayerstate mstate;


static void makeroomcb(ws_conn *conn,const Message *msg){
	if(strcmp(msg->typ,"ok")!=0){
		lgw_addf(mstate.lgw,"makeroom returned message with type '%s'",msg->typ);
		msg_send(conn,"makeroom",makeroomcb,0);
		return;
	}
	assert(msg->nargs==1);

	asprintf(&mstate.roomid,"%s",msg->args[0]);
	if(!mstate.roomid)outofmem();
	lgw_addf(mstate.lgw,"Room id: %s",mstate.roomid);
	redraw();
}

static void startroom(void){
	lgw_add(mstate.lgw,"Creating room...");
	msg_send(mstate.conn,"makeroom",makeroomcb,0);
}

static void joinroom(void){
	lgw_add(mstate.lgw,"joinroom");
}

static void pingcb(ws_conn *conn,const Message *msg){
	(void)conn;
	if(strcmp(msg->typ,"pong")!=0){
		lgw_addf(mstate.lgw,"Got '%s' in reply to ping instead of pong!\n",msg->typ);
		redraw();
	}
}

static Menuitem menu1items[]={
	{"Start new room",'s',startroom},
	{"Join existing room",'e',joinroom},
	{"Quit",'q',NULL}
};
static Menudata menu1data={3,menu1items};

static void menu1_redraw(void){
	clearscreen();
	moveto(0,0);
	setbold(true);
	tprintf("Order & Chaos -- Multiplayer");
	setbold(false);

	lgw_redraw(mstate.lgw);
	if(mstate.mw)menu_redraw(mstate.mw);
}

static void getnickcb(ws_conn *conn,const Message *msg){
	if(strcmp(msg->typ,"ok")!=0||msg->nargs!=1){
		lgw_addf(mstate.lgw,"getnick returned message with type '%s'",msg->typ);
		msg_send(conn,"getnick",getnickcb,0);
		return;
	}
	asprintf(&mstate.nickname,"%s",msg->args[0]);
	if(!mstate.nickname)outofmem();
	lgw_addf(mstate.lgw,"Your nickname: %s",msg->args[0]);
	redraw();

	mstate.mw=menu_make(2,2,&menu1data);
	if(!mstate.mw)outofmem();
	redraw();
}

static struct timeval* populate_fdsets(fd_set *readfds,fd_set *writefds){
	(void)writefds;
	FD_SET(0,readfds);
	struct timeval *tv=malloc(sizeof(struct timeval));
	if(!tv)outofmem();
	tv->tv_sec=5;
	tv->tv_usec=0;
	return tv;
}

static void msghandler(ws_conn *conn,const Message *msg){
	(void)conn;
	lgw_addf(mstate.lgw,"Unsollicited message received: id=%d typ='%s' nargs=%d",msg->id,msg->typ,msg->nargs);
	redraw();
}

static void fdhandler(ws_conn *conn,const fd_set *readfds,const fd_set *writefds){
	(void)conn;
	(void)writefds;
	if(!FD_ISSET(0,readfds))return;
	int key=tgetkey();
	if(!mstate.mw){
		bel();
		return;
	}
	Menukey ret=menu_handlekey(mstate.mw,key);
	switch(ret){
		case MENUKEY_HANDLED:
			break;

		case MENUKEY_IGNORED:
			bel();
			break;

		case MENUKEY_QUIT:
			ws_close(conn);
			break;

		case MENUKEY_CALLED:
			menu1_redraw();
			break;
	}
	redraw();
}

static void timeouthandler(ws_conn *conn){
	msg_send(conn,"ping",pingcb,0);
}

void startmultiplayer(void){
	clearscreen();
	moveto(0,0);
	setbold(true);
	tprintf("Order & Chaos -- Multiplayer");
	setbold(false);

	mstate.nickname=NULL;
	mstate.board=NULL;
	mstate.roomid=NULL;
	mstate.lgw=lgw_make(35,0,45,15);
	if(!mstate.lgw)outofmem();
	mstate.mw=NULL;

	lgw_add(mstate.lgw,"Connecting...");
	redraw();

	ws_context *ctx=ws_init();
	if(!ctx){
		lgw_add(mstate.lgw,"Could not initialise websocket context!");
		redraw();
		tgetkey();
		return;
	}

	ws_conn *conn=ws_connect(ctx,SERVERHOST,SERVERPORT,"/ws");
	if(!conn){
		ws_destroy(ctx);
		lgw_addf(mstate.lgw,"Could not connect to server! (%s:%s)",SERVERHOST,SERVERPORT);
		lgw_add(mstate.lgw,"Press a key to return.");
		redraw();
		tgetkey();
		return;
	}

	mstate.conn=conn;

	lgw_add(mstate.lgw,"Connected.");
	redraw();

	msg_send(conn,"getnick",getnickcb,0);
	msg_runloop(conn,populate_fdsets,msghandler,fdhandler,timeouthandler);

	ws_close(conn);
	ws_destroy(ctx);
}
