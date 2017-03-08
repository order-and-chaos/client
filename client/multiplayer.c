#define _GNU_SOURCE  // asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "multiplayer.h"
#include "termio.h"
#include "widget_board.h"
#include "tprintboard.h"
#include "ws.h"
#include "msg.h"
#include "util.h"
#include "../ai.h"

const char *serverhost="localhost";
#define SERVERPORT "1337"


/// MENUDATA'S

static void createroom_menufunc(int);
static void joinroom_menufunc(int);
static Menuitem mainmenuitems[]={
	{"Create new room", 'c', createroom_menufunc},
	{"Join an existing room", 'e', joinroom_menufunc},
	{"Quit", 'q', NULL}
};
static Menudata mainmenudata={3,mainmenuitems};

static void ready_menufunc(int);
static void spectate_menufunc(int);
static void leave_menufunc(int);
static Menuitem lobbymenuitems[]={
	{"Ready", 'r', ready_menufunc},
	{"Spectate", 's', spectate_menufunc},
	{"Leave", 'q', leave_menufunc}
};
static Menudata lobbymenudata={3,lobbymenuitems};



/// MSTATE, STATE MACHINE AND UTILITY FUNCTIONS

typedef enum Statetype{
	SM_WAITNICK,
	SM_MAINMENU,
	SM_INLOBBY,
	SM_INGAME,
	SM_CREATEROOM,
	SM_JOINROOM,
	SM_JOINNAME,
	SM_QUITCONFIRM,
	SM_STOPCONFIRM
} Statetype;

typedef struct Player{
	char *nick;
	bool is_a;
	bool ready;
} Player;
static Player* player_make(const char *nick, bool is_a){
	Player *res=malloc(sizeof(Player));
	res->nick=astrcpy(nick);
	res->is_a=is_a;
	return res;
}
static void player_free(Player *player){
	free(player->nick);
	free(player);
}
static void player_setnick(Player *player, const char *nick){
	free(player->nick);
	player->nick=astrcpy(nick);
}

typedef struct Gamestate{
	Player *player_a;
	Player *player_b;
	Player *onturn; // points to player_{a,b}
} Gamestate;

typedef struct Multiplayerstate{
	ws_conn *conn;

	Logwidget *lgw;
	Logwidget *chw;
	Promptwidget *prw;
	Menuwidget *mw;
	Boardwidget *bdw;
	Promptwidget *inpw;

	char *roomid;

	Statetype state;
	Player *self;
	bool isspectator;

	Gamestate gamestate;
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

	mstate.roomid=NULL;

	mstate.state=SM_WAITNICK;
	mstate.self=player_make("", false);
	mstate.isspectator = false;
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

static bool handleroomjoin(const Message *msg) {
	fprintf(stderr,"inpw: %p\n", mstate.inpw);
	if (strcmp(msg->typ, "ok") != 0) {
		fprintf(stderr,"return\n");
		return false;
	}

	mstate.state=SM_INLOBBY;

	lgw_addf(mstate.lgw,"SM_INLOBBY");
	menu_destroy(mstate.mw);
	mstate.mw = menu_make(2, 2, &lobbymenudata);
	redrawscreen();

	mstate.roomid = astrcpy(msg->args[0]);
	lgw_addf(mstate.lgw,"joined room '%s'\n", msg->args[0]);
	redraw();

	if (msg->nargs == 2) {
		mstate.gamestate.player_a = player_make(msg->args[1], true);
		mstate.gamestate.player_b = mstate.self;
	} else {
		mstate.gamestate.player_a = mstate.self;
	}

	return true;
}

static char* message_format(const Message *msg){
	int len=5+strlen(msg->typ)+3+1;
	for(int i=0;i<msg->nargs;i++){
		if(i!=0)len++;
		len+=strlen(msg->args[i]);
	}
	len++; // '\0'
	char *line=malloc(len);
	if(!line)outofmem();

	int cursor=0;
	strcpy(line+cursor,"typ='");
	cursor+=5;
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

	return line;
}

/// WEBSOCKET CALLBACKS

// simple callback that asserts when server replies with an error, should mostly
// just be used as a placeholder.
static void okcb(ws_conn *conn,const Message *msg){
	(void)conn;
	assert(strcmp(msg->typ,"ok")==0);
}

static void getnickcb(ws_conn *conn,const Message *msg){
	(void)conn;
	fprintf(stderr, "%s\n", message_format(msg));
	if(strcmp(msg->typ,"ok")!=0||msg->nargs!=1) {
		protocolerror();
		return;
	}
	lgw_addf(mstate.lgw,"Nickname: %s",msg->args[0]);

	char *tit;
	asprintf(&tit,"Chat (%s)",msg->args[0]);
	if(!tit)outofmem();
	lgw_changetitle(mstate.chw,tit);
	free(tit);

	player_setnick(mstate.self, msg->args[0]);

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
	if (handleroomjoin(msg)) {
		mstate.isspectator = true;
		lobbymenuitems[1].text = "Play";
		redrawscreen();

#define FREE(x) do { \
	if (x != NULL && x != mstate.self) { \
		player_free(x); \
	} \
	x = NULL; \
} while(0)
		// REVIEW
		FREE(mstate.gamestate.player_a);
		FREE(mstate.gamestate.player_b);
#undef FREE

	} else {
		lgw_addf(mstate.lgw,"spectateroom: %s: %s",msg->typ,msg->args[0]);
		redraw();
	}
}

