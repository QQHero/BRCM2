/*
  <:copyright-BRCM:2019:proprietary:standard

  Copyright (c) 2019 Broadcom 
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
* File Name  : archer_wlan.c
*
* Description: Archer WLAN Interface
*
*******************************************************************************
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/nbuff.h>
#include <linux/bcm_log.h>
#include <linux/blog.h>
#include <linux/bcm_skb_defines.h>
#include <linux/gbpm.h>
#include "bcm_timer.h"
#include "bpm.h"
#include "bcm_prefetch.h"

#include "sysport_rsb.h"
#include "sysport_classifier.h"

#include "archer.h"
#include "archer_driver.h"
#include "archer_thread.h"
#include "archer_socket.h"
#include "archer_wlan.h"
#include "bcm_async_queue.h"

#define CC_ARCHER_WLAN_XFER_THREAD

// FIXME: Remove this
#if !defined(BCM_PKTFWD)
#define BCM_PKTFWD
#define CC_ARCHER_WLAN_STANDALONE
#endif
#if !defined(CONFIG_BCM_WLAN_16BIT_STATION_CHAIN_IDX_SUPPORT)
#define CONFIG_BCM_WLAN_16BIT_STATION_CHAIN_IDX_SUPPORT
#endif

#include "bcm_pktfwd.h" /* BCM_PKTFWD && BCM_PKTLIST */

#include "bcm_archer.h"

#if defined(CC_ARCHER_WLAN_STANDALONE)
pktlist_context_t *pktlist_context_fini(pktlist_context_t * pktlist_context) { return NULL; }
void pktlist_context_dump(pktlist_context_t * pktlist_context, bool dump_peer, bool dump_verbose) { }
pktlist_context_t *pktlist_context_init(
    pktlist_context_t * pktlist_context_peer,
    pktlist_context_xfer_fn_t pktlist_context_xfer_fn,
    pktlist_context_keymap_fn_t pktlist_context_keymap_fn,
    void * driver, const char * driver_name, uint32_t unit) { return NULL; }
#endif

//#define CC_ARCHER_WLAN_DEBUG
#define CC_ARCHER_WLAN_STATS

#if defined(CC_ARCHER_WLAN_STATS)
#define ARCHER_WLAN_STATS_UPDATE(_counter) ( (_counter)++ )
#else
#define ARCHER_WLAN_STATS_UPDATE(_counter)
#endif

#define ARCHER_WLAN_FLUSH_THRESHOLD     32 // packets
#define ARCHER_WLAN_FLUSH_TIMEOUT_USEC  500
#define ARCHER_WLAN_FLUSH_JIFFIES       ( ARCHER_WLAN_FLUSH_TIMEOUT_USEC / BCM_TIMER_PERIOD_uSEC )
#if(!ARCHER_WLAN_FLUSH_JIFFIES)
#error "!ARCHER_WLAN_FLUSH_JIFFIES"
#endif

#define ARCHER_WLAN_FLCTL_PKT_PRIO_FAVOR         4  /* Favor Pkt Prio >= 4 : VI,VO */
#define ARCHER_WLAN_FLCTL_SKB_EXHAUSTION_LO_PCNT 25 /* Favored Pkt threshold */
#define ARCHER_WLAN_FLCTL_SKB_EXHAUSTION_HI_PCNT 10
#define ARCHER_WLAN_FLCTL_DROP_CREDITS           32

#define ARCHER_WLAN_SKB_CACHE_SIZE           64

#define ARCHER_WLAN_RADIO_MAX                SYSPORT_FLOW_WLAN_PORTS_MAX

#define ARCHER_WLAN_SOCKET_TX_QUEUE_SIZE     256
#define ARCHER_WLAN_SOCKET_MISS_QUEUE_SIZE   ( ARCHER_WLAN_SOCKET_TX_QUEUE_SIZE * 2 )
#define ARCHER_WLAN_XFER_QUEUE_SIZE          1024

typedef struct {
    struct sk_buff *skb_p;
} archer_wlan_socket_tx_queue_t;

typedef struct {
    struct sk_buff *skb_p;
} archer_wlan_socket_miss_queue_t;

typedef struct {
    spinlock_t lock;
    bcm_async_queue_t queue;
} archer_wlan_socket_tx_t;

typedef struct {
    archer_wlan_rx_miss_handler_t handler;
    void *context;
    pktlist_t pktl;             /* lockless usage */
} archer_wlan_rx_miss_radio_t;

typedef struct {
    archer_wlan_rx_miss_radio_t radio[ARCHER_WLAN_RADIO_MAX];
    wait_queue_head_t thread_wqh;
    struct task_struct *thread;
    unsigned long work_avail;
    bcm_async_queue_t queue;
    int notify_enable;
    int notify_pending_disable;
} archer_wlan_socket_miss_t;

typedef struct {
    archer_socket_index_t index;
    archer_wlan_socket_tx_t tx;
    archer_wlan_socket_miss_t miss;
} archer_wlan_socket_t;

typedef struct {
    unsigned int packets;
    unsigned int transfers;
    unsigned int expirations;
    unsigned int discards;
    unsigned int fc_discards_hi;
    unsigned int fc_discards_lo;
    unsigned int fc_discards_credit;
} archer_wlan_radio_stats_t;

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
typedef struct {
    pNBuff_t pNBuff;
} archer_wlan_xfer_queue_t;

struct archer_wlan_xfer {
    bcm_async_queue_t queue;
    wait_queue_head_t thread_wqh;
    unsigned long work_avail;
    struct task_struct *thread;
} ____cacheline_aligned;

typedef struct archer_wlan_xfer archer_wlan_xfer_t;
#endif

typedef struct {
#if defined(CC_ARCHER_WLAN_XFER_THREAD)
    archer_wlan_xfer_t xfer;
#endif
    int valid;
    int wl_radio_idx;
    struct net_device *dev_p;
    archer_wlan_radio_mode_t mode;
    pktlist_context_t *pktlist_context;
    struct sk_buff *skb_cache;
    archer_wlan_radio_stats_t stats;
    bcm_timer_user_t timer;
    archer_task_t task;
    int flush_counter;
    int timer_running;
#if defined(CC_AWL_FLCTL)
    unsigned int skb_exhaustion_hi;
    unsigned int skb_exhaustion_lo;
    unsigned short pkt_prio_favor;
#endif /* CC_AWL_FLCTL */
} archer_wlan_radio_t;

typedef struct {
    archer_wlan_socket_t socket;
    archer_wlan_radio_t radio[ARCHER_WLAN_RADIO_MAX];
} archer_wlan_t;

static archer_wlan_t archer_wlan_g;

/*****************************************************************************
 * SKB flow control
 *****************************************************************************/

