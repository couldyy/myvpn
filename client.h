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

typedef enum {
    CON_NOT_CONNECTED = 0,
    CON_REQUEST,
    CON_PING,
    CON_ESTAB,
    CON_LOST,
} Con_state;

typedef struct {
    int client_socket;
    int tun_fd;

    struct sockaddr_in* local_addr;
    struct sockaddr_in* local_addr_tun;

    struct sockaddr_in* server_addr;
    struct sockaddr_in* server_addr_tun;


    Con_state connection_state;
    uint32_t seq_num;
    uint32_t ack_num;
    uint32_t authentication_num;
} Connection;


char* get_user_message(char* buff, size_t size);

// Expects fileds 'client_socket' and server 'server_addr' to be filled before passing into function
// Performs connection to server, fills 'connection' struct with corresponding data
// on success returns 0, on failure:
// -1 - error happend while connecting to server, -2 - error happend locally (e.g. invalid input params);
int connnect_to_server(Connection* connection);

#endif //CLIENT_H
