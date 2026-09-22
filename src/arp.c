#include "arp.h"

int build_arp(arp_msg *a, uint8_t operation, const uint8_t *sender_mac, const uint8_t *sender_ip, const uint8_t *target_ip) {
	a->operation = operation;
	memcpy(a->sender_mac, sender_mac, LINK_MAC_LENGTH);
	memcpy(a->sender_ip, sender_ip, NET_IP_LENGTH);
	memcpy(a->target_ip, target_ip, NET_IP_LENGTH);
	return ARP_OK;
}

size_t serialize_arp(const arp_msg *a, uint8_t *buf, size_t buf_size) {
	if (buf_size < ARP_MSG_LENGTH) return 0;

	size_t index = 0;

	// Operation
	*(buf+index++) = a->operation;
	// Sender MAC
	memcpy(buf+index, a->sender_mac, LINK_MAC_LENGTH);
	index+=LINK_MAC_LENGTH;
	// Sender IP
	memcpy(buf+index, a->sender_ip, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Target IP
	memcpy(buf+index, a->target_ip, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	
	return index;
}

int deserialize_arp(arp_msg *a, const uint8_t *incoming, size_t incoming_size) {
	if (incoming_size != ARP_MSG_LENGTH) return ARP_ERROR;

	size_t index = 0;

	// Operation
	a->operation = *(incoming+index++);
	// Sender MAC
	memcpy(a->sender_mac,incoming+index, LINK_MAC_LENGTH);
	index+=LINK_MAC_LENGTH;
	// Sender IP
	memcpy(a->sender_ip, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;
	// Target IP
	memcpy(a->target_ip, incoming+index, NET_IP_LENGTH);
	index+=NET_IP_LENGTH;

	return ARP_OK;
}

int arp_lookup() {
	// Lookup table.
	// Return value should tell caller hey, youre gonna need to arp_request for this.
}

int arp_request(net_interface *net_iface, uint8_t *target_ip) {
	// Send request
	arp_msg a;
	if (build_arp(&a, ARP_REQUEST, net_iface->iface->mac_address, net_iface->ip_address, target_ip) != ARP_OK) return ARP_ERROR;
	uint8_t buf[ARP_MSG_LENGTH];
	size_t arp_len = serialize_arp(&a, buf, sizeof buf);
	if (arp_len == 0) return ARP_ERROR;

	uint8_t broadcast_mac[LINK_MAC_LENGTH] = LINK_BROADCAST_MAC;
	if (send_frame(net_iface->iface, LINK_TYPE_ARP, broadcast_mac, arp_len, buf) != LINK_OK) return ARP_ERROR;
	return ARP_PENDING;
}

int arp_handle(net_interface *net_iface, uint8_t *payload, size_t payload_size) {
	arp_msg incoming;
	if (deserialize_arp(&incoming, payload, payload_size) != ARP_OK) return ARP_ERROR;
	if (incoming.operation == ARP_REQUEST) {
		// Give back msg
	}
	if (incoming.operation == ARP_RESPONSE) {
		// Store address
	}
}
