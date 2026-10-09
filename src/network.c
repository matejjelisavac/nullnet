#include "network.h"
#include "arp.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

static routing_entry routes[NET_MAX_ROUTES] = {0}; // Every host has exactly one routing table.
static size_t route_count = 0;
static net_interface net_ifaces[NET_MAX_INTERFACES] = {0};
static size_t net_iface_count = 0;

int build_packet(packet *p, uint8_t ttl, uint8_t protocol, const uint8_t *source, const uint8_t *destination, const uint8_t *payload, uint16_t payload_size) {
	if (payload_size > NET_MTU) return 1;
	if (protocol != NET_PROTOCOL_UDP && protocol != NET_PROTOCOL_TCP) return NET_ERROR;
	p->ttl = ttl;
	p->protocol = protocol;
	memcpy(p->source, source, NET_IP_LENGTH);
	memcpy(p->destination, destination, NET_IP_LENGTH);
	p->payload_size = payload_size;
	memcpy(p->payload, payload, payload_size);
	return NET_OK;
}

size_t serialize_packet(const packet *p, uint8_t *buf, size_t buf_size) {
	size_t needed = NET_HEADER_LENGTH + p->payload_size;
	if (buf_size < needed) return 0;

	size_t index = 0;

	// TTL
	*(buf+index++) = p->ttl;
	// Protocol
	*(buf+index++) = p->protocol;
	// Source
	memcpy(buf+index, p->source, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Destination
	memcpy(buf+index, p->destination, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Payload size
	uint8_t hi = p->payload_size >> 8;
	uint8_t lo = p->payload_size & 0xFF;
	*(buf+index++) = hi;
	*(buf+index++) = lo;
	// Payload
	memcpy(buf+index, p->payload, p->payload_size);
	index+=p->payload_size;
	return index;
}

int deserialize_packet(packet *p, const uint8_t *incoming, size_t incoming_size) {
	if (incoming_size < NET_HEADER_LENGTH) return NET_ERROR;

	size_t index = 0;

	// TTL
	p->ttl = *(incoming+index);
	index+=NET_TTL_LENGTH;
	// Protocol
	if (*(incoming+index) != NET_PROTOCOL_UDP && *(incoming+index) != NET_PROTOCOL_TCP) return NET_ERROR;
	p->protocol = *(incoming+index);
	index+=NET_PROTOCOL_LENGTH;
	// Source
	memcpy(p->source, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Destination
	memcpy(p->destination, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Payload size
	p->payload_size = (uint16_t)((incoming[index] << 8) | incoming[index+1]);
	index += NET_PAYLOAD_SIZE_LENGTH;
	if (p->payload_size > NET_MTU) return NET_ERROR;
	
	// Payload
	if (incoming_size != NET_HEADER_LENGTH + p->payload_size) return NET_ERROR;
	memcpy(p->payload, incoming+index, p->payload_size);
	index+=p->payload_size;

	return NET_OK;
}

bool ip_in_subnet(const uint8_t *local, const uint8_t *target, const uint8_t *netmask) {
	for (size_t i = 0; i < NET_IP_LENGTH; i++) {
		if ((target[i] & netmask[i]) != (local[i] & netmask[i])) {
			return false;
		}
	}
	return true;
}

void apply_netmask(uint8_t *result, const uint8_t *ip_address, const uint8_t *netmask) {
	for (size_t i = 0; i < NET_IP_LENGTH; i++) {
		result[i] = ip_address[i] & netmask[i];
	}
}

int add_route(const uint8_t *dest_ip, const uint8_t *dest_netmask, const uint8_t *next_hop_ip, net_interface *out_iface) {
	if (route_count == NET_MAX_ROUTES) return NET_ERROR;
	routing_entry entry;
	apply_netmask(entry.dest_prefix, dest_ip, dest_netmask); //dest-ip is any ip on destination subnet
	memcpy(entry.netmask, dest_netmask, NET_IP_LENGTH);
	entry.out_iface = out_iface;
	memcpy(entry.next_hop_ip, next_hop_ip, NET_IP_LENGTH);
	routes[route_count] = entry;
	route_count++;
	return NET_OK;
}

int add_default_route(const uint8_t *gateway) {
	// Find interface which this gateway is on the same network as
	net_interface *out_iface = NULL;
	for (size_t i = 0; i < net_iface_count; i++) {
		if (ip_in_subnet(net_ifaces[i].ip_address, gateway, net_ifaces[i].netmask)) {
			out_iface = &net_ifaces[i];
			break;
		}
	}
	if (out_iface == NULL) return NET_ERROR; //Gateway unreachable (no shared wire)

	// Add default route
	uint8_t zero_ip[NET_IP_LENGTH] = {0};
	return add_route(zero_ip, zero_ip, gateway, out_iface);
}

static int add_net_iface(interface *iface, const uint8_t *ip_address, const uint8_t *netmask) {
	if (net_iface_count == NET_MAX_INTERFACES) return NET_ERROR;

	net_interface net_iface;
	net_iface.iface = iface;
	memcpy(net_iface.ip_address, ip_address, NET_IP_LENGTH);
	memcpy(net_iface.netmask, netmask, NET_IP_LENGTH);
	net_iface.arp_cache_count = 0;
	net_iface.packet_queue_count = 0;

	net_ifaces[net_iface_count] = net_iface;
	net_iface_count++;
	return NET_OK;
}

int net_init(interface *iface, const uint8_t *ip_address, const uint8_t *netmask) {
	// Fill interface
	if (add_net_iface(iface, ip_address, netmask) != NET_OK) return NET_ERROR;

	// Add routing entry for own subnet
	uint8_t no_hop[NET_IP_LENGTH] = {0}; // 0.0.0.0
	if (add_route(ip_address, netmask, no_hop, &net_ifaces[net_iface_count - 1]) != NET_OK) return NET_ERROR;
	
	return NET_OK;
}

static int queue_packet(net_interface *net_iface, const uint8_t *next_hop_ip, const uint8_t *serialized, size_t serialized_size) {
	// DO NOT REPLACE OLDEST IF FULL; Packet is real data. Simpler rule is to drop new.
	if (net_iface->packet_queue_count == PACKET_QUEUE_SIZE) return NET_ERROR;
	memcpy(net_iface->packet_queue[net_iface->packet_queue_count].next_hop_ip, next_hop_ip, NET_IP_LENGTH);
	net_iface->packet_queue[net_iface->packet_queue_count].serialized_size = serialized_size;
	memcpy(net_iface->packet_queue[net_iface->packet_queue_count].serialized, serialized, serialized_size);
	net_iface->packet_queue[net_iface->packet_queue_count].timestamp = time(NULL);
	net_iface->packet_queue_count++;
	return NET_OK;
}

static void update_packet_queue(net_interface *net_iface) {
	size_t index = 0;
	while (index < net_iface->packet_queue_count) {
		uint8_t mac[LINK_MAC_LENGTH];
		unresolved_packet *up = &net_iface->packet_queue[index];
	
		// FLUSH
		if (arp_cache_lookup(net_iface, mac, up->next_hop_ip) == ARP_FOUND) {
			// Best-effort. We don't care if it sent or not.
			send_frame(net_iface->iface, LINK_TYPE_PACKET, mac, up->serialized, up->serialized_size);
			*up = net_iface->packet_queue[net_iface->packet_queue_count-1];
			net_iface->packet_queue_count--;
			continue;
		}
		// FILTER STALE
		else if (!arp_cache_contains(net_iface, up->next_hop_ip)) {
			*up = net_iface->packet_queue[net_iface->packet_queue_count-1];
			net_iface->packet_queue_count--;
			continue;
		}
		index++;
	}
}

static int resolve_next_hop(net_interface **out_iface, uint8_t *next_hop_ip, const uint8_t *destination_ip) {
	routing_entry *res = NULL;
	uint32_t best = 0;
	for (size_t i = 0; i < route_count; i++) {
		if (ip_in_subnet(routes[i].dest_prefix, destination_ip, routes[i].netmask)) {	
			uint32_t matched = 0;

			// Convert netmask to integer (uint32_t). Since 1s are continous, higher = more matched
			for (size_t r = 0; r < NET_IP_LENGTH; r++) {
				matched = (matched << 8) | routes[i].netmask[r];
			}

			if (res == NULL || matched > best) {
				res = &routes[i];
				best = matched;
			};
		}
	}
	if (res == NULL) return NET_ERROR;
	*out_iface = res->out_iface; // Mutate the pointer

	// If within our subnet, i.e no next-hop
	uint8_t zero_ip[NET_IP_LENGTH] = {0};
	if (memcmp(res->next_hop_ip, zero_ip, NET_IP_LENGTH) == 0) {
		memcpy(next_hop_ip, destination_ip, NET_IP_LENGTH);
		return NET_OK;
	}
	memcpy(next_hop_ip, res->next_hop_ip, NET_IP_LENGTH);
	return NET_OK;
}

static int request_and_queue(net_interface *net_iface, const uint8_t *next_hop_ip, const uint8_t *serialized, size_t serialized_size) {
	if (!arp_cache_contains(net_iface, next_hop_ip)) {
		if (arp_cache_incomplete(net_iface, next_hop_ip) != ARP_OK) return NET_ERROR;
		if (arp_request(net_iface, next_hop_ip) != ARP_SENT) return NET_ERROR;
	}
	return queue_packet(net_iface, next_hop_ip, serialized, serialized_size);
}

// Handles all housekeeping at the network level. Packet queue update filter & flush., etc.
// Flushing could fire on incoming ARP, but dropping has no event - a packet whose address
// was given up on is only droppable, never announced - so both live on the tick together.
void net_tick(void) {
	for (size_t i = 0; i < net_iface_count; i++) {
		arp_tick(&net_ifaces[i]);
		update_packet_queue(&net_ifaces[i]);
	}
}

int net_await_all(net_interface **ready, int timeout_ms) {
	interface *iface = NULL;
	int res = link_await_all(&iface, timeout_ms);
	if (res == LINK_TIMEOUT) return NET_TIMEOUT;
	if (res != LINK_READY) return NET_ERROR;

	for (size_t i = 0; i < net_iface_count; i++) {
		if (net_ifaces[i].iface == iface) {
			*ready = &net_ifaces[i];
			return NET_READY;
		}
	}
	return NET_ERROR; // A frame arrived on an interface with no layer 3 above it.
}

int send_packet(uint8_t ttl, uint8_t protocol, const uint8_t *destination, const uint8_t *payload, uint16_t payload_size) {
	
	// Decide next hop (either in this subnet, or to router)
	uint8_t next_hop_ip[NET_IP_LENGTH];
	net_interface *out_iface = NULL;
	if (resolve_next_hop(&out_iface, next_hop_ip, destination) != NET_OK) return NET_ERROR;

	// Build and serialize packet.
	packet p;
	if (build_packet(&p, ttl, protocol, out_iface->ip_address, destination, payload, payload_size) != NET_OK) return NET_ERROR;
	uint8_t buf[NET_HEADER_LENGTH + NET_MTU];
	size_t p_size = serialize_packet(&p, buf, sizeof buf);
	if (p_size == 0) return NET_ERROR;


	// Find the mac of next hop
	uint8_t next_hop_mac[LINK_MAC_LENGTH];

	if (arp_cache_lookup(out_iface, next_hop_mac, next_hop_ip) != ARP_FOUND)
		return request_and_queue(out_iface, next_hop_ip, buf, p_size);

	if (send_frame(out_iface->iface, LINK_TYPE_PACKET, next_hop_mac, buf, p_size) != LINK_OK) return NET_ERROR;
	return NET_OK;
}

int forward_packet(packet *p) {
	if (p->ttl <= 1) return NET_ERROR;
	p->ttl--;
	uint8_t buf[NET_HEADER_LENGTH + NET_MTU];
	size_t p_size = serialize_packet(p, buf, sizeof buf);
	if (p_size == 0) return NET_ERROR;

	uint8_t next_hop_ip[NET_IP_LENGTH];
	net_interface *out_iface = NULL;
	if (resolve_next_hop(&out_iface, next_hop_ip, p->destination) != NET_OK) return NET_ERROR;
	
	uint8_t next_hop_mac[LINK_MAC_LENGTH];
	if (arp_cache_lookup(out_iface, next_hop_mac, next_hop_ip) != ARP_FOUND)
		return request_and_queue(out_iface, next_hop_ip, buf, p_size);

	if (send_frame(out_iface->iface, LINK_TYPE_PACKET, next_hop_mac, buf, p_size) != LINK_OK) return NET_ERROR;
	return NET_OK;
}

int recv_packet(net_interface *net_iface, packet *p, const uint8_t *payload, size_t payload_size) {
	// Eventually will need broadcast acceptance for DHCP.
	if (deserialize_packet(p, payload, payload_size) != NET_OK) return NET_ERROR;
	if (memcmp(p->destination, net_iface->ip_address, NET_IP_LENGTH) != 0) return NET_NOT_MINE; //Let caller decide. Useful for routers.
	// TTL check will live in router design.
	return NET_OK;
}
