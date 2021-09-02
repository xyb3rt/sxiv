version = 27

srcdir = .
VPATH = $(srcdir)

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# autoreload backend: inotify/nop
AUTORELOAD = inotify

# enable features requiring giflib (-lgif)
HAVE_GIFLIB = 1

# enable features requiring libexif (-lexif)
HAVE_LIBEXIF = 1

# enable features requiring libwebp (-lwebp)
HAVE_LIBWEBP = 1

# enable features requiring libcurl (-lcurl)
HAVE_LIBCURL = 1

# enable features requiring cairo and svg (-lgif)
HAVE_CAIRO_SVG = 0

lib_cairo_svg_0 =
lib_cairo_svg_1 = `pkg-config --cflags --libs librsvg-2.0 cairo`

cflags = -std=c99 -Wall -pedantic $(CFLAGS) $(lib_cairo_svg_$(HAVE_CAIRO_SVG))

cppflags = -I. $(CPPFLAGS) -D_XOPEN_SOURCE=700 \
  -DHAVE_GIFLIB=$(HAVE_GIFLIB) -DHAVE_LIBEXIF=$(HAVE_LIBEXIF) \
  -DHAVE_LIBWEBP=$(HAVE_LIBWEBP) -DHAVE_LIBCURL=$(HAVE_LIBCURL) \
  -DHAVE_CAIRO_SVG=$(HAVE_CAIRO_SVG) \
  -I/usr/include/freetype2 -I$(PREFIX)/include/freetype2

lib_exif_0 =
lib_exif_1 = -lexif
lib_gif_0 =
lib_gif_1 = -lgif
lib_webp_0 =
lib_webp_1 = -lwebpdemux -lwebp
lib_curl_0 =
lib_curl_1 = -lcurl
ldlibs = $(LDLIBS) -lImlib2 -lX11 -lXft -lfontconfig \
  $(lib_exif_$(HAVE_LIBEXIF)) $(lib_gif_$(HAVE_GIFLIB)) \
  $(lib_webp_$(HAVE_LIBWEBP)) $(lib_curl_$(HAVE_LIBCURL))

objs = autoreload_$(AUTORELOAD).o commands.o image.o main.o options.o \
  thumbs.o util.o window.o remote.o

all: sxiv

.PHONY: all clean install uninstall
.SUFFIXES:
.SUFFIXES: .c .o
$(V).SILENT:

sxiv: $(objs)
	@echo "LINK $@"
	$(CC) $(LDFLAGS) -o $@ $(objs) $(ldlibs) $(cflags)

$(objs): Makefile sxiv.h commands.lst config.h
options.o: version.h
window.o: icon/data.h

.c.o:
	@echo "CC $@"
	$(CC) $(cflags) $(cppflags) -c -o $@ $<

config.h:
	@echo "GEN $@"
	cp $(srcdir)/config.def.h $@

version.h: Makefile .git/index
	@echo "GEN $@"
	v="$$(cd $(srcdir); git describe 2>/dev/null)"; \
	echo "#define VERSION \"$${v:-$(version)}\"" >$@

.git/index:

clean:
	rm -f *.o sxiv

install: all
	@echo "INSTALL bin/sxiv"
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp sxiv $(DESTDIR)$(PREFIX)/bin/
	chmod 755 $(DESTDIR)$(PREFIX)/bin/sxiv
	@echo "INSTALL sxiv.1"
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s!PREFIX!$(PREFIX)!g; s!VERSION!$(version)!g" sxiv.1 \
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

