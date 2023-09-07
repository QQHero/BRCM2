#ifndef __BPM_INLINE_H_INCLUDED__
#define __BPM_INLINE_H_INCLUDED__
#ifdef CONFIG_TENDA_PRIVATE_KM
#include <net/km_common.h>
#endif
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
 * File Name  : bpm_inline.h
 * Description: This header provides definition of all APIs that the pool
 *        manager needs to implement and register to GBPM framework.
 *        Some of them have a standard implementations.
 *******************************************************************************
 */

/* Common Pool Manager APIs for buffer pool */
/*
 * Function   : bpm_alloc_mult_buf_ex
 * Description: allocates a buffer from global buffer pool with
 *        reservation priority
 */
static int bpm_alloc_mult_buf_ex(uint32_t num, void **buf_p, uint32_t prio);

static int bpm_alloc_mult_buf(uint32_t num, void **buf_p)
{
    return bpm_alloc_mult_buf_ex(num, buf_p, BPM_LOW_PRIO_ALLOC);
}

/*
 * Function   : bpm_free_mult_buf
 * Description: frees a buffer to global buffer pool
 */
static void bpm_free_mult_buf(uint32_t num, void **buf_p);

/*
 * Function   : bpm_alloc_buf
 * Description: allocates a buffer from global buffer pool
 */
static void *bpm_alloc_buf(void);

/*
 * Function   : bpm_free_buf
 * Description: frees a buffer to global buffer pool
 */
static void bpm_free_buf(void *buf_p);

#if defined(CC_BPM_SKB_POOL_BUILD)
/* Common Pool Manager APIs for SKB Pool */
/*
 * Function   : bpm_total_skb
 * Description: return max number of skbs in pool
 */
static uint32_t bpm_total_skb(void);

/*
 * Function   : bpm_avail_skb
 * Description: return avail number of skbs in free pool
 */
static uint32_t bpm_avail_skb(void);

static void bpm_recycle_skb(void *skbv, unsigned long context,
                uint32_t recycle_action);

/*
 * An skb's scratchpad, namely control buffer and wlan control buffer may
 * be optionally cleared (by default or explicitly within a bypass datapath).
 */
#define SKB_CB_NOOP    (0)
#define SKB_CB_ZERO    (~SKB_CB_NOOP)

static inline void _skb_zero(struct sk_buff *skb, const uint32_t cb_op)
{
    skb->prev = NULL;
    skb->next = NULL;
    skb->priority = 0;
    skb->mark = 0;

    /* optionally, zero out skb::cb[] and skb:wl_cb[] */
    if (cb_op != SKB_CB_NOOP)
        skb_cb_zero(skb);
}

extern void skb_headerreset(struct sk_buff *skb);

/* All sk_buff(s) maintained in BPM are in a pristined state */
static inline void _bpm_pristine_skb(struct sk_buff *skb, uint32_t need_reset)
{
    if (unlikely(need_reset)) {
        skb_headerreset(skb); /* memset 0 and atomic set users = 1 */
        skb->recycle_hook = (RecycleFuncP)bpm_recycle_skb;
    }

    /* BPM SKB must only be attached to a BPM BUF or a BUF that obeys the
     * bcm_pkt_lengths.h layout (e.g. feed ring buffer in xrdp).
     */
    skb->recycle_flags = (SKB_RECYCLE | SKB_BPM_PRISTINE |
                  SKB_DATA_RECYCLE);
}

/*
 * Function   : bpm_attach_skb
 * Description: Attach a BPM allocated sk_buf to a BPM data buffer.
 *
 * CAUTION: The pointer buf is what was allocated using bpm_alloc_buf().
 * This is the starting address where a DMA engine will transfer data, and
 * will serve as the "skb->data".
 *
 * Reference skb_headerinit() in skbuff.c ...
 */
