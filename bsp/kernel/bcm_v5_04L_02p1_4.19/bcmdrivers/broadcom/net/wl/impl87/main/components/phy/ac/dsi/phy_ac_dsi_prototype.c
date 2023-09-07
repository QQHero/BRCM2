/*
 * ACPHY DeepSleepInit module - 4339 Prototype Work
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: phy_ac_dsi_prototype.c 789044 2020-07-17 07:51:16Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>

/* PHY common dependencies */
#include <phy_dbg.h>
#include <phy_mem.h>
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

#include "fcbs.h"

#define DSI_DBG_PROTO_PRINTS 0

/* debug prints */
#if defined(DSI_DBG_PROTO_PRINTS) && DSI_DBG_PROTO_PRINTS
#define DSI_DBG_PROTO(args)	printf args; OSL_DELAY(500);
#else
#define DSI_DBG_PROTO(args)
#endif /* DSI_DBG_PROTO_PRINTS */

/* dispatch handle */
typedef void (*dsi_fn_t)(phy_info_t *pi);

/* local functions */
static void dsi_prototype_save_phyregs(phy_info_t *pi);
static void dsi_prototype_save_phytables(phy_info_t *pi);
static void dsi_prototype_save_radioregs(phy_info_t *pi);

static void dsi_prototype_restore_wars(phy_info_t *pi);
static void dsi_prototype_restore_phyregs(phy_info_t *pi);
static void dsi_prototype_restore_prefvals(phy_info_t *pi);
static void dsi_prototype_restore_phytables(phy_info_t *pi);
static void dsi_prototype_restore_radioregs(phy_info_t *pi);

/* Prints RawData FCBS sequences from fcbs_input_data_t struct */
static void dsi_rawdata(phy_info_t *pi, fcbs_input_data_t *input);

/* Populate FCBS to BM */
static void dsi_populate_fcbs(phy_info_t *pi, fcbs_input_data_t *input, int ds_inx);

/* macros & data structures */
#define NUM_PHYREGS	76
#define NUM_RADIOREGS	55

/* RFSeq len param for split segments */
#define RFSEQ_L_0	0x38
#define RFSEQ_L_1	0x08
#define RFSEQ_L_2	0x08
#define RFSEQ_L_3	0x10
#define RFSEQ_L_4	0x18
#define RFSEQ_L_5	0x08
#define RFSEQ_L_6	0x08
#define RFSEQ_L_7	0x05
#define RFSEQ_L_8	0x02
#define RFSEQ_L_9	0x02
#define RFSEQ_L_10	0x0b
#define RFSEQ_L_11	0x0b
#define RFSEQ_L_12	0x02
#define RFSEQ_L_13	0x02
#define RFSEQ_L_14	0x02
#define RFSEQ_L_15	0x03
#define RFSEQ_L_16	0x02
#define RFSEQ_L_17	0x02

/* RFSeq offset param for split segments */
#define RFSEQ_O_0	0x000
#define RFSEQ_O_1	0x040
#define RFSEQ_O_2	0x050
#define RFSEQ_O_3	0x070
#define RFSEQ_O_4	0x090
#define RFSEQ_O_5	0x0b0
#define RFSEQ_O_6	0x0c0
#define RFSEQ_O_7	0x121
#define RFSEQ_O_8	0x131
#define RFSEQ_O_9	0x137
#define RFSEQ_O_10	0x140
#define RFSEQ_O_11	0x360
#define RFSEQ_O_12	0x3c6
#define RFSEQ_O_13	0x3d6
#define RFSEQ_O_14	0x3e6
#define RFSEQ_O_15	0x3fa
#define RFSEQ_O_16	0x3fe
#define RFSEQ_O_17	0x440

