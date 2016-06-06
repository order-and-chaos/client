// Order and Chaos AI.  - Tom Smeding

// The board is 6x6, in row-major order. Left-top is index 0.
// One player is Order, the other is Chaos; they take alternate turns, and can both place X'es and
// O's at will (one per turn). Order wins when five of the same stone are in a row (orthogonal or
// diagonal); Chaos wins when the board is full.
// There are no ties in this game.

#include "params.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#ifndef ASSERTS
#define NDEBUG
#endif
#include <assert.h>

#define ORDER 0
#define CHAOS 1
#define OO 0
#define XX 1

#define INF 1000000000 //1e9

#include "winmasks.h"
const uint64_t fullmask=0xfffffffff; //36/4=9 f's

#define APPLY(bitmap,idx) do {(bitmap)|=1ULL<<(idx);} while(0)
#define REMOVE(bitmap,idx) do {(bitmap)&=~(1ULL<<(idx));} while(0)
#define ISEMPTY(bitmap,idx) (!((bitmap)&(1ULL<<(idx))))

int max(int a,int b){return b>a?b:a;}
int min(int a,int b){return b<a?b:a;}

int reduceabs(int a,int d){return a>d?a-d:a<-d?a+d:0;}

typedef struct Board{
	uint64_t b[2];
} Board;

typedef struct Move{
	int32_t pos,stone;
} Move;

Board* makeboard(void){
	return calloc(1,sizeof(Board));
}

void applymove(Board *board,Move mv){
	APPLY(board->b[mv.stone],mv.pos);
}

void printboard(Board board){
	for(int y=0;y<6;y++){
		for(int x=0;x<6;x++){
			fputc(".OX"[(board.b[0]&1)+2*(board.b[1]&1)],stderr);
			fputc(' ',stderr);
			board.b[0]>>=1;
			board.b[1]>>=1;
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

int evaluate(const Board *board,int win){
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
int alphabeta(Board *board,int player,int alpha,int beta,int depth){
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

int moveorderitem_compare(const void *a,const void *b){
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

	Board board={{0,0}};

	if(start){
		printf("X 14\n");
		APPLY(board.b[XX],14);
#ifdef PRINTBOARD
		printboard(board);
#endif
	}

	int win;
	while(true){
		char c=getchar();
		if(c=='Q'||c=='q'||feof(stdin))break;
		bool stone=c=='X'||c=='x';
		int pos;
		scanf(" %d",&pos); getchar();
		bool isvalid=ISEMPTY(board.b[0]|board.b[1],pos);
		if(tty){
			if(!isvalid){
				fprintf(stderr,"Invalid move!\n");
				continue;
			}
		} else assert(isvalid);
		APPLY(board.b[stone],pos);
#ifdef PRINTBOARD
		printboard(board);
#endif
		win=checkwin(&board);
		if(win!=-1)break;

		Move mv=calcmove(&board,me);
		assert(ISEMPTY(board.b[0]|board.b[1],mv.pos));
		APPLY(board.b[mv.stone],mv.pos);
		printf("%c %d\n","OX"[mv.stone],mv.pos);
#ifdef PRINTBOARD
		printboard(board);
#endif
		win=checkwin(&board);
		if(win!=-1)break;
	}
	switch(win){
		case ORDER: fprintf(stderr,"%s (Order) won!\n",me==win?"I":"You"); break;
		case CHAOS: fprintf(stderr,"%s (Chaos) won!\n",me==win?"I":"You"); break;
	}
	return 0;
}

int main(void){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	srandom(tv.tv_sec*1000000+tv.tv_usec);

	return terminalio();
}
