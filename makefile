CC = gcc
CFLAGS = -Wall -Wextra -g -Isrc

STACK = src/link.c src/network.c src/arp.c src/utils.c
HEADERS = src/link.h src/network.h src/arp.h src/wire.h src/utils.h

.PHONY: all clean

all: hub host router sniffer

hub: src/hub.c src/wire.h
	$(CC) $(CFLAGS) src/hub.c -o $@

host: src/host.c $(STACK) $(HEADERS)
	$(CC) $(CFLAGS) src/host.c $(STACK) -o $@

router: src/router.c $(STACK) $(HEADERS)
	$(CC) $(CFLAGS) src/router.c $(STACK) -o $@

sniffer: src/sniffer.c $(STACK) $(HEADERS)
	$(CC) $(CFLAGS) src/sniffer.c $(STACK) -o $@

clean:
	rm -f hub host router sniffer *.o
	rm -rf *.dSYM