/* tbl length params */
#define TL_PAPR			68
#define TL_IQLOCAL		160
#define TL_RFSEQEXT		9
#define TL_TXEVMTBL		40
#define TL_EPSILON0		64
#define TL_GAINLIMIT		105
#define TL_FEMCTRLLUT		256
#define TL_ESTPWRLUTS0		128
#define TL_RFSEQBUNDLE		128
#define TL_RSSICLIPGAIN0	22
#define TL_ESTPWRSHFTLUTS	24
#define TL_NVRXEVMSHAPINGTBL	256
#define TL_PHASETRACKTBL_1X1	22
#define TL_NVNOISESHAPINGTBL	256
#define TL_CHANNELSMOOTHING_1x1	512

/* Address Data Pairs : adp vectors */
static adp_t  DECLSPEC_ALIGN(4) phyregs_adp[NUM_PHYREGS];
static adp_t  DECLSPEC_ALIGN(4) radioregs_adp[NUM_RADIOREGS];

static uint8  DECLSPEC_ALIGN(4) data_TXEVMTBL[TL_TXEVMTBL];
static uint8  DECLSPEC_ALIGN(4) data_GAINLIMIT[TL_GAINLIMIT];
static uint8  DECLSPEC_ALIGN(4) data_ESTPWRSHFTLUTS[TL_ESTPWRSHFTLUTS];
static uint8  DECLSPEC_ALIGN(4) data_NVRXEVMSHAPINGTBL[TL_NVRXEVMSHAPINGTBL];
static uint16 DECLSPEC_ALIGN(4) data_IQLOCAL[TL_IQLOCAL];
static uint16 DECLSPEC_ALIGN(4) data_FEMCTRLLUT[TL_FEMCTRLLUT];
static uint16 DECLSPEC_ALIGN(4) data_ESTPWRLUTS0[TL_ESTPWRSHFTLUTS];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQBUNDLE[TL_RFSEQBUNDLE * 3];
static uint16 DECLSPEC_ALIGN(4) data_CHANNELSMOOTHING[TL_CHANNELSMOOTHING_1x1 * 3];
static uint32 DECLSPEC_ALIGN(4) data_PAPR[TL_PAPR];
static uint32 DECLSPEC_ALIGN(4) data_EPSILON0[TL_EPSILON0];
static uint32 DECLSPEC_ALIGN(4) data_RSSICLIPGAIN0[TL_RSSICLIPGAIN0];
static uint32 DECLSPEC_ALIGN(4) data_PHASETRACKTBL[TL_PHASETRACKTBL_1X1];
static uint32 DECLSPEC_ALIGN(4) data_NVNOISESHAPINGTBL[TL_NVNOISESHAPINGTBL];
static uint32 DECLSPEC_ALIGN(4) data_RFSEQEXT[TL_RFSEQEXT * 2];

/* RFSeq data segments for split sequences */
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_0[RFSEQ_L_0];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_1[RFSEQ_L_1];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_2[RFSEQ_L_2];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_3[RFSEQ_L_3];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_4[RFSEQ_L_4];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_5[RFSEQ_L_5];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_6[RFSEQ_L_6];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_7[RFSEQ_L_7];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_8[RFSEQ_L_8];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_9[RFSEQ_L_9];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_10[RFSEQ_L_10];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_11[RFSEQ_L_11];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_12[RFSEQ_L_12];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_13[RFSEQ_L_13];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_14[RFSEQ_L_14];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_15[RFSEQ_L_15];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_16[RFSEQ_L_16];
static uint16 DECLSPEC_ALIGN(4) data_RFSEQ_17[RFSEQ_L_17];

/* size vectors */
static uint16 sz_phyregs;
static uint16 sz_radioregs;

static uint16 sz_TXEVMTBL;
static uint16 sz_GAINLIMIT;
static uint16 sz_ESTPWRSHFTLUTS;
static uint16 sz_NVRXEVMSHAPINGTBL;
static uint16 sz_IQLOCAL;
static uint16 sz_FEMCTRLLUT;
static uint16 sz_ESTPWRLUTS0;
static uint16 sz_RFSEQBUNDLE;
static uint16 sz_CHANNELSMOOTHING;
static uint16 sz_PAPR;
static uint16 sz_EPSILON0;
static uint16 sz_RSSICLIPGAIN0;
static uint16 sz_PHASETRACKTBL;
static uint16 sz_NVNOISESHAPINGTBL;
static uint16 sz_RFSEQEXT;

