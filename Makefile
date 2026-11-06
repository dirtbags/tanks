DESTDIR ?= $(HOME)
CFLAGS = -Wall

BINARIES = forftanks tanksd

all: $(BINARIES)

install: $(BINARIES)
	install -d $(DESTDIR)$(PREFIX)/bin
	install $(BINARIES) $(DESTDIR)$(PREFIX)/bin

forftanks: forftanks.o ctanks.o forf.o
forftanks: LDLIBS = -lm

forftanks.o: forf.h ctanks.h
forf.o: forf.c forf.h
ctanks.o: ctanks.h

tanksd: tanksd.go
	go build $<

forf-test: tests/forf_test.c forf.c forf.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -o $@ tests/forf_test.c forf.c

.PHONY: test-forf
test-forf: forf-test
	./forf-test

clean:
	rm -f *.o next-round round-*.html
	rm -f $(BINARIES) forf-test

image:
	docker buildx build --push --tag=git-registry.cyberfire.ninja/devs/tanks:master .
