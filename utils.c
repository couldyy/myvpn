#include "utils.h"

Ip_data parse_ip_header(uint8_t* ip_packet, size_t packet_size)
{
    uint8_t version = ip_packet[0] >> 4;

    uint32_t src_addr_v4 = 0;
    uint32_t dst_addr_v4 = 0;
    if(version == 4) {
        memcpy(&src_addr_v4, ip_packet+IPV4_SRC_ADDR_BIAS, sizeof(src_addr_v4)); 
        memcpy(&dst_addr_v4, ip_packet+IPV4_DST_ADDR_BIAS, sizeof(dst_addr_v4)); 
    }
    else if(version == 6) {
        // TODO parse ipv6 addresses
    }
    
    return (Ip_data) {.ip_version = version, .src_addr_v4 = src_addr_v4, .dst_addr_v4 = dst_addr_v4};
    
}
Vpn_packet* parse_packet(uint8_t* raw_packet, size_t packet_size)
{
    Vpn_packet* parsed_packet = malloc(sizeof(Vpn_packet));

    uint16_t packet_magic;
    off_t packet_offset = 0;
    // copy magic num first
    memcpy(&packet_magic, raw_packet + packet_offset, sizeof(uint16_t));
    packet_offset += sizeof(packet_magic);

    // if first 2 bytes in received packet is not magic num - it is not vpn packet, ju return raw packet
    if(packet_magic != htons(MAGIC_NUM)) {
        parsed_packet->header = NULL;
        parsed_packet->payload = raw_packet;
        parsed_packet->payload_size = packet_size;
        return parsed_packet;
    }

    // else - parse whole packet
    Vpn_header* header = malloc(sizeof(Vpn_header));
    header->magic = packet_magic;

    memcpy(&header->packet_size, raw_packet + packet_offset, sizeof(header->packet_size));
    packet_offset += sizeof(header->packet_size);
    if(ntohs(header->packet_size) != packet_size) {
        fprintf(stderr, "Packet size dont match\n");
    }

    memcpy(&header->flags, raw_packet + packet_offset, sizeof(header->flags));
    packet_offset += sizeof(header->flags);

    memcpy(&header->command, raw_packet + packet_offset, sizeof(header->command));
    packet_offset += sizeof(header->command);

    uint16_t packet_checksum;
    memcpy(&packet_checksum, raw_packet + packet_offset, sizeof(header->checksum));
    header->checksum = 0;   // for future checksum verification
    packet_offset += sizeof(header->checksum);

    memcpy(&header->seq_num, raw_packet + packet_offset, sizeof(header->seq_num));
    packet_offset += sizeof(header->seq_num);

    memcpy(&header->ack_num, raw_packet + packet_offset, sizeof(header->ack_num));
    packet_offset += sizeof(header->ack_num);

    memcpy(raw_packet + packet_offset, &header->authentication, sizeof(header->authentication));
    packet_offset += sizeof(header->authentication);

    uint16_t calc_packet_checksum = htons(calculate_checksum(header, sizeof(Vpn_header), raw_packet + packet_offset, packet_size - packet_offset));

    if(packet_checksum != calc_packet_checksum) {
        fprintf(stderr, "Invalid packet checksum: got: %hu     calculated: %hu \n", packet_checksum, calc_packet_checksum);
    }
    header->checksum = packet_checksum;

    parsed_packet->header = header;
    parsed_packet->payload = raw_packet + packet_offset;
    parsed_packet->payload_size = packet_size - packet_offset;
    
    return parsed_packet;
}

struct sockaddr_in* create_sockaddr_in(int af, uint16_t port, char* address)
{
    struct sockaddr_in* addr = malloc(sizeof(struct sockaddr_in));
    addr->sin_family = af;
    addr->sin_port = htons(port);
    int res = inet_pton(AF_INET, address, &(addr->sin_addr));
    if(res == 0) {
        fprintf(stderr, "Invalid ip '%s'\n", address);
        return NULL;
    }
    else if (res < 0) {
        fprintf(stderr, "Invalid address family\n");
        fprintf(stderr, "%s\n", strerror(errno));
        return NULL;
    }
    return addr;
}

// TODO last byte is not printed
void _print_packet(uint8_t* packet, ssize_t size)
{
    assert(size > 0);
    char ascii_line[18] = {0};
    char byte_line[64] = {0};
    int written = 0;
    uint8_t byte;
    uint8_t display_line_len = 16;
    for(ssize_t i = 0, ascii_line_index = 0; i < size; i++, ascii_line_index++) {
        if((i % display_line_len == 0) || i == (size -1)) {
            ascii_line[17] = '\0';
            byte_line[display_line_len * 2 + (display_line_len / 2 -1)] = '\0';
            printf("%-*s %s\n", display_line_len * 2 + (display_line_len / 2 -1), byte_line, ascii_line);
            memset(ascii_line, 0, sizeof(ascii_line)); 
            memset(byte_line, 0, sizeof(byte_line)); 
            ascii_line_index = 0;
            written = 0;
        }
        byte = packet[i];
        written += snprintf(byte_line + written, sizeof(byte_line) - written, "%.2x%s", byte, (i % 2 == 0)  ? "" : " ");
        ascii_line[ascii_line_index] = (byte >= 33 && byte <= 126) ? (char)byte : '.'; 
    }
    printf("\n");
}


// Idea stolen from TCP standart
// Sum of all 16-bit words (header + payload)
uint16_t calculate_checksum(Vpn_header* header, size_t header_size, uint8_t* payload, size_t payload_size)
{
    assert(header != NULL && header_size > 0);
    if(payload == NULL && payload_size > 0) {
        fprintf(stderr, "Invalid payload while construsting vpn packet\n");
        abort();    // TODO dont abort on invalid payload
    }
    uint16_t sum = 0;
    for(int i = 0; i < header_size/2; i++) {
        sum += ntohs(((uint16_t*)header)[i]);
    }
    for(int i = 0; i < payload_size/2; i++) {
        sum += ntohs(((uint16_t*)payload)[i]);
    }
    return sum;
}