phytbl_t tbl_info[] =
{
	{ACPHY_TBL_ID_TXEVMTBL, TL_TXEVMTBL, 0, 8, data_TXEVMTBL},
	{ACPHY_TBL_ID_GAINLIMIT, TL_GAINLIMIT, 0, 8, data_GAINLIMIT},
	{ACPHY_TBL_ID_ESTPWRSHFTLUTS, TL_ESTPWRSHFTLUTS, 0, 8, data_ESTPWRSHFTLUTS},
	{ACPHY_TBL_ID_NVRXEVMSHAPINGTBL, TL_NVRXEVMSHAPINGTBL, 0, 8, data_NVRXEVMSHAPINGTBL},
	{ACPHY_TBL_ID_IQLOCAL, TL_IQLOCAL, 0, 16, data_IQLOCAL},
	{ACPHY_TBL_ID_ESTPWRLUTS0, TL_ESTPWRLUTS0, 0, 16, data_ESTPWRLUTS0},
	{ACPHY_TBL_ID_RFSEQBUNDLE, TL_RFSEQBUNDLE, 0, 48, data_RFSEQBUNDLE},
	{ACPHY_TBL_ID_CHANNELSMOOTHING_1x1, TL_CHANNELSMOOTHING_1x1, 0, 48, data_CHANNELSMOOTHING},
	{ACPHY_TBL_ID_PAPR, TL_PAPR, 0, 32, data_PAPR},
	{ACPHY_TBL_ID_EPSILON0, TL_EPSILON0, 0, 32, data_EPSILON0},
	{ACPHY_TBL_ID_RSSICLIPGAIN0, TL_RSSICLIPGAIN0, 0, 32, data_RSSICLIPGAIN0},
	{ACPHY_TBL_ID_PHASETRACKTBL_1X1, TL_PHASETRACKTBL_1X1, 0, 32, data_PHASETRACKTBL},
	{ACPHY_TBL_ID_NVNOISESHAPINGTBL, TL_NVNOISESHAPINGTBL, 0, 32, data_NVNOISESHAPINGTBL},
	/* RFSeq Split Segments */
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_0, RFSEQ_O_0, 16, data_RFSEQ_0},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_1, RFSEQ_O_1, 16, data_RFSEQ_1},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_2, RFSEQ_O_2, 16, data_RFSEQ_2},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_3, RFSEQ_O_3, 16, data_RFSEQ_3},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_4, RFSEQ_O_4, 16, data_RFSEQ_4},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_5, RFSEQ_O_5, 16, data_RFSEQ_5},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_6, RFSEQ_O_6, 16, data_RFSEQ_6},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_7, RFSEQ_O_7, 16, data_RFSEQ_7},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_8, RFSEQ_O_8, 16, data_RFSEQ_8},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_9, RFSEQ_O_9, 16, data_RFSEQ_9},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_10, RFSEQ_O_10, 16, data_RFSEQ_10},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_11, RFSEQ_O_11, 16, data_RFSEQ_11},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_12, RFSEQ_O_12, 16, data_RFSEQ_12},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_13, RFSEQ_O_13, 16, data_RFSEQ_13},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_14, RFSEQ_O_14, 16, data_RFSEQ_14},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_15, RFSEQ_O_15, 16, data_RFSEQ_15},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_15, RFSEQ_O_16, 16, data_RFSEQ_16},
	{ACPHY_TBL_ID_RFSEQ, RFSEQ_L_16, RFSEQ_O_17, 16, data_RFSEQ_17},
	/* Removed from Snapshot */
	/* {ACPHY_TBL_ID_FEMCTRLLUT, TL_FEMCTRLLUT, 0, 16, data_FEMCTRLLUT}, */
	/* {ACPHY_TBL_ID_RFSEQEXT, TL_RFSEQEXT, 0, 60, data_RFSEQEXT}, */
};

