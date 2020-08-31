#ifndef __UTILS__
#define __UTILS__

#define IS_MAC_BROADCAST_ADDR(mac_array)  \
        (mac_array[0] == 0xFF  &&  mac_array[1] == 0xFF && mac_array[2] == 0xFF && \
         mac_array[3] == 0xFF  &&  mac_array[4] == 0xFF && mac_array[5] == 0xFF)

typedef enum{
  FALSE,
  TRUE 
} bool_t;

void
apply_mask(char* prefix , char mask , char* str_prefix);
void
layer2_fill_with_broadcast_mac(char *mac_array);
#endif