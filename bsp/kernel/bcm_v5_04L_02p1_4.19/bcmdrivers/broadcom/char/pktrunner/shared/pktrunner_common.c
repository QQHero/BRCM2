/*
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom 
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

#if defined(RDP_SIM)
#include "pktrunner_rdpa_sim.h"
#else
#include <linux/module.h>

#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/blog_rule.h>

#include <bcmenet_common.h>
#include "linux/bcm_skb_defines.h"
#include <linux/bcm_log.h>
#include <linux/blog.h>
#include <net/ipv6.h>
#include "fcachehw.h"

#include "bcmtypes.h"
#include "bcm_vlan.h"
#include <rdpa_mw_blog_parse.h>
#endif

#include <rdpa_api.h>
#include "pktrunner_common.h"
#include <bcm_util_func.h>

int rx_if_get_by_blog(Blog_t *blog_p, int *is_wan, rdpa_if *rx_if)
{
    *is_wan = 1;
    *rx_if = rdpa_if_wan0;

#if !defined(RDP_SIM)
    if (blog_p->rx.info.phyHdrType == BLOG_GPONPHY)
    {
        __logDebug("source.phy GPON\n");
        *rx_if = rdpa_wan_type_to_if(rdpa_wan_gpon);
    }
    else if (blog_p->rx.info.phyHdrType == BLOG_EPONPHY)
    {
        __logDebug("source.phy EPON\n");
        *rx_if = rdpa_wan_type_to_if(rdpa_wan_epon);
    }
    else if(blog_p->rx.info.bmap.BCM_XPHY)
    {
        __logDebug("source.phy XTM\n");
        *rx_if = rdpa_wan_type_to_if(rdpa_wan_dsl);
    }
    else if (is_netdev_wan((struct net_device *)blog_p->rx_dev_p))
    {
        __logDebug("source.phy ETH WAN\n");
#if defined(CONFIG_BCM963146) || defined(CONFIG_BCM94912) || defined(CONFIG_BCM96813)
        /* TODO_AC: Find a solution to avoid these static mapping to rdpa_if */
        *rx_if = rdpa_mw_root_dev2rdpa_if((struct net_device *)blog_p->rx_dev_p);
#else
        *rx_if = rdpa_wan_type_to_if(rdpa_wan_gbe);
#endif
    }
    else
    {
        /* LAN */ 
        *is_wan = 0;
        if (blog_p->rx.info.phyHdrType == BLOG_ENETPHY)
        {
            __logDebug("source.phy ETH\n");
            *rx_if = rdpa_mw_root_dev2rdpa_if((struct net_device *)blog_p->rx_dev_p);
        }
        else if (blog_p->rx.info.phyHdrType == BLOG_WLANPHY)
        {
            __logDebug("source.phy WLAN\n");
            *rx_if = rdpa_mw_root_dev2rdpa_if((struct net_device *)blog_p->rx_dev_p);
        }
        else if (blog_p->rx.info.phyHdrType == BLOG_SPDTST)
        {
            *rx_if = rdpa_if_cpu;
        }
        else
        {
            __logError("LAN flow is not supported: Rx %u, Tx %u", blog_p->rx.info.phyHdrType,
                blog_p->tx.info.phyHdrType);
            return -1;
        }
    }
    __logDebug("rx-if <%s> rdpa_if <%d>\n", ((struct net_device *)blog_p->rx_dev_p)->name, *rx_if);

#else /* RDP_SIM */
    /* under simulator blog_p->rx_dev_p points to RDPA Port object */
    rdpa_port_index_get((bdmf_object_handle)(blog_p->rx_dev_p), rx_if);

    *is_wan = rdpa_if_is_wan(*rx_if);
#endif
    return 0;
}

