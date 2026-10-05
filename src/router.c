
// Router / gateway: talks to multiple subnet's hubs.

// The router must click into the hubs. The router creates a network interface
// on each corresponding hub (this isn't anything new, its just that hosts 
//  have never had to talk to multiple hubs). It will have separate IPs for each.

// // Routers must then identify each other. To do so, they will send a special
// // L4-adjacent segment. e.g struct routing_message {}. Headers will include
// // -Protocol version -Message Type (Hello) -Source IP -Destination IP (broadcast?) -
// // We can implement "update" messages and pathing later when we want more sophistication.
// // [QUESTION: How will the routers send these on nullnet? Its an L1 problem, same as the hub-host idea]

// A router will have the arguments FOR EACH HUB X:
// <hub-path-x> <own-path-x> <mac-x> <ip-x> <netmask-x>

// It will keep:
// 1. Hub Network Interface List
// // 2. Routing table = SUBNET | NETMASK | NEXT_HOP IP | OUT NET_INTERFACE
	// // Reaching each router on one subnet will send out of one single net_iface, trusting that
	// // the router on that interface will relay it to them.

// When router recieves on one interface MAC:
	// ARP -> handle on that interface
	// PACKET --> Decrement TTL (drop if 0), find an interface with a netmask match,
	//			  send on that interface (ARP may be required) with this interface MAC as source
	// 				- We MUST change the source MAC. Because anything (a switch) that learns host A's MAC 
	//  			  on a diff subnet will hand a frame (without touching, on a port) to its router MAC-addressed 
	// 				  to host A, which needs to be accepted by the router on L2, which only accepts its own MAC first.

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "utils.h"
#include "network.h"
#include "link.h"
#include "wire.h"

#define ROUTER_MAX_INTERFACES 8
#define POLL_TIMEOUT_MS 250
	
int print_usage(char *prog_name) {
	fprintf(stderr, "usage: %s <own-path> <hub-path> <mac> <ip> <netmask> [...]\n", prog_name);
	fprintf(stderr, "       (five arguments per interface, at least two interfaces)\n");
	return 1;
}

int main(int argc, char *argv[]) {
	int iface_args = 5;
	if ((argc-1) % iface_args != 0 || (argc-1) < 2 * iface_args) return print_usage(argv[0]);

	int iface_count = (argc-1) / iface_args;
	if (iface_count > ROUTER_MAX_INTERFACES) return 1;

	net_interface net_interfaces[ROUTER_MAX_INTERFACES] = {0};
	interface interfaces[ROUTER_MAX_INTERFACES] = {0}; //Required to keep the net_iface->iface pointers alive.


	for (int i = 0; i < iface_count ; i++) {
		uint8_t mac[LINK_MAC_LENGTH];
		uint8_t ip[NET_IP_LENGTH];
		uint8_t netmask[NET_IP_LENGTH];

		char *own_path = argv[iface_args*i+1];
		char *hub_path = argv[iface_args*i+2];
		if (read_mac(mac, argv[iface_args*i+3]) != 0) return print_usage(argv[0]); //indices 3, 8, 13, etc.
		if (read_ip(ip, argv[iface_args*i+4]) != 0) return print_usage(argv[0]); 
		if (read_ip(netmask, argv[iface_args*i+5]) != 0) return print_usage(argv[0]); //indices 5, 10, 15, etc.

		if (link_init(&interfaces[i], mac, own_path, hub_path) != LINK_OK) return 1;
		if (net_init(&net_interfaces[i], &interfaces[i], ip, netmask, NULL) != NET_OK) return 1;
	}

	// Receive loop.
	while (true) {
		// Poll for interfaces, get packet on ready
		size_t ready = 0;
		if (link_await_many(interfaces, iface_count, POLL_TIMEOUT_MS, &ready) != LINK_READY) continue;
		net_interface *incoming = &net_interfaces[ready];
		net_tick(incoming);
		frame f;
		recv_frame(&interfaces[ready], &f); // TODO no error check
		if (f.type == LINK_TYPE_ARP) {
			arp_handle(incoming, f.payload, f.payload_size);
			continue;
		};
		packet p;
		int res = recv_packet(incoming, &p, f.payload, f.payload_size);
		if (res == NET_NOT_MINE) { //This packet needs to be forwarded to final IP
			// Find the correct interface to send from with netmask
			net_interface *outgoing = NULL;
			for (int i = 0; i < iface_count; i++) {
				if (ip_in_subnet(net_interfaces[i].ip_address, p.destination, net_interfaces[i].netmask)) {
					outgoing = &net_interfaces[i];
					break;
				}
			}

			// If no interface we are more than one hop away, or doesnt exist. Unreachable anyway
			// Eventually routing table will handle this better
			if (outgoing == NULL) {
				printf("No route to ");
				print_ip(p.destination);
				printf(", dropping.\n");
				continue;
			}

			// Send. Network will handle changing next_hop_mac, Link will handle changing src mac
			if (p.ttl <= 1) {
				printf("TTL expired, dropping.\n");
				continue;
			}
			if (forward_packet(outgoing, &p) != NET_OK) continue; //TODO Decide how to handle errors
		}
		if (res == NET_OK) continue; //Normal packet addressed to router.
	}
}