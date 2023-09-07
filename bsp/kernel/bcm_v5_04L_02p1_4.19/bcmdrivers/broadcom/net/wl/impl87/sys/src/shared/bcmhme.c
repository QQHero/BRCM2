/*
 * Broadcom Full Dongle Host Memory Extension (HME)
 * Implementation of the Interface to manage Host Memory Extensions.
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
 */

#include <osl.h>
#include <bcmhme.h>
#include <bcmpcie.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <sbtopcie.h> // sbtopcie_cpy32()
#include <rte_cons.h> // hnd_cons_add_cmd()
#ifdef HNDPQP
#include <hnd_pqp.h>
#endif

#ifdef SW_PAGING
#include <swpaging.h>
#define HME_HMOSWP_BUILD
#endif /* SW_PAGING */

/**
 * +--------------------------------------------------------------------------+
 *  Revision History
 * +--------------------------------------------------------------------------+
 *   1      HME Service : Users SCRMEM, PKTPGR, MACIFS, LCLPKT
 *   2      User CSIMON : Channel State Information
 *   3      User HMOSWP : Host Memory Offload - SW_PAGING
 *   4      User MBXPGR : Status Transfer Mailbox Pager - STS_XFER with MBX
 *   5      User PSQPKT : Power Save Queue Packet Storage Offload
 *   6      PCIE IPC 84 : Allow variable number of Users, in PCIE IPC rev 0x84
 * +--------------------------------------------------------------------------+
 */
#define HME_VERSION                 (6)

/* Variable number of HME users is supported post PCIe IPC Version 4 */
#define HME_HADDR64_TBLSZ           (sizeof(haddr64_t) * HME_USERS_TOTAL)

#ifdef  BCM_CSIMON
#define HME_CSIMON_BUILD
#endif

#ifdef BCMHWAPP
#define HME_PKTPGR_BUILD
#ifndef HME_MACIFS_BUILD
#define HME_MACIFS_BUILD
#endif
#define HME_LCLPKT_BUILD
#endif /* BCMHWAPP */

#ifdef HNDPQP
#define HME_PSQPKT_BUILD
#endif /* HNDPQP */

/** Scratch Memory: always present */
#ifndef HME_USER_SCRMEM_BYTES
#define HME_USER_SCRMEM_BYTES       PCIE_IPC_HME_PAGE_SIZE // default 4 KBytes
#endif

/** BCMHWAPP: PacketPager BM of 8K items. Each pkt (context+buf) is 256 Bytes */
#ifndef HME_USER_PKTPGR_BYTES
#define HME_USER_PKTPGR_ITEM_SIZE   (256)       // Lfrag+DataBuffer is 256Bytes
#define HME_USER_PKTPGR_ITEMS_MAX   (8 * 1024)  // PktPgr Host BM supports 8K
#define HME_USER_PKTPGR_BYTES \
	((uint32)(HME_USER_PKTPGR_ITEM_SIZE * HME_USER_PKTPGR_ITEMS_MAX))
#endif /* HME_USER_PKTPGR_BYTES */

/** BCMHWAPP: (37 Users) 154 AQM+TxDMA TxFIFOs + 3 RxFIFOs */
#ifndef HME_USER_MACIFS_BYTES
#define HME_USER_MACIFS_BYTES       (2 * 1024 * 1024)
#endif

/** BCMHWAPP: Local heap data packets offload */
#ifndef HME_USER_LCLPKT_BYTES
#define HME_USER_LCLPKT_ITEM_SIZE   (2 * 1024)  // 2 KBytes data buffer
#define HME_USER_LCLPKT_ITEMS_MAX   (256)       // 256 total data buffers
#define HME_USER_LCLPKT_BYTES \
	(HME_USER_LCLPKT_ITEM_SIZE * HME_USER_LCLPKT_ITEMS_MAX)
#endif /* HME_USER_LCLPKT_BYTES */

/** HNDPQP: PowerSave and Suppression Queue offload using Packet Queue Pager */
#ifndef HME_USER_PSQPKT_BYTES
#define HME_USER_PSQPKT_BYTES       PQP_HME_SZ
#endif /* HME_USER_PSQPKT_BYTES */

/** HNDMBX: Mailboxes in DDR with Mail Pager to Dongle SysMem */
#ifndef HME_USER_MBXPGR_BYTES
#define HME_USER_MBXPGR_BYTES       (256 * 1024)
#endif
#ifdef HNDMBX
#include <hndmbx.h>
#if (HME_USER_MBXPGR_BYTES) < (MBX_HME_BYTES_MAX)
#error "Mismatch HME and MBX Sizes"
#endif
#define HME_MBXPGR_BUILD
#endif /* HNDMBX */

#ifdef HME_CSIMON_BUILD
#include <bcm_csimon.h>     /** CSIMON: Channel State Information Monitor */
#define HME_USER_CSIMON_BYTES       CSIMON_IPC_HME_BYTES
#endif /* HME_CSIMON_BUILD */

#define HME_NOOP                    do { /* no-op */ } while(0)
#define HME_PRINT                   printf

/** Conditional Compile: Designer builds for extended debug and statistics */
// #define HME_DEBUG_BUILD
// #define HME_STATS_BUILD

#define HME_CTRS_RDZERO             /* Clear On Read */
#if defined(HME_CTRS_RDZERO)
#define HME_CTR_ZERO(_ctr_)         (_ctr_) = 0U
#else
#define HME_CTR_ZERO(_ctr_)         HME_NOOP
#endif

#if defined(HME_DEBUG_BUILD)
#define HME_ASSERT(expr)            ASSERT(expr)
#define HME_DEBUG(expr)             expr
#else  /* ! HME_DEBUG_BUILD */
#define HME_ASSERT(expr)            HME_NOOP
#define HME_DEBUG(expr)             HME_NOOP
#endif /* ! HME_DEBUG_BUILD */

#if defined(HME_STATS_BUILD)
#define HME_STATS(expr)             expr
#define HME_STATS_ZERO(ctr)         HME_CTR_ZERO(ctr)
#else  /* ! HME_STATS_BUILD */
#define HME_STATS(expr)             HME_NOOP
#define HME_STATS_ZERO(expr)        HME_NOOP
#endif /* ! HME_STATS_BUILD */

// +--- HME typedefs and data structures -------------------------------------+
struct hme_user;

typedef enum hme_state              // per HME user runtime state
{
	HME_USER_ISUP = 0,
	HME_BIND_PEND = 1 << 0,
	HME_LINK_PEND = 1 << 1
} hme_state_t;

// Allocator and deallocator function handler types
typedef haddr64_t (*hme_mgr_get_fn_t)(struct hme_user *hme_user, size_t bytes);
typedef int       (*hme_mgr_put_fn_t)(struct hme_user *hme_user, size_t bytes,
                                      haddr64_t free_haddr);

// HME memory manager: Segment carving
typedef struct hme_mgr_sgmt
{
	uint32             front;       // offset where to carve next block
	uint32             bytes;       // total free bytes available
} hme_mgr_sgmt_t;

// HME memory manager: Pool of fixed sized blocks
typedef struct hme_mgr_pool
{
	uint16             item_size;   // size of each item in pool
	uint16             items_max;   // total number of items in pool
	struct bcm_mwbmap *mwbmap;      // hierarchical multi word bitmap
} hme_mgr_pool_t;

// HME memory manager: Split-coalesce paradigm
typedef struct hme_mgr_spcl
{
	uint32             wip;         // XXX FIXME: work in progress ...
} hme_mgr_spcl_t;

