/*
 * Common (OS-independent) portion of Broadcom 802.11 Networking Device Driver
 * Tx andi PhyRx status transfer module
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
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_sts_xfer.c 808424 2022-02-17 09:05:22Z $
 */

/**
 * @file
 * @brief
 * Source file for STS_XFER module. This file contains the functionality to initialize and run
 * the HW supported TX and RX status transfers in D11 corerevs >= 129.
 */

/**
 * XXX For more information, see:
 * Confluence:[M2MDMA+programming+guide+for+TX+status+and+PHY+RX+status]
 * Confluence:[RxStatus]
 */

/* XXX: Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmdevs.h>
#include <bcm_ring.h>
#include <bcmpcie.h>
#include <bcmendian.h>
#include <wl_export.h>
#include <d11.h>
#include <hndd11.h>
#include <hnddma.h>
#include <m2mdma_core.h>	/* for m2m_core_regs_t, m2m_eng_regs_t */
#if defined(HNDMBX)
#include <hndmbx.h>
#endif /* HNDMBX */
#if defined(HNDM2M)
#include <hndm2m.h>
#endif /* HNDM2M */
#include <hwa_lib.h>
#include <wlc.h>
#include <wlc_types.h>
#include <pcicfg.h>
#include <wlc_rx.h>
#include <wlc_pub.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wlc_sts_xfer.h>
#include <pcie_core.h>
#if defined(BCMDBG)
#include <wlc_dump.h>
#endif

/*
 * ------------------------------------------------------------------------------------------------
 *  STS_XFER module provides the library of API's to transfer the PhyRx Status
 *  for MAC rev >= 129 and Tx Status for MAC rev >= 130.
 *  Details are provided in and Tx and PhyRx Status sections.
 * ------------------------------------------------------------------------------------------------
 */

/**
 * ------------------------------------------------------------------------------------------------
 * M2M Status Transfer Register specification
 *
 * For MAC revs >= 130, M2M DMA (BME) channel #3 is re-purposed by MAC to transfer Tx and PhyRx
 * Status from MAC.
 *
 * USE register sepcifications from sbhnddma.h and m2mdma_core.h.
 * Only new M2M Status registers or re-purposed" dma64regs may be listed here, using
 * naming convention from sbhnddma.h and m2mdma_core.h.
 *
 * m2m_status_eng_regs_t
 * ------------------------------------------------------------------------------------------------
 */

/** All offsets are with respect to m2m_core_regs_t */
#define M2M_STATUS_ENG_TXS_REGS_OFFSET		0x800
#define M2M_STATUS_ENG_PHYRXS_REGS_OFFSET	0x900

/* M2M Engine channel #3 is used to tranfer Tx & PhyRx Status */
#define M2M_STATUS_ENG_M2M_CHANNEL		3

/** m2m_status_eng_regs_t::cfg register */
#define M2M_STATUS_ENG_CFG_MODULEEN_SHIFT	0
#define M2M_STATUS_ENG_CFG_MODULEEN_NBITS	1
#define M2M_STATUS_ENG_CFG_MODULEEN_MASK	BCM_MASK(M2M_STATUS_ENG_CFG_MODULEEN)

/** m2m_status_eng_regs_t::ctl register */
/**< No of Statuses in queue to trigger intr */
#define M2M_STATUS_ENG_CTL_LAZYCOUNT_SHIFT	0
#define M2M_STATUS_ENG_CTL_LAZYCOUNT_NBITS	6
#define M2M_STATUS_ENG_CTL_LAZYCOUNT_MASK	BCM_MASK(M2M_STATUS_ENG_CTL_LAZYCOUNT)

/**< Size of one status unit in bytes. */
#define M2M_STATUS_ENG_CTL_MACSTATUSSIZE_SHIFT	22
#define M2M_STATUS_ENG_CTL_MACSTATUSSIZE_NBITS	8
#define M2M_STATUS_ENG_CTL_MACSTATUSSIZE_MASK	BCM_MASK(M2M_STATUS_ENG_CTL_MACSTATUSSIZE)

/** m2m_status_eng_regs_t::debug register */
/**< Status Transfer Engine State */
#define M2M_STATUS_ENG_DEBUG_STATE_SHIFT	0
#define M2M_STATUS_ENG_DEBUG_STATE_NBITS	4
#define M2M_STATUS_ENG_DEBUG_STATE_MASK		BCM_MASK(M2M_STATUS_ENG_DEBUG_STATE)

/**< Status uints in core */
#define M2M_STATUS_ENG_DEBUG_STATUS_COUNT_SHIFT	4
#define M2M_STATUS_ENG_DEBUG_STATUS_COUNT_NBITS 10
#define M2M_STATUS_ENG_DEBUG_STATUS_COUNT_MASK	BCM_MASK(M2M_STATUS_ENG_DEBUG_STATUS_COUNT)

/** m2m_status_eng_regs_t::size register */
#define M2M_STATUS_ENG_SIZE_QUEUE_SIZE_SHIFT	0
#define M2M_STATUS_ENG_SIZE_QUEUE_SIZE_NBITS	16
#define M2M_STATUS_ENG_SIZE_QUEUE_SIZE_MASK	BCM_MASK(M2M_STATUS_ENG_SIZE_QUEUE_SIZE)

/** m2m_status_eng_regs_t::wridx register */
#define M2M_STATUS_ENG_WRIDX_QUEUE_WRIDX_SHIFT	0
#define M2M_STATUS_ENG_WRIDX_QUEUE_WRIDX_NBITS	16
#define M2M_STATUS_ENG_WRIDX_QUEUE_WRIDX_MASK	BCM_MASK(M2M_STATUS_ENG_WRIDX_QUEUE_WRIDX)

/** m2m_status_eng_regs_t::rdidx register */
#define M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX_SHIFT	0
#define M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX_NBITS	16
#define M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX_MASK	BCM_MASK(M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX)

/** m2m_status_eng_regs_t::dma_template register */
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIEDP_SHIFT		0
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIEDP_NBITS		1
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIEDP_MASK		\
	BCM_MASK(M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIEDP)

#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTDP_SHIFT		1
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTDP_NBITS		1
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTDP_MASK		\
	BCM_MASK(M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTDP)

#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEADDREXTDP_SHIFT		2
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEADDREXTDP_NBITS		2
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEADDREXTDP_MASK		\
	BCM_MASK(M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEADDREXTDP)

#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIESP_SHIFT		4
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIESP_NBITS		1
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIESP_MASK		\
	BCM_MASK(M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIESP)

#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTSP_SHIFT		5
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTSP_NBITS		1
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTSP_MASK		\
	BCM_MASK(M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTSP)

#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEPERDESCWCDP_SHIFT	7
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEPERDESCWCDP_NBITS	1
#define M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEPERDESCWCDP_MASK		\
	BCM_MASK(M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEPERDESCWCDP)

/**
 * For corerev >= 130, Tx and PhyRx Status Memory region offsets inside D11 MAC.
 * Need to add backplane address with this offset for the absolute physical address,
 * from the BME's perspective.
 */
/* rev130 & rev131 */
#define D11_MAC_TXSTATUS_MEM_OFFSET					0x008d8000
#define D11_MAC_PHYRXSTATUS_MEM_OFFSET					0x008d9000
/* rev132 */
#define D11_MAC_TXSTATUS_MEM_OFFSET_REV132				0x008f0000
#define D11_MAC_PHYRXSTATUS_MEM_OFFSET_REV132				0x008f1000

#if defined(STS_XFER_M2M_INTR)

#define STS_XFER_SWITCHCORE(_sih_, _origidx_, _intr_val_)			\
({										\
	BCM_REFERENCE(_origidx_);						\
	BCM_REFERENCE(_intr_val_);						\
})

#define STS_XFER_RESTORECORE(_sih_, _coreid_, _intr_val_)

#else /* ! STS_XFER_M2M_INTR */

#define STS_XFER_SWITCHCORE(_sih_, _origidx_, _intr_val_)			\
({										\
	*(_origidx_) = 0;							\
	*(_intr_val_) = 0;							\
	 if (BUSTYPE(_sih_->bustype) == PCI_BUS) {				\
		si_switch_core(_sih_, M2MDMA_CORE_ID, _origidx_, _intr_val_);	\
	}									\
})

#define STS_XFER_RESTORECORE(_sih_, _coreid_, _intr_val_)			\
	if (BUSTYPE(_sih_->bustype) == PCI_BUS)					\
		si_restore_core(_sih_, _coreid_, _intr_val_);

#endif /* ! STS_XFER_M2M_INTR */

#if defined(WLCNT)
/** Status Transfer statistics */
#define STS_XFER_STATS
#endif /* WLCNT */

/**
 * ------------------------------------------------------------------------------------------------
 * Section: sts_xfer_ring_t
 *
 * sts_xfer_ring_t abstracts a SW to/from DMA interface implemented as a circular
 * ring. It uses a producer consumer paradigm with read and write indexes as
 * implemented in bcm_ring.h.
 *
 * Producer updates WR index and fetches RD index from consumer context.
 * Consumer updates RD index and fetches WR index from producer context.
 *
 * Each sts_xfer_ring, maintains a local bcm_ring state and is initialized with the locations
 * of the HW RD and WR registers, the ring memory base, depth, element size, ring size and
 * host coherency
 *
 * ------------------------------------------------------------------------------------------------
 */

#define STS_XFER_RING_NULL			((sts_xfer_ring_t *) NULL)
#define STS_XFER_RING_NAME_SIZE			16
#define STS_XFER_RING_STATE(sts_xfer_ring)	(&((sts_xfer_ring)->state))

/** sts_xfer_ring object to implement an interface between DMA and SW */
typedef struct sts_xfer_ring		/* Producer/Consumer circular ring */
{
	bcm_ring_t      state;		/* SW context: read and write state */
	void            *memory;	/* memory for ring */
	uint16          depth;          /* ring depth: num elements in ring */
	uint16		elem_size;	/* size of one status element */
	union {
		dmaaddr_t	memory_pa;	/* Physical (DMA MAP'ed) address of Ring */
		haddr64_t	hme_haddr64;	/* HNDMBX: host base address of ring */
	};
	char            name[STS_XFER_RING_NAME_SIZE];
} sts_xfer_ring_t;

/** Locate an element of specified type in a sts_xfer_ring at a given index. */
#define STS_XFER_RING_ELEM(type, sts_xfer_ring, index)		\
	(((type *)((sts_xfer_ring)->memory)) + (index))

#define STS_XFER_RING_NEXT_IDX(ring, index)		MODINC_POW2((index), (ring)->depth)
#define STS_XFER_RING_ADD_IDX(ring, index, val)		MODADD_POW2((index), (val), (ring)->depth)

/**
 * ------------------------------------------------------------------------------------------------
 * Section: M2M Satus transfer interrupts
 * ------------------------------------------------------------------------------------------------
 */
#if defined(STS_XFER_M2M_INTR)

typedef struct sts_xfer_m2m_intr
{
	uint32		defintmask;	/**< M2M default interrupts */
	uint32		intcontrol;	/**< M2M active interrupts */
	uint32		intstatus;	/**< M2M interrupt status */
#if !defined(DONGLEBUILD)
	uint		m2m_core_si_flag;	/**< M2m core bitmap in oobselouta30 reg */
#endif /* ! DONGLEBUILD */
	char		irqname[32];	/**< M2M core interrupt name */
} sts_xfer_m2m_intr_t;

#endif /* STS_XFER_M2M_INTR */

#if defined(STS_XFER_TXS)

/**
 * ------------------------------------------------------------------------------------------------
 * Section: Tx Status Name space: Declarations and definitions
 * ------------------------------------------------------------------------------------------------
 */

/* FIFO of 16 Byte packages are used to construct a TxStatus */
#define STS_XFER_TXS_PKG_SIZE			(sizeof(wlc_txs_pkg16_t))

#define STS_XFER_TXS_RING_ELEM_SIZE		STS_XFER_TXS_PKG_SIZE

/* Depth of Tx Status circular ring */
#define STS_XFER_TXS_RING_DEPTH			\
	(TX_STATUS_MACTXS_RING_DEPTH * (TX_STATUS_MACTXS_BYTES/STS_XFER_TXS_RING_ELEM_SIZE))

/* Aligning Tx Status buffers to 8Bytes */
#define STS_XFER_TXS_ALIGN_BYTES		8

#if defined(STS_XFER_TXS_MBX_PP)
/* Depth of Dongle Tx Status cicular ring; 16 Tx Status units */
#define STS_XFER_TXS_DNGL_RING_DEPTH		(16 * WLC_TXS40_STATUS_PKGS)
#endif /* STS_XFER_TXS_MBX_PP */

#if defined(STS_XFER_TXS_PP)
#define STS_XFER_TXS_CONSUME_POLLLOOP		10

/* Convert circular ring index to Tx Status index.
 * Tx Status is made of 2 ring buffers (2 16B packages).
 */
#define STS_XFER_TXS_RINGIDX_2_STATUSIDX(_idx)	((_idx)/WLC_TXS40_STATUS_PKGS)
#endif /* STS_XFER_TXS_PP */

#if defined(STS_XFER_STATS)

/** Tx Status Statistics */
#define TXS_STATS(txs)				(&(txs)->stats)

typedef struct txs_stats	/** Tx Status Transfer statistics */
{
	uint32 processed;	/* Total Tx Status items processed */
	uint32 prepaged;	/* Tx Statuses prepaged by Mailbox service */

#if defined(STS_XFER_TXS_PP)
	// XXX: Tx Status PKTPGR Debug counters
	uint32 pagein_req_cnt;
	uint32 pagein_proc_cnt;
	uint32 pagein_unproc_cnt;
#endif /* STS_XFER_TXS_PP */
} txs_stats_t;
#endif  /* STS_XFER_STATS */

/** Tx Status private structure */
typedef struct sts_xfer_txs
{
	volatile m2m_status_eng_regs_t *txs_regs;	/* M2M Tx Status Engine regs */
	sts_xfer_ring_t	ring;		/* Circular ring */
#if defined(STS_XFER_TXS_MBX_PP)
	sts_xfer_ring_t	dngl_ring;	/* Dongle Tx Status ring */
	void		*mbx_message_cur; /* Current Tx Status paged into MBX_SLOT_CUR slot */
	void		*mbx_message_alt; /* Next Tx Status pre-paged into MBX_SLOT_ALT slot */
	uint16		mbx_slot_cur;	/* MBX slot of current page-in transfer */
	uint16		mbx_slot_alt;	/* MBX slot of Alternate page-in transfer */
#endif /* STS_XFER_TXS_MBX_PP */
#if defined(STS_XFER_TXS_PP)
	uint16		elem_idx_ack;	/* Index of last consumed Ring elem (Read Index) */
#endif /* STS_XFER_TXS_PP */
	uint16		txs_avail;	/* Cached count of Tx Statuses available to be processed */
#if defined(STS_XFER_STATS)
	txs_stats_t	stats;		/* Tx Status Statistics */
#endif  /* STS_XFER_STATS */
	uint8		*mem_unaligned;	/* Status buffers unaligned memory */
	uint32		mem_size;	/* Unaligned memory size */
} sts_xfer_txs_t;

#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
/**
 * ------------------------------------------------------------------------------------------------
 * Section: PhyRx Status Name space: Declarations and definitions
 * ------------------------------------------------------------------------------------------------
 */

/* Global variable to store current PhyRx Status in process */
d11phyrxsts_t *hndd11_phyrxs_cur_status[HNDD11_RADIO_UNIT_MAX];

/* PhyRx Status unit bytes */
#define STS_XFER_PHYRXS_RING_ELEM_SIZE		PHYRX_STATUS_BYTES

/* Depth of PhyRx Status circular ring */
#define STS_XFER_PHYRXS_RING_DEPTH		PHYRX_STATUS_RING_DEPTH

#define PHYRXS_SEQNUM_2_RINGIDX(_ring, _seqnum)		((_seqnum) & (uint16)((_ring)->depth - 1))

/* PhyRx Status seqnum is encoded into wlc_pkttag_t::phyrxs_seqid to fetch PhyRx Status ring
 * element and release in PKTFREE.
 * using (phyrxs_seqnum + 1) as phyrxs_seqid to avoid using default value '0'
 */
#define PHYRXS_PKTTAG_SEQID_2_SEQNUM(_seqid)		((_seqid) - 1)
#define PHYRXS_SEQNUM_2_PKTTAG_SEQID(_seqnum)		((_seqnum) + 1)

/** sts_xfer_phyrxs_t::cons_seqnum invalid number */
#define PHYRXS_CONS_SEQNUM_INVALID			((uint16) (~0))

/** sts_xfer_phyrxs_t:: inuse_bitmap Accessor Macros */
#define STS_XFER_RING_GET_INUSE(phyrxs, index)	isset((phyrxs)->inuse_bitmap, (index))
#define STS_XFER_RING_SET_INUSE(phyrxs, index)	setbit((phyrxs)->inuse_bitmap, (index))
#define STS_XFER_RING_CLR_INUSE(phyrxs, index)	clrbit((phyrxs)->inuse_bitmap, (index))

/** Rx pending list to hold packets if PhyRx Status is not received */
#if defined(BCMSPLITRX)
#define RX_LIST_PEND_MAX		2
#else /* ! BCMSPLITRX */
#define RX_LIST_PEND_MAX		1
#endif /* ! BCMSPLITRX */

#define RX_LIST_PEND_FIFO0_IDX		0
#define RX_LIST_PEND_FIFO2_IDX		1

#define RX_LIST_PEND(phyrxs, idx)	(&((sts_xfer_phyrxs_t *) (phyrxs))->rx_list_pend[(idx)])

#if defined(STS_XFER_STATS)

/** PhyRx Status Statistics */
#define PHYRXS_STATS(phyrxs)		(&(phyrxs)->stats)

typedef struct phyrxs_stats		/** PhyRx Status Transfer statistics */
{
	uint32 recv;			/** Total Received */
	uint32 cons;			/** Consumed by WLAN driver */
	uint32 invalid;			/** Received with Invalid Seq number */
	uint32 miss;			/** PhyRx Statuses with Missed seqnumber */
	uint32 release;			/** PhyRx Status buffers release by WLAN */
	uint32 mpdu_miss;		/** Count of missing Rx MPDU's */
	uint32 cur_sts_access;		/** Count of current statuses accessed */
	uint32 cur_sts_release;		/** Count of current statuses released */
} phyrxs_stats_t;
#endif  /* STS_XFER_STATS */

/** PhyRx Status private structure */
typedef struct sts_xfer_phyrxs
{
	volatile m2m_status_eng_regs_t *phyrxs_regs;	/* M2M PhyRx Status Engine regs */
	sts_xfer_ring_t		ring;			/* Circular ring */
	uint8		*inuse_bitmap;			/* Bitmap for elements in use */
	rx_list_t	rx_list_pend[RX_LIST_PEND_MAX];	/* Pending Rx packets of last DPC */
	uint16		cons_seqnum[RX_LIST_PEND_MAX];	/* Per FIFO Last consumed seqnum */
	uint16		rd_last;			/* ring idx of latest consumed buffer */
#if defined(STS_XFER_PHYRXS_MBX)
	uint16		mbx_slot_cur;			/* MBX slot of current page-in transfer */
#endif /* STS_XFER_PHYRXS_MBX */
#if defined(HWA_PKTPGR_BUILD)
	uint16		seqnum_mismatch_cnt;		/* WAR for seqnum mispatch with PktPgr */
#endif /* HWA_PKTPGR_BUILD */
#if defined(STS_XFER_STATS)
	phyrxs_stats_t	stats;				/* PhyRx Status Statistics */
#endif  /* STS_XFER_STATS */
	uint8		*mem_unaligned;			/* Status buffers unaligned memory */
	uint32		mem_size;			/* Unaligned memory size */
} sts_xfer_phyrxs_t;

#endif /* STS_XFER_PHYRXS */

/**
 * ------------------------------------------------------------------------------------------------
 * Section: STS_XFER Module Name space: Declarations and definitions
 * ------------------------------------------------------------------------------------------------
 */

/* M2M corerev(si) check */
#define STS_XFER_M2M_SUPPORTED(m2m_corerev)	((m2m_corerev) == 128 ||	/* 63178, 47622 */ \
						 (m2m_corerev) == 129 ||	/* 6710 */	\
						 (m2m_corerev) == 131 ||	/* 6715 */ \
						 (m2m_corerev) == 132)		/* 68550 and 6756 */

#define STS_XFER_PCI64ADDR_HIGH			0x80000000	/* PCI64ADDR_HIGH */
#define STS_XFER_M2M_BURST_LEN			DMA_BL_64	/* 64 byte burst */

/** Mailbox service current slot invalid index */
#define STS_XFER_MBX_SLOT_INVALID		((uint16) -1)

/** STS_XFER Busy wait loop count */
#if !defined(DONGLEBUILD) || defined(HWA_PKTPGR_BUILD)
#define STS_XFER_WAIT_LOOP_COUNT		1000 * 1000
#else
#define STS_XFER_WAIT_LOOP_COUNT		10 * 1000
#endif

/** STS_XFER module private structure */
typedef struct wlc_sts_xfer {
	wlc_sts_xfer_info_t	info;			/* Public exported structure */
	wlc_info_t		*wlc;
	si_t			*sih;
#if defined(STS_XFER_TXS)
	sts_xfer_txs_t		txs;			/* Tx Status transfer handle */
#endif /* STS_XFER_TXS */
#if defined(STS_XFER_PHYRXS)
	sts_xfer_phyrxs_t	phyrxs;			/* PhyRx Status transfer handle */
#endif /* STS_XFER_PHYRXS */
	volatile m2m_core_regs_t * m2m_core_regs;	/* M2M DMA core register space */
#if defined(STS_XFER_M2M_INTR)
	sts_xfer_m2m_intr_t	m2m_intr;		/* M2M core interrupts handle */
#endif /* STS_XFER_M2M_INTR */
	bool	inited;					/* STS_XFER Module init State */
} wlc_sts_xfer_t;

#define WLC_STS_XFER_SIZE			(sizeof(wlc_sts_xfer_t))

/** Status transfer module public structures */
#define WLC_STS_XFER_INFO(sts_xfer)		(&(((wlc_sts_xfer_t *)(sts_xfer))->info))

/** Fetch STS_XFER module pointer from pointer to module's public structure. */
#define WLC_STS_XFER(sts_xfer_info) \
	({ \
		wlc_sts_xfer_t *sts_xfer_module = \
		    (wlc_sts_xfer_t *) CONTAINEROF((sts_xfer_info), wlc_sts_xfer_t, info); \
		sts_xfer_module; \
	})

/** Fetch TXS module pointer */
#define WLC_STS_XFER_TXS(sts_xfer)		(&(((wlc_sts_xfer_t *)(sts_xfer))->txs))

/** Fetch PHYRXS module pointer */
#define WLC_STS_XFER_PHYRXS(sts_xfer)		(&(((wlc_sts_xfer_t *)(sts_xfer))->phyrxs))

/** Fetch M2M interrupt module pointer */
#define WLC_STS_XFER_M2M_INTR(sts_xfer)		(&(((wlc_sts_xfer_t *)(sts_xfer))->m2m_intr))

/** STS_XFER Module statistics */
#if defined(STS_XFER_STATS)
#define STS_XFER_STATS_INCR(_stats, element)		WLCNTINCR((_stats)->element)
#define STS_XFER_STATS_ADD(_stats, element, delta)	WLCNTADD((_stats)->element, (delta))
#else /* ! STS_XFER_STATS */
#define STS_XFER_STATS_INCR(_stats, element)
#define STS_XFER_STATS_ADD(_stats, element, delta)
#endif /* ! STS_XFER_STATS */

