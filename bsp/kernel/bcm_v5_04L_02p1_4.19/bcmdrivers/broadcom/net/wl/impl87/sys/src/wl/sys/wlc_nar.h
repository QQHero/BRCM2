/*
 * wlc_nar.h
 *
 * This module contains the external definitions for the NAR transmit module.
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
 * $Id: wlc_nar.h 801610 2021-07-28 09:01:31Z $
 *
 */

/**
 * Hooked in the transmit path at the same level as the A-MPDU transmit module, it provides balance
 * amongst MPDU and AMPDU traffic by regulating the number of in-transit packets for non-aggregating
 * stations.
 */

#if !defined(__WLC_NAR_H__)
#define __WLC_NAR_H__

/*
 * Module attach and detach functions.
 */
extern wlc_nar_info_t *wlc_nar_attach(wlc_info_t *);

extern void wlc_nar_detach(wlc_nar_info_t *);

extern void wlc_nar_dotxstatus(wlc_nar_info_t *, struct scb *scb, void *sdu, tx_status_t *txs,
	bool pps_retry, uint32 tx_rate_prim, ratespec_t pri_rspec);

extern void wlc_nar_addba(wlc_nar_info_t *nit, struct scb *scb, int prec);

extern void wlc_nar_scb_close_link(wlc_info_t *wlc, struct scb *scb);

#ifdef WLTAF
#include <wlc_taf.h>

extern bool wlc_nar_taf_release(void* narh, void* scbh, void* tidh, bool force,
	taf_scheduler_public_t* taf);
extern void * wlc_nar_get_taf_scb_info(void *narh, struct scb* scb);
extern void * wlc_nar_get_taf_scb_prec_info(void *scb_h, int tid);
extern uint16 wlc_nar_get_taf_scb_prec_pktlen(void *narh, void *scbh, void *tidh, uint32 ts);
#endif

#ifdef PKTQ_LOG
extern struct pktq *wlc_nar_prec_pktq(wlc_info_t* wlc, struct scb* scb);
#endif

#ifdef PROP_TXSTATUS
extern struct pktq *wlc_nar_txq(wlc_nar_info_t * nit, struct scb *scb);
#endif

extern void wlc_nar_flush_scb_queues(wlc_nar_info_t * nit, struct scb *scb);
extern uint32 wlc_nar_tx_in_tansit(wlc_nar_info_t *nit);
extern uint16 wlc_scb_nar_n_pkts(wlc_nar_info_t * nit, struct scb *scb, uint8 prio);
extern uint wlc_nar_scb_txpktcnt(wlc_nar_info_t * nit, struct scb *scb);
extern uint wlc_nar_scb_pktq_mlen(wlc_nar_info_t * nit, struct scb *scb, uint32 precbitmap);
#endif /* __WLC_NAR_H__ */
