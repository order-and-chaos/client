// Order and Chaos AI.  - Tom Smeding

// The board is 6x6, in row-major order. Left-top is index 0.
// One player is Order, the other is Chaos; they take alternate turns, and can both place X'es and
// O's at will (one per turn). Order wins when five of the same stone are in a row (orthogonal or
// diagonal); Chaos wins when the board is full.
// There are no ties in this game.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

//#define NDEBUF
#include <assert.h>

#define ORDER 0
#define CHAOS 1
#define OO 0
#define XX 1

#define MAXDEPTH 2

#define INF 1000000000

#include "winmasks.h"
const uint64_t fullmask=0xfffffffff; //36/4=9 f's

#define APPLY(bitmap,idx) do {(bitmap)|=1ULL<<(idx);} while(0)
#define REMOVE(bitmap,idx) do {(bitmap)&=~(1ULL<<(idx));} while(0)
#define ISEMPTY(bitmap,idx) (!((bitmap)&(1ULL<<(idx))))

inline int max(int a,int b){return b>a?b:a;}
inline int min(int a,int b){return b<a?b:a;}

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

#ifdef NDEBUG
void printboard(Board board){}
#else
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
#endif

int checkwin(const Board *board){
	if(((board->b[0]|board->b[1])&fullmask)==fullmask)return CHAOS;
	for(int i=0;i<nwinmasks;i++){
		if((board->b[0]&winmasks[i])==winmasks[i]||
		   (board->b[1]&winmasks[i])==winmasks[i])return ORDER;
	}
	return -1;
}

int evaluate(const Board *board,int win){
	(void)board; (void)win;
}

// From wikipedia: https://en.wikipedia.org/wiki/Alpha–beta_pruning#Pseudocode
int alphabeta(Board *board,int player,int alpha,int beta,int depth){
	int win=checkwin(board);
	if(depth==0||win!=-1){
		return evaluate(board,win);
	}
	if(player==ORDER){
		int best=-INF;
		for(int p=0;p<36;p++){
			APPLY(board->b[0],p);
			best=max(best,alphabeta(board,!player,alpha,beta,depth-1));
			REMOVE(board->b[0],p);
			alpha=max(alpha,best);
			if(beta<=alpha)break;

			APPLY(board->b[1],p);
			best=max(best,alphabeta(board,!player,alpha,beta,depth-1));
			REMOVE(board->b[1],p);
			alpha=max(alpha,best);
			if(beta<=alpha)break;
		}
		return best;
	} else {
		int best=INF;
		for(int p=0;p<36;p++){
			APPLY(board->b[0],p);
			best=min(best,alphabeta(board,!player,alpha,beta,depth-1));
			REMOVE(board->b[0],p);
			beta=min(beta,best);
			if(beta<=alpha)break;

			APPLY(board->b[1],p);
			best=min(best,alphabeta(board,!player,alpha,beta,depth-1));
			REMOVE(board->b[1],p);
			beta=min(beta,best);
			if(beta<=alpha)break;
		}
		return best;
	}
}

// Positive scores are good for Order, negative are good for Chaos.
Move calcmove(Board *board,int player){
	int bestscore=player==ORDER?-INF:INF,bestat=-1,beststone=-1;
	int score;
	for(int p=0;p<36;p++){
		if(!ISEMPTY(board->b[0]|board->b[1],p))continue;

		APPLY(board->b[0],p);
		score=alphabeta(board,!player,-INF,INF,MAXDEPTH);
		if(player==ORDER?score>bestscore:score<bestscore){
			bestat=p;
			beststone=0;
		}
		REMOVE(board->b[0],p);

		APPLY(board->b[1],p);
		score=alphabeta(board,!player,-INF,INF,MAXDEPTH);
		if(player==ORDER?score>bestscore:score<bestscore){
			bestat=p;
			beststone=1;
		}
		REMOVE(board->b[1],p);
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
	char c=getchar();
	if(c=='Q')return 0;
	int me=c=='C';
	bool start=getchar()=='1';
	scanf("%*[^\n]"); getchar();
	if(feof(stdin))return 0;

	Board board={{0,0}};

	if(start){
		printf("X 14\n");
		APPLY(board.b[XX],14);
		printboard(board);
	}

	while(true){
		char c=getchar();
		if(c=='Q'||feof(stdin))break;
		bool stone=c=='X';
		int pos;
		scanf(" %d",&pos); getchar();
		assert(ISEMPTY(board.b[stone],pos));
		APPLY(board.b[stone],pos);
		printboard(board);
		int win=checkwin(&board);
		if(win!=-1)break;

		Move mv=calcmove(&board,me);
		assert(ISEMPTY(board.b[mv.stone],mv.pos));
		APPLY(board.b[mv.stone],mv.pos);
		printboard(board);
	}
	return 0;
}

int main(void){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	srandom(tv.tv_sec*1000000+tv.tv_usec);

	return terminalio();
}
