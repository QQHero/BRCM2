/*
 * ACPHY DeepSleepInit module
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
 * $Id: phy_ac_dsi.c 789419 2020-07-28 12:09:59Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>

/* PHY common dependencies */
#include <phy_dbg.h>
#include <phy_mem.h>
#include <phy_utils_var.h>
#include <phy_utils_reg.h>
#include <phy_utils_radio.h>

/* PHY type dependencies */
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <wlc_phyreg_ac.h>
#include <wlc_phy_radio.h>
#include <wlc_phytbl_ac.h>

/* DSI module dependencies */
#include <phy_ac_dsi.h>
#include "phy_ac_dsi_data.h"
#include "phy_type_dsi.h"

/* Inter-module dependencies */
#include "phy_ac_radio.h"

#include "fcbs.h"

typedef struct {
	uint8 ds1_napping_enable;
} phy_ac_dsi_params_t;

/* module private states */
struct phy_ac_dsi_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_dsi_info_t *di;
	phy_ac_dsi_params_t *dp;
};

typedef struct {
	int num;
	fcbs_input_data_t *data;
} dsi_fcbs_t;

typedef struct {
	int8 blk_num;
	int8 exec_seq_num;
	fcbs_input_data_t *data;
} dsi_radio_fcbs_t;

static const char BCMATTACHDATA(rstr_ds1nap)[] = "ds1_nap";

#define DSI_DBG_PRINTS 0

#define FCBS_DS0_RADIO_PD_BLOCK_AUX (2)
#define FCBS_DS0_RADIO_PU_BLOCK_AUX (3)

/* debug prints */
#if defined(DSI_DBG_PRINTS) && DSI_DBG_PRINTS
#define DSI_DBG(args)	printf args; OSL_DELAY(500);
#else
#define DSI_DBG(args)
#endif /* DSI_DBG_PRINTS */

/* accessor functions */
dsi_fcbs_t * BCMRAMFN(dsi_get_ram_seq)(phy_info_t *pi);
dsi_radio_fcbs_t *BCMRAMFN(dsi_get_radio_pu_dyn_seq)(phy_info_t *pi, int8 ds_idx);

fcbs_input_data_t *BCMRAMFN(dsi_get_radio_pd_seq)(phy_info_t *pi);

/* top level wrappers */

/* Generic Utils */
static void dsi_update_radio_seq(phy_info_t *pi, int8 ds_idx, bool *seq_en_flags);

typedef enum {
	MINI_PMU_PU_OFF = 0,
	PLL_2G_OFF = 1,
	PLL_5G_TO_2G_OFF = 2,
	PLL_5G_OFF = 3,
#ifdef MULTIBAND
	CHAN_TUNE_OFF = 4,
#else
	CHAN_TUNE_OFF = 3
#endif /* MULTIBAND */
} radio_pu_seq_off_t;

typedef enum {
	RADIO_MINI_PMU_PU_OFF = 0,
	RADIO_PLL_PU_VCOCAL_OFF = 1,
	RADIO_CHAN_TUNE_OFF = 2,
	NUM_RADIO_PU_SEQ = 3,
	RADIO_PD_OFF = 3,
	NUM_RADIO_SEQ = 4
} radio_seq_enbl_off_t;

dsi_fcbs_t *
BCMRAMFN(dsi_get_ram_seq)(phy_info_t *pi)
{
	PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
	ASSERT(0);

	return NULL;
}

dsi_radio_fcbs_t *
BCMRAMFN(dsi_get_radio_pu_dyn_seq)(phy_info_t *pi, int8 ds_idx)
{
	dsi_radio_fcbs_t *ret_ptr = NULL;

	PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
	ASSERT(0);

	return ret_ptr;
}

fcbs_input_data_t *
BCMRAMFN(dsi_get_radio_pd_seq)(phy_info_t *pi)
{
	PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
	ASSERT(0);

	return NULL;
}

