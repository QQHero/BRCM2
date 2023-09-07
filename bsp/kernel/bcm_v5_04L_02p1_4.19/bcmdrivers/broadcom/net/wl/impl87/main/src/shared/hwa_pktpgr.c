/*
 * HWA Packet Pager Library routines
 * - response ring handlers
 * - attach, config, preinit, init, status and debug dump
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

#include <typedefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <bcmpcie.h>
#include <bcmmsgbuf.h>
#include <hwa_lib.h>
#include <bcmhme.h>
#include <wlc_cfg.h>
#include <bcm_buzzz.h>
#ifdef HNDPQP
#include <hnd_pqp.h>
#endif
#include <sbtopcie.h> // sbtopcie_print()

#if defined(HWA_PKTPGR_BUILD)

/**
 * ---------------------------------------------------------------------------+
 * XXX Miscellaneous Packet Pager related registers|fields in other blocks:
 *
 * Block : Registers with Packet Pager related fields.
 * ----- : -------------------------------------------------------------------+
 * common: module_clk_enable, module_clkgating_enable, module_enable,
 *         module_reset, module_clkavail, module_idle
 *         dmatxsel, dmarxsel, hwa2hwcap, pageinstatus, pageinmask
 * rx    : rxpsrc_ring_hwa2cfg,
 *         d11bdest_threshold_l1l0, d11bdest_threshold_l2,
 *         hwa_rx_d11bdest_ring_wrindex_dir,
 *         pagein_status, recycle_status
 * txdma : pp_pagein_cfg, pp_pageout_cfg, pp_pagein_sts, pp_pageout_sts
 *
 * ---------------------------------------------------------------------------+
 */

/**
 * +--------------------------------------------------------------------------+
 *  Section: Handlers invoked to process Packet Pager Responses
 *  Parameter "hwa_pp_cmd_t *" is treated as a const pointer.
 * +--------------------------------------------------------------------------+
 */
// Forward declarations of all response ring handlers
#define HWA_PKTPGR_HANDLER_DECLARE(resp) \
static int hwa_pktpgr_ ## resp(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)

	                                            // PAGEIN response handlers
HWA_PKTPGR_HANDLER_DECLARE(pagein_rxprocess);   //  HWA_PKTPGR_PAGEIN_RXPROCESS
HWA_PKTPGR_HANDLER_DECLARE(pagein_txstatus);    //  HWA_PKTPGR_PAGEIN_TXSTATUS
HWA_PKTPGR_HANDLER_DECLARE(pagein_txpost);      //  HWA_PKTPGR_PAGEIN_TXPOST
	                                            // PAGEOUT response handlers
HWA_PKTPGR_HANDLER_DECLARE(pageout_pktlist);    //  HWA_PKTPGR_PAGEOUT_PKTLIST
//HWA_PKTPGR_HANDLER_DECLARE(pageout_local);      //  HWA_PKTPGR_PAGEOUT_LOCAL
	                                            // PAGEMGR response handlers
HWA_PKTPGR_HANDLER_DECLARE(pagemgr_alloc_rx);   //  HWA_PKTPGR_PAGEMGR_ALLOC_RX
HWA_PKTPGR_HANDLER_DECLARE(pagemgr_alloc_rx_rph); //  HWA_PKTPGR_PAGEMGR_ALLOC_RX_RPH
HWA_PKTPGR_HANDLER_DECLARE(pagemgr_alloc_tx);   //  HWA_PKTPGR_PAGEMGR_ALLOC_TX
HWA_PKTPGR_HANDLER_DECLARE(pagemgr_push);       //  HWA_PKTPGR_PAGEMGR_PUSH
HWA_PKTPGR_HANDLER_DECLARE(pagemgr_pull);       //  HWA_PKTPGR_PAGEMGR_PULL

#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
static inline void _hwa_ddbm_audit(hwa_pktpgr_t *pktpgr, uint32 bufidx, bool dir_tx,
	const bool alloc, const char *func, uint32 line);
static inline void _hwa_hdbm_audit(hwa_pktpgr_t *pktpgr, uint32 pkt_mapid, bool dir_tx,
	const bool alloc, const char *func, uint32 line);
#endif

static void hwa_pktpgr_txfifo_shadow_pull_pkts(hwa_dev_t *dev, uint16 pkt_mapid,
	uint16 pull_mpdu_cnt);

#ifdef HWA_QT_TEST
static void hwa_rxpath_pagemgr_alloc_rx_rsp(hwa_dev_t *dev, uint8 pktpgr_trans_id, int pkt_count,
	hwa_pp_lbuf_t * pktlist_head, hwa_pp_lbuf_t * pktlist_tail);

static bool g_pktpgr_req_commit_delay = FALSE;
#endif

static void hwa_pktpgr_req_wait_to_finish(hwa_dev_t *dev, int16 *req_cnt_p,
	hwa_pktpgr_ring_t pp_ring, const char *who, hwa_callback_t callback, uint8 cmdtype);

#ifdef PSPL_TX_TEST
static void
hwa_pktpgr_pspl_test_push_rsp(hwa_dev_t *dev, uint8 trans, uint8 pktpgr_trans_id,
	uint16 pkt_count, uint16 pkt_mapid, uint16 mpdu_count,
	hwa_pp_lbuf_t *pktlist_head, hwa_pp_lbuf_t *pktlist_tail)
{
	uint16 fifo_idx;
	uint32 elem_ix;
	int trans_id;
	hwa_pp_cmd_t req_cmd;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pspl_req_info_t *pspl_req_info;

	// Setup locals
	pktpgr = &dev->pktpgr;

	// TEST: TxPost-->Push-->Pull-->TxFIFO
	// HWA_PP_PAGEOUT_HDBM_PKTLIST_WR test: before HWA_PP_PAGEMGR_PUSH_MPDU and
	// HWA_PP_PAGEMGR_PULL_MPDU are ready, let's just limit 4-in-1 AMSDU to use
	// HWA_PP_PAGEMGR_PUSH so that we don't need more SW WAR to fixup next pointer.
	HWA_ASSERT(pktpgr->pspl_enable);

	// NOTE: pktlist_head and pktlist_tail aress space are in HDBM.
	HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  <<PAGEMGR::RSP PUSH%s : pkt %3u "
		"list[%p(%d) .. %p] <==PUSH%s-RSP(%d)==\n\n",
		(trans == HWA_PP_PAGEMGR_PUSH_MPDU) ? "_MPDU" : "", pkt_count,
		pktlist_head, pkt_mapid, pktlist_tail,
		(trans == HWA_PP_PAGEMGR_PUSH_MPDU) ? "_MPDU" : "", pktpgr_trans_id);

	// Get pspl_req_info from PUSH req.
	elem_ix = bcm_ring_cons(&pktpgr->pspl_state, pktpgr->pspl_depth);
	HWA_ASSERT(elem_ix != BCM_RING_EMPTY);
	pspl_req_info = BCM_RING_ELEM(pktpgr->pspl_table, elem_ix,
		sizeof(hwa_pp_pspl_req_info_t));
	fifo_idx = pspl_req_info->fifo_idx;
	HWA_ASSERT(pkt_count == pspl_req_info->pkt_count);
	if (trans == HWA_PP_PAGEMGR_PUSH_MPDU) {
		if (pkt_mapid != pspl_req_info->pkt_mapid) {
			HWA_ERROR(("pkt_mapid: 0x%x / 0x%x\n", pkt_mapid,
				pspl_req_info->pkt_mapid));
		}

		// XXX, CRBCAHWA-669: WAR for DB:0721
		pkt_mapid = pspl_req_info->pkt_mapid;
		HWA_ASSERT(mpdu_count == pspl_req_info->mpdu_count);
	} else {
		HWA_ASSERT(pkt_mapid == pspl_req_info->pkt_mapid);
		mpdu_count = pspl_req_info->mpdu_count;
	}

	// TEST: TxPost-->Push-->PGO_HDBM-->TxFIFO
	if (pktpgr->pspl_pgo_mpdu_enable) {
		// Transmit TxLfrags directly from HDBM: *WithResponse
		// Must use HWA_PP_PAGEOUT_HDBM_PKTLIST_WR if SW Tx/AQM
		// descriptor accounting is enabled.
		req_cmd.pageout_req_pktlist.trans        = HWA_PP_PAGEOUT_HDBM_PKTLIST_WR;
		req_cmd.pageout_req_pktlist.fifo_idx     = (uint8)fifo_idx;
		req_cmd.pageout_req_pktlist.zero         = 0;
		req_cmd.pageout_req_pktlist.mpdu_count   = mpdu_count;
		req_cmd.pageout_req_pktlist.pkt_count    = pkt_count;
		req_cmd.pageout_req_pktlist.pktlist_head = (uint32)pktlist_head;
		req_cmd.pageout_req_pktlist.pktlist_tail = (uint32)pktlist_tail;
		if (HWAREV_LE(dev->corerev, 132)) {
			HWA_ASSERT(pkt_count <= HWA_PKTPGR_PAGEOUT_PKTCOUNT_MAX);
			req_cmd.pageout_req_pktlist.pkt_count_copy = (uint8)pkt_count;
		}
		// XXX, CRBCAHWA-662
		if (HWAREV_GE(dev->corerev, 133)) {
			req_cmd.pageout_req_pktlist.swar = 0xBF;
		}

		// NOTE: pktlist_head and pktlist_tail aress space are in HDBM, cannot dump them.
		trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pageout_req_ring, &req_cmd);
		if (trans_id == HWA_FAILURE) {
			HWA_ERROR((">>PAGEOUT::REQ HDBM_PKTLIST_WR failure: pkts<%d> "
				"list[%p .. %p] pkt_mapid %3u", pkt_count,
				pktlist_head, pktlist_tail, pkt_mapid));
			HWA_ASSERT(0);
		}

		HWA_PP_DBG(HWA_PP_DBG_3B, "  >>PAGEOUT::REQ HDBM_PKTLIST_WR pkts %3u/%3u "
			"list[0x%p(%d) .. 0x%p] fifo %3u ==HDBM_PKTLIST_WR(%d)==>\n\n",
			pkt_count, mpdu_count, pktlist_head, pkt_mapid, pktlist_tail,
			fifo_idx, pktpgr_trans_id);
	} else {
		if (pktpgr->pspl_mpdu_enable) { // XXX, CRBCAHWA-669
			if (pktpgr->pspl_mpdu_append_count) {
				pktpgr->pspl_mpdu_append_count--;
				if (pktpgr->pspl_mpdu_append_count) {
					HWA_ERROR(("     Ignore PUSH_MPDU resp: "
						"pspl_mpdu_append_count = %d\n",
						pktpgr->pspl_mpdu_append_count));
					return; // ignore first time
				} else {
					HWA_ERROR(("     Trigger PULL_MPDU req: "
						"pspl_mpdu_append_count = %d\n",
						pktpgr->pspl_mpdu_append_count));
					// PULL full link
					pkt_count = pktpgr->pspl_pkt_count;
					mpdu_count = pktpgr->pspl_mpdu_count;
					pkt_mapid = pktpgr->pspl_mpdu_pkt_mapid_h;

					// Clear temp variables
					pktpgr->pspl_mpdu_count = 0;
					pktpgr->pspl_pkt_count = 0;
				}
			}

			// PULL MPDU Request
			req_cmd.pagemgr_req_pull.trans        = HWA_PP_PAGEMGR_PULL_MPDU;
			req_cmd.pagemgr_req_pull.mpdu_count   = mpdu_count;
		} else {
			// PULL Request
			req_cmd.pagemgr_req_pull.trans        = HWA_PP_PAGEMGR_PULL;
			req_cmd.pagemgr_req_pull.pkt_count    = pkt_count;
		}
		req_cmd.pagemgr_req_pull.pkt_mapid    = pkt_mapid;
		req_cmd.pagemgr_req_pull.tagged       = HWA_PP_CMD_NOT_TAGGED;
		// XXX, CRBCAHWA-662
		if (HWAREV_GE(dev->corerev, 133)) {
			req_cmd.pagemgr_req_pull.swar = 0xBFBF;
		}

		// NOTE: pktlist_head and pktlist_tail aress space are in HDBM, cannot dump them.
		trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req_cmd);
		if (trans_id == HWA_FAILURE) {
			HWA_ERROR((">>PAGEMGR::REQ PULL failure: pkts<%d> "
				"list[%p .. %p] pkt_mapid %3u", pkt_count,
				pktlist_head, pktlist_tail, pkt_mapid));
			HWA_ASSERT(0);
		}

		// Qeue fifo_idx and pkt_mapid for PULL rsp.
		elem_ix = bcm_ring_prod(&pktpgr->pspl_state, pktpgr->pspl_depth);
		pspl_req_info = BCM_RING_ELEM(pktpgr->pspl_table, elem_ix,
			sizeof(hwa_pp_pspl_req_info_t));
		pspl_req_info->fifo_idx = fifo_idx;
		pspl_req_info->pkt_mapid = pkt_mapid;
		pspl_req_info->mpdu_count = mpdu_count;
		pspl_req_info->pkt_count = pkt_count;

		HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  >>PAGEMGR::REQ PULL%s pkts %3u/%3u fifo %3u "
			"pkt_mapid %3u ==PULL%s(%d)==>\n\n",
			(pktpgr->pspl_mpdu_enable) ? "_MPDU" : "",
			pkt_count, mpdu_count, fifo_idx, pkt_mapid,
			(pktpgr->pspl_mpdu_enable) ? "_MPDU" : "",
			pktpgr_trans_id);
	}
}

static void
hwa_pktpgr_pspl_test_pull_rsp(hwa_dev_t *dev, uint8 trans, uint8 pktpgr_trans_id,
	uint16 pkt_count, uint16 mpdu_count, hwa_pp_lbuf_t *pktlist_head,
	hwa_pp_lbuf_t *pktlist_tail)
{
	uint16 fifo_idx, mpdu;
	uint32 elem_ix;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pspl_req_info_t *pspl_req_info;
	hwa_pp_lbuf_t *txlbuf, *txlbuf_link, *tx_head, *tx_tail;

	// Setup locals
	pktpgr = &dev->pktpgr;

	// TEST: TxPost-->Push-->Pull-->TxFIFO
	HWA_ASSERT(pktpgr->pspl_enable);

	HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  <<PAGEMGR::RSP PULL%s : pkt %3u/%3u "
		"list[%p(%d) .. %p(%d)] <==PULL%s-RSP(%d)==\n\n",
		(trans == HWA_PP_PAGEMGR_PULL_MPDU) ? "_MPDU" : "",
		pkt_count, mpdu_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail),
		(trans == HWA_PP_PAGEMGR_PULL_MPDU) ? "_MPDU" : "",
		pktpgr_trans_id);

	// Get pspl_req_info from PULL req.
	elem_ix = bcm_ring_cons(&pktpgr->pspl_state, pktpgr->pspl_depth);
	HWA_ASSERT(elem_ix != BCM_RING_EMPTY);
	pspl_req_info = BCM_RING_ELEM(pktpgr->pspl_table, elem_ix,
		sizeof(hwa_pp_pspl_req_info_t));
	fifo_idx = pspl_req_info->fifo_idx;
	HWA_ASSERT(pspl_req_info->pkt_mapid == PKTMAPID(pktlist_head));
	HWA_ASSERT(pkt_count == pspl_req_info->pkt_count);
	if (trans == HWA_PP_PAGEMGR_PULL_MPDU) {
		HWA_ASSERT(mpdu_count == pspl_req_info->mpdu_count);
	} else {
		mpdu_count = pspl_req_info->mpdu_count;
	}

	txlbuf = pktlist_head;
	tx_head = NULL;
	tx_tail = NULL;
	mpdu = 0;
	// XXX, CRBCAHWA-669, buf in BD:0723
	while (txlbuf != NULL) {
		mpdu++;

		txlbuf_link = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
		PKTSETLINK(txlbuf, NULL);

		if (mpdu == mpdu_count) {
			// XXX, pktlist_tail can be the second TxLfrag of a 8-in-1 AMSDU
			// so txlbuf == pktlist_tail is not TRUE.  Check if the tail is the last
			// next of last MPDU.
			// HWA_ASSERT(txlbuf == pktlist_tail);
			// Make sure the txlbuf is last mpdu and pktlist_tail is last next of txlbuf
			if (txlbuf != pktlist_tail) {
				hwa_pp_lbuf_t *pkt = txlbuf;
				HWA_ASSERT(pktlist_tail != NULL);
				while (pkt) {
					pkt = (hwa_pp_lbuf_t *)PKTNEXT(dev->osh, pkt);
					if (pkt == pktlist_tail) {
						// Reassign the pktlist_tail to the real last mpdu
						HWA_ERROR(("PULL: fixup pktlist_tail %p / %p\n",
							pktlist_tail, txlbuf));
						pktlist_tail = txlbuf;
						break;
					}
				}

				if (pkt == NULL) {
					HWA_ERROR(("PULL: txlbuf %p pktlist_tail %p\n",
						txlbuf, pktlist_tail));
					//HWA_ASSERT(txlbuf == pktlist_tail);
				}
			}
		}

		if (tx_tail == NULL) {
			tx_head = tx_tail = txlbuf;
		} else {
			PKTSETLINK(tx_tail, txlbuf);
			tx_tail = txlbuf;
		}

		txlbuf = txlbuf_link;
	}

	pktlist_head = tx_head;

	// Tx it.
	hwa_txfifo_pktchain_xmit_request_pspl_test_pull(dev, pktpgr_trans_id, fifo_idx,
		mpdu_count, pkt_count, pktlist_head, pktlist_tail);
}

static void // pspl test push request info save
_hwa_pktpgr_pspl_test_push_req_enq(hwa_dev_t *dev, int trans_id, uint32 fifo_idx,
	void *pktlist_head, void *pktlist_tail, uint16 pkt_count, uint16 mpdu_count,
	uint16 prev_pkt_mapid, hwa_pp_pagemgr_cmd_t pagemgr_cmd)
{
	int elem_ix;
	hwa_pp_pspl_req_info_t *pspl_req_info;

	// TEST: TxPost-->Push-->Pull-->TxFIFO
	HWA_ASSERT(dev->pktpgr.pspl_enable);
	HWA_ASSERT(pagemgr_cmd == HWA_PP_PAGEMGR_PUSH ||
		pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_MPDU);

	// Qeue fifo_idx and pkt_mapid for PUSH rsp.
	elem_ix = bcm_ring_prod(&dev->pktpgr.pspl_state, dev->pktpgr.pspl_depth);
	pspl_req_info = BCM_RING_ELEM(dev->pktpgr.pspl_table, elem_ix,
		sizeof(hwa_pp_pspl_req_info_t));
	pspl_req_info->fifo_idx = fifo_idx;
	pspl_req_info->pkt_mapid = PKTMAPID(pktlist_head);
	pspl_req_info->mpdu_count = mpdu_count;
	pspl_req_info->pkt_count = pkt_count;

	HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  >>PAGEMGR::REQ PUSH%s pkts %3u/%3u "
		"list[0x%p(%d) .. 0x%p(%d)] fifo %3u ==PUSH%s-REQ(%d)==>\n\n",
		(pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_MPDU) ? "_MPDU" : "",
		pkt_count, mpdu_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), fifo_idx,
		(pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_MPDU) ? "_MPDU" : "",
		trans_id);
}
#endif /* PSPL_TX_TEST */

// NOTE: pktlist_head and pktlist_tail address space are in HDBM.
// In PUSH req the packet in DDBM will be freed by HW.
static void
hwa_pktpgr_push_rsp(hwa_dev_t *dev, uint8 trans, uint8 pktpgr_trans_id,
	uint8 tagged, uint16 pkt_count, uint16 pkt_mapid, uint16 mpdu_count,
	hwa_pp_lbuf_t *pktlist_head, hwa_pp_lbuf_t *pktlist_tail)
{
	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

#ifdef PSPL_TX_TEST
	if (tagged == HWA_PP_CMD_NOT_TAGGED) {
		hwa_pktpgr_pspl_test_push_rsp(dev, trans, pktpgr_trans_id,
			pkt_count, pkt_mapid, mpdu_count, pktlist_head, pktlist_tail);
	} else
#endif /* PSPL_TX_TEST */
	{
		hwa_pktpgr_t *pktpgr;

		// Setup locals
		pktpgr = &dev->pktpgr;

		// DDBM accounting.
		HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, pkt_count);
		HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_tx, pkt_count);
		HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: + %3u : %d @ %s\n", pkt_count,
			pktpgr->ddbm_avail_sw, __FUNCTION__);

		// Special tagged request process
		HWA_ASSERT(tagged == HWA_PP_CMD_TAGGED);

		HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  <<PAGEMGR::RSP PUSH(tagged) : pkt %3u "
			"list[%p(%d) .. %p] <==PUSH-RSP(%d)==\n\n",
			pkt_count, pktlist_head, pkt_mapid, pktlist_tail, pktpgr_trans_id);
	}
}

static void
hwa_pktpgr_pull_rsp(hwa_dev_t *dev, uint8 trans, uint8 pktpgr_trans_id,
	uint8 tagged, uint16 pkt_count, uint16 mpdu_count,
	hwa_pp_lbuf_t *pktlist_head, hwa_pp_lbuf_t *pktlist_tail)
{
	hwa_pktpgr_t *pktpgr;
	uint16 mpdu;
	hwa_pp_lbuf_t *txlbuf;
	void *next;
	struct spktq temp_q;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	if (pkt_count == 0) {
		HWA_ERROR(("%s: pkt_count is 0\n", __FUNCTION__));
		return;
	}

#ifdef PSPL_TX_TEST
	if (tagged == HWA_PP_CMD_NOT_TAGGED) {
		hwa_pktpgr_pspl_test_pull_rsp(dev, trans, pktpgr_trans_id,
			pkt_count, mpdu_count, pktlist_head, pktlist_tail);
	} else
#endif
	// Special tagged request process
	HWA_ASSERT(tagged == HWA_PP_CMD_TAGGED);

	// Setup locals
	pktpgr = &dev->pktpgr;
	txlbuf = pktlist_head;
	mpdu = 0;

	while (txlbuf != NULL) {
		mpdu++;

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
		// Alloc: PULL case.
		hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_2DBM,
			DBM_AUDIT_ALLOC);
#endif

		// HWA_PP_PAGEMGR_PULL will not pull PKTNEXT, skip the audit.
		if (trans == HWA_PP_PAGEMGR_PULL) {
			goto skip_next;
		}

		next = PKTNEXT(dev->osh, txlbuf);
		for (; next; next = PKTNEXT(dev->osh, next)) {
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
			// Alloc: PULL case.
			hwa_pktpgr_dbm_audit(dev, next, DBM_AUDIT_TX, DBM_AUDIT_2DBM,
				DBM_AUDIT_ALLOC);
#endif
		}

		if (mpdu == mpdu_count) {
			// XXX, pktlist_tail can be the second TxLfrag of a 8-in-1 AMSDU
			// so txlbuf == pktlist_tail is not TRUE.  Check if the tail is the last
			// next of last MPDU.
			// HWA_ASSERT(txlbuf == pktlist_tail);
			// Make sure the txlbuf is last mpdu and pktlist_tail is last next of txlbuf
			if (txlbuf != pktlist_tail) {
				hwa_pp_lbuf_t *pkt = txlbuf;
				HWA_ASSERT(pktlist_tail != NULL);
				while (pkt) {
					pkt = (hwa_pp_lbuf_t *)PKTNEXT(dev->osh, pkt);
					if (pkt == pktlist_tail) {
						// Reassign the pktlist_tail to the real last mpdu
						pktlist_tail = txlbuf;
						break;
					}
				}

				if (pkt == NULL) {
					HWA_ERROR(("PULL: txlbuf %p pktlist_tail %p\n",
						txlbuf, pktlist_tail));
					HWA_ASSERT(txlbuf == pktlist_tail);
				}
			}
		}
skip_next:
		txlbuf = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
	}

	// Init the temporary q
	spktq_init_list(&temp_q, PKTQ_LEN_MAX, pktlist_head, pktlist_tail, mpdu_count);
	// Add to tag_pull_q
	spktq_enq_list(&pktpgr->tag_pull_q, &temp_q);
	// Deinit the temporary queue
	spktq_deinit(&temp_q);
}

int // HWA_PKTPGR_PAGEIN_RXPROCESS handler
hwa_pktpgr_pagein_rxprocess(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pagein_rsp_rxprocess_t *rxprocess;

	HWA_FTRACE(HWApp);

	// Setup locals
	pktpgr = &dev->pktpgr;
	rxprocess = &pp_cmd->pagein_rsp_rxprocess;
	HWA_ASSERT(rxprocess->trans == HWA_PP_PAGEIN_RXPROCESS);

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133) && rxprocess->swar != 0xBFBF) {
		HWA_PRINT(" --> ERR: %s(%d): swar 0x%x\n", __FUNCTION__,
			__LINE__, rxprocess->swar);
	}
#endif

	// Accounting
	HWA_COUNT_DEC(pktpgr->pgi_rxpkt_req, 1);
	HWA_COUNT_DEC(pktpgr->pgi_rxpkt_in_transit, rxprocess->pkt_count);

	// process pktlist
	hwa_rxfill_pagein_rx_rsp(dev,
		rxprocess->trans_id, rxprocess->pkt_count,
		(hwa_pp_lbuf_t*)rxprocess->pktlist_head,
		(hwa_pp_lbuf_t*)rxprocess->pktlist_tail);

	return rxprocess->trans_id;

}   // hwa_pktpgr_pagein_rxprocess()

int // HWA_PKTPGR_PAGEIN_TXSTATUS handler
hwa_pktpgr_pagein_txstatus(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pagein_rsp_txstatus_t *txstatus;

	HWA_FTRACE(HWApp);

	// Setup locals
	pktpgr = &dev->pktpgr;
	txstatus = &pp_cmd->pagein_rsp_txstatus;
	HWA_ASSERT(txstatus->trans == HWA_PP_PAGEIN_TXSTATUS);
	HWA_ASSERT(txstatus->fifo_idx < HWA_TX_FIFOS_MAX);

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133) &&
		(txstatus->swar != 0xBF && txstatus->swar != 0xFB)) {
		HWA_PRINT(" --> ERR: %s(%d): swar 0x%x\n", __FUNCTION__,
			__LINE__, txstatus->swar);
	}
#endif

	// Assume one pagein TxS req map to one pagein TxS rsp.
	HWA_COUNT_DEC(pktpgr->pgi_txs_req[txstatus->fifo_idx], 1);
	HWA_COUNT_DEC(pktpgr->pgi_txs_req_tot, 1);

#ifdef HWA_QT_TEST
	/* XXX, CRBCAHWA-663: Duplicate pkt_mapid in PGI TXS Resp,
	 * this is happened after we got a ZERO PGI TXS RESP.
	 */
	if (HWAREV_GE(dev->corerev, 133) && txstatus->swar == 0xFB) {
		HWA_PRINT(" --> INFO: %s(%d): mpdu_count %d pkt_count %d\n",
			__FUNCTION__, __LINE__, txstatus->mpdu_count, txstatus->pkt_count);
		// Add over counting delta
		HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, 2);
		HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_tx, 2);
		goto done;
	}
#endif

	// process pktlist
	hwa_txstat_pagein_rsp(dev, txstatus->trans_id,
		txstatus->tagged, txstatus->fifo_idx,
		txstatus->mpdu_count, txstatus->pkt_count,
		(hwa_pp_lbuf_t*)txstatus->pktlist_head,
		(hwa_pp_lbuf_t*)txstatus->pktlist_tail);

#ifdef HWA_QT_TEST
done:
#endif

	return txstatus->trans_id;

}   // hwa_pktpgr_pagein_txstatus()

int // HWA_PKTPGR_PAGEIN_TXPOST handler
hwa_pktpgr_pagein_txpost(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	hwa_pp_pagein_rsp_txpost_t *txpost = &pp_cmd->pagein_rsp_txpost;

	HWA_FTRACE(HWApp);
	HWA_ASSERT((txpost->trans == HWA_PP_PAGEIN_TXPOST_WITEMS) ||
	           (txpost->trans == HWA_PP_PAGEIN_TXPOST_WITEMS_FRC));

	// process pktlist
	hwa_txpost_pagein_rsp(dev, txpost->trans_id,
		txpost->pkt_count, txpost->oct_count,
		(hwa_pp_lbuf_t*)txpost->pktlist_head,
		(hwa_pp_lbuf_t*)txpost->pktlist_tail);

	return txpost->trans_id;

}   // hwa_pktpgr_pagein_txpost()

