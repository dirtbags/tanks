BINARIES = run-tanks designer.cgi
HTML = forf.html procs.html intro.html designer.html

CFLAGS = -Wall

all: $(BINARIES) $(HTML)

run-tanks: run-tanks.o ctanks.o forf.o
run-tanks: LDFLAGS = -lm

run-tanks.o: forf.h ctanks.h
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
