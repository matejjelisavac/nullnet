#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
	uint16_t payload_length;
	uint8_t payload[MAX_PAYLOAD]; //Buffer of 1500 bytes allocated to payload.
	//TODO uint8_t/uint16_t ether_type; 0x01 for L3 packets, 0x02 for ARP
	
} frame;

int build_frame(frame *f, const uint8_t *source, const uint8_t *destination, uint16_t payload_length, const uint8_t *payload) {
	if (payload_length > MAX_PAYLOAD) return 1;

	f->preamble[0] = DEFAULT_PREAMBLE_HI;
	f->preamble[1] = DEFAULT_PREAMBLE_LO;
	f->payload_length = payload_length;
	memcpy(f->destination, destination, MAC_ADDRESS_LENGTH);
	memcpy(f->source, source, MAC_ADDRESS_LENGTH);
	memcpy(f->payload, payload, payload_length);

	return 0;
}

size_t serialize_frame(const frame *f, uint8_t *buf, size_t buf_size) {
	size_t needed = FRAME_HEADER_LENGTH + f->payload_length;
	if (needed > buf_size) return 0;

	size_t index = 0;
	// Preamble
	memcpy(buf+index, f->preamble, DEFAULT_PREAMBLE_LENGTH);
	index+=DEFAULT_PREAMBLE_LENGTH;

	// Source
	memcpy(buf+index, f->source, MAC_ADDRESS_LENGTH);
	index+=MAC_ADDRESS_LENGTH;
	
	// Destination
	memcpy(buf+index, f->destination, MAC_ADDRESS_LENGTH);
	index+=MAC_ADDRESS_LENGTH;
	
	// Payload Length
	uint8_t hi = f->payload_length >> 8;
	uint8_t lo = f->payload_length & 0xFF;
	*(buf+index++) = hi;
	*(buf+index++) = lo;
	
	//Payload
	memcpy(buf+index, f->payload, f->payload_length);
	index+=f->payload_length;

	return index;
}

int deserialize_frame(frame *f, const uint8_t *incoming, size_t incoming_size) {
	if (incoming_size < FRAME_HEADER_LENGTH) return 1;

	size_t index = 0;
	// Preamble
	memcpy(f->preamble, incoming+index, DEFAULT_PREAMBLE_LENGTH);

	static const uint8_t default_preamble[DEFAULT_PREAMBLE_LENGTH] = {DEFAULT_PREAMBLE_HI, DEFAULT_PREAMBLE_LO};
	if (memcmp(default_preamble, f->preamble, DEFAULT_PREAMBLE_LENGTH) != 0) return 1;

	index+=DEFAULT_PREAMBLE_LENGTH;

	// Source
	memcpy(f->source, incoming+index, MAC_ADDRESS_LENGTH);
	index+=MAC_ADDRESS_LENGTH;
	
	// Destination
	memcpy(f->destination, incoming+index, MAC_ADDRESS_LENGTH);
	index+=MAC_ADDRESS_LENGTH;
	
	// Payload Length
	f->payload_length = (uint16_t) ((*(incoming+index) << 8) | *(incoming+index+1));
	if (f->payload_length>MAX_PAYLOAD) return 1;
	index+=2;
	
	//Payload
	if (incoming_size != FRAME_HEADER_LENGTH + f->payload_length) return 1;
	memcpy(f->payload, incoming+index, f->payload_length);
	index+=f->payload_length;

	return 0;
}

// Testing ground
int main(size_t argc, char *argv[]) {
	
}