//TODO:
// 1. Make so that routing to tun ins created and set from this program
// 2. create functino that will print ip packets
// 3. I probably need to get curent user network ip, so that tun is set to a different one


#include "client.h"
#include "tun.h"
#include "common.h"
#include "utils.h"




int main(int argc, char** argv)
{

    if(argc != 3) {
        fprintf(stderr, "Invalid args, specify [NET_IP] [MASK]\n");
        exit(1);
    }
    char dev[IFNAMSIZ];
    memset(dev, 0, IFNAMSIZ);
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
        //char buff[1024] = {0};
        int buff[1024] = {0};
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
                printf("[%s]: \n", dev);
                _print_packet(buff, read_bytes);
            }
        }
    }
    return 0;

//////////////////////////////////////////////////

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
