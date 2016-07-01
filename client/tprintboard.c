#include "termio.h"

#include "tprintboard.h"
#include "../params.h"

void tprintboard(const Board *board){
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
