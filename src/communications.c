#include "graph.h"
#include "communications.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>
#include <netdb.h> 
#include <unistd.h>


extern void
layer2_frame_recv(node_t* node , interface_t *intf, char *pkt , uint32_t pkt_size);
static char recv_buffer[MAX_PACKET_BUFFER_SIZE];
static char send_buffer[MAX_PACKET_BUFFER_SIZE];
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
    if(FALSE) printf("Info : UDP/%u bound to %s\n" , node->udp_port_number , node->node_name);
    node->udp_sock_fd = udp_sock_fd;
}
// function for thread to execute
static void
_pkt_receive(node_t *receiving_node,char *pkt_with_aux_data,unsigned int pkt_size  ){
    
    char* recv_intf_name = pkt_with_aux_data;
    interface_t* recv_intf = get_node_if_by_name(receiving_node , recv_intf_name);
    if(receiving_node->debug_status == DEBUG_ON) printf("[_pkt_receive] Info : Incoming packet node %s on interface %s\n" , receiving_node->node_name , recv_intf->if_name);
    if(!recv_intf){
        if(receiving_node->debug_status == DEBUG_ON) printf("[_pkt_receive] Error : Pkt recvd on unknown interface %s on node %s\n", recv_intf->if_name , receiving_node-> node_name);
        return;
    }
    pkt_receive(receiving_node , recv_intf , pkt_with_aux_data + IF_NAME_SIZE, pkt_size - IF_NAME_SIZE);
}
int
pkt_receive( node_t* node , interface_t* intf , char *pkt , unsigned int pkt_size){
    // pkt_dump( (ethernet_frame_t*) pkt ,  pkt_size);
    // ingress of packet into tcp ip stack (at DL layer)
    if(node->debug_status == DEBUG_ON) printf("[pkt_receive] Info : Pkt Recvd (%u bytes) on node %s on IF %s\n" , pkt_size , node->node_name , intf->if_name );

    layer2_frame_recv(node , intf , pkt , pkt_size);

    return  0;

}
static void*
_network_start_pkt_receiver_thread(void *arg){
    node_t *node;
    glthread_t *curr;

    fd_set active_sock_fd_set;
    fd_set backup_sock_fd_set;

    int sock_max_fd = 0;
    int bytes_recvd = 0;

    graph_t* topo = ( void*)arg;

    socklen_t addr_len = sizeof(struct sockaddr);

    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    struct sockaddr_in sender_addr;

    ITERATE_GLTHREAD_BEGIN(&topo->node_list , curr){
        node = graph_glue_to_node(curr);

        if(!node->udp_sock_fd) continue;

        if(node->udp_sock_fd > sock_max_fd)
            sock_max_fd = node->udp_sock_fd;

        FD_SET(node->udp_sock_fd , &backup_sock_fd_set);
    }ITERATE_GLTHREAD_END(&topo->node_list , curr)

        while(1){
            memcpy(&active_sock_fd_set , &backup_sock_fd_set , sizeof(fd_set));
            select(sock_max_fd + 1 , &active_sock_fd_set , NULL , NULL , NULL);

            ITERATE_GLTHREAD_BEGIN(&topo->node_list , curr){
                node = graph_glue_to_node(curr);

                if(FD_ISSET(node->udp_sock_fd , &active_sock_fd_set)){
                    memset(recv_buffer , 0 , MAX_PACKET_BUFFER_SIZE);
                    bytes_recvd = recvfrom(node->udp_sock_fd, (char *) recv_buffer, MAX_PACKET_BUFFER_SIZE , 0 , (struct sockaddr *)&sender_addr , &addr_len);
                    _pkt_receive(node , recv_buffer , bytes_recvd);
                }

            }ITERATE_GLTHREAD_END(&topo->node_list , curr)
        }
}
void
network_start_pkt_receiver_thread(graph_t* topo){

    pthread_attr_t attr; //thread attributes
    pthread_t recv_pkt_thread;  //thread

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr , PTHREAD_CREATE_DETACHED);

    pthread_create(&recv_pkt_thread , &attr , _network_start_pkt_receiver_thread , (void*) topo);

    printf("network receiver thread started for topology \"%s\"\n", topo->topolgy_name);
}
static int
_send_pkt_out(int sock_fd , char *pkt_data , unsigned int pkt_size , unsigned int dst_udp_port_no){
    int rc;
    struct sockaddr_in dest_addr;

    struct hostent *host = (struct hostent *) gethostbyname(LOCALHOST);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = dst_udp_port_no;
    dest_addr.sin_addr = *((struct in_addr *)host->h_addr);

    rc = sendto(sock_fd,pkt_data,pkt_size,0,(struct sockaddr*)&dest_addr , sizeof(struct sockaddr));
    return rc;
}
int
send_pkt_out(char* pkt , unsigned int pkt_size , interface_t *intf){

    int rc = 0;
    node_t* sending_node = intf->att_node;
    node_t* nbr_node = get_nbr_node(intf);
    if(!nbr_node) return -1;

    unsigned int dst_udp_port_no = nbr_node->udp_port_number;
    int sock = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
    if(sock < 0 ){
        printf("[send_pkt_out] Error : Sending socker created failed , errno = %d" , errno);
    }
    
    interface_t* remote_intf  = &intf->link->intf1 == intf ? &intf->link->intf2 : &intf->link->intf1;
    memset(send_buffer , 0 , MAX_PACKET_BUFFER_SIZE);
    char* pkt_wth_aux_data = send_buffer;
    strcpy(pkt_wth_aux_data , remote_intf->if_name );
    pkt_wth_aux_data[IF_NAME_SIZE] = '\0';
    memcpy(pkt_wth_aux_data + IF_NAME_SIZE , pkt , pkt_size);

    rc = _send_pkt_out(sock ,pkt_wth_aux_data , pkt_size + IF_NAME_SIZE ,dst_udp_port_no);
     if(sending_node->debug_status == DEBUG_ON) printf("[send_pkt_out] Info : Pkt Sent from node %s on IF  %s\n" , sending_node->node_name , intf->if_name );

    close(sock);
    return rc;

}
int
send_pkt_flood(node_t *node, interface_t *exempted_intf,char *pkt, unsigned int pkt_size){
    for(int i = 0 ; i < MAX_INTF_PER_NODE ; i++){
        interface_t* intf = node->intf[i];
        if(!intf) continue;
        if(intf == exempted_intf) continue;
        send_pkt_out(pkt ,  pkt_size , intf);
    }
    return 0;
}