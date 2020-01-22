#include "../gluethread/glthread.h"
#include "../graph.h"
#include "../net.h"
#include "layer2.h"
#include "../communications.h"
#include <stdio.h>
#include <stdlib.h>


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

    printf("Info - MAC Address Table Initialized\n");
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
mac_table_entry_delete(mac_table_t *mac_table , char* mac){
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
mac_table_entry_lookup(mac_table_t *mac_table , char* mac){
    glthread_t *curr;
    mac_table_entry_t *mac_table_entry;

    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries , curr){
        mac_table_entry = arp_glue_to_mac_table_entry(curr);
        if(strcmp(mac_table_entry->mac.mac , mac ) == 0 ) return mac_table_entry;
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
    printf("Info : Performing MAC learning\n");
    mac_table_t *mac_table = node->node_nw_prop.mac_table;
    mac_table_entry_t *new_mac_entry = calloc(1 , sizeof(mac_table_entry_t));
    strcpy(new_mac_entry->oif_name , if_name);
    memcpy(new_mac_entry->mac.mac , src_mac , sizeof(mac_addr_t));
    mac_table_entry_add(mac_table , new_mac_entry );
}
void
l2_switch_forward_frame(node_t *node , interface_t *intf , char *pkt , uint32_t pkt_size){
    ethernet_frame_t *eth_frame = ( ethernet_frame_t *)pkt;
    if(IS_MAC_BROADCAST_ADDR(eth_frame->dest_mac.mac)){
        printf("Info - Broadcast Frame received .. will flood frame src MAC %x:%x:%x:%x:%x:%x\n" ,   eth_frame->src_mac.mac[0] , eth_frame->src_mac.mac[1], eth_frame->src_mac.mac[2], eth_frame->src_mac.mac[3], eth_frame->src_mac.mac[4], eth_frame->src_mac.mac[5]);
        send_pkt_flood_l2_intf_only(node , intf , pkt , pkt_size);
        return;
    }
    char *dest_mac = (char* ) eth_frame->dest_mac.mac;
    mac_table_t *mac_table = node->node_nw_prop.mac_table;
    mac_table_entry_t *mac_entry = mac_table_entry_lookup(mac_table , dest_mac);
    if(!mac_entry){
        printf("Info - mac not found in table .. will flood frame\n");
        send_pkt_flood_l2_intf_only(node , intf , pkt , pkt_size);
        return;
    }

    printf("Info - mac not %x:%x:%x:%x:%x:%x found  .. will forward frame to IF %s\n" , mac_entry->mac.mac[0] , mac_entry->mac.mac[1] , mac_entry->mac.mac[2], mac_entry->mac.mac[3], mac_entry->mac.mac[4], mac_entry->mac.mac[5] ,intf->if_name );
    send_pkt_out(pkt , pkt_size , intf);
}
void 
l2_switch_recv_frame(interface_t *intf, char *pkt , uint32_t pkt_size){
    ethernet_frame_t *eth_frame = ( ethernet_frame_t *)pkt;
    node_t *node = intf->att_node;
    printf("Info Frame received on node %s IIF %s with src MAC %x:%x:%x:%x:%x:%x\n" , node->node_name , intf->if_name , eth_frame->src_mac.mac[0] , eth_frame->src_mac.mac[1], eth_frame->src_mac.mac[2], eth_frame->src_mac.mac[3], eth_frame->src_mac.mac[4], eth_frame->src_mac.mac[5]);

    l2_switch_perform_mac_learning(node , eth_frame->src_mac.mac , intf->if_name);
    l2_switch_forward_frame(node , intf , (char *) pkt , pkt_size);
}