/** STS_XFER module init/de-init handlers */
static int wlc_sts_xfer_init(void *ctx);
static int wlc_sts_xfer_deinit(void *ctx);

#if defined(BCMDBG)
/** STS_XFER module dump handlers */
static int wlc_sts_xfer_dump(void *ctx, struct bcmstrbuf *b);
static int wlc_sts_xfer_dump_clr(void *ctx);
#endif

/**
 * ------------------------------------------------------------------------------------------------
 * __sts_xfer_m2m_eng_attach:
 * - Set M2M Status transfer engine register space
 * - In NIC builds, for Chips over PCIe  mapping sixth 4KB region of PCIE BAR 0 space (0x78) to
 * M2M core register space.
 * ------------------------------------------------------------------------------------------------
 */
static INLINE void
__sts_xfer_m2m_eng_attach(wlc_sts_xfer_t *sts_xfer)
{
	si_t				*sih;
	volatile char			*base_addr;	/* M2M core register base */
	uint32				saved_core_id;
	uint32				saved_intr_val;

	sih = sts_xfer->sih;

	ASSERT(si_findcoreidx(sih, M2MDMA_CORE_ID, 0) != BADIDX);

	base_addr = (volatile char *)
		si_switch_core(sih, M2MDMA_CORE_ID, &saved_core_id, &saved_intr_val);
	ASSERT(base_addr != NULL);

	/* Take M2M core out of reset if it's not */
	if (!si_iscoreup(sih))
		si_core_reset(sih, 0, 0);

	ASSERT(STS_XFER_M2M_SUPPORTED(si_corerev(sih)));

#if !defined(DONGLEBUILD) && defined(STS_XFER_M2M_INTR)
	/* For Chips over PCIe, mapping sixth 4KB region of PCIE BAR 0 space (0x78) to
	 * M2M core register space.
	 */
	if (BUSTYPE(sih->bustype) == PCI_BUS) {
		/* Set PCIE register PCIE2_BAR0_CORE2_WIN2 with M2M core register space. */
		si_backplane_pci_config_bar0_core2_win2(sih,
			si_addrspace(sih, CORE_SLAVE_PORT_0, CORE_BASE_ADDR_0));

		/* Adjust the M2M regs with offset for PCIE2_BAR0_CORE2_WIN2 */
		base_addr = base_addr + PCIE2_BAR0_CORE2_WIN2_OFFSET;

		/* Get M2M core bitmap in DMP wrapper's oob_select_a_in register (oobselouta30).
		 * Used to configure M2M core interrupt in PCIEIntMask (PCI_INT_MASK) register.
		 */
		sts_xfer->m2m_intr.m2m_core_si_flag = si_flag(sih);
	}
#endif /* ! DONGLEBUILD && STS_XFER_M2M_INTR */

	sts_xfer->m2m_core_regs = (volatile m2m_core_regs_t *) base_addr;

#if defined(STS_XFER_TXS)
	/* Set M2M TX Status registers - Offset 0x800 */
	ASSERT(OFFSETOF(m2m_core_regs_t, txs_regs) == M2M_STATUS_ENG_TXS_REGS_OFFSET);
	sts_xfer->txs.txs_regs = &sts_xfer->m2m_core_regs->txs_regs;
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
	/* Set M2M CORE PhyRx Status registers - Offset 0x900 */
	ASSERT(OFFSETOF(m2m_core_regs_t, phyrxs_regs) == M2M_STATUS_ENG_PHYRXS_REGS_OFFSET);
	sts_xfer->phyrxs.phyrxs_regs = &sts_xfer->m2m_core_regs->phyrxs_regs;
#endif /* STS_XFER_PHYRXS */

	si_restore_core(sih, saved_core_id, saved_intr_val);

} /* __sts_xfer_m2m_eng_attach() */