/* -------------------------------------------------------------------------------------- */
/* 4335C0 proto work | phyreg init array */
static const adp_t maj1_min2_phy_reg_prefvals[] = {
	{0x0830, 0x46db}, {0x0831, 0x3724}, {0x0832, 0x06db}, {0x0833, 0x36db},
	{0x0834, 0x0924}, {0x0835, 0x0101}, {0x0836, 0x3edd}, {0x0837, 0x00be},
	{0x0838, 0x0600}, {0x0839, 0x0000}, {0x0838, 0x0600}, {0x083a, 0x0000},
	{0x083b, 0x0c0c}, {0x083c, 0x0cff}, {0x083d, 0x8b99}, {0x083e, 0x8f8b},
	{0x083f, 0x3034}, {0x0850, 0x0000}, {0x0851, 0x0882}, {0x0852, 0x0478},
	{0x0853, 0x8888}, {0x0854, 0x6666}, {0x0855, 0x0804}, {0x0856, 0x2010},
	{0x0839, 0x0009}, {0x0837, 0x03be}, {0x0839, 0x0f69}, {0x0836, 0x3edd},
	{0x0838, 0x0640}, {0x0837, 0xb3be}, {0x0838, 0x064c}, {0x0836, 0xbedd},
	{0x0415, 0x0000}, {0x040e, 0x0000}, {0x040c, 0x2000}, {0x0408, 0x0c00},
	{0x0408, 0x0c06}, {0x0408, 0x0c02}, {0x0749, 0x0003}, {0x0410, 0x0077},
	{0x0750, 0x0000}, {0x02ed, 0x000c}, {0x02f1, 0x002c}, {0x02ed, 0x000c},
	{0x02f1, 0x002c}, {0x02f5, 0x000c}, {0x02f9, 0x002c}, {0x0339, 0x0000},
	{0x03c4, 0x0668}, {0x01f2, 0x00c8}, {0x0025, 0x0030}, {0x02f1, 0x000c},
	{0x02ed, 0x000c}, {0x02f9, 0x000c}, {0x02f5, 0x000c}, {0x02ef, 0x2055},
	{0x02eb, 0x2055}, {0x02f7, 0x2055}, {0x02f3, 0x2055}, {0x02ef, 0x204d},
	{0x02eb, 0x204d}, {0x02f7, 0x204d}, {0x02f3, 0x204d}, {0x031c, 0x0080},
	{0x031e, 0x0080}, {0x031d, 0x0080}, {0x031f, 0x0080}, {0x01e1, 0x0040},
	{0x01e2, 0x005c}, {0x029e, 0x0000}, {0x029d, 0x004c}, {0x0176, 0x0006},
	{0x0176, 0x0006}, {0x01b0, 0x9ee9}, {0x02e0, 0x07ba}, {0x02d7, 0x7777},
	{0x02d6, 0x7732}, {0x01ca, 0x3000}, {0x0198, 0x0010}, {0x01b1, 0x5c00},
	{0x01b6, 0x443b}, {0x0160, 0x0029}, {0x0836, 0x3edd}, {0x0837, 0x00be},
	{0x0838, 0x0600}, {0x0839, 0x0000}, {0x0838, 0x0600}, {0x083c, 0x0cff},
	{0x083d, 0x8b99}, {0x083e, 0x8f8b}, {0x083f, 0x3034}, {0x0855, 0x0804},
	{0x0856, 0x2010}, {0x0839, 0x0009}, {0x0837, 0x03be}, {0x0839, 0x0f69},
	{0x0836, 0x3edd}, {0x0838, 0x0640}, {0x0837, 0xb3be}, {0x0838, 0x064c},
	{0x0836, 0xbedd}, {0x01e6, 0x0030}, {0x0020, 0x0012}, {0x0027, 0x02a8},
	{0x040a, 0x0290}, {0x0419, 0x0ffc}, {0x0177, 0x0002}, {0x016b, 0x0000},
	{0x019f, 0x0060}, {0x033a, 0x0415}, {0x033c, 0x0395}, {0x033e, 0x0415},
	{0x0340, 0x0395}, {0x0342, 0x0415}, {0x0344, 0x0395}, {0x0346, 0x0415},
	{0x0348, 0x0395}, {0x03a9, 0x0100}, {0x0413, 0xe19b}, {0x016c, 0x0000},
	{0x0679, 0x0008}, {0x0176, 0x0006}, {0x02e4, 0x0f20}, {0x01ec, 0x0002},
	{0x0419, 0x0ffd}, {0x0171, 0x0001}, {0x0601, 0x0049}, {0x0199, 0x0087},
	{0x01a0, 0x0087}, {0x01e3, 0x0048}, {0x0197, 0x0018}, {0x06ef, 0x0e17},
	{0x0373, 0x03c1}, {0x0372, 0x03c5}, {0x0371, 0x03c9}, {0x0376, 0x0443},
	{0x0375, 0x043f}, {0x0374, 0x043a}, {0x031c, 0x0080}, {0x031e, 0x0080},
	{0x031d, 0x0080}, {0x031f, 0x0080}, {0x0351, 0x0000}, {0x0352, 0x0000},
	{0x0353, 0x0000}, {0x034d, 0x0000}, {0x034e, 0x0000}, {0x034f, 0x0000},
	{0x0350, 0x009f}, {0x03a9, 0x0100}, {0x00ec, 0x0edb}, {0x00ed, 0x01ab},
	{0x00ee, 0x0003}, {0x00ef, 0x0d1d}, {0x00f0, 0x0172}, {0x00f1, 0x0003},
	{0x00f2, 0x0c77}, {0x00f3, 0x00a9}, {0x00f4, 0x0003}, {0x00f5, 0x0082},
	{0x0164, 0x4887}, {0x07d5, 0x0038}, {0x07d4, 0x0028}, {0x07d2, 0x001f},
	{0x07d3, 0x001f}, {0x07d0, 0xffff}, {0x07d1, 0xffff}, {0x016a, 0x003f},
	{0x0c11, 0x2bb7}, {0x0c10, 0x1841}, {0x015e, 0x3840}, {0x016c, 0x0040},
	{0x06f9, 0x1602}, {0x016c, 0x0000}, {0x06dc, 0x016a}, {0x06dd, 0x0624},
	{0x06de, 0x016a}, {0x06df, 0x0014}, {0x06e0, 0x013a}, {0x06e1, 0x0004},
	{0x06e4, 0x0128}, {0x06e5, 0x0014}, {0x06e2, 0x015a}, {0x06e3, 0x0008},
	{0x06ee, 0x0001}, {0x0071, 0x04aa}, {0x0461, 0xffff}, {0x0462, 0x003c},
	{0x0645, 0x02c5}, {0x064c, 0x02c5}, {0x0644, 0x0019}, {0x0392, 0xffff},
	{0x0393, 0xffff}, {0x0272, 0x0200}, {0x0271, 0x0020}, {0x06a0, 0x006b},
	{0x06a1, 0x03ed}, {0x0176, 0x0016}, {0x0401, 0x1111}, {0x0417, 0x0004},
	{0x0720, 0x0180}, {0x0721, 0x5000}, {0x0729, 0x0000}, {0x0723, 0x0000},
	{0x0726, 0x000c}, {0x073b, 0x002c}, {0x073c, 0x0030}, {0x0070, 0x0000},
	{0x0678, 0x15ad}, {0x07bc, 0x0044}, {0x07bd, 0x004e}, {0x07be, 0x0054},
	{0x0400, 0x0000}, {0x0725, 0x0600}, {0x073a, 0x0180}, {0x0c30, 0x033f},
	{0x0c31, 0x06c0}, {0x0080, 0x0008}, {0x06d4, 0x0c61}, {0x02ed, 0x001c},
	{0x02f1, 0x001c}, {0x02f5, 0x001c}, {0x02f9, 0x001c}, {0x0339, 0x0fff},
	{0x016e, 0x001f}, {0x016f, 0x07d0}, {0x0170, 0x07d0}, {0x019e, 0x0250},
	{0xffff, 0xffff}
};

