/*
 * ACPHY TXIQLO CAL module implementation
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
 * $Id: phy_ac_txiqlocal.c 806354 2021-12-20 09:44:21Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <bcm_math.h>
#include <phy_btcx.h>
#include <phy_calmgr.h>
#include <phy_type_txiqlocal.h>
#include <phy_ac.h>
#include <phy_ac_txiqlocal.h>
#include <wlc_radioreg_20693.h>
#include <wlc_radioreg_20698.h>
#include <wlc_radioreg_20704.h>
#include <wlc_radioreg_20707.h>
#include <wlc_radioreg_20708.h>
#include <wlc_radioreg_20709.h>
#include <wlc_radioreg_20710.h>
#include <wlc_phyreg_ac.h>
#include <phy_utils_reg.h>
#include <wlc_phy_radio.h>
#include <phy_ac_info.h>
#include <phy_ac_radio.h>
#include <phy_rxgcrs_api.h>
#include <phy_ac_tof.h>
#include <phy_rstr.h>
#include <phy_utils_var.h>
#include <bcmdevs.h>
#include <phy_stf.h>

/* module private data */
typedef struct _acphy_txcal_phyregs {
	bool   is_orig;
	uint16 BBConfig;
	uint16 RxFeCtrl1;
	uint16 AfePuCtrl;
	uint16 RfctrlOverrideAfeCfg[PHY_CORE_MAX];
	uint16 RfctrlCoreAfeCfg1[PHY_CORE_MAX];
	uint16 RfctrlCoreAfeCfg2[PHY_CORE_MAX];
	uint16 RfctrlIntc[PHY_CORE_MAX];
	uint16 RfctrlOverrideRxPus[PHY_CORE_MAX];
	uint16 RfctrlCoreRxPus[PHY_CORE_MAX];
	uint16 RfctrlOverrideTxPus[PHY_CORE_MAX];
	uint16 RfctrlCoreTxPus[PHY_CORE_MAX];
	uint16 RfctrlOverrideLpfSwtch[PHY_CORE_MAX];
	uint16 RfctrlCoreLpfSwtch[PHY_CORE_MAX];
	uint16 RfctrlOverrideLpfCT[PHY_CORE_MAX];
	uint16 RfctrlCoreLpfCT[PHY_CORE_MAX];
	uint16 RfctrlCoreLpfGmult[PHY_CORE_MAX];
	uint16 RfctrlCoreRCDACBuf[PHY_CORE_MAX];
	uint16 RfctrlOverrideAuxTssi[PHY_CORE_MAX];
	uint16 RfctrlCoreAuxTssi1[PHY_CORE_MAX];
	uint16 PapdEnable[PHY_CORE_MAX];
	uint16 RfseqCoreActv2059;
	uint16 sampleCmd;
	uint16 fineclockgatecontrol;
} txiqcal_phyregs_t;

typedef struct {
	uint16 cmds_REFINE[10];
	uint16 cmds_RESTART[10];
	uint16 cmds_LOPWR[2];
	uint16 cmds_REFINE_FDIQ[10];
	uint16 cmds_RESTART_FDIQ[10];
	uint16 cmds_REFINE_PRERX[4];
	uint16 cmds_RESTART_PRERX[4];
	uint16 pad_16[10];

	uint8 num_cmds_refine;
	uint8 num_cmds_restart;
	uint8 num_cmds_lopwr;
	uint8 num_cmds_refine_fdiq;
	uint8 num_cmds_restart_fdiq;
	uint8 num_cmds_refine_prerx;
	uint8 num_cmds_restart_prerx;
	uint8 nsamp_gctrl[4];
	uint8 nsamp_corrs[4];
	uint8 thres_ladder[7];
	uint8 nsamp_corrs_new[8];
	uint8 pad_8[10];

	txiqcal_phyregs_t		*porig;
	uint16 tone_freq;

	uint8 start_casc;
	uint8 stop_casc;
	uint8 step_casc;
	uint8 start_idx;
	uint8 stop_idx;
	uint8 step_idx;
	int16 delta_tssi_error;
	uint8 lower_lim;
	uint8 initindex;
	bool casc_gctrl;
	int16 target_tssi_set[4];
	bool reset_loftcoefs_prerxcal;

	uint8 num_multilo;
	uint16 multilo_cal_idx[10];
	uint16 multilo_start_idx[10];
	uint16 multilo_end_idx[10];
} txiqcal_params_t;

typedef struct acphy_tx_fdiqi_ctl_struct {
	int8 slope[PHY_CORE_MAX];
	bool enabled;
} acphy_tx_fdiqi_ctl_t;

typedef struct acphy_fdiqi_struct {
	int32 freq;
	int32 angle[PHY_CORE_MAX];
	int32 mag[PHY_CORE_MAX];
} acphy_fdiqi_t;

typedef struct _acphy_txcal_radioregs {
	bool   is_orig;
	uint16 iqcal_cfg1[PHY_CORE_MAX];
	uint16 pa2g_tssi[PHY_CORE_MAX];
	uint16 OVR20[PHY_CORE_MAX];
	uint16 OVR21[PHY_CORE_MAX];
	uint16 tx5g_tssi[PHY_CORE_MAX];
	uint16 iqcal_cfg2[PHY_CORE_MAX];
	uint16 iqcal_cfg3[PHY_CORE_MAX];
	uint16 auxpga_cfg1[PHY_CORE_MAX];
	uint16 iqcal_ovr1[PHY_CORE_MAX];
	uint16 tx_top_5g_ovr1[PHY_CORE_MAX];
	uint16 adc_cfg10[PHY_CORE_MAX];
	uint16 adc_cfg18[PHY_CORE_MAX];
	uint16 auxpga_ovr1[PHY_CORE_MAX];
	uint16 testbuf_ovr1[PHY_CORE_MAX];
	uint16 spare_cfg6[PHY_CORE_MAX];
	uint16 pa2g_cfg1[PHY_CORE_MAX];
	uint16 adc_ovr1[PHY_CORE_MAX];
	uint16 tx5g_misc_cfg1[PHY_CORE_MAX];
	uint16 tx2g_misc_cfg1[PHY_CORE_MAX];
	uint16 testbuf_cfg1[PHY_CORE_MAX];
	uint16 tia_cfg5[PHY_CORE_MAX];
	uint16 tia_cfg9[PHY_CORE_MAX];
	uint16 auxpga_vmid[PHY_CORE_MAX];
	uint16 pmu_cfg4[PHY_CORE_MAX];
	uint16 OVR3[PHY_CORE_MAX];
	uint16 tx_top_2g_ovr_north[PHY_CORE_MAX];
	uint16 tx_top_2g_ovr_east[PHY_CORE_MAX];
	uint16 pmu_ovr[PHY_CORE_MAX];
	uint16 tx_top_5g_ovr2[PHY_CORE_MAX];
	uint16 txmix5g_cfg2[PHY_CORE_MAX];
	uint16 pad5g_cfg1[PHY_CORE_MAX];
	uint16 pa5g_cfg1[PHY_CORE_MAX];
} acphy_txcal_radioregs_t;

/* module private states */
struct phy_ac_txiqlocal_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_txiqlocal_info_t *cmn_info;
	txiqcal_params_t	*paramsi;
	acphy_tx_fdiqi_ctl_t *txfdiqi;
	/* cache coeffs */
	txcal_coeffs_t *txcal_cache; /* Array of size PHY_CORE_MAX */
	acphy_txcal_radioregs_t	*ac_txcal_radioregs_orig;
	uint16 *cmds;
	uint16 papdState[PHY_CORE_MAX];
	uint16 loft_coeffs[3];
	uint16 txcal_cache_cookie;
	uint16 classifier_state;
	uint8  num_cmds_per_core;
	uint8  bw_idx;
	uint8  cmd_stop_idx;
	uint8  cmd_idx;
	uint8  num_cores;
	uint8  prerxcal;
	uint8  phase_id;
	uint8  num_mphases;
	int32   txiqcalidx_iovar;
	int32  txiqcal_target_tssi_iovar;
	int8   txiqcal_tssisearch_en;
	int8   txiqcal_target_tssi_iovar_set;
};

/* local functions */
typedef struct _acphy_precal_4349_radregs_t {
	uint16 auxpga_ovr1[PHY_CORE_MAX];
	uint16 auxpga_cfg1[PHY_CORE_MAX];
	uint16 auxpga_vmid[PHY_CORE_MAX];
	uint16 tx_top_5g_ovr1[PHY_CORE_MAX];
	uint16 tx5g_misc_cfg1[PHY_CORE_MAX];
	uint16 tx_top_2g_ovr_east[PHY_CORE_MAX];
	uint16 pa2g_cfg1[PHY_CORE_MAX];
	uint16 iqcal_cfg1[PHY_CORE_MAX];
	uint16 tx2g_misc_cfg1[PHY_CORE_MAX];
} acphy_precal_4349_radregs_t;

txiqcal_params_t *
phy_ac_txiqlocal_populate_params(phy_ac_txiqlocal_info_t *iqcal_info);
void wlc_phy_txcal_coeffs_print(uint16 *coeffs, uint8 core);

txiqcal_params_t *
BCMATTACHFN(phy_ac_txiqlocal_populate_params)(phy_ac_txiqlocal_info_t *iqcal_info)
{
	phy_info_t *pi = iqcal_info->pi;
	txiqcal_params_t *params = iqcal_info->paramsi;

	uint16 *cmd_restart_ptr = NULL;
	uint16 *cmd_refine_ptr = NULL;
	uint16 *cmd_lopwr_ptr = NULL;
	uint16 *cmd_restart_prerx_ptr = NULL;
	uint16 *cmd_refine_prerx_ptr = NULL;
	uint16 *cmd_restart_fdiq_ptr = NULL;
	uint16 *cmd_refine_fdiq_ptr = NULL;

	uint8 num_cmds_restart;
	uint8 num_cmds_refine;
	uint8 num_cmds_lopwr;
	uint8 num_cmds_restart_prerx;
	uint8 num_cmds_refine_prerx;
	uint8 num_cmds_restart_fdiq = 0;
	uint8 num_cmds_refine_fdiq = 0;

	uint8 nsamp_gctrl[4] = {0};
	uint8 nsamp_corrs[4] = {0};
	uint8 nsamp_corrs_new[8] = {0};
	uint8 thres_ladder[7] = {0};
	uint8 thresh_ladder_len = 7;

	int16 target_tssi_set[4] = {65, 35, 100, 450};

	/* Table of commands for RESTART & REFINE search-modes
	 *
	 *     This uses the following format (three hex nibbles left to right)
	 *      1. cal_type: 0 = IQ (a/b),   1 = deprecated
	 *                   2 = LOFT digital (di/dq)
	 *                   3 = LOFT analog, fine,   injected at mixer      (ei/eq)
	 *                   4 = LOFT analog, coarse, injected at mixer, too (fi/fq)
	 *      2. initial stepsize (in log2)
	 *      3. number of cal precision "levels"
	 *
	 *     Notes: - functions assumes that order of LOFT cal cmds will be f => e => d,
	 *              where it's ok to have multiple cmds (say interrupted by IQ) of
	 *              the same type; this is due to zeroing out of e and/or d that happens
	 *              even during REFINE cal to avoid a coefficient "divergence" (increasing
	 *              LOFT comp over time of different types that cancel each other)
	 *            - final cal cmd should NOT be analog LOFT cal (otherwise have to manually
	 *              pick up analog LOFT settings from best_coeffs and write to radio)
	 */
	uint16 cmds_RESTART[] = { 0x434, 0x334, 0x084, 0x267, 0x056, 0x234};
	uint16 cmds_REFINE[] = { 0x423, 0x334, 0x073, 0x267, 0x045, 0x234};
	uint16 cmds_LOPWR[] = {0x423, 0x334};

	uint16 cmds_RESTART_FDIQ[] = { 0x434, 0x334, 0x084, 0x267, 0x056, 0x234};
	uint16 cmds_REFINE_FDIQ[] =  { 0x423, 0x334, 0x073, 0x267, 0x045, 0x234};

#ifdef WLC_TXFDIQ
	/* using the depricated cal cmd 1 to indicate the start of FDIQ cal loop
	 */
	uint16 cmds_RESTART_80MHZ_FDIQ[] = { 0x434, 0x334, 0x084, 0x267, 0x056, 0x234, 0x156};
	uint16 cmds_REFINE_80MHZ_FDIQ[] =  { 0x423, 0x334, 0x073, 0x267, 0x045, 0x234, 0x156};

	/*  for 4364_3x3 fdiq cmds and  using the depricated cal cmd 1 to indicate the start */
	/*  of FDIQ cal loop */
	uint16 cmds_RESTART_fdiq_4364_3x3[] = { 0x434, 0x334, 0x084, 0x267, 0x056, 0x334,
	   0x234, 0x156};
	uint16 cmds_REFINE_fdiq_4364_3x3[] =  { 0x434, 0x334, 0x084, 0x267, 0x056, 0x334,
	   0x234, 0x156};
#endif
	/* if the LOFT digi coeff is negative , DAC output stays at random value for */
	/* one clock cycle after pkt ends (after tx reset) */

	/* so WAR for this is after IQ cal is finished, force digi coeff to 8,8 and then do */
	/* analog fine cal and then do digi cal with grid size of 8 , which will make sure */
	/* digi coeff is always non negative */

	/* ACPHY majorrev 47 */
	uint16 cmds_RESTART_majrev47[] = {0x265, 0x234, 0x084, 0x074, 0x056};
	uint16 cmds_REFINE_majrev47[] = {0x265, 0x234, 0x084, 0x074, 0x056};
	uint16 cmds_LOPWR_majrev47[] = {0x265, 0x234};

	/* txidx dependent digital loft comp table */
	uint16 cmds_RESTART_TDDLC[] = { 0x434, 0x334, 0x084, 0x267, 0x056, 0x234};
	uint16 cmds_REFINE_TDDLC[] = { 0x423, 0x334, 0x073, 0x267, 0x045, 0x234};

	/* Pre RX IQ cal coeffs */
	uint16 cmds_RESTART_PRERX[] = { 0x084, 0x056};
	uint16 cmds_REFINE_PRERX[] = { 0x073, 0x045};

	/* Reset LOFT comp coeffs before prerx cal */
	uint8 reset_loftcoefs_prerxcal = 0;

	uint16 *multilo_cal_idx_ptr = NULL;
	uint16 *multilo_start_idx_ptr = NULL;
	uint16 *multilo_end_idx_ptr = NULL;
	uint8 num_multilo = 0;

	uint16 multilo_cal_idx_majrev47[] = {35, 20};
	uint16 multilo_start_idx_majrev47[] = {35, 0};
	uint16 multilo_end_idx_majrev47[] = {127, 34};

	uint16 multilo_cal_idx_epa_majrev129[] = {55, 35};
	uint16 multilo_start_idx_epa_majrev129[] = {55, 0};
	uint16 multilo_end_idx_epa_majrev129[] = {127, 54};

	uint16 multilo_cal_idx_ipa_majrev129[] = {50, 45, 44, 40, 35, 28, 5};
	uint16 multilo_start_idx_ipa_majrev129[] = {48, 45, 43, 39, 34, 24, 0};
	uint16 multilo_end_idx_ipa_majrev129[] = {127, 47, 44, 42, 38, 33, 23};

	if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) &&
			(RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) &&
			!(ACRADIO_2069_EPA_IS(pi->pubpi->radiorev))) {

		if (CHSPEC_IS2G(pi->radio_chanspec) == 1) {
			nsamp_gctrl[0] = 0x87; nsamp_gctrl[1] = 0x77; nsamp_gctrl[2] = 0x77;
			nsamp_corrs[0] = 0x79; nsamp_corrs[1] = 0x79; nsamp_corrs[2] = 0x79;
		} else {
			nsamp_gctrl[0] = 0x78; nsamp_gctrl[1] = 0x88; nsamp_gctrl[2] = 0x98;
			nsamp_corrs[0] = 0x89; nsamp_corrs[1] = 0x79; nsamp_corrs[2] = 0x79;
		}

		thres_ladder[0] = 0x3d; thres_ladder[1] = 0x2d; thres_ladder[2] = 0x1d;
		thres_ladder[3] = 0x0d; thres_ladder[4] = 0x07; thres_ladder[5] = 0x03;
		thres_ladder[6] = 0x01;

		thresh_ladder_len = 7;

	} else {
		nsamp_gctrl[0] = 0x76; nsamp_gctrl[1] = 0x87; nsamp_gctrl[2] = 0x98;
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			nsamp_corrs[0] = 0x7D; nsamp_corrs[1] = 0x7E; nsamp_corrs[2] = 0x7E;
		} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
			nsamp_gctrl[0] = 0xA7; nsamp_gctrl[1] = 0xB7;
			nsamp_gctrl[2] = 0xC7; nsamp_gctrl[3] = 0xC7;
			nsamp_corrs[0] = 0x7B; nsamp_corrs[1] = 0x7B;
			nsamp_corrs[2] = 0x7B; nsamp_corrs[3] = 0x7B;
		} else {
			nsamp_corrs[0] = 0x79; nsamp_corrs[1] = 0x79; nsamp_corrs[2] = 0x79;
		}
		/* for 4349-ePA, to improve performance, we choosed to increase no of samples
		 * to correlate
		 */
		nsamp_corrs_new[0] = 0x79; nsamp_corrs_new[1] = 0x7B; nsamp_corrs_new[2] = 0x79;
		nsamp_corrs_new[3] = 0x7B; nsamp_corrs_new[4] = 0x79; nsamp_corrs_new[5] = 0x7B;
		nsamp_corrs_new[6] = 0x79; nsamp_corrs_new[7] = 0x7B;

		if (TINY_RADIO(pi)) {
			thres_ladder[0] = 0x3d; thres_ladder[1] = 0x2d; thres_ladder[2] = 0x1d;
			thres_ladder[3] = 0x0d; thres_ladder[4] = 0x07;
			thres_ladder[5] = 0x03; thres_ladder[6] = 0x01;
			thresh_ladder_len = 7;

			if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
				if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) && !PHY_IPA(pi)) {
					cmds_RESTART[0] = 0x264; cmds_RESTART[1] = 0x233;
					cmds_RESTART[2] = 0x085; cmds_RESTART[3] = 0x044;

					cmds_REFINE[0] = 0x264; cmds_REFINE[1] = 0x233;
					cmds_REFINE[2] = 0x085; cmds_REFINE[3] = 0x044;
				} else {
					cmds_RESTART[0] = 0x265; cmds_RESTART[1] = 0x234;
					cmds_RESTART[2] = 0x084; cmds_RESTART[3] = 0x074;
					cmds_RESTART[4] = 0x056;

					cmds_REFINE[0] = 0x265; cmds_REFINE[1] = 0x234;
					cmds_REFINE[2] = 0x084; cmds_REFINE[3] = 0x074;
					cmds_REFINE[4] = 0x056;
				}
			} else {
				cmds_RESTART[0] = 0x265; cmds_RESTART[1] = 0x234;
				cmds_RESTART[2] = 0x084; cmds_RESTART[3] = 0x074;
				cmds_RESTART[4] = 0x056;

				cmds_REFINE[0] = 0x265; cmds_REFINE[1] = 0x234;
				cmds_REFINE[2] = 0x084; cmds_REFINE[3] = 0x074;
				cmds_REFINE[4] = 0x056;

				if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
					ACMAJORREV_33(pi->pubpi->phy_rev)) {
					/* using the depricated cal cmd 1
					 *  to indicate the start of FDIQ cal loop
					 */
					cmds_RESTART_FDIQ[0] = 0x265; cmds_RESTART_FDIQ[1] = 0x234;
					cmds_RESTART_FDIQ[2] = 0x084; cmds_RESTART_FDIQ[3] = 0x074;
					cmds_RESTART_FDIQ[4] = 0x056; cmds_RESTART_FDIQ[5] = 0x156;

					cmds_REFINE_FDIQ[0] = 0x265; cmds_REFINE_FDIQ[1] = 0x234;
					cmds_REFINE_FDIQ[2] = 0x084; cmds_REFINE_FDIQ[3] = 0x074;
					cmds_REFINE_FDIQ[4] = 0x056; cmds_REFINE_FDIQ[5] = 0x156;
				}
			}
		} else {
			if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
				thres_ladder[0] = 0xff; thres_ladder[1] = 0x3d;
				thres_ladder[2] = 0x2d; thres_ladder[3] = 0x1d;
				thres_ladder[4] = 0x0d; thres_ladder[5] = 0x07;
				thres_ladder[6] = 0x03;
				thresh_ladder_len = 7;
			} else {
				thres_ladder[0] = 0x3d; thres_ladder[1] = 0x1e;
				thres_ladder[2] = 0xf; thres_ladder[3] = 0x07;
				thres_ladder[4] = 0x03; thres_ladder[5] = 0x01;
				thresh_ladder_len = 6;
			}

			cmds_REFINE[0] =  0x423; cmds_REFINE[1] = 0x334;
			cmds_REFINE[2] = 0x073;
			cmds_REFINE[3] =  0x267; cmds_REFINE[4] = 0x045;
			cmds_REFINE[5] = 0x234;
		}
	}

	/* Default command sequences */
	cmd_restart_ptr = cmds_RESTART;
	num_cmds_restart = ARRAYSIZE(cmds_RESTART);
	cmd_refine_ptr  = cmds_REFINE;
	num_cmds_refine  = ARRAYSIZE(cmds_REFINE);
	cmd_lopwr_ptr  = cmds_LOPWR;
	num_cmds_lopwr  = ARRAYSIZE(cmds_LOPWR);
	cmd_restart_prerx_ptr = cmds_RESTART_PRERX;
	num_cmds_restart_prerx = ARRAYSIZE(cmds_RESTART_PRERX);
	cmd_refine_prerx_ptr  = cmds_REFINE_PRERX;
	num_cmds_refine_prerx  = ARRAYSIZE(cmds_REFINE_PRERX);

	/* Rev dependent overrides for command sequences */
	if (ACREV_IS(pi->pubpi->phy_rev, 1) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
		cmd_refine_ptr = cmds_REFINE_TDDLC;
		num_cmds_refine = ARRAYSIZE(cmds_REFINE_TDDLC);
		cmd_restart_ptr = cmds_RESTART_TDDLC;
		num_cmds_restart = ARRAYSIZE(cmds_RESTART_TDDLC);
	} else if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev)) {
		cmd_restart_ptr = cmds_RESTART;
		cmd_refine_ptr  = cmds_REFINE;
		/* numb of commands in tiny are 5 only */
		if (ACMINORREV_2(pi) && (CHSPEC_ISPHY5G6G(pi->radio_chanspec))) {
			num_cmds_restart = 4;
			num_cmds_refine  = 4;
		} else {
			num_cmds_restart = 5;
			num_cmds_refine  = 5;
		}

		if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev)) {
			cmd_restart_fdiq_ptr = cmds_RESTART_FDIQ;
			cmd_refine_fdiq_ptr = cmds_REFINE_FDIQ;
			if (ACMINORREV_2(pi) && (CHSPEC_ISPHY5G6G(pi->radio_chanspec))) {
				num_cmds_restart_fdiq = 4;
				num_cmds_refine_fdiq  = 4;
			} else {
				num_cmds_restart_fdiq = 5;
				num_cmds_refine_fdiq  = 5;
			}
		}
	} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		cmd_restart_ptr = cmds_RESTART_majrev47;
		num_cmds_restart = ARRAYSIZE(cmds_RESTART_majrev47);
		cmd_refine_ptr = cmds_REFINE_majrev47;
		num_cmds_refine = ARRAYSIZE(cmds_REFINE_majrev47);
		cmd_lopwr_ptr = cmds_LOPWR_majrev47;
		num_cmds_lopwr = ARRAYSIZE(cmds_LOPWR_majrev47);
		cmd_restart_prerx_ptr = cmd_restart_ptr;
		num_cmds_restart_prerx = num_cmds_restart;
		cmd_refine_prerx_ptr = cmd_restart_ptr;
		num_cmds_refine_prerx = num_cmds_restart;
		if (ACMAJORREV_51_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) {
			reset_loftcoefs_prerxcal = 1;
		}
	} else {
		cmd_refine_ptr = cmds_REFINE;
		num_cmds_refine = ARRAYSIZE(cmds_REFINE);
		cmd_restart_ptr = cmds_RESTART;
		num_cmds_restart = ARRAYSIZE(cmds_RESTART);
#ifdef WLC_TXFDIQ
		  /* Restricting FDIQ cal to 80Mhz.
		   */
		  if ((iqcal_info->txfdiqi->enabled == 1)) {
		    cmd_restart_ptr = cmds_RESTART_80MHZ_FDIQ;
		    num_cmds_restart = ARRAYSIZE(cmds_RESTART_80MHZ_FDIQ);
		    cmd_refine_ptr = cmds_REFINE_80MHZ_FDIQ;
		    num_cmds_refine = ARRAYSIZE(cmds_REFINE_80MHZ_FDIQ);
		  }

		  /* 4364_3x3 FDIQ cal for 20/40/80Mhz */
		  if ((iqcal_info->txfdiqi->enabled == 1) && IS_4364_3x3(pi)) {
			cmd_restart_ptr = cmds_RESTART_fdiq_4364_3x3;
			num_cmds_restart = ARRAYSIZE(cmds_RESTART_fdiq_4364_3x3);
			cmd_refine_ptr = cmds_REFINE_fdiq_4364_3x3;
			num_cmds_refine = ARRAYSIZE(cmds_REFINE_fdiq_4364_3x3);
		  }

#endif /* fdiq loop */
	}

	/* update params */
	params->num_cmds_refine  = num_cmds_refine;
	params->num_cmds_restart = num_cmds_restart;
	params->num_cmds_lopwr = num_cmds_lopwr;
	params->num_cmds_refine_prerx  = num_cmds_refine_prerx;
	params->num_cmds_restart_prerx = num_cmds_restart_prerx;
	params->num_cmds_refine_fdiq  = num_cmds_refine_fdiq;
	params->num_cmds_restart_fdiq = num_cmds_restart_fdiq;
	params->tone_freq        = 4000;
	params->reset_loftcoefs_prerxcal = reset_loftcoefs_prerxcal;

	memcpy(params->target_tssi_set, target_tssi_set, sizeof(target_tssi_set));

	params->start_casc = 64;
	params->stop_casc = 1;
	params->step_casc = 32;
	params->start_idx = 0;
	params->stop_idx = 100;
	params->step_idx = 50;
	params->delta_tssi_error = 10;
	params->lower_lim = 0;
	params->initindex = 0;
	params->casc_gctrl = FALSE;

	if (ACMAJORREV_47_130(pi->pubpi->phy_rev)) {
		multilo_cal_idx_ptr = multilo_cal_idx_majrev47;
		multilo_start_idx_ptr = multilo_start_idx_majrev47;
		multilo_end_idx_ptr = multilo_end_idx_majrev47;
		num_multilo = ARRAYSIZE(multilo_cal_idx_majrev47);
	} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
		if (PHY_IPA(pi)) {
			multilo_cal_idx_ptr = multilo_cal_idx_ipa_majrev129;
			multilo_start_idx_ptr = multilo_start_idx_ipa_majrev129;
			multilo_end_idx_ptr = multilo_end_idx_ipa_majrev129;
			num_multilo = ARRAYSIZE(multilo_cal_idx_ipa_majrev129);
		} else {
			multilo_cal_idx_ptr = multilo_cal_idx_epa_majrev129;
			multilo_start_idx_ptr = multilo_start_idx_epa_majrev129;
			multilo_end_idx_ptr = multilo_end_idx_epa_majrev129;
			num_multilo = ARRAYSIZE(multilo_cal_idx_epa_majrev129);
		}
	}
	params->num_multilo = num_multilo;

	memcpy(params->multilo_cal_idx, multilo_cal_idx_ptr, sizeof(uint16)*num_multilo);
	memcpy(params->multilo_start_idx, multilo_start_idx_ptr, sizeof(uint16)*num_multilo);
	memcpy(params->multilo_end_idx, multilo_end_idx_ptr, sizeof(uint16)*num_multilo);

	memcpy(params->cmds_REFINE, cmd_refine_ptr, sizeof(uint16)*num_cmds_refine);
	memcpy(params->cmds_RESTART, cmd_restart_ptr, sizeof(uint16)*num_cmds_restart);
	memcpy(params->cmds_LOPWR, cmd_lopwr_ptr, sizeof(uint16)*num_cmds_lopwr);

	memcpy(params->cmds_REFINE_FDIQ, cmd_refine_fdiq_ptr, sizeof(uint16)*num_cmds_refine_fdiq);
	memcpy(params->cmds_RESTART_FDIQ, cmd_restart_fdiq_ptr,
		sizeof(uint16)*num_cmds_restart_fdiq);

	memcpy(params->cmds_REFINE_PRERX, cmd_refine_prerx_ptr,
		sizeof(uint16)*num_cmds_refine_prerx);
	memcpy(params->cmds_RESTART_PRERX, cmd_restart_prerx_ptr,
		sizeof(uint16)*num_cmds_restart_prerx);

	memcpy(params->nsamp_gctrl, nsamp_gctrl, sizeof(nsamp_gctrl));
	memcpy(params->nsamp_corrs, nsamp_corrs, sizeof(nsamp_corrs));
	memcpy(params->nsamp_corrs_new, nsamp_corrs_new, sizeof(nsamp_corrs_new));

	memcpy(params->thres_ladder, thres_ladder, thresh_ladder_len);

	return params;
}
static void wlc_acphy_get_tx_iqcc(phy_info_t *pi, uint16 *a, uint16 *b);
static void wlc_acphy_set_tx_iqcc(phy_info_t *pi, uint16 a, uint16 b);

static void phy_ac_txiqlocal_txiqccget(phy_type_txiqlocal_ctx_t *ctx, void *a);
static void phy_ac_txiqlocal_txiqccset(phy_type_txiqlocal_ctx_t *ctx, void *b);
static void phy_ac_txiqlocal_txloccget(phy_type_txiqlocal_ctx_t *ctx, void *a);
static void phy_ac_txiqlocal_txloccset(phy_type_txiqlocal_ctx_t *ctx, void *b);

#if defined(WLTEST)
static int phy_ac_txiqlocal_get_calidx(phy_type_txiqlocal_ctx_t *ctx, int32* calidx);
static int phy_ac_txiqlocal_set_calidx(phy_type_txiqlocal_ctx_t *ctx, int32 calidx);
static int phy_ac_txiqlocal_get_target_tssi(phy_type_txiqlocal_ctx_t *ctx, int32 *target_tssi);
static int phy_ac_txiqlocal_set_target_tssi(phy_type_txiqlocal_ctx_t *ctx, int32 target_tssi);
static int phy_ac_txiqlocal_get_tssi_search_enable(phy_type_txiqlocal_ctx_t *ctx,
		int32 *tssi_search_en);
static int phy_ac_txiqlocal_set_tssi_search_enable(phy_type_txiqlocal_ctx_t *ctx,
		int32 tssi_search_en);
#endif

/* local functions */
static void phy_ac_txiqlocal_nvram_attach(phy_ac_txiqlocal_info_t *ti);

