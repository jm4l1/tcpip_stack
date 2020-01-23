#include "layer2/layer2.h"
#include <stdio.h>

void
pkt_dump( ethernet_frame_t *eth_frame , unsigned int pkt_size){
    printf("<Ethernet>\n");
    printf("Dest Mac: %02x:%02x:%02x:%02x:%02x:%02x\n" , eth_frame->dest_mac.mac[0] ,eth_frame->dest_mac.mac[1] , eth_frame->dest_mac.mac[2] , eth_frame->dest_mac.mac[3] , eth_frame->dest_mac.mac[4] , eth_frame->dest_mac.mac[5] );
    printf("src Mac: %02x:%02x:%02x:%02x:%02x:%02x\n" , eth_frame->src_mac.mac[0] ,eth_frame->src_mac.mac[1] , eth_frame->src_mac.mac[2] , eth_frame->src_mac.mac[3] , eth_frame->src_mac.mac[4] , eth_frame->src_mac.mac[5] );
    printf("Type : 0x%.4x\n" , eth_frame->type);

    if(eth_frame->type == ARP_PACKET){
        arp_packet_t *arp_packet = ( arp_packet_t *) eth_frame->payload;

        char arp_src_ip[16];
        char arp_target_ip[16];
        convert_ip_from_int_to_str(  htonl(arp_packet->src_ip) ,arp_src_ip);
        convert_ip_from_int_to_str( htonl(arp_packet->dst_ip) , arp_target_ip);

        printf("\t<ARP>\n");
        // | hw_type | proto_type | hw_addr_len | proto_addr_len | op_code |    src_mac    |  src_ip  |    dest_mac  |    dst_ip   |
        printf("\tHW Type: %d\n" , arp_packet->hw_type);
        printf("\tProto Type: 0x%04x\n" , arp_packet->proto_type);
        printf("\tHW ADDR LEN: %d\n" , arp_packet->hw_addr_len);
        printf("\tProto: %d\n" , arp_packet->proto_addr_len);
        printf("\tHW Type: %d\n" , arp_packet->hw_type);
        printf("\tOP Code: %d\n" , arp_packet->op_code);
        printf("\tSrc Mac: %02x:%02x:%02x:%02x:%02x:%02x\n" , arp_packet->src_mac.mac[0], arp_packet->src_mac.mac[1], arp_packet->src_mac.mac[2], arp_packet->src_mac.mac[3], arp_packet->src_mac.mac[4], arp_packet->src_mac.mac[5]);
        printf("\tSrc IP: %s\n" , arp_src_ip);
        printf("\tTarget Mac: %02x:%02x:%02x:%02x:%02x:%02x\n" , arp_packet->dst_mac.mac[0], arp_packet->dst_mac.mac[1], arp_packet->dst_mac.mac[2], arp_packet->dst_mac.mac[3], arp_packet->dst_mac.mac[4], arp_packet->dst_mac.mac[5]);
        printf("\tTarget IP: %s\n" , arp_target_ip);
    }
    else if(eth_frame->type == IP_PROTO){
        printf("\t<IP>\n");
    }
    else {
        printf("\t<UNKNOWN>\n");
        printf("\tOffset: %x\n" , ETH_HDR_SIZE_EXCL_PAYLOAD);
        printf("Pkt Size : %u\n" , pkt_size - ( ETH_HDR_SIZE_EXCL_PAYLOAD + 4 ) );
    };
}