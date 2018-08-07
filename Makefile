BINARIES = forftanks upload.cgi
HTML = forf.html procs.html intro.html designer.html debugger.html
WWW = style.css grunge.png designer.js figures.js tanks.js nav.html.inc jstanks.js

CFLAGS = -Wall

DESTDIR = /opt/tanks

all: $(BINARIES) $(HTML)

install:
	install -d $(DESTDIR)/bin
    install go.sh $(DESTDIR)/bin
	install round.sh $(DESTDIR)/bin
	install forftanks $(DESTDIR)/bin

	install -d $(DESTDIR)/www
	install designer.cgi $(DESTDIR)/www
	install $(HTML) $(DESTDIR)/www
	install $(WWW) $(DESTDIR)/www

    install -d $(DESTDIR)/examples
	cp -r examples $(DESTDIR)/examples

forftanks: forftanks.o ctanks.o forf.o
forftanks: LDLIBS = -lm

forftanks.o: forf.h ctanks.h
forf.o: forf.c forf.h
ctanks.o: ctanks.h

%.html: %.html.m4 nav.html.inc
	m4 $< > $@

clean:
	rm -f *.o next-round round-*.html
	rm -f $(BINARIES) $(HTML)
