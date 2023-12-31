/*
 * ACPHY TEMPerature sense module implementation
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
 * $Id: phy_ac_temp.c 806043 2021-12-11 01:18:33Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <bcm_math.h>
#include <phy_btcx.h>
#include "phy_type_temp.h"
#include "phy_temp_st.h"
#include <phy_ac.h>
#include <phy_ac_temp.h>
#include <phy_ac_calmgr.h>
#include <phy_ac_info.h>
#include <phy_utils_reg.h>
#include <phy_utils_pmu.h>
#include <phy_stf.h>
#include "hndpmu.h"

#include "wlc_phy_radio.h"
#include "wlc_phyreg_ac.h"
#include "wlc_radioreg_20693.h"
#include "wlc_radioreg_20698.h"
#include "wlc_radioreg_20704.h"
#include "wlc_radioreg_20707.h"
#include "wlc_radioreg_20708.h"
#include "wlc_radioreg_20709.h"
#include "wlc_radioreg_20710.h"

#ifdef ATE_BUILD
#include <wl_ate.h>
#endif

#include <phy_rstr.h>
#include <phy_utils_var.h>
#include <phy_temp_api.h>

#define PALDO_VOLTAGE_CODE_FOR_3P4V      0x2u
#define PALDO_VOLTAGE_CODE_FOR_3P3V      0x0u
#define PALDO_VOLTAGE_CODE_FOR_3P2V      0x7u
#define PALDO_VOLTAGE_CODE_FOR_3P1V      0x6u
#define PALDO_VOLTAGE_CODE_FOR_3P0V      0x5u
#define PALDO_VOLTAGE_CODE_FOR_2P9V      0x4u
#define PALDO_VOLTAGE_3P4                34u
#define PALDO_VOLTAGE_3P3                33u
#define PALDO_VOLTAGE_3P2                32u
#define PALDO_VOLTAGE_3P1                31u
#define PALDO_VOLTAGE_3P0                30u
#define PALDO_VOLTAGE_2P9                29u
#define PALDO_VOLTATE_MAPPING_ARRAY_SIZE 6
#define PALDO_VOLTAGE_HEADROOM           2
#define PMU_REG6_PALDO_CTRL              0x000000E0
#define PMU_REG6_PALDO_CTRL_SHFT         5
#define AUXPGA_AV_VBATSENSE              0x6
#define AUXPGA_VMID_VBATSENSE            0x5C
#define AUXPGA_AV_TEMPSENSE              0x3
#define AUXPGA_VMID_TEMPSENSE            0x91

/* module private states */
struct phy_ac_temp_info {
    phy_info_t *pi;
    phy_ac_info_t *aci;
    phy_temp_info_t *ti;
    acphy_tempsense_radioregs_t *ac_tempsense_radioregs_orig;
    acphy_tempsense_phyregs_t   *ac_tempsense_phyregs_orig;
    int16 current_temperature;
    uint8 last_regulated;
    uint8 vbat_codeidx;
    uint8 current_vbat;
/* add other variable size variables here at the end */
};

/* local functions */
#ifndef WL_TVPM
static uint16 phy_ac_temp_throttle(phy_type_temp_ctx_t *ctx);
#endif /* ! WL_TVPM */
static void phy_ac_temp_nvram_attach(phy_ac_temp_info_t *tempi);
static void phy_ac_temp_upd_gain(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj);
static void phy_ac_temp_upd_gain_cal(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj);
static int phy_ac_temp_get_(phy_type_temp_ctx_t *ctx);
static uint8 phy_ac_temp_get_first_actv_core(uint8 coremask);
static int16 phy_ac_temp_do_tempsense(phy_type_temp_ctx_t *ctx);

#if defined(TXPWRBACKOFF) || defined(WL_TVPM)
static uint8 phy_ac_vbat_sense(phy_type_temp_ctx_t *ctx);
static uint8 phy_ac_vbat_get(phy_type_temp_ctx_t *ctx);
#endif /* TXPWRBACKOFF || WL_TVPM */

/* Register/unregister ACPHY specific implementation to common layer. */
phy_ac_temp_info_t *
BCMATTACHFN(phy_ac_temp_register_impl)(phy_info_t *pi, phy_ac_info_t *aci, phy_temp_info_t *ti)
{
    phy_ac_temp_info_t *temp_info;
    phy_type_temp_fns_t fns;
    acphy_tempsense_radioregs_t *ac_tempsense_radioregs_orig = NULL;
    acphy_tempsense_phyregs_t   *ac_tempsense_phyregs_orig = NULL;
    phy_txcore_temp_t *temp = NULL;

    PHY_TRACE(("%s\n", __FUNCTION__));

    /* allocate all storage in once */
    if ((temp_info = phy_malloc(pi, sizeof(phy_ac_temp_info_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((ac_tempsense_radioregs_orig =
        phy_malloc(pi, sizeof(acphy_tempsense_radioregs_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc ac_tempsense_radioregs_orig failed\n", __FUNCTION__));
        goto fail;
    }

    if ((ac_tempsense_phyregs_orig =
        phy_malloc(pi, sizeof(acphy_tempsense_phyregs_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc ac_tempsense_phyregs_orig failed\n", __FUNCTION__));
        goto fail;
    }

    temp_info->pi = pi;
    temp_info->aci = aci;
    temp_info->ti = ti;
    temp_info->ac_tempsense_radioregs_orig = ac_tempsense_radioregs_orig;
    temp_info->ac_tempsense_phyregs_orig = ac_tempsense_phyregs_orig;

    /* Read srom params from nvram */
    phy_ac_temp_nvram_attach(temp_info);

    /* Register PHY type specific implementation */
    bzero(&fns, sizeof(fns));
#ifndef WL_TVPM
    fns.throt = phy_ac_temp_throttle;
#endif /* !WL_TVPM */
    fns.get = phy_ac_temp_get_;
    fns.upd_gain = phy_ac_temp_upd_gain;
    fns.upd_gain_cal = phy_ac_temp_upd_gain_cal;
    fns.do_tempsense = phy_ac_temp_do_tempsense;
    fns.ctx = temp_info;

#if defined(TXPWRBACKOFF) || defined(WL_TVPM)
    fns.vbat_sense = phy_ac_vbat_sense; /* measure Vbat */
    fns.vbat_get   = phy_ac_vbat_get;   /* return last Vbat value */
#endif /* TXPWRBACKOFF || WL_TVPM */

    /* Initialize any common layer variable */
    temp = phy_temp_get_st(ti);
    ASSERT(temp);
    if ((temp->disable_temp == 0) || (temp->disable_temp == 0xff)) {
        temp->disable_temp = ACPHY_CHAIN_TX_DISABLE_TEMP;
    }
    temp->txcore_temp_cnt.phy_temp_reduce_cnt = 0;
    temp->txcore_temp_cnt.phy_temp_incr_cnt = 0;
    temp->txcore_temp_cnt.phy_temp_1_tx_reduce = 0;
    temp->txcore_temp_cnt.phy_temp_2_tx_reduce = 0;
    temp->txcore_temp_cnt.phy_temp_3_tx_reduce = 0;
#if defined(BCM94360X51) && defined(BCM94360X52C)
    if ((CHIPID(pi->sh->chip) == BCM4360_CHIP_ID) &&
        ((pi->sh->boardtype == BCM94360X51) ||
        (pi->sh->boardtype == BCM94360X51P3) ||
        (pi->sh->boardtype == BCM94360X52C))) {
        temp->disable_temp = ACPHY_CHAIN_TX_DISABLE_TEMP_4360;
    }
#endif /* BCM94360X51 && BCM94360X52C */

    phy_temp_register_impl(ti, &fns);

    return temp_info;
fail:
    if (ac_tempsense_phyregs_orig != NULL)
        phy_mfree(pi, ac_tempsense_phyregs_orig, sizeof(acphy_tempsense_phyregs_t));
    if (ac_tempsense_radioregs_orig != NULL)
        phy_mfree(pi, ac_tempsense_radioregs_orig, sizeof(acphy_tempsense_radioregs_t));

    if (temp_info != NULL)
        phy_mfree(pi, temp_info, sizeof(phy_ac_temp_info_t));
    return NULL;
}

void
BCMATTACHFN(phy_ac_temp_unregister_impl)(phy_ac_temp_info_t *temp_info)
{
    phy_info_t *pi;
    phy_temp_info_t *ti;

    ASSERT(temp_info);
    pi = temp_info->pi;
    ti = temp_info->ti;

    PHY_TRACE(("%s\n", __FUNCTION__));

    phy_temp_unregister_impl(ti);

    phy_mfree(pi, temp_info->ac_tempsense_phyregs_orig, sizeof(acphy_tempsense_phyregs_t));

    phy_mfree(pi, temp_info->ac_tempsense_radioregs_orig, sizeof(acphy_tempsense_radioregs_t));

    phy_mfree(pi, temp_info, sizeof(phy_ac_temp_info_t));
}

/* XXX Tx-Core Shut-Down to prevent hitting critical junction temperature
 * Assumptions: Code written assuming max txchain = 7 (3 Tx Chain)
 * Output is stored in pi->tempi->txcore_temp.bitmap.
 * BitMap returns the active RxChain and TxChain.
 */
#ifndef WL_TVPM
static uint16
phy_ac_temp_throttle(phy_type_temp_ctx_t *ctx)
{
    phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
    phy_temp_info_t *ti = info->ti;
    phy_info_t *pi = info->pi;
    phy_txcore_temp_t *temp;
    /* XXX Shut-Down All Tx-Core Except One When Hot
     * When there is only 1 tx-core on, there will be no change
     * When active coremash is 15 (4366), change to 13
     */
    uint8 txcore_shutdown_lut[] = {1, 1, 2, 1, 4, 1, 2, 1,
            8, 8, 2, 9, 4, 9, 4, 13};
    uint8 txcore_shutdown_lut_4366[] = {1, 1, 2, 1, 4, 1, 2, 1,
            8, 8, 2, 9, 4, 9, 4, 13};
    uint8 txcore_shutdown_lut_6710[] = {1, 1, 1, 1, 1, 1, 1, 5};
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 new_phytxchain = 0, old_phytxchain = 0;
    int16 currtemp, delta_temp = 0;
    uint16 duty_cycle_active_chains;
    uint16 phytxchain_reduce = 0;

    PHY_TRACE(("%s\n", __FUNCTION__));
    /* No need to do tempsense/Throttle/Cal when not associated
      * and returning hw_txchain to avoid throttling in upper layer
      */
    if ((ACMAJORREV_4(pi->pubpi->phy_rev) ||
        (ACMAJORREV_5(pi->pubpi->phy_rev) && ACMINORREV_2(pi))) && PUB_NOT_ASSOC(pi))
        return stf_shdata->hw_phytxchain;

    temp = phy_temp_get_st(ti);
    ASSERT(temp != NULL);

    ASSERT(stf_shdata->phytxchain);

    currtemp = wlc_phy_tempsense_acphy(pi);
    if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
        /* Checking temperature for every 10sec and
             Triggering calibration if there is a temperature
             change greater than phycal_tempdelta.
             This is needed to address Tx throughput degradation
             seen on 4349 MIMO mode.
          */
        delta_temp = ((currtemp > pi->cal_info->last_cal_temp) ?
            (currtemp - pi->cal_info->last_cal_temp) :
            (pi->cal_info->last_cal_temp - currtemp));
        /* Before triggering the cal make sure no cals are pending */
        if ((temp->phycal_tempdelta) && (delta_temp > temp->phycal_tempdelta) &&
            (!PHY_PERICAL_MPHASE_PENDING(pi))) {
            pi->cal_info->last_cal_temp = currtemp;
            wlc_phy_cals_acphy(pi->u.pi_acphy->calmgri, PHY_PERICAL_UNDEF,
                    PHY_CAL_SEARCHMODE_RESTART);
        }
    }
#if defined(BCMDBG) || defined(TEMPSENSE_OVERRIDE) || defined(WLTEST)
    if (pi->tempsense_override)
        currtemp = pi->tempsense_override;
#endif /* BCMDBG || TEMPSENSE_OVERRIDE || WLTEST */
    old_phytxchain = stf_shdata->phytxchain;
    if (currtemp >= temp->disable_temp) {
        if (!ACMAJORREV_33(pi->pubpi->phy_rev))
            temp->heatedup = TRUE;
        if (phy_get_phymode(pi) == PHYMODE_MIMO) {
            temp->duty_cycle = 100;
            temp->duty_cycle_throttle_state = 0;
            temp->duty_cycle_throttle_depth = 10;
            if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev) ||
                ACMAJORREV_47(pi->pubpi->phy_rev)) {
                new_phytxchain = txcore_shutdown_lut_4366[stf_shdata->phytxchain];
            } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
                new_phytxchain = txcore_shutdown_lut_6710[stf_shdata->phytxchain];
            } else {
                new_phytxchain = txcore_shutdown_lut[stf_shdata->phytxchain];
            }
            temp->bitmap = ((stf_shdata->phyrxchain << 4) | new_phytxchain);
            if (new_phytxchain != stf_shdata->phytxchain)
                temp->txcore_temp_cnt.phy_temp_reduce_cnt ++;
        } else if(DCT_ENAB(pi)){ /* RSDB */
            if (temp->duty_cycle_throttle_state < temp->duty_cycle_throttle_depth) {
                if (!temp->duty_cycle_throttle_state) {
                    /* First time reduce duty cycle by 20per */
                    temp->duty_cycle -= 20;
                    temp->duty_cycle_throttle_state += 4;
                } else {
                    temp->duty_cycle -= 10;
                    temp->duty_cycle_throttle_state += 2;
                }
            } else if (temp->duty_cycle_throttle_state ==
                temp->duty_cycle_throttle_depth) {
                /* Temperature is not down after duty cycle throttling */
            }
        }
        if (new_phytxchain != old_phytxchain) {
            PHY_ERROR(("Attention! Throttling is activated."
            "TxChain changed from %d to %d \n",
            old_phytxchain, new_phytxchain));
        }
    } else {
        if (currtemp <= temp->enable_temp) {
            if ((phy_get_phymode(pi) == PHYMODE_RSDB) && DCT_ENAB(pi)) {
                if (temp->duty_cycle_throttle_state) {
                    temp->duty_cycle += 5;
                    temp->duty_cycle_throttle_state--;
                    if (!temp->duty_cycle_throttle_state) {
                        new_phytxchain = stf_shdata->hw_phytxchain;
                        if (!ACMAJORREV_33(pi->pubpi->phy_rev))
                            temp->heatedup = FALSE;
                        temp->bitmap = ((stf_shdata->phyrxchain << 4) |
                                new_phytxchain);
                        if (new_phytxchain != stf_shdata->phytxchain) {
                            phytxchain_reduce =
                                temp->txcore_temp_cnt.
                                phy_temp_reduce_cnt
                                 - temp->txcore_temp_cnt.
                                phy_temp_incr_cnt;
                            temp->txcore_temp_cnt.phy_temp_incr_cnt =
                                temp->txcore_temp_cnt.
                                phy_temp_reduce_cnt;
                            if (phytxchain_reduce == 1)
                                temp->txcore_temp_cnt.
                                    phy_temp_1_tx_reduce ++;
                            else if (phytxchain_reduce == 2)
                                temp->txcore_temp_cnt.
                                    phy_temp_2_tx_reduce ++;
                            else if (phytxchain_reduce == 3)
                                temp->txcore_temp_cnt.
                                    phy_temp_3_tx_reduce ++;
                        }
                    }
                }
            } else {
                new_phytxchain = stf_shdata->hw_phytxchain;
                if (!ACMAJORREV_33(pi->pubpi->phy_rev))
                    temp->heatedup = FALSE;
                temp->bitmap = ((stf_shdata->phyrxchain << 4) | new_phytxchain);
                temp->duty_cycle = 100;
                temp->duty_cycle_throttle_depth = 10;
                temp->duty_cycle_throttle_state = 0;
                if (new_phytxchain != stf_shdata->phytxchain) {
                        phytxchain_reduce =
                            temp->txcore_temp_cnt.phy_temp_reduce_cnt
                            - temp->txcore_temp_cnt.phy_temp_incr_cnt;
                        temp->txcore_temp_cnt.phy_temp_incr_cnt =
                            temp->txcore_temp_cnt.phy_temp_reduce_cnt;
                        if (phytxchain_reduce == 1)
                            temp->txcore_temp_cnt.
                                phy_temp_1_tx_reduce ++;
                        else if (phytxchain_reduce == 2)
                            temp->txcore_temp_cnt.
                                phy_temp_2_tx_reduce ++;
                        else if (phytxchain_reduce == 3)
                            temp->txcore_temp_cnt.
                                phy_temp_3_tx_reduce ++;
                    }
            }
            if (phytxchain_reduce != 0) {
                PHY_ERROR(("Throttling is recovering!"
                "TxChain changed from %d to %d \n",
                old_phytxchain, new_phytxchain));
            }
        }
    }
    duty_cycle_active_chains = temp->duty_cycle << 8 | temp->bitmap;
    return duty_cycle_active_chains;
}
#endif /* ! WL_TVPM */

/* read the current temperature */
int
phy_ac_temp_get(phy_ac_temp_info_t *tempi)
{
    return tempi->current_temperature;
}

static int
phy_ac_temp_get_(phy_type_temp_ctx_t *ctx)
{
    phy_ac_temp_info_t *tempi = (phy_ac_temp_info_t *)ctx;
    return phy_ac_temp_get(tempi);
}

static void
BCMATTACHFN(phy_ac_temp_nvram_attach)(phy_ac_temp_info_t *tempi)
{
    phy_info_t *pi = tempi->pi;
    /* read in temperature at calibration time */
    tempi->current_temperature = pi->srom_rawtempsense;
    tempi->current_vbat = PHY_VBAT_MAX;
    pi->sromi->sw_txchain_mask =
        (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_sw_txchain_mask, 0);
    pi->sromi->sw_rxchain_mask =
        (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_sw_rxchain_mask, 0);
    pi->u.pi_acphy->sromi->rpcal2g = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal2g, 0);
    pi->u.pi_acphy->sromi->rpcal5gb0 =
    (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb0, 0);
    pi->u.pi_acphy->sromi->rpcal5gb1 =
    (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb1, 0);
    pi->u.pi_acphy->sromi->rpcal5gb2 =
    (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb2, 0);
    pi->u.pi_acphy->sromi->rpcal5gb3 =
    (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb3, 0);
    pi->sromi->rpcal2gcore3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal2gcore3, 0);
    pi->sromi->rpcal5gb0core3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb0core3, 0);
    pi->sromi->rpcal5gb1core3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb1core3, 0);
    pi->sromi->rpcal5gb2core3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb2core3, 0);
    pi->sromi->rpcal5gb3core3 = (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal5gb3core3, 0);
    pi->u.pi_acphy->sromi->rpcal_phase2g =
        (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal_phase2g, 0);
    pi->u.pi_acphy->sromi->rpcal_phase5gb0 =
        (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal_phase5gb0, 0);
    pi->u.pi_acphy->sromi->rpcal_phase5gb1 =
        (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal_phase5gb1, 0);
    pi->u.pi_acphy->sromi->rpcal_phase5gb2 =
        (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal_phase5gb2, 0);
    pi->u.pi_acphy->sromi->rpcal_phase5gb3 =
        (uint16)PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_rpcal_phase5gb3, 0);
}
/* ********************************************* */
/*                Internal Definitions                    */
/* ********************************************* */
static void wlc_phy_tempsense_radio_setup_acphy(phy_info_t *pi, uint16 Av, uint16 Vmid);
static void wlc_phy_tempsense_radio_cleanup_acphy(phy_info_t *pi);
static void wlc_phy_tempsense_phy_setup_acphy(phy_info_t *pi, uint8 core);
static void wlc_phy_tempsense_phy_cleanup_acphy(phy_info_t *pi, uint8 core);
static int32 wlc_phy_tempsense_paldosense_phy_setup_acphy_tiny(phy_info_t *pi,
    uint8 tempsense_paldosense, uint8 core);
