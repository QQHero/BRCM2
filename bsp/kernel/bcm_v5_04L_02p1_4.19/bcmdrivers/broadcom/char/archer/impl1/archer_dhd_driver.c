/*
* <:copyright-BRCM:2021:proprietary:standard
* 
*    Copyright (c) 2021 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

/*
*******************************************************************************
*
* File Name  : archer_dhd_driver.c
*
* Description: Archer DHD Driver
*
*******************************************************************************
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/bcm_log.h>
#include <linux/nbuff.h>
#include <linux/bcm_skb_defines.h>
#include "bcm_pkt_lengths.h"
#include "bcm_mm.h"

#include "sysport_rsb.h"
#include "sysport_classifier.h"

#include "sysport_driver.h"

#include "archer.h"
#include "archer_driver.h"
#include "archer_thread.h"
#include "archer_dhd.h"

#define RDPA_DHD_HELPER_FEATURE_HWA_WAKEUP_SUPPORT

#if defined(CC_ARCHER_DHD_DEBUG)
uint32_t archer_dhd_debug_g = 64;
#endif

archer_dhd_t archer_dhd_g;

static void archer_dhd_thread_create(uint32_t radio_idx);

/* Initialize DHD helper object */
int rdd_dhd_hlp_cfg(uint32_t radio_idx, rdpa_dhd_init_cfg_t *init_cfg, int enable)
{       
    uint32_t rw_index_sz;
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];
    archer_dhd_post_common_t *post_common_radio_ptr = &radio_ptr->post;
    archer_dhd_complete_common_t *complete_common_radio_ptr = &radio_ptr->complete;
    int i;

    __logDebug("radio_idx = %d, init_cfg = %px, enable = %d\n", radio_idx, init_cfg, enable);
    
    if(radio_idx >= ARCHER_DHD_RADIO_MAX)
    {
        __logError("Invalid radio_idx: %u", radio_idx);

        return -1;
    }

    if(init_cfg->flow_ring_format != FR_FORMAT_WI_CWI32)
    {
        __logError("Flow Ring Format %d is not supported", init_cfg->flow_ring_format);

        return -1;
    }

    if(!enable)
    {
        __logError("Cannot disable radio_idx: %u", radio_idx);

        return 0;
    }

    archer_dhd_thread_create(radio_idx);

#if defined(CC_ARCHER_DHD_AGGREGATION)
    /* rdd_dhd_helper_aggregation_bypass_cpu_tx_set(radio_idx, 1); */
    /* rdd_dhd_helper_aggregation_bypass_non_udp_tcp_set(radio_idx, 1); */
    /* rdd_dhd_helper_aggregation_bypass_tcp_pktlen_set(radio_idx, 64); */
#endif

    complete_common_radio_ptr->rx_post_fr_base_ptr = init_cfg->rx_post_flow_ring_base_addr;

    complete_common_radio_ptr->rx_complete_fr_base_ptr = init_cfg->rx_complete_flow_ring_base_addr;

    complete_common_radio_ptr->tx_complete_fr_base_ptr = init_cfg->tx_complete_flow_ring_base_addr;

    if(init_cfg->hbqd_mode == 0)
    {
        rw_index_sz = sizeof(uint16_t);
    }
    else
    {
        rw_index_sz = sizeof(uint32_t);
    }
    
    complete_common_radio_ptr->rx_post_fr_wr_idx_ptr = (volatile uint16_t *)
        ((uintptr_t)init_cfg->r2d_wr_arr_base_addr + rw_index_sz);
    
    complete_common_radio_ptr->rx_post_fr_rd_idx_ptr = (volatile uint16_t *)
        ((uintptr_t)init_cfg->r2d_rd_arr_base_addr + rw_index_sz);

    post_common_radio_ptr->tx_post_fr_wr_idx_base_ptr = init_cfg->r2d_wr_arr_base_addr;
    
    post_common_radio_ptr->tx_post_fr_rd_idx_base_ptr = init_cfg->r2d_rd_arr_base_addr;
    
    complete_common_radio_ptr->rx_complete_fr_wr_idx_ptr = (volatile uint16_t *)
        ((uintptr_t)init_cfg->d2r_wr_arr_base_addr + rw_index_sz);
    
    complete_common_radio_ptr->rx_complete_fr_rd_idx_ptr = (volatile uint16_t *)
        ((uintptr_t)init_cfg->d2r_rd_arr_base_addr + rw_index_sz);
    
    complete_common_radio_ptr->tx_complete_fr_wr_idx_ptr = (volatile uint16_t *)init_cfg->d2r_wr_arr_base_addr;
    
    complete_common_radio_ptr->tx_complete_fr_rd_idx_ptr = (volatile uint16_t *)init_cfg->d2r_rd_arr_base_addr;

    // FIXME: Initialized twice?
    complete_common_radio_ptr->rx_post_wr_idx = swap16(DHD_RX_POST_FLOW_RING_SIZE - 1);

    post_common_radio_ptr->dhd_doorbell_ptr = (volatile archer_dhd_doorbell_t *)
        init_cfg->dongle_wakeup_register_virt;
    
#ifdef RDPA_DHD_HELPER_FEATURE_HWA_WAKEUP_SUPPORT
    post_common_radio_ptr->idma_active = init_cfg->dongle_wakeup_hwa;

    complete_common_radio_ptr->idma_active = init_cfg->dongle_wakeup_hwa;

    complete_common_radio_ptr->dhd_doorbell_ptr = (volatile archer_dhd_doorbell_t *)
        init_cfg->dongle_wakeup_register_2_virt;
#else
    post_common_radio_ptr->idma_active = 0;

    complete_common_radio_ptr->idma_active = 0;

    complete_common_radio_ptr->dhd_doorbell_ptr = init_cfg->dongle_wakeup_register;
