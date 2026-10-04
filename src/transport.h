#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdint.h>
#include "network.h"

#define TP_PORT_LENGTH 2
#define TP_PAYLOAD_SIZE_LENGTH 2
#define TP_CHECKSUM_LENGTH 2

#define TP_UDP_HEADER_LENGTH (2 * TP_PORT_LENGTH - TP_PAYLOAD_SIZE_LENGTH)
#define TP_UDP_MTU (NET_MTU - TP_UDP_HEADER_LENGTH)

#define TP_OK 0
#define TP_ERROR 1
#define TP_PENDING 2
#define TP_CHECKSUM_MISMATCH 3

typedef struct {
	uint16_t src_port;
	uint16_t dest_port;
	uint16_t payload_size;
	uint16_t checksum;
	uint8_t payload[TP_UDP_MTU];
} datagram;

// typedef struct port_entry {
	// port & handler
// }

int build_datagram(datagram *d, uint16_t src_port, uint16_t dest_port, const uint8_t *payload, uint16_t payload_size);

size_t serialize_datagram(const datagram *d, uint8_t *buf, size_t buf_size);

int deserialize_datagram(datagram *d, const uint8_t *incoming, size_t incoming_size);

int send_datagram(net_interface *net_iface, const uint8_t *dest_ip, uint16_t src_port, uint16_t dest_port, const uint8_t *payload, uint16_t payload_size);

int recv_datagram(net_interface *net_iface, datagram *d, const uint8_t *payload, size_t payload_size);

// assign_port

// disconnect from port

#endif