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



#define IP_HDR_LEN_IN_BYTES(ip_hdr_ptr)     ( (ip_hdr_ptr->ihl) * 4)
#define IP_HDR_TOTAL_LEN_IN_BYTES(ip_hdr_ptr)     ((ip_hdr_ptr->len) * 4)
#define INCREMENT_IPHDR(ip_hdr_ptr)      ( ( char* ) ( ip_hdr +  ((ip_hdr_ptr->ihl) * 4 )) )
#define IP_HDR_PAYLOAD_SIZE(ip_hdr_ptr)     (IP_HDR_TOTAL_LEN_IN_BYTES(ip_hdr) - IP_HDR_LEN_IN_BYTES(ip_hdr) )

#pragma pack (push , 1)
typedef struct ip_hdr_{
    uint8_t version : 4;
    uint8_t ihl : 4;
    uint8_t tos;
    uint16_t len;
    uint16_t identification;
    uint8_t reserved : 1;
    uint8_t DF : 1;
    uint8_t MF : 1;
    uint16_t frag_offset: 13;
    uint8_t TTL;
    uint8_t proto;
    uint16_t checksum;
    uint32_t src;
    uint32_t dest;
}ip_hdr_t;
#pragma pack(pop)

static inline void
init_ip_hdr(ip_hdr_t *ip_hdr){
    ip_hdr -> version = 4;
    ip_hdr -> ihl = 5;
    ip_hdr -> tos = 0;
    ip_hdr -> len = 0;
    ip_hdr -> identification = 0;
    ip_hdr -> reserved = 0;
    ip_hdr -> DF = 1;
    ip_hdr -> MF = 0;
    ip_hdr -> frag_offset = 0;
    ip_hdr -> TTL = 64;
    ip_hdr -> proto = 0;
    ip_hdr -> checksum = 0;
    ip_hdr -> src = 0 ;
    ip_hdr -> dest = 0;
}
void init_rt_table(route_table_t **route_table);
void rt_table_add_direct_route(route_table_t *route_table , char *dst , uint8_t mask);
void rt_dump_table(route_table_t *route_table);
void rt_table_add_route(route_table_t *route_table , char *dst , uint8_t mask , char *gw_ip , char *oif_name);
layer3_route_t * l3rib_lookup_lpm(route_table_t route_table , uint32_t dest);

#endif