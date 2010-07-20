CFLAGS = -Wall

all: html run-tanks designer.cgi
html: forf.html

run-tanks: run-tanks.o ctanks.o forf.o
run-tanks: LDFLAGS = -lm

run-tanks.o: forf.h ctanks.h
forf.o: forf.c forf.h
ctanks.o: ctanks.h

forf.html: forf.html.sh forf/forf.txt
	./forf.html.sh > $@

forf.%: forf/forf.%
	cp forf/$@ $@
forf/%:
	git submodule update --init

clean:
	rm -f run-tanks designer.cgi *.o forf.c forf.h
	rm -f next-round round-*.html summary.html forf.html