#endif

    complete_common_radio_ptr->tx_complete_rd_idx = 0;
    complete_common_radio_ptr->tx_complete_wr_idx = 0;
    complete_common_radio_ptr->rx_complete_rd_idx = 0;
    complete_common_radio_ptr->rx_complete_wr_idx = 0;

    post_common_radio_ptr->add_llcsnap_header = init_cfg->add_llcsnap_header;

    post_common_radio_ptr->flow_ring_format = init_cfg->flow_ring_format;
    complete_common_radio_ptr->flow_ring_format = init_cfg->flow_ring_format;

    radio_ptr->flring_cache = (rdpa_dhd_flring_cache_t *)init_cfg->tx_post_mgmt_arr_base_addr;

    radio_ptr->nbr_of_flow_rings = init_cfg->tx_post_mgmt_arr_entry_count;

    for (i = 0; i < ARCHER_DHD_FLOW_RING_GROUPS_MAX; i++)
    {
        archer_dhd_doorbell_t *doorbell_ptr = &post_common_radio_ptr->tx_post_doorbell_value[i];
        uint32_t frg_id = (i < 15 ? i+1 : 15);

        *doorbell_ptr = (DMA_TYPE_IDMA << DMA_TYPE_SHIFT |
                         frg_id << FRG_ID_SHIFT);
    }
    
    if (init_cfg->hbqd_mode == 0)
    {
        post_common_radio_ptr->idma_last_group_fr = 14*32;
        post_common_radio_ptr->idma_group_shift = 5;
        post_common_radio_ptr->fr_ptrs_size_shift = 1;

        complete_common_radio_ptr->idma_last_group_fr = 14*32;
        complete_common_radio_ptr->idma_group_shift = 5;
        complete_common_radio_ptr->fr_ptrs_size_shift = 1;
    }
    else
    {
        post_common_radio_ptr->idma_last_group_fr = 14*16;
        post_common_radio_ptr->idma_group_shift = 4;
        post_common_radio_ptr->fr_ptrs_size_shift = 2;

        complete_common_radio_ptr->idma_last_group_fr = 14*16;
        complete_common_radio_ptr->idma_group_shift = 4;
        complete_common_radio_ptr->fr_ptrs_size_shift = 2;
    }

    complete_common_radio_ptr->rx_complete_ring_size = DHD_RX_COMPLETE_FLOW_RING_SIZE;
    complete_common_radio_ptr->tx_complete_ring_size = DHD_TX_COMPLETE_FLOW_RING_SIZE;
    complete_common_radio_ptr->rx_post_ring_size = DHD_RX_POST_FLOW_RING_SIZE;

    return 0;
}

typedef spinlock_t bdmf_fastlock;

#define DEFINE_BDMF_FASTLOCK(lock) bdmf_fastlock lock = __SPIN_LOCK_INITIALIZER(lock)

#define bdmf_fastlock_init(plock)               \
    do {                                        \
        spin_lock_init(plock);                  \
    } while(0)

#define bdmf_fastlock_lock(plock)               \
    do {                                        \
        BUG_ON(in_irq());                       \
        spin_lock_bh(plock);                    \
    } while (0)

#define bdmf_fastlock_unlock(plock)             \
    spin_unlock_bh(plock)

DEFINE_BDMF_FASTLOCK(wlan_ucast_lock);

#define CC_ARCHER_DHD_BUFFER_TABLE_ERROR_CHECK
#define ARCHER_DHD_INDEX_POOL_SIZE   (1 << 15)

#define index_pool_ptr (&archer_dhd_g.buffer_table.pool)

#define archer_dhd_index_pool_full() !archer_dhd_index_pool_free_entries()

#define archer_dhd_index_pool_empty()                                   \
    ( index_pool_ptr->write == index_pool_ptr->read )

static inline uint16_t archer_dhd_index_pool_level(void)
{
    return (index_pool_ptr->write - index_pool_ptr->read);
}

static inline uint16_t archer_dhd_index_pool_free_entries(void)
{
    return ARCHER_DHD_INDEX_POOL_SIZE - archer_dhd_index_pool_level();
}

static inline archer_dhd_free_index_t archer_dhd_index_pool_read(void)
{
    int read_index = index_pool_ptr->read++ & (ARCHER_DHD_INDEX_POOL_SIZE - 1);
    volatile archer_dhd_free_index_t *read_p = &index_pool_ptr->alloc_p[read_index];

    return *read_p;
}

static inline void archer_dhd_index_pool_write(archer_dhd_free_index_t entry)
{
    int write_index = index_pool_ptr->write++ & (ARCHER_DHD_INDEX_POOL_SIZE - 1);
    volatile archer_dhd_free_index_t *write_p = &index_pool_ptr->alloc_p[write_index];

    *write_p = entry;
}

static int archer_dhd_index_pool_construct(void)
{
    index_pool_ptr->alloc_p = (volatile archer_dhd_free_index_t *)
        archer_mem_kzalloc(sizeof(archer_dhd_free_index_t) * ARCHER_DHD_INDEX_POOL_SIZE);

    if(index_pool_ptr->alloc_p == NULL)
    {
        __logError("Could not archer_mem_kzalloc");

        return -1;
    }

    index_pool_ptr->write = 0;
    index_pool_ptr->read = 0;

    return 0;
}

static int archer_dhd_buffer_table_construct(void)
{
    archer_dhd_free_index_t free_index;
    int ret;

    archer_dhd_g.buffer_table.table = (pNBuff_t)
        archer_mem_kzalloc(sizeof(pNBuff_t) * ARCHER_DHD_INDEX_POOL_SIZE);

    if(archer_dhd_g.buffer_table.table == NULL)
    {
        __logError("Could not archer_mem_kzalloc");

        return -1;
    }

    ret = archer_dhd_index_pool_construct();
    if(ret)
    {
        archer_mem_kfree(archer_dhd_g.buffer_table.table);

        return ret;
    }

    for(free_index=0; free_index<ARCHER_DHD_INDEX_POOL_SIZE; ++free_index)
    {
        BCM_ASSERT(!archer_dhd_index_pool_full());

        archer_dhd_index_pool_write(free_index);
    }

    BCM_ASSERT(archer_dhd_index_pool_full());

    return 0;
}

#define archer_dhd_buffer_table_not_full()  ( !archer_dhd_index_pool_empty() )

