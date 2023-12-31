/*
 * RadarDetect module implementation (shared by PHY implementations)
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
 * $Id: phy_radar_shared.c 810784 2022-04-13 19:59:25Z $
 */

/* XXX: Define phy_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <phy_cfg.h>
#include <typedefs.h>
#include <phy_api.h>
#include <phy_radar.h>
#include "phy_radar_st.h"
#include <phy_radar_utils.h>
#include <phy_radar_shared.h>
#include <phy_radar_api.h>
#include <wlc_dfs.h>
#include <phy_utils_reg.h>

#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>

#include <wlc_phy_n.h>
#include <wlc_phyreg_n.h>

#include <phy_ac_info.h>
#include <wlc_phyreg_ac.h>
#endif /* !ALL_NEW_PHY_MOD */

#define PHY_RADAR_FIFO_SUBBAND_FORMAT(pi) \
		(ACMAJORREV_GE32((pi)->pubpi->phy_rev))

#define MIN_PULSES		6	/* the minimum number of required consecutive pulses */
#define INTERVAL_TOLERANCE	16	/* in 1/20 us */
#define PULSE_WIDTH_TOLERANCE	30	/* in 1/20 us */
#define PULSE_WIDTH_TOLERANCE_6715 40	/* in 1/20 us */
#define PULSE_WIDTH_DIFFERENCE	12	/* in 1/20 us */
#define PULSE_WIDTH_DIFFERENCE_ETSI4     21 /* in 1/20 us */
#define PULSE_WIDTH_DIFFERENCE_STG_LESI  17 /* in 1/20 us */
#define GOOD_CHIRP_CNT_LIMIT 4

typedef struct {
	uint8 radar_type;	/* one of RADAR_TYPE_XXX */
	uint8 num_pulses;	/* min number of pulses required */
	uint16 min_pw;		/* minimum pulse-width (usec * 20) */
	uint16 max_pw;		/* maximum pulse-width (usec * 20) */
	uint16 min_pri;		/* minimum pulse repetition interval (usec) */
	uint16 max_pri;		/* maximum pulse repetition interval (usec) */
} radar_spec_t;

/*
 * Define the specifications used to detect the different Radar types.
 * Note: These are the simple ones.  The long pulse and staggered Radars will be handled seperately.
 * Note: The number of pulses are currently not used, because due to noise we need to reduce the
 *       actual number of pulses to check for (i.e. MIN_PULSES).
 * Note: The order of these are important.  We could execute a sorting routine at startup, but
 *       that will just add to the code size, so instead we order them properly here.
 *       The order should go from smallest max_pw to largest and from smallest range to largest.
 *       This is so that we detect the correct Radar where there's overlap between them.
 */
static const radar_spec_t radar_spec[] = {
	/* Japan 2.1.1: PW 0.5us, PRI 1389us */
	{ RADAR_TYPE_JP2_1_1, 5, 10, 10, 1389, 1389 },	/* num_pulses reduced from 18 */

	/* FCC 6: PW 1us, PRI 333us - same as RADAR_TYPE_JP4 */
	{ RADAR_TYPE_FCC_6, 9, 20, 20, 333, 333 },

	/* Korean 4: PW 1us, PRI 333us (similar to JP 4) */
	{ RADAR_TYPE_KN4, 3, 20, 20, 333, 333 },

	/* Korean 2: PW 1us, PRI 556us */
	{ RADAR_TYPE_KN2, 10, 20, 20, 556, 556 },

	/* ETSI 0: PW 1us, PRI 1429us */
	{ RADAR_TYPE_ETSI_0, 5, 20, 20, 1428, 1429 },	/* num_pulses reduced from 18 */

	/* FCC 0: PW 1us, PRI 1428us - same RADAR_TYPE_JP1_1, RADAR_TYPE_JP2_2_1, RADAR_TYPE_KN1 */
	{ RADAR_TYPE_FCC_0, 5, 20, 20, 1428, 1429 },	/* num_pulses reduced from 18 */

	/* Japan 1.2: PW 2.5us, PRI 3846us */
	{ RADAR_TYPE_JP1_2, 5, 50, 50, 3846, 3846 },	/* num_pulses reduced from 18 */

	/* Japan 2.1.2: PW 2us, PRI 4000us */
	{ RADAR_TYPE_JP2_1_2, 5, 40, 40, 4000, 4000 },	/* num_pulses reduced from 18 */

	/* Korean 3: PW 2us, PRI 3030us */
	{ RADAR_TYPE_KN3, 70, 40, 40, 3030, 3030 },

	/* FCC 1: PW 1us, PRI 518-3066us */
	{ RADAR_TYPE_FCC_1, 5, 20, 20, 518, 3066 },	/* num_pulses reduced from 18 */

	/* FCC 2: PW 1-5us, PRI 150-230us */
	{ RADAR_TYPE_FCC_2, 23, 20, 100, 150, 230 },

	/* ETSI 1: PW 0.5-5us, PRI 1000-5000us */
	{ RADAR_TYPE_ETSI_1, 3, 10, 100, 1000, 5000 },	/* num_pulses reduced from 10 */

	/* Japan 2.2.2: same as FCC 2 */
	{ RADAR_TYPE_JP2_2_2, 23, 20, 100, 150, 230 },

	/* FCC 3: PW 6-10us, PRI 200-500us */
	{ RADAR_TYPE_FCC_3, 16, 120, 200, 200, 500 },

	/* Japan 2.2.3: PW 6-10us, PRI 250-500us (similar to FCC 3) */
	{ RADAR_TYPE_JP2_2_3, 16, 120, 200, 250, 500 },

	/* ETSI 2: PW 0.5-15us, PRI 625-5000us */
	{ RADAR_TYPE_ETSI_2, 5, 10, 300, 625, 5000 },	/* num_pulses reduced from 15 */

	/* ETSI 3: PW 0.5-15us, PRI 250-435us */
	{ RADAR_TYPE_ETSI_3, 25, 10, 300, 250, 435 },

	/* FCC 4: PW 11-20us, PRI 200-500us */
	{ RADAR_TYPE_FCC_4, 12, 220, 400, 200, 500 },

	/* Japan 2.2.4: PW 11-20us, PRI 250-500us (similar to FCC 4) */
	{ RADAR_TYPE_JP2_2_4, 12, 220, 400, 250, 500 },

	/* ETSI 4: PW 20-30us, PRI 250-500us */
	{ RADAR_TYPE_ETSI_4, 20, 400, 600, 250, 500 },

	/* UK 1: PW 1us, PRI 333us (similar to JP 4) */
	{ RADAR_TYPE_UK1, 3, 20, 20, 333, 333 },

	/* UK 2: PW 20us, PRI 222us */
	{ RADAR_TYPE_UK2, 5, 400, 400, 222, 222 },

	{ RADAR_TYPE_NONE, 0, 0, 0, 0, 0 }
};

typedef struct {
	uint8 radar_type;	/* one of RADAR_TYPE_XXX */
	uint8 num_pulses;	/* min number of pulses required */
	uint16 min_pw_sp;	/* minimum short pulse-width (usec * 20) */
	uint16 max_pw_sp;	/* maximum short pulse-width (usec * 20) */
	uint16 min_pw_lp;	/* minimum long pulse-width (usec * 20) */
	uint16 max_pw_lp;	/* maximum long pulse-width (usec * 20) */
	uint16 min_blk;		/* minimum pulse blank time (usec * 20) */
	uint32 min_pri;		/* minimum pulse repetition interval (usec * 20) */
	uint32 max_pri;         /* maximum pulse repetition interval (usec * 20) */
	uint16 pri_tol;		/* tolerance of pulse repetition interval (usec * 20) */
} radar_specNewJP_t;

