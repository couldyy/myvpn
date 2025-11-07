#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>


int main(int argc, char** argv)
{
    if(argc != 3) {
        fprintf(stderr, "Invalid args, specify [IP] [PORT]\n");
        exit(1);
    }
    char* server_ip = argv[1]; 
    short server_port = atoi(argv[2]);
    printf("Got %s:%hd \n", server_ip, server_port);
    
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket < 0) {
        fprintf(stderr, "socket(): %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(server_port);
    int res = inet_pton(AF_INET, server_ip, &local.sin_addr);
    if(res == 0) {
        fprintf(stderr, "Invalid ip '%s'\n", server_ip);
        exit(1);
    }
    else if (res < 0) {
        fprintf(stderr, "Invalid address family\n");
        fprintf(stderr, "%s\n", strerror(errno));
    }

    if(bind(server_socket, (struct sockaddr*)&local, sizeof(local)) < 0) {
        fprintf(stderr, "bind(): %s\n", strerror(errno));
        exit(0);
    }
    else {
        printf("Bind successfull\n");
    }

    printf("Server started on '%s:%hd'...\n", server_ip, server_port);

    struct sockaddr_in client_addr;
    int len = sizeof(client_addr);
    char msg_buf[BUFSIZ];
    printf("press any key...");
    while(1) {
    fgetc(stdin);
        if(recvfrom(server_socket, msg_buf, sizeof(msg_buf), 0,
                (struct sockaddr*)&client_addr, &len) < 0) {
            fprintf(stderr, "recvfrom(): %s\n", strerror(errno));
        }
        else {
            char peer_addr[16];
            inet_ntop(AF_INET, &client_addr.sin_addr, peer_addr, sizeof(peer_addr));
            printf("'%s:%hu' > %s\n", peer_addr, ntohs(client_addr.sin_port), msg_buf);
            //if(sendto(server_socket, msg_buf, sizeof(msg_buf), 0,
            //        (struct sockaddr*)&client_addr, len) < 0) {
            //    fprintf(stderr, "sendto(): %s\n", strerror(errno));
            //}
            //else {
            //    printf("sent msg\n");
            //}
        }
    }
    
    return 0;
}
