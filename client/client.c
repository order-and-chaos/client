#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <assert.h>

#include "termio.h"
#include "widget.h"
#include "ws.h"
#include "msg.h"
#include "../ai.h"
#include "../params.h"

#define SERVERHOST "localhost"
#define SERVERPORT "1337"


__attribute__((noreturn)) void outofmem(void){
	endkeyboard();
	endscreen();
	printf("OUT OF MEMORY!\n");
	exit(1);
}

void tprintboard(const Board *board){
	uint64_t b0=board->b[0],b1=board->b[1];
	char buf[2*N];
	buf[2*N-1]='\0';
	for(int y=0;y<N;y++){
		for(int x=0;x<N;x++){
			buf[2*x]=".OX"[(b0&1)+2*(b1&1)];
			if(x<N-1)buf[2*x+1]=' ';
			b0>>=1;
			b1>>=1;
		}
		tprintf("%s\n",buf);
	}
}

void notimplemented(void){
	moveto(0,6);
	setbold(true);
	tprintf("Feature not implemented.");
	setbold(false);
	redraw();
	getkey();
}

typedef struct Multistate{
	char *nickname;
	Board *board;
	Logwidget *lgw;
} Multistate;

Multistate mstate;

void pingcb(ws_conn *conn,const Message *msg){
	(void)conn;
	if(strcmp(msg->typ,"pong")!=0){
		char *str;
		asprintf(&str,"Got '%s' in reply to ping instead of pong!\n",msg->typ);
		if(!str)outofmem();
		lgw_add(mstate.lgw,str);
		free(str);
		redraw();
	}
}

void getnickcb(ws_conn *conn,const Message *msg){
	if(strcmp(msg->typ,"ok")!=0||msg->nargs!=1){
		msg_send(conn,"getnick",getnickcb,0);
		return;
	}
	asprintf(&mstate.nickname,"%s",msg->args[0]);
	char *str;
	asprintf(&str,"Your nickname: %s",msg->args[0]);
	if(!mstate.nickname||!str)outofmem();
	lgw_add(mstate.lgw,str);
	free(str);
	redraw();
}

struct timeval* populate_fdsets(fd_set *readfds,fd_set *writefds){
	(void)writefds;
	FD_SET(0,readfds);
	struct timeval *tv=malloc(sizeof(struct timeval));
	if(!tv)outofmem();
	tv->tv_sec=5;
	tv->tv_usec=0;
	return tv;
}

void msghandler(ws_conn *conn,const Message *msg){
	(void)conn;
	char *str;
	asprintf(&str,"Unsollicited message received: id=%d typ='%s' nargs=%d",msg->id,msg->typ,msg->nargs);
	if(!str)outofmem();
	lgw_add(mstate.lgw,str);
	free(str);
	redraw();
	// for(int i=0;i<msg->nargs;i++)fprintf(stderr," '%s'",msg->args[i]);
	// fputc('\n',stderr);
}

void fdhandler(ws_conn *conn,const fd_set *readfds,const fd_set *writefds){
	(void)conn;
	(void)writefds;
	if(!FD_ISSET(0,readfds))return;
	int c=getkey();
	char *str;
	asprintf(&str,"%d",c);
	if(!str)outofmem();
	lgw_add(mstate.lgw,str);
	free(str);
	redraw();
	if(c=='q')ws_close(conn);
}

void timeouthandler(ws_conn *conn){
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
	mstate.lgw=lgw_make(35,0,45,15);
	if(!mstate.lgw)outofmem();

	lgw_add(mstate.lgw,"Connecting...");
	redraw();

	ws_context *ctx=ws_init();
	if(!ctx){
		lgw_add(mstate.lgw,"Could not initialise websocket context!");
		redraw();
		getkey();
		return;
	}

	ws_conn *conn=ws_connect(ctx,SERVERHOST,SERVERPORT,"/ws");
	if(!conn){
		ws_destroy(ctx);
		char *str;
		asprintf(&str,"Could not connect to server! (%s:%s)",SERVERHOST,SERVERPORT);
		if(!str)outofmem();
		lgw_add(mstate.lgw,str);
		free(str);
		redraw();
		getkey();
		return;
	}

	lgw_add(mstate.lgw,"Connected.");
	redraw();

	msg_send(conn,"getnick",getnickcb,0);
	msg_runloop(conn,populate_fdsets,msghandler,fdhandler,timeouthandler);

	ws_close(conn);
	ws_destroy(ctx);
}

