/*
 * +--------------------------------------------------------------------------+
 *  HND Packet Queue Pager (PQP) library using synchronous mem2mem transfer.
 *
 *  Copyright 2022 Broadcom
 *
 *  This program is the proprietary software of Broadcom and/or
 *  its licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.  IF YOU HAVE NO
 *  AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 *  WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 *  THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1. This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use
 *  all reasonable efforts to protect the confidentiality thereof, and to
 *  use this information only in connection with your use of Broadcom
 *  integrated circuit products.
 *
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *  REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 *  OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *  DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *  NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *  ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *  CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *  OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 *  BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 *  SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 *  IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *  IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 *  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 *  OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 *  NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *  <<Broadcom-WL-IPTag/Proprietary:>>
 *
 *  $Id$
 *
 *  vim: set ts=4 noet sw=4 tw=80:
 *  -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * +--------------------------------------------------------------------------+
 */
#include <osl.h>
#include <bcmhme.h>
#include <sbtopcie.h>
#include <bcmpcie.h>
#include <bcmutils.h>
#include <hnd_pkt.h>
#include <hnd_pktq.h>
#include <hnd_pqp.h>
#if defined(BCMHWA)
#include <hwa_lib.h>
#include <hwa_export.h> // hwa_defs.h: HWA_PKTPGR_BUILD
#endif

#if defined(HNDPQP)
/*
 * +--------------------------------------------------------------------------+
 * XXX FIXME for production
 * A. Stall and recovery on resource availability.
 *    Need a scheme to "flush" stations occupying resources while in PS.
 * B. Save 4 PKTGET buf sizes of (256, 512, 1024, 2048).
 * C. _pqp_pgi_qcb invoked after every d11 pdu pgi[_pqp_pgi_req_pktq].
 *    Needs optimization.
 * +--------------------------------------------------------------------------+
 */

/**
 * +--------------------------------------------------------------------------+
 *  Revision History
 * +--------------------------------------------------------------------------+
 *   1.0    Asynchronous PQP (deprecated)
 *   2.0     Synchronous PQP version 2 revision 0
 * +--------------------------------------------------------------------------+
 */
#define PQP_VERSION                 "v2.0"

/**
 * +--------------------------------------------------------------------------+
 *  PQP Conditional Compile
 * +--------------------------------------------------------------------------+
 *  Designer builds for extended debug and statistics. Any modifications to
 *  PQP implementation must pass DEBUG, AUDIT and STATS builds enabled.
 * +--------------------------------------------------------------------------+
 */
//#define PQP_DEBUG_BUILD
//#define PQP_PKTIO_BUILD             // Assumes SBTOPCIE is setup for HBM IO
#define PQP_AUDIT_BUILD
#define PQP_STATS_BUILD

#define PQP_NOOP                    do { /* no-op */ } while(0)
#define PQP_PRINT(fmt, args...)     printf(fmt "\n", ##args)
#define PQP_ERROR(fmt, args...)     printf(fmt "\n", ##args)

#define PQP_CTRS_RDZERO             // Clear On Read
#if defined(PQP_CTRS_RDZERO)
#define PQP_CTR_ZERO(CTR)           (CTR) = 0U
#else   /* ! PQP_CTRS_RDZERO */
#define PQP_CTR_ZERO(CTR)           PQP_NOOP
#endif  /* ! PQP_CTRS_RDZERO */

#if defined(PQP_DEBUG_BUILD)
#define PQP_ASSERT(EXPR)            ASSERT(EXPR)
#define PQP_DEBUG(EXPR)             EXPR
#else  /* ! PQP_DEBUG_BUILD */
#define PQP_ASSERT(EXPR)            PQP_NOOP
#define PQP_DEBUG(EXPR)             PQP_NOOP
#endif /* ! PQP_DEBUG_BUILD */

#if defined(PQP_AUDIT_BUILD)
#define PQP_AUDIT(EXPR)             EXPR
#else
#define PQP_AUDIT(EXPR)             PQP_NOOP
#endif

#if defined(PQP_STATS_BUILD)
#define PQP_STATS(EXPR)             EXPR
#define PQP_STATS_ZERO(CTR)         PQP_CTR_ZERO(CTR)
#else   /* ! PQP_STATS_BUILD */
#define PQP_STATS(EXPR)             PQP_NOOP
#define PQP_STATS_ZERO(EXPR)        PQP_NOOP
#endif  /* ! PQP_STATS_BUILD */

/**
 * +--------------------------------------------------------------------------+
 *  PQP Pretty Print Formating
 * +--------------------------------------------------------------------------+
 */
#define PQP_REQ_HDR \
	"DLL DBMPDU HBMPDU HEAD TAIL DLL PktQueue APPS_SCB\n"
#define PQP_REQ_FMT         " %6u %6u %4u %4u %s %08x %08x\n"
#define PQP_REQ_VAL(_pqp_req_)  \
	(_pqp_req_)->pkts.dbm_cnt, (_pqp_req_)->pkts.hbm_cnt,  \
	(_pqp_req_)->hbm_id.head, (_pqp_req_)->hbm_id.tail, \
	pqp_list_name[(_pqp_req_)->list_idx], \
	(int) (_pqp_req_)->pqp_pktq, (int) (_pqp_req_)->ctx

#define PQP_PKTQ_FMT        " PktQ %08x[r=%08x h=%08x, t=%08x, n=%u pgi=%u,%u]"
#define PQP_PKTQ_VAL(_pqp_pktq_) \
	(int) PQP_PKTQ_REQ(_pqp_pktq_), \
	(int) (_pqp_pktq_), (int) PQP_PKTQ_HEAD(_pqp_pktq_), \
	(int) PQP_PKTQ_TAIL(_pqp_pktq_), PQP_PKTQ_PKTS(_pqp_pktq_), \
	(int) PQP_PKTQ_CONT(_pqp_pktq_), (int) PQP_PKTQ_FILL(_pqp_pktq_)

/**
 * +--------------------------------------------------------------------------+
 *  PQP Constants
 * +--------------------------------------------------------------------------+
 */
#define PQP_MAP_NULL                ((bcm_mwbmap_t *) NULL)
#define PQP_REQ_NULL                ((pqp_req_t *) NULL)
#define PQP_CTX_NULL                ((void *) NULL)
#define PQP_PKTQ_NULL               ((pqp_pktq_t*) NULL)

#define PQP_DBM_PTR_NULL            ((void *) NULL)
#define PQP_HBM_PTR_NULL            ((uintptr) 0)

#define PQP_DBM_PKT_NULL            ((pqp_pkt_t *) PQP_DBM_PTR_NULL)
#define PQP_HBM_PKT_NULL            (PQP_HBM_PTR_NULL)

#define PQP_DBM_PTR_INV             ((void *) ((uintptr)(~0)))
#define PQP_HBM_PTR_INV             ((uintptr)~0)

#define PQP_HBM_ID_NULL             ((uint16)(0))
#define PQP_HBM_ID_INV              ((uint16)(~0))
#define PQP_HBM_PAGE_INV            ((uint32) (~0))

/** pqp_pktq_t (struct pktq_prec) accessors */
#define PQP_PKTQ_REQ(_pktq_)        ((_pktq_)->pqp_req)
#define PQP_PKTQ_HEAD(_pktq_)       ((_pktq_)->head)
#define PQP_PKTQ_TAIL(_pktq_)       ((_pktq_)->tail)
#define PQP_PKTQ_PKTS(_pktq_)       ((_pktq_)->n_pkts)
#define PQP_PKTQ_CONT(_pktq_)       ((_pktq_)->stall_count)
#define PQP_PKTQ_FILL(_pktq_)       ((_pktq_)->dequeue_count)
// Do not re-purpose pqp_pktq::max_pkts as it is used in spktq operations

/**
 * +--------------------------------------------------------------------------+
 *  PQP Packet Structure Accessor Macros (NoHWA, HWA2.1, HWA2.2)
 * +--------------------------------------------------------------------------+
 *      BUILD       HBM_PKT         DBM_PKT
 *      NoHWA       pqp_pkt_t       pqp_pkt_t
 *      HWA2.1      hwapkt_t        pqp_pkt_t
 *      HWA2.2      pqp_pkt_t       pqp_pkt_t
 * +--------------------------------------------------------------------------+
 */

#if defined(BCMHWA)
#if defined(HWA_PKTPGR_BUILD)
#define HBM2DBM_PKT(_hbm_pkt_)      ((pqp_pkt_t*) LBP(_hbm_pkt_))
#define DBM2HBM_PKT(_dbm_pkt_)      ((pqp_pkt_t*) LBP(_dbm_pkt_))

#define PQP_HBM_PAGE(_pkt_)         (PPLBUF(_pkt_)->hbm_pkt.page)
#define PQP_HBM_SEGID(_pkt_)        (PPLBUF(_pkt_)->hbm_pkt.seg_id)
#define PQP_HBM_ID(_pkt_)           (PPLBUF(_pkt_)->hbm_pkt.id)
#define PQP_DBM_ID(_pkt_)           (PKTMAPID(_pkt_))

#define PQP_HBM_NEXT(_pkt_)         (PPLBUF(_pkt_)->hbm_pkt.next)
#define PQP_HBM_LINK(_pkt_)         (PPLBUF(_pkt_)->hbm_pkt.link)

#define PQP_DBM_NEXT(_pkt_)         (PPLBUF(_pkt_)->context.control.next)
#define PQP_DBM_LINK(_pkt_)         (PPLBUF(_pkt_)->context.control.link)

#define PQP_PKT_CTX_SZ              (LBUFMGMTSZ)
#define PQP_PKT_BUF_SZ              (PQP_PKT_SZ - PQP_PKT_CTX_SZ)

#else  /* ! HWA_PKTPGR_BUILD */

#error "PQP only supported on 6715 with PktPgr"

#endif /* ! HWA_PKTPGR_BUILD */
#else  /* ! BCMHWA */
#error "PQP only supported with HWA"
#endif /* ! BCMHWA */

/**
 * +--------------------------------------------------------------------------+
 *  PQP Conversion Macros
 * +--------------------------------------------------------------------------+
 *
 *  - HME Pool Index is in reference to the HME base
 *  - DMA uintptr is used for BME mem2mem haddr64.lo
 *  - PIO uintptr is used for SB2PCIE_MODE_PRIVATE based Programmed IO over PCIe
 * +--------------------------------------------------------------------------+
 */

#define PQP_HBM_ID_VALID(_hbm_id_) \
	(((uint16)(_hbm_id_) != PQP_HBM_ID_INV) && \
	 ((uint16)(_hbm_id_) != PQP_HBM_ID_NULL))
#define PQP_ASSERT_HBM_ID(_hbm_id_) \
	PQP_ASSERT(PQP_HBM_ID_VALID(_hbm_id_))

#define PQP_HBM_UPTR_VALID(_hbm_uptr_) \
	(((uintptr)(_hbm_uptr_) != PQP_HBM_PTR_INV) && \
	 ((uintptr)(_hbm_uptr_) != PQP_HBM_PTR_NULL))
#define PQP_ASSERT_HBM_UPTR(_hbm_uptr_) \
	PQP_ASSERT(PQP_HBM_UPTR_VALID(_hbm_uptr_))

// Convert a PQP Host BM Packet DMA pointer to a PQP Host BM Packet index
#define PQP_HBM_DMA2ID(_pqp_mgr_, _hbm_dma_uptr_) \
({  uint16 _hbm_id_; \
	uintptr _hme_dma_base_ = (_pqp_mgr_)->hme_pkt.dma.uptr; \
	PQP_ASSERT_HBM_UPTR(_hbm_dma_uptr_); \
	_hbm_id_ = ((_hbm_dma_uptr_) - _hme_dma_base_) / PQP_PKT_SZ; \
	(uint16) _hbm_id_; \
})

// Convert a PQP Host BM Packet PIO pointer to a PQP Host BM Packet index
#define PQP_HBM_PIO2ID(_pqp_mgr_, _hbm_pio_uptr_) \
({  uint16 _hbm_id_; \
	uintptr _hme_pio_base_ = (_pqp_mgr_)->hme_pkt.pio_uptr; \
	PQP_ASSERT_HBM_UPTR(_hbm_pio_uptr_); \
	_hbm_id_ = ((_hbm_pio_uptr_) - _hme_pio_base_) / PQP_PKT_SZ; \
	(uint16) _hbm_id_; \
})

// Convert a PQP Host BM Packet index to a PQP Host BM Packet DMA pointer
#define PQP_HBM_ID2DMA(_pqp_mgr_, _hbm_id_) \
({  uintptr _hbm_dma_uptr_; \
	uintptr _hme_dma_base_ = (_pqp_mgr_)->hme_pkt.dma.uptr; \
	PQP_ASSERT_HBM_ID(_hbm_id_); \
	_hbm_dma_uptr_ = _hme_dma_base_ + ((_hbm_id_) * PQP_PKT_SZ); \
	(uintptr) _hbm_dma_uptr_; \
})

// Convert a PQP Host BM Packet index to a PQP Host BM Packet PIO pointer
#define PQP_HBM_ID2PIO(_pqp_mgr_, _hbm_id_) \
({  uintptr _hbm_pio_uptr_; \
	uintptr _hme_pio_base_ = (_pqp_mgr_)->hme_pkt.pio_uptr; \
	PQP_ASSERT_HBM_ID(_hbm_id_); \
	_hbm_pio_uptr_ = _hme_pio_base_ + ((_hbm_id_) * PQP_PKT_SZ); \
	(uintptr) _hbm_pio_uptr_; \
})

// Convert a PQP Host BM Packet PIO pointer to a PQP Host BM Packet DMA pointer
#define PQP_HBM_PIO2DMA(_pqp_mgr_, _hbm_pio_uptr_) \
({  uintptr _hbm_dma_uptr_; \
	uintptr _hme_dma_base_ = (_pqp_mgr_)->hme_pkt.dma.uptr; \
	uintptr _hme_pio_base_ = (_pqp_mgr_)->hme_pkt.pio_uptr; \
	PQP_ASSERT_HBM_UPTR(_hbm_pio_uptr_); \
	_hbm_dma_uptr_ = _hme_dma_base_ + ((_hbm_pio_uptr_) - _hme_pio_base_); \
	(uintptr) _hbm_dma_uptr_; \
})

// Convert a PQP Host BM Packet DMA pointer to a PQP Host BM Packet PIO pointer
#define PQP_HBM_DMA2PIO(_pqp_mgr_, _hbm_dma_uptr_) \
({  uintptr hbm_pio_uptr; \
	uintptr hbm_dma_uptr = (uintptr)(_hbm_dma_uptr_); \
	uintptr hme_dma_base = (_pqp_mgr_)->hme_pkt.dma.uptr; \
	uintptr hme_pio_base = (_pqp_mgr_)->hme_pkt.pio_uptr; \
	PQP_ASSERT_HBM_UPTR(hbm_dma_uptr); \
	hbm_pio_uptr = hme_pio_base + (hbm_dma_uptr - hme_dma_base); \
	(pqp_pkt_t*) hbm_pio_uptr; \
})

