/*
 * Common (OS-independent) portion of Broadcom 802.11 Networking Device Driver
 * Tx and PhyRx status transfer module
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
 * $Id: wlc_sts_xfer.h 803989 2021-10-14 19:10:00Z $
 */

#ifndef _WLC_STS_XFER_
#define _WLC_STS_XFER_

/* Tx and PhyRx Status Tranfer module */
struct wlc_sts_xfer_info {
	int	unit;		/* Radio unit */
};

/* STS_XFER module attach and detach handlers */
extern wlc_sts_xfer_info_t *wlc_sts_xfer_attach(wlc_info_t *wlc);
extern void wlc_sts_xfer_detach(wlc_sts_xfer_info_t *wlc_sts_xfer_info);

#if defined(STS_XFER_TXS)

/* STS_XFER process available Tx Statuses */
extern bool	wlc_sts_xfer_txs_process(wlc_info_t *wlc, bool bound, bool *fatal);
/** STS_XFER Tx Status reclaim */
extern void	wlc_sts_xfer_txs_reclaim(wlc_info_t *wlc, bool reinit);

/* STS_XFER Tx Status access handlers */
extern void *	wlc_sts_xfer_txs_get_status(wlc_info_t *wlc);
extern bool	wlc_sts_xfer_txs_bmac_txstatus_done(wlc_info_t *wlc);

#if defined(STS_XFER_TXS_PP)
/* STS_XFER Tx Status page-in response callback */
extern void	wlc_sts_xfer_txs_pagein_process(wlc_info_t *wlc);
#endif /* STS_XFER_TXS_PP */

/* STS_XFER dump Tx Status error logs */
extern void	wlc_sts_xfer_txs_error_dump(wlc_info_t *wlc, void *status, bool reg_dump);

#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS)

#define STS_XFER_PHYRXS_SEQID_INVALID		((uint16) 0xffff)

/** STS_XFER PhyRx Status reclaim */
extern void wlc_sts_xfer_phyrxs_reclaim(wlc_info_t *wlc);

/** STS_XFER PhyRx Status receive handlers */
extern void wlc_sts_xfer_bmac_recv(wlc_info_t *wlc, uint fifo,
	rx_list_t *rx_list_release, uint8 rxh_offset);
extern void wlc_sts_xfer_bmac_recv_done(wlc_info_t *wlc, uint fifo);

/** STS_XFER PhyRx Status buffer release handlers */
extern void wlc_sts_xfer_phyrxs_release(wlc_info_t *wlc, void *pkt);
extern void wlc_sts_xfer_phyrxs_free(wlc_info_t *wlc, void *pkt);

/** STS_XFER current PhyRx Status handlers */
extern void wlc_sts_xfer_phyrxs_set_cur_status(wlc_info_t *wlc, void *pkt, d11rxhdr_t *rxh);
extern void wlc_sts_xfer_phyrxs_release_cur_status(wlc_info_t *wlc);

/** Rx packets pending */
extern bool wlc_sts_xfer_phyrxs_rxpend(wlc_info_t *wlc, uint fifo);
#endif /* STS_XFER_PHYRXS */

#if defined(STS_XFER_M2M_INTR)

extern char * wlc_sts_xfer_irqname(wlc_info_t *wlc, void *btparam);
#if defined(DONGLEBUILD)
#include <rte_isr.h>

/* Register an IRQ handler for M2M interrupts */
extern int wlc_sts_xfer_isr_register(wlc_info_t *wlc, uint orig_unit, uint bus,
		hnd_isr_fn_t isr_fn, hnd_isr_sec_fn_t isr_sec_fn, hnd_worklet_fn_t worklet_fn,
		void *cbdata, osl_ext_task_t* thread);
#else /* ! DONGLEBUILD */
extern uint wlc_sts_xfer_m2m_si_flag(wlc_info_t *wlc);
#endif /* ! DONGLEBUILD */

/** STS_XFER IRQ handler for M2M core interrupts */
extern bool BCMFASTPATH wlc_sts_xfer_isr(wlc_info_t *wlc, bool *wantdpc);

extern void BCMFASTPATH wlc_sts_xfer_intrsupd(wlc_info_t *wlc);
extern void BCMFASTPATH wlc_sts_xfer_process_m2m_intstatus(wlc_info_t *wlc);

extern void wlc_sts_xfer_intrson(wlc_info_t *wlc);
extern void wlc_sts_xfer_intrsoff(wlc_info_t *wlc);
extern void wlc_sts_xfer_intrsrestore(wlc_info_t *wlc, uint32 macintmask);
extern void wlc_sts_xfer_intrs_deassert(wlc_info_t *wlc);

#endif /* STS_XFER_M2M_INTR */

#endif /* _WLC_STS_XFER_ */