static int32 wlc_phy_tempsense_paldosense_radio_setup_acphy_tiny(phy_info_t *pi, uint16 Av,
    uint16 Vmid, uint8 tempsense_paldosense, uint8 core);
static int32 wlc_phy_tempsense_poll_adc_war_tiny(phy_info_t *pi,
    bool init_adc_inside, int32 *measured_values, uint8 core);
static int32 wlc_phy_tempsense_poll_samps_tiny(phy_info_t *pi,
    uint16 samples, bool init_adc_inside, uint8 core);
static int32 wlc_phy_tempsense_gpiosel_tiny(phy_info_t *pi,
    uint16 sel, uint8 word_swap);
static int32 wlc_phy_tempsense_radio_swap_tiny(phy_info_t *pi,
    acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static int32 wlc_phy_tempsense_paldosense_phy_cleanup_acphy_tiny(phy_info_t *pi, uint8 core);
static int32 wlc_phy_tempsense_paldosense_radio_cleanup_acphy_tiny(phy_info_t *pi,
    uint8 tempsense_paldosense, uint8 core);
static int16 wlc_phy_tempsense_paldosense_acphy_tiny
    (phy_info_t *pi, uint8 tempsense_paldosense);
static int16 wlc_phy_tempsense_vbatsense_acphy_20698
    (phy_info_t *pi, uint8 tempsense_vbatsense);
static int16 wlc_phy_tempsense_vbatsense_acphy_20704
    (phy_info_t *pi, uint8 tempsense_vbatsense);
static int16 wlc_phy_tempsense_vbatsense_acphy_20707
    (phy_info_t *pi, uint8 tempsense_vbatsense);
static int16 wlc_phy_tempsense_vbatsense_acphy_20708
    (phy_info_t *pi, uint8 tempsense_vbatsense);
static int16 wlc_phy_tempsense_acphy_20709(phy_info_t *pi);
static int16 wlc_phy_tempsense_acphy_20710(phy_info_t *pi);
static int32 wlc_phy_tempsense_radio_swap_20698(phy_info_t *pi,
    acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static int32 wlc_phy_tempsense_radio_swap_20704(phy_info_t *pi,
    acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static int32 wlc_phy_tempsense_radio_swap_20707(phy_info_t *pi,
    acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static int32 wlc_phy_tempsense_radio_swap_20708(phy_info_t *pi,
    acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static void wlc_phy_tempsense_radio_swap_20709(phy_info_t *pi,
    acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);
static int32 wlc_phy_tempsense_radio_swap_20710(phy_info_t *pi,
    acphy_tempsense_cfg_opt_t type, uint8 swap, uint8 core);

static int32 wlc_phy_tempsense_poll_adc_war_20698(phy_info_t *pi,
    bool init_adc_inside, int16 *measured_values, uint8 core);
static int32 wlc_phy_tempsense_poll_adc_war_20704(phy_info_t *pi,
    bool init_adc_inside, int16 *measured_values, uint8 core);
static int32 wlc_phy_tempsense_poll_adc_war_20707(phy_info_t *pi,
    bool init_adc_inside, int16 *measured_values, uint8 core);
static int32 wlc_phy_tempsense_poll_adc_war_20708(phy_info_t *pi,
    bool init_adc_inside, int16 *measured_values, uint8 core);
static void wlc_phy_tempsense_poll_adc_20709(phy_info_t *pi,
    bool init_adc_inside, int32 *temperature, uint8 core);
static void wlc_phy_tempsense_poll_adc_20710(phy_info_t *pi,
    bool init_adc_inside, int32 *temperature, uint8 core);

static void
wlc_phy_tempsense_radio_setup_acphy_20698(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core);
static void
wlc_phy_tempsense_radio_setup_acphy_20704(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core);
static void
wlc_phy_tempsense_radio_setup_acphy_20707(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core);
static void
wlc_phy_tempsense_radio_setup_acphy_20708(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core);
static void
wlc_phy_tempsense_radio_setup_acphy_20709(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core);
static void
wlc_phy_tempsense_radio_setup_acphy_20710(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core);

static void
wlc_phy_tempsense_radio_cleanup_acphy_cacherestore(phy_info_t *pi, uint8 core);

static void
wlc_phy_tempsense_radio_setup_acphy_20704(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core)
{
    /* 20704_procs.tcl r773714: 20704_tempsense_vbat_radio_setup */

    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

    phy_ac_reg_cache_save_percore(pi_ac, RADIOREGS_TEMPSENSE_VBAT, core);

    RADIO_REG_LIST_START
        // Powerup
        MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG5, core, loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0x0)
        // Setup Aux path
        MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0)
        // Setup tempsense
        MOD_RADIO_REG_20704_ENTRY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0x1)
        // Setup sel_test_port
        MOD_RADIO_REG_20704_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1)
        // Setup Aux Path
        MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc, 0x0)
        MOD_RADIO_REG_20704_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc, 0x1)
        // Mux AUX path to ADC
        MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0)
        MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0)
        // Turn off TIA
        MOD_RADIO_REG_20704_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, TIA_REG7, core, tia_pu, 0x0)
        // Set AvVmid
        MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 0x1)
        MOD_RADIO_REG_20704_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 0x1)
    RADIO_REG_LIST_EXECUTE(pi, core);

    MOD_RADIO_REG_20704(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
    MOD_RADIO_REG_20704(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);

    return;
}

static void
wlc_phy_tempsense_radio_setup_acphy_20707(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

    phy_ac_reg_cache_save_percore(pi_ac, RADIOREGS_TEMPSENSE_VBAT, core);

    RADIO_REG_LIST_START
        // Powerup
        MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG5, core, loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0x0)
        // Setup Aux path
        MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0)
        // Setup tempsense
        MOD_RADIO_REG_20707_ENTRY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0x1)
        // Setup sel_test_port
        MOD_RADIO_REG_20707_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1)
        // Setup Aux Path
        MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc, 0x0)
        MOD_RADIO_REG_20707_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc, 0x1)
        // Mux AUX path to ADC
        MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0)
        MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0)
        // Turn off TIA
        MOD_RADIO_REG_20707_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, TIA_REG7, core, tia_pu, 0x0)
        // Set AvVmid
        MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 0x1)
        MOD_RADIO_REG_20707_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 0x1)
    RADIO_REG_LIST_EXECUTE(pi, core);

    MOD_RADIO_REG_20707(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
    MOD_RADIO_REG_20707(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);

    return;
}

static void
wlc_phy_tempsense_radio_setup_acphy_20708(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

    phy_ac_reg_cache_save_percore(pi_ac, RADIOREGS_TEMPSENSE_VBAT, core);

    RADIO_REG_LIST_START
        // Powerup
        MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG5, core, loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0x0)
        // Setup Aux path
        MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0)
        // Setup tempsense
        MOD_RADIO_REG_20708_ENTRY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0x1)
        // Setup sel_test_port
        MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1)
        // Setup Aux Path
        MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc, 0x0)
        MOD_RADIO_REG_20708_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc, 0x1)
        // Mux AUX path to ADC
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0)
        // Set AvVmid
        MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 0x1)
        MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 0x1)
    RADIO_REG_LIST_EXECUTE(pi, core);

    MOD_RADIO_REG_20708(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
    MOD_RADIO_REG_20708(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);

    if (RADIOMAJORREV(pi) >= 2) {
        // bias directly from BG instead of mirror ciruit
        MOD_RADIO_REG_20708(pi, TEMPSENSE_CFG, core, tempsense_ibias_sel, 1);
    }

    return;
}

static void
wlc_phy_tempsense_radio_setup_acphy_20709(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core)
{
    /* 20709_procs.tcl r803517: 20709_tempsense_vbat_radio_setup */

    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

    phy_ac_reg_cache_save_percore(pi_ac, RADIOREGS_TEMPSENSE_VBAT, core);

    RADIO_REG_LIST_START
        // Powerup
        MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG5, core, loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0x0)
        // Setup Aux path
        MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0)
        // Setup tempsense
        MOD_RADIO_REG_20709_ENTRY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0x1)
        // Setup sel_test_port
        MOD_RADIO_REG_20709_ENTRY(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1)
        // Setup Aux Path
        MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc, 0x0)
        MOD_RADIO_REG_20709_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc, 0x1)
        // Mux AUX path to ADC
        MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0)
        MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0)
        MOD_RADIO_REG_20709_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1)
        // Turn off TIA
        MOD_RADIO_REG_20709_ENTRY(pi, TIA_REG7, core, tia_pu, 0x0)
        MOD_RADIO_REG_20709_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x1)
        // Set AvVmid
        MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 0x1)
        MOD_RADIO_REG_20709_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 0x1)
    RADIO_REG_LIST_EXECUTE(pi, core);

    MOD_RADIO_REG_20709(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
    MOD_RADIO_REG_20709(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);

    return;
}

static void
wlc_phy_tempsense_radio_setup_acphy_20710(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core)
{
    /* 20710_procs.tcl r773714: 20710_tempsense_vbat_radio_setup */

    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

    phy_ac_reg_cache_save_percore(pi_ac, RADIOREGS_TEMPSENSE_VBAT, core);

    RADIO_REG_LIST_START
        // Powerup
        MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG5, core, loopback_bias_pu, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0x0)
        // Setup Aux path
        MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0)
        // Setup tempsense
        MOD_RADIO_REG_20710_ENTRY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0x1)
        // Setup sel_test_port
        MOD_RADIO_REG_20710_ENTRY(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1)
        // Setup Aux Path
        MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG4, core, iqcal2adc, 0x0)
        MOD_RADIO_REG_20710_ENTRY(pi, IQCAL_CFG4, core, auxpga2adc, 0x1)
        // Mux AUX path to ADC
        MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0)
        MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0)
        // Turn off TIA
        MOD_RADIO_REG_20710_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, TIA_REG7, core, tia_pu, 0x0)
        // Set AvVmid
        MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 0x1)
        MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 0x1)
    RADIO_REG_LIST_EXECUTE(pi, core);

    MOD_RADIO_REG_20710(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
    MOD_RADIO_REG_20710(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);

    return;
}

