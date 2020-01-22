#ifndef __LAYER2_H__
#define __LAYER2_H__

#include "../net.h"
#include "../graph.h"
#include "../tcpconst.h"

#include <stdlib.h>
#include <stdio.h>

#define MAX_ETH_PAYLOAD 1500
#define ETH_HDR_SIZE_EXCL_PAYLOAD ( sizeof(ethernet_frame_t) - sizeof((( ethernet_frame_t*)0)->payload ) - 4 )
#define ETH_FCS(eth_hdr_ptr, payload_size)  ((unsigned int *)(((char *)(((ethernet_frame_t *)eth_hdr_ptr)->payload) + payload_size)))
#define ARP_PACKET_SIZE sizeof(arp_packet_t)
#pragma pack(push,1)
typedef struct arp_packet{
    #if 0
        //                  Ethernet Frame 
        6 bytes            6 bytes    2 bytes      46 - 1500 bytes     4 bytes
        +========================================================================+
        |    dest_mac    |    src_mac   | 0x806  |        payload      |  CRC    |
        +========================================================================+
                                                /                       \
                                               /                         \
                                              /                           \
                                             /                             \
                                            /                               \
        <----------------------------------              payload             --------------------------------------------------->
        +=======================================================================================================================+
        | hw_type | proto_type | hw_addr_len | proto_addr_len | op_code |    src_mac    |  src_ip  |    dest_mac  |    dst_ip   |
        +=======================================================================================================================+
          2 bytes    2 bytes       1 bytes       1 byte         2 bytes      6 bytes      4 bytes        6 bytes      4 bytes
    #endif

    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_addr_len;
    uint8_t proto_addr_len;
    uint16_t op_code;
    mac_addr_t src_mac;
    uint32_t src_ip;
    mac_addr_t dst_mac;
    uint32_t dst_ip;
} arp_packet_t;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct ethernet_frame {
    #if 0
        //                  Ethernet Frame 
        6 bytes            6 bytes    2 bytes      46 - 1500 bytes     4 bytes
        +========================================================================+
        |    dest_mac    |    src_mac   | type |        payload      |  CRC    |
        +========================================================================+
    #endif

    mac_addr_t dest_mac;
    mac_addr_t src_mac;
    uint16_t type;
    char payload[248];
    uint32_t FCS;
} ethernet_frame_t;
#pragma pack(pop)

typedef struct arp_table_{
    glthread_t arp_entries;
}arp_table_t;
typedef struct arp_entry_ {
    ip_add_t ip_addr;
    mac_addr_t mac_addr;
    char oif_name[IF_NAME_SIZE];
    glthread_t arp_glue;
} arp_entry_t;
GLTHREAD_TO_STRUCT(arp_glue_to_arp_entry , arp_entry_t , arp_glue);

// ARP Table
void init_arp_table(arp_table_t **arp_table);
bool_t arp_table_entry_add(arp_table_t *arp_table , arp_entry_t *arp_entry );
arp_entry_t * arp_table_lookup(arp_table_t *arp_table, char *ip_addr);
void arp_table_update_from_arp_reply(arp_table_t *arp_table_t , arp_packet_t *arp_packet , interface_t *iif);
void arp_table_delete_entry(arp_table_t *arp_table, char *ip_add);
void arp_table_dump( arp_table_t* arp_table);

void send_arp_broadcast_rquest(node_t *node , interface_t *oif , char *ip_addr);

static inline ethernet_frame_t *
ALLOC_ETH_HDR_WITH_PAYLOAD(char *pkt, unsigned int pkt_size){
    ethernet_frame_t* header = (ethernet_frame_t *)(pkt - ETH_HDR_SIZE_EXCL_PAYLOAD);
    uint32_t* FCS = ETH_FCS(header , pkt_size);
    memset((char*)header, 0 , ETH_HDR_SIZE_EXCL_PAYLOAD);
    memset((uint32_t*)FCS,0,sizeof(uint32_t));
    return header;
}

void
layer2_frame_recv(node_t* node , interface_t *intf, char *pkt , uint32_t pkt_size);

static inline bool_t
l2_frame_recv_qualify_on_interface(interface_t* intf , ethernet_frame_t* eth_frame){
    unsigned char* dest_mac = eth_frame->dest_mac.mac;
    if(!IS_INTF_L3_MODE(intf)) {
        printf("IP address not configured on IF : %s\n" , intf->if_name);
        return FALSE;
    }
    if(memcmp(IF_MAC(intf) ,dest_mac , sizeof((mac_addr_t*)0)->mac) == 0){
        printf("Frame with MAC of %s\n" , intf->if_name);
        return TRUE;
    }
    if(IS_MAC_BROADCAST_ADDR(dest_mac)){
        printf("Broadcast Frame received %x:%x:%x:%x:%x:%x \n"  , eth_frame->src_mac.mac[0], eth_frame->src_mac.mac[1] , eth_frame->src_mac.mac[2] , eth_frame->src_mac.mac[3] , eth_frame->src_mac.mac[4] , eth_frame->src_mac.mac[5]);
        return TRUE;
    }
    printf("Dropping out of qualify function %x:%x:%x:%x:%x:%x \n"  , eth_frame->src_mac.mac[0], eth_frame->src_mac.mac[1] , eth_frame->src_mac.mac[2] , eth_frame->src_mac.mac[3] , eth_frame->src_mac.mac[4] , eth_frame->src_mac.mac[5]);
    return FALSE;
}

char* pkt_buffer_shift_right( char* pkt , unsigned int pkt_size , unsigned int total_buffer_size);
 
#endif