#if defined(CC_AWL_FLCTL)
static inline void archer_wlan_skb_flctl_init(archer_wlan_radio_t *radio_p, struct pktlist_context *pktlist_context)
{
    uint32_t bpm_total_skb = gbpm_total_skb();

    radio_p->skb_exhaustion_hi = (bpm_total_skb * ARCHER_WLAN_FLCTL_SKB_EXHAUSTION_HI_PCNT)/100;
    radio_p->skb_exhaustion_lo = (bpm_total_skb * ARCHER_WLAN_FLCTL_SKB_EXHAUSTION_LO_PCNT)/100;
    radio_p->pkt_prio_favor = ARCHER_WLAN_FLCTL_PKT_PRIO_FAVOR;

#if defined(BCM_PKTFWD_FLCTL)
    if (pktlist_context->fctable != PKTLIST_FCTABLE_NULL) {
        radio_p->pktlist_context->fctable = pktlist_context->fctable;
        pktlist_context->fctable->pkt_prio_favor = radio_p->pkt_prio_favor;
    } else {
        radio_p->pktlist_context->fctable = PKTLIST_FCTABLE_NULL;
    }
#endif /* BCM_PKTFWD_FLCTL */

    radio_p->stats.fc_discards_hi = 0;
    radio_p->stats.fc_discards_lo = 0;
    radio_p->stats.fc_discards_credit = 0;

    bcm_print("WL_FLCTL[%d] bpm_total_skb %d, exhaustion_hi %d, exhaustion_lo %d, prio_favor %d\n",
              radio_p->wl_radio_idx, bpm_total_skb, radio_p->skb_exhaustion_hi,
              radio_p->skb_exhaustion_lo, radio_p->pkt_prio_favor);
}

static inline bool archer_wlan_skb_flctl_should_drop(archer_wlan_radio_t *radio_p, wlFlowInf_t wl)
{
    /* Check for flow-control over-subscription */
    bool fc_drop = false;

    if(wl.ucast.nic.is_ucast) {
        uint16_t dest;
        uint16_t prio;
        uint32_t avail_skb;

        /* Fetch the 3b WLAN prio, by skipping the 1b IQPRIO */
        prio = GET_WLAN_PRIORITY(wl.ucast.nic.wl_prio);
        avail_skb = gbpm_avail_skb(); /* BPM free skb availability */
        dest = PKTLIST_DEST(wl.ucast.nic.wl_chainidx);

        if (avail_skb <= radio_p->skb_exhaustion_hi) {
            /* High threshold, drop all packets */
            fc_drop = true;
            ARCHER_WLAN_STATS_UPDATE(radio_p->stats.fc_discards_hi);
        } else if (prio < radio_p->pkt_prio_favor) {
            if (avail_skb <= radio_p->skb_exhaustion_lo) {
                /* Low threshold, drop low priority packets */
                fc_drop = true;
                ARCHER_WLAN_STATS_UPDATE(radio_p->stats.fc_discards_lo);
            }
#if defined(BCM_PKTFWD_FLCTL)
            else if ((radio_p->pktlist_context->fctable != PKTLIST_FCTABLE_NULL) &&
                     (__pktlist_fctable_get_credits(radio_p->pktlist_context, prio, dest) <= 0)) {
                /* drop BE/BK if no credits available */
                fc_drop = true;
                ARCHER_WLAN_STATS_UPDATE(radio_p->stats.fc_discards_credit);
            }
#endif /* BCM_PKTFWD_FLCTL */
        }
    }

    return fc_drop;
}

int archer_wlan_flctl_config_set(archer_wlflctl_config_t *cfg_p)
{
    archer_wlan_radio_t *radio_p;

    if (!cfg_p) {
        bcm_print("NULL config\n");
        return -EINVAL;
    }

    if (cfg_p->radio_idx >= ARCHER_WLAN_RADIO_MAX) {
        bcm_print("Invalid radio index %x\n", cfg_p->radio_idx);
        return -ENODEV;
    }

    radio_p = &archer_wlan_g.radio[cfg_p->radio_idx];

    if(!radio_p->valid) {
        bcm_print("Inactive radio index %x\n", cfg_p->radio_idx);
        return -EPERM;
    }

    if ( cfg_p->skb_exhaustion_lo < cfg_p->skb_exhaustion_hi) {
        bcm_print("Invalid exhaustion level lo<%u> hi<%u>\n",
                  cfg_p->skb_exhaustion_lo, cfg_p->skb_exhaustion_hi);
        return -EINVAL;
    }

    if (cfg_p->pkt_prio_favor > 7) { /* prio 0 .. 7 */
        bcm_print("Invalid pkt priority <%u>\n", cfg_p->pkt_prio_favor);
        return -EINVAL;
    }

    radio_p->skb_exhaustion_hi = cfg_p->skb_exhaustion_hi;
    radio_p->skb_exhaustion_lo = cfg_p->skb_exhaustion_lo;
    radio_p->pkt_prio_favor = cfg_p->pkt_prio_favor;

#if defined(BCM_PKTFWD_FLCTL)
    if (radio_p->pktlist_context->fctable != PKTLIST_FCTABLE_NULL) {
        radio_p->pktlist_context->fctable->pkt_prio_favor = cfg_p->pkt_prio_favor;
    }
#endif /* BCM_PKTFWD_FLCTL */

    return 0;
}

int archer_wlan_flctl_config_get(archer_wlflctl_config_t *cfg_p)
{
    archer_wlan_radio_t *radio_p;

    if (!cfg_p) {
        bcm_print("NULL config\n");
        return -EINVAL;
    }

    if (cfg_p->radio_idx >= ARCHER_WLAN_RADIO_MAX) {
        bcm_print("Invalid radio index %x\n", cfg_p->radio_idx);
        return -ENODEV;
    }

    radio_p = &archer_wlan_g.radio[cfg_p->radio_idx];

    if(!radio_p->valid) {
        bcm_print("Inactive radio index %x\n", cfg_p->radio_idx);
        return -EPERM;
    }

    cfg_p->skb_exhaustion_hi = radio_p->skb_exhaustion_hi;
    cfg_p->skb_exhaustion_lo = radio_p->skb_exhaustion_lo;
    cfg_p->pkt_prio_favor = radio_p->pkt_prio_favor;

    return 0;
}
#endif /* CC_AWL_FLCTL */

/* Single Linked List using pktlist */
static inline void archer_wlan_sll_init(pktlist_t *pktl_p)
{
    pktl_p->head = pktl_p->tail = PKTLIST_PKT_NULL;

    PKTLIST_RESET(pktl_p);
}

static inline int archer_wlan_sll_size(pktlist_t *pktl_p)
{
    return pktl_p->len;
}

static inline void archer_wlan_sll_add_skb(pktlist_t *pktl_p, struct sk_buff *skb_p)
{
    if (likely(pktl_p->len != 0))
    {
        /* pend to tail */
        PKTLIST_PKT_SET_SLL(pktl_p->tail, skb_p, SKBUFF_PTR);

        pktl_p->tail = skb_p;
    }
    else
    {
        pktl_p->head = pktl_p->tail = skb_p;
    }

    ++pktl_p->len;

    return;
}

static inline void archer_wlan_rx_miss_sll_transfer(void)
{
    int radio_idx;
    archer_wlan_rx_miss_radio_t *radio_p;
    pktlist_t *sll_p;

    for (radio_idx = 0; radio_idx < ARCHER_WLAN_RADIO_MAX; radio_idx++)
    {
        radio_p = &archer_wlan_g.socket.miss.radio[radio_idx];
        sll_p = &radio_p->pktl;

        if ((radio_p->handler) && archer_wlan_sll_size(sll_p))
        {
            radio_p->handler(radio_p->context, sll_p);
        }
    }

    return;
}

/*****************************************************************************
 * SKB Pool
 *****************************************************************************/

static inline void archer_wlan_skb_cache_prefetch(archer_wlan_radio_t *radio_p)
{
    if(likely(radio_p->skb_cache))
    {
        bcm_prefetch(radio_p->skb_cache); /* prefetch the head sk_buff */
    }
}

