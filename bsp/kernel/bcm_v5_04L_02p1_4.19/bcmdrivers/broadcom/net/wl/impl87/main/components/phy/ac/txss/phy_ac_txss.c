/*
 * ACPHY Transmitter Spectral Shaping module implementation
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
 * $Id: phy_ac_txss.c 877903 2020-05-14 02:47:05Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>
#include <phy_dbg.h>
#include <phy_ac.h>
#include <phy_ac_info.h>
#include <phy_type_txss.h>
#include <phy_rstr.h>
#include <phy_utils_var.h>
#include <phy_utils_reg.h>
#include <phy_utils_channel.h>
#include <wlc_phyreg_ac.h>
#include <wlc_phytbl_ac.h>
#include <phy_mem.h>
#include <d11.h>
#include <bcmwifi_rspec.h>

#ifdef WLC_TXSHAPER

#define TXS_USE_FIXED_IBO

// #define TXS_DEBUG
// #define TXS_DEBUGCODE

// table offsets
#define TXS_TBL_OFFS_PRG_WORD0	48u
#define TXS_TBL_OFFS_PRG_WORD1	55u
#define TXS_TBL_OFFS_RI2RGTBL	64u

#define TXS_PER_CHAN_TABLE_LEN	61u
#define TXS_TBL_BIT_SHAPER_ENABLE 48u

#define TXS_N_RATE_GROUPS	6u
#define TXS_N_RATE_GROUPS_USED	6u
#define TXS_N_CHAN_PROFILES	10u
#define TXS_N_PER_CHAN_REGS	11u
#define TXS_N_BITS_PER_PSATIDX	8u
#define TXS_NVRAM_RATE_CHAN_NUM_ENTRIES 6u
#define TXS_MAX_SUPPORTED_BW_IDX 2u

#ifndef TXSHAPER_EN_2G_NV_DEFAULT
#define TXSHAPER_EN_2G_NV_DEFAULT 0
#endif

#ifndef TXSHAPER_EN_5G_NV_DEFAULT
#define TXSHAPER_EN_5G_NV_DEFAULT 0
#endif

#ifdef TXS_USE_FIXED_IBO
#ifdef TXSHAPER_RATE_CHAN_PROFILE_2G_NV_DEFAULT
#undef TXSHAPER_RATE_CHAN_PROFILE_2G_NV_DEFAULT
#endif
#define TXSHAPER_RATE_CHAN_PROFILE_2G_NV_DEFAULT 1
#endif /* TXS_USE_FIXED_IBO */

#ifndef TXSHAPER_RATE_CHAN_PROFILE_5G_NV_DEFAULT
#define TXSHAPER_RATE_CHAN_PROFILE_5G_NV_DEFAULT 1
#endif

#define TXS_PSAT_MARGIN_GROUPS_2G 5u

#ifdef TXS_USE_FIXED_IBO
#define TXS_IBO_3dB 48u
#define TXS_IBO_4dB 64u
#define TXS_IBO_6dB 96u
#endif /* TXS_USE_FIXED_IBO */

// precalculated phase_step and initial phase offset table
// values for specified filter cut-offs(for 20 MHz phybw)
#define TXS_FIR_CUTOFF_10p5  0xFB3419
#define TXS_FIR_CUTOFF_m10p5 0x04DBE7
#define TXS_FIR_CUTOFF_11p5  0x71A44D
#define TXS_FIR_CUTOFF_m11p5 0x8E6BB3
#define TXS_FIR_CUTOFF_11p0  0xB66433
#define TXS_FIR_CUTOFF_m11p0 0x49ABCD

typedef enum {NONBE_20, NONBE_40, NONBE_80, BE_20, BE_40, BE_80,
	NONBE_2G20, NONBE_2G40, BE_2G20, BE_2G40}
	txs_chan_profile_index_t;

static int8 phy_ac_txss_txshaper_psat_margin(phy_info_t *pi, bool bandedge);
static void	phy_ac_txss_tshaper_set_table_bits(phy_info_t *pi,
uint8 offset, uint8 start_bit, uint8 nbits, uint64 val);

#endif /* WLC_TXSHAPER */

typedef struct phy_ac_txss_mem phy_ac_txss_mem_t;

typedef struct phy_ac_txshaper_info {
	/* 0 - disable; 1 - enable; 2 - (enable and) bypass */
	uint8 txshaper_en;
	/* -1: auto (follow SROM); 0 - disable; 1 - enable; 2 - (enable and) bypass */
	int16 txshaper_en_ovr;
#ifdef WLC_TXSHAPER
	uint8 txs_psat_index2g[PHY_CORE_MAX][CH_2G_GROUP];
	uint8 txs_psat_index5g[PHY_CORE_MAX][CH_5G_4BAND];
	uint8 txs_rate_chan_en[TXS_NVRAM_RATE_CHAN_NUM_ENTRIES];
	bool  txs_shaper_en_2g;
	bool  txs_shaper_en_5g;
	bool  txs_shaper_en_6g;
	bool  txs_shaper_bypass;
	bool  txs_shaper_dis_2g_nonbndg;
	bool  txs_shaper_dis_5g_nonbndg;
	bool  txs_shaper_en_unii3;
	bool  txs_shaper_en_unii4;
	const uint8 *p_txs_rate_chan_en;
	uint8 txs_shprrealfrmdly[3];
	int8  txs_psat_margin_adj[TXS_PSAT_MARGIN_GROUPS_2G];
#endif
} phy_ac_txshaper_info_t;

/* module private states */
struct phy_ac_txss_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_txss_info_t *txssi;
	phy_ac_txshaper_info_t *txshaperi;
};

struct phy_ac_txss_mem {
	phy_ac_txss_info_t info;
#ifdef WLC_TXSHAPER
	phy_ac_txshaper_info_t txshaperi;
#endif /* WLC_TXSHAPER */
/* add other variable size variables here at the end */
};

/* local functions */
static void phy_ac_txss_std_params_attach(phy_ac_txss_info_t *txssaci);

#ifdef WLC_TXSHAPER
static void phy_ac_txss_txshaper_adjust_fir_cutoff(phy_info_t *pi, int16 chan);
static void phy_ac_txss_txshaper_left_sided_shaping(phy_info_t *pi, bool left_shaping);
static void phy_ac_txss_nvram_read(phy_ac_txss_info_t *txssaci);
#ifdef TXS_USE_FIXED_IBO
static void phy_ac_txss_txshaper_mid_rates_tuning(phy_info_t *pi, int16 chan);
#endif /* TXS_USE_FIXED_IBO */
#endif /* WLC_TXSHAPER */

