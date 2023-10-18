/*
filename: td_skb.c
function: ARP¡¢ICMPºÍDHCPµ÷ÊÔ
*/

#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/netdevice.h>
#include <ethernet.h>

// BCM reference header file
#include <bcmendian.h>
#include <bcmarp.h>
#include <bcmip.h>
#include <bcmicmp.h>
#include <bcmudp.h>
#include <bcmdhcp.h>

#include "td_debug.h"

#define IP_FMT      "%u.%u.%u.%u"
#define IP_ARY(a)   ((a)[0]&0xff), ((a)[1]&0xff), ((a)[2]&0xff), ((a)[3]&0xff)

#define ARPOP_REQUEST   1       /* request to resolve address */
#define ARPOP_REPLY     2       /* response to previous request */
#define ARPOP_REVREQUEST 3      /* request protocol address given hardware */
#define ARPOP_REVREPLY  4       /* response giving protocol address */
#define ARPOP_INVREQUEST 8      /* request to identify peer */
#define ARPOP_INVREPLY  9       /* response identifying peer */
#define ARPOP_NAK       10      /* NAK - only valif for ATM ARP */

#define DHCP_MSGTYPE_DISCOVER   1
#define DHCP_MSGTYPE_OFFER      2
#define DHCP_MSGTYPE_REQUEST    3
#define DHCP_MSGTYPE_DECLINE    4       /* decline, client refuse use this ip */
#define DHCP_MSGTYPE_ACK        5
#define DHCP_MSGTYPE_NAK        6
#define DHCP_MSGTYPE_RELEASE    7
#define DHCP_MSGTYPE_INFORM     8

#define PRINT_ARP_ON()  (g_td_debug & TD_DEBUG_ARP)
#define PRINT_ICMP_ON()  (g_td_debug & TD_DEBUG_ICMP)
#define PRINT_DHCP_ON()  (g_td_debug & TD_DEBUG_DHCP)


