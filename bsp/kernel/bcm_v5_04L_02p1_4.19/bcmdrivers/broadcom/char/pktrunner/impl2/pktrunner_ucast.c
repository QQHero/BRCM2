/*
<:copyright-BRCM:2013:proprietary:standard

   Copyright (c) 2013 Broadcom 
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
* File Name  : ptkrunner_ucast.c
*
* Description: This implementation translates Unicast Blogs into Runner Flows
*              for xDSL platforms.
*
*******************************************************************************
*/
#if defined(RDP_SIM)
#include "pktrunner_rdpa_sim.h"
#else
#include <linux/module.h>
#include <linux/bcm_log.h>
#include <linux/blog.h>
#include <net/ipv6.h>
#include <bcmtypes.h>
#include <bcmdpi.h>
#include "fcachehw.h"
#include "bcmxtmcfg.h"
#endif

#include <rdpa_api.h>
//#include <rdpa_flow_id_pool.h>

#include "cmdlist_api.h"
#include "pktrunner_proto.h"
#include "pktrunner_host.h"
#include "pktrunner_ucast.h"
#include "pktrunner_mcast.h"

#if defined(CONFIG_BCM_CMDLIST_SIM)
#include "runner_sim.h"
#endif

#if !defined(CONFIG_BCM_CMDLIST_SIM)
bdmf_object_handle ucast_class = NULL;
#endif

#ifdef RDP_UFC_TUNNEL
tunnel_info  tunnel_table[TUNNEL_MAX_NUMBER];

#define PACKET_HEADER_LENGTH          110

void tunnel_table_init(void)
{
    static uint8_t first_time_init = 1;
    uint8_t entry_id;

    if (!first_time_init)
        return;

    for (entry_id = 0; entry_id < TUNNEL_MAX_NUMBER; entry_id++) 
    {
        tunnel_table[entry_id].index = BDMF_INDEX_UNASSIGNED;
        tunnel_table[entry_id].key = TUNNEL_EMPTY_FIELD;
        tunnel_table[entry_id].ref_num = 0;
    }
    first_time_init = 0;
    bcm_print("Initialized Tunnel table key\n");
}

static int _tunnel_entry_ref_num_inc(uint8_t index)
{
    tunnel_table[index].ref_num++;
    return tunnel_table[index].ref_num;
}

static int _tunnel_entry_ref_num_dec(uint8_t index)
{
    BCM_ASSERT(tunnel_table[index].ref_num != 0);

    tunnel_table[index].ref_num--;
    return tunnel_table[index].ref_num;
}

static int _tunnel_entry_find(bdmf_index flow_index, uint32_t key, uint8_t *index)
{
    uint8_t entry_id;
    for (entry_id = 0; entry_id < TUNNEL_MAX_NUMBER; entry_id++) 
    {
        if ((tunnel_table[entry_id].index == flow_index) && (tunnel_table[entry_id].key == key))
        {
            *index = entry_id;
            return 0;
        }
    }
    return -1;
}

static int _tunnel_entry_empty_find(uint8_t *index)
{
    uint8_t entry_id;
    for (entry_id = 0; entry_id < TUNNEL_MAX_NUMBER; entry_id++) 
    {
        if ((tunnel_table[entry_id].index == BDMF_INDEX_UNASSIGNED) && (tunnel_table[entry_id].key == TUNNEL_EMPTY_FIELD))
        { 
            *index = entry_id;
            return 0;
        }
    }
    return -1;
}

int tunnel_entry_update(bdmf_index flow_index, uint32_t key, uint8_t *index)
{
    uint8_t entry_id;
    int rc;

    rc = _tunnel_entry_find(flow_index, key, &entry_id);
    if (!rc)
    {
        _tunnel_entry_ref_num_inc(entry_id);
        *index = entry_id;
        return 0;
    }
    if (!_tunnel_entry_empty_find(&entry_id))
    {
        tunnel_table[entry_id].index = flow_index;
        tunnel_table[entry_id].key = key;
        tunnel_table[entry_id].ref_num = 1;  
        *index = entry_id;
        return 0;
    }
    return -1;
}

