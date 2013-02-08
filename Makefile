VERSION = git-20130208

PREFIX    = /usr/local
MANPREFIX = $(PREFIX)/share/man

CC      = gcc
CFLAGS  = -Wall -pedantic -O2 -I$(PREFIX)/include -DHAVE_GIFLIB
LDFLAGS = -L$(PREFIX)/lib
LIBS    = -lX11 -lImlib2 -lgif

SRC = commands.c exif.c image.c main.c options.c thumbs.c util.c window.c
OBJ = $(SRC:.c=.o)

all: sxiv

$(OBJ): Makefile config.h

.c.o:
	$(CC) $(CFLAGS) -DVERSION=\"$(VERSION)\" -c -o $@ $<

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
	cp image-info $(DESTDIR)$(PREFIX)/share/sxiv/exec/image-info
	chmod 755 $(DESTDIR)$(PREFIX)/share/sxiv/exec/image-info

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sxiv
	rm -f $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	rm -rf $(DESTDIR)$(PREFIX)/share/sxiv
