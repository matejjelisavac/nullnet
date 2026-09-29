#include "utils.h"
#include <stdlib.h>

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
		curr = endptr + 1; //Skip over the period/delim
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
		curr = endptr + 1; //Skip over the semicolon/delim
	}
	if (ipindex != NET_IP_LENGTH) return 1; //Too few fields
	return 0;
}

int get_packet(packet *p, net_interface *net_iface) {
	frame f;
	if (recv_frame(net_iface->iface, &f) != LINK_OK) return UTIL_ERROR; //no need to check for not_mine mac address
	if (f.type == LINK_TYPE_ARP) {
		if (arp_handle(net_iface, f.payload, f.payload_size) != ARP_OK) return UTIL_ERROR;
			return UTIL_ARP_HANDLED;
		}
	if (recv_packet(net_iface, f.payload, f.payload_size, p) == NET_NOT_MINE) return UTIL_NOT_MINE;
	return UTIL_RECEIVED;
}
