all: sxiv

CC?=gcc
PREFIX?=/usr/local
CFLAGS+= -Wall -pedantic -g
LDFLAGS+= 
LIBS+= 

SRCFILES=$(wildcard *.c)
OBJFILES=$(SRCFILES:.c=.o)

physlock:	$(OBJFILES)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

install: all
	install -D -m 4755 -o root -g root sxiv $(PREFIX)/sbin/sxiv

clean:
	rm -f sxiv *.o

tags: *.h *.c
	ctags $^

cscope: *.h *.c
	cscope -b