/* register phy type specific implementation */
phy_ac_txss_info_t *
BCMATTACHFN(phy_ac_txss_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_txss_info_t *txssi)
{
	phy_ac_txss_info_t *txssaci = NULL;
	phy_type_txss_fns_t fns;
	phy_ac_txss_mem_t *mem;

	PHY_TRACE(("phy_ac_txss_register_impl\n"));

	/* allocate all storage together */
	if ((mem = phy_malloc(pi, sizeof(phy_ac_txss_mem_t))) == NULL) {
		PHY_ERROR(("phy_ac_txss_mem_t: phy_malloc failed\n"));
		goto fail;
	}
	txssaci = &mem->info;
#ifdef WLC_TXSHAPER
	txssaci->txshaperi = &mem->txshaperi;
#else
	txssaci->txshaperi = NULL;
#endif /* WLC_TXSHAPER */

	txssaci->pi = pi;
	txssaci->aci = aci;
	txssaci->txssi = txssi;

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctx = txssaci;

	phy_ac_txss_std_params_attach(txssaci);

	phy_txss_register_impl(txssi, &fns);

	return txssaci;

	/* error handling */
fail:
	if (txssaci != NULL)
		phy_mfree(pi, txssaci, sizeof(phy_ac_txss_mem_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_txss_unregister_impl)(phy_ac_txss_info_t *txssaci)
{
	phy_info_t *pi;
	phy_txss_info_t *txssi;

	ASSERT(txssaci);
	pi = txssaci->pi;
	txssi = txssaci->txssi;

	PHY_TRACE(("phy_ac_txss_unregister_impl\n"));

	/* unregister from common */
	phy_txss_unregister_impl(txssi);

	phy_mfree(pi, txssaci, sizeof(phy_ac_txss_mem_t));
}

static void
phy_ac_txss_set_shaper_en_srom(phy_ac_txss_info_t *txssaci)
{
	phy_info_t *pi = txssaci->pi;

	txssaci->txshaperi->txshaper_en = CHSPEC_IS2G(pi->radio_chanspec) ?
	        txssaci->txshaperi->txs_shaper_en_2g : CHSPEC_IS5G(pi->radio_chanspec) ?
	        txssaci->txshaperi->txs_shaper_en_5g : txssaci->txshaperi->txs_shaper_en_6g;

	// additional restrictions of txShaper on/off setting on top of srom field
	if (txssaci->txshaperi->txshaper_en) {
		uint8 ch = CHSPEC_CHANNEL(pi->radio_chanspec);

		// for 2g non-BNDG channels, bypass TxShaper based on srom setting
		if (CHSPEC_IS2G(pi->radio_chanspec) &&
		    ((txssaci->txshaperi->txs_shaper_dis_2g_nonbndg &&
		      ((IS20MHZ(pi) && (ch > 2 && ch < 10)) ||
		       (IS40MHZ(pi) && (ch > 3 && ch < 9)))) ||
		     (IS40MHZ(pi) && ch >= 11))) {  /* 40MHz chan11m always disable TxShaper */
			txssaci->txshaperi->txshaper_en = 0;
		}

		if (CHSPEC_IS5G(pi->radio_chanspec)) {
			uint8 is_bndg;

			is_bndg = IS20MHZ(pi) && (ch == 36 || ch == 64 || ch == 100 || ch == 149);
			is_bndg |= IS40MHZ(pi) && (ch == 38 || ch == 62 || ch == 102 || ch == 151);
			is_bndg |= IS80MHZ(pi) && (ch == 42 || ch == 58 || ch == 106 || ch == 155);
			is_bndg |= IS160MHZ(pi);
			is_bndg |= ch > 165; // enabling txShaper for all UNII-4 channels

			if (IS20MHZ(pi) && (ch == 140)) {
				txssaci->txshaperi->txshaper_en = 0;
			}
			if (txssaci->txshaperi->txs_shaper_dis_5g_nonbndg && !is_bndg) {
				txssaci->txshaperi->txshaper_en = 0;
			}
			if ((txssaci->txshaperi->txs_shaper_en_unii3 == 0) &&
				(ch >= 149 && ch <= 165)) {
				txssaci->txshaperi->txshaper_en = 0;
			}
			if ((txssaci->txshaperi->txs_shaper_en_unii4 == 0) && (ch > 165)) {
				txssaci->txshaperi->txshaper_en = 0;
			}
		}
	}

	if (txssaci->txshaperi->txshaper_en == 0 && txssaci->txshaperi->txs_shaper_bypass) {
		txssaci->txshaperi->txshaper_en = 2; /* enable but bypassed */
	}
}

static void
BCMATTACHFN(phy_ac_txss_std_params_attach)(phy_ac_txss_info_t *txssaci)
{
	txssaci->txshaperi->txshaper_en = 0;
	txssaci->txshaperi->txshaper_en_ovr = -1;

#ifdef WLC_TXSHAPER
	phy_ac_txss_nvram_read(txssaci);
	phy_ac_txss_set_shaper_en_srom(txssaci);
#endif /* WLC_TXSHAPER */
}

#ifdef WLC_TXSHAPER

static void
BCMATTACHFN(phy_ac_txss_nvram_read)(phy_ac_txss_info_t *txssaci)
{
	phy_info_t *pi = txssaci->pi;
	uint8 j, core;
	uint8 rate_chan;

	/* Program with the typical values across chips */
	FOREACH_CORE(pi, core) {
		txssaci->txshaperi->txs_psat_index2g[core][0] = 25;
		for (j = 0; j < CH_5G_4BAND; j++) {
			txssaci->txshaperi->txs_psat_index5g[core][j] = 16;
		}
	}

	/* Get psat margin adjust for 2g */
	if CHSPEC_IS2G(txssaci->pi->radio_chanspec) {
		for (j = 0; j < TXS_PSAT_MARGIN_GROUPS_2G; j++) {
			txssaci->txshaperi->txs_psat_margin_adj[j] = 0;
		}
	}

	/* disable TxShaper for 6715X/X2 */
	if (BCM6715X_PKG(pi->sh->sih->otpflag) || BCM6715X2_PKG(pi->sh->sih->otpflag)) {
		txssaci->txshaperi->txs_shaper_en_2g = 0;
		txssaci->txshaperi->txs_shaper_en_5g = 0;
		txssaci->txshaperi->txs_shaper_en_6g = 0;
		txssaci->txshaperi->txs_shaper_bypass = 0;
	} else {
		txssaci->txshaperi->txs_shaper_en_2g = PHY_GETINTVAR_DEFAULT_SLICE(pi,
				rstr_txs_shaper_en_2g, 0);
		txssaci->txshaperi->txs_shaper_en_5g = PHY_GETINTVAR_DEFAULT_SLICE(pi,
				rstr_txs_shaper_en_5g, 0);
		txssaci->txshaperi->txs_shaper_en_6g = PHY_GETINTVAR_DEFAULT_SLICE(pi,
				rstr_txs_shaper_en_6g, 0);
		txssaci->txshaperi->txs_shaper_bypass = PHY_GETINTVAR_DEFAULT_SLICE(pi,
				rstr_txs_shaper_bypass, 0);
	}

	txssaci->txshaperi->txs_shaper_dis_2g_nonbndg = PHY_GETINTVAR_DEFAULT_SLICE(pi,
			rstr_txs_shaper_dis_2g_nonbndg, 1);
	txssaci->txshaperi->txs_shaper_dis_5g_nonbndg = PHY_GETINTVAR_DEFAULT_SLICE(pi,
			rstr_txs_shaper_dis_5g_nonbndg, 1);
	txssaci->txshaperi->txs_shaper_en_unii3 = PHY_GETINTVAR_DEFAULT_SLICE(pi,
			rstr_txs_shaper_en_unii3, 1);
	txssaci->txshaperi->txs_shaper_en_unii4 = PHY_GETINTVAR_DEFAULT_SLICE(pi,
			rstr_txs_shaper_en_unii4, 1);
	rate_chan = PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_txs_chan_rate_en, 0);

	txssaci->txshaperi->p_txs_rate_chan_en = txssaci->txshaperi->txs_rate_chan_en;

	for (j = 0; j < TXS_NVRAM_RATE_CHAN_NUM_ENTRIES; j++) {
		txssaci->txshaperi->txs_rate_chan_en[j] = rate_chan & 0xe7;
		/* LSB=>MSB: HR,MR,LR,11B,VHR,MixR,rsvds rate groups
		 * turn off txshaper for 11b and mcs9/10/11 rate group
		 * currently, all channels have the same settting
		 */
	}
}

#endif /* WLC_TXSHAPER */

#ifdef WLC_TXSHAPER

// per channel per rate group tx shaper table values
// generated with tcl txs_gen_driver_tables/txs_dump_cur_drv_tbl
// or with tcl:  set list  [phy_read_table txshprcmnluttbl 61 0]; txs_dump_list $list 4 c
// 10 groups: 5G non-BE 20/40/80, BE 20/40/80, 2G non-BE 20/40, 2G BE 20/40MHz
// Note: CH13 profile will be used for all 2G BE 20MHz channels for better performance

static const uint64 k_shaper_prg_chan_table_0[TXS_PER_CHAN_TABLE_LEN] =
{ 0x32f903a3505ac, 0x32f903b0105ac, 0x32f90390105ac, 0x32f90390105ac,
0x32f903c0105ac, 0x32f903f0105ac, 0x32f903d0105ac, 0x32f903d0105ac,
0x33bc03d3773ac, 0x33bc03e0373ac, 0x33bc03c0373ac, 0x33bc03c0373ac,
0x33bc03f0373ac, 0x33bc0020373ac, 0x33bc0000373ac, 0x33bc0000373ac,
0x3d80001377300, 0x3d80002037300, 0x3d80000037300, 0x3d80000037300,
0x3d80003037300, 0x3d80006037300, 0x3d80004037300, 0x3d80004037300,
0xffc098237007f, 0x8500982030060, 0x5040982030030, 0x3d80982010500,
0x3d80982010500, 0x3d80982010500, 0x3d80982010500, 0x3d80982010500,
0x32f7038350590, 0x32f7039010590, 0x32f7037010590, 0x32f7037010590,
0x32f703a010590, 0x32f703d010590, 0x32f703b010590, 0x32f703b010590,
0x3d80000010000, 0x3d80000010000, 0x13d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x100000c0b0c0b, 0x10e400c0b0c0b, 0x10f000c0b0c0b, 0x100000c0b0c0b,
0x100000c0b0c0b, 0xdc00c0b0c0b, 0x100000c0b0c0b, 0x300700,
0x4004300700, 0x4644300700, 0x4a84300700, 0x42c4300700,
0x4704300700};
static const uint64 k_shaper_prg_chan_table_1[TXS_PER_CHAN_TABLE_LEN] =
{ 0x32f903a3505ac, 0x32f903b0105ac, 0x32f90390105ac, 0x32f90390105ac,
0x32f903c0105ac, 0x32f903f0105ac, 0x32f903d0105ac, 0x32f903d0105ac,
0x33bc03d3773ac, 0x33bc03e0373ac, 0x33bc03c0373ac, 0x33bc03c0373ac,
0x33bc03f0373ac, 0x33bc0020373ac, 0x33bc0000373ac, 0x33bc0000373ac,
0x3d80001377300, 0x3d80002037300, 0x3d80000037300, 0x3d80000037300,
0x3d80003037300, 0x3d80006037300, 0x3d80004037300, 0x3d80004037300,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x32f7038350590, 0x32f7039010590, 0x32f7037010590, 0x32f7037010590,
0x32f703a010590, 0x32f703d010590, 0x32f703b010590, 0x32f703b010590,
0x3d80000010000, 0x3d80000010000, 0x13d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x100000c0b0c0b, 0x10e400c0b0c0b, 0x10f000c0b0c0b, 0x100000c0b0c0b,
0x100000c0b0c0b, 0xdc00c0b0c0b, 0x100000c0b0c0b, 0x380580,
0x4004380580, 0x4644380580, 0x4a84380580, 0x40c4380580,
0x4704380580};
static const uint64 k_shaper_prg_chan_table_2[TXS_PER_CHAN_TABLE_LEN] =
{ 0x32f903a3505ac, 0x32f903c0105ac, 0x32f90390105ac, 0x32f90390105ac,
0x32f903c0105ac, 0x32f903f0105ac, 0x32f903d0105ac, 0x32f903d0105ac,
0x33bc03d3773ac, 0x33bc03f0373ac, 0x33bc03c0373ac, 0x33bc03c0373ac,
0x33bc03f0373ac, 0x33bc0020373ac, 0x33bc0000373ac, 0x33bc0000373ac,
0x3d80001377300, 0x3d80003037300, 0x3d80000037300, 0x3d80000037300,
0x3d80003037300, 0x3d80006037300, 0x3d80004037300, 0x3d80004037300,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x32f7038350590, 0x32f703a010590, 0x32f7037010590, 0x32f7037010590,
0x32f703a010590, 0x32f703d010590, 0x32f703b010590, 0x32f703b010590,
0x3d80000010000, 0x3d80000010000, 0x13d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x100000c0b0c0b, 0x10e400c0b0c0b, 0x10f000c0b0c0b, 0x100000c0b0c0b,
0x100000c0b0c0b, 0xdc00c0b0c0b, 0x100000c0b0c0b, 0x3c04c0,
0x40043c04c0, 0x46443c04c0, 0x4a843c04c0, 0x40c43c04c0,
0x47043c04c0};
static const uint64 k_shaper_prg_chan_table_3[TXS_PER_CHAN_TABLE_LEN] =
{ 0x32fa03a0105ac, 0x32f99b9cd05ac, 0x32fa039cd05ac, 0x32f96bacd05ac,
0x32f903f6905ac, 0x32f903f6905ac, 0x32f903f6905ac, 0x32f903f6905ac,
0x33bd03d0373ac, 0x33bc9bccf73ac, 0x33bd03ccf73ac, 0x33bc6bdcf73ac,
0x33bc0026b73ac, 0x33bc0026b73ac, 0x33bc0026b73ac, 0x33bc0026b73ac,
0x3d81001037300, 0x3d80980cf7300, 0x3d81000cf7300, 0x3d80681cf7300,
0x3d800066b7300, 0x3d800066b7300, 0x3d800066b7300, 0x3d800066b7300,
0xffc0cc0cf007f, 0xffc100203007f, 0x8501002030060, 0x3d80982010500,
0x37409820105dd, 0x34c09820105c0, 0x34c09820105c0, 0x33809820105ac,
0x32f8038010590, 0x32f79b7cd0590, 0x32f8037cd0590, 0x32f76b8cd0590,
0x32f703d690590, 0x32f703d690590, 0x32f703d690590, 0x32f703d690590,
0x3d80000010000, 0x3d80000010000, 0x13d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x150000c0b0c0b, 0x15e400c0b0c0b, 0x15f000c0b0c0b, 0x150000c0b0c0b,
0x150000c0b0c0b, 0x5dc00c0b0c0b, 0x150000c0b0c0b, 0x966a33,
0x4004966a33, 0x4644966a33, 0x4e84966a33, 0x42c4966a33,
0x4704966a33};
static const uint64 k_shaper_prg_chan_table_4[TXS_PER_CHAN_TABLE_LEN] =
{ 0x32fa37b0105ac, 0x32fa37a6905ac, 0x32fa039cd05ac, 0x32fa039cd05ac,
0x32f99ba3505ac, 0x32f937f6905ac, 0x32f937f6905ac, 0x32f937f6905ac,
0x33bd37e0373ac, 0x33bd37d6b73ac, 0x33bd03ccf73ac, 0x33bd03ccf73ac,
0x33bc9bd3773ac, 0x33bc3426b73ac, 0x33bc3426b73ac, 0x33bc3426b73ac,
0x3d81342037300, 0x3d813416b7300, 0x3d81000cf7300, 0x3d81000cf7300,
0x3d80981377300, 0x3d803466b7300, 0x3d803466b7300, 0x3d803466b7300,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x32f8379010590, 0x32f8378690590, 0x32f8037cd0590, 0x32f8037cd0590,
0x32f79b8350590, 0x32f737d690590, 0x32f737d690590, 0x32f737d690590,
0x3d80000010000, 0x3d80000010000, 0x13d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x170000c0b0c0b, 0x17e400c0b0c0b, 0x17f000c0b0c0b, 0x170000c0b0c0b,
0x170000c0b0c0b, 0x7dc00c0b0c0b, 0x170000c0b0c0b, 0xc80a80,
0x4004c80a80, 0x4644c80a80, 0x4a84c80a80, 0x40c4c80a80,
0x4704c80a80};
static const uint64 k_shaper_prg_chan_table_5[TXS_PER_CHAN_TABLE_LEN] =
{ 0x32fa03d3505ac, 0x32fa37a3505ac, 0x32fa37a3505ac, 0x32fa3790105ac,
0x32f9cf9cd05ac, 0x32f9379cd05ac, 0x32f9039cd05ac, 0x32f9039cd05ac,
0x33bd0003773ac, 0x33bd37d3773ac, 0x33bd37d3773ac, 0x33bd37c0373ac,
0x33bccfccf73ac, 0x33bc37ccf73ac, 0x33bc03ccf73ac, 0x33bc03ccf73ac,
0x3d81004377300, 0x3d81341377300, 0x3d81341377300, 0x3d81340037300,
0x3d80cc0cf7300, 0x3d80340cf7300, 0x3d80000cf7300, 0x3d80000cf7300,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x32f803b350590, 0x32f8378350590, 0x32f8378350590, 0x32f8377010590,
0x32f7cf7cd0590, 0x32f7377cd0590, 0x32f7037cd0590, 0x32f7037cd0590,
0x3d80000010000, 0x3d80000010000, 0x13d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x170000c0b0c0b, 0x17e400c0b0c0b, 0x17f000c0b0c0b, 0x170000c0b0c0b,
0x170000c0b0c0b, 0x7dc00c0b0c0b, 0x170000c0b0c0b, 0xc40b40,
0x4004c40b40, 0x4644c40b40, 0x4a84c40b40, 0x40c4c40b40,
0x4704c40b40};
static const uint64 k_shaper_prg_chan_table_6[TXS_PER_CHAN_TABLE_LEN] =
{ 0x33b8039350590, 0x33b803a010590, 0x33b8038010590, 0x33b8038010590,
0x33b803b010590, 0x33b803e010590, 0x33b803c010590, 0x33b803c010590,
0x33ba03b3700ac, 0x33ba03c0300ac, 0x33ba03a0300ac, 0x33ba03a0300ac,
0x33ba03d0300ac, 0x33ba0000300ac, 0x33ba03e0300ac, 0x33ba03e0300ac,
0x35c00013700c0, 0x35c00020300c0, 0x35c00000300c0, 0x35c00000300c0,
0x35c00030300c0, 0x35c00060300c0, 0x35c00040300c0, 0x35c00040300c0,
0xffc098237007f, 0x8500982030060, 0x5040982030030, 0x3d80982010500,
0x3d80982010500, 0x3d80982010500, 0x3d80982010500, 0x3d80982010500,
0x33b6037350590, 0x33b6038010590, 0x33b6036010590, 0x33b6036010590,
0x33b6039010590, 0x33b603c010590, 0x33b603a010590, 0x33b603a010590,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x100000c0b0c0b, 0x10e000c0b0c0b, 0x10e800c0b0c0b, 0x100000c0b0c0b,
0xc0b0c0b, 0xd800c0b0c0b, 0x100000c0b0c0b, 0xa00300700,
0x4004300700, 0x4044300700, 0x4a84300700, 0x4ac4300700,
0x4104300700};
static const uint64 k_shaper_prg_chan_table_7[TXS_PER_CHAN_TABLE_LEN] =
{ 0x33ae02f3700a4, 0x33ae0300300a4, 0x33ae02e0300a4, 0x33ae02e0300a4,
0x33ae0310300a4, 0x33ae0340300a4, 0x33ae0320300a4, 0x33ae0320300a4,
0x33ae02f3700a4, 0x33ae0300300a4, 0x33ae02e0300a4, 0x33ae02e0300a4,
0x33ae0310300a4, 0x33ae0340300a4, 0x33ae0320300a4, 0x33ae0320300a4,
0x33b70383700a4, 0x33b70390300a4, 0x33b70370300a4, 0x33b70370300a4,
0x33b703a0300a4, 0x33b703d0300a4, 0x33b703b0300a4, 0x33b703b0300a4,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x33ae02f350590, 0x33ae030010590, 0x33ae02e010590, 0x33ae02e010590,
0x33ae031010590, 0x33ae034010590, 0x33ae032010590, 0x33ae032010590,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x100000c0b0c0b, 0x10dc00c0b0c0b, 0x10dc00c0b0c0b, 0x10dc00c0b0c0b,
0xc0b0c0b, 0xdc00c0b0c0b, 0x100000c0b0c0b, 0xa00380580,
0x4004380580, 0x4044380580, 0x4084380580, 0x4ac4380580,
0x4104380580};
static const uint64 k_shaper_prg_chan_table_8[TXS_PER_CHAN_TABLE_LEN] =
{ 0x33b8377990590, 0x33b89b7cd0590, 0x33b89b7cd0590, 0x33b8377350590,
0x33b86b7010590, 0x33b9037010590, 0x33b8037010590, 0x33b7037010590,
0x33bb37a9b00ac, 0x33bb9bacf00ac, 0x33bb9bacf00ac, 0x33bb37a3700ac,
0x33bb6ba0300ac, 0x33bc03a0300ac, 0x33bb03a0300ac, 0x33ba03a0300ac,
0x35c13409b00c0, 0x35c1980cf00c0, 0x35c1980cf00c0, 0x35c13403700c0,
0x35c16800300c0, 0x35c20000300c0, 0x35c10000300c0, 0x35c00000300c0,
0x3d80982030000, 0x36009820300d0, 0x33800003505ac, 0x33800003505ac,
0x33800003505ac, 0x33800003505ac, 0x33800003505ac, 0x33800003505ac,
0x33b7376990590, 0x33b79b6cd0590, 0x33b79b6cd0590, 0x33b7376350590,
0x33b76b6010590, 0x33b8036010590, 0x33b7036010590, 0x33b6036010590,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x170000c0b0c0b, 0x17dc00c0b0c0b, 0x17e800c0b0c0b, 0x170000c0b0c0b,
0x70000c0b0c0b, 0x7d800c0b0c0b, 0x170000c0b0c0b, 0xa00fb341a,
0x4004fb341a, 0x4044fb341a, 0x4a84fb341a, 0x4ac4fb341a,
0x4104fb341a};
static const uint64 k_shaper_prg_chan_table_9[TXS_PER_CHAN_TABLE_LEN] =
{ 0x33b9378990590, 0x33b99b8cd0590, 0x33b99b8cd0590, 0x33b9378350590,
0x33b96b8010590, 0x33ba038010590, 0x33b9038010590, 0x33b8038010590,
0x33bc37b9b00ac, 0x33bc9bbcf00ac, 0x33bc9bbcf00ac, 0x33bc37b3700ac,
0x33bc6bb0300ac, 0x33bd03b0300ac, 0x33bc03b0300ac, 0x33bb03b0300ac,
0x35c13409b00c0, 0x35c1980cf00c0, 0x35c1980cf00c0, 0x35c13403700c0,
0x35c16800300c0, 0x35c20000300c0, 0x35c10000300c0, 0x35c00000300c0,
0x3d80982030000, 0x36009820300d0, 0x33800003505ac, 0x33800003505ac,
0x33800003505ac, 0x33800003505ac, 0x33800003505ac, 0x33800003505ac,
0x33b8377990590, 0x33b89b7cd0590, 0x33b89b7cd0590, 0x33b8377350590,
0x33b86b7010590, 0x33b9037010590, 0x33b8037010590, 0x33b7037010590,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x3d80000010000, 0x3d80000010000, 0x3d80000010000, 0x3d80000010000,
0x170000c0b0c0b, 0x17e000c0b0c0b, 0x17ec00c0b0c0b, 0x170000c0b0c0b,
0x70000c0b0c0b, 0x7dc00c0b0c0b, 0x170000c0b0c0b, 0xa0049abcc,
0x400449abcc, 0x404449abcc, 0x4a8449abcc, 0x4ac449abcc,
0x410449abcc};

/* 10 groups: (5G) non-BE 20/40/80, BE 20/40/80, (2G) non-BE 20/40, BE 20/40
 * Note: 2G 20 BE is using the channel 13 profile (most agressive)
 */
static const uint64 *k_shaper_prg_chan_tables[TXS_N_CHAN_PROFILES] = {
	k_shaper_prg_chan_table_0,
	k_shaper_prg_chan_table_1,
	k_shaper_prg_chan_table_2,
	k_shaper_prg_chan_table_3,
	k_shaper_prg_chan_table_4,
	k_shaper_prg_chan_table_5,
	k_shaper_prg_chan_table_6,
	k_shaper_prg_chan_table_7,
	k_shaper_prg_chan_table_8,
	k_shaper_prg_chan_table_9
};

/* Default phyreg values corresponding to the 10 groups */
static const int16 shaper_chan_reg_vals[TXS_N_PER_CHAN_REGS][TXS_N_CHAN_PROFILES] = {
	{  77,     77,     77,     64,    128,    205,     77,     77,      0,      0 },
	{   0,      0,      0,     64,     77,     77,      0,      0,    102,     77 },
	{  45,     45,     45,     96,     96,    101,     45,     45,     96,     96 },
	{-218,   -218,   -218,   -218,   -218,   -256,   -218,   -218,   -218,   -192 },
	{ 332,    332,    332,    552,    552,    586,    332,    332,    552,    552 },
	{  -2,     -2,     -2,     -2,     -2,     -2,     -2,     -2,     -2,     -2 },
	{   0,      0,      0,      0,      0,      0,      0,      0,      0,      0 },
	{   0,      0,      0,      0,      0,      0,      0,      0,      0,      0 },
	{   0,      0,      0,      0,      0,      0,      0,      0,      0,      0 },
	{   0,      0,      0,      0,      0,      0,      0,      0,      0,      0 },
	{   0,      0,      0,      0,      0,      0,      0,      0,      0,      0 }
};

#ifdef TXS_USE_FIXED_IBO
// Tuning for mid rates when using fixed IBO
static void
phy_ac_txss_txshaper_mid_rates_tuning(phy_info_t *pi, int16 chan)
{
	return;
#ifdef TXS_DEBUGCODE
	uint64 tbl_vals = 0;
	uint8 rpt_index;
	/* The following tunings were done for WCC chips
	 * skip these tunings as they have been done in TCL for BCA
	 */
	// Values partly generated using txs_rpt_shift_psat proc in shaperdev.tcl
	if (chan <= 14) {
		// adjust clip protect and scaleup to simulate IBO 4 when fixed IBO 3 is used
		// mid rate is assumed to be rate group 1 and there are 8 power levels in
		// rate power table
		// index in TXSHPRCMNLUTTBL that corresponds to IBO 3 is 10, after
		// accounting for 11n offset of 1 dB
		// read index 10
		// 10 to 11 covers IBO 3 indexing for all rates
		for (rpt_index = 10; rpt_index <= 11; rpt_index++) {
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, 1,
				rpt_index, 64, &tbl_vals);
			// Set bits order in increasing order start bits is needed to ensure
			// negative numbers are written correctly
			if (chan == 1) {
				// change scale up to -128
				setbits((uint8*) &tbl_vals, 8, 0, 8, -128);
			} else {
				// change scale up to -108
				setbits((uint8*) &tbl_vals, 8, 0, 8, -108);
			}
			// change clip_protect_0 to -371
			setbits((uint8*) &tbl_vals, 8, 18, 12, -371);
			// change clip_protect_1 to -307
			setbits((uint8*) &tbl_vals, 8, 30, 12, -307);
			if (chan == 1) {
				// change nlm_norm to 201
				setbits((uint8*) &tbl_vals, 8, 42, 10, 201);
			} else {
				// change nlm_norm to 204
				setbits((uint8*) &tbl_vals, 8, 42, 10, 204);
				// change cliprotect_dB_2 to -384
				phy_ac_txss_tshaper_set_table_bits(pi, 50, 32, 12, -384);
			}
			// write index 10
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, 1,
				rpt_index, 64, &tbl_vals);
		}
		/* for noop to 11ac frames (mcs10/mcs11) */
		/* comment out as this WAR is no longer used */
		//phy_ac_txss_tshaper_set_table_bits(pi, 61, 33, 4, 5);
	} else { // 5g settings
		// adjust clip protect and scaleup to simulate IBO 6 when fixed IBO 4 is used
		// mid rate is assumed to be rate group 1 and there are 8 power levels in
		// rate power table
		// index in TXSHPRCMNLUTTBL that corresponds to IBO 4 is 8 - after accounting
		// for 11n offset of 1 dB
		// index 9 for 11ac and legacy
		// read index 8
		// 8 to 10 covers IBO 4 indexing for all BWs and rates
		for (rpt_index = 8; rpt_index <= 10; rpt_index++) {
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, 1,
				rpt_index, 64, &tbl_vals);
			// change scale up to -128
			setbits((uint8*) &tbl_vals, 8, 0, 8, -128);
			// change pbgain to 0
			setbits((uint8*) &tbl_vals, 8, 8, 10, 0);
			// change clip_protect_0 to -141
			setbits((uint8*) &tbl_vals, 8, 18, 12, -141);
			// change clip_protect_1 to -230
			setbits((uint8*) &tbl_vals, 8, 30, 12, -230);
			// change nlm_norm to 246
			setbits((uint8*) &tbl_vals, 8, 42, 10, 201);
			// change cliprotect_dB_2 to -256
			phy_ac_txss_tshaper_set_table_bits(pi, 50, 32, 12, -256); /* LR */
			phy_ac_txss_tshaper_set_table_bits(pi, 54, 32, 12, -256); /* MixR */
			// write index 8
			wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, 1,
				rpt_index, 64, &tbl_vals);
		}
		/* for noop to 11ac frames (mcs10/mcs11) */
		/* comment out as this WAR is no longer used */
		//phy_ac_txss_tshaper_set_table_bits(pi, 61, 33, 4, 7);
	}