int // HWA_PKTPGR_PAGEOUT_PKTLIST, HWA_PKTPGR_PAGEOUT_LOCAL handler
hwa_pktpgr_pageout_pktlist(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	uint16 pkt_count;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pageout_rsp_pktlist_t *pktlist;
	HWA_PP_DBG_EXPR(char *who);

	HWA_FTRACE(HWApp);

	// Setup locals
	pktpgr = &dev->pktpgr;
	pktlist = &pp_cmd->pageout_rsp_pktlist;

	HWA_ASSERT(pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_WR ||
		pktlist->trans == HWA_PP_PAGEOUT_PKTLOCAL ||
		pktlist->trans == HWA_PP_PAGEOUT_HDBM_PKTLIST_WR);

	if (HWAREV_LE(dev->corerev, 132))
		pkt_count = pktlist->pkt_count_copy;
	else
		pkt_count = pktlist->pkt_count; // pkt_count is 0 in HWA_PP_PAGEOUT_PKTLOCAL Resp

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662.
	if (HWAREV_GE(dev->corerev, 133) && (pktlist->swar != 0xBF)) {
		HWA_PRINT(" --> ERR: %s(%d): PAGEOUT_HDBM_PKTLIST swar 0x%x\n",
			__FUNCTION__, __LINE__, pktlist->swar);
	}
#endif

	// PKTLOCAL specific
	if (pktlist->trans == HWA_PP_PAGEOUT_PKTLOCAL) {
		HWA_PP_DBG_EXPR(who = "TXLOCAL");
		// Accounting
		HWA_COUNT_DEC(pktpgr->pgo_local_req, 1);
	} else {

#ifdef HWA_QT_TEST
		if (pktlist->trans == HWA_PP_PAGEOUT_HDBM_PKTLIST_WR &&
			pktlist->mpdu_count >= 2) {
			hwa_kflag |= HWA_KFLAG_664; // XXX, CRBCAHWA-664
		}
#endif

		HWA_PP_DBG_EXPR({
			if (pktlist->trans == HWA_PP_PAGEOUT_HDBM_PKTLIST_WR) {
				who = "HDBM_TXPKTLIST";
			} else {
				who = "TXPKTLIST";
			}
		});

		// Accounting
		if (pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_WR) {
			HWA_COUNT_DEC(pktpgr->pgo_pktlist_req, 1);
		}

		// DDBM accounting
		HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, pkt_count);
		HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_tx, pkt_count);
		HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: + %3u : %d @ %s\n", pkt_count,
			pktpgr->ddbm_avail_sw, __FUNCTION__);

#ifdef HNDPQP
		// Try to refill pqp packet pool
		hwa_pktpgr_pqplbufpool_fill(dev);
#endif /* HNDPQP */
	}

	HWA_PP_DBG(HWA_PP_DBG_3B, "  <<PAGEOUT::RSP %s : mpdu %3u txdma_desc %3u "
		"pkt @ 0x%p fifo %3u <==%s-RSP(%d)==\n\n", who, pktlist->mpdu_count,
		pktlist->total_txdma_desc, (hwa_pp_lbuf_t*)pktlist->pkt_local,
		pktlist->fifo_idx, who, pktlist->trans_id);

	return pktlist->trans_id;

}   // hwa_pktpgr_pageout_pktlist()

int // HWA_PKTPGR_PAGEMGR_ALLOC_RX handler
hwa_pktpgr_pagemgr_alloc_rx(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	hwa_pp_pagemgr_rsp_alloc_t *alloc_rx = &pp_cmd->pagemgr_rsp_alloc;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(alloc_rx->trans == HWA_PP_PAGEMGR_ALLOC_RX);

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133) && alloc_rx->swar != 0xBFBFBFBF) {
		HWA_PRINT(" --> ERR: %s(%d): swar 0x%x\n", __FUNCTION__,
			__LINE__, alloc_rx->swar);
	}

	// Use allocated rxlfrags
	hwa_rxpath_pagemgr_alloc_rx_rsp(dev,
		alloc_rx->trans_id, alloc_rx->pkt_count,
		(hwa_pp_lbuf_t*)alloc_rx->pktlist_head,
		(hwa_pp_lbuf_t*)alloc_rx->pktlist_tail);
#endif

	return alloc_rx->trans_id;

}   // hwa_pktpgr_pagemgr_alloc_rx()

// XXX NOTE: We only need RPH.  Rx packet is useless.
int // HWA_PKTPGR_PAGEMGR_ALLOC_RX_RPH handler
hwa_pktpgr_pagemgr_alloc_rx_rph(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	hwa_pp_pagemgr_rsp_alloc_t *alloc_rph = &pp_cmd->pagemgr_rsp_alloc;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(alloc_rph->trans == HWA_PP_PAGEMGR_ALLOC_RX_RPH);

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133) && alloc_rph->swar != 0xBFBFBFBF) {
		HWA_PRINT(" --> ERR: %s(%d): swar 0x%x\n", __FUNCTION__,
			__LINE__, alloc_rph->swar);
	}
#endif

	// XXX --- use allocated rxpost host info
	hwa_rxpath_pagemgr_alloc_rx_rph_rsp(dev,
		alloc_rph->trans_id, alloc_rph->pkt_count,
		(hwa_pp_lbuf_t*)alloc_rph->pktlist_head,
		(hwa_pp_lbuf_t*)alloc_rph->pktlist_tail);

	return alloc_rph->trans_id;

}   // hwa_pktpgr_pagemgr_alloc_rx_rph()

int // HWA_PKTPGR_PAGEMGR_ALLOC_TX handler
hwa_pktpgr_pagemgr_alloc_tx(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	hwa_pp_pagemgr_rsp_alloc_t *alloc_tx = &pp_cmd->pagemgr_rsp_alloc;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(alloc_tx->trans == HWA_PP_PAGEMGR_ALLOC_TX);

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133) && alloc_tx->swar != 0xBFBFBFBF) {
		HWA_PRINT(" --> ERR: %s(%d): swar 0x%x\n", __FUNCTION__,
			__LINE__, alloc_tx->swar);
	}
#endif

	// process pktlist
	hwa_txpost_pagemgr_alloc_tx_rsp(dev,
		alloc_tx->trans_id, alloc_tx->tagged, alloc_tx->pkt_count,
		(hwa_pp_lbuf_t*)alloc_tx->pktlist_head,
		(hwa_pp_lbuf_t*)alloc_tx->pktlist_tail);

	return alloc_tx->trans_id;

}   // hwa_pktpgr_pagemgr_alloc_tx()

int // HWA_PKTPGR_PAGEMGR_PUSH handler
hwa_pktpgr_pagemgr_push(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	hwa_pp_pagemgr_rsp_push_t *push = &pp_cmd->pagemgr_rsp_push;

	HWA_FTRACE(HWApp);
	HWA_ASSERT((push->trans == HWA_PP_PAGEMGR_PUSH) ||
	           (push->trans == HWA_PP_PAGEMGR_PUSH_PKTTAG) ||
	           (push->trans == HWA_PP_PAGEMGR_PUSH_MPDU));

	if (push->tagged == HWA_PP_CMD_TAGGED) {
		HWA_COUNT_DEC(dev->pktpgr.tag_push_req, 1);
	}

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133) &&
		((push->trans == HWA_PP_PAGEMGR_PUSH) ||
	     (push->trans == HWA_PP_PAGEMGR_PUSH_PKTTAG)) &&
	    (push->swar != 0xBFBF)) {
		HWA_PRINT(" --> ERR: %s(%d): swar 0x%x\n", __FUNCTION__,
			__LINE__, push->swar);
	}
#endif

	if (push->trans < HWA_PP_PAGEMGR_PUSH_MPDU) {
		push->mpdu_count = push->pkt_count;
	}

	// NOTE: pktlist_head and pktlist_tail address space are in HDBM.
	hwa_pktpgr_push_rsp(dev, push->trans, push->trans_id, push->tagged,
		push->pkt_count, push->pkt_mapid, push->mpdu_count,
		(hwa_pp_lbuf_t*)push->pktlist_head, (hwa_pp_lbuf_t*)push->pktlist_tail);

	return push->trans_id;

}   // hwa_pktpgr_pagemgr_push()

int // HWA_PKTPGR_PAGEMGR_PULL handler
hwa_pktpgr_pagemgr_pull(void *ctx, hwa_dev_t *dev, hwa_pp_cmd_t *pp_cmd)
{
	hwa_pp_pagemgr_rsp_pull_t *pull = &pp_cmd->pagemgr_rsp_pull;

	HWA_FTRACE(HWApp);
	HWA_ASSERT((pull->trans == HWA_PP_PAGEMGR_PULL) ||
	           (pull->trans == HWA_PP_PAGEMGR_PULL_KPFL_LINK) ||
	           (pull->trans == HWA_PP_PAGEMGR_PULL_MPDU));

	if (pull->tagged == HWA_PP_CMD_TAGGED) {
		HWA_COUNT_DEC(dev->pktpgr.tag_pull_req, 1);
	}

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133) &&
		((pull->trans == HWA_PP_PAGEMGR_PULL) ||
	     (pull->trans == HWA_PP_PAGEMGR_PULL_KPFL_LINK)) &&
	     (pull->swar != 0xBFBF)) {
		HWA_PRINT(" --> ERR: %s(%d): swar 0x%x\n", __FUNCTION__,
			__LINE__, pull->swar);
	}
#endif

	if (pull->trans < HWA_PP_PAGEMGR_PULL_MPDU) {
		pull->mpdu_count = pull->pkt_count;
	}

	// process pull
	hwa_pktpgr_pull_rsp(dev, pull->trans, pull->trans_id,
		pull->tagged, pull->pkt_count, pull->mpdu_count,
		(hwa_pp_lbuf_t*)pull->pktlist_head,
		(hwa_pp_lbuf_t*)pull->pktlist_tail);

	return pull->trans_id;

}   // hwa_pktpgr_pagemgr_pull()

static INLINE int // Ring base, free a buffer to HDBM or DDBM base on its pkt_mapid/buf_idx
hwa_pktpgr_dbm_free_fast(hwa_dev_t *dev, hwa_dbm_instance_t dbm_instance, uint16 buf_idx, bool tx,
	const char *func, uint32 line)
{
	int trans_id, req_loop;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_freedbm_req_t req;

	HWA_FTRACE(HWApp);

	// Setup locals
	pktpgr = &dev->pktpgr;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(buf_idx < pktpgr->hostpktpool_max);

	HWA_TRACE(("%s DBM<%d> free buf_idx<%u>\n", HWApp, dbm_instance, buf_idx));

	req.trans        = (dbm_instance == HWA_HDBM) ?
		HWA_PP_PAGEMGR_FREE_HDBM : HWA_PP_PAGEMGR_FREE_DDBM;
	req.pkt_count    = 1;
	// buf_idx is pkt_mapid for HDBM. buf_idx is dbm index for DDBM
	req.pkt_mapid[0] = buf_idx;

	req_loop = HWA_LOOPCNT;

#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
	if (dbm_instance == HWA_HDBM)
		_hwa_hdbm_audit(pktpgr, buf_idx, tx, DBM_AUDIT_FREE, func, line);
	else
		_hwa_ddbm_audit(pktpgr, buf_idx, tx, DBM_AUDIT_FREE, func, line);
#endif

req_again:
	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_freepkt_req_ring, &req);
	if (trans_id == HWA_FAILURE) {
		if (--req_loop > 0) {
			OSL_DELAY(10);
			goto req_again;
		}
		HWA_ERROR((">>PAGEMGR::REQ FREE_%s failure: buf_idx<%u>\n",
			(dbm_instance == HWA_HDBM) ? "HDBM" : "DDBM", buf_idx));
		HWA_ASSERT(0);
		return HWA_FAILURE;
	}

	// DDBM accounting
	// XXX, be careful the latancy
	if (dbm_instance == HWA_DDBM) {
		HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, 1);
		if (tx)
			HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_tx, 1);
		else
			HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_rx, 1);
		HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: + %3u : %d @ %s\n", 1,
			pktpgr->ddbm_avail_sw, __FUNCTION__);
	}

	HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  >>PAGEMGR::REQ FREE_%s      : buf_idx<%u> "
		"==FREE_%s-REQ(%d), no rsp ==>\n\n", (dbm_instance == HWA_HDBM) ? "HDBM" : "DDBM",
		buf_idx, (dbm_instance == HWA_HDBM) ? "HDBM" : "DDBM", trans_id);

	return HWA_SUCCESS;
} // hwa_pktpgr_dbm_free_fast

static INLINE int // Register base, free a buffer to HDBM or DDBM base on its buf_idx
__hwa_pktpgr_dbm_free(hwa_dev_t *dev, hwa_dbm_instance_t dbm_instance, uint16 buf_idx, bool tx,
	const char *func, uint32 line)
{
	uint32 u32, status, loopcnt;
	hwa_pktpgr_t *pktpgr;
	hwa_pager_regs_t *pager_regs;

	HWA_FTRACE(HWApp);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pager_regs = &dev->regs->pager;
	pktpgr = &dev->pktpgr;

	HWA_TRACE(("%s DBM<%d> free buf_idx<%u>\n", HWApp, dbm_instance, buf_idx));

#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
	if (dbm_instance == HWA_HDBM)
		_hwa_hdbm_audit(pktpgr, buf_idx, tx, DBM_AUDIT_FREE, func, line);
	else
		_hwa_ddbm_audit(pktpgr, buf_idx, tx, DBM_AUDIT_FREE, func, line);
#endif

	if (dbm_instance == HWA_HDBM) {
		HWA_ASSERT(buf_idx < pktpgr->hostpktpool_max);
		u32 = BCM_SBF(buf_idx, HWA_PAGER_PP_HOSTPKTPOOL_DEALLOC_INDEX_PPHDDEALLOCIDX);
		HWA_WR_REG_ADDR(HWApp, &pager_regs->hostpktpool.dealloc_index, u32);
	} else { // DDBM
		HWA_ASSERT(buf_idx < pktpgr->dnglpktpool_max);
		u32 = BCM_SBF(buf_idx, HWA_PAGER_PP_DNGLPKTPOOL_DEALLOC_INDEX_PPDDDEALLOCIDX);
		HWA_WR_REG_ADDR(HWApp, &pager_regs->dnglpktpool.dealloc_index, u32);
	}

	for (loopcnt = 0; loopcnt < HWA_BM_LOOPCNT; loopcnt++) {
		if (dbm_instance == HWA_HDBM) {
			u32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->hostpktpool.dealloc_status);
			status = BCM_GBF(u32,
				HWA_PAGER_PP_HOSTPKTPOOL_DEALLOC_STATUS_PPHDDEALLOCSTS);
		} else {
			u32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->dnglpktpool.dealloc_status);
			status = BCM_GBF(u32,
				HWA_PAGER_PP_DNGLPKTPOOL_DEALLOC_STATUS_PPDDDEALLOCSTS);
		}

		// WAR for HWA2.x
		if (status == HWA_BM_SUCCESS_SW) {
			if (dbm_instance == HWA_DDBM) {
				HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, 1);
				if (tx)
					HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_tx, 1);
				else
					HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_rx, 1);
				HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: + %3u : %d @ %s\n", 1,
					pktpgr->ddbm_avail_sw, __FUNCTION__);
			}
			return HWA_SUCCESS;
		}

		if (status & HWA_BM_DONEBIT) {
			HWA_ASSERT(status == HWA_BM_SUCCESS);
			if (dbm_instance == HWA_DDBM) {
				HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, 1);
				if (tx)
					HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_tx, 1);
				else
					HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_rx, 1);
				HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: + %3u : %d @ %s\n", 1,
					pktpgr->ddbm_avail_sw, __FUNCTION__);
			}
			return HWA_SUCCESS;
		}
	}

	HWA_WARN(("%s DBM<%d> free failure\n", HWApp, dbm_instance));

	HWA_ASSERT(0);

	return HWA_FAILURE;

} // hwa_pktpgr_dbm_free

int // Free a buffer to HDBM or DDBM base on its pkt_mapid/buf_idx
_hwa_pktpgr_dbm_free(hwa_dev_t *dev, hwa_dbm_instance_t dbm_instance, uint16 buf_idx, bool tx,
	const char *func, uint32 line)
{
	int ret = HWA_SUCCESS;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	if (HWAREV_LE(dev->corerev, 132)) {
		ret = __hwa_pktpgr_dbm_free(dev, dbm_instance, buf_idx, tx, func, line);
	} else {
		// buf_idx is pkt_mapid for HDBM. buf_idx is dbm index for DDBM
		ret = hwa_pktpgr_dbm_free_fast(dev, dbm_instance, buf_idx, tx, func, line);
	}

	return ret;
}

static uint16 // Get available count of HDBM or DDBM
hwa_pktpgr_dbm_availcnt(hwa_dev_t *dev, hwa_dbm_instance_t dbm_instance)
{
	uint16 availcnt;
	uint32 u32;
	hwa_pager_regs_t *pager_regs;

	HWA_FTRACE(HWApp);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pager_regs = &dev->regs->pager;
	availcnt = 0;

	if (dbm_instance == HWA_DDBM) {
		u32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->dnglpktpool.ctrl);
		availcnt = BCM_GBF(u32, HWA_PAGER_PP_DNGLPKTPOOL_CTRL_PPDDAVAILCNT);
		if (availcnt > HWA_PKTPGR_PAGEIN_REQ_DDBMTH)
			availcnt -= HWA_PKTPGR_PAGEIN_REQ_DDBMTH;
		else
			availcnt = 0;
	} else { // HDBM
		u32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->hostpktpool.ctrl);
		availcnt = BCM_GBF(u32, HWA_PAGER_PP_HOSTPKTPOOL_CTRL_PPHDAVAILCNT);
		if (availcnt > HWA_PKTPGR_PAGEIN_REQ_HDBMTH)
			availcnt -= HWA_PKTPGR_PAGEIN_REQ_HDBMTH;
		else
			availcnt = 0;
	}

	return availcnt;
}

void
hwa_pktpgr_print_pooluse(hwa_dev_t *dev)
{
	uint32 mem_sz;
	hwa_pktpgr_t *pktpgr = &dev->pktpgr;

	mem_sz = sizeof(hwa_pp_lbuf_t) * pktpgr->dnglpktpool_max;

	HWA_PRINT("\tIn use HDBM %d(%d) DDBM %d:(H:%d,S:%d)(%dK)\n",
		pktpgr->hostpktpool_max, hwa_pktpgr_dbm_availcnt(dev, HWA_HDBM),
		pktpgr->dnglpktpool_max, hwa_pktpgr_dbm_availcnt(dev, HWA_DDBM),
		pktpgr->ddbm_avail_sw, KB(mem_sz));
}

/**
 * +--------------------------------------------------------------------------+
 *  Generic Packet Pager consumer loop. Each element of the Response ring is
 *  processed by invoking the handler identified by the transaction command type
 *
 *  All elements from RD to WR index are processed, unbounded. Each response
 *  element is holding onto packets in dongle's packet storage, and needs to be
 *  promptly processed to completion.
 *  Bounding algorithms should be enforced during requests.
 *
 *  There are no response rings corresponding to Free pkt/rph/d11b requests.
 * +--------------------------------------------------------------------------+
 */
void
hwa_pktpgr_response(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring,
	hwa_callback_t callback)
{
	uint8          trans_cmd;
	uint32         elem_ix;         // location of next element to read
	hwa_pp_cmd_t  *rsp_cmd;         // pointer to command in h2s response ring
	hwa_handler_t *rsp_handler;     // response handler
	hwa_ring_t    *rsp_ring;        // hw to sw response ring
	hwa_pktpgr_t  *pktpgr;		// pktpgr local state

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	pktpgr = &dev->pktpgr;
	rsp_ring = pktpgr->rsp_ring[pp_ring];

	// Skip it if the response ring is processing by another DPC.
	if (isset(&pktpgr->rsp_ring_in_process, pp_ring)) {
		goto done;
	}

	setbit(&pktpgr->rsp_ring_in_process, pp_ring);

	hwa_ring_cons_get(rsp_ring);    // fetch response ring's WR index once

	while ((elem_ix = hwa_ring_cons_upd(rsp_ring)) != BCM_RING_EMPTY) {
		rsp_cmd     = HWA_RING_ELEM(hwa_pp_cmd_t, rsp_ring, elem_ix);
		trans_cmd   = rsp_cmd->u8[HWA_PP_CMD_TRANS];
		if (!BCM_TBIT(trans_cmd, HWA_PP_CMD_INVALID)) {
			rsp_handler = &dev->handlers[callback + trans_cmd];

			// Mark this response as invalid to avoid processing again.
			rsp_cmd->u8[HWA_PP_CMD_TRANS] |= BCM_SBIT(HWA_PP_CMD_INVALID);

			(*rsp_handler->callback)(rsp_handler->context, dev, rsp_cmd);
		}
	}

	hwa_ring_cons_put(rsp_ring); // commit RD index now

	clrbit(&pktpgr->rsp_ring_in_process, pp_ring);
done:
	if (!hwa_ring_is_empty(rsp_ring)) {
		/* need re-schdeule */
		dev->pageintstatus |= pktpgr->rsp_ring_int_mask[pp_ring];
		dev->intstatus |= HWA_COMMON_INTSTATUS_PACKET_PAGER_INTR_MASK;
	}

	return;

}    // hwa_pktpgr_response()

/**
 * +--------------------------------------------------------------------------+
 *  Generic API to post a Request command into one of the S2H interfaces
 *  to the Packet Pager. Caller must compose/marshall/pack a Packet Pager
 *  command, and pass to this requst API. This API copies the 16B command into
 *  a slot in the request interface. If the request ring is full a HWA_FAILURE
 *  is returned, otherwise a transaction id is returned.
 *
 *  Transaction ID is a 8bit incrementing unsigned char. As a request ring may
 *  be deeper than 256 elements, a transaction Id does not necessarily identify
 *  a unique RD index in the ring.
 * +--------------------------------------------------------------------------+
 */

int // HWApp: Post a request command
hwa_pktpgr_request(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring, void *pp_cmd)
{
	uint8         trans_id;         // per s2h_ring incrementing transaction id
	hwa_ring_t   *req_ring;         // sw to hw request ring
	hwa_pp_cmd_t *req_cmd;          // location in s2h ring to place the cmd
	hwa_pktpgr_t *pktpgr;           // pktpgr local state

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	pktpgr = &dev->pktpgr;
	req_ring = pktpgr->req_ring[pp_ring];

	if (hwa_ring_is_full(req_ring))
		goto failure;

	trans_id = pktpgr->trans_id[pp_ring]++; // increment transaction id
	req_cmd  = HWA_RING_PROD_ELEM(hwa_pp_cmd_t, req_ring);

	*req_cmd = *((hwa_pp_cmd_t*)pp_cmd);    // copy 16 byte command into ring
	// Clear invalid field for new request.
	req_cmd->u8[HWA_PP_CMD_TRANS] = BCM_CBIT(req_cmd->u8[HWA_PP_CMD_TRANS],
		HWA_PP_CMD_INVALID);

	// Take one bit from trans_id for special purpose in pagein req.
	// XXX, hwa_pktpgr_freerph_req_ring, hwa_pktpgr_pagemgr_req_ring
	// trans_id must keep 8 bits because of Rx reclaim and map_pkts_cb.
	if (pp_ring == hwa_pktpgr_pagein_req_ring ||
		pp_ring == hwa_pktpgr_pagemgr_req_ring) {
		pktpgr->trans_id[pp_ring] &= HWA_PP_CMD_TRANS_ID_MASK;
		req_cmd->pagein_req.trans_id = trans_id; // overwrite trans_id
		// tagged bit must be handled by caller.
	} else {
		// Repropuse trans_id for hwa_pktpgr_pageout_req_ring
		if (HWAREV_GE(dev->corerev, 133) ||
			pp_ring != hwa_pktpgr_pageout_req_ring) {
			req_cmd->u8[HWA_PP_CMD_TRANS_ID] = trans_id; // overwrite trans_id
		}
	}

#ifdef HWA_QT_TEST
	if (g_pktpgr_req_commit_delay) {
		hwa_ring_prod_upd(req_ring, 1, FALSE);	// No update/commit WR
	} else
#endif
	{
		hwa_ring_prod_upd(req_ring, 1, TRUE);	// update/commit WR
	}

	return trans_id;

failure:
	HWA_TRACE(("%s req ring %u pp_cmd type 0x%x failure\n", HWApp,
		pp_ring, ((hwa_pp_cmd_t*)pp_cmd)->u8[HWA_PP_CMD_TRANS]));

	return HWA_FAILURE;

}   // hwa_pktpgr_request()

/**
 * +--------------------------------------------------------------------------+
 *  Generic Packet Pager consumer loop. Each element of the Response ring is
 *  processed by invoking the handler identified by the transaction command type
 *
 *  Only process specific transaction command type. If the Response ring is in process in DPC,
 *  Don't commit the RD to break the origianl loop.
 *
 *  There are no response rings corresponding to Free pkt/rph/d11b requests.
 * +--------------------------------------------------------------------------+
 */
static void
hwa_pktpgr_response_for_trans_cmd(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring,
	hwa_callback_t callback, uint8 cmdtype)
{
	uint8          trans_cmd;
	hwa_pp_cmd_t  *rsp_cmd;         // pointer to command in h2s response ring
	hwa_handler_t *rsp_handler;     // response handler
	hwa_ring_t    *rsp_ring;        // hw to sw response ring
	hwa_pktpgr_t  *pktpgr;		// pktpgr local state
	uint16         depth;
	int            rd_idx;
	int            wr_idx;
	bool           commit_sw_rd;
	bool           commit_hw_rd;
	bool           rsp_ring_in_process;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	pktpgr = &dev->pktpgr;
	rsp_ring = pktpgr->rsp_ring[pp_ring];

	if (isset(&pktpgr->rsp_ring_in_process, pp_ring)) {
		rsp_ring_in_process = TRUE;
	} else {
		rsp_ring_in_process = FALSE;
		setbit(&pktpgr->rsp_ring_in_process, pp_ring);
	}

	// Driver will mark the response as invalid before processing.
	// This could avoid re-entry to those processed response.
	// So if cmdtype == HWA_PP_CMD_CONS_ALL, we should update read index to inform HWA.
	// Then HWA could continue to generate pending response.
	if ((!rsp_ring_in_process) || (cmdtype == HWA_PP_CMD_CONS_ALL)) {
		commit_sw_rd = TRUE;
	} else {
		commit_sw_rd = FALSE;
	}
	commit_hw_rd = FALSE;

	hwa_ring_cons_get(rsp_ring);    // fetch response ring's WR index once

	wr_idx = HWA_RING_STATE(rsp_ring)->write;
	rd_idx = HWA_RING_STATE(rsp_ring)->read;
	depth = rsp_ring->depth;

	while (rd_idx != wr_idx) {
		rsp_cmd     = HWA_RING_ELEM(hwa_pp_cmd_t, rsp_ring, rd_idx);
		trans_cmd   = rsp_cmd->u8[HWA_PP_CMD_TRANS];
		if ((cmdtype == HWA_PP_CMD_CONS_ALL) || (trans_cmd == cmdtype)) {
			if (!BCM_TBIT(trans_cmd, HWA_PP_CMD_INVALID)) {
				rsp_handler = &dev->handlers[callback + trans_cmd];

				// Mark this response as invalid to avoid processing again.
				rsp_cmd->u8[HWA_PP_CMD_TRANS] |= BCM_SBIT(HWA_PP_CMD_INVALID);

				(*rsp_handler->callback)(rsp_handler->context, dev, rsp_cmd);
			}
		} else {
			// Don't commit the SW RD if there is unprocessed response.
			commit_sw_rd = FALSE;
		}

		// Next.
		rd_idx = (rd_idx + 1) % depth;

		if (commit_sw_rd) {
			// Commit pending SW read
			hwa_ring_cons_done(rsp_ring, rd_idx);
			commit_hw_rd = TRUE;
		}
	}

	if (commit_hw_rd) {
		// Commit HW RD index
		hwa_ring_cons_put(rsp_ring);
	}

	if (!rsp_ring_in_process) {
		clrbit(&pktpgr->rsp_ring_in_process, pp_ring);
		if (!hwa_ring_is_empty(rsp_ring)) {
			/* need re-schdeule */
			dev->pageintstatus |= pktpgr->rsp_ring_int_mask[pp_ring];
			dev->intstatus |= HWA_COMMON_INTSTATUS_PACKET_PAGER_INTR_MASK;
		}
	}

	return;

}    // hwa_pktpgr_response_for_trans_cmd()

// +--------------------------------------------------------------------------+

hwa_ring_t * // Initialize SW and HW ring contexts.
BCMATTACHFN(hwa_pktpgr_ring_init)(hwa_dev_t *dev, hwa_ring_t *hwa_ring,
	const char *ring_name, uint8 ring_dir, uint8 ring_num,
	uint16 depth, void *memory,
	hwa_regs_t *regs, hwa_pp_ring_t *hwa_pp_ring)
{
	uint32 v32;
	hwa_ring_init(hwa_ring, ring_name, HWA_PKTPGR_ID, ring_dir, ring_num,
		depth, memory, &hwa_pp_ring->wr_index, &hwa_pp_ring->rd_index);

	v32 = HWA_PTR2UINT(memory);
	HWA_WR_REG_ADDR(ring_name, &hwa_pp_ring->addr, v32);

	HWA_ASSERT(depth == HWA_PKTPGR_INTERFACE_DEPTH);
	v32 = BCM_SBF(depth, HWA_PAGER_PP_RING_CFG_DEPTH);
	if (ring_dir == HWA_RING_S2H) {
		if (ring_num == HWA_PKTPGR_PAGEOUT_S2H_RINGNUM)
			v32 |= BCM_SBF(HWA_PKTPGR_PGO_REQ_WAITTIME,
			               HWA_PAGER_PP_REQ_RING_CFG_WAITTIME);
		else
			v32 |= BCM_SBF(HWA_PKTPGR_REQ_WAITTIME,
			               HWA_PAGER_PP_REQ_RING_CFG_WAITTIME);
		if (ring_num == HWA_PKTPGR_PAGEIN_S2H_RINGNUM) {
			v32 |= BCM_SBF(HWA_PKTPGR_RX_BESTEFFORT,
			               HWA_PAGER_PP_PAGEIN_REQ_RING_CFG_PPINRXPROCESSBE);
			if (HWAREV_GE(dev->corerev, 133)) {
				v32 |= BCM_SBIT(HWA_PAGER_PP_PAGEIN_REQ_RING_CFG_PPINRXPROCESSMODE);
			}
		}
	}
	HWA_WR_REG_ADDR(ring_name, &hwa_pp_ring->cfg, v32);

	v32 = BCM_SBF(HWA_PKTPGR_INTRAGGR_COUNT,
	              HWA_PAGER_PP_RING_LAZYINT_CFG_AGGRCOUNT)
	    | BCM_SBF(HWA_PKTPGR_INTRAGGR_TMOUT,
	              HWA_PAGER_PP_RING_LAZYINT_CFG_AGGRTIMER);
	HWA_WR_REG_ADDR(ring_name, &hwa_pp_ring->lazyint_cfg, v32);

	return hwa_ring;

}   // hwa_pktpgr_ring_init()

