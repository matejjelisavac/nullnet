#ifndef NETWORK_H
#define NETWORK_H

#include "link.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

// Size in bytes of one IP Address.
#define NET_IP_LENGTH 4

#define NET_TTL 15
#define NET_TTL_LENGTH 1

#define NET_PROTOCOL_UDP 0x01
#define NET_PROTOCOL_TCP 0x02
#define NET_PROTOCOL_LENGTH 1

#define NET_PAYLOAD_SIZE_LENGTH 2

// Expected length of a packet header. The sum of all header fields' lengths (TTL, Protocol, src, dst, payload size).
#define NET_HEADER_LENGTH (NET_TTL_LENGTH + NET_PROTOCOL_LENGTH + 2 * NET_IP_LENGTH + NET_PAYLOAD_SIZE_LENGTH)

// Maximum payload length (bytes). Calculated from the maximum frame payload length, minus the packet header length.
#define NET_MTU (LINK_MTU - NET_HEADER_LENGTH)

#define NET_OK 0
#define NET_ERROR 1
#define NET_NOT_MINE 2

#define NET_READY 3
#define NET_TIMEOUT 4

#define ARP_CACHE_SIZE 32
#define PACKET_QUEUE_SIZE 8

#define NET_MAX_ROUTES 8
#define NET_MAX_INTERFACES 8

typedef struct {
	uint8_t ttl;
	uint8_t protocol;
	uint8_t source[NET_IP_LENGTH];
	uint8_t destination[NET_IP_LENGTH];
	uint16_t payload_size;
	uint8_t payload[NET_MTU];
} packet;

typedef struct {
	uint8_t ip_address[NET_IP_LENGTH];
	uint8_t mac_address[LINK_MAC_LENGTH];
	size_t retries;
	time_t timestamp; //since last retry
} arp_entry;

typedef struct {
	uint8_t next_hop_ip[NET_IP_LENGTH];
	uint16_t serialized_size;
	uint8_t serialized[NET_HEADER_LENGTH + NET_MTU];
	time_t timestamp;
} unresolved_packet;

typedef struct {
	interface *iface;
	uint8_t ip_address[NET_IP_LENGTH];
	uint8_t netmask[NET_IP_LENGTH];
	arp_entry arp_cache[ARP_CACHE_SIZE];
	size_t arp_cache_count;
	unresolved_packet packet_queue[PACKET_QUEUE_SIZE];
	size_t packet_queue_count;
} net_interface;

typedef struct {
	uint8_t dest_prefix[NET_IP_LENGTH];
	uint8_t netmask[NET_IP_LENGTH];
	uint8_t next_hop_ip[NET_IP_LENGTH];
	net_interface *out_iface;
} routing_entry;

int build_packet(packet *p, uint8_t ttl, uint8_t protocol, const uint8_t *source, const uint8_t *destination, const uint8_t *payload, uint16_t payload_size);

size_t serialize_packet(const packet *p, uint8_t *buf, size_t buf_size);

int deserialize_packet(packet *p, const uint8_t *incoming, size_t incoming_size);

bool ip_in_subnet(const uint8_t *local, const uint8_t *target, const uint8_t *netmask);

void apply_netmask(uint8_t *result, const uint8_t *ip_address, const uint8_t *netmask);

int add_route(const uint8_t *dest_ip, const uint8_t *dest_netmask, const uint8_t *next_hop_ip, net_interface *out_iface);

int add_default_route(const uint8_t *gateway);

int net_init(interface *iface, const uint8_t *ip_address, const uint8_t *netmask);

void net_tick(void);

int net_await_all(net_interface **ready, int timeout_ms);

int send_packet(uint8_t ttl, uint8_t protocol, const uint8_t *destination, const uint8_t *payload, uint16_t payload_size);

int forward_packet(packet *p);

int recv_packet(net_interface *net_iface, packet *p, const uint8_t *payload, size_t payload_size);

#endif