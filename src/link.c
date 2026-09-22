#include <stdint.h>
#include <string.h>
#include "link.h"
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include "wire.h"

int build_frame(frame *f, uint8_t type, const uint8_t *source, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload) {
	if (payload_size > LINK_MTU) return LINK_ERROR;

	f->preamble[0] = LINK_PREAMBLE_HI;
	f->preamble[1] = LINK_PREAMBLE_LO;
	f->type = type;
	f->payload_size = payload_size;
	memcpy(f->destination, destination, LINK_MAC_LENGTH);
	memcpy(f->source, source, LINK_MAC_LENGTH);
	memcpy(f->payload, payload, payload_size);
	return LINK_OK;
}

size_t serialize_frame(const frame *f, uint8_t *buf, size_t buf_size) {
	size_t needed = LINK_HEADER_LENGTH + f->payload_size;
	if (buf_size < needed) return 0;

	size_t index = 0;
	// Preamble
	memcpy(buf+index, f->preamble, LINK_PREAMBLE_LENGTH);
	index+=LINK_PREAMBLE_LENGTH;

	// Type
	*(buf+index) = f->type;
	index+=LINK_TYPE_LENGTH;

	// Source
	memcpy(buf+index, f->source, LINK_MAC_LENGTH);
	index+=LINK_MAC_LENGTH;
	
	// Destination
	memcpy(buf+index, f->destination, LINK_MAC_LENGTH);
	index+=LINK_MAC_LENGTH;
	
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
	if (incoming_size < LINK_HEADER_LENGTH) return 1;

	size_t index = 0;
	// Preamble
	memcpy(f->preamble, incoming+index, LINK_PREAMBLE_LENGTH);

	static const uint8_t link_preamble[LINK_PREAMBLE_LENGTH] = {LINK_PREAMBLE_HI, LINK_PREAMBLE_LO};
	if (memcmp(link_preamble, f->preamble, LINK_PREAMBLE_LENGTH) != 0) return 1;

	index+=LINK_PREAMBLE_LENGTH;

	// Type
	f->type = *(incoming+index);
	index+=LINK_TYPE_LENGTH;

	// Source
	memcpy(f->source, incoming+index, LINK_MAC_LENGTH);
	index+=LINK_MAC_LENGTH;
	
	// Destination
	memcpy(f->destination, incoming+index, LINK_MAC_LENGTH);
	index+=LINK_MAC_LENGTH;
	
	// Payload Length
	f->payload_size = (uint16_t) ((*(incoming+index) << 8) | *(incoming+index+1));
	if (f->payload_size>LINK_MTU) return 1;
	index+=LINK_PAYLOAD_SIZE_LENGTH;
	
	//Payload
	if (incoming_size != LINK_HEADER_LENGTH + f->payload_size) return 1;
	memcpy(f->payload, incoming+index, f->payload_size);
	index+=f->payload_size;

	return 0;
}

int send_frame(interface *iface, uint8_t type, const uint8_t *destination, uint16_t payload_size, const uint8_t *payload) {
	frame f;
	if (build_frame(&f, type, iface->mac_address, destination, payload_size, payload) != 0 ) return 1;

	uint8_t buf[LINK_HEADER_LENGTH + LINK_MTU];
	size_t f_len = serialize_frame(&f, buf, sizeof buf);
	if (f_len == 0) return 1;
	
	if (sendto(iface->socket_fd, buf, f_len, 0, (struct sockaddr *) &(iface->hub), sizeof iface->hub) == -1) return 1;
	return 0;
}

int recv_frame(interface *iface, frame *f) {
	uint8_t buf[LINK_HEADER_LENGTH + LINK_MTU];
	ssize_t incoming_size = recv(iface->socket_fd, buf, sizeof buf, 0);
	if (incoming_size == -1) return LINK_ERROR;

	if (deserialize_frame(f, buf, incoming_size) != 0) return LINK_ERROR;
	uint8_t broadcast_mac[LINK_MAC_LENGTH] = LINK_BROADCAST_MAC;
	if (memcmp(f->destination, iface->mac_address, LINK_MAC_LENGTH) != 0 && memcmp(f->destination, broadcast_mac, LINK_MAC_LENGTH) != 0) return LINK_NOT_MINE;

	return LINK_OK;
}

int link_init(interface *iface, uint8_t mac_address[LINK_MAC_LENGTH], char *own_path, char *hub_path) {
	// Listening socket
	struct sockaddr_un s = {0};
	s.sun_family = AF_UNIX;
	strcpy(s.sun_path, own_path);

	unlink(own_path);
	int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (socket_fd == -1) return LINK_ERROR; 
	iface->socket_fd = socket_fd;
	if (bind(iface->socket_fd, (struct sockaddr *) &s, sizeof s) != 0) return LINK_ERROR;

	// Hub Socket
	memset(&iface->hub, 0, sizeof(struct sockaddr_un));
	iface->hub.sun_family = AF_UNIX;
	strcpy(iface->hub.sun_path, hub_path);

	// Fill interface
	memcpy(iface->mac_address, mac_address, LINK_MAC_LENGTH);

	// Send HELLO to hub.
	uint8_t hello = WIRE_HELLO;
	if (sendto(iface->socket_fd, &hello, WIRE_MSG_LENGTH, 0, (struct sockaddr *) &(iface->hub), sizeof iface->hub) == -1) return LINK_ERROR;

	return LINK_OK;
}