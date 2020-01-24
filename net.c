#include "net.h"
#include "graph.h"
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static unsigned int 
hash_code(void* ptr, unsigned int size){
    unsigned int value = 0 , i = 0;
    char* str = (char*)ptr;
    while(i < size){
        value += *str;
        value *= 97;
        str++;
        i++;
    }
    return value;
}
void interface_assign_mac_address(interface_t *interface){
    node_t* node = interface->att_node;
    if(!node) return;

    unsigned int hash_code_val = 0;
    hash_code_val = hash_code(node->node_name,NODE_NAME_SIZE);
    hash_code_val *= hash_code(interface->if_name , IF_NAME_SIZE);
    memset(IF_MAC(interface) , 0 , sizeof(IF_MAC(interface)));
    memcpy(IF_MAC(interface) , (char*) &hash_code_val , sizeof(IF_MAC(interface)));

}
bool_t node_set_loopback_address(node_t* node, char* ip_addr){
    node->node_nw_prop.is_lb_addr_config = TRUE;
    strcpy(node->node_nw_prop.lb_addr.ip_addr , ip_addr);
    return TRUE;
}
bool_t node_set_intf_ip_address(node_t* node, char* local_if, char* ip_addr , char mask){
    interface_t* intf = get_node_if_by_name(node , local_if);
    if(!intf) return FALSE;
    intf->intf_nw_props.is_ipadd_config = TRUE;
    intf->intf_nw_props.mask = mask;
    intf->intf_nw_props.intf_l2_mode = L2_MODE_UNKNOWN;
    strcpy(IF_IP(intf),ip_addr);
    return TRUE;

}
bool_t node_unset_intf_ip_address(node_t* node, char* local_if){
    interface_t* intf = get_node_if_by_name(node , local_if);
    if(!intf) return FALSE;
    intf->intf_nw_props.is_ipadd_config = FALSE;
    memset(intf->intf_nw_props.ip_add.ip_addr , 0 , 16);
    intf->intf_nw_props.mask = 0;
    return TRUE;
}
void dump_nw_graph(graph_t* graph){

    printf("Topology Name : %s\n" , graph->topolgy_name);
    printf("==========================================\n");
    
    node_t *node;
    glthread_t *curr;

    ITERATE_GLTHREAD_BEGIN(&graph->node_list, curr){
        node = graph_glue_to_node(curr);
        dump_nw_node(node);

    } ITERATE_GLTHREAD_END(&topo->node_list, curr);
}
void dump_nw_node(node_t* node){
    printf("Node Name : %s\n" , node->node_name);
    char* lo_addr = strcat(NODE_LO_ADDR(node) , "/32") ? node->node_nw_prop.lb_addr.ip_addr : "null";
    printf("\t lo addr : %s \n" ,lo_addr);
    for(int intf_idx = 0 ; intf_idx < MAX_INTF_PER_NODE - 1 ; intf_idx ++){
        interface_t* intf = node->intf[intf_idx];
        if(!intf) break;
        dump_nw_interface(intf);
    }
    printf("\n");
}
void dump_vlan_membership(interface_t *intf){
    if(IF_L2_MODE(intf) != L2_MODE_UNKNOWN ) {
        printf("VLAN Membership : [ ");
        for(uint16_t i = 0 ; i < MAX_VLAN_MEMBERSHIP ; i++){
            uint16_t vlan = intf->intf_nw_props.vlans[i];
            if(vlan) printf("%hu ", vlan);
        }
        printf("]\n");
    }
}
void dump_nw_interface(interface_t* intf){
    node_t* nbr = get_nbr_node(intf);
    char ip_subnet[16];
    memset(ip_subnet , 0 ,16);
    char mask = intf->intf_nw_props.mask;
    if(intf->intf_nw_props.is_ipadd_config){
        apply_mask(IF_IP(intf) , mask , ip_subnet);
    };
    printf("Interface Name : %s (%s)\n " , intf->if_name , intf_l2_mode_str(intf->intf_nw_props.intf_l2_mode));
    dump_vlan_membership(intf);
    printf("\tNbr Node %s, Local Node : %s , cost = %d\n" , nbr->node_name , intf->if_name , intf->link->cost );
    printf("\tIP Addr : %s(%s/%d )  MAC : %02x:%02x:%02x:%02x:%02x:%02x \n" , IF_IP(intf), ip_subnet  ,mask ,  IF_MAC(intf)[0] , IF_MAC(intf)[1] , IF_MAC(intf)[2] , IF_MAC(intf)[3] , IF_MAC(intf)[4] , IF_MAC(intf)[5]);
}
interface_t*
node_get_matching_subnet_interface(node_t* node , char* ip_addr){
     char ip_subnet[16];
     char if_subnet[16];
     for(int i = 0 ; i < MAX_INTF_PER_NODE -1 ; i++){
        interface_t* intf = node->intf[i];
        if(!intf) return NULL;
        if(!intf->intf_nw_props.is_ipadd_config) continue;
        char mask = intf->intf_nw_props.mask;
        memset(ip_subnet, 0 , 16);
        memset(if_subnet, 0 , 16);
        apply_mask(ip_addr, mask , ip_subnet);
        apply_mask(IF_IP(intf) ,  mask , if_subnet);
        if(strcmp(if_subnet , ip_subnet) == 0) return intf;
     }
     return NULL;
 };

