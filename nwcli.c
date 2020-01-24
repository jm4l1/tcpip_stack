#include "lib/CommandParser/libcli.h"
#include "lib/CommandParser/cmdtlv.h"
#include "lib/CommandParser/css.h"
#include "cmdcodes.h"
#include "graph.h"
#include <stdio.h>

extern graph_t *topo;

extern void send_arp_broadcast_rquest(node_t *node , interface_t *oif , char *ip_addr);
extern void arp_table_dump( arp_table_t* arp_table);
extern void mac_table_dump( mac_table_t* mac_table);
extern void set_node_debug_status(node_t* node ,debug_status_t status);

static int 
show_nw_topology_handler (
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
    int CMDCODE = -1;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buff);

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
show_node_handler (
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
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
show_debug_status_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
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
debug_status_handler(
    param_t* param,                 //parameter passed to handler call back
    ser_buff_t * tlv_buff,          // tlv structure described param
    op_mode enable_or_disable       // is command enable or disable function
){
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
    send_arp_broadcast_rquest(node,NULL,ip_address);
    return 0;

}
static int
validate_ip_address(char* ip){
    return VALIDATION_SUCCESS;
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

    //show commands
    {
        // -> topology
        static param_t topology;
        init_param( &topology , CMD , "topology" , show_nw_topology_handler ,0 , INVALID , 0 , "Dump Completed Network Topolgy");
        libcli_register_param(show , &topology);
        set_param_cmd_code(&topology , CMDCODE_SHOW_NW_TOPOLOGY);
    }
    //node 
    {
        static param_t node;
        init_param(&node,
                    CMD,
                    "node",
                    0,
                    0,
                    INVALID,
                    0,
                    "Help : node"
                );
        libcli_register_param(show , &node);
        //<node-name>
        {
            static param_t node_name;
            init_param(&node_name,
                    LEAF,
                    0,
                    show_node_handler,
                    0,
                    STRING,
                    "node-name",
                    "Help : Node name"
            );
            libcli_register_param(&node , &node_name);
             set_param_cmd_code(&node_name , CMDCODE_SHOW_NODE);
            //arp
            {
                static param_t arp;
                init_param(&arp,
                            CMD,
                            "arp",
                            show_arp_handler,
                            0,
                            INVALID,
                            0,
                            "Show ARP table on node"
                        );
                libcli_register_param(&node_name , &arp);
                set_param_cmd_code(&arp , CMDCODE_SHOW_NODE_ARP_TABLE);
            }
            //mac-table
            {
                static param_t mac_table;
                init_param(&mac_table,
                            CMD,
                            "mac-table",
                            show_mac_handler,
                            0,
                            INVALID,
                            0,
                            "Show MAC table on node"
                        );
                libcli_register_param(&node_name , &mac_table);
                set_param_cmd_code(&mac_table , CMDCODE_SHOW_NODE_MAC_TABLE);
            } 
            //debug-status
            {
                static param_t debug_status;
                init_param(&debug_status,
                            CMD,
                            "debug-status",
                            show_debug_status_handler,
                            0,
                            INVALID,
                            0,
                            "Show debug status of node"
                        );
                libcli_register_param(&node_name , &debug_status);
                set_param_cmd_code(&debug_status , CMDCODE_SHOW_NODE_DEBUG_STATUS);
            } 
            //interface
            {
                static param_t interface;
                init_param(&interface,
                            CMD,
                            "interface",
                            show_node_interfaces,
                            0,
                            INVALID,
                            0,
                            "node interface"
                        );
                libcli_register_param(&node_name , &interface);
                set_param_cmd_code(&interface , CMDCODE_SHOW_NODE_INTERFACES);
                //interface-name
                {
                    static param_t interface_name;
                    init_param(&interface_name,
                            LEAF,
                            0,
                            0,
                            0,
                            STRING,
                            "interface-name",
                            "Help : Interface Name"
                    );
                    libcli_register_param(&interface , &interface_name);
                    //mode
                    {
                        static param_t mode;
                        init_param(&mode,
                                    CMD,
                                    "mode",
                                    show_mode_handler,
                                    0,
                                    INVALID,
                                    0,
                                    "Show Mode of Interface on Node"
                                );
                        libcli_register_param(&interface_name , &mode);
                        set_param_cmd_code(&mode , CMDCODE_SHOW_NODE_INTERFACE_MODE);
                    }
                    //vlans
                    {
                        static param_t vlans;
                        init_param(&vlans,
                                    CMD,
                                    "vlans",
                                    show_vlans_handler,
                                    0,
                                    INVALID,
                                    0,
                                    "Show Vlan membership of Interface on Node"
                                );
                        libcli_register_param(&interface_name , &vlans);
                        set_param_cmd_code(&vlans , CMDCODE_SHOW_NODE_INTERFACE_VLANS);
                    }
                }
            
            }
            //route
            {
                static param_t route;
                init_param(&route,
                            CMD,
                            "route",
                            show_route_handler,
                            0,
                            INVALID,
                            0,
                            "Show Route table on node"
                        );
                libcli_register_param(&node_name , &route);
                set_param_cmd_code(&route , CMDCODE_SHOW_NODE_ROUTE_TABLE);
            } 
        }
    }

    //run commands
    {
        // -> node <node-name> resolve-arp <ip-address>
        {
            //node
            static param_t node;
            init_param(&node,
                        CMD,
                        "node",
                        0,
                        0,
                        INVALID,
                        0,
                        "Help : node"
                    );
            libcli_register_param(run , &node);
            //<node-name>
            {
                static param_t node_name;
                init_param(&node_name,
                        LEAF,
                        0,
                        0,
                        0,
                        STRING,
                        "node-name",
                        "Help : Node name"
                );
                libcli_register_param(&node , &node_name);
                //debug_status
                {
                    static param_t debug_status;
                    init_param(&debug_status,
                                CMD,
                                "debug-status",
                                0,
                                0,
                                INVALID,
                                0,
                                "Help : debug-status"
                            );
                    libcli_register_param(&node_name , &debug_status);
                    // status
                    {
                        static param_t status;
                        init_param(&status,
                                LEAF,
                                0,
                                debug_status_handler,
                                validate_debug_status,
                                STRING,
                                "status",
                                "Help : Status (oon or off)"
                        );
                        libcli_register_param(&debug_status , &status);
                        set_param_cmd_code(&status , CMDCODE_RUN_NODE_DEBUG_STATUS);
                    }
                }
                //resolve_arp
                {
                    static param_t resolve_arp;
                    init_param(&resolve_arp,
                                CMD,
                                "resolve-arp",
                                0,
                                0,
                                INVALID,
                                0,
                                "Help : resolve-arp"
                            );
                    libcli_register_param(&node_name , &resolve_arp);
                    //<ip-address>
                    {
                        static param_t ip_address;
                        init_param(&ip_address,
                                LEAF,
                                0,
                                resolve_arp_handler,
                                validate_ip_address,
                                STRING,
                                "ip-address",
                                "Help : IP Address"
                        );
                        libcli_register_param(&resolve_arp , &ip_address);
                        set_param_cmd_code(&ip_address , CMDCODE_RUN_NODE_RESOLVE_ARP);
                    }
                } 
                //interface
                {
                    static param_t interface;
                    init_param(&interface,
                                CMD,
                                "interface",
                                0,
                                0,
                                INVALID,
                                0,
                                "node interface"
                            );
                    libcli_register_param(&node_name , &interface);
                    //interface-name
                    {
                        static param_t interface_name;
                        init_param(&interface_name,
                                LEAF,
                                0,
                                0,
                                0,
                                STRING,
                                "interface-name",
                                "Help : Interface Name"
                        );
                        libcli_register_param(&interface , &interface_name);
                        //mode
                        {
                            static param_t mode;
                            init_param(&mode,
                                        CMD,
                                        "mode",
                                        0,
                                        0,
                                        INVALID,
                                        0,
                                        "Show Mode of Interface on Node"
                                    );
                            libcli_register_param(&interface_name , &mode);
                            //intf_mode
                            {
                                static param_t intf_mode;
                                init_param(&intf_mode,
                                        LEAF,
                                        0,
                                        mode_set_handler,
                                        validate_interface_mode,
                                        STRING,
                                        "intf_mode",
                                        "Help : Set Interface  mode "
                                );
                                libcli_register_param(&mode , &intf_mode);
                                set_param_cmd_code(&intf_mode , CMDCODE_RUN_NODE_MODE);
                            }
                        }
                        //vlans
                        {
                            static param_t vlans;
                            init_param(&vlans,
                                        CMD,
                                        "vlans",
                                        0,
                                        0,
                                        INVALID,
                                        0,
                                        "Show Vlan membership of Interface on Node"
                                    );
                            libcli_register_param(&interface_name , &vlans);
                            //add
                            {
                                static param_t add;
                                init_param(&add,
                                        CMD,
                                        "add",
                                        0,
                                        0,
                                        INVALID,
                                        0,
                                        "Help : Add Vlan to Interface "
                                );
                                libcli_register_param(&vlans , &add);
                                //vlan_id
                                static param_t vlan_id;
                                init_param(&vlan_id,
                                        LEAF,
                                        0,
                                        add_vlan_handler,
                                        0,
                                        INT,
                                        "vlan-id",
                                        "Help : Vlan ID"
                                );
                                libcli_register_param(&add , &vlan_id);
                                set_param_cmd_code(&vlan_id , CMDCODE_RUN_NODE_INTERFACE_VLAN_ADD);
                            }
                            //remove
                            {
                                static param_t remove;
                                init_param(&remove,
                                        CMD,
                                        "remove",
                                        0,
                                        0,
                                        INVALID,
                                        0,
                                        "Help : remove Vlan From Interface "
                                );
                                libcli_register_param(&vlans , &remove);
                                //vlan_id
                                static param_t vlan_id;
                                init_param(&vlan_id,
                                        LEAF,
                                        0,
                                        remove_vlan_handler,
                                        0,
                                        INT,
                                        "vlan-id",
                                        "Help : Vlan ID"
                                );
                                libcli_register_param(&remove , &vlan_id);
                                set_param_cmd_code(&vlan_id , CMDCODE_RUN_NODE_INTERFACE_VLAN_REMOVE);
                            }
                        }
                    }
                
                }
            }
        } 
    }
    support_cmd_negation(config);

}