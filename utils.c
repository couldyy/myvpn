#include "utils.h"


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
    assert(header != NULL && payload != NULL);
    uint16_t sum = 0;
    for(int i = 0; i < header_size/2; i++) {
        sum += ntohs(((uint16_t*)header)[i]);
    }
    for(int i = 0; i < payload_size/2; i++) {
        sum += ntohs(((uint16_t*)payload)[i]);
    }
    return sum;
}