/**
 * +--------------------------------------------------------------------------+
 *  Packet Queue Pager Accessor Macros
 * +--------------------------------------------------------------------------+
 */

#define PQP_MGR() \
({ \
	pqp_mgr_t * _pqp_mgr_ = &pqp_mgr_g; \
	ASSERT((_pqp_mgr_)->req_pool != PQP_REQ_NULL); \
	_pqp_mgr_; \
})
#define PQP_DEV(_mgr_)          ((_mgr_)->dev)
#define PQP_HWA_DEV(_mgr_)      (struct hwa_dev *)(PQP_DEV(_mgr_))
#define PQP_RDY(_mgr_)          ((_mgr_)->fsm_bmap == 0U) // No stall pending

/**
 * +--------------------------------------------------------------------------+
 *  Packet Queue Pager Typedefs
 * +--------------------------------------------------------------------------+
 */
typedef struct pqp_stats        // Packet Queue Pager Statistics
{
	struct {
		uint32      req;        // Count of PageOp requests
		uint32      dma;        // Count of PageOp PKT requests
		uint32      res;        // Count of PageOp resumes
		uint32      oom;        // Count of PageOp OutOfMemory Request stalls
	} pgo, pgi;

	uint32          deplete;    // Free pool of pqp req is depeleted
	uint32          failure;    // Count of operational failures

} pqp_stats_t;

typedef union pqp_dma
{
	void          * ptr;        // pointer access
	uintptr         uptr;       // uintptr arithmetic in PQP_HBM_PKTPTR
	uint64          u64;        // BME mem2mem programming
	haddr64_t       haddr64;    // dma64addr[lo, hi]. lo used as uintptr
} pqp_dma_t;

typedef struct pqp_hme          // Host Memory Extension context
{
	pqp_dma_t       dma;        // Haddr64 of base of HME
	uintptr         pio_uptr;   // SB2PCIE remapped base address for PIO
	bcm_mwbmap_t  * mwbmap;     // Multi Word bitmap index allocator handle
	uint32          free_cnt;   // Indices free in multi word bmap allocator
	uint32          bytes;      // Total bytes in HME segment

} pqp_hme_t;

typedef enum pqp_list_idx       // Index of PQP manager lists
{
	PQP_LIST_IDX_REQ    =  0,   // List of free PQP requests
	PQP_LIST_IDX_HBM    =  1,   // List PQP requests fully in hbm
	PQP_LIST_IDX_PGO    =  2,   // List of PQP requests awaiting HBM resources
	PQP_LIST_IDX_PGI    =  3,   // List of PQP requests awaiting DBM resources
	PQP_LIST_IDX_MAX    =  4

} pqp_list_idx_t;

typedef struct pqp_list         // List of PQP requests
{
	dll_t           dll;        // dll node for list
	uint16          idx;        // List index
	uint16          cnt;        // Count of PQP requests in list

} pqp_list_t;

/**
 * +--------------------------------------------------------------------------+
 *  Tracking of a PQP managed pqp_pktq, with partial page outs.
 *
 *  List of MPDUs a->b->c->d->e->f->g->h->i in DBM or HBM
 *
 *    pqp_pktq_t   |  pqp_req_t
 *  ---------------|--------------------------------------------------------
 *  DBM[a]->DBM[b] |  HBM[C]=>HBM[D]->DBM[e]->DBM[f]=>HBM[G]=>HBM[H]->DBM[i]
 *
 *  Since Head segment is in DBM, it is tracked in pqp_pktq_t
 *      pqp_pktq_t::head        = DBM[a]
 *      pqp_pktq_t::tail        = DBM[b]
 *      pqp_pktq_t::n_pkts      = 2
 *
 *  Count of total MPDUs in DBM and HBM (not including those in pqp_pktq_t)
 *      pqp_req_t::pkts::dcnt   = 3 (e,f,i)
 *      pqp_req_t::pkts::hcnt   = 4 (C,D,G,H)
 *
 *  "First" HBM segment
 *      pqp_req_t::hbm_id::head = HBM[C]
 *      pqp_req_t::hbm_id::tail = HBM[D]
 *
 *  Last pkt is a DBM pkt, so hbm last page index is invalid
 *      pqp_req_t::hbm_id_last  = PQP_HBM_ID_INV
 *      pqp_req_t::dbm_last     = DBM[i]
 *
 * +--------------------------------------------------------------------------+
 */

typedef struct pqp_req          // Packet Queue Pager Request
{
	dll_t           node;       // Manage pqp_req in lists
	pqp_pktq_t    * pqp_pktq;   // Link back to paired pktq_prec_t
	void          * ctx;

	union {
		struct {                // DBM and HBM pkts managed by pqp_req
			uint16  dbm_cnt;    // Count of DBM pkts. excluding in pqp_pktq
			uint16  hbm_cnt;    // Count of HBM pkts
		};
		uint32      u32;
	} pkts;                     // Packets tracked by pqp_req

	union {
		struct {                // Host BM pkt list managed by pqp_req
			uint16  head;       // Head HBM pkt page index
			uint16  tail;       // Tail HBM pkt in first HBM Queue segment
		};
		uint32      page;       // Host BM: [head, tail] queue index
	} hbm_id;

	uint16          list_idx;   // list index in which pqp_req resides

} pqp_req_t;

/*
 * XXX FIXME B. Stall and recovery on resource availability.
 *
 * Presently, no callback scheme exists for the following resources.
 *      PQP PGO STALL: HME LCLPKT (managed by PktPgr)
 *      PQP PGI STALL: PKTGET() OS_MEM_AVAIL() < MEM_LOWATLIMIT
 * Recovery for above 2 cases is via another pqp_pgo() or pqp_pgi().
 * Need a scheme to "flush" a station that is occupying resources while in PS.
 */
typedef enum pqp_fsm_evt        // PQP Manager finite state machine event
{
	PQP_FSM_DBM_OOM     =  1,   // PQP Manager stalled on DBM resource
	PQP_FSM_PGI_STR     =  2,   // PQP Manager PGI start
	PQP_FSM_PGI_SPL     =  3,   // PQP Manager PGI special handling(like PS flush,
	                            // map_pkts_cb_fn, PS age-out)
	PQP_FSM_EVT_MAX     =  4
} pqp_fsm_evt_t;

typedef struct pqp_mgr          // Packet Queue Pager Manager
{
	uint32          fsm_map;    // bitmap of pqp_fsm_evt_t stalls
	union {
		struct {                // DBM and HBM pkts managed by pqp_req
			uint16  dbm_cnt;    // Count of DBM pkts. excluding in pqp_pktq
			uint16  hbm_cnt;    // Count of HBM pkts
		};
		uint32      u32;
	} pkts;                     // Packets tracked by pqp_req

	int             hbm_avail_sw;  // Available HBM Packets, SW accounting

	pqp_hme_t       hme_pkt;    // Host Memory Extension for HBM Packets
	pqp_hme_t       hme_lcl;    // HME manages LCL pool, shared with HWA_PP

	struct {
		uint32      free_cnt;   // Free DDBM HWA packets
		uint32      mem_avail;  // Free memory for PKTGET
		uint32      buff_cnt;   // Free heap buffer for LCL_PKT
	} dbm_pkt;
	                            // PQP requests exist is one of below lists
	pqp_list_t      list[PQP_LIST_IDX_MAX];
	                            // H2D and D2H User keys in BME service
	int             bme_h2d;    // BME key for user BME_USR_H2D
	int             bme_d2h;    // BME key for user BME_USR_D2H
	                            // Callbacks: Page Out Packet and Page In Queue
	pqp_cb_fn_t     pgo_pcb;    // Page Out Packet Callback
	pqp_cb_fn_t     pgi_qcb;    // Page In Queue Callback

	osl_t         * wl_osh;     // WL AP PS module osh
	osl_t         * rte_osh;    // Backplane RTE osh
	void          * dev;        // see pqp_config()

	pqp_req_t     * req_pool;   // Memory for pool of pqp_req
	uint32          req_max;    // Total number of pqp_req in pool

#if defined(PQP_STATS_BUILD)
	pqp_stats_t     stats;      // Manager statistics
#endif

} pqp_mgr_t;

static pqp_mgr_t pqp_mgr_g = { .req_pool = PQP_REQ_NULL };

const char * pqp_list_name[PQP_LIST_IDX_MAX] = { "REQ", "HBM", "PGO", "PGI" };
const char * pqp_fsm_evt_name[PQP_FSM_EVT_MAX] = { "HBM_OOM", "DBM_OOM", "PGI_STR" };
const char * pqp_policy_name[PQP_POLICY_MAX] = { "PREPEND", " APPEND" };

/**
 * +--------------------------------------------------------------------------+
 *  Section: PQP FSM Helper Routines
 * +--------------------------------------------------------------------------+
 */
static INLINE uint32
__pqp_fsm_evt_get(pqp_mgr_t * pqp_mgr, pqp_fsm_evt_t fsm_evt)
{
	return (pqp_mgr->fsm_map & (1 << fsm_evt));
}   // __pqp_fsm_evt_get()

static INLINE void // Transition PQP manager FSM into a stall
__pqp_fsm_evt_set(pqp_mgr_t * pqp_mgr, pqp_fsm_evt_t fsm_evt)
{
	PQP_DEBUG(PQP_PRINT("PQP: FSM set %s", pqp_fsm_evt_name[fsm_evt]));
	switch (fsm_evt) {
		case PQP_FSM_DBM_OOM:
			PQP_STATS(pqp_mgr->stats.pgi.oom++);
			break;
		default: break;
	}

	pqp_mgr->fsm_map |= (1 << fsm_evt);

}   // __pqp_fsm_evt_set()

static INLINE void // Transition PQP manager FSM out of a stall
__pqp_fsm_evt_clr(pqp_mgr_t * pqp_mgr, pqp_fsm_evt_t fsm_evt)
{
	PQP_DEBUG(PQP_PRINT("PQP: FSM clr %s", pqp_fsm_evt_name[fsm_evt]));
	switch (fsm_evt) {
		case PQP_FSM_DBM_OOM:
			PQP_STATS(pqp_mgr->stats.pgi.res++);
			break;
		default: break;
	}

	pqp_mgr->fsm_map &= ~(1 << fsm_evt);

}   // __pqp_fsm_evt_clr()

static INLINE uint32
__pqp_pgi_dbm_noresource(pqp_mgr_t * pqp_mgr)
{
	uint32 noresource;
	uint32 buf_thresh, dbm_thresh;

	// Reserve buf is used for the scenario of map_pkts_cb, pkt flush, and ps timeout
	if (__pqp_fsm_evt_get(pqp_mgr, PQP_FSM_PGI_SPL)) {
		dbm_thresh = PQP_PGI_DBM_THRESH_RSV;
		buf_thresh = 0;
	} else {
		dbm_thresh = PQP_PGI_DBM_THRESH;
		buf_thresh = PQP_PGI_BUF_THRESH;
	}

	if (pqp_mgr->dbm_pkt.free_cnt < PQP_PGI_DBM_THRESH) {
		// Try get more from HWA
		hwa_pktpgr_pqplbufpool_fill(PQP_HWA_DEV(pqp_mgr));
		hwa_pktpgr_pagemgr_alloc_pqp_pkt_wait_to_finish(PQP_HWA_DEV(pqp_mgr));
		// Update new free_cnt
		pqp_mgr->dbm_pkt.free_cnt  = hwa_pqp_pkt_cnt(PQP_HWA_DEV(pqp_mgr));
		PQP_DEBUG(PQP_PRINT("PQP: new dbm_pkt.free_cnt<%d>", pqp_mgr->dbm_pkt.free_cnt));
	}

	noresource = (pqp_mgr->dbm_pkt.free_cnt < dbm_thresh)
		| (pqp_mgr->dbm_pkt.buff_cnt <= buf_thresh);

	return noresource;
}   // __pqp_pgi_dbm_noresource()

/**
 * +--------------------------------------------------------------------------+
 *  Section: PQP List Helper Routines
 *
 *  Algorithms must use these INLINE __pqp_list_###() APIs allowing designer
 *  debug support to be added in terms of PQP_ASSERT, Audits, FSM Logging, etc.
 * +--------------------------------------------------------------------------+
 */

static INLINE pqp_list_t * // Given a PQP list index, fetch pqp_list
__pqp_list(pqp_mgr_t *pqp_mgr, uint16 list_idx)
{
	pqp_list_t * pqp_list;
	PQP_ASSERT(list_idx < PQP_LIST_IDX_MAX);
	pqp_list = &pqp_mgr->list[list_idx];
	PQP_ASSERT(pqp_list->idx == list_idx);
	return pqp_list;
}   // __pqp_list()

static INLINE void
__pqp_list_audit(pqp_list_t * pqp_list)
{
#if defined(PQP_AUDIT_BUILD)
	uint16   list_cnt_calc = 0;
	uint16   list_cnt_audit;
	dll_t  * iter_node;
	dll_t  * list_node = &pqp_list->dll;

	dll_for_each(iter_node, list_node) {
		pqp_req_t * pqp_req = CONTAINEROF(iter_node, pqp_req_t, node);
		BCM_REFERENCE(pqp_req);
		PQP_ASSERT(pqp_list->idx == pqp_req->list_idx);
		list_cnt_calc++; // iteration can fail
	}
	list_cnt_audit = pqp_list->cnt;
	if (list_cnt_audit != list_cnt_calc) {
		PQP_PRINT("PQP: __pqp_list_audit %u failure cnt %u found %u\n",
			pqp_list->idx, list_cnt_audit, list_cnt_calc);
		PQP_ASSERT(list_cnt_audit == list_cnt_calc);
	}
#endif /* PQP_AUDIT_BUILD */
}   // __pqp_list_audit()

static INLINE uint16
__pqp_list_cnt(pqp_list_t * pqp_list)
{
	if (pqp_list->cnt == 0)
		PQP_ASSERT(dll_empty(&pqp_list->dll));
	return pqp_list->cnt;
}

static INLINE pqp_req_t *
__pqp_list_head(pqp_list_t * pqp_list)
{
	dll_t      * dll_node = dll_head_p(&pqp_list->dll);
	pqp_req_t  * pqp_req  = CONTAINEROF(dll_node, pqp_req_t, node);
	PQP_ASSERT(__pqp_list_cnt(pqp_list) != 0);
	return pqp_req;
}   // __pqp_list_head()

// PQP DLL List Management helper routines
static INLINE void // Initialize a PQP list
__pqp_list_init(pqp_list_t * pqp_list, uint16 list_idx)
{
	dll_init(&pqp_list->dll);
	pqp_list->cnt = 0;
	pqp_list->idx = list_idx;
}   // __pqp_list_init()

static INLINE void // Delete a PQP request from a specified PQP list
__pqp_list_delete(pqp_list_t * pqp_list, pqp_req_t * pqp_req)
{
	dll_delete(&pqp_req->node);
	pqp_list->cnt--;
	__pqp_list_audit(pqp_list);
}   // __pqp_list_delete()