/** STS_XFER module attach handler */
wlc_sts_xfer_info_t *
BCMATTACHFN(wlc_sts_xfer_attach)(wlc_info_t *wlc)
{
	wlc_sts_xfer_info_t	*sts_xfer_info;
	wlc_sts_xfer_t		*sts_xfer;

	WL_TRACE(("wl%d: %s: ENTER \n", wlc->pub->unit, __FUNCTION__));

	sts_xfer_info = NULL;

	/* Allocate STS_XFER module */
	if ((sts_xfer = MALLOCZ(wlc->osh, WLC_STS_XFER_SIZE)) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	sts_xfer_info = WLC_STS_XFER_INFO(sts_xfer);
	sts_xfer->wlc = wlc;
	sts_xfer->sih = wlc->pub->sih;
	sts_xfer_info->unit = wlc->pub->unit;

	wlc->pub->_sts_xfer_txs = FALSE;
	wlc->pub->_sts_xfer_phyrxs = PHYRXS_MODE_OFF;

	if (D11REV_LT(wlc->pub->corerev, 129)) {
		WL_ERROR(("wl%d: %s: Not enabling Tx and PhyRx Status transfer support\n",
			wlc->pub->unit, __FUNCTION__));
		return sts_xfer_info;
	}

#if defined(STS_XFER_TXS)
	/* Tx Status: Allocate Memory and intialize ADT's */
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		sts_xfer_txs_t		*txs;
		sts_xfer_ring_t		*txs_ring;
		void			*ring_memory = NULL;
		uint16			elem_size, ring_depth;
		uint16			alignment_req = STS_XFER_TXS_ALIGN_BYTES;

		txs = WLC_STS_XFER_TXS(sts_xfer);
		txs_ring = &(txs->ring);

		BCM_REFERENCE(alignment_req);
		/* Validate MAC Tx Status bytes */
		STATIC_ASSERT(sizeof(tx_status_mactxs_t) == TX_STATUS_MACTXS_BYTES);

		elem_size = STS_XFER_TXS_RING_ELEM_SIZE;
		ring_depth = STS_XFER_TXS_RING_DEPTH;
		txs->mem_size = elem_size * ring_depth;

#if defined(STS_XFER_TXS_MBX_PP)
		{	/* Allocate memory for Dongle Tx Status Ring */
			sts_xfer_ring_t	*dngl_ring;

			dngl_ring = &(txs->dngl_ring);
			dngl_ring->elem_size = STS_XFER_TXS_RING_ELEM_SIZE;
			dngl_ring->depth = STS_XFER_TXS_DNGL_RING_DEPTH;

			ring_memory = MALLOC(wlc->osh, (dngl_ring->elem_size * dngl_ring->depth));
			if (ring_memory == NULL) {
				WL_ERROR(("%s: out of mem for Dongle Tx ring, malloced %d bytes\n",
					__FUNCTION__, MALLOCED(wlc->osh)));
				goto fail;
			}

			dngl_ring->memory = ring_memory;

			snprintf(dngl_ring->name, STS_XFER_RING_NAME_SIZE,
				"wl%d_txs_dngl", wlc->pub->unit);
			bcm_ring_init(STS_XFER_RING_STATE(dngl_ring));

			/* With MBX, Tx Status circular ring memory is carved from host using
			 * HME_USER_MBXPGR after finishing HME service link_pcie handshake
			 */
			ring_memory = NULL;
		}
#else /* ! STS_XFER_TXS_MBX_PP */

		/* Allocate memory for Tx Status Ring */
#if defined(BCM_SECURE_DMA)
		txs->mem_unaligned = SECURE_DMA_MAP_STS_XFER_TXS(wlc->osh, txs->mem_size,
			alignment_req, NULL, &txs_ring->memory_pa, NULL);

		ring_memory = ALIGN_ADDR(txs->mem_unaligned, alignment_req);
#else /* ! BCM_SECURE_DMA */

		txs->mem_size += alignment_req;
		txs->mem_unaligned = MALLOC(wlc->osh, txs->mem_size);
		if (txs->mem_unaligned == NULL) {
			WL_ERROR(("wl%d: %s: out of mem for Tx ring, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			goto fail;
		}

		ring_memory = ALIGN_ADDR(txs->mem_unaligned, alignment_req);
#endif /* ! BCM_SECURE_DMA */
#endif /* ! STS_XFER_TXS_MBX_PP */

		/* Tx Status ring configuration */
		txs_ring->memory = ring_memory;
		txs_ring->depth = ring_depth;
		txs_ring->elem_size = elem_size;

		snprintf(txs_ring->name, STS_XFER_RING_NAME_SIZE, "wl%d_txs", wlc->pub->unit);
		bcm_ring_init(STS_XFER_RING_STATE(txs_ring));

		WL_INFORM(("%d: %s: Tx Status is transferred through M2M DMA Ch#3\n",
			wlc->pub->unit, __FUNCTION__));

		/* For rev >= 130, Tx Status is transferred using M2M (BME ch#3) */
		wlc->pub->_sts_xfer_txs = TRUE;
	}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
	{ /* PhyRx Status: Allocate Memory and intialize ADT's */
		sts_xfer_phyrxs_t	*phyrxs;
		sts_xfer_ring_t		*phyrxs_ring;
		void			*ring_memory = NULL;
		uint16			elem_size, ring_depth;
		uint16			alignment_req = D11PHYRXSTS_GE129_ALIGN_BYTES;

		phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);
		phyrxs_ring = &(phyrxs->ring);

		BCM_REFERENCE(alignment_req);
		/* Validate PhyRx Status bytes */
		STATIC_ASSERT(sizeof(d11phyrxsts_t) == STS_XFER_PHYRXS_RING_ELEM_SIZE);

		ring_depth = STS_XFER_PHYRXS_RING_DEPTH;
		elem_size = STS_XFER_PHYRXS_RING_ELEM_SIZE;

		/* PhyRx Status buffers should always be aligned to 8bytes */
		STATIC_ASSERT((sizeof(d11phyrxsts_t) % D11PHYRXSTS_GE129_ALIGN_BYTES) == 0);
		ASSERT(ALIGN_SIZE(elem_size, alignment_req) == elem_size);

		phyrxs->mem_size = elem_size * ring_depth;

		/** With MBX, PhyRx Status circular ring memory is carved from host using
		 * HME_USER_MBXPGR after finishing HME service link_pcie handshake
		 */
#if !defined(STS_XFER_PHYRXS_MBX)
		/* Allocate memory for PhyRx Status Ring */
#if defined(BCM_SECURE_DMA)
		 /* PhyRx Status buffers are allocated from SEC DMA mapped region.
		 * Exporting phyrxs_ring->memory_pa to hnddma module and avoiding
		 * SECURE_DMA_MAP/SECURE_DMA_UNMAP on PhyRx Status buffers.
		 */
		phyrxs->mem_unaligned = SECURE_DMA_MAP_STS_PHYRX(wlc->osh, phyrxs->mem_size,
			alignment_req, NULL, &phyrxs_ring->memory_pa, NULL);

		ring_memory = ALIGN_ADDR(phyrxs->mem_unaligned, alignment_req);

#else /* ! BCM_SECURE_DMA */
		phyrxs->mem_size += alignment_req;
		phyrxs->mem_unaligned = MALLOC(wlc->osh, phyrxs->mem_size);
		if (phyrxs->mem_unaligned == NULL) {
			WL_ERROR(("wl%d: %s: out of mem for PhyRx ring, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			goto fail;
		}

		ring_memory = ALIGN_ADDR(phyrxs->mem_unaligned, alignment_req);
#endif /* ! BCM_SECURE_DMA */
#endif /* ! STS_XFER_PHYRXS_MBX */

		/* PhyRx Status ring configuration */
		phyrxs_ring->memory = ring_memory;
		phyrxs_ring->depth = ring_depth;
		phyrxs_ring->elem_size = elem_size;

		snprintf(phyrxs_ring->name, STS_XFER_RING_NAME_SIZE, "wl%d_phyrxs", wlc->pub->unit);
		bcm_ring_init(STS_XFER_RING_STATE(phyrxs_ring));

		/* Allocate inuse bitmap */
		phyrxs->inuse_bitmap = MALLOCZ(wlc->osh, CEIL(ring_depth, NBBY));

		if (phyrxs->inuse_bitmap == NULL) {
			WL_ERROR(("wl%d: %s: out of mem for inuse_bitmap, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
			goto fail;
		}

		if (D11REV_IS(wlc->pub->corerev, 129)) {
			WL_INFORM(("%d: %s: PhyRx Status is transferred over Rx FIFO-3\n",
				wlc->pub->unit, __FUNCTION__));

			/* For rev129, PhyRx Status is transferred over RxFIFO-3 */
			wlc->pub->_sts_xfer_phyrxs = PHYRXS_MODE_FIFO;

		} else if (D11REV_GE(wlc->pub->corerev, 130)) {
			WL_INFORM(("%d: %s: PhyRx Status is transferred M2M DMA Ch#3\n",
				wlc->pub->unit, __FUNCTION__));

			/* For rev >= 130, PhyRx Status is transferred using M2M (BME ch#3) */
			wlc->pub->_sts_xfer_phyrxs = PHYRXS_MODE_M2M;
		}
	}
#endif /* STS_XFER_PHYRXS */

	if (D11REV_GE(wlc->pub->corerev, 130)) {
		/* Prepare M2M Status Engine */
		__sts_xfer_m2m_eng_attach(sts_xfer);
	}

	/* Register STS_XFER module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, NULL, "sts_xfer", sts_xfer, NULL, NULL,
		wlc_sts_xfer_init, wlc_sts_xfer_deinit) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG)
	wlc_dump_add_fns(wlc->pub, "sts_xfer", wlc_sts_xfer_dump, wlc_sts_xfer_dump_clr,
		(void *)sts_xfer_info);
#endif

	sts_xfer->inited = FALSE; /* STS_XFER is attached, Still not initialized */

	WL_INFORM(("wl%d: %s: STS_XFER module %p STS_XFER info %p \n",  wlc->pub->unit,
		__FUNCTION__, sts_xfer, sts_xfer_info));
	return sts_xfer_info; /* return the STS_XFER module public pointer */

fail:
	MODULE_DETACH(sts_xfer_info, wlc_sts_xfer_detach);
	return NULL;
} /* wlc_sts_xfer_attach() */

/** Status transfer module detach handler */
void
BCMATTACHFN(wlc_sts_xfer_detach)(wlc_sts_xfer_info_t *sts_xfer_info)
{
	wlc_info_t		*wlc;
	wlc_sts_xfer_t		*sts_xfer;

	if (sts_xfer_info == NULL)
		return;

	/* coverity[CID:32658] : sts_xfer_info is freed via containerof() */
	sts_xfer = WLC_STS_XFER(sts_xfer_info);
	wlc = sts_xfer->wlc;

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	if (D11REV_LT(wlc->pub->corerev, 129)) {
		goto done;
	}

#if defined(STS_XFER_TXS)
	/* Destruct Tx Status Ring */
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		sts_xfer_txs_t	*txs;

		txs = WLC_STS_XFER_TXS(sts_xfer);

#if defined(STS_XFER_TXS_MBX_PP)
		/* Deallocate Dongle Tx Staus Ring memory */
		{
			sts_xfer_ring_t	*dngl_ring;

			dngl_ring = &(txs->dngl_ring);

			if (dngl_ring->memory != NULL) {
				MFREE(wlc->osh, dngl_ring->memory,
					(dngl_ring->elem_size * dngl_ring->depth));
			}
		}
#else /* ! STS_XFER_TXS_MBX_PP */

		/* Deallocate Tx Status Ring memory */
		if (txs->mem_unaligned != NULL) {
#if defined(BCM_SECURE_DMA)
			SECURE_DMA_UNMAP_STS_XFER(wlc->osh, txs->mem_unaligned,
				txs->mem_size, txs->ring.memory_pa, NULL);
#else /* ! BCM_SECURE_DMA */

			DMA_UNMAP(wlc->osh, txs->ring.memory_pa,
				(txs->ring.elem_size * txs->ring.depth), DMA_RX, NULL, NULL);

			MFREE(wlc->osh, txs->mem_unaligned, txs->mem_size);
#endif /* ! BCM_SECURE_DMA */
			txs->mem_unaligned = NULL;
		}
#endif /* ! STS_XFER_TXS_MBX_PP */

	}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
	{ /* Destruct PhyRx Status Ring */
		sts_xfer_phyrxs_t	*phyrxs;

		phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);

		/* Free inuse bitmap */
		if (phyrxs->inuse_bitmap != NULL) {
			MFREE(wlc->osh, phyrxs->inuse_bitmap, CEIL(phyrxs->ring.depth, NBBY));
			phyrxs->inuse_bitmap = NULL;
		}

#if !defined(STS_XFER_PHYRXS_MBX)
		/* Deallocate PhyRx Status Ring memory */
		if (phyrxs->mem_unaligned != NULL) {
#if defined(BCM_SECURE_DMA)
			SECURE_DMA_UNMAP_STS_XFER(wlc->osh, phyrxs->mem_unaligned,
				phyrxs->mem_size, phyrxs->ring.memory_pa, NULL);
#else /* ! BCM_SECURE_DMA */

			DMA_UNMAP(wlc->osh, phyrxs->ring.memory_pa,
				(phyrxs->ring.elem_size * phyrxs->ring.depth), DMA_RX, NULL, NULL);

			MFREE(wlc->osh, phyrxs->mem_unaligned, phyrxs->mem_size);
#endif /* ! BCM_SECURE_DMA */
			phyrxs->mem_unaligned = NULL;
		}
#endif /* ! STS_XFER_PHYRXS_MBX */

	}
#endif /* STS_XFER_PHYRXS */

	/* Unregister the module */
	wlc_module_unregister(wlc->pub, "sts_xfer", sts_xfer);

	wlc->pub->_sts_xfer_txs = FALSE;
	wlc->pub->_sts_xfer_phyrxs = PHYRXS_MODE_OFF;

done:
	/* Free up the STS_XFER module memory */
	MFREE(wlc->osh, sts_xfer, WLC_STS_XFER_SIZE);
} /* wlc_sts_xfer_detach() */

/** __sts_xfer_m2m_eng_init: Initialize M2M Status Transfer Engine (BME CH#3) */
static INLINE void
__sts_xfer_m2m_eng_init(wlc_sts_xfer_t *sts_xfer)
{
	si_t		*sih;
	m2m_eng_regs_t	*m2m_eng_regs;
	uint32		saved_core_id;
	uint32		saved_intr_val;
	uint32		v32; /* used in REG read/write */
	uint32		txblen, rxblen;

	sih = sts_xfer->sih;

	si_switch_core(sih, M2MDMA_CORE_ID, &saved_core_id, &saved_intr_val);

	/* Take M2M core out of reset if it's not */
	if (!si_iscoreup(sih))
		si_core_reset(sih, 0, 0);

	/* Override DMA parameters if the chip is PCIE2 device */
	txblen = rxblen = STS_XFER_M2M_BURST_LEN;
	if (BUSCORETYPE(sih->buscoretype) == PCIE2_CORE_ID &&
		(BUSTYPE(sih->bustype) == PCI_BUS ||	/* NIC mode */
		BCMPCIEDEV_ENAB())) {	/* PCIE FD mode */
#if defined(BCMDBG)
		if (txblen > pcie_dmatxblen(sts_xfer->wlc->osh, sih)) {
			WL_ERROR(("=== CAUTION: Tx DMA BurstLen in STS_XFER is downsized "
			          "from %d to %d per PCIE MRRS ===\n",
			          txblen, pcie_dmatxblen(sts_xfer->wlc->osh, sih)));
		}
		if (rxblen > pcie_dmarxblen(sts_xfer->wlc->osh, sih)) {
			WL_ERROR(("=== CAUTION: Rx DMA BurstLen in STS_XFER is downsized "
			          "from %d to %d per PCIE MPS ===\n",
			          rxblen, pcie_dmarxblen(sts_xfer->wlc->osh, sih)));
		}
#endif /* BCMDBG */
		txblen = MIN(txblen, pcie_dmatxblen(sts_xfer->wlc->osh, sih));
		rxblen = MIN(rxblen, pcie_dmarxblen(sts_xfer->wlc->osh, sih));
	}

#if !defined(DONGLEBUILD) && defined(STS_XFER_M2M_INTR)
	/* For Chips over PCIe, mapping sixth 4KB region of PCIE BAR 0 space (0x78) to
	 * M2M core register space.
	 */
	if (BUSTYPE(sih->bustype) == PCI_BUS) {
		/* Set PCIE register PCIE2_BAR0_CORE2_WIN2 with M2M core register space. */
		si_backplane_pci_config_bar0_core2_win2(sih,
			si_addrspace(sih, CORE_SLAVE_PORT_0, CORE_BASE_ADDR_0));
	}
#endif /* ! DONGLEBUILD && STS_XFER_M2M_INTR */

	ASSERT(sts_xfer->m2m_core_regs != NULL);

	/* Offset address of M2M Status Transfer engine ch#3
	 * Transmit DMA Processor	- 0x2C0
	 * Receive DMA processor	- 0x2E0
	 */
	m2m_eng_regs = &sts_xfer->m2m_core_regs->eng_regs[M2M_STATUS_ENG_M2M_CHANNEL];

	/* XXX Some fields of XmtCtrl and RcvCtrl apply to descr processor.
	 * Use read-modify-write, to update Enable and BurstLen.
	 */

	/* First disable the transmit and receive channels of M2M Status Engine */
	AND_REG(sts_xfer->wlc->osh, &m2m_eng_regs->tx.control, ~D64_XC_XE);
	AND_REG(sts_xfer->wlc->osh, &m2m_eng_regs->rx.control, ~D64_RC_RE);

	/* Now configure the M2M DMA channel #3 */

	/* Enable the receive channel of Status transfer engine */
	v32 = R_REG(sts_xfer->wlc->osh, &m2m_eng_regs->rx.control);
	v32 = BCM_CBF(v32, D64_RC_BL);
	v32 |= (D64_RC_RE					/* Receive Enable */
		| BCM_SBF(rxblen, D64_RC_BL));	/* Recieve Burst Length */
	W_REG(sts_xfer->wlc->osh, &m2m_eng_regs->rx.control, v32);

	/* Enable the transmit channel of Status transfer engine */
	v32 = R_REG(sts_xfer->wlc->osh, &m2m_eng_regs->tx.control);
	v32 = BCM_CBF(v32, D64_XC_BL);
	v32 |= (D64_XC_XE					/* Transmit Enable */
		| BCM_SBF(txblen, D64_XC_BL));	/* Transmit Burst Length */
	W_REG(sts_xfer->wlc->osh, &m2m_eng_regs->tx.control, v32);

	si_restore_core(sih, saved_core_id, saved_intr_val);

} /* __sts_xfer_m2m_eng_init() */

/** De-initialize M2M Status Transfer engine ch#3 */
static INLINE void
__sts_xfer_m2m_eng_deinit(wlc_sts_xfer_t *sts_xfer)
{
	m2m_eng_regs_t	*m2m_eng_regs;
	uint32		saved_core_id;
	uint32		saved_intr_val;

	ASSERT(sts_xfer->m2m_core_regs != NULL);
	m2m_eng_regs = &sts_xfer->m2m_core_regs->eng_regs[M2M_STATUS_ENG_M2M_CHANNEL];

	STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

	/* Disable the transmit and receive channels of M2M Status Engine */
	AND_REG(sts_xfer->wlc->osh, &m2m_eng_regs->tx.control, ~D64_XC_XE);
	AND_REG(sts_xfer->wlc->osh, &m2m_eng_regs->rx.control, ~D64_RC_RE);

	STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

} /* __sts_xfer_m2m_eng_deinit() */

#if defined(STS_XFER_TXS) || defined(STS_XFER_PHYRXS_M2M)
/**
 * ------------------------------------------------------------------------------------------------
 *  _sts_xfer_m2m_status_eng_init: Initialize Status (Tx/PhyRx) specific Transfer Engine
 *  - Enable Status engine
 *  - Configure M2M engine source (MAC status memory) and destination (ring memory) address space
 *  - In NIC builds, enable M2M interrupts
 * ------------------------------------------------------------------------------------------------
 */
static void
BCMINITFN(_sts_xfer_m2m_status_eng_init)(wlc_sts_xfer_t *sts_xfer,
	sts_xfer_ring_t *ring, m2m_status_eng_regs_t *status_eng_regs,
	uint32 status_src_addr, uint8 lazycount, bool hme_mem)
{
	volatile m2m_eng_regs_t *m2m_eng_regs;
	void	*osh;
	uint32 da_base_l, da_base_h;
	uint32	dma_template;
	uint32	v32; /* used in REG read/write */

	osh = sts_xfer->wlc->osh;
	m2m_eng_regs = &sts_xfer->m2m_core_regs->eng_regs[M2M_STATUS_ENG_M2M_CHANNEL];

	/* Disable M2M Status engine */
	AND_REG(osh, &status_eng_regs->cfg, ~M2M_STATUS_ENG_CFG_MODULEEN_MASK);

	/* Map M2M Engine src address to MAC Status Memory region */
	W_REG(osh, &status_eng_regs->sa_base_l, status_src_addr);
	W_REG(osh, &status_eng_regs->sa_base_h, 0);

	dma_template = (M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIESP_MASK
			| M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATEPERDESCWCDP_MASK
			| M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATECOHERENTDP_MASK);

	if (hme_mem) {
		/* Status transfer is using Mailbox service */
		da_base_l = (uint32) HADDR64_LO(ring->hme_haddr64);
		da_base_h = (uint32) HADDR64_HI(ring->hme_haddr64);

		/* Map host memory address to PCIE large memory window */
		da_base_h |= STS_XFER_PCI64ADDR_HIGH;
	} else {
		ASSERT((sizeof(PHYSADDRLO(ring->memory_pa)) > sizeof(uint32)) ?
			((PHYSADDRLO(ring->memory_pa) >> 32) == 0) : TRUE);

		ASSERT((PHYSADDRHI(ring->memory_pa) & STS_XFER_PCI64ADDR_HIGH) == 0);

		da_base_l = (uint32) PHYSADDRLO(ring->memory_pa);
		da_base_h = (uint32) PHYSADDRHI(ring->memory_pa);

		if (BUSTYPE(sts_xfer->sih->bustype) == PCI_BUS) {
			/* XXX: For destination address setting, the da_base_l/h is from
			 * BME's perspective.
			 * In NIC-400; remap the host memory address to PCIE large memory window
			 */
			da_base_h |= STS_XFER_PCI64ADDR_HIGH;
		} else {

			dma_template |= M2M_STATUS_ENG_DMA_TEMPLATE_DMATEMPLATENOTPCIEDP_MASK;
		}
	}

	/* Map M2M Engine destination address to status ring memory */
	W_REG(osh, &status_eng_regs->da_base_l, da_base_l);
	W_REG(osh, &status_eng_regs->da_base_h, da_base_h);

	/* Set DMA_TEMPLATE setting */
	W_REG(osh, &status_eng_regs->dma_template, dma_template);

	/* XXX: Enable WCPD bit in RcvCtrl register as WCPD per descriptor is set
	 * in dma_template register, as suggested by HW designer, for consistency
	 */
	v32 = R_REG(osh, &m2m_eng_regs->rx.control);
	v32 |= D64_RC_WCPD;
	W_REG(osh, &m2m_eng_regs->rx.control, v32);

	/* Reset Write Index */
	v32 = BCM_SBF(0, M2M_STATUS_ENG_WRIDX_QUEUE_WRIDX);
	W_REG(osh, &status_eng_regs->wridx, v32);
	/* Reset Read Index */
	v32 = BCM_SBF(0, M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX);
	W_REG(osh, &status_eng_regs->rdidx, v32);

	/* Set status ring size (No of status elements) */
	v32 = BCM_SBF(ring->depth, M2M_STATUS_ENG_SIZE_QUEUE_SIZE);
	W_REG(osh, &status_eng_regs->size, v32);

	v32 = R_REG(osh, &status_eng_regs->ctl);
	v32 = BCM_CBF(v32, M2M_STATUS_ENG_CTL_MACSTATUSSIZE);  /* Status element size */
	v32 |= BCM_SBF(ring->elem_size, M2M_STATUS_ENG_CTL_MACSTATUSSIZE);

#if defined(STS_XFER_M2M_INTR)
	/* Interrupt on every status written to Status Queue. */
	v32 = BCM_CBF(v32, M2M_STATUS_ENG_CTL_LAZYCOUNT);	/* Status interrupt lazycount */
	v32 |= BCM_SBF(lazycount, M2M_STATUS_ENG_CTL_LAZYCOUNT);
#endif /* STS_XFER_M2M_INTR */

	W_REG(osh, &status_eng_regs->ctl, v32);

	/* Enable M2M Status engine */
	OR_REG(osh, &status_eng_regs->cfg, M2M_STATUS_ENG_CFG_MODULEEN_MASK);

	WL_INFORM(("wl%d: %s: for %s elem_size[%d] depth[%d]\n", sts_xfer->wlc->pub->unit,
		__FUNCTION__, ring->name, ring->elem_size, ring->depth));
} /* _sts_xfer_m2m_status_eng_init() */

/** De-initialize Status (Tx/PhyRx) specific Transfer Engine */
static void
BCMUNINITFN(_sts_xfer_m2m_status_eng_deinit)(wlc_sts_xfer_t *sts_xfer, sts_xfer_ring_t *ring,
	m2m_status_eng_regs_t *status_eng_regs)
{
	BCM_REFERENCE(ring);
	/* Disable M2M Status engine */
	AND_REG(sts_xfer->wlc->osh, &status_eng_regs->cfg, ~M2M_STATUS_ENG_CFG_MODULEEN_MASK);

} /* _sts_xfer_m2m_status_eng_deinit() */

/* Sync readback all M2M Status Engine register */
static void
_sts_xfer_m2m_status_eng_sync(wlc_sts_xfer_t *sts_xfer, m2m_status_eng_regs_t *status_eng_regs)
{
	void	*osh = sts_xfer->wlc->osh;

	/* sync readback */
	(void)R_REG(osh, &status_eng_regs->cfg);
	(void)R_REG(osh, &status_eng_regs->ctl);
	(void)R_REG(osh, &status_eng_regs->sts);
	(void)R_REG(osh, &status_eng_regs->debug);
	(void)R_REG(osh, &status_eng_regs->da_base_l);
	(void)R_REG(osh, &status_eng_regs->da_base_h);
	(void)R_REG(osh, &status_eng_regs->size);
	(void)R_REG(osh, &status_eng_regs->wridx);
	(void)R_REG(osh, &status_eng_regs->rdidx);
	(void)R_REG(osh, &status_eng_regs->sa_base_l);
	(void)R_REG(osh, &status_eng_regs->sa_base_h);

} /* _sts_xfer_m2m_status_eng_sync() */

#endif /* STS_XFER_TXS || STS_XFER_PHYRXS_M2M */

/** STS_XFER init handler */
static int
BCMINITFN(wlc_sts_xfer_init)(void *ctx)
{
	wlc_info_t	*wlc;
	wlc_sts_xfer_t	*sts_xfer;
	uint32		defintmask = 0;
	uint16		v16; /* used in REG read/write */

	sts_xfer = (wlc_sts_xfer_t *)ctx;
	wlc = sts_xfer->wlc;

	BCM_REFERENCE(v16);
	BCM_REFERENCE(defintmask);

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	if (D11REV_GE(wlc->pub->corerev, 130)) {
		/* Suspend MAC before programming Status Transfer Engines */
		wlc_bmac_suspend_mac_and_wait(wlc->hw);
		/* Initialize M2M DMA Ch#3 */
		__sts_xfer_m2m_eng_init(sts_xfer);
	}

#if defined(STS_XFER_TXS)
	if (STS_XFER_TXS_ENAB(wlc->pub)) {
		sts_xfer_txs_t	*txs;
		sts_xfer_ring_t	*txs_ring;
		uint32		d11_src_addr;
		uint32		saved_core_id;
		uint32		saved_intr_val;
		uint8		lazycount;
		bool		hme_mem = FALSE;

		txs = WLC_STS_XFER_TXS(sts_xfer);
		txs_ring = &(txs->ring);

		/* Reset status ring SW indexes */
		bcm_ring_init(STS_XFER_RING_STATE(txs_ring));
		txs->txs_avail = 0;

#if defined(STS_XFER_STATS)
		memset((void*)TXS_STATS(txs), 0, sizeof(txs_stats_t));
#endif /* STS_XFER_STATS */

		/* HME service link_pcie handshake occurs AFTER attach phase.
		 * So registerring a Tx Status status pager in STS_XFER init handler.
		 */
#if defined(STS_XFER_TXS_MBX_PP)
		/* Check if Tx Status is already registered with MBX */
		if (HADDR64_IS_ZERO(txs_ring->hme_haddr64)) {
			uint16 msg_len;
			uint16 msg_num;

			/* Register a pager with Mailbox service for Tx Status transfer and get
			 * Host address of ring.
			 */
			/* XXX: Since Tx Status is made of 2 16B packages, configuring Mailbox pager
			 * message length to Full Tx Status (txs_ring->elem_size * 2 = 32B).
			 * and total messages to half of Tx Status Ring (txs_ring->depth / 2).
			 */
			msg_len = txs_ring->elem_size * WLC_TXS40_STATUS_PKGS;
			msg_num = txs_ring->depth/WLC_TXS40_STATUS_PKGS;

			if (mbx_register_user(wlc->osh, MBX_USR_MACTXS, msg_len, msg_num,
				&txs_ring->hme_haddr64) != BCME_OK) {
				WL_ERROR(("wl%d: %s: MBX_USR_MACTXS user register failed\n",
					wlc->pub->unit, __FUNCTION__));
				return BCME_ERROR;
			}
		}

		hme_mem = TRUE;

		/* Reset Dongle Tx Status ring SW indexes */
		bcm_ring_init(STS_XFER_RING_STATE(&txs->dngl_ring));

		txs->elem_idx_ack = 0;
		txs->mbx_slot_cur = STS_XFER_MBX_SLOT_INVALID;
		txs->mbx_slot_alt = STS_XFER_MBX_SLOT_INVALID;
		txs->mbx_message_cur = NULL;
		txs->mbx_message_alt = NULL;
#else /* ! STS_XFER_TXS_MBX_PP */

#if defined(STS_XFER_TXS_PP)
		txs->elem_idx_ack = 0;
#endif /* STS_XFER_TXS_PP */

#if !defined(BCM_SECURE_DMA)
		/* Map ring memory for device use only */
		txs_ring->memory_pa = DMA_MAP(wlc->osh, txs_ring->memory,
			(txs_ring->elem_size * txs_ring->depth), DMA_RX, NULL, NULL);
#endif /* ! BCM_SECURE_DMA */

		ASSERT(ISALIGNED(PHYSADDRLO(txs_ring->memory_pa), STS_XFER_TXS_ALIGN_BYTES));
#endif /* STS_XFER_TXS_MBX_PP */

		STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

		d11_src_addr = SI_ENUM_BASE_BP(sts_xfer->sih);

		if (D11REV_IS(wlc->pub->corerev, 132))
			d11_src_addr += D11_MAC_TXSTATUS_MEM_OFFSET_REV132;
		else
			d11_src_addr += D11_MAC_TXSTATUS_MEM_OFFSET;

		lazycount = WLC_TXS40_STATUS_BYTES/txs_ring->elem_size; // 2 16B packages

		/* Prepare and enable M2M Tx Status Engine */
		_sts_xfer_m2m_status_eng_init(sts_xfer, txs_ring, txs->txs_regs,
			d11_src_addr, lazycount, hme_mem);

		STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

#if defined(STS_XFER_M2M_INTR)
		/* Enable M2M Interrupt on update of Tx Status Write Index */
		defintmask |= BCM_SBF(1, M2M_CORE_INTCONTROL_TXSWRINDUPD_INTMASK);
#endif /* STS_XFER_M2M_INTR */

		/**
		 * HWA_MACIF_CTL register settings for Tx Status
		 * - Enable Req-Ack based MAC_HWA i/f
		 * - Tx Status memory Access using AXI Slave Acess.
		 * - 2 16B packages per one Status.
		 */
		v16 = BCM_SBF(1, _HWA_MACIF_CTL_TXSTATUSEN);
		v16 |= BCM_SBF(_HWA_MACIF_CTL_TXSTATUSMEM_AXI, _HWA_MACIF_CTL_TXSTATUSMEM);
		v16 |= BCM_SBF(STS_XFER_TXS_RING_ELEM_SIZE/STS_XFER_TXS_PKG_SIZE,
			_HWA_MACIF_CTL_TXSTATUS_COUNT);

		/* CAUTION: HWA_MACIF_CTL register is also configured by HWA blocks.
		 * SET/RESET only Tx Status fields.
		 */
		HWA_UPD_REG16_ADDR(STS_XFER, D11_HWA_MACIF_CTL(wlc),
			v16, _HWA_MACIF_CTL_TXSTATUS_XFER);

		// For Debug; Access full Tx Status FIFO
		// v16 = BCM_SBF(1, _HWA_MACIF_CTL_TXSTATUSFIFO_AXI);
		// HWA_UPD_REG16_ADDR(STS_XFER, D11_HWA_MACIF_CTL(wlc),
		//	v16, _HWA_MACIF_CTL_TXSTATUSFIFO_AXI);

		/* Sync readback all Tx Status transfer engine registers */
		_sts_xfer_m2m_status_eng_sync(sts_xfer, txs->txs_regs);
	}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
	if (STS_XFER_PHYRXS_ENAB(wlc->pub)) {
		sts_xfer_phyrxs_t	*phyrxs;
		sts_xfer_ring_t		*phyrxs_ring;
		uint32			idx;

		phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);
		phyrxs_ring = &(phyrxs->ring);

		/* Reset status ring SW indexes */
		bcm_ring_init(STS_XFER_RING_STATE(phyrxs_ring));
		phyrxs->rd_last = 0;

		for (idx = 0; idx < RX_LIST_PEND_MAX; idx++) {
			RX_LIST_INIT(RX_LIST_PEND(phyrxs, idx));
			phyrxs->cons_seqnum[idx] = PHYRXS_CONS_SEQNUM_INVALID;
		}

		/* Status units might be dropped in reclaim path; Reset inuse bitmap */
		memset((void *)phyrxs->inuse_bitmap, 0, CEIL(phyrxs_ring->depth, NBBY));

#if defined(STS_XFER_STATS)
		memset((void*)PHYRXS_STATS(phyrxs), 0, sizeof(phyrxs_stats_t));
#endif /* STS_XFER_STATS */

		/* HME service link_pcie handshake occurs AFTER attach phase.
		 * So registering a status pager and configuring STS_FIFO DMA params in
		 * STS_XFER init handler.
		 */
#if defined(STS_XFER_PHYRXS_MBX)
		/* Check if PhyRx Status is already registered with MBX */
		if (HADDR64_IS_ZERO(phyrxs_ring->hme_haddr64)) {
			/* Register a pager with Mailbox service for PhyRx Status transfer and get
			 * Host address of ring.
			 */
			if (mbx_register_user(wlc->osh, MBX_USR_PHYRXS, phyrxs_ring->elem_size,
				phyrxs_ring->depth, &phyrxs_ring->hme_haddr64) != BCME_OK) {
				WL_ERROR(("wl%d: %s: MBX_USR_PHYRXS register failed\n",
					wlc->pub->unit, __FUNCTION__));
				return BCME_ERROR;
			}
		}

		phyrxs->mbx_slot_cur = STS_XFER_MBX_SLOT_INVALID;

#else /* ! STS_XFER_PHYRXS_MBX */

#if !defined(BCM_SECURE_DMA)
		/* Map ring memory for device use only */
		phyrxs_ring->memory_pa = DMA_MAP(wlc->osh, phyrxs_ring->memory,
			(phyrxs_ring->elem_size * phyrxs_ring->depth), DMA_RX, NULL, NULL);
#endif /* ! BCM_SECURE_DMA */

		ASSERT(ISALIGNED(PHYSADDRLO(phyrxs_ring->memory_pa),
			D11PHYRXSTS_GE129_ALIGN_BYTES));
#endif /* ! STS_XFER_PHYRXS_MBX */

#if defined(STS_XFER_PHYRXS_FIFO)
		if (STS_XFER_PHYRXS_FIFO_ENAB(wlc->pub)) {

			ASSERT((wlc->hw->di[STS_FIFO]) != NULL);
			/* Set PhyRx Status ring memory for STS_FIFO dma */
#if defined(STS_XFER_PHYRXS_MBX)
			dma_sts_set_memory(wlc->hw->di[STS_FIFO], PHYRXS_MBX,
				HADDR64_HI(phyrxs_ring->hme_haddr64),
				HADDR64_LO(phyrxs_ring->hme_haddr64));
#else /* ! STS_XFER_PHYRXS_MBX */
			dma_sts_set_memory(wlc->hw->di[STS_FIFO], PHYRXS,
				PHYSADDRHI(phyrxs_ring->memory_pa),
				PHYSADDRLO(phyrxs_ring->memory_pa));
#endif /* ! STS_XFER_PHYRXS_MBX */

			/* Post PhyRx buffers to STS_FIFO */
			dma_sts_rxinit(wlc->hw->di[STS_FIFO]);
			wlc_bmac_dma_rxfill(wlc->hw, STS_FIFO);
		}
#endif /* STS_XFER_PHYRXS_FIFO */

#if defined(STS_XFER_PHYRXS_M2M)
		if (STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
			uint32	d11_src_addr;
			uint32	saved_core_id;
			uint32	saved_intr_val;
			bool	hme_mem = FALSE;

			STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

			d11_src_addr = SI_ENUM_BASE_BP(sts_xfer->sih);

			if (D11REV_IS(wlc->pub->corerev, 132))
				d11_src_addr += D11_MAC_PHYRXSTATUS_MEM_OFFSET_REV132;
			else
				d11_src_addr += D11_MAC_PHYRXSTATUS_MEM_OFFSET;

#if defined(STS_XFER_PHYRXS_MBX)
			hme_mem = TRUE;
#endif /* STS_XFER_PHYRXS_MBX */

			/* Prepare and enable M2M PhyRx Status Engine; lazycount = 1 */
			_sts_xfer_m2m_status_eng_init(sts_xfer, phyrxs_ring,
				phyrxs->phyrxs_regs, d11_src_addr, 1, hme_mem);

			STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

#if defined(STS_XFER_M2M_INTR)
			/* Enable M2M Interrupt on update of PhyRx Status Write Index */
			defintmask |= BCM_SBF(1, M2M_CORE_INTCONTROL_PHYRXSWRINDUPD_INTMASK);
#endif /* STS_XFER_M2M_INTR */

			/**
			 * HWA_MACIF_CTL register settings for PhyRx Status
			 * - Enable Req-Ack based MAC_HWA i/f
			 * - PhyRx Status memory Access through SHM DMA and RXE_PHYSTS_DATA_L/H
			 * - 1 package per one Status.
			 */
			v16 = BCM_SBF(1, _HWA_MACIF_CTL_PHYRXSTATUSEN);
			v16 |= BCM_SBF(1, _HWA_MACIF_CTL_PHYRXSTATUSMEM);
			v16 |= BCM_SBF(1, _HWA_MACIF_CTL_PHYRXSTATUS_COUNT);

			/* CAUTION: HWA_MACIF_CTL register is also configured by HWA blocks.
			 * SET/RESET only PhyRx Status fields.
			 */
			HWA_UPD_REG16_ADDR(STS_XFER, D11_HWA_MACIF_CTL(wlc),
				v16, _HWA_MACIF_CTL_PHYRXSTATUS_XFER);

			// For Debug; Access full Tx Status FIFO
			// v16 = BCM_SBF(1, _HWA_MACIF_CTL_PHYRXSTATUSFIFO_AXI);
			// HWA_UPD_REG16_ADDR(STS_XFER, D11_HWA_MACIF_CTL(wlc),
			//	v16, _HWA_MACIF_CTL_PHYRXSTATUSFIFO_AXI);

			/* Sync readback all PhyRx Status transfer engine registers */
			_sts_xfer_m2m_status_eng_sync(sts_xfer, phyrxs->phyrxs_regs);
		}
#endif /* STS_XFER_PHYRXS_M2M */

	}
#endif /* STS_XFER_PHYRXS */

	if (D11REV_GE(wlc->pub->corerev, 130)) {
#if defined(STS_XFER_M2M_INTR)
		sts_xfer->m2m_intr.defintmask = defintmask;

		ASSERT(defintmask != 0);
		wlc_sts_xfer_intrson(wlc);
#endif /* STS_XFER_M2M_INTR */

		/* Enable back MAC */
		wlc_bmac_enable_mac(wlc->hw);
	}

	WL_INFORM(("wl%d: %s: M2M interrupt defintmask[0x%x]\n", wlc->pub->unit, __FUNCTION__,
		defintmask));

	sts_xfer->inited = TRUE;

	return BCME_OK;
} /* wlc_sts_xfer_init() */

/** STS_XFER de-init handler */
static int
BCMUNINITFN(wlc_sts_xfer_deinit)(void *ctx)
{
	wlc_sts_xfer_t	*sts_xfer;
	wlc_info_t	*wlc;

	sts_xfer = (wlc_sts_xfer_t *) ctx;
	wlc = sts_xfer->wlc;

	WL_TRACE(("wl%d:%s: ENTER\n", sts_xfer->wlc->pub->unit, __FUNCTION__));

	if (D11REV_GE(wlc->pub->corerev, 130)) {
		/* Suspend MAC before programming Status Transfer Engines */
		wlc_bmac_suspend_mac_and_wait(wlc->hw);

		/* Wait for pending Status unit transfers to complete before disabling M2M Engine */
#if defined(STS_XFER_TXS)
		if (STS_XFER_TXS_ENAB(sts_xfer->wlc->pub)) {
			wlc_sts_xfer_txs_reclaim(wlc, TRUE);
		}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
		if (STS_XFER_PHYRXS_ENAB(sts_xfer->wlc->pub)) {
			wlc_sts_xfer_phyrxs_reclaim(wlc);
		}
#endif /* STS_XFER_PHYRXS */

		/* Deinitialize M2M Engine */
		__sts_xfer_m2m_eng_deinit(sts_xfer);

#if defined(STS_XFER_M2M_INTR)
		/* Disable M2M interrupts */
		sts_xfer->m2m_intr.defintmask = 0;
		wlc_sts_xfer_intrsoff(sts_xfer->wlc);
#endif /* STS_XFER_M2M_INTR */
	}

#if defined(STS_XFER_TXS)
	if (STS_XFER_TXS_ENAB(sts_xfer->wlc->pub)) {
		sts_xfer_txs_t	*txs;
		uint32	saved_core_id;
		uint32	saved_intr_val;

		txs = WLC_STS_XFER_TXS(sts_xfer);

		if (txs->txs_avail) {
			WL_ERROR(("wl%d: %s: %d Tx Statuses are not processed\n",
				wlc->pub->unit, __FUNCTION__, txs->txs_avail));
		}

		/* Reset status ring SW indexes */
		bcm_ring_init(STS_XFER_RING_STATE(&txs->ring));

#if defined(STS_XFER_TXS_MBX_PP)
		/* Reset Dongle Tx Status ring SW indexes */
		bcm_ring_init(STS_XFER_RING_STATE(&txs->dngl_ring));
		txs->elem_idx_ack = 0;

		ASSERT(txs->mbx_slot_cur == STS_XFER_MBX_SLOT_INVALID);
		ASSERT(txs->mbx_slot_alt == STS_XFER_MBX_SLOT_INVALID);
		ASSERT(txs->mbx_message_cur == NULL);
		ASSERT(txs->mbx_message_alt == NULL);
#elif defined(STS_XFER_TXS_PP)
		txs->elem_idx_ack = 0;
#endif /* STS_XFER_TXS_PP */

		STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

		/* Disable M2M Tx Status Engine */
		_sts_xfer_m2m_status_eng_deinit(sts_xfer, &txs->ring, txs->txs_regs);

		STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

		/* Disable Req-Ack based MAC_HWA i/f for Tx Status. */
		HWA_UPD_REG16_ADDR(STS_XFER, D11_HWA_MACIF_CTL(wlc), 0, _HWA_MACIF_CTL_TXSTATUSEN);

	}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
	if (STS_XFER_PHYRXS_ENAB(sts_xfer->wlc->pub)) {
		sts_xfer_phyrxs_t	*phyrxs;

		phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);

		/* Reset status ring SW indexes */
		bcm_ring_init(STS_XFER_RING_STATE(&phyrxs->ring));

#if defined(STS_XFER_PHYRXS_MBX)
		ASSERT(phyrxs->mbx_slot_cur == STS_XFER_MBX_SLOT_INVALID);
#endif /* STS_XFER_PHYRXS_MBX */

#if defined(STS_XFER_PHYRXS_FIFO)
		if (STS_XFER_PHYRXS_FIFO_ENAB(wlc->pub)) {
			/* Reset PhyRx Status memory in STS FIFO DMA */
#if defined(STS_XFER_PHYRXS_MBX)
			dma_sts_set_memory(wlc->hw->di[STS_FIFO], PHYRXS_MBX, 0, 0);
#else /* ! STS_XFER_PHYRXS_MBX */
			dma_sts_set_memory(wlc->hw->di[STS_FIFO], PHYRXS, 0, 0);
#endif /* ! STS_XFER_PHYRXS_MBX */

		}
#endif /* STS_XFER_PHYRXS_FIFO */

#if defined(STS_XFER_PHYRXS_M2M)
		/* Disable M2M PhyRx Status Engine */
		if (STS_XFER_PHYRXS_M2M_ENAB(sts_xfer->wlc->pub)) {
			uint32	saved_core_id;
			uint32	saved_intr_val;

			STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

			_sts_xfer_m2m_status_eng_deinit(sts_xfer, &phyrxs->ring,
				phyrxs->phyrxs_regs);

			STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

			/* Disable Req-Ack based MAC_HWA i/f for PhyRx Status. */
			HWA_UPD_REG16_ADDR(STS_XFER, D11_HWA_MACIF_CTL(wlc),
				0, _HWA_MACIF_CTL_PHYRXSTATUSEN);

		}
#endif /* STS_XFER_PHYRXS_M2M */

		BCM_REFERENCE(phyrxs);
	}
#endif /* STS_XFER_PHYRXS */

	if (D11REV_GE(wlc->pub->corerev, 130)) {
		/* Enable back MAC */
		wlc_bmac_enable_mac(wlc->hw);
	}

	sts_xfer->inited = FALSE;

	return 0; /* callbacks */
} /* wlc_sts_xfer_deinit() */

#if defined(BCMDBG)

#if defined(STS_XFER_TXS) || defined(STS_XFER_PHYRXS_M2M)
/* M2M Status Engine register dump */
static void
_sts_xfer_m2m_status_eng_dump(wlc_sts_xfer_t *sts_xfer, m2m_status_eng_regs_t *status_eng_regs)
{
	void *osh = sts_xfer->wlc->osh;

	WL_PRINT(("NAME: \t\t| OFFSET \t| VAL\n"));
	WL_PRINT(("--------------------------------------------\n"));
	WL_PRINT(("CFG: \t\t| 0x0 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->cfg)));
	WL_PRINT(("CTL: \t\t| 0x4 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->ctl)));
	WL_PRINT(("STS: \t\t| 0x8 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->sts)));
	WL_PRINT(("DEBUG: \t\t| 0xC \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->debug)));
	WL_PRINT(("DA_BASE_L: \t| 0x10 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->da_base_l)));
	WL_PRINT(("DA_BASE_H: \t| 0x14 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->da_base_h)));
	WL_PRINT(("SIZE: \t\t| 0x18 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->size)));
	WL_PRINT(("WrIdx: \t\t| 0x1C \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->wridx)));
	WL_PRINT(("RdIdx: \t\t| 0x20 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->rdidx)));
	WL_PRINT(("DMA_TEMPLATE: \t| 0x24 \t\t| 0x%08X\n",
			R_REG(osh, &status_eng_regs->dma_template)));
	WL_PRINT(("SA_BASE_L: \t| 0x0 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->sa_base_l)));
	WL_PRINT(("SA_BASE_H: \t| 0x0 \t\t| 0x%08X\n", R_REG(osh, &status_eng_regs->sa_base_h)));
	WL_PRINT(("\n"));

} /* _sts_xfer_m2m_status_eng_dump() */

/** Dump Status Transfer (M2M, HWA) registers */
static void
_sts_xfer_reg_dump(wlc_info_t *wlc, bool txs_dump)
{
	void		*osh;
	wlc_sts_xfer_t	*sts_xfer;
	m2m_eng_regs_t	*m2m_eng_regs;
	uint32		saved_core_id;
	uint32		saved_intr_val;

	osh = wlc->osh;
	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);

	ASSERT(sts_xfer->m2m_core_regs != NULL);

	m2m_eng_regs = &sts_xfer->m2m_core_regs->eng_regs[M2M_STATUS_ENG_M2M_CHANNEL];

	STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

	if (STS_XFER_TXS_ENAB(wlc->pub) || STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		WL_PRINT(("M2M Ch#3 Registers\n"));
		WL_PRINT(("NAME: \t\t| OFFSET \t| VAL\n"));
		WL_PRINT(("--------------------------------------------\n"));
		WL_PRINT(("XmtCtrl: \t| 0x2C0 \t\t| 0x%08X\n",
			R_REG(osh, &m2m_eng_regs->tx.control)));
		WL_PRINT(("XmtStatus0: \t| 0x2D0 \t\t| 0x%08X\n",
			R_REG(osh, &m2m_eng_regs->tx.status0)));
		WL_PRINT(("XmtStatus1: \t| 0x2D4 \t\t| 0x%08X\n",
			R_REG(osh, &m2m_eng_regs->tx.status1)));
		WL_PRINT(("RcvCtrl: \t| 0x2E0 \t\t| 0x%08X\n",
			R_REG(osh, &m2m_eng_regs->rx.control)));
		WL_PRINT(("RcvStatus0: \t| 0x2F0 \t\t| 0x%08X\n",
			R_REG(osh, &m2m_eng_regs->rx.status0)));
		WL_PRINT(("RcvStatus1: \t| 0x2F4 \t\t| 0x%08X\n",
			R_REG(osh, &m2m_eng_regs->rx.status1)));
	}

#if defined(STS_XFER_TXS)
	if (STS_XFER_TXS_ENAB(wlc->pub) && txs_dump) {	// Dump Tx Status engine registers
		WL_PRINT(("Tx Status Registers\n"));
		_sts_xfer_m2m_status_eng_dump(sts_xfer, WLC_STS_XFER_TXS(sts_xfer)->txs_regs);
	}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS_M2M)
	if (STS_XFER_PHYRXS_M2M_ENAB(wlc->pub) && !txs_dump) { // Dump PhyRx Status Engine reg
		WL_PRINT(("PhyRx Status Registers\n"));
		_sts_xfer_m2m_status_eng_dump(sts_xfer, WLC_STS_XFER_PHYRXS(sts_xfer)->phyrxs_regs);
	}
#endif /* STS_XFER_PHYRXS_M2M */

#if defined(STS_XFER_M2M_INTR)
	WL_PRINT(("M2M INTR: intcontrol[0x%08X] intstatus[0x%08X]\n",
		R_REG(wlc->osh, &sts_xfer->m2m_core_regs->intcontrol),
		R_REG(wlc->osh, &sts_xfer->m2m_core_regs->intstatus)));
#endif /* STS_XFER_M2M_INTR */

	STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

	WL_PRINT(("D11_HWA_MACIF_CTL[0x%04X]\n", R_REG(wlc->osh, D11_HWA_MACIF_CTL(wlc))));

	return;
} /* _sts_xfer_reg_dump() */

#endif /* STS_XFER_TXS || STS_XFER_PHYRXS_M2M */

/** Dump Status Transfer stats */
static int
wlc_sts_xfer_dump(void *ctx, struct bcmstrbuf *b)
{
	wlc_sts_xfer_t *sts_xfer = (wlc_sts_xfer_t *)ctx;
	wlc_info_t *wlc;

	wlc = sts_xfer->wlc;
	BCM_REFERENCE(wlc);

#if defined(STS_XFER_TXS)
	if (STS_XFER_TXS_ENAB(sts_xfer->wlc->pub)) {
		sts_xfer_txs_t	*txs;
		sts_xfer_ring_t	*txs_ring;

		txs = WLC_STS_XFER_TXS(sts_xfer);
		txs_ring = &(txs->ring);

		bcm_bprintf(b, "\ntxs_avail[%d]\n", txs->txs_avail);
		bcm_bprintf(b, "%s Ring: Element Size[%d] depth[%d] rdidx[%d] wridx[%d]\n",
			txs_ring->name, txs_ring->elem_size, txs_ring->depth,
			STS_XFER_RING_STATE(txs_ring)->read,
			STS_XFER_RING_STATE(txs_ring)->write);

#if defined(STS_XFER_TXS_MBX_PP)
		txs_ring = &(txs->dngl_ring);
		bcm_bprintf(b, "%s Ring: Element Size[%d] depth[%d] rdidx[%d] wridx[%d]\n",
			txs_ring->name, txs_ring->elem_size, txs_ring->depth,
			STS_XFER_RING_STATE(txs_ring)->read,
			STS_XFER_RING_STATE(txs_ring)->write);

		bcm_bprintf(b, "elem_idx_ack[%d] mbx_slot_cur[%d] mbx_slot_alt[%d]\n",
			txs->elem_idx_ack, txs->mbx_slot_cur, txs->mbx_slot_alt);
#elif defined(STS_XFER_TXS_PP)
		bcm_bprintf(b, "elem_idx_ack[%d]\n", txs->elem_idx_ack);
#endif /* STS_XFER_TXS_PP */

#if defined(STS_XFER_STATS)
		bcm_bprintf(b, "Tx Status Stats: processed[%d] prepaged[%d]\n",
			TXS_STATS(txs)->processed, TXS_STATS(txs)->prepaged);
#if defined(STS_XFER_TXS_PP)
		bcm_bprintf(b, "TXS PAGE-IN req_cnt[%d] proc_cnt[%d] unproc_cnt[%d]\n",
			TXS_STATS(txs)->pagein_req_cnt, TXS_STATS(txs)->pagein_proc_cnt,
			TXS_STATS(txs)->pagein_unproc_cnt);
#endif /* STS_XFER_TXS_PP */
#endif /* STS_XFER_STATS */

	}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
	if (STS_XFER_PHYRXS_ENAB(wlc->pub)) {	/* Dump PhyRx Status transfer stats */
		sts_xfer_phyrxs_t	*phyrxs;
		sts_xfer_ring_t		*phyrxs_ring;

		phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);
		phyrxs_ring = &(phyrxs->ring);

		bcm_bprintf(b, "\nPhyRx Status Transfer through - %s\n",
			STS_XFER_PHYRXS_FIFO_ENAB(wlc->pub) ? "RxFIFO-3" : "M2M DMA");

		bcm_bprintf(b, "RxFIFO-0: cons_seqnum[%d] RX_LIST_PEND[%s]\n",
			phyrxs->cons_seqnum[RX_LIST_PEND_FIFO0_IDX],
			RX_LIST_RX_HEAD(RX_LIST_PEND(phyrxs, RX_LIST_PEND_FIFO0_IDX)) != NULL ?
				"TRUE" : "FALSE");
#if defined(BCMSPLITRX)
		bcm_bprintf(b, "RxFIFO-2: cons_seqnum[%d] RX_LIST_PEND[%s]\n",
			phyrxs->cons_seqnum[RX_LIST_PEND_FIFO2_IDX],
			RX_LIST_RX_HEAD(RX_LIST_PEND(phyrxs, RX_LIST_PEND_FIFO2_IDX)) != NULL ?
				"TRUE" : "FALSE");
#endif /* BCMSPLITRX */

		bcm_bprintf(b, "\n%s Ring: Element Size[%d] depth[%d] rdidx[%d] wridx[%d]\n",
			phyrxs_ring->name, phyrxs_ring->elem_size, phyrxs_ring->depth,
			STS_XFER_RING_STATE(phyrxs_ring)->read,
			STS_XFER_RING_STATE(phyrxs_ring)->write);

		/* Dump ring inuse_bitmap */
		{
			int iter;
			bcm_bprintf(b, "Ring inuse_bitmap:");
			for (iter = 0; iter < CEIL(phyrxs_ring->depth, NBBY); iter++) {
				if ((iter % 16) == 0) {
					bcm_bprintf(b, "\n%4d: \t|", (iter * 8));
				}
				bcm_bprintf(b, " %02x", phyrxs->inuse_bitmap[iter]);
			}
		}

		bcm_bprintf(b, "\n");

#if defined(STS_XFER_STATS)
		bcm_bprintf(b, "PhyRx Stats: recv[%d] cons[%d] invalid[%d] release[%d]\n",
			PHYRXS_STATS(phyrxs)->recv, PHYRXS_STATS(phyrxs)->cons,
			PHYRXS_STATS(phyrxs)->invalid, PHYRXS_STATS(phyrxs)->release);
		bcm_bprintf(b, "\tmiss[%d] mpdu_miss[%d]\n", PHYRXS_STATS(phyrxs)->miss,
			PHYRXS_STATS(phyrxs)->mpdu_miss);
		bcm_bprintf(b, "\tcur_sts_access[%d] cur_sts_release[%d]\n",
			PHYRXS_STATS(phyrxs)->cur_sts_access,
			PHYRXS_STATS(phyrxs)->cur_sts_release);
#endif /* STS_XFER_STATS */
	}
#endif /* STS_XFER_PHYRXS */

#if defined(STS_XFER_M2M_INTR)
	{
		sts_xfer_m2m_intr_t *m2m_intr;
		m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);

		bcm_bprintf(b, "\nM2M interrupt:\n");
		bcm_bprintf(b, "defintmask:0x%08x\t intcontrol: 0x%08x\t intstatus: 0x%08x\n",
			m2m_intr->defintmask, m2m_intr->intcontrol, m2m_intr->intstatus);
	}
#endif /* STS_XFER_M2M_INTR */

	bcm_bprintf(b, "\n");

	return BCME_OK;
} /* wlc_sts_xfer_dump() */

/** Clear Status Transfer stats */
static int
wlc_sts_xfer_dump_clr(void *ctx)
{
#if defined(STS_XFER_STATS)
	wlc_sts_xfer_t *sts_xfer = (wlc_sts_xfer_t *)ctx;

#if defined(STS_XFER_TXS)
	if (STS_XFER_TXS_ENAB(sts_xfer->wlc->pub)) { /* Tx Status transfer stats */
		sts_xfer_txs_t	*txs;

		txs = WLC_STS_XFER_TXS(sts_xfer);
		memset((void*)TXS_STATS(txs), 0, sizeof(txs_stats_t));
	}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
	if (STS_XFER_PHYRXS_ENAB(sts_xfer->wlc->pub)) {	/* Phy Rx Status Transfer Stats */
		sts_xfer_phyrxs_t	*phyrxs;

		phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);
		memset((void*)PHYRXS_STATS(phyrxs), 0, sizeof(phyrxs_stats_t));
	}
#endif /* STS_XFER_PHYRXS */

	BCM_REFERENCE(sts_xfer);
#endif /* STS_XFER_STATS */

	return BCME_OK;
} /* wlc_sts_xfer_dump_clr() */

#endif

#if defined(STS_XFER_TXS)
/**
 * ------------------------------------------------------------------------------------------------
 * Section: Tx Status Processing
 *
 * From MAC rev >=130, Tx Status is transferred using a M2MDMA (BME) Ch#3, avoiding SW to
 * explicitly read 16 Tx Status FIFO registers.
 *
 * STS_XFER module maintains a ring (sts_xfer_ring_t) of Tx Status buffers with read and write
 * indexes, programs M2MDMA with attributes of the ring and enables interrupt on WR index
 * update (bit30 in M2MCORE IntControl)
 * On arrival of interrupt on Tx Status WR index update, STS_XFER module will set MI_TFS in
 * D11 MAC SW intstatus (wlc_hw_info:macintstatus) and schedule a DPC. In DPC, WLAN will process
 * the Tx Status from Circular ring and upate the read index.
 *
 *
 * STS_XFER_TXS_PP:
 * In full dongle, Tx Status transfer module is integrated with PKTPGR
 * - TX Status handling with PKTPGR is a two pass algorithm
 *   1. Tx Status Inspection: On arrival on Tx Status A PAGE-IN Tx Status command is issued for
 *	consumed MPDU's identified from Tx Status record.
 *   2. Tx Status Processing: A PAGE-IN response is sent to SW upon construction of packet list
 *	in dongle memory. WLAN will process the corresponding Tx Status in PAGE-In response
 *	callback.
 *
 * STS_XFER_TXS_MBX_PP:
 * - With Mailbox service (HNDMBX), Tx Status Ring is placed in Host memory using HME service,
 *   and on-demand Tx Status is transferred to dongle ping-pong buffers (Max 2).
 * - But with PKTPGR, TxStatus is required at 2 stages (Inspection and Processing) so just
 *   two dongle buffers are insufficient so maintaining a 16 entry deep Tx Status ring in
 *   dongle and upon completion of the PKTPGR inspection stage we copy Tx Status from
 *   Mailbox buffer to Dongle Tx Status Ring.
 * - In Tx Status PAGE-IN response, WLAN driver will process Tx Statuses from Dongle Ring.
 * ------------------------------------------------------------------------------------------------
 */

/** __txs_ring_write_update: Updates the SW write index */
static INLINE void __txs_ring_write_update(wlc_sts_xfer_t *sts_xfer,
	sts_xfer_txs_t *txs, sts_xfer_ring_t *txs_ring);

/** Reclaim Tx Statuses from Circular ring */
void
wlc_sts_xfer_txs_reclaim(wlc_info_t *wlc, bool reinit)
{
	volatile m2m_status_eng_regs_t *status_eng_regs;
	wlc_sts_xfer_t	*sts_xfer;
	sts_xfer_txs_t	*txs;
	sts_xfer_ring_t	*txs_ring;
	uint32	v32, loop_count;
	uint32	busy, eng_state, status_count;
	uint32	saved_core_id;
	uint32	saved_intr_val;
	bool	fatal = FALSE;
	bool	status_stall = FALSE;
	bool	txs_ring_full = FALSE;

	WL_TRACE(("wl%d: %s: ENTER reinit[%d]\n", wlc->pub->unit, __FUNCTION__, reinit));

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	txs = WLC_STS_XFER_TXS(sts_xfer);
	txs_ring = &(txs->ring);
	status_eng_regs = txs->txs_regs;

	if (!sts_xfer->inited) {
		WL_ERROR(("wl%d: %s: STS_XFER Module is Uninitialized \n",
			wlc->pub->unit, __FUNCTION__));
		return;
	}

	STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

txs_dma_pend:
	loop_count = 0;

	/* Poll Tx Status debug_reg to make sure no Tx Status DMA in progress. */
	do {
		v32 = R_REG(wlc->osh, &status_eng_regs->debug);
		eng_state = BCM_GBF(v32, M2M_STATUS_ENG_DEBUG_STATE);
		status_count = BCM_GBF(v32, M2M_STATUS_ENG_DEBUG_STATUS_COUNT);

		busy = (eng_state > 1) | (status_count != 0);
		WL_TRACE(("wl%d: %s Polling Tx Status debug reg<%u:%u>\n", wlc->pub->unit,
			__FUNCTION__, eng_state, status_count));

		__txs_ring_write_update(sts_xfer, txs, txs_ring);

		if (bcm_ring_is_full(STS_XFER_RING_STATE(txs_ring), txs_ring->depth)) {
			txs_ring_full = TRUE;
			break;
		}

		OSL_DELAY(1);
	} while (busy && (++loop_count != STS_XFER_WAIT_LOOP_COUNT));

	if (loop_count == STS_XFER_WAIT_LOOP_COUNT) {
		WL_ERROR(("wl%d: %s Tx Status debug reg is not idle <%u:%u>\n", wlc->pub->unit,
			__FUNCTION__, eng_state, status_count));

		wlc_sts_xfer_txs_error_dump(wlc, NULL, TRUE);

		/* WAR to process stall Tx Statuses.
		 * Disable M2M Tx Status Engine, process pending Tx Statuses in ring
		 * and reinit M2M Tx Status Engine.
		 */
		STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);
		_sts_xfer_m2m_status_eng_deinit(sts_xfer, &txs->ring, txs->txs_regs);
		STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

		/* Disable Req-Ack based MAC_HWA i/f for Tx Status. */
		HWA_UPD_REG16_ADDR(STS_XFER, D11_HWA_MACIF_CTL(wlc), 0, _HWA_MACIF_CTL_TXSTATUSEN);

		status_stall = TRUE;
	}

	STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

#if defined(STS_XFER_TXS_PP)
	{
		/**
		 *  If DDBM resource is not enought to consume all Tx Status.
		 *  1. Do PageIn for some of Tx Statuses
		 *  2. Wait for PageIn Tx Status finished.
		 *  3. Continue step 1 if there is unprocessed Tx Statuses.
		 */
		uint32 loop, pgi_txs_pending;
		hwa_dev_t *hwadev;
		hwa_pktpgr_t *pktpgr;

		/* Setup locals */
		loop = STS_XFER_TXS_CONSUME_POLLLOOP;
		pgi_txs_pending = 0;
		hwadev = WL_HWA_DEVP(wlc);
		pktpgr = &hwadev->pktpgr;

		// Reset it.
		PGIEMERRESETTXSRECLAIM(pktpgr);

		// Drain DBM in TxCpl(pciedev related).
		hwa_pktpgr_txcpl_sync(hwadev);

		// Process pending PGI txstatus resp to drain more DBM.
		if (pktpgr->pgi_txs_req_tot) {
			hwa_pktpgr_txstatus_wait_to_finish(hwadev, -1);
			hwa_pktpgr_txcpl_sync(hwadev);
		}

		while (wlc_sts_xfer_txs_process(wlc, FALSE, &fatal)) {
			/* Use SHARED DBM(ddbm_avail_sw) first. */
			if (pktpgr->pgi_txs_req_tot == 0) {
				/* SHARED DBM is not enough, use RESV DBM. */
				PGIEMERSETTXSRECLAIM(pktpgr);
				WL_ERROR(("wl%d: %s using RESV DBM\n",
					wlc->pub->unit, __FUNCTION__));
			} else {
				/* Use SHARED DBM(ddbm_avail_sw) to consume it piece by piece. */
				/* In above hwa_txstat_process, all 4A-TxS are convert to PageIn TxS
				 * req in PageIn Req Ring, wait for all TxS Rsp process finished.
				 */
				loop = HWA_TXS_CONSUME_POLLLOOP;

				PGIEMERRESETTXSRECLAIM(pktpgr);

				hwa_pktpgr_txcpl_sync(hwadev);
				hwa_pktpgr_txstatus_wait_to_finish(hwadev, -1);

				// Check pgi_txs_req_tot to detect dead loop due to PGI stuck.
				if (pktpgr->pgi_txs_req_tot != 0) {
					pgi_txs_pending++;
				} else {
					pgi_txs_pending = 0;
				}
				ASSERT(pgi_txs_pending < STS_XFER_TXS_CONSUME_POLLLOOP);
			}

			loop--;
			if (loop == 0) {
				WL_ERROR(("wl%d: %s part of Tx Status not consumed "
					"HAMMERING fatal txs err\n",
					wlc->pub->unit, __FUNCTION__));
				status_stall = TRUE;
#if defined(BCMDBG) || defined(HWA_DUMP)
				hwa_dump(hwadev, NULL, HWA_DUMP_PP,
					TRUE, TRUE, FALSE, NULL);
#endif
				break;
			}
		}

		/* In above hwa_txstat_process, all 4A-TxS are convert to PageIn TxS req in
		 * PageIn Req Ring, wait for all TxS Rsp process finished.
		 */
		hwa_pktpgr_txcpl_sync(hwadev);
		hwa_pktpgr_txstatus_wait_to_finish(hwadev, -1);

		if (txs->txs_avail) {
			/* Process pending Tx Statuses */
			wlc_bmac_txstatus(wlc->hw, FALSE, &fatal);
		}

#if defined(STS_XFER_TXS_MBX_PP)
		ASSERT(txs->mbx_message_cur == NULL);
		ASSERT(txs->mbx_message_alt == NULL);
#endif /* STS_XFER_TXS_MBX_PP */
	}
#else  /* ! STS_XFER_TXS_PP */

	/* Process pending Tx Statuses */
	wlc_sts_xfer_txs_process(wlc, FALSE, &fatal);
#endif /* ! STS_XFER_TXS_PP */

	if (!fatal && txs_ring_full) {
		txs_ring_full = FALSE;
		/* TxStatus DMA in progress; Wait for DMA done and process new TxStatuses */
		goto txs_dma_pend;
	}

	ASSERT((txs->txs_avail == 0) || fatal);

	if (status_stall) {
		wlc_check_assert_type(wlc, WL_REINIT_RC_INV_TX_STATUS);
	}
} /* wlc_sts_xfer_txs_reclaim() */

/** __txs_ring_write_update: Updates the SW write index */
static INLINE void BCMFASTPATH
__txs_ring_write_update(wlc_sts_xfer_t *sts_xfer, sts_xfer_txs_t *txs, sts_xfer_ring_t *txs_ring)
{
	volatile m2m_status_eng_regs_t *status_eng_regs;
	uint32	saved_core_id;
	uint32	saved_intr_val;
	uint32	v32; /* used in REG read/write */
	uint16	wridx;

	status_eng_regs = txs->txs_regs;

	STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);
	v32 = R_REG(sts_xfer->wlc->osh, &status_eng_regs->wridx);
	STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

	wridx = BCM_GBF(v32, M2M_STATUS_ENG_WRIDX_QUEUE_WRIDX);

#if !defined(STS_XFER_TXS_MBX_PP)
#if !defined(OSL_CACHE_COHERENT)
	/* Sync Tx Status buffers if host is not coherent */
	{
		dmaaddr_t sync_addr;
		uint32 sync_size;
		uint32 last_wridx;

		last_wridx = STS_XFER_RING_STATE(txs_ring)->write;

		if (last_wridx == wridx) {
			/* No new status received */
			return;
		}

		/* Check for roll-over */
		if (last_wridx > wridx) {
			sync_size = ((txs_ring->depth - last_wridx) * txs_ring->elem_size);

			PHYSADDRLOSET(sync_addr, (PHYSADDRLO(txs_ring->memory_pa) +
					(last_wridx * txs_ring->elem_size)));
			PHYSADDRHISET(sync_addr, PHYSADDRHI(txs_ring->memory_pa));

			DMA_SYNC(sts_xfer->wlc->osh, sync_addr, sync_size, DMA_RX);

			last_wridx = 0; /* Overwriting last_wridx */
		}

		sync_size = (wridx - last_wridx) * txs_ring->elem_size;

		PHYSADDRLOSET(sync_addr, (PHYSADDRLO(txs_ring->memory_pa) +
				(last_wridx * txs_ring->elem_size)));
		PHYSADDRHISET(sync_addr, PHYSADDRHI(txs_ring->memory_pa));

		DMA_SYNC(sts_xfer->wlc->osh, sync_addr, sync_size, DMA_RX);
	}
#endif /* ! OSL_CACHE_COHERENT */
#endif /* ! STS_XFER_TXS_MBX_PP */

	WL_INFORM(("%s: wridx[%d]\n", __FUNCTION__, wridx));
	STS_XFER_RING_STATE(txs_ring)->write = wridx;

} /* __txs_ring_write_update() */

/* Dump Tx Status error logs.
 * CAUTION: Input status buffer might not be a Tx Status ring buffer
 * or Mailbox serive ping-pong buffer.
 */
void
wlc_sts_xfer_txs_error_dump(wlc_info_t *wlc, void *status, bool reg_dump)
{
	wlc_sts_xfer_t	*sts_xfer;
	sts_xfer_txs_t	*txs;
	sts_xfer_ring_t	*txs_ring;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	txs = WLC_STS_XFER_TXS(sts_xfer);
	txs_ring = &(txs->ring);

	WL_PRINT(("\ntxs_avail[%d]\n", txs->txs_avail));
	WL_PRINT(("%s Ring: Element Size[%d] depth[%d] rdidx[%d] wridx[%d]\n",
		txs_ring->name, txs_ring->elem_size, txs_ring->depth,
		STS_XFER_RING_STATE(txs_ring)->read,
		STS_XFER_RING_STATE(txs_ring)->write));

#if defined(STS_XFER_TXS_MBX_PP)
	txs_ring = &(txs->dngl_ring);
	WL_PRINT(("%s Ring: Element Size[%d] depth[%d] rdidx[%d] wridx[%d]\n",
		txs_ring->name, txs_ring->elem_size, txs_ring->depth,
		STS_XFER_RING_STATE(txs_ring)->read,
		STS_XFER_RING_STATE(txs_ring)->write));

	WL_PRINT(("elem_idx_ack[%d] mbx_slot_cur[%d] mbx_slot_alt[%d]\n",
		txs->elem_idx_ack, txs->mbx_slot_cur, txs->mbx_slot_alt));
#elif defined(STS_XFER_TXS_PP)
	WL_PRINT(("elem_idx_ack[%d]\n", txs->elem_idx_ack));
#endif /* STS_XFER_TXS_PP */

#if defined(STS_XFER_STATS)
	WL_PRINT(("Tx Status Stats: processed[%d] prepaged[%d]\n",
		TXS_STATS(txs)->processed, TXS_STATS(txs)->prepaged));
#if defined(STS_XFER_TXS_PP)
	WL_PRINT(("TXS PAGE-IN req_cnt[%d] proc_cnt[%d] unproc_cnt[%d]\n",
		TXS_STATS(txs)->pagein_req_cnt, TXS_STATS(txs)->pagein_proc_cnt,
		TXS_STATS(txs)->pagein_unproc_cnt));
#endif /* STS_XFER_TXS_PP */
#endif /* STS_XFER_STATS */

#if defined(BCMDBG)
	if (reg_dump) {
		_sts_xfer_reg_dump(wlc, TRUE);
	}
#endif

} /* wlc_sts_xfer_txs_error_dump() */

#if defined(STS_XFER_TXS_PP)
/*
 * ------------------------------------------------------------------------------------------------
 * wlc_sts_xfer_txs_process():
 * - Update Tx Status ring SW write index
 * - Initiate and sync a page-in transfer of Current Tx Status from host to dongle ping (CUR)
 *   buffer and next Tx Status to dongle pong (ALT) buffer using Mailbox Servie.
 * - Issue a TX Status PAGE-IN request command and copy the Tx Status from Mailbox buffer (CUR)
 *   to Dongle Tx Status Ring.
 *
 * Return: TRUE if need WLAN worlet reschedule.
 * ------------------------------------------------------------------------------------------------
 */
bool BCMFASTPATH
wlc_sts_xfer_txs_process(wlc_info_t *wlc, bool bound, bool *fatal)
{
	wlc_sts_xfer_t	*sts_xfer;
	sts_xfer_txs_t	*txs;
	sts_xfer_ring_t	*txs_ring;
	void		*txs40_status;
#if defined(STS_XFER_TXS_MBX_PP)
	sts_xfer_ring_t	*dngl_ring;
	uint16	dngl_wridx;
#endif /* STS_XFER_TXS_MBX_PP */
	uint	budget;
	uint16	elem_idx;
	uint16	host_txs_avail;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	txs = WLC_STS_XFER_TXS(sts_xfer);
	txs_ring = &(txs->ring);
#if defined(STS_XFER_TXS_MBX_PP)
	dngl_ring = &(txs->dngl_ring);
	dngl_wridx = STS_XFER_RING_STATE(dngl_ring)->write;
#endif /* STS_XFER_TXS_MBX_PP */

	WL_TRACE(("wl%d: %s: ENTER bound[%d]\n", wlc->pub->unit, __FUNCTION__, bound));

	/* Update write once */
	__txs_ring_write_update(sts_xfer, txs, txs_ring);

	/* Tx Status is made of 2 16B packages so minimum 2 ring buffers should be available */
	host_txs_avail = bcm_ring_cons_avail(STS_XFER_RING_STATE(txs_ring),
			txs_ring->depth) / WLC_TXS40_STATUS_PKGS;

	if (host_txs_avail == 0) {
		goto txs_process_done;
	}

#if defined(STS_XFER_TXS_MBX_PP)
	/* Get available buffers in Dongle Tx Status Ring */
	budget = bcm_ring_prod_avail(STS_XFER_RING_STATE(dngl_ring),
			dngl_ring->depth) / WLC_TXS40_STATUS_PKGS;

	/* Max Tx Status units to be processed */
	budget = MIN(budget, host_txs_avail);
#else /* STS_XFER_TXS_PP */
	/* Max Tx Status units to be processed */
	budget = host_txs_avail;
#endif /* STS_XFER_TXS_PP */

	if (bound) {
		/* !give others some time to run! */
		budget = MIN(budget, wlc->pub->tunables->txsbnd);
	}

	elem_idx = STS_XFER_RING_STATE(txs_ring)->read;

	while (budget) {
		/* Two Status buffers should be contiguous */
		ASSERT(elem_idx < STS_XFER_RING_NEXT_IDX(txs_ring, elem_idx));

#if defined(STS_XFER_TXS_MBX_PP)
		/* Check if Tx Status is available in MBX_SLOT_CUR */
		if (txs->mbx_message_cur != NULL) {
			ASSERT(txs->mbx_slot_cur != STS_XFER_MBX_SLOT_INVALID);

		/* Check if Tx Status is pre-paged to MBX_SLOT_ALT */
		} else if (mbx_page(MBX_USR_MACTXS, MBX_SLOT_CUR) !=
			STS_XFER_TXS_RINGIDX_2_STATUSIDX(elem_idx)) {

			/* No pre-paged Tx Status available */
			ASSERT(txs->mbx_slot_alt == STS_XFER_MBX_SLOT_INVALID);
			ASSERT(txs->mbx_message_alt == NULL);

			/* Initiate a page-in transfer of Tx Status into MBX_SLOT_CUR */
			txs->mbx_slot_cur = mbx_xfer(MBX_USR_MACTXS,
				STS_XFER_TXS_RINGIDX_2_STATUSIDX(elem_idx), MBX_SLOT_CUR);

			/* Sync ongoing DMA and get paged-in message */
			txs->mbx_message_cur = mbx_sync(MBX_USR_MACTXS, MBX_SLOT_CUR);

		} else { /* Use Tx Status pre-paged into ALT slot */

			ASSERT(txs->mbx_message_cur == NULL);
			ASSERT(txs->mbx_slot_cur == STS_XFER_MBX_SLOT_INVALID);

			/* Sync ongoing DMA */
			txs->mbx_message_cur = mbx_sync(MBX_USR_MACTXS, MBX_SLOT_CUR);
			txs->mbx_slot_cur = txs->mbx_slot_alt;
			STS_XFER_STATS_INCR(TXS_STATS(txs), prepaged);

			ASSERT(txs->mbx_message_cur == txs->mbx_message_alt);
			txs->mbx_message_alt = NULL;
			txs->mbx_slot_alt = STS_XFER_MBX_SLOT_INVALID;
		}

		txs40_status = txs->mbx_message_cur;
		host_txs_avail--;

		/* If next Tx Status is available then prefetch it to ALT slot */
		if (host_txs_avail && (txs->mbx_slot_alt == STS_XFER_MBX_SLOT_INVALID)) {
			uint16 next_elem_idx = STS_XFER_RING_ADD_IDX(txs_ring, elem_idx,
				WLC_TXS40_STATUS_PKGS);
			txs->mbx_slot_alt = mbx_xfer(MBX_USR_MACTXS,
				STS_XFER_TXS_RINGIDX_2_STATUSIDX(next_elem_idx), MBX_SLOT_ALT);
		}

#else /* STS_XFER_TXS_PP */
		/* Get Tx Status from circular ring located in SysMem */
		txs40_status = STS_XFER_RING_ELEM(wlc_txs_pkg16_t, txs_ring, elem_idx);
		ASSERT(txs40_status != NULL);

		host_txs_avail--;
#endif /* STS_XFER_TXS_PP */

		/* Process Tx Status ncons, fifo for HWA_PP_PAGEIN_TXSTATUS */
		if (hwa_txstat_pagein_req(WL_HWA_DEVP(wlc), txs40_status,
			STS_XFER_TXS_RINGIDX_2_STATUSIDX(elem_idx)) == BCME_NORESOURCE) {

			/* pagein_req_ring full; reuse status from MBX_SLOT_CUR in next iter. */
			WL_INFORM(("%s: pagein_req_ring full\n", __FUNCTION__));
			host_txs_avail++;
			break;
		}

		STS_XFER_STATS_INCR(TXS_STATS(txs), pagein_req_cnt);

#if defined(STS_XFER_TXS_MBX_PP)
		/* Copy Status from Mailbox buffer to Dongle Tx Status ring */
		{
			void * dngl_ring_elem;

			dngl_ring_elem = STS_XFER_RING_ELEM(wlc_txs_pkg16_t, dngl_ring, dngl_wridx);

			memcpy(dngl_ring_elem, txs40_status, WLC_TXS40_STATUS_BYTES);
			/* Advance next index by two elements */
			dngl_wridx = STS_XFER_RING_ADD_IDX(dngl_ring,
					dngl_wridx, WLC_TXS40_STATUS_PKGS);
		}

		/* Free Mailbox current buffer */
		txs->mbx_slot_cur = STS_XFER_MBX_SLOT_INVALID;
		txs->mbx_message_cur = NULL;

		/* Iterate to next slot, making ALT as the CUR. */
		txs->mbx_message_alt = mbx_iter(MBX_USR_MACTXS);
		/* mbx_message_alt is NULL if Tx Status in not available. */
		if (txs->mbx_message_alt == NULL) {
			txs->mbx_slot_alt = STS_XFER_MBX_SLOT_INVALID;
		}
#endif /* STS_XFER_TXS_MBX_PP */

		/* Advance next index by two elements */
		elem_idx = STS_XFER_RING_ADD_IDX(txs_ring, elem_idx, WLC_TXS40_STATUS_PKGS);
		budget--;
	}

#if defined(STS_XFER_TXS_MBX_PP)
	/* Update wridx of dngl_ring */
	STS_XFER_RING_STATE(dngl_ring)->write = dngl_wridx;
#endif /* STS_XFER_TXS_MBX_PP */

	/* Commit Tx Status Host ring SW read index */
	STS_XFER_RING_STATE(txs_ring)->read = elem_idx;

	/* SW rdidx is committed to HW register after processing Tx Statuses */

txs_process_done:
	if (txs->txs_avail) {
		/* Process pre-paged Tx Statuses if any */
		wlc_bmac_txstatus(wlc->hw, bound, fatal);
	}

	/* Need reschedule ? */
	return (host_txs_avail != 0);
} /* wlc_sts_xfer_txs_process() */

/*
 * ------------------------------------------------------------------------------------------------
 * wlc_sts_xfer_txs_pagein_process(): Tx Status PAGE-In response callback
 * - Update Write index of Dongle Tx Status Ring and process Tx Statuses
 * ------------------------------------------------------------------------------------------------
 */
void BCMFASTPATH
wlc_sts_xfer_txs_pagein_process(wlc_info_t *wlc)
{
	bool		fatal = FALSE;
	hwa_pktpgr_t	*pktpgr;
	sts_xfer_txs_t	*txs;

	pktpgr = &(WL_HWA_DEVP(wlc)->pktpgr);
	txs = WLC_STS_XFER_TXS(WLC_STS_XFER(wlc->sts_xfer_info));
	/* Update Tx Statuses avialable in dongle to be consumed */
	txs->txs_avail++;
	STS_XFER_STATS_INCR(TXS_STATS(txs), pagein_proc_cnt);

	if (pktpgr->pgi_txs_unprocessed && (pktpgr->pgi_txs_req_tot == 0)) {
		WL_INFORM(("%s: TXS: process all unprocessed Tx Statuses\n", __FUNCTION__));
		txs->txs_avail += pktpgr->pgi_txs_unprocessed;
		STS_XFER_STATS_ADD(TXS_STATS(txs), pagein_unproc_cnt, pktpgr->pgi_txs_unprocessed);
		pktpgr->pgi_txs_unprocessed = 0;
	}

	/* Process Tx Statuses from dongle ring */
	wlc_bmac_txstatus(wlc->hw, TRUE, &fatal);

	if (fatal) {
		WL_ERROR(("wl%d: %s HAMMERING fatal txs err\n", wlc->pub->unit, __FUNCTION__));
		wlc_sts_xfer_txs_error_dump(wlc, NULL, TRUE);
		wlc_check_assert_type(wlc, WL_REINIT_RC_INV_TX_STATUS);
	}
} /* wlc_sts_xfer_txs_pagein_process() */

#else /* ! STS_XFER_TXS_PP */

/* Update SW write index and process all Tx Statuses from Circular Ring */
bool BCMFASTPATH
wlc_sts_xfer_txs_process(wlc_info_t *wlc, bool bound, bool *fatal)
{
	wlc_sts_xfer_t	*sts_xfer;
	sts_xfer_txs_t	*txs;
	sts_xfer_ring_t	*txs_ring;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	txs = WLC_STS_XFER_TXS(sts_xfer);
	txs_ring = &(txs->ring);

	WL_TRACE(("wl%d: %s: ENTER bound[%d]\n", wlc->pub->unit, __FUNCTION__, bound));

	/* Update write once */
	__txs_ring_write_update(sts_xfer, txs, txs_ring);	// Update write once
	txs->txs_avail = bcm_ring_cons_avail(STS_XFER_RING_STATE(txs_ring),
			txs_ring->depth) / WLC_TXS40_STATUS_PKGS;

	if (txs->txs_avail) {
		/* Process Tx Statuses from Ring */
		return wlc_bmac_txstatus(wlc->hw, bound, fatal);
	}

	/* Tx Status is not available */
	return FALSE;
} /* wlc_sts_xfer_txs_process() */

#endif /* ! STS_XFER_TXS_PP */

/* Get Tx Status from Circular Ring. */
void * BCMFASTPATH
wlc_sts_xfer_txs_get_status(wlc_info_t *wlc)
{
	wlc_sts_xfer_t	*sts_xfer;
	sts_xfer_txs_t	*txs;
	sts_xfer_ring_t	*txs_ring;
	void		*txs40_status;
	uint16		rdidx;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	txs = WLC_STS_XFER_TXS(sts_xfer);

	WL_TRACE(("wl%d: %s: ENTER \n", wlc->pub->unit, __FUNCTION__));

#if defined(STS_XFER_TXS_MBX_PP)
	/* Get Status from Dongle ring */
	txs_ring = &(txs->dngl_ring);

#else /* ! STS_XFER_TXS_MBX_PP */
	/* Get Status from Tx Status ring */
	txs_ring = &(txs->ring);
#endif /* ! STS_XFER_TXS_MBX_PP */

	if (txs->txs_avail == 0) {
		/* Tx Status is not available */
		return NULL;
	}

#if defined(STS_XFER_TXS_MBX_PP)
	/* XXX FIXME
	 * For txs->status.is_intermediate, Tx Status packets may have prepaged-in to SW shadow
	 * and TXS PAGE-IN response callback is invoked but corresponding Tx Status is not yet
	 * transferred (MBX) to dongle i.e., Dongle ring is empty but txs_avail is non-zero.
	 */
	if (bcm_ring_is_empty(STS_XFER_RING_STATE(txs_ring))) {
		WL_INFORM(("%s: txs_avail[%d] but Tx Status is paged to dongle ye\n",
			__FUNCTION__, txs->txs_avail));
		/* Tx Status is not available */
		return NULL;
	}
#endif /* STS_XFER_TXS_MBX_PP */

#if defined(STS_XFER_TXS_PP) && !defined(STS_XFER_TXS_MBX_PP)
	rdidx = txs->elem_idx_ack;
#else
	rdidx = STS_XFER_RING_STATE(txs_ring)->read;
#endif /* ! STS_XFER_TXS_MBX_PP */

	/* Get Tx Status from circular ring */
	txs40_status = STS_XFER_RING_ELEM(wlc_txs_pkg16_t, txs_ring, rdidx);
	ASSERT(txs40_status != NULL);

	STS_XFER_STATS_INCR(TXS_STATS(txs), processed);

	WL_INFORM(("wl%d: %s: Tx Status Ring rdidx[%d] wridx[%d] txs40_status[%p]\n",
		wlc->pub->unit, __FUNCTION__, STS_XFER_RING_STATE(txs_ring)->read,
		STS_XFER_RING_STATE(txs_ring)->write, txs40_status));

#if !defined(STS_XFER_TXS_PP) || defined(STS_XFER_TXS_MBX_PP)
	/** XXX: For PKTPGR only builds, SW read index is already updated in
	 * wlc_sts_xfer_txs_process().
	 */
	/* Advance read index by two elements */
	rdidx = STS_XFER_RING_ADD_IDX(txs_ring, rdidx, WLC_TXS40_STATUS_PKGS);
	STS_XFER_RING_STATE(txs_ring)->read = rdidx;
#endif /* !STS_XFER_TXS_PP || STS_XFER_TXS_MBX_PP */

#if defined(STS_XFER_TXS_PP)
	/* Update last consumed Host ring index; Used to update HW Tx Status read register */
	txs->elem_idx_ack = STS_XFER_RING_ADD_IDX(&txs->ring,
		txs->elem_idx_ack, WLC_TXS40_STATUS_PKGS);
#endif /* STS_XFER_TXS_PP */

	txs->txs_avail--;

	/* SW rdidx is committed to HW register after processing all(bound) pending Tx Statuses */

	return txs40_status;
} /* wlc_sts_xfer_txs_get_status() */

/* Commit Tx Status ring SW read to M2MDMA HW register
 * Returns TRUE if Tx Statuses are pending.
 */
bool BCMFASTPATH
wlc_sts_xfer_txs_bmac_txstatus_done(wlc_info_t *wlc)
{
	volatile m2m_status_eng_regs_t *status_eng_regs;
	wlc_sts_xfer_t	*sts_xfer;
	sts_xfer_txs_t	*txs;
	uint32	saved_core_id;
	uint32	saved_intr_val;
	uint32	v32; /* used in REG read/write */
	uint16	rdidx;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	txs = WLC_STS_XFER_TXS(sts_xfer);

	status_eng_regs = txs->txs_regs;

	/* Get ring index of last acked Tx Status (Read index) */
#if defined(STS_XFER_TXS_PP)
	rdidx = txs->elem_idx_ack;
#else /* ! STS_XFER_TXS_PP */
	rdidx = STS_XFER_RING_STATE(&txs->ring)->read;
#endif /* ! STS_XFER_TXS_PP */

	STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

	/* Update M2M status engine read index */
	v32 = R_REG(wlc->osh, &status_eng_regs->rdidx);
	v32 = BCM_CBF(v32, M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX);
	v32 |= BCM_SBF(rdidx, M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX);
	W_REG(wlc->osh, &status_eng_regs->rdidx, v32);

	STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

	return (txs->txs_avail != 0);

} /* wlc_sts_xfer_txs_bmac_txstatus_done() */

#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)
/**
 * ------------------------------------------------------------------------------------------------
 * Section: PhyRx Status Processing
 *
 * From MAC rev >= 129, PhyRx Status is sent asynchronously to host (WLAN driver*) independent
 * of Ucode and HW status. Because of the asynchronous nature of PHY status, PHY Status and
 * Ucode status for the MPDU's are "linked" together with a 12bit - sequence number.
 * 12 bit sequence number is maintained by Ucode and is updated if at least one Ucode status was
 * sent out for the current PPDU.
 * The sequence number is indicated in ucode status of MPDU's (d11rxhdr_ge129_t::RxStatus3) and in
 * PhyRx Status (d11phyrxsts_t::seq)
 * Note: MAC will post a dummy PhyRx Status in case the PhyStatus had not arrived from PHY to MAC
 * rather than simply dropping (not posting) with skipped seqnum.
 *
 * - MAC rev = 129: STS_XFER_PHYRXS_FIFO_ENAB()
 *   The Phy Rx status goes out on D11 Rx FIFO-3 (STS_FIFO)
 * - MAC rev >= 130: STS_XFER_PHYRXS_M2M_ENAB()
 *   BME channel# 3 (M2M DMA) is used to transfer Phy Rx Status from MAC to HOST.
 *
 * Note: XXX PhyRx Status processing of uplink multi-user ofdma traffic is not handled yet.
 *
 * STS_XFER module maintains a ring (sts_xfer_ring_t) of PhyRx Status buffers with read and write
 * indexes and programs DMA decriptors/M2M DMA with attributes of the ring.
 * On arrival of Rx packets, one MPDU of an AMPDU (need not to be last) is linked with PhyRx Status
 * buffer and sent up for WLAN Rx Processing. PhyRx Status buffers are released after processing
 * Phy Rx Status or on PKTFREE.
 * WLAN driver will not hold any PhyRx Status buffers. In case of AMPDU reordering, PhyRx Status
 * buffer will be de-linked with Rx packet and only Rx packet will held in AMPDU Rx Queue.
 *
 * A callback (wlc_sts_xfer_phyrxs_free()) is registered with OSL layer to release a PhyRx Status
 * buffer on PKTFREE of Rx packet.
 *
 * Implementation Notes:
 * - Lower N bits of PhyRx Status seqnumber is used as index to PhyRx Status buffer in the ring,
 *   where 2^N is the depth of the ring.
 * - PhyRx Status seqnum is encoded into wlc_pkttag_t::phyrxs_seqid to fetch PhyRx Status ring
 *   element and release in PKTFREE.
 * - WLAN subsystem will process one Rx packet at a time and if a Rx packet has a valid PhyRx Status
 *   then WLAN will set the hndd11_phyrxs_cur_status[] to the corresponding PhyRx Status buffer.
 * - HNDD11 PhyRx Status MACROs uses hndd11_phyrxs_cur_status to access PhyRx Status elements.
 *
 * Mailbox Pager service (HNDMBX):
 * If HNDMBX is enabled, then PhyRx Status cirular ring is placed in Host DDR using HME service.
 * And before WLAN Rx processing (e.g., wlc_recv()), PhyRx Status linked to the Rx packet is
 * transferred from host memory to dongle ping-pong buffer using Mailbox pager.
 *
 * TODO: Mailbox page service provides two ping-pong buffers. So while processing current PhyRx
 * Status, look ahead and initiate the page-in transfer of next PhyRx Status into alternate
 * ping-pong buffer.
 * ------------------------------------------------------------------------------------------------
 */

#if defined(BCMDBG) && defined(HNDMBX)
#define STS_XFER_PHYRXS_MISS_DEBUG
#include <sbtopcie.h>
#endif /* BCMDBG && HNDMBX */

#if defined(STS_XFER_PHYRXS_MISS_DEBUG)

#define D11PHYRXSTS_SEQ_SEQNUM(seq)	((((seq) & 0xf000) >> 4) | ((seq) & 0xff))
/** Debug code to dump from PhyRx Status seqnum of all status units from DDR. */
static void
__phyrxs_debug_dump_ring_seqnum(sts_xfer_ring_t *phyrxs_ring)
{
	uint16	d11phyrxsts_seq;
	uint32	haddr4_lo;
	uint64	haddr_u64;
	int idx;

	printf("PhyRx Status Seqnum of all status units from DDR\n");
	printf("idx:\td11phyrxsts::seqnum\n");
	printf("=======================================================================\n");

	for (idx = 0; idx < phyrxs_ring->depth; idx++) {
		haddr4_lo = HADDR64_LO(phyrxs_ring->hme_haddr64);
		haddr4_lo += (idx * phyrxs_ring->elem_size) + OFFSETOF(d11phyrxsts_t, seq);

		haddr_u64 = HADDR64_HI(phyrxs_ring->hme_haddr64);
		haddr_u64 = ((haddr_u64 << (NBITS(uint32))) | haddr4_lo);

		/* Fetch the seqnum from DDR */
		sbtopcie_cpy((uintptr)&d11phyrxsts_seq, haddr_u64, 2, SBTOPCIE_DIR_H2D);

		if ((idx % 16) == 0) {
			printf("\n%02d:\t", idx);
		}
		printf("%04d  ", D11PHYRXSTS_SEQ_SEQNUM(d11phyrxsts_seq));
	}
	printf("\n");
} /* __phyrxs_debug_dump_ring_seqnum() */
#endif /* STS_XFER_PHYRXS_MISS_DEBUG */

/** Dump PhyRx Status error logs */
static void
_phyrxs_error_log_dump(wlc_info_t *wlc, sts_xfer_phyrxs_t *phyrxs,
	sts_xfer_ring_t *phyrxs_ring, bool reg_dump, bool fetch_seqnum)
{
	WL_PRINT(("rxswrst_cnt[%u] prxsfull_cnt[%u]\n",
		wlc_read_shm(wlc, M_RXSWRST_CNT(wlc)),
		wlc_read_shm(wlc, M_PHYRXSFULL_CNT(wlc))));

#if defined(STS_XFER_PHYRXS_FIFO)
	if (STS_XFER_PHYRXS_FIFO_ENAB(wlc->pub)) {
		WL_PRINT(("STS_FIFO: dma_rxactive[%d]\n", dma_rxactive(wlc->hw->di[STS_FIFO])));
	}
#endif /* STS_XFER_PHYRXS_FIFO */

	WL_PRINT(("RxFIFO-0: cons_seqnum[%d]\n", phyrxs->cons_seqnum[RX_LIST_PEND_FIFO0_IDX]));
#if defined(BCMSPLITRX)
	WL_PRINT(("RxFIFO-2: cons_seqnum[%d]\n", phyrxs->cons_seqnum[RX_LIST_PEND_FIFO2_IDX]));
#endif /* BCMSPLITRX */

	WL_PRINT(("Ring: rdidx[%d] wridx[%d] rd_last[%d]\n",
		STS_XFER_RING_STATE(phyrxs_ring)->read, STS_XFER_RING_STATE(phyrxs_ring)->write,
		phyrxs->rd_last));

#if defined(STS_XFER_STATS)
	WL_PRINT(("PhyRx Stats: recv[%d] cons[%d] invalid[%d] release[%d]\n",
		PHYRXS_STATS(phyrxs)->recv, PHYRXS_STATS(phyrxs)->cons,
		PHYRXS_STATS(phyrxs)->invalid, PHYRXS_STATS(phyrxs)->release));
	WL_PRINT(("\tmiss[%d] mpdu_miss[%d]\n", PHYRXS_STATS(phyrxs)->miss,
		PHYRXS_STATS(phyrxs)->mpdu_miss));
#endif /* STS_XFER_STATS */

#if defined(STS_XFER_PHYRXS_M2M) && defined(BCMDBG)
	if (STS_XFER_PHYRXS_M2M_ENAB(wlc->pub) && reg_dump) {
		_sts_xfer_reg_dump(wlc, FALSE);
	}
#endif

#if defined(STS_XFER_PHYRXS_MISS_DEBUG)
	if (fetch_seqnum) {
		__phyrxs_debug_dump_ring_seqnum(phyrxs_ring);
	}
#endif /* STS_XFER_PHYRXS_MISS_DEBUG */

} /* _phyrxs_error_log_dump() */

/** __phyrxs_ring_write_update: Updates the SW write index */
static INLINE void BCMFASTPATH
__phyrxs_ring_write_update(wlc_sts_xfer_t *sts_xfer, sts_xfer_phyrxs_t *phyrxs,
	sts_xfer_ring_t *phyrxs_ring)
{
	wlc_info_t	*wlc;
	uint16 wridx = 0;

	wlc = sts_xfer->wlc;

#if defined(STS_XFER_PHYRXS_FIFO)
	if (STS_XFER_PHYRXS_FIFO_ENAB(wlc->pub)) {

		/* Receive and sync PhyRx Status buffers from STS_FIFO dma */
		wridx = dma_sts_rx(wlc->hw->di[STS_FIFO]);
	}
#endif /* STS_XFER_PHYRXS_FIFO */

#if defined(STS_XFER_PHYRXS_M2M)
	if (STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		volatile m2m_status_eng_regs_t *status_eng_regs;
		uint32	saved_core_id;
		uint32	saved_intr_val;
		uint32	v32; /* used in REG read/write */

		ASSERT(STS_XFER_PHYRXS_M2M_ENAB(wlc->pub));
		status_eng_regs = phyrxs->phyrxs_regs;

		STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);
		v32 = R_REG(wlc->osh, &status_eng_regs->wridx);
		STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

		wridx = BCM_GBF(v32, M2M_STATUS_ENG_WRIDX_QUEUE_WRIDX);
	}
#endif /* STS_XFER_PHYRXS_M2M */

#if !defined(STS_XFER_PHYRXS_MBX)
#if !defined(OSL_CACHE_COHERENT)
	/* Sync PhyRx Status buffers if host is not coherent */
	{
		dmaaddr_t sync_addr;
		uint32 sync_size;
		uint32 last_wridx;

		last_wridx = STS_XFER_RING_STATE(phyrxs_ring)->write;

		if (last_wridx == wridx) {
			/* No new status received */
			return;
		}

		/* Check for roll-over */
		if (last_wridx > wridx) {
			sync_size = ((phyrxs_ring->depth - last_wridx) * phyrxs_ring->elem_size);

			PHYSADDRLOSET(sync_addr, (PHYSADDRLO(phyrxs_ring->memory_pa) +
					(last_wridx * phyrxs_ring->elem_size)));
			PHYSADDRHISET(sync_addr, PHYSADDRHI(phyrxs_ring->memory_pa));

			DMA_SYNC(wlc->osh, sync_addr, sync_size, DMA_RX);

			last_wridx = 0; /* Overwriting last_wridx */
		}

		sync_size = (wridx - last_wridx) * phyrxs_ring->elem_size;

		PHYSADDRLOSET(sync_addr, (PHYSADDRLO(phyrxs_ring->memory_pa) +
				(last_wridx * phyrxs_ring->elem_size)));
		PHYSADDRHISET(sync_addr, PHYSADDRHI(phyrxs_ring->memory_pa));

		DMA_SYNC(wlc->osh, sync_addr, sync_size, DMA_RX);
	}
#endif /* ! OSL_CACHE_COHERENT */
#endif /* ! STS_XFER_PHYRXS_MBX */

	WL_INFORM(("wl%d: %s: wridx[%d]\n", wlc->pub->unit, __FUNCTION__, wridx));
	STS_XFER_RING_STATE(phyrxs_ring)->write = wridx;

} /* __phyrxs_ring_write_update() */

/**
 * Reclaim
 * - Rx packets pending in PhyRx Status pending list
 * - Pending PhyRx Status units in circular ring.
 */
void
wlc_sts_xfer_phyrxs_reclaim(wlc_info_t *wlc)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_phyrxs_t	*phyrxs;
	sts_xfer_ring_t		*phyrxs_ring;
	rx_list_t		*rx_list_pend;
	void	*pkt;
	int	idx;
	uint16	wridx;

	WL_TRACE(("wl%d: %s: ENTER \n", wlc->pub->unit, __FUNCTION__));

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);
	phyrxs_ring = &(phyrxs->ring);

	if (!sts_xfer->inited) {
		WL_ERROR(("wl%d: %s: STS_XFER Module is NOT initialized \n",
			wlc->pub->unit, __FUNCTION__));
		return;
	}

	/* Free all packets from rx_list_pend */
	for (idx = 0; idx < RX_LIST_PEND_MAX; idx++) {
		phyrxs->cons_seqnum[idx] = PHYRXS_CONS_SEQNUM_INVALID;

		rx_list_pend = RX_LIST_PEND(phyrxs, idx);
		while (RX_LIST_RX_HEAD(rx_list_pend) != NULL) {
			pkt = RX_LIST_DELETE_HEAD(rx_list_pend);
			PKTFREE(wlc->osh, pkt, FALSE);
		}
	}

	/* Data and Management packets will be reclaimed and dropped in dma_rxreclaim().
	 * Here, Just ignore the pending PhyRx Status units and update HW/SW ring indexes.
	 */

#if defined(STS_XFER_PHYRXS_FIFO)
	if (STS_XFER_PHYRXS_FIFO_ENAB(wlc->pub)) {
		/* STS_FIFO is already reclaimed (dma_sts_rxreclaim());
		 * Update the SW ring indexes
		 */
		wridx = dma_sts_rxin(wlc->hw->di[STS_FIFO]);
		STS_XFER_RING_STATE(phyrxs_ring)->read = wridx;
		STS_XFER_RING_STATE(phyrxs_ring)->write = wridx;

		phyrxs->rd_last = wridx;
	}
#endif /* STS_XFER_PHYRXS_FIFO */

#if defined(STS_XFER_PHYRXS_M2M)
	if (STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		volatile m2m_status_eng_regs_t *status_eng_regs;
		uint32	v32, loop_count;
		uint32	busy, eng_state, status_count;
		uint32	saved_core_id;
		uint32	saved_intr_val;
		bool	phyrxs_ring_full = FALSE;

		status_eng_regs = phyrxs->phyrxs_regs;

		STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

phyrxs_dma_pend:
		loop_count = 0;

		/* Poll PhyRx Status debug_reg to make sure Status transfer is not in progress. */
		do {
			v32 = R_REG(wlc->osh, &status_eng_regs->debug);
			eng_state = BCM_GBF(v32, M2M_STATUS_ENG_DEBUG_STATE);
			status_count = BCM_GBF(v32, M2M_STATUS_ENG_DEBUG_STATUS_COUNT);

			busy = (eng_state > 1) | (status_count != 0);

			__phyrxs_ring_write_update(sts_xfer, phyrxs, phyrxs_ring);

			if (bcm_ring_is_full(
				STS_XFER_RING_STATE(phyrxs_ring), phyrxs_ring->depth)) {
				phyrxs_ring_full = TRUE;
				break;
			}

			OSL_DELAY(1);
		} while (busy && (++loop_count != STS_XFER_WAIT_LOOP_COUNT));

		WL_INFORM(("wl%d: %s Polled PhyRx Status debug reg<%u:%u> for %dus\n",
			wlc->pub->unit, __FUNCTION__, eng_state, status_count, loop_count));

		wridx = STS_XFER_RING_STATE(phyrxs_ring)->write;
		STS_XFER_RING_STATE(phyrxs_ring)->read = wridx;

		phyrxs->rd_last = wridx;

		/* Update M2M status engine read index */
		v32 = R_REG(wlc->osh, &status_eng_regs->rdidx);
		v32 = BCM_CBF(v32, M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX);
		v32 |= BCM_SBF(wridx, M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX);
		W_REG(wlc->osh, &status_eng_regs->rdidx, v32);

		if (phyrxs_ring_full) {
			phyrxs_ring_full = FALSE;
			/* PhyRx Status transfer in progres, Wait to complete */
		       goto phyrxs_dma_pend;
		}

		STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);

		if (loop_count == STS_XFER_WAIT_LOOP_COUNT) {
			WL_ERROR(("wl%d: %s PhyRx Status debug reg is not idle <%u:%u>\n",
				wlc->pub->unit, __FUNCTION__, eng_state, status_count));

			_phyrxs_error_log_dump(wlc, phyrxs, phyrxs_ring, TRUE, FALSE);

			wlc_check_assert_type(wlc, WL_REINIT_RC_RX_DMA_STALL);
		}
	}
#endif /* STS_XFER_PHYRXS_M2M */

} /* wlc_sts_xfer_phyrxs_reclaim() */

/**
 * _phyrxs_seqnum_valid:  Validate a given index to the PhyRx Status buffer.
 * - In Status processing, wridx points to Write index of the Ring.
 * - In Status release, wridx points to the next index of last consumed PhyRx Status buffer.
 */
static bool BCMFASTPATH
_phyrxs_seqnum_valid(uint16 phyrxs_seqnum, uint16 depth,
	uint16 rdidx, uint16 wridx)
{
	uint16 seqidx;
	bool seqnum_valid = FALSE;

	/* Fetch phyrxs->ring lookup index from seqnum and validate index */
	seqidx = phyrxs_seqnum & (depth - 1);
	if (wridx > rdidx) {
		seqnum_valid = ((rdidx <= seqidx) & (seqidx < wridx));
	} else {
		seqnum_valid = ((rdidx <= seqidx) | (seqidx < wridx));
	}
	return seqnum_valid;
} /* _phyrxs_seqnum_valid() */

/** __phyrxs_seqnum_avail: Returns TRUE if PhyRx Status is availabe for a given sequence number */
static INLINE bool BCMFASTPATH
__phyrxs_seqnum_avail(wlc_sts_xfer_t *sts_xfer, sts_xfer_phyrxs_t *phyrxs,
	uint16 phyrxs_seqnum, sts_xfer_ring_t *phyrxs_ring, bool rxh_next_seqnum_avail)
{
	uint16 rdidx, wridx;
	bool seqnum_avail = FALSE;

	if (bcm_ring_is_empty(STS_XFER_RING_STATE(phyrxs_ring))) {
		/* Check if new statuses are received */
		/* Update PhyRx Status ring write index to latest */
		__phyrxs_ring_write_update(sts_xfer, phyrxs, phyrxs_ring);

		if (bcm_ring_is_empty(STS_XFER_RING_STATE(phyrxs_ring))) {

			if (rxh_next_seqnum_avail) {

				uint32 loop_count = 0;

				rdidx = STS_XFER_RING_STATE(phyrxs_ring)->read;
				wridx =	STS_XFER_RING_STATE(phyrxs_ring)->write;

				/* With MBX PhyRx Status ring is offloaded to Host (DDR) memory.
				 * Intermittently, observed a latency in transferring PhyRx Status
				 * over PCIe and reporting no PhyRx Status available.
				 * Poll wridx for 10ms and check for PhyRx Status availablity.
				 */
				while (++loop_count != STS_XFER_WAIT_LOOP_COUNT) {
					__phyrxs_ring_write_update(sts_xfer, phyrxs, phyrxs_ring);

					if (!bcm_ring_is_empty(STS_XFER_RING_STATE(phyrxs_ring))) {
						seqnum_avail = TRUE;
						break;
					}

					OSL_DELAY(1);
				}
			}

			if (seqnum_avail == FALSE) {
				return FALSE;
			}
		}
	}

	/* Check if Status is already consumed */
	if (STS_XFER_RING_GET_INUSE(phyrxs,
		PHYRXS_SEQNUM_2_RINGIDX(phyrxs_ring, phyrxs_seqnum))) {
		return FALSE;
	}

	rdidx = STS_XFER_RING_STATE(phyrxs_ring)->read;
	wridx = STS_XFER_RING_STATE(phyrxs_ring)->write;

	seqnum_avail = _phyrxs_seqnum_valid(phyrxs_seqnum, phyrxs_ring->depth, rdidx, wridx);

	return seqnum_avail;
} /* __phyrxs_seqnum_avail() */

/**
 * __phyrxs_consume_d11phyrxsts:
 * - Validates Seqnumber and links PhyRx Status buffer to the Rx packet.
 */
static bool BCMFASTPATH
__phyrxs_consume_d11phyrxsts(wlc_info_t *wlc, sts_xfer_phyrxs_t *phyrxs,
	sts_xfer_ring_t *phyrxs_ring, void *pkt, uint8 rxh_offset)
{
	d11rxhdr_t	*rxh;
	wlc_d11rxhdr_t	*wrxh;
	uint16		seqidx, rxh_seqnum;
	bool		phyrxs_consumed;

	rxh = (d11rxhdr_t *)((uint8 *)PKTDATA(wlc->osh, pkt) + rxh_offset);

	ASSERT(!IS_D11RXHDRSHORT(rxh, wlc->pub->corerev));

	rxh_seqnum = D11RXHDR_GE129_SEQNUM(rxh);
	phyrxs_consumed = FALSE;

	STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), recv);

	if (bcm_ring_is_empty(STS_XFER_RING_STATE(phyrxs_ring))) {
		WL_ERROR(("wl%d: %s: PhyRx Status ring is empty\n", wlc->pub->unit, __FUNCTION__));
		goto d11phyrxsts_invalid;
	}

	seqidx = PHYRXS_SEQNUM_2_RINGIDX(phyrxs_ring, rxh_seqnum);

	/* If Mailbox service is enabled, Only link PhyRx Status to the Rx Packet.
	 * PhyRx Status is tranferred to Mailbox service dongle ping-pong during WLAN Rx processing
	 */
#if !defined(STS_XFER_PHYRXS_MBX)
	{
		d11phyrxsts_t *d11phyrxsts;

		/** Verify seqnumber in Ucode status with seqnumber in PhyRx Status */
		d11phyrxsts = STS_XFER_RING_ELEM(d11phyrxsts_t, phyrxs_ring, seqidx);
		if ((rxh_seqnum != D11PHYRXSTS_SEQNUM(d11phyrxsts)) ||
			D11PHYRXSTS_LEN_INVALID(d11phyrxsts)) {

			WL_ERROR(("Invalid PhyRx Status; seqnum[%d] len[%d]\n",
				D11PHYRXSTS_SEQNUM(d11phyrxsts), d11phyrxsts->PhyRxStatusLen));

			if (D11PHYRXSTS_LEN_INVALID(d11phyrxsts)) {
				/* XXX: If PhyRx Status len is invalid then just ignoring
				 * the status and continuing Rx Processing.
				 */
				/* Set inuse bit for PhyRx Status buffer */
				STS_XFER_RING_SET_INUSE(phyrxs, seqidx);
				phyrxs_consumed = TRUE;
			}

			goto d11phyrxsts_invalid;
		}
	}
#endif /* ! STS_XFER_PHYRXS_MBX */

	ASSERT(STS_XFER_RING_GET_INUSE(phyrxs, seqidx) == 0);
	/* Set inuse bit for PhyRx Status buffer */
	STS_XFER_RING_SET_INUSE(phyrxs, seqidx);

	STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), cons);

	/* Set Valid PhyRx Status flag in d11rxhdr_t::dma_flags */
	RXHDR_GE129_SET_DMA_FLAGS_RXS_TYPE(rxh, RXS_MAC_UCODE_PHY);

	/* Set PhyRx Status radio unit in SW header */
	wrxh = CONTAINEROF(rxh, wlc_d11rxhdr_t, rxhdr);
	wrxh->radio_unit = WLC_UNIT(wlc);

	WL_INFORM(("wl%d: %s: Linking PhyRx Status buffer with seqnum[%d] to pkt[%p]\n",
		wlc->pub->unit, __FUNCTION__, rxh_seqnum, pkt));

	/* Encode seqnumber into PKTTAG */
	WLPKTTAG(pkt)->phyrxs_seqid = PHYRXS_SEQNUM_2_PKTTAG_SEQID(rxh_seqnum);

	return TRUE;

d11phyrxsts_invalid:

	STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), invalid);
	WL_ERROR(("wl%d: %s: PhyRx Status buffer with seqnum[%d] linking failed\n",
		wlc->pub->unit, __FUNCTION__, rxh_seqnum));

	WLPKTTAG(pkt)->phyrxs_seqid = STS_XFER_PHYRXS_SEQID_INVALID;
	return phyrxs_consumed;

} /* __phyrxs_consume_d11phyrxsts() */

/* __phyrxs_is_seqnum_consumed: return TRUE if PhyRx Status is already consumed.
 *
 * In UL-OFDMA, a single PPDU can have both mgmt/ctrl and data frames but only a single
 * PhyRxStatus is generated. So, PhyRxStatus will be linked to the first processed frame
 * i,e either to an mgmt frame or to a data frame. When processing other MPDU (data or mgmt frame)
 * check if PhyRx Status is already consumed before reporting "Missing Phy Rx Status" error.
 */
static bool BCMFASTPATH
__phyrxs_is_seqnum_consumed(sts_xfer_ring_t *phyrxs_ring, uint16 rxh_seqnum)
{
	uint16 rdidx, wridx, seqidx;

	seqidx = PHYRXS_SEQNUM_2_RINGIDX(phyrxs_ring, rxh_seqnum);

	rdidx = STS_XFER_RING_STATE(phyrxs_ring)->read;
	wridx = STS_XFER_RING_STATE(phyrxs_ring)->write;

	/* Check if read index is advanced but not more than half of ring depth. */
	if ((MODSUB_POW2(rdidx, seqidx, phyrxs_ring->depth) > 0) &&
		(MODSUB_POW2(rdidx, seqidx, phyrxs_ring->depth) < (phyrxs_ring->depth >> 1))) {
		return TRUE;
	}

	/* Check if write index is advanced but not more than half of ring depth. */
	if ((MODSUB_POW2(wridx, seqidx, phyrxs_ring->depth) > 0) &&
		(MODSUB_POW2(wridx, seqidx, phyrxs_ring->depth) < (phyrxs_ring->depth >> 1))) {
		return TRUE;
	}

	return FALSE;
} /* __phyrxs_is_seqnum_consumed() */

/*
 * __phyrxs_find_last_mpdu:
 * - Update rx_list_iter with last MPDU among set of packets with same seqnumber of head packet
 *   in rx_list_pend.
 * - All other MPDUS with same sequence of head packet are appended to the rx_list_release.
 * - rxh_next_seqnum_avail is set to TRUE if a packet with next seqnumber is found in rx_list_pend
 */
static INLINE void BCMFASTPATH
__phyrxs_find_last_mpdu(wlc_info_t * wlc, rx_list_t *rx_list_pend, rx_list_t *rx_list_iter,
	rx_list_t *rx_list_release, bool *rxh_next_seqnum_avail, uint8 rxh_offset)
{
	d11rxhdr_t *rxh;
	void *cur_pkt;
	uint16 rxh_seqnum, prev_rxh_seqnum = 0;

	while (RX_LIST_RX_HEAD(rx_list_pend) != NULL) {

		cur_pkt = RX_LIST_DELETE_HEAD(rx_list_pend);
		rxh = (d11rxhdr_t *)((uint8 *)PKTDATA(wlc->osh, cur_pkt) + rxh_offset);

		if (!IS_D11RXHDRSHORT(rxh, wlc->pub->corerev)) {

			rxh_seqnum = D11RXHDR_GE129_SEQNUM(rxh);
			ASSERT(rxh_seqnum < D11PHYRXSTS_SEQNUM_MAX);

			/* Break out on sequence jump, PhyRx Status should be present */
			if ((RX_LIST_RX_HEAD(rx_list_iter) != NULL) &&
				(rxh_seqnum != prev_rxh_seqnum)) {
				RX_LIST_INSERT_HEAD(rx_list_pend, cur_pkt);
				*rxh_next_seqnum_avail = TRUE;

				WL_INFORM(("wl%d: Next rxh_seqnum[%d] is available\n",
					wlc->pub->unit, rxh_seqnum));
				break;
			}

			prev_rxh_seqnum = rxh_seqnum;

			/* Append MPDU (iter_list) to release list */
			if (RX_LIST_RX_HEAD(rx_list_iter) != NULL) {
				RX_LIST_APPEND(rx_list_release, rx_list_iter);
				RX_LIST_INIT(rx_list_iter); /* rx_head = rx_tail = NULL */
			}
		}

		/* Update iter list */
		RX_LIST_INSERT_TAIL(rx_list_iter, cur_pkt);

	} /* while (RX_LIST_RX_HEAD(rx_list_pend) != NULL) */

} /* __phyrxs_find_last_mpdu() */

/*
 * ------------------------------------------------------------------------------------------------
 * wlc_sts_xfer_bmac_recv():
 *   - Three Rx lists are maintained:
 *     1. rx_list_pend: A list of Rx Packets to be linked with PhyRx Status. (Global scope)
 *     2. rx_list_release: A list of Rx Packets ready for WLAN Rx processing in current DPC call.
 *     3. rx_list_iter: Used for iteration of rx_list_pend
 *
 *   On arrival of Rx Packets, new packets are transferred to a Rx Pending list (rx_list_pend) and
 *   SW write index of ring is updated.
 *   If corresponding PhyRx Status is available then PhyRx Status is linked to the Last MPDU of an
 *   AMPDU and move the packets to the rx_list_release.
 *   if PhyRx status or all MPDU's of an AMPDU are not yet recieved then last MPDU with same
 *   sequence number will be held in RX_LIST_PEND and will be processed in next oppurtunity (DPC).
 *
 *   Implementatiton Notes:
 *    sts_xfer_phyrxs_t::inuse_bitmap is used to track PhyRx Buffers linked to Rx MPDU and to
 *    update read index.
 *
 * ------------------------------------------------------------------------------------------------
 */
void BCMFASTPATH
wlc_sts_xfer_bmac_recv(wlc_info_t *wlc, uint fifo, rx_list_t *rx_list_release, uint8 rxh_offset)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_phyrxs_t	*phyrxs;
	sts_xfer_ring_t		*phyrxs_ring;
	rx_list_t		*rx_list_pend;	/**< list of 'pending' rx packets */
	rx_list_t		rx_list_iter;	/**< iter list; Declared on stack */
	d11rxhdr_t		*rxh;
	void			*cur_pkt;
#if defined(HWA_PKTPGR_BUILD)
	void			*rxh_cur_pkt;
#endif
	uint16			rxh_seqnum, cons_seqidx;
	bool			rxh_next_seqnum_avail, phyrxs_avail;
	uint8			idx = RX_LIST_PEND_FIFO0_IDX;

	WL_TRACE(("wl%d: %s: ENTER \n", wlc->pub->unit, __FUNCTION__));

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);
	phyrxs_ring = &phyrxs->ring;

#if defined(BCMSPLITRX)
	if (fifo == RX_FIFO2) {
		idx = RX_LIST_PEND_FIFO2_IDX;
	}
#endif /* BCMSPLITRX */

	rx_list_pend = RX_LIST_PEND(phyrxs, idx);

	/* Append newly received packets to pendling list */
	if (RX_LIST_RX_HEAD(rx_list_release) != NULL) {
		RX_LIST_APPEND(rx_list_pend, rx_list_release);
		/* Don't reset rx_list_t::rxfifocnt */
		RX_LIST_RESET(rx_list_release); /* rx_head = rx_tail = NULL */
	}

	/* Update PhyRx Status ring write index to latest */
	__phyrxs_ring_write_update(sts_xfer, phyrxs, phyrxs_ring);

	while (RX_LIST_RX_HEAD(rx_list_pend) != NULL) {

		/* Peek into head packet; CAUTION: Not removing from list */
		cur_pkt = RX_LIST_RX_HEAD(rx_list_pend);
#if defined(HWA_PKTPGR_BUILD)
		rxh_cur_pkt = cur_pkt;
#endif

		/* MAC/uCode RXHDR */
		rxh = (d11rxhdr_t *)((uint8 *)PKTDATA(wlc->osh, cur_pkt) + rxh_offset);

		if (!IS_D11RXHDRSHORT(rxh, wlc->pub->corerev)) {
			rxh_next_seqnum_avail = FALSE;
			RX_LIST_INIT(&rx_list_iter); /* rx_head = rx_tail = NULL */

			rxh_seqnum = D11RXHDR_GE129_SEQNUM(rxh);
			ASSERT(rxh_seqnum < D11PHYRXSTS_SEQNUM_MAX);

			__phyrxs_find_last_mpdu(wlc, rx_list_pend, &rx_list_iter,
				rx_list_release, &rxh_next_seqnum_avail, rxh_offset);

			cur_pkt = RX_LIST_RX_HEAD(&rx_list_iter);
			ASSERT(cur_pkt != NULL);

			/* XXX: Seqnum might have already consumed by previous MPDU of AMPDU.
			 * We hit this condition when LAST MPDU DMA is not completed but
			 * PhyRx Status transfer/dma is completed and wridx is updated.
			 */
			if (phyrxs->cons_seqnum[idx] == rxh_seqnum) {
				/* Update release list */
				RX_LIST_APPEND(rx_list_release, &rx_list_iter);
				RX_LIST_INIT(&rx_list_iter); // rx_head = rx_tail = NULL
				continue;
			}

			phyrxs_avail = __phyrxs_seqnum_avail(sts_xfer, phyrxs,
					rxh_seqnum, phyrxs_ring, rxh_next_seqnum_avail);

			/* Consume new PhyRx Status if avialable else check if PhyRx Status is
			 * already consumed.
			 */
			if ((phyrxs_avail && __phyrxs_consume_d11phyrxsts(wlc, phyrxs, phyrxs_ring,
					cur_pkt, rxh_offset)) ||
				__phyrxs_is_seqnum_consumed(phyrxs_ring, rxh_seqnum)) {

				phyrxs->cons_seqnum[idx] = rxh_seqnum;

				/* Update release list */
				RX_LIST_APPEND(rx_list_release, &rx_list_iter);
				RX_LIST_INIT(&rx_list_iter); // rx_head = rx_tail = NULL
				continue;

			} else if (rxh_next_seqnum_avail || phyrxs_avail) {

				/* Missing PhyRx Status */
				if (rxh_next_seqnum_avail)
					STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), miss);

				/* XXX: Missing PhyRx Status is a critical error.
				 * Dump ucode and MAC stats and perform reinit (wlc_fatal_error()).
				 */
				WL_ERROR(("Big Hammer\n"));
				WL_ERROR(("%s: fifo[%d] Missing Phy Rx Status for rxh_seqnum[%d]\n",
					__FUNCTION__, fifo, rxh_seqnum));

				WL_ERROR(("next_seqnum_avail[%d]\n", rxh_next_seqnum_avail));

#if defined(HWA_PKTPGR_BUILD)
				if (idx == RX_LIST_PEND_FIFO0_IDX) {
					/* Dump HDBM and d11rxhdr_t */
					hwa_hdbm_dump(WL_HWA_DEVP(wlc), PKTMAPID(rxh_cur_pkt),
						((char *)rxh)-((char *)rxh_cur_pkt),
						sizeof(d11rxhdr_t));
					prhex("rxhdr", (uint8 *)rxh, sizeof(d11rxhdr_t));
				}

				/* WAR for HWA page-in RxHdr corruption. */
				if (idx == RX_LIST_PEND_FIFO0_IDX && rxh_seqnum == 0) {
					_phyrxs_error_log_dump(wlc, phyrxs, phyrxs_ring,
						FALSE, FALSE);
					phyrxs->seqnum_mismatch_cnt++;
				} else
#endif /* HWA_PKTPGR_BUILD */
				{
					_phyrxs_error_log_dump(wlc, phyrxs, phyrxs_ring,
						TRUE, TRUE);

#if defined(STS_XFER_PHYRXS_MISS_DEBUG)
#endif /* STS_XFER_PHYRXS_MISS_DEBUG */

					ASSERT(0);
				}

				/* Update release list anyway */
				RX_LIST_APPEND(rx_list_release, &rx_list_iter);
				RX_LIST_INIT(&rx_list_iter);
				continue;
			} else {
				/**
				 * PhyRx Status is not yet available. Move packets from iter list to
				 * head of pending list and hold pkt until next DPC or Status intr.
				 */
				RX_LIST_PREPEND(rx_list_pend, &rx_list_iter);
				RX_LIST_INIT(&rx_list_iter);
				break;
			}

		} else { /* ! IS_D11RXHDRSHORT() */

			cur_pkt = RX_LIST_DELETE_HEAD(rx_list_pend);
			/* Update release list */
			RX_LIST_INSERT_TAIL(rx_list_release, cur_pkt);
		}

	} /* while (RX_LIST_RX_HEAD(rx_list_pend) != NULL) */

	/* Move rd_last of to the next index of latest consumed buffer */
	cons_seqidx = PHYRXS_SEQNUM_2_RINGIDX(phyrxs_ring, (phyrxs->cons_seqnum[idx] + 1));

#if defined(BCMSPLITRX)
	{
		uint16 wridx = STS_XFER_RING_STATE(&phyrxs->ring)->write;
		/** Find latest of consumed seqnum of FIFO-0/1 and FIFO-2 */
		if (MODSUB_POW2(wridx, cons_seqidx, phyrxs_ring->depth) <
			MODSUB_POW2(wridx, phyrxs->rd_last, phyrxs_ring->depth)) {
			phyrxs->rd_last = cons_seqidx;
		}
	}
#else /* ! BCMSPLITRX */
	phyrxs->rd_last = cons_seqidx;
#endif /* ! BCMSPLITRX */

} /* wlc_sts_xfer_bmac_recv */

