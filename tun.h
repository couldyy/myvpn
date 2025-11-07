#ifndef TUN_H
#define TUN_H

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

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#define TUN_PATH "/dev/net/tun"

typedef enum {
    UP,
    DOWN
} tun_state;



int tun_alloc(char* dev, struct ifreq* ifr);
int set_tun_ip(int control_socket, struct ifreq* ifr , char* net_ip, char* net_mask);
int set_tun_state(int control_socket, struct ifreq* ifr, tun_state state);
int init_tun_if(char* dev, char* ip, char* mask);

#endif // TUN_H