static inline struct sk_buff *archer_wlan_skb_header_alloc(uint32_t num_skbs)
{
    struct sk_buff *skb;

#if (defined(CONFIG_BCM_BPM) || defined(CONFIG_BCM_BPM_MODULE))
    skb = gbpm_alloc_mult_skb(num_skbs);
#else /* ! (CONFIG_BCM_BPM || CONFIG_BCM_BPM_MODULE) */
    skb = skb_header_alloc();
    if (skb != (struct sk_buff*)NULL)
    {
        skb->next = (struct sk_buff *)NULL;
    }
#endif /* ! (CONFIG_BCM_BPM || CONFIG_BCM_BPM_MODULE) */

    return skb;
}

static inline void archer_wlan_skb_header_init(struct sk_buff *skb, void *data, uint32_t len)
{
#if (defined(CONFIG_BCM_BPM) || defined(CONFIG_BCM_BPM_MODULE))

    gbpm_attach_skb(skb, data, len);

#else /* ! (CONFIG_BCM_BPM || CONFIG_BCM_BPM_MODULE) */

    skb_headerinit(archer_packet_headroom_g,
#if defined(CC_NBUFF_FLUSH_OPTIMIZATION)
                   SKB_DATA_ALIGN(len + archer_skb_tailroom_g),
#else
                   archer_packet_length_max_g,
#endif
                   skb, data, archer_wlan_recycle, 0, NULL);

    skb_trim(skb, len);
    skb->recycle_flags &= SKB_NO_RECYCLE;

#endif /* ! (CONFIG_BCM_BPM || CONFIG_BCM_BPM_MODULE) */

    memset(&(skb->bcm_ext.wlan), 0, sizeof(struct wlan_ext));

#if defined(CC_NBUFF_FLUSH_OPTIMIZATION)
    skb_shinfo(skb)->dirty_p = skb->data + BCM_DCACHE_LINE_LEN;
#endif

    bcm_prefetch(data); /* 1 BCM_DCACHE_LINE_LEN of data in cache */
}

static inline struct sk_buff *archer_wlan_skb_alloc(archer_wlan_radio_t *radio_p)
{
    struct sk_buff *skb;

    if(likely(radio_p->skb_cache))
    {
        goto skb_alloc;
    }
    else
    {
        radio_p->skb_cache = archer_wlan_skb_header_alloc(ARCHER_WLAN_SKB_CACHE_SIZE);

        if(likely(radio_p->skb_cache))
        {
            goto skb_alloc;
        }
    }

    return (struct sk_buff *)NULL;

skb_alloc:

    skb = radio_p->skb_cache;
    radio_p->skb_cache = skb->next;

    return skb;
}

/*****************************************************************************
 * Buffer Recycling
 *****************************************************************************/

#if defined(CONFIG_BCM_GLB_COHERENCY)
#define archer_wlan_data_cache_inval(addr, size)
#else
static inline void archer_wlan_data_cache_inval(unsigned long addr, unsigned long size)
{
    //blast_inv_dcache_range(addr, addr+size);
    cache_invalidate_len((void*)addr, size);
}
#endif

static inline void archer_wlan_data_buffer_free(void *datap)
{
    /*do cache invalidate */
    archer_wlan_data_cache_inval((unsigned long)datap, archer_packet_length_max_g);
#if (defined(CONFIG_BCM_BPM) || defined(CONFIG_BCM_BPM_MODULE))
    /* BPM expects data buffer ptr offseted by FkBuff_t, archer_packet_headroom_g */
    gbpm_free_buf((void*)datap);
#else
    /* datap points to start of buffer, i.e. pFkBuff */
    kfree((void *)PDATA_TO_PFKBUFF(datap, archer_packet_headroom_g));
#endif
}

static void archer_wlan_recycle(pNBuff_t pNBuff, unsigned long context, uint32_t flags)
{
    if(IS_FKBUFF_PTR(pNBuff))
    {
        /* Transmit driver is expected to perform cache invalidations */

        FkBuff_t* fkb = PNBUFF_2_FKBUFF(pNBuff);

        archer_wlan_data_buffer_free(PFKBUFF_TO_PDATA(fkb, archer_packet_headroom_g));
    }
    else /* skb */
    {
        struct sk_buff *skb = PNBUFF_2_SKBUFF(pNBuff);

        if(likely(flags & SKB_DATA_RECYCLE))
        {
#if !defined(CONFIG_BCM_GLB_COHERENCY)
//            void *data_startp = (void *)((uint8_t *)(skb->head) + archer_packet_headroom_g);
            void *data_startp = BPM_PHEAD_TO_BUF(skb->head);
            void *data_endp;
#if defined(CC_NBUFF_FLUSH_OPTIMIZATION)
            {
                void *dirty_p = skb_shinfo(skb)->dirty_p;
                void *shinfoBegin = (void*) skb_shinfo(skb);

                /* BPM skbs benefit from below. Linux skbs continue to relocate
                 * via skb_headerinit(). Caller would need to inform skb_headerinit
                 * of what is the true end of the data buffer. skb_headerinit
                 * may not assume bcm_pkt_lengths.h definitions.
                 */
                if((uintptr_t)shinfoBegin < ((uintptr_t)data_startp +
                                             (BPM_BUF_TO_END_OFFSET)))
                {
                    void *shinfoEnd;
            
                    /* skb_shared_info is located in a DMA-able region.
                     * nr_frags could be ++/-- during sk_buff life.
                     */
                    shinfoEnd = shinfoBegin + sizeof(struct skb_shared_info);
                    cache_invalidate_region(shinfoBegin, shinfoEnd);
                }

                if (dirty_p) {
                    if ((dirty_p < (void *)skb->head) || (dirty_p > shinfoBegin)) {
                        bcm_print("invalid dirty_p detected: %px valid=[%px %px]\n",
                                  dirty_p, skb->head, shinfoBegin);
                        data_endp = shinfoBegin;
                    } else {
                        data_endp = (dirty_p < data_startp) ? data_startp : dirty_p;
                    }
                } else {
                    data_endp = shinfoBegin;
                }
            }
#else
            data_endp = (void *)(skb_shinfo(skb)) + sizeof(struct skb_shared_info);
#endif
            cache_invalidate_region(data_startp, data_endp);
#endif /* !CONFIG_BCM_GLB_COHERENCY */

            /* free the data buffer */
//            archer_wlan_data_buffer_free((uint8_t *)(skb->head) + archer_packet_headroom_g);
            archer_wlan_data_buffer_free(BPM_PHEAD_TO_BUF(skb->head));
        }
#if defined(CC_ARCHER_WLAN_DEBUG)
        else
        {
            bcm_print("%s: Error only DATA recycle is supported\n", __FUNCTION__);
        }
#endif
    }
}

/*****************************************************************************
 * WLAN Receive
 *****************************************************************************/

/************************** Socket Tx Queue *************************/

static void archer_wlan_socket_free_skb_and_data(void *skb_p)
{
#if defined(CONFIG_BCM_GLB_COHERENCY)
    nbuff_free(SKBUFF_2_PNBUFF(skb_p));
#else
    nbuff_flushfree(SKBUFF_2_PNBUFF(skb_p));
#endif
}

static void archer_wlan_socket_free_skb_list(struct sk_buff *skb_p)
{
    struct sk_buff *skb_next_p = skb_p;
    struct sk_buff *skb_curr_p;

    do {
        skb_curr_p = skb_next_p;
        skb_next_p = skb_curr_p->prev; // SKBs are linked using the "prev" pointer

        archer_wlan_socket_free_skb_and_data(skb_curr_p);
    } while(skb_next_p);
}

