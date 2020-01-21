#include "layer2.h"
#include <stdio.h>


char* 
pkt_buffer_shift_right( char* pkt , unsigned int pkt_size , unsigned int total_buffer_size){
    #if 0
        // Input buffer
        +--------------------------+------------------------------------------------------+
        | Aux Info |     Data      |           Empty buffer remaining                     |
        +--------------------------+------------------------------------------------------+
                   ^
                  pkt
    

        // Out buffer
        +---------+-----------------------------------------------------------------------+
        | Aux Info|           Empty buffer remaining                      |     Data      |
        +---------+-----------------------------------------------------------------------+
                                                                          ^                
                                                                         pkt               
    #endif
    // calculate size of empty buffer
    unsigned int empty_buffer_size = total_buffer_size - pkt_size;
    // create pointer to area after empty buffer
    char* new_pkt = pkt + empty_buffer_size;
    // copy pkt data to new_pkt area
    memcpy(new_pkt , pkt , pkt_size );
    // zeroise empty buffer at front of char buffer
    memset( pkt , 0 , empty_buffer_size);
    return new_pkt;
}