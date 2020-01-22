#include "lib/CommandParser/libcli.h"
#include "lib/CommandParser/cmdtlv.h"
#include "cmdcodes.h"
#include "graph.h"
#include <stdio.h>

extern graph_t *topo;

extern void send_arp_broadcast_rquest(node_t *node , interface_t *oif , char *ip_addr);
extern void arp_table_dump( arp_table_t* arp_table);
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

    {
        //node <node-name> arp
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
            libcli_register_param(show , &node);{
                //<node-name>
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
                {
                    //arp
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
            {
                //<node-name>
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
                {
                    //resolve_arp
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
                    {
                        //<ip-address>
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
            }
        } 
    }
    support_cmd_negation(config);

}