hwa_pktpgr_t * // HWApp: Allocate all required memory for PktPgr
BCMATTACHFN(hwa_pktpgr_attach)(hwa_dev_t *dev)
{
	int ring;
	uint32 v32, mem_sz, depth;
	hwa_regs_t *regs;
	hwa_pager_regs_t *pager_regs;
	hwa_pktpgr_t *pktpgr;
	void *memory[hwa_pktpgr_ring_max], *ring_memory;
#ifdef PSPL_TX_TEST
	uint32 pspl_table_sz;
	void *pspl_table = NULL;
#endif

	HWA_FTRACE(HWApp);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	regs = dev->regs;
	pager_regs = &regs->pager;
	pktpgr = &dev->pktpgr;

	// Initial variables
	pktpgr->pgi_rxpkt_req_max = HWA_PKTPGR_PGI_RXPKT_REQ_MAX;
	pktpgr->pgi_rxpkt_count_max = HWA_PKTPGR_PGI_RXPKT_COUNT_MAX;
	// 2048 - 8 - 256 = 1784
	pktpgr->ddbm_avail_sw = (HWA_DNGL_PKTS_MAX - HWA_PKTPGR_PAGEIN_REQ_DDBMTH -
		HWA_PKTPGR_DDBM_SW_RESV);
	pktpgr->ddbm_avail_sw_lwm = pktpgr->ddbm_avail_sw;
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: Init : %d @ %s\n",
		pktpgr->ddbm_avail_sw, __FUNCTION__);

	// Extra ddbm resv for Txs
	pktpgr->txs_ddbm_resv = MIN(pktpgr->ddbm_avail_sw, 128);

	// Verify PP block structures
	HWA_ASSERT(HWA_PP_LBUF_SZ ==
	           (HWA_PP_PKTCONTEXT_BYTES + HWA_PP_PKTDATABUF_BYTES));
	HWA_ASSERT(sizeof(hwa_pp_cmd_t) == HWA_PP_COMMAND_BYTES);

	// Align dnglpktpool_mem to 256B
	mem_sz = sizeof(hwa_pp_lbuf_t) * HWA_DNGL_PKTS_MAX;
#ifdef HWA_QT_TEST
	if (HWAREV_GE(dev->corerev, 133)) {
		// VELOCE TEST: Force 8 bytes alignment.
		mem_sz = sizeof(hwa_pp_lbuf_t) * (HWA_DNGL_PKTS_MAX+1);
	}
#endif
	pktpgr->dnglpktpool_mem = MALLOC_ALIGN(dev->osh, mem_sz, 8);
	if (pktpgr->dnglpktpool_mem == NULL) {
		HWA_ERROR(("%s lbuf pool items<%u> mem_size<%u> failure\n",
			HWApp, HWA_DNGL_PKTS_MAX, mem_sz));
		HWA_ASSERT(pktpgr->dnglpktpool_mem != ((hwa_pp_lbuf_t *)NULL));
		goto lbufpool_failure;
	}
	ASSERT(ISALIGNED(pktpgr->dnglpktpool_mem, 256));

	pktpgr->dnglpktpool_max = HWA_DNGL_PKTS_MAX;

#ifdef HWA_QT_TEST
	if (HWAREV_GE(dev->corerev, 133)) {
		// VELOCE TEST: Force 8 bytes alignment.
		pktpgr->dnglpktpool_mem = ((char *)pktpgr->dnglpktpool_mem) + 8;
	}
#endif

	// Initial SW lbufpools
	pktpgr->txlbufpool.max_pkts = HWA_PP_TXLBUFPOOL_LEN_MAX;
	pktpgr->rxlbufpool.max_pkts = HWA_PP_RXLBUFPOOL_LEN_MAX;
#ifdef HNDPQP
	pktpgr->pqplbufpool.max_pkts = HWA_PP_PQPLBUFPOOL_LEN_MAX;
#endif

	// Allocate and initialize the S2H Request and H2S Response interfaces
	// XXX, Be careful if you want to adjust some rings depth. The ring index
	// here to allocate memory is not the same in hwa_pktpgr_ring_init.
	depth = HWA_PKTPGR_INTERFACE_DEPTH;
	mem_sz = depth * sizeof(hwa_pp_cmd_t);
	for (ring = 0; ring < hwa_pktpgr_ring_max; ++ring) {
		// 4KB alignment can reduce transaction times.
		if ((memory[ring] = MALLOC_ALIGN(dev->osh, mem_sz, 12)) == NULL) {
			HWA_ERROR(("%s ring %u size<%u> failure\n", HWApp, ring, mem_sz));
			HWA_ASSERT(memory[ring] != (void*)NULL);
			goto ring_failure;
		}
		ASSERT(ISALIGNED(memory[ring], 4096));
	}

#ifdef PSPL_TX_TEST
	// Allocate and initialize the PUSH/PULL request info ring
	pspl_table_sz = HWA_PKTPGR_PSPL_DEPTH * sizeof(hwa_pp_pspl_req_info_t);
	if ((pspl_table = MALLOCZ(dev->osh, pspl_table_sz)) == NULL) {
		HWA_ERROR(("%s pspl ring malloc size<%u> failure\n",
			HWApp, pspl_table_sz));
		HWA_ASSERT(pspl_table != (void*)NULL);
		goto ring_failure;
	}
#endif /* PSPL_TX_TEST */

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
	// HDBM
	pktpgr->hdbm_map = bcm_mwbmap_init(dev->osh, HWA_HOST_PKTS_MAX + 1);
	if (pktpgr->hdbm_map == NULL) {
		HWA_ERROR(("hdbm_map for audit allocation failed\n"));
		goto ring_failure;
	}
	bcm_mwbmap_force(pktpgr->hdbm_map, 0); /* id=0 is invalid */

	// DDBM
	pktpgr->ddbm_map = bcm_mwbmap_init(dev->osh, HWA_DNGL_PKTS_MAX + 1);
	if (pktpgr->ddbm_map == NULL) {
		HWA_ERROR(("ddbm_map for audit allocation failed\n"));
		goto ring_failure;
	}
	bcm_mwbmap_force(pktpgr->ddbm_map, 0); /* id=0 is invalid */
#endif /* HWA_PKTPGR_DBM_AUDIT_ENABLED */

	// Initial tag_pull_q
	spktq_init(&pktpgr->tag_pull_q, PKTQ_LEN_MAX);

	// HWA_COMMON_HWA2HWCAP_HWPPSUPPORT must be set before
	// Dongle packet pool configuration.
	// Enable PP support in HWA 2.2. see also hwa_config
	v32 = HWA_RD_REG_NAME(HWApp, dev->regs, common, hwa2hwcap);
	v32 |= BCM_SBIT(HWA_COMMON_HWA2HWCAP_HWPPSUPPORT);
	HWA_WR_REG_NAME(HWApp, dev->regs, common, hwa2hwcap, v32);

	// Setup Rx Block with RxLfrag databuffer offset in words and to
	// use ext/internal D11Bdest and default Rxblock to PAGER mode
	v32 = HWA_RD_REG_NAME(HWApp, regs, rx_core[0], rxpsrc_ring_hwa2cfg);
	if (HWAREV_GE(dev->corerev, 133)) {
		// If pp_alloc_freerph is set, the alloc_rph request may be fulfilled by taking
		// the rph from free_rph request.
		v32 |= BCM_SBIT(HWA_RX_RXPSRC_RING_HWA2CFG_PP_ALLOC_FREERPH)
			| BCM_SBF(dev->d11b_axi, HWA_RX_RXPSRC_RING_HWA2CFG_PP_INT_D11B_WR);
	}
	v32 |= BCM_SBF(dev->d11b_axi, HWA_RX_RXPSRC_RING_HWA2CFG_PP_INT_D11B)
	    | BCM_SBIT(HWA_RX_RXPSRC_RING_HWA2CFG_PP_PAGER_MODE);
	HWA_WR_REG_NAME(HWApp, regs, rx_core[0], rxpsrc_ring_hwa2cfg, v32);

	// NOTE: These rx_core registers setting will be reset in rxpath, hwa_pcie.c
	// We need to reconfig it in hwa_pktpgr_init
	v32 = HWA_RD_REG_NAME(HWApp, regs, rx_core[0], pagein_status);
	// Lbuf Context includes 4 FragInfo's Haddr64. In RxPath, only one fraginfo
	// is needed, and the memory of the remaining three are repurposed for
	// databuffer, allowing larger a 152 Byte databuffer.
	v32 = BCM_CBF(v32, HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_LEN)
		| BCM_SBF(HWA_PP_RXLFRAG_DATABUF_LEN_WORDS,
			HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_LEN);
	// PP_RXLFRAG_DATA_BUF_OFFSET is used for HWA to DMA data from MAC.
	// rx::rxfill_ctrl1::d11b_offset is used to program MAC descriptor by HWA
	v32 = BCM_CBF(v32, HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_OFFSET)
		| BCM_SBF(HWA_PP_RXLFRAG_DATABUF_OFFSET_WORDS,
			HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_OFFSET);
	// DMA template. Dongle page and Host page.
	// XXX, revisit ADDREXT_H when having high address bit63 in 64bits host.
	v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_STATUS_TEMPLATE_ADDREXT_H);
	v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_STATUS_TEMPLATE_NOTPCIE_H);
	v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_STATUS_TEMPLATE_COHERENT_H);
	v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_STATUS_TEMPLATE_ADDREXT_D);
	v32 = v32
		| BCM_SBIT(HWA_RX_PAGEIN_STATUS_TEMPLATE_NOTPCIE_D)
		| BCM_SBIT(HWA_RX_PAGEIN_STATUS_TEMPLATE_COHERENT_D)
		| 0U;
	HWA_WR_REG_NAME(HWApp, regs, rx_core[0], pagein_status, v32);

	if (HWAREV_GE(dev->corerev, 133)) {
		// rx_core::pagein_cfg
		v32 = HWA_RD_REG_NAME(HWApp, regs, rx_core[0], pagein_cfg);
		// Clear mem_pend_wait_halt.
		v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_CFG_MEM_PEND_WAIT_HALT);
		// Wait dma timeout 15 us.
		v32 = BCM_CBF(v32, HWA_RX_PAGEIN_CFG_DMA_PEND_WAIT)
			| BCM_SBF(15, HWA_RX_PAGEIN_CFG_DMA_PEND_WAIT);
		HWA_WR_REG_NAME(HWApp, regs, rx_core[0], pagein_cfg, v32);
	}

	v32 = BCM_SBF(HWA_PKTPGR_D11BDEST_TH0,
	              HWA_RX_D11BDEST_THRESHOLD_L1L0_PP_D11THRESHOLD_L0)
	    | BCM_SBF(HWA_PKTPGR_D11BDEST_TH1,
	              HWA_RX_D11BDEST_THRESHOLD_L1L0_PP_D11THRESHOLD_L1);
	HWA_WR_REG_NAME(HWApp, regs, rx_core[0], d11bdest_threshold_l1l0, v32);
	v32 = BCM_SBF(HWA_PKTPGR_D11BDEST_TH2,
	              HWA_RX_D11BDEST_THRESHOLD_L2_PP_D11THRESHOLD_L2);
	HWA_WR_REG_NAME(HWApp, regs, rx_core[0], d11bdest_threshold_l2, v32);

	// Turn on common::module_enable bit packet pager: see hwa_enable()

	// Describe Lbuf::Context and Lbuf::Databuffer size
	v32 = BCM_SBF(HWA_PP_PKTCONTEXT_BYTES,
	              HWA_PAGER_PP_PKTCTX_SIZE_PPPKTCTXSIZE);
	HWA_WR_REG_NAME(HWApp, regs, pager, pp_pktctx_size, v32);
	v32 = BCM_SBF(HWA_PP_PKTDATABUF_BYTES,
	              HWA_PAGER_PP_PKTBUF_SIZE_PPPKTBUFSIZE);
	HWA_WR_REG_NAME(HWApp, regs, pager, pp_pktbuf_size, v32);

	// Configure the DMA Descriptor template
	v32 = 0U
	    // All interface's ring memory is coherent and NotPCIe
	    | BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPINREQNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPINREQCOHERENT)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPINRSPNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPINRSPCOHERENT)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPOUTREQNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPOUTREQCOHERENT)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPOUTRSPNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPOUTRSPCOHERENT)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPALLOCREQNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPALLOCREQCOHERENT)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPALLOCRSPNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPALLOCRSPCOHERENT)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPFREEREQNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPFREEREQCOHERENT)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPFREERPHREQNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPFREERPHREQCOHERENT)
		// PPAPKTNOTPCIE: bit for PAGER to DMA data to donglepool address
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPAPKTNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPAPKTCOHERENT)
		// PPRXAPKTNOTPCIE: bit for PAGER to DMA data to donglepool address
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPRXAPKTNOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPRXAPKTCOHERENT)
		// PPPUSHSANOTPCIE: bit for PAGER to DMA data from SA:Dongle to dest address
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPUSHSANOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPUSHSACOHERENT)
		// PPPUSHDANOTPCIE: bit for PAGER to DMA data from src to DA:Host address
		//| BCM_CBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPUSHDANOTPCIE)
		//| BCM_CBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPUSHDACOHERENT)
		// XXX clear it for now
		//| BCM_CBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPUSHDAWCPDESC)
		// PPPULLSANOTPCIE: bit for PAGER to DMA data from SA:Host to dest address
		//| BCM_CBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPULLSANOTPCIE)
		//| BCM_CBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPULLSACOHERENT)
		// PPPULLDANOTPCIE: bit for PAGER to DMA data from src to DA:Dongle address
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPULLDANOTPCIE)
		| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPULLDACOHERENT)
		// XXX clear it for now
		//| BCM_CBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPULLDAWCPDESC)
		| 0U;
	if (HWAREV_GE(dev->corerev, 133)) {
		v32 = BCM_CBF(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_PPPULLSAADDREXT);
	}
	HWA_WR_REG_NAME(HWApp, regs, pager, pp_dma_descr_template, v32);

	// DMA Descriptor template2
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, pp_dma_descr_template_2);
	v32 = BCM_CBIT(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPINRSPWCPDESC);
	v32 = BCM_CBIT(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPOUTSPWCPDESC);
	v32 = BCM_CBIT(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPALLOCSPWCPDESC);
	v32 = BCM_CBIT(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPAPKTWCPDESC);
	v32 = BCM_CBIT(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPRXAPKTWCPDESC);
	if (HWAREV_GE(dev->corerev, 133)) {
		v32 = BCM_CBF(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPPULLDAADDREXT);
		v32 = BCM_CBF(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPPUSHSAADDREXT);
		v32 = BCM_CBF(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPPUSHDAADDREXT);
		v32 |= BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPFREENOTPCIE)
			| BCM_SBIT(HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPFREECOHERENT);
		v32 = BCM_CBF(v32, HWA_PAGER_PP_DMA_DESCR_TEMPLATE_2_PPFREEADDREXT);
	}
	HWA_WR_REG_NAME(HWApp, regs, pager, pp_dma_descr_template_2, v32);

	// DBM thresholds for stopping pagein req.
	// SW controls all pagein request based on DDBM availability.
	v32 = (0U
		| BCM_SBF(HWA_PKTPGR_PAGEIN_REQ_DDBMTH,
		          HWA_PAGER_PP_PAGEIN_REQ_DDBMTH_PAGEIN_REQ_DDBMTH)
		| BCM_SBF(HWA_PKTPGR_PAGEIN_REQ_HDBMTH,
		          HWA_PAGER_PP_PAGEIN_REQ_DDBMTH_PAGEIN_REQ_HDBMTH));
	HWA_WR_REG_NAME(HWApp, regs, pager, pp_pagein_req_ddbmth, v32);

	// Configure Dongle Packet Pool
	v32 = HWA_PTR2UINT(pktpgr->dnglpktpool_mem);
	HWA_WR_REG_ADDR(HWApp, &pager_regs->dnglpktpool.addr.lo, v32);
	HWA_WR_REG_ADDR(HWApp, &pager_regs->dnglpktpool.addr.hi, 0U);
	HWA_WR_REG_ADDR(HWApp, &pager_regs->dnglpktpool.size,
		pktpgr->dnglpktpool_max);
	v32 = (0U
		| BCM_SBF(HWA_PKTPGR_DNGLPKTPOOL_TH1,
		          HWA_PAGER_PP_DNGLPKTPOOL_INTR_TH_PPDDTH1)
		| BCM_SBF(HWA_PKTPGR_DNGLPKTPOOL_TH2,
		          HWA_PAGER_PP_DNGLPKTPOOL_INTR_TH_PPDDTH2));
	HWA_WR_REG_ADDR(HWApp, &pager_regs->dnglpktpool.intr_th, v32);
	v32 = BCM_SBIT(HWA_PAGER_PP_DNGLPKTPOOL_CTRL_PPDDENABLE);
	HWA_WR_REG_ADDR(HWApp, &pager_regs->dnglpktpool.ctrl, v32);

	// Host Packet Pool configured in hwa_config - after dhd::dongle handshake

	// Attach all Packet Pager Interfaces
	ring = 0;

	ring_memory = memory[ring++]; // pagein_req_ring + PPInRxProcessBE
	HWA_ERROR(("%s pagein_req +memory[%p,%u]\n", HWApp, ring_memory, mem_sz));
	pktpgr->req_ring[hwa_pktpgr_pagein_req_ring] =
		hwa_pktpgr_ring_init(dev, &pktpgr->pagein_req_ring, "PI>",
			HWA_RING_S2H, HWA_PKTPGR_PAGEIN_S2H_RINGNUM,
			depth, ring_memory, regs, &pager_regs->pagein_req_ring);
	pktpgr->req_ring_reg[hwa_pktpgr_pagein_req_ring] = &pager_regs->pagein_req_ring;

	ring_memory = memory[ring++]; // pagein_rsp_ring
	HWA_ERROR(("%s pagein_rsp +memory[%p,%u]\n", HWApp, ring_memory, mem_sz));
	pktpgr->rsp_ring[hwa_pktpgr_pagein_rsp_ring] =
		hwa_pktpgr_ring_init(dev, &pktpgr->pagein_rsp_ring, "PI<",
			HWA_RING_H2S, HWA_PKTPGR_PAGEIN_H2S_RINGNUM,
			depth, ring_memory, regs, &pager_regs->pagein_rsp_ring);
	pktpgr->rsp_ring_int_mask[hwa_pktpgr_pagein_rsp_ring] = HWA_COMMON_PAGEIN_INT_MASK;

	ring_memory = memory[ring++]; // pageout_req_ring
	HWA_ERROR(("%s pageout_req +memory[%p,%u]\n", HWApp, ring_memory, mem_sz));
	pktpgr->req_ring[hwa_pktpgr_pageout_req_ring] =
		hwa_pktpgr_ring_init(dev, &pktpgr->pageout_req_ring, "PO>",
			HWA_RING_S2H, HWA_PKTPGR_PAGEOUT_S2H_RINGNUM,
			depth, ring_memory, regs, &pager_regs->pageout_req_ring);
	pktpgr->req_ring_reg[hwa_pktpgr_pageout_req_ring] = &pager_regs->pageout_req_ring;

	ring_memory = memory[ring++]; // pageout_rsp_ring
	HWA_ERROR(("%s pageout_rsp +memory[%p,%u]\n", HWApp, ring_memory, mem_sz));
	pktpgr->rsp_ring[hwa_pktpgr_pageout_rsp_ring] =
		hwa_pktpgr_ring_init(dev, &pktpgr->pageout_rsp_ring, "PO<",
			HWA_RING_H2S, HWA_PKTPGR_PAGEOUT_H2S_RINGNUM,
			depth, ring_memory, regs, &pager_regs->pageout_rsp_ring);
	pktpgr->rsp_ring_int_mask[hwa_pktpgr_pageout_rsp_ring] = HWA_COMMON_PAGEOUT_INT_MASK;

	ring_memory = memory[ring++]; // pagemgr_req_ring
	HWA_ERROR(("%s pagemgr_req +memory[%p,%u]\n", HWApp, ring_memory, mem_sz));
	pktpgr->req_ring[hwa_pktpgr_pagemgr_req_ring] =
		hwa_pktpgr_ring_init(dev, &pktpgr->pagemgr_req_ring, "PM>",
			HWA_RING_S2H, HWA_PKTPGR_PAGEMGR_S2H_RINGNUM,
			depth, ring_memory, regs, &pager_regs->pagemgr_req_ring);
	pktpgr->req_ring_reg[hwa_pktpgr_pagemgr_req_ring] = &pager_regs->pagemgr_req_ring;

	ring_memory = memory[ring++]; // pagemgr_rsp_ring
	HWA_ERROR(("%s pagemgr_rsp +memory[%p,%u]\n", HWApp, ring_memory, mem_sz));
	pktpgr->rsp_ring[hwa_pktpgr_pagemgr_rsp_ring] =
		hwa_pktpgr_ring_init(dev, &pktpgr->pagemgr_rsp_ring, "PM<",
			HWA_RING_H2S, HWA_PKTPGR_PAGEMGR_H2S_RINGNUM,
			depth, ring_memory, regs, &pager_regs->pagemgr_rsp_ring);
	pktpgr->rsp_ring_int_mask[hwa_pktpgr_pagemgr_rsp_ring] = HWA_COMMON_PAGEMGR_INT_MASK;

	ring_memory = memory[ring++]; // freepkt_req_ring
	HWA_ERROR(("%s freepkt_req +memory[%p,%u]\n", HWApp, ring_memory, mem_sz));
	pktpgr->req_ring[hwa_pktpgr_freepkt_req_ring] =
		hwa_pktpgr_ring_init(dev, &pktpgr->freepkt_req_ring, "PKT",
			HWA_RING_S2H, HWA_PKTPGR_FREEPKT_S2H_RINGNUM,
			depth, ring_memory, regs, &pager_regs->freepkt_req_ring);
	pktpgr->req_ring_reg[hwa_pktpgr_freepkt_req_ring] = &pager_regs->freepkt_req_ring;

	ring_memory = memory[ring++]; // freerph_req_ring
	HWA_ERROR(("%s freerph_req +memory[%p,%u]\n", HWApp, ring_memory, mem_sz));
	pktpgr->req_ring[hwa_pktpgr_freerph_req_ring] =
		hwa_pktpgr_ring_init(dev, &pktpgr->freerph_req_ring, "RPH",
			HWA_RING_S2H, HWA_PKTPGR_FREERPH_S2H_RINGNUM,
			depth, ring_memory, regs, &pager_regs->freerph_req_ring);
	pktpgr->req_ring_reg[hwa_pktpgr_freerph_req_ring] = &pager_regs->freerph_req_ring;

	HWA_ASSERT(ring == hwa_pktpgr_ring_max); // done all rings

#ifdef PSPL_TX_TEST
	// Initial SW PUSH/PULL circular ring
	HWA_ERROR(("%s pspl req info ring +memory[%p,%u]\n", HWApp, pspl_table, pspl_table_sz));
	bcm_ring_init(&pktpgr->pspl_state);
	pktpgr->pspl_table = pspl_table;
	pktpgr->pspl_depth = HWA_PKTPGR_PSPL_DEPTH;

	if (HWAREV_GE(dev->corerev, 133)) {
		pktpgr->pspl_mpdu_pkt_mapid_h = HWA_PP_PKT_MAPID_INVALID;
		pktpgr->pspl_mpdu_pkt_mapid_t = HWA_PP_PKT_MAPID_INVALID;
	}
#endif /* PSPL_TX_TEST */

	// Register process item callback for the PKTPGR H2S interfaces.
	hwa_register(dev, HWA_PKTPGR_PAGEIN_RXPROCESS, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagein_rxprocess);
	hwa_register(dev, HWA_PKTPGR_PAGEIN_TXSTATUS, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagein_txstatus);
	hwa_register(dev, HWA_PKTPGR_PAGEIN_TXPOST, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagein_txpost);
	hwa_register(dev, HWA_PKTPGR_PAGEIN_TXPOST_FRC, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagein_txpost);
	hwa_register(dev, HWA_PKTPGR_PAGEOUT_PKTLIST, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pageout_pktlist);
	hwa_register(dev, HWA_PKTPGR_PAGEOUT_LOCAL, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pageout_pktlist);
	hwa_register(dev, HWA_PKTPGR_PAGEOUT_HDBM_PKTLIST, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pageout_pktlist);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_ALLOC_RX, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_alloc_rx);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_ALLOC_RX_RPH, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_alloc_rx_rph);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_ALLOC_TX, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_alloc_tx);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_PUSH, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_push);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_PULL, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_pull);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_PUSH_PKTTAG, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_push);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_PULL_KPFL_LINK, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_pull);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_PUSH_MPDU, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_push);
	hwa_register(dev, HWA_PKTPGR_PAGEMGR_PULL_MPDU, dev,
	             (hwa_callback_fn_t) hwa_pktpgr_pagemgr_pull);

	return pktpgr;

ring_failure:
	for (ring = 0; ring < hwa_pktpgr_ring_max; ++ring) {
		if (memory[ring] == NULL)
			break;
		MFREE(dev->osh, memory[ring], mem_sz);
	}

#ifdef PSPL_TX_TEST
	if (pspl_table)
		MFREE(dev->osh, pspl_table, pspl_table_sz);
#endif

lbufpool_failure:
	hwa_pktpgr_detach(pktpgr);
	HWA_WARN(("%s attach failure\n", HWApp));

	return (hwa_pktpgr_t*) NULL;

}   // hwa_pktpgr_attach()

void // HWApp: Cleanup/Free resources used by PktPgr block
BCMATTACHFN(hwa_pktpgr_detach)(hwa_pktpgr_t *pktpgr)
{
	hwa_dev_t *dev;

	HWA_FTRACE(HWApp);

	if (pktpgr == (hwa_pktpgr_t*)NULL)
		return;

	// Audit pre-conditions
	dev = HWA_DEV(pktpgr);

	if (pktpgr->dnglpktpool_mem != (hwa_pp_lbuf_t*)NULL) {
		void * memory = (void*)pktpgr->dnglpktpool_mem;
		uint32 mem_sz = pktpgr->dnglpktpool_max * HWA_PP_LBUF_SZ;
		HWA_TRACE(("%s lbuf pool items<%u> mem_size<%u> free\n",
			HWApp, HWA_DNGL_PKTS_MAX, mem_sz));
		MFREE(dev->osh, memory, mem_sz);
		pktpgr->dnglpktpool_mem = (hwa_pp_lbuf_t*)NULL;
		pktpgr->dnglpktpool_max = 0;
	}

#ifdef PSPL_TX_TEST
	if (pktpgr->pspl_table) {
		MFREE(dev->osh, pktpgr->pspl_table,
			HWA_PKTPGR_PSPL_DEPTH * sizeof(hwa_pp_pspl_req_info_t));
		pktpgr->pspl_table = NULL;
	}
#endif

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
	if (pktpgr->hdbm_map != NULL) {
		bcm_mwbmap_fini(dev->osh, pktpgr->hdbm_map);
		pktpgr->hdbm_map = NULL;
	}
	if (pktpgr->ddbm_map != NULL) {
		bcm_mwbmap_fini(dev->osh, pktpgr->ddbm_map);
		pktpgr->ddbm_map = NULL;
	}
#endif
}   // hwa_pktpgr_detach()