void pktrunner_build_wlan_result(Blog_t *blog_p, rdpa_ip_flow_result_t *ip_flow_result)
{
    __logDebug("Blog info: is_tx_hw_acc_en %d (need accel), is_wfd %d, "
               "is_chain %d\n", blog_p->wfd.nic_ucast.is_tx_hw_acc_en,
               blog_p->wfd.nic_ucast.is_wfd, blog_p->wfd.nic_ucast.is_chain);

#if defined(CONFIG_BCM_DSL_XRDP) || defined(BCM_DSL_XRDP) || \
    defined(CONFIG_BCM_DSL_RDP) || defined(BCM_DSL_RDP) || \
    defined(BCM6855) || defined(CONFIG_BCM96855)
    /* queue_id not used for WLAN */
    ip_flow_result->queue_id = 0;
#endif

    /* is_wfd is shared between all unions so it could be filled in one place */
    ip_flow_result->wfd.nic_ucast.is_wfd = blog_p->wfd.nic_ucast.is_wfd;

    if (blog_p->wfd.nic_ucast.is_wfd)
    {
        /* is_chain, is_wmf_enabled, wfd_idx, and wfd_prio are shared between
         * all unions in the same location, so it could be filled in one place */
        ip_flow_result->wfd.nic_ucast.is_chain = blog_p->wfd.nic_ucast.is_chain;
        ip_flow_result->wfd.nic_ucast.wfd_idx = blog_p->wfd.nic_ucast.wfd_idx;
        ip_flow_result->wfd.nic_ucast.wfd_prio = blog_p->wfd.nic_ucast.wfd_prio;

        /* default is_wmf_enabled = 1, but clear it in multicast +
         * blog_p->is_wmf_enable == 0 case. */
        ip_flow_result->wfd.mcast.is_wmf_enabled = 1;

        if (blog_p->rx.multicast && !blog_p->wfd.nic_ucast.is_wmf_enabled)
        {
            ip_flow_result->wfd.mcast.ssid = blog_p->wfd.mcast.ssid;
#if defined(CONFIG_BCM_DSL_XRDP) || defined(BCM_DSL_XRDP) || \
    defined(CONFIG_BCM_DSL_RDP) || defined(BCM_DSL_RDP) || \
    defined(BCM6855) || defined(CONFIG_BCM96855)
            __logDebug("WFD Multicast, wfd_prio %d, wfd_idx %d, ssid %d\n",
                       blog_p->wfd.mcast.wfd_prio, blog_p->wfd.mcast.wfd_idx,
                       blog_p->wfd.mcast.ssid);

#else /* CONFIG_BCM_PON_XRDP */
            __logDebug("WFD Multicast, wfd_prio<%d>, wfd_ssid<%d(%d)>, "
                       "wfd_idx %d, result port %d<%d+radio>, skb_tx_prio<%d>\n",
                       blog_p->wfd.mcast.wfd_prio, blog_p->wfd.mcast.ssid,
                       ip_flow_result->ssid, blog_p->wfd.mcast.wfd_idx,
                       ip_flow_result->port, rdpa_if_wlan0,
                       (int)SKBMARK_GET_Q_PRIO(blog_p->mark));

            ip_flow_result->wfd.mcast.wfd_prio = SKBMARK_GET_Q_PRIO(blog_p->mark);
#endif
            ip_flow_result->wfd.nic_ucast.is_wmf_enabled = 0;
        }
        else if (blog_p->wfd.nic_ucast.is_chain)
        {
            __logDebug("WFD Chain info: wfd_prio %d, wfd_idx %d, priority %d, "
                       "chain_idx %d\n", blog_p->wfd.nic_ucast.wfd_prio,
                       blog_p->wfd.nic_ucast.wfd_idx, blog_p->wfd.nic_ucast.priority,
                       blog_p->wfd.nic_ucast.chain_idx);
            ip_flow_result->wfd.nic_ucast.priority = blog_p->wfd.nic_ucast.priority;
            ip_flow_result->wfd.nic_ucast.chain_idx = blog_p->wfd.nic_ucast.chain_idx;
#if defined(CONFIG_BCM_PON_XRDP) || defined(BCM_PON_XRDP)
            ip_flow_result->queue_id = blog_p->wfd.nic_ucast.wfd_idx;
#endif
        }
        else
        {
            __logDebug("WFD DHD info: wfd_prio %d, wfd_idx %d, priority %d, "
                       "ssid %d, flowring_idx %d\n", blog_p->wfd.dhd_ucast.wfd_prio,
                       blog_p->wfd.dhd_ucast.wfd_idx, blog_p->wfd.dhd_ucast.priority,
                       blog_p->wfd.dhd_ucast.ssid, blog_p->wfd.dhd_ucast.flowring_idx);
            ip_flow_result->wfd.dhd_ucast.ssid = blog_p->wfd.dhd_ucast.ssid;
            ip_flow_result->wfd.dhd_ucast.priority = blog_p->wfd.dhd_ucast.priority;
            ip_flow_result->wfd.dhd_ucast.flowring_idx = blog_p->wfd.dhd_ucast.flowring_idx;
#if defined(CONFIG_BCM_PON_XRDP) || defined(BCM_PON_XRDP)
            ip_flow_result->queue_id = blog_p->wfd.dhd_ucast.wfd_idx;
#endif
        }
    }
    else
    {
        __logDebug("DHD offload info: radio_idx %d, priority %d, ssid %d, "
                   "flowring_idx %d\n", blog_p->rnr.radio_idx,
                   blog_p->rnr.priority, blog_p->rnr.ssid, blog_p->rnr.flowring_idx);
#if defined(CONFIG_BCM_PON_XRDP) || defined(BCM_PON_XRDP)
        ip_flow_result->queue_id = blog_p->rnr.priority;
#endif
        ip_flow_result->rnr.radio_idx = blog_p->rnr.radio_idx;
        ip_flow_result->rnr.priority = blog_p->rnr.priority;
        ip_flow_result->rnr.ssid = blog_p->rnr.ssid;
        ip_flow_result->rnr.flowring_idx = blog_p->rnr.flowring_idx;
        ip_flow_result->rnr.flow_prio =  blog_p->rnr.flow_prio;
#if defined(CONFIG_BCM_DSL_RDP) || defined(BCM_DSL_RDP)
        /* remove it once DSL_RDP verify supporting llcsnap via GPE */
        ip_flow_result->rnr.llcsnap_flag = blog_p->rnr.llcsnap_flag;
#endif
    }
}

