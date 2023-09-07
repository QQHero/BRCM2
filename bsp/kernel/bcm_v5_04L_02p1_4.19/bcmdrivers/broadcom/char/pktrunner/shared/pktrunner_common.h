
#ifndef __PKT_RUNNER_COMMON_H_INCLUDED__
#define __PKT_RUNNER_COMMON_H_INCLUDED__

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

#include <rdpa_api.h>
#include <rdpa_flow_idx_pool.h>

#if !defined(RDP_SIM)
#define CC_PKTRUNNER_PROCFS
#include <linux/seq_file.h>
#endif
#if defined(CONFIG_BLOG_IPV6)
#define CC_PKTRUNNER_IPV6
#endif

 
typedef struct {
    uint32_t status;        /* status: Enable=1 or Disable=0    */
    uint32_t mcast_mode;    /* mcast mode: provision=0, learning=1 */
    uint32_t activates;     /* number of activate (downcalls)   */
    uint32_t deactivates;   /* number of deactivate (downcalls) */
    uint32_t failures;      /* number of activate failures      */
    uint32_t l3_errors;     /* number of L3 flow errors         */
    uint32_t l2_errors;     /* number of L2 flow errors         */
    uint32_t mc_errors;     /* number of MC flow errors         */
    uint32_t hw_add_fail;   /* number of hask collision         */
    uint32_t no_free_idx;   /* number of no_free_idx errors     */
    uint32_t flushes;       /* number of clear (upcalls)        */
    uint32_t active;        /* number of active flows           */
    uint32_t ucast_active;  /* number of active unicast flows   */
    uint32_t mcast_active;  /* number of active mcast flows     */
    uint32_t max_flows;     /* Max allowed flows by HW(special/63132)*/
} pktRunner_state_t;

/* PktRunner Accelerator instance data */
typedef struct {
    uint32_t            max_flow_idxs;      /* Max number of flows supported by this accelerator */
    pktRunner_state_t   state;
    IdxPool_t           idx_pool;           /* pktRunner index pool */
    uint32_t            *rdpa_flow_key_p;   /* mapped RDPA Flow ID (opaque 32bit value) */
    int                 *ref_cnt_p;         /* ref_cnt per index */
    bdmf_number         *flowResetStats_p;  /* Flow Reset Stats */
    void*               *cmdlist_buffer_pointers;   /* used by L2 and L3-Ucast for command list overflow */
    uint8_t             *flow_type_p;       /* Pointer to array holding flow-type information per flow */
} pktRunner_data_t;

typedef union {
    struct {
        BE_DECL(
            uint32_t unused:6;
            uint32_t accel:2;
            uint32_t flow_type:4;
            uint32_t flow_idx:20;
        )
        LE_DECL(
            uint32_t flow_idx:20;
            uint32_t flow_type:4;
            uint32_t accel:2;
            uint32_t unused:6;
        )
    };
    uint32_t word;
} PktRunnerFlowKey_t;

#define PKTRUNNER_ACCEL_FLOW    (0)
#ifdef XRDP
#define PKTRNR_MAX_FHW_ACCEL    (2)  /* Max number of HW accelerators towards FHW */
#define PKTRUNNER_ACCEL_MCAST_WHITELIST (1)
#else
#define PKTRNR_MAX_FHW_ACCEL    (1)  /* Max number of HW accelerators towards FHW */
#endif

extern pktRunner_data_t  pktRunner_data_g[PKTRNR_MAX_FHW_ACCEL];    /* Protocol layer global context */

#define PKTRUNNER_DATA(accel)                   (pktRunner_data_g[accel])
#define PKTRUNNER_STATE(accel)                  (PKTRUNNER_DATA(accel).state)
#define PKTRUNNER_STATS(accel, idx)             (PKTRUNNER_DATA(accel).flowResetStats_p[idx])
#define PKTRUNNER_RDPA_KEY(accel, idx)          (PKTRUNNER_DATA(accel).rdpa_flow_key_p[idx])
#define PKTRUNNER_CMDLIST_PTR(accel, idx)       (PKTRUNNER_DATA(accel).cmdlist_buffer_pointers[idx])
#define PKTRUNNER_REF_CNT(accel, idx)           (PKTRUNNER_DATA(accel).ref_cnt_p[idx])

typedef enum {
    PKTRNR_FLOW_TYPE_INV    = 0,
    PKTRNR_FLOW_TYPE_L3     = 1,
    PKTRNR_FLOW_TYPE_L2     = 2,
    PKTRNR_FLOW_TYPE_MC     = 3,
    PKTRNR_FLOW_TYPE_MAX    = PKTRNR_FLOW_TYPE_MC,
} e_PKTRNR_FLOW_TYPE;

static inline int __pktRunnerFlowIdx(uint32_t pktRunner_Key)
{
    return ((PktRunnerFlowKey_t*)&pktRunner_Key)->flow_idx;
}
static inline int __pktRunnerFlowType(uint32_t pktRunner_Key)
{
    return ((PktRunnerFlowKey_t*)&pktRunner_Key)->flow_type;
}
static inline int __pktRunnerAccelIdx(uint32_t pktRunner_Key)
{
    return ((PktRunnerFlowKey_t *)&pktRunner_Key)->accel;
}

