CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv

.PHONY: all clean remake players

all: competition players

clean:
	rm -f competition *.dylib *.so

remake: clean all


players:
	make -C ..
	cp $$(find .. -maxdepth 1 -name '*.dylib' -o -name '*.so') .


competition: competition.c
	$(CC) $(CFLAGS) -o $@ $<
