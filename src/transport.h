#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdint.h>
#include "network.h"

#define TP_PORT_LENGTH 2
#define TP_PAYLOAD_SIZE_LENGTH 2
#define TP_CHECKSUM_LENGTH 2

#define TP_OK 0
#define TP_ERROR 1
#define TP_PENDING 2
#define TP_CHECKSUM_MISMATCH 3

#define TP_MAX_PORTS 16

typedef void (*tp_handler)(const uint8_t *payload, uint16_t payload_size);

typedef struct {
	uint16_t port;
	tp_handler handler;
} port_entry;

#endif