/**
 * ------------------------------------------------------------------------------------------------
 * wlc_sts_xfer_bmac_recv_done:
 * - Invoked at end of wlc_bmac_recv()
 * - Move read index to the next index of last consumed PhyRx Status buffer and update
 *   corresponding DMA attributes (dma_info_t::rxin or M2M DMA wridx).
 *
 * CAUTION: WLAN driver should not hold any PhyRx Status buffer at end of wlc_bmac_recv().
 * ------------------------------------------------------------------------------------------------
 */
void BCMFASTPATH
wlc_sts_xfer_bmac_recv_done(wlc_info_t *wlc, uint fifo)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_phyrxs_t	*phyrxs;
	sts_xfer_ring_t		*phyrxs_ring;
	uint16 rdidx;

	WL_TRACE(("wl%d: %s: ENTER \n", wlc->pub->unit, __FUNCTION__));

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);
	phyrxs_ring = &phyrxs->ring;

	if (!sts_xfer->inited) {
		WL_INFORM(("wl%d: %s: STS_XFER Module is NOT initialized \n",
			wlc->pub->unit, __FUNCTION__));
		return;
	}

	/**
	 * Rx-Mpdu's might be dropped in HNDDMA with bad frame lengths (hnddma_pub::rxgiant) or MAC
	 * but WLAN driver (rx_list_t) will not be aware of these frames and corresponding
	 * PhyRx Status buffer will not be linked to any Rx Packet.
	 * Due to this hole, read index will not be advanced and leads to PhyRx circular ring
	 * overflow and Rx traffic stall.
	 *
	 * NIC mode:
	 *	Rx-Packets are received in a single FIFO with linear PhyRx Status numbers so
	 *	if there is a hole in circular buffer inuse bitmap (sts_xfer_phyrxs_t::inuse_bitmap)
	 *	then ignore unlinked PhyRx Status buffer and advance read idx,
	 *
	 * Full Dongle:
	 *	In split FIFO mode and Rx Packets are received over FIFO-0/1 and FIFO-2 but
	 *	PhyRx Statuses for packets of both FIFO's are transferred into a single
	 *	circular ring. Unlinked PhyRx Status buffer (hole) in circular ring could belong
	 *	to either missing Rx-MPDU or pending packets to be processed by WLAN Rx.
	 *
	 *	XXX WAR:
	 *	WLAN driver will not hold any PhyRx Statuses in Rx path and will release all
	 *	consumed PhyRx Status buffers to circular ring in same DPC call. So if inuse
	 *	PhyRx Status buffers (rd_last - rdix) are more than 256 (Half of ring depth)
	 *	then ignore unlinked PhyRx Status buffer and advance index.
	 */
	rdidx = STS_XFER_RING_STATE(phyrxs_ring)->read;

	while (rdidx != phyrxs->rd_last) {
		if (STS_XFER_RING_GET_INUSE(phyrxs, rdidx) == 0) {

#if defined(BCMSPLITRX)
			if (MODSUB_POW2(phyrxs->rd_last, rdidx, phyrxs_ring->depth) < 256) {
				break;
			}
#endif /* BCMSPLITRX */

#if defined(HWA_PKTPGR_BUILD)
			if (phyrxs->seqnum_mismatch_cnt) {
			       phyrxs->seqnum_mismatch_cnt--;
			} else
#endif /* HWA_PKTPGR_BUILD */
			{

			/* Rx-Mpdu for this PhyRx Sequence number is either dropped in
			 * dma_rx() or missing in rx_list of FIFO-0/1 (rb168786).
			 * Ignore this Orphan PhyRx Status and advance rdidx.
			 */
			STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), mpdu_miss);

			/* XXX: Missing Rx packets is a critical error.
			 * Dump ucode and MAC stats and perform reinit (wlc_fatal_error()).
			 */
#if defined(STS_XFER_PHYRXS_MBX)
			WL_ERROR(("wl%d: %s: Rx Packet missing for seqidx[%d]\n",
				wlc->pub->unit, __FUNCTION__, rdidx));
#else /* ! STS_XFER_PHYRXS_MBX */
			WL_ERROR(("wl%d: %s: Rx Packet missing for seqnum[%d]\n",
				wlc->pub->unit, __FUNCTION__,
				D11PHYRXSTS_SEQNUM(STS_XFER_RING_ELEM(d11phyrxsts_t,
					phyrxs_ring, rdidx))));
#endif /* ! STS_XFER_PHYRXS_MBX */

			WL_ERROR(("ERROR: Frames should not dropped in HNDDMA or MAC. Fix it.\n"));

			_phyrxs_error_log_dump(wlc, phyrxs, phyrxs_ring, FALSE, FALSE);
			}
		}

		/* Clear inuse and advance Read index */
		STS_XFER_RING_CLR_INUSE(phyrxs, rdidx);
		rdidx = STS_XFER_RING_NEXT_IDX(phyrxs_ring, rdidx);
	}

	if (STS_XFER_RING_STATE(phyrxs_ring)->read != rdidx)
		STS_XFER_RING_STATE(phyrxs_ring)->read = rdidx;
	else
		return;

	WL_INFORM(("wl%d: %s: updated rdidx[%d]\n", wlc->pub->unit, __FUNCTION__, rdidx));

