// Order and Chaos AI: Monte Carlo.  - Tom Smeding

#include "params.h"

#define _XOPEN_SOURCE 700  // random
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "ai.h"

#ifndef ASSERTS
#define NDEBUG
#endif
#include <assert.h>

#define INF 1000000000 //1e9

#include "winmasks.h"
const uint64_t fullmask=(1ULL<<(N*N))-1;

#define APPLY(bitmap,idx) do {(bitmap)|=1ULL<<(idx);} while(0)
#define REMOVE(bitmap,idx) do {(bitmap)&=~(1ULL<<(idx));} while(0)
#define ISEMPTY(bitmap,idx) (!((bitmap)&(1ULL<<(idx))))

Board* makeboard(void){
	return calloc(1,sizeof(Board));
}

void applymove(Board *board,Move mv){
	APPLY(board->b[mv.stone],mv.pos);
}

bool isempty(const Board *board,int pos){
	return ISEMPTY(board->b[0]|board->b[1],pos);
}

void printboard(const Board *board){
	uint64_t b0=board->b[0],b1=board->b[1];
	for(int y=0;y<N;y++){
		for(int x=0;x<N;x++){
			fputc(".OX"[(b0&1)+2*(b1&1)],stderr);
			fputc(' ',stderr);
			b0>>=1;
			b1>>=1;
		}
		fputc('\n',stderr);
	}
}

int checkwin(const Board *board){
	for(int i=0;i<nwinmasks;i++){
		if((board->b[0]&winmasks[i])==winmasks[i]||
		   (board->b[1]&winmasks[i])==winmasks[i])return ORDER;
	}
	if(((board->b[0]|board->b[1])&fullmask)==fullmask)return CHAOS;
	return -1;
}

static int playthrough(Board board,int player){
	(void)player;
	int win;
	Move poss[2*N*N];
	int nposs;
	while(true){
		win=checkwin(&board);
		if(win!=-1)return 1-2*win;

		nposs=0;
		for(int p=0;p<N*N;p++){
			for(int stone=0;stone<2;stone++){
				if(!ISEMPTY(board.b[0]|board.b[1],p))continue;
				poss[nposs].pos=p;
				poss[nposs++].stone=stone;
			}
		}
		assert(nposs>0); //else Chaos would've won

		int i=random()%nposs;
		APPLY(board.b[poss[i].stone],poss[i].pos);
	}
}

Move calcmove(Board *board,int player){
	if(board->b[0]==0&&board->b[1]==0){
		Move mv={player==ORDER?N*(N/2)+N/2:1,XX};
		return mv;
	}

	int score,best,bestp=-1,beststone=-1;
	for(int p=0;p<N*N;p++){
		if(!ISEMPTY(board->b[0]|board->b[1],p))continue;
		
		for(int stone=0;stone<2;stone++){
			score=0;
			Board b2=*board;
			APPLY(b2.b[stone],p);
			for(int i=0;i<MC_PLAYTHROUGHS;i++){
				score+=playthrough(b2,!player);
			}
			if(bestp==-1||(player==ORDER?score>best:score<best)){
				best=score;
				bestp=p;
				beststone=stone;
			}
		}
	}
	
	Move mv={bestp,beststone};
	return mv;
}
