#include <string.h>
#include <stdbool.h>
#include "network.h"

int build_packet(packet *p, uint8_t protocol, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload) {
	if (payload_size > NET_MTU) return 1;
	if (protocol != NET_PROTOCOL_UDP && protocol != NET_PROTOCOL_TCP) return NET_ERROR;
	p->ttl = NET_TTL;
	p->protocol = protocol;
	memcpy(p->source, source, NET_IP_LENGTH);
	memcpy(p->destination, destination, NET_IP_LENGTH);
	p->payload_size = payload_size;
	memcpy(p->payload, payload, payload_size);
	return NET_OK;
}

size_t serialize_packet(const packet *p, uint8_t *buf, size_t buf_size) {
	size_t needed = NET_HEADER_LENGTH + p->payload_size;
	if (buf_size < needed) return 0;

	size_t index = 0;

	// TTL
	*(buf+index++) = p->ttl;
	// Protocol
	*(buf+index++) = p->protocol;
	// Source
	memcpy(buf+index, p->source, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Destination
	memcpy(buf+index, p->destination, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Payload size
	uint8_t hi = p->payload_size >> 8;
	uint8_t lo = p->payload_size & 0xFF;
	*(buf+index++) = hi;
	*(buf+index++) = lo;
	// Payload
	memcpy(buf+index, p->payload, p->payload_size);
	index+=p->payload_size;
	return index;
}

int deserialize_packet(packet *p, const uint8_t *incoming, size_t incoming_size) {
	if (incoming_size < NET_HEADER_LENGTH) return 1;

	size_t index = 0;

	// TTL
	p->ttl = *(incoming+index++);
	// Protocol
	if (*(incoming+index) != NET_PROTOCOL_UDP && *(incoming+index) != NET_PROTOCOL_TCP) return 1;
	p->protocol = *(incoming+index++);
	// Source
	memcpy(p->source, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Destination
	memcpy(p->destination, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Payload size
	p->payload_size = (uint16_t)((incoming[index] << 8) | incoming[index+1]);
	index += NET_PAYLOAD_SIZE_LENGTH;
	if (p->payload_size > NET_MTU) return 1;
	
	// Payload
	if (incoming_size != NET_HEADER_LENGTH + p->payload_size) return 1;
	memcpy(p->payload, incoming+index, p->payload_size);
	index+=p->payload_size;

	return 0;
}

bool ip_in_subnet(uint8_t *source, uint8_t *destination, uint8_t *netmask) {
	for (size_t i = 0; i < NET_IP_LENGTH; i++) {
		if ((destination[i] & netmask[i]) != (source[i] & netmask[i])) {
			return false;
		}
	}
	return true;
}


int net_init(net_interface *net_iface, interface *iface, uint8_t *ip_address, uint8_t *netmask, uint8_t *gateway) {
	// TODO WIP.
}

int send_packet(net_interface *net_iface, uint8_t protocol, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload) {

	// Decide next hop (either in this subnet, or to router)
	uint8_t next_hop_ip[NET_IP_LENGTH];
	if (!ip_in_subnet(net_iface->ip_address, destination, net_iface->netmask)) {
		memcpy(next_hop_ip, net_iface->gateway, NET_IP_LENGTH);
	}
	else {
		memcpy(next_hop_ip, destination, NET_IP_LENGTH);
	}

	uint8_t next_hop_mac[LINK_MAC_LENGTH];

	// ARP for the MAC address of the next hop

		// TODO ARP lookup
			// 	if found, continue
			//  otherwise, arp_request, return some pending value to caller
			// 		caller will retry later? either with autoqueue or on-arp-response with a table ADDRESS | PENDING PACKET
		// This will work for both host IPs and router IPs (decided already in next_hop_ip)

	// Once MAC resolved, build and serialize packet, send to L2
	packet p;
	if (build_packet(&p, protocol, net_iface->ip_address, destination, payload_size, payload) != NET_OK) return NET_ERROR;
	uint8_t buf[NET_HEADER_LENGTH + NET_MTU];
	size_t p_len = serialize_packet(&p, buf, sizeof buf);
	if (p_len == 0) return NET_ERROR;

	if (send_frame(net_iface->iface, LINK_TYPE_PACKET, next_hop_mac, p_len, buf) != LINK_OK) return NET_ERROR;
	return NET_OK;
}