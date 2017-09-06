PREFIX    := /usr/local
MANPREFIX := $(PREFIX)/share/man

CC        ?= gcc
CFLAGS    += -std=c99 -Wall -pedantic
CPPFLAGS  += -I/usr/include/freetype2 -D_XOPEN_SOURCE=700
LDFLAGS   += 
LIBS      := -lImlib2 -lX11 -lXft

# autoreload backend: inotify/nop
AUTORELOAD := inotify

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

