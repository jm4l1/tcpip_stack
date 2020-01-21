#ifndef __LAYER2_H__
#define __LAYER2_H__

#include "../net.h"
#include "../graph.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_ETH_PAYLOAD 1500
#define ETH_HDR_SIZE_EXCL_PAYLOAD ( sizeof(ethernet_frame_t) - sizeof((( ethernet_frame_t*)0)->payload ) - 4 )
#define ETH_FCS(eth_hdr_ptr, payload_size)  ((unsigned int *)(((char *)(((ethernet_frame_t *)eth_hdr_ptr)->payload) + payload_size)))


#pragma pack(push,1)
typedef struct ethernet_frame {
    #if 0
        //                  Ethernet Frame 
        6 bytes            6 bytes    2 bytes      46 - 1500 bytes     4 bytes
        +========================================================================+
        |    dest_mac    |    src_mac   | length |        payload      |  CRC    |
        +========================================================================+
    #endif

    mac_addr_t dest_mac;
    mac_addr_t src_mac;
    uint16_t length;
    char payload[248];
    uint32_t FCS;
} ethernet_frame_t;
#pragma pack(pop)


static inline ethernet_frame_t *
ALLOC_ETH_HDR_WITH_PAYLOAD(char *pkt, unsigned int pkt_size){
    ethernet_frame_t* header = (ethernet_frame_t *)(pkt - ETH_HDR_SIZE_EXCL_PAYLOAD);
    uint32_t* FCS = ETH_FCS(header , pkt_size);
    memset((char*)header, 0 , ETH_HDR_SIZE_EXCL_PAYLOAD);
    memset((uint32_t*)FCS,0,sizeof(uint32_t));
    return header;
}

static inline bool_t
l2_frame_recv_qualify_on_interface(interface_t* intf , ethernet_frame_t* eth_frame){
    unsigned char* dest_mac = eth_frame->dest_mac.mac;
    if(!IS_INTF_L3_MODE(intf)) return FALSE;
    if(memcmp(IF_MAC(intf) ,dest_mac , sizeof((mac_addr_t*)0)->mac) == 0)   return TRUE;
    if(IS_MAC_BROADCAST_ADDR(dest_mac)) return TRUE;
    return FALSE;
}

char* pkt_buffer_shift_right( char* pkt , unsigned int pkt_size , unsigned int total_buffer_size);
 
#endif