static void
wlc_phy_tempsense_radio_setup_acphy_20698(phy_info_t *pi, uint16 Av, uint16 Vmid, uint8 core)
{
    /* 20698_procs.tcl r708059: 20698_tempsense_vbat_radio_setup */

    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

    phy_ac_reg_cache_save_percore(pi_ac, RADIOREGS_TEMPSENSE_VBAT, core);

    //FIXME43684 need to condition on NO_DFS_CORE and NO_RADAR_CHAN
    //should the pu be after or before the save&restore
    //it was first before - now moved it after
    MOD_RADIO_REG_20698(pi, IQCAL_CFG5, core, loopback_bias_pu, 0x1);
    MOD_RADIO_REG_20698(pi, IQCAL_OVR1, core, ovr_loopback_bias_pu, 0x1);
    MOD_RADIO_REG_20698(pi, TESTBUF_CFG1, core, testbuf_sel_test_port, 0x1);
    MOD_RADIO_REG_20698(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 0x1);
    MOD_RADIO_REG_20698(pi, TESTBUF_CFG1, core, testbuf_PU, 0x1);
    MOD_RADIO_REG_20698(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 0x1);
    MOD_RADIO_REG_20698(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0x0);
    // setup Aux path
    MOD_RADIO_REG_20698(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0x1);
    MOD_RADIO_REG_20698(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0x1);
    MOD_RADIO_REG_20698(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
    MOD_RADIO_REG_20698(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 0x1);
    MOD_RADIO_REG_20698(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);
    MOD_RADIO_REG_20698(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 0x1);
    MOD_RADIO_REG_20698(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0);
    // setup tempsense
    MOD_RADIO_REG_20698(pi, TEMPSENSE_CFG, core, tempsense_pu, 0x1);
    MOD_RADIO_REG_20698(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0x1);
    // setup sel_test_port
    MOD_RADIO_REG_20698(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1);
    MOD_RADIO_REG_20698(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 0x1);
    //Setup Aux Path
    MOD_RADIO_REG_20698(pi, IQCAL_CFG4, core, iqcal2adc, 0x0);
    MOD_RADIO_REG_20698(pi, IQCAL_CFG4, core, auxpga2adc, 0x1);
    // Mux AUX path to ADC
    MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
    MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
    MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
    MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);
    MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x1);
    MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
    // Turn off TIA
    MOD_RADIO_REG_20698(pi, TIA_REG7, core, tia_pu, 0x0);
    MOD_RADIO_REG_20698(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x1);
    MOD_RADIO_REG_20698(pi, AFEBIAS_REG1, core, bias_reg1, 0x1);

    return;
}

static void
wlc_phy_tempsense_radio_cleanup_acphy_cacherestore(phy_info_t *pi, uint8 core)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    /* restore radio config back */
    phy_ac_reg_cache_restore_percore(pi_ac, RADIOREGS_TEMPSENSE_VBAT, core);

    return;
}

static void
wlc_phy_tempsense_radio_setup_acphy(phy_info_t *pi, uint16 Av, uint16 Vmid)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    tempsense_radioregs_t *porig =
        &(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs);
    uint8 core;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

    BCM_REFERENCE(stf_shdata);

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
    FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
        /* Reg conflict with 2069 rev 16 */
        if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0)
            porig->OVR18[core]         = READ_RADIO_REGC(pi, RF, OVR18, core);
        else
            porig->OVR19[core]         = READ_RADIO_REGC(pi, RF, GE16_OVR19, core);
        porig->tempsense_cfg[core] = READ_RADIO_REGC(pi, RF, TEMPSENSE_CFG, core);
        porig->OVR5[core]          = READ_RADIO_REGC(pi, RF, OVR5, core);
        porig->testbuf_cfg1[core]  = READ_RADIO_REGC(pi, RF, TESTBUF_CFG1, core);
        porig->OVR3[core]          = READ_RADIO_REGC(pi, RF, OVR3, core);
        porig->auxpga_cfg1[core]   = READ_RADIO_REGC(pi, RF, AUXPGA_CFG1, core);
        porig->auxpga_vmid[core]   = READ_RADIO_REGC(pi, RF, AUXPGA_VMID, core);

        MOD_RADIO_REGC(pi, OVR5, core, ovr_tempsense_pu, 0x1);
        MOD_RADIO_REGC(pi, TEMPSENSE_CFG, core, pu, 0x1);
        MOD_RADIO_REGC(pi, OVR5, core, ovr_testbuf_PU, 0x1);
        MOD_RADIO_REGC(pi, TESTBUF_CFG1, core, PU, 0x1);
        MOD_RADIO_REGC(pi, TESTBUF_CFG1, core, GPIO_EN, 0x0);
        MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_auxpga_i_sel_vmid, 0x1);
        MOD_RADIO_REGC(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
        MOD_RADIO_REGC(pi, OVR3, core, ovr_auxpga_i_sel_gain, 0x1);
        MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);

        if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
            MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0x0);
            /* This bit is supposed to be controlled by phy direct control line.
             * Please check: http://jira.broadcom.com/browse/HW11ACRADIO-45
             */
            MOD_RADIO_REGC(pi, AUXPGA_CFG1, core, auxpga_i_sel_input, 0x1);
        }
    }
}

static void
wlc_phy_tempsense_radio_cleanup_acphy(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    tempsense_radioregs_t *porig =
        &(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs);
    uint8 core;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

    BCM_REFERENCE(stf_shdata);

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
    FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
        /* Reg conflict with 2069 rev 16 */
        if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0)
            phy_utils_write_radioreg(pi, RF_2069_OVR18(core), porig->OVR18[core]);
        else
            phy_utils_write_radioreg(pi, RF_2069_GE16_OVR19(core), porig->OVR19[core]);
        phy_utils_write_radioreg(pi, RF_2069_TEMPSENSE_CFG(core),
            porig->tempsense_cfg[core]);
        phy_utils_write_radioreg(pi, RF_2069_OVR5(core), porig->OVR5[core]);
        phy_utils_write_radioreg(pi, RF_2069_TESTBUF_CFG1(core), porig->testbuf_cfg1[core]);
        phy_utils_write_radioreg(pi, RF_2069_OVR3(core), porig->OVR3[core]);
        phy_utils_write_radioreg(pi, RF_2069_AUXPGA_CFG1(core), porig->auxpga_cfg1[core]);
        phy_utils_write_radioreg(pi, RF_2069_AUXPGA_VMID(core), porig->auxpga_vmid[core]);
    }
}

static uint8
phy_ac_temp_get_first_actv_core(uint8 coremask)
{
    /* Get the first active core */
    switch (coremask & (256-coremask)) {
        case 1:
            return 0;
        case 2:
            return 1;
        case 4:
            return 2;
        case 8:
            return 3;
        default:
            return 0;
    }
}

static void
wlc_phy_tempsense_phy_setup_acphy(phy_info_t *pi, uint8 coreidx)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    acphy_tempsense_phyregs_t *porig = (pi_ac->tempi->ac_tempsense_phyregs_orig);
    uint16 sdadc_config, mask;
    uint8 core;
    uint8 phyrxchain;

    BCM_REFERENCE(phyrxchain);

    if (CHSPEC_IS80(pi->radio_chanspec) || PHY_AS_80P80(pi, pi->radio_chanspec)) {
        sdadc_config = sdadc_cfg80;
    } else if (CHSPEC_IS160(pi->radio_chanspec)) {
        sdadc_config = sdadc_cfg80; // FIXME
    } else if (CHSPEC_IS40(pi->radio_chanspec)) {
        if (pi->sdadc_config_override)
            sdadc_config = sdadc_cfg40hs;
        else
        sdadc_config = sdadc_cfg40;
    } else {
        sdadc_config = sdadc_cfg20;
    }

    if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        porig->DcFiltAddress = READ_PHYREG(pi, DcFiltAddress);
    }

    porig->RxFeCtrl1 = READ_PHYREG(pi, RxFeCtrl1);
    MOD_PHYREG_4(pi, RxFeCtrl1, swap_iq0, 0, swap_iq1, 0, swap_iq2, 0, swap_iq3, 0);

    phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;

    if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        ASSERT((1<<coreidx) & phyrxchain);
    }

    FOREACH_ACTV_CORE(pi, phyrxchain, core) {

        if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && (core != coreidx))
            continue;

        mask = ACPHY_RfctrlIntc0_override_tr_sw_MASK(rev) |
            ACPHY_RfctrlIntc0_tr_sw_tx_pu_MASK(rev);
        porig->RfctrlIntc[core] = phy_utils_read_phyreg(pi,
            ACPHYREGCE(pi, RfctrlIntc, core));
        phy_utils_write_phyreg(pi, ACPHYREGCE(pi,
            RfctrlIntc, core), mask);  /* elna off */

        porig->RfctrlOverrideAuxTssi[core]  = READ_PHYREGCE(pi, RfctrlOverrideAuxTssi,
                                                            core);
        porig->RfctrlCoreAuxTssi1[core]     = READ_PHYREGCE(pi, RfctrlCoreAuxTssi1, core);
        porig->RfctrlOverrideRxPus[core]    = READ_PHYREGCE(pi, RfctrlOverrideRxPus, core);
        porig->RfctrlCoreRxPus[core]        = READ_PHYREGCE(pi, RfctrlCoreRxPus, core);
        porig->RfctrlOverrideTxPus[core]    = READ_PHYREGCE(pi, RfctrlOverrideTxPus, core);
        porig->RfctrlCoreTxPus[core]        = READ_PHYREGCE(pi, RfctrlCoreTxPus, core);
        porig->RfctrlOverrideLpfSwtch[core] = READ_PHYREGCE(pi, RfctrlOverrideLpfSwtch,
                                                            core);
        porig->RfctrlCoreLpfSwtch[core]     = READ_PHYREGCE(pi, RfctrlCoreLpfSwtch, core);
        porig->RfctrlOverrideAfeCfg[core]   = READ_PHYREGCE(pi, RfctrlOverrideAfeCfg, core);
        porig->RfctrlCoreAfeCfg1[core]      = READ_PHYREGCE(pi, RfctrlCoreAfeCfg1, core);
        porig->RfctrlCoreAfeCfg2[core]      = READ_PHYREGCE(pi, RfctrlCoreAfeCfg2, core);
        porig->RfctrlOverrideGains[core]    = READ_PHYREGCE(pi, RfctrlOverrideGains, core);
        porig->RfctrlCoreLpfGain[core]      = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);
        porig->RfctrlCoreRXGAIN1[core]      = READ_PHYREGCE(pi, RfctrlCoreRXGAIN1, core);

        MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, amux_sel_port, 1);
        MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, amux_sel_port, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, afe_iqadc_aux_en, 1);
        MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, afe_iqadc_aux_en, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus,    core, lpf_pu_dc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus,        core, lpf_pu_dc, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideTxPus,    core, lpf_bq1_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreTxPus,        core, lpf_bq1_pu, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideTxPus,    core, lpf_bq2_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreTxPus,        core, lpf_bq2_pu, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideTxPus,    core, lpf_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreTxPus,        core, lpf_pu, 1);

        WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core,         0x154);
        WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core,     0x3ff);
        if (!ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
            wlc_phy_txcal_phy_setup_acphy_core_sd_adc(pi, core, sdadc_config);
        }

        MOD_PHYREGCE(pi, RfctrlCoreLpfGain,    core, lpf_bq1_gain,    0);
        MOD_PHYREGCE(pi, RfctrlOverrideGains,    core, lpf_bq1_gain,    1);
        MOD_PHYREGCE(pi, RfctrlOverrideGains,    core, rxgain,        1);
        MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1,    core, rxgain_dvga,    0);
    }

    // Need to enable dc bypass for 7271
    if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        MOD_PHYREG(pi, DcFiltAddress, dcBypass, 1);
    }
}

static void
wlc_phy_tempsense_phy_cleanup_acphy(phy_info_t *pi, uint8 coreidx)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    acphy_tempsense_phyregs_t *porig = (pi_ac->tempi->ac_tempsense_phyregs_orig);
    uint8 core;
    uint8 phyrxchain;

    BCM_REFERENCE(phyrxchain);

    WRITE_PHYREG(pi, RxFeCtrl1, porig->RxFeCtrl1);
    if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        WRITE_PHYREG(pi, DcFiltAddress, porig->DcFiltAddress);
    }

    phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
    if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        ASSERT((1<<coreidx) & phyrxchain);
    }

    FOREACH_ACTV_CORE(pi, phyrxchain, core) {

        if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && (core != coreidx))
            continue;

        WRITE_PHYREGCE(pi, RfctrlIntc, core, porig->RfctrlIntc[core]);

        WRITE_PHYREGCE(pi, RfctrlOverrideAuxTssi, core, porig->RfctrlOverrideAuxTssi[core]);
        WRITE_PHYREGCE(pi, RfctrlCoreAuxTssi1, core, porig->RfctrlCoreAuxTssi1[core]);
        WRITE_PHYREGCE(pi, RfctrlOverrideRxPus, core, porig->RfctrlOverrideRxPus[core]);
        WRITE_PHYREGCE(pi, RfctrlCoreRxPus, core, porig->RfctrlCoreRxPus[core]);
        WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, core, porig->RfctrlOverrideTxPus[core]);
        WRITE_PHYREGCE(pi, RfctrlCoreTxPus, core, porig->RfctrlCoreTxPus[core]);
        WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core,
                porig->RfctrlOverrideLpfSwtch[core]);
        WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, porig->RfctrlCoreLpfSwtch[core]);
        WRITE_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, porig->RfctrlOverrideAfeCfg[core]);
        WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, porig->RfctrlCoreAfeCfg1[core]);
        WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, porig->RfctrlCoreAfeCfg2[core]);
        WRITE_PHYREGCE(pi, RfctrlOverrideGains, core, porig->RfctrlOverrideGains[core]);
        WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core, porig->RfctrlCoreLpfGain[core]);
        WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, porig->RfctrlCoreRXGAIN1[core]);
    }
}

