/*
 * ACPHY Rx Spur canceller module implementation
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
 * $Id: phy_ac_rxspur.c 804412 2021-10-29 10:14:26Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <bcm_math.h>
#include "phy_type_rxspur.h"
#include <phy_ac.h>
#include <phy_ac_rxspur.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_phy_shim.h>
#include <wlc_phy_radio.h>
#include <wlc_phytbl_ac.h>
#include <wlc_phyreg_ac.h>

#include <hndpmu.h>
#include <phy_utils_channel.h>
#include <phy_utils_reg.h>
#include <phy_utils_var.h>
#include <phy_rstr.h>

/* add the feature to disable DSSF  0: disable 1: enable */
#define DSSF_ENABLE 1
#define DSSFB_ENABLE 1

/* module private structs */
struct acphy_dssf_values {
	uint16 channel;
	uint8 core;
	uint8 DSSF_gain_th0_s1;
	uint8 DSSF_gain_th1_s1;
	uint8 DSSF_gain_th2_s1;
	uint8 DSSF_gain_th0_s2;
	uint8 DSSF_gain_th1_s2;
	uint8 DSSF_gain_th2_s2;
	uint8 idepth_s1;
	uint8 idepth_s2;
	uint8 enabled_s1;
	uint8 enabled_s2;
	uint16 theta_i_s1;
	uint16 theta_q_s1;
	uint16 theta_i_s2;
	uint16 theta_q_s2;
	uint8 DSSF_C_CTRL;
	bool on;
};

struct acphy_dssfB_values {
	uint16 channel;
	uint8 core;
	uint8 DSSFB_gain_th0_s1;
	uint8 DSSFB_gain_th1_s1;
	uint8 DSSFB_gain_th2_s1;
	uint8 DSSFB_gain_th0_s2;
	uint8 DSSFB_gain_th1_s2;
	uint8 DSSFB_gain_th2_s2;
	uint8 idepth_s1;
	uint8 idepth_s2;
	uint8 enabled_s1;
	uint8 enabled_s2;
	uint16 theta_i_s1;
	uint16 theta_q_s1;
	uint16 theta_i_s2;
	uint16 theta_q_s2;
	uint8 DSSFB_C_CTRL;
	bool on;
};

struct acphy_spurcan_values {
	uint16 channel;
	uint8 core;
	uint8 bw;
	uint8 spurcan_en;
	uint8 s1_en;
	uint16 s1_omega_high;
	uint16 s1_omega_low;
	uint8 s2_en;
	uint16 s2_omega_high;
	uint16 s2_omega_low;
	uint8 s3_en;
	uint16 s3_omega_high;
	uint16 s3_omega_low;
	uint8 s4_en;
	uint16 s4_omega_high;
	uint16 s4_omega_low;
	uint8 s5_en;
	uint16 s5_omega_high;
	uint16 s5_omega_low;
};

/* module private states */
struct phy_ac_rxspur_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_rxspur_info_t *cmn_info;
	acphy_dssf_values_t *dssf_params;
	acphy_dssfB_values_t *dssfB_params;
	acphy_spurcan_values_t *spurcan_params;
	uint16	*spurcan_ChanList;
	int16	*spurcan_SpurFreq;
	uint8	spurcan_NoSpurs;
	uint8	spurcan_CoreMask;
	uint8	acphy_spuravoid_mode;
	uint8	curr_spurmode;
	int8	acphy_spuravoid_mode_override;
};

static const uint32 acphy_spurcan_spur_freqKHz_rev12[] = {2431000, 2468400,
	5198600, 5236000, 5273400, 5310800, 5497800, 5535200, 5572600, 5647400,
	5684800, 5722200, 5759600, 5797000, 5834400};

/* In 53573 DDR freq of 392MHz introduces a spur at 2432MHz */
/* 53573-RSDB mode will have spur at 2457.778MHz when the aggressor core is in Ch-106q */
static const uint32 acphy_spurcan_spur_freqKHz_router_4349_mimo[] = {2432000};
static const uint32 acphy_spurcan_spur_freqKHz_router_4349_rsdb[] = {2432000, 2457778};

// 6710 has Xtal spurs at harmonic of 50 MHz
static const uint32 acphy_spurcan_spur_freqKHz_rev129[] = {2450000,
	5200000, 5300000, 5400000, 5500000, 5600000, 5700000, 5750000, 5800000};

// 6715 has Xtal spurs at harmonic of 50 MHz
// happens spurs @ 5600MHz for BUPC boardstations
static const uint32 acphy_spurcan_spur_freqKHz_rev130[] = {5600000, 6000000,
	6500000, 6666000, 6800000};

// 63178/47622 has Xtal spurs at harmonic of 50 MHz
static const uint32 acphy_spurcan_spur_freqKHz_rev51[] = {2450000,
	5200000, 5300000, 5400000, 5500000, 5600000, 5700000, 5750000, 5800000};

// 6756/68550 has Xtal spurs at harmonic of 50 MHz
static const uint32 acphy_spurcan_spur_freqKHz_rev131[] = {2450000,
	5200000, 5300000, 5400000, 5500000, 5550000, 5600000, 5700000, 5750000, 5800000};

// 6878 has an Xtal spur on 2450 MHz, 49th harmonic of 50 MHz
// also seeing 25 MHz spurs so making seperate entry for 6878
// 5GHz spurs also need to be cancelled out because they cause PER floor with LESI
static const uint32 acphy_spurcan_spur_freqKHz_rev128[] = {2425000, 2450000, 2475000,
	5200000, 5300000, 5400000, 5500000, 5600000, 5700000, 5750000, 5760000, 5800000, 5754350};

/* local functions */
static int phy_ac_rxspur_std_params(phy_ac_rxspur_info_t *info);
static void phy_ac_set_spurmode(phy_type_rxspur_ctx_t *ctx, uint16 freq);
static void phy_ac_get_spurmode(phy_ac_rxspur_info_t *rxspuri, uint16 freq);
#if defined(WLTEST)
static int phy_ac_rxspur_set_force_spurmode(phy_type_rxspur_ctx_t *ctx, int16 int_val);
static int phy_ac_rxspur_get_force_spurmode(phy_type_rxspur_ctx_t *ctx, int32 *ret_int_ptr);
#endif /* WLTEST */
static void phy_ac_set_dssf_freq(phy_info_t *pi, uint8 stage, uint8 core, int32 f_kHz,
	uint8 phy_bw);