static void joinroomcb(ws_conn *conn,const Message *msg){
	(void)conn;
	if (handleroomjoin(msg)) {
		mstate.isspectator = false;
		lobbymenuitems[1].text = "Spectate";
		redrawscreen();
	} else {
		lgw_addf(mstate.lgw,"joinroom: %s: %s",msg->typ,msg->args[0]);
		redraw();
	}
}

static void leaveroomcb(ws_conn *conn,const Message *msg){
	(void)conn;
	assert(strcmp(msg->typ, "ok") == 0);
	mstate.state=SM_MAINMENU;
	lgw_add(mstate.lgw,"SM_MAINMENU");
	menu_destroy(mstate.mw);
	mstate.mw=menu_make(2,2,&mainmenudata);
	mstate.roomid=NULL;
	redrawscreen();
}

static void createroomcb(ws_conn *conn,const Message *msg){
	(void)conn;

	assert(strcmp(msg->typ, "ok") == 0 && msg->nargs == 1);

	if (handleroomjoin(msg)) {
		mstate.isspectator = false;
	} else {
		lgw_addf(mstate.lgw, "joined room %s\n", msg->args[0]);
		redraw();
	}
}


/// WEBSOCKET ACTIONS

static void tryspectateroom(char *roomid){
	msg_send(mstate.conn,"spectateroom",spectateroomcb,1,roomid);
}

static void tryjoinroom(char *roomid){
	msg_send(mstate.conn,"joinroom",joinroomcb,1,roomid);
}

static void leaveroom(void) {
	msg_send(mstate.conn, "leaveroom", leaveroomcb, 0);
}

static void printchatmessage(const char *from,const char *msg){
	lgw_addf(mstate.chw,"<%s> %s",from,msg);
}

static void sendchatline(const char *line){
	if(mstate.state!=SM_INLOBBY)lgw_add(mstate.lgw,"[WARN] sendchatline while not in room");
	msg_send(mstate.conn,"sendroomchat",sendroomchatcb,1,line);
	printchatmessage(mstate.self->nick,line);
}


/// MENU FUNCTIONS

static void createroom_menufunc(){
	lgw_add(mstate.lgw,"createroom_menufunc");
	redraw();
	msg_send(mstate.conn,"makeroom",createroomcb,0);
}