static int32
wlc_phy_tempsense_paldosense_phy_setup_acphy_tiny(phy_info_t *pi, uint8 tempsense_paldosense
, uint8 core)
{
    /* # this proc is used to configure the tempsense phy settings. */
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    acphy_tempsense_phyregs_t *porig = (pi_ac->tempi->ac_tempsense_phyregs_orig);

    /* Applicable # foreach core */
    porig->RfctrlOverrideAuxTssi[core] = READ_PHYREGCE(pi, RfctrlOverrideAuxTssi, core);
    porig->RfctrlCoreAuxTssi1[core]    = READ_PHYREGCE(pi, RfctrlCoreAuxTssi1, core);
    porig->RfctrlOverrideRxPus[core]   = READ_PHYREGCE(pi, RfctrlOverrideRxPus, core);
    porig->RfctrlCoreRxPus[core]       = READ_PHYREGCE(pi, RfctrlCoreRxPus, core);
    porig->RxFeCtrl1                   = READ_PHYREG(pi, RxFeCtrl1);
    porig->RfctrlOverrideTxPus[core]   = READ_PHYREGCE(pi, RfctrlOverrideTxPus, core);
    porig->RfctrlCoreTxPus[core]       = READ_PHYREGCE(pi, RfctrlCoreTxPus, core);
    porig->RfctrlOverrideGains[core]   = READ_PHYREGCE(pi, RfctrlOverrideGains, core);
    porig->RfctrlCoreLpfGain[core]     = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);
    porig->RxSdFeConfig1               = READ_PHYREG(pi, RxSdFeConfig1);
    porig->RxSdFeConfig6               = READ_PHYREG(pi, RxSdFeConfig6);
    porig->RfctrlOverrideLpfSwtch[core] = READ_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core);
    porig->RfctrlCoreLpfSwtch[core]    = READ_PHYREGCE(pi, RfctrlCoreLpfSwtch, core);
    porig->RfctrlOverrideAfeCfg[core]  = READ_PHYREGCE(pi, RfctrlOverrideAfeCfg, core);
    porig->RfctrlCoreAfeCfg1[core]     = READ_PHYREGCE(pi, RfctrlCoreAfeCfg1, core);
    porig->RfctrlCoreAfeCfg2[core]     = READ_PHYREGCE(pi, RfctrlCoreAfeCfg2, core);

    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        // Additional settings for 4365 core3 GPIO WAR
        if (core == 3) {
            MOD_PHYREG(pi, DcFiltAddress,         dcBypass,       1);
            MOD_PHYREG(pi, RfctrlCoreRXGAIN13,     rxgain_dvga,  0);
            MOD_PHYREG(pi, RfctrlCoreLpfGain3,     lpf_bq2_gain, 0);
            MOD_PHYREG(pi, RfctrlOverrideGains3, rxgain,       1);
            MOD_PHYREG(pi, RfctrlOverrideGains3, lpf_bq2_gain, 1);
        }

        MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, amux_sel_port, 1);
        MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, amux_sel_port, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideAuxTssi,  core, afe_iqadc_aux_en, 1);
        MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     core, afe_iqadc_aux_en, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus,     core, lpf_pu_dc, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus,         core, lpf_pu_dc, 0);
        WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core,         0x154);
        WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core,     0x3ff);

        /* Power down LNAs - isolation of TIA power down is insufficient */
        /* at high signal power */
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus,     core, rxrf_lna1_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideRxPus,     core, rxrf_lna1_5G_pwrup, 1);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, rxrf_lna1_pwrup, 0);
        MOD_PHYREGCE(pi, RfctrlCoreRxPus,     core, rxrf_lna1_5G_pwrup, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideTxPus,     core, lpf_bq1_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreTxPus,         core, lpf_bq1_pu, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideTxPus,     core, lpf_bq2_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreTxPus,         core, lpf_bq2_pu, 0);
        MOD_PHYREGCE(pi, RfctrlOverrideTxPus,     core, lpf_pu, 1);
        MOD_PHYREGCE(pi, RfctrlCoreTxPus,         core, lpf_pu, 1);
        MOD_PHYREGCE(pi, RfctrlOverrideGains,     core, lpf_bq1_gain, 1);
        MOD_PHYREGCE(pi, RfctrlCoreLpfGain,         core, lpf_bq1_gain, 0);

        MOD_PHYREG(pi, RxFeCtrl1, swap_iq0, 0);
        MOD_PHYREG(pi, RxFeCtrl1, swap_iq1, 0);
        MOD_PHYREG(pi, RxFeCtrl1, swap_iq2, 0);
        MOD_PHYREG(pi, RxFeCtrl1, swap_iq3, 0);
        MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 0);
        MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
        MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0,
        READ_PHYREGFLD(pi, RxSdFeConfig1, farrow_rshift_tx));
    } else {
        ACPHY_REG_LIST_START
            MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideAuxTssi,  0, afe_iqadc_aux_en, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlCoreAuxTssi1,     0, afe_iqadc_aux_en, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,     0, lpf_pu_dc, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,         0, lpf_pu_dc, 0)
            MOD_PHYREG_ENTRY(pi, RxFeCtrl1, swap_iq0, 0)
            MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_bq1_pu, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_bq1_pu, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_bq2_pu, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_bq2_pu, 0)
            MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideTxPus,    0, lpf_pu, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlCoreTxPus,        0, lpf_pu, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideGains,    0, lpf_bq1_gain, 1)
            MOD_PHYREGCE_ENTRY(pi, RfctrlCoreLpfGain,      0, lpf_bq1_gain, 0)

            MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideAuxTssi,  0, amux_sel_port, 1)
        ACPHY_REG_LIST_EXECUTE(pi);
            MOD_PHYREGCE(pi, RfctrlCoreAuxTssi1,     0, amux_sel_port,
                tempsense_paldosense ? 3 : 1);
            MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_force, 1);
            MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, tempsense_paldosense ? 2 :
                ACMAJORREV_4(pi->pubpi->phy_rev) ? 1 :
                READ_PHYREGFLD(pi, RxSdFeConfig1, farrow_rshift_tx));
        if (!(tempsense_paldosense)) {
            /* Power down LNAs - isolation of TIA power down is insufficient */
            /* at high signal power */
            ACPHY_REG_LIST_START
                MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,     0,
                    rxrf_lna1_pwrup, 1)
                MOD_PHYREGCE_ENTRY(pi, RfctrlOverrideRxPus,     0,
                    rxrf_lna1_5G_pwrup, 1)
                MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,     0, rxrf_lna1_pwrup, 0)
                MOD_PHYREGCE_ENTRY(pi, RfctrlCoreRxPus,     0, rxrf_lna1_5G_pwrup, 0)
            ACPHY_REG_LIST_EXECUTE(pi);
            if (ACMAJORREV_4(pi->pubpi->phy_rev))
                MOD_PHYREG(pi, RxSdFeConfig1, farrow_rshift_tx, 1);
        }
    }
    return BCME_OK;
}

static int32
wlc_phy_tempsense_paldosense_radio_setup_acphy_tiny(phy_info_t *pi, uint16 Av, uint16 Vmid,
uint8 tempsense_paldosense, uint8 coreidx)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    tempsense_radioregs_tiny_t *porig =
        &(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs_tiny);
    uint8 core, cores;
    if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev))
        cores = PHYCORENUM((pi)->pubpi->phy_corenum);
    else
        cores = 1;

    for (core = 0; core < cores; core++) {
        if ((ACMAJORREV_32(pi->pubpi->phy_rev) ||
            ACMAJORREV_33(pi->pubpi->phy_rev)) && (core != coreidx))
            continue;

        porig->tempsense_cfg[core]     = READ_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core);
        porig->tempsense_ovr1[core]    = READ_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core);
        porig->testbuf_cfg1[core]      = READ_RADIO_REG_TINY(pi, TESTBUF_CFG1, core);
        porig->testbuf_ovr1[core]      = READ_RADIO_REG_TINY(pi, TESTBUF_OVR1, core);
        porig->auxpga_cfg1[core]       = READ_RADIO_REG_TINY(pi, AUXPGA_CFG1, core);
        porig->auxpga_vmid[core]       = READ_RADIO_REG_TINY(pi, AUXPGA_VMID, core);
        porig->auxpga_ovr1[core]       = READ_RADIO_REG_TINY(pi, AUXPGA_OVR1, core);
        porig->tia_cfg5[core]          = READ_RADIO_REG_TINY(pi, TIA_CFG5, core);
        porig->tia_cfg7[core]          = READ_RADIO_REG_TINY(pi, TIA_CFG7, core);
        porig->tia_cfg9[core]          = READ_RADIO_REG_TINY(pi, TIA_CFG9, core);
        porig->adc_ovr1[core]          = READ_RADIO_REG_TINY(pi, ADC_OVR1, core);
        porig->adc_cfg10[core]         = READ_RADIO_REG_TINY(pi, ADC_CFG10, core);
        porig->rx_bb_2g_ovr_east[core] = READ_RADIO_REG_TINY(pi, RX_BB_2G_OVR_EAST, core);

        if (!(tempsense_paldosense)) {
            /* # Setup Tempsense */
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 1);
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_pu, 1);
        } else {
            /* # Setup paldosense */
            MOD_RADIO_REG_TINY(pi, VBAT_OVR1, core, ovr_vbat_monitor_pu, 1);
            MOD_RADIO_REG_TINY(pi, VBAT_CFG, core, vbat_monitor_pu, 1);
        }

        /* # Setup Testbuf */
        MOD_RADIO_REG_TINY(pi, TESTBUF_OVR1, core, ovr_testbuf_PU, 1);
        MOD_RADIO_REG_TINY(pi, TESTBUF_CFG1, core, testbuf_PU, 1);
        MOD_RADIO_REG_TINY(pi, TESTBUF_CFG1, core, testbuf_GPIO_EN, 0);
        MOD_RADIO_REG_TINY(pi, TESTBUF_OVR1, core, ovr_testbuf_sel_test_port, 1);

        MOD_RADIO_REG_TINY(pi, TESTBUF_CFG1, core, testbuf_sel_test_port,
            tempsense_paldosense ? 3 : 1);

        /* # Setup AuxPGA */
        MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 1);
        MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 1);
        MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_vmid, 1);

        MOD_RADIO_REG_TINY(pi, AUXPGA_VMID, core, auxpga_i_sel_vmid, Vmid);
        MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_gain, 1);
        MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_sel_gain, Av);
        MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_vcm_ctrl, 0);
        MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_sel_input, 1);
        MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_sel_input,
            tempsense_paldosense ? 3 : 1);
        if (tempsense_paldosense) {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_tx2g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX2G_CFG1, core, tx2g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_pa2g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA2G_CFG1, core, pa2g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_pa2g_bias_cas_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA2G_IDAC2, core,
                    pa2g_bias_cas_pu, 1);
            } else {
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
                    ovr_tx5g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX5G_CFG1, core, tx5g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
                    ovr_pa5g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core, pa5g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR2, core,
                    ovr_pa5g_bias_cas_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core, pa5g_bias_cas_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core, ovr_pa5g_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA5G_CFG4, core, pa5g_pu, 1);
            }
        }

        /* # Setup Aux Path */
        MOD_RADIO_REG_TINY(pi, TIA_CFG9, core, txbb_dac2adc, 0);
        MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_in_test, 1);
        MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_in_test, 0xF);
        MOD_RADIO_REG_TINY(pi, TIA_CFG5, core, tia_out_test, 1);

        /* # Turn off TIA otherwise it dominates the ADC input */
        MOD_RADIO_REG_TINY(pi, RX_BB_2G_OVR_EAST, core, ovr_tia_amp1_pwrup, 1);
        MOD_RADIO_REG_TINY(pi, TIA_CFG5, core, tia_amp1_pwrup, 0);
        MOD_RADIO_REG_TINY(pi, RX_BB_2G_OVR_EAST, core, ovr_tia_pwrup_amp2, 1);
        MOD_RADIO_REG_TINY(pi, TIA_CFG7, core, tia_pwrup_amp2, 0);
    }
    return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_adc_war_tiny(phy_info_t *pi, bool init_adc_inside, int32 *measured_values
, uint8 core)
{
    uint16 nsamp = 200;
    wlc_phy_tempsense_radio_swap_tiny(pi, ACPHY_TEMPSENSE_VBE, 0, core);
    measured_values[0] = wlc_phy_tempsense_poll_samps_tiny(pi, nsamp, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_tiny(pi, ACPHY_TEMPSENSE_VBG, 0, core);
    measured_values[1] = wlc_phy_tempsense_poll_samps_tiny(pi, nsamp, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_tiny(pi, ACPHY_TEMPSENSE_VBE, 1, core);
    measured_values[2] = wlc_phy_tempsense_poll_samps_tiny(pi, nsamp, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_tiny(pi, ACPHY_TEMPSENSE_VBG, 1, core);
    measured_values[3] = wlc_phy_tempsense_poll_samps_tiny(pi, nsamp, init_adc_inside, core);

    return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_adc_war_20704(phy_info_t *pi, bool init_adc_inside, int16 *measured_values,
    uint8 core)
{
    wlc_phy_tempsense_radio_swap_20704(pi, ACPHY_TEMPSENSE_VBE, 0, core);
    measured_values[0] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20704(pi, ACPHY_TEMPSENSE_VBG, 0, core);
    measured_values[1] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20704(pi, ACPHY_TEMPSENSE_VBE, 1, core);
    measured_values[2] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20704(pi, ACPHY_TEMPSENSE_VBG, 1, core);
    measured_values[3] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);
    return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_adc_war_20707(phy_info_t *pi, bool init_adc_inside, int16 *measured_values,
    uint8 core)
{
    wlc_phy_tempsense_radio_swap_20707(pi, ACPHY_TEMPSENSE_VBE, 0, core);
    measured_values[0] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20707(pi, ACPHY_TEMPSENSE_VBG, 0, core);
    measured_values[1] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20707(pi, ACPHY_TEMPSENSE_VBE, 1, core);
    measured_values[2] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20707(pi, ACPHY_TEMPSENSE_VBG, 1, core);
    measured_values[3] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);
    return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_adc_war_20708(phy_info_t *pi, bool init_adc_inside, int16 *measured_values,
    uint8 core)
{
    uint8 stall_val;
    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);

    wlc_phy_tempsense_radio_swap_20708(pi, ACPHY_TEMPSENSE_VBE, 0, core);
    measured_values[0] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20708(pi, ACPHY_TEMPSENSE_VBG, 0, core);
    measured_values[1] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20708(pi, ACPHY_TEMPSENSE_VBE, 1, core);
    measured_values[2] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20708(pi, ACPHY_TEMPSENSE_VBG, 1, core);
    measured_values[3] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    ACPHY_ENABLE_STALL(pi, stall_val);
    return BCME_OK;
}

static void
wlc_phy_tempsense_poll_adc_20709(phy_info_t *pi, bool init_adc_inside, int32 *temperature,
    uint8 core)
{
    const int32 t_scale = 16384;
    const int32 t_slope[] = {6189, 5969, 5774, 6464};
    const int32 t_offset[] = {1842100, 1859900, 1830430, 1838500};
    int16 measured_voltage[4] = {0};
    int32 radio_temp = 0;

    wlc_phy_tempsense_radio_swap_20709(pi, ACPHY_TEMPSENSE_VBE, 0, core);
    measured_voltage[0] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20709(pi, ACPHY_TEMPSENSE_VBG, 0, core);
    measured_voltage[1] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20709(pi, ACPHY_TEMPSENSE_VBE, 1, core);
    measured_voltage[2] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20709(pi, ACPHY_TEMPSENSE_VBG, 1, core);
    measured_voltage[3] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    radio_temp += (int32)(((measured_voltage[0] + measured_voltage[2]
        - measured_voltage[1] - measured_voltage[3]) / 2)) * t_slope[core];

    radio_temp = (radio_temp + t_offset[core]) / t_scale;

    PHY_THERMAL(("phy_tempsense::measured_voltage[core%d]=[%d,%d,%d,%d], temp=%d C\n",
            core, measured_voltage[0],
            measured_voltage[1], measured_voltage[2],
            measured_voltage[3], (int) radio_temp));

    *temperature = radio_temp;
}

static void
wlc_phy_tempsense_poll_adc_20710(phy_info_t *pi, bool init_adc_inside, int32 *temperature,
    uint8 core)
{
    const int32 t_scale = 16384;
    const int32 t_slope[] = {6189, 5969, 5774, 6464};
    const int32 t_offset[] = {1842100, 1859900, 1830430, 1838500};
    int16 measured_voltage[4] = {0};
    int32 radio_temp = 0;

    wlc_phy_tempsense_radio_swap_20710(pi, ACPHY_TEMPSENSE_VBE, 0, core);
    measured_voltage[0] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20710(pi, ACPHY_TEMPSENSE_VBG, 0, core);
    measured_voltage[1] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20710(pi, ACPHY_TEMPSENSE_VBE, 1, core);
    measured_voltage[2] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20710(pi, ACPHY_TEMPSENSE_VBG, 1, core);
    measured_voltage[3] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    radio_temp += (int32)(((measured_voltage[0] + measured_voltage[2]
        - measured_voltage[1] - measured_voltage[3]) / 2)) * t_slope[core];

    radio_temp = (radio_temp + t_offset[core]) / t_scale;

    PHY_THERMAL(("phy_tempsense::measured_voltage[core%d]=[%d,%d,%d,%d], temp=%d C\n",
            core, measured_voltage[0],
            measured_voltage[1], measured_voltage[2],
            measured_voltage[3], (int) radio_temp));

    *temperature = radio_temp;
}

static int32
wlc_phy_tempsense_poll_adc_war_20698(phy_info_t *pi, bool init_adc_inside, int16 *measured_values,
    uint8 core)
{
    wlc_phy_tempsense_radio_swap_20698(pi, ACPHY_TEMPSENSE_VBE, 0, core);
    measured_values[0] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20698(pi, ACPHY_TEMPSENSE_VBG, 0, core);
    measured_values[1] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20698(pi, ACPHY_TEMPSENSE_VBE, 1, core);
    measured_values[2] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);

    wlc_phy_tempsense_radio_swap_20698(pi, ACPHY_TEMPSENSE_VBG, 1, core);
    measured_values[3] = wlc_phy_tempsense_poll_samps_tiny(pi, 200, init_adc_inside, core);
    return BCME_OK;
}

static int32
wlc_phy_tempsense_poll_samps_tiny(phy_info_t *pi, uint16 samples, bool init_adc_inside
, uint8 core)
{
    int32 measured_voltage;
    int32 i_val = 0;
    int32 i_sum = 0;
    int32 i_sum_limit = 4096;
    int32 retry, retry_limit, iq_sign = 0;
    int i = 0;
    uint16 mask, signcheck;
    uint32 signextend, discardbits;
    uint16 nsamps = 0;
    /* Need to set the swap bit, otherwise there is a bug */
    if (init_adc_inside) {
        if (!((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
            (core == 3)) && !(ACMAJORREV_GE47(pi->pubpi->phy_rev))) {
            wlc_phy_tempsense_gpiosel_tiny(pi, 16, 1);
        }
    }
    if (ACMAJORREV_4(pi->pubpi->phy_rev) ||
        ACMAJORREV_32(pi->pubpi->phy_rev)|| ACMAJORREV_33(pi->pubpi->phy_rev) ||
        ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        /* 13 bit signed to 16 bit signed conversion. */
        mask = 0x1FFF;
        signcheck = 0x1000;
        signextend = 0xFFFFF000;
        discardbits = 3;
    } else {
        /* 12 bit signed to 16 bit signed conversion. */
        mask = 0x0FFF;
        signcheck = 0x0800;
        signextend = 0xFFFFF800;
        discardbits = 2;
    }

    OSL_DELAY(10);
    if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
        (core == 3)) {
            phy_iq_est_t rx_iq_est[PHY_CORE_MAX];
            MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 1);
            nsamps = 2*(1<<2*discardbits);
            wlc_phy_rx_iq_est_acphy(pi, rx_iq_est,    nsamps, 32, 0, TRUE);
            i_sum = (int32)
                math_sqrt_int_32((uint32)(rx_iq_est[core].i_pwr))*4;
            measured_voltage = 0 - i_sum;
            //MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);
    } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        phy_iq_est_t rx_iq_est[PHY_CORE_MAX];
        //MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 1);
        nsamps = 2 * (1 << 2 * discardbits);
        retry_limit = ACMAJORREV_130(pi->pubpi->phy_rev)? 4 : 1;
        for (retry = 0; retry < retry_limit; retry++) {
            wlc_phy_rx_iq_est_acphy(pi, rx_iq_est,    128, 32, 0, TRUE);
            i_sum = (int32)
                (math_sqrt_int_32((uint32)(rx_iq_est[core].i_pwr)/2));
            if (i_sum < i_sum_limit) { break; }
        }
        iq_sign = (int32)rx_iq_est[core].iq_prod;
        if (ACMAJORREV_130(pi->pubpi->phy_rev) && iq_sign >= 0) {
            measured_voltage = i_sum;
        } else {
            measured_voltage = 0 - i_sum;
        }
    } else {
        for (i = 0; i < samples; i++) {
            i_val = READ_PHYREG(pi, gpioHiOut);
            i_val = (i_val & mask);
            if ((i_val & signcheck)) {
                i_val = (i_val | signextend);
            }
            i_sum += i_val;
        }
        measured_voltage = i_sum/samples;
    }
    return (measured_voltage >> discardbits);
}