/* register phy type specific implementation */
phy_ac_txiqlocal_info_t *
BCMATTACHFN(phy_ac_txiqlocal_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_txiqlocal_info_t *cmn_info)
{
	phy_ac_txiqlocal_info_t *ac_info;
	phy_type_txiqlocal_fns_t fns;
	phy_param_info_t *phy_param = NULL;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ac_info = phy_malloc(pi, sizeof(phy_ac_txiqlocal_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if ((ac_info->ac_txcal_radioregs_orig =
			phy_malloc(pi, sizeof(acphy_txcal_radioregs_t))) == NULL) {
		PHY_ERROR(("%s: ac_txcal_radioregs_orig malloc failed\n", __FUNCTION__));
		goto fail;
	}
#ifdef WLC_TXFDIQ
	if ((ac_info->txfdiqi = phy_malloc(pi, sizeof(acphy_tx_fdiqi_ctl_t))) == NULL) {
		PHY_ERROR(("%s: txfdiqi malloc failed\n", __FUNCTION__));
		goto fail;
	}
#endif
	if ((ac_info->txcal_cache = phy_malloc(pi, sizeof(txcal_coeffs_t[PHY_CORE_MAX]))) == NULL) {
		PHY_ERROR(("%s: txcal_cache malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if ((phy_param = phy_malloc(pi, sizeof(phy_param_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_param malloc failed\n", __FUNCTION__));
		goto fail;
	}

	pi->u.pi_acphy->paramsi = phy_param;

	/* initialize ptrs */
	ac_info->pi = pi;
	ac_info->aci = aci;
	ac_info->cmn_info = cmn_info;
	ac_info->txcal_cache_cookie = 0;

	if ((ac_info->paramsi = phy_malloc(pi, sizeof(txiqcal_params_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc txiqcal_params_t failed\n", __FUNCTION__));
		goto fail;
	}
	phy_ac_txiqlocal_populate_params(ac_info);
	/* allocate phyreg save restore bin */
	if ((ac_info->paramsi->porig =
			phy_malloc(pi, sizeof(txiqcal_phyregs_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc txiqcal_phyregs_t failed\n", __FUNCTION__));
		goto fail;
	}

	/* Read srom params from nvram */
	phy_ac_txiqlocal_nvram_attach(ac_info);

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = ac_info;
	fns.txiqccget = phy_ac_txiqlocal_txiqccget;
	fns.txiqccset = phy_ac_txiqlocal_txiqccset;
	fns.txloccget = phy_ac_txiqlocal_txloccget;
	fns.txloccset = phy_ac_txiqlocal_txloccset;
#if defined(WLTEST)
	fns.get_calidx = phy_ac_txiqlocal_get_calidx;
	fns.set_calidx = phy_ac_txiqlocal_set_calidx;
	fns.get_target_tssi = phy_ac_txiqlocal_get_target_tssi;
	fns.set_target_tssi = phy_ac_txiqlocal_set_target_tssi;
	fns.get_tssi_search_enable = phy_ac_txiqlocal_get_tssi_search_enable;
	fns.set_tssi_search_enable = phy_ac_txiqlocal_set_tssi_search_enable;
#endif

#if !defined(PHYCAL_CACHING)
	fns.scanroam_cache = wlc_phy_scanroam_cache_txcal_acphy;
#endif

	if (phy_txiqlocal_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_txiqlocal_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ac_info;

	/* error handling */
fail:

	if (phy_param != NULL)
		phy_mfree(pi, phy_param, sizeof(phy_param_info_t));
	phy_ac_txiqlocal_unregister_impl(ac_info);
	return NULL;
}

void
BCMATTACHFN(phy_ac_txiqlocal_unregister_impl)(phy_ac_txiqlocal_info_t *ac_info)
{
	phy_txiqlocal_info_t *cmn_info;
	phy_info_t *pi;

	if (ac_info == NULL)
		return;

	pi = ac_info->pi;
	cmn_info = ac_info->cmn_info;

	PHY_CAL(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_txiqlocal_unregister_impl(cmn_info);

	if (ac_info->paramsi) {
		if (ac_info->paramsi->porig) {
			phy_mfree(pi, ac_info->paramsi->porig, sizeof(txiqcal_phyregs_t));
		}
		phy_mfree(pi, ac_info->paramsi, sizeof(txiqcal_params_t));
	}

	if (ac_info->txcal_cache != NULL) {
		phy_mfree(pi, ac_info->txcal_cache, sizeof(txcal_coeffs_t[PHY_CORE_MAX]));
	}
#ifdef WLC_TXFDIQ
	if (ac_info->txfdiqi != NULL) {
		phy_mfree(pi, ac_info->txfdiqi, sizeof(acphy_tx_fdiqi_ctl_t));
	}
#endif
	if (ac_info->ac_txcal_radioregs_orig != NULL) {
		phy_mfree(pi, ac_info->ac_txcal_radioregs_orig, sizeof(acphy_txcal_radioregs_t));
	}

	phy_mfree(pi, ac_info, sizeof(phy_ac_txiqlocal_info_t));
}

int8
phy_ac_txiqlocal_get_fdiqi_slope(phy_ac_txiqlocal_info_t *txiqlocali, uint8 core)
{
	return txiqlocali->txfdiqi->slope[core];
}

void
phy_ac_txiqlocal_set_fdiqi_slope(phy_ac_txiqlocal_info_t *txiqlocali, uint8 core, int8 slope)
{
	txiqlocali->txfdiqi->slope[core] = slope;
}

bool
phy_ac_txiqlocal_is_fdiqi_enabled(phy_ac_txiqlocal_info_t *txiqlocali)
{
	return txiqlocali->txfdiqi->enabled;
}

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */
#define CAL_TYPE_IQ                 0
#define CAL_TYPE_LOFT_DIG           2
#define CAL_TYPE_LOFT_ANA_FINE      3
#define CAL_TYPE_LOFT_ANA_COARSE    4

#define MAX_PAD_GAIN				0xFF

#define MPHASE_TXCAL_CMDS_PER_PHASE  2 /* number of tx iqlo cal commands per phase in mphase cal */

#define WLC_PHY_PRECAL_TRACE(tx_idx, target_gains) \
	PHY_TRACE(("Index was found to be %d\n", tx_idx)); \
	PHY_TRACE(("Gain Code was found to be : \n")); \
	PHY_TRACE(("radio gain = 0x%x%x%x, bbm=%d, dacgn = %d  \n", \
		target_gains->rad_gain_hi, \
		target_gains->rad_gain_mi, \
		target_gains->rad_gain, \
		target_gains->bbmult, \
		target_gains->dac_gain))
#define WLC_PHY_PRECAL_TRACE_PERCORE(tx_idx, target_gains, core) \
	PHY_TRACE(("Index was found to be %d\n", tx_idx)); \
	PHY_TRACE(("Gain Code was found to be : \n")); \
	PHY_TRACE(("rad_gain_hi: 0x%x, rad_gain_mi: 0x%x, rad_gain: 0x%x, bbm=%d, dacgn = %d  \n", \
		target_gains[core].rad_gain_hi, \
		target_gains[core].rad_gain_mi, \
		target_gains[core].rad_gain, \
		target_gains[core].bbmult, \
		target_gains[core].dac_gain))
#define WLC_PHY_PRECAL_TRACE_PERCORE_CASC(casc, target_gains, core) \
	PHY_TRACE(("cascode gain was found to be %d\n", casc)); \
	PHY_TRACE(("Gain Code was found to be : \n")); \
	PHY_TRACE(("rad_gain_hi: 0x%x, rad_gain_mi: 0x%x, rad_gain: 0x%x, bbm=%d, dacgn = %d  \n", \
		target_gains[core].rad_gain_hi, \
		target_gains[core].rad_gain_mi, \
		target_gains[core].rad_gain, \
		target_gains[core].bbmult, \
		target_gains[core].dac_gain))
typedef struct {
	uint8 percent;
	uint8 g_env;
} acphy_txiqcal_ladder_t;

typedef struct {
	uint8 nwords;
	uint8 offs;
	uint8 boffs;
} acphy_coeff_access_t;

typedef struct {
	acphy_txgains_t gains;
	bool useindex;
	uint8 index;
} acphy_ipa_txcalgains_t;

#define TXFILT_SHAPING_OFDM20   0
#define TXFILT_SHAPING_OFDM40   1
#define TXFILT_SHAPING_CCK      2
#define TXFILT_DEFAULT_OFDM20   3
#define TXFILT_DEFAULT_OFDM40   4

#define LPFCONF_TXIQ_RX2 0
#define LPFCONF_TXIQ_RX4 1

typedef struct acphy_papd_restore_state_t {
	uint16 fbmix[2];
	uint16 vga_master[2];
	uint16 intpa_master[2];
	uint16 afectrl[2];
	uint16 afeoverride[2];
	uint16 pwrup[2];
	uint16 atten[2];
	uint16 mm;
	uint16 tr2g_config1;
	uint16 tr2g_config1_core[2];
	uint16 tr2g_config4_core[2];
	uint16 reg10;
	uint16 reg20;
	uint16 reg21;
	uint16 reg29;
} acphy_papd_restore_state;

typedef struct _acphy_ipa_txrxgain {
	uint16 hpvga;
	uint16 lpf_biq1;
	uint16 lpf_biq0;
	uint16 lna2;
	uint16 lna1;
	int8 txpwrindex;
} acphy_ipa_txrxgain_t;

/* The following are the txiqcc offsets in the ACPHY_TBL_ID_IQLOCAL table, from acphyprocs.tcl */
static const uint8 tbl_offset_ofdm_a[] = {96, 100, 104, 108};
static const uint8 tbl_offset_bphy_a[] = {112, 116, 120, 124};

/* The following are the txlocc offsets in the ACPHY_TBL_ID_IQLOCAL table, from acphyprocs.tcl */
static const uint8 tbl_offset_ofdm_d[] = {98, 102, 106, 110};
static const uint8 tbl_offset_bphy_d[] = {114, 118, 122, 126};

static void wlc_phy_precal_target_tssi_search(phy_info_t *pi, txgain_setting_t *target_gains);
static void wlc_phy_precal_target_tssi_fine_tune(phy_info_t *pi, uint8 core,
		txgain_setting_t *target_gains);
static void wlc_phy_txcal_radio_setup_acphy_tiny(phy_info_t *pi);
static void wlc_phy_txcal_radio_setup_acphy_20698(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp);
static void wlc_phy_txcal_radio_setup_acphy_20704(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp);
static void wlc_phy_txcal_radio_setup_acphy_20707(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp);
static void wlc_phy_txcal_radio_setup_acphy_20708(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp);
static void wlc_phy_txcal_radio_setup_acphy_20709(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp);
static void wlc_phy_txcal_radio_setup_acphy_20710(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp);
static void wlc_phy_txcal_radio_setup_acphy(phy_info_t *pi);
static void wlc_phy_txcal_phy_setup_acphy(phy_info_t *pi, uint8 Biq2byp);
static void wlc_phy_cal_txiqlo_update_ladder_acphy(phy_info_t *pi, uint16 bbmult, uint8 core);
static uint16 wlc_poll_adc_clamp_status(phy_info_t *pi, uint8 core, uint8 do_reset);
static void wlc_phy_txcal_phy_cleanup_acphy(phy_info_t *pi);
static void wlc_phy_txcal_radio_cleanup_acphy_tiny(phy_info_t *pi);
static void wlc_phy_txcal_radio_cleanup_acphy_28nm(phy_ac_txiqlocal_info_t *ti);
static void wlc_phy_txcal_radio_cleanup_acphy(phy_info_t *pi);
static void wlc_phy_precal_txgain_control(phy_info_t *pi, txgain_setting_t *target_gains);
static void wlc_txprecal4349_gain_control(phy_info_t *pi, txgain_setting_t *target_gains);
static void wlc_phy_txcal_phy_setup_acphy_core(phy_info_t *pi, txiqcal_phyregs_t *porig,
	uint8 core, uint16 bw_idx, uint16 sdadc_config, uint8 Biq2byp);
static void wlc_phy_txcal_phy_setup_acphy_core_disable_rf(phy_info_t *pi, uint8 core);
static void wlc_phy_txcal_phy_setup_acphy_core_loopback_path(phy_info_t *pi, uint8 core,
	uint8 lpf_config);
static void wlc_phy_txcal_phy_setup_acphy_core_lpf(phy_info_t *pi, uint8 core, uint16 bw_idx);
static void wlc_phy_populate_tx_loft_comp_tbl_acphy(phy_info_t *pi, uint16 *loft_coeffs);
static void wlc_phy_poll_adc_acphy(phy_info_t *pi, int32 *adc_buf, uint8 nsamps,
	bool switch_gpiosel, uint16 core, bool is_tssi);
void wlc_phy_poll_samps_acphy(phy_info_t *pi, int16 *samp, bool is_tssi,
	uint8 log2_nsamps, bool init_adc_inside,
	uint16 core);
#ifdef WLC_TXFDIQ
static void phy_ac_txiqlocal_fdiqi_lin_reg(phy_ac_txiqlocal_info_t *ti, acphy_fdiqi_t *freq_ang_mag,
                                        uint16 num_data, int fdiq_data_valid);
#endif

static void
wlc_phy_precal_target_tssi_fine_tune(phy_info_t *pi, uint8 core, txgain_setting_t *target_gains)
{
	int16  target_tssi;
	int16  idle_tssi[PHY_CORE_MAX] = {0};
	int8   tx_idx, tx_idx0;
	int8   tx_idx_min, tx_idx_max;
	int16  tone_tssi = 0;
	int16  tssi[PHY_CORE_MAX] = {0};
	int16 tx_idx_step, delta_tssi;
	phy_ac_info_t *pi_ac = pi->u.pi_acphy;
	phy_ac_txiqlocal_info_t *ti = pi_ac->txiqlocali;

	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	if (core == 0) ti->txiqcalidx_iovar = 0;

	if (ti->txiqcal_target_tssi_iovar_set) {
		target_tssi = (ti->txiqcal_target_tssi_iovar % 1000);
	} else {
		target_tssi = 200;
		if (PHY_IPA(pi)) {
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				ti->txiqcal_target_tssi_iovar = 250;
				target_tssi = 250;
			} else {
				ti->txiqcal_target_tssi_iovar = 175;
				target_tssi = 175;
			}
		} else {
			ti->txiqcal_target_tssi_iovar = 500;
			target_tssi = 500;
		}
	}

	PHY_TRACE(("Target_tssi is set to: %d\n", target_tssi));

	phy_ac_tssi_loopback_path_setup(pi, LOOPBACK_FOR_TSSICAL);

	/* Measure the Idle TSSI */
	wlc_phy_poll_samps_WAR_acphy(pi, idle_tssi, TRUE, TRUE, target_gains, FALSE, TRUE, core, 0);

	/* Measure the tone TSSI before start searching */
	if (PHY_IPA(pi)) {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			tx_idx0 = 35;
		} else {
			tx_idx0 = 40;
		}
	} else {
		tx_idx0 = 20;
	}
	tx_idx = tx_idx0;
	tx_idx_max = tx_idx0 + 10;
	tx_idx_min = tx_idx0 - 5;

	wlc_phy_get_txgain_settings_by_index_acphy(
				pi, target_gains, tx_idx);

	target_gains->bbmult = 64;
	PHY_TRACE(("radio gain = 0x%x%x%x, bbm=%d, dacgn = %d  \n",
		target_gains->rad_gain_hi,
		target_gains->rad_gain_mi,
		target_gains->rad_gain,
		target_gains->bbmult,
		target_gains->dac_gain));

	wlc_phy_poll_samps_WAR_acphy(pi, tssi, TRUE, FALSE, target_gains, FALSE, TRUE, core, 0);

	tone_tssi = tssi[core] - idle_tssi[core];

	delta_tssi = tone_tssi - target_tssi;

	PHY_TRACE(("Initial: Core = %d Index = %3d target_TSSI = %4i tone_TSSI = %4i"
			"delta_TSSI = %4i, TSSI = %4i, IDLE_TSSI = %4i\n", core,
			tx_idx, target_tssi, tone_tssi, delta_tssi, tssi[core], idle_tssi[core]));

	PHY_TRACE(("*********** Search Control loop begins now ***********\n"));

	tx_idx_step = 1;

	/* exiting from loop when the delta_tssi is less than zero or the idx goes to high */
	while ((delta_tssi > 0) && (tx_idx < (tx_idx_max))) {
		tx_idx = tx_idx + tx_idx_step;

		wlc_phy_get_txgain_settings_by_index_acphy(
				pi, target_gains, tx_idx);
		target_gains->bbmult = 64;
		wlc_phy_poll_samps_WAR_acphy(pi, tssi, TRUE,
				FALSE, target_gains, FALSE, TRUE, core, 0);
		tone_tssi = tssi[core] - idle_tssi[core];
		delta_tssi = tone_tssi - target_tssi;

		PHY_TRACE(("Core = %d Index = %3d target_TSSI = %4i tone_TSSI = %4i"
			"delta_TSSI = %4i\n", core,
			tx_idx, target_tssi, tone_tssi, delta_tssi));

	}
	/* Making sure the idx is not too high, decrease it if more tan 20 below target. */
	while ((delta_tssi < -20) && (tx_idx > (tx_idx_min))) {
		tx_idx = tx_idx - tx_idx_step;

		wlc_phy_get_txgain_settings_by_index_acphy(
				pi, target_gains, tx_idx);
		target_gains->bbmult = 64;
		wlc_phy_poll_samps_WAR_acphy(pi, tssi, TRUE,
				FALSE, target_gains, FALSE, TRUE, core, 0);
		tone_tssi = tssi[core] - idle_tssi[core];
		delta_tssi = tone_tssi - target_tssi;

		PHY_TRACE(("Core = %d Index = %3d target_TSSI = %4i tone_TSSI = %4i"
			"delta_TSSI = %4i\n", core,
			tx_idx, target_tssi, tone_tssi, delta_tssi));

	}

	ti->txiqcalidx_iovar = (ti->txiqcalidx_iovar*100)+tx_idx;

	wlc_phy_get_txgain_settings_by_index_acphy(
			pi, target_gains, tx_idx);
	target_gains->bbmult = 64;

	/* Restore the original Gain code */
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);

	return;
}

static void
wlc_phy_precal_target_tssi_search(phy_info_t *pi, txgain_setting_t *target_gains)
{
	int8  gain_code_found, delta_threshold, dont_alter_step;
	int16  target_tssi, min_delta, prev_delta, delta_tssi;
	int16  idle_tssi[PHY_CORE_MAX] = {0};
	uint8  tx_idx;
	int16  tone_tssi[PHY_CORE_MAX] = {0};
	int16  tssi[PHY_CORE_MAX] = {0};

	int16  pad_gain_step, curr_pad_gain, pad_gain;

	txgain_setting_t orig_txgain[PHY_CORE_MAX];
	int16  sat_count, sat_threshold, sat_delta, ct;
	int16 temp_val;
	int16 tx_idx_step, pad_step_size, pad_iteration_count;

	/* prevent crs trigger */
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

	/* Set the target TSSIs for different bands/Bandwidth cases.
	 * These numbers are arrived by running the TCL proc:
	 * "get_target_tssi_for_iqlocal" for a representative channel
	 * by sending a tone at a chosen Tx gain which gives best
	 * Image/LO rejection at room temp
	 */

	if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) == 1) {
		if (CHSPEC_IS80(pi->radio_chanspec) ||
			PHY_AS_80P80(pi, pi->radio_chanspec)) {
			target_tssi = 900;
		} else if (CHSPEC_IS160(pi->radio_chanspec)) {
			target_tssi = 900;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			target_tssi = 900;
		} else {
			target_tssi = 950;
		}
	} else {
		if (CHSPEC_IS40(pi->radio_chanspec)) {
			target_tssi = 913;
		} else {
			target_tssi = 950;
		}

	}

	phy_ac_tssi_loopback_path_setup(pi, LOOPBACK_FOR_TSSICAL);

	gain_code_found = 0;

	/* delta_threshold is the minimum tolerable difference between
	 * target tssi and the measured tssi. This was determined by experimental
	 * observations. delta_tssi ( target_tssi - measured_tssi ) values upto
	 * 15 are found to give identical performance in terms of Tx EVM floor
	 * when compared to delta_tssi values upto 10. Threshold value of 15 instead
	 * of 10 will cut down the algo time as the algo need not search for
	 * index to meet delta of 10.
	 */
	delta_threshold = 15;

	min_delta = 1024;
	prev_delta = 1024;

	/* Measure the Idle TSSI */
	wlc_phy_poll_samps_WAR_acphy(pi, idle_tssi, TRUE, TRUE, target_gains, FALSE, TRUE, 0, 0);

	/* Measure the tone TSSI before start searching */
	tx_idx = 0;
	wlc_phy_txpwr_by_index_acphy(pi, 1, tx_idx);

	wlc_phy_get_txgain_settings_by_index_acphy(
				pi, target_gains, tx_idx);

	/* Save the original Gain code */
	wlc_phy_txcal_txgain_setup_acphy(pi, target_gains, &orig_txgain[0]);

	PHY_TRACE(("radio gain = 0x%x%x%x, bbm=%d, dacgn = %d  \n",
		target_gains->rad_gain_hi,
		target_gains->rad_gain_mi,
		target_gains->rad_gain,
		target_gains->bbmult,
		target_gains->dac_gain));

	wlc_phy_poll_samps_WAR_acphy(pi, tssi, TRUE, FALSE, target_gains, FALSE, TRUE, 0, 0);

	tone_tssi[0] = tssi[0] - idle_tssi[0];

	delta_tssi = target_tssi - tone_tssi[0];

	PHY_TRACE(("Index = %3d target_TSSI = %4i tone_TSSI = %4i"
			"delta_TSSI = %4i min_delta = %4i\n",
			tx_idx, target_tssi, tone_tssi[0], delta_tssi, min_delta));

	PHY_TRACE(("*********** Search Control loop begins now ***********\n"));

	/* When the measured tssi saturates and is unable to meet
	 * the target tssi, there is no point in continuing search
	 * for the next higher PAD gain. The variable 'sat_count'
	 * is the threshold which will control when to stop the search.
	 * change in PAD gain code by "10" ticks should atleast translate
	 * to 1dBm of power level change when not saturated. When the
	 * measured tssi is saturated, this doesnt hold good and we
	 * need to break out.
	 */
	sat_count = 10;
	sat_threshold = 20;

	/* delta_tssi > 0 ==> target_tssi is greater than tone tssi and
	 * hence we have to increase the PAD gain as the inference was
	 * drawn by measuring the tone tssi at index 0
	 */

	if (delta_tssi > 0) {
		PHY_TRACE(("delta_tssi > 0 ==> target_tssi is greater than tone tssi and\n"));
		PHY_TRACE(("hence we have to increase the PAD gain as the inference was\n"));
		PHY_TRACE(("drawn by measuring the tone tssi at index 0\n"));

		tx_idx = 0;
		wlc_phy_txpwr_by_index_acphy(pi, 1, tx_idx);

		wlc_phy_get_txgain_settings_by_index_acphy(
				pi, target_gains, tx_idx);

		min_delta = 1024;
		prev_delta = 1024;

		sat_delta = 0;
		ct = 0;

		curr_pad_gain = target_gains->rad_gain_mi & 0x00ff;

		PHY_TRACE(("Current PAD Gain (Before Search) is %d\n", curr_pad_gain));
		pad_gain_step = 1;

		for (pad_gain = curr_pad_gain; pad_gain <= MAX_PAD_GAIN; pad_gain += pad_gain_step)
		{
			target_gains->rad_gain_mi = target_gains->rad_gain_mi & 0xff00;
			target_gains->rad_gain_mi |= pad_gain;

			PHY_TRACE(("Current PAD Gain is %d\n", pad_gain));

			wlc_phy_poll_samps_WAR_acphy(pi, tssi, TRUE, FALSE,
			                             target_gains, FALSE, TRUE, 0, 0);
			tone_tssi[0] = tssi[0] - idle_tssi[0];

			delta_tssi = target_tssi - tone_tssi[0];

			/* Manipulate the step size to cut down the search time */
			if (delta_tssi > 50) {
				pad_gain_step = 10;
			} else if (delta_tssi > 30) {
				pad_gain_step = 5;
			} else if (delta_tssi > 15) {
				pad_gain_step = 2;
			} else {
				pad_gain_step = 1;
			}

			/* Check for TSSI Saturation */
			if (ct == 0) {
				sat_delta = delta_tssi;
			} else {
				sat_delta = delta_tssi - prev_delta;
				sat_delta = ABS(sat_delta);

				PHY_TRACE(("Ct=%d sat_delta=%d delta_tssi=%d sat_delta=%d\n",
					ct, sat_delta, delta_tssi, sat_delta));
			}

			if (sat_delta > sat_threshold) {
				ct = 0;
			}

			if ((ct == sat_count) && (sat_delta < sat_threshold)) {

				PHY_TRACE(("Ct = %d\t sat_delta = %d \t "
						"sat_threshold = %d\n",
						ct, sat_delta, sat_threshold));

				gain_code_found = 0;

				PHY_TRACE(("Breaking out of search as TSSI "
						" seems to have saturated\n"));
				WLC_PHY_PRECAL_TRACE(tx_idx, target_gains);

				break;
			}

			ct = ct + 1;

			PHY_TRACE(("Index = %3d target_TSSI = %4i tone_TSSI = %4i"
					"delta_TSSI = %4i min_delta = %4i radio gain = 0x%x%x%x, "
					"bbm=%d, dacgn = %d\n", tx_idx,
					target_tssi, tone_tssi[0], delta_tssi, min_delta,
					target_gains->rad_gain_hi, target_gains->rad_gain_mi,
					target_gains->rad_gain, target_gains->bbmult,
					target_gains->dac_gain));

			temp_val = ABS(delta_tssi);
			if (temp_val <= min_delta) {
				min_delta = ABS(delta_tssi);

				if (min_delta <= delta_threshold) {
					gain_code_found	= 1;

					PHY_TRACE(("Breaking out of search as min delta"
							" tssi threshold conditions are met\n"));

					WLC_PHY_PRECAL_TRACE(tx_idx, target_gains);

					break;
				}
			}
			prev_delta = delta_tssi;
		}

		if (gain_code_found == 0) {
			PHY_TRACE(("*** Search failed Again ***\n"));
		}

	/* delta_tssi < 0 ==> target tssi is less than tone tssi and we have to reduce the gain */
	} else {

		PHY_TRACE(("delta_tssi < 0 ==> target tssi is less than"
				"tone tssi and we have to reduce the gain\n"));

		tx_idx_step = 1;
		dont_alter_step = 0;
		pad_step_size = 0;
		pad_iteration_count = 0;

		sat_delta = 0;
		ct = 0;

		for (tx_idx = 0; tx_idx <= MAX_TX_IDX; tx_idx +=  tx_idx_step) {
			wlc_phy_txpwr_by_index_acphy(pi, 1, tx_idx);

			wlc_phy_get_txgain_settings_by_index_acphy(
					pi, &(target_gains[0]), tx_idx);

			if (pad_step_size != 0) {
				curr_pad_gain = target_gains->rad_gain_mi & 0x00ff;
				curr_pad_gain = curr_pad_gain -
				(pad_iteration_count * pad_step_size);

				target_gains->rad_gain_mi =
				target_gains->rad_gain_mi & 0xff00;

				target_gains->rad_gain_mi |= curr_pad_gain;
			}

			wlc_phy_poll_samps_WAR_acphy(pi, tssi, TRUE,
			                             FALSE, &(target_gains[0]), FALSE, TRUE, 0, 0);
			tone_tssi[0] = tssi[0] - idle_tssi[0];

			delta_tssi = target_tssi - tone_tssi[0];

			PHY_TRACE(("Index = %3d target_TSSI = %4i "
					"tone_TSSI = %4i delta_TSSI = %4i min_delta = %4i "
					"radio gain = 0x%x%x%x, bbm=%d, dacgn = %d\n", tx_idx,
					target_tssi, tone_tssi[0], delta_tssi, min_delta,
					target_gains->rad_gain_hi, target_gains->rad_gain_mi,
					target_gains->rad_gain,
					target_gains->bbmult, target_gains->dac_gain));

			/* Check for TSSI Saturation */
			if (ct == 0) {
				sat_delta = delta_tssi;
			} else {
				sat_delta = delta_tssi - prev_delta;
				sat_delta = ABS(sat_delta);

				PHY_TRACE(("Ct=%d sat_delta=%d delta_tssi=%d sat_delta=%d\n",
					ct, sat_delta, delta_tssi, sat_delta));
			}

			if (sat_delta > sat_threshold) {
				ct = 0;
			}

			if ((ct == sat_count) && (sat_delta < sat_threshold) &&
				(ABS(delta_tssi) < sat_threshold)) {

				PHY_TRACE(("Ct = %d\t sat_delta = %d \t sat_threshold = %d\n",
					ct, sat_delta, sat_threshold));

				gain_code_found	= 0;

				PHY_TRACE(("Breaking out of search as TSSI "
						" seems to have saturated\n"));

				WLC_PHY_PRECAL_TRACE(tx_idx, target_gains);

				break;
			}

			ct = ct + 1;

			temp_val = ABS(delta_tssi);
			if (temp_val <= min_delta) {
				min_delta = ABS(delta_tssi);

				if (min_delta <= delta_threshold) {
					gain_code_found	= 1;

					PHY_TRACE(("Breaking out of search "
							"as min delta tssi threshold "
							"conditions are met\n"));

					WLC_PHY_PRECAL_TRACE(tx_idx, target_gains);

					PHY_TRACE(("===== IQLOCAL PreCalGainControl: END =====\n"));
					break;
				}
			}

			/* Change of sign in delta tssi => increase
			 * the step size with smaller resolution
			 */

			if ((prev_delta < 0) && (delta_tssi > 0)&& (tx_idx != 0))
			{
				PHY_TRACE(("Scenario 2 -- BELOW TARGET\n"));
				/* Now that tx idx is sufficiently dropped ,
				 * there is change in sign of the delta tssi.
				 * implies, now target tssi is more than tone tssi.
				 * So increase the gain in very small steps
				 * by decrementing the index
				 */
				tx_idx_step = -1;
				dont_alter_step = 1;
			} else if ((prev_delta < 0) && (delta_tssi < 0) && (dont_alter_step == 1)) {
				PHY_TRACE(("Scenario 3 --  OSCILLATORY\n"));

				/* this case is to take care of the oscillatory
				 * behaviour of the tone tssi about the target
				 * tssi. Here tone tssi has again
				 * overshot the target tssi. So donot change the
				 * tx gain index, but reduce the PAD gain
				 */
				tx_idx_step = 0;
				pad_step_size = 1;
				pad_iteration_count += 1;

			} else {
				PHY_TRACE(("Scenario 1 -- NORMAL\n"));
				/* tone tssi is more than target tssi.
				 * So increase the index and hence reduce the gain
				 */
				if (dont_alter_step == 0) {
					/* Manipulate the step size to cut down the search time */
					if (delta_tssi >= 50) {
						tx_idx_step = 5;
					} else if (delta_tssi >= 25) {
						tx_idx_step = 3;
					} else if (delta_tssi >= 10) {
						tx_idx_step = 2;
					} else {
						tx_idx_step = 1;
					}
				}
			}
			prev_delta = delta_tssi;
		}

	}

	/* Search found the right gain code meeting required tssi conditions */
	if (gain_code_found == 1) {

		PHY_TRACE(("******* SUMMARY *******\n"));
		WLC_PHY_PRECAL_TRACE(tx_idx, target_gains);

		PHY_TRACE(("Measured TSSI Value is %d\n", tone_tssi[0]));
		PHY_TRACE(("***********************\n"));
	}
	/* prevent crs trigger */
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	PHY_TRACE(("======= IQLOCAL PreCalGainControl : END =======\n"));

	/* Restore the original Gain code */
	wlc_phy_txcal_txgain_cleanup_acphy(pi, &orig_txgain[0]);

	return;
}

static void
wlc_phy_txcal_radio_setup_acphy_20698(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp)
{
	/* 20698_procs.tcl r708279: 20698_tx_iqlo_cal_radio_setup */

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 *
	 * Note that Radio Behavior controlled via RFCtrl is handled in the
	 * phy_setup routine, not here; also note that we use the "shotgun"
	 * approach here ("coreAll" suffix to write to all jtag cores at the
	 * same time)
	 */
	phy_info_t *pi = ti->pi;
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20698_ID);

	/* save radio config before changing it */
	phy_ac_reg_cache_save(ti->aci, RADIOREGS_TXIQCAL);

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 * inits & abbreviations
	 */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		/* Power loopback blocks bias and wideband PGA for IQCAL */
		MOD_RADIO_REG_20698(pi, IQCAL_CFG1, core, iqcal_PU_iqcal,	0x1);
		MOD_RADIO_REG_20698(pi, IQCAL_CFG5, core, wbpga_pu,		0x1);
		MOD_RADIO_REG_20698(pi, IQCAL_CFG1, core, iqcal_PU_tssi,	0x0);
		MOD_RADIO_REG_20698(pi, IQCAL_OVR1, core, ovr_iqcal_PU_tssi,	0x1);
		MOD_RADIO_REG_20698(pi, IQCAL_CFG5, core, loopback_bias_pu,	0x1);
		MOD_RADIO_REG_20698(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu,	0x1);

		/* Mux AUX path to ADC, disable other paths to ADC */
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq2_adc,		0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,	0x1);
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq1_adc,		0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,	0x1);
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_aux_adc,		0x1);
		MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,	0x1);

		if (Biq2byp) {
			/* Put alpf in Rx mode for RXIQ cal */
			MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq1_bq2,		0x1);
			MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,	0x1);
			MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_dac_bq2,		0x0);
			MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,	0x1);
			MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq2_rc,		0x0);
			MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,	0x1);
			MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_dac_rc,		0x1);
			MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,	0x1);
		} else {
		}
		MOD_RADIO_REG_20698(pi, TXDAC_REG3, core, i_config_IQDACbuf_cm_2g_sel,
				CHSPEC_IS2G(pi->radio_chanspec) ? 1 : 0);

		/* Adjust the accuracy of power detector for 5g */
		MOD_RADIO_REG_20698(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl,		0xb);

		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			MOD_RADIO_REG_20698(pi, TX2G_MIX_REG0, core, pa2g_tssi_ctrl_pu, 0x1);
			MOD_RADIO_REG_20698(pi, TX2G_CFG1_OVR, core, ovr_pa2g_tssi_ctrl_pu, 0x1);
			MOD_RADIO_REG_20698(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0x8);
			MOD_RADIO_REG_20698(pi, TX2G_MISC_CFG1, core, pa2g_tssi_ctrl_sel, 0x0);
			MOD_RADIO_REG_20698(pi, TX2G_CFG1_OVR, core, ovr_pa2g_tssi_ctrl_sel, 0x1);
			MOD_RADIO_REG_20698(pi, TX2G_MISC_CFG1, core, pa2g_tssi_ctrl_range, 0x1);
			MOD_RADIO_REG_20698(pi, TX2G_CFG1_OVR, core, ovr_pa2g_tssi_ctrl_range, 0x1);
		} else {
			MOD_RADIO_REG_20698(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0xA);
			MOD_RADIO_REG_20698(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl_sel, 0x0);
			MOD_RADIO_REG_20698(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_sel, 0x1);
			MOD_RADIO_REG_20698(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl_range, 0x0);
			MOD_RADIO_REG_20698(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_range, 0x1);
		}

		MOD_RADIO_REG_20698(pi, IQCAL_CFG1, core, iqcal_sel_ext_tssi,		0x0);
		MOD_RADIO_REG_20698(pi, IQCAL_CFG1, core, iqcal_tssi_GPIO_ctrl,		0x0);

		/* AUX PGA is powered down because we are using WBPGA */
		MOD_RADIO_REG_20698(pi, AUXPGA_CFG1, core, auxpga_i_pu,			0x0);
		MOD_RADIO_REG_20698(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu,		0x1);
		MOD_RADIO_REG_20698(pi, AUXPGA_CFG1, core, auxpga_i_sel_input,		0x0);
		MOD_RADIO_REG_20698(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input,	0x1);

		MOD_RADIO_REG_20698(pi, IQCAL_CFG4, core, iqcal2adc,		0x1);
		MOD_RADIO_REG_20698(pi, IQCAL_CFG4, core, auxpga2adc,		0x0);

		MOD_RADIO_REG_20698(pi, TESTBUF_CFG1, core, testbuf_PU,		0x0);
		MOD_RADIO_REG_20698(pi, TESTBUF_OVR1, core, ovr_testbuf_PU,	0x1);

		WRITE_RADIO_REG_20698(pi, IQCAL_GAIN_RIN, core,			0x1000);
		WRITE_RADIO_REG_20698(pi, IQCAL_GAIN_RFB, core,			0x0200);
	}
}

static void
wlc_phy_txcal_radio_setup_acphy_20704(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp)
{
	/* 20704_procs.tcl r830494: 20704_tx_iqlo_cal_radio_setup */

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 *
	 * Note that Radio Behavior controlled via RFCtrl is handled in the
	 * phy_setup routine, not here; also note that we use the "shotgun"
	 * approach here ("coreAll" suffix to write to all jtag cores at the
	 * same time)
	 */
	phy_info_t *pi = ti->pi;
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20704_ID);

	/* save radio config before changing it */
	phy_ac_reg_cache_save(ti->aci, RADIOREGS_TXIQCAL);

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 * inits & abbreviations
	 */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		RADIO_REG_LIST_START
			MOD_RADIO_REG_20704_ENTRY(pi, TXDAC_REG3, core,
					i_config_IQDACbuf_cm_2g_sel, 0x0)
			/* Power loopback blocks bias and wideband PGA for IQCAL */
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG5, core, loopback_bias_pu,	0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu,	0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG1, core, iqcal_PU_tssi,		0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_OVR1, core, ovr_iqcal_PU_tssi,	0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG1, core, iqcal_PU_iqcal,		0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG5, core, wbpga_pu,		0x1)

			/* Mux AUX path to ADC, disable other paths to ADC */
			MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc,		0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,	0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc,		0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,	0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc,		0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,	0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);

		if (Biq2byp) {
			RADIO_REG_LIST_START
				/* Put alpf in Rx mode for RXIQ cal */
				MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,
					0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,
					0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,
					0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,
					0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,
					0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,
					0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,
					0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,
					0x1)
			RADIO_REG_LIST_EXECUTE(pi, core);

			if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
				/* keep RX RCCR ON during aux TX IQ CAL for best RX IQ cal */
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_rx_rccr_pu,	0x1)
					MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_rx_rccr_pu,	0x1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}

		RADIO_REG_LIST_START
			/* Adjust the accuracy of power detector - dual band knob for this radio */
			MOD_RADIO_REG_20704_ENTRY(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl,	0xb)
			/* Enable TSSI det at PAD out (dual band circuit) and mux it to iqcal path
			*/
			MOD_RADIO_REG_20704_ENTRY(pi, TX5G_MIX_REG0, core, pa5g_tssi_ctrl_pu,	0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_pu,
				0x1)
			/* Selects PAD TSSI output */
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG1, core, iqcal_sel_sw,		0xa)
			MOD_RADIO_REG_20704_ENTRY(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl_sel,	0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_sel,
				0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl_range,
				0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_range,
				0x1)

			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG1, core, iqcal_sel_ext_tssi,	0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG1, core, iqcal_tssi_GPIO_ctrl,	0x0)

			/* AUX PGA is powered down because we are using WBPGA */
			MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu,		0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu,	0x1)

			MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input,	0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input,
				0x1)

			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc,		0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc,		0x0)

			MOD_RADIO_REG_20704_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU,		0x0)
			MOD_RADIO_REG_20704_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU,	0x1)

			WRITE_RADIO_REG_20704_ENTRY(pi, IQCAL_GAIN_RIN, core,		0x1000)
		RADIO_REG_LIST_EXECUTE(pi, core);

		if (Biq2byp) {
			/* Reduce WBPGA gain by 6 dB wrt 0x200, as alpf byp has more gain */
			WRITE_RADIO_REG_20704(pi, IQCAL_GAIN_RFB, core,	0x0400);
		} else {
			WRITE_RADIO_REG_20704(pi, IQCAL_GAIN_RFB, core,	0x0200);
		}

		if (CHSPEC_IS2G(pi->radio_chanspec) && !PHY_IPA(pi)) {
			/* For 2GHz ePA we need to increase PAD gain during TxIQLO cal */
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad_ind_short,	0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad5g_tune,	0x7)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	}
}

