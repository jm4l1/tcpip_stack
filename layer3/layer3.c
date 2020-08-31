#include "layer3.h"
#include "../utils.h"
#include "../tcpconst.h"
#include  <arpa/inet.h>
#include <math.h>
#include "../communications.h"

extern void
demote_pkt_to_layer2(node_t *node , uint32_t next_hop_ip ,interface_t *intf,  char* pkt, uint32_t pkt_size , uint8_t protocol_number);
extern char* 
pkt_buffer_shift_right( char* pkt , unsigned int pkt_size , unsigned int total_buffer_size);
extern void
ip_pkt_dump(char *pkt);
extern void
icmp_pkt_dump(char *pkt);

void init_rt_table(route_table_t **route_table)
{
    *route_table = calloc(1 , sizeof(route_table_t));
    init_glthread(&( (*route_table)->route_entries ));
    if(FALSE) printf("[init_rt_table] Info - Route Table Initialized\n");
};
void rt_table_add_direct_route(route_table_t *route_table , char *dst , uint8_t mask)
{
    layer3_route_t *direct_route = calloc(1 , sizeof(layer3_route_t));
    strcpy(direct_route->dest , dst);
    direct_route->mask = mask;
    direct_route->is_direct = TRUE;

    init_glthread(&direct_route->l3route_glue);
    glthread_add_next(&route_table->route_entries , &direct_route->l3route_glue);
};
void rt_dump_table(route_table_t *route_table)
{
    glthread_t *curr;
    layer3_route_t *l3route;
    ITERATE_GLTHREAD_BEGIN(&route_table->route_entries , curr)
    {
        if(!curr) return;
        l3route = route_glue_to_l3_route(curr);
        printf("Dst: %2s/%-4hhu %-5s , GW: %s, OIF : %s\n" , l3route->dest , l3route->mask , ( l3route->is_direct ? "directly connected" : "remote subnet" ) , l3route->gw , l3route->oif);
    }ITERATE_GLTHREAD_END(&route_table->route_entries , curr)
};
void rt_table_add_route(route_table_t *route_table , char *dst , uint8_t mask , char *gw , char *oif_name)
{
    printf("[rt_table_add_route] Adding route : %s / %hhu next-hop %s via %s\n" , dst , mask , gw , oif_name );
    layer3_route_t *direct_route = calloc(1 , sizeof(layer3_route_t));
    char dest_network[16];
    memset(dest_network , 0 , INET_ADDRSTRLEN);
    apply_mask(dst ,  (char)mask ,dest_network);
    strcpy(direct_route->dest, dest_network);
    direct_route->mask = mask;
    direct_route->is_direct = FALSE;
    strcpy(direct_route->gw , gw);
    strcpy(direct_route->oif , oif_name);

    init_glthread(&direct_route->l3route_glue);
    glthread_add_next(&route_table->route_entries , &direct_route->l3route_glue);

}
layer3_route_t * l3rib_lookup_lpm(route_table_t *route_table, uint32_t dest)
{
    // printf("[l3rib_lookup_lpm] - looking up route\n");
    layer3_route_t *l3_route = NULL;
    layer3_route_t *lpm_l3_route = NULL;

    glthread_t *curr = NULL;
    char subnet[16];
    char dest_ip_str[16];
    uint8_t longest_mask = 0;
    
    inet_ntop(AF_INET , &dest , dest_ip_str , INET_ADDRSTRLEN);

   ITERATE_GLTHREAD_BEGIN(&route_table->route_entries, curr){
        l3_route = route_glue_to_l3_route(curr);
        memset(subnet, 0, 16);
        apply_mask(dest_ip_str, l3_route->mask, subnet);
        // printf("[l3rib_lookup_lpm] - destination ip %s , subnet calculated %s , l3_route mask %hu ,l3_route dest  %s  , match check %d\n",  
        //         dest_ip_str , subnet , l3_route->mask , l3_route->dest , strcmp(subnet, l3_route->dest) );
        if(strcmp(subnet, l3_route->dest) == 0)
        {
            // printf("[l3rib_lookup_lpm] - route is a match\n");
            if( l3_route->mask > longest_mask){
                longest_mask = l3_route->mask;
                lpm_l3_route = l3_route;
            }
        }
    }ITERATE_GLTHREAD_END(&route_table->route_entries, curr);

    // printf("[l3rib_lookup_lpm] - Dest %s , Subnet Calculated %s ,  Subnet Matched %s \n", dest_ip_str , subnet ,  lpm_l3_route->dest);
    return lpm_l3_route;
}
static bool_t
l3_is_direct_route(layer3_route_t *l3_route)
{
    return l3_route->is_direct;
}
bool_t
is_layer3_local_delivery(node_t *node, unsigned int dst_ip)
{
    char ip_addr[INET_ADDRSTRLEN];
    memset(ip_addr, 0 , INET_ADDRSTRLEN);
    if(node->debug_status == DEBUG_ON) printf("[is_layer3_local_delivery] - Node %s - checking local delivery\n" , node->node_name);
    convert_ip_from_int_to_str(dst_ip , ip_addr);
    printf("Comparing lo %s to %s\n", NODE_LO_ADDR(node) , ip_addr);
    if( strcmp(NODE_LO_ADDR(node) , ip_addr ) == 0)
    {
        if(node->debug_status == DEBUG_ON) printf("[is_layer3_local_delivery] - Node %s - IP is a loopback\n" , node->node_name);
        return TRUE;
    }
    if(node->debug_status == DEBUG_ON) printf("[is_layer3_local_delivery] - Node %s - IP is not Local Loopback\n" , node->node_name);
    interface_t *intf;
    for(int i = 0 ; i < MAX_INTF_PER_NODE ; i++)
    {
        intf = node->intf[i];
        if(!intf) continue;
        if(!IS_INTF_L3_MODE(intf)) continue;

        if(node->debug_status == DEBUG_ON) printf("[is_layer3_local_delivery] - Node %s - Comparing IP %s and %s\n" , node->node_name ,IF_IP(intf) , ip_addr);
        if(strcmp(IF_IP(intf) , ip_addr ) == 0)
        {
            if(node->debug_status == DEBUG_ON) printf("[is_layer3_local_delivery] - Node %s - IP is a local address\n" , node->node_name);
            return TRUE;
        }
    }
    if(node->debug_status == DEBUG_ON) printf("[is_layer3_local_delivery] - Node %s - IP is not a local delivery\n" , node->node_name);
    return FALSE;
}
static icmp_pkt_t *
make_icmp_time_exceeded(ip_hdr_t *ip_hdr , uint8_t code){
    icmp_hdr_t *header = calloc(1 , sizeof(icmp_hdr_t));
    icmp_pkt_t *time_exceeded = calloc(1 , sizeof(icmp_pkt_t));
    uint32_t data = ip_hdr->dest; 
    header->type = ICMP_TYPE_TIME_EXCEEDED;
    header->code = code;
    header->checksum = 0;
    header->identifier = NULL;
    header->seq_num = NULL;
    memcpy(&time_exceeded->header , header , sizeof(icmp_hdr_t));
    memset(&time_exceeded->data , 0 , sizeof(uint32_t));
    memcpy(&time_exceeded->data , &data , sizeof(uint32_t));

    return time_exceeded;
}