// Per HME User runtime state
typedef struct hme_user
{
	uint8               state;      // hme_state_t
	uint8               policy;     // hme_mgr_policy_t: none, pool, sgmt,
	uint16              pages;      // requested number of PCIE IPC pages
	uint32              bytes;      // size in bytes allocated by host
	haddr64_t           haddr64;    // base address in host memory

	hme_mgr_get_fn_t    get_fn;     //   allocator function handler
	hme_mgr_put_fn_t    put_fn;     // deallocator function handler

	union {                         // Memory manager options
		hme_mgr_sgmt_t  sgmt;       // + segment carving
		hme_mgr_pool_t  pool;       // + pool of fixed size blocks
		hme_mgr_spcl_t  spcl;       // + split-coalesce
	} mgr;

#if defined(HME_STATS_BUILD)        // Statistic
	uint32              get_stat;   // hme_alloc() invocation count
	uint32              put_stat;   // hme_free()  invocation count
	uint32              bad_stat;   // resource allocation failure count
	uint32              err_stat;   // system error count e.g. not up
#endif   /* HME_STATS_BUILD */

} hme_user_t;

// PCIE_IPC_VERSION 4 and above, structure passed to DHD via PCIE IPC
typedef struct hme_pcie_ipc {
	pcie_ipc_hme_t      hme;
	pcie_ipc_hme_user_t hme_user[HME_USERS_TOTAL];
} hme_pcie_ipc_t;

/** See PCIE_IPC_HME_SIZE(_users_) in bcmpcie.h */
#define HME_PCIE_IPC_SIZE           (sizeof(hme_pcie_ipc_t))

// Chipset wide HME Runtime State
typedef struct hme_sys
{
	hme_user_t          user[HME_USERS_TOTAL];
	int                 bme_key[BME_USR_MAX];
	osl_t             * osh;        // backplane hnd_osh
	hme_pcie_ipc_t      pcie_ipc;   // passed via PCIe IPC rev 0x84 to DHD
	pcie_ipc_t        * pcie_ipc_p; // system bus owned pcie_ipc

	// void           * pciedev_p;
} hme_sys_t;

// +--- Globals and Static state ---------------------------------------------+

hme_sys_t hme_sys_g;

// Accessor Macros
#define HME_SYS()                       (&hme_sys_g)
#define HME_USER(_sys_, _id_)           (&((_sys_)->user[_id_]))
#define PCIE_IPC_HME(_sys_)             (&((_sys_)->pcie_ipc.hme))
#define PCIE_IPC_HME_USER(_sys_, _id_)  (&((_sys_)->pcie_ipc.hme_user[_id_]))

// Error reporting and debug dump strings. Max size 8bytes with '\0'.
static const char * hme_user_name[HME_USERS_TOTAL] =
{
	"SCRMEM", "PKTPGR", "MACIFS", "LCLPKT", "PSQPKT",
    "CSIMON", "HMOSWP", "MBXPGR", "AIR_IQ"
};

// +--- File scoped helper macros and routines -------------------------------+

// Alloc and Free Handler declarations
#define HME_MGR_DECL(MGR)                                                     \
static haddr64_t   _hme_mgr_ ## MGR ## _get(hme_user_t *user, size_t bytes);  \
static int         _hme_mgr_ ## MGR ## _put(hme_user_t *user, size_t bytes,   \
	                                        haddr64_t haddr64);               \
static INLINE int  _hme_mgr_ ## MGR ## _free(hme_user_t *user);               \
static INLINE int  _hme_mgr_ ## MGR ## _used(hme_user_t *user);               \
static INLINE void _hme_mgr_ ## MGR ## _dump(hme_user_t *user);

HME_MGR_DECL(none)      // Default error handler: UNDEF user or !HME_USER_ISUP
HME_MGR_DECL(sgmt)      // Segment Carving
HME_MGR_DECL(pool)      // Pool of Fixed Sized Blocks
// HME_MGR_DECL(spcl)      Split-Coalesce paradigm WIP

// Wrapper: user index to user object conversion, with audit
static INLINE hme_user_t * __hme_user_isup(int user_id);

// Backplane copy table of user HME addresses from host memory to dongle memory
static void _hme_table_copy(haddr64_t haddr64, uintptr daddr);

static INLINE void __hme_sys_audit(pcie_ipc_t * pcie_ipc, bool post_link);

// +--------------------------------------------------------------------------+
// Section: Default NONE Memory Manager
// +--------------------------------------------------------------------------+

/**
 * Default Alloc and Free handlers.
 *
 * No allocation/deallocation can occur until Link phase. All Users are assigned
 * the default handler, until the Link Phase, when the HME user state ISUP.
 *
 * All undefined users or users with policy HME_MGR_NONE will use the default
 * handlers.
 */

static haddr64_t // default allocator
_hme_mgr_none_get(hme_user_t *hme_user, size_t bytes)
{
	HME_STATS(hme_user->err_stat++);

	return hme_user->haddr64;
}   // _hme_mgr_none_get()

static int // default deallocator
_hme_mgr_none_put(hme_user_t *hme_user, size_t bytes, haddr64_t haddr64)
{
	HME_STATS(hme_user->err_stat++);

	ASSERT(0);
	return BCME_ERROR;

}   // _hme_mgr_none_put()

static INLINE int // default free bytes query
_hme_mgr_none_free(hme_user_t *hme_user)
{
	return 0;
}   // _hme_mgr_none_free()

static INLINE int // default used bytes query
_hme_mgr_none_used(hme_user_t *hme_user)
{
	return hme_user->bytes; // case of a bound user with HME_MGR_NONE policy
}   // _hme_mgr_none_used()

static INLINE void
_hme_mgr_none_dump(hme_user_t *hme_user)
{
	HME_PRINT("MGR_NONE\n");
}

// +--------------------------------------------------------------------------+
// Section: Segment Carving Memory Manager
// +--------------------------------------------------------------------------+

/**
 * Segment Carving Memory Manager
 *
 * A running offset of the next allocatable block is maintained by carving
 * memory blocks from the front of the memory region.
 * Freed blocks are NOT tracked on deallocation.
 * The entire memory segment is deemed free, when all blocks are deallocated,
 * reclaiming the un-tracked deallocated blocks.
 *
 * This memory manager may be used when memory is statically allocated at init
 * time and the memory persists for the operational duration of the system. In
 * the event of a "soft restart", wherein all blocks are freed and a "re-init"
 * is required, the user is required to ensure that all blocks are freed prior
 * to proceeding to re-init.
 */

static haddr64_t // Allocate memory from front of users HME
_hme_mgr_sgmt_get(hme_user_t *hme_user, size_t bytes)
{
	haddr64_t haddr64;
	hme_mgr_sgmt_t *hme_mgr_sgmt = &hme_user->mgr.sgmt;

	if (hme_mgr_sgmt->bytes >= bytes) {
		// Carve from current front: front
		uint32 haddr64_lo = HADDR64_LO(hme_user->haddr64)
			              + hme_mgr_sgmt->front;
		HADDR64_LO_SET(haddr64, haddr64_lo);
		HADDR64_HI_SET(haddr64, HADDR64_HI(hme_user->haddr64));
		hme_mgr_sgmt->front += bytes;
		hme_mgr_sgmt->bytes -= bytes;
	} else {
		HADDR64_LO_SET(haddr64, 0U);
		HADDR64_HI_SET(haddr64, 0U);
		HME_STATS(hme_user->bad_stat++); // 0ULL is not considered an error
	}

	return haddr64; // haddr64 may be 0ULL, and must be treated as invalid.

}   // _hme_mgr_sgmt_get()