static int32
wlc_phy_tempsense_gpiosel_tiny(phy_info_t *pi, uint16 sel, uint8 word_swap)
{
    if (!(ACMAJORREV_32(pi->pubpi->phy_rev) ||
            ACMAJORREV_33(pi->pubpi->phy_rev))) {
        W_REG(pi->sh->osh, D11_PSM_GPIOEN(pi), (uint16)0x0);
    }
    WRITE_PHYREG(pi, gpioLoOutEn, 0x0000);
    WRITE_PHYREG(pi, gpioHiOutEn, 0X8000);

    /* set up acphy GPIO sel */
    WRITE_PHYREG(pi, gpioSel, (word_swap<<8) | sel);
    return BCME_OK;
}

static int32
wlc_phy_tempsense_radio_swap_tiny(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap
, uint8 coreidx)
{
    uint8 core, cores;
    if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev))
        cores = PHYCORENUM((pi)->pubpi->phy_corenum);
    else
        cores = 1;

    for (core = 0; core < cores; core++) {
        if ((ACMAJORREV_32(pi->pubpi->phy_rev) ||
            ACMAJORREV_33(pi->pubpi->phy_rev)) && core != coreidx)
            continue;
        /* Enable override */
        MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
        MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

        if (swap == 0) {
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
        } else if (swap == 1) {
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
        } else {
            PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
            return BCME_ERROR;
        }
        if (type == ACPHY_TEMPSENSE_VBG) {
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
        } else if (type == ACPHY_TEMPSENSE_VBE) {
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
        } else {
            PHY_ERROR(("Unsupported, supported types are"));
            PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
            return BCME_ERROR;
        }
    }
    return BCME_OK;
}

