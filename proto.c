#include <netinet/in.h>
#include "proto.h"
#include "utils.h"


// TODO: make construct_* specific functions as a macro


Raw_packet* construct_connestab_repl_packet(uint32_t auth_num, uint32_t seq_num, uint32_t ack_num) 
{
    UNUSED(seq_num);    // TODO
    UNUSED(ack_num);    // TODO        
    Vpn_header __conestab_repl_header = {0};
    __conestab_repl_header.magic = MAGIC_NUM;
    __conestab_repl_header.msg_type = MYVPN_MSG_TYPE_CONNECT_ESTAB;
    __conestab_repl_header.authentication_num = auth_num;
    return construct_vpn_packet(&__conestab_repl_header, NULL, 0);
}


// 'error_code' MUST be passed in HOST byte order. 
// NOTE#1: Because of enums elemet type is not defined in C standart, it can vary among systems.
// In order to fix this, just assign enum element to uint32_t and do htonl();
Raw_packet* construct_error_packet(uint32_t authentication_num, uint32_t seq_num, uint32_t ack_num, myvpn_errno_t error_code)
{
    UNUSED(seq_num);    // TODO    
    UNUSED(ack_num);    // TODO
    Vpn_header header = {0};
    header.magic = MAGIC_NUM;
    header.msg_type = MYVPN_MSG_TYPE_ERROR;
    header.authentication_num = authentication_num;

    uint32_t error_code_fixed_size = error_code;
    error_code_fixed_size = htonl(error_code_fixed_size);

    return construct_vpn_packet(&header, (uint8_t*)&error_code, sizeof(error_code_fixed_size));
}

// all args MUST be in NETWORK byte order (mb except for 'seq_num' and 'ack_num')
Raw_packet* construct_con_repl_packet(uint32_t authentication_num, uint32_t seq_num, uint32_t ack_num, 
        uint32_t server_tun_ip, uint32_t tun_net_mask, uint32_t assigned_client_tun_ip)
{
    UNUSED(seq_num);    // TODO    
    UNUSED(ack_num);    // TODO
    Vpn_header header = {0};
    header.magic = MAGIC_NUM;
    header.msg_type = MYVPN_MSG_TYPE_CONNECT_REPLY;
    header.authentication_num = authentication_num;
    // just to be explicit, compiler will optimize it anyway
    size_t payload_size = sizeof(authentication_num) + sizeof(server_tun_ip) + 
                sizeof(tun_net_mask) + sizeof(assigned_client_tun_ip);

    uint8_t* payload = malloc(payload_size * sizeof(uint8_t));
    assert(payload != NULL && "malloc() failed");
    size_t offset = 0;
    memcpy(payload, &server_tun_ip, sizeof(server_tun_ip));
    offset += sizeof(server_tun_ip);

    memcpy(payload + offset, &assigned_client_tun_ip, sizeof(assigned_client_tun_ip));
    offset += sizeof(assigned_client_tun_ip);

    memcpy(payload + offset, &tun_net_mask, sizeof(tun_net_mask));
    offset += sizeof(tun_net_mask);

    memcpy(payload + offset, &authentication_num, sizeof(authentication_num));
    return construct_vpn_packet(&header, payload, payload_size); 
}

Raw_packet* encapsulate_data_packet(uint32_t authentication_num, uint8_t* raw_packet, size_t packet_size)
{
    assert(raw_packet != NULL && packet_size > 0);

    Vpn_header header = {0};
    header.magic = MAGIC_NUM;
    header.flags = 0;
    header.msg_type = MYVPN_MSG_TYPE_DATA;
    header.seq_num = 0;    // TODO
    header.ack_num = 0;    // TODO
    header.authentication_num = authentication_num;  // Authenticatin number is stored in NETWORK byte order on both sides
    //header.packet_size = htons(sizeof(Vpn_header) + packet_size);
    Raw_packet* packet = construct_vpn_packet(&header, raw_packet, packet_size);
    return packet;
}

