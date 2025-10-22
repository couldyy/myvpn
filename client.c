//TODO:
// 1. Make so that routing to tun ins created and set from this program
// 2. create functino that will print ip packets
// 3. I probably need to get curent user network ip, so that tun is set to a different one

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

char* get_user_message(char* buff, size_t size);

int tun_alloc(char* dev, struct ifreq* ifr);
int set_tun_ip(int control_socket, struct ifreq* ifr , char* net_ip, char* net_mask);
int set_tun_state(int control_socket, struct ifreq* ifr, tun_state state);
int init_tun_if(char* dev, char* ip, char* mask);


int main(int argc, char** argv)
{

    if(argc != 3) {
        fprintf(stderr, "Invalid args, specify [NET_IP] [MASK]\n");
        exit(1);
    }
    char dev[IFNAMSIZ];
    //char *addr = "192.168.1.0";
    char *addr = argv[1];
    char *mask = argv[2];
    int tun_fd = init_tun_if(dev, addr, mask);
    if(tun_fd < 0) {
        fprintf(stderr, "Error setting up tun interface\n");
        exit(1);
    }
    else {
        printf("Configured tun interface '%s' successfully\n", dev);
        char buff[1024] = {0};
        ssize_t read_bytes = 0;
        while(1) {
            read_bytes = read(tun_fd, buff, sizeof(buff));
            if(read_bytes == 0) {
                fprintf(stderr, "EOF reached on '%s'\n", dev);
            }
            else if(read_bytes < 0) {
                fprintf(stderr, "read(%s): %s\n", dev, strerror(errno));
            }
            else {
                printf("[%s]: %s\n", dev, buff);
            }
        }
    }
    return 0;



    if(argc != 3) {
        fprintf(stderr, "Invalid args, specify [IP] [PORT]\n");
        exit(1);
    }
    char* server_ip = argv[1]; 
    short server_port = atoi(argv[2]);
    //printf("Got %s:%hd \n", client_ip, client_port);
    
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_socket < 0) {
        fprintf(stderr, "socket(): %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_in remote;
    remote.sin_family = AF_INET;
    remote.sin_port = htons(server_port);
    int res = inet_pton(AF_INET, server_ip, &remote.sin_addr);
    if(res == 0) {
        fprintf(stderr, "Invalid ip '%s'\n", server_ip);
        exit(1);
    }
    else if (res < 0) {
        fprintf(stderr, "Invalid address family\n");
        fprintf(stderr, "%s\n", strerror(errno));
    }

    if(connect(client_socket, (struct sockaddr*)&remote, sizeof(remote)) < 0) {
        fprintf(stderr, "connect(): %s\n", strerror(errno));
        exit(1);
    }
    else {
        printf("Successfully connected to '%s:%hd'...\n", server_ip, server_port);
    }

    int len = sizeof(remote);
    char msg_buf[BUFSIZ];
    while(1) {
        get_user_message(msg_buf, sizeof(msg_buf));
        memset(msg_buf, 0, sizeof(msg_buf));
        for(int i = 0; i < 100; i++) {
            snprintf(msg_buf, sizeof(msg_buf), "%d", i);
            if(sendto(client_socket, msg_buf, sizeof(msg_buf), 0,
                    (struct sockaddr*)&remote, len) < 0) {
                fprintf(stderr, "sendto(): %s\n", strerror(errno));
            }
            else {
                printf("sent: %s\n", msg_buf);
            }
            usleep(100);
        }

        if(recvfrom(client_socket, msg_buf, sizeof(msg_buf), 0,
                (struct sockaddr*)&remote, &len) < 0) {
            fprintf(stderr, "recvfrom(): %s\n", strerror(errno));
        }
        else {
            char server_addr[16];
            inet_ntop(AF_INET, &remote.sin_addr, server_addr, sizeof(server_addr));
            printf("'%s:%hu' > %s\n", server_addr, ntohs(remote.sin_port), msg_buf);
        }
    }
    
    return 0;
}

char* get_user_message(char* buff, size_t size)
{
    printf("message > ");
    char* read = fgets(buff, size, stdin);
    if(read == NULL) {
        fprintf(stderr, "fgets(): Error getting user input\n");
        return NULL;
    }
    size_t read_len = strlen(buff);
    if(buff[read_len-1] == '\n') {
        buff[read_len-1] = '\0';
    }

    return read;
}
int tun_alloc(char* dev, struct ifreq* ifr)
{
    assert(dev != NULL);
    assert(ifr != NULL);
    // NOTE: as it stnds in ioctl man page, open() can cause some issues
    // that can be fixed by passing O_NONBLOCK. If you encounter some, try whis flag
    const int tun_fd = open(TUN_PATH, O_RDWR);
    if(tun_fd < 0) {
        fprintf(stderr, "open(): %s\n", strerror(errno));
        return -1;
    }
    printf("tun open success\n");

    // create interface
    // TODO ifreq == interface request??
    memset(ifr, 0, sizeof(*ifr));

    ifr->ifr_flags = IFF_TUN;
    // TODO  NULL ptr dereference?
    // if dev name is passed try to get requested, 
    // if not - let kernel to give free interface name
    if(*dev)
        strlcpy(ifr->ifr_name, dev, IFNAMSIZ);

    if(ioctl(tun_fd, TUNSETIFF, ifr) < 0) {
        fprintf(stderr, "ioctl(TUNSETIFF): %s\n", strerror(errno));
        return -1;
    }
    
    // write tun interface name given by kernel
    if(!(*dev))
        strlcpy(dev, ifr->ifr_name, IFNAMSIZ);

    return tun_fd;
}

// sets ip using socket + ioctl
// ip should be passed with network mask
// 192.168.1.0/24
// TODO make so mask is passed with ip in format '/xx'
int set_tun_ip(int control_socket, struct ifreq* ifr , char* net_ip, char* net_mask)
{
    assert(control_socket > 0);
    assert(ifr != NULL);
    assert(net_ip != NULL);
    assert(net_mask != NULL);

    struct sockaddr_in *sin;
    sin = (struct sockaddr_in*)&ifr->ifr_addr;
    sin->sin_family = AF_INET;


    int err = inet_pton(AF_INET, net_ip, &(sin->sin_addr));
    if(err == 0) {
        fprintf(stderr, "Invalid address '%s' for tun\n", net_ip);
        return -1;
    }
    else if(err < 0) {
        fprintf(stderr, "inet_pton(): %s\n", strerror(errno));
        return -1;
    }

    // set ip
    if(ioctl(control_socket, SIOCSIFADDR, ifr) < 0) {
        fprintf(stderr, "ioctl(SIOCSIFADDR): %s\n", strerror(errno));
        return -1;
    }

    sin = (struct sockaddr_in*)&ifr->ifr_netmask;
    err = inet_pton(AF_INET, net_mask, &(sin->sin_addr));
    if(err == 0) {
        fprintf(stderr, "Invalid mask '%s' for tun\n", net_mask);
        return -1;
    }
    else if(err < 0) {
        fprintf(stderr, "inet_pton(): %s\n", strerror(errno));
        return -1;
    }

    
    // set mask
    if(ioctl(control_socket, SIOCSIFNETMASK, ifr) < 0) {
        fprintf(stderr, "ioctl(SIOCSIFNETMASK): %s\n", strerror(errno));
        return -1;
    }

    return 1;
}

int set_tun_state(int control_socket, struct ifreq* ifr, tun_state state)
{

    if(ioctl(control_socket, SIOCGIFFLAGS, ifr) < 0) {
        fprintf(stderr, "ioctl(SIOCGIFFLAGS): %s\n", strerror(errno));
        return -1;
    }
    switch(state) {
        case UP:
            ifr->ifr_flags |= IFF_UP;
            break;
        case DOWN:
            ifr->ifr_flags &= ~IFF_UP;
            break;
        default:
            fprintf(stderr, "set_tun_state(): invalid state\n");
            return -1;
    }

    if(ioctl(control_socket, SIOCSIFFLAGS, ifr) < 0) {
        fprintf(stderr, "ioctl(SIOCSIFFLAGS): %s\n", strerror(errno));
        return -1;
    }

    return 1;
}


// TODO make so mask is passed with ip in format '/xx'
int init_tun_if(char* dev, char* ip, char* mask)
{
    int control_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(control_socket < 0) {
        fprintf(stderr, "socket(): %s\n", strerror(errno));
        return -1;
    }

    struct ifreq ifr;
    int tun_fd = tun_alloc(dev, &ifr);
    if(tun_fd < 0) {
        fprintf(stderr, "Failed allocating tun interface\n");
err_exit:
        close(control_socket);
        return -1;
    }
    if(set_tun_ip(control_socket, &ifr, ip, mask) < 0) {
        fprintf(stderr, "Failed setting ip and mask for tun interface\n");
        goto err_exit;
    }

    if(set_tun_state(control_socket, &ifr, UP) < 0) {
        fprintf(stderr, "Failed setting tun interface UP\n");
        goto err_exit;
    }

    return tun_fd;
}
