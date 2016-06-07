CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv
UNAME = $(shell uname)

ifeq ($(UNAME),Darwin)
	LIB_EXT = dylib
	LIB_FLAGS = -dynamiclib
else
	LIB_EXT = so
	LIB_FLAGS = -shared -fPIC
endif

AI_LIB = ai.$(LIB_EXT)

PRODUCTS = $(AI_LIB) ai_term genwinmasks winmasks.h

.PHONY: all clean remake

all: $(PRODUCTS)

clean:
	rm -f $(PRODUCTS)

remake: clean all


$(AI_LIB): ai.c winmasks.h $(wildcard *.h)
	$(CC) $(CFLAGS) $(LIB_FLAGS) -o $@ $<

ai_term: ai_term.c $(AI_LIB) $(wildcard *.h)
	$(CC) $(CFLAGS) -o $@ $(filter-out %.h,$^)

genwinmasks: genwinmasks.c
	$(CC) $(CFLAGS) -o $@ $<

winmasks.h: genwinmasks
	./genwinmasks >winmasks.h
