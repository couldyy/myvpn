// TODO:
// 1. Make so that routing to tun is created and set from this program
// 2. create functino that will print ip packets
// 3. I probably need to get curent user network ip, so that tun is set to a different one


#include "client.h"
#include "tun.h"
#include "proto.h"
#include "utils.h"
#include <poll.h>


char* server_tun_addr = "10.0.0.1";
uint16_t server_real_port = 9000;


int main(int argc, char** argv)
{
    if(argc != 6) {
        fprintf(stderr, "Invalid args, specify [NET_IP] [MASK] [SOCKET ADDR] [SOCKET PORT] [SERVER ADDR]\n");
        exit(1);
    }
    char dev[IFNAMSIZ];
    memset(dev, 0, IFNAMSIZ);
    //char *addr = "192.168.1.0";
    char *addr = argv[1];
    char *mask = argv[2];
    char* local_ip = argv[3];
    uint16_t local_port = atoi(argv[4]);
    char* server_real_addr = argv[5];
    // TODO connect to server before stting up IP address on tun interface
    int tun_fd = init_tun_if(dev, addr, mask);
    if(tun_fd < 0) {
        fprintf(stderr, "Error setting up tun interface\n");
        exit(1);
    }
    else {
        printf("Configured tun interface '%s' successfully\n", dev);
    }

    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_socket < 0) {
        fprintf(stderr, "socket(): %s\n", strerror(errno));
        exit(1);
    }

    printf("socket: %d\n", client_socket);
    // TODO free() local
    struct sockaddr_in *local = create_sockaddr_in(AF_INET, local_port, local_ip);
    printf("struct: %zu dereference: %zu\n", sizeof(struct sockaddr_in), sizeof(*local));
    if(local == NULL) {
        exit(1);
    }


    if(bind(client_socket, (struct sockaddr*) local, sizeof(*local)) < 0) {
        fprintf(stderr, "bind(): %s\n", strerror(errno));
        exit(1);
    }
    printf("bind '%s:%hu' success\n", local_ip, local_port);

    // TODO free() local
    struct sockaddr_in *server_addr = create_sockaddr_in(AF_INET, server_real_port, server_real_addr);
    if(server_addr == NULL) {
        exit(1);
    }

    size_t fds_count = 2;
    struct pollfd fds[fds_count];
    memset(fds, 0, sizeof(fds));

    fds[0] = (struct pollfd) { .fd = client_socket, .events = POLLIN, .revents = 0};
    fds[1] = (struct pollfd) { .fd = tun_fd, .events = POLLIN, .revents = 0 };
    int poll_res;
    while(1) {
        poll_res = poll(fds, fds_count, -1);
        if(poll_res < 0) {
            fprintf(stderr, "poll(): %s\n", strerror(errno));
            exit(1);
        }
        for(int i = 0; i < fds_count; i++) {
            struct pollfd* current = &fds[i];
            if(current->revents & POLLIN == 1) {
                if (current->fd == client_socket) {
                    struct sockaddr_in src_addr;
                    socklen_t src_addr_len = sizeof(struct sockaddr_in);
                    uint8_t buff[MTU] = {0};
                    ssize_t read_bytes = recvfrom(client_socket, buff, sizeof(buff), 0,
                            (struct sockaddr*) &src_addr, &src_addr_len);

                    char peer_addr[16];
                    inet_ntop(AF_INET, &src_addr.sin_addr, peer_addr, sizeof(peer_addr));

                    printf("got message on socket (%d) ('%s:%hu'):\n", current->fd, 
                            peer_addr, ntohs(src_addr.sin_port));
                    if(read_bytes == 0) {
                        fprintf(stderr, "EOF reached on '%d'\n", current->fd);
                    }
                    else if(read_bytes < 0) {
                        fprintf(stderr, "read(%d): %s\n", current->fd, strerror(errno));
                    }
                    else {
                        _print_packet(buff, read_bytes);
                        ssize_t written_bytes = write(tun_fd, buff, read_bytes);
                        if(written_bytes < 0) {
                            fprintf(stderr, "Faild to write packet: %s\n", strerror(errno));
                        }
                        else if(written_bytes == 0) {
                            fprintf(stderr, "EOF on tun interface\n");
                        }
                        else if(written_bytes != read_bytes) {
                            fprintf(stderr, "Error: wrote %d/%d bytes of packet\n", written_bytes, read_bytes);
                        }
                        else {
                            printf("Successfully wrote packet to '%s'\n", dev);
                        }
                    }
                }
                else if (current->fd == tun_fd) {
                    printf("got message on tun (%d):\n", current->fd);
                    uint8_t buff[1500];
                    ssize_t read_bytes = 0;
                    read_bytes = read(current->fd, buff, sizeof(buff));
                    if(read_bytes == 0) {
                        fprintf(stderr, "EOF reached on '%d'\n", current->fd);
                    }
                    else if(read_bytes < 0) {
                        fprintf(stderr, "read(%d): %s\n", current->fd, strerror(errno));
                    }
                    else {
                        _print_packet(buff, read_bytes);
                        Ip_data ip_data = parse_ip_header(buff, read_bytes);
                        printf("version: %d\n", ip_data.ip_version);
                        if(ip_data.ip_version == 6) {
                            
                            continue;
                        }

                        // TODO encapsulate
                        ssize_t written_bytes = sendto(client_socket, buff, read_bytes, 0,
                            (struct sockaddr*) server_addr, sizeof(*server_addr));
                        if(written_bytes < 0) {
                            fprintf(stderr, "Faild to send message: %s\n", strerror(errno));
                        }
                        else if(written_bytes == 0) {
                            fprintf(stderr, "EOF on receiving side\n");
                        }
                        else if(written_bytes != read_bytes) {
                            fprintf(stderr, "Error: send %d/%d bytes of packet\n", written_bytes, read_bytes);
                        }
                        else {
                            printf("Successfully sent packet to '%u'\n", ntohl(server_addr->sin_addr.s_addr));
                        }
                    }
                }
                else {
                    fprintf(stderr, "Unexpected fd - %d\n", current->fd);
                }
            }
        }
    }

    return 0;
}

