/*
 * ACPHY TXIQLO CAL module interface (to other PHY modules).
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
 * $Id: phy_ac_txiqlocal.h 791053 2020-09-15 02:11:42Z $
 */

#ifndef _phy_ac_txiqlocal_h_
#define _phy_ac_txiqlocal_h_

#include <phy_api.h>
#include <phy_ac.h>
#include <phy_txiqlocal.h>

/* forward declaration */
typedef struct phy_ac_txiqlocal_info phy_ac_txiqlocal_info_t;

/* register/unregister ACPHY specific implementations to/from common */
phy_ac_txiqlocal_info_t *phy_ac_txiqlocal_register_impl(phy_info_t *pi,
	phy_ac_info_t *aci, phy_txiqlocal_info_t *mi);
void phy_ac_txiqlocal_unregister_impl(phy_ac_txiqlocal_info_t *info);

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

/* ************************************************************************* */
/* The following definitions shared between TSSI, Cache, RxIQCal and TxIQLOCal			*/
/* ************************************************************************* */
#define ACPHY_IQCAL_TONEFREQ_16MHz 16000
#define ACPHY_IQCAL_TONEFREQ_8MHz 8000
#define ACPHY_IQCAL_TONEFREQ_4MHz 4000
#define ACPHY_IQCAL_TONEFREQ_2MHz 2000
#define ACPHY_IQCAL_TONEFREQ_1MHz 1000
#ifdef WLC_TXFDIQ
#define ACPHY_TXCAL_MAX_NUM_FREQ 4
#endif

typedef struct {
	uint8 lofttbl_start_idx;
	uint8 lofttbl_end_idx;
} txiqlocal_multilo_t;

/* Enable the pre-RX TxIQ cal for these revs */
#define ACPHY_TXCAL_PRERXCAL(pi) \
	(!TINY_RADIO(pi))

int8 phy_ac_txiqlocal_get_fdiqi_slope(phy_ac_txiqlocal_info_t *txiqlocali, uint8 core);
void phy_ac_txiqlocal_set_fdiqi_slope(phy_ac_txiqlocal_info_t *txiqlocali, uint8 core, int8 slope);
bool phy_ac_txiqlocal_is_fdiqi_enabled(phy_ac_txiqlocal_info_t *txiqlocali);
extern void wlc_phy_poll_samps_WAR_acphy(phy_info_t *pi, int16 *samp, bool is_tssi,
                                         bool for_idle, txgain_setting_t *target_gains,
                                         bool for_iqcal, bool init_adc_inside, uint16 core,
                                         bool champ);
extern void wlc_phy_cal_txiqlo_coeffs_acphy(phy_info_t *pi, uint8 rd_wr, uint16 *coeff_vals,
	uint8 select, uint8 core);
extern void wlc_phy_ipa_set_bbmult_acphy(phy_info_t *pi, uint16 *m0, uint16 *m1,
	uint16 *m2, uint16 *m3, uint8 coremask);
void wlc_phy_precal_txgain_acphy(phy_info_t *pi, txgain_setting_t *target_gains,
	uint8 multilo_cal_cnt, bool Biq2byp);
uint8 wlc_phy_get_tbl_id_iqlocal(phy_info_t *pi, uint16 core);
extern void wlc_phy_txcal_phy_setup_acphy_core_sd_adc(phy_info_t *pi, uint8 core,
	uint16 sdadc_config);
extern void wlc_phy_poll_samps_acphy(phy_info_t *pi, int16 *samp, bool is_tssi,
	uint8 log2_nsamps, bool init_adc_inside,
	uint16 core);
extern void wlc_phy_populate_tx_loftcoefluts_acphy(phy_info_t *pi, uint8 core, uint16 coeffs,
                                                   uint8 start_idx, uint8 stop_idx);
#ifdef WLC_TXFDIQ
extern void wlc_phy_tx_fdiqi_comp_acphy(phy_info_t *pi, bool enable, int fdiq_data_valid);
#endif
void wlc_phy_cal_txiqlo_init_acphy(phy_info_t *pi);
void phy_ac_txiqlocal_multiphase(phy_info_t *pi, uint8 searchmode, bool Biq2byp, uint16 cts_time,
	uint8 multilo_cal_cnt);
void wlc_phy_cal_txiqlo_acphy(phy_info_t *pi, uint8 searchmode, uint8 mphase, bool Biq2byp,
	uint8 multilo_cal_cnt);
void wlc_phy_txcal_set_cal_params(phy_info_t *pi, uint8 searchmode, uint8 mphase, bool Biq2byp);
void wlc_phy_txcal_coeffs_upd(phy_info_t *pi, txcal_coeffs_t *txcal_cache);
uint8 phy_txiqlocal_num_multilo(phy_info_t *pi);
void wlc_phy_txiqlocal_lopwr_gettblidx(phy_info_t *pi,
	txiqlocal_multilo_t *multilo_cal, uint8 multilo_cal_cnt);
void wlc_phy_txcal_txgain_setup_acphy(phy_info_t *pi, txgain_setting_t *txcal_txgain,
	txgain_setting_t *orig_txgain);
void wlc_phy_txcal_txgain_cleanup_acphy(phy_info_t *pi, txgain_setting_t *orig_txgain);
#ifdef PHYCAL_CACHING
void phy_ac_txiqlocal_save_cache(phy_ac_txiqlocal_info_t *txiqlocali,
                                 ch_calcache_t *ctx, uint8 multilo_cal_cnt);
#else
void wlc_phy_scanroam_cache_txcal_acphy(void *ctx, bool set);
#endif
extern uint16 wlc_acphy_get_tx_locc(phy_info_t *pi, uint8 core);
extern void wlc_acphy_set_tx_locc(phy_info_t *pi, uint16 didq, uint8 core);
#endif /* _phy_ac_txiqlocal_h_ */
