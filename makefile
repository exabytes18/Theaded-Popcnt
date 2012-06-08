CC:=/usr/gcc470/bin/gcc
OPTS:=-O3 -std=c99 -pedantic -Wall -Wextra -march=native
LIBS:=-lpthread

all: popcnt

popcnt: popcnt.c
	$(CC) $(OPTS) $(LIBS) -o $@ $^

clean:
	-rm -rf popcnt

