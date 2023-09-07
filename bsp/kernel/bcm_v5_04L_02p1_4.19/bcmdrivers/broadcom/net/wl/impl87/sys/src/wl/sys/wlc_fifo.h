/*
 * MU-MIMO transmit module for Broadcom 802.11 Networking Adapter Device Drivers
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
 * $Id: wlc_fifo.h 804593 2021-11-03 23:29:18Z $
 */

#ifndef _wlc_fifo_h_
#define _wlc_fifo_h_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlc_tx.h>
#include <wlc_txcfg.h>

/* Standard attach and detach functions */
wlc_fifo_info_t* BCMATTACHFN(wlc_fifo_attach)(wlc_info_t *wlc);
void BCMATTACHFN(wlc_fifo_detach)(wlc_fifo_info_t *fifo_info);

/* public APIs. */
extern uint8 wlc_fifo_index_get(wlc_fifo_info_t *fifo_info, struct scb *scb, enum wme_ac ac);
extern uint8 wlc_fifo_index_peek(wlc_fifo_info_t *fifo_info, struct scb *scb, enum wme_ac ac);
extern uint8 wlc_fifo_max_get(wlc_fifo_info_t *fifo_info);
extern void wlc_fifo_alloc(wlc_fifo_info_t *fifo_info, struct scb *scb,
	mu_type_t mu, enum wme_ac ac);
extern void wlc_fifo_free_all(wlc_fifo_info_t *fifo_info, scb_t *scb);
extern void wlc_fifo_free_ac(wlc_fifo_info_t *fifo_info, scb_t *scb, enum wme_ac ac);
extern void wlc_fifo_sta_bitmap(wlc_fifo_info_t *fifo_info, struct scb *scb,
	void *fifo_bitmap);
extern bool wlc_fifo_isMU(wlc_fifo_info_t *fifo_info, struct scb *scb, enum wme_ac ac);
extern bool wlc_check_fifo_type(wlc_fifo_info_t *fifo_info, struct scb *scb, enum wme_ac ac,
	mu_type_t mu);
extern uint8 wlc_fifo_get_ac(wlc_fifo_info_t *fifo_info, uint fifo_index);

extern void wlc_fifo_mutx_ac_release(wlc_fifo_info_t *fifo_info, wl_mutx_ac_mg_t *mutx_ac_mask);
extern int wlc_fifo_avail_count(wlc_fifo_info_t *fifo_info, enum wme_ac ac, mu_type_t mu);
extern int wlc_fifo_global_remaining(wlc_fifo_info_t *fifo_info);
extern bool wlc_fifo_all_released(wlc_fifo_info_t *fifo_info);
extern void wlc_fifo_sta_mu_bitmap(wlc_fifo_info_t *fifo_info, struct scb *scb, void *fifo_bitmap);
extern bool wlc_fifo_is_ulofdma(wlc_fifo_info_t *fifo_info, struct scb *scb, enum wme_ac);
extern int wlc_fifo_max_per_ac(wlc_fifo_info_t *fifo_info, mu_type_t mu);
extern int wlc_scb_mu_fifo_count(wlc_fifo_info_t *fifo_info, struct scb *scb);
extern int wlc_fifo_user_count(wlc_fifo_info_t *fifo_info, struct scb *scb, enum wme_ac ac);
extern int wlc_fifo_remaining_mu(wlc_fifo_info_t *fifo_info, mu_type_t mu);
extern int wlc_mu_fifo_count(wlc_fifo_info_t *fifo_info, mu_type_t mu);
extern int wlc_fifo_dlmmu_avail_count(wlc_fifo_info_t *fifo_info);
extern uint16 wlc_fifo_sta_ulo_bitmap(wlc_fifo_info_t *fifo_info, struct scb *scb,
		uint16 *fifo_bitmap);
extern bool wlc_fifo_overload_allowed(wlc_fifo_info_t *fifo_info, mu_type_t mu);
extern uint32 wlc_fifo_scb_inflt_ac(struct scb *scb, enum wme_ac ac);

#define FIFO_INDEX_GET_AC(scb, ac) (scb)->fifo_idx[(ac)]

extern bool wlc_fifo_dlmmu_set_max_per_ac(wlc_fifo_info_t *fifo_info,
	dlmmu_subtype_t subtype, int ac, int count);
extern int wlc_fifo_dlmmu_get_max_per_ac(wlc_fifo_info_t *fifo_info,
	dlmmu_subtype_t subtype, int ac);
extern int wlc_fifo_dlmmu_get_inuse_per_ac(wlc_fifo_info_t *fifo_info,
	dlmmu_subtype_t subtype, int ac);
extern void wlc_fifo_dlmmu_reset_subtype_max(wlc_fifo_info_t *fifo_info);
extern int wlc_fifo_inuse(wlc_fifo_info_t *fifo_info, uint8 *fifo_bitmap);
extern int wlc_fifo_free_in_progress_count(wlc_fifo_info_t *fifo_info);
extern bool wlc_fifo_switch_mu_type(wlc_fifo_info_t *fifo_info, uint fifo_index, enum wme_ac ac,
	mu_type_t from_mu, uint8 from_mu_sub_type, mu_type_t to_mu, uint8 to_mu_sub_type);
extern bool wlc_fifo_is_deep_dma(wlc_fifo_info_t *fifo_info, struct scb *scb, enum wme_ac ac);
#endif   /* _wlc_fifo_h_ */