static inline archer_dhd_free_index_t archer_dhd_buffer_table_write(pNBuff_t pNBuff)
{
    archer_dhd_free_index_t free_index;
    pNBuff_t *table_ptr;

#if defined(CC_ARCHER_DHD_BUFFER_TABLE_ERROR_CHECK)
    if(unlikely(archer_dhd_index_pool_empty()))
    {
        __logError("Free Index Underflow");
    }
#endif

    free_index = archer_dhd_index_pool_read();

    table_ptr = &archer_dhd_g.buffer_table.table[free_index];

#if defined(CC_ARCHER_DHD_BUFFER_TABLE_ERROR_CHECK)
    if(unlikely(*table_ptr))
    {
        __logError("Double write: free_index %d", free_index);
    }
#endif

    *table_ptr = pNBuff;

    archer_dhd_g.buffer_table.writes++;

    return free_index;
}

int archer_dhd_buffer_table_try_write(pNBuff_t pNBuff, archer_dhd_free_index_t *free_index_p)
{
    int ret = -1;

    bdmf_fastlock_lock(&wlan_ucast_lock);

    if(likely(archer_dhd_buffer_table_not_full()))
    {
        *free_index_p = archer_dhd_buffer_table_write(pNBuff);

        ret = 0;
    }

    bdmf_fastlock_unlock(&wlan_ucast_lock);

    return ret;
}

static inline pNBuff_t archer_dhd_buffer_table_read(archer_dhd_free_index_t free_index)
{
    pNBuff_t *table_ptr = &archer_dhd_g.buffer_table.table[free_index];
    pNBuff_t pNBuff = *table_ptr;

#if defined(CC_ARCHER_DHD_BUFFER_TABLE_ERROR_CHECK)
    if(unlikely(free_index >= ARCHER_DHD_INDEX_POOL_SIZE))
    {
        __logError("free_index >= ARCHER_DHD_INDEX_POOL_SIZE");

        return NULL;
    }

    if(unlikely(archer_dhd_index_pool_full()))
    {
        __logError("Free Index Overflow: free_index %d", free_index);

        return NULL;
    }

    if(unlikely(*table_ptr == NULL))
    {
        __logError("Double Read: free_index %d", free_index);

        return NULL;
    }

    *table_ptr = NULL;
#endif

    archer_dhd_index_pool_write(free_index);

    archer_dhd_g.buffer_table.reads++;

    return pNBuff;
}

/* Allocate FPM tokens and fills RxPost FlowRing */
int rdp_drv_rx_post_init(uint32_t radio_idx, uint32_t num_items)
{
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];
    archer_dhd_complete_common_t *complete_common_radio_ptr = &radio_ptr->complete;
    uintptr_t write_ptr;
    int i;

    __logDebug("radio_idx = %d\n", radio_idx);

    if(radio_idx >= ARCHER_DHD_RADIO_MAX)
    {
        __logError("Invalid radio_idx: %u", radio_idx);

        return -1;
    }

    for (i = 0, write_ptr = (uintptr_t)complete_common_radio_ptr->rx_post_fr_base_ptr; i < num_items; i++)
    {
        void *buffer_ptr;
        uintptr_t buffer_phys_addr;

        buffer_ptr = archer_dhd_buffer_alloc();
        if(!buffer_ptr)
        {
            __logError("Could not archer_dhd_buffer_alloc");

            return -1;
        }

        if(i < 64)
        {
            archer_dhd_debug("RX_POST[%d]: 0x%px", i, buffer_ptr);
        }

        buffer_phys_addr = ARCHER_DHD_PHYS_ADDR(buffer_ptr);

        BCM_ASSERT(complete_common_radio_ptr->flow_ring_format == FR_FORMAT_WI_CWI32);

        {
            hwa_rxpost_cwi32_t *rx_post_descr_cwi32_ptr = 
                (hwa_rxpost_cwi32_t *)write_ptr;

            rx_post_descr_cwi32_ptr->host_pktid = (uintptr_t)buffer_ptr;
            rx_post_descr_cwi32_ptr->data_buf_haddr32 = swap32(buffer_phys_addr);

            write_ptr += sizeof(hwa_rxpost_cwi32_t);
        }
    }

    *complete_common_radio_ptr->rx_post_fr_rd_idx_ptr = 0;
    *complete_common_radio_ptr->rx_post_fr_wr_idx_ptr = num_items;
    complete_common_radio_ptr->rx_post_wr_idx = num_items;

    wmb();

//    bcm_print("\tArcher DHD: Rx Post Ring Initialized (radio %d, format %d, 0x%px, %d entries)\n",
//              radio_idx, complete_common_radio_ptr->flow_ring_format, complete_common_radio_ptr->rx_post_fr_base_ptr, num_items);

    return 0;
}


/* Frees FPM tokens from RxPost FlowRing */
int rdp_drv_dhd_rx_post_uninit(uint32_t radio_idx, uint32_t *num_items)
{
    void *buffer_ptr;
    uint16_t start, end;
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];
    archer_dhd_complete_common_t *complete_common_radio_ptr = &radio_ptr->complete;

    __logDebug("radio_idx = %d\n", radio_idx);

    if(radio_idx >= ARCHER_DHD_RADIO_MAX)
    {
        __logError("Invalid radio_idx: %u", radio_idx);

        return -1;
    }

    /* the logic behind this is, there should always be
     * (DHD_RX_POST_FLOW_RING_SIZE - 1) buffers in RxPost ring, because Runner
     * allocates 1 back when it receives 1 in RxComplete.  (Wr_idx - 1) should
     * represent the last refilled buffer, and WRAP(wr_idx + 1) should be
     * the oldest refilled buffer in RxPost.  Therefore, we will free by
     * going from wr_idx + 1, toward wr_idx + 2, and on until it wraps
     * around and gets to (wr_idx - 1) */

    /*  When function called dongle not supposed to send rxcomplete packets anymore
        But runner could process last ones. Temporary add delay to avoid race between runner and host*/
    mdelay(100);

    rmb();

    /* end = *ring_info->wr_idx_addr; */
    end = *complete_common_radio_ptr->rx_post_fr_wr_idx_ptr;
    *num_items = 0;

    start = (end + 1) & (DHD_RX_POST_FLOW_RING_SIZE - 1);

    BCM_ASSERT(complete_common_radio_ptr->flow_ring_format == FR_FORMAT_WI_CWI32);

    do {
        hwa_rxpost_cwi32_t *rx_post_descr_cwi32_ptr = 
            (hwa_rxpost_cwi32_t *)complete_common_radio_ptr->rx_post_fr_base_ptr;

        buffer_ptr = (void *)rx_post_descr_cwi32_ptr[start].host_pktid;

        archer_dhd_buffer_free((uint8_t *)buffer_ptr);

        (*num_items)++;

        start++;
        if (unlikely(start == DHD_RX_POST_FLOW_RING_SIZE))
            start = 0;

    } while (start != end);

    return 0;
}

