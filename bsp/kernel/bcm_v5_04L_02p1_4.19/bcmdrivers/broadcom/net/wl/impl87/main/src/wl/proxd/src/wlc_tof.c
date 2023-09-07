/*
 * TOF based proximity detection implementation for Broadcom 802.11 Networking Driver
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
 * $Id: wlc_tof.c 795955 2021-02-18 19:37:54Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcm_math.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <802.11.h>
#include <wlioctl.h>
#include <bcmwpa.h>
#include <d11.h>
#include <wlc_cfg.h>
#include <wlc_pub.h>
#include <wlc_hrt.h>
#include <wlc_rate.h>
#include <wlc_key.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scan.h>
#include <wl_export.h>
#include <wlc_assoc.h>
#include <wlc_bmac.h>
#include <wlc_pcb.h>

#include <wlc_pdsvc.h>
#include <wlc_pddefs.h>
#include <wlc_pdmthd.h>
#include <wlc_fft.h>
#include <phy_tof_api.h>
#undef TOF_DBG

/* Different subband support */
static int wlc_tof_bw_offset(tof_rtd_adj_params_t *params)
{
	int offset = 0;

	switch (params->subband) {
		case PRXS_SUBBAND_80:
		case PRXS_SUBBAND_40L:
		case PRXS_SUBBAND_20LL:
			offset = 0;
			break;
		case PRXS_SUBBAND_40U:
		case PRXS_SUBBAND_20UL:
			offset = 128;
			break;
		case PRXS_SUBBAND_20LU:
			offset = 64;
			break;
		case PRXS_SUBBAND_20UU:
			offset = 192;
			break;
		default:
			break;
		}

	return (offset * 2);
}

/* output of FFT is a time reversed impulse response */
static int
wlc_tof_fft_n(wlc_info_t *wlc, int bw, cint32 *H)
{
	int ret = BCME_OK;

	if (bw == TOF_BW_160MHZ) {
		ret = FFT512(wlc->osh, H, H);
	} else if (bw == TOF_BW_80MHZ) {
		ret = FFT256(wlc->osh, H, H);
	} else if (bw == TOF_BW_40MHZ) {
		ret = FFT128(wlc->osh, H, H);
	} else if (bw == TOF_BW_20MHZ) {
		ret = FFT64(wlc->osh, H, H);
	} else {
		ret = BCME_UNSUPPORTED;
	}

	return ret;
}

static int
wlc_tof_subband_derotation(int bw, cint32*H)
{
	int ret = BCME_OK;
	uint16 i;
	int32  tmp;

	switch (bw) {
		/* 1: k < -192 , -1: -192 <= k < 0, 1: 0 <= k < 64, -1: k > 64 */
		case TOF_BW_160MHZ:
			for (i = 0u; i < NFFT_BASE; i++) {
				H[i].i = -H[i].i;
				H[i].q = -H[i].q;
			}
			for (i = (4u * NFFT_BASE); i < (5u * NFFT_BASE); i++) {
					H[i].i = -H[i].i;
					H[i].q = -H[i].q;
			}
			break;
		case TOF_BW_80MHZ:
			/* 1: k < -64, else -1: k > 64 */
			for (i = 0u; i < NFFT_BASE; i++) {
					H[i].i = -H[i].i;
					H[i].q = -H[i].q;
				}
			break;
		case TOF_BW_40MHZ:
			/* 1: k < 0, j: k >= 0 */
			for (i = 0u; i < NFFT_BASE; i++) {
				tmp = H[i].i;
				H[i].i = H[i].q;
				H[i].q = -tmp;
				}
			break;
		case TOF_BW_20MHZ:
			/* No de-rotation required */
			break;
		default:
			ret = BCME_UNSUPPORTED;
			break;
	}

	return ret;
}

