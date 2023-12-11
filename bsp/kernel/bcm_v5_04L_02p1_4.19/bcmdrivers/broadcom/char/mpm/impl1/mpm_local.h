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

/*
*******************************************************************************
*
* File Name  : mpm_local.h
*
* Description: Broadcom Memory Pool Manager (MPM) Local Header File
*
*******************************************************************************
*/

#ifndef __MPM_LOCAL_H_INCLUDED__
#define __MPM_LOCAL_H_INCLUDED__

#include <linux/bcm_log.h>
#include "mpm_driver.h"

#if !IS_ENABLED(CONFIG_BCM_ARCHER)
#define CC_MPM_RUNNER
#endif

#define CC_MPM_SKB_TAIL_END_INIT
#define CC_MPM_DATA_SHINFO_INIT

#define isLogDebug bcmLog_logIsEnabled(BCM_LOG_ID_MPM, BCM_LOG_LEVEL_DEBUG)
#define __logDebug(fmt, arg...)   BCM_LOG_DEBUG(BCM_LOG_ID_MPM, fmt, ##arg)
#define __logInfo(fmt, arg...)    BCM_LOG_INFO(BCM_LOG_ID_MPM, fmt, ##arg)
#define __logNotice(fmt, arg...)  BCM_LOG_NOTICE(BCM_LOG_ID_MPM, fmt, ##arg)
#define __logError(fmt, arg...)   BCM_LOG_ERROR(BCM_LOG_ID_MPM, fmt, ##arg)

#define __debug(fmt, arg...)                    \
    BCM_LOGCODE(                                \
        if(isLogDebug)                          \
            bcm_print(fmt, ##arg); )

#define MPM_MEMBER_SIZEOF(_type, _member)       \
    (sizeof(((_type *)(NULL))->_member))

#if ((defined(CONFIG_BCM94912) && (CONFIG_BRCM_CHIP_REV == 0x4912A0)) || \
     defined(CONFIG_BCM963146))
#define MPM_EBUF_TOTAL_MAX_SHIFT        17 /* 128K EBUFs */
#elif defined(CONFIG_BCM96756) || defined(CONFIG_BCM94912) || defined(CONFIG_BCM96813)
#define MPM_EBUF_TOTAL_MAX_SHIFT        18 /* 256K EBUFs */
#else
#error "Unknown SoC"
#endif

#define MPM_EBUF_TOTAL_MAX              (1 << MPM_EBUF_TOTAL_MAX_SHIFT)
#define MPM_NBR_OF_POOLS                4

#if ((defined(CONFIG_BCM94912) && (CONFIG_BRCM_CHIP_REV == 0x4912A0)) || \
     defined(CONFIG_BCM96756))
#define MPM_BUF_SIZE_IMPL               1
#else /* if defined(CONFIG_BCM94912) || defined(CONFIG_BCM963146) */
#define MPM_BUF_SIZE_IMPL               2
#endif

#if (MPM_BUF_SIZE_IMPL == 1)
#define MPM_BUF_SIZE_DISABLED           0
#define MPM_BUF_SIZE_1                  1
#define MPM_BUF_SIZE_2                  2
#define MPM_BUF_SIZE_3                  3
#define MPM_BUF_SIZE_4                  4
#define MPM_BUF_SIZE_5                  5
#define MPM_BUF_SIZE_8                  6
#define MPM_BUF_SIZE_10                 7
#define MPM_BUF_SIZE_20                 8
#define MPM_BUF_SIZE_40                 9
#define MPM_BUF_SIZE_MAX                10
#else /* if (MPM_BUF_SIZE_IMPL == 2) */
#define MPM_BUF_SIZE_DISABLED           0
#define MPM_BUF_SIZE_1                  1
#define MPM_BUF_SIZE_2                  2
#define MPM_BUF_SIZE_3                  3
#define MPM_BUF_SIZE_4                  4
#define MPM_BUF_SIZE_5                  5
#define MPM_BUF_SIZE_6                  6
#define MPM_BUF_SIZE_7                  7
#define MPM_BUF_SIZE_8                  8
#define MPM_BUF_SIZE_10                 10
#define MPM_BUF_SIZE_20                 20
#define MPM_BUF_SIZE_MAX                20
#endif

