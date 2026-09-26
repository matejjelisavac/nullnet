#include "arp.h"
#include <string.h>

int build_arp(arp_msg *a, uint8_t operation, const uint8_t *source_mac, const uint8_t *source_ip, const uint8_t *destination_ip) {
	a->operation = operation;
	memcpy(a->source_mac, source_mac, LINK_MAC_LENGTH);
	memcpy(a->source_ip, source_ip, NET_IP_LENGTH);
	memcpy(a->destination_ip, destination_ip, NET_IP_LENGTH);
	return ARP_OK;
}

size_t serialize_arp(const arp_msg *a, uint8_t *buf, size_t buf_size) {
	if (buf_size < ARP_MSG_LENGTH) return 0;

	size_t index = 0;

	// Operation
	*(buf+index++) = a->operation;
	// Source MAC
	memcpy(buf+index, a->source_mac, LINK_MAC_LENGTH);
	index+=LINK_MAC_LENGTH;
	// Source IP
	memcpy(buf+index, a->source_ip, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Destination IP
	memcpy(buf+index, a->destination_ip, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	
	return index;
}

int deserialize_arp(arp_msg *a, const uint8_t *incoming, size_t incoming_size) {
	if (incoming_size != ARP_MSG_LENGTH) return ARP_ERROR;

	size_t index = 0;

	// Operation
	a->operation = *(incoming+index++);
	// Source MAC
	memcpy(a->source_mac,incoming+index, LINK_MAC_LENGTH);
	index+=LINK_MAC_LENGTH;
	// Source IP
	memcpy(a->source_ip, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Destination IP
	memcpy(a->destination_ip, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;

	return ARP_OK;
}

void filter_stale_cache(arp_entry *cache, size_t *cache_size) {
	time_t now = time(NULL); //Only compute once. Technically incorrect but negligibly so.
	size_t index = 0;
	while (index < *cache_size) {
		if (now - cache[index].timestamp >= ARP_CACHE_EXPIRY) {
			cache[index] = cache[*cache_size-1];
			*cache_size = *cache_size-1;
			continue;
		}
		index++;
	}
}

int arp_lookup(net_interface *net_iface, const uint8_t *destination_ip, uint8_t *destination_mac_buf) {
	filter_stale_cache(net_iface->arp_cache, &(net_iface->arp_cache_count));
	// Lookup table.
	for (size_t i = 0; i < net_iface->arp_cache_count; i++) {
		if (memcmp(net_iface->arp_cache[i].ip_address, destination_ip, NET_IP_LENGTH) == 0) {
			memcpy(destination_mac_buf, net_iface->arp_cache[i].mac_address, LINK_MAC_LENGTH);
			return ARP_FOUND;
		}
	}
	return ARP_NOT_FOUND;
}

int arp_request(net_interface *net_iface, const uint8_t *destination_ip) {
	// Send request
	arp_msg a;
	if (build_arp(&a, ARP_REQUEST, net_iface->iface->mac_address, net_iface->ip_address, destination_ip) != ARP_OK) return ARP_ERROR;
	uint8_t buf[ARP_MSG_LENGTH];
	size_t arp_len = serialize_arp(&a, buf, sizeof buf);
	if (arp_len == 0) return ARP_ERROR;

	uint8_t broadcast_mac[LINK_MAC_LENGTH] = LINK_BROADCAST_MAC;
	if (send_frame(net_iface->iface, LINK_TYPE_ARP, broadcast_mac, arp_len, buf) != LINK_OK) return ARP_ERROR;
	return ARP_PENDING;
}

int arp_response(net_interface *net_iface, const uint8_t *destination_ip, const uint8_t *destination_mac) {
	// Send response
	arp_msg outgoing;
	if (build_arp(&outgoing, ARP_RESPONSE, net_iface->iface->mac_address, net_iface->ip_address, destination_ip) != ARP_OK) return ARP_ERROR;
	uint8_t buf[ARP_MSG_LENGTH];
	size_t arp_len = serialize_arp(&outgoing, buf, sizeof buf);
	if (arp_len == 0) return ARP_ERROR;
	if (send_frame(net_iface->iface, LINK_TYPE_ARP, destination_mac, arp_len, buf) != LINK_OK) return ARP_ERROR;
	return ARP_OK;
}

void cache_arp(net_interface *net_iface, const uint8_t *incoming_ip, const uint8_t *incoming_mac) {
	filter_stale_cache(net_iface->arp_cache, &(net_iface->arp_cache_count));
	// Duplicate check, finding oldest at the same time to avoid 2 loops.
	size_t oldest_i = 0;
	time_t oldest = net_iface->arp_cache[oldest_i].timestamp;
	for (size_t i = 0; i < net_iface->arp_cache_count; i++) {

		if (memcmp(net_iface->arp_cache[i].ip_address, incoming_ip, NET_IP_LENGTH) == 0) {
			// Duplicate found. Update MAC and time
			memcpy(net_iface->arp_cache[i].mac_address, incoming_mac, LINK_MAC_LENGTH);
			net_iface->arp_cache[i].timestamp = time(NULL);
			return;
		}

		if (net_iface->arp_cache[i].timestamp < oldest) {
			oldest = net_iface->arp_cache[i].timestamp;
			oldest_i = i;
		}
	}

	if (net_iface->arp_cache_count == ARP_CACHE_SIZE) {
		//Replace oldest
		memcpy(net_iface->arp_cache[oldest_i].ip_address, incoming_ip, NET_IP_LENGTH);
		memcpy(net_iface->arp_cache[oldest_i].mac_address, incoming_mac, LINK_MAC_LENGTH);
		net_iface->arp_cache[oldest_i].timestamp = time(NULL);
		return;
	}
	// Else add new entry
	arp_entry entry = {0};
	memcpy(entry.ip_address, incoming_ip, NET_IP_LENGTH);
	memcpy(entry.mac_address, incoming_mac, LINK_MAC_LENGTH);
	entry.timestamp = time(NULL);
	net_iface->arp_cache[net_iface->arp_cache_count] = entry;
	net_iface->arp_cache_count++;
}

int arp_handle(net_interface *net_iface, uint8_t *payload, size_t payload_size) {
	arp_msg incoming;
	if (deserialize_arp(&incoming, payload, payload_size) != ARP_OK) return ARP_ERROR;

	// Learn from all ARPs, even if not a RESPONSE, even if not addressed to me.
	cache_arp(net_iface, incoming.source_ip, incoming.source_mac);
	if (incoming.operation == ARP_REQUEST && memcmp(net_iface->ip_address, incoming.destination_ip, NET_IP_LENGTH) == 0) {
		if (arp_response(net_iface, incoming.source_ip, incoming.source_mac) != ARP_OK) return ARP_ERROR;
	}
	return ARP_OK;
}
