// Order and Chaos AI: Minimax/alpha-beta pruning.  - Tom Smeding

#include "params.h"

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

static int max(int a,int b){return b>a?b:a;}
static int min(int a,int b){return b<a?b:a;}

static int reduceabs(int a,int d){return a>d?a-d:a<-d?a+d:0;}

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

// High scores are good for Order, low are good for Chaos.
static int evaluate(const Board *board,int win){
	if(win!=-1){
		return 200*(1-2*win); // positive for Order, negative for Chaos
	}

	int x,y,i,co1,cx1,co2,cx2;
	uint64_t m;
	int score=0;

	// Horizontal (1), vertical (2)
	for(y=0;y<N;y++)for(x=0;x<2;x++){
		co1=cx1=co2=cx2=0;
		for(i=0;i<N-1;i++){
			m=1ULL<<(N*y+x+i); // horizontal
			co1+=(board->b[0]&m)!=0;
			cx1+=(board->b[1]&m)!=0;

			m=1ULL<<(N*(x+i)+y); //vertical
			co2+=(board->b[0]&m)!=0;
			cx2+=(board->b[1]&m)!=0;
		}

		if(co1==0)score+=cx1;
		if(cx1==0)score+=co1;

		if(co2==0)score+=cx2;
		if(cx2==0)score+=co2;
	}

	// Diagonal \ (1), / (2)
	for(y=0;y<2;y++)for(x=0;x<2;x++){
		co1=cx1=co2=cx2=0;
		for(i=0;i<N-1;i++){
			m=1ULL<<(N*(y+i)+x+i); // \.
			co1+=(board->b[0]&m)!=0;
			cx1+=(board->b[1]&m)!=0;

			m=1ULL<<(N*(N-1-y-i)+x+i); // /
			co2+=(board->b[0]&m)!=0;
			cx2+=(board->b[1]&m)!=0;
		}

		if(co1==0)score+=cx1;
		if(cx1==0)score+=co1;

		if(co2==0)score+=cx2;
		if(cx2==0)score+=co2;
	}

	return score;
}

#define HASHSZ 1000003

typedef struct Hashitem{
	Board board;
	int player,alpha,beta,depth;
	int score;
} Hashitem;

Hashitem boardhash[HASHSZ]={{{{0,0}},0,0,0,0,0}};

static int hash(const Board *board){
	return (board->b[0]^(board->b[1]<<28))%HASHSZ;
}

// From Wikipedia: https://en.wikipedia.org/wiki/Alpha–beta_pruning#Pseudocode
static int alphabeta(Board *board,int player,int alpha,int beta,int depth){
	assert(board->b[0]!=0||board->b[1]!=0);

	const int hh=hash(board);
	Hashitem *hi=boardhash+hh;
	if(hi->board.b[0]==board->b[0]&&hi->board.b[1]==board->b[1]&&
	   hi->player==player&&hi->alpha==alpha&&hi->beta==beta&&hi->depth==depth){
		return hi->score;
	}

#define RETURN_STORE(sc) do { \
		hi->board=*board; \
		hi->player=player; hi->alpha=alpha; hi->beta=beta; hi->depth=depth; \
		return hi->score=sc; \
	} while(0)
//#define RETURN_STORE(sc) return sc

	int win=checkwin(board);
	if(depth==0||win!=-1){
		RETURN_STORE(evaluate(board,win));
	}

	int best;
	if(player==ORDER){
		best=-INF;
		for(int p=0;p<N*N;p++){
			if(!ISEMPTY(board->b[0]|board->b[1],p))continue;
			APPLY(board->b[0],p);
			best=max(best,reduceabs(alphabeta(board,!player,alpha,beta,depth-1),1));
			REMOVE(board->b[0],p);
			alpha=max(alpha,best);
			if(beta<=alpha)RETURN_STORE(INF); // exact value doesn't matter, but >beta

			APPLY(board->b[1],p);
			best=max(best,reduceabs(alphabeta(board,!player,alpha,beta,depth-1),1));
			REMOVE(board->b[1],p);
			alpha=max(alpha,best);
			if(beta<=alpha)RETURN_STORE(INF); // exact value doesn't matter, but >beta
		}
	} else {
		best=INF;
		for(int p=0;p<N*N;p++){
			if(!ISEMPTY(board->b[0]|board->b[1],p))continue;
			APPLY(board->b[0],p);
			best=min(best,reduceabs(alphabeta(board,!player,alpha,beta,depth-1),1));
			REMOVE(board->b[0],p);
			beta=min(beta,best);
			if(beta<=alpha)RETURN_STORE(-INF); // exact value doesn't matter, but <alpha

			APPLY(board->b[1],p);
			best=min(best,reduceabs(alphabeta(board,!player,alpha,beta,depth-1),1));
			REMOVE(board->b[1],p);
			beta=min(beta,best);
			if(beta<=alpha)RETURN_STORE(-INF); // exact value doesn't matter, but <alpha
		}
	}

	RETURN_STORE(best);

#undef RETURN_STORE
}

typedef struct Moveorderitem {
	int p,stone,score;
} Moveorderitem;

static int moveorderitem_compare(const void *a,const void *b){
	return ((Moveorderitem*)b)->score-((Moveorderitem*)a)->score;
}

Move calcmove(Board *board,int player){
	/*if(board->b[0]==0&&board->b[1]==0){
		Move mv={player==ORDER?N*(N/2)+N/2:1,XX};
		return mv;
	}*/

	Moveorderitem mvs[72];
	int nmvs=0;
	int win;
	for(int p=0;p<N*N;p++){
		if(!ISEMPTY(board->b[0]|board->b[1],p))continue;

		for(int stone=0;stone<2;stone++){
			mvs[nmvs].p=p;
			mvs[nmvs].stone=stone;
			APPLY(board->b[stone],p);
			win=checkwin(board);
			if(win==player){
				REMOVE(board->b[stone],p);
				Move mv={p,stone};
				return mv;
			}
			mvs[nmvs].score=evaluate(board,win)*(1-2*player);
			REMOVE(board->b[stone],p);
			nmvs++;
		}
	}

	qsort(mvs,nmvs,sizeof(Moveorderitem),moveorderitem_compare);

	/*Moveorderitem mvs[6]={{12,0,0},{0,0,0},{1,0,0},{2,0,0},{6,0,0},{7,0,0}};
	int nmvs=6;
	Moveorderitem mvs[3]={{0,0,0},{1,0,0},{5,0,0}};
	int nmvs=3;*/

	// FILE *f=fopen("log.txt","w");
	int bestscore=player==ORDER?-INF:INF,bestat=-1,beststone=-1;
	int score;
	for(int i=0;i<nmvs;i++){
		int p=mvs[i].p;
		int stone=mvs[i].stone;

		// fprintf(f,"%d %c: ",p,"OX"[stone]);

		APPLY(board->b[stone],p);
		score=alphabeta(board,!player,player==ORDER?bestscore:-INF,player==CHAOS?bestscore:INF,MMAB_MAXDEPTH);
		// score=alphabeta(board,!player,-INF,INF,MMAB_MAXDEPTH);
		REMOVE(board->b[stone],p);
		// fprintf(f,"%03d (p=%d, s=%c)\n",score,p,"OX"[stone]); fflush(f);
		if(player==ORDER?score>bestscore:score<bestscore){
			bestat=p;
			beststone=stone;
			bestscore=score;
		}
	}
	// fclose(f);

	Move mv={bestat,beststone};
	return mv;
}
