#include "libcli.h"
#include "cmdtlv.h"
#include "css.h"
#include "cmdcodes.h"
#include "graph.h"
#include <stdio.h>
#include <arpa/inet.h>

#define NUM_SAVED_TOPOS 5
char* saved_topologies[NUM_SAVED_TOPOS] = { 
"0 - Simple Host Topolgy \n\n\
                               +----------+ \n \
                           0/4 |          |0/0 \n \
        +----------------------+   R0_re  +---------------------+ \n \
        |           40.1.1.1/24| 122.1.1.0|20.1.1.1/24          | \n \
        |                      +----------+                     | \n \
        |                                                       | \n \
        |                                                       | \n \
        |                                                       | \n \
        |40.1.1.2/24                                            |20.1.1.2/24 \n \
        |0/5                                                    |0/1 \n \
    +---+---+                                              +----+-----+ \n \
    |       |0/3                                        0/2|          | \n \
    | R2_re +----------------------------------------------+    R1_re | \n \
    |       |30.1.1.2/24                        30.1.1.1/24|          | \n \
    +-------+                                              +----------+  \n\n ",
"1 - Linear \n\n\
    +-----------+                        +-----------+                         +-----------+                         +-----------+\n \
    |           |0/1                  0/2|           |0/3                   0/4|           |0/3                   0/2|           |\n \
    +     R0    +------------------------+     R1    +-------------------------+     R2    +-------------------------+     R3    +\n \
    |           |10.1.1.1/24  10.1.1.2/24|           |20.1.1.2/24   20.1.1.1/24|           |30.1.1.1/24   30.1.1.2/24|           |\n \
    +-----------+                        +-----------+                         +-----------+                         +-----------+\n\n " ,
"2 - Simple Layer 2 Topology \n\n\
                                        +--------+ \n \
                                        |   H4   | \n \
                                        +--------+ \n \
                                            | eth0/0 \n \
                                            | 10.1.1.3/24 \n \
                                            | \n \
                                            | eth0/4 \n \
                                      +-----------+                                    \n \
    +--------+ 10.1.1.1/24            |           |              10.1.1.3/24 +--------+ \n \
    |   H1   |------------------------+    SW01   +--------------------------|   H3   | \n \
    +--------+ eth0/0           eth0/1|           | eth0/3            eth0/0 +--------+ \n \
                                      +-----------+                                    \n \
                                            | eth0/2 \n \
                                            | \n \
                                            |10.1.1.2/24 \n \
                                            | eth0/0 \n \
                                       +--------+ \n \
                                       |   H2   | \n \
                                       +--------+ \n\n " ,
"3 - Dual Switch Technology \n\n\
                                        +--------+                                              +--------+\n \
                                        |   H3   |                                              |   H4   |\n \
                                        +--------+                                              +--------+\n \
                                            | eth0/0                                                | eth0/0\n \
                                            | 10.1.1.3/24                                           | 10.1.1.4/24\n \
                                            |                                                       |\n \
                                            | eth0/3  , VID10                                       | eth0/4  , VID 10\n \
                                      +-----------+                                           +----------+\n \
    +--------+ 10.1.1.1/24       VID10|           | TR VID 10 , 11             TR VID 10 , 11 |          |              10.1.1.6/24 +--------+\n \
    |   H1   |------------------------+    SW01   +===========================================+   SW02   +--------------------------|   H6   |\n \
    +--------+ eth0/0           eth0/1|           | ge0/1                                ge0/1|          | eth0/6 , VID 11   eth0/0 +--------+\n \
                                      +-----------+                                           +----------+\n \
                                            | eth0/2 , VID11                                        | eth0/5  , VID11\n \
                                            |                                                       |\n \
                                            |10.1.1.2/24                                            |10.1.1.5/24\n \
                                            | eth0/0                                                | eth0/0\n \
                                       +--------+                                              +--------+\n \
                                       |   H2   |                                              |   H5   |\n \
                                       +--------+                                              +--------+\n\n",
"4 - Simple Layer 3 Topology \n\n\
                                          .-''-.                                         \n \
                           40.1.1.1/24  /       \\  20.1.1.1/24                           \n \
                +----------------------|   R1    |-----------------------+               \n \
                |                 ge0/2\\       / ge0/3                  |               \n \
                |                         `-..-'                         |               \n \
   40.1.1.2/24  |ge0/1                 Lo : 1.1.1.1                ge0/1 |  20.1.1.3/24  \n \
             .-''-.                                                    .-''-.            \n \
            /      \\ ge0/3                                     ge0/2  /      \\           \n \
           |   R2   |------------------------------------------------|   R3   |          \n \
           \\        / 30.1.1..2/24                       30.1.1.3/24 \\      /           \n \
             `-..-'                                                    `-..-'            \n \
        Lo : 2.2.2.2                                                 Lo : 3.3.3.3        \n\n"
};

