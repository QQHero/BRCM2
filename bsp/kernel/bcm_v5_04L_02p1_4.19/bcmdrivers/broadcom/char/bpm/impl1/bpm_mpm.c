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
* File Name  : bpm_mpm.c
*
* Description: MPM based Buffer Pool Manager
*
*******************************************************************************
*/

#include <linux/module.h>
#include <linux/bcm_log.h>
#include <linux/nbuff.h>
#include <linux/gbpm.h>
#include <linux/bcm_skb_defines.h>

#include "bcm_pkt_lengths.h"
#include "mpm.h"

#define CC_BPM_MPM_ERROR_CHECK
#define CC_BPM_MPM_LOCK

#define __STATIC__ static

// FIXME
#define BPM_MPM_TOTAL_SKB_FAKE        (1 << 14)
#define BPM_MPM_AVAIL_SKB_FAKE        (BPM_MPM_TOTAL_SKB_FAKE - 512)

#define BPM_MPM_ALLOC_RING_SIZE_LOG2  7 // 128
#define BPM_MPM_ALLOC_RING_SIZE       (1 << BPM_MPM_ALLOC_RING_SIZE_LOG2)
#define BPM_MPM_FREE_RING_SIZE_LOG2   7 // 128
#define BPM_MPM_FREE_RING_SIZE        (1 << BPM_MPM_FREE_RING_SIZE_LOG2)

#define BPM_MPM_RETRY_MAX             1000

#define isLogDebug bcmLog_logIsEnabled(BCM_LOG_ID_BPM, BCM_LOG_LEVEL_DEBUG)
#define __logDebug(fmt, arg...)   BCM_LOG_DEBUG(BCM_LOG_ID_BPM, fmt, ##arg)
#define __logInfo(fmt, arg...)    BCM_LOG_INFO(BCM_LOG_ID_BPM, fmt, ##arg)
#define __logNotice(fmt, arg...)  BCM_LOG_NOTICE(BCM_LOG_ID_BPM, fmt, ##arg)
#define __logError(fmt, arg...)   BCM_LOG_ERROR(BCM_LOG_ID_BPM, fmt, ##arg)

#if defined(CC_BPM_MPM_LOCK)
#define BPM_MPM_FLAGS_DEF()   unsigned long _flags
#define BPM_MPM_LOCK_IRQ()    spin_lock_irqsave(&bpm_mpm_g.lock, _flags)
#define BPM_MPM_UNLOCK_IRQ()  spin_unlock_irqrestore(&bpm_mpm_g.lock, _flags)
#else
#define BPM_MPM_FLAGS_DEF()
#define BPM_MPM_LOCK_IRQ()
#define BPM_MPM_UNLOCK_IRQ()
#endif

#define BPM_MPM_SKB_HEAD_TO_BUF(_head)  ( (uint8_t *)(_head) + BCM_PKT_HEADROOM )

typedef enum {
    BPM_MPM_FREE_TYPE_SINGLE=0,
    BPM_MPM_FREE_TYPE_LIST,
    BPM_MPM_FREE_TYPE_MAX
} bpm_mpm_free_type_t;

typedef struct {
    spinlock_t lock;
    mpm_ring_index_t alloc_ring_index[MPM_BUF_MODE_MAX];
    mpm_ring_index_t free_ring_index[BPM_MPM_FREE_TYPE_MAX];
} bpm_mpm_t;

static bpm_mpm_t bpm_mpm_g;

static inline int __bpm_mpm_alloc_mult_buf_ex(uint32_t num, void **buf_p, uint32_t prio)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.alloc_ring_index[MPM_BUF_MODE_PDATA];
    int retry_count = BPM_MPM_RETRY_MAX;
    int ret;
    BPM_MPM_FLAGS_DEF();

    // FIXME: Priority

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(num > BPM_MPM_ALLOC_RING_SIZE))
    {
        __logError("Too many buffers: %u, max %u", num, BPM_MPM_ALLOC_RING_SIZE);

        return GBPM_ERROR;
    }
