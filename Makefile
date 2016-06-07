# Configuration section
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv

# library used for ai_term, and list of all ai libnames
TERMLIB = mc
AI_LIBNAMES = mmab mc


# --------------------

UNAME = $(shell uname)

ifeq ($(UNAME),Darwin)
	LIB_EXT = dylib
	LIB_FLAGS = -dynamiclib
else
	LIB_EXT = so
	LIB_FLAGS = -shared -fPIC
endif

AI_LIBS = $(foreach base,$(AI_LIBNAMES),$(base).$(LIB_EXT))

PRODUCTS = $(AI_LIBS) ai_term genwinmasks winmasks.h

.PHONY: all clean remake

all: $(PRODUCTS)

clean:
	rm -f $(PRODUCTS)

remake: clean all


%.$(LIB_EXT): %.c winmasks.h $(wildcard *.h)
	$(CC) $(CFLAGS) $(LIB_FLAGS) -o $@ $<

ai_term: ai_term.c $(TERMLIB).$(LIB_EXT) $(wildcard *.h)
	$(CC) $(CFLAGS) -o $@ $(filter-out %.h,$^)

genwinmasks: genwinmasks.c
	$(CC) $(CFLAGS) -o $@ $<

winmasks.h: genwinmasks
	./genwinmasks >winmasks.h
