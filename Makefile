VERSION = git-20120328

CC      = gcc
CFLAGS  = -ansi -Wall -pedantic -O2
LDFLAGS =
LIBS    = -lX11 -lImlib2 -lgif

PREFIX    = /usr/local
MANPREFIX = $(PREFIX)/share/man

SRC = commands.c exif.c image.c main.c options.c thumbs.c util.c window.c
OBJ = $(SRC:.c=.o)

all: options sxiv

options:
	@echo "sxiv build options:"
	@echo "CC      = $(CC)"
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "PREFIX  = $(PREFIX)"

.c.o:
	@echo "CC $<"
	@$(CC) $(CFLAGS) -DVERSION=\"$(VERSION)\" -c -o $@ $<

$(OBJ): Makefile config.h

config.h:
	@echo "creating $@ from config.def.h"
	@cp config.def.h $@

sxiv:	$(OBJ)
	@echo "CC -o $@"
	@$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

clean:
	@echo "cleaning"
	@rm -f $(OBJ) sxiv sxiv-$(VERSION).tar.gz

install: all
	@echo "installing executable file to $(DESTDIR)$(PREFIX)/bin"
	@install -D -m 755 sxiv $(DESTDIR)$(PREFIX)/bin/sxiv
	@echo "installing desktop file to $(DESTDIR)$(PREFIX)/share/applications"
	@install -D -m 0644 sxiv.desktop $(DESTDIR)$(PREFIX)/share/applications/sxiv.desktop
	@echo "installing manual page to $(DESTDIR)$(MANPREFIX)/man1"
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@sed "s/VERSION/$(VERSION)/g" sxiv.1 > $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man1/sxiv.1

uninstall:
	@echo "removing executable file from $(DESTDIR)$(PREFIX)/bin"
	@rm -f $(DESTDIR)$(PREFIX)/bin/sxiv
	@echo "removind desktop file from $(DESTDIR)$(PREFIX)/share/applications"
	@rm -f $(DESTDIR)$(PREFIX)/share/applications/sxiv.desktop
	@echo "removing manual page from $(DESTDIR)$(MANPREFIX)/man1"
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