static int // Deallocate memory into users HME
_hme_mgr_sgmt_put(hme_user_t *hme_user, size_t bytes, haddr64_t haddr64)
{
	hme_mgr_sgmt_t *hme_mgr_sgmt = &hme_user->mgr.sgmt;

	hme_mgr_sgmt->bytes += bytes;
	hme_mgr_sgmt->front -= bytes;

	// Each individual freed block is not tracked.
	if (hme_mgr_sgmt->bytes == hme_user->bytes)
		hme_mgr_sgmt->front = 0U; // All blocks freed, carve from start

	return BCME_OK;

}   // _hme_mgr_sgmt_put()

static INLINE int
_hme_mgr_sgmt_free(hme_user_t *hme_user)
{
	return (hme_user->bytes - hme_user->mgr.sgmt.front);
}   // _hme_mgr_sgmt_free()

static INLINE int
_hme_mgr_sgmt_used(hme_user_t *hme_user)
{
	return hme_user->mgr.sgmt.front;
}   // _hme_mgr_sgmt_used()

static INLINE void
_hme_mgr_sgmt_dump(hme_user_t *hme_user)
{
	hme_mgr_sgmt_t *hme_mgr_sgmt = &hme_user->mgr.sgmt;
	HME_PRINT("MGR_SGMT: front %u bytes %u\n",
		hme_mgr_sgmt->front, hme_mgr_sgmt->bytes);
}   // _hme_mgr_sgmt_dump()

// +--------------------------------------------------------------------------+
// Section: Pool Memory Manager
// +--------------------------------------------------------------------------+

/**
 * Pool Memory managed leverages the bcm hierarchical multi word bitmp abstract
 * data type, to track BCM_MWBMAP_ITEMS_MAX (e.g. 10 Kitems DONGLEBUILD) at just
 * over 1KBytes of memory. 1b1 implies a free item in the mwbmap.
 *
 * See bcmutils.[h,c].
 */
static haddr64_t // Allocate memory from front of users HME
_hme_mgr_pool_get(hme_user_t *hme_user, size_t bytes)
{
	uint32 item_idx;
	haddr64_t haddr64;
	hme_mgr_pool_t *hme_mgr_pool = &hme_user->mgr.pool;

	HME_ASSERT(hme_mgr_pool->mwbmap != NULL);

	// Use hierarchical multi word bitmap to find a free block in pool
	item_idx = bcm_mwbmap_alloc(hme_mgr_pool->mwbmap);

	if (item_idx != BCM_MWBMAP_INVALID_IDX) {
		uint32 haddr64_lo = HADDR64_LO(hme_user->haddr64)
			              + (hme_mgr_pool->item_size * item_idx);
		HADDR64_LO_SET(haddr64, haddr64_lo);
		HADDR64_HI_SET(haddr64, HADDR64_HI(hme_user->haddr64));
	} else {
		HADDR64_LO_SET(haddr64, 0U);
		HADDR64_HI_SET(haddr64, 0U);
		HME_STATS(hme_user->bad_stat++); // 0ULL is not considered an error
	}

	return haddr64; // haddr64 may be 0ULL, and must be treated as invalid.

}   // _hme_mgr_pool_get()

static int // Deallocate memory into users HME
_hme_mgr_pool_put(hme_user_t *hme_user, size_t bytes, haddr64_t haddr64)
{
	uint32 item_idx;
	hme_mgr_pool_t *hme_mgr_pool = &hme_user->mgr.pool;

	HME_ASSERT(hme_mgr_pool->mwbmap != NULL);

	item_idx = (HADDR64_LO(haddr64) - HADDR64_LO(hme_user->haddr64))
		       / hme_mgr_pool->item_size;
	HME_ASSERT(item_idx < hme_mgr_pool->items_max);

	bcm_mwbmap_free(hme_mgr_pool->mwbmap, item_idx);

	return BCME_OK;

}   // _hme_mgr_pool_put()

static INLINE int // in units of number of blocks free
_hme_mgr_pool_free(hme_user_t *hme_user)
{
	hme_mgr_pool_t *hme_mgr_pool = &hme_user->mgr.pool;
	return (hme_mgr_pool->mwbmap == NULL) ?
		0 : bcm_mwbmap_free_cnt(hme_mgr_pool->mwbmap);

}   // _hme_mgr_pool_free()

static INLINE int // in units of number of blocks in-use
_hme_mgr_pool_used(hme_user_t *hme_user)
{
	hme_mgr_pool_t *hme_mgr_pool = &hme_user->mgr.pool;
	return (hme_mgr_pool->mwbmap == NULL) ?
		0 : hme_mgr_pool->items_max - bcm_mwbmap_free_cnt(hme_mgr_pool->mwbmap);
}   // _hme_mgr_pool_used()

static INLINE void
_hme_mgr_pool_dump(hme_user_t *hme_user)
{
	hme_mgr_pool_t *hme_mgr_pool = &hme_user->mgr.pool;
	HME_PRINT("MGR_POOL: item_size %u items_max %u free_cnt %u\n",
		hme_mgr_pool->item_size, hme_mgr_pool->items_max,
		(hme_mgr_pool->mwbmap == NULL) ?
		0 : bcm_mwbmap_free_cnt(hme_mgr_pool->mwbmap));
}   // _hme_mgr_pool_dump()

// +--------------------------------------------------------------------------+
static INLINE hme_user_t *
__hme_user_isup(int user_id)
{
	hme_sys_t  * hme_sys  = HME_SYS();
	hme_user_t * hme_user = HME_USER(hme_sys, user_id);

	ASSERT(user_id < HME_USERS_TOTAL);
	HME_ASSERT(hme_user->state == HME_USER_ISUP);

	return hme_user;

}   // __hme_user_isup()

static void // Copy the table of User HME addresses from host into dongle
_hme_table_copy(haddr64_t haddr64, uintptr daddr)
{
	uint64 haddr_u64;
	uint32 words = (HME_HADDR64_TBLSZ / sizeof(uint32)); // 4 Byte words

	HADDR64_TO_U64(haddr64, haddr_u64); // convert haddr64_t to uint64
	sbtopcie_cpy32(daddr, haddr_u64, words, SBTOPCIE_DIR_H2D);

}   // _hme_table_copy()

static void // hnd_cons_add_cmd()
hme_cmd(void * arg, int argc, char * argv[])
{
	char *p = argv[1];
	if (argc < 2)  goto cmd_help;
	if (*p != '-') goto cmd_help;

	switch (*++p) {
		case 'd': hme_dump(FALSE); return;
		case 'v': hme_dump(TRUE);  return;
		default : break;
	}

cmd_help: printf("hme -[d|v|h]\n");
}   // hme_cmd()

