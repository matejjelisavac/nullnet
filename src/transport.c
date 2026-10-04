#include "transport.h"
#include <stdint.h>
#include <string.h>

static uint16_t udp_checksum(const datagram *d) {
	uint32_t sum = d->src_port + d->dest_port + d->payload_size;
	size_t index = 0;
	while (index < d->payload_size) {
		if (index == d->payload_size - 1) sum += (uint16_t) (d->payload[index] << 8);
		else sum += (uint16_t) (d->payload[index] << 8 | d->payload[index+1]);
		index+=2;
	}
	while (sum > UINT16_MAX) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	return ~sum;
}

int build_datagram(datagram *d, uint16_t src_port, uint16_t dest_port, const uint8_t *payload, uint16_t payload_size) {
	if (payload_size > TP_UDP_MTU) return TP_ERROR;
	d->src_port = src_port;
	d->dest_port = dest_port;
	d->payload_size = payload_size;
	memcpy(d->payload, payload, payload_size);
	d->checksum = udp_checksum(d);
	return TP_OK;
}

size_t serialize_datagram(const datagram *d, uint8_t *buf, size_t buf_size) {
	if (buf_size < TP_UDP_HEADER_LENGTH + d->payload_size) return 0;

	size_t index = 0;
	// Source Port
	*(buf+index++) = d->src_port >> 8;
	*(buf+index++) = d->src_port & 0xFF;

	// Destination Port
	*(buf+index++) = d->dest_port >> 8;
	*(buf+index++) = d->dest_port & 0xFF;

	// Payload size
	*(buf+index++) = d->payload_size >> 8;
	*(buf+index++) = d->payload_size & 0xFF;

	// Checksum
	*(buf+index++) = d->checksum >> 8;
	*(buf+index++) = d->checksum & 0xFF;

	// Payload
	memcpy(buf+index, d->payload, d->payload_size);
	index+=d->payload_size;

	return index;
}

int deserialize_datagram(datagram *d, const uint8_t *incoming, size_t incoming_size) {
	if (incoming_size < TP_UDP_HEADER_LENGTH) return TP_ERROR;

	size_t index = 0;
	// Source Port
	d->src_port = (uint16_t) ((*(incoming+index) << 8) | *(incoming+index+1));
	index+=TP_PORT_LENGTH;

	// Destination Port
	d->dest_port = (uint16_t) ((*(incoming+index) << 8) | *(incoming+index+1));
	index+=TP_PORT_LENGTH;

	// Payload size
	d->payload_size = (uint16_t) ((*(incoming+index) << 8) | *(incoming+index+1));
	if (d->payload_size>TP_UDP_MTU) return TP_ERROR;
	index+=TP_PAYLOAD_SIZE_LENGTH;

	// Checksum
	d->checksum = (uint16_t) ((*(incoming+index) << 8) | *(incoming+index+1));
	index+=TP_CHECKSUM_LENGTH;
	
	// Payload
	if (incoming_size != TP_UDP_HEADER_LENGTH + d->payload_size) return TP_ERROR;
	memcpy(d->payload, incoming+index, d->payload_size);
	index+=d->payload_size;
	
	if (udp_checksum(d) != d->checksum) return TP_CHECKSUM_MISMATCH;
	return TP_OK;
}

int send_datagram(net_interface *net_iface, const uint8_t *dest_ip, uint16_t src_port, uint16_t dest_port, const uint8_t *payload, uint16_t payload_size) {
	datagram d;
	if (build_datagram(&d, src_port, dest_port, payload, payload_size) != TP_OK) return TP_ERROR;

	uint8_t buf[TP_UDP_HEADER_LENGTH + TP_UDP_MTU];
	size_t d_len = serialize_datagram(&d, buf, sizeof buf);
	if (d_len == 0) return TP_ERROR;


	if (send_packet(net_iface, NET_TTL, NET_PROTOCOL_UDP, dest_ip, buf, d_len) != NET_OK) return TP_ERROR;
	return TP_OK;
}

int recv_datagram(net_interface *net_iface, datagram *d, const uint8_t *payload, size_t payload_size) {
	if (deserialize_datagram(d, payload, payload_size) != NET_OK) return TP_ERROR;
	// Give datagram to port.
	return NET_OK;
}