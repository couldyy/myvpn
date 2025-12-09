#include "proto.h"
#include "utils.h"
#include <limits.h>

Raw_packet* encapsulate_data_packet(uint32_t authentication_num, uint8_t* raw_packet, size_t packet_size)
{
    assert(raw_packet != NULL && packet_size > 0);

    Vpn_header header = {0};
    header.magic = htons(MAGIC_NUM);
    header.flags = VPN_FLAG_DAT;
    header.command = 0;
    header.seq_num = 0;    // UNUSED for data packets
    header.ack_num = 0;    // UNUSED for data packets
    header.authentication = htons(authentication_num);  
    header.packet_size = htons(sizeof(Vpn_header) + packet_size);
    Raw_packet* packet = construct_vpn_packet(&header, sizeof(Vpn_header), raw_packet, packet_size);
    return packet;
}

Raw_packet* construct_vpn_packet(Vpn_header* header, uint8_t* payload, size_t payload_size)
{
    assert(header != NULL);
    assert(payload_size < USHRT_MAX - sizeof(Vpn_header)-1);
    if(payload == NULL && payload_size > 0) {
        fprintf(stderr, "Invalid payload!\n");
        return NULL;
    }
    

    size_t header_size = sizeof(Vpn_header);
    uint16_t checksum = calculate_chechsum(header, header_size, payload, payload_size);
    header.checksum = htons(checksum);

    uint16_t packet_size = header_size + payload_size;
    uint16_t* packet = malloc(packet_size);

    memset(packet, 0, packet_size);

    // write each field of header structure
    off_t packet_offset = 0;
    memcpy(packet + packet_offset, header->magic, sizeof(header->magic));
    packet_offset += sizeof(header->magic);

    memcpy(packet + packet_offset, header->packet_size, sizeof(header->packet_size));
    packet_offset += sizeof(header->packet_size);

    memcpy(packet + packet_offset, header->flags, sizeof(header->flags));
    packet_offset += sizeof(header->flags);

    memcpy(packet + packet_offset, header->command, sizeof(header->command));
    packet_offset += sizeof(header->command);

    memcpy(packet + packet_offset, header->seq_num, sizeof(header->seq_num));
    packet_offset += sizeof(header->seq_num);

    memcpy(packet + packet_offset, header->ack_num, sizeof(header->ack_num));
    packet_offset += sizeof(header->ack_num);

    memcpy(packet + packet_offset, header->authentication, sizeof(header->authentication));
    packet_offset += sizeof(header->authentication);

    // copy payload of packet
    if(payload != NULL)
        memcpy(packet+packet_offset, payload, payload_size);

    return &Raw_packet{ .data = packet, .data_size = packet_size };
}

ssize_t send_data_packet(int socket, struct sockaddr_in* dst, Vpn_packet* packet)
{
    
}
