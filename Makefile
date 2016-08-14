# Configuration section
CC = gcc

CFLAGS = -Wall -Wextra -O2 -std=c11 -fwrapv
ifdef DEBUG
	CFLAGS += -g
else
	CFLAGS += -O2
endif

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
TOMJSON_LIB = client/vendor/tomjson

CLIENTDEPS = $(TERMIO_LIB)/libtermio.a $(TOMJSON_LIB)/libtomjson.a

.PHONY: all clean remake client competition

all: ai_term client competition

clean:
	rm -f ai_term genwinmasks winmasks.h *.o *.dylib *.so *.a
	rm -f client/client
	rm -f competition/competition competition/*.dylib competition/*.so
	rm -rf *.dSYM client/*.dSYM competition/*.dSYM
	make -C $(TERMIO_LIB) clean
	make -C $(TOMJSON_LIB) clean

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

$(TERMIO_LIB)/libtermio.a: $(wildcard $(TERMIO_LIB)/*.c) $(wildcard $(TERMIO_LIB)/*.h)
	make -C $(TERMIO_LIB) clean staticlib

$(TOMJSON_LIB)/libtomjson.a: $(wildcard $(TOMJSON_LIB)/*.c) $(wildcard $(TOMJSON_LIB)/*.h)
	make -C $(TOMJSON_LIB) clean staticlib test

client/client: $(CLIENTDEPS) $(NOPOLL_LIB)/libnopoll.a $(wildcard client/*.c client/*.h) $(CLIENTLIB)
	$(CC) $(CFLAGS) -I$(NOPOLL_INC) -I$(TERMIO_LIB) -I$(TOMJSON_LIB) -o $@ $(filter-out %.h,$^) -lssl -lcrypto


competition/competition: $(wildcard competition/*.c competition/*.h)
	$(CC) $(CFLAGS) -ldl -o $@ $(filter-out %.h,$^)

competition/%.$(DYLIB_EXT): %.c winmasks.h $(wildcard *.h)
	$(CC) $(CFLAGS) $(DYLIB_FLAGS) -o $@ $(filter-out %.h,$^)
