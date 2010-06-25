all: test

test: test-tanks
	./test-tanks

test-tanks: test-tanks.o ctanks.o
