#ifndef COMMON_H
#define COMMON_H

typedef uint8_t vpnflag_t
typedef uint8_t vpncmd_t

typedef struct {
    int addr;
    int mask;
    //int port;
} Net_info; 



#define VPN_FLAG_SYN  ((vpnflag_t)1)
#define VPN_FLAG_ACK  ((vpnflag_t)2)
#define VPN_FLAG_FIN  ((vpnflag_t)4)
#define VPN_FLAG_CMD  ((vpnflag_t)8)    
#define VPN_FLAG_DAT  ((vpnflag_t)16)



#define VPN_CMD_KEEPALIVE ((vpncmd_t)1)
#define VPN_CMD_CONNECT ((vpncmd_t)2)
#define VPN_CMD_REQUEST_NEW_IP ((vpncmd_t)3)
#define VPN_MSG_RCT ((vpncmd_t)4),    // recreate(will be used for recrating connection

/*
    MyVPN packet structure:

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |         magic number          |         Packet size           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |    Flags      |    CMD        |                               | 
  |S A F C D      |K C R R        |                               | 
  |E C I M A      |P O Q C        |        Header checksum        |
  |Q K N D T      |L N I T        |                               |
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

typedef struct Vpn_packet {
    uint8_t data*;
    size_t data_size;
};

typedef struct Vpn_header {
    uint16_t magic;
    uint16_t packet_size;
    vpnflag_t flags;
    vpncmd_t command;
    uint16_t checksum; 
    uint32_t syn_num;
    uint32_t ack_num;
    uint32_t authentication;
};

// Each field in network byte order (Big-endian)
typedef struct Connection {
    int32_t src_ip;
    int16_t src_port;

    int32_t dst_ip;
    int16_t dst_port;

    uint32_t src_seq_num;
    uint32_t src_ack_num;
};

Vpn_packet construct_vpn_packet(Vpn_header header, uint8_t* payload, size_t payload_size);
#endif //COMMON_H