#endif /* TXS_DEBUGCODE */
}
#endif /* TXS_USE_FIXED_IBO */

static void
phy_ac_txss_txshaper_adjust_fir_cutoff(phy_info_t *pi, int16 chan)
{
	uint k;
	uint64 tbl_vals[TXS_N_RATE_GROUPS_USED+1];
	uint32 fir_cutoff;

	// please note this function is bypassed for 40mhz for now.
	// later, we may consider adding explicit setting for 40mhz as well to avoid confusion
	if (!(CHSPEC_IS2G(pi->radio_chanspec) && CHSPEC_IS20(pi->radio_chanspec))) {
		return;
	}
	if (chan <= 2) {
		fir_cutoff = TXS_FIR_CUTOFF_m11p5;
	} else if (chan >= 10 && chan <= 12) {
		fir_cutoff = TXS_FIR_CUTOFF_11p5;
	} else if (chan == 13) {
		fir_cutoff = TXS_FIR_CUTOFF_10p5;
	} else {
		// no change needed
		return;
	}
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, TXS_N_RATE_GROUPS_USED+1,
		TXS_TBL_OFFS_PRG_WORD1, 64, tbl_vals);

	for (k = 0; k < TXS_N_RATE_GROUPS_USED+1; k++) {
		setbits((uint8*) &tbl_vals[k], 8, 0, 24, fir_cutoff);
	}

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, TXS_N_RATE_GROUPS_USED+1,
		TXS_TBL_OFFS_PRG_WORD1, 64, tbl_vals);

	return;
}

