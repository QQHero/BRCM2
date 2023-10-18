/*
<:copyright-BRCM:2009:proprietary:standard

   Copyright (c) 2009 Broadcom
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
 * File Name  : bpm.c
 *
 *******************************************************************************
 */
/* -----------------------------------------------------------------------------
 *                      Global Buffer Pool Manager (BPM)
 * -----------------------------------------------------------------------------
 * When the system boots up all the buffers are owned by BPM.
 *
 * Interface Initialization:
 * ------------------------
 * When an interface is initialized, the interface assigns buffers to
 * descriptors in it's private RX ring by requesting buffer allocation
 * from BPM (APIs: gbpm_alloc_mult_buf() or gbpm_alloc_buf()),
 * and also informs BPM how many buffers were assigned to RX ring
 * (gbpm_resv_rx_buf()).
 *
 * Similarly, when an interface is uninitialized, the interface frees
 * buffers from descriptors in it's private RX ring by requesting buffer
 * free to BPM (APIs: gbpm_free_mult_buf() or gbpm_free_buf()),
 * and also informs BPM how many buffers were freed from RX ring
 * (gbpm_unresv_rx_buf()).
 *
 * Knowing the size of RX ring allows BPM to keep track of the reserved
 * buffers (assigned to rings) and hence find out how many dynamic buffers
 * are available, which can be shared between the interfaces in the system.
 *
 * Buffer Allocation:
 * ------------------
 * When a packet is received, the buffer is not immediately replenished
 * into RX ring, rather a count is incremented, to keep track of how many
 * buffer allocation requests are pending. This is done to delay as much
 * as possible going to BPM because of overheads (locking, cycles, etc.),
 * and it likely that the RX ring will be soon replenished with a recycled
 * buffer.
 *
 * When an interface's RX ring buffers usage (RX DMA queue depth) exceeds
 * the configured threshold (because the earlier used buffers are not yet
 * recycled to RX ring), the interface requests BPM for allocation of
 * more buffers. The buffer request is fullfilled from the available
 * dynamic buffers in BPM. After one or two such buffer allocation requests
 * equilibirium is established where the newly used buffers are replenished
 * with the recycled buffers.
 *
 * Buffer Free/Recycle:
 * --------------------
 *  After a packet is transmitted or dropped, the recycle function of
 *  RX interface is invoked. Recycle function first checks whether there
 *  is a space in the RX ring. If yes, the buffer is recycled to ring.
 *  If not, buffer is freed to BPM.
 *
 *
 * FkBuff_t:
 * Even though a Buffer has a FkBuff_t at the start of a buffer, this FkBuff_t
 * structure is not initialized as in fkb_preinit(). The recycle_hook needs to
 * be appropriately setup. An FkBuff_t structure does not have a recycle_flags
 * and the recycle_context field was re-purposed. Need to undo this and use the
 * recycle context to save the &bpm_g.
 *
 *
 * TX Queue Threhsolds:
 * --------------------
 * TX Queue Thresholds are configured for the slower interfaces like XTM,
 * so that a lot of buffers are not held up in the TX queues of
 * these interfaces.
 *
 * There are two TX Q thresholds per queue of an interface: low and high.
 * Only one of the low or high threshold is used to compare against the
 * current queue depth. If the TX Q depth is lower than the compared
 * threshold, the packet is enqueued, else the packet is dropped.
 *
 * The low or high threshold to be used is decided based on the current
 * level of dynamic buffers available in the system
 * (API gbpm_get_dyn_buf_lvl() ). If the current dynamic buffer
 * level is less than the configured threshold, then the low threshold
 * is used else the high threshold is used.
 *
 *
 * Pre-allocated sk_buff Pool
 * --------------------------
 * BPM also supports a preallocated pool of sk_buffs. BPM managed sk_buffs,
 * avoid the need for a kmem skb_head_cache alloc, and reduced memsets of the
 * struct sk_buff and the skb_shared_info. Unlike skb_header_init(), where a
 * skb_shared_info may be relocated, soon after the tail, given a specified
 * tailroom, in a BPM sk_buff, the tailroom will always be beyond the DMA-able
 * region, on a cacheline aligned boundary. BPM sk_buffs may only be attached to
 * BPM buffers (or buffers that follow the bcm_pkt_lengths.h formats).
 *
 *
 * Note: API prototypes are given in gbpm.h file.
 */
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/nbuff.h>
#include <linux/fs.h>
#include <linux/bcm_log_mod.h>
#include <linux/bcm_log.h>
#include <linux/export.h>
#include <board.h>
#include <linux/gbpm.h>
#include <bpmctl_common.h>
#include <bcm_pkt_lengths.h>
#include <bcmPktDma_defines.h>
#include <bpm.h>
#include "bcm_prefetch.h"
#include <linux/kthread.h>
#include "bpm_common.h"
#include "bpm_inline.h"
#ifdef CONFIG_TENDA_PRIVATE_KM
#include <net/km_common.h>
#endif
#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
#include "bpm_track.h"
#endif

#if !(defined(CONFIG_BCM_RDPA) && !defined(CONFIG_BCM_RDPA_MODULE))
#if defined(CONFIG_BCM_PKTDMA)
extern int bcmPktDma_GetTotRxBds(void);
#endif
#endif

#define BPM_DEFS()             bpm_t *bpm_pg = &bpm_g

#if defined(CONFIG_SMP) || defined(CONFIG_PREEMPT)
#define BPM_LOCK               (&bpm_pg->lock)
#define BPM_FLAGS_DEF()        unsigned long _flags
#define BPM_LOCK_IRQ()         spin_lock_irqsave(BPM_LOCK, _flags)
#define BPM_UNLOCK_IRQ()       spin_unlock_irqrestore(BPM_LOCK, _flags)
#define BPM_LOCK_BH()          spin_lock_bh(BPM_LOCK)
#define BPM_UNLOCK_BH()        spin_unlock_bh(BPM_LOCK)
#else
#define BPM_FLAGS_DEF()             /* */
#define BPM_LOCK_IRQ()         local_irq_disable()
#define BPM_UNLOCK_IRQ()       local_irq_enable()
#define BPM_LOCK_BH()          local_bh_disable()
#define BPM_UNLOCK_BH()        local_bh_enable()
#endif

#if defined(CC_BPM_SKB_POOL_BUILD)
#if defined(CC_BPM_SKB_POOL_STATS)
#define BPM_SKB_POOL_STATS_ADD(u32, val)  (bpm_pg->u32 += (val))
#define BPM_SKB_POOL_STATS_SUB(u32, val)  (bpm_pg->u32 -= (val))
#define BPM_SKB_POOL_STATS_BPM_DEFS()     BPM_DEFS()
#else /* !defined(CC_BPM_SKB_POOL_STATS) */
#define BPM_SKB_POOL_STATS_ADD(u32, val)  do { } while (0)
#define BPM_SKB_POOL_STATS_SUB(u32, val)  do { } while (0)
#define BPM_SKB_POOL_STATS_BPM_DEFS()
#endif /* defined(CC_BPM_SKB_POOL_STATS) */
#endif /* CC_BPM_SKB_POOL_BUILD */

struct task_struct *monitor_task = NULL;
wait_queue_head_t monitor_task_wqh;
volatile uint32_t monitor_task_wakeup;

#define BPM_SOAK_TIMER_DURATION       10000

//Monitor thread wakeup events
#define BPM_BUF_EXPAND_EVENT          0x1
#define BPM_SKB_EXPAND_EVENT          0x2
#define BPM_SOAK_TIMER_EXPIRED        0x4

typedef struct _bpm_dyn_cfg {
    uint32_t bpm_max_buf_count;
    uint32_t bpm_initial_buf_count;
    uint32_t bpm_buf_avail_lower_th;
    uint32_t bpm_buf_expand_step_size;
    uint32_t bpm_buf_shrink_step_size;
    uint32_t bpm_max_skb_count;
    uint32_t bpm_initial_skb_count;
    uint32_t bpm_skb_avail_lower_th;
    uint32_t bpm_skb_expand_step_size;
    uint32_t bpm_skb_shrink_step_size;
    uint32_t bpm_soak_timer_duration;
} bpm_dyn_cfg_t;


/* Implementation notes on global instance of bpm_g.
 *
 * 1. Do not use a global pointer in bss to access global object in data.
 * 2. spinlock dynamically initialized in module_init, "after" memset.
 *    If bpm_g needs to be statically initialized as follows,
 *       = { .lock = __SPIN_LOCK_UNLOCKED(.lock), .buf_pool = NULL };
 *    ensure that memset does not re-zero-out spinlock_t lock.
 * 3. Arrange fields to avail of data locality (cacheline).
 * 4. BPM Buf Pool managed using an inverted stack, as opposed to a circular
 *    buffer, with a head, tail, last index.
 * 5. BPM buf alloc request is serviced by a pointer offseted into the buffer.
 *    All buffers will have a fixed FkBuff_t and HEADROOM. Pointer returned is
 *    hence the "data" pointer fed to DMA.
 * 6. All buffers will have a skb_shared_info at the end. Ths skb_shared_info
 *    is not to be relocated, based on the length of a received packet.
 * 7. A BPM based SKB MUST ONLY be attached to a BPM based buffer. BPM manages
 *    sk_buffs with minimal sk_buff initializaton.
 */
struct bpm {

    spinlock_t lock;

    struct kmem_cache *bpmbuff_cache;  /* kmem cache pointer */
    uint32_t buf_alloc_cnt; /* count of allocated buffers */
    uint32_t buf_free_cnt;  /* count of free buffers (avail?) */
    uint32_t buf_fail_cnt;  /* statistics of allocs requests failures */
    uint32_t min_avail_cnt; /* minimal availability that bpm reached through its lifetime */
    uint32_t max_buf_wm;    /* maximum buffer use watermark */
    uint32_t buf_expand_cnt;/* number of buffer expansion */
    uint32_t buf_shrink_cnt;/* number of buffer shrinks */
    uint32_t buf_exp_fail_cnt;/* number of buffer expansion failures */
    uint32_t buf_inthr_alloc_cnt; /* number of kernel allocation in bpm buf alloc context*/
    uint32_t skb_inthr_alloc_cnt; /* number of skb allocation that happens in skb alloc request context*/

    uint32_t buf_top_idx;   /* true index of last slot in buf_pool */
    uint32_t buf_cur_idx;   /* cur index in inverted stack for allocation */
    void **buf_pool;        /* pool is managed as an inverted stack */
#if defined(CC_BPM_SKB_POOL_BUILD)
    struct sk_buff * skb_freelist;  /* free list of sk_buff(s) in pool */
    uint32_t skb_avail;     /* sk_buff(s) available in free list */
    uint32_t skb_total;     /* total managed by pool. (alloc = total - avail) */
    uint32_t skb_bound;     /* pool total may not extend beyond bound */
#if defined(CC_BPM_SKB_POOL_STATS)
    uint32_t skb_alloc_cnt; /* number of allocations serviced from pool */
    uint32_t skb_bpm_alloc_cnt; /* number of allocations with SKB */
    uint32_t skb_free_cnt;  /* number of recycles (frees back) to pool */
    uint32_t skb_fast_free_cnt;  /* number of recycles (frees back) to pool */
    uint32_t skb_error_cnt; /* number of errors */
    uint32_t skb_fail_cnt;  /* number of failures, not essentially errors */
    uint32_t skb_grow_cnt;  /* number of times pool was extended */
    uint32_t skb_hdr_reset_cnt;  /* number of skb hdr resets */
    uint32_t shinfo_reset_cnt;   /* number of skb_shinfo resets */
    uint32_t full_cache_inv_cnt; /* number of full buffer cache invalidates */
    uint32_t part_cache_inv_cnt; /* number of partial cache invalidates */
#endif /* CC_BPM_SKB_POOL_STATS */
#endif /* CC_BPM_SKB_POOL_BUILD */

#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
    uint32_t last_memsz;
#endif
    uint32_t last_buf_avail_snapshot;
    struct timer_list soak_timer;
    void **buffers_to_free;         /* temporary pool for storing buffer ptrs that will be freed for shrinking */
    struct sk_buff * skbs_to_free;  /* temporary list of sk_buff(s) to be freed back to kernel */
    bpm_dyn_cfg_t dyn_cfg;
    uint32_t dyn_buf_lo_thresh;
    uint32_t max_dyn;
    void **mem_pool;
    uint32_t mem_idx;
    uint32_t rxbds;
    uint32_t tot_resv_buf;
    uint32_t tot_rx_ring_buf;
    uint32_t tot_alloc_trig;
    uint32_t status[GBPM_PORT_MAX][GBPM_RXCHNL_MAX];
    uint32_t num_rx_buf[GBPM_PORT_MAX][GBPM_RXCHNL_MAX];
    uint32_t alloc_trig_thresh[GBPM_PORT_MAX][GBPM_RXCHNL_MAX];

} ____cacheline_aligned;

