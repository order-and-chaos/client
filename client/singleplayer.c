#include "singleplayer.h"
#include "termio.h"
#include "../ai.h"
#include "../params.h"

static void tprintboard(const Board *board){
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
