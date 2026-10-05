#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include "link.h"
#include "network.h"
#include "arp.h"
#include "utils.h"
#include "transport.h"
#include "udp.h"

int print_usage(char *prog_name) {
	fprintf(stderr, "usage: %s <own-path> <hub-path> <mac> <ip> <netmask> <gateway> <destination-ip>\n", prog_name);
	return 1;
}

void handle(const uint8_t *payload, uint16_t payload_size) {
	(void)payload;
	printf("Incoming datagram of payload size %u parsed.\n", payload_size);
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
	uint16_t port = 12;

	bind_udp_port(port, handle);
	
	int res = 0;
	time_t last_send = time(NULL);
	time_t send_again_after = 1; // in seconds
	while (true) {
		if (link_await(&iface, TIMEOUT_MS) == LINK_READY) handle_incoming(&net_iface);
		net_tick(&net_iface);
		if (time(NULL) - last_send < send_again_after) continue;
		
		res = send_datagram(&net_iface, destination, port, port, payload, payload_size);
		if (res != TP_OK) {
			printf("Could not send datagram.\n");
		};
		last_send = time(NULL);
	}
	
	return 0;
}