typedef struct bpm bpm_t;

bpm_t bpm_g; /* GLOBAL Instance of a BufferPoolManager */


/* Helper macros for BPM Buf Pool management */
// BPM_BUF_AVAIL_CNT provides number of buffers currently available in pool
#define BPM_BUF_AVAIL_CNT(bpm)        ((bpm)->buf_top_idx - (bpm)->buf_cur_idx)
// BPM_BUF_MAX_AVAIL_CNT provides number of buffers currently available in pool
// and buffers that can be added to the pool by expansion
#define BPM_BUF_MAX_AVAIL_CNT(bpm)        ((bpm)->max_dyn - (bpm)->buf_cur_idx)

#if defined(CC_BPM_SKB_POOL_BUILD)
// BPM_SKB_AVAIL_CNT provides number of skb currently available in skb pool
#define BPM_SKB_AVAIL_CNT(bpm)        ((bpm)->skb_avail)
// BPM_SKB_MAX_AVAIL_CNT provides number of buffers currently available in pool
// and skbs that can be added to the pool by the expansion
#define BPM_SKB_MAX_AVAIL_CNT(bpm)    ((bpm)->skb_avail + ((bpm)->skb_bound - (bpm)->skb_total))
#else
#define BPM_SKB_AVAIL_CNT(bpm)        (0)
#define BPM_SKB_MAX_AVAIL_CNT(bpm)    (0)
#endif   //CC_BPM_SKB_POOL_BUILD

/* Prefetch helper macros */
#define BPM_PREFETCH()                bcm_prefetch_ld(bpm_pg)
#define BPM_BUF_POOL_PREFETCH_LD(idx) bcm_prefetch_ld(&bpm_pg->buf_pool[idx])
#define BPM_BUF_POOL_PREFETCH_ST(idx) bcm_prefetch_st(&bpm_pg->buf_pool[idx])

/* Dynamic expansion shrink macros and helper function defs*/
#define IS_BPM_DYN_ALLOC_MODE() (bpm_pg->dyn_cfg.bpm_max_buf_count != bpm_pg->dyn_cfg.bpm_initial_buf_count)
#define IS_BPM_ALLOC_REACHED_MAX() (bpm_pg->buf_top_idx == bpm_pg->dyn_cfg.bpm_max_buf_count)

static int bpm_buf_expand(uint32_t num_bufs);
static inline void start_soak_timer(bpm_t *bpm_pg);
static inline void *bpm_alloc_buf_from_cache(bpm_t *bpm_pg);
static inline void bpm_buf_expand_post_event(bpm_t *bpm_pg);
static inline void bpm_skb_expand_post_event(bpm_t *bpm_pg);
static int bpm_monitor_thread(void *data);
static int bpm_grow_skb_pool(uint32_t skb_grow);


#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
/*
 *------------------------------------------------------------------------------
 * function   : bpm_get_buffer_base
 * description: Caculates base address given a pointer into a BPM buffer.
 *------------------------------------------------------------------------------
 */
void *bpm_get_buffer_base(void *buf_p)
{
    int i;
    uint32_t memsz;
    uint8_t *mem_p;
    uint8_t *test_p;
    uintptr_t diff;

    BPM_DEFS();

    if (buf_p == NULL)
        return NULL;

    test_p = (uint8_t *)buf_p;
    for (i = 0; i < bpm_pg->mem_idx; ++i) {
        /* Check if address is in a mem_pool range */
        mem_p = (uint8_t *)bpm_pg->mem_pool[i];
        memsz = (i == bpm_pg->mem_idx - 1) ? bpm_pg->last_memsz :
                                             BPM_MAX_MEMSIZE - L1_CACHE_BYTES;
        if ((test_p >= mem_p) && (test_p < (mem_p + memsz))) {
            diff = (uintptr_t)(test_p - mem_p);
            diff = (diff / BPM_BUF_SIZE) * BPM_BUF_SIZE;
            return (void *)(mem_p + diff);
        }
    }

    /* Invalid address */
    return (void *)NULL;
}

/*
 *------------------------------------------------------------------------------
 * function   : bpm_get_last_idx
 * description: Return the last index of the current active pool.
 *------------------------------------------------------------------------------
 */
int bpm_get_last_idx(void)
{
    BPM_DEFS();
    return bpm_pg->buf_top_idx;
}
#endif

/*
 *------------------------------------------------------------------------------
 * function   : bpm_validate_resv_rx_buf
 * description: validates the port enable
 *------------------------------------------------------------------------------
 */
static int bpm_validate_resv_rx_buf(gbpm_port_t port, uint32_t chnl,
                                    uint32_t num_rx_buf,
                                    uint32_t alloc_trig_thresh)
{
    /* validate parameters */
    if ((port >= GBPM_PORT_MAX) || (chnl >= GBPM_RXCHNL_MAX)) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,
            "invalid port=%d or chnl=%d", port, chnl);
        return BPM_ERROR;
    }

    if ((num_rx_buf < alloc_trig_thresh)) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,
            "invalid alloc_trig_thresh=%d num_rx_buf=%d",
            alloc_trig_thresh, num_rx_buf);
        return BPM_ERROR;
    }

    return BPM_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * function   : bpm_upd_dyn_buf_lo_thresh
 * description: updates the dynamic buffer low threshold
 *------------------------------------------------------------------------------
 */
static void bpm_upd_dyn_buf_lo_thresh(void)
{
    BPM_DEFS();

    /* calc the low thresh for dynamic buffers in the global buffer pool */
    bpm_pg->dyn_buf_lo_thresh =
       (bpm_pg->max_dyn * BPM_PCT_DYN_BUF_LO_THRESH / 100);
}

static int bpm_resv_rx_buf(gbpm_port_t port, uint32_t chnl, uint32_t num_rx_buf,
                           uint32_t alloc_trig_thresh)
{
    BPM_DEFS();
    BPM_FLAGS_DEF();

    /* validate parameters */
    if (bpm_validate_resv_rx_buf(port, chnl, num_rx_buf, alloc_trig_thresh))
        return BPM_ERROR;

    BPM_LOCK_IRQ();

    /* flag the chnl has been enabled */
    bpm_pg->status[port][chnl] = GBPM_RXCHNL_ENABLED;
    bpm_pg->num_rx_buf[port][chnl] = num_rx_buf;
    bpm_pg->alloc_trig_thresh[port][chnl] = alloc_trig_thresh;
    bpm_pg->tot_rx_ring_buf += num_rx_buf;
    bpm_pg->tot_alloc_trig += alloc_trig_thresh;

    BCM_LOG_DEBUG(BCM_LOG_ID_BPM,
                  "port=%d chnl=%d resv_rx_buf=%d alloc_trig_thresh=%d",
                  port, chnl, bpm_pg->num_rx_buf[port][chnl],
                  bpm_pg->alloc_trig_thresh[port][chnl]);

    BPM_UNLOCK_IRQ();

    return BPM_SUCCESS;
}


static int bpm_unresv_rx_buf(gbpm_port_t port, uint32_t chnl)
{
    BPM_DEFS();

    /* flag the chnl has been disabled */
    bpm_pg->status[port][chnl] = GBPM_RXCHNL_DISABLED;

    bpm_pg->tot_alloc_trig -= bpm_pg->alloc_trig_thresh[port][chnl];
    bpm_pg->tot_rx_ring_buf -= bpm_pg->num_rx_buf[port][chnl];

    bpm_pg->num_rx_buf[port][chnl] = 0;
    bpm_pg->alloc_trig_thresh[port][chnl] = 0;

    BCM_LOG_DEBUG(BCM_LOG_ID_BPM, "port=%d chnl=%d unresv_rx_buf=%d",
                  port, chnl, bpm_pg->num_rx_buf[port][chnl]);

    return BPM_SUCCESS;
}

static inline void update_min_availability(void)
{
    BPM_DEFS();
    const uint32_t buf_avail_cnt = BPM_BUF_AVAIL_CNT(bpm_pg);

    if (unlikely(buf_avail_cnt < bpm_pg->dyn_cfg.bpm_buf_avail_lower_th))
        bpm_buf_expand_post_event(bpm_pg);

    if (unlikely(buf_avail_cnt >
                 (bpm_pg->dyn_cfg.bpm_buf_avail_lower_th +
                  bpm_pg->dyn_cfg.bpm_buf_shrink_step_size)))
        start_soak_timer(bpm_pg);

    if (unlikely(bpm_pg->min_avail_cnt > buf_avail_cnt))
        bpm_pg->min_avail_cnt = buf_avail_cnt;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_alloc_buf
 * Description: allocates a buffer from global buffer pool
 *------------------------------------------------------------------------------
 */
static void *bpm_alloc_buf(void)
{
    BPM_DEFS();
    BPM_FLAGS_DEF();
    void *buf_p;
    BPM_PREFETCH();

    BPM_LOCK_IRQ();

    BPM_BUF_POOL_PREFETCH_LD(bpm_pg->buf_cur_idx);
    ++bpm_pg->buf_alloc_cnt;

    /* if buffers available in global buffer pool */
    if (likely(bpm_pg->buf_cur_idx < bpm_pg->buf_top_idx)) {
        buf_p = bpm_pg->buf_pool[bpm_pg->buf_cur_idx++]; // POST ++
        update_min_availability();
    } else if (bpm_pg->buf_top_idx < bpm_pg->dyn_cfg.bpm_max_buf_count) {
        /* Allocate buffer */
        if ((buf_p = bpm_alloc_buf_from_cache(bpm_pg)) == NULL) {
            BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Kmem_cache_alloc failed to "
                                          "allocate a BPM buffer");
            buf_p = NULL;
            --bpm_pg->buf_alloc_cnt;
            ++bpm_pg->buf_fail_cnt;
            goto alloc_exit;
        }
        bpm_pg->buf_inthr_alloc_cnt++;

        /* Save the data_p and not the in the buf_pool[] table */
        bpm_pg->buf_pool[bpm_pg->buf_top_idx++] = (void *)buf_p;
        buf_p = bpm_pg->buf_pool[bpm_pg->buf_cur_idx++]; // POST ++
        update_min_availability();
    } else {
        buf_p = NULL;
        --bpm_pg->buf_alloc_cnt;
        ++bpm_pg->buf_fail_cnt;
    }
alloc_exit:
    BPM_UNLOCK_IRQ();

    return buf_p;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_free_buf
 * Description: frees a buffer to global buffer pool
 *------------------------------------------------------------------------------
 */
static void bpm_free_buf(void *buf_p)
{
    BPM_DEFS();
    BPM_FLAGS_DEF();
    BPM_PREFETCH();

    if (unlikely(buf_p == NULL)) {
        ++bpm_pg->buf_fail_cnt;
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,"bpm free NULL buf_p\n");
        BCM_ASSERT(buf_p != NULL);
        return;
    }

    BPM_LOCK_IRQ();

    BPM_BUF_POOL_PREFETCH_ST(bpm_pg->buf_cur_idx - 1);
    ++bpm_pg->buf_free_cnt;

    BPM_DBG_ASSERT(bpm_pg->buf_cur_idx != 0U);
    bpm_pg->buf_pool[--bpm_pg->buf_cur_idx] = buf_p; // PRE --

    BPM_UNLOCK_IRQ();
}

#define BPM_LOW_ALLOC_THRESH    (1024)

