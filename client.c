// TODO:
// 1. Make so that routing to tun is created and set from this program
// 2. create functino that will print ip packets
// 3. I probably need to get curent user network ip, so that tun is set to a different one
#include "client.h"

Log_context g_log_context;

int main(int argc, char** argv)
{
    if(argc != 5) {
        fprintf(stderr, "Invalid args, specify [SOCKET ADDR] [SOCKET PORT] [SERVER ADDR] [SERVER PORT]\n");
        exit(1);
    }
    char* local_ip = argv[1];
    uint16_t local_port = atoi(argv[2]);
    char* server_real_addr = argv[3];
    uint16_t server_port = (uint16_t)atoi(argv[4]);
    printf("Got %s:%hu %s:%hu \n", local_ip, local_port, server_real_addr, server_port);

    // init loggin context
    g_log_context = init_log_context(NULL, MYVPN_LOG_VERBOSE_VERBOSE);
    //g_log_context = init_log_context(NULL, MYVPN_LOG);

    // init client context
    Client_ctx* client_ctx = init_client_ctx(local_ip, local_port, server_real_addr, server_port);
    if(client_ctx == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to create client context (client addr: '%s:%hu', server addr: '%s:%hu' : %s\n",
            local_ip, local_port, server_real_addr, server_port, myvpn_strerror(myvpn_errno));
        exit(1);
    }
    MYVPN_LOG(MYVPN_LOG, "Succsessfully initialized client context (client addr: '%s:%hu', server addr: '%s:%hu'\n",
        local_ip, local_port, server_real_addr, server_port);

    Connection* client_connection = init_connection();
    if(client_connection == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to initialize a connection data for \n");
        exit(1);
    }
    MYVPN_LOG(MYVPN_LOG, "Succsessfully initialized a connection data\n");

    if(connect_to_server(client_ctx, client_connection) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to connect to server: %s", myvpn_strerror(myvpn_errno));
        exit(1);
    }

    // TODO init tun in 'init_client_ctx()'
    // and get rid of this piece of shit
    char tun_addr_str[32] = {0};
    if (inet_ntop(AF_INET, &(client_ctx->tun_addr), tun_addr_str, sizeof(tun_addr_str)) == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "inet_ntop(): %s\n", strerror(errno));
        exit(1);
    }
    char tun_mask_str[32] = {0};
    if (inet_ntop(AF_INET, &(client_ctx->tun_mask), tun_mask_str, sizeof(tun_mask_str)) == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "inet_ntop(): %s\n", strerror(errno));
        exit(1);
    }
    printf("got tun: %s & %s\n", tun_addr_str, tun_mask_str);
    int tun_fd = init_tun_if(client_ctx->tun_dev, tun_addr_str, tun_mask_str);
    if(tun_fd < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Error setting up tun interface\n");
        exit(1);
    }
    else {
        MYVPN_LOG(MYVPN_LOG, "Configured tun interface '%s' successfully\n", client_ctx->tun_dev);
    }
    client_ctx->tun_fd = tun_fd;

    size_t fds_count = 2;
    struct pollfd fds[fds_count];
    memset(fds, 0, sizeof(fds));

    fds[0] = (struct pollfd) { .fd = client_ctx->socket, .events = POLLIN, .revents = 0};
    fds[1] = (struct pollfd) { .fd = client_ctx->tun_fd, .events = POLLIN, .revents = 0 };
    int poll_res;
    while(1) {
        poll_res = poll(fds, fds_count, -1);
        if(poll_res < 0) {
            MYVPN_LOG(MYVPN_LOG_ERROR, "poll(): %s\n", strerror(errno));
            exit(1);
        }
        for(int i = 0; i < fds_count; i++) {
            struct pollfd* current = &fds[i];
            if(current->revents & POLLIN == 1) {
                if (current->fd == client_ctx->socket) {
                    // TODO handle packet based on its msg type
                    struct sockaddr_in src_addr;
                    socklen_t src_addr_len = sizeof(struct sockaddr_in);
                    uint8_t buff[PACKET_BUFFER_SIZE] = {0};
                    Vpn_packet* received_packet = recv_vpn_packet(client_ctx->socket, buff, sizeof(buff), 0, &src_addr, &src_addr_len, client_ctx->server_real_addr);
                    if(received_packet == NULL) {
                        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to receive packet from server: %s\n",
                            myvpn_strerror(myvpn_errno));
                        continue;
                    }

                    //char peer_addr[16];
                    //inet_ntop(AF_INET, &src_addr.sin_addr, peer_addr, sizeof(peer_addr));

                    //printf("got message on socket (%d) ('%s:%hu'):\n", current->fd, 
                    //        peer_addr, ntohs(src_addr.sin_port));
                        //_print_packet(buff, read_bytes);
                    if(tun_write(client_ctx->tun_fd, received_packet->payload, received_packet->payload_size) < 0) {
                        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to write packet to tun '%s' : %s\n", 
                            client_ctx->tun_dev, myvpn_strerror(myvpn_errno));
                        continue;
                    }
                    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Successfully wrote packet to tun '%d'\n", client_ctx->tun_dev);
                }
                else if (current->fd == tun_fd) {
                    if(handle_tun_packet(client_ctx, client_connection) < 0) {
                        continue;
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

// reads and handles tun packet based on dst addr of raw packet
// On success returns 0, on error -1
int handle_tun_packet(Client_ctx* client_ctx, Connection* connection)
{
    //printf("got message on tun (%d):\n", current->fd);
    uint8_t buff[PACKET_BUFFER_SIZE];
    ssize_t read_bytes = tun_read(client_ctx->tun_fd, buff, sizeof(buff));
    if(read_bytes < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to read packet from tun '%s': %s\n", 
            client_ctx->tun_dev, myvpn_strerror(myvpn_errno));
        return -1;
    }
    _print_packet(buff, read_bytes);
    Ip_data ip_data = parse_ip_header(buff, read_bytes);
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Read tun packet(version: %d, ip: %u > %u)\n", 
        ip_data.ip_version, ntohl(ip_data.src_addr_v4), ntohl(ip_data.dst_addr_v4));

    if(ip_data.ip_version != 4) {
        MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Received packet with unsupported verision: %d\n", ip_data.ip_version); 
        return -1;
    }
    // check if dst ip in tun network
    if((ip_data.dst_addr_v4 & client_ctx->tun_mask) == (client_ctx->tun_addr & client_ctx->tun_mask)) {
        if(ip_data.dst_addr_v4 == client_ctx->tun_addr) {
            MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "tun packet dst_addr(%u) == client tun_addr(%u)\n",
                ip_data.dst_addr_v4, client_ctx->tun_addr);
            if(tun_write(client_ctx->tun_fd, buff, read_bytes) < 0) {
                MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to write packet to tun '%s' : %s\n", 
                    client_ctx->tun_dev, myvpn_strerror(myvpn_errno));
                return -1;
            }
        }
        // just send packet to server over UDP socket
        else {
            Raw_packet* encapsulated_packet = encapsulate_data_packet(connection->authentication_num, buff, read_bytes);
            if(encapsulated_packet == NULL) {
                MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to encapsulate data packet: %s\n", 
                    myvpn_strerror(myvpn_errno));
                return -1;
            }
            if(send_vpn_packet(client_ctx->socket, encapsulated_packet, 0, client_ctx->server_real_addr) < 0) {
                MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to send data packet to server ('%u:%hu'): %s\n", 
                    myvpn_strerror(myvpn_errno), ntohl(client_ctx->server_real_addr->sin_addr.s_addr), 
                    ntohs(client_ctx->server_real_addr->sin_port));
                return -1;
            }
            MYVPN_LOG(MYVPN_LOG_VERBOSE,"Successfully sent packet to server '%u:%hu'\n", 
                ntohl(client_ctx->server_real_addr->sin_addr.s_addr), ntohs(client_ctx->server_real_addr->sin_port));
            return 0;
        }
    }
    // if dst is not in tun net, chech whether internet routing feature is enabled
    else {
        if (INTERNET_ROUTE) {
            assert(0 && "TODO: routing to internet via server will be implemented in future");
        }
        else {
            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "dst ip(%u) is not in tun network(%u) and INTERNET_ROUTE is not set\n",
                ip_data.dst_addr_v4, client_ctx->tun_addr & client_ctx->tun_mask);
            return -1;
        }
    }
}