#if defined(STS_XFER_PHYRXS_FIFO)
	if (STS_XFER_PHYRXS_FIFO_ENAB(sts_xfer->wlc->pub)) {

		/* Move rxin and refill status buffers for STS_FIFO dma */
		dma_sts_rx_done(wlc->hw->di[STS_FIFO], rdidx);
		wlc_bmac_dma_rxfill(sts_xfer->wlc->hw, STS_FIFO);
	}
#endif /* STS_XFER_PHYRXS_FIFO */

#if defined(STS_XFER_PHYRXS_M2M)
	if (STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {

		volatile m2m_status_eng_regs_t *status_eng_regs;
		uint32	saved_core_id;
		uint32	saved_intr_val;
		uint32	v32; /* used in REG read/write */

		ASSERT(STS_XFER_PHYRXS_M2M_ENAB(sts_xfer->wlc->pub));
		status_eng_regs = phyrxs->phyrxs_regs;

		STS_XFER_SWITCHCORE(sts_xfer->sih, &saved_core_id, &saved_intr_val);

		/* Update M2M status engine read index */
		v32 = R_REG(wlc->osh, &status_eng_regs->rdidx);
		v32 = BCM_CBF(v32, M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX);
		v32 |= BCM_SBF(rdidx, M2M_STATUS_ENG_RDIDX_QUEUE_RDIDX);
		W_REG(wlc->osh, &status_eng_regs->rdidx, v32);

		STS_XFER_RESTORECORE(sts_xfer->sih, saved_core_id, saved_intr_val);
	}
#endif /* STS_XFER_PHYRXS_M2M */

} /* wlc_sts_xfer_bmac_recv_done() */

