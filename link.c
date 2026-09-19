#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "link.h"

int build_frame(frame *f, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload) {
	if (payload_size > LINK_MTU) return 1;

	f->preamble[0] = DEFAULT_PREAMBLE_HI;
	f->preamble[1] = DEFAULT_PREAMBLE_LO;
	f->payload_size = payload_size;
	memcpy(f->destination, destination, MAC_ADDRESS_LENGTH);
	memcpy(f->source, source, MAC_ADDRESS_LENGTH);
	memcpy(f->payload, payload, payload_size);

	return 0;
}

size_t serialize_frame(const frame *f, uint8_t *buf, size_t buf_size) {
	size_t needed = FRAME_HEADER_LENGTH + f->payload_size;
	if (buf_size < needed) return 0;

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
	uint8_t hi = f->payload_size >> 8;
	uint8_t lo = f->payload_size & 0xFF;
	*(buf+index++) = hi;
	*(buf+index++) = lo;
	
	//Payload
	memcpy(buf+index, f->payload, f->payload_size);
	index+=f->payload_size;

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
	f->payload_size = (uint16_t) ((*(incoming+index) << 8) | *(incoming+index+1));
	if (f->payload_size>LINK_MTU) return 1;
	index+=2;
	
	//Payload
	if (incoming_size != FRAME_HEADER_LENGTH + f->payload_size) return 1;
	memcpy(f->payload, incoming+index, f->payload_size);
	index+=f->payload_size;

	return 0;
}