static void
phy_ac_txss_txshaper_left_sided_shaping(phy_info_t *pi, bool left_shaping)
{
	uint k;
	uint64 tbl_vals[TXS_N_RATE_GROUPS_USED+1];
	uint32 uval0, uval1;

	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, TXS_N_RATE_GROUPS_USED+1,
		TXS_TBL_OFFS_PRG_WORD1, 64, tbl_vals);

	for (k = 0; k < TXS_N_RATE_GROUPS_USED+1; k++) {

		uval0 = getbits((uint8*) &tbl_vals[k], 8, 0, 12);
		uval1 = getbits((uint8*) &tbl_vals[k], 8, 12, 12);

		// init_phasor_A_down and phase_step_A should be -ve for left sided shaping
		if (((uval0 & 0x800)?1:0) ^ left_shaping) {
			uval0 = ~uval0+1;
			uval1 = ~uval1+1;
		}

		setbits((uint8*) &tbl_vals[k], 8, 0, 12, uval0);
		setbits((uint8*) &tbl_vals[k], 8, 12, 12, uval1);
	}

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, TXS_N_RATE_GROUPS_USED+1,
		TXS_TBL_OFFS_PRG_WORD1, 64, tbl_vals);
}

// set values in specified bit range in a TXSHPRCMNLUTTBL entry
static void
phy_ac_txss_tshaper_set_table_bits(phy_info_t *pi, uint8 offset, uint8 start_bit, uint8 nbits,
	uint64 val)
{
	uint64 tbl_val;

	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, 1, offset, 64u, &tbl_val);

	setbits((uint8*) &tbl_val, 8u, start_bit, nbits, val);

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, 1,
		offset, 64u, &tbl_val);
}

