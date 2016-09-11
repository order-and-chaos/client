#include "params.h"

#define _XOPEN_SOURCE 700  // srandom
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>

#include "ai.h"

#ifndef ASSERTS
#define NDEBUG
#endif
#include <assert.h>

void winreport(int me,int win){
	switch(win){
		case ORDER: fprintf(stderr,"%s (Order) won!\n",me==win?"I":"You"); break;
		case CHAOS: fprintf(stderr,"%s (Chaos) won!\n",me==win?"I":"You"); break;
	}
}

// First input line: matches /[OC][12]/ where O/C mean Order/Chaos, 1/2 mean first/second player
// Afterwards, all I/O lines are /[XO] [0-9]+/, where X/O mean the choice of stone, and the number
// is the location on the board. Input is move by opponent, output is move of the program. (Left-
// top is 0, row-major order.)
// When the program should stop, input /Q/. The program should stop automatically when either
// player has won.
int terminalio(void){
	bool tty=isatty(0);

	char c=getchar();
	if(c=='Q'||c=='q')return 0;
	int me=c=='C'||c=='c';
	bool start=getchar()=='1';
	scanf("%*[^\n]"); getchar();
	if(feof(stdin))return 0;

	Board *board=makeboard();

	int win=checkwin(board);
	if(win!=-1){
		winreport(me,win);
		return 0;
	}

	Move mv;

	if(start){
		mv.stone=XX;
		mv.pos=N*(N/2)+N/2;
		/*mv=calcmove(board,me);
		printf("X %d\n",mv.pos);*/
		applymove(board,mv);
#ifdef PRINTBOARD
		printboard(board); fputc('\n',stderr);
#endif
	}

	while(true){
		char c=getchar();
		if(c=='Q'||c=='q'||feof(stdin))break;
		bool stone=c=='X'||c=='x';
		int pos;
		scanf(" %d",&pos); getchar();
		bool isvalid=isempty(board,pos);
		if(tty){
			if(!isvalid){
				fprintf(stderr,"Invalid move!\n");
				continue;
			}
		} else assert(isvalid);
		mv.stone=stone;
		mv.pos=pos;
		applymove(board,mv);
#ifdef PRINTBOARD
		printboard(board); fputc('\n',stderr);
#endif
		win=checkwin(board);
		if(win!=-1)break;

		mv=calcmove(board,me);
		assert(isempty(board,mv.pos));
		applymove(board,mv);
		printf("%c %d\n","OX"[mv.stone],mv.pos);
#ifdef PRINTBOARD
		printboard(board); fputc('\n',stderr);
#endif
		win=checkwin(board);
		if(win!=-1)break;
	}
	winreport(me,win);

	free(board);
	return 0;
}

int main(void){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	srandom(tv.tv_sec*1000000+tv.tv_usec);

	/*Board *board=makeboard();
	Move mv=calcmove(board,ORDER);
	printf("mv={pos=%d,stone=%c}\n",mv.pos,"OX"[mv.stone]);
	return 0;*/

	return terminalio();
}
