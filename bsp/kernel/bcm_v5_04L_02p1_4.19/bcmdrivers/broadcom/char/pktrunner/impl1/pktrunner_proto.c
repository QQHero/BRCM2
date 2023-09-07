/*
<:copyright-BRCM:2012:proprietary:standard

   Copyright (c) 2012 Broadcom
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
 * File Name  : pktrunner_proto.c
 *
 * Description: This file contains Linux character device driver entry points
 *              for the Runner packet Driver.
 *******************************************************************************
 * Todo:
 *   GRE support: fcache.c:_fc_ipproto( ip_protocol ) + rxTupleV4.ports = GRE_PORT
 *   priority and queues
 *   ...
 */

#if defined(RDP_SIM)
#include "pktrunner_rdpa_sim.h"
#else
//#include <bcmnet.h> /* SKBMARK_GET_Q_PRIO */
#include <bcmenet_common.h>
#include <linux/blog.h>
#include <linux/bcm_log.h>
#include <linux/blog_rule.h>
#include <linux/if.h>
#endif

#include "rdpa_api.h"
#if !defined(RDP_SIM)
#include "fcachehw.h"
#include "linux/bcm_skb_defines.h"
#endif

#include "rdpa_mw_blog_parse.h"


#include "rdpa_mw_qos.h"
#if !defined(CONFIG_ONU_TYPE_SFU)
#include "pktrunner_wlan.h"
#endif
#include "pktrunner_proto.h"
#include "pktrunner_l2_common.h"

#if !defined(RDP_SIM)
#include "idx_pool_util.h"
#endif

#include <rdpa_flow_idx_pool.h>

#if !defined(RDP_SIM)
#include <bcmdpi.h>
#endif
#include "pktrunner_common.h"
#include "pktrunner_mcast_whitelist.h"

// #define MCAST_FORWARD_TO_HOST

void build_gre_tunnel_header(Blog_t *blog_p, rdpa_tunnel_cfg_t *rdpa_tunnel);
void build_dslite_tunnel_header(Blog_t *blog_p, rdpa_tunnel_cfg_t *rdpa_tunnel);

int max_num_of_mcast_flows;

typedef union {
    bdmf_number    num;
    rdpa_stat_t    rdpastat;
} pktRunner_flowStat_t;

/* Used for single-flow mode.
 * TODO: Get rid of this DLL struct (worst case memory = 8+8+4+4+4 = 28 * 1024 = 28KB )
 * Move this to pktRunner_data_t (similar to UCAST in DSL (worst case memory = 4+4 = 8KB) */
struct mcast_hw_channel_key_entry
{
    DLIST_ENTRY(mcast_hw_channel_key_entry) list;
    uint16_t key;
    uint32_t request_idx; /* Channel index in RDD */
    int ref_cnt[MAX_NUM_VLAN_TAG + 1]; /* ref cnt per tag */
};

static rdpa_flow_idx_pool_t rdpa_shared_flw_idx_pool_g;  /* shared flow index pool as such has no relation
                                                          * with pktRunner but needed so can be shared across objects */

typedef struct mcast_hw_channel_key_entry mcast_hw_channel_key_entry_t;
DLIST_HEAD(mcast_hw_channel_key_list_t, mcast_hw_channel_key_entry);

typedef struct
{
    bdmf_ip_t dst_ip;
    bdmf_ip_t src_ip;
    uint32_t prot_key;
} tunnel_key;

struct tunnel_entry
{
    DLIST_ENTRY(tunnel_entry) list; 
    tunnel_key key;
    bdmf_object_handle p_tunnel;
    bdmf_number index;
};
typedef struct tunnel_entry tunnel_entry_t;
DLIST_HEAD(tunnel_entry_list_t, tunnel_entry);
struct tunnel_entry_list_t tunnel_entry_list;

bdmf_object_handle ip_class = NULL;
bdmf_object_handle system_obj = NULL;
static bdmf_object_handle iptv = NULL;
static bdmf_object_handle l2_class = NULL;
#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
#define MAX_NUM_OF_RADIOS 3
static bdmf_object_handle dhd_helper_obj[MAX_NUM_OF_RADIOS] = {};
#endif

extern pktRunner_data_t  pktRunner_data_g[PKTRNR_MAX_FHW_ACCEL];    /* Protocol layer global context */

#if defined(CONFIG_BCM_FHW)
static FC_CLEAR_HOOK fc_clear_hook_runner = (FC_CLEAR_HOOK)NULL;
#endif

rdpa_iptv_flow_mode_t mc_flow_mode;

uint8_t pathstat_idx_map[256] = {};

/* XXX: if possible, merge with bcmenet bcmtag code */
#pragma pack(push,1)
typedef struct
{
   uint32_t opcode :3 __PACKING_ATTRIBUTE_FIELD_LEVEL__; //opcode for ingress traffic is allways 1
   uint32_t tc :3 __PACKING_ATTRIBUTE_FIELD_LEVEL__; //traffic class for the ingress traffic
   uint32_t te :2 __PACKING_ATTRIBUTE_FIELD_LEVEL__; //tag enforcement at switch ingress
#define D_BCM_TAG_TE_NO_ENFORCE 0
#define D_BCM_TAG_TE_UNTAG_ENFORCE 1
#define D_BCM_TAG_TE_TAG_ENFORCE 2
   uint32_t ts :1 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ; //timestamp request
   uint32_t reserved :14 __PACKING_ATTRIBUTE_FIELD_LEVEL__; //nothing
   uint32_t dst_port :9 __PACKING_ATTRIBUTE_FIELD_LEVEL__; //destination port bitmap
} S_INGRESS_BCM_SWITCH_TAG;
#pragma pack(pop)

static S_INGRESS_BCM_SWITCH_TAG localBcmTag = {.opcode=1,.tc=0,.te=0,.ts=0,.reserved=0,.dst_port=0};

#if defined CC_PKTRUNNER_PROCFS
uint32_t pktRunnerGetState(struct seq_file *sf, uint32_t accel)
{
    if (accel == 0)
    {
        seq_printf(sf, "RUNNER-UCAST:\n");
    }
    else
    {
        seq_printf(sf, "RUNNER-MCAST:\n");
    }
    seq_printf(sf, "status         : %u\n",PKTRUNNER_STATE(accel).status);
    seq_printf(sf, "active         : %u\n",PKTRUNNER_STATE(accel).active);
    seq_printf(sf, "ucast_active   : %u\n",PKTRUNNER_STATE(accel).ucast_active);
    seq_printf(sf, "mcast_active   : %u\n",PKTRUNNER_STATE(accel).mcast_active);
    seq_printf(sf, "max_flows      : %u\n",PKTRUNNER_DATA(accel).max_flow_idxs);
    seq_printf(sf, "activates      : %u\n",PKTRUNNER_STATE(accel).activates);
    seq_printf(sf, "deactivates    : %u\n",PKTRUNNER_STATE(accel).deactivates);
    seq_printf(sf, "failures       : %u\n",PKTRUNNER_STATE(accel).failures);
    seq_printf(sf, "hw_add_fail    : %u\n",PKTRUNNER_STATE(accel).hw_add_fail);
    seq_printf(sf, "no_free_idx    : %u\n",PKTRUNNER_STATE(accel).no_free_idx);
    seq_printf(sf, "flushes        : %u\n",PKTRUNNER_STATE(accel).flushes);
    seq_printf(sf, "Flow_idx_in_use: %u\n",idx_pool_num_in_use(&PKTRUNNER_DATA(accel).idx_pool));
    seq_printf(sf, "----------------------------\n");

    return 0;
}
#endif /* CC_PKTRUNNER_PROCFS */

static void pktrunner_flow_tunnels_destroy(void)
{
    tunnel_entry_t *entry, *tmp_entry;

    DLIST_FOREACH_SAFE(entry, &tunnel_entry_list, list, tmp_entry)
    {
        bdmf_destroy(entry->p_tunnel);
        DLIST_REMOVE(entry, list);
        bdmf_free(entry);
    }
}

void pktrunner_flow_remove_all(void)
{
    if (ip_class)
    {
        rdpa_ip_class_flush_set(ip_class, 1);
    }
    if (l2_class)
    {
        rdpa_l2_class_flush_set(l2_class, 1);
    }
    pktrunner_flow_tunnels_destroy();
    if (iptv)
        rdpa_iptv_flush_set(iptv, 1);
}

static bdmf_boolean is_ecn_remark_set(void)
{
    rdpa_system_cfg_t system_cfg;
    int rc;
    bdmf_boolean is_ecn_remark = 0;

    if ((rc = rdpa_system_cfg_get(system_obj, &system_cfg) == BDMF_ERR_OK))
    {
    	is_ecn_remark = (system_cfg.options == 2);
    }
    return is_ecn_remark;
}

int runnerResetStats(uint32_t pktrunner_key)
{
    int           rc = -1;
    pktRunner_flowStat_t flow_stats;
    bdmf_index    idx = __pktRunnerFlowIdx(pktrunner_key);
    uint32_t      flow_type = __pktRunnerFlowType(pktrunner_key);
    uint32_t      accel = __pktRunnerAccelIdx(pktrunner_key);

    if (idx < 0 || idx >= PKTRUNNER_DATA(accel).max_flow_idxs)
    {
        return 1;
    }

    switch (flow_type) {
        case PKTRNR_FLOW_TYPE_MC:
            /* TODO */
            break;
        case PKTRNR_FLOW_TYPE_L3:
            if (!ip_class)
            {
                return 1;
            }
            rc = rdpa_ip_class_flow_stat_get(ip_class, PKTRUNNER_RDPA_KEY(accel,idx), &flow_stats.rdpastat);
            break;
        case PKTRNR_FLOW_TYPE_L2:
            if (!l2_class)
            {
                return 1;
            }
            rc = rdpa_l2_class_flow_stat_get(l2_class, PKTRUNNER_RDPA_KEY(accel,idx), &flow_stats.rdpastat);
            break;
        default:
            __logError("Failed to match flow_type =%d",flow_type);
            rc = -1;

    }
    if (rc)
        return 2;

    PKTRUNNER_STATS(accel,idx) = flow_stats.num;

    return 0;
}

/* This function is invoked when all entries pertaining to a entIx in runner need to be cleared */
int runner_clear(uint32_t pktrunner_key)
{
    return 0;
}

static int runner_refresh_ucast(uint32_t pktrunner_key, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    int rc, flowIdx = __pktRunnerFlowIdx(pktrunner_key);
    pktRunner_flowStat_t flowStat, resetFlowStat;
    uint32_t flow_type = __pktRunnerFlowType(pktrunner_key);
    uint32_t      accel = __pktRunnerAccelIdx(pktrunner_key);

    if (flow_type == PKTRNR_FLOW_TYPE_L2)
    {
        if (!l2_class)
        {
            return 0;
        }

        rc = rdpa_l2_class_flow_stat_get(l2_class, PKTRUNNER_RDPA_KEY(accel,flowIdx), &flowStat.rdpastat);
        if (rc < 0)
        {
            __logError("Could not get flowIdx<%d> stats", flowIdx);
            return rc;
        }
    }
    else
    {
        if (!ip_class)
        {
            return 0;
        }

        rc = rdpa_ip_class_flow_stat_get(ip_class, PKTRUNNER_RDPA_KEY(accel,flowIdx), &flowStat.rdpastat);
        if (rc < 0)
        {
            __logError("Could not get flowIdx<%d> stats", flowIdx);
            return rc;
        }
    }

    resetFlowStat.num = PKTRUNNER_STATS(accel, flowIdx);

    *pktsCnt_p = flowStat.rdpastat.packets - resetFlowStat.rdpastat.packets; /* cummulative packets */
    *octetsCnt_p = flowStat.rdpastat.bytes - resetFlowStat.rdpastat.bytes;

    __logDebug( "flowIdx<%03u> "
        "cumm_pkt_hits<%u> cumm_octet_hits<%u>\n",
        flowIdx, *pktsCnt_p, *octetsCnt_p );

    return 0;
}

static int runner_refresh_mcast(uint32_t pktrunner_key, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
#ifndef MCAST_FORWARD_TO_HOST
    int rc, flowIdx = __pktRunnerFlowIdx(pktrunner_key);
    pktRunner_flowStat_t flowStat, resetFlowStat;
    uint32_t accel = __pktRunnerAccelIdx(pktrunner_key);
    uint32_t rdpa_flow_idx = PKTRUNNER_RDPA_KEY(accel, flowIdx);

    rc = rdpa_iptv_flow_stat_get(iptv, rdpa_flow_idx, &flowStat.rdpastat);

    if (rc < 0)
    {
        if (rc == BDMF_ERR_NOENT)
        {
            __logNotice("flowIdx<%d>, channel_index<%d>, not exist!", flowIdx, rdpa_flow_idx);
        }
        else
        {
            __logError("Could not get flowIdx<%d> stats, rc<%d>", flowIdx, rc);
        }
        *pktsCnt_p = 0;
        *octetsCnt_p = 0;
        return rc;
    }

    resetFlowStat.num = PKTRUNNER_STATS(accel, flowIdx);

    *pktsCnt_p = flowStat.rdpastat.packets - resetFlowStat.rdpastat.packets; /* cummulative packets */
    *octetsCnt_p = flowStat.rdpastat.bytes - resetFlowStat.rdpastat.bytes;

    __logDebug( "flowIdx<%03u> "
        "cumm_pkt_hits<%u> cumm_octet_hits<%u>\n",
        flowIdx, *pktsCnt_p, *octetsCnt_p );
#endif

    return 0;
}

/* This function is invoked to check activity for a NATed flow */
int runner_refresh_flow(uint32_t pktrunner_key, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    uint32_t flow_type = __pktRunnerFlowType(pktrunner_key);

    if (flow_type == PKTRNR_FLOW_TYPE_MC)
        return runner_refresh_mcast(pktrunner_key, pktsCnt_p, octetsCnt_p);

    return runner_refresh_ucast(pktrunner_key, pktsCnt_p, octetsCnt_p);
}


/*
 *------------------------------------------------------------------------------
 * Function   : runner_refresh_pathstat
 * Description: This function is invoked to refresh path statistics
 * Parameters :
 *  pathIdx : 8bit index to refer to a path
 * Returns    : Total hits on this connection.
 *------------------------------------------------------------------------------
 */
