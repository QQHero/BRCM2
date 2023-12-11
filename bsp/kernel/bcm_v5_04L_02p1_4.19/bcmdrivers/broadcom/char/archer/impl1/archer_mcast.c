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
* File Name  : archer_mcast.c
*
* Description: Translation of Multicast Blogs into Multicast Archer Flows.
*
*******************************************************************************
*/
#include <linux/module.h>
#include <linux/bcm_log.h>
#include <linux/blog.h>
#include "bcmenet.h"
#include "pktHdr.h"

#include "sysport_rsb.h"
#include "sysport_parser.h"
#include "sysport_classifier.h"
#include "sysport_driver.h"

#include "archer.h"
#include "archer_driver.h"
#include "archer_drop.h"
#include "archer_xtmrt.h"
#include "bcm_archer_dpi.h"

#include "cmdlist_api.h"

#if (CONFIG_BCM_MAX_MCAST_CLIENTS_PER_GROUP > SYSPORT_FLOW_MCAST_CLIENTS_MAX)
#error "CONFIG_BCM_MAX_MCAST_CLIENTS_PER_GROUP > SYSPORT_FLOW_MCAST_CLIENTS_MAX"
#endif

/*******************************************************************************
 *
 * Local Functions
 *
 *******************************************************************************/

static int archer_mcast_flow_tuple_set(Blog_t *blog_p, sysport_classifier_flow_t *flow_p,
                                       sysport_rsb_flow_tuple_info_t *flow_info_p,
                                       sysport_classifier_rsb_overwrite_t *rsb_overwrite_p)
{
    sysport_rsb_flow_mcast_t *mcast_p = &flow_p->tuple.mcast;
    int vlan_id;

    mcast_p->header.valid = 1;
    mcast_p->header.flow_type = SYSPORT_RSB_FLOW_TYPE_MCAST;

#if defined(CC_ARCHER_DSL)
    if(blog_p->rx.info.bmap.BCM_XPHY)
    {
        mcast_p->header.ingress_phy = SYSPORT_RSB_PHY_DSL;
        mcast_p->header.ingress_port = blog_p->rx.info.channel;
    }
    else
#endif
    {
        if(ARCHER_RX_ENET(blog_p))
        {
            sysport_rsb_phy_t enet_ingress_phy;
            int enet_ingress_phys_port;
            int ret;

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

            mcast_p->header.ingress_phy = enet_ingress_phy;
            mcast_p->header.ingress_port = blog_p->rx.info.channel;
            rsb_overwrite_p->parser.header.ingress_port = enet_ingress_phys_port;
        }
        else if(ARCHER_RX_WLAN(blog_p))
        {
            mcast_p->header.ingress_phy = SYSPORT_RSB_PHY_WLAN;
            mcast_p->header.ingress_port = archer_ucast_wlan_ingress_port(blog_p);
        }
        else
        {
            __logError("Invalid Multicast Source PHY: %d", blog_p->rx.info.phyHdrType);

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }
    }

    if(blog_p->mcast_client_id >= SYSPORT_FLOW_MCAST_BITMAP_SIZE)
    {
        __logError("Invalid Client ID: %d, max %d",
                   blog_p->mcast_client_id, SYSPORT_FLOW_MCAST_BITMAP_SIZE);

        return SYSPORT_CLASSIFIER_ERROR_INVALID;
    }

    mcast_p->client_id = blog_p->mcast_client_id;

    mcast_p->ip_protocol = blog_p->key.protocol;

    mcast_p->ip_tos = blog_p->rx.tuple.tos;
    rsb_overwrite_p->parser.mcast.ip_tos = 0;

#if defined(CONFIG_BLOG_IPV6)
    if(blog_p->rx.info.bmap.PLD_IPv6)
    {
        mcast_p->ip_src_addr = sysport_parser_crc32(blog_p->tupleV6.saddr.p8,
                                                    BLOG_IPV6_ADDR_LEN);

        mcast_p->ip_dst_addr = sysport_parser_crc32(blog_p->tupleV6.daddr.p8,
                                                    BLOG_IPV6_ADDR_LEN);

        flow_info_p->l3.is_ipv6 = 1;

        memcpy(flow_info_p->l3.ipv6.src_addr.u8, blog_p->tupleV6.saddr.p8, BLOG_IPV6_ADDR_LEN);
        memcpy(flow_info_p->l3.ipv6.dst_addr.u8, blog_p->tupleV6.daddr.p8, BLOG_IPV6_ADDR_LEN);
    }
    else
#endif
    {
        mcast_p->ip_src_addr = ntohl(blog_p->rx.tuple.saddr);

        mcast_p->ip_dst_addr = ntohl(blog_p->rx.tuple.daddr);

        memset(flow_info_p, 0, sizeof(sysport_rsb_flow_tuple_info_t));
    }

    mcast_p->nbr_of_vlans = blog_p->vtag_num;

    vlan_id = ntohl(blog_p->vtag[0]);
    mcast_p->outer_vlan_id = vlan_id & 0xFFF;

    vlan_id = ntohl(blog_p->vtag[1]);
    mcast_p->inner_vlan_id = vlan_id & 0xFFF;

    return 0;
}

