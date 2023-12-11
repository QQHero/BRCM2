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
* File Name  : ptkrunner_l2_ucast.c
*
* Description: This implementation translates L2 Unicast Blogs into Runner Flows
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
#include "fcachehw.h"
#include "bcmxtmcfg.h"
#endif

#include "cmdlist_api.h"

#include <rdpa_api.h>

#include "pktrunner_proto.h"
#include "pktrunner_mcast.h"

#if defined(CONFIG_BCM_CMDLIST_SIM)
#include "runner_sim.h"
#endif

/*******************************************************************************
 *
 * Global Variables and Definitions
 *
 *******************************************************************************/
#if !defined(CONFIG_BCM_CMDLIST_SIM)
static int l2_ucast_class_created_here   = 0;
static bdmf_object_handle l2_ucast_class = NULL;
#endif


int pktrunner_l2_ucast_cmdlist_create(Blog_t *blog_p, uint8_t *prependData_p, int prependSize, 
                                   void **cmdlist_buffer_pp, rdpa_l2_flow_result_t *result_p)
{
#if (defined(BCM63158) && !defined(BCM63158_CMDLIST_XPE_SIM)) || (defined(CONFIG_BCM_CMDLIST_GPE) && (defined(BCM63146) || defined(BCM4912) || defined(BCM6813)))
    cmdlist_cmd_target_t target = CMDLIST_CMD_TARGET_SRAM;
#else
    cmdlist_cmd_target_t target = CMDLIST_CMD_TARGET_DDR;
#endif
    int cmd_list_data_length;
    int tx_adjust;
    int err;
	
    cmdlist_init(result_p->cmd_list, RDPA_CMD_LIST_UCAST_LIST_SIZE,
                 RDPA_CMD_LIST_UCAST_LIST_OFFSET);

    err = cmdlist_l2_ucast_create(blog_p, target, prependData_p, prependSize,
                                   cmdlist_buffer_pp, PKTRUNNER_BRCM_TAG_MODE, &tx_adjust);
    if(err != 0)
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

    return err;
}

int runnerL2Ucast_activate(Blog_t *blog_p, uint8_t *prependData_p, int prependSize, void **cmdlist_buffer_pp, int *err)
{
    int flowIdx = FHW_TUPLE_INVALID;
    rdpa_l2_flow_info_t l2_flow;
#ifdef RDP_UFC_TUNNEL
    rdpa_ip_flow_info_t ip_flow_tunnel = {};
#endif

    memset(&l2_flow, 0, sizeof(rdpa_l2_flow_info_t));
    *err = pktrunner_l2_ucast_cmdlist_create(blog_p, prependData_p, prependSize, cmdlist_buffer_pp, &l2_flow.result);
    if(*err != 0)
    {
        __logInfo("Could not l2_ucast_cmdlist_create");

        goto abort_activate;
    }
#ifdef RDP_UFC_TUNNEL
    if (GRE_TERMINATION(blog_p) || RX_VXLAN(blog_p))
    {
        ip_flow_tunnel.result.cmd_list_data_length = TUNNEL_FLOW_CONTEXT_SIZE;
        ip_flow_tunnel.result.cmd_list_length = TUNNEL_FLOW_CONTEXT_SIZE;
        ip_flow_tunnel.result.is_tunnel = 1;

        *err = __ucastSetFwdAndFilters(blog_p, &ip_flow_tunnel);
        if(*err != 0)
        {
            __logInfo("Could not setFwdAndFilters for tunnel");

            goto abort_activate;
        }
    }
#endif
    *err = __l2ucastSetFwdAndFilters(blog_p, &l2_flow);
    if(*err != 0)
    {
        __logInfo("Could not setFwdAndFilters");

        goto abort_activate;
    }

#if defined(CONFIG_BCM_CMDLIST_SIM)
    {
        int skip_brcm_tag_len = pktrunner_is_rx_enet_wan_port(blog_p) ? BRCM_TAG_TYPE2_LEN : 0;

        runnerSim_activate(blog_p, l2_flow.result.cmd_list, RDPA_CMD_LIST_UCAST_LIST_OFFSET,
                           NULL, 0, skip_brcm_tag_len, cmd_list_data_length, tx_adjust);
    }
#else
    {
        bdmf_index index_flow = FHW_TUPLE_INVALID;
#ifdef RDP_UFC_TUNNEL
        uint8_t tunnel_index_ref = TUNNEL_INVALID_NUMBER;
        if (RX_VXLAN(blog_p))
        {
            *err = tunnel_construct_flow(blog_p, &ip_flow_tunnel, &tunnel_index_ref);
            if (*err != 0)
            {
                __logError("Cannot tunnel_construct_flow");
                goto abort_activate;
            }
            __logInfo("[tunnel_construct_flow] tunnel_index_ref: %d", tunnel_index_ref);
        }

        l2_flow.key.vtag_num = blog_p->vtag_num;

        l2_flow.result.tunnel_index_ref = tunnel_index_ref;
        __logInfo("Cannot rdpa_ucast_flow_add: collision list full");
#endif
        *err = rdpa_l2_ucast_flow_add(l2_ucast_class, &index_flow, &l2_flow);
        if (*err != 0)
        {
            __logInfo("Cannot rdpa_l2_ucast_flow_add");

            goto abort_activate;
        }
        else if(index_flow == FHW_TUPLE_INVALID)
        {
            __logInfo("Cannot rdpa_l2_ucast_flow_add: collision list full");

            goto abort_activate;
        }
        __logInfo("[rdpa_l2_ucast_flow_add] index_flow: %ld", index_flow);

        flowIdx = (int)index_flow;
    }
#endif /* CONFIG_BCM_CMDLIST_SIM */

    return flowIdx;

abort_activate:
    if(*cmdlist_buffer_pp)
    {
        cmdlist_buffer_free(*cmdlist_buffer_pp);
    }

    return FHW_TUPLE_INVALID;
}

