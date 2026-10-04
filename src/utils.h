#ifndef UTILS_H
#define UTILS_H

#include "link.h"
#include "network.h"
#include "arp.h"
#include <stdint.h>

#define UTIL_RECEIVED 0
#define UTIL_ERROR 1
#define UTIL_ARP_HANDLED 2
#define UTIL_NOT_MINE 3

void print_mac(const uint8_t *mac);

void print_ip(const uint8_t *ip);

int read_mac(uint8_t *mac_buf, const char *mac);

int read_ip(uint8_t *ip_buf, const char *ip);

int get_packet(net_interface *net_iface, packet *p);

#endif