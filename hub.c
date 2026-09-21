#include <stdbool.h>
#include <stdint.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include "wire.h"
#include <string.h>
#include <stdio.h>

#define HUB_MAX_MESSAGE 2048
#define HUB_MAX_PORTS 16

int find_port(struct sockaddr_un *connections, struct sockaddr_un *target, size_t connection_count) {
	for (size_t i = 0; i < connection_count; i++) {
		if (strcmp(connections[i].sun_path, target->sun_path) == 0) return i;
	}
	return -1;
}

int main(int argc, char *argv[]) {
	struct sockaddr_un connections[HUB_MAX_PORTS];
	size_t active_ports = 0;

	// TODO argument checking
	// char *path = argv[1]; // Actual path
	char *path = "/tmp/nullnet/hub";
	struct sockaddr_un s = {0};
	s.sun_family = AF_UNIX;
	strncpy(s.sun_path, path, sizeof s.sun_path - 1);

	unlink(path);
	int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (socket_fd == -1) return 1; 
	if (bind(socket_fd, (struct sockaddr *) &s, sizeof s) != 0) return 1;

	while (true) {
		uint8_t buf[HUB_MAX_MESSAGE];
		struct sockaddr_un sender = {0};
		socklen_t sender_len = sizeof sender;
		ssize_t received = recvfrom(socket_fd, buf, HUB_MAX_MESSAGE, 0, (struct sockaddr *) &sender, &sender_len);
		if (received == -1) continue; //Failed to receive

		// Wire Messages
		if (received == WIRE_MSG_LENGTH) {

			if (buf[0] == WIRE_HELLO) {
				if (active_ports == HUB_MAX_PORTS) continue; //Maximum ports reached
				if (find_port(connections, &sender, active_ports) != -1) continue; //Duplicate
				connections[active_ports] = sender;
				active_ports++;
			}
			else if (buf[0] == WIRE_GOODBYE) {
				int leaving = find_port(connections, &sender, active_ports);
				if (leaving == -1) continue; //Never existed
				connections[leaving] = connections[active_ports-1];
				active_ports--;
			}
			else {
				// Unknown 1-byte message..
				continue;
			}
		}

		// Relay
		else {
			size_t skip = find_port(connections, &sender, active_ports);
			printf("Received and retransmitting\n");
			for (size_t i = 0; i < active_ports; i++) {
				if (i == skip) continue;
				sendto(socket_fd, buf, received, 0, (struct sockaddr *) (connections+i), sizeof connections[i]);
			}
			
		}
	}
}