/* For computing group delay */
#define CORDIC32_LOG2_2_PI (CORDIC32_LOG2_PI_OVER_TWO + 2)
static int
wlc_tof_compute_gd(cint32 *H, tof_rtd_adj_params_t *params, int32 *theta,
	int32 *gd_adj_ns_q3)
{
	int ret = BCME_OK;
	uint8 rshift, sub80_idx, no_sub80 = 1;
	uint16 nosc_lowsb, nosc, idx, sub80_offset, dctone_160, nfft;
	cint32 acc;
	int32 tmp_32;
	int64 tmp_64;

	nfft = params->nfft;
	/* rshift: used to avoid overflow during accumulation */
	/* nosc_lowsb: No. of subcarries in lower sub-band */
	/* Even though if some of the subcarriers are zero in the specified range */
	/* it will not impact the gd computation as H(n)*conj(H(n+1)) will be zero */
	switch (params->bw) {
		case TOF_BW_20MHZ:
			rshift = TOF_BW_20MHZ_INDEX_V2;
			nosc_lowsb = 28;
			break;
		case TOF_BW_40MHZ:
			rshift = TOF_BW_40MHZ_INDEX_V2;
			nosc_lowsb = 58;
			break;
		case TOF_BW_80MHZ:
			rshift = TOF_BW_80MHZ_INDEX_V2;
			nosc_lowsb = 122;
			break;
		case TOF_BW_160MHZ:
			rshift = TOF_BW_160MHZ_INDEX_V2;
			nosc_lowsb = 122;
			no_sub80 = 2;
			break;
		default:
			ret = BCME_UNSUPPORTED;
			goto done;
	}
	/* compute sigma(H(n) * conj(H(n+1))) */
	acc.i = 0; acc.q = 0;
	for (sub80_idx = 0; sub80_idx < no_sub80; sub80_idx++) {
		sub80_offset = sub80_idx * (nfft >> 1);

		/* For 160MHz case, each 80MHz sub band has 3 DC tones which are */
		/* not nulled in the chanest table. So, explicitly zero it out */
		if (params->bw == TOF_BW_160MHZ) {
			dctone_160 = (nfft >> 2);
			H[dctone_160 - 1 + sub80_offset].i = 0;
			H[dctone_160 - 1 + sub80_offset].q = 0;
			H[dctone_160 + sub80_offset].i = 0;
			H[dctone_160 + sub80_offset].q = 0;
			H[dctone_160 + 1 + sub80_offset].i = 0;
			H[dctone_160 + 1 + sub80_offset].q = 0;
		}

		/* starting index skipping the edge dc subc */
		if (params->bw == TOF_BW_160MHZ) {
			idx = (nfft >> 2) - nosc_lowsb + sub80_offset;
		} else {
			idx = (nfft >> 1) - nosc_lowsb + sub80_offset;
		}
		nosc = (nosc_lowsb << 1);
		while (nosc-- > 0) {
			acc.i += ((H[idx].i * H[idx +1].i) + (H[idx].q * H[idx +1].q)) >> rshift;
			acc.q += ((H[idx].q * H[idx +1].i) - (H[idx].i * H[idx +1].q)) >> rshift;
			idx++;
		}
	}
	/* Make sure that there is some space for cordic */
	tmp_32 = ((acc.i >> 28) ^ (acc.q >> 28)) & 0xf;
		if ((tmp_32 != 0) && (tmp_32 != 0xf)) {
			acc.i = acc.i >> 4;
			acc.q = acc.q >> 4;
		}
	/* Find angle(sigma(H(n) * conj(H(n+1)))) */
	/* theta is in i.e, [-pi, pi] [2^[-19 to 19]] */
	*theta = math_cordic(acc);
	/* theta([2^[-19 to 19]] max 20bits) * nfft (max 9bits for 160MHz) = 29 bits */
	/* so, (theta * nfft) value will not overflow */
	tmp_32 = (*theta) * nfft;
	/* gd = (N*theta/2*pi) -> gives gd in units of samples */
	params->gd = (tmp_32 >> (CORDIC32_LOG2_2_PI));
	/* Compute gd in ns = (gd_samples(tmp_32/2*pi) * Ts) */
	/* tmp_32 (max 29 bits) * h_ts (u(6,3) max 9bits) */
	/* total needed is 38bits to maintain resolution */
	/* tmp_64/2*pi = max 18bits with q3 format */
	tmp_64 = (int64)((int64)tmp_32 * (int64)params->h_ts);
	*gd_adj_ns_q3 = (int32)ROUND(tmp_64, CORDIC32_LOG2_2_PI);
	params->gd_ns = *gd_adj_ns_q3 >> 3;
done:
	return ret;
}

