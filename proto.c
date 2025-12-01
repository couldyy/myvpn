#include "proto.h"
#include "utils.h"
#include <limits.h>

Vpn_packet construct_vpn_packet(Vpn_header header, uint8_t* payload, size_t payload_size)
{
    assert(payload_size < USHRT_MAX - sizeof(Vpn_header)-1);
    size_t header_size = sizeof(Vpn_header);
    uint16_t checksum = calculate_chechsum(header, header_size, payload, payload_size);
    header.checksum = htons(checksum);

    uint16_t packet_size = header_size + payload_size;
    uint16_t* packet = malloc(packet_size);

    memset(packet, 0, packet_size);
    memcpy(packet, header, header_size);
    memcpy(packet+header_size, payload, payload_size);

    return Vpn_packet{ .data = packet, .data_size = packet_size;
}
