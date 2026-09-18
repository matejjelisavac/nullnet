#include "link.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	char path[] = "/tmp/nullnet-test";
	unlink(path);
	struct sockaddr_un s1 = {0};
	s1.sun_family = AF_UNIX;
	strcpy(s1.sun_path, path);

	while (true) {
		bind(sock, (struct sockaddr *) &s1, sizeof s1);
		
		size_t buf_size = 1600;
		uint8_t buf[buf_size];
		ssize_t received_size = recv(sock, buf, buf_size, 0);
		frame f;
		deserialize_frame(&f, buf, received_size);

		printf("%u\n", f.payload_size);
	}
}