static int archer_wlan_socket_tx_queue_read(void **skb_pp, int *ingress_port_p)
{
    if(bcm_async_queue_not_empty(&archer_wlan_g.socket.tx.queue))
    {
        archer_wlan_socket_tx_queue_t *entry_p = (archer_wlan_socket_tx_queue_t *)
            bcm_async_queue_entry_read(&archer_wlan_g.socket.tx.queue);
        struct sk_buff *skb_p;

        skb_p = READ_ONCE(entry_p->skb_p);

        *skb_pp = skb_p;
        *ingress_port_p = ARCHER_WLAN_RADIO_IDX(skb_p);

        bcm_async_queue_entry_dequeue(&archer_wlan_g.socket.tx.queue);

        ARCHER_WLAN_STATS_UPDATE(archer_wlan_g.socket.tx.queue.stats.reads);

        return 0;
    }

    return -1;
}

static int archer_wlan_socket_tx_queue_not_empty(void)
{
    return bcm_async_queue_not_empty(&archer_wlan_g.socket.tx.queue);
}

/************************** Socket Miss Queue *************************/

static int archer_wlan_socket_miss_thread(void *context)
{
    archer_wlan_socket_miss_t *miss_p = &archer_wlan_g.socket.miss;

    bcm_print("Archer WLAN Rx Thread Initialized\n");

    miss_p->notify_enable = 1;

    while(1)
    {
        wait_event_interruptible(miss_p->thread_wqh,
                                 miss_p->work_avail ||
                                 kthread_should_stop());

        if(kthread_should_stop())
        {
            __logError("kthread_should_stop detected\n");

            break;
        }

        if(miss_p->work_avail)
        {
            int budget = ARCHER_WLAN_RX_BUDGET;

            if(miss_p->notify_pending_disable)
            {
                miss_p->notify_enable = 0;
            }

            while(likely(budget-- && bcm_async_queue_not_empty(&miss_p->queue)))
            {
                archer_wlan_socket_miss_queue_t *entry_p = (archer_wlan_socket_miss_queue_t *)
                    bcm_async_queue_entry_read(&miss_p->queue);
                archer_wlan_rx_miss_radio_t *radio_p;
                struct sk_buff *skb_p;

                skb_p = READ_ONCE(entry_p->skb_p);

                bcm_async_queue_entry_dequeue(&miss_p->queue);

                radio_p = &archer_wlan_g.socket.miss.radio[ARCHER_WLAN_RADIO_IDX(skb_p)];

                if(radio_p->handler)
                {
                    archer_wlan_sll_add_skb(&radio_p->pktl, skb_p);

                    ARCHER_WLAN_STATS_UPDATE(miss_p->queue.stats.reads);
                }
                else
                {
                    archer_wlan_socket_free_skb_and_data(skb_p);

                    ARCHER_WLAN_STATS_UPDATE(miss_p->queue.stats.discards);
                }
            }

            archer_wlan_rx_miss_sll_transfer();

            if(bcm_async_queue_not_empty(&miss_p->queue))
            {
                schedule();
            }
            else
            {
                /* Queue is empty */
                clear_bit(0, &miss_p->work_avail);

                miss_p->notify_enable = 1;
            }
        }
    }

    return 0;
}

static void archer_wlan_socket_miss_notify(void)
{
    archer_wlan_socket_miss_t *miss_p = &archer_wlan_g.socket.miss;

    if(miss_p->notify_enable)
    {
        if(!miss_p->notify_pending_disable)
        {
            miss_p->notify_pending_disable = 1;
        }

        set_bit(0, &miss_p->work_avail);

        wake_up_interruptible(&miss_p->thread_wqh);
    }
    else if(miss_p->notify_pending_disable)
    {
        miss_p->notify_pending_disable = 0;
    }
}

static void archer_wlan_socket_miss_write(pNBuff_t pNBuff, int ingress_port, void *rx_skb_p)
{
    archer_wlan_socket_miss_t *miss_p = &archer_wlan_g.socket.miss;
    struct sk_buff *skb_p;

    if(IS_FKBUFF_PTR(pNBuff))
    {
        FkBuff_t *fkb_p = PNBUFF_2_FKBUFF(pNBuff);
        uint8_t *pData = PFKBUFF_TO_PDATA(fkb_p, archer_packet_headroom_g);

        skb_p = gbpm_alloc_skb();

        if(likely(skb_p))
        {
            struct sk_buff *orig_skb_p = (struct sk_buff *)rx_skb_p;

            archer_wlan_skb_header_init(skb_p, pData, fkb_p->len);

            skb_p->dev = orig_skb_p->dev;
            skb_p->bcm_ext = orig_skb_p->bcm_ext;
        }
        else
        {
            archer_wlan_data_buffer_free(pData);

            ARCHER_WLAN_STATS_UPDATE(miss_p->queue.stats.discards);
        }
    }
    else
    {
        skb_p = PNBUFF_2_SKBUFF(pNBuff);
    }

    if(likely(skb_p))
    {
        if(likely(bcm_async_queue_not_full(&miss_p->queue)))
        {
            archer_wlan_socket_miss_queue_t *entry_p = (archer_wlan_socket_miss_queue_t *)
                bcm_async_queue_entry_write(&miss_p->queue);

            WRITE_ONCE(entry_p->skb_p, skb_p);

            bcm_async_queue_entry_enqueue(&miss_p->queue);

            ARCHER_WLAN_STATS_UPDATE(miss_p->queue.stats.writes);
        }
        else
        {
            archer_wlan_socket_free_skb_and_data(skb_p);

            ARCHER_WLAN_STATS_UPDATE(miss_p->queue.stats.discards);
        }
    }

    archer_wlan_socket_miss_notify();
}

/************************** Socket Initialization *************************/

static int archer_wlan_rx_construct(void)
{
    archer_socket_args_t args;
    int entry_size;
    int ret;

    /* Initialize Socket Miss Queue */

    entry_size = ((sizeof(archer_wlan_socket_miss_queue_t) + 3) & ~3); // 32-bit aligned

    ret = bcm_async_queue_init(&archer_wlan_g.socket.miss.queue,
                               ARCHER_WLAN_SOCKET_MISS_QUEUE_SIZE, entry_size);
    if(ret)
    {
        __logError("Could not bcm_async_queue_init");

        return -1;
    }

    /* Create Socket Miss Thread */

    init_waitqueue_head(&archer_wlan_g.socket.miss.thread_wqh);

    archer_wlan_g.socket.miss.thread =
        kthread_create(archer_wlan_socket_miss_thread, NULL, "bcm_archer_wlan");

    if(IS_ERR(archer_wlan_g.socket.miss.thread)) 
    {
        return (int)PTR_ERR(archer_wlan_g.socket.miss.thread);
    }

    wake_up_process(archer_wlan_g.socket.miss.thread);

    /* Initialize Socket Tx Queue */

    entry_size = ((sizeof(archer_wlan_socket_tx_queue_t) + 3) & ~3); // 32-bit aligned

    ret = bcm_async_queue_init(&archer_wlan_g.socket.tx.queue,
                               ARCHER_WLAN_SOCKET_TX_QUEUE_SIZE, entry_size);
    if(ret)
    {
        __logError("Could not bcm_async_queue_init");

        return -1;
    }

    spin_lock_init(&archer_wlan_g.socket.tx.lock);

    /* Allocate Socket */

    args.tx_queue_read = archer_wlan_socket_tx_queue_read;
    args.tx_queue_not_empty = archer_wlan_socket_tx_queue_not_empty;
    args.free_skb_and_data = archer_wlan_socket_free_skb_and_data;
    args.miss_write = archer_wlan_socket_miss_write;
    args.ingress_phy = SYSPORT_RSB_PHY_WLAN;

    ret = archer_socket_alloc(&args, &archer_wlan_g.socket.index);
    if(ret)
    {
        return ret;
    }

    return 0;
}

