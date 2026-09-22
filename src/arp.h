#ifndef ARP_H
#define ARP_H

#include "network.h"
#include "link.h"

#define ARP_REQUEST 0x01
#define ARP_RESPONSE 0x02
#define ARP_OPERATION_LENGTH 1

#define ARP_OK 0
#define ARP_ERROR 1
#define ARP_PENDING 2

#define ARP_MSG_LENGTH (ARP_OPERATION_LENGTH + LINK_MAC_LENGTH + 2 * NET_IP_LENGTH)

typedef struct {
	uint8_t operation;
	uint8_t sender_mac[LINK_MAC_LENGTH];
	uint8_t sender_ip[NET_IP_LENGTH];
	uint8_t target_ip[NET_IP_LENGTH];
} arp_msg;

#endif