static void
wlc_phy_txcal_radio_setup_acphy_20707(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp)
{
	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 *
	 * Note that Radio Behavior controlled via RFCtrl is handled in the
	 * phy_setup routine, not here; also note that we use the "shotgun"
	 * approach here ("coreAll" suffix to write to all jtag cores at the
	 * same time)
	 */
	phy_info_t *pi = ti->pi;
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20707_ID);

	/* save radio config before changing it */
	phy_ac_reg_cache_save(ti->aci, RADIOREGS_TXIQCAL);

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 * inits & abbreviations
	 */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		RADIO_REG_LIST_START
			/* Power loopback blocks bias and wideband PGA for IQCAL */
			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG5, core,
				loopback_bias_pu,       0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_OVR1, core,
				ovr_loopback_bias_pu,   0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG1, core,
				iqcal_PU_tssi,          0x0)
			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_OVR1, core,
				ovr_iqcal_PU_tssi,      0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG1, core,
				iqcal_PU_iqcal,         0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG5, core,
				wbpga_pu,               0x1)

			/* Mux AUX path to ADC, disable other paths to ADC */
			MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core,
				lpf_sw_bq2_adc,           0x0)
			MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core,
				ovr_lpf_sw_bq2_adc,       0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core,
				lpf_sw_bq1_adc,           0x0)
			MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR2, core,
				ovr_lpf_sw_bq1_adc,       0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core,
				lpf_sw_aux_adc,           0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR2, core,
				ovr_lpf_sw_aux_adc,       0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);

		/* Biq2byp = 1 (for RxIQ cal) corresponds to "lpf_mode = txiq_rx4" in TCL */
		if (Biq2byp) {
			RADIO_REG_LIST_START
				/* Put alpf in Rx mode for RXIQ cal */
				MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,
					0x0)
				MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,
					0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,
					0x0)
				MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,
					0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,
					0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,
					0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,
					0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,
					0x1)
			RADIO_REG_LIST_EXECUTE(pi, core);
			if CHSPEC_ISPHY5G6G(pi->radio_chanspec) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_rx_rccr_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_rx_rccr_pu, 0x1)
				RADIO_REG_LIST_EXECUTE(pi, core);
				if (pi->epagain5g != 2) {
					/* use pad/mixer settings used in iPA gain table
					   for higher gain.
					 */
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_MIX_REG2,
							core, tx5g_mx_idac_bb, 18)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_gm, 6)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG3,
							core, tx5g_pad_idac_cas, 18)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_mirror_cas, 6)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_MIX_GC_REG,
							core, tx5g_mx_gc, 14)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG1_OVR,
							core, ovr_tx5g_mx_gc, 1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_MIX_GC_REG,
							core, tx5g_mx_gc_branch, 3)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG1_OVR,
							core, ovr_tx5g_mx_gc_branch, 1)
					RADIO_REG_LIST_EXECUTE(pi, core);
				}
			} else {
				if (pi->epagain2g != 2) {
					/* use pad/mixer settings used in iPA gain table
					   for higher gain.
					 */
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20707_ENTRY(pi, TX2G_MIX_GC_REG,
							core, tx2g_mx_gc, 9)
						MOD_RADIO_REG_20707_ENTRY(pi, TX2G_CFG1_OVR,
							core, ovr_tx2g_mx_gc, 1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX2G_MIX_GC_REG,
							core, tx2g_mx_gc_branch, 3)
						MOD_RADIO_REG_20707_ENTRY(pi, TX2G_CFG1_OVR,
							core, ovr_tx2g_mx_gc_branch, 1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX2G_MIX_REG2,
							core, tx2g_mx_idac_bb, 12)
						MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG1,
							core, tx2g_pad_idac_gm, 6)
						MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG3,
							core, tx2g_pad_idac_cas, 18)
						MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG1,
							core, tx2g_pad_idac_mirror_cas, 6)
					RADIO_REG_LIST_EXECUTE(pi, core);
				}
			}
			/* alpf position during rxiqlocal is in Rx path! */
		} else {
			/* RFSeq takes care of the alpf switches config.
			   alpf default position during txiqlocal is in Tx path!
			 */
		}

		if CHSPEC_IS2G(pi->radio_chanspec) {
			if (Biq2byp) {
				RADIO_REG_LIST_START
					/* Adjust the accuracy of power detector */
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG0, core,
						tx2g_tssi_ctrl, 0x0)

					/* Power up and enable TSSI det at PAD out and mux it
					   to iqcal path.
					*/
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG0, core,
						tx2g_tssi_ctrl_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_CFG_OVR, core,
						ovr_tx2g_tssi_ctrl_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG1, core,
						iqcal_sel_sw, 0x8)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG1, core,
						tx2g_tssi_sel, 0x1)

					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG0, core,
						tx2g_tssi_ctrl_range, 0x0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_CFG_OVR, core,
						ovr_tx2g_tssi_ctrl_range, 0x1)

					/* Set DACbuf cm sel */
					MOD_RADIO_REG_20707_ENTRY(pi, TXDAC_REG3, core,
						i_config_IQDACbuf_cm_2g_sel, 0x1)

					/* Power down the iPA, not needed in 6878 */
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0, core,
						tx2g_ipa_bias_pu, 0x0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_CFG_OVR, core,
						ovr_tx2g_ipa_bias_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG0, core,
						tx2g_ipa_tssi_ctrl_pu, 0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_CFG_OVR, core,
						ovr_tx2g_ipa_tssi_ctrl_pu, 1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			} else {
				RADIO_REG_LIST_START
					/* Adjust the accuracy of power detector */
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG0, core,
						tx2g_tssi_ctrl, 0x0)

					/* power up and enable TSSI det at PAD out and mux it
					   to iqcal path.
					*/
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG0, core,
						tx2g_tssi_ctrl_pu, 0x0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_CFG_OVR, core,
						ovr_tx2g_tssi_ctrl_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG1, core,
						iqcal_sel_sw, 0x8)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG1, core,
						tx2g_tssi_sel, 0x0)

					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG0, core,
						tx2g_tssi_ctrl_range, 0x0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_CFG_OVR, core,
						ovr_tx2g_tssi_ctrl_range, 0x1)

					/* Set DACbuf cm sel */
					MOD_RADIO_REG_20707_ENTRY(pi, TXDAC_REG3, core,
						i_config_IQDACbuf_cm_2g_sel, 0x1)

					/* Power down the iPA, not needed in 6878 */
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0, core,
						tx2g_ipa_bias_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_CFG_OVR, core,
						ovr_tx2g_ipa_bias_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_TSSI_REG0, core,
						tx2g_ipa_tssi_ctrl_pu, 1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_CFG_OVR, core,
						ovr_tx2g_ipa_tssi_ctrl_pu, 1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		} else {
			if (Biq2byp) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
					tx5g_tssi_ctrl, 0x0)

					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_pu, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG1, core,
						iqcal_sel_sw, 0xa)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG1, core,
						tx5g_tssi_sel, 0x1)

					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_sel, 0x0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_sel, 0x1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_range, 0x0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_range, 0x1)

					/* Set DACbuf cm sel */
					MOD_RADIO_REG_20707_ENTRY(pi, TXDAC_REG3, core,
						i_config_IQDACbuf_cm_2g_sel, 0)

					/* Power down the iPA, not needed in 6878 */
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0, core,
						tx5g_ipa_bias_pu, 0x0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_CFG_OVR, core,
						ovr_tx5g_ipa_bias_pu, 0x1)

					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_ipa_tssi_ctrl_pu, 0)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_ipa_tssi_ctrl_pu, 1)

					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_ipa_tssi_ctrl_range, 1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_ipa_tssi_ctrl_range, 1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			} else {
				if (pi->epagain5g == 2) {
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_tssi_ctrl, 0x0)

						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_tssi_ctrl_pu, 0x0)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_tssi_ctrl_pu, 0x1)
						MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG1, core,
							iqcal_sel_sw, 0xa)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG1, core,
							tx5g_tssi_sel, 0x0)

						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_tssi_ctrl_sel, 0x0)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_tssi_ctrl_sel, 0x1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_tssi_ctrl_range, 0x1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_tssi_ctrl_range, 0x1)

						/* Set DACbuf cm sel */
						MOD_RADIO_REG_20707_ENTRY(pi, TXDAC_REG3, core,
							i_config_IQDACbuf_cm_2g_sel, 0)

						/* Power up the iPA and TSSI det */
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0, core,
							tx5g_ipa_bias_pu, 0x1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_CFG_OVR,
							core, ovr_tx5g_ipa_bias_pu, 0x1)

						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_ipa_tssi_ctrl_pu, 1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_ipa_tssi_ctrl_pu, 1)

						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_ipa_tssi_ctrl_range, 1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_ipa_tssi_ctrl_range, 1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_GC_REG, core,
							tx5g_ipa_gc, 255)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_CFG_OVR,
							core, ovr_tx5g_ipa_gc, 1)
					RADIO_REG_LIST_EXECUTE(pi, core);
				} else {
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_tssi_ctrl, 0x0)

						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_tssi_ctrl_pu, 0x0)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_tssi_ctrl_pu, 0x1)
						MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG1, core,
							iqcal_sel_sw, 0xa)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG1, core,
							tx5g_tssi_sel, 0x0)

						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_tssi_ctrl_sel, 0x0)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_tssi_ctrl_sel, 0x1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_tssi_ctrl_range, 0x1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_tssi_ctrl_range, 0x1)

						/* Set DACbuf cm sel */
						MOD_RADIO_REG_20707_ENTRY(pi, TXDAC_REG3, core,
							i_config_IQDACbuf_cm_2g_sel, 0)

						/* Power up the iPA and TSSI det */
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0, core,
							tx5g_ipa_bias_pu, 0x1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_CFG_OVR,
							core, ovr_tx5g_ipa_bias_pu, 0x1)

						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_ipa_tssi_ctrl_pu, 1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_ipa_tssi_ctrl_pu, 1)

						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_TSSI_REG0, core,
							tx5g_ipa_tssi_ctrl_range, 0)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_CFG_OVR, core,
							ovr_tx5g_ipa_tssi_ctrl_range, 1)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_GC_REG, core,
							tx5g_ipa_gc, 255)
						MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_CFG_OVR,
							core, ovr_tx5g_ipa_gc, 1)
					RADIO_REG_LIST_EXECUTE(pi, core);
				}
			}
		}

		RADIO_REG_LIST_START
			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG1, core, iqcal_sel_ext_tssi,     0x0)

			/* AUX PGA is powered down because we are using WBPGA */
			MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu,           0x0)
			MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu,       0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input,    0x0)
			MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input,
				0x1)

			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc,              0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc,             0x0)

			MOD_RADIO_REG_20707_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU,           0x0)
			MOD_RADIO_REG_20707_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU,       0x1)

			WRITE_RADIO_REG_20707_ENTRY(pi, IQCAL_GAIN_RIN, core,           0x1000)
			WRITE_RADIO_REG_20707_ENTRY(pi, IQCAL_GAIN_RFB, core,           0x0200)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}

static void
wlc_phy_txcal_radio_setup_acphy_20708(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp)
{
	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 *
	 * Note that Radio Behavior controlled via RFCtrl is handled in the
	 * phy_setup routine, not here; also note that we use the "shotgun"
	 * approach here ("coreAll" suffix to write to all jtag cores at the
	 * same time)
	 */
	phy_info_t *pi = ti->pi;
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20708_ID);

	/* save radio config before changing it */
	phy_ac_reg_cache_save(ti->aci, RADIOREGS_TXIQCAL);

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 * inits & abbreviations
	 */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		RADIO_REG_LIST_START
			/* Power loopback blocks bias and wideband PGA for IQCAL */
			/* Turn on and configure Loopback path (loopback bias,
			 * IQcal buffer, WBPGA, AUXPGA and TestBuf)
			 */
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_OVR1, core,
				ovr_loopback_bias_pu,   0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG5, core,
				loopback_bias_pu,       0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG1, core,
				iqcal_PU_iqcal,         0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG1, core,
				iqcal_sel_sw,		0xa)
		RADIO_REG_LIST_EXECUTE(pi, core);
		/* Current: use_wbpga is always 1 (via wbpga) */
		/* Future: depending on how it works, auxpga option will be
		 *		avaialble during bring-up
		*/
		RADIO_REG_LIST_START
			/* Configure Loopback path to use WBPGA */
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG5, core,
				wbpga_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG4, core,
				iqcal2adc, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG4, core,
				auxpga2adc, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core,
				auxpga_i_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_OVR1, core,
				ovr_testbuf_PU, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_CFG1, core,
				testbuf_PU, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core,
				ovr_auxpga_i_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core,
				auxpga_i_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core,
				ovr_auxpga_i_sel_input, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core,
				auxpga_i_sel_input, 0x1)

			/* Set WBPGA gain and BW */
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_GAIN_RIN, core,
				iqcal_rin, 0x1000)
			MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_GAIN_RFB, core,
				iqcal_rfb, 0x0200)
			/* Testbuf not used, power down and disconnect input
			 * from TxIQ cal path
			*/
			MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_OVR1, core,
				ovr_testbuf_sel_test_port, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_CFG1, core,
				testbuf_sel_test_port, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_OVR1, core,
				ovr_testbuf_PU, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_CFG1, core,
				testbuf_PU, 0x0)
		RADIO_REG_LIST_EXECUTE(pi, core);
		/*	// Once auxpga is enable, it will be on followings
		else {
			RADIO_REG_LIST_START
				// Configure Loopback path to use AUXPGA
				MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG5, core,
					wbpga_pu, 0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG4, core,
					iqcal2adc, 0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG4, core,
					auxpga2adc, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core,
					ovr_auxpga_i_pu, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core,
					auxpga_i_pu, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core,
					ovr_auxpga_i_sel_input, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core,
					auxpga_i_sel_input, 0x0)
				// Set AUXPGA gain, BW and reference voltage
				MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core,
					ovr_auxpga_i_sel_gain, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core,
					auxpga_i_sel_gain, 0x7)
				MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core,
					ovr_auxpga_i_sel_vmid, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_VMID, core,
					auxpga_i_sel_vmid, 0x0)
				// Enable testbuf and connect to TxIQ cal path
				MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_OVR1, core,
					ovr_testbuf_sel_test_port, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_CFG1, core,
					testbuf_sel_test_port, 0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_OVR1, core,
					ovr_testbuf_PU, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_CFG1, core,
					testbuf_PU, 0x1)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
		*/

		RADIO_REG_LIST_START
			/* Set LPF switches correctly (ADC input to come
			 * from loopback path)
			*/
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core,
				ovr_lpf_sw_bq2_adc, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core,
				ovr_lpf_sw_bq1_adc, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core,
				ovr_lpf_sw_aux_adc, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core,
				lpf_sw_bq2_adc, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core,
				lpf_sw_bq1_adc, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core,
				lpf_sw_aux_adc, 0x1)

			/* Turn on and configure dual band TSSI envelope detector */
			MOD_RADIO_REG_20708_ENTRY(pi, TXDB_CFG1_OVR, core,
				ovr_tx_tssi_ctrl_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG0, core,
				tx_tssi_ctrl_pu, 0xf)
			MOD_RADIO_REG_20708_ENTRY(pi, TXDB_CFG1_OVR, core,
				ovr_tx_tssi_ctrl_range, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MISC_CFG1, core,
				tx_tssi_ctrl_range, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MISC_CFG1, core,
				tx_tssi_ctrl_range_MSB, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, TXDB_CFG1_OVR, core,
				ovr_tx_tssi_ctrl_sel, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MISC_CFG1, core,
				tx_tssi_ctrl_sel, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MISC_CFG1, core,
				tx_tssi_ctrl_sel_MSB, 0x0)
		RADIO_REG_LIST_EXECUTE(pi, core);

		if (Biq2byp) {
			RADIO_REG_LIST_START
				/* Put alpf in Rx mode for RXIQ cal */
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core,
					ovr_lpf_sw_bq2_rc, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core,
					ovr_lpf_sw_dac_bq2, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core,
					ovr_lpf_sw_dac_rc, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core,
					ovr_lpf_sw_bq1_bq2, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core,
					lpf_sw_bq1_bq2, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core,
					lpf_sw_dac_bq2, 0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core,
					lpf_sw_bq2_rc, 0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core,
					lpf_sw_dac_rc, 0x1)
				/* lpf_mode set to txiq_rx4, this will bypass the LPF
				 * in Tx path and the DAC drives the RC-notch directly
				 */
				/* this mode is used to get the Tx cal coefficients
				 * to be used during RxIQ cal
				 */

				MOD_RADIO_REG_20708_ENTRY(pi, TXDAC_REG6, core,
					iqdac_buf_vocmrefsel, 0x3)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core,
					ovr_lpf_rc_gain, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, LPF_NOTCH_CONTROL2, core,
					lpf_rc_gain, 0x4)
			RADIO_REG_LIST_EXECUTE(pi, core);

			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20708_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_tx_pad_band_sel, 0x1)
					MOD_RADIO_REG_20708_ENTRY(pi, TXDB_REG0, core,
						tx_pad_band_sel, 0x1)
					MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_tune, 0xf)
				RADIO_REG_LIST_EXECUTE(pi, core);
				if (RADIOREV(pi->pubpi->radiorev) < 2) {
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
							txdb_mx_idac_bb, 0x1f)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
							txdb_pad_idac_gm, 0x12)
						MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
							txdb_pad_idac_cas, 0xf)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
							txdb_pad_idac_mirror_cas, 0x0)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_CFG1_OVR, core,
							ovr_txdb_mx_gc, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_GC_REG, core,
							txdb_mx_gc, 0xf)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_CFG1_OVR, core,
							ovr_txdb_mx_gc_branch, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_GC_REG, core,
							txdb_mx_gc_branch, 0x3)
						MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
							tx_pad_tune, 0xf)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, 0,
							txdb_pad_idac_cas_off, 0x0)
					RADIO_REG_LIST_EXECUTE(pi, core);
				} else {
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_CFG1_OVR, core,
							ovr_txdb_mx_gc_branch, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_GC_REG, core,
							txdb_mx_gc_branch, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core,
							ovr_lpf_rc_gain, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, LPF_NOTCH_CONTROL2,
							core, lpf_rc_gain, 0x3)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
							txdb_mx_idac_bb, 0x1f)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_CFG1_OVR, core,
							ovr_pad_gc, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_GC_REG, core,
							txdb_pad_gc, 0x10)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_CFG1_OVR, core,
							ovr_txdb_mx_gc_cas, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_GC_REG, core,
							txdb_mx_gc_cas, 0x14)
						MOD_RADIO_REG_20708_ENTRY(pi, TXDB_REG0, core,
							tx_mx_bias_gm_boost, 0x1)
					RADIO_REG_LIST_EXECUTE(pi, core);
				}
			} else {
				if (RADIOREV(pi->pubpi->radiorev) < 2) {
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
							txdb_mx_idac_bb, 0x1f)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
							txdb_pad_idac_gm, 0x12)
						MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
							txdb_pad_idac_cas, 0x3)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
							txdb_pad_idac_mirror_cas, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_CFG1_OVR, core,
							ovr_txdb_mx_gc, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_GC_REG, core,
							txdb_mx_gc, 0xf)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_CFG1_OVR, core,
							ovr_txdb_mx_gc_branch, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_GC_REG, core,
							txdb_mx_gc_branch, 0x3)
					RADIO_REG_LIST_EXECUTE(pi, core);
				} else {
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core,
							ovr_lpf_rc_gain, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, LPF_NOTCH_CONTROL2,
							core, lpf_rc_gain, 0x3)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
							txdb_mx_idac_bb, 0x1f)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_CFG1_OVR, core,
							ovr_txdb_mx_gc_branch, 0x1)
						MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_GC_REG, core,
							txdb_mx_gc_branch, 0x3)
						MOD_RADIO_REG_20708_ENTRY(pi, TXDB_REG0, core,
							tx_mx_bias_gm_boost, 0x1)
					RADIO_REG_LIST_EXECUTE(pi, core);
				}
			}
		} /* End of Biq2byp */
	} /* End of Cores */
}

static void
wlc_phy_txcal_radio_setup_acphy_20709(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp)
{
	/* 20709_procs.tcl r830494: 20709_tx_iqlo_cal_radio_setup */
	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 *
	 * Note that Radio Behavior controlled via RFCtrl is handled in the
	 * phy_setup routine, not here; also note that we use the "shotgun"
	 * approach here ("coreAll" suffix to write to all jtag cores at the
	 * same time)
	 */
	phy_info_t *pi = ti->pi;
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20709_ID);

	/* save radio config before changing it */
	phy_ac_reg_cache_save(ti->aci, RADIOREGS_TXIQCAL);

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 * inits & abbreviations
	 */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		RADIO_REG_LIST_START
			/* Power loopback blocks bias and wideband PGA for IQCAL */
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG5, core, loopback_bias_pu,	0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu,	0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG1, core, iqcal_PU_tssi,		0x0)
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_OVR1, core, ovr_iqcal_PU_tssi,	0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG1, core, iqcal_PU_iqcal,		0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG5, core, wbpga_pu,		0x1)

			/* Mux AUX path to ADC, disable other paths to ADC */
			MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc,		0x0)
			MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,	0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc,		0x0)
			MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,	0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc,		0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,	0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);

		if (Biq2byp) {
			RADIO_REG_LIST_START
				/* Put alpf in Rx mode for RXIQ cal */
				MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,
					0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,
					0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,
					0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,
					0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,
					0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,
					0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,
					0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,
					0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_rx_rccr_pu, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_rx_rccr_pu, 0x1)
			RADIO_REG_LIST_EXECUTE(pi, core);
		} else {
			/* alpf default position during txiqlocal is in Tx path! */
		}

		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			RADIO_REG_LIST_START
				/* Adjust the accuracy of power detector */
				MOD_RADIO_REG_20709_ENTRY(pi, TX2G_TSSI_REG0, core, tx2g_tssi_ctrl,
					0x0)
				/* Power up and enable TSSI det at PAD out and mux
				 * it to iqcal path.
				 */
				MOD_RADIO_REG_20709_ENTRY(pi, TX2G_TSSI_REG0, core,
					tx2g_tssi_ctrl_pu, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, TX2G_CFG_OVR, core,
					ovr_tx2g_tssi_ctrl_pu, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG1, core,
					iqcal_sel_sw, 0x8)
				MOD_RADIO_REG_20709_ENTRY(pi, TX2G_TSSI_REG1, core,
					tx2g_tssi_sel, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, TX2G_TSSI_REG0, core,
					tx2g_tssi_ctrl_range, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, TX2G_CFG_OVR, core,
					ovr_tx2g_tssi_ctrl_range, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG3, core,
					i_config_IQDACbuf_cm_2g_sel, 0x1)
			RADIO_REG_LIST_EXECUTE(pi, core);
		} else {
			if (PHY_IPA(pi)) {
				RADIO_REG_LIST_START
					/* Adjust the accuracy of power detector */
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl, 0x0)
					/* Power up and enable TSSI det at PAD out and mux
					 * it to iqcal path.
					 */
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_pu, 0x1)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_pu, 0x1)
					MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG1, core,
						iqcal_sel_sw, 0xA)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG1, core,
						tx5g_tssi_sel, 0x1)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_sel, 0x0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_sel, 0x0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_range, 0x0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_range, 0x1)
					MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG3, core,
						i_config_IQDACbuf_cm_2g_sel, 0x0)
				RADIO_REG_LIST_EXECUTE(pi, core);
			} else {
				RADIO_REG_LIST_START
					/* Adjust the accuracy of power detector */
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl, 0x0)
					/* power up and enable TSSI det at PAD out and
					 *  mux it to iqcal path
					 */
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_pu, 0x0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_pu, 0x1)
					MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG1, core,
						iqcal_sel_sw, 0xa)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG1, core,
						tx5g_tssi_sel, 0x0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_sel, 0x0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_sel, 0x1)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_range, 0x0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_range, 0x1)
					/* Set DACbuf cm sel */
					MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG3, core,
						i_config_IQDACbuf_cm_2g_sel, 0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_ipa_tssi_ctrl_pu, 1)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_ipa_tssi_ctrl_pu, 1)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_TSSI_REG0, core,
						tx5g_tssi_ctrl_range, 0)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_CFG_OVR, core,
						ovr_tx5g_tssi_ctrl_range, 1)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_GC_REG, core,
						tx5g_ipa_gc, 255)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_CFG_OVR, core,
						ovr_tx5g_ipa_gc, 1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}

		RADIO_REG_LIST_START
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG1, core, iqcal_sel_ext_tssi, 0x0)
			/* AUX PGA is powered down because we are using WBPGA */
			MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x0)
			MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x0)
			MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input,
				0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc, 0x0)
			MOD_RADIO_REG_20709_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU, 0x0)
			MOD_RADIO_REG_20709_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1)
			WRITE_RADIO_REG_20709_ENTRY(pi, IQCAL_GAIN_RIN, core, 0x1000)
			WRITE_RADIO_REG_20709_ENTRY(pi, IQCAL_GAIN_RFB, core, 0x0200)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}

static void
wlc_phy_txcal_radio_setup_acphy_20710(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp)
{
	/* 20710_procs.tcl r830494: 20710_tx_iqlo_cal_radio_setup */

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 *
	 * Note that Radio Behavior controlled via RFCtrl is handled in the
	 * phy_setup routine, not here; also note that we use the "shotgun"
	 * approach here ("coreAll" suffix to write to all jtag cores at the
	 * same time)
	 */
	phy_info_t *pi = ti->pi;
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20710_ID);

	/* save radio config before changing it */
	phy_ac_reg_cache_save(ti->aci, RADIOREGS_TXIQCAL);

	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 * inits & abbreviations
	 */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		RADIO_REG_LIST_START
			MOD_RADIO_REG_20710_ENTRY(pi, TXDAC_REG3, core,
					i_config_IQDACbuf_cm_2g_sel, 0x0)
			/* Power loopback blocks bias and wideband PGA for IQCAL */
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG5, core, loopback_bias_pu,	0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu,	0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG1, core, iqcal_PU_tssi,		0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_OVR1, core, ovr_iqcal_PU_tssi,	0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG1, core, iqcal_PU_iqcal,		0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG5, core, wbpga_pu,		0x1)

			/* Mux AUX path to ADC, disable other paths to ADC */
			MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc,		0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,	0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc,		0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,	0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc,		0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,	0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);

		if (Biq2byp) {
			RADIO_REG_LIST_START
				/* Put alpf in Rx mode for RXIQ cal */
				MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,
					0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,
					0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,
					0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,
					0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,
					0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,
					0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,
					0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,
					0x1)
			RADIO_REG_LIST_EXECUTE(pi, core);

			if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
				/* keep RX RCCR ON during aux TX IQ CAL for best RX IQ cal */
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_rx_rccr_pu,	0x1)
					MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_rx_rccr_pu,	0x1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}

		RADIO_REG_LIST_START
			/* Adjust the accuracy of power detector - dual band knob for this radio */
			MOD_RADIO_REG_20710_ENTRY(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl,	0xb)
			/* Enable TSSI det at PAD out (dual band circuit) and mux it to iqcal path
			*/
			MOD_RADIO_REG_20710_ENTRY(pi, TX5G_MIX_REG0, core, pa5g_tssi_ctrl_pu,	0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_pu,
				0x1)
			/* Selects PAD TSSI output */
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG1, core, iqcal_sel_sw,		0xa)
			MOD_RADIO_REG_20710_ENTRY(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl_sel,	0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_sel,
				0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, TX5G_MISC_CFG1, core, pa5g_tssi_ctrl_range,
				0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, TX5G_CFG2_OVR, core, ovr_pa5g_tssi_ctrl_range,
				0x1)

			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG1, core, iqcal_sel_ext_tssi,	0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG1, core, iqcal_tssi_GPIO_ctrl,	0x0)

			/* AUX PGA is powered down because we are using WBPGA */
			MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu,		0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu,	0x1)

			MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input,	0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input,
				0x1)

			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc,		0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc,		0x0)

			MOD_RADIO_REG_20710_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU,		0x0)
			MOD_RADIO_REG_20710_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU,	0x1)

			WRITE_RADIO_REG_20710_ENTRY(pi, IQCAL_GAIN_RIN, core,		0x1000)
		RADIO_REG_LIST_EXECUTE(pi, core);

		if (Biq2byp) {
			/* Reduce WBPGA gain by 6 dB wrt 0x200, as alpf byp has more gain */
			WRITE_RADIO_REG_20710(pi, IQCAL_GAIN_RFB, core,	0x0400);
		} else {
			WRITE_RADIO_REG_20710(pi, IQCAL_GAIN_RFB, core,	0x0200);
		}

		if (CHSPEC_IS2G(pi->radio_chanspec) && !PHY_IPA(pi)) {
			/* For 2GHz ePA we need to increase PAD gain during TxIQLO cal */
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad_ind_short,	0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad5g_tune,	0x7)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	}
}

static void
wlc_phy_txcal_radio_setup_acphy_tiny(phy_info_t *pi)
{
	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 *
	 * Note that Radio Behavior controlled via RFCtrl is handled in the
	 * phy_setup routine, not here; also note that we use the "shotgun"
	 * approach here ("coreAll" suffix to write to all jtag cores at the
	 * same time)
	 */
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_txcal_radioregs_t *porig = (pi_ac->txiqlocali->ac_txcal_radioregs_orig);
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(TINY_RADIO(pi));

	/* SETUP: set 2059 into iq/lo cal state while saving off orig state */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		/* save off orig */
	        porig->iqcal_cfg1[core] = READ_RADIO_REG_TINY(pi, IQCAL_CFG1, core);
		porig->auxpga_cfg1[core] = READ_RADIO_REG_TINY(pi, AUXPGA_CFG1, core);
		porig->iqcal_cfg3[core] = READ_RADIO_REG_TINY(pi, IQCAL_CFG3, core);
		porig->tx_top_5g_ovr1[core] = READ_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core);
		porig->adc_cfg10[core] = READ_RADIO_REG_TINY(pi, ADC_CFG10, core);
		porig->auxpga_ovr1[core] = READ_RADIO_REG_TINY(pi, AUXPGA_OVR1, core);
		porig->testbuf_ovr1[core] = READ_RADIO_REG_TINY(pi, TESTBUF_OVR1, core);
		porig->adc_ovr1[core] = READ_RADIO_REG_TINY(pi, ADC_OVR1, core);
		porig->tx5g_misc_cfg1[core] = READ_RADIO_REG_TINY(pi, TX5G_MISC_CFG1, core);
		porig->testbuf_cfg1[core] = READ_RADIO_REG_TINY(pi, TESTBUF_CFG1, core);
		porig->tia_cfg5[core] = READ_RADIO_REG_TINY(pi, TIA_CFG5, core);
		porig->tia_cfg9[core] = READ_RADIO_REG_TINY(pi, TIA_CFG9, core);
		porig->auxpga_vmid[core] = READ_RADIO_REG_TINY(pi, AUXPGA_VMID, core);
		porig->pmu_cfg4[core] = READ_RADIO_REG_TINY(pi, PMU_CFG4, core);

		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
			porig->tx_top_2g_ovr_north[core] = READ_RADIO_REG_20693(pi,
				TX_TOP_2G_OVR_NORTH, core);
			porig->tx_top_2g_ovr_east[core] = READ_RADIO_REG_20693(pi,
				TX_TOP_2G_OVR_EAST, core);
			porig->iqcal_cfg2[core] = READ_RADIO_REG_20693(pi, IQCAL_CFG2, core);
			porig->pmu_ovr[core] = READ_RADIO_REG_20693(pi, PMU_OVR, core);
			porig->tx2g_misc_cfg1[core] = READ_RADIO_REG_20693(pi,
					TX2G_MISC_CFG1, core);
			porig->adc_cfg18[core] = READ_RADIO_REG_20693(pi, ADC_CFG18, core);
			porig->tx_top_5g_ovr2[core] = READ_RADIO_REG_20693(pi,
					TX_TOP_5G_OVR2, core);
			porig->txmix5g_cfg2[core] = READ_RADIO_REG_20693(pi, TXMIX5G_CFG2, core);
			porig->pad5g_cfg1[core] = READ_RADIO_REG_20693(pi, PAD5G_CFG1, core);
			porig->pa5g_cfg1[core] = READ_RADIO_REG_20693(pi, PA5G_CFG1, core);
		} else {
			porig->iqcal_ovr1[core] = READ_RADIO_REG_TINY(pi, IQCAL_OVR1, core);
			porig->pa2g_cfg1[core] = READ_RADIO_REG_TINY(pi, PA2G_CFG1, core);
		}

		/* # Enabling and Muxing per band */
	        if (CHSPEC_IS2G(pi->radio_chanspec)) {
		        MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0x8);
			if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20693_ENTRY(pi, TX2G_MISC_CFG1, core,
						pa2g_tssi_ctrl_sel, 0)
					MOD_RADIO_REG_20693_ENTRY(pi, TX_TOP_2G_OVR_NORTH, core,
						ovr_pa2g_tssi_ctrl_sel, 0x1)
					MOD_RADIO_REG_20693_ENTRY(pi, TX2G_MISC_CFG1, core,
						pa2g_tssi_ctrl, 0x5)
					MOD_RADIO_REG_20693_ENTRY(pi, TX_TOP_2G_OVR_EAST, core,
						ovr_pa2g_tssi_ctrl_range, 0x1)
					MOD_RADIO_REG_20693_ENTRY(pi, TX2G_MISC_CFG1, core,
						pa2g_tssi_ctrl_range, 1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			} else
				MOD_RADIO_REG_TINY(pi, PA2G_CFG1, core, pa2g_tssi_ctrl_sel, 0);
		} else {
		        MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_sel_sw, 0x0a);
			if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20693_ENTRY(pi, TX5G_MISC_CFG1, core,
							pa5g_tssi_ctrl_sel, 0)
					MOD_RADIO_REG_20693_ENTRY(pi, TX_TOP_5G_OVR1, core,
							ovr_pa5g_tssi_ctrl_sel, 0x1)
					MOD_RADIO_REG_20693_ENTRY(pi, TX5G_MISC_CFG1, core,
						pa5g_tssi_ctrl, 0x8)
					MOD_RADIO_REG_20693_ENTRY(pi, TX_TOP_5G_OVR1, core,
						ovr_pa5g_tssi_ctrl_range, 0x1)
					MOD_RADIO_REG_20693_ENTRY(pi, TX5G_MISC_CFG1, core,
						pa5g_tssi_ctrl_range, 0x0)
				RADIO_REG_LIST_EXECUTE(pi, core);
			} else {
				/* #tap from PA output */
				MOD_RADIO_REG_TINY(pi, TX5G_MISC_CFG1, core,
					pa5g_tssi_ctrl_sel, 0);
				MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
					ovr_pa5g_tssi_ctrl_sel, 0x1);
			}
		}

		MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_tssi_GPIO_ctrl, 0x0);

		if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3))
			MOD_RADIO_REG_TINY(pi, IQCAL_OVR1, core, ovr_iqcal_PU_iqcal, 1);
		/* # power up iqlocal */
		MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_PU_iqcal, 0x1);
		if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3)) {
			MOD_RADIO_REG_TINY(pi, IQCAL_OVR1, core, ovr_iqcal_PU_tssi, 1);
			MOD_RADIO_REG_TINY(pi, IQCAL_CFG1, core, iqcal_PU_tssi, 0x0);
		}

		/* ### JV: May be there is a  direct control for this, could'nt find it */
		MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1);
		MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1); /* # power up auxpga */

		MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1);
		MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x0);
		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		        MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_in_test, 0xF);
		}
		MOD_RADIO_REG_TINY(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 1);
		MOD_RADIO_REG_TINY(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0);
		MOD_RADIO_REG_TINY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1);
		MOD_RADIO_REG_TINY(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1);

		/* #pwr up adc and set auxpga gain */
		MOD_RADIO_REG_TINY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 1);
		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
			MOD_RADIO_REG_20693(pi, PMU_OVR, core, ovr_wlpmu_ADCldo_pu, 1);
			MOD_RADIO_REG_20693(pi, IQCAL_CFG2, core, iqcal_iq_cm_center, 0x8);
		}

		MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 1);
		MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, 0x3);
		MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 1);
		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
			MOD_RADIO_REG_TINY(pi, AUXPGA_VMID, 0, auxpga_i_sel_vmid, 0x97);
			MOD_RADIO_REG_TINY(pi, AUXPGA_VMID, 1, auxpga_i_sel_vmid, 0x99);
			MOD_RADIO_REG_TINY(pi, AUXPGA_VMID, 2, auxpga_i_sel_vmid, 0x98);
			MOD_RADIO_REG_TINY(pi, AUXPGA_VMID, 3, auxpga_i_sel_vmid, 0x95);
		} else {
			MOD_RADIO_REG_TINY(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, 0x9c);
		}

		/* #ensure that the dac mux is OFF because it shares a line with auxpga o/p */
		MOD_RADIO_REG_TINY(pi, TIA_CFG9, core, txbb_dac2adc, 0x0);
	        /* #these two lines disable the TIA gpaio mux and enable the */
		MOD_RADIO_REG_TINY(pi, TIA_CFG5, core, tia_out_test, 0x0);
		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
			MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_in_test, 0x0);
			if (CHSPEC_IS80(pi->radio_chanspec) ||
				PHY_AS_80P80(pi, pi->radio_chanspec)) {
				MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_od_pu, 0x1);
				MOD_RADIO_REG_20693(pi, ADC_CFG18, core, adc_od_pu, 0x1);
			} else if (CHSPEC_IS160(pi->radio_chanspec)) {
				ASSERT(0); // FIXME
			} else {
				MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_od_pu, 0x1);
				MOD_RADIO_REG_20693(pi, ADC_CFG18, core, adc_od_pu, 0x0);
			}
		} else {
			MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_in_test, 0x3);
		}
	}
}