static icmp_pkt_t *
make_icmp_dest_unreacheable(ip_hdr_t *ip_hdr , uint8_t code)
{
    icmp_hdr_t *header = calloc(1 , sizeof(icmp_hdr_t));
    icmp_pkt_t *dest_unreachable = calloc(1 , sizeof(icmp_pkt_t));
    uint32_t data = ip_hdr->dest; 
    header->type = ICMP_TYPE_DEST_UNREACH;
    header->code = code;
    header->checksum = 0;
    header->identifier = NULL;
    header->seq_num = NULL;
    memcpy(&dest_unreachable->header , header , sizeof(icmp_hdr_t));
    memset(&dest_unreachable->data , 0 , sizeof(uint32_t));
    memcpy(&dest_unreachable->data , &data , sizeof(uint32_t));

    return dest_unreachable;
}
void send_icmp_time_exceeded(node_t *node,ip_hdr_t *ip_hdr)
{
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_time_exceeded] - creating ICMP Time Exceeded\n");
    icmp_pkt_t *icmp_time_exceeded = make_icmp_time_exceeded(ip_hdr  , ICMP_CODE_TIME_EXCEEDED_TTL);
    demote_pkt_to_layer3(node , (char *) icmp_time_exceeded , sizeof(icmp_pkt_t) , ICMP_PROTO , ip_hdr->src);
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_time_exceeded] - Time Exceeded Packet sent\n");
}
void send_icmp_dest_unreacheable(node_t *node,ip_hdr_t *ip_hdr)
{
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_dest_unreacheable] - creating Destination Unreachable\n");
    icmp_pkt_t *icmp_dest_unreacheable = make_icmp_dest_unreacheable(ip_hdr  , ICMP_CODE_DEST_UNREACH_NET);
    demote_pkt_to_layer3(node , (char *) icmp_dest_unreacheable , sizeof(icmp_pkt_t) , ICMP_PROTO , ip_hdr->src);
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_dest_unreacheable] - Destination Unreachable Packet sent\n");
}
static void
layer3_pkt_receive_from_top(node_t *node, char *pkt, uint32_t data_size , uint8_t protocol_number , uint32_t dest_ip_address)
{
    //create IP header to be added to data
    ip_hdr_t ip_hdr;
    init_ip_hdr(&ip_hdr);
    char dest_string[16];
    uint32_t next_hop_add;
    uint32_t src_addr;
    interface_t *oif;
    layer3_route_t *l3_rt = l3rib_lookup_lpm(node->node_nw_prop.route_table , dest_ip_address);
    char *l3_pkt = NULL;
    uint32_t new_pkt_size ;
    memset(dest_string , 0 , INET_ADDRSTRLEN);
    inet_ntop(AF_INET , &dest_ip_address , dest_string , INET_ADDRSTRLEN);
    // printf("destination ip is %s\n", dest_string);     //src address chosen to be that of the next hop interface 
    ip_hdr.proto = protocol_number;
    ip_hdr.ihl = 5;
    ip_hdr.dest = dest_ip_address; 
    if(!l3_rt){
        if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - No route to destination!\n");
        if(ip_hdr.src == 0 )
        {
            inet_pton(AF_INET , NODE_LO_ADDR(node) , &ip_hdr.src);
        }
        send_icmp_dest_unreacheable(node,&ip_hdr);
        //send ICMP not route to destination to layer 2
        return;
    }
    if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - Route found to %s via %s!\n" , l3_rt->dest , l3_rt->oif);
    //can aditionally be loop back interface
    if(l3_is_direct_route(l3_rt))
    {
        oif = node_get_matching_subnet_interface(node , l3_rt->dest);
        if(!oif)
        {
            inet_pton(AF_INET , NODE_LO_ADDR(node) ,&src_addr);
            ip_hdr.src = src_addr;
            if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - Direct Route found, will use Loopback\n");
        }
        else
        {
            inet_pton(AF_INET , IF_IP(oif) ,&src_addr);
            ip_hdr.src = src_addr;
            if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - Direct Route found, will use OIF %s\n" , oif->if_name);
        }

    }
    else
    {
        oif = node_get_matching_subnet_interface(node , l3_rt->gw);
        if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - Packet will be forwarded, will use OIF %s\n" , oif->if_name);
        inet_pton(AF_INET , IF_IP(oif) ,&src_addr);
        ip_hdr.src = src_addr;
    }
    
    ip_hdr.TTL = 64;
    ip_hdr.version = IPPROTO_IPV4;
    ip_hdr.len =  ((uint16_t) (ip_hdr.ihl  * 4 ) ) +  ((uint16_t) data_size);
    // calculate checksum
    ip_hdr.checksum = 0;
    l3_pkt = calloc(1 , MAX_PACKET_BUFFER_SIZE);
    memcpy(l3_pkt , (char *) &ip_hdr , ip_hdr.ihl * 4);
    memcpy(l3_pkt +(ip_hdr.ihl * 4) , pkt , data_size );
    new_pkt_size = ip_hdr.len;
    if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - IP header created\n");
    char *shifted_pkt_buffer = pkt_buffer_shift_right(l3_pkt , new_pkt_size , MAX_PACKET_BUFFER_SIZE);

    if(!l3_is_direct_route(l3_rt))
    {
        if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - route is not direct route\n");
        inet_pton(AF_INET , l3_rt->gw , &next_hop_add);
        if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - sending out OIF %s (%s)\n" , oif->if_name , oif->intf_nw_props.ip_add.ip_addr);
        demote_pkt_to_layer2(node,next_hop_add , oif , shifted_pkt_buffer , new_pkt_size , IP_PROTO);
    }
    else
    {
        if(node->debug_status == DEBUG_ON) printf("[layer3_pkt_receive_from_top] - route direct route\n");
        demote_pkt_to_layer2(node,dest_ip_address , 0 , shifted_pkt_buffer , new_pkt_size , IP_PROTO);
    }
    free(l3_pkt);
    return;
}
void demote_pkt_to_layer3(node_t *node, char *pkt, uint32_t data_size , uint8_t protocol_number , uint32_t dest_ip_address)
{
    layer3_pkt_receive_from_top(node , pkt , data_size , protocol_number , dest_ip_address);
}
static icmp_pkt_t *
make_icmp_echo_reply(uint16_t identifier , uint16_t seq_num , uint32_t data)
{
    icmp_hdr_t *header = calloc(1 , sizeof(icmp_hdr_t));
    icmp_pkt_t * echo_reply = calloc(1 , sizeof(icmp_hdr_t) + sizeof(uint32_t));

    header->type = ICMP_TYPE_ECHO_REPLY;
    header->code = ICMP_CODE_ECHO;
    header->checksum = 0;
    header->identifier = identifier;
    header->seq_num = seq_num;

    memcpy(&echo_reply->header , header , sizeof(icmp_hdr_t));
    memset(&echo_reply->data , 0 , sizeof(uint32_t));
    memcpy(&echo_reply->data , &data, sizeof(uint32_t));

    return echo_reply;
}
static icmp_pkt_t *
make_icmp_echo_message(uint16_t identifier , uint16_t seq_num )
{
    uint32_t data = 0xDEADBEEF;
    icmp_hdr_t *header = calloc(1 , sizeof(icmp_hdr_t));
    icmp_pkt_t * echo_request = calloc(1 , sizeof(icmp_hdr_t) + sizeof(data));
    header->type = ICMP_TYPE_ECHO_MESSAGE;
    header->code = ICMP_CODE_ECHO;
    header->checksum = 0;
    header->identifier = identifier;
    header->seq_num = seq_num;

    memcpy(&echo_request->header , header , sizeof(icmp_hdr_t));

    memset(&echo_request->data , 0 , sizeof(uint32_t));
    echo_request->data = data ;
    return echo_request;
}
void send_icmp_echo_reply (node_t *node, uint16_t identifier , uint16_t seq_num , uint32_t reply_data , uint32_t dest_ip )
{
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_echo_reply] - creating echo reply\n");
    icmp_pkt_t *icmp_echo_reply = make_icmp_echo_reply(identifier , seq_num , reply_data);
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_echo_reply] - sending Packet with sequence number %hu\n", seq_num);
    demote_pkt_to_layer3(node , (char *) icmp_echo_reply , sizeof(icmp_pkt_t) , ICMP_PROTO , dest_ip);
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_echo_reply] - Packet %hu sent\n", seq_num);
}
void send_icmp_echo_message (node_t *node, uint16_t identifier , uint16_t seq_num , uint32_t dest_ip )
{    
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_echo_message] - creating echo message\n");
    icmp_pkt_t *icmp_echo_message = make_icmp_echo_message(identifier , seq_num);
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_echo_message] - sending Packet with sequence number %hu\n", seq_num);
    demote_pkt_to_layer3(node , (char *) icmp_echo_message , sizeof(icmp_pkt_t) , ICMP_PROTO , dest_ip);
    if(node->debug_status == DEBUG_ON) printf("[send_icmp_echo_message] - Packet %hu sent\n", seq_num);
}
static void
process_icmp_pkt (node_t *node , ip_hdr_t* ip_hdr ){
    char src_add[INET_ADDRSTRLEN];
    char dest_add[INET_ADDRSTRLEN];
    uint32_t reply_data;
    char *ip_payload = (char *) ip_hdr + IP_HDR_LEN_IN_BYTES(ip_hdr);
    icmp_pkt_t *icmp_pkt = (icmp_pkt_t *) ip_payload;
    convert_ip_from_int_to_str(ip_hdr->src, src_add);
    switch (icmp_pkt->header.type)
    {
    case ICMP_TYPE_ECHO_REPLY:
        memset(src_add , 0 , INET_ADDRSTRLEN);
        printf("Node %s : %hu bytes received from %s: icmp_seq=%hu ttl=%hhu\n", 
                node->node_name , IP_HDR_PAYLOAD_SIZE(ip_hdr) , src_add , icmp_pkt->header.seq_num , ip_hdr->TTL);
        break;
    
    case ICMP_TYPE_DEST_UNREACH:
        memset( dest_add , 0 , INET_ADDRSTRLEN);
        convert_ip_from_int_to_str(icmp_pkt->data , dest_add);
        switch (icmp_pkt->header.code)
            {
                case ICMP_CODE_DEST_UNREACH_NET:
                    printf("Node %s : From IP %s ,Destination  Unreachable  (3) %s - %s \n", 
                        node->node_name , src_add,  dest_add , "Network Unreachable (0)");
                    break;
                
                default:
                    printf("Node %s : From IP %s ,Destination  Unreachable  %s - %s \n", 
                            node->node_name , src_add,  dest_add , "Code Unknown");
                    break;
            }
        break;
    case ICMP_TYPE_ECHO_MESSAGE:
        reply_data = htonl(icmp_pkt->data);
        send_icmp_echo_reply(node ,icmp_pkt->header.identifier , icmp_pkt->header.seq_num ,reply_data , ip_hdr->src );
        break;
    case ICMP_TYPE_TIME_EXCEEDED:
        if(node->debug_status == DEBUG_ON) printf("[process_icmp_pkt] - Processing ICMP Packet\n");
        memset( dest_add , 0 , INET_ADDRSTRLEN);
        convert_ip_from_int_to_str(icmp_pkt->data , dest_add);
        switch (icmp_pkt->header.code)
        {
            case ICMP_CODE_TIME_EXCEEDED_TTL:
                printf("Node %s : From IP %s ,TTL Expired In transit - %s \n", 
                        node->node_name , src_add,  dest_add);
                break;
            
            default:
                break;
        }

    default:
        break;
    }
}
static void
layer3_ip_pkt_recv_from_layer2(node_t *node , interface_t *intf , ip_hdr_t *pkt, uint32_t pkt_size )
{
    ip_hdr_t *ip_hdr = pkt;
    uint32_t dest_ip = ip_hdr->dest;
    uint32_t next_hop_add ;
    char dest_ip_add[INET_ADDRSTRLEN] ;

    inet_ntop(AF_INET , &dest_ip , dest_ip_add ,  INET_ADDRSTRLEN );

    if (node->debug_status == DEBUG_ON) printf("[layer3_ip_pkt_recv_from_layer2] - Node %s - Info : Looking up ip %s in route table\n", node->node_name, dest_ip_add);
    //lookup route
    layer3_route_t *l3_rt = l3rib_lookup_lpm(node->node_nw_prop.route_table , dest_ip); 
    //route not found
    if(!l3_rt)
    {
       if (node->debug_status == DEBUG_ON) printf("[layer3_ip_pkt_recv_from_layer2] - Node %s - Info : No route found for ip %s\n", node->node_name, dest_ip_add);
        send_icmp_dest_unreacheable(node,ip_hdr);
        return;
    }
    if (node->debug_status == DEBUG_ON) printf("[layer3_ip_pkt_recv_from_layer2] - Node %s - Info : Route found for ip %s\n", node->node_name,dest_ip_add);
    // route found
    if(l3_is_direct_route(l3_rt))
    {//is route local
       if (node->debug_status == DEBUG_ON) printf("[layer3_ip_pkt_recv_from_layer2] - Node %s - Info Route Matched Direct Route\n", node->node_name);
        if(is_layer3_local_delivery(node, dest_ip))
        {//local delivery
            if (node->debug_status == DEBUG_ON) printf("[layer3_ip_pkt_recv_from_layer2] - Node %s - Local Delivery\n", node->node_name);
            switch (ip_hdr->proto)
            {
                case ICMP_PROTO:
                    process_icmp_pkt(node , ip_hdr);
                    break;
                default:
                    break;
            }
        }
        else
        {//directly connected host delivery
            if (node->debug_status == DEBUG_ON) printf("[layer3_ip_pkt_recv_from_layer2] - Node %s - Connected host Delivery\n", node->node_name);
            demote_pkt_to_layer2(node, dest_ip , NULL ,  (char *)ip_hdr , pkt_size, IP_PROTO);
        }
        return;
    }
    else
    {//forwading case
       if (node->debug_status == DEBUG_ON) printf("[layer3_ip_pkt_recv_from_layer2] - Node %s  : Packet to be forwarded\n", node->node_name);
        --ip_hdr->TTL;
        if((ip_hdr->TTL) == 0){
            send_icmp_time_exceeded(node,ip_hdr);
            return;
        }
        inet_pton(AF_INET, l3_rt->gw , &next_hop_add);
        demote_pkt_to_layer2(node, next_hop_add  , l3_rt->oif ,  (char *)ip_hdr , pkt_size , IP_PROTO);
    }
}
static void
_layer3_pkt_recv_from_layer2(node_t *node , interface_t *intf , char *pkt , uint32_t pkt_size , uint16_t protocol_type)
{
    switch (protocol_type)
    {
        case IP_PROTO:
        {
            if(node->debug_status == DEBUG_ON) printf("[_layer3_pkt_recv_from_layer2] -  Protocol Type  0x%x Received , Forwarding to IP Handler\n" ,protocol_type );
            layer3_ip_pkt_recv_from_layer2(node , intf ,  (ip_hdr_t *) pkt , pkt_size);
            break;
        }
        default:
            if(node->debug_status == DEBUG_ON) printf("[_layer3_pkt_recv_from_layer2] - Unknown Protocol Type  0x%x , Dropping Packet\n" ,protocol_type );
            break;
    }
}
void promote_pkt_to_layer3(node_t *node , interface_t *recv_intf , char *payload , uint32_t app_data_size , uint16_t protocol_number)
{
    _layer3_pkt_recv_from_layer2(node , recv_intf , payload , app_data_size ,  protocol_number);
}
