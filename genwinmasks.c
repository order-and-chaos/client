// Generate winmasks.h for ai.c.

#include <stdio.h>
#include <stdint.h>

#define APPLY(b,i) do {b|=1ULL<<(i);} while(0)
#define APPLYXY(b,x,y) APPLY(b,6*(y)+(x))

#define PRINTMASK(b) do {printf("\t0x%llx,\n",(b));} while(0)

int main(void){
	printf("#pragma once\n\nconst uint64_t winmasks[]={\n");

	//horizontal & vertical
	for(int i=0;i<6;i++)for(int j=0;j<2;j++){
		uint64_t hor=0,ver=0;
		for(int k=0;k<5;k++){
			APPLYXY(hor,j+k,i);
			APPLYXY(ver,i,j+k);
		}
		PRINTMASK(hor);
		PRINTMASK(ver);
	}

	//both diagonals, \ then /
	for(int y=0;y<2;y++)for(int x=0;x<2;x++){
		uint64_t d1=0,d2=0;
		for(int k=0;k<5;k++){
			APPLYXY(d1,x+k,y+k);
			APPLYXY(d2,x+k,5-y-k);
		}
		PRINTMASK(d1);
		PRINTMASK(d2);
	}

	printf("};\n\nconst int nwinmasks=sizeof(winmasks)/sizeof(winmasks[0]);\n");
}
