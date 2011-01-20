all: sxiv

CC?=gcc
PREFIX?=/usr/local
CFLAGS+= -std=c99 -Wall -pedantic -g
LDFLAGS+= 
LIBS+= -lX11 -lImlib2

SRCFILES=$(wildcard *.c)
OBJFILES=$(SRCFILES:.c=.o)

sxiv:	$(OBJFILES)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c Makefile config.h
	$(CC) $(CFLAGS) -c -o $@ $<

install: all
	install -D -m 4755 -o root -g root sxiv $(PREFIX)/sbin/sxiv

clean:
	rm -f sxiv *.o

tags: *.h *.c
	ctags $^

cscope: *.h *.c
	cscope -b
