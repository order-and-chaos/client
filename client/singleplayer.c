#include "singleplayer.h"
#include "termio.h"
#include "tprintboard.h"
#include "../ai.h"
#include "../params.h"

void startsingleplayer(){
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
		int key=tgetkey();
		int stone;
		switch(key){
			case 'q':
				moveto(0,N+3);
				setbold(true);
				tprintf("Really quit? [y/N] ");
				setbold(false);
				redraw();
				key=tgetkey();
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
	tgetkey();
}
