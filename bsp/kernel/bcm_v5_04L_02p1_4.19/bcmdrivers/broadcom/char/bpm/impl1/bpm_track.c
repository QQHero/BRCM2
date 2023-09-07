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
 * File Name  : bpm_track.c
 *
 *******************************************************************************
 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/nbuff.h>
#include <linux/bcm_log_mod.h>
#include <linux/bcm_log.h>
#include <linux/export.h>
#include <board.h>
#include <linux/gbpm.h>
#include <bpmctl_common.h>
#include <bcm_pkt_lengths.h>
#include <bcmPktDma_defines.h>
#include <bpm.h>
#include "bpm_track.h"

/*----- Globals -----*/

#define BPM_TBUF_IRQ_SAVE(f)	local_irq_save(f)
#define BPM_TBUF_IRQ_RESTORE(f)	local_irq_restore(f)


const char *bpmtrk_drv_name[] =
{
	"BPM ",
	"ETH ",
	"XTM ",
	"KER ",
	"BDMF ",
	"ARCHER ",
	"MAX "
};

const char *bpmtrk_val_name[] =
{
	"nomrk",
	"alloc",
	"clone",
	"recyl",
	"free ",
	"rx   ",
	"tx   ",
	"enter",
	"exit ",
	"info ",
	"init ",
	"cpsrc",
	"cpdst",
	"xlate",
	"max "
};


/* Global instance for BPM Tracking */
bpm_track_data_t bpm_track_data_g;

static int bpm_track_enabled_g = 0;  /* Enabled during module init */
static int bpm_trail_len_g = BPM_TRK_DEF_LEN;
extern gbpm_mark_buf_hook_t gbpm_mark_buf_hook_g;
extern gbpm_add_ref_hook_t gbpm_add_ref_hook_g;


/*
 *------------------------------------------------------------------------------
 * BPM Tracking GBPM API
 *------------------------------------------------------------------------------
 */
static void bpm_track_mark_buf(void *buf_p, void *addr, int reftype, int driver,
			       int value, int info)
{
	void *base_p;
	gbpm_trail_t *trail_p;
	gbpm_mark_t *mark_p;
	unsigned long flags;

	base_p = bpm_get_buffer_base(buf_p);

	BPM_TBUF_IRQ_SAVE(flags);

	if (bpm_track_enabled_g) {
		if (likely(base_p != NULL)) {
			trail_p = (gbpm_trail_t *)base_p;
			mark_p = &trail_p->mbuf_p[trail_p->write];

			if (unlikely(trail_p->write == 0 &&
				     mark_p->value == GBPM_VAL_UNMARKED))
				bpm_track_data_g.marked_cnt++;

			trail_p->write++;
			if (unlikely(trail_p->write >= bpm_trail_len_g))
				trail_p->write = 0;

			mark_p->reftype = reftype;
			mark_p->addr = (size_t)((addr == NULL) ? buf_p : addr);
			mark_p->driver = driver;
			mark_p->info = info;
			mark_p->value = value;
		}
	}

	BPM_TBUF_IRQ_RESTORE(flags);
}

static void bpm_track_add_ref(void *buf_p, int i)
{
	gbpm_trail_t *trail_p;
	trail_p = (gbpm_trail_t *)bpm_get_buffer_base(buf_p);

	if (bpm_track_enabled_g && (trail_p != NULL))
		atomic_add(i, &trail_p->ref_cnt);
}

/*
 *------------------------------------------------------------------------------
 * BPM Tracking Userspace Utility Functions
 *------------------------------------------------------------------------------
 */
static inline void __bpm_track_print_bytes(uint8_t *data_p, int cnt,
					   int flip_endian)
{
	int i = 0;
	int j;
	int init = 0;
	int done = sizeof(void *);
	int inc = 1;

	if (flip_endian) {
		init = sizeof(void *) - 1;
		done = -1;
		inc = -1;
	}

	while (cnt) {
		if (i % (sizeof(void *) * 4) == 0)
			printk("\n[%px] ", &data_p[i]);

		for (j = init; j != done; j += inc)
			printk("%02x", data_p[i+j]);
		printk(" ");

		cnt -= sizeof(void *);
		i += sizeof(void *);
	}

	printk("\n");
}

