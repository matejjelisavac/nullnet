#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "link.h"
#include "network.h"
#include "arp.h"
#include "udp.h"
#include "utils.h"

#define TIMEOUT_MS 1000

int print_usage(char *prog_name) {
	fprintf(stderr, "usage: %s <own-path> <hub-path>\n", prog_name);
	return 1;
}

void print_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *lt = localtime(&tv.tv_sec);
    char buf[16];
    strftime(buf, sizeof buf, "%H:%M:%S", lt);

    printf("[%s.%03d] ", buf, (int)(tv.tv_usec / 1000));
}

void print_summary(frame *f) {
	print_timestamp();
	print_mac(f->source);
	printf(" > ");
	print_mac(f->destination);
}

void print_packet_summary(frame *f, packet *p) {
	print_summary(f);
	printf("  %-8s", "PACKET");
	print_ip(p->source);
	printf(" > ");
	print_ip(p->destination);
	printf("  ttl %-3u", p->ttl);
	printf("  %-4s", p->protocol == NET_PROTOCOL_TCP ? "TCP" : "UDP");
	printf("  size %u", p->payload_size);
	printf("\n");
}

void print_datagram_summary(frame *f, packet *p, datagram *d) {
	print_summary(f);
	printf("  %-8s", "UDP");
	print_ip(p->source);
	printf(":%u > ", d->src_port);
	print_ip(p->destination);
	printf(":%u", d->dest_port);
	printf("  ttl %-3u", p->ttl);
	printf("  cksum %04X", d->checksum);
	printf("  size %u", d->payload_size);
	printf("\n");
}

void print_arp_summary(frame *f, arp_msg *a) {
	print_summary(f);
	printf("  %-8s", "ARP");
	if (a->operation == ARP_REQUEST) {
		print_ip(a->source_ip);
		printf(" is requesting ");
		print_ip(a->destination_ip);
	}
	else {
		print_ip(a->source_ip);
		printf(" is at ");
		print_mac(a->source_mac);
	}
	printf("\n");
}


int main(int argc, char *argv[]) {
	if (argc != 3) return print_usage(argv[0]);

	setvbuf(stdout, NULL, _IONBF, 0);

	uint8_t mac[LINK_MAC_LENGTH] = {0};

	interface iface = {0};
	if (link_init(&iface, mac, argv[1], argv[2]) != LINK_OK) return 1;

	while (true) {
		int poll = link_await(&iface, TIMEOUT_MS);
		if (poll == LINK_READY) {
			frame f;
			int frame_res = recv_frame(&iface, &f);
			if (frame_res != LINK_OK && frame_res != LINK_NOT_MINE) continue;
			switch (f.type) {
				case LINK_TYPE_ARP: {
					arp_msg arp;
					if (deserialize_arp(&arp, f.payload, f.payload_size) != ARP_OK) {
						print_summary(&f);
						printf("\t malformed ARP\n");
						continue;
					};
					print_arp_summary(&f, &arp);
					break;
				}
				case LINK_TYPE_PACKET: {
					packet p;
					if (deserialize_packet(&p, f.payload, f.payload_size) != NET_OK) {
						print_summary(&f);
						printf("\t malformed packet\n");
						continue;
					};
					
					if (p.protocol == NET_PROTOCOL_UDP) {
						datagram d;
						if (deserialize_datagram(&d, p.payload, p.payload_size) == TP_OK) {
							print_datagram_summary(&f, &p, &d);
							break;
						}
						// Fall through to the packet line if the datagram won't parse.
					}
					print_packet_summary(&f, &p);
					break;
				}
			}

		}
		if (poll == LINK_ERROR) {
			printf("Something went wrong when polling the hub.\n");
			return 1;
		}
	}

}