static int bpm_alloc_mult_buf_ex(uint32_t num, void **buf_p, uint32_t prio)
{
    int ret = BPM_SUCCESS;
    uint32_t expand_num = 0;
    uint32_t buf_avail_cnt, buf_idx;
    BPM_DEFS();
    BPM_FLAGS_DEF();
    BPM_PREFETCH();

    BCM_ASSERT(buf_p != NULL);

    BPM_LOCK_IRQ();

    BPM_BUF_POOL_PREFETCH_LD(bpm_pg->buf_cur_idx);

    buf_avail_cnt = BPM_BUF_AVAIL_CNT(bpm_pg); /* avail = top - cur */

    if (unlikely(((buf_avail_cnt < num) ||
                  ((buf_avail_cnt - num < BPM_LOW_ALLOC_THRESH) &&
                  (prio != BPM_HIGH_PRIO_ALLOC))) &&
                 (IS_BPM_ALLOC_REACHED_MAX()))) {
        /* We already allocated maximum number of BPM buffers */
        if (BPM_PRINT_FREQ(bpm_pg->buf_fail_cnt))
        {
            BCM_LOG_NOTICE(BCM_LOG_ID_BPM, "Failed to allocate %d buffers; buf_fail_cnt = %u",
                           num, bpm_pg->buf_fail_cnt);
        }
        bpm_pg->buf_fail_cnt++;
        ret = BPM_ERROR;
        goto exit_func;
    } else if (unlikely(buf_avail_cnt < num)) {
        // Allocate more buffers.
        if (num < (bpm_pg->dyn_cfg.bpm_max_buf_count - bpm_pg->buf_top_idx))
            expand_num = num;
        else
            expand_num = bpm_pg->dyn_cfg.bpm_max_buf_count - bpm_pg->buf_top_idx;

        bpm_pg->buf_inthr_alloc_cnt += bpm_buf_expand(expand_num);
    }
    buf_avail_cnt = BPM_BUF_AVAIL_CNT(bpm_pg); /* avail = top - cur */

    /* buffers available in global buffer pool */
    if (likely(buf_avail_cnt >= num)) {

        /* do we insert this in loop ...? */
        BPM_BUF_POOL_PREFETCH_LD(bpm_pg->buf_cur_idx +
            BCM_DCACHE_LINE_LEN / sizeof(void*));

        for (buf_idx = 0; buf_idx < num; ++buf_idx)
            buf_p[buf_idx] = bpm_pg->buf_pool[bpm_pg->buf_cur_idx++]; // POST ++
    
        bpm_pg->buf_alloc_cnt += num;
        update_min_availability();
    } else {
        if (BPM_PRINT_FREQ(bpm_pg->buf_fail_cnt))
        {
            BCM_LOG_NOTICE(BCM_LOG_ID_BPM,"Failed to allocate %d buffers; buf_fail_cnd = %u",
                           num, bpm_pg->buf_fail_cnt);
        }
        ++bpm_pg->buf_fail_cnt;
        bpm_pg->buf_alloc_cnt -= num;
        ret = BPM_ERROR;
    }

exit_func:
    BPM_UNLOCK_IRQ();

    return ret;
}

static void bpm_free_mult_buf(uint32_t num, void ** buf_p)
{
    uint32_t buf_idx;
    BPM_DEFS();
    BPM_FLAGS_DEF();
    BPM_PREFETCH();

    BCM_ASSERT(buf_p != NULL);

    BPM_LOCK_IRQ();

    BPM_BUF_POOL_PREFETCH_ST(bpm_pg->buf_cur_idx - 1);

    BPM_DBG_ASSERT(bpm_pg->buf_cur_idx >= num);

    bpm_pg->buf_free_cnt += num;

    if (likely(bpm_pg->buf_cur_idx >= num)) {
        for (buf_idx = 0; buf_idx < num; ++buf_idx) {
            bpm_pg->buf_pool[--bpm_pg->buf_cur_idx] = buf_p[buf_idx]; // PRE --
        }
    } else {
        BPM_DBG_ASSERT(bpm_pg->buf_cur_idx > num);
        bpm_pg->buf_free_cnt -= num;
    }

    BPM_UNLOCK_IRQ();
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_alloc_buf_mem
 * Description: Allocate a large memory chunk for carving out RX buffers
 *  The memory allocated is reset and flushed. A pointer to a cache aligned
 *  address of the requested size is returned. A pointer to the allocated
 *  memory is saved.
 *------------------------------------------------------------------------------
 */
static void *bpm_alloc_buf_mem(size_t memsz)
{
    BPM_DEFS();
    void *mem_p;

    BCM_LOG_FUNC(BCM_LOG_ID_BPM);

    memsz += L1_CACHE_BYTES;

    if (bpm_pg->mem_idx >= BPM_MAX_MEM_POOL_IX) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,
                "too many memory pools %d", bpm_pg->mem_idx);
        return NULL;
    }

#if defined(CONFIG_BCM94908) && defined(CONFIG_BCM_HND_EAP)
    /* Due to a RDP CPU_TX limitation that can only support <1GB address
     * space buffer, a workaround is implemented to support 1GB+ address
     * space buffer.  However, this workaround has a performance issue.
     * We address it by making sure BPM always allocate buffer from <1GB address
     * space by using GFP_DMA which is set to 1GB boundary, so drivers
     * using BPM buffer will not be affected by the workaround. */
    if ((mem_p = kmalloc(memsz, GFP_ATOMIC | GFP_DMA)) == NULL) {
#else
    if ((mem_p = kmalloc(memsz, GFP_ATOMIC)) == NULL) {
#endif
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "kmalloc %d failure", (int)memsz);
        return NULL;
    }

    /* Future kfree */
    bpm_pg->mem_pool[bpm_pg->mem_idx] = mem_p;
    bpm_pg->mem_idx++;

    memset(mem_p, 0, memsz);
    cache_flush_len(mem_p, memsz); /* Flush invalidate */

    mem_p = (void *)L1_CACHE_ALIGN((uintptr_t)mem_p);  /* L1 cache aligned */

    return mem_p;
}
static int bpm_mem_pool_buf_alloc(uint32_t tot_num_buf)
{
    void *mem_p;
    uint8_t *buf_p;
    uint32_t tot_mem_pool;
    uint32_t mem_pool_sz;
    uint32_t bufs_per_mem, memsz, i, num;
    void *mem_pool_p = NULL;
    uint8_t *base_p;   /* Start of a buffer where FkBuff_t resides */
    uint8_t *data_p;   /* Start of "data" buffer. Skip FkBuff, headroom */
    uintptr_t shinfo_p; /* Start of skb_shared_info in buffer */
    BPM_DEFS();

    tot_mem_pool = (tot_num_buf / (BPM_MAX_MEMSIZE/BPM_BUF_SIZE));
    tot_mem_pool += ((tot_num_buf % (BPM_MAX_MEMSIZE/BPM_BUF_SIZE))? 1 : 0);
    num = tot_num_buf;

    if (tot_mem_pool > BPM_MAX_MEM_POOL_IX) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,
                "too many memory pools %d", tot_mem_pool);
        return BPM_ERROR;
    }

    mem_pool_sz = tot_mem_pool * sizeof(void *);
    if ((mem_pool_p = kmalloc(mem_pool_sz, GFP_ATOMIC)) == NULL) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,
            "kmalloc %d failure for mem_pool_p", mem_pool_sz);
        return BPM_ERROR;
    }
    bpm_pg->mem_pool = (void **)mem_pool_p;

    bpm_pg->mem_idx = 0; /* Index into memory pools allocated */

    /* Allocate chunks of memory, carve buffers */
    bufs_per_mem = (BPM_MAX_MEMSIZE - L1_CACHE_BYTES) / BPM_BUF_SIZE;

    while (num) {
        /* Chunk size */
        bufs_per_mem = (bufs_per_mem < num) ? bufs_per_mem : num;
        memsz = bufs_per_mem * BPM_BUF_SIZE;

        if ((mem_p = bpm_alloc_buf_mem(memsz)) == NULL)
            return BPM_ERROR;

        buf_p = (uint8_t *)mem_p; /* Buffers are cached */

        for (i = 0; i < bufs_per_mem; i++, buf_p += BPM_BUF_SIZE) {
            /* L1 cache aligned */
            base_p = (uint8_t *)L1_CACHE_ALIGN((uintptr_t)buf_p);

            data_p = BPM_PFKBUFF_TO_BUF(base_p);

#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
            data_p += BPM_TRACKING_HDR;
            bpm_pg->last_memsz = memsz;
#endif

            /* skb_shared_info must be cacheline aligned */
            shinfo_p = (uintptr_t)BPM_BUF_TO_PSHINFO(data_p);

            BPM_DBG_ASSERT(IS_ALIGNED(shinfo_p, L1_CACHE_BYTES));

            /* Save the data_p and not the base_p in the buf_pool[] table */
            bpm_pg->buf_pool[bpm_pg->buf_top_idx++] = (void *)data_p;

            buf_p = base_p;
        }

        /* BCM_DBG_ASSERT that buf_p after loop <= (mem_p + memsz) */

        BCM_LOG_DEBUG(BCM_LOG_ID_BPM,
                      "allocated %4u %8s @ mem_p<%px> memsz<%06u>",
                      bufs_per_mem, "RxBufs", (void *)mem_p, memsz);

        num -= bufs_per_mem;

    }   /* while num */
    BCM_LOG_NOTICE(BCM_LOG_ID_BPM, "tot_mem_pool=%u mem_idx:%u",
                   tot_mem_pool, bpm_pg->mem_idx);
    WARN_ON(bpm_pg->mem_idx < tot_mem_pool);
    BUG_ON(bpm_pg->mem_idx > tot_mem_pool);

    return BPM_SUCCESS;
}
/*
 *------------------------------------------------------------------------------
 * Function   : bpm_init_buf_pool
 * Description: Initialize buffer pool, with num buffers
 *  Pointers to pre-allocated memory pools are saved. See bpm_alloc_buf_mem()
 *
 *  Buffers in buf_pool are managed in an inverted stack, with buf_cur_idx
 *  moving from 0 to buf_top_idx-1. buf_cur_idx will always point to the next
 *  buffer available for allocation. A buffer may be freed by decrementing the
 *  buf_cur_idx (stack grows down). A stack algo is used to reduce the cache
 *  working set and wraparound handling of a <head,tail> index walking the
 *  entire buf_pool with wraparound.
 *------------------------------------------------------------------------------
 */
static int bpm_init_buf_pool(uint32_t tot_num_buf)
{
    uint32_t num;
    BPM_DEFS();
    BPM_FLAGS_DEF();
    uint8_t *data_p;   /* Start of "data" buffer. Skip FkBuff, headroom */
    BPM_LOCK_IRQ();

    num = tot_num_buf;

    bpm_pg->buf_top_idx = 0;
    bpm_pg->min_avail_cnt = tot_num_buf;
    if (bpm_pg->dyn_cfg.bpm_max_buf_count == tot_num_buf)
        bpm_mem_pool_buf_alloc(tot_num_buf);
    else {
        while (num) {
            if ((data_p = bpm_alloc_buf_from_cache(bpm_pg)) == NULL) {
                BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Kmem_cache_alloc %d failed",
                              (int)num);
                return BPM_ERROR;
            }

            /* Save the data_p and not the base_p in the buf_pool[] table */
            bpm_pg->buf_pool[bpm_pg->buf_top_idx++] = (void *)data_p;

            num--;
        } /* while num */
    }
    BCM_LOG_NOTICE(BCM_LOG_ID_BPM, "buf_top_idx:%u", bpm_pg->buf_top_idx);

#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
    /* Set up static buffer pool */
    if (bpm_track_init_static_pool(bpm_pg->dyn_cfg.bpm_max_buf_count,
                                   bpm_pg->buf_top_idx, bpm_pg->buf_pool))
        return BPM_ERROR;
#endif

    BPM_DBG_ASSERT(bpm_pg->buf_top_idx != 0U);
    BPM_DBG_ASSERT(bpm_pg->buf_top_idx == tot_num_buf);

    /* Inverted Stack - buf_cur_idx moves from [0 .. buf_top_idx) */
    bpm_pg->buf_cur_idx = 0U;