static int runner_refresh_pathstat(uint32_t flow_type, int pathIdx, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    rdpa_stat_t pathstat;
    int rc = -1;

    if ((flow_type == PKTRNR_FLOW_TYPE_L3 && !ip_class)
        || (flow_type == PKTRNR_FLOW_TYPE_L2 && !l2_class))
    {
        *pktsCnt_p = 0;
        *octetsCnt_p = 0;
		
        return 0;
    }

    if (flow_type == PKTRNR_FLOW_TYPE_L3)
        rc = rdpa_ip_class_pathstat_get(ip_class, pathIdx, &pathstat);
    else if (flow_type == PKTRNR_FLOW_TYPE_L2)
        rc = rdpa_l2_class_pathstat_get(l2_class, pathIdx, &pathstat);
    if (rc < 0)
    {
        __logError("%s: Could not get pathIdx<%d> stats, rc %d", __FUNCTION__, pathIdx, rc);
        return rc;
    }

    *pktsCnt_p = pathstat.packets; /* collect packets */
    *octetsCnt_p = pathstat.bytes;

    __logDebug( "pathIdx<%03u> "
                "pkt_hits<%u> pkt_bytes<%u>\n",
                pathIdx, *pktsCnt_p, *octetsCnt_p );
    return 0;
}


/*
 *------------------------------------------------------------------------------
 * Function   : runnerRefreshPathStat
 * Description: This function is invoked to collect path statistics
 * Parameters :
 *  pathIdx : 16bit index to refer to a Runner path
 * Returns    : Total hits on this connection.
 *------------------------------------------------------------------------------
 */
int runnerRefreshPathStat(uint8_t pathIdx, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    int rc;
    rdpa_stat_t rawStat;
    uint32_t flow_type = pathstat_idx_map[pathIdx];

    if (flow_type == PKTRNR_FLOW_TYPE_INV)
        return -1;

    flow_type = pathstat_idx_map[pathIdx];
    if (flow_type == PKTRNR_FLOW_TYPE_MC)
        return -1; /* Not supported yet */

    rc = runner_refresh_pathstat(flow_type, pathIdx, &rawStat.packets, &rawStat.bytes);
    if (rc < 0)
    {
        __logDebug("%s: Could not get pathIdx<%d> stats, rc %d", __FUNCTION__, pathIdx, rc);
        return rc;
    }

    *pktsCnt_p = rawStat.packets;
    *octetsCnt_p = rawStat.bytes;

    return 0;
}


static inline int runner_deactivate_ucast(uint32_t pktrunner_key, uint32_t *pktsCnt_p,
	uint32_t *octetsCnt_p, struct blog_t *blog_p)
{
    int rc;
    rdpa_ip_flow_info_t flow_info = {};
    uint32_t flowIdx = __pktRunnerFlowIdx(pktrunner_key);
    uint32_t flow_type = __pktRunnerFlowType(pktrunner_key);
    rdpa_l2_flow_info_t l2_flow_info = {};
    bdmf_object_handle tunnel_obj = NULL;
    uint32_t accel = __pktRunnerAccelIdx(pktrunner_key);

    if (accel != PKTRUNNER_ACCEL_FLOW)
    {
        __logError("Failed to remove flow key<0x%08x>, unexpected accel type %d\n", pktrunner_key, accel);
        return -1;
    }
    if((flow_type == PKTRNR_FLOW_TYPE_L2) && l2_class)
    {
        rc = rdpa_l2_class_flow_get(l2_class, PKTRUNNER_RDPA_KEY(accel, flowIdx), &l2_flow_info);
        if (rc < 0)
        {
            __logError("Failed to get flow, key<0x%08x>, flowIdx<%d>, flow_type<%d>, rc %d\n",
              pktrunner_key, flowIdx, flow_type, rc);
            return rc;
        }
    }
    else if((flow_type == PKTRNR_FLOW_TYPE_L3) && ip_class)
    {
        rc = rdpa_ip_class_flow_get(ip_class, PKTRUNNER_RDPA_KEY(accel,flowIdx), &flow_info);
        if (rc < 0)
        {
            __logError("Failed to get flow, key<0x%08x>, flowIdx<%d>, flow_type<%d>, rc %d\n",
              pktrunner_key, flowIdx, flow_type, rc);
            return rc;
        }
    }
    /* Fetch last hit count */
    rc = runner_refresh_ucast(pktrunner_key, pktsCnt_p, octetsCnt_p);
    if (flow_type == PKTRNR_FLOW_TYPE_L2)
    {
        if (l2_class)
            rc = rc ? rc : rdpa_l2_class_flow_delete(l2_class, PKTRUNNER_RDPA_KEY(accel,flowIdx));
    }
    else
    {
        rc = rc ? rc : rdpa_ip_class_flow_delete(ip_class, PKTRUNNER_RDPA_KEY(accel,flowIdx));
    }

    if (rc < 0)
    {
        __logError("Failed to remove flow, key<0x%08x>, flowIdx<%d>, flow_type<%d>, rc %d\n",
          pktrunner_key, flowIdx, flow_type, rc);
        return rc;
    }
    if (flow_type == PKTRNR_FLOW_TYPE_L2)
        tunnel_obj = l2_flow_info.result.tunnel_obj;
    else
        tunnel_obj = flow_info.result.tunnel_obj;
    if (tunnel_obj != NULL)
    {
        bdmf_number ref_cnt, tunnel_index;
        rc = rdpa_tunnel_ref_cnt_get(tunnel_obj, &ref_cnt);
        if (rc < 0)
        {
            __logError("Failed to get ref_count from tunnel, rc %d\n", rc);
            return rc;
        }
        if (!ref_cnt)
        {
            tunnel_entry_t *entry, *tmp_entry;

            rdpa_tunnel_index_get(flow_info.result.tunnel_obj, &tunnel_index);
            DLIST_FOREACH_SAFE(entry, &tunnel_entry_list, list, tmp_entry)
            {
                if (entry->index == tunnel_index)
                {
                    if (entry->p_tunnel != flow_info.result.tunnel_obj)
                        __logError("entry->p_tunnel(%p) !=  flow_info.result.tunnel_obj(%p)\n", entry->p_tunnel, flow_info.result.tunnel_obj);
 
                    bdmf_destroy(entry->p_tunnel);
                    DLIST_REMOVE(entry, list);
                    bdmf_free(entry);
                    break;
                }
            }
        }
    }
    rc = idx_pool_return_index(&PKTRUNNER_DATA(accel).idx_pool, flowIdx);
    if (rc < 0)
    {
        __logError("Failed to free flow key<0x%08x>, rc %d\n", pktrunner_key, rc);
        return rc;
    }

    PKTRUNNER_RDPA_KEY(accel, flowIdx) = FHW_TUPLE_INVALID;
    PKTRUNNER_STATS(accel,flowIdx) = 0;
    __pktRunnerFlowTypeSet(&PKTRUNNER_DATA(accel), flowIdx, PKTRNR_FLOW_TYPE_INV);

    PKTRUNNER_STATE(accel).deactivates++;
    PKTRUNNER_STATE(accel).active--;
    PKTRUNNER_STATE(accel).ucast_active--;

    __logDebug(
        "::: runner_deactivate_ucast key<0x%08x>, flowIx<%03u>, flow_type<%d>, hits<%u> bytes<%u> cumm_deactivates<%u> :::\n",
        pktrunner_key, flowIdx, flow_type, *pktsCnt_p, *octetsCnt_p, PKTRUNNER_STATE(accel).deactivates);

    return 0;
}

/* This function is invoked when a flow in the runner needs to be deactivated */
static inline int runner_deactivate_mcast_flow_multi(uint32_t pktrunner_key, uint32_t *pktsCnt_p,
	uint32_t *octetsCnt_p, struct blog_t *blog_p)
{
    uint32_t flowIdx = __pktRunnerFlowIdx(pktrunner_key);
    uint32_t accel = __pktRunnerAccelIdx(pktrunner_key);
    uint32_t rdpa_flow_idx = PKTRUNNER_RDPA_KEY(accel, flowIdx);
    rdpa_fc_mcast_flow_t flow = {};
    int rc;

    rc = runner_refresh_mcast(pktrunner_key, pktsCnt_p, octetsCnt_p);
    if (rc >= 0)
    {
        rc = rdpa_iptv_flow_get(iptv, rdpa_flow_idx, &flow);
        if (rc)
        {
            __logError("Failed to find flow to delete, rdpa_flow_idx<%d>, rc<%d>",
                rdpa_flow_idx, rc);
            return -1;
        }
        if (rdpa_if_is_wifi(flow.result.port))
        {
            /* XXX: TODO - update master flow if needed (NIC mode). */
        }
        rc = rdpa_iptv_flow_delete(iptv, rdpa_flow_idx);
        __logDebug("delete flow, index<%d>, rc<%d>\n", rdpa_flow_idx, rc);
    }

    if (rc < 0)
    {
        __logError("Failed to remove flow key<0x%08x>, rc %d\n", pktrunner_key, rc);
        return rc;
    }

    return 0;
}

/* This function is invoked when a flow in the runner needs to be deactivated */
static inline int runner_deactivate_mcast(uint32_t pktrunner_key, uint32_t *pktsCnt_p,
	uint32_t *octetsCnt_p, struct blog_t *blog_p)
{
    uint32_t accel = __pktRunnerAccelIdx(pktrunner_key);
    uint32_t flowIdx = __pktRunnerFlowIdx(pktrunner_key);
    int more_ref, rc;

    if (accel != PKTRUNNER_ACCEL_FLOW)
    {
        __logError("Failed to remove flow key<0x%08x>, unexpected accel type %d\n", pktrunner_key, accel);
        return -1;
    }

    more_ref = runner_deactivate_mcast_flow_multi(pktrunner_key, pktsCnt_p, octetsCnt_p, blog_p);
    if (more_ref < 0)
    {
        __logDebug("Failed to invoke deactivate callback, rc = %d\n", more_ref);
        return more_ref;
    }

    if (!more_ref)
    {
        rc = idx_pool_return_index(&PKTRUNNER_DATA(accel).idx_pool, flowIdx);
        if (rc < 0)
        {
            __logError("Failed to free flow key<0x%08x>, rc %d\n", pktrunner_key, rc);
            return -1;
        }
        PKTRUNNER_RDPA_KEY(accel, flowIdx) = FHW_TUPLE_INVALID;
        PKTRUNNER_STATS(accel, flowIdx) = 0;
        __pktRunnerFlowTypeSet(&PKTRUNNER_DATA(accel), flowIdx, PKTRNR_FLOW_TYPE_INV);
    }

    PKTRUNNER_STATE(accel).deactivates++;
    PKTRUNNER_STATE(accel).active--;
    PKTRUNNER_STATE(accel).mcast_active--;

    __logDebug(
        "::: runner_deactivate_mcast key<0x%08x> hits<%u> bytes<%u> cumm_deactivates<%u> :::\n",
        pktrunner_key, *pktsCnt_p, *octetsCnt_p, PKTRUNNER_STATE(accel).deactivates);

    return more_ref;
}

/* This function is invoked when a flow in the runner needs to be deactivated */
static int runner_deactivate_flow(uint32_t pktrunner_key, uint32_t *pktsCnt_p,
	uint32_t *octetsCnt_p, struct blog_t *blog_p)
{
    return blog_p->rx.multicast ? runner_deactivate_mcast(pktrunner_key,  pktsCnt_p, octetsCnt_p, blog_p) :
        runner_deactivate_ucast(pktrunner_key, pktsCnt_p, octetsCnt_p, blog_p);
}


static void add_qos_commands(rdpa_traffic_dir dir, rdpa_ip_flow_result_t *ip_flow_result, Blog_t *blog_p)
{
    BOOL enable;
    bdmf_object_handle policer_obj = NULL;

    if (0 == rdpa_mw_pkt_based_qos_get(
        dir, RDPA_MW_QOS_TYPE_FC, &enable))
    {
        ip_flow_result->qos_method =
            enable? rdpa_qos_method_pbit : rdpa_qos_method_flow;
    }

    /* Assign policer */
    blog_parse_policer_get(blog_p, &policer_obj);
    ip_flow_result->policer_obj = policer_obj;
    if (policer_obj)
        bdmf_put(policer_obj);

    if (blog_p->dscp2q)
        ip_flow_result->qos_method = rdpa_qos_method_pbit;

    if (blog_p->dscp2pbit)
    {
        ip_flow_result->action_vec |= rdpa_fc_action_opbit_remark;
        ip_flow_result->opbit_action = rdpa_pbit_act_dscp_copy;
    }
}