static void
wlc_phy_txcal_radio_setup_acphy(phy_info_t *pi)
{
	/* This stores off and sets Radio-Registers for Tx-iqlo-Calibration;
	 *
	 * Note that Radio Behavior controlled via RFCtrl is handled in the
	 * phy_setup routine, not here; also note that we use the "shotgun"
	 * approach here ("coreAll" suffix to write to all jtag cores at the
	 * same time)
	 */

	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_txcal_radioregs_t *porig = (pi_ac->txiqlocali->ac_txcal_radioregs_orig);
	uint8 core;
	uint16 Av[3] = {5, 3, 4};
	uint16 vmid[3] = {80, 120, 100};
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* SETUP: set 2059 into iq/lo cal state while saving off orig state */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		/* save off orig */
		porig->iqcal_cfg1[core]  = READ_RADIO_REGC(pi, RF, IQCAL_CFG1, core);
		porig->iqcal_cfg2[core]  = READ_RADIO_REGC(pi, RF, IQCAL_CFG2, core);
		porig->iqcal_cfg3[core]  = READ_RADIO_REGC(pi, RF, IQCAL_CFG3, core);
		porig->pa2g_tssi[core]   = READ_RADIO_REGC(pi, RF, PA2G_TSSI, core);
		porig->tx5g_tssi[core]   = READ_RADIO_REGC(pi, RF, TX5G_TSSI, core);
		porig->auxpga_cfg1[core] = READ_RADIO_REGC(pi, RF, AUXPGA_CFG1, core);
		porig->OVR3[core] = READ_RADIO_REGC(pi, RF, OVR3, core);
		porig->auxpga_vmid[core] = READ_RADIO_REGC(pi, RF, AUXPGA_VMID, core);

		/* Reg conflict with 2069 rev 16 */
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0)
			porig->OVR20[core] = READ_RADIO_REGC(pi, RF, OVR20, core);
		else
			porig->OVR21[core] = READ_RADIO_REGC(pi, RF, GE16_OVR21, core);

		/* now write desired values */

		if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
			MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_sw, 0xb);
			MOD_RADIO_REGC(pi, TX5G_TSSI, core, pa5g_ctrl_tssi_sel, 0x1);

			/* Reg conflict with 2069 rev 16 */
			if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) {
				MOD_RADIO_REGC(pi, OVR20, core, ovr_pa5g_ctrl_tssi_sel, 0x1);
				MOD_RADIO_REGC(pi, OVR20, core, ovr_pa2g_ctrl_tssi_sel, 0x0);
			} else {
				MOD_RADIO_REGC(pi, GE16_OVR21, core, ovr_pa5g_ctrl_tssi_sel, 0x1);
				MOD_RADIO_REGC(pi, GE16_OVR21, core, ovr_pa2g_ctrl_tssi_sel, 0x0);
			}
			MOD_RADIO_REGC(pi, PA2G_TSSI, core, pa2g_ctrl_tssi_sel, 0x0);
		} else {
			MOD_RADIO_REGC(pi, IQCAL_CFG1, core, sel_sw, 0x8);
			MOD_RADIO_REGC(pi, TX5G_TSSI, core, pa5g_ctrl_tssi_sel, 0x0);
			/* Reg conflict with 2069 rev 16 */
			if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) {
				MOD_RADIO_REGC(pi, OVR20, core, ovr_pa5g_ctrl_tssi_sel, 0x0);
				MOD_RADIO_REGC(pi, OVR20, core, ovr_pa2g_ctrl_tssi_sel, 0x1);
			} else {
				MOD_RADIO_REGC(pi, GE16_OVR21, core, ovr_pa5g_ctrl_tssi_sel, 0x0);
				MOD_RADIO_REGC(pi, GE16_OVR21, core, ovr_pa2g_ctrl_tssi_sel, 0x1);
			}
			MOD_RADIO_REGC(pi, PA2G_TSSI, core, pa2g_ctrl_tssi_sel, 0x1);
		}
		MOD_RADIO_REGC(pi, IQCAL_CFG1, core, tssi_GPIO_ctrl, 0x0);

		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
			MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0x0);
			/* This bit is supposed to be controlled by phy direct control line.
			 * Please check: http://jira.broadcom.com/browse/HW11ACRADIO-45
			 */
			MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x0);
		}

		if (ACMAJORREV_5(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) {
			if (CHSPEC_IS2G(pi->radio_chanspec)) {
				MOD_RADIO_REGC(pi, OVR3, core, ovr_auxpga_i_sel_gain, 0x1);
				MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, 0);
				MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_auxpga_i_sel_vmid, 0x1);
				MOD_RADIO_REGC(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, 160);
			} else {
			   MOD_RADIO_REGC(pi, OVR3, core, ovr_auxpga_i_sel_gain, 0x1);
			   MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av[core]);
			   MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_auxpga_i_sel_vmid, 0x1);
		       MOD_RADIO_REGC(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, vmid[core]);
			}
		}
	} /* for core */
}

static void
wlc_phy_txcal_phy_setup_acphy(phy_info_t *pi, uint8 Biq2byp)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	txiqcal_phyregs_t *porig = (pi_ac->txiqlocali->paramsi->porig);
	uint16 sdadc_config;
	uint8  core, bw_idx;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	porig->RxFeCtrl1 = READ_PHYREG(pi, RxFeCtrl1);
	porig->AfePuCtrl = READ_PHYREG(pi, AfePuCtrl);
	porig->sampleCmd = READ_PHYREG(pi, sampleCmd);
	porig->RfseqCoreActv2059 = READ_PHYREG(pi, RfseqCoreActv2059);
	porig->fineclockgatecontrol = READ_PHYREG(pi, fineclockgatecontrol);

	if (CHSPEC_IS80(pi->radio_chanspec) || PHY_AS_80P80(pi, pi->radio_chanspec)) {
		bw_idx = 2;
		sdadc_config = sdadc_cfg80;
	} else if (CHSPEC_IS160(pi->radio_chanspec)) {
		bw_idx = 3;
		sdadc_config = sdadc_cfg80;
		//sdadc_config is not used under 43684
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		bw_idx = 1;
		if (pi->sdadc_config_override)
			sdadc_config = sdadc_cfg40hs;
		else
			sdadc_config = sdadc_cfg40;
	} else {
		bw_idx = 0;
		sdadc_config = sdadc_cfg20;
	}

	if (TINY_RADIO(pi) ||
	    ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
		ACPHY_REG_LIST_START
			/* #set sd adc full scale */
			MOD_PHYREG_ENTRY(pi, RxSdFeConfig1, farrow_rshift_force, 1)
			MOD_PHYREG_ENTRY(pi, RxSdFeConfig6, rx_farrow_rshift_0, 2)
			MOD_PHYREG_ENTRY(pi, TSSIMode, tssiADCSel, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

	/* turn off tssi sleep feature during cal */
	MOD_PHYREG(pi, AfePuCtrl, tssiSleepEn, 0);

	/*  SETUP: save off orig reg values and configure for cal  */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		wlc_phy_txcal_phy_setup_acphy_core(pi, porig, core, bw_idx, sdadc_config, Biq2byp);
	} /* for core */

	ACPHY_REG_LIST_START
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq0, 1)
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq1, 1)
		MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq2, 1)
	ACPHY_REG_LIST_EXECUTE(pi);
	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq3, 1);
	}

	/* ADC pulse clamp en fix */
	if (!ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
		wlc_phy_pulse_adc_reset_acphy(pi);
	}
	/* we should not need spur avoidance anymore
	porig->BBConfig = READ_PHYREG(pi, BBConfig);
	MOD_PHYREG(pi, BBConfig, resample_clk160, 0);
	*/
}

static void
wlc_phy_cal_txiqlo_update_ladder_acphy(phy_info_t *pi, uint16 bbmult, uint8 core)
{
	uint8  indx;
	uint32 bbmult_scaled;
	uint16 tblentry;
	uint8 stall_val;
	uint64 txgain;
	uint8 iqlocal_tbl_id = wlc_phy_get_tbl_id_iqlocal(pi, core);

	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		acphy_txiqcal_ladder_t ladder_lo[] = {
		{3, 0}, {4, 0}, {6, 0}, {9, 0}, {13, 0}, {18, 0},
		{25, 6}, {35, 7}, {45, 7}, {70, 7}, {81, 7}, {100, 7}};

		acphy_txiqcal_ladder_t ladder_iq[] = {
		{3, 0}, {4, 0}, {6, 0}, {9, 0}, {13, 0}, {18, 0},
		{25, 0}, {35, 0}, {50, 0}, {71, 0}, {80, 0}, {100, 1}};

		stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
		ACPHY_DISABLE_STALL(pi);

		for (indx = 0; indx < 12; indx++) {
			/* calculate and write LO cal gain ladder */
			bbmult_scaled = ladder_lo[indx].percent * bbmult;
			bbmult_scaled /= 100;
			tblentry = ((bbmult_scaled & 0xff) << 8) | ladder_lo[indx].g_env;
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, indx, 16, &tblentry);

			/* calculate and write IQ cal gain ladder */
			bbmult_scaled = ladder_iq[indx].percent * bbmult;
			bbmult_scaled /= 100;
			tblentry = ((bbmult_scaled & 0xff) << 8) | ladder_iq[indx].g_env;
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, indx+32, 16, &tblentry);
		}
		ACPHY_ENABLE_STALL(pi, stall_val);
	} else {
		acphy_txiqcal_ladder_t ladder_iqlo_4350_80mhz[] = {
			{75, 0}, {75, 1}, {75, 2}, {75, 3}, {75, 4}, {75, 5},
			{75, 6}, {75, 7}};
		acphy_txiqcal_ladder_t ladder_lo[] = {
		{3, 0}, {4, 0}, {6, 0}, {9, 0}, {13, 0}, {18, 0},
		{25, 0}, {25, 1}, {25, 2}, {25, 3}, {25, 4}, {25, 5},
		{25, 6}, {25, 7}, {35, 7}, {50, 7}, {71, 7}, {100, 7}};

		acphy_txiqcal_ladder_t ladder_iq[] = {
		{3, 0}, {4, 0}, {6, 0}, {9, 0}, {13, 0}, {18, 0},
		{25, 0}, {35, 0}, {50, 0}, {71, 0}, {100, 0}, {100, 1},
		{100, 2}, {100, 3}, {100, 4}, {100, 5}, {100, 6}, {100, 7}};

		acphy_txiqcal_ladder_t ladder_iqlo_majorrev47[] = {
		{2, 0}, {4, 0}, {6, 0}, {9, 0}, {13, 0}, {18, 0},
		{25, 0}, {35, 0}, {50, 0}, {71, 0}, {100, 0}, {141, 0}};

		acphy_txiqcal_ladder_t ladder_lo_majorrev130[] = {
		{3, 0}, {4, 0}, {6, 0}, {9, 0}, {13, 0}, {18, 0},
		{25, 0}, {35, 0}, {50, 0}, {71, 0}};

		acphy_txiqcal_ladder_t ladder_iq_majorrev130[] = {
		{6, 0}, {9, 0}, {13, 0}, {18, 0}, {25, 0}, {35, 0},
		{50, 0}, {71, 0}, {100, 0}, {141, 0}};

		acphy_txiqcal_ladder_t *ladder_lo_pt;
		acphy_txiqcal_ladder_t *ladder_iq_pt;
		uint8 lad_len = 0;

		stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
		ACPHY_DISABLE_STALL(pi);
		if (ACMAJORREV_2(pi->pubpi->phy_rev) && CHSPEC_IS80(pi->radio_chanspec)) {
			lad_len = 8;
			ladder_lo_pt = &(ladder_iqlo_4350_80mhz[0]);
			ladder_iq_pt = &(ladder_iqlo_4350_80mhz[0]);
		} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
			if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
				// lpf_bq2_gain = 0 is used for regular TX in 6715
				txgain = 0;
				lad_len = ARRAYSIZE(ladder_lo_majorrev130);
				for (indx = 0; indx < lad_len; indx++) {
					ladder_lo_majorrev130[indx].g_env = (uint8)txgain;
					ladder_iq_majorrev130[indx].g_env = (uint8)txgain;
				}
				ladder_lo_pt = &(ladder_lo_majorrev130[0]);
				ladder_iq_pt = &(ladder_iq_majorrev130[0]);
			} else {
				wlc_phy_table_read_acphy(pi, AC2PHY_TBL_ID_GAINCTRLBBMULTLUTS0,
				1, 0, 48, &txgain);
				txgain = (txgain & 0x0F000) >> 12;
				lad_len = ARRAYSIZE(ladder_iqlo_majorrev47);
				for (indx = 0; indx < lad_len; indx++) {
					ladder_iqlo_majorrev47[indx].g_env = (uint8)txgain;
				}
				ladder_lo_pt = &(ladder_iqlo_majorrev47[0]);
				ladder_iq_pt = &(ladder_iqlo_majorrev47[0]);
			}
		} else {
			lad_len = 18;
			ladder_lo_pt = &(ladder_lo[0]);
			ladder_iq_pt = &(ladder_iq[0]);
		}
		for (indx = 0; indx < lad_len; indx++) {
			/* calculate and write LO cal gain ladder */
			bbmult_scaled = ladder_lo_pt[indx].percent * bbmult;
			bbmult_scaled /= 100;
			tblentry = ((bbmult_scaled & 0xff) << 8) | ladder_lo_pt[indx].g_env;
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, indx, 16, &tblentry);

			/* calculate and write IQ cal gain ladder */
			bbmult_scaled = ladder_iq_pt[indx].percent * bbmult;
			bbmult_scaled /= 100;
			tblentry = ((bbmult_scaled & 0xff) << 8) | ladder_iq_pt[indx].g_env;
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, indx+32, 16, &tblentry);
		}
		ACPHY_ENABLE_STALL(pi, stall_val);
	}
}

static uint16
wlc_poll_adc_clamp_status(phy_info_t *pi, uint8 core, uint8 do_reset)
{
	uint16 ovr_status;

	if (TINY_RADIO(pi)) {
		ovr_status = READ_RADIO_REGFLD_TINY(pi, ADC_CFG14, core, adc_overload_i) |
			READ_RADIO_REGFLD_TINY(pi, ADC_CFG14, core, adc_overload_q);
	} else {
		ovr_status = READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core), ADC_STATUS,
		                                i_wrf_jtag_afe_iqadc_overload);
	}

	if (ovr_status && do_reset) {
		/* MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, 0, afe_iqadc_reset_ov_det, 1); */
		MOD_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, afe_iqadc_reset_ov_det,  1);
		MOD_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, afe_iqadc_reset_ov_det, 0);
	}
	return ovr_status;
}

static void
wlc_phy_txcal_phy_cleanup_acphy(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	txiqcal_phyregs_t *porig = (pi_ac->txiqlocali->paramsi->porig);
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	if (TINY_RADIO(pi) ||
	    ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 0);
		MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);
	}

	/*  CLEANUP: Restore Original Values  */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		/* restore ExtPA PU & TR */
		WRITE_PHYREGCE(pi, RfctrlIntc, core, porig->RfctrlIntc[core]);

		/* restore Rfctrloverride setting */
		WRITE_PHYREGCE(pi, RfctrlOverrideRxPus, core, porig->RfctrlOverrideRxPus[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreRxPus, core, porig->RfctrlCoreRxPus[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, core, porig->RfctrlOverrideTxPus[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreTxPus, core, porig->RfctrlCoreTxPus[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core,
			porig->RfctrlOverrideLpfSwtch[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, porig->RfctrlCoreLpfSwtch[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideLpfCT, core, porig->RfctrlOverrideLpfCT[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreLpfCT, core, porig->RfctrlCoreLpfCT[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreLpfGmult, core, porig->RfctrlCoreLpfGmult[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreRCDACBuf, core, porig->RfctrlCoreRCDACBuf[core]);
		WRITE_PHYREGCE(pi, RfctrlOverrideAuxTssi, core, porig->RfctrlOverrideAuxTssi[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreAuxTssi1, core, porig->RfctrlCoreAuxTssi1[core]);

		WRITE_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, porig->RfctrlOverrideAfeCfg[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, porig->RfctrlCoreAfeCfg1[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, porig->RfctrlCoreAfeCfg2[core]);

		/* restore PAPD Enable
		 * FIXME: not supported (and not needed) yet
		 * phy_utils_write_phyreg(pi, NPHY_PapdEnable(core), porig->PapdEnable[core]);
		 */

	} /* for core */

	WRITE_PHYREG(pi, RxFeCtrl1, porig->RxFeCtrl1);
	WRITE_PHYREG(pi, AfePuCtrl, porig->AfePuCtrl);
	WRITE_PHYREG(pi, sampleCmd, porig->sampleCmd);
	WRITE_PHYREG(pi, fineclockgatecontrol, porig->fineclockgatecontrol);

	WRITE_PHYREG(pi, RfseqCoreActv2059, porig->RfseqCoreActv2059);

	/* we should not need spur avoidance anymore
	WRITE_PHYREG(pi, BBConfig, porig->BBConfig);
	*/
	wlc_phy_resetcca_acphy(pi);
}

static void
wlc_phy_txcal_radio_cleanup_acphy_28nm(phy_ac_txiqlocal_info_t *ti)
{
	/* restore radio config back */
	phy_ac_reg_cache_restore(ti->aci, RADIOREGS_TXIQCAL);
}

static void
wlc_phy_txcal_radio_cleanup_acphy_tiny(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_txcal_radioregs_t *porig = (pi_ac->txiqlocali->ac_txcal_radioregs_orig);
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(TINY_RADIO(pi));

	/* CLEANUP: restore reg values */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		phy_utils_write_radioreg(pi, RADIO_REG(pi, IQCAL_CFG1, core),
		                         porig->iqcal_cfg1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_CFG1, core),
		                porig->auxpga_cfg1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, IQCAL_CFG3, core),
		                         porig->iqcal_cfg3[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TX_TOP_5G_OVR1, core),
		                porig->tx_top_5g_ovr1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, ADC_CFG10, core),
		                         porig->adc_cfg10[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_OVR1, core),
		                porig->auxpga_ovr1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TESTBUF_OVR1, core),
		                porig->testbuf_ovr1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, ADC_OVR1, core),
		                         porig->adc_ovr1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TX5G_MISC_CFG1, core),
		                         porig->tx5g_misc_cfg1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TESTBUF_CFG1, core),
		                         porig->testbuf_cfg1[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TIA_CFG5, core),
		                         porig->tia_cfg5[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, TIA_CFG9, core),
		                         porig->tia_cfg9[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_VMID, core),
		                         porig->auxpga_vmid[core]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, PMU_CFG4, core),
		                         porig->pmu_cfg4[core]);
		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, TX_TOP_2G_OVR_NORTH, core),
			                         porig->tx_top_2g_ovr_north[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core),
			                         porig->tx_top_2g_ovr_east[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, IQCAL_CFG2, core),
			                         porig->iqcal_cfg2[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OVR, core),
			                         porig->pmu_ovr[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, TX2G_MISC_CFG1, core),
			                         porig->tx2g_misc_cfg1[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, ADC_CFG18, core),
			                         porig->adc_cfg18[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, TX_TOP_5G_OVR2, core),
			                         porig->tx_top_5g_ovr2[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, TXMIX5G_CFG2, core),
			                         porig->txmix5g_cfg2[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PAD5G_CFG1, core),
			                         porig->pad5g_cfg1[core]);
			phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PA5G_CFG1, core),
			                         porig->pa5g_cfg1[core]);
		} else {
			phy_utils_write_radioreg(pi, RADIO_REG(pi, IQCAL_OVR1, core),
					porig->iqcal_ovr1[core]);
			phy_utils_write_radioreg(pi, RADIO_REG(pi, PA2G_CFG1, core),
					porig->pa2g_cfg1[core]);
		}
		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && (phy_get_phymode(pi) ==
			PHYMODE_MIMO) && (RADIOMAJORREV(pi) != 3)) {
			MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core, ovr_pa5g_pu, 0);
			MOD_RADIO_REG_TINY(pi, PA5G_CFG4, core, pa5g_pu, 0);
		}
	} /* for core */
}

static void
wlc_phy_txcal_radio_cleanup_acphy(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_txcal_radioregs_t *porig = (pi_ac->txiqlocali->ac_txcal_radioregs_orig);
	uint8 core;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* CLEANUP: restore reg values */
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		phy_utils_write_radioreg(pi, RF_2069_IQCAL_CFG1(core), porig->iqcal_cfg1[core]);
		phy_utils_write_radioreg(pi, RF_2069_IQCAL_CFG2(core), porig->iqcal_cfg2[core]);
		phy_utils_write_radioreg(pi, RF_2069_IQCAL_CFG3(core), porig->iqcal_cfg3[core]);
		phy_utils_write_radioreg(pi, RF_2069_PA2G_TSSI(core),  porig->pa2g_tssi[core]);
		phy_utils_write_radioreg(pi, RF_2069_TX5G_TSSI(core),  porig->tx5g_tssi[core]);
		phy_utils_write_radioreg(pi, RF_2069_AUXPGA_CFG1(core),  porig->auxpga_cfg1[core]);
		phy_utils_write_radioreg(pi, RF_2069_OVR3(core),  porig->OVR3[core]);
		phy_utils_write_radioreg(pi, RF_2069_AUXPGA_VMID(core),  porig->auxpga_vmid[core]);

		/* Reg conflict with 2069 rev 16 */
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0)
			phy_utils_write_radioreg(pi, RF_2069_OVR20(core),      porig->OVR20[core]);
		else
			phy_utils_write_radioreg(pi, RF_2069_GE16_OVR21(core), porig->OVR21[core]);
	} /* for core */
}

static void
wlc_phy_precal_txgain_control(phy_info_t *pi, txgain_setting_t *target_gains)
{
	int16  avvmid_set_local[2][2]     = {{1, 145}, {1,  145}};
	int16  target_tssi_set[2][5]   = {
		{710, 720, 425, 340, 370},
		{350, 350, 350, 350, 350}
	};
	int16 delta_tssi_error = 25;
	uint8  start_gain_idx[2][5][2] = {
		{{5, 31}, {5, 31}, {16, 31}, {16, 21}, {16, 24}},
		{{8, 31}, {8, 31}, {10, 31}, {14, 21}, {14, 24}}
	};
	uint8  gain_ladder[32] =
		{0x07, 0x0F, 0x17, 0x1F, 0x27, 0x2F, 0x37, 0x3F,
	0x47, 0x4F, 0x57, 0x5F, 0x67, 0x6F, 0x77, 0x7F,
	0x87, 0x8F, 0x97, 0x9F, 0xA7, 0xAF, 0xB7, 0xBF,
	0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7, 0xFF};

	uint8  band_idx, majorrev_idx, band_bw_idx, pad_gain, pga_gain, core;
	uint8  idx_min = 0, idx_max = 31, idx_curr = 0, idx_curr1 = 0, stall_val;
	uint8  need_more_gain, reduce_gain, adjust_pga, final_step;
	int16  idle_tssi[PHY_CORE_MAX], tone_tssi[PHY_CORE_MAX];
	int16  target_tssi, delta_tssi, delta_tssi1;

	struct _orig_reg_vals {
		uint8 core;
		uint16 orig_OVR3;
		uint16 orig_auxpga_cfg1;
		uint16 orig_auxpga_vmid;
		uint16 orig_iqcal_cfg1;
		uint16 orig_tx5g_tssi;
		uint16 orig_pa2g_tssi;
		uint16 orig_RfctrlIntc;
		uint16 orig_RfctrlOverrideRxPus;
		uint16 orig_RfctrlCoreRxPu;
		uint16 orig_RfctrlOverrideAuxTssi;
		uint16 orig_RfctrlCoreAuxTssi1;
		uint16 orig_RfctrlOverrideTxPus;
		uint16 orig_RfctrlCoreTxPus;
	} orig_reg_vals[PHY_CORE_MAX];

	uint core_count = 0;
	uint8 max_pad_idx = 31;

	txgain_setting_t curr_gain, curr_gain1;
	bool init_adc_inside = FALSE;
	uint16 save_afePuCtrl, save_gpio, save_gpioHiOutEn;
	uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
	uint32 save_chipc = 0;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	BCM_REFERENCE(stf_shdata);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	/* prevent crs trigger */
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

	band_idx = CHSPEC_ISPHY5G6G(pi->radio_chanspec);

	if (CHSPEC_IS80(pi->radio_chanspec) || PHY_AS_80P80(pi, pi->radio_chanspec)) {
		band_bw_idx = band_idx * 2 + 2;
	} else if (CHSPEC_IS160(pi->radio_chanspec)) {
		band_bw_idx = band_idx * 2 + 3;
		ASSERT(0); // FIXME
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		band_bw_idx = band_idx * 2 + 1;
	} else {
		band_bw_idx = band_idx * 2 + 0;
	}

	majorrev_idx = 1;
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	/* Turn off epa/ipa and unused rxrf part to prevent energy go into air */
	FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {

		/* save phy/radio regs going to be touched */
		orig_reg_vals[core_count].orig_RfctrlIntc = READ_PHYREGCE(pi, RfctrlIntc, core);
		orig_reg_vals[core_count].orig_RfctrlOverrideRxPus
			= READ_PHYREGCE(pi, RfctrlOverrideRxPus, core);
		orig_reg_vals[core_count].orig_RfctrlCoreRxPu
			= READ_PHYREGCE(pi, RfctrlCoreRxPus, core);
		orig_reg_vals[core_count].orig_RfctrlOverrideAuxTssi
			= READ_PHYREGCE(pi, RfctrlOverrideAuxTssi, core);
		orig_reg_vals[core_count].orig_RfctrlCoreAuxTssi1
			= READ_PHYREGCE(pi, RfctrlCoreAuxTssi1, core);

		orig_reg_vals[core_count].orig_OVR3 = READ_RADIO_REGC(pi, RF, OVR3, core);
		orig_reg_vals[core_count].orig_auxpga_cfg1 =
			READ_RADIO_REGC(pi, RF, AUXPGA_CFG1, core);
		orig_reg_vals[core_count].orig_auxpga_vmid =
			READ_RADIO_REGC(pi, RF, AUXPGA_VMID, core);
		orig_reg_vals[core_count].orig_iqcal_cfg1 =
			READ_RADIO_REGC(pi, RF, IQCAL_CFG1, core);
		orig_reg_vals[core_count].orig_tx5g_tssi = READ_RADIO_REGC(pi, RF, TX5G_TSSI, core);
		orig_reg_vals[core_count].orig_pa2g_tssi = READ_RADIO_REGC(pi, RF, PA2G_TSSI, core);
		orig_reg_vals[core_count].orig_RfctrlOverrideTxPus =
			READ_PHYREGCE(pi, RfctrlOverrideTxPus, core);
		orig_reg_vals[core_count].orig_RfctrlCoreTxPus =
			READ_PHYREGCE(pi, RfctrlCoreTxPus, core);
		orig_reg_vals[core_count].core = core;

		/* set proper Av/Vmid */
		MOD_RADIO_REGC(pi, OVR3, core, ovr_auxpga_i_sel_gain, 0x1);
		MOD_RADIO_REGC(pi, AUXPGA_CFG1, core,
		               auxpga_i_sel_gain, avvmid_set_local[band_idx][0]);
		MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_auxpga_i_sel_vmid, 0x1);
		MOD_RADIO_REGC(pi, AUXPGA_VMID, core,
		               auxpga_i_sel_vmid, avvmid_set_local[band_idx][1]);

		/* turn off ext-pa and put ext-trsw in r position */
		WRITE_PHYREGCE(pi, RfctrlIntc, core, 0x1400);
		/* turn off iPA */
		if (PHY_IPA(pi)) {
			MOD_PHYREGCE(pi, RfctrlOverrideTxPus,  core, pa_pwrup, 1);
			MOD_PHYREGCE(pi, RfctrlCoreTxPus,  core, pa_pwrup, 0);
		}
		/* set tssi_range = 0 (it suppose to bypass 10dB attenuation before pdet) */
		MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, tssi_range, 1);
		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, tssi_range, 0);

		/* turn off lna and other unsed rxrf components */
		WRITE_PHYREGCE(pi, RfctrlOverrideRxPus, core, 0x7CE0);
		WRITE_PHYREGCE(pi, RfctrlCoreRxPus,     core, 0x0);

		++core_count;
	}
	ACPHY_ENABLE_STALL(pi, stall_val);

	/* tssi loopback setup */
	phy_ac_tssi_loopback_path_setup(pi, LOOPBACK_FOR_IQCAL);

	if (!init_adc_inside) {
		wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
		                      &save_chipc, &fval2g_orig, &fval5g_orig,
		                      &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);
	}

	FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
		if (!init_adc_inside)
			wlc_phy_gpiosel_acphy(pi, 16+core, 1);
		/* Measure the Idle TSSI */
		wlc_phy_poll_samps_WAR_acphy(pi, idle_tssi, TRUE, TRUE, NULL,
		                             TRUE, init_adc_inside, core, 0);
		/* Adjust Target TSSI based on Idle TSSI */
		target_tssi = target_tssi_set[majorrev_idx][band_bw_idx] + idle_tssi[core];

		/* set the initial txgain */
		wlc_phy_get_txgain_settings_by_index_acphy(pi, &curr_gain, 0);
		pad_gain = gain_ladder[start_gain_idx[majorrev_idx][band_bw_idx][0]];
		pga_gain = gain_ladder[start_gain_idx[majorrev_idx][band_bw_idx][1]];
		curr_gain.rad_gain_mi = (pad_gain & 0xFF) | ((pga_gain & 0xFF) << 8);
		curr_gain.bbmult = 64;

		/* Measure the tone TSSI */
		wlc_phy_poll_samps_WAR_acphy(pi, tone_tssi, TRUE, FALSE,
		                             &curr_gain, TRUE, init_adc_inside, core, 0);
		delta_tssi  = target_tssi - tone_tssi[core];
		need_more_gain = (delta_tssi >= delta_tssi_error);
		reduce_gain = (delta_tssi < -delta_tssi_error);
		if (need_more_gain || reduce_gain) {
			/* if need more gain, try first max pad gain; otherwise, try min pga gain */
			curr_gain1 = curr_gain;
			curr_gain1.rad_gain_mi = (need_more_gain)
				? (
				   (curr_gain.rad_gain_mi & 0xFF00)
				   |((gain_ladder[max_pad_idx] & 0xFF) << 0))
				: (
				   (curr_gain.rad_gain_mi & 0x00FF)
				   |((gain_ladder[0]  & 0xFF) << 8));

			wlc_phy_poll_samps_WAR_acphy(pi, tone_tssi, TRUE, FALSE,
			                             &curr_gain1, TRUE, init_adc_inside, core, 0);
			delta_tssi1 = target_tssi - tone_tssi[core];
			adjust_pga = (delta_tssi1 >= 0);

			if (need_more_gain) {
				idx_min = start_gain_idx[majorrev_idx][band_bw_idx][adjust_pga];
				idx_max = max_pad_idx;
				curr_gain.rad_gain_mi = (adjust_pga)?
					curr_gain1.rad_gain_mi : curr_gain.rad_gain_mi;
				curr_gain1  = curr_gain;
				delta_tssi1 = (adjust_pga)? delta_tssi1: delta_tssi;
			} else if (reduce_gain) {
				idx_min = 0;
				idx_max = start_gain_idx[majorrev_idx][band_bw_idx][adjust_pga];
				curr_gain.rad_gain_mi = (adjust_pga)?
					curr_gain.rad_gain_mi : curr_gain1.rad_gain_mi;
				curr_gain1  = curr_gain;
				delta_tssi1 = (adjust_pga)? delta_tssi: delta_tssi1;
			}

			final_step = 0;
			do {
				if (idx_min >= idx_max-1) {
					final_step = 1;
					idx_curr = (idx_curr == idx_min)? idx_max: idx_min;
				} else {
					idx_curr = (idx_min + idx_max) >> 1;
				}

				if (adjust_pga) {
					curr_gain.rad_gain_mi =
						(curr_gain.rad_gain_mi & 0x00FF) |
						((gain_ladder[idx_curr] & 0xFF) << 8);
				} else {
					curr_gain.rad_gain_mi =
						(curr_gain.rad_gain_mi & 0xFF00) |
						((gain_ladder[idx_curr] & 0xFF) << 0);
				}

				wlc_phy_poll_samps_WAR_acphy(pi, tone_tssi, TRUE, FALSE,
				                             &curr_gain, TRUE,
				                             init_adc_inside, core, 0);
				delta_tssi  = target_tssi - tone_tssi[core];

				if (final_step) {
					if (ABS(delta_tssi) > ABS(delta_tssi1)) {
						idx_min  = idx_curr1;
						idx_max  = idx_curr1;
						idx_curr = idx_curr1;
						delta_tssi = delta_tssi1;
						curr_gain  = curr_gain1;
					} else {
						idx_min  = idx_curr;
						idx_max  = idx_curr;
					}
				} else {
					if (delta_tssi >= delta_tssi_error) {
						idx_min = idx_curr;
					} else if (delta_tssi < -delta_tssi_error) {
						idx_max = idx_curr;
					} else {
						idx_min = idx_curr;
						idx_max = idx_curr;
					}

					/* always log the current tssi & gain */
					delta_tssi1 = delta_tssi;
					curr_gain1  = curr_gain;
					idx_curr1   = idx_curr;
				}

				/* only used for debugging print */
				if (0) {
					if (adjust_pga) {
						printf("PGA-idx = (%d,%d,%d), dtssi = (%d, %d)\n",
						       idx_min, idx_curr, idx_max,
						       delta_tssi, delta_tssi1);
					} else {
						printf("PAD-idx = (%d,%d,%d), dtssi = (%d, %d)\n",
						       idx_min, idx_curr, idx_max,
						       delta_tssi, delta_tssi1);
					}
				}

			} while (idx_min < idx_max);
		}
		/* assign the best found gain */
		target_gains[core] = curr_gain;

		PHY_TRACE(("Best txgain found for Core%d: (%2x %2x %2x %2x %2x %2x)\n",
		           core, (target_gains[core].rad_gain & 0xF),
		           (target_gains[core].rad_gain >> 4) & 0xF,
		           (target_gains[core].rad_gain >> 8) & 0xFF,
		           (target_gains[core].rad_gain_mi >> 0) & 0xFF,
		           (target_gains[core].rad_gain_mi >> 8) & 0xFF,
		           (target_gains[core].rad_gain_hi >> 0) & 0xFF));
	}

	if (!init_adc_inside)
		wlc_phy_restore_after_adc_read(pi, &save_afePuCtrl, &save_gpio,
		                               &save_chipc,  &fval2g_orig,  &fval5g_orig,
		                               &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

	/* restore phy/radio regs */
	while (core_count > 0) {
		--core_count;
		phy_utils_write_radioreg(pi, RF_2069_OVR3(orig_reg_vals[core_count].core),
			orig_reg_vals[core_count].orig_OVR3);
		phy_utils_write_radioreg(pi, RF_2069_AUXPGA_CFG1(orig_reg_vals[core_count].core),
			orig_reg_vals[core_count].orig_auxpga_cfg1);
		phy_utils_write_radioreg(pi, RF_2069_AUXPGA_VMID(orig_reg_vals[core_count].core),
			orig_reg_vals[core_count].orig_auxpga_vmid);
		phy_utils_write_radioreg(pi, RF_2069_IQCAL_CFG1(orig_reg_vals[core_count].core),
			orig_reg_vals[core_count].orig_iqcal_cfg1);
		phy_utils_write_radioreg(pi, RF_2069_TX5G_TSSI(orig_reg_vals[core_count].core),
			orig_reg_vals[core_count].orig_tx5g_tssi);
		phy_utils_write_radioreg(pi, RF_2069_PA2G_TSSI(orig_reg_vals[core_count].core),
			orig_reg_vals[core_count].orig_pa2g_tssi);
		WRITE_PHYREGCE(pi, RfctrlIntc, orig_reg_vals[core_count].core,
			orig_reg_vals[core_count].orig_RfctrlIntc);
		WRITE_PHYREGCE(pi, RfctrlOverrideRxPus, orig_reg_vals[core_count].core,
			orig_reg_vals[core_count].orig_RfctrlOverrideRxPus);
		WRITE_PHYREGCE(pi, RfctrlCoreRxPus, orig_reg_vals[core_count].core,
			orig_reg_vals[core_count].orig_RfctrlCoreRxPu);
		WRITE_PHYREGCE(pi, RfctrlOverrideAuxTssi, orig_reg_vals[core_count].core,
			orig_reg_vals[core_count].orig_RfctrlOverrideAuxTssi);
		WRITE_PHYREGCE(pi, RfctrlCoreAuxTssi1, orig_reg_vals[core_count].core,
			orig_reg_vals[core_count].orig_RfctrlCoreAuxTssi1);
		if (PHY_IPA(pi)) {
			WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, orig_reg_vals[core_count].core,
			orig_reg_vals[core_count].orig_RfctrlOverrideTxPus);
			WRITE_PHYREGCE(pi, RfctrlCoreTxPus, orig_reg_vals[core_count].core,
			orig_reg_vals[core_count].orig_RfctrlCoreTxPus);
		}
	}

	/* prevent crs trigger */
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	PHY_TRACE(("======= IQLOCAL PreCalGainControl : END =======\n"));

	return;
}