void // HWApp: Init PktPgr block AFTER DHD handshake pcie_ipc initialized.
hwa_pktpgr_preinit(hwa_pktpgr_t *pktpgr)
{
	uint32 v32, mem_sz;
	hwa_dev_t *dev;
	hwa_pager_regs_t *pager_regs;

	HWA_FTRACE(HWApp);

	// Audit pre-conditions
	dev = HWA_DEV(pktpgr);
	HWA_ASSERT(dev->pcie_ipc != (pcie_ipc_t*)NULL);

	pager_regs = &dev->regs->pager; // Setup locals

	// Allocate Host Packet Pool
	mem_sz = HWA_HOST_PKTS_MAX * HWA_PP_LBUF_SZ;
#ifdef HWA_QT_TEST
	if (HWAREV_GE(dev->corerev, 133)) {
		// VELOCE TEST: Force 8 bytes alignment.
		mem_sz = (HWA_HOST_PKTS_MAX+1) * HWA_PP_LBUF_SZ;
	}
#endif
	pktpgr->hostpktpool_haddr64 = hme_get(HME_USER_PKTPGR, mem_sz);

	if (HADDR64_IS_ZERO(pktpgr->hostpktpool_haddr64)) {
		HWA_ERROR(("Allocate Host Packet Pool size %d failed\n",
			(HWA_HOST_PKTS_MAX * HWA_PP_LBUF_SZ)));
		HWA_ASSERT(0);
	}
	pktpgr->hostpktpool_max = HWA_HOST_PKTS_MAX;

#ifdef HWA_QT_TEST
	if (HWAREV_GE(dev->corerev, 133)) {
		// VELOCE TEST: Force 8 bytes alignment.
		pktpgr->hostpktpool_haddr64.loaddr += 8;
	}
#endif

	v32 = HADDR64_LO(pktpgr->hostpktpool_haddr64);
	HWA_WR_REG_ADDR(HWApp, &pager_regs->hostpktpool.addr.lo, v32);
	v32 = HWA_HOSTADDR64_HI32(HADDR64_HI(pktpgr->hostpktpool_haddr64));
	HWA_WR_REG_ADDR(HWApp, &pager_regs->hostpktpool.addr.hi, v32);
	HWA_WR_REG_ADDR(HWApp, &pager_regs->hostpktpool.size,
		pktpgr->hostpktpool_max);
	v32 = (0U
		| BCM_SBF(HWA_PKTPGR_HOSTPKTPOOL_TH1,
		          HWA_PAGER_PP_HOSTPKTPOOL_INTR_TH_PPHDTH1)
		| BCM_SBF(HWA_PKTPGR_HOSTPKTPOOL_TH2,
		          HWA_PAGER_PP_HOSTPKTPOOL_INTR_TH_PPHDTH2));
	HWA_WR_REG_ADDR(HWApp, &pager_regs->hostpktpool.intr_th, v32);
	v32 = BCM_SBIT(HWA_PAGER_PP_HOSTPKTPOOL_CTRL_PPHDENABLE);
	HWA_WR_REG_ADDR(HWApp, &pager_regs->hostpktpool.ctrl, v32);

	// Enable Packet Pager
	v32 = BCM_SBIT(HWA_PAGER_PP_PAGER_CFG_PAGER_EN);
	HWA_WR_REG_NAME(HWApp, dev->regs, pager, pp_pager_cfg, v32);

}   // hwa_pktpgr_preinit()

void
hwa_pktpgr_init(hwa_pktpgr_t *pktpgr)
{
	uint32 v32;
	uint32 d11b_offset, d11b_length;
	hwa_regs_t *regs;
	hwa_dev_t *dev;

	HWA_FTRACE(HWApp);

	// Audit pre-conditions
	dev = HWA_DEV(pktpgr);

	// Setup locals
	regs = dev->regs;

	// d11b_length is used to program the MAC descriptor.
	d11b_offset = dev->rxfill.config.wrxh_offset +
		dev->rxfill.config.d11_offset;
	d11b_length = HWA_PP_RXLFRAG_DATABUF_LEN - d11b_offset;
	// d11b_offset is start from base address of rxlfrag.
	d11b_offset += HWA_PP_RXLFRAG_DATABUF_OFFSET;
	HWA_ASSERT((ROUNDUP(d11b_length, 4) == d11b_length));
	HWA_ASSERT((ROUNDUP(d11b_offset, 4) == d11b_offset));
	// XXX, B0, NOTE: both d11b_length and d11b_offset MUST 8 bytes aligned.

	// NOTE: These rx_core registers setting will be reset in rxpath, hwa_pcie.c
	// We need to reconfig it in hwa_pktpgr_init
	v32 = HWA_RD_REG_NAME(HWApp, regs, rx_core[0], pagein_status);
	// Lbuf Context includes 4 FragInfo's Haddr64. In RxPath, only one fraginfo
	// is needed, and the memory of the remaining three are repurposed for
	// databuffer, allowing larger a 152 Byte databuffer.
	// Set the length same as RXFILL_CTRL1.
	v32 = BCM_CBF(v32, HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_LEN)
		| BCM_SBF((d11b_length / NBU32),
			HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_LEN);
	// PP_RXLFRAG_DATA_BUF_OFFSET is used for HWA to DMA data from MAC.
	// rx::rxfill_ctrl1::d11b_offset is used to program MAC descriptor by HWA
	// Set the offset same as RXFILL_CTRL1.
	v32 = BCM_CBF(v32, HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_OFFSET)
		| BCM_SBF((d11b_offset / NBU32),
			HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_OFFSET);
	// DMA template. Dongle page and Host page.
	// XXX, revisit ADDREXT_H when having high address bit63 in 64bits host.
	v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_STATUS_TEMPLATE_ADDREXT_H);
	v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_STATUS_TEMPLATE_NOTPCIE_H);
	v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_STATUS_TEMPLATE_COHERENT_H);
	v32 = BCM_CBIT(v32, HWA_RX_PAGEIN_STATUS_TEMPLATE_ADDREXT_D);
	v32 = v32
		| BCM_SBIT(HWA_RX_PAGEIN_STATUS_TEMPLATE_NOTPCIE_D)
		| BCM_SBIT(HWA_RX_PAGEIN_STATUS_TEMPLATE_COHERENT_D)
		| 0U;
	HWA_WR_REG_NAME(HWApp, regs, rx_core[0], pagein_status, v32);

	// pagein_cfg, use default value

	v32 = BCM_SBF(HWA_PKTPGR_D11BDEST_TH0,
	              HWA_RX_D11BDEST_THRESHOLD_L1L0_PP_D11THRESHOLD_L0)
	    | BCM_SBF(HWA_PKTPGR_D11BDEST_TH1,
	              HWA_RX_D11BDEST_THRESHOLD_L1L0_PP_D11THRESHOLD_L1);
	HWA_WR_REG_NAME(HWApp, regs, rx_core[0], d11bdest_threshold_l1l0, v32);
	v32 = BCM_SBF(HWA_PKTPGR_D11BDEST_TH2,
	              HWA_RX_D11BDEST_THRESHOLD_L2_PP_D11THRESHOLD_L2);
	HWA_WR_REG_NAME(HWApp, regs, rx_core[0], d11bdest_threshold_l2, v32);

	// Clear PageOut req ring stop bit which may set in hwa_txfifo_disable_prep
	v32 = HWA_RD_REG_ADDR(HWApp, &regs->pager.pageout_req_ring.cfg);
	v32 = BCM_CBIT(v32, HWA_PAGER_PP_PAGEOUT_REQ_RING_CFG_PPOUTREQSTOP);
	HWA_WR_REG_ADDR(HWApp, &regs->pager.pageout_req_ring.cfg, v32);
	HWA_INFO(("%s: Start PageOut Req\n", __FUNCTION__));

	// pktpgr::pagein_intstatus are read only for
	// second level error handling.  The mask setting is
	// merged in HWA_PKTPGR_INT_MASK
	// NOTE: We don't turn on ERROR bits in common::pageintstatus

	// First level IntStatus: common::pageintstatus
	dev->pageintmask = HWA_PKTPGR_INT_MASK;
	HWA_WR_REG_NAME(HWApp, dev->regs, common, pageintmask, dev->pageintmask);

	// Top level IntStatus: common::intstatus
	dev->defintmask |= HWA_COMMON_INTSTATUS_PACKET_PAGER_INTR_MASK;

}   // hwa_pktpgr_init()

void // HWApp: Deinit PktPgr block
hwa_pktpgr_deinit(hwa_pktpgr_t *pktpgr)
{
}   // hwa_pktpgr_deinit()

/**
 * Single debug interface to read all registers carrying packet pager "status"
 * Uses a wrapper _hwa_pktpgr_ring_status() to dump S2H and H2S interface.
 */
static void _hwa_pktpgr_ring_status(const char *ring_name,
                                    hwa_pp_ring_t *hwa_pp_ring_regs);
static void
_hwa_pktpgr_ring_status(const char *ring_name, hwa_pp_ring_t *hwa_pp_ring_regs)
{
	uint32 v32, wr_index, rd_index;
	wr_index = HWA_RD_REG_ADDR(HWApp, &hwa_pp_ring_regs->wr_index);
	rd_index = HWA_RD_REG_ADDR(HWApp, &hwa_pp_ring_regs->rd_index);
	if (wr_index != rd_index)
		HWA_PRINT("\t %s [wr,rd] = [%u, %u]\n", ring_name, wr_index, rd_index);
	v32 = HWA_RD_REG_ADDR(HWApp, &hwa_pp_ring_regs->debug);
	if (v32) HWA_PRINT("\t %s debug<0x%08x>\n", ring_name, v32);

}   // _hwa_pktpgr_ring_status()

void // HWApp: Query various interfaces and module for status and errors
hwa_pktpgr_status(hwa_dev_t *dev)
{
	uint32 v32;
	hwa_regs_t *regs;
	hwa_pager_regs_t *pager_regs;

	HWA_AUDIT_DEV(dev);
	regs = dev->regs;
	pager_regs = &regs->pager;

	// Ring debug status or processing stalled
	HWA_PRINT("%s Ring Status\n", HWApp);
	_hwa_pktpgr_ring_status("pagein_req ", &pager_regs->pagein_req_ring);
	_hwa_pktpgr_ring_status("pagein_rsp ", &pager_regs->pagein_rsp_ring);
	_hwa_pktpgr_ring_status("pageout_req", &pager_regs->pageout_req_ring);
	_hwa_pktpgr_ring_status("pageout_rsp", &pager_regs->pageout_rsp_ring);
	_hwa_pktpgr_ring_status("pagemgr_req", &pager_regs->pagemgr_req_ring);
	_hwa_pktpgr_ring_status("pagemgr_rsp", &pager_regs->pagemgr_rsp_ring);
	_hwa_pktpgr_ring_status("freepkt_req", &pager_regs->freepkt_req_ring);
	_hwa_pktpgr_ring_status("freerph_req", &pager_regs->freerph_req_ring);

	// Errors reported via Instatus
	HWA_PRINT("%s Ring Errors\n", HWApp);
	v32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->pagein_intstatus)
	    & HWA_PAGER_PAGEIN_ERRORS_MASK;
	if (v32) HWA_PRINT("\t pagein_errors<0x%08x>\n", v32);
	v32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->pageout_intstatus)
	    & HWA_PAGER_PAGEOUT_ERRORS_MASK;
	if (v32) HWA_PRINT("\t pageout_errors<0x%08x>\n", v32);
	v32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->pagemgr_intstatus)
	    & HWA_PAGER_PAGEMGR_ERRORS_MASK;
	if (v32) HWA_PRINT("\t pagemgr_errors<0x%08x>\n", v32);
	v32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->pagerbm_intstatus)
		& HWA_PAGER_PAGERBM_ERRORS_MASK;
	if (v32) HWA_PRINT("\t pagerbm_errors<0x%08x>\n", v32);

	HWA_PRINT("%s Transaction Id\n", HWApp);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, rx_alloc_transaction_id);
	HWA_PRINT("\t rx_alloc<0x%08x>\n", v32);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, rx_free_transaction_id);
	HWA_PRINT("\t rx_free<0x%08x>\n", v32);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, tx_alloc_transaction_id);
	HWA_PRINT("\t tx_alloc<0x%08x>\n", v32);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, tx_free_transaction_id);
	HWA_PRINT("\t tx_free<0x%08x>\n", v32);

	HWA_PRINT("%s Module Debug\n", HWApp);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, pp_apkt_sts_dbg);
	if (v32) HWA_PRINT("\t alloc<0x%08x>\n", v32);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, pp_rx_apkt_sts_dbg);
	if (v32) HWA_PRINT("\t rx_alloc<0x%08x>\n", v32);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, pp_fpkt_sts_dbg);
	if (v32) HWA_PRINT("\t free<0x%08x>\n", v32);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, pp_tb_sts_dbg);
	if (v32) HWA_PRINT("\t table<0x%08x>\n", v32);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, pp_push_sts_dbg);
	if (v32) HWA_PRINT("\t push<0x%08x>\n", v32);
	v32 = HWA_RD_REG_NAME(HWApp, regs, pager, pp_pull_sts_dbg);
	if (v32) HWA_PRINT("\t pull<0x%08x>\n", v32);

	HWA_TXFIFO_EXPR({
		HWA_PRINT("%s Tx Status\n", HWApp);
		v32 = HWA_RD_REG_NAME(HWApp, regs, txdma, pp_pageout_sts);
		if (v32) HWA_PRINT("\t pp_pageout_sts<0x%08x>\n", v32);
		v32 = HWA_RD_REG_NAME(HWApp, regs, txdma, pp_pagein_sts);
		if (v32) HWA_PRINT("\t pp_pagein_sts<0x%08x>\n", v32);
	});

	HWA_RXFILL_EXPR({
		HWA_PRINT("%s Rx Status\n", HWApp);
		v32 = HWA_RD_REG_NAME(HWApp, regs, rx_core[0], pagein_status);
		if (v32) HWA_PRINT("\t pagein_status<0x%08x>\n", v32);
		v32 = HWA_RD_REG_NAME(HWApp, regs, rx_core[0], recycle_status);
		if (v32) HWA_PRINT("\t recycle_status<0x%08x>\n", v32);
		v32 = HWA_RD_REG_NAME(HWApp, regs, rx_core[0], recycle_cfg);
		if (v32) HWA_PRINT("\t recycle_cfg<0x%08x>\n", v32);
	})

}   // hwa_pktpgr_status()

// HWA Packet Pager Debug Support
#if defined(BCMDBG) || defined(HWA_DUMP)
void // Dump all SW and HWA Packet Pager state
hwa_pktpgr_dump(hwa_pktpgr_t *pktpgr, struct bcmstrbuf *b,
                bool verbose, bool dump_regs, uint8 *fifo_bitmap)
{
	hwa_dev_t *dev;
	hwa_fastdma_t *fastdma;

	HWA_BPRINT(b, "pktpgr dump\n");

	if (pktpgr == (hwa_pktpgr_t*)NULL)
		return;

	// Setup locals
	dev = HWA_DEV(pktpgr);
	fastdma = &dev->fastdma;

	// FastDMA
	HWA_BPRINT(b, "%s fastdma channels[%u] burstlen[%u:%u:%u:%u",
			HWA00, fastdma->channels_max,
			fastdma->burst_length[HWA_FASTDMA_CHANNEL0].tx,
			fastdma->burst_length[HWA_FASTDMA_CHANNEL0].rx,
			fastdma->burst_length[HWA_FASTDMA_CHANNEL1].tx,
			fastdma->burst_length[HWA_FASTDMA_CHANNEL1].rx);
	if (HWAREV_GE(dev->corerev, 133)) {
		HWA_BPRINT(b, ":%u:%u",
			fastdma->burst_length[HWA_FASTDMA_CHANNEL2].tx,
			fastdma->burst_length[HWA_FASTDMA_CHANNEL2].rx);
	}
	HWA_BPRINT(b, ":%u] %s\n", fastdma->burst_length_max,
		fastdma->enabled ? "ENABLED" : "DISABLED");

	// Rings
	hwa_ring_dump(&pktpgr->pagein_req_ring, b, "+ pagein_req");
	hwa_ring_dump(&pktpgr->pagein_rsp_ring, b, "+ pagein_rsp");
	hwa_ring_dump(&pktpgr->pageout_req_ring, b, "+ pageout_req");
	hwa_ring_dump(&pktpgr->pageout_rsp_ring, b, "+ pageout_rsp");
	hwa_ring_dump(&pktpgr->pagemgr_req_ring, b, "+ pagemgr_req");
	hwa_ring_dump(&pktpgr->pagemgr_rsp_ring, b, "+ pagemgr_rsp");
	hwa_ring_dump(&pktpgr->freepkt_req_ring, b, "+ freepkt_req");
	hwa_ring_dump(&pktpgr->freerph_req_ring, b, "+ freerph_req");

	// Table of Host and Dongle Packet Pool context
	HWA_BPRINT(b, "+ HDBM<0x%x_%x> max<%u> avail<%u>\n",
		HADDR64_HI(pktpgr->hostpktpool_haddr64),
		HADDR64_LO(pktpgr->hostpktpool_haddr64),
		pktpgr->hostpktpool_max,
		hwa_pktpgr_dbm_availcnt(dev, HWA_HDBM));
	HWA_BPRINT(b, "+ DDBM<0x%p> max<%u> avail<H:%u,S:%d,lwm:%d> hold<T:%d,"
		"hwm:%d,R:%d,hwm:%d>\n",
		pktpgr->dnglpktpool_mem, pktpgr->dnglpktpool_max,
		hwa_pktpgr_dbm_availcnt(dev, HWA_DDBM),
		pktpgr->ddbm_avail_sw, pktpgr->ddbm_avail_sw_lwm,
		pktpgr->ddbm_sw_tx, pktpgr->ddbm_sw_tx_hwm,
		pktpgr->ddbm_sw_rx, pktpgr->ddbm_sw_rx_hwm);
	// Read clear
	pktpgr->ddbm_avail_sw_lwm = pktpgr->ddbm_avail_sw;
	pktpgr->ddbm_sw_tx_hwm = 0;
	pktpgr->ddbm_sw_rx_hwm = 0;

#if defined(WLTEST) || defined(HWA_DUMP)
	if (dump_regs)
		hwa_pktpgr_regs_dump(pktpgr, b);
#endif

	if (verbose == TRUE) {
		int idx, count;
		uint32 u32;
		hwa_pp_rxlbufpool_t *lbufpool;

#ifdef HWA_QT_TEST
		lbufpool = &pktpgr->rxpktpool;
		HWA_BPRINT(b, "+ rxpktpool: head<0x%p> tail<0x%p> avail<%u> "
			"n_pkts<%u> trans_id<%u>\n", lbufpool->pkt_head,
			lbufpool->pkt_tail, lbufpool->avail, lbufpool->n_pkts,
			lbufpool->trans_id);
#endif

		// SW maintain variables.
		lbufpool = &pktpgr->rxlbufpool;
		HWA_BPRINT(b, "+ rxlbufpool: head<0x%p> tail<0x%p> avail<%u> "
			"n_pkts<%u> trans_id<%u>\n", lbufpool->pkt_head,
			lbufpool->pkt_tail, lbufpool->avail, lbufpool->n_pkts,
			lbufpool->trans_id);
		lbufpool = &pktpgr->txlbufpool;
		HWA_BPRINT(b, "+ txlbufpool: head<0x%p> tail<0x%p> avail<%u> "
			"n_pkts<%u> trans_id<%u>\n", lbufpool->pkt_head,
			lbufpool->pkt_tail, lbufpool->avail, lbufpool->n_pkts,
			lbufpool->trans_id);
#ifdef HNDPQP
		lbufpool = &pktpgr->pqplbufpool;
		HWA_BPRINT(b, "+ pqplbufpool: head<0x%p> tail<0x%p> avail<%u> "
			"n_pkts<%u> trans_id<%u>\n", lbufpool->pkt_head,
			lbufpool->pkt_tail, lbufpool->avail, lbufpool->n_pkts,
			lbufpool->trans_id);
#endif /* HNDPQP */
		HWA_BPRINT(b, "+ tag_pull: head<0x%p> tail<0x%p> req pending<%d> count<%u>\n",
			spktq_peek_head(&pktpgr->tag_pull_q), spktq_peek_tail(&pktpgr->tag_pull_q),
			pktpgr->tag_pull_req, spktq_n_pkts(&pktpgr->tag_pull_q));
		HWA_BPRINT(b, "+ tag_push: req pending<%d>\n", pktpgr->tag_push_req);

		u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[0], d11bdest_ring_wrindex_dir);
		HWA_BPRINT(b, "+ rxpkt: ready<%u> req pending<%d> in_transit<%d>\n",
			BCM_GBF(u32, HWA_RX_D11BDEST_RING_WRINDEX_DIR_OCCUPIED_AFTER_MAC),
			pktpgr->pgi_rxpkt_req, pktpgr->pgi_rxpkt_in_transit);
		HWA_BPRINT(b, "+ localpkt: pgo req pending<%d>\n", pktpgr->pgo_local_req);
		HWA_BPRINT(b, "+ pgi_txs: req_tot pending<%d,hwm:%d>\n", pktpgr->pgi_txs_req_tot,
			pktpgr->pgi_txs_req_tot_hwm);
		pktpgr->pgi_txs_req_tot_hwm = 0; // Read clear
		HWA_BPRINT(b, "+ pgi_fail: txpost<%u> txstatus<%u/%u/%u> rxpkt<%u/%u>\n",
			pktpgr->pgi_txpost_fail, pktpgr->pgi_txstatus_fail,
			pktpgr->pgi_txs_cont_fail, pktpgr->pgi_txstatus_emer,
			pktpgr->pgi_rxpkt_fail, pktpgr->pgi_rx_emer);

		// Read clear
		pktpgr->pgi_txpost_fail = 0;
		pktpgr->pgi_txstatus_fail = 0;
		pktpgr->pgi_rxpkt_fail = 0;

		count = 0;
		if (fifo_bitmap) {
			for (idx = 0; idx < HWA_TX_FIFOS_MAX; idx++) {
				if (isset(fifo_bitmap, idx) &&
					isset(dev->txfifo.fifo_enab, idx) &&
					pktpgr->pgi_txs_req[idx] != 0) {
					if (count == 0)
						HWA_BPRINT(b, "         :");
					count++;
					HWA_BPRINT(b, " req%-2d<%d>", idx,
						pktpgr->pgi_txs_req[idx]);
					if ((count % 8) == 0)
						HWA_BPRINT(b, "\n         :");
				}
			}
		}
		HWA_BPRINT(b, "\n");
	}
}   // hwa_pktpgr_dump()

#if defined(WLTEST) || defined(HWA_DUMP)
// Dump HWA Packet Pager registers
void
hwa_pktpgr_regs_dump(hwa_pktpgr_t *pktpgr, struct bcmstrbuf *b)
{
	hwa_dev_t *dev;
	hwa_regs_t *regs;

	if (pktpgr == (hwa_pktpgr_t*)NULL)
		return;

	dev = HWA_DEV(pktpgr);
	regs = dev->regs;

#define HWA_BPR_PP_RING(b, SNAME) \
	({ \
		HWA_BPR_REG(b, pager, SNAME.addr); \
		HWA_BPR_REG(b, pager, SNAME.wr_index); \
		HWA_BPR_REG(b, pager, SNAME.rd_index); \
		HWA_BPR_REG(b, pager, SNAME.cfg); \
		HWA_BPR_REG(b, pager, SNAME.lazyint_cfg); \
		HWA_BPR_REG(b, pager, SNAME.debug); \
	})

// Skip: following registers as reading has side effect.
// alloc_index, dealloc_index, dealloc_status
#define HWA_BPR_PP_PKTPOOL(b, SNAME) \
	({ \
		HWA_BPR_REG(b, pager, SNAME.addr.lo); \
		HWA_BPR_REG(b, pager, SNAME.addr.hi); \
		HWA_BPR_REG(b, pager, SNAME.ctrl); \
		HWA_BPR_REG(b, pager, SNAME.size); \
		HWA_BPR_REG(b, pager, SNAME.intr_th); \
	})

#define HWA_BPR_PKT_INUSE(b, SNAME) \
	({ \
		HWA_BPR_REG(b, pager, SNAME.all); \
		HWA_BPR_REG(b, pager, SNAME.tx); \
		HWA_BPR_REG(b, pager, SNAME.rx); \
	})

	HWA_BPRINT(b, "%s registers[%p] offset[0x%04x]\n",
		HWApp, &regs->pager, (uint32)OFFSETOF(hwa_regs_t, pager));
	HWA_BPR_REG(b, pager, pp_pager_cfg);
	HWA_BPR_REG(b, pager, pp_pktctx_size);
	HWA_BPR_REG(b, pager, pp_pktbuf_size);
	HWA_BPR_PP_RING(b,    pagein_req_ring);
	HWA_BPR_PP_RING(b,    pagein_rsp_ring);
	HWA_BPR_REG(b, pager, pagein_intstatus);
	HWA_BPR_PP_RING(b,    pageout_req_ring);
	HWA_BPR_PP_RING(b,    pageout_rsp_ring);
	HWA_BPR_REG(b, pager, pageout_intstatus);
	HWA_BPR_PP_RING(b,    pagemgr_req_ring);
	HWA_BPR_PP_RING(b,    pagemgr_rsp_ring);
	HWA_BPR_REG(b, pager, pagemgr_intstatus);
	HWA_BPR_PP_RING(b,    freepkt_req_ring);
	HWA_BPR_PP_RING(b,    freerph_req_ring);
	HWA_BPR_REG(b, pager, rx_alloc_transaction_id);
	HWA_BPR_REG(b, pager, rx_free_transaction_id);
	HWA_BPR_REG(b, pager, tx_alloc_transaction_id);
	HWA_BPR_REG(b, pager, tx_free_transaction_id);
	HWA_BPR_PP_PKTPOOL(b, hostpktpool);
	HWA_BPR_PP_PKTPOOL(b, dnglpktpool);
	HWA_BPR_REG(b, pager, pagerbm_intstatus);
	HWA_BPR_REG(b, pager, pp_dma_descr_template);
	HWA_BPR_REG(b, pager, pp_pagein_req_ddbmth);
	HWA_BPR_REG(b, pager, pp_dma_descr_template_2);
	HWA_BPR_REG(b, pager, pp_apkt_cfg);
	HWA_BPR_REG(b, pager, pp_rx_apkt_cfg);
	HWA_BPR_REG(b, pager, pp_fpkt_cfg);
	HWA_BPR_REG(b, pager, pp_phpl_cfg);
	HWA_BPR_REG(b, pager, pp_apkt_sts_dbg);
	HWA_BPR_REG(b, pager, pp_rx_apkt_sts_dbg);
	HWA_BPR_REG(b, pager, pp_fpkt_sts_dbg);
	HWA_BPR_REG(b, pager, pp_tb_sts_dbg);
	HWA_BPR_REG(b, pager, pp_push_sts_dbg);
	HWA_BPR_REG(b, pager, pp_pull_sts_dbg);
	if (HWAREV_GE(dev->corerev, 133)) {
		HWA_BPR_REG(b, pager, pp_fpkt_sts_dbg2);
		HWA_BPR_REG(b, pager, pp_fpkt_sts_dbg3);
	}
	HWA_BPR_PKT_INUSE(b, hostpkt_cnt);
	HWA_BPR_PKT_INUSE(b, dngltpkt_cnt);
	HWA_BPR_PKT_INUSE(b, hostpkt_hwm);
	HWA_BPR_PKT_INUSE(b, dngltpkt_hwm);
	HWA_BPR_REG(b, pager, pp_pagein_req_stats);
	HWA_BPR_REG(b, pager, pp_pageout_req_stats);
	HWA_BPR_REG(b, pager, pp_pagealloc_req_stats);
	HWA_BPR_REG(b, pager, pp_pagefree_req_stats);
	HWA_BPR_REG(b, pager, pp_pagefreerph_req_stats);
}   // hwa_pktpgr_regs_dump()

#endif
#endif /* BCMDBG */

// PUSH packet from DDBM to HDBM, HW will free pkt in DDBM.
static void // Push request
_hwa_pktpgr_push_req(hwa_dev_t *dev, uint32 fifo_idx, uint8 tagged,
	void *pktlist_head, void *pktlist_tail, uint16 pkt_count,
	uint16 mpdu_count, uint16 prev_pkt_mapid, hwa_pp_pagemgr_cmd_t pagemgr_cmd)
{
	hwa_pp_pagemgr_req_push_t req;
	int trans_id, req_loop;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pkt_count != 0);
	HWA_ASSERT((pagemgr_cmd == HWA_PP_PAGEMGR_PUSH) ||
	           (pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_PKTTAG) ||
	           (pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_MPDU));

#ifdef PSPL_TX_TEST
	if (tagged == HWA_PP_CMD_NOT_TAGGED &&
		bcm_ring_is_full(&dev->pktpgr.pspl_state, dev->pktpgr.pspl_depth)) {
		HWA_ERROR((">>PAGEMGR::REQ PUSH failure: Ring full\n"));
		HWA_ASSERT(0);
	}
#endif /* PSPL_TX_TEST */

	// PUSH Request
	req.trans        = pagemgr_cmd;
	req.pkt_count    = pkt_count;
	req.pktlist_head = (uint32)pktlist_head;
	req.pktlist_tail = (uint32)pktlist_tail;
	req.tagged       = tagged;
#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133) &&
		((pagemgr_cmd == HWA_PP_PAGEMGR_PUSH) ||
	     (pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_PKTTAG))) {
		req.swar = 0xBFBF;
	}
#endif
	// XXX, CRBCAHWA-669
	if (pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_MPDU) {
		req.mpdu_count = mpdu_count;
		req.prev_pkt_mapid = prev_pkt_mapid;
	}

	req_loop = HWA_LOOPCNT;

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
{
	uint16 mpdu;
	void *txlbuf;
	txlbuf = pktlist_head;
	// Free: Push case.
	for (mpdu = 0; mpdu < mpdu_count; mpdu++) {
		hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_2DBM, DBM_AUDIT_FREE);
		if (pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_MPDU) {
			void *next;
			next = PKTNEXT(dev->osh, txlbuf);
			for (; next; next = PKTNEXT(dev->osh, next)) {
				hwa_pktpgr_dbm_audit(dev, next, DBM_AUDIT_TX, DBM_AUDIT_2DBM,
					DBM_AUDIT_FREE);
			}
		}
		txlbuf = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
	}
}
#endif /* HWA_PKTPGR_DBM_AUDIT_ENABLED */

