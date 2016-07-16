#define _GNU_SOURCE  // asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "multiplayer.h"
#include "termio.h"
#include "tprintboard.h"
#include "widget.h"
#include "ws.h"
#include "msg.h"
#include "util.h"
#include "../ai.h"

const char *serverhost="localhost";
#define SERVERPORT "1337"


/// MENUDATA'S

static void createroom_menufunc(void);
static void joinroom_menufunc(void);

static Menuitem mainmenuitems[]={
	{"Create new room", 'c', createroom_menufunc},
	{"Join an existing room", 'e', joinroom_menufunc},
	{"Quit", 'q', NULL}
};
static Menudata mainmenudata={3,mainmenuitems};



/// MSTATE, STATE MACHINE AND UTILITY FUNCTIONS

typedef enum Statetype{
	SM_WAITNICK,
	SM_MAINMENU,
	SM_INROOM,
	SM_INGAME,
	SM_CREATEROOM,
	SM_JOINROOM,
	SM_JOINNAME,
	SM_QUITCONFIRM,
	SM_STOPCONFIRM
} Statetype;

typedef struct Multiplayerstate{
	ws_conn *conn;

	Logwidget *lgw;
	Logwidget *chw;
	Promptwidget *prw;
	Menuwidget *mw;
	Boardwidget *bdw;
	Promptwidget *inpw;

	char *nick;
	char *roomid;

	Statetype state;
} Multiplayerstate;

Multiplayerstate mstate;


static void mstate_initialise(void){
	mstate.conn=NULL;

	mstate.lgw=NULL;
	mstate.chw=NULL;
	mstate.prw=NULL;
	mstate.mw=NULL;
	mstate.bdw=NULL;
	mstate.inpw=NULL;

	mstate.nick=NULL;
	mstate.roomid=NULL;

	mstate.state=SM_WAITNICK;
}

static void protocolerror(void){
	if(!mstate.lgw)return;
	lgw_add(mstate.lgw,"<<Server protocol error!>>");
}

static void redrawscreen(void){
	clearscreen();
	moveto(0,0);
	setbold(true);
	tprintf("Order & Chaos -- Multiplayer");
	setbold(false);

	if(mstate.lgw)lgw_redraw(mstate.lgw);
	if(mstate.chw)lgw_redraw(mstate.chw);
	if(mstate.bdw)bdw_redraw(mstate.bdw);
	if(mstate.prw)prw_redraw(mstate.prw);
	if(mstate.mw)menu_redraw(mstate.mw);
	if(mstate.inpw)prw_redraw(mstate.inpw);
	redraw();
}

static void createroom_menufunc(void){
	lgw_add(mstate.lgw,"createroom_menufunc");
	redraw();
}

static void joinroom_menufunc(void){
	mstate.state=SM_JOINROOM;
	lgw_add(mstate.lgw,"SM_JOINROOM");
	mstate.inpw=prw_make(2,6,20);
	redrawscreen();
}



/// WEBSOCKET CALLBACKS

static void getnickcb(ws_conn *conn,const Message *msg){
	(void)conn;
	if(strcmp(msg->typ,"ok")!=0||msg->nargs!=1){protocolerror(); return;}
	asprintf(&mstate.nick,"%s",msg->args[0]);
	if(!mstate.nick)outofmem();
	lgw_addf(mstate.lgw,"Nickname: %s",mstate.nick);

	mstate.state=SM_MAINMENU;
	lgw_add(mstate.lgw,"SM_MAINMENU");
	mstate.mw=menu_make(2,2,&mainmenudata);
	redraw();
}

static void sendroomchatcb(ws_conn *conn,const Message *msg){
	(void)conn;
	(void)msg;
}

static void spectateroomcb(ws_conn *conn,const Message *msg){
	(void)conn;
	if(strcmp(msg->typ,"ok")!=0){
		lgw_addf(mstate.lgw,"spectateroom: %s: %s",msg->typ,msg->args[0]);
		redraw();
	}
}



/// GENERAL FUNCTIONS

static void tryjoinroom(void){
	msg_send(mstate.conn,"spectateroom",spectateroomcb,1,mstate.roomid);
}