/* Refills FPM tokens from RxPost FlowRing */
int rdp_drv_dhd_rx_post_reinit(uint32_t radio_idx)
{
    uint32_t num_items;
    int rc;

    /* First phase - just empty and refill the ring */
    rc = rdp_drv_dhd_rx_post_uninit(radio_idx, &num_items);

    rc = rc ? rc: rdp_drv_rx_post_init(radio_idx, num_items);

    return 0;
}

/* Sends message to runner via QM */
static int rdp_drv_dhd_cpu_tx_send_message(uint32_t message_type, uint32_t radio_idx, uint32_t read_idx_flow_ring_idx)
{
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];
    bcm_async_queue_t *queue_ptr = &radio_ptr->cpu_msg.queue;
    rdpa_dhd_ffd_data_t params = (rdpa_dhd_ffd_data_t)read_idx_flow_ring_idx;
    int timeout;

    bdmf_fastlock_lock(&wlan_ucast_lock);

    timeout = 1000;

    while(--timeout)
    {
        if(likely(bcm_async_queue_not_full(queue_ptr)))
        {
            archer_dhd_cpu_msg_queue_t *entry_p = (archer_dhd_cpu_msg_queue_t *)
                bcm_async_queue_entry_write(queue_ptr);

            WRITE_ONCE(entry_p->type, message_type);
            WRITE_ONCE(entry_p->radio_idx, radio_idx);
            WRITE_ONCE(entry_p->read_idx_valid, params.read_idx_valid);
            WRITE_ONCE(entry_p->read_idx, params.read_idx);
            WRITE_ONCE(entry_p->flow_ring_id, params.flowring_idx);

            bcm_async_queue_entry_enqueue(queue_ptr);

            ARCHER_DHD_STATS_INCR(queue_ptr->stats.writes);

            archer_dhd_cpu_msg_task_schedule(radio_ptr);

            break;
        }

        udelay(100);
    }

    if(!timeout)
    {
        ARCHER_DHD_STATS_INCR(queue_ptr->stats.discards);

        __logError("Could not send CPU Message: message_type=%d, radio_idx=%d, flow_ring_idx=%d\n",
                   message_type, radio_idx, params.flowring_idx);

        bdmf_fastlock_unlock(&wlan_ucast_lock);

        return -1;
    }

    // Wait for any Archer active threads finish running

    timeout = 1000;

    while(--timeout && bcm_async_queue_not_empty(queue_ptr))
    {
        udelay(100);
    }

    bdmf_fastlock_unlock(&wlan_ucast_lock);

    return 0;
}

/* sending "flush" rings message */
int rdp_drv_dhd_helper_flow_ring_flush(uint32_t radio_idx, uint32_t read_idx_flow_ring_id)
{
    rdpa_dhd_ffd_data_t  params = (rdpa_dhd_ffd_data_t)read_idx_flow_ring_id;

    __logDebug("radio_idx=%d, flow_ring_idx=%d, read_idx=%d, read_idx_valid=%d\n", radio_idx, params.flowring_idx, params.read_idx, params.read_idx_valid);

    return rdp_drv_dhd_cpu_tx_send_message(DHD_MSG_TYPE_FLOW_RING_FLUSH, radio_idx, read_idx_flow_ring_id);
}

/* sending "ring disable" rings message */
int rdp_drv_dhd_helper_flow_ring_enable_set(uint32_t radio_idx, uint32_t flow_ring_idx, int enable)
{
    __logDebug("radio_idx=%d, flow_ring_idx=%d, enable=%d\n", radio_idx, flow_ring_idx, enable);

    if (enable)
    {
        return rdp_drv_dhd_cpu_tx_send_message(DHD_MSG_TYPE_FLOW_RING_SET_ENABLED, radio_idx, flow_ring_idx);
    }

    return rdp_drv_dhd_cpu_tx_send_message(DHD_MSG_TYPE_FLOW_RING_SET_DISABLED, radio_idx, flow_ring_idx);
}

/* Creates CPU TX complete ring and propagates ring descriptor to runnner */
int rdp_drv_dhd_helper_dhd_complete_ring_create(uint32_t radio_idx, uint32_t ring_size)
{
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];
    int entry_size;
    int ret;

    if(radio_idx >= ARCHER_DHD_RADIO_MAX)
    {
        __logError("Invalid radio_idx: %u", radio_idx);

        return -1;
    }

    entry_size = ((sizeof(archer_dhd_tx_complete_queue_t) + 3) & ~3); // 32-bit aligned

    ret = bcm_async_queue_init(&radio_ptr->tx_complete.queue, ring_size, entry_size);
    if(ret)
    {
        __logError("Could not bcm_async_queue_init");

        return -1;
    }

    ARCHER_TASK_INIT(&radio_ptr->tx_complete.task, ARCHER_THREAD_ID_US,
                     archer_dhd_tx_complete_task_handler, radio_ptr);

    bcm_print("Create DHD Complete Ring: radio_idx %d, ring_size %d, entry_size %d\n",
              radio_idx, ring_size, entry_size);

    return 0;
}

