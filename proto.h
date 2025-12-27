#ifndef PROTO_H
#define PROTO_H
#include <limits.h>
#include <stdint.h>
#include <unistd.h>

#include "myvpn_log.h"
#include "myvpn_errno.h"

#define MYVPN_DEBUG_PROTO



// TODO#3 make structures 'Connection'(client side) and 'Client_connection'(server_side) consistend
// maybe use some sort of polymorphysm (just a common struct in them)

typedef uint8_t vpnflag_t;
typedef uint8_t vpnmsgtype_t;

#define MAGIC_NUM 0x909f    // randomly choosen number, will be protocol number
// TODO set MTU for tun interface and let user provide it with flags
#define MTU 1500  //bytes

#define PACKET_BUFFER_SIZE 8192 // TODO maybe set to max udp size (65kb)?

//#define VPN_FLAG_SEQ  ((vpnflag_t)1)
//#define VPN_FLAG_ACK  ((vpnflag_t)2)
//#define VPN_FLAG_FIN  ((vpnflag_t)4)
//#define VPN_FLAG_RST  ((vpnflag_t)8)    

#define TODO_AUTH_NUM 1
#define TODO_SEQ_NUM 0
#define TODO_ACK_NUM 0

#define MYVPN_MSG_TYPE_CONNECT ((vpnmsgtype_t)1)
#define MYVPN_MSG_TYPE_CONNECT_REPLY ((vpnmsgtype_t)2)
#define MYVPN_MSG_TYPE_CONNECT_ESTAB ((vpnmsgtype_t)3)
#define MYVPN_MSG_TYPE_CONNECT_FIN ((vpnmsgtype_t)4)
#define MYVPN_MSG_TYPE_PING ((vpnmsgtype_t)5)
#define MYVPN_MSG_TYPE_PING_REPLY ((vpnmsgtype_t)6)    
#define MYVPN_MSG_TYPE_RECONNECT_SOFT ((vpnmsgtype_t)7)    // reconnect on SSL layer, tun ip stays same
#define MYVPN_MSG_TYPE_RECONNECT_HARD ((vpnmsgtype_t)8)    // reset and init full new connection(ssl+myvpn) 
#define MYVPN_MSG_TYPE_DATA ((vpnmsgtype_t)9)
#define MYVPN_MSG_TYPE_ERROR ((vpnmsgtype_t)10)
#define MYVPN_MSG_TYPE_SERVICE ((vpnmsgtype_t)11)     // for acknowlegement, retransmission, resetting etc

//typedef enum {
//    // ...
//} Vpn_error;

/*
    MyVPN packet structure:

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |         magic number          |         Packet size           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |    Flags      |    MSG Type   |                               | 
  | Reserved for  |               |                               | 
  | future usage  |               |        Header checksum        |
  |               |               |                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |                  Synchronization number                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |                   Acknowlegement number                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |                 Authentication(Client ID)                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |                         Payload                               |
  |            (encapsulated packet or CMD data)                  |
  |                          ...                                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  

  Total size: 20 bytes (160 bits) (can probably remove 'checksum' 'magic' and 'packet size')

  RULES:
    - all data in `Vpn_header` struct MUST be stored in NETWORK BYTE ORDER

  Notes:
  - `magic number` is either a made up protocol number or (proto number XOR Client ID) didnt really choose now
  - since header size is static as of right now (can be changed to dyamic in future) i think it is a good idea to include
    size of whole packet, not just header
  - checksum is just a sum of all fields in header
  - Only 1 CMD can be sent at a time, while multiple flags can be sent at a time
  
  Abbreviations:
    CMDs:
      - KPL - Keep alive (ping message)
      - CON - Connect
      - RQI - Request new tun IP
      - RCT - Recreate connection

    Flags:
      - SEQ - Sequence number field is significant
      - ACK - Acknowlegement number field is significant 
      - FIN - Finish
      - CMD - Command
      - DAT - Data
*/

/* 
    Client -> Server connection
    
Client   |                                                                                               | Server
         | -------[magic|size|flags| CON_REQ |checksum|(todo)seq_num|0(ack)|0(auth)|no payload]--->>>    | 
         |                                                                                               |
writes   |<<<-[m|sz|flg|CON_REPL|cksum|(todo)seq_num|(todo)ack|client_auth_num| ( server tun IP )]--     | 
serv auth|                                                                      ( client tun IP )        |
and con  |                                                                      ( tun net mask  )        |
data     |                                                                      ( client auth num)       |
         |                                                                                               |
         |   -------[m|se|flg|CON_ESTAB|cksum|(todo)seq_num|(todo)ack|auth_num|no payload]--->>>         | 
         |   <<<----[m|se|flg|CON_ESTAB|cksum|(todo)seq_num|(todo)ack|auth_num|no payload]------         | 
         |                                                                                               |

At any point either a client or server may send following packet:
         |   [m|se|flg|MSG_ERR|cksum|seq_num|ack|auth_num| ERROR_NUMBER ]        | 
*/


typedef struct {
    uint8_t *data;
    size_t data_size;
} Raw_packet;