static int _tunnel_entry_is_empty(uint8_t index)
{
    if ((tunnel_table[index].index == BDMF_INDEX_UNASSIGNED) && (tunnel_table[index].key == TUNNEL_EMPTY_FIELD))
        return 1;
    return 0;    
}

int tunnel_entry_remove(uint8_t index, bdmf_index *flow_index, uint32_t *key)
{
    if (_tunnel_entry_is_empty(index))
        return -1;

    *flow_index = tunnel_table[index].index;
    *key = tunnel_table[index].key;

    if (_tunnel_entry_ref_num_dec(index) == 0)
    {
        tunnel_table[index].index = BDMF_INDEX_UNASSIGNED;
        tunnel_table[index].key = TUNNEL_EMPTY_FIELD;
    }
    return 0;
}

static int _tunnel_key_flow_update(rdpa_ip_flow_info_t *ip_flow_tunnel, uint32_t key, bdmf_index rdpa_flow_index)
{
    uint8_t index = MAX_NUM_OF_KEYS_PER_TUNNEL_FLOW;
    int rc, entry_id;
    uint32_t *tunnel_key = ip_flow_tunnel->result.tunnel_key;
    
    for (entry_id = (MAX_NUM_OF_KEYS_PER_TUNNEL_FLOW - 1); entry_id >= 0; entry_id--) 
    {
        if (tunnel_key[entry_id] == CAM_LKP_EMPTY_ENTRY)
            index = entry_id;
        if (tunnel_key[entry_id] == key) 
            break;
    }
    if (entry_id < 0)
    {
        if (index < MAX_NUM_OF_KEYS_PER_TUNNEL_FLOW)
        {    
            tunnel_key[index] = key;
            rc = rdpa_ucast_flow_set(ucast_class, rdpa_flow_index, ip_flow_tunnel);
            if (rc != 0)
            {
                __logError("Cannot rdpa_ucast_flow_write for tunnel");
                return -1; 
            }
        }
        else
        {
            __logError("No place for key tunnel in table");
            return -1; 
        }
    }
    return 0;
}

int tunnel_construct_flow(Blog_t *blog_p, rdpa_ip_flow_info_t *ip_flow_tunnel, uint8_t *tunnel_index_ref)
{
    bdmf_index index_flow_tunnel = FHW_TUPLE_INVALID;
    bdmf_index rdpa_flow_index;
    rdpa_ip_flow_info_t ip_flow_tunnel_old;
    int32_t entry_id, rc;
    uint32 key = TUNNEL_EMPTY_FIELD;

    rc = rdpa_ucast_flow_find(ucast_class, &rdpa_flow_index, ip_flow_tunnel);
    if (rc == 0)
    {
        /* tunnel tuple exist -> update tunnel key */
        rdpa_ucast_flow_get(ucast_class, rdpa_flow_index, &ip_flow_tunnel_old);
        if (RX_GRE_KEY_ENABLED(blog_p))
            key = blog_p->grerx.key;
        else if (RX_VXLAN(blog_p))
            key = blog_p->vxlan.vni << 8;
        else
            key =  CAM_LKP_EMPTY_ENTRY;       

        if (key != CAM_LKP_EMPTY_ENTRY)
        {
            rc = _tunnel_key_flow_update(&ip_flow_tunnel_old, key, rdpa_flow_index);
            if (rc != 0)
            {
                __logError("Cannot update tunnel key");
                return -1;
            }
        }
        if (tunnel_entry_update(rdpa_flow_index, key, tunnel_index_ref))
        {
            __logError("Cannot add tunnel entry");
            return -1;
        }
    }
    else
    {
        /* new tunnel flow */
        for (entry_id = 0; entry_id < MAX_NUM_OF_KEYS_PER_TUNNEL_FLOW; entry_id++) 
            ip_flow_tunnel->result.tunnel_key[entry_id] = CAM_LKP_EMPTY_ENTRY;

        if (RX_GRE_KEY_ENABLED(blog_p))
            ip_flow_tunnel->result.tunnel_key[0] = blog_p->grerx.key;
        else if (RX_VXLAN(blog_p))
            ip_flow_tunnel->result.tunnel_key[0] = blog_p->vxlan.vni << 8;

        /* new tuple tunnel */
        rc = rdpa_ucast_flow_add(ucast_class, &index_flow_tunnel, ip_flow_tunnel);
        if (rc != 0)
        {
            __logError("Cannot rdpa_ucast_flow_add for tunnel");
            return -1;
        }
        if (tunnel_entry_update(index_flow_tunnel, ip_flow_tunnel->result.tunnel_key[0], tunnel_index_ref))
        {
            __logError("Cannot add tunnel entry");
            return -1;
        }
    }
    return 0;
}