/* frees allocated ring and resets descriptor in SRAM */
int rdp_drv_dhd_helper_dhd_complete_ring_destroy(uint32_t radio_idx, uint32_t ring_size)
{
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];

    if(radio_idx >= ARCHER_DHD_RADIO_MAX)
    {
        __logError("Invalid radio_idx: %u", radio_idx);

        return -1;
    }

    bcm_print("Destroy DHD Complete Ring: radio_idx %d\n", radio_idx);

    bcm_async_queue_uninit(&radio_ptr->tx_complete.queue);

    return 0;
}

/* Reads number of dropped packets for given ssid/radio pair */
uint32_t rdp_drv_dhd_helper_ssid_tx_dropped_packets_get(uint32_t radio_idx, uint32_t ssid)
{
    /* uint32_t cntr_arr[MAX_NUM_OF_COUNTERS_PER_READ] = {}; */
    /* uint32_t cntr_id; */

    /* RDD_BTRACE("radio_idx = %d, ssid = %d\n", radio_idx, ssid); */

    /* cntr_id = DHD_CNTR_DHD_TX_DROP_0_SSID_0 + radio_idx*16 + ssid; */
    /* drv_cntr_counter_read(CNTR_GROUP_DHD_CNTRS, cntr_id, cntr_arr); */
    /* return cntr_arr[0]; */

    // FIXME

    return 0;
}

int rdp_drv_dhd_cpu_tx(const rdpa_dhd_tx_post_info_t *info, void *buffer, uint32_t pkt_length)
{
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[info->radio_idx];
    archer_dhd_post_common_t *post_common_radio_ptr = &radio_ptr->post;
    bcm_async_queue_t *queue_ptr = &radio_ptr->tx_post.queue;
    uint8_t *data_ptr;
    int wifi_priority;

    if(IS_FKBUFF_PTR(buffer))
    {
        FkBuff_t *fkb = PNBUFF_2_FKBUFF(buffer);

        data_ptr = fkb->data;

        wifi_priority = fkb->wl.ucast.dhd.wl_prio;
    }
    else
    {
        struct sk_buff *skb = PNBUFF_2_SKBUFF(buffer);

        data_ptr = skb->data;

        wifi_priority = LINUX_GET_PRIO_MARK(skb->mark);
    }

    archer_dhd_debug("\nCPU Tx: SSID %d, FlowRing ID %d, Radio %d\n",
                     info->ssid_if_idx, info->flow_ring_id, info->radio_idx);
    archer_dhd_packet_dump(data, pkt_length, 0);

    if(post_common_radio_ptr->add_llcsnap_header)
    {
        /* LLCSNAP insertion */
        data_ptr = archer_dhd_insert_llcsnap_header(data_ptr, &pkt_length);
    }

    bdmf_fastlock_lock(&wlan_ucast_lock);

    if(likely(bcm_async_queue_not_full(queue_ptr) &&
              archer_dhd_buffer_table_not_full()))
    {
        archer_dhd_tx_post_queue_t *entry_p = (archer_dhd_tx_post_queue_t *)
            bcm_async_queue_entry_write(queue_ptr);
        archer_dhd_free_index_t free_index =
            archer_dhd_buffer_table_write(buffer);
        archer_dhd_request_id_t request_id = { .word_32[0] = 0 };

        request_id.free_index = free_index;
        request_id.drop = 0;
        request_id.buffer_type = DHD_TX_POST_HOST_BUFFER_VALUE;

        WRITE_ONCE(entry_p->data_ptr, data_ptr);
        WRITE_ONCE(entry_p->request_id.word_32[0], request_id.word_32[0]);
        WRITE_ONCE(entry_p->flow_ring_id, info->flow_ring_id);
        WRITE_ONCE(entry_p->data_len, pkt_length);
        WRITE_ONCE(entry_p->priority, wifi_priority);

        bcm_async_queue_entry_enqueue(queue_ptr);

        ARCHER_DHD_STATS_INCR(queue_ptr->stats.writes);

        archer_dhd_tx_post_task_schedule(radio_ptr);

        bdmf_fastlock_unlock(&wlan_ucast_lock);

        return 0;
    }
    else
    {
        ARCHER_DHD_STATS_INCR(queue_ptr->stats.discards);

        bdmf_fastlock_unlock(&wlan_ucast_lock);

        return BDMF_ERR_IO;
    }
}

int rdp_drv_dhd_helper_dhd_complete_message_get(rdpa_dhd_complete_data_t *dhd_complete_info)
{
    int rc = BDMF_ERR_OK;
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[dhd_complete_info->radio_idx];
    bcm_async_queue_t *queue_ptr = &radio_ptr->tx_complete.queue;
    archer_dhd_tx_complete_queue_t entry;
    uint32_t drop_ind = 1;
    uint8_t buf_type = 0;
    void *txp = NULL;

    bdmf_fastlock_lock(&wlan_ucast_lock);

    while(bcm_async_queue_not_empty(queue_ptr) && drop_ind)
    {
        archer_dhd_tx_complete_queue_t *entry_ptr = (archer_dhd_tx_complete_queue_t *)
            bcm_async_queue_entry_read(queue_ptr);

        entry.u64 = READ_ONCE(entry_ptr->u64);

        bcm_async_queue_entry_dequeue(queue_ptr);

        ARCHER_DHD_STATS_INCR(queue_ptr->stats.reads);

        buf_type = entry.request_id.buffer_type;

        if(buf_type == DHD_TX_POST_HOST_BUFFER_VALUE)
        {
            /* It is a buffer from offloaded ring - release an index and pass the ptr to DHD */
            archer_dhd_free_index_t free_index = entry.request_id.free_index;

            drop_ind = entry.drop || entry.request_id.drop;

            txp = archer_dhd_buffer_table_read(free_index);

            if(drop_ind && txp)
            {
                nbuff_free(txp);
            }
        }
        else
        {
            txp = NULL;
            drop_ind = 0;
        }
    }

    if(drop_ind)
    {
        rc = BDMF_ERR_ALREADY;
    }
    else
    {
        /* Set the return parameters. */
        dhd_complete_info->request_id = entry.request_id.word_32[0];
        dhd_complete_info->buf_type = buf_type;
        dhd_complete_info->txp = txp;
        dhd_complete_info->status = entry.status;
        dhd_complete_info->flow_ring_id = entry.flow_ring_id;
    }

    bdmf_fastlock_unlock(&wlan_ucast_lock);

    return rc;
}