/*****************************************************************************
 * WLAN Receive API
 *****************************************************************************/

int archer_wlan_rx_register(int radio_index, archer_wlan_rx_miss_handler_t handler, void *context)
{
    if(radio_index < 0 || radio_index >= ARCHER_WLAN_RADIO_MAX)
    {
        __logError("Invalid radio_index %d", radio_index);

        return -1;
    }

    archer_wlan_g.socket.miss.radio[radio_index].handler = handler;
    archer_wlan_g.socket.miss.radio[radio_index].context = context;

    archer_wlan_sll_init(&archer_wlan_g.socket.miss.radio[radio_index].pktl);

    return 0;
}
EXPORT_SYMBOL(archer_wlan_rx_register);

void archer_wlan_rx_send(struct sk_buff *skb_p)
{
    spin_lock_bh(&archer_wlan_g.socket.tx.lock);

    if(bcm_async_queue_not_full(&archer_wlan_g.socket.tx.queue))
    {
        archer_wlan_socket_tx_queue_t *entry_p = (archer_wlan_socket_tx_queue_t *)
            bcm_async_queue_entry_write(&archer_wlan_g.socket.tx.queue);

        WRITE_ONCE(entry_p->skb_p, skb_p);

        bcm_async_queue_entry_enqueue(&archer_wlan_g.socket.tx.queue);

        ARCHER_WLAN_STATS_UPDATE(archer_wlan_g.socket.tx.queue.stats.writes);
    }
    else
    {
        archer_wlan_socket_free_skb_list(skb_p);

        ARCHER_WLAN_STATS_UPDATE(archer_wlan_g.socket.tx.queue.stats.discards);
    }

    spin_unlock_bh(&archer_wlan_g.socket.tx.lock);

    /* notify send thread regardless if the queue is full or not to make sure send can proceed */
    archer_socket_run(archer_wlan_g.socket.index);
}
EXPORT_SYMBOL(archer_wlan_rx_send);

/*****************************************************************************
 * WLAN Transmit
 *****************************************************************************/

static inline struct sk_buff *archer_wlan_fkb_to_skb(archer_wlan_radio_t *radio_p,
                                                     FkBuff_t *fkb_p)
{
    struct sk_buff *skb_p = NULL;

    if(_is_in_skb_tag_(fkb_p->ptr, fkb_p->flags))
    {
        fkb_p->flags = 0;

        skb_p = (struct sk_buff *)
            ((uintptr_t)fkb_p - BLOG_OFFSETOF(sk_buff, fkbInSkb));

        if(likely(skb_p->wl.ucast.nic.is_ucast))
        {
            DECODE_WLAN_PRIORITY_MARK(skb_p->wl.ucast.nic.wl_prio, skb_p->mark);
        }
    }
    else
    {
#if defined(CC_AWL_FLCTL)
        if (archer_wlan_skb_flctl_should_drop(radio_p, fkb_p->wl) == false)
#endif /* CC_AWL_FLCTL */
        {
            archer_wlan_skb_cache_prefetch(radio_p);

            skb_p = archer_wlan_skb_alloc(radio_p);

            if(likely(skb_p))
            {
                uint8_t *pData = PFKBUFF_TO_PDATA(fkb_p, archer_packet_headroom_g);
                int offset_from_pData = (int)((uintptr_t)(fkb_p->data) - (uintptr_t)(pData));
                int length_at_pData = fkb_p->len + offset_from_pData;
                wlFlowInf_t wl = fkb_p->wl;

                bcm_prefetch(fkb_p->data + /* skb_shared_info at end of data buf */
                             BCM_DCACHE_ALIGN(archer_packet_length_max_g + archer_skb_tailroom_g));

//        bcm_prefetch(&skb_p->tail); /* tail, end, head, truesize, users */

                archer_wlan_skb_header_init(skb_p, pData, length_at_pData);

                if(offset_from_pData < 0)
                {
                    skb_push(skb_p, abs(offset_from_pData));
                }
                else if(offset_from_pData > 0)
                {
                    skb_pull(skb_p, offset_from_pData);
                }

                skb_p->wl = wl;

                if(likely(wl.ucast.nic.is_ucast))
                {
                    DECODE_WLAN_PRIORITY_MARK(wl.ucast.nic.wl_prio, skb_p->mark);
                }
            }
        }
    }

    return skb_p;
}

static inline int archer_wlan_tx_pktlist_add_fkb(archer_wlan_radio_t *radio_p, FkBuff_t *fkb_p)
{
    uint16_t pktfwd_key = (uint16_t) ~0;
    uint16_t flowring_idx;
    uint16_t pktlist_dest;
    uint16_t pktlist_prio;

    /* Fetch the 3b WLAN prio, by skipping the 1b IQPRIO */
    pktlist_prio = fkb_p->wl.ucast.dhd.wl_prio;

    if(fkb_p->wl.ucast.dhd.is_ucast)
    {
        pktlist_context_t *dst_pktlist_context = radio_p->pktlist_context->peer;

        BCM_ASSERT(dst_pktlist_context->keymap_fn);
        flowring_idx = fkb_p->wl.ucast.dhd.flowring_idx;

        (dst_pktlist_context->keymap_fn)(radio_p->wl_radio_idx, &pktfwd_key,
                                         &flowring_idx, pktlist_prio, PKTFWD_KEYMAP_F2K);

        if (pktfwd_key == ((uint16_t)~0)) {
            /* Stale packets */
            return 0;
        }

        pktlist_dest = PKTLIST_DEST(pktfwd_key);
    }
    else
    {
        pktlist_dest = PKTLIST_MCAST_ELEM; /* last col in 2d pktlist */
    }

    PKTFWD_ASSERT(pktlist_dest <= PKTLIST_MCAST_ELEM);
    PKTFWD_ASSERT(pktlist_prio <  PKTLIST_PRIO_MAX);

    __pktlist_add_pkt(radio_p->pktlist_context, /* add to local pktlist */
                      pktlist_prio, pktlist_dest, pktfwd_key,
                      FKBUFF_2_PNBUFF(fkb_p), FKBUFF_PTR);

    // FIXME: Not supported yet, drop packet

    return 1;
}

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
static inline void archer_wlan_xfer_queue_write(archer_wlan_radio_t *radio_p, pNBuff_t pNBuff);
#endif

