/*
 * PAPD CAL module implementation - iovar handlers & registration
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
 * $Id: phy_papdcal_iov.c 802962 2021-09-10 22:14:35Z $
 */

#include <phy_papdcal_iov.h>
#include <phy_papdcal.h>
#include <phy_type_papdcal.h>
#include <wlc_phyreg_ac.h>
#include <phy_utils_reg.h>
#include <wlc_iocv_reg.h>
#include <wlc_phy_int.h>

static const bcm_iovar_t phy_papdcal_iovars[] = {
#if defined(WLTEST) || defined(BCMDBG)
	{"phy_enable_epa_dpd_2g", IOV_PHY_ENABLE_EPA_DPD_2G, IOVF_SET_UP, 0, IOVT_INT8, 0},
	{"phy_enable_epa_dpd_5g", IOV_PHY_ENABLE_EPA_DPD_5G, IOVF_SET_UP, 0, IOVT_INT8, 0},
	{"phy_epacal2gmask", IOV_PHY_EPACAL2GMASK, 0, 0, IOVT_INT16, 0},
#endif /* defined(WLTEST) || defined(BCMDBG) */

#if defined(WLTEST)
	{"phy_pacalidx0", IOV_PHY_PACALIDX0, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_pacalidx1", IOV_PHY_PACALIDX1, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_pacalidx", IOV_PHY_PACALIDX, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_papdbbmult", IOV_PHY_PAPDBBMULT, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_papdextraepsoffset", IOV_PHY_PAPDEXTRAEPSOFFSET, (IOVF_GET_UP | IOVF_MFG), 0,
	IOVT_UINT32, 0},
	{"phy_papdtiagain", IOV_PHY_PAPDTIAGAIN, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"papdcomp_disable", IOV_PAPDCOMP_DISABLE, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_papd_endeps", IOV_PHY_PAPD_ENDEPS, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_papd_epstbl", IOV_PHY_PAPD_EPSTBL, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_papd_dump", IOV_PHY_PAPD_DUMP, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_papd_abort", IOV_PHY_PAPD_ABORT, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_wbpapd_gctrl", IOV_PHY_WBPAPD_GCTRL, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
	{"phy_wbpapd_multitbl", IOV_PHY_WBPAPD_MULTITBL, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32,
	0},
	{"phy_wbpapd_sampcapt", IOV_PHY_WBPAPD_SAMPCAPT, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32,
	0},
	{"phy_peak_psd_limit", IOV_PHY_PEAK_PSD_LIMIT, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT32, 0},
#endif
#if defined(WLTEST) || defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG) || \
	defined(ATE_BUILD)
	{"phy_papd_en_war", IOV_PAPD_EN_WAR, (IOVF_SET_UP | IOVF_MFG), 0, IOVT_UINT8, 0},
#ifndef ATE_BUILD
	{"phy_skippapd", IOV_PHY_SKIPPAPD, (IOVF_GET_UP | IOVF_MFG), 0, IOVT_UINT8, 0},
#endif /* !ATE_BUILD */
#endif
#if defined(WFD_PHY_LL)
	{"phy_wfd_ll_enable", IOV_PHY_WFD_LL_ENABLE, 0, 0, IOVT_UINT8, 0},
#endif /* WFD_PHY_LL */
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_papdcal_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	BCM_REFERENCE(*pi);
	BCM_REFERENCE(*ret_int_ptr);

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
#if defined(WLTEST) || defined(BCMDBG)
		case IOV_SVAL(IOV_PHY_ENABLE_EPA_DPD_2G):
		case IOV_SVAL(IOV_PHY_ENABLE_EPA_DPD_5G):
		{
			if ((int_val < 0) || (int_val > 1)) {
				err = BCME_RANGE;
				PHY_ERROR(("Value out of range\n"));
				break;
			}
			phy_papdcal_epa_dpd_set(pi, (uint8)int_val,
				(aid == IOV_SVAL(IOV_PHY_ENABLE_EPA_DPD_2G)));
			break;
		}
		case IOV_GVAL(IOV_PHY_EPACAL2GMASK): {
			*ret_int_ptr = (uint32)pi->papdcali->data->epacal2g_mask;
			break;
		}

		case IOV_SVAL(IOV_PHY_EPACAL2GMASK): {
			pi->papdcali->data->epacal2g_mask = (uint16)int_val;
			break;
		}
#endif /* defined(WLTEST) || defined(BCMDBG) */

#if defined(WLTEST)
		case IOV_GVAL(IOV_PHY_PACALIDX0):
			err = phy_papdcal_get_lut_idx0(pi, ret_int_ptr);
			break;

		case IOV_GVAL(IOV_PHY_PACALIDX1):
			err = phy_papdcal_get_lut_idx1(pi, ret_int_ptr);
			break;

		case IOV_GVAL(IOV_PHY_PACALIDX):
			err = phy_papdcal_get_idx(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PACALIDX):
			err = phy_papdcal_set_idx(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_PAPDBBMULT):
			err = phy_papdcal_get_bbmult(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PAPDBBMULT):
			err = phy_papdcal_set_bbmult(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_PAPDEXTRAEPSOFFSET):
			err = phy_papdcal_get_extraepsoffset(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PAPDEXTRAEPSOFFSET):
			err = phy_papdcal_set_extraepsoffset(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_PAPDTIAGAIN):
			err = phy_papdcal_get_tiagain(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PAPDTIAGAIN):
			err = phy_papdcal_set_tiagain(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PAPDCOMP_DISABLE):
			err = phy_papdcal_get_comp_disable(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PAPDCOMP_DISABLE):
			err = phy_papdcal_set_comp_disable(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_PAPD_ENDEPS):
			err = phy_papdcal_get_end_epstblidx(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PAPD_ENDEPS):
			err = phy_papdcal_set_end_epstblidx(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_PAPD_EPSTBL):
			err = phy_papdcal_get_epstblsel(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PAPD_EPSTBL):
			err = phy_papdcal_set_epstblsel(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_PAPD_DUMP):
			err = phy_papdcal_get_dump(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PAPD_DUMP):
			err = phy_papdcal_set_dump(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_PAPD_ABORT):
			err = phy_papdcal_get_abort(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PAPD_ABORT):
			err = phy_papdcal_set_abort(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_WBPAPD_GCTRL):
			err = phy_papdcal_get_wbpapd_gctrl(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_WBPAPD_GCTRL):
			err = phy_papdcal_set_wbpapd_gctrl(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_WBPAPD_MULTITBL):
			err = phy_papdcal_get_wbpapd_multitbl(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_WBPAPD_MULTITBL):
			err = phy_papdcal_set_wbpapd_multitbl(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_WBPAPD_SAMPCAPT):
			err = phy_papdcal_get_wbpapd_sampcapt(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_WBPAPD_SAMPCAPT):
			err = phy_papdcal_set_wbpapd_sampcapt(pi, (int32)int_val);
			break;

		case IOV_GVAL(IOV_PHY_PEAK_PSD_LIMIT):
			err = phy_papdcal_get_peak_psd_limit(pi, ret_int_ptr);
			break;

		case IOV_SVAL(IOV_PHY_PEAK_PSD_LIMIT):
			err = phy_papdcal_set_peak_psd_limit(pi, (int32)int_val);
			break;
#endif
#if defined(WLTEST) || defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG) || \
	defined(ATE_BUILD)
		case IOV_SVAL(IOV_PAPD_EN_WAR):
			wlapi_bmac_write_shm(pi->sh->physhim, M_PAPDOFF_MCS(pi), (uint16)int_val);
			break;

		case IOV_GVAL(IOV_PAPD_EN_WAR):
			*ret_int_ptr = wlapi_bmac_read_shm(pi->sh->physhim, M_PAPDOFF_MCS(pi));
			break;
#ifndef ATE_BUILD
		case IOV_SVAL(IOV_PHY_SKIPPAPD):
			if ((int_val != 0) && (int_val != 1)) {
				err = BCME_RANGE;
				break;
			}
			err = phy_papdcal_set_skip(pi, (uint8)int_val);
			break;

		case IOV_GVAL(IOV_PHY_SKIPPAPD):
			err = phy_papdcal_get_skip(pi, ret_int_ptr);
			break;
#endif /* !ATE_BUILD */
#endif
#if defined(WFD_PHY_LL)
		case IOV_SVAL(IOV_PHY_WFD_LL_ENABLE):
			if ((int_val < 0) || (int_val > 2)) {
				err = BCME_RANGE;
			} else {
				err = phy_papdcal_set_wfd_ll_enable(pi->papdcali, (uint8) int_val);
			}
			break;

		case IOV_GVAL(IOV_PHY_WFD_LL_ENABLE):
			err = phy_papdcal_get_wfd_ll_enable(pi->papdcali, ret_int_ptr);
			break;
#endif /* WFD_PHY_LL */
		default:
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_papdcal_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
	wlc_iov_disp_fn_t disp_fn = IOV_PATCH_FN;
	const bcm_iovar_t *patch_table = IOV_PATCH_TBL;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	const bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_papdcal_iovars,
	                   NULL, NULL,
	                   phy_papdcal_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
