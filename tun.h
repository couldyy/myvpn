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



// Allocates tun device by calling open on 'TUN_PATH' and creates tun interface with name 'dev' via ioctl()
// 'dev' must be allocated before calling this function, and  
//  if 'dev' is empty string, tun_alloc will try to allocate any available tun interface
// otherwise allocates tun interface specified by 'dev'
int tun_alloc(char* dev, size_t dev_size);

// sets 'net_addr' and 'net_mask' for tun interface 'dev_name' using socket + ioctl
// 'net_addr' and 'net_mask' MUST be in NETWORK byte order
// On success returns 0, on error -1
int set_tun_addr(char* dev_name, size_t dev_name_size, uint32_t addr, uint32_t net_mask);

// Sets state of tun interface 'dev_name' to 'state'
// On success returns 0, on error -1
int set_tun_state(char* dev_name, size_t dev_name_size, tun_state state);

// Initializes tun interface with 'dev' name (if it is provided, if not - let kernel give name)
//  sets up 'addr' and 'mask' for interface and sets state UP
// On success returns non negative tun fd, on error -1
int init_tun_if(char* dev, size_t dev_size, char* addr, char* mask);

#endif // TUN_H