static const radar_specNewJP_t radar_specNewJP[] = {
	{ RADAR_TYPE_NEW_JP_5, 30, 10, 30, 570, 672, 1000, 17889, 17953, 700 },
	{ RADAR_TYPE_NEW_JP_6, 25, 10, 30, 570, 672, 1000, 21459, 21552, 700 },
	{ RADAR_TYPE_NEW_JP_7, 24, 10, 30, 570, 672, 1000, 22472, 22573, 700 },
	{ RADAR_TYPE_NEW_JP_8, 20, 10, 30, 570, 672, 1000, 26954, 27100, 700 },
	{ RADAR_TYPE_NEW_JP_3, 26, 10, 100, 400, 2200, 1400, 20000, 100000, 2310 },
	{ RADAR_TYPE_NEW_JP_4, 30, 10, 300, 400, 2200, 1400, 12500, 100000, 2530 },
	{ RADAR_TYPE_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

/* current radar fifo sample info */
typedef struct {
	/* pulses (in tstamps) read from PHY FIFO */
	uint16 length_input[RDR_NANTENNAS];
	pulse_data_t pulses_input[RDR_NANTENNAS][RDR_LIST_SIZE];

	/* pulses (in tstamps) used for checking FCC 5 Radar match */
	uint16 length_bin5[RDR_NANTENNAS];
	pulse_data_t pulses_bin5[RDR_NANTENNAS][RDR_LIST_SIZE];

	/* pulses (in intervals) to process for checking short-pulse Radar match */
	uint16 length;
	pulse_data_t pulses[RDR_LIST_SIZE];
} radar_work_t;

/* Field Description: fm_chk_pw_thrs1, fm_chk_pw_thrs2 */
static const uint16 fm_chk_pw[] = {25, 65};
/* Set individual fm threshold in below three pw regions:
 * Region 1: pw < fm_chk_pw_thrs1; Region 2: fm_chk_pw_thrs1 < pw < fm_chk_pw_thrs2;
 * Region 3: fm_chk_pw_thrs2 < pw; Field Description: fmthrs_20_rgn1, fmthrs_20_rgn2,
 * fmthrs_20_rgn3, fmthrs_40_rgn1, fmthrs_40_rgn2, fmthrs_40_rgn3, fmthrs_80_rgn1,
 * fmthrs_80_rgn2, fmthrs_80_rgn3, fmthrs_160_rgn1, fmthrs_160_rgn2, fmthrs_160_rgn3
 */
static const uint16 fm_thrs_sp[] = {0, 15, 60, 0, 170, 255, 30, 250, 255, 10, 250, 255};
static const uint16 fm_thrs_sp_lesi[] = {0, 15, 60, 0, 0, 200, 0, 0, 100, 0, 20, 200};
static const uint16 fm_thrs_sp_6715[] = {0, 15, 60, 0, 0, 100, 0, 0, 100, 0, 20, 200};

#define SUBBAND_NUM_BW40	2
#define SUBBAND_NUM_BW80	4
#define SUBBAND_NUM_BW160	8
#define ALLSUBBAND_BW20	1
#define ALLSUBBAND_BW40	3
#define ALLSUBBAND_BW80	15
#define ALLSUBBAND_BW160_CH50 15
#define ALLSUBBAND_BW160_CH114 255

#define FC_TOL_SB 2
#define FC_TOL_BIN5_SB 5
#define PW_TOL_HIPOWWAR_BIN5 400
#define PW_CHIRP_ADJ_TH_BIN5 800
#define CHIRP_TH1 3
#define CHIRP_TH2 3
#define CHIRP_FC_TH 60

#define FCC5_CHIRP	40	/* 20 MHz */
#define ETSI4_CHIRP	12	/* 6 MHz */
#define SUBBAND_BW	40	/* 20 MHz */

#define MIN_FM_THRESH_SHORT_PULSES	100
#define MIN_FM_THRESH_ETSI4		240

#define MAX_INTV_LP		42000	/* 2 msec max interval of fcc bin 5 pulses */
#define TWO_POWER_32	4294967296
#define CSECT_SINGLE_DECR	2
#define CHIRP_RELAX_LV2_FC_BOUND 0
#define CHIRP_TOL_RELAX_COUNT 1

#define MAX_PW_NEWJP	2500	/* 125us */

void
BCMATTACHFN(phy_radar_shared_attach)(phy_info_t *pi)
{
	/* scratch buffer to use in phy_radar_run_nphy() for fifo sample data */
	phy_cache_register_reuse_size(pi->cachei, sizeof(radar_work_t));
}

static void
wlc_phy_radar_read_table(phy_info_t *pi, phy_radar_st_t *st, radar_work_t *rt, bool sec_pll,
bool bw80_80_mode)
{
	int i;
	uint8 core;
	uint8 ANT_num = 1;
	uint16 w0, w1, w2, w3, w4, w5;
	int FMOffset = 0;
	uint8 time_shift = 0;
	uint16 RadarFifoCtrlReg[2];
	uint16 RadarFifoDataReg[2];
	radar_lp_info_t *rlpt = (sec_pll) ? st->radar_work_lp_sc : &st->radar_lp_info;
	uint8 rdr_core_mask = READ_PHYREGFLD(pi, CoreConfig, CoreMask) & READ_PHYREGFLD(pi,
		RadarDetectConfig1, core_select_overwrite);

	bzero(rt->pulses_input, sizeof(rt->pulses_input));

	if (ISACPHY(pi)) {
		if (bw80_80_mode == FALSE) {
			//4x4, 2core normal radar
			RadarFifoCtrlReg[0] = ACPHY_Antenna0_radarFifoCtrl(pi->pubpi->phy_rev);
			RadarFifoCtrlReg[1] = ACPHY_Antenna1_radarFifoCtrl(pi->pubpi->phy_rev);
			RadarFifoDataReg[0] = ACPHY_Antenna0_radarFifoData(pi->pubpi->phy_rev);
			RadarFifoDataReg[1] = ACPHY_Antenna1_radarFifoData(pi->pubpi->phy_rev);

			//3+1, 1core  scan radar
			if (sec_pll == TRUE) {
				if (ACMAJORREV_47_130(pi->pubpi->phy_rev)) {
					RadarFifoCtrlReg[0] =
						ACPHY_Antenna0_radarFifoCtrl(pi->pubpi->phy_rev);
					RadarFifoCtrlReg[1] = 0;
					RadarFifoDataReg[0] =
						ACPHY_Antenna0_radarFifoData(pi->pubpi->phy_rev);
					RadarFifoDataReg[1] = 0;
				} else { /* 4365/66 */
					RadarFifoCtrlReg[0] =
						ACPHY_Antenna0_radarFifoCtrl_SC(pi->pubpi->phy_rev);
					RadarFifoCtrlReg[1] = 0;
					RadarFifoDataReg[0] =
						ACPHY_Antenna0_radarFifoData_SC(pi->pubpi->phy_rev);
					RadarFifoDataReg[1] = 0;
				}
				ANT_num = 1;
			} else if ((ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
				!ACMAJORREV_128(pi->pubpi->phy_rev)) && ((rdr_core_mask  == 0x8) ||
				(rdr_core_mask == 0x4))) {
				RadarFifoCtrlReg[0] =
					ACPHY_Antenna1_radarFifoCtrl(pi->pubpi->phy_rev);
				RadarFifoCtrlReg[1] = 0;
				RadarFifoDataReg[0] =
					ACPHY_Antenna1_radarFifoData(pi->pubpi->phy_rev);
				RadarFifoDataReg[1] = 0;
				ANT_num = 1;
			} else {
				ANT_num = GET_RDR_NANTENNAS(pi);
			}
		} else {
			ANT_num = 1;
			//L80 in 80+80, 1core normal radar
			if (sec_pll == FALSE) {
				RadarFifoCtrlReg[0] =
					ACPHY_Antenna0_radarFifoCtrl(pi->pubpi->phy_rev);
				RadarFifoCtrlReg[1] = 0;
				RadarFifoDataReg[0] =
					ACPHY_Antenna0_radarFifoData(pi->pubpi->phy_rev);
				RadarFifoDataReg[1] = 0;
			} else {
			//U80 in 80+80, 1core normal radar
				RadarFifoCtrlReg[0] =
					ACPHY_Antenna1_radarFifoCtrl(pi->pubpi->phy_rev);
				RadarFifoCtrlReg[1] = 0;
				RadarFifoDataReg[0] =
					ACPHY_Antenna1_radarFifoData(pi->pubpi->phy_rev);
				RadarFifoDataReg[1] = 0;
			}
		}
		if (TONEDETECTION) {
			FMOffset = 256;
		}
	} else {
		RadarFifoCtrlReg[0] = NPHY_Antenna0_radarFifoCtrl;
		RadarFifoCtrlReg[1] = NPHY_Antenna1_radarFifoCtrl;
		RadarFifoDataReg[0] = NPHY_Antenna0_radarFifoData;
		RadarFifoDataReg[1] = NPHY_Antenna1_radarFifoData;
	}

	if (!PHY_RADAR_FIFO_SUBBAND_FORMAT(pi)) {
		if (IS20MHZ(pi)) {
			time_shift = 0;
		} else if (IS40MHZ(pi)) {
			time_shift = 1;
		} else {
			time_shift = 2;
		}
	}

	for (core = 0; core < ANT_num; core++) {
		if (ACMAJORREV_47_130(pi->pubpi->phy_rev) && (sec_pll == TRUE)) /* 43684 */
			rt->length_input[core] =
				phy_utils_read_phyreg_p1c(pi, RadarFifoCtrlReg[core]) & 0x3ff;
		else /* 4365/66 */
			rt->length_input[core] =
				phy_utils_read_phyreg(pi, RadarFifoCtrlReg[core]) & 0x3ff;

		if (rt->length_input[core] > MAX_FIFO_SIZE) {
			PHY_RADAR(("FIFO LENGTH in ant %d is greater than max_fifo_size of %d\n",
			           core, MAX_FIFO_SIZE));
			rt->length_input[core] = 0;
		}

		/* enable pulses received at each antenna messages if feature_mask bit-2 is set */
		if (st->rparams.radar_args.feature_mask & RADAR_FEATURE_DEBUG_PULSES_PER_ANT &&
		    rt->length_input[core] > 5) {
			PHY_RADAR(("ant %d:%d\n", core, rt->length_input[core]));
		}
		if (PHY_RADAR_FIFO_SUBBAND_FORMAT(pi)) {
			rt->length_input[core] /= 6; /* 6 words per pulse */
		} else {
			rt->length_input[core] /= 4; /* 4 words per pulse */
		}

		/* use the last sample for bin5 */
		rt->pulses_bin5[core][0] = rlpt->pulse_tail[core];

		for (i = 0; i < rt->length_input[core]; i++) {
			ASSERT(i < RDR_LIST_SIZE);
			if (ACMAJORREV_47_130(pi->pubpi->phy_rev) && (sec_pll == TRUE)) {
				w0 = phy_utils_read_phyreg_p1c(pi, RadarFifoDataReg[core]);
				w1 = phy_utils_read_phyreg_p1c(pi, RadarFifoDataReg[core]);
				w2 = phy_utils_read_phyreg_p1c(pi, RadarFifoDataReg[core]);
				w3 = phy_utils_read_phyreg_p1c(pi, RadarFifoDataReg[core]);
			} else {
				w0 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
				w1 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
				w2 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
				w3 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
			}
			if (PHY_RADAR_FIFO_SUBBAND_FORMAT(pi)) {
				if (ACMAJORREV_47_130(pi->pubpi->phy_rev) && (sec_pll == TRUE)) {
					w4 = phy_utils_read_phyreg_p1c(pi, RadarFifoDataReg[core]);
					w5 = phy_utils_read_phyreg_p1c(pi, RadarFifoDataReg[core]);
				} else {
					w4 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
					w5 = phy_utils_read_phyreg(pi, RadarFifoDataReg[core]);
				}

				/* extract the timestamp, pw, chirp, notradar from the 6 words */
				rt->pulses_input[core][i].interval = (uint32)((w0 << 16) +
						((w1 & 0x0fff) << 4) + (w3 & 0xf));
				if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
					!ACMAJORREV_128(pi->pubpi->phy_rev)) {
					rt->pulses_input[core][i].pw = (((w3 & 0x10) << 8) +
						((w2 & 0x00ff) << 4) + ((w1 >> 12) & 0x000f)) +
						((w3 & 0x8000) << 13);
				} else {
					rt->pulses_input[core][i].pw = (((w3 & 0x10) << 8) +
						((w2 & 0x00ff) << 4) + ((w1 >> 12) & 0x000f));
				}
				rt->pulses_input[core][i].fm =
					((w3 & 0x20) << 3) + ((w2 >> 8) & 0x00ff) - FMOffset;

				rt->pulses_input[core][i].fc = (int16)(w4 & 0x01ff);
				if (rt->pulses_input[core][i].fc > 256) {
					rt->pulses_input[core][i].fc =
						rt->pulses_input[core][i].fc-512;
				}
				rt->pulses_input[core][i].chirp = ((w3 & 0xffc0) >> 6);
				rt->pulses_input[core][i].notradar = ((w5 & 0xfff8) >> 3);

			} else {
				/* extract the timestamp and pulse widths from the 4 words */
				rt->pulses_input[core][i].interval =
					(uint32)(((w0 << 16) + ((w1 & 0x0fff) << 4) + (w3 & 0xf))
					>> time_shift);
				rt->pulses_input[core][i].pw =
					(((w3 & 0x10) << 8) +
					((w2 & 0x00ff) << 4) + ((w1 >> 12) & 0x000f))
					>> time_shift;
				rt->pulses_input[core][i].fm =
					((w3 & 0x20) << 3) + ((w2 >> 8) & 0x00ff) - FMOffset;
				rt->pulses_input[core][i].fc = 0;
				rt->pulses_input[core][i].chirp = 0;
				rt->pulses_input[core][i].notradar = 0;
			}

			rt->pulses_bin5[core][i + 1] =
				rt->pulses_input[core][i];
		}

		/* save the last (tail) sample */
		rlpt->pulse_tail[core] = rt->pulses_bin5[core][i+1];
		rt->length_bin5[core] = rt->length_input[core] + 1;
	}
}

static bool
wlc_phy_radar_valid(const phy_info_t *pi, uint16 feature_mask, uint8 radar_type, bool sec_pll)
{
	bool uk_new_valid = FALSE;
	bool new_jp_valid = FALSE;
	int32 radio_chanspec_sc, chanspec;

	phy_ac_chanmgr_get_val_sc_chspec(PHY_AC_CHANMGR(pi), &radio_chanspec_sc);
	chanspec = sec_pll ? radio_chanspec_sc : pi->radio_chanspec;

	if ((CHSPEC_CHANNEL(chanspec) == 138) ||
		(CHSPEC_CHANNEL(chanspec) == 142) || (CHSPEC_CHANNEL(chanspec) == 144))
		uk_new_valid = TRUE;

	if (CHSPEC_CHANNEL(chanspec) <= 64)
		new_jp_valid = TRUE;

	switch (radar_type) {
	case RADAR_TYPE_NONE:
	case RADAR_TYPE_UNCLASSIFIED:
		return FALSE;

	case RADAR_TYPE_ETSI_1:
	case RADAR_TYPE_ETSI_2:
		return (((feature_mask & RADAR_FEATURE_ETSI_DETECT) != 0) ||
			(new_jp_valid && ((feature_mask & RADAR_FEATURE_NEWJP_DETECT) != 0)));
	case RADAR_TYPE_ETSI_0:
	case RADAR_TYPE_ETSI_3:
	case RADAR_TYPE_ETSI_4:
		return ((feature_mask & RADAR_FEATURE_ETSI_DETECT) != 0);
	case RADAR_TYPE_UK1:
	case RADAR_TYPE_UK2:
		return (uk_new_valid && ((feature_mask & RADAR_FEATURE_UK_DETECT) != 0));

	default:
		;
	}

	return (feature_mask & RADAR_FEATURE_FCC_DETECT) ? TRUE : FALSE;
}

static const radar_spec_t *
wlc_phy_radar_detect_match(phy_info_t *pi, radar_params_t *rparams, const pulse_data_t *pulse,
	uint16 feature_mask, bool sec_pll)
{
	const radar_spec_t *pRadar;
	const pulse_data_t *next_pulse = pulse + 1;
	int intv_tol = (rparams->radar_args.quant != INTERVAL_TOLERANCE) ?
		rparams->radar_args.quant : INTERVAL_TOLERANCE;

	/* scan the list of Radars for a possible match */
	for (pRadar = &radar_spec[0]; pRadar->radar_type != RADAR_TYPE_NONE; ++pRadar) {

		/* skip Radars not in our region */
		if (!wlc_phy_radar_valid(pi, feature_mask, pRadar->radar_type, sec_pll))
			continue;

		/* need consistent intervals */
		if (ABS((int32)(pulse->interval - next_pulse->interval)) <
		    intv_tol + (int32)pulse->interval / 4000) {
			uint16 max_pw = pRadar->max_pw + pRadar->max_pw/10 + PULSE_WIDTH_TOLERANCE;
			uint16 min_pw = (pRadar->min_pw > 100) ? 70 : rparams->radar_args.min_pw;
			/* check against current radar specifications */

			/* Note: min_pw is currently not used due to a hardware fault causing
			 *       some valid pulses to be spikes of very small widths.  We need to
			 *       ignore the min_pw otherwise we could miss some valid Radar signals
			 */
			if (pulse->pw <= max_pw && next_pulse->pw <= max_pw &&
			    pulse->pw >= min_pw && next_pulse->pw >= min_pw &&
			    pulse->interval >=
			    (uint32)pRadar->min_pri * 20 - pRadar->min_pri/9 - intv_tol &&
			    pulse->interval <=
			    (uint32)pRadar->max_pri * 20 + pRadar->max_pri/9 + intv_tol) {
				/* match found */
				break;
			}
		}
	}

	return pRadar;	/* points to an entry in radar_spec[] */
}

/* remove outliers from a list */
static int
wlc_phy_radar_filter_list(pulse_data_t inlist[], int length, uint32 min_val, uint32 max_val)
{
	int i, j;
	j = 0;
	for (i = 0; i < length; i++) {
		if ((inlist[i].interval >= min_val) && (inlist[i].interval <= max_val)) {
			inlist[j] = inlist[i];
			j++;
		}
	}
	return j;
}

static void
wlc_phy_radar_fc_chirp_cmb(pulse_data_t *input0, pulse_data_t *input1)
{
	int16 l_fc = 0;
	int16 s_fc = 0;
	uint16 l_pw = 0;
	uint16 s_pw = 0;
	if (input0->pw > input1->pw) {
		l_fc = input0->fc;
		l_pw = input0->pw;
		s_fc = input1->fc;
		s_pw = input1->pw;
	} else {
		l_fc = input1->fc;
		l_pw = input1->pw;
		s_fc = input0->fc;
		s_pw = input0->pw;
	}
	if (l_pw > (s_pw << 4)) {
		input0->fc = ((l_fc << 4)-l_fc+s_fc) >> 4;
	} else if (l_pw > (s_pw << 3)) {
		input0->fc = ((l_fc << 3)-l_fc+s_fc) >> 3;
	} else if (l_pw > (s_pw << 2)) {
		input0->fc = ((l_fc << 2)-l_fc+s_fc) >> 2;
	} else {
		input0->fc = (l_fc+s_fc) >> 1;
	}
	/* Combine chirp */
	input0->chirp = input0->chirp + input1->chirp;
	/* Combine notradar */
	input0->notradar = (input0->notradar > input1->notradar ?
			input0->notradar : input1->notradar);
}

static uint16
wlc_phy_radar_fc_chirp_2_subband(const phy_info_t *pi, int fc,
	uint chirp, bool sec_pll, uint fcc5)
{
	uint16 subband_result = 0;
	uint16 subband_num = 1;
	uint16 subband_idx = 0;
	uint16 subband_weighting = 1;
	int fc_tol = 0;
	int32 radio_chanspec_sc, chanspec;

	phy_ac_chanmgr_get_val_sc_chspec(PHY_AC_CHANMGR(pi), &radio_chanspec_sc);

	chanspec = sec_pll ? radio_chanspec_sc : pi->radio_chanspec;
	/* prohibit subband radar detection on non-DFS channels */
	if ((CHSPEC_CHANNEL(chanspec) == 50) && (fc < -1))
		return 0;

	if (CHSPEC_IS20(chanspec)) {
		subband_result = ALLSUBBAND_BW20;
		return subband_result;
	} else if (CHSPEC_IS40(chanspec)) {
		subband_num = SUBBAND_NUM_BW40;
	} else if (CHSPEC_IS80(chanspec)) {
		subband_num = SUBBAND_NUM_BW80;
	} else if (CHSPEC_IS160(chanspec)) {
		subband_num = SUBBAND_NUM_BW160;
	}

	if (fcc5 == 0) {
		fc_tol = FC_TOL_SB;
	} else {
		fc_tol = FC_TOL_BIN5_SB;
	}

	subband_weighting = subband_weighting << (subband_num - 1);
	for (subband_idx = 0; subband_idx < subband_num; subband_idx++) {
		if ((fc + (int)(chirp/2) + fc_tol) >=
			(((int)(0 - subband_num/2 + subband_idx))*SUBBAND_BW) &&
			(fc - (int)(chirp/2) - fc_tol) <=
			(((int)(1 - subband_num/2 + subband_idx))*SUBBAND_BW)) {
			subband_result = subband_result + (subband_weighting >> subband_idx);
		}
	}
	/* subband results should be zero on non-DFS channels */
	if (CHSPEC_CHANNEL(chanspec) == 50)
		subband_result = subband_result & 0x0f;

	return subband_result;
}

static int phy_radar_check_newJP(phy_info_t *pi, radar_params_t *rparams, int *nconseq,
	const pulse_data_t pulse[], uint16 j)
{
	const radar_specNewJP_t *pRadar;
	int min_curr_pw, max_curr_pw, min_prev_pw, max_prev_pw;
	int curr_pw_tol, prev_pw_tol, curr_pri;

	for (pRadar = &radar_specNewJP[0]; pRadar->radar_type != RADAR_TYPE_NONE; ++pRadar) {
		if ((int)pulse[j].pw <= 350) { /* New JP Short Pulses Detection, pw <= 17.5us */
			min_curr_pw = pRadar->min_pw_sp;
			max_curr_pw = pRadar->max_pw_sp;
			min_prev_pw = pRadar->min_pw_lp;
			max_prev_pw = pRadar->max_pw_lp;
			curr_pw_tol = PULSE_WIDTH_TOLERANCE;
			prev_pw_tol = 65;
		} else { /* New JP Long Pulses Detection, pw > 17.5us */
			min_curr_pw = pRadar->min_pw_lp;
			max_curr_pw = pRadar->max_pw_lp;
			min_prev_pw = pRadar->min_pw_sp;
			max_prev_pw = pRadar->max_pw_sp;
			curr_pw_tol = 65;
			prev_pw_tol = PULSE_WIDTH_TOLERANCE;
		}

		curr_pri = (int)pulse[j].interval + (int)pulse[j].pw + (int)pulse[j - 1].interval +
			(int)pulse[j - 1].pw;

		if (ABS((int32)(pulse[j].interval - pulse[j - 2].interval)) <= INTERVAL_TOLERANCE &&
			ABS((int32)(pulse[j - 1].interval - pulse[j - 3].interval)) <=
			INTERVAL_TOLERANCE && (int)pulse[j].pw >= min_curr_pw - curr_pw_tol &&
			(int)pulse[j].pw <= max_curr_pw + curr_pw_tol &&
			(int)pulse[j - 1].pw >= min_prev_pw - prev_pw_tol &&
			(int)pulse[j - 1].pw <= max_prev_pw + prev_pw_tol &&
			(int)pulse[j].interval >= pRadar->min_blk - INTERVAL_TOLERANCE &&
			(int)pulse[j - 1].interval >= pRadar->min_blk - INTERVAL_TOLERANCE &&
			curr_pri >= pRadar->min_pri - pRadar->pri_tol &&
			curr_pri <= pRadar->max_pri + pRadar->pri_tol &&
			ABS((int32)(pulse[j].pw - pulse[j - 1].pw)) >= 300) {
			++(*nconseq);
			break;
		}
	}

	return pRadar->radar_type;
}

#define MIN_STAGGERED_INTERVAL		16667	/* 833 us */
#define MAX_STAGGERED_INTERVAL		66667	/* 3333 us */
#define MAX_STAGGERED_PULSE_WIDTH	40	/* 2 us */
#define	MAX_STAGGERED_ETSI_6_INTERVAL	50000	/* 2500 us */

static void phy_radar_check_staggered(phy_info_t *pi, radar_params_t *rparams, int *nconseq,
	const pulse_data_t pulse[], uint16 j, int stg, bool sec_pll)
{
	int intv_tol = (rparams->radar_args.quant != INTERVAL_TOLERANCE) ?
		rparams->radar_args.quant : INTERVAL_TOLERANCE;
	int pw_tol = (ACMAJORREV_130(pi->pubpi->phy_rev) && sec_pll) ?
		PULSE_WIDTH_TOLERANCE_6715 : PULSE_WIDTH_TOLERANCE;
	int lesi_rev47 = ACMAJORREV_47(pi->pubpi->phy_rev) ? TRUE : FALSE;

	if (ABS((int32)(pulse[j].interval - pulse[j - stg].interval)) <= intv_tol &&
	    ABS((int32)(pulse[j - stg + 1].interval - pulse[j - stg].interval)) >=
	    ((rparams->radar_args.autocorr >> 5) & 0x7ff) &&
	    (int)pulse[j].pw <= MAX_STAGGERED_PULSE_WIDTH + pw_tol &&
	    (int)pulse[j].interval >= MIN_STAGGERED_INTERVAL - intv_tol &&
	    (int)pulse[j].interval <= MAX_STAGGERED_INTERVAL + intv_tol) {
		int k;
		uint16 curr_pw = pulse[j].pw;
		uint16 min_pw = curr_pw;
		uint16 max_pw = curr_pw;

		++(*nconseq);

		/* check all alternate pulses for pulse width tolerance */
		for (k = 0; k < *nconseq; k++) {
			curr_pw = pulse[j - stg * k].pw;
			if (curr_pw < min_pw) {
				min_pw = curr_pw;
			}
			if (curr_pw > max_pw) {
				max_pw = curr_pw;
			}
		}
		if (max_pw - min_pw > ((rparams->radar_args.max_pw_tol != PULSE_WIDTH_DIFFERENCE) ?
			rparams->radar_args.max_pw_tol : ((lesi_rev47 ?
			PULSE_WIDTH_DIFFERENCE_STG_LESI :
			PULSE_WIDTH_DIFFERENCE) + curr_pw / 200))) {
			/* alternate pulse widths inconsistent */
			*nconseq = 0;
		}
	} else {
		/* alternate intervals inconsistent */
		*nconseq = 0;
	}
}

static void phy_radar_check_staggered2(phy_info_t *pi, radar_params_t *rparams, int *nconseq,
	const pulse_data_t pulse[], uint16 j, bool sec_pll)
{
	phy_radar_check_staggered(pi, rparams, nconseq, pulse, j, 2, sec_pll);
}

static void phy_radar_check_staggered3(phy_info_t *pi, radar_params_t *rparams, int *nconseq,
	const pulse_data_t pulse[], uint16 j, bool sec_pll)
{
	phy_radar_check_staggered(pi, rparams, nconseq, pulse, j, 3, sec_pll);
}

#define EXT_FM_CHK_DBG 0x1
#define EXT_FM_ABS_CHK_SP 0x2
#define EXT_FM_VAR_CHK_SP 0x4
#define EXT_FM_ABS_CHK_ETSI4 0x8

static bool phy_radar_ext_fm_check(phy_info_t *pi, radar_params_t *rparams, pulse_data_t pulses[],
	uint8 det_type, uint8 previous_det_type, uint16 j, int nconsecq_pulses, int min_det_pw,
	bool sec_pll)
{
	int k;
	int fm_dif;
	int fm_tol;
	int fm_min = 1030;
	int fm_max = 0;
	bool fm_chk = TRUE;
	wl_radar_args_t *radar_args = &rparams->radar_args;
	wl_radar_thr2_t *radar_thrs2 = &rparams->radar_thrs2;
	int32 radio_chanspec_sc;
	bool is43684mch2;
	int etsi4_fm_thresh = MIN_FM_THRESH_ETSI4;
	int etsi4_fm_thresh_args = ((radar_args->fra_pulse_err >> 8) & 0xff) -
		((rparams->radar_args.ncontig >> 6) & 0xf);
	int pw_thrs_fm_var_chk = (radar_thrs2->fm_chk_opt & EXT_FM_CHK_DBG) ?
		radar_thrs2->fm_var_chk_pw : 25;
	int div_fm_tol = (radar_thrs2->fm_chk_opt & EXT_FM_CHK_DBG) ?
		radar_thrs2->fm_tol_div : 5;

	phy_ac_chanmgr_get_val_sc_chspec(PHY_AC_CHANMGR(pi), &radio_chanspec_sc);

	is43684mch2 = ACMAJORREV_47(pi->pubpi->phy_rev) && CHSPEC_IS2G(pi->radio_chanspec) &&
		CHSPEC_ISPHY5G6G(radio_chanspec_sc) && !BF_ELNA_5G(pi->u.pi_acphy);

	/* special case for ETSI 4, as it has a chirp of +/-2.5MHz */
	if (det_type == RADAR_TYPE_ETSI_4) {
		if (radar_thrs2->fm_chk_opt & EXT_FM_CHK_DBG) {
			etsi4_fm_thresh = radar_thrs2->fm_thresh_etsi4;
		} else if (etsi4_fm_thresh_args != MIN_FM_THRESH_ETSI4) {
			etsi4_fm_thresh = etsi4_fm_thresh_args;
		} else if (ACMAJORREV_51_131(pi->pubpi->phy_rev) &&
			(CHSPEC_IS80(pi->radio_chanspec) && !(CHSPEC_CHANNEL(pi->radio_chanspec) <=
			WL_THRESHOLD_LO_BAND))) {
			etsi4_fm_thresh = 100;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			etsi4_fm_thresh = CHSPEC_IS20(pi->radio_chanspec) ? 170 :
				CHSPEC_IS40(pi->radio_chanspec) ? 240 : 160;
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev) ||
			ACMAJORREV_4(pi->pubpi->phy_rev)) {
			/* 4360 or 53573/47189 */
			etsi4_fm_thresh = 50;
		} else if (sec_pll == TRUE) {
			etsi4_fm_thresh = is43684mch2 ? 0 : 50;
		}
		/* not a valid ETSI 4 if fm doesn't vary */
		for (k = 0; k < nconsecq_pulses; k++) {
			if (ISACPHY(pi) && TONEDETECTION) {
				if (radar_thrs2->fm_chk_opt & EXT_FM_ABS_CHK_ETSI4) {
					if (pulses[j - k].fm < etsi4_fm_thresh) {
						fm_chk = FALSE;
						if ((radar_args->feature_mask &
							RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0) {
							PHY_RADAR(("\nShort pulses fail ETSI-4"
							" absolute fm value check: fm=%d "
							"< etsi4_fm_thresh=%d\n",
							pulses[j - k].fm, etsi4_fm_thresh));
						}
						break;
					}
				} else {
					break;
				}
			} else {
				if (pulses[j - k].fm < fm_min) {
					fm_min = pulses[j - k].fm;
				}
				if (pulses[j - k].fm > fm_max) {
					fm_max = pulses[j - k].fm;
				}
				fm_dif = ABS(fm_max - fm_min);
				fm_tol = (fm_min + fm_max) / 4;
				if (fm_dif > fm_tol) {
					fm_chk = FALSE;
					break;
				}
			}
		}
	} else if ((radar_thrs2->fm_chk_opt & EXT_FM_VAR_CHK_SP) &&
		(min_det_pw >= pw_thrs_fm_var_chk) && (sec_pll == FALSE)) {
		for (k = 0; k < nconsecq_pulses; k++) {
			if (pulses[j - k].fm < fm_min) {
				fm_min = pulses[j - k].fm;
			}
			if (pulses[j - k].fm > fm_max) {
				fm_max = pulses[j - k].fm;
			}
			fm_dif = ABS(fm_max - fm_min);
			fm_tol = (fm_min + fm_max) / div_fm_tol;
			if (fm_dif > fm_tol) {
				fm_chk = FALSE;
				if ((radar_args->feature_mask &
					RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0) {
					PHY_RADAR(("\nShort pulses fail fm"
					" variation check"));
				}
				break;
			}
		}
	}
	return fm_chk;
}

#define EXT_PW_TOL_SET1 0x1
#define EXT_PW_TOL_SET2 0x2
#define EXT_PW_TOL_SET3 0x4
#define EXT_PW_TOL_SET4 0x8
#define EXT_PW_TOL_SETALL 0Xf

static bool phy_radar_ext_pw_tol(phy_info_t *pi, radar_params_t *rparams, int max_det_pw,
	int min_det_pw, uint16 blank_time, uint16 set)
{
	bool chk_set1, chk_set2, chk_set3, chk_set4;
	int max_pw_tol = (rparams->radar_args.max_pw_tol != PULSE_WIDTH_DIFFERENCE) ?
		rparams->radar_args.max_pw_tol : PULSE_WIDTH_DIFFERENCE;
	int lesi_rev47 = ACMAJORREV_47(pi->pubpi->phy_rev) ? TRUE : FALSE;
	int lesi_rev47_130 = ACMAJORREV_47_130(pi->pubpi->phy_rev) ? TRUE : FALSE;
	int lesi_rev128 = ACMAJORREV_128(pi->pubpi->phy_rev) ? TRUE : FALSE;
	/* ext pw tol for pw variations <= 37(1.85us)  */
	chk_set1 = ((set & EXT_PW_TOL_SET1) && (((max_det_pw >= 30) && (max_det_pw <= (lesi_rev47 ?
		60 : 50))) && ((max_det_pw - min_det_pw) <= (max_pw_tol + blank_time)))) ?
		TRUE : FALSE;
	/* ext pw tol for pw variations >= 19(0.95us) and pw variations <= 31(1.55us) */
	chk_set2 = ((set & EXT_PW_TOL_SET2) && (((max_det_pw > 50) && (max_det_pw <= 430)) &&
		(((max_det_pw - min_det_pw)  >= (blank_time - ((lesi_rev47_130 || lesi_rev128) ?
		max_pw_tol : max_pw_tol/2))) && ((max_det_pw - min_det_pw) <= (blank_time +
		((lesi_rev47_130 || lesi_rev128) ? max_pw_tol : max_pw_tol/2)))))) ? TRUE : FALSE;
	/* ext pw tol for pw variations >= 44(2.2us) and pw variations <= 56(2.8us) */
	chk_set3 = ((set & EXT_PW_TOL_SET3) && (((max_det_pw > 80) && (max_det_pw <= 430)) &&
		(((max_det_pw - min_det_pw)  >= (2*blank_time - (lesi_rev128 ? max_pw_tol :
		max_pw_tol/2))) && ((max_det_pw - min_det_pw) <= (2*blank_time + (lesi_rev128 ?
		max_pw_tol : max_pw_tol/2)))))) ? TRUE : FALSE;
	/* ext pw tol for pw variations >= 69(3.45us) and pw variations <= 81(4.05us) */
	chk_set4 = ((set & EXT_PW_TOL_SET4) && (((max_det_pw > 200) && (max_det_pw <= 430)) &&
		(((max_det_pw - min_det_pw)  >= (3*blank_time - (max_pw_tol/2))) &&
		((max_det_pw - min_det_pw) <= (3*blank_time + (max_pw_tol/2)))))) ?
		TRUE : FALSE;

	return (chk_set1 || chk_set2 || chk_set3 || chk_set4);
}

#define MIN_INTERVAL	3000	/* 150 us */
#define MAX_INTERVAL	120000	/* 6 ms */
#define MIN_INTERVAL_NEWJP 1100	/* 55 us */
#define MIN_CHIRP	0	/* 0 MHz */
#define MAX_CHIRP	80	/* 40 MHz */
#define MIN_FC	-160	/* -80 MHz */
#define MAX_FC	160	/* 80 MHz */
#define MAX_FC_VARIATION	7

static void
get_min_max_avg_statistic(pulse_data_t pulses[], radar_params_t *rparams, int idx,
	int num_pulses, uint32 *min_detected_interval_p, uint32 *max_detected_interval_p,
	int *avg_fc_p, int *var_fc_p)
{
	int i;
	uint32 min_interval = MAX_INTERVAL;
	uint32 max_interval = (rparams->radar_args.feature_mask &
		RADAR_FEATURE_NEWJP_DETECT) ? MIN_INTERVAL_NEWJP : MIN_INTERVAL;
	uint32 interval;
	uint min_chirp = MAX_CHIRP;
	uint max_chirp = MIN_CHIRP;
	uint chirp;
	int min_fc = MAX_FC;
	int max_fc = MIN_FC;
	int fc;
	int sum_detected_fc = 0;

	for (i = idx; i < idx + num_pulses; ++i) {
		ASSERT(i < RDR_LIST_SIZE);
		interval = pulses[i].interval;
		chirp = pulses[i].chirp;
		fc = pulses[i].fc;
		if (interval < min_interval) {
			min_interval = interval;
		}
		if (interval > max_interval) {
			max_interval = interval;
		}
		if (chirp < min_chirp) {
			min_chirp = chirp;
		}
		if (chirp > max_chirp) {
			max_chirp = chirp;
		}
		if (fc < min_fc) {
			min_fc = fc;
		}
		if (fc > max_fc) {
			max_fc = fc;
		}
		sum_detected_fc = sum_detected_fc + fc;
	}

	*min_detected_interval_p = min_interval;
	*max_detected_interval_p = max_interval;
	*avg_fc_p = sum_detected_fc / num_pulses;
	if ((max_fc - *avg_fc_p) >
		(*avg_fc_p - min_fc)) {
		*var_fc_p = *avg_fc_p - min_fc;
	} else {
		*var_fc_p = max_fc - *avg_fc_p;
	}
}

static void
wlc_phy_radar_detect_run_epoch(phy_info_t *pi,
	radar_work_t *rt, radar_params_t *rparams,
	int epoch_length,
	uint8 *det_type_p, int *nconsecq_pulses_p,
	int *detected_pulse_index_p, int *min_detected_pw_p, int *max_detected_pw_p,
	uint32 *min_detected_interval_p, uint32 *max_detected_interval_p, int *avg_fc_p,
	int *var_fc_p, uint16 blank_time, bool sec_pll)
{
	uint16 j;
	bool radar_detected = FALSE, fm_in_range = TRUE;
	uint8 det_type;
	uint8 det_type_newjp = 0;
	uint8 previous_det_type = RADAR_TYPE_NONE;
	uint32 min_interval = (rparams->radar_args.feature_mask &
		RADAR_FEATURE_NEWJP_DETECT) ? MIN_INTERVAL_NEWJP : MIN_INTERVAL;
	int detected_pulse_index = 0;
	int nconsecq_pulses = 0;
	int nconseq2even, nconseq2odd;
	int nconseq3a, nconseq3b, nconseq3c;
	int nconnewjp, npulse_newjp;
	int first_interval;
	bool pw_blank_condition;
	const radar_spec_t *pRadar;
	int pw_var_tol;
	int32 radio_chanspec_sc, chanspec;

	phy_ac_chanmgr_get_val_sc_chspec(PHY_AC_CHANMGR(pi), &radio_chanspec_sc);
	chanspec = sec_pll ? radio_chanspec_sc : pi->radio_chanspec;

	epoch_length = wlc_phy_radar_filter_list(rt->pulses, epoch_length,
	                                         min_interval, MAX_INTERVAL);

	/* Detect contiguous only pulses */
	detected_pulse_index = 0;
	nconsecq_pulses = 0;
	nconseq2even = 0;
	nconseq2odd = 0;
	nconseq3a = 0;
	nconseq3b = 0;
	nconseq3c = 0;
	nconnewjp = 0;

	first_interval = rt->pulses[0].interval;
	*min_detected_pw_p = rt->pulses[0].pw;
	*max_detected_pw_p = *min_detected_pw_p;

	for (j = 0; j < epoch_length - 1; j++) {
		uint16 curr_pw = rt->pulses[j].pw;
		uint32 curr_interval = rt->pulses[j].interval;

		/* contiguous pulse detection */
		pRadar = wlc_phy_radar_detect_match(pi, rparams, &rt->pulses[j],
		                                    rparams->radar_args.feature_mask, sec_pll);
		det_type = pRadar->radar_type;

		if (det_type != RADAR_TYPE_NONE) {
			/* same interval detected as first ? */
			if (ABS((int32)(curr_interval - first_interval)) <
			    (rparams->radar_args.quant != INTERVAL_TOLERANCE) ?
			    rparams->radar_args.quant : (INTERVAL_TOLERANCE +
			    (int32)curr_interval / 4000)) {
				if (curr_pw < *min_detected_pw_p) {
					*min_detected_pw_p = curr_pw;
				}
				if (curr_pw > *max_detected_pw_p) {
					*max_detected_pw_p = curr_pw;
				}
				if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
					ACMAJORREV_33(pi->pubpi->phy_rev)) {
					if (PHY_LESI_ON(pi)) {
						pw_blank_condition = phy_radar_ext_pw_tol(
						pi, rparams, *max_detected_pw_p, *min_detected_pw_p,
						blank_time, EXT_PW_TOL_SETALL);
					} else {
						pw_blank_condition = phy_radar_ext_pw_tol(
						pi, rparams, *max_detected_pw_p, *min_detected_pw_p,
						blank_time, EXT_PW_TOL_SET1);
					}
					pw_var_tol = PULSE_WIDTH_DIFFERENCE + curr_pw / 200;
				} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
					!ACMAJORREV_128(pi->pubpi->phy_rev)) {
					pw_blank_condition = phy_radar_ext_pw_tol(
					pi, rparams, *max_detected_pw_p, *min_detected_pw_p,
					blank_time, (EXT_PW_TOL_SET1 | EXT_PW_TOL_SET2));
					pw_var_tol = ((det_type == RADAR_TYPE_ETSI_4) ?
						PULSE_WIDTH_DIFFERENCE_ETSI4 :
						PULSE_WIDTH_DIFFERENCE) + curr_pw / 200;
				} else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
					pw_blank_condition = phy_radar_ext_pw_tol(
					pi, rparams, *max_detected_pw_p, *min_detected_pw_p,
					blank_time, ((rparams->radar_args.autocorr & 0x1) ?
					((rparams->radar_args.autocorr >> 1) & 0xf) :
					EXT_PW_TOL_SETALL));
					pw_var_tol = PULSE_WIDTH_DIFFERENCE + curr_pw / 200;
				} else {
					pw_blank_condition = FALSE;
					pw_var_tol = PULSE_WIDTH_DIFFERENCE + curr_pw / 200;
				}

				if ((*max_detected_pw_p - *min_detected_pw_p <=
				    ((rparams->radar_args.max_pw_tol != PULSE_WIDTH_DIFFERENCE) ?
				    rparams->radar_args.max_pw_tol : pw_var_tol)) ||
				    pw_blank_condition) {
					uint8 min_pulses;
					if ((ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
					    !ACMAJORREV_128(pi->pubpi->phy_rev)) &&
					    ((rparams->radar_args.max_deltat & 0x1) == 1)) {
					    min_pulses = (det_type == RADAR_TYPE_FCC_1 ||
					      det_type == RADAR_TYPE_JP4 ||
					      det_type == RADAR_TYPE_JP2_1_1) ?
					      ((rparams->radar_args.ncontig >> 13) & 0x7) - 1 :
					      (det_type == RADAR_TYPE_JP1_2) ?
					      ((rparams->radar_args.ncontig >> 10) & 0x7) - 1 :
					      (det_type == RADAR_TYPE_ETSI_1) ?
					      (rparams->radar_args.npulses_fra & 0xf) - 1 :
					      (det_type == RADAR_TYPE_ETSI_2) ?
					      ((rparams->radar_args.npulses_fra >> 4) & 0xf) - 1 :
					      (det_type == RADAR_TYPE_ETSI_3 ||
					      det_type == RADAR_TYPE_ETSI_4) ?
					      ((rparams->radar_args.npulses_fra >> 8) & 0xf) - 1 :
					      rparams->radar_args.npulses - 1;
					} else {
					    min_pulses = MIN(MIN_PULSES, pRadar->num_pulses);
					}
					if (++nconsecq_pulses >= min_pulses) {
						fm_in_range = phy_radar_ext_fm_check(pi, rparams,
						rt->pulses, det_type, previous_det_type, j,
						nconsecq_pulses, *min_detected_pw_p, sec_pll);
						/* requirements match a Radar */
						radar_detected = TRUE;
						*nconsecq_pulses_p = nconsecq_pulses;
						detected_pulse_index = j - min_pulses + 1;
						get_min_max_avg_statistic(rt->pulses, rparams,
								detected_pulse_index, min_pulses,
								min_detected_interval_p,
								max_detected_interval_p,
								avg_fc_p,
								var_fc_p);

						if (rparams->radar_args.feature_mask &
							RADAR_FEATURE_NEWJP_DETECT) {
							if (det_type == RADAR_TYPE_ETSI_1)
								det_type = RADAR_TYPE_NEW_JP_1;
							else if (det_type == RADAR_TYPE_ETSI_2)
								det_type = RADAR_TYPE_NEW_JP_2;
						}

						PHY_RADAR(("Radar %d: detected_pulse_index=%d\n",
							det_type, detected_pulse_index));
						break;
					}
				} else {
					/* reset to current pulse */
					first_interval = rt->pulses[j].interval;
					*min_detected_pw_p = curr_pw;
					*max_detected_pw_p = curr_pw;
					previous_det_type = det_type;
					nconsecq_pulses = 0;
				}
			} else {
				/* reset to current pulse */
				first_interval = rt->pulses[j].interval;
				*min_detected_pw_p = curr_pw;
				*max_detected_pw_p = curr_pw;
				previous_det_type = det_type;
				nconsecq_pulses = 0;
			}
		} else {
			previous_det_type = RADAR_TYPE_NONE;
			first_interval = rt->pulses[j].interval;
			nconsecq_pulses = 0;
		}

		/* new JP radar detection, only support W53 band */
		if ((rparams->radar_args.feature_mask & RADAR_FEATURE_NEWJP_DETECT) &&
			CHSPEC_CHANNEL(chanspec) <= 64) {

			if (j >= 3) {
				det_type_newjp = phy_radar_check_newJP(pi, rparams, &nconnewjp,
					rt->pulses, j);
			}

			npulse_newjp = (rparams->radar_args.npulses_fra >> 12) & 0xf;

			if (nconnewjp >= npulse_newjp) {
				radar_detected = TRUE;
				detected_pulse_index = j - npulse_newjp + 1;
				if (detected_pulse_index < 0)
					detected_pulse_index = 0;
				det_type = det_type_newjp;
				*min_detected_pw_p = rt->pulses[detected_pulse_index].pw;
				*max_detected_pw_p = *min_detected_pw_p;
				*nconsecq_pulses_p = nconnewjp;
				get_min_max_avg_statistic(rt->pulses, rparams, detected_pulse_index,
					npulse_newjp, min_detected_interval_p,
				max_detected_interval_p, avg_fc_p, var_fc_p);
				break;	/* radar detected */
			}
		}

		/* staggered 2/3 single filters */
		if (rparams->radar_args.feature_mask & RADAR_FEATURE_ETSI_DETECT) {
			/* staggered 2 even */
			if (j >= 2 && j % 2 == 0) {
				phy_radar_check_staggered2(pi, rparams, &nconseq2even,
					rt->pulses, j, sec_pll);
			}

			/* staggered 2 odd */
			if (j >= 3 && j % 2 == 1) {
				phy_radar_check_staggered2(pi, rparams, &nconseq2odd,
					rt->pulses, j, sec_pll);
			}

			if (nconseq2even >= rparams->radar_args.npulses_stg2 / 2 &&
			    nconseq2odd >= rparams->radar_args.npulses_stg2 / 2) {
				radar_detected = TRUE;
				detected_pulse_index = j -
					(rparams->radar_args.npulses_stg2 / 2) * 2 - 1;
				if (detected_pulse_index < 0)
					detected_pulse_index = 0;
				if ((rt->pulses[detected_pulse_index].interval +
				    rt->pulses[detected_pulse_index+1].interval) / 2 >=
				    MAX_STAGGERED_ETSI_6_INTERVAL) {
					det_type = RADAR_TYPE_ETSI_5_STG2;
				} else {
					det_type = RADAR_TYPE_ETSI_6_STG2;
				}
				*min_detected_pw_p = rt->pulses[detected_pulse_index].pw;
				*max_detected_pw_p = *min_detected_pw_p;
				*nconsecq_pulses_p = nconseq2odd + nconseq2even;
				get_min_max_avg_statistic(rt->pulses, rparams, detected_pulse_index,
					rparams->radar_args.npulses_stg2, min_detected_interval_p,
					max_detected_interval_p, avg_fc_p, var_fc_p);
				PHY_RADAR(("j=%d detected_pulse_index=%d\n",
					j, detected_pulse_index));
				break;	/* radar detected */
			}

			/* staggered 3-a */
			if (j >= 3 && j % 3 == 0) {
				phy_radar_check_staggered3(pi, rparams, &nconseq3a, rt->pulses,
					j, sec_pll);
			}

			/* staggered 3-b */
			if (j >= 4 && j % 3 == 1) {
				phy_radar_check_staggered3(pi, rparams, &nconseq3b, rt->pulses,
					j, sec_pll);
			}

			/* staggered 3-c */
			if (j >= 5 && j % 3 == 2) {
				phy_radar_check_staggered3(pi, rparams, &nconseq3c, rt->pulses,
					j, sec_pll);
			}

			if (nconseq3a >= rparams->radar_args.npulses_stg3 / 3 &&
			    nconseq3b >= rparams->radar_args.npulses_stg3 / 3 &&
			    nconseq3c >= rparams->radar_args.npulses_stg3 / 3) {
				radar_detected = TRUE;
				detected_pulse_index = j -
					(rparams->radar_args.npulses_stg3 / 3) * 3 - 2;
				if (detected_pulse_index < 0)
					detected_pulse_index = 0;
				if ((rt->pulses[detected_pulse_index].interval +
				    rt->pulses[detected_pulse_index+1].interval) / 2 >=
				    MAX_STAGGERED_ETSI_6_INTERVAL) {
					det_type = RADAR_TYPE_ETSI_5_STG3;
				} else {
					det_type = RADAR_TYPE_ETSI_6_STG3;
				}
				*min_detected_pw_p = rt->pulses[detected_pulse_index].pw;
				*max_detected_pw_p = *min_detected_pw_p;
				*nconsecq_pulses_p = nconseq3a + nconseq3b + nconseq3c;
				get_min_max_avg_statistic(rt->pulses, rparams, detected_pulse_index,
					rparams->radar_args.npulses_stg3, min_detected_interval_p,
					max_detected_interval_p, avg_fc_p, var_fc_p);
				PHY_RADAR(("j=%d detected_pulse_index=%d\n",
					j, detected_pulse_index));
				break;	/* radar detected */
			}
		}
	}  /* for (j = 0; j < epoch_length - 2; j++) */

	if (radar_detected && (fm_in_range == TRUE)) {
		*det_type_p = det_type;
		*detected_pulse_index_p = detected_pulse_index;
	} else {
		*det_type_p = RADAR_TYPE_NONE;
		*min_detected_interval_p = 0;
		*max_detected_interval_p = 0;
		*nconsecq_pulses_p = 0;
		*detected_pulse_index_p = 0;
		*var_fc_p = 0;
	}
}

