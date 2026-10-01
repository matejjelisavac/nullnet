#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include "link.h"
#include "network.h"
#include "arp.h"
#include "utils.h"

int print_usage(char *prog_name) {
	fprintf(stderr, "usage: %s <own-path> <hub-path> <mac> <ip> <netmask> <gateway> <destination-ip>\n", prog_name);
	return 1;
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
	int res = 0;
	while (true) {

		usleep(TIMEOUT_MS * 1000);
		res = send_packet(&net_iface, NET_TTL, NET_PROTOCOL_UDP, destination, payload, payload_size);
		if (res == NET_ERROR) {
			printf("Could not send packet.\n");
		};
		if (res == NET_PENDING) {
			if (tries == 4) {
				printf("ARP failed on 4 tries to destination or gateway.\n");
				return 1;
			}
			tries++;
		};

		if (link_await(&iface, TIMEOUT_MS) == LINK_READY) {
			packet p;
			int receive = get_packet(&net_iface, &p); // fills the ARP cache as a side effect
			if (receive == UTIL_RECEIVED) return 0;
		}
	}
	
	return 0;
}