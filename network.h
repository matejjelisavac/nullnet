#ifndef NETWORK_H
#define NETWORK_H

#include "link.h"
// Size in bytes of one IP Address.
#define IP_ADDRESS_LENGTH 4

#define DEFAULT_TTL 15
#define TTL_LENGTH 1

#define UDP_PROTOCOL 0x01
#define TCP_PROTOCOL 0x02
#define PROTOCOL_LENGTH 1

#define PACKET_PAYLOAD_SIZE_LENGTH 2

// Expected length of a packet header. The sum of all header fields' lengths (TTL, Protocol, src, dst, payload size).
#define PACKET_HEADER_LENGTH (TTL_LENGTH + PROTOCOL_LENGTH + 2 * IP_ADDRESS_LENGTH + PACKET_PAYLOAD_SIZE_LENGTH)

// Maximum payload length (bytes). Calculated from the maximum frame payload length, minus the packet header length.
#define NETWORK_MTU (LINK_MTU - PACKET_HEADER_LENGTH)

typedef struct {
	uint8_t ttl;
	uint8_t protocol;
	uint8_t source[IP_ADDRESS_LENGTH];
	uint8_t destination[IP_ADDRESS_LENGTH];
	uint16_t payload_size;
	uint8_t payload[NETWORK_MTU];
} packet;

#endif

int build_packet(packet *p, uint8_t protocol, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload);

size_t serialize_packet(const packet *p, uint8_t *buf, size_t buf_size);

int deserialize_packet(packet *p, const uint8_t *incoming, size_t incoming_size);
