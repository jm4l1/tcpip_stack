#include "graph.h"
#include "lib/CommandParser/libcli.h"

extern graph_t* build_first_topo();
extern void nw_init_cli();

graph_t* topo = NULL ;

int
main(int argc, char** argv){
    nw_init_cli();
    topo = build_first_topo();
    // dump_graph(topo);
    // dump_nw_graph(topo);
    // delete_graph(topo);
    start_shell();
    return 0;
};