static INLINE pqp_req_t * // Delete and return the PQP request at PQP list head
__pqp_list_pop(pqp_list_t * pqp_list)
{
	pqp_req_t * pqp_req = __pqp_list_head(pqp_list);
	__pqp_list_delete(pqp_list, pqp_req);
	return pqp_req;
}   // __pqp_list_pop()

static INLINE void // Append a PQP request to tail of a specified PQP list
__pqp_list_append(pqp_list_t * pqp_list, pqp_req_t * pqp_req)
{
	dll_append(&pqp_list->dll, &pqp_req->node);
	pqp_list->cnt++;
	pqp_req->list_idx = pqp_list->idx;
	__pqp_list_audit(pqp_list);
}   // __pqp_list_append()

static INLINE void // Prepend a PQP request to head of a specified PQP list
__pqp_list_prepend(pqp_list_t * pqp_list, pqp_req_t * pqp_req)
{
	dll_prepend(&pqp_list->dll, &pqp_req->node);
	pqp_list->cnt++;
	pqp_req->list_idx = pqp_list->idx;
	__pqp_list_audit(pqp_list);
}   // __pqp_list_prepend()

static INLINE void // Transfer a PQP request from a src to tail of dst PQP list
__pqp_list_transfer(pqp_list_t * pqp_list_src, pqp_list_t * pqp_list_dst,
	pqp_req_t * pqp_req)
{
	__pqp_list_delete(pqp_list_src, pqp_req);
	__pqp_list_append(pqp_list_dst, pqp_req);
}   // __pqp_list_transfer()

static INLINE void // Transfer a PQP request from a src to head of dst PQP list
__pqp_list_transfer_head(pqp_list_t * pqp_list_src, pqp_list_t * pqp_list_dst,
	pqp_req_t * pqp_req)
{
	__pqp_list_delete(pqp_list_src, pqp_req);
	__pqp_list_prepend(pqp_list_dst, pqp_req);
}   // __pqp_list_transfer_head()

/**
 * +--------------------------------------------------------------------------+
 *  Section: PQP Packet Queue and Packet Helper Routines
 * +--------------------------------------------------------------------------+
 */
static INLINE void // PGO: Reset a pktq managed packet's HBM context
__pqp_pktq_sdu_reset(pqp_pkt_t * pqp_pkt)
{
	// Mark a pktq managed packet's HBM information as INVALID
	PQP_HBM_PAGE(pqp_pkt) = PQP_HBM_PAGE_INV; // hbm_pkt::id, hbm_pkt::segid
	PQP_HBM_NEXT(pqp_pkt) = PQP_HBM_PTR_INV;  // hbm_pkt::next
	PQP_HBM_LINK(pqp_pkt) = PQP_HBM_PTR_INV;  // hbm_pkt::link
}   // __pqp_pktq_sdu_reset()

static INLINE void // PGO: Reset all pktq managed packet's HBM context
__pqp_pktq_pdus_reset(pqp_pktq_t * pktq_pktq)
{
	pqp_pkt_t * pdu;       // iterate through PKTLINK list of mpdus
	pqp_pkt_t * sdu;       // iterate through PKTNEXT list of msdus
	int         pkts = 0;  // count of pdus encountered for auditing

	pdu = (pqp_pkt_t*) PQP_PKTQ_HEAD(pktq_pktq);
	while (pdu != PQP_DBM_PKT_NULL)       // traverse DBM PKTLINK list of sdu(s)
	{
		pkts++;                           // count of pdus
		sdu = pdu;
		while (sdu != PQP_DBM_PKT_NULL) { // traverse DBM PKTNEXT list of sdu(s)
			__pqp_pktq_sdu_reset(sdu);
			sdu = PQP_DBM_NEXT(sdu);
		}
		pdu = PQP_DBM_LINK(pdu);
	}

	PQP_ASSERT(pkts == PQP_PKTQ_PKTS(pktq_pktq)); // audit
}   // __pqp_pktq_pdus_reset()

static void // PGO: Empty the pktq after packets are moved into PQP request
_pqp_pktq_empty(pqp_pktq_t * pqp_pktq)
{
	PQP_PKTQ_HEAD(pqp_pktq) = PQP_DBM_PKT_NULL;
	PQP_PKTQ_TAIL(pqp_pktq) = PQP_DBM_PKT_NULL;
	PQP_PKTQ_PKTS(pqp_pktq) = 0;
}   // _pqp_pktq_empty()

static INLINE pqp_req_t * // Fetch paired PQP request with a PQP pktq
__pqp_pktq_req_get(pqp_pktq_t * pqp_pktq)
{
	pqp_req_t * pqp_req;
	PQP_ASSERT(pqp_pktq != PQP_PKTQ_NULL);
	pqp_req = PQP_PKTQ_REQ(pqp_pktq);
	if (pqp_req != PQP_REQ_NULL) {
		PQP_ASSERT(pqp_req->pqp_pktq == pqp_pktq);
	}
	return pqp_req;
}   // __pqp_pktq_req_get()

static INLINE pqp_req_t *
__pqp_pktq_req_clr(pqp_pktq_t * pqp_pktq)
{
	pqp_req_t    * pqp_req = PQP_PKTQ_REQ(pqp_pktq);
	PQP_PKTQ_REQ(pqp_pktq) = PQP_REQ_NULL;
	return pqp_req;
}   // __pqp_pktq_req_clr()

/**
 * +--------------------------------------------------------------------------+
 *  Section: PQP Request Helper Routines
 * +--------------------------------------------------------------------------+
 */
// Reset a PQP request.
static INLINE void // Clear a pqp request
__pqp_req_reset(pqp_req_t * pqp_req)
{
	pqp_req->pqp_pktq    = PQP_PKTQ_NULL;
	pqp_req->ctx         = PQP_CTX_NULL;
	pqp_req->pkts.u32    = 0U;               // pkts.dbm_cnt = pkts.hbm_cnt = 0
	pqp_req->hbm_id.page = PQP_HBM_PAGE_INV; // hbm_id.head = hbm_id.tail = ~0
}   // __pqp_req_reset()

/**
 * +--------------------------------------------------------------------------+
 *  Section: PQP Manager Helper Routines
 * +--------------------------------------------------------------------------+
 */

static INLINE void // PQP transfer a request from one list to another at tail
__pqp_mgr_transfer(pqp_mgr_t * pqp_mgr, pqp_req_t * pqp_req,
	pqp_list_idx_t pqp_list_idx_src, pqp_list_idx_t pqp_list_idx_dst)
{
	pqp_list_t * pqp_list_src = __pqp_list(pqp_mgr, pqp_list_idx_src);
	pqp_list_t * pqp_list_dst = __pqp_list(pqp_mgr, pqp_list_idx_dst);

	__pqp_list_transfer(pqp_list_src, pqp_list_dst, pqp_req);
}   // __pqp_mgr_transfer()

static INLINE void // PQP transfer a request from one list to another at head
__pqp_mgr_transfer_head(pqp_mgr_t * pqp_mgr, pqp_req_t * pqp_req,
	pqp_list_idx_t pqp_list_idx_src, pqp_list_idx_t pqp_list_idx_dst)
{
	pqp_list_t * pqp_list_src = __pqp_list(pqp_mgr, pqp_list_idx_src);
	pqp_list_t * pqp_list_dst = __pqp_list(pqp_mgr, pqp_list_idx_dst);

	__pqp_list_transfer_head(pqp_list_src, pqp_list_dst, pqp_req);
}   // __pqp_mgr_transfer_head()

// PQP Manager allocation/deallocation of PQP requests
static INLINE pqp_req_t * // Allocate a pqp request from pqp mgr's free list
__pqp_mgr_req_get(pqp_mgr_t * pqp_mgr)
{
	pqp_req_t  * pqp_req;
	pqp_list_t * pqp_list_req = __pqp_list(pqp_mgr, PQP_LIST_IDX_REQ);
	if (__pqp_list_cnt(pqp_list_req) == 0) {
		PQP_DEBUG(PQP_PRINT("PQP: __pqp_mgr_req_get pool depleted warning"));
		PQP_STATS(pqp_mgr->stats.deplete++);
		pqp_req = PQP_REQ_NULL;
	} else {
		pqp_list_t * pqp_list_pgo;
		pqp_list_pgo = __pqp_list(pqp_mgr, PQP_LIST_IDX_PGO);

		pqp_req = __pqp_list_pop(pqp_list_req);
		PQP_DEBUG(__pqp_req_reset(pqp_req));
		// Transfer new PQP request to tail of PGO list
		__pqp_list_append(pqp_list_pgo, pqp_req);
	}
	return pqp_req;
}   // __pqp_mgr_req_get()

static INLINE void // Clear a pqp request & deallocate into pqp mgr's free list
__pqp_mgr_req_put(pqp_mgr_t * pqp_mgr, pqp_req_t * pqp_req)
{
	pqp_list_t * pqp_list_src = __pqp_list(pqp_mgr, pqp_req->list_idx);
	pqp_list_t * pqp_list_req = __pqp_list(pqp_mgr, PQP_LIST_IDX_REQ);

	__pqp_list_delete(pqp_list_src, pqp_req);
	__pqp_req_reset(pqp_req);
	__pqp_list_prepend(pqp_list_req, pqp_req);

}   // __pqp_mgr_req_put()

static INLINE uint16 // Allocate a HBM id from the PQP manager mwbmap
__pqp_mgr_hbm_id_get(pqp_mgr_t * pqp_mgr)
{
	uint16 hbm_id;
	if (pqp_mgr->hme_pkt.free_cnt > 0) {
		hbm_id = (uint16) bcm_mwbmap_alloc(pqp_mgr->hme_pkt.mwbmap);
		pqp_mgr->hme_pkt.free_cnt--;
	} else {
		hbm_id = PQP_HBM_ID_INV;
		ASSERT(0); // design : caller confirms availability before allocation
	}
	return hbm_id;
}   // __pqp_mgr_hbm_id_get()

static INLINE void // De-allocate a HBM id into PQP manager mwbmap
__pqp_mgr_hbm_id_put(pqp_mgr_t * pqp_mgr, uint16 hbm_id)
{
	bcm_mwbmap_free(pqp_mgr->hme_pkt.mwbmap, (uint32)hbm_id);
	pqp_mgr->hme_pkt.free_cnt++;
}   // __pqp_mgr_hbm_id_put()

static INLINE pqp_pkt_t *
__pqp_mgr_dbm_get(pqp_mgr_t * pqp_mgr)
{
	pqp_pkt_t * dbm_pkt;
	dbm_pkt = (pqp_pkt_t *) hwa_pqp_pkt_get(PQP_HWA_DEV(pqp_mgr));
	if (dbm_pkt == PQP_DBM_PKT_NULL) {
		ASSERT(0); // design : caller confirm availability before allocation
	}
	pqp_mgr->dbm_pkt.free_cnt--;
	return dbm_pkt;
}   // __pqp_mgr_dbm_get()

static INLINE void
__pqp_mgr_dbm_put(pqp_mgr_t * pqp_mgr, pqp_pkt_t * dbm_pkt)
{
	PQP_DBM_LINK(dbm_pkt) = PQP_DBM_PTR_NULL; // delink
	// Reset HBM context and delink before put it back
	__pqp_pktq_sdu_reset(dbm_pkt);
	// When pqplbuf_use_rsv_ddbm is set, enqueue DDBM to pqplbufpool if resource is low.
	if (!hwa_pktpgr_pqplbufpool_enq(PQP_HWA_DEV(pqp_mgr), PPLBUF(dbm_pkt))) {
		hwa_pqp_pkt_put(PQP_HWA_DEV(pqp_mgr), (void*)dbm_pkt);
	}
}   // __pqp_mgr_dbm_put()

static INLINE void *
__pqp_mgr_heap_buf_get(pqp_mgr_t * pqp_mgr)
{
	void * heap_buf;
	heap_buf = lfbufpool_get(PQP_HEAP_BUF_POOL);
	if (heap_buf == (uchar *)NULL) {
		ASSERT(0); // design : caller confirm availability before allocation
	}
	pqp_mgr->dbm_pkt.buff_cnt--;
	return heap_buf;
}   // __pqp_mgr_heap_buf_get()

static INLINE void
__pqp_mgr_heap_buf_put(pqp_mgr_t * pqp_mgr, void * heap_buf)
{
	lfbufpool_free(heap_buf);
	pqp_mgr->dbm_pkt.buff_cnt++;
}   // __pqp_mgr_heap_buf_put()

/**
 * +--------------------------------------------------------------------------+
 *  Section: PQP Functional Interface
 * +--------------------------------------------------------------------------+
 */

#if defined(PQP_PKTIO_BUILD)

#define PKTF(_pkt_, _flag_, _fstr_) \
	(LBFP(_pkt_)->control.flags & (_flag_)) ? _fstr_ : ""

// Debug dump a HBM or DBM packet. Accesses HBM resident packet using SBTOPCIE
static void // _pqp_pktio_dump(sdu, "PGO", "LINK")
_pqp_pktio_dump(pqp_pkt_t * pkt, const char * op, const char * fsm_str)
{
	pqp_mgr_t   * pqp_mgr = PQP_MGR();
	osl_t       * osh     = pqp_mgr->wl_osh;

	// hwa_pp_lbuf::hbm_pkt::[next, link]
	uintptr  hbm_pkt_next = PQP_HBM_NEXT(pkt);
	uintptr  hbm_pkt_link = PQP_HBM_LINK(pkt);

	printf("PQP: %s PKT<%p> ID<M%04u H%04u P%u L%08x> FLAGS[%s%s%s%s%s%s%s%s]"
		" fsm %s\n",
		op, pkt, PKTMAPID(pkt), PPLBUF(pkt)->hbm_pkt.id, PKTPOOL(osh, pkt),
		PKTLOCAL(osh, pkt),
		PKTF(pkt, LBF_HWA_DDBM_PKT, "DBM "), PKTF(pkt, LBF_HEAPBUF,   "BUF "),
		PKTF(pkt, LBF_BUF_ALLOC,    "ALC "), PKTF(pkt, LBF_HME_DATA,  "HME "),
		PKTF(pkt, LBF_MGMT_TX_PKT,  "MGT "), PKTF(pkt, LBF_TX_FRAG,   "TXF "),
		PKTF(pkt, LBF_HWA_PKT,      "HWA "), PKTF(pkt, LBF_HWA_3BPKT, "H3B "),
		fsm_str);
	printf("\t\tBUF[%c<%08x..%08x>|%c<%08x..%08x>] hdrm %u DATA<%08x,%04u>\n",
		PKTISTXFRAG(osh, pkt) ? 'B' : 'b',
		(int) LB_HEAD_B(pkt), (int) LB_END_B(pkt),
		PKTISTXFRAG(osh, pkt) ? 'f' : 'F',
		(int) LB_HEAD_F(pkt), (int) LB_END_F(pkt),
		PKTHEADROOM(osh, pkt), (int) PKTDATA(osh, pkt), PKTLEN(osh, pkt));
	printf("\t\tHBM NEXT<D%08x H%08x:%04u> LINK<D%08x H%08x:%04u> %s\n",
		(int) PQP_DBM_NEXT(pkt), (int) hbm_pkt_next,
		PQP_HBM_UPTR_VALID(hbm_pkt_next)
		? PQP_HBM_PIO2ID(pqp_mgr, hbm_pkt_next) : PQP_HBM_ID_NULL,
		(int) PQP_DBM_LINK(pkt), (int) hbm_pkt_link,
		PQP_HBM_UPTR_VALID(hbm_pkt_link)
		? PQP_HBM_PIO2ID(pqp_mgr, hbm_pkt_link) : PQP_HBM_ID_NULL,
		fsm_str);
}   // _pqp_pgo_pkt_dump()
#else  /* ! PQP_PKTIO_BUILD */
#define _pqp_pktio_dump(pkt, op, fsm_str)   PQP_NOOP
#endif /* ! PQP_PKTIO_BUILD */

