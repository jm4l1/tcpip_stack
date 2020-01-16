#include "graph.h"

extern graph_t* build_first_topo();

int
main(int argc, char** argv){
    graph_t* topo = build_first_topo();
    // dump_graph(topo);
    dump_nw_graph(topo);
    delete_graph(topo);
    return 0;
};