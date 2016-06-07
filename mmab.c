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
const uint64_t fullmask=0xfffffffff; //36/4=9 f's

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
	for(int y=0;y<6;y++){
		for(int x=0;x<6;x++){
			fputc(".OX"[(b0&1)+2*(b1&1)],stderr);
			fputc(' ',stderr);
			b0>>=1;
			b1>>=1;
		}
		fputc('\n',stderr);
	}
	fputc('\n',stderr);
}

int checkwin(const Board *board){
	if(((board->b[0]|board->b[1])&fullmask)==fullmask)return CHAOS;
	for(int i=0;i<nwinmasks;i++){
		if((board->b[0]&winmasks[i])==winmasks[i]||
		   (board->b[1]&winmasks[i])==winmasks[i])return ORDER;
	}
	return -1;
}

static int evaluate(const Board *board,int win){
	if(win!=-1){
		return 150*(1-2*win); // positive for Order, negative for Chaos
	}

	int x,y,i,co1,cx1,co2,cx2;
	uint64_t m;
	int score=0;

	// Horizontal (1), vertical (2)
	for(y=0;y<6;y++)for(x=0;x<2;x++){
		co1=cx1=co2=cx2=0;
		for(i=0;i<5;i++){
			m=1ULL<<(6*y+x+i); // horizontal
			co1+=(board->b[0]&m)!=0;
			cx1+=(board->b[1]&m)!=0;

			m=1ULL<<(6*(x+i)+y); //vertical
			co2+=(board->b[0]&m)!=0;
			cx2+=(board->b[1]&m)!=0;
		}

		if(co1==0)score+=cx1*cx1;
		if(cx1==0)score+=co1*co1;

		if(co2==0)score+=cx2*cx2;
		if(cx2==0)score+=co2*co2;
	}

	// Diagonal \ (1), / (2)
	for(y=0;y<2;y++)for(x=0;x<2;x++){
		co1=cx1=co2=cx2=0;
		for(i=0;i<5;i++){
			m=1ULL<<(6*(y+i)+x+i); // \.
			co1+=(board->b[0]&m)!=0;
			cx1+=(board->b[1]&m)!=0;

			m=1ULL<<(6*(5-y-i)+x+i); // /
			co2+=(board->b[0]&m)!=0;
			cx2+=(board->b[1]&m)!=0;
		}

		if(co1==0)score+=cx1*cx1;
		if(cx1==0)score+=co1*co1;

		if(co2==0)score+=cx2*cx2;
		if(cx2==0)score+=co2*co2;
	}

	return score;
}

// From Wikipedia: https://en.wikipedia.org/wiki/Alpha–beta_pruning#Pseudocode
static int alphabeta(Board *board,int player,int alpha,int beta,int depth){
	assert(board->b[0]!=0||board->b[1]!=0);

	int win=checkwin(board);
	if(depth==0||win!=-1){
		return evaluate(board,win);
	}

	int best;
	if(player==ORDER){
		best=-INF;
		for(int p=0;p<36;p++){
			if(!ISEMPTY(board->b[0]|board->b[1],p))continue;
			APPLY(board->b[0],p);
			best=max(best,reduceabs(alphabeta(board,!player,alpha,beta,depth-1),1));
			REMOVE(board->b[0],p);
			alpha=max(alpha,best);
			if(beta<=alpha)break;

			APPLY(board->b[1],p);
			best=max(best,reduceabs(alphabeta(board,!player,alpha,beta,depth-1),1));
			REMOVE(board->b[1],p);
			alpha=max(alpha,best);
			if(beta<=alpha)break;
		}
	} else {
		best=INF;
		for(int p=0;p<36;p++){
			if(!ISEMPTY(board->b[0]|board->b[1],p))continue;
			APPLY(board->b[0],p);
			best=min(best,reduceabs(alphabeta(board,!player,alpha,beta,depth-1),1));
			REMOVE(board->b[0],p);
			beta=min(beta,best);
			if(beta<=alpha)break;

			APPLY(board->b[1],p);
			best=min(best,reduceabs(alphabeta(board,!player,alpha,beta,depth-1),1));
			REMOVE(board->b[1],p);
			beta=min(beta,best);
			if(beta<=alpha)break;
		}
	}

	return best;
}

typedef struct Moveorderitem {
	int p,stone,score;
} Moveorderitem;

static int moveorderitem_compare(const void *a,const void *b){
	return ((Moveorderitem*)b)->score-((Moveorderitem*)a)->score;
}

// High scores are good for Order, low are good for Chaos.
Move calcmove(Board *board,int player){
	int bestscore=player==ORDER?-INF:INF,bestat=-1,beststone=-1;
	int score;
	Moveorderitem mvs[72];
	int nmvs=0;
	int win;
	for(int p=0;p<36;p++){
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
			mvs[nmvs].score=evaluate(board,win);
			REMOVE(board->b[stone],p);
			nmvs++;
		}
	}

	qsort(mvs,nmvs,sizeof(Moveorderitem),moveorderitem_compare);

	for(int i=0;i<nmvs;i++){
		int p=mvs[i].p;
		int stone=mvs[i].stone;

		APPLY(board->b[stone],p);
		score=alphabeta(board,!player,-INF,INF,MAXDEPTH);
		REMOVE(board->b[stone],p);
		if(player==ORDER?score>bestscore:score<bestscore){
			bestat=p;
			beststone=stone;
			bestscore=score;
		}
	}

	Move mv={bestat,beststone};
	return mv;
}
