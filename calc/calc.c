// There are 2^(N^2) * (N^2)! possible games.

//TODO: find out asm difference between 1-p and !p

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>

#if LONG_MAX != LLONG_MAX
#error Please use a 64-bit compiler
#endif

#define malloc(n,t) ((t*)malloc((n)*sizeof(t)))
#define calloc(n,t) ((t*)calloc((n),sizeof(t)))
#define realloc(p,n,t) ((t*)realloc((p),(n)*sizeof(t)))

int ilog2(int64_t i){
	if(i<=0)return -1;
	int l=1;
	while(i>>=1)l++;
	return l;
}

#define N (5)
#define Nm1 (N-1)

typedef int64_t Board[2]; //bit 0 is origin in left-top; row-major order

#define BSET(i,p) ((i)|=1L<<(p))
#define BRESET(i,p) ((i)&=~(1L<<(p)))
#define BGET(i,p) (((i)>>(p))&1)
#define FULL(i) ((i)==(1L<<(N*N))-1)

void printboard(const Board board,int indent){
	for(int y=0;y<N;y++){
		for(int i=0;i<indent*2;i++)putchar(' ');
		for(int x=0;x<N;x++){
			if(x!=0)putchar(' ');
			putchar(".XO#"[BGET(board[0],N*y+x)+2*BGET(board[1],N*y+x)]);
		}
		putchar('\n');
	}
}

bool boardsimilar(const Board b1,const Board b2){
	return memcmp(b1,b2,sizeof(Board))==0;
}

#define NWINMASKS (4*N+8)
int64_t winmasks[NWINMASKS];

void genwinmasks(void){
	int idx=0;
	//horizontal
	int64_t m=(1L<<Nm1)-1;
	for(int i=0;i<N;i++){
		winmasks[idx++]=m;
		winmasks[idx++]=m<<1;
		m<<=N;
	}

	//vertical
	m=0;
	for(int i=0;i<Nm1;i++)m|=1L<<(i*N);
	for(int i=0;i<N;i++){
		winmasks[idx++]=m;
		winmasks[idx++]=m<<N;
		m<<=1;
	}
	
	//diagonal \.
	m=0;
	for(int i=0;i<Nm1;i++)m|=1L<<(i*(N+1));
	winmasks[idx++]=m;
	winmasks[idx++]=m<<1;
	winmasks[idx++]=m<<N;
	winmasks[idx++]=m<<(N+1);

	//diagonal /
	m=0;
	for(int i=0;i<Nm1;i++)m|=1L<<(N-2+i*Nm1);
	winmasks[idx++]=m;
	winmasks[idx++]=m<<1;
	winmasks[idx++]=m<<N;
	winmasks[idx++]=m<<(N+1);

	assert(idx==NWINMASKS);
}


#define HASHSZ (22369601)
// #define HASHSZ (1)

typedef struct Hashitem{
	Board board;
	bool orderwin;
} Hashitem;

Hashitem boardhash[HASHSZ];

int hash(const Board board){
	return (board[0]^((board[1]<<17)|(board[1]>>(64-17))))%HASHSZ;
}


bool checkwinorder(const Board board){
	int64_t negx=~board[0],nego=~board[1];
	//TODO: check whether unrolling gives an optimisation
	for(int i=0;i<NWINMASKS;i++){
		if((winmasks[i]&negx)==0||(winmasks[i]&nego)==0)return true;
	}
	return false;
}

#define PRINTDEPTH 2

bool calcmovechaos(Board board,int depth);

bool calcmoveorder(Board board,int depth){
	// printf("Entering order (depth=%d)\n",depth);
	int h=hash(board);
	if(boardsimilar(board,boardhash[h].board)){
		if(depth<=2){
			printf("(or) Hash retrieved: h=%d orderwin=%d\n",h,boardhash[h].orderwin);
			printboard(board,0);
		}
		return boardhash[h].orderwin;
	} else if(boardhash[h].board[0]||boardhash[h].board[1]){
		//printf("(or) COLLISION! h=%d\n",h);
	}
	int64_t taken=board[0]|board[1];
	int i=0;
	for(int64_t m=1;m!=1L<<(N*N);m<<=1,taken>>=1,i++){
		if(taken&1)continue;
		if(depth<=PRINTDEPTH){
			for(int d=0;d<2*depth;d++)putchar(' '); printf("i=%d (or)\n",i);
		}

		board[0]|=m;
		if(checkwinorder(board)||!calcmovechaos(board,depth+1)){
			if(depth<=PRINTDEPTH){
				printboard(board,depth);
				for(int d=0;d<2*depth;d++)putchar(' '); printf("-> true (or, 0; win=%d)\n",checkwinorder(board));
			}
			board[0]&=~m;
			boardhash[h].board[0]=board[0];
			boardhash[h].board[1]=board[1];
			boardhash[h].orderwin=true;
			return true;
		}
		board[0]&=~m;

		board[1]|=m;
		if(checkwinorder(board)||!calcmovechaos(board,depth+1)){
			if(depth<=PRINTDEPTH){
				printboard(board,depth);
				for(int d=0;d<2*depth;d++)putchar(' '); printf("-> true (or, 1; win=%d)\n",checkwinorder(board));
			}
			board[1]&=~m;
			boardhash[h].board[0]=board[0];
			boardhash[h].board[1]=board[1];
			boardhash[h].orderwin=true;
			return true;
		}
		board[1]&=~m;
	}
	boardhash[h].board[0]=board[0];
	boardhash[h].board[1]=board[1];
	boardhash[h].orderwin=false;
	if(depth<=PRINTDEPTH){
		for(int d=0;d<2*depth;d++)putchar(' '); printf("-> false (or)\n");
	}
	return false;
}