static int32
wlc_phy_tempsense_radio_swap_20704(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap,
    uint8 core)
{
    /* 20704_procs.tcl r773714: 20704_tempsense_radio_swap */

    /* Enable override */
    MOD_RADIO_REG_20704(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
    MOD_RADIO_REG_20704(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

    if (swap == 0) {
        MOD_RADIO_REG_20704(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
    } else if (swap == 1) {
        MOD_RADIO_REG_20704(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
    } else {
        PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
        return BCME_ERROR;
    }
    if (type == ACPHY_TEMPSENSE_VBG) {
        MOD_RADIO_REG_20704(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
    } else if (type == ACPHY_TEMPSENSE_VBE) {
        MOD_RADIO_REG_20704(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
    } else {
        PHY_ERROR(("Unsupported, supported types are"));
        PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
        return BCME_ERROR;
    }
    return BCME_OK;
}

static int32
wlc_phy_tempsense_radio_swap_20707(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap,
        uint8 core)
{
    /* Enable override */
    MOD_RADIO_REG_20707(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
    MOD_RADIO_REG_20707(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

    if (swap == 0) {
        MOD_RADIO_REG_20707(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
    } else if (swap == 1) {
        MOD_RADIO_REG_20707(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
    } else {
        PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
        return BCME_ERROR;
    }
    if (type == ACPHY_TEMPSENSE_VBG) {
        MOD_RADIO_REG_20707(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
    } else if (type == ACPHY_TEMPSENSE_VBE) {
        MOD_RADIO_REG_20707(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
    } else {
        PHY_ERROR(("Unsupported, supported types are"));
        PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
        return BCME_ERROR;
    }
    return BCME_OK;
}

static int32
wlc_phy_tempsense_radio_swap_20708(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap,
        uint8 core)
{
    /* Enable override */
    MOD_RADIO_REG_20708(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
    //MOD_RADIO_REG_20708(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

    if (swap == 0) {
        //MOD_RADIO_REG_20708(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
    } else if (swap == 1) {
        //MOD_RADIO_REG_20708(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
    } else {
        PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
        return BCME_ERROR;
    }
    if (type == ACPHY_TEMPSENSE_VBG) {
        MOD_RADIO_REG_20708(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
    } else if (type == ACPHY_TEMPSENSE_VBE) {
        MOD_RADIO_REG_20708(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
    } else {
        PHY_ERROR(("Unsupported, supported types are"));
        PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
        return BCME_ERROR;
    }
    return BCME_OK;
}

static void
wlc_phy_tempsense_radio_swap_20709(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap,
    uint8 core)
{
    /* 20709_procs.tcl r803517: 20709_tempsense_radio_swap */

    /* Enable override */
    MOD_RADIO_REG_20709(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
    MOD_RADIO_REG_20709(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

    if (swap == 0) {
        MOD_RADIO_REG_20709(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
    } else if (swap == 1) {
        MOD_RADIO_REG_20709(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
    } else {
        PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
        ASSERT(0);
    }
    if (type == ACPHY_TEMPSENSE_VBG) {
        MOD_RADIO_REG_20709(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
    } else if (type == ACPHY_TEMPSENSE_VBE) {
        MOD_RADIO_REG_20709(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
    } else {
        PHY_ERROR(("Unsupported, supported types are"));
        PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
        ASSERT(0);
    }
}

static int32
wlc_phy_tempsense_radio_swap_20710(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap,
    uint8 core)
{
    /* 20710_procs.tcl r773714: 20710_tempsense_radio_swap */

    /* Enable override */
    MOD_RADIO_REG_20710(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
    MOD_RADIO_REG_20710(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

    if (swap == 0) {
        MOD_RADIO_REG_20710(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
    } else if (swap == 1) {
        MOD_RADIO_REG_20710(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
    } else {
        PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
        return BCME_ERROR;
    }
    if (type == ACPHY_TEMPSENSE_VBG) {
        MOD_RADIO_REG_20710(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
    } else if (type == ACPHY_TEMPSENSE_VBE) {
        MOD_RADIO_REG_20710(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
    } else {
        PHY_ERROR(("Unsupported, supported types are"));
        PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
        return BCME_ERROR;
    }
    return BCME_OK;
}

static int32
wlc_phy_tempsense_radio_swap_20698(phy_info_t *pi, acphy_tempsense_cfg_opt_t type, uint8 swap,
    uint8 core)
{
    /* 20698_procs.tcl r708059: 20698_tempsense_radio_swap */

    /* Enable override */
    MOD_RADIO_REG_20698(pi, TEMPSENSE_OVR1, core, ovr_tempsense_sel_Vbe_Vbg, 1);
    MOD_RADIO_REG_20698(pi, TEMPSENSE_OVR1, core, ovr_tempsense_swap_amp, 1);

    if (swap == 0) {
        MOD_RADIO_REG_20698(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 0);
    } else if (swap == 1) {
        MOD_RADIO_REG_20698(pi, TEMPSENSE_CFG, core, tempsense_swap_amp, 1);
    } else {
        PHY_ERROR(("Unsupported, swap should be 0 or 1\n"));
        return BCME_ERROR;
    }
    if (type == ACPHY_TEMPSENSE_VBG) {
        MOD_RADIO_REG_20698(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 0);
    } else if (type == ACPHY_TEMPSENSE_VBE) {
        MOD_RADIO_REG_20698(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, 1);
    } else {
        PHY_ERROR(("Unsupported, supported types are"));
        PHY_ERROR((" ACPHY_TEMPSENSE_VBE/ACPHY_TEMPSENSE_VBG\n"));
        return BCME_ERROR;
    }
    return BCME_OK;
}

static int32
wlc_phy_tempsense_paldosense_phy_cleanup_acphy_tiny(phy_info_t *pi, uint8 core)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    acphy_tempsense_phyregs_t *porig = (pi_ac->tempi->ac_tempsense_phyregs_orig);

    // Additional settings for 4365 core3 GPIO WAR
    if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
        (core == 3)) {
        MOD_PHYREG(pi, DcFiltAddress,        dcBypass,     0);
        MOD_PHYREG(pi, RfctrlOverrideGains3, rxgain,       0);
        MOD_PHYREG(pi, RfctrlOverrideGains3, lpf_bq2_gain, 0);
    }

    /* # restore T/R and external PA states */
    WRITE_PHYREG(pi, RxFeCtrl1, porig->RxFeCtrl1);
    WRITE_PHYREG(pi, RxSdFeConfig6, porig->RxSdFeConfig6);
    WRITE_PHYREG(pi, RxSdFeConfig1, porig->RxSdFeConfig1);
    WRITE_PHYREGCE(pi, RfctrlIntc, core, porig->RfctrlIntc[core]);
    WRITE_PHYREGCE(pi, RfctrlOverrideAuxTssi, core, porig->RfctrlOverrideAuxTssi[core]);
    WRITE_PHYREGCE(pi, RfctrlCoreAuxTssi1, core, porig->RfctrlCoreAuxTssi1[core]);
    WRITE_PHYREGCE(pi, RfctrlOverrideRxPus, core, porig->RfctrlOverrideRxPus[core]);
    WRITE_PHYREGCE(pi, RfctrlCoreRxPus, core, porig->RfctrlCoreRxPus[core]);
    WRITE_PHYREGCE(pi, RfctrlOverrideTxPus, core, porig->RfctrlOverrideTxPus[core]);
    WRITE_PHYREGCE(pi, RfctrlCoreTxPus, core, porig->RfctrlCoreTxPus[core]);
    WRITE_PHYREGCE(pi, RfctrlOverrideLpfSwtch, core, porig->RfctrlOverrideLpfSwtch[core]);
    WRITE_PHYREGCE(pi, RfctrlCoreLpfSwtch, core, porig->RfctrlCoreLpfSwtch[core]);
    WRITE_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, porig->RfctrlOverrideAfeCfg[core]);
    WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, porig->RfctrlCoreAfeCfg1[core]);
    WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg2, core, porig->RfctrlCoreAfeCfg2[core]);
    WRITE_PHYREGCE(pi, RfctrlOverrideGains, core, porig->RfctrlOverrideGains[core]);
    WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core, porig->RfctrlCoreLpfGain[core]);
    return BCME_OK;

}

static int32
wlc_phy_tempsense_paldosense_radio_cleanup_acphy_tiny(phy_info_t *pi, uint8 tempsense_paldosense
, uint8 coreidx)
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    tempsense_radioregs_tiny_t *porig =
        &(pi_ac->tempi->ac_tempsense_radioregs_orig->u.acphy_tempsense_radioregs_tiny);
    uint8 core, cores;
    if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev))
        cores = PHYCORENUM((pi)->pubpi->phy_corenum);
    else
        cores = 1;

    for (core = 0; core < cores; core++) {
        if ((ACMAJORREV_32(pi->pubpi->phy_rev) ||
            ACMAJORREV_33(pi->pubpi->phy_rev)) && (core != coreidx))
            continue;
        /* # Get back old values */
        phy_utils_write_radioreg(pi, RADIO_REG(pi, TEMPSENSE_OVR1, core),
            porig->tempsense_ovr1[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, TEMPSENSE_CFG, core),
            porig->tempsense_cfg[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, TESTBUF_OVR1, core),
            porig->testbuf_ovr1[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, TESTBUF_CFG1, core),
            porig->testbuf_cfg1[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_OVR1, core),
            porig->auxpga_ovr1[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_CFG1, core),
            porig->auxpga_cfg1[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, AUXPGA_VMID, core),
            porig->auxpga_vmid[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, TIA_CFG9, core),
            porig->tia_cfg9[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, ADC_OVR1, core),
            porig->adc_ovr1[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, ADC_CFG10, core),
            porig->adc_cfg10[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, TIA_CFG5, core),
            porig->tia_cfg5[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, RX_BB_2G_OVR_EAST, core),
            porig->rx_bb_2g_ovr_east[core]);
        phy_utils_write_radioreg(pi, RADIO_REG(pi, TIA_CFG7, core),
            porig->tia_cfg7[core]);
        if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 1);
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0);
        } else {
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_CFG, core, tempsense_pu, 0);
            MOD_RADIO_REG_TINY(pi, TEMPSENSE_OVR1, core, ovr_tempsense_pu, 0);
            MOD_RADIO_REG_TINY(pi, VBAT_CFG, core, vbat_monitor_pu, 0);
            MOD_RADIO_REG_TINY(pi, VBAT_OVR1, core, ovr_vbat_monitor_pu, 0);
            MOD_RADIO_REG_TINY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 0);
            MOD_RADIO_REG_TINY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 0);
        }
        if (tempsense_paldosense) {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_tx2g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX2G_CFG1, core, tx2g_bias_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_tx2g_bias_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_pa2g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA2G_CFG1, core, pa2g_bias_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_pa2g_bias_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_pa2g_bias_cas_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA2G_IDAC2, core, pa2g_bias_cas_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_2G_OVR_EAST, core,
                    ovr_pa2g_bias_cas_pu, 0);
            } else {
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
                    ovr_tx5g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, TX5G_CFG1, core, tx5g_bias_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
                    ovr_tx5g_bias_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
                    ovr_pa5g_bias_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core, pa5g_bias_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
                    ovr_pa5g_bias_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR2, core,
                    ovr_pa5g_bias_cas_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA5G_CFG1, core,
                    pa5g_bias_cas_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR2, core,
                    ovr_pa5g_bias_cas_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
                    ovr_pa5g_pu, 1);
                MOD_RADIO_REG_TINY(pi, PA5G_CFG4, core, pa5g_pu, 0);
                MOD_RADIO_REG_TINY(pi, TX_TOP_5G_OVR1, core,
                    ovr_pa5g_pu, 0);
            }
        }

    }
    return BCME_OK;
}
static int16
wlc_phy_tempsense_paldosense_acphy_tiny(phy_info_t *pi, uint8 tempsense_paldosense)
{
    /*
     * # Description of mode = single:
     * #    - used to digitally poll the radio's temperature sensors
     * #    - does an absolute measurement by toggling the flip bit
     * #    - saves and restores previous register values
     * #    - returns per core values in degrees Celsius
     * #
     * # Description of mode = "scope": *** FIXME: not implemented ***
     * #    - used to monitor the radio's temperature sensor on the scope by
     * #      means of the analog pwrdet signal
     * #    - changes the state of the muxes and powers up the sensor
     * # --------------------------------------------------------------------
     */

    uint16 auxPGA_Av = 0x3, auxPGA_Vmid = 0x91;
    int64 radio_temp = 0;
    int32 t_scale = 16384;
    int32 t_slope = -10246;
    int32 t_offset = 1765868;
    int32 avbq_scale = 800;
    int32 avbq_slope = 1024;
    uint16 save_afePuCtrl = 0, save_gpio = 0, save_gpioHiOutEn = 0;
    uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
    uint32 save_chipc = 0;
    uint8  stall_val = 0;
    int32 measured_voltage[4] = {0};
    uint8 enRx = 0;
    uint8 enTx = 0;
    uint8 core = 0;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    ASSERT(TINY_RADIO(pi));

    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        avbq_scale = 1200;
        core = phy_ac_temp_get_first_actv_core(stf_shdata->phyrxchain);
        ASSERT(core < PHY_CORE_MAX);
        if (core < 3) {
            auxPGA_Av = 0x5, auxPGA_Vmid = 0x7D;
            t_slope = -3038; t_offset = 1621378;
        } else {
            auxPGA_Av = 0x5, auxPGA_Vmid = 0x9B;
            t_slope = -3795; t_offset = 1606261;
        }
    }

    /*
     * # If mac is suspended, leave it suspended and don't touch the state of the MAC
     * # If not, suspend at the beginning of tempsense and resume it at the end.
     * # (Suspending is required - as we arereading via muxes that are pin-contolled
     * # during normal RX & TX.)
     */
    /* Prepare Mac and Phregs */
    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    if (!ACMAJORREV_32(pi->pubpi->phy_rev) && !ACMAJORREV_33(pi->pubpi->phy_rev)) {
        /* change Av and Vmid for paldosense */
        if ((tempsense_paldosense)) {
            auxPGA_Av = 0x7;
            auxPGA_Vmid = 70;
        }
    }

    /* change slope/offset for 4355/4359 RSDB core 1 */
    if (!(ROUTER_4349(pi)) && (phy_get_phymode(pi) == PHYMODE_RSDB) &&
        (phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1)) {
        t_slope = -11765;
        t_offset = 1671660;
    }
    wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
        stf_shdata->hw_phyrxchain, stf_shdata->hw_phytxchain);
    wlc_phy_tempsense_paldosense_phy_setup_acphy_tiny(pi, tempsense_paldosense, core);
    wlc_phy_tempsense_paldosense_radio_setup_acphy_tiny(pi, auxPGA_Av, auxPGA_Vmid,
    tempsense_paldosense, core);

    wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
                              &save_chipc, &fval2g_orig, &fval5g_orig,
                              &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);
    wlc_phy_tempsense_poll_adc_war_tiny(pi, TRUE, measured_voltage, core);
    if (!(tempsense_paldosense)) {
        radio_temp += (int64)(((measured_voltage[0] + measured_voltage[2]
            - measured_voltage[1] - measured_voltage[3]) / 2)
            * (int64) t_slope * avbq_scale) / avbq_slope;
        radio_temp = (radio_temp + t_offset)/t_scale;
    }

    wlc_phy_tempsense_paldosense_phy_cleanup_acphy_tiny(pi, core);
    wlc_phy_tempsense_paldosense_radio_cleanup_acphy_tiny(pi, tempsense_paldosense, core);
    wlc_phy_restore_after_adc_read(pi,    &save_afePuCtrl, &save_gpio,
        &save_chipc,  &fval2g_orig,  &fval5g_orig,
        &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);
    wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);
    phy_utils_phyreg_exit(pi);

    /* # RESUME MAC as soon as we are done reading/writing regs and muxes
     * # -----------------------------------------------------------------
     */
    wlapi_enable_mac(pi->sh->physhim);

    if (!(tempsense_paldosense)) {
        /* Store temperature and return value */
        pi->u.pi_acphy->tempi->current_temperature = (int16) radio_temp;

    #ifdef ATE_BUILD
        wl_ate_set_buffer_regval(CURR_RADIO_TEMP, radio_temp, -1, phy_get_current_core(pi),
        pi->sh->chip);
    #endif

        return (pi->u.pi_acphy->tempi->current_temperature);
    } else {
        return ((int16)measured_voltage[3]);
    }
}

int16
wlc_phy_tempsense_vbatsense_acphy_20704(phy_info_t *pi, uint8 tempsense_vbatsense)
{
    const int32 t_scale = 16384;
    //const int32 t_slope[] = {4376, 4221, 4083, 4571};
    const int32 t_slope[] = {6189, 5969, 5774, 6464};
    const int32 t_offset[] = {1842100, 1859900, 1830430, 1838500};
    int16 gpio_clk_en;
    int16 measured_voltage[4] = {0};
    int32 radio_temp = 0;
    uint8 core;
    int16 offset = (int16) pi->phy_tempsense_offset;
    uint16 auxPGA_Av = 0, auxPGA_Vmid = 140;
    uint16 save_afePuCtrl, save_gpio, save_gpioHiOutEn;
    uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
    uint32 save_chipc;
    uint8  stall_val;
    phy_ac_temp_info_t *ti = pi->u.pi_acphy->tempi;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 hwobss_en;

    core = phy_ac_temp_get_first_actv_core(stf_shdata->phyrxchain);

    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    /* Prepare Mac and Phy regs */
    gpio_clk_en = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
    MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);
    wlc_phy_tempsense_phy_setup_acphy(pi, core);
    wlc_phy_tempsense_radio_setup_acphy_20704(pi, auxPGA_Av, auxPGA_Vmid, core);

    wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
            &save_chipc, &fval2g_orig, &fval5g_orig,
            &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);

    /* Need to disable HWOBSS when doing voltage measurement */
    hwobss_en = READ_PHYREGFLD(pi, obss_control, obss_mit_en);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, FALSE);
    wlc_phy_tempsense_poll_adc_war_20704(pi, TRUE, measured_voltage, core);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, (hwobss_en != 0));

    radio_temp += (int32)(((measured_voltage[0] + measured_voltage[2]
        - measured_voltage[1] - measured_voltage[3]) / 2)) * t_slope[core];

    radio_temp = (radio_temp + t_offset[core]) / t_scale;

    PHY_THERMAL(("phy_tempsense::measured_voltage[core%d]=[%d,%d,%d,%d], temp=%d C\n",
            core, measured_voltage[0],
            measured_voltage[1], measured_voltage[2],
            measured_voltage[3], (int) radio_temp));

    /* restore registers and resume MAC */
    wlc_phy_restore_after_adc_read(pi, &save_afePuCtrl, &save_gpio,
        &save_chipc,  &fval2g_orig,  &fval5g_orig,
        &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

    wlc_phy_tempsense_phy_cleanup_acphy(pi, core);
    wlc_phy_tempsense_radio_cleanup_acphy_cacherestore(pi, core);

    MOD_PHYREG(pi, gpioClkControl, gpioEn, gpio_clk_en);

    phy_utils_phyreg_exit(pi);
    wlapi_enable_mac(pi->sh->physhim);

    /* Store temperature and return value */
    ti->current_temperature = (int16) radio_temp + offset;

#ifdef ATE_BUILD
    wl_ate_set_buffer_regval(CURR_RADIO_TEMP, pi->u.pi_acphy->tempi->current_temperature, -1,
        phy_get_current_core(pi), pi->sh->chip);
#endif

    return ((int16) radio_temp + offset);
}

int16
wlc_phy_tempsense_vbatsense_acphy_20707(phy_info_t *pi, uint8 tempsense_vbatsense)
{
    const int32 t_scale = 16384;
    //const int32 t_slope[] = {4376, 4221, 4083, 4571};
    const int32 t_slope[] = {6189, 5969, 5774, 6464};
    const int32 t_offset[] = {1842100, 1859900, 1830430, 1838500};
    int16 gpio_clk_en;
    int16 measured_voltage[4] = {0};
    int32 radio_temp = 0;
    uint8 core;
    int16 offset = (int16) pi->phy_tempsense_offset;
    uint16 auxPGA_Av = 0, auxPGA_Vmid = 140;
    uint16 save_afePuCtrl, save_gpio, save_gpioHiOutEn;
    uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
    uint32 save_chipc;
    uint8  stall_val;
    phy_ac_temp_info_t *ti = pi->u.pi_acphy->tempi;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 hwobss_en;

    core = phy_ac_temp_get_first_actv_core(stf_shdata->phyrxchain);

    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    /* Prepare Mac and Phy regs */
    gpio_clk_en = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
    MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);
    wlc_phy_tempsense_phy_setup_acphy(pi, core);
    wlc_phy_tempsense_radio_setup_acphy_20707(pi, auxPGA_Av, auxPGA_Vmid, core);

    wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
            &save_chipc, &fval2g_orig, &fval5g_orig,
            &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);

    /* Need to disable HWOBSS when doing voltage measured */
    hwobss_en = READ_PHYREGFLD(pi, obss_control, obss_mit_en);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, FALSE);
    wlc_phy_tempsense_poll_adc_war_20707(pi, TRUE, measured_voltage, core);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, (hwobss_en != 0));

    radio_temp += (int32)(((measured_voltage[0] + measured_voltage[2]
        - measured_voltage[1] - measured_voltage[3]) / 2)) * t_slope[core];

    radio_temp = (radio_temp + t_offset[core]) / t_scale;

    PHY_THERMAL(("phy_tempsense::measured_voltage[core%d]=[%d,%d,%d,%d], temp=%d C\n",
            core, measured_voltage[0],
            measured_voltage[1], measured_voltage[2],
            measured_voltage[3], (int) radio_temp));

    /* restore registers and resume MAC */
    wlc_phy_restore_after_adc_read(pi, &save_afePuCtrl, &save_gpio,
        &save_chipc,  &fval2g_orig,  &fval5g_orig,
        &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

    wlc_phy_tempsense_phy_cleanup_acphy(pi, core);
    wlc_phy_tempsense_radio_cleanup_acphy_cacherestore(pi, core);

    MOD_PHYREG(pi, gpioClkControl, gpioEn, gpio_clk_en);

    phy_utils_phyreg_exit(pi);
    wlapi_enable_mac(pi->sh->physhim);

    /* Store temperature and return value */
    ti->current_temperature = (int16) radio_temp + offset;

#ifdef ATE_BUILD
    wl_ate_set_buffer_regval(CURR_RADIO_TEMP, pi->u.pi_acphy->tempi->current_temperature, -1,
        phy_get_current_core(pi), pi->sh->chip);
#endif

    return ((int16) radio_temp + offset);
}

