/*
 * ACPHY noise module implementation - iovar handlers & registration
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
 * $Id: $
 */

#include <phy_ac_noise_iov.h>
#include <phy_ac_noise.h>
#include <wlc_iocv_reg.h>
#include <phy_ac_info.h>

/* iovar ids */
enum {
	IOV_PHY_KNOISE = 1,
#if defined(BCMDBG) || defined(WLTEST)
	/* IOVARs for hw-knoise debug/tuning */
	IOV_PHY_KNOISE_RETRY_LIMIT = 2,
	IOV_PHY_KNOISE_RETRY_TMOUT = 3,
	IOV_PHY_KNOISE_RETRY_LIMIT_SCAN = 4,
	IOV_PHY_KNOISE_RETRY_TMOUT_SCAN = 5
#endif // defined(BCMDBG) || defined(WLTEST)
};

static const bcm_iovar_t phy_ac_noise_iovars[] = {
	{"phy_knoise", IOV_PHY_KNOISE, IOVF_SET_UP, 0, IOVT_UINT32, 0},
#if defined(BCMDBG) || defined(WLTEST)
	/* The following IOVARs support hw-knoise debug/tuning for
	 * the hw-knoise FIRST_TRY state averaging algorithms.
	 */
	{ "phy_knoise_retrylimit", IOV_PHY_KNOISE_RETRY_LIMIT, 0, 0, IOVT_UINT32, 0 },
	{ "phy_knoise_retrytimeout", IOV_PHY_KNOISE_RETRY_TMOUT, 0, 0, IOVT_UINT32, 0 },
	{ "phy_knoise_retrylimit_scan", IOV_PHY_KNOISE_RETRY_LIMIT_SCAN, 0, 0, IOVT_UINT32, 0 },
	{ "phy_knoise_retrytimeout_scan", IOV_PHY_KNOISE_RETRY_TMOUT_SCAN, 0, 0, IOVT_UINT32, 0 },
#endif // defined(BCMDBG) || defined(WLTEST)
	{NULL, 0, 0, 0, 0, 0}
};

#include <wlc_patch.h>

static int
phy_ac_noise_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	phy_info_t *pi = (phy_info_t *)ctx;
	int32 int_val = 0;
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	switch (aid) {
		case IOV_SVAL(IOV_PHY_KNOISE):
			phy_ac_knoise_iov(pi, int_val);
			break;
#if defined(BCMDBG) || defined(WLTEST)
		/* The following IOVARs support hw-knoise debug/tuning for
		 * the hw-knoise FIRST_TRY state averaging algorithms.
		 */
		case IOV_SVAL(IOV_PHY_KNOISE_RETRY_LIMIT):
			err = phy_ac_hwknoise_set_retrylimit_iov(pi, int_val);
			break;
		case IOV_GVAL(IOV_PHY_KNOISE_RETRY_LIMIT):
			*ret_int_ptr = phy_ac_hwknoise_get_retrylimit_iov(pi);
			break;
		case IOV_SVAL(IOV_PHY_KNOISE_RETRY_TMOUT):
			err = phy_ac_hwknoise_set_retrytmout_iov(pi, int_val);
			break;
		case IOV_GVAL(IOV_PHY_KNOISE_RETRY_TMOUT):
			*ret_int_ptr = phy_ac_hwknoise_get_retrytmout_iov(pi);
			break;
		case IOV_SVAL(IOV_PHY_KNOISE_RETRY_LIMIT_SCAN):
			err = phy_ac_hwknoise_set_retrylimit_scan_iov(pi, int_val);
			break;
		case IOV_GVAL(IOV_PHY_KNOISE_RETRY_LIMIT_SCAN):
			*ret_int_ptr = phy_ac_hwknoise_get_retrylimit_scan_iov(pi);
			break;
		case IOV_SVAL(IOV_PHY_KNOISE_RETRY_TMOUT_SCAN):
			err = phy_ac_hwknoise_set_retrytmout_scan_iov(pi, int_val);
			break;
		case IOV_GVAL(IOV_PHY_KNOISE_RETRY_TMOUT_SCAN):
			*ret_int_ptr = phy_ac_hwknoise_get_retrytmout_scan_iov(pi);
			break;
#endif // defined(BCMDBG) || defined(WLTEST)
		default:
			BCM_REFERENCE(ret_int_ptr);
			err = BCME_UNSUPPORTED;
			break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_ac_noise_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
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

	wlc_iocv_init_iovd(phy_ac_noise_iovars,
	                   NULL, NULL,
	                   phy_ac_noise_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}
