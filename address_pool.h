#ifndef ADDRESS_POOL_H
#define ADDRESS_POOL_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "proto.h"
#include "utils.h"
#include "myvpn_errno.h"

#define MAX_PREALLOC_ADDRESSES 2048

#include "thirdparty/bitarray.h"

#include "thirdparty/cash_table.h"

#define ADDRESS_FREE ((elem_t) 0)
#define ADDRESS_IN_USE ((elem_t) 1)

// IP addresses are just indexes in bitarray + network mask. Network addrress is not used and thus, not present in 
//      bitarray. So whole network range must be biased. For example, in net 192.168.1.0/24, addr 192.168.1.1 would be
//      at index 0 beacuse of ADDR_POOL_BIAS (192.168.1.1 - 192.168.1.0 = 1,    1 - ADDR_POOL_BIAS = 0)
#define ADDR_POOL_BIAS 1


typedef struct {
    // HOST byte order
    uint32_t network_addr;
    uint32_t range_start;
    uint32_t range_end;
    size_t addr_count;
    //
} Network;

// TODO there are a lot of copies happening, optimise it somehow
// i could just save pointer, but in all structures they point to single local variable
// and even if i do store ptr at accept_connection(), i wont be able to find it in estab_connection(), because in this func,
// it is a local variable
typedef struct {
    // HOST byte order
    Network* network;
    Bit_array* network_address_pool;
    //

    // NETWORK byte order (except 'client_connection')

    // key: struct sockaddr_in real_addr (actual copy of struct)
    // value: Connection* client_connection (just pointer)
    HashTable* clients_table;

    // key: struct sockaddr_in real_addr (actual copy of struct)
    // value: Connection* client_connection (just pointer)
    HashTable* unverified_client_connections;

    // key: uint32_t(as ptr)  tun_addr  
    // value: struct sockaddr_in* real_addr (actual copy of struct)
    HashTable* tun_to_ip_route_table;


    //
} Vpn_network;

/* 
 'server_addr' MUST be in NETWORK byte order
 initializes 'Vpn_network' structure with data based on supplied 'addr' and 'network_mask':
    - inits 'Network' structure (num network, range, count)
    - inits 'Bit_array' of whole address range of network
    - inits 'HashTable' of client address + port - 'Client connection' struct
    - inits 'HashTable' of tun client address (+ port?) - real client address + port
    - marks server IP as ADDRESS_IN_USE
 on success address to 'Vpn_network' is returned, on failure - NULL
*/
Vpn_network* init_vpn_network(char* addr, char* network_mask, uint32_t server_tun_addr);

// WARNING all functions accept and return addresses in HOST BYTE ORDER
// it is up to user to later convert to network byte order

// checks wether address is free or in use
// return either ADDRESS_FREE or ADDRESS_IN_USE on success and -1 on error
int check_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr);

// marks address in 'address_pool_mask' as ADDRESS_FREE
// 'address' must be in HOST byte order
// on success 0 is returned, on error -1
int free_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr);

// marks requested address as ADDRESS_IN_USE and returns numerical IP address in HOST byte order
// on success numerical IP address is returned, on error - 0
uint32_t request_address(Bit_array* address_pool_mask, uint32_t network_addr);

// sets status of IP address (index in bit array) to either ADDRESS_FREE / ADDRESS_IN_USE
// on success returns 0, on error -1
int set_address_state(Bit_array* address_pool_mask, uint32_t target_addr, uint32_t network_addr, int state);

// converts 'addr' and 'network_mask' into numerical values in HOST byte order, applies mask on addr and returns
// corresponding struct
// on success address of 'Network' structure is return, on failure - NULL
Network* get_network(char* addr, char* network_mask);
#endif //ADDRESS_POOL_H