static void sendchatline(const char *line){
	if(mstate.state!=SM_INGAME)lgw_add(mstate.lgw,"[WARN] sendchatline while not in room");
	msg_send(mstate.conn,"sendroomchat",sendroomchatcb,1,line);
}

static void msghandler(ws_conn *conn,const Message *msg){
	if(strcmp(msg->typ,"ping")==0){
		msg_reply(msg->id,conn,"pong",NULL,0);
	} else if(strcmp(msg->typ,"pong")==0){
		//do nothing
	}
}

static void fdhandler(ws_conn *conn,const fd_set *rdset,const fd_set *wrset){
	(void)rdset;
	(void)wrset;
	int key=tgetkey();

	switch(mstate.state){
		case SM_WAITNICK:
			break;
		case SM_MAINMENU:
			switch(menu_handlekey(mstate.mw,key)){
				case MENUKEY_HANDLED:
					redraw();
					break;
				case MENUKEY_IGNORED:
					bel();
					break;
				case MENUKEY_QUIT:
					ws_close(conn);
					break;
				case MENUKEY_CALLED:
					redrawscreen();
					break;
			}
			break;
		case SM_INROOM:{
			char *line=prw_handlekey(mstate.prw,key);
			if(line!=NULL){
				sendchatline(line);
				free(line);
			}
			break;
		}
		case SM_CREATEROOM:
			break;
		case SM_JOINROOM:{
			char *line=prw_handlekey(mstate.inpw,key);
			if(line!=NULL){
				prw_destroy(mstate.inpw);
				mstate.inpw=NULL;
				mstate.roomid=line;
				mstate.state=SM_MAINMENU;
				lgw_add(mstate.lgw,"SM_MAINMENU");
				tryjoinroom();
				redrawscreen();
			} else redraw();
			break;
		}
		case SM_INGAME:
		case SM_JOINNAME:
		case SM_QUITCONFIRM:
		case SM_STOPCONFIRM:
			break;
	}
}

static void timeouthandler(ws_conn *conn){
	msg_send(conn,"ping",NULL,0);
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

static void signalhandler(int sig){
	if(sig!=SIGINT)return;
	if(mstate.conn){
		ws_close(mstate.conn);
		mstate.conn=NULL;
	}
	exit(130);
}

void startmultiplayer(void){
	clearscreen();
	moveto(0,0);
	setbold(true);
	tprintf("Order & Chaos -- Multiplayer");
	setbold(false);

	mstate_initialise();

	mstate.lgw=lgw_make(35,0,45,15,"Log");
	if(!mstate.lgw)outofmem();

	mstate.chw=lgw_make(35,15,45,15,"Chat");
	if(!mstate.chw)outofmem();

	mstate.prw=prw_make(36,30,43);
	if(!mstate.prw)outofmem();

	lgw_add(mstate.lgw,"Connecting...");
	redraw();

	ws_context *ctx=ws_init();
	if(!ctx){
		lgw_add(mstate.lgw,"Could not initialise websocket context!");
		redraw();
		tgetkey();
		return;
	}

	ws_conn *conn=ws_connect(ctx,serverhost,SERVERPORT,"/ws");
	if(!conn){
		ws_destroy(ctx);
		lgw_addf(mstate.lgw,"Could not connect to server! (%s:%s)",serverhost,SERVERPORT);
		lgw_add(mstate.lgw,"Press a key to return.");
		redraw();
		tgetkey();
		return;
	}

	mstate.conn=conn;

	signal(SIGINT,signalhandler);

	lgw_add(mstate.lgw,"Connected.");
	redraw();

	msg_send(conn,"getnick",getnickcb,0);
	mstate.state=SM_WAITNICK;
	lgw_add(mstate.lgw,"SM_WAITNICK");
	msg_runloop(conn,populate_fdsets,msghandler,fdhandler,timeouthandler);

	ws_close(conn);
	ws_destroy(ctx);

	if(mstate.lgw)lgw_destroy(mstate.lgw);
	if(mstate.chw)lgw_destroy(mstate.chw);
	if(mstate.mw)menu_destroy(mstate.mw);
	if(mstate.prw)prw_destroy(mstate.prw);
	if(mstate.bdw)bdw_destroy(mstate.bdw);
}