static void // Debug dump a pqp_req, along with the HBM and DBM packet list
_pqp_req_dump(pqp_req_t * pqp_req)
{
	printf(PQP_REQ_FMT, PQP_REQ_VAL(pqp_req));
}   // _pqp_req_dump()

void // Debug dump a pqp_pktq
pqp_pktq_dump(pqp_pktq_t * pqp_pktq)
{
	pqp_req_t * pqp_req = PQP_PKTQ_REQ(pqp_pktq);
	printf(PQP_PKTQ_FMT " Req::Q %p\n", PQP_PKTQ_VAL(pqp_pktq),
		(pqp_req != PQP_REQ_NULL) ? pqp_req->pqp_pktq : NULL);
}   //pqp_pktq_dump()

void // Debug dump the internal PQP runtime state
pqp_dump(bool verbose)
{
	pqp_mgr_t  * pqp_mgr = PQP_MGR();

	pqp_list_t * pqp_list_req = __pqp_list(pqp_mgr, PQP_LIST_IDX_REQ);
	pqp_list_t * pqp_list_hbm = __pqp_list(pqp_mgr, PQP_LIST_IDX_HBM);
	pqp_list_t * pqp_list_pgo = __pqp_list(pqp_mgr, PQP_LIST_IDX_PGO);
	pqp_list_t * pqp_list_pgi = __pqp_list(pqp_mgr, PQP_LIST_IDX_PGI);

	pqp_mgr->hme_lcl.free_cnt = hme_free(HME_USER_LCLPKT);

	printf("PQP " PQP_VERSION " %p pool %p max %u fsm 0x%08x\n",
		pqp_mgr, pqp_mgr->req_pool, pqp_mgr->req_max, pqp_mgr->fsm_map);
	printf("\tpkts[ dbm_cnt %u hbm_cnt %u hbm_avail_sw %d]\n",
		pqp_mgr->pkts.dbm_cnt, pqp_mgr->pkts.hbm_cnt, pqp_mgr->hbm_avail_sw);
	printf("\tlist[ free %u hbm %u pgo %u pgi %u ]\n",
		pqp_list_req->cnt, pqp_list_hbm->cnt,
		pqp_list_pgo->cnt, pqp_list_pgi->cnt);
	printf("\tHME PKT" HADDR64_FMT " free %u bytes %u pio_uptr %p mwbmap %p\n",
		HADDR64_VAL(pqp_mgr->hme_pkt.dma.haddr64), pqp_mgr->hme_pkt.free_cnt,
		pqp_mgr->hme_pkt.bytes, (void*) (pqp_mgr->hme_pkt.pio_uptr),
		pqp_mgr->hme_pkt.mwbmap);
	printf("\tHME LCL" HADDR64_FMT " free %u\n\n",
		HADDR64_VAL(pqp_mgr->hme_lcl.dma.haddr64), pqp_mgr->hme_lcl.free_cnt);

	pqp_stats();

	if (verbose) {
		uint16 list_idx;
		printf("\t" PQP_REQ_HDR);

		for (list_idx = PQP_LIST_IDX_HBM; list_idx < PQP_LIST_IDX_MAX; ++list_idx) {
			dll_t      * iter_node;
			const char * list_name = pqp_list_name[list_idx];
			pqp_list_t * pqp_list  = __pqp_list(pqp_mgr, list_idx);
			dll_for_each(iter_node, &pqp_list->dll) {
				pqp_req_t  * pqp_req = CONTAINEROF(iter_node, pqp_req_t, node);
				printf("\t%s" PQP_REQ_FMT, list_name, PQP_REQ_VAL(pqp_req));
			}
			__pqp_list_audit(pqp_list);
		}

		if (pqp_mgr->hme_pkt.mwbmap)
			bcm_mwbmap_show(pqp_mgr->hme_pkt.mwbmap);
	}
	printf("\n");

}   // pqp_dump()

void // Debug dump the internal PQP runtime statistics
pqp_stats(void)
{
#if defined(PQP_STATS_BUILD)
	pqp_mgr_t   * pqp_mgr = PQP_MGR();
	pqp_stats_t * stats   = &pqp_mgr->stats;

	printf("\tREQ CALLS  DMAS  CONT OOMEM\n");
	printf("\tPGO %5u %5u %5u %5u\n",
		stats->pgo.req, stats->pgo.dma, stats->pgo.res, stats->pgo.oom);
	printf("\tPGI %5u %5u %5u %5u\n",
		stats->pgi.req, stats->pgi.dma, stats->pgi.res, stats->pgi.oom);
	printf("\tdeplete %u failure %u\n\n", stats->deplete, stats->failure);
#endif  /* PQP_STATS_BUILD */
}   // pqp_stats()

static void
pqp_help(void)
{
	printf("PQP version %s\n"
	       "\tUsage: dhd -i <ifname> cons \"pqp -[d|h|s|v]\"\n"
	       "\t\t-d: Dump\n"
	       "\t\t-s: Stats\n"
	       "\t\t-v: Verbose dump\n", PQP_VERSION);
}   // pqp_help()

void // Parse dhd cons "pqp -[d|h|s|v]" and invoke dump
pqp_cmd(void * arg, int argc, char * argv[])
{
	char *p = argv[1];

	if (argc < 2)  goto cmd_help;
	if (*p != '-') goto cmd_help;

	switch (*++p) {
		case 'd': pqp_dump(FALSE); return;
		case 'v': pqp_dump(TRUE);  return;
		case 's': pqp_stats();     return;
		default : break;
	}

cmd_help: pqp_help();

}   // pqp_cmd()

int // Dettach the PQP subsystem
BCMATTACHFN(pqp_fini)(osl_t * osh)
{
	pqp_mgr_t * pqp_mgr = &pqp_mgr_g;

	if (pqp_mgr->req_pool != PQP_REQ_NULL) {
		size_t pqp_req_pool_bytes;
		ASSERT(pqp_mgr->req_max != 0);
		pqp_req_pool_bytes  = sizeof(pqp_req_t) * pqp_mgr->req_max;
		memset(pqp_mgr->req_pool, 0, pqp_req_pool_bytes);
		MFREE(osh, pqp_mgr->req_pool, pqp_req_pool_bytes);
		pqp_mgr->req_pool = PQP_REQ_NULL;
	}
	if (pqp_mgr->hme_pkt.mwbmap != PQP_MAP_NULL) {
		bcm_mwbmap_fini(osh, pqp_mgr->hme_pkt.mwbmap);
		pqp_mgr->hme_pkt.mwbmap = PQP_MAP_NULL;
	}
	memset(pqp_mgr, 0, sizeof(pqp_mgr_t));

	return BCME_OK;

}   // pqp_fini()

int // Initialize the PQP subsystem
BCMATTACHFN(pqp_init)(osl_t * osh, int pqp_req_max)
{
	size_t      pqp_req_pool_bytes;
	pqp_req_t * pqp_req;
	pqp_mgr_t * pqp_mgr = &pqp_mgr_g;

	if (pqp_mgr->req_pool != PQP_REQ_NULL) return BCME_OK; // singleton init
	ASSERT(pqp_req_max != 0);

#if defined(HWA_PKTPGR_BUILD)
	ASSERT(PQP_PKT_SZ == sizeof(hwa_pp_lbuf_t));
#endif

	memset(pqp_mgr, 0, sizeof(pqp_mgr_t));

	pqp_mgr->rte_osh          = osh;

	// Allocate memory for a pool of pqp requests
	pqp_req_pool_bytes        = sizeof(pqp_req_t) * pqp_req_max;
	pqp_mgr->req_pool         = (pqp_req_t*) MALLOCZ(osh, pqp_req_pool_bytes);
	if (pqp_mgr->req_pool == PQP_REQ_NULL) {
		PQP_ERROR("PQP: FAILURE alloc %u req size %u bytes %u",
			pqp_req_max, (uint32)sizeof(pqp_req_t), (uint32)pqp_req_pool_bytes);
		goto init_fail;
	}
	pqp_mgr->req_max          = pqp_req_max;

	// Initialize multi word bitmap index allocator for HBM Packets
	// NOTE: hme_pkt is actually available only after pqp_link_hme()
	pqp_mgr->hme_pkt.mwbmap   = bcm_mwbmap_init(osh, PQP_PKT_MAX);
	if (pqp_mgr->hme_pkt.mwbmap == PQP_MAP_NULL) {
		PQP_ERROR("PQP: FAILURE HME PKT mwbmap init %u", PQP_PKT_MAX);
		goto init_fail;
	}
	// Reserve ID 0 as it is used to signify NULL
	bcm_mwbmap_force(pqp_mgr->hme_pkt.mwbmap, PQP_HBM_ID_NULL);
	pqp_mgr->hme_pkt.free_cnt = (PQP_PKT_MAX - 1);

	pqp_mgr->hbm_avail_sw     = pqp_mgr->hme_pkt.free_cnt;

	// Since hbm credit can also help to control the HWAPP HDBM usage.
	// If PQP HME PSQ size is equal to HWAPP HDBM size,
	// Reserve NRXD HDBM packets which are used for HWA RX and
	// HWA_PKTPGR_PAGEIN_REQ_HDBMTH HDBM packets to avoid HWA stuck in getting HDBM resource.
	if (pqp_mgr->hbm_avail_sw == (HWA_HOST_PKTS_MAX - 1)) {
		pqp_mgr->hbm_avail_sw -= (WL_NRXD + HWA_PKTPGR_PAGEIN_REQ_HDBMTH);
	}

	// Fetch BME user keys for H2D and D2H mem2mem engines
	pqp_mgr->bme_h2d          = bme_get_key(osh, BME_USR_H2D);
	if (pqp_mgr->bme_h2d == BME_INVALID) {
		PQP_ERROR("PQP: FAILURE bme_get_key BME_USR_H2D");
		goto init_fail;
	}
	pqp_mgr->bme_d2h          = bme_get_key(osh, BME_USR_D2H);
	if (pqp_mgr->bme_d2h == BME_INVALID) {
		PQP_ERROR("PQP: FAILURE bme_get_key BME_USR_D2H");
		goto init_fail;
	}

	// Cannot fail now

	{   // Reset all PQP Manager lists
		uint16 list_idx;
		for (list_idx = 0; list_idx < PQP_LIST_IDX_MAX; ++list_idx) {
			// don't use  __pqp_list here as list is not initialized
			pqp_list_t * pqp_list = &pqp_mgr->list[list_idx];
			__pqp_list_init(pqp_list, list_idx);
		}
	}

	{   // Reset each PQP request and add to PQP manager request free list
		uint32 pqp_req_idx;
		pqp_list_t * pqp_list_req;

		pqp_list_req = __pqp_list(pqp_mgr, PQP_LIST_IDX_REQ);
		for (pqp_req_idx = 0; pqp_req_idx < pqp_req_max; ++pqp_req_idx)
		{
			pqp_req = pqp_mgr->req_pool + pqp_req_idx;
			dll_init(&pqp_req->node);
			__pqp_req_reset(pqp_req);
			__pqp_list_append(pqp_list_req, pqp_req);
		}
	}

	PQP_PRINT("Packet Queue Pager Initialized with %u queue", pqp_mgr->req_max);

	return BCME_OK;

init_fail:
	(void)pqp_fini(osh);

	return BCME_NOMEM;

}   // pqp_init()

int // Configure HME pages for Packet Storage
pqp_bind_hme(void)
{
	int         bcme_code;
	pqp_mgr_t * pqp_mgr       = PQP_MGR();
	uint32      hme_pkt_bytes = PQP_PKT_SZ * PQP_PKT_MAX;
	uint32      hme_pkt_pages = PCIE_IPC_HME_PAGES(hme_pkt_bytes);

	uint32 align_bits = PCIE_IPC_HME_ALIGN_BITS;
	uint32 bound_bits = PCIE_IPC_HME_BOUND_BITS;
	uint32 hme_flags = PCIE_IPC_HME_FLAG_PRIVATE
	                 | PCIE_IPC_HME_FLAG_ALIGNED  | PCIE_IPC_HME_FLAG_BOUNDARY
	                 | PCIE_IPC_HME_FLAG_SBTOPCIE | PCIE_IPC_HME_FLAG_DMA_XFER;

	pqp_mgr->hme_pkt.bytes = hme_pkt_bytes;

	PQP_DEBUG(PQP_PRINT("PQP: pqp_bind_hme PSQPKT bytes %u pages %u",
		hme_pkt_bytes, hme_pkt_pages));

	bcme_code = hme_attach_mgr(HME_USER_PSQPKT, hme_pkt_pages,
			PQP_PKT_SZ, PQP_PKT_MAX, HME_MGR_NONE,
			align_bits, bound_bits, hme_flags);

	PQP_ASSERT(bcme_code == BCME_OK);

	return bcme_code;

}   // pqp_bind_hme()

int // Link HME memory segments for Packet Storage and Lcl Buffer Storage
pqp_link_hme(void)
{
	pqp_mgr_t * pqp_mgr = PQP_MGR();

	/* Setup DMA base address of HME PSQ Packet Memory and LCL data buffer */
	pqp_mgr->hme_pkt.dma.haddr64 = hme_haddr64(HME_USER_PSQPKT);
	pqp_mgr->hme_lcl.dma.haddr64 = hme_haddr64(HME_USER_LCLPKT);

	ASSERT(!HADDR64_IS_ZERO(pqp_mgr->hme_pkt.dma.haddr64));
	ASSERT(!HADDR64_IS_ZERO(pqp_mgr->hme_lcl.dma.haddr64));

	PQP_DEBUG(PQP_PRINT("PQP: pqp_link_hme PKT" HADDR64_FMT " LCL" HADDR64_FMT,
		HADDR64_VAL(pqp_mgr->hme_pkt.dma.haddr64),
		HADDR64_VAL(pqp_mgr->hme_lcl.dma.haddr64)));

	/* Setup a SBTOPCIE window for PIO access into HME PSQ Packet Memory */
	pqp_mgr->hme_pkt.pio_uptr =
		sbtopcie_setup(SBTOPCIE_USR_PQP, pqp_mgr->hme_pkt.dma.u64,
			pqp_mgr->hme_pkt.bytes, SBTOPCIE_MODE_PRIVATE);

	PQP_DEBUG(PQP_PRINT("PQP: sbtopcie_setup pio_uptr 0x%08x",
		(uint32)pqp_mgr->hme_pkt.pio_uptr));

	/* Do not setup a SBTPCIE access for pqp_mgr->hme_lcl.pio_uptr */

	return BCME_OK;

}   // pqp_link_hme()