static int pktrunner_l3_flow_key_construct(Blog_t *blog_p, rdpa_ip_flow_key_t *key)
{
    int rc, is_wan;
    uint16_t src_port = 0, dst_port = 0;

    if (blog_p->tx.info.phyHdrType == BLOG_GPONPHY || blog_p->tx.info.phyHdrType == BLOG_EPONPHY)
        key->dir = rdpa_dir_us;
    else if ((blog_p->tx.info.phyHdrType == BLOG_ENETPHY || blog_p->tx.info.phyHdrType == BLOG_WLANPHY ||
        blog_p->tx.info.phyHdrType == BLOG_NETXLPHY || blog_p->tx.info.phyHdrType == BLOG_SPDTST))
    {

        if (pktrunner_is_rx_enet_wan_port(blog_p))
            key->dir = rdpa_dir_ds;
        else
            key->dir = rdpa_dir_us;
    }

    rc = rx_if_get_by_blog(blog_p, &is_wan, &key->ingress_if);
    if (rc)
        return rc;

    if (!is_wan && blog_p->rx.info.phyHdrType == BLOG_SPDTST)
        key->dir = rdpa_dir_us;

    __logDebug("<dir:%s, ingress port:%d>", key->dir == rdpa_dir_us ? "us" : "ds", key->ingress_if);

    key->prot = blog_p->key.protocol;
    key->tcp_pure_ack = blog_p->key.tcp_pure_ack;
    if (blog_p->rx.info.bmap.PLD_IPv6 && !(T4in6DN(blog_p)))
    {
        /* This is a IPv6 flow */
        memcpy(&key->src_ip.addr.ipv6, &blog_p->tupleV6.saddr, sizeof(ip6_addr_t));
        memcpy(&key->dst_ip.addr.ipv6, &blog_p->tupleV6.daddr, sizeof(ip6_addr_t));
        if (blog_p->key.protocol == IPPROTO_TCP || blog_p->key.protocol == IPPROTO_UDP)
        {
            src_port = blog_p->tupleV6.port.source;
            dst_port = blog_p->tupleV6.port.dest;
        }
        key->src_ip.family = bdmf_ip_family_ipv6;
        key->dst_ip.family = bdmf_ip_family_ipv6;
    }
    else
    {
        /* This is a IPv4 flow/tunnel */
        key->src_ip.addr.ipv4 = htonl(blog_p->rx.tuple.saddr);
        key->dst_ip.addr.ipv4 = htonl(blog_p->rx.tuple.daddr);
        if (blog_p->key.protocol == IPPROTO_TCP || blog_p->key.protocol == IPPROTO_UDP)
        {
            src_port = blog_p->rx.tuple.port.source;
            dst_port = blog_p->rx.tuple.port.dest;
        }
        else if (blog_p->key.protocol == IPPROTO_GRE)
            dst_port = blog_p->rx.tuple.gre_callid;
        else if (blog_p->key.protocol == IPPROTO_ESP)
        {
#if !defined(CONFIG_CPU_BIG_ENDIAN)
            dst_port = blog_p->rx.tuple.esp_spi >> 16;
            src_port = blog_p->rx.tuple.esp_spi & 0xffff;
#else
            dst_port = blog_p->rx.tuple.esp_spi & 0xffff;
            src_port = blog_p->rx.tuple.esp_spi >> 16;
#endif
        }

        key->src_ip.family = bdmf_ip_family_ipv4;
        key->dst_ip.family = bdmf_ip_family_ipv4;
    }

    key->src_port = htons(src_port);
    key->dst_port = htons(dst_port);

    return 0;
}

static int add_gre_tunnel_commands(Blog_t *blog_p, rdpa_ip_flow_result_t *result)
{
    tunnel_key flow_tunnel_key = {};
    tunnel_entry_t *entry, *tmp_entry, *new_entry;
    int rc;

    result->action_vec |= rdpa_fc_action_gre_tunnel;

    if (TX_GIP46in4(blog_p))
    {
        flow_tunnel_key.dst_ip.family = bdmf_ip_family_ipv4;
        flow_tunnel_key.dst_ip.addr.ipv4 = blog_p->deltx_tuple.daddr;
        flow_tunnel_key.src_ip.family = bdmf_ip_family_ipv4;
        flow_tunnel_key.src_ip.addr.ipv4 = blog_p->deltx_tuple.saddr;
        flow_tunnel_key.prot_key = blog_p->gretx.key;
    }
    else
    {
        flow_tunnel_key.dst_ip.family = bdmf_ip_family_ipv6;
        memcpy(&flow_tunnel_key.dst_ip.addr.ipv6, &blog_p->del_tupleV6.daddr, 16);
        flow_tunnel_key.src_ip.family = bdmf_ip_family_ipv6;
        memcpy(&flow_tunnel_key.src_ip.addr.ipv6, &blog_p->del_tupleV6.saddr, 16);
        flow_tunnel_key.prot_key = blog_p->gretx.key;
    }

    DLIST_FOREACH_SAFE(entry, &tunnel_entry_list, list, tmp_entry)
    {
        if (memcmp(&(entry->key), &flow_tunnel_key, sizeof(tunnel_key)) == 0)
        {
            result->tunnel_obj = entry->p_tunnel;
            break;
        }
    }

    if (result->tunnel_obj == NULL)
    {
        BDMF_MATTR(tunnel_attrs, rdpa_tunnel_drv());
        rdpa_tunnel_cfg_t rdpa_tunnel;

        /* create new tunnel object */
        rc = bdmf_new_and_set(rdpa_tunnel_drv(), NULL, tunnel_attrs, &(result->tunnel_obj));
        if (rc)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "failed creating tunnel object\n");
            return -1;
        }

        build_gre_tunnel_header(blog_p, &rdpa_tunnel);

        rc = rdpa_tunnel_cfg_set(result->tunnel_obj, &rdpa_tunnel);
        if (rc)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "failed updating tunnel object\n");
            return -1;
        }

        new_entry = bdmf_alloc(sizeof(tunnel_entry_t));
        memcpy(&(new_entry->key), &flow_tunnel_key, sizeof(tunnel_key));
        new_entry->p_tunnel = result->tunnel_obj;
        rdpa_tunnel_index_get(result->tunnel_obj, &(new_entry->index)); 
        DLIST_INSERT_HEAD(&tunnel_entry_list, new_entry, list);
    }
    return 0;
}

static int add_fwd_commands(rdpa_ip_flow_result_t *result, Blog_t *blog_p, rdpa_traffic_dir dir)
{
    int rc;

    if (blog_p->tx.info.phyHdrType == BLOG_GPONPHY || blog_p->tx.info.phyHdrType == BLOG_EPONPHY)
    {
        result->wan_flow = blog_p->tx.info.channel;
        __logDebug("<port:gpon/epon,gemid/llid:%d>", result->wan_flow);
    }
    /* US GBE, DS and LAN<->WLAN */
    else if ((blog_p->tx.info.phyHdrType == BLOG_ENETPHY || blog_p->tx.info.phyHdrType == BLOG_WLANPHY ||
        blog_p->tx.info.phyHdrType == BLOG_NETXLPHY || blog_p->tx.info.phyHdrType == BLOG_SPDTST))
    {
#if !defined(CONFIG_ONU_TYPE_SFU)
        if (blog_p->tx.info.phyHdrType == BLOG_NETXLPHY)
        {
            result->port = rdpa_if_wlan0 + blog_p->rnr.radio_idx;
            result->ssid = blog_p->rnr.ssid;
        }
        else if (blog_p->tx.info.phyHdrType == BLOG_SPDTST)
        {
            uint8_t is_hw_mode = blog_p->spdtst_bits.is_hw; 

            result->port = rdpa_if_cpu;
            result->action = rdpa_forward_action_forward;
            result->trap_reason = rdpa_cpu_rx_reason_udef_7;
            result->is_tcpspdtest = is_hw_mode;
            result->tcpspdtest_is_upload = blog_p->spdtst_bits.is_dir_upload;
            result->spdtest_stream_id = blog_p->spdtst_bits.stream_idx;
            result->action_vec |= is_hw_mode ? 0 : rdpa_fc_action_no_forward;
        }
        else
        {
            result->port = rdpa_mw_root_dev2rdpa_if((struct net_device *)blog_p->tx_dev_p);
            __logDebug("LAN/WLAN result port %d\n", result->port);
            if (blog_p->tx.info.phyHdrType == BLOG_WLANPHY)
                result->ssid = rdpa_mw_root_dev2rdpa_ssid((struct net_device *)blog_p->tx_dev_p);
        }
#else
        result->port = rdpa_mw_root_dev2rdpa_if((struct net_device *)blog_p->tx_dev_p);
        __logDebug("LAN result port %d\n", result->port);
#endif
        __logDebug("<dir:%s, port:%d>", dir == rdpa_dir_us ? "us" : "ds", result->port);
    }
    else
    {
        __logDebug("Unsupported phy<%d>", blog_p->tx.info.phyHdrType);
        return -1;
    }

    if (blog_p->rx.info.phyHdrType == BLOG_SPDTST)
    {
        if (blog_p->spdtst_bits.is_hw)
        {
            /* To avoid drop of the packets by QM, mark the packets as generated by speed test. */
            result->action_vec |= rdpa_fc_action_spdt_gen;
        }
    }

#if !defined(CONFIG_ONU_TYPE_SFU)
    /* Assign queue (queue should be configured elsewhere of course),
       XXX: group is alway 0 SP */
    if(blog_p->tx.info.phyHdrType != BLOG_WLANPHY && blog_p->tx.info.phyHdrType != BLOG_NETXLPHY)
#endif
    	result->queue_id = SKBMARK_GET_Q_PRIO(blog_p->mark);

    if (CHK4in6(blog_p))
    {
        result->action_vec |= rdpa_fc_action_ttl | rdpa_fc_action_dslite_tunnel;
        if (dir == rdpa_dir_us)
        {
            tunnel_entry_t *entry, *tmp_entry, *new_entry;
            tunnel_key flow_tunnel_key = {};
            int rc;
           
            flow_tunnel_key.dst_ip.family = bdmf_ip_family_ipv6;
            memcpy(&flow_tunnel_key.dst_ip.addr.ipv6, &blog_p->tupleV6.daddr, 16);
            flow_tunnel_key.src_ip.family = bdmf_ip_family_ipv6;
            memcpy(&flow_tunnel_key.src_ip.addr.ipv6, &blog_p->tupleV6.saddr, 16);
            flow_tunnel_key.prot_key = 0;

            DLIST_FOREACH_SAFE(entry, &tunnel_entry_list, list, tmp_entry)
            {
                if (memcmp(&(entry->key), &flow_tunnel_key, sizeof(tunnel_key)) == 0)
                {
                    result->tunnel_obj = entry->p_tunnel;
                    break;
                }
            }

            if (result->tunnel_obj == NULL)
            {
                BDMF_MATTR(tunnel_attrs, rdpa_tunnel_drv());
                rdpa_tunnel_cfg_t rdpa_tunnel;

                /* create new tunnel object */
                rc = bdmf_new_and_set(rdpa_tunnel_drv(), NULL, tunnel_attrs, &(result->tunnel_obj));
                if (rc)
                {
                    BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "failed creating tunnel object\n");
                    return -1;
                }

                build_dslite_tunnel_header(blog_p, &rdpa_tunnel);

                rc = rdpa_tunnel_cfg_set(result->tunnel_obj, &rdpa_tunnel);
                if (rc)
                {
                    BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "failed updating tunnel object\n");
                    return -1;
                }

                new_entry = bdmf_alloc(sizeof(tunnel_entry_t));
                memcpy(&(new_entry->key), &flow_tunnel_key, sizeof(tunnel_key));
                new_entry->p_tunnel = result->tunnel_obj;
                rdpa_tunnel_index_get(result->tunnel_obj, &(new_entry->index)); 
                DLIST_INSERT_HEAD(&tunnel_entry_list, new_entry, list);
            }
        }
    }
    else if (CHK4to4(blog_p) || PTG4(blog_p) || PTE4(blog_p))
    {
        if (blog_p->rx.tuple.ttl != blog_p->tx.tuple.ttl)
            result->action_vec |= rdpa_fc_action_ttl;
    }
    else if (CHK6to6(blog_p))
    {
        if (blog_p->tupleV6.rx_hop_limit != blog_p->tupleV6.tx_hop_limit)
            result->action_vec |= rdpa_fc_action_ttl;
    }
    else if (MAPT(blog_p))
    {
        result->action_vec |= rdpa_fc_action_mapt;
        result->action_vec |= rdpa_fc_action_ttl;
        result->is_df = blog_p->is_df;

        if (MAPT_DN(blog_p))
        {
            result->mapt_cfg.tos_tc = blog_p->tx.tuple.tos;
            result->mapt_cfg.proto = blog_p->key.protocol;
            result->mapt_cfg.src_ip.family = bdmf_ip_family_ipv4;
            memcpy(&result->mapt_cfg.src_ip.addr.ipv4, &blog_p->tx.tuple.saddr, sizeof(bdmf_ipv4));
            memcpy(&result->mapt_cfg.dst_ip.addr.ipv4, &blog_p->tx.tuple.daddr, sizeof(bdmf_ipv4));
            result->mapt_cfg.src_port = blog_p->tx.tuple.port.source;
            result->mapt_cfg.dst_port = blog_p->tx.tuple.port.dest;
            result->mapt_cfg.l4csum = blog_p->tx.tuple.check;
            result->mapt_cfg.l3csum = blog_p->rx.tuple.check;
        }
        else
        {
            result->mapt_cfg.tos_tc = PKT_IPV6_GET_TOS_WORD(blog_p->tupleV6.word0);
            result->mapt_cfg.proto = blog_p->key.protocol;
            result->mapt_cfg.src_ip.family = bdmf_ip_family_ipv6;
            memcpy(&result->mapt_cfg.src_ip.addr.ipv6, &blog_p->tupleV6.saddr, sizeof(bdmf_ipv6_t));
            memcpy(&result->mapt_cfg.dst_ip.addr.ipv6, &blog_p->tupleV6.daddr, sizeof(bdmf_ipv6_t));
            result->mapt_cfg.src_port = blog_p->tupleV6.port.source;
            result->mapt_cfg.dst_port = blog_p->tupleV6.port.dest;
            result->mapt_cfg.l4csum = blog_p->tx.tuple.check;
            result->mapt_cfg.l3csum = 0; /* IPv6 header does not have CSUM */
        }
    }
    else if (TX_GRE(blog_p) ^ RX_GRE(blog_p)) /* The transmit should be different from recieve for perfrom termination */
    {
        if TX_GRE(blog_p)
        {
            rc = add_gre_tunnel_commands(blog_p, result);
            if (rc)
                return -1;
        }
    }
    else if (RX_VXLAN(blog_p) ^ TX_VXLAN(blog_p)) /* The transmit should be different from recieve for perfrom termination */
    {
        if (TX_VXLAN(blog_p))
        {
            tunnel_key flow_tunnel_key = {};
            tunnel_entry_t *entry, *tmp_entry, *new_entry;
            int rc;

            result->action_vec |= rdpa_fc_action_vxlan_tunnel;

            if (blog_p->vxlan.ipv4)
            {
                BlogIpv4Hdr_t *p_hdr = (BlogIpv4Hdr_t*)(blog_p->vxlan.tunnel_data + blog_p->vxlan.l2len);

                flow_tunnel_key.dst_ip.family = bdmf_ip_family_ipv4;
                memcpy(&(flow_tunnel_key.dst_ip.addr.ipv4), (uint8_t *)(&(p_hdr->dAddr)), 4);
                flow_tunnel_key.src_ip.family = bdmf_ip_family_ipv4;
                memcpy(&(flow_tunnel_key.src_ip.addr.ipv4), (uint8_t *)(&(p_hdr->sAddr)), 4);
                flow_tunnel_key.prot_key = blog_p->vxlan.vni;
            }
            else
            {
                BlogIpv6Hdr_t *p_hdr = (BlogIpv6Hdr_t*)(blog_p->vxlan.tunnel_data + blog_p->vxlan.l2len);
                flow_tunnel_key.dst_ip.family = bdmf_ip_family_ipv6;
                memcpy(&flow_tunnel_key.dst_ip.addr.ipv6, (uint8_t *)(&(p_hdr->dAddr)), 16);
                flow_tunnel_key.src_ip.family = bdmf_ip_family_ipv6;
                memcpy(&flow_tunnel_key.src_ip.addr.ipv6, (uint8_t *)(&(p_hdr->sAddr)), 16);
                flow_tunnel_key.prot_key = blog_p->vxlan.vni;               
            }
            DLIST_FOREACH_SAFE(entry, &tunnel_entry_list, list, tmp_entry)
            {
                if (memcmp(&(entry->key), &flow_tunnel_key, sizeof(tunnel_key)) == 0)
                {
                    result->tunnel_obj = entry->p_tunnel;
                    break;
                }
            }
            if (result->tunnel_obj == NULL)
            {
                BDMF_MATTR(tunnel_attrs, rdpa_tunnel_drv());
                rdpa_tunnel_cfg_t rdpa_tunnel;

                rc = bdmf_new_and_set(rdpa_tunnel_drv(), NULL, tunnel_attrs, &(result->tunnel_obj));
                if (rc)
                {
                    BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "failed creating tunnel object\n");
                    return -1;
                }

                rdpa_tunnel.tunnel_type = rdpa_tunnel_vxlan;
                rdpa_tunnel.tunnel_header_length = blog_p->vxlan.length;
                rdpa_tunnel.layer3_offset = blog_p->vxlan.l2len;
                rdpa_tunnel.tunnel_info = blog_p->vxlan.vni;

                if (rdpa_tunnel.tunnel_header_length > RDPA_MAX_TUNNEL_HEADER_LEN)
                    __logError("tunnel header length(%d) > RDPA_MAX_TUNNEL_HEADER_LEN(%d)", rdpa_tunnel.tunnel_header_length, RDPA_MAX_TUNNEL_HEADER_LEN);

                if (blog_p->vxlan.ipv4)
                {
                    rdpa_tunnel.local_ip.family = bdmf_ip_family_ipv4;
                    rdpa_tunnel.local_ip.addr.ipv4 = ntohl(flow_tunnel_key.src_ip.addr.ipv4);
                    rdpa_tunnel.layer4_offset = rdpa_tunnel.layer3_offset + BLOG_IPV4_HDR_LEN;
                }
                else
                {
                    rdpa_tunnel.local_ip.family = bdmf_ip_family_ipv6;
                    memcpy(&(rdpa_tunnel.local_ip.addr.ipv6), &(flow_tunnel_key.src_ip.addr.ipv6), 16);
                    rdpa_tunnel.layer4_offset = rdpa_tunnel.layer3_offset + BLOG_IPV6_HDR_LEN;
                }

                memcpy(rdpa_tunnel.tunnel_header, blog_p->vxlan.tunnel_data, blog_p->vxlan.length);

                if (blog_p->vxlan.ipv4)
                {
                    BlogIpv4Hdr_t *p_hdr = (BlogIpv4Hdr_t*)(rdpa_tunnel.tunnel_header + blog_p->vxlan.l2len);
                    uint16_t l3_len = htons(blog_p->vxlan.length - blog_p->vxlan.l2len);
 
                    p_hdr->len = l3_len;
                    p_hdr->chkSum = _apply_icsum(p_hdr->chkSum, (__force uint32_t)(_compute_icsum16( 0, 0, l3_len )));
                }
                else
                {
                    BlogIpv6Hdr_t *p_hdr = (BlogIpv6Hdr_t*)(rdpa_tunnel.tunnel_header + blog_p->vxlan.l2len);
                    uint16_t l3_len = htons(blog_p->vxlan.length - blog_p->vxlan.l2len - BLOG_IPV6_HDR_LEN);

                    p_hdr->len = l3_len;
                }

                rc = rdpa_tunnel_cfg_set(result->tunnel_obj, &rdpa_tunnel);
                if (rc)
                {
                    BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "failed updating tunnel object\n");
                    return -1;
                }

                new_entry = bdmf_alloc(sizeof(tunnel_entry_t));
                memcpy(&(new_entry->key), &flow_tunnel_key, sizeof(tunnel_key));
                new_entry->p_tunnel = result->tunnel_obj;
                rdpa_tunnel_index_get(result->tunnel_obj, &(new_entry->index)); 
                DLIST_INSERT_HEAD(&tunnel_entry_list, new_entry, list);
            }
        }
        else
        {
        }      
    }
    else
    {
        __logError("Unable to determine if the flow is routed/bridged (%d)", __LINE__);
        return -1;
    }

    return 0;
}

