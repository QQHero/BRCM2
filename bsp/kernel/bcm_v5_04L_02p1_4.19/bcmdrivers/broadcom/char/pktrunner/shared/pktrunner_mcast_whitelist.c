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
* File Name  : pktrunner_mcast_whitelist.c
*
* Description: This implementation supports the Runner Multicast Whitelist
*
*******************************************************************************
*/


#if defined(RDP_SIM)
#include "pktrunner_rdpa_sim.h"
#else
#include <linux/module.h>
#include <linux/if_ether.h>

#include <linux/bcm_log.h>
#include <linux/blog.h>
#include <net/ipv6.h>
#include "fcachehw.h"

#include "bcmtypes.h"
#endif

#include <rdpa_api.h>

#include "pktrunner_proto.h"
#include "pktrunner_common.h"
#include "pktrunner_mcast_whitelist.h"

#if !defined(RDP_SIM) && (!defined(CONFIG_BCM_PON_XRDP) || defined(CONFIG_MCAST_MULTI_FLOW_SUPPORT))
static bdmf_object_handle mcast_whitelist_class = NULL;

static void build_mcast_whitelist_key(Blog_t *blog_p, 
                                      rdpa_mcast_whitelist_t *mcast_wlist)
{
    uint16_t vlan_id;

#if defined(CC_PKTRUNNER_IPV6)
    if (blog_p->rx.info.bmap.PLD_IPv6)
    {
        mcast_wlist->mcast_group.l3.src_ip.family = bdmf_ip_family_ipv6;
        mcast_wlist->mcast_group.l3.gr_ip.family = bdmf_ip_family_ipv6;
        if (blog_p->is_ssm) 
        {
            /* Use source to lookup only for SSM flows */
            memcpy(mcast_wlist->mcast_group.l3.src_ip.addr.ipv6.data, blog_p->tupleV6.saddr.p8, 16); 
        }
        else
        {
            memset(mcast_wlist->mcast_group.l3.src_ip.addr.ipv6.data, 0, 16); 
        }
        memcpy(mcast_wlist->mcast_group.l3.gr_ip.addr.ipv6.data, blog_p->tupleV6.daddr.p8, 16);
    }
    else
#endif
    {
        mcast_wlist->mcast_group.l3.src_ip.family = bdmf_ip_family_ipv4;
        mcast_wlist->mcast_group.l3.gr_ip.family = bdmf_ip_family_ipv4;
        if (blog_p->is_ssm) 
        {
            /* Use source to lookup only for SSM flows */
            mcast_wlist->mcast_group.l3.src_ip.addr.ipv4 = ntohl(blog_p->rx.tuple.saddr);
        }
        else
        {
            mcast_wlist->mcast_group.l3.src_ip.addr.ipv4 = 0;
        }
        mcast_wlist->mcast_group.l3.gr_ip.addr.ipv4 = ntohl(blog_p->rx.tuple.daddr);
    }
    vlan_id = ntohl(blog_p->vtag[0]);
    mcast_wlist->vid = vlan_id & 0xFFF;
#if !defined(CONFIG_BCM_PON_XRDP) && !defined(CONFIG_MCAST_MULTI_FLOW_SUPPORT)
    mcast_wlist->num_vlan_tags = blog_p->vtag_num;
    vlan_id = ntohl(blog_p->vtag[1]);
    mcast_wlist->inner_vid = vlan_id & 0xFFF;
#endif
}

/*
*******************************************************************************
* Function   : runnerMcastWhitelist_add
* Description: adding the Mcast in the Whitelist
*******************************************************************************
*/
int runnerMcastWhitelist_add(Blog_t *blog_p, uint32_t *index)
{
    rdpa_mcast_whitelist_t mcast_wlist;
    int rc = 0;
    bdmf_index rdpa_index = FHW_TUPLE_INVALID;

    *index = FHW_TUPLE_INVALID;

    if (mcast_whitelist_class == NULL)
        return -1;

    bdmf_lock();

    memset(&mcast_wlist, 0, sizeof(rdpa_mcast_whitelist_t));
    build_mcast_whitelist_key(blog_p, &mcast_wlist);

    rc = rdpa_mcast_whitelist_entry_find(mcast_whitelist_class, &rdpa_index, &mcast_wlist);
    /* if entry has been added before, skip */
    if ((rc >= 0) && (rdpa_index != FHW_TUPLE_INVALID))
    {
        bdmf_unlock();
        *index = (uint32_t)rdpa_index;
        __logDebug("Entry exists already\n");
        return 1;
    }

    rc = rdpa_mcast_whitelist_entry_add(mcast_whitelist_class, &rdpa_index, &mcast_wlist);
    if (rc < 0)
    {
        bdmf_unlock();
        __logError("Could not rdpa_mcast_whitelist_entry_add, rc = %d", rc);
        return -1;
    }

    bdmf_unlock();
    *index = (uint32_t)rdpa_index;

    return 0;
}