#if !defined(CONFIG_BCM_CMDLIST_SIM)
static int l2UcastDeleteFlow(uint32_t rdpa_flow_key, int speculative, void *cmdlist_buffer_p)
{
    rdpa_l2_flow_info_t l2_flow;
    int rc;

    rc = rdpa_l2_ucast_flow_get(l2_ucast_class, rdpa_flow_key, &l2_flow);
    if(rc < 0)
    {
        if(!speculative)
        {
            __logError("Cannot rdpa_l2_ucast_flow_get (rdpa_flow_key %u)", rdpa_flow_key);
        }

        return rc;
    }

#ifdef RDP_UFC_TUNNEL
    if (l2_flow.result.tunnel_index_ref != TUNNEL_INVALID_NUMBER)
    {
        rc = tunnel_destruct_flow(l2_flow.result.tunnel_index_ref, rdpa_flow_key);
        if(rc < 0)
        {
            return rc;
        }
    }
#endif    
    rc = rdpa_l2_ucast_flow_delete(l2_ucast_class, rdpa_flow_key);
    if(rc < 0)
    {
        __logError("Cannot rdpa_l2_ucast_flow_delete (rdpa_flow_key %u)", rdpa_flow_key);

        return rc;
    }

    if(cmdlist_buffer_p)
    {
        cmdlist_buffer_free(cmdlist_buffer_p);
        cmdlist_buffer_p = NULL;
    }

    return 0;
}

int runnerL2Ucast_deactivate(uint32_t rdpa_flow_key, void *cmdlist_buffer_p)
{
    return l2UcastDeleteFlow(rdpa_flow_key, 0, cmdlist_buffer_p);
}

int runnerL2Ucast_update(BlogUpdate_t update, uint32_t rdpa_flow_key, Blog_t *blog_p)
{
    rdpa_l2_flow_info_t l2_flow = {};

    switch(update)
    {
        case BLOG_UPDATE_DPI_QUEUE:
            l2_flow.result.service_queue_id = blog_p->dpi_queue;
            break;

        case BLOG_UPDATE_DPI_PRIORITY:
#if defined(CONFIG_BCM_DPI_WLAN_QOS)
            rdpa_l2_ucast_flow_get(l2_ucast_class, rdpa_flow_key, &l2_flow);
            if (blog_p->wfd.nic_ucast.is_chain)
                l2_flow.result.wfd.nic_ucast.priority = blog_p->wfd.nic_ucast.priority;
            else{
                if (blog_p->rnr.is_wfd)
                    l2_flow.result.wfd.dhd_ucast.flowring_idx = blog_p->wfd.dhd_ucast.flowring_idx;
                else{
                    l2_flow.result.rnr.priority = blog_p->rnr.priority;
                    l2_flow.result.rnr.flowring_idx = blog_p->rnr.flowring_idx;
                }
            }
#endif
            break;

        default:
            __logError("Invalid BLOG Update: <%d>", update);
            return -1;
    }

    return rdpa_l2_ucast_flow_set(l2_ucast_class, rdpa_flow_key, &l2_flow);
}