static int archer_mcast_flow_context_set(Blog_t *blog_p, sysport_classifier_flow_t *flow_p,
                                         sysport_cmdlist_table_t *cmdlist_p, int *cmdlist_length_p)
{
    sysport_classifier_flow_context_t *context_p = &flow_p->context;
    sysport_classifier_flow_port_t *port_p = &context_p->ucast.port;
    cmdlist_brcm_tag_t brcm_tag;
    void *buffer_p = NULL;
    int cmdlist_data_length;
    int tx_adjust;
    uint8_t is_routed;
    int ret;

    if(BLOG_MCAST_MASTER_CLIENT_ID == blog_p->mcast_client_id)
    {
        // Multicast Master Flow
        ret = blog_copy_mcast_client_bitmap(blog_p->mcast_bitmap_idx, 
                                            context_p->mcast.bitmap, 
                                            sizeof(context_p->mcast.bitmap)/4);

        if(ret)
        {
            __logError("Failed to copy mcast client bitmap");

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }
        context_p->mtu = blog_getTxMtu(blog_p);

        *cmdlist_length_p = 0;

        context_p->ip_addr_index = SYSPORT_IP_ADDR_TABLE_INVALID;

        return 0;
    }

    // Multicast Client Flow

    if(BLOG_WLANPHY == blog_p->tx.info.phyHdrType)
    {
        if(blog_p->wfd.mcast.is_wmf_enabled)
        {
            int egress_port;

            context_p->egress_phy = SYSPORT_RSB_PHY_WLAN;

            if((ret = archer_ucast_wlan_egress_port(blog_p, &egress_port)))
            {
                return ret;
            }

            context_p->egress_port = egress_port;

            ret = archer_ucast_wlan_egress_queue_wfd(blog_p, flow_p);
            if(ret)
            {
                return ret;
            }

            __debug("MCAST WLAN Client: WMF Enabled\n");
        }
        else
        {
            if(blog_p->wfd.mcast.is_wfd)
            {
                __logInfo("WLAN Multicast Client\n");

                if(blog_p->wfd.mcast.wfd_idx >= SYSPORT_FLOW_WLAN_PORTS_MAX)
                {
                    __logError("Invalid wfd_idx %d (max %d)",
                               blog_p->wfd.mcast.wfd_idx, SYSPORT_FLOW_WLAN_PORTS_MAX);

                    return SYSPORT_CLASSIFIER_ERROR_INVALID;
                }

                if(blog_p->wfd.mcast.wfd_prio >= SYSPORT_FLOW_WLAN_QUEUES_MAX)
                {
                    __logError("Invalid wfd_prio %d (max %d)",
                               blog_p->wfd.mcast.wfd_prio, SYSPORT_FLOW_WLAN_QUEUES_MAX);

                    return SYSPORT_CLASSIFIER_ERROR_INVALID;
                }

                if(blog_p->wfd.mcast.ssid >= SYSPORT_FLOW_WLAN_SSID_MAX)
                {
                    __logError("SSID %d exceeds the maximum number of SSIDs (%d)",
                               blog_p->wfd.mcast.ssid, SYSPORT_FLOW_WLAN_SSID_MAX);

                    return SYSPORT_CLASSIFIER_ERROR_INVALID;
                }

                context_p->egress_phy = SYSPORT_RSB_PHY_WLAN;

                context_p->egress_port = blog_p->wfd.mcast.wfd_idx;

                port_p->u32 = 0;

                // wfd_queue index = (wfd_idx << 1) + wfd_prio
                port_p->egress_queue = ((blog_p->wfd.mcast.wfd_idx * 2) +
                                        blog_p->wfd.mcast.wfd_prio);

                context_p->ucast.wlan.wfd.u32 = 0;
                context_p->ucast.wlan.wfd.mcast.wl_prio = SKBMARK_GET_Q_PRIO(blog_p->mark);
                context_p->ucast.wlan.wfd.mcast.ssid_vector = (1 << blog_p->wfd.mcast.ssid);

                __debug("MCAST WLAN Client: WMF Disabled\n");
            }
            else
            {
                __logError("Only WFD acceleration is supported");

                return SYSPORT_CLASSIFIER_ERROR_INVALID;
            }
        }
    }
#if defined(CC_ARCHER_DSL)
    else if(blog_p->tx.info.bmap.BCM_XPHY)
    {
        archer_drop_config_t drop_config;

        context_p->egress_phy = SYSPORT_RSB_PHY_DSL;

        context_p->egress_port = 0;

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
        archer_drop_config_t drop_config;
        sysport_rsb_phy_t enet_egress_phy;
        int switch_queue;
        int txq_index;
        int ret;

        __logInfo("Ethernet Multicast Client\n");

        ret = sysport_driver_logical_port_to_phy(blog_p->tx.info.channel,
                                                 &enet_egress_phy);
        if(ret)
        {
            return ret;
        }

        context_p->egress_phy = enet_egress_phy;

        context_p->egress_port = blog_p->tx.info.channel;

        switch_queue = SKBMARK_GET_Q_PRIO(blog_p->mark);

        if(sysport_driver_switch_queue_to_txq_index(context_p->egress_port, switch_queue, &txq_index))
        {
            __logError("Could not sysport_driver_switch_queue_to_txq_index");

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }

        port_p->egress_queue = txq_index;

        ret = sysport_driver_drop_config_get(context_p->egress_port, switch_queue, &drop_config);
        if(ret)
        {
            __logError("Could not sysport_driver_drop_config_get");

            return SYSPORT_CLASSIFIER_ERROR_INVALID;
        }

        context_p->drop_profile =
            archer_drop_profile_by_tc(&drop_config, SKBMARK_GET_TC_ID(blog_p->mark));
    }

    if(SKBMARK_GET_SQ_MARK(blog_p->mark))
    {
        context_p->dpi_queue = SKBMARK_GET_DPIQ_MARK(blog_p->mark);
    }
    else
    {
        context_p->dpi_queue = ARCHER_DPI_BYPASS_QUEUE;
    }

    context_p->mtu = blog_getTxMtu(blog_p);

#if defined(CONFIG_BCM_ARCHER_SIM)
    {
        struct net_device *dev_p = (struct net_device *)
            blog_p->tx_dev_p;

        context_p->dev_xmit = (sysport_classifier_dev_xmit)
            dev_p->netdev_ops->ndo_start_xmit;

        context_p->tx_dev_p = blog_p->tx_dev_p;
    }
#endif

    ret = archer_ucast_brcm_tag_get(flow_p, &brcm_tag);
    if(ret)
    {
        __logError("Could not archer_ucast_brcm_tag_get");

        return SYSPORT_CLASSIFIER_ERROR_INVALID;
    }

    /*
     * Create L2/L3 Command List
     */

    __debug("\n*** Command List ***\n\n");

    cmdlist_init(cmdlist_p->cmdlist, CMDLIST_CMD_LIST_SIZE_MAX, 0);

    ret = cmdlist_ucast_create(blog_p, CMDLIST_CMD_TARGET_SRAM,
                               NULL, 0, &buffer_p, brcm_tag, &tx_adjust);
    if(ret)
    {
        __logInfo("Could not cmdlist_ucast_create");

        return SYSPORT_CLASSIFIER_ERROR_INVALID;
    }

    *cmdlist_length_p = cmdlist_get_length(&cmdlist_data_length);

    __debug("cmdlist_length = %u, data_length = %u\n", *cmdlist_length_p, cmdlist_data_length);
    if(*cmdlist_length_p)
    {
        __dump_cmdlist(cmdlist_p->cmdlist);
    }

#if defined(CC_SYSPORT_SW_PADDING)
    archer_ucast_padding_len_set(flow_p, tx_adjust);
#endif
    context_p->ip_addr_index = SYSPORT_IP_ADDR_TABLE_INVALID;

    context_p->is_routed = is_routed;

    return 0;
}

