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
* File Name  : runner_mcast.c
*
* Description: This implementation supports the Runner Multicast Flows
*
*******************************************************************************
*/

#if defined(RDP_SIM)
#include "pktrunner_rdpa_sim.h"
#else
#include <linux/module.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/blog_rule.h>

#include <linux/bcm_log.h>
#include <linux/blog.h>
#include <net/ipv6.h>
#include "fcachehw.h"

#include "bcmtypes.h"
#include "bcm_vlan.h"
#endif

#include "cmdlist_api.h"

#include <rdpa_api.h>

#include "pktrunner_proto.h"
#include "pktrunner_ucast.h"
#include "pktrunner_mcast.h"

#if defined(CONFIG_BCM_CMDLIST_SIM)
#include "runner_sim.h"
#endif

/*******************************************************************************
 *
 * Global Variables and Definitions
 *
 *******************************************************************************/

/* IPv4 Multicast range: 224.0.0.0 to 239.255.255.255 (E0.*.*.* to EF.*.*.*) */
#define RUNNER_MCAST_IPV4_MASK  0xF0000000
#define RUNNER_MCAST_IPV4_VAL   0xE0000000

#define RUNNER_MCAST_IS_MCAST_IPV4(_addr)                                  \
    ( ((_addr) & RUNNER_MCAST_IPV4_MASK) == RUNNER_MCAST_IPV4_VAL )

/* IPv6 Multicast range:  FF00::/8  */
#define RUNNER_MCAST_IPV6_VAL   0xFF

#define RUNNER_MCAST_IS_MCAST_IPV6(_addr)                                \
    ( (_addr)  == RUNNER_MCAST_IPV6_VAL)

#define WLAN_RADIO_IF(radio_idx)      (rdpa_if_wlan0 + radio_idx)

#if !defined(CONFIG_BCM_CMDLIST_SIM)
static int mcast_class_created_here   = 0;
static bdmf_object_handle mcast_class = NULL;
#endif

/*******************************************************************************
 *
 * Local Functions
 *
 *******************************************************************************/

static blogRuleAction_t *__findBlogRuleCommand(blogRule_t *blogRule_p,
                                               blogRuleCommand_t blogRuleCommand,
                                               uint32 *cmdIndex_p)
{
    blogRuleAction_t *action_p;
    int i;

    for(i=*cmdIndex_p; i<blogRule_p->actionCount; ++i)
    {
        action_p = &blogRule_p->action[i];

        if(action_p->cmd == blogRuleCommand)
        {
            *cmdIndex_p = i;

            return action_p;
        }
    }

    return NULL;
}

static inline blogRuleAction_t *findBlogRuleCommand(blogRule_t *blogRule_p,
                                                    blogRuleCommand_t blogRuleCommand)
{
    uint32 cmdIndex = 0;

    return __findBlogRuleCommand(blogRule_p, blogRuleCommand, &cmdIndex);
}

static void mcast_flow_master_ctx_set_fwd_and_trap(Blog_t *blog_p, rdpa_ip_flow_result_t *result)
{
    result->fwd_and_trap = blog_p->fwd_and_trap;
}

static int __mcastSetCmdList(Blog_t *blog_p, rdpa_mcast_flow_result_t *result_p,
                             void **cmdlist_buffer_pp)
{
    return pktrunner_ucast_cmdlist_create(blog_p, NULL, 0, cmdlist_buffer_pp, result_p);
}
static int __mcastSetLkupKey(Blog_t *blog_p, flowParams_t *flow_params_p, rdpa_mcast_flow_key_t *key_p)
{
    int ret;

    ret = pktrunner_build_mcast_lkp_key(blog_p, key_p);
    if (ret)
    {
        __logError("Failed to pktrunner_build_mcast_lkp_key, err<%d>", ret);
        return ret;
    }

    key_p->rx_if = flow_params_p->key.ingress_if;
    /* Client index in the key */
    key_p->entry_idx = blog_p->mcast_client_id;
    return ret;
}
static int __mcastSetMasterResult(Blog_t *blog_p, flowParams_t *flow_params_p, rdpa_mcast_flow_result_t *result_p)
{
    int ret = 0;
    /* Set client vector/bitmap */
    ret = mcast_flow_master_ctx_set_clients_vector(blog_p, result_p);
    if (ret)
    {
        __logError("Failed to set client bitmap, err<%d>", ret);
        return ret;
    }

    /* Set Service Queue - TBD first understand */

    /* Set Policer - TBD first understand */

    return ret;
}
static int __mcastSetClientResult(Blog_t *blog_p, flowParams_t *flow_params_p, rdpa_mcast_flow_result_t *result_p)
{
    /* Once the new configuration for a policer or a service queue is set for the client flow, it's taken
     * globally for all flows under the same master, meaning overwrite the master flow configuration. The
     * cleanup of the service queue and the policer from the master is the responsibility of the upper layer. */

    /* TBD - need to understand */

    return 0;
}
static int __mcastSetResult(Blog_t *blog_p, flowParams_t *flow_params_p, rdpa_mcast_flow_result_t *result_p)
{
    int ret = 0;
    /* If master and client specific result/context */
    if (blog_p->mcast_client_id == BLOG_MCAST_MASTER_CLIENT_ID)
    {
        ret = __mcastSetMasterResult(blog_p,flow_params_p,result_p);
    }
    else
    {
        ret = __mcastSetClientResult(blog_p,flow_params_p,result_p);
    }
    if (ret)
    {
        return ret;
    }
    /* Setting all the fields in result even for master
       TODO - should we differentiate? */
    return ucastSetResult(blog_p, flow_params_p, result_p);
}

