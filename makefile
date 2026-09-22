CC = gcc
CFLAGS = -Wall -Wextra -g -Isrc
HEADERS = src/link.h src/network.h src/arp.h src/wire.h

.PHONY: all clean

all: hub sender receiver

hub: hub.c $(HEADERS)
	$(CC) $(CFLAGS) hub.c -o $@

sender: src/link.c test_sender.c $(HEADERS)
	$(CC) $(CFLAGS) src/link.c test_sender.c -o $@

receiver: src/link.c test_receiver.c $(HEADERS)
	$(CC) $(CFLAGS) src/link.c test_receiver.c -o $@

clean:
	rm -f hub sender receiver *.o