/*
*******************************************************************************
* Function   : runnerMcastWhitelist_delete
* Description: deleting the Mcast in the Whitelist
*******************************************************************************
*/
int runnerMcastWhitelist_delete(uint32_t index)
{
    int rc = 0;
    bdmf_index rdpa_index = (bdmf_index)index;

    if (mcast_whitelist_class == NULL)
        return -1;

    bdmf_lock();

    rc = rdpa_mcast_whitelist_entry_delete(mcast_whitelist_class, rdpa_index);
    if (rc < 0)
    {
        __logError("Cannot rdpa_mcast_whitelist_entry_delete, rc = %d\n", rc);
    }

    /* TODO! if we do decide to trap everything when previously add
     * fails, then work/logic to disable the global trap-everyhing flag
     * has to be implemented here */

    bdmf_unlock();

    return rc;
}

/*
*******************************************************************************
* Function   : runnerMcastWhitelist_construct
* Description: Constructs the Runner Protocol layer
*******************************************************************************
*/
#if defined(XRDP)
int __init runnerMcastWhitelist_construct(void *p)
{
#if !defined(CONFIG_BCM_CMDLIST_SIM)
    int ret;
    BDMF_MATTR(mcast_whitelist_attrs, rdpa_mcast_whitelist_drv());

    ret = rdpa_mcast_whitelist_get(&mcast_whitelist_class);
    if (ret)
    {
        ret = bdmf_new_and_set(rdpa_mcast_whitelist_drv(), NULL, mcast_whitelist_attrs, &mcast_whitelist_class);
        if (ret)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "rdpa mcast_whitelist class object does not exist and can't be created.\n");
            return ret;
        }
    }

    bcm_print("Initialized Runner Multicast Layer\n");
#endif

    return 0;
}
#endif

/*
*******************************************************************************
* Function   : runnerMcastWhitelist_destruct
* Description: Destructs the Runner Protocol layer
* WARNING: __exit_refok suppresses warnings from CONFIG_DEBUG_SECTION_MISMATCH
*          This should only be called from __init or __exit functions.
*******************************************************************************
*/
#if defined(XRDP)
void __ref runnerMcastWhitelist_destruct(void)
{
#if !defined(CONFIG_BCM_CMDLIST_SIM)
    if (mcast_whitelist_class)
    {
        bdmf_destroy(mcast_whitelist_class);
    }
#endif
}
#endif

#ifdef XRDP

static int inline __pktRunner_reverse_get_index(uint32_t accel, uint32_t *pktRunner_idx, uint32_t rdpa_key)
{
    uint32_t idx;
    for (idx = 0; idx < PKTRUNNER_DATA(accel).max_flow_idxs; idx++)
    {
        /* TODO : Just matching the rdpa_key may not be sufficient because rpda_key
         * could be same for two different type of RDPA flows 
         * ex ucast/l2_ucast & mcast could have same rdpa_key */
        if (PKTRUNNER_RDPA_KEY(accel,idx) == rdpa_key)
        {
            *pktRunner_idx = idx;
            return 0;
        }
    }
    return -1;
}