/** Returns TRUE if Rx packets are pending with PhyRx Status to be received */
bool BCMFASTPATH
wlc_sts_xfer_phyrxs_rxpend(wlc_info_t *wlc, uint fifo)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_phyrxs_t	*phyrxs;
	uint8			idx = 0;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);

#if defined(BCMSPLITRX)
	idx = ((fifo == RX_FIFO2) ? RX_LIST_PEND_FIFO2_IDX : RX_LIST_PEND_FIFO0_IDX);
#endif /* BCMSPLITRX */

	if (RX_LIST_RX_HEAD(RX_LIST_PEND(phyrxs, idx)) != NULL)
		return TRUE;

	return FALSE;
} /* wlc_sts_xfer_phyrxs_rxpend() */

/**
 * wlc_sts_xfer_phyrxs_set_cur_status:
 * Set Global HNDD11 PhyRx Status to point to the current PhyRx Status in process
 *
 * If Mailbox service is supported (HNDMBX) then
 *  - Initiate and sync a page-in transfer of PhyRx Status to dongle ping-pong buffer.
 *  - Validate seqnumber in PhyRx Status and point Global HNDD11 PhyRx Status to ping-ping buffer.
 */
void BCMFASTPATH
wlc_sts_xfer_phyrxs_set_cur_status(wlc_info_t *wlc, void *pkt, d11rxhdr_t *rxh)
{
	sts_xfer_phyrxs_t *phyrxs;
	d11phyrxsts_t	*d11phyrxsts;
	uint16		seqidx, rxh_seqnum;

	phyrxs = WLC_STS_XFER_PHYRXS(WLC_STS_XFER(wlc->sts_xfer_info));
	rxh_seqnum = D11RXHDR_GE129_SEQNUM(rxh);
	seqidx = PHYRXS_SEQNUM_2_RINGIDX(&phyrxs->ring, rxh_seqnum);

	ASSERT(WLPKTTAG(pkt)->phyrxs_seqid != STS_XFER_PHYRXS_SEQID_INVALID);

	/* WLAN will process one PhyRx Status at a time */
	ASSERT(HNDD11_PHYRXSTS_GET_CUR_STATUS(WLC_UNIT(wlc)) == D11PHYRXSTS_NULL);

#if defined(STS_XFER_PHYRXS_MBX)
	ASSERT(phyrxs->mbx_slot_cur == STS_XFER_MBX_SLOT_INVALID);

	/* Initiate a page-in PhyRx Status transfer into ping-pong slot */
	phyrxs->mbx_slot_cur = mbx_xfer(MBX_USR_PHYRXS, seqidx, MBX_SLOT_CUR);

	/* Sync ongoing DMA and get paged-in message */
	d11phyrxsts = (d11phyrxsts_t *) mbx_sync(MBX_USR_PHYRXS, MBX_SLOT_CUR);
	ASSERT(d11phyrxsts != NULL);

	/** Verify seqnumber in Ucode status with seqnumber in PhyRx Status */
	if ((rxh_seqnum != D11PHYRXSTS_SEQNUM(d11phyrxsts)) ||
		D11PHYRXSTS_LEN_INVALID(d11phyrxsts)) {

		WLPKTTAG(pkt)->phyrxs_seqid = STS_XFER_PHYRXS_SEQID_INVALID;
		/* Revert the d11rxhdr_t::dma_flags to MAC+uCode Rx Status */
		RXHDR_GE129_SET_DMA_FLAGS_RXS_TYPE(rxh, RXS_MAC_UCODE);

		STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), invalid);

		mbx_iter(MBX_USR_PHYRXS); // Iterate to next slot
		phyrxs->mbx_slot_cur = STS_XFER_MBX_SLOT_INVALID;

		WL_ERROR(("wl%d: %s: PhyRx Status buffer with seqnum[%d] linking failed\n",
			wlc->pub->unit, __FUNCTION__, rxh_seqnum));

		WL_ERROR(("Invalid PhyRx Status; seqnum[%d] len[%d]\n",
			D11PHYRXSTS_SEQNUM(d11phyrxsts), d11phyrxsts->PhyRxStatusLen));

		if (rxh_seqnum != D11PHYRXSTS_SEQNUM(d11phyrxsts)) {
			/* XXX: Missing PhyRx Status is a critical error.
			 * Dump ucode and MAC stats and perform reinit (wlc_fatal_error()).
			 */
			WL_ERROR(("Big Hammer\n"));

			_phyrxs_error_log_dump(wlc, phyrxs, &phyrxs->ring, TRUE, TRUE);

			ASSERT(0);
		}
		return;
	}
