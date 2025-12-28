#include "server.h"

Log_context g_log_context; 

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

    // init loggin context
    g_log_context = init_log_context(NULL, MYVPN_LOG_VERBOSE_VERBOSE);
    MYVPN_LOG(MYVPN_LOG, ">>> [ MyVPN server started ] <<<\n");

    // init server context
    Server_ctx *server_ctx = init_server_ctx(server_addr, server_port, tun_addr, tun_mask);

    if(server_ctx == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to initialize server ctx with server: '%s:%hu', tun: '%s' & '%s'\n",
            server_addr, server_port, tun_addr, tun_mask);
        exit(1);
    }
    MYVPN_LOG(MYVPN_LOG, "Successfully initialized server ctx with: '%s:%hu', tun: '%s' & '%s'\n",
        server_addr, server_port, tun_addr, tun_mask);

    Vpn_network* vpn_network = init_vpn_network(tun_addr, tun_mask, server_ctx->tun_addr);
    if(vpn_network == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to initialize tun network '%s' & '%s' \n", tun_addr, tun_mask);
        exit(1);
    }
    MYVPN_LOG(MYVPN_LOG, "initialized network %u (%u - %u) %zu addresses\n", vpn_network->network->network_addr, 
        vpn_network->network->range_start, vpn_network->network->range_end, vpn_network->network->addr_count);

    
    //else {
    //    printf("Configured tun interface '%s' on '%s' '%s' successfully\n", dev, tun_addr, tun_mask);
    //}


    MYVPN_LOG(MYVPN_LOG, "Server started on '%s:%hd'...\n", server_addr, server_port);

    // init pollfd for poll() calls
    size_t fds_count = 2;
    struct pollfd fds[fds_count];
    memset(fds, 0, sizeof(fds));

    fds[0] = (struct pollfd) { .fd = server_ctx->socket, .events = POLLIN, .revents = 0};
    fds[1] = (struct pollfd) { .fd = server_ctx->tun_fd, .events = POLLIN, .revents = 0 };
    int poll_res;

    while(1) {
        poll_res = poll(fds, fds_count, -1);
        for(int i = 0; i < fds_count; i++) {
            struct pollfd* current = &fds[i];
            if(current->revents & POLLIN == 1) {
                if (current->fd == server_ctx->socket) {
                    struct sockaddr_in src_addr = {0};
                    socklen_t src_addr_len = sizeof(struct sockaddr_in);
                    // TODO maybe let's make buffer size bigger than MTU?
                    // TODO allocate buffer on heap
                    uint8_t buff[PACKET_BUFFER_SIZE] = {0};
                    Vpn_packet* received_packet = recv_vpn_packet(server_ctx->socket, buff, sizeof(buff), 0,
                        &src_addr, &src_addr_len, NULL);    // dont need to receive from specific addr, so NULL in target filed

                    if(received_packet == NULL) {
                        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "failed receive packet. recv_vpn_packet(): %s\n", myvpn_strerror(myvpn_errno));
                        continue;
                    }

                    printf("got here\n");
                    //char peer_addr[16];
                    //inet_ntop(AF_INET, &src_addr.sin_addr, peer_addr, sizeof(peer_addr));

                    //printf("got message on socket (%d) ('%s:%hu'):\n", current->fd, 
                    //        peer_addr, ntohs(src_addr.sin_port));
                    _print_packet(buff, sizeof(Vpn_header) + received_packet->payload_size);
                    if(handle_received_packet(server_ctx, vpn_network, received_packet, &src_addr, &src_addr_len) < 0) {
                        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to handle received packet from '%u:%hu'\n",
                            ntohs(src_addr.sin_addr.s_addr), ntohs(src_addr.sin_port));
                        continue;
                    }
                }
                else if (current->fd == server_ctx->tun_fd) {
                    printf("got message on tun (%d):\n", current->fd);
                    // TODO allocate buffer on heap
                    uint8_t buff[PACKET_BUFFER_SIZE];
                    ssize_t read_bytes = 0;
                    read_bytes = read(current->fd, buff, sizeof(buff));
                    if(read_bytes == 0) {
                        fprintf(stderr, "EOF reached on '%d'\n", current->fd);
                        continue;
                    }
                    else if(read_bytes < 0) {
                        fprintf(stderr, "read(%d): %s\n", current->fd, strerror(errno));
                        continue;
                    }
                    else {
                        // TODO
                        // TODO check for dst and src addr, and send accordiong to clients table
                        _print_packet(buff, read_bytes);
                        Ip_data ip_data = parse_ip_header(buff, read_bytes);
                        printf("version: %d\n", ip_data.ip_version);

                        if(ip_data.ip_version == 6 || ip_data.ip_version != 4) {
                            continue;
                        }
                        struct sockaddr_in* client = ht_find(vpn_network->tun_to_ip_route_table, 
                            (void*)(ip_data.src_addr_v4));
                        if(client == NULL) {
                            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Could not find dst client in 'tun_to_ip_route_table'\n");
                            continue;
                        }
                        Connection* client_data = ht_find(vpn_network->clients_table, client);
                        if(client_data == NULL) {
                            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Could not find dst client in 'clients_table'\n");
                            continue;
                        }

                        Raw_packet* encapsulated_packet = encapsulate_data_packet(client_data->authentication_num, buff, read_bytes);
                        if(encapsulated_packet == NULL) {
                            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to encapsulate data packet: %s\n", 
                                myvpn_strerror(myvpn_errno));
                            continue;
                        }
                        if(send_vpn_packet(server_ctx->socket, encapsulated_packet, 0, client) < 0) {
                            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to send packet to '%u:%hu': %s\n", 
                                ntohl(client->sin_addr.s_addr), ntohs(client->sin_port), myvpn_strerror(myvpn_errno));
                            continue;
                        }
                        MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Successfully sent packet to '%u:%hu'\n", 
                            ntohl(client->sin_addr.s_addr), ntohs(client->sin_port));
                    }
                }
                else {
                    MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Unexpected fd - %d\n", current->fd);
                }
            }
        }
    }

    return 0;
}

