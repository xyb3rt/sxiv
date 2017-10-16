# Works best with GNU make 3.80 or later

VERSION = git-20171016

srcdir = .

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

CC ?= gcc
CFLAGS += -std=c99 -Wall -pedantic
CPPFLAGS += -I/usr/include/freetype2 -D_XOPEN_SOURCE=700
DEPFLAGS = -MMD -MP
LDFLAGS += 

# autoreload backend: inotify/nop
AUTORELOAD = inotify

# enable features requiring giflib (-lgif)
HAVE_GIFLIB = 1

# enable features requiring libexif (-lexif)
HAVE_LIBEXIF = 1

REQ_CPPFLAGS = -I. -DVERSION=\"$(VERSION)\" \
  -DHAVE_GIFLIB=$(HAVE_GIFLIB) -DHAVE_LIBEXIF=$(HAVE_LIBEXIF)
ALL_CPPFLAGS = $(REQ_CPPFLAGS) $(CPPFLAGS)

LIB_EXIF_0 =
LIB_EXIF_1 = -lexif
LIB_GIF_0 =
LIB_GIF_1 = -lgif
LDLIBS = -lImlib2 -lX11 -lXft \
  $(LIB_EXIF_$(HAVE_LIBEXIF)) $(LIB_GIF_$(HAVE_GIFLIB))

-include config.mk

SRCS = autoreload_$(AUTORELOAD).c commands.c image.c main.c options.c \
  thumbs.c util.c window.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

all: sxiv

.PHONY: all clean install uninstall
.SUFFIXES:
.SUFFIXES: .c .o
$(V).SILENT:

sxiv:	$(OBJS)
	@echo "LINK $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJS): config.h $(srcdir)/Makefile

.c.o:
	@echo "CC $@"
	$(CC) $(ALL_CPPFLAGS) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: $(srcdir)/%.c
	@echo "CC $@"
	$(CC) $(ALL_CPPFLAGS) $(DEPFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/$(@:.o=.c)

config.h:
	@echo "GEN $@"
	cp $(srcdir)/config.def.h $@

clean:
	rm -f $(OBJS) $(DEPS) sxiv

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

-include $(DEPS)