/* register phy type specific implementation */
phy_ac_rxspur_info_t *
BCMATTACHFN(phy_ac_rxspur_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_rxspur_info_t *cmn_info)
{
	phy_ac_rxspur_info_t *ac_info;
	phy_type_rxspur_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ac_info = phy_malloc(pi, sizeof(phy_ac_rxspur_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	ac_info->pi = pi;
	ac_info->aci = aci;
	ac_info->cmn_info = cmn_info;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.set_spurmode = phy_ac_set_spurmode;
#if defined(WLTEST)
	fns.set_force_spurmode = phy_ac_rxspur_set_force_spurmode;
	fns.get_force_spurmode  = phy_ac_rxspur_get_force_spurmode;
#endif /* WLTEST */
	fns.ctx = ac_info;

	if ((ac_info->dssf_params = phy_malloc(pi, sizeof(acphy_dssf_values_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc dssf_params failed\n", __FUNCTION__));
		goto fail;
	}
	if ((ac_info->dssfB_params = phy_malloc(pi, sizeof(acphy_dssfB_values_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc dssfB_params failed\n", __FUNCTION__));
		goto fail;
	}
	if ((ac_info->spurcan_params = phy_malloc(pi, sizeof(acphy_spurcan_values_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc spurcan_params failed\n", __FUNCTION__));
		goto fail;
	}

	if (phy_ac_rxspur_std_params(ac_info) != BCME_OK) {
		PHY_ERROR(("%s: phy_ac_rxspur_std_params failed\n", __FUNCTION__));
		goto fail;
	}

	if (phy_rxspur_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_rxspur_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ac_info;

	/* error handling */
fail:
	phy_ac_rxspur_unregister_impl(ac_info);
	return NULL;
}

void
BCMATTACHFN(phy_ac_rxspur_unregister_impl)(phy_ac_rxspur_info_t *ac_info)
{
	phy_info_t *pi;
	phy_rxspur_info_t *cmn_info;

	if (ac_info == NULL) {
		return;
	}

	pi = ac_info->pi;
	cmn_info = ac_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_rxspur_unregister_impl(cmn_info);

	if (ac_info->spurcan_params != NULL) {
		phy_mfree(pi, ac_info->spurcan_params, sizeof(acphy_spurcan_values_t));
	}
	if (ac_info->dssfB_params != NULL) {
		phy_mfree(pi, ac_info->dssfB_params, sizeof(acphy_dssfB_values_t));
	}
	if (ac_info->dssf_params != NULL) {
		phy_mfree(pi, ac_info->dssf_params, sizeof(acphy_dssf_values_t));
	}

	phy_mfree(pi, ac_info, sizeof(phy_ac_rxspur_info_t));
}

static int
BCMATTACHFN(phy_ac_rxspur_std_params)(phy_ac_rxspur_info_t *rxspuri)
{
	phy_info_t *pi = rxspuri->pi;
	//phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	uint8 core;
	rxspuri->curr_spurmode = 0;
	rxspuri->acphy_spuravoid_mode_override = 0;

	if (ACMAJORREV_51_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) {
		rxspuri->spurcan_CoreMask = (uint8)PHY_GETINTVAR_DEFAULT(pi,
				rstr_spurcan_coremask, 0);
	}

	// Setting the coremask based on the available cores.
	// Spurcan will be enabled for all available cores.
	if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
		rxspuri->spurcan_CoreMask = 0;
		FOREACH_CORE(pi, core) {
			rxspuri->spurcan_CoreMask += 1<<core;
		}
	}

	// Setting the coremask based on the available cores.
	// Spurcan will be enabled for all available cores.
	if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
		if (ISSIM_ENAB(pi->sh->sih)) {
			rxspuri->spurcan_CoreMask = 0xf;
		} else {
			rxspuri->spurcan_CoreMask = 0;
			FOREACH_CORE(pi, core) {
				rxspuri->spurcan_CoreMask += 1<<core;
			}
		}
	}

	return BCME_OK;
}
/* ******************************************** */
/*		Internal Definitions		*/
/* ******************************************** */

static void
phy_ac_setup_spurmode(phy_ac_rxspur_info_t *rxspuri)
{
	phy_info_t *pi = rxspuri->pi;

	si_pmu_spuravoid(pi->sh->sih, pi->sh->osh, rxspuri->acphy_spuravoid_mode);
	wlapi_switch_macfreq(pi->sh->physhim, rxspuri->acphy_spuravoid_mode);
}

static void phy_ac_spurcan_clk(phy_info_t *pi, uint8 core, bool enable)
{
	MOD_PHYREGCE(pi, forceFront, core, spurcan_clk_en_slms0, enable);
	if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
		if (core > 0) {
			MOD_PHYREG(pi, forceFront1, spurcan_clk_en_slmsx, enable);
		} else {
			MOD_PHYREG(pi, forceFront0, spurcan_clk_en_slmsx, enable);
		}
	} else {
		MOD_PHYREGCE(pi, forceFront, core, spurcan_clk_en_slms1, enable);
	}
}

static void
phy_ac_spurcan_setup(phy_ac_rxspur_info_t *rxspuri, bool enable)
{
	phy_info_t *pi = rxspuri->pi;
	acphy_spurcan_values_t *spurcan = rxspuri->spurcan_params;
	uint8 core = spurcan->core;

	if (enable) {
		MOD_PHYREG(pi, spur_can_phy_bw_mhz, spur_can_phy_bw_mhz, spurcan->bw);
		if (ROUTER_4349(pi)) {
			MOD_PHYREG(pi, spur_can_P_sp_min, spur_can_P_sp_min_dbm_slms, 0x8f);
		}
		MOD_PHYREGCM(pi, spur_can_p, s1_en, core, spur_can_kf_enable, 0);
		MOD_PHYREGCM(pi, spur_can_p, s1_omega_high, core,
			spur_can_omega_high, spurcan->s1_omega_high);
		MOD_PHYREGCM(pi, spur_can_p, s1_omega_low, core,
			spur_can_omega_low, spurcan->s1_omega_low);
		MOD_PHYREGCM(pi, spur_can_p, s1_en, core,
			spur_can_stage_enable, spurcan->s1_en);
		MOD_PHYREGCM(pi, spur_can_p, s2_omega_high, core,
			spur_can_omega_high, spurcan->s2_omega_high);
		MOD_PHYREGCM(pi, spur_can_p, s2_omega_low, core,
			spur_can_omega_low, spurcan->s2_omega_low);
		MOD_PHYREGCM(pi, spur_can_p, s2_en, core,
			spur_can_stage_enable, spurcan->s2_en);
		MOD_PHYREGCM(pi, spur_can_p, s3_omega_high, core,
			spur_can_omega_high, spurcan->s3_omega_high);
		MOD_PHYREGCM(pi, spur_can_p, s3_omega_low, core,
			spur_can_omega_low, spurcan->s3_omega_low);
		MOD_PHYREGCM(pi, spur_can_p, s3_en, core,
			spur_can_stage_enable, spurcan->s3_en);
		MOD_PHYREGCM(pi, spur_can_p, s4_omega_high, core,
			spur_can_omega_high, spurcan->s4_omega_high);
		MOD_PHYREGCM(pi, spur_can_p, s4_omega_low, core,
			spur_can_omega_low, spurcan->s4_omega_low);
		MOD_PHYREGCM(pi, spur_can_p, s4_en, core,
			spur_can_stage_enable, spurcan->s4_en);
		if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			if (core > 0) {
				MOD_PHYREG(pi, spur_can_p1_s5_omega_high,
					spur_can_omega_high, spurcan->s5_omega_high);
				MOD_PHYREG(pi, spur_can_p1_s5_omega_low,
					spur_can_omega_low, spurcan->s5_omega_low);
				MOD_PHYREG(pi, spur_can_fll_enable_p1,
					spur_can_stage_enable, spurcan->s5_en);
			} else {
				MOD_PHYREG(pi, spur_can_p0_s5_omega_high,
					spur_can_omega_high, spurcan->s5_omega_high);
				MOD_PHYREG(pi, spur_can_p0_s5_omega_low,
					spur_can_omega_low, spurcan->s5_omega_low);
				MOD_PHYREG(pi, spur_can_fll_enable_p0,
					spur_can_stage_enable, spurcan->s5_en);
			}
		}
		MOD_PHYREG(pi, spur_can_en, spur_can_enable,
			spurcan->spurcan_en);
	} else {
		MOD_PHYREG(pi, spur_can_en, spur_can_enable, 0);
		MOD_PHYREGCM(pi, spur_can_p, s1_en, core, spur_can_stage_enable, 0);
		MOD_PHYREGCM(pi, spur_can_p, s2_en, core, spur_can_stage_enable, 0);
		MOD_PHYREGCM(pi, spur_can_p, s3_en, core, spur_can_stage_enable, 0);
		if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			MOD_PHYREGCM(pi, spur_can_p, s4_en, core, spur_can_stage_enable, 0);
			if (core > 0) {
				MOD_PHYREG(pi, spur_can_fll_enable_p1,
					spur_can_stage_enable, 0);
			} else {
				MOD_PHYREG(pi, spur_can_fll_enable_p0,
					spur_can_stage_enable, 0);
			}
		}
	}
}

static void
phy_ac_fll_setup(phy_ac_rxspur_info_t *rxspuri, bool enable)
{
	phy_info_t *pi = rxspuri->pi;
	acphy_spurcan_values_t *spurcan = rxspuri->spurcan_params;
	uint8 core = spurcan->core;

	if (enable) {
		/* Below 3 regs are the minimum settings to enable the FLL section */
		MOD_PHYREGCE(pi, forceFront, core, spurcan_clk_en_fll, 1);
		MOD_PHYREG(pi, spur_can_en, spur_can_enable, 1);
		MOD_PHYREGCE(pi, spur_can_fll_enable_p, core, fll_enable, 1);

		/* Fast acquisition mode disables the LMS adaptation to make FLL fully converge */
		MOD_PHYREGCE(pi, spur_can_fll_enable_p, core, fast_acq_mode, 0);
		/* De-noising, i.e. update omega only if the update term is strong */
		MOD_PHYREGCE(pi, spur_can_fll_enable_p, core, denoising_en, 0);
		/* PI controller to stabilize the loop, passes through P leg if disabled */
		MOD_PHYREGCE(pi, spur_can_fll_enable_p, core, pi_en, 0);
		MOD_PHYREG(pi, spur_can_fll_ctrl3, pi_Kp, 4);
		MOD_PHYREG(pi, spur_can_fll_ctrl3, pi_Ki, 1);

		ACPHY_REG_LIST_START
		  /* Damping amount in omega corrections
			* d_omega = (d_phi >> (damping - log2f_mu))
			*/
		  MOD_PHYREG_ENTRY(pi, spur_can_fll_ctrl1, damping, 16)
		  /* Further heavy LPF (1st IIR) on omega, not used by HW
			* FW can utilize this if needed
			*/
		  MOD_PHYREG_ENTRY(pi, spur_can_fll_ctrl3, omega_lpf_gamma, 10)
		  /* -log2(alpha) of the 1st order IIR for LPFing of down-converted spur
		       * (y[n] = (1-alpha)*y[n-1] + alpha*x[n])
		       */
		  MOD_PHYREG_ENTRY(pi, spur_can_fll_ctrl3, eex1_lpf_alpha, 3)
		  MOD_PHYREG_ENTRY(pi, spur_can_fll_ctrl1, dphi_lpf_beta, 0)
		  /* Some other settings */
		  MOD_PHYREG_ENTRY(pi, spur_can_fll_ctrl1, mu_threshold, 116)
		  MOD_PHYREG_ENTRY(pi, spur_can_fll_ctrl2, lsb_trun_en, 1)
		  MOD_PHYREG_ENTRY(pi, spur_can_fll_ctrl2, boost_skip_count, 24)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else {
		MOD_PHYREGCE(pi, forceFront, core, spurcan_clk_en_fll, 0);
		MOD_PHYREG(pi, spur_can_en, spur_can_enable, 0);
		MOD_PHYREGCE(pi, spur_can_fll_enable_p, core, fll_enable, 0);
	}
}

#ifndef WL_DSSF_DISABLED
static void
phy_ac_dssf_setup(phy_ac_rxspur_info_t *rxspuri)
{
	phy_info_t *pi = rxspuri->pi;
	acphy_dssf_values_t *dssf = rxspuri->dssf_params;

	WRITE_PHYREG(pi, DSSF_C_CTRL, dssf->DSSF_C_CTRL);
	WRITE_PHYREGCE(pi, DSSF_gain_th0_s1, dssf->core, dssf->DSSF_gain_th0_s1);
	WRITE_PHYREGCE(pi, DSSF_gain_th1_s1, dssf->core, dssf->DSSF_gain_th1_s1);
	WRITE_PHYREGCE(pi, DSSF_gain_th2_s1, dssf->core, dssf->DSSF_gain_th2_s1);
	WRITE_PHYREGCE(pi, DSSF_gain_th0_s2, dssf->core, dssf-> DSSF_gain_th0_s2);
	WRITE_PHYREGCE(pi, DSSF_gain_th1_s2, dssf->core, dssf->DSSF_gain_th1_s2);
	WRITE_PHYREGCE(pi, DSSF_gain_th2_s2, dssf->core, dssf->DSSF_gain_th2_s2);
	WRITE_PHYREGCE(pi, DSSF_exp_j_theta_i_s1, dssf->core, dssf->theta_i_s1);
	WRITE_PHYREGCE(pi, DSSF_exp_i_theta_q_s1, dssf->core, dssf->theta_q_s1);
	WRITE_PHYREGCE(pi, DSSF_exp_j_theta_i_s2, dssf->core, dssf->theta_i_s2);
	WRITE_PHYREGCE(pi, DSSF_exp_i_theta_q_s2, dssf->core, dssf->theta_q_s2);

	MOD_PHYREGCE(pi, DSSF_control_, dssf->core, idepth_s1, dssf->idepth_s1);
	MOD_PHYREGCE(pi, DSSF_control_, dssf->core, idepth_s2, dssf->idepth_s2);

	MOD_PHYREGCE(pi, DSSF_control_, dssf->core, enabled_s1, dssf->enabled_s1);
	MOD_PHYREGCE(pi, DSSF_control_, dssf->core, enabled_s2, dssf->enabled_s2);
}
#endif /* WL_DSSF_DISABLED */

static void
phy_ac_dssfB_setup(phy_ac_rxspur_info_t *rxspuri)
{
	phy_info_t *pi = rxspuri->pi;
	acphy_dssfB_values_t *dssfB = rxspuri->dssfB_params;

	WRITE_PHYREG(pi, DSSFB_C_CTRL, dssfB->DSSFB_C_CTRL);
	WRITE_PHYREG(pi, DSSFB_gain_th0_s1,      dssfB->DSSFB_gain_th0_s1);
	WRITE_PHYREG(pi, DSSFB_gain_th1_s1,      dssfB->DSSFB_gain_th1_s1);
	WRITE_PHYREG(pi, DSSFB_gain_th2_s1,      dssfB->DSSFB_gain_th2_s1);
	WRITE_PHYREG(pi, DSSFB_gain_th0_s2,      dssfB->DSSFB_gain_th0_s2);
	WRITE_PHYREG(pi, DSSFB_gain_th1_s2,      dssfB->DSSFB_gain_th1_s2);
	WRITE_PHYREG(pi, DSSFB_gain_th2_s2,      dssfB->DSSFB_gain_th2_s2);
	WRITE_PHYREG(pi, DSSFB_exp_j_theta_i_s1, dssfB->theta_i_s1);
	WRITE_PHYREG(pi, DSSFB_exp_i_theta_q_s1, dssfB->theta_q_s1);
	WRITE_PHYREG(pi, DSSFB_exp_j_theta_i_s2, dssfB->theta_i_s2);
	WRITE_PHYREG(pi, DSSFB_exp_i_theta_q_s2, dssfB->theta_q_s2);

	MOD_PHYREG(pi, DSSFB_control, idepth_s1,  dssfB->idepth_s1);
	MOD_PHYREG(pi, DSSFB_control, idepth_s2,  dssfB->idepth_s2);
	MOD_PHYREG(pi, DSSFB_control, enabled_s1, dssfB->enabled_s1);
	MOD_PHYREG(pi, DSSFB_control, enabled_s2, dssfB->enabled_s2);
}

static bool
chanspec_bbpll_parr_enable(phy_info_t *pi)
{
	return FALSE;
}

/* ******************************************** */
/*		External Definitions		*/
/* ******************************************** */

void
chanspec_bbpll_parr(phy_ac_rxspur_info_t *rxspuri, uint32 *bbpll_parr_in, bool state)
{
	/* input : bbpll_parr_in
	 * 0 : min_res_mask
	 * 1 : max_res_mask
	 * 2 : clk_ctl_st
	 */

#ifdef BBPLL_PARR
	phy_info_t *pi = rxspuri->pi;
	uint32 min_res_mask = 0, max_res_mask = 0, clk_ctl_st = 0;
#endif

	if (!chanspec_bbpll_parr_enable(rxspuri->pi))
		return;

#ifdef BBPLL_PARR
	min_res_mask = bbpll_parr_in[0];
	max_res_mask = bbpll_parr_in[1];
	clk_ctl_st = bbpll_parr_in[2];

	if (state == OFF) {
		/* power down BBPLL */
		phy_ac_get_spurmode(rxspuri,
			(uint16) phy_ac_radio_get_data(rxspuri->aci->radioi)->fc);
		if ((rxspuri->curr_spurmode != rxspuri->acphy_spuravoid_mode)) {
			si_pmu_pll_off_PARR(pi->sh->sih, pi->sh->osh,
				&min_res_mask, &max_res_mask, &clk_ctl_st);
		}
	} else {
		/* update and power up BBPLL */
		if (rxspuri->curr_spurmode != rxspuri->acphy_spuravoid_mode) {
			rxspuri->curr_spurmode =  rxspuri->acphy_spuravoid_mode;
			si_pmu_spuravoid_isdone(pi->sh->sih, pi->sh->osh, min_res_mask,
				max_res_mask, clk_ctl_st, rxspuri->acphy_spuravoid_mode);
			wlapi_switch_macfreq(pi->sh->physhim, rxspuri->acphy_spuravoid_mode);
		}
	}

	bbpll_parr_in[0] = min_res_mask;
	bbpll_parr_in[1] = max_res_mask;
	bbpll_parr_in[2] = clk_ctl_st;
#endif /* BBPLL_PARR */
	return;
}

void
phy_ac_spurcan(phy_ac_rxspur_info_t *rxspuri, bool enable)
{
	phy_info_t *pi = rxspuri->pi;
	phy_info_acphy_t *pi_ac = rxspuri->aci;
	acphy_spurcan_values_t *spurcan = rxspuri->spurcan_params;
	uint8 i, j, core;
	uint8 num_spurs = 0;
	uint8 spur_1st_stage = 2;	/* Use stage2 and stage3 */
	uint8 num_slms = 0, num_fll = 0;
	uint8 tbl_len = 0;
	uint8 tbl_len_vco = 0;
	uint16 channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	uint32 fc_KHz, omega;
	int bw = CHSPEC_BW_LE20(pi->radio_chanspec) ?
		20000 : CHSPEC_IS40(pi->radio_chanspec) ?
		40000 : CHSPEC_IS80(pi->radio_chanspec) ?
		80000 : 160000;
	int freq;
	int fsp, sign;
	const uint32 *spurfreq = NULL;
	uint32 spurfreq_vco[2];
	uint32 omega_frac_adj = 0;

	freq = wf_channel2mhz(channel, CHSPEC_ISPHY5G6G(pi->radio_chanspec) ?
		(CHSPEC_IS5G(pi->radio_chanspec) ? WF_CHAN_FACTOR_5_G :
		WF_CHAN_FACTOR_6_G) : WF_CHAN_FACTOR_2_4_G);
	fc_KHz = freq*1000;

	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		if (ROUTER_4349(pi)) {
			if (phy_get_phymode(pi) == PHYMODE_RSDB) {
				spurfreq = acphy_spurcan_spur_freqKHz_router_4349_rsdb;
				tbl_len = ARRAYSIZE(acphy_spurcan_spur_freqKHz_router_4349_rsdb);
			} else if (phy_get_phymode(pi) == PHYMODE_MIMO) {
				spurfreq = acphy_spurcan_spur_freqKHz_router_4349_mimo;
				tbl_len = ARRAYSIZE(acphy_spurcan_spur_freqKHz_router_4349_mimo);
			}
		} else {
			spurfreq = acphy_spurcan_spur_freqKHz_rev12;
			tbl_len = ARRAYSIZE(acphy_spurcan_spur_freqKHz_rev12);
		}
		num_slms = 2;
		num_fll = 1;

		/* Make default to 0x6E 110 later change the value if there is spur */
		MOD_PHYREG(pi, overideDigiGain1, cckdigigainEnCntValue, 0x6E);

	} else if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
		if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			spurfreq = acphy_spurcan_spur_freqKHz_rev131;
			tbl_len = ARRAYSIZE(acphy_spurcan_spur_freqKHz_rev131);
			num_slms = 4;
			spur_1st_stage = 1; /* No Kalman at s1 */
		} else {
			spurfreq = acphy_spurcan_spur_freqKHz_rev51;
			tbl_len = ARRAYSIZE(acphy_spurcan_spur_freqKHz_rev51);
			num_slms = 2;
		}
		num_fll = 1;
		/* additonal vco spurs (fvco - n*100MHz) found in 2GHz band */
		if (freq < 3000 && bw == 20000) {
			if (freq == 2412) {
				spurfreq_vco[tbl_len_vco] = 2418000;
			} else if (freq == 2417) {
				spurfreq_vco[tbl_len_vco] = 2425500;
			} else if (freq == 2427) {
				spurfreq_vco[tbl_len_vco] = 2436000;
			}
			tbl_len_vco += 1;
		} else if (freq < 3000 && bw == 40000) {
			if (freq == 2422) {
				spurfreq_vco[tbl_len_vco] = 2433000;
			} else if (freq == 2427) {
				spurfreq_vco[tbl_len_vco] = 2436000;
			} else if (freq == 2432) {
				spurfreq_vco[tbl_len_vco] = 2442666;
				omega_frac_adj = 0x14444;
			} else if (freq == 2437) {
				spurfreq_vco[tbl_len_vco] = 2455500;
			} else if (freq == 2447) {
				spurfreq_vco[tbl_len_vco] = 2462666;
				omega_frac_adj = 0x14444;
			} else if (freq == 2452) {
				spurfreq_vco[tbl_len_vco] = 2469333;
				omega_frac_adj = 0xeeee;
			} else if (freq == 2457) {
				spurfreq_vco[tbl_len_vco] = 2476000;
			}
			tbl_len_vco += 1;
		}
	} else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		spurfreq = acphy_spurcan_spur_freqKHz_rev128;
		tbl_len = ARRAYSIZE(acphy_spurcan_spur_freqKHz_rev128);
		num_slms = 1;
		num_fll = 1;
	} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
		spurfreq = acphy_spurcan_spur_freqKHz_rev129;
		tbl_len = ARRAYSIZE(acphy_spurcan_spur_freqKHz_rev129);
		num_slms = 2;
		num_fll = 1;
	} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
		num_slms = 2;
		num_fll = 1;
	}

	ASSERT(tbl_len_vco <= ARRAYSIZE(spurfreq_vco));
	if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
		phy_ac_spurcan_config_priority(rxspuri, fc_KHz, num_slms, num_fll, enable);
	} else {
		if (spurfreq != NULL || tbl_len_vco > 0) {
			FOREACH_CORE(pi, core) {
				bzero(spurcan, sizeof(acphy_spurcan_values_t));
				spurcan->channel = channel;
				spurcan->core = core;
				spurcan->bw = bw/1000;
				num_spurs = 0;
				for (i = 0; i < (tbl_len + tbl_len_vco); i++) {
					// Next statement could be outside loop but this way code
					// is more compact
					if (!enable || ((rxspuri->spurcan_CoreMask &
						(1 << core)) == 0)) {
						break;
					}
					/* handle xtal spurs first followed by vco spurs */
					if (i < tbl_len) {
						fsp = spurfreq[i] - fc_KHz;
					} else {
						/* using j just to pass precommit */
						j = (i >= tbl_len) ? i - tbl_len : 0;
						fsp = spurfreq_vco[j] - fc_KHz;
					}

					/* JIRA: SWWLAN-92418
					 * Issue: Enabling spurcan at DC freq is degrading
					 * rx sensitivity.
					 * Fix: Don't enable spurcan if spur is at DC freq.
					 */
					if (fsp == 0) {
						continue;
					}

					if (((-bw/2) < fsp) && (fsp < (bw/2))) {
						spurcan->spurcan_en = 1;

						sign = (fsp > 0) - (fsp < 0);
						math_uint64_divide(&omega, sign * fsp, 0, bw);
						omega = sign * omega;
						/* correct for fractional part in vco spur freq */
						if  ((i >= tbl_len) && (omega_frac_adj != 0)) {
							omega = omega & 0xffff0000;
							omega = omega + omega_frac_adj;
						}
						PHY_CAL(("%s: spur @ %d kHz, omega = %d\n",
							__FUNCTION__, fsp, omega));
						if ((spur_1st_stage + num_spurs) == 1) {
							spurcan->s1_en = 1;
							spurcan->s1_omega_high = omega >> 16;
							spurcan->s1_omega_low = omega & 0x0000ffff;
						} else if (((spur_1st_stage + num_spurs) == 2) &&
							(num_slms > (0 + (2 - spur_1st_stage)))) {
							spurcan->s2_en = 1;
							spurcan->s2_omega_high = omega >> 16;
							spurcan->s2_omega_low = omega & 0x0000ffff;
						} else if (((spur_1st_stage + num_spurs) == 3) &&
							(num_slms > (1 + (2 - spur_1st_stage)))) {
							spurcan->s3_en = 1;
							spurcan->s3_omega_high = omega >> 16;
							spurcan->s3_omega_low = omega & 0x0000ffff;
						} else if (((spur_1st_stage + num_spurs) == 4) &&
							(num_slms > (2 + (2 - spur_1st_stage)))) {
							spurcan->s4_en = 1;
							spurcan->s4_omega_high = omega >> 16;
							spurcan->s4_omega_low = omega & 0x0000ffff;
						/* if no more slms go to spur can stage 4 (FLL) */
						} else if ((((spur_1st_stage + num_spurs) == 3) ||
							((spur_1st_stage + num_spurs) == 4)) &&
							(num_fll > 0)) {
							spurcan->s4_en = 1;
							spurcan->s4_omega_high = omega >> 16;
							spurcan->s4_omega_low = omega & 0x0000ffff;
						/* chips with 4 slms use stage 5 for FLL */
						} else if (((spur_1st_stage + num_spurs) == 5) &&
							(num_fll > 0)) {
							spurcan->s5_en = 1;
							spurcan->s5_omega_high = omega >> 16;
							spurcan->s5_omega_low = omega & 0x0000ffff;
						} else {
							PHY_ERROR(("wl%d: %s: No more spur"
								"cancellers\n", pi->sh->unit,
								__FUNCTION__));
							ASSERT(0);
						}

						if ((++num_spurs) >= (num_slms + num_fll)) {
							break;
						}

						if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
							MOD_PHYREG(pi, overideDigiGain1,
								cckdigigainEnCntValue, 0x82);
						}
					}
				}
				PHY_CAL(("%s: channel %d, core %d, bw = %d MHz",
					__FUNCTION__, channel, core, spurcan->bw));
				if (spurcan->spurcan_en) {
					PHY_CAL((" ==> enable %d cancellers\n", num_spurs));
				} else {
					PHY_CAL((" ==> disable cancellers\n"));
				}
				phy_ac_spurcan_setup(pi_ac->rxspuri, spurcan->spurcan_en);
				phy_ac_spurcan_clk(pi_ac->rxspuri->pi, core, spurcan->spurcan_en);
				if ((spurcan->s4_en && (num_slms < 4)) || spurcan->s5_en) {
					phy_ac_fll_setup(pi_ac->rxspuri, spurcan->spurcan_en);
				}
			}
		} else {
			// spurfreq is NULL
			FOREACH_CORE(pi, core) {
				bzero(spurcan, sizeof(acphy_spurcan_values_t));
				spurcan->channel = channel;
				spurcan->core = core;
				spurcan->bw = bw/1000;
				phy_ac_spurcan_setup(pi_ac->rxspuri, FALSE);
				phy_ac_spurcan_clk(pi_ac->rxspuri->pi, core, FALSE);
			}
		}
	}
}

