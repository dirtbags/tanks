CFLAGS = -Wall

all: html run-tanks designer.cgi
html: forf.html procs.html intro.html

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
	rm -f run-tanks designer.cgi *.o forf.c forf.h
	rm -f next-round round-*.html summary.html forf.html
