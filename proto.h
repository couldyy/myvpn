#ifndef COMMON_H
#define COMMON_H

typedef uint8_t vpnflag_t;
typedef uint8_t vpnmsgtype_t;

#define MAGIC_NUM 0x909f    // randomly choosen number, will be protocol number
#define MTU 1500  //bytes

#define VPN_FLAG_SEQ  ((vpnflag_t)1)
#define VPN_FLAG_ACK  ((vpnflag_t)2)
#define VPN_FLAG_FIN  ((vpnflag_t)4)
#define VPN_FLAG_CMD  ((vpnflag_t)8)    
#define VPN_FLAG_DAT  ((vpnflag_t)16)



#define VPN_MSG_TYPE_CONNECT ((vpnmsgtype_t)1)
#define VPN_MSG_TYPE_CONNECT_REPLY ((vpnmsgtype_t)2)
#define VPN_MSG_TYPE_PING ((vpnmsgtype_t)3)
#define VPN_MSG_TYPE_PING_REPLY ((vpnmsgtype_t)4)    // recreate(will be used for recrating connection
#define VPN_MSG_TYPE_RECONNECT ((vpnmsgtype_t)5)
#define VPN_MSG_TYPE_ERROR ((vpnmsgtype_t)6)

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
  |Reserved  for  |               |                               | 
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
    uint32_t authentication;
} Vpn_header;

typedef struct {
    Vpn_header* header;
    uint8_t* payload;
    size_t payload_size;
} Vpn_packet;

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

Raw_packet construct_vpn_packet(Vpn_header header, uint8_t* payload, size_t payload_size);
Vpn_packet* encapsulate_data_packet(uint32_t authentication_num, uint8_t* raw_packet, size_t packet_size);

#endif //COMMON_H