void
phy_ac_spurcan_config_priority(phy_ac_rxspur_info_t *rxspuri,
	uint32 fc_KHz, uint8 num_slms, uint8 num_fll, bool enable)
{
	phy_info_t *pi = rxspuri->pi;
	phy_info_acphy_t *pi_ac = rxspuri->aci;
	acphy_spurcan_values_t *spurcan = rxspuri->spurcan_params;

	bool slms_en;
	uint8 tbl_len = 0;
	//uint8 num_slms = 2; /* Use stage2 and stage3 for slms */
	//uint8 num_fll = 1;
	uint8 stage = 0;
	uint8 total_num_stage;
	uint8 stage_priority[] = {4, 2, 3}; /* To make FLL as the 1st priority */
	uint8 num_spurs;
	uint8 i, core;
	uint16 channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	uint32 omega;
	int fsp, sign;
	int bw = CHSPEC_BW_LE20(pi->radio_chanspec) ?
		20000 : CHSPEC_IS40(pi->radio_chanspec) ?
		40000 : CHSPEC_IS80(pi->radio_chanspec) ?
		80000 : 160000;
	const uint32 *spurfreq = NULL;

	total_num_stage = num_slms + num_fll;
	if  (total_num_stage != ARRAYSIZE(stage_priority)) {
		PHY_ERROR(("wl%d: %s: The number of spur cancellers is not aligned\n",
				pi->sh->unit, __FUNCTION__));
		ASSERT(0);
	}
	spurfreq = acphy_spurcan_spur_freqKHz_rev130;
	tbl_len = ARRAYSIZE(acphy_spurcan_spur_freqKHz_rev130);

	if (spurfreq != NULL) {
		FOREACH_CORE(pi, core) {
			bzero(spurcan, sizeof(acphy_spurcan_values_t));
			spurcan->channel = channel;
			spurcan->core = core;
			spurcan->bw = bw/1000;

			num_spurs = 0;
			for (i = 0; i < tbl_len; i++) {
				if (!enable || ((rxspuri->spurcan_CoreMask & (1 << core)) == 0)) {
					break;
				}

				if (i < tbl_len) {
					fsp = spurfreq[i] - fc_KHz;
				}

				/* JIRA: SWWLAN-92418
				 * Issue: Enabling spurcan at DC freq is degrading rx sensitivity.
				 * Fix: Don't enable spurcan if spur is at DC freq.
				 */
				if (fsp == 0) {
					continue;
				}

				if (((-bw/2) < fsp) && (fsp < (bw/2))) {
					spurcan->spurcan_en = 1;

					sign = (fsp > 0) - (fsp < 0);
					math_uint64_divide(&omega, sign * fsp, 0, bw);
					omega = sign * omega;
					PHY_CAL(("%s: spur @ %d kHz, omega = %d\n",
						__FUNCTION__, fsp, omega));

					/* Unlike other chips,
					 * for 6715, spur canceller is operated by stage priority
					 * To make FLL as the 1st priority
					 *	stage = 4 for fll, 2 & 3 for slms
					 */
					stage = stage_priority[num_spurs];
					/* No Kalman at s1 */
					if (stage == 1) {
						spurcan->s1_en = 1;
						spurcan->s1_omega_high = omega >> 16;
						spurcan->s1_omega_low = omega & 0x0000ffff;
					} else if (stage == 2) {
						spurcan->s2_en = 1;
						spurcan->s2_omega_high = omega >> 16;
						spurcan->s2_omega_low = omega & 0x0000ffff;
					} else if (stage == 3) {
						spurcan->s3_en = 1;
						spurcan->s3_omega_high = omega >> 16;
						spurcan->s3_omega_low = omega & 0x0000ffff;
					/* if no more slms go to spur can stage 4 (FLL) */
					/* When num_slms = 2, num_fll = 1 */
					} else if ((stage == 4) && (num_fll > 0) &&
							(num_slms == 2)) {
						spurcan->s4_en = 1;
						spurcan->s4_omega_high = omega >> 16;
						spurcan->s4_omega_low = omega & 0x0000ffff;
					/* if no more slms go to spur can stage 5 (FLL) */
					/* When num_slms = 3, num_fll = 1 */
					} else if ((stage == 5) && (num_fll > 0) &&
							(num_slms == 3)) {
						spurcan->s5_en = 1;
						spurcan->s5_omega_high = omega >> 16;
						spurcan->s5_omega_low = omega & 0x0000ffff;
					} else {
						PHY_ERROR(("wl%d: %s: No more spur cancellers\n",
							pi->sh->unit, __FUNCTION__));
						ASSERT(0);
					}

					if ((++num_spurs) >= (total_num_stage)) {
						break;
					}
				}
			}
			/* In case of multiple spurs at current chan,
			 * slms_en will be TRUE to make spurcan_clk_en_slms0/1 1
			 * FLL stage = 4 if SLMS stages = {2, 3}
			 * FLL stage = 5 if SLMS stages = {2, 3, 4}
			 */
			if (num_spurs <= 1) {
				if ((num_spurs == 0) || (stage == 4) || (stage == 5)) {
					slms_en = FALSE;
				} else {
					slms_en = TRUE;
				}
			} else {
				// if multiple spurs, requires at least one slms on
				slms_en = TRUE;
			}

			PHY_CAL(("%s: channel %d, core %d, bw = %d MHz",
				__FUNCTION__, channel, core, spurcan->bw));
			if (spurcan->spurcan_en) {
				PHY_CAL((" ==> enable %d cancellers\n", num_spurs));
			} else {
				PHY_CAL((" ==> disable cancellers\n"));
			}
			phy_ac_spurcan_setup(pi_ac->rxspuri, spurcan->spurcan_en);
			phy_ac_spurcan_clk(pi_ac->rxspuri->pi, core, slms_en);
			if ((spurcan->s4_en && (num_slms < 4)) || spurcan->s5_en) {
				phy_ac_fll_setup(pi_ac->rxspuri, spurcan->spurcan_en);
			}
		}
	} else {
		// spurfreq is NULL
		FOREACH_CORE(pi, core) {
			bzero(spurcan, sizeof(acphy_spurcan_values_t));
			spurcan->channel = channel;
			spurcan->core = core;
			spurcan->bw = bw/1000;
			phy_ac_spurcan_setup(pi_ac->rxspuri, FALSE);
			phy_ac_spurcan_clk(pi_ac->rxspuri->pi, core, FALSE);
		}
	}
}