int fhwPktRunnerMcastWhitelistActivate(Blog_t *blog_p, uint32_t key_in)
{
    uint32_t rdpa_key;
    int accel = PKTRUNNER_ACCEL_MCAST_WHITELIST;
    int rc, pktRunner_flow_idx;
    PktRunnerFlowKey_t pktRunner_Key = {.word = key_in};
    
    rc = runnerMcastWhitelist_add(blog_p, &rdpa_key);
    if (rc < 0)
    {
        __logInfo("Could not runnerMcastWhitelist_add");

        goto abort_activate;
    }
    else if ((rc > 0) && (rdpa_key != FHW_TUPLE_INVALID))
    {
        /* duplicate entry case */
        rc  = __pktRunner_reverse_get_index(accel, (uint32_t *)&pktRunner_flow_idx,
                                            rdpa_key);
        if (unlikely(rc))
            goto abort_activate;
    }
    else
    {
        pktRunner_flow_idx = idx_pool_get_index(&PKTRUNNER_DATA(accel).idx_pool);
        if (pktRunner_flow_idx < 0)
        {
            __logInfo("No free pkt runner indexes for accel <%d>", accel);
            PKTRUNNER_STATE(accel).no_free_idx++;
            runnerMcastWhitelist_delete(rdpa_key);
            goto abort_activate;
        }
        PKTRUNNER_STATE(accel).active++;
        PKTRUNNER_RDPA_KEY(accel, pktRunner_flow_idx) = rdpa_key;
        __pktRunnerFlowTypeSet(&PKTRUNNER_DATA(accel), pktRunner_flow_idx, PKTRNR_FLOW_TYPE_MC);
    }

    pktRunner_Key.word = __pktRunnerBuildKey(accel, 0, pktRunner_flow_idx);
    PKTRUNNER_STATE(accel).activates++;
    if (PKTRUNNER_DATA(accel).ref_cnt_p != NULL)
        PKTRUNNER_REF_CNT(accel, pktRunner_flow_idx)++;

    __logDebug("Added white list entry, key = %d, flowIdx = %d\n", rdpa_key, pktRunner_flow_idx);
    return pktRunner_Key.word;

abort_activate:

    PKTRUNNER_STATE(accel).failures++;
    __logInfo("cumm_failures<%u>", PKTRUNNER_STATE(accel).failures);

    return FHW_TUPLE_INVALID;
}

int fhwPktRunnerMcastWhitelistDeactivate(uint32_t pktRunner_Key, uint32_t *pktsCnt_p,
                                         uint32_t *octetsCnt_p, Blog_t *blog_p)
{
    uint32_t accel = __pktRunnerAccelIdx(pktRunner_Key);
    int flowIdx = __pktRunnerFlowIdx(pktRunner_Key);
    int rc;

    __logDebug("Removing white list entry, key = %d\n", flowIdx);
    if ((PKTRUNNER_DATA(accel).ref_cnt_p != NULL) &&
        ((--PKTRUNNER_REF_CNT(accel, flowIdx)) > 0))
        return 0;

    rc = runnerMcastWhitelist_delete(PKTRUNNER_RDPA_KEY(accel, flowIdx));
    if (rc)
    {
        PKTRUNNER_STATE(accel).failures++;
        return rc;
    }

    PKTRUNNER_STATE(accel).deactivates++;
    PKTRUNNER_STATE(accel).active--;

    if (idx_pool_return_index(&PKTRUNNER_DATA(accel).idx_pool, flowIdx) < 0)
    {
        __logInfo("Failed to free pkt runner index <%d:%d> for rdpa_key<%u>",
                  accel,flowIdx,PKTRUNNER_RDPA_KEY(accel, flowIdx));
        PKTRUNNER_STATE(accel).failures++;
    }
    else
    {
        PKTRUNNER_RDPA_KEY(accel, flowIdx) = FHW_TUPLE_INVALID;
        __pktRunnerFlowTypeSet(&PKTRUNNER_DATA(accel), flowIdx, PKTRNR_FLOW_TYPE_INV);
    }

    return rc;
}

#endif /* XRDP */

#else /* !defined(CONFIG_BCM_PON_XRDP) || defined(CONFIG_MCAST_MULTI_FLOW_SUPPORT) */

int runnerMcastWhitelist_construct(void *p)
{
    return 0;
}

void runnerMcastWhitelist_destruct(void)
{
}

int fhwPktRunnerMcastWhitelistDeactivate(uint32_t pktRunner_Key, uint32_t *pktsCnt_p,
                                         uint32_t *octetsCnt_p, Blog_t *blog_p)
{
    return FHW_TUPLE_INVALID;
}

int fhwPktRunnerMcastWhitelistActivate(Blog_t *blog_p, uint32_t key_in)
{
    return FHW_TUPLE_INVALID;
}

#endif /* !defined(CONFIG_BCM_PON_XRDP) || defined(CONFIG_MCAST_MULTI_FLOW_SUPPORT) */