#endif

    BPM_MPM_LOCK_IRQ();

    while((ret = mpm_alloc_pdata_array(ring_index, num, (uint8_t **)buf_p)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(ret))
    {
        __logError("Could not mpm_alloc_pdata_array");
    }
#endif

    return ret;
}

__STATIC__ int bpm_mpm_alloc_mult_buf_ex(uint32_t num, void **buf_p, uint32_t prio)
{
    return __bpm_mpm_alloc_mult_buf_ex(num, buf_p, prio);
}

__STATIC__ int bpm_mpm_alloc_mult_buf(uint32_t num, void **buf_p)
{
    // FIXME: Priority

    return __bpm_mpm_alloc_mult_buf_ex(num, buf_p, 0);
}

__STATIC__ void bpm_mpm_free_mult_buf(uint32_t num, void **buf_p)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.free_ring_index[BPM_MPM_FREE_TYPE_SINGLE];
    int retry_count = BPM_MPM_RETRY_MAX;
    int ret;
    BPM_MPM_FLAGS_DEF();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    BCM_ASSERT(num <= BPM_MPM_FREE_RING_SIZE);
#endif

    BPM_MPM_LOCK_IRQ();

    while((ret = mpm_free_pdata_array(ring_index, num, (uint8_t **)buf_p)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(ret))
    {
        __logError("Could not mpm_free_pdata_array");
    }
#endif
}

__STATIC__ void *bpm_mpm_alloc_buf(void)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.alloc_ring_index[MPM_BUF_MODE_PDATA];
    int retry_count = BPM_MPM_RETRY_MAX;
    void *pData;
    BPM_MPM_FLAGS_DEF();

    BPM_MPM_LOCK_IRQ();

    while(!(pData = mpm_alloc_pdata(ring_index)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(!pData))
    {
        __logError("Could not mpm_alloc_pData");
    }
#endif

    return pData;
}

static inline void __bpm_mpm_free_buf(void *buf_p)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.free_ring_index[BPM_MPM_FREE_TYPE_SINGLE];
    int retry_count = BPM_MPM_RETRY_MAX;
    int ret;
    BPM_MPM_FLAGS_DEF();

    BPM_MPM_LOCK_IRQ();

    while((ret = mpm_free_pdata(ring_index, buf_p)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(ret))
    {
        __logError("Could not mpm_free_pData");
    }
#endif
}

__STATIC__ void bpm_mpm_free_buf(void *buf_p)
{
    __bpm_mpm_free_buf(buf_p);
}

__STATIC__ uint32_t bpm_mpm_total_skb(void)
{
    // FIXME
    return BPM_MPM_TOTAL_SKB_FAKE;
}

__STATIC__ uint32_t bpm_mpm_avail_skb(void)
{
    // FIXME
    return BPM_MPM_AVAIL_SKB_FAKE;
}

static inline void __bpm_mpm_free_skb(void *skbp)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.free_ring_index[BPM_MPM_FREE_TYPE_SINGLE];
    int retry_count = BPM_MPM_RETRY_MAX;
    int ret;
    BPM_MPM_FLAGS_DEF();

    BPM_MPM_LOCK_IRQ();

    while((ret = mpm_free_skb_header(ring_index, skbp)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(ret))
    {
        __logError("Could not mpm_free_skb_header");
    }
#endif
}

static inline void __bpm_mpm_recycle_skb(void *skbp, unsigned long context,
                                       uint32_t recycle_action)
{
    if(SKB_DATA_RECYCLE & recycle_action)
    {
        struct sk_buff *skb = skbp;
        uint8_t *buf = BPM_MPM_SKB_HEAD_TO_BUF(skb->head);

        __bpm_mpm_free_buf(buf);
    }
    else if(SKB_RECYCLE & recycle_action)
    {
        __bpm_mpm_free_skb(skbp);
    }
}

__STATIC__ void bpm_mpm_recycle_skb(void *skbp, unsigned long context,
                                    uint32_t recycle_action)
{
    __bpm_mpm_recycle_skb(skbp, context, recycle_action);
}