static int add_l2_commands(rdpa_ip_flow_result_t *ip_flow_result, Blog_t *blog_p)
{
    int is_ext_switch_port = 0;
    int bcmtaglen = 0;
    int bcmtaglocation = 2 * ETH_ALEN;
    int l2_header_offset = 0, num_of_vtags = 0;
    uint8_t rx_length = blog_p->rx.length;
    uint8_t tx_length = blog_p->tx.length;
#ifndef RDP_SIM
    uint16_t etype;
    uint32_t vtag0, vtag1;
#endif

#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
    fc_class_ctx_t fc_key;
#endif

    if (pktrunner_is_rx_enet_wan_port(blog_p) && pktrunner_is_tx_enet_ext_sw_port(blog_p))
    {
        is_ext_switch_port = 1;
        bcmtaglen = sizeof(S_INGRESS_BCM_SWITCH_TAG);
    }

    ip_flow_result->ovid_offset = is_ext_switch_port ? rdpa_vlan_offset_16 :
        rdpa_vlan_offset_12;

    BCM_ASSERT((blog_p->tx.length + bcmtaglen <= 32));
    ip_flow_result->l2_header_size = 0;

#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
    if (blog_p->fc_hybrid)
    {
        fc_key.word = blog_p->fc_context;
        fc_key.id.src_port = rdpa_mw_root_dev2rdpa_if(blog_p->rx_dev_p);
        *(uint32_t*)ip_flow_result->l2_header = fc_key.word;
        ip_flow_result->l2_header_size += L2_FLOW_P_LEN;
        l2_header_offset += L2_FLOW_P_LEN;
    }
#endif

#if !defined(CONFIG_ONU_TYPE_SFU)
    if (blog_p->tx.info.phyHdrType == BLOG_WLANPHY && !blog_p->wfd.nic_ucast.is_wfd
#if !defined(RDP_SIM)
        && l2_header_need_llc_snap(blog_p->rnr.radio_idx)
#endif
        )
    {
        /* Check if need to add LLC Snap */
        l2_header_offset += L2_DOT11_LLC_SNAP_HDR_LEN;
        ip_flow_result->action_vec |= rdpa_fc_action_llc_snap_set_len;
    }
#endif

    ip_flow_result->pathstat_idx = blog_p->hw_pathstat_idx;
    if (blog_p->hw_pathstat_idx >= 0)
    {
        if (blog_p->rx.multicast)
            pathstat_idx_map[blog_p->hw_pathstat_idx] = PKTRNR_FLOW_TYPE_MC;
        else if (blog_p->rx.info.bmap.PLD_L2)
            pathstat_idx_map[blog_p->hw_pathstat_idx] = PKTRNR_FLOW_TYPE_L2;
        else
            pathstat_idx_map[blog_p->hw_pathstat_idx] = PKTRNR_FLOW_TYPE_L3;
    }

    /* get_max_rx_pkt_len including crc according to mtu */
    ip_flow_result->max_pkt_len = blog_getTxMtu(blog_p) + blog_p->rx.length + 4;

    if (TX_GRE(blog_p) && !RX_GRE(blog_p)) /* GRE TX Termination, the L2 should be of the encapsulated packet since the L2 of the tunnel is stored in tunnel object */
    {
        BlogGre_t *gre_p = &blog_p->gretx;

        ip_flow_result->l2_header_offset = blog_p->rx.length - gre_p->l2_hlen - l2_header_offset;
        ip_flow_result->l2_header_size = gre_p->l2_hlen;

        if (ip_flow_result->l2_header_size + l2_header_offset > RDPA_L2_HEADER_SIZE)
        {
            __logError("L2 header size %u exceeded the maximum %d\n", ip_flow_result->l2_header_size, RDPA_L2_HEADER_SIZE);
            return -1;
        }

        memcpy(&ip_flow_result->l2_header[l2_header_offset], gre_p->l2hdr, gre_p->l2_hlen);
    }
    else
    {
        uint8_t *l2hdr = blog_p->tx.l2hdr;

        if (RX_GRE(blog_p) && !TX_GRE(blog_p))
        {
            rx_length = blog_p->grerx.l2_hlen;
        }

#ifndef RDP_SIM
        num_of_vtags = blog_hdr_get_vtags_from_encap(&blog_p->tx, &etype, &vtag0, &vtag1, 0);
#endif

        ip_flow_result->l2_header_number_of_tags = num_of_vtags;
        ip_flow_result->l2_header_offset = rx_length - tx_length - bcmtaglen - l2_header_offset;
        ip_flow_result->l2_header_size += tx_length + bcmtaglen;

        if (ip_flow_result->l2_header_size + l2_header_offset > RDPA_L2_HEADER_SIZE)
        {
            __logError("L2 header size %u exceeded the maximum %d\n", ip_flow_result->l2_header_size, RDPA_L2_HEADER_SIZE);
            return -1;
        }

        memcpy(&ip_flow_result->l2_header[l2_header_offset], l2hdr, bcmtaglocation);
        memcpy(&ip_flow_result->l2_header[l2_header_offset + bcmtaglocation + bcmtaglen], l2hdr + bcmtaglocation, tx_length - bcmtaglocation);
    }

    /* add bcmtag on DS flows on board with external switch */
    if (is_ext_switch_port)
    {
        S_INGRESS_BCM_SWITCH_TAG bcmtag = localBcmTag;
        unsigned int port_map = (1 << blog_p->tx.info.channel);

        bcmtag.dst_port = GET_PORTMAP_FROM_LOGICAL_PORTMAP(port_map, 1);
        memcpy(&ip_flow_result->l2_header[l2_header_offset + bcmtaglocation], &bcmtag, bcmtaglen);

        __logDebug("<bcm chnl %d, bcmtag.dst %x>\n", blog_p->tx.info.channel, bcmtag.dst_port);
    }

#if !defined(CONFIG_ONU_TYPE_SFU)
#if defined(XRDP)
    if ((blog_p->rx.info.bmap.LLC_SNAP == 1) && (blog_p->tx.info.bmap.LLC_SNAP == 1)) /* LLC/SNAP PASSTHROUGH case */
    {
        ip_flow_result->l2_header_size -= BLOG_LLC_SNAP_8023_LEN; /* no need copy LLC_SNAP in case of llc/snap passThrough */
    }
#endif

#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
    if (l2_header_offset - (blog_p->fc_hybrid ? L2_FLOW_P_LEN : 0)) /* Need to add LLC Snap */
#else
    if (l2_header_offset)
#endif
    {
        l2_header_insert_llc_snap(ip_flow_result->l2_header, blog_p);
        ip_flow_result->l2_header_size += L2_DOT11_LLC_SNAP_HDR_LEN;
        if (ip_flow_result->l2_header_size > RDPA_L2_HEADER_SIZE)
        {
            __logError("L2 header size %u exceeded the maximum %d\n", ip_flow_result->l2_header_size, RDPA_L2_HEADER_SIZE);
            return -1;
        }
    }
#endif

    if (TX_PPPOE(blog_p))
    {
        /* GRE or PPOE origination */
        if ((TX_GRE(blog_p)) || !(RX_PPPOE(blog_p)))
        {
            ip_flow_result->action_vec |= rdpa_fc_action_pppoe;
        }
        else if (!blog_p->is_routed && PT_PPPOE(blog_p))
        {
            ip_flow_result->action_vec |= rdpa_fc_action_pppoe_passthrough;
        }
    }

#if !defined(CONFIG_ONU_TYPE_SFU)
    /* set WLAN acceleration info */
    if (blog_p->tx.info.phyHdrType == BLOG_WLANPHY)
    {
        pktrunner_build_wlan_result(blog_p, ip_flow_result);
    }
    else if (blog_p->tx.info.phyHdrType == BLOG_NETXLPHY)
    {
        ip_flow_result->queue_id = 0;
        ip_flow_result->wl_metadata = 0;
        ip_flow_result->wfd.nic_ucast.is_wfd = 1;
    }
#endif

    if (((rx_length == tx_length) && (memcmp ((const void *)blog_p->tx.l2hdr, (const void *)blog_p->rx.l2hdr, rx_length) == 0)) && 
         (!(ip_flow_result->action_vec & (~FC_ACTIONS_NOT_AFFECTING_L2))) && !((RX_VXLAN(blog_p) ^ TX_VXLAN(blog_p))))
    {
        __logDebug("set flow with skip l2_hdr_copy\n");
        ip_flow_result->action_vec |= rdpa_fc_action_skip_l2_hdr_copy;
    }

    return 0;
}

