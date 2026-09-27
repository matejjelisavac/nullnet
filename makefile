CC = gcc
CFLAGS = -Wall -Wextra -g -Isrc

STACK = src/link.c src/network.c src/arp.c
HEADERS = src/link.h src/network.h src/arp.h src/wire.h

.PHONY: all clean

all: hub host

hub: hub.c src/wire.h
	$(CC) $(CFLAGS) hub.c -o $@

host: src/host.c $(STACK) $(HEADERS)
	$(CC) $(CFLAGS) src/host.c $(STACK) -o $@

clean:
	rm -f hub host *.o
	rm -rf *.dSYM
