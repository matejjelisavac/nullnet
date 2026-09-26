#include "network.h"
#include "arp.h"

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
	if (incoming_size < NET_HEADER_LENGTH) return NET_ERROR;

	size_t index = 0;

	// TTL
	p->ttl = *(incoming+index);
	index+=NET_TTL_LENGTH;
	// Protocol
	if (*(incoming+index) != NET_PROTOCOL_UDP && *(incoming+index) != NET_PROTOCOL_TCP) return NET_ERROR;
	p->protocol = *(incoming+index);
	index+=NET_PROTOCOL_LENGTH;
	// Source
	memcpy(p->source, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Destination
	memcpy(p->destination, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Payload size
	p->payload_size = (uint16_t)((incoming[index] << 8) | incoming[index+1]);
	index += NET_PAYLOAD_SIZE_LENGTH;
	if (p->payload_size > NET_MTU) return NET_ERROR;
	
	// Payload
	if (incoming_size != NET_HEADER_LENGTH + p->payload_size) return NET_ERROR;
	memcpy(p->payload, incoming+index, p->payload_size);
	index+=p->payload_size;

	return NET_OK;
}

bool ip_in_subnet(const uint8_t *local, const uint8_t *target, const uint8_t *netmask) {
	for (size_t i = 0; i < NET_IP_LENGTH; i++) {
		if ((target[i] & netmask[i]) != (local[i] & netmask[i])) {
			return false;
		}
	}
	return true;
}


int net_init(net_interface *net_iface, interface *iface, const uint8_t *ip_address, const uint8_t *netmask, const uint8_t *gateway) {
	net_iface->iface = iface;
	memcpy(net_iface->ip_address, ip_address, NET_IP_LENGTH);
	memcpy(net_iface->netmask, netmask, NET_IP_LENGTH);
	memcpy(net_iface->gateway, gateway, NET_IP_LENGTH);
	return NET_OK;
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
	if (arp_lookup(net_iface, next_hop_ip, next_hop_mac) == ARP_NOT_FOUND) { // Only returns Found or Not Found
		if (arp_request(net_iface, next_hop_ip) != ARP_PENDING) return NET_ERROR; // Only returns pending or error
		return NET_PENDING; // Tell the caller the try failed. TODO: Eventually will queue the packet and send it on the ARP reply instead of dropping it.
	}

	// On MAC resolved, build and serialize packet, send to L2
	packet p;
	if (build_packet(&p, protocol, net_iface->ip_address, destination, payload_size, payload) != NET_OK) return NET_ERROR;
	uint8_t buf[NET_HEADER_LENGTH + NET_MTU];
	size_t p_len = serialize_packet(&p, buf, sizeof buf);
	if (p_len == 0) return NET_ERROR;

	if (send_frame(net_iface->iface, LINK_TYPE_PACKET, next_hop_mac, p_len, buf) != LINK_OK) return NET_ERROR;
	return NET_OK;
}

int recv_packet(net_interface *net_iface, uint8_t *payload, size_t payload_size, packet *p) {
	// Eventually will need broadcast acceptance for DHCP.
	if (deserialize_packet(p, payload, payload_size) != NET_OK) return NET_ERROR;
	if (memcmp(p->destination, net_iface->ip_address, NET_IP_LENGTH) != 0) return NET_NOT_MINE; //Let caller decide. Useful for routers.
	// TTL check wi1ll live in router design.
	return NET_OK;
}