void
wlc_phy_txcal_txgain_setup_acphy(phy_info_t *pi, txgain_setting_t *txcal_txgain,
	txgain_setting_t *orig_txgain)
{
	uint8 core;
	uint8 stall_val;
	uint8 phyrxchain;
	BCM_REFERENCE(phyrxchain);

	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;

	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		if (core == 3) {
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x501, 16,
				&(orig_txgain[core].rad_gain));
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x504, 16,
				&(orig_txgain[core].rad_gain_mi));
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x507, 16,
				&(orig_txgain[core].rad_gain_hi));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x501, 16,
				&(txcal_txgain[core].rad_gain));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x504, 16,
				&(txcal_txgain[core].rad_gain_mi));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x507, 16,
				&(txcal_txgain[core].rad_gain_hi));
		} else {
			/* store off orig and set new tx radio gain */
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x100 + core), 16,
				&(orig_txgain[core].rad_gain));
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x103 + core), 16,
				&(orig_txgain[core].rad_gain_mi));
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x106 + core), 16,
				&(orig_txgain[core].rad_gain_hi));

			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x100 + core), 16,
				&(txcal_txgain[core].rad_gain));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x103 + core), 16,
				&(txcal_txgain[core].rad_gain_mi));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x106 + core), 16,
				&(txcal_txgain[core].rad_gain_hi));
		}

		PHY_NONE(("\n radio gain = 0x%x %x %x, bbm=%d, dacgn = %d  \n",
			txcal_txgain[core].rad_gain_hi,
			txcal_txgain[core].rad_gain_mi,
			txcal_txgain[core].rad_gain,
			txcal_txgain[core].bbmult,
			txcal_txgain[core].dac_gain));

		/* store off orig and set new bbmult gain */
		wlc_phy_get_tx_bbmult_acphy(pi, &(orig_txgain[core].bbmult),  core);
		wlc_phy_set_tx_bbmult_acphy(pi, &(txcal_txgain[core].bbmult), core);
	}
	ACPHY_ENABLE_STALL(pi, stall_val);
}

void
wlc_phy_txcal_txgain_cleanup_acphy(phy_info_t *pi, txgain_setting_t *orig_txgain)
{
	uint8 core;
	uint8 stall_val;
	uint8 phyrxchain;
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);

	BCM_REFERENCE(phyrxchain);

	ACPHY_DISABLE_STALL(pi);
	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		/* restore gains: DAC, Radio and BBmult */
		if (core == 3) {
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x501, 16,
				&(orig_txgain[core].rad_gain));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x504, 16,
				&(orig_txgain[core].rad_gain_mi));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x507, 16,
				&(orig_txgain[core].rad_gain_hi));
		} else {
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x100 + core), 16,
				&(orig_txgain[core].rad_gain));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x103 + core), 16,
				&(orig_txgain[core].rad_gain_mi));
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x106 + core), 16,
				&(orig_txgain[core].rad_gain_hi));
		}
		wlc_phy_set_tx_bbmult_acphy(pi, &(orig_txgain[core].bbmult), core);
	}
	ACPHY_ENABLE_STALL(pi, stall_val);
}

static void
wlc_phy_txcal_phy_setup_acphy_core(phy_info_t *pi, txiqcal_phyregs_t *porig, uint8 core,
	uint16 bw_idx, uint16 sdadc_config, uint8 Biq2byp)
{
	/* Power Down External PA (simply always do 2G & 5G),
	 * and set T/R to T (double check TR position)
	 */
	porig->RfctrlIntc[core] = READ_PHYREGCE(pi, RfctrlIntc, core);
	WRITE_PHYREGCE(pi, RfctrlIntc, core, 0);
	MOD_PHYREGCE(pi, RfctrlIntc, core, ext_2g_papu, 0);
	MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 0);
	MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, 0);
	MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, 0);
	MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);
	MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 1);

	/* Core Activate/Deactivate */
	/* MOD_PHYREG(pi, RfseqCoreActv2059, DisRx, 0);
	   MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, (1 << core));
	 */

	/* Internal RFCtrl: save and adjust state of internal PA override */
	/* save state of Rfctrl override */
	porig->RfctrlOverrideAfeCfg[core]   = READ_PHYREGCE(pi, RfctrlOverrideAfeCfg, core);
	porig->RfctrlCoreAfeCfg1[core]      = READ_PHYREGCE(pi, RfctrlCoreAfeCfg1, core);
	porig->RfctrlCoreAfeCfg2[core]      = READ_PHYREGCE(pi, RfctrlCoreAfeCfg2, core);
	porig->RfctrlOverrideRxPus[core]    = READ_PHYREGCE(pi, RfctrlOverrideRxPus, core);
	porig->RfctrlCoreRxPus[core]        = READ_PHYREGCE(pi, RfctrlCoreRxPus, core);
	porig->RfctrlOverrideTxPus[core]    = READ_PHYREGCE(pi, RfctrlOverrideTxPus, core);
	porig->RfctrlCoreTxPus[core]        = READ_PHYREGCE(pi, RfctrlCoreTxPus, core);
	porig->RfctrlOverrideLpfSwtch[core] = READ_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core);
	porig->RfctrlCoreLpfSwtch[core]     = READ_PHYREGCE(pi, RfctrlCoreLpfSwtch, core);
	porig->RfctrlOverrideLpfCT[core]    = READ_PHYREGCE(pi, RfctrlOverrideLpfCT, core);
	porig->RfctrlCoreLpfCT[core]        = READ_PHYREGCE(pi, RfctrlCoreLpfCT, core);
	porig->RfctrlCoreLpfGmult[core]     = READ_PHYREGCE(pi, RfctrlCoreLpfGmult, core);
	porig->RfctrlCoreRCDACBuf[core]     = READ_PHYREGCE(pi, RfctrlCoreRCDACBuf, core);
	porig->RfctrlOverrideAuxTssi[core]  = READ_PHYREGCE(pi, RfctrlOverrideAuxTssi, core);
	porig->RfctrlCoreAuxTssi1[core]     = READ_PHYREGCE(pi, RfctrlCoreAuxTssi1, core);

	/* Turning off all the RF component that are not needed */
	wlc_phy_txcal_phy_setup_acphy_core_disable_rf(pi, core);
	if ((!(CHSPEC_IS2G(pi->radio_chanspec))) && ACMAJORREV_4(pi->pubpi->phy_rev)) {
		MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, tssi_range, 1);
		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, tssi_range, 0);
	}
	/* Setting the loopback path */
	if (Biq2byp == 0) {
	  wlc_phy_txcal_phy_setup_acphy_core_loopback_path(pi, core, LPFCONF_TXIQ_RX2);
	} else {
	  /* select biq2 byp path */
	  wlc_phy_txcal_phy_setup_acphy_core_loopback_path(pi, core, LPFCONF_TXIQ_RX4);
	}

	/* Setting the SD-ADC related stuff */
	if (!(ACMAJORREV_GE40_NE47(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_GE47(pi->pubpi->phy_rev)))
		wlc_phy_txcal_phy_setup_acphy_core_sd_adc(pi, core, sdadc_config);

	/* Setting the LPF related stuff */
	wlc_phy_txcal_phy_setup_acphy_core_lpf(pi, core, bw_idx);

	/* disable PAPD (if enabled)
	 * FIXME: not supported (and not needed) yet
	 * porig->PapdEnable[core] = READ_PHYREGCE(pi, PapdEnable, core);
	 * MOD_PHYREGCE(pi, PapdEnable, core, compEnable, 0);
	 */
}

static void
wlc_phy_txcal_phy_setup_acphy_core_disable_rf(phy_info_t *pi, uint8 core)
{
	if (TINY_RADIO(pi)) {
		if ((CHSPEC_IS2G(pi->radio_chanspec) &&
			(READ_RADIO_REGFLD_TINY(pi, PA2G_CFG1, core, pa2g_tssi_ctrl_sel) == 0)) ||
			(CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
			(READ_RADIO_REGFLD_TINY(pi, TX5G_MISC_CFG1, core,
			pa5g_tssi_ctrl_sel) == 0))) {
			MOD_PHYREGCE(pi, RfctrlCoreTxPus,     core, pa_pwrup,       1);
		}
	} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,     core, pa_pwrup,       1);
	} else {
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,     core, pa_pwrup,       0);
	}

	MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, pa_pwrup,               1);

	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna1_pwrup,        1);
	MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, rxrf_lna1_pwrup,        0);
	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna1_5G_pwrup,     1);
	MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, rxrf_lna1_5G_pwrup,     0);
	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna2_pwrup,        1);
	MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, rxrf_lna2_pwrup,        0);
	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, lpf_nrssi_pwrup,        1);
	MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, lpf_nrssi_pwrup,        0);
	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rssi_wb1g_pu,           1);
	MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, rssi_wb1g_pu,           0);
	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rssi_wb1a_pu,           1);
	MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, rssi_wb1a_pu,           0);
	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, lpf_wrssi3_pwrup,       1);
	MOD_PHYREGCE(pi, RfctrlCoreTxPus,     core, lpf_wrssi3_pwrup,       0);
	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna2_wrssi2_pwrup, 1);
	MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, rxrf_lna2_wrssi2_pwrup, 0);
}

static void
wlc_phy_txcal_phy_setup_acphy_core_loopback_path(phy_info_t *pi, uint8 core, uint8 lpf_config)
{
	MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, lpf_pu_dc,              1);
	MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, lpf_pu_dc,              1);
	MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, tssi_pu,             1);
	MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, tssi_pu,             1);

	if (!ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, lpf_bq1_pu,             1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,     core, lpf_bq1_pu,             1);
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, lpf_bq2_pu,             1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,     core, lpf_bq2_pu,             1);
		MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, lpf_pu,                 1);
		MOD_PHYREGCE(pi, RfctrlCoreTxPus,     core, lpf_pu,                 1);

		WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, 0x3ff);
		if (lpf_config == LPFCONF_TXIQ_RX2) {
			WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, 0x152);
		} else {
			WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, 0x22a);
		}
	}
}

void
wlc_phy_txcal_phy_setup_acphy_core_sd_adc(phy_info_t *pi, uint8 core, uint16 sdadc_config)
{
	MOD_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, afe_iqadc_mode, sdadc_config & 0x7);
	MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, afe_iqadc_mode, 1);
	MOD_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, afe_iqadc_pwrup, (sdadc_config >> 3) & 0x3f);
	MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, afe_iqadc_pwrup, 1);
	MOD_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, afe_iqadc_flashhspd, (sdadc_config >> 9) & 0x1);
	MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, afe_iqadc_flashhspd, 1);
	MOD_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, afe_ctrl_flash17lvl, (sdadc_config >> 10) & 0x1);
	MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, afe_ctrl_flash17lvl, 1);
	MOD_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, afe_iqadc_adc_bias, (sdadc_config >> 11) & 0x3);
	MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, afe_iqadc_adc_bias,  1);
}

static void
wlc_phy_txcal_phy_setup_acphy_core_lpf(phy_info_t *pi, uint8 core, uint16 bw_idx)
{
	if (!ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
		MOD_PHYREGCE(pi, RfctrlOverrideLpfCT,    core, lpf_bq1_bw,    1);
	}
	if (!ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		MOD_PHYREGCE(pi, RfctrlOverrideLpfCT,    core, lpf_bq2_bw,    1);
		MOD_PHYREGCE(pi, RfctrlOverrideLpfCT,    core, lpf_rc_bw,     1);

		MOD_PHYREGCE(pi, RfctrlCoreLpfCT,        core, lpf_bq1_bw,    3+bw_idx);
	}

	if (ACMAJORREV_2(pi->pubpi->phy_rev) ||
		ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		if (CHSPEC_IS80(pi->radio_chanspec) ||
			PHY_AS_80P80(pi, pi->radio_chanspec)) {
			MOD_PHYREGCE(pi, RfctrlCoreLpfCT,     core, lpf_bq2_bw,  6);
			MOD_PHYREGCE(pi, RfctrlCoreRCDACBuf,  core, lpf_rc_bw,   6);
		} else if (CHSPEC_IS160(pi->radio_chanspec)) {
			MOD_PHYREGCE(pi, RfctrlCoreLpfCT,     core, lpf_bq2_bw,  7);
			MOD_PHYREGCE(pi, RfctrlCoreRCDACBuf,  core, lpf_rc_bw,   7);
		} else {
			MOD_PHYREGCE(pi, RfctrlCoreLpfCT,     core, lpf_bq2_bw,  5);
			MOD_PHYREGCE(pi, RfctrlCoreRCDACBuf,  core, lpf_rc_bw,   5);
		}
	} else if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
		if (ACMINORREV_2(pi)) {
			if (CHSPEC_IS80(pi->radio_chanspec)) {
				MOD_PHYREGCE(pi, RfctrlCoreLpfCT,     core, lpf_bq2_bw,  6);
				MOD_PHYREGCE(pi, RfctrlCoreRCDACBuf,  core, lpf_rc_bw,   6);
			} else {
				MOD_PHYREGCE(pi, RfctrlCoreLpfCT,     core, lpf_bq2_bw,  5);
				MOD_PHYREGCE(pi, RfctrlCoreRCDACBuf,  core, lpf_rc_bw,   5);
			}
		} else {
			MOD_PHYREGCE(pi, RfctrlCoreLpfCT,     core, lpf_bq2_bw,  3+bw_idx);
			MOD_PHYREGCE(pi, RfctrlCoreRCDACBuf,  core, lpf_rc_bw,   3+bw_idx);
		}
	}

	MOD_PHYREGCE(pi, RfctrlOverrideLpfCT,    core, lpf_q_biq2,    1);
	MOD_PHYREGCE(pi, RfctrlCoreLpfCT,        core, lpf_q_biq2,    0);
	MOD_PHYREGCE(pi, RfctrlOverrideLpfCT,    core, lpf_dc_bypass, 1);
	MOD_PHYREGCE(pi, RfctrlCoreLpfCT,        core, lpf_dc_bypass, 0);
	MOD_PHYREGCE(pi, RfctrlOverrideLpfCT,    core, lpf_dc_bw,     1);
	MOD_PHYREGCE(pi, RfctrlCoreLpfCT,        core, lpf_dc_bw,     4);  /* 133KHz */

	if (!(ACMAJORREV_2(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev) ||
		ACMAJORREV_GE40(pi->pubpi->phy_rev) ||
		ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))) {
		MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, amux_sel_port, 1);
		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, amux_sel_port, 2);
		MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, afe_iqadc_aux_en, 1);
		MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, afe_iqadc_aux_en, 1);
	}
}

static void
wlc_phy_poll_adc_acphy(phy_info_t *pi, int32 *adc_buf, uint8 nsamps,
                       bool switch_gpiosel, uint16 core, bool is_tssi)
{
	/* Switching gpiosel is time consuming. We move the switch to an outer layer */
	/* and do it less frequently. */
	uint8 samp = 0;
	uint8 word_swap_flag = 1;
	uint8 gpiosel = 16;
	uint16 gpioEn_orig = 0;

	ASSERT(core < PHY_CORE_MAX);
	adc_buf[2*core] = 0;
	adc_buf[2*core+1] = 0;

	if ((phy_get_phymode(pi) == PHYMODE_MIMO) &&
		(ACMAJORREV_4(pi->pubpi->phy_rev) && (is_tssi == 1))) {
		if (core == 0) {
			wlapi_exclusive_reg_access_core0(pi->sh->physhim, 1);
		} else {
			wlapi_exclusive_reg_access_core1(pi->sh->physhim, 1);
		}
	}

	if (switch_gpiosel) {
		gpiosel += (ACMAJORREV_4(pi->pubpi->phy_rev) && (is_tssi == 1)) ? 0 : core;
		if (ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
			gpioEn_orig = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
			MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);
		}
		wlc_phy_gpiosel_acphy(pi, gpiosel, word_swap_flag);
	}

	for (samp = 0; samp < nsamps; samp++) {
		if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
			(is_tssi == 1) && (core == 3)) {
			uint16 bitw_scale = (ACMAJORREV_32(pi->pubpi->phy_rev)) ? 4 : 1;
			phy_iq_est_t rx_iq_est[PHY_CORE_MAX];
			MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 1);
			wlc_phy_rx_iq_est_acphy(pi, rx_iq_est, 8, 32, 0, TRUE);
			adc_buf[2*core] = 0;
			adc_buf[2*core+1] = (int32)
				math_sqrt_int_32((uint32)(rx_iq_est[core].q_pwr >> 3))*bitw_scale;
			adc_buf[2*core+1] = ((1 << 10) - adc_buf[2*core+1]) << 3;
			MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);
		} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
		           !ACMAJORREV_128(pi->pubpi->phy_rev)) {
			int tmp;
			uint16 tssi_val = 0;
			ASSERT(core < 4);
			if (core == 0) {
				MOD_PHYREG(pi, TxPwrCtrl_enable0, force_tssi_en, 1);
				OSL_DELAY(50);
				MOD_PHYREG(pi, TxPwrCtrl_enable0, force_tssi_en, 0);
				tssi_val = READ_PHYREG(pi, TssiVal_path0);
			} else if (core == 1) {
				MOD_PHYREG(pi, TxPwrCtrl_enable1, force_tssi_en, 1);
				OSL_DELAY(50);
				MOD_PHYREG(pi, TxPwrCtrl_enable1, force_tssi_en, 0);
				tssi_val = READ_PHYREG(pi, TssiVal_path1);
			} else if (core == 2) {
				MOD_PHYREG(pi, TxPwrCtrl_enable2, force_tssi_en, 1);
				OSL_DELAY(50);
				MOD_PHYREG(pi, TxPwrCtrl_enable2, force_tssi_en, 0);
				tssi_val = READ_PHYREG(pi, TssiVal_path2);
			} else if (core == 3) {
				MOD_PHYREG(pi, TxPwrCtrl_enable3, force_tssi_en, 1);
				OSL_DELAY(50);
				MOD_PHYREG(pi, TxPwrCtrl_enable3, force_tssi_en, 0);
				tssi_val = READ_PHYREG(pi, TssiVal_path3);
			}
			tmp = tssi_val & 0x3FF;
			tmp = tmp > 511 ? (tmp - 1024) : tmp;
			adc_buf[2*core] += tmp;
			adc_buf[2*core+1] += tmp;
		} else {
			/* read out the i-value */
			adc_buf[2*core] += READ_PHYREG(pi, gpioHiOut);
			/* read out the q-value */
			adc_buf[2*core+1] += READ_PHYREG(pi, gpioLoOut);
		}
	}

	if (switch_gpiosel && ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, gpioClkControl, gpioEn, gpioEn_orig);
	}

	if ((phy_get_phymode(pi) == PHYMODE_MIMO) &&
		(ACMAJORREV_4(pi->pubpi->phy_rev) && (is_tssi == 1))) {
		if (core == 0) {
			wlapi_exclusive_reg_access_core0(pi->sh->physhim, 0);
		} else {
			wlapi_exclusive_reg_access_core1(pi->sh->physhim, 0);
		}
	}
}

void
wlc_phy_poll_samps_acphy(phy_info_t *pi, int16 *samp, bool is_tssi,
                         uint8 log2_nsamps, bool init_adc_inside,
                         uint16 core)
{
	int32 adc_buf[2*PHY_CORE_MAX];
	int32 k, tmp_samp, samp_accum[PHY_CORE_MAX];
	uint8 iq_swap;

	ASSERT(core < PHY_CORE_MAX);

	/* initialization */
	samp_accum[core] = 0;

	iq_swap = (is_tssi)? 1 : 0;

	// this adc_reset is not included in TCL, we could consider bypass it
	// in DRV using condition "if (!ACMAJORREV_GE130(pi->pubpi->phy_rev))"
	wlc_phy_pulse_adc_reset_acphy(pi);
	OSL_DELAY(100);

	/* tssi val is (adc >> 2) */
	for (k = 0; k < (1 << log2_nsamps); k++) {
		wlc_phy_poll_adc_acphy(pi, adc_buf, 1, init_adc_inside, core, is_tssi);
		if (ACMAJORREV_4(pi->pubpi->phy_rev) ||
			ACMAJORREV_128(pi->pubpi->phy_rev)) {
			/* 13-10bit converstion In 4349A0 */
			tmp_samp = adc_buf[2*core+iq_swap] >> 3;
		} else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			tmp_samp = adc_buf[2*core+iq_swap] >> 3;
		} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
			tmp_samp = adc_buf[2*core+iq_swap];
		} else {
			tmp_samp = adc_buf[2*core+iq_swap] >> 2;
		}
		tmp_samp -= (tmp_samp < 512) ? 0 : 1024;
		samp_accum[core] += tmp_samp;
	}

	samp[core] = (int16) (samp_accum[core] >> log2_nsamps);
}

static void phy_ac_txiqlocal_txiqccget(phy_type_txiqlocal_ctx_t *ctx, void *a)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int32 iqccValues[2];
	uint16 valuea = 0;
	uint16 valueb = 0;

	wlc_acphy_get_tx_iqcc(pi, &valuea, &valueb);
	iqccValues[0] = valuea;
	iqccValues[1] = valueb;
	bcopy(iqccValues, a, 2*sizeof(int32));
}

static void phy_ac_txiqlocal_txiqccset(phy_type_txiqlocal_ctx_t *ctx, void *p)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int32 iqccValues[2];
	uint16 valuea, valueb;

	bcopy(p, iqccValues, 2*sizeof(int32));
	valuea = (uint16)(iqccValues[0]);
	valueb = (uint16)(iqccValues[1]);
	wlc_acphy_set_tx_iqcc(pi, valuea, valueb);
}

static void phy_ac_txiqlocal_txloccget(phy_type_txiqlocal_ctx_t *ctx, void *a)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint16 di0dq0;
	uint8 *loccValues = a;

	/* copy the 6 bytes to a */
	di0dq0 = wlc_acphy_get_tx_locc(pi, 0);
	loccValues[0] = (uint8)(di0dq0 >> 8);
	loccValues[1] = (uint8)(di0dq0 & 0xff);
	wlc_acphy_get_radio_loft(pi, &loccValues[2], &loccValues[3],
		&loccValues[4], &loccValues[5]);
}

static void phy_ac_txiqlocal_txloccset(phy_type_txiqlocal_ctx_t *ctx, void *p)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
		/* copy 6 bytes from a to radio */
	uint16 di0dq0;
	uint8 *loccValues = p;

	di0dq0 = ((uint16)loccValues[0] << 8) | loccValues[1];
	wlc_acphy_set_tx_locc(pi, di0dq0, 0);
	wlc_acphy_set_radio_loft(pi, loccValues[2],
		loccValues[3], loccValues[4], loccValues[5]);
}

#if defined(WLTEST)
static int
phy_ac_txiqlocal_set_calidx(phy_type_txiqlocal_ctx_t *ctx, int32 calidx)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	if (calidx >= -1 && calidx <= 127)
		info->txiqcalidx_iovar = calidx;
	return BCME_OK;
}

static int
phy_ac_txiqlocal_get_calidx(phy_type_txiqlocal_ctx_t *ctx, int32* calidx)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	*calidx = (int32)info->txiqcalidx_iovar;
	return BCME_OK;
}
static int
phy_ac_txiqlocal_set_target_tssi(phy_type_txiqlocal_ctx_t *ctx, int32 target_tssi)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	info->txiqcal_target_tssi_iovar = target_tssi;
	info->txiqcal_target_tssi_iovar_set = 1;
	return BCME_OK;
}

static int
phy_ac_txiqlocal_get_target_tssi(phy_type_txiqlocal_ctx_t *ctx, int32* target_tssi)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	*target_tssi = (int32)info->txiqcal_target_tssi_iovar;
	return BCME_OK;
}

static int
phy_ac_txiqlocal_set_tssi_search_enable(phy_type_txiqlocal_ctx_t *ctx, int32 tssi_search_en)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	info->txiqcal_tssisearch_en = tssi_search_en;
	if (tssi_search_en == 0) {
		info->txiqcal_target_tssi_iovar_set = 0;
	}
	return BCME_OK;
}

static int
phy_ac_txiqlocal_get_tssi_search_enable(phy_type_txiqlocal_ctx_t *ctx, int32 *tssi_search_en)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	*tssi_search_en = (int32)info->txiqcal_tssisearch_en;
	return BCME_OK;
}
#endif

static void
BCMATTACHFN(phy_ac_txiqlocal_nvram_attach)(phy_ac_txiqlocal_info_t *ti)
{
	ti->aci->sromi->oob_gaint = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(ti->pi, rstr_oob_gaint, 0);
	ti->txiqcalidx_iovar = -1;
	ti->txiqcal_target_tssi_iovar = 200 + (200*1000) + (200*1000000);
	ti->txiqcal_target_tssi_iovar_set = 0;
	ti->txiqcal_tssisearch_en = 0;
}
/* ********************************************* */
/*				External Definitions					*/
/* ********************************************* */

void
wlc_phy_populate_tx_loftcoefluts_acphy(phy_info_t *pi, uint8 core, uint16 coeffs,
uint8 start_idx, uint8 stop_idx)
{
	uint8 idx, tbl_idx;
	uint8 sz = stop_idx - start_idx + 1;
	uint16 *coeff_list;

	if ((coeff_list = phy_malloc(pi, (sizeof(uint16) * sz))) == NULL) {
		ASSERT(0);
		return;
	}

	idx = 0;
	for (tbl_idx = start_idx; tbl_idx <= stop_idx; tbl_idx ++) {
		coeff_list[idx] = coeffs;
		idx++;
	}
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LOFTCOEFFLUTS(core), sz,
	                          start_idx, 16, coeff_list);

	phy_mfree(pi, coeff_list, sizeof(uint16) * sz);
}

void
wlc_phy_populate_tx_loft_comp_tbl_acphy(phy_info_t *pi, uint16 *loft_coeffs)
{
	/* There are 2 implementations: */
	/* 1) 4360Bx fix, with two subbands and two power steps per core. */
	/* 2) 43602 fix, with three subbands and three power steps per core. Tuned for 43602MCH5. */
	/* Controlled by boardflag "precal_tx_idx". This turns on IDX change during cal. */
	uint8  core, tbl_idx;
	uint8  nwords;
	uint8  txidx_thresh[2][3] = {{128, 128, 32}, {128, 26, 28}};
	uint8  txidx_thresh2[3][2][3] = {{{128, 17, 12}, {128, 27, 22}}, /* Low band */
	                                 {{128, 19, 16}, {128, 29, 26}}, /* Mid Band */
	                                 {{128, 22, 21}, {128, 32, 31}}}; /* High Band */
	/* core, band, pwridx (low then hi) */
	uint8  di_bias_tbl[3][2][2] = {{{0x00, 0x00}, {0x00, 0x00}},
	                               {{0x00, 0x00}, {0xec, 0xf6}},
	                               {{0xf6, 0xf8}, {0xf6, 0xfa}}};
	uint8  dq_bias_tbl[3][2][2] = {{{0x00, 0x00}, {0x00, 0x00}},
	                               {{0x00, 0x00}, {0x00, 0xfb}},
	                               {{0xf2, 0xfc}, {0xf6, 0xfb}}};
	/* Core (0 1 2), Band (low, mid, high), Power IDX (low, mid, high) */
	uint8  di_bias_tbl2[3][3][3] = {{{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	    {0x00, 0x00, 0x00}}, {{0xfa, 0x00, 0x06}, {0xfa, 0x00, 0x04}, {0xf8, 0xff, 0x04}},
	    {{0xfe, 0xfd, 0xf9}, {0xfa, 0xfd, 0xfb}, {0xf8, 0xfe, 0x02}}};
	uint8  dq_bias_tbl2[3][3][3] = {{{0x00, 0x00, 0x00}, {0x00, 0x00, 0x00},
	    {0x00, 0x00, 0x00}}, {{0x05, 0x01, 0x01}, {0x05, 0x00, 0xfc}, {0x04, 0x01, 0xfa}},
	    {{0xfa, 0x02, 0x08}, {0xf9, 0x00, 0x08}, {0xf9, 0xff, 0x05}}};
	uint8  i_bias, q_bias, lowrange, highrange, pwr_idx, pwr_idx_alt, delta1, delta2;
	uint8  tmp1, tmp2, idxp, idxs, i_bias_pri, i_bias_sec, q_bias_pri, q_bias_sec;
	uint8  band_idx[3] = {0, 0, 0};
	uint8  ch_num_thresh[3] = {200, 149, 100};
	/* dividing in 3 bands for each core */
	uint8  ch_num_thresh_alt[2][3] = {{200, 100, 100}, {200, 149, 149}};
	int16  delta, i_bias_delta, q_bias_delta;
	uint16 coeffs, di, dq, di_adj, dq_adj, channel;
	const uint8 lo = 0;
	const uint8 hi = 1;
	/* no radio loft comp for tiny radio */
	if (TINY_RADIO(pi))
		return;
	/* code is only applicable to 43602 and 4360 (3 cores max) */
	ASSERT(PHYCORENUM((pi)->pubpi->phy_corenum) <= 3);
	nwords = 1;
	channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	FOREACH_CORE(pi, core) {
		if (!(pi->sromi->precal_tx_idx))
			band_idx[core] = channel >= ch_num_thresh[core];
		else
			band_idx[core] = (channel >= ch_num_thresh_alt[1][core]) ? 2 :
			    ((channel >= ch_num_thresh_alt[0][core]) ? 1 : 0);
	}
	for (tbl_idx = 0; tbl_idx < 128; tbl_idx ++) {
		FOREACH_CORE(pi, core) {

			switch (core) {
			case 0:
				coeffs = loft_coeffs[core];
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LOFTCOEFFLUTS0, nwords,
				    tbl_idx, 16, &coeffs);
				break;
			case 1:
			case 2:
				if ((CHSPEC_IS2G(pi->radio_chanspec) == 1) ||
				    ((pi->sromi->precal_tx_idx) && (channel < 52))) {
					coeffs = loft_coeffs[core];
				} else {
					if (pi->sromi->precal_tx_idx) {
						/* Interpolation: */
						/* bias = bias_pri + dY/dX * delta, where: */
						/* dY = delta between closest table elements */
						/* dX = 10 (#IDX ticks between 2 table elements) */
						/* bias_pri = table element to start from */
						/* delta = idx delta from bias_pri to target idx */
						tmp1 = tbl_idx <=
							txidx_thresh2[band_idx[core]][lo][core];
						tmp2 = tbl_idx <=
							txidx_thresh2[band_idx[core]][hi][core];
						pwr_idx_alt = (tmp1) ? 2 : ((tmp2) ? 1 : 0);
						delta1 = tbl_idx -
							txidx_thresh2[band_idx[core]][lo][core];
						delta2 = tbl_idx -
							txidx_thresh2[band_idx[core]][hi][core];
						if ((delta1 <= 5) || (delta1 > 251)) {
							lowrange = 1;
							highrange = 0;
						} else if ((delta2 <= 5) || (delta2 > 251)) {
							highrange = 1;
							lowrange = 0;
						} else {
							highrange = 0;
							lowrange = 0;
						}
						if (lowrange) {
							idxp = 2;
							idxs = 1;
							delta = (int16) (delta1 + 5);
						} else if (highrange) {
							idxp = 1;
							idxs = 0;
							delta = (int16) (delta2 + 5);
						} else {
							/* Outside table range, bias = constant */
							idxp = pwr_idx_alt;
							idxs = pwr_idx_alt;
							delta = 0;
						}
						/* starting interpolation from -primary- element */
						i_bias_pri =
						    di_bias_tbl2[core][band_idx[core]][idxp];
						q_bias_pri =
						    dq_bias_tbl2[core][band_idx[core]][idxp];
						/* towards this -secondary- element */
						i_bias_sec =
						    di_bias_tbl2[core][band_idx[core]][idxs];
						q_bias_sec =
						    dq_bias_tbl2[core][band_idx[core]][idxs];
						/* Computing dY */
						i_bias_delta = (uint8) (i_bias_sec - i_bias_pri);
						q_bias_delta = (uint8) (q_bias_sec - q_bias_pri);
						/* converting to int */
						i_bias_delta = (i_bias_delta > 127) ? i_bias_delta -
						    256 : i_bias_delta;
						q_bias_delta = (q_bias_delta > 127) ? q_bias_delta -
						    256 : q_bias_delta;
						/* Delta is distance from primary */
						delta = (delta > 127)? delta - 256:delta;
						/* Interpolation: integer divison is good enough */
						i_bias_delta = (i_bias_delta * delta) / 10;
						q_bias_delta = (q_bias_delta * delta) / 10;
						i_bias = (uint8) (i_bias_pri + i_bias_delta);
						q_bias = (uint8) (q_bias_pri + q_bias_delta);
					} else {
						pwr_idx = tbl_idx <=
						    txidx_thresh[band_idx[core]][core];
						i_bias = di_bias_tbl[core][band_idx[core]][pwr_idx];
						q_bias = dq_bias_tbl[core][band_idx[core]][pwr_idx];
					}

					dq = loft_coeffs[core] & 0xff;
					di = (loft_coeffs[core] >> 8) & 0xff;
					di_adj = (di + i_bias) & 0xff;
					dq_adj = (dq + q_bias) & 0xff;

					/* for overflow protection */
					if ((di_adj & 0x80) && (i_bias & 0x80) &&
					            ((di_adj & 0x80) == 0)) {
						di_adj = 0x80;
					} else if (((di_adj & 0x80) == 0) &&
					            ((i_bias & 0x80) == 0) && (di_adj & 0x80)) {
						di_adj = 0x7f;
					}

					if ((dq_adj & 0x80) && (q_bias & 0x80) &&
					            ((dq_adj & 0x80) == 0)) {
						dq_adj = 0x80;
					} else if (((dq_adj & 0x80) == 0) &&
					            ((q_bias & 0x80) == 0) && (dq_adj & 0x80)) {
						dq_adj = 0x7f;
					}
					/* dq: 8LSB & di: 8MSB */
					coeffs = dq_adj + (di_adj << 8);
				}

				if (core == 1) {
					wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LOFTCOEFFLUTS1,
					      nwords, tbl_idx, 16, &coeffs);
				} else {
					wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LOFTCOEFFLUTS2,
					      nwords, tbl_idx, 16, &coeffs);
				}
				break;
			}
		}
	}
}

uint8
phy_txiqlocal_num_multilo(phy_info_t *pi)
{
	txiqcal_params_t * paramsi = pi->u.pi_acphy->txiqlocali->paramsi;

	/* returns the number of multi-point LO cals */
	if (ACMAJORREV_47_129_130(pi->pubpi->phy_rev) &&
			(CHSPEC_ISPHY5G6G(pi->radio_chanspec) == 1)) {
		/* 43684 and 6710MCM: do 2 multi-point LO cals for 5G. */
		return paramsi->num_multilo;
	} else {
		return 0;
	}
}

void
wlc_phy_txiqlocal_lopwr_gettblidx(phy_info_t *pi, txiqlocal_multilo_t *multilo_cal,
	uint8 multilo_cal_cnt)
{
	txiqcal_params_t * paramsi = pi->u.pi_acphy->txiqlocali->paramsi;

	/* 43684, 6710A0: Settings for multi-point LO cal */
	/* this is for populating after doing the calibration */
	multilo_cal->lofttbl_start_idx = paramsi->multilo_start_idx[multilo_cal_cnt-1];
	multilo_cal->lofttbl_end_idx = paramsi->multilo_end_idx[multilo_cal_cnt-1];

}

