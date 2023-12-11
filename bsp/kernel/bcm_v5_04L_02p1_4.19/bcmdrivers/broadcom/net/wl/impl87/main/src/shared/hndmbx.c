/**
 * -----------------------------------------------------------------------------
 * Generic Broadcom Home Networking Division (HND) Mailbox Pager Revision 1
 * Mailbox in Host Memory Extension with Paging of mail.
 *
 * Copyright 2022 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id$
 *
 * vim: set ts=4 noet sw=4 tw=80:
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * -----------------------------------------------------------------------------
 */
#if defined(DONGLEBUILD) && defined(HNDMBX)
#include <osl.h>
#include <bcmhme.h>
#include <bcmpcie.h>
#include <bcmutils.h>
#include <hndbme.h>
#include <hndmbx.h>

/** HME resident Mailbox messages are paged into dongle 2 ping-pong slots */
#if (MBX_SLOTS_MAX == 2)
#define MBX_SLOT_MASK               (0x1)       // Slot index [0, 1]
#endif

#define MBX_DEBUG_BUILD
#define MBX_STATS_BUILD
//#define MBX_RDCLR_BUILD

#define MBX_NOOP                    do { /* no-op */ } while(0)
#define MBX_PRINT                   printf
#define MBX_ERROR                   printf

#if defined(MBX_DEBUG_BUILD)
#define MBX_DEBUG(expr)             expr
#define MBX_ASSERT(expr)            ASSERT(expr)
#else  /* ! MBX_DEBUG_BUILD */
#define MBX_DEBUG(expr)             MBX_NOOP
#define MBX_ASSERT(expr)            MBX_NOOP
#endif /* ! MBX_DEBUG_BUILD */

// +--------------------------------------------------------------------------+
//  Section: Design Engineering Statistics
// +--------------------------------------------------------------------------+

#if defined(MBX_STATS_BUILD)
#define MBX_STATS(expr)             expr
#define MBX_STATS_ADD(L, R, name)   (L)->stats.name += (R)->stats.name
#else  /* ! MBX_STATS_BUILD */
#define MBX_STATS(expr)
#define MBX_STATS_ADD(L, R, name)   MBX_NOOP
#endif /* ! MBX_STATS_BUILD */

#if defined(MBX_RDCLR_BUILD)
#define MBX_CTR_ZERO(ctr)           (ctr) = 0U
#else
#define MBX_CTR_ZERO(ctr)           MBX_NOOP
#endif

#define MBX_STATS_ZERO(P)           \
({  MBX_CTR_ZERO((P)->stats.xfers); \
	MBX_CTR_ZERO((P)->stats.syncs); \
	MBX_CTR_ZERO((P)->stats.error); \
})

#define MBX_STATS_ACCUM(L, R)       \
({  MBX_STATS_ADD(L, R, xfers);     \
	MBX_STATS_ADD(L, R, syncs);     \
	MBX_STATS_ADD(L, R, error);     \
})

#define MBX_STATS_FMT              "%8u %8u %8u"
#define MBX_STATS_VAL(P)           (P)->stats.xfers, (P)->stats.syncs, \
	                               (P)->stats.error

// +--------------------------------------------------------------------------+
//  Section: Data Types
// +--------------------------------------------------------------------------+

/** Mailbox statistics accumulated at a Mailbox service and per User level */
typedef struct mbx_stats
{
	uint32          xfers;         // messages paged in
	uint32          syncs;         // DMA syncs invoked
	uint32          error;         // errors encountered
} mbx_stats_t;

/*
 * +--------------------------------------------------------------------------+
 *
 *  Mailbox configuration:
 *  - Producer ring in host: 64b host address, ring element size and ring depth.
 *  - Consumer ring (ping-pong slots) in dongle: 32b dongle SysMem address.
 *
 *  Mailbox run time state:
 *  - Iterator used as a running counter and used to identify the next slot in
 *    consumer ping-pong ring (iter-mod-2 for two slot ping-pong).
 *
 *  Mailbox paging state:
 *  - msg_idx[]: tracks ring element indices that are being or have been paged.
 *  - bme_idx[]: tracks the bme_copy engine index for an ongoing pagein.
 *  A ping-pong buffer slot is ready to consume, when, msg_idx[slot] references
 *    a valid ring element index and bme_idx[slot] is idle.
 *
 * +--------------------------------------------------------------------------+
 */

