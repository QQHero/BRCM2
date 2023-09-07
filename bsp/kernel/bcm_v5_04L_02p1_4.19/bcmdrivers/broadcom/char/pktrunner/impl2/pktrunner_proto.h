#ifndef __PKT_RUNNER_PROTO_H_INCLUDED__
#define __PKT_RUNNER_PROTO_H_INCLUDED__

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


#include "pktrunner_common.h"

#if (defined(CONFIG_BCM_RDPA_MCAST) || defined(RDP_SIM) || defined(CONFIG_BCM963158) || \
    defined(CONFIG_BCM963146) || defined(CONFIG_BCM94912) || defined(CONFIG_BCM96813) || defined(CONFIG_BCM963138) || \
    defined(CONFIG_BCM963148) || defined(CONFIG_BCM94908) || defined(CONFIG_BCM96855)) && \
    !defined(WL4908_EAP)
#define CC_PKTRUNNER_MCAST
#endif

#if defined(BCM63146) || defined(BCM4912) || defined(BCM6813) || defined(BCM6855) || defined(BCM63158_CMDLIST_XPE_SIM)
#define PKTRUNNER_BRCM_TAG_MODE  CMDLIST_BRCM_TAG_NONE
#else
#define PKTRUNNER_BRCM_TAG_MODE  CMDLIST_BRCM_TAG_RX_TX
#endif

#define TUNNEL_KEY_OFFSET_INVALID    0xFF
#define RX_GRE_KEY_ENABLED(blog_p)   (RX_GRE(blog_p) && (blog_p->grerx.gre_flags.u16 & BLOG_GRE_FLAGS_KEY_ENABLE))
#define GRE_TERMINATION(blog_p)      (RX_GRE(blog_p) && !((blog_p->rx.info.bmap.PLD_L2 == 1) && RX_GRE(blog_p) && TX_GRE(blog_p)))
/*******************************************************************************
 *
 * Debugging
 *
 *******************************************************************************/

#if !defined(__debug)
#define __debug(fmt, arg...)                    \
    BCM_LOGCODE(                                \
        if(isLogDebug)                          \
            bcm_print(fmt, ##arg); )
#endif

#define __dumpCmdList(_cmdList)                                         \
    BCM_LOGCODE(                                                        \
        if(isLogDebug)                                                  \
            cmdlist_dump((_cmdList), RDPA_CMD_LIST_UCAST_LIST_SIZE_32); )

#define __dumpPartialCmdList()                  \
    BCM_LOGCODE(                                \
        if(isLogDebug)                          \
        {                                       \
            cmdlist_dump_partial();             \
            bcm_print("\n");                    \
        } )

#define __dumpBlog(_blog_p)                     \
    BCM_LOGCODE(                                \
        if(isLogDebug)                          \
        {                                       \
            blog_dump((_blog_p));               \
            bcm_print("\n");                    \
        } )

#define __dumpBlogRule(_blogRule_p)             \
    BCM_LOGCODE(                                \
        if(isLogDebug)                          \
        {                                       \
            blog_rule_dump((_blogRule_p));      \
            bcm_print("\n");                    \
        } )

/*******************************************************************************
 *
 * Functions
 *
 *******************************************************************************/

#ifndef BRCM_TAG_TYPE2_LEN
#define BRCM_TAG_TYPE2_LEN  4
#endif

#ifndef CONFIG_BCM_RUNNER_MAX_FLOWS
#define PKTRUNNER_MAX_L2L3_FLOWS    (16384)
#else
#define PKTRUNNER_MAX_L2L3_FLOWS    (CONFIG_BCM_RUNNER_MAX_FLOWS) 
#endif

#ifdef XRDP
#define PKTRUNNER_MAX_MCAST_FLOWS   1024
#else
#define PKTRUNNER_MAX_MCAST_FLOWS   128
#endif

typedef struct {
    struct {
        rdpa_traffic_dir dir;       /**< Traffic direction */
        rdpa_if ingress_if;         /**< Ingress interface */
        uint16_t wan_flow;          /**< WAN Flow, used f ingress port is wan (e.g. gem_flow), ignored otherwise \XRDP_LIMITED */
    } key;
    rdpa_ip_flow_result_t result;
} flowParams_t;


rdpa_mcast_flow_t *__mcastFlowMalloc(void);
void __mcastFlowFree(rdpa_mcast_flow_t *mcastFlow_p);
uint32_t __enetLogicalPortToPhysicalPort(uint32_t logicalPort);
uint32_t __skbMarkToQueuePriority(uint32_t skbMark);
uint32_t __skbMarkToTrafficClass(uint32_t skbMark);
int __lagPortGet(Blog_t *blog_p);
int __isEnetBondedLanWanPort(uint32_t logicalPort);
int __isWlanPhy(Blog_t *blog_p);
int __isTxWlanPhy(Blog_t *blog_p);
int __isTxEthPhy(Blog_t *blog_p);
int __ucastSetFwdAndFilters(Blog_t *blog_p, rdpa_ip_flow_info_t *ip_flow_p);
int __l2ucastSetFwdAndFilters(Blog_t *blog_p, rdpa_l2_flow_info_t *l2_flow_p);
void __set_dpiqos_flow_results(rdpa_ip_flow_result_t *result, Blog_t *blog_p);
void __set_wlan_qos_flow_results(rdpa_ip_flow_result_t *result, Blog_t *blog_p);
int ucastSetResult(Blog_t *blog_p, flowParams_t *flow_params_p, rdpa_ip_flow_result_t *result_p);
int L2L3ParseBlogFlowParams(Blog_t *blog_p, flowParams_t *params_p);

int __init runnerProto_construct(void);
void __exit runnerProto_destruct(void);

#ifdef RDP_UFC_TUNNEL
#define CAM_LKP_EMPTY_ENTRY              0xFFFFFFFF
#define MAX_NUM_OF_KEYS_PER_TUNNEL_FLOW  4
#define TUNNEL_EMPTY_FIELD               0xFFFFFFFF
#define TUNNEL_MAX_NUMBER                16
#define TUNNEL_INVALID_NUMBER            TUNNEL_MAX_NUMBER
#define TUNNEL_FLOW_CONTEXT_SIZE         40

typedef struct {
    bdmf_index index;
    uint32_t key;
    uint32_t ref_num;
} tunnel_info;

void tunnel_table_init(void);
int tunnel_entry_update(bdmf_index flow_index, uint32_t key, uint8_t *index);
int tunnel_entry_remove(uint8_t index, bdmf_index *flow_index, uint32_t *key);
int tunnel_construct_flow(Blog_t *blog_p, rdpa_ip_flow_info_t *ip_flow_tunnel, uint8_t *tunnel_index_ref);
int tunnel_destruct_flow(uint8_t tunnel_index_ref, uint32_t rdpa_flow_key);
#endif

#endif  /* defined(__PKT_RUNNER_PROTO_H_INCLUDED__) */