extern graph_t *topo;

extern void send_arp_broadcast_request(node_t *node , interface_t *oif , char *ip_addr);
extern void arp_table_dump( arp_table_t* arp_table);
extern void mac_table_dump( mac_table_t* mac_table);
extern void set_node_debug_status(node_t* node ,debug_status_t status);
extern void rt_dump_table(route_table_t *route_table);
extern void rt_table_add_route(route_table_t *route_table , char *dst , uint8_t mask , char *gw_ip , char *oif_name);
extern void send_ping_request(node_t *node , char *ip, uint8_t count);

extern graph_t * build_first_topo();
extern graph_t * build_linear_topo();
extern graph_t * build_simple_l2_switch_topo();
extern graph_t * build_dualswitch_topo();
extern graph_t * build_simple_l3_topo();

static int 
show_nw_topology_handler (
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    int CMDCODE = -1;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buff);

    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }

    switch(CMDCODE){
        case CMDCODE_SHOW_NW_TOPOLOGY:
            dump_nw_graph(topo);
            break;
        default:
            ;
    }
    return 0;
}
static int
show_saved_topologies_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    tlv_struct_t *tlv = NULL;
    int CMDCODE = -1;
    int topology_id;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buff);

    switch(CMDCODE){
        case CMDCODE_SHOW_TOPOLOGIES_SAVED:
            for(int i = 0 ; i <  NUM_SAVED_TOPOS ; i++)
                printf("%s",saved_topologies[i]);
            break;
        case CMDCODE_SHOW_TOPOLOGY_ID:
            TLV_LOOP_BEGIN(tlv_buff , tlv){
                if(strcmp(tlv->leaf_id  , "topology-id") == 0)
                    topology_id = atoi(tlv->value);
                printf("%s",saved_topologies[topology_id]);
            }TLV_LOOP_END;
            break;
        default:
            ;
    }
    return 0;
}
static int
show_node_handler (
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }
    printf("\n");
    dump_nw_node(node);
    return 0;

}
static int
show_node_interfaces(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }
    for(int i = 0 ; i < MAX_INTF_PER_NODE - 1 ; i++){
        interface_t *intf  = node->intf[i];
        if(!intf) continue;
        dump_nw_interface(intf);
    }
    return 0;

}
static int 
show_arp_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){

    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }

    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *ip_address = NULL;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }

    arp_table_dump(node->node_nw_prop.arp_table);
    return 0;

}
static int
show_mac_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }

    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *ip_address = NULL;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }

    mac_table_dump(node->node_nw_prop.mac_table);
    return 0;

}
static int 
show_mode_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }

    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *if_name = NULL;
    node_t* node;
    interface_t* intf;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "interface-name") == 0)
            if_name = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Error : Node %s, not found in topology\n", node_name);
        return -1;
    }
    intf = get_node_if_by_name(node , if_name);
    if(!intf) {
        printf("Error : Node %s, No Interface IF %s\n", node_name , if_name);
        return -1;
    }
    if(IS_INTF_L3_MODE(intf)){
        printf("IF %s is in Layer 3 Mode\n", if_name);
    }
    else{
        printf("IF %s is in %s Mode\n", if_name , intf_l2_mode_str(IF_L2_MODE(intf)));
    }
    return 0;

}
static int 
show_vlans_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }

    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *if_name = NULL;
    node_t* node;
    interface_t* intf;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "interface-name") == 0)
            if_name = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Error : Node %s, not found in topology\n", node_name);
        return -1;
    }
    intf = get_node_if_by_name(node , if_name);
    if(!intf) {
        printf("Error : Node %s, No Interface IF %s\n", node_name , if_name);
        return -1;
    }
    dump_vlan_membership(intf);
    return 0;

}
static int
show_route_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }

    rt_dump_table(node->node_nw_prop.route_table);
    return 0;

}
static int
show_debug_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    debug_status_t status;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }

    printf("Node %s debugging is %s\n" , node->node_name , debug_status_str(node->debug_status) );
    return 0;

}
static int
config_route_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *dest = NULL;
    uint8_t mask ;
    char *gw = NULL;
    char *oif = NULL;
    node_t* node;
    interface_t *intf;
    route_table_t *route_table;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "dest") == 0)
            dest = tlv->value ;
        if(strcmp(tlv->leaf_id  , "mask") == 0)
            mask = (uint8_t)atoi(tlv->value);
        if(strcmp(tlv->leaf_id  , "gw") == 0)
            gw = tlv->value ;
        if(strcmp(tlv->leaf_id  , "oif") == 0)
            oif = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }
    intf = get_node_if_by_name(node , oif);
    if(!intf){
        printf("%s is not a valid interface on %s\n",oif, node_name);
        return -1;
    }
    if(!IS_INTF_L3_MODE(intf)){
        printf("Interface %s on %s is not configured in L3 mode \n",oif, node_name);
        return -1;
    }
    route_table = node->node_nw_prop.route_table;
    if(!route_table){
        printf("Error accessing route table for node %s\n" , node->node_name);
        return -1;
    }
    rt_table_add_route( route_table , dest , mask , gw , oif);
    return 0;

}
static int
config_debug_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }

    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *status = NULL;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "status") == 0)
            status = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }
   
    if( strcmp(status , "on") == 0 || strcmp(status , "ON") == 0 )
        set_node_debug_status(node,DEBUG_ON);
    if( strcmp(status , "off") == 0 || strcmp(status , "OFF") == 0 )
        set_node_debug_status(node,DEBUG_OFF);
    printf("Debug status on node %s set to %s \n" , node->node_name , status);
    return 0;

}
static int 
mode_set_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }

    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *if_name = NULL;
    char *mode = NULL;
    node_t* node;
    interface_t* intf;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "interface-name") == 0)
            if_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "intf_mode") == 0)
            mode = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Error : Node %s, not found in topology\n", node_name);
        return -1;
    }
    intf = get_node_if_by_name(node , if_name);
    if(!intf) {
        printf("Error : Node %s, No Interface IF %s\n", node_name , if_name);
        return -1;
    }
    
    if( strcmp(mode , "access") == 0 || strcmp(mode , "ACCESS") == 0 ){
        node_set_intf_l2_mode(node , if_name , ACCESS);
    }
    else if( strcmp(mode , "trunk") == 0 || strcmp(mode , "TRUNK") == 0 ){
        node_set_intf_l2_mode(node , if_name , TRUNK);
    }
    return 0;

}
static int 
add_vlan_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *if_name = NULL;
    uint16_t vlan_id ;
    node_t* node;
    interface_t* intf;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "interface-name") == 0)
            if_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "vlan-id") == 0)
    printf("vlan_id %s\n" , tlv->value);
            vlan_id = atoi(tlv->value) ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Error : Node %s, not found in topology\n", node_name);
        return -1;
    }
    intf = get_node_if_by_name(node , if_name);
    if(!intf) {
        printf("Error : Node %s, No Interface IF %s\n", node_name , if_name);
        return -1;
    }
    printf("vlan_id %hu\n" , vlan_id);
    node_set_intf_vlan_membership(node , if_name , vlan_id);
    return 0;
}
static int 
remove_vlan_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *if_name = NULL;
    uint16_t vlan_id ;
    node_t* node;
    interface_t* intf;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "interface-name") == 0)
            if_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "vlan-id") == 0)
            vlan_id = atoi(tlv->value) ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Error : Node %s, not found in topology\n", node_name);
        return -1;
    }
    intf = get_node_if_by_name(node , if_name);
    if(!intf) {
        printf("Error : Node %s, No Interface IF %s\n", node_name , if_name);
        return -1;
    }
    node_remove_vlan_membership(node , if_name , vlan_id);
    return 0;
}
static int
resolve_arp_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *ip_address = NULL;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "ip-address") == 0)
            ip_address = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }
    send_arp_broadcast_request(node,NULL,ip_address);
    return 0;

}
static int
ping_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    if(!topo){
        printf("No Topology loaded / created\n");
        return -1;
    }
    tlv_struct_t *tlv = NULL;
    char *node_name = NULL;
    char *ip_address = NULL;
    node_t* node;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "node-name") == 0)
            node_name = tlv->value ;
        if(strcmp(tlv->leaf_id  , "ip-address") == 0)
            ip_address = tlv->value ;
    }TLV_LOOP_END;
    node = get_node_by_node_name(topo , node_name);
    if(!node) {
        printf("Node %s, not found in topology\n", node_name);
        return -1;
    }
    send_ping_request(node , ip_address , 1);
    return 0;

}
static int
load_topology_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    tlv_struct_t *tlv = NULL;
    int topology_id;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "topology-id") == 0)
            topology_id = atoi(tlv->value) ;
    }TLV_LOOP_END;
    switch (topology_id)
    {
    case 0:
        topo = build_first_topo();
        break;
    case 1:
        topo = build_linear_topo();
        break;
    case 2:
        topo = build_simple_l2_switch_topo();
        break;
    case 3:
        topo = build_dualswitch_topo();
        break;
    case 4:
        topo = build_simple_l3_topo();
        break;
    default:
        printf("Invalid Topology Id\n");
        return -1;
        break;
    }
    printf("Topology Loaded.\n");
    return 0;
}
static int
create_topology_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    tlv_struct_t *tlv = NULL;
    char* topology_name;
    TLV_LOOP_BEGIN(tlv_buff , tlv){
        if(strcmp(tlv->leaf_id  , "topology-name") == 0)
            topology_name = tlv->value ;
    }TLV_LOOP_END;
    if(topo)
        memset(topo , 0 , sizeof(graph_t));
    topo = create_new_graph(topology_name);
    printf("Topology Created.\n");
    return 0;
}
static int
validate_topology_id(char* topology_id){
    if( atoi(topology_id) < 0 || atoi(topology_id) >= NUM_SAVED_TOPOS)
        return VALIDATION_FAILED;
    return VALIDATION_SUCCESS;
}
static int
validate_ip_address(char* ip){
    uint32_t binary_prefix;
    int result = inet_pton(AF_INET, ip , &binary_prefix);
    if(result == 1)
        return VALIDATION_SUCCESS;
    return VALIDATION_FAILED;
}
static int
validate_debug_status(char* status){
    if(
        strcmp(status , "on") == 0 ||
        strcmp(status , "ON") == 0 ||
        strcmp(status , "off") == 0 ||
        strcmp(status , "OFF") == 0
        )
        return VALIDATION_SUCCESS;
    return VALIDATION_FAILED;
}
static int
validate_interface_mode(char* mode){
    if(
        strcmp(mode , "access") == 0 ||
        strcmp(mode , "ACCESS") == 0 ||
        strcmp(mode , "trunk") == 0 ||
        strcmp(mode , "TRUNK") == 0
        )
        return VALIDATION_SUCCESS;
        printf("Error : Invalid interface mode \"%s\" provided.\n" , mode);
    return VALIDATION_FAILED;
}static int
validate_mask_handler(char* mask){
    uint8_t _mask = (uint8_t)atoi(mask);
    if((_mask < 0 ) || _mask > 32 ){
        printf("Error : Invalid mask value %hhu entered.\n" , _mask);
        return VALIDATION_FAILED;
        }
    return VALIDATION_SUCCESS;
}
void
nw_init_cli(){

    init_libcli();
    param_t *show = libcli_get_show_hook();
    param_t *debug = libcli_get_debug_hook();
    param_t *config = libcli_get_config_hook();
    param_t *run = libcli_get_run_hook();
    param_t *debug_show = libcli_get_debug_show_hook();
    param_t *root = libcli_get_root();

    //show
    { 
        //topology
        {
            static param_t topology;
            init_param( &topology , CMD , "topology" , show_nw_topology_handler ,0 , INVALID , 0 , "Dump Completed Network Topolgy");
            libcli_register_param(show , &topology);
            set_param_cmd_code(&topology , CMDCODE_SHOW_NW_TOPOLOGY);
            //saved
            {
                static param_t saved;
                init_param( &saved , CMD , "saved" , show_saved_topologies_handler , 0 , INVALID , 0 , "Help : Show topologies saved");
                libcli_register_param(&topology , &saved);
                set_param_cmd_code(&saved , CMDCODE_SHOW_TOPOLOGIES_SAVED);
                // topology-id
                {
                    static param_t topology_id;
                    init_param( &topology_id , LEAF , 0 , show_saved_topologies_handler , validate_topology_id , STRING , "topology-id" , "Help : Topology id");
                    libcli_register_param(&saved , &topology_id);
                    set_param_cmd_code(&topology_id , CMDCODE_SHOW_TOPOLOGY_ID);
                }
            }
        }
        //node 
        {
            static param_t node;
            init_param(&node,CMD,"node",0,0,INVALID,0,"Help : node");
            libcli_register_param(show , &node);
            //<node-name>
            {
                static param_t node_name;
                init_param(&node_name,LEAF,0,show_node_handler,0,STRING,"node-name","Help : Node name");
                libcli_register_param(&node , &node_name);
                set_param_cmd_code(&node_name , CMDCODE_SHOW_NODE);
                //arp
                {
                    static param_t arp;
                    init_param(&arp,CMD,"arp",show_arp_handler,0,INVALID,0,"Show ARP table on node");
                    libcli_register_param(&node_name , &arp);
                    set_param_cmd_code(&arp , CMDCODE_SHOW_NODE_ARP_TABLE);
                }
                //mac-table
                {
                    static param_t mac_table;
                    init_param(&mac_table,CMD,"mac-table",show_mac_handler,0,INVALID,0,"Show MAC table on node");
                    libcli_register_param(&node_name , &mac_table);
                    set_param_cmd_code(&mac_table , CMDCODE_SHOW_NODE_MAC_TABLE);
                } 
                //debug
                {
                    static param_t debug_status;
                    init_param(&debug_status,CMD,"debug",show_debug_handler,0,INVALID,0,"Show debug status of node");
                    libcli_register_param(&node_name , &debug_status);
                    set_param_cmd_code(&debug_status , CMDCODE_SHOW_NODE_DEBUG_STATUS);
                } 
                //interface
                {
                    static param_t interface;
                    init_param(&interface,CMD,"interface",show_node_interfaces,0,INVALID,0,"node interface");
                    libcli_register_param(&node_name , &interface);
                    set_param_cmd_code(&interface , CMDCODE_SHOW_NODE_INTERFACES);
                    //interface-name
                    {
                        static param_t interface_name;
                        init_param(&interface_name,LEAF,0,0,0,STRING,"interface-name","Help : Interface Name");
                        libcli_register_param(&interface , &interface_name);
                        //mode
                        {
                            static param_t mode;
                            init_param(&mode,CMD,"mode",show_mode_handler,0,INVALID,0,"Show Mode of Interface on Node");
                            libcli_register_param(&interface_name , &mode);
                            set_param_cmd_code(&mode , CMDCODE_SHOW_NODE_INTERFACE_MODE);
                        }
                        //vlans
                        {
                            static param_t vlans;
                            init_param(&vlans,CMD,"vlans",show_vlans_handler,0,INVALID,0,"Show Vlan membership of Interface on Node");
                            libcli_register_param(&interface_name , &vlans);
                            set_param_cmd_code(&vlans , CMDCODE_SHOW_NODE_INTERFACE_VLANS);
                        }
                    }
                
                }
                //route
                {
                    static param_t route;
                    init_param(&route,CMD,"route",show_route_handler,0,INVALID,0,"Show Route table on node");
                    libcli_register_param(&node_name , &route);
                    set_param_cmd_code(&route , CMDCODE_SHOW_NODE_ROUTE_TABLE);
                } 
            }
        }
    }
    //config
    {
        //topology
        {
            static param_t topology;
            init_param(&topology,CMD,"topology",0,0,0,0,"Help : Network Topology Confiugration");
            libcli_register_param(config , &topology);
            // load
            {
                static param_t load;
                init_param(&load,CMD,"load",0,0,INVALID,0,"Help : Load Network Topology");
                libcli_register_param(&topology , &load);
                    //topology-id
                    {
                        static param_t topology_id;
                        init_param(&topology_id , LEAF, 0 , load_topology_handler , validate_topology_id, INT , "topology-id" , "Help : ID of topology to load ( show toplogy saved)" );
                        libcli_register_param(&load, &topology_id);
                        set_param_cmd_code(&topology_id , CMDCODE_CONFIG_TOPOLOGY_LOAD);
                    }
            }
            // create
            {
                static param_t create;
                init_param(&create,CMD,"create",0,0,INVALID,0,"Help : Create Network Topology");
                libcli_register_param(&topology , &create);
                    //topology-id
                    {
                        static param_t topology_id;
                        init_param(&topology_id , LEAF, 0 , create_topology_handler , 0, STRING , "topology-name" , "Help : Name of topology to create" );
                        libcli_register_param(&create, &topology_id);
                        set_param_cmd_code(&topology_id , CMDCODE_CONFIG_TOPOLOGY_CREATE);
                    }
                    
            }
        }
        //node
        {
            static param_t node;
            init_param(&node,CMD,"node",0,0,INVALID,0,"Help : node");
            libcli_register_param(config , &node);
            //node-name
            {
                static param_t node_name;
                init_param(&node_name,LEAF,0,0,0,STRING,"node-name","Help : Node name");
                //debug
                {
                    libcli_register_param(&node , &node_name);
                    static param_t debug;
                    init_param(&debug,CMD,"debug",0,0,INVALID,0,"Help : debug");
                    libcli_register_param(&node_name , &debug);
                    // status
                    {
                        static param_t status;
                        init_param(&status,LEAF,0,config_debug_handler,validate_debug_status,STRING,"status","Help : Status (oon or off)");
                        libcli_register_param(&debug , &status);
                        set_param_cmd_code(&status , CMDCODE_CONFIG_NODE_DEBUG);
                    }
                } 
                //interface
                {
                    static param_t interface;
                    init_param(&interface,CMD,"interface",0,0,INVALID,0,"node interface");
                    libcli_register_param(&node_name , &interface);
                    //interface-name
                    {
                        static param_t interface_name;
                        init_param(&interface_name,LEAF,0,0,0,STRING,"interface-name","Help : Interface Name");
                        libcli_register_param(&interface , &interface_name);
                        //mode
                        {
                            static param_t mode;
                            init_param(&mode,CMD,"mode",0,0,INVALID,0,"Show Mode of Interface on Node");
                            libcli_register_param(&interface_name , &mode);
                            //intf_mode
                            {
                                static param_t intf_mode;
                                init_param(&intf_mode,LEAF,0,mode_set_handler,validate_interface_mode,STRING,"intf_mode","Help : Set Interface  mode ");
                                libcli_register_param(&mode , &intf_mode);
                                set_param_cmd_code(&intf_mode , CMDCODE_CONFIG_NODE_MODE);
                            }
                        }
                        //vlans
                        {
                            static param_t vlans;
                            init_param(&vlans,CMD,"vlans",0,0,INVALID,0,"Show Vlan membership of Interface on Node");
                            libcli_register_param(&interface_name , &vlans);
                            //add
                            {
                                static param_t add;
                                init_param(&add,CMD,"add",0,0,INVALID,0,"Help : Add Vlan to Interface ");
                                libcli_register_param(&vlans , &add);
                                //vlan_id
                                {
                                    static param_t vlan_id;
                                    init_param(&vlan_id,LEAF,0,add_vlan_handler,0,INT,"vlan-id","Help : Vlan ID");
                                    libcli_register_param(&add , &vlan_id);
                                    set_param_cmd_code(&vlan_id , CMDCODE_CONFIG_NODE_INTERFACE_VLAN_ADD);
                                }
                            }
                            //remove
                            {
                                static param_t remove;
                                init_param(&remove,CMD,"remove",0,0,INVALID,0,"Help : remove Vlan From Interface ");
                                libcli_register_param(&vlans , &remove);
                                //vlan_id
                                {
                                    static param_t vlan_id;
                                    init_param(&vlan_id,LEAF,0,remove_vlan_handler,0,INT,"vlan-id","Help : Vlan ID");
                                    libcli_register_param(&remove , &vlan_id);
                                    set_param_cmd_code(&vlan_id , CMDCODE_CONFIG_NODE_INTERFACE_VLAN_REMOVE);
                                }
                            }
                        }
                    }
                
                } 
                //route
                {
                    static param_t route;
                    init_param(&route,CMD,"route",0,0,INVALID,0,"node route table");
                    libcli_register_param(&node_name , &route);
                    //dest
                    {
                        static param_t dest;
                        init_param(&dest,LEAF,0,0,0,IPV4,"dest","Help : Destination Network");
                        libcli_register_param(&route , &dest);
                        //mask
                        {
                            static param_t mask;
                            init_param(&mask,LEAF,0,0,validate_mask_handler,INT,"mask","Help : Net Mask (0 - 32 )");
                            libcli_register_param(&dest , &mask);
                            //gw
                            {
                                static param_t gw;
                                init_param(&gw,LEAF,0,0,0,IPV4,"gw","Help : Next hop IP");
                                libcli_register_param(&mask , &gw);
                                // set_param_cmd_code(&gw , CMDCODE_CONFIG_NODE_ROUTE_GW);
                                //oif
                                {
                                    static param_t oif;
                                    init_param(&oif,LEAF,0,config_route_handler,0,STRING,"oif","Help : Egress Interface");
                                    libcli_register_param(&gw , &oif);
                                    set_param_cmd_code(&oif , CMDCODE_CONFIG_NODE_ROUTE_OIF);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //run
    {
        //node
        {
            static param_t node;
            init_param(&node,CMD,"node",0,0,INVALID,0,"Help : node");
            libcli_register_param(run , &node);
            //<node-name>
            {
                static param_t node_name;
                init_param(&node_name,LEAF,0,0,0,STRING,"node-name","Help : Node name");
                libcli_register_param(&node , &node_name);
                 //ping
                {
                    static param_t ping;
                    init_param(&ping,CMD,"ping",0,0,INVALID,0,"Help : ping");
                    libcli_register_param(&node_name , &ping);
                    //<ip-address>
                    {
                        static param_t ip_address;
                        init_param(&ip_address,LEAF,0,ping_handler,validate_ip_address,STRING,"ip-address","Help : IP Address");
                        libcli_register_param(&ping , &ip_address);
                        set_param_cmd_code(&ip_address , CMDCODE_RUN_NODE_PING);
                    }
                }
                //resolve_arp
                {
                    static param_t resolve_arp;
                    init_param(&resolve_arp,CMD,"resolve-arp",0,0,INVALID,0,"Help : resolve-arp");
                    libcli_register_param(&node_name , &resolve_arp);
                    //<ip-address>
                    {
                        static param_t ip_address;
                        init_param(&ip_address,LEAF,0,resolve_arp_handler,validate_ip_address,STRING,"ip-address","Help : IP Address");
                        libcli_register_param(&resolve_arp , &ip_address);
                        set_param_cmd_code(&ip_address , CMDCODE_RUN_NODE_RESOLVE_ARP);
                    }
                }
            }
        } 
    }
}