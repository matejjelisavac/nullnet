#include <stdint.h>
#include <stdio.h>

#ifndef link
#define LINK_H

// Sanity check for incoming frames. Magic number by protocol design. 2 bytes
#define DEFAULT_PREAMBLE_LENGTH 2
#define DEFAULT_PREAMBLE_HI 0xAB
#define DEFAULT_PREAMBLE_LO 0xAB

// Bytes needed for a MAC Address in nullnet
#define MAC_ADDRESS_LENGTH 4

// Maximum payload length (bytes). Header allows up to 65535 but should not exceed 1500.
#define MAX_PAYLOAD 1500

#define FRAME_HEADER_LENGTH (DEFAULT_PREAMBLE_LENGTH + 2 * MAC_ADDRESS_LENGTH + 2)

typedef struct {
	uint8_t preamble[DEFAULT_PREAMBLE_LENGTH];
	uint8_t source[MAC_ADDRESS_LENGTH];
	uint8_t destination[MAC_ADDRESS_LENGTH]; 
	uint16_t payload_size;
	uint8_t payload[MAX_PAYLOAD];
} frame;

int build_frame(frame *f, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload);

size_t serialize_frame(const frame *f, uint8_t *buf, size_t buf_size);

int deserialize_frame(frame *f, const uint8_t *incoming, size_t incoming_size);

#endif