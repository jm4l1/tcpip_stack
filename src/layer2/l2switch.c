#include "glthread.h"
#include "graph.h"
#include "net.h"
#include "layer2.h"
#include "communications.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct mac_table_{
    glthread_t mac_entries;
}mac_table_t;
typedef struct mac_table_entry_{
    mac_addr_t mac;
    char oif_name[IF_NAME_SIZE];
    glthread_t mac_entry_glue;
}mac_table_entry_t;
GLTHREAD_TO_STRUCT(arp_glue_to_mac_table_entry , mac_table_entry_t , mac_entry_glue);

void init_mac_table(mac_table_t** mac_table){
    *mac_table = calloc(1 , sizeof(mac_table_t));
    init_glthread(&( (*mac_table)->mac_entries ));

    if(FALSE) printf("Info - MAC Address Table Initialized\n");
}
void mac_table_dump( mac_table_t* mac_table){
    glthread_t *curr;
    mac_table_entry_t *mac_table_entry;

    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries , curr){
        if(!curr) return;
        mac_table_entry = arp_glue_to_mac_table_entry(curr);
        printf("MAC : %x:%x:%x:%x:%x:%x\t OIF : %s\n" ,
        mac_table_entry->mac.mac[0], 
        mac_table_entry->mac.mac[1], 
        mac_table_entry->mac.mac[2], 
        mac_table_entry->mac.mac[3], 
        mac_table_entry->mac.mac[4], 
        mac_table_entry->mac.mac[5], 
        mac_table_entry->oif_name );
    }ITERATE_GLTHREAD_END(&mac_table->mac_entries , curr)

}
void
mac_table_entry_delete(mac_table_t *mac_table , unsigned char* mac){
    glthread_t *curr;
    mac_table_entry_t *mac_table_entry;

    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries , curr){
        mac_table_entry = arp_glue_to_mac_table_entry(curr);
        if(strcmp(mac_table_entry->mac.mac , mac ) == 0 ){
            remove_glthread(&mac_table_entry->mac_entry_glue);
        }
    }ITERATE_GLTHREAD_END(&mac_table->mac_entries , curr)

}
mac_table_entry_t *
mac_table_entry_lookup(mac_table_t *mac_table , unsigned char* mac){
    glthread_t *curr;
    mac_table_entry_t *mac_table_entry;

    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries , curr){
        mac_table_entry = arp_glue_to_mac_table_entry(curr);
        if(memcmp(mac_table_entry->mac.mac , mac , sizeof(mac_addr_t) ) == 0 ) return mac_table_entry;
    }ITERATE_GLTHREAD_END(&mac_table->mac_entries , curr)

    return NULL;
}
bool_t
mac_table_entry_add(mac_table_t *mac_table , mac_table_entry_t *mac_table_entry){
    mac_table_entry_t *old_entry = mac_table_entry_lookup(mac_table , mac_table_entry->mac.mac);
    if(old_entry){
        mac_table_entry_delete(mac_table , old_entry->mac.mac);
    }
    init_glthread(&mac_table_entry->mac_entry_glue);
    glthread_add_next(&mac_table->mac_entries , &mac_table_entry->mac_entry_glue);
    return TRUE;
}
void
l2_switch_perform_mac_learning(node_t *node, char *src_mac, char *if_name){
    // - lookup mac in mac table
    if(node->debug_status == DEBUG_ON) printf("[l2_switch_perform_mac_learning] Info : %s : Performing MAC learning\n" , node->node_name);
    mac_table_t *mac_table = node->node_nw_prop.mac_table;
    mac_table_entry_t *new_mac_entry = calloc(1 , sizeof(mac_table_entry_t));
    strcpy(new_mac_entry->oif_name , if_name);
    memcpy(new_mac_entry->mac.mac , src_mac , sizeof(mac_addr_t));
    mac_table_entry_add(mac_table , new_mac_entry );
}
static bool_t
l2_switch_send_pkt_out(char *pkt, unsigned int pkt_size,interface_t *oif){
    if(IS_INTF_L3_MODE(oif)){
        if(oif->att_node->debug_status == DEBUG_ON) printf("[l2_switch_send_pkt_out] Fatal - L3 Interface selected to Perform L2 Forwarding\n");
        assert(0);
    }
    ethernet_frame_t *eth_frame = (ethernet_frame_t *) pkt;
    vlan_tagged_ethernet_frame_t *vlan_eth_frame = is_pkt_vlan_tagged(eth_frame);
    uint16_t vlan_id = NULL;
    if(vlan_eth_frame){
        vlan_id = vlan_eth_frame->vlan_8021q_tag.tci_vid;
    }
    if(IF_L2_MODE(oif) == TRUNK){
        if(!is_trunk_interface_vlan_member(oif,vlan_id)){
            if(oif->att_node->debug_status == DEBUG_ON) printf("[l2_switch_send_pkt_out] Error - Node %s , Trunk IF %s is not a memeber of VLAN %hu \n" , oif->att_node->node_name , oif->if_name , vlan_id);
            return FALSE;
        }
        if(oif->att_node->debug_status == DEBUG_ON) printf("[l2_switch_send_pkt_out] Info - Node %s , IF %s is a memeber of VLAN %hu , Pakcet Forwarded on IF\n" , oif->att_node->node_name , oif->if_name , vlan_id);
        send_pkt_out(pkt , pkt_size , oif);
        return TRUE;
    }
    if(IF_L2_MODE(oif) == ACCESS){
        if(get_access_intf_operating_vlan_id(oif) != vlan_id){
            if(oif->att_node->debug_status == DEBUG_ON) printf("[l2_switch_send_pkt_out] Error - Node %s , Access IF %s is not a memeber of VLAN %hu\n" , oif->att_node->node_name , oif->if_name , vlan_id);
            return FALSE;
        }
        if(oif->att_node->debug_status == DEBUG_ON) printf("[l2_switch_send_pkt_out] Info - Node %s , IF %s is a member of VLAN %hu , Will Untag Pakcet and Forwarded on IF\n" , oif->att_node->node_name , oif->if_name , vlan_id);
        uint32_t new_pkt_size = 0;
        eth_frame = untag_pkt_with_vlan_id((ethernet_frame_t *)pkt , pkt_size , &new_pkt_size );
        send_pkt_out( (char *)eth_frame , new_pkt_size , oif);
        return TRUE;
    }
    if(oif->att_node->debug_status == DEBUG_ON) printf("[l2_switch_send_pkt_out] Info : Frame Dropped by L2 on IF %s\n" , oif->if_name);
    return FALSE;
}

