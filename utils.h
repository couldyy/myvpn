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
#include "myvpn_errno.h"

#define UNUSED(var) ((void)var)

#define IPV4_SRC_ADDR_BIAS 12
#define IPV4_DST_ADDR_BIAS 16

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

// parses ip header (currently only ipv4 supported)
// on success returns 'Ip_data' structure with corresponding fileds filled with data
// on error also returns 'Ip_data' struct but values that could not be parsed are set to 0
Ip_data parse_ip_header(uint8_t* ip_packet, size_t packet_size);

// 'port' should be in NETWORK BYTE ORDER
struct sockaddr_in* create_sockaddr_in(int af, uint16_t port, char* addr);

void _print_packet(uint8_t* packet, ssize_t size);
uint16_t calculate_checksum(Vpn_header* header, size_t header_size, uint8_t* payload, size_t payload_size);


uint8_t* get_bytes(uint8_t* num, size_t size);
char* get_bytes_str(uint8_t* num, size_t size);
char* get_bytes_str_num(uint32_t num, size_t size);

uint64_t hash_sockaddr(const void* key);
int cmp_sockaddr(const void* a, const void* b);
uint64_t hash_uint32_ptr(const void* key);
int cmp_uint32_ptr(const void* a, const void* b);

uint8_t* get_bytes(uint8_t* num, size_t size);
char* get_bytes_str(uint8_t* num, size_t size);
char* get_bytes_str_num(uint32_t num, size_t size);
int get_ipv4_numeric_addr(const char* addr, uint32_t* dst);
#endif //UTILS_H