int pktrunner_blog_parse_mcast_group(Blog_t *blog_p, int is_l2, int include_vid, rdpa_iptv_channel_key_t *key)
{
    uint32_t vlan_id;

    if (is_l2)
    {
        uint32_t is_host = 0; 

        if (blog_p->rx.info.bmap.PLD_IPv4)
        {
            is_host = blog_p->rx.tuple.daddr & htonl(0xff000000);

            if (is_host == 0)
            {
                uint32_t daddr = htonl(blog_p->rx.tuple.daddr);
				
                key->mcast_group.mac.b[0] = 0x1;         
                key->mcast_group.mac.b[1] = 0x0;         
                key->mcast_group.mac.b[2] = 0x5e;         
                key->mcast_group.mac.b[3] = (daddr&0x7f0000)>>16; 
                key->mcast_group.mac.b[4] = (daddr&0xff00)>>8; 
                key->mcast_group.mac.b[5] = daddr&0xff; 
            }
            else
            {
                __logDebug("L2 multicast supported for Host control mode only");
                return BDMF_ERR_NOT_SUPPORTED;
            }
        }
        else if (blog_p->rx.info.bmap.PLD_IPv6)
        {
            is_host = blog_p->tupleV6.daddr.p8[0] & 0xff;

            if (is_host == 0)
            {
                key->mcast_group.mac.b[0] = 0x33;         
                key->mcast_group.mac.b[1] = 0x33;         
                key->mcast_group.mac.b[2] = blog_p->tupleV6.daddr.p8[12];
                key->mcast_group.mac.b[3] = blog_p->tupleV6.daddr.p8[13];
                key->mcast_group.mac.b[4] = blog_p->tupleV6.daddr.p8[14];
                key->mcast_group.mac.b[5] = blog_p->tupleV6.daddr.p8[15];
            }
            else
            {
                __logDebug("L2 multicast supported for Host control mode only");
                return BDMF_ERR_NOT_SUPPORTED;
            }
        }
    }
    else
    {
        /* Retrieve group and source IP addresses. */
        if (blog_p->rx.info.bmap.PLD_IPv4)
        {
            /* IGMP */
            key->mcast_group.l3.gr_ip.family = bdmf_ip_family_ipv4;
            key->mcast_group.l3.gr_ip.addr.ipv4 = htonl(blog_p->rx.tuple.daddr);
            key->mcast_group.l3.src_ip.family = bdmf_ip_family_ipv4;
            key->mcast_group.l3.src_ip.addr.ipv4 = htonl(blog_p->rx.tuple.saddr);
        }
        else
        {
            /* MLD. */
            key->mcast_group.l3.gr_ip.family = bdmf_ip_family_ipv6;
            memcpy(&key->mcast_group.l3.gr_ip.addr.ipv6, &blog_p->tupleV6.daddr, sizeof(bdmf_ipv6_t));
            key->mcast_group.l3.src_ip.family = bdmf_ip_family_ipv6;
            memcpy(&key->mcast_group.l3.src_ip.addr.ipv6, &blog_p->tupleV6.saddr, sizeof(bdmf_ipv6_t));
        }
    }
    if (include_vid)
    {
        vlan_id = ntohl(blog_p->vtag[0]);
        key->vid = vlan_id & RDPA_VID_MASK;
    }

    return 0;
}