unsigned int
convert_ip_from_str_to_int(char *ip_addr){
    unsigned int binary_prefix = 0;
    inet_pton(AF_INET , ip_addr , &binary_prefix);
    return binary_prefix;
};
void
convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer){
    inet_ntop(AF_INET, &ip_addr, output_buffer, INET_ADDRSTRLEN );
};
void 
node_set_intf_l2_mode( node_t *node , char *if_name , intf_l2_mode_t intf_l2_mode){
    interface_t *intf = get_node_if_by_name(node, if_name);
    // not a valid interface
    if(!intf) {
        printf("Error : , Node : %s - Unable to set L2 mode. %s not valid IF name\n" , node->node_name , if_name);
        return;
    }

    intf_l2_mode_t curr_l2_mode = intf->intf_nw_props.intf_l2_mode;
    // interface already configured to desired mode
    if(curr_l2_mode == intf_l2_mode)    return;
    //interface configured in L3 mode
    if(IS_INTF_L3_MODE(intf)){
        printf("Unable to configure interface %s in %s mode. Interface has IP configured\n" , if_name , intf_l2_mode_str(intf_l2_mode));
        return;
    }
    //interface configured in Trunk mode and setting to Access
    if(IF_L2_MODE(intf) == TRUNK && intf_l2_mode == ACCESS){
        printf("Info : Changing Interface from trunk to acces mode , removing vlan members\n");
        memset( (uint16_t *)(intf->intf_nw_props.vlans) + 1  , 0 , sizeof(uint16_t)*(MAX_VLAN_MEMBERSHIP - 1) );
    }
    IF_L2_MODE(intf) = intf_l2_mode;
}
void
add_vlan_to_trunk_membership(interface_t *intf, uint16_t vlan_id){
    uint16_t slot = 0;
    int32_t empty_slot = -1;
    for(; slot < MAX_VLAN_MEMBERSHIP ;slot++){
        if(vlan_id == intf->intf_nw_props.vlans[slot]) {
            printf("Info : Node %s - Interface %s is already a member of VLAN %hu\n" , intf->att_node->node_name , intf->if_name , vlan_id);
            return;
        }else{
            if(!intf->intf_nw_props.vlans[slot] && empty_slot == -1){
                empty_slot = slot;
            }
        }
    }
    if(empty_slot != -1 ){
        printf("Info : Node %s - Adding VLAN %hu to Interface %s \n" , intf->if_name , vlan_id, intf->att_node->node_name );
        intf->intf_nw_props.vlans[empty_slot] = vlan_id;
        return;
    }
    printf("Error : Node %s - Unable to set VLAN on IF %s , Max VLAN membership  %hu reached \n", intf->att_node->node_name , intf->if_name , MAX_VLAN_MEMBERSHIP);

}
void
interface_set_vlan(node_t *node , interface_t *intf , uint16_t vlan_id){
    if(IS_INTF_L3_MODE(intf)){
        printf("Error : Node %s - Unable to set VLAN on IF %s , Interface in  L3 mode\n" , node->node_name , intf->if_name);
        return;
    }
    if(IF_L2_MODE(intf) == L2_MODE_UNKNOWN){
        printf("Error : Node %s - Unable to set VLAN on IF %s , Interface in unknown L2 mode\n", node->node_name , intf->if_name);
        return;
    }
    if(IF_L2_MODE(intf) == ACCESS){
        printf("Info - Setting Interface vlan to %hu\n" , vlan_id);
        intf->intf_nw_props.vlans[0] = vlan_id;
        return;
    }
    if(IF_L2_MODE(intf) == TRUNK){
        add_vlan_to_trunk_membership(intf , vlan_id);
        return;
    }}
void
node_set_intf_vlan_membership(node_t *node , char *if_name , uint16_t vlan_id){
    interface_t *intf = get_node_if_by_name(node,if_name);
    if(!intf)  {
        printf("Error : Node : %s - Unable to set VLAN Membership on Interface. %s not valid IF name\n" , node->node_name , if_name);
        return;
    }
    interface_set_vlan(node , intf , vlan_id);
}
void
node_remove_vlan_membership(node_t *node , char *if_name , uint16_t vlan_id){
    interface_t *intf = get_node_if_by_name(node,if_name);
    if(!intf)  {
        printf("Error : Node : %s - Unable to remove VLAN Membership on Interface. %s not valid IF name\n" , node->node_name , if_name);
        return;
    }
    if(IF_L2_MODE(intf) == L2_MODE_UNKNOWN){
        printf("Error : Node : %s - Unable to remove VLAN Membership on Interface %s , Unknown L2 Mode\n" , node->node_name , if_name);
        return;
    }
    uint16_t slot = 0;
    for( ; slot < MAX_VLAN_MEMBERSHIP ; slot++){
        uint16_t vlan = intf->intf_nw_props.vlans[slot];
        if( vlan == vlan_id ) {
            printf("Info : Node :%s - Removing VLAN %hu from IF %s membership\n" , node->node_name , vlan_id , if_name);
            memset( (uint16_t *)(intf->intf_nw_props.vlans) + slot ,  0 , sizeof(uint16_t));
            return;
        }
    }
    printf("Error : Node : %s - Unable to remove VLAN Membership on Interface %s , VLAN %hu is not a member of interface\n" , node->node_name , if_name , vlan_id);
    return;
}
uint16_t
get_access_intf_operating_vlan_id(interface_t *intf){
    if(IF_L2_MODE(intf) != ACCESS) {
        printf("Error : Unable to get access Vlan , Interface %s is not in access mode \n" , intf->if_name);
        return 0;
    }
    return intf->intf_nw_props.vlans[0];
}
bool_t
is_trunk_interface_vlan_member(interface_t *intf,uint16_t vlan_id){
    if(IF_L2_MODE(intf) != TRUNK) {
        printf("Error :  Interface %s is not in trunk mode \n" , intf->if_name);
        return 0;
    }
    for( uint16_t i = 0 ; i < MAX_VLAN_MEMBERSHIP ; i ++){
        if(vlan_id == intf->intf_nw_props.vlans[i]) 
        return TRUE;
    }
    return FALSE;
};