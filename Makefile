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

forf.html: forf/forf.txt

forf.%: forf/forf.%
	cp forf/$@ $@
.PRECIOUS: forf/%
forf/%:
	git submodule update --init

clean:
	rm -f *.o forf.c forf.h next-round round-*.html
	rm -f $(BINARIES) $(HTML)