#if !(defined(CONFIG_BCM_RDPA) && !defined(CONFIG_BCM_RDPA_MODULE))
#if defined(CONFIG_BCM_PKTDMA)
    bpm_pg->rxbds = bcmPktDma_GetTotRxBds();
#endif
#endif

    bpm_pg->tot_resv_buf = bpm_pg->rxbds;
    bpm_pg->max_dyn = bpm_pg->dyn_cfg.bpm_max_buf_count - bpm_pg->tot_resv_buf;
    bpm_pg->max_buf_wm = bpm_pg->buf_top_idx;

    bpm_upd_dyn_buf_lo_thresh();
    BPM_UNLOCK_IRQ();

    if ((bpm_pg->buf_top_idx - bpm_pg->buf_cur_idx) == 0)
        return BPM_ERROR;
    else
        return BPM_SUCCESS;
}

#ifdef BPM_TEST
/*
 *------------------------------------------------------------------------------
 * Function   : bpm_fini_buf_pool
 * Description: Releases all buffers of the pool
 *------------------------------------------------------------------------------
 */
static void bpm_fini_buf_pool(void)
{
    void *mem_p;
    uint32_t i;
    BPM_DEFS();
    BPM_FLAGS_DEF();

    BCM_LOG_FUNC(BCM_LOG_ID_BPM);

    BPM_LOCK_IRQ();
    /* Release all memory pools allocated */
    if (IS_BPM_DYN_ALLOC_MODE()) {
        while (bpm_pg->buf_cur_idx == bpm_pg->buf_top_idx) {
            mem_p = bpm_pg->buf_pool[--bpm_pg->buf_top_idx];
            if (mem_p) {
#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
                mem_p -= BPM_TRACKING_HDR;
#endif
                kmem_cache_free(bpm_pg->bpmbuff_cache,
                                PDATA_TO_PFKBUFF(mem_p, BCM_PKT_HEADROOM));
            }
        }
    } else {
        for (i = 0; i < bpm_pg->mem_idx; i++) {
            mem_p = (uint8_t *)bpm_pg->mem_pool[i];
            if (mem_p)
                kfree(mem_p);
        }
    }
#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
    bpm_track_disable();
    bpm_pg->last_memsz = 0;
#endif

    memset((void*)bpm_pg, 0, sizeof(bpm_t));
    BPM_UNLOCK_IRQ();

    return;
}
#endif

#if defined(CC_BPM_SKB_POOL_BUILD)
extern void skb_shinforeset(struct skb_shared_info *skb_shinfo);
extern struct sk_buff *skb_header_alloc(void);
extern void skb_header_free(struct sk_buff *skb);

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_free_skb
 * Description: Free a sk_buff into free pool
 *------------------------------------------------------------------------------
 */
static void bpm_free_skb(void *skbp)
{
    struct sk_buff *skb = (struct sk_buff *)skbp;
    BPM_DEFS();
    BPM_FLAGS_DEF();
    BPM_PREFETCH();

    /* Add skb to BPM sk_buff Pool free list */

    BPM_LOCK_IRQ();

    skb->next = bpm_pg->skb_freelist;
    bpm_pg->skb_freelist = skb;
    ++bpm_pg->skb_avail;

    BPM_SKB_POOL_STATS_ADD(skb_free_cnt, 1);

    BPM_UNLOCK_IRQ();
} /* bpm_free_skb() */

#if defined(CONFIG_BCM_BPM_BULK_FREE)
/*
 *------------------------------------------------------------------------------
 * Function   : bpm_free_mult_skb
 * Description: Free a list of sk_buff into free pool
 *------------------------------------------------------------------------------
 */
static void bpm_free_mult_skb(void *head, void *tail, uint32_t len)
{
    struct sk_buff *skb_head = (struct sk_buff *)head;
    struct sk_buff *skb_tail = (struct sk_buff *)tail;
    BPM_DEFS();
    BPM_FLAGS_DEF();
    BPM_PREFETCH();

    /* Add skb to BPM sk_buff Pool free list */

    BPM_LOCK_IRQ();

    skb_tail->next = bpm_pg->skb_freelist;
    bpm_pg->skb_freelist = skb_head;
    bpm_pg->skb_avail += len;

    BPM_SKB_POOL_STATS_ADD(skb_fast_free_cnt, len);

    BPM_UNLOCK_IRQ();

} /* bpm_free_mult_skb */
#endif /* CONFIG_BCM_BPM_BULK_FREE */

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_invalidate_dirtyp
 * Description: Invalidate dirtyp of a given skb and return
 *              the bufp associated with it
 *------------------------------------------------------------------------------
 */
