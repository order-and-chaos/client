CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv

AI_LIB = ../mmab.a

.PHONY: all clean remake

all: client

clean:
	rm -f client

remake: clean all


../%.a:
	make -C .. $*.a

client: $(wildcard *.c *.h) $(AI_LIB)
	$(CC) $(CFLAGS) -o $@ $(filter-out %.h,$^)