int runnerMcast_activate(Blog_t *blog_p, int *isActivation_p, void **cmdlist_buffer_pp, int *err)
{
    rdpa_mcast_flow_t flow = {};
    flowParams_t flow_params = {};
    bdmf_index flow_idx = FHW_TUPLE_INVALID;

    /* In multi-flow implementation, isActivation has no meaning.
       TBD - clean this up once all platforms done */
    *isActivation_p = 1;

    BCM_ASSERT(blog_p != BLOG_NULL);

   __logInfo("ACTIVATE");

    __debug("\n%s: ************** Multicast Flow **************\n\n", __FUNCTION__);

    *err = L2L3ParseBlogFlowParams(blog_p, &flow_params);
    if (*err)
    {
        __logError("Failed to __L2L3ParseBlogFlowParams, err<%d>", *err);
        goto abort_activate;
    }

    *err = __mcastSetLkupKey(blog_p, &flow_params, &flow.key);
    if (*err)
    {
        __logError("Failed to __mcastSetLkupKey, err<%d>", *err);
        goto abort_activate;
    }

    *err = __mcastSetResult(blog_p, &flow_params, &flow.result);
    if(*err)
    {
        __logInfo("Could not __mcastSetResult");
        goto abort_activate;
    }
    *err = __mcastSetCmdList(blog_p, &flow.result, cmdlist_buffer_pp);
    if(*err)
    {
        __logInfo("Could not __mcastSetCmdList");
        goto abort_activate;
    }

#if defined(CONFIG_BCM_CMDLIST_SIM)
    {
        int skip_brcm_tag_len = 0;
        int tx_adjust;
        int cmd_list_data_length;

        if(PKTRUNNER_BRCM_TAG_MODE != CMDLIST_BRCM_TAG_NONE)
        {
            skip_brcm_tag_len = (pktrunner_is_rx_enet_wan_port(blog_p) && !__isTxWlanPhy(blog_p)) ? BRCM_TAG_TYPE2_LEN : 0;
        }

        flow_idx = runnerSim_activate(blog_p, flow.result_p.cmd_list, RDPA_CMD_LIST_UCAST_LIST_OFFSET,
                                     NULL, 0, skip_brcm_tag_len, cmd_list_data_length, tx_adjust);
    }
#else
    {
        *err = rdpa_mcast_flow_add(mcast_class, &flow_idx, &flow);
        if (*err)
        {
            __logInfo("Failed to add multicast %s flow (id<%d>), err<%d>\n",
                blog_p->mcast_client_id == BLOG_MCAST_MASTER_CLIENT_ID ? "master" : "client", blog_p->mcast_client_id, *err);
            goto abort_activate;
        }
    }
#endif /* CONFIG_BCM_CMDLIST_SIM */

    return (uint32_t)flow_idx;

abort_activate:
    if(*cmdlist_buffer_pp)
    {
        cmdlist_buffer_free(*cmdlist_buffer_pp);
    }
    return FHW_TUPLE_INVALID;
}


#if !defined(CONFIG_BCM_CMDLIST_SIM)
int runnerMcast_deactivate(Blog_t *blog_p, int rdpa_flow_key)
{
    int rc = 0;

    BCM_ASSERT(blog_p != BLOG_NULL);

    __logInfo("DEACTIVATE");

    rc = rdpa_mcast_flow_delete(mcast_class, rdpa_flow_key);
    __logDebug("delete flow, index<%d>, rc<%d>\n", rdpa_flow_key, rc);

    if (rc < 0)
    {
        __logError("Failed to remove flow key<0x%08x>, rc %d\n", rdpa_flow_key, rc);
        goto out;
    }

    __logInfo("DELETE: SUCCESSFUL <<<<");

out:
    return rc;
}

/*
 *------------------------------------------------------------------------------
 * Function   : runnerMcast_refresh
 * Description: This function is invoked to collect flow statistics
 * Parameters :
 *  tuple : 16bit index to refer to a Runner flow
 * Returns    : Total hits on this connection.
 *------------------------------------------------------------------------------
 */