int rdp_drv_dhd_cpu_rx(uint32_t radio_idx, rdpa_cpu_rx_info_t *info)
{
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];
    bcm_async_queue_t *queue_ptr = &radio_ptr->rx_complete.queue;

    if(likely(bcm_async_queue_not_empty(queue_ptr)))
    {
        archer_dhd_rx_complete_queue_t *entry_ptr = (archer_dhd_rx_complete_queue_t *)
            bcm_async_queue_entry_read(queue_ptr);

        info->data = READ_ONCE(entry_ptr->data);
        info->data_offset = READ_ONCE(entry_ptr->data_offset);
        info->size = READ_ONCE(entry_ptr->size);
        info->dest_ssid = READ_ONCE(entry_ptr->dest_ssid);

        bcm_async_queue_entry_dequeue(queue_ptr);

        ARCHER_DHD_STATS_INCR(queue_ptr->stats.reads);

        return 0;
    }

    return -1;
}

#if 1
void rdp_drv_dhd_complete_wakeup(uint32_t radio_idx, int is_tx_complete)
{
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];

    if(is_tx_complete)
    {
        archer_dhd_tx_complete_task_schedule(radio_ptr);
    }
    else
    {
        archer_dhd_rx_complete_task_schedule(radio_ptr);
    }
}
#else
void rdp_drv_dhd_complete_wakeup(uint32_t radio_idx, int is_tx_complete)
{
}
#endif

#if defined(CC_SYSPORT_DRIVER_USER_IRQ)
static FN_HANDLER_RT archer_dhd_isr(int irq, void *param)
{
    uint32_t irq_status = sysport_driver_user_irq_status();

    while(irq_status)
    {
        int irq_source = ffs(irq_status) - 1;
        int radio_idx = (irq_source >> 1);

        irq_status &= ~(1 << irq_source);

        if(likely(radio_idx < ARCHER_DHD_RADIO_MAX))
        {
            archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];

            if(likely(ARCHER_DHD_RADIO_IS_ENABLED(radio_ptr)))
            {
                int is_tx_complete = (irq_source & 1);

                sysport_driver_user_irq_disable(irq_source);

                if(is_tx_complete)
                {
#if 1
                    archer_dhd_tx_complete_task_schedule(radio_ptr);
#else
                    sysport_driver_user_irq_clear(irq_source);
                    sysport_driver_user_irq_enable(irq_source);
                    bcm_print("***************** DHD TX IRQ %d *****************\n\n", irq_source);
#endif
                }
                else
                {
#if 1
                    archer_dhd_rx_complete_task_schedule(radio_ptr);
#else
                    sysport_driver_user_irq_clear(irq_source);
                    sysport_driver_user_irq_enable(irq_source);
                    bcm_print("***************** DHD RX IRQ %d *****************\n\n", irq_source);
#endif
                }

                continue;
            }
        }

        __logError("Unexpected IRQ (%d): irq_status 0x%08x, irq_source %d",
                   irq, irq_status, irq_source);
    }

    return BCM_IRQ_HANDLED;
}

void rdp_drv_dhd_helper_wakeup_information_get(rdpa_dhd_wakeup_info_t *wakeup_info)
{
    int rx_complete_source = (wakeup_info->radio_idx << 1);
    int rx_complete_value = (1 << rx_complete_source);
    int tx_complete_source = (rx_complete_source + 1);
    int tx_complete_value = (1 << tx_complete_source);

    wakeup_info->rx_complete_wakeup_register = archer_dhd_g.irq_set_phys_addr;
    wakeup_info->rx_complete_wakeup_value = rx_complete_value;
    sysport_driver_user_irq_clear(rx_complete_source);
    sysport_driver_user_irq_enable(rx_complete_source);

    wakeup_info->tx_complete_wakeup_register = archer_dhd_g.irq_set_phys_addr;
    wakeup_info->tx_complete_wakeup_value = tx_complete_value;
    sysport_driver_user_irq_clear(tx_complete_source);
    sysport_driver_user_irq_enable(tx_complete_source);

    bcm_print("wakeup_information: radio_idx %d, reg 0x%08x, rx 0x%x(%d), tx 0x%x(%d)\n",
              wakeup_info->radio_idx, wakeup_info->rx_complete_wakeup_register,
              wakeup_info->rx_complete_wakeup_value, rx_complete_source,
              wakeup_info->tx_complete_wakeup_value, tx_complete_source);
}
#else
void rdp_drv_dhd_helper_wakeup_information_get(rdpa_dhd_wakeup_info_t *wakeup_info) { }
#endif

void archer_dhd_recycle(pNBuff_t pNBuff, unsigned long context, uint32_t flags)
{
    if(IS_FKBUFF_PTR(pNBuff))
    {
        FkBuff_t *fkb = PNBUFF_2_FKBUFF(pNBuff);
        uint8_t *data_ptr = PFKBUFF_TO_PDATA(fkb, BCM_PKT_HEADROOM);

        archer_dhd_buffer_free(data_ptr);
    }
    else /* skb */
    {
        struct sk_buff *skb = PNBUFF_2_SKBUFF(pNBuff);

        if(likely(SKB_DATA_RECYCLE & flags))
        {
            FkBuff_t *fkb = PDATA_TO_PFKBUFF(skb->head, 0);
            uint8_t *data_ptr = PFKBUFF_TO_PDATA(fkb, BCM_PKT_HEADROOM);

            archer_dhd_buffer_free(data_ptr);
        }
        else
        {
            __logError("Only DATA recycle is supported: skb 0x%px", skb);
        }
    }
}

