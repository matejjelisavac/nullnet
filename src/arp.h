#ifndef ARP_H
#define ARP_H

#include "network.h"
#include "link.h"
#include <stdint.h>
#include <stddef.h>

#define ARP_REQUEST 0x01
#define ARP_RESPONSE 0x02
#define ARP_OPERATION_LENGTH 1

#define ARP_OK 0
#define ARP_ERROR 1

#define ARP_SENT 2

#define ARP_FOUND 3
#define ARP_NOT_FOUND 4

#define ARP_MSG_LENGTH (ARP_OPERATION_LENGTH + LINK_MAC_LENGTH + 2 * NET_IP_LENGTH)

// Time to expire, in seconds
#define ARP_CACHE_EXPIRY 30

// Waiting on ARP Response timeout until try again.
#define ARP_CACHE_INCOMPLETE_TIMEOUT 1 // PACKET_QUEUE_EXPIRY > INCOMPLETE_TIMOUT × MAX_RETRIES

#define ARP_MAX_RETRIES 3

typedef struct {
	uint8_t operation;
	uint8_t source_mac[LINK_MAC_LENGTH];
	uint8_t source_ip[NET_IP_LENGTH];
	uint8_t destination_ip[NET_IP_LENGTH];
} arp_msg;

int deserialize_arp(arp_msg *a, const uint8_t *incoming, size_t incoming_size);

int arp_cache_lookup(net_interface *net_iface, uint8_t *destination_mac_buf, const uint8_t *destination_ip);

bool arp_cache_contains(const net_interface *net_iface, const uint8_t *target_ip);

int arp_cache_incomplete(net_interface *net_iface, const uint8_t *ip_address);

int arp_request(net_interface *net_iface, const uint8_t *destination_ip);

int arp_handle(net_interface *net_iface, const uint8_t *payload, size_t payload_size);

void arp_tick(net_interface *net_iface);

#endif