static INLINE void
__hme_sys_audit(pcie_ipc_t * pcie_ipc, bool post_link)
{
#if defined(HME_DEBUG_BUILD)
	int              user_id, hme_pages;
	hme_sys_t      * hme_sys = HME_SYS();
	pcie_ipc_t     * pcie_ipc_p = hme_sys->pcie_ipc_p;
	pcie_ipc_hme_t * pcie_ipc_hme;
	pcie_ipc_hme   = PCIE_IPC_HME(hme_sys);
	BCM_REFERENCE(pcie_ipc_p);
	BCM_REFERENCE(pcie_ipc_hme);

	HME_ASSERT(pcie_ipc_hme->version == HME_VERSION);
	HME_ASSERT(pcie_ipc_hme->users   == HME_USERS_TOTAL);
	HME_ASSERT(pcie_ipc_hme->size    == HME_PCIE_IPC_SIZE);

	if (post_link == FALSE) {
		HME_ASSERT(pcie_ipc_hme->bytes == 0U);
		HME_ASSERT(HADDR64_IS_ZERO(pcie_ipc_hme->haddr64));
		return;
	}

	/* post hme_link_pcie_ipc */

	hme_pages = 0U;
	for (user_id = 0; user_id < HME_USERS_TOTAL; ++user_id)
	{
		hme_user_t * hme_user = HME_USER(hme_sys, user_id);
		pcie_ipc_hme_user_t * pcie_ipc_hme_user;
		pcie_ipc_hme_user   = PCIE_IPC_HME_USER(hme_sys, user_id);
		BCM_REFERENCE(pcie_ipc_hme_user);

		HME_ASSERT(hme_user->pages == pcie_ipc_hme_user->pages);
		if (hme_user->pages == 0U) {
			HME_ASSERT(HADDR64_IS_ZERO(hme_user->haddr64));
		} else {
			HME_ASSERT(!HADDR64_IS_ZERO(hme_user->haddr64));
			HME_ASSERT(hme_user->state == HME_USER_ISUP);
		}
		HME_ASSERT(hme_user->bytes == PCIE_IPC_HME_BYTES(hme_user->pages));
		hme_pages += hme_user->pages;
	}

	HME_ASSERT(pcie_ipc_p != (pcie_ipc_t*)NULL);
	HME_ASSERT(pcie_ipc_p->host_mem_len == PCIE_IPC_HME_BYTES(hme_pages));
	HME_ASSERT(pcie_ipc_p->host_mem_len == pcie_ipc_hme->bytes);

	HME_ASSERT(HADDR64_LO(pcie_ipc_p->host_mem_haddr64)
	           == HADDR64_LO(pcie_ipc_hme->haddr64));
	HME_ASSERT(HADDR64_HI(pcie_ipc_p->host_mem_haddr64)
	           == HADDR64_HI(pcie_ipc_hme->haddr64));
#endif /* HME_DEBUG_BUILD */

}   // __hme_sys_audit()

// +--------------------------------------------------------------------------+

int // HME global system initialization. Invoked in hnd_init().
BCMATTACHFN(hme_init)(osl_t *hnd_osh)
{
	int         user_id, bme_usr_id;
	hme_sys_t * hme_sys = HME_SYS();

	ASSERT(PCIE_IPC_HME_SIZE(HME_USERS_TOTAL) == HME_PCIE_IPC_SIZE);

	memset(hme_sys, 0, sizeof(hme_sys_t));
	hme_sys->osh = hnd_osh; // see rte.c: backplane hnd osh

	for (user_id = 0; user_id < HME_USERS_TOTAL; user_id++)
	{
		hme_user_t * hme_user = HME_USER(hme_sys, user_id);

		hme_user->state  = HME_BIND_PEND | HME_LINK_PEND;
		hme_user->get_fn = _hme_mgr_none_get;
		hme_user->put_fn = _hme_mgr_none_put;
	}

	for (bme_usr_id = 0; bme_usr_id < BME_USR_MAX; bme_usr_id++) {
		hme_sys->bme_key[bme_usr_id] = BCME_ERROR;
	}

	{
		// Initialize pcie_ipc_hme_t hme::pcie_ipc::hme
		pcie_ipc_hme_t * pcie_ipc_hme = PCIE_IPC_HME(hme_sys);

		pcie_ipc_hme->version = HME_VERSION;
		pcie_ipc_hme->users   = HME_USERS_TOTAL;
		pcie_ipc_hme->size    = HME_PCIE_IPC_SIZE;
		// Initialized in hme_link_pcie_ipc()
		// + pcie_ipc_hme::bytes   from pcie_ipc::host_mem_len
		// + pcie_ipc_hme::haddr64 from pcie_ipc::host_mem_haddr64

		for (user_id = 0; user_id < HME_USERS_TOTAL; ++user_id)
		{
			// Initialize pcie_ipc_hme_user_t hme::pcie_ipc::hme_user[]
			pcie_ipc_hme_user_t * pcie_ipc_hme_user;
			pcie_ipc_hme_user   = PCIE_IPC_HME_USER(hme_sys, user_id);

			/* Apply default PCIE IPC HME attributes */
			pcie_ipc_hme_user->user_id    = user_id;
			pcie_ipc_hme_user->align_bits = PCIE_IPC_HME_ALIGN_BITS;
			pcie_ipc_hme_user->bound_bits = PCIE_IPC_HME_BOUND_BITS;
			pcie_ipc_hme_user->flags      = PCIE_IPC_HME_FLAGS;

			strncpy(pcie_ipc_hme_user->name, hme_user_name[user_id],
			        PCIE_IPC_HME_USER_NAME_SIZE - 1);
		}
	}

	if (!hnd_cons_add_cmd("hme", hme_cmd, (void*)hme_sys))
	{
#ifdef BCMDBG
		HME_PRINT("%s: Failed to add hme command \n", __FUNCTION__);
#endif  /* BCMDBG */
		return BCME_NOMEM;
	}

	printf("BCM HME Service Initialized\n");
	return BCME_OK;

}   // hme_init()

void // Debug dump HME subsystem
hme_dump(bool verbose)
{
	int         user_id;
	hme_sys_t * hme_sys = HME_SYS();

	HME_PRINT("BCM Host Memory Extension Service %p ver %u users %u size %u\n",
		HME_SYS(), HME_VERSION, HME_USERS_TOTAL, HME_PCIE_IPC_SIZE);

	if (hme_sys->pcie_ipc_p != (pcie_ipc_t*)NULL) {
		pcie_ipc_t * pcie_ipc = hme_sys->pcie_ipc_p;
		HME_PRINT("\tPCIe IPC Dngl: users %u size %u daddr32 0x%08x\n",
			pcie_ipc->host_mem_users, pcie_ipc->host_mem_size,
			pcie_ipc->host_mem_daddr32);
		HME_PRINT("\tPCIe IPC Host: len %u" HADDR64_FMT "\n",
			pcie_ipc->host_mem_len,
			HADDR64_VAL(pcie_ipc->host_mem_haddr64));
	}
#if defined(HME_DEBUG_BUILD)
	{
		pcie_ipc_hme_t * pcie_ipc_hme = PCIE_IPC_HME(hme_sys);
		HME_PRINT("\tPCIe IPC Host Mem: bytes %u" HADDR64_FMT "\n",
			pcie_ipc_hme->bytes, HADDR64_VAL(pcie_ipc_hme->haddr64));
	}
#endif /* HME_DEBUG_BUILD */

	HME_PRINT("\t  USER PAGES   BYTES HADDR_HI HADDR_LO ALN BND FLAG");
	HME_STATS(HME_PRINT("  GET_STAT   PUT_STAT   BAD_STAT   ERR_STAT "));
	HME_PRINT("\n");

	for (user_id = 0; user_id < HME_USERS_TOTAL; ++user_id)
	{
		pcie_ipc_hme_user_t * pcie_ipc_hme_user;
		hme_user_t * hme_user = HME_USER(hme_sys, user_id);
		pcie_ipc_hme_user     = PCIE_IPC_HME_USER(hme_sys, user_id);

		if (hme_user->state != HME_USER_ISUP)
			continue;

		// Display user configuration
		HME_PRINT("\t%6s %5u %7u %08x %08x %3u %3u %04x",
			hme_user_name[user_id], hme_user->pages, hme_user->bytes,
			HADDR64_HI(hme_user->haddr64), HADDR64_LO(hme_user->haddr64),
			pcie_ipc_hme_user->align_bits, pcie_ipc_hme_user->bound_bits,
			pcie_ipc_hme_user->flags);

		HME_STATS({
			HME_PRINT("%10u %10u %10u %10u ",
				hme_user->get_stat, hme_user->put_stat,
				hme_user->bad_stat,  hme_user->err_stat);
			HME_STATS_ZERO(hme_user->get_stat);
			HME_STATS_ZERO(hme_user->put_stat);
			HME_STATS_ZERO(hme_user->bad_stat);
			HME_STATS_ZERO(hme_user->err_stat);
		});

		switch (hme_user->policy)
		{
			case HME_MGR_NONE: _hme_mgr_none_dump(hme_user); break;
			case HME_MGR_SGMT: _hme_mgr_sgmt_dump(hme_user); break;
			case HME_MGR_POOL: _hme_mgr_pool_dump(hme_user); break;
			case HME_MGR_SPCL: HME_PRINT("MGR_SPCL: "); // fall through
			default:           HME_PRINT("INVALID POLICY\n");
		} // switch user policy
	} // for each user_id

#ifdef HNDBME
	{
		int *key = hme_sys->bme_key; // blindly dump 8 ints
		HME_PRINT("\tbme_key: %08x %08x %08x %08x %08x %08x %08x %08x\n",
		      *(key + 0), *(key + 1), *(key + 2), *(key + 3),
		      *(key + 4), *(key + 5), *(key + 6), *(key + 7));
	}
#endif /* HNDBME */

	if (!verbose)
		return;

	// Verbose dump of supporting hierarchical multi word bitmap allocator
	for (user_id = 0; user_id < HME_USERS_TOTAL; user_id++)
	{
		hme_user_t * hme_user = HME_USER(hme_sys, user_id);
		if ((hme_user->policy == HME_MGR_POOL) && (hme_user->mgr.pool.mwbmap)) {
			HME_PRINT("%s MWBMAP dump:\n", hme_user_name[user_id]);
			bcm_mwbmap_show(hme_user->mgr.pool.mwbmap);
		}
	}

}   // hme_dump()