void
phy_ac_dssf(phy_ac_rxspur_info_t *rxspuri, bool on)
{
#ifndef WL_DSSF_DISABLED
	phy_info_t *pi = rxspuri->pi;
	phy_info_acphy_t *pi_ac = rxspuri->aci;
	acphy_dssf_values_t *dssf = rxspuri->dssf_params;

	if (ACMAJORREV_47(pi->pubpi->phy_rev) && !pi->sromi->dssf_dis_ch138) {
		phy_ac_dssf_43684(rxspuri, TRUE);
		return;
	}

	if (!ACMAJORREV_128(pi->pubpi->phy_rev)) {
		return;
	}

	if (ACMAJORREV_4(pi->pubpi->phy_rev) ||
		ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		(ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev))) {
		return;
	}

	if (ACMAJORREV_128(pi->pubpi->phy_rev) &&
		!(CHSPEC_IS2G(pi->radio_chanspec) && BF2_2G_SPUR_WAR(pi_ac))) {
		return;
	}

	dssf->channel = 0;
	dssf->core = 0;

	if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		dssf->DSSF_gain_th0_s1 = 0;
		dssf->DSSF_gain_th1_s1 = 0;
		dssf->DSSF_gain_th2_s1 = 0;
		dssf->DSSF_gain_th0_s2 = 0;
		dssf->DSSF_gain_th1_s2 = 0;
		dssf->DSSF_gain_th2_s2 = 0;
	} else {
		dssf->DSSF_gain_th0_s1 = 68;
		dssf->DSSF_gain_th1_s1 = 74;
		dssf->DSSF_gain_th2_s1 = 80;
		dssf->DSSF_gain_th0_s2 = 68;
		dssf->DSSF_gain_th1_s2 = 74;
		dssf->DSSF_gain_th2_s2 = 80;
	}

	dssf->idepth_s1 = 2;
	dssf->idepth_s2 = 2;
	dssf->enabled_s1 = 1;
	dssf->enabled_s2 = 0;
	dssf->theta_i_s1 = 0;
	dssf->theta_q_s1 = 0;
	dssf->theta_i_s2 = 0;
	dssf->theta_q_s2 = 0;
	dssf->DSSF_C_CTRL = 0;
	dssf->on = on;

	wlapi_bmac_phyclk_fgc(pi->sh->physhim, ON);

	/* Reset DSSF filter */
	WRITE_PHYREG(pi, DSSF_C_CTRL, 0);

	if (on) {
		dssf->channel = CHSPEC_CHANNEL(pi->radio_chanspec);

		if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
			dssf->idepth_s1 = 0;
			dssf->idepth_s2 = 0;
			dssf->enabled_s1 = 1;
			dssf->enabled_s2 = 0;
			dssf->DSSF_C_CTRL = 4;
			switch (dssf->channel) {
				case 3:
					/* bw=20, fc= 2422 MHz */
					/* xtal harmonic freq =  +9 MHz */
					dssf->theta_i_s1 =  4298;
					dssf->theta_q_s1 =  1265;
					dssf->DSSF_gain_th0_s1 = 55;
					dssf->DSSF_gain_th1_s1 = 100;
					dssf->DSSF_gain_th2_s1 = 100;
					break;
				case 4:
					/* bw=20, fc= 2427 MHz */
					/* xtal harmonic freq =  +4 MHz */
					dssf->theta_i_s1 =  1265;
					dssf->theta_q_s1 =  3894;
					dssf->DSSF_gain_th0_s1 = 55;
					dssf->DSSF_gain_th1_s1 = 100;
					dssf->DSSF_gain_th2_s1 = 100;
					break;
				case 5:
					/* bw=20, fc= 2432 MHz */
					/* xtal harmonic freq =  -1 MHz */
					dssf->theta_i_s1 =  3894;
					dssf->theta_q_s1 =  6927;
					dssf->DSSF_gain_th0_s1 = 55;
					dssf->DSSF_gain_th1_s1 = 100;
					dssf->DSSF_gain_th2_s1 = 100;
					break;
				case 6:
					/* bw=20, fc= 2437 MHz */
					/* xtal harmonic freq =  -6 MHz */
					dssf->theta_i_s1 =  6927;
					dssf->theta_q_s1 =  4298;
					dssf->DSSF_gain_th0_s1 = 55;
					dssf->DSSF_gain_th1_s1 = 100;
					dssf->DSSF_gain_th2_s1 = 100;
					break;
				case 11:
					/* bw=20, fc= 2462 MHz */
					/* xtal harmonic freq =  +6.4 MHz */
					dssf->theta_i_s1 =  6449;
					dssf->theta_q_s1 =  3705;
					dssf->DSSF_gain_th0_s1 = 55;
					dssf->DSSF_gain_th1_s1 = 100;
					dssf->DSSF_gain_th2_s1 = 100;
					break;
				case 12:
					/* bw=20, fc= 2467 MHz */
					/* xtal harmonic freq =  +1.4 MHz */
					dssf->theta_i_s1 =  3705;
					dssf->theta_q_s1 =  1743;
					dssf->DSSF_gain_th0_s1 = 55;
					dssf->DSSF_gain_th1_s1 = 100;
					dssf->DSSF_gain_th2_s1 = 100;
					break;
				case 13:
					/* bw=20, fc= 2472 MHz */
					/* xtal harmonic freq =  -3.6 MHz */
					dssf->theta_i_s1 =  1743;
					dssf->theta_q_s1 =  4487;
					dssf->DSSF_gain_th0_s1 = 55;
					dssf->DSSF_gain_th1_s1 = 100;
					dssf->DSSF_gain_th2_s1 = 100;
					break;
				default:
				;
			}
		}
	} else {
		dssf->DSSF_C_CTRL = 0;
		dssf->enabled_s1 = 0;
		dssf->enabled_s2 = 0;
	}

	phy_ac_dssf_setup(pi_ac->rxspuri);

	wlc_phy_resetcca_acphy(pi);
	wlapi_bmac_phyclk_fgc(pi->sh->physhim, OFF);
