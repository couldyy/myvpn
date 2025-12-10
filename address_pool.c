#include "address_pool.h"

//#define USE_U32
#define BITARRAY_IMPLEMENTATION
#include "thirdparty/bitarray.h"

// WARNING all functions accept and return addresses in HOST BYTE ORDER
// it is up to user to later convert to network byte order

// checks wether address is free or in use
// return either ADDRESS_FREE or ADDRESS_IN_USE on success and -1 on error
int check_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr)
{
    assert(address_pool_mask != NULL);
    assert(address > network_addr);
    return get_bit(address_pool_mask, address-network_addr-1); 
}

// marks address in 'address_pool_mask' as ADDRESS_FREE
// on success 0 is returned, on error -1
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

// marks requested address as ADDRESS_IN_USE and returns numerical IP address in HOST byte order
// on success numerical IP address is returned, on error - 0
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

// converts 'addr' and 'network_mask' into numerical values in HOST byte order, applies mask on addr and returns
// corresponding struct
// on success address of 'Network' structure is return, on failure - NULL
Network* get_network(char* network_addr, char* network_mask)
{
    assert(network_addr != NULL && network_mask != NULL);
    uint32_t net_addr_num;
    uint32_t net_mask_num;

    int res = inet_pton(AF_INET, network_addr, &net_addr_num);
    if(res = 0) {
        fprintf(stderr, "Invalid address '%s'\n", network_addr);
        return NULL;
    }
    else if(res < 0) {
        fprintf(stderr, "Error converting address '%s': %s\n", network_addr, strerror(errno));
        return NULL;
    }
    
    res = inet_pton(AF_INET, network_mask, &net_mask_num);
    if(res = 0) {
        fprintf(stderr, "Invalid mask '%s'\n", network_mask);
        return NULL;
    }
    else if(res < 0) {
        fprintf(stderr, "Error converting mask '%s': %s\n", network_mask, strerror(errno));
        return NULL;
    }
    net_addr_num = ntohl(net_addr_num);
    net_mask_num = ntohl(net_mask_num);

    uint32_t network_addr_masked = net_addr_num & net_mask_num;
    uint32_t range_start = network_addr_masked + 1; 
    uint32_t range_end = network_addr_masked | (~net_mask_num) - 1; 
    Network* network = malloc(sizeof(Network)); 
    network->network_addr = network_addr_masked;
    network->range_start = range_start;
    network->range_end = range_end;
    return network;
}