static void
phy_radar_output_pulses(phy_info_t *pi, pulse_data_t pulses[], uint16 length,
	uint8 feature_mask, bool ntstart)
{
	uint16 i;

	if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_GE47(pi->pubpi->phy_rev)) &&
		!ACMAJORREV_128(pi->pubpi->phy_rev)) {
		if (ntstart) {
			PHY_RADAR(("\ntstart0=[  "));
			for (i = 0; i < length; i++)
				if (feature_mask & RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
					PHY_RADAR(("%u-%d ", pulses[i].interval/20, i));
				} else {
					PHY_RADAR(("%u-%d ", pulses[i].interval, i));
				}
			PHY_RADAR(("];"));
		}

		PHY_RADAR(("\nInterval:  "));
		for (i = 1; i < length; i++)
			if (feature_mask & RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
				PHY_RADAR(("%u-%d ", (pulses[i].interval -
					pulses[i - 1].interval)/20, i));
			} else {
				PHY_RADAR(("%u-%d ", pulses[i].interval -
					pulses[i - 1].interval, i));
			}

		PHY_RADAR(("\nPulse Widths:  "));
		for (i = 0; i < length; i++)
			if (feature_mask & RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
				PHY_RADAR(("%d-%d ", pulses[i].pw/20, i));
			} else {
				PHY_RADAR(("%d-%d ", pulses[i].pw, i));
			}

		PHY_RADAR(("\nFM:  "));
		for (i = 0; i < length; i++)
		  PHY_RADAR(("%d-%d ", pulses[i].fm, i));

		PHY_RADAR(("\nCHIRP:  "));
		for (i = 0; i < length; i++)
		  PHY_RADAR(("%d-%d ", pulses[i].chirp, i));

		PHY_RADAR(("\nNotradar:  "));
		for (i = 0; i < length; i++)
		  PHY_RADAR(("%d-%d ", pulses[i].notradar, i));
		PHY_RADAR(("\n"));
	} else {
		PHY_RADAR(("\ntstamp=[ "));
		for (i = 0; i < length; i++) {
			if (feature_mask & RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
				PHY_RADAR(("%u ", pulses[i].interval/20));
			} else {
				PHY_RADAR(("%u ", pulses[i].interval));
			}
		}
		PHY_RADAR(("];\n"));

		PHY_RADAR(("(Interval, PW, FM, FC, CHIRP, NOTRADAR) = [ "));
		for (i = 1; i < length; i++) {
			if (feature_mask & RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
				PHY_RADAR(("(%u, %u, %d, %d, %u, %u) ",
					(pulses[i].interval - pulses[i - 1].interval)/20,
					pulses[i].pw, pulses[i].fm,
					pulses[i].fc, pulses[i].chirp,
					pulses[i].notradar));
			} else {
				PHY_RADAR(("(%u, %u, %d, %d, %u, %u) ",
					pulses[i].interval - pulses[i - 1].interval,
					pulses[i].pw, pulses[i].fm,
					pulses[i].fc, pulses[i].chirp,
					pulses[i].notradar));
			}
		}
		PHY_RADAR(("];\n"));
	}
}

