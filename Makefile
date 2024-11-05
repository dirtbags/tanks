DESTDIR ?= $(HOME)
CFLAGS = -Wall

BINARIES = forftanks upload.cgi

all: $(BINARIES)

install: $(BINARIES)
	install -d $(DESTDIR)$(PREFIX)/bin
	install $(BINARIES) $(DESTDIR)$(PREFIX)/bin

forftanks: forftanks.o ctanks.o forf.o
forftanks: LDLIBS = -lm

forftanks.o: forf.h ctanks.h
forf.o: forf.c forf.h
ctanks.o: ctanks.h

clean:
	rm -f *.o next-round round-*.html
	rm -f $(BINARIES)