fcbs_input_data_t dsi_data[] = {
	{FCBS_PHY_TBL, ARRAYSIZE(tbl_info), 0, tbl_info},
	{FCBS_PHY_REG, ARRAYSIZE(phyregs_adp), 0, phyregs_adp},
	{FCBS_RADIO_REG, ARRAYSIZE(radioregs_adp), 0, radioregs_adp},
	{FCBS_TYPE_MAX, NULL, NULL, NULL}
};

static void
dsi_prototype_restore_prefvals(phy_info_t *pi)
{
	int i = 0;

	while (maj1_min2_phy_reg_prefvals[i].addr != 0xffff) {
		phy_utils_write_phyreg(pi, (uint16)maj1_min2_phy_reg_prefvals[i].addr,
				(uint16)maj1_min2_phy_reg_prefvals[i].data);
		i++;
	}
}

static void
dsi_prototype_restore_wars(phy_info_t *pi)
{
	/* Enable the uCode TSSI_DIV WAR */
	if ((ACMAJORREV_2(pi->pubpi->phy_rev)) &&
	    BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
		wlapi_bmac_mhf(pi->sh->physhim, MHF2, MHF2_PPR_HWPWRCTL, MHF2_PPR_HWPWRCTL,
		               WLC_BAND_ALL);
	}
}

