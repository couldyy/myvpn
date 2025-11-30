#ifndef SERVER_H
#define SERVER_H
#include <stdint.h>

typedef struct {
   uint32_t real_ip; 
   char[15] real_ip_char; // TODO maybe dont need
   uint16_t port;

   uint32_t tun_ip; 
   char[15] tun_ip_char; // TODO maybe dont need
} User;

#endif //SERVER_H