static int add_l3_commands(rdpa_ip_flow_result_t *ip_flow_result, Blog_t *blog_p)
{
    BlogTuple_t *rxIp_p = &blog_p->rx.tuple;
    BlogTuple_t *txIp_p = &blog_p->tx.tuple;
    rdpa_traffic_dir dir = pktrunner_is_rx_enet_wan_port(blog_p) ? rdpa_dir_ds : rdpa_dir_us;

    if (T4in6DN(blog_p))
    {
        /* XXX: tunneling */
    }
    else if (T4in6UP(blog_p))
    { }
    else if ((blog_p->rx.info.bmap.PLD_IPv6) || (blog_p->l2_ipv6))
    {   /* rx IPv6; tx IPv6 */
        if (is_ecn_remark_set())
        {
            if (blog_p->rx.tuple.tos !=
                PKT_IPV6_GET_TOS_WORD(blog_p->tupleV6.word0))
            {
                ip_flow_result->action_vec |= rdpa_fc_action_dscp_remark;
                ip_flow_result->dscp_value = PKT_IPV6_GET_TOS_WORD(blog_p->tupleV6.word0);
                __logDebug("Replace DSCPv6<%d>", ip_flow_result->dscp_value);
            }
        }
        else
        {  /* Note: we are not remarking ECN */
            if (blog_p->rx.tuple.tos >> BLOG_RULE_DSCP_IN_TOS_SHIFT !=
                PKT_IPV6_GET_TOS_WORD(blog_p->tupleV6.word0) >> BLOG_RULE_DSCP_IN_TOS_SHIFT)
            {
                ip_flow_result->action_vec |= rdpa_fc_action_dscp_remark;
                ip_flow_result->dscp_value = PKT_IPV6_GET_TOS_WORD(blog_p->tupleV6.word0) >> BLOG_RULE_DSCP_IN_TOS_SHIFT;
                __logDebug("Replace DSCPv6<%d>", ip_flow_result->dscp_value);
            }
        }
    }

    if ((rxIp_p->tos >> BLOG_RULE_DSCP_IN_TOS_SHIFT != txIp_p->tos >> BLOG_RULE_DSCP_IN_TOS_SHIFT) &&
        (!T6in4UP(blog_p)) && (!T6in4DN(blog_p)) && !(blog_p->l2_ipv6) && !(blog_p->rx.info.bmap.PLD_IPv6))
    {
        ip_flow_result->action_vec |= rdpa_fc_action_dscp_remark;
        /* Note: we are not remarking ECN */
        ip_flow_result->dscp_value = *(uint8_t *)(&txIp_p->tos) >> BLOG_RULE_DSCP_IN_TOS_SHIFT;
        __logDebug("Replace DSCP<%d>", ip_flow_result->dscp_value);
    }

    if ((blog_p->rx.info.bmap.NPT6) || (blog_p->tx.info.bmap.NPT6))
    {
        __logDebug("%sNPT6", blog_p->rx.info.bmap.NPT6 ? "S" : "D");
        memcpy(&ip_flow_result->nat_ip.addr.ipv6, &blog_p->tupleV6.addr_npt6, sizeof(bdmf_ipv6_t));
        ip_flow_result->nat_ip.family = bdmf_ip_family_ipv6;
        ip_flow_result->action_vec |= rdpa_fc_action_nat;
    }

    if (blog_p->tx.info.bmap.PLD_IPv4 == 0 && !blog_p->l2_ipv4) /* tx is IPv6  */
        return 0; /* L3 IPv6 fields taken care of. OK to return */

    if (dir == rdpa_dir_us && rxIp_p->saddr != txIp_p->saddr && !(MAPT(blog_p)))
    {
        __logDebug("SNAT");
        ip_flow_result->nat_ip.addr.ipv4 = htonl(txIp_p->saddr);
        ip_flow_result->nat_ip.family = bdmf_ip_family_ipv4;
        if (blog_p->key.protocol == IPPROTO_TCP || blog_p->key.protocol == IPPROTO_UDP)
        {
            ip_flow_result->nat_port = htons(txIp_p->port.source);
            /* XXX: In current FW version NAT for non TCP/UDP traffic is not supported */
            ip_flow_result->action_vec |= rdpa_fc_action_nat;
        }
        else if (blog_p->key.protocol == IPPROTO_GRE || blog_p->key.protocol == IPPROTO_ESP)
            ip_flow_result->action_vec |= rdpa_fc_action_nat;

    }
    else if (dir == rdpa_dir_ds && rxIp_p->daddr != txIp_p->daddr && !(MAPT(blog_p)))
    {
        __logDebug("DNAT");
        ip_flow_result->nat_ip.addr.ipv4 = htonl(txIp_p->daddr);
        ip_flow_result->nat_ip.family = bdmf_ip_family_ipv4;
        if (blog_p->key.protocol == IPPROTO_TCP || blog_p->key.protocol == IPPROTO_UDP)
        {
            ip_flow_result->nat_port = htons(txIp_p->port.dest);
            /* XXX: In current FW version NAT for non TCP/UDP traffic is not supported */
            ip_flow_result->action_vec |= rdpa_fc_action_nat;
        }
        else if (blog_p->key.protocol == IPPROTO_GRE || blog_p->key.protocol == IPPROTO_ESP)
            ip_flow_result->action_vec |= rdpa_fc_action_nat;
    }

    if (blog_p->key.protocol == IPPROTO_GRE && rxIp_p->gre_callid != txIp_p->gre_callid)
    {
        ip_flow_result->nat_port = htons(txIp_p->gre_callid);
        ip_flow_result->action_vec |= rdpa_fc_action_gre_remark;
    }

    if ((ip_flow_result->action_vec & rdpa_fc_action_skip_l2_hdr_copy) && (!(ip_flow_result->action_vec & (~FC_ACTIONS_NOT_AFFECTING_PAYLOAD))))
    {
        __logDebug("set flow with skip hdr_copy\n");
        ip_flow_result->action_vec |= rdpa_fc_action_skip_hdr_copy;
    }

    return 0;
}

static int add_sq_commands(rdpa_ip_flow_result_t *ip_flow_result, Blog_t *blog_p)
{
    rdpa_traffic_dir dir = pktrunner_is_rx_enet_wan_port(blog_p) ? rdpa_dir_ds : rdpa_dir_us;

    if(SKBMARK_GET_SQ_MARK(blog_p->mark) && (dir == rdpa_dir_ds))
        blog_p->dpi_queue = SKBMARK_GET_DPIQ_MARK(blog_p->mark);

    if (blog_p->dpi_queue >= RDPA_MAX_SERVICE_QUEUES)
    {
        ip_flow_result->action_vec &= ~rdpa_fc_action_service_q;
        return 0;
    }

    if (dir == rdpa_dir_us)
    {
        ip_flow_result->queue_id = blog_p->dpi_queue + DPI_XRDP_US_SQ_OFFSET;
    }
    else
    {
        ip_flow_result->service_q_id = blog_p->dpi_queue;
        ip_flow_result->action_vec |= rdpa_fc_action_service_q;
    }

    return 0;
}

static int is_ip_proto_acceleration_supported(Blog_t *blog_p)
{
    switch (blog_p->key.protocol)
    {
    case IPPROTO_ICMP:
    case IPPROTO_IGMP:
        return 0;
    case IPPROTO_GRE:
        /* Only GRE v1 to GRE v1 flow is supported */
        return (blog_p->grerx.gre_flags.ver == 1 && blog_p->gretx.gre_flags.ver == 1);
    default:
        break;
    }
    return 1;
}

static int runner_ucast_flow_common_result_prepare(Blog_t *blog_p, rdpa_traffic_dir dir, rdpa_ip_flow_result_t *result)
{
    int rc;

    add_qos_commands(dir, result, blog_p);

    rc = add_l2_commands(result, blog_p);
    if (rc)
    {
        __logInfo("Failed to add L2 commands");
        goto exit;
    }

    rc = add_l3_commands(result, blog_p);
    if (rc)
    {
        __logInfo("Failed to add L3 commands");
        goto exit;
    }

    rc = add_sq_commands(result, blog_p);
    if (rc)
    {
        __logInfo("Failed to add SQ commands");
        goto exit;
    }

exit:
    if (rc)
        return -1;
    return 0;
}


static bdmf_index runner_activate_l3_flow(Blog_t *blog_p)
{
    rdpa_ip_flow_info_t ip_flow = {};
    bdmf_index index;
    int rc;

    rc = pktrunner_l3_flow_key_construct(blog_p, &ip_flow.key);
    if (rc)
        return -1;

    rc = add_fwd_commands(&ip_flow.result, blog_p, ip_flow.key.dir);
    if (rc)
    {
        __logInfo("Failed to add FWD commands");
        return -1;
    }

    rc = runner_ucast_flow_common_result_prepare(blog_p, ip_flow.key.dir, &ip_flow.result);
    if (rc)
        return -1;

    rc = rdpa_ip_class_flow_add(ip_class, &index, &ip_flow);
    if (rc < 0)
    {
        __logInfo("Failed to activate flow");
        return -1;
    }

    if (blog_p->rx.info.phyHdrType == BLOG_SPDTST && blog_p->tx.info.phyHdrType == BLOG_WLANPHY
        && blog_p->wfd.nic_ucast.is_wfd)
    {
        rc = pktrunner_udpspdt_tx_start(blog_p);
        if (rc < 0)
        {
            __logInfo("Failed to start UDP Speed test to WLAN interface");
            rdpa_ip_class_flow_delete(ip_class, index);
            return -1;
        }
        __logDebug("UDP Speed test (TX) started");
    }
 
    __logDebug("L3 flow activated, index = %ld", index);

    return index;
}

static bdmf_index runner_activate_l2_flow(Blog_t *blog_p)
{
    rdpa_l2_flow_info_t l2_flow = {};
    bdmf_index index;
    int rc;

    /*Build Key*/
    pktrunner_l2flow_key_construct(blog_p, &l2_flow.key);

    /* Start of result build */
    l2_flow.result.port = rdpa_mw_root_dev2rdpa_if((struct net_device *)blog_p->tx_dev_p);

#if !defined(CONFIG_ONU_TYPE_SFU)
    if (blog_p->tx.info.phyHdrType == BLOG_WLANPHY)
        l2_flow.result.ssid = rdpa_mw_root_dev2rdpa_ssid((struct net_device *)blog_p->tx_dev_p);
#endif

    if (blog_p->tx.info.phyHdrType == BLOG_GPONPHY || blog_p->tx.info.phyHdrType == BLOG_EPONPHY)
    {
        l2_flow.result.wan_flow = blog_p->tx.info.channel;
    }

#if !defined(CONFIG_ONU_TYPE_SFU)
    if(blog_p->tx.info.phyHdrType != BLOG_WLANPHY && blog_p->tx.info.phyHdrType != BLOG_NETXLPHY)
#endif
        l2_flow.result.queue_id = SKBMARK_GET_Q_PRIO(blog_p->mark);

    if (TX_GRE(blog_p) ^ RX_GRE(blog_p)) /* The transmit should be different from recieve for perfrom termination */
    {
        if TX_GRE(blog_p)
        {
            rc = add_gre_tunnel_commands(blog_p, &l2_flow.result);
            if (rc)
                return -1;
	   }
    }
    if (TX_VXLAN(blog_p))
    {
        tunnel_key flow_tunnel_key = {};
        tunnel_entry_t *entry, *tmp_entry, *new_entry;
        int rc;

        l2_flow.result.action_vec |= rdpa_fc_action_vxlan_tunnel;

        if (blog_p->vxlan.ipv4)
        {
            BlogIpv4Hdr_t *p_hdr = (BlogIpv4Hdr_t*)(blog_p->vxlan.tunnel_data + blog_p->vxlan.l2len);

            flow_tunnel_key.dst_ip.family = bdmf_ip_family_ipv4;
            memcpy(&(flow_tunnel_key.dst_ip.addr.ipv4), (uint8_t *)(&(p_hdr->dAddr)), 4);
            flow_tunnel_key.src_ip.family = bdmf_ip_family_ipv4;
            memcpy(&(flow_tunnel_key.src_ip.addr.ipv4), (uint8_t *)(&(p_hdr->sAddr)), 4);
            flow_tunnel_key.prot_key = blog_p->vxlan.vni;
        }
        else
        {
            flow_tunnel_key.dst_ip.family = bdmf_ip_family_ipv6;
            memcpy(&flow_tunnel_key.dst_ip.addr.ipv6, &blog_p->tupleV6.daddr, 16);
            flow_tunnel_key.src_ip.family = bdmf_ip_family_ipv6;
            memcpy(&flow_tunnel_key.src_ip.addr.ipv6, &blog_p->tupleV6.saddr, 16);
            flow_tunnel_key.prot_key = blog_p->vxlan.vni;               
        }
        DLIST_FOREACH_SAFE(entry, &tunnel_entry_list, list, tmp_entry)
        {
            if (memcmp(&(entry->key), &flow_tunnel_key, sizeof(tunnel_key)) == 0)
            {
                l2_flow.result.tunnel_obj = entry->p_tunnel;
                break;
            }
        }
        if (l2_flow.result.tunnel_obj == NULL)
        {
            BDMF_MATTR(tunnel_attrs, rdpa_tunnel_drv());
            rdpa_tunnel_cfg_t rdpa_tunnel;

            rc = bdmf_new_and_set(rdpa_tunnel_drv(), NULL, tunnel_attrs, &(l2_flow.result.tunnel_obj));
            if (rc)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "failed creating tunnel object\n");
                return -1;
            }

            rdpa_tunnel.tunnel_type = rdpa_tunnel_vxlan;
            rdpa_tunnel.tunnel_header_length = blog_p->vxlan.length;
            rdpa_tunnel.layer3_offset = blog_p->vxlan.l2len;
            rdpa_tunnel.tunnel_info = blog_p->vxlan.vni;

            if (rdpa_tunnel.tunnel_header_length > RDPA_MAX_TUNNEL_HEADER_LEN)
                __logError("tunnel header length(%d) > RDPA_MAX_TUNNEL_HEADER_LEN(%d)", rdpa_tunnel.tunnel_header_length, RDPA_MAX_TUNNEL_HEADER_LEN);

            if (blog_p->vxlan.ipv4)
            {
                rdpa_tunnel.local_ip.family = bdmf_ip_family_ipv4;
                rdpa_tunnel.local_ip.addr.ipv4 = ntohl(flow_tunnel_key.src_ip.addr.ipv4);
                rdpa_tunnel.layer4_offset = rdpa_tunnel.layer3_offset + BLOG_IPV4_HDR_LEN;
            }
            else
            {
                rdpa_tunnel.local_ip.family = bdmf_ip_family_ipv6;
                memcpy(&rdpa_tunnel.local_ip.addr.ipv6, &blog_p->tupleV6.daddr, 16);
                rdpa_tunnel.layer4_offset = rdpa_tunnel.layer3_offset + BLOG_IPV6_HDR_LEN;
            }

            memcpy(rdpa_tunnel.tunnel_header, blog_p->vxlan.tunnel_data, blog_p->vxlan.length);

            if (blog_p->vxlan.ipv4)
            {
                BlogIpv4Hdr_t *p_hdr = (BlogIpv4Hdr_t*)(rdpa_tunnel.tunnel_header + blog_p->vxlan.l2len);
                uint16_t l3_len = htons(blog_p->vxlan.length - blog_p->vxlan.l2len);

                p_hdr->len = l3_len;
                p_hdr->chkSum = _apply_icsum(p_hdr->chkSum, (__force uint32_t)(_compute_icsum16( 0, 0, l3_len )));
            }
            else
            {
                BlogIpv6Hdr_t *p_hdr = (BlogIpv6Hdr_t*)(rdpa_tunnel.tunnel_header + blog_p->vxlan.l2len);
                uint16_t l3_len = htons(blog_p->vxlan.length - blog_p->vxlan.l2len - BLOG_IPV6_HDR_LEN);

                p_hdr->len = l3_len;
            }

            rc = rdpa_tunnel_cfg_set(l2_flow.result.tunnel_obj, &rdpa_tunnel);
            if (rc)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "failed updating tunnel object\n");
                return -1;
            }

            new_entry = bdmf_alloc(sizeof(tunnel_entry_t));
            memcpy(&(new_entry->key), &flow_tunnel_key, sizeof(tunnel_key));
            new_entry->p_tunnel = l2_flow.result.tunnel_obj;
            rdpa_tunnel_index_get(l2_flow.result.tunnel_obj, &(new_entry->index)); 
            DLIST_INSERT_HEAD(&tunnel_entry_list, new_entry, list);
        }
    }


    rc = runner_ucast_flow_common_result_prepare(blog_p, l2_flow.key.dir, &l2_flow.result);
    if (rc)
        return -1;

    rc = rdpa_l2_class_flow_add(l2_class, &index, &l2_flow);
    if (rc < 0)
    {
        __logInfo("Failed to activate flow");
        return -1;
    }

    __logDebug("L2 flow activated, index = %ld", index);
    return index;
}