int tunnel_destruct_flow(uint8_t tunnel_index_ref, uint32_t rdpa_flow_key)
{
    bdmf_index flow_index = FHW_TUPLE_INVALID;
    rdpa_ip_flow_info_t ip_flow_tunnel;
    uint32_t key;
    uint8_t empty_entry_num = 0, entry_id;
    int rc;

    rc = tunnel_entry_remove(tunnel_index_ref, &flow_index, &key);
    if (rc)
    {
        __logError("Cannot find tunnel flow ref (index %d)", tunnel_index_ref);
        return -1;
    }

    /* Do not remove tunnel since still in use by other flows */
    if (!_tunnel_entry_is_empty(tunnel_index_ref))
        return 0;

    /* tunnel tuple exist / key is removed -> update tunnel key */
    rc = rdpa_ucast_flow_get(ucast_class, flow_index, &ip_flow_tunnel);
    if(rc < 0)
    {
        __logError("Cannot get tunnel flow context (index %lx)", flow_index);
        return -1;
    }

    for (entry_id = 0; entry_id < MAX_NUM_OF_KEYS_PER_TUNNEL_FLOW; entry_id++) 
    {
        if (ip_flow_tunnel.result.tunnel_key[entry_id] == CAM_LKP_EMPTY_ENTRY)
            empty_entry_num++;
        else if (ip_flow_tunnel.result.tunnel_key[entry_id] == key)
        {
            ip_flow_tunnel.result.tunnel_key[entry_id] = CAM_LKP_EMPTY_ENTRY;
            empty_entry_num++;
        }
    }
    if (empty_entry_num == MAX_NUM_OF_KEYS_PER_TUNNEL_FLOW )
    {
        /* all tunnel_key entries are empty, flow should be deleted */
        rc = rdpa_ucast_flow_delete(ucast_class, flow_index);
        if(rc < 0)
        {
            __logError("Cannot rdpa_ucast_flow_delete tunnel (rdpa_flow_key %u)", rdpa_flow_key);
            return rc;
        }
    }
    else
    {
        /* Other keys still exist -> remove specific tunnel key in context array */
        rc = rdpa_ucast_flow_set(ucast_class, flow_index, &ip_flow_tunnel);
        if(rc < 0)
        {
            __logError("Cannot set tunnel flow context (index %lx)", flow_index);
            return -1;
        }
    }

    return 0;
}

#endif

static int ip_addresses_table_index_g;

#if !defined(CONFIG_BCM_CMDLIST_SIM)
int runnerUcast_ipv6_addresses_table_add(Blog_t *blog_p, uint32_t *table_sram_address_p)
{
    rdpa_ip_addresses_table_t ipAddr_table;
    bdmf_index index;
    int ret;

    ipAddr_table.src_addr.family = bdmf_ip_family_ipv6;
    ipAddr_table.dst_addr.family = bdmf_ip_family_ipv6;

    memcpy(ipAddr_table.src_addr.addr.ipv6.data,
           blog_p->tupleV6.saddr.p8,
           sizeof(bdmf_ipv6_t));

    memcpy(ipAddr_table.dst_addr.addr.ipv6.data,
           &blog_p->tupleV6.saddr.p8[sizeof(bdmf_ipv6_t)],
           sizeof(bdmf_ipv6_t));

    ret = rdpa_ucast_ip_addresses_table_add(ucast_class, &index, &ipAddr_table);
    if(ret < 0)
    {
        __logInfo("Could not rdpa_ucast_ip_addresses_table_add");

        return ret;
    }

    ip_addresses_table_index_g = index;

    *table_sram_address_p = ipAddr_table.sram_address;

    return 0;
}

