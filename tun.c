#include "tun.h"
#include "myvpn_log.h"
//TODO get rid of ioctl and replace it with socket(NETLINK)

// Allocates tun device by calling open on 'TUN_PATH' and creates tun interface with name 'dev' via ioctl()
// 'dev' must be allocated before calling this function, and  
//  if 'dev' is empty string, tun_alloc will try to allocate any available tun interface
// otherwise allocates tun interface specified by 'dev'
int tun_alloc(char* dev, size_t dev_size)
{
    assert(dev != NULL);
    assert(dev_size == IFNAMSIZ && "'dev_size' must be IFNAMSIZ");

    // NOTE: as it stnds in ioctl man page, open() can cause some issues
    // that can be fixed by passing O_NONBLOCK. If you encounter some, try whis flag
    const int tun_fd = open(TUN_PATH, O_RDWR);
    if(tun_fd < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to allocate tun fd(open()): %s\n", strerror(errno));
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE, "Allocated tun fd successfully\n");

    struct ifreq ifr = {0};

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;       // NO_PI - no packet info (flags + proto num) (additional bits befora pkt)
    // if dev name is passed try to get requested, 
    // if not - let kernel to give free interface name
    if(*dev)
        strlcpy(ifr.ifr_name, dev, IFNAMSIZ);

    // request interface creation 
    if(ioctl(tun_fd, TUNSETIFF, &ifr) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to create interface '%s' on tun fd (ioctl(TUNSETIFF)): %s\n", 
            (*dev) ? dev : "(name was not requested)", strerror(errno));
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG, "Created tun interface '%s'\n", ifr.ifr_name);
    
    // write tun interface name given by kernel
    if(!(*dev))
        strlcpy(dev, ifr.ifr_name, IFNAMSIZ);

    return tun_fd;
}

// sets 'addr' and 'net_mask' for tun interface 'dev_name' using socket + ioctl
// 'addr' and 'net_mask' MUST be in NETWORK byte order
// On success returns 0, on error -1
int set_tun_addr(char* dev_name, size_t dev_name_size, uint32_t net_addr, uint32_t net_mask)
{
    assert(dev_name != NULL);
    assert(dev_name_size == IFNAMSIZ && "`dev_name_size` must be IFNAMSIZ");
    assert(net_addr > 0);
    assert(net_mask > 0);

    int control_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(control_socket < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to create ctl socket: %s\n", strerror(errno));
        return -1;
    }
    struct ifreq ifr = {0};
    strlcpy(ifr.ifr_name, dev_name, IFNAMSIZ);

    struct sockaddr_in *sin;
    sin = (struct sockaddr_in*)&ifr.ifr_addr;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = net_addr;

    // set ip
    if(ioctl(control_socket, SIOCSIFADDR, &ifr) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to set network address (%u) for interface '%s' (ioctl(SIOCSIFADDR)): %s\n", 
            net_addr, dev_name, strerror(errno));
        return -1;
    }

    sin = (struct sockaddr_in*)&ifr.ifr_netmask;
    sin->sin_addr.s_addr = net_mask;
    
    // set mask
    if(ioctl(control_socket, SIOCSIFNETMASK, &ifr) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to set network mask (%u) for interface '%s' (ioctl(SIOCSIFNETMASK)): %s\n", 
            net_mask, dev_name, strerror(errno));
        return -1;
    }


    if(close(control_socket) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to close ctl socket(%d): %s\n", control_socket, strerror(errno));
        // TODO maybe i should anyway?
        // NOTE: Dont return -1, since main goal of function was acomplished succesfully
    }
    return 0;
}

// Sets state of tun interface 'dev_name' to 'state'
// On success returns 0, on error -1
int set_tun_state(char* dev_name, size_t dev_name_size, tun_state state)
{
    assert(dev_name != NULL);
    assert(dev_name_size == IFNAMSIZ && "`dev_name_size` must be IFNAMSIZ");

    int control_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(control_socket < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to create ctl socket: %s\n", strerror(errno));
        return -1;
    }

    struct ifreq ifr = {0};
    strlcpy(ifr.ifr_name, dev_name, IFNAMSIZ);

    // NOTE: get current flags
    if(ioctl(control_socket, SIOCGIFFLAGS, &ifr) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to get current flags for interface '%s' (ioctl(SIOCGIFFLAGS)): %s\n", 
            dev_name, strerror(errno));
        return -1;
    }
    switch(state) {
        case UP:
            ifr.ifr_flags |= IFF_UP;
            break;
        case DOWN:
            ifr.ifr_flags &= ~IFF_UP;
            break;
        default:
            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "set_tun_state(): invalid state '%d'\n", state);
            return -1;
    }

    // NOTE: set updated flags
    if(ioctl(control_socket, SIOCSIFFLAGS, &ifr) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to set flags for interface '%s' (ioctl(SIOCSIFFLAGS)): %s\n", 
            dev_name, strerror(errno));
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE, "Set tun interface '%s' state to %s\n", dev_name, state == UP ? "UP" : "DOWN");

    if(close(control_socket) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to close ctl socket(%d): %s\n", control_socket, strerror(errno));
        // TODO maybe i should anyway?
        // NOTE: Dont return -1, since main goal of function was acomplished succesfully
    }
    return 0;
}


// TODO make so mask is passed with ip in format '/xx'
// Initializes tun interface with 'dev' name (if it is provided, if not - let kernel give name)
//  sets up 'addr' and 'mask' for interface and sets state UP
// On success returns non negative tun fd, on error -1
int init_tun_if(char* dev, size_t dev_size, char* addr, char* mask)
{
    assert(dev != NULL);
    assert(dev_size > 0);
    int tun_fd = tun_alloc(dev, dev_size);
    if(tun_fd < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed allocating tun interface\n");
        return -1;
    }

    uint32_t addr_num = 0;
    uint32_t mask_num = 0;
    int res = inet_pton(AF_INET, addr, &addr_num);
    if(res == 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Invalid tun addr '%s'\n", addr);
        return -1;
    }
    else if(res < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "inet_pton() '%s'\n", strerror(errno));
        return -1;
    }

    res = inet_pton(AF_INET, mask, &mask_num);

    if(res == 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Invalid tun addr '%s'\n", addr);
        return -1;
    }
    else if(res < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "inet_pton() '%s'\n", strerror(errno));
        return -1;
    }

    if(set_tun_addr(dev, dev_size, addr_num, mask_num) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed setting addr '%s' & '%s' for tun '%s' interface\n", addr, mask);
        return -1;
    }

    if(set_tun_state(dev, dev_size, UP) < 0) {
        fprintf(stderr, "Failed setting tun interface '%s' to UP\n", dev);
        return -1;
    }

    return tun_fd;
}
