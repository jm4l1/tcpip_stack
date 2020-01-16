#include "utils.h"
#include <arpa/inet.h>
#include <sys/socket.h>

void
apply_mask(char* prefix ,char mask , char* str_prefix){
    unsigned int binary_prefix = 0;
    inet_pton(AF_INET , prefix , &binary_prefix);
    binary_prefix=htonl(binary_prefix);
    unsigned int bit_mask = 0xFFFFFFFF  << ( 32 - mask ) ;
    binary_prefix &= bit_mask;
    binary_prefix=htonl(binary_prefix);
    inet_ntop(AF_INET, &binary_prefix, str_prefix, INET_ADDRSTRLEN );
    str_prefix[15] = '\0';
}

void
layer2_fill_with_broadcast_mac(char *mac_array){
    for(int idx = 0 ; idx < 6 ; idx++)
        mac_array[idx] = 0xFF;
};