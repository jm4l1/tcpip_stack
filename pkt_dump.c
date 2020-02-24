#include "layer2/layer2.h"
#include "layer3/layer3.h"
#include <stdio.h>

void
icmp_pkt_dump(char *pkt){
    icmp_pkt_t *icmp_pkt = (icmp_pkt_t *) pkt;
    printf("\t<ICMP>\n");
    printf("\tType : %hhu\n", icmp_pkt->header.type);
    printf("\tCode : %hhu\n", icmp_pkt->header.code);
    printf("\tChecksum : %hhu\n", icmp_pkt->header.checksum);

    switch (icmp_pkt->header.type)
    {
    case 0:
    case 8:
        printf("\tIdentifier: %hu\n" , icmp_pkt->header.identifier);
        printf("\tSequence Number: %hu\n" , icmp_pkt->header.seq_num);
        printf("\tData : 0x%8x\n", icmp_pkt->data);
        break;
    
    default:
        break;
    }
}
void
arp_pkt_dump(char * pkt)
{
    arp_packet_t *arp_packet = ( arp_packet_t *) (char *) pkt;

    char arp_src_ip[16];
    char arp_target_ip[16];
    convert_ip_from_int_to_str(  htonl(arp_packet->src_ip) ,arp_src_ip);
    convert_ip_from_int_to_str( htonl(arp_packet->dst_ip) , arp_target_ip);

    printf("\t<ARP>\n");
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
void
ip_pkt_dump(char *pkt)
{
    ip_hdr_t *ip_packet = ( ip_hdr_t *)pkt;
    char ip_src_add[16];
    char ip_dest_add[16];
    convert_ip_from_int_to_str( htonl(ip_packet->src) , ip_src_add);
    convert_ip_from_int_to_str( htonl(ip_packet->dest) , ip_dest_add);
    printf("\t<IP>\n");
    printf("\tVersion: %hhu\n",ip_packet->version);
    printf("\tHeader Length: %hhu\n",ip_packet->ihl);
    printf("\tTOS: %hhu\n",ip_packet->tos);
    printf("\tTotal Length: %hhu\n",ip_packet->len);
    printf("\tIdentification: %hhu\n",ip_packet->identification);
    printf("\tFlags: %hhu %hhu %hhu\n",ip_packet->reserved , ip_packet->DF , ip_packet->MF);
    printf("\tFrament Offset: %hu\n",ip_packet->frag_offset);
    printf("\tTTL: %hhu\n",ip_packet->TTL);
    printf("\tProtocol: %hhu\n",ip_packet->proto);
    printf("\tChecksum: %hu\n",ip_packet->checksum);
    printf("\tSource Address: %s\n",ip_src_add);
    printf("\tDestination Address: %s\n",ip_dest_add);
    switch (ip_packet->proto)
    {
    case ICMP_PROTO:
        icmp_pkt_dump( pkt +  ( ip_packet-> ihl * 4 ) );
        break;
    default:
        break;
    }
}
void
pkt_dump( ethernet_frame_t *eth_frame , unsigned int pkt_size){
    uint16_t type;
    char payload[MAX_ETH_PAYLOAD];
    printf("<Ethernet>\n");
    printf("Dst Mac: %02x:%02x:%02x:%02x:%02x:%02x\n" , eth_frame->dest_mac.mac[0] ,eth_frame->dest_mac.mac[1] , eth_frame->dest_mac.mac[2] , eth_frame->dest_mac.mac[3] , eth_frame->dest_mac.mac[4] , eth_frame->dest_mac.mac[5] );
    printf("Src Mac: %02x:%02x:%02x:%02x:%02x:%02x\n" , eth_frame->src_mac.mac[0] ,eth_frame->src_mac.mac[1] , eth_frame->src_mac.mac[2] , eth_frame->src_mac.mac[3] , eth_frame->src_mac.mac[4] , eth_frame->src_mac.mac[5] );
    
    vlan_tagged_ethernet_frame_t *vlan_eth_frame = is_pkt_vlan_tagged(eth_frame);
    if(vlan_eth_frame){
        type = vlan_eth_frame->type;
        memcpy(payload , vlan_eth_frame->payload , pkt_size - VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD);
        printf("<VLAN TAGGED>\n");
        printf("\tTPID : 0x%4x\n" , vlan_eth_frame->vlan_8021q_tag.tpid);
        printf("\tPRI : %hu\n" , vlan_eth_frame->vlan_8021q_tag.tci_pri);
        printf("\tDEI: %hu\n" , vlan_eth_frame->vlan_8021q_tag.tci_dei);
        printf("\tVLAN ID: %hu\n" , vlan_eth_frame->vlan_8021q_tag.tci_vid);
    }
    else{
        type = eth_frame->type;
        memcpy(payload , eth_frame->payload , pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD);
        printf("<VLAN Untagged>\n");
    }    
    printf("Type : 0x%.4x\n" , type);

    if(type == ARP_PACKET)
    {
        arp_pkt_dump(payload);
    }
    else if(type == IP_PROTO){
        ip_pkt_dump(payload);
    }
    else {
        printf("\t<UNKNOWN>\n");
        printf("\tOffset: %lx\n" , ETH_HDR_SIZE_EXCL_PAYLOAD);
        printf("Pkt Size : %lu\n" , pkt_size - ( ETH_HDR_SIZE_EXCL_PAYLOAD + 4 ) );
    };
}