static inline int archer_wlan_tx_packet_fkb(archer_wlan_radio_t *radio_p, FkBuff_t *fkb_p)
{
    if(_is_in_skb_tag_(fkb_p->ptr, fkb_p->flags))
    {
        struct sk_buff *skb_p = (struct sk_buff *)
            ((uintptr_t)fkb_p - BLOG_OFFSETOF(sk_buff, fkbInSkb));

        /* Reset fkbInSkb flags, as this is a union with blog_p */

        fkb_p->flags = 0;

        /* Move the FKB to the SKB data buffer */

        fkb_p = fkb_init(skb_p->head, 0, skb_p->data, skb_p->len);

        fkb_p->wl.u32 = skb_p->wl.u32;

        /* Free the SKB */

        gbpm_free_skb(skb_p);
    }

#if defined(CC_NBUFF_FLUSH_OPTIMIZATION)
    fkb_p->dirty_p = _to_dptr_from_kptr_(fkb_p->data + BCM_DCACHE_LINE_LEN);
#endif
    fkb_p->recycle_hook = archer_wlan_recycle;
    fkb_p->recycle_context = 0;

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
    archer_wlan_xfer_queue_write(radio_p, FKBUFF_2_PNBUFF(fkb_p));
#else
    if(unlikely(!archer_wlan_tx_pktlist_add_fkb(radio_p, fkb_p)))
    {
        return 0;
    }
#endif

    ARCHER_WLAN_STATS_UPDATE(radio_p->stats.packets);

    radio_p->flush_counter++;

    return 1;
}

static inline void archer_wlan_tx_pktlist_add_skb(archer_wlan_radio_t *radio_p, struct sk_buff *skb_p)
{
    uint16_t wl_key;
    uint16_t pktlist_dest;
    uint16_t pktlist_prio;

    if(skb_p->wl.ucast.nic.is_ucast)
    {
        wl_key = skb_p->wl.ucast.nic.wl_chainidx;

        pktlist_dest = PKTLIST_DEST(wl_key);
    }
    else
    {
        wl_key = 0;

        pktlist_dest = PKTLIST_MCAST_ELEM; /* last col in 2d pktlist */
    }

    /* Fetch the 3b WLAN prio, by skipping the 1b IQPRIO */
    pktlist_prio = GET_WLAN_PRIORITY(skb_p->wl.ucast.nic.wl_prio);

    PKTFWD_ASSERT(pktlist_prio == LINUX_GET_PRIO_MARK(skb_p->mark));
    PKTFWD_ASSERT(pktlist_dest <= PKTLIST_MCAST_ELEM);
    PKTFWD_ASSERT(pktlist_prio <  PKTLIST_PRIO_MAX);

    /* add to local pktlist */
    __pktlist_add_pkt(radio_p->pktlist_context, pktlist_prio, pktlist_dest,
                      wl_key, (pktlist_pkt_t *)skb_p, SKBUFF_PTR);

#if defined(BCM_PKTFWD_FLCTL)
    if (radio_p->pktlist_context->fctable != PKTLIST_FCTABLE_NULL) {
        /* Decrement available credits for pktlist */
        __pktlist_fctable_dec_credits(radio_p->pktlist_context, pktlist_prio, pktlist_dest);
    }
#endif /* BCM_PKTFWD_FLCTL */
}

static inline int archer_wlan_tx_packet_skb(archer_wlan_radio_t *radio_p, FkBuff_t *fkb_p)
{
    struct sk_buff *skb_p = archer_wlan_fkb_to_skb(radio_p, fkb_p);

    if(likely(skb_p))
    {
#if defined(CC_ARCHER_WLAN_XFER_THREAD)
        archer_wlan_xfer_queue_write(radio_p, SKBUFF_2_PNBUFF(skb_p));
#else
        archer_wlan_tx_pktlist_add_skb(radio_p, skb_p);
#endif
        ARCHER_WLAN_STATS_UPDATE(radio_p->stats.packets);

        radio_p->flush_counter++;

        return 1;
    }
    else
    {
        ARCHER_WLAN_STATS_UPDATE(radio_p->stats.discards);

        return 0;
    }
}

static inline void archer_wlan_tx_transfer(archer_wlan_radio_t *radio_p);

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
static inline void archer_wlan_xfer_processing(archer_wlan_radio_t *radio_p, pNBuff_t pNBuff)
{
    if(likely(pNBuff))
    {
        if(IS_SKBUFF_PTR(pNBuff))
        {
            struct sk_buff *skb_p = (struct sk_buff *)PNBUFF_2_PBUF(pNBuff);

#if defined(CONFIG_TENDA_PRIVATE_KM) && defined(CONFIG_MESH_MULTIFREQ_LOADBALANCE) && defined(CONFIG_TD_MESH_V3)
            int fc_open_flag = 1;
            //由于经过软加速、协议栈的hook点转发包时会重复加锁，分离硬加速Hook点进来的数据包进行处理,0软加速处理,1硬加速处理

            if (km_driver_wireless_rx_packet_hook(skb_p, radio_p->dev_p)) {
                return ;
            }

            if (km_driver_wireless_tx_packet_hook(skb_p, radio_p->dev_p)) {
                return ;
            }

            if (km_meshv3_multifreq_lb_hook) {
                if(NETDEV_TX_OK == km_meshv3_multifreq_lb_hook(skb_p, radio_p->dev_p, ((struct ethhdr *)(skb_p->data))->h_dest, fc_open_flag)) {
                    return ;
                }
            }
#endif

            archer_wlan_tx_pktlist_add_skb(radio_p, skb_p);
        }
        else
        {
            FkBuff_t *fkb_p = (FkBuff_t *)PNBUFF_2_PBUF(pNBuff);
            int ret;

            ret = archer_wlan_tx_pktlist_add_fkb(radio_p, fkb_p);
            if(!ret)
            {
#if defined(CONFIG_BCM_GLB_COHERENCY)
                nbuff_free(pNBuff);
#else
                nbuff_flushfree(pNBuff);
#endif
            }
        }
    }
    else
    {
        archer_wlan_tx_transfer(radio_p);
    }
}

static int archer_wlan_xfer_thread(void *context)
{
    archer_wlan_radio_t *radio_p = (archer_wlan_radio_t *)context;

    bcm_print("Archer WLAN XFER Thread %u Initialized\n", radio_p->wl_radio_idx);

    while(1)
    {
        int budget;

        wait_event_interruptible(radio_p->xfer.thread_wqh,
                                 radio_p->xfer.work_avail);

        if(unlikely(kthread_should_stop()))
        {
            __logError("kthread_should_stop detected\n");

            break;
        }

        budget = 512; //ARCHER_WLAN_RX_BUDGET;

        while(likely(budget-- && bcm_async_queue_not_empty(&radio_p->xfer.queue)))
        {
            archer_wlan_xfer_queue_t *entry_p = (archer_wlan_xfer_queue_t *)
                bcm_async_queue_entry_read(&radio_p->xfer.queue);
            pNBuff_t pNBuff;

            pNBuff = READ_ONCE(entry_p->pNBuff);

            bcm_async_queue_entry_dequeue(&radio_p->xfer.queue);

            ARCHER_WLAN_STATS_UPDATE(radio_p->xfer.queue.stats.reads);

            archer_wlan_xfer_processing(radio_p, pNBuff);
        }

        if(bcm_async_queue_not_empty(&radio_p->xfer.queue))
        {
            schedule();
        }
        else
        {
            clear_bit(0, &radio_p->xfer.work_avail);
        }
    }

    return 0;
}

static inline void archer_wlan_xfer_notify(archer_wlan_radio_t *radio_p)
{
    if(!radio_p->xfer.work_avail)
    {
        set_bit(0, &radio_p->xfer.work_avail);

        wake_up_interruptible(&radio_p->xfer.thread_wqh);
    }
}