int handle_received_packet(Server_ctx* server_ctx, Vpn_network* vpn_network, Vpn_packet* packet, struct sockaddr_in* src_addr, socklen_t* src_addr_len)
{
    if(packet == NULL) {
        myvpn_errno = MYVPN_E_PKT_NULL;
        return -1;
    }
    if(src_addr == NULL || src_addr_len == NULL | *src_addr_len <= 0) {
        myvpn_errno = MYVPN_E_ADDR_NULL;    // TODO myvpn_strerror for that error doesnt say anything about 'src_addr_len' ptr
        return -1;
    }
    assert(packet->header != NULL);
    assert(vpn_network != NULL);
    switch(packet->header->msg_type) {
        case MYVPN_MSG_TYPE_CONNECT:
            if(accept_connection(server_ctx, vpn_network, packet, src_addr, src_addr_len) < 0) {
                return -1;
            }
            break;
        case MYVPN_MSG_TYPE_CONNECT_ESTAB:
            if(estab_connection(server_ctx, vpn_network, packet, src_addr, src_addr_len) < 0) {
                return -1;
            }
            break;
        case MYVPN_MSG_TYPE_DATA:
            if(handle_data_packet(server_ctx, vpn_network, packet, src_addr, src_addr_len) < 0) {
                return -1;
            }
            return 0;
        default:
            myvpn_errno = MYVPN_E_UNEXPECTED_MSG_TYPE;    
            return -1;
    }
}