void startsingleplayer(void){
	const int player=ORDER;

	clearscreen();
	moveto(0,0);
	setbold(true);
	tprintf("Order & Chaos -- Single player");
	setbold(false);

	Board *board=makeboard();

	int curx=0,cury=0;
	bool aiturn=false;
	Move mv;
	int win=-1;
	while(true){
		moveto(2,2);
		tprintboard(board);
		redraw();

		if(win!=-1)break;

		if(aiturn){
			aiturn=false;
			moveto(0,N+3);
			tprintf("Calculating...");
			redraw();
			mv=calcmove(board,!player);
			applymove(board,mv);
			win=checkwin(board);
			if(win!=-1)break;
			moveto(0,N+3);
			tprintf("              ");
			continue;
		}

		moveto(2+2*curx,2+cury);
		redraw();
		int key=getkey();
		int stone;
		switch(key){
			case 'q':
				moveto(0,N+3);
				setbold(true);
				tprintf("Really quit? [y/N] ");
				setbold(false);
				redraw();
				key=getkey();
				if(key=='y'||key=='Y')return;
				moveto(0,N+3);
				tprintf("                  ");
				break;

			case 'h': case KEY_LEFT:
				if(curx>0)curx--; else bel();
				break;

			case 'j': case KEY_DOWN:
				if(cury<N-1)cury++; else bel();
				break;

			case 'k': case KEY_UP:
				if(cury>0)cury--; else bel();
				break;

			case 'l': case KEY_RIGHT:
				if(curx<N-1)curx++; else bel();
				break;

			case 'x':
				stone=XX;
				if(false){
			case 'o':
					stone=OO;
				}
				if(!isempty(board,N*cury+curx)){
					bel();
					break;
				}
				mv.pos=N*cury+curx;
				mv.stone=stone;
				applymove(board,mv);
				win=checkwin(board);
				if(win!=-1)break;
				aiturn=true;
				break;


			default:
				bel();
				break;
		}
	}

	moveto(0,N+3);
	setbold(true);
	const char *plstr=win==ORDER?"Order":"Chaos";
	if(win==player)tprintf("You (%s) won! Congratulations!",plstr);
	else tprintf("The AI (%s) won! Better next time...",plstr);
	setbold(false);
	redraw();
	getkey();
}

void showmenu(int basex,int basey){
	typedef struct Menuitem{
		const char *text;
		char hotkey;
		void (*func)(void);
	} Menuitem;
	static const Menuitem items[]={
		{"Multiplayer online",'m',startmultiplayer},
		{"Single player versus AI",'s',startsingleplayer},
		{"Quit",'q',NULL}
	};
	static const int nitems=sizeof(items)/sizeof(items[0]);

	int choice=0;
	while(true){
		clearscreen();

		moveto(0,0);
		setbold(true);
		tprintf("Order & Chaos");
		setbold(false);
		
		for(int i=0;i<nitems;i++){
			moveto(basex,basey+i);
			tprintf("> %s",items[i].text);
			if(items[i].hotkey!='\0'){
				tprintf(" (");
				setbold(true);
				tputc(items[i].hotkey);
				setbold(false);
				tputc(')');
			}
		}
		moveto(basex,basey+choice);
		redraw();

		bool restartmenu=false;
		while(!restartmenu){
			int key=getkey();
			switch(key){
				case 'j': case KEY_DOWN:
					if(choice<nitems-1){
						choice++;
						moveto(basex,basey+choice);
						redraw();
					} else bel();
					break;

				case 'k': case KEY_UP:
					if(choice>0){
						choice--;
						moveto(basex,basey+choice);
						redraw();
					} else bel();
					break;

				case '\n':
					if(items[choice].func!=NULL){
						items[choice].func();
						restartmenu=true;
					} else return;
					break;

				case 'm':
					startmultiplayer();
					restartmenu=true;
					break;

				case 's':
					startsingleplayer();
					restartmenu=true;
					break;

				case 'q':
					return;

				default:
					bel();
					break;
			}
		}
	}
}

int main(void){
	initkeyboard();
	atexit(endkeyboard);
	initscreen();
	atexit(endscreen);

	installrefreshhandler(true);

	showmenu(2,2);
}
