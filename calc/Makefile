CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -fwrapv
BIN = calc

.PHONY: all clean remake

all: $(BIN)

clean:
	rm $(BIN) *.dSYM *.o

remake: clean all


$(BIN): $(wildcard *.c *.h)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