void
wlc_phy_precal_txgain_acphy(phy_info_t *pi, txgain_setting_t *target_gains,
	uint8 multilo_cal_cnt, bool Biq2byp)
{
	/*   This function determines the tx gain settings to be
	 *   used during tx iqlo calibration; that is, it sends back
	 *   the following settings for each core:
	 *       - radio gain
	 *       - dac gain
	 *       - bbmult
	 *   This is accomplished by choosing a predefined power-index, or by
	 *   setting gain elements explicitly to predefined values, or by
	 *   doing actual "pre-cal gain control". Either way, the idea is
	 *   to get a stable setting for which the swing going into the
	 *   envelope detectors is large enough for good "envelope ripple"
	 *   while avoiding distortion or EnvDet overdrive during the cal.
	 *
	 *   Note:
	 *       - this function and the calling infrastructure is set up
	 *         in a way not to leave behind any modified state; this
	 *         is in contrast to mimophy ("nphy"); in acphy, only the
	 *         desired gain quantities are set/found and set back
	 */
	phy_ac_info_t *pi_ac = pi->u.pi_acphy;
	phy_ac_txiqlocal_info_t *ti = pi_ac->txiqlocali;
	txiqcal_params_t * paramsi = ti->paramsi;

	uint8 core;
	uint8 subband;
	uint8 phy_bw;
	acphy_cal_result_t *accal = &pi->cal_info->u.accal;

	uint8 en_precal_gain_control = 0;
	int8 tx_pwr_idx[3] = {20, 30, 20};
	const int8 tx_pwr_idx_5g[4][3] = {{20, 30, 30}, {20, 23, 20}, {20, 25, 20}, {20, 30, 20}};
	bool suspend = TRUE;
	uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	int16 idle_tssi[PHY_CORE_MAX], tone_tssi[PHY_CORE_MAX], tssi[PHY_CORE_MAX];

	BCM_REFERENCE(stf_shdata);

	if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
		/* Disable PAPD */
		FOREACH_CORE(pi, core) {
			ti->papdState[core] = READ_PHYREGCE(pi, PapdEnable, core);
			MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 0);
		}
	}

	if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
		if (!suspend) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		}
		wlc_phy_btcx_override_enable(pi);
	}
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	/* reset ladder_updated flags so tx-iqlo-cal ensures appropriate recalculation */
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		accal->txiqlocal_ladder_updated[core] = 0;
	}
	/* phy_bw */
	if (CHSPEC_IS80(pi->radio_chanspec) ||
		PHY_AS_80P80(pi, pi->radio_chanspec)) {
		phy_bw = 80;
	} else if (CHSPEC_IS160(pi->radio_chanspec)) {
		phy_bw = 160;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		phy_bw = 40;
	} else {
		phy_bw = 20;
	}

	/* Enable Precal gain control only for 4335 */
	if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		en_precal_gain_control = 2;
	} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		if (!ROUTER_4349(pi) && !PHY_IPA(pi)) {
			en_precal_gain_control = 4;
		} else {
			en_precal_gain_control = 0;
		}
	}

	if (IS_4364_3x3(pi)) {
	   en_precal_gain_control = 0;
	}

	if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
	   en_precal_gain_control = ti->txiqcal_tssisearch_en;
	}

	if (en_precal_gain_control == 0) {
		/* get target tx gain settings */
		FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
			/* specify tx gain by index (reads from tx power table) */
			int8 target_pwr_idx;
			if (ACREV_IS(pi->pubpi->phy_rev, 1) || (ACMAJORREV_5(pi->pubpi->phy_rev) &&
				!(ACMINORREV_2(pi)))) {
				/* for 4360B0 and 43602 using 0.5dB-step, idx is lower */
				subband = phy_ac_chanmgr_get_chan_freq_range(pi,
					0, PRIMARY_FREQ_SEGMENT);
				if ((pi->sromi->precal_tx_idx) &&
				    CHSPEC_ISPHY5G6G(pi->radio_chanspec) && (subband >= 1)) {
					--subband;
					target_pwr_idx = tx_pwr_idx_5g[subband][core];
				} else {
					target_pwr_idx = (core != 0) ? 30 : 20;
					if (phy_ac_chanmgr_get_chan_freq_range(pi, 0,
					PRIMARY_FREQ_SEGMENT) == WL_CHAN_FREQ_RANGE_5G_BAND3) {
						target_pwr_idx = tx_pwr_idx[core];
					}
				}
			} else if (ACMAJORREV_5(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) {
			   /* for 4364_3x3 tx_idx 30 is being used */
			   target_pwr_idx = 30;
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) &&
			           (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) &&
			           !(ACRADIO_2069_EPA_IS(pi->pubpi->radiorev))) {
				if (CHSPEC_IS2G(pi->radio_chanspec) == 1) {
					target_pwr_idx = 1;
				} else {
					if (phy_bw == 20)
						target_pwr_idx = 0;
					else if (phy_bw == 40)
						target_pwr_idx = 15;
					else
						target_pwr_idx = 10;
				}
			} else if (TINY_RADIO(pi)) {
				if (PHY_IPA(pi)) {
					if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
						if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
							target_pwr_idx = ROUTER_4349(pi) ?
								(IS_ACR(pi) ? 70 : 40) : 40;
						} else {
							target_pwr_idx = 50;
						}
					} else {
						if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
							target_pwr_idx = 62;
						} else {
							target_pwr_idx = 50;
						}
					}
				} else {
					if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
						if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
							if (RADIOREV(pi->pubpi->radiorev) == 10) {
								target_pwr_idx = 4;
							} else {
								if (phy_get_phymode(pi)
								== PHYMODE_MIMO) {
									target_pwr_idx =
									(core == 0)
										? 10 : 16;
								} else {
									target_pwr_idx
									= (phy_get_current_core(pi)
									== PHY_RSBD_PI_IDX_CORE0)
										? 10 : 16;
								}
							}
						} else {
							if (ROUTER_4349(pi))
								target_pwr_idx = 10;
							else
								target_pwr_idx = 0;
						}
					} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
						ACMAJORREV_33(pi->pubpi->phy_rev)) {
						target_pwr_idx = 35;
					} else {
						if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
							if (pi->txgaintbl5g == 1) {
								target_pwr_idx = 60;
							} else {
								target_pwr_idx = 62;
							}
						} else {
							target_pwr_idx = 50;
						}
					}
				}
				if (CHSPEC_IS2G(pi->radio_chanspec) &&
						(pi->txiqlocali->data->txiqcalidx2g != -1)) {
					target_pwr_idx = pi->txiqlocali->data->txiqcalidx2g;
				} else if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
					(pi->txiqlocali->data->txiqcalidx5g != -1)) {
					target_pwr_idx = pi->txiqlocali->data->txiqcalidx5g;
				}
			} else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					target_pwr_idx = 30;
				} else {
					if (PHY_IPA(pi)) {
						target_pwr_idx = 30;
					} else {
						target_pwr_idx = 40;
					}
				}
			} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
				if (multilo_cal_cnt == 0) {
					if (ti->txiqcalidx_iovar != -1 && !Biq2byp) {
						target_pwr_idx = (ti->txiqcalidx_iovar % 100);
					} else {
						/* Aux TxIQLO and main TxIQ cal index */
						if (PHY_IPA(pi)) {
							if (CHSPEC_IS2G(pi->radio_chanspec)) {
								target_pwr_idx = 35;
							} else {
								target_pwr_idx = 44;
							}
						} else {
							target_pwr_idx = 20;
						}
					}
				} else {
					/* for multi LO cal, choose the right index */
					target_pwr_idx =
					    paramsi->multilo_cal_idx[multilo_cal_cnt-1];
				}
			} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
				if (multilo_cal_cnt == 0) {
					if (ti->txiqcalidx_iovar != -1 && !Biq2byp) {
						target_pwr_idx = ti->txiqcalidx_iovar;
					} else if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
						if (CHSPEC_IS2G(pi->radio_chanspec) &&
							!PHY_IPA(pi)) {
							target_pwr_idx = 20;
						} else {
							target_pwr_idx = 30;
						}
					} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
						if (CHSPEC_IS2G(pi->radio_chanspec)) {
							target_pwr_idx = 40;
						} else {
							target_pwr_idx = Biq2byp ? 40 : 20;
						}
					} else {
						target_pwr_idx = 20;
					}
				} else {
					target_pwr_idx =
					    paramsi->multilo_cal_idx[multilo_cal_cnt-1];
				}
			} else {
				target_pwr_idx = 30;
			}

			if (ACMAJORREV_129(pi->pubpi->phy_rev) && (ti->txiqcalidx_iovar != -1)) {
			/* Measure the Idle TSSI */
				wlc_phy_poll_samps_WAR_acphy(pi, idle_tssi, TRUE, TRUE,
					&(target_gains[core]), FALSE, TRUE, core, 0);
			}

			wlc_phy_get_txgain_settings_by_index_acphy(
				pi, &(target_gains[core]), target_pwr_idx);

			if (ACMAJORREV_129(pi->pubpi->phy_rev) && (ti->txiqcalidx_iovar != -1)) {
			/* Measure TSSI */
				wlc_phy_poll_samps_WAR_acphy(pi, tssi, TRUE, FALSE,
					&(target_gains[core]), FALSE, TRUE, core, 0);
				tone_tssi[core] = tssi[core] - idle_tssi[core];
				ti->txiqcal_target_tssi_iovar = tone_tssi[0]+(tone_tssi[1]*1000)
					+(tone_tssi[2]*1000000);
				PHY_TRACE(("Index = %3d target_TSSI = %4i\n",
					target_pwr_idx, ti->txiqcal_target_tssi_iovar));
			}

			/* use PA gain 255 for TXIQLOCAL for 2G/ipa/4349B0 */
			if ((CHSPEC_IS2G(pi->radio_chanspec) == 1) &&
				(ACMAJORREV_4(pi->pubpi->phy_rev))) {
				target_gains[core].rad_gain |= 0xff00;
				target_gains[core].bbmult = 20;
			}

			if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec) == 1) &&
			    (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) &&
			     (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1))) {
				/* use PAD gain 255 for TXIQLOCAL */
				target_gains[core].rad_gain_mi |= 0xff;
			}
			if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
				/* BW160 align with tcl setting,
				* bbmult and LO ladder and IQ ladder will be algined
				* in AC2PHY_TBL_ID_IQLOCAL phytble
				*/
				target_gains[core].bbmult = 64;
				if (multilo_cal_cnt != 0) target_gains[core].rad_gain |= 0xff00;
			}
		}

	} else if (en_precal_gain_control == 1) {
		wlc_phy_precal_target_tssi_search(pi, &(target_gains[0]));
	} else if (en_precal_gain_control == 2) {
		wlc_phy_precal_txgain_control(pi, &(target_gains[0]));
	} else if (en_precal_gain_control == 3) {
		ti->txiqcalidx_iovar = 0;
		FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
			wlc_phy_precal_target_tssi_fine_tune(pi, core, &(target_gains[core]));
		}
	} else if (en_precal_gain_control == 4) {
		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			wlc_txprecal4349_gain_control(pi, &(target_gains[0]));
		}
	} else if (en_precal_gain_control == 5) {
		ASSERT(0);
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
	if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		wlc_phy_btcx_override_disable(pi);
		if (!suspend) {
			wlapi_enable_mac(pi->sh->physhim);
		}
	}
	if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
		/* Restore PAPD state */
		FOREACH_CORE(pi, core) {
			WRITE_PHYREGCE(pi, PapdEnable, core, ti->papdState[core]);
		}
	}
}

static int phy_ac_txiqlocal(phy_info_t *pi, uint8 searchmode, bool Biq2byp, bool last_phase,
	uint8 multilo_cal_cnt);
static int phy_ac_txiqlocal_cmd(phy_ac_txiqlocal_info_t *ti, bool last_phase,
	int32 tone_freq, uint16 tone_ampl, int bcmerror, uint8 multilo_cal_cnt);
static int phy_ac_txiqlocal_cleanup(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp,
	txgain_setting_t orig_txgain[], bool suspend);

void
wlc_phy_cal_txiqlo_init_acphy(phy_info_t *pi)
{
	phy_ac_txiqlocal_info_t *ti = pi->u.pi_acphy->txiqlocali;
	ti->phase_id = 0;
}

void
phy_ac_txiqlocal_multiphase(phy_info_t *pi, uint8 searchmode, bool Biq2byp, uint16 cts_time,
	uint8 multilo_cal_cnt)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	phy_ac_txiqlocal_info_t *ti = pi_ac->txiqlocali;
	bool last_phase;

	wlc_phy_cts2self(pi, cts_time);

	wlc_phy_cal_txiqlo_acphy(pi, searchmode, TRUE, Biq2byp, multilo_cal_cnt);
	last_phase = (ti->phase_id >= (ti->num_mphases - 1));

	if (last_phase) {
		/* End of multiphases */
		ti->phase_id = 0;
		pi->cal_info->cal_phase_id++;
	} else {
		ti->phase_id++;
	}
}

void
wlc_phy_cal_txiqlo_acphy(phy_info_t *pi, uint8 searchmode, uint8 mphase, bool Biq2byp,
	uint8 multilo_cal_cnt)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	phy_ac_txiqlocal_info_t *ti = pi_ac->txiqlocali;
	bool first_phase, last_phase;

	/* Get all the Cal params */
	wlc_phy_txcal_set_cal_params(pi, searchmode, mphase, Biq2byp);
	first_phase = (mphase == 0) || (ti->phase_id == 0);
	last_phase = (mphase == 0) || (ti->phase_id >= (ti->num_mphases - 1));

	/* First Phase - call precal-txgain */
	if (first_phase)
		wlc_phy_precal_txgain_acphy(pi, pi->cal_info->u.accal.txcal_txgain,
			multilo_cal_cnt, Biq2byp);

	/* Call the Cal */
	if (phy_ac_txiqlocal(pi, searchmode, Biq2byp, last_phase, multilo_cal_cnt) != BCME_OK) {
		/* rare case, just reset */
		PHY_ERROR(("wlc_phy_cal_txiqlo_acphy failed\n"));
		phy_calmgr_mphase_reset(pi->calmgri);
		return;
	}
}

static int
phy_ac_txiqlocal(phy_info_t *pi, uint8 searchmode, bool Biq2byp, bool last_phase,
	uint8 multilo_cal_cnt)
{
	uint16 tone_ampl, tone_freq, *coeff_ptr;
	int    bcmerror = BCME_OK;
	uint8  core, k;
	txgain_setting_t orig_txgain[4];
	acphy_cal_result_t *accal = &pi->cal_info->u.accal;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	phy_ac_info_t *pi_ac = pi->u.pi_acphy;
	phy_ac_txiqlocal_info_t *ti = pi_ac->txiqlocali;

	/* zeros start coeffs (a,b,di/dq,ei/eq,fi/fq for each core) */
	uint16 start_coeffs_RESTART[] = {0, 0, 0, 0, 0,  0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,  0, 0, 0, 0, 0};
	bool suspend = TRUE;
#ifdef ATE_BUILD
	printf("===> Running Tx IQ Lo Cal.\n");
#endif /* ATE_BUILD */

	BCM_REFERENCE(stf_shdata);

	/* -------
	 *  Inits
	 * -------
	 */

	if (ISSIM_ENAB(pi->sh->sih)) {
		return BCME_OK;
	}

	bzero(ti->loft_coeffs, sizeof(ti->loft_coeffs));

	if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		/* Let WLAN have FEMCTRL to ensure cal is done properly */
		suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
		if (!suspend) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		}
		wlc_phy_btcx_override_enable(pi);
	}
	 /* Disable PAPD */
	FOREACH_CORE(pi, core) {
		ti->papdState[core] = READ_PHYREGCE(pi, PapdEnable, core);
		MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 0);
	}

	/* prevent crs trigger */
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	/* phy_bw */
	if (CHSPEC_IS80(pi->radio_chanspec) ||
		PHY_AS_80P80(pi, pi->radio_chanspec)) {
		ti->bw_idx = 2;
	} else if (CHSPEC_IS160(pi->radio_chanspec)) {
		ti->bw_idx = 3;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		ti->bw_idx = 1;
	} else {
		ti->bw_idx = 0;
	}

	/* Put the radio and phy into TX iqlo cal state, including tx gains */
	ti->classifier_state = READ_PHYREG(pi, ClassifierCtrl);
	phy_rxgcrs_sel_classifier(pi, 4);

	if (ACMAJORREV_47(pi->pubpi->phy_rev))
		wlc_phy_txcal_radio_setup_acphy_20698(ti, Biq2byp);
	else if (ACMAJORREV_51(pi->pubpi->phy_rev))
		wlc_phy_txcal_radio_setup_acphy_20704(ti, Biq2byp);
	else if (ACMAJORREV_128(pi->pubpi->phy_rev))
		wlc_phy_txcal_radio_setup_acphy_20709(ti, Biq2byp);
	else if (ACMAJORREV_129(pi->pubpi->phy_rev))
		wlc_phy_txcal_radio_setup_acphy_20707(ti, Biq2byp);
	else if (ACMAJORREV_130(pi->pubpi->phy_rev))
		wlc_phy_txcal_radio_setup_acphy_20708(ti, Biq2byp);
	else if (ACMAJORREV_131(pi->pubpi->phy_rev))
		wlc_phy_txcal_radio_setup_acphy_20710(ti, Biq2byp);
	else if (TINY_RADIO(pi))
		wlc_phy_txcal_radio_setup_acphy_tiny(pi);
	else
		wlc_phy_txcal_radio_setup_acphy(pi);

	wlc_phy_txcal_phy_setup_acphy(pi, Biq2byp);
	wlc_phy_txcal_txgain_setup_acphy(pi, &accal->txcal_txgain[0], &orig_txgain[0]);

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_47(pi->pubpi->phy_rev)) {
		/* In low rate TSSI mode, adc running low,
		 * use overrideds to configure ADC to normal mode
		 */
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20698_afe_div_ratio(pi, 1, 0, 0);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_51(pi->pubpi->phy_rev)) {
		/* In low rate TSSI mode, adc running low,
		 * use overrideds to configure ADC to normal mode
		 */
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20704_afe_div_ratio(pi, 1);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_128(pi->pubpi->phy_rev)) {
		/* In low rate TSSI mode, adc running low,
		 * use overrideds to configure ADC to normal mode
		 */
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20709_afe_div_ratio(pi, 1);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_129(pi->pubpi->phy_rev)) {
		/* In low rate TSSI mode, adc running low,
		 * use overrideds to configure ADC to normal mode
		 */
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20707_afe_div_ratio(pi, 1);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_130(pi->pubpi->phy_rev)) {
		/* In low rate TSSI mode, adc running low,
		 * use overrideds to configure ADC to normal mode
		 */
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20708_tx2cal_normal_adc_rate(pi, 1, 0);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_131(pi->pubpi->phy_rev)) {
		/* In low rate TSSI mode, adc running low,
		 * use overrideds to configure ADC to normal mode
		 */
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20710_afe_div_ratio(pi, 1, 0, FALSE);
	}

	/* 4350A0 FIXME: Add Jira
	 * Need to force gated clks on to allow iqcal_done to be cleared
	 * only needed for 80 MHz but enable for 20 and 40 MHz anyway
	 */
	if (!ACMAJORREV_130(pi->pubpi->phy_rev) &&
		!(CHSPEC_IS160(pi->radio_chanspec) && ACMAJORREV_47(pi->pubpi->phy_rev)))
		wlapi_bmac_phyclk_fgc(pi->sh->physhim, ON);

	/* This is different from TCL proc  for 4347, and thus for phy_maj44 as well */
	if (TINY_RADIO(pi)) {
		/* # no radio LOFT or programmable radio gain for tiny */
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, TX_iqcal_gain_bwAddress, 0)
			WRITE_PHYREG_ENTRY(pi, TX_loft_fine_iAddress, 0)
			WRITE_PHYREG_ENTRY(pi, TX_loft_fine_qAddress, 0)
			WRITE_PHYREG_ENTRY(pi, TX_loft_coarse_iAddress, 0)
			WRITE_PHYREG_ENTRY(pi, TX_loft_coarse_qAddress, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		/* BCM7271 CRDOT11ACPHY-2219 radioreg crash WAR */
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, TX_iqcal_gain_bwAddress, 0x51)
			WRITE_PHYREG_ENTRY(pi, TX_loft_fine_iAddress, 0x51)
			WRITE_PHYREG_ENTRY(pi, TX_loft_fine_qAddress, 0x51)
			WRITE_PHYREG_ENTRY(pi, TX_loft_coarse_iAddress, 0x51)
			WRITE_PHYREG_ENTRY(pi, TX_loft_coarse_qAddress, 0x51)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

	/* Set IQLO Cal Engine Gain Control Parameters including engine Enable
	 * Format: iqlocal_en<15> / gain start_index / NOP / ladder_length_d2)
	 */
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		WRITE_PHYREG(pi, iqloCalCmdGctl, 0x8a06);
	} else if (ACMAJORREV_2(pi->pubpi->phy_rev) && CHSPEC_IS80(pi->radio_chanspec)) {
		WRITE_PHYREG(pi, iqloCalCmdGctl, 0x8304);
	} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		const uint8 lad_len = ACMAJORREV_130(pi->pubpi->phy_rev)? 10: 12;
		const uint8 start_idx = 7;
		WRITE_PHYREG(pi, iqloCalCmdGctl, 0x8000 | (start_idx << 8) | (lad_len/2));
	} else {
		WRITE_PHYREG(pi, iqloCalCmdGctl, 0x8a09);
	}

	if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, fineclockgatecontrol, forcetxlbClkEn, 0xf);
	}

	/*
	 *   Retrieve and set Start Coeffs
	 */
	if (ti->phase_id > 0) {
		/* mphase cal and have done at least 1 Tx phase already */
		coeff_ptr = accal->txiqlocal_interm_coeffs; /* use results from previous phase */
	} else {
		/* single-phase cal or first phase of mphase cal */
		if ((searchmode == PHY_CAL_SEARCHMODE_REFINE) ||
		    (searchmode == PHY_CAL_SEARCHMODE_MULTILO)) {
			/* recal ("refine") */
			coeff_ptr = accal->txiqlocal_coeffs; /* use previous cal's final results */
		} else {
			/* start from zero coeffs ("restart") */
			coeff_ptr = start_coeffs_RESTART; /* zero coeffs */
		}
		/* copy start coeffs to intermediate coeffs, for pairwise update from here on
		 *    (after all cmds/phases have filled this with latest values, this
		 *    will be copied to OFDM/BPHY coeffs and to accal->txiqlocal_coeffs
		 *    for use by possible REFINE cal next time around)
		 */
		for (k = 0; k < 5*ti->num_cores; k++) {
			accal->txiqlocal_interm_coeffs[k] = coeff_ptr[k];
		}
	}
	FOREACH_CORE(pi, core) {
		wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeff_ptr + 5*core + 0,
		                                TB_START_COEFFS_AB, core);
		/* Restart or refine with Biq2byp option should not touch
		 * d,e,f coeffs. Can be bypassed due to: BCAWLAN-209401
		 */

		if (ti->prerxcal == 0 || ti->paramsi->reset_loftcoefs_prerxcal) {
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeff_ptr + 5*core + 2,
			                            TB_START_COEFFS_D,  core);
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeff_ptr + 5*core + 3,
			                            TB_START_COEFFS_E,  core);
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeff_ptr + 5*core + 4,
			                            TB_START_COEFFS_F,  core);
		}
	}
#ifdef WLC_TXFDIQ
	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			wlc_phy_tx_fdiqi_comp_acphy(pi, FALSE, 0xFF);
		}
	} else {
		wlc_phy_tx_fdiqi_comp_acphy(pi, FALSE, 0xFF);
	}
#endif

	/* turn on test tone */
	tone_ampl = 250;

	/* Fixing tone_ampl = 450/400/450 in 5g20/40/80 for 4364_3x3 */
	if (IS_4364_3x3(pi) && !(USE_OOB_GAINT(pi))) {
		if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) &&
		(CHSPEC_IS20(pi->radio_chanspec))) {
			tone_ampl = 450;
		} else if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) &&
		(CHSPEC_IS40(pi->radio_chanspec))) {
			tone_ampl = 400;
		} else if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) &&
		(CHSPEC_IS80(pi->radio_chanspec))) {
			tone_ampl = 450;
		}
	} else if (IS_4364_3x3(pi) && (USE_OOB_GAINT(pi))) {
		if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) &&
			(CHSPEC_IS20(pi->radio_chanspec))) {
				tone_ampl = 300;
		} else if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) &&
				(CHSPEC_IS40(pi->radio_chanspec))) {
			tone_ampl = 325;
		} else if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) &&
				(CHSPEC_IS80(pi->radio_chanspec))) {
			tone_ampl = 375;
		}
	}

	/* fixme: wlc_phy_tx_tone_acphy is playing 2x frequency.
	 * once that's fixed, we should use 4/8/12 mHz for iqlocal
	 */
	tone_freq = (CHSPEC_IS80(pi->radio_chanspec) ||
		PHY_AS_80P80(pi, pi->radio_chanspec)) ? ACPHY_IQCAL_TONEFREQ_8MHz :
		CHSPEC_IS160(pi->radio_chanspec) ? ACPHY_IQCAL_TONEFREQ_16MHz :
		CHSPEC_IS40(pi->radio_chanspec) ? ACPHY_IQCAL_TONEFREQ_4MHz :
		ACPHY_IQCAL_TONEFREQ_2MHz;

	if (!((ACMAJORREV_4(pi->pubpi->phy_rev)) || IS_4364_3x3(pi))) {
		tone_freq = tone_freq >> 1;
	}

	if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		bcmerror = BCME_OK;
	} else {
		MOD_PHYREG(pi, iqloCalCmdGctl, iqlo_cal_en, 0);
		bcmerror = wlc_phy_tx_tone_acphy(pi, (int32)tone_freq, tone_ampl,
			TX_TONE_IQCAL_MODE_ON, FALSE);
		OSL_DELAY(5);
	}

	if (TINY_RADIO(pi)) {
		/* #restore bbmult overwritten by tone */
		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			wlc_phy_ipa_set_bbmult_acphy(pi, &(accal->txcal_txgain[0].bbmult),
				&(accal->txcal_txgain[1].bbmult), NULL, NULL,
				pi->pubpi->phy_coremask);
		} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev)) {
			uint16 m[4] = {0, 0, 0, 0};

			FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
				m[core] = accal->txcal_txgain[core].bbmult;
			}
			wlc_phy_ipa_set_bbmult_acphy(pi, &m[0], &m[1], &m[2], &m[3],
				pi->pubpi->phy_coremask);

		}
		else {
			wlc_phy_ipa_set_bbmult_acphy(pi, &(accal->txcal_txgain[0].bbmult),
				NULL, NULL, NULL, pi->pubpi->phy_coremask);
		}
	}

	if (!ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
		FOREACH_CORE(pi, core) {
			MOD_PHYREGCE(pi, RfctrlCoreAfeCfg2,    core, afe_iqadc_reset_ov_det, 0);
			MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, afe_iqadc_reset_ov_det, 1);
		}
	}
	PHY_NONE(("wlc_phy_cal_txiqlo_acphy (after inits): SearchMd=%d,"
		" CmdIds=(%d to %d), Biq2byp=%d, prerxcal=%d\n",
		  searchmode, ti->cmd_idx, ti->cmd_stop_idx, Biq2byp, ti->prerxcal));

	bcmerror = phy_ac_txiqlocal_cmd(ti, last_phase,
	                                tone_freq, tone_ampl, bcmerror, multilo_cal_cnt);
	phy_ac_txiqlocal_cleanup(ti, Biq2byp, orig_txgain, suspend);

	return bcmerror;
}

static int
phy_ac_txiqlocal_cmd(phy_ac_txiqlocal_info_t *ti, bool last_phase,
                     int32 tone_freq, uint16 tone_ampl, int bcmerror, uint8 multilo_cal_cnt)
{
	phy_info_t *pi = ti->pi;
	uint8  rd_select = 0, wr_select1 = 0, wr_select2 = 0;
	uint8  cal_type, core;
	uint16 cmd, coeffs[2], zero = 0;
	acphy_cal_result_t *accal = &pi->cal_info->u.accal;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	uint8  thidx, thidx_start, thidx_stop;
	/* structure for saving txpu_ovrd, txpu_val
	 * 2 register values for each core
	 */
	struct _save_regs {
		uint16 reg_val;
		uint16 reg_addr;
	} savereg[PHY_CORE_MAX*2];
	uint   core_count = 0;
	uint   core_off = 0;
	txiqcal_params_t * paramsi = ti->paramsi;
	uint16 *loft_coeffs_ptr = ti->loft_coeffs;
	uint16 idx_for_loft_comp_tbl = 5;
	uint8 start_idx, end_idx;
#ifdef WLC_TXFDIQ
	int16 tone;
	int32 tx_fdiq_tone_list[ACPHY_TXCAL_MAX_NUM_FREQ] = {8, 4, -4, -8};
	acphy_fdiqi_t tfreq_ang_mag[ACPHY_TXCAL_MAX_NUM_FREQ];
	uint8 fdiq_loop = 0;
	math_cint32 tmp;
	math_cint32 tmp2;
	int32 ang;
	int fdiq_data_valid = 0;
	/* initialize the tfreq_ang_mag array */
	memset(&tfreq_ang_mag, 0x00, sizeof(tfreq_ang_mag));
#endif
	uint8 sweep_tone = 0;
	uint8 tone_idx = 0;
	uint16 stall_val;
	txgain_setting_t siso_txgain[PHY_CORE_MAX], dummygain[PHY_CORE_MAX];
	txgain_setting_t *txcal_txgain = &(pi->cal_info->u.accal).txcal_txgain[0];
	txiqlocal_multilo_t multilo_cal = {0, 0};
#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif

	BCM_REFERENCE(stf_shdata);

	/* ---------------
	 *  Cmd Execution
	 * ---------------
	 */
	if (bcmerror == BCME_OK) { /* in case tone doesn't start (still needed?) */

		/* loop over commands in this cal phase */
		for (; ti->cmd_idx <= ti->cmd_stop_idx; ti->cmd_idx++) {
			/* get command, cal_type, and core */
			core = ti->cmd_idx / ti->num_cmds_per_core; /* integer divide */
			if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
				if (phy_get_phymode(pi) == PHYMODE_MIMO) {
					uint8 off_core = 0;
					if (core == 0) {
						off_core = 1;
					} else {
						off_core = 0;
					}
					MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, off_core,
						ovr_pa5g_pu, 1);
					MOD_RADIO_REG_TINY(pi, PA5G_CFG4, off_core, pa5g_pu, 0);
					MOD_PHYREGCE(pi, RfctrlOverrideTxPus, off_core,
						txrf_pwrup, 0x1);
					MOD_PHYREGCE(pi, RfctrlCoreTxPus, off_core,
						txrf_pwrup, 0x0);
					MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
						ovr_pa5g_pu, 1);
					MOD_RADIO_REG_TINY(pi, PA5G_CFG4, core, pa5g_pu, 1);
					MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core,
						txrf_pwrup, 0x1);
					MOD_PHYREGCE(pi, RfctrlCoreTxPus, core, txrf_pwrup, 0x1);
				}
			}
			/* only execute commands when the current core is active
			 * if ((phy_stf_get_data(pi->stfi)->phytxchain >> core) & 0x1)
			 */
			/* Turn Off Inactive cores for 43602 to improve IQ cal */
			if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
				FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core_off) {
				if (core != core_off) {
				    savereg[core_count].reg_val =
				      READ_PHYREGCE(pi, RfctrlOverrideTxPus, core_off);
				    savereg[core_count].reg_addr =
				      ACPHYREGCE(pi, RfctrlOverrideTxPus, core_off);
				    ++core_count;
				    savereg[core_count].reg_val =
				      READ_PHYREGCE(pi, RfctrlCoreTxPus, core_off);
				    savereg[core_count].reg_addr =
				      ACPHYREGCE(pi, RfctrlCoreTxPus, core_off);
				    ++core_count;
				    MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core_off, txrf_pwrup, 1);
				    MOD_PHYREGCE(pi, RfctrlCoreTxPus, core_off, txrf_pwrup, 0);
				}
			    }
			}

			if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
				(ti->cmd_idx % ti->num_cmds_per_core == 0)) {
				// Before the 1st cmd for a core, set tx gains of other cores to 0.
				memset(siso_txgain, 0, PHY_CORE_MAX * sizeof(txgain_setting_t));
				memcpy(&siso_txgain[core], &txcal_txgain[core],
					sizeof(txgain_setting_t));
				wlc_phy_txcal_txgain_setup_acphy(pi, &siso_txgain[0], dummygain);
			}

			cmd = ti->cmds[ti->cmd_idx % ti->num_cmds_per_core] | 0x8000 | (core << 12);
			cal_type = ((cmd & 0x0F00) >> 8);
			sweep_tone = 1;
#ifdef WLC_TXFDIQ
			if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
				ACMAJORREV_33(pi->pubpi->phy_rev)) {
				if (CHSPEC_IS80(pi->radio_chanspec)) {
					fdiq_loop = (ti->cmds[ti->cmd_idx %
						ti->num_cmds_per_core] & 0xF00) >> 8;
					if (fdiq_loop == 1) {
						if ((fdiq_data_valid & (1 << core)) == 0) {
							fdiq_data_valid |= 1 << core;
						}
						cmd = (ti->cmds[ti->cmd_idx % ti->num_cmds_per_core]
							& 0xFF) | 0x8000 | (core << 12);
						cal_type = 0;
						sweep_tone = ACPHY_TXCAL_MAX_NUM_FREQ;
					}
				}
			} else {
				fdiq_loop = (ti->cmds[ti->cmd_idx %
					ti->num_cmds_per_core] & 0xF00) >> 8;
				if (fdiq_loop == 1) {
					if ((fdiq_data_valid & (1 << core)) == 0) {
						fdiq_data_valid |= 1 << core;
					}
					cmd = (ti->cmds[ti->cmd_idx % ti->num_cmds_per_core]
						& 0xFF) | 0x8000 | (core << 12);
					cal_type = 0;
					sweep_tone = ACPHY_TXCAL_MAX_NUM_FREQ;
				}
			}
#endif /* WLC_TXFDIQ */

				/* PHY_CAL(("wlc_phy_cal_txiqlo_acphy:
				 *  Cmds => cmd_idx=%2d, Cmd=0x%04x,	\
				 *  cal_type=%d, core=%d\n", cmd_idx, cmd, cal_type, core));
				 */

			for (tone_idx = 0; tone_idx < sweep_tone; tone_idx++) {
#ifdef WLC_TXFDIQ
				if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
					ACMAJORREV_33(pi->pubpi->phy_rev)) {
					if (CHSPEC_IS80(pi->radio_chanspec)) {
						if (fdiq_loop == 1) {
							tone = (int16)tx_fdiq_tone_list[tone_idx];
							tone = (tone * 1000) >> 1;
							bcmerror = wlc_phy_tx_tone_acphy(pi,
								(int32)tone, tone_ampl,
								TX_TONE_IQCAL_MODE_ON, FALSE);
							tfreq_ang_mag[tone_idx].freq =
								(int32)tx_fdiq_tone_list[tone_idx];
						}
					}
				} else {
					if (fdiq_loop == 1) {
						tone = (int16)tx_fdiq_tone_list[tone_idx];
						tone = (tone * 1000) >> 1;
						bcmerror = wlc_phy_tx_tone_acphy(pi, (int32)tone,
							tone_ampl, TX_TONE_IQCAL_MODE_ON, FALSE);
						tfreq_ang_mag[tone_idx].freq =
							(int32)tx_fdiq_tone_list[tone_idx];
					}
				}