static int
l2_switch_flood_pkt_out(node_t *node,interface_t *exempted_intf,char *pkt, unsigned int pkt_size){
    for(int i = 0 ; i < MAX_INTF_PER_NODE ; i++){
        interface_t *intf = node->intf[i];
        if(!intf) continue;
        if(intf == exempted_intf ) continue;
        if(IF_L2_MODE(intf) == L2_MODE_UNKNOWN ) continue;
        l2_switch_send_pkt_out(pkt ,  pkt_size , intf);
    }
    return 0;
};
void
l2_switch_forward_frame(node_t *node , interface_t *intf , char *pkt , uint32_t pkt_size){
    ethernet_frame_t *eth_frame = ( ethernet_frame_t *)pkt;
    if(IS_MAC_BROADCAST_ADDR(eth_frame->dest_mac.mac)){
        if(node->debug_status == DEBUG_ON) printf("[l2_switch_forward_frame] Info : %s -  Broadcast Frame received .. will flood frame src MAC %x:%x:%x:%x:%x:%x\n" ,  node->node_name, eth_frame->src_mac.mac[0] , eth_frame->src_mac.mac[1], eth_frame->src_mac.mac[2], eth_frame->src_mac.mac[3], eth_frame->src_mac.mac[4], eth_frame->src_mac.mac[5]);
        l2_switch_flood_pkt_out(node , intf , pkt , pkt_size);
        return;
    }
    char *dest_mac = (char* ) eth_frame->dest_mac.mac;
    mac_table_t *mac_table = node->node_nw_prop.mac_table;
    mac_table_entry_t *mac_entry = mac_table_entry_lookup(mac_table , dest_mac);
    if(!mac_entry){
        if(node->debug_status == DEBUG_ON) printf("[l2_switch_forward_frame] Info : %s - mac not found in table .. will flood frame\n" , node->node_name);
        l2_switch_flood_pkt_out(node , intf , pkt , pkt_size);
        return;
    }

    if(node->debug_status == DEBUG_ON) printf("[l2_switch_forward_frame] Info : %s - mac not %x:%x:%x:%x:%x:%x found  .. will forward frame to IF %s\n" , node->node_name , mac_entry->mac.mac[0] , mac_entry->mac.mac[1] , mac_entry->mac.mac[2], mac_entry->mac.mac[3], mac_entry->mac.mac[4], mac_entry->mac.mac[5] ,mac_entry->oif_name );
    l2_switch_send_pkt_out(pkt , pkt_size ,  get_node_if_by_name(node , mac_entry->oif_name) );
    return;
}
void 
l2_switch_recv_frame(interface_t *intf, char *pkt , uint32_t pkt_size){
    ethernet_frame_t *eth_frame = ( ethernet_frame_t *)pkt;
    node_t *node = intf->att_node;
    if(node->debug_status == DEBUG_ON) printf("[l2_switch_recv_frame ] Info : Frame received on node %s IIF %s with src MAC %x:%x:%x:%x:%x:%x\n" , node->node_name , intf->if_name , eth_frame->src_mac.mac[0] , eth_frame->src_mac.mac[1], eth_frame->src_mac.mac[2], eth_frame->src_mac.mac[3], eth_frame->src_mac.mac[4], eth_frame->src_mac.mac[5]);

    l2_switch_perform_mac_learning(node , eth_frame->src_mac.mac , intf->if_name);
    l2_switch_forward_frame(node , intf , (char *) pkt , pkt_size);
}