#if !defined(RDP_SIM) || defined(RDP_UFC)
int blog_hdr_get_vtags_from_encap(BlogHeader_t *bHdr_p, uint16_t *etype, uint32_t *vtag0, uint32_t *vtag1, uint32_t is_mcast)
{
    int i = 0, length = 0, num_of_vtags = 0;
    char *value = (char *)bHdr_p->l2hdr;

    for (i = 0; i<bHdr_p->count; i++)
    {
        switch (bHdr_p->encap[i])
        {
        case PPP_1661:
            length = BLOG_PPP_HDR_LEN;
            break;
        case PPPoE_2516:
#if !defined(RDP_UFC)
            length = BLOG_PPPOE_HDR_LEN;
#else
            if (is_mcast)
                length = 0; /* we need PPPOE type instead of ETYPE for CRC construction */
            else
                length = BLOG_PPPOE_HDR_LEN;
#endif
            break;
        case VLAN_8021Q:
            length = BLOG_VLAN_HDR_LEN;
            *etype = *((uint16_t *)(value + length - 2));
            break;
        case ETH_802x:
            length = BLOG_ETH_HDR_LEN;
            *etype = *((uint16_t *)(value + length - 2));
            break;
#if !defined(RDP_SIM)
        case LLC_SNAP:
            length = BLOG_LLC_SNAP_8023_LEN;
            break;
#endif
        case BCM_SWC:
            if (*((uint16_t *)(bHdr_p->l2hdr + 12)) == htons(BLOG_ETH_P_BRCM4TAG))
                length = BLOG_BRCM4_HDR_LEN;
            else
                length = BLOG_BRCM6_HDR_LEN;
            break;
        default:
            length = -1;
            break;
        }
        if (length == -1)
            break;
        if (bHdr_p->encap[i] == VLAN_8021Q)
        {
            num_of_vtags++;
            if (num_of_vtags == 1)
            {
                if (is_mcast)
                {
                    *vtag0 = *(uint32_t *)(value - 2);
                }
                else
                {
                    *vtag0 = (uint32_t)(*(uint16_t *)(value));
                }
            }
            else if (num_of_vtags == 2)
            {
                if (is_mcast)
                {
                    *vtag1 = *(uint32_t *)(value - 2);
                }
                else
                {
                    *vtag1 = (uint32_t)(*(uint16_t *)(value));
                }
            }
            else
                break;
        }
        value += length;
    }

    return num_of_vtags;
}
#endif

