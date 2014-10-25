VERSION = 1.3

PREFIX    = /usr/local
MANPREFIX = $(PREFIX)/share/man

CC       = gcc
CFLAGS   = -std=c99 -Wall -pedantic -O2
CPPFLAGS = -I$(PREFIX)/include -D_XOPEN_SOURCE=500 -DHAVE_LIBEXIF -DHAVE_GIFLIB
LDFLAGS  = -L$(PREFIX)/lib
LIBS     = -lX11 -lImlib2 -lexif -lgif

SRC = commands.c image.c main.c options.c thumbs.c util.c window.c
OBJ = $(SRC:.c=.o)

all: sxiv

$(OBJ): Makefile config.h

depend: .depend

.depend: $(SRC) config.h
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ >./.depend

-include .depend

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DVERSION=\"$(VERSION)\" -c -o $@ $<

config.h:
	cp config.def.h $@

sxiv:	$(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

clean:
	rm -f $(OBJ) sxiv

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp sxiv $(DESTDIR)$(PREFIX)/bin/
	chmod 755 $(DESTDIR)$(PREFIX)/bin/sxiv
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s!PREFIX!$(PREFIX)!g; s!VERSION!$(VERSION)!g" sxiv.1 > $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	mkdir -p $(DESTDIR)$(PREFIX)/share/sxiv/exec
	cp exec/* $(DESTDIR)$(PREFIX)/share/sxiv/exec/
	chmod 755 $(DESTDIR)$(PREFIX)/share/sxiv/exec/*

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sxiv
	rm -f $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	rm -rf $(DESTDIR)$(PREFIX)/share/sxiv