bool calcmovechaos(Board board,int depth){
	// printf("Entering chaos (depth=%d)\n",depth);
	int h=hash(board);
	if(boardsimilar(board,boardhash[h].board)){
		if(depth<=2){
			printf("(ch) Hash retrieved: h=%d orderwin=%d\n",h,boardhash[h].orderwin);
			printboard(board,0);
		}
		return !boardhash[h].orderwin;
	} else if(boardhash[h].board[0]||boardhash[h].board[1]){
		//printf("(ch) COLLISION! h=%d\n",h);
	}
	boardhash[h].board[0]=board[0];
	boardhash[h].board[1]=board[1];
	int64_t taken=board[0]|board[1];
	int i=0;
	for(int64_t m=1;m!=1L<<(N*N);m<<=1,taken>>=1,i++){
		if(taken&1)continue;
		if(depth<=PRINTDEPTH){
			for(int d=0;d<2*depth;d++)putchar(' '); printf("i=%d (ch)\n",i);
		}

		board[0]|=m;
		if(!checkwinorder(board)&&(FULL(board[0]|board[1])||!calcmoveorder(board,depth+1))){
			if(depth<=PRINTDEPTH){
				printboard(board,depth);
				for(int d=0;d<2*depth;d++)putchar(' '); printf("-> true (ch, 0)\n");
			}
			board[0]&=~m;
			boardhash[h].board[0]=board[0];
			boardhash[h].board[1]=board[1];
			boardhash[h].orderwin=false;
			return true;
		}
		board[0]&=~m;

		board[1]|=m;
		if(!checkwinorder(board)&&(FULL(board[0]|board[1])||!calcmoveorder(board,depth+1))){
			if(depth<=PRINTDEPTH){
				printboard(board,depth);
				for(int d=0;d<2*depth;d++)putchar(' '); printf("-> true (ch, 1)\n");
			}
			board[1]&=~m;
			boardhash[h].board[0]=board[0];
			boardhash[h].board[1]=board[1];
			boardhash[h].orderwin=false;
			return true;
		}
		board[1]&=~m;
	}
	boardhash[h].board[0]=board[0];
	boardhash[h].board[1]=board[1];
	boardhash[h].orderwin=true;
	if(depth<=PRINTDEPTH){
		printboard(board,depth);
		for(int d=0;d<2*depth;d++)putchar(' '); printf("-> false (ch)\n");
	}
	return false;
}

bool calcmove(Board board,int depth,int player){
	if(player==0)return calcmoveorder(board,depth);
	else return calcmovechaos(board,depth);
}

void play(void){
	Board board={0,0};
	while(true){
		int i;
		for(i=0;i<N*N;i++){
			if(BGET(board[0]|board[1],i))continue;
			BSET(board[0],i);
			if(!calcmovechaos(board,1)){
				printf("AI move: X %d\n",i);
				break;
			}
			BRESET(board[0],i);
			BSET(board[1],i);
			if(!calcmovechaos(board,1)){
				printf("AI move: O %d\n",i);
				break;
			}
			BRESET(board[1],i);
		}
		if(i==N*N){
			printf("AI thinks it has lost\n");
			break;
		}
		printboard(board,0);
		printf("Your move > "); fflush(stdout);
		char stone;
		int mv;
		scanf(" %c %d",&stone,&mv);
		if(feof(stdin))break;
		assert(stone=='X'||stone=='O');
		assert(mv>=0&&mv<N*N);
		assert(BGET(board[0]|board[1],mv)==0);
		BSET(board[stone=='O'],mv);
		printboard(board,0);
	}
}

int main(void){
	genwinmasks();
	/*for(int i=0;i<NWINMASKS;i++){
		Board board={winmasks[i],winmasks[i]};
		printboard(board,0);
		putchar('\n');
	}*/

	//play();

	Board board={0,0};
	int p=0;
	printboard(board,0);
	int i;
	for(i=0;i<N*N;i++){
		if(BGET(board[0]|board[1],i))continue;
		printf("i=%d\n",i);
		BSET(board[0],i);
		if(!calcmove(board,1,!p)){
			printf("Move %d X\n",i);
		}
		BRESET(board[0],i);
	}

	/*Board board={0,0};
	BSET(board[0],0);
	printf("%d\n",calcmovechaos(board,1));
	BRESET(board[0],0);*/

	/*BSET(board[0],0);
	BSET(board[1],1);
	BSET(board[0],0);
	printf("%d\n",checkwinorder(board));*/
}
