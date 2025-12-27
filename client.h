#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>

#include "myvpn_errno.h"
#include "proto.h"
#include "tun.h"
#include "utils.h"
#include "myvpn_log.h"

#define INTERNET_ROUTE 0    // for debug, routing to internet via server will be implemented in future


typedef struct {
    // HOST byte order
    int socket;
    int tun_fd;
    //

    // NETWORK byte order
    struct sockaddr_in* client_real_addr;
    struct sockaddr_in* server_real_addr;   // TODO this should be in 'Connection' struct

    uint32_t server_tun_addr;       // TODO this should be in 'Connection' struct
    uint32_t tun_addr;
    uint32_t tun_mask;
    //

    char tun_dev[IFNAMSIZ];
} Client_ctx;

//typedef struct {
//    int client_socket;
//    int tun_fd;
//
//    // HOST byte order
//    uint32_t tun_network_mask;
//    //
//
//    // NETWORK byte order
//    struct sockaddr_in* local_addr;     // TODO do i need this field?
//    struct sockaddr_in* local_addr_tun;
//
//    struct sockaddr_in* server_addr;
//    struct sockaddr_in* server_addr_tun;
//    //
//
//
//    // HOST byte order 
//    Con_state connection_state;
//    uint32_t seq_num;
//    uint32_t ack_num;
//    //
//
//    // NETWORK byte order
//    uint32_t authentication_num;
//    //uint32_t server_authentication_num;   
//    //
//} Connection;



// TODO: also init tun, but DON'T assign ip to it
// initializes Client_ctx structure by:
//      - creates client socket
//      - creates server sockaddr and 'connects' it to client socket
//      - creates client sockaddr
//      - allocates Client_ctx structure and writes 'client_socket' and 'real_addr'
// On succress returns address to Client_ctx structure, on error NULL and 'myvpn_errno' is set to indicate an error
Client_ctx* init_client_ctx(char* client_addr, uint16_t client_port, char* server_addr, uint16_t server_port);

// Creates 'Connection' struct and initializes it with 0
// on success address to 'Connection' structure is returned, on error NULL
Connection* init_connection(void);

/* Expects fields 'client_socket', 'tun_fd', 'server_addr' in to be initialized in Connection structure
    before passing into function
   Performs connection to server, fills 'connection' struct with corresponding data
   on success returns 0, on error -1 and 'myvpn_errno' is set to indicate an error */
int connect_to_server(Client_ctx* client_ctx, Connection* connection);

// TODO move to proto.h (to do that, see TODO#3)
// client side
Raw_packet* construct_connection_packet(void);
Raw_packet* construct_connestab_packet(Connection* connection);

// reads and handles tun packet based on dst addr of raw packet
// On success returns 0, on error -1
int handle_tun_packet(Client_ctx* client_ctx, Connection* connection);

// Parses connection data for payload (server and client tun IPs, net mask, auth num) and 
// writes parsed data into 'connetion'
// on success returns 0, on error -1 and 'myvpn_errno' is set to indicate and error
int parse_connection_payload(Client_ctx* client_ctx, Connection* connection, uint8_t* payload, size_t payload_size);

// validates CON_REPL reply from server, by msg_type field
// on success returns 0, on error -1 and 'myvpn_errno' is set to indicate and error
int validate_con_repl_pkt(Vpn_packet* packet);

// validates CON_ESTAB reply from server, by msg_type field
// on success returns 0, on error -1 and 'myvpn_errno' is set to indicate and error
int validate_connestab_repl(Connection* connection, Vpn_packet* packet);
#endif //CLIENT_H