/**
 * +--------------------------------------------------------------------------+
 *  Query routines
 * +--------------------------------------------------------------------------+
 */
int // DBM resident packets owned by pqp_req, excluding those in pqp_pktq
pqp_dbm_cnt(pqp_pktq_t * pqp_pktq)
{
	pqp_req_t * pqp_req = PQP_PKTQ_REQ(pqp_pktq);
	return (pqp_req != PQP_REQ_NULL) ? (int)pqp_req->pkts.dbm_cnt : 0;
}   // pqp_dbm_cnt()

int // HBM resident packets owned by pqp_req, excluding those in pqp_pktq
pqp_hbm_cnt(pqp_pktq_t * pqp_pktq)
{
	pqp_req_t * pqp_req = PQP_PKTQ_REQ(pqp_pktq);
	return (pqp_req != PQP_REQ_NULL) ? (int)pqp_req->pkts.hbm_cnt : 0;
}   // pqp_hbm_cnt()

int // DBM + HBM resident packets owned by pqp_req including those in pqp_pktq
pqp_pkt_cnt(pqp_pktq_t * pqp_pktq)
{
	int tot_cnt = 0;
	pqp_req_t * pqp_req = PQP_PKTQ_REQ(pqp_pktq);
	tot_cnt = (pqp_req != PQP_REQ_NULL) ?
		(int)(pqp_req->pkts.dbm_cnt + pqp_req->pkts.hbm_cnt): 0;

	tot_cnt += PQP_PKTQ_PKTS(pqp_pktq);

	return tot_cnt;
}   // pqp_pkt_cnt()

bool
pqp_owns(pqp_pktq_t *pqp_pktq)
{
	return ((__pqp_pktq_req_get(pqp_pktq) == PQP_REQ_NULL) ? FALSE : TRUE);
}

int // WL AP Power Save module configures callbacks for Page Out and Page In
pqp_config(osl_t * wl_osh, void * dev, pqp_cb_fn_t pgo_pcb, pqp_cb_fn_t pgi_qcb)
{
	pqp_mgr_t * pqp_mgr = PQP_MGR();

	PQP_DEBUG(PQP_PRINT("PQP: pqp_config pgo_pcb[@%08x] pgi_qcb[@%08x] ",
		(uintptr)pgo_pcb, (uintptr)pgi_qcb));

	pqp_mgr->wl_osh     = wl_osh;
	pqp_mgr->pgo_pcb    = pgo_pcb;
	pqp_mgr->pgi_qcb    = pgi_qcb;

	PQP_DEV(pqp_mgr)    = dev;

	return BCME_OK;

}   // pqp_config()

// +--------------------------------------------------------------------------+
//  HBM Credit Section
// +--------------------------------------------------------------------------+

int
pqp_hbm_avail(void)
{
	pqp_mgr_t * pqp_mgr = PQP_MGR();
	ASSERT(pqp_mgr->hbm_avail_sw >= 0);
	return pqp_mgr->hbm_avail_sw;
}   // pqp_hbm_avail()

void
pqp_hbm_avail_add(int delta)
{
	pqp_mgr_t * pqp_mgr = PQP_MGR();
	pqp_mgr->hbm_avail_sw += delta;
}   // pqp_hbm_avail_add()

void
pqp_hbm_avail_sub(int delta)
{
	pqp_mgr_t * pqp_mgr = PQP_MGR();
	pqp_mgr->hbm_avail_sw -= delta;
	ASSERT(pqp_mgr->hbm_avail_sw >= 0);
}   // pqp_hbm_avail_sub()

// +--------------------------------------------------------------------------+
//  PGO Section
// +--------------------------------------------------------------------------+

static void // BME mem2mem transfer a data buffer from DBM to HBM, if required
__pqp_pgo_dma_buf(pqp_mgr_t * pqp_mgr, pqp_pkt_t * dbm_pkt) // dbm resident sdu
{
	osl_t   * osh       = pqp_mgr->wl_osh;
	uint16    pktlen    = (uint32) PKTLEN(osh, dbm_pkt);
	uint16    headroom  = PKTHEADROOM(osh, dbm_pkt);
	pqp_dma_t dbm_buf_src, hme_buf_dst;

#if defined(PQP_USE_MGMT_LOCAL)
	// No dma buf because we still use mgmt local buffer.
	if (PKTISMGMTTXPKT(osh, dbm_pkt)) {
		// Store original mgmt local buffer
		PKTSETLOCAL(osh, dbm_pkt, dbm_pkt);

		return;
	}
#endif

	// Packets from TxStatus suppressed may have data in HME
	// Check if packet data is in a buffer discrete from the packet context
	if ((!PKTISBUFALLOC(osh, dbm_pkt) && ((headroom + pktlen) <= PQP_PKT_BUF_SZ)) ||
		PKTHASHMEDATA(osh, dbm_pkt))
		return;

	// Keep the local buffer if there is no LCL resource
	if (pqp_mgr->hme_lcl.free_cnt == 0)
		return;

	dbm_buf_src.u64 = 0;
	dbm_buf_src.ptr = (void*) PKTDATA(osh, dbm_pkt);

	// Allocate a HME LCLPKT data buffer and D2H dma transfer data buffer
	hme_buf_dst.haddr64 = hme_get(HME_USER_LCLPKT, pktlen); // pktlen ignored

	// Initiate a D2H dma transfer of data buffer to allocated LCLPKT buffer
	bme_sync_usr(pqp_mgr->rte_osh, BME_USR_D2H); // sync before use
	bme_copy64(pqp_mgr->rte_osh, pqp_mgr->bme_d2h,
	           dbm_buf_src.u64, hme_buf_dst.u64, pktlen);
	PQP_STATS(pqp_mgr->stats.pgo.dma++);

	// Save headroom and lcl_haddr64, and tag packet as carrying HME DMATE
	PKTSAVEHEADROOM(osh, dbm_pkt,  headroom);
	PKTSETHMELEN(osh,    dbm_pkt,  pktlen); // redundant
	PKTSETHME_HI(osh,    dbm_pkt,  HADDR64_HI(hme_buf_dst.haddr64));
	PKTSETHME_LO(osh,    dbm_pkt,  HADDR64_LO(hme_buf_dst.haddr64));

	PKTSETHMEDATA(osh,   dbm_pkt);

	pqp_mgr->hme_lcl.free_cnt--; // consumed one HME LCLPKT entry

	PQP_DEBUG(
		PQP_PRINT("PQP: _pqp_pgo_dma_buf %p %u lcl" HADDR64_FMT "\n",
			dbm_buf_src.ptr, pktlen, HADDR64_VAL(hme_buf_dst.haddr64)));

}   // __pqp_pgo_dma_buf()

// +--------------------------------------------------------------------------+

static void // BME mem2mem transfer a pqp_pkt_t from DBM to HBM
_pqp_pgo_dma_pkt(pqp_mgr_t * pqp_mgr, pqp_pkt_t * dbm_pkt) // dbm resident sdu
{
	uint32    dma_len;
	pqp_dma_t dbm_pkt_src, hbm_pkt_dst;
	osl_t   * osh = pqp_mgr->wl_osh;

	PQP_DEBUG(_pqp_pktio_dump(dbm_pkt, "PGO", "DMA"));
	PQP_DEBUG(PQP_PRINT("PQP: _pqp_pgo_dma_pkt dbm %p", dbm_pkt));

	// DMA transfer data buffer to HME LCL, if required
	__pqp_pgo_dma_buf(pqp_mgr, dbm_pkt);

	if (PQP_HBM_LINK(dbm_pkt) == PQP_HBM_PTR_INV) {
		PQP_ASSERT(PQP_DBM_LINK(dbm_pkt) == PQP_DBM_PKT_NULL);
		PQP_HBM_LINK(dbm_pkt) = PQP_HBM_PTR_NULL;
	}

	// If packet is tagged as HAS HME DATA and DATA is LOCAL, then need only transfer context
	dma_len = ((PKTHASHMEDATA(osh, dbm_pkt) && PKTDATAISLOCAL(osh, dbm_pkt)) |
		(PKTLEN(osh, dbm_pkt) == 0)) ? PQP_PKT_CTX_SZ : PQP_PKT_SZ;

	dbm_pkt_src.u64  = 0;
	dbm_pkt_src.ptr  = (void*) dbm_pkt;
	hbm_pkt_dst.u64  = pqp_mgr->hme_pkt.dma.u64;
	hbm_pkt_dst.uptr = PQP_HBM_ID2DMA(pqp_mgr, PQP_HBM_ID(dbm_pkt));

	bme_sync_usr(pqp_mgr->rte_osh, BME_USR_D2H); // sync before use
	bme_copy64(pqp_mgr->rte_osh, pqp_mgr->bme_d2h,
	           dbm_pkt_src.u64, hbm_pkt_dst.u64, dma_len);
	PQP_STATS(pqp_mgr->stats.pgo.dma++);

}   // _pqp_pgo_dma_pkt()

// +--------------------------------------------------------------------------+

static INLINE void // Page out an entire DBM PDU - PKTNEXT linked list of SDUs
__pqp_pgo_pdu(pqp_mgr_t * pqp_mgr, pqp_req_t * pqp_req, pqp_pkt_t * dbm_pdu)
{
	pqp_pkt_t * sdu_curr;
	pqp_pkt_t * sdu_next;
	pqp_pkt_t * dbm_link;
	uint16      hbm_id    = PQP_HBM_ID_INV;

	PQP_DEBUG(PQP_PRINT("PQP: __pqp_pgo_pdu dbm %p", dbm_pdu));

	// Prepare the pdu::link by allocating a HBM packet
	dbm_link   = PQP_DBM_LINK(dbm_pdu);
	if (dbm_link != PQP_DBM_PKT_NULL) {   // linked packet is in dbm
		hbm_id = PQP_HBM_ID(dbm_link);
		if (hbm_id == PQP_HBM_ID_INV) {   // linked packet does not a HBM id
			hbm_id                = __pqp_mgr_hbm_id_get(pqp_mgr);
			PQP_HBM_ID(dbm_link)  = hbm_id;
			PQP_HBM_LINK(dbm_pdu) = PQP_HBM_ID2PIO(pqp_mgr, hbm_id);
		}
	}

	if (PQP_HBM_ID(dbm_pdu) == PQP_HBM_ID_INV) {
		hbm_id              = __pqp_mgr_hbm_id_get(pqp_mgr);
		PQP_HBM_ID(dbm_pdu) = hbm_id;
	}

	sdu_curr = dbm_pdu; // PKTNEXT list iterator
	sdu_next = PQP_DBM_NEXT(sdu_curr);

	while (sdu_next != PQP_DBM_PKT_NULL) // iterate to end of PKTNEXT list
	{
		PQP_ASSERT(PQP_HBM_ID(sdu_curr) != PQP_HBM_ID_INV);

		// Setup HBM for next sdu
		if (PQP_HBM_ID(sdu_next) == PQP_HBM_ID_INV) {
			hbm_id                 = __pqp_mgr_hbm_id_get(pqp_mgr);
			PQP_HBM_ID(sdu_next)   = hbm_id;
			PQP_HBM_NEXT(sdu_curr) = PQP_HBM_ID2PIO(pqp_mgr, hbm_id);
		} else {
			PQP_HBM_NEXT(sdu_curr) = PQP_HBM_PKT_NULL;
		}

		_pqp_pgo_dma_pkt(pqp_mgr, sdu_curr); // BME M2M TRANSFER SDU

		sdu_curr = sdu_next;
		sdu_next = PQP_DBM_NEXT(sdu_curr);
	}

	PQP_HBM_NEXT(sdu_curr) = PQP_HBM_PKT_NULL;

	_pqp_pgo_dma_pkt(pqp_mgr, sdu_curr);    // BME M2M TRANSFER TAIL SDU

	pqp_mgr->pkts.hbm_cnt++;
	pqp_req->pkts.hbm_cnt++;

}   // __pqp_pgo_pdu()

// +--------------------------------------------------------------------------+

static INLINE int // Page out packets in pktq upto the first HBM segment
__pqp_pgo_req_pktq(pqp_mgr_t *pqp_mgr, pqp_req_t *pqp_req, pqp_pktq_t *pqp_pktq)
{
	pqp_cb_t     pqp_cb;
	pqp_pkt_t  * dbm_pdu;

	PQP_DEBUG(PQP_PRINT("PQP: __pqp_pgo_req_pktq req %p pktq %p",
		pqp_req, pqp_pktq));

	// Prepare the callback for all pdus paged out
	pqp_cb.osh      = pqp_mgr->wl_osh;
	pqp_cb.ctx      = pqp_req->ctx;
	pqp_cb.pqp_pktq = pqp_req->pqp_pktq;

	// Determine free HME LCLPKT data buffers
	pqp_mgr->hme_lcl.free_cnt = hme_free(HME_USER_LCLPKT);

	// -------------------------------------
	// PGO PKTQ list upto First HBM Segment
	// -------------------------------------
	dbm_pdu = PQP_PKTQ_HEAD(pqp_pktq);

	while (dbm_pdu != PQP_DBM_PKT_NULL)
	{
		// Page out all sdu in 1 pdu i.e. PKTNEXT linked list of sdu(s)

		__pqp_pgo_pdu(pqp_mgr, pqp_req, dbm_pdu);

		// Update HBM first pkt list segment head and tail index
		if (dbm_pdu == PQP_PKTQ_HEAD(pqp_pktq)) {
			pqp_req->hbm_id.head = PQP_HBM_ID(dbm_pdu);
		}
		if (PQP_HBM_SEGID(dbm_pdu) == PQP_HBM_ID_INV) {
			pqp_req->hbm_id.tail = PQP_HBM_ID(dbm_pdu);
		} else { // use saved HBM tail in prepend
			PQP_ASSERT(PQP_DBM_LINK(dbm_pdu) == PQP_DBM_PKT_NULL);
			pqp_req->hbm_id.tail = PQP_HBM_SEGID(dbm_pdu);
		}

		// Delink current DBM pdu from pktq managed list, as pdu is now in HBM
		pqp_cb.pqp_pkt       = dbm_pdu; // store into the callback bucket
		dbm_pdu              = (pqp_pkt_t*) PQP_DBM_LINK(dbm_pdu); // iterate
		PQP_DBM_LINK(pqp_cb.pqp_pkt) = PQP_DBM_PTR_NULL; // delink

		bme_sync_usr(pqp_mgr->rte_osh, BME_USR_D2H); // sync before freeing

		PQP_PKTQ_PKTS(pqp_pktq)--;

		// Return DBM pdu to WL APPS, so it may free the DBM pdu to DBM pkt pool
		pqp_mgr->pgo_pcb(&pqp_cb); // DBM pdu may be returned to DBM pkt pool

	} // while dbm_pdu PKTLINK iteration

	_pqp_pktq_empty(pqp_pktq); // empty out the pktq

	return BCME_OK;

}   // __pqp_pgo_req_pktq()