static const int16 txs_exp_lut_vals[] = {0x09d2, 0x09c6, 0x09b9, 0x09ab, 0x099d, 0x098d, 0x097c,
	0x096a, 0x0957, 0x0943, 0x092e, 0x0917, 0x08ff, 0x08e6, 0x08cb, 0x08af, 0x0892,
	0x0872, 0x0852, 0x0830, 0x080c, 0x07e7, 0x07c0, 0x0798, 0x076e, 0x0743, 0x0716,
	0x06e8, 0x06b8, 0x0686, 0x0653, 0x061e, 0x05e8, 0x05b1, 0x0578, 0x053f, 0x0506,
	0x04cd, 0x0495, 0x045c, 0x0423, 0x03e8, 0x03b0, 0x037b, 0x0349, 0x031a, 0x02ee,
	0x02c4, 0x029c, 0x0277, 0x0254, 0x0232, 0x0213, 0x01f5, 0x01d9, 0x01bf, 0x01a6,
	0x018e, 0x0178, 0x0163, 0x014f, 0x013c, 0x012b, 0x011a};

static uint8
phy_ac_bw_idx(phy_info_t *pi)
{
	uint8 bw_idx;
	if (CHSPEC_IS20(pi->radio_chanspec))
		bw_idx = 0;
	else if (CHSPEC_IS40(pi->radio_chanspec))
		bw_idx = 1;
	else if (CHSPEC_IS80(pi->radio_chanspec))
		bw_idx = 2;
	else /* 160MHz */
		bw_idx = 2;
	return bw_idx;
}