req_again:
	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req);
	if (trans_id == HWA_FAILURE) {
		if (--req_loop > 0) {
			OSL_DELAY(10);
			goto req_again;
		}
		HWA_ERROR((">>PAGEMGR::REQ PUSH failure: pkts<%d> list[%p(%d) .. %p(%d)]\n",
			pkt_count, pktlist_head, PKTMAPID(pktlist_head), pktlist_tail,
			PKTMAPID(pktlist_tail)));
		HWA_ASSERT(0);
		return;
	}

#ifdef PSPL_TX_TEST
	if (tagged == HWA_PP_CMD_NOT_TAGGED) {
		_hwa_pktpgr_pspl_test_push_req_enq(dev, trans_id, fifo_idx,
			pktlist_head, pktlist_tail, pkt_count, mpdu_count,
			prev_pkt_mapid, pagemgr_cmd);
	} else
#endif /* PSPL_TX_TEST */
	{
		HWA_ASSERT(tagged == HWA_PP_CMD_TAGGED);

		HWA_COUNT_INC(dev->pktpgr.tag_push_req, 1);

		HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  >>PAGEMGR::REQ%sPUSH(tagged) : pkts %3u "
			"list[0x%p(%d) .. 0x%p(%d)] fifo %3u ==PUSH-REQ(%d)==>\n\n",
			(pagemgr_cmd == HWA_PP_PAGEMGR_PUSH_PKTTAG) ? " PKTTAG " : " ",
			pkt_count, pktlist_head, PKTMAPID(pktlist_head),
			pktlist_tail, PKTMAPID(pktlist_tail), fifo_idx, trans_id);
	}
}   // _hwa_pktpgr_push_req()

// PUSH packet from DDBM to HDBM, HW will free pkt in DDBM.
void // Push request
hwa_pktpgr_push_req(hwa_dev_t *dev, uint32 fifo_idx, uint8 tagged,
	void *pktlist_head, void *pktlist_tail, uint16 pkt_count,
	uint16 mpdu_count)
{
	_hwa_pktpgr_push_req(dev, fifo_idx, tagged, pktlist_head, pktlist_tail,
		pkt_count, mpdu_count, HWA_PP_PKT_MAPID_INVALID, HWA_PP_PAGEMGR_PUSH);
}

// PUSH pkttag part of packet from DDBM to HDBM, HW will free pkt in DDBM.
void // PUSH pkttag part
hwa_pktpgr_push_pkttag_req(hwa_dev_t *dev, uint32 fifo_idx, uint8 tagged,
	void *pktlist_head, void *pktlist_tail, uint16 pkt_count,
	uint16 mpdu_count)
{
	_hwa_pktpgr_push_req(dev, fifo_idx, tagged, pktlist_head, pktlist_tail,
		pkt_count, mpdu_count, HWA_PP_PKT_MAPID_INVALID, HWA_PP_PAGEMGR_PUSH_PKTTAG);
}

// PUSH packet from DDBM to HDBM, HW will free pkt in DDBM.
void // Push MPDU request
hwa_pktpgr_push_mpdu_req(hwa_dev_t *dev, uint32 fifo_idx, uint8 tagged,
	void *pktlist_head, void *pktlist_tail, uint16 pkt_count,
	uint16 mpdu_count, uint16 prev_pkt_mapid)
{
	_hwa_pktpgr_push_req(dev, fifo_idx, tagged, pktlist_head, pktlist_tail,
		pkt_count, mpdu_count, prev_pkt_mapid, HWA_PP_PAGEMGR_PUSH_MPDU);
}

uint8 // HWApp: Get trans_id from specific pktpgr ring
hwa_pktpgr_get_trans_id(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	pktpgr = &dev->pktpgr;

	HWA_INFO(("%s, trans_id %d\n", __FUNCTION__, pktpgr->trans_id[pp_ring]));

	return pktpgr->trans_id[pp_ring];
}   // hwa_pktpgr_get_trans_id()

void // HWApp: Update trans_id to specific pktpgr ring
hwa_pktpgr_update_trans_id(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring, uint8 trans_id)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	pktpgr = &dev->pktpgr;
	pktpgr->trans_id[pp_ring] = trans_id;

	HWA_INFO(("%s, trans_id %d\n", __FUNCTION__, pktpgr->trans_id[pp_ring]));
}

void // HWApp: Update wr_index to specific pktpgr ring
hwa_pktpgr_update_ring_wr_index(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring, uint16 wr_index)
{
	hwa_pktpgr_t *pktpgr;
	hwa_ring_t   *req_ring;

	HWA_INFO(("%s, wr_index <%d>\n", __FUNCTION__, wr_index));

	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	pktpgr = &dev->pktpgr;
	req_ring = pktpgr->req_ring[pp_ring];

	hwa_ring_prod_replace(req_ring, wr_index, TRUE);
}

// Wait for number of "countes" empty slots is available for specific pktpgr request ring
bool
hwa_pktpgr_req_ring_wait_for_avail(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring,
	uint32 counts, bool wait)
{
	hwa_pktpgr_t *pktpgr;
	hwa_ring_t   *req_ring;
	uint32 loop_count;
	int avail;

	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	pktpgr = &dev->pktpgr;
	req_ring = pktpgr->req_ring[pp_ring];
	HWA_ASSERT(counts < req_ring->depth);
	loop_count = HWA_FSM_IDLE_POLLLOOP;

	// Flushes WR index to HWA
	hwa_ring_prod_put(req_ring);
	// Update HW RD index from HWA
	hwa_ring_prod_get(req_ring);
	// Available empty slots
	avail = hwa_ring_prod_avail(req_ring);

	HWA_INFO(("Does \"%s\" ring have %d empty slots?", req_ring->name, counts));

	// Enough empty slots.
	if (avail >= counts) {
		HWA_INFO((" [Y..%d]\n", avail));
		return TRUE;
	}

	// No,
	if (!wait) {
		HWA_INFO((" [N..%d]\n", avail));
		return FALSE;
	}

	// No but wait until requested pp ring can have number of counts available
	HWA_INFO(("  Wait...\n"));
	while (avail < counts) {
		HWA_TRACE(("%s HWA consuming pp ring<%d>\n", __FUNCTION__, pp_ring));
		OSL_DELAY(1);
		if (--loop_count == 0) {
			HWA_ERROR(("%s Cannot consume pp ring<%d> for avail counts<%d>\n",
				__FUNCTION__, pp_ring, counts));
			break;
		}
		// Update HW RD
		hwa_ring_prod_get(req_ring);
		avail = hwa_ring_prod_avail(req_ring);
	}

	HWA_INFO(("  [%s] I have %d empty slots\n", loop_count ? "Y" : "N", avail));

	return (loop_count != 0);
}

// Check if number of "counts" empty slots is available for specific pktpgr response ring
bool
hwa_pktpgr_rsp_ring_avail_for_reqcnt(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring,
	uint32 counts)
{
	hwa_pktpgr_t *pktpgr;
	hwa_ring_t   *rsp_ring;
	int avail;

	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_rsp_ring_max);

	pktpgr = &dev->pktpgr;
	rsp_ring = pktpgr->rsp_ring[pp_ring];
	HWA_ASSERT(counts < rsp_ring->depth);

	// fetch response ring's WR index once
	hwa_ring_cons_get(rsp_ring);

	// Available empty slots for HWA
	avail = hwa_ring_prod_avail(rsp_ring);

	HWA_INFO(("Does \"%s\" ring have %d empty slots?", rsp_ring->name, counts));

	// Check empty slots.
	if (avail >= counts) {
		HWA_INFO((" [Y..%d]\n", avail));
		return TRUE;
	} else {
		HWA_INFO((" [N..%d]\n", avail));
		return FALSE;
	}
}

static inline int
__hwa_pktpgr_freepkt(hwa_dev_t *dev, hwa_pp_lbuf_t *pktlist_head,
	hwa_pp_lbuf_t *pktlist_tail, int pkt_count, hwa_pp_pagemgr_cmd_t pagemgr_cmd,
	const char *pagemgr_cmd_str)
{
	int trans_id, req_loop;
	hwa_pp_freepkt_req_t req;

	HWA_ASSERT(pktlist_head != NULL);

	// Construct a request
	req.trans        = pagemgr_cmd;
	req.pkt_count    = pkt_count;
	req.pktlist_head = (uint32)pktlist_head;
	req.pktlist_tail = (uint32)pktlist_tail;

	req_loop = HWA_LOOPCNT;

req_again:
	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_freepkt_req_ring, &req);
	if (trans_id == HWA_FAILURE) {
		if (--req_loop > 0) {
			OSL_DELAY(10);
			goto req_again;
		}
		HWA_ERROR((">>PAGEMGR::REQ %s failure: pkts<%d> list[%p(%d) .. %p(%d)]\n",
			pagemgr_cmd_str, pkt_count, pktlist_head, PKTMAPID(pktlist_head),
			pktlist_tail, PKTMAPID(pktlist_tail)));

#if defined(WLTEST) || defined(HWA_DUMP)
{
		uint8 fifo_bitmap[(128/NBBY)+1];
		memset(fifo_bitmap, 0, sizeof(fifo_bitmap));
		setbit(fifo_bitmap, 65);
		hwa_pktpgr_dump(&dev->pktpgr, NULL, TRUE, TRUE, fifo_bitmap);
}
#endif

		HWA_ASSERT(0);
	}

	return trans_id;
}

static int
_hwa_pktpgr_freepkt(hwa_dev_t *dev, hwa_pp_lbuf_t *pktlist_head,
	hwa_pp_lbuf_t *pktlist_tail, int pkt_count, hwa_pp_pagemgr_cmd_t pagemgr_cmd,
	const char *pagemgr_cmd_str)
{
	int trans_id;
	hwa_pktpgr_t *pktpgr;
#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
	int i;
	hwa_pp_lbuf_t *pkt;
#endif

	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(pkt_count != 0);
	HWA_ASSERT(pagemgr_cmd == HWA_PP_PAGEMGR_FREE_RX ||
		pagemgr_cmd == HWA_PP_PAGEMGR_FREE_TX ||
		pagemgr_cmd == HWA_PP_PAGEMGR_FREE_DDBM_LINK);
	HWA_ASSERT(pagemgr_cmd_str);

	if (pkt_count == 0) {
		HWA_ERROR(("%s: pkt_count is 0 from %s request\n",
			__FUNCTION__, pagemgr_cmd_str));
		return HWA_FAILURE;
	}

	if (PKTLINK(pktlist_tail) != 0) {
		HWA_INFO(("%s: pktlist_tail @ %p is not NULL\n",
			__FUNCTION__, PKTLINK(pktlist_tail)));
	}

	// Setup locals
	pktpgr = &dev->pktpgr;
	trans_id = HWA_FAILURE;

#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
	pkt = pktlist_head;
	for (i = 0; i < pkt_count; i++) {
		// Free: Normal case.
		hwa_pktpgr_dbm_audit(dev, pkt, (pagemgr_cmd == HWA_PP_PAGEMGR_FREE_TX),
			DBM_AUDIT_2DBM, DBM_AUDIT_FREE);
		pkt = (hwa_pp_lbuf_t *)PKTLINK(pkt);
	}
#endif /* HWA_PKTPGR_DBM_AUDIT_ENABLED */

	trans_id = __hwa_pktpgr_freepkt(dev, pktlist_head, pktlist_tail,
		pkt_count, pagemgr_cmd, pagemgr_cmd_str);

	// DDBM accounting.
	// XXX No resp, be careful the latancy.
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, pkt_count);
	if (pagemgr_cmd == HWA_PP_PAGEMGR_FREE_RX)
		HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_rx, pkt_count);
	else
		HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_tx, pkt_count);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: + %3u : %d @ %s (%s)\n", pkt_count,
		pktpgr->ddbm_avail_sw, __FUNCTION__, pagemgr_cmd_str);

	HWA_PP_DBG(HWA_PP_DBG_MGR, "  >>PAGEMGR::REQ %s      : pkts<%d> "
		"list[%p(%d) .. %p(%d)] ==%s-REQ(%d), no rsp ==>\n\n",
		pagemgr_cmd_str, pkt_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), pagemgr_cmd_str, trans_id);

	return trans_id;
} // _hwa_pktpgr_freepkt

// Free RxLfrag. No RSP
void
hwa_pktpgr_free_rx(struct hwa_dev *dev, hwa_pp_lbuf_t *pktlist_head,
	hwa_pp_lbuf_t * pktlist_tail, int pkt_count)
{
	_hwa_pktpgr_freepkt(dev, pktlist_head, pktlist_tail, pkt_count,
		HWA_PP_PAGEMGR_FREE_RX, "FREE_RX");
}

// Free RxPostHostInfo. No RSP
void
hwa_pktpgr_free_rph(struct hwa_dev *dev, uint32 host_pktid,
	uint16 host_datalen, dma64addr_t data_buf_haddr64)
{
	int trans_id, req_loop;
	hwa_pp_freerph_req_t req;

	HWA_AUDIT_DEV(dev);

	req.trans            = HWA_PP_PAGEMGR_FREE_RPH;
	req.host_datalen     = host_datalen;
	req.host_pktid       = host_pktid;
	req.data_buf_haddr64 = data_buf_haddr64;

	req_loop = HWA_LOOPCNT;

req_again:
	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_freerph_req_ring, &req);
	if (trans_id == HWA_FAILURE) {
		if (--req_loop > 0) {
			OSL_DELAY(10);
			goto req_again;
		}
		HWA_ERROR((">>PAGEMGR::REQ FREE_RPH failure : pktid<%u> "
			"haddr<0x%08x:0x%08x> len<%u>\n",
			host_pktid, data_buf_haddr64.hiaddr,
			data_buf_haddr64.loaddr, host_datalen));
		HWA_ASSERT(0);
		return;
	}

	HWA_PP_DBG(HWA_PP_DBG_MGR_RX, "  >>PAGEMGR::REQ FREE_RPH     : pktid<%u> "
		"haddr<0x%08x:0x%08x> len<%u> ==FREERPH-REQ(%d), no rsp ==>\n\n",
		host_pktid, data_buf_haddr64.hiaddr,
		data_buf_haddr64.loaddr, host_datalen, trans_id);
}   // hwa_pktpgr_free_rph()

// Free TxLfrag. No RSP
void
hwa_pktpgr_free_tx(struct hwa_dev *dev, hwa_pp_lbuf_t *pktlist_head,
	hwa_pp_lbuf_t * pktlist_tail, int pkt_count)
{
	_hwa_pktpgr_freepkt(dev, pktlist_head, pktlist_tail, pkt_count,
		HWA_PP_PAGEMGR_FREE_TX, "FREE_TX");
}

// Free DDBM in pktlist. No RSP
void
hwa_pktpgr_free_ddbm_pkt(hwa_dev_t *dev, hwa_pp_lbuf_t *pktlist_head,
	hwa_pp_lbuf_t * pktlist_tail, int pkt_count)
{
	_hwa_pktpgr_freepkt(dev, pktlist_head, pktlist_tail, pkt_count,
		HWA_PP_PAGEMGR_FREE_DDBM_LINK, "FREE_DDBM");
}

static void
_hwa_pktpgr_lbufpool_fill(hwa_dev_t *dev, hwa_pp_lbufpool_t *lbufpool,
	hwa_pp_pagemgr_cmd_t pagemgr_cmd, const char *pagemgr_cmd_str)
{
	int trans_id;
	hwa_pp_pagemgr_req_alloc_t req;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(pagemgr_cmd == HWA_PP_PAGEMGR_ALLOC_RX_RPH ||
		pagemgr_cmd == HWA_PP_PAGEMGR_ALLOC_TX);
	HWA_ASSERT(pagemgr_cmd_str);

	HWA_ASSERT(lbufpool->n_pkts <= lbufpool->max_pkts);
	if (lbufpool->n_pkts >= lbufpool->max_pkts)
		return;

	// Request resources to refill lbufpool
	req.trans      = pagemgr_cmd;
	req.pkt_count  = (lbufpool->max_pkts - lbufpool->n_pkts);
	req.tagged     = HWA_PP_CMD_NOT_TAGGED;

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133)) {
		req.swar = 0xBFBFBFBF;
	}
#endif

	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req);
	if (trans_id != HWA_FAILURE) {
		lbufpool->n_pkts += req.pkt_count;
		lbufpool->trans_id = trans_id;
	}

	// DDBM accounting.
	// Thought to handle pkt_count is zero in resp.
	// 1. Pre-reserved HWA_PP_LBUFPOOL_LEN_MAX * 2.
	// 2. Account it in resp rather than in req.

	HWA_PP_DBG(HWA_PP_DBG_MGR, "  >>PAGEMGR::REQ %s     : "
		"pkt_count<%u> ==%s-REQ(%d)==>\n\n", pagemgr_cmd_str,
		req.pkt_count, pagemgr_cmd_str, trans_id);
}

// ###############
// ## RX: hwa_pcie.c ##
// ###############

void
hwa_rxpath_rxlbufpool_fill(hwa_dev_t *dev)
{
	_hwa_pktpgr_lbufpool_fill(dev, &dev->pktpgr.rxlbufpool,
		HWA_PP_PAGEMGR_ALLOC_RX_RPH, "ALLOCRXRPH");
}

// PAGEMGR_ALLOC_RX_RPH give us both PP_LBUF and RPH.
// Free RxLfrag which we don't use it.
int
hwa_rph_allocate(uint32 *bufid, uint16 *len, dma64addr_t *haddr64, bool pre_req)
{
	hwa_dev_t *dev;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbuf_t *rxlbuf;
	hwa_pp_rxlbufpool_t *rxlbufpool;

	HWA_FTRACE(HWApp);
	dev = HWA_DEVP(TRUE);

	// Setup locals
	pktpgr = &dev->pktpgr;
	rxlbufpool = &pktpgr->rxlbufpool;

	// Check pre_req case.
	if (pre_req) {
		if (rxlbufpool->avail == 0) {
			hwa_rxpath_rxlbufpool_fill(dev);
			return HWA_FAILURE;
		}

		return HWA_SUCCESS;
	}

	// Take one rxlbuf;
	ASSERT(rxlbufpool->pkt_head != NULL);
	rxlbuf = rxlbufpool->pkt_head;
	rxlbufpool->pkt_head = (hwa_pp_lbuf_t *)PKTLINK(rxlbuf);

	if (rxlbufpool->pkt_head == NULL) {
		rxlbufpool->pkt_tail = NULL;
	}

	// Terminate next
	HWAPKTSETLINK(rxlbuf, NULL);

	// Copy out the RPH
	*bufid = RPH_HOSTPKTID(rxlbuf);
	*len = RPH_HOSTDATALEN(rxlbuf);
	*haddr64 = RPH_HOSTADDR64(rxlbuf);
	if (dev->host_addressing & HWA_32BIT_ADDRESSING) {
		// 32bit host
		HWA_ASSERT(HADDR64_HI(*haddr64) == dev->host_physaddrhi);
	}

	HWA_TRACE(("%s rph alloc pktid<%u> len <%u> haddr<0x%08x:0x%08x>\n",
		HWA1x, *bufid, *len, HADDR64_HI(*haddr64), HADDR64_LO(*haddr64)));

	// Free RxLfrag which we don't use it.
	hwa_pktpgr_free_rx(dev, rxlbuf, rxlbuf, 1);

	rxlbufpool->avail--;
	rxlbufpool->n_pkts--;

	// Refill rxlbufpool
	hwa_rxpath_rxlbufpool_fill(dev);

	return HWA_SUCCESS;
}

#ifdef HWA_QT_TEST
void
hwa_rxpktpool_add(hwa_dev_t *dev, uint16 count, uint16 times)
{
	int trans_id;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pagemgr_req_alloc_t req;
	hwa_pp_rxlbufpool_t *rxpktpool;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	if (times == 0)
		return;

	// Setup locals
	pktpgr = &dev->pktpgr;
	rxpktpool = &pktpgr->rxpktpool;

	// Refill rxpktpool
	req.trans      = HWA_PP_PAGEMGR_ALLOC_RX;
	req.tagged     = HWA_PP_CMD_NOT_TAGGED;
	req.pkt_count  = count;
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133)) {
		req.swar = 0xBFBFBFBF;
	}

#ifdef PKTPGR_ALLOC_LOOP_TEST
	if (pktpgr->rx_alt_en) {
		int loop;
		// First times - 1 requests w/o WR commit
		g_pktpgr_req_commit_delay = TRUE;
		for (loop = 1; loop < times; loop++) {
			trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req);
			if (trans_id != HWA_FAILURE) {
				rxpktpool->n_pkts += req.pkt_count;
				rxpktpool->trans_id = trans_id;
				pktpgr->rx_alt_times_n++;
			} else {
				HWA_ASSERT(0);
			}
		}
		g_pktpgr_req_commit_delay = FALSE;
	}
#endif /* PKTPGR_ALLOC_LOOP_TEST */

	// Last one request w/ WR commit
	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req);
	if (trans_id != HWA_FAILURE) {
		rxpktpool->n_pkts += req.pkt_count;
		rxpktpool->trans_id = trans_id;
#ifdef PKTPGR_ALLOC_LOOP_TEST
		if (pktpgr->rx_alt_en) {
			pktpgr->rx_alt_times_n++;
		}
#endif
	} else {
		HWA_ASSERT(0);
	}

	HWA_PP_DBG(HWA_PP_DBG_MGR, "  >>PAGEMGR::REQ ALLOC_RX     : "
		"pkt_count<%u> ==ALLOCRX-REQ(%d)==>\n\n", req.pkt_count, trans_id);
}

int
hwa_rxpktpool_del(hwa_dev_t *dev, uint16 count)
{
	int i;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbuf_t *head, *tail, *curr;
	hwa_pp_rxlbufpool_t *rxpktpool;

	HWA_FTRACE(HWApp);

	// Setup locals
	pktpgr = &dev->pktpgr;
	rxpktpool = &pktpgr->rxpktpool;

	// Check pre_req case.
	if (rxpktpool->avail < count)
		return HWA_FAILURE;

	// Take one rxlbuf;
	ASSERT(rxpktpool->pkt_head != NULL);
	head = curr = rxpktpool->pkt_head;
	for (i = 1; i < count; i++)
		curr = (hwa_pp_lbuf_t *)PKTLINK(curr);
	tail = curr;
	rxpktpool->pkt_head = (hwa_pp_lbuf_t *)PKTLINK(curr);

	HWA_PP_DBG(HWA_PP_DBG_MGR, "Free <%u> packet to DDBM\n", count);

	rxpktpool->avail -= count;
	rxpktpool->n_pkts -= count;

	// Free RxLfrag which we don't use it.
	PKTSETLINK(tail, NULL);
	hwa_pktpgr_free_rx(dev, head, tail, count);

	return HWA_SUCCESS;
}

static void
hwa_rxpath_pagemgr_alloc_rx_rsp(hwa_dev_t *dev, uint8 pktpgr_trans_id, int pkt_count,
	hwa_pp_lbuf_t * pktlist_head, hwa_pp_lbuf_t * pktlist_tail)
{
	uint16 pkt = 0;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_rxlbufpool_t *rxpktpool;
	hwa_pp_lbuf_t *rxlbuf = pktlist_head;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	rxpktpool = &pktpgr->rxpktpool;

	HWA_PP_DBG(HWA_PP_DBG_MGR, "  <<PAGEMGR::RSP PAGEMGR_ALLOC_RX : pkts %3u "
		"list[%p(%d) .. %p(%d)] id %u <==ALLOCRX-RSP(%d)==\n\n",
		pkt_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), pktpgr_trans_id, pktpgr_trans_id);

	if (pkt_count == 0) {
		HWA_ERROR(("%s: pkt_count is 0\n", __FUNCTION__));
		// Sync avail and n_pkts at last rsp.
		HWA_ASSERT(rxpktpool->avail < rxpktpool->n_pkts);
		if (rxpktpool->trans_id == pktpgr_trans_id &&
			rxpktpool->n_pkts != rxpktpool->avail) {
			rxpktpool->n_pkts = rxpktpool->avail;
		}
		return;
	}

	// Sanity check.
	HWA_ASSERT(PKTLINK(pktlist_tail) == NULL);
	for (pkt = 0; pkt < pkt_count - 1; ++pkt) {
		// No RPH, host_pktid field is garbage
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
		// Alloc: SW allocation case
		hwa_pktpgr_dbm_audit(dev, rxlbuf, DBM_AUDIT_RX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif
		rxlbuf = (hwa_pp_lbuf_t *)PKTLINK(rxlbuf);
	}
	HWA_ASSERT(rxlbuf == pktlist_tail);
	// No RPH, host_pktid field is garbage
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
	// Alloc: SW allocation case
	hwa_pktpgr_dbm_audit(dev, rxlbuf, DBM_AUDIT_RX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif

	// Add to rxpktpool
	if (rxpktpool->pkt_head == NULL) {
		rxpktpool->pkt_head = pktlist_head;
		rxpktpool->pkt_tail = pktlist_tail;
	} else {
		PKTSETLINK(rxpktpool->pkt_tail, pktlist_head);
		rxpktpool->pkt_tail = pktlist_tail;
	}
	rxpktpool->avail += pkt_count;

	// DDBM accounting.
	HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, pkt_count);
	HWA_DDBM_COUNT_LWM(pktpgr->ddbm_avail_sw, pktpgr->ddbm_avail_sw_lwm);
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_rx, pkt_count);
	HWA_DDBM_COUNT_HWM(pktpgr->ddbm_sw_rx, pktpgr->ddbm_sw_rx_hwm);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM-RX: - %3u : %d/%d @ %s\n", pkt_count,
		pktpgr->ddbm_avail_sw, pktpgr->ddbm_sw_rx, __FUNCTION__);

	// Sync avail and n_pkts at last rsp.
	HWA_ASSERT(rxpktpool->avail <= rxpktpool->n_pkts);
	if (rxpktpool->trans_id == pktpgr_trans_id &&
		rxpktpool->n_pkts != rxpktpool->avail) {
		rxpktpool->n_pkts = rxpktpool->avail;
	}

#ifdef PKTPGR_ALLOC_LOOP_TEST
	if (pktpgr->rx_alt_en) {
		pktpgr->rx_alt_times_n--;
		if (pktpgr->rx_alt_times_n == 0) {
			hwa_rxpktpool_del(dev, rxpktpool->n_pkts);
			hwa_rxpktpool_add(dev, pktpgr->rx_alt_count, pktpgr->rx_alt_times);
		}
	}
#endif

	return;
}
#endif /* HWA_QT_TEST */

HWA_PP_DBG_EXPR(static uint32 g_rph_starvation = 0);

void
hwa_rxpath_pagemgr_alloc_rx_rph_rsp(hwa_dev_t *dev, uint8 pktpgr_trans_id, int pkt_count,
	hwa_pp_lbuf_t * pktlist_head, hwa_pp_lbuf_t * pktlist_tail)
{
	uint16 pkt = 0;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_rxlbufpool_t *rxlbufpool;
	hwa_pp_lbuf_t *rxlbuf = pktlist_head;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	rxlbufpool = &pktpgr->rxlbufpool;

	HWA_PP_DBG(HWA_PP_DBG_MGR_RX, "  <<PAGEMGR::RSP PAGEMGR_ALLOC_RX_RPH : pkts %3u "
		"list[%p(%d) .. %p(%d)] id %u <==ALLOCRXRPH-RSP(%d)==\n\n",
		pkt_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), pktpgr_trans_id, pktpgr_trans_id);

	if (pkt_count == 0) {
		HWA_ERROR(("%s: pkt_count is 0\n", __FUNCTION__));

		HWA_PP_DBG_EXPR({
			if (rxlbufpool->avail == 0) {
				g_rph_starvation++;
				HWA_ERROR(("  g_rph_starvation<%d>\n", g_rph_starvation));

				// Dump Host info
				hwa_wlc_mac_event(dev, WLC_E_HWA_DHD_DUMP);

#if defined(BCMDBG) || defined(HWA_DUMP)
				// Dump Dongle info
				hwa_pktpgr_dump(pktpgr, NULL, TRUE, TRUE, NULL);
				hwa_cpleng_dump(&dev->cpleng, NULL, TRUE, TRUE);
#endif
				HWA_ASSERT(g_rph_starvation < 3);
			}
		});

		// Sync avail and n_pkts at last rsp.
		HWA_ASSERT(rxlbufpool->avail < rxlbufpool->n_pkts);
		if (rxlbufpool->trans_id == pktpgr_trans_id &&
			rxlbufpool->n_pkts != rxlbufpool->avail) {
			rxlbufpool->n_pkts = rxlbufpool->avail;
		}
		return;
	}

	// Reset g_rph_starvation
	HWA_PP_DBG_EXPR(g_rph_starvation = 0);

	// Sanity check.
	HWA_ASSERT(PKTLINK(pktlist_tail) == NULL);
	for (pkt = 0; pkt < pkt_count - 1; ++pkt) {
		HWA_ASSERT(RPH_HOSTPKTID(rxlbuf) != 0xFFFF);
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
		// Alloc: SW allocation case
		hwa_pktpgr_dbm_audit(dev, rxlbuf, DBM_AUDIT_RX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif
		rxlbuf = (hwa_pp_lbuf_t *)PKTLINK(rxlbuf);
	}
	HWA_ASSERT(rxlbuf == pktlist_tail);
	HWA_ASSERT(RPH_HOSTPKTID(rxlbuf) != 0xFFFF);
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
	// Alloc: SW allocation case
	hwa_pktpgr_dbm_audit(dev, rxlbuf, DBM_AUDIT_RX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif

	// Add to rxlbufpool
	if (rxlbufpool->pkt_head == NULL) {
		rxlbufpool->pkt_head = pktlist_head;
		rxlbufpool->pkt_tail = pktlist_tail;
	} else {
		PKTSETLINK(rxlbufpool->pkt_tail, pktlist_head);
		rxlbufpool->pkt_tail = pktlist_tail;
	}
	rxlbufpool->avail += pkt_count;

	// DDBM accounting.
	// Thought to handle pkt_count is zero in resp.
	// 1. Pre-reserved HWA_PP_LBUFPOOL_LEN_MAX * 2.
	// 2. Account it in resp rather than in req.
	HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, pkt_count);
	HWA_DDBM_COUNT_LWM(pktpgr->ddbm_avail_sw, pktpgr->ddbm_avail_sw_lwm);
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_rx, pkt_count);
	HWA_DDBM_COUNT_HWM(pktpgr->ddbm_sw_rx, pktpgr->ddbm_sw_rx_hwm);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", pkt_count,
		pktpgr->ddbm_avail_sw, __FUNCTION__);

	// Sync avail and n_pkts at last rsp.
	HWA_ASSERT(rxlbufpool->avail <= rxlbufpool->n_pkts);
	if (rxlbufpool->trans_id == pktpgr_trans_id &&
		rxlbufpool->n_pkts != rxlbufpool->avail) {
		rxlbufpool->n_pkts = rxlbufpool->avail;
	}

	return;
}

