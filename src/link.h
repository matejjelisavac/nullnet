#ifndef LINK_H
#define LINK_H

#include <stdint.h>
#include <stdio.h>
#include <sys/un.h>

// Preamble: sanity check for incoming frames. Magic number by protocol design. 2 bytes

// Length of frame preamble
#define LINK_PREAMBLE_LENGTH 2
// First half of preamble password
#define LINK_PREAMBLE_HI 0xAB
// Second half of preamble password
#define LINK_PREAMBLE_LO 0xAB

#define LINK_TYPE_PACKET 0x01
#define LINK_TYPE_ARP    0x02
#define LINK_TYPE_LENGTH 1

// Bytes needed for a MAC Address in nullnet
#define LINK_MAC_LENGTH 4

#define LINK_PAYLOAD_SIZE_LENGTH 2

// Maximum payload length for a frame (bytes). Header allows up to 65535 but should not exceed 1500.
#define LINK_MTU 1500

#define LINK_BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF}

#define LINK_OK        0
#define LINK_ERROR     1
#define LINK_NOT_MINE  2

// Expected length of a frame header. The sum of all header fields' lengths (preamble, src, dst, payload size).
#define LINK_HEADER_LENGTH (LINK_PREAMBLE_LENGTH + LINK_TYPE_LENGTH + 2 * LINK_MAC_LENGTH + LINK_PAYLOAD_SIZE_LENGTH)

typedef struct {
	uint8_t preamble[LINK_PREAMBLE_LENGTH];
	uint8_t type;
	uint8_t source[LINK_MAC_LENGTH];
	uint8_t destination[LINK_MAC_LENGTH]; 
	uint16_t payload_size;
	uint8_t payload[LINK_MTU];
} frame;

// The connection descriptor between a host and its link (hub).
typedef struct {
	uint8_t mac_address[LINK_MAC_LENGTH];
	int socket_fd;
	struct sockaddr_un hub;
} interface;

int send_frame(interface *iface, uint8_t type, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload);

int recv_frame(interface *iface, frame *f);

int link_init(interface *iface, uint8_t mac_address[LINK_MAC_LENGTH], char *own_path, char *hub_path);

#endif