/*
<:copyright-BRCM:2017:proprietary:standard

   Copyright (c) 2017 Broadcom 
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:> 
*/

/*
*******************************************************************************
*
* File Name  : archer_ucast.c
*
* Description: Translation of Unicast Blogs into L3 Unicast Archer Flows.
*
*******************************************************************************
*/
#include <linux/module.h>
#include <linux/bcm_log.h>
#include <linux/blog.h>
#include "bcmenet.h"
#include "pktHdr.h"

#include "bcm_wlan_defs.h"

#include "sysport_rsb.h"
#define CC_SYSPORT_PARSER_CONFIG_DEFAULTS
#include "sysport_parser.h"
#include "sysport_classifier.h"
#include "sysport_driver.h"

#include "archer.h"
#include "archer_driver.h"
#include "archer_drop.h"
#include "archer_xtmrt.h"
#include "archer_wlan.h"
#include "bcm_archer_dpi.h"

#include "cmdlist_api.h"

static int ip_addr_table_index_g;

int archer_ucast_ipv4_addresses_table_add(Blog_t *blog_p, uint32_t *table_sram_address_p)
{
    sysport_ip_addr_table_t ip_addr_table;
    int ret;

    ip_addr_table.ip_src_addr = ntohl(blog_p->rx.tuple.saddr);
    ip_addr_table.ip_dst_addr = ntohl(blog_p->rx.tuple.daddr);

    ret = sysport_classifier_ip_addr_table_add(&ip_addr_table,
                                               &ip_addr_table_index_g);
    if(ret)
    {
        __logInfo("Could not sysport_classifier_ip_addr_table_add");

        return ret;
    }

    *table_sram_address_p = 0;

    return 0;
}