/* Post-processing of the channel to compute the first path */
#define DELAY_IMPULSE 16u
int
tof_rtd_adj(wlc_info_t *wlc, tof_rtd_adj_params_t *params,
	int32 *chan_data, uint32 *chan_data_len)
{
	int ret = BCME_OK;
	uint16 nfft;
	cint32 *H = NULL;
	int32  theta, tmp, adj_ns;
	int32 gd_adj_ns = 0, gd_adj_ns_q3 = 0;

	if (params->longwin_en) {
		params->w_offset = params->w_len << 1;
	}

	/* take nfft and h_Ts from params */
	nfft = params->nfft;

	if (params->w_ext) {
		/* HW Adj case, gd computed is in q1 format */
		gd_adj_ns = params->gd_ns; /* Q1 */
		params->gd_ns = (gd_adj_ns + 1) >>1;
		params->gd_shift = FALSE;
	} else {
		/* SW Adj case */
		params->gd_shift = TRUE;
		/* Read Channel */
		H = (cint32*)(params->H + wlc_tof_bw_offset(params));
#ifdef TOF_COLLECT_INLINE
		if (chan_data && chan_data_len) {
			uint32 len = 2 * nfft * sizeof(uint32);
			if (*chan_data_len >= len) {
				(void)memcpy_s(chan_data, *chan_data_len, H, len);
				*chan_data_len = len;
			}
		}
#endif /* TOF_COLLECT_INLINE */
		/* Undo sub-band rotation */
		if (!params->tdcs_en) {
			wlc_tof_subband_derotation(params->bw, H);
		}
		/* Compute group delay value */
		/* theta in [-pi, pi] -> [2^[-19 to 19]] */
		ret = wlc_tof_compute_gd(H, params, &theta, &gd_adj_ns_q3);
		if (ret != BCME_OK) {
			goto fail;
		}

#ifdef WL_PROXD_GDCOMP
		/* Apply GD comp. on H and add int. delay (DELAY_IMPULSE) to the imp. */
		/* Also, positions impulse response at appropriate place which avoids */
		/* cases where it wraps which causes issues in first path computation */
		phy_tof_gdcomp(H, theta, nfft, DELAY_IMPULSE);
#endif /* WL_PROXD_GDCOMP */

		/* Compute IFFT to find the impulse response */
		ret = wlc_tof_fft_n(wlc, params->bw, H);
		if (ret != BCME_OK)
		{
			goto fail;
		}

#ifdef RSSI_REFINE
		uint16 i;
		for (i = 0u; i < nfft && params->p_A; i++) {
			int32 tmp1, tmp2;
			tmp1 = H[i].i;
			tmp2 = H[i].q;
			tmp = tmp1 * tmp1 + tmp2 * tmp2;
			params->p_A[i] = tmp;
		}
#endif
	}
	ret = tof_pdp_ts(params->log2_nfft, H, params->bw, 1, params, &tmp, &adj_ns, NULL);

	if (ret != BCME_OK) {
		goto fail;
	}
	if (!(params->w_ext)) {
		/* q3 format */
		adj_ns = (adj_ns / 5) << 2; /* return is in 0.1ns, need to make Q3 */
		gd_adj_ns_q3 += adj_ns;
		gd_adj_ns = ROUND(gd_adj_ns_q3, 3);
		params->adj_ns = gd_adj_ns;
	} else {
		/* q1 format */
		adj_ns = (adj_ns / 5); /* return is in 0.1ns, need to make Q1 */
		gd_adj_ns += adj_ns;
		gd_adj_ns = (gd_adj_ns + 1) >> 1;
		params->adj_ns = gd_adj_ns;
	}
fail:
	return ret;
}

#define k_tof_w_Q 6
#define k_tof_w_ts_mult 10000

static int32 tof_pdp_thresh_crossing(int start, int end, uint32* pW, uint32 thresh,
	uint32 overflow_mask, int check)
{
	int k, check_start = -1, check_end;
	uint32 crossing = 0;

	if (end < 0)
		end = 0;

	k = start;
	while (k > end) {
		if ((pW[k] >= thresh) && (pW[k-1] < thresh)) {
			uint32 d1, d0;
			int shft = 0;

			/* Scaling to avoid overflow */
			d1 = pW[k] - pW[k-1];
			d0 = thresh - pW[k-1];
			while ((shft < 31) && (d1 & overflow_mask)) {
				shft++;
				d0 >>= 1;
				d1 >>= 1;
			};
			check_start = k-1;
			crossing = (((k - 1)*d1 + d0) << k_tof_w_Q)/d1;
		}
		k--;
	}

	if (check > 0) {
		if ((check_start > 0)) {
			check_end = check_start - check;
			if (check_end < 0)
				check_end = 0;
			while (check_start >= check_end) {
				if (pW[check_start] > (1*thresh >> 0))
					crossing = 0;
				check_start--;
			};
		} else {
			crossing = 0;
		}
	}

	return crossing;
}

