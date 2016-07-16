#include <stdlib.h>
#include <assert.h>

#include "widget.h"
#include "termio.h"
#include "tprintboard.h"
#include "../ai.h"
#include "../params.h"

struct Boardwidget{
	Board *board;
	int basex,basey;
	int curx,cury;
	bool colorcursor;
};

Boardwidget* bdw_make(int basex,int basey,bool colorcursor){
	Boardwidget *bdw=malloc(sizeof(Boardwidget));
	if(!bdw)return NULL;
	Board *board=makeboard();
	if(!board){
		free(bdw);
		return NULL;
	}
	bdw->board=board;
	bdw->basex=basex;
	bdw->basey=basey;
	bdw->curx=bdw->cury=0;
	bdw->colorcursor=colorcursor;
	bdw_redraw(bdw);
	return bdw;
}

void bdw_destroy(Boardwidget *bdw){
	free(bdw->board);
}

void bdw_redraw(Boardwidget *bdw){
	assert(bdw);
	moveto(bdw->basex,bdw->basey);
	tprintboard(bdw->board);
	int x=bdw->basex+2*bdw->curx,y=bdw->basey+bdw->cury;
	moveto(x,y);
	if(bdw->colorcursor){
		setbg(5);
		tputc(getbufferchar(x,y));
		setbg(9);
	}
}

Boardkey bdw_handlekey(Boardwidget *bdw,int key){
	int stone;
	switch(key){
		case 'o':
			stone=OO;
			if(false){
		case 'x':
				stone=XX;
			}
			if(isempty(bdw->board,N*bdw->cury+bdw->curx)){
				Move mv={N*bdw->cury+bdw->curx,stone};
				applymove(bdw->board,mv);
				bdw_redraw(bdw);
			} else bel();
			return BOARDKEY_HANDLED;

		case 'h': case KEY_LEFT:
			if(bdw->curx>0){
				bdw->curx--;
				bdw_redraw(bdw);
			} else bel();
			return BOARDKEY_HANDLED;

		case 'j': case KEY_DOWN:
			if(bdw->cury<N-1){
				bdw->cury++;
				bdw_redraw(bdw);
			} else bel();
			return BOARDKEY_HANDLED;

		case 'k': case KEY_UP:
			if(bdw->cury>0){
				bdw->cury--;
				bdw_redraw(bdw);
			} else bel();
			return BOARDKEY_HANDLED;

		case 'l': case KEY_RIGHT:
			if(bdw->curx<N-1){
				bdw->curx++;
				bdw_redraw(bdw);
			} else bel();
			return BOARDKEY_HANDLED;

		default:
			return BOARDKEY_IGNORED;
	}
}