typedef struct mbx_usr
{
	haddr64_t       haddr64;        // host base address of ring in ddr
	uintptr         daddr32;        // dngl base address of consumer slots

	uint16          msg_len;        // mbx ring element size in bytes
	uint16          msg_num;        // number of elements in mbx ring

	// MBX Service Rev 1 supports a max of 2 ping-pong slots
	uint16          msg_idx[MBX_SLOTS_MAX]; // ring elem index active
	int             bme_idx[MBX_SLOTS_MAX]; // bme_copy eng idx
	uint16          iter;           // mbx slot being consumed

	uint8           init;           // mbx init state: TRUE | FALSE
	uint8           idx;            // user index

#if defined(MBX_STATS_BUILD)
	mbx_stats_t     stats;
#endif
} mbx_usr_t;

/*
 * +--------------------------------------------------------------------------+
 *  Mailbox Pager service
 *  Maintains the configuration and runtime state per Mailbox (user).
 *  Uses the Byte Move Engine (BME) service for paging in Mailbox elements into
 *  Dongle consumer slots. BME user BME_USR_H2D is used to perform Host DDR to
 *  Dongle SysMem DMA.
 * +--------------------------------------------------------------------------+
 */

typedef struct mbx_sys
{
	mbx_usr_t       usr[MBX_USR_TOTAL];

	int             h2d_bme_key;    // BME_USR_H2D bme key
	uint8           init;           // mbx init state: TRUE | FALSE

	osl_t         * osh;            // RTE osh
#if defined(MBX_STATS_BUILD)
	mbx_stats_t     stats;
#endif
} mbx_sys_t;

// +--------------------------------------------------------------------------+
//  Section: Globals
// +--------------------------------------------------------------------------+

mbx_sys_t mbx_sys_g;

static const char * mbx_usr_str[] = { "MACTXS", "PHYRXS", "MBXREQ", "MBXRSP" };

// +--------------------------------------------------------------------------+
//  Section: Macro Accessors
// +--------------------------------------------------------------------------+

#define MBX_SYS()				    (&mbx_sys_g) // singleton global

/** Given a usr_idx, locate the mbx_usr_t */
#define _MBX_USR(USR_IDX)           (&MBX_SYS()->usr[(USR_IDX)])
#define MBX_USR(USR_IDX) \
({  mbx_usr_t * _mbx_usr; \
	MBX_ASSERT((USR_IDX) < MBX_USR_TOTAL); \
	_mbx_usr = _MBX_USR(USR_IDX); \
	MBX_ASSERT(_mbx_usr->init == TRUE); \
	_mbx_usr; \
})

/** Determine SLOT Index based on current iterator value */
/** Fetch ping-pong slot at ping-pong offset PPO from current iterator */
#define _MBX_SLOT(USR, PPO)         (((USR)->iter + (PPO)) & MBX_SLOT_MASK)
#define MBX_SLOT(USR, PPO) \
({  uint16 _slot = _MBX_SLOT(USR, PPO); \
	MBX_ASSERT((PPO) < MBX_SLOTS_MAX); \
	_slot; \
})

/** Fetch the current ping-pong slot index [0,1] */
#define MBX_CUR(USR)                _MBX_SLOT(USR, MBX_SLOT_CUR)
/** Fetch the alternate ping-pong slot index [0,1] */
#define MBX_ALT(USR)                _MBX_SLOT(USR, MBX_SLOT_ALT)

/** Give a slot index, compute slot run-time attributes */
/** Fetch the address of ping-pong buffer given the slot index [0,1] */
#define MBX_BUF(USR, SLOT)          ((USR)->daddr32 + ((SLOT) * (USR)->msg_len))
/** Fetch HME Mailbox message index that may be active in given slot [0,1] */
#define MBX_MSG(USR, SLOT)          ((USR)->msg_idx[(SLOT)])
/** Fetch BME engine index that may be operating on given slot [0,1] */
#define MBX_BME(USR, SLOT)          ((USR)->bme_idx[(SLOT)])

// +--------------------------------------------------------------------------+
//  Section: Functional Interface
// +--------------------------------------------------------------------------+