#define TWO_STAGE_THRESH_CROSSING 0
int tof_pdp_ts(int log2n, void* pBuf, int FsMHz, int rx, void* pparams,
	int32* p_ts_thresh, int32* p_thresh_adj, wl_proxd_phy_error_t* tof_phy_error)
{
	int i;
	int n, mask = 0, pmax = 0;
	uint32 *pW, *pWr, max = 0, thresh;
	cint32* pIn = (cint32*)pBuf;
	int wpos;
	int wlen = ((struct tof_rtd_adj_params *)pparams)->w_len;
	int wzero = ((struct tof_rtd_adj_params *)pparams)->w_offset;
	uint32 overflow_mask, tmp;
	uint32 th0, th1;
	bool longwin_en = ((struct tof_rtd_adj_params *)pparams)->longwin_en;

	n = (1 << log2n);
	if (((struct tof_rtd_adj_params *)pparams)->w_ext) {
		pW = (uint32*)((struct tof_rtd_adj_params *)pparams)->w_ext;
		pWr = NULL;
	} else if (pIn) {
		/* Mag sqrd */
		mask = (n - 1);
		pWr = (uint32*)pIn;
		for (i = 0; i < n; i++) {
			int32 di, dq;
			uint32 d;

			di = pIn->i;
			dq = pIn->q;
			d = (uint32)(di * di + dq * dq);
			if (d >= max) {
				pmax = i;
				max = d;
			}
			*pWr++ = d;
			pIn++;
		}
		pW = pWr;
		pWr -= n;
	} else {
		return BCME_ERROR;
	}

	if (tof_find_window(pparams, pW, pWr, max, &pmax, &wzero, &wpos, mask, rx,
			longwin_en, &thresh, tof_phy_error)) {
		return BCME_ERROR; /* Window likely doesnt include leading edge */
	}

	if (longwin_en) {
#ifdef WL_PROXD_GDCOMP
		wzero = wzero + DELAY_IMPULSE - (n - pmax + 1);
#else
		wzero = wzero + (((struct tof_rtd_adj_params *)pparams)->gd) - (n - pmax + 1);
#endif
	} else {
		wzero = wzero - 1;
	}

	wzero <<= k_tof_w_Q;
	/* Threshold crossing in window */
	wpos  <<= k_tof_w_Q;
	overflow_mask = 0xffffffff;
	overflow_mask <<= (32 - log2n - k_tof_w_Q);
	while (n > wlen) {
		overflow_mask <<= 1;
		wlen <<= 1;
	}

	th0 = th1 = 0;
	if (rx && (FsMHz == TOF_BW_160MHZ) && TWO_STAGE_THRESH_CROSSING) {
		int w_u, w_l;

		th0 = tof_pdp_thresh_crossing(pmax, 0, pW, (thresh<<1), overflow_mask, 0);

		w_u = (th0 >> k_tof_w_Q) + 1;
		w_l = w_u - 4;
		th1 = tof_pdp_thresh_crossing(w_u, w_l, pW, (thresh), overflow_mask, 0);

		tmp = th1;
	} else {
		tmp = tof_pdp_thresh_crossing(pmax, 0, pW, (thresh), overflow_mask, 0);
	}

	*p_ts_thresh  = (k_tof_w_ts_mult*((int32)tmp + (int32)wpos))/(FsMHz << k_tof_w_Q);
	if (p_thresh_adj)
		*p_thresh_adj = (k_tof_w_ts_mult*((int32)tmp - (int32)wzero))/(FsMHz << k_tof_w_Q);

	return BCME_OK;
}

void tof_retrieve_thresh(void* pparams, uint16* bitflip_thresh, uint16* snr_thresh)
{
	if (!pparams || !bitflip_thresh || !snr_thresh)
		return;

	*bitflip_thresh = ((struct tof_rtd_adj_params *)pparams)->bitflip_thresh;
	*snr_thresh = ((struct tof_rtd_adj_params *)pparams)->snr_thresh;
}