// +--------------------------------------------------------------------------+

static INLINE int
__pqp_pgo_hbm_dbm(pqp_mgr_t *pqp_mgr, pqp_req_t *pqp_req)
{
	int          pgo_cnt;
	uint16       hbm_id;
	pqp_cb_t     pqp_cb;
	pqp_pkt_t  * dbm_pdu;
	pqp_pkt_t  * hbm_pdu;

	PQP_DEBUG(PQP_PRINT("PQP: __pqp_pgo_hbm_dbm req %p pktq %p hbm %u dbm %u",
		pqp_req, pqp_req->pqp_pktq,
		pqp_req->pkts.hbm_cnt, pqp_req->pkts.dbm_cnt));

	// Prepare the callback for all pdus paged out
	pqp_cb.osh      = pqp_mgr->wl_osh;
	pqp_cb.ctx      = pqp_req->ctx;
	pqp_cb.pqp_pktq = pqp_req->pqp_pktq;

	pqp_mgr->hme_lcl.free_cnt = hme_free(HME_USER_LCLPKT);

	// -------------------------------------
	// SKIP first HBM segment
	// -------------------------------------
	hbm_id  = pqp_req->hbm_id.tail;
	hbm_pdu = (pqp_pkt_t*) PQP_HBM_ID2PIO(pqp_mgr, hbm_id);

	pgo_cnt = 0;
	// NOTE: sbtopcie access using SBTOPCIE_MODE_PRIVATE
	dbm_pdu = (pqp_pkt_t*) PQP_DBM_LINK(hbm_pdu);

	while (dbm_pdu != PQP_DBM_PKT_NULL)
	{
		// Page out a DBM pdu i.e. PKTNEXT linked list of sdu(s)
		__pqp_pgo_pdu(pqp_mgr, pqp_req, dbm_pdu);

		if (pgo_cnt++ == 0) {
			uintptr hbm_pdu_uptr = PQP_HBM_ID2PIO(pqp_mgr, PQP_HBM_ID(dbm_pdu));
			// NOTE: sbtopcie access using SBTOPCIE_MODE_PRIVATE
			PQP_HBM_LINK(hbm_pdu) = hbm_pdu_uptr;

			PQP_DEBUG(_pqp_pktio_dump(hbm_pdu, "PGO", "LINK"));
		}

		pqp_req->hbm_id.tail = PQP_HBM_ID(dbm_pdu);

		// Delink current DBM pdu from pktq managed list, as pdu is now in HBM
		pqp_cb.pqp_pkt       = dbm_pdu; // store into the callback bucket
		dbm_pdu              = (pqp_pkt_t*) PQP_DBM_LINK(dbm_pdu); // iterate
		PQP_DBM_LINK(pqp_cb.pqp_pkt) = PQP_DBM_PTR_NULL; // delink

		// Ensure D2H DMA have completed for all SDUs in PDU
		bme_sync_usr(pqp_mgr->rte_osh, BME_USR_D2H);

		// Return DBM pdu to WL APPS, so it may free DBM pdu to DBM pkt pool
		pqp_mgr->pgo_pcb(&pqp_cb); // DBM pdu may be returned to DBM pkt pool

		pqp_req->pkts.dbm_cnt--;
		pqp_mgr->pkts.dbm_cnt--;
	}

	return BCME_OK;

}   // __pqp_pgo_hbm_dbm()

// +--------------------------------------------------------------------------+

static int // Wake pending PGO requests that may be pending in PGO list
_pqp_pgo_wake(pqp_mgr_t * pqp_mgr)
{
	int bcme_code;
	pqp_list_t * pqp_list_pgo = __pqp_list(pqp_mgr, PQP_LIST_IDX_PGO);

	PQP_DEBUG(PQP_PRINT("PQP: _pqp_pgo_wake"));

	// Iterate through PGO list of requests pending page out
	while (__pqp_list_cnt(pqp_list_pgo) > 0)
	{
		pqp_req_t  * pqp_req  = __pqp_list_head(pqp_list_pgo);
		pqp_pktq_t * pqp_pktq = pqp_req->pqp_pktq;

		// Start paging out the first DBM segment: in pktq or after HBM segment
		if (pqp_pktq->n_pkts) { // start paging out DBM packets in pktq
			bcme_code = __pqp_pgo_req_pktq(pqp_mgr, pqp_req, pqp_pktq);
		} else { // skip first HBM segment, and start paging DBM segment
			bcme_code = __pqp_pgo_hbm_dbm(pqp_mgr, pqp_req);
		}

		// If PQP request has no DBM packets, move request to HBM list
		if ((pqp_req->pkts.dbm_cnt == 0) && (pqp_pktq->n_pkts == 0))
		{
			__pqp_mgr_transfer(pqp_mgr, pqp_req,
			                   PQP_LIST_IDX_PGO, PQP_LIST_IDX_HBM);
		}

		if (bcme_code) break; // BCME_NORESOURCE

	}   // while

	// DBM resources may be available as part of PGO
	// Try to fill pqp pkt pool for PGI.
	if (__pqp_fsm_evt_get(pqp_mgr, PQP_FSM_DBM_OOM))
	{
		hwa_pktpgr_pqplbufpool_fill(PQP_HWA_DEV(pqp_mgr));
	}

	return bcme_code;

}   // _pqp_pgo_wake()

// +--------------------------------------------------------------------------+

static void // Append a pktq packet list to tail of PQP request
_pqp_pgo_append(pqp_mgr_t * pqp_mgr, pqp_req_t *pqp_req, pqp_pktq_t *pqp_pktq)
{
	uintptr hbm_last       = PQP_HBM_ID2PIO(pqp_mgr, pqp_req->hbm_id.tail);

	// At least one packet in HBM, so PQP request takes ownership of pktq list
	PQP_ASSERT(pqp_req->pkts.hbm_cnt != 0);
	PQP_ASSERT(pqp_req->hbm_id.page != PQP_HBM_PAGE_INV);

	PQP_DEBUG(PQP_PRINT("PQP: _pqp_pgo_append last hbm_id %u dbm %p",
		pqp_req->hbm_id_last, pqp_req->dbm_last));

	PQP_DBM_LINK(hbm_last) = PQP_PKTQ_HEAD(pqp_pktq);

	// PQP owns the pktq packet list
	pqp_req->pkts.dbm_cnt += PQP_PKTQ_PKTS(pqp_pktq); // request DBM packet cnt
	pqp_mgr->pkts.dbm_cnt += PQP_PKTQ_PKTS(pqp_pktq); // manager DBM packet cnt

	_pqp_pktq_empty(pqp_pktq); // empty out the pktq

}   // _pqp_pgo_append()

// +--------------------------------------------------------------------------+

static void // Prepend a pktq packet list by appending first HBM segment
_pqp_pgo_prepend(pqp_mgr_t * pqp_mgr, pqp_req_t *pqp_req, pqp_pktq_t *pqp_pktq)
{
	pqp_pkt_t  * dbm_pdu   = PQP_PKTQ_TAIL(pqp_pktq);
	uintptr      hbm_pdu   = PQP_HBM_ID2PIO(pqp_mgr, pqp_req->hbm_id.head);

	PQP_DEBUG(PQP_PRINT("PQP: _pqp_pgo_prepend hbm_id head %u tail %u",
		pqp_req->hbm_id.head, pqp_req->hbm_id.tail));

	// Join DBM list to HBM first segment, using tail DBM packet
	PQP_HBM_LINK(dbm_pdu)  = hbm_pdu; // Link DBM tail to HBM head
	PQP_HBM_SEGID(dbm_pdu) = pqp_req->hbm_id.tail; // HBM first segment end
	// End of first HBM segment, allows to jump to tail without PIO list walk

}   // _pqp_pgo_prepend()

// +--------------------------------------------------------------------------+

int // Page out packets from pqp_pktq using append/prepend mode
pqp_pgo(pqp_pktq_t * pqp_pktq, pqp_policy_t policy, void * cb_ctx)
{
	int ret;
	pqp_mgr_t  * pqp_mgr      = PQP_MGR();
	pqp_req_t  * pqp_req      = __pqp_pktq_req_get(pqp_pktq);

	PQP_DEBUG(PQP_PRINT("PQP: pqp_pgo" PQP_PKTQ_FMT " %s ctx %p",
		PQP_PKTQ_VAL(pqp_pktq), pqp_policy_name[policy], cb_ctx));

	PQP_STATS(pqp_mgr->stats.pgo.req++);

	if (PQP_PKTQ_PKTS(pqp_pktq) == 0)
	{
		return BCME_OK;
	}

	// Step 0. Reset all pktq managed packet's HBM context
	__pqp_pktq_pdus_reset(pqp_pktq);

	// Step 1. Check if pktq is managed by PQP
	if (pqp_req == PQP_REQ_NULL) // PQP does not manage the pktq
	{	// Allocate a PQP request, transfer to PQP_LIST_IDX_PGO, pair with pktq
		pqp_req = __pqp_mgr_req_get(pqp_mgr);
		PQP_DEBUG(PQP_PRINT("PQP: pqp_pgo alloc pqp_req %p", pqp_req));
		if (pqp_req == PQP_REQ_NULL) {
			PQP_PRINT("PQP: pqp_pgo no resource warning");
			return BCME_NORESOURCE;
		}
		// Convert pktq into a PQP managed pktq
		PQP_PKTQ_REQ(pqp_pktq) = pqp_req; // pair PQP pktq and new PQP request
		pqp_req->pqp_pktq      = pqp_pktq;
		// Initialize new PQP request
		pqp_req->ctx           = cb_ctx; // save pqp_pktq callback ctx
	}
	else if (pqp_req->list_idx != PQP_LIST_IDX_PGO) // PQP request in HBM or PGI
	{
		// Transfer PQP request from src list to tail of PGO list
		__pqp_mgr_transfer(pqp_mgr, pqp_req,
		                   pqp_req->list_idx, PQP_LIST_IDX_PGO);
	}

	// Reset any previous queue pgi fill and cont attributes
	PQP_PKTQ_CONT(pqp_pktq) = 0;
	PQP_PKTQ_FILL(pqp_pktq) = 0;

	PQP_DEBUG(PQP_PRINT("PQP: pqp_pgo <%p,%p> fsm %s",
		pqp_pktq, pqp_req, pqp_list_name[pqp_req->list_idx]));

	if (pqp_req->pkts.hbm_cnt != 0) // append packet list to pqp_req tail
	{
		if (policy == PQP_APPEND) { // apply PGO policy APPEND
			_pqp_pgo_append(pqp_mgr, pqp_req, pqp_pktq);
		} else {
			_pqp_pgo_prepend(pqp_mgr, pqp_req, pqp_pktq);
		}
	}

	ret = _pqp_pgo_wake(pqp_mgr); // current PQP request may not be at head

	// Should always success if using HBM credit scheme
	if (ret != BCME_OK) {
		_pqp_req_dump(pqp_req);
		ASSERT(0);
	}

	return ret;

}   // pqp_pgo()

int // Resume a PQP Page Out when PQP managed HBM resources become available
pqp_pgo_wake(void)
{
	pqp_mgr_t * pqp_mgr = PQP_MGR();
	PQP_DEBUG(PQP_PRINT("PQP: pqp_pgo_wake"));
	return _pqp_pgo_wake(pqp_mgr);
}   // pqp_pgo_wake()

// +--------------------------------------------------------------------------+
//  Join Section
//  Designer Note: Do not try to optimize between cases
// +--------------------------------------------------------------------------+

static void // q_X[head, tail]req_X[ HBM ]  q_Y[head, tail]req_Y[ HBM ]
_pqp_join_req(pqp_mgr_t * pqp_mgr, pqp_req_t * pqp_req_X,
	pqp_req_t * pqp_req_Y, pqp_policy_t pqp_policy)
{
	uintptr hbm_tail;
	uintptr hbm_head;

	if (pqp_policy == PQP_APPEND) {
		// PQP::X[HBM tail] >> PQP::Y[HBM head]
		hbm_tail = PQP_HBM_ID2PIO(pqp_mgr, pqp_req_X->hbm_id.tail);
		hbm_head = PQP_HBM_ID2PIO(pqp_mgr, pqp_req_Y->hbm_id.head);
		PQP_HBM_LINK(hbm_tail) = hbm_head;
		pqp_req_X->hbm_id.tail = pqp_req_Y->hbm_id.tail;
	} else {
		// PQP::Y[HBM tail] >> PQP::X[HBM head]
		hbm_tail = PQP_HBM_ID2PIO(pqp_mgr, pqp_req_Y->hbm_id.tail);
		hbm_head = PQP_HBM_ID2PIO(pqp_mgr, pqp_req_X->hbm_id.head);
		PQP_HBM_LINK(hbm_tail) = hbm_head;
		pqp_req_X->hbm_id.head = pqp_req_Y->hbm_id.head;
	}

	// PQP::X owns PQP::Y's HBM packets
	pqp_req_X->pkts.hbm_cnt += pqp_req_Y->pkts.hbm_cnt;
}   // _pqp_join_req()