static void bpm_attach_skb( void *skbp, void *buf, uint32_t datalen)
{
    /* lock free helper routine */
    struct sk_buff *skb = (struct sk_buff *)skbp;
    struct skb_shared_info *skb_shinfo;

    /* skb_shared_info is ALWAYS aligned to end of the BPM buffer */
    skb_shinfo = BPM_BUF_TO_PSHINFO(buf);

    bcm_prefetch(skb_shinfo);

    /* Ensure skb is not already contaminated from alloc to attach */
    BPM_DBG_ASSERT((skb->recycle_flags & SKB_BPM_PRISTINE) == SKB_BPM_PRISTINE);
    BPM_DBG_ASSERT(skb_shinfo->dirty_p == NULL);

    /* All BPM allocated sk_buffs were in pristine state.  */

    skb->truesize = datalen + sizeof(struct sk_buff);

    skb->head = BPM_BUF_TO_PHEAD(buf);
    skb->data = buf;

    /* skb control buffer cb[] and wl_cb[] are not zeroed */
    _skb_zero(skb, SKB_CB_NOOP); /* prev, next, priority, mark zeroed */

    /* NET_SKBUFF_DATA_USES_OFFSET see skb_set_tail_pointer(skb, datalen) */
    skb->tail = BPM_BUF_TO_TAIL(buf, datalen);
    skb->end = BPM_BUF_TO_END(buf); /* do not relocate skb_shared_info */
    skb->len = datalen;

	memset(skb_shinfo, 0, sizeof(struct skb_shared_info));
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
    refcount_set(&skb->users, 1);
#else
    atomic_set(&skb->users, 1);
#endif
    atomic_set(&(skb_shinfo->dataref), 1);

#ifdef CONFIG_TENDA_PRIVATE_KM
        km__alloc_skb_hook(skbp);
#endif

} /* bpm_attach_skb() */

#else

static uint32_t bpm_total_skb(void)
{
    BPM_DBG_ASSERT(0);
    return 0U;
}

static uint32_t bpm_avail_skb(void)
{
    BPM_DBG_ASSERT(0);
    return 0U;
}

static void bpm_attach_skb(void *skb, void *data, uint32_t datalen)
{
    BPM_DBG_ASSERT(0);
}

static void *bpm_alloc_skb(void)
{
    return NULL;
}

static void *bpm_alloc_buf_skb_attach(uint32_t datalen)
{
    return NULL;
}

static void *bpm_alloc_mult_skb(uint32_t request_num)
{
    return NULL;
}

static void bpm_free_skb(void *skb)
{
    BPM_DBG_ASSERT(0);
}

static void bpm_free_skblist(void *head, void *tail, uint32_t len, void **bufp_arr)
{
    BPM_DBG_ASSERT(0);
}

static void *bpm_invalidate_dirtyp(void *skb)
{
    BPM_DBG_ASSERT(0);
}

static void bpm_recycle_skb(void *skbv, unsigned long context,
                uint32_t recycle_action)
{
    BPM_DBG_ASSERT(0);
}
#endif /* CC_BPM_SKB_POOL_BUILD */

/* Common Pool Manager APIs for pNBuff */
/*
 * Function : Recycle BPM based NBuff type buffer
 * CAUTION  : Only DATA recycle is supported
 *          No BPM LOCKS taken
 *  TODO    : Do cache invalidations on fkb_init() data buffer
 */
static void bpm_recycle_pNBuff(void *pNBuff, unsigned long context,
                   uint32_t recycle_action)
{
    if (IS_FKBUFF_PTR(pNBuff)) {
        FkBuff_t *fkb = PNBUFF_2_FKBUFF(pNBuff);

        /* Transmit driver is expected to perform cache invalidations */
        /* BPM expects data buffer ptr offseted by
         * FkBuff_t, BCM_PKT_HEADROOM */
        bpm_free_buf((void*)PFKBUFF_TO_PDATA(fkb, BCM_PKT_HEADROOM));
    } else {
        BCM_ASSERT(recycle_action == SKB_DATA_RECYCLE);
        bpm_recycle_skb(pNBuff, context, recycle_action);
    }
} /* bpm_recycle_pNBuff() */

/* Common Pool Manager APIs for Get accessors */
/*
 * function   : bpm_get_dyn_buf_lvl
 * description: finds the current dynamic buffer level is high or low.
 */
static int bpm_get_dyn_buf_lvl(void);

/*
 * function   : bpm_get_total_bufs
 * description: Return the total number of buffers managed by BPM
 */
static uint32_t bpm_get_total_bufs(void);

/*
 * function   : bpm_get_avail_bufs
 * description: Return the current number of free buffers in the BPM pool
 */
static uint32_t bpm_get_avail_bufs(void);

/*
 * function   : bpm_get_max_dyn_bufs
 * description: Return the current number of free buffers in the BPM pool
 */
static uint32_t bpm_get_max_dyn_bufs(void);


/* Common Pool Manager APIs for runtime buffer reservation update */
/*
 * function   : bpm_resv_rx_buf
 * description: reserves num of rxbufs and updates thresholds
 */
static int bpm_resv_rx_buf(gbpm_port_t port, uint32_t chnl,
               uint32_t num_rx_buf, uint32_t alloc_trig_thresh);

/*
 * function   : bpm_unresv_rx_buf
 * description: unreserves the previously reserved rx bufs
 */
static int bpm_unresv_rx_buf(gbpm_port_t port, uint32_t chnl);

#endif /*  __BPM_INLINE_H_INCLUDED__ */