haddr64_t
hme_haddr64(int user_id) // Fetch the base haddr64 of a HME segment for a user
{
	hme_user_t * hme_user = __hme_user_isup(user_id);
	return hme_user->haddr64;
}   // hme_haddr64()

haddr64_t // Allocate a host memory block from a user HME
hme_get(int user_id, size_t bytes)
{
	hme_user_t * hme_user = __hme_user_isup(user_id);
	HME_STATS(hme_user->get_stat++);
	return hme_user->get_fn(hme_user, bytes);

}   // hme_alloc()

int // Deallocate a host memory block into a user HME
hme_put(int user_id, size_t bytes, haddr64_t haddr64)
{
	hme_user_t * hme_user = __hme_user_isup(user_id);
	ASSERT(!HADDR64_IS_ZERO(haddr64));
	HME_STATS(hme_user->put_stat++);
	return hme_user->put_fn(hme_user, bytes, haddr64);

}   // hme_free()

int // Query total bytes available for allocation in HME for a given user.
hme_free(int user_id)
{
	int bytes;
	hme_user_t * hme_user = __hme_user_isup(user_id);
	switch (hme_user->policy) {
		case HME_MGR_NONE: bytes = _hme_mgr_none_free(hme_user); break;
		case HME_MGR_SGMT: bytes = _hme_mgr_sgmt_free(hme_user); break;
		case HME_MGR_POOL: bytes = _hme_mgr_pool_free(hme_user); break;
		default:           bytes = 0;
			HME_PRINT("HME %s free bytes invalid\n", hme_user_name[user_id]);
			HME_STATS(hme_user->err_stat++);
			HME_ASSERT(0);
	}
	return bytes;

}   // hme_free()

int // Query total used bytes in HME for a given user
hme_used(int user_id)
{
	int bytes;
	hme_user_t * hme_user = __hme_user_isup(user_id);
	switch (hme_user->policy) {
		case HME_MGR_NONE: bytes = _hme_mgr_none_used(hme_user); break;
		case HME_MGR_SGMT: bytes = _hme_mgr_sgmt_used(hme_user); break;
		case HME_MGR_POOL: bytes = _hme_mgr_pool_used(hme_user); break;
		default:           bytes = 0;
			HME_PRINT("HME %s used bytes invalid\n", hme_user_name[user_id]);
			HME_STATS(hme_user->err_stat++);
			HME_ASSERT(0);
	}
	return bytes;

}   // hme_used()

bool // Check capabilities
hme_cap(int user_id)
{
	bool cap = FALSE;

	ASSERT(user_id < HME_USERS_TOTAL);

	switch (user_id) {
		case HME_USER_SCRMEM: cap = TRUE; break;
#ifdef HME_PKTPGR_BUILD
		case HME_USER_PKTPGR: cap = TRUE; break;
#endif
#ifdef HME_MACIFS_BUILD
		case HME_USER_MACIFS: cap = TRUE; break;
#endif
#ifdef HME_LCLPKT_BUILD
		case HME_USER_LCLPKT: cap = TRUE; break;
#endif
#ifdef HME_PSQPKT_BUILD
		case HME_USER_PSQPKT: cap = TRUE; break;
#endif
#ifdef HME_CSIMON_BUILD
		case HME_USER_CSIMON: cap = TRUE; break;
#endif
#ifdef HME_HMOSWP_BUILD
		case HME_USER_HMOSWP: cap = TRUE; break;
#endif
#ifdef HME_MBXPGR_BUILD
		case HME_USER_MBXPGR: cap = TRUE; break;
#endif
		default: break;
	}

	return cap;
}   // hme_cap()

/**
 * User may override the default configuration applied in hme_bind_pcie_ipc().
 * hme_bind_pcie_ipc() will skip the application of a default configuration for
 * a user. This design allows for a user's attachment to occur after the pciedev
 * attach sequence.
 *
 * Caveat: An hme_attach_mgr() may not invoked after pciedev to DHD PCIE IPC
 * handshake has started.
 */
int // Attach and configure a HME Request during PCIE IPC Bind phase
BCMATTACHFN(hme_attach_mgr)(int user_id, uint32 pages,
                        uint32 item_size, uint32 items_max, hme_policy_t policy,
                        uint32 align_bits, uint32 bound_bits, uint32 flags)
{
	uint32       hme_user_bytes;
	hme_sys_t  * hme_sys  = HME_SYS();
	hme_user_t * hme_user = HME_USER(hme_sys, user_id);

	pcie_ipc_hme_user_t * pcie_ipc_hme_user;
	pcie_ipc_hme_user   = PCIE_IPC_HME_USER(hme_sys, user_id);

	BCM_REFERENCE(hme_user_bytes);

	ASSERT(user_id < HME_USERS_TOTAL);
	ASSERT(pages <= PCIE_IPC_HME_PAGES_MAX);

	ASSERT(hme_user->state != HME_USER_ISUP);

	// HME manager handlers are attached in Link phase and state set ISUP.
	if (policy == HME_MGR_POOL) // configure HME Manager Pool
	{
		hme_user_bytes = (item_size * items_max);
		ASSERT(pages == PCIE_IPC_HME_PAGES(hme_user_bytes));
		hme_user->mgr.pool.item_size = item_size;
		hme_user->mgr.pool.items_max = items_max;
	}
	else if (policy == HME_MGR_SGMT) // configure HME Manager Sgmt
	{
		hme_user->mgr.sgmt.front = 0U; // redundant
	} // HME MGR policy setting

	// configure HME user state
	hme_user->policy  = policy;
	hme_user->pages   = pages;
	hme_user->state   = HME_LINK_PEND;
	// fields filled in hme_link_pcie_ipc: hme_user::bytes, hme_user::haddr64

	// Configuration to be passed to DHD over PCIe IPC
	pcie_ipc_hme_user->align_bits = align_bits;
	pcie_ipc_hme_user->bound_bits = bound_bits;
	pcie_ipc_hme_user->flags = flags;
	pcie_ipc_hme_user->pages = pages;

	HME_PRINT("HME Attach %s pages %u\n", pcie_ipc_hme_user->name, pages);

	return BCME_OK;

}   // hme_attach_mgr()