/** File scoped helper Routines */

static INLINE void /* Sync for BME xfer in a slot to finish, if is ongoing. */
__mbx_bme_sync(mbx_usr_t * mbx_usr, uint16 slot)
{
	if (MBX_BME(mbx_usr, slot) != MBX_BME_IDLE) { // if BME xfer in progress
		bme_sync_eng(MBX_SYS()->osh, MBX_BME(mbx_usr, slot)); // sync done
		MBX_STATS(mbx_usr->stats.syncs++);
		MBX_BME(mbx_usr, slot) = MBX_BME_IDLE; // track as IDLE
	}
} // __mbx_bme_sync()

static INLINE void /* Vacate a slot by marking it IDLE */
__mbx_idle(mbx_usr_t * mbx_usr, uint16 slot)
{
	MBX_ASSERT(MBX_BME(mbx_usr, slot) == MBX_BME_IDLE);
	MBX_MSG(mbx_usr, slot) = MBX_MSG_IDLE;
}

static INLINE void * /* Return a paged in ready message in a slot */
__mbx_message(mbx_usr_t * mbx_usr, uint16 slot, bool in_sync)
{
	/* If caller had scheduled a BME xfer, sync completion and return element */
	if (MBX_MSG(mbx_usr, slot) != MBX_MSG_IDLE) { // slot occupied by message
		__mbx_bme_sync(mbx_usr, slot); // if any BME transfer in progress, sync
		return (void*) MBX_BUF(mbx_usr, slot); // ready message
	}

	if (in_sync == TRUE) {
		MBX_DEBUG(MBX_PRINT("MBX: warning %s slot %u is idle\n",
			mbx_usr_str[mbx_usr->idx], slot));
		MBX_STATS(mbx_usr->stats.error++);
	}

	return NULL; // No prefetch BME xfer was initiated

} // __mbx_message()

// +--------------------------------------------------------------------------+
//  Section: Mailbox service run time API
// +--------------------------------------------------------------------------+

/**
 *  Initiate a page-in transfer into ping-pong slot for a HME mailbox message in
 *  circular ring index msg_idx. The current or alternate slot in the ping-pong
 *  buffer is specified using slot_offset.
 *  If the iterator points to slot N, then a slot_offset identifies destination
 *  ping-pong buffer as ((N + slot_offset) % MBX_SLOTS_MAX).
 *
 *  Return:
 *     BCME_ERROR: if a xfer is active in destination slot.
 *     slot# [0, 1] is a xfer was successfully initiated.
 */
int
mbx_xfer(uint32 usr_idx, uint16 msg_idx, uint16 slot_offset)
{
	mbx_usr_t * mbx_usr = MBX_USR(usr_idx);
	uint16      slot    = MBX_SLOT(mbx_usr, slot_offset);

	if (MBX_MSG(mbx_usr, slot) != MBX_MSG_IDLE) {
		MBX_DEBUG(MBX_PRINT("MBX: warning %s slot %u not idle\n",
			mbx_usr_str[mbx_usr->idx], slot));
		MBX_STATS(mbx_usr->stats.error++);
		return BCME_ERROR;
	}

	MBX_ASSERT(MBX_BME(mbx_usr, slot) == MBX_BME_IDLE);

	{
		mbx_sys_t * mbx_sys = MBX_SYS();
		uint64 msg_src64 = (((uint64)(HADDR64_HI(mbx_usr->haddr64))) << 32) |
		          (HADDR64_LO(mbx_usr->haddr64) + (msg_idx * mbx_usr->msg_len));
		uint64 msg_dst64 = MBX_BUF(mbx_usr, slot);

		MBX_BME(mbx_usr, slot) = bme_copy64(mbx_sys->osh, mbx_sys->h2d_bme_key,
		                             msg_src64, msg_dst64, mbx_usr->msg_len);
		MBX_MSG(mbx_usr, slot) = msg_idx;

		MBX_STATS(mbx_usr->stats.xfers++);
	}

	return slot;

} // mbx_xfer()

/**
 * Get the msg_idx occupying a slot (page-in transfer may be in progress.
 * Return: msg_idx or MBX_MSG_IDLE
 */
