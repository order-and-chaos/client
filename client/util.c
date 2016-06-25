#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "termio.h"

__attribute__((noreturn)) void outofmem(void){
	endkeyboard();
	endscreen();
	printf("OUT OF MEMORY!\n");
	exit(1);
}