/**
 * +--------------------------------------------------------------------------+
 *
 * PCIE IPC HME Bind Phase: Invoked by pciedev during bind phase.
 *
 * If a User feature is enabled and an explicit user HME configuration was
 * not applied using hme_attach_mgr, then the system defaults are applied.
 *
 * The PCIE IPC structure is data filled with per user HME page requests.
 *
 * +--------------------------------------------------------------------------+
 */
int
BCMATTACHFN(hme_bind_pcie_ipc)(void *pciedev, pcie_ipc_t *pcie_ipc)
{
	uint32       user_id, hme_pages;
	hme_sys_t  * hme_sys = HME_SYS();

	HME_DEBUG(HME_PRINT("PCIE IPC HME Bind"));

	HME_ASSERT(pcie_ipc != (pcie_ipc_t*) NULL);

	hme_pages = 0U; // used to set pcie_ipc dcap1

	// hme_sys->pciedev_p = pciedev;

	for (user_id = 0; user_id < HME_USERS_TOTAL; ++user_id)
	{
		hme_user_t * hme_user = HME_USER(hme_sys, user_id);
		pcie_ipc_hme_user_t * pcie_ipc_hme_user;
		pcie_ipc_hme_user   = PCIE_IPC_HME_USER(hme_sys, user_id);

		// Check whether user was explicity pre-configured by hme_attach_mgr()
		if ((hme_user->state & HME_BIND_PEND) == 0) {  // hme_attach_mgr cleared
			HME_ASSERT(hme_user->pages == pcie_ipc_hme_user->pages);
			hme_pages += hme_user->pages;
			continue;
		}

		// Apply default build driven values

		// HME manager handlers are attached in Link phase and state set ISUP
		switch (user_id)
		{
			case HME_USER_SCRMEM:
				hme_user->pages  = PCIE_IPC_HME_PAGES(HME_USER_SCRMEM_BYTES);
				hme_user->policy = HME_MGR_SGMT;
				break;
#ifdef HME_PKTPGR_BUILD
			case HME_USER_PKTPGR:
				hme_user->pages  = PCIE_IPC_HME_PAGES(HME_USER_PKTPGR_BYTES);
				hme_user->policy = HME_MGR_NONE;
				break;
#endif /* HME_PKTPGR_BUILD */
#ifdef HME_MACIFS_BUILD
			case HME_USER_MACIFS:
				hme_user->pages  = PCIE_IPC_HME_PAGES(HME_USER_MACIFS_BYTES);
				hme_user->policy = HME_MGR_SGMT;
				break;
#endif /* HME_MACIFS_BUILD */
#ifdef HME_LCLPKT_BUILD
			case HME_USER_LCLPKT:
				hme_user->pages  = PCIE_IPC_HME_PAGES(HME_USER_LCLPKT_BYTES);
				hme_user->policy = HME_MGR_POOL;
				hme_user->mgr.pool.item_size = HME_USER_LCLPKT_ITEM_SIZE;
				hme_user->mgr.pool.items_max = HME_USER_LCLPKT_ITEMS_MAX;
				break;
#endif /* HME_LCLPKT_BUILD */
#ifdef HME_PSQPKT_BUILD
			case HME_USER_PSQPKT:
				hme_user->pages  = PCIE_IPC_HME_PAGES(HME_USER_PSQPKT_BYTES);
				hme_user->policy = HME_MGR_NONE;
				break;
#endif /* HME_PSQPKT_BUILD */
#ifdef HME_CSIMON_BUILD
			case HME_USER_CSIMON:
				hme_user->pages  = PCIE_IPC_HME_PAGES(HME_USER_CSIMON_BYTES);
				hme_user->policy = HME_MGR_NONE;
				break;
#endif /* HME_CSIMON_BUILD */
#ifdef HME_HMOSWP_BUILD
			case HME_USER_HMOSWP: // sw_paging_bind() configures HME_USER_HMOSWP
				break;
#endif /* HME_HMOSWP_BUILD */
#ifdef HME_MBXPGR_BUILD
			case HME_USER_MBXPGR:
			{
				hme_user->pages  = PCIE_IPC_HME_PAGES(HME_USER_MBXPGR_BYTES);
				hme_user->policy = HME_MGR_SGMT;
				break;
			}
#endif /* HME_MBXPGR_BUILD */
			default: // undef users or feature is disabled
				break;
		} // switch user_id

		pcie_ipc_hme_user->pages = hme_user->pages;
		hme_pages += hme_user->pages;

		hme_user->state  = HME_LINK_PEND; // also for undef users
	} // for each user_id

	if (hme_pages) {
		HME_PRINT("HME bind PCIE IPC Request total %u pages\n", hme_pages);
		pcie_ipc->dcap1 |= PCIE_IPC_DCAP1_HOST_MEM_EXTN;
	}

	pcie_ipc->host_mem_users   = HME_USERS_TOTAL;
	pcie_ipc->host_mem_size    = HME_PCIE_IPC_SIZE;
	pcie_ipc->host_mem_daddr32 = (uint32)(uintptr)(PCIE_IPC_HME(hme_sys));

	hme_sys->pcie_ipc_p        = pcie_ipc;

	HME_DEBUG(__hme_sys_audit(pcie_ipc, FALSE));

	return BCME_OK;

}   // hme_bind_pcie_ipc()

/**
 * +--------------------------------------------------------------------------+
 *
 * PCIE IPC HME Link Phase: Invoked by pciedev during link phase.
 *
 * + Audits DHD compatability with HME and its ability to service all user's HME
 *   request.
 * + Table of per user HME addresses are fetched from host memory via backplane
 *   copy to local stack memory.
 * + User's memory manager are initialized with the memory extension in host.
 * + User is marked as ISUP, upon binding the allocation/deallocation handlers.
 *
 * +--------------------------------------------------------------------------+
 */

