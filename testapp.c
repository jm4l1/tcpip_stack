#include "graph.h"
#include "layer2/layer2.h"
#include "libcli.h"

extern graph_t * build_first_topo();
extern graph_t * build_linear_topo();
extern graph_t * build_simple_l2_switch_topo();
extern graph_t * build_dualswitch_topo();
extern graph_t * build_simple_l3_topo();
extern void nw_init_cli();
extern int send_pkt_out(char* pkt , unsigned int pkt_size , interface_t *intf);
extern int send_pkt_flood(node_t *node, interface_t *exempted_intf,char *pkt, unsigned int pkt_size);
graph_t* topo = NULL ;

int
main(int argc, char** argv){

    nw_init_cli();
    set_device_name("simulator");
    start_shell();
    return 0;
};