static inline uint32_t __pktRunnerBuildKey(uint32_t accel, uint32_t flow_type, uint32_t flow_idx)
{
    PktRunnerFlowKey_t key = {.word = 0 };
    key.accel = accel;
    key.flow_type = flow_type;
    key.flow_idx = flow_idx;

    return key.word;
}

/* flow_type array accessor functions */

/* Using macro to avoid include files */
#define __pktRunnerFlowTypeInit(p)                                                  \
{                                                                                   \
    if (!p->flow_type_p)                                                            \
    {                                                                               \
        uint32_t array_size = p->max_flow_idxs/4 + ( (p->max_flow_idxs%4)? 1 : 0 ); \
        p->flow_type_p = kmalloc(array_size, GFP_KERNEL);                           \
    }                                                                               \
}

static inline void __pktRunnerFlowTypeSet(pktRunner_data_t *p, uint32_t flow_idx, e_PKTRNR_FLOW_TYPE type)
{
    uint32_t byte_num, bit_pos;
    if (p->flow_type_p && p->max_flow_idxs > flow_idx)
    {
        byte_num = flow_idx/4;
        bit_pos = (flow_idx%4)*2;
        p->flow_type_p[byte_num] &= ~( 0x3 << bit_pos ); /* reset the bits */
        p->flow_type_p[byte_num] |= ( (type & 0x3) << bit_pos ); /* set the bits */
    }
}
static inline e_PKTRNR_FLOW_TYPE __pktRunnerFlowTypeGet(pktRunner_data_t *p, uint32_t flow_idx)
{
    uint32_t byte_num, bit_pos;
    e_PKTRNR_FLOW_TYPE value = PKTRNR_FLOW_TYPE_INV;

    if (p->flow_type_p && p->max_flow_idxs > flow_idx)
    {
        byte_num = flow_idx/4;
        bit_pos = (flow_idx%4)*2;
        value = ( p->flow_type_p[byte_num] & ( 0x3 << bit_pos ) ) >> bit_pos ; 
    }
    return value;
}

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)  (sizeof(a)/sizeof(a[0]))
#endif

#define isLogDebug bcmLog_logIsEnabled(BCM_LOG_ID_PKTRUNNER, BCM_LOG_LEVEL_DEBUG)
#define __logDebug(fmt, arg...)   BCM_LOG_DEBUG(BCM_LOG_ID_PKTRUNNER, fmt, ##arg)
#define __logInfo(fmt, arg...)    BCM_LOG_INFO(BCM_LOG_ID_PKTRUNNER, fmt, ##arg)
#define __logNotice(fmt, arg...)  BCM_LOG_NOTICE(BCM_LOG_ID_PKTRUNNER, fmt, ##arg)
#define __logError(fmt, arg...)   BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, fmt, ##arg)

int rx_if_get_by_blog(Blog_t *blog_p, int *is_wan, rdpa_if *rx_if);

void pktrunner_build_wlan_result(Blog_t *blog_p, rdpa_ip_flow_result_t *ip_flow_result);
int pktrunner_blog_parse_mcast_group(Blog_t *blog_p, int is_l2, int include_vid, rdpa_iptv_channel_key_t *key);
int pktrunner_build_mcast_lkp_key(Blog_t *blog_p, rdpa_fc_mcast_flow_key_t *key);
int mcast_flow_master_ctx_set_clients_vector(Blog_t *blog_p, rdpa_ip_flow_result_t *result);
int blog_hdr_get_vtags_from_encap(BlogHeader_t *bHdr_p, uint16_t *etype, uint32_t *vtag0, uint32_t *vtag1, uint32_t is_mcast);
#if defined(POLICER_SUPPORT)
void blog_parse_policer_get(Blog_t *blog_p, bdmf_object_handle *policer);
#endif

#if defined(CC_PKTRUNNER_PROCFS)
uint32_t pktRunnerGetState(struct seq_file *, uint32_t accel);
#endif

int __init _pktRunnerAccelInit(uint32_t accel, uint32_t num_flows, uint32_t cmd_list_ovrflw, uint32_t ref_count,
    uint32_t mcast_mode);
int __exit _pktRunnerAccelExit(uint32_t accel);

int pktrunner_is_enet_wan_port(Blog_t *blog_p, int is_rx);
#define pktrunner_is_rx_enet_wan_port(b) pktrunner_is_enet_wan_port(b, 1)
#define pktrunner_is_tx_enet_wan_port(b) pktrunner_is_enet_wan_port(b, 0)

int pktrunner_is_enet_ext_sw_port(Blog_t *blog_p, int is_rx);
#define pktrunner_is_rx_enet_ext_sw_port(b) pktrunner_is_enet_ext_sw_port(b, 1)
#define pktrunner_is_tx_enet_ext_sw_port(b) pktrunner_is_enet_ext_sw_port(b, 0)

extern int isRdpaGbeWanConfigured;

int pktrunner_udpspdt_tx_start(Blog_t *blog_p);

#endif  /* defined(__PKT_RUNNER_COMMON_H_INCLUDED__) */

