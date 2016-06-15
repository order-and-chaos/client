#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include "termio.h"
#include "../ai.h"
#include "../params.h"


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

void startmultiplayer(void){
	moveto(0,6);
	setbold(true);
	tprintf("Feature not implemented.");
	setbold(false);
	redraw();
	getkey();
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
