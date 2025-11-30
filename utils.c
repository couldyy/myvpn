#include "utils.h"


void _print_packet(int* packet, ssize_t size)
{
    assert(size > 0);
    char ascii_line[18] = {0};
    int ascii_line_index = 0;
    uint8_t byte;
    uint8_t byte_host;
    int num_net;
    for(int i = 0; i < size; i++) {
        num_net = packet[i];
        if(i % 4 == 0) {
            ascii_line[17] = '\0';
            printf(" %s\n", ascii_line);
            memset(ascii_line, 0, sizeof(ascii_line)); 
            ascii_line_index = 0;
        }
        for(int j = 0; j < sizeof(int); j++) {
            byte = (uint8_t)(((uint8_t*)&(packet[i]))[j]);
            byte_host = (uint8_t)(((uint8_t*)&(num_net))[j]);
            printf("%.2x%s", byte, (j % 2 == 0)  ? "" : " ");
            ascii_line[ascii_line_index] = (byte_host >= 33 && byte_host <= 126) ? (char)byte_host : '.'; 
            ascii_line_index++;
        }
    }
    printf("\n");
}