int runnerUcast_ipv4_addresses_table_add(Blog_t *blog_p, uint32_t *table_sram_address_p)
{
    rdpa_ip_addresses_table_t ipAddr_table;
    bdmf_index index;
    int ret;

    /* Add the IPv4 tunnel addresses to Runner's IP Adrresses Table,
       so FW can verify the tunnel's IPv4 SA */

    ipAddr_table.src_addr.family = bdmf_ip_family_ipv4;
    ipAddr_table.dst_addr.family = bdmf_ip_family_ipv4;

    ipAddr_table.src_addr.addr.ipv4 = blog_p->rx.tuple.saddr;
    ipAddr_table.dst_addr.addr.ipv4 = blog_p->rx.tuple.daddr;

    ret = rdpa_ucast_ip_addresses_table_add(ucast_class, &index, &ipAddr_table);
    if(ret < 0)
    {
        __logInfo("Could not rdpa_ucast_ip_addresses_table_add");

        return ret;
    }

    ip_addresses_table_index_g = index;

    *table_sram_address_p = ipAddr_table.sram_address;

    return 0;
}
#endif /* !CONFIG_BCM_CMDLIST_SIM */
int pktrunner_ucast_cmdlist_create(Blog_t *blog_p, uint8_t *prependData_p, int prependSize, 
                                   void **cmdlist_buffer_pp, rdpa_ip_flow_result_t *result_p)
{
#if defined(BCM63158)
    cmdlist_cmd_target_t target = CMDLIST_CMD_TARGET_SRAM;
#else
    cmdlist_cmd_target_t target = CMDLIST_CMD_TARGET_DDR;
#endif
    int cmd_list_data_length;
    int tx_adjust;
    int err;

    ip_addresses_table_index_g = RDPA_UCAST_IP_ADDRESSES_TABLE_INDEX_INVALID;
    result_p->ip_addresses_table_index = RDPA_UCAST_IP_ADDRESSES_TABLE_INDEX_INVALID;

    cmdlist_init(result_p->cmd_list, RDPA_CMD_LIST_UCAST_LIST_SIZE,
                 RDPA_CMD_LIST_UCAST_LIST_OFFSET);

    err = cmdlist_ucast_create(blog_p, target, prependData_p, prependSize,
                               cmdlist_buffer_pp, PKTRUNNER_BRCM_TAG_MODE, &tx_adjust);
    if(err)
    {
        __logInfo("Could not cmdlist_create");

        return err;
    }

    result_p->cmd_list_length = cmdlist_get_length(&cmd_list_data_length);
    result_p->cmd_list_data_length = cmd_list_data_length;
    result_p->tx_adjust = tx_adjust;
    result_p->pathstat_idx = blog_p->hw_pathstat_idx;

    __debug("cmd_list_length = %u, data_length = %u\n",
            result_p->cmd_list_length, cmd_list_data_length);
    __dumpCmdList(result_p->cmd_list);

    {
        /* Current mcast implementation in firmware uses the last 32Byte of context SRAM space
         * to store the client vector, so we cannot allow >96 bytes of cmdlist. */
#if defined(CONFIG_BCM_CMDLIST_XPE)
        int len_to_cmp = result_p->cmd_list_data_length;
#else
        int len_to_cmp = result_p->cmd_list_length;
#endif
        if (blog_p->rx.multicast && len_to_cmp > 96)
        {
            __logError("Multicast cmdlist len=%d >96 not allowed",len_to_cmp);
            return -1;
        }
    }

    /* ip_addresses_table_index gets set as part of command list creation.*/
#if defined(CC_PKTRUNNER_IPV6)
    if(T4in6UP(blog_p) || T6in4DN(blog_p))
    {
        result_p->ip_addresses_table_index = ip_addresses_table_index_g;
    }
#endif
    return err;
}

