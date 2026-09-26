#ifndef ARP_H
#define ARP_H

#include "network.h"
#include "link.h"
#include <time.h>

#define ARP_REQUEST 0x01
#define ARP_RESPONSE 0x02
#define ARP_OPERATION_LENGTH 1

#define ARP_OK 0
#define ARP_ERROR 1

#define ARP_PENDING 2

#define ARP_FOUND 3
#define ARP_NOT_FOUND 4

#define ARP_MSG_LENGTH (ARP_OPERATION_LENGTH + LINK_MAC_LENGTH + 2 * NET_IP_LENGTH)

#define ARP_CACHE_SIZE 32

// Time to expire, in seconds
#define ARP_CACHE_EXPIRY 30

typedef struct {
	uint8_t operation;
	uint8_t source_mac[LINK_MAC_LENGTH];
	uint8_t source_ip[NET_IP_LENGTH];
	uint8_t destination_ip[NET_IP_LENGTH];
} arp_msg;

#include "arp.h"
#include <string.h>

int arp_lookup(net_interface *net_iface, const uint8_t *destination_ip, uint8_t *destination_mac_buf);

int arp_request(net_interface *net_iface, const uint8_t *destination_ip);

int arp_response(net_interface *net_iface, const uint8_t *destination_ip, const uint8_t *destination_mac);

int arp_handle(net_interface *net_iface, uint8_t *payload, size_t payload_size);

#endif