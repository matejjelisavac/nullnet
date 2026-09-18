#include "link.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

int main(int argc, char *argv[]) {

	uint8_t from[] = {0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t to[] = {0x00, 0x00, 0x00, 0x00};
	uint16_t payload_size = 800;
	uint8_t payload[payload_size];

	for (size_t i = 0; i < payload_size; i++)
	{
		payload[i] = 0xAA;
	}

	frame f;
	build_frame(&f, from, to, payload_size, payload);

	uint16_t buf_size = 1600;
	uint8_t buf[buf_size];
	size_t f_len = serialize_frame(&f, buf, buf_size);

	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);

	char path[] = "/tmp/nullnet-test";

	struct sockaddr_un s1 = {0};
	s1.sun_family = AF_UNIX;
	strcpy(s1.sun_path, path);

	sendto(sock, buf, f_len, 0, (struct sockaddr *) &s1, sizeof s1);

}