int archer_mcast_activate(Blog_t *blog_p, sysport_flow_key_t *flow_key_p)
{
    sysport_classifier_flow_t flow;
    sysport_rsb_flow_tuple_info_t flow_info;
    sysport_classifier_rsb_overwrite_t rsb_overwrite;
    sysport_cmdlist_table_t cmdlist;
    int cmdlist_length;
    int ret;

    BCM_ASSERT(blog_p != BLOG_NULL);

    __logInfo("ACTIVATE");

    if((blog_p->key.protocol != IPPROTO_UDP) &&
       (blog_p->key.protocol != IPPROTO_TCP) &&
       (blog_p->key.protocol != IPPROTO_IPV6) &&
       (blog_p->key.protocol != IPPROTO_IPIP))
    {
        __logInfo("Flow Type proto<%d> is not supported", blog_p->key.protocol);

        return SYSPORT_CLASSIFIER_ERROR_INVALID;
    }

    if(!SYSPORT_PARSER_IS_MCAST_IPV4(ntohl(blog_p->rx.tuple.daddr))
#if defined(CONFIG_BLOG_IPV6)
       && !SYSPORT_PARSER_IS_MCAST_IPV6(blog_p->tupleV6.daddr.p8[0])
#endif
        )
    {
        __logError("Not IPv4 or IPv6 Multicast : %pI4, %pI6",
                   &blog_p->rx.tuple.daddr, blog_p->tupleV6.daddr.p8);

        return SYSPORT_CLASSIFIER_ERROR_INVALID;
    }

    if(blog_p->rx.info.channel == 0xFF)
    {
        __logInfo("LAN to LAN Multicast acceleration is not supported\n");

        return SYSPORT_CLASSIFIER_ERROR_INVALID;
    }

    __debug("\n%s: ************** Multicast Flow **************\n\n", __FUNCTION__);

    memset(&flow, 0, sizeof(sysport_classifier_flow_t));

    sysport_classifier_rsb_overwrite_init(&rsb_overwrite);

    ret = archer_mcast_flow_tuple_set(blog_p, &flow, &flow_info, &rsb_overwrite);
    if(ret)
    {
        __logError("Could not archer_mcast_flow_tuple_set");

        return ret;
    }

    ret = archer_mcast_flow_context_set(blog_p, &flow, &cmdlist, &cmdlist_length);
    if(ret)
    {
        __logError("Could not archer_mcast_flow_context_set");

        return ret;
    }

    /* Create new Flow */

    ret = sysport_classifier_flow_create(&flow, &flow_info, cmdlist.cmdlist,
                                         cmdlist_length, flow_key_p, &rsb_overwrite);
    if(ret)
    {
        __logInfo("Could not sysport_classifier_flow_create");

        return ret;
    }

#if defined(CC_ARCHER_SIM_FC_HOOK)
    archer_sim_fc_hook_set(blog_p);
#endif

    return 0;
}

int archer_mcast_deactivate(sysport_flow_key_t flow_key)
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

    ret = sysport_classifier_flow_delete(flow_key);
    if(ret)
    {
        __logError("Could not sysport_classifier_flow_delete (flow_key 0x%08X)",
                   flow_key.u32);
        return ret;
    }

    return 0;
}

int archer_mcast_wlan_dump(sysport_classifier_flow_t *flow_p)
{
    if(SYSPORT_RSB_FLOW_TYPE_MCAST == flow_p->tuple.header.flow_type)
    {
        wlFlowInf_t wfd = flow_p->context.ucast.wlan.wfd;

        if(wfd.mcast.is_ucast)
        {
            bcm_print("WMF Enabled: ");
        }
        else
        {
            int ssid = ffs(wfd.mcast.ssid_vector) - 1;

            bcm_print("WMF Disabled: egress_queue %d, ssid %d, wlan_priority %d\n",
                      flow_p->context.ucast.port.egress_queue, ssid, wfd.mcast.wl_prio);

            return 1;
        }
    }

    return 0;
}
