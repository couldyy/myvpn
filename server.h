#ifndef SERVER_H
#define SERVER_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "myvpn_errno.h"
#include "proto.h"
#include "address_pool.h"
#include "myvpn_log.h"
#include "tun.h"
#include "utils.h"

//#define MYVPN_PROTO_DEBUG_PRINT_PACKET        // print all packets payload in bytes

typedef struct {
    // HOST byte order
    int socket;
    int tun_fd;
    //

    // NETWORK byte order
    struct sockaddr_in* real_addr;
    uint32_t tun_addr;
    uint32_t tun_mask;
    //

    char tun_dev[IFNAMSIZ];  // WARNING: changing it to just a pointer will cause bugs in server and client, since some
                             //          functions depend on sizeof(Client_ctx->tun_dev). Thus, changing it to a pointer
                             //          will make it return invalid size and lead to invalid behaivor or vulnerabilities.
} Server_ctx;

int accept_connection(Server_ctx* server_ctx, Vpn_network* vpn_network, Vpn_packet* packet, struct sockaddr_in* src_addr, socklen_t* src_addr_len);

int estab_connection(Server_ctx* server_ctx, Vpn_network* vpn_network, Vpn_packet* packet, struct sockaddr_in* src_addr, socklen_t* src_addr_len);


// TODO move to proto.h (to do that, see TODO#3)
// server side
// all args MUST be in NETWORK byte order (mb except for 'seq_num' and 'ack_num')
Raw_packet* construct_con_repl_packet(uint32_t authentication_num, uint32_t seq_num, uint32_t ack_num, 
        uint32_t server_tun_ip, uint32_t tun_net_mask, uint32_t assigned_client_tun_ip);
Raw_packet* construct_connestab_repl_packet(uint32_t auth_num, uint32_t seq_num, uint32_t ack_num) ;

int handle_received_packet(Server_ctx* server_ctx, Vpn_network* vpn_network, Vpn_packet* packet, struct sockaddr_in* src_addr, socklen_t* src_addr_len);
// if encapsulated IP packet DST addr == server tun addr - writes packet to tun interface
// otherwise looks for client addr in 'tun_to_ip_route_table' and in clients table, if fount - routes packet to client
//
// On success returns 0, on error -1 and 'myvpn_errno' is set to indicat and error (BUT NOT IN ALL CASES)
int handle_data_packet(Server_ctx* server_ctx, Vpn_network* vpn_network, Vpn_packet* packet, struct sockaddr_in* src_addr, socklen_t* src_addr_len);

// Reads IP packet from 'packet_buff[packet_buff_size]', finds client in tun->IP routing table based on 
//      dst addr in IP packet. Encapsulates IP packet into VPN packet and sends to client
//
// On success returns 0, on error -1 

int handle_tun_packet(Server_ctx* server_ctx, Vpn_network* vpn_network, uint8_t* packet_buff, size_t packet_buff_size);
// creates 'Server_ctx' structure and fills all fields with corresponding data based on supplied function arguments
// on success returns address to created structure, on error NULL and 'myvpn_errno' is set to indicate an error
Server_ctx* init_server_ctx(char* server_addr, uint16_t server_port, char* tun_addr, char* tun_mask);
#endif //SERVER_H
