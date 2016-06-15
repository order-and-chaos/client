# Configuration section
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv

# library used for ai_term
TERMLIB = mmab.a

# library used for client
CLIENTLIB = mmab.a

# library names used for competition
COMPLIBNAMES = mmab mc rand


# --------------------

UNAME = $(shell uname)

ifeq ($(UNAME),Darwin)
	DYLIB_EXT = dylib
	DYLIB_FLAGS = -dynamiclib
else
	DYLIB_EXT = so
	DYLIB_FLAGS = -shared -fPIC
endif

.PHONY: all clean remake client competition

all: ai_term client competition

clean:
	rm -f ai_term genwinmasks winmasks.h *.o *.dylib *.so *.a
	rm -f client/client
	rm -f competition/competition competition/*.dylib competition/*.so
	rm -rf *.dSYM client/*.dSYM competition/*.dSYM

remake: clean all


client: client/client

competition: competition/competition $(foreach base,$(COMPLIBNAMES),competition/$(base).$(DYLIB_EXT))


%.a: %.o
	ar -cr $@ $^

%.o: %.c winmasks.h
	$(CC) $(CFLAGS) -c -o $@ $<

ai_term: ai_term.c $(TERMLIB) $(wildcard *.h)
	$(CC) $(CFLAGS) -o $@ $(filter-out %.h,$^)

genwinmasks: genwinmasks.c
	$(CC) $(CFLAGS) -o $@ $<

winmasks.h: genwinmasks
	./genwinmasks >winmasks.h


client/client: $(wildcard client/*.c client/*.h) $(CLIENTLIB)
	$(CC) $(CFLAGS) -o $@ $(filter-out %.h,$^)


competition/competition: $(wildcard competition/*.c competition/*.h)
	$(CC) $(CFLAGS) -o $@ $(filter-out %.h,$^)

competition/%.$(DYLIB_EXT): %.c winmasks.h $(wildcard *.h)
	$(CC) $(CFLAGS) $(DYLIB_FLAGS) -o $@ $(filter-out %.h,$^)
