#include "server.h"
#include "proto.h"
#include "tun.h"
#include "utils.h"

int main(int argc, char** argv)
{
    if(argc != 5) {
        fprintf(stderr, "Invalid args, specify [IP] [PORT] [TUN IP] [TUN MASK]\n");
        exit(1);
    }
    char* server_addr = argv[1]; 
    uint16_t server_port = atoi(argv[2]);
    char* tun_addr = argv[3];
    char* tun_mask = argv[4];
    printf("Got %s:%hu %s %s \n", server_addr, server_port, tun_addr, tun_mask);
    
    // init socket
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket < 0) {
        fprintf(stderr, "socket(): %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_in* local = create_sockaddr_in(AF_INET, server_port, server_addr);
    if(local == NULL) {
        exit(1);
    }
    if(bind(server_socket, (struct sockaddr*)local, sizeof(*local)) < 0) {
        fprintf(stderr, "bind(): %s\n", strerror(errno));
        exit(0);
    }
    printf("Bind successfull\n");


    printf("Server started on '%s:%hd'...\n", server_addr, server_port);

    // just for testing purposes
    char* temp_client_addr = "192.168.1.2";
    uint16_t temp_client_port = 8080;
    char* temp_client_tun_addr = "10.0.0.2";
    struct sockaddr_in* temp_client = create_sockaddr_in(AF_INET, temp_client_port, temp_client_addr);
    if(temp_client == NULL) {
        exit(1);
    }

    // Init tun interface
    char dev[IFNAMSIZ];
    memset(dev, 0, IFNAMSIZ);
    // TODO connect to server before stting up IP address on tun interface
    int tun_fd = init_tun_if(dev, tun_addr, tun_mask);
    if(tun_fd < 0) {
        fprintf(stderr, "Error setting up tun interface\n");
        exit(1);
    }
    else {
        printf("Configured tun interface '%s' on '%s' '%s' successfully\n", dev, tun_addr, tun_mask);
    }

    // init pollfd for poll() calls
    size_t fds_count = 2;
    struct pollfd fds[fds_count];
    memset(fds, 0, sizeof(fds));

    fds[0] = (struct pollfd) { .fd = server_socket, .events = POLLIN, .revents = 0};
    fds[1] = (struct pollfd) { .fd = tun_fd, .events = POLLIN, .revents = 0 };
    int poll_res;

    while(1) {
        poll_res = poll(fds, fds_count, -1);
        for(int i = 0; i < fds_count; i++) {
            struct pollfd* current = &fds[i];
            if(current->revents & POLLIN == 1) {
                if (current->fd == server_socket) {
                    struct sockaddr_in src_addr;
                    socklen_t src_addr_len = sizeof(struct sockaddr_in);
                    uint8_t buff[MTU] = {0};
                    ssize_t read_bytes = recvfrom(server_socket, buff, sizeof(buff), 0,
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
                    uint8_t buff[MTU];
                    ssize_t read_bytes = 0;
                    read_bytes = read(current->fd, buff, sizeof(buff));
                    if(read_bytes == 0) {
                        fprintf(stderr, "EOF reached on '%d'\n", current->fd);
                    }
                    else if(read_bytes < 0) {
                        fprintf(stderr, "read(%d): %s\n", current->fd, strerror(errno));
                    }
                    else {
                        // TODO check for dst and src addr, and send accordiong to clients table
                        _print_packet(buff, read_bytes);
                        Ip_data ip_data = parse_ip_header(buff, read_bytes);
                        printf("version: %d\n", ip_data.ip_version);

                        if(ip_data.ip_version == 6 || ip_data.ip_version != 4) {
                            continue;
                        }

                        // TODO encapsulate
                        ssize_t written_bytes = sendto(server_socket, buff, read_bytes, 0,
                            (struct sockaddr*) temp_client, sizeof(*temp_client));
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
                            printf("Successfully sent packet to '%u'\n", ntohl(temp_client->sin_addr.s_addr));
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
