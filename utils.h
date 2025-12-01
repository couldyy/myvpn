#ifndef UTILS_H
#define UTILS_H
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void _print_packet(uint8_t* packet, ssize_t size);
uint16_t calculate_checksum(Vpn_header* header, size_t header_size, uint8_t* payload, size_t payload_size);
#endif //UTILS_H

