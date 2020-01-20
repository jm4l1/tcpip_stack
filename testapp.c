#include "graph.h"
#include "lib/CommandParser/libcli.h"

extern graph_t* build_first_topo();
extern graph_t* build_linear_topo();
extern void nw_init_cli();
extern int send_pkt_out(char* pkt , unsigned int pkt_size , interface_t *intf);
extern int send_pkt_flood(node_t *node, interface_t *exempted_intf,char *pkt, unsigned int pkt_size);
graph_t* topo = NULL ;

int
main(int argc, char** argv){
    nw_init_cli();
    // topo = build_first_topo();
    topo = build_linear_topo();
    
    // {
    //     sleep(2);
    //     node_t* R0_re =  get_node_by_node_name(topo,"R0_re");
    //     interface_t* intf = get_node_if_by_name(R0_re , "eth0/4");

    //     char* pkt = "this is a packet";
    //     // send_pkt_out( pkt, sizeof(pkt) ,intf);
    //     // send_pkt_flood(R0_re , intf , pkt , sizeof(pkt));

    //     sleep(2);
    // }
    start_shell();
    return 0;
};