// allocates new array[sizeof(Vpn_header) + payload_size) and copies data from 'header' and 'payload'
// on success returns address to allocated 'Raw_packet' structure, 
// on error NULL and 'myvpn_errno' is set to indicate an error
Raw_packet* construct_vpn_packet(Vpn_header* header, uint8_t* payload, size_t payload_size)
{
    assert(header != NULL);     // TODO handle gracefully
    if((payload_size + sizeof(Vpn_header)) > MTU) {     // TODO '>' or '>=' ?
        myvpn_errno = MYVPN_E_PSIZE_BIGGER_MTU;
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Packet size (header + payload size) (%zu) > MTU (%d)\n", 
            payload_size + sizeof(header), MTU);
        return NULL;
    }
    if((payload == NULL && payload_size > 0) || (payload != NULL && payload_size == 0)) {
        myvpn_errno = MYVPN_E_INVALID_PAYLOAD;
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Invalid payload: %p (size: %zu)\n", payload, payload_size);
        return NULL;
    }

    header->packet_size = sizeof(Vpn_header) + payload_size;
    

    size_t header_size = sizeof(Vpn_header);
    // header MUST be calculated in HOST_BO, see calculate_checksum() comments
    uint16_t checksum = calculate_checksum(header, header_size, payload, payload_size);
    printf("n %hu    h %hu\n", checksum, htons(checksum));
    header->checksum = checksum;    // will be converted later with HEADER_TO_NETWORK_BO()

    uint16_t vpn_packet_size = header_size + payload_size;
    uint8_t* vpn_packet_payload = malloc(vpn_packet_size);
    memset(vpn_packet_payload, 0, vpn_packet_size);

    // convert header to NETWORK byte order
    HEADER_TO_NETWORK_BO(header);
    // write each field of header structure
    off_t packet_offset = 0;
    memcpy(vpn_packet_payload + packet_offset, &(header->magic), sizeof(header->magic));
    packet_offset += sizeof(header->magic);

    memcpy(vpn_packet_payload + packet_offset, &(header->packet_size), sizeof(header->packet_size));
    packet_offset += sizeof(header->packet_size);

    memcpy(vpn_packet_payload + packet_offset, &(header->flags), sizeof(header->flags));
    packet_offset += sizeof(header->flags);

    memcpy(vpn_packet_payload + packet_offset, &(header->msg_type), sizeof(header->msg_type));
    packet_offset += sizeof(header->msg_type);

    memcpy(vpn_packet_payload + packet_offset, &(header->checksum), sizeof(header->checksum));
    packet_offset += sizeof(header->checksum);

    memcpy(vpn_packet_payload + packet_offset, &(header->seq_num), sizeof(header->seq_num));
    packet_offset += sizeof(header->seq_num);

    memcpy(vpn_packet_payload + packet_offset, &(header->ack_num), sizeof(header->ack_num));
    packet_offset += sizeof(header->ack_num);

    memcpy(vpn_packet_payload + packet_offset, &(header->authentication_num), sizeof(header->authentication_num));
    packet_offset += sizeof(header->authentication_num);

    // copy payload of packet
    if(payload != NULL)
        memcpy(vpn_packet_payload+packet_offset, payload, payload_size);

#ifdef MYVPN_DEBUG_PROTO
    _print_packet(vpn_packet_payload, vpn_packet_size);
#endif

    Raw_packet* vpn_packet = malloc(sizeof(Raw_packet));
    assert(vpn_packet != NULL && "malloc() failed");
    vpn_packet->data = vpn_packet_payload;
    vpn_packet->data_size = vpn_packet_size ;

    return vpn_packet;
}


