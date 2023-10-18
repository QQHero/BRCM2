/*
 * ACPHY PAPD CAL module implementation
 *
 * Copyright 2021 Broadcom
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
 * $Id: phy_ac_papdcal.c 798973 2021-05-18 06:22:32Z $
 */
#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_cache.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <bcm_math.h>
#include <phy_btcx.h>
#include <phy_type_papdcal.h>
#include <phy_ac.h>
#include <phy_ac_papdcal.h>
#include <phy_ac_tpc.h>
#include <phy_ac_calmgr.h>
#include <phy_ac_dccal.h>
#include <phy_ac_info.h>
#include <phy_ac_misc.h>
#include <phy_ac_radio.h>
#include <phy_ac_tbl.h>
#include <wlc_phy_shim.h>
#include <wlc_phy_radio.h>
#include <wlc_phyreg_ac.h>
#include <phy_utils_var.h>
#include <phy_utils_reg.h>
#include <wlc_radioreg_20693.h>
#include <wlc_radioreg_20704.h>
#include <wlc_radioreg_20707.h>
#include <wlc_radioreg_20708.h>
#include <wlc_radioreg_20709.h>
#include <wlc_radioreg_20710.h>
#include <qmath.h>
#include "phy_ac_papdcal_data.h"
#include <d11.h>
#include <hndd11.h>
#include <phy_rstr.h>
#include <phy_stf.h>
#include <sbchipc.h>
uint16 papd_gainctrl_pga[PHY_CORE_MAX];
uint16 papd_gainctrl_calidx_2g[PHY_CORE_MAX];
uint16 papd_gainctrl_calidx_5g[PHY_CORE_MAX];
int16 papd_gainctrl_bbmult[PHY_CORE_MAX];
int16 papd_channel[PHY_CORE_MAX];
uint8 papd_tiagain_1st[PHY_CORE_MAX];
uint8 papd_tiagain[PHY_CORE_MAX];
uint8 papdcal_dumpinfo;
uint16 papd_prefilter_coeff_i[PHY_CORE_MAX][7], papd_prefilter_coeff_q[PHY_CORE_MAX][7];

/* MACROS */
#define WBPAPD_REFDB_BASE    6872
#define WBPAPD_REFDB_BASE_R131    6770
#define WBPAPD_REFDB_STEP    102

#define AUXGM_PATH    0
#define MAINGM_PATH    1

/* module PARAM structure */
typedef struct {
    uint16    buf_offset_2g;
    uint16    delayfilt_gamma_2g;
    uint16    cal_refdb_2g;
    uint16    epsilon_offset_2g;
    uint16    bbmult_2g[PHY_CORE_MAX];
    uint16    bbmult_2g_40M[PHY_CORE_MAX];
    uint8    rx_atten_2g;
    uint8    rx_atten_5g;
    uint8    tx_atten_2g;
    uint8    tx_atten_5g;
    uint8    papd_calidx_2g;
    uint8    papd_calidx_5g;
    uint8    papd_iter;
    uint8    papd_cal_settle;
    uint8    papd_cal_corr;
    uint8    papd_corr_shift;
    uint8    tia_mode_2g;
    uint8    tia_mode_5g;
    uint8    cal_dac_mode;
    uint8    cal_adc_mode;
    uint32    wbcal_lutstep_dB;
    uint8    wbcal_lut_len;
    uint32    wbcal_start;
    uint32    wbcal_end;
    uint32    wbcal_scale_start;
    uint32    wbcal_scale_end;
    uint8    wbcal_gfrac_bits;
    uint8    wbcal_dcc_en;
    uint8    wbcal_dly_filt_en;
    uint8    wbcal_dccorr_ovr_en;
    uint8    wbcal_dcoffset_real;
    uint8    wbcal_dcoffset_imag;
    uint8    wbcal_twotbl_en;
    uint8    wbcal_eps_fxdpt;
    uint16    wbcal_phyreg0_val;
    uint8    eps_stop_idx;
    uint16    wbcal_const_pow_scale;
    uint32    wbcal_macbuf_offset;
    uint32    wbcal_waveform_sz;
    uint16    bbmult_5g;
    uint16    bbmult_5g_40M;
    uint16    bbmult_5g_80M_160M;
    uint16    epsilon_offset_5g;
    uint16    cal_refdb_5g;
    uint16    buf_offset_5g;
    uint16    buf_offset_5g_160M;
    uint16    delayfilt_gamma_5g;
    uint8    papd_loopback_mode;
    uint8    papd_bypass_mode;
    uint8    wbcal_window_len_pow;
    uint8    wbcal_dc_accum_wait;
    uint8    dac_rate_mode;
    uint8    dac_rate_mode_80M_160M;
    bool    highres_en;
    int16    target_sum_RX;
    uint8    num_eps_tables;
    uint8    tx_idx_thresh_tbl[2];
    uint8    papd_calidx_tbl[2];
    uint8    tia_mode_tbl[2];
    uint16    cal_refdb_tbl[2];
    int8    eps_offset_lut[2];
} phy_ac_papdcal_params_t;

typedef struct _phy_ac_papdcal_radioregs {
    uint16 adc_cfg10[PHY_CORE_MAX];
    uint16 adc_ovr1[PHY_CORE_MAX];
} phy_ac_papdcal_radioregs_t;

typedef struct _phy_ac_papdcal_phyregs {
    uint16 RxSdFeConfig1;
    uint16 RxSdFeConfig6;
} phy_ac_papdcal_phyregs_t;

/* module private states */
struct phy_ac_papdcal_info {
    phy_info_t            *pi;
    phy_ac_info_t            *aci;
    phy_papdcal_info_t        *cmn_info;
    phy_ac_papdcal_params_t        *papd_params;
    phy_ac_papdcal_radioregs_t    *papd_radioregs;
    phy_ac_papdcal_phyregs_t    *papd_phyregs;
    int16    acphy_papd_epsilon_offset[PHY_CORE_MAX];
    uint16    papd_cal_time;
    uint8    papdmode;
    int8    pacalshift2g[3];
    int8    pacalshift5g[3];
    int8    pacalindex2g;
    int8    pacalindex5g[3];
    int8    pacalpwr2g;
    int8    pacalpwr5g[8];
    int8    pacalpwr5g40[8];
    int8    pacalpwr5g80[8];
    int8    parfps2g;
    int8    parfps5g;
    int16    papdbbmult2g;
    int16    papdbbmult5g;
    int32    papdbbmult_iovar;
    int32    end_epstblidx_iovar;
    uint8    papd_abort;            /* abort papd cal to limit max power */
    bool    fullcal;            /* to indicate full cal */
    int32    epstblsel_iovar;
    int32    extraepsoffset_iovar;
    int8    papdtiagain_iovar;
    int8    pacalmode;
    int8    pacalopt;
    int8    patoneidx2g;
    int8    patoneidx5g[4];
    int8    pacalshift5ga0[12];
    int8    pacalshift5ga1[12];
    uint8   papdpwrctrl;
    uint8   edpdcalset;
    uint8    acphy_papd_skip;        /* skip papd calibration for IPA case */
    uint8    acphy_papd_tx_gain_at_last_cal[PHY_CORE_MAX]; /* Tx gain index at last papd cal */
    uint8    srom_pagc2g;            /* iPa Pa gain override */
    uint8    srom_pagc2g_ovr;
    uint8    srom_pagc5g;
    uint8    srom_pagc5g_ovr;
    int8    papd_lut0_cal_idx;        /* PAPD index for lut0 */
    int8    papd_lut1_cal_idx;        /* PAPD index for lut1 */
    int32    pacalidx_iovar;            /* force papd cal index */
    int8    comp_disable_iovar;        /* force papd comp enable/disable */
    bool    _apapd;
    bool    perratedpd2g;            /* Per Rate DPD */
    bool    perratedpd5g;
    bool    papdcck_disable;        /* nvram control for cck papd */
    int32    wbpapd_gctrl_iovar;
    int32    wbpapd_multitbl_iovar;
/* add other variable size variables here at the end */
};

#ifdef WL_APAPD
    #define DO_PAPD_GAINCTRL 0
#else
    #define DO_PAPD_GAINCTRL 1
#endif /* WL_PAPD */

/* Analytic PAPD Support */
#ifndef DONGLEBUILD
    #ifndef WL_APAPD
        #define WL_APAPD
        #define APAPD_ENAB(papdcali)    (papdcali->_apapd)
    #endif /* WL_APAPD */
#else
    #ifdef WL_APAPD
        #define APAPD_ENAB(papdcali)    (papdcali->_apapd)
    #else
        #define APAPD_ENAB(papdcali)    (0)
    #endif /* WL_APAPD */
#endif /* !DONGLEBUILD */

static const char BCMATTACHDATA(rstr_pacalshift2g)[] = "pacalshift2g";
static const char BCMATTACHDATA(rstr_pacalshift5g)[] = "pacalshift5g";
static const char BCMATTACHDATA(rstr_pacalindex2g)[] = "pacalindex2g";
static const char BCMATTACHDATA(rstr_pacalindex5g)[] = "pacalindex5g";
static const char BCMATTACHDATA(rstr_pacalpwr2g)[] = "pacalpwr2g";
static const char BCMATTACHDATA(rstr_pacalpwr5g)[] = "pacalpwr5g";
static const char BCMATTACHDATA(rstr_pacalpwr5g40)[] = "pacalpwr5g40";
static const char BCMATTACHDATA(rstr_pacalpwr5g80)[] = "pacalpwr5g80";
static const char BCMATTACHDATA(rstr_parfps2g)[] = "parfps2g";
static const char BCMATTACHDATA(rstr_parfps5g)[] = "parfps5g";
static const char BCMATTACHDATA(rstr_papdbbmult2g)[] = "papdbbmult2g";
static const char BCMATTACHDATA(rstr_papdbbmult5g)[] = "papdbbmult5g";
static const char BCMATTACHDATA(rstr_pacalmode)[] = "pacalmode";
static const char BCMATTACHDATA(rstr_pacalopt)[] = "pacalopt";
static const char BCMATTACHDATA(rstr_patoneidx2g)[] = "patoneidx2g";
static const char BCMATTACHDATA(rstr_patoneidx5g)[] = "patoneidx5g";
static const char BCMATTACHDATA(rstr_pacalshift5ga0)[] = "pacalshift5ga0";
static const char BCMATTACHDATA(rstr_pacalshift5ga1)[] = "pacalshift5ga1";
static const char BCMATTACHDATA(rstr_papdpwrctrl)[] = "papdpwrctrl";
static const char BCMATTACHDATA(rstr_edpdcalset)[] = "edpdcalset";

/* papd params added for 43012 */
/* -- wb cal -- */
static const char BCMATTACHDATA(rstr_wb_tx_attn)[] = "wb_txattn";
static const char BCMATTACHDATA(rstr_wb_rx_attn)[] = "wb_rxattn";
static const char BCMATTACHDATA(rstr_wb_papd_calidx)[] = "wb_papdcalidx";
static const char BCMATTACHDATA(rstr_wb_eps_offset)[] = "wb_eps_offset";
static const char BCMATTACHDATA(rstr_wb_bbmult)[] = "wb_bbmult";
static const char BCMATTACHDATA(rstr_wb_calref_db)[] = "wb_calref_db";
static const char BCMATTACHDATA(rstr_wb_tia_gain_mode)[] = "wb_tia_gain_mode";
static const char BCMATTACHDATA(rstr_wb_txbuf_offset)[] = "wb_txbuf_offset";
static const char BCMATTACHDATA(rstr_wb_frac_del)[] = "wb_frac_del";
static const char BCMATTACHDATA(rstr_wb_g_frac_bits)[] = "wb_g_frac_bits";
static const char BCMATTACHDATA(rstr_wb_pacalshift2g)[] = "wb_pacalshift2g";
static const char BCMATTACHDATA(rstr_wb_pacalshift5g)[] = "wb_pacalshift5g";
/* -- nb cal -- */
static const char BCMATTACHDATA(rstr_nb_tx_attn)[] = "nb_txattn";
static const char BCMATTACHDATA(rstr_nb_rx_attn)[] = "nb_rxattn";
static const char BCMATTACHDATA(rstr_nb_papd_calidx)[] = "nb_papdcalidx";
static const char BCMATTACHDATA(rstr_nb_eps_offset)[] = "nb_eps_offset";
static const char BCMATTACHDATA(rstr_nb_bbmult)[] = "nb_bbmult";
static const char BCMATTACHDATA(rstr_nb_tia_gain_mode)[] = "nb_tia_gain_mode";
/* -- PAPD loopback config -- */
static const char BCMATTACHDATA(rstr_papd_loopback_mode)[] = "papd_loopback_mode";
static const char BCMATTACHDATA(rstr_papd_bypass_mode)[] = "papd_bypass_mode";

/* local functions */
#if defined(WLTEST) || defined(BCMDBG)
static void wlc_phy_epa_dpd_set_acphy(phy_type_papdcal_ctx_t *ctx, uint8 enab_epa_dpd,
    bool in_2g_band);
#endif /* defined(WLTEST) || defined(BCMDBG) */
static void phy_ac_papd_phy_setup(phy_info_t *pi, uint8 core);
static void phy_ac_papd_phy_setup_majorrev51_128_129_130_131(phy_info_t *pi);
static void phy_ac_papd_phy_core_setup_majorrev51_128_129_131(phy_info_t *pi, uint8 calcore);
static void phy_ac_papd_cal(phy_info_t *pi, uint16 num_iter, uint8 core, uint16 startindex,
                                   uint16 yrefindex, uint16 stopindex);
static void phy_ac_papd_phy_cleanup(phy_info_t *pi, uint8 core);
static void phy_ac_papd_phy_cleanup_majorrev51_128_129_130_131(phy_info_t *pi);
static void acphy_papd_cal_phyreg_sr(phy_info_t *pi, uint8 core, acphy_rxcal_phyregs_t *porig,
    bool sr);
static void phy_ac_papd_cal_set_tx_gain(phy_info_t *pi, uint8 core, uint16 *bbmult, uint8 *calmode);
static void phy_ac_papd_cal_mode0_1(phy_info_t *pi, acphy_papdCalParams_t *calParams,
    uint8 *calmode);
static int phy_ac_wbcal_run(phy_info_t *pi, uint8 core);
static int phy_ac_papd_mac_play(phy_info_t *pi, const uint32* buf, uint32 start_ptr, uint32 len,
    bool start, bool load);
static int phy_ac_papd_mac_load(phy_info_t *pi, const uint32* buf, uint32 len);
static void phy_ac_papd_cal_mode3(phy_info_t *pi, acphy_papdCalParams_t *calParams);
static void phy_ac_papd_cal_eps_calc_tiny(phy_info_t *pi, uint8 core, uint16 *bbmult);
void wlc_phy_papd_set_rfpwrlut(phy_info_t *pi);
static int phy_ac_populate_papd_params(phy_ac_papdcal_info_t *papd_info);

static void wlc_phy_txpwr_papd_cal_run_acphy(phy_info_t *pi, uint8 tx_pre_cal_pwr_ctrl_state);
static void wlc_phy_txpwr_papd_cal_run_tiny(phy_info_t *pi, uint8 tx_pre_cal_pwr_ctrl_state,
    int8 cal_core);
#ifdef WFD_PHY_LL
static void phy_ac_wd_wfd_ll(phy_type_papdcal_ctx_t *ctx);
static int phy_ac_papdcal_set_wfd_ll_enable(phy_type_papdcal_ctx_t *ctx, uint8 int_val);
static int phy_ac_papdcal_get_wfd_ll_enable(phy_type_papdcal_ctx_t *ctx, int32 *ret_int_ptr);
#endif /* WFD_PHY_LL */

#if (defined(WLTEST) || defined(WLPKTENG))
static bool wlc_phy_isperratedpden_acphy(phy_type_papdcal_ctx_t *ctx);
static void wlc_phy_perratedpdset_acphy(phy_type_papdcal_ctx_t *ctx, bool enable);
#endif
#if defined(WLTEST)
static int phy_ac_papdcal_get_lut_idx0(phy_type_papdcal_ctx_t *ctx, int32* idx);
static int phy_ac_papdcal_get_lut_idx1(phy_type_papdcal_ctx_t *ctx, int32* idx);
static int phy_ac_papdcal_get_idx(phy_type_papdcal_ctx_t *ctx, int32* idx);
static int phy_ac_papdcal_set_idx(phy_type_papdcal_ctx_t *ctx, int32 idx);
static int phy_ac_papdcal_get_bbmult(phy_type_papdcal_ctx_t *ctx, int32* bbmult);
static int phy_ac_papdcal_set_bbmult(phy_type_papdcal_ctx_t *ctx, int32 bbmult);
static int phy_ac_papdcal_get_extraepsoffset(phy_type_papdcal_ctx_t *ctx, int32* extraepsoffset);
static int phy_ac_papdcal_set_extraepsoffset(phy_type_papdcal_ctx_t *ctx, int32 extraepsoffset);
static int phy_ac_papdcal_get_tiagain(phy_type_papdcal_ctx_t *ctx, int32* tiagain);
static int phy_ac_papdcal_set_tiagain(phy_type_papdcal_ctx_t *ctx, int8 tiagain);
static int phy_ac_papdcal_get_comp_disable(phy_type_papdcal_ctx_t *ctx, int32* comp_disable);
static int phy_ac_papdcal_set_comp_disable(phy_type_papdcal_ctx_t *ctx, int8 comp_disable);
static int phy_ac_papdcal_get_end_epstblidx(phy_type_papdcal_ctx_t *ctx, int32* end_epstblidx);
static int phy_ac_papdcal_set_end_epstblidx(phy_type_papdcal_ctx_t *ctx, int32 end_epstblidx);
static int phy_ac_papdcal_get_epstblsel(phy_type_papdcal_ctx_t *ctx, int32* epstblsel);
static int phy_ac_papdcal_set_epstblsel(phy_type_papdcal_ctx_t *ctx, int32 epstblsel);
static int phy_ac_papdcal_get_dump(phy_type_papdcal_ctx_t *ctx, int32* papdcal_dump);
static int phy_ac_papdcal_set_dump(phy_type_papdcal_ctx_t *ctx, int8 papdcal_dump);
static int phy_ac_papdcal_get_abort(phy_type_papdcal_ctx_t *ctx, int32* papd_abort);
static int phy_ac_papdcal_set_abort(phy_type_papdcal_ctx_t *ctx, int32 papd_abort);
static int phy_ac_papdcal_get_wbpapd_gctrl(phy_type_papdcal_ctx_t *ctx, int32* wbpapd_gctrl);
static int phy_ac_papdcal_set_wbpapd_gctrl(phy_type_papdcal_ctx_t *ctx, int32 wbpapd_gctrl);
static int phy_ac_papdcal_get_wbpapd_multitbl(phy_type_papdcal_ctx_t *ctx, int32* wbpapd_multitbl);
static int phy_ac_papdcal_set_wbpapd_multitbl(phy_type_papdcal_ctx_t *ctx, int32 wbpapd_multitbl);
#endif
#if defined(WLTEST) || defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
#ifndef ATE_BUILD
static int phy_ac_papdcal_set_skip(phy_type_papdcal_ctx_t *ctx, uint8 skip);
static int phy_ac_papdcal_get_skip(phy_type_papdcal_ctx_t *ctx, int32 *skip);
#endif /* !ATE_BUILD */
#endif

static void phy_ac_papdcal_nvram_attach_old(phy_ac_papdcal_info_t *ac_info);
static uint16 phy_ac_papd_find_maxamam_endfill_epstbl(phy_info_t *pi, uint8 core,
        bool only_report_maxamam_idx);

static void
BCMATTACHFN(phy_ac_papdcal_nvram_attach)(phy_info_t *pi, phy_ac_papdcal_info_t *ac_info)
{
    uint8 i;

    BCM_REFERENCE(rstr_nb_tx_attn);
    BCM_REFERENCE(rstr_nb_rx_attn);
    BCM_REFERENCE(rstr_nb_papd_calidx);
    BCM_REFERENCE(rstr_nb_eps_offset);
    BCM_REFERENCE(rstr_nb_bbmult);
    BCM_REFERENCE(rstr_nb_tia_gain_mode);

    for (i = 0; i < 3; i++) {
        if (ACMAJORREV_131(pi->pubpi->phy_rev) && WBPAPD_ENAB(pi)) {
            ac_info->pacalshift2g[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                (pi, rstr_wb_pacalshift2g, i, 0));
        } else {
            ac_info->pacalshift2g[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                (pi, rstr_pacalshift2g, i, 0));
        }
    }

    if (PHY_BAND5G_ENAB(pi)) {
        for (i = 0; i < 3; i++) {
            if (ACMAJORREV_131(pi->pubpi->phy_rev) && WBPAPD_ENAB(pi)) {
                ac_info->pacalshift5g[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                    (pi, rstr_wb_pacalshift5g, i, 0));
            } else {
                ac_info->pacalshift5g[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                    (pi, rstr_pacalshift5g, i, 0));

            }
            ac_info->pacalindex5g[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                    (pi, rstr_pacalindex5g, i, -1));
        }

        /* For 4350/6878 */
        for (i = 0; i < 12; i++) {
            ac_info->pacalshift5ga0[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                (pi, rstr_pacalshift5ga0, i, 0));
            ac_info->pacalshift5ga1[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                (pi, rstr_pacalshift5ga1, i, 0));
        }
    }

    ac_info->pacalindex2g = (int8) (PHY_GETINTVAR_DEFAULT(pi, rstr_pacalindex2g, -1));
    ac_info->pacalpwr2g = (int8) (PHY_GETINTVAR_DEFAULT(pi, rstr_pacalpwr2g, -99));
    ac_info->papdpwrctrl = (uint8) (PHY_GETINTVAR_DEFAULT(pi, rstr_papdpwrctrl, 0));
    ac_info->edpdcalset = (uint8) (PHY_GETINTVAR_DEFAULT(pi, rstr_edpdcalset, 0));
    if (ACMAJORREV_2(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
        /* For 4350, pacalpwr5g = lo, mi, hi, x1, lo, mi, hi, x1 */
        /*                       |    core 0     |     core 1    | */
        for (i = 0; i < 8; i++) {
            ac_info->pacalpwr5g[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                (pi, rstr_pacalpwr5g, i, -99));
            ac_info->pacalpwr5g40[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                (pi, rstr_pacalpwr5g40, i, -99));
            ac_info->pacalpwr5g80[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                (pi, rstr_pacalpwr5g80, i, -99));
        }
    } else {
        if (PHY_BAND5G_ENAB(pi)) {
            /* For 4345 and others */
            for (i = 0; i < 4; i++) {
                ac_info->pacalpwr5g[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                    (pi, rstr_pacalpwr5g, i, -99));
            }
        }
    }

    ac_info->parfps2g = (int8) (PHY_GETINTVAR_DEFAULT(pi, rstr_parfps2g, -1));
    ac_info->papdbbmult2g = (int16) (PHY_GETINTVAR_DEFAULT(pi, rstr_papdbbmult2g, -1));
    ac_info->pacalmode = (int8) (PHY_GETINTVAR_DEFAULT(pi, rstr_pacalmode, -1));
    ac_info->pacalopt = (int8) (PHY_GETINTVAR_DEFAULT(pi, rstr_pacalopt, -1));
    ac_info->patoneidx2g = (int8) (PHY_GETINTVAR_DEFAULT(pi, rstr_patoneidx2g, -1));

    if (PHY_BAND5G_ENAB(pi)) {
        ac_info->parfps5g = (int8) (PHY_GETINTVAR_DEFAULT(pi, rstr_parfps5g, -1));
        ac_info->papdbbmult5g = (int16) (PHY_GETINTVAR_DEFAULT(pi, rstr_papdbbmult5g, -1));

        for (i = 0; i < 4; i++) {
            ac_info->patoneidx5g[i] = (int8) (PHY_GETINTVAR_ARRAY_DEFAULT
                (pi, rstr_patoneidx5g, i, -1));
        }
    }
    ac_info->papdcck_disable = (bool)(pi->sh->boardflags4 & BFL4_SROM18_PAPDCCK_DISABLE);
}

/* register phy type specfic implementation */
phy_ac_papdcal_info_t *
BCMATTACHFN(phy_ac_papdcal_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
    phy_papdcal_info_t *cmn_info)
{
    phy_ac_papdcal_info_t *ac_info;
    phy_type_papdcal_fns_t fns;

    PHY_CAL(("%s\n", __FUNCTION__));

    /* allocate all storage together */
    if ((ac_info = phy_malloc(pi, sizeof(phy_ac_papdcal_info_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((ac_info->papd_radioregs =
            phy_malloc(pi, sizeof(phy_ac_papdcal_radioregs_t))) == NULL) {
        PHY_ERROR(("%s: ac_txcal_radioregs_orig malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((ac_info->papd_phyregs =
            phy_malloc(pi, sizeof(phy_ac_papdcal_phyregs_t))) == NULL) {
        PHY_ERROR(("%s: ac_txcal_phyregs_orig malloc failed\n", __FUNCTION__));
        goto fail;
    }

    /* Initialize params */
    ac_info->pi = pi;
    ac_info->aci = aci;
    ac_info->cmn_info = cmn_info;

    phy_ac_papdcal_nvram_attach(pi, ac_info);

    /* register PHY type specific implementation */
    bzero(&fns, sizeof(fns));
#if defined(WLTEST) || defined(BCMDBG)
    fns.epa_dpd_set = wlc_phy_epa_dpd_set_acphy;
#endif /* defined(WLTEST) || defined(BCMDBG) */
    fns.ctx = ac_info;
#if (defined(WLTEST) || defined(WLPKTENG))
    fns.isperratedpden = wlc_phy_isperratedpden_acphy;
    fns.perratedpdset = wlc_phy_perratedpdset_acphy;
#endif
#if defined(WLTEST)
    fns.get_idx0 = phy_ac_papdcal_get_lut_idx0;
    fns.get_idx1 = phy_ac_papdcal_get_lut_idx1;
    fns.get_idx = phy_ac_papdcal_get_idx;
    fns.set_idx = phy_ac_papdcal_set_idx;
    fns.get_bbmult = phy_ac_papdcal_get_bbmult;
    fns.set_bbmult = phy_ac_papdcal_set_bbmult;
    fns.get_extraepsoffset = phy_ac_papdcal_get_extraepsoffset;
    fns.set_extraepsoffset = phy_ac_papdcal_set_extraepsoffset;
    fns.get_tiagain = phy_ac_papdcal_get_tiagain;
    fns.set_tiagain = phy_ac_papdcal_set_tiagain;
    fns.get_comp_disable = phy_ac_papdcal_get_comp_disable;
    fns.set_comp_disable = phy_ac_papdcal_set_comp_disable;
    fns.get_end_epstblidx = phy_ac_papdcal_get_end_epstblidx;
    fns.set_end_epstblidx = phy_ac_papdcal_set_end_epstblidx;
    fns.get_epstblsel = phy_ac_papdcal_get_epstblsel;
    fns.set_epstblsel = phy_ac_papdcal_set_epstblsel;
    fns.get_dump = phy_ac_papdcal_get_dump;
    fns.set_dump = phy_ac_papdcal_set_dump;
    fns.get_abort = phy_ac_papdcal_get_abort;
    fns.set_abort = phy_ac_papdcal_set_abort;
    fns.get_wbpapd_gctrl = phy_ac_papdcal_get_wbpapd_gctrl;
    fns.set_wbpapd_gctrl = phy_ac_papdcal_set_wbpapd_gctrl;
    fns.get_wbpapd_multitbl = phy_ac_papdcal_get_wbpapd_multitbl;
    fns.set_wbpapd_multitbl = phy_ac_papdcal_set_wbpapd_multitbl;
#endif
#if defined(WLTEST) || defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
#ifndef ATE_BUILD
    fns.set_skip = phy_ac_papdcal_set_skip;
    fns.get_skip = phy_ac_papdcal_get_skip;
#endif /* !ATE_BUILD */
#endif
#if defined(WFD_PHY_LL)
    fns.wd_wfd_ll = phy_ac_wd_wfd_ll;
    fns.set_wfd_ll_enable = phy_ac_papdcal_set_wfd_ll_enable;
    fns.get_wfd_ll_enable = phy_ac_papdcal_get_wfd_ll_enable;
#endif /* WFD_PHY_LL */

    /* Populate the PAPD Params */
    if (phy_ac_populate_papd_params(ac_info) != BCME_OK) {
        goto fail;
    }
    /* setup srom cfg */
    phy_ac_papdcal_nvram_attach_old(ac_info);

    if (phy_papdcal_register_impl(cmn_info, &fns) != BCME_OK) {
        PHY_ERROR(("%s: phy_papdcal_register_impl failed\n", __FUNCTION__));
        goto fail;
    }

#ifdef WL_APAPD
    if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
        ac_info->_apapd = TRUE;
    } else {
        ac_info->_apapd = FALSE;
    }
#else
    ac_info->_apapd = FALSE;
#endif /* WL_PAPD */
    return ac_info;

    /* error handling */
fail:
    phy_ac_papdcal_unregister_impl(ac_info);
    return NULL;
}

void
BCMATTACHFN(phy_ac_papdcal_unregister_impl)(phy_ac_papdcal_info_t *ac_info)
{
    phy_papdcal_info_t *cmn_info;
    phy_info_t *pi;

    if (ac_info == NULL) {
        return;
    }

    cmn_info = ac_info->cmn_info;
    pi = ac_info->pi;

    PHY_CAL(("%s\n", __FUNCTION__));

    /* unregister from common */
    phy_papdcal_unregister_impl(cmn_info);

    if (ac_info->papd_params) {
        /* Free PAPD params */
        phy_mfree(pi, ac_info->papd_params, sizeof(phy_ac_papdcal_params_t));
    }
    if (ac_info->papd_radioregs != NULL) {
        phy_mfree(pi, ac_info->papd_radioregs, sizeof(phy_ac_papdcal_radioregs_t));
    }
    if (ac_info->papd_phyregs != NULL) {
        phy_mfree(pi, ac_info->papd_phyregs, sizeof(phy_ac_papdcal_phyregs_t));
    }
    phy_mfree(pi, ac_info, sizeof(phy_ac_papdcal_info_t));
}

#ifdef WFD_PHY_LL
static void
phy_ac_wd_wfd_ll(phy_type_papdcal_ctx_t *ctx)
{
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)ctx;
    phy_papdcal_data_t *data = papdcali->cmn_info->data;

    /* Be sure there is no cal in progress to enable/disable optimization */
    if (!PHY_PERICAL_MPHASE_PENDING(papdcali->pi)) {
        if (data->wfd_ll_enable != data->wfd_ll_enable_pending) {
            data->wfd_ll_enable = data->wfd_ll_enable_pending;
            if (!data->wfd_ll_enable) {
                /* Force a watchdog CAL when disabling WFD optimization
                 * As PADP CAL has not been executed since a long time
                 * a PADP CAL is executed at the next watchdog timeout
                 */
                papdcali->pi->cal_info->last_cal_time = 0;
            }
        }
    }
}
#endif /* WFD_PHY_LL */

static int
phy_ac_populate_papd_params(phy_ac_papdcal_info_t *papd_info)
{
    phy_ac_papdcal_params_t *papd_params;
    phy_info_t *pi;
    uint8 core;
    uint8 pdgain2g;

    pi = papd_info->pi;

    if ((papd_params = phy_malloc(pi, sizeof(phy_ac_papdcal_params_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
        goto fail;
    }

    // Assign default values for legacy chips.
    papd_params->highres_en = 0;
    papd_params->dac_rate_mode = 1;
    papd_params->dac_rate_mode_80M_160M = 1;
    papd_params->cal_refdb_2g = 20;
    papd_params->cal_refdb_5g = 20;
    papd_params->num_eps_tables = 1;

    if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
        papd_params->rx_atten_2g = 0x3;
        papd_params->rx_atten_5g = 0x3;
        papd_params->tx_atten_2g = 0x3;
        papd_params->tx_atten_5g = 0x2;
    } else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
        papd_params->rx_atten_2g = 0x3;
        papd_params->tx_atten_2g = 0x3;
        papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_papd_calidx, PHY_EPAPD(pi) ? 48: 26)) & PAPD_CAL_IDX_2G;
        papd_params->papd_iter = 64;
        papd_params->tia_mode_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_tia_gain_mode, 0x2)) & TIA_GAIN_MODE_2G;
        papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;

        papd_params->papd_loopback_mode = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_papd_loopback_mode, MAINGM_PATH));
        papd_params->papd_bypass_mode = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_papd_bypass_mode, 1));

        if (WBPAPD_ENAB(pi)) {
            papd_params->tia_mode_2g = 0x6;
            papd_params->cal_adc_mode = 2; /* 180 MHz mode */
            papd_params->wbcal_lutstep_dB = 10240;
            papd_params->wbcal_lut_len = 64;
            papd_params->wbcal_start = 512*4;
            papd_params->wbcal_end   = 143384;
            papd_params->wbcal_scale_start = 600;
            papd_params->wbcal_scale_end   = 1624;
            papd_params->wbcal_dcc_en      = 1;
            papd_params->wbcal_dly_filt_en = 1;
            papd_params->wbcal_dccorr_ovr_en = 0;
            papd_params->wbcal_dcoffset_real = 0;
            papd_params->wbcal_dcoffset_imag = 0;
            papd_params->wbcal_twotbl_en = 0;
            papd_params->wbcal_phyreg0_val = 0xA924;
            papd_params->eps_stop_idx = 38;
            papd_params->wbcal_const_pow_scale = 710/2;
            papd_params->wbcal_macbuf_offset = 65536;
            papd_params->wbcal_waveform_sz = 4000;
            papd_params->buf_offset_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_txbuf_offset, 33)) & PAPD_BUF_OFFSET_2G;
            papd_params->delayfilt_gamma_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_frac_del, 26)) & PAPD_FRACDELAY_OFFSET_2G;
            /* By default frac bits is 11 */
            papd_params->wbcal_gfrac_bits = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_g_frac_bits, 0xBB));
            papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;
        }
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        papd_params->rx_atten_2g = 0x3;
        papd_params->tx_atten_2g = 0x2;
        papd_params->rx_atten_5g = 0x3;
        papd_params->tx_atten_5g = 0x1;
        if (PHY_EPAPD(pi)) {
            papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_papd_calidx, 48)) & PAPD_CAL_IDX_2G;
            papd_params->papd_calidx_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_papd_calidx, 48 << PAPD_CAL_IDX_5G_SHIFT)) &
                PAPD_CAL_IDX_5G) >> PAPD_CAL_IDX_5G_SHIFT;
        } else {
            papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_papd_calidx, 25)) & PAPD_CAL_IDX_2G;
            papd_params->papd_calidx_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_papd_calidx, 18 << PAPD_CAL_IDX_5G_SHIFT)) &
                PAPD_CAL_IDX_5G) >> PAPD_CAL_IDX_5G_SHIFT;
        }
        FOREACH_CORE(pi, core) {
            papd_gainctrl_calidx_2g[core] = papd_params->papd_calidx_2g;
            papd_gainctrl_calidx_5g[core] = papd_params->papd_calidx_5g;
        }
        papd_params->papd_iter = 32;
        papd_params->tia_mode_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_tia_gain_mode, 3)) & TIA_GAIN_MODE_2G;
        papd_params->tia_mode_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_tia_gain_mode, 3 << TIA_GAIN_MODE_5G_SHIFT)) &
                TIA_GAIN_MODE_5G) >> TIA_GAIN_MODE_5G_SHIFT;
        papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;
        papd_params->epsilon_offset_5g = (((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_eps_offset, 0)) & PAPD_EPS_OFFSET_5G) >>
                PAPD_EPS_OFFSET_5G_SHIFT;
        if (WBPAPD_ENAB(pi)) {
            papd_params->tia_mode_2g = 0x6;
            papd_params->cal_adc_mode = 2; /* 180 MHz mode */
            papd_params->wbcal_lutstep_dB = 10240;
            papd_params->wbcal_lut_len = 64;
            papd_params->wbcal_start = 512*4;
            papd_params->wbcal_end   = 143384;
            papd_params->wbcal_scale_start = 600;
            papd_params->wbcal_scale_end   = 1624;
            papd_params->wbcal_dcc_en      = 1;
            papd_params->wbcal_dly_filt_en = 1;
            papd_params->wbcal_dccorr_ovr_en = 0;
            papd_params->wbcal_dcoffset_real = 0;
            papd_params->wbcal_dcoffset_imag = 0;
            papd_params->wbcal_twotbl_en = 0;
            papd_params->wbcal_phyreg0_val = 0xA924;
            papd_params->eps_stop_idx = 38;
            papd_params->wbcal_const_pow_scale = 710/2;
            papd_params->wbcal_macbuf_offset = 65536;
            papd_params->wbcal_waveform_sz = 4000;
            papd_params->buf_offset_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_txbuf_offset, 33)) & PAPD_BUF_OFFSET_2G;
            papd_params->delayfilt_gamma_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_frac_del, 26)) & PAPD_FRACDELAY_OFFSET_2G;
            /* By default frac bits is 11 */
            papd_params->wbcal_gfrac_bits = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_g_frac_bits, 0xBB));
            papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;
        }
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        papd_params->rx_atten_2g = 0x3;
        papd_params->tx_atten_2g = 0x3;
        papd_params->rx_atten_5g = 0x3;
        papd_params->tx_atten_5g = 0x3;
        papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_papd_calidx, 30)) & PAPD_CAL_IDX_2G;
        papd_params->papd_calidx_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
            rstr_nb_papd_calidx, 30 << PAPD_CAL_IDX_5G_SHIFT)) &
            PAPD_CAL_IDX_5G) >> PAPD_CAL_IDX_5G_SHIFT;
        papd_params->papd_iter = 128;
        papd_params->tia_mode_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_tia_gain_mode, 2)) & TIA_GAIN_MODE_2G;
        papd_params->tia_mode_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_tia_gain_mode, 6 << TIA_GAIN_MODE_5G_SHIFT)) &
                TIA_GAIN_MODE_5G) >> TIA_GAIN_MODE_5G_SHIFT;
        papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;
        papd_params->epsilon_offset_5g = (((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_eps_offset, 0)) & PAPD_EPS_OFFSET_5G) >>
                PAPD_EPS_OFFSET_5G_SHIFT;
        FOREACH_CORE(pi, core) {
            papd_params->bbmult_2g[core] = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_bbmult, 90)) & PAPD_BBMULT_2G;
        }
        papd_params->bbmult_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_bbmult, 90)) & PAPD_BBMULT_5G) >> PAPD_BBMULT_5G_SHIFT;
        FOREACH_CORE(pi, core) {
            papd_gainctrl_calidx_2g[core] = papd_params->papd_calidx_2g;
            papd_gainctrl_calidx_5g[core] = papd_params->papd_calidx_5g;
        }
        if (WBPAPD_ENAB(pi)) {
            papd_params->tia_mode_2g = 0x6;
            papd_params->cal_adc_mode = 2; /* 180 MHz mode */
            papd_params->wbcal_lutstep_dB = 10240;
            papd_params->wbcal_lut_len = 64;
            papd_params->wbcal_start = 512*4;
            papd_params->wbcal_end   = 143384;
            papd_params->wbcal_scale_start = 600;
            papd_params->wbcal_scale_end   = 1624;
            papd_params->wbcal_dcc_en      = 1;
            papd_params->wbcal_dly_filt_en = 1;
            papd_params->wbcal_dccorr_ovr_en = 0;
            papd_params->wbcal_dcoffset_real = 0;
            papd_params->wbcal_dcoffset_imag = 0;
            papd_params->wbcal_twotbl_en = 0;
            papd_params->wbcal_phyreg0_val = 0xA924;
            papd_params->eps_stop_idx = 38;
            papd_params->wbcal_const_pow_scale = 710/2;
            papd_params->wbcal_macbuf_offset = 65536;
            papd_params->wbcal_waveform_sz = 4000;
            papd_params->buf_offset_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_txbuf_offset, 33)) & PAPD_BUF_OFFSET_2G;
            papd_params->delayfilt_gamma_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_frac_del, 26)) & PAPD_FRACDELAY_OFFSET_2G;
            /* By default frac bits is 11 */
            papd_params->wbcal_gfrac_bits = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_g_frac_bits, 0xBB));
            papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;
        }
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        pdgain2g = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_pdgain2g, 0);
        papd_params->rx_atten_2g =  ((uint32)PHY_GETINTVAR_DEFAULT(pi,
            rstr_nb_rx_attn, 0)) & PAPD_RX_ATTN_2G;
        papd_params->tx_atten_2g = 0x3;
        papd_params->rx_atten_5g = (((uint32)PHY_GETINTVAR_DEFAULT(pi,
            rstr_nb_rx_attn, 0 << PAPD_RX_ATTN_5G_SHIFT)) &
            PAPD_RX_ATTN_5G) >> PAPD_RX_ATTN_5G_SHIFT;
        papd_params->tx_atten_5g = 0x3;
        papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
            rstr_nb_papd_calidx, 32)) & PAPD_CAL_IDX_2G;
        papd_params->papd_calidx_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
            rstr_nb_papd_calidx, 32 << PAPD_CAL_IDX_5G_SHIFT)) &
            PAPD_CAL_IDX_5G) >> PAPD_CAL_IDX_5G_SHIFT;
        FOREACH_CORE(pi, core) {
            papd_gainctrl_calidx_2g[core] = papd_params->papd_calidx_2g;
            papd_gainctrl_calidx_5g[core] = papd_params->papd_calidx_5g;
        }
        papd_params->papd_iter = 32;
        papd_params->tia_mode_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_tia_gain_mode, 4)) & TIA_GAIN_MODE_2G;
        papd_params->tia_mode_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_tia_gain_mode, 4 << TIA_GAIN_MODE_5G_SHIFT)) &
                TIA_GAIN_MODE_5G) >> TIA_GAIN_MODE_5G_SHIFT;
        papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;
        papd_params->epsilon_offset_5g = (((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_eps_offset, 0)) & PAPD_EPS_OFFSET_5G) >>
                PAPD_EPS_OFFSET_5G_SHIFT;
        if (WBPAPD_ENAB(pi)) {
            papd_params->dac_rate_mode = 2; // 20M and 40M
            papd_params->dac_rate_mode_80M_160M = 1; // 80M and 160M
            papd_params->highres_en = 1;
            papd_params->cal_adc_mode = 2; /* 180 MHz mode */
            if (papd_params->highres_en)
                papd_params->wbcal_lutstep_dB = 20480;
            else
                papd_params->wbcal_lutstep_dB = 10240;
            papd_params->wbcal_lut_len = 128;
            papd_params->wbcal_start = 13500;
            papd_params->wbcal_end   = 208920;
            papd_params->wbcal_scale_start = 600;
            papd_params->wbcal_scale_end   = 2648;
            papd_params->wbcal_dcc_en      = 1;
            papd_params->wbcal_dc_accum_wait = 6;
            papd_params->wbcal_dly_filt_en = 0;
            papd_params->wbcal_dccorr_ovr_en = 0;
            papd_params->wbcal_dcoffset_real = 0;
            papd_params->wbcal_dcoffset_imag = 0;
            papd_params->wbcal_twotbl_en = 0;
            papd_params->wbcal_phyreg0_val = 0x2924;
            papd_params->eps_stop_idx = 38;
            papd_params->wbcal_const_pow_scale = 128;
            papd_params->wbcal_window_len_pow = 11;
            papd_params->wbcal_macbuf_offset = 65536;
            papd_params->wbcal_waveform_sz = 4000;
            papd_params->rx_atten_2g =  ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_rx_attn, 0)) & PAPD_RX_ATTN_2G;
            papd_params->rx_atten_5g = (((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_rx_attn, 3 << PAPD_RX_ATTN_5G_SHIFT)) &
                PAPD_RX_ATTN_5G) >> PAPD_RX_ATTN_5G_SHIFT;
            papd_params->buf_offset_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_txbuf_offset, 33)) & PAPD_BUF_OFFSET_2G;
            papd_params->buf_offset_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_txbuf_offset, 34 << PAPD_BUF_OFFSET_5G_SHIFT)) &
                PAPD_BUF_OFFSET_5G) >> PAPD_BUF_OFFSET_5G_SHIFT;
            papd_params->buf_offset_5g_160M = 0x2c;
            //papd_params->delayfilt_gamma_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
            //    rstr_wb_frac_del, 26)) & PAPD_FRACDELAY_OFFSET_2G;
            papd_params->delayfilt_gamma_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_frac_del, 26 << PAPD_FRACDELAY_OFFSET_5G_SHIFT)) &
                PAPD_FRACDELAY_OFFSET_5G) >> PAPD_FRACDELAY_OFFSET_5G_SHIFT;
            /* By default frac bits is 11 */
            papd_params->wbcal_gfrac_bits = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_g_frac_bits, 0xBB));
            if (pdgain2g == 5) {
                // Sky 85336-11 ES5 FEM.
                papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                    rstr_wb_eps_offset, 0x1bb)) & PAPD_EPS_OFFSET_2G;
                papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                    rstr_wb_papd_calidx, 35)) & PAPD_CAL_IDX_2G;
            } else {
                // Sky 85336-11 ES6 FEM.
                papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                    rstr_wb_eps_offset, 0x1b9)) & PAPD_EPS_OFFSET_2G;
                papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                    rstr_wb_papd_calidx, 33)) & PAPD_CAL_IDX_2G;
            }
            // target_sum_RX = 160, log10(160) = 2257/2^10.
            papd_params->target_sum_RX = 2257;
            papd_params->epsilon_offset_5g = (((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_eps_offset, 0x1ce << PAPD_EPS_OFFSET_5G_SHIFT)) &
                PAPD_EPS_OFFSET_5G) >> PAPD_EPS_OFFSET_5G_SHIFT;
            papd_params->papd_calidx_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_papd_calidx, 39 << PAPD_CAL_IDX_5G_SHIFT)) &
                PAPD_CAL_IDX_5G) >> PAPD_CAL_IDX_5G_SHIFT;
            FOREACH_CORE(pi, core) {
                papd_params->bbmult_2g[core] = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_bbmult, 58)) & PAPD_BBMULT_2G;
                papd_params->bbmult_2g_40M[core] = 82;
            }
            papd_params->bbmult_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_bbmult, 58 << PAPD_BBMULT_5G_SHIFT)) &
                PAPD_BBMULT_5G) >> PAPD_BBMULT_5G_SHIFT; // 20M
            papd_params->bbmult_5g_40M = 82; // 40M
            papd_params->bbmult_5g_80M_160M = 116; // 80M and 160M
            papd_params->cal_refdb_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_calref_db, 11)) & PAPD_CALREF_DB_2G;
            papd_params->cal_refdb_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_calref_db, 11 << PAPD_CALREF_DB_5G_SHIFT)) &
                PAPD_CALREF_DB_5G) >> PAPD_CALREF_DB_5G_SHIFT;
            papd_params->tia_mode_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_tia_gain_mode, 4)) & TIA_GAIN_MODE_2G;
            papd_params->tia_mode_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_tia_gain_mode, 6 << TIA_GAIN_MODE_5G_SHIFT)) &
                TIA_GAIN_MODE_5G) >> TIA_GAIN_MODE_5G_SHIFT;
            papd_params->num_eps_tables = 2;
            papd_params->tx_idx_thresh_tbl[0] = 43;
            papd_params->tx_idx_thresh_tbl[1] = 127;
            papd_params->papd_calidx_tbl[0] = 33;
            papd_params->papd_calidx_tbl[1] = 46;
            papd_params->tia_mode_tbl[0] = 5;
            papd_params->tia_mode_tbl[1] = 6;
            papd_params->cal_refdb_tbl[0] = 11;
            papd_params->cal_refdb_tbl[1] = 17;
            papd_params->eps_offset_lut[0] = 0;
            papd_params->eps_offset_lut[1] = 19;
        }
    } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
        papd_params->rx_atten_2g = 0x3;
        papd_params->tx_atten_2g = 0x3;
        papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_papd_calidx, PHY_EPAPD(pi) ? 48: 26)) & PAPD_CAL_IDX_2G;
        papd_params->papd_iter = 64;
        papd_params->tia_mode_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_tia_gain_mode, 0x2)) & TIA_GAIN_MODE_2G;
        papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_nb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;

        papd_params->papd_loopback_mode = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_papd_loopback_mode, MAINGM_PATH));
        papd_params->papd_bypass_mode = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_papd_bypass_mode, 1));

        if (WBPAPD_ENAB(pi)) {
            if (CHSPEC_IS20(pi->radio_chanspec) || CHSPEC_IS40(pi->radio_chanspec))
                papd_params->dac_rate_mode = 2;
            papd_params->papd_calidx_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_papd_calidx, PHY_EPAPD(pi) ? 48: 26)) & PAPD_CAL_IDX_2G;
            papd_params->tia_mode_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_tia_gain_mode, 0x2)) & TIA_GAIN_MODE_2G;
            papd_params->highres_en = 0;
            papd_params->cal_adc_mode = 2; /* 180 MHz mode */
            papd_params->wbcal_lutstep_dB = 10240;
            papd_params->wbcal_lut_len = 64;
            papd_params->wbcal_start = 13500;
            papd_params->wbcal_end   = 208920;
            papd_params->wbcal_scale_start = 600;
            papd_params->wbcal_scale_end   = 2648;
            papd_params->wbcal_dcc_en      = 1;
            papd_params->wbcal_dc_accum_wait = 6;
            papd_params->wbcal_dly_filt_en = 0;
            papd_params->wbcal_dccorr_ovr_en = 0;
            papd_params->wbcal_dcoffset_real = 0;
            papd_params->wbcal_dcoffset_imag = 0;
            papd_params->wbcal_twotbl_en = 0;
            papd_params->wbcal_phyreg0_val = 0xA924;
            papd_params->eps_stop_idx = 38;
            papd_params->wbcal_const_pow_scale = 128;
            papd_params->wbcal_window_len_pow = 11;
            papd_params->wbcal_macbuf_offset = 65536;
            papd_params->wbcal_waveform_sz = 4000;
            papd_params->rx_atten_2g =  ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_rx_attn, 3)) & PAPD_RX_ATTN_2G;
            papd_params->rx_atten_5g = (((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_rx_attn, 3 << PAPD_RX_ATTN_5G_SHIFT)) &
                PAPD_RX_ATTN_5G) >> PAPD_RX_ATTN_5G_SHIFT;
            papd_params->buf_offset_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_txbuf_offset, 33)) & PAPD_BUF_OFFSET_2G;
            papd_params->buf_offset_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_txbuf_offset, 33 << PAPD_BUF_OFFSET_5G_SHIFT)) &
                PAPD_BUF_OFFSET_5G) >> PAPD_BUF_OFFSET_5G_SHIFT;
            papd_params->delayfilt_gamma_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_frac_del, 26)) & PAPD_FRACDELAY_OFFSET_2G;
            /* By default frac bits is 11 */
            papd_params->wbcal_gfrac_bits = ((uint8)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_g_frac_bits, 0xBB));

            // target_sum_RX = 160, log10(160) = 2257/2^10.
            papd_params->target_sum_RX = 2257;
            papd_params->epsilon_offset_2g = ((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_eps_offset, 0)) & PAPD_EPS_OFFSET_2G;
            papd_params->epsilon_offset_5g = (((uint32)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_eps_offset, 0x1c4 << PAPD_EPS_OFFSET_5G_SHIFT)) &
                PAPD_EPS_OFFSET_5G) >> PAPD_EPS_OFFSET_5G_SHIFT;
            papd_params->papd_calidx_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_papd_calidx, 32 << PAPD_CAL_IDX_5G_SHIFT)) &
                PAPD_CAL_IDX_5G) >> PAPD_CAL_IDX_5G_SHIFT;
            FOREACH_CORE(pi, core) {
                papd_params->bbmult_2g[core] = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                    rstr_wb_bbmult, 58)) & PAPD_BBMULT_2G;
            }
            papd_params->bbmult_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_bbmult, 58 << PAPD_BBMULT_5G_SHIFT)) &
                PAPD_BBMULT_5G) >> PAPD_BBMULT_5G_SHIFT;
            papd_params->cal_refdb_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_calref_db, 11)) & PAPD_CALREF_DB_2G;
            papd_params->cal_refdb_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_calref_db, 11 << PAPD_CALREF_DB_5G_SHIFT)) &
                PAPD_CALREF_DB_5G) >> PAPD_CALREF_DB_5G_SHIFT;
            papd_params->tia_mode_2g = ((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_tia_gain_mode, 4)) & TIA_GAIN_MODE_2G;
            papd_params->tia_mode_5g = (((uint16)PHY_GETINTVAR_DEFAULT(pi,
                rstr_wb_tia_gain_mode, 4 << TIA_GAIN_MODE_5G_SHIFT)) &
                TIA_GAIN_MODE_5G) >> TIA_GAIN_MODE_5G_SHIFT;
        }
    } else {
        papd_params->rx_atten_2g = 0x3;
        papd_params->rx_atten_5g = 0x3;
        papd_params->tx_atten_2g = 0x3;
        papd_params->tx_atten_5g = 0x3;
        papd_params->papd_calidx_2g = PHY_EPAPD(pi) ? 48: 26;

    }

    /* setup ptr */
    papd_info->papd_params = papd_params;

    return (BCME_OK);

fail:
    if (papd_params != NULL)
        phy_mfree(pi, papd_params, sizeof(phy_ac_papdcal_params_t));

    return (BCME_NOMEM);

}

#if (defined(WLTEST) || defined(WLPKTENG))
static bool
wlc_phy_isperratedpden_acphy(phy_type_papdcal_ctx_t *ctx)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    phy_info_t *pi = info->pi;

    if (CHSPEC_IS2G(pi->radio_chanspec))
        return (pi->papdcali->data->epacal2g && info->perratedpd2g);
    else
        return (pi->papdcali->data->epacal5g && info->perratedpd5g);
}

static void
wlc_phy_perratedpdset_acphy(phy_type_papdcal_ctx_t *ctx, bool enable)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    phy_info_t *pi = info->pi;

    /* Set the new value */
    MOD_PHYREG(pi, PapdEnable0, papd_compEnb0, enable);
}
#endif

#if defined(WLTEST)
static int
phy_ac_papdcal_get_lut_idx0(phy_type_papdcal_ctx_t *ctx, int32* idx)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *idx = (int32)info->papd_lut0_cal_idx;
    return BCME_OK;
}

static int
phy_ac_papdcal_get_lut_idx1(phy_type_papdcal_ctx_t *ctx, int32* idx)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *idx = (int32)info->papd_lut1_cal_idx;
    return BCME_OK;
}

static int phy_ac_papdcal_get_idx(phy_type_papdcal_ctx_t *ctx, int32* idx)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *idx = (int32)info->pacalidx_iovar;
    return BCME_OK;
}

static int phy_ac_papdcal_set_idx(phy_type_papdcal_ctx_t *ctx, int32 idx)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (idx < -1 || idx > 127) {
        return BCME_ERROR;
    } else {
        info->pacalidx_iovar = idx;
        return BCME_OK;
    }
}

static int
phy_ac_papdcal_get_bbmult(phy_type_papdcal_ctx_t *ctx, int32* bbmult)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *bbmult = (int32)info->papdbbmult_iovar;
    return BCME_OK;
}

static int
phy_ac_papdcal_set_bbmult(phy_type_papdcal_ctx_t *ctx, int32 bbmult)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (bbmult < -1 || bbmult > 255) {
        return BCME_ERROR;
    } else {
        info->papdbbmult_iovar = bbmult;
        return BCME_OK;
    }
}

static int
phy_ac_papdcal_get_extraepsoffset(phy_type_papdcal_ctx_t *ctx, int32* extraepsoffset)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *extraepsoffset = (int32)info->extraepsoffset_iovar;
    return BCME_OK;
}

static int
phy_ac_papdcal_set_extraepsoffset(phy_type_papdcal_ctx_t *ctx, int32 extraepsoffset)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (extraepsoffset < -20 || extraepsoffset > 20) {
        return BCME_ERROR;
    } else {
        info->extraepsoffset_iovar = extraepsoffset;
        return BCME_OK;
    }
}

static int
phy_ac_papdcal_set_tiagain(phy_type_papdcal_ctx_t *ctx, int8 tiagain)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (tiagain < -1 || tiagain > 9 || tiagain == 0) {
        return BCME_ERROR;
    } else {
        info->papdtiagain_iovar = tiagain;
        return BCME_OK;
    }
}

static int
phy_ac_papdcal_get_tiagain(phy_type_papdcal_ctx_t *ctx, int32* tiagain)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *tiagain = (int32)info->papdtiagain_iovar;
    return BCME_OK;
}

static int
phy_ac_papdcal_set_comp_disable(phy_type_papdcal_ctx_t *ctx, int8 comp_disable)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    phy_info_t *pi = info->pi;
    if (comp_disable == 1 || comp_disable == 0) {
        uint8 core;
        FOREACH_CORE(pi, core) {
            MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, !comp_disable);
            MOD_PHYREGCEE(pi, PapdEnable, core, papd_compCckEnb, !comp_disable);
        }
        info->comp_disable_iovar = comp_disable;
    } else if (comp_disable == -1) {
        info->comp_disable_iovar = comp_disable;
    } else {
        return BCME_ERROR;
    }
    return BCME_OK;
}

static int
phy_ac_papdcal_get_comp_disable(phy_type_papdcal_ctx_t *ctx, int32* comp_disable)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *comp_disable = (int32)info->comp_disable_iovar;
    return BCME_OK;
}

static int
phy_ac_papdcal_get_end_epstblidx(phy_type_papdcal_ctx_t *ctx, int32* end_epstblidx)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *end_epstblidx = (int32)info->end_epstblidx_iovar;
    return BCME_OK;
}

static int
phy_ac_papdcal_set_end_epstblidx(phy_type_papdcal_ctx_t *ctx, int32 end_epstblidx)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (end_epstblidx < 0 || end_epstblidx > 63) {
        return BCME_ERROR;
    } else {
        info->end_epstblidx_iovar = end_epstblidx;
        return BCME_OK;
    }
}

static int
phy_ac_papdcal_get_epstblsel(phy_type_papdcal_ctx_t *ctx, int32* epstblsel)
{
    int32 epstblval;
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    phy_info_t *pi = info->pi;
    if (READ_PHYREGFLDCEE(pi, PapdLutSel0, 0, papd_lut_select_ovr) == 0)
        epstblval = 0;
    else
        epstblval = READ_PHYREGFLDCEE(pi, PapdLutSel0, 0, papd_lut_select_ovr_val);
    *epstblsel = epstblval;
    return BCME_OK;
}

static int
phy_ac_papdcal_set_epstblsel(phy_type_papdcal_ctx_t *ctx, int32 epstblsel)
{
    uint8 core;
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    phy_info_t *pi = info->pi;
    if (epstblsel < 0 || epstblsel > 3) {
        return BCME_ERROR;
    } else {
        info->epstblsel_iovar = epstblsel;
        for (core = 0; core < PHYCORENUM((pi)->pubpi->phy_corenum); core++) {
            MOD_PHYREGCEE(pi, PapdLutSel0, core,
                papd_lut_select_ovr_val, epstblsel);
            MOD_PHYREGCEE(pi, PapdLutSel0, core,
                papd_lut_select_ovr_val_cck, epstblsel);
            MOD_PHYREGCEE(pi, PapdLutSel0, core,
                papd_lut_select_ovr, (epstblsel == 0) ? 0 : 1);
        }
        return BCME_OK;
    }
}

static int
phy_ac_papdcal_set_dump(phy_type_papdcal_ctx_t *ctx, int8 papdcal_dump)
{
    if (papdcal_dump == 1 || papdcal_dump == 0) {
        papdcal_dumpinfo = papdcal_dump;
        return BCME_OK;
    } else {
        return BCME_ERROR;
    }
}

static int
phy_ac_papdcal_get_dump(phy_type_papdcal_ctx_t *ctx, int32* papdcal_dump)
{
    *papdcal_dump = (int32)papdcal_dumpinfo;
    return BCME_OK;
}

static int
phy_ac_papdcal_get_abort(phy_type_papdcal_ctx_t *ctx, int32* papd_abort)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *papd_abort = (int32)info->papd_abort;
    return BCME_OK;
}

static int
phy_ac_papdcal_set_abort(phy_type_papdcal_ctx_t *ctx, int32 papd_abort)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (papd_abort < 0 || papd_abort > 1) {
        return BCME_ERROR;
    } else {
        info->papd_abort = papd_abort;
        return BCME_OK;
    }
}

static int
phy_ac_papdcal_get_wbpapd_gctrl(phy_type_papdcal_ctx_t *ctx, int32* wbpapd_gctrl)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *wbpapd_gctrl = (int32)info->wbpapd_gctrl_iovar;
    return BCME_OK;
}

static int
phy_ac_papdcal_set_wbpapd_gctrl(phy_type_papdcal_ctx_t *ctx, int32 wbpapd_gctrl)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (wbpapd_gctrl < 0 || wbpapd_gctrl > 2) {
        return BCME_ERROR;
    } else {
        info->wbpapd_gctrl_iovar = wbpapd_gctrl;
        return BCME_OK;
    }
}

static int
phy_ac_papdcal_get_wbpapd_multitbl(phy_type_papdcal_ctx_t *ctx, int32* wbpapd_multitbl)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *wbpapd_multitbl = (int32)info->wbpapd_multitbl_iovar;
    return BCME_OK;
}

static int
phy_ac_papdcal_set_wbpapd_multitbl(phy_type_papdcal_ctx_t *ctx, int32 wbpapd_multitbl)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (wbpapd_multitbl < 0 || wbpapd_multitbl > 1) {
        return BCME_ERROR;
    } else {
        info->wbpapd_multitbl_iovar = wbpapd_multitbl;
        return BCME_OK;
    }
}

#endif

#if defined(WLTEST) || defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
#ifndef ATE_BUILD
static int
phy_ac_papdcal_set_skip(phy_type_papdcal_ctx_t *ctx, uint8 skip)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    if (skip == 0 || skip == 1)
        info->acphy_papd_skip = skip;
    return BCME_OK;
}

static int
phy_ac_papdcal_get_skip(phy_type_papdcal_ctx_t *ctx, int32 *skip)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    *skip = (int32)info->acphy_papd_skip;
    return BCME_OK;
}
#endif /* !ATE_BUILD */
#endif

void
phy_ac_papdcal_get_gainparams(phy_ac_papdcal_info_t *papdcali, uint8 *Gainoverwrite, uint8 *PAgain)
{
    if (PHY_IPA(papdcali->pi)) {
        *Gainoverwrite = (CHSPEC_IS2G(papdcali->pi->radio_chanspec)) ?
            papdcali->srom_pagc2g_ovr :
            papdcali->srom_pagc5g_ovr;
        *PAgain = (CHSPEC_IS2G(papdcali->pi->radio_chanspec)) ?
            papdcali->srom_pagc2g :
            papdcali->srom_pagc5g;
    }
}

/* ********************************************* */
/*                Internal Definitions                    */
/* ********************************************* */
#define PAPD_GAIN_CTRL
#define ACPHY_SPINWAIT_PAPDCAL            5000000

static int8 gain_ctrl = 0; /* Use this flag for gain control using analytic PAPD */
static const int8 *rfpwrlut_ptr;
static const uint32 *macplay_wfm_ptr;

#ifdef PAPD_GAIN_CTRL
static uint16 phy_ac_papd_gain_ctrl_acradio(phy_info_t *pi, uint16 num_iter, uint8 core,
    uint16 startindex, uint16 yrefindex, uint16 stopindex);
static uint8 phy_ac_papd_gain_ctrl_28nm(phy_info_t *pi, uint8 core, uint16 yrefindex,
        uint16 stopindex, uint16 bbmult_init);
static void phy_ac_papd_write_tx_gain(phy_info_t *pi, uint8 core,
    acphy_txgains_t *target_gain, uint16 *bbmult, bool zero_rfgain);
#endif /* PAPD_GAIN_CTRL */

static void phy_ac_papd_radio_lpbk_cleanup_acradio(phy_info_t *pi, uint8 core);
static void phy_ac_papd_radio_lpbk_setup_acradio(phy_info_t *pi, uint16 tx_atten,
    uint16 rx_atten, uint8 core);
static void phy_ac_get_tx_gain(phy_info_t *pi, uint8 core_no, acphy_txgains_t *target_gain);
static void phy_ac_papd_smooth(phy_info_t *pi, uint8 core,
    uint32 winsz, uint32 start, uint32 end);
static void phy_ac_papd_radio_lpbk_setup_tiny(phy_info_t *pi,
    uint16 tx_atten, uint16 rx_atten);
static void phy_ac_papd_radio_lpbk_setup_20704(phy_info_t *pi, bool epapd_flag);
static void phy_ac_papd_radio_lpbk_setup_20707(phy_info_t *pi);
static void phy_ac_papd_radio_lpbk_setup_20708(phy_info_t *pi, uint16 tx_atten, uint16 rx_atten);
static void phy_ac_papd_radio_lpbk_setup_20709(phy_info_t *pi, bool epapd_flag);
static void phy_ac_papd_radio_lpbk_setup_20710(phy_info_t *pi, bool epapd_flag);
static void phy_ac_papd_radio_lpbk_cleanup_tiny(phy_info_t *pi);
static void phy_ac_papd_radio_lpbk_cleanup_28nm(phy_info_t *pi);
static void phy_ac_papd_radio_lpbk_cleanup_majorrev51_128_130_131(phy_info_t *pi);
static void phy_ac_papd_set_tia_gain_tiny(phy_info_t *pi, uint16 gain, uint8 core);
static void phy_ac_apapd_init_seq(phy_info_t *pi, uint8 core, uint16 yrefindex);
static void phy_ac_papd_turningoff_inactivecore(phy_info_t *pi, uint8 core);
static uint8 phy_ac_papd_gain_ctrl_tiny(phy_info_t *pi, uint8 core, uint16 yrefindex);
static int8 phy_ac_wbcal_eps_stopidx(phy_info_t *pi, uint8 core);
static void phy_ac_papd_tiagain_search_majrev129(phy_info_t *pi, uint8 core, uint16 tiagain_bbmult,
                                                 bool tia_2nd);
static uint32 phy_ac_papd_adc_pow_est(phy_info_t *pi, uint8 core, bool dBx8);
static void phy_ac_wbcal_widen_filter(phy_info_t *pi, uint8 tia_mode, uint8 core);
static void tia_calc_20708(phy_info_t *pi, uint8 bq1_gain, uint8 core);
static void lpf_calc_20708(phy_info_t *pi, uint8 core);
static void tia_lpf_bias_calc_20708(phy_info_t *pi, uint8 core);
static bool calculate_calibrated_epsOffset(phy_info_t *pi, uint8 core, uint8 cal_idx,
    uint8 papd_lut_select, uint8 gfrac_bits, uint32 *original_eps_table, int16 *epsilonOffset);
static void set_epsilon_offset(phy_info_t *pi, uint8 core, bool use_calc, int16 calc_epsoffset);

const uint32 *BCMRAMFN(get_wbpapd_wfm_phyrev130)(phy_info_t *pi);

#ifdef PAPD_GAIN_CTRL
static uint16
phy_ac_papd_gain_ctrl_acradio(phy_info_t *pi, uint16 num_iter, uint8 core, uint16 startindex,
    uint16 yrefindex, uint16 stopindex)
{
#define EPS_MAX 4095
#define EPS_MIN -4095
#define GAINCTRL_ITER 8
    bool clipping_flag = 0;
    uint8 i = 0;
    /* Change PRF to avoid radar detection */
    uint16 numidx_array[GAINCTRL_ITER] = {4, 5, 6, 7, 4, 5, 6, 7};
    uint8 PAPD_FAST_GAIN_CTRL = 0;
    uint16 pga_u = 255, pga_l = 0, pga_mid = 127;
    uint32 tempclip = 0;
    uint32 clipthresholdl = 47894000, clipthresholdu = 49034000, clipthreshold = 48462000;
    acphy_txgains_t tx_gains;
    int32 eps_re, eps_im;
    uint16 bbmult;
    uint32 eps_complex;
    uint16 numidx;
    uint8 PAgain = 0xff;
    uint8 Gainoverwrite = 0;
    uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2};
    phy_ac_papdcal_info_t *papdi = pi->u.pi_acphy->papdcali;
    uint8 edpdcalset = papdi->edpdcalset;
    uint8 papdmode = papdi->papdmode;

    if (PHY_IPA(pi) && ACMAJORREV_2(pi->pubpi->phy_rev)) {
        PAPD_FAST_GAIN_CTRL = 1;
        /* 43569 */
        if (RADIOREV(pi->pubpi->radiorev) == 0x27 ||
        RADIOREV(pi->pubpi->radiorev) == 0x29 ||
        RADIOREV(pi->pubpi->radiorev) == 0x28 ||
        RADIOREV(pi->pubpi->radiorev) == 0x2C) {
                PAPD_FAST_GAIN_CTRL = 0;
        }
    }

    bbmult = 0x3f;
    tx_gains.txlpf = 0x0;
    tx_gains.txgm = 0xff;
    tx_gains.pga = 0xff;
    tx_gains.pad = 0xff;
    phy_ac_papdcal_get_gainparams(papdi, &Gainoverwrite, &PAgain);
    tx_gains.ipa = (Gainoverwrite == 0) ? 0xff : PAgain;
    if (RADIOMAJORREV(pi) == 2 && PHY_EPAPD(pi)) {
        /* 4350EPAPD */
        tx_gains.txlpf = 0x0;
        tx_gains.txgm = (CHSPEC_IS2G(pi->radio_chanspec)) ? 0x67 : 0x91;
        tx_gains.pga = 0xff;
        tx_gains.pad = (CHSPEC_IS2G(pi->radio_chanspec)) ? 0xff : 0x7f;
        tx_gains.ipa = (CHSPEC_IS2G(pi->radio_chanspec)) ? 0xc0 : 0x3;
    }

    if (PAPD_FAST_GAIN_CTRL) {
        PHY_CAL(("---------- Using Fast Gain Control ------------"));
        if ((papdmode == PAPD_ANALYTIC) || (papdmode == PAPD_ANALYTIC_WO_YREF)) {
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
                bbmult = 0x30;
                if (!CHSPEC_IS80(pi->radio_chanspec)) {
                    clipthresholdl = 59901000;
                    clipthresholdu = 61175000;
                    clipthreshold = 60536000;
                    /* 1.9 */
                } else {
                    clipthresholdl = 47894000;
                    clipthresholdu = 49034000;
                    clipthreshold = 48462000;
                    /* 1.7 */
                }
            }
        } else {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                clipthresholdl = 30110000;
                clipthresholdu = 31016000;
                /* clipthreshold 1.35 */
                clipthreshold = 30562000;
            }
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
                bbmult = 0x30;
                if (!CHSPEC_IS80(pi->radio_chanspec)) {
                    clipthresholdl = 39255000;
                    clipthresholdu = 40288000;
                    clipthreshold = 39769000;
                } else {
                    clipthresholdl = 32399000;
                    clipthresholdu = 33338000;
                    clipthreshold = 32867289;
                    /* 1.2 */
                }
            }
        }
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* AM2AM of (4095 * x * d) ^ 2
         * Where x = 1.2 and d = 0.95, 1, 1.05 for 2G
         *       x = 1.25 and d = 0.95, 1, 1.05 for 5G
         */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            clipthresholdl  = 21793025;
            clipthresholdu  = 26622504;
            clipthreshold   = 24147396;
        } else {
            clipthresholdl  = 23646945;
            clipthresholdu  = 28887266;
            clipthreshold   = 26201602;
        }

    } else {
        numidx = 16;
    }

    if (RADIOMAJORREV(pi) == 2 && PHY_EPAPD(pi)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            clipthresholdl = 13886000;
            clipthresholdu = 14504000;
            clipthreshold = 14193000;
            pga_u = 127;
            pga_l = 40;
            pga_mid = 84;
        } else {
            if (edpdcalset == 1) {
                if (CHSPEC_IS20(pi->radio_chanspec)) {
                    clipthresholdl = 32399000;
                    clipthresholdu = 33338000;
                    clipthreshold = 32867000;
                } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                    clipthresholdl = 27905000;
                    clipthresholdu = 28777000;
                    clipthreshold = 28340000;
                } else {
                    clipthresholdl = 19923000;
                    clipthresholdu = 20661000;
                    clipthreshold = 20291000;
                }
            }
        }
    }

    /* Continuous tone throughout gain control to avoid radar detection */
    (void)wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_1MHz, 186, TX_TONE_IQCAL_MODE_OFF,
        FALSE);
    /* Binary search */
    for (i = 1; i <= GAINCTRL_ITER; i++) {
        /* For fast papd numidx value is changed per iteration otherwise fixed to 16 */
        if (PAPD_FAST_GAIN_CTRL) {
            numidx = numidx_array[i-1];
        }
        tx_gains.pga = pga_mid;
        phy_ac_papd_write_tx_gain(pi, core, &tx_gains, &bbmult, 0);
        /* TODO: check start and stop indexs do we want to check more than the last index */
        phy_ac_papd_cal(pi, num_iter, core, stopindex - numidx + 1,
            yrefindex, stopindex);
        wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core], 1, 63, 32,
            &eps_complex, core);
        phy_papdcal_decode_epsilon(eps_complex, &eps_re, &eps_im);
        tempclip = (4095 + eps_re) * (4095 + eps_re) + (eps_im * eps_im);
        if (tempclip >= clipthreshold)
            clipping_flag = 1;
        else
            clipping_flag = 0;

        if (tempclip >= clipthresholdl && tempclip <= clipthresholdu) {
            break;
        }

        if (clipping_flag)
            pga_u = pga_mid;
        else
            pga_l = pga_mid;

        pga_mid = (pga_u + pga_l)/ 2;
    }
    /* Stop Continuous tone after gain control to avoid radar detection */
    wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);
    return tx_gains.pga;
}

static void
phy_ac_apapd_init_seq(phy_info_t *pi, uint8 core, uint16 yrefindex)
{
    uint32 cal_corr = 0x0a0, corr_shift = 0x9;
    MOD_PHYREGCE(pi, EpsilonOverrideI_, core, epsilonFixedPoint, 0x1);
    MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 1);
    MOD_PHYREGCEE(pi, PapdCalShifts, core, papd_calEnb, 1);
    MOD_PHYREGCEE(pi, PapdEnable, core, avgPapdPowerEnb, 0);
    MOD_PHYREG(pi, PapdCalYrefEpsilon, papdInitYref, 0x1);
    MOD_PHYREG(pi, PapdCalYrefEpsilon, papdEpsilonInit, 0x0);
    cal_corr = (ROUTER_4349(pi)) ? (CHSPEC_IS2G(pi->radio_chanspec)
            ? 0x050 : 0xa00) : 0xa0;
    corr_shift = (ROUTER_4349(pi)) ? (CHSPEC_IS2G(pi->radio_chanspec)
            ? 0x5 : 0x9) : 0x5;
    MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime, cal_corr);
    MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift, corr_shift);
    ACPHY_REG_LIST_START
        MOD_PHYREG_ENTRY(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0x0)
        MOD_PHYREG_ENTRY(pi, PapdIpaOffPower, papd_calIpaOffPower, 0x0)
        MOD_PHYREG_ENTRY(pi, PapdEpsilonUpdateIterations, epsilonUpdateIterations, 1)
    ACPHY_REG_LIST_EXECUTE(pi);
    MOD_PHYREG(pi, PapdCalYrefEpsilon, papdYrefAddr, yrefindex);
    MOD_PHYREG(pi, PapdCalCoreSel, papdCoreSel, core);

}

static void
phy_ac_papd_turningoff_inactivecore(phy_info_t *pi, uint8 core)
{
    uint8 off_core = 0;
    off_core = (core == 0) ? 1: 0;
    MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, off_core, ovr_pa5g_pu, 1);
    MOD_RADIO_REG_TINY(pi, PA5G_CFG4, off_core, pa5g_pu, 0);
    MOD_PHYREGCE(pi, RfctrlOverrideTxPus, off_core, txrf_pwrup, 0x1);
    MOD_PHYREGCE(pi, RfctrlCoreTxPus, off_core, txrf_pwrup, 0x0);
    MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core, ovr_pa5g_pu, 1);
    MOD_RADIO_REG_TINY(pi, PA5G_CFG4, core, pa5g_pu, 1);
    MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, txrf_pwrup, 0x1);
    MOD_PHYREGCE(pi, RfctrlCoreTxPus, core, txrf_pwrup, 0x1);
}

/* gain _ctrl for Tiny papd */

static uint8
phy_ac_papd_gain_ctrl_tiny(phy_info_t *pi, uint8 core, uint16 yrefindex)

{
#define EPS_MAX 4095
#define EPS_MIN -4095
#define NUM_ITER_BINARY_SEARCH 8
#define BB_MID 60
#define BB_UP 90
#define BB_LO 20
#define SCAL_IDX 60
#define TXIDX 30

    int8 i;
    bool is2g = (CHSPEC_IS2G(pi->radio_chanspec));
    int8 tt_u = 0, tt_l = 0, tt_mid = 0, coremask;
    int32 tempclip = 0;
    int32 clipthreshold = 0;
    /* analytic Gain control  with bbmult from B0 onwards */
    uint16 m[4] = {0, 0, 0, 0};
    uint16 scal_idx = SCAL_IDX;
    int16  corr_I = 0, corr_Q = 0;
    int16  Yref_I = 0, Yref_Q = 0;
    int32 y_den = 0, yref_den = 0;
    int32 yref = 0;
    uint8 scalar_table_ids[] = { ACPHY_TBL_ID_SCALAR0, ACPHY_TBL_ID_SCALAR1,
    ACPHY_TBL_ID_SCALAR2};
    wlc_phy_table_write_acphy(pi, scalar_table_ids[core], 128, 0, 32,
            acphy_papd_scaltbl_128);
    phy_ac_apapd_init_seq(pi, core, yrefindex);
    clipthreshold  = (is2g)? 13:19;
    tt_mid = BB_MID;
    tt_u = BB_UP;
    tt_l = BB_LO;
    coremask = (phy_get_phymode(pi) == PHYMODE_RSDB) ? 1 : 3;
    if ((phy_get_phymode(pi) == PHYMODE_MIMO))
        phy_ac_papd_turningoff_inactivecore(pi, core);
    wlc_phy_txpwr_by_index_acphy(pi, core + 1, TXIDX);

    (void)wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_1MHz, 186, TX_TONE_IQCAL_MODE_OFF,
        FALSE);

    for (i = 1; i <= NUM_ITER_BINARY_SEARCH; i++) {
        m[core] = tt_mid;
        wlc_phy_ipa_set_bbmult_acphy(pi, &m[0], &m[1], &m[2], &m[3], coremask);
        MOD_PHYREG(pi, PapdCalAddress, papdStartAddr, scal_idx);
        MOD_PHYREG(pi, PapdCalAddress, papdEndAddr, scal_idx);
        WRITE_PHYREG(pi, papdCalCorrDebugAddr, scal_idx);

        MOD_PHYREG(pi, PapdCalCoreSel, papdCoreSel, core);
        OSL_DELAY(10);
        MOD_PHYREG(pi, PapdCalStart, papdStart, 1);
        SPINWAIT(READ_PHYREG(pi, PapdCalStart), ACPHY_SPINWAIT_PAPDCAL);
        if ((READ_PHYREG(pi, PapdCalStart) & 1)) {
            PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :"
                " PAPD cal failed \n", __FUNCTION__));
            PHY_FATAL_ERROR(pi, PHY_RC_PAPD_CAL_FAILED);
        }
        SPINWAIT(READ_PHYREG(pi, PapdCalStart), ACPHY_SPINWAIT_PAPDCAL);
        corr_I = (int16) (READ_PHYREGCE(pi, papdCalFirstCorr_I, core)
                & 0xffff);
        corr_Q = (int16) (READ_PHYREGCE(pi, papdCalFirstCorr_Q, core)
                & 0xffff);
        Yref_I = (int16) (READ_PHYREGCE(pi, PapdCalYref_I, core) & 0xffff);
        Yref_Q = (int16) (READ_PHYREGCE(pi, PapdCalYref_Q, core) & 0xffff);

        y_den = (int32)corr_I*(int32)corr_I +
            (int32)corr_Q*(int32)corr_Q;
        yref_den =  (int32)Yref_I*(int32)Yref_I +
            (int32)Yref_Q*(int32)Yref_Q;
        yref = yref_den*10;
        tempclip = y_den*clipthreshold;

        if (tempclip >= yref)
            tt_l = tt_mid;
        else
            tt_u = tt_mid;

        tt_mid = (tt_u+tt_l) >> 1;
    }
    wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);

    return tt_mid;
}

static uint8
phy_ac_papd_gain_ctrl_28nm(phy_info_t *pi, uint8 core, uint16 yrefindex,
        uint16 stopindex, uint16 bbmult_init)
{

#define GAINCTRL_ITER_28NM 7

/* Does a binary search to find a BB Mult value that makes the AM-to-AM power reach */
/* a preset target value for the EPS curve to have optimum sampling range */

    bool  clipping_flag = 0;
    uint8 i = 0;
    uint16 m[4] = {0, 0, 0, 0};

    /* Change PRF to avoid radar detection */
    const uint16 numidx_array[GAINCTRL_ITER_28NM] = {3, 4, 5, 6, 3, 4, 5};
    uint8  num_iter = 128;

    uint32 tempclip = 0;
    uint32 temp_re  = 0;
    uint32 clipthresholdl, clipthresholdu, clipthreshold;
    uint8  coremask = pi->pubpi->phy_coremask;
    uint16 bbmult       = 64;
    uint16 bbmult_next;
    uint16 bbmult_lower = 32;
    uint16 bbmult_upper = 128;
    int32  eps_re, eps_im;
    uint32 eps_complex;
    uint16 numidx;
    uint32 eps_ref = 61;
    uint8 epsilon_table_ids[] = {ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
    acphy_papdCalParams_t calParams = {core, stopindex, stopindex,    yrefindex,
        epsilon_table_ids[core], num_iter, 3, 128};
    uint8 calmode = 0;
    bool disable_table_extension = 1;
    bool is2g = (CHSPEC_IS2G(pi->radio_chanspec));
    bool is20M = (CHSPEC_IS20(pi->radio_chanspec)), is40M = (CHSPEC_IS40(pi->radio_chanspec));

    if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        disable_table_extension = READ_PHYREGFLDCE(pi, papdEpsilonTable, core,
            epsilon_tbl_extend_dis);
    }

    /* Initialize bbmult */
    if (pi->u.pi_acphy->papdcali->papd_abort == 1)
        bbmult_next = bbmult_init;
    else
        bbmult_next = 64;

    /* Speeding up gain control */
    if (ACMAJORREV_51_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) {
        calParams.num_iter = 32;
    } else if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        calParams.num_iter = 16;
    }
    eps_ref <<= !disable_table_extension;

    if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        if (is2g) {
            /* AM2AM of (4095 * x * d) ^ 2
            * Where x = 1.31 and d = 0.98, 1, 1.02
            */
            clipthresholdl = 27637742;
            clipthresholdu = 29939928;
            clipthreshold  = 28777324;
        } else {
            uint16 fc = CHAN5G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
            /* AM2AM of (4095 * x * d) ^ 2, d = 0.98, 1, 1.02 */
            if (is20M) {
                if (fc <= 5320) {
                    /* x = 1.32 for low 5G */
                    clipthresholdl = 28061303;
                    clipthresholdu = 30398770;
                    clipthreshold  = 29218349;
                } else if (fc <= 5710) {
                    /* x = 1.40 for mid 5G */
                    clipthresholdl = 31565744;
                    clipthresholdu = 34195127;
                    clipthreshold  = 32867289;
                } else {
                    /* x = 1.32 for low and high 5G */
                    clipthresholdl = 28061303;
                    clipthresholdu = 30398770;
                    clipthreshold  = 29218349;
                }
            } else if (is40M) {
                if (fc <= 5320) {
                    /* x = 1.32 for low 5G */
                    clipthresholdl = 28061303;
                    clipthresholdu = 30398770;
                    clipthreshold  = 29218349;
                } else if (fc <= 5710) {
                    /* x = 1.40 for mid 5G */
                    clipthresholdl = 31565744;
                    clipthresholdu = 34195127;
                    clipthreshold  = 32867289;
                } else {
                    /* x = 1.40 for high 5G */
                    clipthresholdl = 31565744;
                    clipthresholdu = 34195127;
                    clipthreshold  = 32867289;
                }
            } else {
                if (fc <= 5320) {
                    /* x = 1.32 for low 5G */
                    clipthresholdl = 28061303;
                    clipthresholdu = 30398770;
                    clipthreshold  = 29218349;
                } else if (fc <= 5710) {
                    /* x = 1.45 for mid 5G */
                    clipthresholdl = 33860703;
                    clipthresholdu = 36681253;
                    clipthreshold  = 35256875;
                } else {
                    /* x = 1.38 for high 5G */
                    clipthresholdl = 30670308;
                    clipthresholdu = 33225102;
                    clipthreshold  = 31934931;
                }
            }
            bbmult_upper = 192;
        }
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* AM2AM of (4095 * x * d) ^ 2
         * Where x = 1.2 and d = 0.95, 1, 1.05 for 2G
         *       x = 1.25 and d = 0.95, 1, 1.05 for 5G
         */
        clipthresholdl  = (is2g ? 21793025 : 23646945);
        clipthresholdu  = (is2g ? 26622504 : 28887266);
        clipthreshold   = (is2g ? 24147396 : 26201602);

    } else {
        /* AM2AM of (4095 * x * d) ^ 2
         * Where x = 1.5 and d = 0.97, 1, 1.03 for 2G
         *       x = 1.55 and d = 0.98, 1, 1.02 for 5G
         */
        clipthresholdl = (is2g ? 35500445 : 38692194);
        clipthresholdu = (is2g ? 40028082 : 41915201);
        clipthreshold  = (is2g ? 37730306 : 40287583);
    }

    /* Binary search */
    for (i = 0; i < GAINCTRL_ITER_28NM; i++) {

        bbmult    = bbmult_next;
        numidx    = numidx_array[i];
        if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
            calParams.startindex = 57;
        } else {
            calParams.startindex = stopindex - numidx;
        }

        m[core] = bbmult;

        /* When setting BB Mult, coremask needs to enable all cores */
        wlc_phy_ipa_set_bbmult_acphy(pi, &m[0], &m[1], &m[2], &m[3], coremask);

        if (!ACMAJORREV_51_131(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev) &&
            !ACMAJORREV_130(pi->pubpi->phy_rev)) {
            if (i == 0) {
                /* Preheat is required for the first run */
                phy_ac_papd_cal_mode0_1(pi, &calParams, &calmode);
                phy_ac_papd_cal_mode0_1(pi, &calParams, &calmode);
            }
        }
        phy_ac_papd_cal_mode0_1(pi, &calParams, &calmode);

        wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core], 1, eps_ref, 32,
            &eps_complex, core);
        phy_papdcal_decode_epsilon(eps_complex, &eps_re, &eps_im);

        /* Compute the AM-to-AM power */
        temp_re  = 4095 + eps_re;
        tempclip = (temp_re * temp_re) + (eps_im * eps_im);

        /* Compare the AM-to-AM power to the threshold to determine clipping */
        if (ACMAJORREV_129_130(pi->pubpi->phy_rev) && is2g) {
            if (tempclip >= clipthreshold || eps_im == 4095 || eps_re == 4095)
                clipping_flag = 1;
            else
                clipping_flag = 0;
        } else {
            if (tempclip >= clipthreshold)
                clipping_flag = 1;
            else
                clipping_flag = 0;
        }

        /* Determine the next BB Mult value by reducing the search window */
        if (clipping_flag)
            bbmult_upper = bbmult;
        else
            bbmult_lower = bbmult;

        bbmult_next = (bbmult_upper + bbmult_lower) >> 1;

        /* If BB Mult results in a healthy AM-to-AM power then stop */
        if (ACMAJORREV_129_130(pi->pubpi->phy_rev) && is2g) {
            if (tempclip >= clipthresholdl && tempclip <= clipthresholdu &&
                eps_im < 4095 && eps_re < 4095)
                break;
        } else {
            if (tempclip >= clipthresholdl && tempclip <= clipthresholdu)
                break;
        }
    }
    PHY_PAPD(("wl%d %s: cal_bbmult core %d: %d\n", pi->sh->unit, __FUNCTION__,
        core, bbmult));
    return bbmult;
}

static void
phy_ac_papd_write_tx_gain(phy_info_t *pi, uint8 core, acphy_txgains_t *target_gain,
    uint16 *bbmult, bool zero_rfgain)
{
    uint8 stall_val = 0;
    uint16 curr_gains_0, curr_gains_1, curr_gains_2;
    uint16 txgain1, txgain2, lpf_gain, dac_gain;
    const uint16 rfseq[] = { 0x100, 0x101, 0x102, 0x501 };
    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);

    ACPHY_DISABLE_STALL(pi);

    if (zero_rfgain == FALSE) {
        txgain1  = ((target_gain->ipa & 0x00FF) | ((target_gain->pad  << 8) & 0xFF00));
        txgain2  = ((target_gain->pga & 0x00FF) | ((target_gain->txgm  << 8) & 0xFF00));
        lpf_gain = (target_gain->txlpf & 0xF0) >> 4;
        dac_gain = (target_gain->txlpf & 0x0F) >> 0;

        WRITE_PHYREGCE(pi, RfctrlCoreTXGAIN1, core, txgain1);
        WRITE_PHYREGCE(pi, RfctrlCoreTXGAIN2, core, txgain2);
        WRITE_PHYREGCE(pi, Dac_gain, core, dac_gain);
        MOD_PHYREGCE(pi, RfctrlCoreLpfGain, core, lpf_bq2_gain, lpf_gain);
        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, txgain, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, lpf_bq2_gain, 1);
        /* emulate RFSEQ GAIN CHANGE */
        curr_gains_0 = (target_gain->txlpf & 0xFF) | ((target_gain->ipa << 8) & 0xFF00);
        curr_gains_1 = (target_gain->pad & 0xFF) | ((target_gain->pga << 8) & 0xFF00);
        curr_gains_2 = (target_gain->txgm & 0xff);
    } else {
        /* Only verified on 6715 */
        if (!ACMAJORREV_130(pi->pubpi->phy_rev))
            ASSERT(0);
        curr_gains_0 = 0;
        curr_gains_1 = 0;
        curr_gains_2 = 0;
    }

    if (core == 3) {
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x501, 16,
            &curr_gains_0);
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x504, 16,
            &curr_gains_1);
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x507, 16,
            &curr_gains_2);
    } else {
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]), 16,
            &curr_gains_0);
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]+3), 16,
            &curr_gains_1);
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]+6), 16,
            &curr_gains_2);
    }
    wlc_phy_set_tx_bbmult_acphy(pi, bbmult, core);

    ACPHY_ENABLE_STALL(pi, stall_val);
}
#endif /* PAPD_GAIN_CTRL */

#if defined(WLTEST) || defined(BCMDBG)
static void
wlc_phy_epa_dpd_set_acphy(phy_type_papdcal_ctx_t *ctx, uint8 enab_epa_dpd, bool in_2g_band)
{
    phy_ac_papdcal_info_t *info = (phy_ac_papdcal_info_t *)ctx;
    phy_info_t *pi = info->pi;
    bool turn_papd_on = FALSE;
    bool iovar_in_band;
    uint8 core = 0;

    if (in_2g_band) {
        pi->papdcali->data->epacal2g = enab_epa_dpd;
        turn_papd_on = (pi->papdcali->data->epacal2g == 1);
    } else {
        pi->papdcali->data->epacal5g = enab_epa_dpd;
        turn_papd_on = (pi->papdcali->data->epacal5g == 1);
    }
    iovar_in_band = ((in_2g_band &&
        (CHSPEC_IS2G(pi->radio_chanspec))) ||
        (!in_2g_band && (CHSPEC_ISPHY5G6G(pi->radio_chanspec))));
    if (iovar_in_band) {
        if (!PHY_PAPDEN(pi) && !PHY_IPA(pi) && in_2g_band) {
            if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
                /* WAR for FDIQI when bq_bw = 9, 25 MHz */
                wlc_phy_radio_tiny_lpf_tx_set(pi, 2, 2, 1, 1);
            } else {
                wlc_phy_radio_tiny_lpf_tx_set(pi, 2, 2, 2, 1);
            }
        }
        if (turn_papd_on) {
            wlc_phy_cals_acphy(pi->u.pi_acphy->calmgri, PHY_PERICAL_UNDEF,
                PHY_CAL_SEARCHMODE_RESTART);
        } else {
            MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 0);
        }
    }
}
#endif /* defined(WLTEST) || defined(BCMDBG) */
static void
phy_ac_papd_radio_lpbk_cleanup_acradio(phy_info_t *pi, uint8 core)
{
    uint16 radio_rev_id;
    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

/* CAL PER CORE - FOREACH_ACTV_CORE(pi, phy_stf_get_data(pi->stfi)->phyrxchain, core) */
    {
        radio_rev_id = READ_RADIO_REGC(pi, RF, REV_ID, core);

        MOD_RADIO_REGC(pi, GE16_OVR20, core, ovr_tia_GainI, 0);
        MOD_RADIO_REGC(pi, GE16_OVR20, core, ovr_tia_GainQ, 0);
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            MOD_RADIO_REGC(pi, TXRX2G_CAL_TX, core, i_calPath_pa2g_pu, 0);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_TX, core, i_calPath_pad2g_pu, 0);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_cal_pu, 0);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_papdcal_pu, 0);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_rxiqcal_pu, 0);
            MOD_RADIO_REGC(pi, RXRF2G_CFG2, core, lna2g_epapd_en, 0);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_TX, core, i_cal_pa_atten_2g, 0);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_papdcal_rx_attn, 0);
            MOD_RADIO_REGC(pi, RXRF2G_CFG1, core, pwrsw_en, 1);
/*
            if (radio_rev_id == 0 || radio_rev_id == 1 || radio_rev_id == 2 ||
                radio_rev_id == 3 || radio_rev_id == 4) {
                MOD_RADIO_REGC(pi, OVR18, core, ovr_rxrf2g_pwrsw_en, 1);
            } else {
                MOD_RADIO_REGC(pi, OVR19, core, ovr_rxrf2g_pwrsw_en, 1);
            }
*/

        } else {
            MOD_RADIO_REGC(pi, TXRX5G_CAL_TX, core, i_calPath_pa_pu_5g, 0);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_TX, core, i_calPath_pad_pu_5g, 0);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_cal_pu, 0);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_papdcal_pu, 0);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_rxiqcal_pu, 0);
            MOD_RADIO_REGC(pi, RXRF5G_CFG2, core, lna5g_epapd_en, 0);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_TX, core, i_cal_pa_atten_5g, 0);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_papdcal_rx_attn, 0);

            if (radio_rev_id == 0 || radio_rev_id == 1 || radio_rev_id == 2 ||
                radio_rev_id == 3 || radio_rev_id == 4) {
                MOD_RADIO_REGC(pi, OVR18, core, ovr_rxrf5g_pwrsw_en, 0);
            } else {
                MOD_RADIO_REGC(pi, OVR19, core, ovr_rxrf5g_pwrsw_en, 0);
            }

        }
    }

/*
    ASSERT(porig->is_orig);
    porig->is_orig = FALSE;
*/

    if (ACREV_IS(pi->pubpi->phy_rev, 2)) {
        ACPHY_REG_LIST_START
            MOD_PHYREG_ENTRY(pi, RxFeCtrl1, rxfe_bilge_cnt, 0)
            MOD_PHYREG_ENTRY(pi, RxFeCtrl1, soft_sdfeFifoReset, 1)
            MOD_PHYREG_ENTRY(pi, RxFeCtrl1, soft_sdfeFifoReset, 0)
        ACPHY_REG_LIST_EXECUTE(pi);
    }

}

static void
phy_ac_papd_radio_lpbk_setup_acradio(phy_info_t *pi, uint16 tx_atten, uint16 rx_atten,
 uint8 core)
{
    uint16 radio_rev_id;

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

    /* CAL PER CORE - FOREACH_ACTV_CORE(pi, phy_stf_get_data(pi->stfi)->phyrxchain, core) */
    {
        radio_rev_id = READ_RADIO_REGC(pi, RF, REV_ID, core);

        MOD_RADIO_REGC(pi, GE16_OVR20, core, ovr_tia_GainI, 1);
        MOD_RADIO_REGC(pi, GE16_OVR20, core, ovr_tia_GainQ, 1);
        MOD_RADIO_REGC(pi, TIA_CFG1, core, GainI, 2);
        MOD_RADIO_REGC(pi, TIA_CFG1, core, GainQ, 2);

        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            if (RADIO2069REV(pi->pubpi->radiorev) == 25) {
                MOD_RADIO_REGC(pi, TIA_CFG1, core, GainI, 0);
                MOD_RADIO_REGC(pi, TIA_CFG1, core, GainQ, 0);
            }
            MOD_RADIO_REGC(pi, TXRX2G_CAL_TX, core, i_calPath_pa2g_pu, 1);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_TX, core, i_calPath_pad2g_pu, 0);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_cal_pu, 1);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_papdcal_pu, 1);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_rxiqcal_pu, 0);
            MOD_RADIO_REGC(pi, RXRF2G_CFG2, core, lna2g_epapd_en, 0);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_TX, core, i_cal_pa_atten_2g, tx_atten);
            MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core,
                           loopback2g_papdcal_rx_attn, rx_atten);

            /* Enable the vdd switch on mixer */
            MOD_RADIO_REGC(pi, RXRF2G_CFG1, core, pwrsw_en, 1);
            if (radio_rev_id == 0 || radio_rev_id == 1 || radio_rev_id == 2 ||
                radio_rev_id == 3 || radio_rev_id == 4) {
                MOD_RADIO_REGC(pi, OVR18, core, ovr_rxrf2g_pwrsw_en, 1);
            } else {
                MOD_RADIO_REGC(pi, OVR19, core, ovr_rxrf2g_pwrsw_en, 1);
            }
            if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) && PHY_EPAPD(pi)) {
                /* 4350EPAPD */
                MOD_RADIO_REGC(pi, TIA_CFG1, core, GainI, 4);
                MOD_RADIO_REGC(pi, TIA_CFG1, core, GainQ, 4);
                MOD_RADIO_REGC(pi, TXRX2G_CAL_TX, core, i_calPath_pa2g_pu, 0);
                MOD_RADIO_REGC(pi, TXRX2G_CAL_TX, core, i_calPath_pad2g_pu, 0);
                MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_cal_pu, 0);
                MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_papdcal_pu, 1);
                MOD_RADIO_REGC(pi, TXRX2G_CAL_RX, core, loopback2g_rxiqcal_pu, 0);
                MOD_RADIO_REGC(pi, RXRF2G_CFG2, core, lna2g_epapd_en, 1);
                MOD_RADIO_REGC(pi, RXRF2G_CFG2, core, lna2g_epapd_attn, 0);
            }
        } else {
            if (RADIO2069REV(pi->pubpi->radiorev) == 25) {
                if (CHSPEC_IS80(pi->radio_chanspec)) {
                    MOD_RADIO_REGC(pi, TIA_CFG1, core, GainI, 4);
                    MOD_RADIO_REGC(pi, TIA_CFG1, core, GainQ, 4);
                } else {
                    MOD_RADIO_REGC(pi, TIA_CFG1, core, GainI, 2);
                    MOD_RADIO_REGC(pi, TIA_CFG1, core, GainQ, 2);
                }
            }
            MOD_RADIO_REGC(pi, TXRX5G_CAL_TX, core, i_calPath_pa_pu_5g, 1);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_TX, core, i_calPath_pad_pu_5g, 0);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_cal_pu, 1);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_papdcal_pu, 1);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_rxiqcal_pu, 0);
            MOD_RADIO_REGC(pi, RXRF5G_CFG2, core, lna5g_epapd_en, 0);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_TX, core, i_cal_pa_atten_5g, tx_atten);
            MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core,
                loopback5g_papdcal_rx_attn, rx_atten);
            MOD_RADIO_REGC(pi, RXRF5G_CFG1, core, pwrsw_en, 1);
            if (radio_rev_id == 0 || radio_rev_id == 1 || radio_rev_id == 2 ||
                radio_rev_id == 3 || radio_rev_id == 4) {
                MOD_RADIO_REGC(pi, OVR18, core, ovr_rxrf5g_pwrsw_en, 1);
            } else {
                MOD_RADIO_REGC(pi, OVR19, core, ovr_rxrf5g_pwrsw_en, 1);
            }
            if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) && PHY_EPAPD(pi)) {
                /* 4350EPAPD */
                MOD_RADIO_REGC(pi, TIA_CFG1, core, GainI, 7);
                MOD_RADIO_REGC(pi, TIA_CFG1, core, GainQ, 7);
                MOD_RADIO_REGC(pi, TXRX5G_CAL_TX, core, i_calPath_pa_pu_5g, 0);
                MOD_RADIO_REGC(pi, TXRX5G_CAL_TX, core, i_calPath_pad_pu_5g, 0);
                MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_cal_pu, 0);
                MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_papdcal_pu, 1);
                MOD_RADIO_REGC(pi, TXRX5G_CAL_RX, core, loopback5g_rxiqcal_pu, 0);
                MOD_RADIO_REGC(pi, RXRF5G_CFG2, core, lna5g_epapd_en, 1);
                MOD_RADIO_REGC(pi, RXRF5G_CFG2, core, lna5g_epapd_attn, 0);
            }
        }
        /* #Powerdown LNA1, LNA2 */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna1_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_lna1_pwrup, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna2_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_lna2_pwrup, 0);

        /* acphy_rfctrl_override lpf_sw rxiq_rx2 $core; */
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_dac_adc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_dac_adc, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_aux_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_aux_bq1, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_iqcal_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_iqcal_bq1, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq2_adc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq2_adc, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_dac_rc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_dac_rc, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq1_bq2, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq1_bq2, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq1_adc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq1_adc, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_tia_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_tia_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq2_rc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq2_rc, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_dac_bq2, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_dac_bq2, 1);

        /* acphy_rfctrl_override lpf_pu_dc 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, lpf_pu_dc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, lpf_pu_dc, 1);

        /* acphy_rfctrl_override tia_DC_loop_PU 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, tia_DC_loop_PU, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, tia_DC_loop_PU, 1);

        /* acphy_rfctrl_override fast_nap_bias_pu 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, fast_nap_bias_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, fast_nap_bias_pu, 1);

        /* acphy_rfctrl_override rxrf_pwrup 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_pwrup, 1);

        /* acphy_rfctrl_override logen_rx_pwrup 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, logen_rx_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, logen_rx_pwrup, 1);
    }
}

static void
phy_ac_get_tx_gain(phy_info_t *pi, uint8 core_no, acphy_txgains_t *target_gain)
{
    uint16 curr_gains_0 = 0, curr_gains_1 = 0, curr_gains_2 = 0;
    const uint16 rfseq[] = { 0x100, 0x101, 0x102, 0x501 };
    uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);

    if (pi->acphy_txpwrctrl == PHY_TPC_HW_OFF) {
        /* read current tx gain from RFSeq table and use as target_gain */

        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core_no]), 16,
            &curr_gains_0);
        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core_no]+3), 16,
            &curr_gains_1);
        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core_no]+6), 16,
            &curr_gains_2);

        /* extract gain values */
        target_gain->txlpf = (uint16) ((curr_gains_0 & 0xFF));
        target_gain->ipa  = (uint16) ((curr_gains_0 & 0xFF00) >> 8);
        target_gain->pad  = (uint16) ((curr_gains_1 & 0xFF)   >> 0);
        target_gain->pga  = (uint16) ((curr_gains_1 & 0xFF00) >> 8);
        target_gain->txgm = (uint16) ((curr_gains_2 & 0xFF)   >> 0);
    }

    ACPHY_ENABLE_STALL(pi, stall_val);
}

static void
phy_ac_papd_radio_lpbk_setup_tiny(phy_info_t *pi, uint16 tx_atten, uint16 rx_atten)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_radioregs_t *porig = (pi_ac->papdcali->papd_radioregs);
    uint8 core;

    ASSERT(TINY_RADIO(pi));

    FOREACH_CORE(pi, core) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* PAPD loopback in g-band */

            MOD_RADIO_REG_TINY(pi, RX_TOP_2G_OVR_EAST, core,
                ovr_gm2g_pwrup, 1);
            MOD_RADIO_REG_TINY(pi, RX_TOP_2G_OVR_EAST, core,
                    ovr_gm2g_auxgm_pwrup, 1);

            /* AUXLNA2 Power Up */
            MOD_RADIO_REG_TINY(pi, LNA2G_CFG2, core, gm2g_auxgm_pwrup, 1);
            if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                /* LNA2 Power Up */
                MOD_RADIO_REG_20693(pi, LNA2G_CFG2, core, gm2g_pwrup, 0);
                MOD_RADIO_REG_TINY(pi, LNA2G_CFG2, core, gm2g_auxgm_pwrup, 1);

            } else {
                /* LNA2 Power Down */
                MOD_RADIO_REG_TINY(pi, LNA2G_CFG2, core, gm2g_pwrup, 0);
                MOD_RADIO_REG_TINY(pi, RX_TOP_2G_OVR_EAST, core,
                    ovr_gm2g_auxgm_pwrup, 1);
            }

            if (!PHY_EPAPD(pi)) {
                /* Enable ipapd */
                MOD_RADIO_REG_TINY(pi, TX2G_MISC_CFG1, core, cal2g_pa_pu, 1);
                MOD_RADIO_REG_TINY(pi, RXRF2G_CFG2, core,
                    loopback2g_papdcal_pu, 1);
                MOD_RADIO_REG_TINY(pi, RXRF2G_CFG2, core, lna2g_epapd_en, 0);
                MOD_RADIO_REG_TINY(pi, TX2G_MISC_CFG1, core,
                    cal2g_pa_atten, tx_atten);
                MOD_RADIO_REG_TINY(pi, RXRF2G_CFG2, core,
                    rf2g_papdcal_rx_attn, rx_atten);
                if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                    RADIO_REG_LIST_START
                        MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_2G_OVR_EAST,
                            core, ovr_lna2g_lna2_gain, 1)
                        MOD_RADIO_REG_20693_ENTRY(pi, LNA2G_CFG2, core,
                            lna2g_lna2_gain, 0)
                        /* LNA1 Kill switch */
                        MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_2G_OVR_EAST2,
                            core, ovr_lna2g_tr_rx_en, 1)
                        MOD_RADIO_REG_20693_ENTRY(pi, LNA2G_CFG1, core,
                            lna2g_tr_rx_en, 0)
                        MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_2G_OVR_EAST2,
                            core, ovr_lna2g_lna1_Rout, 1)
                        MOD_RADIO_REG_20693_ENTRY(pi, LNA2G_CFG1, core,
                            lna2g_lna1_Rout, 0xb)
                        MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_2G_OVR_EAST2,
                            core, ovr_lna2g_lna1_out_short_pu, 1)
                        MOD_RADIO_REG_20693_ENTRY(pi, LNA2G_CFG1, core,
                            lna2g_lna1_out_short_pu, 1)
                        MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_2G_OVR_EAST2,
                            core, ovr_lna2g_lna1_pu, 0)
                    RADIO_REG_LIST_EXECUTE(pi, core);
                }
            } else {
                /* Enable epapd */
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_TINY_ENTRY(pi, TX2G_MISC_CFG1, core,
                        cal2g_pa_pu, 0)
                    MOD_RADIO_REG_TINY_ENTRY(pi, RXRF2G_CFG2, core,
                        loopback2g_papdcal_pu, 0)
                    MOD_RADIO_REG_TINY_ENTRY(pi, RXRF2G_CFG2, core,
                        lna2g_epapd_en, 1)
                    MOD_RADIO_REG_TINY_ENTRY(pi, LNA2G_CFG1, core,
                        lna2g_lna1_bypass, 1)
                    MOD_RADIO_REG_TINY_ENTRY(pi, RX_TOP_2G_OVR_NORTH, core,
                        ovr_lna2g_lna1_bypass, 1)
                RADIO_REG_LIST_EXECUTE(pi, core);
                MOD_RADIO_REG_TINY(pi, RXRF2G_CFG2, core,
                    lna2g_epapd_attn, rx_atten);
            }

            /* #Powerdown 2G LNA1 */
            MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna1_pwrup, 1);
            MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_lna1_pwrup, 0);
        } else {
            /* PAPD loopback in a-band */

            MOD_RADIO_REG_TINY(pi, RX_TOP_5G_OVR, core, ovr_gm5g_pwrup, 1);

            /* AUXLNA2 Power Up */
            if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna1_5G_pwrup, 1);
                MOD_PHYREGCE(pi, RfctrlCoreRxPus, core,    rxrf_lna1_5G_pwrup, 0);
                MOD_RADIO_REG_20693(pi, LNA5G_CFG2, core, lna5g_pu_auxlna2, 0);
                MOD_RADIO_REG_20693(pi, SPARE_CFG16, core, loopback5g_gm5g_pu, 1);
            } else {
                MOD_RADIO_REG_TINY(pi, LNA5G_CFG2, core, lna5g_pu_auxlna2, 1);
            }

            /* power down rxgm5g */
            MOD_RADIO_REG_TINY(pi, LNA5G_CFG2, core, lna5g_pu_lna2, 0);

            if (!RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                /* powerup auxgm5g */
                MOD_RADIO_REG_TINY(pi, RX_TOP_5G_OVR, core,
                    ovr_lna5g_pu_auxlna2, 1);
            }

            if (!PHY_EPAPD(pi)) {
                /* Enable ipapd */
                MOD_RADIO_REG_TINY(pi, TX5G_MISC_CFG1, core, cal5g_pa_pu, 1);
                MOD_RADIO_REG_TINY(pi, TXRX5G_CAL_RX, core, loopback5g_cal_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core, rf5g_epapd_en, 0);
                MOD_RADIO_REG_TINY(pi, TX5G_MISC_CFG1, core,
                                    cal5g_pa_atten, tx_atten);
                MOD_RADIO_REG_TINY(pi, RXRF5G_CFG2, core,
                                    loopback5g_papdcel_rx_attn, rx_atten);
                if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                    MOD_RADIO_REG_20693(pi, LNA5G_CFG1, core,
                        lna5g_lna2_gain, 0);
                    MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR2, core,
                                        ovr_lna5g_lna2_gain, 1);
                    MOD_RADIO_REG_TINY(pi, RX_TOP_5G_OVR, core,
                                        ovr_lna5g_tr_rx_en, 1);
                    if (ROUTER_4349(pi)) {
                        MOD_RADIO_REG_TINY(pi, LNA5G_CFG1, core,
                            lna5g_tr_rx_en, 0);
                    } else {
                        MOD_RADIO_REG_TINY(pi, LNA5G_CFG1, core,
                            lna5g_tr_rx_en, 1);
                    }
                }
            } else {
                RADIO_REG_LIST_START
                    /* Enable  */
                    MOD_RADIO_REG_TINY_ENTRY(pi, TX5G_MISC_CFG1, core,
                        cal5g_pa_pu, 0)
                    MOD_RADIO_REG_TINY_ENTRY(pi, TXRX5G_CAL_RX, core,
                        loopback5g_cal_pu, 0)
                    MOD_RADIO_REG_TINY_ENTRY(pi, PA5G_CFG1, core,
                        rf5g_epapd_en, 1)
                    MOD_RADIO_REG_TINY_ENTRY(pi, LNA5G_CFG1, core,
                        lna5g_lna1_bypass, 1)
                    MOD_RADIO_REG_TINY_ENTRY(pi, RX_TOP_5G_OVR, core,
                        ovr_lna5g_lna1_bypass, 1)
                RADIO_REG_LIST_EXECUTE(pi, core);
                MOD_RADIO_REG_TINY(pi, RXRF5G_SPARE, core,
                                    rf5g_epapd_attn, rx_atten);
            }

            /* #Powerdown 5G LNA1 */
            MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna1_5G_pwrup, 1);
            MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_lna1_5G_pwrup, 0);
        }

        /* PHY register writes */
        /* Farrow */
        MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
        if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
            MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 1);
            MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);
        } else {
            MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0,
                (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) ? 0 : 2);
        }

        /* #Powerup LNA2 for 20693 & power up otherwise */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_lna2_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_lna2_pwrup,
            (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) ? 1 : 0);
        /* Configure the LPF switches and powerup the DC loop */
        /* MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, 0x3ff, 0x3ff);    */
        /* MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, 0x3ff, 0x151);        */
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq2_adc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq2_adc, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq1_adc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq1_adc, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_dac_adc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_dac_adc, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq2_rc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq2_rc, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_dac_rc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_dac_rc, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq1_bq2, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq1_bq2, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_dac_bq2, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_dac_bq2, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_aux_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_aux_bq1, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_iqcal_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_iqcal_bq1, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_tia_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_tia_bq1, 0);

        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_dac_bq2, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_dac_bq2, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq2_rc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq2_rc, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_tia_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_tia_bq1, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, lpf_sw_bq1_adc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, lpf_sw_bq1_adc, 1);

        /* acphy_rfctrl_override lpf_pu_dc 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, lpf_pu_dc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, lpf_pu_dc, 1);

        /* acphy_rfctrl_override tia_DC_loop_PU 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, tia_DC_loop_PU, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, tia_DC_loop_PU, 1);

        /* acphy_rfctrl_override fast_nap_bias_pu 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, fast_nap_bias_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, fast_nap_bias_pu, 0);

        /* acphy_rfctrl_override rxrf_pwrup 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_pwrup, 1);

        /* acphy_rfctrl_override logen_rx_pwrup 1 $core */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, logen_rx_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, logen_rx_pwrup, 1);

        MOD_RADIO_REG_TINY(pi, CLK_DIV_CFG1, core, dac_driver_size, 8);
        porig->adc_cfg10[core] = READ_RADIO_REG_TINY(pi, ADC_CFG10, core);
        porig->adc_ovr1[core] = READ_RADIO_REG_TINY(pi, ADC_OVR1, core);
        if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
            MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_od_pu, 1);
            MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_pu,
                ((CHSPEC_IS20(pi->radio_chanspec)) ||
                (CHSPEC_IS40(pi->radio_chanspec))) ? 0 : 1);
        } else {
            MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_od_pu, 0);
        }
        MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_in_test, 1);
        MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_in_test, 0x0);
        /* Setting TIA gain */
        if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID))  {
            if (HW_RADIOREV(pi->pubpi->radiorev) == 5)  {
                phy_ac_papd_set_tia_gain_tiny(pi, (CHSPEC_IS2G(pi->radio_chanspec))
                    ?((phy_get_phymode(pi) == PHYMODE_RSDB) ? 0x5 : 0x6)
                        : 0x0, core);
            } else  {
                phy_ac_papd_set_tia_gain_tiny(pi, (CHSPEC_IS2G(pi->radio_chanspec))
                    ? 0x3 : 0x0, core);
            }
        } else
            phy_ac_papd_set_tia_gain_tiny(pi, (CHSPEC_IS2G(pi->radio_chanspec))
                ? 0x3 : 0x0, core);

    }
}

static void
phy_ac_papd_radio_lpbk_setup_20704(phy_info_t *pi, bool epapd_flag)
{
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)pi->u.pi_acphy->papdcali;
    phy_ac_papdcal_params_t *params    = papdcali->papd_params;
    phy_ac_papdcal_phyregs_t *porig = papdcali->papd_phyregs;

    uint8 core, papd_loopback_mode, papd_bypass_mode;
    uint8 tia_gain = params->tia_mode_2g;
    uint8 rx_atten = params->rx_atten_2g;

    /* Specific cache for Radio Loopback PHY regs that are not per core */
    porig->RxSdFeConfig1 = READ_PHYREG(pi, RxSdFeConfig1);
    porig->RxSdFeConfig6 = READ_PHYREG(pi, RxSdFeConfig6);

    /* Initially save the registers */
    phy_ac_reg_cache_save(aci, RADIOREGS_PAPDCAL);

    if ((RADIOREV(pi->pubpi->radiorev) < 4) &&
        ((READ_RADIO_PLLREGFLD_20704(pi, PLL_CFGR2, 0, rfpll_spare0_out) & 0x2) == 0)) {
        /* MAINGM with bypass */
        papd_loopback_mode = MAINGM_PATH;
        papd_bypass_mode   = 1;
    } else {
        /* Config from NVRAM */
        papd_loopback_mode = params->papd_loopback_mode;
        papd_bypass_mode   = params->papd_bypass_mode;
    }

    FOREACH_CORE(pi, core) {
        /* configuring radio registers per core */
        RADIO_REG_LIST_START
            /* ALPF (BQ2) is in Tx path, as in regular Tx mode */
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR2,    core, ovr_lpf_sw_bq1_bq2,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,        0)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc,        0)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc,        1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,        1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,        1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,        0)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc,        0)
            /* TIA power up sequence */
            MOD_RADIO_REG_20704_ENTRY(pi, TIA_CFG1_OVR,    core, ovr_tia_pu,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG7,        core, tia_pu,        1)
            MOD_RADIO_REG_20704_ENTRY(pi, TIA_CFG1_OVR,    core, ovr_tia_bias_pu,    1)
            MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG7,        core, tia_bias_pu,    1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* PAPD CAL 2G is using PAPD path and aux GM */
            RADIO_REG_LIST_START
                MOD_RADIO_REG_20704_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_out_short,    1)
                MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_out_short, 0)
                MOD_RADIO_REG_20704_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_pu, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_pu, 0)
                MOD_RADIO_REG_20704_ENTRY(pi, RX2G_CFG1_OVR,    core,
                    ovr_rxdb_lna2g_bypass, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG2,    core,
                    rxdb_lna2g_bypass, 0)
                MOD_RADIO_REG_20704_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rx2g_lna_tr_rx_en, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, RX2G_REG4,    core,
                    rx2g_lna_tr_rx_en, 0)
                MOD_RADIO_REG_20704_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_rout, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_rout, 0)

                /* Rx mixer, Rx rccr and rx div2 buf power up/down */
                MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG5,    core,
                    rxdb_mix_pu, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_mix_pu, 1)

                MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0,    core,
                    ovr_logen_div2_rxbuf_pu, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0,    core,
                    logen_2g_div2_rx_pu, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0,    core,
                    logen_rx_rccr_pu, 0)
                MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0,    core,
                    ovr_logen_rx_rccr_pu, 1)

                MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG2, core,
                    rxdb_lna_auxpath_en, 0)
                MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_pu, 0)
                MOD_RADIO_REG_20704_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_gm_pu, 1)
            RADIO_REG_LIST_EXECUTE(pi, core);

            if (papd_loopback_mode == MAINGM_PATH) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 0)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 0)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 0)
                    MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0, core,
                        tx2g_pa_cal_pu, 0)
                RADIO_REG_LIST_EXECUTE(pi, core);

                MOD_RADIO_REG_20704(pi, RX5G_REG4, core,
                    rxdb_gm_bypass, papd_bypass_mode);
                MOD_RADIO_REG_20704(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_gm_bypass, 1);
            } else {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_bypass, 0)
                    MOD_RADIO_REG_20704_ENTRY(pi, RXDB_CFG1_OVR,    core,
                        ovr_rxdb_gm_bypass, 1)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 0)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 1)
                    MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0, core,
                        tx2g_pa_cal_pu, 1)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 1)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG5, core,
                        rxdb_mix_Cin_tune, 2)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_cc, 6)
                RADIO_REG_LIST_EXECUTE(pi, core);

                if (papd_bypass_mode == 0) {
                    MOD_RADIO_REG_20704(pi, RX5G_REG6, core,
                        rxdb_spare, 0x10);
                    tia_gain = 3;

                } else {
                    MOD_RADIO_REG_20704(pi, RX5G_REG6, core,
                        rxdb_spare, 0x20);
                    rx_atten = 0;
                    tia_gain = 7;
                }
            }
            /* Set TX and RX coupler attenuation */
            MOD_RADIO_REG_20704(pi, TX2G_PA_REG0, core, tx2g_pa_cal_atten,
                params->tx_atten_2g);
            MOD_RADIO_REG_20704(pi, RX5G_REG4, core, rxdb_coup_loopback_attn,
                rx_atten);

            if (!epapd_flag) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG6, core,
                        rxdb_lna2g_epapd_en, 0)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG5, core,
                        rxdb_lna5g_epapd_en, 0)
                RADIO_REG_LIST_EXECUTE(pi, core);
            } else {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG6, core,
                        rxdb_lna2g_epapd_en, 1)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG5, core,
                        rxdb_lna5g_epapd_en, 0)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG5, core,
                        rxdb_lna_epapd_attn, 3)
                    MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 0)
                    MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0, core,
                        tx2g_pa_cal_pu, 0)
                RADIO_REG_LIST_EXECUTE(pi, core);
            }
        } else {
            PHY_ERROR(("wl%d: %s: 5GHz PAPD not supported\n", pi->sh->unit,
                __FUNCTION__));
        }

        RADIO_REG_LIST_START
            MOD_RADIO_REG_20704_ENTRY(pi, RXADC_CFG0, core, rxadc_puI, 1)
            MOD_RADIO_REG_20704_ENTRY(pi, RXADC_CFG0, core, rxadc_puQ, 1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        /* Set TIA gain */
        WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, 0);
        MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain, tia_gain);
        /* Watch out, OVR below is common for lna1/2 gain, mixtia and dvga
         * hence zero lna gains via phyregs too
         */
        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain, 1);

        /* Set TIA bandwidth - TCL SVN 802152 */
        if (CHSPEC_IS40(pi->radio_chanspec)) {
            MOD_PHYREGCE(pi, RfctrlCoreLpfCT, core, lpf_bq1_bw, 2);
            MOD_PHYREGCE(pi, RfctrlOverrideLpfCT, core, lpf_bq1_bw, 1);
        }

        if (WBPAPD_ENAB(pi)) {
            RADIO_REG_LIST_START
                /* Widen TX filter */
                /* TIA */
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g11, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g12, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g21, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g22, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_c11, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG1, core, tia_g11, 0x0)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG2, core, tia_g12, 0x10a)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG3, core, tia_g21, 0x1765)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG4, core, tia_g22, 0x760)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG6, core, tia_c11, 0x7)
                /* LPF */
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g32, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g33, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g34, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g43, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_c42, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG1, core, lpf_g32, 0xfae)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG2, core, lpf_g33, 0xb16)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG3, core, lpf_g34, 0x18e4)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG4, core, lpf_g43, 0x277)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_c42, 0xb)
                /* Bias */
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG5, core, tia_opamp1_bias, 0x28)
                MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG5, core, tia_opamp2_bias, 0x19)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_opamp3_bias,
                    1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_opamp4_bias,
                    1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG11, core, lpf_opamp3_bias,
                    0x14)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG6, core, lpf_opamp4_bias, 0x14)

                /* Tighten TX notch */
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g1, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g1_cm, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g2, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g3, 1)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_NOTCH_REG2, core,
                    lpf_notch_g1, 0x159)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_NOTCH_REG5, core,
                    lpf_notch_g1_cm, 0x0)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_NOTCH_REG3, core,
                    lpf_notch_g2, 0x159)
                MOD_RADIO_REG_20704_ENTRY(pi, LPF_NOTCH_REG4, core,
                    lpf_notch_g3, 0x55)
            RADIO_REG_LIST_EXECUTE(pi, core);
        }
    }

    MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
    MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);

}

static void
phy_ac_papd_radio_lpbk_setup_20707(phy_info_t *pi)
{
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)pi->u.pi_acphy->papdcali;
    phy_ac_papdcal_params_t *params = papdcali->papd_params;
    uint8 core, radio_rev_id, rxdb_spare_auxgm, rxdb_spare_maingm;

    /* Initially save the registers */
    phy_ac_reg_cache_save(aci, RADIOREGS_PAPDCAL);

    FOREACH_CORE(pi, core) {
        radio_rev_id = READ_RADIO_REGC(pi, RF, REV_ID, core);
        /* From 6710A1 there is a separate AuxGm PU and bypass in rx5g_reg6.rxdb_spare
         * To use auxgm loopback as in A0 (non-bypass), rxdb_spare[4] = 1 (auxgm_en),
         * rxdb_spare[5] = 0 (auxgm_bypass). To enable bypass mode gm must be PD,
         * therefore: rxdb_spare[4] = 0 (auxgm_en), rxdb_spare[5] = 1 (auxgm_bypass)
         * Also, Here are PAD loopback switces
         */
        rxdb_spare_maingm = 0;
        rxdb_spare_auxgm = (radio_rev_id == 0)? 0 : 16;
        /* configuring radio registers per core */
        RADIO_REG_LIST_START
            /* ALPF (BQ2) is in Tx path, as in regular Tx mode */
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,           0)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,       1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc,           0)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,       1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc,           1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,       1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,           1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,       1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,            1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,        1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,            0)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,        1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc,           0)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,       1)
            /* TIA power up sequence */
            MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG7,         core, tia_pu,           1)
            MOD_RADIO_REG_20707_ENTRY(pi, TIA_CFG1_OVR,     core, ovr_tia_pu,       1)
            MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG7,         core, tia_bias_pu,      1)
            MOD_RADIO_REG_20707_ENTRY(pi, TIA_CFG1_OVR,     core, ovr_tia_bias_pu,  1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG6,         core, lpf_bq_pu,        1)
            MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1,         core, ovr_lpf_bq_pu,    1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* Routing into AuxGM, so LNA settings below are redundand, but preferably
             * keep the LNA in PD and NOT bypassed/shorted.
             */
            RADIO_REG_LIST_START
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1,        core,
                    rxdb_lna_out_short, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_out_short, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1,        core,
                    rxdb_lna_pu, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_pu, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG2,        core,
                    rxdb_lna2g_bypass, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RX2G_CFG1_OVR,    core,
                    ovr_rxdb_lna2g_bypass, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RX2G_REG4,        core,
                    rx2g_lna_tr_rx_en, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rx2g_lna_tr_rx_en, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1,        core,
                    rxdb_lna_rout, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_rout, 1)

                /* Rx mixer, Rx rccr and rx div2 buf power up/down */
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5,        core,
                    rxdb_mix_pu, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_mix_pu, 1)

                /* Starting from Tx mode div2 and txbuf will be on
                   need to turn on the rxbuf.
                 */
                MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG5,  core,
                    logen_div2_rxbuf_pu, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0,  core,
                    ovr_logen_div2_rxbuf_pu, 1)

                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_REG6,  core,
                    lpf_notch_sel_2g_out_gm, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1,  core,
                    ovr_lpf_notch_sel_2g_out_gm, 1)
            RADIO_REG_LIST_EXECUTE(pi, core);

            if (1) {
                /* AuxGM Section */
                /* PAPD CAL 2G is using PAPD path and aux GM. */
                RADIO_REG_LIST_START
                    /* AuxGm does not need mainGM gm_pu, but also gm_bypass
                     * needs to be off.
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_bypass, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_bypass, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_pu, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_pu, 1)

                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 0x1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 0x0)

                    MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0, core,
                        tx2g_ipa_cal_pu, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0, core,
                        tx5g_ipa_cal_pu, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_5g_en, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG2, core,
                        rxdb_lna_auxpath_en, 0)
                    /* AuxGM and AuxGM loading */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_cc, 15)
                    /* from 63178 corners */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_ibias_ptat_slope, 4)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_mix_Cin_tune, 2)
                RADIO_REG_LIST_EXECUTE(pi, core);
                /* From 6710A1 there is a separate AuxGm PU and bypass in
                 * rx5g_reg6.rxdb_spare. To use auxgm loopback as in A0
                 * (non-bypass), rxdb_spare[4] = 1 (auxgm_en),
                 * rxdb_spare[5] = 0 (auxgm_bypass). To enable bypass
                 * mode gm must be PD,
                 * therefore: rxdb_spare[4] = 0 (auxgm_en),
                 * rxdb_spare[5] = 1 (auxgm_bypass)
                 * Also, Here are PAD loopback switces
                 */
                MOD_RADIO_REG_20707(pi, RX5G_REG6, core,
                    rxdb_spare, rxdb_spare_auxgm);
            } else {
                /* MainGM Section. Toggle with previous for when the MainGM path
                 * is preferred.
                 */

                /* PAPD CAL 2G is using PAPD path and MAIN GM */
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_pu, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_pu, 1)

                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 1)

                    /* Tuning Elements at the input of the MainGM.
                     * Therefore they need tweaking.
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_rout, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_lna_rout, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_tune, 11)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_sel2g5g_loadind, 0)

                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_bypass, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_bypass, 1)

                    /* Trim for desired Loopback Gain; unless we opt to use
                     * gm_bypass. Then Re-iterate attenuator values
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_gm_gc, 2)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_gc, 1)

                    /* Routing & Attenuator Settings
                     * Main GM Path couples through the LNA Load inductor.
                     */
                    /* Should be 1, but even if this turned off, significant
                     * signal due to iPA <-> RX coupling
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0, core,
                        tx2g_ipa_cal_pu, 0)
                    /* Should be 1, but even if this turned off, significant
                     * signal due to iPA <-> RX coupling
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 0)
                    /* 0 for 2G path */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_5g_en, 0)

                    /* MainGM and MainGM loading, to investigate for 2G/5G */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_cc, 15)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_mix_Cin_tune, 2)
                RADIO_REG_LIST_EXECUTE(pi, core);
                /* From 6710A1 there is a separate AuxGm PU and bypass
                 * in rx5g_reg6.rxdb_spare. To use auxgm loopback as
                 * in A0 (non-bypass), rxdb_spare[4] = 1 (auxgm_en),
                 * rxdb_spare[5] = 0 (auxgm_bypass)
                 * To enable bypass mode gm must be PD, therefore:
                 * rxdb_spare[4] = 0 (auxgm_en),
                 * rxdb_spare[5] = 1 (auxgm_bypass)
                 * In spare register are PAD loopback switches also.
                 */

                MOD_RADIO_REG_20707(pi, RX5G_REG6, core,
                    rxdb_spare, rxdb_spare_maingm);
            }

            /* Set TX and RX coupler attenuation */
            MOD_RADIO_REG_20707(pi, TX2G_IPA_REG0, core, tx2g_ipa_cal_atten,
                params->tx_atten_2g);
            MOD_RADIO_REG_20707(pi, RX5G_REG4, core, rxdb_coup_loopback_attn,
                params->rx_atten_2g);
        } else {
            RADIO_REG_LIST_START
                /* Routing into AuxGM, so LNA settings below are redundand,
                 * but preferably keep the LNA in PD and NOT bypassed/shorted.
                 */
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                    rxdb_lna_out_short, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_lna_out_short, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                    rxdb_lna_pu, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_lna_pu, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG2, core,
                    rxdb_lna2g_bypass, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RX2G_CFG1_OVR, core,
                    ovr_rxdb_lna2g_bypass, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RX2G_REG4, core,
                    rx2g_lna_tr_rx_en, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rx2g_lna_tr_rx_en, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                    rxdb_lna_rout, 0)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_lna_rout, 1)

                /* Rx mixer, Rx rccr and rx div2 buf power up/down */
                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                    rxdb_mix_pu, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_mix_pu, 1)

                MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG0,  core,
                    logen_rx_rccr_pu, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0,  core,
                    ovr_logen_rx_rccr_pu, 1)

                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_REG6,  core,
                    lpf_notch_sel_5g_out_gm, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1,  core,
                    ovr_lpf_notch_sel_5g_out_gm, 1)

                MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG2, core,
                    rxdb_lna_auxpath_en, 0)
            RADIO_REG_LIST_EXECUTE(pi, core);

            if (0) {
                /* AuxGM Section */

                /* PAPD CAL 5G is using PAPD path and aux GM */
                RADIO_REG_LIST_START
                    /* AuxGm does not need mainGM gm_pu, but also gm_bypass
                     * needs to be off.
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_bypass, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_bypass, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_pu, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_pu, 1)

                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 1)

                    MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0, core,
                        tx5g_ipa_cal_pu, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_5g_en, 1)
                    /* AuxGM biasing and AuxGM loading */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_cc, 15)
                    /* from 63178 corners */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_ibias_ptat_slope, 4)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_mix_Cin_tune, 2)
                RADIO_REG_LIST_EXECUTE(pi, core);
                /* From 6710A1 there is a separate AuxGm PU and bypass
                 * in rx5g_reg6.rxdb_spare. To use auxgm loopback as
                 * in A0 (non-bypass), rxdb_spare[4] = 1 (auxgm_en),
                 * rxdb_spare[5] = 0 (auxgm_bypass)
                 * To enable bypass mode gm must be PD, therefore:
                 * rxdb_spare[4] = 0 (auxgm_en),
                 * rxdb_spare[5] = 1 (auxgm_bypass)
                 * Also, Here are PAD loopback switces
                 */
                MOD_RADIO_REG_20707(pi, RX5G_REG6, core,
                    rxdb_spare, rxdb_spare_auxgm);
            } else if (1) {
                if (radio_rev_id == 0) {
                    MOD_RADIO_REG_20707(pi, TX2G_MISC_CFG6, core,
                        tx2g_spare0, 0);
                    MOD_RADIO_REG_20707(pi, RX5G_REG6, core,
                        rxdb_spare, 0);
                    MOD_RADIO_REG_20707(pi, RX2G_REG4, core,
                        rx2g_lna_tr_rx_en, 0);
                    MOD_RADIO_REG_20707(pi, RXDB_CFG1_OVR, core,
                        ovr_rx2g_lna_tr_rx_en, 1);
                    MOD_RADIO_REG_20707(pi, RX5G_REG1, core,
                        rx5g_lna_tr_rx_en, 0);
                    MOD_RADIO_REG_20707(pi, RXDB_CFG1_OVR, core,
                        ovr_rx5g_lna_tr_rx_en, 1);
                } else {
                    MOD_RADIO_REG_20707(pi, TX2G_MISC_CFG6, core,
                        tx2g_spare0, 2);
                    MOD_RADIO_REG_20707(pi, RX5G_REG6, core,
                        rxdb_spare, 16);
                    MOD_RADIO_REG_20707(pi, RX2G_REG4, core,
                        rx2g_lna_tr_rx_en, 1);
                    /* Now the trsw are not bound with logic with epapd_en,
                     * so they can be (not) used if desired
                     */
                    MOD_RADIO_REG_20707(pi, RXDB_CFG1_OVR, core,
                        ovr_rx2g_lna_tr_rx_en, 1);
                    MOD_RADIO_REG_20707(pi, RX5G_REG1, core,
                        rx5g_lna_tr_rx_en, 1);
                    MOD_RADIO_REG_20707(pi, RXDB_CFG1_OVR, core,
                        ovr_rx5g_lna_tr_rx_en, 1);
                }
                RADIO_REG_LIST_START
                    /* PAPD CAL 5G is using ePAPD paths and AuxGM
                     * (LNA5G in bypass to avoid nonlinear capacitance)"
                     */
                    /* Bypass 5G LNA in ePAPD, 2G bypass is
                     * not very effective
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
                        rxdb_lna2g_epapd_en, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_lna5g_epapd_en, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_lna_epapd_attn, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_pu, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_lna_pu, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_gc, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_lna_gc, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
                        rxdb_lna2g_bias_en, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
                        rxdb_lna5g_bias_en, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG2, core,
                        rxdb_lna2g_bypass, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX2G_CFG1_OVR, core,
                        ovr_rxdb_lna2g_bypass, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna5g_bypass, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_lna5g_bypass, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG2, core,
                        rxdb_lna_auxpath_en, 0)
                    /* Other settings around LNA that may impact
                     * since the LNA bypass sems to be required
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_out_short, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_lna_out_short, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_rout, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_lna_rout, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_tune, 15)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_sel2g5g_loadind, 1)
                    /* Common RXDB */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_bypass, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_bypass, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_pu, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_pu, 1)
                    /* From 6710A1 there is a separate AuxGm PU and
                     * bypass in rx5g_reg6, core, rxdb_spare.
                     * To use auxgm loopback as in A0 (non-bypass),
                     * rxdb_spare[4] = 1 (auxgm_en),
                     * rxdb_spare[5] = 0 (auxgm_bypass),
                     * i.e. spare=16
                     * To enable bypass mode gm must be PD, therefore:
                     * rxdb_spare[4] = 0 (auxgm_en),
                     * rxdb_spare[5] = 1 (auxgm_bypass),
                     * i.e. spare=32
                     * In spare register are PAD loopback switches also
                     * [0:1]=PAD atten, [2]=2G PAD EN, [3]=5G PAD EN
                     */

                    /* Selecting the AuxGM */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 0)
                    /* Routing & Attenuator Settings */
                    MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0, core,
                        tx2g_ipa_cal_pu, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0, core,
                        tx5g_ipa_cal_pu, 0)
                    /* AuxGM biasing and AuxGM loading */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_5g_en, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_cc, 15)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_ibias_ptat_slope, 4)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_mix_Cin_tune, 2)
                RADIO_REG_LIST_EXECUTE(pi, core);
            } else {
                /* MainGM Section. Toggle with previous for when the MainGM path
                 * is preferred.
                 */

                /* PAPD CAL 5G is using PAPD path and MAIN GM */
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_pu, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_pu, 1)

                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 1)

                    /* Tuning Elements at the input of the MainGM.
                     * Therefore they need tweaking.
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_rout, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_lna_rout, 1)
                    /* For 5G band use low vals */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG1, core,
                        rxdb_lna_tune, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_sel2g5g_loadind, 1)

                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_bypass, 0)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_bypass, 1)

                    /* Trim for desired Loopback Gain; unless we opt to use
                     * gm_bypass. Then Re-iterate attenuator values
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_gm_gc, 1)
                    MOD_RADIO_REG_20707_ENTRY(pi, RXDB_CFG1_OVR, core,
                        ovr_rxdb_gm_gc, 1)

                    /* Routing & Attenuator Settings */

                    /* Should be 1, but even if this turned off, significant
                     * signal due to iPA <-> RX coupling
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0, core,
                        tx5g_ipa_cal_pu, 0)
                    /* Should be 1, but even if this turned off, significant
                     * signal due to iPA <-> RX coupling
                     */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_5g_en, 0)
                    /* 0 for 5G path */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 0)
                    /* MainGM and MainGM loading, to investigate for 2G/5G */
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_cc, 15)
                    MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
                        rxdb_mix_Cin_tune, 1)
                RADIO_REG_LIST_EXECUTE(pi, core);
                /* From 6710A1 there is a separate AuxGm PU and bypass
                 * in rx5g_reg6.rxdb_spare. To use auxgm loopback as
                 * in A0 (non-bypass), rxdb_spare[4] = 1 (auxgm_en),
                 * rxdb_spare[5] = 0 (auxgm_bypass)
                 * To enable bypass mode gm must be PD, therefore:
                 * rxdb_spare[4] = 0 (auxgm_en),
                 * rxdb_spare[5] = 1 (auxgm_bypass)
                 * In spare register are PAD loopback switches also.
                 */
                MOD_RADIO_REG_20707(pi, RX5G_REG6, core,
                    rxdb_spare, rxdb_spare_maingm);
            }

            /* Set TX and RX coupler attenuation */
            MOD_RADIO_REG_20707(pi, TX5G_IPA_REG0, core, tx5g_ipa_cal_atten,
                params->tx_atten_5g);
            MOD_RADIO_REG_20707(pi, RX5G_REG4, core, rxdb_coup_loopback_attn,
                params->rx_atten_5g);

        }

        RADIO_REG_LIST_START
            MOD_RADIO_REG_20707_ENTRY(pi, RXADC_CFG0, core, rxadc_puI, 1)
            MOD_RADIO_REG_20707_ENTRY(pi, RXADC_CFG0, core, rxadc_puQ, 1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        /* Set TIA gain */
        WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, 0);
        if (aci->papdcali->papdtiagain_iovar != -1) {
            MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain,
                aci->papdcali->papdtiagain_iovar);
        } else {
            MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain,
                CHSPEC_IS2G(pi->radio_chanspec) ?
                params->tia_mode_2g : params->tia_mode_5g);
        }

        /* Watch out, OVR below is common for lna1/2 gain, mixtia and dvga
         * hence zero lna gains via phyregs too
         */
        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain, 1);

        /* Set TIA bandwidth - TCL SVN 805365 */
        if (CHSPEC_IS40(pi->radio_chanspec) || CHSPEC_IS80(pi->radio_chanspec)) {
            MOD_PHYREGCE(pi, RfctrlCoreLpfCT, core, lpf_bq1_bw, 2);
            MOD_PHYREGCE(pi, RfctrlOverrideLpfCT, core, lpf_bq1_bw, 1);
        }

        if (WBPAPD_ENAB(pi)) {
            RADIO_REG_LIST_START
                /* Widen TX filter */
                /* TIA */
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g11, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g12, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g21, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g22, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_c11, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG1, core, tia_g11, 0x0)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG2, core, tia_g12, 0x10a)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG3, core, tia_g21, 0x1765)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG4, core, tia_g22, 0x760)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG6, core, tia_c11, 0x7)
                /* LPF */
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g32, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g33, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g34, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g43, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_c42, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG1, core, lpf_g32, 0xfae)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG2, core, lpf_g33, 0xb16)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG3, core, lpf_g34, 0x18e4)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG4, core, lpf_g43, 0x277)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_c42, 0xb)
                /* Bias */
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG5, core, tia_opamp1_bias, 0x28)
                MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG5, core, tia_opamp2_bias, 0x19)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_opamp3_bias,
                    1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_opamp4_bias,
                    1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG11, core, lpf_opamp3_bias,
                    0x14)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG6, core, lpf_opamp4_bias, 0x14)

                /* Tighten TX notch */
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g1, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g1_cm, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g2, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g3, 1)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_REG2, core,
                    lpf_notch_g1, 0x159)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_REG5, core,
                    lpf_notch_g1_cm, 0x0)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_REG3, core,
                    lpf_notch_g2, 0x159)
                MOD_RADIO_REG_20707_ENTRY(pi, LPF_NOTCH_REG4, core,
                    lpf_notch_g3, 0x55)
            RADIO_REG_LIST_EXECUTE(pi, core);
        }
    }

    MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
    MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);
}

static void
phy_ac_papd_radio_lpbk_setup_20708(phy_info_t *pi, uint16 tx_atten, uint16 rx_atten)
{
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)pi->u.pi_acphy->papdcali;
    phy_ac_papdcal_params_t *params = papdcali->papd_params;
    phy_ac_papdcal_phyregs_t *porig = papdcali->papd_phyregs;
    uint8 core;

    /* Specific cache for Radio Loopback PHY regs that are not per core */
    porig->RxSdFeConfig1 = READ_PHYREG(pi, RxSdFeConfig1);
    porig->RxSdFeConfig6 = READ_PHYREG(pi, RxSdFeConfig6);

    /* Initially save the registers */
    phy_ac_reg_cache_save(aci, RADIOREGS_PAPDCAL);

    FOREACH_CORE(pi, core) {
        /* configuring radio registers per core */
        RADIO_REG_LIST_START
            /* ALPF (BQ2) is in Tx path, as in regular Tx mode */
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,           0)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,       1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc,           0)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,       1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc,           1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,       1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,           1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,       1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,            1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,        1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,            0)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,        1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc,           0)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,       1)
            /* TIA power up sequence */
            MOD_RADIO_REG_20708_ENTRY(pi, TIA_REG7,         core, tia_pu,           1)
            MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR,     core, ovr_tia_pu,       1)
            MOD_RADIO_REG_20708_ENTRY(pi, TIA_REG7,         core, tia_bias_pu,      1)
            MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR,     core, ovr_tia_bias_pu,  1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG6,         core, lpf_bq_pu,        1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1,         core, ovr_lpf_bq_pu,    1)
            /* With shared Tx path always use the 5g LPF output */
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_NOTCH_REG6, core,
                lpf_notch_sel_5g_out_gm, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core,
                ovr_lpf_notch_sel_5g_out_gm, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_NOTCH_REG6, core,
                lpf_notch_sel_2g_out_gm, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core,
                ovr_lpf_notch_sel_2g_out_gm, 1)
            /* LNA Settings common between 2G band and 5G band */
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1,        core,
                rxdb_lna_out_short, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR,    core,
                ovr_rxdb_lna_out_short, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1,        core,
                rxdb_lna_pu, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR,    core,
                ovr_rxdb_lna_pu, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG2,        core,
                rxdb_lna2g_bypass, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RX2G_CFG1_OVR,    core,
                ovr_rxdb_lna2g_bypass, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1, core,
                rxdb_lna5g_bypass, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
                ovr_rxdb_lna5g_bypass, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RX2G_REG4,        core,
                rx2g_lna_tr_rx_en, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR,    core,
                ovr_rx2g_lna_tr_rx_en, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1, core,
                rx5g_lna_tr_rx_en, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
                ovr_rx5g_lna_tr_rx_en, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1,        core,
                rxdb_lna_rout, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR,    core,
                ovr_rxdb_lna_rout, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG6, core,
                rxdb_lna2g_bias_en, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
                ovr_rxdb_lna2g_bias_en, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG6, core,
                rxdb_lna5g_bias_en, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
                ovr_rxdb_lna5g_bias_en, 1)
            /* Mixer Settings common between 2G band and 5G band */
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5,        core,
                rxdb_mix_pu, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR,    core,
                ovr_rxdb_mix_pu, 1)
            /* Don't need loopback coupler, only used for Tx2Rx loopbacks */
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                rxdb_coup_loopback_en, 0)
            MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                rxdb_coup_loopback_attn, 0)
        RADIO_REG_LIST_EXECUTE(pi, core);

        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* AuxGM Section */
            /* PAPD CAL 2G is using PAPD path and aux GM. */
            RADIO_REG_LIST_START
                /* AuxGm does not need mainGM gm_pu, but also gm_bypass
                 * needs to be off.
                 */
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG6, core,
                    rxdb_lna2g_epapd_en, 1)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                    rxdb_lna5g_epapd_en, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                    rxdb_sel2g5g_loadind, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_bypass, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_gm_bypass, 1)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_pu, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_gm_pu, 1)
                MOD_RADIO_REG_20708_ENTRY(pi, RXDB_SPARE, core,
                    rxdb_spare, 4)
            RADIO_REG_LIST_EXECUTE(pi, core);

            /* For B0 specific settings */
            if (RADIOMAJORREV(pi) >= 2) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20708_ENTRY(pi, RXDB_SPARE, core,
                        rxdb_spare, 0)
                    MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                        rxdb_auxgm_bypass, 0)
                    MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                        rxdb_auxgm_en, 1)
                RADIO_REG_LIST_EXECUTE(pi, core);
            }

            RADIO_REG_LIST_START
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_loopback_en_auxpath, 0x1)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_loopback_en_mainpath, 0x0)

                //MOD_RADIO_REG_20708_ENTRY(pi, TX2G_IPA_REG0, core,
                //    tx2g_ipa_cal_pu, 1)
                //MOD_RADIO_REG_20708_ENTRY(pi, TX5G_IPA_REG0, core,
                //    tx5g_ipa_cal_pu, 0)
                //MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                //    rxdb_coup_loopback_5g_en, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG2, core,
                    rxdb_lna_auxpath_en, 0)
                /* AuxGM and AuxGM loading */
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_cc, 15)
                /* from 63178 corners */
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_ibias_ptat_slope, 4)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                    rxdb_mix_Cin_tune, 2)
            RADIO_REG_LIST_EXECUTE(pi, core);
            MOD_RADIO_REG_20708(pi, RX5G_REG5, core, rxdb_lna_epapd_attn, rx_atten);
        } else {
            RADIO_REG_LIST_START
                /* PAPD CAL 5G is using ePAPD paths and AuxGM
                 * (LNA5G in bypass to avoid nonlinear capacitance)"
                 */
                /* Bypass 5G LNA in ePAPD, 2G bypass is
                 * not very effective
                 */
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG6, core,
                    rxdb_lna2g_epapd_en, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                    rxdb_lna5g_epapd_en, 1)
                /* Routing into AuxGM, so LNA settings below are redundand,
                 * but preferably keep the LNA in PD and NOT bypassed/shorted.
                 */
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG2, core,
                    rxdb_lna_auxpath_en, 0)
                /* Other settings around LNA that may impact
                 * since the LNA bypass sems to be required
                 */
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                    rxdb_sel2g5g_loadind, 1)
                /* Common RXDB */
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_bypass, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_gm_bypass, 1)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_pu, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_gm_pu, 1)
                MOD_RADIO_REG_20708_ENTRY(pi, RXDB_SPARE, core,
                    rxdb_spare, 4)
            RADIO_REG_LIST_EXECUTE(pi, core);

            /* For B0 specific settings */
            if (RADIOMAJORREV(pi) >= 2) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20708_ENTRY(pi, RXDB_SPARE, core,
                        rxdb_spare, 0)
                    MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                        rxdb_auxgm_bypass, 0)
                    MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                        rxdb_auxgm_en, 1)
                RADIO_REG_LIST_EXECUTE(pi, core);
            }

            RADIO_REG_LIST_START
                /* Selecting the AuxGM */
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_loopback_en_auxpath, 1)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_loopback_en_mainpath, 0)
                /* Routing & Attenuator Settings */
                //MOD_RADIO_REG_20708_ENTRY(pi, TX2G_IPA_REG0, core,
                //    tx2g_ipa_cal_pu, 0)
                //MOD_RADIO_REG_20708_ENTRY(pi, TX5G_IPA_REG0, core,
                //    tx5g_ipa_cal_pu, 0)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_cc, 15)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_ibias_ptat_slope, 4)
                MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
                    rxdb_mix_Cin_tune, 2)
            RADIO_REG_LIST_EXECUTE(pi, core);
            MOD_RADIO_REG_20708(pi, RX5G_REG5, core, rxdb_lna_epapd_attn, rx_atten);
        }

        RADIO_REG_LIST_START
            MOD_RADIO_REG_20708_ENTRY(pi, RXADC_CFG0, core, rxadc_puI, 1)
            MOD_RADIO_REG_20708_ENTRY(pi, RXADC_CFG0, core, rxadc_puQ, 1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        /* Set TIA gain */
        WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, 0);
        MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain,
            CHSPEC_IS2G(pi->radio_chanspec) ?
            params->tia_mode_2g : params->tia_mode_5g);

        /* Watch out, OVR below is common for lna1/2 gain, mixtia and dvga
         * hence zero lna gains via phyregs too
         */
        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain, 1);

        /* Set TIA bandwidth - TCL SVN 805365 */
        if (CHSPEC_IS40(pi->radio_chanspec) || CHSPEC_IS80(pi->radio_chanspec)) {
            MOD_PHYREGCE(pi, RfctrlCoreLpfCT, core, lpf_bq1_bw, 2);
            MOD_PHYREGCE(pi, RfctrlOverrideLpfCT, core, lpf_bq1_bw, 1);
        }
    }

    MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
    MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);
}

static void
phy_ac_papd_radio_lpbk_setup_20709(phy_info_t *pi, bool epapd_flag)
{
/* ALPF bypass mode matches the normal reception mode, where ALPF is also bypassed */
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)pi->u.pi_acphy->papdcali;
    phy_ac_papdcal_params_t *params = papdcali->papd_params;
    phy_ac_papdcal_phyregs_t *porig = papdcali->papd_phyregs;
    uint8 core, papd_loopback_mode, papd_bypass_mode;

    uint8 tia_mode_2g = params->tia_mode_2g;
    uint8 tx_atten_2g = params->tx_atten_2g;
    uint8 tia_mode_5g = params->tia_mode_5g;

    /* Specific cache for Radio Loopback PHY regs that are not per core */
    porig->RxSdFeConfig1 = READ_PHYREG(pi, RxSdFeConfig1);
    porig->RxSdFeConfig6 = READ_PHYREG(pi, RxSdFeConfig6);

    /* Initially save the registers */
    phy_ac_reg_cache_save(aci, RADIOREGS_PAPDCAL);

    if (RADIOREV(pi->pubpi->radiorev) == 0) {
        /* A0 chip does not have AUXGM bypass */
        papd_loopback_mode = AUXGM_PATH;
        papd_bypass_mode   = 0;
    } else if (pi->sh->did == BCM6878_D11AC2G_ID) {
        /* 15x15 A1 and above use auxgm without bypass */
        papd_loopback_mode = AUXGM_PATH;
        papd_bypass_mode   = 0;
    } else {
        /* 21x21 A1 and above using auxgm with bypass */
        papd_loopback_mode = AUXGM_PATH;
        papd_bypass_mode   = 1;
    }

    FOREACH_CORE(pi, core) {
        /* configuring radio registers per core */
        RADIO_REG_LIST_START

            /* ALPF (BQ2) is not in Tx path, as in regular Tx mode */
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,           1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2,       1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc,           1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,       1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc,           0)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,       1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,           0)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,       1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,            0)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,        1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,            1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,        1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc,           0)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,       1)

            /* TIA power up sequence */
            MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG7,         core, tia_pu,           1)
            MOD_RADIO_REG_20709_ENTRY(pi, TIA_CFG1_OVR,     core, ovr_tia_pu,       1)
            MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG7,         core, tia_bias_pu,      1)
            MOD_RADIO_REG_20709_ENTRY(pi, TIA_CFG1_OVR,     core, ovr_tia_bias_pu,  1)

            MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG6, core, lpf_bq_pu,     0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_bq_pu, 0x1)

            /* Rx mixer, Rx rccr and rx div2 buf power up/down
             * (shared between 2G/5G))
             */
            MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                core, rxdb_mix_pu, 0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                core, ovr_rxdb_mix_pu, 0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG0,
                core, logen_rx_rccr_pu, 0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_OVR0,
                core, ovr_logen_rx_rccr_pu, 0x1)

            MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_OVR0,
                core, ovr_logen_div2_rxbuf_pu, 0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG5,
                core, logen_div2_rxbuf_pu, 0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG0,
                core, logen_div2_pu, 0x1)

            /*  power down dualband LNA and set short (same setting for both bands)) */
            MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG1,
                core, rxdb_lna_out_short,     0x0)
            MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                core, ovr_rxdb_lna_out_short, 0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG1,
                core, rxdb_lna_pu, 0x0)
            MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                core, ovr_rxdb_lna_pu, 0x1)

            /* open up db LNA bypass for each band
             * (same setting for both bands))
             */
            MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG2,
                core, rxdb_lna2g_bypass,     0x0)
            MOD_RADIO_REG_20709_ENTRY(pi, RX2G_CFG1_OVR,
                core, ovr_rxdb_lna2g_bypass, 0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG1,
                core, rxdb_lna5g_bypass,     0x0)
            MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                core, ovr_rxdb_lna5g_bypass, 0x1)

            MOD_RADIO_REG_20709_ENTRY(pi, RX2G_REG4,
                core, rx2g_lna_tr_rx_en,     0x0)
            MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                core, ovr_rx2g_lna_tr_rx_en, 0x1)
            MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG1,
                core, rx5g_lna_tr_rx_en,     0x0)
            MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                core, ovr_rx5g_lna_tr_rx_en, 0x1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* PAPD CAL 2G is using PAPD path and aux GM */
            RADIO_REG_LIST_START
                /* Configure GM (AUX or MAIN) */
                /* This circuit is dual band
                 * (shared over 2/5G, but we might want to)
                 * set it different per band,    core, )
                 */
                MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG0,
                    core, tx2g_pad_gain_offset_en, 0) /* TCL r813823 */
                MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG1,
                    core, rxdb_lna_rout, 0x0)
                MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                    core, ovr_rxdb_lna_rout, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_REG6,
                    core, lpf_notch_sel_2g_out_gm, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1,
                    core, ovr_lpf_notch_sel_2g_out_gm, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG2,
                    core, rxdb_lna_auxpath_en, 0x0)
            RADIO_REG_LIST_EXECUTE(pi, core);

            if (papd_loopback_mode == MAINGM_PATH) {
                /* Follow 63178 config: MAIN GM */
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_gm_pu, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                        core, ovr_rxdb_gm_pu, 0x1)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_gm_loopback_en_mainpath, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_gm_loopback_en_auxpath, 0x0)
                    /* 1 gave more G and lower 3fbb in dBc */
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                        core, rxdb_mix_Cin_tune, 1)
                RADIO_REG_LIST_EXECUTE(pi, core);
            } else {
                /* AUX GM */
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_gm_pu, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                        core, ovr_rxdb_gm_pu, 0x1)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_gm_loopback_en_mainpath, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_gm_loopback_en_auxpath, 0x1)
                    /* 1 gave more G and lower 3fbb in dBc */
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                        core, rxdb_mix_Cin_tune, 2)
                RADIO_REG_LIST_EXECUTE(pi, core);
            }

            /* BYPASS */
            MOD_RADIO_REG_20709(pi, RX5G_REG4,
                core, rxdb_gm_bypass, papd_bypass_mode);
            MOD_RADIO_REG_20709(pi, RXDB_CFG1_OVR,
                core, ovr_rxdb_gm_bypass, 0x1);

            /* Radio Rev 1 ECO for AUX GM bypass */
            if (RADIOREV(pi->pubpi->radiorev) > 0) {
                tia_mode_2g = 4;
                if (!papd_bypass_mode) {
                    MOD_RADIO_REG_20709(pi, RX5G_REG6,
                        core, rxdb_spare, 0x10);
                    tx_atten_2g = 3;
                } else {
                    MOD_RADIO_REG_20709(pi, RX5G_REG6,
                        core, rxdb_spare, 0x20);
                    tx_atten_2g = 1;
                }
            }

            /* Set TX and RX coupler attenuation */
            MOD_RADIO_REG_20709(pi, TX2G_IPA_REG0,
                core, tx2g_ipa_cal_atten, tx_atten_2g);
            MOD_RADIO_REG_20709(pi, RX5G_REG4,
                core, rxdb_coup_loopback_attn, params->rx_atten_2g);

            if (!epapd_flag) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG0,
                        core, tx2g_ipa_cal_pu, 0x1)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG6,
                        core, rxdb_lna2g_epapd_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                        core, rxdb_lna5g_epapd_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_coup_loopback_2g_en, 0x1)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_coup_loopback_5g_en, 0x0)
                RADIO_REG_LIST_EXECUTE(pi, core);
            } else {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG6,
                        core, rxdb_lna2g_epapd_en, 0x1)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                        core, rxdb_lna5g_epapd_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                        core, rxdb_lna_epapd_attn, 0x3)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_coup_loopback_2g_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_coup_loopback_5g_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG0,
                        core, tx2g_ipa_cal_pu, 0x0)
                RADIO_REG_LIST_EXECUTE(pi, core);
            }
        } else {
            RADIO_REG_LIST_START
                /* Configure GM (AUX or MAIN w/o bypass) */
                /* This circuit is dual band
                 * (shared over 2/5G) but we might want to
                 * set it different per band.
                 */
                /* Use AUX GM path, as originally intended */
                MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                    core, rxdb_gm_pu, 0x0)
                MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                    core, ovr_rxdb_gm_pu, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                    core, rxdb_gm_bypass, 0x0)
                MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                    core, ovr_rxdb_gm_bypass, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                    core, rxdb_gm_loopback_en_mainpath, 0x0)
                MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                    core, rxdb_gm_loopback_en_auxpath, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG1,
                    core, rxdb_lna_rout, 0x0)
                MOD_RADIO_REG_20709_ENTRY(pi, RXDB_CFG1_OVR,
                    core, ovr_rxdb_lna_rout, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_REG6,
                    core, lpf_notch_sel_5g_out_gm, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1,
                    core, ovr_lpf_notch_sel_5g_out_gm, 0x1)
                MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG2,
                    core, rxdb_lna_auxpath_en, 0x0)
            RADIO_REG_LIST_EXECUTE(pi, core);

            /* Radio Rev 1 ECO for AUX GM bypass */
            if (RADIOREV(pi->pubpi->radiorev) > 0) {
                if (!papd_bypass_mode) {
                    MOD_RADIO_REG_20709(pi, RX5G_REG6,
                        core, rxdb_spare, 0x10);
                    tia_mode_5g = 2;
                } else {
                    MOD_RADIO_REG_20709(pi, RX5G_REG6,
                        core, rxdb_spare, 0x20);
                    tia_mode_5g = 4;
                }
            }

            /* Set TX and RX coupler attenuation */
            MOD_RADIO_REG_20709(pi, TX5G_IPA_REG0,
                core, tx5g_ipa_cal_atten, params->tx_atten_5g);
            MOD_RADIO_REG_20709(pi, RX5G_REG4,
                core, rxdb_coup_loopback_attn, params->rx_atten_5g);

            if (!epapd_flag) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG0,
                        core, tx5g_ipa_cal_pu, 0x1)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG6,
                        core, rxdb_lna2g_epapd_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                        core, rxdb_lna5g_epapd_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_coup_loopback_2g_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_coup_loopback_5g_en, 0x1)
                RADIO_REG_LIST_EXECUTE(pi, core);
            } else {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG6,
                        core, rxdb_lna2g_epapd_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                        core, rxdb_lna5g_epapd_en, 0x1)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5,
                        core, rxdb_lna_epapd_attn, 0x3)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_coup_loopback_2g_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4,
                        core, rxdb_coup_loopback_5g_en, 0x0)
                    MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG0,
                        core, tx5g_ipa_cal_pu, 0x0)
                RADIO_REG_LIST_EXECUTE(pi, core);
            }

        }

        RADIO_REG_LIST_START
            MOD_RADIO_REG_20709_ENTRY(pi, RXADC_CFG0,
                core, rxadc_puI, 1)
            MOD_RADIO_REG_20709_ENTRY(pi, RXADC_CFG0,
                core, rxadc_puQ, 1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        /* Set TIA gain */
        WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, 0);
        MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain,
            CHSPEC_IS2G(pi->radio_chanspec) ?
            tia_mode_2g : tia_mode_5g);
        /* Watch out, OVR below is common for lna1/2 gain, mixtia and dvga
         * hence zero lna gains via phyregs too
         */
        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain, 1);

        if (WBPAPD_ENAB(pi)) {
            RADIO_REG_LIST_START
                /* Widen TX filter */
                /* TIA */
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG1, core, tia_g11, 0x0)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g11, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG2, core, tia_g12, 0x10a)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g12, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG3, core, tia_g21, 0x1765)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g21, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG4, core, tia_g22, 0x760)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g22, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG6, core, tia_c11, 0x7)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_c11, 1)
                /* LPF */
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG1, core, lpf_g32, 0xfae)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g32, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG2, core, lpf_g33, 0xb16)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g33, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG3, core, lpf_g34, 0x18e4)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g34, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG4, core, lpf_g43, 0x277)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g43, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_c42, 0xb)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_c42, 1)
                /* Bias */
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG5, core, tia_opamp1_bias, 0x28)
                MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG5, core, tia_opamp2_bias, 0x19)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG11, core, lpf_opamp3_bias,
                    0x14)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_opamp3_bias,
                    1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG6, core, lpf_opamp4_bias, 0x14)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_opamp4_bias,
                    1)

                /* Tighten TX notch */
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_REG2, core,
                    lpf_notch_g1, 0x159)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g1, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_REG5, core,
                    lpf_notch_g1_cm, 0x0)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g1_cm, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_REG3, core,
                    lpf_notch_g2, 0x159)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g2, 1)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_REG4, core,
                    lpf_notch_g3, 0x55)
                MOD_RADIO_REG_20709_ENTRY(pi, LPF_NOTCH_OVR1, core,
                    ovr_lpf_notch_g3, 1)
            RADIO_REG_LIST_EXECUTE(pi, core);
        }
    }

    MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
    MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);

}

static void
phy_ac_papd_radio_lpbk_setup_20710(phy_info_t *pi, bool epapd_flag)
{
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)pi->u.pi_acphy->papdcali;
    phy_ac_papdcal_params_t *params    = papdcali->papd_params;
    phy_ac_papdcal_phyregs_t *porig = papdcali->papd_phyregs;

    uint8 core, papd_loopback_mode, papd_bypass_mode;
    uint8 tia_gain = params->tia_mode_2g;
    uint8 rx_atten = params->rx_atten_2g;

    /* Specific cache for Radio Loopback PHY regs that are not per core */
    porig->RxSdFeConfig1 = READ_PHYREG(pi, RxSdFeConfig1);
    porig->RxSdFeConfig6 = READ_PHYREG(pi, RxSdFeConfig6);

    /* Initially save the registers */
    phy_ac_reg_cache_save(aci, RADIOREGS_PAPDCAL);

    /* Config from NVRAM */
    papd_loopback_mode = params->papd_loopback_mode;
    papd_bypass_mode   = params->papd_bypass_mode;

    FOREACH_CORE(pi, core) {
        /* configuring radio registers per core */
        RADIO_REG_LIST_START
            /* ALPF (BQ2) is in Tx path, as in regular Tx mode */
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR2,    core, ovr_lpf_sw_bq1_bq2,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_bq2,        0)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc,        0)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc,        1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_bq2,        1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_rc,        1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_dac_rc,        0)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc,        0)
            /* TIA power up sequence */
            MOD_RADIO_REG_20710_ENTRY(pi, TIA_CFG1_OVR,    core, ovr_tia_pu,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, TIA_REG7,        core, tia_pu,        1)
            MOD_RADIO_REG_20710_ENTRY(pi, TIA_CFG1_OVR,    core, ovr_tia_bias_pu,    1)
            MOD_RADIO_REG_20710_ENTRY(pi, TIA_REG7,        core, tia_bias_pu,    1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* PAPD CAL 2G is using PAPD path and aux GM */
            RADIO_REG_LIST_START
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_out_short,    1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_out_short, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_pu, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RX2G_CFG1_OVR,    core,
                    ovr_rxdb_lna2g_bypass, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG2,    core,
                    rxdb_lna2g_bypass, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rx2g_lna_tr_rx_en, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX2G_REG4,    core,
                    rx2g_lna_tr_rx_en, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_rout, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_rout, 0)

                /* Rx mixer, Rx rccr and rx div2 buf power up/down */
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5,    core,
                    rxdb_mix_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_mix_pu, 1)

                MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0,    core,
                    ovr_logen_div2_rxbuf_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0,    core,
                    logen_2g_div2_rx_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0,    core,
                    logen_rx_rccr_pu, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0,    core,
                    ovr_logen_rx_rccr_pu, 1)

                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG2, core,
                    rxdb_lna_auxpath_en, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                    rxdb_gm_pu, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR, core,
                    ovr_rxdb_gm_pu, 1)
            RADIO_REG_LIST_EXECUTE(pi, core);

            if (papd_loopback_mode == MAINGM_PATH) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 0)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 0)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 0)
                    MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0, core,
                        tx2g_pa_cal_pu, 0)
                RADIO_REG_LIST_EXECUTE(pi, core);

                MOD_RADIO_REG_20710(pi, RX5G_REG4, core,
                    rxdb_gm_bypass, papd_bypass_mode);
                MOD_RADIO_REG_20710(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_gm_bypass, 1);
            } else {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_bypass, 0)
                    MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                        ovr_rxdb_gm_bypass, 1)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_mainpath, 0)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_loopback_en_auxpath, 1)
                    MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0, core,
                        tx2g_pa_cal_pu, 1)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 1)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5, core,
                        rxdb_mix_Cin_tune, 2)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_gm_cc, 6)
                RADIO_REG_LIST_EXECUTE(pi, core);

                if (papd_bypass_mode == 0) {
                    MOD_RADIO_REG_20710(pi, RX5G_REG6, core,
                        rxdb_spare, 0x10);
                    tia_gain = 3;

                } else {
                    MOD_RADIO_REG_20710(pi, RX5G_REG6, core,
                        rxdb_spare, 0x20);
                    rx_atten = 0;
                    tia_gain = 7;
                }
            }
            /* Set TX and RX coupler attenuation */
            MOD_RADIO_REG_20710(pi, TX2G_PA_REG0, core, tx2g_pa_cal_atten,
                params->tx_atten_2g);
            MOD_RADIO_REG_20710(pi, RX5G_REG4, core, rxdb_coup_loopback_attn,
                rx_atten);

            if (!epapd_flag) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6, core,
                        rxdb_lna2g_epapd_en, 0)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5, core,
                        rxdb_lna5g_epapd_en, 0)
                RADIO_REG_LIST_EXECUTE(pi, core);
            } else {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6, core,
                        rxdb_lna2g_epapd_en, 1)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5, core,
                        rxdb_lna5g_epapd_en, 0)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5, core,
                        rxdb_lna_epapd_attn, 3)
                    MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4, core,
                        rxdb_coup_loopback_2g_en, 0)
                    MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0, core,
                        tx2g_pa_cal_pu, 0)
                RADIO_REG_LIST_EXECUTE(pi, core);
            }
        } else {
            /* ePAPD CAL 5G - cal path goes through whole receiver (including LNA)
             * and mainGM with bypass
             */
            RADIO_REG_LIST_START
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_out_short,    1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_out_short, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX2G_CFG1_OVR,    core,
                    ovr_rxdb_lna2g_bypass, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG2,    core,
                    rxdb_lna2g_bypass, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rx5g_lna_tr_rx_en, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG1,    core,
                    rx5g_lna_tr_rx_en, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_lna_rout, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG1,    core,
                    rxdb_lna_rout, 0)

                /* Rx mixer, Rx rccr and rx div2 buf power up */
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5,    core,
                    rxdb_mix_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_mix_pu, 1)

                MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0,    core,
                    ovr_logen_div2_rxbuf_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0,    core,
                    logen_2g_div2_rx_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0,    core,
                    logen_rx_rccr_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0,    core,
                    ovr_logen_rx_rccr_pu, 1)

                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4,    core,
                    rxdb_gm_pu, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_gm_pu, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4,    core,
                    rxdb_gm_bypass, 1)
                MOD_RADIO_REG_20710_ENTRY(pi, RXDB_CFG1_OVR,    core,
                    ovr_rxdb_gm_bypass, 1)

                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4,    core,
                    rxdb_gm_loopback_en_mainpath, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4,    core,
                    rxdb_gm_loopback_en_auxpath, 0)

                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG2,    core,
                    rxdb_lna_auxpath_en, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6,    core,
                    rxdb_lna2g_epapd_en, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5,    core,
                    rxdb_lna5g_epapd_en, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5,    core,
                    rxdb_lna_epapd_attn, 0)

                MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG4,    core,
                    rxdb_coup_loopback_2g_en, 0)
                MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0,    core,
                    tx2g_pa_cal_pu, 0)
            RADIO_REG_LIST_EXECUTE(pi, core);
        }

        RADIO_REG_LIST_START
            MOD_RADIO_REG_20710_ENTRY(pi, RXADC_CFG0, core, rxadc_puI, 1)
            MOD_RADIO_REG_20710_ENTRY(pi, RXADC_CFG0, core, rxadc_puQ, 1)
        RADIO_REG_LIST_EXECUTE(pi, core);

        /* Set TIA gain */
        WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, 0);
        MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain, tia_gain);
        /* Watch out, OVR below is common for lna1/2 gain, mixtia and dvga
         * hence zero lna gains via phyregs too
         */
        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain, 1);

        /* Set TIA bandwidth - TCL SVN 802152 */
        if (CHSPEC_IS40(pi->radio_chanspec)) {
            MOD_PHYREGCE(pi, RfctrlCoreLpfCT, core, lpf_bq1_bw, 2);
            MOD_PHYREGCE(pi, RfctrlOverrideLpfCT, core, lpf_bq1_bw, 1);
        }

        if (WBPAPD_ENAB(pi)) {
            MOD_PHYREGCE(pi, RfctrlCoreLpfCT, core, lpf_bq1_bw, 3);
            MOD_PHYREGCE(pi, RfctrlOverrideLpfCT, core, lpf_bq1_bw, 1);
        }
    }

    MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
    MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);

}

static void
phy_ac_papd_radio_lpbk_cleanup_tiny(phy_info_t *pi)
{
    uint8 core;

    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_radioregs_t *porig = (pi_ac->papdcali->papd_radioregs);

    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    FOREACH_CORE(pi, core) {
        /* # Enable the loopback path */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* # Powerup the 2G iPA attenuation on Tx side (loops back to Rx) */
            MOD_RADIO_REG_TINY(pi, TX2G_MISC_CFG1, core, cal2g_pa_pu, 0x0);
            /* # Powerup the papd loopback path on 2G Rx side */
            MOD_RADIO_REG_TINY(pi, RXRF2G_CFG2, core, loopback2g_papdcal_pu, 0x0);

            if (ROUTER_4349(pi)) {
                MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST2, core,
                    ovr_lna2g_tr_rx_en, 0);
            } else {
                MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST2, core,
                    ovr_lna2g_tr_rx_en, 1);
                MOD_RADIO_REG_20693(pi, LNA2G_CFG1, core,
                    lna2g_tr_rx_en, 1);
            }

            MOD_RADIO_REG_TINY(pi, RX_TOP_2G_OVR_EAST, core,
                ovr_gm2g_pwrup, 0);
            if (!RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                MOD_RADIO_REG_TINY(pi, RX_TOP_2G_OVR_NORTH, core,
                    ovr_rf2g_mix1st_en, 0);
                MOD_RADIO_REG_TINY(pi, RX_TOP_2G_OVR_EAST, core,
                                    ovr_gm2g_auxgm_pwrup, 1);
                MOD_RADIO_REG_TINY(pi, RX_TOP_2G_OVR_NORTH, core,
                    ovr_lna2g_lna1_bypass, 0);
            }
            MOD_RADIO_REG_TINY(pi, LNA2G_CFG2, core, gm2g_auxgm_pwrup, 0);
            MOD_RADIO_REG_TINY(pi, RXRF2G_CFG2, core, lna2g_epapd_en, 0);
            if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST, core,
                    ovr_lna2g_lna2_gain, 0);
                MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST2, core,
                    ovr_lna2g_lna1_Rout, 0);
                MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST2, core,
                    ovr_lna2g_lna1_out_short_pu, 0);
            }
        } else {
            /* # Powerdown the 5G iPA attenuation on Tx side (loops back to Rx) */
            MOD_RADIO_REG_TINY(pi, TX5G_MISC_CFG1, core, cal5g_pa_pu, 0x0);
            /* # Powerdown the master cal pu signal on 5G Rx side (common to papd &
             * rxiqcal). Not needed for rc/cr rxiqcal pu.
             */
            MOD_RADIO_REG_TINY(pi, TXRX5G_CAL_RX, core, loopback5g_cal_pu, 0x0);
            MOD_RADIO_REG_TINY(pi, RX_TOP_5G_OVR, core, ovr_gm5g_pwrup, 0);
            if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                MOD_RADIO_REG_TINY(pi, LNA5G_CFG2, core, lna5g_pu_lna2, 0);
            } else {
                MOD_RADIO_REG_TINY(pi, RX_TOP_5G_OVR, core,
                                    ovr_lna5g_pu_auxlna2, 1);
            }
            RADIO_REG_LIST_START
                MOD_RADIO_REG_TINY_ENTRY(pi, LNA5G_CFG2, core, lna5g_pu_auxlna2, 0)
                MOD_RADIO_REG_TINY_ENTRY(pi, RX_TOP_5G_OVR, core,
                    ovr_lna5g_lna1_bypass, 0)
                MOD_RADIO_REG_TINY_ENTRY(pi, PA5G_CFG1, core, rf5g_epapd_en, 0)
            RADIO_REG_LIST_EXECUTE(pi, core);

            if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
                RADIO_REG_LIST_START
                    MOD_RADIO_REG_20693_ENTRY(pi, SPARE_CFG16, core,
                        loopback5g_gm5g_pu, 0)
                    MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_5G_OVR, core,
                        ovr_lna5g_lna1_pu, 0)
                    MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_5G_OVR2, core,
                        ovr_lna5g_lna2_gain, 0)
                    MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_5G_OVR, core,
                        ovr_mix5g_en, 1)
                    MOD_RADIO_REG_20693_ENTRY(pi, RX_TOP_5G_OVR, core,
                        ovr_lna5g_tr_rx_en, 0)
                    MOD_RADIO_REG_20693_ENTRY(pi, TX_TOP_5G_OVR1, core,
                        ovr_pa5g_pu, 0)
                    MOD_RADIO_REG_20693_ENTRY(pi, PA5G_CFG4, core, pa5g_pu, 0)
                RADIO_REG_LIST_EXECUTE(pi, core);
            }
        }
        MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_od_pu, 0);
    }
    MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 0);
    MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);

    FOREACH_CORE(pi, core) {
        phy_utils_write_radioreg(pi, RADIO_REG(pi, ADC_CFG10, core),
                                 porig->adc_cfg10[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, ADC_OVR1, core),
                                 porig->adc_ovr1[core]);
    }
}

static void
phy_ac_papd_radio_lpbk_cleanup_28nm(phy_info_t *pi)
{
    /* restore radio config back */
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_reg_cache_restore(aci, RADIOREGS_PAPDCAL);
}

static void
phy_ac_wbcal_widen_filter(phy_info_t *pi, uint8 tia_mode, uint8 core)
{
    /* Only support 6715 so far */
    if (!ACMAJORREV_130(pi->pubpi->phy_rev)) {
        return;
    } else { // For 6715, follow TCL proc 20708_tia_lpf_set_by_override
        /* TIA */
        tia_calc_20708(pi, tia_mode, core);
        /* LPF */
        lpf_calc_20708(pi, core);
        /* Bias */
        tia_lpf_bias_calc_20708(pi, core);
    }
}

static void
tia_calc_20708(phy_info_t *pi, uint8 bq1_gain, uint8 core)
{
    phy_ac_radio_data_t *data = phy_ac_radio_get_data(pi->u.pi_acphy->radioi);
    uint16  rccal_gmult_rc = data->rccal_gmult_rc;
    uint16 temp;
    uint16 g11, g12, g21, g22, c11;

    uint8 c11_tbl[] = {31, 31, 31, 21, 14, 10, 7, 5, 3, 2, 1, 3, 5, 5, 5, 5};
    uint16 g22_tbl[] = {1816, 1816, 1816, 1816, 1816, 1816, 1816, 1816,
        1816, 1816, 1816, 1573, 1124, 1124, 1124, 1124};
    uint16 g12_tbl[] = {2048, 1448, 1024, 724, 512, 362, 256, 181, 128, 90, 64, 0, 0, 0, 0, 0};
    uint16 g11_tbl[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1024, 1024, 1024, 1024, 1024};
    uint16 g21_tbl[] = {3188, 4509, 6377, 6109, 5759, 5818, 5759, 5818,
        4937, 4654, 3291, 6295, 4496, 4496, 4496, 4496};

    temp = (g11_tbl[bq1_gain] * rccal_gmult_rc) >> 12;
    g11 = (temp > 8191) ? 8191 : temp;
    temp = (g12_tbl[bq1_gain] * rccal_gmult_rc) >> 12;
    g12 = (temp > 8191) ? 8191 : temp;
    temp = (g21_tbl[bq1_gain] * rccal_gmult_rc) >> 12;
    g21 = (temp > 8191) ? 8191 : temp;
    temp = (g22_tbl[bq1_gain] * rccal_gmult_rc) >> 12;
    g22 = (temp > 8191) ? 8191 : temp;
    c11 = c11_tbl[bq1_gain];

    RADIO_REG_LIST_START
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g11, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g12, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g21, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_g22, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_c11, 1)
    RADIO_REG_LIST_EXECUTE(pi, core);
    MOD_RADIO_REG_20708(pi, TIA_REG1, core, tia_g11, g11);
    MOD_RADIO_REG_20708(pi, TIA_REG2, core, tia_g12, g12);
    MOD_RADIO_REG_20708(pi, TIA_REG3, core, tia_g21, g21);
    MOD_RADIO_REG_20708(pi, TIA_REG4, core, tia_g22, g22);
    MOD_RADIO_REG_20708(pi, TIA_REG6, core, tia_c11, c11);
}

static void
lpf_calc_20708(phy_info_t *pi, uint8 core)
{
    phy_ac_radio_data_t *data = phy_ac_radio_get_data(pi->u.pi_acphy->radioi);
    uint16  rccal_gmult_rc = data->rccal_gmult_rc;
    uint16 temp;
    uint16 g32, g33, g34, g43, c42;
    uint8 bw_idx;

    uint16 g32_tbl[] = {2084, 2222, 1138, 2295};
    uint16 g33_tbl[] = {1170, 3119, 1012, 2958};
    uint16 g34_tbl[] = {3308, 3529, 1806, 3643};
    uint16 g43_tbl[] = {321, 1378, 705, 1421};
    uint8 c42_tbl[] = {8, 8, 1, 2};

    if (CHSPEC_IS160(pi->radio_chanspec)) {
        bw_idx = 3; // bq2bwrcbw = 7
    } else if (CHSPEC_IS80(pi->radio_chanspec)) {
        bw_idx = 2; // bq2bwrcbw = 6
    } else if (CHSPEC_IS40(pi->radio_chanspec)) {
        // bq2bwrcbw = 5 for 2G and 6 for 5G due to insufficient SNR at 40M low pwr region
        bw_idx = CHSPEC_IS2G(pi->radio_chanspec) ? 1 : 2;
    } else {
        bw_idx = 0; // bq2bwrcbw = 4
    }

    temp = (g32_tbl[bw_idx] * rccal_gmult_rc) >> 12;
    g32 = (temp > 8191) ? 8191 : temp;
    temp = (g33_tbl[bw_idx] * rccal_gmult_rc) >> 12;
    g33 = (temp > 8191) ? 8191 : temp;
    temp = (g34_tbl[bw_idx] * rccal_gmult_rc) >> 12;
    g34 = (temp > 8191) ? 8191 : temp;
    temp = (g43_tbl[bw_idx] * rccal_gmult_rc) >> 12;
    g43 = (temp > 8191) ? 8191 : temp;
    c42 = c42_tbl[bw_idx];

    RADIO_REG_LIST_START
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g32, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g33, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g34, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_g43, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_c42, 1)
    RADIO_REG_LIST_EXECUTE(pi, core);
    MOD_RADIO_REG_20708(pi, LPF_REG1, core, lpf_g32, g32);
    MOD_RADIO_REG_20708(pi, LPF_REG2, core, lpf_g33, g33);
    MOD_RADIO_REG_20708(pi, LPF_REG3, core, lpf_g34, g34);
    MOD_RADIO_REG_20708(pi, LPF_REG4, core, lpf_g43, g43);
    MOD_RADIO_REG_20708(pi, LPF_REG7, core, lpf_c42, c42);
}

static void
tia_lpf_bias_calc_20708(phy_info_t *pi, uint8 core)
{
    uint8 bw_idx;
    uint8 opamp3_bias_table[] = {20, 20, 20, 40};
    uint8 opamp4_bias_table[] = {20, 20, 20, 40};

    if (CHSPEC_IS160(pi->radio_chanspec)) {
        bw_idx = 3; // bq2bwrcbw = 7
    } else if (CHSPEC_IS80(pi->radio_chanspec)) {
        bw_idx = 2; // bq2bwrcbw = 6
    } else if (CHSPEC_IS40(pi->radio_chanspec)) {
        // bq2bwrcbw = 5 for 2G and 6 for 5G due to insufficient SNR at 40M low pwr region
        bw_idx = CHSPEC_IS2G(pi->radio_chanspec) ? 1 : 2;
    } else {
        bw_idx = 0; // bq2bwrcbw = 4
    }

    RADIO_REG_LIST_START
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_opamp1_bias, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_opamp2_bias, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_REG5, core, tia_opamp1_bias, 0x28)
        MOD_RADIO_REG_20708_ENTRY(pi, TIA_REG5, core, tia_opamp2_bias, 0x28)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_opamp3_bias, 1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_opamp4_bias, 1)
    RADIO_REG_LIST_EXECUTE(pi, core);
    MOD_RADIO_REG_20708(pi, LPF_REG11, core, lpf_opamp3_bias, opamp3_bias_table[bw_idx]);
    MOD_RADIO_REG_20708(pi, LPF_REG6, core, lpf_opamp4_bias, opamp4_bias_table[bw_idx]);
}

static bool
calculate_calibrated_epsOffset(phy_info_t *pi, uint8 core, uint8 cal_idx, uint8 papd_lut_select,
    uint8 gfrac_bits, uint32 *original_eps_table, int16 *epsilonOffset)
{
    uint16 tbl_size = phy_ac_papdcal_eps_table_size(pi, core);
    uint8 tbl_step = 1;
    uint32 eps_complex;
    int32 eps_re, eps_im;
    uint32 tempclip;
    uint16 one = (1 << gfrac_bits);
    uint8 curr_max_location = 0;
    bool success;
    int16 dacplusrfgain;
    int16 log10mantissa, log10exp, fortylog10;
    uint8 eps_table_ids[] = {ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
    uint8 rfpwrlut_table_ids[] = {ACPHY_TBL_ID_RFPWRLUTS0, ACPHY_TBL_ID_RFPWRLUTS1,
        ACPHY_TBL_ID_RFPWRLUTS2, ACPHY_TBL_ID_RFPWRLUTS3};

    if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        tbl_step = 2;
    }

    curr_max_location = phy_ac_wbcal_eps_stopidx(pi, core);

    success = (curr_max_location > 0);
    if (!success) {
        PHY_PAPD(("%s epsOffset cal failed with curr_max_location = %d.\n",
            __FUNCTION__, curr_max_location));
    }

    if (success) {
        eps_complex = original_eps_table[tbl_step * curr_max_location];
        phy_papdcal_decode_epsilon(eps_complex, &eps_re, &eps_im);
        tempclip = (one + eps_re) * (one + eps_re) + (eps_im * eps_im);

        wlc_phy_table_read_acphy(pi, rfpwrlut_table_ids[core],
            1, cal_idx, 16, &dacplusrfgain);
        // Sign extension.
        if (dacplusrfgain >> 8)
            dacplusrfgain -= 512;

        /* Calculate 40*log10(520^2 / (tempclip/2^(2*gfrac_bits)) / 16])
         * = 40*log10(520^2/16) - 40*log10(tempclip/2^(2*gfrac_bits))
         * = 169.1155 - [40*log10(tempclip) - 80 * gfrac_bits * log10(2)]
         * = 169.1155 + 24.0824 * gfrac_bits - 40*log10(tempclip)
         * = 173174 + 24660 * gfrac_bits - 40*log10(tempclip) in 10 fractional bits.
         */
        // Multiplied by 10 to increase precision of qm_log10
        qm_log10((int32)(10*tempclip), 0, &log10mantissa, &log10exp);
        // Align the exponent to 10.
        if (log10exp > 10) {
            log10mantissa >>= (log10exp - 10);
        } else if (log10exp < 10) {
            log10mantissa <<= (10 - log10exp);
        }
        // Add 40 = 40log10(10) back due to *10 before
        fortylog10 = ((173174 + 24660 * gfrac_bits - 40 * log10mantissa + 512) >> 10) + 40;
        *epsilonOffset = curr_max_location - (fortylog10 + dacplusrfgain);

        // Manual offset to improve EVM.
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            if (CHSPEC_IS20(pi->radio_chanspec)) {
                *epsilonOffset -= 2;
            } else if (CHSPEC_IS40(pi->radio_chanspec) && (papd_lut_select == 0)) {
                *epsilonOffset -= 4;
            }
        }

        // Load original eps table back into chip.
        wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core], tbl_size,
            papd_lut_select * tbl_size, 32, original_eps_table, core);

        PHY_PAPD(("%s core%d EpsTable %d Peak@ %d 40log10 %d Calibrated epsilonOffset %d\n",
            __FUNCTION__, core, papd_lut_select, curr_max_location, fortylog10,
            *epsilonOffset));
    }

    return success;
}

static void
set_epsilon_offset(phy_info_t *pi, uint8 core, bool use_calc, int16 calc_epsoffset)
{
    phy_ac_papdcal_params_t *params    = pi->u.pi_acphy->papdcali->papd_params;
    int16 param_epsoffset = CHSPEC_IS2G(pi->radio_chanspec) ?
            params->epsilon_offset_2g : params->epsilon_offset_5g;
    int16 offsetOfOffset;
    uint8 papd_lut_select;
    int32 multi_tbl_en = pi->u.pi_acphy->papdcali->wbpapd_multitbl_iovar;
    int32 extraepsoffset = pi->u.pi_acphy->papdcali->extraepsoffset_iovar;

    if (multi_tbl_en) {
        // Overwrite the statically configured value with the one calculated by
        // gain control or the value from epsilonOffset calibration.
        papd_lut_select = READ_PHYREGFLDCEE(pi, PapdLutSel0, core, papd_lut_select_ovr_val);
        offsetOfOffset = calc_epsoffset - param_epsoffset;
        MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset, param_epsoffset);
        if (papd_lut_select == 0) {
            MOD_PHYREGCEE(pi, PapdLutSel1, core, papd_index_offset_lut0,
                offsetOfOffset + extraepsoffset);
        } else {
            MOD_PHYREGCEE(pi, PapdLutSel1, core, papd_index_offset_lut1,
                offsetOfOffset + extraepsoffset);
        }

    } else {
        if (use_calc) {
            // Overwrite the statically configured value with the one calculated by
            // gain control or the value from epsilonOffset calibration.
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset,
                calc_epsoffset + extraepsoffset);
        } else {
            // Write the statically configured value into register.
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset,
                param_epsoffset + extraepsoffset);
        }
    }
}

static void
phy_ac_papd_radio_lpbk_cleanup_majorrev51_128_130_131(phy_info_t *pi)
{
    /* restore radio config back */
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_phyregs_t *porig = (aci->papdcali->papd_phyregs);
    phy_ac_reg_cache_restore(aci, RADIOREGS_PAPDCAL);

    /* Restore phy config -part of radio loopback setup- back */
    WRITE_PHYREG(pi, RxSdFeConfig1, porig->RxSdFeConfig1);
    WRITE_PHYREG(pi, RxSdFeConfig6, porig->RxSdFeConfig6);
}

static void
phy_ac_papd_set_tia_gain_tiny(phy_info_t *pi, uint16 gain, uint8 core)
{
    /* clear bits 0-5 of the RX_BB_2G_OVR_EAST radio register */
    phy_utils_write_radioreg(pi, RADIO_REG(pi, RX_BB_2G_OVR_EAST, core),
        (READ_RADIO_REG_TINY(pi, RX_BB_2G_OVR_EAST, core) & 0xffc0));
    MOD_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain, 1);
    MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain, gain);
}

static void
wlc_phy_txpwr_papd_cal_run_acphy(phy_info_t *pi, uint8 tx_pre_cal_pwr_ctrl_state)
{
    bool suspend = TRUE, suspend_flag = FALSE;
    uint8 tx_pwr_ctrl_state;
    uint8 core;
    int16  tx_atten = 0, rx_atten = 0, i;
    uint32 eps_init_val = 0x0;
    int16 pgagain_offset = 0;
    int16 epsilonoffset = 0;
    uint8 scalar_table_ids[] = { ACPHY_TBL_ID_SCALAR0, ACPHY_TBL_ID_SCALAR1,
        ACPHY_TBL_ID_SCALAR2, ACPHY_TBL_ID_SCALAR3};
    uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
#ifdef PAPD_GAIN_CTRL
    acphy_txgains_t tx_gains;
    uint8 PAgain = 0xff;
    uint16 bbmult = 0x3f;
    uint8 Gainoverwrite = 0;
#endif /* PAPD_GAIN_CTRL */
#ifdef BCMDBG
    int16 epsregval;
#endif /* BCMDBG */
    int16 pgag = 0, eps_offset = 0;
    uint16 numIterLMS_papd = 0x10, startindex = 5, yrefindex = 5, stopindex = 63;
    acphy_txgains_t txgains[4] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
    int8 tx_idx = 20, tx_idx_pwr[PHY_CORE_MAX] = {0};
    txgain_setting_t txgain_settings;
    phy_ac_papdcal_info_t *papdi = pi->u.pi_acphy->papdcali;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

    BCM_REFERENCE(stf_shdata);

    if (papdi->acphy_papd_skip == 1)
        return;
    if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
        numIterLMS_papd = 0x10;
#ifdef PAPD_GAIN_CTRL
        bbmult = 0x30;
#endif /* PAPD_GAIN_CTRL */
    }
#ifdef PAPD_GAIN_CTRL
    if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
        numIterLMS_papd = (CHSPEC_IS2G(pi->radio_chanspec)) ? 0x20 : 0x40;
    }
    tx_gains.txlpf = 0x0;
    tx_gains.txgm = 0xff;
    tx_gains.pga = 0xff;
    tx_gains.pad = 0xff;
    phy_ac_papdcal_get_gainparams(papdi, &Gainoverwrite, &PAgain);
    tx_gains.ipa = (Gainoverwrite == 0) ? 0xff : PAgain;
    if (RADIOMAJORREV(pi) == 2 && PHY_EPAPD(pi)) {
        tx_gains.txlpf = 0x0;
        tx_gains.txgm = (CHSPEC_IS2G(pi->radio_chanspec)) ? 0x67 : 0x91;
        tx_gains.pga = 0xff;
        tx_gains.pad = (CHSPEC_IS2G(pi->radio_chanspec)) ? 0xff : 0x7f;
        tx_gains.ipa = (CHSPEC_IS2G(pi->radio_chanspec)) ? 0xc0 : 0x3;
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            startindex = 0;
            yrefindex = 0;
        }
    }
#endif /* PAPD_GAIN_CTRL */

    /* skip cal if phy is muted */
    if (PHY_MUTED(pi) || ISSIM_ENAB(pi->sh->sih))
        return;

    /* suspend mac if haven't done so */
    suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
    if (!suspend) {
        printf("MAC was not suspended before calling wlc_phy_txpwr_papd_cal_run_acphy!\n");
        suspend_flag = TRUE;
        wlapi_suspend_mac_and_wait(pi->sh->physhim);
    }
    /* Disable BT as it affects PAPD CAL */
    wlc_phy_btcx_override_enable(pi);
    /* Disable CRS */
    phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

    /* Turn off the txpowercontrol and save the txindex */
    tx_pwr_ctrl_state = pi->acphy_txpwrctrl;
    wlc_phy_txpwrctrl_enable_acphy(pi, PHY_TPC_HW_OFF);

    /* Make it work for both 4335 and 4360 */
    FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
        /* initialize scaler table */
        wlc_phy_table_write_acphy(pi, scalar_table_ids[core], 64, 0, 32,
            acphy_papd_scaltbl);
    /* Fill up epsilon table with eps_init_val = 0 */
        for (i = 0; i < 64; i++) {
            wlc_phy_table_write_acphy_dac_war(pi, epsilon_table_ids[core], 1, i, 32,
                &eps_init_val, core);
        }
    }

    /* Call power control before radio/phy set up to not mess with papd setup */
    if (PHY_EPAPD(pi) && (papdi->papdpwrctrl)) {
        FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
            tx_idx_pwr[core] = wlc_phy_tone_pwrctrl(pi, 20, core);
        }
    }

    FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
        /* acphy_papd_cal_phy_setup */
        phy_ac_papd_phy_setup(pi, core);

        /* 2069_papd_cal_setup */
        phy_ac_papd_radio_lpbk_setup_acradio(pi, tx_atten, rx_atten, core);
        pi->u.pi_acphy->txpwrindex[core] = 16;

        MOD_PHYREGCEE(pi, PapdCalShifts, core, papdYrefOverride, 0x0);
        MOD_PHYREG(pi, PapdCalYrefEpsilon, papdInitYref, 0x1);
        MOD_PHYREG(pi, PapdCalYrefEpsilon, papdEpsilonInit, 0x0);
        MOD_PHYREG(pi, PapdCalCoreSel, papdCoreSel, core); /* Specify the core */
#ifdef PAPD_GAIN_CTRL
        gain_ctrl = 1;
        /* This is needed to put analytic cal, if enabled, in gain control mode */
        if (PHY_EPAPD(pi) && (papdi->papdpwrctrl)) {
            if (tx_idx_pwr[core] != -1)
                tx_idx = tx_idx_pwr[core];
            wlc_phy_get_txgain_settings_by_index_acphy(
                pi, &txgain_settings, tx_idx);
            papd_gainctrl_pga[core] =  (txgain_settings.rad_gain_mi >> 8) & 0xff;
            tx_gains.pga = papd_gainctrl_pga[core];
            wlc_phy_txpwr_by_index_acphy(pi, (1 << core), tx_idx);
        } else {
            papd_gainctrl_pga[core] = phy_ac_papd_gain_ctrl_acradio(pi,
                numIterLMS_papd, core, startindex, yrefindex, stopindex);
            tx_gains.pga = papd_gainctrl_pga[core];
            phy_ac_papd_write_tx_gain(pi, core, &tx_gains, &bbmult, 0);
        }
        papd_gainctrl_pga[core] = phy_ac_papd_gain_ctrl_acradio(pi, numIterLMS_papd,
            core, startindex, yrefindex, stopindex);
        gain_ctrl = 0; /* Set analytic cal, if enabled, to normal mode */

#endif  /* PAPD_GAIN_CTRL */

        phy_ac_papd_cal(pi, numIterLMS_papd, core, startindex, yrefindex, stopindex);

        for (i = 0; i < startindex; i++) {
            wlc_phy_table_write_acphy_dac_war(pi, epsilon_table_ids[core], 1, i, 32,
                &eps_init_val, core);
        }

        phy_ac_get_tx_gain(pi, core, &(txgains[core]));
        papd_gainctrl_pga[core] = txgains[core].pga;
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            pgag = txgains[core].pga;
            pgagain_offset = *(rfpwrlut_ptr + pgag);
            eps_offset = -4;

        } else {
            pgag = txgains[core].pga;
            pgagain_offset = *(rfpwrlut_ptr + pgag);
            eps_offset = 0;
        }

        /* papd_index_shift in tcl-- needs to be taken care of */
        epsilonoffset = -66 + pgagain_offset + eps_offset;

        MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 8);
        if (tx_pre_cal_pwr_ctrl_state == PHY_TPC_HW_OFF) {
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset,
                epsilonoffset);
        } else {
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset, 0);
        }
#ifdef BCMDBG
        epsregval = READ_PHYREGFLDCEE(pi, EpsilonTableAdjust, core, epsilonOffset);
        PHY_PAPD((" ------------------ \n"));
        PHY_PAPD(("read epsilonTableAdjust In PAPD is %d\n", epsregval));
        PHY_PAPD((" ------------------ \n"));
#endif /* BCMDBG */

        /* save this value, in case some other pro re-init phyregs */
        papdi->acphy_papd_epsilon_offset[core] = epsilonoffset;

        /* acradio papd_cal_cleanup */
        phy_ac_papd_radio_lpbk_cleanup_acradio(pi, core);

        /* acphy_papd_cal_phy_cleanup */
        phy_ac_papd_phy_cleanup(pi, core);
        /* acphy_rfctrl_override txgain off all */
        WRITE_PHYREGCE(pi, RfctrlCoreTXGAIN1, core, 0);
        WRITE_PHYREGCE(pi, RfctrlCoreTXGAIN2, core, 0);
        WRITE_PHYREGCE(pi, Dac_gain, core, 0);
        MOD_PHYREGCE(pi, RfctrlCoreLpfGain, core, lpf_bq2_gain, 0);

        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, txgain, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideGains, core, lpf_bq2_gain, 0);

        /* acphy_papd_stats-- Some debug related thing. Not done here */
        /* acphy_papd on {0} or {0 1 2} */
    }

    /* restore tx pwr index to original power index */
    /* wlc_phy_txpwrctrl_enable_acphy(pi, PHY_TPC_HW_OFF); */
    wlc_phy_txpwrctrl_enable_acphy(pi, tx_pwr_ctrl_state);
    FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
        MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 1);
        MOD_PHYREGCEE(pi, PapdCalShifts, core, papd_calEnb, 0);
    }
    wlc_phy_papd_set_rfpwrlut(pi);
    /* Disabling BTCX Override */
    wlc_phy_btcx_override_disable(pi);
    /* Enable CRS */
    phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
    /* If the mac was suspended from this function, enable it */
    if (suspend_flag)
        wlapi_enable_mac(pi->sh->physhim);
}

/* Dump the PAPD LUT (eps table) to PHY_CAL trace */
void
wlc_phy_papd_dump_eps_trace_acphy(phy_info_t *pi, struct bcmstrbuf *b)
{
    uint8 core;
    uint16 j, eps_table_size;
    uint32 *eps_table;
    int32 eps_re, eps_im, epsOffset;
    uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    int32 multi_tbl_en = pi->u.pi_acphy->papdcali->wbpapd_multitbl_iovar;

    if (papdcal_dumpinfo == 0)
        return;

    BCM_REFERENCE(stf_shdata);

    FOREACH_ACTV_CORE(pi, stf_shdata->phytxchain, core) {
        eps_table_size = phy_ac_papdcal_eps_table_size(pi, core);
        eps_table = phy_malloc_fatal(pi, eps_table_size * sizeof(*eps_table));
        if (ACMAJORREV_129(pi->pubpi->phy_rev) &&
            READ_PHYREGFLDCEE(pi, PapdLutSel0, core, papd_lut_select_ovr)) {
            uint8 papd_lut_select = READ_PHYREGFLDCEE(pi, PapdLutSel0, core,
                    papd_lut_select_ovr_val);
            wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core],
                eps_table_size, eps_table_size*papd_lut_select, 32,
                eps_table, core);
        } else {
            wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core],
                eps_table_size, 0, 32, eps_table, core);
        }
        PHY_CAL(("core %d\n", core));
        bcm_bprintf(b, "\n  PAPD Epsilon Table  Real Image CORE %d \n", core);
        for (j = 0; j < eps_table_size; j++) {
            phy_papdcal_decode_epsilon(eps_table[j], &eps_re, &eps_im);
            if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
                /* 6710 has duplicate entries, only keep the even ones */
                if (!(j % 2)) {
                    PHY_CAL(("{%d %d} ", eps_re, eps_im));
                    bcm_bprintf(b, "{%d %d}\n", eps_re, eps_im);
                }
            } else {
                PHY_CAL(("{%d %d} ", eps_re, eps_im));
                bcm_bprintf(b, "{%d %d}\n", eps_re, eps_im);
            }
        }

        if (ACMAJORREV_130(pi->pubpi->phy_rev) && multi_tbl_en) {
            // Dump the 2nd table.
            wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core],
                eps_table_size, eps_table_size, 32, eps_table, core);
            for (j = 0; j < eps_table_size; j++) {
                phy_papdcal_decode_epsilon(eps_table[j], &eps_re, &eps_im);
                /* 6715 has duplicate entries, only keep the even ones */
                if (!(j % 2)) {
                    PHY_CAL(("{%d %d} ", eps_re, eps_im));
                    bcm_bprintf(b, "{%d %d}\n", eps_re, eps_im);
                }
            }
        }
        phy_mfree(pi, eps_table, eps_table_size * sizeof(*eps_table));
        bcm_bprintf(b, "papdcalidx core%d %d\n", core, CHSPEC_IS2G(pi->radio_chanspec) ?
            papd_gainctrl_calidx_2g[core] : papd_gainctrl_calidx_5g[core]);
        bcm_bprintf(b, "papdbbmult core%d %d\n", core, papd_gainctrl_bbmult[core]);
        if (CHSPEC_IS5G(pi->radio_chanspec) && ACMAJORREV_129(pi->pubpi->phy_rev)) {
            /* Initial TIA gain search result */
            bcm_bprintf(b, "papdtiagain 1st core%d %d\n", core,
                    papd_tiagain_1st[core]);
        }
        bcm_bprintf(b, "papdtiagain core%d %d\n", core, papd_tiagain[core]);
        epsOffset = READ_PHYREGFLDCEE(pi, EpsilonTableAdjust, core, epsilonOffset);
        bcm_bprintf(b, "papdepsilonOffset core%d %d\n", core, (epsOffset >= 256) ?
            epsOffset - 512 : epsOffset);

        if (ACMAJORREV_130_131(pi->pubpi->phy_rev) && WBPAPD_ENAB(pi)) {
            bcm_bprintf(b, "PAPD pre-filter coefficients core%d\n", core);
            for (j = 0; j < 7; j++) {
                bcm_bprintf(b, "w_i%d = 0x%x\n", j,
                    papd_prefilter_coeff_i[core][j]);
                bcm_bprintf(b, "w_q%d = 0x%x\n", j,
                    papd_prefilter_coeff_q[core][j]);
            }
        }
        PHY_CAL(("\n"));
    }
    PHY_CAL(("\n"));
}

static void
phy_ac_papd_smooth(phy_info_t *pi, uint8 core, uint32 winsz, uint32 start, uint32 end)
{
    uint32 *buf, *src, *dst, sz;
    uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2};
    uint16 eps_table_size = phy_ac_papdcal_eps_table_size(pi, core);
    PHY_CAL(("Smoothing papd cal on core: %d\n", core));

    sz = end - start + 1;
    ASSERT(end > start);
    ASSERT(end < eps_table_size);

    /* Allocate storage for both source & destination tables */
    buf = phy_malloc_fatal(pi, 2 * sizeof(*buf) * eps_table_size);

    /* Setup source & destination pointers */
    src = buf;
    dst = buf + eps_table_size;

    /* Read original table */
    wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core], eps_table_size,
            0, 32, src, core);

    /* Average coeffs across window */
    do {
        uint32 win_start, win_end;
        int32 nAvr, eps_r, eps_i, eps_real, eps_imag;

        win_start = end - MIN(end, (winsz >> 1));
        win_end = MIN(eps_table_size - 1, end + (winsz >> 1));
        nAvr = win_end - win_start + 1;
        eps_real = 0;
        eps_imag = 0;

        do {
            phy_papdcal_decode_epsilon(src[win_end], &eps_r, &eps_i);
            eps_real += eps_r;
            eps_imag += eps_i;
        } while (win_end-- != win_start);

        eps_real /= nAvr;
        eps_imag /= nAvr;
        dst[end] = ((uint32)eps_imag << 13) | ((uint32)eps_real & 0x1fff);
    } while (end-- != start);

    /* Write updated table */
    wlc_phy_table_write_acphy_dac_war(pi, epsilon_table_ids[core], sz, start, 32, dst, core);

    /* Free allocated buffer */
    phy_mfree(pi, buf, 2 * sizeof(*buf) * eps_table_size);
}

static void
BCMATTACHFN(phy_ac_papdcal_nvram_attach_old)(phy_ac_papdcal_info_t *papdcal_info)
{
    uint8 core;
    phy_info_t *pi = papdcal_info->pi;

    pi->papdcali->data->epacal2g = (uint8) (PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_epacal2g, 0));
    pi->papdcali->data->epacal5g = (uint8) (PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_epacal5g, 0));
    pi->papdcali->data->epacal2g_mask = (uint16) (PHY_GETINTVAR_DEFAULT_SLICE(pi,
        rstr_epacal2g_mask, 0x3fff));
    pi->papdcali->data->epacal2g |= (pi->epagain2g >> 2) & 1;
    pi->papdcali->data->epacal5g |= (pi->epagain5g >> 2) & 1;

    /* Skip PAPD cal in WD before doing other cals */
    pi->skip_wdpapd = TRUE;
    /* Default value for forced papd cal index, bbmult, extra epsilon offset, tia gain */
    papdcal_info->acphy_papd_skip = 0;
    papdcal_info->pacalidx_iovar = -1;
    papdcal_info->papdbbmult_iovar = -1;
    papdcal_info->extraepsoffset_iovar = 0;
    papdcal_info->papdtiagain_iovar = -1;
    papdcal_info->comp_disable_iovar = -1;
    papdcal_info->end_epstblidx_iovar = -1;
    papdcal_info->epstblsel_iovar = 0;
    papdcal_info->papd_abort = 0; /* Default: do complete PAPD cal everytime */
    papdcal_info->fullcal = TRUE;
    papdcal_info->papdmode = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_papdmode, PAPD_LMS);
    papdcal_info->wbpapd_gctrl_iovar = ACMAJORREV_131(pi->pubpi->phy_rev) ? 0 : 2;
    papdcal_info->wbpapd_multitbl_iovar = 0;
#if defined(BCMDBG)
    papdcal_dumpinfo = 1;
#else
    papdcal_dumpinfo = 0;
#endif

    FOREACH_CORE(pi, core) {
        papd_gainctrl_bbmult[core] = -1;
        papd_channel[core] = -1;
    }

    if ((PHY_GETVAR_SLICE(pi, rstr_pagc2g)) != NULL) {
        papdcal_info->srom_pagc2g = (uint8)PHY_GETINTVAR_SLICE(pi, rstr_pagc2g);
        papdcal_info->srom_pagc2g_ovr = 0x1;
    } else {
        papdcal_info->srom_pagc2g = 0xff;
        papdcal_info->srom_pagc2g_ovr = 0x0;
    }

    if ((PHY_GETVAR_SLICE(pi, rstr_pagc5g)) != NULL) {
        papdcal_info->srom_pagc5g = (uint8)PHY_GETINTVAR_SLICE(pi, rstr_pagc5g);
        papdcal_info->srom_pagc5g_ovr = 0x1;
    } else {
        papdcal_info->srom_pagc5g = 0xff;
        papdcal_info->srom_pagc5g_ovr = 0x0;
    }

#if (defined(WLTEST) || defined(WLPKTENG))
    /* Read the per rate dpd enable param */
    papdcal_info->perratedpd2g = (bool)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_perratedpd2g, 0);
    papdcal_info->perratedpd5g = (bool)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_perratedpd5g, 0);
#endif
}

/* ***************************** */
/*        External Definitions            */
/* ***************************** */

void
wlc_phy_papd_set_rfpwrlut(phy_info_t *pi)
{
    int16 epsilonoffset, pga_gain;
#ifdef BCMDBG
    int16 epsregval;
#endif /* BCMDBG */
    uint8 idx, core = 0;
    txgain_setting_t txgain_settings;
    uint16 channel = 5180;
    int8 epscaldelta = 0, eps_offset = 0, delta = 0;
    uint32 rfpwrlut_table_ids[] = { ACPHY_TBL_ID_RFPWRLUTS0,
        ACPHY_TBL_ID_RFPWRLUTS1, ACPHY_TBL_ID_RFPWRLUTS2};
    uint8 subbandidx = 0;
    phy_ac_papdcal_info_t *papdi = pi->u.pi_acphy->papdcali;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

    BCM_REFERENCE(stf_shdata);

    PHY_PAPD((" ------- In RFPWRLUT ------- \n"));

    FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
        if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
            /* 4354A0 */
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                rfpwrlut_ptr = pga_gain_array_2g_4354;
                if ((RADIOREV(pi->pubpi->radiorev) == 0x27 ||
                     RADIOREV(pi->pubpi->radiorev) == 0x29 ||
                     RADIOREV(pi->pubpi->radiorev) == 0x28 ||
                     RADIOREV(pi->pubpi->radiorev) == 0x2C) &&
                    (PHY_XTAL_IS40M(pi))) {
                    eps_offset = 18; /* 43569 */
                } else {
                    eps_offset = 19;
                }
            } else {
                if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) &&
                    RADIOREV(pi->pubpi->radiorev) == 0x28 &&
                    PHY_XTAL_IS40M(pi)) {
                    /* 43566/43567/43569/43570 A0 */
                    rfpwrlut_ptr = pga_gain_array_5g_435x_radiorev40;
                    eps_offset = 3;
                } else if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) &&
                           RADIOREV(pi->pubpi->radiorev) == 0x2C &&
                           PHY_XTAL_IS40M(pi)) {
                    channel = CHSPEC_CHANNEL(pi->radio_chanspec);

                    /* 43567/43570 A2 */
                    if (CST4350_IFC_MODE(pi->sh->sih->chipst) ==
                        CST4350_IFC_MODE_PCIE) {
                        rfpwrlut_ptr = pga_gain_array_5g_435x_radiorev40;
                        if (CHSPEC_IS40(pi->radio_chanspec)) {
                            eps_offset = 3;

                        } else {
                            eps_offset = 4;
                        }
                    } else {
                        /* 43566/43569 A2 */
                        rfpwrlut_ptr = pga_gain_array_5g_435x_radiorev44;
                        if (CHSPEC_IS80(pi->radio_chanspec)) {
                            if (core == 0) {
                                eps_offset = 4;
                            } else {
                                eps_offset = 5;
                            }
                        } else {
                            if (core == 0) {
                                eps_offset = 3;
                            } else {
                                eps_offset = 4;
                            }
                        }
                    }
                } else {
                    rfpwrlut_ptr = pga_gain_array_5g_4354;
                    if (core == 0)
                        eps_offset = 2;
                    else if (core == 1)
                        eps_offset = 3;
                }
            }
            if (RADIOMAJORREV(pi) == 2 && PHY_EPAPD(pi)) {
                /* 4350EPAPD */
                if (CHSPEC_IS2G(pi->radio_chanspec)) {
                    rfpwrlut_ptr = pga_gain_array_2g_epapd[core];
                    if (core == 0)
                        eps_offset = -3;
                    else
                        eps_offset = -6;
                } else {
                    channel = CHSPEC_CHANNEL(pi->radio_chanspec);
                    channel = 5000 + 5 * channel;
                    eps_offset = 3;
                    /* pacalshift5gaX=lo,mi,hi,x1,lo,mi,hi,x1,lo,mi,hi,x1
                     *                |    20mhz |   40mhz   |   80mhz   |
                     */
                    if (channel >= 5180 && channel <= 5320) {
                        rfpwrlut_ptr = pga_gain_array_5g_epapd_0;
                        subbandidx = 0;
                    } else if (channel >= 5500 && channel <= 5620) {
                        rfpwrlut_ptr = pga_gain_array_5g_epapd_1;
                        subbandidx = 1;
                    } else if (channel >= 5630 && channel <= 5700) {
                        rfpwrlut_ptr = pga_gain_array_5g_epapd_2;
                        subbandidx = 2;
                    } else if (channel >= 5710 && channel <= 5825) {
                        rfpwrlut_ptr = pga_gain_array_5g_epapd_3;
                        subbandidx = 3;
                    }
                    if (CHSPEC_IS20(pi->radio_chanspec))
                        eps_offset += (core == 0) ?
                            papdi->pacalshift5ga0[0 + subbandidx] :
                            papdi->pacalshift5ga1[0 + subbandidx];
                    else if (CHSPEC_IS40(pi->radio_chanspec))
                        eps_offset += (core == 0) ?
                            papdi->pacalshift5ga0[4 + subbandidx] :
                            papdi->pacalshift5ga1[4 + subbandidx];
                    else if (CHSPEC_IS80(pi->radio_chanspec))
                        eps_offset += (core == 0) ?
                            papdi->pacalshift5ga0[8 + subbandidx] :
                            papdi->pacalshift5ga1[8 + subbandidx];
                }
            }
        }

        for (idx = 0; idx < 128; idx++)
        {
            wlc_phy_get_txgain_settings_by_index_acphy(pi, &txgain_settings, idx);
            pga_gain =  (txgain_settings.rad_gain_mi >> 8) & 0xff;
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
                epscaldelta = (int8)*(rfpwrlut_ptr+papd_gainctrl_pga[core])
                    - (int8)*(rfpwrlut_ptr + 167);
            } else {
                epscaldelta = (int8)*(rfpwrlut_ptr+papd_gainctrl_pga[core])
                    - (int8)*(rfpwrlut_ptr + 71);
            }

            epsilonoffset = (-66 + (int8)*(rfpwrlut_ptr + pga_gain)
                - epscaldelta + eps_offset + delta) << 1;

            wlc_phy_table_write_acphy(pi, rfpwrlut_table_ids[core], 1, idx,
                16, &epsilonoffset);
        }
        MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset, 0);
#ifdef BCMDBG
        epsregval = READ_PHYREGFLDCEE(pi, EpsilonTableAdjust, core, epsilonOffset);
        PHY_PAPD((" ------------------ \n"));
        PHY_PAPD(("read epsilonTableAdjust In RF PowerLUT is %d\n", epsregval));
        PHY_PAPD((" ------------------ \n"));
#endif /* BCMDBG */
    }
}

void
wlc_phy_papd_set_rfpwrlut_phymaj_rev36(phy_info_t *pi)
{
    ASSERT(0); // this function is not supposed to get called
}

void
wlc_phy_papd_set_rfpwrlut_tiny(phy_info_t *pi)
{
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_info_t *papdcal_info = aci->papdcali;
    int16 radiogainqdb;
    uint8 idx;
    uint16 txgain[3], bbmult;
    int16 temp, temp1, temp2, qQ, qQ1, qQ2, shift;
    int16 _2xidx, _27minus2xidx, _5timestemp, _offset_for_round, _80logbbmultdiv64;
    uint8 scale_factor = 1;
    int8 papd_rf_pwr_scale = 32; /* Q5 format */
    int32 val = 0;
    uint8 tx_gain_tbl_id, core;
    uint8 rfpwrlut_table_ids[] = { ACPHY_TBL_ID_RFPWRLUTS0,
        ACPHY_TBL_ID_RFPWRLUTS1, ACPHY_TBL_ID_RFPWRLUTS2, ACPHY_TBL_ID_RFPWRLUTS3};

    if (CHSPEC_IS2G(pi->radio_chanspec) && (papdcal_info->parfps2g != -1)) {
        papd_rf_pwr_scale = papdcal_info->parfps2g;
    } else if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) && (papdcal_info->parfps5g != -1)) {
        papd_rf_pwr_scale = papdcal_info->parfps5g;
    }

    /* acphy_beDeaf??? */
    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

    FOREACH_CORE(pi, core) {
        for (idx = 0; idx < 128; idx++)  {
            tx_gain_tbl_id = wlc_phy_get_tbl_id_gainctrlbbmultluts(pi, core);
            wlc_phy_table_read_acphy(pi, tx_gain_tbl_id, 1, idx, 48, &txgain);
            bbmult = (txgain[0] & 0xff);

            qm_log10((int32)(bbmult), 0, &temp1, &qQ1);
            qm_log10((int32)(1<<6), 0, &temp2, &qQ2);

            if (qQ1 < qQ2) {
                temp2 = qm_shr16(temp2, qQ2-qQ1);
                qQ = qQ1;
            } else {
                temp1 = qm_shr16(temp1, qQ1-qQ2);
                qQ = qQ2;
            }
            temp = qm_sub16(temp1, temp2);

            if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
                    ACMAJORREV_128(pi->pubpi->phy_rev)) {
                /* Normalized BB Mult output in log10 format is multiplied by 16 */
                shift = qQ-4;
                /* Assuming half dB steps, equalling 2 qdB per idx */
                _2xidx = qm_shl16(idx, 1);
                /* offsetting RF PWR LUT by 27 */
                _27minus2xidx = qm_sub16(27, _2xidx);
                /* Multiplying by 80 is split into 16 and 5 */
                _5timestemp = temp * 5;
                /* add 0.5 * 2^shift = 2^(shift-1), to obtain round */
                _offset_for_round = qm_shl16(1, shift-1);
                /* 5 * 16 * log10 = 80 log10. */
                _80logbbmultdiv64 = qm_shr16(_5timestemp + _offset_for_round,
                    shift);
                radiogainqdb = qm_sub16(_27minus2xidx, _80logbbmultdiv64);
            } else {
                if (qQ >= 4)
                    shift = qQ-4;
                else
                    shift = 4-qQ;

                val = ((((idx*papd_rf_pwr_scale/32) << shift) + (5*temp) +
                (1<<(scale_factor+shift-3)))>>(scale_factor+shift-2));

                radiogainqdb = -(val)/2;
            }
            /* No need of iteration delays for 4349 family */
            if (!ACMAJORREV_4(pi->pubpi->phy_rev)) {
                /* adding 10us of delay as table_write is throwing assert */
                OSL_DELAY(10);
            }
            wlc_phy_table_write_acphy(pi, rfpwrlut_table_ids[core], 1, idx,
                16, &radiogainqdb);
        }
    }
    phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
    wlapi_enable_mac(pi->sh->physhim);
}

static void
phy_ac_papd_phy_cleanup(phy_info_t *pi, uint8 core)
{

    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    acphy_rxcal_phyregs_t *porig = (pi_ac->ac_rxcal_phyregs_orig);
    uint8 stall_val;

    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);

    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    ASSERT(porig->is_orig);
    if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
        /* FIXME: Need to make this generic fix. Checking in urgently because of UTF failure
         * JIRA: SWWLAN-121435
         */
        /* Will hit assert for second core onwards without below check */
        if (core == PHYCORENUM((pi)->pubpi->phy_corenum)-1)
            porig->is_orig = FALSE;
    } else {
        porig->is_orig = FALSE;
    }

    WRITE_PHYREG(pi, RfseqCoreActv2059, porig->RfseqCoreActv2059);

    WRITE_PHYREG(pi, RfctrlCoreGlobalPus, porig->RfctrlCoreGlobalPus);
    WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, porig->RfctrlOverrideGlobalPus);

/* CAL PER CORE - FOREACH_ACTV_CORE(pi, phy_stf_get_data(pi->stfi)->phyrxchain, core) */
    {

        /* Are the following three statements redundant?
            wlc_phy_txpwr_by_index_acphy would overwrite on the following anyway
        */
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x100 + core), 16,
            &porig->rfseq_txgain[core + 0]);
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x103 + core), 16,
            &porig->rfseq_txgain[core + 3]);
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x106 + core), 16,
            &porig->rfseq_txgain[core + 6]);
        wlc_phy_txpwr_by_index_acphy(pi, (1 << core), porig->txpwridx[core]);
        wlc_phy_set_tx_bbmult_acphy(pi, &(porig->bbmult[core]), core);

        /* Restore papd cal phy registers */
        acphy_papd_cal_phyreg_sr(pi, core, porig, 0);

    }
    wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RESET2RX);
    if (!TINY_RADIO(pi)) {
        WRITE_PHYREG(pi, lbFarrowCtrl, porig->lbFarrowCtrl);
    }

    ACPHY_ENABLE_STALL(pi, stall_val);
}

static void
phy_ac_papd_phy_cleanup_majorrev51_128_129_130_131(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    acphy_rxcal_phyregs_t *porig = (pi_ac->ac_rxcal_phyregs_orig);
    uint8 stall_val, core;
    const uint16 rfseq[] = { 0x100, 0x101, 0x102, 0x501 };

    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);

    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    ASSERT(porig->is_orig);
    porig->is_orig = FALSE;

    if ((ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) &&
        WBPAPD_ENAB(pi)) {
        /* Restore farrow */
        wlc_phy_tx_farrow_setup_28nm(pi, 1 /* DAC rate mode */, 0 /* wbcal */);
    }
    /* Restore the PHY registers modified during setup */
    phy_ac_reg_cache_restore(pi_ac, PHYREGS_PAPDCAL);

    WRITE_PHYREG(pi, RfseqCoreActv2059, porig->RfseqCoreActv2059);

    if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        WRITE_PHYREG(pi, RxSdFeConfig1, porig->RxSdFeConfig1);
        WRITE_PHYREG(pi, RxSdFeConfig6, porig->RxSdFeConfig6);
    }

    FOREACH_CORE(pi, core) {
        /* Restore the gains */
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]), 16,
            &porig->rfseq_txgain[core * 3 + 0]);
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]+3), 16,
            &porig->rfseq_txgain[core * 3 + 1]);
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]+6), 16,
            &porig->rfseq_txgain[core * 3 + 2]);
        wlc_phy_txpwr_by_index_acphy(pi, (1 << core), porig->txpwridx[core]);
        wlc_phy_set_tx_bbmult_acphy(pi, &(porig->bbmult[core]), core);
    }

    wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RESET2RX);

    ACPHY_ENABLE_STALL(pi, stall_val);
}

static void
phy_ac_papd_cal(phy_info_t *pi, uint16 num_iter, uint8 core, uint16 startindex,
    uint16 yrefindex, uint16 stopindex)
{
    uint16 bbmult;
    uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
    acphy_papdCalParams_t calParams = {core, startindex, stopindex, yrefindex,
            epsilon_table_ids[core], num_iter, 4, 256};
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)pi->u.pi_acphy->papdcali;
    uint8 calmode = 0;
    int32 end_epstblidx_orig = papdcali->end_epstblidx_iovar, first_cal = 1;
    uint32 copyvalue = 0;
    uint16 j, tbl_size = phy_ac_papdcal_eps_table_size(pi, core);

    PHY_PAPD(("\nPAPD Main Cal .. \n"));

    /* Initialize gainctrl with old bbmult if papd_abort is enabled */
    if (papdcali->papd_abort == 1) {
        if (papd_channel[core] != CHSPEC_CHANNEL(pi->radio_chanspec) ||
            papdcali->fullcal) {
            /* Reset PAPD cal if channel changes or full cal */
            papd_gainctrl_bbmult[core] = -1;
        }
        if (papd_gainctrl_bbmult[core] != -1) {
            bbmult = papd_gainctrl_bbmult[core];
            first_cal = 0;
        } else {
            papdcali->end_epstblidx_iovar = -1; /* first-time cal is full cal */
            bbmult = 64;
        }
    } else {
        bbmult = 64;
    }
    papd_channel[core] = CHSPEC_CHANNEL(pi->radio_chanspec);

    /* Use phy_ac_papd_cal_set_tx_gain for NBPAPD cal to execute bbmult gainctrl and set tx_idx
     * and bbmult. For WBPAPD cal, set tx_idx and bbmult inside phy_ac_wbcal_run.
     */
    if (!WBPAPD_ENAB(pi)) {
        /* Enable TIA gain search for 5G only */
        if (CHSPEC_IS5G(pi->radio_chanspec) && ACMAJORREV_129(pi->pubpi->phy_rev)) {
            /* TIA gain search for PAPD cal. Initial estimation, since papd
             * cal_bbmult_final is not yet determined.
             */
            phy_ac_papd_tiagain_search_majrev129(pi, core, 192, FALSE);

            /* Setting tx gain for all tx cores */
            phy_ac_papd_cal_set_tx_gain(pi, core, &bbmult, &calmode);

            /* TIA gain search for PAPD cal. Second try done after
             * axphy_papd_tx_bbmult_ctrl is done.
             */
            phy_ac_papd_tiagain_search_majrev129(pi, core, bbmult, TRUE);
        } else {
            /* Do gainctrl only when papd_abort is disabled or first-time/full cal */
            if (papdcali->papd_abort == 0 || first_cal == 1 || papdcali->fullcal) {
                /* Setting tx gain for all tx cores */
                phy_ac_papd_cal_set_tx_gain(pi, core, &bbmult, &calmode);
            }
            /* For dumping TIA gain */
            papd_tiagain[core] = (CHSPEC_IS2G(pi->radio_chanspec)) ?
                papdcali->papd_params->tia_mode_2g
                : papdcali->papd_params->tia_mode_5g;
        }
    }

    /* Selecting PAPD cal mode */
    if (APAPD_ENAB(papdcali)) {
        /* TCL PAPDCAL Mode 3 - Analytic Cal */
        phy_ac_papd_cal_mode3(pi, &calParams);
    } else if (WBPAPD_ENAB(pi)) {
        phy_ac_wbcal_run(pi, core);
    } else {
        if (papdcali->papd_abort == 0 || first_cal == 1 || papdcali->fullcal) {
            /* TCL PAPDCAL Mode 0/1 - LMS Cal */
            phy_ac_papd_cal_mode0_1(pi, &calParams, &calmode);
            papdcali->end_epstblidx_iovar = end_epstblidx_orig;

            /* Save first-time cal (0th) eps table to 2nd eps table */
            for (j = 0; j < tbl_size; j++) {
                /* 6710: Odd epsilon table entries repeat even ones */
                wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core],
                    1, j, 32, &copyvalue, core);
                wlc_phy_table_write_acphy_dac_war(pi, epsilon_table_ids[core],
                    1, j + 2*tbl_size, 32, &copyvalue, core);
            }
        } else {
            uint8 coremask = pi->pubpi->phy_coremask, coremask_idx = 1 << core, tx_idx;
            uint16 m[4] = {0, 0, 0, 0};

            /* Abort PAPD cal for high epsilon indices to limit max power */
            if (papdcali->end_epstblidx_iovar != -1) {
                calParams.stopindex = papdcali->end_epstblidx_iovar;
            } else {
                calParams.stopindex = 35;
            }
            /* PAPD cal gain setting */
            tx_idx = (CHSPEC_IS2G(pi->radio_chanspec)) ?
                papdcali->papd_params->papd_calidx_2g :
                papdcali->papd_params->papd_calidx_5g;
            wlc_phy_txpwr_by_index_acphy(pi, coremask_idx, tx_idx);
            m[core] = bbmult;
            wlc_phy_ipa_set_bbmult_acphy(pi, &m[0], &m[1], &m[2], &m[3],
                                         coremask);

            /* TCL PAPDCAL Mode 0/1 - LMS Cal */
            phy_ac_papd_cal_mode0_1(pi, &calParams, &calmode);

            /* Restore high index entries from first-cal (2nd) eps tbl */
            if (calParams.stopindex < 63) {
                for (j = calParams.stopindex+1; j < 64; j++) {
                    /* 6710: Odd epsilon table entries repeat even ones */
                    wlc_phy_table_read_acphy_dac_war(pi,
                        epsilon_table_ids[core],
                        1, 2*j + 2*tbl_size, 32, &copyvalue, core);
                    wlc_phy_table_write_acphy_dac_war(pi,
                        epsilon_table_ids[core],
                        1, 2*j, 32, &copyvalue, core);
                    wlc_phy_table_write_acphy_dac_war(pi,
                        epsilon_table_ids[core],
                        1, 2*j+1, 32, &copyvalue, core);
                }
            }
        }
        papd_gainctrl_bbmult[core] = bbmult;
    }

    /* Calculate epsilon offset, for tiny radio */
    /* papd_gainctrl_bbmult stands for the final bbmult used in PAPD cal */
    bbmult = papd_gainctrl_bbmult[core];
    if (TINY_RADIO(pi)) {
        phy_ac_papd_cal_eps_calc_tiny(pi, core, &bbmult);
    } else if (ACMAJORREV_51_129_131(pi->pubpi->phy_rev) ||
            ACMAJORREV_128(pi->pubpi->phy_rev)) {
        phy_ac_papd_cal_eps_calc_tiny(pi, core, &bbmult);
        if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            WRITE_PHYREGCE(pi, PapdLutSel0, core, 1); /* disable LUT Sel */
        }
    }
}

/* PAPD Functions */
static void
phy_ac_papd_phy_setup(phy_info_t *pi, uint8 core)
{
    /* XXX Notes:
     *   - also note that in the driver we do a resetCCA after this to be on the safe
     *     side; may want to revisit this here, too, in case we run into issues
     */

    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    acphy_rxcal_phyregs_t *porig = (pi_ac->ac_rxcal_phyregs_orig);
    uint16 sdadc_config = 0;
    uint8 bw_idx;
    uint8 stall_val;
    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);
    if (CHSPEC_IS80(pi->radio_chanspec)) {
        bw_idx = 2;
        sdadc_config = sdadc_cfg80;
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

    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    ASSERT(!porig->is_orig);
    if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
        /* FIXME: Need to make this generic fix. Checking in urgently because of UTF failure
         * JIRA: SWWLAN-121435
         */
        /* Will hit assert for second core onwards without below check */
        if (core == PHYCORENUM((pi)->pubpi->phy_corenum)-1)
            porig->is_orig = TRUE;
    } else {
        porig->is_orig = TRUE;
    }
    /* 4335 phy_rev is 7. For phy_rev 2, following things are required. */
    if (ACREV_IS(pi->pubpi->phy_rev, 2)) {
        ACPHY_REG_LIST_START
            MOD_PHYREG_ENTRY(pi, RxFeCtrl1, rxfe_bilge_cnt, 4)
            MOD_PHYREG_ENTRY(pi, RxFeCtrl1, soft_sdfeFifoReset, 1)
            MOD_PHYREG_ENTRY(pi, RxFeCtrl1, soft_sdfeFifoReset, 0)
        ACPHY_REG_LIST_EXECUTE(pi);
    }

    porig->RfctrlOverrideGlobalPus = READ_PHYREG(pi, RfctrlOverrideGlobalPus);
    porig->RfctrlCoreGlobalPus = READ_PHYREG(pi, RfctrlCoreGlobalPus);

    /* CAL PER CORE - FOREACH_ACTV_CORE(pi, phy_stf_get_data(pi->stfi)->phyrxchain, core) */
    {
        porig->txpwridx[core] = pi->u.pi_acphy->txpwrindex[core];

        /* Save papd cal phy registers */
        acphy_papd_cal_phyreg_sr(pi, core, porig, 1);

        wlc_phy_get_tx_bbmult_acphy(pi, &(porig->bbmult[core]), core);
        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x100 + core), 16,
            &porig->rfseq_txgain[core+0]);
        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x103 + core), 16,
            &porig->rfseq_txgain[core+3]);
        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x106 + core), 16,
            &porig->rfseq_txgain[core+6]);

    }

    /* RfseqCoreActv2059 is core indepedant and will be modified below hence backing
       up during core 0
     */
    if (core == 0)
        porig->RfseqCoreActv2059 = READ_PHYREG(pi, RfseqCoreActv2059);

    /* XXX Core Activate/Deactivate
        for now, keep all rx's enabled for most realistic rx conditions
     */
    /* MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, phy_stf_get_data(pi->stfi)->phyrxchain); */
    MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, (ACMAJORREV_4(pi->pubpi->phy_rev) ? 7 : 1 << core));
    MOD_PHYREG(pi, RfseqCoreActv2059, DisRx, (ACMAJORREV_4(pi->pubpi->phy_rev) ? 7 :
        ~(1 << core)));
    if (!TINY_RADIO(pi)) {
        porig->lbFarrowCtrl = READ_PHYREG(pi, lbFarrowCtrl);
        MOD_PHYREG(pi, lbFarrowCtrl, lb_decimator_output_shift, 2);
    }

/* CAL PER CORE - FOREACH_ACTV_CORE(pi, phy_stf_get_data(pi->stfi)->phyrxchain, core) */
    {

        /* XXX RF External Settings
         *   - Power Down External PA,
         *   - T/R on T to protect against interference
         */
        /* acphy_rfctrlintc_override  ext_pa 1  $core */
        MOD_PHYREGCE(pi, RfctrlIntc, core, ext_2g_papu, 1);
        MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 1);
        MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);

        /* acphy_rfctrlintc_override  ext_lna_pu 0  $core */
        MOD_PHYREGCE(pi, RfctrlIntc, core, ext_lna_2g_pu, 0);
        MOD_PHYREGCE(pi, RfctrlIntc, core, ext_lna_5g_pu, 0);
        MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_lna, 1);

        MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, 1);
        MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, 0);
        MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 1);

        /* XXX Required for loopback to work correctly
           acphy_rfctrl_override fast_nap_bias_pu 1 $core
        */
        /* XXX RfCtrl
        *   - turn off Internal PA
        *   - turn off LNA1 to protect against interference and reduce thermal noise
        *   - force LPF to Rx Chain
        *   - force LPF bw
        *   - NOTE: this also saves off state of possible Tx/Rx gain override states
        */

        /* Setting the SD-ADC related stuff */
        /* acphy_rfctrl_override iqadc 1 $core */
        wlc_phy_txcal_phy_setup_acphy_core_sd_adc(pi, core, sdadc_config);
        /* acphy_rfctrl_override pa_pwrup 1 $core; */
        MOD_PHYREGCE(pi, RfctrlCoreTxPus, core, pa_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, pa_pwrup, 1);
        /* acphy_rfctrl_override lpf_nrssi_pwrup 0 $core */
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, lpf_nrssi_pwrup, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, lpf_nrssi_pwrup, 1);
        /* acphy_rfctrl_override wb1_pu 0 $core */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rssi_wb1g_pu, 0);
            MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rssi_wb1g_pu, 1);
        } else {
            MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rssi_wb1a_pu, 0);
            MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rssi_wb1a_pu, 1);
        }

        /* xxx Debug printfs:
        printf("\nRfctrlCoreRxPus = %x", READ_PHYREG(pi, RfctrlCoreRxPus0));
        printf("\nRfctrlCoreTxPus = %x", READ_PHYREG(pi, RfctrlCoreTxPus0));
        */

        /* xxx This is what is done in tcl:
        acphy_rfctrl_override lpf_bq1_bw [expr $def(phybw) + 0] $core;
        */
        MOD_PHYREGCE(pi, RfctrlOverrideLpfCT, core, lpf_bq1_bw, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfCT, core, lpf_bq1_bw, bw_idx);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfCT, core, lpf_bq2_bw, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideLpfCT, core, lpf_rc_bw, 1);
        if ((RADIOREV(pi->pubpi->radiorev) == 17) ||
            (RADIOREV(pi->pubpi->radiorev) == 23) ||
        (RADIOREV(pi->pubpi->radiorev) == 25)) {
            MOD_PHYREGCE(pi, RfctrlCoreLpfCT, core, lpf_bq2_bw, 7);
            MOD_PHYREGCE(pi, RfctrlCoreRCDACBuf, core, lpf_rc_bw, 7);
        } else {
            MOD_PHYREGCE(pi, RfctrlCoreLpfCT, core, lpf_bq2_bw, bw_idx + 5);
            MOD_PHYREGCE(pi, RfctrlCoreRCDACBuf, core, lpf_rc_bw, bw_idx + 5);
        }

        MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 0);

        if (ACMAJORREV_2(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev) ||
            ACMAJORREV_4(pi->pubpi->phy_rev)) {
            MOD_PHYREGCE(pi, forceFront, core, freqEst, 1);
            MOD_PHYREGCE(pi, forceFront, core, freqCor, 1);
        }
    }

    ACPHY_ENABLE_STALL(pi, stall_val);
}

static void
phy_ac_papd_phy_setup_majorrev51_128_129_130_131(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    acphy_rxcal_phyregs_t *porig = (pi_ac->ac_rxcal_phyregs_orig);
    uint8 core;
    uint8 stall_val;
    const uint16 rfseq[] = { 0x100, 0x101, 0x102, 0x501 };
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_params_t *params    = aci->papdcali->papd_params;
    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);
    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    ASSERT(!porig->is_orig);
    porig->is_orig = TRUE;

    /* Save the PHY override registers for papd cal */
    porig->RfseqCoreActv2059 = READ_PHYREG(pi, RfseqCoreActv2059);

    if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        // Core-independent registers cannot be backed up by phy_ac_reg_cache_save.
        porig->RxSdFeConfig1 = READ_PHYREG(pi, RxSdFeConfig1);
        porig->RxSdFeConfig6 = READ_PHYREG(pi, RxSdFeConfig6);
    }

    phy_ac_reg_cache_save(pi_ac, PHYREGS_PAPDCAL);
    MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, 15);
    MOD_PHYREG(pi, RfseqCoreActv2059, DisRx, 0);
    FOREACH_CORE(pi, core) {
        wlc_phy_get_tx_bbmult_acphy(pi, &(porig->bbmult[core]), core);
        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]), 16,
            &porig->rfseq_txgain[core * 3]);
        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]+3), 16,
            &porig->rfseq_txgain[core * 3 + 1]);
        wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (rfseq[core]+6), 16,
            &porig->rfseq_txgain[core * 3 + 2]);

        /* Band Specific register settings */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            MOD_PHYREGCE(pi, RfctrlIntc, core, ext_2g_papu, 1);
            MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, ext_lna_2g_pu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, ext_lna_5g_pu, 0);
            /* wb1_pu */
            MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rssi_wb1g_pu, 0);
            MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rssi_wb1g_pu, 1);
        } else {
            MOD_PHYREGCE(pi, RfctrlIntc, core, ext_2g_papu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 1);
            MOD_PHYREGCE(pi, RfctrlIntc, core, ext_lna_2g_pu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, ext_lna_5g_pu, 0);
            /* wb1_pu */
            MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rssi_wb1a_pu, 0);
            MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rssi_wb1a_pu, 1);
            MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_5G_pwrup, 1);
            MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_5G_pwrup, 1);
        }

        MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);
        MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_lna, 1);
        MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, 1);
        MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, 0);
        MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 1);

        /* Power up Rx chain */
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_pwrup, 1);
        /* Power up LOGEN */
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, logen_rx_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, logen_rx_pwrup, 1);
        /* TIA Bias PU */
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, fast_nap_bias_pu, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, fast_nap_bias_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, rxrf_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, rxrf_pwrup, 1);
        /* Turn off LPF nrssi */
        MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, lpf_nrssi_pwrup, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, lpf_nrssi_pwrup, 1);

        /* acphy_rfctrl_override pa_pwrup 1 $core; */
        MOD_PHYREGCE(pi, RfctrlCoreTxPus, core, pa_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, pa_pwrup, 1);

        /* No need to set LPF Bq1, Bq2 and RC bandwidth */

        /* Ensure DAC Bandwidth is at 120 */

        /* Enable FreqEst and FreqCorr */
        MOD_PHYREGCE(pi, forceFront, core, freqEst, 1);
        MOD_PHYREGCE(pi, forceFront, core, freqCor, 1);

        /* Enable PAPD Comp during PAPD Cal */
        MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 0);
    }

    if ((ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) &&
        WBPAPD_ENAB(pi)) {
        /* Setup WBPAPD calibration.
         * Run in cal mode 5 like in TCL - may be subject to change.
         * Warning - when changing DAC rate mode, WBPAPD waveform
         * needs to change as well.
         */
        uint8 dac_rate_mode = params->dac_rate_mode;
        if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            if (CHSPEC_IS80(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec))
                dac_rate_mode = params->dac_rate_mode_80M_160M;
        }
        wlc_phy_tx_farrow_setup_28nm(pi, dac_rate_mode /* DAC rate mode */, 1 /* wbcal */);

        FOREACH_CORE(pi, core) {
            MOD_PHYREGCE(pi, wbcal_ctl_2c, core, wbcal_div_clk_sel, 0);
            MOD_PHYREGCE(pi, wbcal_ctl_21, core, wbcal_bypass_fir, 0);
            MOD_PHYREGCE(pi, wbcal_ctl_21, core, wbcal_bypassd_downsamp, 0);
        }
    }

    ACPHY_ENABLE_STALL(pi, stall_val);
}

static void
phy_ac_papd_phy_core_setup_majorrev51_128_129_131(phy_info_t *pi, uint8 calcore)
{
    uint8 stall_val = 0;
    uint8 core, regvalue;
    const uint8 powerup = 1;
    const uint8 powerdown = 0;
    uint8 coremask  =  1 << calcore;

    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);
    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
    MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, coremask);
    MOD_PHYREG(pi, RfseqCoreActv2059, DisRx, (~coremask)&0xF);

    FOREACH_CORE(pi, core) {
        if (core == calcore) {
            regvalue = powerup;
        } else {
            regvalue = powerdown;
        }
        if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
            MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, txrf_pwrup, regvalue);
        } else {
            MOD_PHYREGCE(pi, RfctrlOverrideTxPus, core, txrf_pwrup, 0x1);
        }
        MOD_PHYREGCE(pi, RfctrlCoreTxPus, core, txrf_pwrup, regvalue);
    }
    ACPHY_ENABLE_STALL(pi, stall_val);
}

/* Run PAPD calibration for Tiny */
static void
wlc_phy_txpwr_papd_cal_run_tiny(phy_info_t *pi,    uint8 tx_pre_cal_pwr_ctrl_state, int8 cal_core)
{
    uint8 core, core_start, core_end;
    uint32 initvalue = 0, copyvalue = 0;
    bool suspend = TRUE;
    uint8 yref = 5, start = 5;
    uint8 startvalue;
    uint8 tx_atten = 3, rx_atten = 3;
    uint16 numiter = 128, tbl_size, j;
    uint8 scalar_table_ids[] = { ACPHY_TBL_ID_SCALAR0, ACPHY_TBL_ID_SCALAR1,
        ACPHY_TBL_ID_SCALAR2, ACPHY_TBL_ID_SCALAR3 };
    uint8 eps_table_ids[] = {ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};

    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_params_t *params    = aci->papdcali->papd_params;

    BCM_REFERENCE(aci);

    if (cal_core < 0) {
        /* To indicate if it is a full cal */
        aci->papdcali->fullcal = (cal_core == -2) ? TRUE : FALSE;
        /* do PAPD cal for all cores */
        core_start = 0;
        core_end = PHYCORENUM((pi)->pubpi->phy_corenum)-1;
    } else {
        /* do single core PAPD cal */
        core_start = cal_core;
        core_end = cal_core;
    }

#ifdef ATE_BUILD
    printf("===> Running PAPD cal\n");
#endif /* ATE_BUILD */

    /* PHY may be muted, e.g. for DFS/CAC, so must not transmit anything */
    if (PHY_MUTED(pi)) {
        PHY_CAL(("wl%d: %s: PHY muted - no PAPD cal\n", pi->sh->unit, __FUNCTION__));
        return;
    }

    for (core = core_start; core <= core_end; core++) {
        aci->papdcali->acphy_papd_tx_gain_at_last_cal[core] =
            READ_PHYREGFLDCE(pi, TxPwrCtrlStatus_path, core, baseIndex);
    }

    if (!ACMAJORREV_51_131(pi->pubpi->phy_rev) &&
        !ACMAJORREV_128(pi->pubpi->phy_rev) &&
        !ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        /* ADC cal is removed for 180mz ADC mode in 43012 */
        /* ADC cal coeff for 40mhz mode should be good for 180mhz as well */
        wlc_phy_dac_rate_mode_acphy(pi, 1);
    } else if (ACMAJORREV_130_131(pi->pubpi->phy_rev) && WBPAPD_ENAB(pi)) {
        uint8 dac_rate_mode = params->dac_rate_mode;
        if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            if (CHSPEC_IS80(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec))
                dac_rate_mode = params->dac_rate_mode_80M_160M;
        }
        wlc_phy_dac_rate_mode_acphy(pi, dac_rate_mode);
        // Restore the 3 DC tables that were cleared to 0 during dac_rate_mode change.
        phy_ac_dccal_restore(pi);
        phy_ac_dccal_idacres_restore(pi);
    }

    if (ACMAJORREV_51_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) {
        yref = 10;
        start = 10;
    } else if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        yref = 5;
        start = 5;
    }

    for (core = core_start; core <= core_end; core++) {
        tbl_size = phy_ac_papdcal_eps_table_size(pi, core);

        if (aci->papdcali->papd_abort == 1) {
            /* Save old (0th) eps table to 1st eps table */
            for (j = 0; j < tbl_size; j++) {
                /* 6710: Odd epsilon table entries repeat even ones */
                wlc_phy_table_read_acphy_dac_war(pi, eps_table_ids[core],
                    1, j, 32, &copyvalue, core);
                wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core],
                    1, j + tbl_size, 32, &copyvalue, core);
            }
        }

        /* clear eps table  */
        for (j = 0; j < tbl_size; j++) {
            wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core], 1, j, 32,
                &initvalue, core);
        }
        /* initialize scalar table */
        if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
            ACMAJORREV_128(pi->pubpi->phy_rev))
            wlc_phy_table_write_acphy(pi, scalar_table_ids[core], 128, 0, 32,
                acphy_papd_scaltbl_128_plus3dB);
        else if (APAPD_ENAB(aci->papdcali))
            wlc_phy_table_write_acphy(pi, scalar_table_ids[core], 128, 0, 32,
                acphy_papd_scaltbl_128);
        else
            wlc_phy_table_write_acphy(pi, scalar_table_ids[core], 64, 0, 32,
                acphy_papd_scaltbl);

        /* acphy_papd_cal_phy_setup */
        if (!ACMAJORREV_51_131(pi->pubpi->phy_rev) &&
            !ACMAJORREV_128(pi->pubpi->phy_rev) &&
            !ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
            phy_ac_papd_phy_setup(pi, core);
        }
    }

    if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
        ACMAJORREV_128(pi->pubpi->phy_rev))
        /* FIXME6715 */
        phy_ac_papd_phy_setup_majorrev51_128_129_130_131(pi);
    /* Values of Tx and Rx atten set in phy_ac_populate_papd_params */
    tx_atten = (CHSPEC_IS2G(pi->radio_chanspec) == 1) ?
        params->tx_atten_2g : params->tx_atten_5g;
    rx_atten = (CHSPEC_IS2G(pi->radio_chanspec) == 1) ?
        params->rx_atten_2g : params->rx_atten_5g;

    /* 20693_loopback_papd 0 $tx_atten $rx_atten */
    if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
        numiter = params->papd_iter;
        phy_ac_papd_radio_lpbk_setup_20704(pi, 0);
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        numiter = params->papd_iter;
        phy_ac_papd_radio_lpbk_setup_20709(pi, 0);
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        numiter = params->papd_iter;
        phy_ac_papd_radio_lpbk_setup_20707(pi);
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        numiter = params->papd_iter;
        phy_ac_papd_radio_lpbk_setup_20708(pi, tx_atten, rx_atten);
    } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
        numiter = params->papd_iter;
        phy_ac_papd_radio_lpbk_setup_20710(pi, 0);
    } else {
        phy_ac_papd_radio_lpbk_setup_tiny(pi, tx_atten, rx_atten);
    }

    /* LPF bandwidth is controlled by bandwidth in use. But for WBPAPD we need to widen
     * Rx filters.
     */
    if (WBPAPD_ENAB(pi)) {
        if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            FOREACH_CORE(pi, core) {
                phy_ac_wbcal_widen_filter(pi, CHSPEC_IS2G(pi->radio_chanspec) ?
                    params->tia_mode_2g : params->tia_mode_5g, core);
            }
        }
    }

    if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
        if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
            wlc_phy_radio20704_afe_div_ratio(pi, 1);
        } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
            wlc_phy_radio20707_afe_div_ratio(pi, 1);
        } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            uint8 dac_rate_mode = params->dac_rate_mode;
            if (CHSPEC_IS80(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec))
                dac_rate_mode = params->dac_rate_mode_80M_160M;
            wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
            wlc_phy_radio20708_tx2cal_normal_adc_rate(pi, 1, 0);
            wlc_phy_radio20708_afe_div_ratio(pi, 1, 0, 0, 0, dac_rate_mode, 1);
        } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
            wlc_phy_radio20709_afe_div_ratio(pi, 1);
        } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
            wlc_phy_radio20710_afe_div_ratio(pi, 1, params->dac_rate_mode);
        }
    }

    if (CHSPEC_IS2G(pi->radio_chanspec) && ACREV_GE(pi->pubpi->phy_rev, 4) &&
        (!PHY_EPAPD(pi)) && (!ACMAJORREV_4(pi->pubpi->phy_rev)) &&
        (!ACMAJORREV_51_131(pi->pubpi->phy_rev)) && (!ACMAJORREV_128(pi->pubpi->phy_rev)) &&
        (!ACMAJORREV_129_130(pi->pubpi->phy_rev))) {
        ASSERT(0);
    }

    if (ACMAJORREV_4(pi->pubpi->phy_rev))
        wlc_dcc_fsm_reset(pi);

    /* suspend mac if haven't done so */
    suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
    if (!suspend) {
        printf("MAC was not suspended before calling wlc_phy_txpwr_papd_cal_run_acphy!\n");
        wlapi_suspend_mac_and_wait(pi->sh->physhim);
    }

    /* Disable BT as it affects PAPD CAL */
    if (!ACMAJORREV_51_131(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev) &&
        !ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        wlc_phy_btcx_override_enable(pi);
    }

    /* Disable CRS */
    phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

    /* Due to heat issues, do gain control ahead of actual cal */
    if (ACMAJORREV_51_131(pi->pubpi->phy_rev) && !WBPAPD_ENAB(pi)) {
        FOREACH_CORE(pi, core) {
            wlc_phy_txpwr_by_index_acphy(pi, 1 << core, params->papd_calidx_2g);
            params->bbmult_2g[core] = phy_ac_papd_gain_ctrl_28nm(pi, core, 10, 63, 64);
        }
    }

    if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        FOREACH_CORE(pi, core) {
            /* Force PAPD cal at 0th epsilon table. */
            MOD_PHYREGCEE(pi, PapdLutSel0, core,
                papd_lut_select_ovr_val, 0);
            MOD_PHYREGCEE(pi, PapdLutSel0, core,
                papd_lut_select_ovr_val_cck, 0);
            MOD_PHYREGCEE(pi, PapdLutSel0, core,
                papd_lut_select_ovr, 0);
        }
    }
    for (core = core_start; core <= core_end; core++) {
        phy_ac_papd_cal(pi, numiter, core, start, yref, 63);

        /* eps scalar */
        if (!params->highres_en) {
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 8);
        } else {
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 16);
        }

        if (!WBPAPD_ENAB(pi)) {
            /* Write the LUT table with 0's for index idx=0 upto
             * idx=start
             */
            if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev)) {
                startvalue = 2*start;
            } else {
                startvalue = start;
            }
            for (j = 0; j < startvalue; j++) {
                wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core], 1, j, 32,
                    &initvalue, core);
            }

            if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
                int32 end_epstblidx = 63;

                if (aci->papdcali->end_epstblidx_iovar != -1) {
                    end_epstblidx = aci->papdcali->end_epstblidx_iovar;
                } else {
                    end_epstblidx = CHSPEC_IS2G(pi->radio_chanspec) ? 52 : 50;
                }

                tbl_size = phy_ac_papdcal_eps_table_size(pi, core);
                for (j = 0; j < tbl_size; j++) {
                    /* 6710: Odd epsilon table entries repeat even ones */
                    if (j <= end_epstblidx*2 + 1) {
                        wlc_phy_table_read_acphy_dac_war(pi,
                            eps_table_ids[core], 1, j, 32,
                            &copyvalue, core);
                    }
                    wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core],
                        1, j + tbl_size, 32, &copyvalue, core);
                }
                if (aci->papdcali->epstblsel_iovar == 0) {
                    /* Point to truncated epsilon table for Ucode WAR */
                    MOD_PHYREGCEE(pi, PapdLutSel0, core,
                        papd_lut_select_ovr_val, 1);
                    MOD_PHYREGCEE(pi, PapdLutSel0, core,
                        papd_lut_select_ovr_val_cck, 1);
                    MOD_PHYREGCEE(pi, PapdLutSel0, core,
                        papd_lut_select_ovr, 0);
                } else {
                    uint8 epstblsel_iovar = aci->papdcali->epstblsel_iovar;
                    MOD_PHYREGCEE(pi, PapdLutSel0, core,
                        papd_lut_select_ovr_val, epstblsel_iovar);
                    MOD_PHYREGCEE(pi, PapdLutSel0, core,
                        papd_lut_select_ovr_val_cck, epstblsel_iovar);
                    MOD_PHYREGCEE(pi, PapdLutSel0, core,
                        papd_lut_select_ovr, 1);
                }
            }
        }
    }

    /* acradio papd_cal_cleanup */
    if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        phy_ac_papd_radio_lpbk_cleanup_28nm(pi);
    } else if (ACMAJORREV_51_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev) ||
        ACMAJORREV_130(pi->pubpi->phy_rev)) {
        phy_ac_papd_radio_lpbk_cleanup_majorrev51_128_130_131(pi);
    } else {
        phy_ac_papd_radio_lpbk_cleanup_tiny(pi);
    }

    /* phy clean up */
    for (core = core_start; core <= core_end; core++) {
        if (!ACMAJORREV_51_131(pi->pubpi->phy_rev) &&
            !ACMAJORREV_128(pi->pubpi->phy_rev) &&
            !ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
            phy_ac_papd_phy_cleanup(pi, core);
        }
    }

    if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
        ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* FIXME6715 */
        phy_ac_papd_phy_cleanup_majorrev51_128_129_130_131(pi);
    }

    /* Enable CRS */
    phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
    /* Enable BT */
    if (!ACMAJORREV_51_131(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev) &&
        !ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        wlc_phy_btcx_override_disable(pi);
    }

    /* acphy_papd on */
    for (core = core_start; core <= core_end; core++) {
        if (aci->papdcali->comp_disable_iovar != -1) {
            int8 comp_disable = aci->papdcali->comp_disable_iovar;
            MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, !comp_disable);
            MOD_PHYREGCEE(pi, PapdEnable, core, papd_compCckEnb, !comp_disable);
        } else {
            MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 1);
            if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
                MOD_PHYREGCEE(pi, PapdEnable, core, papd_mcs_dependent_comp_enable,
                    (CHSPEC_IS2G(pi->radio_chanspec) &&
                    CHSPEC_IS20(pi->radio_chanspec)) ? 1 : 0);
                MOD_PHYREGCEE(pi, PapdEnable, core, papd_compCckEnb, 1);
            } else {
                MOD_PHYREGCEE(pi, PapdEnable, core, papd_compCckEnb,
                    !(aci->papdcali->papdcck_disable));
            }
        }
        MOD_PHYREGCEE(pi, PapdCalShifts, core, papd_calEnb, 0);
    }
    if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
        /* Remove afe_div overrides */
        if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
            wlc_phy_radio20704_afe_div_ratio(pi, 0);
        } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
            wlc_phy_radio20707_afe_div_ratio(pi, 0);
        } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
            wlc_phy_radio20708_tx2cal_normal_adc_rate(pi, 0, 0);
            wlc_phy_radio20708_afe_div_ratio(pi, 1, 0, 0, 1, 1, 0);
        } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
            wlc_phy_radio20709_afe_div_ratio(pi, 0);
        } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
            wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
            wlc_phy_radio20710_afe_div_ratio(pi, 0, 0);
        }
    }

    if (ACMAJORREV_130_131(pi->pubpi->phy_rev) && WBPAPD_ENAB(pi)) {
        wlc_phy_dac_rate_mode_acphy(pi, 1);
    }
#ifdef ATE_BUILD
        printf("===> Finished PAPD cal\n");
#endif /* ATE_BUILD */
}

void
wlc_phy_do_papd_cal_acphy(phy_info_t *pi, int8 cal_core)
{

    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_info_t *papdcal_info = aci->papdcali;
    uint8 tx_pwr_ctrl_state = pi->txpwrctrl;
    int8 tx_idx = 0;
    uint8 band;
    uint8 bands[NUM_CHANS_IN_CHAN_BONDING];
#if defined(PHYCAL_CACHING)
    ch_calcache_t *ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif /* PHYCAL_CACHING */

    if (PHY_PAPDEN(pi) && ACMAJORREV_129(pi->pubpi->phy_rev) &&
        CHSPEC_IS5G(pi->radio_chanspec)) {
        /* 6710 5G PAPD requires TIA gain search and special dccal for each
         * TIA gain so we need to save and restore regular dccal coefficients.
         */
        phy_ac_dccal_save(pi);
    }

    if (PHY_PAPDEN(pi) && ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
        /* 6715: DC-Cal coefficients are cleared to 0 during dac_rate_mode change.
         * Changing tia_gain_mode likely requires special dccal.
         * so we need to save and restore regular dccal coefficients.
         */
        phy_ac_dccal_save(pi);
        phy_ac_dccal_idacres_save(pi);
    }

    if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
        phy_ac_chanmgr_get_chan_freq_range_80p80(pi, 0, bands);
        band = bands[0];
        PHY_INFORM(("wl%d: %s: FIXME for 80P80\n", pi->sh->unit, __FUNCTION__));
    } else {
        /* Get current subband information */
        band = phy_ac_chanmgr_get_chan_freq_range(pi, 0, PRIMARY_FREQ_SEGMENT);
    }

    /* Explicitly Turn PAPD off if not enabled
     * (JIRA: SWWLAN-66077)
     */
    if (!PHY_PAPDEN(pi)) {
        uint8 core = 0;

        FOREACH_CORE(pi, core)
            MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 0);
        return;
    }

    PHY_PAPD(("PAPD : PHY_IPA(pi) = %d", PHY_IPA(pi)));
    ASSERT(papdcal_info != NULL);

    if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) {
        if (papdcal_info->acphy_papd_skip == 1)
            return;

        /* TX Index is set inside the Cal function for 43012 */
        wlc_phy_txpwr_papd_cal_run_tiny(pi, tx_pwr_ctrl_state, cal_core);

    } else if (TINY_RADIO(pi)) {

        if (!ACMAJORREV_4(pi->pubpi->phy_rev)) {
            int8 tx_idx_pwr;

            /* use for phy_pacalidx0 and phy_pacalidx1 iovar */
            papdcal_info->papd_lut0_cal_idx = -1;
            papdcal_info->papd_lut1_cal_idx = -1;

            /* 4th priority: default cal index */
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                tx_idx = PHY_EPAPD(pi) ? 48: 26;
            } else {
                tx_idx = PHY_EPAPD(pi) ?
                    ((band <= WL_CHAN_FREQ_RANGE_5G_BAND1) ? 56 : 64) : 30;
            }

            /* 3rd priority: pacalindex from nvram */
            if (CHSPEC_IS2G(pi->radio_chanspec) &&
                    papdcal_info->pacalindex2g != -1) {
                tx_idx = papdcal_info->pacalindex2g;
            } else {
                if ((band <= WL_CHAN_FREQ_RANGE_5G_BAND1) &&
                        (papdcal_info->pacalindex5g[0] != -1)) {
                    tx_idx = papdcal_info->pacalindex5g[0];
                } else if ((band == WL_CHAN_FREQ_RANGE_5G_BAND2) &&
                        (papdcal_info->pacalindex5g[1] != -1)) {
                    tx_idx = papdcal_info->pacalindex5g[1];
                } else if ((band == WL_CHAN_FREQ_RANGE_5G_BAND3) &&
                        (papdcal_info->pacalindex5g[2] != -1)) {
                    tx_idx = papdcal_info->pacalindex5g[2];
                }
            }

            /* 2nd priority: pacalpwr from nvram */
            tx_idx_pwr = wlc_phy_tone_pwrctrl(pi, 96, 0);
            if (tx_idx_pwr != -1)
                tx_idx = tx_idx_pwr;

            /* 1st priority: force cal index through iovar */
            if (papdcal_info->pacalidx_iovar != -1)
                tx_idx = papdcal_info->pacalidx_iovar;

            papdcal_info->papd_lut0_cal_idx = tx_idx;

        } else {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                tx_idx = PHY_EPAPD(pi) ? 48: 26;
            } else {
                tx_idx = PHY_EPAPD(pi) ?
                    ((band <= WL_CHAN_FREQ_RANGE_5G_BAND1) ? 36 : 44): 22;
            }
        }

        wlc_phy_txpwr_by_index_acphy(pi, 1, tx_idx);

        wlc_phy_cals_mac_susp_en_other_cr(pi, TRUE);
        wlc_phy_txpwr_papd_cal_run_tiny(pi, tx_pwr_ctrl_state, cal_core);
        wlc_phy_cals_mac_susp_en_other_cr(pi, FALSE);

    } else {
        wlc_phy_txpwr_papd_cal_run_acphy(pi, tx_pwr_ctrl_state);
    }

#ifdef PHYCAL_CACHING
    if (ctx)
        phy_ac_papdcal_save_cache(pi->u.pi_acphy->papdcali, ctx);
#endif
    if (PHY_PAPDEN(pi) && ACMAJORREV_129(pi->pubpi->phy_rev) &&
        CHSPEC_IS5G(pi->radio_chanspec)) {
        /* 6710 5G PAPD requires TIA gain search and special dccal for each
         * TIA gain so we need to save and restore regular dccal coefficients.
         */
        phy_ac_dccal_restore(pi);
    }

    if (PHY_PAPDEN(pi) && ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
        /* 6715: DC-Cal coefficients are cleared to 0 during dac_rate_mode change.
         * Changing tia_gain_mode likely requires special dccal.
         * so we need to save and restore regular dccal coefficients.
         */
        phy_ac_dccal_restore(pi);
        phy_ac_dccal_idacres_restore(pi);
    }

}

void
wlc_phy_get_papd_cal_pwr_acphy(phy_info_t *pi, int8 *targetpwr, int8 *returned_tx_idx, uint8 core)
{
    if (PHY_PAPDEN(pi)) {
        phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
        phy_ac_papdcal_info_t *papdcal_info = aci->papdcali;
        uint8 band;
        uint8 pacalpwr_idx;
        uint8 core_freq_segment_map;
        int8 tx_idx = -1;

        ASSERT(papdcal_info != NULL);

        /* core_freq_segment_map is only required for 80P80 mode.
        For other modes, it is ignored
        */
        core_freq_segment_map =
            phy_ac_chanmgr_get_data(aci->chanmgri)->core_freq_mapping[core];

        /* Get current subband information */
        band = phy_ac_chanmgr_get_chan_freq_range(pi, 0, core_freq_segment_map);

        switch (band) {
        case WL_CHAN_FREQ_RANGE_2G:
            *targetpwr = papdcal_info->pacalpwr2g;
            tx_idx = papdcal_info->patoneidx2g;
            break;
        case WL_CHAN_FREQ_RANGE_5G_BAND0:
        case WL_CHAN_FREQ_RANGE_5G_BAND1:
        case WL_CHAN_FREQ_RANGE_5G_BAND2:
        case WL_CHAN_FREQ_RANGE_5G_BAND3:
            pacalpwr_idx = band - 1;
            *targetpwr = papdcal_info->pacalpwr5g[core * 4 + pacalpwr_idx];
            if (ACMAJORREV_2(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
                if (CHSPEC_IS40(pi->radio_chanspec)) {
                    *targetpwr =
                        papdcal_info->pacalpwr5g40[core * 4 + pacalpwr_idx];
                } else if (CHSPEC_IS80(pi->radio_chanspec)) {
                    *targetpwr =
                        papdcal_info->pacalpwr5g80[core * 4 + pacalpwr_idx];
                }
            }
            tx_idx = papdcal_info->patoneidx5g[pacalpwr_idx];
            break;
        default:
            ;
        }

        if (tx_idx != -1) {
            *returned_tx_idx = tx_idx;
        }
    }
}

/* TCL PAPDCAL Mode 0/1 - LMS Cal */
void
phy_ac_papd_cal_mode0_1(phy_info_t *pi, acphy_papdCalParams_t *calParams,  uint8 *calmode)
{

    uint8 core = calParams->core, epsilon_table_id = calParams->epsilon_table_id;
    uint8 core_idx;
    uint16 startindex = calParams->startindex, stopindex = calParams->stopindex;
    uint16 yrefindex = calParams->yrefindex, num_iter = calParams->num_iter;
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_info_t *papdi = pi->u.pi_acphy->papdcali;
    uint8 edpdcalset = papdi->edpdcalset;
    BCM_REFERENCE(aci);
    ASSERT(aci->papdcali != NULL);

    MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 1);
    MOD_PHYREGCEE(pi, PapdCalShifts, core, papd_calEnb, 1);

    if (ACMAJORREV_2(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
        if (core == 0) {
            MOD_PHYREGCEE(pi, PapdEnable, 1, papd_compEnb, 0);
            MOD_PHYREGCEE(pi, PapdCalShifts, 1, papd_calEnb, 0);
        } else if (core == 1) {
            MOD_PHYREGCEE(pi, PapdEnable, 0, papd_compEnb, 0);
            MOD_PHYREGCEE(pi, PapdCalShifts, 0, papd_calEnb, 0);
        }
    } else if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
            ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* Turn off PAPD CAL and PAPD COMP on cores that are not cal'd */
        FOREACH_CORE(pi, core_idx) {
            if (core_idx != core) {
                MOD_PHYREGCEE(pi, PapdEnable, core_idx, papd_compEnb, 0);
                MOD_PHYREGCEE(pi, PapdCalShifts, core_idx, papd_calEnb, 0);
            }
        }
    }
    if (TINY_RADIO(pi)) {
        if ((CHSPEC_IS80(pi->radio_chanspec)) && (PHY_EPAPD(pi))) {
            ACPHY_REG_LIST_START
                MOD_PHYREG_ENTRY(pi, PapdEpsilonUpdateIterations,
                    epsilonUpdateIterations, 256)
                MOD_PHYREG_ENTRY(pi, PapdCalSettle, papd_calSettleTime,
                    0x80)
                MOD_PHYREG_ENTRY(pi, PapdCalCorrelate, papd_calCorrTime,
                    0x100)
            ACPHY_REG_LIST_EXECUTE(pi)
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift, 0x7);
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0x0);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I, 0x9);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q, 0x9);
        } else {
            MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime, 0x40);
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0x0);
            if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
                MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                    epsilonUpdateIterations,
                    (CHSPEC_IS2G(pi->radio_chanspec))?32:16);
                MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I,
                    (CHSPEC_IS2G(pi->radio_chanspec))?0x9:0x7);
                MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q,
                    (CHSPEC_IS2G(pi->radio_chanspec))?0x9:0x7);

                if (CHSPEC_IS80(pi->radio_chanspec)) {
                    MOD_PHYREG(pi, PapdCalCorrelate,
                        papd_calCorrTime, 0x80);
                    MOD_PHYREG(pi, PapdCalSettle,
                        papd_calSettleTime,
                        (CHSPEC_IS2G(pi->radio_chanspec))
                        ?0x80:0x80);
                    MOD_PHYREGCEE(pi, PapdCalShifts, core,
                        papdCorrShift, 0x6);

                } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                    MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime,
                        (CHSPEC_IS2G(pi->radio_chanspec))
                        ?0x40:0x80);
                    MOD_PHYREG(pi, PapdCalSettle,
                        papd_calSettleTime,
                        (CHSPEC_IS2G(pi->radio_chanspec))
                        ?0x40:0x40);
                    MOD_PHYREGCEE(pi, PapdCalShifts,
                        core, papdCorrShift,
                        (CHSPEC_IS2G(pi->radio_chanspec))?0x4:0x6);
                } else {
                    MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime,
                        (CHSPEC_IS2G(pi->radio_chanspec))
                        ?0x20:0x40);
                    MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime,
                        (CHSPEC_IS2G(pi->radio_chanspec))
                        ?0x20:0x20);
                    MOD_PHYREGCEE(pi, PapdCalShifts,
                        core, papdCorrShift,
                        (CHSPEC_IS2G(pi->radio_chanspec))?0x3:0x5);
                }

            } else {
                MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x80);
                MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                    epsilonUpdateIterations, num_iter);
                MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift, 0x4);
                MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I, 0xa);
                MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q, 0xa);
            }
            if (aci->papdcali->pacalopt == 1 || aci->papdcali->pacalopt == 2) {
                MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime, 0x100);
                MOD_PHYREG(pi, PapdCalShifts0, papdCorrShift0, 0x7);
            }
        }
    } else {
        if (ACMAJORREV_2(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
            /* For 4350 */
            MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                    epsilonUpdateIterations, num_iter);
            /* setup LMS convergence related params */
            if (CHSPEC_IS80(pi->radio_chanspec)) {
                MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x80);
                MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime, 0x100);
            } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x40);
                MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime, 0x80);
            } else {
                MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x20);
                MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime, 0x40);
            }
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift, 0x4);
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0x0);
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
                if (PHY_EPAPD(pi)) {
                    if (edpdcalset == 2) {
                        /* For MSC 5G FEM */
                        if ((CHSPEC_IS20(pi->radio_chanspec))) {
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x20);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x100);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdCorrShift, 0x6);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_I, 0x8);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_Q, 0x8);
                        } else if ((CHSPEC_IS40
                                (pi->radio_chanspec))) {
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x20);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x100);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdCorrShift, 0x6);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_I, 0x8);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_Q, 0x8);
                        } else {
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x80);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x100);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdCorrShift, 0x4);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_I, 0xa);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_Q, 0xa);
                        }
                    } else if (edpdcalset == 1) {
                        /* For Triquint 5G FEM */
                        if ((CHSPEC_IS20(pi->radio_chanspec))) {
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x20);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x40);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdCorrShift, 0x4);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_I, 0xa);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_Q, 0xa);
                        } else if ((CHSPEC_IS40
                                (pi->radio_chanspec))) {
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x40);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x80);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdCorrShift, 0x5);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_I, 0xa);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_Q, 0xa);
                        } else {
                            MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                                epsilonUpdateIterations, 0x80);
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x40);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x80);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdCorrShift, 0x7);
                            MOD_PHYREG(pi, PapdIpaOffCorr,
                                papd_calIpaOffCorr, 0x40);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_I, 0x9);
                            MOD_PHYREGCEE(pi, PapdCalShifts,
                                core, papdLambda_Q, 0x9);
                        }
                    } else if (edpdcalset == 0) {
                        /* For Skyworks 5G FEM */
                        MOD_PHYREGCEE(pi, PapdCalShifts, core,
                            papdCorrShift, 0x4);
                        MOD_PHYREGCEE(pi, PapdCalShifts, core,
                            papdLambda_I, 0x8);
                        MOD_PHYREGCEE(pi, PapdCalShifts, core,
                            papdLambda_Q, 0x8);
                        if ((CHSPEC_IS20(pi->radio_chanspec))) {
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x20);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x40);
                        } else if ((CHSPEC_IS40
                                (pi->radio_chanspec))) {
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x40);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x80);
                        } else {
                            MOD_PHYREG(pi, PapdCalSettle,
                                papd_calSettleTime, 0x80);
                            MOD_PHYREG(pi, PapdCalCorrelate,
                                papd_calCorrTime, 0x100);
                        }
                    }
                } else {
                    MOD_PHYREGCEE(pi, PapdCalShifts, core,
                        papdLambda_I, 0x7);
                    MOD_PHYREGCEE(pi, PapdCalShifts, core,
                        papdLambda_Q, 0x7);
                }
            } else {
                if (PHY_EPAPD(pi)) {
                    MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                        epsilonUpdateIterations, num_iter);
                    MOD_PHYREG(pi, PapdCalSettle,
                        papd_calSettleTime, 0x80);
                    MOD_PHYREG(pi, PapdCalCorrelate,
                        papd_calCorrTime, 0x40);
                    MOD_PHYREGCEE(pi, PapdCalShifts, core,
                        papdCorrShift, 0x4);
                    MOD_PHYREG(pi, PapdIpaOffCorr,
                        papd_calIpaOffCorr, 0x0);
                    MOD_PHYREGCEE(pi, PapdCalShifts, core,
                        papdLambda_I, 0x8);
                    MOD_PHYREGCEE(pi, PapdCalShifts, core,
                        papdLambda_Q, 0x8);
                } else {
                    if (PHY_IPA(pi) &&
                        !ACMAJORREV_5(pi->pubpi->phy_rev)) {
                        MOD_PHYREGCEE(pi, PapdCalShifts, core,
                            papdLambda_I, 0x8);
                        MOD_PHYREGCEE(pi, PapdCalShifts, core,
                            papdLambda_Q, 0x8);
                    } else {
                        MOD_PHYREGCEE(pi, PapdCalShifts, core,
                            papdLambda_I, 0x9);
                        MOD_PHYREGCEE(pi, PapdCalShifts, core,
                            papdLambda_Q, 0x9);
                    }
                }
            }
        } else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
            MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                epsilonUpdateIterations, num_iter);
            MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x20);
            WRITE_PHYREG(pi, PapdCalCorrelate, calParams->corr_time);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift,
                calParams->corr_shift);
            /* Continuous PAPD PAoff = 0smps, in BW20 1smp = 25ns */
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0x0);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I, 0xa);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q, 0xa);
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 0x8);
            phy_ac_papd_phy_core_setup_majorrev51_128_129_131(pi, core);
        } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                epsilonUpdateIterations, num_iter);
            MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x80);
            WRITE_PHYREG(pi, PapdCalCorrelate, 0x120);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift, 0x4);
            /* Pulsed PAPD PAoff = 512smps, in BW20 1smp = 25ns */
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0x1ff);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I, 0xa);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q, 0xa);
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 0x8);
            phy_ac_papd_phy_core_setup_majorrev51_128_129_131(pi, core);
        } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
            MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                epsilonUpdateIterations, num_iter);
            MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x10);
            WRITE_PHYREG(pi, PapdCalCorrelate,
                    (CHSPEC_IS2G(pi->radio_chanspec)) ? 0x64 : 0x80);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift,
                    (CHSPEC_IS2G(pi->radio_chanspec)) ? 0x6 : 0x5);
            /* Continuous PAPD PAoff = 0smps, in BW20 1smp = 25ns */
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I, 0xa);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q, 0xa);
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 0x8);
            phy_ac_papd_phy_core_setup_majorrev51_128_129_131(pi, core);
        } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                epsilonUpdateIterations, num_iter);
            MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x10);
            WRITE_PHYREG(pi, PapdCalCorrelate,
                    (CHSPEC_IS2G(pi->radio_chanspec)) ? 0x64 : 0x80);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift,
                    (CHSPEC_IS2G(pi->radio_chanspec)) ? 0x6 : 0x5);
            /* Continuous PAPD PAoff = 0smps, in BW20 1smp = 25ns */
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I, 0xa);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q, 0xa);
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 0x8);
            phy_ac_papd_phy_core_setup_majorrev51_128_129_131(pi, core);
        } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
            MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                epsilonUpdateIterations, num_iter);
            MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x20);
            WRITE_PHYREG(pi, PapdCalCorrelate, calParams->corr_time);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift,
                calParams->corr_shift);
            /* Continuous PAPD PAoff = 0smps, in BW20 1smp = 25ns */
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0x0);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I, 0xa);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q, 0xa);
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 0x8);
            phy_ac_papd_phy_core_setup_majorrev51_128_129_131(pi, core);
        } else {
            MOD_PHYREG(pi, PapdEpsilonUpdateIterations,
                epsilonUpdateIterations, num_iter);
            MOD_PHYREG(pi, PapdCalSettle, papd_calSettleTime, 0x80);
            MOD_PHYREG(pi, PapdCalCorrelate, papd_calCorrTime, 0x40);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdCorrShift, 0x4);
            MOD_PHYREG(pi, PapdIpaOffCorr, papd_calIpaOffCorr, 0x0);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_I, 0xa);
            MOD_PHYREGCEE(pi, PapdCalShifts, core, papdLambda_Q, 0xa);
        }
    }

    MOD_PHYREG(pi, PapdCalYrefEpsilon, papdYrefAddr, yrefindex);

    /* use s2.10 PAPD epsilon fixed point format */
    MOD_PHYREGCE(pi, EpsilonOverrideI_, core, epsilonFixedPoint, 0x1);
    if (*calmode == 0) {
        /* Run the PAPD automatic machine on all indices */
        /* setup iter, Yref, start and end address */
        MOD_PHYREG(pi, PapdCalAddress, papdStartAddr, startindex);
        MOD_PHYREG(pi, PapdCalAddress, papdEndAddr, stopindex);

        if (gain_ctrl == 0) {
            (void)wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_1MHz, 186,
                TX_TONE_IQCAL_MODE_OFF, FALSE);
        }
        OSL_DELAY(100);

        if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            WRITE_PHYREGCE(pi, TxFIFOReset, core, 0x1);
            MOD_PHYREG(pi, RxFeCtrl1, soft_sdfeFifoReset, 1);
            OSL_DELAY(10);
            MOD_PHYREG(pi, RxFeCtrl1, soft_sdfeFifoReset, 0);
            OSL_DELAY(100);
        }

        /* start PAPD calibration */
        MOD_PHYREG(pi, PapdCalCoreSel, papdCoreSel, core);
        MOD_PHYREG(pi, PapdCalStart, papdStart, 1);
        SPINWAIT(READ_PHYREG(pi, PapdCalStart), ACPHY_SPINWAIT_PAPDCAL);
        if ((READ_PHYREG(pi, PapdCalStart) & 1)) {
            PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :"
                " PAPD cal failed \n", __FUNCTION__));
            PHY_FATAL_ERROR(pi, PHY_RC_PAPD_CAL_FAILED);
        }
        SPINWAIT(READ_PHYREG(pi, PapdCalStart), ACPHY_SPINWAIT_PAPDCAL);
        if (gain_ctrl == 0) {
            wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);
        }
    } else { /* cal mode 1, hardware cal runs single index at a time */
        uint16 mtable_idx;
        uint32 eps_pre, eps, eps_next;
        int32 epspre_r, epspre_i, eps_r, eps_i, epsnext_r, epsnext_i;
        /* run single index of PAPD table only */
        /* (mode 1 which has been used for debug of 4335 PAPD) */
        /* start the tone */
        if (gain_ctrl == 0) {
            (void)wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_1MHz, 186,
                TX_TONE_IQCAL_MODE_OFF, FALSE);
        }
        MOD_PHYREG(pi, PapdCalCoreSel, papdCoreSel, core);
        OSL_DELAY(600);

        for (mtable_idx = startindex; mtable_idx <= stopindex; mtable_idx++) {
            MOD_PHYREG(pi, PapdCalYrefEpsilon, papdInitYref, 0x1);

            MOD_PHYREG(pi, PapdCalYrefEpsilon, papdEpsilonInit,
                       (mtable_idx == startindex) ? 0x0 : 0x1);

            MOD_PHYREG(pi, PapdCalAddress, papdStartAddr, mtable_idx);
            MOD_PHYREG(pi, PapdCalAddress, papdEndAddr, mtable_idx);

            /* start PAPD calibration */
            OSL_DELAY(10);
            MOD_PHYREG(pi, PapdCalStart, papdStart, 1);
            SPINWAIT(READ_PHYREG(pi, PapdCalStart), ACPHY_SPINWAIT_PAPDCAL);
            if ((READ_PHYREG(pi, PapdCalStart) & 1)) {
                PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :"
                    " PAPD cal failed \n", __FUNCTION__));
                PHY_FATAL_ERROR(pi, PHY_RC_PAPD_CAL_FAILED);
            }
            SPINWAIT(READ_PHYREG(pi, PapdCalStart), ACPHY_SPINWAIT_PAPDCAL);
            OSL_DELAY(10);

            /* predict the next epsilon point */
            wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_id, 1,
                mtable_idx-1, 32, &eps_pre, core);
            wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_id, 1,
                mtable_idx, 32, &eps, core);

            if (aci->papdcali->pacalopt == 2) {
                eps_next = eps;
            } else {
                /* linear extrapolation of prev 2 points to set
                starting point for next eps
                */

                phy_papdcal_decode_epsilon(eps_pre, &epspre_r, &epspre_i);
                phy_papdcal_decode_epsilon(eps, &eps_r, &eps_i);
                if (mtable_idx == startindex) {
                    epsnext_r = eps_r;
                    epsnext_i = eps_i;
                } else {
                    epsnext_r = 2*eps_r-epspre_r;
                    epsnext_i = 2*eps_i-epspre_i;
                }
                if (epsnext_r >= 4095) {
                    epsnext_r = 4095;
                }
                if (epsnext_r <= -4095) {
                    epsnext_r = -4095;
                }
                if (epsnext_r < 0) {
                    epsnext_r = 8192+epsnext_r;
                }
                if (epsnext_i >= 4095) {
                    epsnext_i = 4095;
                }
                if (epsnext_i <= -4095) {
                    epsnext_i = -4095;
                }
                if (epsnext_i < 0) {
                    epsnext_i = 8192+epsnext_i;
                }
                eps_next = ((uint32)epsnext_i << 13) |
                    ((uint32)epsnext_r & 0x1fff);
            }

            wlc_phy_table_write_acphy_dac_war(pi, epsilon_table_id,
                1, mtable_idx+1, 32, &eps_next, core);

            PHY_PAPD(("\n We are in %u M table iteration", mtable_idx));
        }
        if (gain_ctrl == 0) {
            wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);
        }
    }
    if (RADIOMAJORREV(pi) == 2 && PHY_EPAPD(pi) &&
        CHSPEC_IS2G(pi->radio_chanspec)) {
        phy_ac_papd_smooth(pi, core, 5, 0, 32);
    }

    if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        /* TIA gain search needs these registers cleaned up */
        MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, 15);
        MOD_PHYREG(pi, RfseqCoreActv2059, DisRx, 0);
    }
}

/* WBPAPD core function */
static int
phy_ac_wbcal_run(phy_info_t *pi, uint8 for_core)
{
    struct write_regs {
        uint16 reg_addr;
        uint16 reg_val;
    };

    phy_iq_comp_t iqcoeffs;
    int i;
    int err = BCME_OK;
    int initPval = 0x7E49;
    int initPval0 = 1038, initPval1 = 6711;
    int initEpsVal = 0;
    uint8 stall_val = 0;
    uint8 stop_idx = 63;
    uint8 coreidx, core = for_core;
    uint8 cals_on_the_core = 0;
    uint8 target_stop_index = 119;
    uint16 wbcal_sum_RX, wbcal_sum_TX;
    int error_stop_index;
    uint8 eps_table_ids[] = {ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
    uint8 papdlutsel_table_ids[] = {ACPHY_TBL_ID_PAPDLUTSELECT0, ACPHY_TBL_ID_PAPDLUTSELECT1,
        ACPHY_TBL_ID_PAPDLUTSELECT2, ACPHY_TBL_ID_PAPDLUTSELECT3};
    int16 tia_gain_adj_dB_part1, tia_gain_adj_dB_part2, tia_gain_adj_dB, tia_gain_mode_delta;
    int8 diff_cal_idx = 0, offsetOfOffset;
    uint16 tbl_size = phy_ac_papdcal_eps_table_size(pi, core);
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)pi->u.pi_acphy->papdcali;
    phy_ac_papdcal_params_t *params    = papdcali->papd_params;
    int32 wbpapd_gctrl = papdcali->wbpapd_gctrl_iovar;
    int32 multi_tbl_en = papdcali->wbpapd_multitbl_iovar;

    int16 log10mantissa1 = params->target_sum_RX, log10mantissa2, log10exp2;
    const uint16 wbcal_phyreg00_val = (params->wbcal_phyreg0_val);

    /* Set wbcal specific variables from param */
    uint16 lut_step_lo = (uint16) (params->wbcal_lutstep_dB & 0xFFFF);
    uint8 lut_step_hi  = (uint8) ((params->wbcal_lutstep_dB >> 16) & 0xF);
    uint16 start_lo = (uint16)(params->wbcal_start & 0xFFFF);
    uint8 start_hi  = (uint8)((params->wbcal_start >> 16) & 0x3F);
    uint16 stop_lo = (uint16)(params->wbcal_end & 0xFFFF);
    uint8 stop_hi  = (uint8)((params->wbcal_end >> 16) & 0xF);
    uint16 scale_start_lo = (uint16)(params->wbcal_scale_start & 0xFFFF);
    uint8 scale_start_hi  = (uint8)((params->wbcal_scale_start >> 16) & 0xF);
    uint16 scale_stop_lo = (uint16)(params->wbcal_scale_end & 0xFFFF);
    uint8 scale_stop_hi  = (uint8)((params->wbcal_scale_end >> 16) & 0x3F);
    uint16 bbmult = CHSPEC_IS2G(pi->radio_chanspec) ?
        params->bbmult_2g[core] : params->bbmult_5g;
    int16 epsilon_offset = CHSPEC_IS2G(pi->radio_chanspec) ?
            params->epsilon_offset_2g : params->epsilon_offset_5g;
    int16 calc_epsoffset = 0;
    uint16 ref_dB = CHSPEC_IS2G(pi->radio_chanspec) ?
        params->cal_refdb_2g : params->cal_refdb_5g;
    uint16 delayfilt_gamma =  CHSPEC_IS2G(pi->radio_chanspec) ?
        params->delayfilt_gamma_2g : params->delayfilt_gamma_5g;
    uint16 bufoffset =  CHSPEC_IS2G(pi->radio_chanspec) ?
        params->buf_offset_2g : (CHSPEC_IS160(pi->radio_chanspec) ?
        params->buf_offset_5g_160M : params->buf_offset_5g);
    uint8 wbcal_gfrac_bits = CHSPEC_IS2G(pi->radio_chanspec) ?
        (params->wbcal_gfrac_bits & 0x0F) :
        ((params->wbcal_gfrac_bits >> 4) & 0x0F);
    uint8 papd_calidx;
    uint16 wbcal_const_pow_scale = params->wbcal_const_pow_scale;
    uint8 tia_mode = CHSPEC_IS2G(pi->radio_chanspec) ? params->tia_mode_2g:params->tia_mode_5g;
    uint8 papd_lut_select = 0;
    uint8 initLUTSelectVal;
    bool cal_epsOffset_en = ACMAJORREV_131(pi->pubpi->phy_rev) ? FALSE : TRUE;
    bool cal_epsOffset_success;
    enum wbcal_state {FIRST, SECOND_GCTRL, CAL_EPSOFFSET, BBMULT_GCTRL} wbstate;
    uint16 wbcal_scale_int_in = 0, wbcal_scale_int_out = 0;
    uint32 *eps_table, *orig_eps_table, *tmp_table;
    acphy_papd_swctrl_t *swctrl = pi->u.pi_acphy->sromi->papd_swctrl;
    uint8 band_idx = CHSPEC_IS2G(pi->radio_chanspec) ? 0 :
        (CHSPEC_IS5G(pi->radio_chanspec) ? 1 : 2);
    uint8 num_eps_tables = (multi_tbl_en == 0) ? 1 : (params->num_eps_tables);
    bool xcore_wbpapd_cal_en;
    uint16 eps_stop_idx, saved_eps_stop_idx = 0;
    uint16 saved_epsilonOffset = 0;
    uint16 saved_papd_index_offset_lut = 0;
    int8 wbpapd_bbm_gctrl_iter = ACMAJORREV_131(pi->pubpi->phy_rev) ? 3 : 0;
    int16 bbm_corr_dB;
    int32 bbm_corr_multipl;
    uint16 bbmult_dacg = bbmult;

    if (PHY_EPAPD(pi) && swctrl->valid && swctrl->band[band_idx].mode != 0) {
        xcore_wbpapd_cal_en = TRUE;
        core = swctrl->band[band_idx].lp_core[core];
    } else {
        xcore_wbpapd_cal_en = FALSE;
        core = for_core;
    }

    // Allocate space to save/restore epsilon tables.
    eps_table = phy_malloc_fatal(pi, tbl_size * sizeof(*eps_table));
    orig_eps_table = phy_malloc_fatal(pi, tbl_size * sizeof(*orig_eps_table));
    // Temp table to read/write entries in a chunk, of size the largest table size to be used.
    tmp_table = phy_malloc_fatal(pi, 256 * sizeof(*tmp_table));

    /* In epapd, enable FEM's on-chip coupler. */
    if (PHY_EPAPD(pi) && swctrl->valid) {
        MOD_PHYREGCE(pi, FemOutputOvrCtrl, for_core, femCtrlOutput,
            swctrl->band[band_idx].femctrl[core]);
        MOD_PHYREGCE(pi, FemOutputOvrCtrl, for_core, femCtrlOutputOvr, 1);
    }
    if (xcore_wbpapd_cal_en) {
        MOD_PHYREGCE(pi, FemOutputOvrCtrl, core,
            femCtrlOutput, swctrl->band[band_idx].femctrl_lp[core]);
        MOD_PHYREGCE(pi, FemOutputOvrCtrl, core,
            femCtrlOutputOvr, 1);

        /* Backup all contexts of on_core */
        saved_eps_stop_idx = READ_PHYREGFLDCEE(pi, PapdCompStopIdxLutSel1,
            core, papd_comp_stop_index_lut0);
        saved_papd_index_offset_lut = READ_PHYREGFLDCEE(pi, PapdLutSel1,
            core, papd_index_offset_lut0);
        saved_epsilonOffset = READ_PHYREGFLDCEE(pi, EpsilonTableAdjust,
            core, epsilonOffset);

        /* Save the original eps table for on_core, will restore later */
        wlc_phy_table_read_acphy_dac_war(pi, eps_table_ids[core],
            tbl_size, 0, 32, orig_eps_table, core);
    }

    ref_dB = (ACMAJORREV_131(pi->pubpi->phy_rev) ? WBPAPD_REFDB_BASE_R131 :
        WBPAPD_REFDB_BASE) + ((ref_dB) * WBPAPD_REFDB_STEP);

    PHY_PAPD(("wl%d %s core %d: WBPAPD CAL\n", pi->sh->unit, __FUNCTION__, core));

    if (!xcore_wbpapd_cal_en) {
        phy_ac_papd_phy_core_setup_majorrev51_128_129_131(pi, core);
    } else {
        /* Enable all cores in alignment with TCL cross-core settings. */
        uint8 stall_val_tmp = 0;
        stall_val_tmp = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
        ACPHY_DISABLE_STALL(pi);
        MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, 0xf);
        MOD_PHYREG(pi, RfseqCoreActv2059, DisRx, 0x0);
        FOREACH_CORE(pi, coreidx) {
            MOD_PHYREGCE(pi, RfctrlOverrideTxPus, coreidx, txrf_pwrup, 0);
            MOD_PHYREGCE(pi, RfctrlCoreTxPus, coreidx, txrf_pwrup, 0);
        }
        ACPHY_ENABLE_STALL(pi, stall_val_tmp);
    }

    FOREACH_CORE(pi, coreidx) {
        if (coreidx == core) {
            MOD_PHYREGCE(pi, wbcal_ctl_21, coreidx, wbcal_no_update_tbl, 0);
        } else {
            MOD_PHYREGCE(pi, wbcal_ctl_21, coreidx, wbcal_no_update_tbl, 1);
        }
    }

    /* Read RX IQ Cal coeffs */
    iqcoeffs.a = 0;
    iqcoeffs.b = 0;
    wlc_phy_rx_iq_comp_acphy(pi, 0, &(iqcoeffs), core);

    MOD_PHYREGCE(pi, wbcal_ctl_12, core, wbcal_iq_coeff_a, iqcoeffs.a);
    MOD_PHYREGCE(pi, wbcal_ctl_13, core, wbcal_iq_coeff_b, iqcoeffs.b);
    MOD_PHYREGCE(pi, wbcal_ctl_5, core, wbcal_ref_dB, ref_dB);
    MOD_PHYREGCE(pi, wbcal_ctl_6, core, wbcal_lutstep_dB_lo, lut_step_lo);
    MOD_PHYREGCE(pi, wbcal_ctl_7, core, wbcal_lutstep_dB_hi, lut_step_hi);
    MOD_PHYREGCE(pi, wbcal_ctl_1D, core, wbcomp_lutstep_dB_lo, lut_step_lo);
    MOD_PHYREGCE(pi, wbcal_ctl_8, core, wbcal_LUT_LEN, params->wbcal_lut_len);
    MOD_PHYREGCE(pi, wbcal_ctl_1F, core, wbcomp_LUT_LEN, params->wbcal_lut_len);
    //MOD_PHYREGCE(pi, wbcal_ctl_E, core, wbcal_start_lo, start_lo);
    //MOD_PHYREGCE(pi, wbcal_ctl_F, core, wbcal_start_hi, start_hi);
    //MOD_PHYREGCE(pi, wbcal_ctl_10, core, wbcal_stop_lo, stop_lo);
    //MOD_PHYREGCE(pi, wbcal_ctl_11, core, wbcal_stop_hi, stop_hi);
    MOD_PHYREGCE(pi, wbcal_ctl_2d, core, wbpapd_scale_start_lo, scale_start_lo);
    MOD_PHYREGCE(pi, wbcal_ctl_2e, core, wbpapd_scale_stop_lo, scale_stop_lo);
    MOD_PHYREGCE(pi, wbcal_ctl_2f, core, wbcal_scale_start_hi, scale_start_hi);
    MOD_PHYREGCE(pi, wbcal_ctl_2f, core, wbcal_scale_stop_hi, scale_stop_hi);
    MOD_PHYREGCE(pi, wbcal_ctl_2, core, wbcal_lambda, 32768);
    MOD_PHYREGCE(pi, wbcal_ctl_18, core, wbpapd_cal_scale_in_lo, 16565);
    MOD_PHYREGCE(pi, wbcal_ctl_19, core, wbpapd_cal_scale_out_lo, 20764);
    if (!params->highres_en) {
        MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 8);
    } else {
        MOD_PHYREGCE(pi, papdEpsilonTable, core, papd_index_scalar_sel, 0);
        MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonScalar, 16);
        MOD_PHYREGCEE(pi, PapdPolarSaturation2, core, inv_index_scalar, 512);
        MOD_PHYREG(pi, PapdCalTableMode, papdHighResEn, 1);
    }

    // LMS Pre-filter registers.
    MOD_PHYREGCEE(pi, dpdeq_loopback_config1, core, loopback_eq_cal_start_ctr, 3072);
    MOD_PHYREGCEE(pi, dpdeq_loopback_config2, core, loopback_eq_cal_numiter, 10000);
    MOD_PHYREG(pi, loopback_eq_cal_config0, loopback_eq_cal_mu_man, 0);
    MOD_PHYREG(pi, loopback_eq_cal_config0, loopback_eq_cal_mu_exp, 2);

    /* G Frac bits */
    MOD_PHYREGCE(pi, wbcal_ctl_21, core, wbpapd_cal_g_frac_bits, wbcal_gfrac_bits);
    MOD_PHYREGCE(pi, wbcal_ctl_22, core, wbpapd_cal_dcc_en, params->wbcal_dcc_en);
    /* Delay Filter */
    MOD_PHYREGCE(pi, wbcal_ctl_22, core, wbpapd_delay_filter_en, params->wbcal_dly_filt_en);
    MOD_PHYREGCE(pi, wbcal_ctl_23, core, wbpapd_cal_dc_corr_override_en,
        params->wbcal_dccorr_ovr_en);
    MOD_PHYREGCE(pi, wbcal_ctl_24, core, wbpapd_cal_dc_offset_value_real,
            params->wbcal_dcoffset_real);
    MOD_PHYREGCE(pi, wbcal_ctl_24, core, wbpapd_cal_dc_offset_value_imag,
            params->wbcal_dcoffset_imag);
    /* 2 tbl enble */
    MOD_PHYREGCE(pi, wbcal_ctl_25, core, wbpapd_two_table_enable, params->wbcal_twotbl_en);
    MOD_PHYREGCE(pi, wbcal_ctl_1B, core, wbpapd_cal_const_pow_scale, wbcal_const_pow_scale);
    MOD_PHYREGCE(pi, wbcal_ctl_1, core, wbcal_txbuf_offset, bufoffset);
    MOD_PHYREGCE(pi, wbcal_ctl_22, core, wbpapd_filter_delay_gamma, delayfilt_gamma);

    if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
        MOD_PHYREGCE(pi, wbcal_ctl_1A, core, wbpapd_scale_window_len_pow,
            params->wbcal_window_len_pow);
        MOD_PHYREGCE(pi, wbcal_ctl_22, core, wbcal_dc_accum_wait,
            params->wbcal_dc_accum_wait);
    } else {
        MOD_PHYREGCE(pi, wbcal_ctl_22, core, wbcal_dc_accum_wait, 12);
    }

    /* Turn off papd before WBCAL */
    // comp_enable = READ_PHYREGFLDCEE(pi, PapdEnable, core, papd_compEnb);
    MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 0);
    MOD_PHYREGCEE(pi, PapdEnable, for_core, papd_compEnb, 0);

    /* Transmit packet from MAC FIFO */
    /* sizeof gives sizeof array in bytes and we want it in words */
    macplay_wfm_ptr = get_wbpapd_wfm_phyrev130(pi);

    if (multi_tbl_en) {
        /* Populate paplutselect table used in txpwrctrl with proper table index. */
        for (i = 0; i < 128; i++) {
            if (i <= params->tx_idx_thresh_tbl[0])
                initLUTSelectVal = 0;
            else
                initLUTSelectVal = 9;
            wlc_phy_table_write_acphy(pi, papdlutsel_table_ids[core], 1, i, 8,
                &initLUTSelectVal);
        }
    } else {
        /* Populate paplutselect table used in txpwrctrl with proper table index. */
        for (i = 0; i < 128; i++) {
            initLUTSelectVal = 0;
            wlc_phy_table_write_acphy(pi, papdlutsel_table_ids[core], 1, i, 8,
                &initLUTSelectVal);
        }
    }

    /* Program bbmult */
    if (papdcali->papdbbmult_iovar != -1) {
        /* Force bbmult through iovar 'wl phy_papdbbmult' */
        bbmult = papdcali->papdbbmult_iovar;
    } else {
        if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            if (CHSPEC_IS20(pi->radio_chanspec)) {
                bbmult = (CHSPEC_IS2G(pi->radio_chanspec)) ?
                    params->bbmult_2g[core] : params->bbmult_5g;
            } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                bbmult = (CHSPEC_IS2G(pi->radio_chanspec)) ?
                    params->bbmult_2g_40M[core] : params->bbmult_5g_40M;
            } else {
                bbmult = params->bbmult_5g_80M_160M;
            }
        } else if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
            bbmult = CHSPEC_IS2G(pi->radio_chanspec) ?
                params->bbmult_2g[core] : params->bbmult_5g;
        }
    }
    /* Program cal index, could be changed by Tx gain control later */
    if (papdcali->pacalidx_iovar != -1) {
        /* Force cal index through iovar 'wl phy_pacalidx' */
        papd_calidx = papdcali->pacalidx_iovar;
    } else {
        papd_calidx = (CHSPEC_IS2G(pi->radio_chanspec)) ? params->papd_calidx_2g :
            params->papd_calidx_5g;
    }
    /* Program TIA gain, could be changed by Rx gain control later */
    if (papdcali->papdtiagain_iovar != -1) {
        /* Force TIA gain through iovar 'wl phy_papdtiagain' */
        tia_mode = papdcali->papdtiagain_iovar;
    } else {
        tia_mode = (CHSPEC_IS2G(pi->radio_chanspec)) ?
            params->tia_mode_2g : params->tia_mode_5g;
    }

    wbstate = FIRST;
    while (papd_lut_select < num_eps_tables) {
        if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
            /* DAC rate mode 2 used for cal increases Tx power.
             * Compensate here so cal power is the same for bw20 and bw40,
             * leading to the same iPA amount of compression.
             * Warning: scaling should be used only for cal, not comp.
             * That is why bbmult is not modified.
             */
            if (CHSPEC_IS20(pi->radio_chanspec)) {
                bbmult_dacg = bbmult * 5 / 8;
            } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                bbmult_dacg = bbmult * 4 / 5;
            } else {
                bbmult_dacg = bbmult;
            }
        }
        /* Write Tx gains */
        FOREACH_CORE(pi, coreidx) {
            if (coreidx == for_core) {
                wlc_phy_txpwr_by_index_acphy(pi, (uint8)(1 << coreidx),
                    papd_calidx);
                if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
                    wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_dacg, coreidx);
                } else {
                    wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, coreidx);
                }
            } else {
                if (xcore_wbpapd_cal_en) {
                    acphy_txgains_t tx_gains = {0, 0, 0, 0, 0};
                    /* Not changing bbmult */
                    phy_ac_papd_write_tx_gain(pi, coreidx, &tx_gains,
                        &bbmult, 1);
                    MOD_PHYREGCEE(pi, PapdLutSel0, coreidx,
                        papd_lut_select_ovr, 0);
                }
            }
        }
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            papd_gainctrl_calidx_2g[for_core] = papd_calidx;
        } else {
            papd_gainctrl_calidx_5g[for_core] = papd_calidx;
        }
        papd_gainctrl_bbmult[core] = bbmult;

        /* Write TIA gain */
        WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, 0);
        MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain, tia_mode);
        /* Widen Rx filter */
        phy_ac_wbcal_widen_filter(pi, tia_mode, core);
        papd_tiagain[core] = tia_mode;

        /* Write the following 2 regs after tx_idx, because tx_idx modifies them. */
        MOD_PHYREGCEE(pi, PapdLutSel0, core, papd_lut_select_ovr_val,
            papd_lut_select);
        MOD_PHYREGCEE(pi, PapdLutSel0, core, papd_lut_select_ovr_val_cck,
            papd_lut_select);
        PHY_PAPD(("%s core%d papd_lut_select %d, papd_calidx %d, wbstate %d\n",
            __FUNCTION__, core, papd_lut_select, papd_calidx, wbstate));

        /* papd_lut_select_ovr also modified after setting tx gains */
        if (multi_tbl_en) {
            MOD_PHYREGCEE(pi, PapdLutSel0, core, papd_lut_select_ovr, 1);

            if (wbstate == FIRST) {
                papd_calidx = params->papd_calidx_tbl[papd_lut_select];
                // Statically configured per-table epsilon offset.
                if (papd_lut_select == 0)
                    MOD_PHYREGCEE(pi, PapdLutSel1, core,
                        papd_index_offset_lut0, params->eps_offset_lut[0]);
                else
                    MOD_PHYREGCEE(pi, PapdLutSel1, core,
                        papd_index_offset_lut1, params->eps_offset_lut[1]);
            }
            ref_dB = params->cal_refdb_tbl[papd_lut_select];
            ref_dB = (ACMAJORREV_131(pi->pubpi->phy_rev) ? WBPAPD_REFDB_BASE_R131 :
                    WBPAPD_REFDB_BASE) + ((ref_dB) * WBPAPD_REFDB_STEP);
            MOD_PHYREGCE(pi, wbcal_ctl_5, core, wbcal_ref_dB, ref_dB);
            tia_mode = params->tia_mode_tbl[papd_lut_select];

            /* Set TIA gain */
            WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, 0);
            MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain, tia_mode);
            /* Widen RX filter */
            phy_ac_wbcal_widen_filter(pi, tia_mode, core);

            /* clear eps table  */
            for (i = 0; i < tbl_size; i++) {
                tmp_table[i] = initEpsVal;
            }
            wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core], tbl_size,
                papd_lut_select * tbl_size, 32, tmp_table, core);

        } else {
            MOD_PHYREGCEE(pi, PapdLutSel0, core, papd_lut_select_ovr, 0);

            /* clear 0-th and 1-th eps tables */
            for (i = 0; i < tbl_size; i++) {
                tmp_table[i] = initEpsVal;
            }
            wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core], tbl_size,
                0, 32, tmp_table, core);
            wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core], tbl_size,
                tbl_size, 32, tmp_table, core);
        }

        /* Ptbl holds moving average of squared magnitude of Rx samples.
         * To help convergence, roughly init lower indexes with smaller
         * values, and init higher indexes with larger values.
         */
        for (i = 0; i < 128; i++) {
            if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
                tmp_table[i] = (i < 100) ? initPval0 : initPval1;
            } else {
                tmp_table[i] = initPval;
            }
        }
        if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
            wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_WBCAL_PTBL,
                128, 0, 32, tmp_table);
        } else {
            wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_WBCAL_PTBL0,
                128, 0, 32, tmp_table);
        }

        if (wbstate == CAL_EPSOFFSET) {
            // LMS Pre-filter registers.
            MOD_PHYREGCEE(pi, dpdeq_loopback_config0, core, loopback_eq_cal_en, 0);
            MOD_PHYREGCEE(pi, dpdeq_loopback_config0, core, loopback_eq_cal_scale_en,
                1);
            MOD_PHYREGCEE(pi, dpdeq_loopback_config0, core, loopback_eq_filter_en, 1);

            // Init values of pre-filter coefficients.
            MOD_PHYREG(pi, loopback_eq_cal_config1, loopback_lmseq_cal_w0_i,
                papd_prefilter_coeff_i[core][0]);
            MOD_PHYREG(pi, loopback_eq_cal_config2, loopback_lmseq_cal_w0_q,
                papd_prefilter_coeff_q[core][0]);
            MOD_PHYREG(pi, loopback_eq_cal_config3, loopback_lmseq_cal_w1_i,
                papd_prefilter_coeff_i[core][1]);
            MOD_PHYREG(pi, loopback_eq_cal_config4, loopback_lmseq_cal_w1_q,
                papd_prefilter_coeff_q[core][1]);
            MOD_PHYREG(pi, loopback_eq_cal_config5, loopback_lmseq_cal_w2_i,
                papd_prefilter_coeff_i[core][2]);
            MOD_PHYREG(pi, loopback_eq_cal_config6, loopback_lmseq_cal_w2_q,
                papd_prefilter_coeff_q[core][2]);
            MOD_PHYREG(pi, loopback_eq_cal_config7, loopback_lmseq_cal_w3_i,
                papd_prefilter_coeff_i[core][3]);
            MOD_PHYREG(pi, loopback_eq_cal_config8, loopback_lmseq_cal_w3_q,
                papd_prefilter_coeff_q[core][3]);
            MOD_PHYREG(pi, loopback_eq_cal_config9, loopback_lmseq_cal_w4_i,
                papd_prefilter_coeff_i[core][4]);
            MOD_PHYREG(pi, loopback_eq_cal_config10, loopback_lmseq_cal_w4_q,
                papd_prefilter_coeff_q[core][4]);
            MOD_PHYREG(pi, loopback_eq_cal_config11, loopback_lmseq_cal_w5_i,
                papd_prefilter_coeff_i[core][5]);
            MOD_PHYREG(pi, loopback_eq_cal_config12, loopback_lmseq_cal_w5_q,
                papd_prefilter_coeff_q[core][5]);
            MOD_PHYREG(pi, loopback_eq_cal_config13, loopback_lmseq_cal_w6_i,
                papd_prefilter_coeff_i[core][6]);
            MOD_PHYREG(pi, loopback_eq_cal_config14, loopback_lmseq_cal_w6_q,
                papd_prefilter_coeff_q[core][6]);

            MOD_PHYREGCE(pi, wbcal_ctl_E, core, wbcal_start_lo, 600);
            MOD_PHYREGCE(pi, wbcal_ctl_F, core, wbcal_start_hi, 0);
            MOD_PHYREGCE(pi, wbcal_ctl_10, core, wbcal_stop_lo, 5000);
            MOD_PHYREGCE(pi, wbcal_ctl_11, core, wbcal_stop_hi, 0);

            WRITE_PHYREGCE(pi, wbcal_ctl_0, core, wbcal_phyreg00_val);
            MOD_PHYREGCE(pi, wbcal_ctl_0, core, wbpapd_cal_scale_inout_en, 0);
            WRITE_PHYREGCE(pi, wbcal_ctl_18, core, wbcal_scale_int_in);
            WRITE_PHYREGCE(pi, wbcal_ctl_19, core, wbcal_scale_int_out);
        } else {
            // LMS Pre-filter registers.
            MOD_PHYREGCEE(pi, dpdeq_loopback_config0, core, loopback_eq_cal_en, 1);
            MOD_PHYREGCEE(pi, dpdeq_loopback_config0, core, loopback_eq_cal_scale_en,
                1);
            MOD_PHYREGCEE(pi, dpdeq_loopback_config0, core, loopback_eq_filter_en, 1);

            // Init values of pre-filter coefficients.
            MOD_PHYREG(pi, loopback_eq_cal_config1, loopback_lmseq_cal_w0_i, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config2, loopback_lmseq_cal_w0_q, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config3, loopback_lmseq_cal_w1_i, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config4, loopback_lmseq_cal_w1_q, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config5, loopback_lmseq_cal_w2_i, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config6, loopback_lmseq_cal_w2_q, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config7, loopback_lmseq_cal_w3_i, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config8, loopback_lmseq_cal_w3_q, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config9, loopback_lmseq_cal_w4_i, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config10, loopback_lmseq_cal_w4_q, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config11, loopback_lmseq_cal_w5_i, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config12, loopback_lmseq_cal_w5_q, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config13, loopback_lmseq_cal_w6_i, 0);
            MOD_PHYREG(pi, loopback_eq_cal_config14, loopback_lmseq_cal_w6_q, 0);

            MOD_PHYREGCE(pi, wbcal_ctl_E, core, wbcal_start_lo, start_lo);
            MOD_PHYREGCE(pi, wbcal_ctl_F, core, wbcal_start_hi, start_hi);
            MOD_PHYREGCE(pi, wbcal_ctl_10, core, wbcal_stop_lo, stop_lo);
            MOD_PHYREGCE(pi, wbcal_ctl_11, core, wbcal_stop_hi, stop_hi);

            WRITE_PHYREGCE(pi, wbcal_ctl_0, core, wbcal_phyreg00_val);
        }

        /* disable Stalls  before mac play and restore it later */
        stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
        MOD_PHYREG(pi, RxFeCtrl1, disable_stalls, 0x1);
        MOD_PHYREG(pi, sampleCmd, stop, 0x1);

        /* Enable WBCAL */
        MOD_PHYREGCE(pi, wbcal_ctl_0, core, wbcal_en, 1);
        if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
            // Mainly for resetting read & write pointers of TXDELAYBUF & RXSYNCBUF.
            MOD_PHYREGCE(pi, TxFIFOReset, core, tx_fifo_reset, 1);
        }
        MOD_PHYREGCE(pi, wbcal_ctl_0, core, wbcal_on, 1);
        MOD_PHYREGCE(pi, wbcal_ctl_0, core, wbcal_trig, 1);

        if (wbstate == CAL_EPSOFFSET) {
            err = phy_ac_papd_mac_play(pi, macplay_wfm_ptr,
                params->wbcal_waveform_sz*8*4, 2500, ON, FALSE);
            if (err != BCME_OK) {
                PHY_ERROR(("%s: MAC-PLAY START FAILED\n", __FUNCTION__));
                goto fail;
            }

            OSL_DELAY(750);
        } else {
            err = phy_ac_papd_mac_play(pi, macplay_wfm_ptr, 0,
                params->wbcal_waveform_sz*8*4, ON,
                (for_core == 0 && cals_on_the_core == 0 &&
                papd_lut_select == 0));
            if (err != BCME_OK) {
                PHY_ERROR(("%s: MAC-PLAY START FAILED\n", __FUNCTION__));
                goto fail;
            }

            /* Wait for 300 microseconds */
            OSL_DELAY(750);
        }

        SPINWAIT(READ_PHYREGFLDCE(pi, wbcal_ctl_17, core, wbcal_done) == 0,
            ACPHY_SPINWAIT_PAPDCAL);
        if (READ_PHYREGFLDCE(pi, wbcal_ctl_17, core, wbcal_done) == 0) {
            PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :"
                " WBPAPD cal failed \n", __FUNCTION__));
            PHY_FATAL_ERROR(pi, PHY_RC_PAPD_CAL_FAILED);
        }

        /* For dumping pre-filter coefficients. */
        if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
            papd_prefilter_coeff_i[core][0] = READ_PHYREG(pi,
                loopback_lmseq_debug_group0);
            papd_prefilter_coeff_q[core][0] = READ_PHYREG(pi,
                loopback_lmseq_debug_group1);
            papd_prefilter_coeff_i[core][1] = READ_PHYREG(pi,
                loopback_lmseq_debug_group2);
            papd_prefilter_coeff_q[core][1] = READ_PHYREG(pi,
                loopback_lmseq_debug_group3);
            papd_prefilter_coeff_i[core][2] = READ_PHYREG(pi,
                loopback_lmseq_debug_group4);
            papd_prefilter_coeff_q[core][2] = READ_PHYREG(pi,
                loopback_lmseq_debug_group5);
            papd_prefilter_coeff_i[core][3] = READ_PHYREG(pi,
                loopback_lmseq_debug_group6);
            papd_prefilter_coeff_q[core][3] = READ_PHYREG(pi,
                loopback_lmseq_debug_group7);
            papd_prefilter_coeff_i[core][4] = READ_PHYREG(pi,
                loopback_lmseq_debug_group8);
            papd_prefilter_coeff_q[core][4] = READ_PHYREG(pi,
                loopback_lmseq_debug_group9);
            papd_prefilter_coeff_i[core][5] = READ_PHYREG(pi,
                loopback_lmseq_debug_group10);
            papd_prefilter_coeff_q[core][5] = READ_PHYREG(pi,
                loopback_lmseq_debug_group11);
            papd_prefilter_coeff_i[core][6] = READ_PHYREG(pi,
                loopback_lmseq_debug_group12);
            papd_prefilter_coeff_q[core][6] = READ_PHYREG(pi,
                loopback_lmseq_debug_group13);
        }
        wbcal_sum_TX = READ_PHYREGFLDCE(pi, wbcal_ctl_2a, core, wbcal_sum_TX);
        BCM_REFERENCE(wbcal_sum_TX);
        wbcal_sum_RX = READ_PHYREGFLDCE(pi, wbcal_ctl_2b, core, wbcal_sum_RX);
        wbcal_scale_int_in = READ_PHYREGFLDCE(pi, wbcal_ctl_40, core,
            wbcal_scale_factor_0);
        wbcal_scale_int_out = READ_PHYREGFLDCE(pi, wbcal_ctl_41, core,
            wbcal_scale_factor_1);
        wbcal_scale_int_out >>= 2;
        PHY_PAPD(("%s core%d sum_TX %d, sum_RX %d, scale_out 0x%x, scale_in 0x%x\n",
            __FUNCTION__, core, wbcal_sum_TX, wbcal_sum_RX, wbcal_scale_int_out,
            wbcal_scale_int_in));

        /* ToDo: Stop playback from MAC FIFO */
        err = phy_ac_papd_mac_play(pi, NULL, 0, 0, OFF, FALSE);
        if (err != BCME_OK) {
            PHY_ERROR(("%s: MAC-PLAY STOP FAILED\n", __FUNCTION__));
            goto fail;
        }
        ACPHY_ENABLE_STALL(pi, stall_val);

        /* Stop WBCAL */
        MOD_PHYREGCE(pi, wbcal_ctl_0, core, wbpapd_core_en, 0);
        MOD_PHYREGCE(pi, wbcal_ctl_0, core, wbcal_on, 0);
        MOD_PHYREGCE(pi, wbcal_ctl_0, core, wbcal_trig, 0);
        MOD_PHYREGCE(pi, wbcal_ctl_0, core, wbcal_en, 0);

        /* Calculate the stop index */
        if (wbstate != CAL_EPSOFFSET) {
            err = phy_ac_wbcal_eps_stopidx(pi, core);
            if (err < 0) {
                PHY_ERROR(("%s: stop_idx undetermined (%d)\n", __FUNCTION__, err));
                goto fail;
            }
            stop_idx = (uint8)err;
        }

        // Gain control. Gain difference across parts/cores necessitates multiple cals.
        error_stop_index = stop_idx - target_stop_index;
        if ((error_stop_index >= 2 || error_stop_index <= -2) &&
            (wbstate == FIRST) && (wbpapd_gctrl == 2)) {
            // Need to do another cal.
            // Step 1. Tx gain control.
            if (papdcali->pacalidx_iovar == -1) {
                /* Do this only when no calidx overriding with iovar */
                if (error_stop_index >= 0)
                    diff_cal_idx = -((error_stop_index + 1) / 2);
                else
                    diff_cal_idx = -((error_stop_index - 1) / 2);
                papd_calidx += diff_cal_idx;
            }
            // Step 2. Rx TIA gain control.
            if (papdcali->papdtiagain_iovar == -1) {
                /* Do this only when no TAI gain overriding with iovar */
                qm_log10((int32)(wbcal_sum_RX), 0, &log10mantissa2, &log10exp2);
                // Align the exponent to 10.
                if (log10exp2 > 10) {
                    log10mantissa2 >>= (log10exp2 - 10);
                } else if (log10exp2 < 10) {
                    log10mantissa2 <<= (10 - log10exp2);
                }
                // 10 fractional bits.
                tia_gain_adj_dB_part1 = (log10mantissa1 - log10mantissa2) * 20;
                // 10 fractional bits.
                tia_gain_adj_dB_part2 = error_stop_index << 8;
                tia_gain_adj_dB = tia_gain_adj_dB_part1 - tia_gain_adj_dB_part2;
                tia_gain_mode_delta = (tia_gain_adj_dB / 3) >> 10;
                tia_mode = tia_mode + tia_gain_mode_delta;
                if (tia_mode > 15) {
                    /* TIA hard limit high */
                    tia_mode = 15;
                }
            }
            PHY_PAPD(("%s core%d after gain ctrl: erstp %d, calidx %d, tiagain %d\n",
                __FUNCTION__, core, error_stop_index, papd_calidx, tia_mode));
            cals_on_the_core++;
            wbstate = SECOND_GCTRL;
        } else if ((wbstate == FIRST || wbstate == BBMULT_GCTRL) &&
            (wbpapd_bbm_gctrl_iter-- > 0)) {
            /* First make sure stop_idx is correctly set for this iteration */
            if (multi_tbl_en) {
                if (papd_lut_select == 0) {
                    MOD_PHYREGCEE(pi, PapdCompStopIdxLutSel1, for_core,
                        papd_comp_stop_index_lut0, stop_idx);
                } else {
                    MOD_PHYREGCEE(pi, PapdCompStopIdxLutSel1, for_core,
                        papd_comp_stop_index_lut1, stop_idx);
                }
            } else {
                MOD_PHYREGCEE(pi, PapdCompStopIdxLutSel1, for_core,
                papd_comp_stop_index_lut0, stop_idx);
            }
            cals_on_the_core++;

            /* Coarse bbmult gain control */
            eps_stop_idx = phy_ac_papd_find_maxamam_endfill_epstbl(pi, core, TRUE);
            if (eps_stop_idx > 62) {
                /* Eps table overstretched, increase digital gain
                 * to saturate earlier.
                 */
                bbmult = (bbmult * 126 + 50) / 100;
                wbstate = BBMULT_GCTRL;
            } else if (eps_stop_idx < 54) {
                /* Eps table compressed too much, reduce digital gain
                 * to saturate later.
                 */
                bbmult = (bbmult * 100 + 50) / 126;
                wbstate = BBMULT_GCTRL;
            } else {
                /* Coarse bbmult GC done - do fine bbmult gain control */
                bbm_corr_dB = 62 - eps_stop_idx;
                if (bbm_corr_dB < 0) {
                    /* No correction needed */
                    bbm_corr_dB = 0;
                }
                if (bbm_corr_dB > 3) {
                    /* Need one more fine correction run */
                    wbpapd_bbm_gctrl_iter = 0;
                    /* Taylor series of inverse of 20log */
                    bbm_corr_multipl = 100000 + (5750 * bbm_corr_dB) +
                        (166 * bbm_corr_dB * bbm_corr_dB);
                    /* Do rounding and scale back */
                    bbmult = (uint16)((((int32)bbmult) * 100000 + 50000) /
                        bbm_corr_multipl);
                    wbstate = BBMULT_GCTRL;
                } else {
                    /* Done */
                    if (cal_epsOffset_en) {
                        wbstate = CAL_EPSOFFSET;
                        /* Save the eps table from the real cal. */
                        wlc_phy_table_read_acphy_dac_war(pi,
                            eps_table_ids[core], tbl_size,
                            tbl_size * papd_lut_select, 32, eps_table,
                            core);
                    } else {
                        wbstate = FIRST;
                        cals_on_the_core = 0;
                        set_epsilon_offset(pi, for_core,
                            (wbpapd_gctrl > 0),
                            calc_epsoffset);
                        /* Go to cal next table. */
                        papd_lut_select++;
                    }
                }
            }
            if (bbmult > 159) {
                /* After scaling this is max 127 */
                bbmult = 159;
            }
            cals_on_the_core++;
            PHY_PAPD(("%s core%d eps_stop_idx=%d iter=%d next_gctrl_bbmult=%d "
                    "next_wbstate=%d\n", __FUNCTION__, core, eps_stop_idx,
                    wbpapd_bbm_gctrl_iter, bbmult, wbstate));
        } else if (wbstate == FIRST || wbstate == SECOND_GCTRL ||
                    wbstate == BBMULT_GCTRL) {
            if ((wbpapd_gctrl > 0) && !cal_epsOffset_en) {
                // error_stop_index small or already done 2 cals.
                // Step 3. Calculate epsilonOffset.
                offsetOfOffset = 2 * diff_cal_idx + error_stop_index;
                calc_epsoffset = epsilon_offset + offsetOfOffset;
                PHY_PAPD(("%s core%d EpsOffset cal: erstp %d, eps offset %d\n",
                    __FUNCTION__, core, error_stop_index, calc_epsoffset));
            }
            if (cal_epsOffset_en) {
                wbstate = CAL_EPSOFFSET;
                cals_on_the_core++;
                // Save the eps table from the real cal.
                wlc_phy_table_read_acphy_dac_war(pi, eps_table_ids[core], tbl_size,
                    tbl_size * papd_lut_select, 32, eps_table, core);
            } else {
                wbstate = FIRST;
                cals_on_the_core = 0;
                set_epsilon_offset(pi, for_core, (wbpapd_gctrl > 0),
                    calc_epsoffset);
                // Go to cal next table.
                papd_lut_select++;
            }
            if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
                if (multi_tbl_en) {
                    if (papd_lut_select == 0) {
                        MOD_PHYREGCEE(pi, PapdCompStopIdxLutSel1, for_core,
                            papd_comp_stop_index_lut0, stop_idx);
                    } else {
                        MOD_PHYREGCEE(pi, PapdCompStopIdxLutSel1, for_core,
                            papd_comp_stop_index_lut1, stop_idx);
                    }
                } else {
                    MOD_PHYREGCEE(pi, PapdCompStopIdxLutSel1, for_core,
                        papd_comp_stop_index_lut0, stop_idx);
                }
            } else {
                MOD_PHYREGCEE(pi, PapdPolarSaturation0, core,
                    stop_index, stop_idx);
            }
        } else {
            // wbstate == CAL_EPSOFFSET
            cal_epsOffset_success = calculate_calibrated_epsOffset(pi, core,
                papd_calidx, papd_lut_select, wbcal_gfrac_bits, eps_table,
                &calc_epsoffset);
            if (cal_epsOffset_success) {
                wbstate = FIRST;
                cals_on_the_core = 0;
                // Go to cal next table.
                papd_lut_select++;
                set_epsilon_offset(pi, for_core, TRUE, calc_epsoffset);
            } else {
                // Continue doing the cal of epsilonOffset.
                cals_on_the_core++;
            }
        }
    }

    /* Set comp to same precisison as Cal: 11 bits */
    if (wbcal_gfrac_bits == 11)
        params->wbcal_eps_fxdpt = 0;
    else if (wbcal_gfrac_bits == 10)
        params->wbcal_eps_fxdpt = 2;
    else
        params->wbcal_eps_fxdpt = 1; /* 12 bit case */

    MOD_PHYREGCE(pi, EpsilonOverrideI_, for_core, epsilonFixedPoint, params->wbcal_eps_fxdpt);
    MOD_PHYREGCEE(pi, PapdLutSel0, for_core, papd_lut_select_ovr, 0);
    if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
        // Not to use the low power noisy eps table entries in papd comp.
        MOD_PHYREGCEE(pi, PapdPolarSaturation3, for_core, start_index, 10);
    }

    /* Restore the value of PAPD COMP enable bit */
    MOD_PHYREGCEE(pi, PapdEnable, core, papd_compEnb, 0x1);
    MOD_PHYREGCEE(pi, PapdEnable, for_core, papd_compEnb, 0x1);

    /* In epapd, disable FEM's on-chip coupler. */
    if (PHY_EPAPD(pi) && swctrl->valid) {
        MOD_PHYREGCE(pi, FemOutputOvrCtrl, for_core, femCtrlOutputOvr, 0);
        MOD_PHYREGCE(pi, FemOutputOvrCtrl, for_core, femCtrlOutput, 0);
        if (xcore_wbpapd_cal_en) {
            MOD_PHYREGCE(pi, FemOutputOvrCtrl, core, femCtrlOutputOvr, 0);
            MOD_PHYREGCE(pi, FemOutputOvrCtrl, core, femCtrlOutput, 0);

            /* Restore all contexts of $mimocfg(on_core) */
            MOD_PHYREGCEE(pi, PapdCompStopIdxLutSel1, core,
                papd_comp_stop_index_lut0, saved_eps_stop_idx);
            MOD_PHYREGCEE(pi, PapdLutSel1, core, papd_index_offset_lut0,
                saved_papd_index_offset_lut);
            MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset,
                saved_epsilonOffset);

            // Load on_core eps table to for_core.
            wlc_phy_table_read_acphy_dac_war(pi, eps_table_ids[core],
                tbl_size, 0, 32, eps_table, core);
            wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[for_core],
                tbl_size, 0, 32, eps_table, for_core);

            // Load original on_core eps table back to on_core.
            wlc_phy_table_write_acphy_dac_war(pi, eps_table_ids[core],
                tbl_size, 0, 32, orig_eps_table, core);
        }
    }

    phy_mfree(pi, eps_table, tbl_size * sizeof(*eps_table));
    phy_mfree(pi, orig_eps_table, tbl_size * sizeof(*orig_eps_table));
    phy_mfree(pi, tmp_table, 256 * sizeof(*tmp_table));

    if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
        /* Fill in end of table with the eps coef with maximum AMAM */
        phy_ac_papd_find_maxamam_endfill_epstbl(pi, core, FALSE);
    }

fail:
    return err;
}

static int8
phy_ac_wbcal_eps_stopidx(phy_info_t *pi, uint8 core)
{
    uint16 eps_table_size = phy_ac_papdcal_eps_table_size(pi, core);
    uint8 stop_idx = eps_table_size - 1;
    uint32 eps_table_entry;
    uint8 idx;
    uint8 papd_lut_select = 0;
    int32 multi_tbl_en = pi->u.pi_acphy->papdcali->wbpapd_multitbl_iovar;
    uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};

    if (multi_tbl_en) {
        papd_lut_select = READ_PHYREGFLDCEE(pi, PapdLutSel0, core, papd_lut_select_ovr_val);
    }

    for (idx = stop_idx; idx > 0; idx--) {
        wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core], 1,
            idx + eps_table_size * papd_lut_select, 32, &eps_table_entry, core);
        if (eps_table_entry > 0) {
            break;
        }
    }

    stop_idx = idx;
    if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev)) {
        stop_idx >>= 1;
    }
    return stop_idx;
}

static void
phy_ac_papd_tiagain_search_majrev129(phy_info_t *pi, uint8 core, uint16 tiagain_bbmult,
                                     bool tia_2nd)
{
    uint16 m[4] = {0, 0, 0, 0};
    uint8 allcoremask = pi->pubpi->phy_coremask;
    int8 tiagain_final, tx_idx, i;
    int8 coremask = 1 << core;
    int8 Pd2W = 52;
    uint32 adc_pow_est_dBx8, adc_pow_est, adc_pow_limit;
    int8 adc_pow_offset = 0;
    bool is40M = (CHSPEC_IS40(pi->radio_chanspec)), is80M = (CHSPEC_IS80(pi->radio_chanspec));
    uint16 fc = CHAN5G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_papdcal_params_t *params    = aci->papdcali->papd_params;

    /* Set tx_idx */
    if (aci->papdcali->pacalidx_iovar != -1) {
        /* 1st priority: force cal index through iovar */
        tx_idx = aci->papdcali->pacalidx_iovar;
    } else {
        /* Currently not reading tx index from NVRAM */
        tx_idx = (CHSPEC_IS2G(pi->radio_chanspec)) ? params->papd_calidx_2g :
            params->papd_calidx_5g;
    }

    /* Set bbmult */
    wlc_phy_txpwr_by_index_acphy(pi, coremask, tx_idx);
    m[core] = tiagain_bbmult;
    wlc_phy_ipa_set_bbmult_acphy(pi, &m[0], &m[1], &m[2], &m[3], allcoremask);

    if (!tia_2nd) {
        MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain, params->tia_mode_5g);
        /* Special IDAC cal for TIA gain in PAPD loopback path */
        phy_ac_dccal_papd_special(pi, params->tia_mode_5g, core);

        /* Offset ADC output power to compensate additional white noise for 40M and 80M */
        if (is40M || is80M) {
            adc_pow_offset = 3;
        }
        adc_pow_est_dBx8 = phy_ac_papd_adc_pow_est(pi, core, TRUE) - adc_pow_offset;
        tiagain_final = params->tia_mode_5g + (Pd2W*8 - adc_pow_est_dBx8) / 24;
        if (tiagain_final > 9) {
            /* TIA hard limit high */
            tiagain_final = 9;
        } else if (tiagain_final < 1) {
            /* TIA hard limit low */
            tiagain_final = 1;
        }
        papd_tiagain_1st[core] = tiagain_final;
    } else {
        /* Force TIA gain through iovar */
        if (aci->papdcali->papdtiagain_iovar != -1) {
            tiagain_final = aci->papdcali->papdtiagain_iovar;
        } else {
            /* Search for the largest TIA gain without exceeding ADC power limit. */
            if (fc <= 5320) {
                /* x = 10 ^ ( a / 80 ), a = 433 (80M), 428 (40M), 425 (20M) */
                adc_pow_limit = is80M ? 258523 : is40M ? 223872 : 205353;
            } else if (fc <= 5710) {
                /* x = 10 ^ ( a / 80 ), a = 428 (80M), 415 (40M), 410 (20M) */
                adc_pow_limit = is80M ? 223872 : is40M ? 153993 : 133352;
            } else {
                /* x = 10 ^ ( a / 80 ), a = 400 (80M), 393 (40M), 388 (20M) */
                adc_pow_limit = is80M ? 100000 : is40M ? 81752 : 70795;
            }

            for (i = 9; i >= 1; i--) {
                tiagain_final = i;
                MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain, i);
                /* Special IDAC cal for TIA gain in PAPD loopback path */
                phy_ac_dccal_papd_special(pi, i, core);
                adc_pow_est = phy_ac_papd_adc_pow_est(pi, core, FALSE);
                /* Higher noise power for 80M so larger adc_pow_limit. */
                if (adc_pow_est <= adc_pow_limit) {
                    break;
                }
            }
        }
        papd_tiagain[core] = tiagain_final;
        PHY_PAPD(("wl%d %s: cal_tiagain core %d: %d\n", pi->sh->unit, __FUNCTION__,
            core, tiagain_final));
    }
    MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_tia_gain, tiagain_final);
    phy_ac_dccal_papd_special(pi, tiagain_final, core);
}

static uint32
phy_ac_papd_adc_pow_est(phy_info_t *pi, uint8 core, bool dBx8)
{
    uint16 num_samps = 2048;
    uint32 i_pwr, q_pwr, adc_pow_est;
    int16 adc_pow_est_dBx8, temp;

    phy_iq_est_t est[PHY_CORE_MAX];

    /* Turn on test tone */
    (void)wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_1MHz, 186,
    TX_TONE_IQCAL_MODE_OFF, FALSE);
    OSL_DELAY(100);

    wlc_phy_rx_iq_est_acphy(pi, est, num_samps, 32, 0, FALSE);

    /* Turn off test tone */
    wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);

    i_pwr = (est[core].i_pwr + num_samps / 2) / num_samps;
    q_pwr = (est[core].q_pwr + num_samps / 2) / num_samps;
    adc_pow_est = i_pwr + q_pwr;
    if (!dBx8) {
        return adc_pow_est;
    } else {
        qm_log10((int32)(100*adc_pow_est), 0, &adc_pow_est_dBx8, &temp);
        adc_pow_est_dBx8 = (((10*adc_pow_est_dBx8) - (20 << temp)) << 3) >> temp;
        return (uint32)adc_pow_est_dBx8;
    }
}

static int
phy_ac_papd_mac_play(phy_info_t *pi, const uint32* buf, uint32 start_ptr, uint32 len, bool start,
    bool load)
{
    phy_ac_papdcal_params_t *params = pi->u.pi_acphy->papdcali->papd_params;
    uint32 startidx = params->wbcal_macbuf_offset, stopidx;
    uint16 startidx_low, startidx_high, stopidx_low, stopidx_high, SampleCollectPlayPtrHigh;
    uint16 play_mode = 0x01u;
    uint8 sampleCmdEnable;
    uint8 dac_rate_mode = params->dac_rate_mode;

    if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS80(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec))
            dac_rate_mode = params->dac_rate_mode_80M_160M;
    }

    if (start) {
        if (load) {
            /* Load the waveform into MAC buffer */
            ASSERT(buf != NULL);
            // Times 8 because of 8x upsampling.
            // Times 4 because of scaling & repetition in phy_ac_papd_mac_load.
            phy_ac_papd_mac_load(pi, buf, len/8/4);

            if (dac_rate_mode == 1) {
                if (CHSPEC_IS160(pi->radio_chanspec)) {
                    play_mode = 0x03u;
                } else if (CHSPEC_IS80(pi->radio_chanspec) ||
                    PHY_AS_80P80(pi, pi->radio_chanspec)) {
                    play_mode = 0x03u;
                } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                    play_mode = 0x02u;
                }
            } else if (dac_rate_mode == 2) {
                play_mode = 0x03u;
            }
            MOD_PHYREG(pi, macbasedDACPlay, macBasedDACPlayMode, play_mode);
        }
        /* Update SamplePlay start and stop pointers of 32-bit words. */
        startidx >>= 2;
        startidx += start_ptr;
        stopidx = startidx + len - 2;
        startidx_low = (uint16)startidx;
        startidx_high = (uint16)(startidx >> 16);
        stopidx_low = (uint16)stopidx;
        stopidx_high = (uint16)(stopidx >> 16);
        if (ACMAJORREV_130_131(pi->pubpi->phy_rev) && (stopidx_low % 2)) {
            /* For 64-bit sample play, LSB has to be 0. */
            stopidx_low &= 0xfffeu;
        }
        W_REG(pi->sh->osh, D11_SPP_STRTPTR(pi), startidx_low);
        W_REG(pi->sh->osh, D11_SPP_STOPPTR(pi), stopidx_low);

        /* Update the high address of start and stop ptr */
        SampleCollectPlayPtrHigh = R_REG(pi->sh->osh, D11_SMP_PTR_H(pi));
        SampleCollectPlayPtrHigh &= ~(SAMPLE_PLAY_START_PTR_HIGH_MASK |
            SAMPLE_PLAY_STOP_PTR_HIGH_MASK);
        SampleCollectPlayPtrHigh |= ((stopidx_high & UINT16_LOW_NIBBLE_MASK) <<
            SAMPLE_PLAY_STOP_PTR_HIGH_SHIFT);
        SampleCollectPlayPtrHigh |= ((startidx_high & UINT16_LOW_NIBBLE_MASK) <<
            SAMPLE_PLAY_START_PTR_HIGH_SHIFT);
        W_REG(pi->sh->osh, D11_SMP_PTR_H(pi), SampleCollectPlayPtrHigh);

        sampleCmdEnable = READ_PHYREGFLD(pi, sampleCmd, enable);
        if (sampleCmdEnable)
            MOD_PHYREG(pi, sampleCmd, stop, 0x1);
        else
            MOD_PHYREG(pi, sampleCmd, enable, 0x1);

        /* Play the Waveform */
        MOD_PHYREG(pi, macbasedDACPlay, macBasedDACPlayEn, 0x01u);

        PHY_TRACE(("Starting MAC based Sample Play"));
        wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);

        if (D11REV_IS(pi->sh->corerev, 50) || D11REV_GE(pi->sh->corerev, 53)) {
            uint16 SampleCollectPlayCtrl =
                R_REG(pi->sh->osh, D11_SMP_CTRL(pi));
            SampleCollectPlayCtrl |= (1 << SAMPLE_COLLECT_PLAY_CTRL_PLAY_START_SHIFT);
            // Clear the single packe mode bit
            SampleCollectPlayCtrl &= SAMPLE_COLLECT_PLAY_CTRL_SINGLE_PKT_MASK;
            W_REG(pi->sh->osh, D11_SMP_CTRL(pi),
                    SampleCollectPlayCtrl);
        } else {
            uint16 phy_ctl;
            phy_ctl = (1 << PHYCTRL_SAMPLEPLAYSTART_SHIFT)
                | (1 << PHYCTRL_MACPHYFORCEGATEDCLKSON_SHIFT);
            W_REG(pi->sh->osh, D11_PSM_PHY_CTL(pi), phy_ctl);
        }
    } else {
        /* To stop playback */
        bool mac_sample_play_on = 0;
        if (D11REV_IS(pi->sh->corerev, 50) || D11REV_GE(pi->sh->corerev, 53)) {
            uint16 SampleCollectPlayCtrl =
                R_REG(pi->sh->osh, D11_SMP_CTRL(pi));
            mac_sample_play_on =
                ((SampleCollectPlayCtrl >>
                  SAMPLE_COLLECT_PLAY_CTRL_PLAY_START_SHIFT) & 1);

            if (mac_sample_play_on) {
                /* Make the 9th bit zero */
                SampleCollectPlayCtrl &= 0xFDFF;
                W_REG(pi->sh->osh, D11_SMP_CTRL(pi),
                    SampleCollectPlayCtrl);
            }
        } else {
            uint16 phy_ctl;
            phy_ctl = R_REG(pi->sh->osh, D11_PSM_PHY_CTL(pi));

            if (phy_ctl & 0x800) {
                W_REG(pi->sh->osh, D11_PSM_PHY_CTL(pi),
                    (uint16)(phy_ctl & 0xf7ff));
            }

        }
        // Wait for the data in the fifo in the MAC side to be drained,
        // and then stop the PHY side. This delay is critical, especially in full dongle.
        OSL_DELAY(10);
        /* Stop MAC play  by setting macBasedDACPlayEn to 0 */
        MOD_PHYREG(pi, macbasedDACPlay, macBasedDACPlayEn, 0);
    }  /* End of start */

    return BCME_OK;
}

/* scal_tbl_map_shift = -1.0 -0.75 -0.50 -0.25 0.00
 * +0.25  +0.50  +0.75  +1.00 +1.25
 * tbl_map_scale = 0.891 0.9173 0.9441 0.9716 1.0000
 * 1.0292 1.0593 1.0902 1.1220 1.1548
 * tbl_map_scale = 3649 3757 3867 3980 4096
 * 4216 4339 4465 4596 4730
 * tbl_map_scale can be tuned to reduce IFS dependence
 *
 * int16  scal_tbl_map_shift = 1;
 * int16 tbl_map_scale_2G = 4596;
 * int16 tbl_map_scale_5G_20[4] = {4596, 4596, 4596, 4596};
 * int16 tbl_map_scale_5G_40[4] = {4596, 4596, 4596, 4596};
 * int16 tbl_map_scale_5G_80[4] = {4596, 4596, 4596, 4596};
 * int16 tbl_map_scale_2G_formin1 = 3649;
 * int16 tbl_map_scale_2G_forp75 = 4465;
 * int16 tbl_map_scale_2G_forminp75 = 3757;
 * int16 tbl_map_scale_5G_20_forp75[4] = {4465, 4465, 4465, 4465};
 * int16 tbl_map_scale_5G_40_forp75[4] = {4465, 4465, 4465, 4465};
 * int16 tbl_map_scale_5G_80_forp75[4] = {4465, 4465, 4465, 4465};
 * just made scal_tbl_map_shift *100, since float is not accepted
 * scal_tbl_map_shift 100 = 1 , -100 = -1, 75 = 0.75
 */

/* TCL PAPDCAL Mode 3 - Analytic Cal */
void
phy_ac_papd_cal_mode3(phy_info_t *pi, acphy_papdCalParams_t *calParams)
{
    uint8 core = calParams->core, epsilon_table_id = calParams->epsilon_table_id;
    uint16 startindex = calParams->startindex;
    uint16 yrefindex = calParams->yrefindex;
    uint16 corr_end = 79;
    uint16 idx;
    int16  corr_I = 0, corr_Q = 0;
    int16  Yref_I = 0, Yref_Q = 0;
    int32  eps_den = 0;
    uint16  abs_c = 0;
    int16  eps_I_curr = 0, eps_Q_curr = 0;
    int16  tbl_map_scale = 1;

    /* fixed-point fractional bits shift */
    uint16 fixpt_shift_bits = 12;
    /* eps interpolation */
    int16  eps_I_prev = 0, eps_Q_prev = 0;
    int16  eps_I_interp_curr = 0, eps_Q_interp_curr = 0;
    uint16 scal_tbl_interp = 0, scal_tbl_map_prev = 0, scal_tbl_map_curr = 0;
    uint16 next_idx_interp_init = 0, next_idx_interp = 0;
    uint32 dst;

    uint16 scal_tbl[APAPD_ARRAYSIZE], scal_tbl_map[APAPD_ARRAYSIZE];
    int16 eps_I[APAPD_ARRAYSIZE], eps_Q[APAPD_ARRAYSIZE], eps_I_interp[APAPD_ARRAYSIZE],
          eps_Q_interp[APAPD_ARRAYSIZE];

    /* Changing scal tbl map shift to reduce EVM dependance on IFS */
    if ((ROUTER_4349(pi)) && (CHSPEC_ISPHY5G6G(pi->radio_chanspec)))
        tbl_map_scale = 4596;
    else
        tbl_map_scale = 4096;

    for (idx = startindex; idx <= corr_end; idx++) {
        scal_tbl[idx-startindex] = (uint16)(acphy_papd_scaltbl_128[idx]
                & 0xffff);
        eps_I[idx-startindex] = 0;
        eps_Q[idx-startindex] = 0;
    }

    phy_ac_apapd_init_seq(pi, core, yrefindex);
    (void)wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_1MHz, 186, TX_TONE_IQCAL_MODE_OFF,
        FALSE);

    for (idx = 0; idx <= corr_end; idx++) {
        MOD_PHYREG(pi, PapdCalAddress, papdStartAddr, idx);
        MOD_PHYREG(pi, PapdCalAddress, papdEndAddr, idx);
        WRITE_PHYREG(pi, papdCalCorrDebugAddr, idx);

        MOD_PHYREG(pi, PapdCalCoreSel, papdCoreSel, core);
        OSL_DELAY(10);
        MOD_PHYREG(pi, PapdCalStart, papdStart, 1);

        SPINWAIT(READ_PHYREG(pi, PapdCalStart), ACPHY_SPINWAIT_PAPDCAL);
        if ((READ_PHYREG(pi, PapdCalStart) & 1)) {
            PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :"
                " PAPD cal failed \n", __FUNCTION__));
            PHY_FATAL_ERROR(pi, PHY_RC_PAPD_CAL_FAILED);
        }
        SPINWAIT(READ_PHYREG(pi, PapdCalStart), ACPHY_SPINWAIT_PAPDCAL);
        if (idx >= startindex) {
            corr_I = (int16) (READ_PHYREGCE(pi, papdCalFirstCorr_I, core)
                    & 0xffff);
            corr_Q = (int16) (READ_PHYREGCE(pi, papdCalFirstCorr_Q, core)
                    & 0xffff);
            Yref_I = (int16) (READ_PHYREGCE(pi, PapdCalYref_I, core) & 0xffff);
            Yref_Q = (int16) (READ_PHYREGCE(pi, PapdCalYref_Q, core) & 0xffff);

            /* 1+eps = c = Yref/Y = Yref*conj(Y)/abs(Y)^2 */
            eps_den = (int32)corr_I*(int32)corr_I +
                (int32)corr_Q*(int32)corr_Q;
            /* prevent divide-by-zero: if (eps_den >> 12) */
            if (eps_den >> 12) {
                eps_I_curr = (((int32)corr_I*(int32)Yref_I +
                    (int32)corr_Q*(int32)Yref_Q)/(eps_den >> 12))
                    - 4096;
                eps_Q_curr =  ((int32)corr_Q*(int32)Yref_I -
                    (int32)corr_I*(int32)Yref_Q)/(eps_den >> 12);
            } else {
                eps_I_curr = 0;
                eps_Q_curr = 0;
            }

            eps_I[idx-startindex] = eps_I_curr;
            eps_Q[idx-startindex] = eps_Q_curr;

            abs_c = (uint16)math_sqrt_int_32(
                    (uint32)(((int32)4096 + (int32)eps_I_curr) *
                        ((int32)4096 + (int32)eps_I_curr) +
                        ((int32)eps_Q_curr*(int32)eps_Q_curr)));
            /* prevent divide-by-zero */
            abs_c = (abs_c == 0)? 1:abs_c;
            scal_tbl_map[idx-startindex] = (uint16)
                ((((uint32)scal_tbl[idx-startindex])<<12)/abs_c);

            /* shift PA curve to better fit PA in normal operation */
            scal_tbl_map[idx-startindex] = (uint16)
                (((uint32)scal_tbl_map[idx-startindex] * tbl_map_scale)
                 >> fixpt_shift_bits);
        }
    }
    wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);

    /* next_idx_interp */
    while ((next_idx_interp_init <= corr_end - startindex) &&
            (scal_tbl[next_idx_interp_init] < scal_tbl_map[0])) {
        eps_I_interp[next_idx_interp_init] = 0;
        eps_Q_interp[next_idx_interp_init] = 0;
        next_idx_interp_init++;
    }
    next_idx_interp = next_idx_interp_init;

    for (idx = 0; idx < corr_end-startindex; idx++) {
        scal_tbl_map_prev = scal_tbl_map[idx];
        scal_tbl_map_curr = scal_tbl_map[idx+1];
        eps_I_prev = eps_I[idx];
        eps_Q_prev = eps_Q[idx];
        eps_I_curr = eps_I[idx+1];
        eps_Q_curr = eps_Q[idx+1];
        while ((next_idx_interp <= corr_end-startindex) &&
                (scal_tbl[next_idx_interp] >= scal_tbl_map_prev) &&
                (scal_tbl[next_idx_interp] < scal_tbl_map_curr)) {
            scal_tbl_interp = scal_tbl[next_idx_interp];
            eps_I_interp_curr = (int16)
                (((int32)eps_I_prev *
                  (int32)(scal_tbl_map_curr - scal_tbl_interp) +
                  (int32)eps_I_curr *
                  (int32)(scal_tbl_interp - scal_tbl_map_prev))/
                 (int16)(scal_tbl_map_curr - scal_tbl_map_prev));
            eps_Q_interp_curr = (int16)
                (((int32)eps_Q_prev *
                  (int32)(scal_tbl_map_curr - scal_tbl_interp) +
                  (int32)eps_Q_curr *
                  (int32)(scal_tbl_interp - scal_tbl_map_prev))/
                 (int16)(scal_tbl_map_curr - scal_tbl_map_prev));
            eps_I_interp[next_idx_interp - next_idx_interp_init] =
                (eps_I_interp_curr >= 4095)?
                4095 : (eps_I_interp_curr <= -4096)?
                -4096 : eps_I_interp_curr;
            eps_Q_interp[next_idx_interp - next_idx_interp_init] =
                (eps_Q_interp_curr >= 4095)?
                4095 : (eps_Q_interp_curr <= -4096)?
                -4096 : eps_Q_interp_curr;
            next_idx_interp++;
        }
    }

    /* write table */
    if (next_idx_interp_init != next_idx_interp) {
        for (idx = 0; idx <= 63; idx++) {
            if (idx < startindex + next_idx_interp_init) {
                dst = 0x0;
                wlc_phy_table_write_acphy_dac_war(pi,
                    epsilon_table_id, 1, idx, 32, &dst, core);
            } else if (idx < (next_idx_interp + startindex)) {
                dst = (uint32)((((uint32)eps_Q_interp
                    [idx-startindex-next_idx_interp_init]
                    & 0x1fff) << 13) | ((uint32)eps_I_interp
                    [idx-startindex-next_idx_interp_init]
                    & 0x1fff));
                wlc_phy_table_write_acphy_dac_war(pi,
                    epsilon_table_id, 1, idx, 32, &dst, core);
            } else {
                    dst = (uint32)((((uint32)eps_Q_interp
                        [next_idx_interp-next_idx_interp_init-1]
                        & 0x1fff) << 13) | ((uint32)eps_I_interp
                        [next_idx_interp-next_idx_interp_init-1]
                        & 0x1fff));
                wlc_phy_table_write_acphy_dac_war(pi,
                    epsilon_table_id, 1, idx, 32, &dst, core);
            }
        }
    } else {
        dst = (uint32)
            ((((int32)eps_Q[corr_end-startindex] & 0x1fff) << 13) |
            ((int32)eps_I[corr_end-startindex] & 0x1fff));
        wlc_phy_table_write_acphy_dac_war(pi, epsilon_table_id,
                1, 63, 32, &dst, core);

    }
}

void
phy_ac_papd_cal_eps_calc_tiny(phy_info_t *pi, uint8 core, uint16 *bbmult)
{
    bool is2g = (CHSPEC_IS2G(pi->radio_chanspec)),
        is5g = (CHSPEC_ISPHY5G6G(pi->radio_chanspec)),
        is80M = (CHSPEC_IS80(pi->radio_chanspec));
    uint8 scalar_table_ids[] = { ACPHY_TBL_ID_SCALAR0, ACPHY_TBL_ID_SCALAR1,
        ACPHY_TBL_ID_SCALAR2, ACPHY_TBL_ID_SCALAR3};
    uint32 scalartblval, papdmult, epsilonscalartemp;
    int8 k;
    int16 cal_tone_mag = 186;
    int16 temp, temp1, qQ1, lut_shift, epsilonoffsettemp, dac_rf_offset;
    int32 dig_gain_dB;
    uint8 subbandidx = 0;
    uint16 fc;
    uint8 channel = CHSPEC_CHANNEL(pi->radio_chanspec);
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)pi->u.pi_acphy->papdcali;
    phy_ac_papdcal_params_t *papd_params = papdcali->papd_params;
    ASSERT(papdcali != NULL);

    wlc_phy_table_read_acphy(pi, scalar_table_ids[core], 1, 0, 32, &scalartblval);
    papdmult = scalartblval & 0x1fff;

    if (ACMAJORREV_129_130(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* Increase calculation precisison */
        temp = ((*bbmult)*cal_tone_mag*papdmult*2000)/(64*1024*100);
        qm_log10((int32)(temp), 0, &temp1, &qQ1);
        dig_gain_dB = (((20*temp1) - (66 << qQ1)) << 3) >> qQ1;
    } else {
        if (WBPAPD_ENAB(pi)) {
            /* Scale up 1000x log10 input argument due to integer division */
            temp = (int16)((((int32)(*bbmult)) * 1000)/64);
            qm_log10((int32)(temp), 0, &temp1, &qQ1);
            /* Need to subtract -3 from log10 output due to input x1000: this is -60 */
            /* Need to also add +0.5 for rounding */
            dig_gain_dB = (20 * ((int32)(temp1)) - (60 << qQ1) + (1 << (qQ1 - 1))) >>
                qQ1;
        } else {
            temp = ((*bbmult)*cal_tone_mag*papdmult*1000)/(64*1024*100);
            qm_log10((int32)(temp), 0, &temp1, &qQ1);
            dig_gain_dB = ((20*temp1) - (60 << qQ1)) >> qQ1;
        }
    }

    if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
        lut_shift = 0;
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        if (is2g) {
            lut_shift = 1;
        } else {
            lut_shift = 0;
        }
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        fc = is2g ? CHAN2G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec))
                      : CHAN5G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));
        lut_shift = is2g ? 3 : (fc <= 5710) ? 1 : 2;
    } else {
        lut_shift = -2;
    }
    lut_shift = lut_shift + papdcali->extraepsoffset_iovar;

    if (is5g && ACMAJORREV_128(pi->pubpi->phy_rev)) {
        fc = CHAN5G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec));

        /* pacalshift5gaX=lo,mi,hi,x1,lo,mi,hi,x1,lo,mi,hi,x1
         *                |    20MHz |   40MHz   |   80MHz   |
         */
        if (fc >= 5180 && fc <= 5320) {
            subbandidx = 0;
        } else if (fc >= 5500 && fc <= 5620) {
            subbandidx = 1;
        } else if (fc >= 5630 && fc <= 5720) {
            subbandidx = 2;
        } else if (fc >= 5725 && fc <= 5825) {
            subbandidx = 3;
        }
    }

    if (ACREV_GE(pi->pubpi->phy_rev, 4)) {
        if (APAPD_ENAB(papdcali)) {
            if (is2g) {
                lut_shift = -2;
            } else {
                lut_shift = (ACMINORREV_2(pi))? -1 :
                    (is80M) ? -3 : (channel < 100)? -2 : -3;
            }
            if (ROUTER_4349(pi)) {
                lut_shift = (is2g) ? -2 :
                    (is80M) ? 1 : -2;
            }
        } else {
            if (is5g && (!PHY_EPAPD(pi)) &&
                ACMAJORREV_4(pi->pubpi->phy_rev)) {
                lut_shift = (IS20MHZ(pi)) ? 0 : 2;
            }
            if (is5g) {
                if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
                    if (CHSPEC_IS20(pi->radio_chanspec))
                        lut_shift += (core == 0) ?
                            papdcali->pacalshift5ga0[0 + subbandidx] :
                            papdcali->pacalshift5ga1[0 + subbandidx];
                    else if (CHSPEC_IS40(pi->radio_chanspec))
                        lut_shift += (core == 0) ?
                            papdcali->pacalshift5ga0[4 + subbandidx] :
                            papdcali->pacalshift5ga1[4 + subbandidx];
                    else if (CHSPEC_IS80(pi->radio_chanspec))
                        lut_shift += (core == 0) ?
                            papdcali->pacalshift5ga0[8 + subbandidx] :
                            papdcali->pacalshift5ga1[8 + subbandidx];
                } else {
                    if (IS20MHZ(pi)) {
                        lut_shift += papdcali->pacalshift5g[0];
                    } else if (IS40MHZ(pi)) {
                        lut_shift += papdcali->pacalshift5g[1];
                    } else {
                        lut_shift += papdcali->pacalshift5g[2];
                    }
                }
            } else {
                if (IS20MHZ(pi)) {
                    lut_shift += papdcali->pacalshift2g[0];
                } else if (IS40MHZ(pi)) {
                    lut_shift += papdcali->pacalshift2g[1];
                } else {
                    lut_shift += papdcali->pacalshift2g[2];
                }
            }
        }
    }
    k = -80;
    dac_rf_offset = READ_PHYREGFLDCEE(pi, PapdEnable, core, gain_dac_rf_reg);
    if (dac_rf_offset >= 256) {
        dac_rf_offset = dac_rf_offset - 512;
    }
    epsilonscalartemp = READ_PHYREGFLDCEE(pi, EpsilonTableAdjust, core, epsilonScalar);
    if (ACMAJORREV_129_130(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* Increase calculation precisison */
        epsilonoffsettemp = k - 2*dig_gain_dB/8 + lut_shift -
            dac_rf_offset*epsilonscalartemp/16;
    } else if (ACMAJORREV_131(pi->pubpi->phy_rev) && WBPAPD_ENAB(pi)) {
        epsilonoffsettemp = -24 - 2*dig_gain_dB + lut_shift -
            (CHSPEC_IS2G(pi->radio_chanspec) ? papd_params->cal_refdb_2g :
            papd_params->cal_refdb_5g);
    } else {
        epsilonoffsettemp = k - 2*dig_gain_dB + lut_shift -
            dac_rf_offset*epsilonscalartemp/16;
    }
    if (epsilonoffsettemp < 0) {
        epsilonoffsettemp = 512 + epsilonoffsettemp;
    }
    MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset,
        epsilonoffsettemp);
}

void
phy_ac_papd_cal_set_tx_gain(phy_info_t *pi, uint8 core, uint16 *bbmult, uint8 *calmode)
{
    bool is2g = (CHSPEC_IS2G(pi->radio_chanspec)),
        is80M = (CHSPEC_IS80(pi->radio_chanspec));
    uint8 channel = CHSPEC_CHANNEL(pi->radio_chanspec);
    int8 tx_idx = 40;
    int8 coremask = 1 << core;
    uint16 m[4] = {0, 0, 0, 0};
    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    BCM_REFERENCE(aci);

    ASSERT(aci->papdcali != NULL);

    if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
        ACMAJORREV_128(pi->pubpi->phy_rev)) {
        phy_ac_papdcal_params_t *params    = pi->u.pi_acphy->papdcali->papd_params;

        if (aci->papdcali->pacalidx_iovar != -1) {
            /* 1st priority: force cal index through iovar */
            tx_idx = aci->papdcali->pacalidx_iovar;
        } else {
            /* Currently not reading tx index from NVRAM */
            tx_idx = (CHSPEC_IS2G(pi->radio_chanspec)) ? params->papd_calidx_2g :
                params->papd_calidx_5g;
        }
        if (is2g) {
            papd_gainctrl_calidx_2g[core] = tx_idx;
        } else {
            papd_gainctrl_calidx_5g[core] = tx_idx;
        }

        /* Set the TX index */
        aci->papdcali->papd_lut0_cal_idx = tx_idx;
        aci->papdcali->papd_lut1_cal_idx = tx_idx;

        wlc_phy_txpwr_by_index_acphy(pi, coremask, tx_idx);
        PHY_PAPD(("wl%d %s: cal_index core %d: %d\n", pi->sh->unit, __FUNCTION__,
            core, tx_idx));
        /* Optimize BB Mult setting */
        coremask = pi->pubpi->phy_coremask; /* all cores (active + inactive) */
        if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
            *bbmult = (int16)(is2g ? params->bbmult_2g[core] : params->bbmult_5g);
        } else if (ACMAJORREV_129_130(pi->pubpi->phy_rev) ||
            ACMAJORREV_128(pi->pubpi->phy_rev)) {
            *bbmult  = phy_ac_papd_gain_ctrl_28nm(pi, core, 0, 63, *bbmult);
        } else {
            /* No BBMULT set */
            ASSERT(FALSE);
        }
        /* force bbmult through iovar */
        if (aci->papdcali->papdbbmult_iovar != -1)
            *bbmult = aci->papdcali->papdbbmult_iovar;
    } else if (TINY_RADIO(pi))  {
        if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
            *calmode = 0;
            if (ROUTER_4349(pi)) {
                *bbmult = (is2g)? 90 : (is80M) ?
                    ((channel < 100) ? 55 : 65) :
                    ((core == 0) ? 60 : 50);
                tx_idx = (is2g)? 24 : 40;
                if ((channel == 38) || (channel == 151) || (channel == 42) ||
                    (channel == 106) || (channel == 159)) {
                    tx_idx = 65;
                }
            } else {

                *bbmult = phy_ac_papd_gain_ctrl_tiny(pi,
                        core, 5);
                tx_idx = (is2g) ? 24 : 30;
            }
            if ((phy_get_phymode(pi) == PHYMODE_MIMO))
                phy_ac_papd_turningoff_inactivecore(pi, core);

            /* Logic added to 'turn' off tone on inactive core */
            /* Need to turn off PA on the inactive core as the next step */
            wlc_phy_txpwr_by_index_acphy(pi, core + 1, tx_idx);
            coremask = (phy_get_phymode(pi) == PHYMODE_RSDB) ? 1 : 3;

        } else {
            *calmode = 1; /* Run single index of PAPD table only */

            *bbmult = (is2g) ? 64 : 60;
            if (PHY_EPAPD(pi))
                *bbmult = (is2g) ? 100 : 75;
            if ((is2g) && (aci->papdcali->papdbbmult2g != -1)) {
                *bbmult = aci->papdcali->papdbbmult2g;
            } else if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
                    (aci->papdcali->papdbbmult5g != -1)) {
                *bbmult = aci->papdcali->papdbbmult5g;
            }
            if (aci->papdcali->pacalmode != -1) {
                *calmode = aci->papdcali->pacalmode;
            }
        }
    }  else {
        *calmode = 0;  /* Run the PAPD automatic machine on all indices */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            *bbmult = 0x3f;
            if (RADIOMAJORREV(pi) == 2 && PHY_EPAPD(pi)) {
                *bbmult = (core == 0) ? 100 : 110;
            }
        } else
            *bbmult = 0x30;
    }

    m[core] = *bbmult;
    /* Setting appropriate bbmult for all tx cores */
    wlc_phy_ipa_set_bbmult_acphy(pi, &m[0], &m[1], &m[2], &m[3], coremask);
}

/* Function to save and restore papd cal PHY registers
 * sr = 1 => SAVE
 * sr = 0 => RESTORE
 */
void
acphy_papd_cal_phyreg_sr(phy_info_t *pi, uint8 core, acphy_rxcal_phyregs_t *porig, bool sr)
{
    struct addr_val {
        uint16 source;
        uint16 * destination;
    } reg_data[] = {
        {ACPHY_RfctrlOverrideTxPus0(pi->pubpi->phy_rev), &porig->RfctrlOverrideTxPus[core]},
        {ACPHY_RfctrlOverrideRxPus0(pi->pubpi->phy_rev), &porig->RfctrlOverrideRxPus[core]},
        {ACPHY_RfctrlOverrideGains0(pi->pubpi->phy_rev), &porig->RfctrlOverrideGains[core]},
        {ACPHY_RfctrlOverrideLpfCT0(pi->pubpi->phy_rev), &porig->RfctrlOverrideLpfCT[core]},
        {ACPHY_RfctrlOverrideLpfSwtch0(pi->pubpi->phy_rev),
        &porig->RfctrlOverrideLpfSwtch[core]},
        {ACPHY_RfctrlOverrideAfeCfg0(pi->pubpi->phy_rev),
        &porig->RfctrlOverrideAfeCfg[core]},
        {ACPHY_RfctrlOverrideLowPwrCfg0(pi->pubpi->phy_rev),
        &porig->RfctrlOverrideLowPwrCfg[core]},
        {ACPHY_RfctrlOverrideAuxTssi0(pi->pubpi->phy_rev),
        &porig->RfctrlOverrideAuxTssi[core]},
        {ACPHY_RfctrlCoreTxPus0(pi->pubpi->phy_rev), &porig->RfctrlCoreTxPus[core]},
        {ACPHY_RfctrlCoreRxPus0(pi->pubpi->phy_rev), &porig->RfctrlCoreRxPus[core]},
        {ACPHY_RfctrlCoreTXGAIN10(pi->pubpi->phy_rev), &porig->RfctrlCoreTXGAIN1[core]},
        {ACPHY_RfctrlCoreTXGAIN20(pi->pubpi->phy_rev), &porig->RfctrlCoreTXGAIN2[core]},
        {ACPHY_RfctrlCoreRXGAIN10(pi->pubpi->phy_rev), &porig->RfctrlCoreRXGAIN1[core]},
        {ACPHY_RfctrlCoreRXGAIN20(pi->pubpi->phy_rev), &porig->RfctrlCoreRXGAIN2[core]},
        {ACPHY_RfctrlCoreLpfGain0(pi->pubpi->phy_rev), &porig->RfctrlCoreLpfGain[core]},
        {ACPHY_RfctrlCoreLpfCT0(pi->pubpi->phy_rev), &porig->RfctrlCoreLpfCT[core]},
        {ACPHY_RfctrlCoreLpfGmult0(pi->pubpi->phy_rev), &porig->RfctrlCoreLpfGmult[core]},
        {ACPHY_RfctrlCoreRCDACBuf0(pi->pubpi->phy_rev), &porig->RfctrlCoreRCDACBuf[core]},
        {ACPHY_RfctrlCoreLpfSwtch0(pi->pubpi->phy_rev), &porig->RfctrlCoreLpfSwtch[core]},
        {ACPHY_RfctrlCoreAfeCfg10(pi->pubpi->phy_rev), &porig->RfctrlCoreAfeCfg1[core]},
        {ACPHY_RfctrlCoreAfeCfg20(pi->pubpi->phy_rev), &porig->RfctrlCoreAfeCfg2[core]},
        {ACPHY_RfctrlCoreLowPwr0(pi->pubpi->phy_rev), &porig->RfctrlCoreLowPwr[core]},
        {ACPHY_RfctrlCoreAuxTssi10(pi->pubpi->phy_rev), &porig->RfctrlCoreAuxTssi1[core]},
        {ACPHY_RfctrlCoreAuxTssi20(pi->pubpi->phy_rev), &porig->RfctrlCoreAuxTssi2[core]},
        {ACPHY_Dac_gain0(pi->pubpi->phy_rev), &porig->Dac_gain[core]},
        {ACPHY_RfctrlIntc0(pi->pubpi->phy_rev), &porig->RfctrlIntc[core]},
        {ACPHY_PapdEnable0(pi->pubpi->phy_rev), &porig->PapdEnable[core]},
        {ACPHY_forceFront0(pi->pubpi->phy_rev), &porig->forceFront[core]},
        { 0, 0}
    };
    struct addr_val * addrp;

    for (addrp = reg_data; addrp->source; addrp++) {
        if (sr) {
            *addrp->destination = phy_utils_read_phyreg(pi, addrp->source +
                (core * PHY_REG_BANK_CORE1_OFFSET));
        } else {
            phy_utils_write_phyreg(pi, addrp->source + (core *
                PHY_REG_BANK_CORE1_OFFSET), *addrp->destination);
        }
    }
}

const uint32 *
BCMRAMFN(get_wbpapd_wfm_phyrev130)(phy_info_t *pi)
{
    const uint32 *wfm;
    phy_ac_papdcal_params_t *params = pi->u.pi_acphy->papdcali->papd_params;

#if defined(WL_WBPAPD) && !defined(WL_WBPAPD_DISABLED)
    params->wbcal_waveform_sz = ARRAYSIZE(acphy_wbpapd_waveform_phyrev130);
    wfm = (const uint32 *)&acphy_wbpapd_waveform_phyrev130[0];
#else
    params->wbcal_waveform_sz = 0;
    wfm = NULL;
#endif /* WL_WBPAPD && !WL_WBPAPD_DISABLED */

    return wfm;
}

void
phy_ac_papdcal_cal_init(phy_info_t *pi)
{
    /* If single phase cal send out CTS to self to ensure assoc/join */
    uint8 phase_id = pi->cal_info->cal_phase_id;
    uint16 cal_exec_time = 29000;
    if (phase_id == PHY_CAL_PHASE_IDLE) {
        if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
            ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
            ACMAJORREV_128(pi->pubpi->phy_rev)) {
            if (pi->sh->up) {
                wlc_phy_cts2self(pi, cal_exec_time);
            }
        } else {
            wlc_phy_cts2self(pi, cal_exec_time);
        }
    }
}

#ifdef PHYCAL_CACHING
void
phy_ac_papdcal_save_cache(phy_ac_papdcal_info_t *papdcali, ch_calcache_t *ctx)
{
    phy_info_t *pi = papdcali->pi;
    acphy_ram_calcache_t *ctx_ac = ctx->u_ram_cache.acphy_ram_calcache;
    uint32 *epsilon_cache;
    uint16 *epstbl_offset_cache;
    uint32 epsilon_table_ids[] =
        {ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1, ACPHY_TBL_ID_EPSILON2,
            ACPHY_TBL_ID_EPSILON3};
    uint32 rfpwrlut_table_ids[] =
        {ACPHY_TBL_ID_RFPWRLUTS0, ACPHY_TBL_ID_RFPWRLUTS1, ACPHY_TBL_ID_RFPWRLUTS2,
            ACPHY_TBL_ID_RFPWRLUTS3};
    uint8 core;
    uint8 disable_table_extension = 0;
    uint16 eps_table_size;

    epsilon_cache = ctx->u.acphy_cache.papd_eps;
    epstbl_offset_cache = ctx->u.acphy_cache.eps_offset_cache;
    /* save the calibration to cache */
    FOREACH_CORE(pi, core) {
        /* save PAPD epsilon offsets */
        eps_table_size = phy_ac_papdcal_eps_table_size(pi, core);
        ctx_ac->epsilon_offset[core] = READ_PHYREGFLDCEE(pi,
            EpsilonTableAdjust, core, epsilonOffset);
        ctx_ac->papd_comp_en[core] = READ_PHYREGFLDCEE(pi, PapdEnable,
            core, papd_compEnb);

        if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
            // Devices with duplicate even and odd entries.
            disable_table_extension = READ_PHYREGFLDCE(pi, papdEpsilonTable, core,
                epsilon_tbl_extend_dis);
        }

        if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev) &&
            !disable_table_extension) {
            // Even and odd entries are in duplicate. Only read and cache one of them.
            MOD_PHYREGCE(pi, papdEpsilonTable, core, epsilon_tbl_extend_dis, 1);
            wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core],
                eps_table_size>>1, 0, 32, epsilon_cache, core);
            epsilon_cache += (eps_table_size>>1);
            MOD_PHYREGCE(pi, papdEpsilonTable, core, epsilon_tbl_extend_dis, 0);
        } else {
            wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core],
                eps_table_size, 0, 32, epsilon_cache, core);
            epsilon_cache += eps_table_size;
        }
        if (!ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
            wlc_phy_table_read_acphy(pi, rfpwrlut_table_ids[core],
                ACPHY_PAPD_RFPWRLUT_TBL_SIZE, 0, 16, epstbl_offset_cache);
            epstbl_offset_cache += ACPHY_PAPD_RFPWRLUT_TBL_SIZE;
        }
    }
}
#endif /* PHYCAL_CACHING */

void
phy_ac_papdcal_multiphase(phy_info_t *pi, int8 cal_core)
{
    wlc_phy_cals_mac_susp_en_other_cr(pi, TRUE);

#ifdef WFD_PHY_LL
    if (pi->papdcali->data->wfd_ll_enable) {
        /* skip the PAPD calibration */
        wlc_phy_cts2self(pi, 0);
        pi->cal_info->cal_phase_id++;
        return;
    }
#endif
    if (PHY_PAPDEN(pi)) {
        wlc_phy_cts2self(pi, pi->u.pi_acphy->papdcali->papd_cal_time);
        wlc_phy_do_papd_cal_acphy(pi, cal_core);
    } else {
        /* To make phyreg_enter & mac_suspend in sync for PAPD_EN=0 */
        wlc_phy_cts2self(pi, 0);
    }
    wlc_phy_cals_mac_susp_en_other_cr(pi, FALSE);

    if (cal_core == -1 || cal_core == PHYCORENUM((pi)->pubpi->phy_corenum)-1) {
        /* move on */
        pi->cal_info->cal_phase_id++;
    }
}

#if defined(WFD_PHY_LL)
static int
phy_ac_papdcal_set_wfd_ll_enable(phy_type_papdcal_ctx_t *ctx, uint8 int_val)
{
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)ctx;
    phy_papdcal_data_t *data = papdcali->cmn_info->data;

    /* Force the channel to be active */
    data->wfd_ll_chan_active_force = (int_val == 2) ? TRUE : FALSE;
    data->wfd_ll_enable_pending = int_val;
    if (!PHY_PERICAL_MPHASE_PENDING(papdcali->pi)) {
        /* Apply it since there is no CAL in progress */
        data->wfd_ll_enable = int_val;
        if (!int_val) {
            /* Force a watchdog CAL when disabling WFD optimization
             * As PADP CAL has not been executed since a long time
             * a PADP CAL is executed at the next watchdog timeout
             */
             papdcali->pi->cal_info->last_cal_time = 0;
        }
    }
    return BCME_OK;
}

static int
phy_ac_papdcal_get_wfd_ll_enable(phy_type_papdcal_ctx_t *ctx, int32 *ret_int_ptr)
{
    phy_ac_papdcal_info_t *papdcali = (phy_ac_papdcal_info_t *)ctx;
    *ret_int_ptr = papdcali->cmn_info->data->wfd_ll_enable;
    return BCME_OK;
}
#endif /* WFD_PHY_LL */

uint16
phy_ac_papdcal_eps_table_size(phy_info_t *pi, uint8 core)
{
    /* Devices after 43684A0 have two PAPD tables which can be read interleaved */
    /* If papdEpsilonTable.epsilon_tbl_extend_dis is not set, */
    /* twice the elements are accessible. Not available on 6878 */
    /* The returned value accounts for both even and odd entries */
    uint8 disable_table_extension = 0;
    uint16 eps_table_size = ACPHY_PAPD_EPS_TBL_SIZE;

    if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        disable_table_extension = READ_PHYREGFLDCE(pi, papdEpsilonTable, core,
            epsilon_tbl_extend_dis);
        eps_table_size >>= disable_table_extension;
    } else {
        eps_table_size >>= 1;
    }
    if (ACMAJORREV_130(pi->pubpi->phy_rev) && WBPAPD_ENAB(pi)) {
        // High resolution mode. Eps table step size 0.25 dB.
        eps_table_size <<= 1;
    }
    return eps_table_size;
}

void
wlc_phy_txpwr_papd_cal_acphy(phy_info_t *pi)
{
    uint32 delta_idx;
    uint8 txpwr_idx_cur[PHY_CORE_MAX] = { 0 };
    uint8 core;
    uint8 searchmode = PHY_PERICAL_AUTO;
    int8 orgi_cal_core;
    uint8 orgi_phase_id;

    phy_info_acphy_t *aci = (phy_info_acphy_t *)pi->u.pi_acphy;
    phy_ac_calmgr_info_t *ci = pi->u.pi_acphy->calmgri;

    /* skip cal if phy is muted */
    if (PHY_MUTED(pi) || (pi->txpwrctrl != PHY_TPC_HW_ON))
        return;

    /* (Temporarily disabled) Perform a PAPD cal if the Tx gain changes by 1 dB */
    delta_idx = 50;

    FOREACH_CORE(pi, core) {
        txpwr_idx_cur[core] = READ_PHYREGFLDCE(pi, TxPwrCtrlStatus_path, core, baseIndex);
        /* Do single core PAPD cal if core baseidx change >= delta_idx */
        if ((uint32)ABS(txpwr_idx_cur[core] -
            aci->papdcali->acphy_papd_tx_gain_at_last_cal[core]) >= delta_idx) {
                orgi_cal_core = pi->cal_info->cal_core;
                pi->cal_info->cal_core = core;
                orgi_phase_id = pi->cal_info->cal_phase_id;
                pi->cal_info->cal_phase_id = PHY_CAL_PHASE_PAPDCAL;

                wlc_phy_cals_acphy(ci, PHY_PERICAL_AUTO, searchmode);

                pi->cal_info->cal_core = orgi_cal_core;
                pi->cal_info->cal_phase_id = orgi_phase_id;
        }
    }
}

static int
phy_ac_papd_mac_load(phy_info_t *pi, const uint32* buf, uint32 len)
{
    phy_ac_papdcal_params_t *params = pi->u.pi_acphy->papdcali->papd_params;
    uint32 startidx = params->wbcal_macbuf_offset;
    uint16 k0, k4 = 0;
    uint8 k1, k2, k3;
    uint32 sample32;
    int16 samplei, sampleq;
    int32 tmpi, tmpq;
    int16 fOuti, fOutq;
    uint32 fOut;
    const uint16 scale9dB = 27145; // 2*sqrt(2) approximated as 2 + 27145/2^15.
    const uint16 scale1p898 = 3887; // 1.898 * 2^11.
    uint32 wPtr0, wPtr1, wPtr2, wPtr3, wPtr4;
    int8 wp0 = 0, wp1 = 0, wp2 = 0;
    const int16 hbf0[] = {11, -64, 309};
    const int8 hbf1[] = {1, -12, 75};
    const int8 hbf2[] = {-5, 37};
    int16 buf0i[6], buf0q[6], buf1i[6], buf1q[6], buf2i[4], buf2q[4];
    int16 hbf0Outi[2], hbf0Outq[2], hbf1Outi[2], hbf1Outq[2], hbf2Outi[2], hbf2Outq[2];

    wPtr0 = startidx; // len * 4 in bytes.
    wPtr1 = startidx + len*8*4;
    wPtr2 = startidx + len*8*3*4 - 4;
    wPtr3 = startidx + len*8*4*4 - 4;
    wPtr4 = startidx + len*8*4*4;

    for (k0 = 0; k0 < 6; k0++) {
        buf0i[k0] = 0;
        buf0q[k0] = 0;
    }
    for (k1 = 0; k1 < 6; k1++) {
        buf1i[k1] = 0;
        buf1q[k1] = 0;
    }
    for (k2 = 0; k2 < 4; k2++) {
        buf2i[k2] = 0;
        buf2q[k2] = 0;
    }

    for (k0 = 0; k0 < len; k0++) {
        // 1st stage of 2x upsampling and halfband filter.
        sample32 = buf[k0];
        buf0i[wp0] = (int16)(sample32>>16);
        buf0q[wp0] = (int16)(sample32 & 0xffff);

        tmpi = hbf0[0]*(buf0i[wp0]+buf0i[((wp0-5)%6+6)%6]) +
            hbf0[1]*(buf0i[((wp0-1)%6+6)%6]+buf0i[((wp0-4)%6+6)%6]) +
            hbf0[2]*(buf0i[((wp0-2)%6+6)%6]+buf0i[((wp0-3)%6+6)%6]);
        fOuti = tmpi >> 9;
        tmpq = hbf0[0]*(buf0q[wp0]+buf0q[((wp0-5)%6+6)%6]) +
            hbf0[1]*(buf0q[((wp0-1)%6+6)%6]+buf0q[((wp0-4)%6+6)%6]) +
            hbf0[2]*(buf0q[((wp0-2)%6+6)%6]+buf0q[((wp0-3)%6+6)%6]);
        fOutq = tmpq >> 9;
        hbf0Outi[0] = fOuti;
        hbf0Outq[0] = fOutq;
        hbf0Outi[1] = buf0i[((wp0-2)%6+6)%6];
        hbf0Outq[1] = buf0q[((wp0-2)%6+6)%6];
        wp0 = ((wp0+1)%6+6)%6;

        // 2nd stage of 2x upsampling and halfband filter.
        for (k1 = 0; k1 < 2; k1++) {
            buf1i[wp1] = hbf0Outi[k1];
            buf1q[wp1] = hbf0Outq[k1];

            tmpi = hbf1[0]*(buf1i[wp1]+buf1i[((wp1-5)%6+6)%6]) +
                hbf1[1]*(buf1i[((wp1-1)%6+6)%6]+buf1i[((wp1-4)%6+6)%6]) +
                hbf1[2]*(buf1i[((wp1-2)%6+6)%6]+buf1i[((wp1-3)%6+6)%6]);
            fOuti = tmpi >> 7;
            tmpq = hbf1[0]*(buf1q[wp1]+buf1q[((wp1-5)%6+6)%6]) +
                hbf1[1]*(buf1q[((wp1-1)%6+6)%6]+buf1q[((wp1-4)%6+6)%6]) +
                hbf1[2]*(buf1q[((wp1-2)%6+6)%6]+buf1q[((wp1-3)%6+6)%6]);
            fOutq = tmpq >> 7;
            hbf1Outi[0] = fOuti;
            hbf1Outq[0] = fOutq;
            hbf1Outi[1] = buf1i[((wp1-2)%6+6)%6];
            hbf1Outq[1] = buf1q[((wp1-2)%6+6)%6];
            wp1 = ((wp1+1)%6+6)%6;

            // 3rd stage of 2x upsampling and halfband filter.
            for (k2 = 0; k2 < 2; k2++) {
                buf2i[wp2] = hbf1Outi[k2];
                buf2q[wp2] = hbf1Outq[k2];

                tmpi = hbf2[0]*(buf2i[wp2]+buf2i[((wp2-3)%4+4)%4]) +
                    hbf2[1]*(buf2i[((wp2-1)%4+4)%4]+buf2i[((wp2-2)%4+4)%4]);
                fOuti = tmpi >> 6;
                tmpq = hbf2[0]*(buf2q[wp2]+buf2q[((wp2-3)%4+4)%4]) +
                    hbf2[1]*(buf2q[((wp2-1)%4+4)%4]+buf2q[((wp2-2)%4+4)%4]);
                fOutq = tmpq >> 6;
                hbf2Outi[0] = fOuti;
                hbf2Outq[0] = fOutq;
                hbf2Outi[1] = buf2i[((wp2-1)%4+4)%4];
                hbf2Outq[1] = buf2q[((wp2-1)%4+4)%4];
                wp2 = ((wp2+1)%4+4)%4;

                // Write all 4 segments alltogether.
                // The 1st segment is the 8x upsampled waveform.
                // The 2nd segment is the 1st segment scaled by 9 dB.
                // The 3rd segment is the 2nd segment in reverse order.
                // The 4th segment is the 1st segment in reverse order.
                // The 5th segment is the 1st segment index 78~2577, times 1.898.
                for (k3 = 0; k3 < 2; k3++) {
                    // Segment 1 & 4.
                    fOut = (((uint16)hbf2Outi[k3])<<16) | (uint16)hbf2Outq[k3];
                    W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_PTR(pi),
                        wPtr0);
                    W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_DATA(pi),
                        fOut);
                    W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_PTR(pi),
                        wPtr3);
                    W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_DATA(pi),
                        fOut);
                    // Segment 2 & 3.
                    samplei = hbf2Outi[k3];
                    sampleq = hbf2Outq[k3];
                    tmpi = samplei * scale9dB + (samplei<<16);
                    tmpq = sampleq * scale9dB + (sampleq<<16);
                    fOuti = tmpi >> 15;
                    fOutq = tmpq >> 15;
                    fOut = (((uint16)fOuti)<<16) | (uint16)fOutq;
                    W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_PTR(pi),
                        wPtr1);
                    W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_DATA(pi),
                        fOut);
                    W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_PTR(pi),
                        wPtr2);
                    W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_DATA(pi),
                        fOut);
                    // Segment 5.
                    if (k4 >= 78 && k4 < 2578) {
                        tmpi = samplei * scale1p898 + 1024;
                        tmpq = sampleq * scale1p898 + 1024;
                        fOuti = tmpi >> 11;
                        fOutq = tmpq >> 11;
                        fOut = (((uint16)fOuti)<<16) | (uint16)fOutq;
                        W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_PTR
                            (pi), wPtr4);
                        W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_DATA
                            (pi), fOut);
                        wPtr4 += 4;
                    }
                    k4++;

                    wPtr0 += 4;
                    wPtr1 += 4;
                    wPtr2 -= 4;
                    wPtr3 -= 4;

                }
            }
        }
    }

    return BCME_OK;
}

/* Finds maximum valid AMAM eps coefficient (avoiding potential kinks at the table end),
 * then fills remainder of the eps table with that maximum valid AMAM eps coefficient.
 * Can also just report the maximum valid AMAM eps coefficient index, without modifying the table.
 */
static uint16
phy_ac_papd_find_maxamam_endfill_epstbl(phy_info_t *pi, uint8 core,
        bool only_report_maxamam_idx)
{
    uint32 *buf, last_eps, next_last_eps;
    uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
        ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
    uint16 eps_table_size = phy_ac_papdcal_eps_table_size(pi, core);
    uint16 endfill_start;
    uint8 eps_stop_idx, step = 1;
    int32 eps_r, eps_i, next_eps_r, next_eps_i;

    PHY_CAL(("End-filling papd cal table on core: %d\n", core));

    ASSERT(core < ARRAYSIZE(epsilon_table_ids));

    /* Find the last valid max AMAM */
    eps_stop_idx = READ_PHYREGFLDCEE(pi, PapdCompStopIdxLutSel1, core,
            papd_comp_stop_index_lut0);

    if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev)) {
        step = 2;
    }
    ASSERT((eps_stop_idx * step) < eps_table_size);

    /* Max AMAM happens to be one of last 7 eps elements */
    eps_stop_idx = eps_stop_idx > 7 ? eps_stop_idx - 7 : 0;
    endfill_start = eps_stop_idx * step;

    /* Allocate storage for the table */
    buf = phy_malloc_fatal(pi, sizeof(*buf) * eps_table_size);

    /* Read original table */
    wlc_phy_table_read_acphy_dac_war(pi, epsilon_table_ids[core], eps_table_size,
            0, 32, buf, core);

    last_eps = buf[eps_stop_idx * step];
    next_last_eps = buf[(eps_stop_idx + 1) * step];

    phy_papdcal_decode_epsilon(last_eps, &eps_r, &eps_i);
    phy_papdcal_decode_epsilon(next_last_eps, &next_eps_r, &next_eps_i);

    /* Find the last valid entry with maximum AMAM value */
    while ((eps_r < next_eps_r) && (((++eps_stop_idx) * step) < (eps_table_size - step))) {
        last_eps = next_last_eps;
        eps_r = next_eps_r;
        next_last_eps = buf[(eps_stop_idx + 1) * step];
        phy_papdcal_decode_epsilon(next_last_eps, &next_eps_r, &next_eps_i);
    }

    if (!only_report_maxamam_idx) {
        eps_stop_idx *= step;

        /* Fill remainder of the table with the last valid entry with maximum AMAM */
        while (eps_stop_idx < eps_table_size) {
            buf[eps_stop_idx++] = last_eps;
        }

        /* Write updated table */
        wlc_phy_table_write_acphy_dac_war(pi, epsilon_table_ids[core],
            eps_table_size - endfill_start,
            endfill_start, 32, buf + endfill_start, core);
    }

    /* Free allocated buffer */
    phy_mfree(pi, buf, sizeof(*buf) * eps_table_size);

    return eps_stop_idx;
}
