CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv

.PHONY: all clean remake

all: ai

clean:
	rm -f ai genwinmasks winmasks.h

remake: clean all


ai: ai.c winmasks.h
	$(CC) $(CFLAGS) -o $@ $<

genwinmasks: genwinmasks.c
	$(CC) $(CFLAGS) -o $@ $<

winmasks.h: genwinmasks
	./genwinmasks >winmasks.h