__STATIC__ void bpm_mpm_attach_skb(void *skbp, void *buf, uint32_t datalen)
{
    struct sk_buff *skb = (struct sk_buff *)skbp;

    // FIXME: Change to mpm_skb_header_init() when MPM_BUF_MODE_PDATA_SHINFO is supported
    mpm_skb_header_and_shinfo_init(skb, buf, datalen, bpm_mpm_recycle_skb, 0, NULL);

    // SKB may come from a linked-list
    skb->next = NULL;
    skb->prev = NULL;
}

__STATIC__ void *bpm_mpm_alloc_skb(void)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.alloc_ring_index[MPM_BUF_MODE_SKB_HEADER];
    int retry_count = BPM_MPM_RETRY_MAX;
    struct sk_buff *skb;
    BPM_MPM_FLAGS_DEF();

    BPM_MPM_LOCK_IRQ();

    while(!(skb = mpm_alloc_skb_header(ring_index)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(!skb))
    {
        __logError("Could not mpm_alloc_skb_header");
    }
#endif

    return skb;
}

__STATIC__ void *bpm_mpm_alloc_buf_skb_attach(uint32_t datalen)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.alloc_ring_index[MPM_BUF_MODE_SKB_AND_DATA];
    int retry_count = BPM_MPM_RETRY_MAX;
    struct sk_buff *skb;
    BPM_MPM_FLAGS_DEF();

    BPM_MPM_LOCK_IRQ();

    while(!(skb = mpm_alloc_skb_and_data(ring_index)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

    if(likely(skb))
    {
        mpm_skb_and_data_init(skb, datalen, bpm_mpm_recycle_skb, 0, NULL);
    }
#if defined(CC_BPM_MPM_ERROR_CHECK)
    else
    {
        __logError("Could not mpm_alloc_skb_and_data");
    }
#endif

    return skb;
}

__STATIC__ void *bpm_mpm_alloc_mult_skb(uint32_t request_num)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.alloc_ring_index[MPM_BUF_MODE_SKB_HEADER_LIST];
    int retry_count = BPM_MPM_RETRY_MAX;
    struct sk_buff *head_skb;
    struct sk_buff *tail_skb;
    BPM_MPM_FLAGS_DEF();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(request_num > BPM_MPM_ALLOC_RING_SIZE))
    {
        __logError("Too many buffers: %u, max %u", request_num, BPM_MPM_ALLOC_RING_SIZE);

        return NULL;
    }
