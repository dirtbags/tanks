BINARIES = forftanks upload.cgi
HTML = forf.html procs.html intro.html designer.html debugger.html
WWW += style.css designer.js nav.html.inc jstanks.js
WWW += docs/assets/images/grunge.png docs/assets/js/figures.js docs/assets/js/tanks.js

CFLAGS = -Wall

DESTDIR = /opt/tanks

all: $(BINARIES) $(HTML)

install: $(BINARIES) $(HTML)
	install -d $(DESTDIR)/bin
	install go.sh $(DESTDIR)/bin
	install round.sh $(DESTDIR)/bin
	install rank.awk $(DESTDIR)/bin
	install summary.awk $(DESTDIR)/bin
	install forftanks $(DESTDIR)/bin

	install -d -o $(id -u www) -g $(id -g www) $(DESTDIR)/www
	install upload.cgi $(DESTDIR)/www
	install -m 0644 $(HTML) $(DESTDIR)/www
	install -m 0644 $(WWW) $(DESTDIR)/www

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