static void* bpm_invalidate_dirtyp(void *p)
{
#if !defined(CONFIG_BCM_BPM_BULK_FREE)
    BPM_DBG_ASSERT(0);
    return NULL;
#else
    struct sk_buff *skb;
    void *buf_p = NULL;
    struct skb_shared_info *skb_shinfo;
#if !defined(CONFIG_BCM_GLB_COHERENCY)
    uint8_t *dirty_p = NULL;
#endif

    skb = (struct sk_buff *)p;
    BPM_DBG_ASSERT(skb != NULL);
    skb_shinfo = skb_shinfo(skb);
    BPM_DBG_ASSERT(skb->head != NULL);
    buf_p = (void*)BPM_PHEAD_TO_BUF(skb->head);

#if !defined(CONFIG_BCM_GLB_COHERENCY)
    dirty_p = (uint8_t*)skb_shinfo->dirty_p;
    if (dirty_p > (uint8_t *)buf_p) {
        int len = (uint8_t *)(dirty_p) - (uint8_t *)buf_p;
        len = len > BCM_MAX_PKT_LEN ? BCM_MAX_PKT_LEN : len;
        cache_invalidate_len(buf_p, len);
    }
#endif
    skb_shinfo->dirty_p = NULL;

    return (void *)buf_p;
#endif /* CONFIG_BCM_BPM_BULK_FREE */
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_free_skblist
 * Description: Free a list of skb into free pool
 *------------------------------------------------------------------------------
 */
static void bpm_free_skblist(void *head, void *tail, uint32_t len, void **bufp_arr)
{
#if !defined(CONFIG_BCM_BPM_BULK_FREE)
    BPM_DBG_ASSERT(0);
#else
    /* bulk free buf - single lock operation */
#if !defined(CONFIG_BCM_XRDP)
    bpm_free_mult_buf(len, bufp_arr);
#endif /* CONFIG_BCM_XRDP */
    /* bulk free skb */
    bpm_free_mult_skb(head, tail, len);

#endif /* CONFIG_BCM_BPM_BULK_FREE */
} /* bpm_free_skblist() */

/*
 *------------------------------------------------------------------------------
 * Function: bpm_recycle_skb
 * Description: RecycleFuncP hook registered with a skb allocated from the
 * BPM sk_buff pool. A BPM sk_buff, may ONLY be attached to a BPM data buffer.
 *
 * Recycle a data buf or sk_buff from the BPM preallocated buf or sk_buff pool
 *
 * Kernel may invoke the recycle function with recycle_flags either indicating
 * - that the attached BPM buffer needs to be returned into BPM "buf" pool.
 * - that the sk_buff itself needs to be returned to BPM "sk_buff" pool.
 *
 * An fkb_init() data buffer will be returned via:
 *      fkb_p->recycle_hook = bdmf_sysb_recycle
 *
 * An fkb may be nbuf_xlate() to a BPM skb (i.e. bpm_recycle_skb), while
 *    inheriting the original FkBuff_t:recycle_flags.
 *
 * The notion of recycle_context in FkBuff_t and sk_buff is INVALID:
 * - recycle_context is 32bit.
 * - FkBuff_t repurposed (union) dhd_pkttag_info_p. "repurpose = blatant hack"
 *
 *------------------------------------------------------------------------------
 */
static void bpm_recycle_skb(void *skbv, unsigned long context,
                            uint32_t recycle_action)
{
    struct sk_buff *skb = (struct sk_buff *)skbv;
    uint32_t need_reset;
    BPM_SKB_POOL_STATS_BPM_DEFS();

    /* SKB_DATA_RECYCLE action: recycle bpm buf */
    if (recycle_action & SKB_DATA_RECYCLE) {
        void *buf_p = (void*)BPM_PHEAD_TO_BUF(skb->head);
        struct skb_shared_info *skb_shinfo = skb_shinfo(skb);
#if !defined(CONFIG_BCM_GLB_COHERENCY)
        uint8_t *dirty_p = (uint8_t*)skb_shinfo->dirty_p;
#endif /* !CONFIG_BCM_GLB_COHERENCY */

        /* Audit a BPM skb attached to a BPM buf layout */
        BPM_DBG_ASSERT(BPM_BUF_TO_PSHINFO(buf_p) == skb_shinfo);

        /* User chose to clear SKB_BPM_PRISTINE, so full reset */
        if (unlikely((skb->recycle_flags & SKB_BPM_PRISTINE) == 0)) {
            skb_shinforeset(skb_shinfo); /* memset 0 and dataref = 1 */
            BPM_SKB_POOL_STATS_ADD(shinfo_reset_cnt, 1);
#if !defined(CONFIG_BCM_GLB_COHERENCY)
            cache_invalidate_len(buf_p, BCM_MAX_PKT_LEN);
            BPM_SKB_POOL_STATS_ADD(full_cache_inv_cnt, 1);
#endif /* !CONFIG_BCM_GLB_COHERENCY */
        }

#if !defined(CONFIG_BCM_GLB_COHERENCY)
        else if (dirty_p > (uint8_t *)buf_p) {
            int len = (uint8_t *)(dirty_p) - (uint8_t *)buf_p;
            len = len > BCM_MAX_PKT_LEN ? BCM_MAX_PKT_LEN : len;
            cache_invalidate_len(buf_p, len);
            BPM_SKB_POOL_STATS_ADD(part_cache_inv_cnt, 1);
        }
#endif /* !CONFIG_BCM_GLB_COHERENCY */

        /* else do nothing */

        skb_shinfo->dirty_p = NULL;

        bpm_free_buf(buf_p); /* buf_p = skb->head + BCM_PKT_HEADROOM */
        return;
    }

    /* SKB_RECYCLE action: recycle sk_buff to BPM sk_buff pool */
    if (recycle_action & SKB_RECYCLE) {
        /* Make skb pristine if tainted, i.e. user cleared SKB_BPM_PRISTINE */
        need_reset = skb->recycle_flags & SKB_BPM_PRISTINE ? 0 : 1;
        _bpm_pristine_skb(skb, need_reset);
        if (unlikely(need_reset))
            BPM_SKB_POOL_STATS_ADD(skb_hdr_reset_cnt, 1);
        bpm_free_skb(skb); /* Now free into BPM skb pool */
        return;
    }
    BPM_DBG_ASSERT(0);
}

/* Accessor function to return max number of skbs in pool */
static uint32_t bpm_total_skb(void)
{
    BPM_DEFS();
    BCM_LOG_INFO(BCM_LOG_ID_BPM,":%u\n",bpm_pg->skb_bound);
    return bpm_pg->skb_bound;
}

/* Accessor function to return avail number of skbs in free pool */
static uint32_t bpm_avail_skb(void)
{
    BPM_DEFS();
    BCM_LOG_INFO(BCM_LOG_ID_BPM,":%u\n",((bpm_pg->skb_bound - bpm_pg->skb_total) + bpm_pg->skb_avail));
    return ((bpm_pg->skb_bound - bpm_pg->skb_total) + bpm_pg->skb_avail);
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_alloc_skb
 * Description: Allocate a sk_buff from free pool
 *------------------------------------------------------------------------------
 */
static void *bpm_alloc_skb(void)
{
    struct sk_buff *skb;
    BPM_DEFS();
    BPM_FLAGS_DEF();
    BPM_PREFETCH();

    BPM_LOCK_IRQ();

    bcm_prefetch(bpm_pg->skb_freelist);

    skb = bpm_pg->skb_freelist;
    BPM_SKB_POOL_STATS_ADD(skb_alloc_cnt, 1);

    if (likely(bpm_pg->skb_avail)) {
        bpm_pg->skb_freelist = skb->next;
        skb->next = (struct sk_buff *)NULL;
        --bpm_pg->skb_avail;
        if (unlikely(bpm_pg->skb_avail <
                     bpm_pg->dyn_cfg.bpm_skb_avail_lower_th)) {
            /* schedule monitor thread to add skb to skb freelist. */
            bpm_skb_expand_post_event(bpm_pg);
        }
    } else {
        skb = NULL;
        BPM_SKB_POOL_STATS_ADD(skb_fail_cnt, 1);
        BPM_SKB_POOL_STATS_SUB(skb_alloc_cnt, 1);
    }

    BPM_UNLOCK_IRQ();

    /* bcm_prefetch(bpm_pg->skb_freelist); */
#ifdef CONFIG_TENDA_PRIVATE_KM
    km__alloc_skb_hook(skb);
#endif

    return (void *)skb;

} /* bpm_alloc_skb() */


/*
 *------------------------------------------------------------------------------
 * Function   : bpm_alloc_mult_skb
 * Description: Allocate mulitple sk_buff from free pool
 *------------------------------------------------------------------------------
 */
static void *bpm_alloc_mult_skb(uint32_t request_num)
{
    struct sk_buff *skb, *skb_list;
    uint32_t alloc;
    BPM_DEFS();
    BPM_FLAGS_DEF();
    BPM_PREFETCH();

    BPM_DBG_ASSERT(request_num != 0U);

    BPM_LOCK_IRQ();

    bcm_prefetch(bpm_pg->skb_freelist);
    BPM_SKB_POOL_STATS_ADD(skb_alloc_cnt, request_num);

    if (unlikely(bpm_pg->skb_avail < request_num)) {
        if ((bpm_pg->skb_total + request_num) <
             bpm_pg->dyn_cfg.bpm_max_skb_count) {
            /* Add to skb_freelist */
            BPM_UNLOCK_IRQ();
            bpm_pg->skb_inthr_alloc_cnt += bpm_grow_skb_pool(request_num);
            BPM_LOCK_IRQ();
        }
    }

    if (likely(bpm_pg->skb_avail >= request_num)) {
        skb = bpm_pg->skb_freelist;
        bcm_prefetch(skb->next);

        bpm_pg->skb_avail -= request_num;
        skb_list = skb; /* list to be returned */

        for (alloc = 1; alloc < request_num; ++alloc) {
            skb = skb->next;
            bcm_prefetch(skb->next);
        }

        bpm_pg->skb_freelist = skb->next;
        skb->next = (struct sk_buff *)NULL;
        if (unlikely(bpm_pg->skb_avail <
                    bpm_pg->dyn_cfg.bpm_skb_avail_lower_th))
            bpm_skb_expand_post_event(bpm_pg);
    } else {
        skb_list = NULL;
        BPM_SKB_POOL_STATS_SUB(skb_alloc_cnt, request_num);
        BPM_SKB_POOL_STATS_ADD(skb_fail_cnt, 1);
    }

    BPM_UNLOCK_IRQ();

    return (void *)skb_list;
} /* bpm_alloc_mult_skb() */

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_alloc_buf_attach_skb
 * Description: allocates a buffer, an SKB from global buffer pool
 *              and attaches the SKB to the buffer and return the SKB
 * NOTE: Cut-n-paste from bpm_alloc_buf, bpm_alloc_skb, bpm_attach_skb
 *       to avoid multiple lock/unlock, branch conditionals, prefetch, etc.
 *------------------------------------------------------------------------------
 */
static void *bpm_alloc_buf_skb_attach(uint32_t datalen)
{
    BPM_DEFS();
    BPM_FLAGS_DEF();
    void *buf;
    struct sk_buff *skb;
    struct skb_shared_info *skb_shinfo;

    BPM_PREFETCH();

    BPM_LOCK_IRQ();

    /* Allocate buffer : reference bpm_alloc_buf() */
    BPM_BUF_POOL_PREFETCH_LD(bpm_pg->buf_cur_idx);

    /* if buffers available in global buffer pool */
    if (likely(bpm_pg->buf_cur_idx < bpm_pg->buf_top_idx)) {
        buf = bpm_pg->buf_pool[bpm_pg->buf_cur_idx++]; // POST ++
        ++bpm_pg->buf_alloc_cnt;
        bcm_prefetch(bpm_pg->skb_freelist);

        update_min_availability();
    } else {
        buf = NULL;
        --bpm_pg->buf_alloc_cnt;
        ++bpm_pg->buf_fail_cnt;

        BPM_UNLOCK_IRQ();

        return (void *)NULL;
    }

    /* Allocate SKB : reference bpm_alloc_skb() */
    skb = bpm_pg->skb_freelist;
    BPM_SKB_POOL_STATS_ADD(skb_alloc_cnt, 1);
    BPM_SKB_POOL_STATS_ADD(skb_bpm_alloc_cnt, 1);

    if (likely(bpm_pg->skb_avail)) {
        bpm_pg->skb_freelist = skb->next;
        skb->next = (struct sk_buff *)NULL;
        --bpm_pg->skb_avail;
        if (unlikely(bpm_pg->skb_avail <
                     bpm_pg->dyn_cfg.bpm_skb_avail_lower_th))
            bpm_skb_expand_post_event(bpm_pg);
    } else {
        BPM_SKB_POOL_STATS_ADD(skb_fail_cnt, 1);
        BPM_SKB_POOL_STATS_SUB(skb_alloc_cnt, 1);
        BPM_SKB_POOL_STATS_SUB(skb_bpm_alloc_cnt, 1);

        BPM_UNLOCK_IRQ();

        bpm_free_buf(buf);

        return (void *)NULL;
    }

    BPM_UNLOCK_IRQ();

    /* bcm_prefetch(bpm_pg->skb_freelist); */

    /* Attach SKB to the buffer : reference bpm_attach_skb() */
    skb_shinfo = BPM_BUF_TO_PSHINFO(buf);
    bcm_prefetch(skb_shinfo);

    /* Ensure skb is not already contaminated */
    BPM_DBG_ASSERT((skb->recycle_flags & SKB_BPM_PRISTINE) == SKB_BPM_PRISTINE);
    BPM_DBG_ASSERT(skb_shinfo->dirty_p == NULL);

    /* All BPM allocated sk_buffs were in pristine state.  */

    skb->truesize = datalen + sizeof(struct sk_buff);

    _skb_zero(skb, SKB_CB_ZERO); /* prev, next, priority, mark, cb[], wl_cb[] */

    skb->head = BPM_BUF_TO_PHEAD(buf);
    skb->data = buf;

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
    km__alloc_skb_hook(skb);
#endif

    return (void *)skb;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_grow_skb_pool
 * Description: Extend the skb_pool with N buffers, or upto "bound".
 * This function is not reentrant safe.
 *------------------------------------------------------------------------------
 */
static int bpm_grow_skb_pool(uint32_t skb_grow)
{
    uint32_t skb_alloced;
    struct sk_buff *skb, *skb_head, *skb_tail;
    BPM_DEFS();
    BPM_FLAGS_DEF();

    BCM_LOG_FUNC(BCM_LOG_ID_BPM);

    BPM_LOCK_IRQ();

    /* Ensure pool does not get extended beyond configured bound */
    if ((bpm_pg->skb_total + skb_grow) > bpm_pg->skb_bound) {
        /* permissible extends */
        skb_grow = bpm_pg->skb_bound - bpm_pg->skb_total;

        if (skb_grow == 0U) { /* no more extensions, as reached bound */
            BPM_UNLOCK_IRQ();
            BCM_LOG_DEBUG(BCM_LOG_ID_BPM, "extend bound failure");
            BPM_SKB_POOL_STATS_ADD(skb_fail_cnt, 1);
            return BPM_ERROR;
        }
    }

    BPM_UNLOCK_IRQ();

    skb_alloced = 0U;
    skb_head = skb_tail = (struct sk_buff *)NULL;

    do { /* Allocate sk_buffs and place them into local list */
        skb = skb_header_alloc(); /* allocate sk_buff structure */

        if (!skb) {
            BCM_LOG_DEBUG(BCM_LOG_ID_BPM, "extend skb failure");
            BPM_SKB_POOL_STATS_ADD(skb_fail_cnt, 1);
            break; /* skbs_alloced may be 0 */
        }

        /* All sk_buffs in BPM pool are in a "pristine" state */
        _bpm_pristine_skb(skb, 1); /* need_reset = 1 */
        BPM_SKB_POOL_STATS_ADD(skb_hdr_reset_cnt, 1);

        if (skb_alloced) {
            skb->next = skb_head; /* uses skb::next for linking */
            skb_head = skb;
        } else {
            skb_head = skb;
            skb_tail = skb;
        }

    } while (++skb_alloced < skb_grow);

    if (skb_alloced) { /* Prepend local pool to BPM pool, with lock */
        BPM_LOCK_IRQ();

        /* Prepend list to bpm's sk_buff pool free list */
        skb_tail->next = bpm_pg->skb_freelist;
        bpm_pg->skb_freelist = skb_head;

        bpm_pg->skb_avail += skb_alloced;
        bpm_pg->skb_total += skb_alloced;

        BPM_SKB_POOL_STATS_ADD(skb_grow_cnt, 1);
        BPM_UNLOCK_IRQ();
    }

    BCM_LOG_DEBUG(BCM_LOG_ID_BPM,
        "skbs pool extended by %u total %u", skb_alloced, bpm_pg->skb_total);

    return skb_alloced;
}   /* bpm_grow_skb_pool() */



#ifdef BPM_TEST
/*
 *------------------------------------------------------------------------------
 * Function   : bpm_fini_skb_pool
 * Description: Release all sk_buffs in skb_freelist.
 *              For Use only in a test.
 *------------------------------------------------------------------------------
 */
static void bpm_fini_skb_pool(void)
{
    struct sk_buff *skb, *skb_freelist;
    uint32_t skb_errors, skb_avail, skb_total;
    BPM_DEFS();
    BPM_FLAGS_DEF();

    BCM_LOG_FUNC(BCM_LOG_ID_BPM);

    BPM_LOCK_IRQ();

    skb_freelist = bpm_pg->skb_freelist;
    skb_avail = bpm_pg->skb_avail;
    skb_total = bpm_pg->skb_total;

    bpm_pg->skb_total = 0U;
    bpm_pg->skb_bound = 0U;
    bpm_pg->skb_avail = 0U;
    bpm_pg->skb_freelist = (struct sk_buff *)NULL;

    BPM_UNLOCK_IRQ();

    /* sk_buffs allocated and not yet freed */
    skb_errors = skb_total - skb_avail;
    if (skb_errors)
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "fini skb_pool inuse %u", skb_errors);

    /* walk free list releasing sk_buffs back to kmem_cache skbuff_head_cache */
    while ((skb = skb_freelist) != (struct sk_buff *)NULL) {
        skb_freelist = skb->next;

        skb_header_free(skb); /* kmem_cache_free skbuff_head_cache */

        if (skb_avail)
            --skb_avail;
        else
            ++skb_errors; /* BPM internal error */
    }

    skb_errors += skb_avail; /* BPM internal error */

    if (skb_errors)
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "fini skb_pool errors %u", skb_errors);
}
#endif

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_init_skb_pool
 * Description: Initialize sk_buff pool, with  percent of the total buf_pool.
 * But when DYNAMIC_ALLOC is enabled we don't use the percent concept.
 * Preallocate a pool of sk_buff headers that have been preinitialized.
 * These sk_buff headers are not pointing to any data buffers.
 *
 * Assumes Kernel Network is inited (soc_init() --> skb_init() which will
 * setup the kmem_cache skbuff_head_cache, needed for skb_header_alloc()
 *
 *------------------------------------------------------------------------------
 */
static int bpm_init_skb_pool(uint32_t total_num_skb)
{
    struct sk_buff *skb;
    uint32_t skb_avail;
    BPM_DEFS();

    BCM_LOG_FUNC(BCM_LOG_ID_BPM);

    /* NO LOCK */

    bpm_pg->skb_freelist = NULL;

    for (skb_avail = 0U; skb_avail < total_num_skb; ++skb_avail) {
        skb = skb_header_alloc(); /* allocate sk_buff structure */
        if (!skb) {
            BCM_LOG_ERROR(BCM_LOG_ID_BPM, "skb pool init %u failure",
                total_num_skb - skb_avail);
            return BPM_ERROR;
        }

#ifdef CONFIG_TENDA_PRIVATE_KM
        km__alloc_skb_hook(skb);
#endif
        /* All sk_buffs in BPM pool are in a "pristine" state */
        _bpm_pristine_skb(skb, 1); /* need_reset = 1 */
        BPM_SKB_POOL_STATS_ADD(skb_hdr_reset_cnt, 1);

        skb->next = bpm_pg->skb_freelist;
        bpm_pg->skb_freelist = skb;
    }

    bpm_pg->skb_avail = bpm_pg->skb_total = skb_avail;
    bpm_pg->skb_bound = bpm_pg->dyn_cfg.bpm_max_buf_count;
    BCM_LOG_NOTICE(BCM_LOG_ID_BPM,"skbs pool inited total %u", skb_avail);

    return BPM_SUCCESS;
}   /* bpm_init_skb_pool() */
#endif /* CC_BPM_SKB_POOL_BUILD */

static int bpm_get_dyn_buf_lvl(void)
{
    BPM_DEFS();

    if (BPM_BUF_MAX_AVAIL_CNT(bpm_pg) > bpm_pg->dyn_buf_lo_thresh)
        return 1;
    return 0;
}

static uint32_t bpm_get_total_bufs(void)
{
    BPM_DEFS();
    BCM_LOG_INFO(BCM_LOG_ID_BPM,":%u\n",bpm_pg->dyn_cfg.bpm_max_buf_count);
    return bpm_pg->dyn_cfg.bpm_max_buf_count;
}


static uint32_t bpm_get_avail_bufs(void)
{
    BPM_DEFS();
    BCM_LOG_INFO(BCM_LOG_ID_BPM,":%u\n",BPM_BUF_MAX_AVAIL_CNT(bpm_pg));
    return BPM_BUF_MAX_AVAIL_CNT(bpm_pg); /* computed using max_dyn and cur idx */
}

static uint32_t bpm_get_max_dyn_bufs(void)
{
    BPM_DEFS();
    BCM_LOG_INFO(BCM_LOG_ID_BPM,":%u\n",bpm_pg->max_dyn);
    return bpm_pg->max_dyn;
}

#ifdef BPM_TEST
static void bpm_test(void)
{
    void *buf_p;

    bpm_init_buf_pool(896);
    buf_p = bpm_alloc_buf();
    bpm_free_buf(buf_p);
#if defined(CC_BPM_SKB_POOL_BUILD)
    bpm_init_skb_pool(896);
    /* bpm_alloc_buf_skb_attach(100); */
    bpm_fini_skb_pool();
#endif
    bpm_fini_buf_pool();
}
#endif

void bpm_dump_skbuffs(void)
{
#if defined(CC_BPM_SKB_POOL_BUILD)
    BPM_DEFS();
    BPM_FLAGS_DEF();

    BPM_LOCK_IRQ();
    printk("BPM Skbuff Pool: avail %u total %u bound %u\n",
           bpm_pg->skb_avail, bpm_pg->skb_total, bpm_pg->skb_bound);
#if defined(CC_BPM_SKB_POOL_STATS)
    printk("\tStats: alloc %u bpm_alloc %u free %u fast free %u error %u fail %u grow %u\n",
           bpm_pg->skb_alloc_cnt, bpm_pg->skb_bpm_alloc_cnt,
           bpm_pg->skb_free_cnt, bpm_pg->skb_fast_free_cnt, bpm_pg->skb_error_cnt,
           bpm_pg->skb_fail_cnt, bpm_pg->skb_grow_cnt);
    printk("\nBPM Reset Counters: \n--------------------\n"
           "skb_hdr_rst %u\tshinf_rst %u\tfull_cache_inv %u\t"
           "part_cache_inv %u \n",
           bpm_pg->skb_hdr_reset_cnt, bpm_pg->shinfo_reset_cnt,
           bpm_pg->full_cache_inv_cnt, bpm_pg->part_cache_inv_cnt);
#endif /* CC_BPM_SKB_POOL_STATS */
#else /* ! CC_BPM_SKB_POOL_BUILD */
    printk("BPM does not support SKB_POOL\n");
#endif /* CC_BPM_SKB_POOL_BUILD */
    BPM_UNLOCK_IRQ();
}


void bpm_dump_status(void)
{
    BPM_DEFS();
    if (IS_BPM_DYN_ALLOC_MODE()) {
        printk("\n------------------ BPM Config : Dynamic ----------------\n");
        printk("bpm_max_buf_count           = %u\n",
               bpm_pg->dyn_cfg.bpm_max_buf_count);
        printk("bpm_initial_buf_count       = %u\n",
               bpm_pg->dyn_cfg.bpm_initial_buf_count);
        printk("bpm_buf_avail_lower_th      = %u\n",
               bpm_pg->dyn_cfg.bpm_buf_avail_lower_th);
        printk("bpm_buf_expand_step_size    = %u\n",
               bpm_pg->dyn_cfg.bpm_buf_expand_step_size);
        printk("bpm_buf_shrink_step_size    = %u\n",
               bpm_pg->dyn_cfg.bpm_buf_shrink_step_size);
        printk("bpm_max_skb_count           = %u\n",
               bpm_pg->dyn_cfg.bpm_max_skb_count);
        printk("bpm_initial_skb_count       = %u\n",
               bpm_pg->dyn_cfg.bpm_initial_skb_count);
        printk("bpm_skb_avail_lower_th      = %u\n",
               bpm_pg->dyn_cfg.bpm_skb_avail_lower_th);
        printk("bpm_skb_expand_step_size    = %u\n",
               bpm_pg->dyn_cfg.bpm_skb_expand_step_size);
        printk("bpm_skb_shrink_step_size    = %u\n",
               bpm_pg->dyn_cfg.bpm_skb_shrink_step_size);
        printk("bpm_soak_timer_duration     = %u\n",
               bpm_pg->dyn_cfg.bpm_soak_timer_duration);
        printk("\n");
    }
#if !defined(CONFIG_BCM_BPM_DYNAMIC)
    else {
        printk("\n------------------- BPM Config : Static -----------------\n");
        printk("Memory Percent              = %u\n",
               CONFIG_BCM_BPM_BUF_MEM_PRCNT);
        printk("bpm_max_buf_count           = %u\n",
               bpm_pg->dyn_cfg.bpm_max_buf_count);
    }
#endif
    printk("\n----------------------- BPM Status --------------------------\n");
    printk("tot_buf                 = %u\n", bpm_pg->buf_top_idx);
    printk("avail                   = %u\n", BPM_BUF_AVAIL_CNT(bpm_pg));
    printk("cur                     = %u\n", bpm_pg->buf_cur_idx);
    printk("no_buf_err              = %u\n", bpm_pg->buf_fail_cnt);
    printk("cum_alloc               = %u\n", bpm_pg->buf_alloc_cnt);
    printk("cum_free                = %u\n", bpm_pg->buf_free_cnt);
    printk("min_availability        = %u\n", bpm_pg->min_avail_cnt);
    if (IS_BPM_DYN_ALLOC_MODE()) {
        printk("max_watermark           = %u\n", bpm_pg->max_buf_wm);
        printk("buf_expansion_count     = %u\n", bpm_pg->buf_expand_cnt);
        printk("buf_shrink_count        = %u\n", bpm_pg->buf_shrink_cnt);
        printk("buf_exp_fail_count      = %u\n", bpm_pg->buf_exp_fail_cnt);
        printk("buf_inthr_alloc_count   = %u\n", bpm_pg->buf_inthr_alloc_cnt);
        printk("skb_inthr_alloc_count   = %u\n", bpm_pg->skb_inthr_alloc_cnt);
    }
    printk("max_dyn                 = %u\n", bpm_pg->max_dyn);
    printk("tot_resv_buf            = %u\n", bpm_pg->tot_resv_buf);
    printk("rxbds                   = %u\n", bpm_pg->rxbds);

    printk("\n-------------------------------------------------------------\n");

    bpm_dump_skbuffs();

    return;
}


void bpm_dump_buffers(void)
{
    int buf_idx;
    BPM_DEFS();
    BPM_FLAGS_DEF();

    printk("\n----------------- Buffer Pool ----------------\n");
    printk("\n  Idx Address0 Address1 Address2 Address3");
    printk(" Address4 Address5 Address6 Address7\n");

    if (IS_BPM_DYN_ALLOC_MODE())
        buf_idx = bpm_pg->buf_cur_idx;
    else
        buf_idx = 0;

    /* WARNING:
     * 1. buf_top_idx may not be a multiple of 8.
     * 2. buffers may appear multiple times, in a dump.
     */
    for (; (buf_idx < bpm_pg->buf_top_idx); buf_idx += 8) {
        if ((buf_idx % 256) == 0)
            printk("\n");

        BPM_LOCK_IRQ();
        /* NOTE: Pointer to base (FkBuff_t) is dumped */
        printk("[%3u] %px %px %px %px %px %px %px %px\n", buf_idx,
               BPM_BUF_TO_PFKBUFF(bpm_pg->buf_pool[buf_idx + 0]),
               BPM_BUF_TO_PFKBUFF(bpm_pg->buf_pool[buf_idx + 1]),
               BPM_BUF_TO_PFKBUFF(bpm_pg->buf_pool[buf_idx + 2]),
               BPM_BUF_TO_PFKBUFF(bpm_pg->buf_pool[buf_idx + 3]),
               BPM_BUF_TO_PFKBUFF(bpm_pg->buf_pool[buf_idx + 4]),
               BPM_BUF_TO_PFKBUFF(bpm_pg->buf_pool[buf_idx + 5]),
               BPM_BUF_TO_PFKBUFF(bpm_pg->buf_pool[buf_idx + 6]),
               BPM_BUF_TO_PFKBUFF(bpm_pg->buf_pool[buf_idx + 7]));
        BPM_UNLOCK_IRQ();
    }

    printk("\n");
    return;
}

const char *strBpmPort[] =
{
    "ETH ",
    "XTM ",
    "FWD ",
    "WLAN",
    "USB ",
    "MAX "
};

void bpm_dump_thresh(void)
{
    gbpm_port_t port;
    uint32_t chnl;
    BPM_DEFS();

    printk("\n-------------------- BPM Thresh -------------------\n");
    printk("tot_buf tot_resv_buf max_dyn   avail dyn_buf_lo_thr\n");
    printk("%7u %12u %7u %7u %14u\n",
        bpm_pg->buf_top_idx, bpm_pg->tot_resv_buf, bpm_pg->max_dyn,
        BPM_BUF_AVAIL_CNT(bpm_pg), bpm_pg->dyn_buf_lo_thresh);
    printk("\n---------------------------------\n");
    printk("port chnl rx_ring_buf alloc_trig\n" );
    printk("---- ---- ----------- ----------\n");
    for (port = 0; port < GBPM_PORT_MAX; port++) {
        for (chnl = 0; chnl < GBPM_RXCHNL_MAX; chnl++) {
            if (bpm_pg->status[port][chnl] == GBPM_RXCHNL_ENABLED) {
                printk("%4s %4u %11u %10u\n", strBpmPort[port], chnl,
                    bpm_pg->num_rx_buf[port][chnl],
                    bpm_pg->alloc_trig_thresh[port][chnl]);
            }
        }
    }
    printk("\n");
}

/*
 *------------------------------------------------------------------------------
 * function   : bpm_calc_num_buf
 * description: Finds the total memory available on the board, and assigns
 * one-ourth of the total memory to the buffers. Finally calculates the
 * number of buffers to be allocated based on buffer memory and buffer size.
 *------------------------------------------------------------------------------
 */
int bpm_calc_num_buf(uint32_t prcnt)
{
    uint64_t temp_size, tot_mem_size = kerSysGetSdramSize();
    uint32_t buf_mem_size;

    temp_size = tot_mem_size;
    do_div(temp_size, 100);
    buf_mem_size = temp_size * prcnt;

#if !defined(CONFIG_BCM_BPM_DYNAMIC)
    /* Only print in static mode; Dynamic mode could call this function
     * multiple times */
    printk("BPM: tot_mem_size=%lldB (%lldMB), ", tot_mem_size, tot_mem_size>>20);
    printk("buf_mem_size <%d%%> =%dB (%dMB), ",
           prcnt, buf_mem_size, buf_mem_size>>20);
    printk("num of buffers=%d, buf size=%d\n",
           (unsigned int)(buf_mem_size / BPM_BUF_SIZE),
           (unsigned int)BPM_BUF_SIZE);
#endif

    return (buf_mem_size / BPM_BUF_SIZE);
}

/*
 * -----------------------------------------------------------------------------
 * function    : bpm_alloc_buf_from_cache
 * description : Allocates buffer from bpmbuff_cache of size BPM_BUF_SIZE.
 * These buffers are L1_CACHE_BYTES aligned and it is also invalidated.
 * -----------------------------------------------------------------------------
 */
static inline void *bpm_alloc_buf_from_cache(bpm_t *bpm_pg)
{
    uint8_t *base_p = NULL;
    uint8_t *data_p = NULL; /* Start of "data" buffer. Skip FkBuff, headroom */
    uintptr_t shinfo_p;

    // Buffer from bpmbuff_cache is L1_CACHE_BYTES aligned.
#if defined(CONFIG_BCM94908) && defined(CONFIG_BCM_HND_EAP)
    /* Due to a RDP CPU_TX limitation that can only support <1GB address
     * space buffer, a workaround is implemented to support 1GB+ address
     * space buffer.  However, this workaround has a performance issue.
     * We address it by making sure BPM always allocate buffer from <1GB address
     * space by using GFP_DMA which is set to 1GB boundary, so drivers
     * using BPM buffer will not be affected by the workaround. */
    if ((base_p = kmem_cache_alloc(bpm_pg->bpmbuff_cache,
                                   GFP_ATOMIC | GFP_DMA)) == NULL) {
#else
    if ((base_p = kmem_cache_alloc(bpm_pg->bpmbuff_cache, GFP_ATOMIC)) == NULL) {
#endif
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Kmem_cache_alloc failed");
    } else {
        //Clear shinfo area.
        data_p = BPM_PFKBUFF_TO_BUF(base_p);
#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
        data_p += BPM_TRACKING_HDR;
#endif
        shinfo_p = (uintptr_t)BPM_BUF_TO_PSHINFO(data_p);
        memset((void *)shinfo_p,0,BCM_SKB_SHAREDINFO);
        BPM_DBG_ASSERT(IS_ALIGNED(shinfo_p, L1_CACHE_BYTES));
        cache_flush_len(base_p,BPM_BUF_SIZE);
    }

    return data_p;
}

/*
 * -----------------------------------------------------------------------------
 * function    : bpm_buf_expand
 * description : Expands BPM allocated buffer from kernel by given number of
 * num_bufs. This function has to be called by holding the BPM_LOCK
 * ------------------------------------------------------------------------------
 */
static int bpm_buf_expand(uint32_t num_bufs)
{
    BPM_DEFS();
    uint32_t count;
    uint8_t *data_p = NULL; /* Start of "data" buffer. Skip FkBuff, headroom */

    for (count = 0; count < num_bufs; count++) {
        if (IS_BPM_ALLOC_REACHED_MAX()) {
            BCM_LOG_DEBUG(BCM_LOG_ID_BPM,
                          "Allocated maximum number of BPM buffers %u",
                          bpm_pg->buf_top_idx);
            break;
        }
        if ((data_p = bpm_alloc_buf_from_cache(bpm_pg)) == NULL) {
            BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Kmem_cache_alloc %d failed",
                          (int)num_bufs);
            break;
        }

        /* Save the data_p and not the base_p in the buf_pool[] table */
        bpm_pg->buf_pool[bpm_pg->buf_top_idx++] = (void *)data_p;
    }

    if (bpm_pg->max_buf_wm < bpm_pg->buf_top_idx)
        bpm_pg->max_buf_wm = bpm_pg->buf_top_idx;
    if (likely(count))
        bpm_pg->buf_expand_cnt++;
    if (unlikely(count < num_bufs))
    {
        /* Could not allocate requested number of buffers */
        if (BPM_PRINT_FREQ(bpm_pg->buf_exp_fail_cnt))
        {
            BCM_LOG_NOTICE(BCM_LOG_ID_BPM,
                          "Could not allocate requested buffers (%u:%u)",
                          count, bpm_pg->buf_top_idx);
        }
        bpm_pg->buf_exp_fail_cnt++;
    }
    BCM_LOG_DEBUG(BCM_LOG_ID_BPM, "Expanded by %4u %8s", count, "RxBufs");
    return count;
}

static inline void start_soak_timer(bpm_t *bpm_pg)
{
    if ((IS_BPM_DYN_ALLOC_MODE()) &&
        (!timer_pending(&bpm_pg->soak_timer)) &&
        (bpm_pg->buf_top_idx >= bpm_pg->dyn_cfg.bpm_initial_buf_count)) {
        bpm_pg->soak_timer.expires = jiffies +
                                     bpm_pg->dyn_cfg.bpm_soak_timer_duration;
        add_timer(&bpm_pg->soak_timer);
    }
}

static inline void bpm_buf_expand_post_event(bpm_t *bpm_pg)
{
    if (IS_BPM_DYN_ALLOC_MODE()) {
        monitor_task_wakeup |= BPM_BUF_EXPAND_EVENT;
        wake_up_interruptible(&monitor_task_wqh);
    }
}

static inline void bpm_skb_expand_post_event(bpm_t *bpm_pg)
{
    if (bpm_pg->dyn_cfg.bpm_initial_skb_count !=
        bpm_pg->dyn_cfg.bpm_max_skb_count) {
        monitor_task_wakeup |= BPM_SKB_EXPAND_EVENT;
        wake_up_interruptible(&monitor_task_wqh);
    }
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
static void bpm_soak_timer_handler(unsigned long arg)
#else
static void bpm_soak_timer_handler(struct timer_list *timer)
#endif
{
    monitor_task_wakeup |= BPM_SOAK_TIMER_EXPIRED;
    wake_up_interruptible(&monitor_task_wqh);
}

static int bpm_monitor_thread(void *data)
{
    BPM_DEFS();
    BPM_FLAGS_DEF();
    bpm_dyn_cfg_t *dyn_cfg = &bpm_pg->dyn_cfg;
    uint32_t wakeup_event;
    uint32_t avail_count = 0;
    uint32_t buffer_count = 0;
    int32_t buf_free_count = 0;
    int32_t skb_free_count = 0;
    uint32_t shrink_step = 0;
    struct sk_buff *free_skb = NULL;

    while (1) {
        buf_free_count = 0;
        skb_free_count = 0;
        wait_event_interruptible(monitor_task_wqh, monitor_task_wakeup);
        wakeup_event = monitor_task_wakeup;
        monitor_task_wakeup = 0;
        BCM_LOG_INFO(BCM_LOG_ID_BPM, "wakeup_event:%08x", wakeup_event);
        do {
            if (wakeup_event & (BPM_BUF_EXPAND_EVENT)) {
                BCM_LOG_INFO(BCM_LOG_ID_BPM,"BUF Expand Event");
                BPM_LOCK_IRQ();
                avail_count = BPM_BUF_AVAIL_CNT(bpm_pg);
                if (avail_count < dyn_cfg->bpm_buf_avail_lower_th) {
                    buffer_count = bpm_buf_expand(dyn_cfg->bpm_buf_expand_step_size);
                    if (unlikely(buffer_count != dyn_cfg->bpm_buf_expand_step_size))
                        BCM_LOG_DEBUG(BCM_LOG_ID_BPM,
                            "Failed to expand buffer pool. Added only %u "
                            "buffers", buffer_count);
                }
                avail_count = BPM_BUF_AVAIL_CNT(bpm_pg);
                BPM_UNLOCK_IRQ();
                if (avail_count >= dyn_cfg->bpm_buf_avail_lower_th)
                    wakeup_event &= ~BPM_BUF_EXPAND_EVENT;
            }

            if (wakeup_event & (BPM_SKB_EXPAND_EVENT)) {
                BCM_LOG_INFO(BCM_LOG_ID_BPM,"SKB Expand Event");
                bpm_grow_skb_pool(dyn_cfg->bpm_skb_expand_step_size);
                BPM_LOCK_IRQ();
                avail_count = BPM_SKB_AVAIL_CNT(bpm_pg);
                BPM_UNLOCK_IRQ();
                if (avail_count >= dyn_cfg->bpm_skb_avail_lower_th)
                    wakeup_event &= ~BPM_SKB_EXPAND_EVENT;
            }

            if (wakeup_event & (BPM_SOAK_TIMER_EXPIRED)) {
                BCM_LOG_INFO(BCM_LOG_ID_BPM,"Soak Timer Expired");
                //Shrink BPM buffers
                BPM_LOCK_IRQ();
                avail_count = BPM_BUF_AVAIL_CNT(bpm_pg);
                shrink_step = dyn_cfg->bpm_buf_shrink_step_size;
                // check if we can reduce available buffers
                if ((avail_count >= bpm_pg->last_buf_avail_snapshot) && 
                      ((bpm_pg->buf_top_idx - shrink_step) >= dyn_cfg->bpm_initial_buf_count)) {
                    // We can free shrink_step_size buffers back to kernel
                    for (buf_free_count = 0;
                         buf_free_count < shrink_step;
                         buf_free_count++) {
                        if ((bpm_pg->buf_top_idx - 1) > bpm_pg->buf_cur_idx)
                            bpm_pg->buffers_to_free[buf_free_count] =
                                bpm_pg->buf_pool[--bpm_pg->buf_top_idx];  //PRE
                        else
                            break;
                    }
                }
                bpm_pg->last_buf_avail_snapshot = BPM_BUF_AVAIL_CNT(bpm_pg);
                
                //Shrink SKB's
                shrink_step = bpm_pg->dyn_cfg.bpm_skb_shrink_step_size;
                if((bpm_pg->skb_total > bpm_pg->dyn_cfg.bpm_initial_skb_count) &&
                      (bpm_pg->skb_avail > shrink_step))
                {
                   //create a list of SKB that can be freed back to kernel.
                   for(skb_free_count=0;
                       skb_free_count < shrink_step;
                       skb_free_count++) {
                      free_skb = bpm_pg->skb_freelist;
                      if(free_skb != NULL) {
                         bpm_pg->skb_freelist = free_skb->next;
                         free_skb->next = bpm_pg->skbs_to_free;
                         bpm_pg->skbs_to_free = free_skb;
                         bpm_pg->skb_avail--;
                         bpm_pg->skb_total--;
                      }
                      else
                         break;
                   }
                }

                BPM_UNLOCK_IRQ();

                // Free the buffers here.
                BCM_LOG_INFO(BCM_LOG_ID_BPM, "Freeing %d BPM buffers",
                             buf_free_count);
                if (buf_free_count != 0)
                    bpm_pg->buf_shrink_cnt++;
                while (buf_free_count > 0) {
                    if (bpm_pg->buffers_to_free[--buf_free_count] != NULL) {
#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
                        bpm_pg->buffers_to_free[buf_free_count] -= BPM_TRACKING_HDR;
#endif
                        kmem_cache_free(bpm_pg->bpmbuff_cache,
                            PDATA_TO_PFKBUFF(bpm_pg->buffers_to_free[buf_free_count],
                                             BCM_PKT_HEADROOM));
                        bpm_pg->buffers_to_free[buf_free_count] = NULL;
                    }
                }

                //Free the skbs here
                BCM_LOG_INFO(BCM_LOG_ID_BPM, "Freeing %d skb",
                             skb_free_count);
                while (bpm_pg->skbs_to_free != (struct sk_buff *)NULL) {
                    free_skb = bpm_pg->skbs_to_free;
                    bpm_pg->skbs_to_free = free_skb->next;
                    free_skb->recycle_hook = NULL;
                    free_skb->recycle_flags = 0;
                    skb_header_free(free_skb);
                }

                wakeup_event &= ~BPM_SOAK_TIMER_EXPIRED;
            }
            if (wakeup_event)
                yield();
        } while(wakeup_event);
        BPM_LOCK_IRQ();
        avail_count = BPM_BUF_AVAIL_CNT(bpm_pg);
        if ((avail_count > (dyn_cfg->bpm_buf_avail_lower_th +
                            dyn_cfg->bpm_buf_shrink_step_size)))
            start_soak_timer(bpm_pg);

        BPM_UNLOCK_IRQ();
    }
    return 0;
}

static int check_bpm_configs(void)
{
    BPM_DEFS();
    bpm_dyn_cfg_t *dyn_cfg = &bpm_pg->dyn_cfg;
    int ret = BPM_SUCCESS;

    if (!IS_BPM_DYN_ALLOC_MODE())
        return BPM_SUCCESS;

   if ((dyn_cfg->bpm_max_buf_count == 0) ||
       (dyn_cfg->bpm_max_buf_count < dyn_cfg->bpm_initial_buf_count)) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_max_buf_count:%u",
                      dyn_cfg->bpm_max_buf_count);
        ret = BPM_ERROR;
        goto config_check_exit;
   }

   if (dyn_cfg->bpm_initial_buf_count == 0) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_initial_buf_count:%u",
                      dyn_cfg->bpm_initial_buf_count);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

    if ((dyn_cfg->bpm_buf_avail_lower_th == 0)) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_buf_avail_lower_th:%u",
                      dyn_cfg->bpm_buf_avail_lower_th);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

    if ((dyn_cfg->bpm_buf_expand_step_size == 0) ||
        (dyn_cfg->bpm_buf_expand_step_size < dyn_cfg->bpm_buf_shrink_step_size)) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_buf_expand_step_size:%u",
                      dyn_cfg->bpm_buf_expand_step_size);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

   if (dyn_cfg->bpm_buf_shrink_step_size == 0) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_buf_shrink_step_size:%u",
                      dyn_cfg->bpm_buf_shrink_step_size);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

    if ((dyn_cfg->bpm_max_skb_count == 0) ||
        (dyn_cfg->bpm_max_skb_count < dyn_cfg->bpm_initial_skb_count)) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_max_skb_count:%u",
                      dyn_cfg->bpm_max_skb_count);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

    if (dyn_cfg->bpm_initial_skb_count == 0) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_initial_skb_count:%u",
                      dyn_cfg->bpm_initial_skb_count);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

    if ((dyn_cfg->bpm_skb_avail_lower_th == 0)) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_skb_avail_lower_th:%u",
                      dyn_cfg->bpm_skb_avail_lower_th);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

    if ((dyn_cfg->bpm_skb_expand_step_size == 0) ||
        (dyn_cfg->bpm_skb_expand_step_size < dyn_cfg->bpm_skb_shrink_step_size)) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_skb_expand_step_size:%u",
                      dyn_cfg->bpm_skb_expand_step_size);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

    if (dyn_cfg->bpm_skb_shrink_step_size == 0) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_skb_shrink_step_size:%u",
                      dyn_cfg->bpm_skb_shrink_step_size);
        ret = BPM_ERROR;
        goto config_check_exit;
    }

    if (dyn_cfg->bpm_soak_timer_duration == 0) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Incorrect bpm_soak_timer_duration:%u",
                      dyn_cfg->bpm_soak_timer_duration);
        ret = BPM_ERROR;
    }

