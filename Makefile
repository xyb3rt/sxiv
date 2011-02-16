all: sxiv

VERSION=git-20110216

CC?=gcc
PREFIX?=/usr/local
CFLAGS+= -Wall -pedantic -DVERSION=\"$(VERSION)\"
LDFLAGS+= 
LIBS+= -lX11 -lImlib2

SRCFILES=$(wildcard *.c)
OBJFILES=$(SRCFILES:.c=.o)

sxiv:	$(OBJFILES)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c Makefile config.h
	$(CC) $(CFLAGS) -c -o $@ $<

install: all
	install -D -m 755 -o root -g root sxiv $(PREFIX)/bin/sxiv
	mkdir -p $(PREFIX)/share/man/man1
	sed "s/VERSION/$(VERSION)/g" sxiv.1 > $(PREFIX)/share/man/man1/sxiv.1
	chmod 644 $(PREFIX)/share/man/man1/sxiv.1

clean:
	rm -f sxiv *.o

tags: *.h *.c
	ctags $^

cscope: *.h *.c
	cscope -b
