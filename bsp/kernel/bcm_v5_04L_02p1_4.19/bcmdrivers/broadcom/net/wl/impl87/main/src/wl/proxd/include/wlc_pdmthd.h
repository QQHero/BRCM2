/*
 * Required functions exported by the wlc_pdrssi.c
 * to common driver code
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
 * $Id: wlc_pdmthd.h 803758 2021-10-06 18:57:51Z $
 */
#ifndef _wlc_pdmthd_h
#define _wlc_pdmthd_h

/* Params required for post processings */
typedef struct tof_rtd_adj_params {
	int         bw;         /* bandwidth */
	uint16      subband;    /* subband */
	int32       *H;         /* channel freq response */
	uint16      nfft;       /* Size of the FFT */
	uint8       log2_nfft;	/* log2 value of nfft */
	uint16      h_ts;       /* Sampling time, in q3 format */
	int         thresh_log2[2]; /* log2 number of simple threshold crossing */
	int         thresh_scale[2]; /* scale number of simple threshold crossing */
	int         w_len;      /* search window length */
	int         w_offset;   /* search window offset */
	bool        gd_shift;   /* center window using gd */
	int32       gd;         /* gd in samples */
	int32       gd_ns;      /* gd in ns */
	int32       adj_ns;     /* RX time adjustment */
	int32       *p_A;       /* RSSI refine */
	uint16      bitflip_thresh; /* bit flip threshold */
	uint16      snr_thresh; /* SNR flip threshold */
	int32       *w_ext;     /* hardware channel smoothing data */
	bool        tdcs_en;    /* Read from TDCS memory */
	bool        longwin_en;    /* bigger search window for threshold crossing */
	uint16      buf_len;    /* Length of buffer allocated for post-processing */
	uint16      buf_len_extra; /* If any extra buffer needs to be allocated */
} tof_rtd_adj_params_t;

extern int32 wlc_pdsvc_average(int32 *arr, int n);
extern uint32 wlc_pdsvc_deviation(int32 *arr, int32 mean, int n, uint8 decimaldigits);
#ifdef WL_PROXD_OUTLIER_FILTERING
extern void wlc_pdsvc_sortasc(int32 *arr, uint16 arr_size);
extern int32 wlc_pdsvc_median(int32 *arr, uint16 arr_size);
#endif /* WL_PROXD_OUTLIER_FILTERING */
extern uint32 wlc_pdsvc_sqrt(uint32 x);
extern int tof_rtd_adj(wlc_info_t *wlc, tof_rtd_adj_params_t *params,
	int32 *chan_data, uint32 *chan_data_len);
extern uint32 proxd_get_ratespec_idx(ratespec_t rspec, ratespec_t ackrspec);

#ifdef TOF_COLLECT
extern int pdburst_collection(wlc_info_t *wlc, void *collect,
	wl_proxd_collect_query_t *query, void *buff, int len, uint16 *reqLen);
#endif /* TOF_DBG */

#endif /* _wlc_pdmthd_h */
