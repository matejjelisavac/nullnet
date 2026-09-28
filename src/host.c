#include <stdio.h>
#include <stdlib.h>
#include "link.h"
#include "network.h"
#include "arp.h"


void print_mac(const uint8_t *mac) {
	for (size_t i = 0; i < LINK_MAC_LENGTH; i++) {
		printf("%02X", mac[i]);
		if (i < LINK_MAC_LENGTH - 1) printf(":");
	}
}

void print_ip(const uint8_t *ip) {
	for (size_t i = 0; i < NET_IP_LENGTH; i++) {
		printf("%u", ip[i]);
		if (i < NET_IP_LENGTH - 1) printf(".");
	}
}

int read_mac(uint8_t *mac_buf, char *mac) {
	// FF:FF:FF:FF
	char *curr = mac;
	int macindex = 0;

	while (true) {
		char *endptr;
		unsigned long val = strtoul(curr, &endptr, 16);
		if (val > UINT8_MAX) return 1; //Too large
		if (endptr == curr) return 1; //No hex found

		if (macindex >= LINK_MAC_LENGTH) return 1; //Too many fields

		uint8_t serialized_val = (uint8_t) val;
		*(mac_buf+macindex++) = serialized_val;

		if (*endptr == '\0') break;
		curr = endptr + 1; //Skip over the semicolon
	}
	if (macindex != LINK_MAC_LENGTH) return 1; //Too few fields
	return 0;
}

int read_ip(uint8_t *ip_buf, char *ip) {
	// 255.255.255.255

	char *curr = ip;
	int ipindex = 0;

	while (true) {
		char *endptr;
		unsigned long val = strtoul(curr, &endptr, 10);
		if (val > UINT8_MAX) return 1; //Too large
		if (endptr == curr) return 1; //No int found

		if (ipindex >= NET_IP_LENGTH) return 1; //Too many fields

		uint8_t serialized_val = (uint8_t) val;
		*(ip_buf+ipindex++) = serialized_val;

		if (*endptr == '\0') break;
		curr = endptr + 1; //Skip over the semicolon
	}
	if (ipindex != NET_IP_LENGTH) return 1; //Too few fields
	return 0;
}

int print_usage(char *prog_name) {
	fprintf(stderr, "usage: %s <own-path> <hub-path> <mac> <ip> <netmask> <gateway> <destination-ip>\n", prog_name);
	return 1;
}

#define PACKET_RECEIVED 0
#define PACKET_NOT_RECEIVED 1
#define PACKET_ARP_HANDLED 2 

int get_packet(packet *p, net_interface *net_iface) {
	frame f;
	if (recv_frame(net_iface->iface, &f) != LINK_OK) return PACKET_NOT_RECEIVED;
	if (f.type == LINK_TYPE_ARP) {
		if (arp_handle(net_iface, f.payload, f.payload_size) != ARP_OK) return PACKET_NOT_RECEIVED;
			return PACKET_ARP_HANDLED;
		}
	if (recv_packet(net_iface, f.payload, f.payload_size, p) != NET_OK) return PACKET_NOT_RECEIVED;
	return PACKET_RECEIVED;
}

// Testing ground for in/out. Will be overwritten by a real app and transport layer
int main(int argc, char *argv[]) {
	if (argc != 8) return print_usage(argv[0]);
	// ownpath, hubpath, mac, ip, netmask, gateway, destinationip
	char *own_path = argv[1];
	char *hub_path = argv[2];

	uint8_t mac[LINK_MAC_LENGTH];
	uint8_t ip[NET_IP_LENGTH];
	uint8_t netmask[NET_IP_LENGTH];
	uint8_t gateway[NET_IP_LENGTH];
	uint8_t destination[NET_IP_LENGTH];

	if (read_mac(mac, argv[3]) != 0) return print_usage(argv[0]);
	if (read_ip(ip, argv[4]) != 0) return print_usage(argv[0]);
	if (read_ip(netmask, argv[5]) != 0) return print_usage(argv[0]);
	if (read_ip(gateway, argv[6]) != 0) return print_usage(argv[0]);
	if (read_ip(destination, argv[7]) != 0) return print_usage(argv[0]);
	
	interface iface = {0};
	if (link_init(&iface, mac, own_path, hub_path) != LINK_OK) return 1;
	net_interface net_iface = {0};
	if (net_init(&net_iface, &iface, ip, netmask, gateway) != NET_OK) return 1;

	int TIMEOUT_MS = 1000;
	// Send hello to destination
	uint16_t payload_size = 0x0003;
	uint8_t payload[] = {0xAA, 0xAA, 0xAA};
	
	int tries = 0;
	int res = send_packet(&net_iface, NET_PROTOCOL_UDP, destination, payload_size, payload);

	while (true) {
		if (res == NET_ERROR) return 1;
		if (link_await(&iface, TIMEOUT_MS) == LINK_READY) {
			packet p;
			int receive = get_packet(&p, &net_iface); // fills the ARP cache as a side effect
			if (receive == PACKET_RECEIVED) {
				printf("Payload size of %u\n", p.payload[0]);
				printf("Received from ");
				print_ip(p.source);
				printf("\n");
				return 0;
			}
			if (receive == PACKET_ARP_HANDLED) {
				printf("ARP request from ");
				print_mac(net_iface.arp_cache[net_iface.arp_cache_count-1].mac_address);
				printf(" handled and learned. \n");
			}
		}
		if (res == NET_PENDING) {
			res = send_packet(&net_iface, NET_PROTOCOL_UDP, destination, payload_size, payload);
			if (tries == 4) {
				printf("ARP failed on 4 tries to %s.", argv[7]);
				return 1;
			}
			tries++;
		};
	}
	
	return 0;
}