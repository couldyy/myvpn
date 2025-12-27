#ifndef MYVPN_ERRNO_H
#define MYVPN_ERRNO_H

#include <string.h>
#include <errno.h>


// prefix _E_  - local error, used to indicate localy what error hppened
// prefix _EM_ - _ErrorMessage_, error code for sendnig over net to other side as reply
/* NOTE that some of _E_ also can be used to send error messages:
        - MYVPN_E_INVALID_AUTH_NUM, 
*/
typedef enum {
    MYVPN_E_PSIZE_BIGGER_MTU = 1,
    MYVPN_E_AFNOSUPPORT,
    MYVPN_E_IP_V_NOSUPPORT,
    MYVPN_E_PKT_ZERO_SIZE,
    MYVPN_E_FAIL_SEND_CON,
    MYVPN_E_UNINITIALIZED_CON_DATA,
    MYVPN_E_INCOMPLETE_PACKET,
    MYVPN_E_PKTS_SIZE_DONT_MATCH,
    MYVPN_E_INVALID_PAYLOAD,
    MYVPN_E_INVALID_IP_ADDRESS,
    MYVPN_E_INVALID_CHECKSUM,
    MYVPN_E_NOT_MYVPN_PACKET,
    MYVPN_E_UNEXPECTED_MSG_TYPE,
    MYVPN_E_INVALID_AUTH_NUM,
    MYVPN_E_PKT_NULL,
    MYVPN_E_ADDR_NULL,
    MYVPN_E_CLIENT_NOT_FOUND,
    MYVPN_E_ADDR_POOL_OVERFLOW,
    MYVPN_E_INVALID_POOLSIZE,
    MYVPN_E_RECVPKT_NOT_FROM_TARGET,
    MYVPN_EM_INTERNAL_SERVER_ERR,
    MYVPN_E_USE_ERRNO,
} myvpn_errno_t;

char* myvpn_strerror(myvpn_errno_t myvpn_errno);

extern myvpn_errno_t myvpn_errno;
#endif //MYVPN_ERRNO_H
