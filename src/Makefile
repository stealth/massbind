CC=gcc
CFLAGS=-c -O2 -Wall -std=c11 -pedantic

.PHONY: all clean

all: mass

mass: massbind.o mass.o
	$(CC) $^ -o $@

mass.o: mass.c
	$(CC) $(CFLAGS) $^

massbind.o: massbind.c
	$(CC) $(CFLAGS) $^

clean:
	rm -f *.o mass