/*
 *------------------------------------------------------------------------------
 * Function   : runnerL2Ucast_refresh
 * Description: This function is invoked to collect flow statistics
 * Parameters :
 *  flowIdx :  30bit index to refer to a Runner flow
 * Returns    : Total hits on this connection.
 *------------------------------------------------------------------------------
 */
int runnerL2Ucast_refresh(int rdpa_flow_key, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    rdpa_stat_t flow_stat;
    int rc;

    rc = rdpa_l2_ucast_flow_stat_get(l2_ucast_class, rdpa_flow_key, &flow_stat);
    if (rc < 0)
    {
//        __logDebug("Could not get flowIdx<%d> stats, rc %d", flowIdx, rc);
        return rc;
    }

    *pktsCnt_p = flow_stat.packets; /* cummulative packets */
    *octetsCnt_p = flow_stat.bytes;

    __logDebug( "rdpa_flow_key<%03u> "
                "cumm_pkt_hits<%u> cumm_octet_hits<%u>\n",
                rdpa_flow_key, *pktsCnt_p, *octetsCnt_p );
    return 0;
}

#else /* CONFIG_BCM_CMDLIST_SIM */

int runnerL2Ucast_deactivate(uint32_t rdpa_flow_key, void *cmdlist_buffer_p)
{
    return 0;
}

int runnerL2Ucast_refresh(int rdpa_flow_key, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    *pktsCnt_p = 1;
    *octetsCnt_p = 1;

    return 0;
}

#endif /* CONFIG_BCM_CMDLIST_SIM */

/*
 *------------------------------------------------------------------------------
 * Function   : runnerL2Ucast_refresh_pathstat
 * Description: This function is invoked to refresh path statistics
 * Parameters :
 *  pathIdx : 8bit index to refer to a path
 * Returns    : Total hits on this connection.
 *------------------------------------------------------------------------------
 */
#if !defined(CONFIG_BCM_CMDLIST_SIM)
int runnerL2Ucast_refresh_pathstat(int pathIdx, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    rdpa_stat_t pathstat;
    int rc;

    rc = rdpa_l2_ucast_pathstat_get(l2_ucast_class, pathIdx, &pathstat);
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
#else
int runnerL2Ucast_refresh_pathstat(int pathIdx, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    *pktsCnt_p = 1;
    *octetsCnt_p = 1;

    return 0;
}
#endif

/*
*******************************************************************************
* Function   : runnerL2Ucast_construct
* Description: Constructs the Runner Protocol layer
*******************************************************************************
*/
int __init runnerL2Ucast_construct(void *idx_p, void *disp_p)
{
#if !defined(CONFIG_BCM_CMDLIST_SIM)
    int ret;

    BDMF_MATTR(l2_ucast_attrs, rdpa_l2_ucast_drv());

#ifdef RDP_UFC_TUNNEL
    tunnel_table_init();
#endif    

    ret = rdpa_l2_ucast_get(&l2_ucast_class);
    if (ret)
    {
        ret = rdpa_l2_ucast_flow_idx_pool_ptr_set(l2_ucast_attrs, idx_p);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa l2_ucast_class object cannot set rdpa_flow_idx_pool.\n");
            return ret;
        }
        ret = rdpa_l2_ucast_flow_disp_pool_ptr_set(l2_ucast_attrs, disp_p);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa l2_ucast_class object cannot set flow_disp_pool_ptr.\n");
            return ret;
        }
        ret = bdmf_new_and_set(rdpa_l2_ucast_drv(), NULL, l2_ucast_attrs, &l2_ucast_class);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa l2_ucast_class object does not exist and can't be created.\n");
            return ret;
        }
        l2_ucast_class_created_here = 1;
    }
#endif

    bcm_print("Initialized Runner L2 Unicast Layer\n");

    return 0;
}

/*
*******************************************************************************
* Function   : runnerL2Ucast_destruct
* Description: Destructs the Runner Protocol layer
* WARNING: __exit_refok suppresses warnings from CONFIG_DEBUG_SECTION_MISMATCH
*          This should only be called from __init or __exit functions.
*******************************************************************************
*/
void __ref runnerL2Ucast_destruct(void)
{
#if !defined(CONFIG_BCM_CMDLIST_SIM)
    if(l2_ucast_class)
    {
        /* L2 flow deletion will happen when L2 class is destroyed */

        if(l2_ucast_class_created_here)
        {
            bdmf_destroy(l2_ucast_class);
            l2_ucast_class_created_here = 0;
        }
        else
        {
            bdmf_put(l2_ucast_class);
        }
    }
#endif
}