static inline void archer_wlan_xfer_queue_write(archer_wlan_radio_t *radio_p, pNBuff_t pNBuff)
{
    if(likely(bcm_async_queue_not_full(&radio_p->xfer.queue)))
    {
        archer_wlan_xfer_queue_t *entry_p = (archer_wlan_xfer_queue_t *)
            bcm_async_queue_entry_write(&radio_p->xfer.queue);

        WRITE_ONCE(entry_p->pNBuff, pNBuff);

        bcm_async_queue_entry_enqueue(&radio_p->xfer.queue);

        ARCHER_WLAN_STATS_UPDATE(radio_p->xfer.queue.stats.writes);

        if(!pNBuff)
        {
            archer_wlan_xfer_notify(radio_p);
        }
    }
    else
    {
        bcm_print("XFER Queue Full\n");

        archer_wlan_xfer_notify(radio_p);

        if(pNBuff)
        {
#if defined(CONFIG_BCM_GLB_COHERENCY)
            nbuff_free(pNBuff);
#else
            nbuff_flushfree(pNBuff);
#endif
        }

        ARCHER_WLAN_STATS_UPDATE(radio_p->xfer.queue.stats.discards);
    }
}
#endif /* CC_ARCHER_WLAN_XFER_THREAD */

int archer_wlan_tx_packet_int(int radio_index, FkBuff_t *fkb_p)
{
    archer_wlan_radio_t *radio_p = &archer_wlan_g.radio[radio_index];

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
    if(likely(bcm_async_queue_free_entries(&radio_p->xfer.queue) >= 4))
#endif
    {
        int ret;

        if(ARCHER_WLAN_RADIO_MODE_FKB == radio_p->mode)
        {
            ret = archer_wlan_tx_packet_fkb(radio_p, fkb_p);
        }
        else // SKB
        {
            ret = archer_wlan_tx_packet_skb(radio_p, fkb_p);
        }

        if(likely(ret))
        {
            if(unlikely(radio_p->flush_counter >= ARCHER_WLAN_FLUSH_THRESHOLD))
            {
                radio_p->flush_counter = 0;
                radio_p->timer_running = 0;

                bcm_timer_delete(&radio_p->timer);

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
                archer_wlan_xfer_queue_write(radio_p, NULL);
#else
                archer_wlan_tx_transfer(radio_p);
#endif
            }
            else if(unlikely(!radio_p->timer_running))
            {
                radio_p->timer_running = 1;

                bcm_timer_add(&radio_p->timer, bcm_timer_jiffies() + ARCHER_WLAN_FLUSH_JIFFIES);
            }
        }

        return ret;
    }
#if defined(CC_ARCHER_WLAN_XFER_THREAD)
    else
    {
        archer_wlan_xfer_notify(radio_p);

        ARCHER_WLAN_STATS_UPDATE(radio_p->xfer.queue.stats.discards);

        return 0;
    }
#endif
}

static inline void archer_wlan_tx_transfer(archer_wlan_radio_t *radio_p)
{
    pktlist_context_t *src_pktlist_context = radio_p->pktlist_context;
    pktlist_context_t *dst_pktlist_context = src_pktlist_context->peer;
    NBuffPtrType_t type = ((ARCHER_WLAN_RADIO_MODE_FKB == radio_p->mode) ?
                           FKBUFF_PTR : SKBUFF_PTR);
    int prio;

    /* Grab AWL pktlist_context */
    PKTLIST_LOCK(dst_pktlist_context);

    /* Dispatch active mcast pktlists from archer to wl - not by priority */
    __pktlist_xfer_work(src_pktlist_context, dst_pktlist_context,
                        &src_pktlist_context->mcast,
                        &dst_pktlist_context->mcast, "MCAST", type);
 
    /* Dispatch active ucast pktlists from archer to wl - by priority */
    for(prio=0; prio<PKTLIST_PRIO_MAX; ++prio)
    {
        /* Process non empty ucast[] worklists in archer pktlist context */
        __pktlist_xfer_work(src_pktlist_context, dst_pktlist_context,
                            &src_pktlist_context->ucast[prio],
                            &dst_pktlist_context->ucast[prio],
                            "UCAST", type);
    }

    /* Release AWL pktlist context */
    PKTLIST_UNLK(dst_pktlist_context);

    /* Wake peer wl thread: invoke handoff handler to wake peer driver.
     * handoff handler is the HOOK32 wl_completeHook in archer_wlan_bind.
     */
    (src_pktlist_context->xfer_fn)(dst_pktlist_context);

    ARCHER_WLAN_STATS_UPDATE(radio_p->stats.transfers);
}

int archer_wlan_tx_task_handler(void *arg_p)
{
    archer_wlan_radio_t *radio_p = (archer_wlan_radio_t *)arg_p;

//    bcm_print("Radio Timeout!\n");

    if(radio_p->flush_counter)
    {
        radio_p->flush_counter = 0;

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
        archer_wlan_xfer_queue_write(radio_p, NULL);
#else
        archer_wlan_tx_transfer(radio_p);
#endif
        ARCHER_WLAN_STATS_UPDATE(radio_p->stats.expirations);
    }

    radio_p->timer_running = 0;

    return 0;
}

void archer_wlan_tx_timer_handler(void *arg_p)
{
    archer_wlan_radio_t *radio_p = (archer_wlan_radio_t *)arg_p;

    archer_task_schedule(&radio_p->task, ARCHER_TASK_PRIORITY_LOW);
}

char *archer_wlan_tx_dev_name_int(int radio_index)
{
    archer_wlan_radio_t *radio_p = &archer_wlan_g.radio[radio_index];

    return radio_p->dev_p->name;
}

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
static int archer_wlan_xfer_init(archer_wlan_radio_t *radio_p)
{
    int entry_size;
    int ret;

    /* Initialize Tx Queue */

    entry_size = ((sizeof(archer_wlan_xfer_queue_t) + 3) & ~3); // 32-bit aligned

    ret = bcm_async_queue_init(&radio_p->xfer.queue, ARCHER_WLAN_XFER_QUEUE_SIZE, entry_size);
    if(ret)
    {
        __logError("Could not bcm_async_queue_init");

        return -1;
    }

    /* Create Transmit Thread */

    init_waitqueue_head(&radio_p->xfer.thread_wqh);

    radio_p->xfer.thread = kthread_create(archer_wlan_xfer_thread, radio_p,
                                          "bcm_awl_xfer_%u", radio_p->wl_radio_idx);
    if(IS_ERR(radio_p->xfer.thread)) 
    {
        return (int)PTR_ERR(radio_p->xfer.thread);
    }

    wake_up_process(radio_p->xfer.thread);

    return 0;
}
#endif

/*****************************************************************************
 * WLAN Statistics
 *****************************************************************************/

#if defined(CC_ARCHER_WLAN_STATS)
static void archer_wlan_cpu_queue_dump(bcm_async_queue_t *queue_p, char *name, int index)
{
    bcm_print("%s[%d]: Depth %d, Level %d, Writes %d, Reads %d, Discards %d, Writes+Discards %d\n", name, index,
              queue_p->depth, queue_p->depth - bcm_async_queue_free_entries(queue_p),
              queue_p->stats.writes, queue_p->stats.reads, queue_p->stats.discards,
              queue_p->stats.writes + queue_p->stats.discards);
}
#else
static void archer_wlan_cpu_queue_dump(bcm_async_queue_t *queue_p, char *name, int index)
{
    bcm_print("%s[%d]: Depth %d, Level %d, Write %d, Read %d\n", name, index,
              queue_p->depth, queue_p->depth - bcm_async_queue_free_entries(queue_p),
              queue_p->write, queue_p->read);
}
#endif