static void
phy_radar_detect_fcc5(phy_info_t *pi,
                     phy_radar_st_t *st, radar_work_t *rt, bool sec_pll, bool bw80_80_mode,
                     radar_detected_info_t *radar_detected)
{
	uint16 i;
	uint16 j;
	int k;
	uint8 ant;
	int wr_ptr;
	uint16 mlength;
	int32 deltat;
	int skip_type;
	uint16 width;
	int16 fm;
	bool valid_lp;
	uint8 invalid_type;
	int pw_dif, pw_tol, fm_dif, fm_tol;
	int FMCombineOffset;
	int16 ant_num = 1;
	int min_fm_lp_sc = 0;
	int16 var_fc_bin5 = 0;
	radar_params_t *rparams = &st->rparams;
	bool *first_radar_indicator;
	radar_lp_info_t *rlpt;
	wl_radar_status_t *rt_status;
	int64 rollover_compl;
	int16 avg_fc_bin5;
	bool chirp_relax_lv1 = FALSE;
	bool chirp_relax_lv2 = FALSE;
	int32 radio_chanspec_sc, chanspec;

	phy_ac_chanmgr_get_val_sc_chspec(PHY_AC_CHANMGR(pi), &radio_chanspec_sc);
	chanspec = sec_pll ? radio_chanspec_sc : pi->radio_chanspec;

	if (ISACPHY(pi) && TONEDETECTION) {
		FMCombineOffset =
			MAX((2 << (((rparams->radar_args.st_level_time>> 12) & 0xf) -1)) -1, 0);
	} else {
		FMCombineOffset = 0;
	}

	/* t2_min[15:12] = x; if n_non_single >= x && lp_length > npulses_lp => bin5 detected */
	/* t2_min[11:10] = # times combining adjacent pulses < min_pw_lp  */
	/* t2_min[9] = fm_tol enable */
	/* t2_min[8] = skip_type 5 enable */
	/* t2_min[7:4] = y; bin5 remove pw <= 10*y  */
	/* t2_min[3:0] = t; non-bin5 remove pw <= 5*y */
	if (sec_pll == FALSE || !(PHY_SUPPORT_SCANCORE(pi) || PHY_SUPPORT_BW80P80(pi))) {
		if (bw80_80_mode == FALSE) {
			ant_num = GET_RDR_NANTENNAS(pi);
		} else {
			ant_num = 1;
		}
		first_radar_indicator = &st->first_radar_indicator;
		rt_status = &st->radar_status;
		rlpt = &st->radar_lp_info;
	} else {
		ant_num = 1;
		if (((st->rparams.radar_thrs2.highpow_war_enb >> 2) & 0x3) == 0) {
			min_fm_lp_sc = rparams->radar_args.min_fm_lp >> 1;
		} else if (((st->rparams.radar_thrs2.highpow_war_enb >> 2) & 0x3) == 1) {
			min_fm_lp_sc = (rparams->radar_args.min_fm_lp * 3) >> 2;
		} else if (((st->rparams.radar_thrs2.highpow_war_enb >> 2) & 0x3) == 2) {
			min_fm_lp_sc = (rparams->radar_args.min_fm_lp * 7) >> 3;
		} else {
			min_fm_lp_sc = rparams->radar_args.min_fm_lp;
		}
		if (bw80_80_mode == FALSE) {
			rt_status = st->radar_status_sc;
			first_radar_indicator = &st->first_radar_indicator_sc;
		} else {
			rt_status = &st->radar_status;
			first_radar_indicator = &st->first_radar_indicator;
		}
		rlpt = st->radar_work_lp_sc;
	}
	/* remove "noise" pulses with small pw */
	for (ant = 0; ant < ant_num; ant++) {
		wr_ptr = 0;
		mlength = rt->length_bin5[ant];
		for (i = 0; i < rt->length_bin5[ant]; i++) {
			if (rt->pulses_bin5[ant][i].pw >
				10*((rparams->radar_args.t2_min >> 4) & 0xf) &&
				rt->pulses_bin5[ant][i].fm >
				((ISACPHY(pi) && TONEDETECTION &&
				sec_pll == FALSE && bw80_80_mode == FALSE) ?
				rparams->radar_args.min_fm_lp : min_fm_lp_sc)) {
				rt->pulses_bin5[ant][wr_ptr] = rt->pulses_bin5[ant][i];
				++wr_ptr;
			} else {
				mlength--;
			}
		}	/* for mlength loop */
		rt->length_bin5[ant] = mlength;
	}	/* for ant loop */

	if (rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_BIN5_PULSE) {
		/* bin5 */
		for (ant = 0; ant < ant_num; ant++) {
			PHY_RADAR(("\nBin5 after removing noise pulses with pw <= %dus",
				((rparams->radar_args.t2_min >> 4) & 0xf) * 10/20));
			PHY_RADAR(("\nAnt %d: %d pulses, ", ant, rt->length_bin5[ant]));

			phy_radar_output_pulses(pi, rt->pulses_bin5[ant], rt->length_bin5[ant],
				rparams->radar_args.feature_mask, TRUE);
		}
	}

	/* Combine pulses that are adjacent for ant 0 and ant 1 */
	for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) {
		rt->length =  rt->length_bin5[ant];
		for (k = 0; k < ((rparams->radar_args.t2_min >> 10) & 0x3); k++) {
			mlength = rt->length;
			if (mlength > 1) {
				for (i = 1; i < mlength; i++) {
					deltat = ABS((int32)(rt->pulses_bin5[ant][i].interval -
					             rt->pulses_bin5[ant][i-1].interval));

					if (deltat <= (int32)rparams->radar_args.max_pw_lp) {
						wlc_phy_radar_fc_chirp_cmb(
							&rt->pulses_bin5[ant][i-1],
							&rt->pulses_bin5[ant][i]);
						rt->pulses_bin5[ant][i-1].pw =
							deltat + rt->pulses_bin5[ant][i].pw;
						rt->pulses_bin5[ant][i-1].fm =
							(rt->pulses_bin5[ant][i-1].fm +
							rt->pulses_bin5[ant][i].fm) -
							FMCombineOffset;
						for (j = i; j < mlength - 1; j++) {
							rt->pulses_bin5[ant][j] =
								rt->pulses_bin5[ant][j+1];
						}
						mlength--;
						rt->length--;
					}	/* if deltat */
				}	/* for mlength loop */
			}	/* mlength > 1 */
		}
	} /* for ant loop */

	/* Use data from one of the antenna */
	for (ant = 0; ant < ant_num; ant++) {

	bzero(rt->pulses, sizeof(rt->pulses));
	rt->length = 0;
	for (i = 0; i < rt->length_bin5[ant]; i++) {
		rt->pulses[rt->length] = rt->pulses_bin5[ant][i];
		rt->length++;
	}

	if (rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_BIN5_PULSE) {
		/* bin5 */
		PHY_RADAR(("\nBin5 after use data from one of two antennas"));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(pi, rt->pulses, rt->length,
			rparams->radar_args.feature_mask, TRUE);
	}

	/* remove pulses that are spaced less than high byte of "blank" */
	for (i = 1; i < rt->length; i++) {
		deltat = ABS((int32)(rt->pulses[i].interval - rt->pulses[i-1].interval));
		if (deltat < (ISACPHY(pi) ?
			(((int32)rparams->radar_args.blank & 0xff00) >> 8) : 128)) {
			for (j = i - 1; j < (rt->length); j++) {
				rt->pulses[j] = rt->pulses[j + 1];
			}
			rt->length--;
		}
	}

	if (PHY_RADAR_FIFO_SUBBAND_FORMAT(pi)) {
		/* remove entries chirp out of range */
		j = 0;
		for (i = 0; i < rt->length; i++) {
			if (rt->pulses[i].chirp <= FCC5_CHIRP) {
				rt->pulses[j] = rt->pulses[i];
				j++;
			}
		}
		rt->length = j;
	}

	/* use FIFO data of next antenna */
	if (rt->length == 0)
		continue;

	if (rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_PRUNED_BIN5_PULSE) {
		/* bin5 */
		PHY_RADAR(("\nBin5 pulses after pruning (filtering)"));

		phy_radar_output_pulses(pi, rt->pulses, rt->length,
			rparams->radar_args.feature_mask, TRUE);
	}

	if (CHSPEC_IS20(chanspec)) {
		rollover_compl = (int64)TWO_POWER_32;
	} else if (CHSPEC_IS40(chanspec)) {
		rollover_compl = (int64)TWO_POWER_32/2;
	} else if (CHSPEC_IS80(chanspec)) {
		rollover_compl = (int64)TWO_POWER_32/4;
	} else if (CHSPEC_IS160(chanspec)) {
		rollover_compl = (int64)TWO_POWER_32/8;
	} else {
		rollover_compl = (int64)TWO_POWER_32;
	}

	/* prune lp buffer */
	/* remove any entry outside the time MAX_LP_BUFFER_SPAN_20MHZ */
	/* compared to the latest incoming pulse */
	if ((rlpt->lp_length > 0) && (rt->length > 0)) {
		/* calculate deltat taking into account possible numerical rollover */
		if (rlpt->lp_buffer[0] > rt->pulses[rt->length - 1].interval) {
			deltat = ABS((int32)((int64)rt->pulses[rt->length - 1].interval -
				(int64)rlpt->lp_buffer[0] + rollover_compl));
			PHY_RADAR(("    LP_BUFFER ROLLOVER: rt->pulses.interval[%d]=%u "
				"lp_buffer[%d]=%u rollover_compl=%lld Kdeltat=%u\n",
				rt->length - 1, rt->pulses[rt->length - 1].interval,
				0, rlpt->lp_buffer[0], rollover_compl, deltat/1000));
		} else {
			deltat = ABS((int32)(rt->pulses[rt->length - 1].interval -
				rlpt->lp_buffer[0]));
		}

		i = 0;
		while ((i < rlpt->lp_length) && (deltat > MAX_LP_BUFFER_SPAN_20MHZ)) {
			i++;
			/* calculate deltat taking into account possible numerical rollover */
			if (rlpt->lp_buffer[i] > rt->pulses[rt->length - 1].interval) {
				deltat = ABS((int32)((int64)rt->pulses[rt->length - 1].interval -
					(int64)rlpt->lp_buffer[i] + rollover_compl));
				PHY_RADAR(("    LP_BUFFER ROLLOVER: rt->pulses.interval[%d]=%u "
					"lp_buffer[%d]=%u rollover_compl=%lld Kdeltat=%u\n",
					rt->length - 1, rt->pulses[rt->length - 1].interval,
					i, rlpt->lp_buffer[i], rollover_compl, deltat/1000));
			} else {
				deltat = ABS((int32)(rt->pulses[rt->length - 1].interval
					- rlpt->lp_buffer[i]));
			}
		}

		if (i > 0) {
			PHY_RADAR(("    **** LP_BUFFER old entries purched: # of entries"
				" purched i=%d\n", i));
			for (j = i; j < rlpt->lp_length; j++) {
				rlpt->lp_buffer[j-i] = rlpt->lp_buffer[j];
			}

			if (i <= rlpt->lp_length) {
				rlpt->lp_length -= i;
			} else {
				PHY_RADAR(("    **** unexpected error: lp_length=%d i=%d\n",
					rlpt->lp_length, i));
				rlpt->lp_length = 0;
			}
		}

		/* reset detection parameters */
		if (rlpt->lp_length == 0) {
			rlpt->lp_cnt = 0;
			rlpt->lp_skip_cnt = 0;
			rlpt->lp_csect_single = 0;
			rlpt->lp_len_his_idx = 0;
			rlpt->min_detected_fc_bin5 = MAX_FC;
			rlpt->max_detected_fc_bin5 = MIN_FC;
			rlpt->avg_detected_fc_bin5 = 0;
			rlpt->last_detection_time = pi->sh->now;
			rlpt->last_detection_time_lp = pi->sh->now;
			rlpt->subband_result = 0;
			rlpt->last_tstart = 0;
			rlpt->transmission_chirp = 0;
			rlpt->chirp_fail_cnt = 0;
			rlpt->chirp_match_cnt = 0;
		}
	}

	/* First perform FCC-5 detection */
	/* add new pulses */

	/* process each new pulse */
	for (i = 0; i < rt->length; i++) {
		invalid_type = 0;
		/* calculate deltat taking into account possible numerical rollover */
		if (rlpt->last_tstart > rt->pulses[i].interval) {
			deltat = ABS((int32)((int64)rt->pulses[i].interval
				- (int64)rlpt->last_tstart + rollover_compl));
			PHY_RADAR(("    TSTART ROLLOVER: tstart=%u last_tstart=%u "
				"rollover_compl=%lld Kdeltat=%u\n",
				rt->pulses[i].interval, rlpt->last_tstart, rollover_compl,
				deltat/1000));
		} else {
			deltat = ABS((int32)(rt->pulses[i].interval - rlpt->last_tstart));
		}

		if (PHY_RADAR_FIFO_SUBBAND_FORMAT(pi) &&
			(st->rparams.radar_thrs2.highpow_war_enb & 0x1) == 1) {
			valid_lp = (rt->pulses[i].pw >= PW_TOL_HIPOWWAR_BIN5) &&
				(rt->pulses[i].pw <= rparams->radar_args.max_pw_lp) &&
				(rt->pulses[i].fm >=
				((ISACPHY(pi) && TONEDETECTION &&
					sec_pll == FALSE && bw80_80_mode == FALSE) ?
				rparams->radar_args.min_fm_lp/2 :
				min_fm_lp_sc/2)) &&
				(deltat >= rparams->min_deltat_lp);
			if (!valid_lp) {
				invalid_type = 1;
			}
		} else {
			valid_lp = (rt->pulses[i].pw >= rparams->radar_args.min_pw_lp) &&
				(rt->pulses[i].pw <= rparams->radar_args.max_pw_lp) &&
				(rt->pulses[i].fm >= ((sec_pll == FALSE && bw80_80_mode == FALSE) ?
				rparams->radar_args.min_fm_lp : min_fm_lp_sc)) &&
				(deltat >= rparams->min_deltat_lp);
			if (!valid_lp) {
				invalid_type = 2;
			}
		}

		if (valid_lp && PHY_RADAR_FIFO_SUBBAND_FORMAT(pi)) {
			if (((st->rparams.radar_thrs2.highpow_war_enb >> 1) & 0x1) == 1) {
				bool chirp_condi;
				if (!PHY_AS_80P80(pi, pi->radio_chanspec)) {
					chirp_condi = (rt->pulses[i].chirp >
					((rt->pulses[i].pw >=
					PW_CHIRP_ADJ_TH_BIN5) ?
					CHIRP_TH2 : CHIRP_TH1));
				} else {
					/* ignore chirp condition when its */
					/* estimation is abort and fc is > 30MHz */
					chirp_condi = (rt->pulses[i].chirp >
					((rt->pulses[i].pw >=
					PW_CHIRP_ADJ_TH_BIN5) ?
					CHIRP_TH2 : CHIRP_TH1)) ||
					(rt->pulses[i].chirp == 0 &&
					rt->pulses[i].fc >= CHIRP_FC_TH);
				}
				valid_lp = valid_lp && chirp_condi;
				if (!valid_lp) {
					invalid_type = 3;
				}
			}
		}

		/* filter out: pulse interval > max_deltat_lp, and pulse interval <
		 * min_burst_intv_lp,  and pulse interval > max_burst_intv_lp while nLP != 0
		 */
		if (valid_lp) {
			if (deltat > (int32) rparams->max_deltat_lp &&
				deltat < (int32) rparams->radar_args.min_burst_intv_lp) {
				valid_lp = FALSE;
				invalid_type = 4;
			}
		}

		if (valid_lp && (rt_status->detected == FALSE)) {
			rt_status->intv[(rlpt->lp_length)%10] = deltat;
			rt_status->pw[(rlpt->lp_length)%10] = rt->pulses[i].pw;
			rt_status->fm[(rlpt->lp_length)%10] = rt->pulses[i].fm;
		}

		//Need to check notradar for bin5 radar detection of scan core
		if (valid_lp && PHY_RADAR_FIFO_SUBBAND_FORMAT(pi)) {
			if (sec_pll == FALSE && bw80_80_mode == FALSE) {
				if ((st->rparams.radar_thrs2.notradar_enb>>3 & 0x1) == 1)
					valid_lp = valid_lp &&
						(rt->pulses[i].notradar <
						st->rparams.radar_thrs2.max_notradar_lp);
				if (!valid_lp) {
					invalid_type = 5;
				}
			} else {
				if ((st->rparams.radar_thrs2.notradar_enb>>1 & 0x1) == 1)
					valid_lp = valid_lp &&
						(rt->pulses[i].notradar <
						st->rparams.radar_thrs2.max_notradar_lp_sc);
				if (!valid_lp) {
					invalid_type = 6;
				}
			}
		}

		/* remove pulses that start at the same time as the last one */
		if (valid_lp && rlpt->lp_length > 0 && rlpt->lp_length < LP_BUFFER_SIZE) {
			valid_lp = valid_lp &&
				(rt->pulses[i].interval != rlpt->lp_buffer[rlpt->lp_length]);
			if (!valid_lp) {
				PHY_RADAR(("lp_length=%d tstart=%d lp_buff->tstart=%d\n",
					rlpt->lp_length, rt->pulses[i].interval,
					rlpt->lp_buffer[rlpt->lp_length]));
				invalid_type = 7;
			}
		}

		skip_type = 0;

		if (valid_lp && deltat >= (int32) rparams->radar_args.min_burst_intv_lp) {
			rlpt->lp_cnt = 0;
		}

		/* Skip the pulse if outside of pulse interval range (1-2ms), */
		/* burst to burst interval not within range, more than 3 pulses in a */
		/* burst */

		if ((deltat < (int32) rparams->min_deltat_lp) ||
			(deltat > (int32) rparams->max_deltat_lp &&
			deltat < (int32) rparams->radar_args.min_burst_intv_lp)
			) {	/* possible skip lp */
			/* get skip type */
			if (deltat < (int32) rparams->min_deltat_lp) {
				skip_type = 1;
			} else if (deltat > (int32) rparams->max_deltat_lp &&
				deltat < (int32) rparams->radar_args.min_burst_intv_lp) {
				skip_type = 2;
			} else {
				skip_type = 999;
			}
		}

		if (rlpt->avg_detected_fc_bin5 == 0) {
			avg_fc_bin5 = rt->pulses[i].fc;
		} else {
			avg_fc_bin5 = rlpt->avg_detected_fc_bin5;
		}

		if (CHSPEC_IS20(chanspec) && CHSPEC_CHANNEL(chanspec) > WL_THRESHOLD_LO_BAND) {
			chirp_relax_lv1 = TRUE;
			if ((avg_fc_bin5 > CHIRP_RELAX_LV2_FC_BOUND) ||
				((st->rparams.radar_thrs2.highpow_war_enb >> 15) & 0x1)) {
				chirp_relax_lv2 = TRUE;
			} else {
				chirp_relax_lv2 = FALSE;
			}
		} else {
			chirp_relax_lv1 = FALSE;
		}

		/*
		   PHY_RADAR(("avg_fc_bin5=%d chirp_relax_lv1=%d chirp_relax_lv2=%d "
		   "CHSPEC_CHANNEL(pi->radio_chanspec)=%d WL_THRESHOLD_LO_BAND=%d\n",
		   avg_fc_bin5, chirp_relax_lv1, chirp_relax_lv2,
		   sec_pll ? CHSPEC_CHANNEL(radio_chanspec_sc) :
		   CHSPEC_CHANNEL(pi->radio_chanspec), WL_THRESHOLD_LO_BAND));
		*/

		/* Check for chirp consistency within a transmission period (12 sec) */
		if (PHY_RADAR_FIFO_SUBBAND_FORMAT(pi) && valid_lp &&
			((st->rparams.radar_thrs2.highpow_war_enb >> 13) & 0x1) &&
			!(rparams->radar_args.feature_mask & RADAR_FEATURE_NEWJP_DETECT) &&
			!chirp_relax_lv2) {
			if (rlpt->lp_length == 0) {
				rlpt->transmission_chirp = rt->pulses[i].chirp;
				rlpt->chirp_match_cnt = 0;
				rlpt->chirp_fail_cnt = 0;
				skip_type = -2;
			} else if (rlpt->lp_length > 0) {
				if (ABS(rt->pulses[i].chirp - rlpt->transmission_chirp)
					<= (chirp_relax_lv1 ?
					(((st->rparams.radar_thrs2.highpow_war_enb
					>> 4) & 0x7) + CHIRP_TOL_RELAX_COUNT)
					:((st->rparams.radar_thrs2.highpow_war_enb >> 4) & 0x7))) {
					/* bits 4-6 of highpow_war_enb = good_chirp_tollerance */
					PHY_RADAR(("    GOOD Chirp: txCHIRP=%d CHIRP=%d"
						" chirp_fail_cnt=%d"
						" chirp_match_cnt=%d"
						" NOTRADAR=%d\n",
						rlpt->transmission_chirp,
						rt->pulses[i].chirp, rlpt->chirp_fail_cnt,
						rlpt->chirp_match_cnt,
						rt->pulses[i].notradar));
					rlpt->chirp_match_cnt++;
					rlpt->chirp_fail_cnt = 0;
					if (deltat > (int32) rparams->radar_args.max_burst_intv_lp)
					{
						/* This to align txChirp to the one from a new */
						/* burst in case it's too close to the existing */
						/* one and passing the check */
						rlpt->transmission_chirp = rt->pulses[i].chirp;
						PHY_RADAR(("    NEW Chirp 2: txCHIRP=%d CHIRP=%d"
							" chirp_fail_cnt=%d"
							" chirp_match_cnt=%d"
							" NOTRADAR=%d\n",
							rlpt->transmission_chirp,
							rt->pulses[i].chirp, rlpt->chirp_fail_cnt,
							rlpt->chirp_match_cnt,
							rt->pulses[i].notradar));
						rlpt->lp_length = 0;
						rlpt->chirp_match_cnt = 0;
						rlpt->chirp_fail_cnt = 0;
						rlpt->lp_skip_cnt = 0;
						rlpt->lp_csect_single = 0;
						rlpt->lp_cnt = 0;
						rlpt->min_detected_fc_bin5 = MAX_FC;
						rlpt->max_detected_fc_bin5 = MIN_FC;
						rlpt->avg_detected_fc_bin5 = 0;
						rlpt->subband_result = 0;
						skip_type = -2;
					}
				} else {
					PHY_RADAR(("    BAD Chirp: txCHIRP=%d CHIRP=%d"
						" chirp_fail_cnt=%d chirp_match_cnt=%d"
						" NOTRADAR=%d"
						"\n",
						rlpt->transmission_chirp,
						rt->pulses[i].chirp, rlpt->chirp_fail_cnt,
						rlpt->chirp_match_cnt,
						rt->pulses[i].notradar));

					if ((rlpt->chirp_match_cnt > 0) &&
						(rlpt->chirp_match_cnt
						< GOOD_CHIRP_CNT_LIMIT)) {
						rlpt->chirp_match_cnt--;
					}
					rlpt->chirp_fail_cnt++;
					if (deltat > (int32) rparams->radar_args.max_burst_intv_lp)
					{
						/* New burst comes. Update txChirp */
						rlpt->transmission_chirp = rt->pulses[i].chirp;
						PHY_RADAR(("    NEW Chirp: txCHIRP=%d CHIRP=%d"
							" chirp_fail_cnt=%d"
							" chirp_match_cnt=%d"
							" NOTRADAR=%d\n",
							rlpt->transmission_chirp,
							rt->pulses[i].chirp, rlpt->chirp_fail_cnt,
							rlpt->chirp_match_cnt,
							rt->pulses[i].notradar));
						rlpt->lp_length = 0;
						rlpt->chirp_match_cnt = 0;
						rlpt->chirp_fail_cnt = 0;
						rlpt->lp_skip_cnt = 0;
						rlpt->lp_csect_single = 0;
						rlpt->lp_cnt = 0;
						rlpt->min_detected_fc_bin5 = MAX_FC;
						rlpt->max_detected_fc_bin5 = MIN_FC;
						rlpt->avg_detected_fc_bin5 = 0;
						rlpt->subband_result = 0;
						skip_type = -2;
					} else {
						if (rlpt->chirp_fail_cnt >=
							((st->rparams.radar_thrs2.highpow_war_enb
							>> 7) & 0x7)) {
							skip_type = 4;
						} else {
							valid_lp = 0;
							invalid_type = 9;
						}
					}
				}
			}
		}

		width = 0;
		fm = 0;
		pw_dif = 0;
		fm_dif = 0;
		fm_tol = 0;
		pw_tol = rparams->radar_args.max_span_lp & 0xff;
		/* monitor the number of pw and fm matching */
		/* max_span_lp[11:8] = x, x/16 = % alowed fm tollerance */
		/* max_span_lp[7:0] = alowed pw tollerance */
		if (valid_lp && skip_type <= 0) {
			if ((rlpt->lp_cnt == 0) ||
				(deltat >= (int32) rparams->radar_args.min_burst_intv_lp)) {
				rlpt->lp_pw[0] = rt->pulses[i].pw;
				rlpt->lp_fm[0] = rt->pulses[i].fm;
				rlpt->lp_cnt++;
			} else if (rlpt->lp_cnt == 1) {
				width = rlpt->lp_pw[0];
				fm = rlpt->lp_fm[0];
				pw_dif = ABS(rt->pulses[i].pw - width);
				fm_dif = ABS(rt->pulses[i].fm - fm);
				if (rparams->radar_args.t2_min & 0x200) {
					if ((st->rparams.radar_thrs2.highpow_war_enb & 0x1)
						== 1) {
						fm_tol = (fm*
							((rparams->radar_args.max_span_lp
							>> 8) & 0xf))/4;
					} else {
						fm_tol = (fm*((rparams->radar_args.max_span_lp >> 8)
							& 0xf))/16;
					}
				} else {
					fm_tol = 999;
				}
				if (pw_dif < pw_tol && fm_dif < fm_tol) {
					if (deltat < MAX_INTV_LP) {
						rlpt->lp_pw[1] = rt->pulses[i].pw;
						rlpt->lp_fm[1] = rt->pulses[i].fm;
						rlpt->lp_cnt++;
					} else {
						skip_type = 5;
					}
				} else if (rparams->radar_args.t2_min & 0x100) {
						skip_type = 6;
				}
			} else if (rlpt->lp_cnt == 2) {
				width = rlpt->lp_pw[1];
				fm = rlpt->lp_fm[1];
				pw_dif = ABS(rt->pulses[i].pw - width);
				fm_dif = ABS(rt->pulses[i].fm - fm);
				if (rparams->radar_args.t2_min & 0x200) {
					fm_tol = (fm*((rparams->radar_args.max_span_lp >> 8)
						& 0xf))/16;
				} else {
					fm_tol = 999;
				}
				if (pw_dif < pw_tol && fm_dif < fm_tol) {
					if (deltat < MAX_INTV_LP) {
						rlpt->lp_pw[2] = rt->pulses[i].pw;
						rlpt->lp_fm[2] = rt->pulses[i].fm;
						rlpt->lp_cnt++;
					} else {
						skip_type = 7;
					}
				} else if (rparams->radar_args.t2_min & 0x100) {
						skip_type = 8;
				}
			} else {
				rlpt->lp_cnt++;
				skip_type = 9;
			}
		}

		if (skip_type >= 3 && skip_type <= 9)  {	/* skipped lp */
			valid_lp = FALSE;
			rlpt->lp_skip_cnt++;
			rlpt->lp_skipped_pw = rt->pulses[i].pw;
			rlpt->lp_skipped_fm = rt->pulses[i].fm;

			/* print "SKIPPED LP" debug messages */
			/* print inteval in us or 20MHz k-sample */
			if (rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
				PHY_RADAR(("SKIPPED LP:"
					" nLP=%d skip_cnt=%d (K)Intv=%u"
					" PW=%d FM=%d"
					" skip_type=%d lp_cnt=%d csect_single=%d CHIRP=%d"
					" txCHIRP=%d chirp_match_cnt=%d chirp_fail_cnt=%d"
					" NOTRADAR=%d tot_lp_cnt=%d"
					"\n",
					rlpt->lp_length, rlpt->lp_skip_cnt, deltat/20,
					rt->pulses[i].pw/20, rt->pulses[i].fm,
					skip_type, rlpt->lp_cnt, rlpt->lp_csect_single,
					rt->pulses[i].chirp, rlpt->transmission_chirp,
					rlpt->chirp_match_cnt, rlpt->chirp_fail_cnt,
					rt->pulses[i].notradar,
					rlpt->tot_lp_cnt));
			} else {
				PHY_RADAR(("SKIPPED LP:"
					" nLP=%d skip_cnt=%d (K)Intv=%u"
					" PW=%d FM=%d"
					" skip_type=%d lp_cnt=%d csect_single=%d CHIRP=%d"
					" txCHIRP=%d chirp_match_cnt=%d chirp_fail_cnt=%d"
					" NOTRADAR=%d tot_lp_cnt=%d"
					"\n",
					rlpt->lp_length, rlpt->lp_skip_cnt, deltat/1000,
					rt->pulses[i].pw, rt->pulses[i].fm,
					skip_type, rlpt->lp_cnt, rlpt->lp_csect_single,
					rt->pulses[i].chirp, rlpt->transmission_chirp,
					rlpt->chirp_match_cnt, rlpt->chirp_fail_cnt,
					rt->pulses[i].notradar,
					rlpt->tot_lp_cnt));
			}
			if ((skip_type >= 5) && (skip_type <= 9)) {
				if (rparams->radar_args.feature_mask &
					RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
					PHY_RADAR(("           "
						" pw_prv=%d pw_dif=%d pw_tol=%d fm_prv=%d fm_dif=%d"
						" fm_tol=%d\n", width/20, pw_dif/20, pw_tol/20, fm,
						fm_dif, fm_tol));
				} else {
					PHY_RADAR(("           "
						" pw_prv=%d pw_dif=%d pw_tol=%d fm_prv=%d fm_dif=%d"
						" fm_tol=%d\n", width, pw_dif, pw_tol, fm,
						fm_dif, fm_tol));
				}
			}
			if (skip_type == 999) {
				PHY_RADAR(("UNKOWN SKIP TYPE: %d\n", skip_type));
			}

			/* if a) 2 consecutive skips and */
			/*    b) LP length has not reached detection threshold */
			/* then reset lp buffer ... */
			if ((rlpt->lp_skip_cnt >= rparams->radar_args.nskip_rst_lp) &&
				(rlpt->lp_length < rparams->radar_args.npulses_lp)) {
				if (rlpt->lp_len_his_idx < LP_LEN_HIS_SIZE) {
					rlpt->lp_len_his[rlpt->lp_len_his_idx] = rlpt->lp_length;
					rlpt->lp_len_his_idx++;
				}
				PHY_RADAR(("**** SKIPPED RESET ****\n"));
				rlpt->lp_length = 0;
				rlpt->lp_skip_cnt = 0;
				rlpt->lp_csect_single = 0;
				rlpt->lp_cnt = 0;
				rlpt->min_detected_fc_bin5 = MAX_FC;
				rlpt->max_detected_fc_bin5 = MIN_FC;
				rlpt->avg_detected_fc_bin5 = 0;
				rlpt->subband_result = 0;
				rlpt->chirp_fail_cnt = 0;
			}
		} else if (valid_lp && (rlpt->lp_length < LP_BUFFER_SIZE)) {	/* valid lp */
			if (rt->pulses[i].fc <= rlpt->min_detected_fc_bin5)
				rlpt->min_detected_fc_bin5 = rt->pulses[i].fc;
			if (rt->pulses[i].fc >= rlpt->max_detected_fc_bin5)
				rlpt->max_detected_fc_bin5 = rt->pulses[i].fc;

			if (rlpt->avg_detected_fc_bin5 == 0) {
				rlpt->avg_detected_fc_bin5 = (rt->pulses[i].fc);
			} else {
				/* add 1(1/2=0.5) for rounding case */
				rlpt->avg_detected_fc_bin5 = (rlpt->avg_detected_fc_bin5
								+ rt->pulses[i].fc + 1) / 2;
			}
			/* reset consecutive singles counter if # of pulses > 1 */
			if (rlpt->lp_cnt > 1) {
				rlpt->lp_csect_single = 0;
			} else {
				++rlpt->lp_csect_single;
			}

			rlpt->lp_skip_cnt = 0;
			rlpt->lp_buffer[rlpt->lp_length] = rt->pulses[i].interval;
			rlpt->lp_length++;
			if (rlpt->lp_length == 1) {
				rlpt->tot_lp_cnt = 0;
			} else if (rlpt->lp_length >= 2 && deltat < MAX_INTV_LP) {
				rlpt->tot_lp_cnt++;
			}

			/* print "VALID LP" debug messages */
			/* print interval in us or 20MHz k-sample */
			if (rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
				PHY_RADAR(("VALID LP:"
					" (K)Intv=%u"
					" PW=%d FM=%d lp_cnt=%d"
					" pw_prv=%d pw_dif=%d pw_tol=%d fm_prv=%d"
					" fm_dif=%d fm_tol=%d"
					" FC=%d CHIRP=%d NOTRADAR=%d\n",
					deltat/20,
					rt->pulses[i].pw/20, rt->pulses[i].fm, rlpt->lp_cnt,
					width/20, pw_dif/20, pw_tol/20,
					fm, fm_dif, fm_tol,
					rt->pulses[i].fc, rt->pulses[i].chirp,
					rt->pulses[i].notradar));
			} else {
				PHY_RADAR(("VALID LP:"
					" (K)Intv=%u"
					" PW=%d FM=%d lp_cnt=%d"
					" pw_prv=%d pw_dif=%d pw_tol=%d fm_prv=%d"
					" fm_dif=%d fm_tol=%d"
					" FC=%d CHIRP=%d NOTRADAR=%d\n",
					deltat/1000,
					rt->pulses[i].pw, rt->pulses[i].fm, rlpt->lp_cnt,
					width, pw_dif, pw_tol,
					fm, fm_dif, fm_tol,
					rt->pulses[i].fc, rt->pulses[i].chirp,
					rt->pulses[i].notradar));
			}
			PHY_RADAR(("         "
				" nLP=%d skip_cnt=%d"
				" csect_single=%d"
				" txCHIRP=%d chirp_match_cnt=%d chirp_fail_cnt=%d tot_lp_cnt=%d"
				"\n",
				rlpt->lp_length, rlpt->lp_skip_cnt,
				rlpt->lp_csect_single, rlpt->transmission_chirp,
				rlpt->chirp_match_cnt, rlpt->chirp_fail_cnt, rlpt->tot_lp_cnt));

				rlpt->last_tstart = rt->pulses[i].interval;

			if (rlpt->lp_csect_single >= ((rparams->radar_args.t2_min >> 12) & 0xf)) {
				if (rlpt->lp_length >
					((st->rparams.radar_thrs2.highpow_war_enb >> 10) & 0x7)) {
					rlpt->lp_length -=
						((st->rparams.radar_thrs2.highpow_war_enb >> 10)
						& 0x7);
				} else {
					rlpt->lp_length = 0;
				}
				PHY_RADAR(("    **** lp_csect_single=%d >= %d, new nLP=%d\n",
					rlpt->lp_csect_single, ((rparams->radar_args.t2_min >> 12)
					& 0xf), rlpt->lp_length));
				rlpt->lp_csect_single -= CSECT_SINGLE_DECR;
			}
		} else if (!valid_lp || invalid_type != 0) {
		/* the above "if" is to make ivalide_type usefull to not trigger compile warning */
			if (rparams->radar_args.feature_mask &
				RADAR_FEATURE_DEBUG_INVLID_LP && !valid_lp) {
				/* print in3terval in us or 20MHz k-sample */
				if (rparams->radar_args.feature_mask &
					RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
					PHY_RADAR(("INVALID LP:"
						" nLP=%d Invalid_type=%u (K)Intv=%u PW=%d FM=%d"
						" FC=%d CHIRP=%d NOTRADAR=%d\n",
						rlpt->lp_length, invalid_type, deltat/20,
						rt->pulses[i].pw/20, rt->pulses[i].fm,
						rt->pulses[i].fc, rt->pulses[i].chirp,
						rt->pulses[i].notradar));
				} else {
					PHY_RADAR(("INVALID LP:"
						" nLP=%d Invalid_type=%u (K)Intv=%u PW=%d FM=%d"
						" FC=%d CHIRP=%d NOTRADAR=%d\n",
						rlpt->lp_length, invalid_type, deltat/1000,
						rt->pulses[i].pw, rt->pulses[i].fm,
						rt->pulses[i].fc, rt->pulses[i].chirp,
						rt->pulses[i].notradar));
				}
			}
		}
	}

	if (rlpt->lp_length > LP_BUFFER_SIZE)
		PHY_ERROR(("WARNING: LP buffer size is too long\n"));

#ifdef RADAR_DBG
	PHY_RADAR(("\n FCC-5 \n"));
	for (i = 0; i < rlpt->lp_length; i++) {
		PHY_RADAR(("%u  ", rlpt->lp_buffer[i]));
	}
	PHY_RADAR(("\n"));
#endif
	/* nLP (meaning rlpt->lp_length) > = detection threshold */
	if (rlpt->lp_length >= rparams->radar_args.npulses_lp) {
		/* check fc variation */
		if ((rlpt->max_detected_fc_bin5 -
			rlpt->avg_detected_fc_bin5) >=
			(rlpt->avg_detected_fc_bin5 -
			rlpt->min_detected_fc_bin5)) {
			var_fc_bin5 = rlpt->avg_detected_fc_bin5
				- rlpt->min_detected_fc_bin5;
		} else {
			var_fc_bin5 = rlpt->max_detected_fc_bin5
				- rlpt->avg_detected_fc_bin5;
		}
		PHY_RADAR(("max_detected_fc_bin5=%d min_detected_fc_bin5=%d"
			" avg_detected_fc_bin5=%d var_fc_bin5=%d\n",
			rlpt->max_detected_fc_bin5, rlpt->min_detected_fc_bin5,
			rlpt->avg_detected_fc_bin5, var_fc_bin5));

		/* check fc variation and tot_lp_cnt */
		if ((((var_fc_bin5 > MAX_FC_VARIATION) && (PHY_RADAR_FIFO_SUBBAND_FORMAT(pi))) &&
			((st->rparams.radar_thrs2.highpow_war_enb >> 14) & 0x1)) ||
			((rlpt->tot_lp_cnt < ((rparams->radar_args.max_span_lp >> 12) & 0x7)) &&
			((rparams->radar_args.max_span_lp >> 15) & 0x1))) {
			if ((var_fc_bin5 > MAX_FC_VARIATION) &&
				((st->rparams.radar_thrs2.highpow_war_enb >> 14) & 0x1) &&
				(PHY_RADAR_FIFO_SUBBAND_FORMAT(pi))) {
				PHY_RADAR(("**** Failed FC variation check: var_fc_bin5=%d"
					" > %d\n", var_fc_bin5, MAX_FC_VARIATION));
			}
			if ((rlpt->tot_lp_cnt <
				((rparams->radar_args.max_span_lp >> 12) & 0x7)) &&
				((rparams->radar_args.max_span_lp >> 15) & 0x1)) {
				PHY_RADAR(("**** Failed tot_lp_cnt check:"
					" tot_lp_cnt=%d < %d\n", rlpt->tot_lp_cnt,
					(rparams->radar_args.max_span_lp >> 12) & 0x7));
			}
		} else {
			/* reject detection spaced more than x minutes */
			deltat = (uint32) (pi->sh->now - rlpt->last_detection_time_lp);
			PHY_RADAR(("last_detection_time_lp=%u, watchdog_timer=%u, deltat=%d,"
				" deltat_min=%d, deltat_sec=%d\n",
				rlpt->last_detection_time_lp, pi->sh->now, deltat, deltat/60,
				deltat%60));
			rlpt->last_detection_time_lp = pi->sh->now;
			if ((uint32) deltat < (rparams->radar_args.fra_pulse_err & 0xff)*60 ||
			    (*first_radar_indicator == 1 && (uint32) deltat < 15 * 60)) {
				PHY_RADAR(("FCC-5 Radar Detection. Time from last detection"
					" = %u, = %dmin %dsec\n",
					deltat, deltat / 60, deltat % 60));

				radar_detected->radar_type = RADAR_TYPE_FCC_5;
				rt_status->count =
					rt_status->count + 1;
				rt_status->pretended = FALSE;
				rt_status->radartype = radar_detected->radar_type;
				rt_status->timenow = (uint32) (pi->sh->now);
				rt_status->timefromL = deltat;
				rt_status->ch = pi->radio_chanspec;
				rt_status->lp_csect_single = rlpt->lp_csect_single;

				if (var_fc_bin5 > st->rparams.radar_thrs2.fc_varth_bin5_sb) {
					if (CHSPEC_IS20(chanspec)) {
						radar_detected->subband = ALLSUBBAND_BW20;
					} else if (CHSPEC_IS40(chanspec)) {
						radar_detected->subband = ALLSUBBAND_BW40;
					} else if (CHSPEC_IS80(chanspec) ||
						PHY_AS_80P80(pi, pi->radio_chanspec)) {
						radar_detected->subband = ALLSUBBAND_BW80;
					} else if ((CHSPEC_CHANNEL(chanspec) == 50) &&
						(rlpt->avg_detected_fc_bin5 >= 0)) {
						radar_detected->subband = ALLSUBBAND_BW160_CH50;
					} else if (CHSPEC_CHANNEL(chanspec) == 114) {
						radar_detected->subband = ALLSUBBAND_BW160_CH114;
					}
				} else {
					//subband detection
					radar_detected->subband =
						wlc_phy_radar_fc_chirp_2_subband(pi,
						rlpt->avg_detected_fc_bin5, FCC5_CHIRP,
						sec_pll, 1);
				}
				rlpt->subband_result = radar_detected->subband;
				rt_status->detected = ((CHSPEC_CHANNEL(chanspec) == 50) &&
					((rlpt->subband_result & 0x0f) == 0)) ? FALSE : TRUE;
			}
			else {
				if (RADAR_FEATURE_DEBUG_REJECTED_RADAR) {
					PHY_RADAR(("SKIPPED false FCC-5 Radar Detection."
						" Time from last detection = %u, = %dmin %dsec,"
						" ncsect_single=%d\n",
						deltat, deltat / 60, deltat % 60,
						rlpt->lp_csect_single));
				}
			}
		}
		if (rlpt->lp_len_his_idx < LP_LEN_HIS_SIZE) {
			rlpt->lp_len_his[rlpt->lp_len_his_idx] = rlpt->lp_length;
			rlpt->lp_len_his_idx++;
		}
		rlpt->lp_length = 0;
		rlpt->lp_skip_cnt = 0;
		rlpt->lp_csect_single = 0;
		*first_radar_indicator = 0;
		rlpt->min_detected_fc_bin5 = MAX_FC;
		rlpt->max_detected_fc_bin5 = MIN_FC;
		rlpt->avg_detected_fc_bin5 = 0;
	} /* if rlpt->lp_length >= rparams->radar_args.npulses_lp */
	break;
	} /* for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) */
}

