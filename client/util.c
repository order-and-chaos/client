#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "termio.h"

__attribute__((noreturn)) void outofmem(void){
	endkeyboard();
	endscreen();
	printf("OUT OF MEMORY!\n");
	exit(1);
}

char *astrcpy(const char *src) {
	assert(src);
	int len=strlen(src);
	char *buf=malloc(len+1);
	assert(buf);
	memcpy(buf,src,len+1);
	return buf;
}
