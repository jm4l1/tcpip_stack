#ifndef __LAYER2_H__
#define __LAYER2_H__

#include "../net.h"
#include "../graph.h"
#include "../tcpconst.h"

#include <stdlib.h>
#include <stdio.h>

#define MAX_ETH_PAYLOAD 1500
#define ETH_HDR_SIZE_EXCL_PAYLOAD ( sizeof(ethernet_frame_t) - sizeof((( ethernet_frame_t*)0)->payload ) - 4 )
#define VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD ( sizeof(vlan_tagged_ethernet_frame_t) - sizeof((( vlan_tagged_ethernet_frame_t*)0)->payload ) - 4 )
#define ETH_FCS(eth_frame, payload_size)  ((unsigned int *)(((char *)(((ethernet_frame_t *)eth_frame)->payload) + payload_size)))
#define VLAN_ETH_FCS(vlan_tagged_eth_frame, payload_size)  ((unsigned int *)(((char *)(((vlan_tagged_ethernet_frame_t *)vlan_tagged_eth_frame)->payload) + payload_size)))
#define ARP_PACKET_SIZE sizeof(arp_packet_t)
#define GET_COMMON_ETH_FCS(eth_frame, payload_size) ( is_pkt_vlan_tagged(eth_frame) ? VLAN_ETH_FCS( eth_frame , payload_size) : ETH_FCS(eth_frame , payload_size) )
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

#pragma pack(push,1)
typedef struct vlan_8021q_tag_ {
    #if 0
                            //                  Ethernet Frame 
        6 bytes            6 bytes                  2 bytes      46 - 1500 bytes     4 bytes
        +==================================================================================+
        |    dest_mac    |    src_mac   | 802.1q Tag | type |       payload      |  CRC    |
        +==================================================================================+
                                       /                     \
                                      /       802.1 tag       \
                                     /                         \
                                2 bytes        3 bits 1 bit      12 bits      
                    +========================================================+
                    |          TPID          |  PRI  | CFI |     VLAN ID     |
                    +========================================================+
    #endif

    uint16_t tpid;
    uint16_t tci_pri : 3 ;
    uint16_t tci_dei : 1;
    uint16_t tci_vid : 12;
} vlan_8021q_tag_t;
#pragma pack(pop)



#pragma pack(push,1)
typedef struct vlan_tagged_ethernet_frame {
    #if 0
        //                  Ethernet Frame 
        6 bytes            6 bytes                  2 bytes      46 - 1500 bytes     4 bytes
        +==================================================================================+
        |    dest_mac    |    src_mac   | 802.1q Tag | type |       payload      |  CRC    |
        +==================================================================================+

    #endif

    mac_addr_t dest_mac;
    mac_addr_t src_mac;
    vlan_8021q_tag_t vlan_8021q_tag;
    uint16_t type;
    char payload[248];
    uint32_t FCS;
} vlan_tagged_ethernet_frame_t;
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

static inline vlan_tagged_ethernet_frame_t *
is_pkt_vlan_tagged(ethernet_frame_t *eth_frame){
    if(eth_frame->type == VLAN_TAG)
        return (vlan_tagged_ethernet_frame_t *)eth_frame;
    return NULL;
}

static inline unsigned int
get_8021q_vlan_id( vlan_8021q_tag_t *vlan_8021q_tag){
    return vlan_8021q_tag->tci_vid;
}

static inline char *
get_ethernet_frame_payload(ethernet_frame_t *eth_frame){
    if(is_pkt_vlan_tagged(eth_frame)){
        return (char *)( ( (vlan_tagged_ethernet_frame_t *) eth_frame ) ->payload );
    }
    return (char *) (eth_frame -> payload) ;
};

static inline void
set_common_eth_fcs(ethernet_frame_t *eth_frame , uint32_t payload_size , uint32_t new_fcs){
    uint32_t *fcs = GET_COMMON_ETH_FCS(eth_frame , payload_size);
    fcs = &new_fcs;
}
static inline unsigned int
get_eth_hdr_size_excl_payload(ethernet_frame_t* eth_frame){
    vlan_tagged_ethernet_frame_t *vlan_eth_frame = is_pkt_vlan_tagged(eth_frame);
    if(vlan_eth_frame){
        return VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD;
    }
    return ETH_HDR_SIZE_EXCL_PAYLOAD;
}


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


// bool_t
// is_tagged_arp_broadcast_request_msg(ethernet_frame_t *ethernet_frame){
//     vlan_tagged_ethernet_frame_t* vlan_eth_frame = is_pkt_vlan_tagged(ethernet_frame);
    
//     if(!vlan_eth_frame) return FALSE;
    
//     uint16_t vid = get_8021q_vlan_id(&vlan_eth_frame->vlan_8021q_tag);
//     if((vid < 10) || (vid > 20)) return FALSE;
    
//     if(vlan_eth_frame->type != ARP_PACKET) return FALSE;

//     arp_packet_t* arp_packet = (arp_packet_t *) get_ethernet_frame_payload(vlan_eth_frame);

//     if(arp_packet->op_code != ARP_REQUEST) return FALSE;

//     return TRUE;

// }



void
layer2_frame_recv(node_t* node , interface_t *intf, char *pkt , uint32_t pkt_size);

static inline bool_t
l2_frame_recv_qualify_on_interface(interface_t* intf , ethernet_frame_t* eth_frame){
    unsigned char* dest_mac = eth_frame->dest_mac.mac;
    if(!IS_INTF_L3_MODE(intf) && IF_L2_MODE(intf) == L2_MODE_UNKNOWN) {
        if(intf->att_node->debug_status == DEBUG_ON) printf("Info : %s - IP address not configured on IF : %s\n" , intf->att_node->node_name , intf->if_name);
        return FALSE;
    }
    if(memcmp(IF_MAC(intf) ,dest_mac , sizeof((mac_addr_t*)0)->mac) == 0){
        if(intf->att_node->debug_status == DEBUG_ON) printf("Info : %s - Frame with MAC of %s\n" , intf->att_node->node_name , intf->if_name);
        return TRUE;
    }
    if(IS_MAC_BROADCAST_ADDR(dest_mac)){
        if(intf->att_node->debug_status == DEBUG_ON) printf("Info : %s - Broadcast Frame received %x:%x:%x:%x:%x:%x \n" , intf->att_node->node_name  , eth_frame->src_mac.mac[0], eth_frame->src_mac.mac[1] , eth_frame->src_mac.mac[2] , eth_frame->src_mac.mac[3] , eth_frame->src_mac.mac[4] , eth_frame->src_mac.mac[5]);
        return TRUE;
    }
    if(IF_L2_MODE(intf) != L2_MODE_UNKNOWN){
        if(intf->att_node->debug_status == DEBUG_ON) printf("Info : %s - Interface in operating in L2 mode\n" , intf->att_node->node_name);
        return TRUE;
    }
    if(intf->att_node->debug_status == DEBUG_ON) printf("Info : %s - Dropping out of qualify function %x:%x:%x:%x:%x:%x \n" , intf->att_node->node_name  , eth_frame->src_mac.mac[0], eth_frame->src_mac.mac[1] , eth_frame->src_mac.mac[2] , eth_frame->src_mac.mac[3] , eth_frame->src_mac.mac[4] , eth_frame->src_mac.mac[5]);
    return FALSE;
}

char* pkt_buffer_shift_right( char* pkt , unsigned int pkt_size , unsigned int total_buffer_size);
 
#endif