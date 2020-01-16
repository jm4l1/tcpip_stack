#include <stdio.h>
#include <string.h>
#include <stdlib.h>  // for strtol
#include <arpa/inet.h>
#include <sys/socket.h>


#define UNSET_BIT(n, pos)       (n = n & ((1 << pos) ^ 0xFFFFFFFF))
#define SET_BIT(n, pos)     (n = n | 1 << pos)
#define BITMASK(mask)   (0xFFFFFFFF  << ( 32 - mask ))
#define IS_MAC_BROADCAST_ADDR(mac)  (mac_array[0] == 0xFF  &&  mac_array[1] == 0xFF && mac_array[2] == 0xFF && mac_array[3] == 0xFF  &&  mac_array[4] == 0xFF && mac_array[5] == 0xFF)

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

unsigned int
convert_ip_from_str_to_int(char *ip_addr){
    unsigned int binary_prefix = 0;
    inet_pton(AF_INET , ip_addr , &binary_prefix);
    return binary_prefix;
};

void
convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer){
    inet_ntop(AF_INET, &ip_addr, output_buffer, INET_ADDRSTRLEN );
};



int main(int argc , char** argv){
    char str_prefix[16] ;
    char mac_array[6];
    memset(str_prefix , 0 , 16);
    apply_mask(argv[1] , atoi(argv[2]) , str_prefix);
    printf("Mask %s applied to prefix %s gives prefix %s\n" , argv[2] , argv[1] , str_prefix);
    layer2_fill_with_broadcast_mac(mac_array);
    printf("Mac Broadcast Array is %u \n" , mac_array);
    if(IS_MAC_BROADCAST_ADDR(mac_array))
        printf("Is a broadcast mac");
    unsigned int ip_as_int = convert_ip_from_str_to_int(argv[1]);
    printf("%s as number : %d\n" , argv[1] , ip_as_int);
    char str_pref2[16];
    convert_ip_from_int_to_str(ip_as_int , str_pref2);
    printf("revered : %s\n" , str_pref2);
    return 0;
}