void archer_wlan_stats(void)
{
    archer_wlan_radio_t *radio_p;
    int radio_index;

    for(radio_index=0; radio_index<ARCHER_WLAN_RADIO_MAX; ++radio_index)
    {
        radio_p = &archer_wlan_g.radio[radio_index];

        if(radio_p->valid)
        {
            int coalescing = (radio_p->stats.transfers) ?
                (radio_p->stats.packets / radio_p->stats.transfers) : 0;

            bcm_print("WLAN_TX[%d]: %s, packets %u, transfers %u, coalescing %u, "
                      "expirations %u, discards %u (fc:lo %u hi %u credit %u)\n",
                      radio_index, archer_wlan_tx_dev_name_int(radio_index),
                      radio_p->stats.packets, radio_p->stats.transfers,
                      coalescing, radio_p->stats.expirations,
                      radio_p->stats.discards, radio_p->stats.fc_discards_lo,
                      radio_p->stats.fc_discards_hi, radio_p->stats.fc_discards_credit);
        }
    }

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
    for(radio_index=0; radio_index<ARCHER_WLAN_RADIO_MAX; ++radio_index)
    {
        radio_p = &archer_wlan_g.radio[radio_index];

        if(radio_p->valid)
        {
            archer_wlan_cpu_queue_dump(&radio_p->xfer.queue, "WLAN_XFER", radio_index);
        }
    }
#endif

    archer_wlan_cpu_queue_dump(&archer_wlan_g.socket.tx.queue, "WLAN_RXQ", 0);
    archer_wlan_cpu_queue_dump(&archer_wlan_g.socket.miss.queue, "WLAN_MISSQ", 0);
}

static int archer_wlan_pktlist_context_keymap_fn_t(uint32_t radio_idx,
                                                   uint16_t * key, uint16_t * flowid, uint16_t prio, bool k2f) 
{
    __logError("Not supported");

    return 0;
}

/*****************************************************************************
 * WLAN Transmit API
 *****************************************************************************/

int archer_wlan_bind(struct net_device *dev_p,
                     struct pktlist_context *wl_pktlist_context,
                     archer_wlan_radio_mode_t mode,
                     HOOK32 wl_completeHook,
                     int wl_radio_idx)
{
    archer_wlan_radio_t *radio_p;
    int radio_index;
#if defined(CC_ARCHER_WLAN_XFER_THREAD)
    int ret;
#endif

    if(wl_pktlist_context == PKTLIST_CONTEXT_NULL)
    {
        __logError("Invalid wl_pktlist_context (NULL)");

        return -1;
    }

    if(wl_completeHook == NULL)
    {
        __logError("Invalid wl_completeHook (NULL)");

        return -1;
    }

    if(mode >= ARCHER_WLAN_RADIO_MODE_MAX)
    {
        __logError("Invalid mode %d", mode);

        return -1;
    }

#if defined(CONFIG_BCM_WLAN_DPDCTL)
    /* Use radio_idx as wfd idx as well */
    radio_index = wl_radio_idx;
    if(radio_index >= ARCHER_WLAN_RADIO_MAX)
    {
        __logError("Radios idx %d range exceeded, max %d", radio_index, ARCHER_WLAN_RADIO_MAX);

        return -1;
    }
    radio_p = &archer_wlan_g.radio[radio_index];
    if(radio_p->valid)
    {
        __logError("Already in use radio idx %d", radio_index);

        return -1;
    }
#else  /* !CONFIG_BCM_WLAN_DPDCTL */
    // Find a free radio entry
    for(radio_index=0; radio_index<ARCHER_WLAN_RADIO_MAX; ++radio_index)
    {
        radio_p = &archer_wlan_g.radio[radio_index];

        if(!radio_p->valid)
        {
            break;
        }
    }

    if(ARCHER_WLAN_RADIO_MAX == radio_index)
    {
        __logError("Too many Radios, max %d", ARCHER_WLAN_RADIO_MAX);

        return -1;
    }
#endif /* !CONFIG_BCM_WLAN_DPDCTL */

    memset(radio_p, 0, sizeof(archer_wlan_radio_t));

    radio_p->wl_radio_idx = wl_radio_idx;
    radio_p->dev_p = dev_p;
    radio_p->mode = mode;

    radio_p->pktlist_context = pktlist_context_init(wl_pktlist_context,
                                                    (pktlist_context_xfer_fn_t)wl_completeHook,
                                                    archer_wlan_pktlist_context_keymap_fn_t,
                                                    radio_p, "ARCHER_WLAN", radio_index);

    if(radio_p->pktlist_context == PKTLIST_CONTEXT_NULL)
    {
        __logError("Could not pktlist_context_init, wl_radio_idx %d", wl_radio_idx);

        return -1;
    }

#if defined(CC_AWL_FLCTL)
    if ((wl_pktlist_context != PKTLIST_CONTEXT_NULL) &&
        (mode == ARCHER_WLAN_RADIO_MODE_SKB)) {
        archer_wlan_skb_flctl_init(radio_p, wl_pktlist_context);
    }
#endif /* CC_AWL_FLCTL */

    bcm_timer_init(&radio_p->timer, BCM_TIMER_MODE_ONESHOT, 0,
                   archer_wlan_tx_timer_handler, radio_p);

    ARCHER_TASK_INIT(&radio_p->task, ARCHER_THREAD_ID_US,
                     archer_wlan_tx_task_handler, radio_p);

#if defined(CC_ARCHER_WLAN_XFER_THREAD)
    ret = archer_wlan_xfer_init(radio_p);
    if(ret)
    {
        return ret;
    }
#endif

    radio_p->valid = 1;

    bcm_print(CLRb "Archer WLAN Bind: %s, wl_radio_idx %d, radio_index %d, mode %s" CLRnl,
              dev_p->name, wl_radio_idx, radio_index,
              (mode == ARCHER_WLAN_RADIO_MODE_SKB) ? "SKB" : "FKB");

    return radio_index;
}
EXPORT_SYMBOL(archer_wlan_bind);

int archer_wlan_unbind(int radio_index)
{
    archer_wlan_radio_t *radio_p = &archer_wlan_g.radio[radio_index];

    if(radio_index < 0 || radio_index >= ARCHER_WLAN_RADIO_MAX)
    {
        __logError("Invalid radio_index %d", radio_index);

        return -1;
    }

    if(!radio_p->valid)
    {
        __logError("radio_index %d not bound", radio_index);

        return -1;
    }

    if(radio_p->pktlist_context)
    {
        pktlist_context_dump(radio_p->pktlist_context, true, true);

        pktlist_context_fini(radio_p->pktlist_context);
    }

    bcm_print(CLRb "Archer WLAN Unbind: %s, radio_index %d, mode %s" CLRnl,
              radio_p->dev_p->name, radio_index,
              (radio_p->mode == ARCHER_WLAN_RADIO_MODE_SKB) ? "SKB" : "FKB");

    radio_p->valid = 0;

    return 0;
}
EXPORT_SYMBOL(archer_wlan_unbind);

/*****************************************************************************
 * Initialization
 *****************************************************************************/

int __init archer_wlan_construct(void)
{
    memset(&archer_wlan_g, 0, sizeof(archer_wlan_t));

    bcm_print("Archer WLAN Interface Construct (Threshold %u packets, Timeout %u usec, Jiffies %u)\n",
              ARCHER_WLAN_FLUSH_THRESHOLD, ARCHER_WLAN_FLUSH_TIMEOUT_USEC, ARCHER_WLAN_FLUSH_JIFFIES);

    return archer_wlan_rx_construct();
}