int accept_connection(Server_ctx* server_ctx, Vpn_network* vpn_network, Vpn_packet* packet, struct sockaddr_in* src_addr, socklen_t* src_addr_len)
{
    assert(vpn_network != NULL);
    assert(packet != NULL);
    assert(packet->header != NULL);
    assert(src_addr != NULL);
    assert(src_addr_len != NULL);
    assert(vpn_network->network_address_pool != NULL);
    assert(vpn_network->network != NULL);

    myvpn_errno_t error_code; 
    
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Received connection request from '%u:%hu'\n",
            ntohl(src_addr->sin_addr.s_addr), ntohs(src_addr->sin_port));

    if(packet->header->authentication_num != 0) {
        // myvpn_errno = MYVPN_E_INVALID_AUTH_NUM;      // if sending error msg fails this will be overwritten anyway
        error_code = MYVPN_E_INVALID_AUTH_NUM;
        MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Received invalid authenticain number: %d\n", 
            ntohl(packet->header->authentication_num));
        goto send_err_msg;
    }

    uint32_t assigned_client_tun = request_address(vpn_network->network_address_pool, vpn_network->network->network_addr);
    if(assigned_client_tun == 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to assign tun address for '%u:%hu' : %s\n", 
            ntohl(src_addr->sin_addr.s_addr), ntohs(src_addr->sin_port), myvpn_strerror(myvpn_errno));
        error_code = MYVPN_EM_INTERNAL_SERVER_ERR;
        goto send_err_msg; 
    }
    Raw_packet* con_repl_packet = construct_con_repl_packet(htonl(TODO_AUTH_NUM), TODO_SEQ_NUM, TODO_ACK_NUM, server_ctx->tun_addr, server_ctx->tun_mask, htonl(assigned_client_tun));
    if(con_repl_packet == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to construct CONNECT_REPLY packet for '%u:%hu' : %s\n", 
            ntohl(src_addr->sin_addr.s_addr), ntohs(src_addr->sin_port), myvpn_strerror(myvpn_errno));
        error_code = MYVPN_EM_INTERNAL_SERVER_ERR;
        goto send_err_msg;
    }

    // Retain creating unverified client record as possible. That is why is is between creating and sending CON_REPL packet

    // TODO set up timer for waiting for the response
    // Create client and add to unverified list
    Connection* client_connection = malloc(sizeof(Connection));
    assert(client_connection != NULL && "malloc() failed");
    client_connection->tun_addr = assigned_client_tun;
    client_connection->authentication_num = htonl(TODO_AUTH_NUM);
    client_connection->seq_num = TODO_SEQ_NUM;
    client_connection->ack_num = TODO_ACK_NUM;
    client_connection->connection_state = CON_UNVERIFIED;


    // TODO: make sure, that if client is connected, if somebody would send fake connection packet, client won't be deleted
    //      ht_add() will return false, if client is already in hash table, so it is probably fine
    //      but in that case maybe set proper errno (or maybe not, let's not expose any info to hacker)
    if(!(ht_add(vpn_network->unverified_client_connections, 
        src_addr, sizeof(*src_addr), // copy whole struct
        client_connection, HT_PTR_IS_DATA
        ))) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to add client '%u:%hu' to unverified list\n", 
            ntohl(src_addr->sin_addr.s_addr), ntohs(src_addr->sin_port));
        error_code = MYVPN_EM_INTERNAL_SERVER_ERR;
        goto send_err_msg;
    }

    if(send_vpn_packet(server_ctx->socket, con_repl_packet, 0, src_addr) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to send CONNECT_REPLY packet for '%u:%hu' : %s\n", 
            ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), myvpn_strerror(myvpn_errno));
        if(!(ht_remove(vpn_network->unverified_client_connections, src_addr))) {
            MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to remove dangling client '%u:%hu' from unverified list\n", 
                ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port));
        }
        return -1;      // makes no sense to send error message if this one failed
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE, "Added client '%u:%hu' (tun ip: '%u') to unverified connection list\n", 
                ntohl(src_addr->sin_addr.s_addr), ntohs(src_addr->sin_port), assigned_client_tun);
    return 0;

send_err_msg:
    Raw_packet* error_packet = construct_error_packet(TODO_AUTH_NUM, TODO_SEQ_NUM, TODO_ACK_NUM, error_code);
    if(error_packet == NULL) {
         MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to create error packet with code: %d : %s\n", 
             error_code, myvpn_strerror(myvpn_errno));
         return -1;
    }
    // 'src_addr' is client addr, from which packet was received from
    // just sending it back. Might seem a little confsing
    if(send_vpn_packet(server_ctx->socket, error_packet, 0, src_addr) < 0) {
         MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to send error message with code: %d to '%u:%hu' : %s\n", 
             error_code, ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), myvpn_strerror(myvpn_errno));
         return -1;
    }
    return -1;   // error message was sent successfully, but we still failed to connect client
}

