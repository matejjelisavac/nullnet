#include <stdio.h>
#include <stdlib.h>
#include "link.h"
#include "network.h"


int read_mac(uint8_t *mac_buf, char *mac) {
	// FF:FF:FF:FF
	char *curr = mac;
	int macindex = 0;

	while (true) {
		char *endptr;
		unsigned long val = strtoul(curr, &endptr, 16);
		if (val > UINT8_MAX) return 1; //Too large
		if (endptr == curr) return 1; //No hex found

		uint8_t serialized_val = (uint8_t) val;
		*(mac_buf+macindex++) = serialized_val;
		if (macindex >= LINK_MAC_LENGTH) return 1;


		if (*endptr == '\0') break;
		curr = endptr + 1; //Skip over the semicolon
	}
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

		uint8_t serialized_val = (uint8_t) val;
		*(ip_buf+ipindex++) = serialized_val;
		if (ipindex >= NET_IP_LENGTH) return 1;

		if (*endptr == '\0') break;
		curr = endptr + 1; //Skip over the semicolon
	}
	return 0;
}

int print_usage(char *prog_name) {
	fprintf(stderr, "usage: %s <own-path> <hub-path> <mac> <ip> <netmask> <gateway>\n", prog_name);
	return 1;
}

int main(int argc, char *argv[]) {
	if (argc != 7) return print_usage(argv[0]);
	char *own_path = argv[1];
	char *hub_path = argv[2];

	uint8_t mac[LINK_MAC_LENGTH];
	uint8_t ip[NET_IP_LENGTH];
	uint8_t netmask[NET_IP_LENGTH];
	uint8_t gateway[NET_IP_LENGTH];

	if (read_mac(mac, argv[3]) != 0) return print_usage(argv[0]);
	if (read_ip(ip, argv[4]) != 0) return print_usage(argv[0]);
	if (read_ip(netmask, argv[5]) != 0) return print_usage(argv[0]);
	if (read_ip(gateway, argv[6]) != 0) return print_usage(argv[0]);
	
	interface iface;
	link_init(&iface, mac, own_path, hub_path);
	
	net_interface net_iface;
	net_init(&net_iface, &iface, ip, netmask, gateway);

	// Exploratory. Very WIP
	// TODO Demux on f.type for ARP/Packet
	while (true) {
		frame f;
		recv_frame(&iface, &f);
		packet p;
		recv_packet(&net_iface, f.payload, f.payload_size, &p);

		printf("%d", p.payload_size);
	}
	
	return 0;
}