// ###############
// ## TX: hwa_pcie.c ##
// ###############
void
hwa_txpost_txlbufpool_fill(hwa_dev_t *dev)
{
	_hwa_pktpgr_lbufpool_fill(dev, &dev->pktpgr.txlbufpool,
		HWA_PP_PAGEMGR_ALLOC_TX, "ALLOCTX");
}

void * // Handle SW allocate tx buffer from TxBM. Return txpost buffer || NULL
hwa_txpost_txbuffer_get(struct hwa_dev *dev)
{
	uint16 pkt_mapid;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbuf_t *txlbuf;
	hwa_pp_txlbufpool_t *txlbufpool;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	txlbufpool = &pktpgr->txlbufpool;

	if (txlbufpool->avail == 0) {
		hwa_txpost_txlbufpool_fill(dev);
		return NULL;
	}

	ASSERT(txlbufpool->pkt_head != NULL);
	txlbuf = txlbufpool->pkt_head;
	txlbufpool->pkt_head = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);

	if (txlbufpool->pkt_head == NULL) {
		txlbufpool->pkt_tail = NULL;
	}

	// Clear context 128B except pkt_mapid.
	pkt_mapid = PKTMAPID(txlbuf);
	bzero(txlbuf, LBUFSZ);
	PKTMAPID(txlbuf) = pkt_mapid;
	// Clear repurpose latest 7 words of databuffer
	bzero(&txlbuf->txreset, HWA_PP_PKTDATABUF_TXFRAG_RESV_BYTES);

	/* Mark packet as TXFRAG and HWAPKT */
	PKTSETTXFRAG(dev->osh, txlbuf);
	PKTSETHWAPKT(dev->osh, txlbuf);
	PKTRESETHWA3BPKT(dev->osh, txlbuf);

	/* Set data point */
	PKTSETTXBUF(dev->osh, txlbuf, PKTPPBUFFERP(txlbuf), HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES);

	txlbufpool->avail--;
	txlbufpool->n_pkts--;

	// Refill txlbufpool
	hwa_txpost_txlbufpool_fill(dev);

	return txlbuf;
}

#if defined(HNDPQP) && defined(HWA_PKTPGR_BUILD)

/**
 *  PQP Lbuf Pool management.
 *
 *  Each DBM packet in the pqplbufpool has a paired HBM packet.
 *  Packets allocated from pqplbufpool are NOT cleared as PQP will explicitly
 *  DMA into them, to overwrite the lbuf context and data buffer.
 *
 *  hwa_pktpgr_t::pqplbufpool is pre-filled in hwa_up(), see hwa_mac.c
 */

void // Schedule a HWA_PP_PAGEMGR_ALLOC_TX request to replenish pqplbufpool
hwa_pktpgr_pqplbufpool_fill(hwa_dev_t *dev)
{
	int trans_id;
	uint16 pkt_count, ppddavailcnt;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pagemgr_req_alloc_t req;
	hwa_pp_pqplbufpool_t *pqplbufpool;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	pqplbufpool = &pktpgr->pqplbufpool;

	HWA_ASSERT(pqplbufpool->n_pkts <= pqplbufpool->max_pkts);
	if (pqplbufpool->n_pkts >= pqplbufpool->max_pkts)
		return;

	// Pqp is similar to TxPost, do resource checking.
	// Force using resv DDBM to fill pqplbufpool if shared DDBM is not enough.
	pkt_count = pqplbufpool->max_pkts - pqplbufpool->n_pkts;
	if (pktpgr->ddbm_avail_sw < pkt_count) {
		if (pktpgr->ddbm_avail_sw > 0) {
			pkt_count = pktpgr->ddbm_avail_sw;
		} else if (pktpgr->pqplbuf_use_rsv_ddbm &&
			(pqplbufpool->n_pkts < PQP_PGI_DBM_THRESH)) {
			ppddavailcnt = hwa_pktpgr_dbm_availcnt(dev, HWA_DDBM);
			pkt_count = MIN((PQP_PGI_DBM_THRESH - pqplbufpool->n_pkts),
				ppddavailcnt);
			HWA_INFO(("%s: use resv ddbm, ppddavailcnt<%u> pkt_count<%u>\n",
				__FUNCTION__, ppddavailcnt, pkt_count));
			if (pkt_count == 0) {
				return;
			}
		} else {
			HWA_INFO(("%s: failed. ddbm_avail_sw<%u> ddbm_sw_tx<%u> pkt_count<%u>\n",
				__FUNCTION__, pktpgr->ddbm_avail_sw, pktpgr->ddbm_sw_tx,
				pkt_count));
			return;
		}
	}

	// Refill pqplbufpool
	req.trans      = HWA_PP_PAGEMGR_ALLOC_TX;
	req.pkt_count  = pkt_count;
	req.tagged     = HWA_PP_CMD_TAGGED;

	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req);
	if (trans_id != HWA_FAILURE) {
		HWA_COUNT_INC(pktpgr->pqplbuf_alloc_req, 1);
		pqplbufpool->n_pkts  += req.pkt_count;
		pqplbufpool->trans_id = trans_id;
	}

	HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  >>PAGEMGR::REQ ALLOC_TX PQP : "
		"pkt_count<%u> ==ALLOCTX-REQ(%d)==>\n\n", req.pkt_count, trans_id);

}   // hwa_pktpgr_pqplbufpool_fill()

int  // Determine the number of packets readilly available in pqplbufpool
hwa_pqp_pkt_cnt(struct hwa_dev *dev)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	pktpgr = &dev->pktpgr;

	return pktpgr->pqplbufpool.avail;

}   // hwa_pqp_pkt_cnt()

void * // Allocate a packet from pqplbufpool, scheduling a refill if required
hwa_pqp_pkt_get(struct hwa_dev *dev)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbuf_t *pqp_lbuf;
	hwa_pp_pqplbufpool_t *pqplbufpool;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	pktpgr = &dev->pktpgr;
	pqplbufpool = &pktpgr->pqplbufpool;

	if (pqplbufpool->avail <= HWA_PP_PQPLBUFPOOL_THRESHOLD)
		hwa_pktpgr_pqplbufpool_fill(dev);

	if (pqplbufpool->avail == 0)
		return NULL;

	ASSERT(pqplbufpool->pkt_head != NULL);
	pqp_lbuf = pqplbufpool->pkt_head;
	pqplbufpool->pkt_head = (hwa_pp_lbuf_t *)PKTLINK(pqp_lbuf);

	if (pqplbufpool->pkt_head == NULL) {
		pqplbufpool->pkt_tail = NULL;
	}

	// No bzero, fixup. PQP will overwrite entire lbuf context + databuffer

	pqplbufpool->avail--;
	pqplbufpool->n_pkts--;

	return pqp_lbuf;

}   // hwa_pqp_pkt_get()

void // Deallocate a packet
hwa_pqp_pkt_put(struct hwa_dev *dev, void *pkt)
{
	hwa_pktpgr_free_tx(dev, (hwa_pp_lbuf_t*)pkt, (hwa_pp_lbuf_t*)pkt, 1);
}   // hwa_pqp_pkt_put()

// Wait for all PQP Lbuf Pool allocation request responsed.
void
hwa_pktpgr_pagemgr_alloc_pqp_pkt_wait_to_finish(struct hwa_dev *dev)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	if (pktpgr->pqplbuf_alloc_req <= 0)
		return;

	// Poll until tagged PULL are processed.
	hwa_pktpgr_req_wait_to_finish(dev, &pktpgr->pqplbuf_alloc_req,
		hwa_pktpgr_pagemgr_rsp_ring, "ALLOCPQP",
		HWA_PKTPGR_PAGEMGR_CALLBACK, HWA_PP_CMD_CONS_ALL);
}

void // Force using emergency resv DDBM to fill pqplbufpool
hwa_pktpgr_pqplbufpool_rsv_ddbm_set(struct hwa_dev *dev, bool rsv_ddbm)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	pktpgr->pqplbuf_use_rsv_ddbm = rsv_ddbm;
}

bool // Enqueue DDBM to pqplbufpool
hwa_pktpgr_pqplbufpool_enq(hwa_dev_t *dev, hwa_pp_lbuf_t *pkt)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbufpool_t *lbufpool;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	lbufpool = &pktpgr->pqplbufpool;

	// Don't enqueue the DDBM to pqplbufpool if pqplbuf_use_rsv_ddbm is not set
	// or resource is enough.
	if ((!pktpgr->pqplbuf_use_rsv_ddbm) ||
		((lbufpool->max_pkts - lbufpool->n_pkts) == 0)) {
		return FALSE;
	}

	// add to lbufpool
	if (lbufpool->pkt_head == NULL) {
		lbufpool->pkt_head = pkt;
		lbufpool->pkt_tail = pkt;
	} else {
		PKTSETLINK(lbufpool->pkt_tail, pkt);
		lbufpool->pkt_tail = pkt;
	}
	lbufpool->avail += 1;
	lbufpool->n_pkts += 1;

	return TRUE;

}
#endif /* HNDPQP && HWA_PKTPGR_BUILD */

void // PAGEMGR ALLOC TX RESP for txlbufpool and pqplbufpool
hwa_txpost_pagemgr_alloc_tx_rsp(hwa_dev_t *dev,
	uint8 pktpgr_trans_id, uint8 pktpgr_trans_tagged, int pkt_count,
	hwa_pp_lbuf_t * pktlist_head, hwa_pp_lbuf_t * pktlist_tail)
{
	uint16 pkt = 0;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbufpool_t *lbufpool;
	hwa_pp_lbuf_t *txlbuf;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	lbufpool = &pktpgr->txlbufpool;
#ifdef HNDPQP
	if (pktpgr_trans_tagged) {
		HWA_COUNT_DEC(pktpgr->pqplbuf_alloc_req, 1);
		lbufpool = &pktpgr->pqplbufpool;
	}
#endif /* HNDPQP */
	txlbuf = pktlist_head;

	HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  <<PAGEMGR::RSP PAGEMGR_ALLOC_TX : pkts %3u "
		"list[%p(%d) .. %p(%d)] id %u <==ALLOCTX-RSP(%d)==\n\n",
		pkt_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), pktpgr_trans_id, pktpgr_trans_id);

	if (pkt_count == 0) {
		HWA_ERROR(("%s: pkt_count is 0\n", __FUNCTION__));
		// Sync avail and n_pkts at last rsp.
		HWA_ASSERT(lbufpool->avail < lbufpool->n_pkts);
		if (lbufpool->trans_id == pktpgr_trans_id &&
			lbufpool->n_pkts != lbufpool->avail) {
			lbufpool->n_pkts = lbufpool->avail;
		}

		return;
	}

	HWA_ASSERT(pktlist_tail->context.control.link == NULL);
	for (pkt = 0; pkt < pkt_count - 1; ++pkt) {
		// Only control::pkt_mapid and control::link have values, others are garbage
		// NOTE: Clear some fields of txlbuf in hwa_txpost_txbuffer_get
		HWA_TRACE(("pkt_mapid<%d> link 0x%p\n",
			txlbuf->context.control.pkt_mapid,
			txlbuf->context.control.link));
		HWA_PKT_DUMP_EXPR(hwa_txpost_dump_pkt(txlbuf, NULL, "alloc_tx", pkt, TRUE));

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
		// Alloc: SW allocation case
		hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif

		txlbuf = (hwa_pp_lbuf_t*)txlbuf->context.control.link;
	}
	HWA_ASSERT(txlbuf == pktlist_tail);
	HWA_TRACE(("pkt_mapid<%d> link 0x%p\n",
		txlbuf->context.control.pkt_mapid,
		txlbuf->context.control.link));
	HWA_PKT_DUMP_EXPR(hwa_txpost_dump_pkt(txlbuf, NULL, "alloc_tx", pkt, TRUE));

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
	// Alloc: SW allocation case
	hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif

	// add to lbufpool
	if (lbufpool->pkt_head == NULL) {
		lbufpool->pkt_head = pktlist_head;
		lbufpool->pkt_tail = pktlist_tail;
	} else {
		PKTSETLINK(lbufpool->pkt_tail, pktlist_head);
		lbufpool->pkt_tail = pktlist_tail;
	}
	lbufpool->avail += pkt_count;

	// DDBM accounting.
	// Thought to handle pkt_count is zero in resp.
	// 1. Pre-reserved HWA_PP_LBUFPOOL_LEN_MAX * 2.
	// 2. Account it in resp rather than in req.
	HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, pkt_count);
	HWA_DDBM_COUNT_LWM(pktpgr->ddbm_avail_sw, pktpgr->ddbm_avail_sw_lwm);
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_tx, pkt_count);
	HWA_DDBM_COUNT_HWM(pktpgr->ddbm_sw_tx, pktpgr->ddbm_sw_tx_hwm);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", pkt_count,
		pktpgr->ddbm_avail_sw, __FUNCTION__);

	// Sync avail and n_pkts at last rsp.
	HWA_ASSERT(lbufpool->avail <= lbufpool->n_pkts);
	if (lbufpool->trans_id == pktpgr_trans_id &&
		lbufpool->n_pkts != lbufpool->avail) {
		lbufpool->n_pkts = lbufpool->avail;
	}

#ifdef HNDPQP
	// Direct call pqp_pgi_wake() inside hwa_txpost_pagemgr_alloc_tx_rsp()
	// may have a chance to cause infinite loop in hwa_pktpgr_req_wait_to_finish()
	// because rd_idx will not be updated when rsp_ring_in_process is not set in
	// hwa_pktpgr_response_for_trans_cmd().
	// So postpone the PQP resume after all responses are processed.
	if (pktpgr_trans_tagged) {
		pktpgr->pqp_pgi_wake = TRUE;
	}
#endif

	return;

} // hwa_txpost_pagemgr_alloc_tx_rsp

// Flag to enable HW capability of one MPDU has more than one TxLfrag.
// Mandatory for more thean 4-in-1 AMSDU.
uint8
hwa_pktpgr_multi_txlfrag(struct hwa_dev *dev)
{
	return dev->flexible_txlfrag;
}

void // Free HME LCLPKT
hwa_pktpgr_hmedata_free(struct hwa_dev *dev, void *pkt)
{
#if defined(BCMHME)
	haddr64_t haddr64;

	// Put HME LCLPK back
	HADDR64_HI_SET(haddr64, PKTHME_HI(dev->osh, pkt));
	HADDR64_LO_SET(haddr64, PKTHME_LO(dev->osh, pkt));
	hme_put(HME_USER_LCLPKT, PKTHMELEN(dev->osh, pkt), haddr64);
#endif /* BCMHME */
}

// Wait for specific request have responsed.
static void
hwa_pktpgr_req_wait_to_finish(hwa_dev_t *dev, int16 *req_cnt_p,
	hwa_pktpgr_ring_t pp_ring, const char *who, hwa_callback_t callback, uint8 cmdtype)
{
	hwa_pktpgr_t *pktpgr;
	int16 req_cnt, req_cnt_orig;
	uint32 loop = HWA_FSM_IDLE_POLLLOOP;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev);

	pktpgr = &dev->pktpgr;
	BCM_REFERENCE(pktpgr);

	// Polling until all request are processed.
	req_cnt_orig = req_cnt = *req_cnt_p;
	BCM_REFERENCE(req_cnt_orig);

	do {
		// NOTE: here we may process mixed tran type
		// Only process specific cmdtype
		hwa_pktpgr_response_for_trans_cmd(dev, pp_ring, callback, cmdtype);

		// Wait a moment if no progress.
		if (req_cnt == *req_cnt_p) {
			OSL_DELAY(1);
			loop--;

			// There is one corner case that polling will fail
			// when the pagein_rsp_ring is full.
			// Assume WR index is 0x160 and RD index is 0x161.
			// The pagein response in RD 0x161 is txpost response.
			// If driver only process txstatus in pagein_rsp_ring,
			// the txpost response will be skipped.
			// Then polling this ring may be fail due to
			// the RD index will not be advanced.
			// So if there is no enough space for txs page in req.
			// Driver should use HWA_PP_CMD_CONS_ALL to process all page in resp.
			if ((cmdtype != HWA_PP_CMD_CONS_ALL) &&
				!hwa_pktpgr_rsp_ring_avail_for_reqcnt(dev, pp_ring, req_cnt)) {
				cmdtype = HWA_PP_CMD_CONS_ALL;
			}
		}
		req_cnt = *req_cnt_p;
	} while (*req_cnt_p > 0 && loop);

	if (loop == 0) {
		HWA_PRINT("  HWA[%d] %s polling \"%s\" ring, pending<%d/%d>, try<%d>. Timeout\n",
			dev->unit, who, pktpgr->rsp_ring[pp_ring]->name, req_cnt_orig, *req_cnt_p,
			HWA_FSM_IDLE_POLLLOOP);
#if defined(BCMDBG) || defined(HWA_DUMP)
		hwa_dump(dev, NULL, HWA_DUMP_PP, TRUE, TRUE, FALSE, NULL);
		HWA_ASSERT(0);
#endif

	} else {
		HWA_INFO(("  HWA[%d] %s polling \"%s\" ring, pending<%d/%d>, try<%d>. Done\n",
			dev->unit, who, pktpgr->rsp_ring[pp_ring]->name, req_cnt_orig, *req_cnt_p,
			(HWA_FSM_IDLE_POLLLOOP - loop)));
	}
}

// Wait for all PageIn TxS have responsed.
void
hwa_pktpgr_txstatus_wait_to_finish(hwa_dev_t *dev, int fifo_idx)
{
	hwa_pktpgr_t *pktpgr;
	uint8 cmdtype;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// XXX May need optimization
	// The PageIn Req service three type of req [TxS, TxPost and Rx],
	// how can the TxS pagein be serviced at high priority in "wl reinit" condition?
	// For example in hwa_txstat_reclaim(reinit == TRUE), we need to process
	// all PageIn TxS Rsp so we use hwa_pktpgr_txstatus_wait_to_finish to polling.
	// But if PageIn Req Q has a lot of TxPost/Rx reqs before TxS then we may try
	// to swap TxPost, Rx PageIn Req to the end [hint: use invalid bit].
	// If the req in Q is few then we don't need to swap it.

	// Setup locals
	pktpgr = &dev->pktpgr;
	cmdtype = HWA_PP_PAGEIN_TXSTATUS;

	if (pktpgr->pgi_txs_req_tot == 0)
		return;

	if ((fifo_idx >= 0) && (pktpgr->pgi_txs_req[fifo_idx] == 0))
		return;

	// Poll until all PageIn TxS are processed.
	hwa_pktpgr_req_wait_to_finish(dev, (fifo_idx >= 0) ?
		&pktpgr->pgi_txs_req[fifo_idx] : &pktpgr->pgi_txs_req_tot,
		hwa_pktpgr_pagein_rsp_ring, "TxStatus", HWA_PKTPGR_PAGEIN_CALLBACK, cmdtype);
}

// Reclaim more DBM hold by SW in TxCpl path.
void
hwa_pktpgr_txcpl_sync(hwa_dev_t *dev)
{
	// Always try to reclaim more DDBM hold by SW.
	// 1. Refresh TxCple to get room to avoid DDBM be queued in pciedev d2h_req_q.
	// 2. Drain DDBM in pciedev d2h_req_q.
	hwa_txcple_refresh(dev);
	hwa_txcple_pciedev_d2h_req_q_send(dev);
	hwa_txcple_refresh(dev);
}

// Wait for tagged PUSH have responsed.
static void
hwa_pktpgr_tag_push_wait_to_finish(hwa_dev_t *dev)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	if (pktpgr->tag_push_req <= 0)
		return;

	// Poll until tagged PUSH are processed.
	hwa_pktpgr_req_wait_to_finish(dev, &pktpgr->tag_push_req,
		hwa_pktpgr_pagemgr_rsp_ring, "Push(Tag)", HWA_PKTPGR_PAGEMGR_CALLBACK,
		HWA_PP_CMD_CONS_ALL);
}

// Wait for all PULL have responsed.
static void
hwa_pktpgr_tag_pull_wait_to_finish(hwa_dev_t *dev)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	if (pktpgr->tag_pull_req <= 0)
		return;

	// Poll until tagged PULL are processed.
	hwa_pktpgr_req_wait_to_finish(dev, &pktpgr->tag_pull_req,
		hwa_pktpgr_pagemgr_rsp_ring, "Pull(Tag)", HWA_PKTPGR_PAGEMGR_CALLBACK,
		HWA_PP_CMD_CONS_ALL);

}

// Wait for all PageOut pktlist response.
void
hwa_pktpgr_pageout_pktlist_rsp_wait_to_finish(hwa_dev_t *dev)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	if (pktpgr->pgo_pktlist_req <= 0)
		return;

	// Poll until PageOut local are processed.
	hwa_pktpgr_req_wait_to_finish(dev, &pktpgr->pgo_pktlist_req,
		hwa_pktpgr_pageout_rsp_ring, "Tx(Pktlist)", HWA_PKTPGR_PAGEOUT_CALLBACK,
		HWA_PP_CMD_CONS_ALL);
}

// Wait for all PageOut local response.
void
hwa_pktpgr_pageout_local_rsp_wait_to_finish(hwa_dev_t *dev)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	if (pktpgr->pgo_local_req <= 0)
		return;

	// Poll until PageOut local are processed.
	hwa_pktpgr_req_wait_to_finish(dev, &pktpgr->pgo_local_req,
		hwa_pktpgr_pageout_rsp_ring, "Tx(Local)", HWA_PKTPGR_PAGEOUT_CALLBACK,
		HWA_PP_CMD_CONS_ALL);
}

// Wait for all PageIn RxProcess have responsed.
void
hwa_pktpgr_rxprocess_wait_to_finish(hwa_dev_t *dev)
{
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	if (pktpgr->pgi_rxpkt_req <= 0)
		return;

	// Poll until PageIn RxProcess are processed.
	hwa_pktpgr_req_wait_to_finish(dev, &pktpgr->pgi_rxpkt_req,
		hwa_pktpgr_pagein_rsp_ring, "RxProcess", HWA_PKTPGR_PAGEIN_CALLBACK,
		HWA_PP_PAGEIN_RXPROCESS);
}

static void
hwa_pktpgr_txfifo_link_tail_process(hwa_dev_t *dev, uint16 pkt_mapid)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbuf_t *txlbuf;
	struct spktq *tag_pull_q;
	uint32 loaddr;
	haddr64_t haddr64;
	const void *src_addr;
	uint16 offset;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	tag_pull_q = &pktpgr->tag_pull_q;

	// Check.
	HWA_ASSERT(spktq_n_pkts(tag_pull_q) == 1);
	if (spktq_empty(tag_pull_q)) {
		HWA_INFO(("%s: Why, tag_pull_q is empty\n", __FUNCTION__));
		return;
	}

	// DDBM accounting.
	HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, 1);
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_tx, 1);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", 1,
		pktpgr->ddbm_avail_sw, __FUNCTION__);

	txlbuf = spktq_deq(tag_pull_q);
	ASSERT(txlbuf);
#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
	// Alloc: PULL case.
	hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif

	haddr64 = pktpgr->hostpktpool_haddr64;
	loaddr = HADDR64_LO(haddr64);
	loaddr += pkt_mapid * sizeof(hwa_pp_lbuf_t);

	PKTSETLINK(txlbuf, loaddr);

	// offset of txlbuf link pointer
	offset = OFFSETOF(hwa_pp_lbuf_control_t, link);
	ASSERT(offset == 8);
	//offset = 8;

	// For 6715A0, I cannot use PUSH to update the change because
	// HW PUSH will clear the link pointer of tail packet in the
	// PUSH req. It breaks the HW shadow list. CRBCAHWA-640

	// 6175A0 SW WAR:
	// Use BME to sync link pointer of tail pkt
	// HDBM base
	haddr64 = pktpgr->hostpktpool_haddr64;
	loaddr = HADDR64_LO(haddr64);
	loaddr += PKTMAPID(txlbuf) * sizeof(hwa_pp_lbuf_t);
	loaddr += offset;
	HADDR64_LO_SET(haddr64, loaddr);
	src_addr = (const void *)((uintptr)txlbuf + offset);
	if (hme_d2h_xfer(src_addr, &haddr64, 4,
		TRUE, HME_USER_NOOP) < 0) {
		HWA_ERROR(("ERROR: hme_d2h_xfer failed\n"));
		HWA_ASSERT(0);
	}
	// Free to DDBM by DDBM buffer index.
	// Note: pkt_mapid is not equal to DDBM buffer index.
	// Both HWA_PP_PAGEMGR_FREE_DDBM and
	// HWA_PP_PAGEMGR_FREE_DDBM_LINK are verified.
#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
	// Free: PULL process done case. HDBM part
	hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_HDBM,
		DBM_AUDIT_FREE);
#endif
	hwa_pktpgr_dbm_free(dev, HWA_DDBM, HWA_TABLE_INDX(hwa_pp_lbuf_t,
		pktpgr->dnglpktpool_mem, txlbuf), TRUE);

	HWA_ASSERT(spktq_empty(tag_pull_q));
}

// For 6715Ax, there is no PUSH_MPDU cmd, driver need to link the pktlist manually.
// Link two pktlist in the HDBM
static void
hwa_pktpgr_txfifo_link_tail(hwa_dev_t *dev, uint32 fifo_idx, uint16 tail_pkt_mapid,
	uint16 link_pkt_mapid)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pagemgr_req_pull_t req;
	int trans_id, req_loop;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	// Setup locals
	pktpgr = &dev->pktpgr;

	// Request tagged PULL
	// B0: Since I use spktq to store PULLed packets, so
	// I cannot use HWA_PP_PAGEMGR_PULL_KPFL_LINK
	req.trans        = HWA_PP_PAGEMGR_PULL;
	req.pkt_count    = 1;
	req.pkt_mapid    = tail_pkt_mapid;
	req.tagged       = HWA_PP_CMD_TAGGED;

	req_loop = HWA_LOOPCNT;

req_again:
	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req);
	if (trans_id == HWA_FAILURE) {
		if (--req_loop > 0) {
			OSL_DELAY(10);
			goto req_again;
		}
		HWA_ERROR(("Tagged PULL_REQ failure: pkt_mapid %3u", tail_pkt_mapid));
		HWA_ASSERT(0);
		return;
	}

	HWA_TRACE(("PULL HDBM pkt_mapid %3u\n", tail_pkt_mapid));

	HWA_COUNT_INC(pktpgr->tag_pull_req, 1);

	// Wait for tag PULL resp.
	hwa_pktpgr_tag_pull_wait_to_finish(dev);

	hwa_pktpgr_txfifo_link_tail_process(dev, link_pkt_mapid);
}

// When using HWA_PP_PAGEMGR_PULL cmd, driver need pull next pointer manually
// PULL next pointer of packet from HDBM to DDBM
static void
hwa_pktpgr_txfifo_shadow_pull_next_process(hwa_dev_t *dev, void *pkt)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbuf_t *txlbuf;
	struct spktq *tag_pull_q;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	tag_pull_q = &pktpgr->tag_pull_q;

	// Check.
	HWA_ASSERT(spktq_n_pkts(tag_pull_q) == 1);
	if (spktq_empty(tag_pull_q)) {
		HWA_INFO(("%s: Why, tag_pull_q is empty\n", __FUNCTION__));
		return;
	}

	// DDBM accounting.
	HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, 1);
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_tx, 1);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", 1,
		pktpgr->ddbm_avail_sw, __FUNCTION__);

	txlbuf = spktq_deq(tag_pull_q);
	ASSERT(txlbuf);
#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
	// Alloc: PULL case.
	hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_2DBM,
		DBM_AUDIT_ALLOC);
#endif

	PKTSETNEXT(dev->osh, pkt, txlbuf);
}

