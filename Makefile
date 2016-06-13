# Configuration section
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv

# library used for ai_term
TERMLIB = mmab


# --------------------

UNAME = $(shell uname)

ifeq ($(UNAME),Darwin)
	LIB_EXT = dylib
	LIB_FLAGS = -dynamiclib
else
	LIB_EXT = so
	LIB_FLAGS = -shared -fPIC
endif

.PHONY: all clean remake

all: ai_term

clean:
	rm -f ai_term genwinmasks winmasks.h *.o *.dylib *.so *.a
	rm -rf *.dSYM

remake: clean all


%.$(LIB_EXT): %.c winmasks.h $(wildcard *.h)
	$(CC) $(CFLAGS) $(LIB_FLAGS) -o $@ $<

%.a: %.o
	ar -cr $@ $^

%.o: %.c winmasks.h
	$(CC) $(CFLAGS) -c -o $@ $<

ai_term: ai_term.c $(TERMLIB).$(LIB_EXT) $(wildcard *.h)
	$(CC) $(CFLAGS) -o $@ $(filter-out %.h,$^)

genwinmasks: genwinmasks.c
	$(CC) $(CFLAGS) -o $@ $<

winmasks.h: genwinmasks
	./genwinmasks >winmasks.h