#if defined(CC_ARCHER_DHD_STATS)
static void archer_dhd_cpu_queue_dump(bcm_async_queue_t *queue_p, char *name, int index)
{
    bcm_print("%s[%d]: Level %u/%u, Writes %u, Reads %u, Discards %u, Writes+Discards %u\n", name, index,
              bcm_async_queue_avail_entries(queue_p), queue_p->depth,
              queue_p->stats.writes, queue_p->stats.reads, queue_p->stats.discards,
              queue_p->stats.writes + queue_p->stats.discards);
}
#else
static void archer_dhd_cpu_queue_dump(bcm_async_queue_t *queue_p, char *name, int index)
{
    bcm_print("%s[%d]: Depth %d, Level %d, Write %d, Read %d\n", name, index,
              queue_p->depth, queue_p->depth - bcm_async_queue_free_entries(queue_p),
              queue_p->write, queue_p->read);
}
#endif

void archer_dhd_stats(void)
{
    int buffer_table_in_use = 0;
    int radio_idx;

    for(radio_idx=0; radio_idx<ARCHER_DHD_RADIO_MAX; ++radio_idx)
    {
        archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];

        if(ARCHER_DHD_RADIO_IS_ENABLED(radio_ptr))
        {
            archer_dhd_complete_common_t *complete_common_radio_ptr = &radio_ptr->complete;

            archer_dhd_cpu_queue_dump(&radio_ptr->rx_complete.queue, "DHD_RXCMPL", radio_idx);

            if(complete_common_radio_ptr->tx_complete_fr_base_ptr)
            {
                archer_dhd_cpu_queue_dump(&radio_ptr->tx_post.queue, "DHD_TXPOST", radio_idx);
                archer_dhd_cpu_queue_dump(&radio_ptr->tx_complete.queue, "DHD_TXCMPL", radio_idx);
                archer_dhd_cpu_queue_dump(&radio_ptr->cpu_msg.queue, "DHD_CPUMSG", radio_idx);
            }

            bcm_print("DHD_RXPOST[%d]: ring 0x%px, sw %d:-, dg %d:%d (0x%px:0x%px)\n",
                      radio_idx, complete_common_radio_ptr->rx_post_fr_base_ptr,
                      complete_common_radio_ptr->rx_post_wr_idx,
                      *complete_common_radio_ptr->rx_post_fr_wr_idx_ptr,
                      *complete_common_radio_ptr->rx_post_fr_rd_idx_ptr,
                      complete_common_radio_ptr->rx_post_fr_wr_idx_ptr,
                      complete_common_radio_ptr->rx_post_fr_rd_idx_ptr);

            bcm_print("DHD_RXCMPL[%d]: ring 0x%px, sw %d:%d, dg %d:%d (0x%px:0x%px)\n",
                      radio_idx, complete_common_radio_ptr->rx_complete_fr_base_ptr,
                      complete_common_radio_ptr->rx_complete_wr_idx,
                      complete_common_radio_ptr->rx_complete_rd_idx,
                      *complete_common_radio_ptr->rx_complete_fr_wr_idx_ptr,
                      *complete_common_radio_ptr->rx_complete_fr_rd_idx_ptr,
                      complete_common_radio_ptr->rx_complete_fr_wr_idx_ptr,
                      complete_common_radio_ptr->rx_complete_fr_rd_idx_ptr);

            if(complete_common_radio_ptr->tx_complete_fr_base_ptr)
            {
                uint32_t flow_ring_id;
                int flow_ring_count = 0;
                int coalescing;

                bcm_print("DHD_TXCMPL[%d]: ring 0x%px, sw %d:%d, dg %d:%d (0x%px:0x%px)\n",
                          radio_idx, complete_common_radio_ptr->tx_complete_fr_base_ptr,
                          complete_common_radio_ptr->tx_complete_wr_idx,
                          complete_common_radio_ptr->tx_complete_rd_idx,
                          *complete_common_radio_ptr->tx_complete_fr_wr_idx_ptr,
                          *complete_common_radio_ptr->tx_complete_fr_rd_idx_ptr,
                          complete_common_radio_ptr->tx_complete_fr_wr_idx_ptr,
                          complete_common_radio_ptr->tx_complete_fr_rd_idx_ptr);

                coalescing = (radio_ptr->tx_post.stats.transfers) ?
                    (radio_ptr->tx_post.stats.packets / radio_ptr->tx_post.stats.transfers) : 0;

                bcm_print("DHD_TXPOST[%d]: packets %u, transfers %u, coalescing %u, "
                          "expirations %u, discards: flow_ring %u, free_index %u\n", radio_idx,
                          radio_ptr->tx_post.stats.packets,
                          radio_ptr->tx_post.stats.transfers,
                          coalescing, radio_ptr->tx_post.stats.expirations,
                          radio_ptr->tx_post.stats.flow_ring_discards,
                          radio_ptr->tx_post.stats.free_index_discards);

                bcm_print("RADIO[%d]: Flow Rings %d\n", radio_idx, radio_ptr->nbr_of_flow_rings);

                for(flow_ring_id=0; flow_ring_id<radio_ptr->nbr_of_flow_rings; ++flow_ring_id)
                {
                    rdpa_dhd_flring_cache_t *flow_ring_ptr =
                        &radio_ptr->flring_cache[flow_ring_id];

                    if(!(flow_ring_ptr->flags & FLOW_RING_FLAG_DISABLED_HTONS))
                    {
                        archer_dhd_flow_ring_dump(radio_idx, flow_ring_id);

                        if(++flow_ring_count == 16)
                        {
                            bcm_print("...\n");

                            break;
                        }
                    }
                }

                buffer_table_in_use = 1;
            }

            bcm_print("HOST[%d]: irq_status %d, irq_enable %d\n", radio_idx,
                      radio_ptr->cpu_cfg.irq_status, radio_ptr->cpu_cfg.irq_enable);
        }
    }

    if(buffer_table_in_use)
    {
        bcm_print("BUFFER_TABLE: %d/%d, wr %u, rd %u\n",
                  archer_dhd_index_pool_level(), ARCHER_DHD_INDEX_POOL_SIZE,
                  archer_dhd_g.buffer_table.writes, archer_dhd_g.buffer_table.reads);
    }

