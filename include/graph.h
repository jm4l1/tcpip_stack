/*
 * =====================================================================================
 *
 *       Filename:  graph.h
 *
 *    Description:  This file contains the definition of all structures required to create a NetworkGraph
 *
 *        Version:  1.0
 *        Created:  Wednesday 18 September 2019 02:17:17  IST
 *       Revision:  1.0
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Networking Developer (AS), sachinites@gmail.com
 *        Company:  Brocade Communications(Jul 2012- Mar 2016), Current : Juniper Networks(Apr 2017 - Present)
 *        
 *        This file is part of the NetworkGraph distribution (https://github.com/sachinites).
 *        Copyright (c) 2017 Abhishek Sagar.
 *        This program is free software: you can redistribute it and/or modify
 *        it under the terms of the GNU General Public License as published by  
 *        the Free Software Foundation, version 3.
 *
 *        This program is distributed in the hope that it will be useful, but 
 *        WITHOUT ANY WARRANTY; without even the implied warranty of 
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 *        General Public License for more details.
 *
 *        You should have received a copy of the GNU General Public License 
 *        along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */
#ifndef __GRAPH__
#define __GRAPH__

#include "glthread.h"
#include <string.h>
#include <stdio.h>
#include "net.h"
#include <stdlib.h>

#define NODE_NAME_SIZE 16
#define IF_NAME_SIZE 16
#define MAX_INTF_PER_NODE 10

// #define ANSI_COLOR_RED     "\x1b[31m"
// #define ANSI_COLOR_GREEN   "\x1b[32m"
// #define ANSI_COLOR_YELLOW  "\x1b[33m"
// #define ANSI_COLOR_BLUE    "\x1b[34m"
// #define ANSI_COLOR_MAGENTA "\x1b[35m"
// #define ANSI_COLOR_CYAN    "\x1b[36m"
// #define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct link_ link_t;
typedef struct node_ node_t;

typedef struct interface_{
    char if_name[IF_NAME_SIZE];
    struct node_ *att_node;
    struct link_ *link;
    intf_nw_props_t intf_nw_props;
} interface_t;

struct link_ {
    interface_t intf1;
    interface_t intf2;
    unsigned int cost;
};

typedef enum debug_status {
    DEBUG_ON ,
    DEBUG_OFF
}debug_status_t;
struct node_ {
    char node_name[NODE_NAME_SIZE];
    interface_t *intf[MAX_INTF_PER_NODE];
    glthread_t graph_glue;
    node_nw_prop_t node_nw_prop;
    int udp_sock_fd;
    unsigned int udp_port_number;
    debug_status_t debug_status;
};

GLTHREAD_TO_STRUCT(graph_glue_to_node, node_t, graph_glue);

typedef struct graph_ {
    char topolgy_name[32];
    glthread_t node_list;
} graph_t;

static inline node_t*
get_nbr_node(interface_t *interface){
    link_t* link = interface->link;

    if(&link->intf1 != interface)
        return link->intf1.att_node;
    return link->intf2.att_node;
};

static inline int
get_node_intf_available_slot(node_t* node){
    int index = 0;
    while( index < (MAX_INTF_PER_NODE - 1)){
        if(!node->intf[index])
            return index;
        index++;
     }
     return -1;
};

static inline interface_t*
get_node_if_by_name(node_t* node , char* if_name){
    for (int intf_index = 0 ; intf_index < (MAX_INTF_PER_NODE - 1); intf_index++){
        if(!node->intf[intf_index]) return NULL;
        if(strcmp(node->intf[intf_index]->if_name , if_name) == 0)
            return node->intf[intf_index];
    }
    return NULL;
}
static char* debug_status_str(debug_status_t status){
    switch(status){
        case DEBUG_ON :
            return "on";
        case DEBUG_OFF :
            return "off";
        default :
            return "unknown";
    }
}
static inline void 
set_node_debug_status(node_t* node ,debug_status_t status){
    node->debug_status = status;
    printf("Info : %s - debug %s\n" , node->node_name , debug_status_str(status));
}
static inline node_t*
get_node_by_node_name(graph_t* topo , char* node_name){
    node_t *node;
    glthread_t *curr;    

    ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr){

        node = graph_glue_to_node(curr);
        if(strncmp(node->node_name, node_name, strlen(node_name)) == 0)
            return node;
    } ITERATE_GLTHREAD_END(&topo->node_list, curr);
    return NULL;
}
graph_t*
create_new_graph(char* topology_name);

node_t*
create_graph_node(graph_t* graph, char* node_name);

void insert_link_between_two_nodes(node_t* node1 , node_t* node2 ,char* from_if_name , char* to_if_name, unsigned int cost);

void dump_graph(graph_t* graph);
void dump_node(node_t* node);
void dump_interface(interface_t* interface);
void delete_graph(graph_t* graph);

#endif