#endif /* WL_DSSF_DISABLED */
}

void
phy_ac_dssf_43684(phy_ac_rxspur_info_t *rxspuri, bool on)
{
#ifndef WL_DSSF_DISABLED
	phy_info_t *pi = rxspuri->pi;
	phy_info_acphy_t *pi_ac = rxspuri->aci;
	acphy_dssf_values_t *dssf = rxspuri->dssf_params;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	uint8 core;

	dssf->channel = 0;
	dssf->core = 0;

	dssf->theta_i_s1 = 0;
	dssf->theta_q_s1 = 0;
	dssf->theta_i_s2 = 0;
	dssf->theta_q_s2 = 0;
	dssf->enabled_s1 = 0;
	dssf->enabled_s2 = 0;
	dssf->DSSF_C_CTRL = 0;
	dssf->on = on;
	dssf->channel = CHSPEC_CHANNEL(pi->radio_chanspec);

	wlapi_suspend_mac_and_wait(pi->sh->physhim);

	WRITE_PHYREG(pi, DSSF_C_CTRL, 0);

	if (on) {
		dssf->idepth_s1 = 2;
		dssf->idepth_s2 = 2;
		dssf->enabled_s1 = 1;
		dssf->enabled_s2 = 1;
		dssf->DSSF_C_CTRL = 1;
		switch (dssf->channel) {
			case 138:
				/* bw=80, fc=5690 MHz */
				/* platform spur at 10MHz+ppm */
				/* placing double notch: 9925kHz & 10075kHz */
				dssf->theta_i_s1 =  2913;
				dssf->theta_q_s1 =  2878;
				dssf->theta_i_s2 =  2879;
				dssf->theta_q_s2 =  2912;
				break;
			case 142:
				/* bw=40, fc=5710 MHz */
				/* platform spur at -10MHz+ppm */
				/* placing double notch: -9950kHz & -10050kHz */
				dssf->theta_i_s1 =  32;
				dssf->theta_q_s1 =  4097;
				dssf->theta_i_s2 =  8160;
				dssf->theta_q_s2 =  4097;
				break;
			default:
				dssf->idepth_s1 = 0;
				dssf->idepth_s2 = 0;
				dssf->enabled_s1 = 0;
				dssf->enabled_s2 = 0;
				dssf->DSSF_C_CTRL = 0;
				break;
		}
	} else {
		dssf->DSSF_C_CTRL = 0;
		dssf->enabled_s1 = 0;
		dssf->enabled_s2 = 0;
	}

	FOREACH_ACTV_CORE(pi, stf_shdata->phytxchain, core) {
		dssf->core = core;
		phy_ac_dssf_setup(pi_ac->rxspuri);
	}
	wlapi_enable_mac(pi->sh->physhim);
#endif /* WL_DSSF_DISABLED */
}