uint32_t mcast_key_exclude_fields = 0;

int pktrunner_build_mcast_lkp_key(Blog_t *blog_p, rdpa_fc_mcast_flow_key_t *key)
{
#if defined(CONFIG_BCM_PON_XRDP)
    int is_wan;
#endif
    uint32_t vlan_id;
    uint16_t vtag_mask = (uint16_t)RDPA_VLAN_TCI_MASK; /* VID + PBIT */

    /* Parse group and source IP address
     * is_l2 = 0; include_vid = 0 */
    pktrunner_blog_parse_mcast_group(blog_p, 0, 1, &(key->key));

    if (mcast_key_exclude_fields & rdpa_mcast_flow_key_exclude_pbit_field)
        vtag_mask &= ~BLOG_RULE_PBITS_MASK;
    if (mcast_key_exclude_fields & rdpa_mcast_flow_key_exclude_dei_field)
        vtag_mask &= ~BLOG_RULE_DEI_MASK;
    __logDebug("vtag_mask <0x%x>\n", vtag_mask);

    /* Get other multi-flow key fields from the blog */
    key->entry_idx = blog_p->mcast_client_id; /* 0 for master */
    key->num_vlan_tags = blog_p->vtag_num;

    vlan_id = ntohl(blog_p->vtag[0]);
    key->outer_vlan = vlan_id & RDPA_VLAN_TCI_MASK; /* VID + PBIT */
    vlan_id = ntohl(blog_p->vtag[1]);
    key->inner_vlan = vlan_id & RDPA_VLAN_TCI_MASK; /* VID + PBIT */

    key->etype = ntohs(blog_p->eth_type);
    key->tos = blog_p->rx.tuple.tos;
#if defined(CONFIG_BCM_PON_XRDP)
    /* For DSL platforms rdpa_if is retrieved differently */
    if (rx_if_get_by_blog(blog_p, &is_wan, &key->rx_if))
        return -1;
#endif
#if  !defined(RDP_SIM) || defined(RDP_UFC)
    {
        /* WAR: Validate vtags from blog RX encap */
        uint16_t etype;
        uint32_t vtag0, vtag1;
        int num_of_vtags;

        num_of_vtags = blog_hdr_get_vtags_from_encap(&blog_p->rx, &etype, &vtag0, &vtag1, 1);

        key->etype = ntohs(etype);
        if (num_of_vtags > 0)
        {
            key->outer_vlan = ntohl(vtag0);
            __logDebug("Retrieved vtag0<0x%x : 0x%x> from blog_p->rx encap\n", vtag0, key->outer_vlan);
        }
        
        if (key->num_vlan_tags > 1)
        {
            if (num_of_vtags > 1)
            {
                key->inner_vlan = ntohl(vtag1);
                __logDebug("Retrieved vtag1<0x%x : 0x%x> from blog_p->rx encap\n", vtag1, key->inner_vlan);
            }
            else
            {
                BCM_LOGCODE(if(isLogDebug)
                    { bcm_printk("Unexpected RX encap, inner VID not included\n"); blog_dump(blog_p); });
            }
        }
    }
#endif
#if !defined(RDP_UFC)
    key->outer_vlan &= vtag_mask;
    key->inner_vlan &= vtag_mask;
#endif
    __logDebug("Flow Key:\n"
        "\tentry_idx<%d> etype<0x%04x> num_vlan_tags<%u> outer_vlan<0x%04x> inner_vlan<0x%04x>"
        "tos<%d> rx_if<%d>\n",
        key->entry_idx, key->etype, key->num_vlan_tags, key->outer_vlan, key->inner_vlan,
        key->tos, key->rx_if);
    return 0;
}