/* WARNING 
 if packet is MyVPN packet:
  field 'payload' contains a pointer to 'raw_packet' + sizeof(Vpn_header). Take that in mind when you alloc or free smth
*/
/* parses raw vpn packet:
        - creates Vpn_header structure and fills each field with corresponding data from 'raw_packet'
        - writes 'raw_packet' + packet_size offset to 'payload' pointer field (if packet is actually MyVPN packet)
        - writes 'packet_size' - sizeof(Vpn_header) to 'payload_size' field

    On success returns pointer to 'Vpn_packet' structer with all fields filled up in HOST byte order
    If packet is not MyVPN packet, 'header' field is set as NULL and 'payload' pointer is set to 'raw_packet'
        so does 'payload_size' is set to 'packet_size'. 'header' == NULL indicates that packet is not from MyVPN protocol
    On error returns NULL 
*/
Vpn_packet* parse_packet(uint8_t* raw_packet, size_t packet_size)
{
    assert(raw_packet != NULL && packet_size > 0);
    Vpn_packet* parsed_packet = malloc(sizeof(Vpn_packet));

#ifdef MYVPN_DEBUG_PROTO
    _print_packet(raw_packet, packet_size);
#endif

    uint16_t packet_magic;
    off_t packet_offset = 0;
    // copy magic num first
    memcpy(&packet_magic, raw_packet + packet_offset, sizeof(packet_magic));
    packet_offset += sizeof(packet_magic);

    // if first 2 bytes in received packet is not magic num - it is not vpn packet, ju return raw packet
    if(packet_magic != ntohs(MAGIC_NUM)) {
        parsed_packet->header = NULL;
        parsed_packet->payload = raw_packet;
        parsed_packet->payload_size = packet_size;
        return parsed_packet;
    }

    // else - parse whole packet in NETWORK byte order (after that convert to HOST_BO)
    Vpn_header* header = malloc(sizeof(Vpn_header));
    header->magic = packet_magic;

    memcpy(&(header->packet_size), raw_packet + packet_offset, sizeof(header->packet_size));
    packet_offset += sizeof(header->packet_size);
    if(ntohs(header->packet_size) != packet_size) {
        free(header);   // TODO in future, when migrate to arena allocators dont free 
        myvpn_errno = MYVPN_E_PKTS_SIZE_DONT_MATCH;
        MYVPN_LOG(MYVPN_LOG_WARNING, "Received packet size(%hu) and size in header(%hu) dont match\n", 
            packet_size, ntohs(header->packet_size));
        return NULL; 
    }

    memcpy(&(header->flags), raw_packet + packet_offset, sizeof(header->flags));
    packet_offset += sizeof(header->flags);

    memcpy(&(header->msg_type), raw_packet + packet_offset, sizeof(header->msg_type));
    packet_offset += sizeof(header->msg_type);

    uint16_t packet_checksum;
    memcpy(&(packet_checksum), raw_packet + packet_offset, sizeof(header->checksum));
    packet_checksum = ntohs(packet_checksum);   // convert to HOST_BO for future verification
    header->checksum = 0;   // for correct checksum verification this should be 0
    packet_offset += sizeof(header->checksum);

    memcpy(&(header->seq_num), raw_packet + packet_offset, sizeof(header->seq_num));
    packet_offset += sizeof(header->seq_num);

    memcpy(&(header->ack_num), raw_packet + packet_offset, sizeof(header->ack_num));
    packet_offset += sizeof(header->ack_num);

    memcpy(&(header->authentication_num), raw_packet + packet_offset, sizeof(header->authentication_num));
    packet_offset += sizeof(header->authentication_num);



    HEADER_TO_HOST_BO(header);
    uint8_t* payload_ptr;
    size_t payload_size;
    if(packet_size - packet_offset == 0) {
        payload_ptr = NULL;     // WARNING: calculate_checksum() can handle this, but not sure about others
        payload_size = 0;
    }
    else {
        payload_ptr = raw_packet + packet_offset;
        payload_size = packet_size - packet_offset;
    }
    printf("%p, %zu\n", payload_ptr, payload_size);
    uint16_t calc_packet_checksum = calculate_checksum(header, sizeof(Vpn_header), payload_ptr, payload_size);

    if(packet_checksum != calc_packet_checksum) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Invalid packet checksum: got: %hu     calculated: %hu \n", 
            packet_checksum, calc_packet_checksum);
        myvpn_errno = MYVPN_E_INVALID_CHECKSUM;
        //free(header);
        //return NULL;
    }

    header->checksum = packet_checksum; // already in HOST_BO


    parsed_packet->header = header;
    parsed_packet->payload = payload_ptr;
    parsed_packet->payload_size = payload_size;

    return parsed_packet;
}


