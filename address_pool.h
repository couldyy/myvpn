#ifndef ADDRESS_POOL_H
#define ADDRESS_POOL_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#define USE_U32
#include "thirdparty/bitarray.h"

#define ADDRESS_FREE 0
#define ADDRESS_IN_USE 1

typedef struct {
    uint32_t network_addr;
    uint32_t range_start;
    uint32_t range_end;
} Network;

// WARNING all functions accept and return addresses in HOST BYTE ORDER
// it is up to user to later convert to network byte order

// checks wether address is free or in use
// return either ADDRESS_FREE or ADDRESS_IN_USE on success and -1 on error
int check_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr);

// marks address in 'address_pool_mask' as ADDRESS_FREE
// on success 0 is returned, on error -1
int free_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr);

// marks requested address as ADDRESS_IN_USE and returns numerical IP address in HOST byte order
// on success numerical IP address is returned, on error - 0
uint32_t request_address(Bit_array* address_pool_mask, uint32_t network_addr);

// converts 'addr' and 'network_mask' into numerical values in HOST byte order, applies mask on addr and returns
// corresponding struct
// on success address of 'Network' structure is return, on failure - NULL
Network* get_network(char* addr, char* network_mask);
#endif //ADDRESS_POOL_H