int // Join pktq_A and pktq_B by prepending or appending pktq_B to pktq_A
pqp_join(pqp_pktq_t * pqp_pktq_A, pqp_pktq_t * pqp_pktq_B,
         pqp_policy_t pqp_policy, void *cb_ctx)
{
	pqp_mgr_t  * pqp_mgr      = PQP_MGR();
	pqp_req_t  * pqp_req_A    = __pqp_pktq_req_get(pqp_pktq_A);
	pqp_req_t  * pqp_req_B    = __pqp_pktq_req_get(pqp_pktq_B);

	PQP_DEBUG(PQP_PRINT("PQP: pqp_join pktq %p %p policy %s",
		pqp_pktq_A, pqp_pktq_B, pqp_policy_name[pqp_policy]));

	// Case 0: Both Queues are NOT PQP managed. Need to allocate PQP request.
	if ((pqp_req_A == PQP_REQ_NULL) && (pqp_req_B == PQP_REQ_NULL))
	{
		// Case 0.1: Queue B is empty. Queue A may or may not be empty.
		if (PQP_PKTQ_PKTS(pqp_pktq_B) == 0) {
			goto pgo_pktq_A;                   // PGO Queue A
		}
		// Case 0.2: Queue A is empty and Queue B is not empty.
		if (PQP_PKTQ_PKTS(pqp_pktq_A) == 0) {  // A is empty. Move all B to A
			PQP_PKTQ_HEAD(pqp_pktq_A)               = PQP_PKTQ_HEAD(pqp_pktq_B);
			PQP_PKTQ_TAIL(pqp_pktq_A)               = PQP_PKTQ_TAIL(pqp_pktq_B);
		}
		// Case 0.3: Both Queues A and B are not empty. APPEND B to A
		else if (pqp_policy == PQP_APPEND)   // A[head, tail]  B[head, tail]
		{	                                  // A[head       >>        tail]
			PQP_DBM_LINK(PQP_PKTQ_TAIL(pqp_pktq_A)) = PQP_PKTQ_HEAD(pqp_pktq_B);
			PQP_PKTQ_TAIL(pqp_pktq_A)               = PQP_PKTQ_TAIL(pqp_pktq_B);
		}
		// Case 0.4: Both Queues A and B are not empty. PREPEND B to A
		else                                 // B[head, tail]  A[head, tail]
		{	                                  // A[head       >>        tail]
			PQP_DBM_LINK(PQP_PKTQ_TAIL(pqp_pktq_B)) = PQP_PKTQ_HEAD(pqp_pktq_A);
			PQP_PKTQ_HEAD(pqp_pktq_A)               = PQP_PKTQ_HEAD(pqp_pktq_B);
		}
		PQP_PKTQ_PKTS(pqp_pktq_A)                  += PQP_PKTQ_PKTS(pqp_pktq_B);
		goto reset_pktq_B_pgo_req_A;          // Reset Queue B, PGO Queue A

	}   // End Case 0: Both Queues are NOT PQP managed

	// +-----------------------------------------------------------------------
	//  Case 1: Queue A is not PQP managed and Queue B is PQP managed
	// +-----------------------------------------------------------------------
	if (pqp_req_A == PQP_REQ_NULL) // (pqp_req_B != PQP_REQ_NULL))
	{
		pqp_req_t * pqp_req = pqp_req_B; // Reuse Queue B's pqp_req

		// Page Out Queue B first if there are DBM packets
		if (PQP_PKTQ_PKTS(pqp_pktq_B)) {
			// By default, packets held in Q::B are prepended.
			pqp_pgo(pqp_pktq_B, PQP_PREPEND, cb_ctx);
		}

		// Move the PQP request B from Queue B to Queue A
		PQP_PKTQ_REQ(pqp_pktq_A) = pqp_req;
		pqp_req->pqp_pktq        = pqp_pktq_A;
		PQP_ASSERT(pqp_req->ctx == cb_ctx);
		pqp_req->ctx             = cb_ctx;
		__pqp_pktq_req_clr(pqp_pktq_B);  // Dettach pqp_req from Queue B

		// Since Queue B will be page out first, need to reverse the pqp_policy.
		pqp_policy ^= PQP_APPEND;

		goto pgo_pktq_A;                  // PGO Queue A
	}   // Case 1: A is not PQP managed and B is PQP managed

	// +-----------------------------------------------------------------------
	//  Case 2: Queue A is PQP managed and Queue B is not PQP managed
	// +-----------------------------------------------------------------------
	if (pqp_req_B == PQP_REQ_NULL)
	{
		// Page Out Queue A first if there are DBM packets
		if (PQP_PKTQ_PKTS(pqp_pktq_A)) {
			// By default, packets held in Q::A are prepended.
			pqp_pgo(pqp_pktq_A, PQP_PREPEND, cb_ctx);
		}
		// Move Queue B packets to Queue A
		PQP_PKTQ_HEAD(pqp_pktq_A) = PQP_PKTQ_HEAD(pqp_pktq_B);
		PQP_PKTQ_TAIL(pqp_pktq_A) = PQP_PKTQ_TAIL(pqp_pktq_B);
		PQP_PKTQ_PKTS(pqp_pktq_A) = PQP_PKTQ_PKTS(pqp_pktq_B);
		goto reset_pktq_B_pgo_req_A;          // Reset Queue B, PGO Queue A
	}   // Case 2: Queue A is PQP managed and Queue B is not PQP managed

	// +-----------------------------------------------------------------------
	//  Case 3: Both Queues are PQP managed, i.e. hbm_cnt > 0
	// +-----------------------------------------------------------------------
	PQP_ASSERT(pqp_req_A->pkts.hbm_cnt != 0);
	PQP_ASSERT(pqp_req_B->pkts.hbm_cnt != 0);

	// Page Out Queue A if there are DBM packets
	if (PQP_PKTQ_PKTS(pqp_pktq_A)) { // Leading Q::A packets are in DBM
		pqp_pgo(pqp_pktq_A, PQP_PREPEND, cb_ctx);
	}

	// Page Out Queue B if there are DBM packets
	if (PQP_PKTQ_PKTS(pqp_pktq_B)) { // Leading Q::B packets are in DBM
		pqp_pgo(pqp_pktq_B, PQP_PREPEND, cb_ctx);
	}

	// Case 3.1 PQP_APPEND	A[head, tail][ HBM ]  B[head, tail][ HBM ]
	// Case 3.2 PQP_PREPEND  B[head, tail][ HBM ]  A[head, tail][ HBM ]
	_pqp_join_req(pqp_mgr, pqp_req_A, pqp_req_B, pqp_policy);

	// fall through: dettach, delink and free PQP::B

	__pqp_pktq_req_clr(pqp_pktq_B);                // Dettach from Queue B
	__pqp_mgr_req_put(pqp_mgr, pqp_req_B);         // Delink and free Request B

	return BCME_OK;

reset_pktq_B_pgo_req_A:
	_pqp_pktq_empty(pqp_pktq_B);                   // Empty out Queue B

pgo_pktq_A:                                        // PGO Queue A
	return pqp_pgo(pqp_pktq_A, pqp_policy, cb_ctx);

}   // pqp_join()

// +--------------------------------------------------------------------------+
//  PGI Section
// +--------------------------------------------------------------------------+

static void // Resume wl apps if queue has the minimum requested packets
_pqp_pgi_qcb(pqp_mgr_t * pqp_mgr, pqp_req_t * pqp_req, pqp_pktq_t * pqp_pktq)
{
	// Check whether queue has the minimum requested packets
	if ((PQP_PKTQ_PKTS(pqp_pktq) >= PQP_PKTQ_CONT(pqp_pktq)) ||
	    (PQP_PKTQ_FILL(pqp_pktq) == 0))
	{
		pqp_cb_t pqp_cb;

		PQP_DEBUG(PQP_PRINT("PQP: _pqp_pgi_qcb"));
		pqp_cb.osh = pqp_mgr->wl_osh;
		pqp_cb.ctx = pqp_req->ctx;
		pqp_cb.pqp_pktq = pqp_pktq;

		pqp_mgr->pgi_qcb(&pqp_cb); // resume wl apps
	}

}   // _pqp_pgi_qcb()

// +--------------------------------------------------------------------------+

static INLINE pqp_pkt_t *
__pqp_pgi_fix_pkt(pqp_mgr_t * pqp_mgr, pqp_pkt_t * dbm_pkt)
{
	osl_t * osh = pqp_mgr->wl_osh;

	PQP_DEBUG(PQP_PRINT("PQP: __pqp_pgi_fix_pkt dbm %p", dbm_pkt));

	if (PKTDATAISLOCAL(osh, dbm_pkt)) // LBF_MGMT_TX_PKT || LBF_BUF_ALLOC
	{
		uchar  * data_buf;
		uint16   buf_size;

		uint16   headroom = PKTHEADROOM(osh, dbm_pkt);
		uint16   pktlen   = PKTLEN(osh, dbm_pkt);
		bool     pkt_drop = FALSE;

		// MGMT frame or data buffer in local should not have next pointer.
		if (PKTNEXT(osh, dbm_pkt)) {
			PQP_PRINT("PQP: dbm %p data is local but it has next\n", dbm_pkt);
			ASSERT(0);
		}

		if (PKTISMGMTTXPKT(osh, dbm_pkt)) // PKTGET and transfer to it.
		{
			pqp_pkt_t * mgmt_pkt;

#if defined(PQP_USE_MGMT_LOCAL)

			ASSERT(!PKTHASHMEDATA(osh, dbm_pkt));

			// Use original mgmt local buf
			mgmt_pkt = (pqp_pkt_t *)PKTLOCAL(osh, dbm_pkt);
			PKTRESETLOCAL(osh, mgmt_pkt);

			// HBM link will not be set for original local buf
			// when dirver use sb2pcie to update the hbm link.
			// Sync HBM link to original local buf before free the dbm packet.
			PQP_HBM_LINK(mgmt_pkt) = PQP_HBM_LINK(dbm_pkt);

			// Free dbm_pkt
			__pqp_mgr_dbm_put(pqp_mgr, dbm_pkt);

			PQP_DEBUG(PQP_PRINT("PQP: __pqp_pgi_fix_pkt dbm %p to mgmt %p\n",
			                    dbm_pkt, mgmt_pkt));

			return mgmt_pkt;
#else
			uint16      cpy_size;
			buf_size = headroom + pktlen;

			// Use heap buffer from pre-allocated pqp heap buffer pool
			// if the available memory is under the threshold
			if (pqp_mgr->dbm_pkt.mem_avail < PQP_PGI_MEM_THRESH) {
				mgmt_pkt = (pqp_pkt_t *)__pqp_mgr_heap_buf_get(pqp_mgr);

				// Packet size from pqp heap pool is MAXPKTDATABUFSZ.
				// If MGMT packet size is bigger than MAXPKTDATABUFSZ.
				// Set LBF_HWA_DDBM_PKT flag for later free and drop it.
				if ((PQP_PKT_CTX_SZ + buf_size) > MAXPKTDATABUFSZ) {
					PQP_PRINT("PQP: mgmt dbm %p size %d bigger than pool\n",
						dbm_pkt, (PQP_PKT_CTX_SZ + buf_size));
					PKTSETHWADDMBPKT(osh, dbm_pkt);
					__pqp_mgr_heap_buf_put(pqp_mgr, mgmt_pkt);
					pkt_drop = TRUE;
					goto hmedata_put;
				}

				// Set PQPHEAPPOOL if this mgmt_pkt is allocated from pool.
				// Use this flag to identify the packet is coming from pool or heap.
				PKTSETPQPHEAPPOOL(osh, dbm_pkt);
			} else {
				// Recover the credit because it will decrease in hnd_pkt_get().
				pqp_hbm_avail_add(1);
				mgmt_pkt = (pqp_pkt_t *)PKTGET(osh, buf_size, TRUE);
				ASSERT(mgmt_pkt != (pqp_pkt_t *) NULL);
			}

			cpy_size = PKTHASHMEDATA(osh, dbm_pkt)
			         ? PQP_PKT_CTX_SZ : PQP_PKT_CTX_SZ + buf_size;
			memcpy(mgmt_pkt, dbm_pkt, cpy_size); // headroom too
			__pqp_mgr_dbm_put(pqp_mgr, dbm_pkt);

			PQP_DEBUG(PQP_PRINT("PQP: __pqp_pgi_fix_pkt dbm %p to mgmt %p\n",
			                    dbm_pkt, mgmt_pkt));

			dbm_pkt  = mgmt_pkt;
			data_buf = PKTPPBUFFERP(dbm_pkt);
#endif /* PQP_USE_MGMT_LOCAL */
		}
		else
		{
			// There is no hmedata because the local buffer was kept in sysmem
			// when there is no LCL resource. So skip heap buffer allocaction.
			if (!PKTHASHMEDATA(osh, dbm_pkt)) {
				PQP_PRINT("PQP: dbm %p skip heap buffer allocaction\n",
					dbm_pkt);
				return dbm_pkt;
			}

			buf_size = MAXPKTDATABUFSZ;

			// Use heap buffer from pre-allocated pqp heap buffer pool
			// if the available memory is under the threshold
			if (pqp_mgr->dbm_pkt.mem_avail < PQP_PGI_MEM_THRESH) {
				data_buf = __pqp_mgr_heap_buf_get(pqp_mgr);

				// Clear LBF_HEAPBUF flag due to data buffer is from pool
				PKTRESETHEAPBUF(osh, dbm_pkt);
			} else {
				data_buf = hnd_malloc(MAXPKTDATABUFSZ);

				ASSERT(pqp_mgr->dbm_pkt.mem_avail >= MAXPKTDATABUFSZ);
				pqp_mgr->dbm_pkt.mem_avail -= MAXPKTDATABUFSZ;

				ASSERT(data_buf != (uchar*) NULL);
			}
		}

		PKTADJDATA(osh, dbm_pkt, data_buf, buf_size, headroom);
#if !defined(PQP_USE_MGMT_LOCAL)
hmedata_put:
#endif
		if (PKTHASHMEDATA(osh, dbm_pkt))
		{
			pqp_dma_t hme_buf_src, dbm_buf_dst;

			dbm_buf_dst.u64 = 0;
			dbm_buf_dst.ptr = (void*) PKTDATA(osh, dbm_pkt);
			HADDR64_HI_SET(hme_buf_src.haddr64, PKTHME_HI(osh, dbm_pkt));
			HADDR64_LO_SET(hme_buf_src.haddr64, PKTHME_LO(osh, dbm_pkt));

			PKTSETHMELEN(osh,  dbm_pkt,   0);
			PKTHME_HI(osh,     dbm_pkt) = 0U;
			PKTHME_LO(osh,     dbm_pkt) = 0U;
			PKTRESETHMEDATA(osh, dbm_pkt);

			PQP_DEBUG(
				PQP_PRINT("PQP: _pqp_pgi_dma_buf %p %u lcl" HADDR64_FMT "\n",
					dbm_buf_dst.ptr, pktlen, HADDR64_VAL(hme_buf_src.haddr64)));

			if (!pkt_drop) {
				bme_copy64(pqp_mgr->rte_osh, pqp_mgr->bme_h2d,
				           hme_buf_src.u64, dbm_buf_dst.u64, pktlen);
				PQP_STATS(pqp_mgr->stats.pgi.dma++);

				bme_sync_usr(pqp_mgr->rte_osh, BME_USR_H2D);
			}

			hme_put(HME_USER_LCLPKT, pktlen, hme_buf_src.haddr64);
		}

	} else {
		uint16   headroom = PKTHEADROOM(osh, dbm_pkt);
		uchar  * data_buf = PKTPPBUFFERP(dbm_pkt);
		PKTADJDATA(osh, dbm_pkt, data_buf, PQP_PKT_BUF_SZ, headroom);
	}

	return dbm_pkt;

}   // __pqp_pgi_fix_pkt()

// +--------------------------------------------------------------------------+

static INLINE pqp_pkt_t * // BME mem2mem transfer pqp_pkt_t from HBM to DBM
__pqp_pgi_dma_pkt(pqp_mgr_t * pqp_mgr, pqp_pkt_t * dbm_pkt, uint16 hbm_id)
{
	int       bme_eng_idx;
	uint16    dbm_id;
	pqp_dma_t hbm_pkt_src, dbm_pkt_dst;

	dbm_id              = PQP_DBM_ID(dbm_pkt); // save DDBM PktMapID

	PQP_DEBUG(PQP_PRINT("PQP: __pqp_pgi_dma_pkt hbm_id %u to dbm %p pktid %u",
	                    hbm_id, dbm_pkt, dbm_id));

	hbm_pkt_src.u64     = pqp_mgr->hme_pkt.dma.u64;
	hbm_pkt_src.uptr    = PQP_HBM_ID2DMA(pqp_mgr, hbm_id);
	dbm_pkt_dst.u64     = 0;
	dbm_pkt_dst.ptr     = (void *) dbm_pkt;

	/* Transfer from HBM packet into DDBM managed HWA packet */
	bme_eng_idx         =
		bme_copy64(pqp_mgr->rte_osh, pqp_mgr->bme_h2d,
	               hbm_pkt_src.u64, dbm_pkt_dst.u64, PQP_PKT_SZ);
	PQP_STATS(pqp_mgr->stats.pgi.dma++);
	bme_sync_eng(pqp_mgr->rte_osh, bme_eng_idx); // sync done
	PQP_DBM_ID(dbm_pkt) = dbm_id; // restore DDBM PktMapId

	/* Perform all packet fixups (may copy from HWA DBM to PKTGET) */
	dbm_pkt = __pqp_pgi_fix_pkt(pqp_mgr, dbm_pkt);

	// Free HBM packet
	__pqp_mgr_hbm_id_put(pqp_mgr, hbm_id);

	PQP_DEBUG(_pqp_pktio_dump(dbm_pkt, "PGI", "DMA"));

	return dbm_pkt;

}   // __pqp_pgi_dma_pkt()

