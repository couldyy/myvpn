#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#define USE_U32
#define BITARRAY_IMPLEMENTATION
#include "../thirdparty/bitarray.h"

#define ADDRESS_FREE 0
#define ADDRESS_IN_USE 1

void print_addresses(uint32_t* addresses, int last)
{
    for(int i = 0; i < last; i++) {
        if(addresses[i] != 0) {
            uint32_t addr = htonl(addresses[i]);
            char addr_str[16] = {0};
            inet_ntop(AF_INET, &addr, addr_str, sizeof(addr_str));
            printf("%d. %s (%u)\n", i, addr_str, addresses[i]);
        }
    }
}

int check_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr)
{
    assert(address_pool_mask != NULL);
    assert(address > network_addr);
    return get_bit(address_pool_mask, address-network_addr-1); 
}

int free_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr)
{
    assert(address_pool_mask != NULL);
    assert(address > network_addr);
    if(check_address(address_pool_mask, address, network_addr) == ADDRESS_FREE)
        return -1;
    else
        set_bit(address_pool_mask, address - network_addr - 1, ADDRESS_FREE);
    return 0;
}

uint32_t request_address(Bit_array* address_pool_mask, uint32_t network_addr)
{
    assert(address_pool_mask != NULL);
    ssize_t index = find_first_bit(address_pool_mask, ADDRESS_FREE);
    if(index < 0)
        return 0;
    else
        set_bit(address_pool_mask, index, ADDRESS_IN_USE);
    return network_addr+1+index;
}


uint8_t* get_bytes(uint8_t* num, size_t size)
{
    uint8_t* bytes = malloc(size);
    for(int i = 0; i < size; i++) {
        bytes[i] = num[i];
    }
    return bytes;
}

char* get_bytes_str(uint8_t* num, size_t size) 
{
    uint8_t* bytes = get_bytes(num, size);
    size_t str_size = size*5;
    char* bytes_str = malloc(str_size);
    memset(bytes_str, 0, str_size);
    int written = 0;
    for(int i = 0; i < size; i++) {
        written += snprintf(bytes_str + written, str_size - written, "%.2x ", bytes[i]);
    }
    return bytes_str;
}

char* get_bytes_str_num(uint32_t num, size_t size)
{
    uint8_t* num_bytes = (uint8_t*) &num;
    return get_bytes_str(num_bytes, size);
}


int main(int argc, char** argv)
{
    if(argc != 3) {
        fprintf(stderr, "Invaid usage, please specify [Network address] [Mask]\n"
        "\t e.g. 172.16.0.0 255.240.0.0\n");
        exit(1);
    }
    char* net_addr = argv[1];
    char* net_mask = argv[2];
    
    uint32_t net_addr_num;
    uint32_t net_mask_num;

    int res = inet_pton(AF_INET, net_addr, &net_addr_num);
    if(res = 0) {
        fprintf(stderr, "Invalid address '%s'\n", net_addr);
        exit(1);
    }
    else if(res < 0) {
        fprintf(stderr, "Error converting address '%s': %s\n", net_addr, strerror(errno));
        exit(1);
    }
    
    res = inet_pton(AF_INET, net_mask, &net_mask_num);
    if(res = 0) {
        fprintf(stderr, "Invalid address '%s'\n", net_mask);
        exit(1);
    }
    else if(res < 0) {
        fprintf(stderr, "Error converting address '%s': %s\n", net_mask, strerror(errno));
        exit(1);
    }
    net_addr_num = ntohl(net_addr_num);
    net_mask_num = ntohl(net_mask_num);
    printf("%u - %s\n%u - %s\n", net_addr_num, 
        get_bytes_str_num(net_addr_num, sizeof(uint32_t)),
        net_mask_num,
        get_bytes_str_num(net_mask_num, sizeof(uint32_t)));

    printf("%s : %u -  %s\n",
        net_addr, net_addr_num, get_bytes_str_num(net_addr_num, sizeof(net_addr_num))); 

    printf("%s : %u -  %s\n",
        net_mask, net_mask_num, get_bytes_str_num(net_mask_num, sizeof(net_mask_num))); 

    printf("%u %s\n%u %s\n", net_mask_num, get_bytes_str_num(net_mask_num, 4), 
        ~net_mask_num, get_bytes_str_num(~net_mask_num, 4));

    uint32_t network = net_addr_num & net_mask_num;
    uint32_t range_start = network + 1; 
    uint32_t range_end = network | (~net_mask_num) - 1; 
    range_start = htonl(range_start);
    range_end = htonl(range_end);
    char range_start_str[16] = {0};
    char range_end_str[16] = {0};
    if(inet_ntop(AF_INET, &range_start, range_start_str, sizeof(range_start_str)) == NULL) {
        fprintf(stderr, "failed converting binary address to str: %s\n", strerror(errno));
        exit(1);
    }
    if(inet_ntop(AF_INET, &range_end, range_end_str, sizeof(range_end_str)) == NULL) {
        fprintf(stderr, "failed converting binary address to str: %s\n", strerror(errno));
        exit(1);
    }
    range_start = ntohl(range_start);
    range_end = ntohl(range_end);
    printf("net: %u - %s\n", network, get_bytes_str_num(network, sizeof(network)));
    //printf("(%u - %s)\n\n", htonl(network), get_bytes_str_num(htonl(network), sizeof(network)));
    printf("range: %u - %u\n (%s - %s)\n", range_start, range_end, 
        get_bytes_str_num(range_start, sizeof(range_start)),
        get_bytes_str_num(range_end, sizeof(range_end)));
    printf("range: %s - %s\n", range_start_str, range_end_str);
    //printf("   (%u - %u\n (%s - %s))\n\n", htonl(range_start), htonl(range_end),
    //    get_bytes_str_num(htonl(range_start), sizeof(range_start)), 
    //    get_bytes_str_num(htonl(range_end), sizeof(range_end)));
    printf("%u addresses\n", range_end+1 - range_start);

    // 1 - adress in use, 0 - address is free
    Bit_array ip_address_pool;
    if(init_bit_array(&ip_address_pool, range_end+1 - range_start, ADDRESS_FREE) < 0) {
        fprintf(stderr, "Failed to init ip address pool\n");
        exit(1);
    }
    
    uint32_t addresses[100] = {0};
    int last = 0;
    char ch;
    do {
        printf("[r]equest address [l]ist addresses [d]elete address [q]uit\nchoise: \n");
        ch = fgetc(stdin);
        char temp;
        while((temp = fgetc(stdin)) != '\n');
        switch(ch) {
            case 'r':
                uint32_t address = request_address(&ip_address_pool, network);
                if(address != 0) {
                    addresses[last++] = address;
                    printf("added address %u\n", address);
                    print_addresses(addresses, last);
                }
                break;
            case 'd':
                int choise;
                do {
                    print_addresses(addresses, last);
                    printf("ender id to delete: ");
                    scanf(" %d", &choise);
                    while((temp = fgetc(stdin)) != '\n');
                } while(choise < 0 || choise > last);

                if(free_address(&ip_address_pool, addresses[choise], network) < 0) {
                    fprintf(stderr, "Failed to free address %d\n", addresses[choise]);
                }
                addresses[choise] = 0;
                print_addresses(addresses, last);
                break;
            case 'l':
                print_addresses(addresses, last);
                break;

            case 'q':
                break;
            default:
                fprintf(stderr, "Invalid option\n");
                break;
        }
    } while(ch != 'q');
    return 0;
}
