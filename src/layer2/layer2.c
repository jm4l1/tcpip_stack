#include "layer2.h"
#include <stdio.h>
#include <assert.h>
#include "communications.h"
#include  <arpa/inet.h>


extern void 
l2_switch_recv_frame(interface_t *intf, char *pkt , uint32_t pkt_size);
extern void 
promote_pkt_to_layer3(node_t *node , interface_t *recv_intf , char *payload , uint32_t app_data_size , uint16_t protocol_number);
extern bool_t
is_layer3_local_delivery(node_t *node, unsigned int dst_ip);
extern void
pkt_dump( ethernet_frame_t *eth_frame , unsigned int pkt_size);
char* 
pkt_buffer_shift_right( char* pkt , unsigned int pkt_size , unsigned int total_buffer_size){
    #if 0
        // Input buffer
        +--------------------------+------------------------------------------------------+
        | Aux Info |     Data      |               Empty buffer remaining                 |
        +--------------------------+------------------------------------------------------+
                   ^
                  pkt

        // Out buffer
        +---------+-----------------------------------------------------------------------+
        | Aux Info|                 Empty buffer remaining                |     Data      |
        +---------+-----------------------------------------------------------------------+
                                                                          ^                
                                                                         pkt               
    #endif
    // calculate size of empty buffer
    unsigned int empty_buffer_size = total_buffer_size - pkt_size;
    // create pointer to area after empty buffer
    char* new_pkt = pkt + empty_buffer_size;
    // copy pkt data to new_pkt area
    memcpy(new_pkt , pkt , pkt_size );
    // zeroise empty buffer at front of char buffer
    memset( pkt , 0 , empty_buffer_size);
    return new_pkt;
}
extern void
ip_pkt_dump(char *pkt);
// Arp Table APIs
void
init_arp_table(arp_table_t** arp_table){
    *arp_table = calloc(1 , sizeof(arp_table_t));
    init_glthread(&( (*arp_table)->arp_entries ));
    arp_entry_t *arp_entry = calloc(1 , sizeof(arp_entry_t));
    strcpy(arp_entry->ip_addr.ip_addr, "255.255.255.255");
    layer2_fill_with_broadcast_mac(arp_entry->mac_addr.mac);
    strcpy(arp_entry->oif_name , "local" );
    glthread_add_next(&( (*arp_table)->arp_entries ), &arp_entry->arp_glue);
    if(FALSE) printf("[init_arp_table] Info - Arp Table Initialized\n");
}
bool_t arp_table_entry_add(arp_table_t *arp_table , arp_entry_t *arp_entry ){
    //look up arp table for IP address
    arp_entry_t* arp_entry_old = arp_table_lookup(arp_table , arp_entry->ip_addr.ip_addr);
    if(arp_entry_old){
        //if entry is already in table 
        if(memcmp(arp_entry_old , arp_entry , sizeof(arp_entry_t)) == 0) return FALSE;

        // if entry exists but ip address is different
        arp_table_delete_entry(arp_table,arp_entry->ip_addr.ip_addr );
    }
    init_glthread(&arp_entry->arp_glue);
    glthread_add_next(&arp_table->arp_entries , &arp_entry->arp_glue);
    return TRUE;
}
arp_entry_t * arp_table_lookup(arp_table_t *arp_table, char *ip_addr){
    glthread_t *curr;
    arp_entry_t *arp_entry;

    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries , curr){
        arp_entry = arp_glue_to_arp_entry(curr);
        if(strcmp(arp_entry->ip_addr.ip_addr , ip_addr ) == 0 ) return arp_entry;
    }ITERATE_GLTHREAD_END(&arp_table->arp_entries , curr)

    return NULL;
};
void arp_table_update_from_arp_reply(arp_table_t *arp_table , arp_packet_t *arp_packet , interface_t *iif){
    assert(arp_packet->op_code == ARP_REPLY);

    uint32_t src_ip = 0;
    arp_entry_t* arp_entry = calloc(1 , sizeof(arp_entry_t));
    src_ip = htonl(arp_packet->src_ip);
    convert_ip_from_int_to_str(src_ip, arp_entry->ip_addr.ip_addr);
    memcpy(arp_entry->mac_addr.mac , arp_packet->src_mac.mac , sizeof(mac_addr_t));
    strcpy(arp_entry->oif_name , iif->if_name );
    
    bool_t entry_added = arp_table_entry_add(arp_table , arp_entry);
    if(entry_added == FALSE ) 
        free(arp_entry);
};
void arp_table_delete_entry(arp_table_t *arp_table, char *ip_addr){
    glthread_t *curr;
    arp_entry_t *arp_entry;

    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries , curr){
        arp_entry = arp_glue_to_arp_entry(curr);
        if(strcmp(arp_entry->ip_addr.ip_addr , ip_addr ) == 0 ){
            remove_glthread(&arp_entry->arp_glue);
        };
    }ITERATE_GLTHREAD_END(&arp_table->arp_entries , curr)
    
};
void arp_table_dump( arp_table_t *arp_table){
    glthread_t *curr;
    arp_entry_t *arp_entry;

    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries , curr){
        if(!curr) return;
        arp_entry = arp_glue_to_arp_entry(curr);
        printf("IP : %-16s\t MAC : %x:%x:%x:%x:%x:%x\t OIF : %s\n" , 
        arp_entry->ip_addr.ip_addr , 
        arp_entry->mac_addr.mac[0], 
        arp_entry->mac_addr.mac[1], 
        arp_entry->mac_addr.mac[2], 
        arp_entry->mac_addr.mac[3], 
        arp_entry->mac_addr.mac[4], 
        arp_entry->mac_addr.mac[5], 
        arp_entry->oif_name );
    }ITERATE_GLTHREAD_END(&arp_table->arp_entries , curr)

}
// Packet Processing APIs
arp_entry_t * create_arp_sane_entry(arp_table_t *arp_table , char *ip_addr)
{
    arp_entry_t *arp_entry = arp_table_lookup(arp_table , ip_addr);
    if(arp_entry){
        return arp_entry;
    }
    arp_entry = calloc(1 , sizeof(arp_entry_t));
    strcpy(arp_entry->ip_addr.ip_addr , ip_addr);
    init_glthread(&arp_entry->arp_pending_list);
    arp_entry->is_sane = TRUE;
    bool_t rc = arp_table_entry_add(arp_table , arp_entry);
    return arp_entry;
}
static void add_arp_pending_entry(arp_entry_t *arp_entry, arp_process_fun callback , char *pkt , uint32_t pkt_size)
{
    arp_pending_entry_t *arp_pending_entry = calloc( 1 , sizeof(arp_pending_entry_t) + pkt_size);
    init_glthread(&arp_pending_entry->arp_pending_entry_glue);
    arp_pending_entry->cb = callback;
    arp_pending_entry->pkt_size = pkt_size;
    memcpy(arp_pending_entry->pkt , pkt , pkt_size);
    glthread_add_next(&arp_entry->arp_pending_list , &arp_pending_entry->arp_pending_entry_glue);
}
void send_arp_broadcast_rquest(node_t *node , interface_t *oif , char *ip_addr){
    // if output interface is unknown to the caller 
    if(!oif){
        // mactch output interface based on ip_addr subnet
        oif = node_get_matching_subnet_interface(node , ip_addr);
        if(!oif){
            if(node->debug_status == DEBUG_ON) printf("[send_arp_broadcast_rquest] Info : node %s , No eligble interface found for IP %s\n" , node->node_name , ip_addr);
            return;
        } 
        if(strcmp(ip_addr , IF_IP(oif)) == 0){
            if(node->debug_status == DEBUG_ON) printf("[send_arp_broadcast_rquest] Error : node %s , Attempting to send Arp for local IP %s on interface OIF : %s \n" , node->node_name , ip_addr, oif->if_name);
            return;
        }
    }
    // allocate buffer for arp packet + ethernet header
    uint32_t eth_frame_size = ETH_HDR_SIZE_EXCL_PAYLOAD + ARP_PACKET_SIZE + 4 ;
    ethernet_frame_t *eth_frame = calloc( 1 , sizeof(eth_frame_size ));
    char* if_mac = IF_MAC(oif);
    uint32_t src_ip_l = convert_ip_from_str_to_int(IF_IP(oif));
    uint32_t dst_ip_l = convert_ip_from_str_to_int(ip_addr);

    //prepare L2 header
    layer2_fill_with_broadcast_mac(eth_frame->dest_mac.mac);
    memcpy(eth_frame->src_mac.mac,if_mac, sizeof(mac_addr_t));
    eth_frame->type = ARP_PACKET;

    // Prepare ARP packet (L2 payload)
    arp_packet_t* arp_broadcast = (arp_packet_t*) eth_frame->payload;

    arp_broadcast->hw_type = ETH_HW;
    arp_broadcast->proto_type = IP_PROTO;
    arp_broadcast->hw_addr_len = ETH_HW_LEN;
    arp_broadcast->proto_addr_len = IP_PROTO_LEN;
    arp_broadcast->op_code = ARP_REQUEST;
    memcpy(arp_broadcast->src_mac.mac , if_mac, sizeof(mac_addr_t));
    arp_broadcast->src_ip = htonl(src_ip_l);
    memset(arp_broadcast->dst_mac.mac , 0 , sizeof(mac_addr_t));
    arp_broadcast->dst_ip = htonl(dst_ip_l);

    // set FCS
    uint32_t* eth_fcs = ETH_FCS(eth_frame , ARP_PACKET_SIZE);
    *eth_fcs = 0;

    if (node->debug_status == DEBUG_ON) printf("[send_arp_broadcast_rquest] Info : sending Arp Broadcast from node %s on to IP %s on OIF: %s\n" , node->node_name , ip_addr , oif->if_name);
    send_pkt_out( (char *)eth_frame ,eth_frame_size ,oif );

    free(eth_frame);
}
static void
pending_arp_processing_callback_function(node_t *node,interface_t *oif, arp_entry_t *arp_entry,arp_pending_entry_t *arp_pending_entry)
{
    ethernet_frame_t *eth_frame = (ethernet_frame_t *)arp_pending_entry->pkt;
    unsigned int pkt_size = arp_pending_entry->pkt_size;
    memcpy(eth_frame->dest_mac.mac, arp_entry->mac_addr.mac, sizeof(mac_addr_t));
    memcpy(eth_frame->src_mac.mac, IF_MAC(oif), sizeof(mac_addr_t));
    set_common_eth_fcs(eth_frame, pkt_size - get_eth_hdr_size_excl_payload(eth_frame), 0);
    send_pkt_out((char *)eth_frame, pkt_size, oif);
}
static void 
send_arp_reply_msg(ethernet_frame_t *eth_frame_in , interface_t *oif){
    arp_packet_t *arp_request = (arp_packet_t*) eth_frame_in->payload;
    ethernet_frame_t *eth_frame_out = calloc(1 ,MAX_PACKET_BUFFER_SIZE);
    char *if_mac = IF_MAC(oif);

    memcpy(eth_frame_out->dest_mac.mac , eth_frame_in->src_mac.mac , sizeof(mac_addr_t));
    memcpy(eth_frame_out->src_mac.mac , if_mac , sizeof(mac_addr_t));
    eth_frame_out->type = ARP_PACKET;

    arp_packet_t *arp_reply= (arp_packet_t*) eth_frame_out->payload;
    
    arp_reply->hw_type = ETH_HW;
    arp_reply->proto_type = IP_PROTO;
    arp_reply->hw_addr_len = ETH_HW_LEN;
    arp_reply->proto_addr_len = IP_PROTO_LEN;
    arp_reply->op_code = ARP_REPLY;

    memcpy(arp_reply->src_mac.mac , if_mac, sizeof(mac_addr_t));
    arp_reply->src_ip = arp_request->dst_ip;
    memset(arp_reply->dst_mac.mac , arp_request->src_mac.mac , sizeof(mac_addr_t));
    arp_reply->dst_ip = arp_request->src_ip;

    // set FCS
    uint32_t* eth_fcs = ETH_FCS(eth_frame_out , ARP_PACKET_SIZE);
    *eth_fcs = 0;

    uint32_t total_pkt_size = ETH_HDR_SIZE_EXCL_PAYLOAD + ARP_PACKET_SIZE + 4;

    char *shifted_pkt_buffer = pkt_buffer_shift_right((char*) eth_frame_out , total_pkt_size , MAX_PACKET_BUFFER_SIZE);

    if( oif->att_node->debug_status == DEBUG_ON) printf("[send_arp_reply_msg] Info : %s - Sending Arp Reply\n",  oif->att_node->node_name);

    send_pkt_out(shifted_pkt_buffer , total_pkt_size , oif );

    free(eth_frame_out);
};
static void 
process_arp_broadcast_request(node_t *node , interface_t *iif , ethernet_frame_t *eth_frame){
    if(node->debug_status == DEBUG_ON) printf("[process_arp_broadcast_request] Info : Incoming ARP msg Revcd on IF %s of Node %s\n" , iif->if_name , node->node_name);
    arp_packet_t* arp_packet = (arp_packet_t*) eth_frame->payload;
    char target_ip[16];
    uint32_t target_ip_l = htonl(arp_packet->dst_ip);
    convert_ip_from_int_to_str(target_ip_l , target_ip);

    char* local_if_ip = IF_IP(iif);
    if( strcmp(local_if_ip , target_ip) != 0){
        if(node->debug_status == DEBUG_ON) printf("[process_arp_broadcast_request] Info :  Discarding ARP msg Recvd on IF %s of Node %s for IP %s\n" , iif->if_name , node->node_name , target_ip);
        return;
    }
    send_arp_reply_msg(eth_frame , iif);
}
static void
process_arp_reply_message(node_t *node , interface_t *iif , ethernet_frame_t *eth_frame){
    if(node->debug_status == DEBUG_ON) printf("[process_arp_reply_message] Info : %s - received arp reply\n" , node->node_name);

    arp_table_update_from_arp_reply(node->node_nw_prop.arp_table , (arp_packet_t*) eth_frame->payload , iif);
}
//L2 APIs
static void
promote_pkt_to_layer2(node_t *node , interface_t *intf ,ethernet_frame_t *eth_frame , uint32_t pkt_size ){
        uint16_t ethertype = eth_frame ->type;
        switch ( ethertype)
        {
            case ARP_PACKET:
                {
                    arp_packet_t *arp_packet = ( arp_packet_t*) eth_frame->payload;
                    switch (arp_packet->op_code)
                    {
                    case ARP_REQUEST:
                        if(node->debug_status == DEBUG_ON) printf("[promote_pkt_to_layer2] Info : %s - ARP Request received \n" , node->node_name);
                        process_arp_broadcast_request(node , intf , eth_frame);
                        break;
                    case ARP_REPLY:
                        if(node->debug_status == DEBUG_ON) printf("[promote_pkt_to_layer2] Info : %s - ARP Reply received \n", node->node_name);
                        process_arp_reply_message(node , intf , eth_frame);
                        break;
                    default:
                        break;
                    } 
                }
                break;
            case IP_PROTO:
                {
                    if(node->debug_status == DEBUG_ON) printf("[promote_pkt_to_layer2] Info : %s - IP (0x%4x) Packet Received , sending to layer 3 \n" , node->node_name , ethertype);
                    promote_pkt_to_layer3(node , intf , get_ethernet_frame_payload(eth_frame) ,  pkt_size - get_eth_hdr_size_excl_payload(eth_frame) ,  ethertype);
                }
                break;
            default:
                    if(node->debug_status == DEBUG_ON) printf("[promote_pkt_to_layer2] Info : %s - Unknown Ethertype 0x%4x \n" , ethertype);
            ;
        }
}
static void
l2_forward_ip_packet(node_t *node , uint32_t next_hop_ip, char *intf , ethernet_frame_t *eth_frame, uint32_t pkt_size){
    if(node->debug_status == DEBUG_ON) printf("[l2_forward_ip_packet] - Node %s - starting packet forward.\n", node->node_name);
    char next_hop_ip_str[16];
    interface_t *oif;
    inet_ntop(AF_INET , &next_hop_ip , next_hop_ip_str ,INET_ADDRSTRLEN );
    if(!intf)
    {
        if(is_layer3_local_delivery(node , next_hop_ip))
        {
            promote_pkt_to_layer3(node, 0 , get_ethernet_frame_payload(eth_frame) , pkt_size - get_eth_hdr_size_excl_payload(eth_frame) , eth_frame->type);
            return;
        }
        oif = node_get_matching_subnet_interface(node , next_hop_ip_str);
        if(node->debug_status == DEBUG_ON) printf("[l2_forward_ip_packet] - Node %s - Packet to be fowarded  out interface to %s\n" , node->node_name , oif->if_name , next_hop_ip_str);
    }
    else
    {
        oif = get_node_if_by_name(node , intf);
    }
    
    if(oif) if(node->debug_status == DEBUG_ON) printf("[l2_forward_ip_packet] - Node %s - interface %s found\n",node->node_name , oif->if_name);
    arp_entry_t *arp_entry =  arp_table_lookup(node->node_nw_prop.arp_table , next_hop_ip_str);
    if(!arp_entry)
    {
        if(node->debug_status == DEBUG_ON) printf("[l2_forward_ip_packet] - Node %s - no arp entry found for %s\n" ,node->node_name , next_hop_ip_str );
        return;
        //resolve arp
    }
    memcpy(eth_frame->dest_mac.mac , arp_entry->mac_addr.mac , 6);
    memcpy(eth_frame->src_mac.mac , IF_MAC(oif), 6);
    set_common_eth_fcs(eth_frame , pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD , 0 );
    send_pkt_out( (char*) eth_frame , pkt_size , oif);
    return;

}
void
demote_pkt_to_layer2(node_t *node , uint32_t next_hop_ip , interface_t *intf, char* pkt, uint32_t pkt_size , uint16_t protocol_number)
{ 
    if(node->debug_status == DEBUG_ON) printf("[demote_pkt_to_layer2] - protocol number %hu\n" , protocol_number);
    ethernet_frame_t *eth_frame = ALLOC_ETH_HDR_WITH_PAYLOAD(pkt, pkt_size);
    eth_frame->type = IP_PROTO;

    if(node->debug_status == DEBUG_ON) printf("[demote_pkt_to_layer2] - eth proto 0x%4x\n", eth_frame->type);
    if(node->debug_status == DEBUG_ON) printf("[demote_pkt_to_layer2] - L2 frame created for IP Packet\n");
    l2_forward_ip_packet(node , next_hop_ip , intf->if_name , eth_frame , ETH_HDR_SIZE_EXCL_PAYLOAD + pkt_size);
}
void
layer2_frame_recv(node_t* node , interface_t *intf, char *pkt , uint32_t pkt_size){

    ethernet_frame_t *eth_frame = (ethernet_frame_t *) pkt;
    uint16_t vlan_to_tag ;
    //Should the node accept Frame
    if(l2_frame_recv_qualify_on_interface(intf , eth_frame, &vlan_to_tag ) == FALSE){
        if(node->debug_status == DEBUG_ON) printf("[layer2_frame_recv] Info : %s - L2 frame discarded\n", node->node_name);
        return;
    };
    // Is IIF in L3 mode
    if(IS_INTF_L3_MODE(intf)){
        if(node->debug_status == DEBUG_ON) printf("[layer2_frame_recv] Info : %s - interface %s on node %s in %s mode , Sending to DL Layer\n" , node->node_name ,  intf->if_name , intf->att_node->node_name  , intf_l2_mode_str(intf->intf_nw_props.intf_l2_mode) );
        promote_pkt_to_layer2(node , intf , eth_frame , pkt_size);
    }
    // IS IIF in L2 mode
    else if(IF_L2_MODE(intf) != L2_MODE_UNKNOWN){
        if(node->debug_status == DEBUG_ON) printf("[layer2_frame_recv] Info : %s - interface %s on node %s in %s mode\n" ,node->node_name , intf->if_name , intf->att_node->node_name  , intf_l2_mode_str(intf->intf_nw_props.intf_l2_mode) );
        uint32_t new_pkt_size = 0;
        if(vlan_to_tag){
            pkt = (char *)tag_pkt_with_vlan_id((ethernet_frame_t *)pkt ,pkt_size , vlan_to_tag , &new_pkt_size);
        }
        l2_switch_recv_frame(intf, pkt , vlan_to_tag ? new_pkt_size : pkt_size );
    }
}
ethernet_frame_t *
tag_pkt_with_vlan_id(ethernet_frame_t *eth_frame , uint32_t total_pkt_size , uint16_t vlan_id , uint32_t *new_pkt_size){
    vlan_tagged_ethernet_frame_t* vlan_eth_frame ;
    vlan_eth_frame = is_pkt_vlan_tagged(eth_frame);

    char dest_mac[16];
    char src_mac[16];
    uint16_t type = eth_frame->type;
    memcpy(src_mac , eth_frame->src_mac.mac , sizeof(mac_addr_t));
    memcpy(dest_mac , eth_frame->dest_mac.mac , sizeof(mac_addr_t));
    uint32_t new_pkt_size_val = total_pkt_size;

    //if frame alread has a tag
    if(vlan_eth_frame){
        printf("[tag_pkt_with_vlan_id] Info : Frame is tagged with VLAN ID %hu , Updating to %hu\n" , vlan_eth_frame->vlan_8021q_tag.tci_vid  , vlan_id );
        vlan_eth_frame->vlan_8021q_tag.tci_vid = (uint16_t)vlan_id;
    }
    else{
        printf("[tag_pkt_with_vlan_id] Info : Tagging frame with VLAN ID %hu \n" , vlan_id );
        vlan_eth_frame = (vlan_tagged_ethernet_frame_t *)((char *)eth_frame - 4);
        vlan_eth_frame->vlan_8021q_tag.tpid = VLAN_TAG;
        vlan_eth_frame->vlan_8021q_tag.tci_vid = vlan_id;
        memcpy(vlan_eth_frame->src_mac.mac, src_mac, sizeof(mac_addr_t));
        memcpy(vlan_eth_frame->dest_mac.mac, dest_mac, sizeof(mac_addr_t));
        vlan_eth_frame->type = type;
        
        new_pkt_size_val = total_pkt_size + sizeof(vlan_8021q_tag_t);
    }
    *new_pkt_size = new_pkt_size_val;

    return (ethernet_frame_t *)vlan_eth_frame;
}
ethernet_frame_t *
untag_pkt_with_vlan_id(ethernet_frame_t *eth_frame , uint32_t total_pkt_size , uint32_t *new_pkt_size){
    uint32_t new_pkt_size_val = total_pkt_size;
    // check if frame is tagged
    vlan_tagged_ethernet_frame_t *vlan_eth_frame = is_pkt_vlan_tagged(eth_frame);

    if(!vlan_eth_frame){
        printf("[untag_pkt_with_vlan_id] Info : No Tag found in frame\n");
        *new_pkt_size = new_pkt_size_val;
        return eth_frame;
    }
     printf("[untag_pkt_with_vlan_id] Info :  Tag found %hu in frame , Removing\n" , vlan_eth_frame->vlan_8021q_tag.tci_vid);
    //copy src & dest mac from tagged frame
    char src_mac[16];
    char dest_mac[16];
    memcpy(src_mac , vlan_eth_frame->src_mac.mac , sizeof(mac_addr_t));
    memcpy(dest_mac , vlan_eth_frame->dest_mac.mac , sizeof(mac_addr_t));

    //zeroise src , dest , vlan header
    memset(vlan_eth_frame->src_mac.mac , 0 , sizeof(mac_addr_t));
    memset(vlan_eth_frame->dest_mac.mac , 0 , sizeof(mac_addr_t));
    memset(&vlan_eth_frame->vlan_8021q_tag , 0 , sizeof(vlan_8021q_tag_t));

    //cast memory to untagged ethernet fra,
    ethernet_frame_t *new_eth_frame = (ethernet_frame_t *)((char *)(vlan_eth_frame) + sizeof(vlan_8021q_tag_t));

    //set src &  dest mac
    memcpy(new_eth_frame->src_mac.mac , src_mac , sizeof(mac_addr_t));
    memcpy(new_eth_frame->dest_mac.mac , dest_mac , sizeof(mac_addr_t));

    *new_pkt_size = new_pkt_size_val;
    return new_eth_frame;
};