uint16
mbx_page(uint32 usr_idx, uint16 slot_offset)
{
	mbx_usr_t * mbx_usr = MBX_USR(usr_idx);
	uint16      slot    = MBX_SLOT(mbx_usr, slot_offset);
	return MBX_MSG(mbx_usr, slot);

} // mbx_page()

/**
 * Return a paged-in meassage ready for processing upon syncing ongoing DMA.
 * If no mbx_xfer was invoked for the specified slot, then NULL is returned.
 */
void *
mbx_sync(uint32 usr_idx, uint16 slot_offset)
{
	mbx_usr_t * mbx_usr = MBX_USR(usr_idx);
	uint16      slot    = MBX_SLOT(mbx_usr, slot_offset);

	return __mbx_message(mbx_usr, slot, TRUE); // may return NULL

} // mbx_sync()

/**
 * Advance the iterator to the next slot making the alternate slot the current
 * slot. If a pagein xfer was initiated then the DMA xfer will be synced if not
 * previously synced, and the ready message pointer is returned.
 * If no pagein xfer was previously initiated on the alternate slot, then NULL
 * is returned.
 */
void *
mbx_iter(uint32 usr_idx)
{
	mbx_usr_t * mbx_usr = MBX_USR(usr_idx);

	__mbx_idle(mbx_usr, MBX_CUR(mbx_usr));
	mbx_usr->iter++; // advance iterator to alternate slot, makeing it current

	return __mbx_message(mbx_usr, MBX_CUR(mbx_usr), FALSE); // may return NULL

} // mbx_iter()

/**
 * Sync on any active BME xfers and vacates all slots. NO iteration occurs.
 */
void
mbx_done(uint32 usr_idx)
{
	uint16 slot;
	mbx_usr_t * mbx_usr = MBX_USR(usr_idx);

	for (slot = 0; slot < MBX_SLOTS_MAX; ++slot) {
		__mbx_bme_sync(mbx_usr, slot);
		__mbx_idle(mbx_usr, slot);
	}

} // mbx_done()

/**
 * Query number of BME xfers active without explicit mbx_sync or mbx_done calls
 */
uint32
mbx_busy(uint32 usr_idx)
{
	uint16 slot;
	uint32 busy = 0U;
	mbx_usr_t * mbx_usr = MBX_USR(usr_idx);

	for (slot = 0; slot < MBX_SLOTS_MAX; ++slot)
		if (MBX_BME(mbx_usr, slot) != MBX_BME_IDLE) { ++busy; }
	return busy;

} // mbx_busy()

/**
 * Query number of slots in-use, either in-xfer or ready to consume.
 */
uint32
mbx_used(uint32 usr_idx)
{
	uint16 slot;
	uint32 used = 0U;
	mbx_usr_t * mbx_usr = MBX_USR(usr_idx);

	for (slot = 0; slot < MBX_SLOTS_MAX; ++slot)
		if (MBX_MSG(mbx_usr, slot) != MBX_MSG_IDLE) { ++used; }
	return used;

} // mbx_used()

/**
 * Query number of messages ready to consume.
 */
uint32
mbx_msgs(uint32 usr_idx)
{
	uint16 slot;
	uint32 msgs = 0U;
	mbx_usr_t * mbx_usr = MBX_USR(usr_idx);

	for (slot = 0; slot < MBX_SLOTS_MAX; ++slot)
		if ((MBX_MSG(mbx_usr, slot) != MBX_MSG_IDLE) &&
			(MBX_BME(mbx_usr, slot) == MBX_BME_IDLE)) { ++msgs; }
	return msgs;

} // mbx_msgs()

// +--------------------------------------------------------------------------+
//  Section: Mailbox service Initialization and User Configuration API
// +--------------------------------------------------------------------------+

/**
 * +--------------------------------------------------------------------------+
 *  Mailbox service Initialization API
 *
 *  Initialize Mailbox service with the Run Time Environment (RTOS).
 *  HME service registers BME user BME_USR_H2D with the BME service, and MBX
 *  acquires BME_USR_H2D bme_key for PageIn.
 *
 *  Caveats:
 *  BME service and HME services must be initialized before Mailbox service.
 * +--------------------------------------------------------------------------+
 */