config_check_exit:
    return ret;
}

#if defined(CONFIG_BCM_BPM_DYNAMIC)
/* NOTE: Below function must not change value_p NVRAM content is not present
 * or invalid */
static int bpm_get_nvram_value(char *key_name, unsigned int *value_p)
{
    char key_value[PSP_BUFLEN_16] = {};
    unsigned int value;

    if (kerSysScratchPadGet(key_name, (char*)key_value,
                            (int)sizeof(key_value)) > 0) {
        if (kstrtouint(key_value, 10, &value) == 0)
            *value_p = value;
        else {
            BCM_LOG_ERROR(BCM_LOG_ID_BPM,
                          "Error converting the stored string for %s to uint\n",
                          key_name);
            return BPM_ERROR;
        }
    }

    return BPM_SUCCESS;
}
#endif

static int get_bpm_dynamic_configs(void)
{
    BPM_DEFS();
    bpm_dyn_cfg_t *dyn_cfg = &bpm_pg->dyn_cfg;
    unsigned int value;
    int ret = BPM_ERROR; /* Start with ERROR and rely on check_bpm_configs() to set */

#if !defined(CONFIG_BCM_BPM_DYNAMIC)
    value = CONFIG_BCM_BPM_BUF_MEM_PRCNT;
    dyn_cfg->bpm_max_buf_count = bpm_calc_num_buf(value);
    dyn_cfg->bpm_initial_buf_count = dyn_cfg->bpm_max_buf_count;
#else /* Dynamic BPM Buffer pool */

#if defined(CONFIG_BCM_BPM_DYNAMIC_TYPE_PRCNT)
    value = CONFIG_BCM_BPM_DYNAMIC_PRCNT_MAX_BUF;
    if (bpm_get_nvram_value(BPM_MAX_PRCNT_BUFFER_PSP_KEY, &value))
        goto config_exit;
    dyn_cfg->bpm_max_buf_count = bpm_calc_num_buf(value);

    value = CONFIG_BCM_BPM_DYNAMIC_PRCNT_INIT_BUF;
    if (bpm_get_nvram_value(BPM_INITIAL_PRCNT_BUFFER_PSP_KEY, &value))
        goto config_exit;
    dyn_cfg->bpm_initial_buf_count = bpm_calc_num_buf(value);
#else
    value = CONFIG_BCM_BPM_DYNAMIC_ABS_MAX_BUF;
    if (bpm_get_nvram_value(BPM_MAX_NUM_BUFFER_PSP_KEY, &value))
        goto config_exit;
    dyn_cfg->bpm_max_buf_count = value;

    value = CONFIG_BCM_BPM_DYNAMIC_ABS_INIT_BUF;
    if (bpm_get_nvram_value(BPM_INITIAL_NUM_BUFFER_PSP_KEY, &value))
        goto config_exit;
    dyn_cfg->bpm_initial_buf_count = value;
#endif

    value = CONFIG_BCM_BPM_DYNAMIC_AVAIL_LOW_TH;
    if (bpm_get_nvram_value(BPM_BUF_AVAIL_LOWER_THRESHOLD_PSP_KEY, &value))
        goto config_exit;
    dyn_cfg->bpm_buf_avail_lower_th = value;

    value = CONFIG_BCM_BPM_DYNAMIC_EXPAND_STEP_SIZE;
    if (bpm_get_nvram_value(BPM_BUF_EXPAND_STEP_PSP_KEY, &value))
        goto config_exit;
    dyn_cfg->bpm_buf_expand_step_size = value;

    value = CONFIG_BCM_BPM_DYNAMIC_SHRINK_STEP_SIZE;
    if (bpm_get_nvram_value(BPM_BUF_SHRINK_STEP_PSP_KEY, &value))
        goto config_exit;
    dyn_cfg->bpm_buf_shrink_step_size = value;

    value = BPM_SOAK_TIMER_DURATION;
    if (bpm_get_nvram_value(BPM_SOAK_TIMER_DURATION_PSP_KEY, &value))
        goto config_exit;
    dyn_cfg->bpm_soak_timer_duration = value;
#endif

    dyn_cfg->bpm_max_skb_count        = dyn_cfg->bpm_max_buf_count;
    dyn_cfg->bpm_initial_skb_count    = dyn_cfg->bpm_initial_buf_count;
    dyn_cfg->bpm_skb_avail_lower_th   = dyn_cfg->bpm_buf_avail_lower_th;
    dyn_cfg->bpm_skb_expand_step_size = dyn_cfg->bpm_buf_expand_step_size;
    dyn_cfg->bpm_skb_shrink_step_size = dyn_cfg->bpm_buf_shrink_step_size;

    ret = check_bpm_configs();

#if defined(CONFIG_BCM_BPM_DYNAMIC)
config_exit:
#endif
    return ret;
}

