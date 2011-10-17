VERSION = git-20111017

CC      = gcc
CFLAGS  = -ansi -Wall -pedantic -O2
LDFLAGS =
LIBS    = -lX11 -lImlib2

PREFIX    = /usr/local
MANPREFIX = $(PREFIX)/share/man

SRC = commands.c image.c main.c options.c thumbs.c util.c window.c
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

$(OBJ) config: Makefile config.h

config: config.c
	@$(CC) $(CFLAGS) -o $@ $@.c

config.h:
	@echo "creating $@ from config.def.h"
	@cp config.def.h $@

sxiv:	$(OBJ) config
	@echo "CC -o $@"
	@$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS) $$(./config -l)

clean:
	@echo "cleaning"
	@rm -f $(OBJ) config sxiv sxiv-$(VERSION).tar.gz

dist: clean
	@echo "creating dist tarball"
	@mkdir -p sxiv-$(VERSION)
	@cp LICENSE Makefile README.md config.def.h sxiv.1 $(SRC) config.c \
	    sxiv-$(VERSION)
	@tar -cf sxiv-$(VERSION).tar sxiv-$(VERSION)
	@gzip sxiv-$(VERSION).tar
	@rm -rf sxiv-$(VERSION)

install: all
	@echo "installing executable file to $(DESTDIR)$(PREFIX)/bin"
	@install -D -m 755 sxiv $(DESTDIR)$(PREFIX)/bin/sxiv
	@echo "installing manual page to $(DESTDIR)$(MANPREFIX)/man1"
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@sed "s/VERSION/$(VERSION)/g" sxiv.1 > $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	@chmod 644 $(DESTDIR)$(MANPREFIX)/man1/sxiv.1

uninstall:
	@echo "removing executable file from $(DESTDIR)$(PREFIX)/bin"
	@rm -f $(DESTDIR)$(PREFIX)/bin/sxiv
	@echo "removing manual page from $(DESTDIR)$(MANPREFIX)/man1"
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