#endif

    BPM_MPM_LOCK_IRQ();

    while(!(head_skb = mpm_alloc_skb_header_list(ring_index, request_num, &tail_skb)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

    if(likely(head_skb))
    {
        head_skb->prev = NULL;
        tail_skb->next = NULL;
    }
#if defined(CC_BPM_MPM_ERROR_CHECK)
    else
    {
        __logError("Could not mpm_alloc_skb_header_list");
    }
#endif

    return head_skb;
}

__STATIC__ void bpm_mpm_free_skb(void *skbp)
{
    __bpm_mpm_free_skb(skbp);
}

__STATIC__ void bpm_mpm_free_mult_skb(void *head, void *tail, uint32_t len)
{
    mpm_ring_index_t ring_index = bpm_mpm_g.free_ring_index[BPM_MPM_FREE_TYPE_LIST];
    int retry_count = BPM_MPM_RETRY_MAX;
    int ret;
    BPM_MPM_FLAGS_DEF();

    BPM_MPM_LOCK_IRQ();

    while((ret = mpm_free_skb_header_list(ring_index, head)) && --retry_count);

    BPM_MPM_UNLOCK_IRQ();

#if defined(CC_BPM_MPM_ERROR_CHECK)
    if(unlikely(ret))
    {
        __logError("Could not mpm_free_skb_header_list");
    }
#endif
}

__STATIC__ void bpm_mpm_free_skblist(void *head, void *tail, uint32_t len, void **bufp_arr)
{
#if defined(CONFIG_BCM_BPM_BULK_FREE)
#if !defined(CONFIG_BCM_XRDP)
    bpm_mpm_free_mult_buf(len, bufp_arr);
#endif
    bpm_mpm_free_mult_skb(head, tail, len);
#else
    BPM_DBG_ASSERT(0);
#endif
}

__STATIC__ void *bpm_mpm_invalidate_dirtyp(void *p)
{
#if !defined(CONFIG_BCM_BPM_BULK_FREE)
    BPM_DBG_ASSERT(0);
    return NULL;
#else
    struct sk_buff *skb = p;
    uint8_t *buf = BPM_MPM_SKB_HEAD_TO_BUF(skb->head);

    return buf;
#endif /* CONFIG_BCM_BPM_BULK_FREE */
}

__STATIC__ void bpm_mpm_recycle_pNBuff(void *pNBuff, unsigned long context,
                                       uint32_t recycle_action)
{
    if(IS_FKBUFF_PTR(pNBuff))
    {
        FkBuff_t *fkb = PNBUFF_2_FKBUFF(pNBuff);

        __bpm_mpm_free_buf((void*)PFKBUFF_TO_PDATA(fkb, BCM_PKT_HEADROOM));
    }
    else
    {
#if defined(CC_BPM_MPM_ERROR_CHECK)
        BCM_ASSERT(recycle_action == SKB_DATA_RECYCLE);
#endif
        __bpm_mpm_recycle_skb(pNBuff, context, recycle_action);
    }
}

__STATIC__ int bpm_mpm_get_dyn_buf_lvl(void)
{
    // FIXME
    return 0;
}

__STATIC__ uint32_t bpm_mpm_get_total_bufs(void)
{
    // FIXME
    return BPM_MPM_TOTAL_SKB_FAKE;
}


__STATIC__ uint32_t bpm_mpm_get_avail_bufs(void)
{
    // FIXME
    return BPM_MPM_AVAIL_SKB_FAKE;
}

__STATIC__ uint32_t bpm_mpm_get_max_dyn_bufs(void)
{
    // FIXME
    return BPM_MPM_AVAIL_SKB_FAKE;
}

__STATIC__ int bpm_mpm_resv_rx_buf(gbpm_port_t port, uint32_t chnl, uint32_t num_rx_buf,
                                   uint32_t alloc_trig_thresh)
{
    // FIXME
    return GBPM_SUCCESS;
}

__STATIC__ int bpm_mpm_unresv_rx_buf(gbpm_port_t port, uint32_t chnl)
{
    // FIXME
    return GBPM_SUCCESS;
}

int __init bpm_pool_manager_init(void)
{
    bpm_mpm_free_type_t free_type;
    mpm_buf_mode_t buf_mode;
    uint32_t gbpm_debug = 1;
    int ret;

    bcmLog_setLogLevel(BCM_LOG_ID_BPM, BCM_LOG_LEVEL_ERROR);

    memset(&bpm_mpm_g, 0, sizeof(bpm_mpm_t));

    spin_lock_init(&bpm_mpm_g.lock);

    for(buf_mode=0; buf_mode<MPM_BUF_MODE_MAX; ++buf_mode)
    {
        if(MPM_BUF_MODE_PDATA == buf_mode ||
           MPM_BUF_MODE_SKB_HEADER == buf_mode ||
           MPM_BUF_MODE_SKB_HEADER_LIST == buf_mode ||
           MPM_BUF_MODE_SKB_AND_DATA == buf_mode)
        {
            ret = mpm_alloc_ring_alloc(buf_mode, BPM_MPM_ALLOC_RING_SIZE_LOG2,
                                       &bpm_mpm_g.alloc_ring_index[buf_mode]);
            if(ret)
            {
                return ret;
            }
        }
        else
        {
            bpm_mpm_g.alloc_ring_index[buf_mode] = -1;
        }
    }

    for(free_type=0; free_type<BPM_MPM_FREE_TYPE_MAX; ++free_type)
    {
        ret = mpm_free_ring_alloc(BPM_MPM_FREE_RING_SIZE_LOG2,
                                  &bpm_mpm_g.free_ring_index[free_type]);
        if(ret)
        {
            return ret;
        }
    }

#undef GBPM_DECL
#define GBPM_DECL(HOOKNAME)   bpm_mpm_ ## HOOKNAME,
    gbpm_bind(GBPM_BIND() gbpm_debug);

    bcm_print("MPM over BPM Initialized\n");

    return 0;
}