typedef struct {
    uint16_t magic;
    uint16_t packet_size;
    vpnflag_t flags;
    vpnmsgtype_t msg_type;
    uint16_t checksum; 
    uint32_t seq_num;
    uint32_t ack_num;
    uint32_t authentication_num;
} Vpn_header;

typedef struct {
    Vpn_header* header;
    uint8_t* payload;
    size_t payload_size;
} Vpn_packet;

typedef enum {
    CON_NOT_CONNECTED = 0,
    CON_REQUEST,
    CON_UNVERIFIED,
    CON_ESTAB,
    CON_LOST,
} Con_state;

typedef struct {
    // NETWORK byte order
    uint32_t tun_addr;  
    uint32_t authentication_num;
    //
    // HOST byte order
    uint32_t seq_num;
    uint32_t ack_num;

    Con_state connection_state;
    //
} Connection;

#define HEADER_TO_NETWORK_BO(header_ptr) \
    do { \
        (header)->magic = htons((header)->magic); \
        (header)->packet_size = htons((header)->packet_size); \
        /* vpnflag_t flags;    Since it is 1 bit in since, no need to convert */ \
        /* vpnmsgtype_t msg_type;    Since it is 1 bit in since, no need to convert */ \
        (header)->checksum  = htons((header)->checksum ); \
        (header)->seq_num = htonl((header)->seq_num); \
        (header)->ack_num = htonl((header)->ack_num); \
        /* authentication_num is stored in NETWORK byte order on both sides */ \
    } while(0)

#define HEADER_TO_HOST_BO(header_ptr) \
    do { \
        (header)->magic = ntohs((header)->magic); \
        (header)->packet_size = ntohs((header)->packet_size); \
        /* vpnflag_t flags;    Since it is 1 bit in since, no need to convert */ \
        /* vpnmsgtype_t msg_type;    Since it is 1 bit in since, no need to convert */ \
        (header)->checksum  = ntohs((header)->checksum ); \
        (header)->seq_num = ntohl((header)->seq_num); \
        (header)->ack_num = ntohl((header)->ack_num); \
        /* authentication_num is stored in NETWORK byte order on both sides */ \
    } while(0)

#define PACKET_FROM_TARGET(__packet_src_sockaddr_ptr, __target_sockaddr_ptr) \
    ((__packet_src_sockaddr_ptr)->sin_family == (__target_sockaddr_ptr)->sin_family && \
    (__packet_src_sockaddr_ptr)->sin_port == (__target_sockaddr_ptr)->sin_port && \
    (__packet_src_sockaddr_ptr)->sin_addr.s_addr == (__target_sockaddr_ptr)->sin_addr.s_addr) 

//// Each field in network byte order (Big-endian)
//typedef struct Connection {
//    int server_socket;
//    struct sockaddr_in* real_src;
//    struct sockaddr_in* tun_src;
//
//    struct sockaddr_in* real_dst;
//    struct sockaddr_in* tun_dst;
//
//    uint32_t src_seq_num;
//    uint32_t src_ack_num;
//    uint32_t authentication;    // on both sides(client and server - client's authentication num)
//};

Raw_packet* encapsulate_data_packet(uint32_t authentication_num, uint8_t* raw_packet, size_t packet_size);

// TODO maybe it is better to leave other functions to call HEADER_TO_NETWORK_BO(), so to provide more flexibility
//      and not to confuse, with passing header as HOST_BO and payload as NETWORK_BO
// 'header' MUST be in HOST byte order, 'construct_vpn_packet()' will call HEADER_TO_NETWORK_BO() macro
Raw_packet* construct_vpn_packet(Vpn_header *header, uint8_t* payload, size_t payload_size);

Raw_packet* construct_error_packet(uint32_t authentication_num, uint32_t seq_num, uint32_t ack_num, myvpn_errno_t error_code);



Vpn_packet* parse_packet(uint8_t* raw_packet, size_t packet_size);

//ssize_t send_vpn_packet_sockconn(int sender_socket, Raw_packet* packet, int flags);
ssize_t send_vpn_packet(int sender_socket, Raw_packet* packet, int flags, struct sockaddr_in* dst_addr);

//Vpn_packet* recv_vpn_packet_sockconn(int receiver_socket, uint8_t* buf, size_t size, int flags);
Vpn_packet* recv_vpn_packet(int receiver_socket, uint8_t* buf, size_t size, int flags, struct sockaddr_in* src_addr, socklen_t* addrlen, struct sockaddr_in* recv_target);

// TODO in which file place this func?
// write 'packet' of size 'packet_size, to tun device opened at 'tun_fd'.
// On success returns number of bytes written, or error -1 and 'myvpn_errno' is set to indicate an error
ssize_t tun_write(int tun_fd, uint8_t* packet, size_t packet_size);

// TODO in which file place this func?
// reads data from tun device, opened with 'tun_fd' to 'buf' with size 'buf_size' 
// On success returns number of bytes read, or error -1 and 'myvpn_errno' is set to indicate an error
ssize_t tun_read(int tun_fd, uint8_t* buf, size_t buf_size);
#endif //PROTO_H
