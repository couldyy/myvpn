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

char* get_user_message(char* buff, size_t size);

int connnect_to_server(char* server_ip, uint16_t server_port, char* desired_ip);

#endif //CLIENT_H