int16
wlc_phy_tempsense_vbatsense_acphy_20708(phy_info_t *pi, uint8 tempsense_vbatsense)
{

    phy_ac_temp_info_t *ti = pi->u.pi_acphy->tempi;

    const int32 t_scale = 16300;
    const int32 t_slope[] = {5374, 5415, 5227, 5439};
    const int32 t_offset[] = {1002242, 1010046, 1009433, 1000485};
    int16 gpio_clk_en;
    int16 measured_voltage[4] = {0};
    int32 radio_temp = 0;
    int32 tempsense_lowerbound = -100, tempsense_upperbound = 150;
    uint8 core;
    int16 offset = (int16) pi->phy_tempsense_offset;
    uint16 auxPGA_Av = 0, auxPGA_Vmid = 190;
    uint16 save_afePuCtrl, save_gpio, save_gpioHiOutEn;
    uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
    uint32 save_chipc;
    uint8  stall_val;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 hwobss_en;
    uint16 classifier_state = 0;

    core = phy_ac_temp_get_first_actv_core(stf_shdata->phyrxchain);

    /* Turn off classifiers to avoid iq_est corruption */
    classifier_state = READ_PHYREG(pi, ClassifierCtrl);
    MOD_PHYREG(pi, ClassifierCtrl, classifierSel1, 0);
    MOD_PHYREG(pi, ClassifierCtrl, classifierSel0, 0);

    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    /* Prepare Mac and Phy regs */
    gpio_clk_en = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
    MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);
    wlc_phy_tempsense_phy_setup_acphy(pi, core);
    wlc_phy_tempsense_radio_setup_acphy_20708(pi, auxPGA_Av, auxPGA_Vmid, core);

    wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
            &save_chipc, &fval2g_orig, &fval5g_orig,
            &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);

    /* Need to disable HWOBSS when doing voltage measured */
    hwobss_en = READ_PHYREGFLD(pi, obss_control, obss_mit_en);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, FALSE);
    wlc_phy_tempsense_poll_adc_war_20708(pi, TRUE, measured_voltage, core);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, (hwobss_en != 0));

    radio_temp += (int32)(((measured_voltage[0] + measured_voltage[2]
        - measured_voltage[1] - measured_voltage[3]) / 2)) * t_slope[core];

    radio_temp = (radio_temp + t_offset[core]) / t_scale;

    /* For 6715, also introduce a 5deg offset/fudging factor to improve accuracy.
     * The choice of 5C is based on the analysis of: phy_tempsense, case temperature
     * measurement and PVTMON reporting
     */
    radio_temp += 5;

    PHY_THERMAL(("phy_tempsense::measured_voltage[core%d]=[%d,%d,%d,%d], temp=%d C\n",
            core, measured_voltage[0],
            measured_voltage[1], measured_voltage[2],
            measured_voltage[3], (int) radio_temp));

    if ((tempsense_upperbound < radio_temp) || (tempsense_lowerbound > radio_temp)) {
        PHY_ERROR(("%s: Temperature overshooting is happening!\n", __FUNCTION__));
    }

    /* restore registers and resume MAC */
    wlc_phy_restore_after_adc_read(pi, &save_afePuCtrl, &save_gpio,
        &save_chipc,  &fval2g_orig,  &fval5g_orig,
        &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

    wlc_phy_tempsense_phy_cleanup_acphy(pi, core);
    wlc_phy_tempsense_radio_cleanup_acphy_cacherestore(pi, core);

    MOD_PHYREG(pi, gpioClkControl, gpioEn, gpio_clk_en);
    WRITE_PHYREG(pi, ClassifierCtrl, classifier_state);

    phy_utils_phyreg_exit(pi);
    wlapi_enable_mac(pi->sh->physhim);

    /* Store temperature and return value */
    ti->current_temperature = (int16) radio_temp + offset;

#ifdef ATE_BUILD
    wl_ate_set_buffer_regval(CURR_RADIO_TEMP, pi->u.pi_acphy->tempi->current_temperature, -1,
        phy_get_current_core(pi), pi->sh->chip);
#endif

    return ((int16) radio_temp + offset);
}

int16
wlc_phy_tempsense_acphy_20709(phy_info_t *pi)
{
    int16 gpio_clk_en;
    int32 radio_temp = 0;
    uint8 core;
    int16 offset = (int16) pi->phy_tempsense_offset;
    uint16 auxPGA_Av = 0, auxPGA_Vmid = 140;
    uint16 save_afePuCtrl, save_gpio, save_gpioHiOutEn;
    uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
    uint32 save_chipc;
    uint8  stall_val;
    phy_ac_temp_info_t *ti = pi->u.pi_acphy->tempi;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 hwobss_en;

    core = phy_ac_temp_get_first_actv_core(stf_shdata->phyrxchain);

    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    /* Prepare Mac and Phy regs */
    gpio_clk_en = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
    MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);
    wlc_phy_tempsense_phy_setup_acphy(pi, core);
    wlc_phy_tempsense_radio_setup_acphy_20709(pi, auxPGA_Av, auxPGA_Vmid, core);

    wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
            &save_chipc, &fval2g_orig, &fval5g_orig,
            &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);

    /* Need to disable HWOBSS when doing voltage measurement */
    hwobss_en = READ_PHYREGFLD(pi, obss_control, obss_mit_en);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, FALSE);
    wlc_phy_tempsense_poll_adc_20709(pi, TRUE, &radio_temp, core);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, (hwobss_en != 0));

    /* Restore registers */
    wlc_phy_restore_after_adc_read(pi, &save_afePuCtrl, &save_gpio,
        &save_chipc,  &fval2g_orig,  &fval5g_orig,
        &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

    wlc_phy_tempsense_phy_cleanup_acphy(pi, core);
    wlc_phy_tempsense_radio_cleanup_acphy_cacherestore(pi, core);

    MOD_PHYREG(pi, gpioClkControl, gpioEn, gpio_clk_en);

    /* Resume MAC */
    phy_utils_phyreg_exit(pi);
    wlapi_enable_mac(pi->sh->physhim);

    /* Store temperature and return value */
    ti->current_temperature = (int16) radio_temp + offset;

#ifdef ATE_BUILD
    wl_ate_set_buffer_regval(CURR_RADIO_TEMP, pi->u.pi_acphy->tempi->current_temperature, -1,
        phy_get_current_core(pi), pi->sh->chip);
#endif

    return ((int16) radio_temp + offset);
}

int16
wlc_phy_tempsense_acphy_20710(phy_info_t *pi)
{
    int16 gpio_clk_en;
    int32 radio_temp = 0;
    uint8 core;
    int16 offset = (int16) pi->phy_tempsense_offset;
    uint16 auxPGA_Av = 0, auxPGA_Vmid = 140;
    uint16 save_afePuCtrl, save_gpio, save_gpioHiOutEn;
    uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
    uint32 save_chipc;
    uint8  stall_val;
    phy_ac_temp_info_t *ti = pi->u.pi_acphy->tempi;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 hwobss_en;

    core = phy_ac_temp_get_first_actv_core(stf_shdata->phyrxchain);

    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    /* Prepare Mac and Phy regs */
    gpio_clk_en = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
    MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);
    wlc_phy_tempsense_phy_setup_acphy(pi, core);
    wlc_phy_tempsense_radio_setup_acphy_20710(pi, auxPGA_Av, auxPGA_Vmid, core);

    wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
            &save_chipc, &fval2g_orig, &fval5g_orig,
            &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);

    /* Need to disable HWOBSS when doing voltage measurement */
    hwobss_en = READ_PHYREGFLD(pi, obss_control, obss_mit_en);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, FALSE);
    wlc_phy_tempsense_poll_adc_20710(pi, TRUE, &radio_temp, core);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, (hwobss_en != 0));

    /* Restore registers */
    wlc_phy_restore_after_adc_read(pi, &save_afePuCtrl, &save_gpio,
        &save_chipc,  &fval2g_orig,  &fval5g_orig,
        &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

    wlc_phy_tempsense_phy_cleanup_acphy(pi, core);
    wlc_phy_tempsense_radio_cleanup_acphy_cacherestore(pi, core);

    MOD_PHYREG(pi, gpioClkControl, gpioEn, gpio_clk_en);

    /* Resume MAC */
    phy_utils_phyreg_exit(pi);
    wlapi_enable_mac(pi->sh->physhim);

    /* Store temperature and return value */
    ti->current_temperature = (int16) radio_temp + offset;

#ifdef ATE_BUILD
    wl_ate_set_buffer_regval(CURR_RADIO_TEMP, pi->u.pi_acphy->tempi->current_temperature, -1,
        phy_get_current_core(pi), pi->sh->chip);
#endif

    return ((int16) radio_temp + offset);
}

int16
wlc_phy_tempsense_vbatsense_acphy_20698(phy_info_t *pi, uint8 tempsense_vbatsense)
{
    const int32 t_scale = 16384;
    //const int32 t_slope[] = {4376, 4221, 4083, 4571};
    const int32 t_slope[] = {6189, 5969, 5774, 6464};
    const int32 t_offset[] = {1842100, 1859900, 1830430, 1838500};
    int16 gpio_clk_en;
    int16 measured_voltage[4] = {0};
    int32 radio_temp = 0;
    uint8 core;
    int16 offset = (int16) pi->phy_tempsense_offset;
    uint16 auxPGA_Av = 0, auxPGA_Vmid = 140;
    uint16 save_afePuCtrl, save_gpio, save_gpioHiOutEn;
    uint16 fval2g_orig, fval5g_orig, fval2g, fval5g;
    uint32 save_chipc;
    uint8  stall_val;
    phy_ac_temp_info_t *ti = pi->u.pi_acphy->tempi;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 hwobss_en;

    core = phy_ac_temp_get_first_actv_core(stf_shdata->phyrxchain);

    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    /* Prepare Mac and Phy regs */
    gpio_clk_en = READ_PHYREGFLD(pi, gpioClkControl, gpioEn);
    MOD_PHYREG(pi, gpioClkControl, gpioEn, 1);
    wlc_phy_tempsense_phy_setup_acphy(pi, core);
    wlc_phy_tempsense_radio_setup_acphy_20698(pi, auxPGA_Av, auxPGA_Vmid, core);

    //phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

    wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
            &save_chipc, &fval2g_orig, &fval5g_orig,
            &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);

    /* FIXME43684: need to skip on DFS_CORE and on NO_RADAR_CHAN */
    /* Need to disable HWOBSS when doing voltage measured */
    hwobss_en = READ_PHYREGFLD(pi, obss_control, obss_mit_en);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, FALSE);
    wlc_phy_tempsense_poll_adc_war_20698(pi, TRUE, measured_voltage, core);
    phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, (hwobss_en != 0));
    radio_temp += (int32)(((measured_voltage[0] + measured_voltage[2]
        - measured_voltage[1] - measured_voltage[3]) / 2)) * t_slope[core];

    radio_temp = (radio_temp + t_offset[core]) / t_scale;

    PHY_THERMAL(("phy_tempsense::measured_voltage[core%d]=[%d,%d,%d,%d], temp=%d C\n",
            core, measured_voltage[0],
            measured_voltage[1], measured_voltage[2],
            measured_voltage[3], (int) radio_temp));

    /* restore registers and resume MAC */
    wlc_phy_restore_after_adc_read(pi, &save_afePuCtrl, &save_gpio,
        &save_chipc,  &fval2g_orig,  &fval5g_orig,
        &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);

    //phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
    wlc_phy_tempsense_phy_cleanup_acphy(pi, core);
    wlc_phy_tempsense_radio_cleanup_acphy_cacherestore(pi, core);

    MOD_PHYREG(pi, gpioClkControl, gpioEn, gpio_clk_en);

    phy_utils_phyreg_exit(pi);
    //wlc_phy_resetcca_acphy(pi);
    wlapi_enable_mac(pi->sh->physhim);

    /* Store temperature and return value */
    ti->current_temperature = (int16) radio_temp + offset;

#ifdef ATE_BUILD
    wl_ate_set_buffer_regval(CURR_RADIO_TEMP, pi->u.pi_acphy->tempi->current_temperature, -1,
        phy_get_current_core(pi), pi->sh->chip);
#endif

    return ((int16) radio_temp + offset);
}

#define NUM_VOLTAGE_SAMPLES 4