void
phy_ac_dssfB(phy_ac_rxspur_info_t *rxspuri, bool on)
{
	phy_info_t *pi = rxspuri->pi;
	phy_info_acphy_t *pi_ac = rxspuri->aci;
	acphy_dssfB_values_t *dssfB = rxspuri->dssfB_params;

	if (DSSFB_ENABLE) {
		dssfB->channel = 0;
		dssfB->DSSFB_gain_th0_s1 = 61;
		dssfB->DSSFB_gain_th1_s1 = 67;
		dssfB->DSSFB_gain_th2_s1 = 73;
		dssfB->DSSFB_gain_th0_s2 = 61;
		dssfB->DSSFB_gain_th1_s2 = 67;
		dssfB->DSSFB_gain_th2_s2 = 73;

		dssfB->idepth_s1 = 0;
		dssfB->idepth_s2 = 0;
		dssfB->enabled_s1 = 0;
		dssfB->enabled_s2 = 0;
		dssfB->theta_i_s1 = 0;
		dssfB->theta_q_s1 = 0;
		dssfB->theta_i_s2 = 0;
		dssfB->theta_q_s2 = 0;
		dssfB->DSSFB_C_CTRL = 0;
		dssfB->on = on;
		wlapi_bmac_phyclk_fgc(pi->sh->physhim, ON);
		/* Reset DSSFB filter */
		WRITE_PHYREG(pi, DSSFB_C_CTRL, 0);
		if (on) {
			dssfB->channel = CHSPEC_CHANNEL(pi->radio_chanspec);
			if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
				(ACMINORREV_1(pi) || ACMINORREV_3(pi))) {
				if (PHY_ILNA(pi)) {
					switch (dssfB->channel) {
						/* 2G, 20Mhz && 40Mhz */
						case 4:
							PHY_INFORM((
							"%s: applying dssfB for channel %d\n",
							__FUNCTION__, dssfB->channel));
							dssfB->idepth_s1 = 1;
							dssfB->idepth_s2 = 1;
							dssfB->enabled_s1 = 1;
							dssfB->enabled_s2 = 0;
							dssfB->DSSFB_C_CTRL = 0xc;
							dssfB->theta_i_s1 = 3313; /* freq = 4 */
							dssfB->theta_q_s1 = 2407; /* freq = 4 */
							break;
						case 5:
							PHY_INFORM((
							"%s: applying dssfB for channel %d\n",
							__FUNCTION__, dssfB->channel));
							dssfB->idepth_s1 = 1;
							dssfB->idepth_s2 = 1;
							dssfB->enabled_s1 = 1;
							dssfB->enabled_s2 = 0;
							dssfB->DSSFB_C_CTRL = 0xc;
							dssfB->theta_i_s1 = 4045; /* freq = -1 */
							dssfB->theta_q_s1 = 7551; /* freq = -1 */
							break;
						case 6:
							PHY_INFORM((
							"%s: applying dssfB for channel %d\n",
							__FUNCTION__, dssfB->channel));
							dssfB->idepth_s1 = 1;
							dssfB->idepth_s2 = 1;
							dssfB->enabled_s1 = 1;
							dssfB->enabled_s2 = 0;
							dssfB->DSSFB_C_CTRL = 0xc;
							dssfB->theta_i_s1 = 2406; /* freq = -6 */
							dssfB->theta_q_s1 = 4879; /* freq = -6 */
							break;
						case 11:
							PHY_INFORM((
							"%s: applying dssfB for channel %d\n",
							__FUNCTION__, dssfB->channel));
							dssfB->idepth_s1 = 1;
							dssfB->idepth_s2 = 1;
							dssfB->enabled_s1 = 1;
							dssfB->enabled_s2 = 0;
							dssfB->DSSFB_C_CTRL = 0xc;
							dssfB->theta_i_s1 = 2406; /* freq = 6 */
							dssfB->theta_q_s1 = 3313; /* freq = 6 */
							break;
						case 12:
							PHY_INFORM((
							"%s: applying dssfB for channel %d\n",
							__FUNCTION__, dssfB->channel));
							dssfB->idepth_s1 = 1;
							dssfB->idepth_s2 = 1;
							dssfB->enabled_s1 = 1;
							dssfB->enabled_s2 = 0;
							dssfB->DSSFB_C_CTRL = 0xc;
							dssfB->theta_i_s1 = 4044; /* freq = 1 */
							dssfB->theta_q_s1 = 8833; /* freq = 1 */
							break;
						case 13:
							PHY_INFORM((
							"%s: applying dssfB for channel %d\n",
							__FUNCTION__, dssfB->channel));
							dssfB->idepth_s1 = 1;
							dssfB->idepth_s2 = 1;
							dssfB->enabled_s1 = 1;
							dssfB->enabled_s2 = 0;
							dssfB->DSSFB_C_CTRL = 0xc;
							dssfB->theta_i_s1 = 3313; /* freq = -4 */
							dssfB->theta_q_s1 = 5785; /* freq = -4 */
							break;
						default:
							break;
					}
				}
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
		           ((RADIOMINORREV(pi) == 4) ||
		            (RADIOMINORREV(pi) == 10) ||
		            (RADIOMINORREV(pi) == 11) ||
		            (RADIOMINORREV(pi) == 13)) &&
		           (pi->xtalfreq == 37400000)) {
			switch (dssfB->channel) {
			case 4:
				dssfB->idepth_s1 = 2;
				dssfB->enabled_s1 = 1;
				dssfB->theta_i_s1 = 3313;
				dssfB->theta_q_s1 = 2407;
				break;
			case 5:
				dssfB->idepth_s1 = 2;
				dssfB->enabled_s1 = 1;
				dssfB->theta_i_s1 = 4045;
				dssfB->theta_q_s1 = 7551;
				break;
			case 6:
				dssfB->idepth_s1 = 2;
				dssfB->enabled_s1 = 1;
				dssfB->theta_i_s1 = 2406;
				dssfB->theta_q_s1 = 4879;
				break;
			case 11:
				dssfB->idepth_s1 = 1;
				dssfB->enabled_s1 = 1;
				dssfB->theta_i_s1 = 2106;
				dssfB->theta_q_s1 = 3313;
				break;
			case 12:
				dssfB->idepth_s1 = 1;
				dssfB->enabled_s1 = 1;
				dssfB->theta_i_s1 = 4044;
				dssfB->theta_q_s1 =  901;
				break;
			case 13:
				dssfB->idepth_s1 = 1;
				dssfB->enabled_s1 = 1;
				dssfB->theta_i_s1 = 3313;
				dssfB->theta_q_s1 = 6086;
				break;
			}
			if (dssfB->theta_i_s1 || dssfB->theta_i_s2) {
				dssfB->DSSFB_C_CTRL = 0xb;
				PHY_INFORM(("%s: xtal freq 37.4M, applying dssfB for channel %d\n",
				            __FUNCTION__, dssfB->channel));
			}
		}
		} else {
			dssfB->DSSFB_C_CTRL = 0;
			dssfB->enabled_s1 = 0;
			dssfB->enabled_s2 = 0;
		}

		phy_ac_dssfB_setup(pi_ac->rxspuri);
		wlc_phy_resetcca_acphy(pi);
		wlapi_bmac_phyclk_fgc(pi->sh->physhim, OFF);
	}
}