int // HME Users Memory Extensions are retrieved from PCIE IPC in Link Phase
hme_link_pcie_ipc(void *pciedev, pcie_ipc_t *pcie_ipc)
{
	uint32       user_id, hme_dcap, hme_hcap, hme_pages;
	haddr64_t    pcie_ipc_hme_haddr64[HME_USERS_TOTAL]; // on stack!
	hme_sys_t  * hme_sys = HME_SYS();

	HME_DEBUG(HME_PRINT("PCIE IPC LINK HME"));
	memset(pcie_ipc_hme_haddr64, 0, sizeof(pcie_ipc_hme_haddr64));

	HME_DEBUG(hme_dump_pcie_ipc(pciedev, pcie_ipc));

	HME_ASSERT(pcie_ipc != (pcie_ipc_t*) NULL);

	hme_dcap = (pcie_ipc->dcap1 & PCIE_IPC_DCAP1_HOST_MEM_EXTN) ? 1 : 0;
	hme_hcap = (pcie_ipc->hcap1 & PCIE_IPC_HCAP1_HOST_MEM_EXTN) ? 1 : 0;

	// dcap::PCIE_IPC_DCAP1_HOST_MEM_EXTN is set only-if at-least-one HME page
	// was requested by any HME user
	if (!hme_dcap) // not even a single HME page requested across all users
		goto done;

	// HME service handshake log
	HME_PRINT("HME LINK PCIE IPC dcap<%u> hcap<%u> HME len<%u>"
		HADDR64_FMT "\n", hme_dcap, hme_hcap,
		pcie_ipc->host_mem_len, HADDR64_VAL(pcie_ipc->host_mem_haddr64));

	// Check whether DHD is HME compatible, as at-least-one page requested
	if (!hme_hcap) { // ERROR: DHD not compatible
		HME_PRINT("ERROR: HME Link Failure hcap 0x%08x\n", pcie_ipc->hcap1);
		HME_PRINT("ERROR: DHD not HME compatible\n");
		goto failure;
	}

	// Check whether DHD can service all HME requests
	if (pcie_ipc->host_mem_len == 0U) { // length is sum total of all requests
		HME_PRINT("ERROR: DHD cannot fulfill all HME requests\n");
		goto failure;
	}

	// Fetch the entire HME table from host memory to dongle local
	ASSERT(!HADDR64_IS_ZERO(pcie_ipc->host_mem_haddr64));
	_hme_table_copy(pcie_ipc->host_mem_haddr64, (uintptr)pcie_ipc_hme_haddr64);

	// Compute total HME pages requested using hme_sys_g and audit against DHD
	hme_pages = 0U;
	for (user_id = 0; user_id < HME_USERS_TOTAL; user_id++)
	{
		uint16       pcie_ipc_hme_pages;
		pcie_ipc_hme_user_t * pcie_ipc_hme_user;
		hme_user_t * hme_user = HME_USER(hme_sys, user_id);
		pcie_ipc_hme_user     = PCIE_IPC_HME_USER(hme_sys, user_id);
		pcie_ipc_hme_pages    = pcie_ipc_hme_user->pages;

		if (hme_user->pages == 0) {
			ASSERT(pcie_ipc_hme_pages == 0);
			ASSERT(HADDR64_IS_ZERO(pcie_ipc_hme_haddr64[user_id]));
			continue;
		}

		// HME user config must not change after pciedev handshake with DHD
		ASSERT(hme_user->pages == pcie_ipc_hme_pages);
		ASSERT(!HADDR64_IS_ZERO(pcie_ipc_hme_haddr64[user_id]));

		HME_ASSERT(hme_user->state == HME_LINK_PEND);

		hme_pages += pcie_ipc_hme_pages;
	}

	// Check whether host could fulfill all HME requests
	if (pcie_ipc->host_mem_len != PCIE_IPC_HME_BYTES(hme_pages)) {
		HME_PRINT("ERROR: HME Allocated memory %u != Requested %u bytes\n",
			pcie_ipc->host_mem_len, PCIE_IPC_HME_BYTES(hme_pages));
		goto failure;
	}

	// Complete HME manager configuration
	for (user_id = 0; user_id < HME_USERS_TOTAL; user_id++)
	{
		hme_user_t * hme_user = HME_USER(hme_sys, user_id);

		if (hme_user->pages == 0) // undef user or feature disabled
			continue;

		// Now, record the HME user bytes and haddr64 allocated by DHD
		hme_user->bytes = PCIE_IPC_HME_BYTES(hme_user->pages);

		HADDR64_LTOH(pcie_ipc_hme_haddr64[user_id]); // handle endian: redundant
		HADDR64_SET(hme_user->haddr64, pcie_ipc_hme_haddr64[user_id]);

		// Configure the HME manager and bind handlers
		switch (hme_user->policy) {
			case HME_MGR_SGMT:
			{
				hme_user->mgr.sgmt.front = 0U;
				hme_user->mgr.sgmt.bytes = hme_user->bytes;
				hme_user->get_fn = _hme_mgr_sgmt_get;
				hme_user->put_fn = _hme_mgr_sgmt_put;
				break;
			}

			case HME_MGR_POOL:
			{
				HME_ASSERT((hme_user->mgr.pool.item_size != 0) &&
				           (hme_user->mgr.pool.items_max != 0));
				ASSERT(hme_user->bytes == (uint32)
				 (hme_user->mgr.pool.item_size * hme_user->mgr.pool.items_max));

				hme_user->mgr.pool.mwbmap =
					bcm_mwbmap_init(hme_sys->osh, hme_user->mgr.pool.items_max);
				ASSERT(hme_user->mgr.pool.mwbmap != NULL);
				hme_user->get_fn = _hme_mgr_pool_get;
				hme_user->put_fn = _hme_mgr_pool_put;
				break;
			}

			default:
				break;

		} // switch user policy

		hme_user->state = HME_USER_ISUP; // now, user can avail of HME

	} // for each user_id

	{
		// Save pcie_ipc::host_mem_len, pcie_ipc::host_mem_haddr64 into hme_sys
		pcie_ipc_hme_t * pcie_ipc_hme;
		pcie_ipc_hme   = PCIE_IPC_HME(hme_sys);

		pcie_ipc_hme->bytes = pcie_ipc->host_mem_len;
		HADDR64_SET(pcie_ipc_hme->haddr64, pcie_ipc->host_mem_haddr64);
	}

	HME_DEBUG(hme_dump(FALSE));
	HME_DEBUG(__hme_sys_audit(pcie_ipc, TRUE));

	HME_PRINT("PCIE IPC LINK HME OK\n");

#ifdef HNDPQP
	pqp_link_hme();
#endif

done:
	return BCME_OK;

failure:
	HME_PRINT("PCIE IPC LINK HME KO\n");
	return BCME_ERROR;

}   // hme_link_pcie_ipc()

/**
 * +--------------------------------------------------------------------------+
 * PCIE IPC HME Dump (raw - no audit)
 * +--------------------------------------------------------------------------+
 */
void // Display the PCIE IPC HME dongle request and host response
hme_dump_pcie_ipc(void *pciedev, pcie_ipc_t *pcie_ipc)
{
	uint32    user_id, hme_dcap, hme_hcap;
	haddr64_t pcie_ipc_hme_haddr64[HME_USERS_TOTAL]; // On stack !

	HME_ASSERT(pcie_ipc != (pcie_ipc_t*) NULL);

	memset(pcie_ipc_hme_haddr64, 0, sizeof(pcie_ipc_hme_haddr64));

	hme_dcap = (pcie_ipc->dcap1 & PCIE_IPC_DCAP1_HOST_MEM_EXTN) ? 1 : 0;
	hme_hcap = (pcie_ipc->hcap1 & PCIE_IPC_HCAP1_HOST_MEM_EXTN) ? 1 : 0;

	HME_PRINT("\tHME dcap %u hcap %u host_mem len %u" HADDR64_FMT " page %u\n",
		hme_dcap, hme_hcap, pcie_ipc->host_mem_len,
		HADDR64_VAL(pcie_ipc->host_mem_haddr64), PCIE_IPC_HME_PAGE_SIZE);
	HME_PRINT("\tusers %u size %u daddr32 0x%08x\n",
		pcie_ipc->host_mem_users, pcie_ipc->host_mem_size,
		pcie_ipc->host_mem_daddr32);

	// Read the HME table from host memory to dongle local
	if ((hme_dcap & hme_hcap) && pcie_ipc->host_mem_len) {
		_hme_table_copy(pcie_ipc->host_mem_haddr64,
		                (uintptr)pcie_ipc_hme_haddr64);
	}

	if (pcie_ipc->host_mem_daddr32 != 0U) {
		pcie_ipc_hme_t      * pcie_ipc_hme;
		pcie_ipc_hme_user_t * pcie_ipc_hme_user;

		pcie_ipc_hme = (pcie_ipc_hme_t*)(uintptr)(pcie_ipc->host_mem_daddr32);

		HME_PRINT("\tversion %u users %u size %u bytes %u" HADDR64_FMT "\n\n",
			pcie_ipc_hme->version, pcie_ipc_hme->users, pcie_ipc_hme->size,
			pcie_ipc_hme->bytes, HADDR64_VAL(pcie_ipc_hme->haddr64));

		// Dump as-is the requested dngl user config and dhd allocated info
		HME_PRINT("\tID HMEUSR PAGES   BYTES HADDR_HI HADDR_LO ALN BND FLAG\n");
		for (user_id = 0; user_id < HME_USERS_TOTAL; user_id++) {
			pcie_ipc_hme_user = &pcie_ipc_hme->user[user_id];
			HME_PRINT("\t%2u %6s %5u %7u %08x %08x %3u %3u %04x\n",
				pcie_ipc_hme_user->user_id, pcie_ipc_hme_user->name,
				pcie_ipc_hme_user->pages,
				PCIE_IPC_HME_BYTES(pcie_ipc_hme_user->pages),
				HADDR64_VAL(pcie_ipc_hme_haddr64[user_id]),
				pcie_ipc_hme_user->align_bits, pcie_ipc_hme_user->bound_bits,
				pcie_ipc_hme_user->flags);
		}
	}

}   // hme_dump_pcie_ipc()