#if defined(CC_ARCHER_DHD_DEBUG)
    bcm_print("archer_dhd_debug_g (0x%px) = %d\n", &archer_dhd_debug_g, archer_dhd_debug_g);
#endif
}

/**********************************************************************************
 * Archer DHD Thread                                                              *
 **********************************************************************************/

typedef struct {
    unsigned long work_avail;
    wait_queue_head_t wqh;
    struct task_struct *task;
    archer_dhd_radio_t *radio_ptr;
} archer_dhd_thread_t;

static archer_dhd_thread_t archer_dhd_thread_g[ARCHER_DHD_RADIO_MAX];

static int archer_dhd_thread_handler(void *arg)
{
    archer_dhd_thread_t *thread_p = (archer_dhd_thread_t *)arg;
    rdpa_cpu_rxq_cfg_t *cpu_ptr = &thread_p->radio_ptr->cpu_cfg;

    bcm_print("Archer DHD Thread %u initialized\n",
              thread_p->radio_ptr->radio_idx);

    while(1)
    {
        wait_event_interruptible(thread_p->wqh,
                                 thread_p->work_avail);

        if(unlikely(kthread_should_stop()))
        {
            __logError("kthread_should_stop detected");

            break;
        }

        cpu_ptr->rx_isr(cpu_ptr->isr_priv);

        clear_bit(0, &thread_p->work_avail);
    }

    return 0;
}

void archer_dhd_thread_wakeup(uint32_t radio_idx)
{
    archer_dhd_thread_t *thread_p = &archer_dhd_thread_g[radio_idx];

    if(!thread_p->work_avail)
    {
        set_bit(0, &thread_p->work_avail);

        wake_up_interruptible(&thread_p->wqh);
    }
}

static void archer_dhd_thread_create(uint32_t radio_idx)
{
    archer_dhd_thread_t *thread_p = &archer_dhd_thread_g[radio_idx];

    memset(thread_p, 0, sizeof(archer_dhd_thread_t));

    thread_p->radio_ptr = &archer_dhd_g.radio[radio_idx];

    init_waitqueue_head(&thread_p->wqh);

    thread_p->task = kthread_create(archer_dhd_thread_handler, thread_p,
                                    "archer_dhd_%u", radio_idx);

    wake_up_process(thread_p->task);
}

/**********************************************************************************
 * Archer DHD Construct                                                              *
 **********************************************************************************/

int archer_dhd_construct(void)
{
    int radio_idx;
    int ret;

    memset(&archer_dhd_g, 0, sizeof(archer_dhd_t));

#if defined(CC_SYSPORT_DRIVER_USER_IRQ)
    ret = sysport_driver_user_irq_register(archer_dhd_isr, NULL,
                                           &archer_dhd_g.irq_set_phys_addr);
    if(ret)
    {
        __logError("Could not sysport_driver_user_irq_register");

        return ret;
    }
#endif

    archer_dhd_processing_construct();

    for(radio_idx=0; radio_idx<ARCHER_DHD_RADIO_MAX; ++radio_idx)
    {
        archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];
        int entry_size;

        radio_ptr->radio_idx = radio_idx;

        // Backup Queues

#if defined(CC_ARCHER_DHD_BACKUP_QUEUE)
        ret = archer_dhd_backup_queue_construct(radio_ptr);
        if(ret)
        {
            return ret;
        }
#endif
        // Rx Complete

        entry_size = ((sizeof(archer_dhd_rx_complete_queue_t) + 3) & ~3); // 32-bit aligned

        ret = bcm_async_queue_init(&radio_ptr->rx_complete.queue,
                                   ARCHER_DHD_RX_COMPLETE_QUEUE_SIZE, entry_size);
        if(ret)
        {
            __logError("Could not bcm_async_queue_init");

            return -1;
        }

        ARCHER_TASK_INIT(&radio_ptr->rx_complete.task, ARCHER_THREAD_ID_US,
                         archer_dhd_rx_complete_task_handler, radio_ptr);

        // Tx Post

        entry_size = ((sizeof(archer_dhd_tx_post_queue_t) + 3) & ~3); // 32-bit aligned

        ret = bcm_async_queue_init(&radio_ptr->tx_post.queue,
                                   ARCHER_DHD_TX_POST_QUEUE_SIZE, entry_size);
        if(ret)
        {
            __logError("Could not bcm_async_queue_init");

            return -1;
        }

        ARCHER_TASK_INIT(&radio_ptr->tx_post.task, ARCHER_THREAD_ID_US,
                         archer_dhd_tx_post_task_handler, radio_ptr);

        // Tx Coalescing

        ARCHER_TASK_INIT(&radio_ptr->tx_post.coalescing_task, ARCHER_THREAD_ID_US,
                         archer_dhd_tx_coalescing_task_handler, radio_ptr);

        bcm_timer_init(&radio_ptr->tx_post.timer, BCM_TIMER_MODE_ONESHOT, 0,
                       archer_dhd_tx_coalescing_timer_handler, radio_ptr);

        // CPU Message

        entry_size = ((sizeof(archer_dhd_cpu_msg_queue_t) + 3) & ~3); // 32-bit aligned

        ret = bcm_async_queue_init(&radio_ptr->cpu_msg.queue,
                                   ARCHER_DHD_CPU_MSG_QUEUE_SIZE, entry_size);
        if(ret)
        {
            __logError("Could not bcm_async_queue_init");

            return -1;
        }

        ARCHER_TASK_INIT(&radio_ptr->cpu_msg.task, ARCHER_THREAD_ID_US,
                         archer_dhd_cpu_msg_task_handler, radio_ptr);
    }

    archer_dhd_buffer_table_construct();

#if defined(CC_ARCHER_DHD_DEBUG)
    bcm_print("archer_dhd_debug_g (0x%px) = %d\n", &archer_dhd_debug_g, archer_dhd_debug_g);
#endif

    bcm_print("Archer DHD Initialized\n");

    return 0;
}