int runnerMcast_refresh(int rdpa_key, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    rdpa_stat_t flow_stat;
    int rc;
    rc = rdpa_mcast_flow_stat_get(mcast_class, rdpa_key, &flow_stat);
    if (rc < 0)
    {
        __logError("Could not get rdpa_key<%d> stats, rc %d", rdpa_key, rc);
        return rc;
    }

    *pktsCnt_p = flow_stat.packets; /* cummulative packets */
    *octetsCnt_p = flow_stat.bytes;

  //  __logDebug( "rdpa_key<%03u> "
    //            "cumm_pkt_hits<%u> cumm_octet_hits<%u>\n",
      //          rdpa_key, *pktsCnt_p, *octetsCnt_p );
    return 0;
}

int runnerMcast_update(BlogUpdate_t update, uint32_t rdpa_flow_key, Blog_t *blog_p)
{
    int rc = -1;
    rdpa_fc_mcast_flow_t flow = {};

    __logDebug("rdpa_flow_key<%03u> update<%d>  blog_p<%px> fwd_and_trap<%d>\n",
        rdpa_flow_key, update, blog_p, blog_p->fwd_and_trap);

    rc = rdpa_mcast_flow_get(mcast_class, rdpa_flow_key, &flow);

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

    rc = rdpa_mcast_flow_set(mcast_class, rdpa_flow_key, &flow);
    if (rc)
    {
        __logError("Failed to update mcast flow, rdpa_flow_key<%03u>, "
            "update<%d> blog_p<%px> fwd_and_trap<%d>, rc<%d>\n",
            rdpa_flow_key, update, blog_p, blog_p->fwd_and_trap, rc);
    }

    return rc;
}

#else  /* CONFIG_BCM_CMDLIST_SIM */

int runnerMcast_deactivate(Blog_t *blog_p, int *isDeactivation_p)
{
    return 0;
}

int runnerMcast_refresh(int flowIdx, uint32_t *pktsCnt_p, uint32_t *octetsCnt_p)
{
    *pktsCnt_p = 1;
    *octetsCnt_p = 1;

    return 0;
}
#endif /* CONFIG_BCM_CMDLIST_SIM */


/*
*******************************************************************************
* Function   : runnerMcast_construct
* Description: Constructs the Runner Protocol layer
*******************************************************************************
*/
extern uint32_t mcast_key_exclude_fields;
int __init runnerMcast_construct(void *idx_p, void *disp_p)
{
#if !defined(CONFIG_BCM_CMDLIST_SIM)
    int ret;
    BDMF_MATTR(mcast_attrs, rdpa_mcast_drv());

    ret = rdpa_mcast_get(&mcast_class);
    if (ret)
    {
        ret = rdpa_mcast_flow_idx_pool_ptr_set(mcast_attrs, idx_p);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa mcast_class object cannot set rdpa_flow_idx_pool.\n");
            return ret;
        }
        ret = rdpa_mcast_flow_disp_pool_ptr_set(mcast_attrs, disp_p);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa mcast_class object cannot set flow_disp_pool_ptr.\n");
            return ret;
        }
        ret = bdmf_new_and_set(rdpa_mcast_drv(), NULL, mcast_attrs, &mcast_class);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa mcast class object does not exist and can't be created.\n");
            return ret;
        }
        mcast_class_created_here = 1;
#if defined(CONFIG_BCM963146) || defined(BCM63146) || defined(CONFIG_BCM94912) || defined(BCM4912) || defined(CONFIG_BCM96813) || defined(BCM6813)
        mcast_key_exclude_fields = rdpa_mcast_flow_key_exclude_pbit_field |
            rdpa_mcast_flow_key_exclude_dei_field;
#else
        /* exclude pbit, dei and ethernet type in natc lookup */
        mcast_key_exclude_fields = rdpa_mcast_flow_key_exclude_pbit_field |
            rdpa_mcast_flow_key_exclude_dei_field | rdpa_mcast_flow_key_exclude_etype_field;
#endif
        rdpa_mcast_key_exclude_fields_set(mcast_class, mcast_key_exclude_fields);
    }

    bcm_print("Initialized Runner Multicast Layer\n");
#endif

    return 0;
}

/*
*******************************************************************************
* Function   : runnerMcast_destruct
* Description: Destructs the Runner Protocol layer
* WARNING: __exit_refok suppresses warnings from CONFIG_DEBUG_SECTION_MISMATCH
*          This should only be called from __init or __exit functions.
*******************************************************************************
*/
void __ref runnerMcast_destruct(void)
{
#if !defined(CONFIG_BCM_CMDLIST_SIM)
    if (mcast_class)
    {
        /* Mcast flows will be removed by mcast object destroy */

        if (mcast_class_created_here)
        {
            bdmf_destroy(mcast_class);
            mcast_class_created_here = 0;
        }
        else
        {
            bdmf_put(mcast_class);
        }
    }
#endif
}
