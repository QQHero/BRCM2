/*
 * ACPHY TOF module implementation
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
 * $Id: phy_ac_tof.c 639713 2016-05-24 18:02:57Z $
 */
#ifdef WL_PROXDETECT

#include <hndpmu.h>
#include <sbchipc.h>
#include <phy_mem.h>
#include <bcm_math.h>
#include <wlc_phyreg_ac.h>
#include <phy_utils_reg.h>
#include <phy_utils_var.h>
#include <phy_rstr.h>
#include <phy_cache.h>
#include "phy_type_tof.h"
#include <phy_ac.h>
#include <phy_rxgcrs_api.h>
#include <phy_tof_api.h>
#include <phy_ac_info.h>
#include <phy_ac_tof.h>
#include <phy_stf.h>
#include <phy_samp.h>

/* uncomment the below define when doing calibration */
/* #define CALIBRATION */

/* 43684 k values */
#define TOF_INITIATOR_K_43684_160M     34564 /* initiator K value for 160M */
#define TOF_TARGET_K_43684_160M        34564 /* target K value for 160M */

#define TOF_INITIATOR_K_43684_80M      33889 /* initiator K value for 80M */
#define TOF_TARGET_K_43684_80M         33889 /* target K value for 80M */

#define TOF_INITIATOR_K_43684_40M      34997 /* initiator K value for 40M */
#define TOF_TARGET_K_43684_40M         34997 /* target K value for 40M */

#define TOF_INITIATOR_K_43684_20M      35952 /* initiator K value for 20M */
#define TOF_TARGET_K_43684_20M         35952 /* target K value for 20M */

#define TOF_INITIATOR_K_43684_2G       36055 /* initiator K value for 2G */
#define TOF_TARGET_K_43684_2G          36055 /* target K value for 2G */

static const  uint32 BCMATTACHDATA(proxd_43684_160m_k_values)[] =
/* 50, 114 */
{0x00100010, 0x00000000};

#ifdef WL_PROXD_PHYTS
/* 43684 */
static const uint32 BCMATTACHDATA(proxd_43684_phyts_5g80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x1abf, 0x12e6, 0x1d08, 0x1cc0, 0x1d20, 0x1c6b};

static const uint32 BCMATTACHDATA(proxd_43684_phyts_5g40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x4a43, 0x4c17, 0x44e9, 0x3ae1, 0x3f2f, 0x43be,
/* 118, 126, 134, 142, 151, 159 */
0x4513, 0x444c, 0x46cc, 0x4522, 0x44ff, 0x4755};

static const uint32 BCMATTACHDATA(proxd_43684_phyts_5g20m_k_values)[] =
/* 36 - 64 */
{0x6ba7, 0x665f, 0x686e, 0x5e19, 0x5783, 0x5308, 0x67c0, 0x66a0,
/* 100 - 144 */
0x67df, 0x6c21, 0x6aaf, 0x6a30, 0x6ae9, 0x6a3f,
0x6917, 0x6a1a, 0x688b, 0x69a8, 0x6c89, 0x6b05,
/* 149 - 165 */
0x662d, 0x69ad, 0x67e8, 0x62c8, 0x675c};

static const uint32 BCMATTACHDATA(proxd_43684_phyts_2g20m_k_values)[] =
/* 1 - 7 */
{0, 0, 0, 0, 0, 0, 0,
/* 8 - 14 */
0, 0, 0, 0, 0, 0, 0};

/* 47622 */
static const uint32 BCMATTACHDATA(proxd_47622_phyts_5g80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x1f05, 0x16fe, 0x1713, 0x16e7, 0x1f93, 0xe5e};

static const uint32 BCMATTACHDATA(proxd_47622_phyts_5g40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x4a0e, 0x47c8, 0x4ba0, 0x3db3, 0x52de, 0x48bc,
/* 118, 126, 134, 142, 151, 159 */
0x3322, 0x4613, 0x4ad2, 0x52ed, 0x6fa0, 0x41e8};

static const uint32 BCMATTACHDATA(proxd_47622_phyts_5g20m_k_values)[] =
/* 36 - 64 */
{0x6d82, 0x6a69, 0x6f48, 0x71e7, 0x713f, 0x6a2c, 0x5c06, 0x6e75,
/* 100 - 144 */
0x6e0b, 0x63a7, 0x67a4, 0x6298, 0x4c28, 0x661c,
0x68ad, 0x67fc, 0x6fec, 0x70e9, 0x72e0, 0x7062,
/* 149 - 165 */
0x6280, 0x2a81, 0x6e8b, 0x73dc, 0x6f0d};

static const uint32 BCMATTACHDATA(proxd_47622_phyts_2g20m_k_values)[] =
/* 1 - 7 */
{0x7776, 0x7875, 0x77a9, 0x796c, 0x75fe, 0x7725, 0x6fea,
/* 8 - 14 */
0x6f01, 0x788b, 0x7625, 0x7b11, 0x7552, 0x7080, 0};

/* 47623 */
static const uint32 BCMATTACHDATA(proxd_47623_phyts_5g80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x1222, 0x219e, 0x20dc, 0x1580, 0, 0x1c30};

static const uint32 BCMATTACHDATA(proxd_47623_phyts_5g40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x4444, 0, 0x4eca, 0x512d, 0x5976, 0x5c6d,
/* 118, 126, 134, 142, 151, 159 */
0x4921, 0x4a07, 0x52ec, 0x49de, 0x4a7f, 0x5933};

static const uint32 BCMATTACHDATA(proxd_47623_phyts_5g20m_k_values)[] =
/* 36 - 64 */
{0x5f99, 0x55cd, 0, 0, 0x6a52, 0x72e0, 0x6a02, 0x742e,
/* 100 - 144 */
0x6ec4, 0x6fc5, 0x6d22, 0x6c1b, 0x6fd6, 0x7565,
0x70a2, 0x75eb, 0x6535, 0x5e71, 0x6484, 0x65aa,
/* 149 - 165 */
0x7a69, 0x7c83, 0x744f, 0x6ed8, 0x6e9c};

static const uint32 BCMATTACHDATA(proxd_47623_phyts_2g20m_k_values)[] =
/* 1 - 7 */
{0x5536, 0x57cd, 0x646f, 0x6650, 0x6c05, 0x7740, 0x79c0,
/* 8 - 14 */
0x7fac, 0x7ef8, 0x7e50, 0x7e96, 0x7e80, 0x7dea, 0};

/* 43794 */
static const uint32 BCMATTACHDATA(proxd_43794_phyts_5g80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x20f8, 0x2c37, 0x260d, 0x218d, 0x2309, 0x2d37};

static const uint32 BCMATTACHDATA(proxd_43794_phyts_5g40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x4d75, 0x53aa, 0x2ecb, 0x2def, 0x4727, 0x5177,
/* 118, 126, 134, 142, 151, 159 */
0x4dec, 0x4fd1, 0x5544, 0x50f0, 0x55fb, 0x5bc4};

static const uint32 BCMATTACHDATA(proxd_43794_phyts_5g20m_k_values)[] =
/* 36 - 64 */
{0x60b9, 0x6085, 0x5d04, 0x3094, 0x455d, 0, 0x6d10, 0x6e33,
/* 100 - 144 */
0x5dff, 0x5aef, 0x5587, 0x564e, 0x53be, 0x5469,
0x599d, 0x5e90, 0x5fd6, 0x5a48, 0x5a35, 0,
/* 149 - 165 */
0x5a27, 0x5987, 0x5b56, 0x6482, 0x7035};

static const uint32 BCMATTACHDATA(proxd_43794_phyts_2g20m_k_values)[] =
/* 1 - 7 */
{0, 0, 0, 0, 0, 0, 0,
/* 8 - 14 */
0, 0, 0, 0, 0, 0, 0};
#endif /* WL_PROXD_PHYTS */

static const  uint32 BCMATTACHDATA(proxd_43684_80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x000e000e, 0x00100010, 0x000d000d, 0x000b000b, 0x000a000a, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_43684_40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x00000000, 0x00000000, 0x00170017, 0x00260026, 0x000f000f, 0x369e369e,
/* 118, 126, 134, 142, 151, 159 */
0x36b236b2, 0x36ab36ab, 0x000d000d, 0x36823682, 0x38423842, 0x00080008};

static const  uint32 BCMATTACHDATA(proxd_43684_20m_k_values)[] =
/* 36 - 64 */
{0x00430043, 0x003c003c, 0x00400040, 0x00380038, 0x00360036, 0x003d003d, 0x003e003e, 0x00340034,
/* 100 - 144 */
0x00250025, 0x001c001c, 0x00210021, 0x001a001a, 0x001c001c, 0x00200020,
0x00210021, 0x00250025, 0x001b001b, 0x00190019, 0x001a001a, 0x00170017,
/* 149 - 165 */
0x00080008, 0x000f000f, 0x00040004, 0x00000000, 0x0c6d0c6d};

static const  uint32 BCMATTACHDATA(proxd_43684_2g_k_values)[] =
/* 1 - 7 */
{0x00190019, 0x00190019, 0x00160016, 0x000b000b, 0x000a000a, 0x000d000d, 0x000d000d,
/* 8 - 14 */
0x000e000e, 0x000b000b, 0x000a000a, 0x000b000b, 0x00070007, 0x00030003, 0x00000000};

/* ratespec related k offset table for initiator <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdi_rate_offset_2g_43684)[] = { -2119, 7852, 3497, 3497 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_20m_43684)[] = { 1317, 7867, 38, -6509 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_40m_43684)[] = { 1602, 7668, 160, -5898 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_80m_43684)[] = { 1623, 7429, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_160m_43684)[] = { 1590, 69128, 0, 0 };

/* ratespec related k offset table for target <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdt_rate_offset_2g_43684)[] = { -2119, 7852, 3497, 3497 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_20m_43684)[] = { 1317, 7867, 38, -6509 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_40m_43684)[] = { 1602, 7668, 160, -5898 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_80m_43684)[] = { 1623, 7429, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_160m_43684)[] = { 1590, 69128, 0, 0 };

/* different bandwidth  k offset table <VHT legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const int32 BCMATTACHDATA(proxd_subbw_offset_43684)[3][5] = {
	/* 80M-40M */
	{ -427, 66582, 60776, 0, 0 },
	/* 80M -20M */
	{ -992, -1975, -2037, 0, 0 },
	/* 40M - 20M */
	{ -380, -37050, -383, 36073, -648 }
};

static const int32 BCMATTACHDATA(proxd_subbw_offset_160m_43684)[3][5] = {
	/* 160M-80M */
	{ 314, 342, -60860, 0, 0 },
	/* 160M -40M */
	{ -12443, 12026, 12443, 0, 0 },
	/* 160M - 20M */
	{ -53333, 51455, -9688, 0, 0 }
};
static const int16 BCMATTACHDATA(proxdt_ack_offset_43684)[] = { 0, 0, 0, 0 };
/* end 43684 k values */

/* 47622 k values */
#define TOF_INITIATOR_K_47622_80M      34333 /* initiator K value for 80M */
#define TOF_TARGET_K_47622_80M         34333 /* target K value for 80M */

#define TOF_INITIATOR_K_47622_40M      35315 /* initiator K value for 40M */
#define TOF_TARGET_K_47622_40M         35315 /* target K value for 40M */

#define TOF_INITIATOR_K_47622_20M      36287 /* initiator K value for 20M */
#define TOF_TARGET_K_47622_20M         36287 /* target K value for 20M */

#define TOF_INITIATOR_K_47622_2G       36412 /* initiator K value for 2G */
#define TOF_TARGET_K_47622_2G          36412 /* target K value for 2G */

static const  uint32 BCMATTACHDATA(proxd_47622_80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x000c000c, 0x000c000c, 0x00030003, 0x00000000, 0x00000000, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_47622_40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x000c000c, 0x001b001b, 0x00340034, 0x00240024, 0x002c002c, 0x002a002a,
/* 118, 126, 134, 142, 151, 159 */
0x00210021, 0x001f001f, 0x00050005, 0x001a001a, 0x00000000, 0x00240024};

static const  uint32 BCMATTACHDATA(proxd_47622_20m_k_values)[] =
/* 36 - 64 */
{0x00360036, 0x00340034, 0x00340034, 0x00330033, 0x002e002e, 0x002b002b, 0x002d002d, 0x002a002a,
/* 100 - 144 */
0x00490049, 0x001e001e, 0x001c001c, 0x001b001b, 0x00130013, 0x00110011, 0x00120012, 0x00120012,
0x00100010, 0x000a000a, 0x00100010, 0x00000000,
/* 149 - 165 */
0x00090009, 0x00120012, 0x00140014, 0x00120012, 0x000b000b};

static const  uint32 BCMATTACHDATA(proxd_47622_2g_k_values)[] =
{0x001d001d, 0x001a001a, 0x00170017, 0x001a001a, 0x00180018, 0x00130013, 0x00150015, /* 1 -7 */
0x00110011, 0x00100010, 0x00130013, 0x00140014, 0x00000000, 0x00030003, 0x00080008 /* 8 - 14 */
};

/* ratespec related k offset table for initiator <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdi_rate_offset_2g_47622)[] = { 1434, 7878, 46, 46 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_20m_47622)[] = { 1419, 7844, 38, -6382 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_40m_47622)[] = { 1747, 7727, 168, -5835 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_80m_47622)[] = { 1777, 7537, 0, 0 };

/* ratespec related k offset table for target <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdt_rate_offset_2g_47622)[] = { 1434, 7878, 46, 46 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_20m_47622)[] = { 1419, 7844, 38, -6382 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_40m_47622)[] = { 1747, 7727, 168, -5835 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_80m_47622)[] = { 1777, 7537, 0, 0 };

/* different bandwidth  k offset table <VHT legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const int32 BCMATTACHDATA(proxd_subbw_offset_47622)[3][5] = {
	/* 80M-40M */
	{ -404, -756, 61533, 0, 0},
	/* 80M -20M */
	{ -1196, -1742, -1781, 0, 0 },
	/* 40M - 20M */
	{ -383, -1202, -382, -99, -534 }
};

static const int16 BCMATTACHDATA(proxdt_ack_offset_47622)[] = { 0, 0, 0, 0 };
/* end 47622 k values */

/* 47623 k values */
#define TOF_INITIATOR_K_47623_160M     34181 /* initiator K value for 160M */
#define TOF_TARGET_K_47623_160M        34181 /* target K value for 160M */

#define TOF_INITIATOR_K_47623_80M      34334 /* initiator K value for 80M */
#define TOF_TARGET_K_47623_80M         34334 /* target K value for 80M */

#define TOF_INITIATOR_K_47623_40M      35323 /* initiator K value for 40M */
#define TOF_TARGET_K_47623_40M         35323 /* target K value for 40M */

#define TOF_INITIATOR_K_47623_20M      36289 /* initiator K value for 20M */
#define TOF_TARGET_K_47623_20M         36289 /* target K value for 20M */

#define TOF_INITIATOR_K_47623_2G       36426 /* initiator K value for 2G */
#define TOF_TARGET_K_47623_2G          36426 /* target K value for 2G */

static const  uint32 BCMATTACHDATA(proxd_47623_160m_k_values)[] =
/* 50, 114 */
{0x00010001, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_47623_80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x00100010, 0x000e000e, 0x00050005, 0x00040004, 0x00010001, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_47623_40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x00330033, 0x00310031, 0x002f002f, 0x003c003c, 0x00290029, 0x002b002b,
/* 118, 126, 134, 142, 151, 159 */
0x00000000, 0x000c000c, 0x00260026, 0x00090009, 0x00120012, 0x000a000a};

static const  uint32 BCMATTACHDATA(proxd_47623_20m_k_values)[] =
/* 36 - 64 */
{0x00390039, 0x00380038, 0x003a003a, 0x00350035, 0x00320032, 0x00350035, 0x00380038, 0x003c003c,
/* 100 - 144 */
0x00250025, 0x001e001e, 0x00200020, 0x001f001f, 0x00170017, 0x00210021, 0x00190019, 0x00130013,
0x00120012, 0x00000000, 0x00100010, 0x000b000b,
/* 149 - 165 */
0x00080008, 0x00140014, 0x00100010, 0x00100010, 0x00110011};

static const  uint32 BCMATTACHDATA(proxd_47623_2g_k_values)[] =
{0x004c004c, 0x004b004b, 0x00170017, 0x00160016, 0x00140014, 0x00130013, 0x00440044, /* 1 -7 */
0x00440044, 0x00420042, 0x00400040, 0x000c000c, 0x000b000b, 0x00080008, 0x00000000 /* 8 - 14 */
};

/* ratespec related k offset table for initiator <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdi_rate_offset_2g_47623)[] = { 1420, 7859, 42, 42 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_20m_47623)[] = { 1437, 7877, 45, -6393 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_40m_47623)[] = { 1951, 7904, 161, -5791 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_80m_47623)[] = { 1781, 7541, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_160m_47623)[] = { 1776, 7574, 0, 0 };

/* ratespec related k offset table for target <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdt_rate_offset_2g_47623)[] = { 1420, 7859, 42, 42 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_20m_47623)[] = { 1437, 7877, 45, -6393 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_40m_47623)[] = { 1951, 7904, 161, -5791 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_80m_47623)[] = { 1781, 7541, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_160m_47623)[] = { 1776, 7574, 0, 0 };

/* different bandwidth  k offset table <VHT legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const int32 BCMATTACHDATA(proxd_subbw_offset_47623)[3][5] = {
	/* 80M-40M */
	{ -294, -960, -943, 0, 0},
	/* 80M -20M */
	{ -715, -1297, -1318, 0, 0 },
	/* 40M - 20M */
	{ -350, -665, -561, -462, -571 }
};

static const int32 BCMATTACHDATA(proxd_subbw_offset_160m_47623)[3][5] = {
	/* 160M-80M */
	{ -55, -46, -37, 0, 0 },
	/* 160M -40M */
	{ -721, 67307, 61509, 0, 0 },
	/* 160M - 20M */
	{ -1019, -1439, -1744, 0, 0 }
};

static const int16 BCMATTACHDATA(proxdt_ack_offset_47623)[] = { 0, 0, 0, 0 };
/* end 47623 k values */

/* 43693 k values */
#define TOF_INITIATOR_K_43693_160M     0 /* initiator K value for 160M */
#define TOF_TARGET_K_43693_160M        0 /* target K value for 160M */

#define TOF_INITIATOR_K_43693_80M      0 /* initiator K value for 80M */
#define TOF_TARGET_K_43693_80M         0 /* target K value for 80M */

#define TOF_INITIATOR_K_43693_40M      0 /* initiator K value for 40M */
#define TOF_TARGET_K_43693_40M         0 /* target K value for 40M */

#define TOF_INITIATOR_K_43693_20M      0 /* initiator K value for 20M */
#define TOF_TARGET_K_43693_20M         0 /* target K value for 20M */

#define TOF_INITIATOR_K_43693_2G       0 /* initiator K value for 2G */
#define TOF_TARGET_K_43693_2G          0 /* target K value for 2G */

static const  uint32 BCMATTACHDATA(proxd_43693_160m_k_values)[] =
/* 50, 114 */
{0x00000000, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_43693_80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_43693_40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
/* 118, 126, 134, 142, 151, 159 */
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_43693_20m_k_values)[] =
/* 36 - 64 */
{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
/* 100 - 144 */
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
/* 149 - 165 */
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_43693_2g_k_values)[] =
{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, /* 1 -7 */
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 /* 8 - 14 */
};

/* ratespec related k offset table for initiator <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdi_rate_offset_2g_43693)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_20m_43693)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_40m_43693)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_80m_43693)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_160m_43693)[] = { 0, 0, 0, 0 };

/* ratespec related k offset table for target <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdt_rate_offset_2g_43693)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_20m_43693)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_40m_43693)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_80m_43693)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_160m_43693)[] = { 0, 0, 0, 0 };

/* different bandwidth  k offset table <VHT legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const int32 BCMATTACHDATA(proxd_subbw_offset_43693)[3][5] = {
	/* 80M-40M */
	{ 0, 0, 0, 0, 0},
	/* 80M -20M */
	{ 0, 0, 0, 0, 0 },
	/* 40M - 20M */
	{ 0, 0, 0, 0, 0 }
};

static const int32 BCMATTACHDATA(proxd_subbw_offset_160m_43693)[3][5] = {
	/* 160M-80M */
	{ 0, 0, 0, 0, 0 },
	/* 160M -40M */
	{ 0, 0, 0, 0, 0 },
	/* 160M - 20M */
	{ 0, 0, 0, 0, 0 }
};

static const int16 BCMATTACHDATA(proxdt_ack_offset_43693)[] = { 0, 0, 0, 0 };
/* end 43693 k values */

/* 43794 k values */
#define TOF_INITIATOR_K_43794_160M     33975 /* initiator K value for 160M */
#define TOF_TARGET_K_43794_160M        33975 /* target K value for 160M */

#define TOF_INITIATOR_K_43794_80M      34110 /* initiator K value for 80M */
#define TOF_TARGET_K_43794_80M         34110 /* target K value for 80M */

#define TOF_INITIATOR_K_43794_40M      33736 /* initiator K value for 40M */
#define TOF_TARGET_K_43794_40M         33736 /* target K value for 40M */

#define TOF_INITIATOR_K_43794_20M      36079 /* initiator K value for 20M */
#define TOF_TARGET_K_43794_20M         36079 /* target K value for 20M */

#define TOF_INITIATOR_K_43794_2G       36159 /* initiator K value for 2G */
#define TOF_TARGET_K_43794_2G          36159 /* target K value for 2G */

static const  uint32 BCMATTACHDATA(proxd_43794_160m_k_values)[] =
/* 50, 114 */
{0x002d002d, 0x00000000};

static const  uint32 BCMATTACHDATA(proxd_43794_80m_k_values)[] =
/* 42, 58, 106, 122, 138, 155 */
{0x00160016, 0x00120012, 0x000b000b, 0x00000000, 0x00030003, 0x00040004};

static const  uint32 BCMATTACHDATA(proxd_43794_40m_k_values)[] =
/* 38, 46, 54, 62, 102, 110 */
{0x00200020, 0x00030003, 0x00180018, 0x00260026, 0x00170017, 0x00020002,
/* 118, 126, 134, 142, 151, 159 */
0x00190019, 0x00000000, 0x000c000c, 0x00060006, 0x00010001, 0x00060006};

static const  uint32 BCMATTACHDATA(proxd_43794_20m_k_values)[] =
/* 36 - 64 */
{0x00420042, 0x00480048, 0x00480048, 0x00500050, 0x00520052, 0x003d003d, 0x00430043, 0x00380038,
/* 100 - 144 */
0x00230023, 0x00230023, 0x00240024, 0x00210021, 0x002a002a, 0x00180018, 0x00160016, 0x00200020,
0x00190019, 0x00180018, 0x00180018, 0x00110011,
/* 149 - 165 */
0x000d000d, 0x000d000d, 0x00000000, 0x00000000, 0x00080008};

static const  uint32 BCMATTACHDATA(proxd_43794_2g_k_values)[] =
{0x00140014, 0x000c000c, 0x00040004, 0x00040004, 0x00030003, 0x00060006, 0x00020002, /* 1 -7 */
0x00020002, 0x00000000, 0x000f000f, 0x000b000b, 0x000f000f, 0x00140014, 0x00090009 /* 8 - 14 */
};

/* ratespec related k offset table for initiator <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdi_rate_offset_2g_43794)[] = { 1221, 7729, 14, 14 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_20m_43794)[] = { 1230, 7718, 17, -6469 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_40m_43794)[] = { 715, 6365, 66757, -5604 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_80m_43794)[] = { 1523, 7336, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_160m_43794)[] = { 1475, 7335, 0, 0 };

/* ratespec related k offset table for target <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdt_rate_offset_2g_43794)[] = { 1221, 7729, 14, 14 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_20m_43794)[] = { 1230, 7718, 17, -6469 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_40m_43794)[] = { 715, 6365, 66757, -5604 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_80m_43794)[] = { 1523, 7336, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_160m_43794)[] = { 1475, 7335, 0, 0 };

/* different bandwidth  k offset table <VHT legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const int32 BCMATTACHDATA(proxd_subbw_offset_43794)[3][5] = {
	/* 80M-40M */
	{ 384, 66313, 60500, 0, 0},
	/* 80M -20M */
	{ -761, -1313, -1326, 0, 0 },
	/* 40M - 20M */
	{ -1144, -1075, -1086, -67883, -1162 }
};

static const int32 BCMATTACHDATA(proxd_subbw_offset_160m_43794)[3][5] = {
	/* 160M-80M */
	{ -96, -48, -40, 0, 0 },
	/* 160M -40M */
	{ 270, -277, 60345, 0, 0 },
	/* 160M - 20M */
	{ -864, -1327, -1339, 0, 0 }
};

static const int16 BCMATTACHDATA(proxdt_ack_offset_43794)[] = { 0, 0, 0, 0 };
/* end 43794 k values */

/* 436846g k values */
#define TOF_6G_K_43684_160M     0 /* 6G K value for 160M */
#define TOF_6G_K_43684_80M      0 /* 6G K value for 80M */
#define TOF_6G_K_43684_40M      0 /* 6G K value for 40M */
#define TOF_6G_K_43684_20M      0 /* 6G K value for 20M */

static const uint8 BCMATTACHDATA(proxd_43684_6g160m_k_values)[] =
/* 15, 47, 79, 111, 143, 175, 207 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43684_6g80m_k_values)[] =
/* 7, 23, 39, 55, 71, 87, 103 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 119, 135, 151, 167, 183, 199, 215 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43684_6g40m_k_values)[] =
/* 3, 11, 19, 27, 35, 43, 51, 59 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 67, 75, 83, 91, 99, 107, 115, 123 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 131, 139, 147, 155, 163, 171, 179, 187 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 195, 203, 211, 219, 227 */
0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43684_6g20m_k_values)[] =
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00};
/* end 436846g k values */

/* 43693 k values */
#define TOF_6G_K_43693_160M     0 /* 6G K value for 160M */
#define TOF_6G_K_43693_80M      0 /* 6G K value for 80M */
#define TOF_6G_K_43693_40M      0 /* 6G K value for 40M */
#define TOF_6G_K_43693_20M      0 /* 6G K value for 20M */

static const uint8 BCMATTACHDATA(proxd_43693_6g160m_k_values)[] =
/* 15, 47, 79, 111, 143, 175, 207 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43693_6g80m_k_values)[] =
/* 7, 23, 39, 55, 71, 87, 103 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 119, 135, 151, 167, 183, 199, 215 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43693_6g40m_k_values)[] =
/* 3, 11, 19, 27, 35, 43, 51, 59 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 67, 75, 83, 91, 99, 107, 115, 123 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 131, 139, 147, 155, 163, 171, 179, 187 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 195, 203, 211, 219, 227 */
0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43693_6g20m_k_values)[] =
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00};
/* end 43693 k values */

/* 43794 k values */
#define TOF_6G_K_43794_160M     0 /* 6G K value for 160M */
#define TOF_6G_K_43794_80M      0 /* 6G K value for 80M */
#define TOF_6G_K_43794_40M      0 /* 6G K value for 40M */
#define TOF_6G_K_43794_20M      0 /* 6G K value for 20M */

static const uint8 BCMATTACHDATA(proxd_43794_6g160m_k_values)[] =
/* 15, 47, 79, 111, 143, 175, 207 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43794_6g80m_k_values)[] =
/* 7, 23, 39, 55, 71, 87, 103 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 119, 135, 151, 167, 183, 199, 215 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43794_6g40m_k_values)[] =
/* 3, 11, 19, 27, 35, 43, 51, 59 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 67, 75, 83, 91, 99, 107, 115, 123 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 131, 139, 147, 155, 163, 171, 179, 187 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 195, 203, 211, 219, 227 */
0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_43794_6g20m_k_values)[] =
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00};
/* end 43794 k values */

/* 47623 k values */
#define TOF_6G_K_47623_160M     0 /* 6G K value for 160M */
#define TOF_6G_K_47623_80M      0 /* 6G K value for 80M */
#define TOF_6G_K_47623_40M      0 /* 6G K value for 40M */
#define TOF_6G_K_47623_20M      0 /* 6G K value for 20M */

static const uint8 BCMATTACHDATA(proxd_47623_6g160m_k_values)[] =
/* 15, 47, 79, 111, 143, 175, 207 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_47623_6g80m_k_values)[] =
/* 7, 23, 39, 55, 71, 87, 103 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 119, 135, 151, 167, 183, 199, 215 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_47623_6g40m_k_values)[] =
/* 3, 11, 19, 27, 35, 43, 51, 59 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 67, 75, 83, 91, 99, 107, 115, 123 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 131, 139, 147, 155, 163, 171, 179, 187 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 195, 203, 211, 219, 227 */
0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_47623_6g20m_k_values)[] =
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00};
/* end 47623 k values */

/* 47622 6g k values */
#define TOF_6G_K_47622_160M     0 /* 6G K value for 160M */
#define TOF_6G_K_47622_80M      0 /* 6G K value for 80M */
#define TOF_6G_K_47622_40M      0 /* 6G K value for 40M */
#define TOF_6G_K_47622_20M      0 /* 6G K value for 20M */

static const uint8 BCMATTACHDATA(proxd_47622_6g160m_k_values)[] =
/* 15, 47, 79, 111, 143, 175, 207 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_47622_6g80m_k_values)[] =
/* 7, 23, 39, 55, 71, 87, 103 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 119, 135, 151, 167, 183, 199, 215 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_47622_6g40m_k_values)[] =
/* 3, 11, 19, 27, 35, 43, 51, 59 */
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 67, 75, 83, 91, 99, 107, 115, 123 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 131, 139, 147, 155, 163, 171, 179, 187 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 195, 203, 211, 219, 227 */
0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8 BCMATTACHDATA(proxd_47622_6g20m_k_values)[] =
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00};
/* end 47622 6g k values */

/* ratespec related k offset table for initiator <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdi_rate_offset_2g)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_20m)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_40m)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_80m)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdi_rate_offset_160m)[] = { 0, 0, 0, 0 };

/* ratespec related k offset table for target <legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const  int32 BCMATTACHDATA(proxdt_rate_offset_2g)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_20m)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_40m)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_80m)[] = { 0, 0, 0, 0 };
static const  int32 BCMATTACHDATA(proxdt_rate_offset_160m)[] = { 0, 0, 0, 0 };

/* legacy ack offset table for initiator  <80M, 40M, 20M, 2g> */
static const int16 BCMATTACHDATA(proxdi_ack_offset)[] = { 0, 0, 0, 0 };

/* legacy ack offset table for target  <80M, 40M, 20M, 2g> */
static const int16 BCMATTACHDATA(proxdt_ack_offset)[] = { 0, 0, 0, 0 };

/* different bandwidth  k offset table <VHT legacy 6M, legacy non-6M, HT-MCS0, HT-MCS(1-7)> */
static const int32 BCMATTACHDATA(proxd_subbw_offset)[3][5] = {
	/* 80M-40M */
	{ 0, 0, 0, 0, 0 },
	/* 80M -20M */
	{ 0, 0, 0, 0, 0 },
	/* 40M - 20M */
	{ 0, 0, 0, 0, 0 }
};

#ifdef WL_PROXD_SEQ

static const uint16 k_tof_seq_tiny_tbls[] = {
	ACPHY_TBL_ID_RFSEQ,
	0x260,
	15,
	0x42, 0x10, 0x8b, 0x9b, 0xaa, 0xb9, 0x8c, 0x9c, 0x9d,
	0xab, 0x8d, 0x9e, 0x43, 0x42, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x290,
	14,
	0x42, 0x10, 0x88, 0x98, 0xa8, 0xb8, 0x89, 0x99, 0xa9,
	0x8a, 0x9a, 0x43, 0x42, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x2f0,
	15,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04, 0x04,
	0x04, 0x01, 0x04, 0x1e, 0x01, 0x01,
	ACPHY_TBL_ID_RFSEQ,
	0x320,
	14,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04, 0x04,
	0x01, 0x04, 0x1e, 0x01, 0x01,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x008,
	18,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x018,
	21,
	0x0009, 0x8000, 0x0007, 0xf009, 0x8066, 0x0007, 0xf009, 0x8066, 0x0004,
	0x00c9, 0x8060, 0x0007, 0xf7d9, 0x8066, 0x0007, 0xf7f9, 0x8066, 0x0007,
	0xf739, 0x8066, 0x0004,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x028,
	12,
	0x0009, 0x0000, 0x0000, 0x0229, 0x0000, 0x0000, 0x0019, 0x0000, 0x0000,
	0x0099, 0x0000, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x038,
	6,
	0x0fc9, 0x00a6, 0x0000, 0x0fc9, 0x00a6, 0x0000,
};

/* RF Seq Bundle Table with RF Controls for TX ON and TX OFF in 2G mode */
static const uint16 k_tof_seq_tiny_tbls_2G[] = {
	ACPHY_TBL_ID_RFSEQ,
	0x260,
	15,
	0x42, 0x10, 0x8b, 0x9b, 0xaa, 0xb9, 0x8c, 0x9c, 0x9d,
	0xab, 0x8d, 0x9e, 0x43, 0x42, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x290,
	14,
	0x42, 0x10, 0x88, 0x98, 0xa8, 0xb8, 0x89, 0x99, 0xa9,
	0x8a, 0x9a, 0x43, 0x42, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x2f0,
	15,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04, 0x04,
	0x04, 0x01, 0x04, 0x1e, 0x01, 0x01,
	ACPHY_TBL_ID_RFSEQ,
	0x320,
	14,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04, 0x04,
	0x01, 0x04, 0x1e, 0x01, 0x01,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x008,
	18,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x018,
	21,
	0x0009, 0x8000, 0x0007, 0xf009, 0x801d, 0x0007, 0xf009, 0x801d, 0x0004,
	0x00c9, 0x8018, 0x0007, 0xf7d9, 0x801d, 0x0007, 0xf7f9, 0x801d, 0x0007,
	0xf739, 0x801d, 0x0004,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x028,
	12,
	0x0009, 0x0000, 0x0000, 0x0129, 0x0000, 0x0000, 0x0019, 0x0000, 0x0000,
	0x0059, 0x0000, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x038,
	6,
	0x0fc9, 0x00a6, 0x0000, 0x0fc9, 0x00a6, 0x0000,
};

static const uint16 k_tof_seq_tbls[] = {
	ACPHY_TBL_ID_RFSEQ,
	0x260,
	14,
	0x10, 0x8b, 0x9b, 0xaa, 0xb9, 0xc9, 0xd9, 0x8c, 0x9c,
	0x9d, 0xab, 0x8d, 0x9e, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x290,
	13,
	0x10, 0x88, 0x98, 0xa8, 0xb8, 0xc8, 0xd8, 0x89, 0x99,
	0xa9, 0x8a, 0x9a, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x2f0,
	14,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04,
	0x04, 0x04, 0x01, 0x04, 0x01,
	ACPHY_TBL_ID_RFSEQ,
	0x320,
	13,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04,
	0x04, 0x01, 0x04, 0x01,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x008,
	18,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x018,
	21,
	0x0009, 0x8000, 0x000f, 0xff09, 0x8066, 0x000f, 0xff09, 0x8066, 0x000c,
	0x00c9, 0x8060, 0x000f, 0xffd9, 0x8066, 0x000f, 0xfff9, 0x8066, 0x000f,
	0xff39, 0x8066, 0x000c,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x028,
	12,
	0x0009, 0x0000, 0x0000, 0x0229, 0x0000, 0x0000, 0x0019, 0x0000, 0x0000,
	0x0099, 0x0000, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x038,
	6,
	0x0fc9, 0x00a6, 0x0000, 0x0fc9, 0x00a6, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x048,
	6,
	0x2099, 0x586e, 0x0000, 0x1519, 0x586e, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x058,
	6,
	0x2409, 0xc040, 0x000c, 0x6b89, 0xc040, 0x000c,
};

const uint16 k_tof_seq_tbls_2G[] = {
	ACPHY_TBL_ID_RFSEQ,
	0x260,
	15,
	0x0, 0x10, 0x8b, 0x9b, 0xaa, 0xb9, 0xc9, 0xd9, 0x8c, 0x9c,
	0x9d, 0xab, 0x8d, 0x9e, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x290,
	14,
	0x0, 0x10, 0x88, 0x98, 0xa8, 0xb8, 0xc8, 0xd8, 0x89, 0x99,
	0xa9, 0x8a, 0x9a, 0x1f,
	ACPHY_TBL_ID_RFSEQ,
	0x2f0,
	15,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04,
	0x04, 0x04, 0x01, 0x04, 0x01,
	ACPHY_TBL_ID_RFSEQ,
	0x320,
	14,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x04,
	0x04, 0x01, 0x04, 0x01,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x008,
	18,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x018,
	21,
	0x0009, 0x8000, 0x0003, 0x7f09, 0x801d, 0x0003, 0x7f09, 0x801d, 0x0000,
	0x00c9, 0x8018, 0x0003, 0x7fd9, 0x801d, 0x0003, 0x7ff9, 0x801d, 0x0003,
	0x7f39, 0x801d, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x028,
	12,
	0x0009, 0x0000, 0x0000, 0x0129, 0x0000, 0x0000, 0x0019, 0x0000, 0x0000,
	0x0059, 0x0000, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x038,
	6,
	0x0fc9, 0x00a6, 0x0000, 0x0fc9, 0x00a6, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x048,
	6,
	0x2099, 0x586e, 0x0000, 0x1519, 0x586e, 0x0000,
	ACPHY_TBL_ID_RFSEQBUNDLE,
	0x058,
	6,
	0x2409, 0xc040, 0x000c, 0x6b89, 0xc040, 0x000c,
};

static const uint16 k_tof_seq_fem_gains[] = {
	(8 | (1 << 5) | (1 << 9)), /* fem hi */
	(8 | (1 << 5)), /* fem lo */
};

const uint16 k_tof_seq_fem_gains_2g[] = {
	(8 | (1 << 5) | (1 << 8)), /* fem hi */
	(8 | (1 << 4) | (1 << 8)), /* fem lo */
};

#endif /* WL_PROXD_SEQ */

/* adjust the window for the EB delay, delay is in tenth of nano-sec */
#define TOF_W_ADJ_EMU(emu_delay, FS) ((emu_delay * 2 * FS) / 10000)
/* are we using the emulator box? */
#define TOF_EB(emu_delay) ((emu_delay > 2000) ? 1 : 0)
/* extend the sc buffer if using EB */
#define TOF_BUF_EXT 670
#define TOF_RX_MODE_EXT 8 /* extend the Rx mode when EB is used, in usec */
#define TOF_SC_FS_80MHZ 160
#define TOF_SC_FS_40MHZ 80
#define TOF_SC_FS_20MHZ 40

#ifdef WL_PROXD_SEQ
const uint32 k_tof_seq_spb_tx[2 * K_TOF_SEQ_SPB_LEN_MAX] = { 0 };
const uint32 k_tof_seq_spb_rx[2 * K_TOF_SEQ_SPB_LEN_MAX] = { 0 };

const uint16 band_length_20MHz[K_TOF_NUM_LEGACY_BL_20M] = { 25, 25 };
const uint16 nonzero_sc_idx_legacy_20MHZ[K_TOF_NUM_LEGACY_NZ_SC_20M] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
	38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
	61, 62
};

const uint16 band_length_80MHz[K_TOF_NUM_LEGACY_BL_80M] = { 13, 13, 13, 10, 10, 13, 13, 13 };
const uint16 nonzero_sc_idx_legacy_80MHZ[K_TOF_NUM_LEGACY_NZ_SC_80M] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
	198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221,
	227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253,
};

#if defined(TOF_SEQ_20MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT)
/* 20MHz case */
static const uint16 k_tof_ucode_dlys_us_20MHz[2][5] = {
	{ 1, 8, 8, TOF_W(200, 2, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
	200, 0, K_TOF_SEQ_LOG2_N_20MHZ) }, /* RX -> TX */

	{ 2, 6, 8, TOF_W(480, -2, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
	480, 0, K_TOF_SEQ_LOG2_N_20MHZ) }, /* TX -> RX */
};

/* 2G case */
const uint16 k_tof_ucode_dlys_us_2g_20MHz[2][5] = {
	{ 1, 21, 10, TOF_W(564, 4, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
	564, 0, K_TOF_SEQ_LOG2_N_20MHZ) }, /* RX -> TX 1012, 500 */
	{ 8, 11, 13, TOF_W(1084, -4, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
	1084, 0, K_TOF_SEQ_LOG2_N_20MHZ) }, /* TX -> RX 508, 1020 */
};

#ifdef TOF_DLY_290us_313us
/* 2G case */
const uint16 K_TOF_UCODE_DLYS_US_2G_20MHZ_2_0[2][5] = {
	{1, 29, 7, TOF_W(444, 7, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
		444, 0, K_TOF_SEQ_LOG2_N_20MHZ)}, /* 1900, 364 RX->TX Orig=428, n=12 */
	{9, 7, 47, TOF_W(1434, -7, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
		1434, 0, K_TOF_SEQ_LOG2_N_20MHZ)}, /* 444, 1980 TX->RX */
};
#endif

/* 2G Secure Ranging 2.0 case */
/* For Delays 273us from Target, 313us from Initiator */
const uint16 K_TOF_UCODE_DLYS_US_2G_20MHZ_2_0[2][5] = {
	{1, 45, 7, TOF_W(404, 12, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
		404, 0, K_TOF_SEQ_LOG2_N_20MHZ)}, /* 1900, 364 RX->TX Orig=428, n=12 */
	{9, 7, 47, TOF_W(2044, -12, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
		2044, 0, K_TOF_SEQ_LOG2_N_20MHZ)}, /* 444, 1980 TX->RX */
};

/* 2G case for 4360 and 43602 */
const uint16 K_TOF_UCODE_DLYS_US_2G_20MHZ_4360[2][5] = {
	{1, 21, 10, TOF_W(554, 4, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
		554, 0, K_TOF_SEQ_LOG2_N_20MHZ)}, /* RX -> TX 1000, 488 */
	{8, 11, 13, TOF_W(1094, -4, K_TOF_SEQ_LOG2_N_20MHZ), TOF_W(
		1094, 0, K_TOF_SEQ_LOG2_N_20MHZ)}, /* TX -> RX 488, 1000 */
};
#endif /* TOF_SEQ_20MHz_BW || TOF_SEQ_20MHz_BW_512IFFT */

#ifdef TOF_SEQ_40MHz_BW
/* For 20 in 40MHz */
static const uint16 k_tof_ucode_dlys_us_40MHz[2][5] = {
	{ 1, 8, 8, TOF_W(338, 3, K_TOF_SEQ_LOG2_N_40MHZ), TOF_W(
	338, 0, K_TOF_SEQ_LOG2_N_40MHZ) },  /* RX -> TX */
	{ 2, 6, 8, TOF_W(1188, -3, K_TOF_SEQ_LOG2_N_40MHZ), TOF_W(
	1188, 0, K_TOF_SEQ_LOG2_N_40MHZ) }, /* TX -> RX */
};
#endif /* TOF_SEQ_40MHz_BW */

#ifdef TOF_SEQ_40_IN_40MHz
/* For 40 in 40MHz */
static const uint16 k_tof_ucode_dlys_us_40MHz_40[2][5] = {
	{ 1, 8, 8, TOF_W(338, 3, K_TOF_SEQ_LOG2_N_40MHZ), TOF_W(
	338, 0, K_TOF_SEQ_LOG2_N_40MHZ) },  /* RX -> TX */
	{ 2, 6, 8, TOF_W(1188, -3, K_TOF_SEQ_LOG2_N_40MHZ), TOF_W(
	1188, 0, K_TOF_SEQ_LOG2_N_40MHZ) }, /* TX -> RX */
};
#endif /* TOF_SEQ_40_IN_40MHz */

#ifdef TOF_SEQ_20_IN_80MHz
/* For 20 in 80MHz */
static const uint16 k_tof_ucode_dlys_us_80MHz_20[2][5] = {
	{ 1, 8, 8, TOF_W(698, 3, K_TOF_SEQ_LOG2_N_80MHZ_20), TOF_W(
	698, 0, K_TOF_SEQ_LOG2_N_80MHZ_20) }, /* RX -> TX */
	{ 2, 6, 8, TOF_W(2368, -3, K_TOF_SEQ_LOG2_N_80MHZ_20), TOF_W(
	2368, 0, K_TOF_SEQ_LOG2_N_80MHZ_20) }, /* TX -> RX */
};
#endif /* TOF_SEQ_20_IN_80MHz */

#if !defined(TOF_SEQ_20_IN_80MHz)
/* Original 80MHz case */
static const uint16 k_tof_ucode_dlys_us_80MHz[2][5] = {
	{ 1, 6, 6, TOF_W(750, 4, K_TOF_SEQ_LOG2_N_80MHZ), TOF_W(
	750, 0, K_TOF_SEQ_LOG2_N_80MHZ) }, /* RX -> TX */
	{ 2, 6, 6, TOF_W(1850, -4, K_TOF_SEQ_LOG2_N_80MHZ), TOF_W(
	1850, 0, K_TOF_SEQ_LOG2_N_80MHZ) }, /* TX -> RX */
};
#endif /* !TOF_SEQ_20_IN_80MHz */

#if defined(TOF_SEQ_20_IN_80MHz) || defined(TOF_SEQ_20MHz_BW) || \
	defined(TOF_SEQ_40MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT)
static const uint32 k_tof_seq_spb_20MHz[2 * K_TOF_SEQ_SPB_LEN_20MHZ] = {
	0x1f11ff10,
	0x1fffff1f,
	0x1f1f1ff1,
	0x000ff111,
	0x11110000,
	0x1f1f11ff,
	0x1ff11111,
	0x1111f1f1,
};
#endif /* TOF_SEQ_20_IN_80MHz || TOF_SEQ_20MHz_BW || TOF_SEQ_40MHz_BW || TOF_SEQ_20MHz_BW_512IFFT */

#ifdef TOF_SEQ_40_IN_40MHz
static const uint32 k_tof_seq_spb_40MHz[2 * K_TOF_SEQ_SPB_LEN_40MHZ] = {
	0xf11ff110,
	0x111111f1,
	0x1f1f11ff,
	0x1ff11111,
	0xfff1f1f1,
	0xf1ff11ff,
	0xff1111f1,
	0x0000001f,
	0xf0000000,
	0x1ff11f11,
	0x1111f1f1,
	0x1f11ff11,
	0xf111111f,
	0xf1f1f11f,
	0xff11ffff,
	0x1111f1f1,
};
#endif /* TOF_SEQ_40_IN_40MHz */

#if !defined(TOF_SEQ_20_IN_80MHz)
static const uint32 k_tof_seq_spb_80MHz[2 * K_TOF_SEQ_SPB_LEN_80MHZ] = {
	0xee2ee200,
	0xe2e2ee22,
	0xe22eeeee,
	0xeeee2e2e,
	0xe2ee22ee,
	0xe22222e2,
	0xe2e2e22e,
	0x00000eee,
	0x11000000,
	0x1f1f11ff,
	0x1ff11111,
	0x1111f1f1,
	0x1f11ff11,
	0x1fffff1f,
	0x1f1f1ff1,
	0x01fff111,
};
#endif /* !defined(TOF_SEQ_20_IN_80MHz) */
#endif /* WL_PROXD_SEQ */

#ifdef WL_PROXD_PHYTS
/* Loopback related */

/* Default Rxgain loopback bundle */
/* Bundle format
 * [6:5]:gm_index, [10:8]:tia_lut_gain_index, [13:11]:lpf_lut_gain_index
 * [14]:vpath_auxpath_sw, [15]:ipath_mainpath_sw, [16]:ext_lna_gain
 * [17]:ext_lna_gain_valid
 */
#define PHYTS_LOOPBACK_RX_GAIN_BUNDLE	(2 << 6 | 3 << 3 | 4);
#define PHYTS_INVALID_TX_PWR -128

/* Typical ePA gain */
#define PHYTS_EPA_GAIN 27
/* MCS0 back off in dB */
#define PHYTS_MCS0_BACKOFF 2
/* Internal TR isolation */
#define PHYTS_ITR_ISOLATION 23
/* Ref power at ADC output */
#define PHYTS_LOOPBACK_REF_PWR -4

/* 11a standard defined lltf sequences, including sub-band rotation */
/* Unpacking */
/* (1->  1+  0*j) */
/* (f-> -1+  0*j) */
/* (2->  0+  1*j) */
/* (e->  0+ -1*j) */
#define REF_SYMB_NIBBLE_MASK 0x0000000fu
#define REF_SYMB_NIBBLE_SIZE 4u
/* L-LTF[0:N-1] 20MHz OFDM symbol */
static const uint32 lltf_20_mhz[8] = {
	0x1f11ff10, 0x1fffff1f, 0x1f1f1ff1, 0x00000111,
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1
};

/* L-LTF[0:N-1] 40MHz OFDM symbol */
static const uint32 lltf_40_mhz[16] = {
	0x22000000, 0x2e2e22ee, 0x2ee22222, 0x2222e2e2,
	0x2e22ee20, 0x2eeeee2e, 0x2e2e2ee2, 0x00000222,
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1,
	0x1f11ff10, 0x1fffff1f, 0x1f1f1ff1, 0x00000111
};

/* L-LTF[0:N-1] 80MHz OFDM symbol */
static const uint32 lltf_80_mhz[32] = {
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1,
	0x1f11ff10, 0x1fffff1f, 0x1f1f1ff1, 0x00000111,
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff
};

/* L-LTF[0:N-1] 160MHz OFDM symbol */
static const uint32 lltf_160_mhz[64] = {
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1,
	0x1f11ff10, 0x1fffff1f, 0x1f1f1ff1, 0x00000111,
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1,
	0x1f11ff10, 0x1fffff1f, 0x1f1f1ff1, 0x00000111,
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0xff000000, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11f0, 0xf11111f1, 0xf1f1f11f, 0x00000fff
};

/* 11ac standard defined vht ltf sequences, including sub-band rotation */
/* VHT-LTF[0:N-1] 20MHz OFDM symbol */
static const uint32 vhtltf_20_mhz[8] = {
	0x1f11ff10, 0x1fffff1f, 0x1f1f1ff1, 0x000ff111,
	0x11110000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1
};

/* VHT-LTF[0:N-1] 40MHz OFDM symbol */
static const uint32 vhtltf_40_mhz[16] = {
	0x22e22e00, 0x2e2e22ee, 0x2ee22222, 0x2222e2e2,
	0x2e22ee22, 0x2eeeee2e, 0x2e2e2ee2, 0x00000222,
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1,
	0x1f11ff11, 0x1fffff1f, 0x1f1f1ff1, 0x01fff111
};

/* VHT-LTF[0:N-1] 80MHz OFDM symbol */
static const uint32 vhtltf_80_mhz[32] = {
	0xfff11f00, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0xff111fff,
	0xff1ff1f1, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1,
	0x1f11ff11, 0x1fffff1f, 0x1f1f1ff1, 0x11fff111,
	0xff1ff1f1, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0x01f1ffff
};

/* VHT-LTF[0:N-1] 160MHz OFDM symbol */
static const uint32 vhtltf_160_mhz[64] = {
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1,
	0x1f11ff11, 0x1fffff1f, 0x1f1f1ff1, 0x11fff111,
	0xff1ff1f1, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0x01f1ffff,
	0xfff11f00, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0xff111fff,
	0xff1ff1f1, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0x00000fff,
	0x11000000, 0x1f1f11ff, 0x1ff11111, 0x1111f1f1,
	0x1f11ff11, 0x1fffff1f, 0x1f1f1ff1, 0x11fff111,
	0xff1ff1f1, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0x01f1ffff,
	0xfff11f00, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0xff111fff,
	0xff1ff1f1, 0xf1f1ff11, 0xf11fffff, 0xffff1f1f,
	0xf1ff11ff, 0xf11111f1, 0xf1f1f11f, 0x00000fff
};

/* HE-LTF[0:N/4-1] 2x 80MHz OFDM symbol, sampled at 4*delta_f */
static const uint32 heltf_2x80_mhz[32] = {
	0x11f11f10, 0x111fff1f, 0x1111ff1f, 0xff1f1ff1,
	0x11f11f1f, 0xff1f1fff, 0xf1ff11f1, 0x1111fff1,
	0xfffff111, 0xf11f1f11, 0x1fff1f11, 0xffff1f11,
	0x1f1ff11f, 0x11111fff, 0xf1f111ff, 0x001f1f11,
	0x1f1f1000, 0xf111f1f1, 0xff11111f, 0x11ff1f1f,
	0x1f1fffff, 0x1f1fff11, 0x1f1f11f1, 0x11fffff1,
	0x111fffff, 0x1ff11f1f, 0x11f1f11f, 0xf1ff1ff1,
	0x11f1f111, 0xf11fffff, 0xf111fff1, 0x11f11ff1
};

/* HE-LTF[0:N/4-1] 2x 160MHz OFDM symbol, sampled at 4*delta_f */
static const uint32 heltf_2x160_mhz[64] = {
	0x1f1f1000, 0xf111f1f1, 0xff11111f, 0x11ff1f1f,
	0x1f1fffff, 0x1f1fff11, 0x1f1f11f1, 0x11fffff1,
	0xfff11111, 0xf11ff1f1, 0xff1f1ff1, 0x1f11f11f,
	0xff1f1fff, 0x1ff11111, 0x1fff111f, 0x11f1f11f,
	0x11f11f10, 0x111fff1f, 0x1111ff1f, 0xff1f1ff1,
	0x11f11f1f, 0xff1f1fff, 0xf1ff11f1, 0x1111fff1,
	0x11111ff1, 0x1ff1f1ff, 0xf111f1ff, 0x1111f1ff,
	0xf1111ff1, 0xfffff111, 0x1f1fff11, 0x00f1f1ff,
	0x1f1f1000, 0xf111f1f1, 0xff11111f, 0x11ff1f1f,
	0x1f1fffff, 0x1f1fff11, 0x1f1f11f1, 0x11fffff1,
	0x111fffff, 0x1ff11f1f, 0x11f1f11f, 0xf1ff1ff1,
	0x11f1f111, 0xf11fffff, 0xf111fff1, 0x11f11ff1,
	0x11f11f10, 0x111fff1f, 0x1111ff1f, 0xff1f1ff1,
	0x11f11f1f, 0xff1f1fff, 0xf1ff11f1, 0x1111fff1,
	0xfffff111, 0xf11f1f11, 0x1fff1f11, 0xffff1f11,
	0x1f1ff11f, 0x11111fff, 0xf1f111ff, 0x001f1f11
};

/* FFT out of MF = (H * Window) to smooth the channel */
static const uint16 phyts_smooth_win[256u] = {
	64938u, 64896u, 64859u, 64826u, 64793u, 64722u, 64650u, 64577u,
	64502u, 64394u, 64285u, 64178u, 64069u, 63926u, 63782u, 63639u,
	63494u, 63316u, 63137u, 62960u, 62781u, 62569u, 62356u, 62144u,
	61931u, 61686u, 61440u, 61195u, 60950u, 60673u, 60395u, 60119u,
	59843u, 59535u, 59227u, 58920u, 58613u, 58275u, 57938u, 57602u,
	57266u, 56900u, 56535u, 56172u, 55809u, 55417u, 55026u, 54637u,
	54249u, 53833u, 53417u, 53004u, 52592u, 52153u, 51714u, 51279u,
	50844u, 50384u, 49926u, 49470u, 49015u, 48537u, 48060u, 47586u,
	47113u, 46618u, 46125u, 45635u, 45146u, 44636u, 44128u, 43623u,
	43121u, 42599u, 42079u, 41563u, 41050u, 40517u, 39987u, 39461u,
	38937u, 38397u, 37859u, 37326u, 36796u, 36249u, 35706u, 35167u,
	34632u, 34082u, 33536u, 32994u, 32456u, 31905u, 31358u, 30815u,
	30277u, 29726u, 29180u, 28639u, 28102u, 27555u, 27012u, 26474u,
	25941u, 25399u, 24862u, 24330u, 23803u, 23267u, 22737u, 22213u,
	21693u, 21159u, 20634u, 20122u, 19623u, 19131u, 18635u, 18126u,
	17598u, 17032u, 16488u, 16005u, 15625u, 15376u, 15290u, 15376u,
	15625u, 16005u, 16488u, 17032u, 17598u, 18126u, 18635u, 19131u,
	19623u, 20122u, 20634u, 21159u, 21693u, 22213u, 22737u, 23267u,
	23803u, 24330u, 24862u, 25399u, 25941u, 26474u, 27012u, 27555u,
	28102u, 28639u, 29180u, 29726u, 30277u, 30815u, 31358u, 31905u,
	32456u, 32994u, 33536u, 34082u, 34632u, 35167u, 35706u, 36249u,
	36796u, 37326u, 37859u, 38397u, 38937u, 39461u, 39987u, 40517u,
	41050u, 41563u, 42079u, 42599u, 43121u, 43623u, 44128u, 44636u,
	45146u, 45635u, 46125u, 46618u, 47113u, 47586u, 48060u, 48537u,
	49015u, 49470u, 49926u, 50384u, 50844u, 51279u, 51714u, 52153u,
	52592u, 53004u, 53417u, 53833u, 54249u, 54637u, 55026u, 55417u,
	55809u, 56172u, 56535u, 56900u, 57266u, 57602u, 57938u, 58275u,
	58613u, 58920u, 59227u, 59535u, 59843u, 60119u, 60395u, 60673u,
	60950u, 61195u, 61440u, 61686u, 61931u, 62144u, 62356u, 62569u,
	62781u, 62960u, 63137u, 63316u, 63494u, 63639u, 63782u, 63926u,
	64069u, 64178u, 64285u, 64394u, 64502u, 64577u, 64650u, 64722u,
	64793u, 64826u, 64859u, 64896u, 64938u, 64954u, 64981u, 65024u
};

/* Sample-capture related */
/* module private states */
#define SC_STATE_NONE 0x00u
#define SC_STATE_SETUP 0x01u
#define SC_STATE_ENAB 0x02u
#define SC_STATE_DISB 0x04u
#define SC_STATE_READ 0x08u
/* SC modes */
#define SC_MODE_FARROW_OUTPUT 4u
#define SC_MODE_RX_FITLER 7u
/* SC read modes */
#define SC_READ_ENERGY_DETECT		0u
#define SC_READ_FTM_LLTF		1u
#define SC_READ_FTM_HE_VHTLTF		2u
#define SC_READ_ACK_LLTF		3u
#define SC_READ_ACK_HE_VHTLTF		4u
#define SC_READ_FULL			5u
#define SC_READ_HEACK_ENERGY_DETECT	6u
/* SC read buffer length
 * Read buff will have samples as below
 * For 5G band
 * | 16us energy_det_samps | 0.1us gap | 3.2us FTM LLTF | 0.1us gap | 3.2us FTM VHTLTF | ...
 * For 6G band, supporting 2xHE-LTF
 * | 16us energy_det_samps | 0.1us gap | 3.2us FTM LLTF | 0.1us gap | 6.4us FTM HELTF | ...
 */
#define SC_READ_BUF_GAP_DUR_NS 100u
#define PHYTS_SC_READ_BUF_DUR_5G_US 30u
#define PHYTS_SC_READ_BUF_DUR_6G_US 36u
#define PHYTS_SC_READ_BUF_5G_LEN(bw_mhz) (PHYTS_SC_READ_BUF_DUR_5G_US * bw_mhz)
#define PHYTS_SC_READ_BUF_6G_LEN(bw_mhz) (PHYTS_SC_READ_BUF_DUR_6G_US * bw_mhz)

/* Match filtering modes at interface */
#define PHYTS_MF_MODE_FTM 0u
#define PHYTS_MF_MODE_ACK 1u
#define PHYTS_MF_FREQ_MODE_FTM_LLTF 0u
#define PHYTS_MF_FREQ_MODE_FTM_VHTLTF 1u
#define PHYTS_MF_FREQ_MODE_ACK_LLTF 2u
#define PHYTS_MF_FREQ_MODE_ACK_VHTLTF 3u
#define PHYTS_MF_FREQ_MODE_FTM_HELTF 4u
#define PHYTS_MF_FREQ_MODE_ACK_HELTF 5u

#define PHYTS_L_FTM_MODE(mode) (mode == PHYTS_MF_FREQ_MODE_FTM_LLTF)
#define PHYTS_L_ACK_MODE(mode) (mode == PHYTS_MF_FREQ_MODE_ACK_LLTF)
#define PHYTS_L_LTF_MODE(mode) (PHYTS_L_FTM_MODE(mode) || PHYTS_L_ACK_MODE(mode))
#define PHYTS_VHT_LTF_MODE(mode) ((mode == PHYTS_MF_FREQ_MODE_FTM_VHTLTF) ||\
		(mode == PHYTS_MF_FREQ_MODE_ACK_VHTLTF))
#define PHYTS_HE_LTF_MODE(mode) ((mode == PHYTS_MF_FREQ_MODE_FTM_HELTF) ||\
		(mode == PHYTS_MF_FREQ_MODE_ACK_HELTF))
#define PHYTS_VHT_HE_LTF_MODE(mode) (PHYTS_VHT_LTF_MODE(mode) || PHYTS_HE_LTF_MODE(mode))
#define PHYTS_VHT_HE_FTM_MODE(mode) ((mode == PHYTS_MF_FREQ_MODE_FTM_VHTLTF) ||\
			(mode == PHYTS_MF_FREQ_MODE_FTM_HELTF))
#define PHYTS_VHT_HE_ACK_MODE(mode) ((mode == PHYTS_MF_FREQ_MODE_ACK_VHTLTF) ||\
			(mode == PHYTS_MF_FREQ_MODE_ACK_HELTF))

#define PHYTS_IS_INITIATOR(role) (role == PHYTS_ROLE_INITIATOR)
#define PHYTS_IS_TARGET(role) (role == PHYTS_ROLE_TARGET)

/* Energy detection related */
#define ENERGYDET_SC_READ_DUR_NS 16000u
#define ENERGYDET_SC_READ_LEN(bw_mhz) (ENERGYDET_SC_READ_DUR_NS * bw_mhz / 1000u)
#define ENERGYDET_ACC_WIN_NS 200u
#define ENERGYDET_ACC_WIN_LEN(bw_mhz)  ((ENERGYDET_ACC_WIN_NS * bw_mhz)/1000u)
#define ENERGYDET_DELTA_THRESH_INITIATOR	1000000u
#define ENERGYDET_DELTA_THRESH_TARGET	1000000u
#define ENERGYDET_SATURATION_CHECK(x) (((x >> 30u) & 0x3) > 0u)
#define ENERGYDET_WAIT_IN_ENERGY_DETECT 0x0u
#define ENERGYDET_WAIT_IN_SPIKE_DETECT 0x1u
#define ENERGYDET_ENERGY_DROP_THRESH_SHIFT 3u
#define ENERGYDET_SPIKE_DETECT_WINDOWS 5u

#define FFT_WINDOW_DUR_NS 3200u
/* Extra duration read before preamble */
#define EXTR_NOISE_DUR_US	12u
/* L-STF(8us) + L-LTF(8us) + L-SIG (4us) */
#define LEGACY_PREAMBLE_DUR_US	20u
/* Fixed portion of the VHT preamble */
/* VHT-SIGA(8us) + VHT-STF(4us) + VHT-SIGB(4us), excluding VHTLTF */
#define VHT_PREAMBLE_FIXED_DUR_US	16u
#define VHT_LTF_DUR_US	4u
/* Fixed portion of the HE preamble */
/* RLSIG(4us) + HE-SIGA(8us) + HE-STF(4us), excluding HELTF */
#define HE_PREAMBLE_FIXED_DUR_US	16u
#define HE_LTF_2X_1P6_DUR_US	8u
/* Data portion */
#define VHT_SGI_DURATION_NS(sgi) (sgi ? 3600u : 4000u)
#define DATA_SYMBOLS_DUR_US(nsym, sgi)\
	((nsym * VHT_SGI_DURATION_NS(sgi)) / 1000u)
/* 1.6us CP mode supported, 12.8 + 1.6 */
#define HE_DATA_SYMBOLS_DUR_NS(nsym) (nsym * 14400u)
#define HE_DATA_SYMBOLS_DUR_US(nsym) (HE_DATA_SYMBOLS_DUR_NS(nsym) / 1000u)

/* MAX SIFS variation */
#define MAX_SIFS_VARIATION_NS 800u
#define SIFS_PLUS_VARIATIONS_NS (16000u + MAX_SIFS_VARIATION_NS)

/* Picking Match filtering windows */
/* From packet start considering 8us of STF and 1.6us of DGI */
#define PKTSTRT_TO_LLTF_OFF_NS 10400 //9600u
#define PKTSTRT_TO_LLTF_OFF_LEN(bw_mhz) ((PKTSTRT_TO_LLTF_OFF_NS * bw_mhz)/1000u)
/* 3.2us (L-LTF2) + 12us (SIG) + 4(VHT-STF) + 0.4us(to avoid ISI) = 19.6us */
#define LLTF2_TO_VHTLTF_OFF_NS 19600u
#define LLTF2_TO_VHTLTF_OFF_LEN(bw_mhz) ((LLTF2_TO_VHTLTF_OFF_NS * bw_mhz)/1000u)
/* Considering 1.6us GI + 2xLTF */
/* 3.2us (L-LTF2) + 16us (LSIG + RLSIG + HE-SIGA) + 4us (HE-STF) + 0.8us (to avoid ISI, */
/* considering 1.6us GI) = 24us, BCA: Add 0.4us for better timing of match filtering windows. */
#define LLTF2_TO_1P6GI_2XHELTF_OFF_NS 24400u
#define LLTF2_TO_1P6GI_2XHELTF_OFF_LEN(bw_mhz) ((LLTF2_TO_1P6GI_2XHELTF_OFF_NS * bw_mhz)/1000u)
/* VHT-LTF's MF output's starting point to data, 3.2us for first VHT LTF + 4us*(num_ltf-1) */
/* + 4us for VHT-sigb */
#define VHTLTF_TO_DATA_OFF_NS(num_ltf) (7200u + ((VHT_LTF_DUR_US * 1000u) * (num_ltf - 1)))
#define VHTLTF_TO_DATA_OFF_LEN(bw_mhz, num_ltf)\
	((VHTLTF_TO_DATA_OFF_NS(num_ltf) * bw_mhz)/1000u)
/* HE-LTF's MF output's starting point to data, 6.4us for first HE-LTF + 8us * (num_ltf-1) */
/* Condidering only 1.6us GI and 2x mode */
#define HELTF_TO_DATA_OFF_NS(num_ltf) (6400u + \
	((HE_LTF_2X_1P6_DUR_US * 1000u) * (num_ltf - 1)))
#define HELTF_TO_DATA_OFF_LEN(bw_mhz, num_ltf) \
	((HELTF_TO_DATA_OFF_NS(num_ltf) * bw_mhz)/1000u)
/* (VHT Data portion(Nsym * (4us or 3.6us based on sGI)) + SIFS + */
/* pktstart to LLTF including DGI */
#define DATA_SIFS_PKTSTRT_TO_LLTF_OFF_NS(nsym, sgi) (nsym * VHT_SGI_DURATION_NS(sgi) +\
	SIFS_PLUS_VARIATIONS_NS + PKTSTRT_TO_LLTF_OFF_NS)
#define DATA_SIFS_PKTSTRT_TO_LLTF_OFF_LEN(nsym, sgi, bw_mhz)\
	((DATA_SIFS_PKTSTRT_TO_LLTF_OFF_NS(nsym, sgi) * bw_mhz) / 1000u)
/* HE Data portion = Nsym * 8us) + PE +  SIFS + pktstart to LLTF including DGI */
#define HE_DATA_SIFS_PKTSTRT_TO_LLTF_OFF_NS(nsym, pe_us) \
	(HE_DATA_SYMBOLS_DUR_NS(nsym) + (pe_us * 1000u) + SIFS_PLUS_VARIATIONS_NS +\
	PKTSTRT_TO_LLTF_OFF_NS)
#define HE_DATA_SIFS_PKTSTRT_TO_LLTF_OFF_LEN(nsym, pe_us, bw_mhz) \
	((HE_DATA_SIFS_PKTSTRT_TO_LLTF_OFF_NS(nsym, pe_us) * bw_mhz) / 1000u)

/* First path related */
#define FIRSTP_IDX_SCALE 10000u
/* Bits required to hold the scale value */
/* ceil(log2(FIRSTP_IDX_SCALE)) */
#define FIRSTP_IDX_SCALE_BITS 14u
#define FIRSTP_IDX_BITS_ALLOWED (32u - FIRSTP_IDX_SCALE_BITS)
#define FIRSTP_US_TO_PS 1000000u

/* To enable 2xFFT at match filtering output */
#define PHYTS_APPLY_2X_IFFT
/* Optimization: average 3.2us windows for HE-LTF to use reduced FFT size */
#ifdef PHYTS_APPLY_2X_IFFT /* 2x Memory is reserved and needed for averaging */
#define PHYTS_APPLY_3P2US_WIN_AVERAGE
#endif /* PHYTS_APPLT_2X_IFFT */
/* To apply smoothing window in freq domain */
#define PHYTS_APPLY_SMOOTHING_WIN
#define SMOOTHING_WIN_BITS 16
/* To enable right search of the firstp */
/* Apply a window offset from the max power tap and search in the right direction */
/* i.e., ([max_tap -0.4us to max_tap]), to avoid missing any significant precursors */
#define PHYTS_FIRSTP_RIGHT_SEARCH
#define PHYTS_FIRSTP_SEARCH_WINDOW_NS 400u
/* To enable spike detection during packet/energy detection phase */
#define PHYTS_ENERGYDET_SPIKE_DETECT
/* To enable validation check at the output of match-filtering */
#define PHYTS_MATCHFILTER_VALIDATION
/* (max_acc_energy_win - min_acc_energy_win) > thresh */
#define PHYTS_ENERGY_ACC_RANGE_THRESH_DB 6u

#endif /* WL_PROXD_PHYTS */
#define PHYTS_ENERGY_ACC_WINDOWS 4u

typedef struct phy_ac_tof_phyts_info {
	/* Common */
	uint16 bw_mhz;
	uint16 nfft;
	int cfo;
	int rxiq_a;
	int rxiq_b;
	/* Sample capture related */
	uint8 sc_mode;
	uint32 sc_start;
	uint32 sc_end;
	uint32 sc_sz;
	uint8 sc_state;
	uint32 sc_ftm_start;
	uint32 sc_ftm_end;
	uint32 sc_ftm_dur;
	uint32 sc_curr_ptr;
	uint32 sc_read_offsets[7u];
	uint32 sc_buff_offsets[5u];
	uint32 save_clkgatests;
	/* loopback related */
	uint32 loopback_rxgain_bundle;		/* Loopback rxgain bundle value */
	uint16 saved_rx2tx_cmd[16];
	uint16 saved_tx2rx_cmd[16];
	uint16 saved_location_control;
	uint16 saved_tssimode;
	uint16 saved_rffectrl1;
	uint16 save_sdfeClkGatingCtrl;		/* Save sdfeClkGatingCtrl PHY Register */
	uint16 saved_bundle_farrow[3];
	uint16 saved_rx1spare;
	uint16 saved_tdsfocorr;
	uint16 saved_adcdatacollect;
	uint16 saved_rxfetesmmuxctrl;
	uint16 saved_spectrumanalyzermode;
	uint16 saved_specanadatacollect;
	uint16 radio_regs_afe[PHY_CORE_MAX][4];
	uint16 radio_regs_adc[PHY_CORE_MAX][5];
	uint16 radio_regs_dac[PHY_CORE_MAX][3];
	uint16 save_rfcoreactv;
	uint16 save_forceFront[PHY_CORE_MAX];
	uint16 save_RX2G_REG3[PHY_CORE_MAX];
	uint16 save_RX2G_CFG1_OVR[PHY_CORE_MAX];
	uint16 saved_rfseq_bundle_48[3];
	uint16 saved_rfseq_rx2tx_tia[PHY_CORE_MAX];
	uint16 saved_rfseq_bundle_aux[PHY_CORE_MAX];
	uint16 saved_rfseq_bundle_aux_tone[PHY_CORE_MAX];
	uint16 save_fineRxclockgatecontrol;
	uint16 save_gaincode[PHY_CORE_MAX];
	uint16 save_AfePuCtrl;			/* Save AfePuCtrl Radio Register */
	uint16 save_tssi_sense[PHY_CORE_MAX];   /* Save TSSI sens limit reg */
	int8 tx_pwr;				/* Transmit power */
	int8 loopback_rxgain_applied;		/* Loopbaxk rxgain applied */
	bool is_loopback_setup;                 /* Flag to indicate loopback setup status */
	/* Post-processing related */
	uint8 firstp_thresh_scale;
	uint8 firstp_thresh_log2_div;
	uint16 energydet_win_len;
	uint16 pktdet_limit_len;
	uint32 energy_win_1;
	uint32 energy_win_n;
	int32 energy_win_delta;
	int32 energy_delta_thres;
	uint16 pktstart_idx;
	uint16 ack_pktstart_idx;
	uint16 ftm_lltf_coarse_idx;
	uint32 ftm_lltf_maxpwr;
	uint16 ftm_lltf_maxpwr_idx;
	uint16 ftm_lltf_maxpwr_fft_idx;
	uint16 ftm_he_vhtltf_coarse_idx;
	uint32 ftm_he_vhtltf_maxpwr;
	uint16 ftm_he_vhtltf_maxpwr_idx;
	uint16 ftm_he_vhtltf_maxpwr_fft_idx;
	uint32 ftm_he_vhtltf_firstp_idx;
	uint32 ftm_he_vhtltf_firstp_ps;
	uint16 ack_lltf_coarse_idx;
	uint32 ack_lltf_maxpwr;
	uint16 ack_lltf_maxpwr_idx;
	uint16 ack_lltf_maxpwr_fft_idx;
	uint16 ack_he_vhtltf_coarse_idx;
	uint32 ack_he_vhtltf_maxpwr;
	uint16 ack_he_vhtltf_maxpwr_idx;
	uint16 ack_he_vhtltf_maxpwr_fft_idx;
	uint32 ack_he_vhtltf_firstp_idx;
	uint32 ack_he_vhtltf_firstp_ps;
	/* Params added in the end to avoid ROM abandons */
	/* Common */
	bool is_setup_called;
	uint8 role;				/* Role : Initiator/Responder */
	uint8 fft_2x_mode;
	/* Kvalue related */
	uint32 phyts_kval_2g20m[14];
	uint32 phyts_kval_5g20m[25];
	uint32 phyts_kval_5g40m[12];
	uint32 phyts_kval_5g80m[6];		/* kvalues across channels */
	uint32 phyts_kval_6g80m[14];
	uint8 core;
	uint32 lsig_length;
	uint16 frame_type;
	uint32 acc_cir_pwr[PHYTS_ENERGY_ACC_WINDOWS];
} phy_ac_tof_phyts_info_t;

struct phy_ac_tof_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_tof_info_t *ti;
	uint32 proxd_80m_k_values[6];
	uint32 proxd_40m_k_values[12];
	uint32 proxd_20m_k_values[25];
	uint32 proxd_2g_k_values[14];
	int32 proxdi_rate_offset_80m[4];
	int32 proxdi_rate_offset_40m[4];
	int32 proxdi_rate_offset_20m[4];
	int32 proxdi_rate_offset_2g[4];
	int32 proxdt_rate_offset_80m[4];
	int32 proxdt_rate_offset_40m[4];
	int32 proxdt_rate_offset_20m[4];
	int32 proxdt_rate_offset_2g[4];
	int16 proxdi_ack_offset[4];
	int16 proxdt_ack_offset[4];
	int32 proxd_subbw_offset[3][5];
	int32 rssi[PHY_CORE_MAX];
	uint16 proxd_ki[4];
	uint16 proxd_kt[4];
	uint16 tof_ucode_dlys_us[2][5];
	uint16 tof_rfseq_bundle_offset;
	uint16 tof_shm_ptr;
	uint32 *tof_seq_spb_tx;
	uint32 *tof_seq_spb_rx;
	uint8 tof_sc_FS;
	uint8 tof_seq_log2_n;
	uint8 tof_seq_spb_len;
	uint8 tof_rx_fdiqcomp_enable;
	uint8 tof_tx_fdiqcomp_enable;
	uint8 tof_core;
	uint8 tof_smth_enable;
	uint8 tof_smth_dump_mode;
	uint32 alloc_size;
	uint8 *rtx;
	uint8 *rrx;
	uint8 *ri_rr;
	int32 *chan;
	wl_proxd_phy_error_t tof_phy_error;
	wl_proxd_snr_t snr;
	wl_proxd_bitflips_t bitflips;
	bool tof_setup_done;
	bool tof_active;
	bool tof_smth_forced;
	bool tof_tx;
	uint32 tof_pllctrl;
	uint8 isInvalid;
	uint16 proxd_seq_kval[4];
	uint8 aci_en;
	uint8 aci_state;
	uint8 prev_aci_state;
	uint8 hwaci_sleep;
	int32 emu_delay;
	bool flag_sec_2_0;
	uint16 bundle_offs_38[3];
	uint16 bundle_offs_39[3];
	uint16 start_seq_time;
	uint16 delta_time_tx2rx;
	int32 tof_lesi_saved;
	uint8 hwobss_en;
	uint16 proxd_ki_5g_160;
	uint16 proxd_kt_5g_160;
	uint32 proxd_160m_k_values[2];
	int32 proxdi_rate_offset_160m[4];
	int32 proxdt_rate_offset_160m[4];
	int32 proxd_subbw_offset_160m[3][5];
	uint16 proxd_k_6g[4];	/* Base kvalues for 6G 160, 80, 40, 20 */
	uint8 proxd_6g_160m_kval_off[7]; /* Offset across channels */
	uint8 proxd_6g_80m_kval_off[14]; /* Offset across channels */
	uint8 proxd_6g_40m_kval_off[29]; /* Offset across channels */
	uint8 proxd_6g_20m_kval_off[59]; /* Offset across channels */
	phy_ac_tof_phyts_info_t *phytsi;
};

typedef struct {
	uint32 id;
	uint32 core;
	uint32 width;
	uint32 read_len;
	uint32 sts_offset;
	uint32 Hsb80l_offset;
	bool chansmooth;
	uint8 nsts;
} h_tbl_info_t;

/* Local functions */
#ifdef WL_PROXD_SEQ
static void phy_ac_tof_en_dis_aci(phy_ac_tof_info_t *tofi, bool enter);
#endif /* WL_PROXD_SEQ */
static void phy_ac_tof_fdiqcomp_save_disable(phy_ac_tof_info_t *tofi);
static int phy_ac_tof(phy_type_tof_ctx_t *ctx, bool enter, bool tx, bool hw_adj,
	bool seq_en, int core, int emu_delay);
static int phy_ac_tof_info(phy_type_tof_ctx_t *ctx, wlc_phy_tof_info_t *tof_info,
	wlc_phy_tof_info_type_t tof_info_mask, uint8 core);
static void phy_ac_tof_cmd(phy_type_tof_ctx_t *ctx, bool seq, int emu_delay);
#ifdef WL_PROXD_SEQ
static int phy_ac_tof_seq_params_get_set(phy_type_tof_ctx_t *ctx, uint8 *delays,
	bool set, bool tx, int size);
static int phy_ac_tof_set_ri_rr(phy_type_tof_ctx_t *ctx, const uint8 *ri_rr, const uint16 len,
	const uint8 core, const bool isInitiator, const bool isSecure,
	wlc_phy_tof_secure_2_0_t secure_params);
#endif /* WL_PROXD_SEQ */
static int phy_ac_tof_kvalue(phy_type_tof_ctx_t *ctx, chanspec_t chanspec, uint32 rspecidx,
	uint32 *kip, uint32 *ktp, uint8 seq_en);
#ifdef WL_PROXD_SEQ
static void phy_ac_tof_seq_upd_dly(phy_type_tof_ctx_t *ctx, bool tx, uint8 core,
	bool mac_suspend);
static int phy_ac_tof_seq_params(phy_type_tof_ctx_t *ctx, bool assign_buffer);
#endif /* WL_PROXD_SEQ */
static int phy_ac_tof_chan_freq_response(phy_type_tof_ctx_t *ctx, int len, int nbits,
		bool swap_pn_half, uint32 offset, cint32* H, uint32* Hraw, uint8 core,
		uint8 sts_offset, const bool single_core, bool tdcs_en, bool he_frm);
#ifdef WL_PROXD_SEQ
static void phy_ac_tof_setup_ack_core(phy_type_tof_ctx_t *ctx, int core);
static void phy_ac_tof_core_select(phy_type_tof_ctx_t *ctx, const uint32 gdv_th,
		const int32 gdmm_th, const int8 rssi_th, const int8 delta_rssi_th,
		uint8* core, uint8 core_mask);
static int phy_ac_tof_calc_snr_bitflips(phy_type_tof_ctx_t *ctx, void *In,
	wl_proxd_bitflips_t *bit_flips, wl_proxd_snr_t *snr);
#endif /* WL_PROXD_SEQ */
static int phy_ac_chan_mag_sqr_impulse_response(phy_type_tof_ctx_t *ctx, int frame_type,
	int len, int offset, int nbits, int32* h, int* pgd, uint32* hraw, uint16 tof_shm_ptr);
#ifdef WL_PROXD_SEQ
static int phy_ac_seq_ts(phy_type_tof_ctx_t *ctx, int n, cint32* p_buffer, int tx, int cfo,
	int adj, void* pparams, int32* p_ts, int32* p_seq_len, uint32* p_raw, uint8* ri_rr,
	const uint8 smooth_win_en);
#endif /* WL_PROXD_SEQ */
static void phy_ac_nvram_proxd_read_6g(phy_info_t *pi,  phy_ac_tof_info_t *tofi);
static void phy_ac_nvram_proxd_read(phy_info_t *pi,  phy_ac_tof_info_t *tofi);
#ifdef WL_PROXD_SEQ
static int phy_ac_tof_reset(phy_type_tof_ctx_t *ctx);
static int phy_ac_tof_set_ri_rr_spb_only(phy_info_t *pi, const bool isInitiator,
		const bool macSuspend);
static void phy_ac_tof_fill_write_k_tof_seq_ucode_regs(phy_info_t *pi);
static void phy_ac_tof_fill_write_shm(phy_info_t *pi, uint16 *shm, uint8 core, uint16 mask);
#endif /* WL_PROXD_SEQ */
static int phy_ac_tof_read_chan_tbl(phy_info_t* pi, uint32 *H, h_tbl_info_t *h_tbl_info);
static int phy_ac_tof_get_chan_tbl_info(phy_info_t* pi, h_tbl_info_t *h_tbl_info,
		int fft_len, bool he_frm);
static int phy_ac_tof_get_chan(phy_info_t* pi, uint32 *Hout,
		h_tbl_info_t *h_tbl_info, int fft_len, bool he_frm);
/* DEBUG */
#ifdef TOF_DBG
static int phy_ac_tof_dbg(phy_type_tof_ctx_t *ctx, int arg);
#endif
#ifdef WL_PROXD_SEQ
static void phy_ac_tof_init_gdmm_th(phy_type_tof_ctx_t *ctx, int32 *gdmm_th);
static void phy_ac_tof_init_gdv_th(phy_type_tof_ctx_t *ctx, uint32 *gdv_th);
static void phy_ac_tof_adjust_rx_tx_timing(phy_info_t *pi, bool tx);
#endif /* WL_PROXD_SEQ */

#if defined(SAMPLE_COLLECT) || defined(WL_PROXDETECT)
extern void phy_ac_disab_mac_clkgate_sc(phy_info_t *pi, uint32 *savereg);
extern void phy_ac_enab_mac_clkgate_sc(phy_info_t *pi, uint32 restorereg);
#endif /* SAMPLE_COLLECT || WL_PROXDETECT */

#ifdef WL_PROXD_PHYTS
static int phy_ac_tof_phyts_setup(phy_type_tof_ctx_t *ctx, bool setup, uint8 role);
static int phy_ac_tof_phyts_read_sc(phy_type_tof_ctx_t *ctx, cint16 *sc_mem);
static int phy_ac_tof_phyts_enable_sc(phy_type_tof_ctx_t *ctx);
static int phy_ac_tof_phyts_pkt_detect(phy_type_tof_ctx_t *ctx, cint16 *sc_mem);
static int phy_ac_tof_phyts_he_ack_pkt_detect(phy_type_tof_ctx_t *ctx, cint16 *sc_mem,
	uint16 sc_off);
static int phy_ac_tof_phyts_mf(phy_type_tof_ctx_t *ctx, cint16 *sc_mem, uint32 *ts,
	bool mode);
static int phy_ac_tof_phyts_loopback(phy_type_tof_ctx_t *ctx, bool setup);
static int phy_ac_tof_phyts_set_txpwr(phy_type_tof_ctx_t *ctx, int8 tx_pwr_dbm);
static int phy_ac_tof_phyts_get_sc_read_buf_sz(phy_type_tof_ctx_t *ctx, uint32 *sc_read_buf_sz);
#ifdef WL_PROXD_PHYTS_DEBUG
/* Stores the phytsi structure of past PHYTSI_DBG_ARR_LEN ftms */
#define PHYTSI_DBG_ARR_LEN 10u
#define PHYTSI_MAX_CIR_LEN 512u /* stores the last ftm and ack vhtltfs cir_mag array */
phy_ac_tof_phyts_info_t phytsi_dbg[PHYTSI_DBG_ARR_LEN];
uint32 phyts_cir_mag_dbg[2u][PHYTSI_MAX_CIR_LEN];
uint8 phyts_dbg_arr_idx = 0u;
#endif /* WL_PROXD_PHYTS_DEBUG */
#endif /* WL_PROXD_PHYTS */

/* register phy type specific implementation */
phy_ac_tof_info_t *
BCMATTACHFN(phy_ac_tof_register_impl)(phy_info_t *pi, phy_ac_info_t *aci, phy_tof_info_t *ti)
{
	phy_ac_tof_info_t *tofi;
	phy_type_tof_fns_t fns;

#ifdef WL_PROXD_PHYTS
	phy_ac_tof_phyts_info_t *phyts_info = NULL;
#endif /* WL_PROXD_PHYTS */

	uint32 tofi_size = sizeof(phy_ac_tof_info_t);
	uint32 rtx_size = FTM_TPK_RI_PHY_LEN_SECURE_2_0 * sizeof(uint8);
	uint32 rrx_size = FTM_TPK_RI_PHY_LEN_SECURE_2_0 * sizeof(uint8);
	uint32 ri_rr_size = FTM_RI_RR_BUF_LEN * sizeof(uint8);
	uint32 chan_size = 2 * (PHY_CORE_MAX + 1) * K_TOF_COLLECT_CHAN_SIZE * sizeof(int32);
	uint8 align_boundary = 4;
	/*
	Alloc size is 3 bytes more than total required size, this is get 4 byte alignment
	for chan
	*/
	uint32 alloc_size = tofi_size + rtx_size + rrx_size + ri_rr_size + chan_size;
	alloc_size = ALIGN_SIZE(alloc_size, align_boundary);

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((tofi = phy_malloc(pi, alloc_size)) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	tofi->pi = pi;
	tofi->aci = aci;
	tofi->ti = ti;
	tofi->alloc_size = alloc_size;

	tofi->rtx = ((uchar *)tofi) + tofi_size;
	tofi->rrx = ((uchar *)tofi->rtx) + rtx_size;
	tofi->ri_rr = ((uchar *)tofi->rrx) + rrx_size;
	tofi->chan = (int32 *)(((uint8 *)tofi->ri_rr) + ri_rr_size);
	/* Align chan to 4 byte address */
	tofi->chan = ALIGN_ADDR(tofi->chan, align_boundary);

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));

#ifdef WL_PROXD_PHYTS
	phyts_info = phy_malloc(pi, sizeof(phy_ac_tof_phyts_info_t));
	if (phyts_info == NULL) {
		PHY_ERROR(("phy_ac_tof_register_impl: phy_malloc failed\n"));
		goto fail;
	}
	tofi->phytsi = phyts_info;

	/* Initialize default rxgain bundle value */
	phyts_info->loopback_rxgain_bundle = PHYTS_LOOPBACK_RX_GAIN_BUNDLE;
	phyts_info->tx_pwr = PHYTS_INVALID_TX_PWR;

	/* Intialize flags */
	phyts_info->is_setup_called = FALSE;
	phyts_info->is_loopback_setup = FALSE;

	/* Initialize first path threshold, around 9dB lower compared to max value */
	phyts_info->firstp_thresh_scale = 1u;
	phyts_info->firstp_thresh_log2_div = 3u;

	/* SC mode */
	phyts_info->sc_mode = SC_MODE_RX_FITLER;
#endif /* WL_PROXD_PHYTS */

#ifdef WL_PROXD_SEQ
	fns.init_tof = phy_ac_tof_reset;
#endif /* WL_PROXD_SEQ */
	fns.tof = phy_ac_tof;
	fns.cmd = phy_ac_tof_cmd;
	fns.info = phy_ac_tof_info;
	fns.kvalue = phy_ac_tof_kvalue;
#ifdef WL_PROXD_SEQ
	fns.seq_params = phy_ac_tof_seq_params;
	fns.seq_upd_dly = phy_ac_tof_seq_upd_dly;
#endif /* WL_PROXD_SEQ */
	fns.chan_freq_response = phy_ac_tof_chan_freq_response;
	fns.chan_mag_sqr_impulse_response = phy_ac_chan_mag_sqr_impulse_response;
#ifdef WL_PROXD_SEQ
	fns.seq_ts = phy_ac_seq_ts;
	fns.set_ri_rr = phy_ac_tof_set_ri_rr;
	fns.seq_params_get_set_acphy = phy_ac_tof_seq_params_get_set;
	fns.setup_ack_core = phy_ac_tof_setup_ack_core;
	fns.core_select = phy_ac_tof_core_select;
	fns.init_gdmm_th = phy_ac_tof_init_gdmm_th;
	fns.init_gdv_th = phy_ac_tof_init_gdv_th;
	fns.calc_snr_bitflips = phy_ac_tof_calc_snr_bitflips;
#endif /* WL_PROXD_SEQ */

#ifdef WL_PROXD_PHYTS
	fns.phyts_setup = phy_ac_tof_phyts_setup;
	fns.phyts_read_sc = phy_ac_tof_phyts_read_sc;
	fns.phyts_enable_sc = phy_ac_tof_phyts_enable_sc;
	fns.phyts_pkt_detect = phy_ac_tof_phyts_pkt_detect;
	fns.phyts_mf = phy_ac_tof_phyts_mf;
	fns.phyts_loopback = phy_ac_tof_phyts_loopback;
	fns.phyts_set_txpwr = phy_ac_tof_phyts_set_txpwr;
	fns.phyts_get_sc_read_buf_sz = phy_ac_tof_phyts_get_sc_read_buf_sz;
#endif /* WL_PROXD_PHYTS */

	/* DEBUG */
#ifdef TOF_DBG
	fns.dbg = phy_ac_tof_dbg;
#endif
	fns.ctx = tofi;
	phy_ac_nvram_proxd_read(pi, tofi);
	phy_ac_nvram_proxd_read_6g(pi, tofi);
	phy_tof_register_impl(ti, &fns);

#ifdef WL_PROXD_SEQ
	/* Register the maximum required scratch buffer size for phy_ac_tof_seq_params() The max
	 * required buffer size depends on tofi->tof_seq_log2_n.
	 */
#ifdef TOF_SEQ_20_IN_80MHz
	phy_cache_register_reuse_size(pi->cachei, sizeof(cint32) *
		(1 << K_TOF_SEQ_LOG2_N_80MHZ_20));
#else
	phy_cache_register_reuse_size(pi->cachei, sizeof(cint32) *
		(1 << K_TOF_SEQ_LOG2_N_80MHZ));
#endif /* TOF_SEQ_20_IN_80MHz */
#endif /* WL_PROXD_SEQ */

	return tofi;

	/* error handling */
fail:
#ifdef WL_PROXD_PHYTS
	if (phyts_info != NULL) {
		phy_mfree(pi, phyts_info, sizeof(phy_ac_tof_phyts_info_t));
	}
#endif /* WL_PROXD_PHYTS */
	if (tofi != NULL) {
		phy_mfree(pi, tofi, alloc_size);
	}
	return NULL;
}

void
BCMATTACHFN(phy_ac_tof_unregister_impl)(phy_ac_tof_info_t *tofi)
{
	phy_info_t *pi = tofi->pi;
	phy_tof_info_t *ti = tofi->ti;
	uint32 alloc_size = tofi->alloc_size;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_tof_unregister_impl(ti);
#ifdef WL_PROXD_PHYTS
	if (tofi->phytsi != NULL) {
		phy_mfree(pi, tofi->phytsi, sizeof(phy_ac_tof_phyts_info_t));
	}
#endif /* WL_PROXD_PHYTS */
	phy_mfree(pi, tofi, alloc_size);
}

#ifdef WL_PROXD_SEQ
static int
WLBANDINITFN(phy_ac_tof_reset)(phy_type_tof_ctx_t *ctx)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;

	tofi->tof_setup_done = FALSE;
	tofi->tof_active = FALSE;
	tofi->tof_smth_forced = FALSE;
	tofi->tof_rfseq_bundle_offset = 0;
	tofi->tof_shm_ptr =
		(wlapi_bmac_read_shm(tofi->pi->sh->physhim, M_TOF_BLK_PTR(tofi->pi)) * 2);
	tofi->start_seq_time = TOF_DEFAULT_START_SEQ_TIME;
	tofi->delta_time_tx2rx = TOF_DEFAULT_DELTA_TIME_TX2RX;

	return BCME_OK;
}
#endif /* WL_PROXD_SEQ */

/* Inter-module data api */
bool
phy_ac_tof_is_active(phy_ac_tof_info_t *tofi)
{
	return tofi->tof_active;
}

bool
phy_ac_tof_forced_smth(phy_ac_tof_info_t *tofi)
{
	return tofi->tof_smth_forced;
}

#ifdef WL_PROXD_SEQ
void
phy_ac_tof_tbl_offset(phy_ac_tof_info_t *tofi, uint32 id, uint32 offset)
{
	/* To keep track of first available offset usable by tof procs */
	if ((id == ACPHY_TBL_ID_RFSEQBUNDLE) && ((offset & 0xf) > tofi->tof_rfseq_bundle_offset))
		tofi->tof_rfseq_bundle_offset = (uint16)(offset & 0xf);
	if ((id == ACPHY_TBL_ID_RFSEQBUNDLE) || (id == ACPHY_TBL_ID_SAMPLEPLAY))
		tofi->tof_setup_done = FALSE;
}

#undef TOF_TEST_TONE

static void wlc_phy_tof_conj_arr(cint32* pIn, int len)
{
	int i = 0;
	cint32* pTmp = pIn;
	for (i = 0; i < len; i++) {
		pTmp->q = -pTmp->q;
		pTmp++;
	}
}

static void wlc_phy_tof_fliplr(cint32* pIn, int len, bool conj)
{
	cint32 tmp = {0, 0};
	cint32* pTmps = pIn + 1;
	cint32* pTmpe = pIn + len - 1;
	while (pTmps < pTmpe) {
		tmp.i = pTmps->i;
		tmp.q = pTmps->q;
		pTmps->i = pTmpe->i;
		pTmps->q = pTmpe->q;
		pTmpe->i = tmp.i;
		pTmpe->q = tmp.q;
		pTmps++;
		pTmpe--;
	}
	if (conj) {
		wlc_phy_tof_conj_arr(pIn, len);
	}
}

static void phy_ac_tof_sc(phy_info_t *pi, bool setup, int sc_start, int sc_stop, uint16 cfg)
{
	uint16 phy_ctl;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	if (D11REV_GE(pi->sh->corerev, 50)) {
		phy_ctl = R_REG(pi->sh->osh, D11_SMP_CTRL(pi));
		phy_ctl &= ~((1<<0) | (1<<2));
		W_REG(pi->sh->osh, D11_SMP_CTRL(pi), phy_ctl);
	} else {
		phy_ctl = R_REG(pi->sh->osh, D11_PSM_PHY_CTL(pi)) & ~((1<<4) | (1<<5));
		W_REG(pi->sh->osh, D11_PSM_PHY_CTL(pi), phy_ctl);
	}

		MOD_PHYREG(pi, RxFeTesMmuxCtrl, samp_coll_core_sel, pi_ac->tofi->tof_core);
		MOD_PHYREG(pi, RxFeTesMmuxCtrl, rxfe_dbg_mux_sel, 4);

	WRITE_PHYREG(pi, AdcDataCollect, 0);

	if (setup) {
		acphy_set_sc_startptr(pi, (uint32)sc_start);
		acphy_set_sc_stopptr(pi, (uint32)sc_stop);
	}
	if (cfg) {
		if (D11REV_GE(pi->sh->corerev, 50)) {
			W_REG(pi->sh->osh, D11_SMP_CTRL(pi),
			(uint16)(phy_ctl | (1 << 0) | (1 << 2)));
		} else {
			W_REG(pi->sh->osh, D11_PSM_PHY_CTL(pi),
			(uint16)(phy_ctl | ((1 << 4) | (1 << 5))));
		}
		WRITE_PHYREG(pi, AdcDataCollect, cfg);
	}
}

static int phy_ac_tof_sc_read(phy_info_t *pi, bool iq, int n, cint32* pIn,
	int16 sc_ptr, int16 sc_base_ptr, int16* p_sc_start_ptr)
{
	uint32 dataL = 0;
	cint32* pEnd = pIn + n;
	int n_out = 0;
	int32* pOut = (int32*)pIn;
	int16 sc_end_ptr;

	if (sc_ptr <= 0) {
		/* Offset from sc_base_ptr */
		sc_ptr = (-sc_ptr >> 2);
		*p_sc_start_ptr = (sc_ptr << 2);
		sc_ptr = (sc_ptr << 2);
		sc_ptr += sc_base_ptr;
	} else {
		/* Actual address */
		*p_sc_start_ptr = sc_ptr;
	}

	sc_end_ptr = R_REG(pi->sh->osh, D11_SCP_CURPTR(pi));
	W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_PTR(pi), ((uint32)sc_ptr << 2));
	while ((pIn < pEnd) && (sc_ptr < sc_end_ptr)) {
		dataL = (uint32)R_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_DATA(pi));
		pIn->i = (int32)(dataL & 0xfff);
		pIn->q = (int32)((dataL >> 16) & 0xfff);
		pIn++;
		n_out++;
		sc_ptr++;
	}

	if (iq) {
		int32 datum;

		n = 2 * n_out;
		while (n-- > 0) {
			datum = *pOut;
			if (datum > 2047)
				datum -= 4096;
			*pOut++ = datum;
		}
	}
	return n_out;
}

static int
wlc_tof_rfseq_event_offset(phy_info_t *pi, uint16 event, uint16* rfseq_events)
{
	int i;

	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, (uint32)0, 16, (void*)rfseq_events);

	for (i = 0; i < 16; i++) {
		if (rfseq_events[i] == event) {
			break;
		}
	}
	return i;
}

static void phy_ac_tof_cfo(phy_info_t *pi, int cfo, int n, cint32* pIn)
{
	/* CFO Correction Function Placeholder */
}

static void phy_ac_tof_cross_corr_sum(const cint32* In, const int32 len, cint32* accum)
{
	int i = 0;
	cint32 mult = { 0, 0 };
	for (i = 1; i < len; i++) {
		math_cmplx_mult_cint32_cfixed((In + i), (In + i - 1), 0, &mult, TRUE);
		math_cmplx_add_cint32(&mult, accum, accum);
	}
}
static void phy_ac_tof_group_delay_mv(phy_info_t *pi, const cint32* In, const uint16 len,
		const uint16* band_length, const uint16* sc_idx_arr, const int32 th,
		int32* gd_m, uint32* gd_v, int32* delta_th)
{
	cint32 mult = {0, 0};
	int32 max = INT32_MIN, min = INT32_MAX;
	uint32 run_sum = 0, div_mask = (1 << 30), theta_err = 0;
	uint16 band_offset = 0, i, j, idx, num_s = 0;
	const size_t theta_size = sizeof(int32) * 256;
	int32 *theta = phy_malloc(pi, theta_size);

	bzero(theta, theta_size);

	*gd_m = 0;
	*gd_v = 0;
	*delta_th = 0;

	for (i = 0; i < len; i++) {
		num_s += band_length[i] - 1;
	}

	if (num_s != 0) {
		for (i = 0; i < len; i++) {
			const cint32* pTmp = (In + sc_idx_arr[band_offset]);
			for (j = 1; j < band_length[i]; j++) {
				math_cmplx_mult_cint32_cfixed((pTmp + j), (pTmp + j - 1),
					0, &mult, TRUE);
				idx = i*(band_length[i]-1) + j - 1;
				theta[idx] = -math_cordic_ptr(&mult);
				if (theta[idx] < min) {
					min = theta[idx];
				}
				if (theta[idx] > max) {
					max = theta[idx];
				}
				*gd_m += theta[idx];
			}
			band_offset += band_length[i];
		}
		(*delta_th) = th - max + min;
		(*gd_m) = (*gd_m) / num_s;
		for (i = 0; i < num_s; i++) {
			theta_err = theta[i] - (*gd_m);
			run_sum += theta_err*theta_err;
			if (run_sum & div_mask) {
				*gd_v += (run_sum/num_s);
				run_sum = 0;
			}
		}
		*gd_v += (run_sum/num_s);
	}
	phy_mfree(pi, theta, theta_size);
}

static int32 phy_ac_tof_group_delay(const cint32* In, const uint16 len, const uint16* band_length,
		const uint16* sc_idx_arr)
{
	uint16 band_offset = 0, i;
	cint32 accum = {0, 0};
	for (i = 0; i < len; i++) {
		uint16 idx = sc_idx_arr[band_offset];
		phy_ac_tof_cross_corr_sum((In+idx), band_length[i], &accum);
		band_offset += band_length[i];
	}
	return math_cordic_ptr(&accum);
}

static int32 phy_ac_tof_phase_est_theta(cint32* In, int32 gd_theta, int32 ph_offset,
	int32 len, bool gd_corr)
{
	int32 i = 0;
	cint32 mult = { 0, 0 }, accum = { 0, 0 };
	cint32 expj = { 0, 0 };
	cint32* tmp = &mult;
	for (i = 0; i < len; i++) {
		expj.i = math_cos_tbl(i*gd_theta + ph_offset);
		expj.q = math_sin_tbl(-(i*gd_theta + ph_offset));
		if (gd_corr) {
			tmp = In + i;
		}
		math_cmplx_mult_cint32_cfixed((In + i), &expj, K_TOF_TWDL_SFT, tmp, FALSE);
		math_cmplx_add_cint32(tmp, &accum, &accum);
	}
	return math_cordic_ptr(&accum);
}

static void phy_ac_tof_phase_corr_chan(uint16 len, const uint16* band_length,
	const uint16* sc_idx_arr, cint32* chan, cint32* mf_out)
{
	cint32 ph_corr = { 0, 0 };
	int32 i = 0, j = 0;
	uint16 band_offset;
	int32 gd_theta1, gd_theta2;
	int32 phi_off_c[len], phi_off_m[len];
	int32 ph_corr_th;

	gd_theta1 = phy_ac_tof_group_delay(chan, len, band_length, sc_idx_arr);
	band_offset = 0;
	for (i = 0; i < len; i++) {
		uint16 idx = sc_idx_arr[band_offset];
		phi_off_c[i] = phy_ac_tof_phase_est_theta((chan + idx), gd_theta1,
			(idx)*gd_theta1, band_length[i], TRUE);
		band_offset += band_length[i];
	}
	gd_theta2 = phy_ac_tof_group_delay(mf_out, len, band_length, sc_idx_arr);
	band_offset = 0;
	for (i = 0; i < len; i++) {
		uint16 idx = sc_idx_arr[band_offset];
		phi_off_m[i] = phy_ac_tof_phase_est_theta((mf_out + idx), gd_theta2,
			(idx)*gd_theta2, band_length[i], FALSE);
		band_offset += band_length[i];
	}

	/* Debug Prints */
	PHY_INFORM(("gd_theta1=[%d];\n", gd_theta1));
	for (i = 0; i < len; i++) {
		PHY_INFORM(("phi_off_c%d=[%d];\n", i, phi_off_c[i]));
	}
	PHY_INFORM(("gd_theta2=[%d];\n", gd_theta2));
	for (i = 0; i < len; i++) {
		PHY_INFORM(("phi_off_m%d=[%d];\n", i, phi_off_m[i]));
	}
	/* Debug Prints */

	band_offset = 0;
	for (i = 0; i < len; i++) {
		for (j = 0; j < band_length[i]; j++) {
			int idx = sc_idx_arr[band_offset + j];
			ph_corr_th = phi_off_m[i] - phi_off_c[i] + idx*gd_theta2;
			ph_corr.i = math_cos_tbl(ph_corr_th);
			ph_corr.q = math_sin_tbl(ph_corr_th);
			math_cmplx_mult_cint32_cfixed((chan + idx), &ph_corr, K_TOF_TWDL_SFT,
				(chan + idx), FALSE);
		}
		band_offset += band_length[i];
	}
}

static void phy_ac_tof_calc_snr_bpsk(const cint32* mf_out, const cint32* chan,
	const uint16* sc_idx_arr, const uint16 len, wl_proxd_bitflips_t *bit_flips,
	wl_proxd_snr_t *snr)
{
	uint32 sig_pwr = 0, chan_pwr = 0, noise_pwr = 0, final_sc, sig_sc, chan_sc;
	cint32 sig_val, chan_out;
	int32 i = 0, k = 0, noise_val;
	cint32 sig_sc_cmplx = { 0, 0};
	cint32 chan_sc_cmplx = { 0, 0};

	*bit_flips = 0;
	/*
	* We use matched filter output instead of rx'ed signal to calculate noise as
	* MF output is sign inversed in some bit positions where LTF is (-1).
	* If rx'ed sig is given by R = H*T + N, then N = R - H*T.
	* Instead we calculate N*T = R*T - H (T*T = 1, as T = +/- 1).
	* Here R*T is MF out and H is channel estimate.
	* N*T = +/- N depending on T = +/-1, we don't care about sign as we need to calculate
	* N*T*N*T = N*N which is always +ve.`
	*
	* All the tricks in this function can be used only because transmitted signal is BPSK
	* (+1, -1).
	*/
	math_cmplx_power_cint32_arr(mf_out, sc_idx_arr, len, &sig_pwr);
	sig_pwr = sqrt_int(sig_pwr);
	math_cmplx_power_cint32_arr(chan, sc_idx_arr, len, &chan_pwr);
	chan_pwr = sqrt_int(chan_pwr);
	/*
	* The scaling factors for normalization have been chosen this way to minimize
	* the impact of dividing by a large number and getting a 0, as data type is
	* integer type.
	*/
	if ((sig_pwr == 0) || (chan_pwr == 0)) {
		PHY_ERROR(("Error : chan_pwr = %d, sig_pwr = %d\n", chan_pwr, sig_pwr));
		ASSERT(0);
		*snr = 1;
		*bit_flips = len;
		return;
	}
	if (sig_pwr > chan_pwr) {
		sig_sc = (1 << K_TOF_SNR_FP_PREC) * 1;
		chan_sc = (sig_sc*sig_pwr) / chan_pwr;
		final_sc = sig_pwr;
	}
	else {
		chan_sc = (1 << K_TOF_SNR_FP_PREC) * 1;
		sig_sc = (chan_sc*chan_pwr) / sig_pwr;
		final_sc = chan_pwr;
	}

	/* Debug Prints */
	PHY_INFORM(("sig_sc = [%d];\n", sig_sc));
	PHY_INFORM(("chan_sc = [%d];\n", chan_sc));
	PHY_INFORM(("final_sc = [%d];\n", final_sc));
	PHY_INFORM(("noise_val = [ "));
	/* Debug Prints */

	sig_sc_cmplx.i = sig_sc;  /* {q, i} = {0, sig_sc} */
	chan_sc_cmplx.i = chan_sc; /* {q, i} = {0, chan_sc} */

	for (k = 0; k < len; k++) {
		i = sc_idx_arr[k];
		math_cmplx_mult_cint32_cfixed((mf_out + i), &sig_sc_cmplx, 0, &sig_val, FALSE);
		math_cmplx_mult_cint32_cfixed((chan + i), &chan_sc_cmplx, 0, &chan_out, FALSE);
		/*
		* We are using BPSK for LTF (+1, -1) so we are concerned with in phase noise only.
		* noise_val being calculated here can be +/- of actual noise but we don't care as
		* we want only noise power.
		*/
		noise_val = sig_val.i - chan_out.i;

		/* Debug Prints */
		PHY_INFORM(("%d, ", noise_val));
		/* Debug Prints */

		noise_pwr += ((noise_val*noise_val) / (1 << (2 * K_TOF_SNR_FP_PREC)));
		/*
		* Calculate bit flips
		* Again instead of calculating R*H'/|H|^2 = T + N*H'/|H|^2
		* we calculate R*T*H'/|H|^2 = 1 + N*T*H'/|H|^2, so the slicer decision becomes
		* easier
		*/
		math_cmplx_mult_cint32_cfixed((mf_out + i), (chan + i), 0, &sig_val, TRUE);
		*bit_flips += ((sig_val.i < 0) ? 1 : 0);
	}

	/* Debug Prints */
	PHY_INFORM(("];\n"));
	PHY_INFORM(("noise_pwr=[%d];\n", noise_pwr));
	/* Debug Prints */

	*snr = (wl_proxd_snr_t)(noise_pwr == 0) ? 0xffff : (len*((final_sc*final_sc) / noise_pwr));
}

static int phy_ac_tof_demod_snr(phy_info_t *pi, void *In, void *chan_in, uint8 bw_factor,
	wl_proxd_bitflips_t *bit_flips, wl_proxd_snr_t *snr)
{
	cint32* pIn = (cint32*)In;
	cint32* chan;
	cint32* chan_out;
	const uint16 *sc_idx_arr = NULL, *band_length = NULL;
	uint16 num_bl = 0;
	uint32 i = 0, j = 0;
	uint32 alloc_size = K_TOF_HALF_CHAN_LENGTH_2X_OS_80M * sizeof(cint32*);
	int NUM_LEGACY_NZ_SC = 0;

	if ((chan_out = (cint32*) phy_malloc(pi, alloc_size)) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		return BCME_NOMEM;
	}

	switch (bw_factor) {
	case 2:
		chan = chan_out;
		j = 0;
		for (i = 0; i < (K_TOF_HALF_CHAN_LENGTH_2X_OS_80M >> 1); i++) {
			chan[j].i = (((cint32 *)chan_in) + 2 * i)->i;
			chan[j].q = (((cint32 *)chan_in) + 2 * i)->q;
			j++;
			if (j == ((K_TOF_HALF_CHAN_LENGTH_80M >> 1) + 1)) {
				j += (K_TOF_HALF_CHAN_LENGTH_2X_OS_80M >> 1);
			}
		}
		num_bl = K_TOF_NUM_LEGACY_BL_80M;
		band_length = band_length_80MHz;
		sc_idx_arr = nonzero_sc_idx_legacy_80MHZ;
		break;
	case 0:
	default:
		num_bl = K_TOF_NUM_LEGACY_BL_20M;
		band_length = band_length_20MHz;
		sc_idx_arr = nonzero_sc_idx_legacy_20MHZ;
		chan = (cint32*)chan_in;
	}

	phy_ac_tof_phase_corr_chan(num_bl, band_length, sc_idx_arr, chan, pIn);

	for (i = 0; i < num_bl; i++) {
		NUM_LEGACY_NZ_SC += band_length[i];
	}
	phy_ac_tof_calc_snr_bpsk(pIn, chan, sc_idx_arr, NUM_LEGACY_NZ_SC, bit_flips, snr);

	if (chan_out != NULL) {
		phy_mfree(pi, chan_out, alloc_size);
	}

	return BCME_OK;
}

static int phy_ac_tof_mf(phy_info_t *pi, int n, cint32* pIn, bool seq, bool isTx,
	int a, int b, int cfo, int s1, int k2, int s2,
	uint16 bitflip_thresh, uint16 snr_thresh, const uint8 smooth_win_en)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	int i, k, ret_val = BCME_OK;
	cint32 *pTmp;
	int32 tmp;
	int nF = tofi->tof_seq_spb_len;
	const uint32* pF = (seq || isTx) ? tofi->tof_seq_spb_tx : tofi->tof_seq_spb_rx;
	const uint16 smooth_win[K_TOF_SEQ_FFT_20MHZ] = {
		65518, 65371, 65078, 64641, 64061, 63341, 62484, 61494,
		60377, 59136, 57777, 56307, 54733, 53061, 51298, 49453,
		47534, 45549, 43506, 41416, 39285, 37124, 34941, 32746,
		30547, 28353, 26173, 24015, 21887, 19798, 17755, 15764,
		15764, 17755, 19798, 21887, 24015, 26173, 28353, 30547,
		32746, 34941, 37124, 39285, 41416, 43506, 45549, 47534,
		49453, 51298, 53061, 54733, 56307, 57777, 59136, 60377,
		61494, 62484, 63341, 64061, 64641, 65078, 65371, 65518};
	int idx = 0;
	cint32 rot = {-1, 1};

	uint16 bw_factor = 0;
	const int num_max_cores = PHYCORENUM((pi)->pubpi->phy_corenum);

	if (CHSPEC_IS80(pi->radio_chanspec)) {
		bw_factor = 2;
	} else if (CHSPEC_IS20(pi->radio_chanspec)) {
		bw_factor = 0;
	} else {
		PHY_ERROR(("ERROR: Bandwidth other than 20 and 80 is not supported.\n"));
	}

	/* Debug Prints */
	if ((seq) && (isTx)) {
		PHY_INFORM(("SPB.... : \n"));
	} else if ((!seq) && (isTx)) {
		PHY_INFORM(("Tx... : \n"));
	} else {
		PHY_INFORM(("Matched filter... : \n"));
	}
	for (i = 0; i < 2 * nF; i++) {
		PHY_INFORM(("0x%08x \n", *(pF + i)));
	}
	/* Debug Prints */

	pTmp = pIn;
	for (i = 0; i < n; i++) {
		if (seq) {
			pTmp->q = 0;
			pTmp->i = (int32)s1;
		} else {
			pTmp->q = (pTmp->q + ((pTmp->i*(int32)a) >> 10)) << s1;
			pTmp->i = (pTmp->i + ((pTmp->i*(int32)b) >> 10)) << s1;
		}
		pTmp++;
	}

	if (!seq) {
		if (cfo) {
			phy_ac_tof_cfo(pi, cfo, n, pIn);
		}
#ifdef TOF_SEQ_20MHz_BW_512IFFT
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			pTmp = pIn;
			for (i = 0; i < K_TOF_SEQ_FFT_20MHZ; i++) {
				/* No down sampling for 43012 */
				pTmp[i] = pIn[i * 2];
			}

			memset(&pIn[K_TOF_SEQ_FFT_20MHZ], 0,
			(K_TOF_SEQ_FFT_20MHZ * sizeof(cint32)));
			n = K_TOF_SEQ_FFT_20MHZ;
		}
#endif /* TOF_SEQ_20MHz_BW_512IFFT */

		/* Undo IQ-SWAP for 43012 */
		wlapi_fft(pi->sh->physhim, n, (void*)pIn, (void*)pIn, 2);
#ifdef TOF_DBG_SEQ
		PRINT_SAMP("fft_out", seq, n, pIn);
#endif
		if (pi_ac->tofi->flag_sec_2_0) {
			pTmp = pIn;
			for (i = 0; i < n; i++) {
				math_cmplx_mult_cint32_cfixed(pTmp, &rot,
				0, pTmp, TRUE);
				pTmp->q = -pTmp->q;
				pTmp++;
			}
		}
#ifdef TOF_DBG_SEQ
		PRINT_SAMP("derot_fft_out", seq, n, pIn);
#endif
	}

	pTmp = pIn;
	for (k = 0; k < 2; k++) {
		for (i = 0; i < nF; i++) {
			uint32 f;
			int j;

			f = *pF++;
			for (j = 0; j < 32; j += 4) {
				if (f & K_TOF_FILT_NON_ZERO_MASK) {
					if (!(f & K_TOF_FILT_1_MASK)) {
						tmp = pTmp->q;
						if (pi_ac->tofi->flag_sec_2_0) {
							pTmp->q = pTmp->i;
							pTmp->i = -tmp;
						} else {
							pTmp->q = -pTmp->i;
							pTmp->i = tmp;
						}
					}
					if (f & K_TOF_FILT_NEG_MASK) {
						pTmp->i = -pTmp->i;
						pTmp->q = -pTmp->q;
					}
				} else {
					pTmp->i = 0;
					pTmp->q = 0;
				}
				if (pi_ac->tofi->flag_sec_2_0) {
					if (seq) {
						math_cmplx_mult_cint32_cfixed(pTmp,
						&rot, 0, pTmp, FALSE);
						pTmp->q = -pTmp->q;
					}
				}
				pTmp++;
				f = f >> 4;
			}
		}
		if (!k) {
			for (i = 0; i < (n - 2 * 8 * nF); i++) {
				pTmp->i = 0;
				pTmp->q = 0;
				pTmp++;
			}
		}
	}
#ifdef TOF_DBG_SEQ
	PRINT_SAMP("mfout0", seq, n, pIn);
#endif

	if (!(seq || isTx)) {
		cint32* chan = (cint32*)tofi->chan;
		uint16	print_chan_len = (K_TOF_CHAN_LENGTH_20M << bw_factor);

		chan = (chan + num_max_cores*print_chan_len);
		ret_val = phy_ac_tof_demod_snr(pi, (void*)pIn, (void*)chan, bw_factor,
		&(tofi->bitflips), &(tofi->snr));

		PHY_ERROR(("SNR = %d, Bit Flips = %d\n", tofi->snr,  tofi->bitflips));
		if ((ret_val != BCME_OK) || (tofi->bitflips > bitflip_thresh) ||
		(tofi->snr < snr_thresh)) {
			if (ret_val != BCME_OK) {
				tofi->tof_phy_error |= WL_PROXD_PHY_ERR_LOW_CONFIDENCE;
			}
			if (tofi->bitflips > bitflip_thresh) {
				tofi->tof_phy_error |= WL_PROXD_PHY_ERR_BITFLIP;
			}
			if (tofi->snr < snr_thresh) {
				tofi->tof_phy_error |= WL_PROXD_PHY_ERR_SNR;
			}
			PHY_ERROR(("SNR_Thres = %d,  BF_Thres = %d\n", snr_thresh, bitflip_thresh));
		}
	}

	if ((!seq) && (smooth_win_en == 1) && CHSPEC_IS20(pi->radio_chanspec)) {
		pTmp = pIn;
		for (k = 0; k < 2; k++) {
			for (i = 0; i < 8*nF; i++) {
				idx = 8*nF*k + i;
				pTmp->i *= smooth_win[idx];
				pTmp->q *= smooth_win[idx];
				pTmp->i >>= 16;
				pTmp->q >>= 16;
				pTmp++;
			}
			if (!k) {
				pTmp += (n - 2*8*nF);
			}
		}
	}

#ifdef TOF_SEQ_20MHz_BW_512IFFT
	if (!seq) {
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			pTmp = pIn;
			bcopy(pTmp, pIn, (2 * 32 * sizeof(uint32)));
			bcopy((pTmp + 32), (pIn + 480), (2 * 32 * sizeof(uint32)));
			memset(&pIn[32], 0, (448 * sizeof(cint32)));
			n = K_TOF_SEQ_IFFT_20MHZ;
		}
	}
#endif /* TOF_SEQ_20MHz_BW_512IFFT */

	wlapi_fft(pi->sh->physhim, n, (void*)pIn, (void*)pIn, 2);
	if (pi_ac->tofi->flag_sec_2_0) {
		if (!seq) {
			wlc_phy_tof_fliplr(pIn, n, FALSE);
		}
	}
	if (s2) {
		int32 *pTmpIQ, m2 = (int32)(1 << (s2 - 1));
		pTmpIQ = (int32*)pIn;
		for (i = 0; i < 2 * n; i++) {
			tmp = ((k2*(*pTmpIQ) + m2) >> s2);
			*pTmpIQ++ = tmp;
		}
	}
	return ret_val;
}

static void
wlc_tof_seq_write_shm_acphy(phy_info_t *pi, int len, uint16 offset, uint16* p)
{
	uint16 p_shm = pi->u.pi_acphy->tofi->tof_shm_ptr;

	while (len-- > 0) {
		wlapi_bmac_write_shm(pi->sh->physhim, (p_shm + offset), *p);
		p++;
		offset += 2;
	}
}

static void
phy_ac_tof_adjust_rx_tx_timing(phy_info_t *pi, bool tx)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;

	int16 trig_dly_offset;
	uint16 ofdm_mult, ofdm_mult_remainder, ofdm_sym_len_10us;

	ofdm_sym_len_10us = (((1 << K_TOF_SEQ_LOG2_N_20MHZ) *
		TOF_US_TO_TEN_US)/tofi->tof_sc_FS);
	trig_dly_offset = (tofi->delta_time_tx2rx -
		TOF_DEFAULT_DELTA_TIME_TX2RX);
	ofdm_mult = (int)((tofi->delta_time_tx2rx * TOF_US_TO_TEN_US)/
		ofdm_sym_len_10us);
	ofdm_mult_remainder = (int) ((tofi->delta_time_tx2rx *
		TOF_US_TO_TEN_US) - (((tofi->delta_time_tx2rx *
		TOF_US_TO_TEN_US)/ofdm_sym_len_10us)*ofdm_sym_len_10us));
#ifdef TOF_DBG_SEQ
	PHY_ERROR(("%s: ofdm_mult = %d, ofdm_mult_remainder = %d\n",
		__FUNCTION__, ofdm_mult, ofdm_mult_remainder));
#endif
	if (ofdm_mult_remainder >= (ofdm_sym_len_10us/2)) {
		ofdm_mult++;
	}
	if (tx) {
		tofi->tof_ucode_dlys_us[1][2] += trig_dly_offset;
		tofi->tof_ucode_dlys_us[1][4] =
			(tofi->tof_ucode_dlys_us[1][3] +
			(1 << K_TOF_SEQ_LOG2_N_20MHZ) * ofdm_mult);
#ifdef TOF_DBG_SEQ
		PHY_ERROR(("%s: trig_dly_offset = %d,",
			__FUNCTION__, trig_dly_offset));
		PHY_ERROR(("Adj_Delay = %d\n",
			tofi->tof_ucode_dlys_us[1][2]));
#endif
	} else {
		tofi->tof_ucode_dlys_us[0][1] += trig_dly_offset;
		tofi->tof_ucode_dlys_us[0][3] =
			(tofi->tof_ucode_dlys_us[0][4] +
			(1 << K_TOF_SEQ_LOG2_N_20MHZ) * ofdm_mult);
#ifdef TOF_DBG_SEQ
		PHY_ERROR(("%s: trig_dly_offset = %d,",
			__FUNCTION__, trig_dly_offset));
		PHY_ERROR(("Adj_Delay = %d\n",
			tofi->tof_ucode_dlys_us[0][1]));
#endif
	}
}

static void
phy_ac_tof_trig_dly_setup_acphy(phy_info_t *pi, bool enter, bool tx)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint offset;
	uint16 shm_val, tof_trig_delay;
	int16 trig_dly_offset;

	if (pi_ac->tofi->flag_sec_2_0) {
		PHY_ERROR(("%s: SECURE_RANGING 2.0: Enter = %d\n",
			__FUNCTION__, enter));
		if ((ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_1(pi) ||
			ACMINORREV_4(pi))) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
			if (enter) {
				trig_dly_offset = ((pi_ac->tofi->start_seq_time -
					TOF_DEFAULT_START_SEQ_TIME) * TOF_GPT_TIMER_RES_MULT);
				offset = pi_ac->tofi->tof_shm_ptr + M_TOF_FLAGS_OFFSET(pi);
				shm_val = wlapi_bmac_read_shm(pi->sh->physhim, offset);
				if (tx) {
					shm_val |= K_TOF_IS_TARGET_FLAG;
					tof_trig_delay = K_TOF_TARGET_TRIG_DLY;
				} else {
					tof_trig_delay = K_TOF_INI_TRIG_DLY;
					shm_val &= ~K_TOF_IS_TARGET_FLAG;
				}
				tof_trig_delay += trig_dly_offset;
				wlapi_bmac_write_shm(pi->sh->physhim, offset, shm_val);
				offset = pi_ac->tofi->tof_shm_ptr + M_TOF_TRIG_DLY;
				PHY_ERROR(("%s: tof_trig_delay = %d\n",
					__FUNCTION__, tof_trig_delay));
				wlapi_bmac_write_shm(pi->sh->physhim, offset, tof_trig_delay);
				phy_ac_tof_adjust_rx_tx_timing(pi, tx);
			} else {
				offset = pi_ac->tofi->tof_shm_ptr + M_TOF_TRIG_DLY;
				wlapi_bmac_write_shm(pi->sh->physhim, offset, 0);
			}
		}
	}
}

static void
phy_ac_tof_seq_upd_dly(phy_type_tof_ctx_t *ctx, bool tx, uint8 core, bool mac_suspend)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	uint8 stall_val;
	int i;
	uint16 *pSrc;
	uint16 shm[18];
	uint16 wrds_per_us, rfseq_trigger;
	bool  suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);

	if (mac_suspend) {
		if (!suspend) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		}
		phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
		stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
		ACPHY_DISABLE_STALL(pi);
	}

	/* Setup delays and triggers */
	wrds_per_us = tofi->tof_sc_FS;
	pSrc = shm;
	rfseq_trigger = READ_PHYREG(pi, RfseqTrigger) &
		ACPHY_RfseqTrigger_en_pkt_proc_dcc_ctrl_MASK(0);
	for (i = 0; i < 3; i++) {
		*pSrc++ = tofi->tof_ucode_dlys_us[(tx ? 1 : 0)][i] * wrds_per_us;
		if (i ^ tx)
			*pSrc = rfseq_trigger | ACPHY_RfseqTrigger_ocl_shut_off_MASK(0);
		else
			*pSrc = rfseq_trigger | ACPHY_RfseqTrigger_ocl_reset2rx_MASK(0);
		pSrc++;
	}
	shm[K_TOF_SEQ_SHM_DLY_LEN - 1] = rfseq_trigger; /* Restore value */
	shm[K_TOF_SEQ_SHM_DLY_LEN] = ACPHY_PhyStatsGainInfo0(0) + 0x200 * core;

	wlc_tof_seq_write_shm_acphy(pi,
		(K_TOF_SEQ_SHM_DLY_LEN + 1),
		K_TOF_SEQ_SHM_DLY_OFFSET,
		shm);
	if (mac_suspend) {
		ACPHY_ENABLE_STALL(pi, stall_val);
		phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
		if (!suspend) {
			wlapi_enable_mac(pi->sh->physhim);
		}
	}
}

static void phy_ac_tof_fill_write_k_tof_seq_ucode_regs(phy_info_t *pi)
{
	int i = 0;
	uint16 k_tof_seq_ucode_regs[K_TOF_SEQ_SHM_SETUP_REGS_LEN] = { 0, };

	k_tof_seq_ucode_regs[i++] = ACPHY_RxControl(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_TableID(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_TableOffset(0);
	k_tof_seq_ucode_regs[i++] = (ACPHY_TableDataWide(0) | (7 << 12));
	k_tof_seq_ucode_regs[i++] = ACPHY_TableID(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_TableOffset(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_TableDataLo(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_TableOffset(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_TableDataLo(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_RfseqMode(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_sampleCmd(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_RfseqMode(0);
	if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
		k_tof_seq_ucode_regs[i++] = ACPHY_AdcDataCollect(0);
	} else {
		k_tof_seq_ucode_regs[i++] = ACPHY_SlnaControl(pi->pubpi->phy_rev);
	}
	k_tof_seq_ucode_regs[i++] = ACPHY_AdcDataCollect(0);
	k_tof_seq_ucode_regs[i++] = ACPHY_RxControl(0);

	wlc_tof_seq_write_shm_acphy(pi,
	                            K_TOF_SEQ_SHM_SETUP_REGS_LEN,
	                            K_TOF_SEQ_SHM_SETUP_REGS_OFFSET,
	                            (uint16*)k_tof_seq_ucode_regs);

}

static void phy_ac_tof_fill_write_shm(phy_info_t *pi, uint16 *shm, uint8 core, uint16 mask)
{
	uint16 rfseq_mode, rfseq_offset, rx_ctrl, tof_rfseq_event;

	/* Set rx gain during loopback -- cant use rf bundles due to hw bug in 4350 */
	if (TINY_RADIO(pi)) {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			shm[0] = K_TOF_SEQ_RX_LOOPBACK_GAIN_TINY_2G;
		} else {
			shm[0] = K_TOF_SEQ_RX_LOOPBACK_GAIN_TINY;
		}
	} else {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
				shm[0] = K_TOF_SEQ_RX_LOOPBACK_GAIN_2G_4360;
			} else if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
				if (IS_4364_3x3(pi)) {
					shm[0] = K_TOF_SEQ_RX_LOOPBACK_GAIN_2G_4364;
				} else {
					shm[0] = K_TOF_SEQ_RX_LOOPBACK_GAIN_2G_43602;
				}
			} else {
				shm[0] = K_TOF_SEQ_RX_LOOPBACK_GAIN_2G;
			}
		} else {
			shm[0] = K_TOF_SEQ_RX_LOOPBACK_GAIN;
		}
	}
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
		(K_TOF_SEQ_RFSEQ_GAIN_BASE + core * 0x10
		+ K_TOF_SEQ_RFSEQ_LOOPBACK_GAIN_OFFSET),
		16, (void*)&shm[0]);

	/* Setup shm which tells ucode sequence of phy reg writes */
	/* before/after triggering sequence */
	rx_ctrl = READ_PHYREG(pi, RxControl);
	rfseq_mode = (READ_PHYREG(pi, RfseqMode) &
		~(ACPHY_RfseqMode_CoreActv_override_MASK(0) |
		ACPHY_RfseqMode_Trigger_override_MASK(0)));
	rfseq_offset = wlc_tof_rfseq_event_offset(pi, K_TOF_RFSEQ_TX_GAIN_EVENT, shm);
	rfseq_offset += 1;
	tof_rfseq_event = shm[rfseq_offset];

	phy_ac_tof_fill_write_k_tof_seq_ucode_regs(pi);

	bzero((void *)shm, sizeof(uint16)* K_TOF_SHM_ARR_LENGTH);
	shm[0] = rx_ctrl | ACPHY_RxControl_dbgpktprocReset_MASK(0); /* first setup */
	shm[1] = ACPHY_TBL_ID_RFSEQBUNDLE;
	if (TINY_RADIO(pi))
	  shm[2] = K_TOF_SEQ_TINY_RX_FEM_GAIN_OFFSET;
	else
	  shm[2] = K_TOF_SEQ_RX_FEM_GAIN_OFFSET;
	if (CHSPEC_IS2G(pi->radio_chanspec))
		shm[3] = k_tof_seq_fem_gains_2g[0] | mask;
	else
		shm[3] = k_tof_seq_fem_gains[0] | mask;
	shm[6] = ACPHY_TBL_ID_RFSEQ; /* first restore */
	shm[7] = K_TOF_SEQ_RFSEQ_GAIN_BASE + core*0x10 + K_TOF_SEQ_RFSEQ_RX_GAIN_OFFSET;
#ifdef TOF_DBG_SEQ
	if (TINY_RADIO(pi))
	  shm[8] = K_TOF_SEQ_RX_GAIN_TINY;
	else
	  shm[8] = K_TOF_SEQ_RX_GAIN;
#endif
	shm[9] = rfseq_offset;
	shm[10] = K_TOF_RFSEQ_END_EVENT;
	shm[11] = (rfseq_mode | ACPHY_RfseqMode_CoreActv_override_MASK(0));
	shm[12] = ACPHY_sampleCmd_start_MASK(0);
	shm[13] = (rfseq_mode |
	           ACPHY_RfseqMode_Trigger_override_MASK(0) |
	           ACPHY_RfseqMode_CoreActv_override_MASK(0));
	if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
		shm[14] = ACPHY_AdcDataCollect_adcDataCollectEn_MASK(0);
	}
	else {
		PHY_ERROR(("Slna Core = 0x%x\n",
			((~core) & 3) << ACPHY_SlnaControl_SlnaCore_SHIFT(0)));
		shm[14] = ((~core) & 3) << ACPHY_SlnaControl_SlnaCore_SHIFT(0);
	}
	shm[15] = ACPHY_AdcDataCollect_adcDataCollectEn_MASK(0); /* last setup */
	wlc_tof_seq_write_shm_acphy(pi,
	                            K_TOF_SEQ_SHM_SETUP_VALS_LEN,
	                            K_TOF_SEQ_SHM_SETUP_VALS_OFFSET,
	                            shm);
	shm[10] = tof_rfseq_event;
	shm[11] = rfseq_mode;
	shm[12] = ACPHY_sampleCmd_stop_MASK(0);
	shm[13] = rfseq_mode;
	if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
		shm[14] = 0;
	}
	else {
		shm[14] = READ_PHYREG(pi, SlnaControl);
	}
	shm[15] = 0;
	shm[16] = rx_ctrl; /* last restore */
	wlc_tof_seq_write_shm_acphy(pi,
	                            K_TOF_SET_SHM_RESTR_VALS_LEN,
	                            K_TOF_SET_SHM_RESTR_VALS_OFFSET,
	                            &shm[9]);
}

static void
phy_ac_tof_setup_rf_bundle(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	uint32 read_val[2];
	uint8 afe_iqadc_flash_only, afe_iqadc_reset_ov_det, afe_iqadc_clamp_en;
	uint8 afe_iqadc_rx_div4_en, afe_iqadc_adc_bias, afe_ctrl_flash17lvl;
	uint8 afe_iqadc_flashhspd, afe_iqadc_pwrup, afe_iqadc_mode;

	if (CHSPEC_IS20(pi->radio_chanspec)) {
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQEXT, 1,
				TOF_RFSEQEXT_ADC_20MHz_OFFS, TOF_RFSEQEXT_SIZE, &read_val);
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQEXT, 1,
				TOF_RFSEQEXT_ADC_40MHz_OFFS, TOF_RFSEQEXT_SIZE, &read_val);
	} else {
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQEXT, 1,
				TOF_RFSEQEXT_ADC_80MHz_OFFS, TOF_RFSEQEXT_SIZE, &read_val);
	}
	afe_iqadc_flash_only = ((read_val[0] >> TOF_RFSEQ_AFE_IQADC_FLASH_ONLY_SHIFT)
			& TOF_RFSEQ_AFE_IQADC_FLASH_ONLY_SZ);
	afe_iqadc_reset_ov_det = ((read_val[0] >> TOF_RFSEQ_AFE_RESET_OV_DET_SHIFT)
			& TOF_RFSEQ_AFE_RESET_OV_DET_SZ);
	afe_iqadc_clamp_en = ((read_val[0] >> TOF_RFSEQ_AFE_IQADC_CLAMP_EN_SHIFT)
			& TOF_RFSEQ_AFE_IQADC_CLAMP_EN_SZ);
	afe_iqadc_rx_div4_en = ((read_val[0] >> TOF_RFSEQ_AFE_IQADC_RX_DIV4_EN_SHIFT)
			& TOF_RFSEQ_AFE_IQADC_RX_DIV4_EN_SZ);
	afe_iqadc_adc_bias = ((read_val[0] >> TOF_RFSEQ_AFE_IQADC_ADC_BIAS_SHIFT)
			& TOF_RFSEQ_AFE_IQADC_ADC_BIAS_SZ);
	afe_ctrl_flash17lvl = ((read_val[0] >> TOF_RFSEQ_AFE_CTRL_FLASH17LVL_SHIFT)
			& TOF_RFSEQ_AFE_CTRL_FLASH17LVL_SZ);
	afe_iqadc_flashhspd = ((read_val[0] >> TOF_RFSEQ_AFE_IQADC_FLASHHSPD_SHIFT)
			& TOF_RFSEQ_AFE_IQADC_FLASHHSPD_SZ);
	afe_iqadc_pwrup = ((read_val[0] >> TOF_RFSEQ_AFE_IQADC_PWRUP_SHIFT)
			& TOF_RFSEQ_AFE_IQADC_PWRUP_SZ);
	afe_iqadc_mode = ((read_val[0]) & TOF_RFSEQ_AFE_IQADC_MODE_SZ);

	PHY_INFORM(("%s: Readval = 0x%x\n", __FUNCTION__, read_val[0]));
	PHY_INFORM(("RFSeq offset 38 Before = 0x%04x%04x%04x\n",
			tofi->bundle_offs_38[2], tofi->bundle_offs_38[1], tofi->bundle_offs_38[0]));
	PHY_INFORM(("RFSeq offset 39 Before = 0x%04x%04x%04x\n",
			tofi->bundle_offs_39[2], tofi->bundle_offs_39[1], tofi->bundle_offs_39[0]));
	PHY_INFORM(("afe_iqadc_flash_only = 0x%x\n", afe_iqadc_flash_only));
	PHY_INFORM(("afe_iqadc_reset_ov_det = 0x%x\n", afe_iqadc_reset_ov_det));
	PHY_INFORM(("afe_iqadc_clamp_en = 0x%x\n", afe_iqadc_clamp_en));
	PHY_INFORM(("afe_iqadc_rx_div4_en = 0x%x\n", afe_iqadc_rx_div4_en));
	PHY_INFORM(("afe_iqadc_adc_bias = 0x%x\n", afe_iqadc_adc_bias));
	PHY_INFORM(("afe_ctrl_flash17lvl = 0x%x\n", afe_ctrl_flash17lvl));
	PHY_INFORM(("afe_iqadc_flashhspd = 0x%x\n", afe_iqadc_flashhspd));
	PHY_INFORM(("afe_iqadc_pwrup = 0x%x\n", afe_iqadc_pwrup));
	PHY_INFORM(("afe_iqadc_mode = 0x%x\n", afe_iqadc_mode));

	tofi->bundle_offs_38[0] &= ~TOF_RFBNDL_ADC_MASK_LB;
	tofi->bundle_offs_38[1] &= ~TOF_RFBNDL_ADC_MASK_HB;

	tofi->bundle_offs_39[0] &= ~TOF_RFBNDL_ADC_MASK_LB;
	tofi->bundle_offs_39[1] &= ~TOF_RFBNDL_ADC_MASK_HB;

	PHY_INFORM(("RFSeq offset 38 After Mask = 0x%04x%04x%04x\n", tofi->bundle_offs_38[2],
			tofi->bundle_offs_38[1], tofi->bundle_offs_38[0]));
	PHY_INFORM(("RFSeq offset 39 After Mask = 0x%04x%04x%04x\n", tofi->bundle_offs_39[2],
			tofi->bundle_offs_39[1], tofi->bundle_offs_39[0]));

	tofi->bundle_offs_38[0] |=
		((afe_iqadc_reset_ov_det << TOF_RFBNDL_AFE_RESET_OV_DET_SHIFT) |
		(afe_iqadc_clamp_en << TOF_RFBNDL_AFE_IQADC_CLAMP_EN_SHIFT) |
		(afe_iqadc_adc_bias << TOF_RFBNDL_AFE_IQADC_ADC_BIAS_SHIFT) |
		(afe_iqadc_pwrup << TOF_RFBNDL_AFE_IQADC_PWRUP_SHIFT) |
		((afe_iqadc_mode & TOF_RFBNDL_AFE_IQADC_MODE_LB_MASK) <<
		 TOF_RFBNDL_AFE_IQADC_MODE_LB_SHIFT));

	tofi->bundle_offs_38[1] |=
		((afe_iqadc_rx_div4_en << TOF_RFBNDL_AFE_IQADC_RX_DIV4_EN_SHIFT) |
		(afe_ctrl_flash17lvl << TOF_RFBNDL_AFE_CTRL_FLASH17LVL_SHIFT) |
		(afe_iqadc_flashhspd << TOF_RFBNDL_AFE_IQADC_FLASHHSPD_SHIFT) |
		(((afe_iqadc_mode &  TOF_RFBNDL_AFE_IQADC_MODE_HB_MASK) >>
		TOF_RFBNDL_AFE_IQADC_MODE_HB_SHIFT)) |
		(afe_iqadc_flash_only << TOF_RFBNDL_AFE_IQADC_FLASH_ONLY_SHIFT));

	tofi->bundle_offs_39[0] |=
		((afe_iqadc_reset_ov_det << TOF_RFBNDL_AFE_RESET_OV_DET_SHIFT) |
		(afe_iqadc_clamp_en << TOF_RFBNDL_AFE_IQADC_CLAMP_EN_SHIFT) |
		(afe_iqadc_adc_bias << TOF_RFBNDL_AFE_IQADC_ADC_BIAS_SHIFT) |
		(afe_iqadc_pwrup << TOF_RFBNDL_AFE_IQADC_PWRUP_SHIFT) |
		((afe_iqadc_mode & TOF_RFBNDL_AFE_IQADC_MODE_LB_MASK) <<
		 TOF_RFBNDL_AFE_IQADC_MODE_LB_SHIFT));

	tofi->bundle_offs_39[1] |=
		((afe_iqadc_rx_div4_en << TOF_RFBNDL_AFE_IQADC_RX_DIV4_EN_SHIFT) |
		(afe_ctrl_flash17lvl << TOF_RFBNDL_AFE_CTRL_FLASH17LVL_SHIFT) |
		(afe_iqadc_flashhspd << TOF_RFBNDL_AFE_IQADC_FLASHHSPD_SHIFT) |
		(((afe_iqadc_mode &  TOF_RFBNDL_AFE_IQADC_MODE_HB_MASK) >>
		TOF_RFBNDL_AFE_IQADC_MODE_HB_SHIFT)) |
		(afe_iqadc_flash_only << TOF_RFBNDL_AFE_IQADC_FLASH_ONLY_SHIFT));

	PHY_INFORM(("RFSeq offset 38 After = 0x%04x%04x%04x\n", tofi->bundle_offs_38[2],
			tofi->bundle_offs_38[1], tofi->bundle_offs_38[0]));
	PHY_INFORM(("RFSeq offset 39 After = 0x%04x%04x%04x\n", tofi->bundle_offs_39[2],
			tofi->bundle_offs_39[1], tofi->bundle_offs_39[0]));

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, TOF_RFBNDL_ADC_OFFSET_38,
			TOF_RFSEQ_BUNDLE_SIZE, (void *)(tofi->bundle_offs_38));

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, TOF_RFBNDL_ADC_OFFSET_39,
			TOF_RFSEQ_BUNDLE_SIZE, (void *)(tofi->bundle_offs_39));
}

static int
phy_ac_tof_setup_core_params(phy_info_t *pi, uint8 core, bool assign_buffer)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	const uint16 *pSrc;
	const uint16 *pEnd;
	uint16 shm[K_TOF_SHM_ARR_LENGTH], mask;
	int i = 0;

	tofi->tof_core = core;
	mask = (1 << tofi->tof_core);

	if (phy_ac_tof_seq_params(tofi, assign_buffer) != BCME_OK)
		return BCME_ERROR;

	if (TINY_RADIO(pi)) {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			pSrc = k_tof_seq_tiny_tbls_2G;
			pEnd = pSrc + ARRAYSIZE(k_tof_seq_tiny_tbls_2G);
		} else {
			pSrc = k_tof_seq_tiny_tbls;
			pEnd = pSrc + ARRAYSIZE(k_tof_seq_tiny_tbls);
		}
	} else {
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			pSrc = k_tof_seq_tbls_2G;
			pEnd = pSrc + ARRAYSIZE(k_tof_seq_tbls_2G);
		} else {
			pSrc = k_tof_seq_tbls;
			pEnd = pSrc + ARRAYSIZE(k_tof_seq_tbls);
		}
	}
	while (pSrc != pEnd) {
		uint32 id, width, len, tbl_len, offset;
		const uint16 *pTblData;

		id = (uint32)*pSrc++;
		offset = (uint32)*pSrc++;
		len = (uint32)*pSrc++;
		if (id == ACPHY_TBL_ID_RFSEQBUNDLE) {
			width = 48;
			tbl_len = len / 3;
			for (i = 0; i < len; i++) {
				shm[i] = pSrc[i];
				if ((offset >= 0x10) && ((i % 3) == 0)) {
					shm[i] = (shm[i] & ~7) | mask;
				}
			}
			pTblData = shm;
		} else {
			width = 16;
			tbl_len = len;
			pTblData = pSrc;
		}
		if (offset == 0x38) {
			tofi->bundle_offs_38[0] = pTblData[0];
			tofi->bundle_offs_38[1] = pTblData[1];
			tofi->bundle_offs_38[2] = pTblData[2];
			tofi->bundle_offs_39[0] = pTblData[3];
			tofi->bundle_offs_39[1] = pTblData[4];
			tofi->bundle_offs_39[2] = pTblData[5];
			PHY_INFORM(("len = %d RFSeq offset 0x%x Init = 0x%04x%04x%04x\n",
				tbl_len, offset, pTblData[2],
				pTblData[1], pTblData[0]));
		}
		wlc_phy_table_write_acphy(pi, id, tbl_len, offset, width, pTblData);
		pSrc += len;
	}
	phy_ac_tof_fill_write_shm(pi, shm, core, mask);
	phy_ac_tof_setup_rf_bundle(pi);

	return BCME_OK;
}

static int
phy_ac_tof_seq_setup(phy_info_t *pi, bool enter, bool tx, uint8 core, bool isInitiator,
		bool loadSPB, bool firstCall)
{
	/*
	 * 1) When firstCall = FALSE, enter and tx are warning.
	 * 2) isInitiator value is warning if TOF_TEST_TONE is not defined or if loadSPB is FALSE.
	*/

	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	uint8 stall_val;
	uint16 tof_rfseq_bundle_offset;
	int ret = BCME_OK;
	bool  suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	uint16 tof_seq_n;

	if ((!enter) && firstCall) {
		return BCME_OK;
	}

	if (((tofi->tof_rfseq_bundle_offset >= K_TOF_RFSEQ_BUNDLE_BASE) && !TINY_RADIO(pi)) ||
		((tofi->tof_rfseq_bundle_offset >= K_TOF_RFSEQ_TINY_BUNDLE_BASE) &&
		TINY_RADIO(pi)) || (READ_PHYREGFLD(pi, OCLControl1, ocl_mode_enable))) {
		return BCME_ERROR;
	}

	if (!enter) {
		phy_ac_tof_trig_dly_setup_acphy(pi, enter, tx);
	}

	if (firstCall) {
		if (phy_ac_tof_seq_params(tofi, TRUE) != BCME_OK)
			return BCME_ERROR;

		tof_seq_n = (1 << tofi->tof_seq_log2_n);
		WRITE_PHYREG(pi, sampleLoopCount, 0xffff);
		WRITE_PHYREG(pi, sampleDepthCount, (tof_seq_n - 1));

		if ((tx != tofi->tof_tx) && tofi->tof_setup_done) {
			tofi->tof_setup_done = FALSE;
		}

		if (tofi->tof_setup_done) {
			return BCME_OK;
		}

		tofi->tof_tx = tx;
		tofi->tof_phy_error = 0;
		tofi->isInvalid = 0;
	}

	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	/* Setup rfcontrol sequence for tx_on / tx_off events */
	tof_rfseq_bundle_offset = tofi->tof_rfseq_bundle_offset;
	phy_ac_tof_setup_core_params(pi, core, TRUE);
	phy_ac_tof_trig_dly_setup_acphy(pi, enter, tx);
	phy_ac_tof_seq_upd_dly(tofi, tx, core, FALSE);
	tofi->tof_rfseq_bundle_offset = tof_rfseq_bundle_offset;

	if (loadSPB) {
		ret = phy_ac_tof_set_ri_rr_spb_only(pi, isInitiator, FALSE);
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	if (!suspend) {
		wlapi_enable_mac(pi->sh->physhim);
	}

	/* tofi->tof_setup_done = TRUE; */
	return ret;
}

#if defined(TOF_CALC_SYM_BNDRY_OFFSET)
static int
wlc_phy_tof_calc_offset_from_sym_bndry(phy_info_t *pi, int16 sliding_ptr_offset,
	int16* ofdm_sym_bndry, int32* offset_from_bndry, int* bndry_idx)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	int i, offset, diff, sym_len;

	sym_len = (1 << pi_ac->tof_seq_log2_n);

	offset = (int)(sliding_ptr_offset-ofdm_sym_bndry[K_TOF_SYM_ARR_LEN-1]);

	for (i = 0; i < K_TOF_SYM_ARR_LEN; i++) {
		diff = (int)(sliding_ptr_offset-ofdm_sym_bndry[i]);
		if ((diff >= 0) && (diff < offset)) {
			offset = diff;
			*bndry_idx = i;
		}
	}
	if (offset > sym_len) {
		PHY_ERROR(("%s: ERROR: Couldn't Find First Symbol Boundary\n", __FUNCTION__));
		offset = 0;
		*offset_from_bndry = offset;
		*bndry_idx = 0;
		return BCME_ERROR;
	}

	*offset_from_bndry = offset;
	return BCME_OK;

}
#endif /* TOF_CALC_SYM_BNDRY_OFFSET */

static int
phy_ac_tof_chk_rx_window_off(phy_type_tof_ctx_t *ctx, int16 sc_ptr_offset,
	int16 sc_base_ptr, uint16 tof_seq_n)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	cint32 sc_win[10];
	int n, i;
	uint32 acc_pwr_start_win, acc_pwr_end_win;
	int16 sc_ptr;

	n = phy_ac_tof_sc_read(pi, TRUE, K_TOF_RX_SAMP_CHECK_NUM, &sc_win[0],
		sc_ptr_offset, sc_base_ptr, &sc_ptr);
	if (n != K_TOF_RX_SAMP_CHECK_NUM) {
		PHY_ERROR(("%s: Unable to dump the first %d samples of the window\n",
		__FUNCTION__, K_TOF_RX_SAMP_CHECK_NUM));
		return BCME_ERROR;
	}

	acc_pwr_start_win = 0;
	for (i = 0; i < K_TOF_RX_SAMP_CHECK_NUM; i++) {
#ifdef TOF_DBG_SEQ

		PHY_ERROR(("%s: i = %d, Start i,q = %d, %d\n", __FUNCTION__, i,
			sc_win[i].i, sc_win[i].q));
#endif
		acc_pwr_start_win += (sc_win[i].i * sc_win[i].i + sc_win[i].q * sc_win[i].q);
	}

	if (acc_pwr_start_win < K_MIN_ACC_TOF_RX_SAMP_PWR) {
		PHY_ERROR(("%s: Rx Power Lower than Min Pwr, start_pwr = %d\n",  __FUNCTION__,
			acc_pwr_start_win));
		tofi->tof_phy_error |= WL_PROXD_PHY_RX_STRT_WIN_OFF;
	}

	if (sc_ptr_offset < 0)
		sc_ptr_offset -= (tof_seq_n - K_TOF_RX_SAMP_CHECK_NUM -1);
	else
		sc_ptr_offset += (tof_seq_n - K_TOF_RX_SAMP_CHECK_NUM - 1);

	n = phy_ac_tof_sc_read(pi, TRUE, K_TOF_RX_SAMP_CHECK_NUM, &sc_win[0],
		sc_ptr_offset, sc_base_ptr, &sc_ptr);

	if (n != K_TOF_RX_SAMP_CHECK_NUM) {
		PHY_ERROR(("%s: Unable to dump the last %d samples of the window\n",
		__FUNCTION__, K_TOF_RX_SAMP_CHECK_NUM));
		return BCME_ERROR;
	}
	acc_pwr_end_win = 0;

	for (i = 0; i < K_TOF_RX_SAMP_CHECK_NUM; i++) {
#ifdef TOF_DBG_SEQ
		PHY_ERROR(("%s: i = %d, End i,q = %d, %d\n", __FUNCTION__, i,
			sc_win[i].i, sc_win[i].q));
#endif
		acc_pwr_end_win += (sc_win[i].i * sc_win[i].i + sc_win[i].q * sc_win[i].q);
	}

	PHY_ERROR(("%s: acc_pwr_start_win = %d, acc_pwr_end_win = %d\n", __FUNCTION__,
		acc_pwr_start_win, acc_pwr_end_win));
	if (acc_pwr_end_win < K_MIN_ACC_TOF_RX_SAMP_PWR) {
		PHY_ERROR(("%s: Rx Power Lower than Min Pwr, end_pwr = %d, Thresh = %d\n",
		__FUNCTION__, acc_pwr_end_win, K_MIN_ACC_TOF_RX_SAMP_PWR));
		tofi->tof_phy_error |= WL_PROXD_PHY_RX_END_WIN_OFF;
		if (acc_pwr_start_win < K_MIN_ACC_TOF_RX_SAMP_PWR) {
			PHY_ERROR(("%s: Rx Power failed check on Start and End. No Valid Samples\n",
			 __FUNCTION__));
		}
	}
	return BCME_OK;
}

static int phy_ac_seq_ts(phy_type_tof_ctx_t *ctx, int n, cint32* p_buffer, int tx,
	int cfo, int adj, void* pparams, int32* p_ts,
	int32* p_seq_len, uint32* p_raw, uint8* ri_rr,
	const uint8 smooth_win_en)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	int16 sc_ptr;
	int32 ts[2], dT;
	int i, n_out, a, b, ret_val_l, ret_val = BCME_OK;
	uint16 tof_seq_n;
	uint16 tmp_tof_sc_Fs;
	int32 tof_seq_M;
	uint16 snr_thresh, bitflip_thresh;
#if !defined(TOF_TEST_TONE)
	uint16 tmp_seq_log2_n;
#endif

#ifdef TOF_COLLECT
	int collect_hraw_size = 0;
	if (CHSPEC_IS20(pi->radio_chanspec)) {
		collect_hraw_size = K_TOF_COLLECT_HRAW_SIZE_20MHZ;
	} else {
		collect_hraw_size = K_TOF_COLLECT_HRAW_SIZE_80MHZ;
	}
#endif /* TOF_COLLECT */

	tof_seq_n = (1 << tofi->tof_seq_log2_n);

	tofi->tof_phy_error = 0;
	if (n < tof_seq_n) {
		return BCME_ERROR;
	}

	a = READ_PHYREGCE(pi, Core1RxIQCompA, tofi->tof_core);
	b = READ_PHYREGCE(pi, Core1RxIQCompB, tofi->tof_core);
	if (a > 511) {
		a -= 1024;
	}
	if (b > 511) {
		b -= 1024;
	}

	wlapi_tof_retrieve_thresh(pparams, &bitflip_thresh, &snr_thresh);

	for (i = 0; i < 2; i++) {
		tof_seq_n = (1 << tofi->tof_seq_log2_n);
		if (i) {
			phy_ac_tof_chk_rx_window_off(pi, -(tofi->tof_ucode_dlys_us[tx][3 + i]),
				K_TOF_SEQ_SC_START, tof_seq_n);
		}
		n_out = phy_ac_tof_sc_read(pi, TRUE, n, p_buffer,
			-(tofi->tof_ucode_dlys_us[tx][3 + i]),
			K_TOF_SEQ_SC_START, &sc_ptr);

		if (n_out != n) {
			return BCME_ERROR;
		}
#ifdef TOF_COLLECT
		if (p_raw && (2 * (n_out + 1) <= collect_hraw_size)) {
			int j;
			for (j = 0; j < n_out; j++) {
#if defined(TOF_DBG_SEQ)
				if ((j == 0) || (j == (n_out-1))) {
					PHY_ERROR(("%s: buf[%d] = %d, %d\n",
					 __FUNCTION__, j, p_buffer[j].i, p_buffer[j].q));
				}
#endif
				*p_raw++ = (uint32)(p_buffer[j].i & 0xffff) |
					((uint32)(p_buffer[j].q & 0xffff) << 16);
			}
			*p_raw++ = (uint32)((int32)a & 0xffff) |
				(uint32)(((int32)b & 0xffff) << 16);
		}
#endif /* TOF_COLLECT */

#if defined(TOF_TEST_TONE)
		ts[i] = 0;
		tmp_tof_sc_Fs = tofi->tof_sc_FS;
		adj = 0;
#else
		ret_val_l = phy_ac_tof_mf(pi, tof_seq_n, p_buffer, FALSE, (i == 0),
			a, b, (i) ? cfo : 0,
			K_TOF_MF_IN_SHIFT,
			K_TOF_MF_OUT_SCALE,
			K_TOF_MF_OUT_SHIFT,
			bitflip_thresh,
			snr_thresh,
			smooth_win_en);
#ifdef TOF_SEQ_20MHz_BW_512IFFT
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			tmp_seq_log2_n = K_TOF_SEQ_N_20MHZ;
			tmp_tof_sc_Fs = 160;
			tof_seq_n = (1 << tmp_seq_log2_n);
		} else
#endif /* TOF_SEQ_20MHz_BW_512IFFT */
		{
			tmp_seq_log2_n = tofi->tof_seq_log2_n;
			tmp_tof_sc_Fs = tofi->tof_sc_FS;
		}

		ret_val_l = wlapi_tof_pdp_ts(tmp_seq_log2_n, (void*)p_buffer, tmp_tof_sc_Fs, i,
			pparams, &ts[i], NULL, &(tofi->tof_phy_error));
		if (ret_val_l != BCME_OK) {
			printf("func %s iter %d error 1 \n", __FUNCTION__, i);
			ret_val = BCME_ERROR;
		}

		if (tofi->isInvalid) {
			PHY_ERROR(("Low confidence in the"
				" measurement on this core due to low rssi.\n"));
		}

#endif /* defined(TOF_TEST_TONE) || 0 */
	}

#ifdef TOF_COLLECT
	int ri_rr_len;
	if (pi_ac->tofi->flag_sec_2_0) {
		ri_rr_len = FTM_TPK_RI_RR_LEN_SECURE_2_0;
	} else {
		ri_rr_len = FTM_TPK_RI_RR_LEN;
	}
	if (ri_rr != NULL) {
		for (i = 0; i < ri_rr_len; i++) {
			*(ri_rr + i) = tofi->ri_rr[i] & 0xff;
		}
	}
#endif /* TOF_COLLECT */

	tof_seq_M = ((10000 * tof_seq_n) / tmp_tof_sc_Fs);

	ts[0] += adj;
	dT = (ts[tx] - ts[tx ^ 1]);
	if (dT < 0) {
		dT += tof_seq_M;
	}
	*p_ts = dT;
	*p_seq_len = tof_seq_M;
	return ret_val;
}

#if defined(TOF_DBG_SEQ)
static int
phy_ac_tof_dbg(phy_type_tof_ctx_t *ctx, int arg)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	cint32 buf[K_TOF_DBG_SC_DELTA];
	int16  p, p_start;
	int i = 0, n = 0;
	bool  suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	uint8 stall_val;

	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	p = K_TOF_SEQ_SC_START;
	p += (K_TOF_DBG_SC_DELTA * arg);
	if (arg >= 255) {
		int j;
		uint16 v, offset = 0;
		uint16 bundle[3 * 16];
		const uint16 bundle_addr[] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50 };
		for (i = 0; i < ARRAYSIZE(bundle_addr); i++) {
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE,
				8, bundle_addr[i] + 8, 48, (void*)bundle);
			for (j = 0; j < 16; j++) {
				printf("RFBUNDLE 0x%x : 0x%04x%04x%04x\n",
					(bundle_addr[i]+j+8),
					bundle[3 * j + 2], bundle[3 * j + 1], bundle[3 * j + 0]);
			}
		}
		for (i = 0; i < 16; i++) {
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x290 + i), 16,
				(void *)&bundle[0]);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x320 + i), 16,
				(void *)&bundle[1]);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x260 + i), 16,
				(void *)&bundle[2]);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x2f0 + i), 16,
				(void *)&bundle[3]);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x000 + i), 16,
				(void *)&bundle[4]);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x070 + i), 16,
				(void *)&bundle[5]);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x010 + i), 16,
				(void *)&bundle[6]);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0x080 + i), 16,
				(void *)&bundle[7]);
			PHY_INFORM(("RFSEQ RXs 0x%04x 0x%04x TXs 0x%04x 0x%04x TX 0x%04x 0x%04x "
				"RX 0x%04x 0x%04x\n",
				bundle[0], bundle[1], bundle[2], bundle[3],
				bundle[4], bundle[5], bundle[6], bundle[7]));
		}
		for (i = 0; i < 3; i++) {
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
				(K_TOF_SEQ_RFSEQ_GAIN_BASE + i * 0x10 +
				K_TOF_SEQ_RFSEQ_LOOPBACK_GAIN_OFFSET), 16, (void*)&bundle[0]);
			wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
				(K_TOF_SEQ_RFSEQ_GAIN_BASE + i * 0x10 +
				K_TOF_SEQ_RFSEQ_RX_GAIN_OFFSET), 16, (void*)&bundle[1]);
			printf("GC%d  LBK 0x%04x RX 0x%04x\n", i, bundle[0], bundle[1]);
		}
		offset = K_TOF_SEQ_SHM_SETUP_REGS_OFFSET;
		n = 46;
		while (n-- > 0) {
			v = wlapi_bmac_read_shm(pi->sh->physhim, (tofi->tof_shm_ptr + offset));
			PHY_INFORM(("SHM %d 0x%04x\n",
				((offset - K_TOF_SEQ_SHM_SETUP_REGS_OFFSET) >> 1), v));
			offset += 2;
		}
		n = 1;
	} else if (arg == 252) {
		printf("MAC 0x%x STRT %d STP %d CUR %d\n",
			R_REG(pi->sh->osh, D11_PSM_PHY_CTL(pi)),
			R_REG(pi->sh->osh, D11_SCP_STRTPTR(pi)),
			R_REG(pi->sh->osh, D11_SCP_STOPPTR(pi)),
			R_REG(pi->sh->osh, D11_SCP_CURPTR(pi)));
		n = 1;
	} else {
		if (arg == 0) {
			printf("MAC 0x%x STRT %d STP %d CUR %d\n",
				R_REG(pi->sh->osh, D11_PSM_PHY_CTL(pi)),
				R_REG(pi->sh->osh, D11_SCP_STRTPTR(pi)),
				R_REG(pi->sh->osh, D11_SCP_STOPPTR(pi)),
				R_REG(pi->sh->osh, D11_SCP_CURPTR(pi)));
		}

		n = ((int)R_REG(pi->sh->osh, D11_SCP_CURPTR(pi)) - p);
		if (n > K_TOF_DBG_SC_DELTA) {
			n = K_TOF_DBG_SC_DELTA;
		}

		if (n > 0) {
			arg = phy_ac_tof_sc_read(pi, TRUE, n, buf, p, K_TOF_SEQ_SC_START, &p_start);
			i = 0;
			while (i < arg) {
				if (buf[i].i > 2047)
					buf[i].i -= 4096;
				if (buf[i].q > 2047)
					buf[i].q -= 4096;
				printf("SD %4d %d %d\n", p_start, buf[i].i, buf[i].q);
				i++;
				p_start++;
			}
		} else {
			printf("\n");
		}
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	if (!suspend) {
		wlapi_enable_mac(pi->sh->physhim);
	}

	return (n > 0) ? 1 : 0;
}
#endif /* defined(TOF_DBG_SEQ) */

#endif /* WL_PROXD_SEQ */

static int phy_ac_tof(phy_type_tof_ctx_t *ctx, bool enter, bool tx, bool hw_adj,
	bool seq_en, int core, int emu_delay)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	phy_info_acphy_t *aci = tofi->aci;
	bool change = tofi->tof_active != enter;
	bool suspend;
	int retval = BCME_OK;
#ifdef WL_PROXD_SEQ
	bool loadSPB;
#endif /* WL_PROXD_SEQ */
	phy_ac_rxgcrs_info_t *rxgcrsi = pi->u.pi_acphy->rxgcrsi;

	if (!pi->sh->clk) {
		retval = BCME_NOCLK;
		goto fail;
	}

	suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);

	tofi->emu_delay = emu_delay;
	if (change) {
#ifdef WL_PROXD_SEQ
		if (seq_en) {
#ifndef K_TOF_LOAD_SPB_ONLY
			loadSPB = TRUE;
#else
			loadSPB = FALSE;
#endif /* !K_TOF_LOAD_SPB_ONLY */
			retval = phy_ac_tof_seq_setup(pi, enter, tx, core, TRUE, loadSPB,
				TRUE);
			if (!suspend) {
				wlapi_suspend_mac_and_wait(pi->sh->physhim);
			}
			if (enter) {
				phy_watchdog_suspend(pi);
				phy_ac_tof_en_dis_aci(tofi, enter);
				phy_ac_tof_fdiqcomp_save_disable(tofi);
				phy_rxgcrs_sel_classifier(pi, TOF_CLASSIFIER_BPHY_OFF_OFDM_ON);
			} else {
				phy_ac_tof_en_dis_aci(tofi, enter);
				/* Restore fdiqcomp state */
				MOD_PHYREG(pi, rxfdiqImbCompCtrl, rxfdiqImbCompEnable,
					tofi->tof_rx_fdiqcomp_enable);
				MOD_PHYREG(pi, fdiqImbCompEnable, txfdiqImbCompEnable,
					tofi->tof_tx_fdiqcomp_enable);
				phy_rxgcrs_sel_classifier(pi, TOF_CLASSIFIER_BPHY_ON_OFDM_ON);
				phy_watchdog_resume(pi);
			}
			if (!suspend) {
				wlapi_enable_mac(pi->sh->physhim);
			}
		}
		else
#endif /* WL_PROXD_SEQ */
		{
			tofi->tof_setup_done = FALSE;
			if (!suspend) {
				wlapi_suspend_mac_and_wait(pi->sh->physhim);
			}
			if (enter) {
				phy_watchdog_suspend(pi);
				if (phy_ac_rxgcrs_get_cap_lesi(pi)) {
					phy_ac_rxgcrs_iovar_get_lesi_ovrd(rxgcrsi,
						&tofi->tof_lesi_saved);
					phy_ac_rxgcrs_iovar_set_lesi_ovrd(rxgcrsi, FALSE);
				}
				/* Disable HWOBSS */
				tofi->hwobss_en = READ_PHYREGFLD(pi, obss_control, obss_mit_en);
				if (tofi->hwobss_en) {
					phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, FALSE);
				}
				/* Disable BT coex */
				phy_btcx_disable_arbiter(pi->btcxi);
				phy_ac_tof_fdiqcomp_save_disable(tofi);
				if (hw_adj) {
					/* Save channel smoothing state and enable special  mode */
					phy_ac_chanmgr_save_smoothing(aci->chanmgri,
						&tofi->tof_smth_enable, &tofi->tof_smth_dump_mode);
					wlc_phy_smth(pi, SMTH_ENABLE, SMTH_TIMEDUMP_AFTER_IFFT);
					tofi->tof_smth_forced = TRUE;
				}
			} else {
				/* Enable BT coex */
				phy_btcx_enable_arbiter(pi->btcxi);
				/* Restore fdiqcomp state */
				MOD_PHYREG(pi, rxfdiqImbCompCtrl, rxfdiqImbCompEnable,
					tofi->tof_rx_fdiqcomp_enable);
				MOD_PHYREG(pi, fdiqImbCompEnable, txfdiqImbCompEnable,
					tofi->tof_tx_fdiqcomp_enable);
				if (tofi->tof_smth_forced) {
					/* Restore channel smoothing state */
					tofi->tof_smth_forced = FALSE;
					wlc_phy_smth(pi, tofi->tof_smth_enable,
						tofi->tof_smth_dump_mode);
				}

				/* Re-enable HWOBSS */
				if (tofi->hwobss_en) {
					phy_ac_chanmgr_hwobss(pi->u.pi_acphy->chanmgri, TRUE);
				}
				if (phy_ac_rxgcrs_get_cap_lesi(pi)) {
					phy_ac_rxgcrs_iovar_set_lesi_ovrd(rxgcrsi,
						tofi->tof_lesi_saved);
				}
				phy_watchdog_resume(pi);
			}

			wlc_phy_resetcca_acphy(pi);
			if (!suspend) {
				wlapi_enable_mac(pi->sh->physhim);
			}
		}
	}
fail:
	tofi->tof_active = enter;
	return retval;
}

/* Save state fdiqcomp and disable */
static void
phy_ac_tof_fdiqcomp_save_disable(phy_ac_tof_info_t *tofi)
{
	tofi->tof_rx_fdiqcomp_enable = (uint8)READ_PHYREGFLD(tofi->pi, rxfdiqImbCompCtrl,
		rxfdiqImbCompEnable);
	tofi->tof_tx_fdiqcomp_enable = (uint8)READ_PHYREGFLD(tofi->pi, fdiqImbCompEnable,
		txfdiqImbCompEnable);
	MOD_PHYREG(tofi->pi, rxfdiqImbCompCtrl, rxfdiqImbCompEnable, 0);
	MOD_PHYREG(tofi->pi, fdiqImbCompEnable, txfdiqImbCompEnable, 0);
}

#ifdef WL_PROXD_SEQ
/* Enable Disable ACI */
static void
phy_ac_tof_en_dis_aci(phy_ac_tof_info_t *tofi, bool enter)
{
	phy_info_t *pi = tofi->pi;
	if (enter) {
		/* Force ACI Mitigation mode */
		if ((ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_1(pi) ||
			ACMINORREV_4(pi))) ||
			(ACMAJORREV_5(pi->pubpi->phy_rev))) {
			tofi->aci_en = READ_PHYREGFLD(pi, ACI_Detect_CTRL,
				aci_detect_enable);
			PHY_ERROR(("%s: ACI Mode = %d\n", __FUNCTION__,
				tofi->aci_en));
			MOD_PHYREG(pi, ACI_Detect_CTRL,
				aci_detect_enable, 0);
			wlc_phy_hwaci_mitigate_acphy(pi, TRUE);
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
		}
	} else {
		/* Restore back ACI state */
		if ((ACMAJORREV_2(pi->pubpi->phy_rev) &&
			(ACMINORREV_1(pi) ||
			ACMINORREV_4(pi))) ||
			(ACMAJORREV_5(pi->pubpi->phy_rev))) {
			/* This ensures an interrupt is triggered with current */
			/* aci_present_state once detection is enabled */
			wlapi_bmac_write_shm(pi->sh->physhim, M_HWACI_STATUS(pi),
				K_TOF_RESTORE_HWACI_INTR);
			wlc_phy_hwaci_mitigate_acphy(pi, tofi->prev_aci_state);
			MOD_PHYREG(pi, ACI_Detect_CTRL, aci_detect_enable,
				tofi->aci_en);
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
		}

	}
}
#endif /* WL_PROXD_SEQ */

/* Unpacks floating point to fixed point for further processing */
/* Fixed point format:

A.fmt = TRUE
sign      real         sign       image             exp
|-|--------------------||-|--------------------||----------|
size:            nman                   nman              nexp

B.fmt = FALSE
exp     sign		 image                  real
|----------||-|-||-------------------||--------------------|
size:		 nexp	 1 1          nman					nman

When Hi is NULL, we return "real * real + image * image" in Hr array, otherwise
real and image is save in Hr and Hi.

When autoscale is TRUE, calculate the max shift to save fixed point value into uint32
*/

/*
H holds upacked 32 bit data when the function is called.
H and Hout could partially overlap.
H and h can not overlap
*/

static int
wlc_unpack_float_acphy(int nbits, int autoscale, int shft,
int fmt, int nman, int nexp, int nfft, uint32* H, cint32* Hout, int32* h)
{
	int e_p, maxbit, e, i, pwr_shft = 0, e_zero, sgn;
	int n_out, e_shift;
	int8 He[TOF_NFFT_160MHZ];
	int32 vi, vq, *pOut;
	uint32 x, iq_mask, e_mask, sgnr_mask, sgni_mask;
	int err = BCME_OK;

	if (nfft > TOF_NFFT_160MHZ)
		return BCME_RANGE;
	/* when fmt is TRUE, the size nman include its sign bit */
	/* so need to minus one to get value mask */
	if (fmt)
		iq_mask = (1 << (nman - 1)) - 1;
	else
		iq_mask = (1 << nman) - 1;

	e_mask = (1 << nexp) - 1;	/* exp part mask */
	e_p = (1 << (nexp - 1));	/* max abs value of exp */

	if (h) {
		/* Set pwr_shft to make sure that square sum can be hold by uint32 */
		pwr_shft = (2 * nman + 1 - 31);
		if (pwr_shft < 0)
			pwr_shft = 0;
		pwr_shft = (pwr_shft + 1) >> 1;
		sgnr_mask = 0;	/* don't care sign for square sum */
		sgni_mask = 0;
		e_zero = -(2 * (nman - pwr_shft) + 1);
		pOut = (int32*)h;
		n_out = nfft;
		e_shift = 0;
	} else {
		/* Set the location of sign bit */
		if (fmt) {
			sgnr_mask = (1 << (nexp + 2 * nman - 1));
			sgni_mask = (sgnr_mask >> nman);
		} else {
			sgnr_mask = (1 << 2 * nman);
			sgni_mask = (sgnr_mask << 1);
		}
		e_zero = -nman;
		pOut = (int32*)Hout;
		n_out = (nfft << 1);
		e_shift = 1;
	}

	maxbit = -e_p;
	for (i = 0; i < nfft; i++) {
		/* get the real, image and exponent value */
		if (fmt) {
			vi = (int32)((H[i] >> (nexp + nman)) & iq_mask);
			vq = (int32)((H[i] >> nexp) & iq_mask);
			e = (int)(H[i] & e_mask);
		} else {
			vi = (int32)(H[i] & iq_mask);
			vq = (int32)((H[i] >> nman) & iq_mask);
			e = (int32)((H[i] >> (2 * nman + 2)) & e_mask);
		}

		/* adjust exponent */
		if (e >= e_p)
			e -= (e_p << 1);

		if (h) {
			/* calculate square sum of real and image data */
			vi = (vi >> pwr_shft);
			vq = (vq >> pwr_shft);
			h[i] = vi*vi + vq*vq;
			vq = 0;
			e = 2 * (e + pwr_shft);
		}

		He[i] = (int8)e;

		/* auto scale need to find the maximus exp bits */
		x = (uint32)vi | (uint32)vq;
		if (autoscale && x) {
			uint32 m = 0xffff0000, b = 0xffff;
			int s = 16;

			while (s > 0) {
				if (x & m) {
					e += s;
					x >>= s;
				}
				s >>= 1;
				m = (m >> s) & b;
				b >>= s;
			}
			if (e > maxbit)
				maxbit = e;
		}

		if (!h) {
			if (H[i] & sgnr_mask)
				vi |= K_TOF_UNPACK_SGN_MASK;
			if (H[i] & sgni_mask)
				vq |= K_TOF_UNPACK_SGN_MASK;
			Hout[i].i = vi;
			Hout[i].q = vq;
		}
	}

	/* shift bits */
	if (autoscale)
		shft = nbits - maxbit;

	/* scal and sign */
	for (i = 0; i < n_out; i++) {
		e = He[(i >> e_shift)] + shft;
		vi = *pOut;
		sgn = 1;
		if (!h && (vi & K_TOF_UNPACK_SGN_MASK)) {
			sgn = -1;
			vi &= ~K_TOF_UNPACK_SGN_MASK;
		}
		/* trap the zero case */
		if (e < e_zero) {
			vi = 0;
		} else if (e < 0) {
			e = -e;
			vi = (vi >> e);
		} else {
			vi = (vi << e);
		}
		*pOut++ = (int32)sgn*vi;
	}

	return err;
}

#ifdef WL_PROXD_SEQ
static void
phy_ac_tof_setup_ack_core(phy_type_tof_ctx_t *ctx, int core)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	uint16 phyctl0;
	uint offset = tofi->tof_shm_ptr + M_TOF_PHYCTL0_OFFSET(pi);

	ASSERT(pi->sh->clk);

	phyctl0 = wlapi_bmac_read_shm(pi->sh->physhim, offset);
	phyctl0 &= ~(0x7 << 6);
	phyctl0 |=  (1 << (core + 6));

	wlapi_bmac_write_shm(pi->sh->physhim, offset, phyctl0);
}

static int
phy_ac_tof_calc_snr_bitflips(phy_type_tof_ctx_t *ctx, void *In,
	wl_proxd_bitflips_t *bit_flips, wl_proxd_snr_t *snr)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	cint32* chan = (cint32*) tofi->chan;
	uint16 bw_factor = 0;
	uint16  print_chan_len;
	int retval = BCME_OK;
	const int num_max_cores = PHYCORENUM((pi)->pubpi->phy_corenum);

	if (CHSPEC_IS80(pi->radio_chanspec)) {
		bw_factor = 2;
	} else if (CHSPEC_IS20(pi->radio_chanspec)) {
		bw_factor = 0;
	} else {
		PHY_INFORM(("ERROR: Bandwidth other than 20 and 80 is not supported.\n"));
		return BCME_UNSUPPORTED;
	}
	print_chan_len = (K_TOF_CHAN_LENGTH_20M << bw_factor);
	chan += (num_max_cores * print_chan_len);
	if (tofi->flag_sec_2_0) {
		wlc_phy_tof_conj_arr(chan, print_chan_len);
	}
	retval = phy_ac_tof_demod_snr(pi, In, (void*)chan, bw_factor, bit_flips, snr);
	return retval;
}

static void
phy_ac_tof_core_select(phy_type_tof_ctx_t *ctx, const uint32 gdv_th, const int32 gdmm_th,
		const int8 rssi_th, const int8 delta_rssi_th, uint8* core, uint8 core_mask)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	const int num_max_cores = PHYCORENUM((pi)->pubpi->phy_corenum);
	const uint16 *band_length, *sc_idx_arr;
	cint32* chan = (cint32*) tofi->chan;
	int32 gd_m[num_max_cores], minm_gd = INT32_MAX, minm_idx = -1, minv_idx = -1, minr_idx = -1;
	int32 delta_th[num_max_cores], maxdth_v = INT32_MIN, maxdth_idx = -1, gd_th;
	uint32 gd_v[num_max_cores], minv_gd = UINT32_MAX;
	uint16 num_bl, chan_len, num_rssi_true = 0, num_var_true = 0, num_delta_th_true = 0;
	uint8 bw_factor = 0, i; //, shared_core = 0;
	int8 sel_core = -1, max_rssi = INT8_MIN;
	bool cond_rssi[num_max_cores], cond_var[num_max_cores], cond_delta_rssi[num_max_cores];
	bool cond_delta_th[num_max_cores], cond_core_on[num_max_cores];

	gd_th = gdmm_th;
	PHY_ERROR(("gd_th = %d", gd_th));

	tofi->isInvalid = 0;
	/* if (num_max_cores > 1)
		shared_core = (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags2)
						& BFL2_BT_SHARE_ANT0) ? 0 : 1;
	*/

	for (i = 0; i < num_max_cores; i++) {
		cond_rssi[i] = FALSE;
		cond_delta_rssi[i] = FALSE;
		cond_var[i] = FALSE;
		cond_delta_th[i] = FALSE;
		delta_th[i] = 0;
		tofi->rssi[i] = phy_rssi_get_rssi(pi, i);

		cond_core_on[i] = (((stf_shdata->phytxchain >> i) & 0x1) &
			((stf_shdata->phyrxchain >> i) & 0x1)) ? ((core_mask >> i) & 0x1) : FALSE;

		PHY_ERROR(("rssi[%d] = %d, cond_core_on[%d] = %d\n", i, tofi->rssi[i],
			i, cond_core_on[i]));

		if ((tofi->rssi[i] > rssi_th) && (tofi->rssi[i] != 0) && cond_core_on[i]) {
			num_rssi_true++;
			minr_idx = i;
			cond_rssi[i] = TRUE;
		}

		if ((tofi->rssi[i] > max_rssi) && cond_rssi[i]) {
			max_rssi = tofi->rssi[i];
		}
	}

	if (num_rssi_true != 1) {
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			bw_factor = 2;
		} else if (CHSPEC_IS20(pi->radio_chanspec)) {
			bw_factor = 0;
		} else {
			PHY_ERROR(("ERROR: Bandwidth other than 20 and 80 is not supported.\n"));
		}

		chan_len = (K_TOF_CHAN_LENGTH_20M << bw_factor);
		switch (bw_factor) {
			case 2:
				num_bl = K_TOF_NUM_LEGACY_BL_80M;
				band_length = band_length_80MHz;
				sc_idx_arr = nonzero_sc_idx_legacy_80MHZ;
				break;
			case 0:
			default:
				num_bl = K_TOF_NUM_LEGACY_BL_20M;
				band_length = band_length_20MHz;
				sc_idx_arr = nonzero_sc_idx_legacy_20MHZ;
		}
		for (i = 0; i < num_max_cores; i++) {
			cond_delta_rssi[i] = cond_rssi[i] &&
				((max_rssi - tofi->rssi[i]) < delta_rssi_th);
			PHY_ERROR(("cond_delta_rssi[%d] = %d\n", i, cond_delta_rssi[i]));

			if (((num_rssi_true == 0) || cond_rssi[i]) &&
					cond_delta_rssi[i] && cond_core_on[i]) {
				phy_ac_tof_group_delay_mv(pi, (chan + i*chan_len), num_bl,
					band_length, sc_idx_arr, gd_th, (gd_m + i), (gd_v + i),
					(delta_th + i));
				PHY_ERROR(("core = %d, gd_m[%d] = %d, gd_v[%d] = %u,"
						"delta_th[%d] = %d\n",
						i, i, gd_m[i], i, gd_v[i], i, delta_th[i]));
				if (delta_th[i] >= 0) {
					cond_delta_th[i] = TRUE;
					num_delta_th_true++;
				}
				if (delta_th[i] >= maxdth_v) {
					maxdth_v = delta_th[i];
					maxdth_idx = i;
				}
				if (gd_v[i] < gdv_th) {
					num_var_true++;
					cond_var[i] = TRUE;
				}
				if (gd_v[i] <= minv_gd) {
					minv_gd = gd_v[i];
					minv_idx = i;
				}
			}
		}
		if (num_delta_th_true == 0) {
			sel_core = maxdth_idx;
		} else {
			if (num_var_true == 0) {
				sel_core = minv_idx;
			} else {
				for (i = 0; i < num_max_cores; i++) {
					if ((gd_m[i] <= minm_gd) && (cond_var[i]) &&
							(cond_delta_th[i])) {
						minm_gd = gd_m[i];
						minm_idx = i;
					}
					PHY_ERROR(("core = %d, minm_gd = %d, minm_idx = %d\n",
							i, minm_gd, minm_idx));
				}
				sel_core = minm_idx;
			}
		}
		PHY_ERROR(("Selecting core : "));
	} else {
		PHY_ERROR(("Selecting core based on rssi : "));
		sel_core = minr_idx;
	}
	if ((sel_core < 0) || (sel_core >= num_max_cores)) {
		*core = (cond_core_on[0]) ? 0 : 1;
		PHY_ERROR(("Error : Can't select core. Defaulting to core %d.\n", *core));
	} else {
		*core = sel_core;
		PHY_ERROR(("core = %d\n", *core));
	}
	phy_ac_tof_setup_ack_core(tofi, *core);
	if (!cond_rssi[*core]) {
		tofi->isInvalid = tofi->isInvalid | 0x1;
	}
	/*
	if (!cond_var[*core]) {
		tofi->isInvalid = tofi->isInvalid | 0x2;
	}
	*/
}
#endif /* WL_PROXD_SEQ */

static int
phy_ac_tof_get_chan_tbl_info(phy_info_t* pi, h_tbl_info_t *h_tbl_info, int fft_len, bool he_frm)
{
	h_tbl_info->id = ACPHY_TBL_ID_CHANEST(h_tbl_info->core);
	h_tbl_info->read_len = (he_frm) ? (4 * fft_len) : fft_len;
	h_tbl_info->sts_offset = (h_tbl_info->nsts * CHANESTTBL_REV0_DATA_OFFSET);
	h_tbl_info->width = CORE0CHANESTTBL_TABLE_WIDTH;
	return BCME_OK;
}

static int
phy_ac_tof_read_chan_tbl(phy_info_t* pi, uint32 *H, h_tbl_info_t *h_tbl_info)
{
	int err = BCME_OK;
	uint32 table_idx, t_offset, table_width;
	int t_read_len;

	table_idx = h_tbl_info->id;
	t_read_len = h_tbl_info->read_len;
	t_offset = h_tbl_info->sts_offset;
	table_width = h_tbl_info->width;

	wlc_phy_table_read_acphy(pi, table_idx, t_read_len, t_offset, table_width, H);

	return err;
}

static int
phy_ac_tof_get_chan(phy_info_t* pi, uint32 *Hout, h_tbl_info_t *h_tbl_info,
		int fft_len, bool he_frm)
{
	uint32 *H_tbl;
	int idx;
	uint8 deci_fac;
	int ret = BCME_OK;

	if ((H_tbl = (uint32 *)phy_malloc(pi, h_tbl_info->read_len * sizeof(uint32))) == NULL) {
		PHY_ERROR(("phy_ac_tof_get_chan: phy_malloc failed\n"));
		ret = BCME_NOMEM;
		goto fail;
	}

	if (!he_frm) {
		/* non-11ax */
		uint8 num_sb80, sb80;
		uint16 nlen;
		uint32 *H_sb80 = H_tbl;
		if (fft_len == TOF_NFFT_160MHZ) {
			num_sb80 = 2;
			nlen = fft_len >> 1;
		} else {
			num_sb80 = 1;
			nlen = fft_len;
		}
		/* Treat each 80MHz sub-bands in 160MHz separately same as 80MHz */
		for (sb80 = 0u; sb80 < num_sb80; sb80++) {
			ret = phy_ac_tof_read_chan_tbl(pi, H_sb80, h_tbl_info);
			if (ret != BCME_OK) {
				goto fail;
			}

			/* FFT shift */
			if (fft_len == TOF_NFFT_160MHZ)	{
				uint32 H_tmp;
				uint16 i, nlen_by2 = nlen >> 1;
				for (i = 0u; i < nlen_by2; i++) {
					H_tmp = H_sb80[i];
					H_sb80[i] = H_sb80[i + nlen_by2];
					H_sb80[i + nlen_by2] = H_tmp;
				}
			}

			memcpy((void*)(Hout + (nlen * sb80)), (void*)H_sb80, nlen * sizeof(uint32));
		}
	} else {
		/* Reading final tracked channel from the chanest table */
		phy_ac_tof_read_chan_tbl(pi, H_tbl, h_tbl_info);
		/* Decimating by factor of 4 for HE case */
		deci_fac = 4u;
		for (idx = 0; idx < fft_len; idx++) {
			*(Hout + idx) = *(H_tbl + (deci_fac * idx));
		}
	}
fail:
	if (H_tbl != NULL) {
		phy_mfree(pi, H_tbl, h_tbl_info->read_len * sizeof(uint32));
	}
	return ret;
}

static int
phy_ac_tof_reorder_tones(uint32 *Hpacked, uint32 *Hpacked_buf, bool swap_pn_half,
	bool tdcs_en, int fft_len)
{
	int i, i_l, i_r, n1, n2, n3;
	int nfft, nfft_over_2;

	bzero((void *)Hpacked_buf, fft_len * sizeof(int32));
	if (swap_pn_half) {
		/* reorder tones */
		nfft = fft_len;
		nfft_over_2 = (fft_len >> 1);
		if (nfft == TOF_NFFT_160MHZ) {
			i_l = 250;
			i_r = 6;
		} else if (nfft == TOF_NFFT_80MHZ) {
			i_l = 122;
			i_r = 2;
		} else if (nfft == TOF_NFFT_40MHZ) {
			i_l = 58;
			i_r = 2;
		} else {
			/* for legacy this is 26, for vht-20 this is 28 */
			i_l = 28;
			i_r = 1;
		}
		if (tdcs_en) {
			i_l = nfft_over_2 - 1;
			i_r = 1;
			Hpacked_buf[i_r-1] = Hpacked[i_l+1];
			Hpacked_buf[i_l+1] = Hpacked[i_r-1];
		}
		for (i = i_l; i >= i_r; i--) {
			n1 = nfft_over_2 - i;
			n2 = nfft_over_2 + i;
			n3 = nfft - i;
			Hpacked_buf[n1] = Hpacked[n3];
			Hpacked_buf[n2] = Hpacked[i];
		}
	} else {
		for (i = 0; i < fft_len; i++) {
			Hpacked_buf[i] = Hpacked[i];
		}
	}

	return BCME_OK;
}

static int
phy_ac_tof_unpack_chan(phy_info_t *pi, uint32* Hin, cint32* Hout, int nbits,
	bool tdcs_en, int fft_len)
{
	int err = BCME_OK;

	err = wlc_unpack_float_acphy(nbits, UNPACK_FLOAT_AUTO_SCALE, 0,
		CORE0CHANESTTBL_FLOAT_FORMAT, CORE0CHANESTTBL_REV0_DATA_SIZE,
		CORE0CHANESTTBL_REV0_EXP_SIZE, fft_len, Hin, Hout, NULL);

	return err;
}

/* Get channel frequency response for deriving 11v rx timestamp */
static int
phy_ac_tof_chan_freq_response(phy_type_tof_ctx_t *ctx, int fft_len, int nbits, bool swap_pn_half,
		uint32 offset, cint32* H, uint32* Hraw, uint8 core, uint8 sts,
		const bool single_core, bool tdcs_en, bool he_frm)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	h_tbl_info_t h_tbl_info;
	int err = BCME_OK;
	uint8 classifier_ctrl;
	uint32 *Hpacked, *Hpacked_buf;
	bool suspend = FALSE;

	if (!pi->sh->clk) {
		return BCME_NOCLK;
	}

	suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}

	/* Check for a valid and supported FFT length value */
	if ((fft_len != TOF_NFFT_20MHZ) && (fft_len != TOF_NFFT_40MHZ) &&
			(fft_len != TOF_NFFT_80MHZ) && (fft_len != TOF_NFFT_160MHZ)) {
		err = BCME_ERROR;
		goto fail;
	}

	/* Check if PHY is deaf or not */
	classifier_ctrl = READ_PHYREG(pi, ClassifierCtrl) & 0x3;
	if (classifier_ctrl != 0) {
		PHY_ERROR(("ClassifierCtrl[1:0] : 0x%x\n", classifier_ctrl));
		err = BCME_ERROR;
		goto fail;
	}

	if (!swap_pn_half) {
		H = ((cint32 *)tofi->chan + offset);
		ASSERT(H != NULL);
	}
	Hpacked = (uint32*)H;
	Hpacked_buf = (uint32*)H + fft_len;

	if (single_core) {
		h_tbl_info.core = tofi->tof_core;
		h_tbl_info.nsts = 0;
	} else {
		h_tbl_info.core = core;
		h_tbl_info.nsts = sts;
	}
	if (tdcs_en) {
		h_tbl_info.chansmooth = TRUE;
	} else {
		h_tbl_info.chansmooth = FALSE;
	}
	/* Get the table details */
	phy_ac_tof_get_chan_tbl_info(pi, &h_tbl_info, fft_len, he_frm);
	/* Read the channel */
	err =  phy_ac_tof_get_chan(pi, Hpacked, &h_tbl_info, fft_len, he_frm);
	if (err != BCME_OK) {
		goto fail;
	}

#ifdef TOF_DBG
	/* store raw data for log collection */
#if defined(K_TOF_COLLECT_HRAW_SIZE_20MHZ) && defined(K_TOF_COLLECT_HRAW_SIZE_80MHZ)
	int collect_hraw_size = 0;
	if (CHSPEC_IS20(pi->radio_chanspec)) {
		collect_hraw_size = K_TOF_COLLECT_HRAW_SIZE_20MHZ;
	} else {
		collect_hraw_size = K_TOF_COLLECT_HRAW_SIZE_80MHZ;
	}
	if (Hraw && (fft_len <= collect_hraw_size)) {
		bcopy((void*)Hpacked, (void*)Hraw, fft_len * sizeof(uint32));
	}
#else
	if (Hraw) {
		bcopy((void*)Hpacked, (void*)Hraw, fft_len * sizeof(uint32));
	}
#endif  /* defined(K_TOF_COLLECT_HRAW_SIZE_20MHZ) && defined(K_TOF_COLLECT_HRAW_SIZE_80MHZ) */
#endif /* TOF_DBG */

	phy_ac_tof_reorder_tones(Hpacked, Hpacked_buf, swap_pn_half, tdcs_en, fft_len);

	err = phy_ac_tof_unpack_chan(pi, Hpacked_buf, H, nbits, tdcs_en, fft_len);
	if (err != BCME_OK) {
		goto fail;
	}

fail:
	if (!suspend) {
		wlapi_enable_mac(pi->sh->physhim);
	}
	return err;
}

/* Get mag sqrd channel impulse response(from channel smoothing hw) to derive 11v rx timestamp */
static int
phy_ac_chan_mag_sqr_impulse_response(phy_type_tof_ctx_t *ctx, int frame_type,
	int len, int offset, int nbits, int32* h, int* pgd, uint32* hraw, uint16 tof_shm_ptr)
{
	return BCME_UNSUPPORTED;
}

/* get rxed frame type, bandwidth and rssi value */
static int phy_ac_tof_info(phy_type_tof_ctx_t *ctx, wlc_phy_tof_info_t *tof_info,
	wlc_phy_tof_info_type_t tof_info_mask, uint8 core)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	uint16 status0, status1, status2, status5, freq_est;
	uint16 subband_shift;
	int frame_bw, cfo, coarse_fo, fine_fo;
	bool suspend;

#ifdef WL_PROXD_PHYTS
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phytsi->core = core;
	phytsi->frame_type = tof_info->frame_type;

	if (tof_info_mask == WLC_PHY_TOF_INFO_TYPE_NONE) {
		return BCME_OK;
	}
#endif
	if (!pi->sh->clk) {
		return BCME_NOCLK;
	}

	suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}

	status0 = (int)READ_PHYREG(pi, RxStatus0) & 0xffff;
	status1 = (int)READ_PHYREG(pi, RxStatus1);
	status2 = (int)READ_PHYREG(pi, RxStatus2);
	status5 = (int)READ_PHYREG(pi, RxStatus5);
	freq_est = (int)READ_PHYREG(pi, PhyStatsFreqEst);

	BCM_REFERENCE(status2);
	BCM_REFERENCE(status5);

	/* Update frame type */
	if (tof_info_mask & WLC_PHY_TOF_INFO_TYPE_FRAME_TYPE) {
		if (ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
			tof_info->frame_type = status0 & PRXS0_FT_MASK_GE128;
		} else {
			tof_info->frame_type = status0 & PRXS0_ACPHY_FT_MASK;
		}
		tof_info->info_mask |= WLC_PHY_TOF_INFO_TYPE_FRAME_TYPE;
	}
	/* Update frame bandwidth and subband location */
	if (tof_info_mask & WLC_PHY_TOF_INFO_TYPE_FRAME_BW) {
		if (ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
			frame_bw = status1 & PRXS1_ACPHY_SUBBAND_MASK_GEN2;
			subband_shift = PRXS1_ACPHY_SUBBAND_SHIFT_GEN2;
		} else {
			frame_bw = status0 & PRXS0_ACPHY_SUBBAND_MASK;
			subband_shift = PRXS0_ACPHY_SUBBAND_SHIFT;
		}
		/* Packing frame_bw as: [SUBBAND(16) BW_IDX(16)] */
		if (frame_bw == (PRXS_SUBBAND_160 << subband_shift)) {
			frame_bw = TOF_BW_160MHZ_INDEX_V2 | (frame_bw << 16);
		} else if ((frame_bw == (PRXS_SUBBAND_80L << subband_shift)) ||
			(frame_bw == (PRXS_SUBBAND_80U << subband_shift))) {
			frame_bw = TOF_BW_80MHZ_INDEX_V2 | (frame_bw << 16);
		} else if ((frame_bw == (PRXS_SUBBAND_40LL << subband_shift)) ||
			(frame_bw == (PRXS_SUBBAND_40LU << subband_shift)) ||
			(frame_bw == (PRXS_SUBBAND_40UL << subband_shift)) ||
			(frame_bw == (PRXS_SUBBAND_40UU << subband_shift))) {
			frame_bw = TOF_BW_40MHZ_INDEX_V2 | (frame_bw << 16);
		} else {
			frame_bw = TOF_BW_20MHZ_INDEX_V2 | (frame_bw << 16);
		}
		tof_info->frame_bw = frame_bw;
		tof_info->info_mask |= WLC_PHY_TOF_INFO_TYPE_FRAME_BW;
	}

	if (tof_info_mask & WLC_PHY_TOF_INFO_TYPE_RSSI) {
		tof_info->rssi = (wl_proxd_rssi_t)phy_rssi_get_rssi(pi, core);
		tof_info->info_mask |= WLC_PHY_TOF_INFO_TYPE_RSSI;
	}

	if (tof_info_mask & WLC_PHY_TOF_INFO_TYPE_CFO) {
		fine_fo = ((int)freq_est & 0xff);
		coarse_fo = ((int)(freq_est >> 8) & 0xff);
		if (coarse_fo > 127)
			coarse_fo -= 256;
		if (fine_fo > 127)
			fine_fo -= 256;

		cfo = coarse_fo + fine_fo;
		//tofi->tof_cur_cfo = cfo;
		tof_info->cfo = cfo;
		tof_info->info_mask |= WLC_PHY_TOF_INFO_TYPE_CFO;
	}

	if (tof_info_mask & WLC_PHY_TOF_INFO_TYPE_SNR) {
		tof_info->snr = tofi->snr;
		tof_info->info_mask |= WLC_PHY_TOF_INFO_TYPE_SNR;
	}

	if (tof_info_mask & WLC_PHY_TOF_INFO_TYPE_BITFLIPS) {
		tof_info->bitflips = tofi->bitflips;
		tof_info->info_mask |= WLC_PHY_TOF_INFO_TYPE_BITFLIPS;
	}

	if (tof_info_mask & WLC_PHY_TOF_INFO_TYPE_PHYERROR) {
		tof_info->tof_phy_error = tofi->tof_phy_error;
		tof_info->info_mask |= WLC_PHY_TOF_INFO_TYPE_PHYERROR;
	}
	if (!suspend) {
		wlapi_enable_mac(pi->sh->physhim);
	}
	return BCME_OK;
}

/* turn on classification to receive frames */
static void phy_ac_tof_cmd(phy_type_tof_ctx_t *ctx, bool seq, int emu_delay)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	bool suspend;

	if (!pi->sh->clk) {
		return;
	}

	suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	if (!suspend) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}
#ifdef WL_PROXD_SEQ
	if (seq) {
		uint16 tof_seq_fem_gains[2], mask = (1 << tofi->tof_core);
		uint32 sc_stop = K_TOF_SEQ_SC_STOP;
		tofi->emu_delay = emu_delay;
		/* extend the sample capture buffer to accomodate the emulator delay */
		if (TOF_EB(emu_delay)) {
		  sc_stop += TOF_BUF_EXT;
		}
		MOD_PHYREG(pi, RfseqCoreActv2059, EnTx, mask);
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			tof_seq_fem_gains[0] = k_tof_seq_fem_gains_2g[0] | mask;
			tof_seq_fem_gains[1] = k_tof_seq_fem_gains_2g[1] | mask;
		}
		else {
			tof_seq_fem_gains[0] = k_tof_seq_fem_gains[0] | mask;
			tof_seq_fem_gains[1] = k_tof_seq_fem_gains[1] | mask;
		}
		wlc_tof_seq_write_shm_acphy(pi, 1, K_TOF_SEQ_SHM_FEM_RADIO_HI_GAIN_OFFSET,
			(uint16*)tof_seq_fem_gains);
		wlc_tof_seq_write_shm_acphy(pi, 1, K_TOF_SEQ_SHM_FEM_RADIO_LO_GAIN_OFFSET,
			(uint16*)tof_seq_fem_gains + 1);
		phy_ac_tof_sc(pi, TRUE, K_TOF_SEQ_SC_START, sc_stop, 0);
	}
#endif /* WL_PROXD_SEQ */

	phy_rxgcrs_sel_classifier(pi, CHSPEC_IS2G(pi->radio_chanspec) ?
			TOF_CLASSIFIER_BPHY_ON_OFDM_ON : TOF_CLASSIFIER_BPHY_OFF_OFDM_ON);

	wlc_phy_resetcca_acphy(pi);

	if (!suspend) {
		wlapi_enable_mac(pi->sh->physhim);
	}
}

/* get 6G K value for initiator or target  */
static const uint8*
phy_ac_tof_kvalue_tables_6g(phy_info_t *pi, phy_ac_tof_info_t *tofi,
	chanspec_t chanspec, int32* ki, int32* kt)
{
	uint8 const *kvalueptr = NULL;

	/* Base K value for 6G HE FTM and ACK */
	if (CHSPEC_IS160(chanspec)) {
		*ki = tofi->proxd_k_6g[0];
		*kt = tofi->proxd_k_6g[0];
		kvalueptr = tofi->proxd_6g_160m_kval_off;
	} else if (CHSPEC_IS80(chanspec)) {
		*ki = tofi->proxd_k_6g[1];
		*kt = tofi->proxd_k_6g[1];
		kvalueptr = tofi->proxd_6g_80m_kval_off;
	} else if (CHSPEC_IS40(chanspec)) {
		*ki = tofi->proxd_k_6g[2];
		*kt = tofi->proxd_k_6g[2];
		kvalueptr = tofi->proxd_6g_40m_kval_off;
	} else if (CHSPEC_IS20(chanspec)) {
		*ki = tofi->proxd_k_6g[3];
		*kt = tofi->proxd_k_6g[3];
		kvalueptr = tofi->proxd_6g_20m_kval_off;
	}

	return kvalueptr;
}

/* get K value for initiator or target  */
static
const uint32 *phy_ac_tof_kvalue_tables(phy_info_t *pi, phy_ac_tof_info_t *tofi,
chanspec_t chanspec, int32* ki, int32* kt, int32* kseq)
{
	uint32 const *kvalueptr = NULL;

	/* Base K value (VHT FTM, VHT ACK) */
	if (CHSPEC_IS160(chanspec)) {
		*ki = tofi->proxd_ki_5g_160;
		*kt = tofi->proxd_kt_5g_160;
		kvalueptr = tofi->proxd_160m_k_values;
	} else if (CHSPEC_IS80(chanspec)) {
		*ki = tofi->proxd_ki[0];
		*kt = tofi->proxd_kt[0];
		kvalueptr = tofi->proxd_80m_k_values;
	} else if (CHSPEC_IS40(chanspec)) {
		*ki = tofi->proxd_ki[1];
		*kt = tofi->proxd_kt[1];
		kvalueptr = tofi->proxd_40m_k_values;
	} else if (CHSPEC_IS20_5G(chanspec)) {
		*ki = tofi->proxd_ki[2];
		*kt = tofi->proxd_kt[2];
		kvalueptr = tofi->proxd_20m_k_values;
	} else {
		*ki = tofi->proxd_ki[3];
		*kt = tofi->proxd_kt[3];
		kvalueptr = tofi->proxd_2g_k_values;
	}

	return kvalueptr;
}

#ifdef WL_PROXD_PHYTS
static uint32
phy_ac_tof_phyts_kvalue(phy_ac_tof_phyts_info_t *phytsi, chanspec_t chanspec)
{
	uint8 idx = 0u;
	uint32 kval_ps = 0u, channel = CHSPEC_CHANNEL(chanspec);

	if (CHSPEC_IS6G(chanspec) && CHSPEC_IS80(chanspec)) {
		idx = (channel - 1) >> 4;
		kval_ps = phytsi->phyts_kval_6g80m[idx];
	} else if (CHSPEC_IS5G(chanspec) && CHSPEC_IS80(chanspec)) {
		if (channel <= 58u) {
			idx = (channel - 42u) >> 4u;
		} else if (channel <= 138u) {
			idx = ((channel - 106u) >> 4u) + 2u;
		} else {
			idx = 5u;
		}
		kval_ps = phytsi->phyts_kval_5g80m[idx];
	} else if (CHSPEC_IS5G(chanspec) && CHSPEC_IS40(chanspec)) {
		if (channel <= 62) {
			idx = (channel - 38) >> 3;
		} else if (channel <= 142) {
			idx = ((channel - 102) >> 3) + 4;
		} else {
			idx = ((channel - 151) >> 3) + 10;
		}
		kval_ps = phytsi->phyts_kval_5g40m[idx];

	} else if (CHSPEC_IS20_5G(chanspec)) {
		if (channel <= 64) {
			idx = (channel - 36) >> 2;
		} else if (channel <= 144) {
			idx = ((channel - 100) >> 2) + 8;
		} else {
			idx = ((channel - 149) >> 2) + 20;
		}
		kval_ps = phytsi->phyts_kval_5g20m[idx];
	} else {
		kval_ps =0; /* kvalue not supported */
	}
	/* TBU - Support for other BWs */

	return kval_ps;
}
#endif /* WL_PROXD_PHYTS */

static int phy_ac_tof_kvalue(phy_type_tof_ctx_t *ctx, chanspec_t chanspec, uint32 rspecidx,
	uint32 *kip, uint32 *ktp, uint8 flag)
{
	uint32 const *kvaluep = NULL;
	uint8 const *kvaluep_6g = NULL;
	int idx = 0, array_idx = 0, channel = CHSPEC_CHANNEL(chanspec);
	int32 ki = 0, kt = 0, kseq = 0;
	int rtt_adj = 0, rtt_adj_ts, irate_adj = 0, iack_adj = 0, trate_adj = 0, tack_adj = 0;
	int rtt_adj_papd = 0, papd_en = 0;
	int rtt_adj_ts_array[4] = {25, 40, 80, 80};
	int rtt_adj_papd_array[4] = {25, 30, 66, 0 /* 70 */};
	bool suspend;
	uint8 bwidx;

	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	if (!pi->sh->clk) {
		return BCME_NOCLK;
	}
#ifdef CALIBRATION
	if (kip) {
		*kip = 0;
		return BCME_OK;
	}
	if (ktp) {
		*ktp = 0;
		return BCME_OK;
	}
#endif /* CALIBRATION */

#ifdef WL_PROXD_PHYTS
	/* Only VHT-FTM and VHT-ACK is supported */
	/* Adding support for 80MHz - TBU for others */
	if (flag & WL_PROXD_PHTS_MASK) {
		uint32 kval = 0u;
		kval = phy_ac_tof_phyts_kvalue(tofi->phytsi, chanspec);
		if (kip) {
			*kip = kval;
		}
		if (ktp) {
			*ktp = kval;
		}
		return BCME_OK;
	}
#endif /* WL_PROXD_PHYTS */

	suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);

	if (CHSPEC_IS6G(chanspec)) {
		kvaluep_6g = phy_ac_tof_kvalue_tables_6g(pi, tofi, chanspec, &ki, &kt);
	} else {
		kvaluep = phy_ac_tof_kvalue_tables(pi, tofi, chanspec, &ki, &kt, &kseq);
	}
	if (flag & WL_PROXD_SEQEN) {
		if (kip) {
			*kip = kseq;
		}
		if (ktp) {
			*ktp = kseq;
		}
		return BCME_OK;
	}
	/* Sub-band Idx for Init and Resp */
	bwidx = flag & WL_PROXD_BW_MASK;

	if ((CHSPEC_IS6G(chanspec) && kvaluep_6g)) {
		if (CHSPEC_IS160(chanspec)) {
			idx	= (channel - 1) >> 5;
		} else if (CHSPEC_IS80(chanspec)) {
			idx	= (channel - 1) >> 4;
		} else if (CHSPEC_IS40(chanspec)) {
			idx	= (channel - 1) >> 3;
		} else {
			idx	= (channel - 1) >> 2;
		}
		/* Adjust offset across channels */
		if (kip) {
			*kip = (uint32)(ki + (int8)(kvaluep_6g[idx]));
		}
		if (ktp) {
			*ktp = (uint32)(kt + (int8)(kvaluep_6g[idx]));
		}
		PHY_TRACE(("kvaluep[%d] 0x%04x\n", idx, kvaluep_6g[idx]));
	} else if (kvaluep) {
		/* VHT = -1, legacy6M = 0, legacy = 1, mcs0 = 2, mcs = 3 */
		int8 rateidx, ackidx;
		rateidx = (rspecidx & WL_RSPEC_FTMIDX_MASK) >> WL_RSPEC_FTMIDX_SHIFT;
		rateidx--;
		ackidx = (rspecidx & WL_RSPEC_ACKIDX_MASK) >> WL_RSPEC_ACKIDX_SHIFT;
		ackidx--;

		if (!suspend) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
		}
		/* To check constant value */
		rtt_adj = (4 - READ_PHYREGFLD(pi, RxFeCtrl1, rxfe_bilge_cnt));
		if (READ_PHYREGFLD(pi, PapdEnable0, papd_compEnb0)) {
			papd_en = 1;
		}
		if (!suspend) {
			wlapi_enable_mac(pi->sh->physhim);
		}

		if (CHSPEC_IS160(chanspec)) {
			if (channel == 50) {
				idx = 0;
			} else {
				idx = 1;
			}
			if (rateidx != -1) {
				irate_adj = tofi->proxdi_rate_offset_160m[rateidx];
			}
			if (ackidx != -1) {
				trate_adj = tofi->proxdt_rate_offset_160m[ackidx];
			}
		} else if (CHSPEC_IS80(chanspec)) {
			if (channel <= 58) {
				idx = (channel - 42) >> 4;
			} else if (channel <= 138) {
				idx = ((channel - 106) >> 4) + 2;
			} else {
				idx = 5;
			}
			array_idx = 0;
			if (ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
				if (rateidx != -1) {
					irate_adj = tofi->proxdi_rate_offset_80m[rateidx];
				}
				if (ackidx != -1) {
					trate_adj = tofi->proxdt_rate_offset_80m[ackidx];
				}
			} else {
				if (rateidx != -1) {
					irate_adj = tofi->proxdi_rate_offset_80m[rateidx];
					trate_adj = tofi->proxdt_rate_offset_80m[rateidx];
				}
				if (ackidx != -1) {
					iack_adj = tofi->proxdi_ack_offset[0];
					tack_adj = tofi->proxdt_ack_offset[0];
				}
			}
		} else if (CHSPEC_IS40(chanspec)) {
			if (channel <= 62) {
				idx = (channel - 38) >> 3;
			} else if (channel <= 142) {
				idx = ((channel - 102) >> 3) + 4;
			} else {
				idx = ((channel - 151) >> 3) + 10;
			}
			array_idx = 1;
			if (ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
				if (rateidx != -1) {
					irate_adj = tofi->proxdi_rate_offset_40m[rateidx];
				}
				if (ackidx != -1) {
					trate_adj = tofi->proxdt_rate_offset_40m[ackidx];
				}
			} else {
				if (rateidx != -1) {
					irate_adj = tofi->proxdi_rate_offset_40m[rateidx];
					trate_adj = tofi->proxdt_rate_offset_40m[rateidx];
				}
				if (ackidx != -1) {
					iack_adj = tofi->proxdi_ack_offset[1];
					tack_adj = tofi->proxdt_ack_offset[1];
				}
			}
		} else if (CHSPEC_IS20_5G(chanspec)) {
			/* 5G 20M Hz channels */
			if (channel <= 64) {
				idx = (channel - 36) >> 2;
			} else if (channel <= 144) {
				idx = ((channel - 100) >> 2) + 8;
			} else {
				idx = ((channel - 149) >> 2) + 20;
			}
			array_idx = 2;
			if (ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
				if (rateidx != -1) {
					irate_adj = tofi->proxdi_rate_offset_20m[rateidx];
				}
				if (ackidx != -1) {
					trate_adj = tofi->proxdt_rate_offset_20m[ackidx];
				}
			} else {
				if (rateidx != -1) {
					irate_adj = tofi->proxdi_rate_offset_20m[rateidx];
					trate_adj = tofi->proxdt_rate_offset_20m[rateidx];
				}
				if (ackidx != -1) {
					iack_adj = tofi->proxdi_ack_offset[2];
					tack_adj = tofi->proxdt_ack_offset[2];
				}
			}
		} else if (channel >= 1 && channel <= 14) {
			/* 2G channels */
			idx = channel - 1;
			array_idx = 3;
			if (ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
				if (rateidx != -1) {
					irate_adj = tofi->proxdi_rate_offset_2g[rateidx];
				}
				if (ackidx != -1) {
					trate_adj = tofi->proxdt_rate_offset_2g[ackidx];
				}
			} else {
				if (rateidx != -1) {
					irate_adj = tofi->proxdi_rate_offset_2g[rateidx];
					trate_adj = tofi->proxdt_rate_offset_2g[rateidx];
				}
				if (ackidx != -1) {
					iack_adj = tofi->proxdi_ack_offset[3];
					tack_adj = tofi->proxdt_ack_offset[3];
				}
			}
		}
		if (papd_en) {
			rtt_adj_papd = rtt_adj_papd_array[array_idx];
		}
		rtt_adj_ts = rtt_adj_ts_array[array_idx];
		rtt_adj = (rtt_adj_ts * rtt_adj) >> K_TOF_K_RTT_ADJ_Q;
		PHY_TRACE(("init kt %d, ki %d\n", kt, ki));
		ki += ((int32)rtt_adj + (int32)rtt_adj_papd - irate_adj - iack_adj);
		kt += ((int32)rtt_adj + (int32)rtt_adj_papd - trate_adj - tack_adj);
		PHY_TRACE(("pre-bwidx kt %d, ki %d\n", kt, ki));
		if (bwidx) {
			if (ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
				if (CHSPEC_IS160(chanspec)) {
					/* To FIX: update the mapping of bw-idx/rateidx/ackidx */
					/* for 160MHz */
					kt -= tofi->proxd_subbw_offset_160m[bwidx - 4][ackidx + 1];
					ki -= tofi->proxd_subbw_offset_160m[bwidx - 4][rateidx + 1];
				} else {
					kt -= tofi->proxd_subbw_offset[bwidx - 1][ackidx + 1];
					ki -= tofi->proxd_subbw_offset[bwidx - 1][rateidx + 1];
				}
			} else {
				kt -= tofi->proxd_subbw_offset[bwidx - 1][rateidx + 1];
			}
		}
		PHY_TRACE(("rtt_adj %d, rtt_adj_papd %d, ackidx %d, rateidx %d, bwidx %d\n",
			rtt_adj, rtt_adj_papd, ackidx, rateidx, bwidx));
		PHY_TRACE(("ki %d, irate_adj %d, iack_adj %d\n", ki, irate_adj, iack_adj));
		PHY_TRACE(("kt %d, trate_adj %d, tack_adj %d\n", kt, trate_adj, tack_adj));
		PHY_TRACE(("kvaluep[%d] 0x%04x\n", idx, kvaluep[idx]));
		if (kip) {
			*kip = (uint32)(ki + (int16)(kvaluep[idx] & 0xffff));
		}
		if (ktp) {
			*ktp = (uint32)(kt + (int16)(kvaluep[idx] >> 16));
		}

		return BCME_OK;
	}

	return BCME_ERROR;
}

#ifdef WL_PROXD_SEQ
int
phy_ac_tof_set_ri_rr(phy_type_tof_ctx_t *ctx, const uint8 *ri_rr, const uint16 len,
		const uint8 core, const bool isInitiator, const bool isSecure,
		wlc_phy_tof_secure_2_0_t secure_params)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	uint16 i = 0;
	uint16 loop_len, ri_phy_len, rr_phy_len;
	int ret = BCME_OK;
	uint8 tmp = 0;
	bool loadSPB;
	uint8 *r_ptr = (isInitiator) ? tofi->rtx : tofi->rrx;
	uint8 align_boundary = 4;
	uint8 *tmp_rtx;
	uint8 *tmp_rrx;
	uint32 alloc_size_rtx, alloc_size_rrx;

	BCM_REFERENCE(secure_params);

	if ((len != FTM_TPK_RI_RR_LEN) && (len != FTM_TPK_RI_RR_LEN_SECURE_2_0)) {
		return BCME_BADLEN;
	}

	tofi->flag_sec_2_0 = (len == FTM_TPK_RI_RR_LEN_SECURE_2_0) ? TRUE : FALSE;
	PHY_ERROR(("%s: secure ranging 2.0 Flag : %d\n", __FUNCTION__, tofi->flag_sec_2_0));

	loop_len = tofi->flag_sec_2_0 ? len : (len+1);
	ri_phy_len = tofi->flag_sec_2_0 ? FTM_TPK_RI_PHY_LEN_SECURE_2_0 : FTM_TPK_RI_PHY_LEN;
	rr_phy_len = tofi->flag_sec_2_0 ? FTM_TPK_RR_PHY_LEN_SECURE_2_0 : FTM_TPK_RR_PHY_LEN;

	if (tofi->flag_sec_2_0) {
		tofi->start_seq_time = secure_params.start_seq_time;
		tofi->delta_time_tx2rx = secure_params.delta_time_tx2rx;
	}
#ifdef TOF_DBG_SEQ
	PHY_ERROR(("%s: start_seq_time = %d, delta_time_tx2rx = %d\n",
		__FUNCTION__, tofi->start_seq_time, tofi->delta_time_tx2rx));
#endif

	if (isSecure) {
		for (i = 0; i < loop_len; i++) {
			if (!tofi->flag_sec_2_0) {
				if (i < (FTM_TPK_RI_PHY_LEN - 1)) {
					tmp = *(ri_rr + i) & 0xff;
				} else if (i == (FTM_TPK_RI_PHY_LEN - 1)) {
					tmp = *(ri_rr + i) & 0xf;
				} else if (i < FTM_TPK_RI_RR_LEN) {
					tmp = (((*(ri_rr + i - 1) >> 4) & 0xf) |
						(*(ri_rr + i) << 4)) & 0xff;
				} else {
					tmp = ((*(ri_rr + i - 1) >> 4) & 0xf);
				}
			} else {
				tmp = *(ri_rr + i) & 0xff;
			}
			if (*r_ptr != tmp) {
				*r_ptr = tmp;
			}
			if (i == (ri_phy_len - 1)) {
				r_ptr = (isInitiator) ? tofi->rrx : tofi->rtx;
			} else {
				r_ptr++;
			}
		}
	}

	alloc_size_rtx = ri_phy_len * sizeof(uint8);
	alloc_size_rtx = ALIGN_SIZE(alloc_size_rtx, align_boundary);
	if ((tmp_rtx = phy_malloc(pi, alloc_size_rtx)) == NULL) {
			PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
			return BCME_NOMEM;
	}
	alloc_size_rrx = rr_phy_len * sizeof(uint8);
	alloc_size_rrx = ALIGN_SIZE(alloc_size_rrx, align_boundary);
	if ((tmp_rrx = phy_malloc(pi, alloc_size_rrx)) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		if (tmp_rtx != NULL) {
			phy_mfree(pi, tmp_rtx, alloc_size_rtx);
		}
		return BCME_NOMEM;
	}

	if (isSecure) {
		/*
		* Scramble FTM provided random bit sequences
		*/
		PHY_INFORM(("rtx = ["));
		for (i = 0; i < ri_phy_len; i++) {
			PHY_INFORM(("0x%x, ", tofi->rtx[i]));
		}
		PHY_INFORM(("];\n"));
		phy_ac_tof_gen_scrambled_output(tofi->rtx, tofi->rrx,
			((8*len) >> 1), tmp_rtx, tofi->flag_sec_2_0);
		PHY_INFORM(("tmp_rtx = ["));
		for (i = 0; i < ri_phy_len; i++) {
			PHY_INFORM(("0x%x, ", tmp_rtx[i]));
		}
		PHY_INFORM(("];\n"));
		PHY_INFORM(("rrx = ["));
		for (i = 0; i < rr_phy_len; i++) {
			PHY_INFORM(("0x%x, ", tofi->rrx[i]));
		}
		PHY_INFORM(("];\n"));

		phy_ac_tof_gen_scrambled_output(tofi->rrx, tofi->rtx,
			((8*len) >> 1), tmp_rrx, tofi->flag_sec_2_0);
		PHY_INFORM(("tmp_rrx = ["));
		for (i = 0; i < rr_phy_len; i++) {
			PHY_INFORM(("0x%x, ", tmp_rrx[i]));
		}
		PHY_INFORM(("];\n"));
		/* tof_seq_spb_len and tof_seq_spb has already been assigned and
		 * allocated memory during call to wlc_phy_tof_seq_params_acphy
		 */
		GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST();
		tofi->tof_seq_spb_tx = (uint32 *)k_tof_seq_spb_tx;
		tofi->tof_seq_spb_rx = (uint32 *)k_tof_seq_spb_rx;
		GCC_DIAGNOSTIC_POP();

		phy_ac_tof_bytes_to_spb(tmp_rtx, 2 * tofi->tof_seq_spb_len,
			tofi->tof_seq_spb_tx, tofi->flag_sec_2_0);
		phy_ac_tof_bytes_to_spb(tmp_rrx, 2 * tofi->tof_seq_spb_len,
			tofi->tof_seq_spb_rx, tofi->flag_sec_2_0);

		/*
		PHY_INFORM(("Tx : \n"));
		for(i = 0; i < 2 * aci->tof_seq_spb_len; i++) {
			PHY_INFORM(("0x%08x\n", tofi->tof_seq_spb_tx[i]));
		}
		PHY_INFORM(("\n"));
		PHY_INFORM(("Rx : \n"));
		for(i = 0; i < 2*aci->tof_seq_spb_len; i++) {
			PHY_INFORM(("0x%08x\n", tofi->tof_seq_spb_rx[i]));
		}
		PHY_INFORM(("\n"));
		*/
		for (i = 0; i < len; i++) {
			tofi->ri_rr[i] = *(ri_rr + i) & 0xff;
		}
	} else {
		for (i = 0; i < len; i++) {
			tofi->ri_rr[i] = 0;
		}
	}
	PHY_INFORM(("%s: PHY_RI_RR:\n", __FUNCTION__));
	for (i = 0; i < len; i++) {
		PHY_INFORM(("%x\t", *(ri_rr + i) & 0xff));
	}
	PHY_INFORM(("\n"));
	if (tmp_rtx != NULL) {
		phy_mfree(pi, tmp_rtx, alloc_size_rtx);
	}
	if (tmp_rrx != NULL) {
		phy_mfree(pi, tmp_rrx, alloc_size_rrx);
	}

#ifdef K_TOF_LOAD_SPB_ONLY
	loadSPB = TRUE;
#else
	loadSPB = FALSE;
#endif /* K_TOF_LOAD_SPB_ONLY */
	ret = phy_ac_tof_seq_setup(pi, FALSE, FALSE, core, isInitiator, loadSPB,
		FALSE);
	tofi->tof_phy_error = 0;
	return ret;
}

int
phy_ac_tof_set_ri_rr_spb_only(phy_info_t *pi, const bool isInitiator, const bool macSuspend)
{
	/*
	 * isInitiator value is warning if TOF_TEST_TONE is not defined or if loadSPB is FALSE.
	*/

	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	bool suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	uint16 tof_seq_n = (1 << tofi->tof_seq_log2_n);

#ifdef TOF_TEST_TONE
	uint16 i = 0;
	const int32 v[16] = { 96, 89, 68, 37, 0, -37, -68, -89, -96, -89, -68, -37, 0, 37,
		68, 89 };
	const int32 x[16] = { 96, 68, 0, -68, -96, -68, 0, 68, 96, 	68, 0, -68, -96, -68,
		0, 68 };
#endif /* TOF_TEST_TONE */

	/* Acquire the memory buffer, make sure the correct buffer size is registered during attach
	 */
	cint32 *pSeq = phy_cache_acquire_reuse_buffer(pi->cachei, tof_seq_n * sizeof(*pSeq));

	if ((macSuspend) && (!suspend)) {
		wlapi_suspend_mac_and_wait(pi->sh->physhim);
	}
	/*
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);
	*/

	/* Setup sample play */
	phy_ac_tof_mf(pi, tof_seq_n, pSeq, TRUE, TRUE, 0, 0, 0,
		K_TOF_SEQ_IN_SCALE, K_TOF_SEQ_OUT_SCALE, K_TOF_SEQ_OUT_SHIFT,
		K_TOF_BITFLIP_TH_DEFAULT, K_TOF_SNR_TH_DEFAULT, 0);

#ifdef TOF_TEST_TONE
	for (i = 0; i < tof_seq_n; i++) {
		if (isInitiator) {
			pSeq[i].i = v[i & 0xf];
			pSeq[i].q = v[(i - 4) & 0xf];
		} else {
			pSeq[i].i = x[i & 0xf];
			pSeq[i].q = x[(i - 4) & 0xf];
		}
	}
#endif /* TOF_TEST_TONE */

	wlc_phy_loadsampletable_acphy(pi, pSeq, tof_seq_n, TRUE);
	PHY_INFORM(("SETUP CORE %d\n", 0));
	/* Release the memory buffer */
	phy_cache_release_reuse_buffer(pi->cachei, pSeq);

	/*
	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	*/
	if ((macSuspend) && (!suspend)) {
		wlapi_enable_mac(pi->sh->physhim);
	}

	return BCME_OK;
}

static int
phy_ac_tof_seq_params(phy_type_tof_ctx_t *ctx, bool assign_buffer)
{
	/*
	In case we are running secure ranging (ranging sequence is genrated out of ri_rr every
	session), this function should be called with assign_buffer = TRUE only before FTM1.
	After FTM1 when we have finished set_ri_rr function call, this should never be called
	with assign_buffer = TRUE otherwise ri_rr will never go in effect.
	*/

	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	if (assign_buffer) {
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			tofi->tof_sc_FS = TOF_SC_FS_80MHZ;
#ifdef TOF_SEQ_20_IN_80MHz
			tofi->tof_seq_log2_n = K_TOF_SEQ_LOG2_N_80MHZ_20;
			tofi->tof_seq_spb_len = K_TOF_SEQ_SPB_LEN_20MHZ;

			GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST();
			tofi->tof_seq_spb_tx = (uint32 *)k_tof_seq_spb_20MHz;
			tofi->tof_seq_spb_rx = (uint32 *)k_tof_seq_spb_20MHz;
			GCC_DIAGNOSTIC_POP();
#else
			tofi->tof_seq_log2_n = K_TOF_SEQ_LOG2_N_80MHZ;
			tofi->tof_seq_spb_len = K_TOF_SEQ_SPB_LEN_80MHZ;

			GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST();
			tofi->tof_seq_spb_tx = (uint32 *)k_tof_seq_spb_80MHz;
			tofi->tof_seq_spb_rx = (uint32 *)k_tof_seq_spb_80MHz;
			GCC_DIAGNOSTIC_POP();

#endif /* TOF_SEQ_20_IN_80MHz */

			return BCME_OK;
		}

#if defined(TOF_SEQ_40MHz_BW) || defined(TOF_SEQ_40_IN_40MHz)
		else if (CHSPEC_IS40(pi->radio_chanspec)) {
			tofi->tof_sc_FS = TOF_SC_FS_40MHZ;
			tofi->tof_seq_log2_n = K_TOF_SEQ_LOG2_N_40MHZ;

#ifdef TOF_SEQ_40_IN_40MHz
			tofi->tof_seq_spb_len = K_TOF_SEQ_SPB_LEN_40MHZ;

			GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST();
			tofi->tof_seq_spb_tx = (uint32 *)k_tof_seq_spb_40MHz;
			tofi->tof_seq_spb_rx = (uint32 *)k_tof_seq_spb_40MHz;
			GCC_DIAGNOSTIC_POP();

#else
			tofi->tof_seq_spb_len = K_TOF_SEQ_SPB_LEN_20MHZ;

			GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST();
			tofi->tof_seq_spb_tx = (uint32 *)k_tof_seq_spb_20MHz;
			tofi->tof_seq_spb_rx = (uint32 *)k_tof_seq_spb_20MHz;
			GCC_DIAGNOSTIC_POP();

#endif /* TOF_SEQ_40_IN_40MHz */

			return BCME_OK;
		}
#endif /* defined(TOF_SEQ_40MHz_BW) || defined (TOF_SEQ_40_IN_40MHz) */

#if defined(TOF_SEQ_20MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT)
		else if (CHSPEC_IS2G(pi->radio_chanspec)) {
			tofi->tof_sc_FS = TOF_SC_FS_20MHZ;
			tofi->tof_seq_log2_n = K_TOF_SEQ_LOG2_N_20MHZ;
			tofi->tof_seq_spb_len = K_TOF_SEQ_SPB_LEN_20MHZ;

			GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST();
			tofi->tof_seq_spb_tx = (uint32 *)k_tof_seq_spb_20MHz;
			tofi->tof_seq_spb_rx = (uint32 *)k_tof_seq_spb_20MHz;
			GCC_DIAGNOSTIC_POP();

			return BCME_OK;
		} else {
			tofi->tof_sc_FS = TOF_SC_FS_20MHZ;
			tofi->tof_seq_log2_n = K_TOF_SEQ_LOG2_N_20MHZ;
			tofi->tof_seq_spb_len = K_TOF_SEQ_SPB_LEN_20MHZ;

			GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST();
			tofi->tof_seq_spb_tx = (uint32 *)k_tof_seq_spb_20MHz;
			tofi->tof_seq_spb_rx = (uint32 *)k_tof_seq_spb_20MHz;
			GCC_DIAGNOSTIC_POP();

			return BCME_OK;
		}
#endif /* defined(TOF_SEQ_20MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT) */
	}
	if (CHSPEC_IS80(pi->radio_chanspec)) {
#ifdef TOF_SEQ_20_IN_80MHz
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_80MHz_20, sizeof(uint16)*10);
#else
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_80MHz, sizeof(uint16)*10);
#endif
		return BCME_OK;
	}
#if defined(TOF_SEQ_40MHz_BW) || defined(TOF_SEQ_40_IN_40MHz)
	else if (CHSPEC_IS40(pi->radio_chanspec)) {
#ifdef TOF_SEQ_40_IN_40MHz
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_40MHz_40, sizeof(uint16)*10);
#else
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_40MHz, sizeof(uint16)*10);
#endif
		return BCME_OK;

	}
#endif /* defined(TOF_SEQ_40MHz_BW) || defined (TOF_SEQ_40_IN_40MHz) */

#if defined(TOF_SEQ_20MHz_BW) || defined(TOF_SEQ_20MHz_BW_512IFFT)
	else if (CHSPEC_IS2G(pi->radio_chanspec)) {
		if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
			memcpy(tofi->tof_ucode_dlys_us, K_TOF_UCODE_DLYS_US_2G_20MHZ_4360,
				sizeof(uint16)*K_TOF_SEQ_LOG3_N_20MHZ);
		} else {
			if (tofi->flag_sec_2_0) {
				PHY_ERROR(("%s: SECURE RANGING 2.0 params", __FUNCTION__));
				memcpy(tofi->tof_ucode_dlys_us, K_TOF_UCODE_DLYS_US_2G_20MHZ_2_0,
					sizeof(uint16)* K_TOF_SEQ_LOG3_N_20MHZ);
			} else {
				PHY_ERROR(("%s: SECURE RANGING 1.0 params", __FUNCTION__));
				memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_2g_20MHz,
					sizeof(uint16)* K_TOF_SEQ_LOG3_N_20MHZ);
			}
		}
		tofi->tof_ucode_dlys_us[0][3] += TOF_W_ADJ_EMU(tofi->emu_delay, tofi->tof_sc_FS);
		tofi->tof_ucode_dlys_us[0][4] += TOF_W_ADJ_EMU(tofi->emu_delay, tofi->tof_sc_FS);
		tofi->tof_ucode_dlys_us[1][4] += TOF_W_ADJ_EMU(tofi->emu_delay, tofi->tof_sc_FS);
		/* extent the Rx mode for 8 usec if emulator box is used */
		if (TOF_EB(tofi->emu_delay)) {
		  tofi->tof_ucode_dlys_us[0][1] += TOF_RX_MODE_EXT;
		  tofi->tof_ucode_dlys_us[1][2] += TOF_RX_MODE_EXT;
		}
		return BCME_OK;
	}
	else {
		memcpy(tofi->tof_ucode_dlys_us, k_tof_ucode_dlys_us_2g_20MHz, sizeof(uint16)*10);
		tofi->tof_ucode_dlys_us[0][3] += TOF_W_ADJ_EMU(tofi->emu_delay, tofi->tof_sc_FS);
		tofi->tof_ucode_dlys_us[0][4] += TOF_W_ADJ_EMU(tofi->emu_delay, tofi->tof_sc_FS);
		tofi->tof_ucode_dlys_us[1][4] += TOF_W_ADJ_EMU(tofi->emu_delay, tofi->tof_sc_FS);
		/* extend the Rx mode for 8 usec if emulator box is used */
		if (TOF_EB(tofi->emu_delay)) {
		  tofi->tof_ucode_dlys_us[0][1] += TOF_RX_MODE_EXT;
		  tofi->tof_ucode_dlys_us[1][2] += TOF_RX_MODE_EXT;
		}
		return BCME_OK;

	}
#endif /* TOF_SEQ_20MHz_BW || TOF_SEQ_20MHz_BW_512IFFT */

	return BCME_ERROR;
}
#endif /* WL_PROXD_SEQ */

#define NUM_BW_MODES_SUPP 4
#define NUM_6G_160MHZ_CHAN 7
#define NUM_6G_80MHZ_CHAN 14
#define NUM_6G_40MHZ_CHAN 29
#define NUM_6G_LOW_20MHZ_CHAN 30
#define NUM_6G_HIGH_20MHZ_CHAN 29
static void
BCMATTACHFN(phy_ac_nvram_proxd_read_6g)(phy_info_t *pi, phy_ac_tof_info_t *tofi)
{
	uint8 i;
	uint8 const *kvalueptr;
#ifdef WL_PROXD_PHYTS
	if (PHY_GETVAR(pi, rstr_phyts_6g80_kval)) {
		for (i = 0; i < NUM_6G_80MHZ_CHAN; i++) {
			tofi->phytsi->phyts_kval_6g80m[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_phyts_6g80_kval, i);
		}
	}
#endif /* WL_PROXD_PHYTS */

	if (PHY_GETVAR(pi, rstr_proxd_basekval_6g)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_k_6g); i++) {
			tofi->proxd_k_6g[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_basekval_6g, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_k_6g[0] = TOF_6G_K_43684_160M;
			tofi->proxd_k_6g[1] = TOF_6G_K_43684_80M;
			tofi->proxd_k_6g[2] = TOF_6G_K_43684_40M;
			tofi->proxd_k_6g[3] = TOF_6G_K_43684_20M;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxd_k_6g[0] = TOF_6G_K_47622_160M;
			tofi->proxd_k_6g[1] = TOF_6G_K_47622_80M;
			tofi->proxd_k_6g[2] = TOF_6G_K_47622_40M;
			tofi->proxd_k_6g[3] = TOF_6G_K_47622_20M;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_k_6g[0] = TOF_6G_K_43693_160M;
			tofi->proxd_k_6g[1] = TOF_6G_K_43693_80M;
			tofi->proxd_k_6g[2] = TOF_6G_K_43693_40M;
			tofi->proxd_k_6g[3] = TOF_6G_K_43693_20M;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_k_6g[0] = TOF_6G_K_43794_160M;
			tofi->proxd_k_6g[1] = TOF_6G_K_43794_80M;
			tofi->proxd_k_6g[2] = TOF_6G_K_43794_40M;
			tofi->proxd_k_6g[3] = TOF_6G_K_43794_20M;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_k_6g[0] = TOF_6G_K_47623_160M;
			tofi->proxd_k_6g[1] = TOF_6G_K_47623_80M;
			tofi->proxd_k_6g[2] = TOF_6G_K_47623_40M;
			tofi->proxd_k_6g[3] = TOF_6G_K_47623_20M;
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_6g_160mkval_offset)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_6g_160m_kval_off); i++) {
			tofi->proxd_6g_160m_kval_off[i] = (uint8)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_6g_160mkval_offset, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_6g160m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_6g160m_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_6g160m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_6g160m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_6g160m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_6g_160m_kval_off); i++) {
			tofi->proxd_6g_160m_kval_off[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_6g_80mkval_offset)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_6g_80m_kval_off); i++) {
			tofi->proxd_6g_80m_kval_off[i] = (uint8)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_6g_80mkval_offset, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_6g80m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_6g80m_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_6g80m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_6g80m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_6g80m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_6g_80m_kval_off); i++) {
			tofi->proxd_6g_80m_kval_off[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_6g_40mkval_offset)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_6g_40m_kval_off); i++) {
			tofi->proxd_6g_40m_kval_off[i] = (uint8)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_6g_40mkval_offset, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_6g40m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_6g40m_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_6g40m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_6g40m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_6g40m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_6g_40m_kval_off); i++) {
			tofi->proxd_6g_40m_kval_off[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_6g_20mkval_offset)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_6g_20m_kval_off); i++) {
			tofi->proxd_6g_20m_kval_off[i] = (uint8)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_6g_20mkval_offset, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_6g20m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_6g20m_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_6g20m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_6g20m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_6g20m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_6g_20m_kval_off); i++) {
			tofi->proxd_6g_20m_kval_off[i] = kvalueptr[i];
		}
	}
}

static void
BCMATTACHFN(phy_ac_nvram_proxd_read)(phy_info_t *pi, phy_ac_tof_info_t *tofi)
{
	uint8 i;
	uint32 const *kvalueptr;

#ifdef WL_PROXD_PHYTS
	if (PHY_GETVAR(pi, rstr_phyts_5g80_kval)) {
		for (i = 0; i < ARRAYSIZE(tofi->phytsi->phyts_kval_5g80m); i++) {
			tofi->phytsi->phyts_kval_5g80m[i] = PHY_GETINTVAR_ARRAY(pi,
				rstr_phyts_5g80_kval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_phyts_5g80m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_phyts_5g80m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_phyts_5g80m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_phyts_5g80m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->phytsi->phyts_kval_5g80m); i++) {
			tofi->phytsi->phyts_kval_5g80m[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_phyts_5g40_kval)) {
		for (i = 0; i < ARRAYSIZE(tofi->phytsi->phyts_kval_5g40m); i++) {
			tofi->phytsi->phyts_kval_5g40m[i] = PHY_GETINTVAR_ARRAY(pi,
				rstr_phyts_5g40_kval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_phyts_5g40m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_phyts_5g40m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_phyts_5g40m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_phyts_5g40m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->phytsi->phyts_kval_5g40m); i++) {
			tofi->phytsi->phyts_kval_5g40m[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_phyts_5g20_kval)) {
		for (i = 0; i < ARRAYSIZE(tofi->phytsi->phyts_kval_5g20m); i++) {
			tofi->phytsi->phyts_kval_5g20m[i] = PHY_GETINTVAR_ARRAY(pi,
				rstr_phyts_5g20_kval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_phyts_5g20m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_phyts_5g20m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_phyts_5g20m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_phyts_5g20m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->phytsi->phyts_kval_5g20m); i++) {
			tofi->phytsi->phyts_kval_5g20m[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_phyts_2g20_kval)) {
		for (i = 0; i < ARRAYSIZE(tofi->phytsi->phyts_kval_2g20m); i++) {
			tofi->phytsi->phyts_kval_2g20m[i] = PHY_GETINTVAR_ARRAY(pi,
				rstr_phyts_2g20_kval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_phyts_2g20m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_phyts_2g20m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_phyts_2g20m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_phyts_2g20m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->phytsi->phyts_kval_2g20m); i++) {
			tofi->phytsi->phyts_kval_2g20m[i] = kvalueptr[i];
		}
	}
#endif /* WL_PROXD_PHYTS */

	if (PHY_GETVAR(pi, rstr_proxd_basekival_5g_160)) {
		tofi->proxd_ki_5g_160 = (uint16)PHY_GETINTVAR_DEFAULT(pi,
			rstr_proxd_basekival_5g_160, 0);
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_ki_5g_160 = TOF_INITIATOR_K_43684_160M;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_ki_5g_160 = TOF_INITIATOR_K_43693_160M;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_ki_5g_160 = TOF_INITIATOR_K_43794_160M;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_ki_5g_160 = TOF_INITIATOR_K_47623_160M;
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_basektval_5g_160)) {
		tofi->proxd_kt_5g_160 = (uint16)PHY_GETINTVAR_DEFAULT(pi,
			rstr_proxd_basektval_5g_160, 0);
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_kt_5g_160 = TOF_TARGET_K_43684_160M;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_kt_5g_160 = TOF_TARGET_K_43693_160M;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_kt_5g_160 = TOF_TARGET_K_43794_160M;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_kt_5g_160 = TOF_TARGET_K_47623_160M;
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_basekival)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_ki); i++) {
			tofi->proxd_ki[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_basekival, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_ki[0] = TOF_INITIATOR_K_43684_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_43684_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_43684_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_43684_2G;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxd_ki[0] = TOF_INITIATOR_K_47622_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_47622_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_47622_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_47622_2G;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_ki[0] = TOF_INITIATOR_K_43693_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_43693_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_43693_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_43693_2G;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_ki[0] = TOF_INITIATOR_K_43794_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_43794_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_43794_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_43794_2G;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_ki[0] = TOF_INITIATOR_K_47623_80M;
			tofi->proxd_ki[1] = TOF_INITIATOR_K_47623_40M;
			tofi->proxd_ki[2] = TOF_INITIATOR_K_47623_20M;
			tofi->proxd_ki[3] = TOF_INITIATOR_K_47623_2G;
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_basektval)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_kt); i++) {
			tofi->proxd_kt[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_basektval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_kt[0] = TOF_TARGET_K_43684_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_43684_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_43684_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_43684_2G;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxd_kt[0] = TOF_TARGET_K_47622_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_47622_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_47622_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_47622_2G;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_kt[0] = TOF_TARGET_K_43693_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_43693_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_43693_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_43693_2G;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_kt[0] = TOF_TARGET_K_43794_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_43794_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_43794_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_43794_2G;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_kt[0] = TOF_TARGET_K_47623_80M;
			tofi->proxd_kt[1] = TOF_TARGET_K_47623_40M;
			tofi->proxd_kt[2] = TOF_TARGET_K_47623_20M;
			tofi->proxd_kt[3] = TOF_TARGET_K_47623_2G;
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_160mkval)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_160m_k_values); i++) {
			tofi->proxd_160m_k_values[i] = (uint16)PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_160mkval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_160m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_160m_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_160m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_160m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_160m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_160m_k_values); i++) {
			tofi->proxd_160m_k_values[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_80mkval)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_80m_k_values); i++) {
			tofi->proxd_80m_k_values[i] = PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_80mkval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_80m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_80m_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_80m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_80m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_80m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_80m_k_values); i++) {
			tofi->proxd_80m_k_values[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_40mkval)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_40m_k_values); i++) {
			tofi->proxd_40m_k_values[i] = PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_40mkval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_40m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_40m_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_40m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_40m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_40m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_40m_k_values); i++) {
			tofi->proxd_40m_k_values[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_20mkval)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_20m_k_values); i++) {
			tofi->proxd_20m_k_values[i] = PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_20mkval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_20m_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_20m_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_20m_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_20m_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_20m_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_20m_k_values); i++) {
			tofi->proxd_20m_k_values[i] = kvalueptr[i];
		}
	}

	if (PHY_GETVAR(pi, rstr_proxd_2gkval)) {
		for (i = 0; i < ARRAYSIZE(tofi->proxd_2g_k_values); i++) {
			tofi->proxd_2g_k_values[i] = PHY_GETINTVAR_ARRAY(pi,
				rstr_proxd_2gkval, i);
		}
	} else {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43684_2g_k_values;
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47622_2g_k_values;
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43693_2g_k_values;
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_43794_2g_k_values;
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			kvalueptr = proxd_47623_2g_k_values;
		} else {
			return;
		}
		for (i = 0; i < ARRAYSIZE(tofi->proxd_2g_k_values); i++) {
			tofi->proxd_2g_k_values[i] = kvalueptr[i];
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdi_rate_offset_160m); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate160m, i, proxdi_rate_offset_160m_43684[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate160m, i, proxdi_rate_offset_160m_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate160m, i, proxdi_rate_offset_160m_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate160m, i, proxdi_rate_offset_160m_47623[i]);
		} else {
			tofi->proxdi_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate160m, i, 0);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdi_rate_offset_80m); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate80m, i, proxdi_rate_offset_80m_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate80m, i, proxdi_rate_offset_80m_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate80m, i, proxdi_rate_offset_80m_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate80m, i, proxdi_rate_offset_80m_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate80m, i, proxdi_rate_offset_80m_47623[i]);
		} else {
			tofi->proxdi_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate80m, i, proxdi_rate_offset_80m[i]);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdi_rate_offset_40m); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate40m, i, proxdi_rate_offset_40m_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate40m, i, proxdi_rate_offset_40m_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate40m, i, proxdi_rate_offset_40m_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate40m, i, proxdi_rate_offset_40m_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate40m, i, proxdi_rate_offset_40m_47623[i]);
		} else {
			tofi->proxdi_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate40m, i, proxdi_rate_offset_40m[i]);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdi_rate_offset_20m); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate20m, i, proxdi_rate_offset_20m_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate20m, i, proxdi_rate_offset_20m_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate20m, i, proxdi_rate_offset_20m_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate20m, i, proxdi_rate_offset_20m_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate20m, i, proxdi_rate_offset_20m_47623[i]);
		} else {
			tofi->proxdi_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate20m, i, proxdi_rate_offset_20m[i]);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdi_rate_offset_2g); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate2g, i, proxdi_rate_offset_2g_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate2g, i, proxdi_rate_offset_2g_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate2g, i, proxdi_rate_offset_2g_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate2g, i, proxdi_rate_offset_2g_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdi_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate2g, i, proxdi_rate_offset_2g_47623[i]);
		} else {
			tofi->proxdi_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdi_rate2g, i, proxdi_rate_offset_2g[i]);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdt_rate_offset_160m); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate160m, i, proxdt_rate_offset_160m_43684[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate160m, i, proxdt_rate_offset_160m_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate160m, i, proxdt_rate_offset_160m_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_160m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate160m, i, proxdt_rate_offset_160m_47623[i]);
		} else {
			tofi->proxdt_rate_offset_160m[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdt_rate160m, i, 0);
		}
	}
	for (i = 0; i < ARRAYSIZE(proxdt_rate_offset_80m); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate80m, i, proxdt_rate_offset_80m_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate80m, i, proxdt_rate_offset_80m_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate80m, i, proxdt_rate_offset_80m_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate80m, i, proxdt_rate_offset_80m_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate80m, i, proxdt_rate_offset_80m_47623[i]);
		} else {
			tofi->proxdt_rate_offset_80m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate80m, i, proxdt_rate_offset_80m[i]);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdt_rate_offset_40m); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate40m, i, proxdt_rate_offset_40m_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate40m, i, proxdt_rate_offset_40m_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate40m, i, proxdt_rate_offset_40m_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate40m, i, proxdt_rate_offset_40m_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate40m, i, proxdt_rate_offset_40m_47623[i]);
		} else {
			tofi->proxdt_rate_offset_40m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate40m, i, proxdt_rate_offset_40m[i]);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdt_rate_offset_20m); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate20m, i, proxdt_rate_offset_20m_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate20m, i, proxdt_rate_offset_20m_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate20m, i, proxdt_rate_offset_20m_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate20m, i, proxdt_rate_offset_20m_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate20m, i, proxdt_rate_offset_20m_47623[i]);
		} else {
			tofi->proxdt_rate_offset_20m[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate20m, i, proxdt_rate_offset_20m[i]);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdt_rate_offset_2g); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate2g, i, proxdt_rate_offset_2g_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate2g, i, proxdt_rate_offset_2g_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate2g, i, proxdt_rate_offset_2g_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate2g, i, proxdt_rate_offset_2g_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdt_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate2g, i, proxdt_rate_offset_2g_47623[i]);
		} else {
			tofi->proxdt_rate_offset_2g[i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_rate2g, i, proxdt_rate_offset_2g[i]);
		}
	}

	for (i = 0; i < ARRAYSIZE(proxdi_ack_offset); i++) {
		tofi->proxdi_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxdi_ack, i, proxdi_ack_offset[i]);
	}

	for (i = 0; i < ARRAYSIZE(proxdt_ack_offset); i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxdt_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_ack, i, proxdt_ack_offset_43684[i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxdt_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_ack, i, proxdt_ack_offset_47622[i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxdt_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_ack, i, proxdt_ack_offset_43693[i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxdt_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_ack, i, proxdt_ack_offset_43794[i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxdt_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_ack, i, proxdt_ack_offset_47623[i]);
		} else {
			tofi->proxdt_ack_offset[i] = (int16)PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxdt_ack, i, proxdt_ack_offset[i]);
		}
	}

	for (i = 0; i < 5; i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m80m, i, proxd_subbw_offset_160m_43684[0][i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m80m, i, proxd_subbw_offset_160m_43693[0][i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m80m, i, proxd_subbw_offset_160m_43794[0][i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m80m, i, proxd_subbw_offset_160m_47623[0][i]);
		} else {
			tofi->proxd_subbw_offset_160m[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m80m, i, 0);
		}
	}

	for (i = 0; i < 5; i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m40m, i, proxd_subbw_offset_160m_43684[1][i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m40m, i, proxd_subbw_offset_160m_43693[1][i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m40m, i, proxd_subbw_offset_160m_43794[1][i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m40m, i, proxd_subbw_offset_160m_47623[1][i]);
		} else {
			tofi->proxd_subbw_offset_160m[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxd_sub160m40m, i, 0);
		}
	}

	for (i = 0; i < 5; i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m20m, i, proxd_subbw_offset_160m_43684[2][i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m20m, i, proxd_subbw_offset_160m_43693[2][i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m20m, i, proxd_subbw_offset_160m_43794[2][i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset_160m[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub160m20m, i, proxd_subbw_offset_160m_47623[2][i]);
		} else {
			tofi->proxd_subbw_offset_160m[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
			rstr_proxd_sub160m20m, i, 0);
		}
	}

	for (i = 0; i < 5; i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m40m, i, proxd_subbw_offset_43684[0][i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m40m, i, proxd_subbw_offset_47622[0][i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m40m, i, proxd_subbw_offset_43693[0][i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m40m, i, proxd_subbw_offset_43794[0][i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m40m, i, proxd_subbw_offset_47623[0][i]);
		} else {
			tofi->proxd_subbw_offset[0][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m40m, i, proxd_subbw_offset[0][i]);
		}
	}

	for (i = 0; i < 5; i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m20m, i, proxd_subbw_offset_43684[1][i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m20m, i, proxd_subbw_offset_47622[1][i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m20m, i, proxd_subbw_offset_43693[1][i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m20m, i, proxd_subbw_offset_43794[1][i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m20m, i, proxd_subbw_offset_47623[1][i]);
		} else {
			tofi->proxd_subbw_offset[1][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub80m20m, i, proxd_subbw_offset[1][i]);
		}
	}

	for (i = 0; i < 5; i++) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub40m20m, i, proxd_subbw_offset_43684[2][i]);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub40m20m, i, proxd_subbw_offset_47622[2][i]);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub40m20m, i, proxd_subbw_offset_43693[2][i]);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub40m20m, i, proxd_subbw_offset_43794[2][i]);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			tofi->proxd_subbw_offset[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub40m20m, i, proxd_subbw_offset_47623[2][i]);
		} else {
			tofi->proxd_subbw_offset[2][i] = PHY_GETINTVAR_ARRAY_DEFAULT(pi,
				rstr_proxd_sub40m20m, i, proxd_subbw_offset[2][i]);
		}
	}

}

#ifdef WL_PROXD_SEQ
int
phy_ac_tof_seq_params_get_set(phy_type_tof_ctx_t *ctx, uint8 *delays,
	bool set, bool tx, int size)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	uint8 *tmp = delays;
	int i;
	int ret = BCME_OK;

	if (tofi->tof_ucode_dlys_us[0][0] == 0) {
	  if ((ret = phy_ac_tof_seq_params(ctx, TRUE)) != BCME_OK) {
			PHY_ERROR(("%s: phy_tof_seq_params_get_set() returned err %d\n",
				__FUNCTION__, ret));
			return ret;
		}
	}
	if (size < TOF_UCODE_DLYS_MIN_LEN)
		return BCME_BADLEN;

	if (!set) {
		for (i = 0; i < TOF_UCODE_DLYS_MIN_LEN; i++) {
			*tmp = tofi->tof_ucode_dlys_us[(tx ? 1 : 0)][i];
			tmp++;
		}
		return BCME_OK;
	} else {
		/* NEED TO ADD LOGIC to Update the Params Received from the Remote */
		return BCME_OK;
	}
}

static void
phy_ac_tof_init_gdmm_th(phy_type_tof_ctx_t *ctx, int32 *gdmm_th)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	BCM_REFERENCE(pi);
	if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
		(ACMINORREV_1(pi) || ACMINORREV_4(pi))) {
		*gdmm_th = K_TOF_GDMM_TH_4350;
	} else if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
		*gdmm_th = K_TOF_GDMM_TH_4360;
	} else {
		*gdmm_th = K_TOF_GDMM_TH_43602;
	}
}

static void
phy_ac_tof_init_gdv_th(phy_type_tof_ctx_t *ctx, uint32 *gdv_th)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;

	BCM_REFERENCE(pi);
	if (ACMAJORREV_2(pi->pubpi->phy_rev) &&
		(ACMINORREV_1(pi) || ACMINORREV_4(pi))) {
		*gdv_th = K_TOF_GDV_TH_4350;
	} else {
		*gdv_th = K_TOF_GDV_TH_4360_43602;
	}
}
void phy_ac_tof_gen_scrambled_output(const uint8* In1, const uint8* In2, const uint16 len,
	uint8 *Out, bool flag_sec_2_0)
{
	uint16 mask4 = 0x8, mask7 = 0x40, Rshift4 = 3, Rshift7 = 6, i = 0, j = 0, imax;
	uint8 byte_in, byte_out;
	uint8 state_mask = 0x7f, mask1 = 0x1, bit, x7 = 0, x4 = 0;
	uint8 state;

	BCM_REFERENCE(*In2);

	if (flag_sec_2_0) {
		state = *In1 & state_mask;
	} else {
		state = (*In2 & (state_mask << 1)) >> 1;
	}
	imax = (len / 8) + 1;
	for (i = 0; i < imax; i++) {
		byte_in = *(In1 + i);
		byte_out = 0;
		for (j = 0; j < 8; j++) {
			if ((i*8 + j) >= len) {
				break;
			}
			x7 = (state & mask7) >> Rshift7;
			x4 = (state & mask4) >> Rshift4;
			bit = byte_in & mask1;
			PHY_INFORM(("state = %x, bit = %x, ",  state, bit));
			bit = bit ^ (x7 ^ x4);
			PHY_INFORM(("bit_out = %x\n", bit));
			byte_out = byte_out | (bit << j);
			state = (state << 1) & state_mask;
			state = state | (x7^x4);
			byte_in = byte_in >> 1;
		}
		*(Out + i) = byte_out;
	}
}

void
phy_ac_tof_bytes_to_spb(const uint8* In, const uint16 len, uint32* Out, bool flag_sec_2_0)
{
	/*
	* Assumes a 52 bit incoming sequence and maps it to 20MHz LTF SPB
	*/
	uint32 tmp[8] = { 0 };
	uint32 out = 0;
	uint8 in = *(In), nibble, bit;
	uint16 i = 0, j = 0, k = 0, idx, shift_ctr = 0;
	const uint16 nibble_shift = 4;
	const uint8 bits_in_byte = 8;
	uint8 pair;

	uint16 ltf_neg_mask;
	uint16 ltf_pos_mask;

	if (flag_sec_2_0) {
		ltf_neg_mask = K_TOF_LTF_MASK_NEG_2_0;
		ltf_pos_mask = K_TOF_LTF_MASK_POS_2_0;
	} else {
		ltf_neg_mask = K_TOF_LTF_MASK_NEG;
		ltf_pos_mask = K_TOF_LTF_MASK_POS;
	}

	PHY_INFORM(("%s ", __FUNCTION__));
	for (i = 0; i < 16; i++) {
		PHY_INFORM(("0x%2x ", *(In + i)));
	}
	PHY_INFORM(("\n"));

	for (i = 0; i < (2 * K_TOF_SEQ_SPB_LEN_20MHZ); i++) {
		out = 0;
		for (j = 0; j < bits_in_byte; j++) {
			idx = i*bits_in_byte + j;
			if ((idx == 0) || ((idx <= ltf_neg_mask) &&
				(idx >= ltf_pos_mask))) {
				nibble = 0x0;
			} else {
				if (shift_ctr == 0) {
					in = *(In + k);
					k++;
				}
				if (flag_sec_2_0) {
					pair = in & 0x3;
					switch (pair) {
					case 0x1:
						nibble = 0x1;
						break;
					case 0x2:
						nibble = 0xf;
						break;
					case 0x3:
						nibble = 0x2;
						break;
					case 0x0:
					default:
						nibble = 0xe;
					}
					in = (in >> 2);
					shift_ctr += 2;
				} else {
					bit = in & 0x1;
					nibble = (bit == 0x0 ? 0xf : 0x1);
					in = (in >> 1);
					shift_ctr++;
				}
				shift_ctr = shift_ctr % bits_in_byte;
			}
			out = out | (nibble << (nibble_shift*j));
		}
		*(tmp + i) = 0xffffffff & out;
	}
	j = (len == 2 * K_TOF_SEQ_SPB_LEN_20MHZ) ? 0 : 4;
	i = 0;
	while (i < len) {
		if (!flag_sec_2_0) {
			if ((len == 2 * K_TOF_SEQ_SPB_LEN_80MHZ) && (i < K_TOF_SEQ_SPB_LEN_80MHZ)) {
				Out[i] = (tmp[j] << 1) & 0xeeeeeeee;
			} else
				Out[i] = tmp[j];
		} else {
			Out[i] = tmp[j];
			PHY_INFORM(("0x%x\n", Out[i]));
		}
		j = j + 1;
		j = j % (2 * K_TOF_SEQ_SPB_LEN_20MHZ);
		i++;
	}
}

void phy_ac_tof_set_setup_done(phy_ac_tof_info_t *tofi, bool done)
{
	tofi->tof_setup_done = done;
}

uint16 phy_ac_tof_get_rfseq_bundle_offset(phy_ac_tof_info_t *tofi)
{
	return tofi->tof_rfseq_bundle_offset;
}
#endif /* WL_PROXD_SEQ */

#endif /* WL_PROXDETECT */

#ifdef WL_PROXD_PHYTS
static uint32
phy_ac_tof_read_currptr(phy_ac_tof_info_t *tofi)
{
	phy_info_t *pi = tofi->pi;
	uint32 curr_ptr_low, curr_ptr_high, curr_ptr;

	curr_ptr_low = R_REG(pi->sh->osh, D11_SCP_CURPTR(pi));
	curr_ptr_high = (R_REG(pi->sh->osh, D11_SCP_CURPTR_H(pi))) & 0xFu;
	curr_ptr = ((curr_ptr_high << 16u) | curr_ptr_low);

	return curr_ptr;
}
static int
phy_ac_tof_phyts_sampcap_enable(phy_ac_tof_info_t *tofi)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	int err = BCME_OK;
	PHY_TRACE(("%s:  \n", __FUNCTION__));

	/* Suspend MAC */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);

	err = phy_samp_capture_enable(pi, &phytsi->save_clkgatests,
		phytsi->sc_start, phytsi->sc_end);
	phytsi->sc_state |= SC_STATE_ENAB;

	/* Resume MAC */
	wlapi_enable_mac(pi->sh->physhim);

	return err;
}

static int
phy_ac_tof_phyts_sampcap_disable(phy_ac_tof_info_t *tofi)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	int err = BCME_OK;
	PHY_TRACE(("%s:  \n", __FUNCTION__));

	err = phy_samp_capture_disable(pi);
	phytsi->sc_state |= SC_STATE_DISB;

	return err;
}

/* BM memory allocations are done considering 1FS RXFILTER mode */
/* Hence, only 7s mode is supported */
#define SC_SINGLE_CORE_MODE 1u
static int
phy_ac_tof_phyts_sampcap_setup(phy_ac_tof_info_t *tofi)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	int err = BCME_OK;

	PHY_INFORM(("wl%d phy_ac_tof_phyts_sampcap_setup: Configuring sample capture on core %d\n",
		PI_SLICE(pi), phytsi->core));

	/* BM space must be pre-allocated to operate properly */
	wlapi_get_sc_info(pi->sh->physhim, &phytsi->sc_start, &phytsi->sc_sz);
	if (phytsi->sc_sz) {
		phytsi->sc_end = phytsi->sc_start + phytsi->sc_sz;
	} else {
		err = BCME_RANGE;
		PHY_ERROR(("wl%d phy_ac_tof_phyts_sampcap_setup: Zero buffer size\n",
			PI_SLICE(pi)));
		goto done;
	}

	PHY_INFORM(("wl%d phy_ac_tof_phyts_sampcap_setup: start 0x%x stop 0x%x sz 0x%x\n",
		PI_SLICE(pi), phytsi->sc_start, phytsi->sc_end, phytsi->sc_sz));

	/* Save Regs */
	phytsi->saved_adcdatacollect = READ_PHYREG(pi, AdcDataCollect);
	phytsi->saved_rxfetesmmuxctrl = READ_PHYREG(pi, RxFeTesMmuxCtrl);
	phytsi->saved_spectrumanalyzermode = READ_PHYREG(pi, SpectrumAnalyzerMode);
	phytsi->saved_specanadatacollect = READ_PHYREG(pi, SpecAnaDataCollect);

	ACPHY_REG_LIST_START
		WRITE_PHYREG_ENTRY(pi, AdcDataCollect, 0u)
		WRITE_PHYREG_ENTRY(pi, RxFeTesMmuxCtrl, 0u)
		WRITE_PHYREG_ENTRY(pi, SpectrumAnalyzerMode, 0u)
		WRITE_PHYREG_ENTRY(pi, SpecAnaDataCollect, 0u)
	ACPHY_REG_LIST_EXECUTE(pi);

	/*
	if (CHSPEC_IS160(pi->radio_chanspec)) {
		MOD_PHYREG(pi, SpecAnaDataCollect, legacy_samp_cap_128bit_mode, 1u);
	}
	*/
	MOD_PHYREG(pi, AdcDataCollect, rxSingleCoreMode, SC_SINGLE_CORE_MODE);
	MOD_PHYREG(pi, AdcDataCollect, sampSel, phytsi->sc_mode);
	MOD_PHYREG(pi, AdcDataCollect, rxCoreSel, phytsi->core);

	if (phytsi->sc_mode == SC_MODE_FARROW_OUTPUT) {
		/* PKT proc */
		MOD_PHYREG(pi, SpecAnaDataCollect, pktprocMuxEn, 1u);
		/* 4s mode config */
		MOD_PHYREG(pi, RxFeTesMmuxCtrl, rxfe_dbg_mux_sel, 4u);
		MOD_PHYREG(pi, RxFeTesMmuxCtrl, samp_coll_core_sel, phytsi->core);
	}

	phytsi->sc_state |= SC_STATE_SETUP;

done:
	return err;
}

static int
phy_ac_tof_phyts_sampcap_cleanup(phy_ac_tof_info_t *tofi)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;

	WRITE_PHYREG(pi, AdcDataCollect, phytsi->saved_adcdatacollect);
	WRITE_PHYREG(pi, RxFeTesMmuxCtrl, phytsi->saved_rxfetesmmuxctrl);
	WRITE_PHYREG(pi, SpectrumAnalyzerMode, phytsi->saved_spectrumanalyzermode);
	WRITE_PHYREG(pi, SpecAnaDataCollect, phytsi->saved_specanadatacollect);

	return BCME_OK;
}

static int
phy_ac_tof_phyts_bw_params(phy_ac_tof_info_t *tofi)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	int ret = BCME_OK;

	/* RxFilter 1FS captures are at same bw rate */
	if (CHSPEC_IS160(pi->radio_chanspec)) {
		phytsi->bw_mhz = PHYBW_160;
		phytsi->nfft = 512u;
	} else if (CHSPEC_IS80(pi->radio_chanspec)) {
		phytsi->bw_mhz = PHYBW_80;
		phytsi->nfft = 256u;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		phytsi->bw_mhz = PHYBW_40;
		phytsi->nfft = 128u;
	} else if (CHSPEC_IS20(pi->radio_chanspec)) {
		phytsi->bw_mhz = PHYBW_20;
		phytsi->nfft = 64u;
	} else {
		PHY_ERROR(("Slice%u: phy_ac_tof_phyts_bw_params: "
			"Unsupported BW\n", PI_SLICE(pi)));
		phytsi->bw_mhz = 0u;
		phytsi->nfft = 0u;
		ret = BCME_ERROR;
	}

	return ret;
}

static int
phy_ac_tof_phyts_read_cfo_iqcomp(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint16 freq_est;
	int coarse_fo, fine_fo;
	int ret = BCME_OK;

	/* Read CFO value */
	freq_est = (int)READ_PHYREG(pi, PhyStatsFreqEst);
	fine_fo = ((int)freq_est & 0xff);
	coarse_fo = ((int)(freq_est >> 8) & 0xff);
	if (coarse_fo > 127) {
		coarse_fo -= 256;
	}
	if (fine_fo > 127) {
		fine_fo -= 256;
	}
	phytsi->cfo = coarse_fo + fine_fo;

	if (phytsi->sc_mode == SC_MODE_FARROW_OUTPUT) {
		/* Read IQ comp */
		phytsi->rxiq_a = READ_PHYREGCE(pi, Core1RxIQCompA, phytsi->core);
		phytsi->rxiq_b = READ_PHYREGCE(pi, Core1RxIQCompB, phytsi->core);
		if (phytsi->rxiq_a > 511) {
			phytsi->rxiq_a -= 1024;
		}
		if (phytsi->rxiq_b > 511) {
			phytsi->rxiq_b -= 1024;
		}
	}
	return ret;
}

#define PHYTS_FRAMETYPE_VHT		3
#define PHYTS_FRAMETYPE_HE		4
#define PHYTS_LSIG_LENGTH_MASK		0xFFFu
#define PHYTS_LSIG_LENGTH_SHIFT		5u

static int
phy_ac_tof_phyts_param_setup(phy_ac_tof_info_t *tofi)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	int ret = BCME_OK;
	uint16 frame_type, hecpltf = 2;
	PHY_TRACE(("%s:  \n", __FUNCTION__));

	wlapi_suspend_mac_and_wait(pi->sh->physhim);

	/* Read LSIG length field */
	if (PHYTS_IS_INITIATOR(phytsi->role)) {
		uint32 lsig;

		/* FTM Rx */
		frame_type = READ_PHYREGFLD(pi, rxmacphy_debug_0, rxmacphy_rxframetype);
		lsig = ((READ_PHYREG(pi, rxmacphy_debug_2) >> 16) +
			READ_PHYREG(pi, rxmacphy_debug_1));
		phytsi->lsig_length = ((lsig >> PHYTS_LSIG_LENGTH_SHIFT) &
		                       PHYTS_LSIG_LENGTH_MASK);
		if (frame_type == PHYTS_FRAMETYPE_HE) {
			hecpltf = ((READ_PHYREG(pi, rxmacphy_debug_4) >> 5) & 0x3);
		}
	} else if (PHYTS_IS_TARGET(phytsi->role)) {
		uint64 tblval;
		uint16 txctl_len, word_offs, byte_offs;

		/* FTM Tx */
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_AXMACPHYIFTBL,
		                         1, 0, 64, &tblval);
		txctl_len = (tblval & 0xFFFF);
		frame_type = ((tblval >> 16) & 0x7);
		phytsi->frame_type = ((tblval >> 16) & 0x7);
		word_offs = ((txctl_len + 2) >> 3);
		byte_offs = ((txctl_len + 2) & 0x7);
		if (byte_offs > 0)
			word_offs = word_offs + 1;
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_AXMACPHYIFTBL,
		                         1, word_offs, 64, &tblval);
		phytsi->lsig_length = ((tblval >> PHYTS_LSIG_LENGTH_SHIFT) &
		                       PHYTS_LSIG_LENGTH_MASK);
		if (frame_type == PHYTS_FRAMETYPE_HE) {
			hecpltf = ((tblval >> 45) & 0x3);
		}
	} else {
		ret = BCME_UNSUPPORTED;
		goto done;
	}
	if (phytsi->frame_type != frame_type) {
		PHY_ERROR(("%s: frame_type = %d and phytsi->frame_type = %d mismatch! \n",
			__FUNCTION__, frame_type, phytsi->frame_type));
	}

	/* Only 1p6us cp and 2x HELTF supported for now */
	if ((frame_type == PHYTS_FRAMETYPE_HE) && (hecpltf != 2u)) {
		ret = BCME_UNSUPPORTED;
		goto done;
	}

	/* Read CFO and Rx IQ compensation */
	ret = phy_ac_tof_phyts_read_cfo_iqcomp(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* BW dependent params */
	ret = phy_ac_tof_phyts_bw_params(tofi);
	if (ret != BCME_OK) {
		goto done;
	}
done:
	wlapi_enable_mac(pi->sh->physhim);
	PHY_TRACE(("%s: %s lsig_length %d, frame_type %d, ret %d \n",
		__FUNCTION__, (phytsi->role == PHYTS_ROLE_INITIATOR) ? "Initiator" : "Responder",
		phytsi->lsig_length, phytsi->frame_type, ret));
	return ret;
}

/* 13 bits of I and Q samples (signed) are packed in 32 bits */
/* 29:16 -> I 12:0 -> Q */
#define SC_Q_MASK 0x00001FFF
#define SC_Q_SHIFT 0u
#define SC_I_MASK 0x1FFF0000
#define SC_I_SHIFT 16u
#define SC_N_BITS 13u
#define SC_IQ_MAX_VAL (1 << (SC_N_BITS - 1u)) - 1
#define SC_IQ_NEG (1 << SC_N_BITS)
#define SC_CHECK_IQ_VAL(x) (x > SC_IQ_MAX_VAL) ? (x - SC_IQ_NEG) : (x)

/* Find maximum FTM size, considering VHT-MCS0, NSS1, NGI Packet */
#define MAX_PAYLOAD_SIZE_OCTET 126u
#define VHT_PHY_HEADER_US 40u
#define OFDM_SYMB_US 4u
#define VHT_NBPS(bw) (26u * (bw/20u))
#define PAYLOAD_DURATION_US(bw) \
	(OFDM_SYMB_US * (((MAX_PAYLOAD_SIZE_OCTET * 8u)/VHT_NBPS(bw)) + 1u))
/* Read extra time to include silence time, PLCP decoding delays,etc */
#define READ_ADDITIONAL_DUR_US 30u
#define MAX_FTM_DUR_US(bw) \
	(READ_ADDITIONAL_DUR_US + VHT_PHY_HEADER_US + PAYLOAD_DURATION_US(bw))

static void
phy_ac_tof_phyts_sampcap_read_frm_bm(phy_ac_tof_info_t *tofi, uint32 sc_read_ptr, uint32 read_cnt,
	cint16 *sc_mem)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	uint32 sc_data, sc_idx, buf_idx = 0;

	/* Configure read pointer, pointer auto increments */
	W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_PTR(pi), (sc_read_ptr << 2u));

	for (sc_idx = 0u; sc_idx < read_cnt; sc_idx++) {
		sc_data = (uint32)R_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_DATA(pi));

		if (phytsi->sc_mode == SC_MODE_FARROW_OUTPUT) {
			/* Skip alternate samples */
			if ((sc_idx % 2) == 0u) {
				/* Unpack into I,Q format */
				/* For 4s mode, I is in lower 16, and Q is in higher 16 */
				sc_mem[buf_idx].q = (int16)((sc_data & SC_I_MASK) >> SC_I_SHIFT);
				sc_mem[buf_idx].q = SC_CHECK_IQ_VAL(sc_mem[buf_idx].q);
				sc_mem[buf_idx].i = (int16)((sc_data & SC_Q_MASK) >> SC_Q_SHIFT);
				sc_mem[buf_idx].i = SC_CHECK_IQ_VAL(sc_mem[buf_idx].i);
				buf_idx++;
			}
		} else {
			/* Unpack into I,Q format */
			/* For 7s mode, Q is packed in lower 16, and I is packet in higher 16 */
			sc_mem[sc_idx].i = (int16)((sc_data & SC_I_MASK) >> SC_I_SHIFT);
			sc_mem[sc_idx].i = SC_CHECK_IQ_VAL(sc_mem[sc_idx].i);
			sc_mem[sc_idx].q = (int16)((sc_data & SC_Q_MASK) >> SC_Q_SHIFT);
			sc_mem[sc_idx].q = SC_CHECK_IQ_VAL(sc_mem[sc_idx].q);
		}
		/* Wrap around condition check */
		if (sc_read_ptr == phytsi->sc_end) {
			sc_read_ptr = phytsi->sc_start;
			/* Reconfigure read pointer due to wrap around */
			W_REG(pi->sh->osh, D11_XMT_TEMPLATE_RW_PTR(pi), (sc_read_ptr << 2u));
		} else {
			sc_read_ptr++;
		}

		/* Read till the current ptr and then exit */
		if (sc_read_ptr == phytsi->sc_curr_ptr) {
			break;
		}
	}

	phytsi->sc_state |= SC_STATE_READ;
}

static int
phy_ac_tof_phyts_compute_ftm_pkt_start(phy_ac_tof_info_t *tofi)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	uint32 sc_ftm_start_tmp;
	int err = BCME_OK;
	PHY_TRACE(("%s:  \n", __FUNCTION__));

	/* Read the last updated sample-ptr in the memory */
	phytsi->sc_curr_ptr = phy_ac_tof_read_currptr(tofi);
	/* Ucode takes the snapshot of the cur-ptr in the FTM goodfcs path */
	/* Use this ptr to compute the start of the FTM start packet */
	phytsi->sc_ftm_end = wlapi_bmac_read_shm(pi->sh->physhim, M_FTM_SC_CURPTR(pi));

	phytsi->sc_ftm_dur = EXTR_NOISE_DUR_US + LEGACY_PREAMBLE_DUR_US;

	/* Read LSIG length field */
	if (PHYTS_IS_INITIATOR(phytsi->role) || PHYTS_IS_TARGET(phytsi->role)) {
		if (phytsi->frame_type == PHYTS_FRAMETYPE_VHT) { // VHT
			phytsi->sc_ftm_dur += ((phytsi->lsig_length + 3) / 3 * 4);
		} else { // HE
			phytsi->sc_ftm_dur += ((phytsi->lsig_length + 3 + 2) / 3 * 4);
		}
	} else {
		err = BCME_UNSUPPORTED;
	}

	/* Converting to count of i/q samples */
	phytsi->sc_ftm_dur *= phytsi->bw_mhz;

	if (phytsi->sc_mode == SC_MODE_FARROW_OUTPUT) {
		/* 4s is at 2fs */
		phytsi->sc_ftm_dur *= 2u;
	}

	sc_ftm_start_tmp = phytsi->sc_ftm_end - phytsi->sc_ftm_dur;
	/* If the duration calculation is with-in sample-capture boundry */
	if ((sc_ftm_start_tmp >= phytsi->sc_start) &&
		(sc_ftm_start_tmp < phytsi->sc_end)) {
		phytsi->sc_ftm_start = sc_ftm_start_tmp;
	} else {
		/* Due to wraping */
		phytsi->sc_ftm_start = (phytsi->sc_ftm_end + phytsi->sc_sz) -
			phytsi->sc_ftm_dur;
	}

	return err;
}

static uint32
phy_ac_tof_phyts_compute_read_ptr(phy_ac_tof_info_t *tofi, uint32 start_ptr, uint32 sc_off)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint32 read_ptr;
	uint32 read_ptr_tmp = start_ptr + sc_off;

	if (read_ptr_tmp > phytsi->sc_end) {
		read_ptr = read_ptr_tmp - phytsi->sc_sz - 1;
	} else {
		read_ptr = read_ptr_tmp;
	}

	return read_ptr;
}

static uint32
phy_ac_tof_phyts_compute_sc_buff_off(phy_ac_tof_info_t *tofi, uint8 read_mode)
{
	phy_info_t *pi = tofi->pi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint32 buf_off = 0u, ltf_off_ns = 0u;

	/* Read buff will have samples as below
	 * For 5G band:
	 * | 16us energy_det_samps | 0.1us gap | 3.2us FTM LLTF | 0.1us gap | 3.2us FTM VHTLTF |
	 * | 0.1us gap | 3.2us ACK LLTF | 0.1us gap | 3.2us ACK VHTLTF |
	 * For 6G band, supporting 2x HELTF:
	 * | 16us energy_det_samps | 0.1us gap | 3.2us FTM LLTF | 0.1us gap | 6.4us FTM HELTF |
	 * | 0.1us gap | 3.2us ACK LLTF | 0.1us gap | 6.4us ACK HELTF |
	 */

	if (read_mode == SC_READ_ENERGY_DETECT || read_mode == SC_READ_HEACK_ENERGY_DETECT) {
		buf_off = 0u;
	} else {
		buf_off = ENERGYDET_SC_READ_DUR_NS + (read_mode * SC_READ_BUF_GAP_DUR_NS);
		/* Offset for reading LTFs */
		switch (read_mode) {
			case SC_READ_ACK_HE_VHTLTF:
				ltf_off_ns += FFT_WINDOW_DUR_NS;
				/* fall through */
			case SC_READ_ACK_LLTF:
				ltf_off_ns += (CHSPEC_IS6G(pi->radio_chanspec)) ?
					(2u * FFT_WINDOW_DUR_NS) : (FFT_WINDOW_DUR_NS);
				/* fall through */
			case SC_READ_FTM_HE_VHTLTF:
				ltf_off_ns += FFT_WINDOW_DUR_NS;
				/* fall through */
			case SC_READ_FTM_LLTF:
				ltf_off_ns += 0u;
				break;
			default:
				ltf_off_ns = 0u;
		}
		buf_off += ltf_off_ns;
		buf_off = (buf_off * phytsi->bw_mhz) / 1000u;
	}

	return buf_off;
}

static int
phy_ac_tof_phyts_setup(phy_type_tof_ctx_t *ctx, bool setup, uint8 role)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	int ret = BCME_OK;
	bool suspended = FALSE;

	PHY_TRACE(("%s:  sc_state 0x%x, setup %d, role %d\n", __FUNCTION__,
		phytsi->sc_state, setup, role));

	if (!pi->sh->clk) {
		ret = BCME_NOCLK;
		goto done;
	}
	phytsi->sc_state = SC_STATE_NONE;

	/* Save role */
	phytsi->role = role;

	if (!setup) {
		if (phytsi->is_setup_called == FALSE) {
			/* cleanup called but nothing to cleanup, this is ok */
			goto done;
		}
	} else {
		if (phytsi->is_setup_called != FALSE) {
			/* setup called before cleanup */
			PHY_ERROR(("%s: ERROR: incoming setup %d, existing role %d, "
				"existing is_setup_called %d\n", __FUNCTION__,
				setup, phytsi->role, phytsi->is_setup_called));
			goto done;
		}

	}

	/* Suspend MAC */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	suspended = TRUE;

	if (setup) {
		phytsi->is_setup_called = TRUE;

		/* Setup loopback path */
		ret = phy_ac_tof_phyts_loopback(tofi, TRUE);
		if (ret != BCME_OK) {
			goto done;
		}
		/* Sample-capture setup */
		ret = phy_ac_tof_phyts_sampcap_setup(tofi);
		if (ret != BCME_OK) {
			goto done;
		}
		/* Enable sample-capture */
		ret = phy_ac_tof_phyts_sampcap_enable(tofi);
		if (ret != BCME_OK) {
			goto done;
		}
	} else {
		phytsi->is_setup_called = FALSE;

		/* Disable sample-capture */
		ret = phy_ac_tof_phyts_sampcap_disable(tofi);
		if (ret != BCME_OK) {
			goto done;
		}

		ret = phy_ac_tof_phyts_sampcap_cleanup(tofi);
		if (ret != BCME_OK) {
			goto done;
		}

		/* Cleanup loopback path */
		ret = phy_ac_tof_phyts_loopback(tofi, FALSE);
		if (ret != BCME_OK) {
			goto done;
		}
	}
done:
	/* Resume MAC */
	if (suspended) {
		wlapi_enable_mac(pi->sh->physhim);
	}

	return ret;
}

/* Junks in the beginning of HE initiator sample captures. Add offset to bypass them. */
#define PHYTS_SC_READ_BUF_HE_OFFSET(role, bw_mhz) ((role == PHYTS_ROLE_INITIATOR) ?\
	(5000u * bw_mhz)/1000u : 0u)

static int
phy_ac_tof_phyts_read_sampcap(phy_ac_tof_info_t *tofi, uint8 read_mode, uint32 sc_off,
	cint16 *sc_mem)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	uint32 sc_read_ptr, read_cnt;
	int ret = BCME_OK;

	/* Suspend MAC */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);

	if (read_mode == SC_READ_ENERGY_DETECT || read_mode == SC_READ_HEACK_ENERGY_DETECT ||
	    read_mode == SC_READ_FULL) {
		/* Explicitly disable sample-capture, though */
		/* Ucode also does it after FTM-ACK TX */
		ret = phy_ac_tof_phyts_sampcap_disable(tofi);
		if (ret != BCME_OK) {
			goto done;
		}

		/* No need to update params and recompute FTM pkt start for ACK energy detect */
		if (!(read_mode == SC_READ_HEACK_ENERGY_DETECT)) {
			/* Update common params */
			ret = phy_ac_tof_phyts_param_setup(tofi);
			if (ret != BCME_OK) {
				goto done;
			}

			/* Compute FTM pkt start offset */
			ret = phy_ac_tof_phyts_compute_ftm_pkt_start(tofi);
			if (ret != BCME_OK) {
				goto done;
			}
		}

		/* Enable mac clock gating */
		phy_ac_enab_mac_clkgate_sc(pi, phytsi->save_clkgatests);

		if (read_mode == SC_READ_ENERGY_DETECT) {
			read_cnt = ENERGYDET_SC_READ_LEN(phytsi->bw_mhz);
			if (phytsi->frame_type == PHYTS_FRAMETYPE_HE) {
				read_cnt +=
					PHYTS_SC_READ_BUF_HE_OFFSET(phytsi->role, phytsi->bw_mhz);
			}
		} else if (read_mode == SC_READ_HEACK_ENERGY_DETECT) {
			read_cnt = ENERGYDET_SC_READ_LEN(phytsi->bw_mhz);
		} else {
			/* Read full sample capture */
			read_cnt = phytsi->sc_sz;
		}
	} else {
		/* For other modes read FFT or 2xFFT length samples */
		if ((phytsi->frame_type == PHYTS_FRAMETYPE_HE) &&
			((read_mode == SC_READ_FTM_HE_VHTLTF) ||
			(read_mode == SC_READ_ACK_HE_VHTLTF))) {
			/* 2x HELTF modes */
			read_cnt = 2u * phytsi->nfft;
		} else {
			read_cnt = phytsi->nfft;
		}
	}

	/* Compute read pointer */
	sc_read_ptr = phy_ac_tof_phyts_compute_read_ptr(tofi, phytsi->sc_ftm_start, sc_off);
	phytsi->sc_read_offsets[read_mode] = sc_read_ptr;

	/* Read from BM */
	phy_ac_tof_phyts_sampcap_read_frm_bm(tofi, sc_read_ptr, read_cnt, sc_mem);
done:
	/* Resume MAC */
	wlapi_enable_mac(pi->sh->physhim);
	return ret;
}

static int
phy_ac_tof_phyts_read_sc(phy_type_tof_ctx_t *ctx, cint16 *sc_mem)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;

	/* Read full sample capture */
	return phy_ac_tof_phyts_read_sampcap(tofi, SC_READ_FULL, 0u, sc_mem);
}

static int
phy_ac_tof_phyts_enable_sc(phy_type_tof_ctx_t *ctx)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	int ret = BCME_OK;

	/* Enable sample-capture */
	ret = phy_ac_tof_phyts_sampcap_enable(tofi);

	return ret;
}

static uint8
phy_ac_tof_phyts_rfseq_event_offset(phy_info_t *pi, uint16 base, uint16 event,
	uint16* rfseq_events, bool last)
{
	uint8 i, k;

	phy_ac_tbl_rfseq_read(pi, 16, base, rfseq_events);
	k = 16u;
	for (i = 0u; i < 16u; i++) {
		if (rfseq_events[i] == event) {
			k = i;
			if (!last) {
				break; /* stop at first occurrance */
			}
		}
	}

	return k;
}

static int
phy_ac_tof_phyts_lpfsw_bq_rfseq_setup(phy_info_t *pi)
{
	int ret = BCME_OK;
	uint8 core;
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint16 rfseq_tiapu_addr[] = {0x14d, 0x15d, 0x16d, 0x48d};
	uint16 rfseq_sw_aux_addr[] = {0x14f, 0x15f, 0x16f, 0x48f};
	uint16 rfseq_sw_aux_tone_addr[] = {0x36b, 0x37b, 0x38b, 0x55b};
	uint16 val;

	/* save the tia rfseq values */
	FOREACH_CORE(pi, core) {
		/* keep radio blocks on for rx2tx (search for tia_pu, bq1_adc in rfseq.vhd */
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_tiapu_addr[core], 16,
			&phytsi->saved_rfseq_rx2tx_tia[core]);
		/* 0x447:  0x7 tia_pu = 1, 0x40 - tx_rx_en = 1, 0x400 = sw_bq1_adc = 1 */
		val = (phytsi->saved_rfseq_rx2tx_tia[core] | 0x447);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_tiapu_addr[core],
			16, &val);

		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_sw_aux_addr[core], 16,
			&phytsi->saved_rfseq_bundle_aux[core]);
		/*  0xbfff:  sw_aux_bq1 = 0  */
		val = (phytsi->saved_rfseq_bundle_aux[core] & 0xbfff);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_sw_aux_addr[core],
			16, &val);

		/* HE packets follow Tx tone RFSeq */
		wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_sw_aux_tone_addr[core],
			16, &phytsi->saved_rfseq_bundle_aux_tone[core]);
		/*  0xfdff:  sw_aux_bq1 = 0, 0x8 = bq1_adc = 1  */
		val = (phytsi->saved_rfseq_bundle_aux_tone[core] & 0xfdff) | 0x8;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_sw_aux_tone_addr[core],
			16, &val);
	}

	return ret;
}

static int
phy_ac_tof_phyts_lpfsw_bq_rfseq_cleanup(phy_info_t *pi)
{
	int ret = BCME_OK;
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint8 core;

	uint16 rfseq_tiapu_addr[] = {0x14d, 0x15d, 0x16d, 0x48d};
	uint16 rfseq_sw_aux_addr[] = {0x14f, 0x15f, 0x16f, 0x48f};
	uint16 rfseq_sw_aux_tone_addr[] = {0x36b, 0x37b, 0x38b, 0x55b};

	FOREACH_CORE(pi, core) {
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_tiapu_addr[core],
			16, &phytsi->saved_rfseq_rx2tx_tia[core]);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_sw_aux_addr[core],
			16, &phytsi->saved_rfseq_bundle_aux[core]);
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, rfseq_sw_aux_tone_addr[core],
			16, &phytsi->saved_rfseq_bundle_aux_tone[core]);
	}

	return ret;
}

static int
phy_ac_tof_phyts_rx2tx_tx2rx_setup(phy_info_t *pi)
{
	int ret = BCME_OK;
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint16 rfseq_bundle_48[3];
	uint8 off_tssi_cfg, off_tssi_sleep;
	uint16 loopback_bundle = 0x88;
	uint16 nop_cmd = K_TOF_RFSEQ_NOP_EVENT;
	uint16 const rfseq_tx2rx_cmd[] = {0x0, 0x4, 0x3, 0x6, 0x5, 0x3d, 0x2, 0x1, 0x8,
		0x2a, 0xf, 0x3e, 0xf, 0x2b, 0x0, 0x1f};

	/* Switch on radio-RX pus using rfseq bundle & command 0x88 in rx2tx command */
	/* save the rfseqbundle */
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 8, 48,
		phytsi->saved_rfseq_bundle_48);
	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		rfseq_bundle_48[0] = 0x0;
		rfseq_bundle_48[1] = 0xcf60;
		rfseq_bundle_48[2] = 0x21d9;
	} else {
		rfseq_bundle_48[0] = 0x49b0;
		rfseq_bundle_48[1] = 0x0f60;
		rfseq_bundle_48[2] = 0x2180;
	}
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 8, 48, rfseq_bundle_48);

	/* save off values */
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0, 16, phytsi->saved_rx2tx_cmd);
	off_tssi_cfg = phy_ac_tof_phyts_rfseq_event_offset(pi, 0, K_TOF_RFSEQ_TSSI_EVENT,
		phytsi->saved_rx2tx_cmd, FALSE);
	off_tssi_sleep = phy_ac_tof_phyts_rfseq_event_offset(pi, 0, K_TOF_RFSEQ_TSSI_SLEEP_EVENT,
		phytsi->saved_rx2tx_cmd, FALSE);
	if (off_tssi_cfg < 16) {
		phy_ac_tbl_rfseq_write(pi, 1, off_tssi_cfg, &loopback_bundle);
	}
	/* Replace TSSI_SLEEP event with NOP command */
	if (off_tssi_sleep < 16) {
		phy_ac_tbl_rfseq_write(pi, 1, off_tssi_sleep, &nop_cmd);
	}

	/* tx2rx - remove farrow reset */
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 16, 16, phytsi->saved_tx2rx_cmd);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 16, 16, rfseq_tx2rx_cmd);

	return ret;
}

static int
phy_ac_tof_phyts_rx2tx_tx2rx_cleanup(phy_info_t *pi)
{
	int ret = BCME_OK;
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 8, 48,
		phytsi->saved_rfseq_bundle_48);

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 0x0, 16,
		phytsi->saved_rx2tx_cmd);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 16, 16, 16,
		phytsi->saved_tx2rx_cmd);
	return ret;
}

static int
phy_ac_tof_phyts_phyregs_setup(phy_info_t *pi)
{
	int ret = BCME_OK;
	uint8 core;
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;

	/* Disable farrow reset */
	phytsi->saved_location_control = READ_PHYREG(pi, location_control);
	phytsi->saved_rffectrl1 = READ_PHYREG(pi, RxFeCtrl1);
	phytsi->saved_rx1spare  = READ_PHYREG(pi, rx1Spare);
	phytsi->saved_tdsfocorr = READ_PHYREG(pi, td_sfo_corr);
	phytsi->save_sdfeClkGatingCtrl = READ_PHYREG(pi, sdfeClkGatingCtrl);
	phytsi->save_fineRxclockgatecontrol = READ_PHYREG(pi, fineRxclockgatecontrol);
	FOREACH_CORE(pi, core) {
		phytsi->save_forceFront[core] = READ_PHYREGCE(pi, forceFront, core);
	}

	MOD_PHYREG(pi, location_control, override_Rx2TxReset, 1);
	MOD_PHYREG(pi, RxFeCtrl1, en_txrx_sdfeFifoReset, 0);
	WRITE_PHYREG(pi, rx1Spare, phytsi->saved_rx1spare & 0xfffb);
	MOD_PHYREG(pi, td_sfo_corr, td_sfo_perpktrst_en, 0);
	MOD_PHYREG(pi, sdfeClkGatingCtrl, disableRxStallonTx, 0);

	/* Don't turn off radio RX (when in TX) */
	/* Keep RX frontend clocks ON even in TX mode */
	MOD_PHYREG(pi, fineRxclockgatecontrol, disablerx1CoreMaskClkGating, 1);
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, forceFront, core, front40, 1);
	}

	return ret;
}

static int
phy_ac_tof_phyts_phyregs_cleanup(phy_info_t *pi)
{
	int ret = BCME_OK;
	uint8 core;
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;

	WRITE_PHYREG(pi, location_control, phytsi->saved_location_control);
	WRITE_PHYREG(pi, RxFeCtrl1, phytsi->saved_rffectrl1);
	WRITE_PHYREG(pi, rx1Spare, phytsi->saved_rx1spare);
	WRITE_PHYREG(pi, td_sfo_corr, phytsi->saved_tdsfocorr);
	WRITE_PHYREG(pi, sdfeClkGatingCtrl, phytsi->save_sdfeClkGatingCtrl);
	WRITE_PHYREG(pi, fineRxclockgatecontrol, phytsi->save_fineRxclockgatecontrol);
	FOREACH_CORE(pi, core) {
		WRITE_PHYREGCE(pi, forceFront, core, phytsi->save_forceFront[core]);
	}

	return ret;
}

static int
phy_ac_tof_phyts_rfctrl_setup(phy_info_t *pi)
{
	int ret = BCME_OK;

	/* Setup lpfsw and bq related RFSeq */
	ret = phy_ac_tof_phyts_lpfsw_bq_rfseq_setup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	ret = phy_ac_tof_phyts_rx2tx_tx2rx_setup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

done:
	return ret;
}

static int
phy_ac_tof_phyts_rfctrl_cleanup(phy_info_t *pi)
{
	int ret = BCME_OK;

	/* Cleanup lpfsw and bq related RFSeq */
	ret = phy_ac_tof_phyts_lpfsw_bq_rfseq_cleanup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	ret = phy_ac_tof_phyts_rx2tx_tx2rx_cleanup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

done:
	return ret;
}

/* rxgain_bundle is as per rfctrl_bundle_rxgain format */
static int
phy_ac_tof_phyts_loopback_rxgain_setup(phy_info_t *pi, uint32 rxgain_bundle)
{
	int ret = BCME_OK;
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint8 core;
	uint16 gc[PHY_CORE_MAX];

	FOREACH_CORE(pi, core)
		gc[core] = rxgain_bundle;

	/* save the tables */
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 3, 0xf3,
		16, &phytsi->save_gaincode[0]);
	wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x503,
		16, &phytsi->save_gaincode[3]);

	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 3,  0xf3, 16, gc);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x503, 16, &gc[3]);

	return ret;
}

static int
phy_ac_tof_phyts_loopback_rxgain_cleanup(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	int ret = BCME_OK;

	/* restore the tables */
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 3, 0xf3,
		16, &phytsi->save_gaincode[0]);
	wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x503,
		16, &phytsi->save_gaincode[3]);
	return ret;
}

static int
phy_ac_tof_phyts_set_txpwr(phy_type_tof_ctx_t *ctx, int8 tx_pwr_dbm)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	int ret = BCME_OK;

	phytsi->tx_pwr = tx_pwr_dbm;
	return ret;
}

static int
phy_ac_tof_phyts_get_sc_read_buf_sz(phy_type_tof_ctx_t *ctx, uint32 *sc_read_buf_sz)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	int ret = BCME_OK;
	*sc_read_buf_sz = 0u;

	/* Get BW params */
	ret = phy_ac_tof_phyts_bw_params(tofi);
	if (ret != BCME_OK) {
		goto done;
	}
	if (CHSPEC_IS6G(pi->radio_chanspec)) {
		*sc_read_buf_sz = PHYTS_SC_READ_BUF_6G_LEN(phytsi->bw_mhz);
	} else {
		*sc_read_buf_sz = PHYTS_SC_READ_BUF_5G_LEN(phytsi->bw_mhz);
	}
done:
	return ret;
}

static int
phy_ac_tof_phyts_afediv_override_setup(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	int ret = BCME_OK;

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		/* Disable low rate ADC */
		phytsi->saved_tssimode = READ_PHYREG(pi, TSSIMode);
		MOD_PHYREG(pi, TSSIMode, tssiADCSel, 1);
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
			wlc_phy_radio20698_afe_div_ratio(pi, 1, 0, 0);
		} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) {
			wlc_phy_radio20704_afe_div_ratio(pi, 1);
		} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) {
			wlc_phy_radio20707_afe_div_ratio(pi, 1);
		} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
			wlc_phy_radio20710_afe_div_ratio(pi, 1, 0, FALSE);
		}
	}

	phytsi->save_AfePuCtrl = READ_PHYREG(pi, AfePuCtrl);
	WRITE_PHYREG(pi, AfePuCtrl, READ_PHYREG(pi, AfePuCtrl) & 0x7c3f);
	//	wlc_phy_resetcca_acphy(pi);

	return ret;
}

static int
phy_ac_tof_phyts_afediv_override_cleanup(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	int ret = BCME_OK;

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		/* Enable low rate ADC */
		if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
			wlc_phy_radio20698_afe_div_ratio(pi, 0, 0, 0);
		} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) {
			wlc_phy_radio20704_afe_div_ratio(pi, 0);
		} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) {
			wlc_phy_radio20707_afe_div_ratio(pi, 0);
		} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
			wlc_phy_radio20710_afe_div_ratio(pi, 0, 0, FALSE);
		}
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		WRITE_PHYREG(pi, TSSIMode, phytsi->saved_tssimode);
	}

	WRITE_PHYREG(pi, AfePuCtrl, phytsi->save_AfePuCtrl);

	return ret;
}

static int
phy_ac_tof_phyts_rxadc_override_setup(phy_info_t *pi)
{
	uint8 core;

	if (0) {
		/* Probbaly don't need it, just keeping the code for now */
		if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REG_20698(pi, RXADC_CFG1, core, rxadc0_en, 0x1);
				MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_rxadc0_en, 0x1);
			}
		}
	}

	return BCME_OK;
}

static int
phy_ac_tof_phyts_rxadc_override_cleanup(phy_info_t *pi)
{
	return BCME_OK;
}

static int
phy_ac_tof_phyts_dac_override_setup(phy_info_t *pi)
{
	return BCME_OK;
}

static int
phy_ac_tof_phyts_dac_override_cleanup(phy_info_t *pi)
{
	int ret = BCME_OK;

	return ret;
}

static int
phy_ac_tof_phyts_openloop_pwrctrl_setup(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint8 core;

	FOREACH_CORE(pi, core) {
		if (core == 0) {
			phytsi->save_tssi_sense[core] = READ_PHYREG(pi, TxPwrCtrlCore0TSSISensLmt);
			WRITE_PHYREG(pi, TxPwrCtrlCore0TSSISensLmt, 127);
		} else if (core == 1) {
			phytsi->save_tssi_sense[core] = READ_PHYREG(pi, TxPwrCtrlCore1TSSISensLmt);
			WRITE_PHYREG(pi, TxPwrCtrlCore1TSSISensLmt, 127);
		} else if (core == 2) {
			phytsi->save_tssi_sense[core] = READ_PHYREG(pi, TxPwrCtrlCore2TSSISensLmt);
			WRITE_PHYREG(pi, TxPwrCtrlCore2TSSISensLmt, 127);
		} else {
			phytsi->save_tssi_sense[core] = READ_PHYREG(pi, TxPwrCtrlCore3TSSISensLmt);
			WRITE_PHYREG(pi, TxPwrCtrlCore3TSSISensLmt, 127);
		}
	}

	return BCME_OK;
}

static int
phy_ac_tof_phyts_openloop_pwrctrl_cleanup(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint8 core;

	FOREACH_CORE(pi, core) {
		if (core == 0) {
			WRITE_PHYREG(pi, TxPwrCtrlCore0TSSISensLmt, phytsi->save_tssi_sense[core]);
		} else if (core == 1) {
			WRITE_PHYREG(pi, TxPwrCtrlCore1TSSISensLmt, phytsi->save_tssi_sense[core]);
		} else if (core == 2) {
			WRITE_PHYREG(pi, TxPwrCtrlCore2TSSISensLmt, phytsi->save_tssi_sense[core]);
		} else {
			WRITE_PHYREG(pi, TxPwrCtrlCore3TSSISensLmt, phytsi->save_tssi_sense[core]);
		}
	}

	return BCME_OK;
}

static void
phy_ac_tof_phyts_compute_loopback_rxgain(phy_info_t *pi, uint8 core)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	int8 max_tx_pwr = pi->tx_power_max_per_core[core] >> 2;
	int8 mcs0_tx_pwr_dBm = max_tx_pwr - PHYTS_MCS0_BACKOFF;
	int8 tx_pwr, req_gain, tia_gain;
	uint8 lna = 4, gm = 3, tia;
	uint8 rev47_iso[] = {60, 48, 57, 66};  /* measured on one board - 149/80 */

	/* Use power configured or mcs0 power */
	tx_pwr = (phytsi->tx_pwr == PHYTS_INVALID_TX_PWR) ? mcs0_tx_pwr_dBm : phytsi->tx_pwr;
	req_gain = PHYTS_LOOPBACK_REF_PWR + rev47_iso[core] - tx_pwr;
	tia_gain = req_gain - 12;   /* ilna 4 = 12 dB */

	/* for now hardcode tia(between 0 & 6) and ilna = 4 */
	tia_gain = MAX(0, tia_gain);
	tia_gain = MIN(28, tia_gain);
	tia = (tia_gain - 10) / 3;

	/*
	printf("core %d, req_gain = %d, tia_gain = %d, tia = %d\n",
		core, req_gain, tia_gain, tia);
	*/
	phytsi->loopback_rxgain_bundle = (tia << 6 | gm << 3 | lna);
}

static int
phy_ac_tof_phyts_loopback_setup(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint8 core;
	int ret = BCME_OK;

	/* Setup flag */
	phytsi->is_loopback_setup = TRUE;

	if (ACMAJORREV_47(pi->pubpi->phy_rev) && CHSPEC_IS2G(pi->radio_chanspec)) {
		FOREACH_CORE(pi, core) {
			/* save radio registers */
			phytsi->save_RX2G_REG3[core] = READ_RADIO_REG_20698(pi, RX2G_REG3, core);
			phytsi->save_RX2G_CFG1_OVR[core] = READ_RADIO_REG_20698(pi,
				RX2G_CFG1_OVR, core);

			/* for some reason gm_en is not getting enabled
				using Rfseq values, this needs debugging
			*/
			MOD_RADIO_REG_20698(pi, RX2G_REG3, core, rx2g_gm_en, 0x1);
			MOD_RADIO_REG_20698(pi, RX2G_CFG1_OVR, core, ovr_rx2g_gm_en, 0x1);
		}
	}

	/* Setup phyregs */
	ret = phy_ac_tof_phyts_phyregs_setup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Configure RFSeq and RFCtrl */
	ret = phy_ac_tof_phyts_rfctrl_setup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Compute loopback rx gain */
	phy_ac_tof_phyts_compute_loopback_rxgain(pi, phytsi->core);

	ret = phy_ac_tof_phyts_loopback_rxgain_setup(pi, phytsi->loopback_rxgain_bundle);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Override AFE div */
	ret = phy_ac_tof_phyts_afediv_override_setup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Override ADC reset and PU */
	ret = phy_ac_tof_phyts_rxadc_override_setup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Override DAC reset and PU */
	ret = phy_ac_tof_phyts_dac_override_setup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Setup open loop power control */
	ret = phy_ac_tof_phyts_openloop_pwrctrl_setup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

done:
	return ret;
}

static int
phy_ac_tof_phyts_loopback_cleanup(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint8 core;
	int ret = BCME_OK;

	/* Remove AFE div override */
	ret = phy_ac_tof_phyts_afediv_override_cleanup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Cleanup loopback rx gain */
	ret = phy_ac_tof_phyts_loopback_rxgain_cleanup(pi);
	if (ret != BCME_OK) {

		goto done;
	}

	/* Restore RFSeq and RFCtrl */
	ret = phy_ac_tof_phyts_rfctrl_cleanup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Cleanup phyregs */
	ret = phy_ac_tof_phyts_phyregs_cleanup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Remove DAC override */
	ret = phy_ac_tof_phyts_dac_override_cleanup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Remove ADC reset and PU override */
	ret = phy_ac_tof_phyts_rxadc_override_cleanup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Cleanup open loop power control */
	ret = phy_ac_tof_phyts_openloop_pwrctrl_cleanup(pi);
	if (ret != BCME_OK) {
		goto done;
	}

	/* Restore RADIO regs */
	if (ACMAJORREV_47(pi->pubpi->phy_rev) && CHSPEC_IS2G(pi->radio_chanspec)) {
		FOREACH_CORE(pi, core) {
			WRITE_RADIO_REG_20698(pi, RX2G_REG3, core, phytsi->save_RX2G_REG3[core]);
			WRITE_RADIO_REG_20698(pi, RX2G_CFG1_OVR, core,
				phytsi->save_RX2G_CFG1_OVR[core]);
		}
	}

	/* Reset flag */
	phytsi->is_loopback_setup = FALSE;
done:
	return ret;
}

static int
phy_ac_tof_phyts_loopback(phy_type_tof_ctx_t *ctx, bool setup)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_info_t *pi = tofi->pi;
	uint8 stall_val;
	int ret = BCME_OK;

	/* Suspend MAC */
	wlapi_suspend_mac_and_wait(pi->sh->physhim);

	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	if (setup) {
		/* Setup loopback */
		ret = phy_ac_tof_phyts_loopback_setup(pi);
		if (ret != BCME_OK) {
			goto done;
		}
	} else {
		/* Cleanup loopback */
		ret = phy_ac_tof_phyts_loopback_cleanup(pi);
		if (ret != BCME_OK) {
			goto done;
		}
	}

done:
	ACPHY_ENABLE_STALL(pi, stall_val);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);

	/* Resume MAC */
	wlapi_enable_mac(pi->sh->physhim);
	return ret;
}

bool
phy_ac_tof_phyts_is_loopback_setup(phy_info_t *pi)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)pi->u.pi_acphy->tofi;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;

	return phytsi->is_loopback_setup;
}

/* */
/* */
/* Sample-capture post processing code */
/* */
/* */

static const uint32*
phy_ac_tof_phyts_get_ref_lltf(uint16 bw_mhz)
{
	const uint32 *ref_freq_n;

	switch (bw_mhz) {
		case 160u:
			ref_freq_n = lltf_160_mhz;
			break;
		case 80u:
			ref_freq_n = lltf_80_mhz;
			break;
		case 40u:
			ref_freq_n = lltf_40_mhz;
			break;
		case 20u:
			ref_freq_n = lltf_20_mhz;
			break;
		default:
			PHY_ERROR(("phy_ac_tof_phyts_get_ref_lltf: Unsupported BW\n"));
			ref_freq_n = NULL;
	}

	return ref_freq_n;
}

static const uint32*
phy_ac_tof_phyts_get_ref_vhtltf(uint16 bw_mhz)
{
	const uint32 *ref_freq_n;

	switch (bw_mhz) {
		case 160u:
			ref_freq_n = vhtltf_160_mhz;
			break;
		case 80u:
			ref_freq_n = vhtltf_80_mhz;
			break;
		case 40u:
			ref_freq_n = vhtltf_40_mhz;
			break;
		case 20u:
			ref_freq_n = vhtltf_20_mhz;
			break;
		default:
			PHY_ERROR(("phy_ac_tof_phyts_get_ref_vhtltf: Unsupported BW\n"));
			ref_freq_n = NULL;
	}

	return ref_freq_n;
}

static const uint32*
phy_ac_tof_phyts_get_ref_heltf(uint16 bw_mhz)
{
	const uint32 *ref_freq_n;

	switch (bw_mhz) {
		case 160u:
			ref_freq_n = heltf_2x160_mhz;
			break;
		case 80u:
			ref_freq_n = heltf_2x80_mhz;
			break;
		default:
			PHY_ERROR(("phy_ac_tof_phyts_get_ref_heltf: Unsupported BW\n"));
			ref_freq_n = NULL;
	}

	return ref_freq_n;
}

#ifdef PHYTS_MATCHFILTER_VALIDATION
#define PHYTS_ACC_OVERFLOW_CHECK(x) ((x) >> 31u)
static int
phy_ac_tof_phyts_mf_validation(phy_ac_tof_phyts_info_t *phytsi, uint32 *cir_mag, uint16 nfft_up)
{
	bool mf_check1, mf_check2;
	uint8 max_acc_cir_idx = 0u;
	uint16 cir_idx = 0u, acc_cir_idx = 0u;
	uint16 acc_cir_win_sz = (nfft_up / PHYTS_ENERGY_ACC_WINDOWS);
	uint32 min_acc_cir_pwr = 0xFFFFFFFFu, max_acc_cir_pwr = 0u;
	int ret = BCME_OK;

	for (acc_cir_idx = 0u; acc_cir_idx < PHYTS_ENERGY_ACC_WINDOWS; acc_cir_idx++) {
		phytsi->acc_cir_pwr[acc_cir_idx] = 0u;
	}
	/* Accumulate energy in each window */
	while (cir_idx < nfft_up) {
		acc_cir_idx = cir_idx / acc_cir_win_sz;
		phytsi->acc_cir_pwr[acc_cir_idx] += cir_mag[cir_idx];
		if (PHYTS_ACC_OVERFLOW_CHECK(phytsi->acc_cir_pwr[acc_cir_idx])) {
			/* Skip validation */
			PHY_INFORM(("Phyts:Match-filtering validation skipped\n"));
			goto done;
		}
		cir_idx++;
	}
	/* Find the range of the measurements, index of max_acc_cir_pwr */
	for (acc_cir_idx = 0u; acc_cir_idx < PHYTS_ENERGY_ACC_WINDOWS; acc_cir_idx++) {
		if (phytsi->acc_cir_pwr[acc_cir_idx] > max_acc_cir_pwr) {
			max_acc_cir_idx = acc_cir_idx;
			max_acc_cir_pwr = phytsi->acc_cir_pwr[acc_cir_idx];
		}
		if (phytsi->acc_cir_pwr[acc_cir_idx] < min_acc_cir_pwr) {
			min_acc_cir_pwr = phytsi->acc_cir_pwr[acc_cir_idx];
		}
	}
	/* Validation */
	/* Check 1: Max energy must be always in window0 (window size 0.8us), */
	/* as VHT/HE-LTF MF output will produce CIR with max shift less < 0.8us */
	/* as it is expected to consumes samples starting in the CP */
	/* Check 2: If MF input is not VHT/HT-LTF, then power profile at the MF */
	/* output, accumulated over WIN_SZ, will be almost flat */
	mf_check1 = (max_acc_cir_idx != 0u);
	mf_check2 = ((max_acc_cir_pwr >> (PHYTS_ENERGY_ACC_RANGE_THRESH_DB/ 3u)) <
		min_acc_cir_pwr);
	if (mf_check1 || mf_check2) {
		PHY_INFORM(("Phyts:Match-filtering validation failed\n"));
		ret = BCME_ERROR;
	}
done:
	return ret;
}
#endif /* PHYTS_MATCHFILTER_VALIDATION */

static int
phy_ac_tof_phyts_maxpwr_idx(phy_ac_tof_phyts_info_t *phytsi, cint32 *cir, uint8 mf_mode)
{
	uint16 cir_idx = 0u, nfft_up = 0u;
	uint16 cir_maxpwr_idx = 0u, cir_maxpwr_idx_1x = 0u, cir_maxpwr_idx_2x = 0u;
	uint32 cir_maxpwr = 0u, cir_pwr;
	uint32* cir_mag;
	int ret = BCME_OK;

	nfft_up = phytsi->nfft;
#ifdef PHYTS_APPLY_2X_IFFT
	if (phytsi->fft_2x_mode) {
		nfft_up *= 2u;
	}
#endif /* PHYTS_APPLY_2X_IFFT */
	cir_mag = (uint32 *)cir;

	while (cir_idx < nfft_up) {
		/* As i,q values are 13 bits, uint32 is enough for sum(i^2+q^2) */
		cir_pwr = (cir[cir_idx].i * cir[cir_idx].i +
			cir[cir_idx].q * cir[cir_idx].q);
		if (cir_pwr > cir_maxpwr) {
			cir_maxpwr = cir_pwr;
			cir_maxpwr_idx = cir_idx;
		}
#ifdef WL_PROXD_PHYTS_DEBUG
		if (PHYTS_VHT_HE_FTM_MODE(mf_mode)) {
			phyts_cir_mag_dbg[0u][cir_idx] = cir_pwr;
		} else if (PHYTS_VHT_HE_ACK_MODE(mf_mode)) {
			phyts_cir_mag_dbg[1u][cir_idx] = cir_pwr;
		}
#endif /* WL_PROXD_PHYTS_DEBUG */
		/* Update first nfft_up "uint32" location of */
		/* cir(cint32) with channel mag response */
		cir_mag[cir_idx] = cir_pwr;
		cir_idx++;
	}
#ifdef PHYTS_MATCHFILTER_VALIDATION
	if (PHYTS_VHT_HE_FTM_MODE(mf_mode)) {
		ret = phy_ac_tof_phyts_mf_validation(phytsi, cir_mag, nfft_up);
		if (ret != BCME_OK) {
			goto done;
		}
	}
#endif /* PHYTS_MATCHFILTER_VALIDATION */
	/* By default overide 1x and 2x with same values */
	cir_maxpwr_idx_2x = cir_maxpwr_idx;
	cir_maxpwr_idx_1x = cir_maxpwr_idx;
#ifdef PHYTS_APPLY_2X_IFFT
	if (phytsi->fft_2x_mode) {
		cir_maxpwr_idx_1x = (cir_maxpwr_idx / 2u);
	}
#endif /* PHYTS_APPLY_2X_IFFT */

	if (PHYTS_L_FTM_MODE(mf_mode)) {
		phytsi->ftm_lltf_maxpwr = cir_maxpwr;
		phytsi->ftm_lltf_maxpwr_fft_idx = cir_maxpwr_idx_2x;
		phytsi->ftm_lltf_maxpwr_idx = phytsi->ftm_lltf_coarse_idx + cir_maxpwr_idx_1x;
	} else if (PHYTS_VHT_HE_FTM_MODE(mf_mode)) {
		phytsi->ftm_he_vhtltf_maxpwr = cir_maxpwr;
		phytsi->ftm_he_vhtltf_maxpwr_fft_idx = cir_maxpwr_idx_2x;
		phytsi->ftm_he_vhtltf_maxpwr_idx = phytsi->ftm_he_vhtltf_coarse_idx +
			cir_maxpwr_idx_1x;
	} else if (PHYTS_L_ACK_MODE(mf_mode)) {
		phytsi->ack_lltf_maxpwr = cir_maxpwr;
		phytsi->ack_lltf_maxpwr_fft_idx = cir_maxpwr_idx_2x;
		phytsi->ack_lltf_maxpwr_idx = phytsi->ack_lltf_coarse_idx + cir_maxpwr_idx_1x;
	} else if (PHYTS_VHT_HE_ACK_MODE(mf_mode)) {
		phytsi->ack_he_vhtltf_maxpwr = cir_maxpwr;
		phytsi->ack_he_vhtltf_maxpwr_fft_idx = cir_maxpwr_idx_2x;
		phytsi->ack_he_vhtltf_maxpwr_idx = phytsi->ack_he_vhtltf_coarse_idx +
			cir_maxpwr_idx_1x;
	} else {
		ret = BCME_UNSUPPORTED;
	}
done:
	return ret;
}

static uint32
phy_ac_tof_phyts_get_firstp_thresh(phy_ac_tof_phyts_info_t *phytsi, uint32 max_pwr)
{
	return (max_pwr * phytsi->firstp_thresh_scale) >> phytsi->firstp_thresh_log2_div;
}

static int
phy_ac_tof_phyts_firstp_ts(phy_ac_tof_phyts_info_t *phytsi, cint32 *cir, uint8 mf_mode)
{
	uint16 max_pwr_idx = 0u, search_idx = 0u;
	uint32 firstp_thresh = 0u, num, den, firstp_idx;
	uint32* cir_mag;
	uint32 max_pwr;
	int ret = BCME_OK;
#ifdef PHYTS_FIRSTP_RIGHT_SEARCH
	uint16 window_offset = (PHYTS_FIRSTP_SEARCH_WINDOW_NS * phytsi->bw_mhz) / 1000u;
#endif /* PHYTS_FIRSTP_RIGHT_SEARCH */

	/* Find maximum power tap, this is index of start of VHT/HE-LTF */
	ret = phy_ac_tof_phyts_maxpwr_idx(phytsi, cir, mf_mode);
	if (ret != BCME_OK) {
		goto done;
	}
	cir_mag = (uint32 *)cir;

	/* Threshold is computed relative to max-pwr */
	if (PHYTS_VHT_HE_FTM_MODE(mf_mode)) {
		max_pwr = phytsi->ftm_he_vhtltf_maxpwr;
		max_pwr_idx = phytsi->ftm_he_vhtltf_maxpwr_fft_idx;
	} else if (PHYTS_VHT_HE_ACK_MODE(mf_mode)) {
		max_pwr = phytsi->ack_he_vhtltf_maxpwr;
		max_pwr_idx = phytsi->ack_he_vhtltf_maxpwr_fft_idx;
	} else {
		ret = BCME_ERROR;
		goto done;
	}
	firstp_thresh = phy_ac_tof_phyts_get_firstp_thresh(phytsi, max_pwr);

/* To enable right search after applying offset to max power idx */
#ifdef PHYTS_FIRSTP_RIGHT_SEARCH
	if ((max_pwr_idx - window_offset) > 0) {
		search_idx = (max_pwr_idx - window_offset);
	} else {
		search_idx = 0;
	}
	while (search_idx < max_pwr_idx) {
		search_idx++;
		if ((cir_mag[search_idx] > firstp_thresh) &&
			(cir_mag[search_idx-1u] < firstp_thresh)) {
			break;
		}
	}
#else
	/* Decrementing from max-pwr indx for searching the threshold crossing point */
	/* Search left */
	search_idx = max_pwr_idx;
	while (search_idx > 0u) {
		if ((cir_mag[search_idx] > firstp_thresh) &&
			(cir_mag[search_idx-1u] < firstp_thresh)) {
			break;
		}
		search_idx--;
	}
#endif /* PHYTS_FIRSTP_RIGHT_SEARCH */
	if (search_idx == 0u) {
		PHY_INFORM(("Unable to find threshold crossing point\n"));
		ret = BCME_ERROR;
		goto done;
	}
	/* Find the threshold crossing point, linear interop */
	num = firstp_thresh - cir_mag[search_idx - 1u];
	den = cir_mag[search_idx] - cir_mag[search_idx - 1u];
	while (((search_idx-1u) * den + num) >= (1u << FIRSTP_IDX_BITS_ALLOWED)) {
		num = num >> 1u;
		den = den >> 1u;
	}
	firstp_idx = (((search_idx-1u) * den + num) * FIRSTP_IDX_SCALE)/den;
#ifdef PHYTS_APPLY_2X_IFFT
	if (phytsi->fft_2x_mode) {
		firstp_idx /= 2u;
	}
#endif /* PHYTS_APPLY_2X_IFFT */

	/* In peco second */
	if (PHYTS_VHT_HE_FTM_MODE(mf_mode)) {
		phytsi->ftm_he_vhtltf_firstp_idx =	(phytsi->ftm_he_vhtltf_coarse_idx *
			FIRSTP_IDX_SCALE) + firstp_idx;
		phytsi->ftm_he_vhtltf_firstp_ps = (phytsi->ftm_he_vhtltf_firstp_idx /
			phytsi->bw_mhz) * (FIRSTP_US_TO_PS/FIRSTP_IDX_SCALE);
	} else if (PHYTS_VHT_HE_ACK_MODE(mf_mode)) {
		phytsi->ack_he_vhtltf_firstp_idx = (phytsi->ack_he_vhtltf_coarse_idx *
			FIRSTP_IDX_SCALE) + firstp_idx;
		phytsi->ack_he_vhtltf_firstp_ps =
			(phytsi->ack_he_vhtltf_firstp_idx / phytsi->bw_mhz)
			* (FIRSTP_US_TO_PS/FIRSTP_IDX_SCALE);
	} else {
		ret = BCME_ERROR;
		goto done;
	}
done:
	return ret;
}

static void
phy_ac_tof_phyts_iqimb(cint32* pIn, int a, int b, int n)
{
	cint32* pTmp = pIn;
	uint16 i;

	for (i = 0u; i < n; i++) {
		pTmp->q += ((pTmp->i * (int32)a) >> 10);
		pTmp->i += ((pTmp->i * (int32)b) >> 10);
		pTmp++;
	}
}

static void
phy_ac_tof_phyts_cfo(cint32* pIn, int cfo, int n, uint16 bw_mhz)
{
	/* CFO Correction */
	int i;
	math_fixed theta = 0, step = 0;
	math_cint32 exp_val;
	math_cint32 tmp;
	cint32 *pTmp;
	int32 cfo_32;

	if (cfo == 0) {
		return;
	}
	/* As we are using 1Fs for 4s/7s */
	cfo_32 = cfo * (CFO_2_HZ >> 1u);
	pTmp = pIn;
	step = FIXED((-cfo_32 * TWO_PI_DEGREES) / bw_mhz) / ONE_MEGA_HZ;
	for (i = 0u; i < n; i++) {
		math_cmplx_cordic(theta, &exp_val);
		tmp.i = FLOAT(pTmp->i * exp_val.i) - FLOAT(pTmp->q * exp_val.q);
		tmp.q = FLOAT(pTmp->i * exp_val.q) + FLOAT(pTmp->q * exp_val.i);
		pTmp->i = (int32)tmp.i;
		pTmp->q = (int32)tmp.q;
		theta += step;
		pTmp++;
	}
}

#ifdef PHYTS_APPLY_SMOOTHING_WIN
static int
phy_ac_tof_phyts_smoothing(phy_ac_tof_phyts_info_t *phytsi, cint32 *fft_out)
{
	uint16 win_idx = 0u, dwnsmple_fac = 0u, fft_idx = 0u;
	/* Max bandwidth 80Mhz */
	/* Indxing from 0 to N-1 */
	int ret = BCME_OK;

	if (phytsi->bw_mhz == PHYBW_160) {
		ret = BCME_OK;
	} else {
		dwnsmple_fac = (PHYBW_80 / phytsi->bw_mhz);
		while (win_idx < phytsi->nfft) {
			fft_out[fft_idx].i *= phyts_smooth_win[win_idx];
			fft_out[fft_idx].q *= phyts_smooth_win[win_idx];
			fft_out[fft_idx].i >>= SMOOTHING_WIN_BITS;
			fft_out[fft_idx].q >>= SMOOTHING_WIN_BITS;
			win_idx += dwnsmple_fac;
			fft_idx++;
		}
	}

	return ret;
}
#endif /* PHYTS_APPLY_SMOOTHING_WIN */

static int
phy_ac_tof_phyts_freq_refltf_mult(phy_ac_tof_phyts_info_t *phytsi, cint32 *fft_in,
	uint8 mf_mode)
{
	uint8 nibble_idx = 0u, nibble;
	uint16 symb_idx = 0u;
	int32 fft_tmp;
	const uint32 *ref_freq_n;
	int ret = BCME_OK;

	/* Referece LTF symbol */
	if (PHYTS_L_LTF_MODE(mf_mode)) {
		ref_freq_n = phy_ac_tof_phyts_get_ref_lltf(phytsi->bw_mhz);
	} else if (PHYTS_HE_LTF_MODE(mf_mode)) {
		ref_freq_n = phy_ac_tof_phyts_get_ref_heltf(phytsi->bw_mhz);
	} else {
		ref_freq_n = phy_ac_tof_phyts_get_ref_vhtltf(phytsi->bw_mhz);
	}
	if (ref_freq_n == NULL) {
		ret = BCME_UNSUPPORTED;
		goto done;
	}
	/* Complex conj of input and output, to use fft as ifft */
	/* Complex conj on output can be ignored, as we are using */
	/* magnitude square value for firstp */
	/* conj(y(k) * H(k)) (ref ltf) = conj(y(k)) * conj(H(k)) */
	while (symb_idx < phytsi->nfft) {
		for (nibble_idx = 0u; nibble_idx < 8u; nibble_idx++) {
			nibble = ((ref_freq_n[symb_idx >> 3u] >>
				(REF_SYMB_NIBBLE_SIZE * nibble_idx)) & REF_SYMB_NIBBLE_MASK);
			switch (nibble) {
				case 0x1: /* conj(y(k)) * conj(1 + 0j) */
					fft_in[symb_idx].q = - fft_in[symb_idx].q;
					break;
				case 0xf: /* conj(y(k)) * conj(-1 + 0j) */
					fft_in[symb_idx].i = - fft_in[symb_idx].i;
					break;
				case 0x2: /* conj(y(k)) * conj(0 + 1j) */
					fft_tmp = fft_in[symb_idx].i;
					fft_in[symb_idx].i = - fft_in[symb_idx].q;
					fft_in[symb_idx].q = - fft_tmp;
					break;
				case 0xe: /* conj(y(k)) * conj(0 - 1j) */
					fft_tmp = fft_in[symb_idx].i;
					fft_in[symb_idx].i = fft_in[symb_idx].q;
					fft_in[symb_idx].q = fft_tmp;
					break;
				case 0x0:
					fft_in[symb_idx].i = 0;
					fft_in[symb_idx].q = 0;
					break;
				default:
					ret = BCME_ERROR;
					goto done;
			}
			symb_idx++;
		}
	}

done:
	return ret;
}

#ifdef PHYTS_APPLY_2X_IFFT
/* H = fftshift([zeros(1,N/2) fftshift(H) zeros(1,N/2)]) */
/* H = [H(1:N/2-1) zeros(1,N) H(N/2,N-1)]) */
static int
phy_ac_tof_phyts_upsample_freq(phy_ac_tof_phyts_info_t *phytsi, cint32 *fft_freq)
{
	uint16 nfft = phytsi->nfft;

	memcpy(fft_freq + (3u * nfft / 2u),
			fft_freq + (nfft / 2u), (nfft / 2u) * sizeof(cint32));
	bzero(fft_freq + (nfft / 2u), nfft * sizeof(cint32));

	return BCME_OK;
}
#endif /* PHYTS_APPLY_2X_IFFT */

static int
phy_ac_tof_phyts_tdcorr(phy_ac_tof_phyts_info_t *phytsi, cint32* fft_in,
	uint16 sym_len, uint8 mf_mode)
{
#ifdef PHYTS_APPLY_3P2US_WIN_AVERAGE
	uint16 fft_idx = 0u;
#endif /* PHYTS_APPLY_3P2US_WIN_AVERAGE */
	int ret = BCME_OK;

	/* IQ Comp, only for 4s mode */
	if (phytsi->sc_mode == SC_MODE_FARROW_OUTPUT) {
		phy_ac_tof_phyts_iqimb(fft_in, phytsi->rxiq_a, phytsi->rxiq_b,
			sym_len);
	}
	/* CFO correction */
	if (((PHYTS_IS_INITIATOR(phytsi->role)) && (PHYTS_VHT_HE_FTM_MODE(mf_mode))) ||
		((PHYTS_IS_TARGET(phytsi->role)) && PHYTS_VHT_HE_ACK_MODE(mf_mode))) {
		phy_ac_tof_phyts_cfo(fft_in, phytsi->cfo, sym_len, phytsi->bw_mhz);
	}

#ifdef PHYTS_APPLY_3P2US_WIN_AVERAGE
	/* Add two 3.2us windows of 6.4us of 2x HE-LTF */
	if (PHYTS_HE_LTF_MODE(mf_mode)) {
		for (fft_idx = 0u; fft_idx < phytsi->nfft; fft_idx++) {
			fft_in[fft_idx].i += fft_in[fft_idx + phytsi->nfft].i;
			fft_in[fft_idx].q += fft_in[fft_idx + phytsi->nfft].q;
		}
	}
#endif /* PHYTS_APPLY_3P2US_WIN_AVERAGE */

	return ret;
}

static int
phy_ac_tof_phyts_match_filtering_freq(phy_ac_tof_info_t *tofi, cint16* sc_mem,
	uint16 sc_off, uint8 mf_mode)
{
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	phy_info_t *pi = tofi->pi;
	uint16 fft_idx = 0u, nfft_up = 0u, sym_len = 0u;
	cint16 *sc_mem_ltf_off;
	cint32 *fft_in, *fft_out;
	int ret = BCME_OK;

	/* Update upsamp factor, used for upsampling IFFT output */
	phytsi->fft_2x_mode = 0u;
	nfft_up = phytsi->nfft;
#ifdef PHYTS_APPLY_2X_IFFT
	if (PHYTS_VHT_HE_LTF_MODE(mf_mode)) {
		/* 1024 FFT is not support yet */
		if (!CHSPEC_IS160(pi->radio_chanspec)) {
			phytsi->fft_2x_mode = 1u;
			nfft_up *= 2u;
		}
	}
#endif /* PHYTS_APPLY_2X_IFFT */

	fft_out = (cint32 *)phy_malloc(pi, nfft_up * sizeof(cint32));
	if (fft_out == NULL) {
		PHY_ERROR(("wl%u: phyts_match_filtering_freq:malloc failed\n",
			PI_SLICE(pi)));
		ret = BCME_NOMEM;
		goto done;
	}
	fft_in = fft_out;

	/* Symbol length -> read 6.4us for 2xHELTF case and 3.2us other VHT */
	/* 2xHE 6.4us is read only if averaging scheme is enabled, otherwise */
	/* read only starting 3.2us (degraded performance) */
	sym_len = phytsi->nfft;
#ifdef PHYTS_APPLY_3P2US_WIN_AVERAGE
	if (PHYTS_HE_LTF_MODE(mf_mode)) {
		sym_len = 2 * phytsi->nfft;
	}
#endif /* PHYTS_APPLY_3P2US_WIN_AVERAGE */

	sc_mem_ltf_off = sc_mem + sc_off;
	/* Read LTF(LLTF or VHTLTF or 2xHELTF) symbol from the sample capture memory */
	for (fft_idx = 0u; fft_idx < sym_len; fft_idx++) {
		fft_in[fft_idx].i = (int32) sc_mem_ltf_off[fft_idx].i;
		fft_in[fft_idx].q = (int32) sc_mem_ltf_off[fft_idx].q;
	}

	/* Time domain correction/optimizations */
	ret = phy_ac_tof_phyts_tdcorr(phytsi, fft_in, sym_len, mf_mode);
	if (ret != BCME_OK) {
		goto done;
	}

	/* FFT: y(k) = fft(x(k)) */
	wlapi_fft(pi->sh->physhim, phytsi->nfft, (void*)fft_in, (void*)fft_out, 0);

	/* conj(y(k) * reference LTF(k)) -> To reuse fft as ifft */
	ret = phy_ac_tof_phyts_freq_refltf_mult(phytsi, fft_out, mf_mode);
	if (ret != BCME_OK) {
		goto done;
	}
#ifdef PHYTS_APPLY_SMOOTHING_WIN
	/* Apply smoothing to FTM and ACKs VHTLTF */
	if (PHYTS_VHT_HE_LTF_MODE(mf_mode)) {
		ret = phy_ac_tof_phyts_smoothing(phytsi, fft_out);
		if (ret != BCME_OK) {
			goto done;
		}
	}
#endif /* PHYTS_APPLY_SMOOTHING_WIN */

#ifdef PHYTS_APPLY_2X_IFFT
	if (phytsi->fft_2x_mode) {
		phy_ac_tof_phyts_upsample_freq(phytsi, fft_in);
	}
#endif /* PHYTS_APPLY_2X_IFFT */

	/* IFFT:(CIR) = ifft(y(k) * H(k)) */
	wlapi_fft(pi->sh->physhim, nfft_up, (void*)fft_in, (void*)fft_out, 0);

	if (PHYTS_L_LTF_MODE(mf_mode)) {
		/* Find maximum power tap, this is index of start of L-LTF2 */
		ret = phy_ac_tof_phyts_maxpwr_idx(phytsi, fft_out, mf_mode);
	} else {
		/* Find first tap with significant power */
		ret = phy_ac_tof_phyts_firstp_ts(phytsi, fft_out, mf_mode);
	}
	if (ret != BCME_OK) {
		goto done;
	}

done:
	if (fft_out != NULL) {
		phy_mfree(pi, fft_out, nfft_up * sizeof(cint32));
	}

	return ret;
}

#define PHYTS_VHT_MF_OFFSET	800u
#define PHYTS_HE_MF_OFFSET	1600u
#define SIFS_BACKOFF_NS		8000u

/* Find approriate LTF window and match filtering to timestamp */
/* Mode 0 FTM packet, 1 ACK packet */
/* ACK mode can be executed only after FTM mode is processed */
static int
phy_ac_tof_phyts_mf(phy_type_tof_ctx_t *ctx, cint16 *sc_mem, uint32 *ts,
	bool mode)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	uint8 mf_mode, sc_read_mode;
	uint16 sc_off = 0u;
	uint32 sc_mem_buf_off;
	cint16 *sc_mem_buf_ptr;
	int ret = BCME_OK;

	if (mode == PHYTS_MF_MODE_ACK) {
		uint32 ltf_to_data_end, ack_pktstart_coarse_idx;

		/* Find coarse idx inside first L-LTF symbol of ACK packet */
		/* Skipping DGI */
		phytsi->ack_lltf_coarse_idx = phytsi->ftm_he_vhtltf_maxpwr_idx;
		if (phytsi->frame_type == PHYTS_FRAMETYPE_HE) {
			ltf_to_data_end = ((phytsi->lsig_length + 3 + 2) / 3 * 4 - 16) * 1000;
			ack_pktstart_coarse_idx = phytsi->ftm_he_vhtltf_maxpwr_idx +
			         (ltf_to_data_end - PHYTS_HE_MF_OFFSET + SIFS_PLUS_VARIATIONS_NS -
			          SIFS_BACKOFF_NS)*(phytsi->bw_mhz) / 1000;
			ret = phy_ac_tof_phyts_he_ack_pkt_detect(ctx, sc_mem,
			                                         ack_pktstart_coarse_idx);
			if (ret != BCME_OK) {
				goto done;
			}

			phytsi->ack_lltf_coarse_idx = phytsi->ack_pktstart_idx +
				PKTSTRT_TO_LLTF_OFF_LEN(phytsi->bw_mhz) + phytsi->energydet_win_len;
		} else {
			// From VHT-LTF to pkt end. Remove VHT-SIGA and VHT-STF.
			ltf_to_data_end = ((phytsi->lsig_length + 3) / 3 * 4 - 12) * 1000;
			phytsi->ack_lltf_coarse_idx +=
				(ltf_to_data_end - PHYTS_VHT_MF_OFFSET +
				SIFS_PLUS_VARIATIONS_NS + PKTSTRT_TO_LLTF_OFF_NS) *
				(phytsi->bw_mhz) / 1000;
		}
		sc_off = phytsi->ack_lltf_coarse_idx;
		sc_read_mode = SC_READ_ACK_LLTF;
		mf_mode = PHYTS_MF_FREQ_MODE_ACK_LLTF;
	} else if (mode == PHYTS_MF_MODE_FTM) {
		/* Find coarse idx inside first L-LTF symbol of FTM packet */
		/* Skipping DGI */
		phytsi->ftm_lltf_coarse_idx = phytsi->pktstart_idx +
			PKTSTRT_TO_LLTF_OFF_LEN(phytsi->bw_mhz) + phytsi->energydet_win_len;
		sc_off = phytsi->ftm_lltf_coarse_idx;
		sc_read_mode = SC_READ_FTM_LLTF;
		mf_mode = PHYTS_MF_FREQ_MODE_FTM_LLTF;
	} else {
		ret = BCME_ERROR;
		goto done;
	}

	/* Read only required no of samples from BM */
	sc_mem_buf_off = phy_ac_tof_phyts_compute_sc_buff_off(tofi, sc_read_mode);
	sc_mem_buf_ptr = sc_mem + sc_mem_buf_off;
	phytsi->sc_buff_offsets[sc_read_mode] = sc_mem_buf_off;
	phy_ac_tof_phyts_read_sampcap(tofi, sc_read_mode, sc_off, sc_mem_buf_ptr);

	/* Match-filtering on the Legacy LTF packet */
	ret = phy_ac_tof_phyts_match_filtering_freq(tofi, sc_mem_buf_ptr, 0u, mf_mode);
	if (ret != BCME_OK) {
		goto done;
	}

	if (mode == PHYTS_MF_MODE_ACK) {
		/* Find coarse idx inside HE/VHT-LTF symbol of ACK packet */
		phytsi->ack_he_vhtltf_coarse_idx = phytsi->ack_lltf_maxpwr_idx;
		if (phytsi->frame_type == PHYTS_FRAMETYPE_HE) {
			phytsi->ack_he_vhtltf_coarse_idx +=
				LLTF2_TO_1P6GI_2XHELTF_OFF_LEN(phytsi->bw_mhz);
			mf_mode = PHYTS_MF_FREQ_MODE_ACK_HELTF;
		} else {
			phytsi->ack_he_vhtltf_coarse_idx += LLTF2_TO_VHTLTF_OFF_LEN(phytsi->bw_mhz);
			mf_mode = PHYTS_MF_FREQ_MODE_ACK_VHTLTF;
		}
		sc_read_mode = SC_READ_ACK_HE_VHTLTF;
		sc_off = phytsi->ack_he_vhtltf_coarse_idx;
	} else {
		/* Find coarse idx inside HE/VHT-LTF symbol of FTM packet */
		phytsi->ftm_he_vhtltf_coarse_idx = phytsi->ftm_lltf_maxpwr_idx;
		if (phytsi->frame_type == PHYTS_FRAMETYPE_HE) {
			phytsi->ftm_he_vhtltf_coarse_idx +=
				LLTF2_TO_1P6GI_2XHELTF_OFF_LEN(phytsi->bw_mhz);
			mf_mode = PHYTS_MF_FREQ_MODE_FTM_HELTF;
		} else {
			phytsi->ftm_he_vhtltf_coarse_idx += LLTF2_TO_VHTLTF_OFF_LEN(phytsi->bw_mhz);
			mf_mode = PHYTS_MF_FREQ_MODE_FTM_VHTLTF;
		}
		sc_read_mode = SC_READ_FTM_HE_VHTLTF;
		sc_off = phytsi->ftm_he_vhtltf_coarse_idx;
	}

	/* Read only required no of samples from BM */
	sc_mem_buf_off = phy_ac_tof_phyts_compute_sc_buff_off(tofi, sc_read_mode);
	sc_mem_buf_ptr = sc_mem + sc_mem_buf_off;
	phytsi->sc_buff_offsets[sc_read_mode] = sc_mem_buf_off;
	phy_ac_tof_phyts_read_sampcap(tofi, sc_read_mode, sc_off, sc_mem_buf_ptr);

	/* Match-filtering on the VHT/HE LTF packet */
	ret = phy_ac_tof_phyts_match_filtering_freq(tofi, sc_mem_buf_ptr, 0u, mf_mode);
	if (ret != BCME_OK) {
		goto done;
	}

	if (mode == PHYTS_MF_MODE_ACK) {
		*ts = phytsi->ack_he_vhtltf_firstp_ps;
	} else {
		*ts = phytsi->ftm_he_vhtltf_firstp_ps;
	}
done:
#ifdef WL_PROXD_PHYTS_DEBUG
	if (phyts_dbg_arr_idx < PHYTSI_DBG_ARR_LEN) {
		memcpy(&phytsi_dbg[phyts_dbg_arr_idx],
			phytsi, sizeof(phytsi_dbg[phyts_dbg_arr_idx]));
		printf("PHYTSI DBG Collect idx %d\n", phyts_dbg_arr_idx);
		if (mode == PHYTS_MF_MODE_ACK) {
			phyts_dbg_arr_idx++;
		}
	} else {
		phyts_dbg_arr_idx = 0u;
	}
#endif /* WL_PROXD_PHYTS_DEBUG */
	return ret;
}

/* Accumulate energy over given window size */
static uint32
phy_ac_tof_phyts_acc_pwr(cint16 *sc_mem, uint16 win_len)
{
	uint16 win_idx = 0u;
	uint32 acc_pwr_win = 0u;

	for (win_idx = 0u; win_idx < win_len; win_idx++) {
		/* As i,q values are 13 bits, uint32 is enough for acc(sum(i^2+q^2)) */
		/* over window sizes like 16 */
		acc_pwr_win += (sc_mem[win_idx].i * sc_mem[win_idx].i +
			sc_mem[win_idx].q * sc_mem[win_idx].q);
		/* Avoid saturation/wrap-around */
		if (ENERGYDET_SATURATION_CHECK(acc_pwr_win)) {
			acc_pwr_win = acc_pwr_win >> 2u;
		}
	}

	return acc_pwr_win;
}

/* Packet detection : Find the coarse start of the packet */
/* based on energy delta */
static int
phy_ac_tof_phyts_pkt_detect(phy_type_tof_ctx_t *ctx, cint16 *sc_mem)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	bool pkt_detected = FALSE;
	uint8 ed_state = ENERGYDET_WAIT_IN_ENERGY_DETECT;
	uint8 sc_read_mode = SC_READ_ENERGY_DETECT;
	uint16 sc_off = 0u, sc_idx = PHYTS_SC_READ_BUF_HE_OFFSET(phytsi->role, phytsi->bw_mhz);
	cint16 *sc_mem_ptr;
	uint32 acc_pwr_win, sc_mem_buf_off;
#ifdef PHYTS_ENERGYDET_SPIKE_DETECT
	uint8 spike_det_win_count = 0u;
	int32 max_energy_win_delta = 0u, energy_drop_det_thresh = 0u;
#endif /* PHYTS_ENERGYDET_SPIKE_DETECT */
	int ret = BCME_OK;

	/* Read required samples from BM */
	sc_mem_buf_off = phy_ac_tof_phyts_compute_sc_buff_off(tofi, sc_read_mode);
	phytsi->sc_buff_offsets[sc_read_mode] = sc_mem_buf_off;
	sc_mem_ptr = sc_mem + sc_mem_buf_off;
	phy_ac_tof_phyts_read_sampcap(tofi, sc_read_mode, sc_off, sc_mem_ptr);

	/* Find energydet window length and pkt detection limits */
	phytsi->energydet_win_len = ENERGYDET_ACC_WIN_LEN(phytsi->bw_mhz);
	phytsi->pktdet_limit_len = ENERGYDET_SC_READ_LEN(phytsi->bw_mhz);
	if (phytsi->frame_type == PHYTS_FRAMETYPE_HE) {
		phytsi->pktdet_limit_len +=
			PHYTS_SC_READ_BUF_HE_OFFSET(phytsi->role, phytsi->bw_mhz);
	}
	phytsi->energy_delta_thres = (PHYTS_IS_INITIATOR(phytsi->role)) ?
		ENERGYDET_DELTA_THRESH_INITIATOR : ENERGYDET_DELTA_THRESH_TARGET;

	while (sc_idx < phytsi->pktdet_limit_len) {
		sc_mem_ptr = sc_mem + sc_mem_buf_off + sc_idx;
		acc_pwr_win = phy_ac_tof_phyts_acc_pwr(sc_mem_ptr, phytsi->energydet_win_len);
		phytsi->energy_win_n = acc_pwr_win;
		if (sc_idx == 0u) {
			/* Acc noise power */
			phytsi->energy_win_1 = acc_pwr_win;
			phytsi->energy_win_delta = 0;
		} else {
			phytsi->energy_win_delta = (int32)(phytsi->energy_win_n -
				phytsi->energy_win_1);
		}
		switch (ed_state) {
			case ENERGYDET_WAIT_IN_ENERGY_DETECT:
				/* Energy detected */
				if (phytsi->energy_win_delta > phytsi->energy_delta_thres) {
					phytsi->pktstart_idx = sc_idx;
#ifdef PHYTS_ENERGYDET_SPIKE_DETECT
					/* Enable spike detection on responder */
					if (PHYTS_IS_TARGET(phytsi->role)) {
						ed_state = ENERGYDET_WAIT_IN_SPIKE_DETECT;
						max_energy_win_delta = phytsi->energy_win_delta;
						energy_drop_det_thresh = max_energy_win_delta >>
							ENERGYDET_ENERGY_DROP_THRESH_SHIFT;
					} else
#endif /* PHYTS_ENERGYDET_SPIKE_DETECT */
					{
						pkt_detected = TRUE;
						PHY_INFORM(("pkt_detected at idx: %u\n", sc_idx));
						goto done;
					}
				}
				break;
#ifdef PHYTS_ENERGYDET_SPIKE_DETECT
			case ENERGYDET_WAIT_IN_SPIKE_DETECT:
				spike_det_win_count += 1u;
				if (max_energy_win_delta < phytsi->energy_win_delta) {
					max_energy_win_delta = phytsi->energy_win_delta;
					energy_drop_det_thresh = max_energy_win_delta >>
						ENERGYDET_ENERGY_DROP_THRESH_SHIFT;
				}
				/* Spike detected */
				if (phytsi->energy_win_delta < energy_drop_det_thresh) {
					spike_det_win_count = 0u;
					phytsi->pktstart_idx = 0u;
					ed_state = ENERGYDET_WAIT_IN_ENERGY_DETECT;
				} else if (spike_det_win_count == ENERGYDET_SPIKE_DETECT_WINDOWS) {
					/* Energy seen over multiple windows, no spike detected */
					pkt_detected = TRUE;
					PHY_INFORM(("pkt_detected at idx: %u\n", sc_idx));
					goto done;
				}
				break;
#endif /* PHYTS_ENERGYDET_SPIKE_DETECT */
		}
		/* Move to next window */
		sc_idx = sc_idx + phytsi->energydet_win_len;
	}

done:
	if (!pkt_detected) {
		ret = BCME_ERROR;
	}
	return ret;
}

static int
phy_ac_tof_phyts_he_ack_pkt_detect(phy_type_tof_ctx_t *ctx, cint16 *sc_mem, uint16 sc_off)
{
	phy_ac_tof_info_t *tofi = (phy_ac_tof_info_t *)ctx;
	phy_ac_tof_phyts_info_t *phytsi = tofi->phytsi;
	bool pkt_detected = FALSE;
	uint8 ed_state = ENERGYDET_WAIT_IN_ENERGY_DETECT;
	uint8 sc_read_mode = SC_READ_HEACK_ENERGY_DETECT;
	uint16 sc_idx = 0;
	cint16 *sc_mem_ptr;
	uint32 acc_pwr_win, sc_mem_buf_off;
#ifdef PHYTS_ENERGYDET_SPIKE_DETECT
	uint8 spike_det_win_count = 0u;
	int32 max_energy_win_delta = 0u, energy_drop_det_thresh = 0u;
#endif /* PHYTS_ENERGYDET_SPIKE_DETECT */
	int ret = BCME_OK;

	/* Read required samples from BM */
	sc_mem_buf_off = phy_ac_tof_phyts_compute_sc_buff_off(tofi, sc_read_mode);
	sc_mem_ptr = sc_mem + sc_mem_buf_off;
	phy_ac_tof_phyts_read_sampcap(tofi, sc_read_mode, sc_off, sc_mem_ptr);
	/* Find energydet window length and pkt detection limits */

	phytsi->energydet_win_len = ENERGYDET_ACC_WIN_LEN(phytsi->bw_mhz);
	phytsi->pktdet_limit_len = ENERGYDET_SC_READ_LEN(phytsi->bw_mhz);
	phytsi->energy_delta_thres = (PHYTS_IS_INITIATOR(phytsi->role)) ?
		ENERGYDET_DELTA_THRESH_INITIATOR : ENERGYDET_DELTA_THRESH_TARGET;

	while (sc_idx < phytsi->pktdet_limit_len) {
		sc_mem_ptr = sc_mem + sc_mem_buf_off + sc_idx;
		acc_pwr_win = phy_ac_tof_phyts_acc_pwr(sc_mem_ptr, phytsi->energydet_win_len);
		phytsi->energy_win_n = acc_pwr_win;
		if (sc_idx == 0u) {
			/* Acc noise power */
			phytsi->energy_win_1 = acc_pwr_win;
			phytsi->energy_win_delta = 0;
		} else {
			phytsi->energy_win_delta = (int32)(phytsi->energy_win_n -
				phytsi->energy_win_1);
		}
		switch (ed_state) {
			case ENERGYDET_WAIT_IN_ENERGY_DETECT:
				/* Energy detected */
				if (phytsi->energy_win_delta > phytsi->energy_delta_thres) {
					phytsi->ack_pktstart_idx = sc_idx + sc_off;
#ifdef PHYTS_ENERGYDET_SPIKE_DETECT
					/* Enable spike detection on responder */
					if (PHYTS_IS_TARGET(phytsi->role)) {
						ed_state = ENERGYDET_WAIT_IN_SPIKE_DETECT;
						max_energy_win_delta = phytsi->energy_win_delta;
						energy_drop_det_thresh = max_energy_win_delta >>
							ENERGYDET_ENERGY_DROP_THRESH_SHIFT;
					} else
#endif /* PHYTS_ENERGYDET_SPIKE_DETECT */
					{
						pkt_detected = TRUE;
						PHY_INFORM(("ACK pkt_detected at idx: %u\n",
						            sc_idx + sc_off));
						goto done;
					}
				}
				break;
#ifdef PHYTS_ENERGYDET_SPIKE_DETECT
			case ENERGYDET_WAIT_IN_SPIKE_DETECT:
				spike_det_win_count += 1u;
				if (max_energy_win_delta < phytsi->energy_win_delta) {
					max_energy_win_delta = phytsi->energy_win_delta;
					energy_drop_det_thresh = max_energy_win_delta >>
						ENERGYDET_ENERGY_DROP_THRESH_SHIFT;
				}
				/* Spike detected */
				if (phytsi->energy_win_delta < energy_drop_det_thresh) {
					spike_det_win_count = 0u;
					phytsi->pktstart_idx = 0u;
					ed_state = ENERGYDET_WAIT_IN_ENERGY_DETECT;
				} else if (spike_det_win_count == ENERGYDET_SPIKE_DETECT_WINDOWS) {
					/* Energy seen over multiple windows, no spike detected */
					pkt_detected = TRUE;
					PHY_INFORM(("ACK pkt_detected at idx: %u\n",
					            sc_idx + sc_off));
					goto done;
				}
				break;
#endif /* PHYTS_ENERGYDET_SPIKE_DETECT */
		}
		/* Move to next window */
		sc_idx = sc_idx + phytsi->energydet_win_len;
	}

done:
	if (!pkt_detected) {
		ret = BCME_ERROR;
	}
	return ret;
}

#endif /* WL_PROXD_PHYTS */