/* register phy type specific implementation */
phy_ac_dsi_info_t *
BCMATTACHFN(phy_ac_dsi_register_impl)(phy_info_t *pi, phy_ac_info_t *aci, phy_dsi_info_t *di)
{
	phy_ac_dsi_info_t *info;
	phy_type_dsi_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((info = phy_malloc(pi, sizeof(phy_ac_dsi_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if ((info->dp = phy_malloc(pi, sizeof(phy_ac_dsi_params_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	info->pi = pi;
	info->aci = aci;
	info->di = di;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));

	fns.ctx = info;

	phy_dsi_register_impl(di, &fns);

	/* Register DS1 entry call back */

	/* By default, DS1 napping will be disabled */
	info->dp->ds1_napping_enable = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_ds1nap, 0);

	return info;

	/* error handling */
fail:
	if (info) {

		if (info->dp)
			phy_mfree(pi, info->dp, sizeof(phy_ac_dsi_params_t));

		phy_mfree(pi, info, sizeof(phy_ac_dsi_info_t));
	}

	return NULL;
}

void
BCMATTACHFN(phy_ac_dsi_unregister_impl)(phy_ac_dsi_info_t *info)
{
	phy_info_t *pi = info->pi;
	phy_dsi_info_t *di = info->di;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_dsi_unregister_impl(di);

	if (info->dp)
		phy_mfree(pi, info->dp, sizeof(phy_ac_dsi_params_t));

	phy_mfree(pi, info, sizeof(phy_ac_dsi_info_t));
}

static void
dsi_update_radio_seq(phy_info_t *pi, int8 ds_idx, bool *seq_en_flags)
{
	PHY_ERROR(("wl%d %s: Invalid ACMAJORREV!\n", PI_INSTANCE(pi), __FUNCTION__));
	ASSERT(0);
}

#ifdef WL_DSI
void
ds0_radio_seq_update(phy_info_t *pi)
{
	/* Seq order : Minpmu_PU, PLL_PU, Chan_tune, Radio_PD */
	bool init_seq_en[NUM_RADIO_SEQ] = {TRUE, TRUE, TRUE, TRUE};
	bool band_change_seq_en[NUM_RADIO_SEQ] = {FALSE, TRUE, TRUE, FALSE};
	bool chan_change_seq_en[NUM_RADIO_SEQ] = {FALSE, FALSE, TRUE, FALSE};

	bool *seq_en_ptr;
	uint err = 0;
	uint blk_shm_addr, cmd_ptr_shm_addr;
	uint32 cmd_ptr = FCBS_DS0_BM_CMDPTR_BASE_CORE0;
	uint32 data_ptr = FCBS_DS0_BM_DATPTR_BASE_CORE0;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint16 drv_ucode_if_blk_addr;

	BCM_REFERENCE(blk_shm_addr);
	BCM_REFERENCE(cmd_ptr_shm_addr);

	if (pi->pubpi->slice == DUALMAC_AUX) {
		cmd_ptr = FCBS_DS0_BM_CMDPTR_BASE_CORE1;
		data_ptr = FCBS_DS0_BM_DATPTR_BASE_CORE1;
	}
	/* Driver ucode interface SHM block address */
	drv_ucode_if_blk_addr = wlapi_bmac_read_shm(pi->sh->physhim, M_DRVR_UCODE_IF_PTR(pi));

	/* Radio PU block SHM address */
	blk_shm_addr = (drv_ucode_if_blk_addr * SHM_ENTRY_SIZE) +
			M_FCBS_DS0_RADIO_PU_BLOCK_OFFSET(pi);

	if (CCT_INIT(pi_ac)) {
		seq_en_ptr = init_seq_en;
	} else if (CCT_BAND_CHG(pi_ac)) {
		seq_en_ptr = band_change_seq_en;

		/* Get PLL PU FCBS sequence cmd pointer SHM address */
		cmd_ptr_shm_addr = blk_shm_addr + SHM_ENTRY_SIZE +
				(RADIO_PLL_PU_VCOCAL_OFF * FCBS_SHM_SEQ_SZ);

		/* Read the command and data pointer address and convert it to byte address */
		cmd_ptr = wlapi_bmac_read_shm(pi->sh->physhim, cmd_ptr_shm_addr) << 2;
		data_ptr = wlapi_bmac_read_shm(pi->sh->physhim,
				(cmd_ptr_shm_addr + SHM_ENTRY_SIZE)) << 2;
	} else {
		seq_en_ptr = chan_change_seq_en;

		/* Get Chan tune FCBS sequence cmd pointer SHM address */
		cmd_ptr_shm_addr = blk_shm_addr + SHM_ENTRY_SIZE +
				(RADIO_CHAN_TUNE_OFF * FCBS_SHM_SEQ_SZ);

		/* Read the command and data pointer address and convert it to byte address */
		cmd_ptr = wlapi_bmac_read_shm(pi->sh->physhim, cmd_ptr_shm_addr) << 2;
		data_ptr = wlapi_bmac_read_shm(pi->sh->physhim,
				(cmd_ptr_shm_addr + SHM_ENTRY_SIZE)) << 2;
	}

	/* Reset FCBS cmd and data pointers */
	err = fcbs_reset_cmd_dat_ptrs(wlapi_bmac_get_fcbs_info(pi->sh->physhim),
		FCBS_DS0, cmd_ptr, data_ptr);

	if (err != BCME_OK) {
		PHY_ERROR(("wl%d %s: Failed to reset FCBS cmd/data pts.", PI_INSTANCE(pi),
				__FUNCTION__));
		ASSERT(0);
	}

	/* Update Radio sequences */
	dsi_update_radio_seq(pi, FCBS_DS0, seq_en_ptr);
}
#endif /* WL_DSI */