static uint32_t runner_activate_ucast(Blog_t *blog_p, uint32_t key_in)
{
    bdmf_index index;
    PktRunnerFlowKey_t pktRunner_key = {};
    int pktRunner_flow_idx;
    uint32_t accel = PKTRUNNER_ACCEL_FLOW;

    BCM_ASSERT((blog_p!=BLOG_NULL));

    BCM_LOGCODE(if(isLogDebug)
        { bcm_printk("\n::: runner_activate_ucast :::\n"); blog_dump(blog_p); });

    if (PKTRUNNER_STATE(accel).ucast_active >= PKTRUNNER_MAX_FLOWS)
    {
        __logInfo("Exceeding max allowed flows - abort");
        goto abort_activate;
    }

    if (!ip_class)
    {
        __logInfo("No ip_class obj");
        goto abort_activate;
    }

    if (!((blog_p->rx.info.bmap.PLD_IPv6 && !T4in6DN(blog_p)) ||
		(blog_p->rx.info.bmap.PLD_IPv4) || (blog_p->rx.info.bmap.PLD_L2)))
    {
        __logInfo("Flow Type is not supported");
        goto abort_activate;
    }

    if (CHK6in4(blog_p))
    {
        __logInfo("6in4 Tunnel not supported");
        goto abort_activate;
    }

    if (!is_ip_proto_acceleration_supported(blog_p))
    {
        __logInfo("Flow Type proto<%d> is not supported", blog_p->key.protocol);
        goto abort_activate;
    }

    pktRunner_flow_idx = idx_pool_get_index(&PKTRUNNER_DATA(accel).idx_pool);
    if (pktRunner_flow_idx < 0)
    {
        PKTRUNNER_STATE(accel).no_free_idx++;
        __logInfo("No free pkt runner indexes for accel %u", accel);
        goto abort_activate;
    }
    /* call provision function per flow type */
    if (blog_p->rx.info.bmap.PLD_L2)
    {
        __logInfo("Accelerating L2 Flow");
        index = runner_activate_l2_flow(blog_p);
        pktRunner_key.word = __pktRunnerBuildKey(accel, PKTRNR_FLOW_TYPE_L2, pktRunner_flow_idx);
        PKTRUNNER_RDPA_KEY(accel, pktRunner_flow_idx) = index;
    }
    else
    {
        __logInfo("Accelerating L3 Flow");
        index = runner_activate_l3_flow(blog_p);
        pktRunner_key.word = __pktRunnerBuildKey(accel, PKTRNR_FLOW_TYPE_L3, pktRunner_flow_idx);
        PKTRUNNER_RDPA_KEY(accel, pktRunner_flow_idx) = index;
    }

    /* check provision status */
    if (index < 0)
    {
        idx_pool_return_index(&PKTRUNNER_DATA(accel).idx_pool, pktRunner_flow_idx);
        PKTRUNNER_RDPA_KEY(accel, pktRunner_flow_idx) = FHW_TUPLE_INVALID;
        PKTRUNNER_STATE(accel).hw_add_fail++;
        goto abort_activate;
    }

    PKTRUNNER_STATS(accel, pktRunner_flow_idx) = 0;

    PKTRUNNER_STATE(accel).activates++;
    PKTRUNNER_STATE(accel).active++;
    PKTRUNNER_STATE(accel).ucast_active++;
    if (blog_p->rx.info.bmap.PLD_L2)
        __pktRunnerFlowTypeSet(&PKTRUNNER_DATA(accel), pktRunner_flow_idx, PKTRNR_FLOW_TYPE_L2);
    else
        __pktRunnerFlowTypeSet(&PKTRUNNER_DATA(accel), pktRunner_flow_idx, PKTRNR_FLOW_TYPE_L3);

    __logDebug("::: runner_activate_ucast flowId<%03u> cumm_activates<%u>, pktRunner_key.word<%x> pktRunner_flow_idx<%x> :::\n\n",
        (int)index, PKTRUNNER_STATE(accel).activates, pktRunner_key.word, pktRunner_flow_idx);

    return pktRunner_key.word;

abort_activate:
    PKTRUNNER_STATE(accel).failures++;
    __logInfo("cumm_failures<%u>", PKTRUNNER_STATE(accel).failures);
    return FHW_TUPLE_INVALID;
}

static int runner_update_flow_ucast_dpi(BlogUpdate_t update, Blog_t *blog_p, rdpa_ip_flow_result_t *result)
{
    int rc = 0;

    /* update flow settings */
    if (update == BLOG_UPDATE_DPI_QUEUE)
    {
        rc = add_sq_commands(result, blog_p);
    }
#if defined(CONFIG_BCM_DPI_WLAN_QOS)
    else
    {
        if (blog_p->wfd.nic_ucast.is_chain)
            result->wfd.nic_ucast.priority = blog_p->wfd.nic_ucast.priority;
        else
        {
            if (blog_p->rnr.is_wfd)
            {
                result->wfd.dhd_ucast.priority = blog_p->wfd.dhd_ucast.priority;
                result->wfd.dhd_ucast.flowring_idx = blog_p->wfd.dhd_ucast.flowring_idx;
            }
            else
            {
                result->rnr.priority = blog_p->rnr.priority;
                result->rnr.flowring_idx = blog_p->rnr.flowring_idx;
            }
        }
    }
#endif
    return rc;
}

static int runner_update_flow_l3_ucast(BlogUpdate_t update, uint32_t rdpa_flow_key, Blog_t *blog_p)
{
    rdpa_ip_flow_info_t ip_flow = {};
    int rc;

    rc = rdpa_ip_class_flow_get(ip_class, rdpa_flow_key, &ip_flow);
    if (rc)
    {
        __logInfo("Failed to get L3 flow");
        return -1;
    }

    rc = runner_update_flow_ucast_dpi(update, blog_p, &ip_flow.result);

    rc = rc ? rc : rdpa_ip_class_flow_set(ip_class, rdpa_flow_key, &ip_flow);
    if (rc)
    {
        __logInfo("Failed to update DPI settings for L3 flow, rc = %d", rc);
        return -1;
    }
    return 0;
}

static int runner_update_flow_l2_ucast(BlogUpdate_t update, uint32_t rdpa_flow_key, Blog_t *blog_p)
{
    rdpa_l2_flow_info_t l2_flow = {};
    int rc;

    rc = rdpa_l2_class_flow_get(l2_class, rdpa_flow_key, &l2_flow);
    if (rc)
    {
        __logInfo("Failed to get L2 flow");
        return -1;
    }
    rc = runner_update_flow_ucast_dpi(update, blog_p, &l2_flow.result);
    rc = rc ? rc : rdpa_l2_class_flow_set(l2_class, rdpa_flow_key, &l2_flow);
    if (rc)
    {
        __logInfo("Failed to update DPI settings for L2 flow, rc = %d", rc);
        return -1;
    }
    return 0;
}

static void mcast_flow_master_ctx_set_fwd_and_trap(Blog_t *blog_p, rdpa_ip_flow_result_t *result)
{
    if (blog_p->fwd_and_trap)
    {
        result->action_vec |= rdpa_fc_action_no_forward;
        result->action = rdpa_forward_action_host;
    }
    else
    {
        result->action_vec &= ~rdpa_fc_action_no_forward;
        result->action = rdpa_forward_action_forward;
    }
}

static int runner_update_flow_mcast(BlogUpdate_t update, uint32_t rdpa_flow_key, Blog_t *blog_p)
{
    int rc = -1;
    rdpa_fc_mcast_flow_t flow = {};

    __logDebug("rdpa_flow_key<%03u> update<%d>  blog_p<%px> fwd_and_trap<%d>\n",
        rdpa_flow_key, update, blog_p, blog_p->fwd_and_trap);

    rc = rdpa_iptv_flow_get(iptv, rdpa_flow_key, &flow);
    if (rc || !flow.result.is_mcast_master_flow) /* Paranoya check */
    {
        __logError("Failed to find mcast master flow, rdpa_flow_key<%d>, rc<%d>, is_master<%d>\n",
            rdpa_flow_key, rc, flow.result.is_mcast_master_flow);
        return -1;
    }

    switch(update)
    {
        case BLOG_UPDATE_BITMAP:
            __logDebug("BLOG_UPDATE_BITMAP\n");
            mcast_flow_master_ctx_set_clients_vector(blog_p, &flow.result);
            break;
        case BLOG_UPDATE_FWD_AND_TRAP:
            __logDebug("BLOG_UPDATE_FWD_AND_TRAP<%d>\n", blog_p->fwd_and_trap);
            mcast_flow_master_ctx_set_fwd_and_trap(blog_p, &flow.result);
            break;
        case BLOG_UPDATE_BITMAP_FWD_AND_TRAP:
            __logDebug("BLOG_UPDATE_BITMAP and BLOG_UPDATE_FWD_AND_TRAP<%d>\n", blog_p->fwd_and_trap);
            mcast_flow_master_ctx_set_fwd_and_trap(blog_p, &flow.result);
            mcast_flow_master_ctx_set_clients_vector(blog_p, &flow.result);
            break;
        default:
            __logError("Invalid BLOG Update: <%d>", update);
            return -1;
    }

    rc = rdpa_iptv_flow_set(iptv, rdpa_flow_key, &flow);
    if (rc)
    {
        __logError("Failed to update mcast flow, rdpa_flow_key<%03u>, "
            "update<%d> blog_p<%px> fwd_and_trap<%d>, rc<%d>\n",
            rdpa_flow_key, update, blog_p, blog_p->fwd_and_trap, rc);
    }

    return rc;
}

uint32_t runner_update_flow(BlogUpdate_t update, uint32_t pktRunner_key, Blog_t *blog_p)
{
    int rc = 0;
    bdmf_index index = __pktRunnerFlowIdx(pktRunner_key);
    uint32_t accel = __pktRunnerAccelIdx(pktRunner_key);
    uint32_t flow_type = __pktRunnerFlowType(pktRunner_key);
    uint32_t rdpa_flow_key = PKTRUNNER_RDPA_KEY(accel, index);

    switch (flow_type)
    {
        case PKTRNR_FLOW_TYPE_L3:
        case PKTRNR_FLOW_TYPE_L2:
            if (update != BLOG_UPDATE_DPI_QUEUE && update != BLOG_UPDATE_DPI_PRIORITY)
            {
                rc = -1;
                break;
            }
            if (flow_type == PKTRNR_FLOW_TYPE_L3)
                rc = runner_update_flow_l3_ucast(update, rdpa_flow_key, blog_p);
            else
                rc = runner_update_flow_l2_ucast(update, rdpa_flow_key, blog_p);
            break;
        case PKTRNR_FLOW_TYPE_MC:
            rc = runner_update_flow_mcast(update, rdpa_flow_key, blog_p);
            break;
        default:
            rc =-1;
            break;
    }

    return rc;
}

static int mcast_flow_master_ctx_prepare(Blog_t *blog_p, rdpa_ip_flow_result_t *result)
{
    int rc;
    bdmf_object_handle policer_obj = NULL;

    /* Update fwd_and_trap */
    mcast_flow_master_ctx_set_fwd_and_trap(blog_p, result);

    /* Update service queue setting */
    rc = add_sq_commands(result, blog_p);
    if (rc)
    {
        __logInfo("Failed to add SQ commands");
        goto exit;
    }

    /* Assign policer */
    blog_parse_policer_get(blog_p, &policer_obj);
    result->policer_obj = policer_obj;
    if (policer_obj)
        bdmf_put(policer_obj);

    result->max_pkt_len = blog_getTxMtu(blog_p) + blog_p->rx.length + 4;

    /* Update clients vector */
    mcast_flow_master_ctx_set_clients_vector(blog_p, result);

exit:
    if (rc)
        return -1;
    return 0;
}

