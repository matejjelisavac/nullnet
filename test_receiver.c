#include "link.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

	uint8_t self[LINK_MAC_LENGTH] = {0x00, 0x00, 0x00, 0x00};
	interface iface;
	link_init(&iface, self, "/tmp/nullnet/test-receiver", "/tmp/nullnet/hub");

	frame f;
	recv_frame(&iface, &f);
	printf("%u\n", f.payload_size);
	
}