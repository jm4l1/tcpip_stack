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

graph_t*
build_simple_l2_switch_topo(){
    #if 0
                                                         
                                               +--------+
                                               |   H4   |
                                               +--------+
                                                   | eth0/0
                                                   | 10.1.1.3/24
                                                   |
                                                   | eth0/4
                                             +-----------+                                   
           +--------+ 10.1.1.1/24            |           |              10.1.1.3/24 +--------+
           |   H1   |------------------------+    SW01   +--------------------------|   H3   |
           +--------+ eth0/0           eth0/1|           | eth0/3            eth0/0 +--------+
                                             +-----------+                                   
                                                   | eth0/2
                                                   |
                                                   |10.1.1.2/24
                                                   | eth0/0
                                              +--------+
                                              |   H2   |
                                              +--------+
    #endif
    graph_t *topo = create_new_graph("Simple L2 Switch Topology");

    node_t *SW01 = create_graph_node(topo, "SW01");
    node_t *H1 = create_graph_node(topo, "H1");
    node_t *H2 = create_graph_node(topo, "H2");
    node_t *H3 = create_graph_node(topo, "H3");
    node_t *H4 = create_graph_node(topo, "H4");

    node_set_loopback_address(H1, "1.1.1.1");
    node_set_loopback_address(H2, "2.2.2.2");
    node_set_loopback_address(H3, "3.3.3.3");
    node_set_loopback_address(H4, "4.4.4.4");

    insert_link_between_two_nodes(SW01 , H1 ,"eth0/1" , "eth0" , 1);
    insert_link_between_two_nodes(SW01 , H2 ,"eth0/2" , "eth0" , 1);
    insert_link_between_two_nodes(SW01 , H3 ,"eth0/3" , "eth0" , 1);
    insert_link_between_two_nodes(SW01 , H4 ,"eth0/4" , "eth0" , 1);

    node_set_intf_ip_address(H1 , "eth0" ,"10.1.1.1" , 24);
    node_set_intf_ip_address(H2 , "eth0" ,"10.1.1.2" , 24);
    node_set_intf_ip_address(H3 , "eth0" ,"10.1.1.3" , 24);
    node_set_intf_ip_address(H4 , "eth0" ,"10.1.1.4" , 24);

    return topo;
}
graph_t*
build_dualswitch_topo(){
    #if 0
                                                                                
                                               +--------+                                              +--------+                                                                             
                                               |   H3   |                                              |   H4   |                                                                             
                                               +--------+                                              +--------+                                                                             
                                                   | eth0/0                                                | eth0/0                                                                               
                                                   | 10.1.1.3/24                                           | 10.1.1.4/24                                                                                    
                                                   |                                                       |                                                                                                                                                        
                                                   | eth0/3  , VID10                                       | eth0/4  , VID 10                                                                                                                                                   
                                             +-----------+                                           +----------+                                                                                                                                                    
           +--------+ 10.1.1.1/24       VID10|           | TR VID 10 , 11             TR VID 10 , 11 |          |              10.1.1.6/24 +--------+                                                                                                                  
           |   H1   |------------------------+    SW01   +===========================================+   SW02   +--------------------------|   H6   |                                                                                                                  
           +--------+ eth0/0           eth0/1|           | ge0/1                                ge0/1|          | eth0/6 , VID 11   eth0/0 +--------+                                                                                                                  
                                             +-----------+                                           +----------+                                                                                                                                                    
                                                   | eth0/2 , VID11                                        | eth0/5  , VID11                                                                            
                                                   |                                                       |                                                                        
                                                   |10.1.1.2/24                                            |10.1.1.5/24                                                                                   
                                                   | eth0/0                                                | eth0/0                                                                               
                                              +--------+                                              +--------+                                                                            
                                              |   H2   |                                              |   H5   |                                                                            
                                              +--------+                                              +--------+                                                                            
    #endif

    graph_t *topo = create_new_graph("Dual Switch Topology");

    node_t *SW01 = create_graph_node(topo , "SW01");
    node_t *SW02 = create_graph_node(topo , "SW02");

    node_t *H1 = create_graph_node(topo , "H1");
    node_t *H2 = create_graph_node(topo , "H2");
    node_t *H3 = create_graph_node(topo , "H3");
    node_t *H4 = create_graph_node(topo , "H4");
    node_t *H5 = create_graph_node(topo , "H5");
    node_t *H6 = create_graph_node(topo , "H6");

    insert_link_between_two_nodes(SW01 , SW02 , "ge0/1" , "ge0/1" , 1);
    node_set_intf_l2_mode(SW01 , "ge0/1", TRUNK);
    node_set_intf_l2_mode(SW02 , "ge0/1", TRUNK);
    node_set_intf_vlan_membership(SW01 , "ge0/1" , 10);
    node_set_intf_vlan_membership(SW01 , "ge0/1" , 11);
    node_set_intf_vlan_membership(SW02 , "ge0/1" , 10);
    node_set_intf_vlan_membership(SW02 , "ge0/1" , 11);


    insert_link_between_two_nodes(SW01 , H1 , "eth0/1" , "eth0" , 1);
    insert_link_between_two_nodes(SW01 , H2 , "eth0/2" , "eth0" , 1);
    insert_link_between_two_nodes(SW01 , H3 , "eth0/3" , "eth0" , 1);

    insert_link_between_two_nodes(SW02 , H4 , "eth0/4" , "eth0" , 1);
    insert_link_between_two_nodes(SW02 , H5 , "eth0/5" , "eth0" , 1);
    insert_link_between_two_nodes(SW02 , H6 , "eth0/6" , "eth0" , 1);

    node_set_intf_vlan_membership(SW01 , "eth0/1" , 10);
    node_set_intf_vlan_membership(SW01 , "eth0/3" , 10);
    node_set_intf_vlan_membership(SW01 , "eth0/2" , 10);
    node_set_intf_vlan_membership(SW02 , "eth0/4" , 10);
    node_set_intf_vlan_membership(SW02 , "eth0/5" , 10);
    node_set_intf_vlan_membership(SW02 , "eth0/6" , 10);
    
    node_set_loopback_address(H1 , "1.1.1.1");
    node_set_loopback_address(H2 , "2.2.2.2");
    node_set_loopback_address(H3 , "3.3.3.3");
    node_set_loopback_address(H4 , "4.4.4.4");
    node_set_loopback_address(H5 , "5.5.5.5");
    node_set_loopback_address(H6 , "6.6.6.6");

    node_set_intf_ip_address(H1, "eth0" , "10.1.1.1" , 24);
    node_set_intf_ip_address(H2, "eth0" , "10.1.1.2" , 24);
    node_set_intf_ip_address(H3, "eth0" , "10.1.1.3" , 24);
    node_set_intf_ip_address(H4, "eth0" , "10.1.1.4" , 24);
    node_set_intf_ip_address(H5, "eth0" , "10.1.1.5" , 24);
    node_set_intf_ip_address(H6, "eth0" , "10.1.1.6" , 24);

    network_start_pkt_receiver_thread(topo);
    return topo;
}