static inline void __bpm_track_print_mark(gbpm_mark_t *mark_p)
{
	printk(" [");
	if (mark_p->driver >= 0 && mark_p->driver < GBPM_DRV_MAX)
		printk("%s:", bpmtrk_drv_name[mark_p->driver]);
	else
		printk("%s:", bpmtrk_drv_name[GBPM_DRV_MAX]);

	if (mark_p->value >= 0 && mark_p->value < GBPM_VAL_MAX)
		printk(" %s", bpmtrk_val_name[mark_p->value]);
	else
		printk(" %s", bpmtrk_val_name[GBPM_VAL_MAX]);

	printk(": %03u]", mark_p->info);
}

static inline void __bpm_track_print_trail(gbpm_trail_t *trail_p, void *base_p)
{
	int i, j;
	gbpm_mark_t *mark_p;
	size_t curr_addr = 0;

	for (i = 0; i < bpm_trail_len_g; i++) {
		j = (trail_p->write - i) % bpm_trail_len_g;
		mark_p = &trail_p->mbuf_p[j];

		if (mark_p->value != GBPM_VAL_UNMARKED) {
			if (mark_p->addr != curr_addr) {
				printk("\n	");

				if (mark_p->reftype == GBPM_REF_BUFF)
					printk("buff");
				else if (mark_p->reftype == GBPM_REF_FKB)
					printk(" fkb");
				else if (mark_p->reftype == GBPM_REF_SKB)
					printk(" skb");
				else
					printk("addr");

				printk(" [%px] : ", (void *)mark_p->addr);
				curr_addr = mark_p->addr;
			}

			__bpm_track_print_mark(mark_p);
		}
	}

	printk("\n\n");
}

static inline void __bpm_track_print_filters(bpmctl_track_t *trk_p)
{
	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_IDLE) ||
	    BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_IDLEMIN) ||
	    BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_REF) ||
	    BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_REFMIN) ||
	    BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_BASE))
		printk(" Buffer with:\n");

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_IDLE))
		printk("   idle cnt == (%u) \n", trk_p->idle);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_IDLEMIN))
		printk("   idle cnt >= (%u) \n", trk_p->idle_min);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_REF))
		printk("   ref cnt == (%u) \n", trk_p->ref);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_REFMIN))
		printk("   ref cnt >= (%u) \n", trk_p->ref_min);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_BASE))
		printk("   base address [%px]\n", (void *)trk_p->base);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_BASE) ||
	    BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_ADDR) ||
	    BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_DRIVER) ||
	    BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_VALUE) ||
	    BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_INFO))
		printk(" Has mark with:\n");

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_BASE))
		printk("   addr->base buffer [%px]\n", (void *)trk_p->base);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_ADDR))
		printk("   addr == [%px] \n", (void *)trk_p->addr);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_DRIVER))
		printk("   driver == %s \n", bpmtrk_drv_name[trk_p->driver]);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_VALUE))
		printk("   value == %s \n", bpmtrk_val_name[trk_p->value]);

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_INFO))
		printk("   info == (%u) \n", trk_p->info);
}