#ifdef HNDBME
/**
 * Library of wrapper routines to leverage ByteMoveEngine based DMA transfers
 * to/from Host Memory Extensions.
 */

/**
 * HME registers itself as BME User H2D and D2H.
 */
int
BCMATTACHFN(hme_bind_bme)(void)
{
	bme_sel_t    bme_sel_any;
	bme_set_t    bme_set_any;
	hme_sys_t  * hme_sys = HME_SYS();
	HME_ASSERT(hme_sys->osh != NULL); // ensure hme_init invoked

	bme_sel_any     = BME_SEL_ANY;
	bme_set_any.map = BME_SET_ANY;
	// hi_src, hi_dst defaults to 0

	HME_ASSERT(hme_sys->bme_key[BME_USR_H2D] == BCME_ERROR);
	hme_sys->bme_key[BME_USR_H2D] = bme_register_user(hme_sys->osh, BME_USR_H2D,
		bme_sel_any, bme_set_any, BME_MEM_PCIE, BME_MEM_DNGL, 0U, 0U);
	HME_DEBUG(HME_PRINT("HME registered user BME_USR_H2D Key[0x%08x]\n",
		hme_sys->bme_key[BME_USR_H2D]));

	HME_ASSERT(hme_sys->bme_key[BME_USR_D2H] == BCME_ERROR);
	hme_sys->bme_key[BME_USR_D2H] = bme_register_user(hme_sys->osh, BME_USR_D2H,
		bme_sel_any, bme_set_any, BME_MEM_DNGL, BME_MEM_PCIE, 0U, 0U);
	HME_DEBUG(HME_PRINT("HME registered user BME_USR_D2H Key[0x%08x]\n",
		hme_sys->bme_key[BME_USR_D2H]));

	return BCME_OK;

}   // hme_bind_bme()

int
hme_d2h_xfer(const void *src_daddr32, haddr64_t *dst_haddr64, uint32 bytes,
             bool sync, int hme_user_id)
{
	int          cpy_key;
	uint64       src_daddr_u64, dst_haddr_u64;
	hme_sys_t  * hme_sys = HME_SYS();

	HME_ASSERT(hme_sys->bme_key[BME_USR_D2H] != BCME_ERROR);

	// Well known HME user requesting allocation operation
	if (hme_user_id != HME_USER_NOOP)
	{
		hme_user_t * hme_user = HME_USER(hme_sys, hme_user_id);
		BCM_REFERENCE(hme_user);

		// Audit parameters
		HME_ASSERT(hme_user_id < HME_USERS_TOTAL);
		HME_ASSERT(hme_user->policy == HME_MGR_POOL);
		HME_ASSERT(hme_user->mgr.pool.item_size >= bytes);

		*dst_haddr64 = hme_get(hme_user_id, bytes); // bytes ignored

		if (HADDR64_IS_ZERO(*dst_haddr64)) {
			HME_DEBUG(HME_PRINT("hme_d2h_xfer no mem\n"));
			return BCME_NOMEM;
		}
	}

	src_daddr_u64 = LOHI32_TO_U64(src_daddr32, 0U);
	HADDR64_TO_U64((*dst_haddr64), dst_haddr_u64);
	cpy_key = bme_mcopy64(hme_sys->osh, hme_sys->bme_key[BME_USR_D2H],
	                     src_daddr_u64, dst_haddr_u64, bytes);

	if (cpy_key < 0) { // 0 is valid, cpy_key is the engine index
		HME_DEBUG(HME_PRINT("hme_d2h_xfer bme_copy failure %d\n", cpy_key));
		return cpy_key;
	}

	if (sync == TRUE) {
		bme_sync_eng(hme_sys->osh, cpy_key);
		return BCME_OK;
	}

	return cpy_key; // BME engine index, to be used in sync opration

}   // hme_d2h_xfer()

int
hme_h2d_xfer(haddr64_t *src_haddr64, void *dst_daddr32, uint32 bytes,
             bool sync, int hme_user_id)
{
	int          cpy_key;
	uint64       src_haddr_u64, dst_daddr_u64;
	hme_sys_t  * hme_sys = HME_SYS();

	HME_ASSERT(hme_sys->bme_key[BME_USR_H2D] != BCME_ERROR);

	HADDR64_TO_U64((*src_haddr64), src_haddr_u64);
	dst_daddr_u64 = LOHI32_TO_U64(dst_daddr32, 0U);
	cpy_key = bme_mcopy64(hme_sys->osh, hme_sys->bme_key[BME_USR_H2D],
	                     src_haddr_u64, dst_daddr_u64, bytes);

	if (cpy_key < 0) { // 0 is valid, cpy_key is the engine index
		HME_DEBUG(HME_PRINT("hme_h2d_xfer bme_copy failure %d\n", cpy_key));
		return cpy_key;
	}

	if (sync == TRUE) {
		bme_sync_eng(hme_sys->osh, cpy_key);
	}

	if (hme_user_id != HME_USER_NOOP)
	{
		hme_user_t * hme_user = HME_USER(hme_sys, hme_user_id);
		BCM_REFERENCE(hme_user);

		// Audit parameters
		HME_ASSERT(sync == TRUE);
		HME_ASSERT(hme_user_id < HME_USERS_TOTAL);
		HME_ASSERT(hme_user->policy == HME_MGR_POOL);
		HME_ASSERT(hme_user->mgr.pool.item_size >= bytes);

		cpy_key = hme_put(hme_user_id, bytes, *src_haddr64); // overwrite cpy_key
	}

	return cpy_key;

}   // hme_h2d_xfer()

void
hme_xfer_sync(int cpy_key)
{
	hme_sys_t * hme_sys = HME_SYS();
	HME_ASSERT(cpy_key > 0);
	bme_sync_eng(hme_sys->osh, cpy_key);

}   // hme_xfer_sync()

void
hme_h2d_sync(void)
{
	hme_sys_t * hme_sys = HME_SYS();
	HME_ASSERT(hme_sys->bme_key[BME_USR_H2D] != BCME_ERROR);
	bme_sync_usr(hme_sys->osh, BME_USR_H2D);
}   // hme_h2d_sync()

void
hme_d2h_sync(void)
{
	hme_sys_t * hme_sys = HME_SYS();
	HME_ASSERT(hme_sys->bme_key[BME_USR_D2H] != BCME_ERROR);
	bme_sync_usr(hme_sys->osh, BME_USR_D2H);
}   // hme_d2h_sync()

#endif /* HNDBME */
