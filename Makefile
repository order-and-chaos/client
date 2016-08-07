# Configuration section
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv

# library used for ai_term
TERMLIB = mmab.a

# library used for client
CLIENTLIB = mc.a

# library names used for competition
COMPLIBNAMES = mmab mc rand

# other library locations
NOPOLL_INC = $(HOME)/prefix/include/nopoll
NOPOLL_LIB = $(HOME)/prefix/lib


# --------------------

UNAME = $(shell uname)

ifeq ($(UNAME),Darwin)
	DYLIB_EXT = dylib
	DYLIB_FLAGS = -dynamiclib
else
	DYLIB_EXT = so
	DYLIB_FLAGS = -shared -fPIC
endif

TERMIO_LIB = client/vendor/termio

.PHONY: all clean remake client competition clientdeps

all: ai_term client competition

clean:
	rm -f ai_term genwinmasks winmasks.h *.o *.dylib *.so *.a
	rm -f client/client
	rm -f competition/competition competition/*.dylib competition/*.so
	rm -rf *.dSYM client/*.dSYM competition/*.dSYM
	make -C $(TERMIO_LIB) clean

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

clientdeps: $(TERMIO_LIB)/libtermio.a

$(TERMIO_LIB)/libtermio.a: $(TERMIO_LIB)/termio.c
	make -C $(TERMIO_LIB) clean staticlib

client/client: clientdeps $(wildcard client/*.c client/*.h) $(CLIENTLIB)
	$(CC) $(CFLAGS) -I$(NOPOLL_INC) -I$(TERMIO_LIB) -o $@ $(filter-out %.h clientdeps,$^) $(NOPOLL_LIB)/libnopoll.a $(TERMIO_LIB)/libtermio.a -lssl -lcrypto


competition/competition: $(wildcard competition/*.c competition/*.h)
	$(CC) $(CFLAGS) -ldl -o $@ $(filter-out %.h,$^)

competition/%.$(DYLIB_EXT): %.c winmasks.h $(wildcard *.h)
	$(CC) $(CFLAGS) $(DYLIB_FLAGS) -o $@ $(filter-out %.h,$^)