void
phy_ac_spurwar(phy_ac_rxspur_info_t *rxspuri, uint8 noise_var[][ACPHY_SPURWAR_NV_NTONES],
               int8 *tone_id, uint8 *core_sp)
{
	phy_info_t *pi = rxspuri->pi;
	uint8 i;
	uint16 channel = 0;

	/* Starting offset for spurwar */
	i = ACPHY_SPURWAR_NTONES_OFFSET;

	channel = CHSPEC_CHANNEL(pi->radio_chanspec);

	if (PHY_ILNA(pi)) {
		/* Spur war for 4350 */
		if (ACMAJORREV_2(pi->pubpi->phy_rev) && (ACMINORREV_1(pi) || ACMINORREV_3(pi))) {
			switch (channel) {
				case 4:
					*core_sp = 1; /* core 0 */
					tone_id[i+0] = 12;
					tone_id[i+1] = 13;
					noise_var[PHY_CORE_0][i+0] = 9;
					noise_var[PHY_CORE_0][i+1] = 9;
					PHY_INFORM(("phy_ac_spurwar: applying spurwar"
								" for channel %d\n", channel));
					break;
				case 5:
					*core_sp = 1; /* core 0 */
					tone_id[i+0] = -3;
					tone_id[i+1] = -4;
					noise_var[PHY_CORE_0][i+0] = 9;
					noise_var[PHY_CORE_0][i+1] = 9;
					PHY_INFORM(("phy_ac_spurwar: applying spurwar"
								" for channel %d\n", channel));
					break;
				case 6:
					*core_sp = 1; /* core 0 */
					tone_id[i+0] = -19;
					tone_id[i+1] = -20;
					noise_var[PHY_CORE_0][i+0] = 9;
					noise_var[PHY_CORE_0][i+1] = 9;
					PHY_INFORM(("phy_ac_spurwar: applying spurwar"
								" for channel %d\n", channel));
					break;
				case 11:
					*core_sp = 1; /* core 0 */
					tone_id[i+0] = 20;
					tone_id[i+1] = 21;
					noise_var[PHY_CORE_0][i+0] = 9;
					noise_var[PHY_CORE_0][i+1] = 9;
					PHY_INFORM(("phy_ac_spurwar: applying spurwar"
								" for channel %d\n", channel));
					break;
				case 12:
					*core_sp = 1; /* core 0 */
					tone_id[i+0] = 4;
					tone_id[i+1] = 5;
					noise_var[PHY_CORE_0][i+0] = 9;
					noise_var[PHY_CORE_0][i+1] = 9;
					PHY_INFORM(("phy_ac_spurwar: applying spurwar"
								" for channel %d\n", channel));
					break;
				case 13:
					*core_sp = 1; /* core 0 */
					tone_id[i+0] = -11;
					tone_id[i+1] = -12;
					noise_var[PHY_CORE_0][i+0] = 9;
					noise_var[PHY_CORE_0][i+1] = 9;
					PHY_INFORM(("phy_ac_spurwar: applying spurwar"
								" for channel %d\n", channel));
					break;
				default:
					break;
			}
		} else {
			*core_sp = 1; /* core 0 */
			switch (channel) {
				case 1:
					tone_id[i+0] = 1;
					noise_var[PHY_CORE_0][i+0] = 0x9;
					break;
				case 2:
					tone_id[i+0] = -15;
					noise_var[PHY_CORE_0][i+0] = 0x9;
					break;
				case 4:
					tone_id[i+0] = 1;
					tone_id[i+1] = -1;
					tone_id[i+2] = 12;
					tone_id[i+3] = 13;
					noise_var[PHY_CORE_0][i+0] = 0x4;
					noise_var[PHY_CORE_0][i+1] = 0x4;
					noise_var[PHY_CORE_0][i+2] = 0x2;
					noise_var[PHY_CORE_0][i+3] = 0x9;
					break;
				case 5:
					tone_id[i+0] = -3;
					tone_id[i+1] = -4;
					tone_id[i+2] = 17;
					tone_id[i+3] = 18;
					tone_id[i+4] = 25;
					tone_id[i+5] = 26;
					noise_var[PHY_CORE_0][i+0] = 0x9;
					noise_var[PHY_CORE_0][i+1] = 0x3;
					noise_var[PHY_CORE_0][i+2] = 0x5;
					noise_var[PHY_CORE_0][i+3] = 0x2;
					noise_var[PHY_CORE_0][i+4] = 0x3;
					noise_var[PHY_CORE_0][i+5] = 0x4;
					break;
				case 6:
					tone_id[i+0] = 28;
					tone_id[i+1] = 29;
					tone_id[i+2] = -19;
					tone_id[i+3] = -20;
					tone_id[i+4] = 1;
					tone_id[i+5] = 2;
					tone_id[i+6] = 9;
					tone_id[i+7] = 10;
					noise_var[PHY_CORE_0][i+0] = 0x3;
					noise_var[PHY_CORE_0][i+1] = 0x3;
					noise_var[PHY_CORE_0][i+2] = 0x9;
					noise_var[PHY_CORE_0][i+3] = 0x3;
					noise_var[PHY_CORE_0][i+4] = 0x5;
					noise_var[PHY_CORE_0][i+5] = 0x2;
					noise_var[PHY_CORE_0][i+6] = 0x3;
					noise_var[PHY_CORE_0][i+7] = 0x4;
					break;
				case 7:
					tone_id[i+0] = 24;
					tone_id[i+1] = 25;
					tone_id[i+2] = -6;
					tone_id[i+3] = -7;
					tone_id[i+4] = -14;
					tone_id[i+5] = -15;
					noise_var[PHY_CORE_0][i+0] = 0x7;
					noise_var[PHY_CORE_0][i+1] = 0x7;
					noise_var[PHY_CORE_0][i+2] = 0x4;
					noise_var[PHY_CORE_0][i+3] = 0x3;
					noise_var[PHY_CORE_0][i+4] = 0x3;
					noise_var[PHY_CORE_0][i+5] = 0x5;
					break;
				case 8:
					tone_id[i+0] = -14;
					tone_id[i+1] = -15;
					tone_id[i+2] = -22;
					tone_id[i+3] = -23;
					tone_id[i+4] = 8;
					tone_id[i+5] = 9;
					noise_var[PHY_CORE_0][i+0] = 0x4;
					noise_var[PHY_CORE_0][i+1] = 0x2;
					noise_var[PHY_CORE_0][i+2] = 0x5;
					noise_var[PHY_CORE_0][i+3] = 0x6;
					noise_var[PHY_CORE_0][i+4] = 0x4;
					noise_var[PHY_CORE_0][i+5] = 0x9;
					break;
				case 9:
					tone_id[i+0] = -7;
					tone_id[i+1] = -8;
					tone_id[i+2] = 25;
					tone_id[i+3] = 26;
					noise_var[PHY_CORE_0][i+0] = 0x8;
					noise_var[PHY_CORE_0][i+1] = 0x3;
					noise_var[PHY_CORE_0][i+2] = 0x4;
					noise_var[PHY_CORE_0][i+3] = 0x5;
					break;
				case 10:
					tone_id[i+0] = 17;
					tone_id[i+1] = 18;
					tone_id[i+2] = -23;
					tone_id[i+3] = -24;
					tone_id[i+4] = 9;
					tone_id[i+5] = 10;
					noise_var[PHY_CORE_0][i+0] = 0x3;
					noise_var[PHY_CORE_0][i+1] = 0x4;
					noise_var[PHY_CORE_0][i+2] = 0x8;
					noise_var[PHY_CORE_0][i+3] = 0x3;
					noise_var[PHY_CORE_0][i+4] = 0x4;
					noise_var[PHY_CORE_0][i+5] = 0x5;
					break;
				case 11:
					tone_id[i+0] = 20;
					tone_id[i+1] = 21;
					tone_id[i+2] = 1;
					tone_id[i+3] = 2;
					noise_var[PHY_CORE_0][i+0] = 0x7;
					noise_var[PHY_CORE_0][i+1] = 0x6;
					noise_var[PHY_CORE_0][i+2] = 0x5;
					noise_var[PHY_CORE_0][i+3] = 0x7;
					break;
				case 12:
					tone_id[i+0] = -14;
					tone_id[i+1] = 4;
					tone_id[i+2] = 5;
					tone_id[i+3] = -22;
					tone_id[i+4] = -23;
					tone_id[i+5] = 25;
					noise_var[PHY_CORE_0][i+0] = 0x4;
					noise_var[PHY_CORE_0][i+1] = 0x8;
					noise_var[PHY_CORE_0][i+2] = 0x7;
					noise_var[PHY_CORE_0][i+3] = 0x7;
					noise_var[PHY_CORE_0][i+4] = 0x7;
					noise_var[PHY_CORE_0][i+5] = 0x5;
					break;
				case 13:
					tone_id[i+0] = -11;
					tone_id[i+1] = -12;
					tone_id[i+2] = 25;
					tone_id[i+3] = 26;
					tone_id[i+4] = 9;
					noise_var[PHY_CORE_0][i+0] = 0x6;
					noise_var[PHY_CORE_0][i+1] = 0x7;
					noise_var[PHY_CORE_0][i+2] = 0x7;
					noise_var[PHY_CORE_0][i+3] = 0x7;
					noise_var[PHY_CORE_0][i+4] = 0x4;
					break;
				case 14:
					tone_id[i+0] = 10;
					tone_id[i+1] = -4;
					tone_id[i+2] = -5;
					noise_var[PHY_CORE_0][i+0] = 0x9;
					noise_var[PHY_CORE_0][i+1] = 0x4;
					noise_var[PHY_CORE_0][i+2] = 0x4;
					break;
				case 108:
					tone_id[i+0] = -15;
					tone_id[i+1] = -16;
					noise_var[PHY_CORE_0][i+0] = 0x9;
					noise_var[PHY_CORE_0][i+1] = 0x9;
					break;
				case 153:
					tone_id[i+0] = -17;
					tone_id[i+1] = -18;
					noise_var[PHY_CORE_0][i+0] = 0x9;
					noise_var[PHY_CORE_0][i+1] = 0x9;
					break;
				case 161:
					tone_id[i+0] = -25;
					tone_id[i+1] = -26;
					noise_var[PHY_CORE_0][i+0] = 0x9;
					noise_var[PHY_CORE_0][i+1] = 0x9;
					break;
				default:
					break;
			}
		}
	} else if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
	           ((RADIOMINORREV(pi) == 4) ||
	            (RADIOMINORREV(pi) == 10) ||
	            (RADIOMINORREV(pi) == 11) ||
	            (RADIOMINORREV(pi) == 13)) &&
	           (pi->xtalfreq == 37400000)) {

		switch (channel) {
		case 4:
			*core_sp = 3;
			tone_id[i+0] = 12;
			tone_id[i+1] = 13;
			noise_var[PHY_CORE_0][i+0] = 0x3;
			noise_var[PHY_CORE_0][i+1] = 0x9;
			noise_var[PHY_CORE_1][i+0] = 0x3;
			noise_var[PHY_CORE_1][i+1] = 0x9;
			PHY_INFORM(("%s: 4350 spurwar nvshp for channel %d\n",
			            __FUNCTION__, channel));
			break;
		case 5:
			*core_sp = 3;
			tone_id[i+0] = -3;
			tone_id[i+1] = -4;
			noise_var[PHY_CORE_0][i+0] = 0x9;
			noise_var[PHY_CORE_0][i+1] = 0x3;
			noise_var[PHY_CORE_1][i+0] = 0x9;
			noise_var[PHY_CORE_1][i+1] = 0x3;
			PHY_INFORM(("%s: 4350 spurwar nvshp for channel %d\n",
			            __FUNCTION__, channel));
			break;
		case 6:
			*core_sp = 3;
			tone_id[i+0] = -19;
			tone_id[i+1] = -20;
			noise_var[PHY_CORE_0][i+0] = 0x9;
			noise_var[PHY_CORE_0][i+1] = 0x3;
			noise_var[PHY_CORE_1][i+0] = 0x9;
			noise_var[PHY_CORE_1][i+1] = 0x3;
			PHY_INFORM(("%s: 4350 spurwar nvshp for channel %d\n",
			            __FUNCTION__, channel));
			break;
		case 11:
			*core_sp = 3;
			tone_id[i+0] = 20;
			tone_id[i+1] = 21;
			noise_var[PHY_CORE_0][i+0] = 9;
			noise_var[PHY_CORE_0][i+1] = 3;
			noise_var[PHY_CORE_1][i+0] = 9;
			noise_var[PHY_CORE_1][i+1] = 3;
			PHY_INFORM(("%s: 4350 spurwar nvshp for channel %d\n",
			            __FUNCTION__, channel));
			break;
		case 12:
			*core_sp = 3;
			tone_id[i+0] = 4;
			tone_id[i+1] = 5;
			noise_var[PHY_CORE_0][i+0] = 3;
			noise_var[PHY_CORE_0][i+1] = 9;
			noise_var[PHY_CORE_1][i+0] = 3;
			noise_var[PHY_CORE_1][i+1] = 9;
			PHY_INFORM(("%s: 4350 spurwar nvshp for channel %d\n",
			            __FUNCTION__, channel));
			break;
		case 13:
			*core_sp = 3;
			tone_id[i+0] = -11;
			tone_id[i+1] = -12;
			noise_var[PHY_CORE_0][i+0] = 6;
			noise_var[PHY_CORE_0][i+1] = 6;
			noise_var[PHY_CORE_1][i+0] = 6;
			noise_var[PHY_CORE_1][i+1] = 6;
			PHY_INFORM(("%s: 4350 spurwar nvshp for channel %d\n",
			            __FUNCTION__, channel));
			break;
		}
	}
}

static void
phy_ac_set_spurmode(phy_type_rxspur_ctx_t *ctx, uint16 freq)
{
	phy_ac_rxspur_info_t *rxspuri = (phy_ac_rxspur_info_t *) ctx;
	phy_ac_get_spurmode(rxspuri, freq);

	if (rxspuri->curr_spurmode != rxspuri->acphy_spuravoid_mode) {
		phy_ac_setup_spurmode(rxspuri);
		rxspuri->curr_spurmode = rxspuri->acphy_spuravoid_mode;
	}
}