static void
dsi_prototype_restore_phytables(phy_info_t *pi)
{
	uint8 i = 0;
	phytbl_t *ti = tbl_info;

	for (i = 0; i < ARRAYSIZE(tbl_info); i++, ti++)
		wlc_phy_table_write_acphy(pi, ti->id, ti->len, ti->offset, ti->width, ti->data);
}

static void
dsi_prototype_restore_phyregs(phy_info_t *pi)
{
	uint16 i = 0;

	for (i = 0; i < ARRAYSIZE(phyregs_adp); i++) {
		phy_utils_write_phyreg(pi,
				(uint16)phyregs_adp[i].addr,
				(uint16)phyregs_adp[i].data);
	}
}

static void
dsi_prototype_restore_radioregs(phy_info_t *pi)
{
	uint16 i = 0;

	for (i = 0; i < ARRAYSIZE(radioregs_adp); i++) {
		phy_utils_write_radioreg(pi,
				(uint16)radioregs_adp[i].addr,
				(uint16)radioregs_adp[i].data);
	}

	phy_ac_dsi_radio_fns(pi);
}

static void
dsi_prototype_save_phytables(phy_info_t *pi)
{
	uint16 i = 0;

	phytbl_t *ti = tbl_info;

	sz_TXEVMTBL          = ARRAYSIZE(data_TXEVMTBL);
	sz_GAINLIMIT         = ARRAYSIZE(data_GAINLIMIT);
	sz_ESTPWRSHFTLUTS    = ARRAYSIZE(data_ESTPWRSHFTLUTS);
	sz_NVRXEVMSHAPINGTBL = ARRAYSIZE(data_NVRXEVMSHAPINGTBL);
	sz_IQLOCAL           = ARRAYSIZE(data_IQLOCAL);
	sz_FEMCTRLLUT        = ARRAYSIZE(data_FEMCTRLLUT);
	sz_ESTPWRLUTS0       = ARRAYSIZE(data_ESTPWRLUTS0);
	sz_RFSEQBUNDLE       = ARRAYSIZE(data_RFSEQBUNDLE);
	sz_CHANNELSMOOTHING  = ARRAYSIZE(data_CHANNELSMOOTHING);
	sz_PAPR              = ARRAYSIZE(data_PAPR);
	sz_EPSILON0          = ARRAYSIZE(data_EPSILON0);
	sz_RSSICLIPGAIN0     = ARRAYSIZE(data_RSSICLIPGAIN0);
	sz_PHASETRACKTBL     = ARRAYSIZE(data_PHASETRACKTBL);
	sz_NVNOISESHAPINGTBL = ARRAYSIZE(data_NVNOISESHAPINGTBL);
	sz_RFSEQEXT          = ARRAYSIZE(data_RFSEQEXT);

	for (i = 0; i < ARRAYSIZE(tbl_info); i++, ti++)
		wlc_phy_table_read_acphy(pi, ti->id, ti->len, ti->offset, ti->width, ti->data);
}