static void
phy_ac_txss_txshaper_set_bypass(phy_ac_txss_info_t *txssaci, uint32 val)
{
	phy_info_t *pi = txssaci->pi;
	uint8 en = txssaci->txshaperi->txshaper_en > 0 ? 1 : 0;
	WRITE_PHYREG(pi, tx_shaper_common12, val);
	MOD_PHYREG(pi, tx_shaper_common12, txshaper_11b_bypass, en);
	if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
		phy_ac_txss_txshaper_bypass_sync(txssaci);
	}
}

static void
phy_ac_txss_txshaper_set_regtbl_on_bw_change(phy_ac_txss_info_t *txssaci, uint8 bwidx, uint8 en)
{
	phy_info_t *pi = txssaci->pi;

	if (en > 0) {
		MOD_PHYREG(pi, tx_shaper_common10, config_shprendrstdly, 0x3a); // reduce phyCrs
		MOD_PHYREG(pi, tx_shaper_common2a, config_txshp_start_dly_count_BW20, 0x52);
		MOD_PHYREG(pi, tx_shaper_common2c, config_txshp_start_dly_count_BW40, 0x52);
		MOD_PHYREG(pi, tx_shaper_common2e, config_txshp_start_dly_count_BW80_BW160, 0xa9);
		MOD_PHYREG(pi, Tx11nplcpDelay, plcpdelay11n,  0x366);
		MOD_PHYREG(pi, TxvhtnplcpDelay, plcpdelayvht, 0x528);
		MOD_PHYREG(pi, TxheplcpDelay, plcpdelayhe, 0x7f0);
		MOD_PHYREG(pi, TxMacIfHoldOff, holdoffval, 0xe);
		switch (bwidx) {
			case 0:
				MOD_PHYREG(pi, tx_shaper_common11, config_shprrealfrmdly, 0x6c);
				MOD_PHYREG(pi, Tx11agplcpDelay, plcpdelay11ag, 0x280);
				break;
			case 1:
				MOD_PHYREG(pi, tx_shaper_common11, config_shprrealfrmdly, 0xa4);
				MOD_PHYREG(pi, Tx11agplcpDelay, plcpdelay11ag, 0x2f8);
				break;
			case 2:
			case 3:
				MOD_PHYREG(pi, tx_shaper_common11, config_shprrealfrmdly, 0xc5);
				MOD_PHYREG(pi, Tx11agplcpDelay, plcpdelay11ag, 0x2f8);
				break;
			default:
				PHY_ERROR(("%s: Invalid bwidx=%d\n", __FUNCTION__, bwidx));
		}
	} else {
		MOD_PHYREG(pi, tx_shaper_common10, config_shprendrstdly, 0xfa);
		MOD_PHYREG(pi, tx_shaper_common11, config_shprrealfrmdly, 0xe2);
		MOD_PHYREG(pi, tx_shaper_common2a, config_txshp_start_dly_count_BW20, 0xc8);
		MOD_PHYREG(pi, tx_shaper_common2c, config_txshp_start_dly_count_BW40, 0xc8);
		MOD_PHYREG(pi, tx_shaper_common2e, config_txshp_start_dly_count_BW80_BW160, 0xc8);
		MOD_PHYREG(pi, Tx11agplcpDelay, plcpdelay11ag, 0x280);
		MOD_PHYREG(pi, Tx11nplcpDelay, plcpdelay11n, 0x2ee);
		MOD_PHYREG(pi, TxvhtnplcpDelay, plcpdelayvht, 0x4b0);
		MOD_PHYREG(pi, TxheplcpDelay, plcpdelayhe, 0x7e0);
		MOD_PHYREG(pi, TxMacIfHoldOff, holdoffval, 0x11);
	}
	if (ACMAJORREV_GE130(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, tx_shaper_common12, txshaper_11b_bypass, en > 0 ? 1 : 0);
		if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			phy_ac_txss_txshaper_bypass_sync(txssaci);
		}
	}
}

int
phy_ac_txss_txshaper_status(phy_ac_txss_info_t *txssaci)
{
	return (int) txssaci->txshaperi->txshaper_en_ovr < 0 ?
		txssaci->txshaperi->txshaper_en : txssaci->txshaperi->txshaper_en_ovr;
}

void
phy_ac_txss_txshaper_enable(phy_ac_txss_info_t *txssaci, uint8 en)
{
	phy_info_t *pi = txssaci->pi;
	uint8 bw_idx = phy_ac_bw_idx(pi);

	txssaci->txshaperi->txshaper_en = en;
	if (en > 0) {
		MOD_PHYREG(pi, tx_shaper_commonf, config_shprmstr_enable, 1);
		if (en > 1) {
			phy_ac_txss_txshaper_set_bypass(txssaci, 0xffff); /* enable but bypassed */
		} else {
			phy_ac_txss_txshaper_set_bypass(txssaci, 0x1); /* enable and not bypassed */
		}
	} else {
		MOD_PHYREG(pi, tx_shaper_commonf, config_shprmstr_enable, 0);
		phy_ac_txss_txshaper_set_bypass(txssaci, 0x0); /* disable and not bypassed */
	}

	phy_ac_txss_txshaper_set_regtbl_on_bw_change(txssaci, bw_idx, en);

}