static void joinroom_menufunc(){
	mstate.state=SM_JOINROOM;
	lgw_add(mstate.lgw,"SM_JOINROOM");
	mstate.inpw=prw_make(2,6,20,"Room ID");
	redrawscreen();
}

static void ready_menufunc() {
	mstate.self->ready = !mstate.self->ready;
	msg_send(mstate.conn, "ready", okcb, 1, !mstate.self->ready ? "0" : "1");

	lobbymenuitems[0].text = mstate.self->ready ? "Unready" : "Ready";
	redrawscreen();
}

static void spectate_menufunc() {
	if (mstate.isspectator) {
		tryjoinroom(mstate.roomid);
	} else {
		tryspectateroom(mstate.roomid);
	}
}

static void leave_menufunc() {
	leaveroom();
}


/// GENERAL FUNCTIONS

static void msghandler(ws_conn *conn,const Message *msg){
#define CHECKTYPE(x) (strcmp(msg->typ, x) == 0)

	if(CHECKTYPE("ping")){
		msg_reply(msg->id,conn,"pong",NULL,0);
	} else if(CHECKTYPE("pong")){
		//do nothing
	} else if(CHECKTYPE("chatmessage")){
		printchatmessage(msg->args[0],msg->args[2]);
		redraw();
	} else if (CHECKTYPE("joinroom") ) {
		if (strcmp(msg->args[1], "0") == 0) {
			mstate.gamestate.player_a = player_make(msg->args[0], true);
		} else {
			mstate.gamestate.player_b = player_make(msg->args[0], false);
		}

		lgw_addf(mstate.lgw, "%s entered the room", msg->args[0]);
		redraw();
	} else if (CHECKTYPE("ready") ) {
		const char *nick = msg->args[0];
		const Gamestate *gs = &mstate.gamestate;

		if (strcmp(gs->player_a->nick, nick) == 0) {
			gs->player_a->ready = true;
		} else if (strcmp(gs->player_b->nick, nick) == 0) {
			gs->player_b->ready = true;
		} else {
			// TODO: fuck
			assert(false);
		}

		lgw_addf(mstate.lgw, "%s is ready!", nick);
		redraw();
	} else {
		lgw_addf(mstate.lgw, "Unsollicited message received: %s", message_format(msg));
		redraw();
	}

#undef CHECKTYPE
}

static void fdhandler(ws_conn *conn,const fd_set *rdset,const fd_set *wrset){
	(void)rdset;
	(void)wrset;
	int key=tgetkey();

	switch(mstate.state){
		case SM_WAITNICK:
			break;
		case SM_INLOBBY:
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
		case SM_INGAME:{
			char *line=prw_handlekey(mstate.prw,key);
			char *trimstart=line==NULL?NULL:trimstring(line);
			if(line!=NULL&&strlen(trimstart)!=0){
				sendchatline(trimstart);
				free(line);
			}
			redraw();
			break;
		}
		case SM_CREATEROOM:
			break;
		case SM_JOINROOM:{
			char *line=prw_handlekey(mstate.inpw,key);
			if(line!=NULL){
				prw_destroy(mstate.inpw);
				mstate.inpw=NULL;
				mstate.state=SM_MAINMENU;
				lgw_add(mstate.lgw,"SM_MAINMENU");
				tryspectateroom(line);
				redrawscreen();
			} else redraw();
			break;
		}
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

void startmultiplayer(){
	clearscreen();
	moveto(0,0);
	setbold(true);
	tprintf("Order & Chaos -- Multiplayer");
	setbold(false);

	mstate_initialise();

	mstate.lgw=lgw_make(35,0,45,15,"Log",true);
	if(!mstate.lgw)outofmem();

	mstate.chw=lgw_make(35,15,45,15,"Chat",true);
	if(!mstate.chw)outofmem();

	mstate.prw=prw_make(36,30,43, NULL);
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
