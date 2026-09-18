#include <stdint.h>

// Size in bytes of one IP Address.
#define IP_ADDRESS_LENGTH 4

// Under UDP this is ....
#define MAX_PAYLOAD 0000

typedef struct {
	uint8_t source[IP_ADDRESS_LENGTH];
	uint8_t destination[IP_ADDRESS_LENGTH];
	uint16_t payload_size;
	uint8_t payload[MAX_PAYLOAD];
} packet;