int
phy_ac_txss_txshaper_init(phy_ac_txss_info_t *txssaci)
{
	phy_info_t *pi = txssaci->pi;
	uint8 core;
	/* Shaper no-op for 11ag 36-54mbps, 11n/ac: mcs6-11, 11ax mcs9-11;
	 * Use WAREng to disable 11ac mcs10/11, which is overlapped with 11ax mcs0/1
	 */
	const uint64 k_ri2rg_tbl_vals[] = {0x849494922493, 0x252524849494, 0x124001};
	/* vht1024qam war */
	//const uint64 k_ri2rg_tbl_vals[] = {0x849494912493, 0x26DB24849494, 0x124001};

	/* Restrict disabling PAPR for chips earlier than 6715  */
	if (!ACMAJORREV_GE130(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, papr_ctrl, papr_blk_en, 0);
	}

	MOD_PHYREG(pi, tx_shaper_commonf, config_shprmstr_enable, 1);

	//Delay tuning is set by phy_ac_txss_txshaper_set_regtbl_on_bw_change()

	// Initialize non-per channel tables
	FOREACH_CORE(pi, core) {
		wlc_phy_table_write_acphy(pi,
			ACPHY_TBL_ID_TXSHPREXPLUTTBL(core), 64, 0, 16, txs_exp_lut_vals);
	}

	// rate index to rate group table
	wlc_phy_table_write_acphy(pi,
		ACPHY_TBL_ID_TXSHPRCMNLUTTBL, 3, TXS_TBL_OFFS_RI2RGTBL, 64, k_ri2rg_tbl_vals);
	// Disable txshaper for 11b by setting to rate group 7
	//phy_ac_txss_tshaper_set_table_bits(pi, TXS_TBL_OFFS_RI2RGTBL, 0, 2, 7);

	MOD_PHYREG(pi, tx_shaper_common1, config_expCompOffset0_value, -6242);
	MOD_PHYREG(pi, tx_shaper_common2, config_polarClipOffset0_value, -7265);
	MOD_PHYREG(pi, tx_shaper_common8, config_rate_pwr_tbl_enable, 1);
	MOD_PHYREG(pi, tx_shaper_commone, config_rategrp_ovr, 0);

	MOD_PHYREG(pi, tx_shaper_common8, config_IBO_ovr, 0);

	MOD_PHYREG(pi, tx_shaper_commona, config_firABProcessingDelay, 3);
	MOD_PHYREG(pi, tx_shaper_commona, config_firCProcessingDelay, 1);

	return 0;
}

// Write tables and registers based on channel profile
static void
phy_ac_txss_txshaper_load_chan_profile(phy_ac_txss_info_t *txssaci, int chan_profile)
{
	phy_info_t *pi = txssaci->pi;
	int k;

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_TXSHPRCMNLUTTBL, TXS_PER_CHAN_TABLE_LEN,
		0, 64, k_shaper_prg_chan_tables[chan_profile]);

	k = 0;
	MOD_PHYREG(pi, tx_shaper_common3, config_polarClipProtect_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_common4, config_polarClipProtect1_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_common5, config_nlmScaleup_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_common6, config_pbGain_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_common7, config_nlmNorm_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_common9, config_iboOffsetN_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_common9, config_iboOffsetAx_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_commonb, config_axClipOffset0_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_commonc, config_axClipOffset1_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_commond, config_axClipOffset2_value,
		shaper_chan_reg_vals[k++][chan_profile]);
	MOD_PHYREG(pi, tx_shaper_commone, config_psat_index_adj_11b,
		shaper_chan_reg_vals[k++][chan_profile]);
}

// enable/disable shaper per rate and channel by interpeting nvram txs_rate_chan_en
static void
phy_ac_txss_txshaper_interpret_rate_chan_mask(phy_ac_txss_info_t *txssaci, int16 chan,
	bool bandedge)
{
	phy_info_t *pi = txssaci->pi;
	uint8 rate_chan_index;
	int8 val;
	uint8 rate_group_en_mask;
	uint rate_group;
	uint8 bw_idx = phy_ac_bw_idx(pi);
	//bool papr_enable;

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			if (chan <= 2) {
				rate_chan_index = 0u;
			} else if (chan <= 9) {
				rate_chan_index = 1u;
			} else if (chan <= 11) {
				rate_chan_index = 2u;
			} else if (chan == 12) {
				rate_chan_index = 3u;
			} else if (chan == 13) {
				rate_chan_index = 4u;
			} else {
				rate_chan_index = 1u;
			}
		} else {
			if (chan <= 4) {
				rate_chan_index = 0u;
			} else if (chan <= 7) {
				rate_chan_index = 1u;
			} else if (chan <= 9) {
				rate_chan_index = 2u;
			} else if (chan <= 11) {
				rate_chan_index = 2u;
			} else {
				rate_chan_index = 1u;
			}
		}
	} else { //5g
		rate_chan_index = 3u*(bandedge?1:0) + bw_idx;
	}

	//printf("channel: %d bandedge: %d rate_chan_index: %d mask: %d\n", chan, bandedge,
	//	rate_chan_index, txssaci->txshaperi->p_txs_rate_chan_en[rate_chan_index]);

	rate_group_en_mask = txssaci->txshaperi->p_txs_rate_chan_en[rate_chan_index];
	for (rate_group = 0; rate_group < TXS_N_RATE_GROUPS_USED; rate_group++) {
		val = (rate_group_en_mask & (1u << rate_group)) ? 1u : 0u;

		phy_ac_txss_tshaper_set_table_bits(pi, TXS_TBL_OFFS_PRG_WORD0 + rate_group + 1,
			TXS_TBL_BIT_SHAPER_ENABLE, 1u, val);
		//printf("rate_group: %d shaper_en: %d\n", rate_group, val);
	}

	// Enable PAPR if shaper is completely off for the channel
	//if ((rate_group_en_mask == 0) && !bandedge) {
	//	papr_enable = FALSE; //phy_ac_txss_papr_enable(txssaci);
	//} else {
	//	papr_enable = FALSE;
	//}
	//MOD_PHYREG(pi, papr_ctrl, papr_blk_en, papr_enable);
	//MOD_PHYREG(pi, tx_shaper_commonf, config_shprmstr_enable, !papr_enable);
	// Update the shared memory flag to tell ucode about shaper after disable/enable for ofdm-a
	//phy_set_mhf(pi, MHF3, MHF3_TXSHAPER_EN, papr_enable ? 0: MHF3_TXSHAPER_EN, WLC_BAND_AUTO);

}

/* per channel configuration */
int
phy_ac_txss_txshaper_chan_set(phy_ac_txss_info_t *txssaci)
{
	phy_info_t *pi = txssaci->pi;
	int ret = 0;
	int chan_profile;
	uint32 fc = 0;
	int16 phy_bw_mhz;
	bool left_sided = FALSE;
	bool bandedge = FALSE;
	int16 chan;
	uint8 bw_idx = phy_ac_bw_idx(pi);
	int psat_index;
	int8 psat_margin;
	uint8 bb_mult_int_en, subband_idx;
	uint8 core;
	bool suspend = FALSE;
	uint8 stall_val;
	uint8 shaper_en;
#ifdef TXS_USE_FIXED_IBO
	uint8 txs_ibo;
#endif /* TXS_USE_FIXED_IBO */

	// update srom setting, for new band
	phy_ac_txss_set_shaper_en_srom(txssaci);

	// keep ovr if previously set
	shaper_en = txssaci->txshaperi->txshaper_en_ovr >= 0 ?
		txssaci->txshaperi->txshaper_en_ovr : txssaci->txshaperi->txshaper_en;

	// update shaper setting
	phy_ac_txss_txshaper_enable(txssaci, shaper_en);

	/* skip the rest if txshaper is off */
	if (!shaper_en) {
#ifdef TXS_DEBUG
		PHY_ERROR(("%s: txss disabled, skip initialization\n", __FUNCTION__));
#endif
		return 0;
	}

	suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	if (!suspend)
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	// Pick channel profile, and left/right sided shaping
	if (CHSPEC_IS160(pi->radio_chanspec)) {
		phy_bw_mhz = 160; /* use real phybw to calculate bandedge */
	} else {
		phy_bw_mhz = 20 << bw_idx;
	}
	chan = CHSPEC_CHANNEL(pi->radio_chanspec);
	if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
		const chan_info_radio20708_rffe_t *chan_info;
		fc = wlc_phy_chan2freq_20708(pi, pi->radio_chanspec, &chan_info);
	} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
		const chan_info_radio20710_rffe_t *chan_info;
		fc = wlc_phy_chan2freq_20710(pi, pi->radio_chanspec, &chan_info);
	} else {
		fc = 5210; // set default to 5210 MHz
	}

#ifdef TXS_DEBUG
	PHY_ERROR(("phy_ac_txss_txshaper_chan_set\n"));
	PHY_ERROR(("chan: %d fc: %d, bw: %d MHz bw_idx: %d\n", chan, fc, phy_bw_mhz, bw_idx));