// Be careful, this functing is ugly as ****, i warned you)
int estab_connection(Server_ctx* server_ctx, Vpn_network* vpn_network, Vpn_packet* packet, struct sockaddr_in* src_addr, socklen_t* src_addr_len)
{
    assert(vpn_network != NULL);
    assert(vpn_network->unverified_client_connections != NULL);
    assert(vpn_network->clients_table != NULL);
    assert(packet != NULL);
    assert(packet->header != NULL);
    assert(src_addr != NULL);
    assert(src_addr_len != NULL);
   
    myvpn_errno_t error_code;

    Connection* unverified_connection = ht_find(vpn_network->unverified_client_connections, src_addr);
    if(unverified_connection == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Could not find client '%u:%hu' in unverified connection list\n",
            ntohl(src_addr->sin_addr.s_addr), ntohs(src_addr->sin_port));
        error_code = MYVPN_E_CLIENT_NOT_FOUND;
        goto send_err_msg;
    }
    if(unverified_connection->connection_state != CON_UNVERIFIED || 
        packet->header->msg_type != MYVPN_MSG_TYPE_CONNECT_ESTAB) {     // dont need this check since it will be checked in handle_received_packet()
        myvpn_errno = MYVPN_E_UNEXPECTED_MSG_TYPE;
        error_code = MYVPN_E_UNEXPECTED_MSG_TYPE;
        MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Received unexpected packet (type: %hhu) client con state: %d(expected: %d)\n",
            packet->header->msg_type, unverified_connection->connection_state, CON_UNVERIFIED);
        goto send_err_msg;
    }
    if(unverified_connection->authentication_num != packet->header->authentication_num) {
        myvpn_errno = MYVPN_E_INVALID_AUTH_NUM;
        error_code = MYVPN_E_INVALID_AUTH_NUM;
        MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Received con_estab request with invalid authentication num: %u\n",
            packet->header->authentication_num);
        goto send_err_msg;
    }
    Raw_packet* conestab_repl_packet = construct_connestab_repl_packet(unverified_connection->authentication_num,
        TODO_SEQ_NUM, TODO_ACK_NUM);
    if(conestab_repl_packet == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to construct 'conn_estab' reply packet: %s\n", myvpn_strerror(myvpn_errno));
        error_code = MYVPN_EM_INTERNAL_SERVER_ERR;
        goto send_err_msg;
    }

    unverified_connection->connection_state = CON_ESTAB;

    // Remove client unverified connection from unverified connections table first, so dont have to do remove client from
    // 'clients_table' if this one fails
    // adding client to client list fails.
    // WARNING
    // This should be fine since in hash table only pointers are stored and we saved id in a local variable.
    // But still, BE CAUTIOS!
    if(!ht_remove(vpn_network->unverified_client_connections, src_addr)) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to remove client(%u:%hu tun: %u, auth: %u) 'unverified_client_connections'\n",
            ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
            unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));
        error_code = MYVPN_EM_INTERNAL_SERVER_ERR;
        goto send_err_msg;
    }
    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Removed client(%u:%hu tun: %u, auth: %u) from 'unverified_client_connections'\n",
        ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
        unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));

    // add to clients table
    if(!ht_add(vpn_network->clients_table, src_addr, sizeof(*src_addr), unverified_connection, HT_PTR_IS_DATA)) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to add client(%u:%hu tun: %u, auth: %u) to 'clients_table'\n",
            ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
            unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));
        error_code = MYVPN_EM_INTERNAL_SERVER_ERR;
        goto send_err_msg;
    }

    // add to tun_to_ip routing table
    if(!ht_add(vpn_network->tun_to_ip_route_table, (void*)(unverified_connection->tun_addr), HT_PTR_IS_DATA, 
            src_addr, sizeof(*src_addr))) {

        MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to add client(%u:%hu tun: %u, auth: %u) to 'clients_table'\n",
            ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
            unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));

        // remove from clients table
        if(!ht_remove(vpn_network->clients_table, src_addr)) {
            MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to remove client(%u:%hu tun: %u, auth: %u) 'clients_table'\n",
                ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
                unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));
        }

        error_code = MYVPN_EM_INTERNAL_SERVER_ERR;
        goto send_err_msg;
    }

    MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Added client(%u:%hu tun: %u, auth: %u) to 'clients_table'\n",
        ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
        unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));


    if(send_vpn_packet(server_ctx->socket, conestab_repl_packet, 0, src_addr) < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to send CONNECT_ESTAB_REPLY packet for '%u:%hu'(tun: %u, auth:%u) : %s\n", 
            ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
            unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num),
            myvpn_strerror(myvpn_errno));

        // remove from clients table
        if(!ht_remove(vpn_network->clients_table, src_addr)) {
            MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to remove client(%u:%hu tun: %u, auth: %u) 'clients_table'\n",
                ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
                unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));
        }

        // remove from 'tun_to_ip_routing_table'
        if(!ht_remove(vpn_network->tun_to_ip_route_table, (void*)(unverified_connection->tun_addr))) {
            MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to remove client(%u:%hu tun: %u, auth: %u) 'tun_to_ip_route_table'\n",
                ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), 
                unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));
        }
        return -1;      // makes no sense to send error message if this one failed
    }

    

    MYVPN_LOG(MYVPN_LOG_VERBOSE, "ESTABLISHED connection with client '%u:%hu' (tun: %u, auth: %u)\n", 
                ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port),
                unverified_connection->tun_addr, ntohl(unverified_connection->authentication_num));
    return 0;

    