// Return count of processed mpdu
static uint16
hwa_pktpgr_txfifo_shadow_pull_process(hwa_dev_t *dev, uint16 mpdu_cnt, uint16 *pkt_mapid,
	bool last, struct spktq *pkt_list, bool push_to_host)
{
	hwa_pktpgr_t *pktpgr;
	struct spktq *tag_pull_q;
	struct spktq sync_pull_q;
	struct spktq sync_free_q;
	uint16 processed;
	uint16 next_pkt_mapid;
	uint16 pkt_count;
	hwa_pp_lbuf_t *txlbuf;
	void *pkt;
	uint8 tagged;
	bool drop_pkt;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	tag_pull_q = &pktpgr->tag_pull_q;
	pkt_count = mpdu_cnt;
	if (push_to_host) {
		tagged = HWA_PP_CMD_TAGGED;
	} else {
		tagged = HWA_PP_CMD_NOT_TAGGED;
	}
	drop_pkt = FALSE;

	// Init the temporary q
	spktq_init(&sync_pull_q, PKTQ_LEN_MAX);
	spktq_init(&sync_free_q, PKTQ_LEN_MAX);

	if (HWAREV_GE(dev->corerev, 133)) {
		BCM_REFERENCE(next_pkt_mapid);
	}

	// Check.
	HWA_ASSERT(spktq_n_pkts(tag_pull_q) == mpdu_cnt);
	if (spktq_empty(tag_pull_q)) {
		HWA_INFO(("%s: Why, tag_pull_q is empty\n", __FUNCTION__));
		return 0;
	}

	// Release tail pkt if last is FALSE, because I need to use tail
	// pkt_mapid as next PULL key.
	if (!last) {
		pkt = spktq_deq_tail(tag_pull_q);
		*pkt_mapid = PKTMAPID(pkt);

		// For 6715B0, driver use HWA_PP_PAGEMGR_PULL_MPDU to pull pkts from host.
		// HWA will pull next pointer of each pkt. So need to free it.
		if (HWAREV_GE(dev->corerev, 133)) {
			void *tail;

			while ((tail = pktchain_deq_tail(dev->osh, pkt)) != pkt) {
				spktq_enq(&sync_free_q, tail);
				pkt_count++;
			}

			spktq_enq_head(&sync_free_q, pkt);
			hwa_pktpgr_free_ddbm_pkt(dev, spktq_peek_head(&sync_free_q),
				spktq_peek_tail(&sync_free_q), spktq_n_pkts(&sync_free_q));
		} else {
#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
			// Free: PULL process done case. HDBM part
			hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_HDBM,
				DBM_AUDIT_FREE);
#endif
			hwa_pktpgr_dbm_free(dev, HWA_DDBM, HWA_TABLE_INDX(hwa_pp_lbuf_t,
				pktpgr->dnglpktpool_mem, pkt), TRUE);
		}
	}

	spktq_enq_list(&sync_pull_q, tag_pull_q);

	while ((pkt = spktq_deq(&sync_pull_q))) {
		txlbuf = (hwa_pp_lbuf_t *)pkt;

		if (PKTISMGMTTXPKT(dev->osh, txlbuf)) {
			txlbuf = hwa_txstat_pagein_rsp_fixup_local(dev, txlbuf, 0, 1, tagged);
		} else {
			hwa_txstat_pagein_rsp_fixup(dev, txlbuf, 1);

			// XXX: For 6836 + 6715, observe HWA didn't do address translation
			// for HWA_PP_PAGEMGR_PULL_MPDU. Manually do it to bypass the issue.
			if (HWA_PTR2UINT(PKTDATA(dev->osh, txlbuf)) >=
				HADDR64_LO(pktpgr->hostpktpool_haddr64)) {
				uint32 data_addr, hdbm_addr, data_offset;

				data_addr = HWA_PTR2UINT(PKTDATA(dev->osh, txlbuf));
				hdbm_addr = HWA_TABLE_ADDR(hwa_pp_lbuf_t,
					HADDR64_LO(pktpgr->hostpktpool_haddr64), PKTMAPID(txlbuf));
				data_offset = data_addr - hdbm_addr;

				if (data_offset < HWA_PP_LBUF_SZ) {
					PKTSETDATA(dev->osh, txlbuf,
						HWA_PTR2UINT(txlbuf) + data_offset);
				} else {
					drop_pkt = TRUE;
				}
			}
		}

		while (PKTNEXT(dev->osh, pkt) != NULL) {
			// For 6715Ax, driver use HWA_PP_PAGEMGR_PULL to pull pkts from host.
			// HWA will not pull next pointer of each pkt.
			// So need to pull next pointer of each pkt
			if (HWAREV_LE(dev->corerev, 132)) {
				// Pull msdu in mpdu
				// HDBM buffer index is equal to pkt_mapid
				next_pkt_mapid = hwa_pktpgr_hdbm_ptr2idx(pktpgr,
					(uintptr)PKTNEXT(dev->osh, pkt));
				hwa_pktpgr_txfifo_shadow_pull_pkts(dev, next_pkt_mapid, 1);
				hwa_pktpgr_txfifo_shadow_pull_next_process(dev, pkt);
			} else {
				pkt_count++;
			}

			pkt = PKTNEXT(dev->osh, pkt);
		}

		if (drop_pkt) {
			// XXX: Assert here to catch if data pointer is corrupted.
			HWA_WARN(("%s: drop pkt<0x%p> due to corrupted data pointer\n",
				__FUNCTION__, txlbuf));
#if defined(BCMDBG) || defined(HWA_DUMP)
			prhex("Corrupted packet", (void *)txlbuf, HWA_PP_LBUF_SZ);
#endif
			ASSERT(0);
			PKTFREE(dev->osh, txlbuf, TRUE);
			drop_pkt = FALSE;
		} else {
			spktq_enq(pkt_list, txlbuf);
		}
	}

	spktq_deinit(&sync_pull_q);
	spktq_deinit(&sync_free_q);

	// DDBM accounting.
	HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, pkt_count);
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_tx, pkt_count);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", pkt_count,
		pktpgr->ddbm_avail_sw, __FUNCTION__);

	processed = last ? mpdu_cnt : (mpdu_cnt - 1);

	return processed;
}

// PULL packet from HDBM to DDBM
static void
hwa_pktpgr_txfifo_shadow_pull_pkts(hwa_dev_t *dev, uint16 pkt_mapid, uint16 pull_mpdu_cnt)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pagemgr_req_pull_t req;
	int trans_id, req_loop;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	// Request tagged PULL
	// B0: Since I use spktq to store PULLed packets, so
	// I cannot use HWA_PP_PAGEMGR_PULL_KPFL_LINK
	if (HWAREV_GE(dev->corerev, 133)) {
		req.trans = HWA_PP_PAGEMGR_PULL_MPDU;
	} else {
		req.trans = HWA_PP_PAGEMGR_PULL;
	}
	req.pkt_count	 = pull_mpdu_cnt;
	req.pkt_mapid	 = pkt_mapid;
	req.tagged	 = HWA_PP_CMD_TAGGED;

	req_loop = HWA_LOOPCNT;

req_again:
	trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req);
	if (trans_id == HWA_FAILURE) {
		if (--req_loop > 0) {
			OSL_DELAY(10);
			goto req_again;
		}
		HWA_ERROR(("Tagged PULL_REQ failure: pkt_mapid %3u", pkt_mapid));
		HWA_ASSERT(0);
		return;
	}

	HWA_TRACE(("PULL sync pkts<%d> pkt_mapid %3u\n", pull_mpdu_cnt, pkt_mapid));

	HWA_COUNT_INC(pktpgr->tag_pull_req, 1);

	// Wait for tag PULL resp.
	hwa_pktpgr_tag_pull_wait_to_finish(dev);
}

// Push pkts to the HDBM due to low DDBM resource
void
hwa_pktpgr_txfifo_shadow_push_to_host(struct hwa_dev *dev, int fifo_idx,
	struct spktq *pkt_list, hwa_pp_hdbm_pktq_t *hpktq)
{
	hwa_pktpgr_t *pktpgr;
	uint16 mpdu_count, pkt_count;
	void *pkt, *pkt_next, *push_pkt, *pktlist_head, *pktlist_tail;
	struct spktq push_q;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	// Setup locals
	pktpgr = &dev->pktpgr;

	spktq_init(&push_q, PKTQ_LEN_MAX);

	if (HWAREV_GE(dev->corerev, 133)) {
		BCM_REFERENCE(pktpgr);
	} else {
		BCM_REFERENCE(pkt_count);
	}

	mpdu_count = spktq_n_pkts(pkt_list);
	pkt_count = mpdu_count;

	while ((pkt = spktq_deq(pkt_list))) {
		if (PKTISMGMTTXPKT(dev->osh, pkt)) {
			if (PKTMGMTDDBMPKT(dev->osh, pkt)) {
				push_pkt = (void *)PKTMGMTDDBMPKT(dev->osh, pkt);
				PKTRESETMGMTDDBMPKT(dev->osh, pkt);
				HWA_INFO(("%s: push mgmt frame, pktmapid=%d\n",
					__FUNCTION__, PKTMAPID(push_pkt)));
			} else {
				HWA_WARN(("%s: push mgmt frame is NULL !!!\n",
					__FUNCTION__));
				ASSERT(0);
				mpdu_count--;
				pkt_count--;
				PKTFREE(dev->osh, pkt, TRUE);
				continue;
			}
		} else {
			push_pkt = pkt;

			pkt_next = PKTNEXT(dev->osh, pkt);
			for (; pkt_next; pkt_next = PKTNEXT(dev->osh, pkt_next)) {
				pkt_count++;

				if (HWAREV_LE(dev->corerev, 132)) {
					// When using PUSH cmd to push pkts back to HDBM,
					// HWA will adjust link pointer and
					// next pointer will keep untouched.
					// Update the first msdu->next to the correct host addr.
					if (pkt_next == PKTNEXT(dev->osh, pkt)) {
						uint32 loaddr;
						loaddr = HADDR64_LO(pktpgr->hostpktpool_haddr64);
						loaddr += PKTMAPID(pkt_next) *
							sizeof(hwa_pp_lbuf_t);
						PKTSETNEXT(dev->osh, pkt, loaddr);
					}

#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
					// Free: PULL process done case. HDBM part.
					hwa_pktpgr_dbm_audit(dev, pkt_next, DBM_AUDIT_TX,
						DBM_AUDIT_HDBM, DBM_AUDIT_FREE);
#endif
					hwa_pktpgr_dbm_free(dev, HWA_DDBM,
						HWA_TABLE_INDX(hwa_pp_lbuf_t,
						pktpgr->dnglpktpool_mem, pkt_next), TRUE);
				}
			}
		}

		spktq_enq(&push_q, push_pkt);
	}

	// Push mpdu
	ASSERT(spktq_n_pkts(&push_q) == mpdu_count);
	if (spktq_n_pkts(&push_q)) {
		pktlist_head = spktq_peek_head(&push_q);
		pktlist_tail = spktq_peek_tail(&push_q);

		if (HWAREV_GE(dev->corerev, 133)) {
			// Use PUSH_MPDU
			hwa_pktpgr_push_mpdu_req(dev, fifo_idx, HWA_PP_CMD_TAGGED,
				pktlist_head, pktlist_tail, pkt_count, mpdu_count,
				(hpktq->mpdu_count > 0) ?
				hpktq->pkt_tail : HWA_PP_PKT_MAPID_INVALID);
		} else {
			hwa_pktpgr_push_req(dev, fifo_idx, HWA_PP_CMD_TAGGED,
				pktlist_head, pktlist_tail, mpdu_count, mpdu_count);
		}

		// Wait for PUSH complete before return.
		hwa_pktpgr_tag_push_wait_to_finish(dev);

		// Save packets in the host before really page-out process
		if (hpktq->mpdu_count == 0) {
			HWA_INFO(("  %s: Push-head %d packet [%d .. %d] for fifo %d to host\n",
				__FUNCTION__, mpdu_count, PKTMAPID(pktlist_head),
				PKTMAPID(pktlist_tail), fifo_idx));

			hpktq->pkt_head = PKTMAPID(pktlist_head);
			hpktq->pkt_tail = PKTMAPID(pktlist_tail);
			hpktq->mpdu_count = mpdu_count;

			// Store partial_relcaim_cnt in hpktq->reclaim_cnt for next step process.
			// reclaim_cnt must be at least 2 to prevent infinite loop in pull process.
			hpktq->reclaim_cnt = pktpgr->partial_relcaim_cnt;
			if (hpktq->reclaim_cnt <= 1) {
				hpktq->reclaim_cnt = 2;
			}
		} else {
			HWA_INFO(("  %s: Push-tail %d packet [%d .. %d] for fifo %d to host "
				"[%d .. %d] mpdu_cnt %d\n", __FUNCTION__, mpdu_count,
				PKTMAPID(pktlist_head), PKTMAPID(pktlist_tail), fifo_idx,
				hpktq->pkt_head, hpktq->pkt_tail,
				hpktq->mpdu_count));

			if (HWAREV_LE(dev->corerev, 132)) {
				hwa_pktpgr_txfifo_link_tail(dev, fifo_idx, hpktq->pkt_tail,
					PKTMAPID(pktlist_head));
			}

			hpktq->pkt_tail = PKTMAPID(pktlist_tail);
			hpktq->mpdu_count += mpdu_count;
		}
	}

	// Deinit the temporary q
	spktq_deinit(&push_q);
}

#define RECLAIM_MPDU_CNT_MAX 64

// Pull pkts from HDBM for later page out process
bool
hwa_pktpgr_txfifo_shadow_pull_from_host(struct hwa_dev *dev, int fifo_idx,
	struct spktq *pkt_list, hwa_pp_hdbm_pktq_t *hpktq, bool push_to_host)
{
	uint16 processed, pull_mpdu_cnt;
	bool empty;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	// Setup locals
	empty = TRUE;

	if (hpktq->mpdu_count) {
		// Wait previous pageout pktlist request finish
		hwa_pktpgr_pageout_pktlist_rsp_wait_to_finish(dev);

		HWA_INFO(("%d MPDU in sync shadow fifo_idx <%d> [%d .. %d]\n",
			hpktq->mpdu_count, fifo_idx,
			hpktq->pkt_head, hpktq->pkt_tail));

		pull_mpdu_cnt = MIN(hpktq->mpdu_count, hpktq->reclaim_cnt);
		hwa_pktpgr_txfifo_shadow_pull_pkts(dev, hpktq->pkt_head, pull_mpdu_cnt);
		processed = hwa_pktpgr_txfifo_shadow_pull_process(dev, pull_mpdu_cnt,
			&hpktq->pkt_head, (pull_mpdu_cnt == hpktq->mpdu_count),
			pkt_list, push_to_host);

		HWA_INFO(("Processed <%d> sync shadow packets on fifo_idx <%d>\n",
			processed, fifo_idx));

		hpktq->mpdu_count -= processed;

		if (hpktq->mpdu_count == 0) {
			hpktq->pkt_head = HWA_PP_PKT_MAPID_INVALID;
			hpktq->pkt_tail = HWA_PP_PKT_MAPID_INVALID;
			hpktq->reclaim_cnt = 0;
		} else {
			empty = FALSE;
		}
	}

	return empty;
}

// Pagein packets from HW shadow
// XXX, FIXME
//  - Caller hwa_txfifo_dma_reclaim free all reclaimed txlbuf.
//  - Caller wlc_tx_fifo_sync_complete may queue back to low q. (OMG)
bool
hwa_pktpgr_txfifo_shadow_reclaim(struct hwa_dev *dev, uint32 fifo_idx, bool fifo_sync)
{
	int mpdu_count, pkt_count, pktpgr_trans_id, req_loop;
	uint16 ppddavailcnt;
	hwa_txfifo_ovflwqctx_t ovflwq;
	hwa_pp_pagein_req_txstatus_t req;
	hwa_pktpgr_t *pktpgr;
	bool do_partial_reclaim;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	do_partial_reclaim = FALSE;

	// Make sure all previous pagein TxS req are responsed before reclaim it.
	hwa_pktpgr_txstatus_wait_to_finish(dev, fifo_idx);

	// Reset pgi_itxs to avoid reuse it
	clrbit(pktpgr->pgi_itxs, fifo_idx);

	// Get packets count from HWA OvfQ.
	mpdu_count = hwa_txfifo_get_ovfq(dev, fifo_idx, &ovflwq);
	if (mpdu_count <= 0) {
		pktpgr->partial_relcaim_cnt = 0;
		return FALSE;
	}

	pkt_count = ovflwq.pkt_count;

	if (pktpgr->partial_relcaim_cnt) {
		do_partial_reclaim = TRUE;
		goto partial_reclaim;
	}

	// Get DDBM available count
	ppddavailcnt = hwa_pktpgr_dbm_availcnt(dev, HWA_DDBM);
	HWA_INFO(("HW shadow%d has %d/%d packets, ddbmavil<H:%d, S:%d>\n", fifo_idx,
		mpdu_count, pkt_count, ppddavailcnt, pktpgr->ddbm_avail_sw));

	// Use PageIn-Process-PageIn piece by piece for low dnglpktspool case.
	// 1. Always do it for tx fifo sync.
	// 2. There is no enough DDBM resource to page in all packets in HWA OvfQ.
	if (fifo_sync || (pkt_count > ppddavailcnt) || (pktpgr->ddbm_avail_sw <= 0) ||
		(pktpgr->ddbm_avail_sw < ((mpdu_count + RECLAIM_MPDU_CNT_MAX) * 2))) {
		pktpgr->partial_relcaim_cnt = MIN(ppddavailcnt / 2, RECLAIM_MPDU_CNT_MAX);
		do_partial_reclaim = TRUE;
		HWA_INFO(("%s low DDBM <H:%d, S:%d>, Use partail shadow reclaim <%d>\n",
			HWApp, ppddavailcnt, pktpgr->ddbm_avail_sw,
			pktpgr->partial_relcaim_cnt));
	}

	// Don't care SW DDBM count for shadow reclaim, we can use HWA_PKTPGR_DDBM_SW_RESV.

partial_reclaim:
	if (do_partial_reclaim) {
		mpdu_count = MIN(mpdu_count, pktpgr->partial_relcaim_cnt);
	}

	// Issue pagein request
	req.trans        = HWA_PP_PAGEIN_TXSTATUS;
	req.fifo_idx     = (uint8)fifo_idx;
	req.zero         = 0;
	req.mpdu_count   = mpdu_count;
	req.tagged       = HWA_PP_CMD_TAGGED; // Just Pagein packets in HW shadow

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133)) {
		req.swar = 0xBF;
	}
#endif

	req_loop = HWA_LOOPCNT;

req_again:
	pktpgr_trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagein_req_ring, &req);
	if (pktpgr_trans_id == HWA_FAILURE) {
		if (--req_loop > 0) {
			OSL_DELAY(10);
			goto req_again;
		}
		HWA_ERROR(("%s PAGEIN::REQ TXSTATUS [shadow] failure mpdu<%u> fifo<%u>,"
			" no resource\n", HWA4a, req.mpdu_count, req.fifo_idx));
		HWA_ASSERT(0);
		return FALSE;
	}

	// No SW DDBM accounting for shadow reclaim, we will do it in resp.

	HWA_COUNT_INC(pktpgr->pgi_txs_req[req.fifo_idx], 1);
	HWA_COUNT_INC(pktpgr->pgi_txs_req_tot, 1);

	HWA_PP_DBG(HWA_PP_DBG_4A, "  >>PAGEIN::REQ TXSTATUS(tagged): mpdu %3u fifo %3u "
		"pgi_txs_req %d/%d ==TXS-REQ(%d)==>\n\n", req.mpdu_count, req.fifo_idx,
		pktpgr->pgi_txs_req[req.fifo_idx], pktpgr->pgi_txs_req_tot,
		pktpgr_trans_id);

	// Wait for all packets in HW shadow pagein to SW shadow.
	hwa_pktpgr_txstatus_wait_to_finish(dev, req.fifo_idx);

	return do_partial_reclaim;
}

static void
hwa_pktpgr_req_ring_stop(hwa_dev_t *dev, const char *who, hwa_pktpgr_ring_t pp_ring)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_ring_t *req_ring_reg;
	uint32 u32, state, loop_count;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	req_ring_reg = pktpgr->req_ring_reg[pp_ring];

	// Stop req ring
	u32 = HWA_RD_REG_ADDR(HWApp, &req_ring_reg->cfg);
	u32 |= BCM_SBIT(HWA_PAGER_PP_REQ_RING_CFG_STOP);
	HWA_WR_REG_ADDR(HWApp, &req_ring_reg->cfg, u32);
	loop_count = 0;
	do {
		if (loop_count)
			OSL_DELAY(1);
		u32 = HWA_RD_REG_ADDR(HWApp, &req_ring_reg->debug);
		state = BCM_GBF(u32, HWA_PAGER_PP_REQ_RING_DEBUG_FSM);
		HWA_TRACE(("%s: Polling %s DEBUG::STATE <%d>\n", __FUNCTION__, who, state));
	} while (state != 0 && ++loop_count != HWA_FSM_IDLE_POLLLOOP);
	if (loop_count == HWA_FSM_IDLE_POLLLOOP) {
		HWA_ERROR(("%s: %s is not idle <%d>\n", __FUNCTION__, who, state));
#if defined(BCMDBG) || defined(HWA_DUMP)
		hwa_dump(dev, NULL, HWA_DUMP_PP, TRUE, TRUE, FALSE, NULL);
		HWA_ASSERT(0);
#endif
	}
	HWA_INFO(("%s: Stop \"%s\" ring at loop_count %d\n", __FUNCTION__, who, loop_count));
}

static void
hwa_pktpgr_req_ring_start(hwa_dev_t *dev, const char *who, hwa_pktpgr_ring_t pp_ring)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_ring_t *req_ring_reg;
	uint32 u32;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	req_ring_reg = pktpgr->req_ring_reg[pp_ring];

	// Clear req ring stop bit
	u32 = HWA_RD_REG_ADDR(HWApp, &req_ring_reg->cfg);
	u32 = BCM_CBIT(u32, HWA_PAGER_PP_REQ_RING_CFG_STOP);
	HWA_WR_REG_ADDR(HWApp, &req_ring_reg->cfg, u32);
	HWA_INFO(("%s: Start \"%s\" ring\n", __FUNCTION__, who));
}

static uint32
hwa_pktpgr_req_ring_is_stop(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring)
{
	hwa_pktpgr_t *pktpgr;
	hwa_pp_ring_t *req_ring_reg;
	uint32 u32;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;
	req_ring_reg = pktpgr->req_ring_reg[pp_ring];

	// Check start req ring
	u32 = HWA_RD_REG_ADDR(HWApp, &req_ring_reg->cfg);
	return BCM_GBIT(u32, HWA_PAGER_PP_REQ_RING_CFG_STOP);
}

// Remove specific packets in PageOut Req Q to SW shadow.
void
hwa_pktpgr_pageout_ring_shadow_reclaim(struct hwa_dev *dev, uint32 fifo_idx)
{
	int wr_idx, rd_idx;
	uint16 depth, mpdu, mpdu_count, pkt_count;
	uint16 txd_count;
	hwa_ring_t *req_ring;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_cmd_t *req_cmd;
	hwa_pp_pageout_req_pktlist_t *pktlist;
	hwa_pp_pageout_req_pktlocal_t *pktlocal;
	hwa_pp_lbuf_t *txlbuf, *pktlist_head, *pktlist_tail;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);

	// Make sure PageOutReq ring is stopped.
	if (!hwa_pktpgr_req_ring_is_stop(dev, hwa_pktpgr_pageout_req_ring)) {
		HWA_ERROR(("PageOutReq ring is not stop!\n"));
		HWA_ASSERT(0);
	}

	// Setup locals
	pktpgr = &dev->pktpgr;
	req_ring = &pktpgr->pageout_req_ring;

	// Is empty? hwa_ring_is_cons_all will sync read.
	if (hwa_ring_is_cons_all(req_ring))
		return;

	wr_idx = HWA_RING_STATE(req_ring)->write;
	rd_idx = HWA_RING_STATE(req_ring)->read;
	depth = req_ring->depth;

	HWA_INFO(("Walk \"PO>\" ring to remove fifo%d packets to SW shadow\n", fifo_idx));

	while (rd_idx != wr_idx) {
		// Fetch pageout req cmd
		req_cmd = HWA_RING_ELEM(hwa_pp_cmd_t, req_ring, rd_idx);
		pktlist = &req_cmd->pageout_req_pktlist;
		pktlocal = &req_cmd->pageout_req_pktlocal;

		if (pktlist->fifo_idx == fifo_idx &&
			pktlist->invalid == HWA_PP_CMD_NOT_INVALID) {
			HWA_ASSERT((pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_NR) ||
				(pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_WR) ||
				(pktlist->trans == HWA_PP_PAGEOUT_PKTLOCAL));

			if (pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_NR ||
				pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_WR) {
				pktlist_head = (hwa_pp_lbuf_t *)pktlist->pktlist_head;
				pktlist_tail = (hwa_pp_lbuf_t *)pktlist->pktlist_tail;
				txlbuf = pktlist_head;
				mpdu_count = pktlist->mpdu_count;
				pkt_count = pktlist->pkt_count;
				txd_count = 0;

				// XXX, do we need to do fixup? review it.
				for (mpdu = 0; mpdu < mpdu_count; mpdu++) {
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
					void *next;
					// Alloc: Tx reclaim case.
					hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX,
						DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
					next = PKTNEXT(dev->osh, txlbuf);
					for (; next; next = PKTNEXT(dev->osh, next)) {
						hwa_pktpgr_dbm_audit(dev, next, DBM_AUDIT_TX,
							DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
					}
#endif
					txd_count += HWAPKTNDESC(txlbuf);
					hwa_txstat_pagein_rsp_fixup(dev, txlbuf, mpdu_count);
					txlbuf = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
				}

				// Accounting
				if (pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_WR) {
					HWA_COUNT_DEC(pktpgr->pgo_pktlist_req, 1);
				}

				HWA_INFO(("  Remove %d packets to SW shadow%d\n",
					mpdu_count, fifo_idx));
			} else {
				txlbuf = (hwa_pp_lbuf_t *)pktlocal->pkt_local;
				pktlist_head = txlbuf;
				pktlist_tail = txlbuf;
				mpdu_count = 1;
				pkt_count = 1;
				txd_count = HWAPKTNDESC(txlbuf);
				HWA_ASSERT(txd_count == 1);
				hwa_txstat_pagein_rsp_fixup_local(dev, txlbuf, fifo_idx,
					mpdu_count, HWA_PP_CMD_NOT_TAGGED);

				// Accounting
				HWA_COUNT_DEC(pktpgr->pgo_local_req, 1);

				HWA_INFO(("  Remove 1 local packet to SW shadow%d\n", fifo_idx));
			}

			// Link the pktlist to shadow list.
			hwa_txfifo_shadow_enq(dev, fifo_idx, mpdu_count,
				pkt_count, txd_count, pktlist_head, pktlist_tail);

			// Mark invalid Pageout Req.
			pktlist->invalid = HWA_PP_CMD_INVALID;
		}

		// Next.
		rd_idx = (rd_idx + 1) % depth;
	}
}

// Invalid request
void
hwa_pktpgr_req_ring_invalid_req(hwa_dev_t *dev, hwa_pktpgr_ring_t pp_ring,
	uint8 cmdtype, const char *cmdstr)
{
	uint8 trans;
	uint16 depth;
	int wr_idx, rd_idx;
	hwa_ring_t *req_ring;
	hwa_pp_cmd_t *req_cmd;
	hwa_pktpgr_t *pktpgr;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);
	HWA_ASSERT(pp_ring < hwa_pktpgr_req_ring_max);

	// Setup locals
	pktpgr = &dev->pktpgr;
	req_ring = pktpgr->req_ring[pp_ring];

	// Is empty? hwa_ring_is_cons_all will sync read.
	if (hwa_ring_is_cons_all(req_ring))
		return;

	// Stop req ring
	hwa_pktpgr_req_ring_stop(dev, req_ring->name, pp_ring);

	// sync read again.
	hwa_ring_prod_get(req_ring);

	wr_idx = HWA_RING_STATE(req_ring)->write;
	rd_idx = HWA_RING_STATE(req_ring)->read;
	depth = req_ring->depth;

	HWA_INFO(("Cancel \"%s\" req in \"%s\" ring.\n", cmdstr, req_ring->name));

	while (rd_idx != wr_idx) {
		// Fetch req cmd
		req_cmd = HWA_RING_ELEM(hwa_pp_cmd_t, req_ring, rd_idx);
		trans = req_cmd->u8[HWA_PP_CMD_TRANS] & HWA_PP_CMD_TRANS_MASK;
		if (trans == cmdtype) {
			req_cmd->u8[HWA_PP_CMD_TRANS] |= BCM_SBIT(HWA_PP_CMD_INVALID);
			HWA_INFO(("  Invalid + @ rd_idx<%d>\n", rd_idx));

			// Reduce one req
			if (cmdtype == HWA_PP_PAGEIN_RXPROCESS) {
				hwa_pp_pagein_req_rxprocess_t *req;

				req = (hwa_pp_pagein_req_rxprocess_t *)req_cmd;

				HWA_COUNT_DEC(pktpgr->pgi_rxpkt_req, 1);
				HWA_COUNT_DEC(pktpgr->pgi_rxpkt_in_transit, req->pkt_count);

				// DDBM accounting
				HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, req->pkt_count);
				HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_rx, req->pkt_count);
				HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: + %3u : %d @ %s\n",
					req->pkt_count, pktpgr->ddbm_avail_sw, __FUNCTION__);
			}
		}

		// Next.
		rd_idx = (rd_idx + 1) % depth;
	}

	hwa_pktpgr_req_ring_start(dev, req_ring->name, pp_ring);
}