#endif

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		if (bw_idx == 0) {
			left_sided = chan <= 2;
			bandedge = chan <= 2 || chan >= 10;
			/* use CH13 profile for all 2G bandedge channels */
			chan_profile = bandedge ? BE_2G20 : NONBE_2G20;
		} else {
			left_sided = chan <= 4;
			bandedge = chan <= 4 || chan >= 8;
			chan_profile = bandedge? BE_2G40: NONBE_2G40;
		}
	} else { // 5G
		uint16 left_edge, right_edge;	// left/right edge of in-band in MHz

		left_edge = fc-phy_bw_mhz/2;
		right_edge = fc+phy_bw_mhz/2;

		if (right_edge == 5330 || (right_edge == 7105 && bw_idx == 1)) {
			bandedge = TRUE;
		} else if ((left_edge == 5170) || (left_edge == 5490) || (left_edge == 5945)) {
			bandedge = TRUE;
			left_sided = TRUE;
		}

		if (CHSPEC_IS160(pi->radio_chanspec)) {
			bandedge = TRUE;
			/* for 160mhz, depending on primary, set the left_sided accordingly */
			if (CHSPEC_CTL_SB(pi->radio_chanspec) < WL_CHANSPEC_CTL_SB_ULL) {
				left_sided = TRUE;
			} else {
				left_sided = FALSE;
			}
		}

		chan_profile = bw_idx + 3*(bandedge?1:0);
	}
#ifdef TXS_DEBUG
	PHY_ERROR(("chan_profile: %d, bandedge: %d left_sided: %d\n",
		chan_profile, bandedge, left_sided));
#endif
	phy_ac_txss_txshaper_load_chan_profile(txssaci, chan_profile);

	phy_ac_txss_txshaper_left_sided_shaping(pi, left_sided);

	phy_ac_txss_txshaper_adjust_fir_cutoff(pi, chan);

	phy_ac_txss_txshaper_interpret_rate_chan_mask(txssaci, chan, bandedge);

	psat_margin = phy_ac_txss_txshaper_psat_margin(pi, bandedge);

	FOREACH_CORE(pi, core) {
		subband_idx = phy_ac_chanmgr_get_chan_freq_range_srom12(pi, pi->radio_chanspec);
		subband_idx = subband_idx >= CH_5G_4BAND ? CH_5G_4BAND - 1 : subband_idx;

		// psat index
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			psat_index = txssaci->txshaperi->txs_psat_index2g[core][0];
		} else {
			psat_index = txssaci->txshaperi->txs_psat_index5g[core][subband_idx - 1];
		}

		psat_index += psat_margin;
		if (psat_index < 0)
			psat_index = 0;
		if (psat_index > 127)
			psat_index = 127;

		//printf("chan: %d psat_true: %d psat_margin: %d (adjusted) psat_index: %d\n",
		//	chan, psat_index - psat_margin, psat_margin, psat_index);

		bb_mult_int_en = READ_PHYREGFLD(pi, TxPwrCtrlCmd, bbMultInt_en);
		if (bb_mult_int_en) {
			MOD_PHYREGCEE(pi, tx_shaper_cfg1, core, config_psat_index, 4*psat_index);
		} else {
			MOD_PHYREGCEE(pi, tx_shaper_cfg1, core, config_psat_index, 2*psat_index);
		}
		// .25 vs. .5 dB gain step
		MOD_PHYREGCEE(pi, tx_shaper_cfg3, core, config_txs_gain_indices_per_dB,
			bb_mult_int_en);
	}

#ifdef TXS_USE_FIXED_IBO
	MOD_PHYREG(pi, tx_shaper_common8, config_IBO_ovr, 1);
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		if (chan == 13) {
			txs_ibo = TXS_IBO_6dB;
		} else {
			txs_ibo = TXS_IBO_3dB;
		}
	} else { // fixed IBO for 5g
		txs_ibo = TXS_IBO_4dB;
	}

	FOREACH_CORE(pi, core) {
		MOD_PHYREGCEE(pi, tx_shaper_cfg0, core, config_iboReg_value,
			txs_ibo);
	}
	phy_ac_txss_txshaper_mid_rates_tuning(pi, chan); // Done in TCL

#endif /* TXS_USE_FIXED_IBO */

	ACPHY_ENABLE_STALL(pi, stall_val);
	if (!suspend)
		wlapi_enable_mac(pi->sh->physhim);
	return ret;
}

int
phy_ac_txss_txshaper_gain_index_set(phy_info_t *pi, uint8 core, int8 txpwrindex, int16 bbmult)
{
	phy_ac_txss_info_t *txssaci = pi->u.pi_acphy->txssi;

	if (!txssaci->txshaperi->txshaper_en)
		return 0;

	MOD_PHYREGCEE(pi, tx_shaper_cfg3, core, config_txs_gain_index, txpwrindex);
	MOD_PHYREGCEE(pi, tx_shaper_cfg4, core, config_bbmult, bbmult);

	phy_ac_txss_txshaper_power_control_enable(pi, FALSE);

	return 0;
}

int
phy_ac_txss_txshaper_power_control_enable(phy_info_t *pi, bool enable)
{
	phy_ac_txss_info_t *txssaci = pi->u.pi_acphy->txssi;

	if (!txssaci->txshaperi->txshaper_en)
		return 0;

	MOD_PHYREG(pi, tx_shaper_common8, config_txs_bbmult_ovr, !enable);

	return 0;
}

static int8
phy_ac_txss_txshaper_psat_margin(phy_info_t *pi, bool bandedge)
{
	phy_ac_txss_info_t *txssaci = pi->u.pi_acphy->txssi;
	int8 margin;
	uint8 chan, index;

	/* psat margin is added to psat index for finer tune
	 * psat index for different band/bw; however, as we use
	 * fixed ibo setting (TXS_USE_FIXED_IBO), these codes
	 * does not really matter it is changed just to match TCL
	 */

	chan = CHSPEC_CHANNEL(pi->radio_chanspec);

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		// special handling for ch13
		if (CHSPEC_IS20(pi->radio_chanspec) && chan == 13) {
			margin = 2;
		} else if (bandedge) { // banedge
			margin = 1;
		} else {	       // non-bandedge
			margin = -2;
		}
		index = (chan <= 2u) ? 0u : (chan < 10u) ? 1u : (chan - 9u);
		margin += txssaci->txshaperi->txs_psat_margin_adj[index];
	} else {	// 5g
		if (bandedge) {
			/* There was a check for "if (CHSPEC_IS20(pi->radio_chanspec))"
			 * here, just in case it's needed in the future.  However, both
			 * cases set the margin to -2, which causes Coverity issues.
			 */
			margin = -2;
		} else {	// !bandedge
			margin = 0;
		}
	}

	return margin;
}

/* called by IOVar */
int
phy_ac_txss_txshaper_force_iov(phy_ac_txss_info_t *txssaci, bool set, int8 val)
{
	if (!set) {
		/* return actual txshaper setting */
		return phy_ac_txss_txshaper_status(txssaci);
	} else {
		/* limit to [-1, 2] */
		txssaci->txshaperi->txshaper_en_ovr = val < 0 ? -1 : val > 2 ? 2 : val;
		if (val < 0) {
			// reset to srom setting
			phy_ac_txss_set_shaper_en_srom(txssaci);
		} else {
			txssaci->txshaperi->txshaper_en = txssaci->txshaperi->txshaper_en_ovr;
		}
	}
	return 0;
}

void
phy_ac_txss_txshaper_bypass_sync(phy_ac_txss_info_t *txssaci)
{
	/* This function should be called for PHY revs that have a txshaper that does not
	 * handle VHT mcs10&11 correctly and that depend on a ucode WAR.
	 */
	phy_info_t *pi = txssaci->pi;

	// Bail out if the loaded ucode does not support the WAR
	ASSERT(M_PHYREG_TX_SHAPER_COMMON12_VAL(pi) != 0xffff);

	wlapi_bmac_write_shm(pi->sh->physhim, M_PHYREG_TX_SHAPER_COMMON12_VAL(pi),
		READ_PHYREG(pi, tx_shaper_common12));
}

#endif /* WLC_TXSHAPER */