// +--------------------------------------------------------------------------+

static INLINE pqp_pkt_t * // Page in HBM PKTNEXT linked SDUs, returning head SDU
__pqp_pgi_pdu(pqp_mgr_t * pqp_mgr, pqp_req_t * pqp_req, uint16 hbm_id)
{
	uintptr     hbm_next;
	pqp_pkt_t * dbm_temp;
	pqp_pkt_t * dbm_iter; // iterate over PKTNEXT linked SDUs
	pqp_pkt_t * dbm_prev; // fixup previous SDU's PKTNEXT pointer
	pqp_pkt_t * dbm_head; // head SDU in PKTNEXT linked SDUs

	PQP_DEBUG(PQP_PRINT("PQP: __pqp_pgi_pdu hbm_id %u", hbm_id));

	dbm_head     = PQP_DBM_PKT_NULL;
	do {
		dbm_temp = __pqp_mgr_dbm_get(pqp_mgr); // cannot fail

		// Transfer hbm pkt, perform fixups, free hbm pkt and lcl buffer if any
		dbm_iter = __pqp_pgi_dma_pkt(pqp_mgr, dbm_temp, hbm_id);

		if (dbm_head != PQP_DBM_PKT_NULL) {
			PQP_DBM_NEXT(dbm_prev) = dbm_iter;
		} else {
			dbm_head = dbm_iter;
		}
		dbm_prev = dbm_iter;

		// Next SDU in HBM
		hbm_next = PQP_HBM_NEXT(dbm_iter);
		if (!PQP_HBM_UPTR_VALID(hbm_next)) break;
		hbm_id   = PQP_HBM_PIO2ID(pqp_mgr, PQP_HBM_NEXT(dbm_iter));

	} while (1);

	pqp_mgr->pkts.hbm_cnt--;
	pqp_req->pkts.hbm_cnt--;

	return dbm_head; // head SDU in PKTNEXT linked SDUs, i.e. MPDU

}   // __pqp_pgi_pdu()

// +--------------------------------------------------------------------------+

static int // Page in pending fill count number of PDUs
_pqp_pgi_req_pktq(pqp_mgr_t * pqp_mgr,
                  pqp_req_t * pqp_req, pqp_pktq_t * pqp_pktq)
{
	uint16      hbm_id;
	pqp_pkt_t * dbm_pdu;
	int         bcme_code;
	pqp_pkt_t * dbm_drop;
	osl_t     * osh;

	// Setup locals
	bcme_code = BCME_OK;
	dbm_drop  = PQP_DBM_PKT_NULL;
	osh       = pqp_mgr->wl_osh;

	// PGI loop start
	__pqp_fsm_evt_set(pqp_mgr, PQP_FSM_PGI_STR);

	PQP_DEBUG(PQP_PRINT("PQP: _pqp_pgi_req_pktq req %p pktq %p",
	                    pqp_req, pqp_pktq));

	pqp_mgr->dbm_pkt.free_cnt  = hwa_pqp_pkt_cnt(PQP_HWA_DEV(pqp_mgr));
	pqp_mgr->dbm_pkt.mem_avail = OSL_MEM_AVAIL();
	pqp_mgr->dbm_pkt.buff_cnt  = PQP_HEAPBUF_POOL_AVAIL();

	while (PQP_PKTQ_FILL(pqp_pktq) > 0)
	{
		if (__pqp_pgi_dbm_noresource(pqp_mgr))
		{
			if (PQP_PKTQ_PKTS(pqp_pktq)) {
				PQP_ASSERT(PQP_PKTQ_TAIL(pqp_pktq)   != PQP_DBM_PKT_NULL);
				PQP_DBM_LINK(PQP_PKTQ_TAIL(pqp_pktq)) = PQP_DBM_PKT_NULL;
			}
			__pqp_fsm_evt_set(pqp_mgr, PQP_FSM_DBM_OOM);
			bcme_code = BCME_NORESOURCE;
			goto done;
		}

		// Pick head HBM pdu in first HBM segment managed by PQP request
		hbm_id = pqp_req->hbm_id.head;
		PQP_ASSERT_HBM_ID(hbm_id);

		// Page in entire PDU, by traversing HBM PKTNEXT linked SDUs
		dbm_pdu = __pqp_pgi_pdu(pqp_mgr, pqp_req, hbm_id);

		PQP_ASSERT(dbm_pdu != PQP_DBM_PKT_NULL);

		// If MGMT packet still use ddbm, it means this packet need to be dropped.
		// Set drop_pkt = TRUE and free later.
		if (PKTISMGMTTXPKT(osh, dbm_pdu) &&
			PKTISHWADDMBPKT(osh, dbm_pdu)) {
			dbm_drop = dbm_pdu;
		}

		// Skip then enqueue if packet will be dropped.
		if (dbm_drop == PQP_DBM_PKT_NULL) {
			// Enqueue into pqp_pktq, at tail
			if (PQP_PKTQ_PKTS(pqp_pktq)) {
				PQP_DBM_LINK(PQP_PKTQ_TAIL(pqp_pktq)) = dbm_pdu;
			} else {
				PQP_PKTQ_HEAD(pqp_pktq) = dbm_pdu;
			}
			PQP_PKTQ_TAIL(pqp_pktq) = dbm_pdu;

			PQP_PKTQ_PKTS(pqp_pktq)++;
		}

		PQP_PKTQ_FILL(pqp_pktq)--;

		if (pqp_req->hbm_id.head == pqp_req->hbm_id.tail)
		{
			if (pqp_req->pkts.hbm_cnt) {
				PQP_ERROR("PQP: FAILURE %s req %p pktq %p hbm_cnt(%d) not zero",
					__FUNCTION__, pqp_req, pqp_pktq, pqp_req->pkts.hbm_cnt);
				ASSERT(0);
			}

			__pqp_pktq_sdu_reset(dbm_pdu);

			// If the packet is dropped, then free the packet.
			if (dbm_drop) {
				PKTFREE(osh, dbm_drop, TRUE);
				dbm_drop = PQP_DBM_PKT_NULL;
			}

			if (pqp_req->pkts.hbm_cnt == 0) { // dettach pqp_req::pktq
				PQP_PKTQ_REQ(pqp_pktq) = PQP_REQ_NULL;
				pqp_req->pqp_pktq      = PQP_PKTQ_NULL;
				break;
			}

		} else {
			pqp_req->hbm_id.head = // advance HBM head
				PQP_HBM_PIO2ID(pqp_mgr, PQP_HBM_LINK(dbm_pdu));

			/* Reset the prev dbm_pdu Link pointer */
			PQP_DBM_LINK(dbm_pdu) = PQP_DBM_PKT_NULL;
		}

		__pqp_pktq_sdu_reset(dbm_pdu);

		// If the packet is dropped, then free the packet.
		if (dbm_drop) {
			PKTFREE(osh, dbm_drop, TRUE);
			dbm_drop = PQP_DBM_PKT_NULL;
		}
	} // fill_pkts

done:
	// PGI loop done
	__pqp_fsm_evt_clr(pqp_mgr, PQP_FSM_PGI_STR);

	return bcme_code;

}   // _pqp_pgi_req_pktq()

// +--------------------------------------------------------------------------+

static int // Wake pending PGI requests that may be pending in PGI list
_pqp_pgi_wake(pqp_mgr_t * pqp_mgr)
{
	int          bcme_code;
	pqp_list_t * pqp_list_pgi = __pqp_list(pqp_mgr, PQP_LIST_IDX_PGI);

	// PGI is under processing.
	if (__pqp_fsm_evt_get(pqp_mgr, PQP_FSM_PGI_STR))
		return BCME_OK;

	// Iterate through PGI list of requests pending page in
	while (__pqp_list_cnt(pqp_list_pgi) > 0)
	{
		pqp_req_t  * pqp_req  = __pqp_list_head(pqp_list_pgi);
		pqp_pktq_t * pqp_pktq = pqp_req->pqp_pktq;

		bcme_code = _pqp_pgi_req_pktq(pqp_mgr, pqp_req, pqp_pktq);

		// If from a PktPgr resume callback, then need to explicitly invoke QCB
		if (__pqp_fsm_evt_get(pqp_mgr, PQP_FSM_DBM_OOM)) {
			// check if pktq has the minimum requested packets
			_pqp_pgi_qcb(pqp_mgr, pqp_req, pqp_pktq);
		} else { // Adjust QCB invocation
			if (PQP_PKTQ_CONT(pqp_pktq) >= PQP_PKTQ_PKTS(pqp_pktq)) {
				PQP_PKTQ_CONT(pqp_pktq) -= PQP_PKTQ_PKTS(pqp_pktq);
			} else {
				PQP_PKTQ_CONT(pqp_pktq)  = 0;
			}
		}

		if (PQP_PKTQ_FILL(pqp_pktq) == 0) // Request has been fullfilled
		{
			if (pqp_req->pkts.dbm_cnt) {
				__pqp_mgr_transfer(pqp_mgr, pqp_req,
				                   PQP_LIST_IDX_PGI, PQP_LIST_IDX_PGO);
			} else if (pqp_req->pkts.hbm_cnt) {
				__pqp_mgr_transfer(pqp_mgr, pqp_req,
				                   PQP_LIST_IDX_PGI, PQP_LIST_IDX_PGO);
			} else {
				__pqp_pktq_req_clr(pqp_pktq); // Dettach request from queue
				__pqp_mgr_req_put(pqp_mgr, pqp_req); // Free request
			}
		}

		if (bcme_code) break; // BCME_NORESOURCE

		// If PQP_FSM_PGI_SPL is set, only process one pending page in request at a time.
		if (__pqp_fsm_evt_get(pqp_mgr, PQP_FSM_PGI_SPL)) {
			break;
		}

	}   // while

	// If PGI list is empty, then PQP manager is not stalled for DBM
	if (__pqp_list_cnt(pqp_list_pgi) == 0) {
		__pqp_fsm_evt_clr(pqp_mgr, PQP_FSM_DBM_OOM);
	}

	return bcme_code;
}   // _pqp_pgi_wake()

// +--------------------------------------------------------------------------+

int // Page in pqp_pktq::req_pkts and place a qcb_min_thresh pkts into pqp_pktq
pqp_pgi(pqp_pktq_t * pqp_pktq, int cont_pkts, int fill_pkts)
{
	pqp_mgr_t  * pqp_mgr    = PQP_MGR();
	pqp_req_t  * pqp_req    = __pqp_pktq_req_get(pqp_pktq);

	PQP_DEBUG(PQP_PRINT("PQP: pqp_pgi req %p pktq %p", pqp_req, pqp_pktq));
	PQP_DEBUG(pqp_pktq_dump(pqp_pktq));

	PQP_STATS(pqp_mgr->stats.pgi.req++);

	// Not pqp managed
	if (pqp_req == PQP_REQ_NULL) {
		PQP_DEBUG(PQP_PRINT("PQP: pqp_pgi NULL"));
		return PQP_PKTQ_PKTS(pqp_pktq);
	}

	// No page-in transition, as queue holds packets to service fill request
	if (PQP_PKTQ_PKTS(pqp_pktq) >= fill_pkts) {
		PQP_DEBUG(PQP_PRINT("PQP: pqp_pgi fill %u in queue %u",
			fill_pkts, PQP_PKTQ_PKTS(pqp_pktq)));
		// pqp_req is not moved to pgi list
		return PQP_PKTQ_PKTS(pqp_pktq);
	}

	// Save PQP request PGI parameters
	// Design: Overwrites a previous cont_pkts and fill_pkts
	PQP_PKTQ_FILL(pqp_pktq) = fill_pkts - PQP_PKTQ_PKTS(pqp_pktq);
	// minimum threshold for invoking qcb
	PQP_PKTQ_CONT(pqp_pktq) = MIN(cont_pkts, PQP_PKTQ_FILL(pqp_pktq));

	// Transition PQP request to pgi list
	if (__pqp_fsm_evt_get(pqp_mgr, PQP_FSM_PGI_SPL)) {
		// Page in this pqp_pktq first for special handling.
		// Ex. PS flush, map_pkts, and PS age out filter.
		__pqp_mgr_transfer_head(pqp_mgr, pqp_req,
		                   pqp_req->list_idx, PQP_LIST_IDX_PGI);
	} else if (pqp_req->list_idx != PQP_LIST_IDX_PGI) {
		__pqp_mgr_transfer(pqp_mgr, pqp_req,
		                   pqp_req->list_idx, PQP_LIST_IDX_PGI);
	}

	return _pqp_pgi_wake(pqp_mgr);

}   // pqp_pgi()

// +--------------------------------------------------------------------------+

int // Callback to be invoked to resume PQP on replenishment of pqplbufpool
pqp_pgi_wake(void)
{
	pqp_mgr_t * pqp_mgr = PQP_MGR();
	PQP_DEBUG(PQP_PRINT("PQP: pqp_pgi_wake"));

	// Resume previous page-in transition when PQP Manager stalled on DBM resource or
	// PGI special handling is not set.
	if (!__pqp_fsm_evt_get(pqp_mgr, PQP_FSM_DBM_OOM) ||
		__pqp_fsm_evt_get(pqp_mgr, PQP_FSM_PGI_SPL))
		return BCME_OK;

	return _pqp_pgi_wake(pqp_mgr);
}   // pqp_pgi_wake()

// +--------------------------------------------------------------------------+

void // Set or clear flag to enable special handling for Page in
pqp_pgi_spl_set(bool pgi_spl)
{
	pqp_mgr_t * pqp_mgr = PQP_MGR();
	PQP_DEBUG(PQP_PRINT("PQP: pqp_pgi_spl %s", pgi_spl ? "set" : "clr"));

	if (pgi_spl)
		__pqp_fsm_evt_set(pqp_mgr, PQP_FSM_PGI_SPL);
	else
		__pqp_fsm_evt_clr(pqp_mgr, PQP_FSM_PGI_SPL);

	hwa_pktpgr_pqplbufpool_rsv_ddbm_set(PQP_HWA_DEV(pqp_mgr), pgi_spl);
}   // pqp_pgi_rsv_buf()

#endif /* DONGLEBUILD: HNDPQP */
