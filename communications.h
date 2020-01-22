#ifndef __COMMUNICATIONS_H__
#define __COMMUNICATIONS_H__

#include "layer2/layer2.h"

#define MAX_PACKET_BUFFER_SIZE 2048
#define LOCALHOST "127.0.0.1"

int send_pkt_out(char* pkt , unsigned int pkt_size , interface_t *intf);
int send_pkt_flood(node_t *node, interface_t *exempted_intf,char *pkt, unsigned int pkt_size);
int pkt_receive( node_t* receiving_node , interface_t* recv_intf , char* pkt , unsigned int pkt_size);

int
send_pkt_flood_l2_intf_only(node_t *node,interface_t *exempted_intf,char *pkt, unsigned int pkt_size);
#endif