send_err_msg:
    Raw_packet* error_packet = construct_error_packet(TODO_AUTH_NUM, TODO_SEQ_NUM, TODO_ACK_NUM, error_code);
    if(error_packet == NULL) {
         MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to create error packet with code: %d : %s\n", 
             error_code, myvpn_strerror(myvpn_errno));
         return -1;
    }
    // 'src_addr' is client addr, from which packet was received from
    // just sending it back. Might seem a little confsing
    if(send_vpn_packet(server_ctx->socket, error_packet, 0, src_addr) < 0) {
         MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to send error message with code: %d to '%u:%hu' : %s\n", 
             error_code, ntohl(src_addr->sin_addr.s_addr), ntohl(src_addr->sin_port), myvpn_strerror(myvpn_errno));
         return -1;
    }
    return -1;   // error message was sent successfully, but we still failed to connect client
}

// if encapsulated IP packet DST addr == server tun addr - writes packet to tun interface
// otherwise looks for client addr in 'tun_to_ip_route_table' and in clients table, if fount - routes packet to client
//
// On success returns 0, on error -1 and 'myvpn_errno' is set to indicat and error (BUT NOT IN ALL CASES)
int handle_data_packet(Server_ctx* server_ctx, Vpn_network* vpn_network, Vpn_packet* packet, struct sockaddr_in* src_addr, socklen_t* src_addr_len)
{
    assert(server_ctx != NULL);
    assert(vpn_network != NULL);
    assert(packet != NULL);
    assert(src_addr != NULL);
    assert(src_addr_len != NULL);
    assert(packet->payload != NULL && packet->payload_size > 0);
    assert(vpn_network->tun_to_ip_route_table != NULL);
    assert(vpn_network->clients_table != NULL);

    Ip_data ip_header_data= parse_ip_header(packet->payload, packet->payload_size);
    if(ip_header_data.ip_version != 4) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Received unsupported IP version packet");
        myvpn_errno = MYVPN_E_IP_V_NOSUPPORT;
        return -1;
    }
    else if(ip_header_data.src_addr_v4 == 0 || ip_header_data.dst_addr_v4 == 0) {
        myvpn_errno = MYVPN_E_INVALID_IP_ADDRESS;
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Could not parse IP header addresses");
        return -1;
    }

    // If encapsulated IP dst addr == server tun_addr - just write it to servers tun
    if(server_ctx->tun_addr == ip_header_data.dst_addr_v4) {
        ssize_t written_bytes = write(server_ctx->tun_fd, packet->payload, packet->payload_size);
        if(written_bytes < 0) {
            MYVPN_LOG(MYVPN_LOG_ERROR, "Faild to write packet to '%s': %s\n", server_ctx->tun_dev, strerror(errno));
            myvpn_errno = MYVPN_E_USE_ERRNO;
            return -1;
        }
        else if(written_bytes == 0) {   // TODO dgram sockets allow packets with 0 size, mb dont interprete it as error
            MYVPN_LOG(MYVPN_LOG_ERROR, "Wrote 0 bytes to '%s' \n", server_ctx->tun_dev);
            myvpn_errno = MYVPN_E_USE_ERRNO;
            return -1;
        }
        else if(written_bytes != packet->payload_size) {
            MYVPN_LOG(MYVPN_LOG_WARNING, "Wrote %d/%d bytes of packet\n", written_bytes, packet->payload_size);
            myvpn_errno = MYVPN_E_INCOMPLETE_PACKET;
            return -1;
        }
        else {
            MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Successfully wrote packet(%hu bytes) to '%s'\n",
                     packet->payload_size, server_ctx->tun_dev);
            return 0;
        }
    }
    // otherwise route to client or drop packet if one was not found
    else { // dont need else in here, but this way it is more explicit
        struct sockaddr_in* dst_client_real_addr = ht_find(vpn_network->tun_to_ip_route_table, 
        (void*)(ntohl(ip_header_data.dst_addr_v4)));
        if(dst_client_real_addr == NULL) {
            myvpn_errno = MYVPN_E_CLIENT_NOT_FOUND;
            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Could not find client IP mapping for tun IP %d\n", ntohl(ip_header_data.dst_addr_v4));
            return -1;
        }
        Connection *client_connection = ht_find(vpn_network->clients_table, dst_client_real_addr);
        if(client_connection == NULL) {
            myvpn_errno = MYVPN_E_CLIENT_NOT_FOUND;
            MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Could not find client record with ip %d\n", 
                ntohl(dst_client_real_addr->sin_addr.s_addr));
            return -1;
        }

        // TODO: since Raw_packet->payload is just a pointer with offset from [Vpn_header][encapsulated_payload]
        //       just change neccessary data in raw_packet->payload - required_offset and send 
        //                                                      (uint8_t*)(raw_packet->payload) - sizeof(Vpn_header)
        //      this will give big permormance improvement
        Raw_packet* encapsulated_packet = encapsulate_data_packet(client_connection->authentication_num, packet->payload,
            packet->payload_size);
        if(send_vpn_packet(server_ctx->socket, encapsulated_packet, 0, dst_client_real_addr) < 0) {
            MYVPN_LOG(MYVPN_LOG_ERROR, "Failed to send data packet to client %d(tun: %d): %s\n ",
                ntohl(dst_client_real_addr->sin_addr.s_addr), ntohl(ip_header_data.dst_addr_v4), myvpn_strerror(myvpn_errno));
            return -1;
        }
        else {
            MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Routed data packet to client %d(tun: %d)",
                ntohl(dst_client_real_addr->sin_addr.s_addr), ntohl(ip_header_data.dst_addr_v4));
            return 0;
        }
    }
}


