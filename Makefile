BINARIES = forftanks designer.cgi
HTML = forf.html procs.html intro.html designer.html

CFLAGS = -Wall

all: $(BINARIES) $(HTML)

forftanks: forftanks.o ctanks.o forf.o
forftanks: LDFLAGS = -lm

forftanks.o: forf.h ctanks.h
forf.o: forf.c forf.h
ctanks.o: ctanks.h

%.html: %.html.m4
	m4 $< > $@

clean:
	rm -f *.o next-round round-*.html
	rm -f $(BINARIES) $(HTML)
