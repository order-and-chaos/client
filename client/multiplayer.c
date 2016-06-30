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
	char *oppnick;
	Board *board;
	char *roomid;
	Logwidget *lgw;
	Menuwidget *mw;
	void (*aux_keyhandler)(int); // Called by fdhandler when lone key pressed (if !=NULL); should reinstall itself if necessary
} Multiplayerstate;

static Multiplayerstate mstate;

static void mstate_initialise(void){
	mstate.conn=NULL;
	mstate.nickname=NULL;
	mstate.oppnick=NULL;
	mstate.board=NULL;
	mstate.roomid=NULL;
	mstate.lgw=NULL;
	mstate.mw=NULL;
	mstate.aux_keyhandler=NULL;
}


static void startroom(void);
static void joinroom(void);
static void redraw_mw_back(void);
static void makeroomcb__cancel_q(int key);

static Menuitem menu1items[]={
	{"Start new room",'s',startroom},
	{"Join existing room",'e',joinroom},
	{"Quit",'q',NULL}
};
static Menudata menu1data={3,menu1items};


static void log_message(const Message *msg,const char *prefix){
	int len=strlen(prefix)+6+strlen(msg->typ)+3+1;
	for(int i=0;i<msg->nargs;i++){
		if(i!=0)len++;
		len+=strlen(msg->args[i]);
	}
	len++; // '\0'
	char *line=malloc(len);
	if(!line)outofmem();

	int cursor=0;
	strcpy(line+cursor,prefix);
	cursor+=strlen(prefix);
	strcpy(line+cursor," typ='");
	cursor+=6;
	strcpy(line+cursor,msg->typ);
	cursor+=strlen(msg->typ);
	strcpy(line+cursor,"' [");
	cursor+=3;
	for(int i=0;i<msg->nargs;i++){
		if(i!=0)line[cursor++]=',';
		strcpy(line+cursor,msg->args[i]);
		cursor+=strlen(msg->args[i]);
	}
	line[cursor++]=']';
	line[cursor]='\0';

	lgw_add(mstate.lgw,line);
	free(line);
}

static void start_game(void){
	lgw_addf(mstate.lgw,"Trying to start a game right here...");
	if(mstate.aux_keyhandler==makeroomcb__cancel_q){
		mstate.aux_keyhandler=NULL;
	}
	if(mstate.mw){
		menu_destroy(mstate.mw);
		mstate.mw=NULL;
	}
	redraw_mw_back();
	redraw();
}


static void leaveroomcb(ws_conn *conn,const Message *msg){
	(void)conn;
	if(strcmp(msg->typ,"ok")==0){
		lgw_add(mstate.lgw,"leaveroom ok");
		redraw();
		return;
	}
	log_message(msg,"leaveroom:");
	//lgw_addf(mstate.lgw,"leaveroom: reponse typ=%s nargs=%d [0]='%s'",msg->typ,msg->nargs,msg->args[0]);
	redraw();
}

static void makeroomcb__cancel_q(int key){
	if(key!=(int)'q'){
		bel();
		mstate.aux_keyhandler=makeroomcb__cancel_q;
		return;
	}

	msg_send(mstate.conn,"leaveroom",leaveroomcb,0);

	mstate.mw=menu_make(2,2,&menu1data);
	if(!mstate.mw)outofmem();
	redraw_mw_back();
	redraw();
}

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

	menu_destroy(mstate.mw);
	mstate.mw=NULL;
	redraw_mw_back();

	moveto(0,3);
	tprintf("Your room id: %s\n\nWaiting for other player...",mstate.roomid);
	redraw();
	pushcursor();
	moveto(0,6);
	tputc('(');
	setbold(true);
	tputc('q');
	setbold(false);
	tprintf(" to cancel)");
	popcursor();
	redraw();

	mstate.aux_keyhandler=makeroomcb__cancel_q;
}

static void joinroomcb(ws_conn *conn,const Message *msg){
	(void)conn;
	log_message(msg,"joinroomcb:");
	//lgw_addf(mstate.lgw,"joinroomcb: typ=%s nargs=%d",msg->typ,msg->nargs);
	redraw();
}

static void startroom(void){
	lgw_add(mstate.lgw,"Creating room...");
	msg_send(mstate.conn,"makeroom",makeroomcb,0);
}

static void joinroom(void){
	// lgw_add(mstate.lgw,"joinroom not implemented yet...");
	moveto(0,6);
	tprintf("Room id to join: ");
	redraw();
	char *line=tgetline();
	if(line==NULL)return; // cancel
	msg_send(mstate.conn,"joinroom",joinroomcb,1,line);
}

static void pingcb(ws_conn *conn,const Message *msg){
	(void)conn;
	if(strcmp(msg->typ,"pong")!=0){
		lgw_addf(mstate.lgw,"Got '%s' in reply to ping instead of pong!\n",msg->typ);
		redraw();
	}
}

static void redraw_mw_back(void){
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
	if(strcmp(msg->typ,"joinroom")==0&&msg->nargs==1){
		lgw_addf(mstate.lgw,"Opponent: %s",msg->args[0]);
		asprintf(&mstate.oppnick,"%s",msg->args[0]);
		if(!mstate.oppnick)outofmem();
		start_game();
	} else {
		lgw_addf(mstate.lgw,"Unsollicited message received: id=%d typ='%s' nargs=%d",msg->id,msg->typ,msg->nargs);
		redraw();
	}
}

static void maybekeyhandler(int key){
	if(mstate.aux_keyhandler){
		void (*func)(int)=mstate.aux_keyhandler;
		mstate.aux_keyhandler=NULL;
		func(key);
	} else bel();
}

static void fdhandler(ws_conn *conn,const fd_set *readfds,const fd_set *writefds){
	(void)conn;
	(void)writefds;
	if(!FD_ISSET(0,readfds))return;
	int key=tgetkey();
	if(!mstate.mw){
		maybekeyhandler(key);
		return;
	}
	Menukey ret=menu_handlekey(mstate.mw,key);
	switch(ret){
		case MENUKEY_HANDLED:
			break;

		case MENUKEY_IGNORED:
			maybekeyhandler(key);
			break;

		case MENUKEY_QUIT:
			ws_close(conn);
			break;

		case MENUKEY_CALLED:
			redraw_mw_back();
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

	mstate_initialise();

	mstate.lgw=lgw_make(35,0,45,15);
	if(!mstate.lgw)outofmem();

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
