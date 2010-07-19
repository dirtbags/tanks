CFLAGS = -Wall
LDFLAGS = -lm

all: run-tanks

test: test-tanks
	./test-tanks | m4 round.html.m4 - > round.html

test-tanks: test-tanks.o ctanks.o

run-tanks: run-tanks.o ctanks.o cforf.o

clean:
	rm -f test-tanks run-tanks *.o