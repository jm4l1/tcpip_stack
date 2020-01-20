#include "graph.h"

extern void
network_start_pkt_receiver_thread(graph_t* topo);

graph_t*
build_first_topo(){
    #if 0

                            +----------+
                        0/4 |          |0/0
            +----------------+   R0_re  +---------------------------+
            |     40.1.1.1/24| 122.1.1.0|20.1.1.1/24                |
            |                +----------+                           |
            |                                                       |
            |                                                       |
            |                                                       |
            |40.1.1.2/24                                            |20.1.1.2/24
            |0/5                                                    |0/1
        +---+---+                                              +----+-----+
        |       |0/3                                        0/2|          |
        | R2_re +----------------------------------------------+    R1_re |
        |       |30.1.1.2/24                        30.1.1.1/24|          |
        +-------+                                              +----------+

    #endif

    //Create New Graph
    graph_t* topo = create_new_graph("My First Graph");

    // //Create Nodes on Graph
    node_t* R0_re = create_graph_node(topo,"R0_re");
    node_t* R1_re = create_graph_node(topo,"R1_re");
    node_t* R2_re = create_graph_node(topo,"R2_re");

    // Add links between node
    insert_link_between_two_nodes(R0_re,R1_re,"eth0/0","eth0/1",1);
    insert_link_between_two_nodes(R1_re,R2_re,"eth0/2","eth0/3",1);
    insert_link_between_two_nodes(R0_re,R2_re,"eth0/4","eth0/5",1);

    //Add network parameter
    node_set_loopback_address(R0_re , "122.1.1.0");
    node_set_intf_ip_address(R0_re , "eth0/0" , "40.1.1.1" , 24);
    node_set_intf_ip_address(R0_re , "eth0/4" , "20.1.1.1" , 24);

    node_set_loopback_address(R1_re , "122.1.1.1");
    node_set_intf_ip_address(R1_re , "eth0/1" , "20.1.1.2" , 24);
    node_set_intf_ip_address(R1_re , "eth0/2" , "30.1.1.1" , 24);

    node_set_loopback_address(R2_re , "122.1.1.2");
    node_set_intf_ip_address(R2_re , "eth0/3" , "30.1.1.2" , 24);
    node_set_intf_ip_address(R2_re , "eth0/5" , "40.1.1.2" , 24);

    // start network receiver thread
    network_start_pkt_receiver_thread(topo);

    return topo;
}

graph_t*
build_linear_topo(){
    #if 0
        +-----------+                        +-----------+                         +-----------+           
        |           |0/1                  0/2|           |0/3                   0/4|           |           
        +   R0_re   +------------------------+   R1_re   +-------------------------+   R2_re   +           
        |           |10.1.1.1/24  10.1.1.2/24|           |20.1.1.2/24   20.1.1.1/24|           |           
        +-----------+                        +-----------+                         +-----------+           
    #endif

    graph_t* topo;
    topo = create_new_graph("Linear Topology");

    node_t* R0_re = create_graph_node(topo , "R0_re");
    node_t* R1_re = create_graph_node(topo , "R1_re");
    node_t* R2_re = create_graph_node(topo , "R2_re");

    insert_link_between_two_nodes(R0_re,R1_re, "eth0/1" , "eth0/2", 1);
    insert_link_between_two_nodes(R1_re,R2_re, "eth0/3" , "eth0/4", 1);

    node_set_loopback_address(R0_re , "1.1.1.1");
    node_set_loopback_address(R1_re , "2.2.2.2");
    node_set_loopback_address(R2_re , "3.3.3.3");

    node_set_intf_ip_address(R0_re , "eth0/1" , "10.1.1.1" , 24);
    node_set_intf_ip_address(R1_re , "eth0/2" , "10.1.1.2" , 24);
    node_set_intf_ip_address(R1_re , "eth0/3" , "20.1.1.2" , 24);
    node_set_intf_ip_address(R2_re , "eth0/4" , "20.1.1.1" , 24);

    network_start_pkt_receiver_thread(topo);

    return topo;
}