#include "address_pool.h"

//#define USE_U32
#define BITARRAY_IMPLEMENTATION
#include "thirdparty/bitarray.h"

#define CASH_TABLE_IMPLEMENTATION
#include "thirdparty/cash_table.h"


// TODO check whether reueted ip is in network range boundaries in all functions

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
Vpn_network* init_vpn_network(char* addr, char* network_mask, uint32_t server_tun_addr)
{
    // TODO variables might not be NULL automatically
    Network* network;
    Bit_array* address_pool;
    HashTable* unverified_client_connections;
    HashTable* clients_table;
    HashTable* tun_to_ip_route_table;

    Vpn_network* vpn_network;

    network = get_network(addr, network_mask);
    if(network == NULL) {
        return NULL;
    }
    size_t init_size = network->addr_count > MAX_PREALLOC_ADDRESSES ? MAX_PREALLOC_ADDRESSES : network->addr_count;

    address_pool = init_bit_array_heap(network->addr_count, ADDRESS_FREE);
    if(address_pool == NULL) {
        myvpn_errno = MYVPN_E_INVALID_POOLSIZE;
        goto err_exit;
    }


    if(set_address_state(address_pool, ntohl(server_tun_addr), network->network_addr, ADDRESS_IN_USE) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to mark server address ADDRESS_IN_USE\n");
        goto err_exit;
    }

    // key: struct sockaddr_in* real_addr 
    // value: Connection* client_connection
    clients_table = init_ht_heap(init_size, 0, hash_sockaddr, cmp_sockaddr);
    if(clients_table == NULL)
        goto err_exit;

    // key: struct sockaddr_in* real_addr 
    // value: Connection* client_connection
    unverified_client_connections = init_ht_heap(init_size, 0, hash_sockaddr, cmp_sockaddr);
    if(unverified_client_connections == NULL)
        goto err_exit;

    // key: uint32_t(as ptr) tun_addr 
    // value: struct sockaddr_in* real_addr 
    tun_to_ip_route_table = init_ht_heap(init_size, 0, hash_uint32_ptr, cmp_uint32_ptr);
    if(tun_to_ip_route_table == NULL)
        goto err_exit;

    vpn_network = malloc(sizeof(Vpn_network));
    if(vpn_network == NULL)
        goto err_exit;

    vpn_network->network = network;
    vpn_network->network_address_pool = address_pool;
    vpn_network->clients_table = clients_table;
    vpn_network->unverified_client_connections = unverified_client_connections;
    vpn_network->tun_to_ip_route_table = tun_to_ip_route_table;
    return vpn_network;

err_exit:
    if(network != NULL) free(network);
    if(address_pool != NULL) {
        if(address_pool->items != NULL) free(address_pool->items);
        free(address_pool);
    }
    if(clients_table != NULL) {
        clear_ht(clients_table);
        free(clients_table);
    }
    if(unverified_client_connections != NULL) {
        clear_ht(clients_table);
        free(clients_table);
    }
    if(tun_to_ip_route_table != NULL) {
        clear_ht(tun_to_ip_route_table);
        free(tun_to_ip_route_table);
    }
    return NULL;
}

// WARNING all functions accept and return addresses in HOST BYTE ORDER
// it is up to user to later convert to network byte order

// checks wether address is free or in use
// return either ADDRESS_FREE or ADDRESS_IN_USE on success and -1 on error
int check_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr)
{
    assert(address_pool_mask != NULL);
    assert(address > network_addr); // TODO check for net range boundaries
    return get_bit(address_pool_mask, address - network_addr - ADDR_POOL_BIAS); 
}

// TODO set up proper myvpn_errno on failures
// marks address in 'address_pool_mask' as ADDRESS_FREE
// on success 0 is returned, on error -1
int free_address(Bit_array* address_pool_mask, uint32_t address, uint32_t network_addr)
{
    assert(address_pool_mask != NULL);
    assert(address > network_addr);     

    int addr_status = check_address(address_pool_mask, address, network_addr);
                                        
    if(addr_status < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to check address %u (net: %u): %s\n", 
            address, network_addr, myvpn_strerror(myvpn_errno));
        return -1;
    }
    else if(addr_status == ADDRESS_FREE) {
        myvpn_errno = MYVPN_E_ADDR_IS_FREE;
        return -1;
    }
    else {
        if(set_bit(address_pool_mask, address - network_addr - ADDR_POOL_BIAS, ADDRESS_FREE) < 0){
            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to set network addr(addr: %u, net: %u) to %hhu\n", 
                address, network_addr, ADDRESS_FREE);
            return -1;
        }
        return 0;       // probably dont need that
    }
    return 0;
}

// TODO: define values for failling functions, e.g. for request_address() - #define REQ_ADDR_FAIL 0 and so on
// NOTE: Since this function needs to return IP address (aka uin32_t), it needs a way to somehow indicate an error,
//          without negative value.
//          The naive way, that i came up with - just return 0. 

// marks requested address as ADDRESS_IN_USE and returns numerical IP address in HOST byte order 
// on success numerical IP address is returned, on error - 0
uint32_t request_address(Bit_array* address_pool_mask, uint32_t network_addr)
{
    assert(address_pool_mask != NULL);
    ssize_t index = find_first_bit(address_pool_mask, ADDRESS_FREE);
    if(index < 0) {
        myvpn_errno = MYVPN_E_ADDR_POOL_OVERFLOW;
        return 0;
    }
    else {
        if(set_bit(address_pool_mask, index, ADDRESS_IN_USE) < 0) {
            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to set network addr(addr: %u, net: %u) to %hhu\n", 
                index, network_addr, ADDRESS_IN_USE);
            return 0;
        }
    }
    return network_addr + index + ADDR_POOL_BIAS;
}

// sets status of IP address (index in bit array) to either ADDRESS_FREE / ADDRESS_IN_USE
// on success returns 0, on error -1
int set_address_state(Bit_array* address_pool_mask, uint32_t target_addr, uint32_t network_addr, int state)
{
    assert(address_pool_mask != NULL);
    assert(target_addr > network_addr);
    elem_t norm_state = state & 1;
    return set_bit(address_pool_mask, target_addr - network_addr - ADDR_POOL_BIAS, norm_state);
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
    size_t addr_count = (range_end - range_start) + 1;
    Network* network = malloc(sizeof(Network)); 
    network->network_addr = network_addr_masked;
    network->range_start = range_start;
    network->range_end = range_end;
    network->addr_count = addr_count;
    return network;
}