int connect_to_server(Client_ctx* client_ctx, Connection* connection)
{
    assert(client_ctx != NULL);
    assert(client_ctx->server_real_addr != NULL);
    assert(connection != NULL);
    // needed for recvfrom() and validation of src_addr
    struct sockaddr_in src_addr;
    socklen_t src_addr_len = sizeof(src_addr);

    if(client_ctx->socket < 0 ||
        connection->connection_state != CON_NOT_CONNECTED ||
        client_ctx->server_real_addr == NULL) {
        myvpn_errno = MYVPN_E_UNINITIALIZED_CON_DATA;
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "[Started connection to server]\n");

    Raw_packet* con_packet = construct_connection_packet();
    ssize_t sent_bytes = send_vpn_packet(client_ctx->socket, con_packet, 0, client_ctx->server_real_addr);
    if(sent_bytes < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to send packet: %s\n", myvpn_strerror(myvpn_errno));
        myvpn_errno = MYVPN_E_FAIL_SEND_CON;
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Send connection packet to server]\n");

    uint8_t packet_buf[PACKET_BUFFER_SIZE];

    // clear src addr for future calls
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr_len = sizeof(src_addr);

    Vpn_packet* received_packet = recv_vpn_packet(client_ctx->socket, packet_buf, sizeof(packet_buf), 0, &src_addr, &src_addr_len, client_ctx->server_real_addr);
    if(received_packet == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "recv_vpn_packet(): %s\n", myvpn_strerror(myvpn_errno));
        return -1;
    }

    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Received connection reply packet from server]\n");
    if(validate_con_repl_pkt(received_packet) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Received invalid connection reply: %s\n", myvpn_strerror(myvpn_errno));
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Connection reply packet is VALID\n");

    if(parse_connection_payload(client_ctx, connection, received_packet->payload, received_packet->payload_size) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to parse connection packet payload: %s\n", myvpn_strerror(myvpn_errno));
        // TODO set errno
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Parsed connection data from server: TODO data\n");

    // TODO memory leak, free header
    Raw_packet* connestab_packet = construct_connestab_packet(connection);
    if(connestab_packet == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to construct con_estab packet: %s\n", myvpn_strerror(myvpn_errno));
        return -1;
    }
    if(send_vpn_packet(client_ctx->socket, connestab_packet, 0, client_ctx->server_real_addr) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to send con_estab packet to server: %s\n", myvpn_strerror(myvpn_errno));
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Send connecstab packet to server\n");

    // clear src addr for future calls
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr_len = sizeof(src_addr);

    // TODO: Memory leak
    received_packet = recv_vpn_packet(client_ctx->socket, packet_buf, sizeof(packet_buf), 0, &src_addr, &src_addr_len, 
        client_ctx->server_real_addr);
    if(received_packet == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "recv_vpn_packet(): %s\n", myvpn_strerror(myvpn_errno));
        return -1;
    }

    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Received connecstab reply packet from server\n");

    if(validate_connestab_repl(connection, received_packet) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Received invalid CON_ESTAB reply: %s\n", myvpn_strerror(myvpn_errno));
        return -1;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Connecstab reply packet is VALID\n");

    // TODO free received_packet
    return 0;
}

// TODO: also init tun, but DON'T assign ip to it
// initializes Client_ctx structure by:
//      - creates client socket
//      - creates server sockaddr and 'connects' it to client socket
//      - creates client sockaddr
//      - allocates Client_ctx structure and writes 'client_socket' and 'real_addr'
// On succress returns address to Client_ctx structure, on error NULL and 'myvpn_errno' is set to indicate an error
Client_ctx* init_client_ctx(char* client_addr, uint16_t client_port, char* server_addr, uint16_t server_port) 
{
    assert(client_addr != NULL && client_port > 0);
    struct sockaddr_in* client_sockaddr;
    struct sockaddr_in* server_sockaddr;
    int client_socket = -1;

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_socket < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        goto init_fail;
    }

    client_sockaddr = create_sockaddr_in(AF_INET, htons(client_port), client_addr);
    if(client_sockaddr == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to create sockaddr for client('%s:%hu'): %s\n",
            client_addr, client_port, myvpn_strerror(myvpn_errno)); 
        goto init_fail;
    }

    server_sockaddr = create_sockaddr_in(AF_INET, htons(server_port), server_addr);
    if(server_sockaddr == NULL){
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to create sockaddr for server('%s:%hu'): %s\n",
            client_addr, client_port, myvpn_strerror(myvpn_errno)); 
        goto init_fail;
    }
    


    if(bind(client_socket, (struct sockaddr*) client_sockaddr, sizeof(*client_sockaddr)) < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        goto init_fail;
    }

    Client_ctx* client_ctx = malloc(sizeof(Client_ctx));
    assert(client_ctx != NULL);
    memset(client_ctx, 0, sizeof(Client_ctx));
    memset(client_ctx->tun_dev, 0, IFNAMSIZ);

    client_ctx->socket = client_socket;
    client_ctx->client_real_addr = client_sockaddr;
    client_ctx->server_real_addr = server_sockaddr;
    return client_ctx; 

