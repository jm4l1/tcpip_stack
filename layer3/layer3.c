#include "layer3.h"
#include "../utils.h"

void init_rt_table(route_table_t **route_table)
{
    *route_table = calloc(1 , sizeof(route_table_t));
    init_glthread(&( (*route_table)->route_entries ));
    if(FALSE) printf("[init_rt_table] Info - Route Table Initialized\n");
};
void rt_table_add_direct_route(route_table_t *route_table , char *dst , uint8_t mask)
{
    layer3_route_t *direct_route = calloc(1 , sizeof(layer3_route_t));
    strcpy(direct_route->dest , dst);
    direct_route->mask = mask;
    direct_route->is_direct = TRUE;

    init_glthread(&direct_route->l3route_glue);
    glthread_add_next(&route_table->route_entries , &direct_route->l3route_glue);
};
void rt_dump_table(route_table_t *route_table)
{
    glthread_t *curr;
    layer3_route_t *l3route;
    ITERATE_GLTHREAD_BEGIN(&route_table->route_entries , curr)
    {
        if(!curr) return;
        l3route = route_glue_to_l3_route(curr);
        printf("Dst: %2s/%-4hhu %-5s , GW: %s, OIF : %s\n" , l3route->dest , l3route->mask , ( l3route->is_direct ? "directly connected" : "remote subnet" ) , l3route->gw , l3route->oif);
    }ITERATE_GLTHREAD_END(&route_table->route_entries , curr)
};
void rt_table_add_route(route_table_t *route_table , char *dst , uint8_t mask , char *gw , char *oif_name)
{
    printf("[rt_table_add_route] Adding route : %s / %hhu next-hop %s via %s" , dst , mask , gw , oif_name );
    layer3_route_t *direct_route = calloc(1 , sizeof(layer3_route_t));

    apply_mask(dst , 32 , direct_route->mask);
    direct_route->mask = mask;
    direct_route->is_direct = FALSE;
    strcpy(direct_route->gw , gw);
    strcpy(direct_route->oif , oif_name);

    init_glthread(&direct_route->l3route_glue);
    glthread_add_next(&route_table->route_entries , &direct_route->l3route_glue);

}
layer3_route_t * l3rib_lookup_lpm(route_table_t route_table, uint32_t dest)
{
    layer3_route_t * l3_route = calloc( 1 , sizeof(layer3_route_t));
    return l3_route;
}
void demote_pkt_to_layer3(node_t *node, char *pkt, uint32_t pkt_sike , uint8_t protocol_number , uint32_t dest_ip_address)
{
}
void promote_pkt_to_layer3(node_t *node , interface_t *recv_intf , char *payload , uint32_t app_data_size , uint8_t protocol_number)
{
}
