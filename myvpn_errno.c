#include "myvpn_errno.h"

myvpn_errno_t myvpn_errno;

char* myvpn_strerror(myvpn_errno_t myvpn_errno)
{
    switch (myvpn_errno) {
        case MYVPN_E_PSIZE_BIGGER_MTU:
            return "Packet size is bigger than MTU";
        case MYVPN_E_AFNOSUPPORT:
            return "Unsupported address family";
        case MYVPN_E_IP_V_NOSUPPORT:
            return "Unsupported IP version (currently supported: IPv4)";
        case MYVPN_E_PKT_ZERO_SIZE:
            return "Packet with size 0";
        case MYVPN_E_FAIL_SEND_CON:
            return "Failed to send connection packet";
        case MYVPN_E_UNINITIALIZED_CON_DATA:
            return "Uninitialized connection data";
        case MYVPN_E_INCOMPLETE_PACKET:
            return "Incomplete packet";
        case MYVPN_E_PKTS_SIZE_DONT_MATCH:
            return "Packets size dont match";
        case MYVPN_E_INVALID_PAYLOAD:
            return "Invalid payload";
        case MYVPN_E_INVALID_IP_ADDRESS:
            return "Invalid IP address";
        case MYVPN_E_INVALID_CHECKSUM:
            return "Invalid checksum";
        case MYVPN_E_NOT_MYVPN_PACKET:
            return "Not MyVPN packet";
        case MYVPN_E_UNEXPECTED_MSG_TYPE:
            return "Unexpected 'msg_type'";
        case MYVPN_E_INVALID_AUTH_NUM:
            return "Invalid authentication number";
        case MYVPN_E_PKT_NULL:
            return "Packet pointer is NULL";
        case MYVPN_E_ADDR_NULL:
            return "'sockaddr_in' pointer is NULL";
        case MYVPN_E_ADDR_IS_FREE:
            return "Address is free";
        case MYVPN_E_CLIENT_NOT_FOUND:
            return "Client with such address was not found in client table";
        case MYVPN_E_ADDR_POOL_OVERFLOW:
            return "Address pool overflow";
        case MYVPN_E_INVALID_POOLSIZE:
            return "Invalid pool size";
        case MYVPN_E_RECVPKT_NOT_FROM_TARGET:
            return "Received packet is NOT from specifed target. Packet was droped\n";
        case MYVPN_EM_INTERNAL_SERVER_ERR:
            return "Internal server error";
        case MYVPN_E_USE_ERRNO:
            return strerror(errno);
        default:
            return "[myvpn_errno ERROR]: got invaid 'myvpn_errno' number";
    }
}