/* [PHY_REARCH] Need to rename this function. It is used in AC, N and HT */
uint8
phy_radar_run_nphy(phy_info_t *pi, radar_detected_info_t *radar_detected, bool sec_pll,
bool bw80_80_mode)
{
	phy_radar_info_t *ri = pi->radari;
	phy_radar_st_t *st = phy_radar_get_st(ri);
	wl_radar_status_t *rt_status;
	bool *first_radar_indicator;
	bool *dfs_lp_buffer_nphy;
/* NOTE: */
/* PLEASE UPDATE THE DFS_SW_VERSION #DEFINE'S IN FILE WLC_PHY_INT.H */
/* EACH TIME ANY DFS FUNCTION IS MODIFIED EXCEPT RADAR THRESHOLD CHANGES */
	uint16 i;
	uint16 j;
	int k;
	int wr_ptr;
	uint8 ant;
	uint16 mlength;
	int32 deltat;
	radar_work_t *rt;
	radar_lp_info_t *rlpt;
	radar_params_t *rparams = &st->rparams;
	wl_radar_thr2_t *radar_thrs2 = &rparams->radar_thrs2;
	uint8 det_type = RADAR_TYPE_NONE;
	int min_detected_pw;
	int max_detected_pw;
	uint32 min_detected_interval;
	uint32 max_detected_interval;
	int nconsecq_pulses = 0;
	int detected_pulse_index = 0;
	int32 deltat2 = 0;
	int min_fm_lp_sc = 0;
	int16 ant_num = 1;
	int var_fc = 0;
	int avg_fc = 0;
	uint16 blank_time = 0;
	int max_detected_chirp = 0;
	uint16 bw_idx = 0;
	uint16 fm_thrs_idx = 0;
	int fm_thrs_dbg = 0;
	int fm_thrs = -50;
	int fm_chk_pw_thrs1 = fm_chk_pw[0];
	int fm_chk_pw_thrs2 = fm_chk_pw[1];
	int prev_interval = 0;
	int32 radio_chanspec_sc, chanspec;
	int32 pulse_cmb_intv = 0;
	int32 ext_pulse_cmb_intv = 0;
	int32 ext_cmb_pulse_idx = 0;
	uint16 clip_th = 0;

	phy_ac_chanmgr_get_val_sc_chspec(PHY_AC_CHANMGR(pi), &radio_chanspec_sc);
	chanspec = sec_pll ? radio_chanspec_sc : pi->radio_chanspec;

	/* 4366/684C0 enlarge ofdm_nominal_clip_th to avoid radar triggers preemption when DFS ON */
	if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		(ACMAJORREV_47(pi->pubpi->phy_rev) && ACMINORREV_3(pi))) && (sec_pll == FALSE)) {
		clip_th = (ACMAJORREV_47(pi->pubpi->phy_rev) && ACMINORREV_3(pi)) ? 0x4000 : 0x8000;
		phy_utils_write_phyreg(pi, ACPHY_PREMPT_ofdm_nominal_clip_th0(pi->pubpi->phy_rev),
				clip_th);
		phy_utils_write_phyreg(pi, ACPHY_PREMPT_ofdm_nominal_clip_th1(pi->pubpi->phy_rev),
				clip_th);
		phy_utils_write_phyreg(pi, ACPHY_PREMPT_ofdm_nominal_clip_th2(pi->pubpi->phy_rev),
				clip_th);
		phy_utils_write_phyreg(pi, ACPHY_PREMPT_ofdm_nominal_clip_th3(pi->pubpi->phy_rev),
				clip_th);
	}

	if (sec_pll == FALSE || !(PHY_SUPPORT_SCANCORE(pi) || PHY_SUPPORT_BW80P80(pi))) {
		rt_status = &st->radar_status;
		rlpt = &st->radar_lp_info;
		first_radar_indicator = &st->first_radar_indicator;
		dfs_lp_buffer_nphy = &pi->dfs_lp_buffer_nphy;
		if (bw80_80_mode == FALSE) {
			ant_num = GET_RDR_NANTENNAS(pi);
		} else {
			ant_num = 1;
		}
		blank_time = phy_utils_read_phyreg(pi,
			ACPHY_RadarBlankCtrl(pi->pubpi->phy_rev));
	} else {
		rlpt = st->radar_work_lp_sc;
		if (bw80_80_mode == FALSE) {
			rt_status = st->radar_status_sc;
		} else {
			//share the same radar status under BW80+80
			rt_status = &st->radar_status;
		}
		first_radar_indicator = &st->first_radar_indicator_sc;
		dfs_lp_buffer_nphy = &pi->dfs_lp_buffer_nphy_sc;
		if (((st->rparams.radar_thrs2.highpow_war_enb >> 2) & 0x3) == 0) {
			min_fm_lp_sc = rparams->radar_args.min_fm_lp >> 1;
		} else if (((st->rparams.radar_thrs2.highpow_war_enb >> 2) & 0x3) == 1) {
			min_fm_lp_sc = (rparams->radar_args.min_fm_lp * 3) >> 2;
		} else if (((st->rparams.radar_thrs2.highpow_war_enb >> 2) & 0x3) == 2) {
			min_fm_lp_sc = (rparams->radar_args.min_fm_lp * 7) >> 3;
		} else {
			min_fm_lp_sc = rparams->radar_args.min_fm_lp;
		}
		ant_num = 1;
		if (bw80_80_mode == FALSE) {
			blank_time = phy_utils_read_phyreg(pi,
				ACPHY_RadarBlankCtrl_SC(pi->pubpi->phy_rev));
		} else {
			blank_time = phy_utils_read_phyreg(pi,
				ACPHY_RadarBlankCtrl(pi->pubpi->phy_rev));
		}
	}

	if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
		ACMAJORREV_33(pi->pubpi->phy_rev) ||
		ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		blank_time = (blank_time & 0xff);
		if (CHSPEC_IS40(pi->radio_chanspec)) {
			blank_time = blank_time/2;
		} else if (CHSPEC_IS80(pi->radio_chanspec) ||
			PHY_AS_80P80(pi, pi->radio_chanspec)) {
			if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
				blank_time = blank_time/2;
			} else {
				blank_time = blank_time/4;
			}
		} else if (CHSPEC_IS160(pi->radio_chanspec)) {
			blank_time = blank_time/8;
		}
	} else {
		blank_time = 0;
	}

	rparams = &st->rparams;

	ASSERT(radar_detected != NULL);

	(void) memset(radar_detected, 0, sizeof(*radar_detected));

	/* clear LP buffer if requested, and print LP buffer count history */
	if (*dfs_lp_buffer_nphy != 0) {
		*dfs_lp_buffer_nphy = 0;
		*first_radar_indicator = 1;
		PHY_RADAR(("DFS LP buffer =  "));
		for (i = 0; i < rlpt->lp_len_his_idx; i++) {
			PHY_RADAR(("%d, ", rlpt->lp_len_his[i]));
		}
		PHY_RADAR(("%d; now CLEARED\n", rlpt->lp_length));
		rlpt->lp_length = 0;
		rlpt->lp_cnt = 0;
		rlpt->lp_skip_cnt = 0;
		rlpt->lp_csect_single = 0;
		rlpt->lp_len_his_idx = 0;
		rlpt->min_detected_fc_bin5 = MAX_FC;
		rlpt->max_detected_fc_bin5 = MIN_FC;
		rlpt->avg_detected_fc_bin5 = 0;
		rlpt->last_detection_time = pi->sh->now;
		rlpt->last_detection_time_lp = pi->sh->now;
		rlpt->subband_result = 0;
		rlpt->last_tstart = 0;
		rlpt->transmission_chirp = 0;
		rlpt->chirp_fail_cnt = 0;
		rlpt->chirp_match_cnt = 0;
	}
	if (!rparams->radar_args.npulses) {
		PHY_ERROR(("%s: radar params not initialized\n", __FUNCTION__));
		return RADAR_TYPE_NONE;
	}

	min_detected_pw = rparams->radar_args.max_pw;
	max_detected_pw = rparams->radar_args.min_pw;

	/* current radar fifo sample info */
	rt = phy_cache_acquire_reuse_buffer(pi->cachei, sizeof(radar_work_t));

	/* suspend mac before reading phyregs */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_utils_phyreg_enter(pi);

	wlc_phy_radar_read_table(pi, st, rt, sec_pll, bw80_80_mode);

	/* restart mac after reading phyregs */
	phy_utils_phyreg_exit(pi);
	wlapi_enable_mac(pi->sh->physhim);

	/* skip radar detect if doing periodic cal
	 * (the test-tones utilized during cal can trigger
	 * radar detect)
	 * NEED TO BE HERE AFTER READING DATA FROM (CLEAR) THE FIFO
	 */
	if (ISNPHY(pi)) {
	  if (pi->u.pi_nphy->nphy_rxcal_active) {
	    pi->u.pi_nphy->nphy_rxcal_active = FALSE;
	    PHY_RADAR(("DOING RXCAL, SKIP RADARS\n"));
	    goto radar_end;
	  }
	}
	if (ISACPHY(pi)) {
	  if (pi->u.pi_acphy->radar_cal_active) {
	    pi->u.pi_acphy->radar_cal_active = FALSE;
	    PHY_RADAR(("DOING CAL, SKIP RADARS\n"));
	    goto radar_end;
	  }
	}

	/*
	 * Reject if no pulses recorded
	 */
	if (ant_num == 2) {
		if ((rt->length_input[0] < 1) && (rt->length_input[1] < 1)) {
			goto radar_end;
		}
	} else {
		if (rt->length_input[0] < 1) {
			goto radar_end;
		}
	}

	if (rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_FIFO_OUTPUT) {
		for (ant = 0; ant < ant_num; ant++) {
			PHY_RADAR(("\nPulses read from PHY FIFO"));
			PHY_RADAR(("\nAnt %d: %d pulses, ", ant, rt->length_input[ant]));

			phy_radar_output_pulses(pi, rt->pulses_input[ant], rt->length_input[ant],
				rparams->radar_args.feature_mask, TRUE);
		}
	}

	/* START LONG PULSES (BIN5) DETECTION */
	if (rparams->radar_args.feature_mask & RADAR_FEATURE_FCC_DETECT) {	/* if fcc */
		phy_radar_detect_fcc5(pi, st, rt, sec_pll, bw80_80_mode, radar_detected);

		if (radar_detected->radar_type == RADAR_TYPE_FCC_5) {
			det_type = RADAR_TYPE_FCC_5;
			goto radar_end;
		}
	}

	/* START SHORT PULSES (NON-BIN5) DETECTION */
	/* remove "noise" pulses with  pw > 400 and fm < 244 */
	if (ISACPHY(pi) && TONEDETECTION) {
	for (ant = 0; ant < ant_num; ant++) {
		wr_ptr = 0;
		mlength = rt->length_input[ant];
		for (i = 0; i < rt->length_input[ant]; i++) {
		  if (rt->pulses_input[ant][i].pw  < (rparams->radar_args.st_level_time & 0x0fff) ||
		      rt->pulses_input[ant][i].fm >
		      ((sec_pll == 0 && bw80_80_mode == 0) ?
		      rparams->radar_args.min_fm_lp : min_fm_lp_sc)) {
				rt->pulses_input[ant][wr_ptr] = rt->pulses_input[ant][i];
				++wr_ptr;
			} else {
				mlength--;
			}
		}	/* for mlength loop */
		rt->length_input[ant] = mlength;
	}	/* for ant loop */
	}
	/* Combine pulses that are adjacent */
	for (ant = 0; ant < ant_num; ant++) {
		for (k = 0; k < 2; k++) {
			mlength = rt->length_input[ant];
			if (mlength > 1) {
			for (i = 1; i < mlength; i++) {
				if (rparams->radar_args.feature_mask & RADAR_FEATURE_NEWJP_DETECT) {
					/* newJP radar is composed with short + long pulses,
					   short and long pulses will be combined if we use
					   LP's max_pw as check condition, so still use SP's
					   max_pw and prev_interval for newJP radar
					*/
					deltat = ABS((int32)(rt->pulses_input[ant][i].interval -
						((i == 1) ? rt->pulses_input[ant][i-1].interval :
						prev_interval)));
					prev_interval = rt->pulses_input[ant][i].interval;
					/* extended pulse combination for longer split pulses */
					if ((rt->pulses_input[ant][i-1].pw <= 300) &&
						(deltat >= 1400) && (deltat <= 3000)) {
						ext_pulse_cmb_intv = (rparams->radar_args.max_deltat
							>> 1) & 0xefff;
						/* extended combination for next pulse only */
						ext_cmb_pulse_idx = i + 1;
					}
					pulse_cmb_intv = (i == ext_cmb_pulse_idx) ?
						ext_pulse_cmb_intv :
						(int32)rparams->radar_args.max_pw;
				} else {
					deltat = ABS((int32)(rt->pulses_input[ant][i].interval -
						rt->pulses_input[ant][i-1].interval));
					pulse_cmb_intv = (int32)rparams->radar_args.max_pw;
				}
				/* combine fc/pw/fm if deltat <= pulse_cmb_intv */
				if (deltat <= pulse_cmb_intv) {
					if (ISACPHY(pi) && TONEDETECTION &&
					    PHY_RADAR_FIFO_SUBBAND_FORMAT(pi)) {
						wlc_phy_radar_fc_chirp_cmb(
							&rt->pulses_input[ant][i-1],
							&rt->pulses_input[ant][i]);
					}

					if (ISNPHY(pi) || ISACPHY(pi)) {
						rt->pulses_input[ant][i-1].pw =
							ABS((int32)(
							rt->pulses_input[ant][i].interval -
							rt->pulses_input[ant][i-1].interval)) +
							rt->pulses_input[ant][i].pw;
					} else {
#ifdef BCMDBG
						/* print pulse combining debug messages */
						PHY_RADAR(("*%d,%d,%d ",
							rt->pulses_input[ant][i].interval -
							rt->pulses_input[ant][i-1].interval,
							rt->pulses_input[ant][i].pw,
							rt->pulses_input[ant][i-1].pw));
#endif
						if (rparams->radar_args.feature_mask &
								RADAR_FEATURE_USE_MAX_PW) {
							rt->pulses_input[ant][i-1].pw =
								(rt->pulses_input[ant][i-1].pw >
								rt->pulses_input[ant][i].pw ?
								rt->pulses_input[ant][i-1].pw :
								rt->pulses_input[ant][i].pw);
						} else {
							rt->pulses_input[ant][i-1].pw =
								rt->pulses_input[ant][i-1].pw +
								rt->pulses_input[ant][i].pw;
						}
					}

					/* Combine fm */
					if (ISACPHY(pi) && TONEDETECTION) {
						rt->pulses_input[ant][i-1].fm =
							(rt->pulses_input[ant][i-1].fm >
							rt->pulses_input[ant][i].fm) ?
							rt->pulses_input[ant][i-1].fm :
							rt->pulses_input[ant][i].fm;
					} else {
						rt->pulses_input[ant][i-1].fm =
							rt->pulses_input[ant][i-1].fm +
							rt->pulses_input[ant][i].fm;
					}

					for (j = i; j < mlength - 1; j++) {
						rt->pulses_input[ant][j] =
							rt->pulses_input[ant][j+1];
					}
					mlength--;
					rt->length_input[ant]--;
					if (rparams->radar_args.feature_mask &
						RADAR_FEATURE_NEWJP_DETECT) {
						i--;
					}
				}
			} /* for i < mlength */
			} /* mlength > 1 */
		}
	}
	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0 &&
	    rt->length_input[0] > 0) {	/* short pulses */
		for (ant = 0; ant < ant_num; ant++) {
			PHY_RADAR(("\nShort Pulse After combining adjacent pulses"));
			PHY_RADAR(("\nAnt %d: %d pulses, ", ant, rt->length_input[ant]));

			phy_radar_output_pulses(pi, rt->pulses_input[ant], rt->length_input[ant],
				rparams->radar_args.feature_mask, TRUE);
		}
	}

	for (ant = 0; ant < ant_num; ant++) {

	bzero(rt->pulses, sizeof(rt->pulses));
	rt->length = 0;

	/* Use data from one of the antenna */
	for (i = 0; i < rt->length_input[ant]; i++) {
		rt->pulses[rt->length] = rt->pulses_input[ant][i];
		rt->length++;
	}

	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0 &&
	    rt->length > 0) {	/* short pulses */
		PHY_RADAR(("\nShort pulses after use data from one of two antennas"));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(pi, rt->pulses, rt->length,
			rparams->radar_args.feature_mask, TRUE);
	}

	/* remove pulses spaced less than lower byte of "blank" */
	for (i = 1; i < rt->length; i++) {
		deltat = (int32)(rt->pulses[i].interval - rt->pulses[i-1].interval);
		if (deltat < (ISACPHY(pi) ? ((int32)rparams->radar_args.blank & 0xff) : 20)) {
			for (j = i; j < (rt->length - 1); j++) {
				rt->pulses[j] = rt->pulses[j + 1];
			}
			rt->length--;
		}
	}

	if (PHY_RADAR_FIFO_SUBBAND_FORMAT(pi) &&
		!(rparams->radar_args.feature_mask & RADAR_FEATURE_NEWJP_DETECT)) {
		/* remove entries chirp greater than min_chirp */
		j = 0;
		for (i = 0; i < rt->length; i++) {
			if (rt->pulses[i].chirp <= ETSI4_CHIRP) {
				rt->pulses[j] = rt->pulses[i];
				j++;
			}
		}
		rt->length = j;
	}

	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0 &&
	    rt->length > 0) {	/* short pulses */
		PHY_RADAR(("\nShort pulses after removing pulses with interval < %d",
		          rparams->radar_args.blank & 0xff));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(pi, rt->pulses, rt->length,
			rparams->radar_args.feature_mask, TRUE);
	}

	/*
	 * filter based on pulse width
	 */
	j = 0;
	for (i = 0; i < rt->length; i++) {
		if ((rt->pulses[i].pw >= rparams->radar_args.min_pw) &&
		    (rt->pulses[i].pw <= ((rparams->radar_args.feature_mask &
			RADAR_FEATURE_NEWJP_DETECT) ? MAX_PW_NEWJP :
			rparams->radar_args.max_pw))) {
			rt->pulses[j] = rt->pulses[i];
			j++;
		}
	}
	rt->length = j;

	if (ISACPHY(pi) && TONEDETECTION) {
		j = 0;
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			fm_thrs = 0;
		} else {
			fm_thrs = -50;
		}
		for (i = 0; i < rt->length; i++) {
			if (radar_thrs2->fm_chk_opt & EXT_FM_ABS_CHK_SP) {
				if (radar_thrs2->fm_chk_opt & EXT_FM_CHK_DBG) {
					fm_chk_pw_thrs1 = (radar_thrs2->fm_chk_pw >> 8) & 0xff;
					fm_chk_pw_thrs2 = radar_thrs2->fm_chk_pw & 0xff;
				}
				bw_idx = CHSPEC_IS20(pi->radio_chanspec) ? 0 :
					CHSPEC_IS40(pi->radio_chanspec) ? 3 :
					CHSPEC_IS80(pi->radio_chanspec) ? 6 : 9;

				if (rt->pulses[i].pw < fm_chk_pw_thrs1) {
					fm_thrs_idx = bw_idx;
					fm_thrs_dbg = radar_thrs2->fm_thresh_sp1;
				} else if (rt->pulses[i].pw >= fm_chk_pw_thrs1 &&
					rt->pulses[i].pw < fm_chk_pw_thrs2) {
					fm_thrs_idx = bw_idx + 1;
					fm_thrs_dbg = radar_thrs2->fm_thresh_sp2;
				} else if (rt->pulses[i].pw >= fm_chk_pw_thrs2) {
					fm_thrs_idx = bw_idx + 2;
					fm_thrs_dbg = radar_thrs2->fm_thresh_sp3;
				}

				if (radar_thrs2->fm_chk_opt & EXT_FM_CHK_DBG) {
					fm_thrs = (sec_pll == TRUE) ?
						(int16)radar_thrs2->fm_thresh_p1c : fm_thrs_dbg;
				} else {
					if (sec_pll == TRUE) {
						fm_thrs = -5;
					} else {
						if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
							fm_thrs = fm_thrs_sp_6715[fm_thrs_idx];
						} else {
							fm_thrs = PHY_LESI_ON(pi) ?
								fm_thrs_sp_lesi[fm_thrs_idx] :
								fm_thrs_sp[fm_thrs_idx];
						}
					}
				}

				if (rt->pulses[i].fm >= fm_thrs) {
					rt->pulses[j] = rt->pulses[i];
					j++;
				}
				else {
					if ((rparams->radar_args.feature_mask &
						RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0) {
						PHY_RADAR(("\nShort pulses fail absolute"
						" fm value check"));
					}
				}
			} else {
				if (rt->pulses[i].fm >= fm_thrs) {
					rt->pulses[j] = rt->pulses[i];
					j++;
				}
			}
		}
		rt->length = j;
	}

	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_SHORT_PULSE) != 0) {
		PHY_RADAR(("\nShort pulses after removing pulses with pw outside [%d, %d]",
		           rparams->radar_args.min_pw, ((rparams->radar_args.feature_mask &
		           RADAR_FEATURE_NEWJP_DETECT) ? MAX_PW_NEWJP :
		           rparams->radar_args.max_pw)));
		PHY_RADAR((" or fm < %d", fm_thrs));
		PHY_RADAR(("\n%d pulses, ", rt->length));

		phy_radar_output_pulses(pi, rt->pulses, rt->length,
			rparams->radar_args.feature_mask, TRUE);
	}

	if (ISACPHY(pi) && TONEDETECTION && PHY_RADAR_FIFO_SUBBAND_FORMAT(pi) &&
		!(rparams->radar_args.feature_mask & RADAR_FEATURE_NEWJP_DETECT)) {
		if (sec_pll == FALSE && bw80_80_mode == FALSE &&
			((st->rparams.radar_thrs2.notradar_enb >> 2) & 0x1)) {
			j = 0;
			for (i = 0; i < rt->length; i++) {
				if ((rt->pulses[i].notradar <=
					st->rparams.radar_thrs2.max_notradar)) {
					rt->pulses[j] = rt->pulses[i];
					j++;
				}
			}
			rt->length = j;
		} else {
			j = 0;
			for (i = 0; i < rt->length; i++) {
				if ((rt->pulses[i].notradar <=
					st->rparams.radar_thrs2.max_notradar_sc)) {
					rt->pulses[j] = rt->pulses[i];
					j++;
				}
			}
			rt->length = j;
		}
	}

	if (rt->length == 0)
		continue;