int archer_ucast_wlan_egress_port(Blog_t *blog_p, int *egress_port_p)
{
    if(blog_p->rnr.is_wfd)
    {
        if(blog_p->wfd.nic_ucast.is_chain)
        {
            if(blog_p->wfd.nic_ucast.wfd_idx >= SYSPORT_FLOW_WLAN_PORTS_MAX)
            {
                __logError("Invalid wfd_idx %d (max %d)",
                           blog_p->wfd.nic_ucast.wfd_idx, SYSPORT_FLOW_WLAN_PORTS_MAX);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            *egress_port_p = blog_p->wfd.nic_ucast.wfd_idx;

            __debug("NIC: wfd_idx %u -> egress_port %u\n",
                    blog_p->wfd.nic_ucast.wfd_idx, *egress_port_p);
        }
        else
        {
            if(blog_p->wfd.dhd_ucast.wfd_idx >= SYSPORT_FLOW_WLAN_PORTS_MAX)
            {
                __logError("Invalid wfd_idx %d (max %d)",
                           blog_p->wfd.dhd_ucast.wfd_idx, SYSPORT_FLOW_WLAN_PORTS_MAX);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            *egress_port_p = blog_p->wfd.dhd_ucast.wfd_idx;

            __debug("DHD: wfd_idx %u -> egress_port %u\n",
                    blog_p->wfd.dhd_ucast.wfd_idx, *egress_port_p);
        }
    }
    else
    {
        if(blog_p->rnr.radio_idx >= SYSPORT_FLOW_WLAN_PORTS_MAX)
        {
            __logError("Invalid wfd_idx %d (max %d)",
                       blog_p->rnr.radio_idx, SYSPORT_FLOW_WLAN_PORTS_MAX);

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }

        *egress_port_p = blog_p->rnr.radio_idx;

        __debug("DHD: egress_port (radio_idx) = %u\n", *egress_port_p);
    }

    return 0;
}

int archer_ucast_wlan_ingress_port(Blog_t *blog_p)
{
    struct net_device *dev = blog_p->rx_dev_p;
    uint32_t hw_port = netdev_path_get_hw_port(dev);
    uint32_t radio_idx = WLAN_RADIO_GET(hw_port);

    if(radio_idx >= SYSPORT_FLOW_WLAN_PORTS_MAX)
    {
        __logError("Invalid WLAN radio %u (hw_port %u)\n", radio_idx, hw_port);

        return -1;
    }

    return radio_idx;
}

int archer_ucast_wlan_egress_queue_wfd(Blog_t *blog_p, sysport_classifier_flow_t *flow_p)
{
    sysport_classifier_flow_context_t *context_p = &flow_p->context;
    sysport_classifier_flow_port_t *port_p = &context_p->ucast.port;

    if(blog_p->rnr.is_wfd)
    {
        if(blog_p->wfd.nic_ucast.is_chain)
        {
            if(blog_p->wfd.nic_ucast.wfd_idx >= SYSPORT_FLOW_WLAN_PORTS_MAX)
            {
                __logError("Invalid wfd_idx %d (max %d)",
                           blog_p->wfd.nic_ucast.wfd_idx, SYSPORT_FLOW_WLAN_PORTS_MAX);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            if(blog_p->wfd.nic_ucast.wfd_prio >= SYSPORT_FLOW_WLAN_QUEUES_MAX)
            {
                __logError("Invalid wfd_prio %d (max %d)",
                           blog_p->wfd.nic_ucast.wfd_prio, SYSPORT_FLOW_WLAN_QUEUES_MAX);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            // wfd_queue index = (wfd_idx << 1) + wfd_prio
            port_p->egress_queue = ((blog_p->wfd.nic_ucast.wfd_idx * 2) +
                                    blog_p->wfd.nic_ucast.wfd_prio);

            context_p->ucast.wlan.wfd.ucast.nic.is_ucast = 1;
            context_p->ucast.wlan.wfd.ucast.nic.wl_prio = blog_p->wfd.nic_ucast.priority;

            context_p->ucast.wlan.wfd.ucast.nic.wl_chainidx = blog_p->wfd.nic_ucast.chain_idx;

            __debug("NIC: ucast, egress_queue %u, priority %u, chain_idx %u\n",
                    port_p->egress_queue, blog_p->wfd.nic_ucast.priority,
                    blog_p->wfd.nic_ucast.chain_idx);
        }
        else
        {
            if(blog_p->wfd.dhd_ucast.wfd_idx >= SYSPORT_FLOW_WLAN_PORTS_MAX)
            {
                __logError("Invalid wfd_idx %d (max %d)",
                           blog_p->wfd.dhd_ucast.wfd_idx, SYSPORT_FLOW_WLAN_PORTS_MAX);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            if(blog_p->wfd.dhd_ucast.wfd_prio >= SYSPORT_FLOW_WLAN_QUEUES_MAX)
            {
                __logError("Invalid wfd_prio %d (max %d)",
                           blog_p->wfd.dhd_ucast.wfd_prio, SYSPORT_FLOW_WLAN_QUEUES_MAX);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            // wfd_queue index = (wfd_idx << 1) + wfd_prio
            port_p->egress_queue = ((blog_p->wfd.dhd_ucast.wfd_idx * 2) +
                                    blog_p->wfd.dhd_ucast.wfd_prio);

            context_p->ucast.wlan.wfd.ucast.dhd.is_ucast = 1;
            context_p->ucast.wlan.wfd.ucast.dhd.wl_prio = blog_p->wfd.dhd_ucast.priority;

            context_p->ucast.wlan.wfd.ucast.dhd.flowring_idx = blog_p->wfd.dhd_ucast.flowring_idx;
            context_p->ucast.wlan.wfd.ucast.dhd.ssid = blog_p->wfd.dhd_ucast.ssid;

            __debug("DHD: ucast, egress_queue %u, priority %u, flowring_idx %u, ssid %u\n",
                    port_p->egress_queue, blog_p->wfd.dhd_ucast.priority,
                    blog_p->wfd.dhd_ucast.flowring_idx, blog_p->wfd.dhd_ucast.ssid);
        }
    }
    else
    {
        context_p->egress_phy_dhd = 1;

        context_p->ucast.wlan.rnr = blog_p->rnr;

        __debug("DHD: radio_idx %u, flowring_idx %u, llcsnap_flag %u, "
                "priority %u, ssid %u, flow_prio %u\n",
                context_p->ucast.wlan.rnr.radio_idx,
                context_p->ucast.wlan.rnr.flowring_idx,
                context_p->ucast.wlan.rnr.llcsnap_flag,
                context_p->ucast.wlan.rnr.priority,
                context_p->ucast.wlan.rnr.ssid,
                context_p->ucast.wlan.rnr.flow_prio);
    }

    return 0;
}

int archer_ucast_common_flow_set(Blog_t *blog_p, sysport_classifier_flow_t *flow_p,
                                 sysport_rsb_flow_type_t flow_type,
                                 sysport_classifier_rsb_overwrite_t *rsb_overwrite_p)
{
    sysport_rsb_flow_header_t *header_p = &flow_p->tuple.header;
    sysport_classifier_flow_context_t *context_p = &flow_p->context;
    sysport_classifier_flow_port_t *port_p;
    sysport_rsb_phy_t enet_ingress_phy = -1;
    sysport_rsb_phy_t enet_egress_phy = -1;
    int enet_ingress_phys_port = -1;
    int egress_port;
    int ret;

    memset(flow_p, 0, sizeof(sysport_classifier_flow_t));

    if(ARCHER_RX_ENET(blog_p))
    {
        ret = sysport_driver_logical_port_to_phys_port(blog_p->rx.info.channel,
                                                       &enet_ingress_phys_port);
        if(ret)
        {
            return ret;
        }

        ret = sysport_driver_logical_port_to_phy(blog_p->rx.info.channel,
                                                 &enet_ingress_phy);
        if(ret)
        {
            return ret;
        }
    }

    if(ARCHER_TX_ENET(blog_p))
    {
        ret = sysport_driver_logical_port_to_phy(blog_p->tx.info.channel,
                                                 &enet_egress_phy);
        if(ret)
        {
            return ret;
        }
    }

#if !defined(CC_SYSPORT_PARSER_V2)
    if(SYSPORT_RSB_FLOW_TYPE_UCAST_L3 == flow_type)
    {
        if(IPPROTO_GRE == blog_p->key.protocol)
        {
            rsb_overwrite_p->parser.header.valid = 0;
        }
    }
#endif

    if(blog_p->hw_pathstat_idx >= SYSPORT_CLASSIFIER_PATHSTAT_MAX)
    {
        __logError("Invalid hw_pathstat_idx: %u\n", blog_p->hw_pathstat_idx);

        return SYSPORT_CLASSIFIER_ERROR_INVALID;
    }

    context_p->pathstat_index = blog_p->hw_pathstat_idx;

    if(SKBMARK_GET_SQ_MARK(blog_p->mark))
    {
        context_p->dpi_queue = SKBMARK_GET_DPIQ_MARK(blog_p->mark);
    }
    else
    {
        context_p->dpi_queue = min(blog_p->dpi_queue, (u8)ARCHER_DPI_BYPASS_QUEUE);
    }

    context_p->tcp_pure_ack = blog_p->key.tcp_pure_ack;

    header_p->valid = 1;
    header_p->flow_type = flow_type;

#if defined(CC_ARCHER_DSL)
    if(blog_p->tx.info.bmap.BCM_XPHY)
    {
        context_p->egress_phy = SYSPORT_RSB_PHY_DSL;
        context_p->egress_port = 0;

        if(ARCHER_RX_ENET(blog_p)) /* LAN/WAN -> XTM-WAN */
        {
            header_p->ingress_phy = enet_ingress_phy;
            header_p->ingress_port = blog_p->rx.info.channel;
            rsb_overwrite_p->parser.header.ingress_port = enet_ingress_phys_port;
        }
        else if(ARCHER_RX_WLAN(blog_p))
        {
            header_p->ingress_phy = SYSPORT_RSB_PHY_WLAN;
            header_p->ingress_port = archer_ucast_wlan_ingress_port(blog_p);
        }
        else
        {
            __logError("Invalid XTM-WAN source: %d, %d",
                       blog_p->rx.info.phyHdrType, blog_p->rx.info.channel);

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }
    }
    else if(blog_p->rx.info.bmap.BCM_XPHY)
    {
        header_p->ingress_phy = SYSPORT_RSB_PHY_DSL;
        header_p->ingress_port = blog_p->rx.info.channel;

        if(ARCHER_TX_ENET(blog_p)) /* XTM-WAN -> LAN/WAN */
        {
            context_p->egress_phy = enet_egress_phy;
            context_p->egress_port = blog_p->tx.info.channel;
        }
        else if(BLOG_IS_TX_HWACC_ENABLED_WLAN_PHY(blog_p->tx.info.phyHdrType)) /* XTM-WAN -> WLAN */
        {
            context_p->egress_phy = SYSPORT_RSB_PHY_WLAN;

            if((ret = archer_ucast_wlan_egress_port(blog_p, &egress_port)))
            {
                return ret;
            }

            context_p->egress_port = egress_port;
        }
        else
        {
            __logError("Invalid XTM-WAN destination: %d, %d",
                       blog_p->tx.info.phyHdrType, blog_p->tx.info.channel);

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }
    }
    else
#endif
    {
        if(ARCHER_TX_ENET_WAN(blog_p))
        {
            context_p->egress_phy = enet_egress_phy;
            context_p->egress_port = blog_p->tx.info.channel;

            if(ARCHER_RX_ENET_LAN(blog_p)) /* LAN -> ENET-WAN */
            {
                header_p->ingress_phy = enet_ingress_phy;
                header_p->ingress_port = blog_p->rx.info.channel;
                rsb_overwrite_p->parser.header.ingress_port = enet_ingress_phys_port;
            }
            else if(ARCHER_RX_WLAN(blog_p))
            {
                header_p->ingress_phy = SYSPORT_RSB_PHY_WLAN;
                header_p->ingress_port = archer_ucast_wlan_ingress_port(blog_p);
            }
            else
            {
                __logError("Invalid ENET-WAN source: %d, %d",
                           blog_p->rx.info.phyHdrType, blog_p->rx.info.channel);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }
        }
        else if(ARCHER_RX_ENET_WAN(blog_p))
        {
            header_p->ingress_phy = enet_ingress_phy;
            header_p->ingress_port = blog_p->rx.info.channel;
            rsb_overwrite_p->parser.header.ingress_port = enet_ingress_phys_port;

            if(ARCHER_TX_ENET_LAN(blog_p)) /* ENET-WAN -> LAN */
            {
                context_p->egress_phy = enet_egress_phy;
                context_p->egress_port = blog_p->tx.info.channel;
            }
            else if(BLOG_IS_TX_HWACC_ENABLED_WLAN_PHY(blog_p->tx.info.phyHdrType)) /* ENET-WAN -> WLAN */
            {
                context_p->egress_phy = SYSPORT_RSB_PHY_WLAN;

                if((ret = archer_ucast_wlan_egress_port(blog_p, &egress_port)))
                {
                    return ret;
                }

                context_p->egress_port = egress_port;
            }
            else
            {
                __logError("Invalid ENET-WAN destination: %d, %d",
                           blog_p->tx.info.phyHdrType, blog_p->tx.info.channel);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }
        }
        else if(ARCHER_RX_ENET_LAN(blog_p))
        {
            header_p->ingress_phy = enet_ingress_phy;
            header_p->ingress_port = blog_p->rx.info.channel;
            rsb_overwrite_p->parser.header.ingress_port = enet_ingress_phys_port;

            if(ARCHER_TX_ENET_LAN(blog_p)) /* LAN -> LAN */
            {
                context_p->egress_phy = enet_egress_phy;
                context_p->egress_port = blog_p->tx.info.channel;
            }
            else if(BLOG_IS_TX_HWACC_ENABLED_WLAN_PHY(blog_p->tx.info.phyHdrType)) /* LAN -> WLAN */
            {
                context_p->egress_phy = SYSPORT_RSB_PHY_WLAN;

                if((ret = archer_ucast_wlan_egress_port(blog_p, &egress_port)))
                {
                    return ret;
                }

                context_p->egress_port = egress_port;
            }
            else
            {
                __logError("Invalid ENET-LAN destination: %d, %d",
                           blog_p->tx.info.phyHdrType, blog_p->tx.info.channel);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }
        }
        else if(ARCHER_RX_WLAN(blog_p))
        {
            header_p->ingress_phy = SYSPORT_RSB_PHY_WLAN;
            header_p->ingress_port = archer_ucast_wlan_ingress_port(blog_p);

            if(ARCHER_TX_ENET_LAN(blog_p)) /* WLAN -> LAN */
            {
                context_p->egress_phy = enet_egress_phy;
                context_p->egress_port = blog_p->tx.info.channel;
            }
            else if(BLOG_IS_TX_HWACC_ENABLED_WLAN_PHY(blog_p->tx.info.phyHdrType)) /* WLAN -> WLAN */
            {
                context_p->egress_phy = SYSPORT_RSB_PHY_WLAN;

                if((ret = archer_ucast_wlan_egress_port(blog_p, &egress_port)))
                {
                    return ret;
                }

                context_p->egress_port = egress_port;
            }
            else
            {
                __logError("Invalid ENET-LAN destination: %d, %d",
                           blog_p->tx.info.phyHdrType, blog_p->tx.info.channel);

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }
        }
        else
        {
            __logError("Invalid source: %d, %d -> destination: %d, %d",
                       blog_p->rx.info.phyHdrType, blog_p->rx.info.channel,
                       blog_p->tx.info.phyHdrType, blog_p->tx.info.channel);

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }
    }

    port_p = &context_p->ucast.port;

#if defined(CONFIG_BCM_ARCHER_SIM)
    {
        struct net_device *dev_p = (struct net_device *)
            blog_p->tx_dev_p;

        context_p->dev_xmit = (sysport_classifier_dev_xmit)
            dev_p->netdev_ops->ndo_start_xmit;

        context_p->tx_dev_p = blog_p->tx_dev_p;
    }
#else
    if(BLOG_IS_TX_HWACC_ENABLED_WLAN_PHY(blog_p->tx.info.phyHdrType))
    {
        ret = archer_ucast_wlan_egress_queue_wfd(blog_p, flow_p);
        if(ret)
        {
            return ret;
        }
    }
    else
#endif
    {
        if(SYSPORT_RSB_PHY_IS_ETH(context_p->egress_phy))
        {
            int switch_queue = SKBMARK_GET_Q_PRIO(blog_p->mark);
            archer_drop_config_t drop_config;
            int txq_index;

            if(sysport_driver_switch_queue_to_txq_index(context_p->egress_port,
                                                        switch_queue, &txq_index))
            {
                __logError("Could not sysport_driver_switch_queue_to_txq_index");

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            port_p->egress_queue = txq_index;

            ret = sysport_driver_drop_config_get(context_p->egress_port,
                                                 switch_queue, &drop_config);
            if(ret)
            {
                __logError("Could not sysport_driver_drop_config_get");

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            context_p->drop_profile =
                archer_drop_profile_by_tc(&drop_config, SKBMARK_GET_TC_ID(blog_p->mark));
        }
#if defined(CC_ARCHER_DSL)
        else if(SYSPORT_RSB_PHY_DSL == context_p->egress_phy)
        {
            archer_drop_config_t drop_config;

            port_p->egress_queue = blog_p->tx.info.channel;

            ret = iudma_tx_dropAlg_get(port_p->egress_queue, &drop_config);
            if(ret)
            {
                __logError("Could not iudma_tx_dropAlg_get");

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }

            context_p->drop_profile =
                archer_drop_profile_by_tc(&drop_config, SKBMARK_GET_TC_ID(blog_p->mark));
        }
#endif
        else
        {
            BCM_ASSERT(0);
        }
    }

    __debug("ingress_phy %s, ingress_port %d, egress_phy %s, egress_port %d, egress_queue %d\n",
            sysport_rsb_phy_name[header_p->ingress_phy], header_p->ingress_port,
            sysport_rsb_phy_name[context_p->egress_phy], context_p->egress_port,
            port_p->egress_queue);

    return 0;
}

static int archer_ucast_flow_l3_set(Blog_t *blog_p, sysport_classifier_flow_t *flow_p,
                                    sysport_rsb_flow_tuple_info_t *flow_info_p)
{
    sysport_rsb_flow_ucast_l3_t *ucast_l3_p = &flow_p->tuple.ucast_l3;
    sysport_classifier_flow_context_t *context_p = &flow_p->context;

    /* L4 protocol is set in the Blog Key for both IPv4 and IPv6 */
    ucast_l3_p->ip_protocol = blog_p->key.protocol;

    __debug("protocol %u\n", ucast_l3_p->ip_protocol);

#if defined(CONFIG_BLOG_IPV6)
    if(blog_p->rx.info.bmap.PLD_IPv6 && !(T4in6DN(blog_p)))
    {
        ucast_l3_p->ip_src_addr = sysport_parser_crc32(blog_p->tupleV6.saddr.p8,
                                                       BLOG_IPV6_ADDR_LEN);
        ucast_l3_p->ip_dst_addr = sysport_parser_crc32(blog_p->tupleV6.daddr.p8,
                                                       BLOG_IPV6_ADDR_LEN);
        ucast_l3_p->l4_ports.src_port = ntohs(blog_p->tupleV6.port.source);
        ucast_l3_p->l4_ports.dst_port = ntohs(blog_p->tupleV6.port.dest);

        ucast_l3_p->ip_tos = blog_p->rx.tuple.tos & SYSPORT_PARSER_CONFIG_UCAST_L3_TOS_MASK;

        flow_info_p->l3.is_ipv6 = 1;
        memcpy(flow_info_p->l3.ipv6.src_addr.u8, blog_p->tupleV6.saddr.p8, BLOG_IPV6_ADDR_LEN);
        memcpy(flow_info_p->l3.ipv6.dst_addr.u8, blog_p->tupleV6.daddr.p8, BLOG_IPV6_ADDR_LEN);

        __debug("IPv6 Src " IP6PHEX " (0x%08x)\n", IP6(blog_p->tupleV6.saddr.p8),
                ucast_l3_p->l4_ports.src_port, ucast_l3_p->ip_src_addr);

        __debug("IPv6 Dst " IP6PHEX " (0x%08x)\n", IP6(blog_p->tupleV6.daddr.p8),
                ucast_l3_p->l4_ports.dst_port,  ucast_l3_p->ip_dst_addr);
    }
    else
#endif
    {
        ucast_l3_p->ip_src_addr = ntohl(blog_p->rx.tuple.saddr);
        ucast_l3_p->ip_dst_addr = ntohl(blog_p->rx.tuple.daddr);
        ucast_l3_p->l4_ports.src_port = ntohs(blog_p->rx.tuple.port.source);
        ucast_l3_p->l4_ports.dst_port = ntohs(blog_p->rx.tuple.port.dest);

        ucast_l3_p->ip_tos = blog_p->rx.tuple.tos & SYSPORT_PARSER_CONFIG_UCAST_L3_TOS_MASK;

        memset(flow_info_p, 0, sizeof(sysport_rsb_flow_tuple_info_t));

        __debug("IPv4 Src <%pI4:%u>\n", &blog_p->rx.tuple.saddr,
                ucast_l3_p->l4_ports.src_port);

        __debug("IPv4 Dst <%pI4:%u>\n", &blog_p->rx.tuple.daddr,
                ucast_l3_p->l4_ports.dst_port);
    }

    __debug("\n");

    /* Check if the flow is routed or bridged */

    context_p->is_routed = 0;

#if defined(CONFIG_BLOG_IPV6)
    if(MAPT(blog_p))
    {
        context_p->is_routed = 1;

        context_p->expected_ip_df = blog_p->is_df;

        if(MAPT_UP(blog_p))
        {
            context_p->check_ip_df = 1;
        }
    }
    else if(CHK4in6(blog_p) || CHK6in4(blog_p) || MAPT(blog_p))
    {
        context_p->is_routed = 1;
    }
    else if(CHK6to6(blog_p))
    {
        if(blog_p->tupleV6.rx_hop_limit != blog_p->tupleV6.tx_hop_limit)
        {
            context_p->is_routed = 1;
        }
    }
    else
#endif
    {
        if(CHK4to4(blog_p))
        {
            if(blog_p->rx.tuple.ttl != blog_p->tx.tuple.ttl)
            {
                context_p->is_routed = 1;
            }
        }
        else if(RX_GRE(blog_p) || TX_GRE(blog_p))
        {
            if(blog_p->rx.tuple.ttl != blog_p->tx.tuple.ttl)
            {
                context_p->is_routed = 1;
            }
        }
        else
        {
            __logError("Unable to determine if the flow is routed or bridged");

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }
    }

    context_p->mtu = blog_getTxMtu(blog_p);

    return 0;
}

int archer_ucast_brcm_tag_get(sysport_classifier_flow_t *flow_p,
                              cmdlist_brcm_tag_t *brcm_tag_p)
{
    sysport_classifier_flow_context_t *context_p = &flow_p->context;
    sysport_classifier_flow_port_t *port_p = &context_p->ucast.port;
    int nbr_of_brcm_tags;

    if(SYSPORT_RSB_PHY_IS_ETH(context_p->egress_phy))
    {
        if(sysport_driver_nbr_of_brcm_tags_get(context_p->egress_port,
                                               port_p->egress_queue, &nbr_of_brcm_tags))
        {
            __logError("Could not sysport_driver_nbr_of_brcm_tags_get");

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }
    }
    else
    {
        nbr_of_brcm_tags = 0;
    }

    switch(nbr_of_brcm_tags)
    {
        case 2:
            *brcm_tag_p = CMDLIST_BRCM_TAG_PT_DUAL;
            break;

        case 1:
            *brcm_tag_p = CMDLIST_BRCM_TAG_PT_SINGLE;
            break;

        default:
            *brcm_tag_p = CMDLIST_BRCM_TAG_NONE;
    }

    return 0;
}

#if defined(CC_SYSPORT_SW_PADDING)
void archer_ucast_padding_len_set(sysport_classifier_flow_t *flow_p, int tx_adjust)
{
    sysport_classifier_flow_context_t *context_p = &flow_p->context;

    if(SYSPORT_RSB_PHY_IS_ETH(context_p->egress_phy))
    {
        int padding_len = SYSPORT_DRIVER_ENET_PADDING_LEN;

        if(tx_adjust > 0)
        {
            padding_len += tx_adjust;
        }

        context_p->ucast.padding_len = padding_len;
    }
}
#endif

int archer_ucast_activate(Blog_t *blog_p, sysport_flow_key_t *flow_key_p,
                          uint8_t *prependData_p, int prependSize)
{
    sysport_classifier_flow_t flow;
    sysport_rsb_flow_tuple_info_t flow_info;
    sysport_classifier_rsb_overwrite_t rsb_overwrite;
    sysport_cmdlist_table_t cmdlist;
    cmdlist_brcm_tag_t brcm_tag;
    void *buffer_p = NULL;
    int cmdlist_data_length;
    int cmdlist_length;
    int tx_adjust;
    int ret;

    ip_addr_table_index_g = SYSPORT_IP_ADDR_TABLE_INVALID;

    if((blog_p->key.protocol != IPPROTO_UDP) &&
       (blog_p->key.protocol != IPPROTO_TCP) &&
       (blog_p->key.protocol != IPPROTO_IPV6) &&
       (blog_p->key.protocol != IPPROTO_IPIP) &&
       (blog_p->key.protocol != IPPROTO_GRE))
    {
        __logInfo("Flow Type proto<%d> is not supported", blog_p->key.protocol);

        ret = SYSPORT_CLASSIFIER_ERROR_INVALID;

        goto abort_activate;
    }

    sysport_classifier_rsb_overwrite_init(&rsb_overwrite);

    ret = archer_ucast_common_flow_set(blog_p, &flow,
                                       SYSPORT_RSB_FLOW_TYPE_UCAST_L3,
                                       &rsb_overwrite);
    if(ret)
    {
        __logInfo("Could not archer_ucast_common_flow_set");

        goto abort_activate;
    }

    ret = archer_ucast_flow_l3_set(blog_p, &flow, &flow_info);
    if(ret)
    {
        __logInfo("Could not archer_ucast_flow_l3_set");

        goto abort_activate;
    }

    ret = archer_ucast_brcm_tag_get(&flow, &brcm_tag);
    if(ret)
    {
        __logInfo("Could not archer_ucast_brcm_tag_get");

        goto abort_activate;
    }

    cmdlist_init(cmdlist.cmdlist, CMDLIST_CMD_LIST_SIZE_MAX, 0);

    ret = cmdlist_ucast_create(blog_p, CMDLIST_CMD_TARGET_SRAM,
                               prependData_p, prependSize, &buffer_p, brcm_tag, &tx_adjust);
    if(ret)
    {
        __logInfo("Could not cmdlist_create");

        goto abort_activate;
    }

#if defined(CC_SYSPORT_SW_PADDING)
    archer_ucast_padding_len_set(&flow, tx_adjust);
#endif
    flow.context.ip_addr_index = ip_addr_table_index_g;

    cmdlist_length = cmdlist_get_length(&cmdlist_data_length);

    __debug("cmdlist_length = %u, data_length = %u\n", cmdlist_length, cmdlist_data_length);
    if(cmdlist_length)
    {
        __dump_cmdlist(cmdlist.cmdlist);
    }

    ret = sysport_classifier_flow_create(&flow, &flow_info, cmdlist.cmdlist,
                                         cmdlist_length, flow_key_p, &rsb_overwrite);
    if(ret)
    {
        __logInfo("Could not sysport_classifier_flow_create");

        goto abort_activate;
    }

#if defined(CC_ARCHER_SIM_FC_HOOK)
    archer_sim_fc_hook_set(blog_p);
#endif

    return 0;

abort_activate:
    if(ip_addr_table_index_g != SYSPORT_IP_ADDR_TABLE_INVALID)
    {
        if(sysport_classifier_ip_addr_table_delete(ip_addr_table_index_g))
        {
            __logError("Could not sysport_classifier_ip_addr_table_delete (index %d)",
                       ip_addr_table_index_g);
        }
    }

    return ret;
}

int archer_ucast_deactivate(sysport_flow_key_t flow_key)
{
    sysport_classifier_flow_t flow;
    int ret;

    ret = sysport_classifier_flow_get(flow_key, &flow);
    if(ret)
    {
        __logError("Could not sysport_classifier_flow_get (flow_key 0x%08X)",
                   flow_key.u32);
        return ret;
    }

    if(flow.context.ip_addr_index != SYSPORT_IP_ADDR_TABLE_INVALID)
    {
        ret = sysport_classifier_ip_addr_table_delete(flow.context.ip_addr_index);
        if(ret)
        {
            __logError("Could not sysport_classifier_ip_addr_table_delete (index %d)",
                       flow.context.ip_addr_index);
            return ret;
        }
    }

    ret = sysport_classifier_flow_delete(flow_key);
    if(ret)
    {
        __logError("Could not sysport_classifier_flow_delete (flow_key 0x%08X)",
                   flow_key.u32);
        return ret;
    }

    return 0;
}