int __init bpm_pool_manager_init(void)
{
    uint32_t tot_num_buf;
    BPM_DEFS();
    uint32_t buf_pool_sz;
    void *buf_pool_p = NULL;
    uint32_t gbpm_debug = 1;

    bcmLog_setLogLevel(BCM_LOG_ID_BPM, BCM_LOG_LEVEL_NOTICE);

    memset((void*)bpm_pg, 0, sizeof(bpm_t));

    if (get_bpm_dynamic_configs() == BPM_ERROR) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,
                      "BPM module failed to get its configurations");
        return BPM_ERROR;
    }
    tot_num_buf = bpm_pg->dyn_cfg.bpm_max_buf_count;
    buf_pool_sz = tot_num_buf * sizeof(void *);

    BCM_LOG_DEBUG(BCM_LOG_ID_BPM, "%s: bpm_pg<0x%px>", __FUNCTION__, bpm_pg);


    spin_lock_init(&bpm_pg->lock);
    /* Create kmem_cache */
    bpm_pg->bpmbuff_cache = kmem_cache_create("bpmbuf", BPM_BUF_SIZE,
                                              L1_CACHE_BYTES, SLAB_HWCACHE_ALIGN, NULL);
    if (bpm_pg->bpmbuff_cache == NULL) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,"Failed to create bpmbuff kmem cache");
        return BPM_ERROR;
    }

    if ((buf_pool_p = kmalloc(buf_pool_sz, GFP_ATOMIC)) == NULL) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM,
                      "kmalloc %d failure for buf_pool_p", buf_pool_sz);
        return BPM_ERROR;
    }

    bpm_pg->buf_pool = (void **)buf_pool_p;
    tot_num_buf = bpm_pg->dyn_cfg.bpm_initial_buf_count;

    if (bpm_init_buf_pool(tot_num_buf) == BPM_ERROR) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "init buf_pool %u failure", tot_num_buf);
        return BPM_ERROR;
    }