#else /* ! STS_XFER_PHYRXS_MBX */
	/* Fetch PhyRx Status buffer from circular ring */
	d11phyrxsts = STS_XFER_RING_ELEM(d11phyrxsts_t, &phyrxs->ring, seqidx);
#endif /* ! STS_XFER_PHYRXS_MBX */

	/* Set current PhyRx Status buffer */
	HNDD11_PHYRXSTS_SET_CUR_STATUS(WLC_UNIT(wlc), d11phyrxsts);

	STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), cur_sts_access);

} /* wlc_sts_xfer_phyrxs_set_cur_status() */

/** Release current PhyRx Status */
void BCMFASTPATH
wlc_sts_xfer_phyrxs_release_cur_status(wlc_info_t *wlc)
{
	if (HNDD11_PHYRXSTS_GET_CUR_STATUS(WLC_UNIT(wlc)) != D11PHYRXSTS_NULL) {
		sts_xfer_phyrxs_t *phyrxs;
		phyrxs = WLC_STS_XFER_PHYRXS(WLC_STS_XFER(wlc->sts_xfer_info));

#if defined(STS_XFER_PHYRXS_MBX)
		ASSERT(phyrxs->mbx_slot_cur != STS_XFER_MBX_SLOT_INVALID);
		mbx_iter(MBX_USR_PHYRXS); // Iterate to next slot

		phyrxs->mbx_slot_cur = STS_XFER_MBX_SLOT_INVALID;
#endif /* STS_XFER_PHYRXS_MBX */

		HNDD11_PHYRXSTS_SET_CUR_STATUS(WLC_UNIT(wlc), D11PHYRXSTS_NULL);

		STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), cur_sts_release);
	}
} /* wlc_sts_xfer_phyrxs_release_cur_status() */

