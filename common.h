#ifndef COMMON_H
#define COMMON_H

typedef struct {
    int addr;
    int mask;
    //int port;
} Net_info; 

// | Msg_type | Msg_flag(if type==service) | Msg_cmd(if msg_flag==CMD) |
typedef enum {
    VPN_MSG_SERVICE = 1,
    VPN_MSG_DATA = 2,
} Msg_type;

typedef enum {
    VPN_MSG_SYN = 1,
    VPN_MSG_ACK = 2,
    VPN_MSG_FIN = 4,
    VPN_MSG_RCT = 8,    // recreate(will be used for recrating connection)
    VPN_MSG_CMD = 16,    
} Msg_flag;



typedef enum {
    VPN_CMD_KEEPALIVE = 1,
    VPN_CMD_CONNECT = 2,
    VPN_CMD_REQUEST_NEW_IP = 4,
} Msg_cmd;

/*
    UDP packet structure:

  +------------+--------------------------------+ 
  | UDP Header | Payload                        |
  +------------|--------------------------------+        
  |            | Msg_type | MSG Size |  Payload |
  |            |   4 bits |  16 bits |          | 
  +------------+--------------------------------+ 


  Different cases 'Msg_type':

  +------------+--------------------------------+ 
  | UDP Header | Payload                        |
  +------------|--------------------------------+        
  |            | Msg_type | MSG Size |  Payload |
  |            | MSG_CON  |  16 bits |          | 
  +------------+--------------------------------+ 

  +------------+--------------------------------------------------+ 
  | UDP Header |               Payload                            |
  +------------|--------------------------------------------------+                  
  |            | Msg_type  | MSG Size |  Payload                  |
  |            |           |          |                           |
  |            | MSG_REPL  |  16 bits | server_tun_ip             | 
  |            |           |          | server_tun_mask           |
  |            |           |          | assigned_clinent_tun_ip   |
  +------------+--------------------------------------------------+ 

  +------------+-----------------------------------------+ 
  | UDP Header | Payload                                 |
  +------------|-----------------------------------------+        
  |            | Msg_type | MSG Size |  Payload          |
  |            | MSG_MSG  |  16 bits |  Originial packet | 
  +------------+-----------------------------------------+ 

  +------------+-----------------------------------------+ 
  | UDP Header | Payload                                 |
  +------------|-----------------------------------------+        
  |            | Msg_type | MSG Size |  Payload          |
  |            | MSG_MSG  |  16 bits |  Originial packet | 
  +------------+-----------------------------------------+ 

*/

#endif //COMMON_H