#endif /* WLC_TXFDIQ */
				/* set up scaled ladders for desired bbmult of current core */
				if (!accal->txiqlocal_ladder_updated[core]) {
					if (TINY_RADIO(pi)) {
						wlc_phy_cal_txiqlo_update_ladder_acphy(pi,
							accal->txcal_txgain[core].bbmult, core);
					} else {
						wlc_phy_cal_txiqlo_update_ladder_acphy(pi,
							accal->txcal_txgain[core].bbmult, core);
						accal->txiqlocal_ladder_updated[core] = TRUE;
					}
					accal->txiqlocal_ladder_updated[core] = TRUE;
				}

				/* set intervals settling and measurement intervals */
				if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
					if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) &&
						!PHY_IPA(pi)) {
						WRITE_PHYREG(pi, iqloCalCmdNnum,
						(paramsi->nsamp_corrs_new[ti->cmd_idx] << 8) |
						paramsi->nsamp_gctrl[ti->bw_idx]);
					} else {
						WRITE_PHYREG(pi, iqloCalCmdNnum,
						(paramsi->nsamp_corrs_new[ti->bw_idx] << 8) |
						paramsi->nsamp_gctrl[ti->bw_idx]);
					}
				} else {
					WRITE_PHYREG(pi, iqloCalCmdNnum,
					(paramsi->nsamp_corrs[ti->bw_idx] << 8) |
					paramsi->nsamp_gctrl[ti->bw_idx]);
				}

				/* if coarse-analog-LOFT cal (fi/fq),
				 *     always zero out ei/eq and di/dq;
				 * if fine-analog-LOFT   cal (ei/dq),
				 *     always zero out di/dq
				 *   - even do this with search-type REFINE, to prevent a "drift"
				 *   - assumes that order of LOFT cal cmds will be f => e => d,
				 *     where it's ok to have multiple cmds (say interrupted by
				 *     IQ cal) of the same type
				 */
				if ((cal_type == CAL_TYPE_LOFT_ANA_COARSE) ||
				    (cal_type == CAL_TYPE_LOFT_ANA_FINE)) {
					if (!(ti->cmd_idx >= 5)) {
						wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
							&zero, TB_START_COEFFS_D, core);
					}
				}

				if (cal_type == CAL_TYPE_LOFT_ANA_COARSE) {
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
					        &zero, TB_START_COEFFS_E, core);
				}

				if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
					thidx_start = 2;
					thidx_stop = 3;
				} else {
					thidx_start = 0;
					thidx_stop = 6;
				}
				if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
				/* BW20, 40, 80, 160 tone gen timing is aligned with TCL
				*  to solve stability issue
				*/
					bcmerror = wlc_phy_tx_tone_acphy(pi, tone_freq, tone_ampl,
						TX_TONE_IQCAL_MODE_ON, FALSE);
					OSL_DELAY(5);
				}

				for (thidx = thidx_start; thidx < thidx_stop; thidx++) {
					/* Set thresh_d2 */
					WRITE_PHYREG(pi, iqloCalgtlthres,
							paramsi->thres_ladder[thidx]);

					/* now execute this command and wait max of ~20ms */
					if (ACMAJORREV_4(pi->pubpi->phy_rev) &&
						phy_get_phymode(pi) != PHYMODE_RSDB) {
						if (core == 1) {
							WRITE_PHYREG(pi, iqloCalCmd, cmd);
						} else {
							wlapi_exclusive_reg_access_core0(
								pi->sh->physhim, 1);
							WRITE_PHYREG(pi, iqloCalCmd, cmd);
							wlapi_exclusive_reg_access_core0(
								pi->sh->physhim, 0);
						}
					} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
						ACMAJORREV_33(pi->pubpi->phy_rev) ||
						ACMAJORREV_47_129_130(pi->pubpi->phy_rev)) {
						stall_val = READ_PHYREGFLD(pi, RxFeCtrl1,
							disable_stalls);
						ACPHY_DISABLE_STALL(pi);
						WRITE_PHYREG(pi, iqloCalCmd, cmd);
						ACPHY_ENABLE_STALL(pi, stall_val);
					} else {
						/* Preferred flow; it is dangerous to disable
						 * stalls before firing iqloCalCmd due to risk
						 *  of phase jumpsin test tone during calibration
						 */
						WRITE_PHYREG(pi, iqloCalCmd, cmd);
					}

					if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {

						/* Make sure mac is suspended */
						ASSERT(!(R_REG(pi->sh->osh, D11_MACCONTROL(pi))
							& MCTL_EN_MAC));

						/* Make sure txiqcl cmd has been written */
						ASSERT((READ_PHYREG(pi, iqloCalCmd) & 0x3fff)
							== (cmd & 0x3fff));

						SPINWAIT(((READ_PHYREG(pi, iqloCalCmd)
							& 0xc000) != 0),
							ACPHY_SPINWAIT_TXIQLO);
					} else {
						SPINWAIT(((READ_PHYREG(pi, iqloCalCmd)
							& 0xc000) != 0), ACPHY_SPINWAIT_TXIQLO);
					}

					if ((READ_PHYREG(pi, iqloCalCmd) & 0xc000) &&
					    !ACMAJORREV_32(pi->pubpi->phy_rev) &&
					    !ACMAJORREV_33(pi->pubpi->phy_rev) &&
					    !ACMAJORREV_47(pi->pubpi->phy_rev) &&
					    !ACMAJORREV_129(pi->pubpi->phy_rev) &&
					    !ACMAJORREV_130(pi->pubpi->phy_rev)) {
						PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :",
							__FUNCTION__));
						PHY_FATAL_ERROR_MESG(("TXIQLO cal failed \n"));
						PHY_FATAL_ERROR(pi, PHY_RC_TXIQLO_CAL_FAILED);
					}

					if (ACMAJORREV_4(pi->pubpi->phy_rev) ||
						ACMAJORREV_32(pi->pubpi->phy_rev) ||
						ACMAJORREV_33(pi->pubpi->phy_rev) ||
						ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
						break;
						//There is no ADC clamp detect
					} else if (wlc_poll_adc_clamp_status(pi, core, 1) == 0) {
						break;
					}

					PHY_CAL(("wlc_phy_cal_txiqlo_acphy: Cmds => cmd_idx=%2d,",
					         ti->cmd_idx));
					PHY_CAL(("Cmd=0x%04x, cal_type=%d, core=%d, ",
					         cmd, cal_type, core));
					PHY_CAL(("thresh_idx = %d\n", thidx));
				}

				if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
					wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_WO_CCA_RESET);
				}
				/* copy coeffs best-to-start and to
				 * "intermediate" coeffs in pi state; in mphase,
				 * the latter is also used as starting point
				 * when coming back for next phase, and
				 * we always use the "intermediate" coeffs at
				 * the very end to apply to OFDM/BPHY,
				 * see below;
				 * (copy step only done for coeff pair that
				 * changed, thereby also covering ei/eq swap
				 * per PR 79353)
				 */
				switch (cal_type) {
				case CAL_TYPE_IQ:
					rd_select  = TB_BEST_COEFFS_AB;
					wr_select1 = TB_START_COEFFS_AB;
					wr_select2 = PI_INTER_COEFFS_AB;
					break;
				case CAL_TYPE_LOFT_DIG:
					rd_select  = TB_BEST_COEFFS_D;
					wr_select1 = TB_START_COEFFS_D;
					wr_select2 = PI_INTER_COEFFS_D;
					break;
				case CAL_TYPE_LOFT_ANA_FINE:
					rd_select  = TB_BEST_COEFFS_E;
					wr_select1 = TB_START_COEFFS_E;
					wr_select2 = PI_INTER_COEFFS_E;
					break;
				case CAL_TYPE_LOFT_ANA_COARSE:
					rd_select  = TB_BEST_COEFFS_F;
					wr_select1 = TB_START_COEFFS_F;
					wr_select2 = PI_INTER_COEFFS_F;
					break;
				default:
					ASSERT(0);
				}
				wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
				                                coeffs, rd_select,  core);
				wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
				                                coeffs, wr_select1, core);
				if (ACREV_IS(pi->pubpi->phy_rev, 1) && ((ti->cmd_idx %
					ti->num_cmds_per_core) >= idx_for_loft_comp_tbl)) {
					/* write to the txpwrctrl tbls */
					*loft_coeffs_ptr++ = coeffs[0];
				}
				wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
				                                coeffs, wr_select2, core);
#ifdef WLC_TXFDIQ
				if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
					ACMAJORREV_33(pi->pubpi->phy_rev)) {
					if (CHSPEC_IS80(pi->radio_chanspec)) {
						if (cal_type == CAL_TYPE_IQ) {
							tmp.q = (int32)(int16)coeffs[0];
							tmp.i = (int32)((int16)coeffs[1]+1024);
							math_cmplx_invcordic(tmp, &ang);
							math_cmplx_cordic(ang, &tmp2);
							if (fdiq_loop == 1) {
							tfreq_ang_mag[tone_idx].angle[core] = ang;
							tfreq_ang_mag[tone_idx].mag[core] =
							((1024 + (int16)coeffs[1])<<16)/tmp2.i;
							}
						}
					}
				} else {
					if ((ti->txfdiqi->enabled == 1) &&
						(cal_type == CAL_TYPE_IQ)) {
						tmp.q = (int32)(int16)coeffs[0];
						tmp.i = (int32)((int16)coeffs[1]+1024);
						math_cmplx_invcordic(tmp, &ang);
						math_cmplx_cordic(ang, &tmp2);
						if (fdiq_loop == 1) {
						/* 2^16*ang in degrees */
						tfreq_ang_mag[tone_idx].angle[core] = ang;
						tfreq_ang_mag[tone_idx].mag[core] =
						((1024 + (int16)coeffs[1])<<16)/tmp2.i;
						}
					}
				}
#endif /* WLC_TXFDIQ */
				/* WAR for random spur issue in 1x1 which */
				/* happens when LOFT coeff is negative */
				if (ACMAJORREV_2(pi->pubpi->phy_rev) && (ti->cmd_idx == 4)) {
					uint16 coeffs_digi[2];
					coeffs_digi[0] = 0x0808;
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
						coeffs_digi, TB_BEST_COEFFS_D, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
						coeffs_digi, TB_START_COEFFS_D, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
						coeffs_digi, PI_INTER_COEFFS_D, core);
				}
			if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
			    /* Restore */
			    while (core_count > 0) {
				--core_count;
				phy_utils_write_phyreg(pi, savereg[core_count].reg_addr,
					savereg[core_count].reg_val);
			    }
			}
#ifdef WLC_TXFDIQ
		/* Restart tone for CAL on core1 after fdiq cal on core0
		* stopplayback below will stop for last core
		*/
		if ((tone_idx == (sweep_tone -1) && fdiq_loop == 1)) {
			bcmerror = wlc_phy_tx_tone_acphy(pi, (int32)tone_freq, tone_ampl,
			                                 TX_TONE_IQCAL_MODE_ON, FALSE);
			OSL_DELAY(5);
		}
#endif
			} /* sweep tone loop */
		} /* command loop */

		/* single phase or last tx stage in multiphase cal: apply & store overall results */
		if (last_phase) {
			if (ti->prerxcal == 0) {
				if (phy_txiqlocal_num_multilo(pi) != 0) {
					wlc_phy_txiqlocal_lopwr_gettblidx(pi, &multilo_cal,
						multilo_cal_cnt);
				}
				FOREACH_CORE(pi, core) {
					/* Save and Apply IQ Cal Results */
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ, coeffs,
					                                PI_INTER_COEFFS_AB, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeffs,
					                                PI_FINAL_COEFFS_AB, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeffs,
					                                TB_OFDM_COEFFS_AB,  core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeffs,
					                                TB_BPHY_COEFFS_AB,  core);

					/* Save and Apply Dig LOFT Cal Results */
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ, coeffs,
					                                PI_INTER_COEFFS_D, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeffs,
					                                PI_FINAL_COEFFS_D, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeffs,
					                                TB_OFDM_COEFFS_D,  core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeffs,
					                                TB_BPHY_COEFFS_D,  core);

					if (phy_txiqlocal_num_multilo(pi) != 0) {
						/* Save LOFT (COEFFS_D) coeff to table */
						start_idx = multilo_cal.lofttbl_start_idx;
						end_idx = multilo_cal.lofttbl_end_idx;
						wlc_phy_populate_tx_loftcoefluts_acphy(pi, core,
							coeffs[0],
							start_idx,
							end_idx);
					}

					/* Apply Analog LOFT Comp
					 * - unncessary if final command on each core is digital
					 * LOFT-cal or IQ-cal
					 * - then the loft comp coeffs were applied to radio
					 * at the beginning of final command per core
					 * - this is assumed to be the case, so nothing done here
					 */

					/* Save Analog LOFT Comp in PI State */
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ, coeffs,
					                                PI_INTER_COEFFS_E, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeffs,
					                                PI_FINAL_COEFFS_E, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ, coeffs,
					                                PI_INTER_COEFFS_F, core);
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE, coeffs,
					                                PI_FINAL_COEFFS_F, core);
					/* Print out Results */
					wlc_phy_txcal_coeffs_print(accal->txiqlocal_coeffs, core);
				} /* for core */

				/* Save into Cache */
#if defined(PHYCAL_CACHING)
				if (ctx)
					phy_ac_txiqlocal_save_cache(ti, ctx, multilo_cal_cnt);
#else
				wlc_phy_scanroam_cache_txcal_acphy(pi->u.pi_acphy->txiqlocali, 1);
#endif /* PHYCAL_CACHING */
			} else {
				FOREACH_CORE(pi, core) {
					/* Save IQ Cal coeffs for RX-cal */
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ, coeffs,
					                                PI_INTER_COEFFS_AB, core);
					accal->txiqcal_biq2byp_coeffs[core*2+0] = coeffs[0];
					accal->txiqcal_biq2byp_coeffs[core*2+1] = coeffs[1];
					wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ, coeffs,
					                                PI_INTER_COEFFS_D, core);
					accal->txlocal_biq2byp_coeffs[core] = coeffs[0];
					/* Print out Results a & b */
					PHY_CAL(("\tcore-%d: a/b = (%4d,%4d)", core,
					        (int16)accal->txiqcal_biq2byp_coeffs[core*2+0],
					        (int16)accal->txiqcal_biq2byp_coeffs[core*2+1]));
					PHY_CAL((", d = (%4d,%4d)\n",
					        (int8)(accal->txlocal_biq2byp_coeffs[core] >> 8),
					        (int8)(accal->txlocal_biq2byp_coeffs[core] & 255)));
				} /* for core */
			}

			/* validate availability of results and store off channel */
			accal->txiqlocal_coeffsvalid = TRUE;
		}

		/* Switch off test tone */
		if (!(ACMAJORREV_GE47(pi->pubpi->phy_rev))) {
			/* mimophy_stop_playback */
			wlc_phy_stopplayback_acphy(pi,
			                           STOPPLAYBACK_W_CCA_RESET);
		}

	} /* if BCME_OK */

	/* disable IQ/LO cal */
	WRITE_PHYREG(pi, iqloCalCmdGctl, 0x0000);

#ifdef WLC_TXFDIQ
	if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
	    ACMAJORREV_33(pi->pubpi->phy_rev)) {
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			if (fdiq_data_valid != 0) {
				phy_ac_txiqlocal_fdiqi_lin_reg(pi->u.pi_acphy->txiqlocali,
					tfreq_ang_mag, ACPHY_TXCAL_MAX_NUM_FREQ, fdiq_data_valid);
			}
		}
	} else {
		if (fdiq_data_valid != 0) {
		  phy_ac_txiqlocal_fdiqi_lin_reg(pi->u.pi_acphy->txiqlocali, tfreq_ang_mag,
		      ACPHY_TXCAL_MAX_NUM_FREQ, fdiq_data_valid);
		}
	}
#endif
	/* XXX FIXME: May consider saving off LOFT comp before 1st phase and
	 *  restoring LOFT comp  after each phase except for last phase
	 */
	return bcmerror;
}

static int
phy_ac_txiqlocal_cleanup(phy_ac_txiqlocal_info_t *ti, uint8 Biq2byp,
	txgain_setting_t orig_txgain[], bool suspend)
{
	uint8  core;
	phy_info_t *pi = ti->pi;
#ifdef ATE_BUILD
	uint16 ab_int[2];
	uint16 d_reg;
	uint16 coremask;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	acphy_cal_result_t *accal = &pi->cal_info->u.accal;
#endif
	/*
	 *-----------*
	 *  Cleanup  *
	 *-----------
	 */

	/* BCM7271 CRDOT11ACPHY-2219 radioreg crash WAR */
	if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		wlc_phy_resetcca_acphy(pi);
	}

	/* 4350A0 FIXME: Add Jira
	 * Remove forcing of gated clks
	 */

	if (!(CHSPEC_IS160(pi->radio_chanspec) && ACMAJORREV_47(pi->pubpi->phy_rev)))
		wlapi_bmac_phyclk_fgc(pi->sh->physhim, OFF);

	/* LOFT WAR for 4360 and 43602 */
	if ((ACREV_IS(pi->pubpi->phy_rev, 1) || ACMAJORREV_5(pi->pubpi->phy_rev)) &&
		!(Biq2byp && (pi->sromi->precal_tx_idx))) {
		/* Skipping for 43602 when Biq2byp = 0, ie, cal is RX. */
		/* Calling for 43602 when srom.precal_tx_idx is not set. */
		wlc_phy_populate_tx_loft_comp_tbl_acphy(pi, ti->loft_coeffs);
	}

	/* Workaround for Hang seen on 4347A0 -- JIRA:CRDOT11ACPHY-2219
	There should be no radio reg access before resetcca once iqcal
	start command is issued
	*/
	if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		wlc_phy_resetcca_acphy(pi);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_47(pi->pubpi->phy_rev)) {
		/* Remove afe_div overrides */
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20698_afe_div_ratio(pi, 0, 0, 0);
		MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_51(pi->pubpi->phy_rev)) {
		/* Remove afe_div overrides */
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20704_afe_div_ratio(pi, 0);
		MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_128(pi->pubpi->phy_rev)) {
		/* Remove afe_div overrides */
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20709_afe_div_ratio(pi, 0);
		MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_129(pi->pubpi->phy_rev)) {
		/* Remove afe_div overrides */
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20707_afe_div_ratio(pi, 0);
		MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_130(pi->pubpi->phy_rev)) {
		/* Remove afe_div overrides */
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20708_tx2cal_normal_adc_rate(pi, 0, 0);
		MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en &&
		ACMAJORREV_131(pi->pubpi->phy_rev)) {
		/* Remove afe_div overrides */
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20710_afe_div_ratio(pi, 0, 0, FALSE);
		MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
	}

	/* clean Up PHY and radio */
	wlc_phy_txcal_txgain_cleanup_acphy(pi, &orig_txgain[0]);
	wlc_phy_txcal_phy_cleanup_acphy(pi);

	if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi))
		wlc_phy_txcal_radio_cleanup_acphy_28nm(ti);
	else if (TINY_RADIO(pi))
		wlc_phy_txcal_radio_cleanup_acphy_tiny(pi);
	else
		wlc_phy_txcal_radio_cleanup_acphy(pi);

	WRITE_PHYREG(pi, ClassifierCtrl, ti->classifier_state);

	if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
		wlc_phy_btcx_override_disable(pi);
		if (!suspend) {
			wlapi_enable_mac(pi->sh->physhim);
		}
	}

	/* prevent crs trigger */
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);

	/* Restore PAPD state */
	FOREACH_CORE(pi, core) {
		WRITE_PHYREGCE(pi, PapdEnable, core, ti->papdState[core]);
	}

#ifdef ATE_BUILD
	printf("===> Finished Tx IQ Lo Cal.\n");
	coremask = stf_shdata->phyrxchain;
	printf("Biq2byp = %d\n", Biq2byp);
	FOREACH_ACTV_CORE(pi, coremask, core) {
		wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ, ab_int,
			TB_OFDM_COEFFS_AB, core);
		wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ, &d_reg,
			TB_OFDM_COEFFS_D, core);
	if (Biq2byp == 1) {
		ab_int[0] = (int16) accal->txiqcal_biq2byp_coeffs[2*core + 0];
		ab_int[1] = (int16) accal->txiqcal_biq2byp_coeffs[2*core + 1];
	}
	printf("   TX-IQ/LOFT CAL COEFFS: core-%d: a/b: (%4d,%4d), d: (%3d,%3d)\n",
		core, (int16) ab_int[0], (int16) ab_int[1],
		(int8)((d_reg & 0xFF00) >> 8), /* di */
		(int8)((d_reg & 0x00FF)));	/* dq */

#ifdef ATE_43684_LOG
/* This code is specific to 43684, it sotres the AuxTxCal coefficients.
 * It stores it into spare phy regs that are not used by 43684 chip
 */
		switch (core) {
			case 0:
				MOD_PHYREG(pi, spur_can_p0_s1_omega_high,
					spur_can_omega_high, (int16) ab_int[0]);
				MOD_PHYREG(pi, spur_can_p0_s1_omega_low,
					spur_can_omega_low, (int16) ab_int[1]);
				break;
			case 1:
				MOD_PHYREG(pi, spur_can_p1_s1_omega_high,
					spur_can_omega_high, (int16) ab_int[0]);
				MOD_PHYREG(pi, spur_can_p1_s1_omega_low,
					spur_can_omega_low, (int16) ab_int[1]);
				break;
			case 2:
				MOD_PHYREG(pi, spur_can_p2_s1_omega_high,
					spur_can_omega_high, (int16) ab_int[0]);
				MOD_PHYREG(pi, spur_can_p2_s1_omega_low,
					spur_can_omega_low, (int16) ab_int[1]);
				break;
			case 3:
				MOD_PHYREG(pi, spur_can_p3_s1_omega_high,
					spur_can_omega_high, (int16) ab_int[0]);
				MOD_PHYREG(pi, spur_can_p3_s1_omega_low,
					spur_can_omega_low, (int16) ab_int[1]);
				break;
		}
#endif /* ATE_43684_LOG */
	}
#endif /* ATE_BUILD */

	return BCME_OK;
}

void
wlc_phy_cal_txiqlo_coeffs_acphy(phy_info_t *pi, uint8 rd_wr, uint16 *coeff_vals,
                                uint8 select, uint8 core) {
	uint8 iqlocal_tbl_id = wlc_phy_get_tbl_id_iqlocal(pi, core);
	uint8 boffs_tmp = 0;

	/* handles IQLOCAL coefficients access (read/write from/to
	 * iqloCaltbl and pi State)
	 *
	 * not sure if reading/writing the pi state coeffs via this appraoch
	 * is a bit of an overkill
	 */

	/* {num of 16b words to r/w, start offset (ie address), core-to-core block offset} */
	acphy_coeff_access_t coeff_access_info[] = {
		{2, 64, 8},  /* TB_START_COEFFS_AB   */
		{1, 67, 8},  /* TB_START_COEFFS_D    */
		{1, 68, 8},  /* TB_START_COEFFS_E    */
		{1, 69, 8},  /* TB_START_COEFFS_F    */
		{2, 128, 7}, /*   TB_BEST_COEFFS_AB  */
		{1, 131, 7}, /*   TB_BEST_COEFFS_D   */
		{1, 132, 7}, /*   TB_BEST_COEFFS_E   */
		{1, 133, 7}, /*   TB_BEST_COEFFS_F   */
		{2, 96,  4}, /* TB_OFDM_COEFFS_AB    */
		{1, 98,  4}, /* TB_OFDM_COEFFS_D     */
		{2, 112, 4}, /* TB_BPHY_COEFFS_AB    */
		{1, 114, 4}, /* TB_BPHY_COEFFS_D     */
		{2, 0, 5},   /*   PI_INTER_COEFFS_AB */
		{1, 2, 5},   /*   PI_INTER_COEFFS_D  */
		{1, 3, 5},   /*   PI_INTER_COEFFS_E  */
		{1, 4, 5},   /*   PI_INTER_COEFFS_F  */
		{2, 0, 5},   /* PI_FINAL_COEFFS_AB   */
		{1, 2, 5},   /* PI_FINAL_COEFFS_D    */
		{1, 3, 5},   /* PI_FINAL_COEFFS_E    */
		{1, 4, 5}    /* PI_FINAL_COEFFS_F    */
	};
	acphy_cal_result_t *accal = &pi->cal_info->u.accal;

	uint8 nwords, offs, boffs, k;

	/* get access info for desired choice */
	nwords = coeff_access_info[select].nwords;
	offs   = coeff_access_info[select].offs;
	boffs  = coeff_access_info[select].boffs;

	boffs_tmp = boffs*core;
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		boffs_tmp = 0;
	}

	/* read or write given coeffs */
	if (select <= TB_BPHY_COEFFS_D) { /* START and BEST coeffs in Table */
		if (rd_wr == CAL_COEFF_READ) { /* READ */
			wlc_phy_table_read_acphy(pi, iqlocal_tbl_id, nwords,
				offs + boffs_tmp, 16, coeff_vals);
		} else { /* WRITE */
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, nwords,
				offs + boffs_tmp, 16, coeff_vals);
		}
	} else if (select <= PI_INTER_COEFFS_F) { /* PI state intermediate coeffs */
		for (k = 0; k < nwords; k++) {
			if (rd_wr == CAL_COEFF_READ) { /* READ */
				coeff_vals[k] = accal->txiqlocal_interm_coeffs[offs +
				                                               boffs*core + k];
			} else { /* WRITE */
				accal->txiqlocal_interm_coeffs[offs +
				                               boffs*core + k] = coeff_vals[k];
			}
		}
	} else { /* PI state final coeffs */
		for (k = 0; k < nwords; k++) { /* PI state final coeffs */
			if (rd_wr == CAL_COEFF_READ) { /* READ */
				coeff_vals[k] = accal->txiqlocal_coeffs[offs + boffs*core + k];
			} else { /* WRITE */
				accal->txiqlocal_coeffs[offs + boffs*core + k] = coeff_vals[k];
			}
		}
	}
}

void
wlc_phy_ipa_set_bbmult_acphy(phy_info_t *pi, uint16 *m0, uint16 *m1, uint16 *m2,
		uint16 *m3, uint8 coremask)
{
	/* TODO: 4360 */
	uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	uint8 iqlocal_tbl_id;
	ACPHY_DISABLE_STALL(pi);

	iqlocal_tbl_id = wlc_phy_get_tbl_id_iqlocal(pi, 0);

	if (PHYCOREMASK(coremask) == 1) {
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 99, 16, m0);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 115, 16, m0);
	} else if (PHYCOREMASK(coremask) == 3) {
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 99, 16, m0);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 115, 16, m0);

		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			iqlocal_tbl_id = wlc_phy_get_tbl_id_iqlocal(pi, 1);
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 99, 16, m1);
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 115, 16, m1);
		} else {
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 103, 16, m1);
			wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 119, 16, m1);
		}
	} else if (PHYCOREMASK(coremask) == 7) {
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 99, 16, m0);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 115, 16, m0);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 103, 16, m1);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 119, 16, m1);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 107, 16, m2);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 123, 16, m2);
	} else if (PHYCOREMASK(coremask) == 15) {
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 99, 16, m0);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 115, 16, m0);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 103, 16, m1);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 119, 16, m1);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 107, 16, m2);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 123, 16, m2);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 111, 16, m3);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, 127, 16, m3);
	}
	ACPHY_ENABLE_STALL(pi, stall_val);
}

void
wlc_acphy_get_tx_iqcc(phy_info_t *pi, uint16 *a, uint16 *b)
{
	uint16 iqcc[2];

	wlc_phy_table_read_acphy(pi, wlc_phy_get_tbl_id_iqlocal(pi, 0), 2,
		tbl_offset_ofdm_a[0], 16, &iqcc);

	*a = iqcc[0];
	*b = iqcc[1];
}

void
wlc_acphy_set_tx_iqcc(phy_info_t *pi, uint16 a, uint16 b)
{
	uint16 iqcc[2];
	uint8 core = 0;
	uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	uint8 iqlocal_tbl_id;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	iqcc[0] = a;
	iqcc[1] = b;

	ACPHY_DISABLE_STALL(pi);

	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		iqlocal_tbl_id = wlc_phy_get_tbl_id_iqlocal(pi, core);

		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 2, tbl_offset_ofdm_a[0], 16, iqcc);
		wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 2, tbl_offset_bphy_a[0], 16, iqcc);
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
}

uint16
wlc_acphy_get_tx_locc(phy_info_t *pi, uint8 core)
{
	uint16 didq;

	wlc_phy_table_read_acphy(pi, wlc_phy_get_tbl_id_iqlocal(pi, 0), 1,
		tbl_offset_ofdm_d[core], 16, &didq);
	return didq;
}

void
wlc_acphy_set_tx_locc(phy_info_t *pi, uint16 didq, uint8 core)
{
	uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	uint8 iqlocal_tbl_id;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ACPHY_DISABLE_STALL(pi);

	iqlocal_tbl_id = wlc_phy_get_tbl_id_iqlocal(pi, core);
	wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, tbl_offset_ofdm_d[core], 16, &didq);
	wlc_phy_table_write_acphy(pi, iqlocal_tbl_id, 1, tbl_offset_bphy_d[core], 16, &didq);

	ACPHY_ENABLE_STALL(pi, stall_val);
}

uint8
wlc_phy_get_tbl_id_iqlocal(phy_info_t *pi, uint16 core)
{
	uint8 tbl_id;
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		tbl_id = ACPHY_TBL_ID_IQLOCAL0;

		if ((phy_get_phymode(pi) != PHYMODE_RSDB) && (core == 1))  {
			tbl_id = ACPHY_TBL_ID_IQLOCAL1;
		}
	} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
		tbl_id = AC2PHY_TBL_ID_IQLOCAL;
	} else {
		tbl_id = ACPHY_TBL_ID_IQLOCAL;
	}
	return (tbl_id);
}

void
wlc_phy_poll_samps_WAR_acphy(phy_info_t *pi, int16 *samp, bool is_tssi,
                             bool for_idle, txgain_setting_t *target_gains,
                             bool for_iqcal, bool init_adc_inside, uint16 ADCcore, bool champ)
{
	uint8 core;
	uint16 save_afePuCtrl = 0, save_gpio = 0, save_gpioHiOutEn = 0;
	uint16 txgain1_save[PHY_CORE_MAX] = {0};
	uint16 txgain2_save[PHY_CORE_MAX] = {0};
	uint16 dacgain_save[PHY_CORE_MAX] = {0};
	uint16 bq2gain_save[PHY_CORE_MAX] = {0};
	uint16 overridegains_save[PHY_CORE_MAX] = {0};
	uint16 overridevlin_save[PHY_CORE_MAX] = {0};
	uint16 overridevlin2_save[PHY_CORE_MAX] = {0};
	uint16 orig_OVR10[PHY_CORE_MAX] = {0};
	uint16 orig_LPF_MAIN_CONTROLS[PHY_CORE_MAX] = {0};
	uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
	uint32 save_chipc = 0;
	uint8  stall_val = 0, log2_nsamps = 0;
	uint16 bbmult_save[PHY_CORE_MAX];
	uint16 bbmult, txgain1, txgain2, lpf_gain, dac_gain, vlin_val;
	uint16 bq1_gain_addr[3] = {0x17e, 0x18e, 0x19e}, bq1_gain;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	uint16 low_rate_adc_idle_tssi = 0;
	struct _orig_reg_vals {
		uint16 orig_afediv_cfg1_ovr;
		uint16 orig_afediv_reg1;
		uint16 orig_afediv_reg2;
		uint16 orig_afediv_reg3;
		} orig_reg_vals[PHY_CORE_MAX];

	BCM_REFERENCE(stf_shdata);

	if (init_adc_inside) {
		wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
		                      &save_chipc, &fval2g_orig, &fval5g_orig,
		                      &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);
	}

	if (is_tssi) {
		ACPHY_DISABLE_STALL(pi);
		/* Save gain for all Tx cores */
		/* Set TX gain to 0, so that LO leakage does not affect IDLE TSSI */
		FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &(bbmult_save[core]), core);
			dacgain_save[core] = READ_PHYREGCE(pi, Dac_gain, core);
			txgain1_save[core] = READ_PHYREGCE(pi, RfctrlCoreTXGAIN1, core);
			txgain2_save[core] = READ_PHYREGCE(pi, RfctrlCoreTXGAIN2, core);
			bq2gain_save[core] = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);
			overridegains_save[core] = READ_PHYREGCE(pi, RfctrlOverrideGains, core);
			overridevlin_save[core] = READ_PHYREGCE(pi, RfctrlOverrideAuxTssi, core);
			overridevlin2_save[core] = READ_PHYREGCE(pi, RfctrlCoreAuxTssi1, core);
		}
		if (for_idle) {
			/* This is to measure the idle tssi */
			bbmult   = 0;
			txgain1  = 0;
			txgain2  = 0;
			lpf_gain = 0;
			dac_gain = 0;

			if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID) &&
				ACMAJORREV_47(pi->pubpi->phy_rev) &&
				(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
				wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
				MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);

				FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
					orig_reg_vals[core].orig_afediv_cfg1_ovr =
						READ_RADIO_REG_20698(pi, AFEDIV_CFG1_OVR, core);
					orig_reg_vals[core].orig_afediv_reg1 =
						READ_RADIO_REG_20698(pi, AFEDIV_REG1, core);
					orig_reg_vals[core].orig_afediv_reg2 =
						READ_RADIO_REG_20698(pi, AFEDIV_REG2, core);
				}
				wlc_phy_radio20698_afe_div_ratio(pi, 1, 0, 0);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID) &&
				(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
				wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
				MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);

				FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
					orig_reg_vals[core].orig_afediv_cfg1_ovr =
						READ_RADIO_REG_20704(pi, AFEDIV_CFG1_OVR, core);
					orig_reg_vals[core].orig_afediv_reg1 =
						READ_RADIO_REG_20704(pi, AFEDIV_REG1, core);
					orig_reg_vals[core].orig_afediv_reg2 =
						READ_RADIO_REG_20704(pi, AFEDIV_REG2, core);
				}
				wlc_phy_radio20704_afe_div_ratio(pi, 1);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID) &&
				(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
				wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
				MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);

				FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
					orig_reg_vals[core].orig_afediv_cfg1_ovr =
						READ_RADIO_REG_20707(pi, AFEDIV_CFG1_OVR, core);
					orig_reg_vals[core].orig_afediv_reg1 =
						READ_RADIO_REG_20707(pi, AFEDIV_REG1, core);
					orig_reg_vals[core].orig_afediv_reg2 =
						READ_RADIO_REG_20707(pi, AFEDIV_REG2, core);
				}
				wlc_phy_radio20707_afe_div_ratio(pi, 1);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID) &&
				(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
				low_rate_adc_idle_tssi = (pi->sh->boardflags4 &
					BFL4_SROM18_LOW_RATE_ADC_IDLE_TSSI) > 0;
				MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);
				wlc_phy_low_rate_adc_enable_acphy(pi, low_rate_adc_idle_tssi);
				wlc_phy_radio20708_tx2cal_normal_adc_rate(pi, 1,
					low_rate_adc_idle_tssi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID) &&
				(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
				wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
				MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);

				FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
					orig_reg_vals[core].orig_afediv_cfg1_ovr =
						READ_RADIO_REG_20709(pi, AFEDIV_CFG1_OVR, core);
					orig_reg_vals[core].orig_afediv_reg1 =
						READ_RADIO_REG_20709(pi, AFEDIV_REG1, core);
					orig_reg_vals[core].orig_afediv_reg2 =
						READ_RADIO_REG_20709(pi, AFEDIV_REG2, core);
				}
				wlc_phy_radio20709_afe_div_ratio(pi, 1);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID) &&
				(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
				wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
				MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);

				FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
					orig_reg_vals[core].orig_afediv_cfg1_ovr =
						READ_RADIO_REG_20710(pi, AFEDIV_CFG1_OVR, core);
					orig_reg_vals[core].orig_afediv_reg1 =
						READ_RADIO_REG_20710(pi, AFEDIV_REG1, core);
					orig_reg_vals[core].orig_afediv_reg2 =
						READ_RADIO_REG_20710(pi, AFEDIV_REG2, core);
				}
				wlc_phy_radio20710_afe_div_ratio(pi, 1, 0, FALSE);
			}

			/* Make sure low-rate TSSI is efective */
			wlc_phy_resetcca_acphy(pi);
		} else {
			/* This is to measure the tone tssi */
			bbmult   = target_gains->bbmult;
			txgain1  = ((target_gains->rad_gain & 0xFF00) >> 8) |
				((target_gains->rad_gain_mi & 0x00FF) << 8);
			txgain2  = ((target_gains->rad_gain_mi & 0xFF00) >> 8) |
				((target_gains->rad_gain_hi & 0x00FF) << 8);
			lpf_gain = (target_gains->rad_gain & 0xF0) >> 4;
			dac_gain = (target_gains->rad_gain & 0x0F) >> 0;
			if ((!TINY_RADIO(pi)) && BF3_VLIN_EN_FROM_NVRAM(pi->u.pi_acphy)) {
				vlin_val =  (target_gains->rad_gain & 0x00F0) >> 7;
				MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi, core, tx_vlin_ovr, 1);
				MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1, core, tx_vlin, vlin_val);
			}
			if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID) &&
				(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
				wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
				MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);

				FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
					orig_reg_vals[core].orig_afediv_cfg1_ovr =
					    READ_RADIO_REG_20707(pi, AFEDIV_CFG1_OVR, core);
					orig_reg_vals[core].orig_afediv_reg1 =
					    READ_RADIO_REG_20707(pi, AFEDIV_REG1, core);
					orig_reg_vals[core].orig_afediv_reg2 =
					    READ_RADIO_REG_20707(pi, AFEDIV_REG2, core);
				}
				wlc_phy_radio20707_afe_div_ratio(pi, 1);
			}

		}

		if ((RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) &&
			!(RADIOMAJORREV(pi) == 3)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20710_ID))) {
			/* set same gain for all cores */
			FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
				WRITE_PHYREGCE(pi, RfctrlCoreTXGAIN1, core, txgain1);
				WRITE_PHYREGCE(pi, RfctrlCoreTXGAIN2, core, txgain2);
				WRITE_PHYREGCE(pi, Dac_gain, core, dac_gain);
				MOD_PHYREGCE(pi, RfctrlCoreLpfGain, core, lpf_bq2_gain, lpf_gain);
				MOD_PHYREGCE(pi, RfctrlOverrideGains, core, txgain, 1);
				MOD_PHYREGCE(pi, RfctrlOverrideGains, core, lpf_bq2_gain, 1);
				wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, core);

				if (!TINY_RADIO(pi) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20698_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20704_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20707_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20708_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20709_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
					/* Enforce lpf_bq1_gain */
					orig_LPF_MAIN_CONTROLS[core] =
						READ_RADIO_REGC(pi, RF, LPF_MAIN_CONTROLS, core);
					if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
						orig_OVR10[core] = READ_RADIO_REGC(pi, RF,
							GE16_OVR11, core);
					} else {
						orig_OVR10[core] = READ_RADIO_REGC(pi, RF,
							OVR10, core);
					}
					wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
						bq1_gain_addr[core], 16, &bq1_gain);
					bq1_gain = bq1_gain & 0x7; /* take the 3LSB */
					MOD_RADIO_REGC(pi, LPF_MAIN_CONTROLS, core,
						lpf_bq1_gain, bq1_gain);
					if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
						MOD_RADIO_REGC(pi, GE16_OVR11, core,
							ovr_lpf_bq1_gain, 1);
					} else {
						MOD_RADIO_REGC(pi, OVR10, core,
							ovr_lpf_bq1_gain, 1);
					}
				}
			}
		}

		ACPHY_ENABLE_STALL(pi, stall_val);

		/* Enable WLAN priority */
		wlc_phy_btcx_override_enable(pi);

		OSL_DELAY(100);
		if (for_idle) {
			wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_2MHz, 0,
				TX_TONE_IQCAL_MODE_OFF, FALSE);
		} else {
			wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_2MHz, (champ) ? 120 : 181,
				TX_TONE_IQCAL_MODE_OFF, FALSE);
		}
		OSL_DELAY(100);

		/* Taking a 256-samp average for 80mHz idle-tssi measuring.
		 * Note: ideally, we can apply the same averaging for 20/40mhz also,
		 *       but we don't want to change the existing 20/40mhz behavior to reduce risk.
		 */
		log2_nsamps = (for_iqcal || ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev) ||
			ACMAJORREV_GE47(pi->pubpi->phy_rev))? 3:
				(CHSPEC_IS80(pi->radio_chanspec) ? 8 : 0);

		if (champ)
			log2_nsamps = 4;
		wlc_phy_poll_samps_acphy(pi, samp, TRUE, log2_nsamps, init_adc_inside, ADCcore);
		wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);

		/* Disable WLAN priority */
		wlc_phy_btcx_override_disable(pi);

		if (for_idle && RADIOID_IS(pi->pubpi->radioid, BCM20698_ID) &&
			ACMAJORREV_47(pi->pubpi->phy_rev) &&
			(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
			wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
			//wlc_phy_radio20698_afe_div_haratio(pi, 0, 0, 0);
			FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
				WRITE_RADIO_REG_20698(pi, AFEDIV_CFG1_OVR, core,
				    orig_reg_vals[core].orig_afediv_cfg1_ovr);
				WRITE_RADIO_REG_20698(pi, AFEDIV_REG1, core,
				    orig_reg_vals[core].orig_afediv_reg1);
				WRITE_RADIO_REG_20698(pi, AFEDIV_REG2, core,
				    orig_reg_vals[core].orig_afediv_reg2);
			}
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
		} else if (for_idle && RADIOID_IS(pi->pubpi->radioid, BCM20704_ID) &&
			(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
			wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
			FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
				WRITE_RADIO_REG_20704(pi, AFEDIV_CFG1_OVR, core,
				    orig_reg_vals[core].orig_afediv_cfg1_ovr);
				WRITE_RADIO_REG_20704(pi, AFEDIV_REG1, core,
				    orig_reg_vals[core].orig_afediv_reg1);
				WRITE_RADIO_REG_20704(pi, AFEDIV_REG2, core,
				    orig_reg_vals[core].orig_afediv_reg2);
			}
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
		} else if (for_idle && RADIOID_IS(pi->pubpi->radioid, BCM20707_ID) &&
			(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
			wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
			FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
				WRITE_RADIO_REG_20707(pi, AFEDIV_CFG1_OVR, core,
				    orig_reg_vals[core].orig_afediv_cfg1_ovr);
				WRITE_RADIO_REG_20707(pi, AFEDIV_REG1, core,
				    orig_reg_vals[core].orig_afediv_reg1);
				WRITE_RADIO_REG_20707(pi, AFEDIV_REG2, core,
				    orig_reg_vals[core].orig_afediv_reg2);
			}
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
		} else if (for_idle && RADIOID_IS(pi->pubpi->radioid, BCM20708_ID) &&
			(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
			wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
			wlc_phy_radio20708_tx2cal_normal_adc_rate(pi, 0, 0);
		} else if (for_idle && RADIOID_IS(pi->pubpi->radioid, BCM20709_ID) &&
			(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
			wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
			FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
				WRITE_RADIO_REG_20709(pi, AFEDIV_CFG1_OVR, core,
				    orig_reg_vals[core].orig_afediv_cfg1_ovr);
				WRITE_RADIO_REG_20709(pi, AFEDIV_REG1, core,
				    orig_reg_vals[core].orig_afediv_reg1);
				WRITE_RADIO_REG_20709(pi, AFEDIV_REG2, core,
				    orig_reg_vals[core].orig_afediv_reg2);
			}
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
		} else if (for_idle && RADIOID_IS(pi->pubpi->radioid, BCM20710_ID) &&
			(pi->u.pi_acphy->sromi->srom_low_adc_rate_en)) {
			wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
			FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
				WRITE_RADIO_REG_20710(pi, AFEDIV_CFG1_OVR, core,
				    orig_reg_vals[core].orig_afediv_cfg1_ovr);
				WRITE_RADIO_REG_20710(pi, AFEDIV_REG1, core,
				    orig_reg_vals[core].orig_afediv_reg1);
				WRITE_RADIO_REG_20710(pi, AFEDIV_REG2, core,
				    orig_reg_vals[core].orig_afediv_reg2);
			}
			MOD_PHYREG(pi, TSSIMode, tssiADCSel, 0);
		}

		if (for_idle) wlc_phy_resetcca_acphy(pi);
	} else {
		wlc_phy_poll_samps_acphy(pi, samp, FALSE, 3, init_adc_inside, ADCcore);
	}

	if (is_tssi) {
		if ((RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) &&
			!(RADIOMAJORREV(pi) == 3)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) ||
			(RADIOID_IS(pi->pubpi->radioid, BCM20710_ID))) {
			FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
				/* Remove TX gain & lpf_bq1 gain override */
				WRITE_PHYREGCE(pi, RfctrlCoreTXGAIN1, core, txgain1_save[core]);
				WRITE_PHYREGCE(pi, RfctrlCoreTXGAIN2, core, txgain2_save[core]);
				WRITE_PHYREGCE(pi, Dac_gain, core, dacgain_save[core]);
				WRITE_PHYREGCE(pi, RfctrlOverrideGains, core,
					overridegains_save[core]);
				WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core, bq2gain_save[core]);
				WRITE_PHYREGCE(pi, RfctrlOverrideAuxTssi, core,
					overridevlin_save[core]);
				WRITE_PHYREGCE(pi, RfctrlCoreAuxTssi1, core,
					overridevlin2_save[core]);
				wlc_phy_set_tx_bbmult_acphy(pi, &(bbmult_save[core]), core);
				if (!TINY_RADIO(pi) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20698_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20704_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20707_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20708_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20709_ID) &&
				    !RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
					phy_utils_write_radioreg(pi,
					                         RF_2069_LPF_MAIN_CONTROLS(core),
					                         orig_LPF_MAIN_CONTROLS[core]);
					if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
						phy_utils_write_radioreg(pi,
						                         RF_2069_GE16_OVR11(core),
						                         orig_OVR10[core]);
					} else {
						phy_utils_write_radioreg(pi, RF_2069_OVR10(core),
							orig_OVR10[core]);
					}
				}
			}
		}
	}
	if (init_adc_inside) {
		wlc_phy_restore_after_adc_read(pi,  &save_afePuCtrl, &save_gpio,
		                               &save_chipc,  &fval2g_orig,  &fval5g_orig,
		                               &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);
	}
}

