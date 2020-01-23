#ifndef __NET_H__
#define __NET_H__

#include <string.h>
#include <stdint.h>
#include "utils.h"

#define MAX_VLAN_MEMBERSHIP (uint16_t)10

typedef struct graph_ graph_t;
typedef struct interface_ interface_t;
typedef struct node_ node_t;
typedef struct arp_table_ arp_table_t;
typedef struct mac_table_ mac_table_t;

extern void init_arp_table(arp_table_t** arp_table);
extern void init_mac_table(mac_table_t** mac_table);
// structure for IP addess in form
// xxx.xxx.xxx
typedef struct ip_add_ {
    char ip_addr[16];
} ip_add_t;

//stutcure for MAC address in from 
// xx:xx:xx:xx:xx:xx
typedef struct mac_add_{
    unsigned char mac[6];
}mac_addr_t;

typedef struct node_nw_prop_{
    //Layer 2 metwork properties
    arp_table_t *arp_table;
    mac_table_t *mac_table;

    //Layer 3 network properties
    bool_t is_lb_addr_config ;
    ip_add_t lb_addr; /*loopback address of node*/
} node_nw_prop_t;

static inline void
init_node_nw_prop(node_nw_prop_t* node_nw_prop){
    node_nw_prop->is_lb_addr_config = FALSE;
    memset(node_nw_prop->lb_addr.ip_addr , 0 , sizeof(node_nw_prop->lb_addr.ip_addr));
    init_arp_table(&(node_nw_prop->arp_table));
    init_mac_table(&(node_nw_prop->mac_table));
}

typedef enum {
    ACCESS,
    TRUNK,
    L2_MODE_UNKNOWN
} intf_l2_mode_t;

static inline char *
intf_l2_mode_str(intf_l2_mode_t intf_l2_mode){
    switch(intf_l2_mode){
        case ACCESS :
            return "access";
        case TRUNK :
            return "trunk";
        default :
            return "L2_MODE_UNKNOWN";
    }
}
typedef struct intf_nw_props{
    //L2 Properties
    mac_addr_t mac_add;
    intf_l2_mode_t intf_l2_mode;
    uint16_t vlans[MAX_VLAN_MEMBERSHIP];

    //Layer 3 network properties
    bool_t is_ipadd_config ;
    ip_add_t ip_add; /*loopback address of node*/
    char mask;
}intf_nw_props_t;

static inline void
init_intf_nw_prop(intf_nw_props_t* intf_nw_props){
    memset(intf_nw_props->mac_add.mac , 0 , sizeof(intf_nw_props->mac_add.mac));

    intf_nw_props->is_ipadd_config = FALSE;
    memset(intf_nw_props->ip_add.ip_addr , 0 , sizeof(intf_nw_props->ip_add.ip_addr));
    intf_nw_props->mask = 0;

    intf_nw_props->intf_l2_mode = ACCESS;
}

//Macros
#define IF_MAC(intf_ptr)    ((intf_ptr)->intf_nw_props.mac_add.mac)
#define IF_IP(intf_ptr)    ((intf_ptr)->intf_nw_props.ip_add.ip_addr)
#define IF_L2_MODE(intf_ptr)    ((intf_ptr)->intf_nw_props.intf_l2_mode)
#define NODE_LO_ADDR(node_ptr) ((node_ptr)->node_nw_prop.lb_addr.ip_addr )
#define IS_INTF_L3_MODE(intf_ptr) ((intf_ptr)->intf_nw_props.is_ipadd_config == TRUE)

//APIs
bool_t node_set_loopback_address(node_t* node, char* ip_addr);
bool_t node_set_intf_ip_address(node_t* node, char* local_if, char* ip_addr , char mask);
bool_t node_unset_intf_ip_address(node_t* node, char* local_if);
void interface_assign_mac_address(interface_t *interface);
interface_t* node_get_matching_subnet_interface(node_t* node , char* ip_addr);
void dump_nw_graph(graph_t* graph);
void dump_nw_node(node_t* node);
void dump_nw_interface(interface_t* intf);

unsigned int convert_ip_from_str_to_int(char *ip_addr);
void convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer);

void node_set_intf_l2_mode( node_t *node , char *if_name , intf_l2_mode_t intf_l2_mode);
void node_set_intf_vlan_membership(node_t *node , char *if_name , uint16_t vlan_id);
void node_remove_vlan_membership(node_t *node , char *if_name , uint16_t vlan_id);

#endif