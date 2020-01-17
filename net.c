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
    memcpy(IF_MAC(interface) , (char*) &hash_code_val , sizeof(unsigned int));

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
void dump_nw_interface(interface_t* intf){
    node_t* nbr = get_nbr_node(intf);
    char ip_subnet[16];
    memset(ip_subnet , 0 ,16);
    char mask = intf->intf_nw_props.mask;
    if(intf->intf_nw_props.is_ipadd_config){
        apply_mask(IF_IP(intf) , mask , ip_subnet);
    };
    printf("Interface Name : %s\n" , intf->if_name );
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

