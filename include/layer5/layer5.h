#ifndef __LAYER5_H__
#define __LAYER5_H__

#include "graph.h"

void promote_pkt_layer5(node_t *node, interface_t *recv_intf , char* payload , uint32_t app_data_size);

#endif