#define MPM_SKB_SIZE                    mpm_skb_size()
#define MPM_SKB_USERS_OFFSET            mpm_skb_users_offset()
#define MPM_SKB_USERS_SIZE              mpm_skb_users_size()
#define MPM_SKB_NEXT_OFFSET             mpm_skb_next_offset()
#define MPM_SKB_PREV_OFFSET             mpm_skb_prev_offset()
#define MPM_SKB_HEAD_OFFSET             mpm_skb_head_offset()
#define MPM_SKB_DATA_OFFSET             mpm_skb_data_offset()
#define MPM_SKB_TAIL_OFFSET             mpm_skb_tail_offset()
#define MPM_SKB_END_OFFSET              mpm_skb_end_offset()
#define MPM_SKB_DATA_SIZE               mpm_skb_data_size()

#define MPM_FKB_SIZE                    sizeof(struct fkbuff)
#define MPM_FKB_DATA_OFFSET             offsetof(struct fkbuff, data)
#define MPM_FKB_NEXT_OFFSET             offsetof(struct fkbuff, list)
#define MPM_FKB_USERS_OFFSET            offsetof(struct fkbuff, users)
#define MPM_FKB_USERS_SIZE              MPM_MEMBER_SIZEOF(struct fkbuff, users)

#define MPM_BCM_PKT_HEADROOM            mpm_bcm_pkt_headroom()
#define MPM_BCM_MAX_PKT_LEN             mpm_bcm_max_pkt_len()
#define MPM_BCM_SKB_TAILROOM            mpm_bcm_skb_tailroom()
#define MPM_BCM_SKB_SHAREDINFO          mpm_bcm_skb_sharedinfo()

#define MPM_DATA_HEAD_OFFSET            BCM_FKB_INPLACE
#define MPM_DATA_PDATA_OFFSET           (MPM_DATA_HEAD_OFFSET + MPM_BCM_PKT_HEADROOM)
#define MPM_DATA_TAIL_OFFSET            (MPM_DATA_PDATA_OFFSET + MPM_BCM_MAX_PKT_LEN)
#define MPM_DATA_END_OFFSET             (MPM_DATA_TAIL_OFFSET + MPM_BCM_SKB_TAILROOM - MPM_DATA_HEAD_OFFSET)
#define MPM_DATA_SHINFO_OFFSET          (MPM_DATA_END_OFFSET + MPM_DATA_HEAD_OFFSET)
#define MPM_DATA_SHINFO_SIZE            MPM_BCM_SKB_SHAREDINFO
#define MPM_DATA_SHINFO_DATAREF_OFFSET  mpm_data_shinfo_dataref_offset()
#define MPM_DATA_SHINFO_DATAREF_SIZE    mpm_data_shinfo_dataref_size()

typedef struct {
    volatile void *host_unaligned_p; /* Unalligned host address */
    volatile void *host_p;           /* Aligned host address */
    dma_addr_t phys_unaligned_addr;  /* Unaligned physical address */
    dma_addr_t phys_addr;            /* Aligned physical address */
    int size;
} mpm_mem_t;

typedef struct {
    uintptr_t phys_base_addr;
    uintptr_t virt_base_addr;
    int total_mem_size; // Bytes
    int ebuf_size;      // Bytes
    int ebuf_total;     // Total number of EBUFs
    int ebuf_total_configured; // Configured total number of EBUFs
    int pool_size[MPM_NBR_OF_POOLS]; // EBUFs
    int skb_pool_nbr;
    int data_pool_nbr;
#if defined(CC_MPM_RUNNER)
    int dqm_pool_nbr;
    int runner_xon_thld;
    int runner_xoff_thld;
#endif
} mpm_config_t;

int mpm_driver_coherent_mem_alloc(mpm_mem_t *mem_p, int size, const char *name);

void mpm_driver_coherent_mem_free(mpm_mem_t *mem_p);

int mpm_init(mpm_config_t *config_p);

void mpm_stats_dump(void);

void mpm_reg_dump(void);

int mpm_skb_size(void);

int mpm_skb_users_offset(void);

int mpm_skb_users_size(void);

int mpm_skb_next_offset(void);

int mpm_skb_prev_offset(void);

int mpm_skb_head_offset(void);

int mpm_skb_data_offset(void);

int mpm_skb_tail_offset(void);

int mpm_skb_end_offset(void);

int mpm_skb_data_size(void);

int mpm_bcm_pkt_headroom(void);

int mpm_bcm_max_pkt_len(void);

int mpm_bcm_skb_tailroom(void);

int mpm_bcm_skb_sharedinfo(void);

int mpm_data_shinfo_dataref_offset(void);

int mpm_data_shinfo_dataref_size(void);

int mpm_ut_run(mpm_test_t test, void *virt_base, unsigned int mem_size);

#endif  /* __MPM_LOCAL_H_INCLUDED__ */
