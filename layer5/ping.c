#include <stdio.h>
#include "../graph.h"
#include "../layer3/layer3.h"

#pragma pack(push , 1)
struct echo_packet_ {
    icmp_hdr_t hdr;
    uint8_t identifier;
    uint8_t sequence_number;
    char *data;
}echo_packet_t;
#pragma pack(pop)

void
send_ping_request(node_t *node , char *ip, uint8_t count){
    printf("Ping handler\n");
    printf("Sending %d packets to %s\n" , count , ip);
}