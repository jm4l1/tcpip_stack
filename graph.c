#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

graph_t*
create_new_graph(char* topology_name){
    graph_t* graph = calloc(1 , sizeof(graph_t));
    strncpy(graph->topolgy_name , topology_name , 32);
    graph->topolgy_name[31] = '\0';

    init_glthread(&graph->node_list);
    return graph ;
}

node_t*
create_graph_node(graph_t* graph, char* node_name){
    node_t* node = calloc(1, sizeof(node_t));
    strcpy(node->node_name,node_name);
    init_node_nw_prop(&node->node_nw_prop);
    init_glthread(&node->graph_glue);
    glthread_add_next(&graph->node_list,&node->graph_glue);
    return node;
}

void 
insert_link_between_two_nodes(node_t* node1 , node_t* node2 ,char* from_if_name , char* to_if_name, unsigned int cost){
    //check that each node has a slot available
    int node1_slot = get_node_intf_available_slot(node1);
    int node2_slot = get_node_intf_available_slot(node2);

    if((node1_slot == -1) || (node2_slot == -1) )
    {
        printf("Unable to create new link between %s %s and %s %s\n" , node1->node_name  , from_if_name , node2->node_name , to_if_name);
        return;
    }
    //creat link 
    link_t* link = calloc(1,sizeof(link_t));
    // Set interface pointer on links
    strncpy(link->intf1.if_name , from_if_name , IF_NAME_SIZE);
    link->intf1.if_name[IF_NAME_SIZE-1] = '\0';
    strncpy(link->intf2.if_name , to_if_name , IF_NAME_SIZE);
    link->intf2.if_name[IF_NAME_SIZE-1] = '\0';


    //Set back pointers of interface structure to the link
    link->intf1.link = link;
    link->intf2.link = link;

    // associate the nodes to each end of the link
    link->intf1.att_node = node1;
    link->intf2.att_node = node2;

    //set link cost
    link->cost = cost;

    //add interface to list on node
    node1->intf[node1_slot] = &link->intf1;
    node2->intf[node2_slot] = &link->intf2;
    
    //Initialise interface properties
    init_intf_nw_prop(&link->intf1.intf_nw_props);
    init_intf_nw_prop(&link->intf2.intf_nw_props);
    interface_assign_mac_address(&link->intf1);
    interface_assign_mac_address(&link->intf2);

}

void 
dump_graph(graph_t* graph){
    printf("Topology Name : %s\n" , graph->topolgy_name);
    printf("==========================================\n");
    
    node_t *node;
    glthread_t *curr;

    ITERATE_GLTHREAD_BEGIN(&graph->node_list, curr){
        node = graph_glue_to_node(curr);
        dump_node(node);
    } ITERATE_GLTHREAD_END(&topo->node_list, curr);
};
void dump_node(node_t* node){
    printf("Node Name : %s\n" , node->node_name);
    for(int intf_idx = 0 ; intf_idx < MAX_INTF_PER_NODE - 1 ; intf_idx ++){
        interface_t* intf = node->intf[intf_idx];
        if(!intf) break;
        dump_interface(intf);
    }
}
void dump_interface(interface_t* intf){
        link_t* link = intf->link;
        node_t* nbr_node = get_nbr_node(intf);
        printf("Local Node : %s , Interface Name : %s , Nbr Node : %s , cost : %d\n", intf->att_node->node_name , intf->if_name , nbr_node->node_name , intf->link->cost);
};

void delete_graph(graph_t* graph){
    free(graph);
}