int runnerUcast_activate(Blog_t *blog_p, uint8_t *prependData_p, int prependSize, void **cmdlist_buffer_pp, int *err)
{
    int flowIdx = FHW_TUPLE_INVALID;
    rdpa_ip_flow_info_t ip_flow = {};
#ifdef RDP_UFC_TUNNEL
    rdpa_ip_flow_info_t ip_flow_tunnel = {};
#endif

    memset(&ip_flow, 0, sizeof(rdpa_ip_flow_info_t));

    *err = pktrunner_ucast_cmdlist_create(blog_p, prependData_p, prependSize, cmdlist_buffer_pp, &ip_flow.result);
    if(*err != 0)
    {
        __logInfo("Could not ucast_cmdlist_create");

        goto abort_activate;
    }
#ifdef RDP_UFC_TUNNEL
    if (GRE_TERMINATION(blog_p) || T4in6DN(blog_p) || RX_VXLAN(blog_p))
    {
        ip_flow_tunnel.result.cmd_list_data_length = TUNNEL_FLOW_CONTEXT_SIZE;
        ip_flow_tunnel.result.cmd_list_length = TUNNEL_FLOW_CONTEXT_SIZE;
        ip_flow_tunnel.result.is_tunnel = 1;

        if (RX_VXLAN(blog_p))
        {
            /* check if inner packet info is found in packet buffer for second parsing */
            int l4_size = (blog_p->key.protocol == IPPROTO_TCP) ? sizeof(struct tcphdr) : sizeof(struct udphdr);
            int l3_size = (blog_p->rx.info.bmap.PLD_IPv6) ? sizeof(struct ipv6hdr) : sizeof(struct iphdr);
            int l4_offset_end = blog_p->vxlan.length + blog_p->rx.length + l3_size + l4_size;

            if( l4_offset_end > PACKET_HEADER_LENGTH )
            {
                __logInfo("RX_VXLAN - no information to parse inner packet");
                goto abort_activate;
            }
        }

        *err = __ucastSetFwdAndFilters(blog_p, &ip_flow_tunnel);
        if(*err != 0)
        {
            __logInfo("Could not setFwdAndFilters for tunnel");

            goto abort_activate;
        }
    }
#endif
    *err = __ucastSetFwdAndFilters(blog_p, &ip_flow);
    if(*err != 0)
    {
        __logInfo("Could not setFwdAndFilters");

        goto abort_activate;
    }


#if defined(CONFIG_BCM_CMDLIST_SIM)
    {
        int skip_brcm_tag_len = 0;

        if(PKTRUNNER_BRCM_TAG_MODE != CMDLIST_BRCM_TAG_NONE)
        {
            skip_brcm_tag_len = (pktrunner_is_rx_enet_wan_port(blog_p)&&
                                 !__isTxWlanPhy(blog_p)) ? BRCM_TAG_TYPE2_LEN : 0;
        }

        flowIdx = runnerSim_activate(blog_p, ip_flow.result.cmd_list, RDPA_CMD_LIST_UCAST_LIST_OFFSET,
                                     NULL, 0, skip_brcm_tag_len, cmd_list_data_length, tx_adjust);
    }
#else
    {
        bdmf_index index_flow = FHW_TUPLE_INVALID;
#ifdef RDP_UFC_TUNNEL
        uint8_t tunnel_index_ref = TUNNEL_INVALID_NUMBER;
        if (GRE_TERMINATION(blog_p) || T4in6DN(blog_p) || RX_VXLAN(blog_p))
        {
            *err = tunnel_construct_flow(blog_p, &ip_flow_tunnel, &tunnel_index_ref);
            if (*err != 0)
            {
                __logError("Cannot tunnel_construct_flow");
                goto abort_activate;
            }
            __logInfo("[tunnel_construct_flow] tunnel_index_ref: %d", tunnel_index_ref);
        }

        ip_flow.key.vtag_num = blog_p->vtag_num;

        ip_flow.result.tunnel_index_ref = tunnel_index_ref;
        __logInfo("Cannot rdpa_ucast_flow_add: collision list full");
#endif
        *err = rdpa_ucast_flow_add(ucast_class, &index_flow, &ip_flow);
        if (*err != 0)
        {
            __logError("Cannot rdpa_ucast_flow_add");

            goto abort_activate;
        }
        else if(index_flow == FHW_TUPLE_INVALID)
        {
            __logInfo("Cannot rdpa_ucast_flow_add: collision list full");

            goto abort_activate;
        }
        __logInfo("[rdpa_ucast_flow_add] index_flow: %ld", index_flow);

        flowIdx = (int)index_flow;
    }
#endif /* CONFIG_BCM_CMDLIST_SIM */

    return flowIdx;

abort_activate:
    if(*cmdlist_buffer_pp)
    {
        cmdlist_buffer_free(*cmdlist_buffer_pp);
    }

#if !defined(CONFIG_BCM_CMDLIST_SIM)
    if(ip_flow.result.ip_addresses_table_index != RDPA_UCAST_IP_ADDRESSES_TABLE_INDEX_INVALID)
    {
        int ret;

        ret = rdpa_ucast_ip_addresses_table_delete(ucast_class, ip_flow.result.ip_addresses_table_index);
        if(ret < 0)
        {
            __logError("Cannot rdpa_ucast_ip_addresses_table_delete (index %d)",
                       ip_flow.result.ip_addresses_table_index);
        }
    }
#endif

    return FHW_TUPLE_INVALID;
}

