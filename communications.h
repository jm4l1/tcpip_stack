#ifndef __COMMUNICATIONS_H__
#define __COMMUNICATIONS_H__

#define MAX_PACKET_BUFFER_SIZE 2048


int send_pkt_out(char* pkt , unsigned int pkt_size , interface_t *intf);
#endif