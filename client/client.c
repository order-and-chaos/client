#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include "termio.h"
#include "../ai.h"


void tprintboard(const Board *board){
	uint64_t b0=board->b[0],b1=board->b[1];
	for(int y=0;y<6;y++){
		for(int x=0;x<6;x++){
			tputc(".OX"[(b0&1)+2*(b1&1)]);
			tputc(' ');
			b0>>=1;
			b1>>=1;
		}
		tputc('\n');
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
	startmultiplayer();
}

void showmenu(int basex,int basey){
	typedef struct Menuitem{
		const char *text;
		void (*func)(void);
	} Menuitem;
	static const Menuitem items[]={
		{"Multiplayer online",startmultiplayer},
		{"Single player versus AI",startsingleplayer},
		{"Quit",NULL}
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

	showmenu(2,2);
}