int mcast_flow_master_ctx_set_clients_vector(Blog_t *blog_p, rdpa_ip_flow_result_t *result)
{
    int i, ret;

    ret = blog_copy_mcast_client_bitmap(blog_p->mcast_bitmap_idx, result->clients_vector,
                                        sizeof(result->clients_vector)/4);
    if (ret)
    {
        __logError("Failed to copy mcast client bitmap\n");
    }
    for (i = 0; i < sizeof(result->clients_vector) / 4; i++)
    {
        __logDebug(" 0x%08x ", result->clients_vector[i]);
    }
    return ret;
}

pktRunner_data_t  pktRunner_data_g[PKTRNR_MAX_FHW_ACCEL];    /* Protocol layer global context */

#if !defined(RDP_SIM)
extern unsigned int UtilGetChipIsLP(void);
#endif
int __init _pktRunnerAccelInit(uint32_t accel, uint32_t num_flows, uint32_t cmd_list_ovrflw, uint32_t ref_count,
    uint32_t mcast_mode)
{
    int ret, idx;   
    pktRunner_data_t *pktRunner_p;
#if !defined(RDP_SIM)
    pktRunner_state_t *pktRunner_state_p;
#endif
    char owner_name[16];

    snprintf(owner_name, sizeof(owner_name), "PktRnr[%d]",accel);

    pktRunner_p = &pktRunner_data_g[accel];

    pktRunner_p->max_flow_idxs = num_flows;
    /* TODO : Need to use _wrap because rdpa_standalone/UT aren't including CE include file */
    ret = idx_pool_init_wrap(&pktRunner_p->idx_pool, num_flows, owner_name);
    if (ret)
    {
        return ret;
    }

    pktRunner_p->rdpa_flow_key_p = kmalloc(sizeof(*pktRunner_p->rdpa_flow_key_p) * num_flows, GFP_KERNEL);

    if (!pktRunner_p->rdpa_flow_key_p)
    {
        /* TODO : Need to use _wrap because rdpa_standalone/UT aren't including CE include file */
        idx_pool_exit_wrap(&pktRunner_p->idx_pool);
        return -1;
    }

    __pktRunnerFlowTypeInit(pktRunner_p);
    for (idx=0; idx < num_flows; idx++)
    {
        PKTRUNNER_RDPA_KEY(accel, idx) = FHW_TUPLE_INVALID;
        __pktRunnerFlowTypeSet(pktRunner_p, idx, PKTRNR_FLOW_TYPE_INV);
    }


    pktRunner_p->flowResetStats_p = kmalloc(sizeof(*pktRunner_p->flowResetStats_p)* num_flows, GFP_KERNEL);

    if (!pktRunner_p->flowResetStats_p)
    {
        /* TODO : Need to use _wrap because rdpa_standalone/UT aren't including CE include file */
        idx_pool_exit_wrap(&pktRunner_p->idx_pool);
        kfree(pktRunner_p->rdpa_flow_key_p);
        return -1;
    }
    memset(pktRunner_p->flowResetStats_p, 0x0, sizeof(*pktRunner_p->flowResetStats_p) * num_flows);

    if (ref_count)
    {
        pktRunner_p->ref_cnt_p = kmalloc(sizeof(*pktRunner_p->ref_cnt_p) * num_flows, GFP_KERNEL);
        if (!pktRunner_p->ref_cnt_p)
        {
            idx_pool_exit_wrap(&pktRunner_p->idx_pool);
            kfree(pktRunner_p->rdpa_flow_key_p);
            kfree(pktRunner_p->flowResetStats_p);
            return -1;
        }
        memset(pktRunner_p->ref_cnt_p, 0x0, sizeof(*pktRunner_p->ref_cnt_p) * num_flows);
    }

    if (cmd_list_ovrflw)
    {
        pktRunner_p->cmdlist_buffer_pointers = kmalloc(sizeof(*pktRunner_p->cmdlist_buffer_pointers) * num_flows, GFP_KERNEL);

        if (!pktRunner_p->cmdlist_buffer_pointers)
        {
            /* TODO : Need to use _wrap because rdpa_standalone/UT aren't including CE include file */
            idx_pool_exit_wrap(&pktRunner_p->idx_pool);
            kfree(pktRunner_p->rdpa_flow_key_p);
            kfree(pktRunner_p->flowResetStats_p);
            if (pktRunner_p->ref_cnt_p)
                kfree(pktRunner_p->ref_cnt_p);
            return -1;
        }
        memset(pktRunner_p->cmdlist_buffer_pointers, 0x0, sizeof(*pktRunner_p->cmdlist_buffer_pointers) * num_flows);
    }

    PKTRUNNER_STATE(accel).mcast_mode = mcast_mode;
#if !defined(RDP_SIM)
    if (UtilGetChipIsLP())
        pktRunner_state_p->max_flows = 704; /* 63132 Runner HW supports only 704 max flows */
#endif
    return ret;
}