static void td_arp_print(struct sk_buff *skb, bool istx)
{
    struct ether_header *eth = (struct ether_header *)skb->data;
    struct bcmarp *arp = (struct bcmarp *)(eth + 1);

    switch (ntoh16(arp->oper)) {
        case ARPOP_REQUEST:
            if (!memcmp(arp->dst_ip, arp->src_ip, 4)) {
                printk("[%s]%s ARP request: %pM > %pM, gratuitous ARP for "IP_FMT"\n", 
                    istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                    IP_ARY(arp->src_ip));
            } else {
                printk("[%s]%s ARP request: %pM > %pM, Who-has "IP_FMT"? Tell "IP_FMT"\n", 
                    istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                    IP_ARY(arp->dst_ip), IP_ARY(arp->src_ip));
            }
            break;
        case ARPOP_REPLY:
            printk("[%s]%s ARP reply: %pM > %pM, "IP_FMT" is-at %pM\n", istx? "TX": "RX", 
                skb->dev->name, eth->ether_shost, eth->ether_dhost, IP_ARY(arp->src_ip), 
                arp->src_eth);
            break;
        case ARPOP_REVREQUEST:
            printk("[%s]%s ARP revert request: %pM > %pM, Who-is %pM? Tell %pM\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                arp->dst_eth, arp->src_eth);
            break;
        case ARPOP_REVREPLY:
            printk("[%s]%s ARP revert reply: %pM > %pM, %pM at "IP_FMT"\n", istx? "TX": "RX", 
                skb->dev->name, eth->ether_shost, eth->ether_dhost, arp->src_eth, 
                IP_ARY(arp->src_ip));
            break;
        default:
            printk("[%s]%s: %pM > %pM, unkown arp type %#x\n", istx? "TX": "RX", skb->dev->name, 
                eth->ether_shost, eth->ether_dhost, ntoh16(arp->oper));
            break;
    }
   
}

static void td_dhcp_print(struct sk_buff *skb, bool istx)
{
    struct ether_header *eth = (struct ether_header *)skb->data;
    struct ipv4_hdr *iph = (struct ipv4_hdr *)(eth + 1);
    struct bcmudp_hdr *udh = (struct bcmudp_hdr *)(iph + 1);
    uint8* bootp = (uint8*)(udh + 1);
    uint8* dhcp = bootp + DHCP_OPT_OFFSET;
    bool is_broadcast = FALSE;
    uint8 msgtype;
    
    /* check dhcp magic cookie */
    if (hton32(0x63825363) != *(uint*)dhcp) {
        return ;
    }

    /* check message type option */
    if (53 != dhcp[4]) {
        return ;
    }else {
        msgtype = dhcp[6];
    }

    /* check broadcast, bootp flags=0x8000 */
    if (hton16(0x8000) == *(uint16*)(bootp + DHCP_FLAGS_OFFSET)) {
        is_broadcast = TRUE;
    }

    switch (msgtype) {
        case DHCP_MSGTYPE_DISCOVER:
            if (50 == dhcp[16]) {
                printk("[%s]%s DHCP discover: %pM > %pM, query ip "IP_FMT", broadcast_flag=%u\n", 
                    istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                    IP_ARY(dhcp + 18), is_broadcast);
            } else {
                printk("[%s]%s DHCP discover: %pM > %pM, broadcast_flag=%u\n", istx? "TX": "RX", 
                    skb->dev->name, eth->ether_shost, eth->ether_dhost, is_broadcast);
            }
            
            break;
        case DHCP_MSGTYPE_OFFER:
            printk("[%s]%s DHCP offer: %pM > %pM, offer ip "IP_FMT", broadcast_flag=%u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                IP_ARY(bootp + DHCP_YIADDR_OFFSET), is_broadcast);
            break;
        case DHCP_MSGTYPE_REQUEST:
            if (50 == dhcp[16]){
                printk("[%s]%s DHCP request: %pM > %pM, request ip "IP_FMT", broadcast_flag=%u\n", 
                    istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                    IP_ARY(dhcp + 18), is_broadcast );
            } else {
                printk("[%s]%s DHCP request: %pM > %pM, broadcast_flag=%u\n", istx? "TX": "RX", 
                    skb->dev->name, eth->ether_shost, eth->ether_dhost, is_broadcast );
            }
            break;
        case DHCP_MSGTYPE_ACK:
            printk("[%s]%s DHCP ack: %pM > %pM, "IP_FMT" take effect, broadcast_flag=%u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                IP_ARY(bootp + DHCP_YIADDR_OFFSET), is_broadcast );
            break;
        case DHCP_MSGTYPE_NAK:
            printk("[%s]%s DHCP nak: %pM > %pM, refuse ip "IP_FMT", broadcast_flag=%u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                IP_ARY(bootp + DHCP_YIADDR_OFFSET), is_broadcast );
            break;
        case DHCP_MSGTYPE_DECLINE:
            printk("[%s]%s DHCP decline: %pM > %pM, ip "IP_FMT" conflict, broadcast_flag=%u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                IP_ARY(bootp + DHCP_YIADDR_OFFSET), is_broadcast );
            break;
        case DHCP_MSGTYPE_RELEASE:
            printk("[%s]%s DHCP release: %pM > %pM, ip "IP_FMT" release, broadcast_flag=%u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                IP_ARY(bootp + DHCP_CIADDR_OFFSET), is_broadcast );
            break;
        case DHCP_MSGTYPE_INFORM:
            printk("[%s]%s DHCP inform: %pM > %pM, ip "IP_FMT" inform, broadcast_flag=%u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                IP_ARY(bootp + DHCP_CIADDR_OFFSET), is_broadcast );
            break;
        default:
            printk("[%s]%s DHCP unkown: %pM > %pM, msgtype %u, broadcast_flag=%u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                msgtype, is_broadcast);
            break;
    }
   
}

static void td_icmp_print(struct sk_buff *skb, bool istx)
{
    struct ether_header *eth = (struct ether_header *)skb->data;
    struct ipv4_hdr *iph = (struct ipv4_hdr *)(eth + 1);
    struct icmphdr *ich = (struct icmphdr *)(iph + 1);

    switch (ich->type) {
        case ICMP_TYPE_ECHO_REQUEST:
             printk("[%s]%s ICMP request: %pM > %pM, "IP_FMT" ping "IP_FMT", id %u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                IP_ARY(iph->src_ip), IP_ARY(iph->dst_ip), ntoh16(ich->un.echo.id));
            break;
        case ICMP_TYPE_ECHO_REPLY:
            printk("[%s]%s ICMP reply: %pM > %pM, "IP_FMT" pong "IP_FMT", id %u\n", 
                istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                IP_ARY(iph->src_ip), IP_ARY(iph->dst_ip), ntoh16(ich->un.echo.id));
            break;
        case ICMP_DEST_UNREACH:
            if (ICMP_NET_UNREACH == ich->code) {
                struct ipv4_hdr *iph2 = (struct ipv4_hdr *)(ich + 1);
                printk("[%s]%s ICMP unreach: %pM > %pM, net "IP_FMT" is unreachable\n", 
                    istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                    IP_ARY(iph2->dst_ip));
            } else if (ICMP_PORT_UNREACH == ich->code) {
                struct ipv4_hdr *iph2 = (struct ipv4_hdr *)(ich + 1);
                struct bcmudp_hdr *udh = (struct bcmudp_hdr *)(iph2 + 1);
                
                printk("[%s]%s ICMP unreach: %pM > %pM, "IP_FMT" UDP port %u is unreachable\n", 
                    istx? "TX": "RX", skb->dev->name, eth->ether_shost, eth->ether_dhost, 
                    IP_ARY(iph2->dst_ip), ntoh16(udh->dst_port));
            } else {
                printk("[%s]%s ICMP unreach: %pM > %pM, reason %u\n", istx? "TX": "RX", 
                    skb->dev->name, eth->ether_shost, eth->ether_dhost, ich->code);
            }
            break;
        default:
            printk("[%s]%s ICMP unkown: %pM > %pM, type %u\n", istx? "TX": "RX", 
            skb->dev->name, eth->ether_shost, eth->ether_dhost, ich->type);
            break;
    }
  
}

void td_skb_debug(void *p, bool is_tx, const char *func, int line)
{
    struct sk_buff *skb = (struct sk_buff *)p;
    struct ether_header *eth = NULL;
    struct ipv4_hdr *iph = NULL;
    struct bcmudp_hdr *udh = NULL;

    if (!p){
        return ;
    }

    /* check debug of arp, icmp, dhcp */
    if (!PRINT_ARP_ON() && !PRINT_ICMP_ON() && !PRINT_DHCP_ON()) {
        return ;
    }

    eth = (struct ether_header *)skb->data;

    /* dev filter */
    if (g_print_dev[0] && strncmp(g_print_dev, skb->dev->name, IFNSIZE)) {
        return ;
    }
    
    /* mac filter */
    if ( !TD_IS_NULL_MAC(&g_print_mac.octet) && 
        memcmp(eth->ether_shost, &g_print_mac.octet, ETHER_ADDR_LEN) &&
        memcmp(eth->ether_dhost, &g_print_mac.octet, ETHER_ADDR_LEN) ) {
        return ;
    }

    switch (ntoh16(eth->ether_type)) {
        case ETHER_TYPE_ARP:
            if (PRINT_ARP_ON()) {
                td_arp_print(skb, is_tx);
            }
            break;
        case ETHER_TYPE_IP:
            iph = (struct ipv4_hdr *)(eth + 1);
            if (IP_PROT_ICMP == iph->prot) {
                if (PRINT_ICMP_ON()) {
                    td_icmp_print(skb, is_tx);
                }
            } else if (IP_PROT_UDP == iph->prot) {
                udh = (struct bcmudp_hdr *)(iph + 1);
                if ( (DHCP_PORT_SERVER == ntoh16(udh->src_port) &&
                    DHCP_PORT_CLIENT == ntoh16(udh->dst_port)) ||
                    (DHCP_PORT_CLIENT == ntoh16(udh->src_port) && 
                    DHCP_PORT_SERVER == ntoh16(udh->dst_port)) ){
                    if (PRINT_DHCP_ON()) {
                        td_dhcp_print(skb, is_tx);
                    }
                }
            }
            
            break;
        default:
            break;
    }

    return ;
}
