#include "graph.h"
#include "layer2/layer2.h"
#include "libcli.h"

extern graph_t *build_first_topology();
extern graph_t *build_linear_topology();
extern graph_t *build_simple_l2_switch_topology();
extern graph_t *build_dualswitch_topology();
extern graph_t *build_simple_l3_topology();
extern void nw_init_cli();
extern int send_pkt_out(char *pkt, unsigned int pkt_size, interface_t *intf);
extern int send_pkt_flood(node_t *node, interface_t *exempted_intf, char *pkt,
                          unsigned int pkt_size);

int main(int argc, char **argv) {
  set_raw_mode();
  nw_init_cli();
  set_device_name("simulator");
  start_shell();
  restore_mode();
  return 0;
};