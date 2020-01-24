#ifndef __LAYER3_H__
#define __LAYER3_H__

#include "../gluethread/glthread.h"
#include "../net.h"
#include "../graph.h"

typedef struct route_table_
{
    glthread_t route_entries;
}route_table_t;

typedef struct layer3_route_
{
    char dest[16];
    uint8_t mask;
    bool_t is_direct;
    char gw[16];
    char oif[IF_NAME_SIZE];
    glthread_t l3route_glue;
}layer3_route_t;

GLTHREAD_TO_STRUCT(route_glue_to_l3_route , layer3_route_t , l3route_glue);

void init_rt_table(route_table_t **route_table);
void rt_table_add_direct_route(route_table_t *route_table , char *dst , uint8_t mask);
void dump_route_table(route_table_t *route_table);
void rt_table_add_route(route_table_t *route_table , char *dst , uint8_t mask , char *gw_ip , char *oif_name);
layer3_route_t * l3rib_lookup_lpm(route_table_t route_table , uint32_t dest);

#endif