static void
phy_ac_get_spurmode(phy_ac_rxspur_info_t *rxspuri, uint16 freq)
{
	phy_info_t *pi = rxspuri->pi;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	if (pi->block_for_slowcal) {
		pi->blocked_freq_for_slowcal = freq;
		return;
	}

	if (EMBEDDED_2x2AX_CORE(pi->sh->chip)) {
		rxspuri->acphy_spuravoid_mode = 1;
	} else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		if (CHSPEC_IS2G(pi->radio_chanspec) &&
			(((CHSPEC_CHANNEL(pi->radio_chanspec) >= 1) &&
			(CHSPEC_CHANNEL(pi->radio_chanspec) <= 5)) ||
			(CHSPEC_IS40(pi->radio_chanspec) &&
			((CHSPEC_CHANNEL(pi->radio_chanspec) >= 3) &&
			(CHSPEC_CHANNEL(pi->radio_chanspec) <= 7))))) {
			rxspuri->acphy_spuravoid_mode = 1;
		} else {
			rxspuri->acphy_spuravoid_mode = 0;
		}
	} else {
		rxspuri->acphy_spuravoid_mode = 0;
	}
}

#if defined(WLTEST)
static int
phy_ac_rxspur_set_force_spurmode(phy_type_rxspur_ctx_t *ctx, int16 int_val)
{
	phy_ac_rxspur_info_t *rxspuri = (phy_ac_rxspur_info_t *) ctx;
	phy_info_t *pi = rxspuri->pi;

	uint16 freq;
	freq = (uint16)phy_utils_channel2freq(CHSPEC_CHANNEL(pi->radio_chanspec));

	if (int_val == -1) {
		rxspuri->acphy_spuravoid_mode_override = 0;
	} else {
		rxspuri->acphy_spuravoid_mode_override = 1;
	}

	if (pi->block_for_slowcal) {
		pi->blocked_freq_for_slowcal = freq;
		return BCME_OK;
	}

	switch (int_val) {
	case -1:
		PHY_TRACE(("Spurmode override is off; default spurmode restored; %s \n",
		__FUNCTION__));
		phy_ac_get_spurmode(rxspuri, freq);
		break;
	case 0:
		PHY_TRACE(("Force spurmode to 0; chanfreq %d: PLLfre:963Mhz; %s \n",
			freq, __FUNCTION__));
		rxspuri->acphy_spuravoid_mode = 0;
		break;
	case 1:
		PHY_TRACE(("Force spurmode to 1; chanfreq %d: PLLfre:960Mhz; %s \n",
			freq, __FUNCTION__));
		rxspuri->acphy_spuravoid_mode = 1;
		break;
	case 2:
		PHY_TRACE(("Force spurmode to 2; chanfreq %d: PLLfre:961Mhz; %s \n",
			freq, __FUNCTION__));
		rxspuri->acphy_spuravoid_mode = 2;
		break;
	case 3:
		PHY_TRACE(("Force spurmode to 3; chanfreq %d: PLLfre:964Mhz; %s \n",
			freq, __FUNCTION__));
		rxspuri->acphy_spuravoid_mode = 3;
		break;
	case 4:
		PHY_TRACE(("Force spurmode to 4; chanfreq %d: PLLfre:962Mhz; %s \n",
			freq, __FUNCTION__));
		rxspuri->acphy_spuravoid_mode = 4;
		break;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		PHY_TRACE(("Force spurmode to %d; chanfreq %d: PLLfre:96%dMhz; %s \n",
			int_val, freq, int_val, __FUNCTION__));
		rxspuri->acphy_spuravoid_mode = (uint8) int_val;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported spurmode %d.\n",
			pi->sh->unit, __FUNCTION__, int_val));
		ASSERT(0);
		phy_ac_get_spurmode(rxspuri, freq);
		return BCME_ERROR;
	}
	/* Call the bbpll settings related function only if the forced */
	/* spur mode is different from the current spur mode */
	if (rxspuri->curr_spurmode != rxspuri->acphy_spuravoid_mode) {
		phy_ac_setup_spurmode(rxspuri);
		rxspuri->curr_spurmode =  rxspuri->acphy_spuravoid_mode;
	}
	return BCME_OK;
}

static int
phy_ac_rxspur_get_force_spurmode(phy_type_rxspur_ctx_t *ctx, int32 *ret_int_ptr)
{
	phy_ac_rxspur_info_t *rxspuri = (phy_ac_rxspur_info_t *) ctx;

	if (rxspuri->acphy_spuravoid_mode_override == 1) {
		*ret_int_ptr = rxspuri->acphy_spuravoid_mode;
	} else {
		*ret_int_ptr = -1;
	}
	return BCME_OK;
}
#endif /* defined(WLTEST) */

void phy_ac_dssf_setup_iov(phy_info_t *pi, void *p)
{
#ifndef WL_DSSF_DISABLED
	int32 *dssf_args = p;
	uint8 en = (uint8) dssf_args[0];
	int32 f_kHz = dssf_args[1];
	int8 depth_mode = (int8) dssf_args[2];
	uint8 stage = (uint8) dssf_args[3];
	uint8 rxcore = (uint8) dssf_args[4];
	uint8 phy_bw, core, depth, disCrsCorr;

	/* disableCRSCorr has nothing to do with DSSF
	 * however it belongs to DSSF_C_CTRL reg, which is written below
	 * hence we save the value here and restore at the end
	 */
	disCrsCorr = READ_PHYREGFLD(pi, DSSF_C_CTRL, disableCRSCorr);

	if (en) { /* enable DSSF on desired stage & core at inputted freq/depth */
		if (CHSPEC_IS160(pi->radio_chanspec))
			phy_bw = 160;
		else if (CHSPEC_IS80(pi->radio_chanspec))
			phy_bw = 80;
		else if (CHSPEC_IS40(pi->radio_chanspec))
			phy_bw = 40;
		else
			phy_bw = 20;

		phy_ac_set_dssf_freq(pi, stage, rxcore, f_kHz, phy_bw);

		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		WRITE_PHYREG(pi, DSSF_C_CTRL, 1);
		switch (depth_mode) {
			case -1:
				WRITE_PHYREG(pi, DSSF_C_CTRL, 0x4); /* auto mode */
				depth = 0;
				break;
			case 0:
				WRITE_PHYREG(pi, DSSF_C_CTRL, 0x0); /* off */
				depth = 0;
				break;
			case 6:
				depth = 0; /* 6dB fixed (always on) */
				break;
			case 12:
				depth = 1; /* 12dB fixed (always on) */
				break;
			case 18:
				depth = 2; /* 18dB fixed (always on) */
				break;
			default:
				depth = 0;
				break;
		}

		if (stage == 1) {
			/* 1st notch */
			MOD_PHYREGCE(pi, DSSF_control_, rxcore, enabled_s1, 1);
			MOD_PHYREGCE(pi, DSSF_control_, rxcore, idepth_s1, depth);
		} else if (stage == 2) {
			/* 2nd notch */
			MOD_PHYREGCE(pi, DSSF_control_, rxcore, enabled_s2, 1);
			MOD_PHYREGCE(pi, DSSF_control_, rxcore, idepth_s2, depth);
		} else if (stage == 3) {
			/* 3rd notch */
			MOD_PHYREGCE(pi, DSSF_control_, rxcore, enabled_s3, 1);
			MOD_PHYREGCE(pi, DSSF_control_, rxcore, idepth_s3, depth);
		}
		MOD_PHYREG(pi, DSSF_C_CTRL, disableCRSCorr, disCrsCorr);
		wlapi_enable_mac(pi->sh->physhim);
	} else { /* disable all cores and reset the regs */
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
		FOREACH_CORE(pi, core) {
			WRITE_PHYREG(pi, DSSF_C_CTRL, 0x2000);
			MOD_PHYREGCE(pi, DSSF_control_, core, enabled_s1, 0);
			MOD_PHYREGCE(pi, DSSF_control_, core, enabled_s2, 0);
			MOD_PHYREGCE(pi, DSSF_control_, core, enabled_s3, 0);
			MOD_PHYREGCE(pi, DSSF_control_, core, idepth_s1, 0);
			MOD_PHYREGCE(pi, DSSF_control_, core, idepth_s2, 0);
			MOD_PHYREGCE(pi, DSSF_control_, core, idepth_s3, 0);
			WRITE_PHYREGCE(pi, DSSF_exp_j_theta_i_s1, core, 0);
			WRITE_PHYREGCE(pi, DSSF_exp_i_theta_q_s1, core, 0);
			WRITE_PHYREGCE(pi, DSSF_exp_j_theta_i_s2, core, 0);
			WRITE_PHYREGCE(pi, DSSF_exp_i_theta_q_s2, core, 0);
			WRITE_PHYREGCE(pi, DSSF_exp_j_theta_i_s3, core, 0);
			WRITE_PHYREGCE(pi, DSSF_exp_i_theta_q_s3, core, 0);
		}
		MOD_PHYREG(pi, DSSF_C_CTRL, disableCRSCorr, disCrsCorr);
		wlapi_enable_mac(pi->sh->physhim);
	}
#endif /* WL_DSSF_DISABLED */
}

static void
phy_ac_set_dssf_freq(phy_info_t *pi, uint8 stage, uint8 core, int32 f_kHz, uint8 phy_bw)
{
	fixed theta = 0, rot = 0;
	uint16 i_samp, q_samp;
	math_cint32 tone_samp;

	rot = FIXED((ABS(f_kHz) * 36)/phy_bw) / 100; /* 2*pi*f/20/1000  Note: f in Hz */
	theta = 0;

	if (f_kHz > 0)
		theta += rot;
	else
		theta -= rot;

	math_cmplx_cordic(theta, &tone_samp);

	i_samp = (uint16)(FLOAT(tone_samp.i * 4095) & 0x1fff);
	q_samp = (uint16)(FLOAT(tone_samp.q * 4095) & 0x1fff);

	wlapi_suspend_mac_and_wait(pi->sh->physhim);

	if (stage == 1) {
		/* 1st notch */
		WRITE_PHYREGCE(pi, DSSF_exp_j_theta_i_s1, core, i_samp);
		WRITE_PHYREGCE(pi, DSSF_exp_i_theta_q_s1, core, q_samp);
	} else if (stage == 2) {
		/* 2nd notch */
		WRITE_PHYREGCE(pi, DSSF_exp_j_theta_i_s2, core, i_samp);
		WRITE_PHYREGCE(pi, DSSF_exp_i_theta_q_s2, core, q_samp);
	} else if (stage == 3) {
		/* 3rd notch */
		WRITE_PHYREGCE(pi, DSSF_exp_j_theta_i_s3, core, i_samp);
		WRITE_PHYREGCE(pi, DSSF_exp_i_theta_q_s3, core, q_samp);
	}
	wlapi_enable_mac(pi->sh->physhim);
}