#if !defined(CONFIG_BCM_CMDLIST_SIM)
static int ucastDeleteFlow(uint32_t rdpa_flow_key, int speculative, void *cmdlist_buffer_p)
{
    rdpa_ip_flow_info_t ip_flow;
    int rc;

    rc = rdpa_ucast_flow_get(ucast_class, rdpa_flow_key, &ip_flow);
    if(rc < 0)
    {
        if(!speculative)
        {
            __logError("Cannot rdpa_ucast_flow_get (rdpa_flow_key %u)", rdpa_flow_key);
        }

        return rc;
    }

    if(ip_flow.result.ip_addresses_table_index != RDPA_UCAST_IP_ADDRESSES_TABLE_INDEX_INVALID)
    {
        rc = rdpa_ucast_ip_addresses_table_delete(ucast_class, ip_flow.result.ip_addresses_table_index);
        if(rc < 0)
        {
            __logError("Cannot rdpa_ucast_ip_addresses_table_delete (index %d)",
                       ip_flow.result.ip_addresses_table_index);

            return rc;
        }
    }
#ifdef RDP_UFC_TUNNEL
    if (ip_flow.result.tunnel_index_ref != TUNNEL_INVALID_NUMBER)
    {
        rc = tunnel_destruct_flow(ip_flow.result.tunnel_index_ref, rdpa_flow_key);
        if(rc < 0)
        {
            return rc;
        }
    }
#endif    
    rc = rdpa_ucast_flow_delete(ucast_class, rdpa_flow_key);
    if(rc < 0)
    {
        __logError("Cannot rdpa_ucast_flow_delete (rdpa_flow_key %u)", rdpa_flow_key);

        return rc;
    }

    if(cmdlist_buffer_p)
    {
        cmdlist_buffer_free(cmdlist_buffer_p);

        cmdlist_buffer_p = NULL;
    }

    return 0;
}

int runnerUcast_deactivate(uint32_t rdpa_flow_key, void *cmdlist_buffer_p)
{
    return ucastDeleteFlow(rdpa_flow_key, 0, cmdlist_buffer_p);
}

int runnerUcast_update(BlogUpdate_t update, uint32_t rdpa_flow_key, Blog_t *blog_p)
{
    rdpa_ip_flow_info_t ip_flow = {};

    rdpa_ucast_flow_get(ucast_class, rdpa_flow_key, &ip_flow);

    switch(update)
    {
        case BLOG_UPDATE_DPI_QUEUE:
            __set_dpiqos_flow_results(&ip_flow.result, blog_p);
            break;

        case BLOG_UPDATE_DPI_PRIORITY:
            __set_wlan_qos_flow_results(&ip_flow.result, blog_p);
            break;

        default:
            __logError("Invalid BLOG Update: <%d>", update);
            return -1;
    }

    return rdpa_ucast_flow_set(ucast_class, rdpa_flow_key, &ip_flow);
}

