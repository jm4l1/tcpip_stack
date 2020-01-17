#include "graph.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

static unsigned int udp_port_number = 40000;
static unsigned int
get_next_udp_port_number(){
    return ++udp_port_number;
}

void
init_udp_socket(node_t* node){
    //get net avaiable udp port 
    node->udp_port_number = get_next_udp_port_number();

    // create udp socket
    int udp_sock_fd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
    
    //creat internet address for socket
    struct sockaddr_in node_addr;
    node_addr.sin_family = AF_INET;
    node_addr.sin_port = node->udp_port_number;
    node_addr.sin_addr.s_addr = INADDR_ANY;

    //bind socket to udp port
    int bind_result = bind(udp_sock_fd , (struct sockaddr*) &node_addr , sizeof(struct sockaddr));
    if( bind_result == -1){
        printf("Error : Sockect bind failed for Node %s\n" , node->node_name);
        return;
    }
    printf("Info : UDP/%u bound to %s\n" , node->udp_port_number , node->node_name);
    node->udp_sock_fd = udp_sock_fd;


}