static int mcast_flow_update_master_ctx_from_client_ctx(Blog_t *blog_p, rdpa_fc_mcast_flow_key_t *key,
    rdpa_ip_flow_result_t *result)
{
    rdpa_fc_mcast_flow_t master_flow;
    bdmf_index flow_idx;
    int rc;

    memcpy(&master_flow.key, key, sizeof(rdpa_fc_mcast_flow_key_t));
    master_flow.key.entry_idx = 0;

    rc = rdpa_iptv_flow_find(iptv, &flow_idx, &master_flow);
    if (rc) /* Paranoya check - master must exist */
    {
        __logError("Cound not find a master flow for client, rc<%d>", rc);
        return rc;
    }
    /* Re-read to update the context */
    rc = rdpa_iptv_flow_get(iptv, flow_idx, &master_flow);
    if (rc) /* Paranoya check - read should not fail */
    {
        __logError("Cound not read a master flow, idx<%ld>, rc<%d>", flow_idx, rc);
        return rc;
    }

    if (result->policer_obj)
    {
        __logInfo("New policer configuration received, overwriting for master\n");
        master_flow.result.policer_obj = result->policer_obj;
    }
    if (result->action_vec & rdpa_fc_action_service_q)
    {
        __logInfo("New Service queue configuration received, overwriting for master\n");
        master_flow.result.action_vec |= rdpa_fc_action_service_q;
        master_flow.result.service_q_id = result->service_q_id;
    }
    rc = rdpa_iptv_flow_set(iptv, flow_idx, &master_flow);
    return 0;
}

static int mcast_flow_client_ctx_prepare(Blog_t *blog_p, rdpa_fc_mcast_flow_key_t *key, rdpa_ip_flow_result_t *result)
{
    int rc;

    memset(result, 0, sizeof(rdpa_ip_flow_result_t));

    rc = add_fwd_commands(result, blog_p, rdpa_dir_ds);
    if (rc)
        return rc;

    rc = runner_ucast_flow_common_result_prepare(blog_p, rdpa_dir_ds, result);
    if (rc)
        return -1;

    if (result->policer_obj || (result->action_vec & rdpa_fc_action_service_q))
    {
        /* Once the new configuration for a policer or a service queue is set for the client flow, it's taken
         * globally for all flows under the same master, meaning overwrite the master flow configuration. The
         * cleanup of the service queue and the policer from the master is the responsibility of the upper layer. */
        rc = mcast_flow_update_master_ctx_from_client_ctx(blog_p, key, result);
        if (rc)
            return -1;
    }

    return 0;
}

static uint32_t runner_activate_mcast_flow_multi(Blog_t *blog_p)
{
    int rc;
    rdpa_fc_mcast_flow_t flow = {};
    bdmf_index flow_idx;

    rc = pktrunner_build_mcast_lkp_key(blog_p, &flow.key);
    if (rc)
    {
        __logError("Failed to build mcast lookup key, rc<%d>", rc);
        return FHW_TUPLE_INVALID;
    }

    /* Prepare context */
    if (blog_p->mcast_client_id == BLOG_MCAST_MASTER_CLIENT_ID)
    {
        /* Master context */
        rc = mcast_flow_master_ctx_prepare(blog_p, &flow.result);
        __logDebug("mcast_flow_master_ctx_prepare, rc<%d>\n", rc);
    }
    else
    {
        /* Client context */
        rc = mcast_flow_client_ctx_prepare(blog_p, &flow.key, &flow.result);
        __logDebug("mcast_flow_client_ctx_prepare, rc<%d>\n", rc);
    }
    if (rc < 0)
        return FHW_TUPLE_INVALID;

    rc = rdpa_iptv_flow_add(iptv, &flow_idx, &flow);
    if (rc)
    {
        __logInfo("Failed to add multicast %s flow (id<%d>), rc<%d>\n",
            blog_p->mcast_client_id == BLOG_MCAST_MASTER_CLIENT_ID ? "master" : "client", blog_p->mcast_client_id, rc);
        return FHW_TUPLE_INVALID;
    }
    return (uint32_t)flow_idx;
}

static uint32_t runner_activate_mcast(Blog_t *blog_p, uint32_t key_in)
{
    uint32_t pktRunner_flow_idx, rdpa_flow_idx;
    uint32_t accel = PKTRUNNER_ACCEL_FLOW;
    uint32_t pktrunner_key;

    BCM_ASSERT((blog_p!=BLOG_NULL));

    BCM_LOGCODE(if(isLogDebug)
        { bcm_printk("\n::: runner_activate_mcast :::\n"); blog_dump(blog_p); });

    pktRunner_flow_idx = idx_pool_get_index(&PKTRUNNER_DATA(accel).idx_pool);
    if (pktRunner_flow_idx < 0 )
    {
        PKTRUNNER_STATE(accel).no_free_idx++;
        __logError("Mcast activate; No free indexes\n");
        return FHW_TUPLE_INVALID;
    }

    if (PKTRUNNER_STATE(accel).mcast_active >= max_num_of_mcast_flows)
    {
        __logInfo("Exceeding max allowed flows - abort");
        return FHW_TUPLE_INVALID;
    }

    rdpa_flow_idx = runner_activate_mcast_flow_multi(blog_p);
    if (rdpa_flow_idx == FHW_TUPLE_INVALID)
    {
        PKTRUNNER_STATE(accel).hw_add_fail++;
        PKTRUNNER_STATE(accel).failures++;
        __logInfo("cumm_failures<%u>", PKTRUNNER_STATE(accel).failures);
        goto error;
    }

    pktrunner_key = __pktRunnerBuildKey(accel, PKTRNR_FLOW_TYPE_MC, pktRunner_flow_idx);
    PKTRUNNER_RDPA_KEY(accel, pktRunner_flow_idx) = rdpa_flow_idx;
    PKTRUNNER_STATS(accel, pktRunner_flow_idx) = 0;
    PKTRUNNER_STATE(accel).activates++;
    PKTRUNNER_STATE(accel).active++;
    PKTRUNNER_STATE(accel).mcast_active++;
    __pktRunnerFlowTypeSet(&PKTRUNNER_DATA(accel), pktRunner_flow_idx, PKTRNR_FLOW_TYPE_MC);
    __logDebug("::: runner_activate_mcast pktrunner_flow_idx<%u> pktrunner_key<%u> flowId<%03u> "
        "cumm_activates<%u> :::\n\n",
        pktRunner_flow_idx, pktrunner_key, PKTRUNNER_RDPA_KEY(accel, pktRunner_flow_idx),
        PKTRUNNER_STATE(accel).activates);
    return pktrunner_key;

error:
    idx_pool_return_index(&PKTRUNNER_DATA(accel).idx_pool, pktRunner_flow_idx);
    return FHW_TUPLE_INVALID;

}

static uint32_t runner_activate_flow(Blog_t *blog_p, uint32_t key_in)
{
    return blog_p->rx.multicast ? runner_activate_mcast(blog_p, key_in) : runner_activate_ucast(blog_p, key_in);
}

#if defined(CONFIG_BCM_FHW)
/* Clears FlowCache association(s) to pktrunner entries. This local function MUST be called
   with the Protocol Layer Lock taken. */
static void __clear_fcache(const FlowScope_t scope)
{
    uint32_t accel = PKTRUNNER_ACCEL_FLOW;

    /* Upcall into FlowCache */
    if(fc_clear_hook_runner != (FC_CLEAR_HOOK)NULL)
    {
        PKTRUNNER_STATE(accel).flushes += fc_clear_hook_runner(0, scope);
    }

    pktrunner_flow_remove_all();

    __logDebug("scope<%s> cumm_flushes<%u>",
               (scope == System_e) ? "System" : "Match",
               PKTRUNNER_STATE(accel).flushes);
}
#endif

void build_gre_tunnel_header(Blog_t *blog_p, rdpa_tunnel_cfg_t *rdpa_tunnel)
{
    uint8_t     *data_p = rdpa_tunnel->tunnel_header;
    BlogTuple_t *deltx_p = &blog_p->deltx_tuple;
    BlogTupleV6_t  *deltxV6_p = &blog_p->del_tupleV6;
    BlogGre_t   *gre_p = &blog_p->gretx;
    uint16_t    proto;
    uint16_t    icsum16, ip_hdr_len;
    uint16_t    l3_total_length, csum;

    if (blog_p->tx.info.bmap.GRE_ETH)
        proto = BLOG_ETH_P_ETH_BRIDGING;
    else
    {
        if(blog_p->tx.info.bmap.PLD_IPv4)
            proto = BLOG_ETH_P_IPV4;
        else
            proto = BLOG_ETH_P_IPV6;   
    }

    /* Copy tunnel L2 */
    memcpy(data_p, blog_p->tx.l2hdr, blog_p->tx.length);
    data_p += blog_p->tx.length;
    rdpa_tunnel->layer3_offset = rdpa_tunnel->tunnel_header_length = blog_p->tx.length;

    if (blog_p->tx.info.bmap.DEL_IPv4)
    {
        ip_hdr_len = BLOG_IPV4_HDR_LEN;
        l3_total_length = ip_hdr_len + gre_p->hlen;
        /* compute incremental checksum for ipid, total length and tos fields */
        icsum16 = _compute_icsum16( 0, 0, htons(l3_total_length) );
        csum = _apply_icsum( deltx_p->check, (__force uint32_t)icsum16 );

        /* Insert IPV4 Hdr */
        *((uint16_t *)data_p + 0) = htons(0x4500 | deltx_p->tos);
        *((uint16_t *)data_p + 1) = htons(l3_total_length);
        *((uint16_t *)data_p + 2) = gre_p->ipid - htons(1);
        *((uint16_t *)data_p + 3) = (gre_p->fragflag ? htons(BLOG_IP_FLAG_DF) : 0);
        *((uint16_t *)data_p + 4) = htons((deltx_p->ttl << 8) | (BLOG_IPPROTO_GRE & 0xFF));
        *((uint16_t *)data_p + 5) = csum;
        memcpy((uint8_t*)&((BlogIpv4Hdr_t *)data_p)->sAddr, (uint8_t*)&deltx_p->saddr, sizeof(deltx_p->saddr) + sizeof(deltx_p->daddr));

        rdpa_tunnel->local_ip.family = bdmf_ip_family_ipv4;
        rdpa_tunnel->local_ip.addr.ipv4 = htonl(deltx_p->saddr);
    }
    else
    {
        ip_hdr_len = BLOG_IPV6_HDR_LEN;

        /* Insert IPV6 Hdr */
        *((uint16_t *)data_p + 0) = htons(0x6000);
        *((uint16_t *)data_p + 1) = htons(0x0000);
        *((uint16_t *)data_p + 2) = htons(gre_p->hlen);
        *((uint16_t *)data_p + 3) = htons(((BLOG_IPPROTO_GRE & 0xFF) << 8) | (deltxV6_p->rx_hop_limit));
        memcpy((uint8_t*)&((BlogIpv6Hdr_t *)data_p)->sAddr, (uint8_t*)&deltxV6_p->saddr, sizeof(deltxV6_p->saddr) + sizeof(deltxV6_p->daddr));

        rdpa_tunnel->local_ip.family = bdmf_ip_family_ipv6;
        memcpy(&rdpa_tunnel->local_ip.addr, deltxV6_p->saddr.p8, sizeof(deltxV6_p->saddr));
    }
    data_p += ip_hdr_len;

    rdpa_tunnel->gre_proto_offset =  rdpa_tunnel->layer3_offset + ip_hdr_len + sizeof(gre_p->gre_flags.u16);

    *((uint16_t *)data_p + 0) = htons(gre_p->gre_flags.u16);
    *((uint16_t *)data_p + 1) = htons(proto);
    if(gre_p->gre_flags.keyIe)
    {
        *((uint32_t *)data_p + 1) = htonl(gre_p->key);
    }

    rdpa_tunnel->tunnel_header_length += ip_hdr_len + gre_p->hlen;

    if (rdpa_tunnel->tunnel_header_length > RDPA_MAX_TUNNEL_HEADER_LEN)
        __logError("tunnel header length(%d) > RDPA_MAX_TUNNEL_HEADER_LEN(%d)", rdpa_tunnel->tunnel_header_length, RDPA_MAX_TUNNEL_HEADER_LEN);

    rdpa_tunnel->tunnel_type = (proto == BLOG_ETH_P_ETH_BRIDGING ? rdpa_tunnel_l2gre : rdpa_tunnel_l3gre);
}

void build_dslite_tunnel_header(Blog_t *blog_p, rdpa_tunnel_cfg_t *rdpa_tunnel)
{
    uint8_t     *data_p = rdpa_tunnel->tunnel_header;
    uint16_t    ip_hdr_len;

    /* Copy tunnel L2 */
    rdpa_tunnel->layer3_offset = 0;

    ip_hdr_len = BLOG_IPV6_HDR_LEN;

    /* Insert IPV6 Hdr */
    *((uint16_t *)data_p + 0) = htons(0x6000);
    *((uint16_t *)data_p + 1) = htons(0x0000);
    *((uint16_t *)data_p + 2) = 0;
    *((uint16_t *)data_p + 3) = htons(((BLOG_IPPROTO_IPIP & 0xFF) << 8) | (blog_p->tupleV6.rx_hop_limit));
    memcpy((uint8_t*)&((BlogIpv6Hdr_t *)data_p)->sAddr, (uint8_t*)&blog_p->tupleV6.saddr, sizeof(blog_p->tupleV6.saddr) + sizeof(blog_p->tupleV6.daddr));

    rdpa_tunnel->local_ip.family = bdmf_ip_family_ipv6;
    memcpy(&rdpa_tunnel->local_ip.addr, blog_p->tupleV6.saddr.p8, sizeof(blog_p->tupleV6.saddr));

    data_p += ip_hdr_len;

    rdpa_tunnel->tunnel_header_length = ip_hdr_len;

    if (rdpa_tunnel->tunnel_header_length > RDPA_MAX_TUNNEL_HEADER_LEN)
        __logError("tunnel header length(%d) > RDPA_MAX_TUNNEL_HEADER_LEN(%d)", rdpa_tunnel->tunnel_header_length, RDPA_MAX_TUNNEL_HEADER_LEN);

    rdpa_tunnel->tunnel_type = rdpa_tunnel_dslite;
}