static void
dsi_prototype_save_phyregs(phy_info_t *pi)
{
	uint16 i = 0;
	sz_phyregs = ARRAYSIZE(phyregs_adp);

	for (i = 0; i < sz_phyregs; i++)
		phyregs_adp[i].data = phy_utils_read_phyreg(pi, (uint16)phyregs_adp[i].addr);
}

static void
dsi_prototype_save_radioregs(phy_info_t *pi)
{
	uint16 i = 0;
	sz_radioregs = ARRAYSIZE(radioregs_adp);

	for (i = 0; i < sz_radioregs; i++)
		radioregs_adp[i].data = phy_utils_read_radioreg(pi, (uint16)radioregs_adp[i].addr);
}

/* Prints RawData FCBS sequences from fcbs_input_data_t struct */
static void
dsi_rawdata(phy_info_t *pi, fcbs_input_data_t *input)
{
	int i = 0;

	while (input->type != FCBS_TYPE_MAX) {
		DSI_DBG_PROTO(("\n"));
		DSI_DBG_PROTO(("**** Type %d, Sz %d, Flags 0x%04x ****\n",
				input->type, input->data_size, input->flags));
		switch (input->type) {
			case FCBS_PHY_REG:
			case FCBS_RADIO_REG:
				{
					adp_t *tmp = (adp_t *)input->data;
					for (i = 0; i < input->data_size; i++, tmp++) {
						DSI_DBG_PROTO(("addr: 0x%04x | data: 0x%04x\n",
								tmp->addr, tmp->data));
					}
					break;
				}
			case FCBS_DELAY:
				{
					uint16 *tmp = (uint16 *)input->data;
					for (i = 0; i < input->data_size; i++, tmp++) {
						DSI_DBG_PROTO(("delay 0x%03x\n", *tmp));
					}
					break;
				}
			case FCBS_PHY_TBL:
				{
					phytbl_t *tmp = (phytbl_t *)input->data;
					for (i = 0; i < input->data_size; i++, tmp++) {
						DSI_DBG_PROTO(("id: %03d | len : %03d |"
								" o : 0x%04x | w : %02d\n",
								tmp->id, tmp->len,
								tmp->offset, tmp->width));
					}
					break;
				}
			default:
				DSI_DBG_PROTO(("Error! Invalid Type!\n"));
				break;
		}
		input++;
	}
}

static void
dsi_populate_fcbs(phy_info_t *pi, fcbs_input_data_t *input, int ds_inx)
{
	uint16 sz = 0;

	/* base ptr */
	fcbs_input_data_t *fin = (fcbs_input_data_t *)input;
	BCM_REFERENCE(fin);

	/* Prints RawData FCBS sequences */
	dsi_rawdata(pi, input);

	while (input->type != FCBS_TYPE_MAX) {
		input++;
		sz++;
	}
}
