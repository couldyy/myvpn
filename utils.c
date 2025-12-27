#include "utils.h"



// TODO maybe it is better to move those 2 funtions to server.h
// TODO #2 change return type to 'hash_t' when updated 'cash_table.h' will be provided
// fnv1a hash
uint64_t hash_sockaddr(const void* key)
{
    assert(key != NULL);
    struct sockaddr_in* addr = (struct sockaddr_in*)key;
    //printf("hash %hu  %u\n", addr->sin_port, addr->sin_addr.s_addr);
    uint64_t h = 1469598103934665603ULL;
    for(int i = 0; i < sizeof(*addr); i++) {
        h ^= ((uint8_t*)addr)[i];
        h *= 1099511628211ULL;
    }
    //printf("hash %zu\n", h);
    return h;
}

int cmp_sockaddr(const void* a, const void* b)
{
    assert(a != NULL && b != NULL);
    struct sockaddr_in* a_addr = (struct sockaddr_in*) a;
    struct sockaddr_in* b_addr = (struct sockaddr_in*) b;
   // printf("cmp %p %p\n", a_addr, b_addr);
   // printf("%d:%d %hu:%hu %u:%u\n", a_addr->sin_family, b_addr->sin_family,  
   //     a_addr->sin_port, b_addr->sin_port,
   //     a_addr->sin_addr.s_addr, b_addr->sin_addr.s_addr);

    if((a_addr->sin_family == b_addr->sin_family) && 
        (a_addr->sin_port == b_addr->sin_port) &&
        (a_addr->sin_addr.s_addr == b_addr->sin_addr.s_addr)) {
        return 0;
    }
    else {
        return 1;   // since in HashTable code it only checks wether compare funce returns 0, we can return 
    }               // anything but 0 if addresses dont match 
                    
}

uint64_t hash_uint32_ptr(const void* key)
{
    uint64_t hash = (uint32_t)key;
    hash = ((hash >> 16) ^ hash) * 0x45d9f3bu;
    hash = ((hash >> 16) ^ hash) * 0x45d9f3bu;
    hash = (hash >> 16) ^ hash;
    return hash;
}
int cmp_uint32_ptr(const void* a, const void* b)
{
    assert(a != NULL && b != NULL);
    return !((uint32_t)a == (uint32_t)b);
}


// NOTE: parsed IP addresses are in NETWORK byte order ('version' field is single byte, so order doesnt matter)
// parses ip header (currently only ipv4 supported)
// on success returns 'Ip_data' structure with corresponding fileds filled with data
// on error also returns 'Ip_data' struct but values that could not be parsed are set to 0
Ip_data parse_ip_header(uint8_t* ip_packet, size_t packet_size)
{
    assert(ip_packet != NULL && packet_size > 0);
    uint8_t version = ip_packet[0] >> 4;

    uint32_t src_addr_v4 = 0;
    uint32_t dst_addr_v4 = 0;
    if(version == 4) {
        memcpy(&src_addr_v4, ip_packet+IPV4_SRC_ADDR_BIAS, sizeof(src_addr_v4)); 
        memcpy(&dst_addr_v4, ip_packet+IPV4_DST_ADDR_BIAS, sizeof(dst_addr_v4)); 
    }
    else if(version == 6) {
        // TODO parse ipv6 addresses
    }
    
    return (Ip_data) {.ip_version = version, .src_addr_v4 = src_addr_v4, .dst_addr_v4 = dst_addr_v4};
    
}

// 'port' should be in NETWORK BYTE ORDER
// on success returns address to 'sockaddr_in' structure
// on error NULL and 'myvpn_errno' is set to indicate an error
struct sockaddr_in* create_sockaddr_in(int af, uint16_t port, char* address)
{
    struct sockaddr_in* addr = malloc(sizeof(struct sockaddr_in));
    assert(addr != NULL && "malloc() failed");
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = af;
    addr->sin_port = port;
    int res = inet_pton(AF_INET, address, &(addr->sin_addr));
    if(res == 0) {
        myvpn_errno = MYVPN_E_INVALID_IP_ADDRESS;
        return NULL;
    }
    else if (res < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        return NULL;
    }
    return addr;
}

// TODO last byte is not printed
void _print_packet(uint8_t* packet, ssize_t size)
{
    assert(size > 0);
    char ascii_line[18] = {0};
    char byte_line[64] = {0};
    int written = 0;
    uint8_t byte;
    uint8_t display_line_len = 16;
    for(ssize_t i = 0, ascii_line_index = 0; i < size; i++, ascii_line_index++) {
        if((i % display_line_len == 0) || i == (size -1)) {
            ascii_line[17] = '\0';
            byte_line[display_line_len * 2 + (display_line_len / 2 -1)] = '\0';
            printf("%-*s %s\n", display_line_len * 2 + (display_line_len / 2 -1), byte_line, ascii_line);
            memset(ascii_line, 0, sizeof(ascii_line)); 
            memset(byte_line, 0, sizeof(byte_line)); 
            ascii_line_index = 0;
            written = 0;
        }
        byte = packet[i];
        written += snprintf(byte_line + written, sizeof(byte_line) - written, "%.2x%s", byte, (i % 2 == 0)  ? "" : " ");
        ascii_line[ascii_line_index] = (byte >= 33 && byte <= 126) ? (char)byte : '.'; 
    }
    printf("\n");
}


// Idea stolen from TCP standart
// Sum of all 16-bit words (header + payload)
// 'header' MUST be in HOST byte order
uint16_t calculate_checksum(Vpn_header* header, size_t header_size, uint8_t* payload, size_t payload_size)
{
    assert(header != NULL && header_size > 0);
    if(payload == NULL && payload_size > 0) {
        MYVPN_LOG(MYVPN_LOG_ERROR_VERBOSE, "Invalid payload while construsting vpn packet\n");
        abort();    // TODO dont abort on invalid payload
    }
    uint16_t sum = 0;
    for(int i = 0; i < header_size/2; i++) {
        sum += ((uint16_t*)header)[i];
    }
    for(int i = 0; i < payload_size/2; i++) {
        sum += ((uint16_t*)payload)[i];
    }
    return sum;
}


uint8_t* get_bytes(uint8_t* num, size_t size)
{
    uint8_t* bytes = malloc(size);
    for(int i = 0; i < size; i++) {
        bytes[i] = num[i];
    }
    return bytes;
}

char* get_bytes_str(uint8_t* num, size_t size) 
{
    uint8_t* bytes = get_bytes(num, size);
    size_t str_size = size*5;
    char* bytes_str = malloc(str_size);
    memset(bytes_str, 0, str_size);
    int written = 0;
    for(int i = 0; i < size; i++) {
        written += snprintf(bytes_str + written, str_size - written, "%.2x ", bytes[i]);
    }
    return bytes_str;
}

char* get_bytes_str_num(uint32_t num, size_t size)
{
    uint8_t* num_bytes = (uint8_t*) &num;
    return get_bytes_str(num_bytes, size);
}

int get_ipv4_numeric_addr(const char* addr, uint32_t* dst)
{
    assert(addr != NULL && dst != NULL);
    int res = inet_pton(AF_INET, addr, dst);
    if(res < 0) {
        myvpn_errno = MYVPN_E_USE_ERRNO;
        return -1;
    }
    else if(res == 0) {
        myvpn_errno = MYVPN_E_INVALID_IP_ADDRESS;
        return -1;
    }
    else {
        return 0;
    }
}
