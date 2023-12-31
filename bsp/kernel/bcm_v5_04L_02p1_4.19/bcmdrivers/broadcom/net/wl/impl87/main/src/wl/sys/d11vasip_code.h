/*
 * VASIP init declarations for Broadcom 802.11
 * Networking Adapter Device Driver.
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
 * $Id: d11vasip_code.h 804728 2021-11-09 00:45:24Z $
 */

/* vasip code and inits */

#ifndef __D11VASIP_CODE_H__
#define __D11VASIP_CODE_H__

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmutils.h>

/* VASIP FW Version 6.5205 */
#define VASIP_FW_VER_MAJOR 6
#define VASIP_FW_VER_MINOR 5205

extern CONST uint32 d11vasipcode_major;
extern CONST uint32 d11vasipcode_minor;
enum {
	__vasip_map_MU_OUTPUT                            = 0,
	__vasip_map_bfd_bf_rpt_buffer                    = 1,
	__vasip_map_bfd_cqi_rpt_buffer                   = 2,
	__vasip_map_bfds_log_buffer                      = 3,
	__vasip_map_bfds_mlbf_angle_buffer               = 4,
	__vasip_map_csi_rpt0                             = 5,
	__vasip_map_csi_rpt1                             = 6,
	__vasip_map_delay_grouping_us                    = 7,
	__vasip_map_delay_precoding_us                   = 8,
	__vasip_map_delay_rualloc_us                     = 9,
	__vasip_map_err_code                             = 10,
	__vasip_map_err_code_bfdcmd7                     = 11,
	__vasip_map_err_code_group                       = 12,
	__vasip_map_err_code_precoder                    = 13,
	__vasip_map_err_count                            = 14,
	__vasip_map_grouping_forced                      = 15,
	__vasip_map_grouping_forced_mcs                  = 16,
	__vasip_map_grouping_method                      = 17,
	__vasip_map_grouping_number                      = 18,
	__vasip_map_grp_forced_buf                       = 19,
	__vasip_map_grp_forced_mand_buf                  = 20,
	__vasip_map_grp_history_buf                      = 21,
	__vasip_map_hwsch_cmd_log                        = 22,
	__vasip_map_hwsch_v2m_buf                        = 23,
	__vasip_map_imbf_rpt                             = 24,
	__vasip_map_interrupt                            = 25,
	__vasip_map_m2v_buf0                             = 26,
	__vasip_map_m2v_buf1                             = 27,
	__vasip_map_m2v_buf_cqi                          = 28,
	__vasip_map_m2v_buf_grp_on_the_fly               = 29,
	__vasip_map_m2v_buf_grp_on_the_fly_he            = 30,
	__vasip_map_m2v_buf_grp_sel                      = 31,
	__vasip_map_m2v_buf_precoder                     = 32,
	__vasip_map_m2v_buf_ru_alloc                     = 33,
	__vasip_map_m2v_buf_snd_update                   = 34,
	__vasip_map_mcs_capping_enable                   = 35,
	__vasip_map_mcs_map                              = 36,
	__vasip_map_mcs_overwrite_flag                   = 37,
	__vasip_map_mi_mcs_recommend_enable              = 38,
	__vasip_map_plugfest_en                          = 39,
	__vasip_map_recommend_mcs                        = 40,
	__vasip_map_sch_phybw                            = 41,
	__vasip_map_sgi_method                           = 42,
	__vasip_map_sgi_threshold_80M                    = 43,
	__vasip_map_snr_calib_en                         = 44,
	__vasip_map_snr_calib_grp_en                     = 45,
	__vasip_map_snr_calib_su_lowlimit                = 46,
	__vasip_map_snr_calib_value                      = 47,
	__vasip_map_spectrumAnyzIn                       = 48,
	__vasip_map_spectrumAnyzInfo                     = 49,
	__vasip_map_spectrumAnyzOut                      = 50,
	__vasip_map_steering_mcs                         = 51,
	__vasip_map_txbf_ppr_tbl                         = 52,
	__vasip_map_txv_decompressed_report              = 53,
	__vasip_map_v2m_buf_cqi                          = 54,
	__vasip_map_v2m_buf_grp                          = 55,
	__vasip_map_v2m_buf_grp_he                       = 56,
	__vasip_map_v2m_buf_grp_on_the_fly               = 57,
	__vasip_map_v2m_buf_grp_on_the_fly_he            = 58,
	__vasip_map_v2m_buf_mvp                          = 59,
	__vasip_map_v2m_buf_ru_alloc                     = 60,
	__vasip_map_v2m_buf_snd_update                   = 61,
	vasipfw_symbol_count                             = 62
};

typedef struct {
	CONST uint32 *map;
	CONST uint16 *size;
	CONST uint32 *code;
	CONST uint32 *data;
	CONST uint code_size;
	CONST uint data_size;
	CONST uint ntx;
} vasip_fw_t;

extern CONST vasip_fw_t vasip_11ac_4x4_fw;

extern CONST vasip_fw_t vasip_11ac_3x3_fw;

extern CONST vasip_fw_t vasip_11ax_4x4_fw;

extern CONST vasip_fw_t vasip_airiq_11ac_fw;

extern CONST vasip_fw_t vasip_11ax_2x2_fw;

extern CONST vasip_fw_t vasip_11ax_3x3_fw;

extern CONST vasip_fw_t vasip_11ax_4x4_wav2_fw;

extern CONST vasip_fw_t vasip_11ax_2x2_160_fw;

#endif /* __D11VASIP_CODE_H__ */