static void
wlc_txprecal4349_gain_control(phy_info_t *pi, txgain_setting_t *target_gains)
{
	uint16 classifier_state, target_tssi;
	int16 target_tssi_set[2][5] = {
		{175, 175, 90, 90, 90},
		{700, 680, 450, 330, 330},
	};
	uint8 band_idx, band_bw_idx, set_idx, core, stall_val;
	uint8 start_casc = 64;
	uint8 stop_casc = 1;
	uint8 step_casc = 32;
	uint8 start_idx = 0;
	uint8 stop_idx = 100;
	uint8 step_idx = 50;
	int16 idle_tssi[PHY_CORE_MAX], tssi[PHY_CORE_MAX];
	int16 tone_tssi, delta_tssi, delta_tssi_error = 10;
	bool done = FALSE, algo_conv = FALSE;
	txgain_setting_t orig_txgain[PHY_CORE_MAX];
	uint8 lower_lim = 0;
	uint8 initindex = 0;
	bool casc_gctrl;

	/* for save-restore of the changed regs */
	uint16 RxSdFeConfig1_orig, RxSdFeConfig6_orig, AfePuCtrl_orig;
	uint16 RfctrlOverrideAuxTssi_orig[PHY_CORE_MAX];
	acphy_precal_4349_radregs_t precal_radioreg_orig;
	uint16 *ptr_start = (uint16 *)&precal_radioreg_orig.auxpga_ovr1[0];

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	/* Save the registers here */
	RxSdFeConfig6_orig = READ_PHYREG(pi, RxSdFeConfig6);
	RxSdFeConfig1_orig = READ_PHYREG(pi, RxSdFeConfig1);
	AfePuCtrl_orig = READ_PHYREG(pi, AfePuCtrl);
	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, AUXPGA_OVR1, core),
			RADIO_REG_20693(pi, AUXPGA_CFG1, core),
			RADIO_REG_20693(pi, AUXPGA_VMID, core),
			RADIO_REG_20693(pi, TX_TOP_5G_OVR1, core),
			RADIO_REG_20693(pi, TX5G_MISC_CFG1, core),
			RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core),
			RADIO_REG_20693(pi, PA2G_CFG1, core),
			RADIO_REG_20693(pi, IQCAL_CFG1, core),
			RADIO_REG_20693(pi, TX2G_MISC_CFG1, core),
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &precal_radioreg_orig.tx2g_misc_cfg1[PHY_CORE_MAX-1]);
			*ptr = _READ_RADIO_REG(pi, porig_offsets[ct]);
		}

		RfctrlOverrideAuxTssi_orig[core] = READ_PHYREGCE(pi, RfctrlOverrideAuxTssi, core);
	}

	/* prevent crs trigger */
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	classifier_state = READ_PHYREG(pi, ClassifierCtrl);
	phy_rxgcrs_sel_classifier(pi, 4);

	/* Save the original Gain code */
	wlc_phy_txcal_txgain_setup_acphy(pi, target_gains, &orig_txgain[0]);

	/* turn off tssi sleep feature during cal */
	MOD_PHYREG(pi, AfePuCtrl, tssiSleepEn, 0);
	phy_ac_tssi_loopback_path_setup(pi, LOOPBACK_FOR_IQCAL);

	set_idx = PHY_IPA(pi);
	band_idx = CHSPEC_ISPHY5G6G(pi->radio_chanspec);

	if (CHSPEC_IS80(pi->radio_chanspec)) {
		band_bw_idx = 2;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		band_bw_idx = 1;
	} else {
		band_bw_idx = 0;
	}
	band_bw_idx += (band_idx * 2);
	target_tssi = target_tssi_set[set_idx][band_bw_idx];

	PHY_TRACE(("%s: set_idx: %d, band_idx: %d, band_bw_idx: %d, target_tssi: %d\n",
		__FUNCTION__, set_idx, band_idx, band_bw_idx, target_tssi));

	/* Enabling Cascode based gctrl for 2G */
	if (CHSPEC_IS2G(pi->radio_chanspec))
		casc_gctrl = TRUE;
	else
		casc_gctrl = FALSE;
	FOREACH_CORE(pi, core) {
		PHY_TRACE(("============== for Core %d ==============\n", core));
		/* Measure the Idle TSSI */
		wlc_phy_poll_samps_WAR_acphy(pi, &idle_tssi[0], TRUE, TRUE, NULL,
			TRUE, TRUE, core, 0);
		if (casc_gctrl) {
			start_casc = 64; stop_casc = 1;
		} else {
			start_idx = 0; stop_idx = 100;
		}
		if (casc_gctrl) {
			lower_lim = stop_casc;
		} else {
			lower_lim = start_idx;
		}
		done = FALSE;

		while (!done) {
			if (casc_gctrl) {
				step_casc = (start_casc + stop_casc) >> 1;
			} else {
				step_idx = (start_idx + stop_idx) >> 1;
			}

			wlc_phy_get_txgain_settings_by_index_acphy(
				pi, &target_gains[core], (casc_gctrl) ? initindex:step_idx);

			/* Max Out IPA gain as done during cal, for 2G */
			if ((CHSPEC_IS2G(pi->radio_chanspec) == 1)) {
				target_gains[core].rad_gain |= 0xff00;
				if (casc_gctrl) {
					target_gains[core].rad_gain_mi &= 0xff00;
					target_gains[core].rad_gain_mi |= step_casc;
				}
			}
			if ((CHSPEC_IS2G(pi->radio_chanspec) == 1)) {
				target_gains[core].bbmult = 20;
			} else {
				target_gains[core].bbmult = 25;
			}

			if (casc_gctrl) {
				wlc_phy_poll_samps_WAR_acphy(pi, &tssi[0], TRUE, FALSE,
					&target_gains[core], TRUE, TRUE, core, 0);
			} else {
				wlc_phy_poll_samps_WAR_acphy(pi, &tssi[0], TRUE, FALSE,
					&target_gains[core], TRUE, TRUE, core, 1);
			}
			tone_tssi = tssi[core] - idle_tssi[core];
			delta_tssi = target_tssi - tone_tssi;

			if (casc_gctrl) {
				PHY_TRACE(("channel: %x core: %1d casc_gcode : %2d idle_tssi: %4d "
					"target_tssi: %4d tone_tssi: %4d delta_tssi: %4d\n",
					pi->radio_chanspec, core, step_casc, idle_tssi[core],
					target_tssi, tone_tssi, delta_tssi));
			} else {
				PHY_TRACE(("channel: %x core: %1d idx : %2d idle_tssi: %4d "
					"target_tssi: %4d tone_tssi: %4d delta_tssi: %4d\n",
					pi->radio_chanspec, core, step_idx, idle_tssi[core],
					target_tssi, tone_tssi, delta_tssi));
			}

			if (delta_tssi > delta_tssi_error) {
				if (casc_gctrl) {
					stop_casc = step_casc;
					PHY_TRACE(("direction is increase power!\n"));
				} else {
					stop_idx = step_idx;
					PHY_TRACE(("direction is increase power!\n"));
				}
			} else {
				if (casc_gctrl) {
					start_casc = step_casc;
					PHY_TRACE(("direction is decrease power!\n"));
				} else {
					start_idx = step_idx;
					PHY_TRACE(("direction is decrease power!\n"));
				}
			}

			if (ABS(delta_tssi) <= delta_tssi_error) {
				done = TRUE;
				algo_conv = TRUE;
				break;
			}
			if (casc_gctrl) {
				if (step_casc == lower_lim) {
					done = TRUE;
					algo_conv = FALSE;
					break;
				}
				if (step_casc == (start_casc - 1)) {
					done = TRUE;
					algo_conv = FALSE;
					break;
				}
			} else {
				if (step_idx == lower_lim) {
					done = TRUE;
					algo_conv = FALSE;
					break;
				}
				if (step_idx == (stop_idx - 1)) {
					done = TRUE;
					algo_conv = FALSE;
					break;
				}
			}
		}

		if (algo_conv == FALSE) {
			PHY_TRACE(("ALGO FAILED TO CONVERGE FOR Core : %d\n", core));

			if (casc_gctrl) {
				if (step_casc == 1) {
					PHY_TRACE(("There seems to be gain deficit\n"));
					PHY_TRACE(("maximize the ipa gain code\n"));
					target_gains[core].rad_gain |= 0xff00;
				}
			} else {
				if (step_idx == 0) {
					PHY_TRACE(("There seems to be gain deficit\n"));
					PHY_TRACE(("maximize the ipa gain code\n"));
					target_gains[core].rad_gain |= 0xff00;
				}
			}
		} else {
			PHY_TRACE(("ALGO CONVERGED FOR Core : %d\n", core));
		}
		PHY_TRACE(("for core %d:\n", core));
		if (casc_gctrl) {
			WLC_PHY_PRECAL_TRACE_PERCORE_CASC(step_casc, target_gains, core);
		} else {
			WLC_PHY_PRECAL_TRACE_PERCORE(step_idx, target_gains, core);
		}
	}

	WRITE_PHYREG(pi, ClassifierCtrl, classifier_state);

	/* Restore the original Gain code */
	wlc_phy_txcal_txgain_cleanup_acphy(pi, &orig_txgain[0]);

	/* Restore the registers */
	WRITE_PHYREG(pi, RxSdFeConfig6, RxSdFeConfig6_orig);
	WRITE_PHYREG(pi, RxSdFeConfig1, RxSdFeConfig1_orig);
	WRITE_PHYREG(pi, AfePuCtrl, AfePuCtrl_orig);

	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, AUXPGA_OVR1, core),
			RADIO_REG_20693(pi, AUXPGA_CFG1, core),
			RADIO_REG_20693(pi, AUXPGA_VMID, core),
			RADIO_REG_20693(pi, TX_TOP_5G_OVR1, core),
			RADIO_REG_20693(pi, TX5G_MISC_CFG1, core),
			RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core),
			RADIO_REG_20693(pi, PA2G_CFG1, core),
			RADIO_REG_20693(pi, IQCAL_CFG1, core),
			RADIO_REG_20693(pi, TX2G_MISC_CFG1, core),
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &precal_radioreg_orig.tx2g_misc_cfg1[PHY_CORE_MAX-1]);
			phy_utils_write_radioreg(pi, porig_offsets[ct], *ptr);
		}

		WRITE_PHYREGCE(pi, RfctrlOverrideAuxTssi, core, RfctrlOverrideAuxTssi_orig[core]);
	}

	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	ACPHY_ENABLE_STALL(pi, stall_val);
	wlapi_enable_mac(pi->sh->physhim);

	PHY_TRACE(("======= IQLOCAL PreCalGainControl : END =======\n"));
	return;
}

#ifdef WLC_TXFDIQ
extern void
wlc_phy_tx_fdiqi_comp_acphy(phy_info_t *pi, bool enable, int fdiq_data_valid)
{
	uint8 core;
	int8 sign_slope;
	int8 idx;
	int8 slope;
#if defined(BCMDBG_RXCAL)
	int16 regval;
#endif /* BCMDBG_RXCAL */

	int16 filtercoeff[11][7] = {
		{	 0,    	0,	   0,  1023,   	0,     0,    0},
		{  -10,    16,   -31,  1023,   32,   -16,    10},
		{  -21,    31,   -62,  1023,   63,   -32,    21},
		{  -31,    46,   -92,  1023,   96,   -47,    31},
		{  -41,    62,  -121,  1022,  129,   -63,    42},
		{  -51,    77,  -150,  1022,  162,   -80,    53},
		{  -61,    91,  -179,  1020,  196,   -96,    63},
		{  -71,   106,  -207,  1019,  230,  -112,    74},
		{  -81,   121,  -234,  1018,  265,  -128,    85},
		{  -91,   135,  -261,  1016,  300,  -145,    95},
		{ -101,   149,  -278,  1014,  335,  -161,    106}
	};

	/* enable: 0 - disable FDIQI comp
	 *         1 - program FDIQI comp filter and enable
	 */

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

#ifdef WL_PROXDETECT
	if (phy_ac_tof_is_active(pi->u.pi_acphy->tofi)) {
		return;
	}
#endif

	/* write values */
	if (enable == FALSE) {
		MOD_PHYREG(pi, fdiqImbCompEnable, txfdiqImbCompEnable, 0);
#if defined(BCMDBG_RXCAL)
	/*	printf("   FDIQI Disabled\n"); */
#endif /* BCMDBG_RXCAL */
		return;
	} else {

#define ACPHY_TXFDIQCOMP_STR(pi, core, tap)	((ACPHY_txfdiqcomp_str0_c0(pi->pubpi->phy_rev) + \
	(0x200 * (core)) + (tap)))

		FOREACH_CORE(pi, core) {
		  if (((fdiq_data_valid & (1 << core)) >> core) == 1) {
		    slope = pi->u.pi_acphy->txiqlocali->txfdiqi->slope[core];
		    sign_slope = slope >= 0 ? 1 : -1;
		    slope *= sign_slope;
		    if (slope > 10) slope = 10;

			ACPHY_REG_LIST_START
			    MOD_PHYREG_ENTRY(pi, txfdiqImbN_offcenter_scale_str, N_offcenter_scale,
			    2)
			    MOD_PHYREG_ENTRY(pi, fdiqImbCompEnable, txfdiq_iorq, 0)
			    MOD_PHYREG_ENTRY(pi, fdiqi_tx_comp_Nshift_out, Nshift_out, 10)
			ACPHY_REG_LIST_EXECUTE(pi)

		    if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
		        CHSPEC_IS80(pi->radio_chanspec))  {
			    MOD_PHYREG(pi, fdiqImbCompEnable, txfdiq_iorq, 1);
		    }

			for (idx = 0; idx < 7; idx++) {
				if (sign_slope == -1) {
					phy_utils_write_phyreg(pi, ACPHY_TXFDIQCOMP_STR
					(pi, core, idx), filtercoeff[slope][6-idx]);
				} else {
					phy_utils_write_phyreg(pi, ACPHY_TXFDIQCOMP_STR
					(pi, core, idx), filtercoeff[slope][idx]);
				}
			}

#if defined(BCMDBG_RXCAL)
			if (!ACMAJORREV_32(pi->pubpi->phy_rev) &&
				!ACMAJORREV_33(pi->pubpi->phy_rev)) {
				printf("   Core=%d, Slope= %d :: ", core, sign_slope*slope);
				for (idx = 0; idx < 7; idx++) {
					regval = _PHY_REG_READ(pi,
						ACPHY_TXFDIQCOMP_STR(pi, core, idx));
					if (regval > 1024) regval -= 2048;
					printf(" %d", regval);
				}
				printf("\n");
			}
#endif /* BCMDBG_RXCAL */

			}
		}
		MOD_PHYREG(pi, fdiqImbCompEnable, txfdiqImbCompEnable, 1);
	}

}
#endif /* WLC_TXFDIQ */

#ifdef WLC_TXFDIQ
void
phy_ac_txiqlocal_fdiqi_lin_reg(phy_ac_txiqlocal_info_t *ti, acphy_fdiqi_t *freq_ang_mag,
                            uint16 num_data, int fdiq_data_valid)
{
	phy_info_t *pi = ti->pi;
	int32 Sf2 = 0;
	int32 Sfa[PHY_CORE_MAX], Sa[PHY_CORE_MAX], Sm[PHY_CORE_MAX];
	int32 intcp[PHY_CORE_MAX], mag[PHY_CORE_MAX];
	int32 refBW;
	int8 idx;
	uint8 core;
	int32 sin_angle, cos_angle;
	math_cint32 cordic_out;
	int32  a, b, sign_sa;
	uint16 coeffs[2];
	phy_ac_txiqlocal_info_t *ti = pi->u.pi_acphy->txiqlocali;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	/* initialize array for all cores to prevent compile warning (UNINIT) */
	FOREACH_CORE(pi, core) {
		Sfa[core] = 0; Sa[core] = 0; Sm[core] = 0;
	}

	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		Sf2 = 0;
		if (((fdiq_data_valid & (1 << core)) >> core) == 1) {
			for (idx = 0; idx < num_data; idx++) {
				Sf2 += freq_ang_mag[idx].freq * freq_ang_mag[idx].freq;
				Sfa[core] += freq_ang_mag[idx].freq * freq_ang_mag[idx].angle[core];
				Sa[core] += freq_ang_mag[idx].angle[core];
				Sm[core] += freq_ang_mag[idx].mag[core];
			}

			sign_sa = Sa[core] >= 0 ? 1 : -1;
			intcp[core] = (Sa[core] + sign_sa * (num_data >> 1)) / num_data;
			mag[core]   = (Sm[core] + (num_data >> 1)) / num_data;
			math_cmplx_cordic(intcp[core], &cordic_out);
			sin_angle = cordic_out.q;
			cos_angle = cordic_out.i;
			b = mag[core] * cos_angle;
			a = mag[core] * sin_angle;

			b = ((b >> 15) + 1) >> 1;
			b -= (1 << 10);  /* 10 bit */
			a = ((a >> 15) + 1) >> 1;

			a = (a < -512) ? -512 : ((a > 511) ? 511 : a);
			b = (b < -512) ? -512 : ((b > 511) ? 511 : b);

			coeffs[0] = (uint16)(a);
			coeffs[1] = (uint16)(b);
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
			                                coeffs, TB_OFDM_COEFFS_AB, core);
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
			                                coeffs, TB_BPHY_COEFFS_AB, core);
			refBW = 30;
			ti->txfdiqi->slope[core] =
			        (((-Sfa[core] * refBW / Sf2) >> 13) + 1 ) >> 1;
		}
	}
	wlc_phy_tx_fdiqi_comp_acphy(pi, TRUE, fdiq_data_valid);
}
#endif /* WLC_TXFDIQ */

#ifdef PHYCAL_CACHING
void
phy_ac_txiqlocal_save_cache(phy_ac_txiqlocal_info_t *txiqlocali, ch_calcache_t *ctx,
	uint8 multilo_cal_cnt)
{
	phy_info_t *pi = txiqlocali->pi;
	acphy_calcache_t *cache;
	uint8 core;
	uint16 ab_int[2];
	uint8 corenum = (uint8) PHYCORENUM(pi->pubpi->phy_corenum);
	cache = &ctx->u.acphy_cache;

	/* save the calibration to cache */
	FOREACH_CORE(pi, core) {
		/* Save OFDM Tx IQ Imb Coeffs A,B and Digital Loft Comp Coeffs */
		wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
		                                ab_int, TB_OFDM_COEFFS_AB, core);
		cache->ofdm_txa[core] = ab_int[0];
		cache->ofdm_txb[core] = ab_int[1];

		if (multilo_cal_cnt != 0) {
			if (ACMAJORREV_47_129_130(pi->pubpi->phy_rev)) {
				wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
					&cache->ofdm_txd[core+corenum*(multilo_cal_cnt-1)],
					TB_OFDM_COEFFS_D, core);
			}
		} else {
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
			                                &cache->ofdm_txd[core],
			                                TB_OFDM_COEFFS_D, core);
		}

		/* Save OFDM Tx IQ Imb Coeffs A,B and Digital Loft Comp Coeffs */
		wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
		                                ab_int, TB_BPHY_COEFFS_AB, core);
		cache->bphy_txa[core] = ab_int[0];
		cache->bphy_txb[core] = ab_int[1];
		wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
		                                &cache->bphy_txd[core], TB_BPHY_COEFFS_D, core);
#ifdef WLC_TXFDIQ
		cache->txs[core] = txiqlocali->txfdiqi->slope[core];
#endif
		if (!TINY_RADIO(pi)) {
			/* Save Analog Tx Loft Comp Coeffs */
			if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
				cache->txei[core] = 0;
				cache->txeq[core] = 0;
				cache->txfi[core] = 0;
				cache->txfq[core] = 0;
			} else {
				cache->txei[core] = (uint8) READ_RADIO_REGC(pi, RF,
				                                            TXGM_LOFT_FINE_I, core);
				cache->txeq[core] = (uint8) READ_RADIO_REGC(pi, RF,
				                                            TXGM_LOFT_FINE_Q, core);
				cache->txfi[core] = (uint8) READ_RADIO_REGC(pi, RF,
				                                            TXGM_LOFT_COARSE_I,
				                                            core);
				cache->txfq[core] = (uint8) READ_RADIO_REGC(pi, RF,
				                                            TXGM_LOFT_COARSE_Q,
				                                            core);
			}
		}
	}
}
#endif /* PHYCAL_CACHING */

void
wlc_phy_txcal_coeffs_upd(phy_info_t *pi, txcal_coeffs_t *txcal_cache)
{
	uint8 core;
	uint16 ab_int[2];

	uint8 phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;

	BCM_REFERENCE(phyrxchain);

	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		ab_int[0] = txcal_cache[core].txa;
		ab_int[1] = txcal_cache[core].txb;
		wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
			ab_int, TB_OFDM_COEFFS_AB, core);
		wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_WRITE,
			&txcal_cache[core].txd,	TB_OFDM_COEFFS_D, core);
		if (!TINY_RADIO(pi) && (!IS_28NM_RADIO(pi)) && (!IS_16NM_RADIO(pi))) {
			phy_utils_write_radioreg(pi, RF_2069_TXGM_LOFT_FINE_I(core),
			                         txcal_cache[core].txei);
			phy_utils_write_radioreg(pi, RF_2069_TXGM_LOFT_FINE_Q(core),
			                         txcal_cache[core].txeq);
			phy_utils_write_radioreg(pi, RF_2069_TXGM_LOFT_COARSE_I(core),
			                txcal_cache[core].txfi);
			phy_utils_write_radioreg(pi, RF_2069_TXGM_LOFT_COARSE_Q(core),
			                txcal_cache[core].txfq);
		}
	}
}

void
wlc_phy_txcal_coeffs_print(uint16 *coeffs, uint8 core)
{
	/* Print out Results */
	PHY_CAL(("\tcore-%d: a/b = (%4d,%4d), d = (%4d,%4d),"
		" e = (%4d,%4d), f = (%4d,%4d)\n", core,
		(int16)coeffs[core*5 + 0],  /* a */
		(int16)coeffs[core*5 + 1],  /* b */
		(int8)(coeffs[core*5 + 2] >> 8),      /* di */
		(int8)(coeffs[core*5 + 2] & 0x00FF),  /* dq */
		(int8)(coeffs[core*5 + 3] >> 8),      /* ei */
		(int8)(coeffs[core*5 + 3] & 0x00FF),  /* eq */
		(int8)(coeffs[core*5 + 4] >> 8),      /* fi */
		(int8)(coeffs[core*5 + 4] & 0x00FF))); /* fq */
}

void
wlc_phy_txcal_set_cal_params(phy_info_t *pi, uint8 searchmode, uint8 mphase, bool Biq2byp)
{
	uint8 num_cmds_total, num_cores = PHYCORENUM(pi->pubpi->phy_corenum);
	phy_ac_txiqlocal_info_t *ti = pi->u.pi_acphy->txiqlocali;
	txiqcal_params_t * paramsi = ti->paramsi;

	/* X52c should run cal only on the 2 active core
	 * Cal on 3rd core corrupts the LO/IQ ladder in IQLOCAL TBL
	 */
	if (IS_X52C_BOARDTYPE(pi)) num_cores = 2;

	ti->num_cores = num_cores;
	ti->prerxcal = ACPHY_TXCAL_PRERXCAL(pi) ? Biq2byp : 0;

	/* Choose Cal Commands for this Phase */
	if (searchmode == PHY_CAL_SEARCHMODE_RESTART) {
		if (ti->prerxcal) {
			ti->cmds = paramsi->cmds_RESTART_PRERX;
			ti->num_cmds_per_core = paramsi->num_cmds_restart_prerx;
		} else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			ti->cmds = paramsi->cmds_RESTART;
			/* numb of commands in tiny are 5 only */
			ti->num_cmds_per_core = 5;
			if (CHSPEC_IS80(pi->radio_chanspec))  {
				/* Restricting FDIQ cal to 80Mhz */
				ti->cmds = paramsi->cmds_RESTART_FDIQ;
				ti->num_cmds_per_core = paramsi->num_cmds_restart_fdiq;
			}
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			ti->cmds = paramsi->cmds_RESTART;
			/* numb of commands in tiny are 5 only */
			if ((CHSPEC_ISPHY5G6G(pi->radio_chanspec)) && !PHY_IPA(pi)) {
				ti->num_cmds_per_core = 4;
			} else {
				ti->num_cmds_per_core = 5;
			}
		} else {
			ti->cmds = paramsi->cmds_RESTART;
			ti->num_cmds_per_core = paramsi->num_cmds_restart;
		}
	} else if (searchmode == PHY_CAL_SEARCHMODE_MULTILO) {
		ti->cmds = paramsi->cmds_LOPWR;
		ti->num_cmds_per_core = paramsi->num_cmds_lopwr;
	} else {
		if (ti->prerxcal) {
			ti->cmds = paramsi->cmds_REFINE_PRERX;
			ti->num_cmds_per_core = paramsi->num_cmds_restart_prerx;
		} else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			ti->cmds = paramsi->cmds_REFINE;
			/* numb of commands in tiny are 5 only */
			ti->num_cmds_per_core = 5;
			if (CHSPEC_IS80(pi->radio_chanspec))  {
				/* Restricting FDIQ cal to 80Mhz */
				ti->cmds = paramsi->cmds_REFINE_FDIQ;
				ti->num_cmds_per_core = paramsi->num_cmds_refine_fdiq;
			}
		} else {
			ti->cmds = paramsi->cmds_REFINE;
			ti->num_cmds_per_core = paramsi->num_cmds_refine;
		}
	}

	num_cmds_total = num_cores * ti->num_cmds_per_core;

	if (mphase) {
		/* multi-phase: get next subset of commands (first & last index) */
		ti->cmd_idx = ti->phase_id * MPHASE_TXCAL_CMDS_PER_PHASE;
		ti->cmd_stop_idx = MIN(num_cmds_total,
		                       ti->cmd_idx + MPHASE_TXCAL_CMDS_PER_PHASE) - 1;
		ti->num_mphases = (num_cmds_total + MPHASE_TXCAL_CMDS_PER_PHASE - 1) /
		    MPHASE_TXCAL_CMDS_PER_PHASE;
	} else {
		/* single-phase: execute all commands for all cores */
		ti->cmd_idx = 0;
		ti->cmd_stop_idx = num_cmds_total - 1;
		ti->num_mphases = 0; ti->phase_id = 0;
	}
}

#if !defined(PHYCAL_CACHING)
void
wlc_phy_scanroam_cache_txcal_acphy(phy_type_txiqlocal_ctx_t *ctx, bool set)
{
	phy_ac_txiqlocal_info_t *info = (phy_ac_txiqlocal_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint16 ab_int[2];
	uint8 core;

	/* Prepare Mac and Phregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	PHY_TRACE(("wl%d: %s: in scan/roam set %d\n", pi->sh->unit, __FUNCTION__, set));

	if (set) {
		uint8 phyrxchain;

		BCM_REFERENCE(phyrxchain);

		PHY_CAL(("wl%d: %s: save the txcal for scan/roam\n",
			pi->sh->unit, __FUNCTION__));
		/* save the txcal to cache */
		phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
		FOREACH_ACTV_CORE(pi, phyrxchain, core) {
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
				ab_int, TB_OFDM_COEFFS_AB, core);
			info->txcal_cache[core].txa = ab_int[0];
			info->txcal_cache[core].txb = ab_int[1];
			wlc_phy_cal_txiqlo_coeffs_acphy(pi, CAL_COEFF_READ,
			&info->txcal_cache[core].txd,
				TB_OFDM_COEFFS_D, core);
			if (!TINY_RADIO(pi)) {
				info->txcal_cache[core].txei =
					(uint8)READ_RADIO_REGC(pi, RF, TXGM_LOFT_FINE_I, core);
				info->txcal_cache[core].txeq =
					(uint8)READ_RADIO_REGC(pi, RF, TXGM_LOFT_FINE_Q, core);
				info->txcal_cache[core].txfi =
					(uint8)READ_RADIO_REGC(pi, RF, TXGM_LOFT_COARSE_I, core);
				info->txcal_cache[core].txfq =
					(uint8)READ_RADIO_REGC(pi, RF, TXGM_LOFT_COARSE_Q, core);
			}
		}

		/* mark the cache as valid */
		info->txcal_cache_cookie = TXCAL_CACHE_VALID;
	} else {
		if (info->txcal_cache_cookie == TXCAL_CACHE_VALID) {
			PHY_CAL(("wl%d: %s: restore the txcal after scan/roam\n",
				pi->sh->unit, __FUNCTION__));
			/* restore the txcal from cache */
			wlc_phy_txcal_coeffs_upd(pi, info->txcal_cache);
		}
	}

	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);
}
#endif /* !defined(PHYCAL_CACHING) */
