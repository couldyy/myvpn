#ifndef UTILS_H
#define UTILS_H
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include "proto.h"

#define IPV4_SRC_ADDR_BIAS 16
#define IPV4_DST_ADDR_BIAS 20

#define IPV6_SRC_ADDR_BIAS 8
#define IPV6_DST_ADDR_BIAS 24

typedef struct {
    uint8_t ip_version;
    uint32_t src_addr_v4;
    uint32_t dst_addr_v4;

    // TODO ipv6 support
    // src_addr_v6;
    // dst_addr_v6;
} Ip_data;

Ip_data parse_ip_header(uint8_t* ip_packet, size_t packet_size);

struct sockaddr_in* create_sockaddr_in(int af, uint16_t port, char* addr);

void _print_packet(uint8_t* packet, ssize_t size);
uint16_t calculate_checksum(Vpn_header* header, size_t header_size, uint8_t* payload, size_t payload_size);

Vpn_packet* parse_packet(uint8_t* raw_packet, size_t packet_size);

uint8_t* get_bytes(uint8_t* num, size_t size);
char* get_bytes_str(uint8_t* num, size_t size);
char* get_bytes_str_num(uint32_t num, size_t size);
#endif //UTILS_H

