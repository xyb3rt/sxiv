SRCDIR    := .

PREFIX    := /usr/local
MANPREFIX := $(PREFIX)/share/man

CC        ?= gcc
CFLAGS    += -std=c99 -Wall -pedantic
CPPFLAGS  += -I/usr/include/freetype2 -D_XOPEN_SOURCE=700
LDFLAGS   += 

# autoreload backend: inotify/nop
AUTORELOAD := inotify

# enable features requiring giflib (-lgif)
HAVE_GIFLIB := 1

# enable features requiring libexif (-lexif)
HAVE_LIBEXIF := 1