#if defined(CC_BPM_SKB_POOL_BUILD)
    /* Preinitialize a pool of sk_buff(s), as a percent of tot_num_buf */
    if (bpm_init_skb_pool(bpm_pg->dyn_cfg.bpm_initial_skb_count) == BPM_ERROR) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "init skb_pool failure");
        return BPM_ERROR;
    }
#endif /* CC_BPM_SKB_POOL_BUILD */

    /* Initialize memory for storing the list of buffers that will be freed */
    bpm_pg->buffers_to_free = kmalloc(bpm_pg->dyn_cfg.bpm_buf_shrink_step_size *
                                      sizeof(void *), GFP_ATOMIC);
    if (bpm_pg->buffers_to_free == NULL) {
        BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Kmalloc failure for dynamic shrink "
                                      "buffers_to_free");
        return BPM_ERROR;
    }

    /* Initialize Soak Timer */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
    init_timer(&bpm_pg->soak_timer);
    bpm_pg->soak_timer.function = bpm_soak_timer_handler;
    bpm_pg->soak_timer.data = (unsigned long)bpm_pg;
#else
    timer_setup(&bpm_pg->soak_timer, bpm_soak_timer_handler, 0);
#endif

    /* Initialize the BPM monitor thread */
    init_waitqueue_head(&monitor_task_wqh);
    monitor_task = kthread_create(bpm_monitor_thread, NULL, "bpm_monitor");
    wake_up_process(monitor_task);

#undef GBPM_DECL
#define GBPM_DECL(HOOKNAME)   bpm_ ## HOOKNAME,
    gbpm_bind(GBPM_BIND() gbpm_debug);

    return 0;
}

void bpm_pool_manager_exit(void)
{
#if 0
/*
 * need to see how we can properly exit the pool manager, as the allocated
 * buffers can still be in used at the time of exiting.  Need to figure out
 * something.  (Or we don't even need to worry about performing exit)
 */
    BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Cannot remove BPM Module!!!");

        gbpm_unbind();
#if defined(CC_BPM_SKB_POOL_BUILD)
        bpm_fini_skb_pool();
#endif
        bpm_fini_buf_pool();
    BCM_LOG_NOTICE(BCM_LOG_ID_BPM, "BPM Module Exit");
    bpm_drv_destruct();
#endif
}

