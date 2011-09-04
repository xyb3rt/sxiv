all: sxiv

VERSION = git-20110904

CC = gcc
DESTDIR =
PREFIX = /usr/local
CFLAGS = -Wall -pedantic -O2 -DVERSION=\"$(VERSION)\" -DHAVE_GIFLIB
LDFLAGS =
LIBS = -lX11 -lImlib2 -lgif

SRC = commands.c image.c main.c options.c thumbs.c util.c window.c
OBJ = $(SRC:.c=.o)

sxiv:	$(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

$(OBJ): Makefile config.h

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

install: all
	install -D -m 755 -o root -g root sxiv $(DESTDIR)$(PREFIX)/bin/sxiv
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	sed "s/VERSION/$(VERSION)/g" sxiv.1 > $(DESTDIR)$(PREFIX)/share/man/man1/sxiv.1
	chmod 644 $(DESTDIR)$(PREFIX)/share/man/man1/sxiv.1

clean:
	rm -f $(OBJ) sxiv
