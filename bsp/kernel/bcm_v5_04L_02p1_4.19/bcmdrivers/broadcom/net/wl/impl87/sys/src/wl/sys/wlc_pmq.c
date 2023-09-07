/*
 * PMQ (Power Management Queue) handling.
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
 * $Id: wlc_pmq.c 809830 2022-03-24 14:35:53Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <wlc_types.h>
#include <osl.h>
#include <wlc_types.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_pmq.h>
#include <wlc_apps.h>
#include <wlc_he.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wlc_dump.h>

/* ======== structures and definitions ======== */

/* bitmap of auxpmq index */
typedef uint8 auxpmq_idx_bitmap_t;

typedef struct auxpmq_entry {
	struct ether_addr ea;		/* station address */
} auxpmq_entry_t;

/* module info */
struct wlc_pmq_info {
	wlc_info_t *wlc;
	int scb_handle;			/* pmq scb cubby handle */

	uint8 read_count; /* how many entries have been read since the last clear */

	/* The following is used for aux pmq */
	auxpmq_entry_t *auxpmq_array;
	auxpmq_idx_bitmap_t *auxpmq_used;
	uint16 auxpmq_entries; /* how many entries hw has */
	uint32 auxpmq_full_cnt;
	uint16 auxpmq_entry_cnt;
	bool auxpmq_full;

	uint32 ps_on_count;
	uint32 ps_pretend_count;
	uint32 ps_omi_count;
	uint8 max_read_count;
	uint8 max_read_count_since_start;
};

#ifdef WL_AUXPMQ

#define AUXPMQ_INVALID_IDX	0xFFFF

typedef struct pmq_scb_cubby {
	uint16		auxpmq_idx;
	uint8		auxpmq_inited;
} pmq_scb_cubby_t;
#define PMQ_SCB_CUBBY(pmq_info, scb) (pmq_scb_cubby_t *)SCB_CUBBY(scb, (pmq_info->scb_handle))

/* scb cubby */
static int wlc_pmq_scb_init(void *ctx, struct scb *scb);
static void wlc_pmq_scb_deinit(void *ctx, struct scb *scb);

#endif /* WL_AUXPMQ */

#if defined(BCMDBG)
static int wlc_dump_pmq(wlc_pmq_info_t *pmq, struct bcmstrbuf *b);
#endif

/* ======== attach/detach ======== */

