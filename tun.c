#include "tun.h"
//TODO get rid of ioctl and replace it with socket(NETLINK)

// 'dev' should be allocate before calling this function, and  
//  if 'dev' is empty string, tun_alloc will try to allocate any available tun interface
// otherwise allocates tun interface specified by 'dev'
// 'ifr' should be allocated before calling this function and used in other functions to get certain metadata
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
// 'mask' should be passed in format xxx.xxx.xxx.xxx i.e. "255.255.255.0"
// 'control_socket' should be allocated before calling function and is used to tell ioctl()
// that we work with socket fd. Actual data about tun interface is stored in 'ifr' that we
// pass with ioctl() call to socket
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