init_fail:
    if(client_sockaddr != NULL) free(client_sockaddr);
    if(server_sockaddr != NULL) free(server_sockaddr);
    if(client_socket > 0) close(client_socket);
    return NULL;
}

Connection* init_connection(void)
{
    Connection* connection = malloc(sizeof(Connection));
    assert(connection != NULL && "malloc() failed");
    memset(connection, 0, sizeof(Connection));

    return connection;
}

// Parses connection data for payload (server and client tun IPs, net mask, auth num) and 
// writes parsed data into 'connetion'
// on success returns 0, on error -1 and 'myvpn_errno' is set to indicate and error
int parse_connection_payload(Client_ctx* client_ctx, Connection* connection, uint8_t* payload, size_t payload_size)
{
    // TODO get rid of assert
    assert(client_ctx != NULL);
    assert(connection != NULL);
    assert(payload != NULL && payload_size);

    uint32_t server_tun_addr;
    uint32_t client_tun_addr;
    uint32_t tun_network_mask; 
    uint32_t client_authentication_num;
    
    // Could just do sizeof(uint32_t) * 4, but that way it is more clear
    int expected_payload_size = sizeof(server_tun_addr) + sizeof(client_tun_addr) + 
            sizeof(tun_network_mask) + sizeof(client_authentication_num);

    if(payload == NULL || payload_size < expected_payload_size) {
        myvpn_errno = MYVPN_E_INVALID_PAYLOAD;
        return -1;
    }
    if(connection == NULL) {
        myvpn_errno = MYVPN_E_UNINITIALIZED_CON_DATA;
        return -1;
    }

    int offset = 0;
    memcpy(&server_tun_addr, payload, sizeof(server_tun_addr));
    offset += sizeof(server_tun_addr);

    memcpy(&client_tun_addr, payload + offset, sizeof(client_tun_addr));
    offset += sizeof(client_tun_addr);

    memcpy(&tun_network_mask, payload + offset, sizeof(tun_network_mask));
    offset += sizeof(tun_network_mask);

    memcpy(&client_authentication_num, payload + offset, sizeof(client_authentication_num));

    client_ctx->server_tun_addr = server_tun_addr;
    client_ctx->tun_addr = client_tun_addr;
    connection->tun_addr = client_tun_addr;
    client_ctx->tun_mask = tun_network_mask;
    connection->authentication_num = client_authentication_num;
    return 0;
}