int __exit _pktRunnerAccelExit(uint32_t accel)
{
    int ret;   
    pktRunner_data_t *pktRunner_p;

    pktRunner_p = &pktRunner_data_g[accel];


    ret = idx_pool_exit_wrap(&pktRunner_p->idx_pool);
    if (ret)
    {
        return ret;
    }

    kfree(pktRunner_p->rdpa_flow_key_p);
    kfree(pktRunner_p->flowResetStats_p);
    if (pktRunner_p->ref_cnt_p)
        kfree(pktRunner_p->ref_cnt_p);
    if (pktRunner_p->cmdlist_buffer_pointers)
    {
        kfree(pktRunner_p->cmdlist_buffer_pointers);
    }

    return ret;
}

#if !defined(RDP_SIM) && (defined(BCM63146) || defined(CONFIG_BCM963146) || \
                          defined(BCM4912) || defined(CONFIG_BCM94912) || \
                          defined(BCM6813) || defined(CONFIG_BCM96813) || \
                          defined(BCM6855) || defined(CONFIG_BCM96855))
int isRdpaGbeWanConfigured = 0;
#endif

int pktrunner_is_enet_wan_port(Blog_t *blog_p, int is_rx)
{
#if !defined(RDP_SIM) && (defined(BCM63146) || defined(CONFIG_BCM963146) || \
                          defined(BCM4912) || defined(CONFIG_BCM94912) || \
                          defined(BCM6813) || defined(CONFIG_BCM96813))
    if (!isRdpaGbeWanConfigured)
    {
        return FALSE;
    }
    else
#endif
    {
        return __isEnetWanPort(blog_p, is_rx);
    }
}

int pktrunner_is_enet_ext_sw_port(Blog_t *blog_p, int is_rx)
{
    BlogInfo_t *info = blog_p ? (is_rx? &blog_p->rx.info : &blog_p->tx.info) : NULL;
    
    return info && info->phyHdrType == BLOG_ENETPHY && IsExternalSwitchPort(info->channel);
}

int pktrunner_udpspdt_tx_start(Blog_t *blog_p)
{
#if defined(XRDP) && defined(CONFIG_BCM_SPDSVC_SUPPORT)
    bdmf_object_handle bdmf_udpspdtest_obj_h;
    uint32_t spdt_mark_type;
    int rc;

    spdt_mark_type = blog_p->spdt_so_mark & RDPA_UDPSPDTEST_SO_MARK_TYPE_MASK;
    if (spdt_mark_type != RDPA_UDPSPDTEST_SO_MARK_BASIC && spdt_mark_type != RDPA_UDPSPDTEST_SO_MARK_IPERF3)
        return -1;

    rc = rdpa_udpspdtest_get(&bdmf_udpspdtest_obj_h);
    if (rc)
        return -1;

    rc = rdpa_udpspdtest_tx_start_set(bdmf_udpspdtest_obj_h, blog_p->spdt_so_mark);
    bdmf_put(bdmf_udpspdtest_obj_h);
    return 0;
#else
    /* UDP speed test is not supported for RDP platforms */
    return -1;
#endif
}