static inline int __bpm_track_filter_trail(gbpm_trail_t *trail_p,
					   bpmctl_track_t *trk_p)
{
	int i, filters;
	gbpm_mark_t *mark_p;
	void *test_p;

	filters = trk_p->filters;

	if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_IDLE) &&
	    atomic_read(&trail_p->idle_cnt) != trk_p->idle)
		goto nomatch;

	if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_IDLEMIN) &&
	    atomic_read(&trail_p->idle_cnt) < trk_p->idle_min)
		goto nomatch;

	if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_REF) &&
	    atomic_read(&trail_p->ref_cnt) != trk_p->ref)
		goto nomatch;

	if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_REFMIN) &&
	    atomic_read(&trail_p->ref_cnt) < trk_p->ref_min)
		goto nomatch;

	if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_BASE) &&
	    trail_p == (void *)trk_p->base)
		filters &= ~BPMCTL_TRK_BASE;

	/* Filter on marks */
	if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_BASE | BPMCTL_TRK_ADDR |
				    BPMCTL_TRK_DRIVER |	BPMCTL_TRK_VALUE |
				    BPMCTL_TRK_INFO)) {
		mark_p = trail_p->mbuf_p;
		i = bpm_trail_len_g;
		while (i) {
			if (mark_p->value != GBPM_VAL_UNMARKED) {
				if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_BASE)) {
					test_p = bpm_get_buffer_base((void *)mark_p->addr);
					if (test_p != (void *)trk_p->base)
						goto iterate;
				}

				if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_ADDR) &&
				    mark_p->addr != trk_p->addr)
					goto iterate;

				if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_DRIVER) &&
				    mark_p->driver != trk_p->driver)
					goto iterate;

				if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_VALUE) &&
				    mark_p->value != trk_p->value)
					goto iterate;

				if (BPMCTL_TRK_GET(filters, BPMCTL_TRK_INFO) &&
				    mark_p->info != trk_p->info)
					goto iterate;

				break;
			}
iterate:
			mark_p++;
			i--;
		}
		if (i <= 0)
			goto nomatch;
	}

	return BPMCTL_TRK_MATCH;

nomatch:
	return BPMCTL_TRK_NOMATCH;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_dump_trails
 * Description: function handler for dumping the trails
 *------------------------------------------------------------------------------
 */
static void bpm_track_dump_trails(bpmctl_track_t *trk_p)
{
	int i, cnt = 0;
	gbpm_trail_t *trail_p;
	unsigned long flags;
	uint32_t last_idx = bpm_get_last_idx();

	if (!bpm_track_enabled_g) {
		printk("\n BPM tracking is disabled!\n\n");
		return;
	}

	if (BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_BASE) && trk_p->base == 0)
		goto out;

	for (i = 0; i < last_idx; i++) {
		if (bpm_track_enabled_g) {
			BPM_TBUF_IRQ_SAVE(flags);

			trail_p = (gbpm_trail_t *)bpm_track_data_g.sbuf_pool[i];
			if (trail_p->mbuf_p->value != GBPM_VAL_UNMARKED &&
			    __bpm_track_filter_trail(trail_p, trk_p) == BPMCTL_TRK_MATCH) {
				printk(" [%px]  idle_cnt (%u)  ref_cnt (%u)",
				       bpm_track_data_g.sbuf_pool[i],
				       atomic_read(&trail_p->idle_cnt),
				       atomic_read(&trail_p->ref_cnt));
				__bpm_track_print_trail(trail_p,
					bpm_track_data_g.sbuf_pool[i]);
				cnt++;
			}

			BPM_TBUF_IRQ_RESTORE(flags);
		}
	}

out:
	printk("\n Found (%u) trails matching:\n", cnt);
	__bpm_track_print_filters(trk_p);
	printk("\n");

	return;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_dump_static_buffers
 * Description: function handler for dumping all BPM buffers.
 *------------------------------------------------------------------------------
 */
static void bpm_track_dump_static_buffers(bpmctl_track_t *trk_p)
{
	int i, cnt = 0;
	gbpm_trail_t *trail_p;
	unsigned long flags;
	uint32_t last_idx = bpm_get_last_idx();

	if (!BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_BASE) ||
	    trk_p->base != 0) {
		for (i = 0; i < last_idx; i++) {
			if (bpm_track_enabled_g) {
				BPM_TBUF_IRQ_SAVE(flags);

				trail_p = (gbpm_trail_t *)bpm_track_data_g.sbuf_pool[i];

				if (__bpm_track_filter_trail(trail_p, trk_p) == BPMCTL_TRK_MATCH) {
					if (cnt % 8 == 0)
						printk("\n [%05u]", cnt);
					cnt++;
					printk(" %px", trail_p);
				}

				BPM_TBUF_IRQ_RESTORE(flags);
			}
		}
	}

	printk("\n\n Found (%u) buffers matching:\n", cnt);
	__bpm_track_print_filters(trk_p);
	printk("\n");

	return;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_inc_trails
 * Description: function handler for incrementing idle_cnt on trails.
 *------------------------------------------------------------------------------
 */
