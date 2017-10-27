VERSION = 24

srcdir = .
VPATH = $(srcdir)

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

CC = cc
DEF_CFLAGS = -std=c99 -Wall -pedantic
DEF_CPPFLAGS = -I/usr/include/freetype2

# autoreload backend: inotify/nop
AUTORELOAD = inotify

# enable features requiring giflib (-lgif)
HAVE_GIFLIB = 1

# enable features requiring libexif (-lexif)
HAVE_LIBEXIF = 1

ALL_CFLAGS = $(DEF_CFLAGS) $(CFLAGS)
REQ_CPPFLAGS = -I. -D_XOPEN_SOURCE=700 -DVERSION=\"$(VERSION)\" \
  -DHAVE_GIFLIB=$(HAVE_GIFLIB) -DHAVE_LIBEXIF=$(HAVE_LIBEXIF)
ALL_CPPFLAGS = $(REQ_CPPFLAGS) $(DEF_CPPFLAGS) $(CPPFLAGS)

LIB_EXIF_0 =
LIB_EXIF_1 = -lexif
LIB_GIF_0 =
LIB_GIF_1 = -lgif
LDLIBS = -lImlib2 -lX11 -lXft \
  $(LIB_EXIF_$(HAVE_LIBEXIF)) $(LIB_GIF_$(HAVE_GIFLIB))

OBJS = autoreload_$(AUTORELOAD).o commands.o image.o main.o options.o \
  thumbs.o util.o window.o

all: sxiv

.PHONY: all clean install uninstall
.SUFFIXES:
.SUFFIXES: .c .o
$(V).SILENT:

sxiv: $(OBJS)
	@echo "LINK $@"
	$(CC) $(LDFLAGS) $(ALL_CFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJS): Makefile sxiv.h commands.lst config.h
window.o: icon/data.h

.c.o:
	@echo "CC $@"
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -c -o $@ $<

config.h:
	@echo "GEN $@"
	cp $(srcdir)/config.def.h $@

clean:
	rm -f *.o sxiv

install: all
	@echo "INSTALL bin/sxiv"
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp sxiv $(DESTDIR)$(PREFIX)/bin/
	chmod 755 $(DESTDIR)$(PREFIX)/bin/sxiv
	@echo "INSTALL sxiv.1"
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s!PREFIX!$(PREFIX)!g; s!VERSION!$(VERSION)!g" sxiv.1 \
		>$(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	@echo "INSTALL share/sxiv/"
	mkdir -p $(DESTDIR)$(PREFIX)/share/sxiv/exec
	cp exec/* $(DESTDIR)$(PREFIX)/share/sxiv/exec/
	chmod 755 $(DESTDIR)$(PREFIX)/share/sxiv/exec/*

uninstall:
	@echo "REMOVE bin/sxiv"
	rm -f $(DESTDIR)$(PREFIX)/bin/sxiv
	@echo "REMOVE sxiv.1"
	rm -f $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	@echo "REMOVE share/sxiv/"
	rm -rf $(DESTDIR)$(PREFIX)/share/sxiv

