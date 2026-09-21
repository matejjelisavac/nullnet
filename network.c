#include <stdint.h>
#include <string.h>
#include "network.h"

int build_packet(packet *p, uint8_t protocol, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload) {
	if (payload_size > NETWORK_MTU) return 1;
	if (protocol != UDP_PROTOCOL && protocol != TCP_PROTOCOL) return 1;
	p->ttl = DEFAULT_TTL;
	p->protocol = protocol;
	memcpy(p->source, source, IP_ADDRESS_LENGTH);
	memcpy(p->destination, destination, IP_ADDRESS_LENGTH);
	p->payload_size = payload_size;
	memcpy(p->payload, payload, payload_size);
	return 0;
}

size_t serialize_packet(const packet *p, uint8_t *buf, size_t buf_size) {
	size_t needed = PACKET_HEADER_LENGTH + p->payload_size;
	if (buf_size < needed) return 0;

	size_t index = 0;

	// TTL
	*(buf+index++) = p->ttl;
	// Protocol
	*(buf+index++) = p->protocol;
	// Source
	memcpy(buf+index, p->source, IP_ADDRESS_LENGTH);
	index+=IP_ADDRESS_LENGTH;
	// Destination
	memcpy(buf+index, p->destination, IP_ADDRESS_LENGTH);
	index+=IP_ADDRESS_LENGTH;
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
	if (incoming_size < PACKET_HEADER_LENGTH) return 1;

	size_t index = 0;

	// TTL
	p->ttl = *(incoming+index++);
	// Protocol
	if (*(incoming+index) != UDP_PROTOCOL && *(incoming+index) != TCP_PROTOCOL) return 1;
	p->protocol = *(incoming+index++);
	// Source
	memcpy(p->source, incoming+index, IP_ADDRESS_LENGTH);
	index+=IP_ADDRESS_LENGTH;
	// Destination
	memcpy(p->destination, incoming+index, IP_ADDRESS_LENGTH);
	index+=IP_ADDRESS_LENGTH;
	// Payload size
	p->payload_size = (uint16_t)((incoming[index] << 8) | incoming[index+1]);
	index += PACKET_PAYLOAD_SIZE_LENGTH;
	if (p->payload_size > NETWORK_MTU) return 1;
	
	// Payload
	if (incoming_size != PACKET_HEADER_LENGTH + p->payload_size) return 1;
	memcpy(p->payload, incoming+index, p->payload_size);
	index+=p->payload_size;

	return 0;
}

int send_packet(uint8_t protocol, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload) {
	// Build the packet with source (its own address), destination, protocol, TTL
	// Decide the next hop — AND the destination with the netmask, compare against its own network. Same segment means the next hop is the destination itself; different means the next hop is the gateway
	// Resolve that next hop to a MAC via ARP
	// Serialise the packet into a byte buffer
	// send_frame(iface, LINK_TYPE_PACKET, next_hop_mac, buf, len)
}