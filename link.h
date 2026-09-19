#ifndef LINK_H
#define LINK_H

#include <stdint.h>
#include <stdio.h>

// Preambe: sanity check for incoming frames. Magic number by protocol design. 2 bytes

// Length of frame preamble
#define DEFAULT_PREAMBLE_LENGTH 2
// First half of preamble password
#define DEFAULT_PREAMBLE_HI 0xAB
// Second half of preamble password
#define DEFAULT_PREAMBLE_LO 0xAB

// Bytes needed for a MAC Address in nullnet
#define MAC_ADDRESS_LENGTH 4

#define FRAME_PAYLOAD_SIZE_LENGTH 2

// Maximum payload length for a frame (bytes). Header allows up to 65535 but should not exceed 1500.
#define LINK_MTU 1500

// Expected length of a frame header. The sum of all header fields' lengths (preamble, src, dst, payload size).
#define FRAME_HEADER_LENGTH (DEFAULT_PREAMBLE_LENGTH + 2 * MAC_ADDRESS_LENGTH + FRAME_PAYLOAD_SIZE_LENGTH)

typedef struct {
	uint8_t preamble[DEFAULT_PREAMBLE_LENGTH];
	uint8_t source[MAC_ADDRESS_LENGTH];
	uint8_t destination[MAC_ADDRESS_LENGTH]; 
	uint16_t payload_size;
	uint8_t payload[LINK_MTU];
} frame;

int build_frame(frame *f, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload);

size_t serialize_frame(const frame *f, uint8_t *buf, size_t buf_size);

int deserialize_frame(frame *f, const uint8_t *incoming, size_t incoming_size);

#endif