/**
 * wlc_sts_xfer_phyrxs_release:
 * It will walk through the AMSDU packet chain and PhyRx Status buffer will be delinked from
 * Rx Packet.
 */
void BCMFASTPATH
wlc_sts_xfer_phyrxs_release(wlc_info_t *wlc, void *pkt)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_phyrxs_t	*phyrxs;
	sts_xfer_ring_t		*phyrxs_ring;
	uint16			phyrxs_seqnum;
	uint16			rdidx;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	phyrxs = WLC_STS_XFER_PHYRXS(sts_xfer);
	phyrxs_ring = &phyrxs->ring;

	rdidx = STS_XFER_RING_STATE(phyrxs_ring)->read;

	while (pkt != NULL) {

		/* Check if PhyRx Status buffer is linked to Rx packet */
		if ((WLPKTTAG(pkt)->phyrxs_seqid == (uint16)STS_XFER_PHYRXS_SEQID_INVALID) ||
			(WLPKTTAG(pkt)->phyrxs_seqid == 0)) {
			pkt = PKTNEXT(wlc->osh, pkt);
			continue;
		}

		if (!sts_xfer->inited) {
			/* Delink PhyRx Status buffer */
			WLPKTTAG(pkt)->phyrxs_seqid = STS_XFER_PHYRXS_SEQID_INVALID;
			pkt = PKTNEXT(wlc->osh, pkt);
			continue;
		}

		ASSERT(bcm_ring_is_empty(STS_XFER_RING_STATE(phyrxs_ring)) == FALSE);

		phyrxs_seqnum = PHYRXS_PKTTAG_SEQID_2_SEQNUM(WLPKTTAG(pkt)->phyrxs_seqid);

		/* Check for stale PhyRx Status buffer */
		if (!_phyrxs_seqnum_valid(phyrxs_seqnum, phyrxs_ring->depth,
			rdidx, phyrxs->rd_last)) {
			WL_ERROR(("%s: seqnum[%d] packet is hold after wlc_bmac_recv done\n",
				__FUNCTION__, phyrxs_seqnum));

			_phyrxs_error_log_dump(wlc, phyrxs, phyrxs_ring, FALSE, FALSE);
		}

		/* Validate PhyRx Status using inuse_bitmap */
		ASSERT(STS_XFER_RING_GET_INUSE(phyrxs,
			PHYRXS_SEQNUM_2_RINGIDX(phyrxs_ring, phyrxs_seqnum)));

		WL_INFORM(("wl%d: %s: PhyRx Status with seqnum[%d] is delinked from pkt[%p]\n",
			wlc->pub->unit, __FUNCTION__, phyrxs_seqnum, pkt));

		STS_XFER_STATS_INCR(PHYRXS_STATS(phyrxs), release);

		/* Delink PhyRx Status buffer */
		WLPKTTAG(pkt)->phyrxs_seqid = STS_XFER_PHYRXS_SEQID_INVALID;
		pkt = PKTNEXT(wlc->osh, pkt);
	}
} /* wlc_sts_xfer_phyrxs_release() */

/** PKTFREE register callback invoked on PKTFREE of Rx packet chain */
void BCMFASTPATH
wlc_sts_xfer_phyrxs_free(wlc_info_t *wlc, void *pkt)
{
	/* Walk through PKTC/PKTNEXT and release PhyRx Status */
#if defined(PKTC) || defined(PKTC_DONGLE)
	while (pkt != NULL) {
		wlc_sts_xfer_phyrxs_release(wlc, pkt);
		pkt = PKTCLINK(pkt);
	}
#else /* !(PKTC || PKTC_DONGLE) */

	ASSERT(PKTLINK(pkt) == NULL);
	wlc_sts_xfer_phyrxs_release(wlc, pkt);
#endif /* !(PKTC || PKTC_DONGLE) */

} /* wlc_sts_xfer_phyrxs_free() */

#endif /* STS_XFER_PHYRXS */

#if defined(STS_XFER_M2M_INTR)

/**
 * ================================================================================================
 * Section: M2M interrupt support
 *
 * Register settings to enable interrupts on Tx/PhyRx Status WR index update.
 * - Set bit 30,31 in M2MCORE IntControl/Status registers to enable Tx/PhyRx Status interrupts.
 * - Set Lazycount (M2M_STATUS_ENG_CTL_LAZYCOUNT) to 1 in M2M TXS/PHYRXS control registers.
 * - Integrated CHIPS
 *	- Register an IRQ handler for IRQ's belonging to M2M (GET_2x2AX_M2M_IRQV()).
 * - External CHIPS over PCIE.
 *	- Interrupts from all cores are routed to the PCIE core. Set M2M coreidx in PCIEIntMask
 *	  register (PCI_INT_MASK).
 *	- M2M intstatus/mask would be in M2M Core requiring switching of PCIe BAR but interrupt
 *	  context switching is not permitted.
 *	  Using sixth 4KB of PCIE BAR 0 space (PCIE2_BAR0_CORE2_WIN2) to map M2M core address space.
 * - Set Interrupt affinity for M2M interrupts (Get IRQ numbers from /proc/interrupts file)
 *	echo <CPU core> > /proc/irq/<IRQ>/smp_affinity
 *
 * PhyRx Status WR index update interrupt:
 *	Triggering WLAN Rx Processing (MI_DMAINT) on PhyRx Status arrival (dummy or valid) as
 *	opposed	to a Data/Mgmt packet arrival on RxFIFO0. By triggering RxProcessing on a
 *	PhyRx Status arrival, we can leverage an entire AMPDUs worth of packets readily available
 *	in a DPC run. MAC (Ucode) will post a dummy PhyRx Status in case the PhyRx Status had not
 *	arrived from PHY to MAC, rather than simply dropping (not posting) with skipped seqnum.
 *
 *	All (Data) packets in an AMPDU would have the same transmitter (AMT Index) and candidate for
 *	CFP bulk upstream processing, allowing long packet trains to flow through the WLAN stack and
 *	and subsequently handoff to the bridging subsystem.
 *
 * TxStatus WR index update interrupt:
 *	Not used and not verified on any platform.
 *
 * Full dongle:
 *	HNDM2M controls M2MCORE IntStatus, IntMask for the descriptor based asynchronous
 *	mem2mem DMA (channels #0 and #1). However, the BME channel #3 is used by D11 Core and
 *	explicit interrupts for WR Index update of TxStatus and PhyRxStatus are transferred via
 *	the "SAME" M2MCORE IntStatus, IntMask in bits 30 and 31.
 *	We can hence NOT have two subsystems managing the Interrupt Status/Mask:
 *	i.e. bits 0,1 by hndm2m.c (a service of the RTOS) and bits 30,31 by a wlan subsystem.
 *
 *	So, processing Tx & PhyRx Status interrupts (bit30, 31) in HNDM2M secondary IRQ handler.
 *
 *	CAUTION: STS_XFER module assumes that HNDM2M will not set any M2M core level interrupts
 *	(m2m_core_regs_t::intcontrol).
 * ================================================================================================
 */

/** Constructs and return name of M2M IRQ */
char *
wlc_sts_xfer_irqname(wlc_info_t *wlc, void *btparam)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_m2m_intr_t	*m2m_intr;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);

	BCM_REFERENCE(btparam);

#if defined(CONFIG_BCM_WLAN_DPDCTL)
	if (btparam != NULL) {
		/* bustype = PCI, even embedded 2x2AX devices have virtual pci underneath */
		snprintf(m2m_intr->irqname, sizeof(m2m_intr->irqname),
			"wlpcie:%s, wlan_%d_m2m", pci_name(btparam), wlc->pub->unit);
	} else
#endif /* CONFIG_BCM_WLAN_DPDCTL */
	{
		snprintf(m2m_intr->irqname, sizeof(m2m_intr->irqname), "wlan_%d_m2m",
			wlc->pub->unit);
	}

	return m2m_intr->irqname;
} /* wlc_sts_xfer_irqname() */

#if defined(DONGLEBUILD)

int	/* Register an IRQ handler for M2M interrupts */
BCMATTACHFN(wlc_sts_xfer_isr_register)(wlc_info_t *wlc, uint orig_unit, uint bus,
		hnd_isr_fn_t isr_fn, hnd_isr_sec_fn_t isr_sec_fn, hnd_worklet_fn_t worklet_fn,
		void *cbdata, osl_ext_task_t* thread)
{

#if defined(HNDM2M)
	/* HNDM2M library has already registered an IRQ handler for M2M core and RTE doesn't
	 * support two subsystems registering for the same interrupt so, Register and use
	 * secondary IRQ handler for TX & PhyRx Status interrupts.
	 *
	 * M2M secondary IRQ/DPC handlers will invoke WLAN IRQ/DPC handlers where all WLAN
	 * interrupts (MAC, M2M) are processed (poll all, disable all/restore-enable all).
	 */
	char *irqname;

	irqname = wlc_sts_xfer_irqname(wlc, NULL);

	/* Register a secondary IRQ handler for Tx and PhyRx Status M2M interrupts */
	return m2m_sec_isr_register(M2M_STATUS_ENG_M2M_CHANNEL, irqname,
		isr_sec_fn, worklet_fn, cbdata);

#else /* ! HNDM2M */

	/* Register an IRQ handler for M2M core interrupts */
	return (hnd_isr_register(M2MDMA_CORE_ID, orig_unit, bus, isr_fn, cbdata,
		worklet_fn, cbdata, thread) != NULL) ? BCME_OK : BCME_ERROR;
#endif /* ! HNDM2M */

} /* wlc_sts_xfer_isr_register() */

#else /* ! DONGLEBUILD */

uint	/* Returns si_flag of M2M core */
wlc_sts_xfer_m2m_si_flag(wlc_info_t *wlc)
{
	wlc_sts_xfer_t		*sts_xfer;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	return sts_xfer->m2m_intr.m2m_core_si_flag;
}
#endif /* ! DONGLEBUILD */

/** Reads and clear status of M2M interrupts set in sts_xfer_m2m_intr_t::intcontrol */
static uint32 BCMFASTPATH
wlc_sts_xfer_m2m_intstatus(wlc_info_t *wlc)
{
	volatile m2m_core_regs_t *m2m_core_regs;
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_m2m_intr_t	*m2m_intr;
	uint32			intstatus;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);
	m2m_core_regs = sts_xfer->m2m_core_regs;

	/* Detect cardbus removed, in power down(suspend) and in reset */
	if (DEVICEREMOVED(wlc))
		return -1;

	intstatus = R_REG(wlc->osh, &m2m_core_regs->intstatus);
	intstatus &= m2m_intr->defintmask;

	if (intstatus) {
		/* Clear interrupt status */
		W_REG(wlc->osh, &m2m_core_regs->intstatus, intstatus);

#if defined(STS_XFER_PHYRXS_M2M)
		/* M2M interrupts are enalbed only for PhyRx Status transfer */
		ASSERT(STS_XFER_PHYRXS_M2M_ENAB(wlc->pub));
		if (BCM_GBF(intstatus, M2M_CORE_INTCONTROL_PHYRXSWRINDUPD_INTMASK)) {
			wlc_intr_process_rxfifo_interrupts(wlc, TRUE);
		}
#endif /* STS_XFER_PHYRXS_M2M */
	}

	return intstatus;
} /* wlc_sts_xfer_m2m_intstatus() */

/**
 * Function:	wlc_sts_xfer_isr()
 * Description:	IRQ handler for M2M interrupts.
 *		Interrupt status is copied to software variable sts_xfer_m2m_intr_t::intstatus
 *		all M2M interrupts are disabled.
 *		A DPC will be scheduled and Tx/PhyRx Status packages are extracted in DPC.
 */
bool BCMFASTPATH
wlc_sts_xfer_isr(wlc_info_t *wlc, bool *wantdpc)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_m2m_intr_t	*m2m_intr;
	uint32			intstatus;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	if (m2m_intr->intcontrol == 0)
		return FALSE;

	/* Read and clear M2M intstatus register */
	intstatus = wlc_sts_xfer_m2m_intstatus(wlc);

	if (intstatus == 0xffffffff) {
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		WL_ERROR(("DEVICEREMOVED detected in the ISR code path.\n"));
		/* In rare cases, we may reach this condition as a race condition may occur
		 * between disabling interrupts and clearing the SW macintmask.
		 * Clear M2M int status as there is no valid interrupt for us.
		 */
		m2m_intr->intstatus = 0;
		/* assume this is our interrupt as before; note: want_dpc is FALSE */
		return TRUE;
	}

	/* It is not for us */
	if (intstatus == 0) {
		return FALSE;
	}

	/* Save interrupt status bits */
	m2m_intr->intstatus = intstatus;

	WL_INFORM(("wl%d: %s: M2M core instatus 0x%x \n", wlc->pub->unit, __FUNCTION__, intstatus));

	/* Turn off interrupts and schedule DPC */
	*wantdpc |= TRUE;

	return TRUE;
} /* wlc_sts_xfer_isr() */

void
wlc_sts_xfer_intrson(wlc_info_t *wlc)
{
	volatile m2m_core_regs_t *m2m_core_regs;
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_m2m_intr_t	*m2m_intr;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);
	m2m_core_regs = sts_xfer->m2m_core_regs;

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	m2m_intr->intcontrol = m2m_intr->defintmask;
	/* Enable M2M interrupts */
	W_REG(wlc->osh, &m2m_core_regs->intcontrol, m2m_intr->intcontrol);

} /* wlc_sts_xfer_intrson() */

void
wlc_sts_xfer_intrsoff(wlc_info_t *wlc)
{
	volatile m2m_core_regs_t *m2m_core_regs;
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_m2m_intr_t	*m2m_intr;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);
	m2m_core_regs = sts_xfer->m2m_core_regs;

#if defined(HNDM2M) && defined(BCMDBG)
	{
		uint32 v32; /* used in REG read/write */

		v32 = R_REG(wlc->osh, &m2m_core_regs->intcontrol);
		/* CAUTION: STS_XFER assumes that HNDM2M will not set any core level interrupts */
		ASSERT((v32 == 0) || (v32 == m2m_intr->intcontrol));

		BCM_REFERENCE(v32);
	}
#endif /* HNDM2M && BCMDBG */

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	/* Disable M2M interrupts */
	W_REG(wlc->osh, &m2m_core_regs->intcontrol, 0);
	m2m_intr->intcontrol = 0;

} /* wlc_sts_xfer_intrsoff() */

void
wlc_sts_xfer_intrsrestore(wlc_info_t *wlc, uint32 macintmask)
{
	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	if (macintmask) {
		wlc_sts_xfer_intrson(wlc);
	} else {
		wlc_sts_xfer_intrsoff(wlc);
	}
} /* wlc_sts_xfer_intrsrestore() */

/** deassert interrupt, gets called by dongle firmware builds during wl_isr() */
void
wlc_sts_xfer_intrs_deassert(wlc_info_t *wlc)
{
	volatile m2m_core_regs_t *m2m_core_regs;
	wlc_sts_xfer_t		*sts_xfer;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	m2m_core_regs = sts_xfer->m2m_core_regs;

#if defined(HNDM2M) && defined(BCMDBG)
	{
		sts_xfer_m2m_intr_t *m2m_intr;
		uint32 v32; /* used in REG read/write */

		m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);
		v32 = R_REG(wlc->osh, &m2m_core_regs->intcontrol);
		/* CAUTION: STS_XFER assumes that HNDM2M will not set any core level interrupts */
		ASSERT((v32 == 0) || (v32 == m2m_intr->intcontrol));

		BCM_REFERENCE(v32);
		BCM_REFERENCE(m2m_intr);
	}
#endif /* HNDM2M && BCMDBG */

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	/* Disable M2M interrupts */
	W_REG(wlc->osh, &m2m_core_regs->intcontrol, 0);

} /* wlc_sts_xfer_intrs_deassert() */

/**
 * Called by DPC when the DPC is rescheduled, updates sts_xfer_m2m_intr_t->intstatus
 *
 * Prerequisite:
 * - Caller should have acquired a spinlock against isr concurrency, or guarantee that interrupts
 *   have been disabled.
 */
void BCMFASTPATH
wlc_sts_xfer_intrsupd(wlc_info_t *wlc)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_m2m_intr_t	*m2m_intr;
	uint32			intstatus;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);

	ASSERT(m2m_intr->intcontrol == 0);
	intstatus = wlc_sts_xfer_m2m_intstatus(wlc);

	/* Device is removed */
	if (intstatus == 0xffffffff) {
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		return;
	}

	/* Update interrupt status in software */
	m2m_intr->intstatus |= intstatus;

} /* wlc_sts_xfer_intrsupd() */

void BCMFASTPATH
wlc_sts_xfer_process_m2m_intstatus(wlc_info_t *wlc)
{
	wlc_sts_xfer_t		*sts_xfer;
	sts_xfer_m2m_intr_t	*m2m_intr;
	uint32			intstatus;

	sts_xfer = WLC_STS_XFER(wlc->sts_xfer_info);
	m2m_intr = WLC_STS_XFER_M2M_INTR(sts_xfer);

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	if (m2m_intr->intstatus == 0)
		return;

	intstatus = m2m_intr->intstatus;

	WL_INFORM(("wl%d: %s: intstatus[%x]\n", wlc->pub->unit, __FUNCTION__, intstatus));

#if defined(STS_XFER_TXS)
	if (BCM_GBF(intstatus, M2M_CORE_INTCONTROL_TXSWRINDUPD_INTMASK)) {
		/* M2M/BME DMA transferred TxStatus to memory */
		intstatus = BCM_CBF(intstatus, M2M_CORE_INTCONTROL_TXSWRINDUPD_INTMASK);
		wlc->hw->macintstatus |= MI_TFS;
	}
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS_M2M)
	if (BCM_GBF(intstatus, M2M_CORE_INTCONTROL_PHYRXSWRINDUPD_INTMASK)) {
		/* M2M/BME DMA transferred PhyRx Status to memory */
		intstatus = BCM_CBF(intstatus, M2M_CORE_INTCONTROL_PHYRXSWRINDUPD_INTMASK);
		wlc->hw->macintstatus |= MI_DMAINT;
	}
#endif /* STS_XFER_PHYRXS_M2M */

	ASSERT(intstatus == 0);
	m2m_intr->intstatus = intstatus;

} /* wlc_sts_xfer_process_m2m_intstatus() */

#endif /* STS_XFER_M2M_INTR */
