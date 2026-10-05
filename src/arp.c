#include "arp.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

static int build_arp(arp_msg *a, uint8_t operation, const uint8_t *source_mac, const uint8_t *source_ip, const uint8_t *destination_ip) {
	a->operation = operation;
	memcpy(a->source_mac, source_mac, LINK_MAC_LENGTH);
	memcpy(a->source_ip, source_ip, NET_IP_LENGTH);
	memcpy(a->destination_ip, destination_ip, NET_IP_LENGTH);
	return ARP_OK;
}

static size_t serialize_arp(const arp_msg *a, uint8_t *buf, size_t buf_size) {
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

int arp_cache_lookup(net_interface *net_iface, uint8_t *destination_mac_buf, const uint8_t *destination_ip) {
	// Lookup table.
	uint8_t zero_mac[LINK_MAC_LENGTH] = {0};
	for (size_t i = 0; i < net_iface->arp_cache_count; i++) {
		// If IP matches a COMPLETE entry
		if (memcmp(net_iface->arp_cache[i].ip_address, destination_ip, NET_IP_LENGTH) == 0 && memcmp(net_iface->arp_cache[i].mac_address, zero_mac, LINK_MAC_LENGTH) != 0) {
			memcpy(destination_mac_buf, net_iface->arp_cache[i].mac_address, LINK_MAC_LENGTH);
			return ARP_FOUND;
		}
	}
	return ARP_NOT_FOUND;
}

bool arp_cache_contains(const net_interface *net_iface, const uint8_t *target_ip) {
	for (size_t i = 0; i < net_iface->arp_cache_count; i++) {
		if (memcmp(net_iface->arp_cache[i].ip_address, target_ip, NET_IP_LENGTH) == 0) return true;
	}
	return false;
}

static void arp_cache_store(net_interface *net_iface, const uint8_t *incoming_ip, const uint8_t *incoming_mac) {
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
		//If full we replace oldest. Symmetrical loss either way.
		memcpy(net_iface->arp_cache[oldest_i].ip_address, incoming_ip, NET_IP_LENGTH);
		memcpy(net_iface->arp_cache[oldest_i].mac_address, incoming_mac, LINK_MAC_LENGTH);
		net_iface->arp_cache[oldest_i].timestamp = time(NULL);
		return;
	}

	// Else add new entry
	arp_entry *a = &net_iface->arp_cache[net_iface->arp_cache_count];
	memcpy(a->ip_address, incoming_ip, NET_IP_LENGTH);
	memcpy(a->mac_address, incoming_mac, LINK_MAC_LENGTH);
	a->timestamp = time(NULL);
	a->retries = 0;
	net_iface->arp_cache_count++;
}

int arp_cache_incomplete(net_interface *net_iface, const uint8_t *ip_address) {
	if (net_iface->arp_cache_count == ARP_CACHE_SIZE) return ARP_ERROR;
	arp_entry *a = &net_iface->arp_cache[net_iface->arp_cache_count];
	memcpy(a->ip_address, ip_address, NET_IP_LENGTH);
	uint8_t zero_mac[LINK_MAC_LENGTH] = {0};
	memcpy(a->mac_address, zero_mac, LINK_MAC_LENGTH);
	a->retries = 0;
	a->timestamp = time(NULL);
	net_iface->arp_cache_count++;
	return ARP_OK;
}

static void arp_cache_expire(net_interface *net_iface) {
	time_t now = time(NULL); //Only compute once. Technically incorrect but negligibly so.
	size_t index = 0;
	while (index < net_iface->arp_cache_count) {
		if (now - net_iface->arp_cache[index].timestamp >= ARP_CACHE_EXPIRY) {
			net_iface->arp_cache[index] = net_iface->arp_cache[net_iface->arp_cache_count-1];
			net_iface->arp_cache_count--;
			continue;
		}
		index++;
	}
}

static void arp_cache_retry(net_interface *net_iface) {
	time_t now = time(NULL);
	uint8_t zero_mac[LINK_MAC_LENGTH] = {0}; 

	size_t index = 0;
	while (index < net_iface->arp_cache_count) {
		arp_entry *a = &net_iface->arp_cache[index];

		// if incomplete and past retry timeout
		if (memcmp(a->mac_address, zero_mac, LINK_MAC_LENGTH) != 0 || now - a->timestamp < ARP_CACHE_INCOMPLETE_TIMEOUT) {
			index++;
			continue;
		}
		
		// If max retries drop it
		if (a->retries == ARP_MAX_RETRIES) {
			*a = net_iface->arp_cache[net_iface->arp_cache_count-1];
			net_iface->arp_cache_count--;
			continue;
		}

		// Otherwise retry
		if (arp_request(net_iface, a->ip_address) == ARP_SENT) {
			a->retries++;
			a->timestamp = now;
		}
		index++;
	}
}

int arp_request(net_interface *net_iface, const uint8_t *destination_ip) {
	// Send request
	arp_msg a;
	if (build_arp(&a, ARP_REQUEST, net_iface->iface->mac_address, net_iface->ip_address, destination_ip) != ARP_OK) return ARP_ERROR;
	uint8_t buf[ARP_MSG_LENGTH];
	size_t arp_size = serialize_arp(&a, buf, sizeof buf);
	if (arp_size == 0) return ARP_ERROR;

	uint8_t broadcast_mac[LINK_MAC_LENGTH] = LINK_BROADCAST_MAC;
	if (send_frame(net_iface->iface, LINK_TYPE_ARP, broadcast_mac, buf, arp_size) != LINK_OK) return ARP_ERROR;

	return ARP_SENT;
}

static int arp_response(net_interface *net_iface, const uint8_t *destination_ip, const uint8_t *destination_mac) {
	// Send response
	arp_msg outgoing;
	if (build_arp(&outgoing, ARP_RESPONSE, net_iface->iface->mac_address, net_iface->ip_address, destination_ip) != ARP_OK) return ARP_ERROR;
	uint8_t buf[ARP_MSG_LENGTH];
	size_t arp_size = serialize_arp(&outgoing, buf, sizeof buf);
	if (arp_size == 0) return ARP_ERROR;
	if (send_frame(net_iface->iface, LINK_TYPE_ARP, destination_mac, buf, arp_size) != LINK_OK) return ARP_ERROR;
	return ARP_OK;
}

int arp_handle(net_interface *net_iface, const uint8_t *payload, size_t payload_size) {
	arp_msg incoming;
	if (deserialize_arp(&incoming, payload, payload_size) != ARP_OK) return ARP_ERROR;

	// Learn from all ARPs, even if not a RESPONSE, even if not addressed to me.
	arp_cache_store(net_iface, incoming.source_ip, incoming.source_mac);

	// Send ARP Response if a request is addressed to me.
	if (incoming.operation == ARP_REQUEST && memcmp(net_iface->ip_address, incoming.destination_ip, NET_IP_LENGTH) == 0) {
		if (arp_response(net_iface, incoming.source_ip, incoming.source_mac) != ARP_OK) return ARP_ERROR;
	}
	return ARP_OK;
}

// Filter cache and perform retries.
// Retries can't be event-driven: a lost reply produces no event, and hanging them off
// send_packet would mean a host with nothing left to send never retries.
void arp_tick(net_interface *net_iface) {
	arp_cache_expire(net_iface);
	arp_cache_retry(net_iface);
}