int
mbx_init(si_t * sih, osl_t * osh)
{
	mbx_sys_t * mbx_sys = MBX_SYS();

	memset(mbx_sys, 0, sizeof(mbx_sys_t));
	mbx_sys->h2d_bme_key = bme_get_key(osh, BME_USR_H2D);
	mbx_sys->osh = osh; // rte osh

	MBX_PRINT("MBX Rev %d init H2D BME key 0x%08x\n",
		HNDMBX, mbx_sys->h2d_bme_key);

	return BCME_OK;

} // mbx_init()

/**
 * +--------------------------------------------------------------------------+
 *  Mailbox User Registration API
 *
 *  1. Carve memory for the producer circular ring in the HME_USER_MBXPGR host
 *     memory extension. HME_USER_MBXPGR uses a HME_MGR_SGMT memory management
 *     policy. Each carved segment is rounded up to 4 KBytes.
 *  2. Allocate a dongle resident ping-pong buffer.
 *  3. Save ring configuration.
 *  4. Initialiase run time state.
 *
 *  Return
 *  haddr64 - Mailbox User ring host memory base address
 *
 *  Caveats:
 *  HME service must be linked with the PCIe IPC [see hme_link_pcie_ipc()].
 * +--------------------------------------------------------------------------+
 */
int
mbx_register_user(osl_t * osh, uint32 usr_idx, uint16 msg_len, uint16 msg_num,
	haddr64_t * haddr64)
{
	uint32 slot, hme_bytes;
	haddr64_t   hme_haddr64;
	uintptr     daddr32;
	mbx_usr_t * mbx_usr;

	MBX_ASSERT(usr_idx < MBX_USR_TOTAL);
	MBX_ASSERT(msg_len != 0);
	MBX_ASSERT(msg_num != 0);

	mbx_usr = _MBX_USR(usr_idx);

	if (mbx_usr->init == TRUE) {
		MBX_ERROR("MBX: warning %s already registered\n",
			mbx_usr_str[usr_idx]);
		HADDR64_SET(*haddr64, mbx_usr->haddr64);
		return BCME_OK;
	}

	hme_bytes   = msg_len * msg_num;
	hme_bytes   = MBX_HME_BYTES(hme_bytes);

	// 1. Carve producer ring memory from HME_USER_MBXPGR region
	hme_haddr64 = hme_get(HME_USER_MBXPGR, hme_bytes);
	if (HADDR64_IS_ZERO(hme_haddr64)) {
		MBX_ERROR("MBX: hme_get %u failure\n", hme_bytes);
		goto hme_fail;
	}

	// 2. Allocate consumer ping-pong buffer in dongle sysmem
	daddr32 = (uintptr)MALLOCZ(osh, msg_len * MBX_SLOTS_MAX);
	if (daddr32 == 0U) {
		MBX_ERROR("MBX: mallocz %u failure\n", msg_len * MBX_SLOTS_MAX);
		goto mem_fail;
	}

	// 3. Save ring configuration
	HADDR64_SET(mbx_usr->haddr64, hme_haddr64);
	mbx_usr->daddr32 = daddr32;

	// 4. Initialize run time state
	mbx_usr->msg_len = msg_len;
	mbx_usr->msg_num = msg_num;

	for (slot = 0; slot < MBX_SLOTS_MAX; ++slot) {
		MBX_MSG(mbx_usr, slot) = MBX_MSG_IDLE;
		MBX_BME(mbx_usr, slot) = MBX_BME_IDLE;
	}

	mbx_usr->iter = 0;
	mbx_usr->init = TRUE;
	mbx_usr->idx  = usr_idx;

	MBX_PRINT("MBX: %s registered msg_len %u msg_num %u"
		    HADDR64_FMT " daddr32 0x%08x\n",
		    mbx_usr_str[mbx_usr->idx], mbx_usr->msg_len, mbx_usr->msg_num,
		    HADDR64_VAL(mbx_usr->haddr64), mbx_usr->daddr32);

	// Return ring host memory base adress
	HADDR64_SET(*haddr64, hme_haddr64);

	return BCME_OK;

mem_fail:
	hme_put(HME_USER_MBXPGR, hme_bytes, hme_haddr64);

hme_fail:
	return BCME_ERROR;

} // mbx_register_user()

// +--------------------------------------------------------------------------+
//  Section: Mailbox service Debug support API
// +--------------------------------------------------------------------------+