// Receives packet and writes it to 'buf' with 'size'. Packet src addr is written to 'src_addr' and its size to 'addrlen'.
// Allocates 'Vpn_packet' structure and writes pointers of header and data with corresponding offsets (see parse_packet() doc)
// 'recv_targer' is used when you need to receive packets ONLY from specific address. 
//  If 'recv_target' field is NULL, recv_vpn_packet() will accept received packet from ANY ip address.
//  If 'recv_targer' is a pointer to sockaddr struct from which receive packet, recv_vpn_packet() will drop any packet that
//      is not 'recv_target' and return an error
//
// On success returns pointer to allocated 'Vpn_packet' struct, on error NULL and 'myvpn_errno' is set to indicate an error
Vpn_packet* recv_vpn_packet(int client_socket, uint8_t* buf, size_t size, int flags, struct sockaddr_in* src_addr, socklen_t* addrlen, struct sockaddr_in* recv_target)
{
    assert(src_addr != NULL);
    assert(addrlen != NULL);
    // TODO validate 'seq' and 'ack' numbers
    ssize_t received_bytes = recvfrom(client_socket, buf, size, flags, (struct sockaddr*)src_addr, addrlen);
    if(received_bytes < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        return NULL;
    }
    else if(received_bytes == 0) {  // TODO maybe dont need that, datagram sockets allow packets with 0 size
        myvpn_errno = MYVPN_E_PKT_ZERO_SIZE;
        return NULL;
    }
    if(recv_target != NULL) {
        if(!PACKET_FROM_TARGET(src_addr, recv_target)) {
            MYVPN_LOG(MYVPN_LOG_VERBOSE_VERBOSE, "Received packet is not from server (recv: '%u:%hu'  serv: '%u:%hu')\n",
            ntohl(src_addr->sin_addr.s_addr), ntohs(src_addr->sin_port), 
            ntohl(recv_target->sin_addr.s_addr), ntohs(recv_target->sin_port)); 
            myvpn_errno = MYVPN_E_RECVPKT_NOT_FROM_TARGET;
            return NULL;
        }
    }
    Vpn_packet* packet = parse_packet(buf, received_bytes);
    if(packet == NULL) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Failed to parse packet: %s\n", myvpn_strerror(myvpn_errno));
        return NULL;
    }
    if(packet->header == NULL) {
        myvpn_errno = MYVPN_E_NOT_MYVPN_PACKET;
        free(packet);
        return NULL;
    }
    if(packet->header->packet_size != received_bytes) {
        myvpn_errno = MYVPN_E_INCOMPLETE_PACKET;
        free(packet);
        return NULL;
    }
    return packet;
}


// sends 'packet' to 'dst_addr' using sendto()
// On success returns number of bytes sent. On error, -1 is returned and 'myvpn_errno' is set to error number
ssize_t send_vpn_packet(int client_socket, Raw_packet* packet, int flags, struct sockaddr_in* dst_addr)
{
    // TODO validate 'seq' and 'ack' numbers
    assert(packet != NULL);
    assert(dst_addr != NULL);
    if(packet->data_size > MTU) {
        myvpn_errno = MYVPN_E_PSIZE_BIGGER_MTU;
        return -1;
    }
    if(dst_addr->sin_family != AF_INET) {
        myvpn_errno = MYVPN_E_AFNOSUPPORT;
        return -1;
    }

    ssize_t sent_bytes = sendto(client_socket, packet->data, packet->data_size, flags, (struct sockaddr*)dst_addr, sizeof(*dst_addr));
    if(sent_bytes < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        return -1;
    }
    else if(sent_bytes == 0) {
        myvpn_errno = MYVPN_E_PKT_ZERO_SIZE;
        return -1;
    }
    else if(sent_bytes != packet->data_size) {
        myvpn_errno = MYVPN_E_INCOMPLETE_PACKET;
        return -1;
    }
    else {
        return sent_bytes;
    }
}

// TODO in which file place this func?
// write 'packet' of size 'packet_size, to tun device opened at 'tun_fd'.
// On success returns number of bytes written, or error -1 and 'myvpn_errno' is set to indicate an error
ssize_t tun_write(int tun_fd, uint8_t* packet, size_t packet_size)
{
    assert(packet_size > 0);
    if(packet == NULL) {
        myvpn_errno = MYVPN_E_PKT_NULL;
        return -1;
    }

    ssize_t written_bytes = write(tun_fd, packet, packet_size);
    if(written_bytes < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        return -1;
    }
    else if(written_bytes == 0) {
        myvpn_errno = MYVPN_E_PKT_ZERO_SIZE;
        return -1;
    }
    else if(written_bytes != packet_size) {
        // TODO: as of righ now, for debugging it returns succes and only prints warning. Maybe also return -1?
        MYVPN_LOG(MYVPN_LOG_WARNING, "wrote %d/%zu bytes of packet\n", written_bytes, packet_size);
    }
    return written_bytes;
}

// TODO in which file place this func?
// reads data from tun device, opened with 'tun_fd' to 'buf' with size 'buf_size' 
// On success returns number of bytes read, or error -1 and 'myvpn_errno' is set to indicate an error
ssize_t tun_read(int tun_fd, uint8_t* buf, size_t buf_size)
{
    assert(buf != NULL && buf_size > 0);
    ssize_t read_bytes = read(tun_fd, buf, buf_size);
    if(read_bytes == 0) {
        myvpn_errno = MYVPN_E_PKT_ZERO_SIZE;
        return -1;
    }
    else if(read_bytes < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        return -1;
    }
    return read_bytes;
}