// TODO: also init tun, but DON'T assign ip to it
// creates 'Server_ctx' structure and fills all fields with corresponding data based on supplied function arguments
// on success returns address to created structure, on error NULL and 'myvpn_errno' is set to indicate an error
Server_ctx* init_server_ctx(char* server_addr, uint16_t server_port, char* tun_addr, char* tun_mask)
{
    assert(server_addr != NULL);
    assert(tun_addr != NULL);
    assert(tun_mask != NULL);

    struct sockaddr_in* local;
    
    Server_ctx* server_ctx = malloc(sizeof(Server_ctx));
    assert(server_ctx != NULL && "malloc() failed");
    memset(server_ctx, 0, sizeof(server_ctx));

    // clear tun interface name
    memset(server_ctx->tun_dev, 0, IFNAMSIZ);

    // init socket
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_socket < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        goto init_fail;
    }

    local = create_sockaddr_in(AF_INET, htons(server_port), server_addr);
    if(local == NULL) {
        // fprintf(stderr, "create_sockaddr_in(): %s\n" myvpn_strerror(myvpn_errno));
        goto init_fail;  // create_sockaddr_in() will set myvpn_errno
    }
    if(bind(server_socket, (struct sockaddr*)local, sizeof(*local)) < 0) {
        //fprintf(stderr, "bind(): %s\n", strerror(errno));
        myvpn_errno = MYVPN_E_USE_ERRNO;
        goto init_fail;
    }

    // TODO dont use single function to init tun, use defined in 'tun.h'
    // so there is no need to conver tun addr and mask second time
    int tun_fd = init_tun_if(server_ctx->tun_dev, tun_addr, tun_mask);
    if(tun_fd < 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR, "Error setting up tun interface\n");
        myvpn_errno = MYVPN_E_USE_ERRNO;
        goto init_fail;
    }
    uint32_t tun_addr_num;
    uint32_t tun_mask_num;
    if(get_ipv4_numeric_addr(tun_addr, &tun_addr_num) < 0) {
        // TODO set errno
        MYVPN_LOG(MYVPN_LOG_ERROR, "Error converting tun address\n");
        goto init_fail;
    }
    if(get_ipv4_numeric_addr(tun_mask, &tun_mask_num) < 0) {
        // TODO set errno
        MYVPN_LOG(MYVPN_LOG_ERROR, "Error converting tun mask\n");
        goto init_fail;
    }
    server_ctx->socket = server_socket;
    server_ctx->real_addr = local;
    server_ctx->tun_fd = tun_fd;
    server_ctx->tun_addr = tun_addr_num;
    server_ctx->tun_mask = tun_mask_num;
    return server_ctx;


init_fail:
    if(local != NULL) free(local);
    if(server_ctx != NULL) free(server_ctx);
    return NULL;
}
