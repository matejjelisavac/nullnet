#include "link.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

int main(int argc, char *argv[]) {

	uint8_t from[LINK_MAC_LENGTH] = {0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t to[LINK_MAC_LENGTH] = {0x00, 0x00, 0x00, 0x00};
	uint16_t payload_size = 800;
	uint8_t payload[payload_size];

	for (size_t i = 0; i < payload_size; i++)
	{
		payload[i] = 0xAA;
	}

	interface iface;
	link_init(&iface, from, "/tmp/nullnet/test-sender", "/tmp/nullnet/hub");
	send_frame(&iface, LINK_TYPE_PACKET, to, payload_size, payload);
	
}