static void pktrunner_bind(int enable)
{
#if !defined(RDP_SIM)
    FhwBindHwHooks_t no_hw_hooks = {};
    FhwBindHwHooks_t hw_hooks_flow = {};
    FhwBindHwHooks_t hwHooks_mc_wlist = {};

    /* Initialize the clear function pointer */
    no_hw_hooks.fhw_clear_fn = &fc_clear_hook_runner;

    /* Initialize FLOW --- START */
    hw_hooks_flow.activate_fn = (HOOKP32)runner_activate_flow;
    hw_hooks_flow.deactivate_fn = (HOOK4PARM)runner_deactivate_flow;
    hw_hooks_flow.update_fn = (HOOK3PARM)runner_update_flow;
    hw_hooks_flow.refresh_fn = (HOOK3PARM)runner_refresh_flow;
    hw_hooks_flow.refresh_pathstat_fn = (HOOK3PARM)runnerRefreshPathStat;
    hw_hooks_flow.fhw_clear_fn = &fc_clear_hook_runner;
    hw_hooks_flow.reset_stats_fn =(HOOK32)runnerResetStats;
    hw_hooks_flow.cap = (1<<HW_CAP_IPV4_UCAST) | (1<<HW_CAP_IPV6_UCAST) | (1<<HW_CAP_IPV6_TUNNEL) |
        (1<<HW_CAP_L2_UCAST);
    hw_hooks_flow.cap |= (1<<HW_CAP_IPV4_MCAST) | (1<<HW_CAP_IPV6_MCAST);

    hw_hooks_flow.clear_fn = (HOOK32)runner_clear;
    hw_hooks_flow.max_ent = PKTRUNNER_MAX_FLOWS + max_num_of_mcast_flows;
    /* Number of HW_pathstat needs to match with HWACC counter group assignment */
    hw_hooks_flow.max_hw_pathstat = RDPA_IP_CLASS_MAX_PATHS;

    hwHooks_mc_wlist.activate_fn = (HOOKP32)fhwPktRunnerMcastWhitelistActivate;
    hwHooks_mc_wlist.deactivate_fn = (HOOK4PARM)fhwPktRunnerMcastWhitelistDeactivate;
    hwHooks_mc_wlist.fhw_clear_fn = &fc_clear_hook_runner;
    hwHooks_mc_wlist.cap = 1 << HW_CAP_MCAST_WHITELIST;
    hwHooks_mc_wlist.max_ent = PKTRUNNER_MAX_FLOWS_MCAST_WHITELIST;
    hwHooks_mc_wlist.max_hw_pathstat = 0;

    /* Block flow-cache from packet processing and try to push the flows */
    blog_lock();

    fhw_bind_hw(FHW_PRIO_0, enable ? &hw_hooks_flow : &no_hw_hooks);
    PKTRUNNER_STATE(PKTRUNNER_ACCEL_FLOW).status = enable;

    if ( 0 )
    {
        fhw_bind_hw(FHW_PRIO_1, enable ? &hwHooks_mc_wlist : &no_hw_hooks);
        PKTRUNNER_STATE(PKTRUNNER_ACCEL_MCAST_WHITELIST).status = enable;
    }

    blog_unlock();

    __logNotice("%s runner binding to Flow Cache", enable ? "Enabled" :
        "Disabled");
#endif
}

/* Binds the Runner Protocol Layer handler functions to Flow Cache hooks. */
static int __init pktrunner_enable(void)
{
    int ret = 0;

    memset(&pktRunner_data_g, 0, sizeof(pktRunner_data_g));
    /* Initialize Flows accelerator (ucast and mcast) */
    ret = _pktRunnerAccelInit(PKTRUNNER_ACCEL_FLOW, PKTRUNNER_MAX_FLOWS + max_num_of_mcast_flows, 0, 0, 1);
    ret = ret ? ret : _pktRunnerAccelInit(PKTRUNNER_ACCEL_MCAST_WHITELIST, PKTRUNNER_MAX_FLOWS_MCAST_WHITELIST,
        0, 1 /* ref_cnt*/, 1);

    pktrunner_bind(1);
#if defined(CONFIG_BCM_FHW)
    BCM_ASSERT((fc_clear_hook_runner != (FC_CLEAR_HOOK)NULL));
#endif

    return ret;
}

/* Clears all active Flow Cache associations with Runner.
 * Unbind all flow cache to Runner hooks. */
static void __exit pktrunner_disable(void)
{
    pktrunner_bind(0);

#if defined(CONFIG_BCM_FHW)
    /* Clear system wide active FlowCache associations, and disable learning. */
    __clear_fcache(System_e);
#endif

    _pktRunnerAccelExit(PKTRUNNER_ACCEL_FLOW);
    _pktRunnerAccelExit(PKTRUNNER_ACCEL_MCAST_WHITELIST);
}

int pktrunner_debug(int log_level)
{
    if(log_level >= 0 && log_level < BCM_LOG_LEVEL_MAX)
    {
        bcmLog_setLogLevel(BCM_LOG_ID_PKTRUNNER, log_level);
    }
    else
    {
        __logError("Invalid Log level %d (max %d)", log_level, BCM_LOG_LEVEL_MAX);
        return -1;
    }

    return 0;
}

static int l2_class_created_here;
static int ip_class_created_here;

#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
static void dhd_helper_destruct(void)
{
    int i;

    for (i = 0; i < MAX_NUM_OF_RADIOS; i++)
    {
        if (dhd_helper_obj[i])
            bdmf_put(dhd_helper_obj[i]);
    }
}
#endif

static void cleanup_rdpa_objects(void)
{
    if (iptv)
    {
        bdmf_destroy(iptv);
    }
    if (ip_class)
    {
        if (ip_class_created_here)
            bdmf_destroy(ip_class);
        else
            bdmf_put(ip_class);
    }
    if (l2_class)
    {
        if (l2_class_created_here)
            bdmf_destroy(l2_class);
        else
            bdmf_put(l2_class);
    }
#if !defined(CONFIG_ONU_TYPE_SFU)
    pktrunner_wlan_destruct();
#endif

#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
    dhd_helper_destruct();
#endif

    runnerMcastWhitelist_destruct();

    bdmf_put(system_obj);
}

static int pktrunner_fc_accel_mode_set( uint32_t accel_mode )
{
    int rc = 0;
    rdpa_system_init_cfg_t init_cfg = {};

    rdpa_system_init_cfg_get(system_obj, &init_cfg);    

    if (accel_mode == BLOG_ACCEL_MODE_L23)
    {
        __logNotice("Fcache change acceleration mode to MODE_L2+L3");

        if (l2_class_created_here)
            return 0;

        rc = rdpa_l2_class_get(&l2_class);
        if (rc)
        {
            BDMF_MATTR(l2_class_attrs, rdpa_l2_class_drv());

            __logNotice("l2_class does not exists, Creating...");

            rdpa_l2_class_flow_idx_pool_ptr_set(l2_class_attrs, &rdpa_shared_flw_idx_pool_g);
            rc = bdmf_new_and_set(rdpa_l2_class_drv(), NULL, l2_class_attrs, &l2_class);
            if (rc)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa l2_class object does not exist and can't be created.\n");
                goto go_out;
            }
            l2_class_created_here = 1;
        }
    }
    else
    {
        __logNotice("Fcache change acceleration mode to MODE_L3");

        if (l2_class)
        {
            __logNotice("Deleting l2_class object");
            if (l2_class_created_here)
                bdmf_destroy(l2_class);
            else
                bdmf_put(l2_class);

            l2_class = NULL;
            l2_class_created_here = 0;
        }
    }

go_out:
    return rc;
}

#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
static void dhd_helper_construct(void)
{
    int i = 0;
    for ( ;i < MAX_NUM_OF_RADIOS; i++)
    {
        dhd_helper_obj[i] = NULL;
    }
}
#endif

extern uint32_t mcast_key_exclude_fields;
int __init runnerProto_construct(void)
{
    int rc;
    bdmf_object_handle port_obj;
    rdpa_wan_type wan_type = rdpa_wan_none;
    BDMF_MATTR(iptv_attrs, rdpa_iptv_drv());
    rdpa_l4_filter_cfg_t l4_filter_cfg = {
        .action = rdpa_forward_action_forward,
    };
    rdpa_system_init_cfg_t init_cfg = {};

#if (BLOG_HDRSZ_MAX > 38)
#error Runner FW does not support L2 header bigger then 38
#endif


    rdpa_system_get(&system_obj);

    DLIST_INIT(&tunnel_entry_list);

    bcmLog_setLogLevel(BCM_LOG_ID_PKTRUNNER, BCM_LOG_LEVEL_ERROR);

    rc = rdpa_system_init_cfg_get(system_obj, &init_cfg);
    if (!rc)
    {        
        BDMF_MATTR(ip_class_attrs, rdpa_ip_class_drv());

        /* Only create for L2+L3; Mcast needs separate pool */
        memset(&rdpa_shared_flw_idx_pool_g, 0, sizeof(rdpa_shared_flw_idx_pool_g));
        rc = rdpa_flow_idx_pool_init(&rdpa_shared_flw_idx_pool_g, PKTRUNNER_MAX_FLOWS, "L2L3-class");
        if (rc)
        {
            return rc;
        }


        rc = rdpa_ip_class_get(&ip_class);
        if (rc)
        {
            rdpa_ip_class_flow_idx_pool_ptr_set(ip_class_attrs, &rdpa_shared_flw_idx_pool_g);
            rc = bdmf_new_and_set(rdpa_ip_class_drv(), NULL, ip_class_attrs, &ip_class);
            if (rc)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa ip_class object does not exist and can't be created.\n");
                goto error;
            }
            ip_class_created_here = 1;
        }

        rc = rdpa_ip_class_key_type_set(ip_class, RDPA_IP_CLASS_6TUPLE);
        if (rc)
        {
             BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "ip_class can't set key_type attribute\n");
        }
    }

    {
        rdpa_system_init_cfg_t init_cfg;
        rdpa_iptv_lookup_method method = iptv_lookup_method_group_ip_src_ip_vid;

        rdpa_system_init_cfg_get(system_obj, &init_cfg);

        rc = rdpa_port_get(rdpa_if_wan0, &port_obj); /* FIXME : Multi-WAN XPON */
        if (rc)
        {
           BCM_LOG_INFO(BCM_LOG_ID_PKTRUNNER, "faild to get port rdpa_if_wan0");
        }
        else
        {
            rdpa_port_wan_type_get(port_obj, &wan_type);
            bdmf_put(port_obj);
        }

        /* CMS MCPD is expected to work with group_ip_src_ip only in GPON SFU */
        if ((wan_type == rdpa_wan_gpon || wan_type == rdpa_wan_xgpon) &&
            init_cfg.operation_mode == rdpa_method_prv)
        {
            method = iptv_lookup_method_group_ip_src_ip;
        }

        rdpa_iptv_lookup_method_set(iptv_attrs, method);
        rc = bdmf_new_and_set(rdpa_iptv_drv(), NULL, iptv_attrs, &iptv);
        if (rc)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa iptv object does not exist and can't be created, rc = %d\n", rc);
            goto error;
        }
    }
    rc = rdpa_iptv_flow_mode_get(iptv, &mc_flow_mode);
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "Cannot get multicast flow mode, rc = %d\n", rc);
        goto error;
    }

    if (mc_flow_mode != rdpa_iptv_flow_mode_multi)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "Single-flow mode supports only provisioned flows, "
            "which are not supported any more\n");
        rc = -1;
        goto error;
    }

    /* SW FC excludes DEI and PBIT from lookup, but blog allows that and in simulator mode we test it */
#if !defined(RDP_SIM)
    mcast_key_exclude_fields = rdpa_mcast_flow_key_exclude_pbit_field |
        rdpa_mcast_flow_key_exclude_dei_field | rdpa_mcast_flow_key_exclude_etype_field; /* not supported yet in BLOG */
#else
    mcast_key_exclude_fields = rdpa_mcast_flow_key_exclude_etype_field;
#endif

    max_num_of_mcast_flows = PKTRUNNER_MAX_FLOWS_MCAST;
    rdpa_iptv_mcast_key_exclude_fields_set(iptv, mcast_key_exclude_fields);

#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
    dhd_helper_construct();
#endif

    l4_filter_cfg.protocol = IPPROTO_GRE;

    /* enable rdpa l4 esp filter */
    l4_filter_cfg.protocol = IPPROTO_ESP;
    rc = rdpa_ip_class_l4_filter_set(ip_class, rdpa_l4_filter_esp, &l4_filter_cfg);
    if (rc)
        BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "ip_class can't set l4_filter[esp] attribute\n");

    __logNotice("Reset and initialized pktrunner protocol Layer");

    rc = runnerMcastWhitelist_construct(NULL);
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "Can't create mcast_whitelist object\n");
        goto error;
    }

    runnerHost_construct();

    /* bind to acceleration mode function hook used by blog/flow_cache */
    blog_accel_mode_set_fn = (blog_accel_mode_set_t) pktrunner_fc_accel_mode_set;

    /* Set the Runner acceleration mode to be in sync with blog/flow cache */
    pktrunner_fc_accel_mode_set( blog_support_get_accel_mode() );

    pktrunner_enable();

#if !defined(CONFIG_ONU_TYPE_SFU)
    pktrunner_wlan_construct();
#endif
    return 0;

error:

    cleanup_rdpa_objects();
    rdpa_flow_idx_pool_exit(&rdpa_shared_flw_idx_pool_g);
    return rc;
}

void __exit runnerProto_destruct(void)
{
    int rc = 0;
    rdpa_system_init_cfg_t init_cfg = {};

    rc = rdpa_system_init_cfg_get(system_obj, &init_cfg); /* get init_cfg before release system object */

    pktrunner_disable();
    cleanup_rdpa_objects();

    if (!rc)
        rdpa_flow_idx_pool_exit(&rdpa_shared_flw_idx_pool_g);

    runnerHost_destruct();
}

int fhwPktRunnerActivate(Blog_t *blog_p, uint32_t key_in)
{
    return runner_activate_flow(blog_p, key_in);
}

int fhwPktRunnerDeactivate(uint32_t pktrunner_key, uint32_t *pktsCnt_p,
                           uint32_t *octetsCnt_p, struct blog_t *blog_p)
{
    return runner_deactivate_flow(pktrunner_key, pktsCnt_p, octetsCnt_p, blog_p);
}