wlc_pmq_info_t *
BCMATTACHFN(wlc_pmq_attach)(wlc_info_t *wlc)
{
	wlc_pmq_info_t *pmq;
	uint corerev = wlc->pub->corerev;

	/* allocate private module info */
	if ((pmq = MALLOCZ(wlc->osh, sizeof(*pmq))) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	pmq->wlc = wlc;

	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, NULL, "pmq", pmq, NULL, NULL, NULL, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG)
	wlc_dump_register(wlc->pub, "pmq", (dump_fn_t)wlc_dump_pmq, (void *)pmq);
#endif

	BCM_REFERENCE(corerev);
#ifdef WL_AUXPMQ
	/* reserve scb cubby */
	pmq->scb_handle = wlc_scb_cubby_reserve(wlc, sizeof(struct pmq_scb_cubby),
		wlc_pmq_scb_init, wlc_pmq_scb_deinit, NULL, (void *)pmq);
	if (pmq->scb_handle < 0) {
		WL_ERROR(("wl%d: %s wlc_scb_cubby_reserve() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* Check if hardware supports WL_AUXPMQ. */
	wlc->pub->_auxpmq = (D11REV_IS(corerev, 65) || D11REV_GE(corerev, 128));
	if (wlc->pub->_auxpmq) {
		if (D11REV_GE(corerev, 132)) {
			pmq->auxpmq_entries = AUXPMQ_ENTRIES_GE132;
		} else if (D11REV_GE(corerev, 129)) {
			pmq->auxpmq_entries = AUXPMQ_ENTRIES_GE129;
		} else {
			pmq->auxpmq_entries = AUXPMQ_ENTRIES;
		}

		/* Preallocate all entries of AuxPMQ. */
		pmq->auxpmq_array = (auxpmq_entry_t *)MALLOCZ(wlc->osh,
				pmq->auxpmq_entries * sizeof(auxpmq_entry_t));
		pmq->auxpmq_used = (auxpmq_idx_bitmap_t *)MALLOCZ(wlc->osh,
			CEIL(pmq->auxpmq_entries, NBBY) * sizeof(auxpmq_idx_bitmap_t));
		if (pmq->auxpmq_array == NULL || pmq->auxpmq_used == NULL) {
			WL_ERROR(("wl%d: Init AuxPMQ error. Out of memory !!\n", wlc->pub->unit));
			goto fail;
		}
	}

#endif /* WL_AUXPMQ */

	return pmq;

fail:
	MODULE_DETACH(pmq, wlc_pmq_detach);
	return NULL;
}

void
BCMATTACHFN(wlc_pmq_detach)(wlc_pmq_info_t *pmq)
{
	wlc_info_t *wlc;

	if (pmq == NULL)
		return;

	wlc = pmq->wlc;

#ifdef WL_AUXPMQ
	if (pmq->auxpmq_array) {
		MFREE(wlc->osh, pmq->auxpmq_array, pmq->auxpmq_entries * sizeof(auxpmq_entry_t));
	}
	if (pmq->auxpmq_used) {
		MFREE(wlc->osh, pmq->auxpmq_used,
		      CEIL(pmq->auxpmq_entries, NBBY) * sizeof(auxpmq_idx_bitmap_t));
	}
#endif /* WL_AUXPMQ */

	(void)wlc_module_unregister(wlc->pub, "pmq", pmq);

	MFREE(wlc->osh, pmq, sizeof(*pmq));
}

#if defined(BCMDBG) && defined(WL_AUXPMQ)
/**
 * Read from HW Aux PMQ according to the index of bmac_auxpmq_entry_t.
 * Ecah entry is stored in Aux PMQ memory.
 * addr is the mac address of the mapping STA.
 * data is the PMQ data for the mapping STA.
 */
static void
wlc_bmac_read_auxpmq(wlc_hw_info_t *wlc_hw, int idx, struct ether_addr *addr, uint16 *data)
{
	uint32 word[2];

	WL_TRACE(("wl%d: %s: idx %d\n", wlc_hw->unit, __FUNCTION__, idx));
	ASSERT(wlc_hw->corerev >= 64);

	/* ObjAddr[6:0] of objAddr refer to the Index*2 of the AuxPMQ
	 * ObjAddr[23:16] = 7, it will select for AuxPmq.
	 * 2 Consecutive reads of ObjData will be 64 bit entry of the AuxPMQ.
	 */
	wlc_bmac_copyfrom_objmem(wlc_hw, (idx * 2) << 2, word,
		sizeof(word), OBJADDR_AUXPMQ_SEL);

	addr->octet[0] = (uint8)word[0];
	addr->octet[1] = (uint8)(word[0] >> 8);
	addr->octet[2] = (uint8)(word[0] >> 16);
	addr->octet[3] = (uint8)(word[0] >> 24);
	addr->octet[4] = (uint8)word[1];
	addr->octet[5] = (uint8)(word[1] >> 8);
	*data = (word[1] >> 16);
}
#endif /* BCMDBG && WL_AUXPMQ */

#if defined(BCMDBG)
static int
wlc_dump_pmq(wlc_pmq_info_t *pmq, struct bcmstrbuf *b)
{
	wlc_info_t *wlc = pmq->wlc;
	struct scb_iter scbiter;
	struct scb *scb;
	pmq_scb_cubby_t *scb_pmq;
	struct ether_addr entre_addr;
	uint16 entry_data;

	bcm_bprintf(b, "max_read_count_since_start %d, max_read_count_since_last_dump %d\n",
		pmq->max_read_count_since_start, pmq->max_read_count);

	bcm_bprintf(b, "ps_on %d, ps_pretend %d, ps_omi %d\n",
		pmq->ps_on_count, pmq->ps_pretend_count, pmq->ps_omi_count);

#ifdef WL_AUXPMQ
	bcm_bprintf(b, "aux pmq entries %d\n", pmq->auxpmq_entries);
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		scb_pmq = PMQ_SCB_CUBBY(wlc->pmq, scb);
		if (scb_pmq) {
			if (scb_pmq->auxpmq_idx < pmq->auxpmq_entries) {
				wlc_bmac_read_auxpmq(wlc->hw, scb_pmq->auxpmq_idx, &entre_addr,
					&entry_data);
				bcm_bprintf(b, "  %d "MACF"\n", scb_pmq->auxpmq_idx,
					ETHER_TO_MACF(entre_addr));
			}
		}
	}
#endif

	pmq->max_read_count = 0;
	return 0;
}
#endif /* BCMDBG */

#ifdef WL_AUXPMQ
/**
 * Write data to HW Aux PMQ. AuxPMQ (APMQ) (width = 64 bits, 48 addr bits + 16 attribute bits)
 * Data is composed of 6 bytes Mac address + 2 bytes PMQ data.
 * idx is mapping from 0 to 63. Each entry is 8 bytes.
 */
static void
wlc_pmq_write_auxpmq(wlc_hw_info_t *wlc_hw, int idx, const struct ether_addr *addr, uint16 data)
{
	uint32 word[2];

	WL_TRACE(("wl%d: %s: idx %d\n", wlc_hw->unit, __FUNCTION__, idx));
	ASSERT(wlc_hw->corerev >= 64);

	/* lower 4 bytes of Mac address */
	word[0] = (addr->octet[3] << 24) |
		(addr->octet[2] << 16) |
		(addr->octet[1] << 8) |
		addr->octet[0];
	/* higher 2 bytes of Mac address and 2 bytes PMQ data */
	word[1] = (data << 16) |
		(addr->octet[5] << 8) |
		addr->octet[4];

	/* ObjAddr[6:0] of objAddr refer to the Index*2 of the AuxPMQ
	 * ObjAddr[23:16] = 7, it will select for AuxPmq.
	 * 2 Consecutive writes of ObjData will be 64 bit entry of the AuxPMQ.
	 */
	wlc_bmac_copyto_objmem(wlc_hw, (idx * 2) << 2, word, sizeof(word), OBJADDR_AUXPMQ_SEL);
}

/** scb cubby init function */
static int
wlc_pmq_scb_init(void *context, struct scb *scb)
{
	wlc_pmq_info_t *pmq = (wlc_pmq_info_t *)context;

	pmq_scb_cubby_t *scb_pmq = PMQ_SCB_CUBBY(pmq, scb);

	scb_pmq->auxpmq_idx = AUXPMQ_INVALID_IDX;
	scb_pmq->auxpmq_inited = 1;

	return BCME_OK;
}

/** scb cubby deinit function */
static void
wlc_pmq_scb_deinit(void *context, struct scb * scb)
{
	wlc_pmq_info_t *pmq = (wlc_pmq_info_t *)context;
	wlc_info_t *wlc = pmq->wlc;
	pmq_scb_cubby_t *scb_pmq = PMQ_SCB_CUBBY(pmq, scb);

	if (!scb_pmq->auxpmq_inited)
		return;
	scb_pmq->auxpmq_inited = 0;

	if (AP_ENAB(wlc->pub) || (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(scb)) && SCB_PS(scb))) {
#if defined(WL_PS_SCB_TXFIFO_BLK)
		if (!wlc->ps_scb_txfifo_blk) {
#else
		if (!(wlc->block_datafifo & DATA_BLOCK_PS)) {
#endif /* WL_PS_SCB_TXFIFO_BLK */
			wlc_pmq_process_switch(wlc, scb, PMQ_CLEAR_ALL);
		} else {
			wlc_pmq_process_switch(wlc, scb, PMQ_REMOVE);
		}
	}
}

static uint16
wlc_pmq_auxpmq_add(wlc_pmq_info_t *pmq, struct ether_addr *ea, uint32 data)
{
	auxpmq_entry_t *add;
	uint16 idx;

	for (idx = 0; idx < pmq->auxpmq_entries; ++idx) {
		if (!isset(pmq->auxpmq_used, idx))
			break;
	}

	if (idx == pmq->auxpmq_entries) {
		WL_ERROR(("wl%d: Add AuxPMQ is full !!\n", pmq->wlc->hw->unit));
		idx = AUXPMQ_INVALID_IDX;
		pmq->auxpmq_full_cnt++;
		pmq->auxpmq_full = TRUE;
		goto done;
	}

	add = &(pmq->auxpmq_array[idx]);

	setbit(pmq->auxpmq_used, idx);

	pmq->auxpmq_entry_cnt++;

	memcpy(&add->ea, ea, sizeof(struct ether_addr));

	WL_PS(("Add "MACF" to AuxPMQ \n", ETHER_TO_MACF(add->ea)));

	/* For PMQ Data, Set BIT1:PSMode(PM) to indicate the STA in PS */
	wlc_pmq_write_auxpmq(pmq->wlc->hw, idx, ea, (data >> 16));
done:
	return idx;
}

static void BCMFASTPATH
wlc_pmq_auxpmq_remove(wlc_pmq_info_t *pmq, uint16 idx)
{
	auxpmq_entry_t *remove;

	ASSERT(isset(pmq->auxpmq_used, idx));

	remove = &(pmq->auxpmq_array[idx]);

	WL_PS(("Removed "MACF" from AuxPMQ \n", ETHER_TO_MACF(remove->ea)));

	wlc_pmq_write_auxpmq(pmq->wlc->hw, idx, &ether_null, 0);

	clrbit(pmq->auxpmq_used, idx);

	pmq->auxpmq_entry_cnt--;

	memset(remove, 0, sizeof(auxpmq_entry_t));

	pmq->auxpmq_full = FALSE;
}

/** clear auxpmq index for each SCB. */
static void
wlc_pmq_clear_auxpmq(wlc_pmq_info_t *pmq)
{
	wlc_info_t *wlc = pmq->wlc;
	struct scb_iter scbiter;
	struct scb *scb;
	pmq_scb_cubby_t *scb_pmq;
	wlc_bsscfg_t *bsscfg;
	uint8 idx;

	WL_PS(("PS : clearing ucode AuxPMQ\n"));

	FOREACH_AP(wlc, idx, bsscfg) {
		scb = WLC_BCMCSCB_GET(wlc, bsscfg);
		if (scb) {
			scb_pmq = PMQ_SCB_CUBBY(wlc->pmq, scb);
			ASSERT(scb_pmq);
			if (scb_pmq->auxpmq_idx < pmq->auxpmq_entries) {
				wlc_pmq_auxpmq_remove(pmq, scb_pmq->auxpmq_idx);
				scb_pmq->auxpmq_idx = AUXPMQ_INVALID_IDX;
			}
		}
	}
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		scb_pmq = PMQ_SCB_CUBBY(wlc->pmq, scb);
		ASSERT(scb_pmq);
		if (scb_pmq->auxpmq_idx < pmq->auxpmq_entries) {
			wlc_pmq_auxpmq_remove(pmq, scb_pmq->auxpmq_idx);
			scb_pmq->auxpmq_idx = AUXPMQ_INVALID_IDX;
		}
	}
	ASSERT(pmq->auxpmq_full == FALSE);
	ASSERT(pmq->auxpmq_entry_cnt == 0);
}

/**
 * This function is called by init, but also during re-int. We want to clear PMQ in both cases,
 * but during init we also want to init objmem with 0. So a proper cleanup using
 * wlc_pmq_clear_auxpmq is done for reinit, while the memset is for init. It is not worth the
 * effort to split this.
 */
void
wlc_pmq_reset_auxpmq(wlc_info_t *wlc)
{
	int auxpmq_len;

	auxpmq_len = wlc->pmq->auxpmq_entries * AUXPMQ_ENTRY_SIZE;
	wlc_pmq_clear_auxpmq(wlc->pmq);
	wlc_bmac_set_objmem32(wlc->hw, 0, 0, auxpmq_len, OBJADDR_AUXPMQ_SEL);
}

#endif /* WL_AUXPMQ */

/**
 * AP specific. Read and drain all the PMQ entries while not EMPTY.
 * When PMQ handling is enabled (MCTL_DISCARD_PMQ in maccontrol is clear),
 * one PMQ entry per packet received from a STA is created with corresponding 'ea' as key.
 * AP reads the entry and handles the PowerSave mode transitions of a STA by
 * comparing the PMQ entry with current PS-state of the STA. If PMQ entry is same as the
 * driver state, it's ignored, else transition is handled.
 *
 * With MBSS code, ON PMQ entries are also added for BSS configs; they are
 * ignored by the SW.
 *
 * Note that PMQ entries remain in the queue for the ucode to search until
 * an explicit delete of the entries is done with PMQH_DEL_MULT (or DEL_ENTRY).
 * When auxPMQ is active then the search happens on auxPMQ and mainPMQ. Driver will move
 * PM ON events for stations which need suppression from mainPMQ to auxPMQ. Once auxPMQ is full,
 * mainPMQ is not flushed till tx drain is complete.
 */
static void
wlc_pmq_clearpmq(wlc_pmq_info_t *pmq)
{
	wlc_hw_info_t *wlc_hw = pmq->wlc->hw;
	volatile uint16 *pmqctrlstatus;

	if (!pmq->read_count)
		return;

	pmqctrlstatus = (volatile uint16 *)(&(D11Reggrp_pmqreg(wlc_hw, 0)->w.pmqctrlstatus));
	/* Clear the PMQ entry unless we are letting the data fifo drain
	 * when txstatus indicates unlocks the data fifo we clear
	 * the PMQ of any processed entries
	 */

	W_REG(wlc_hw->osh, pmqctrlstatus, (uint16)PMQH_DEL_MULT);

	if (pmq->max_read_count < pmq->read_count) {
		pmq->max_read_count = pmq->read_count;
		if (pmq->max_read_count_since_start < pmq->max_read_count) {
			pmq->max_read_count_since_start = pmq->max_read_count;
		}
	}

	pmq->read_count = 0;
}

/**
 * AP specific. Process PMQ operations.
 */
void
wlc_pmq_process_switch(wlc_info_t *wlc, struct scb *scb, uint8 pmq_flags)
{
	wlc_pmq_info_t *pmq = wlc->pmq;
#ifdef WL_AUXPMQ
	pmq_scb_cubby_t *scb_pmq;
#endif /* WL_AUXPMQ */

	/*
	 * PMQ bits are used like this :
	 * - PMQ_CLEAR_ALL : Remove all PMQ entries. For non AuxPMQ it is called when there is no
	 *                   more packets pending. For AuxPMQ it means suppression is complete for
	 *                   all STAs which are in PM.
	 * - PMQ_ADD       : AUXPMQ only, used to add entry to AuxPMQ to keep suppressing this SCB
	 * - PMQ_REMOVE    : AUXPMQ only, used to remove entry from AuxPMQ once suppressing SCB is
	 *                   complete/done.
	 */
	BCM_REFERENCE(scb);
#ifdef WL_AUXPMQ
	if (!AUXPMQ_ENAB(wlc->pub)) {
		goto auxpmq_done;
	}
	if (pmq_flags & PMQ_CLEAR_ALL) {
		/* Clear all entries, none of the STAs need supression anymore */
		if (pmq->auxpmq_entry_cnt) {
			wlc_pmq_clear_auxpmq(pmq);
		}
		goto auxpmq_done;
	}

	ASSERT(scb);
	scb_pmq = PMQ_SCB_CUBBY(pmq, scb);
	ASSERT(scb_pmq);

	if (pmq_flags & PMQ_REMOVE) {
		/* Remove AuxPMQ entry if SCB finished transitioning. */
		if (scb_pmq->auxpmq_idx < pmq->auxpmq_entries) {
			wlc_pmq_auxpmq_remove(pmq, scb_pmq->auxpmq_idx);
			scb_pmq->auxpmq_idx = AUXPMQ_INVALID_IDX;
		} else {
			WL_PS(("wl%d: "MACF" not in auxpmq !\n", wlc->pub->unit,
				ETHERP_TO_MACF(&scb->ea)));
		}
	}
	if (pmq_flags & PMQ_ADD) {
		/* Add AuxPMQ Entry when SCB needs to suppress data. */
		if (scb_pmq->auxpmq_idx != AUXPMQ_INVALID_IDX) {
			WL_PS(("wl%d: "MACF" already in auxpmq, idx %d !\n",
				wlc->pub->unit, ETHERP_TO_MACF(&scb->ea), scb_pmq->auxpmq_idx));
		} else {
			/* If this is BCMC SCB then use the BSSCFG ea iso SCB ea */
			if (WLC_BCMCSCB_GET(wlc, scb->bsscfg) == scb) {
				scb_pmq->auxpmq_idx = wlc_pmq_auxpmq_add(pmq,
					&scb->bsscfg->cur_etheraddr, PMQH_BSSCFG | PMQH_PMON);
			} else {
				scb_pmq->auxpmq_idx = wlc_pmq_auxpmq_add(pmq, &scb->ea, PMQH_PMON);
			}
		}
	}
auxpmq_done:
#endif /* WL_AUXPMQ */

	/* no more packet pending and no more non-acked switches ... clear the PMQ */
	if (pmq_flags & PMQ_CLEAR_ALL) {
		wlc_pmq_clearpmq(pmq);
	}
} /* wlc_pmq_process_switch */

/**
 * wlc_pmq_processpmq is the interrupt processing routine. Read the PMQ queue till all entries
 * have been read. Find SCB and process the PM event by calling APPS.
 */
bool
wlc_pmq_processpmq(wlc_hw_info_t *wlc_hw, bool bounded)
{
	wlc_info_t *wlc = wlc_hw->wlc;
	volatile uint32 *pmqhostdata;
	uint32 pmqdata;
	uint32 pat_hi, pat_lo;
	struct ether_addr eaddr;
	wlc_pmq_info_t *pmq = wlc->pmq;
	uint8 ps_on, ps_pretend, ps_disassoc_deauth, ps_omi, ps_on_rqstr, ps_off_rqstr;
	struct scb *scb;
	int32 idx;
	wlc_bsscfg_t *cfg;
	bool cleared;

#if defined(BCMDBG) || defined(BCMDBG_ERR)
	char eabuf[ETHER_ADDR_STR_LEN];
	BCM_REFERENCE(eabuf);
#endif
	/* Ignore bounded. PMQ interrupts should be processed at once to avoid overflowing */
	BCM_REFERENCE(bounded);

	pmqhostdata = (volatile uint32 *)(&(D11Reggrp_pmqreg(wlc_hw, 0)->pmqhostdata));

	/* read entries until empty or pmq exceeding limit */
	cleared = TRUE;
	while (1) {
		ps_on = ps_omi = 0;
		ps_pretend = 0;
		ps_disassoc_deauth = 0;

		pmqdata = R_REG(wlc_hw->osh, pmqhostdata);
		if (!(pmqdata & PMQH_NOT_EMPTY)) {
			break;
		}
		cleared = FALSE;

		pat_lo = R_REG(wlc_hw->osh, D11_PMQPATL(wlc_hw));
		pat_hi = R_REG(wlc_hw->osh, D11_PMQPATH(wlc_hw));
		eaddr.octet[5] = (pat_hi >> 8)  & 0xff;
		eaddr.octet[4] =  pat_hi	& 0xff;
		eaddr.octet[3] = (pat_lo >> 24) & 0xff;
		eaddr.octet[2] = (pat_lo >> 16) & 0xff;
		eaddr.octet[1] = (pat_lo >> 8)  & 0xff;
		eaddr.octet[0] =  pat_lo	& 0xff;

		pmq->read_count++;

		if (ETHER_ISMULTI(eaddr.octet)) {
			WL_INFORM(("wl%d: %s: skip entry with mc/bc address %s\n",
				wlc_hw->unit, __FUNCTION__, bcm_ether_ntoa(&eaddr, eabuf)));
			continue;
		}
		/* Look for sta's that are associated with the AP, TDLS peers or IBSS peers. */
		scb = NULL;
		FOREACH_BSS(wlc, idx, cfg) {
			if ((BSSCFG_AP(cfg) && cfg->up) || BSS_TDLS_ENAB(wlc, cfg) ||
				BSSCFG_IBSS(cfg)) {
				scb = wlc_scbfind(wlc, cfg, &eaddr);
				if (scb != NULL)
					break;
			}
		}

		if (!scb) {
			if (!AUXPMQ_ENAB(wlc->pub) && (wlc->ps_scb_txfifo_blk == FALSE)) {
				wlc_pmq_clearpmq(pmq);
			}
			continue;
		}

		if (pmqdata & PMQH_DASAT) {
		        ps_disassoc_deauth = PS_SWITCH_DISASSOC_DEAUTH;
		} else {
			ps_on = (pmqdata & PMQH_PMON) ? PS_SWITCH_PMQ_ENTRY : PS_SWITCH_OFF;
			ps_pretend = (pmqdata & PMQH_PMPS) ?
				PS_SWITCH_PMQ_PSPRETEND : PS_SWITCH_OFF;
			ps_omi = PS_SWITCH_OFF;
		}

		ps_on_rqstr = ps_on;
		ps_off_rqstr = (pmqdata & PMQH_PMOFF) ? PS_SWITCH_PMQ_ENTRY : 0;
		if ((pmqdata & PMQH_OMI)) {
			if (wlc_he_omi_pmq_code(wlc, scb,
				(pmqdata >> PMQH_OMI_RXNSS_SHIFT) & PMQH_OMI_RXNSS_MASK,
				(pmqdata >> PMQH_OMI_BW_SHIFT) & PMQH_OMI_BW_MASK)) {
				ps_omi = PS_SWITCH_OMI;
				ps_on_rqstr |= PS_SWITCH_OMI;
			} else {
				/* rx htc is done already so we are switching PM off */
				ps_off_rqstr |= PS_SWITCH_OMI;
			}
		}
		if (ps_on_rqstr || ps_off_rqstr) {
			wlc_apps_ps_requester(wlc, scb, ps_on_rqstr, ps_off_rqstr);
		}

		wlc_apps_process_ps_switch(wlc, scb, ps_on | ps_pretend |
			ps_omi | ps_disassoc_deauth);

		if (ps_on) pmq->ps_on_count++;
		if (ps_pretend) pmq->ps_pretend_count++;
		if (ps_omi) pmq->ps_omi_count++;

		/* After the entry in Main PMQ has been processed and aux PMQ is in use and not
		 * full, the entry can/should be removed immediately.
		 */
		if (AUXPMQ_ENAB(wlc->pub) && !pmq->auxpmq_full) {
			wlc_pmq_clearpmq(pmq);
			cleared = TRUE;
		}
	}
	if (AUXPMQ_ENAB(wlc->pub) && !pmq->auxpmq_full && !cleared) {
		wlc_pmq_clearpmq(pmq);
	}

	return FALSE;
} /* wlc_pmq_processpmq */