// XXX, Need HW enhancement: Can HW do it for SW?
// It's really bad to handle hwa_txfifo_map_pkts by pull-map-bme/push.
// Think about this case, HW shadow has a lot of mpdu per FIFO and DDBM
// is low then the pull-map-push will take log time.
// 1. Stop PageOutReq, PageInReq rings in hwa_pktpgr_map_pkts_prep()
// 2. Shadow map, both SW and HW in hwa_txfifo_map_pkts() per FIFO.
// 3. PageInRsp, PageOutReq map in hwa_pktpgr_ring_map_pkts()
// 4. Start PageOutReq, PageInReq rings in hwa_pktpgr_map_pkts_done()
void
hwa_pktpgr_map_pkts_prep(struct hwa_dev *dev)
{
	// Stop PageOutReq and PageInReq rings because we are going
	// to pull-map-push HW shadow [OvfQ]

	// Stop PageOutReq
	hwa_pktpgr_req_ring_stop(dev, "PageOutReq", hwa_pktpgr_pageout_req_ring);

	// Stop PageInReq
	hwa_pktpgr_req_ring_stop(dev, "PageInReq", hwa_pktpgr_pagein_req_ring);
}

// Start PageOutReq and PageInReq rings
void
hwa_pktpgr_map_pkts_done(struct hwa_dev *dev)
{
	// Clear PageInReq ring stop bit
	hwa_pktpgr_req_ring_start(dev, "PageInReq", hwa_pktpgr_pagein_req_ring);

	// Clear PageOutReq ring stop bit
	hwa_pktpgr_req_ring_start(dev, "PageOutReq", hwa_pktpgr_pageout_req_ring);
}

// Return count of processed mpdu
static int
hwa_pktpgr_tag_pull_process(hwa_dev_t *dev, int mpdu_cnt, uint32 fifo_idx,
	void *cb, void *ctx, uint16 *pkt_mapid, bool last)
{
	uint32 map_result;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_lbuf_t *txlbuf;
	struct spktq *tag_pull_q;
	struct spktq dirty_q, clean_q;
	map_pkts_cb_fn map_pkts_cb;
	int processed;
	//int idx = 0;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	map_pkts_cb = (map_pkts_cb_fn)cb;
	pktpgr = &dev->pktpgr;
	tag_pull_q = &pktpgr->tag_pull_q;
	if (HWAREV_GE(dev->corerev, 133)) {
		// Init the temporary q
		spktq_init(&dirty_q, PKTQ_LEN_MAX);
		spktq_init(&clean_q, PKTQ_LEN_MAX);
	}

	// Check.
	HWA_ASSERT(spktq_n_pkts(tag_pull_q) == mpdu_cnt);
	if (spktq_empty(tag_pull_q)) {
		HWA_INFO(("Why, tag_pull_q is empty\n"));
		return 0;
	}

	HWA_INFO(("  Processing <%d> pulled packets, head<%p>\n",
		spktq_n_pkts(tag_pull_q), spktq_peek_head(tag_pull_q)));

	// Map packets in tag_pull_q
	// Bypass tail if last is FALSE, because I need to use tail
	// pkt_mapid as next PULL key.
	*pkt_mapid = PKTMAPID(spktq_peek_tail(tag_pull_q));
	txlbuf = spktq_deq(tag_pull_q);

	while (txlbuf) {
		//HWA_INFO(("  [mpdu-%d]: txlbuf<0x%p> pkt_mapid<%d>\n",
		//	idx++, txlbuf, PKTMAPID(txlbuf)));

		// Do map cb
		map_result = map_pkts_cb(ctx, (void*)txlbuf);

		if (map_result & MAP_PKTS_CB_DIRTY) {
			// Dirty, push back
			//HWA_INFO(("  Map result is dirty, push pkttag pkt_mapid<%d>\n",
			//	PKTMAPID(txlbuf)));

			if (HWAREV_LE(dev->corerev, 132)) {
				uint32 loaddr;
				haddr64_t haddr64;

				// For 6715A0, I cannot use PUSH to update the change because
				// HW PUSH will clear the link pointer of tail packet in the
				// PUSH req. It breaks the HW shadow list.
				// XXX, CRBCAHWA-640

				// 6175A0 SW WAR: assume map_pkts_cb only touch PKTTAG.
				// Use BME to sync PKTTAG for dirty packet.
				// HDBM base
				haddr64 = pktpgr->hostpktpool_haddr64;
				loaddr = HADDR64_LO(haddr64);
				// Plus offset of txlbuf PKTTAG
				loaddr += PKTMAPID(txlbuf) * sizeof(hwa_pp_lbuf_t);
				loaddr += HWA_PP_PKTTAG_BYTES;
				HADDR64_LO_SET(haddr64, loaddr);
				if (hme_d2h_xfer(PKTTAG(txlbuf), &haddr64, HWA_PP_PKTTAG_BYTES,
					TRUE, HME_USER_NOOP) < 0) {
					HWA_ERROR(("ERROR: hme_d2h_xfer failed\n"));
					HWA_ASSERT(0);
				}
				// Free to DDBM by DDBM buffer index.
				// Note: pkt_mapid is not equal to DDBM buffer index.
				// Both HWA_PP_PAGEMGR_FREE_DDBM and
				// HWA_PP_PAGEMGR_FREE_DDBM_LINK are verified.
#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
				// Free: PULL process done case. HDBM part
				hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_HDBM,
					DBM_AUDIT_FREE);
#endif
				hwa_pktpgr_dbm_free(dev, HWA_DDBM, HWA_TABLE_INDX(hwa_pp_lbuf_t,
					pktpgr->dnglpktpool_mem, txlbuf), TRUE);
			} else {
				// 6715B0-RC3, add PUSH_PKTTAG command.
				spktq_enq(&dirty_q, txlbuf);
			}
		} else {
			// Clean, just free to DDBM
			//HWA_INFO(("  Map result is clean, free to DDBM pkt_mapid<%d>\n",
			//	PKTMAPID(txlbuf)));

			// Free to DDBM by DDBM buffer index.
			// Note: pkt_mapid is not equal to DDBM buffer index.
			// Both HWA_PP_PAGEMGR_FREE_DDBM and
			// HWA_PP_PAGEMGR_FREE_DDBM_LINK are verified.
			if (HWAREV_GE(dev->corerev, 133)) {
				spktq_enq(&clean_q, txlbuf);
			} else {
#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
				// Free: PULL process done case. HDBM part
				hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_HDBM,
					DBM_AUDIT_FREE);
#endif
				hwa_pktpgr_dbm_free(dev, HWA_DDBM, HWA_TABLE_INDX(hwa_pp_lbuf_t,
					pktpgr->dnglpktpool_mem, txlbuf), TRUE);
			}
		}

		// next
		txlbuf = spktq_deq(tag_pull_q);

		// A0: Because the next pull(if any) needs pkt_mapid as a key, the
		// current last packet can be a key for next pull.
		// Ignore current last packet process.
		// B0: Althrough we can use the same logic as above but let use
		// HWA_PP_PAGEMGR_PULL_KPFL_LINK to verify the design.
		// The PKTLINK of tail txlbuf is HDBM lo_addr space.
		// Use hwa_pktpgr_hdbm_ptr2idx to get next pkt_mapid for next pull if any.
		// B0: Since I use spktq to store PULLed packets, so I cannot use
		// HWA_PP_PAGEMGR_PULL_KPFL_LINK
		if (!last && spktq_empty(tag_pull_q)) {
			// Clean, just free the tail to DDBM.
			//HWA_INFO(("  Not last call, use tail pkt_mapid<%d> for next PULL."
			//	" Bypass it.\n", PKTMAPID(txlbuf)));
			// Free to DDBM
			if (HWAREV_GE(dev->corerev, 133)) {
				spktq_enq(&clean_q, txlbuf);
			} else {
#if defined(HWA_PKTPGR_DBM_AUDIT_ENABLED)
				// Free: PULL process done case. HDBM part.
				hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_HDBM,
					DBM_AUDIT_FREE);
#endif
				hwa_pktpgr_dbm_free(dev, HWA_DDBM, HWA_TABLE_INDX(hwa_pp_lbuf_t,
					pktpgr->dnglpktpool_mem, txlbuf), TRUE);
			}
			break;
		}
	}

	HWA_ASSERT(spktq_empty(tag_pull_q));

	processed = last ? mpdu_cnt : (mpdu_cnt-1);

	if (HWAREV_GE(dev->corerev, 133)) {
		uint16 n_pkts;

		// Push dirty pkttag
		n_pkts = spktq_n_pkts(&dirty_q);
		if (n_pkts) {
			hwa_pktpgr_push_pkttag_req(dev, fifo_idx,
				HWA_PP_CMD_TAGGED, spktq_peek_head(&dirty_q),
				spktq_peek_tail(&dirty_q), n_pkts, n_pkts);
		}

		// Free clean
		n_pkts = spktq_n_pkts(&clean_q);
		if (n_pkts) {
			hwa_pktpgr_free_ddbm_pkt(dev, spktq_peek_head(&clean_q),
				spktq_peek_tail(&clean_q), n_pkts);
		}

		// Deinit the temporary q
		spktq_deinit(&dirty_q);
		spktq_deinit(&clean_q);
	}

	return processed;
}

// HW shadow map
// PULL-MAP-PUSH OvfQ
// Need to consider the DDBM availability
#define PULL_MPDU_CNT_MAX 64
void
hwa_pktpgr_map_pkts(hwa_dev_t *dev, uint32 fifo_idx, void *cb, void *ctx)
{
	hwa_pktpgr_t *pktpgr;
	hwa_txfifo_ovflwqctx_t ovflwq;
	hwa_pp_pagemgr_req_pull_t req;
	int processed, trans_id, ovfq_mpdu_cnt, pull_mpdu_cnt, req_loop;
	uint16 pkt_mapid;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	// Setup locals
	pktpgr = &dev->pktpgr;

	// Get OvfQ
	ovfq_mpdu_cnt = hwa_txfifo_get_ovfq(dev, fifo_idx, &ovflwq);
	if (ovfq_mpdu_cnt <= 0)
		return;

	HWA_INFO(("%d MPDU in HW shadow.\n", ovfq_mpdu_cnt));

	// Get first pkt_mapid, for HDBM buffer index is equal to pkt_mapid
	pkt_mapid = hwa_pktpgr_hdbm_ptr2idx(pktpgr, PHYSADDR64LO(ovflwq.pktq_head));

	// Decide pull_mpdu_cnt
	pull_mpdu_cnt = ovfq_mpdu_cnt;
	if (pull_mpdu_cnt > 1) {
		pull_mpdu_cnt = MIN(pull_mpdu_cnt, PULL_MPDU_CNT_MAX);
		pull_mpdu_cnt = MIN(pull_mpdu_cnt, pktpgr->ddbm_avail_sw);
		if (pull_mpdu_cnt < 2) {
			HWA_INFO(("ddbm_avail_sw<%d>, force to request 2 mpdu at least.\n",
				pktpgr->ddbm_avail_sw));
			pull_mpdu_cnt = 2; // 2 at least
		}
	}

	while (ovfq_mpdu_cnt) {
		// Request tagged PULL
		// B0: Since I use spktq to store PULLed packets, so
		// I cannot use HWA_PP_PAGEMGR_PULL_KPFL_LINK
		req.trans        = HWA_PP_PAGEMGR_PULL;
		req.pkt_count    = pull_mpdu_cnt; // mpdu_count base actually.
		req.pkt_mapid    = pkt_mapid;
		req.tagged       = HWA_PP_CMD_TAGGED;

#ifdef HWA_QT_TEST
		// XXX, CRBCAHWA-662
		if (HWAREV_GE(dev->corerev, 133)) {
			req.swar = 0xBFBF;
		}
#endif

		req_loop = HWA_LOOPCNT;

req_again:
		trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagemgr_req_ring, &req);
		if (trans_id == HWA_FAILURE) {
			if (--req_loop > 0) {
				OSL_DELAY(10);
				goto req_again;
			}
			HWA_ERROR(("Tagged PULL_REQ failure: pkts<%d> pkt_mapid %3u",
				pull_mpdu_cnt, pkt_mapid));
			HWA_ASSERT(0);
			return;
		}

		// DDBM accounting.
		HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, pull_mpdu_cnt);
		HWA_DDBM_COUNT_LWM(pktpgr->ddbm_avail_sw, pktpgr->ddbm_avail_sw_lwm);
		HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_tx, pull_mpdu_cnt);
		HWA_DDBM_COUNT_HWM(pktpgr->ddbm_sw_tx, pktpgr->ddbm_sw_tx_hwm);
		HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", pull_mpdu_cnt,
			pktpgr->ddbm_avail_sw, __FUNCTION__);

		HWA_TRACE(("PULL HW shadow pkts<%d> pkt_mapid %3u\n", pull_mpdu_cnt, pkt_mapid));

		HWA_COUNT_INC(pktpgr->tag_pull_req, 1);

		// Wait for tag PULL resp.
		hwa_pktpgr_tag_pull_wait_to_finish(dev);

		// Process tag PULL resp, PUSH dirty else free to DDBM
		processed = hwa_pktpgr_tag_pull_process(dev,
			pull_mpdu_cnt, fifo_idx, cb, ctx, &pkt_mapid,
			(pull_mpdu_cnt == ovfq_mpdu_cnt));
		HWA_INFO(("Processed <%d> HW shadow packets on fifo_idx<%d>\n",
			processed, fifo_idx));

		// Next PULL
		ovfq_mpdu_cnt -= processed;

		// Decide new pull_mpdu_cnt
		pull_mpdu_cnt = ovfq_mpdu_cnt;
		if (pull_mpdu_cnt > 1) {
			pull_mpdu_cnt = MIN(pull_mpdu_cnt, PULL_MPDU_CNT_MAX);
			pull_mpdu_cnt = MIN(pull_mpdu_cnt, pktpgr->ddbm_avail_sw);
			if (pull_mpdu_cnt < 2) {
				HWA_INFO(("ddbm_avail_sw<%d>, force to request 2 mpdu at least.\n",
					pktpgr->ddbm_avail_sw));
				pull_mpdu_cnt = 2; // 2 at least
			}
		}
	}

	// Wait for PUSH complete before return.
	hwa_pktpgr_tag_push_wait_to_finish(dev);
}

static void
hwa_pktpgr_pageout_req_ring_map_pkts(hwa_dev_t *dev, void *cb, void *ctx)
{
	uint16 depth;
	int i, wr_idx, rd_idx;
	hwa_ring_t *req_ring;
	hwa_pp_cmd_t *req_cmd;
	hwa_pp_lbuf_t *txlbuf;
	hwa_pp_pageout_req_pktlist_t *pktlist;
	hwa_pp_pageout_req_pktlocal_t *pktlocal;
	map_pkts_cb_fn map_pkts_cb = (map_pkts_cb_fn)cb;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);

	// Setup locals
	req_ring = &dev->pktpgr.pageout_req_ring;

	// Is empty ? hwa_ring_is_cons_all will sync HW read.
	if (hwa_ring_is_cons_all(req_ring))
		return;

	wr_idx = HWA_RING_STATE(req_ring)->write;
	rd_idx = HWA_RING_STATE(req_ring)->read;
	depth = req_ring->depth;

	HWA_INFO(("Walk \"PO>\" ring for packet mapping. R:W[%d:%d]\n", rd_idx, wr_idx));

	while (rd_idx != wr_idx) {
		// Fetch req cmd
		req_cmd = HWA_RING_ELEM(hwa_pp_cmd_t, req_ring, rd_idx);
		pktlist = &req_cmd->pageout_req_pktlist;
		pktlocal = &req_cmd->pageout_req_pktlocal;
		// HWA_PP_PAGEOUT_PKTLOCAL ??
		if ((pktlist->invalid == HWA_PP_CMD_INVALID) ||
			(pktlist->trans != HWA_PP_PAGEOUT_PKTLIST_NR &&
			pktlist->trans != HWA_PP_PAGEOUT_PKTLIST_WR &&
			pktlist->trans != HWA_PP_PAGEOUT_PKTLOCAL)) {
			goto next;
		}

		if (pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_NR ||
			pktlist->trans == HWA_PP_PAGEOUT_PKTLIST_WR) {
			// Sanity check
			HWA_ASSERT(pktlist->pkt_count);
			HWA_ASSERT(pktlist->mpdu_count);
			HWA_ASSERT(PKTLINK(pktlist->pktlist_tail) == NULL);
			HWA_ASSERT(pktlist->pktlist_head != NULL);

			// Walk through list.
			txlbuf = (hwa_pp_lbuf_t *)pktlist->pktlist_head;

			HWA_INFO(("%s: Found one pktlist<0x%p> pkt_mapid<%d> count<%d>\n",
				__FUNCTION__, txlbuf, PKTMAPID(txlbuf), pktlist->mpdu_count));
			for (i = 0; i < pktlist->mpdu_count; i++) {
				HWA_TRACE(("  mapped txlbuf<0x%p> pkt_mapid<%d>\n",
					txlbuf, PKTMAPID(txlbuf)));
				// Do map cb
				(void)map_pkts_cb(ctx, (void*)txlbuf);
				// Next one
				txlbuf = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
			}
		} else {
			// Sanity check
			HWA_ASSERT(pktlocal->pkt_local != 0U);
			// Walk through list.
			txlbuf = (hwa_pp_lbuf_t *)pktlocal->pkt_local;
			HWA_TRACE(("  mapped local txlbuf<0x%p> pkt_mapid<%d>\n",
				txlbuf, PKTMAPID(txlbuf)));
			(void)map_pkts_cb(ctx, (void*)txlbuf);
		}
next:
		// Next.
		rd_idx = (rd_idx + 1) % depth;
	}
}

static void
hwa_pktpgr_pagein_rsp_ring_map_pkts(hwa_dev_t *dev, void *cb, void *ctx)
{
	uint16 depth;
	int i, wr_idx, rd_idx;
	hwa_ring_t *rsp_ring;
	hwa_pp_cmd_t *rsp_cmd;
	hwa_pp_lbuf_t *txlbuf;
	hwa_pp_pagein_rsp_txstatus_t *txstatus;
	map_pkts_cb_fn map_pkts_cb = (map_pkts_cb_fn)cb;

	HWA_FTRACE(HWApp);
	HWA_ASSERT(dev != (struct hwa_dev *)NULL);

	// Setup locals
	rsp_ring = &dev->pktpgr.pagein_rsp_ring;

	// Is empty?
	hwa_ring_cons_get(rsp_ring); // Update H2S ring WR
	if (hwa_ring_is_empty(rsp_ring))
		return;

	wr_idx = HWA_RING_STATE(rsp_ring)->write;
	rd_idx = HWA_RING_STATE(rsp_ring)->read;
	depth = rsp_ring->depth;

	HWA_INFO(("Walk \"PI<\" ring for packet mapping\n"));

	while (rd_idx != wr_idx) {
		// Fetch rsp cmd
		rsp_cmd = HWA_RING_ELEM(hwa_pp_cmd_t, rsp_ring, rd_idx);
		txstatus = &rsp_cmd->pagein_rsp_txstatus;
		if (txstatus->invalid == HWA_PP_CMD_INVALID ||
			txstatus->trans != HWA_PP_PAGEIN_TXSTATUS) {
			HWA_TRACE(("%s: goto next trans<%d>\n", __FUNCTION__, txstatus->trans));
			goto next;
		}

#ifdef HWA_QT_TEST
		// XXX, CRBCAHWA-662
		if (HWAREV_GE(dev->corerev, 133) && txstatus->swar != 0xBF) {
			HWA_PRINT(" --> ERR: %s(%d): swar 0x%x\n", __FUNCTION__,
				__LINE__, txstatus->swar);
		}
#endif

		if (txstatus->pkt_count == 0 || txstatus->mpdu_count == 0) {
			HWA_INFO(("%s: pkt_count<%d> mpdu_count<%d>\n", __FUNCTION__,
				txstatus->pkt_count, txstatus->mpdu_count));
			goto next;
		}

		// Sanity check
		HWA_ASSERT(PKTLINK(txstatus->pktlist_tail) == NULL);
		HWA_ASSERT(txstatus->pktlist_head != NULL);

		// Walk through list.
		txlbuf = (hwa_pp_lbuf_t *)txstatus->pktlist_head;

		HWA_INFO(("  Found one pktlist<0x%p> pkt_mapid<%d> count<%d>\n",
			txlbuf, PKTMAPID(txlbuf), txstatus->mpdu_count));
		for (i = 0; i < txstatus->mpdu_count; i++) {
			HWA_TRACE(("    mapped txlbuf<0x%p> pkt_mapid<%d>\n",
				txlbuf, PKTMAPID(txlbuf)));
			if (PKTISMGMTTXPKT(dev->osh, txlbuf))
				(void)map_pkts_cb(ctx, (void*)PKTLOCAL(dev->osh, txlbuf));
			else
				(void)map_pkts_cb(ctx, (void*)txlbuf);
			txlbuf = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
		}
next:
		// Next.
		rd_idx = (rd_idx + 1) % depth;
	}
}

void
hwa_pktpgr_ring_map_pkts(struct hwa_dev *dev, void *cb, void *ctx)
{
	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// 1. PageInRsp map
	hwa_pktpgr_pagein_rsp_ring_map_pkts(dev, cb, ctx);

	// 2. PageOutReq map
	hwa_pktpgr_pageout_req_ring_map_pkts(dev, cb, ctx);
}

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED

/* Audits DDBM */
static inline void
_hwa_ddbm_audit(hwa_pktpgr_t *pktpgr, uint32 bufidx, bool dir_tx, const bool alloc,
	const char *func, uint32 line)
{
	// Always +1, because the bufidx can be zero.
	bufidx += 1;

	if (bufidx > HWA_DNGL_PKTS_MAX) {
		HWA_ERROR((_R_ "%s(%d): Invalid DDBM bufidx<%d>, %s" _N_ "\n", func, line,
			bufidx - 1, alloc ? "alloc" : "free"));
		return;
	}

	if (alloc) {
		if (!bcm_mwbmap_isfree(pktpgr->ddbm_map, bufidx)) {
			HWA_ERROR((_R_ "%s(%d): Get duplicate DDBM<%d>" _N_ "\n", func, line,
				bufidx - 1));
			HWA_ERROR((_R_ "    Last access is %s(%d)" _N_ "\n",
				pktpgr->ddbm_func[bufidx], pktpgr->ddbm_line[bufidx]));
			pktpgr->ddbm_err_alloc++;
			return;
		}
		bcm_mwbmap_force(pktpgr->ddbm_map, bufidx);
	} else {
		if (bcm_mwbmap_isfree(pktpgr->ddbm_map, bufidx)) {
			HWA_ERROR((_R_ "%s(%d): Double free DDBM<%d>" _N_ "\n", func, line,
				bufidx - 1));
			HWA_ERROR((_R_ "    Last access is %s(%d)" _N_ "\n",
				pktpgr->ddbm_func[bufidx], pktpgr->ddbm_line[bufidx]));
			pktpgr->ddbm_err_free++;
			return;
		}
		bcm_mwbmap_free(pktpgr->ddbm_map, bufidx);
	}

	// Save last access
	pktpgr->ddbm_func[bufidx] = func;
	pktpgr->ddbm_line[bufidx] = (uint16)line;
} // _hwa_ddbm_audit

/* Audit HDBM */
static inline void
_hwa_hdbm_audit(hwa_pktpgr_t *pktpgr, uint32 pkt_mapid, bool dir_tx, const bool alloc,
	const char *func, uint32 line)
{
	// Always +1, because the pkt_mapid can be zero.
	pkt_mapid += 1;

	if (pkt_mapid > HWA_HOST_PKTS_MAX) {
		HWA_ERROR((_R_ "%s(%d): Invalid HDBM bufidx<%d>, %s" _N_ "\n", func, line,
			pkt_mapid - 1, alloc ? "alloc" : "free"));
		return;
	}

	if (alloc) {
		if (!bcm_mwbmap_isfree(pktpgr->hdbm_map, pkt_mapid)) {
			HWA_ERROR((_R_ "%s(%d): Get duplicate HDBM<%d>" _N_ "\n", func, line,
				pkt_mapid - 1));
			HWA_ERROR((_R_ "    Last access is %s(%d)" _N_ "\n",
				pktpgr->hdbm_func[pkt_mapid], pktpgr->hdbm_line[pkt_mapid]));
			pktpgr->hdbm_err_alloc++;
			return;
		}
		bcm_mwbmap_force(pktpgr->hdbm_map, pkt_mapid);
	} else {
		if (bcm_mwbmap_isfree(pktpgr->hdbm_map, pkt_mapid)) {
			HWA_ERROR((_R_ "%s(%d): Double free HDBM<%d>" _N_ "\n", func, line,
				pkt_mapid - 1));
			HWA_ERROR((_R_ "    Last access is %s(%d)" _N_ "\n",
				pktpgr->hdbm_func[pkt_mapid], pktpgr->hdbm_line[pkt_mapid]));
			pktpgr->hdbm_err_alloc++;
			return;
		}
		bcm_mwbmap_free(pktpgr->hdbm_map, pkt_mapid);
	}

	//Save last access
	pktpgr->hdbm_func[pkt_mapid] = func;
	pktpgr->hdbm_line[pkt_mapid] = (uint16)line;
} // _hwa_rxfill_rxfree_audit_hdbm

void
_hwa_dbm_idx_audit(struct hwa_dev *dev, uint32 dbm_idx, bool dir_tx,
	uint32 type, const bool alloc, const char *func, uint32 line)
{
	if (type == DBM_AUDIT_DDBM) {
		// DDBM audit
		_hwa_ddbm_audit(&dev->pktpgr, dbm_idx, dir_tx, alloc, func, line);
	} else {
		// HDBM audit
		_hwa_hdbm_audit(&dev->pktpgr, dbm_idx, dir_tx, alloc, func, line);
	}
}

/* Audits the DBM buffer id between HWA and SW */
void
_hwa_pktpgr_dbm_audit(struct hwa_dev *dev, void *dbm, bool dir_tx,
	uint32 type, const bool alloc, const char *func, uint32 line)
{
	uint32 bufidx, pkt_mapid;
	hwa_pktpgr_t *pktpgr;

	// Setup locals
	pktpgr = &dev->pktpgr;

	// Audit parameters and pre-conditions
	HWA_ASSERT(dbm != NULL);
	pkt_mapid = PKTMAPID(dbm);
	bufidx = HWA_TABLE_INDX(hwa_pp_lbuf_t, pktpgr->dnglpktpool_mem, dbm);

	// DDBM: align at size of hwa_pp_lbuf_t
	HWA_ASSERT((((uintptr)dbm - HWA_PTR2UINT(
		pktpgr->dnglpktpool_mem)) % sizeof(hwa_pp_lbuf_t)) == 0);

	// DDBM audit
	if (type & DBM_AUDIT_DDBM) {
		_hwa_ddbm_audit(pktpgr, bufidx, dir_tx, alloc, func, line);
	}

	if (type & DBM_AUDIT_HDBM) {
		// HDBM audit
		_hwa_hdbm_audit(pktpgr, pkt_mapid, dir_tx, alloc, func, line);
	}
} // hwa_pktpgr_dbm_audit

void
hwa_pktpgr_dbm_audit_dump(hwa_pktpgr_t *pktpgr, uint32 type)
{
	uint32 i, count, total;
	uint32 err_alloc, err_free;
	struct bcm_mwbmap *dbm_map;
	char *who;

	if (type == DBM_AUDIT_DDBM) {
		who = "DDBM";
		dbm_map = pktpgr->ddbm_map;
		total = pktpgr->dnglpktpool_max;
		err_alloc = pktpgr->ddbm_err_alloc;
		err_free = pktpgr->ddbm_err_free;
	} else {
		who = "HDBM";
		dbm_map = pktpgr->hdbm_map;
		total = pktpgr->hostpktpool_max;
		err_alloc = pktpgr->hdbm_err_alloc;
		err_free = pktpgr->hdbm_err_free;
	}

	// Always -1, because the bufidx can be zero.
	count = 0;
	for (i = 1; i <= total; i++) {
		if (!bcm_mwbmap_isfree(dbm_map, i)) {
			if (type == DBM_AUDIT_DDBM) {
				HWA_PRINT("[%4d] by %s(%d)\n", i - 1, pktpgr->ddbm_func[i],
					pktpgr->ddbm_line[i]);
			} else {
				HWA_PRINT("[%4d] by %s(%d)\n", i - 1, pktpgr->hdbm_func[i],
					pktpgr->hdbm_line[i]);

			}
			count++;
		}
	}
	HWA_PRINT("Total %d %s in SW. err alloc<%d> free<%d>\n\n",
		count, who, err_alloc, err_free);
}
#endif /* HWA_PKTPGR_DBM_AUDIT_ENABLED */

void
hwa_hdbm_dump(hwa_dev_t *dev, uint32 pktmap_id, uint32 offset, uint32 size)
{
	hwa_pktpgr_t *pktpgr;
	uint64 haddr_u64;

	// Setup locals
	pktpgr = &dev->pktpgr;

	HADDR64_TO_U64(pktpgr->hostpktpool_haddr64, haddr_u64);
	haddr_u64 += pktmap_id * sizeof(hwa_pp_lbuf_t);
	haddr_u64 += offset;

	/* Print Host contents using sbtopcie print service */
	sbtopcie_print(haddr_u64, size);
}

#endif /* HWA_PKTPGR_BUILD */
