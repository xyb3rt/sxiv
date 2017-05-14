VERSION   := git-20170213

PREFIX    := /usr/local
MANPREFIX := $(PREFIX)/share/man

CC        ?= gcc
CFLAGS    += -std=c99 -Wall -pedantic
CPPFLAGS  += -I/usr/include/freetype2 -D_XOPEN_SOURCE=700
LDFLAGS   += 
LIBS      := -lImlib2 -lX11 -lXft

# optional dependencies:
# giflib: gif animations
ifndef NO_GIFLIB
	CPPFLAGS += -DHAVE_GIFLIB
	LIBS     += -lgif
endif
# libexif: jpeg auto-orientation, exif thumbnails
ifndef NO_LIBEXIF
	CPPFLAGS += -DHAVE_LIBEXIF
	LIBS     += -lexif
endif

.PHONY: clean install uninstall

SRC := commands.c image.c main.c options.c thumbs.c util.c window.c
# conditionally compile in autoreload-backend; usage: `make AUTORELOAD=nop`
ifeq ($(AUTORELOAD),nop)
	SRC += autoreload_nop.c
else
	SRC += autoreload_inotify.c
endif

DEP := $(SRC:.c=.d)
OBJ := $(SRC:.c=.o)

all: config.h sxiv

$(OBJ): Makefile

-include $(DEP)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DVERSION=\"$(VERSION)\" -MMD -MP -c -o $@ $<

config.h:
	cp config.def.h $@

sxiv:	$(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

clean:
	rm -f $(OBJ) $(DEP) sxiv

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