int validate_con_repl_pkt(Vpn_packet* packet)
{
    assert(packet != NULL);
    switch (packet->header->msg_type) {
        case MYVPN_MSG_TYPE_CONNECT_REPLY:
            return 0;
        case MYVPN_MSG_TYPE_ERROR:
            assert(0 && "TODO: received error");    // TODO parse error from packet and set errno. see NOTE#1
            return -1;
        default:
            myvpn_errno = MYVPN_E_UNEXPECTED_MSG_TYPE;
            return -1;
    }
    return 0;
}

int validate_connestab_repl(Connection* connection, Vpn_packet* packet)
{
    assert(connection != NULL);
    assert(packet != NULL);
    assert(packet->header != NULL);
    switch(packet->header->msg_type) {
        case MYVPN_MSG_TYPE_CONNECT_ESTAB:
            if(packet->header->authentication_num == connection->authentication_num) {
                return 0; // CONNECTION ESTABLISHED
            }
            else {
                myvpn_errno = MYVPN_E_INVALID_AUTH_NUM;
                return -1;
            }
            break;

        case MYVPN_MSG_TYPE_ERROR:
            assert(0 && "TODO: received error");        // See NOTE#1
            break;
        default:
            myvpn_errno = MYVPN_E_UNEXPECTED_MSG_TYPE;
            return -1;
        }
}

Raw_packet* construct_connestab_packet(Connection* connection)
{
    assert(connection != NULL);     // TODO handle gracefully
    Vpn_header header = {0};
    header.magic = MAGIC_NUM;
    header.msg_type = MYVPN_MSG_TYPE_CONNECT_ESTAB;
    header.authentication_num = connection->authentication_num;
    // TODO
    // seq
    // ack
    return construct_vpn_packet(&header, NULL, 0);
}

Raw_packet* construct_connection_packet(void)
{
    Vpn_header header = {0};
    header.magic = MAGIC_NUM;
    header.msg_type = MYVPN_MSG_TYPE_CONNECT;
    header.authentication_num = 0; // init with 0
    // TODO
    // seq
    // ack
    return construct_vpn_packet(&header, NULL, 0);
}