/*
	ASSERT(rt->length <= RDR_LIST_SIZE);
*/
	if (rt->length > RDR_LIST_SIZE) {
		rt->length = RDR_LIST_SIZE;
		PHY_RADAR(("WARNING: radar rt->length=%d > list_size=%d\n",
			rt->length, RDR_LIST_SIZE));
	}

	det_type = RADAR_TYPE_NONE;

	/* convert pulses[].interval from tstamps to intervals */
	--rt->length;
	for (j = 0; j < rt->length; j++) {
		rt->pulses[j].interval = ABS((int32)(rt->pulses[j + 1].interval -
		                                     rt->pulses[j].interval));
	}

	wlc_phy_radar_detect_run_epoch(pi, rt, rparams, rt->length,
		&det_type, &nconsecq_pulses, &detected_pulse_index,
		&min_detected_pw, &max_detected_pw,
		&min_detected_interval, &max_detected_interval,
		&avg_fc, &var_fc, blank_time, sec_pll);

	if ((rparams->radar_args.feature_mask & RADAR_FEATURE_DEBUG_PRUNED_SHORT_PULSE) != 0) {
		PHY_RADAR(("\nShort pulses after pruning (filtering)"));

		phy_radar_output_pulses(pi, rt->pulses, rt->length,
			rparams->radar_args.feature_mask, TRUE);

		PHY_RADAR(("nconsecq_pulses=%d max_pw_delta=%d min_pw=%d max_pw=%d\n",
			nconsecq_pulses, max_detected_pw - min_detected_pw, min_detected_pw,
			max_detected_pw));
	}

	if (min_detected_interval != 0 || det_type != RADAR_TYPE_NONE) {
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
			(ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
			!ACMAJORREV_128(pi->pubpi->phy_rev))) {
			PHY_RADAR(("\n-- Output Pruned Pulses for Detected radar only -- "));
			PHY_RADAR(("\nPruned Intv: "));
			for (i = 0; i < rt->length; i++) {
				if (rparams->radar_args.feature_mask &
					RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
					PHY_RADAR(("%d-%d ", rt->pulses[i].interval/20, i));
				} else {
					PHY_RADAR(("%d-%d ", rt->pulses[i].interval, i));
				}
			}
			PHY_RADAR(("\n"));

			PHY_RADAR(("Pruned PW:  "));
			for (i = 0; i <  rt->length; i++) {
				if (rparams->radar_args.feature_mask &
					RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
					PHY_RADAR(("%i-%d ", rt->pulses[i].pw/20, i));
				} else {
					PHY_RADAR(("%i-%d ", rt->pulses[i].pw, i));
				}
			}
			PHY_RADAR(("\n"));

			PHY_RADAR(("Pruned FM:  "));
			for (i = 0; i <  rt->length; i++) {
				PHY_RADAR(("%i-%d ", rt->pulses[i].fm, i));
			}

			PHY_RADAR(("\n"));

			PHY_RADAR(("Pruned Notradar:  "));
			for (i = 0; i <  rt->length; i++) {
				PHY_RADAR(("%i-%d ", rt->pulses[i].notradar, i));
			}
			PHY_RADAR(("\n"));

			PHY_RADAR(("Pruned Fc:  "));
			for (i = 0; i <  rt->length; i++) {
				PHY_RADAR(("%i-%d ", rt->pulses[i].fc, i));
			}
			PHY_RADAR(("\n"));

			PHY_RADAR(("Pruned Chirp:  "));
			for (i = 0; i <  rt->length; i++) {
				PHY_RADAR(("%i-%d ", rt->pulses[i].chirp, i));
			}

			PHY_RADAR(("\n"));
		} else {
			PHY_RADAR(("\nPruned %d pulses, ", rt->length));

			PHY_RADAR(("\n(Interval, PW, FM) = [ "));
			for (i = 0; i < rt->length; i++) {
				if (rparams->radar_args.feature_mask &
					RADAR_FEATURE_DEBUG_TIME_UNIT_US) {
					PHY_RADAR(("(%u, %u, %d) ",
						rt->pulses[i].interval, rt->pulses[i].pw/20,
						rt->pulses[i].fm));
				} else {
					PHY_RADAR(("(%u, %u, %d) ",
						rt->pulses[i].interval, rt->pulses[i].pw,
						rt->pulses[i].fm));
				}
			}
			PHY_RADAR(("];\n"));
		}
		PHY_RADAR(("det_idx=%d pw_delta=%d min_pw=%d max_pw=%d\n",
				detected_pulse_index,
				max_detected_pw - min_detected_pw, min_detected_pw,
				max_detected_pw));

		bzero(rt_status->intv, sizeof(rt_status->intv));
		bzero(rt_status->pw, sizeof(rt_status->pw));
		bzero(rt_status->fm, sizeof(rt_status->fm));
		bzero(rt_status->fc, sizeof(rt_status->fc));
		bzero(rt_status->chirp, sizeof(rt_status->chirp));
		for (i = 0; i < rt->length; i++) {
			if (i >= detected_pulse_index && i < detected_pulse_index + 10) {
				rt_status->intv[i - detected_pulse_index] =
					rt->pulses[i].interval;
				rt_status->pw[i - detected_pulse_index] = rt->pulses[i].pw;
				rt_status->fm[i - detected_pulse_index] = rt->pulses[i].fm;
				rt_status->fc[i - detected_pulse_index] = rt->pulses[i].fc;
				rt_status->chirp[i - detected_pulse_index] = rt->pulses[i].chirp;
			}
		}

		deltat2 = (uint32) (pi->sh->now - rlpt->last_detection_time);
		/* detection not valid if detected pulse index too large */
		if (detected_pulse_index < ((rparams->radar_args.ncontig) & 0x3f)) {
			rlpt->last_detection_time = pi->sh->now;
		}
		/* reject detection spaced more than 3 minutes and detected pulse index too larg */
		if (((uint32) deltat2 < (rparams->radar_args.fra_pulse_err & 0xff)*60 ||
			(*first_radar_indicator == 1 && (uint32) deltat2 < 30*60)) &&
			(detected_pulse_index < ((rparams->radar_args.ncontig) & 0x3f))) {
			if (PHY_RADAR_FIFO_SUBBAND_FORMAT(pi)) {
				if (det_type == RADAR_TYPE_ETSI_4) {
					//assign chirp value to 5MHz in EU Type4 radar
					max_detected_chirp = ETSI4_CHIRP;
				} else {
					max_detected_chirp = 0;
				}
			}

			//check fc variation
			if (var_fc > st->rparams.radar_thrs2.fc_varth_sb ||
				det_type == RADAR_TYPE_JP4 ||
				det_type == RADAR_TYPE_FCC_6) {
				if (CHSPEC_IS20(chanspec)) {
					radar_detected->subband = ALLSUBBAND_BW20;
				} else if (CHSPEC_IS40(chanspec)) {
					radar_detected->subband = ALLSUBBAND_BW40;
				} else if (CHSPEC_IS80(chanspec) ||
					PHY_AS_80P80(pi, pi->radio_chanspec)) {
					radar_detected->subband = ALLSUBBAND_BW80;
				} else if ((CHSPEC_CHANNEL(chanspec) == 50) &&
					(avg_fc >= 0)) {
					radar_detected->subband = ALLSUBBAND_BW160_CH50;
				} else if (CHSPEC_CHANNEL(chanspec) == 114) {
					radar_detected->subband = ALLSUBBAND_BW160_CH114;
				}
			} else {
				radar_detected->subband =
					wlc_phy_radar_fc_chirp_2_subband(pi,
						avg_fc, max_detected_chirp, sec_pll, 0);
			}
			rlpt->subband_result = radar_detected->subband;

			PHY_RADAR(("Type %d Radar Detection. Detected pulse index=%d"
				" nconsecq_pulses=%d."
				" Time from last detection = %u, = %dmin %dsec \n",
				det_type, detected_pulse_index, nconsecq_pulses,
				deltat2, deltat2/60, deltat2%60));
			rt_status->detected = ((CHSPEC_CHANNEL(chanspec) == 50) &&
				((rlpt->subband_result & 0x0f) == 0)) ? FALSE : TRUE;
			rt_status->count = rt_status->count + 1;
			rt_status->pretended = FALSE;
			rt_status->radartype = det_type;
			rt_status->timenow = (uint32) pi->sh->now;
			rt_status->timefromL = (uint32) deltat2;
			rt_status->detected_pulse_index =  detected_pulse_index;
			rt_status->nconsecq_pulses = nconsecq_pulses;
			rt_status->ch = pi->radio_chanspec;
			*first_radar_indicator = 0;

			radar_detected->radar_type = det_type;
			radar_detected->min_pw = (uint16)min_detected_pw;	/* in 20 * usec */
			radar_detected->max_pw = (uint16)max_detected_pw;	/* in 20 * usec */
			radar_detected->min_pri =
				(uint16)((min_detected_interval + 10) / 20);	/* in usec */
			radar_detected->max_pri =
				(uint16)((max_detected_interval + 10) / 20);	/* in usec */
		} else {
			if (RADAR_FEATURE_DEBUG_REJECTED_RADAR) {
				PHY_RADAR(("SKIPPED false Type %d Radar Detection."
					" min_pw=%d pw_delta=%d pri=%u"
					" nconsecq_pulses=%d. Time from last"
					" detection = %u, = %dmin %dsec",
					det_type, min_detected_pw,
					max_detected_pw - max_detected_pw,
					min_detected_interval, nconsecq_pulses, deltat2,
					deltat2 / 60, deltat2 % 60));
				if (detected_pulse_index < ((rparams->radar_args.ncontig) & 0x3f) -
					rparams->radar_args.npulses)
					PHY_RADAR((". Detected pulse index: %d\n",
						detected_pulse_index));
				else
					PHY_ERROR((". Detected pulse index too high: %d\n",
						detected_pulse_index));
			}
			/* skipping false detection, return radar type none */
			det_type = RADAR_TYPE_NONE;
		}
		break;
	}
	}	/* end for (ant = 0; ant < GET_RDR_NANTENNAS(pi); ant++) */

radar_end:
	phy_cache_release_reuse_buffer(pi->cachei, rt);

	return det_type;
}