/**
 * +--------------------------------------------------------------------------+
 *  Mailbox service Debug Dump
 * +--------------------------------------------------------------------------+
 */
void
mbx_dump(bool verbose)
{
	uint32      usr_idx;
	mbx_sys_t * mbx_sys = MBX_SYS();

	printf("MBX Rev %u BME key 0x%08x\n", HNDMBX, mbx_sys->h2d_bme_key);
	MBX_STATS(memset(&mbx_sys->stats, 0, sizeof(mbx_stats_t)));

	printf("MBX_ID ESIZE DEPTH HADDR64_LO HADDR64_HI PINGPONG"
	       " C_IDX A_IDX BME_ENG  ITER STATS<xfers,syncs,error>\n");

	for (usr_idx = 0; usr_idx < MBX_USR_TOTAL; ++usr_idx)
	{
		mbx_usr_t * mbx_usr = _MBX_USR(usr_idx);
		if (mbx_usr->init == FALSE) continue;

		printf("%s %5u %5u 0x%08x 0x%08x %08x"
		       " %5u %5u %3u %3u %5u ",
		       mbx_usr_str[usr_idx], mbx_usr->msg_len, mbx_usr->msg_num,
		       HADDR64_VAL(mbx_usr->haddr64), mbx_usr->daddr32,
		       MBX_MSG(mbx_usr, 0), MBX_MSG(mbx_usr, 1),
		       (uint8)MBX_BME(mbx_usr, 0), (uint8)MBX_BME(mbx_usr, 1),
		       mbx_usr->iter);
		MBX_STATS({
			MBX_STATS_ACCUM(mbx_sys, mbx_usr);
			printf(MBX_STATS_FMT, MBX_STATS_VAL(mbx_usr));
			MBX_STATS_ZERO(mbx_usr);
		});
		printf("\n");
	}

	if (verbose == FALSE) return;

	printf("\n");

	for (usr_idx = 0; usr_idx < MBX_USR_TOTAL; ++usr_idx)
	{
		uint32 * p;
		int words, i;

		mbx_usr_t * mbx_usr = _MBX_USR(usr_idx);
		if (mbx_usr->init == FALSE) continue;

		words = (mbx_usr->msg_len / sizeof(uint32)) * MBX_SLOTS_MAX;
		p = (uint32*)(mbx_usr->daddr32);

		printf("%s: dump %u slots * %u bytes each\n",
			mbx_usr_str[usr_idx], MBX_SLOTS_MAX, mbx_usr->msg_len);
		for (i = 0; i < words; i += 8, p += 8) {
			printf("%4d. %08x %08x %08x %08x %08x %08x %08x %08x\n",
				i * sizeof(uint32),
				*(p + 0), *(p + 1), *(p + 2), *(p + 3),
				*(p + 4), *(p + 5), *(p + 6), *(p + 7));
		}
	}

	MBX_STATS({
		printf("\n");
		printf("\tSystem Stats<xfers,syncs,error>: " MBX_STATS_FMT "\n",
			MBX_STATS_VAL(mbx_sys));
		MBX_STATS_ZERO(mbx_sys);
	});

} // mbx_dump()

/**
 * +--------------------------------------------------------------------------+
 *  Mailbox service RTE console command line help
 * +--------------------------------------------------------------------------+
 */
static void
mbx_help(void)
{
	printf("MBX Rev %u\n"
	       "\tUsage: dhd -i <ifname> cons \"mbx -[d|v]\"\n"
	       "\t\t-d: Dump\n"
	       "\t\t-v: Verbose dump\n", HNDMBX);

} // mbx_help()

/*
 * +--------------------------------------------------------------------------+
 *  Mailbox service RTE console command handler (registered in rte.c)
 * +--------------------------------------------------------------------------+
 */
void // see rte.c: hnd_cons_add_cmd
mbx_cmd(void *arg, int argc, char *argv[])
{
	char *p = argv[1];

	if (argc < 2)  goto help;
	if (*p != '-') goto help;

	switch (*++p) {
		case 'd': mbx_dump(FALSE);  break;
		case 'v': mbx_dump(TRUE);   break;
		default: goto help;
	}
	return;

help: mbx_help();

} // mbx_cmd()

#endif /* HNDMBX */
