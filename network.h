#ifndef NETWORK_H
#define NETWORK_H

#include "link.h"
// Size in bytes of one IP Address.
#define NET_IP_LENGTH 4

#define NET_TTL 15
#define NET_TTL_LENGTH 1

#define NET_PROTOCOL_UDP 0x01
#define NET_PROTOCOL_TCP 0x02
#define NET_PROTOCOL_LENGTH 1

#define NET_PAYLOAD_SIZE_LENGTH 2

// Expected length of a packet header. The sum of all header fields' lengths (TTL, Protocol, src, dst, payload size).
#define NET_HEADER_LENGTH (NET_TTL_LENGTH + NET_PROTOCOL_LENGTH + 2 * NET_IP_LENGTH + NET_PAYLOAD_SIZE_LENGTH)

// Maximum payload length (bytes). Calculated from the maximum frame payload length, minus the packet header length.
#define NET_MTU (LINK_MTU - NET_HEADER_LENGTH)

#define NET_OK 0
#define NET_ERROR 1

typedef struct {
	uint8_t ttl;
	uint8_t protocol;
	uint8_t source[NET_IP_LENGTH];
	uint8_t destination[NET_IP_LENGTH];
	uint16_t payload_size;
	uint8_t payload[NET_MTU];
} packet;

typedef struct {
	interface *iface;
	uint8_t ip_address[NET_IP_LENGTH];
	uint8_t netmask[NET_IP_LENGTH];
	uint8_t gateway[NET_IP_LENGTH];
} net_interface;

int build_packet(packet *p, uint8_t protocol, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload);

size_t serialize_packet(const packet *p, uint8_t *buf, size_t buf_size);

int deserialize_packet(packet *p, const uint8_t *incoming, size_t incoming_size);

#endif