/*
 *------------------------------------------------------------------------------
 * Function   : runnerUcast_refresh
 * Description: This function is invoked to collect flow statistics
 * Parameters :
 *  flowIdx : 30bit index to refer to a Runner flow
 * Returns    : Total hits on this connection.
 *------------------------------------------------------------------------------
 */
int runnerUcast_refresh(uint32_t rdpa_flow_key, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    rdpa_stat_t flow_stat;
    int rc;

    rc = rdpa_ucast_flow_stat_get(ucast_class, rdpa_flow_key, &flow_stat);
    if (rc < 0)
    {
        //__logError("Could not get flowIdx<%d> stats, rc %d", flowIdx, rc);
        return rc;
    }

    *pktsCnt_p = flow_stat.packets; /* cummulative packets */
    *octetsCnt_p = flow_stat.bytes;

  //  __logDebug( "rdpa_flow_key<%03u> "
    //            "cumm_pkt_hits<%u> cumm_octet_hits<%u>\n",
      //          rdpa_flow_key, *pktsCnt_p, *octetsCnt_p );

    return 0;
}

#else /* CONFIG_BCM_CMDLIST_SIM */

int runnerUcast_deactivate(uint32_t rdpa_flow_key, void *cmdlist_buffer_p)
{
    return 0;
}

int runnerUcast_refresh(uint32_t rdpa_flow_key, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    *pktsCnt_p = 1;
    *octetsCnt_p = 1;

    return 0;
}

#endif /* CONFIG_BCM_CMDLIST_SIM */

/*
*******************************************************************************
* Function   : runnerUcast_construct
* Description: Constructs the Runner Protocol layer
*******************************************************************************
*/
int __init runnerUcast_construct(void *idx_p, void *disp_p)
{
#if !defined(CONFIG_BCM_CMDLIST_SIM)
    int ret;

    BDMF_MATTR(ucast_attrs, rdpa_ucast_drv());

#ifdef RDP_UFC_TUNNEL
    tunnel_table_init();
#endif    

    ret = rdpa_ucast_get(&ucast_class);
    if (ret)
    {
        ret = rdpa_ucast_flow_idx_pool_ptr_set(ucast_attrs, idx_p);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa ucast_class object cannot set rdpa_flow_idx_pool.\n");
            return ret;
        }
        ret = rdpa_ucast_flow_disp_pool_ptr_set(ucast_attrs, disp_p);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa ucast_class object cannot set flow_disp_pool_ptr.\n");
            return ret;
        }
        ret = bdmf_new_and_set(rdpa_ucast_drv(), NULL, ucast_attrs, &ucast_class);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa ucast_class object does not exist and can't be created.\n");
            return ret;
        }
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa ucast_class object already exists\n");
        return -1;
    }

#if !defined(RDP_SIM)
    runnerHost_construct();
#endif // !RDP_SIM
#else /* defined(CONFIG_BCM_CMDLIST_SIM) */
    runnerSim_init();
#endif
    bcm_print("Initialized Runner Unicast Layer\n");
    return 0;
}

/*
*******************************************************************************
* Function   : runnerUcast_destruct
* Description: Destructs the Runner Protocol layer
* WARNING: __exit_refok suppresses warnings from CONFIG_DEBUG_SECTION_MISMATCH
*          This should only be called from __init or __exit functions.
*******************************************************************************
*/
void __ref runnerUcast_destruct(void)
{
#if !defined(CONFIG_BCM_CMDLIST_SIM)
#if !defined(RDP_SIM)
    runnerHost_destruct();
#endif // !defined(RDP_SIM)
    if(ucast_class)
    {
        /* Ucast flow deletion will take place when ucast objects gets destroyed */
        bdmf_destroy(ucast_class);
        ucast_class = NULL;
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa ucast_class object is NULL\n");
    }
#endif /* !defined(CONFIG_BCM_CMDLIST_SIM) */
}
