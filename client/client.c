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

int main(void){
	initkeyboard();
	atexit(endkeyboard);
	initscreen();
	atexit(endscreen);

	setbold(true);
	tprint("Order & Chaos xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\na\na\na\na\na\na\na\na\na\na\na\na\n");
	setbold(false);

	Board *bd=makeboard();
	tprintboard(bd);
	redraw();
	getkey();
	redraw();
	getkey();
}