static void bpm_track_inc_trails(bpmctl_track_t *trk_p)
{
	int i, cnt = 0;
	gbpm_trail_t *trail_p;
	unsigned long flags;
	uint32_t last_idx = bpm_get_last_idx();

	if (!BPMCTL_TRK_GET(trk_p->filters, BPMCTL_TRK_BASE) ||
	    trk_p->base != 0) {
		for (i = 0; i < last_idx; i++) {
			if (bpm_track_enabled_g) {
				BPM_TBUF_IRQ_SAVE(flags);

				trail_p = (gbpm_trail_t *)bpm_track_data_g.sbuf_pool[i];
				if (trail_p->mbuf_p->value != GBPM_VAL_UNMARKED &&
				    __bpm_track_filter_trail(trail_p, trk_p) == BPMCTL_TRK_MATCH) {
					atomic_inc(&trail_p->idle_cnt);
					cnt++;
				}

				BPM_TBUF_IRQ_RESTORE(flags);
			}
		}
	}

	printk("\nIncremented idle cnt on (%u) trails matching:\n", cnt);
	__bpm_track_print_filters(trk_p);
	printk("\n");

	return;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_dump_buffer
 * Description: function handler for dumping a BPM buffer or nbuff
 *------------------------------------------------------------------------------
 */
static int bpm_track_dump_buffer(bpmctl_track_t *trk_p)
{
	int i, j, found;
	unsigned int cnt = 0;
	uint8_t *data_p = NULL;
	gbpm_trail_t *trail_p;
	uint32_t last_idx = bpm_get_last_idx();
	unsigned long flags;

	/* TODO.. what is the use of variable "found"? */

	if (trk_p->reftype == BPMCTL_REF_BUFF || trk_p->reftype == BPMCTL_REF_ANY)
		data_p = bpm_get_buffer_base((void *)trk_p->addr);

	if (data_p != NULL) {
		data_p += BPM_TRACKING_HDR;
		cnt = BCM_PKTBUF_SIZE;
	} else {
		if (trk_p->reftype == BPMCTL_REF_BUFF) {
			printk("\n buff [%px] not found.\n", (void *)trk_p->addr);
			return BPM_SUCCESS;
		}

		for (i = 0; i < last_idx && !cnt; i++) {
			if (bpm_track_enabled_g) {
				BPM_TBUF_IRQ_SAVE(flags);

				trail_p = (gbpm_trail_t *)bpm_track_data_g.sbuf_pool[i];
				for (j = 0; j <= bpm_trail_len_g && !found; j++) {
					if (trail_p->mbuf_p[j].value != GBPM_VAL_UNMARKED &&
					    trail_p->mbuf_p[j].addr == trk_p->addr) {
						data_p = (uint8_t *)trk_p->addr;

						switch (trk_p->reftype) {
						case (BPMCTL_REF_ANY):
						case (BPMCTL_REF_SKB):
							if (trail_p->mbuf_p[j].reftype == GBPM_REF_SKB)
								cnt = sizeof(struct sk_buff);
						case (BPMCTL_REF_FKB):
							if (trail_p->mbuf_p[j].reftype == GBPM_REF_FKB)
								cnt = sizeof(FkBuff_t);
						case (BPMCTL_REF_BUFF):
							break;
						}
					}
				}

				BPM_TBUF_IRQ_RESTORE(flags);
			}
		}
		if (cnt == 0) {
			printk("\n nbuff [%px] not found. Can only dump tracked nbuff\n",
				   (void *)(size_t)trk_p->addr);
			return BPM_SUCCESS;
		}
	}

	__bpm_track_print_bytes(data_p, cnt, trk_p->flip_endian);
	printk("\n");

	return BPM_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_dump_status
 * Description: function handler for dumping BPM tracking status.
 *------------------------------------------------------------------------------
 */
static void bpm_track_dump_status(void)
{
	uint32_t last_idx = bpm_get_last_idx();
	printk("\n------------------- BPM Tracking Status -----------------\n");
	printk(" enabled  trail_len  marked_trails  total_trails\n");
	printk(" %7u  %10u  %13u  %12u\n",
	       bpm_track_enabled_g, bpm_trail_len_g,
	       bpm_track_data_g.marked_cnt, last_idx);
	printk("\n---------------------------------------------------------\n");
	printk(" sbuff_mem  trail_buff_pool_mem  tracking_header_sz\n");
	printk(" %7zuKB  %17uKB  %18u\n",
	       (last_idx * sizeof(size_t)) >> 10,
	       bpm_track_data_g.tbuf_pool_mem >> 10, BPM_TRACKING_HDR);
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_tbuf_pool_alloc
 * Description: Allocate a large memory chunk for carving out trail buffers
 *  The memory allocated is reset and flushed. A pointer to a cache aligned
 *  address of the requested size is returned. A pointer to the allocated
 *  pool is saved.
 *
 *  PS. Although kmalloc guarantees L1_CACHE_ALIGN, we do not assume so.
 *  Based on bpm_buf_mem_alloc.
 *------------------------------------------------------------------------------
 */
static void *bpm_track_tbuf_pool_alloc(size_t memsz)
{
	void *mem_p;

	BCM_LOG_FUNC(BCM_LOG_ID_BPM);

	memsz += L1_CACHE_BYTES;

	if ((mem_p = kmalloc(memsz, GFP_ATOMIC)) == NULL) {
		BCM_LOG_ERROR(BCM_LOG_ID_BPM, "kmalloc %d failure", (int)memsz);
		return NULL;
	}

	/* Future kfree */
	bpm_track_data_g.tbuf_pool[bpm_track_data_g.tbuf_pool_ix] = mem_p;
	bpm_track_data_g.tbuf_pool_ix++;
	bpm_track_data_g.tbuf_pool_mem += memsz;

	memset(mem_p, 0, memsz);
	cache_flush_len(mem_p, memsz); /* Flush invalidate */

	mem_p = (void *)L1_CACHE_ALIGN((uintptr_t)mem_p); /* L1 cache aligned */

	return mem_p;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_init_trail_bufs
 * Description: Initialize trail buffers.
 *  Pointers to pre-allocated trail buffers are saved. See bpm_track_tbuf_pool_alloc()
 *  Based on bpm_init_buf_pool.
 *------------------------------------------------------------------------------
 */
static int bpm_track_init_trail_bufs(uint32_t num, uint32_t tbuf_sz)
{
	void *mem_p;
	uint8_t *buf_p;
	uint32_t tbufs_per_pool, memsz, i, tbuf_ix;
	gbpm_trail_t *trail_p;
	uint8_t *data_p;

	/* Index into tentry pools allocated */
	bpm_track_data_g.tbuf_pool_ix = 0;
	bpm_track_data_g.tbuf_pool_mem = 0;
	tbuf_ix = 0;

	/* Allocate chunks of memory, carve trail buffers */
	tbufs_per_pool = (BPM_MAX_MEMSIZE - L1_CACHE_BYTES) / tbuf_sz;
	while (num) {
		/* Chunk size */
		tbufs_per_pool = (tbufs_per_pool < num) ? tbufs_per_pool : num;
		memsz = tbufs_per_pool * tbuf_sz;

		if ((mem_p = bpm_track_tbuf_pool_alloc(memsz)) == NULL)
			return BPM_ERROR;

		BCM_LOG_DEBUG(BCM_LOG_ID_BPM,
			      "allocated %4u %8s @ mem_p<%px> memsz<%06u>\n",
			      tbufs_per_pool, "TrailBufs", mem_p, memsz);

		buf_p = (uint8_t *)mem_p; /* Trail entries are cached */

		for (i = 0; i < tbufs_per_pool; i++, buf_p += tbuf_sz) {
			/* L1 cache aligned */
			data_p = (void *)L1_CACHE_ALIGN((uintptr_t)buf_p);
			trail_p = (gbpm_trail_t *)bpm_track_data_g.sbuf_pool[tbuf_ix++];
			trail_p->mbuf_p = (gbpm_mark_t *)data_p;
		}
		num -= tbufs_per_pool;
	}

	return BPM_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_enable
 * Description: function handler for enabling BPM buffer tracking feature.
 *------------------------------------------------------------------------------
 */
int bpm_track_enable(uint32_t trail_len)
{
	uint32_t tbuf_sz;
	uint32_t tbuf_align_sz;
	uint32_t tbuf_pool_ix;
	uint32_t tbuf_pool_sz;
	void *tbuf_pool_p = NULL;
	unsigned long flags;
	uint32_t last_idx = bpm_get_last_idx();

	BCM_LOG_FUNC(BCM_LOG_ID_BPM);

	BPM_TBUF_IRQ_SAVE(flags);

	if (bpm_track_enabled_g) {
		BCM_LOG_ERROR(BCM_LOG_ID_BPM,
			      "\n BPM tracking is already enabled!\n");
		bpm_track_dump_status();
		BPM_TBUF_IRQ_RESTORE(flags);
		return BPM_ERROR;
	}

	/* If no length is given, preserve previous settings */
	if (trail_len > 0) {
		bpm_trail_len_g = trail_len;
		if (trail_len > BPM_TRK_MAX_LEN)
			bpm_trail_len_g = BPM_TRK_MAX_LEN;
	}

	/* Calculate mark buffer size */
	tbuf_sz = sizeof(gbpm_mark_t) * bpm_trail_len_g;
	tbuf_align_sz = L1_CACHE_ALIGN(tbuf_sz);

	/* Increase trail length to match L1 cache aligned trail entry */
	bpm_trail_len_g = bpm_trail_len_g +
			  ((tbuf_align_sz - tbuf_sz) / sizeof(gbpm_mark_t));
	printk("Added (%zu) to trail length using (%u) excess bytes\n",
	       (tbuf_align_sz - tbuf_sz) / sizeof(gbpm_mark_t),
	       (tbuf_align_sz - tbuf_sz));

	/* Calculate size of references to memory pools */
	tbuf_pool_ix = (last_idx * tbuf_align_sz/BPM_MAX_MEMSIZE) + 1;
	tbuf_pool_sz = tbuf_pool_ix * sizeof(size_t);

	if ((tbuf_pool_p = kmalloc(tbuf_pool_sz, GFP_ATOMIC)) == NULL) {
		BCM_LOG_ERROR(BCM_LOG_ID_BPM,
			      "kmalloc %d failure for tbuf_pool_p", tbuf_pool_sz);
		BPM_TBUF_IRQ_RESTORE(flags);
		return BPM_ERROR;
	}

	bpm_track_data_g.tbuf_pool = (void **)tbuf_pool_p;
	bpm_track_data_g.tbuf_pool_mem += tbuf_pool_sz;

	printk("About to init %u trails of length %d size %uB, total %uKB\n",
	       last_idx, bpm_trail_len_g, tbuf_align_sz,
	       (tbuf_align_sz * last_idx) >> 10);
	if (bpm_track_init_trail_bufs(last_idx, tbuf_align_sz) == BPM_ERROR) {
		BCM_LOG_ERROR(BCM_LOG_ID_BPM,
			      "kmalloc %d failure for tbuf mem pools",
			      tbuf_align_sz * last_idx);
		BPM_TBUF_IRQ_RESTORE(flags);
		return BPM_ERROR;
	}

	bpm_track_enabled_g = 1;

	printk("\n BPM tracking enabled\n");
	bpm_track_dump_status();

	BPM_TBUF_IRQ_RESTORE(flags);

	return BPM_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * Function   : bpm_track_disable
 * Description: function handler for disabling BPM buffer tracking feature.
 *------------------------------------------------------------------------------
 */
int bpm_track_disable(void)
{
	void *data_p;
	gbpm_trail_t *trail_p;
	uint32_t i;
	unsigned long flags;
	uint32_t last_idx = bpm_get_last_idx();

	BCM_LOG_FUNC(BCM_LOG_ID_BPM);

	BPM_TBUF_IRQ_SAVE(flags);

	if (!bpm_track_enabled_g) {
		BPM_TBUF_IRQ_RESTORE(flags);
		printk("\n BPM tracking is already disabled!\n");
		return BPM_ERROR;
	}

	bpm_track_enabled_g = 0;

	/*
	 * Release all trail buffer pools allocated
	 */
	for (i = 0; i < bpm_track_data_g.tbuf_pool_ix; i++) {
		data_p = bpm_track_data_g.tbuf_pool[i];
		if (data_p)
			kfree(data_p);
	}
	printk(" Released (%u) trail buffer pools\n", bpm_track_data_g.tbuf_pool_ix);
	printk(" About to free trail buffer pool array <%px>\n",
	       bpm_track_data_g.tbuf_pool);
	kfree(bpm_track_data_g.tbuf_pool);
	bpm_track_data_g.tbuf_pool = NULL;
	printk(" Released trail buffer pool array\n");

	/* Clear trail buffer reference in tracking headers */
	for (i = 0; i < last_idx; i++) {
		trail_p = (gbpm_trail_t *)bpm_track_data_g.sbuf_pool[i];
		trail_p->mbuf_p = NULL;
	}
	printk(" Cleared buffer trail heads\n");

	/* Clear stats and flags */
	bpm_track_data_g.tbuf_pool_ix = 0;
	bpm_track_data_g.tbuf_pool_mem = 0;
	bpm_track_data_g.marked_cnt = 0;

	BPM_TBUF_IRQ_RESTORE(flags);

	kfree(bpm_track_data_g.sbuf_pool);

	printk("\n BPM tracking disabled\n");
	return BPM_SUCCESS;
}

int bpm_track_init_static_pool(int max, int top, void **pool_ptr)
{
	uint32_t memsz;
	int i;

	memsz = sizeof(size_t) * max;
	if ((bpm_track_data_g.sbuf_pool = kmalloc(memsz, GFP_ATOMIC)) == NULL)
		return BPM_ERROR;

	for (i = 0; i < top; i++)
		bpm_track_data_g.sbuf_pool[i] = BPM_BUF_TO_PFKBUFF(pool_ptr[i] - BPM_TRACKING_HDR);

	BCM_LOG_DEBUG(BCM_LOG_ID_BPM,
		      "allocated static buf pool %u memsz %uB <%uKB>\n",
		      top, memsz, memsz >> 10);

	return BPM_SUCCESS;
}

int bpm_track_ioctl(bpmctl_track_t *trk_p)
{
	trk_p->base = (unsigned long long)bpm_get_buffer_base((void *)trk_p->base);

	switch (trk_p->cmd) {
	case BPMCTL_TRK_STATUS:
		bpm_track_dump_status();
		return BPM_SUCCESS;
	case BPMCTL_TRK_ENABLE:
		return bpm_track_enable(trk_p->len);
	case BPMCTL_TRK_DISABLE:
		return bpm_track_disable();
	case BPMCTL_TRK_DUMP:
		return bpm_track_dump_buffer(trk_p);
	case BPMCTL_TRK_BUFFERS:
		bpm_track_dump_static_buffers(trk_p);
		return BPM_SUCCESS;
	case BPMCTL_TRK_TRAILS:
		bpm_track_dump_trails(trk_p);
		return BPM_SUCCESS;
	case BPMCTL_TRK_INC:
		bpm_track_inc_trails(trk_p);
		return BPM_SUCCESS;
	default:
		BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Invalid track cmd[%u]",
			      trk_p->cmd);
		return BPM_ERROR;
	}
	return BPM_SUCCESS;
}

int __init bpm_track_init(void)
{
	if (bpm_track_enable(bpm_trail_len_g) == BPM_ERROR)
		return BPM_ERROR;

	gbpm_mark_buf_hook_g = bpm_track_mark_buf;
	gbpm_add_ref_hook_g = bpm_track_add_ref;

	return 0;
}