/* Position window to include leading edge of impulse response */
int
tof_find_window(void *pparams, uint32 *pW, uint32 *pWr, uint32 max, int *pmax, int *wzero,
		int *wpos, int mask, int rx, bool longwin_en, uint32 *thresh,
		wl_proxd_phy_error_t *tof_phy_error)
{
	int wlen = ((struct tof_rtd_adj_params *)pparams)->w_len, wshift;
	int thresh_scale = ((struct tof_rtd_adj_params *)pparams)->thresh_scale[rx];
	int thresh_log2 = ((struct tof_rtd_adj_params *)pparams)->thresh_log2[rx];
	int k, i;
	int ret = BCME_OK;
	wl_proxd_phy_error_t err_mask = 0;
	int loop;
	uint32 lmax = 0;
	int lpmax = 0;

	BCM_REFERENCE(lpmax);

	if (longwin_en) {
		wshift = *pmax;
	} else {
		if (((struct tof_rtd_adj_params *)pparams)->gd_shift) {
#ifdef WL_PROXD_GDCOMP
			wshift = -DELAY_IMPULSE;
#else
			wshift = -(((struct tof_rtd_adj_params *)pparams)->gd);
#endif /* WL_PROXD_GDCOMP */
		} else {
			wshift = *pmax;
		}
	}

	*wpos = (-(wshift + *wzero)) & mask;

	if (longwin_en) {
		/* 'wpos' is the starting pt of the window in impulse response (not time-reversed).
		 * 'wshift' is the index of max in time-reversed impulse response. 'wlen' is the
		 * length of the sample window. The max index is used as reference.
		 * Start the sample window 'wzero' samples to the left from the max.
		 */
		*thresh = ((uint32)thresh_scale * max) >> thresh_log2;
		loop = 0;
		/* Position window to include leading edge of impulse response */
		do {
			lmax = 0;
			for (i = wshift + *wzero - 1, k = 0; k < wlen; i--, k++) {
				if (pWr)
					pW[k] = pWr[(i & mask)];
				if (pW[k] >= lmax) {
					lmax = pW[k];
					lpmax = k;
				}
			}
			*wzero = (*wzero) - 1;
			*wpos = (*wpos) + 1;
			/* The window should contain a threshold crossing */
			if ((pW[0] < (*thresh - (*thresh >> 1))) && (lmax > *thresh))
				break;
			loop++;
		} while (loop <= wlen);
		*wpos = (*wpos) - 1;
	} else {
		do {
			max = 0;
			for (i = wshift + *wzero, k = 0; k < wlen; i--, k++) {
				if (pWr)
					pW[k] = pWr[(i & mask)];
				if (pW[k] >= max) {
					max = pW[k];
					*pmax = k;
				}
			}
			*thresh = ((uint32)thresh_scale*max) >> thresh_log2;
			if (pW[0] < (*thresh - (*thresh >> 1)))
				break;
			*wzero = (*wzero) + 1;
			*wpos = (*wpos) - 1;
		} while (*wzero < wlen);
	}

	if (pW[0] >= *thresh || (max <= TOF_MIN_CORR_PEAK_PWR))  {
		if ((pW[0] >= *thresh) && (tof_phy_error != NULL)) {
			err_mask = rx ?
				WL_PROXD_PHY_ERR_RX_CORR_THRESH :
				WL_PROXD_PHY_ERR_LB_CORR_THRESH;
			*(tof_phy_error) |= err_mask;
		}
		if ((max <= TOF_MIN_CORR_PEAK_PWR) && (tof_phy_error != NULL)) {
			err_mask = rx ?
				WL_PROXD_PHY_ERR_RX_PEAK_POWER :
				WL_PROXD_PHY_ERR_LB_PEAK_POWER;
			*(tof_phy_error) |= err_mask;
		}
		ret = BCME_ERROR;
		printf("tof_find_window() case 2 returning error; check chanest table read\n");
	}

	return ret;
}

#ifdef RSSI_REFINE
int32 find_crossing(int32* T, int max, int nfft, uint32 threshold);
#define RSSI_VHTSCALE	100
#define RSSI_NFFT_RATIO	(RSSI_VHTSCALE*256/8)
/* Find crossing point */
int32 find_crossing(int32* T, int max, int nfft, uint32 threshold)
{
	int i;
	uint32 vhigh, vth, z, rt;
	int32 delta;

	threshold = (1 << (TOF_MAXSCALE - threshold));
	for (i = 0; i < nfft; i++) {
		if (T[i] > (int32)threshold) {
			break;
		}
	}
	if (i == nfft || i == 0) i = 1;

	vhigh = T[i] - T[i-1];
	vth = threshold - T[i-1];
	z = vhigh / 10000000;
	if (z > 1) {
		vhigh /= z;
		vth /= z;
	}

	rt = (vth *RSSI_VHTSCALE) / vhigh;
	rt += RSSI_VHTSCALE * (i-1);
	delta = (nfft * RSSI_VHTSCALE / 2) - rt;
	/* rounding up to the nearest integer */
	delta = (delta * RSSI_NFFT_RATIO)/nfft;

	return delta;
}
#endif /* RSSI_REFINE */
