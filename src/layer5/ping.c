#include <stdio.h>
#include "graph.h"
#include "layer3.h"
#include <arpa/inet.h>


void
send_ping_request(node_t *node , char *ip, uint8_t count){
    uint32_t dest_ip;
    inet_pton(AF_INET , ip , &dest_ip);
    printf("[send_ping_request] Sending %d packets to %s\n" , count , ip); 
    for(uint8_t i ; i < count ; i++){
        send_icmp_echo_message(node , i , i , dest_ip);
    }
}