int16
wlc_phy_tempsense_acphy(phy_info_t *pi)
{
    uint8 core;
    uint8 sel_Vb, swap_amp;
    int8 idx;
    uint16 auxPGA_Av = 0x3, auxPGA_Vmid = 0x91;
    int16 V[NUM_VOLTAGE_SAMPLES][PHY_CORE_MAX];
    int16 Vout[NUM_VOLTAGE_SAMPLES] = {0, 0, 0, 0};
    int16 offset = (int16) pi->phy_tempsense_offset;
    int32 radio_temp = 0;
    int32 t_scale = 16384;
    int32 t_slope, t_offset;
    int32 avbq_scale = 256;

    /* TODO: Extend to 4th core. */
    int32 avbq_slope[4] = {527, 521, 522, 0};

    uint16 save_afePuCtrl = 0, save_gpio = 0, save_gpioHiOutEn = 0;
    uint16 fval2g_orig, fval5g_orig, fval2g, fval5g, temp_val;
    uint32 save_chipc = 0;
    uint8  stall_val = 0, log2_nsamps = 3;
    int32 k, tmp_samp, samp_accum;
    phy_ac_info_t *pi_ac = pi->u.pi_acphy;
    phy_ac_temp_info_t *ti = pi_ac->tempi;
    phy_txcore_temp_t *temp = phy_temp_get_st(pi->tempi);
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 sel_Vb_swap_amp_lut[NUM_VOLTAGE_SAMPLES][2] = {
        {1, 0},
        {0, 0},
        {1, 1},
        {0, 1}
    };
    uint i = 0;

    BCM_REFERENCE(stf_shdata);

    /* Initialize the array to zeros. */
    bzero(V, sizeof(V));

    if (temp->skip_tempsense) return 42;

    /* return dummy temp */
    if (NORADIO_ENAB(pi->pubpi)) {
        ti->current_temperature = 25;
        return (ti->current_temperature);
    }

    if (TINY_RADIO(pi)) {
        return wlc_phy_tempsense_paldosense_acphy_tiny(pi, SENSE_TEMP);
    } else if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
        return wlc_phy_tempsense_vbatsense_acphy_20698(pi, SENSE_TEMP);
    } else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
        return wlc_phy_tempsense_vbatsense_acphy_20704(pi, SENSE_TEMP);
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        return wlc_phy_tempsense_acphy_20709(pi);
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        return wlc_phy_tempsense_vbatsense_acphy_20707(pi, SENSE_TEMP);
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        temp_val = wlc_phy_tempsense_vbatsense_acphy_20708(pi, SENSE_TEMP);
        return temp_val;
    } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
        return wlc_phy_tempsense_acphy_20710(pi);
    }

    /* Prepare Mac and Phregs */
    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    if (IS_4364_3x3(pi)) {
        t_offset = 1860613;
        avbq_slope[0] = 518;
        avbq_slope[1] = 521;
        avbq_slope[2] = 537;
    }

    wlc_phy_tempsense_phy_setup_acphy(pi, 0);
    wlc_phy_tempsense_radio_setup_acphy(pi, auxPGA_Av, auxPGA_Vmid);
    wlc_phy_init_adc_read(pi, &save_afePuCtrl, &save_gpio,
        &save_chipc, &fval2g_orig, &fval5g_orig,
        &fval2g, &fval5g, &stall_val, &save_gpioHiOutEn);
    wlc_phy_pulse_adc_reset_acphy(pi);

    FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
        wlc_phy_gpiosel_acphy(pi, 16+core, 1);
        for (idx = 0; idx < NUM_VOLTAGE_SAMPLES; idx++) {

            /* Lookup the value from the LUT. */
            sel_Vb = sel_Vb_swap_amp_lut[idx][0];
            swap_amp = sel_Vb_swap_amp_lut[idx][1];

            /* Reg conflict with 2069 rev 16 */
            if (RADIOMAJORREV(pi) == 0) {
                MOD_RADIO_REGC(pi, OVR18, core, ovr_tempsense_sel_Vbe_Vbg, 0x1);
            } else {
                MOD_RADIO_REGC(pi, GE16_OVR19,
                    core, ovr_tempsense_sel_Vbe_Vbg, 0x1);
            }
            MOD_RADIO_REGC(pi, TEMPSENSE_CFG, core, tempsense_sel_Vbe_Vbg, sel_Vb);

            /* Reg conflict with 2069 rev 16 */
            if (RADIOMAJORREV(pi) == 0) {
                MOD_RADIO_REGC(pi, OVR18, core, ovr_tempsense_swap_amp, 0x1);
            } else {
                MOD_RADIO_REGC(pi, GE16_OVR19, core, ovr_tempsense_swap_amp, 0x1);
            }
            MOD_RADIO_REGC(pi, TEMPSENSE_CFG, core, swap_amp, swap_amp);

            OSL_DELAY(10);
            samp_accum = 0;
            for (k = 0; k < (1 << log2_nsamps); k++) {
                /* read out the i-value */
                tmp_samp = READ_PHYREG(pi, gpioHiOut) >> 2;
                tmp_samp -= (tmp_samp < 512) ? 0 : 1024;
                samp_accum += tmp_samp;
            }
            V[idx][core] = (int16) (samp_accum >> log2_nsamps);
        }
    }

    if (RADIOMAJORREV(pi) == 1) {
        t_slope = -5453;
        t_offset = 1748881;
        i = 0;
    } else if (RADIOMAJORREV(pi) == 2) {
        t_slope = -5549;
        t_offset = 1697118;
        i = 0;
    } else {
        t_slope = 8766;
        t_offset = 1902100;
        i = 1;
    }

    if ((RADIOMAJORREV(pi) == 1) || (RADIOMAJORREV(pi) == 2)) {
        avbq_scale = 800;
        avbq_slope[0] = 1024;
        avbq_slope[1] = 1024;
    }

    /* TODO: Extend this for the 4th core. */
    ASSERT(PHYCORENUM((pi)->pubpi->phy_corenum) < 4);

    FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
        radio_temp += (int32)(((V[0+i][core] + V[2+i][core]
            - V[1-i][core] - V[3-i][core]) / 2))
            * ((t_slope * avbq_scale) / (avbq_slope[core]
            * PHYCORENUM((pi)->pubpi->phy_corenum)));
        Vout[0] += V[0][core];
        Vout[1] += V[1][core];
        Vout[2] += V[2][core];
        Vout[3] += V[3][core];
    }
    Vout[0] /= PHYCORENUM((pi)->pubpi->phy_corenum);
    Vout[1] /= PHYCORENUM((pi)->pubpi->phy_corenum);
    Vout[2] /= PHYCORENUM((pi)->pubpi->phy_corenum);
    Vout[3] /= PHYCORENUM((pi)->pubpi->phy_corenum);

    radio_temp = (radio_temp + t_offset)/t_scale;

    PHY_THERMAL(("Tempsense\n\tAuxADC0 Av,Vmid = 0x%x,0x%x\n",
                 auxPGA_Av, auxPGA_Vmid));
    PHY_THERMAL(("\tVref1,Vref2,Vctat1,Vctat2 =%d,%d,%d,%d\n",
                 Vout[0], Vout[2], Vout[1], Vout[3]));
    PHY_THERMAL(("\t^C Formula: (%d*(Vctat1+Vctat2-Vref1-Vref2)/2*800/1024+%d)/%d\n",
                 t_slope, t_offset, t_scale));
    PHY_THERMAL(("\t^C = %d, applied offset = %d\n",
                 radio_temp, offset));

    wlc_phy_tempsense_phy_cleanup_acphy(pi, 0);
    wlc_phy_tempsense_radio_cleanup_acphy(pi);
    wlc_phy_restore_after_adc_read(pi,  &save_afePuCtrl, &save_gpio,
        &save_chipc,  &fval2g_orig,  &fval5g_orig,
        &fval2g,  &fval5g, &stall_val, &save_gpioHiOutEn);
    phy_utils_phyreg_exit(pi);
    wlapi_enable_mac(pi->sh->physhim);

    /* Store temperature and return value */
    ti->current_temperature = (int16) radio_temp + offset;

#ifdef ATE_BUILD
    wl_ate_set_buffer_regval(CURR_RADIO_TEMP, pi->u.pi_acphy->tempi->current_temperature, -1,
        phy_get_current_core(pi), pi->sh->chip);
#endif
    return (ti->current_temperature);
}

void
phy_ac_update_tempsense_bitmap(phy_info_t *pi)
{
    phy_info_t *other_pi = phy_get_other_pi(pi);
    phy_temp_info_t *tempi = pi->tempi;
    phy_txcore_temp_t *temp_status = phy_temp_get_st(tempi);

    temp_status->bitmap = pi->pubpi->phy_coremask;
    temp_status->bitmap |= (temp_status->bitmap << 4);

    if (phy_get_phymode(pi) == PHYMODE_RSDB) {
        tempi = other_pi->tempi;
        temp_status = phy_temp_get_st(tempi);
        temp_status->bitmap = other_pi->pubpi->phy_coremask;
        temp_status->bitmap |= (temp_status->bitmap << 4);
    }
}

uint8
wlc_phy_vbat_monitoring_algorithm_acphy(phy_info_t *pi)
{
    uint8 i;
    uint8 do_cals = 1;
    /* Adding PALDO voltage level of 3.4 to the list */
    const uint8 paldo_data[PALDO_VOLTATE_MAPPING_ARRAY_SIZE] = {
        PALDO_VOLTAGE_CODE_FOR_3P4V,
        PALDO_VOLTAGE_CODE_FOR_3P3V,
        PALDO_VOLTAGE_CODE_FOR_3P2V,
        PALDO_VOLTAGE_CODE_FOR_3P1V,
        PALDO_VOLTAGE_CODE_FOR_3P0V,
        PALDO_VOLTAGE_CODE_FOR_2P9V
    };

    uint32 temp;
    int16 threshold = 12;
    int16 read_high_voltage;
    int16 read_new_voltage;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_temp_info_t *ti = pi_ac->tempi;

    /* setting the address for LDO and writing the 3p3 voltage */
    phy_utils_pmu_regcontrol_access(pi, 4, &temp, 0);
    temp = temp & 0xFFFF8FFF;
    temp = temp | (paldo_data[0] << 12);
    phy_utils_pmu_regcontrol_access(pi, 4, &temp, 1);
    /* setting the address for PALDO and writing the 3p3 voltage */
    phy_utils_pmu_regcontrol_access(pi, 7, &temp, 0);
    temp = temp & 0xFFFFFFF8;
    temp = temp | paldo_data[0];
    phy_utils_pmu_regcontrol_access(pi, 7, &temp, 1);
    read_high_voltage = wlc_phy_tempsense_paldosense_acphy_tiny(pi, SENSE_PALDO);
    /* loop over to check till what point regulated conditon is seen */
    for (i = 1; i < PALDO_VOLTATE_MAPPING_ARRAY_SIZE; i++) {
        /* setting the address for LDO and writing the 3p3 voltage */
        phy_utils_pmu_regcontrol_access(pi, 4, &temp, 0);
        temp = temp & 0xFFFF8FFF;
        temp = temp | (paldo_data[i] << 12);
        phy_utils_pmu_regcontrol_access(pi, 4, &temp, 1);
        /* setting the address for PALDO and writing the 3p3 voltage */
        phy_utils_pmu_regcontrol_access(pi, 7, &temp, 0);
        temp = temp & 0xFFFFFFF8;
        temp = temp | paldo_data[i];
        phy_utils_pmu_regcontrol_access(pi, 7, &temp, 1);
        read_new_voltage = wlc_phy_tempsense_paldosense_acphy_tiny(pi, SENSE_PALDO);
        if ((read_high_voltage - read_new_voltage) < threshold) {
            read_high_voltage = read_new_voltage;
        } else {
            break;
        }
    }
    pi_ac->tempi->vbat_codeidx = i-1; /* store the index value */
    if (i != (PALDO_VOLTATE_MAPPING_ARRAY_SIZE + 1)) {
        /* setting the address for LDO and writing the 3p3 voltage */
        phy_utils_pmu_regcontrol_access(pi, 4, &temp, 0);
        temp = temp & 0xFFFF8FFF;
        temp = temp | (paldo_data[i-1] << 12);
        phy_utils_pmu_regcontrol_access(pi, 4, &temp, 1);
        /* setting the address for PALDO and writing the 3p3 voltage */
        phy_utils_pmu_regcontrol_access(pi, 7, &temp, 0);
        temp = temp & 0xFFFFFFF8;
        temp = temp | paldo_data[i-1];
        phy_utils_pmu_regcontrol_access(pi, 7, &temp, 1);
    } else {
        /* setting the address for LDO and writing the 3p3 voltage */
        phy_utils_pmu_regcontrol_access(pi, 4, &temp, 0);
        temp = temp & 0xFFFF8FFF;
        temp = temp | (paldo_data[i-2] << 12);
        phy_utils_pmu_regcontrol_access(pi, 4, &temp, 1);
        /* setting the address for PALDO and writing the 3p3 voltage */
        phy_utils_pmu_regcontrol_access(pi, 7, &temp, 0);
        temp = temp & 0xFFFFFFF8;
        temp = temp | paldo_data[i-2];
        phy_utils_pmu_regcontrol_access(pi, 7, &temp, 1);
    }
    if (i == ti->last_regulated)
        do_cals = 0;
    ti->last_regulated = i;
    return ((uint8)do_cals);
}

void
phy_ac_update_dutycycle_throttle_state(phy_info_t *pi)
{
    phy_info_t *other_pi = phy_get_other_pi(pi);
    phy_temp_info_t *tempi = pi->tempi;
    phy_txcore_temp_t *temp_status = phy_temp_get_st(tempi);
    temp_status->duty_cycle = 100;
    temp_status->duty_cycle_throttle_state = 0;
    temp_status->duty_cycle_throttle_depth = 10;
    if (phy_get_phymode(pi) == PHYMODE_RSDB) {
        tempi = other_pi->tempi;
        temp_status = phy_temp_get_st(tempi);
        temp_status->duty_cycle = 100;
        temp_status->duty_cycle_throttle_state = 0;
        temp_status->duty_cycle_throttle_depth = 10;
    }
}

static void
phy_ac_temp_upd_gain(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj)
{
    phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
    phy_info_t *pi = info->pi;

    /* read in the temperature */
    int16 temp_diff, curr_temp = 0, gain_temp_slope = 0;
    curr_temp = info->current_temperature;
    curr_temp = MIN(MAX(curr_temp, PHY_TEMPSENSE_MIN), PHY_TEMPSENSE_MAX);

    /* check that non programmed SROM for cal temp are not changed */
    if (pi->srom_rawtempsense != 255) {
        temp_diff = curr_temp - pi->srom_rawtempsense;
    } else {
        temp_diff = 0;
    }

    /* adjust gain based on the temperature difference now vs. calibration time:
     * make gain diff rounded to nearest 0.25 dbm, where 1 tick is 0.25 dbm
     */
    gain_temp_slope = CHSPEC_IS2G(pi->radio_chanspec) ?
            ACPHY_GAIN_VS_TEMP_SLOPE_2G : ACPHY_GAIN_VS_TEMP_SLOPE_5G;

    if (temp_diff >= 0) {
        *gain_err_temp_adj = (temp_diff * gain_temp_slope*2 + 25)/50;
    } else {
        *gain_err_temp_adj = (temp_diff * gain_temp_slope*2 - 25)/50;
    }
}

void
phy_ac_temp_upd_gain_cal(phy_type_temp_ctx_t *ctx, int16 *gain_err_temp_adj)
{
    phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
    phy_info_t *pi = info->pi;

    /* read in the temperature */
    int16 temp_diff, curr_temp = 0, gain_temp_slope = 0;
    curr_temp = info->current_temperature;
    curr_temp = MIN(MAX(curr_temp, PHY_TEMPSENSE_MIN), PHY_TEMPSENSE_MAX);

    /* check that non programmed SROM for cal temp are not changed */
    if (pi->srom_gain_cal_temp != 255) {
        temp_diff = curr_temp - pi->srom_gain_cal_temp;
    } else {
        temp_diff = 0;
    }

    /* adjust gain based on the temperature difference now vs. calibration time:
    * make gain diff rounded to nearest 0.25 dbm, where 1 tick is 0.25 dbm
    */
    gain_temp_slope = CHSPEC_IS2G(pi->radio_chanspec) ?
        ACPHY_GAIN_VS_TEMP_SLOPE_2G : ACPHY_GAIN_VS_TEMP_SLOPE_5G;

    if (temp_diff >= 0) {
        *gain_err_temp_adj = (temp_diff * gain_temp_slope*2 + 25)/50;
    } else {
        *gain_err_temp_adj = (temp_diff * gain_temp_slope*2 - 25)/50;
    }
}

/* measure temperature */
static int16
phy_ac_temp_do_tempsense(phy_type_temp_ctx_t *ctx)
{
    phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
    phy_info_t *pi = info->pi;

    return wlc_phy_tempsense_acphy(pi);
}

#if defined(TXPWRBACKOFF) || defined(WL_TVPM)
/* return the last measured:vbat */
static uint8
phy_ac_vbat_get(phy_type_temp_ctx_t *ctx)
{
    phy_ac_temp_info_t *tempi = (phy_ac_temp_info_t *)ctx;
    return tempi->current_vbat;
}

static uint8
phy_ac_vbat_sense(phy_type_temp_ctx_t *ctx)
{

    /* PALDO voltage mapping array */
    const uint8 paldo_voltage[PALDO_VOLTATE_MAPPING_ARRAY_SIZE] = {
        PALDO_VOLTAGE_3P4,
        PALDO_VOLTAGE_3P3,
        PALDO_VOLTAGE_3P2,
        PALDO_VOLTAGE_3P1,
        PALDO_VOLTAGE_3P0,
        PALDO_VOLTAGE_2P9
    };

    phy_ac_temp_info_t *info = (phy_ac_temp_info_t *)ctx;
    phy_info_t *pi = info->pi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_temp_info_t *ti = pi_ac->tempi;

    if (TINY_RADIO(pi)) {
        wlc_phy_vbat_monitoring_algorithm_acphy(pi);
    } else {
        ASSERT(0);
    }

    /* Estimate Vbat by considering PALDO vltg + headroom */
    /* when LDO is in regulation */
    if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        uint8 paldo_idx = PALDO_VOLTATE_MAPPING_ARRAY_SIZE -
            (pi_ac->tempi->vbat_codeidx + 1)/2;
        paldo_idx = MIN(paldo_idx, (PALDO_VOLTATE_MAPPING_ARRAY_SIZE - 1));
        ti->current_vbat = paldo_voltage[paldo_idx];
    } else {
        ti->current_vbat = (paldo_voltage[pi_ac->tempi->vbat_codeidx]
            + PALDO_VOLTAGE_HEADROOM);
    }

    return ti->current_vbat;
}
#endif /* TXPWRBACKOFF || WL_TVPM */
