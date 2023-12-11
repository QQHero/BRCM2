/*
 * CLM API functions.
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
 * $Id: wlc_clm.h 821810 2020-06-17 12:41:05Z $
 */

#ifdef _MSC_VER
	#pragma warning(push, 3)
#endif /* _MSC_VER */

/* Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags through this file.
 */
#include <wlc_cfg.h>

#include <bcmwifi_rates.h>
#include <typedefs.h>

#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include "wlc_clm.h"
#include "wlc_clm_data.h"
#ifdef _MSC_VER
	#pragma warning(pop)
#endif /* _MSC_VER */

/******************************
* MODULE MACROS AND CONSTANTS *
*******************************
*/

/* BLOB format version major number */
#define FORMAT_VERSION_MAJOR 27

/* BLOB format version minor number */
#define FORMAT_VERSION_MINOR 1

/* Minimum supported binary BLOB format's major version */
#define FORMAT_MIN_COMPAT_MAJOR 7

#if (FORMAT_VERSION_MAJOR != CLM_FORMAT_VERSION_MAJOR) || (FORMAT_VERSION_MINOR != \
	CLM_FORMAT_VERSION_MINOR)
#error CLM data format version mismatch between wlc_clm.c and wlc_clm_data.h
#endif

#ifndef NULL
	/** Null pointer */
	#define NULL 0
#endif

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

#ifndef OFFSETOF
	/** Offset of given field in given structure */
	#define OFFSETOF(s, m) (uintptr)&(((s *)0)->m)
#endif /* OFFSETOF */

#ifndef ARRAYSIZE
	/** Number of elements in given array */
	#define ARRAYSIZE(x) (uint32)(sizeof(x)/sizeof(x[0]))
#endif

/** Number of elements in array field of given structure) */
#define FIELD_ARRAYSIZE(s, m) (sizeof(((s *)0)->m)/sizeof((((s *)0)->m)[0]))

#if WL_NUMRATES >= 178
	/** Defined if bcmwifi_rates.h contains TXBF rates */
	#define CLM_TXBF_RATES_SUPPORTED
#endif

#if WL_NUMRATES >= 336
	/** Defined if EXT4 rates (SU rates with code s greater than 2*256) are
	 * used
	 */
	#define CLM_EXT4_RATES_SUPPORTED
#endif /* WL_NUMRATES >= 336 */

#if WL_NUMRATES > 564
	/** EHT rates present */
	#define CLM_EHT_RATES_SUPPORTED
#endif /* WL_NUMRATES > 564 */

#if defined(WL_RU_NUMRATES) && (WL_RU_NUMRATES >= 342)
	/** Defined if EXT RU rates (RU rates with code greater than 256) are
	 * used
	 */
	#define CLM_EXT_RU_RATES_SUPPORTED
#endif /* defined(WL_RU_NUMRATES) && (WL_RU_NUMRATES >= 342) */

#ifdef WL11BE
	/** 320MHz channels supported */
	#define CLM_320_MHZ_SUPPORTED
#endif /* WL11BE */

#ifdef WL_NUM_HE_RT
#if WL_NUM_HE_RT > 9
	/** RU996X2 rates supported */
	#define CLM_RU996_2_SUPPORTED
#endif /* WL_NUM_HE_RT > 9 */
#if WL_NUM_HE_RT > 10
	/** MRU rates supported */
	#define CLM_MRU_RATES_SUPPORTED
#endif /* WL_NUM_HE_RT > 10 */
#endif /* WL_NUM_HE_RT */

#ifndef BCMRAMFN
	#define BCMRAMFN(x) x
#endif /* BCMRAMFN */
#ifndef BCM_NOINLINE
	#define BCM_NOINLINE
#endif /* BCM_NOINLINE */

/** CLM data source IDs */
typedef enum data_source_id {
	/** Incremental CLM data. Placed first so we look there before base
	 * data
	 */
	DS_INC	= 0,

	/** Main CLM data */
	DS_MAIN	= 1,

	/** Number of CLM data source IDs */
	DS_NUM
} data_source_id_t;

/** Indices in base_ht_loc_data[] vector used by some function and containing
 * data pertinent to base and HT locales
 */
typedef enum base_ht_id {
	/** Index for base locale */
	BH_BASE	= 0,

	/** Index for HT locale */
	BH_HT	= 1,

	/** Number of indices (length of base_ht_loc_data vector) */
	BH_NUM
} base_ht_id_t;

/** Module constants */
enum clm_internal_const {
	/** MASKS THAT DEFINE ITERATOR CONTENTS */

	/** Pointed data is in main CLM data source */
	ITER_DS_MAIN = 0x40000000,

	/** Pointed data is in incremental CLM data source */
	ITER_DS_INC = 0x00000000,

	/** Mask of data source field of iterator */
	ITER_DS_MASK = 0x40000000,

	/** Mask of index field of iterator */
	ITER_IDX_MASK = 0x3FFFFFFF,

	/** Number of MCS/OFDM rates, differing only by modulation */
	NUM_MCS_MODS = 8,

	/** Number of DSSS rates, differing only by modulation */
	NUM_DSSS_MODS = 4,

	/** Mask of count field in subchannel path descriptor */
	SUB_CHAN_PATH_COUNT_MASK = 0xF0,

	/** Offset of count field in subchannel path descriptor */
	SUB_CHAN_PATH_COUNT_OFFSET = 4,

	/** Prefill constant for power limits used in clm_limits() */
	UNSPECIFIED_POWER = CLM_DISABLED_POWER + 1,

	/** Minimum possible 8-bit qdBm power */
	MIN_TX_POWER = UNSPECIFIED_POWER + 1,

	/** Maximum possible 8-bit qdBm power */
	MAX_TX_POWER = 0x7F,

	/** clm_country_locales::computed_flags: country flags taken from main
	 * data
	 */
	COUNTRY_FLAGS_DS_MAIN = (uint8)DS_MAIN,

	/** clm_country_locales::computed_flags: country flags taken from
	 * incremental data
	 */
	COUNTRY_FLAGS_DS_INC = (uint8)DS_INC,

	/** clm_country_locales::computed_flags: mask for country flags source
	 * field
	 */
	COUNTRY_FLAGS_DS_MASK = (uint8)(DS_NUM - 1),

#ifdef CLM_EXT4_RATES_SUPPORTED
	/** Base value for rates in extended 4TX rate set */
	BASE_EXT4_RATE = WL_RATE_1X4_DSSS_1,
#endif /* CLM_EXT4_RATES_SUPPORTED */

#ifdef CLM_MRU_RATES_SUPPORTED
	/** Base rate for extended (M)RU rates */
	BASE_EXT_RU_RATE = WL_RU_RATE_1X1_996_484SS1,
#endif /* CLM_MRU_RATES_SUPPORTED */

	/** Base value for rates in extended rate set */
	BASE_EXT_RATE = WL_RATE_1X3_DSSS_1,

	/** Shift value for hash function that computes index of TX mode for
	 * HE0 rates
	 */
	SU_TX_MODE_HASH_SHIFT = 4,

	/** Mask value for hash function that computes index of TX mode index
	 * for HE0 rates
	 */
	SU_TX_MODE_HASH_MASK = 0x3F,

	/** Number of qdBm in dBm */
	QDBM_IN_DBM = 4,

	/** Number of EHT SS1 rates past HE rates */
	EHT_SS1_NUM = 4,

	/** Number of EHT SS2+ rates past HE rates */
	EHT_SS2P_NUM = 2,

	/** Number of valid band/bandwidth pairs */
#ifdef WL_BAND6G
#ifdef CLM_320_MHZ_SUPPORTED
	BB_NUM = 2 + 5 + 5,	/* 2G + 5G + 6G */
#else /* CLM_320_MHZ_SUPPORTED */
	BB_NUM = 2 + 5 + 4,	/* 2G + 5G + 6G */
#endif /* else CLM_320_MHZ_SUPPORTED */
#else /* WL_BAND6G */
	BB_NUM = 2 + 5,		/* 2G + 5G */
#endif /* else WL_BAND6G */

	/** Length of CLM_BW_NUM-dependent certain fields in data_dsc_t and
	 * locale_data_t, made compatible with ROMs, made before punctured
	 * bandwidth' introduction. Using these value prevents massive
	 * invalidation and should be safe as no data for punctured bandwidths
	 * is stored anyway
	 */
#if defined(WL11BE)
	CLM_BW_NUM_ROM_COMPAT = CLM_BW_160_160 + 1,
#else /* defined(WL11BE) */
	CLM_BW_NUM_ROM_COMPAT = CLM_BW_NUM,
#endif /* defined(WL11BE) */

	/** Number of valid band/bandwidth pairs for 40+ MHz bandwidth */
#ifdef WL_BAND6G
#ifdef CLM_320_MHZ_SUPPORTED
	BB40_NUM = 1 + 4 + 4,	/* 2G + 5G + 6G */
#else /* CLM_320_MHZ_SUPPORTED */
	BB40_NUM = 1 + 4 + 3,	/* 2G + 5G + 6G */
#endif /* else CLM_320_MHZ_SUPPORTED */
#else /* WL_BAND6G */
	BB40_NUM = 1 + 4,	/* 2G + 5G */
#endif /* else WL_BAND6G */
};

/** Rate types */
typedef enum clm_rate_type {
	/** DSSS (802.11b) rate */
	RT_DSSS	= 0,

	/** OFDM (802.11a/g) rate */
	RT_OFDM	= 1,

	/** MCS (802.11n/ac) rate */
	RT_MCS	= 2,

	/** HE (802.11ax) SU rate */
	RT_SU	= 3,

#ifdef CLM_EHT_RATES_SUPPORTED
	/** EHT rate */
	RT_EHT	= 4,
#endif /* CLM_EHT_RATES_SUPPORTED */

	RT_NUM
} clm_rate_type_t;

/** Rate encoding types - translation of CLM_DATA_FLAG?_RATE_TYPE_... flags */
typedef enum clm_rate_code_type {
	/** SU rate, encoded as is */
	RCT_MAIN	= 0,

	/** SU rate, encoded as offset from first 3Tx rate */
	RCT_EXT		= 1,

	/** SU rate, encoded as offset from first 4Tx rate */
	RCT_EXT4	= 2,

	/** HE/RU rate, encoded as is */
	RCT_HE		= 3,

	/** (M)RU rate, encoded as offset from first 160MHz MRU rate */
	RCT_EXT_RU	= 4,

	RCT_NUM,

	/** Minimum RU rate encoding */
	RCT_MIN_RU = RCT_HE
} clm_rate_code_type_t;

/** Format of CC/rev representation in aggregations and advertisings */
typedef enum clm_ccrev_format {
	/** As clm_cc_rev_t */
	CCREV_FORMAT_CC_REV	= 0,

	/** As 8-bit index to region table */
	CCREV_FORMAT_CC_IDX8	= 1,

	/** As 16-bit index to region table */
	CCREV_FORMAT_CC_IDX16	= 2
} clm_ccrev_format_t;

/** Internal type for regrevs */
typedef uint16 regrev_t;

/** CLM data set descriptor */
typedef struct data_dsc {
	/** Pointer to registry (TOC) structure of CLM data */
	const clm_registry_t *data;

	/** Relocation factor of CLM DATA set: value that shall be added to
	 * pointer contained in CLM data set to get a true pointer (e.g. 0 if
	 * data is not relocated)
	 */
	uintptr relocation;

	/** Valid channel comb sets (per band, per bandwidth). Empty for
	 * 80+80
	 */
	clm_channel_comb_set_t valid_channel_combs[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** 40+MHz combs stored in BLOB - obsoleted */
	bool has_high_bw_combs_obsoleted;

	/** Index within clm_country_rev_definition_cd10_t::extra of byte with
	 * bits 9-16 of rev. -1 for 8-bit revs
	 */
	int32 reg_rev16_idx;

	/** Length of region record in bytes */
	uint32 country_rev_rec_len;

	/** Address of header for version strings */
	const clm_data_header_t *header;

	/** True if BLOB capable of containing 160MHz data - obsoleted */
	bool has_160mhz_obsoleted;

	/** Obsoleted */
	const clm_channel_range_t *chan_ranges_bw_obsoleted[CLM_BW_NUM_ROM_COMPAT];

	/** Per-band-bandwidth base addresses of rate set definitions */
	const uint8 *rate_sets_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Index within clm_country_rev_definition_cdXX_t::extra of byte with
	 * bits 9-10 of locale index. -1 for 8-bit locale indices
	 */
	int32 reg_loc10_idx;

	/** Index within clm_country_rev_definition_cdXX_t::extra of byte with
	 * bits 11-12 of locale index. -1 for 8 and 10 bit locale indices
	 */
	int32 reg_loc12_idx;

	/** Index within clm_country_rev_definition_cdXX_t::extra of byte with
	 * region flags. -1 if region has no flags
	 */
	int32 reg_flags_idx;

	/** CC/revs representation in aggregations */
	clm_ccrev_format_t ccrev_format;

	/** Per-band-bandwidth base addresses of extended rate set definitions */
	const uint8 *ext_rate_sets_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** True if BLOB contains ULB channels - obsoleted */
	bool has_ulb_obsoleted;

	/** If BLOB has regrev remaps - pointer to remap set structure */
	const clm_regrev_cc_remap_set_t *regrev_remap;

	/** 'flags' from clm_data_registry or 0 if there is no flags there */
	uint32 registry_flags;

	/** Index within clm_country_rev_definition_cdXX_t::extra second of
	 * byte with region flags. -1 if region has no second byte of flags
	 */
	int32 reg_flags_2_idx;

	/** 4-bit subchannel index (for backward compatibility) */
	bool scr_idx_4;

	/** 'flags2' from clm_data_registry or 0 if there is no flags there */
	uint32 registry_flags2;

	/** Index within clm_country_rev_definition_cdXX_t::extra of byte with
	* bits 13-14 of locale index. -1 for 8, 10 and 12 bit locale indices
	*/
	int32 reg_loc14_idx;

	/** Obsoleted */
	const uint8 *he_rate_sets_obsoleted;

	/** Descriptors of HE rates, used in BLOB */
	const clm_he_rate_dsc_t *he_rate_dscs;

	/** Per-band-bandwidth base addresses of extended 4TX rate set
	 * definitions
	 */
	const uint8 *ext4_rate_sets_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Per-band-bandwidth base addresses of OFDMA rate set definitions */
	const uint8 *he_rate_sets_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Per-band-bandwidth base addresses of channel range descriptors */
	const clm_channel_range_t *chan_ranges_band_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Index within clm_country_rev_definition_cdXX_t::extra of byte with
	 * bits 9-12 of 6GHz locale indices. -1 for 8 bit locale indices
	 */
	int32 reg_loc12_6g_idx;

	/** Index within clm_country_rev_definition_cdXX_t::extra of byte with
	 * bits 13-16 of 6GHz locale indices. -1 for 8 and 12 bit locale indices
	 */
	int32 reg_loc16_6g_idx;

	/** Mask that covers valid bits in locale index - used for
	 * interpretation of special indices (CLM_LOC_... constants, that are
	 * truncated in BLOB)
	 */
	uint32 loc_idx_mask;

	/** Per-band-bandwidth indices of rate set definitions */
	const uint16 *rate_sets_indices_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Per-band-bandwidth indices of ext rate set definitions */
	const uint16 *ext_rate_sets_indices_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Per-band-bandwidth indices of ext4 rate set definitions */
	const uint16 *ext4_rate_sets_indices_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Per-band-bandwidth indices of HE/RU rate set definitions */
	const uint16 *he_rate_sets_indices_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Per-band-bandwidth indices of HE/RU EXT rate set definitions */
	const uint16 *ext_ru_rate_sets_indices_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];

	/** Per-band-bandwidth base addresses of OFDMA EXT rate set definitions */
	const uint8 *ext_ru_rate_sets_bw[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];
} data_dsc_t;

/** Addresses of locale-related data */
typedef struct locale_data {
	/** Locale definition */
	const uint8 *tx_limits;

	/** Per-bandwidth base addresses of channel range descriptors */
	const clm_channel_range_t * const *chan_ranges_bw;

	/** Per-bandwidth base addresses of rate set definitions */
	const uint8 * const *rate_sets_bw;

	/** Base address of valid channel sets definitions */
	const uint8 *valid_channels;

	/** Base address of restricted sets definitions */
	const uint8 *restricted_channels;

	/** Per-bandwidth channel combs */
	const clm_channel_comb_set_t *combs[CLM_BW_NUM_ROM_COMPAT];

	/** 80MHz subchannel rules - obsoleted */
	clm_sub_chan_region_rules_80_t sub_chan_channel_rules_80_obsoleted;

	/** 160MHz subchannel rules - obsoleted */
	clm_sub_chan_region_rules_160_t sub_chan_channel_rules_160_obsoleted;

	/** Per-bandwidth base addresses of extended rate set definitions */
	const uint8 * const *ext_rate_sets_bw;

	/** Obsoleted */
	const uint8 *he_rate_sets;

	/** Per-bandwidth base addresses of extended 4TX rate set definitions */
	const uint8 * const *ext4_rate_sets_bw;

	/** Per-bandwidth base addresses of OFDMA rate set definitions */
	const uint8 * const *he_rate_sets_bw;

	/** Per-bandwidth indices of rate set definitions */
	const uint16 * const *rate_sets_indices_bw;

	/** Per-bandwidth indices of ext rate set definitions */
	const uint16 * const *ext_rate_sets_indices_bw;

	/** Per-bandwidth indices of ext4 rate set definitions */
	const uint16 * const *ext4_rate_sets_indices_bw;

	/** Per-bandwidth indices of HE/RU rate set definitions */
	const uint16 * const *he_rate_sets_indices_bw;

	/** Data set that contains locale definition */
	data_dsc_t *ds;

	/** Base locale header bytes for base locale, NULL for HT locale */
	const uint8 *base_hdr;

	/** Pointer to regulatory channel limits. NULL for HT locale */
	const uint8 *reg_limits;

	/** Pointer to regulatory PSD limits. NULL for HT locale */
	const uint8 *psd_limits;

	/** Per-bandwidth base addresses of OFDMA EXT rate set definitions */
	const uint8 * const *ext_ru_rate_sets_bw;

	/** Per-bandwidth indices of HE/RU EXT rate set definitions */
	const uint16 * const *ext_ru_rate_sets_indices_bw;
} locale_data_t;

/** Addresses of region (country)-related data. Replaces previous, completely
 * obsoleted layout
 */
typedef struct country_data_v3 {
	/** Subchannel rules. Compressed [CLM_BAD_NUM][CLM_BW_NUM] array */
	clm_sub_chan_region_rules_t sub_chan_channel_rules[BB40_NUM];

	/** Power increments (offsets) for subchannel rules. Compressed
	 * [CLM_BAD_NUM][CLM_BW_NUM] array
	 */
	const int8 *sub_chan_increments[BB40_NUM];

	/** Per-band-bandwidth base addresses of channel range descriptors.
	 * References correspondent field in data_dsc_t. Used in subchannel
	 * rules' computation where, in case of incremental BLOB use, locale
	 * subchannel ranges can't be used (as they may come from different
	 * data source)
	 */
	const clm_channel_range_t *(*chan_ranges_band_bw)[CLM_BW_NUM_ROM_COMPAT];
} country_data_v3_t;

/** Information about aggregation */
typedef struct aggregate_data {
	/** Default region */
	clm_cc_rev4_t def_reg;

	/** Number of regions */
	int32 num_regions;

	/** Pointer to vector of regions in BLOB-specific format */
	const void *regions;
} aggregate_data_t;

/** Locale type descriptor */
typedef struct loc_type_dsc {
	/** Band */
	clm_band_t band;

	/* Locale flavor (base/HT) */
	base_ht_id_t flavor;

	/** Offset of locale definition field in clm_country_locales_t */
	uint32 def_field_offset;

	/** Offset of locales' definition field in clm_registry_t */
	uint32 loc_def_field_offset;
} loc_type_dsc_t;

/** Descriptor of single bit flag translation */
typedef struct flag_translation {
	/** Source bitmask */
	uint32 from;

	/** Resulting bitmask */
	uint32 to;
} flag_translation_t;

/** Flag translation set descriptor */
typedef struct flag_translation_set {
	/** Set (vector of flag translation descriptors */
	const flag_translation_t *set;

	/** Number of descriptors in set */
	uint32 num;
} flag_translation_set_t;

/** Identifiers of flag translation sets */
typedef enum flag_translation_set_id {
	/** Base locale flags */
	FTS_BASE		= 0,

	/** Country (region) flags, first byte */
	FTS_COUNTRY_FLAG_1	= 1,

	/** Country (region) flags, second byte */
	FTS_COUNTRY_FLAG_2	= 2,

	/** Number of IDs */
	FTS_NUM
} flag_translation_set_id_t;

#ifdef WL_RU_NUMRATES
/** Data for deriving MRU power from correspondent HE0 power */
typedef struct ru_derivation {
	/** (M)RU rate type */
	clm_ru_rates_t rate;

	/** Backoff of rates's power level relative to full-channel HE0 rate */
	int8 backoff;
} ru_derivation_t;
#endif /* WL_RU_NUMRATES */

/** Various bandwidth properties */
typedef struct bandwidth_traits {
	/** True if supported (some bandwidths declared prematurely) */
	bool supported;

	/** Mask in multibandwidth subchannel rule. 0 when irrelevant */
	uint8 sc_mask;

	/** Half step in channel numbering. WL_BW_NUM when irrelevant  */
	uint32 half_stride;

#ifdef WL_NUM_HE_RT
	/** UL RU rate type that covers entire channel. WL_NUM_HE_RT when
	 * irrelevant
	 */
	wl_he_rate_type_t ru_rate_type;
#endif /* WL_NUM_HE_RT */

#ifdef WL_RU_NUMRATES
	/* Whole-channel OFDMA UL RU rate. WL_RU_NUMRATES if irrelevant */
	clm_ru_rates_t channel_ru_rate;
#endif /* WL_RU_NUMRATES */

	/** Half-bandwidth. WL_BW_NUM when irrelevant */
	clm_bandwidth_t half_bw;

	/** Bandwidth of itself for normal channel, bandwidth of contiguous part
	 * for noncontiguous channel
	 */
	clm_bandwidth_t active;

	/** Maximum channel type (number of valid subchannel indices) or ~0u */
	uint32 num_subchannels;

#ifdef CLM_MRU_RATES_SUPPORTED
	/** (M)RU derivations that can be made from HE0 rate on this bandwidth.
	 * Includes full-channel RU rate (from channel_ru_rate)
	 */
	const ru_derivation_t *ru_derivations;

	/** Number of MRU rate derivatons */
	uint32 num_ru_derivations;
#endif /* CLM_MRU_RATES_SUPPORTED */
#ifdef WL11BE
	/** For punctured bandwidth - bandwidth of corredspondent nonpunctured
	 * channels. For nonpunctured bandwidth - CLM_BW_NUM
	 */
	clm_bandwidth_t punctured_base_bw;

	/** For punctured bandwidth - SU backoff (qdB to subtract from base
	 * bandwidth SU limits). For nonpunctured banddwidth - 0
	*/
	uint32 punctured_su_backoff;

	/** For punctured bandwidth - bitmask of valid wl_he_rate_type_t values.
	 * For nonpunctured bandwidth - 0
	 */
	uint32 punctured_rate_type_mask;
#endif /* WL11BE */
} bandwidth_traits_t;

/** Descriptors of main and incremental data sources */
static data_dsc_t data_sources[] = {
	{ NULL, 0, {{{0, 0}}}, 0, 0, 0, NULL, 0, {NULL}, {{NULL}}, 0, 0, 0,
	(clm_ccrev_format_t)0, {{NULL}}, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL,
	{{NULL}}, {{NULL}}, {{NULL}}, 0, 0, 0, {{NULL}}, {{NULL}}, {{NULL}},
	{{NULL}}, {{NULL}}, {{NULL}} },
	{ NULL, 0, {{{0, 0}}}, 0, 0, 0, NULL, 0, {NULL}, {{NULL}}, 0, 0, 0,
	(clm_ccrev_format_t)0, {{NULL}}, 0, NULL, 0, 0, 0, 0, 0, NULL, NULL,
	{{NULL}}, {{NULL}}, {{NULL}}, 0, 0, 0, {{NULL}}, {{NULL}}, {{NULL}},
	{{NULL}}, {{NULL}}, {{NULL}} }
};

/** Rate type by rate index. Values are from enum clm_rate_type, compressed to
 * 2-bits (if no EHT rates) ofr 4 bits (if there are EHT rates)
 */
#ifdef CLM_EHT_RATES_SUPPORTED
static uint8 rate_type[(WL_NUMRATES + 1u)/2u];
#else
static uint8 rate_type[(WL_NUMRATES + 3u)/4u];
#endif /* else CLM_EHT_RATES_SUPPORTED */

/** Valid 40M channels of 2.4G band */
static const struct clm_channel_comb valid_channel_combs_2g_40m[] = {
	{  3u,  11u, 1u}, /* 3 - 11 with step of 1 */
};

/** Set of 40M 2.4G valid channel combs */
static const struct clm_channel_comb_set valid_channel_2g_40m_set = {
	1u, valid_channel_combs_2g_40m
};

/** Valid 40M channels of 5G band */
static const struct clm_channel_comb valid_channel_combs_5g_40m[] = {
	{ 38u,  62u, 8u}, /* 38 - 62 with step of 8 */
	{102u, 142u, 8u}, /* 102 - 142 with step of 8 */
	{151u, 159u, 8u}, /* 151 - 159 with step of 8 */
};

/** Set of 40M 5G valid channel combs */
static const struct clm_channel_comb_set valid_channel_5g_40m_set = {
	3u, valid_channel_combs_5g_40m
};

/** Valid 80M channels of 5G band */
static const struct clm_channel_comb valid_channel_combs_5g_80m[] = {
	{ 42u,  58u, 16u}, /* 42 - 58 with step of 16 */
	{106u, 138u, 16u}, /* 106 - 138 with step of 16 */
	{155u, 155u, 16u}, /* 155 - 155 with step of 16 */
};

/** Set of 80M 5G valid channel combs */
static const struct clm_channel_comb_set valid_channel_5g_80m_set = {
	3u, valid_channel_combs_5g_80m
};

/** Valid 160M channels of 5G band */
static const struct clm_channel_comb valid_channel_combs_5g_160m[] = {
	{ 50u,  50u, 32u}, /* 50 - 50 with step of 32 */
	{114u, 114u, 32u}, /* 114 - 114 with step of 32 */
};

/** Set of 160M 5G valid channel combs */
static const struct clm_channel_comb_set valid_channel_5g_160m_set = {
	2u, valid_channel_combs_5g_160m
};

/** Maps CLM_DATA_FLAG_WIDTH_...  to clm_bandwidth_t */
static const uint8 bw_tx_to_clm[CLM_DATA_FLAG_WIDTH_MASK + 1u] = {
	CLM_BW_20, CLM_BW_40, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_80, CLM_BW_160, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
#ifdef CLM_320_MHZ_SUPPORTED
	CLM_BW_320, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM,	CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_80_80, CLM_BW_NUM};
#else
	CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_NUM, CLM_BW_NUM,	CLM_BW_NUM, CLM_BW_NUM,
	CLM_BW_80_80, CLM_BW_NUM};
#endif /* else CLM_320_MHZ_SUPPORTED */

#if defined(WL_RU_NUMRATES)
/** SU base (modulation index 0) rate indices in same order, as in
 * he_rate_descriptors[] and in clm_ru_rates_t for a particular rate type
 */
static const uint16 su_base_rates[] = {
	WL_RATE_1X1_HE0SS1, WL_RATE_1X2_HE0SS1, WL_RATE_2X2_HE0SS2, WL_RATE_1X2_TXBF_HE0SS1,
	WL_RATE_2X2_TXBF_HE0SS2, WL_RATE_1X3_HE0SS1, WL_RATE_2X3_HE0SS2, WL_RATE_3X3_HE0SS3,
	WL_RATE_1X3_TXBF_HE0SS1, WL_RATE_2X3_TXBF_HE0SS2, WL_RATE_3X3_TXBF_HE0SS3,
	WL_RATE_1X4_HE0SS1, WL_RATE_2X4_HE0SS2, WL_RATE_3X4_HE0SS3, WL_RATE_4X4_HE0SS4,
	WL_RATE_1X4_TXBF_HE0SS1, WL_RATE_2X4_TXBF_HE0SS2, WL_RATE_3X4_TXBF_HE0SS3,
	WL_RATE_4X4_TXBF_HE0SS4,
};

/** Number of TX modes for given RU rate width */
#define CLM_NUM_RU_RATE_MODES (ARRAYSIZE(su_base_rates))

/** Hash table for computation that determines HE0 rate's TX mode index - i.e.
 * that specifies mapping inverse to specified by su_base_rates[]
 */
static uint8 he_tx_mode_hash[SU_TX_MODE_HASH_MASK + 1u];

/** Minimum bandwidths (CLM_BW_...) for various RU rates (WL_RU_RATE_...)
 * Each rate occupies half-byte
 */
static uint8 ru_rate_min_bw[(WL_RU_NUMRATES + 1u) / 2u];

/** Properties of various transmission modes for a particular rate type.
 * Descriptors follow in same order as items in su_base_rates[] and in
 * clm_ru_rates_t for a particular rate type. Items have zero 'rate_type'
 * field, because they described all HE rate types
 */
static const clm_he_rate_dsc_t he_rate_descriptors[] = {
	{0u, 1u, 1u, 0u},			/* 1X1 */
	{0u, 1u, 2u, 0u},			/* 1X2 */
	{0u, 2u, 2u, 0u},			/* 2X2 */
	{0u, 1u, 2u, CLM_HE_RATE_FLAG_TXBF},	/* 1X2_TXBF */
	{0u, 2u, 2u, CLM_HE_RATE_FLAG_TXBF},	/* 2X2_TXBF */
	{0u, 1u, 3u, 0u},			/* 1X3 */
	{0u, 2u, 3u, 0u},			/* 2X3 */
	{0u, 3u, 3u, 0u},			/* 3X3 */
	{0u, 1u, 3u, CLM_HE_RATE_FLAG_TXBF},	/* 1X3_TXBF */
	{0u, 2u, 3u, CLM_HE_RATE_FLAG_TXBF},	/* 2X3_TXBF */
	{0u, 3u, 3u, CLM_HE_RATE_FLAG_TXBF},	/* 3X3_TXBF */
	{0u, 1u, 4u, 0u},			/* 1X4 */
	{0u, 2u, 4u, 0u},			/* 2X4 */
	{0u, 3u, 4u, 0u},			/* 3X4 */
	{0u, 4u, 4u, 0u},			/* 4X4 */
	{0u, 1u, 4u, CLM_HE_RATE_FLAG_TXBF},	/* 1X4_TXBF */
	{0u, 2u, 4u, CLM_HE_RATE_FLAG_TXBF},	/* 2X4_TXBF */
	{0u, 3u, 4u, CLM_HE_RATE_FLAG_TXBF},	/* 3X4_TXBF */
	{0u, 4u, 4u, CLM_HE_RATE_FLAG_TXBF},	/* 4X4_TXBF */
};

#endif /* WL_RU_NUMRATES */

#if defined(WL_NUM_HE_RT) && !defined(WL_RU_NUMRATES)
/** Maps WL_HE_RT_... constants to their minimum bandwidths. UB/LUB mapped to
 * CLM_BW_NUM to designate that they are not allowed on subchannels
 */
static const uint8 min_ru_bw[] = {
	CLM_BW_20,		/* WL_HE_RT_SU */
	CLM_BW_20,		/* WL_HE_RT_RU26 */
	CLM_BW_20,		/* WL_HE_RT_RU52 */
	CLM_BW_20,		/* WL_HE_RT_RU106 */
	CLM_BW_NUM,		/* WL_HE_RT_UB */
	CLM_BW_NUM,		/* WL_HE_RT_LUB */
	CLM_BW_20,		/* WL_HE_RT_RU242 */
	CLM_BW_40,		/* WL_HE_RT_RU484 */
	CLM_BW_80,		/* WL_HE_RT_RU996 */
#ifdef CLM_RU996_2_SUPPORTED
	CLM_BW_160,		/* WL_HE_RT_RU996_2 */
#endif /* CLM_RU996_2_SUPPORTED */
#ifdef CLM_MRU_RATES_SUPPORTED
	CLM_BW_20,		/* WL_HE_RT_RU52_26 */
	CLM_BW_20,		/* WL_HE_RT_RU106_26 */
	CLM_BW_80,		/* WL_HE_RT_RU484_242 */
	CLM_BW_160,		/* WL_HE_RT_RU996_484 */
	CLM_BW_160,		/* WL_HE_RT_RU996_484_242 */
	CLM_BW_320,		/* WL_HE_RT_RU996_2_484 */
	CLM_BW_320,		/* WL_HE_RT_RU996_3 */
	CLM_BW_320,		/* WL_HE_RT_RU996_3_484 */
	CLM_BW_320,		/* WL_HE_RT_RU996_4 */
#endif /* CLM_MRU_RATES_SUPPORTED */
};
#endif /* WL_NUM_HE_RT */

#if defined(WL_NUM_HE_RT)
/** Maps shifted CLM_DATA_FLAG2_OUTER_BW_... constanst to bandwidths when
 * CLM_DATA_FLAG3_OUTER_BW_EXT flag not set
 */
static const uint8 outer_bw_to_bw[] = {0u, CLM_BW_40, CLM_BW_80, CLM_BW_160};
#ifdef CLM_320_MHZ_SUPPORTED
/** Maps shifted CLM_DATA_FLAG2_OUTER_BW_... constanst to bandwidths when
 * CLM_DATA_FLAG3_OUTER_BW_EXT flag set
 */
static const uint8 outer_bw_to_bw_ext[] = {0u, CLM_BW_320, 0u, 0u};
#endif /* CLM_320_MHZ_SUPPORTED */
#endif /* WL_NUM_HE_RT */

/** Maps shifted CLM_DATA_FLAG2_RATE_TYPE_... to RCT_... values when
 * CLM_DATA_FLAG3_RATE_TYPE_EXTENSIONflag not set
 */
static const uint8 rct_translation[] = {RCT_MAIN, RCT_EXT, RCT_HE, RCT_EXT4};
/** Maps shifted CLM_DATA_FLAG2_RATE_TYPE_... to RCT_... values when
 * CLM_DATA_FLAG3_RATE_TYPE_EXTENSIONflag set
 */
static const uint8 rct_translation_ext[] = {0u, 0u, RCT_EXT_RU, 0u};

/** Maps limit types to descriptors of paths from main channel to correspondent
 * subchannel. Each descriptor is a byte. High nibble contains number of steps
 * in path, bits in low nibble describe steps. 0 - select lower subchannel,
 * 1 - select upper subchannel. Least significant bit corresponds to last step
 */
static const uint8 subchan_paths[] = {
	0x00u, /* CHANNEL */
	0x10u, /* SUBCHAN_L */
	0x11u, /* SUBCHAN_U */
	0x20u, /* SUBCHAN_LL */
	0x21u, /* SUBCHAN_LU */
	0x22u, /* SUBCHAN_UL */
	0x23u, /* SUBCHAN_UU */
	0x30u, /* SUBCHAN_LLL */
	0x31u, /* SUBCHAN_LLU */
	0x32u, /* SUBCHAN_LUL */
	0x33u, /* SUBCHAN_LUU */
	0x34u, /* SUBCHAN_ULL */
	0x35u, /* SUBCHAN_ULU */
	0x36u, /* SUBCHAN_UUL */
	0x37u, /* SUBCHAN_UUU */
#ifdef CLM_320_MHZ_SUPPORTED
	0x40u, /* SUBCHAN_LLLL */
	0x41u, /* SUBCHAN_LLLU */
	0x42u, /* SUBCHAN_LLUL */
	0x43u, /* SUBCHAN_LLUU */
	0x44u, /* SUBCHAN_LULL */
	0x45u, /* SUBCHAN_LULU */
	0x46u, /* SUBCHAN_LUUL */
	0x47u, /* SUBCHAN_LUUU */
	0x48u, /* SUBCHAN_ULLL */
	0x49u, /* SUBCHAN_ULLU */
	0x4Au, /* SUBCHAN_ULUL */
	0x4Bu, /* SUBCHAN_ULUU */
	0x4Cu, /* SUBCHAN_UULL */
	0x4Du, /* SUBCHAN_UULU */
	0x4Eu, /* SUBCHAN_UUUL */
	0x4Fu, /* SUBCHAN_UUUU */
#endif /* CLM_320_MHZ_SUPPORTED */
};

/** Offsets of per-antenna power limits inside TX power record */
static const uint32 antenna_power_offsets[] = {
	CLM_LOC_DSC_POWER_IDX, CLM_LOC_DSC_POWER1_IDX, CLM_LOC_DSC_POWER2_IDX,
	CLM_LOC_DSC_POWER3_IDX
};

/** Locale type descriptors, ordered by locale type indices
 * (CLM_LOC_IDX_BASE_... constants)
 */
static const loc_type_dsc_t loc_type_dscs[] = {
	{CLM_BAND_2G, BH_BASE, OFFSETOF(clm_country_locales_t, locale_2G),
	OFFSETOF(clm_registry_t, locales[CLM_LOC_IDX_BASE_2G])},
	{CLM_BAND_5G, BH_BASE, OFFSETOF(clm_country_locales_t, locale_5G),
	OFFSETOF(clm_registry_t, locales[CLM_LOC_IDX_BASE_5G])},
	{CLM_BAND_2G, BH_HT, OFFSETOF(clm_country_locales_t, locale_2G_HT),
	OFFSETOF(clm_registry_t, locales[CLM_LOC_IDX_HT_2G])},
	{CLM_BAND_5G, BH_HT, OFFSETOF(clm_country_locales_t, locale_5G_HT),
	OFFSETOF(clm_registry_t, locales[CLM_LOC_IDX_HT_5G])},
#ifdef WL_BAND6G
	{CLM_BAND_6G, BH_BASE, OFFSETOF(clm_country_locales_t, locale_6G),
	OFFSETOF(clm_registry_t, locales_6g_base)},
	{CLM_BAND_6G, BH_HT, OFFSETOF(clm_country_locales_t, locale_6G_HT),
	OFFSETOF(clm_registry_t, locales_6g_ht)},
#endif /* WL_BAND6G */
};

/** Composes locale type indices (CLM_LOC_IDX_BASE_... constants) out of band
 * and flavor
 */
static const uint32 compose_loc_type[CLM_BAND_NUM][BH_NUM] = {
	{CLM_LOC_IDX_BASE_2G, CLM_LOC_IDX_HT_2G},
	{CLM_LOC_IDX_BASE_5G, CLM_LOC_IDX_HT_5G},
#ifdef WL_BAND6G
	{CLM_LOC_IDX_BASE_6G, CLM_LOC_IDX_HT_6G},
#endif /* WL_BAND6G */
};

/** Flag translation for base locale flags (FTS_BASE) */
static const flag_translation_t base_flag_translations[] = {
	{CLM_DATA_FLAG_FILTWAR1, CLM_FLAG_FILTWAR1},
	{CLM_DATA_FLAG_PSD_LIMITS, CLM_FLAG_PSD}
};

/** Flag translations for first country (region) flag byte (FTS_COUNTRY_FLAG_1) */
static const flag_translation_t country_flag_1_translations[] = {
	{CLM_DATA_FLAG_REG_TXBF, CLM_FLAG_TXBF},
	{CLM_DATA_FLAG_REG_EDCRS_EU, CLM_FLAG_EDCRS_EU},
	{CLM_DATA_FLAG_REG_RED_EU, CLM_FLAG_RED_EU}
};

/** Flag translations for second country (region) flag byte (FTS_COUNTRY_FLAG_2) */
static const flag_translation_t country_flag_2_translations [] = {
	{CLM_DATA_FLAG_2_REG_LO_GAIN_NBCAL, CLM_FLAG_LO_GAIN_NBCAL},
	{CLM_DATA_FLAG_2_REG_CHSPRWAR2, CLM_FLAG_CHSPRWAR2},
	{CLM_DATA_FLAG_2_REG_DSA, CLM_FLAG_DSA}
};

/** Flag translation sets. Must follow the order, specified in
 * flag_translation_set_id_t enum
 */
static const flag_translation_set_t flag_translation_sets [] = {
	/* FTS_BASE */
	{base_flag_translations, ARRAYSIZE(base_flag_translations)},
	/* FTS_COUNTRY_FLAG_1 */
	{country_flag_1_translations, ARRAYSIZE(country_flag_1_translations)},
	/* FTS_COUNTRY_FLAG_2 */
	{country_flag_2_translations, ARRAYSIZE(country_flag_2_translations)}
};

/** Translation of device category codes from API to BLOB (indices of
 * subchannel rule sets). Happened to be identical - but just in case
 */
static const uint32 dev_cat_translate[] = {
	CLM_DATA_DEVICE_CATEGORY_LP, CLM_DATA_DEVICE_CATEGORY_VLP, CLM_DATA_DEVICE_CATEGORY_SP
};

#ifdef CLM_MRU_RATES_SUPPORTED
/** MRU rate derivations for 20MHz channels */
const ru_derivation_t ru_derivations_20mhz[] = {
	{WL_RU_RATE_1X1_242SS1, 0},
	{WL_RU_RATE_1X1_52_26SS1, 20},		/* ceil(4*10*(log10(242)-log10(52+26))) */
	{WL_RU_RATE_1X1_106_26SS1, 11}		/* ceil(4*10*(log10(242)-log10(106+26))) */
};

/** MRU rate derivations for 40MHz channels */
const ru_derivation_t ru_derivations_40mhz[] = {{WL_RU_RATE_1X1_484SS1, 0}};

/** MRU rate derivations for 80MHz channels */
const ru_derivation_t ru_derivations_80mhz[] = {
	{WL_RU_RATE_1X1_996SS1, 0},
	{WL_RU_RATE_1X1_484_242SS1, 6}		/* ceil(4*10*(log10(996)-log10(484+242))) */
};

/** MRU rate derivations for 160MHz channels */
const ru_derivation_t ru_derivations_160mhz[] = {
	{WL_RU_RATE_1X1_996_2SS1, 0},
	{WL_RU_RATE_1X1_996_484SS1, 6},		/* ceil(4*10*(log10(996*2)-log10(996+484))) */
	{WL_RU_RATE_1X1_996_484_242SS1, 3}	/* ceil(4*10*(log10(996*2)-log10(996+484+242))) */
};

/** MRU rate derivations for 320MHz channels */
const ru_derivation_t ru_derivations_320mhz[] = {
	{WL_RU_RATE_1X1_996_4SS1, 0},
	{WL_RU_RATE_1X1_996_2_484SS1, 9},	/* ceil(4*10*(log10(996*4)-log10(996*2+484))) */
	{WL_RU_RATE_1X1_996_3SS1, 5},		/* ceil(4*10*(log10(996*4)-log10(996*3))) */
	{WL_RU_RATE_1X1_996_3_484SS1, 3}	/* ceil(4*10*(log10(996*4)-log10(996*3+484))) */
};
#endif /* CLM_MRU_RATES_SUPPORTED */

/** Traits of various bandwidths */
#ifdef WL_NUM_HE_RT
	#define RU_RATE_TYPE_FIELD(ru_rate_type) ru_rate_type,
#else
	#define RU_RATE_TYPE_FIELD(ru_rate_type)
#endif /* WL_NUM_HE_RT */
#ifdef CLM_MRU_RATES_SUPPORTED
	#define MRU_FIELD(mru_field) /* codestyle */, mru_field
#else
	#define MRU_FIELD(mru_field)
#endif /* CLM_MRU_RATES_SUPPORTED */
#ifdef WL_RU_NUMRATES
	#define CHANNEL_RU_RATE_FIELD(channel_ru_rate) channel_ru_rate,
#else
	#define CHANNEL_RU_RATE_FIELD(channel_ru_rate)
#endif /* else WL_RU_NUMRATES */
#ifdef WL11BE
	#define BE_FIELD(be_field) /* codestyle */, be_field
#else /* WL11BE */
	#define BE_FIELD(be_field)
#endif /* else WL11BE */
#ifdef CLM_MRU_RATES_SUPPORTED
	#define MRU_MASK(rt) | (1 << rt)
#else /* CLM_MRU_RATES_SUPPORTED */
	#define MRU_MASK(rt)
#endif /* else CLM_MRU_RATES_SUPPORTED */
static const bandwidth_traits_t bandwidths_traits[] = {
	/* 20MHz */
	{TRUE, CLM_DATA_FLAG_SC_RULE_BW_20, 2, RU_RATE_TYPE_FIELD(WL_HE_RT_RU242)
	CHANNEL_RU_RATE_FIELD(WL_RU_RATE_1X1_242SS1) CLM_BW_NUM, CLM_BW_20,
	~0u
	MRU_FIELD(ru_derivations_20mhz) MRU_FIELD(ARRAYSIZE(ru_derivations_20mhz))
	BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
	/* 40MHz */
	{TRUE, CLM_DATA_FLAG_SC_RULE_BW_40, 4, RU_RATE_TYPE_FIELD(WL_HE_RT_RU484)
	CHANNEL_RU_RATE_FIELD(WL_RU_RATE_1X1_484SS1) CLM_BW_20, CLM_BW_40,
	CLM_DATA_SUB_CHAN_MAX_40
	MRU_FIELD(ru_derivations_40mhz) MRU_FIELD(ARRAYSIZE(ru_derivations_40mhz))
	BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
	/* 80MHz */
	{TRUE, CLM_DATA_FLAG_SC_RULE_BW_80, 8, RU_RATE_TYPE_FIELD(WL_HE_RT_RU996)
	CHANNEL_RU_RATE_FIELD(WL_RU_RATE_1X1_996SS1) CLM_BW_40, CLM_BW_80,
	CLM_DATA_SUB_CHAN_MAX_80
	MRU_FIELD(ru_derivations_80mhz) MRU_FIELD(ARRAYSIZE(ru_derivations_80mhz))
	BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
#ifdef CLM_RU996_2_SUPPORTED
	/* 160MHz */
	{TRUE, CLM_DATA_FLAG_SC_RULE_BW_160, 16, RU_RATE_TYPE_FIELD(WL_HE_RT_RU996_2)
	CHANNEL_RU_RATE_FIELD(WL_RU_RATE_1X1_996_2SS1) CLM_BW_80, CLM_BW_160,
	CLM_DATA_SUB_CHAN_MAX_160
	MRU_FIELD(ru_derivations_160mhz) MRU_FIELD(ARRAYSIZE(ru_derivations_160mhz))
	BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
#else
	/* 160MHz */
	{TRUE, CLM_DATA_FLAG_SC_RULE_BW_160, 16, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_80, CLM_BW_160,
	CLM_DATA_SUB_CHAN_MAX_160
	MRU_FIELD(NULL) MRU_FIELD(0) BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
#endif /* else CLM_RU996_2_SUPPORTED */
	/* 80+80MHz */
	{TRUE, CLM_DATA_FLAG_SC_RULE_BW_80, 8, RU_RATE_TYPE_FIELD(WL_HE_RT_RU996)
	CHANNEL_RU_RATE_FIELD(WL_RU_RATE_1X1_996SS1) CLM_BW_40, CLM_BW_80,
	CLM_DATA_SUB_CHAN_MAX_80
	MRU_FIELD(ru_derivations_80mhz) MRU_FIELD(ARRAYSIZE(ru_derivations_80mhz))
	BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
#ifdef WL11BE
	/* 240MHz */
	{TRUE, 0, 0, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_NUM, CLM_BW_NUM,
	~0u
	MRU_FIELD(NULL) MRU_FIELD(0), CLM_BW_320, 5,
	(1u << WL_HE_RT_RU26) | (1u << WL_HE_RT_RU52) | (1u << WL_HE_RT_RU106) |
	(1u << WL_HE_RT_UB) | (1u << WL_HE_RT_LUB) | (1u << WL_HE_RT_RU242) |
	(1u << WL_HE_RT_RU484) | (1u << WL_HE_RT_RU996)
#ifdef CLM_RU996_2_SUPPORTED
	| (1u << WL_HE_RT_RU996_2)
#endif /* CLM_RU996_2_SUPPORTED */
	MRU_MASK(WL_HE_RT_RU52_26) MRU_MASK(WL_HE_RT_RU106_26) MRU_MASK(WL_HE_RT_RU484_242)
	MRU_MASK(WL_HE_RT_RU996_484) MRU_MASK(WL_HE_RT_RU996_484_242)
	MRU_MASK(WL_HE_RT_RU996_2_484) MRU_MASK(WL_HE_RT_RU996_3)},
#if defined(CLM_MRU_RATES_SUPPORTED)
	/* 320MHz */
	{TRUE, CLM_DATA_FLAG_SC_RULE_BW_320, 32, WL_HE_RT_RU996_4,
	WL_RU_RATE_1X1_996_4SS1, CLM_BW_160, CLM_BW_320,
	CLM_DATA_SUB_CHAN_MAX_320,
	ru_derivations_320mhz, ARRAYSIZE(ru_derivations_320mhz)
	BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
#else /* CLM_MRU_RATES_SUPPORTED */
	/* 320MHz */
	{TRUE, CLM_DATA_FLAG_SC_RULE_BW_320, 32, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_160, CLM_BW_320,
	CLM_DATA_SUB_CHAN_MAX_320 BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
#endif /* else CLM_MRU_RATES_SUPPORTED */
	/* 160+160MHz */
	{FALSE, 0, 0, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_NUM, CLM_BW_NUM,
	~0u
	MRU_FIELD(NULL) MRU_FIELD(0) BE_FIELD(CLM_BW_NUM) BE_FIELD(0) BE_FIELD(0)},
	/* 60MHz */
	{TRUE, 0, 0, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_NUM, CLM_BW_NUM,
	~0u
	MRU_FIELD(NULL) MRU_FIELD(0), CLM_BW_80, 5,
	(1u << WL_HE_RT_RU26) | (1u << WL_HE_RT_RU52) | (1u << WL_HE_RT_RU106) |
	(1u << WL_HE_RT_UB) | (1u << WL_HE_RT_LUB) | (1u << WL_HE_RT_RU242) |
	(1u << WL_HE_RT_RU484)
	MRU_MASK(WL_HE_RT_RU52_26) MRU_MASK(WL_HE_RT_RU106_26) MRU_MASK(WL_HE_RT_RU484_242)},
	/* 120MHz */
	{TRUE, 0, 0, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_NUM, CLM_BW_NUM,
	~0u
	MRU_FIELD(NULL) MRU_FIELD(0), CLM_BW_160, 5,
	(1u << WL_HE_RT_RU26) | (1u << WL_HE_RT_RU52) | (1u << WL_HE_RT_RU106) |
	(1u << WL_HE_RT_UB) | (1u << WL_HE_RT_LUB) | (1u << WL_HE_RT_RU242) |
	(1u << WL_HE_RT_RU484) | (1u << WL_HE_RT_RU996)
	MRU_MASK(WL_HE_RT_RU52_26) MRU_MASK(WL_HE_RT_RU106_26) MRU_MASK(WL_HE_RT_RU484_242)
	MRU_MASK(WL_HE_RT_RU996_484)},
	/* 140MHz */
	{TRUE, 0, 0, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_NUM, CLM_BW_NUM,
	~0u
	MRU_FIELD(NULL) MRU_FIELD(0), CLM_BW_160, 3,
	(1u << WL_HE_RT_RU26) | (1u << WL_HE_RT_RU52) | (1u << WL_HE_RT_RU106) |
	(1u << WL_HE_RT_UB) | (1u << WL_HE_RT_LUB) | (1u << WL_HE_RT_RU242) |
	(1u << WL_HE_RT_RU484) | (1u << WL_HE_RT_RU996)
	MRU_MASK(WL_HE_RT_RU52_26) MRU_MASK(WL_HE_RT_RU106_26) MRU_MASK(WL_HE_RT_RU484_242)
	MRU_MASK(WL_HE_RT_RU996_484) MRU_MASK(WL_HE_RT_RU996_484_242)},
	/* 200MHz */
	{TRUE, 0, 0, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_NUM, CLM_BW_NUM,
	~0u
	MRU_FIELD(NULL) MRU_FIELD(0), CLM_BW_320, 9,
	(1u << WL_HE_RT_RU26) | (1u << WL_HE_RT_RU52) | (1u << WL_HE_RT_RU106) |
	(1u << WL_HE_RT_UB) | (1u << WL_HE_RT_LUB) | (1u << WL_HE_RT_RU242) |
	(1u << WL_HE_RT_RU484) | (1u << WL_HE_RT_RU996)
#ifdef CLM_RU996_2_SUPPORTED
	| (1u << WL_HE_RT_RU996_2)
#endif /* CLM_RU996_2_SUPPORTED */
	MRU_MASK(WL_HE_RT_RU52_26) MRU_MASK(WL_HE_RT_RU106_26) MRU_MASK(WL_HE_RT_RU484_242)
	MRU_MASK(WL_HE_RT_RU996_484) MRU_MASK(WL_HE_RT_RU996_484_242)
	MRU_MASK(WL_HE_RT_RU996_2_484)},
	/* 280MHz */
	{TRUE, 0, 0, RU_RATE_TYPE_FIELD(WL_NUM_HE_RT)
	CHANNEL_RU_RATE_FIELD(WL_RU_NUMRATES) CLM_BW_NUM, CLM_BW_NUM,
	~0u
	MRU_FIELD(NULL) MRU_FIELD(0), CLM_BW_320, 3,
	(1u << WL_HE_RT_RU26) | (1u << WL_HE_RT_RU52) | (1u << WL_HE_RT_RU106) |
	(1u << WL_HE_RT_UB) | (1u << WL_HE_RT_LUB) | (1u << WL_HE_RT_RU242) |
	(1u << WL_HE_RT_RU484) | (1u << WL_HE_RT_RU996)
#ifdef CLM_RU996_2_SUPPORTED
	| (1u << WL_HE_RT_RU996_2)
#endif /* CLM_RU996_2_SUPPORTED */
	MRU_MASK(WL_HE_RT_RU52_26) MRU_MASK(WL_HE_RT_RU106_26) MRU_MASK(WL_HE_RT_RU484_242)
	MRU_MASK(WL_HE_RT_RU996_484) MRU_MASK(WL_HE_RT_RU996_484_242)
	MRU_MASK(WL_HE_RT_RU996_2_484) MRU_MASK(WL_HE_RT_RU996_3)
	MRU_MASK(WL_HE_RT_RU996_3_484)},
#endif /* WL11BE */
};
#undef RU_RATE_TYPE_FIELD

/** 1-based indices in [BB40_NUM] array for valid band/bandwidth 40+MHz pairs
 * (0 for invalid)
 */
static const uint32 bb40_indices[CLM_BAND_NUM][CLM_BW_NUM] = {
	{0, 1},
	{0, 2, 3, 4, 5},
#ifdef WL_BAND6G
#ifdef CLM_320_MHZ_SUPPORTED
	{0, 6, 7, 8, 0, 0, 9},
#else /* CLM_320_MHZ_SUPPORTED */
	{0, 6, 7, 8},
#endif /* else CLM_320_MHZ_SUPPORTED */
#endif /* WL_BAND6G */
};

#ifdef WL_BAND6G
/** Translates C2C part of base locale flags to clm_c2c_t */
static const uint8 c2c_translation[] = {CLM_C2C_NONE, CLM_C2C_EU, CLM_C2C_US};
#endif /* WL_BAND6G */

/****************************
* MODULE INTERNAL FUNCTIONS *
*****************************
*/

/** Removes bits set in mask from value, shifting right
 * \param[in] value Value to remove bits from
 * \param[in] mask Mask with bits to remove
 * Returns value with bits in mask removed
 */
static uint32
remove_extra_bits(uint32 value, uint32 mask)
{
	while (mask) {
		/* m has mask's lowest set bit and all lower bits set */
		uint32 m = mask ^ (mask - 1u);
		/* Clearing mask's lowest 1 bit */
		mask &= ~m;
		/* m has bits to left of former mask's lowest set bit set */
		m >>= 1u;
		/* Removing mask's (former) lowest set bit from value */
		value = (value & m) | ((value >> 1u) & ~m);
		/* Removing mask's (former) lowest set bit from mask */
		mask >>= 1u;
	}
	return value;
}

/** Returns value of field with given name in given (main of incremental) CLM
 * data set. Interface to get_data that converts field name to offset of field
 * inside CLM data registry
 * \param[in] ds_id CLM data set identifier
 * \param[in] field Name of field in struct clm_registry
 * \return Field value as (const void *) pointer. NULL if given data source was
 * not set
 */
#define GET_DATA(ds_id, field) get_data(ds_id, OFFSETOF(clm_registry_t, field))

/* Accessor function to avoid data_sources structure from getting into ROM.
 * Don't have this function in ROM.
 */
static data_dsc_t *
BCMRAMFN(get_data_sources)(uint32 ds_idx)
{
	return &data_sources[ds_idx];
}

/* Returns type of given rate
 * \param[in] rate_idx Value from clm_rates_t
 * \returns RT_... value
 */
static clm_rate_type_t
BCMRAMFN(get_rate_type)(uint32 rate_idx)
{
	return (clm_rate_type_t)
#ifdef CLM_EHT_RATES_SUPPORTED
		/* Two bits per value */
		((rate_type[rate_idx >> 1u] >> (((rate_idx) & 1u) << 2u)) & 0xFu);
#else
		/* Four bits per value */
		((rate_type[rate_idx >> 2u] >> (((rate_idx) & 3u) << 1u)) & 3u);
#endif /* CLM_EHT_RATES_SUPPORTED */
}

/** Sets rate type
 * \param[in] rate_idx Value from clm_rates_t
 * \param[in] rt RT_... value
 */
static void
BCMRAMFN(set_rate_type)(uint32 rate_idx, clm_rate_type_t rt)
{
#ifdef CLM_EHT_RATES_SUPPORTED
	/* Two bits per value */
	rate_type[rate_idx >> 1u] &= ~(0xFu << (((rate_idx) & 1u) << 2u));
	rate_type[rate_idx >> 1u] |= rt << (((rate_idx) & 1u) << 2u);
#else
	/* Four bits per value */
	rate_type[rate_idx >> 2u] &= ~(3u << (((rate_idx) & 3u) << 1u));
	rate_type[rate_idx >> 2u] |= rt << (((rate_idx) & 3u) << 1u);
#endif /* CLM_EHT_RATES_SUPPORTED */
}

/** Returns traits of given bandwidth */
static const bandwidth_traits_t *
BCMRAMFN(get_bandwidth_traits)(clm_bandwidth_t bw)
{
	return bandwidths_traits + bw;
}

/** True if given bandwidth supported */
static bool
BCMRAMFN(is_valid_bandwidth)(clm_bandwidth_t bw)
{
	return (((uint32)bw < (uint32)CLM_BW_NUM)) && bandwidths_traits[bw].supported;
}

/** Return TX record header's bandwidth
 * \param[in] flags - First flag byte of TX record header
 * \returns Bandwidth encoded in flags (in CLM_DATA_FLAG_WIDTH_... values),
 * CLM_BW_NUM if bandwidth is unsupported (e.g. BLOB generated for some
 * bandwidths, that firmware does not support because some defines not defined)
 */
static clm_bandwidth_t
BCMRAMFN(get_tx_rec_bw)(uint8 flags)
{
	return (clm_bandwidth_t)bw_tx_to_clm[flags & CLM_DATA_FLAG_WIDTH_MASK];
}

#ifdef WL_NUM_HE_RT
/** Return TX record header's outer bandwidth
 * \param[in] flags2 Second flag byte of TX record header
 * \param[in] flags3 Third flag byte of TX record header
 * \param[in] value_for_same Value t return for CLM_DATA_FLAG2_OUTER_BW_SAME
 * \returns Bandwidth encoded in flags (in CLM_DATA_FLAG2_OUTER_BW_... values)
 */
static clm_bandwidth_t
BCMRAMFN(get_outer_bw)(uint8 flags2, uint8 flags3, clm_bandwidth_t value_for_same)
{
	(void)flags3;
	(void)flags2;
	flags2 &= CLM_DATA_FLAG2_OUTER_BW_MASK;
	return flags2
		? (clm_bandwidth_t)
#if defined(CLM_320_MHZ_SUPPORTED)
			((flags3 & CLM_DATA_FLAG3_OUTER_BW_EXT) ?
			outer_bw_to_bw_ext : outer_bw_to_bw)
#else /* defined(CLM_320_MHZ_SUPPORTED) */
			outer_bw_to_bw
#endif /* else defined(CLM_320_MHZ_SUPPORTED) */
			[flags2 >> CLM_DATA_FLAG2_OUTER_BW_SHIFT]
		: value_for_same;
}
#endif /* WL_NUM_HE_RT */

/** Return TX record header's rate code type
 * \param[in] flags2 Second flag byte of TX record header
 * \param[in] flags3 Third flag byte of TX record header
 * \returns Rate code type encoded in flags
 */
static clm_rate_code_type_t
BCMRAMFN(get_rate_code_type)(uint8 flags2, uint8 flags3)
{
	return (clm_rate_code_type_t)
		((flags3 & CLM_DATA_FLAG3_RATE_TYPE_EXTENSION)
			? rct_translation_ext : rct_translation)
		[(flags2 & CLM_DATA_FLAG2_RATE_TYPE_MASK) >> CLM_DATA_FLAG2_RATE_TYPE_SHIFT];
}

#if defined(WL_RU_NUMRATES)
/** Returns HE0 rate of given TX mode
 * \param[in] ru_mode_idx RU rate TX mode index (index in he_rate_descriptors)
 * \returns HE0 rate (one of WL_RATE_...HE0SS... constants)
 */
static clm_rates_t
BCMRAMFN(get_su_base_rate)(uint32 ru_mode_idx)
{
	return su_base_rates[ru_mode_idx];
}

/** Returns RU TX mode descriptor
 * \param[in] ru_mode_idx RU rate TX mode index (index in he_rate_descriptors)
 * \returns Mode descriptor
 */
static const clm_he_rate_dsc_t *
BCMRAMFN(get_tx_mode_info)(uint32 ru_mode_idx)
{
	return he_rate_descriptors + ru_mode_idx;
}

/** Returns TX mode index for given HE0 rate
 * \param[in] he0_rate HE0 rate index from clm_rates_t
 * \return TX mode of this rate (its index in su_base_rates)
 */
static uint32
BCMRAMFN(get_he_tx_mode)(uint32 he0_rate)
{
	return he_tx_mode_hash[(he0_rate >> SU_TX_MODE_HASH_SHIFT) & SU_TX_MODE_HASH_MASK];
}

/** Sets TX mode for given HE0 rate in he_tx_mode_hash
 * \param[in] he0_rate HE0 rate index from clm_rates_t
 * \param[in] he0_rate HE0 rate index from clm_rates_t
 * \param[in] mode Mode (index of this rate in su_base_rates)
 */
static void
BCMRAMFN(set_he_tx_mode)(uint32 he0_rate, uint32 mode)
{
	he_tx_mode_hash[(he0_rate >> SU_TX_MODE_HASH_SHIFT) & SU_TX_MODE_HASH_MASK] = (uint8)mode;
}

/** Returns minimum bandwidth for given RU rate, BW_NUM for non-RU OFDMA rates */
static clm_bandwidth_t
BCMRAMFN(get_ru_rate_min_bw)(uint32 ru_rate)
{
	return (clm_bandwidth_t)
		((ru_rate_min_bw[ru_rate >> 1u] >> (((ru_rate) & 1u) << 2u)) & 0xFu);
}

/** Sets minimum bandwidth for given RU rate
 * \param[in] ru_rate RU rate index (from clm_ru_rates_t)
 * \param[in] min_bw Minimum bandwidth for UL, CLM_BW_NUM for DL
 */
static void
BCMRAMFN(set_ru_rate_min_bw)(uint32 ru_rate, clm_bandwidth_t min_bw)
{
	ru_rate_min_bw[ru_rate >> 1u] &= (uint8)(~(0xFu << (((ru_rate) & 1u) << 2u)));
	ru_rate_min_bw[ru_rate >> 1u] |= (uint8)(min_bw << (((ru_rate) & 1u) << 2u));
}
#elif defined(WL_NUM_HE_RT)
/** Minimum bandwidth for given RU rate type
 * \param[in] ru_rate_type One of WL_HE_RT_... constants
 * \returns Minimum bandwidth that can hold given RU type
 */
static clm_bandwidth_t
BCMRAMFN(get_min_ru_bw)(uint32 ru_rate_type)
{
	return (clm_bandwidth_t)min_ru_bw[ru_rate_type];
}
#endif /* elif defined(WL_NUM_HE_RT) */

/** Return information about path to subchannel for given limit type (subchannel
 * identifier)
 * \param[in] limits_type Subchannel identifier
 * \param[out] path Bit sequence - 0 to low side, 1 - to high side. Least
 *	significant bit designates the last turn
 * \param[out] high_bw_mask bitmask that corresponds to highest bit in path
 *	(first turn)
 */
static void
BCMRAMFN(get_subchan_path)(clm_limits_type_t limits_type, uint8 *path, uint8 *high_bw_mask)
{
	uint8 p = subchan_paths[limits_type];
	*path = p & ~SUB_CHAN_PATH_COUNT_MASK;
	*high_bw_mask =
		(uint8)(1u << ((p & SUB_CHAN_PATH_COUNT_MASK) >> SUB_CHAN_PATH_COUNT_OFFSET));
}

/** For given channel and limit type returns bandwidth that coresponds to this
 * limit type
 * \param[in] limits_type Limit type
 * \param[in] bw Channel bandwidth
 * \return Subchannel bandwidth. E.g. for limit type L and bandwidth 40MHz
 *	returns 20MHz. Returns CLM_BW_NUM if given combination is invalid (e.g.
 *	L and 20MHz)
 */
static clm_bandwidth_t
BCMRAMFN(get_subchan_bw)(clm_limits_type_t limits_type, clm_bandwidth_t bw)
{
	uint32 count = (subchan_paths[limits_type] & SUB_CHAN_PATH_COUNT_MASK) >>
		SUB_CHAN_PATH_COUNT_OFFSET;
	bw = bandwidths_traits[bw].active;
	while ((count--) && (bw != CLM_BW_NUM)) {
		bw = bandwidths_traits[bw].half_bw;
	}
	return bw;
}

/** Returns index in [BB40_NUM]-array for given band and bandwidth
 * (~0u for invalid combinations)
 */
static uint32
BCMRAMFN(get_bb40_idx)(clm_band_t band, clm_bandwidth_t bw)
{
	uint32 ret = bb40_indices[band][bw];
	return ret ? (ret - 1u) : ~0u;
}

/** Returns value of field with given offset in given (main or incremental) CLM
 * data set
 * \param[in] ds_idx CLM data set identifier
 * \param[in] field_offset Offset of field in struct clm_registry
 * \return Field value as (const void *) pointer. NULL if given data source was
 * not set
 */
static const void *
get_data(uint32 ds_idx, uint32 field_offset)
{
	data_dsc_t *ds = get_data_sources(ds_idx);
	const uintptr paddr = (uintptr)ds->data;
	const uint8 **pp = (const uint8 **)(paddr + field_offset);

	return (ds->data && *pp) ? (*pp + ds->relocation) : NULL;
}

/** Converts given pointer value, fetched from some (possibly relocated) CLM
 * data structure to its 'true' value
 * Note that GET_DATA() return values are already converted to 'true' values
 * \param[in] ds_idx Identifier of CLM data set that contained given pointer
 * \param[in] ptr Pointer to convert
 * \return 'True' (unrelocated) pointer value
 */
static const void *
relocate_ptr(uint32 ds_idx, const void *ptr)
{
	return ptr ? ((const uint8 *)ptr + get_data_sources(ds_idx)->relocation) : NULL;
}

/** Returns address of data item, stored in one of ..._set_t structures
 * \param[in] ds_idx Identifier of CLM data set
 * \param[in] field_offset Offset of address of set field in clm_registry_t
 * \param[in] num_offset Offset of 'num' field in ..._set_t structure
 * \param[in] set_offset Offset of 'set' field in ..._set_t structure
 * \param[in] rec_len Length of record referenced by 'set' field
 * \param[in] idx Index of requested field
 * \param[out] num Optional output parameter - value of 'num' field of ..._set
    structure. -1 if structure not found
 * \return Address of idx's field of vector, referenced by set field or NULL
 */
static const void *
get_item(uint32 ds_idx, uint32 field_offset, uint32 num_offset, uint32 set_offset,
	uint32 rec_len, uint32 idx, int32 *num)
{
	const void *data = get_data(ds_idx, field_offset);
	if (data) {
		uint32 n = *(const int32 *)((const uint8 *)data + num_offset);
		if (num) {
			*num = (int32)n;
		}
		return (idx < n)
				? ((const uint8 *)relocate_ptr(ds_idx,
				*(const void * const *)((const uint8 *)data + set_offset)) +
				idx * rec_len)
				: NULL;
	}
	if (num) {
		*num = -1;
	}
	return NULL;
}

/** Removes syntax sugar around get_item() call
 * \param[in] ds_id Identifier of CLM data set
 * \param[in] registry_field name of field in clm_registry_t
 * \param[in] set_struct ..._set_t structure
 * \param[in] record_len Length of record, referenced by 'set' field
 * \param[in] idx Index of record
 * \param[out] num Optional output parameter - value of 'num' field of -1
 * \return Address of record or 0
 */
#define GET_ITEM(ds_id, registry_field, set_struct, record_len, idx, num_value) \
	get_item(ds_id, OFFSETOF(clm_registry_t, registry_field), \
	OFFSETOF(set_struct, num), OFFSETOF(set_struct, set), record_len, idx, num_value)

/** Retrieves CLM data set identifier and index, contained in given iterator
 * Applicable to previously 'packed' iterator. Not applicable to 'just
 * initialized' iterators
 * \param[in] iter Iterator to unpack
 * \param[out] ds_id CLM data set identifier retrieved from iterator
 * \param[out] idx Index retrieved from iterator
 */
static void
iter_unpack(int32 iter, data_source_id_t *ds_id, uint32 *idx)
{
	--iter;
	if (ds_id) {
		*ds_id = ((iter & ITER_DS_MASK) == ITER_DS_MAIN) ? DS_MAIN : DS_INC;
	}
	if (idx) {
		*idx = (uint32)(iter & ITER_IDX_MASK);
	}
}

/** Creates (packs) iterator out of CLM data set identifier and index
 * \param[out] iter Resulted iterator
 * \param[in] ds_id CLM data set identifier to put to iterator
 * \param[in] idx Index to put to iterator
 */
static void
iter_pack(int32 *iter, data_source_id_t ds_id, uint32 idx)
{
	if (iter) {
		*iter = (((ds_id == DS_MAIN) ? ITER_DS_MAIN : ITER_DS_INC) |
			(int32)(idx & ITER_IDX_MASK)) + 1;
	}
}

/** Traversal of byte string sequence
 * \param[in] byte_string_seq Byte string sequence to traverse
 * \param[in] idx Index of string to find
 * \param[in] sequence_index NULL or sequence index, containing offsets of strings
 * \return Address of idx's string in a sequence
 */
static const uint8 *
get_byte_string(const uint8 *byte_string_seq, uint32 idx, const uint16 *sequence_index)
{
	if (sequence_index) {
		return byte_string_seq + sequence_index[idx];
	}
	while (idx--) {
		byte_string_seq += *byte_string_seq + 1u;
	}
	return byte_string_seq;
}

/** Skips sequence of new-style (bandwidth-aware) regulatory power records
 * \param[in] reg_data_ptr Points to beginning of regulatory power data
 * \return Address past end of regulatory power data
 */
static const uint8 *
skip_multibandwidth_regulatory_powers(const uint8 *reg_data_ptr)
{
	uint8 flags;
	do {
		flags = *reg_data_ptr++;
		if (flags & CLM_DATA_FLAG_FLAG2) {
			uint8 flags2 = *reg_data_ptr++;
			if (flags2 & CLM_DATA_FLAG2_FLAG3) {
				++reg_data_ptr;
			}
		}
		reg_data_ptr += 1u + CLM_LOC_DSC_REG_REC_LEN * (uint32)(*reg_data_ptr);
	} while (flags & CLM_DATA_FLAG_MORE);
	return reg_data_ptr;
}

/** Skips base locale header
 * \param[in] loc_def base locale definition
 * \param[in] ds Descriptor of data set locale belongs to
 * \param[out] reg_limits Optional output parameter - locale channel regulatory limits
 * \param[out] psd_limits Optional output parameter - locale PSD regulatory limits
 * \return Address past base locale header
 */
static const uint8 *
skip_base_header(const uint8 *loc_def, data_dsc_t *ds, const uint8 **reg_limits,
	const uint8 **psd_limits)
{
	uint8 base_flags = loc_def[CLM_LOC_DSC_FLAGS_IDX];
	loc_def += CLM_LOC_DSC_BASE_HDR_LEN;
	if (reg_limits) {
		*reg_limits = loc_def;
	}
	if (ds->registry_flags2 & CLM_REGISTRY_FLAG2_DEVICE_CATEGORIES) {
		loc_def = skip_multibandwidth_regulatory_powers(loc_def);
	} else {
		loc_def += 1u + CLM_LOC_DSC_REG_REC_LEN * (uint32)(*loc_def);
	}
	if (reg_limits && ((int32)(loc_def - *reg_limits) < (int32)(CLM_LOC_DSC_REG_REC_LEN + 1))) {
		*reg_limits = NULL;
	}
	if (base_flags & CLM_DATA_FLAG_PSD_LIMITS) {
		if (psd_limits) {
			*psd_limits = loc_def;
		}
		loc_def = skip_multibandwidth_regulatory_powers(loc_def);
	}
	return loc_def;
}

/** Looks up byte string that contains locale definition, precomputes
 * locale-related data
 * \param[in] locales Region locales
 * \param[in] loc_type Type of locale to retrieve (CLM_LOC_IDX_...)
 * \param[out] loc_data Locale-related data. If locale not found all fields are
 * zeroed
 * \return TRUE in case of success, FALSE if region locale definitions
 * structure contents is invalid
 */
static bool
get_loc_def(const clm_country_locales_t *locales, uint32 loc_type,
	locale_data_t *loc_data)
{
	data_source_id_t ds_id;
	uint32 bw_idx;
	const loc_type_dsc_t *ltd;
	clm_band_t band;
	const uint8 *loc_def = NULL;
	data_dsc_t *ds;

	bzero(loc_data, sizeof(*loc_data));
	if ((uint32)loc_type >= ARRAYSIZE(loc_type_dscs)) {
		return FALSE;
	}
	ltd = &loc_type_dscs[loc_type];
	band = ltd->band;
	loc_def = *(const uint8 * const *)((const uint8 *)locales + ltd->def_field_offset);
	if (!loc_def) {
		return TRUE;
	}
	ds_id = (locales->main_loc_data_bitmask & (1u << loc_type)) ? DS_MAIN : DS_INC;
	loc_data->ds = ds = get_data_sources(ds_id);
	if (ltd->flavor == BH_BASE) {
		loc_data->base_hdr = loc_def;
		loc_def = skip_base_header(loc_def, ds, &loc_data->reg_limits,
			&loc_data->psd_limits);
	}
	loc_data->tx_limits = loc_def;
	loc_data->chan_ranges_bw = ds->chan_ranges_band_bw[band];
	loc_data->rate_sets_bw = ds->rate_sets_bw[band];
	loc_data->rate_sets_indices_bw = ds->rate_sets_indices_bw[band];
	loc_data->ext_rate_sets_bw = ds->ext_rate_sets_bw[band];
	loc_data->ext_rate_sets_indices_bw = ds->ext_rate_sets_indices_bw[band];
	loc_data->valid_channels = (const uint8 *)GET_DATA(ds_id, locale_valid_channels);
	loc_data->restricted_channels = (const uint8 *)GET_DATA(ds_id, restricted_channels);
	for (bw_idx = 0u; bw_idx < ARRAYSIZE(loc_data->combs); ++bw_idx) {
		loc_data->combs[bw_idx] = &ds->valid_channel_combs[band][bw_idx];
	}
	loc_data->ext4_rate_sets_bw = ds->ext4_rate_sets_bw[band];
	loc_data->ext4_rate_sets_indices_bw = ds->ext4_rate_sets_indices_bw[band];
	loc_data->he_rate_sets_bw = ds->he_rate_sets_bw[band];
	loc_data->he_rate_sets_indices_bw = ds->he_rate_sets_indices_bw[band];
	loc_data->ext_ru_rate_sets_bw = ds->ext_ru_rate_sets_bw[band];
	loc_data->ext_ru_rate_sets_indices_bw = ds->ext_ru_rate_sets_indices_bw[band];
	return TRUE;
}

/** Fills field or group of fields in given per-band-bandwidth array of pointers
 * \param[out] dst_array Array being filled. Its true signature is:
 * 'const SOMETYPE *somearray[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT]'
 * \param[in] band Band of given piece of data
 * \param[in] bw Bandwidth of given piece of data
 * \param[in] ds_id Data source of given piece of data
 * \param[in] field_offset Piece of data. Pointer field within clm_registry_t,
 * specified by its offset within the structure
 * \param[in] enabled FALSE means that piece of data shall be ignored
 * \param[in] per_band_bw TRUE means that piece of data is per-band-bandwidth
 * field. Should be used to initialize just one field in target array
 * \param[in] per_bw Evaluated if 'per_band_bw' is FALSE. TRUE means that piece
 * is per-bandwidth field (i.e. if 'band' parameter is 5G, then value shall be
 * used for all bands for given bandwidths, if 'band' is not 5GHz, value shall
 * be ignored). FALSE means that value sshal be used for all bands and
 * bandwidths if 'band' is 5GHz and 'bw' is 20MHz, ignored otherwise
 */
static void
fill_band_bw_field(void *dst_array, clm_band_t band, clm_bandwidth_t bw, data_source_id_t ds_id,
	uint32 field_offset, bool enabled, bool per_band_bw, bool per_bw)
{
	const void *(*dst)[CLM_BW_NUM_ROM_COMPAT] =
		(const void *(*)[CLM_BW_NUM_ROM_COMPAT])dst_array;
	const void *ptr;
	uint32 band_idx, bw_idx;
	if (!enabled) {
		return;
	}
	if (!(per_band_bw || (per_bw && (band == CLM_BAND_5G)) ||
		((band == CLM_BAND_5G) && (bw == CLM_BW_20))))
	{
		return;
	}
#ifdef WL_BAND6G
	if ((band == CLM_BAND_6G) &&
		!(get_data_sources(ds_id)->registry_flags2 & CLM_REGISTRY_FLAG2_6GHZ))
	{
		return;
	}
#endif /* WL_BAND6G */
#ifdef WL11BE
	if ((bw == CLM_BW_320) &&
		!(get_data_sources(ds_id)->registry_flags2 & CLM_REGISTRY_FLAG2_320MHZ))
	{
		return;
	}
#endif /* WL11BE */
	ptr = get_data(ds_id, field_offset);
	if (per_band_bw) {
		dst[band][bw] = ptr;
	} else if (per_bw && (band == CLM_BAND_5G)) {
		for (band_idx = 0u; band_idx < CLM_BAND_NUM; ++band_idx) {
			if ((band_idx != CLM_BAND_2G) || (bw <= CLM_BW_40)) {
				dst[band_idx][bw] = ptr;
			}
		}
	} else {
		for (band_idx = 0u; band_idx < CLM_BAND_NUM; ++band_idx) {
			for (bw_idx = 0u; bw_idx < CLM_BW_NUM_ROM_COMPAT; ++bw_idx) {
				if ((band_idx != CLM_BAND_2G) || (bw_idx <= CLM_BW_40)) {
					dst[band_idx][bw_idx] = ptr;
				}
			}
		}
	}
}

/** Tries to fill valid_channel_combs using given CLM data source
 * This function takes information about 20MHz channels from BLOB, 40MHz
 * channels from valid_channel_...g_40m_set structures hardcoded in this module
 * to save BLOB space
 * \param[in] ds_id Identifier of CLM data to get information from
 */
static void
try_fill_valid_channel_combs(data_source_id_t ds_id)
{
	static const struct band_bw_field {
		clm_band_t band;
		clm_bandwidth_t bw;
		uint32 field_offset;
	} fields [] = {
		{CLM_BAND_2G, CLM_BW_20,
		OFFSETOF(clm_registry_t, valid_channels_2g_20m)},
		{CLM_BAND_2G, CLM_BW_40,
		OFFSETOF(clm_registry_t, valid_channels_2g_40m)},
		{CLM_BAND_5G, CLM_BW_20,
		OFFSETOF(clm_registry_t, valid_channels_5g_20m)},
		{CLM_BAND_5G, CLM_BW_40,
		OFFSETOF(clm_registry_t, valid_channels_5g_40m)},
		{CLM_BAND_5G, CLM_BW_80,
		OFFSETOF(clm_registry_t, valid_channels_5g_80m)},
		{CLM_BAND_5G, CLM_BW_160,
		OFFSETOF(clm_registry_t, valid_channels_5g_160m)},
#ifdef WL_BAND6G
		{CLM_BAND_6G, CLM_BW_20,
		OFFSETOF(clm_registry_t, valid_channels_6g_20m)},
		{CLM_BAND_6G, CLM_BW_40,
		OFFSETOF(clm_registry_t, valid_channels_6g_40m)},
		{CLM_BAND_6G, CLM_BW_80,
		OFFSETOF(clm_registry_t, valid_channels_6g_80m)},
		{CLM_BAND_6G, CLM_BW_160,
		OFFSETOF(clm_registry_t, valid_channels_6g_160m)},
#ifdef CLM_320_MHZ_SUPPORTED
		{CLM_BAND_6G, CLM_BW_320,
		OFFSETOF(clm_registry_t, valid_channels_6g_320m)},
#endif /* CLM_320_MHZ_SUPPORTED */
#endif /* WL_BAND6G */
	};
	const clm_channel_comb_set_t *combs[CLM_BAND_NUM][CLM_BW_NUM_ROM_COMPAT];
	uint32 band, bw;
	const struct band_bw_field *fd;
	data_dsc_t *ds = get_data_sources(ds_id);
	if (!ds->data) {
		/* Can't obtain data - make all combs empty */
		for (band = 0u; band < CLM_BAND_NUM; ++band) {
			for (bw = 0u; bw < ARRAYSIZE(ds->valid_channel_combs[band]); ++bw) {
				ds->valid_channel_combs[band][bw].num = 0u;
			}
		}
		return;
	}
	/* Fill combs[][] with references to combs that will be retrieved from
	 * BLOB
	 */
	bzero((void *)combs, sizeof(combs));
	for (fd = fields; fd < (fields + ARRAYSIZE(fields)); ++fd) {
		fill_band_bw_field((void *)combs, fd->band, fd->bw, ds_id, fd->field_offset,
			(fd->bw == CLM_BW_20) ||
			(ds->registry_flags & CLM_REGISTRY_FLAG_HIGH_BW_COMBS),
			TRUE, FALSE);
	}
	/* Transferring combs from BLOB to valid_channel_combs[][] */
	for (band = 0u; band < CLM_BAND_NUM; ++band) {
		for (bw = 0u; bw < ARRAYSIZE(ds->valid_channel_combs[band]); ++bw) {
			const clm_channel_comb_set_t *comb_set = combs[band][bw];
			clm_channel_comb_set_t *dest_comb_set =
				&ds->valid_channel_combs[band][bw];
			if (!comb_set || !comb_set->num) {
				/* Incremental comb set empty or absent?
				 * Borrowing from base
				 */
				if (ds_id == DS_INC) {
					*dest_comb_set =
						get_data_sources(DS_MAIN)->
						valid_channel_combs[band][bw];
				}
				continue;
			}
			/* Nonempty comb set - first copy all fields... */
			*dest_comb_set = *comb_set;
			/* ... then relocate pointer (to comb vector) */
			dest_comb_set->set =
				(const clm_channel_comb_t *)relocate_ptr(ds_id, comb_set->set);
			/* Base comb set empty? Share from incremental */
			if ((ds_id == DS_INC) &&
				!get_data_sources(DS_MAIN)->valid_channel_combs[band][bw].num)
			{
				get_data_sources(DS_MAIN)->valid_channel_combs[band][bw] =
					*dest_comb_set;
			}
		}
	}
	if (!(ds->registry_flags & CLM_REGISTRY_FLAG_HIGH_BW_COMBS)) {
		/* No 40+MHz combs in BLOB => using hardcoded ones */
		ds->valid_channel_combs[CLM_BAND_2G][CLM_BW_40] = valid_channel_2g_40m_set;
		ds->valid_channel_combs[CLM_BAND_5G][CLM_BW_40] = valid_channel_5g_40m_set;
		ds->valid_channel_combs[CLM_BAND_5G][CLM_BW_80] = valid_channel_5g_80m_set;
		ds->valid_channel_combs[CLM_BAND_5G][CLM_BW_160] = valid_channel_5g_160m_set;
	}
}

/** In given comb set looks up for comb that contains given channel
 * \param[in] channel Channel to find comb for
 * \param[in] combs Comb set to find comb in
 * \return NULL or address of given comb
 */
static const clm_channel_comb_t *
get_comb(uint32 channel, const clm_channel_comb_set_t *combs)
{
	const clm_channel_comb_t *ret, *comb_end;
	/* Fail on nonempty comb set with NULL set pointer (which means that
	 * ClmCompiler has a bug)
	 */
	ASSERT((combs->set != NULL) || (combs->num == 0u));
	for (ret = combs->set, comb_end = ret + combs->num; ret != comb_end; ++ret) {
		if ((channel >= ret->first_channel) && (channel <= ret->last_channel) &&
		   (((channel - ret->first_channel) % ret->stride) == 0u))
		{
			return ret;
		}
	}
	return NULL;
}

/** Among combs whose first channel is greater than given looks up one with
 * minimum first channel
 * \param[in] channel Channel to find comb for
 * \param[in] combs Comb set to find comb in
 * \return Address of found comb, NULL if all combs have first channel smaller
 * than given
 */
static const clm_channel_comb_t *
get_next_comb(uint32 channel, const clm_channel_comb_set_t *combs)
{
	const clm_channel_comb_t *ret, *comb, *comb_end;
	/* Fail on nonempty comb set with NULL set pointer (which means that
	 * ClmCompiler has a bug)
	 */
	ASSERT((combs->set != NULL) || (combs->num == 0u));
	for (ret = NULL, comb = combs->set, comb_end = comb + combs->num; comb != comb_end;
		++comb)
	{
		if (comb->first_channel <= channel) {
			continue;
		}
		if (!ret || (comb->first_channel < ret->first_channel)) {
			ret = comb;
		}
	}
	return ret;
}

/** Fills channel set structure from given source
 * \param[out] channels Channel set to fill
 * \param[in] channel_defs Byte string sequence that contains channel set
 * definitions
 * \param[in] def_idx Index of byte string that contains required channel set
 * definition
 * \param[in] ranges Vector of channel ranges
 * \param[in] combs Set of combs of valid channel numbers
 */
static void
get_channels(clm_channels_t *channels, const uint8 *channel_defs, uint8 def_idx,
	const clm_channel_range_t *ranges, const clm_channel_comb_set_t *combs)
{
	uint32 num_ranges;

	if (!channels) {
		return;
	}
	bzero(channels->bitvec, sizeof(channels->bitvec));
	if (!channel_defs) {
		return;
	}
	if (def_idx == CLM_RESTRICTED_SET_NONE) {
		return;
	}
	channel_defs = get_byte_string(channel_defs, def_idx, NULL);
	num_ranges = *channel_defs++;
	if (!num_ranges) {
		return;
	}
	/* Fail on nonempty range set with NULL range pointer (which means that
	 * ClmCompiler has a bug)
	 */
	ASSERT(ranges != NULL);
	/* Fail on empty comb set with nonempty channel range set (which means
	 * that ClmCompiler has a bug)
	 */
	ASSERT(combs != NULL);
	while (num_ranges--) {
		uint8 r = *channel_defs++;
		if (r == CLM_RANGE_ALL_CHANNELS) {
			const clm_channel_comb_t *comb = combs->set;
			uint32 num_combs;
			/* Fail on nonempty comb set with NULL set pointer
			 * (which means that ClmCompiler has a bug)
			 */
			ASSERT((comb != NULL) || (combs->num == 0u));
			for (num_combs = combs->num; num_combs--; ++comb) {
				uint32 chan;
				for (chan = comb->first_channel; chan <= comb->last_channel;
					chan += comb->stride) {
					channels->bitvec[chan / 8u] |= (uint8)(1u << (chan % 8u));
				}
			}
		} else {
			uint32 chan = ranges[r].start, end = ranges[r].end;
			const clm_channel_comb_t *comb = get_comb(chan, combs);
			if (!comb) {
				continue;
			}
			for (;;) {
				channels->bitvec[chan / 8u] |= (uint8)(1u << (chan % 8u));
				if (chan >= end) {
					break;
				}
				if (chan < comb->last_channel) {
					chan += comb->stride;
					continue;
				}
				comb = get_next_comb(chan, combs);
				if (!comb || (comb->first_channel > end)) {
					break;
				}
				chan = comb->first_channel;
			}
		}
	}
}

/** True if given channel belongs to given range and belongs to comb that
 * represents this range
 * \param[in] channel Channel in question
 * \param[in] range Range in question
 * \param[in] combs Comb set
 * \param[in] other_in_pair Other channel in 80+80 channel pair. Used when
 * combs parameter is zero-length
 * \return True if given channel belongs to given range and belong to comb that
 * represent this range
 */
static bool
channel_in_range(uint32 channel, const clm_channel_range_t *range,
	const clm_channel_comb_set_t *combs, uint32 other_in_pair)
{
	const clm_channel_comb_t *comb;
	/* Fail on NULL comb set descriptor pointer (which means that
	 * ClmCompiler has a bug)
	 */
	ASSERT(combs != NULL);
	if (!combs->num) {
		return (channel == range->start) && (other_in_pair == range->end);
	}
	if ((channel < range->start) || (channel > range->end)) {
		return FALSE;
	}
	comb = get_comb(range->start, combs);
	while (comb && (comb->last_channel < channel)) {
		comb = get_next_comb(comb->last_channel, combs);
	}
	return comb && (comb->first_channel <= channel) &&
		!((channel - comb->first_channel) % comb->stride);
}

#if defined(WL_RU_NUMRATES) || defined(WL_NUM_HE_RT)
/** Checks if two numerical ranges intersect by at least given length */
static bool
ranges_intersect(int32 l1, int32 r1, int32 l2, int32 r2, int32 min_intersection)
{
	return (((r1 < r2) ? r1 : r2) - ((l1 > l2) ? l1 : l2)) >= min_intersection;
}
#endif /* defined(WL_RU_NUMRATES) || defined(WL_NUM_HE_RT) */

/** Fills rate_type[] */
static void
fill_rate_types(void)
{
	/** Rate range descriptor */
	static const struct {
		/** First rate in range */
		uint32 start;

		/* Number of rates in range */
		uint32 length;

		/* Rate type for range */
		clm_rate_type_t rt;
	} rate_ranges[] = {
		{0,                      WL_NUMRATES,        RT_MCS},
		{WL_RATE_1X1_DSSS_1,     WL_RATESET_SZ_DSSS, RT_DSSS},
		{WL_RATE_1X2_DSSS_1,     WL_RATESET_SZ_DSSS, RT_DSSS},
		{WL_RATE_1X3_DSSS_1,     WL_RATESET_SZ_DSSS, RT_DSSS},
		{WL_RATE_1X1_OFDM_6,     WL_RATESET_SZ_OFDM, RT_OFDM},
		{WL_RATE_1X2_CDD_OFDM_6, WL_RATESET_SZ_OFDM, RT_OFDM},
		{WL_RATE_1X3_CDD_OFDM_6, WL_RATESET_SZ_OFDM, RT_OFDM},
#ifdef CLM_TXBF_RATES_SUPPORTED
		{WL_RATE_1X2_TXBF_OFDM_6, WL_RATESET_SZ_OFDM, RT_OFDM},
		{WL_RATE_1X3_TXBF_OFDM_6, WL_RATESET_SZ_OFDM, RT_OFDM},
#endif /* LM_TXBF_RATES_SUPPORTED */
#if WL_NUMRATES >= 336
		{WL_RATE_1X4_DSSS_1,     WL_RATESET_SZ_DSSS, RT_DSSS},
		{WL_RATE_1X4_CDD_OFDM_6, WL_RATESET_SZ_OFDM, RT_OFDM},
#ifdef CLM_TXBF_RATES_SUPPORTED
		{WL_RATE_1X4_TXBF_OFDM_6, WL_RATESET_SZ_OFDM, RT_OFDM},
#endif /* CLM_TXBF_RATES_SUPPORTED */
#endif /* WL_NUMRATES >= 336 */
	};
	uint32 range_idx;
	for (range_idx = 0u; range_idx < ARRAYSIZE(rate_ranges); ++range_idx) {
		uint32 rate_idx = rate_ranges[range_idx].start;
		uint32 end = rate_idx + rate_ranges[range_idx].length;
		clm_rate_type_t rt = rate_ranges[range_idx].rt;
		do {
			set_rate_type(rate_idx, rt);
		} while (++rate_idx < end);
	}
#ifdef WL_RU_NUMRATES
	{
		uint32 su_mode_idx;
		for (su_mode_idx = 0u; su_mode_idx < CLM_NUM_RU_RATE_MODES; ++su_mode_idx) {
			uint32 su_base_code = get_su_base_rate(su_mode_idx);
#ifdef CLM_EHT_RATES_SUPPORTED
			uint32 num_eht_rates = (get_tx_mode_info(su_mode_idx)->nss == 1)
				? EHT_SS1_NUM : EHT_SS2P_NUM;
#endif /* CLM_EHT_RATES_SUPPORTED */
			uint32 ri;
			for (ri = su_base_code; ri < (su_base_code + WL_RATESET_SZ_HE_MCS); ++ri) {
				set_rate_type(ri, RT_SU);
			}
#ifdef CLM_EHT_RATES_SUPPORTED
			for (; num_eht_rates-- != 0; ++ri) {
				set_rate_type(ri, RT_EHT);
			}
#endif /* CLM_EHT_RATES_SUPPORTED */
		}
	}
#endif /* WL_RU_NUMRATES */
}

#ifdef WL_RU_NUMRATES
/** Retrieves a parameters of given RU rate
 * \param[in] ru_rate RU rate in question
 * \param[out] rate_dsc Pointer to clm_he_rate_dsc_t, containing all rate
 * parameters, except rate type
 * \param[out] rate_type Rate type of given rate
 */
static void
BCMRAMFN(get_ru_rate_info)(clm_ru_rates_t ru_rate, const clm_he_rate_dsc_t **ru_rate_dsc,
	wl_he_rate_type_t *ru_rate_type)
{
	*ru_rate_dsc = he_rate_descriptors + (ru_rate % CLM_NUM_RU_RATE_MODES);
	*ru_rate_type = (wl_he_rate_type_t)(ru_rate / CLM_NUM_RU_RATE_MODES) + 1u;
}

/** Fills-in RU(OFDMA) rates - related data structures */
static void
fill_ru_rate_info(void)
{
	static const struct {
		clm_ru_rates_t start;
		clm_bandwidth_t min_bw;
	} rate_ranges[] = {
		{WL_RU_RATE_1X1_26SS1, CLM_BW_20},
		{WL_RU_RATE_1X1_52SS1, CLM_BW_20},
		{WL_RU_RATE_1X1_106SS1, CLM_BW_20},
		{WL_RU_RATE_1X1_UBSS1, CLM_BW_NUM},
		{WL_RU_RATE_1X1_LUBSS1, CLM_BW_NUM},
		{WL_RU_RATE_1X1_242SS1, CLM_BW_20},
		{WL_RU_RATE_1X1_484SS1, CLM_BW_40},
		{WL_RU_RATE_1X1_996SS1, CLM_BW_80},
#ifdef CLM_RU996_2_SUPPORTED
		{WL_RU_RATE_1X1_996_2SS1, CLM_BW_160},
#endif /* CLM_RU996_2_SUPPORTED */
#ifdef CLM_MRU_RATES_SUPPORTED
		{WL_RU_RATE_1X1_52_26SS1, CLM_BW_20},
		{WL_RU_RATE_1X1_106_26SS1, CLM_BW_20},
		{WL_RU_RATE_1X1_484_242SS1, CLM_BW_80},
		{WL_RU_RATE_1X1_996_484SS1, CLM_BW_160},
		{WL_RU_RATE_1X1_996_484_242SS1, CLM_BW_160},
#ifdef WL11BE
		{WL_RU_RATE_1X1_996_2_484SS1, CLM_BW_320},
		{WL_RU_RATE_1X1_996_3SS1, CLM_BW_320},
		{WL_RU_RATE_1X1_996_3_484SS1, CLM_BW_320},
		{WL_RU_RATE_1X1_996_4SS1, CLM_BW_320},
#else /* WL11BE */
		{WL_RU_RATE_1X1_996_2_484SS1, CLM_BW_NUM},
		{WL_RU_RATE_1X1_996_3SS1, CLM_BW_NUM},
		{WL_RU_RATE_1X1_996_3_484SS1, CLM_BW_NUM},
		{WL_RU_RATE_1X1_996_4SS1, CLM_BW_NUM},
#endif /* else WL11BE */
#endif /* CLM_MRU_RATES_SUPPORTED */
	};
	uint32 range_idx, mode_idx;
	ASSERT(ARRAYSIZE(he_rate_descriptors) == CLM_NUM_RU_RATE_MODES);
	for (range_idx = 0u; range_idx < ARRAYSIZE(rate_ranges); ++range_idx) {
		for (mode_idx = 0; mode_idx < CLM_NUM_RU_RATE_MODES; ++mode_idx) {
			set_ru_rate_min_bw(rate_ranges[range_idx].start + mode_idx,
				rate_ranges[range_idx].min_bw);
		}
	}
	for (mode_idx = 0u; mode_idx < CLM_NUM_RU_RATE_MODES; ++mode_idx) {
		/* Prefilling hash for collision checking */
		set_he_tx_mode(get_su_base_rate(mode_idx), 0xFF);
	}
	for (mode_idx = 0u; mode_idx < CLM_NUM_RU_RATE_MODES; ++mode_idx) {
		uint32 he0_rate = get_su_base_rate(mode_idx);
		/* Checking for hash collision. If it happened - hash constants
		 * (SU_TX_MODE_HASH_...) must be changed
		 */
		ASSERT(get_he_tx_mode(he0_rate) == 0xFFu);
		/* This check is used to identify HE0 rates during
		 * clm_he_limit() and clm_ru_limits(). If this condition will
		 * cease to be met after next rate code rehash - something need
		 * to be done
		 */
		ASSERT((get_rate_type(he0_rate) == RT_SU) &&
			(get_rate_type(he0_rate - 1u) != RT_SU));
		set_he_tx_mode(he0_rate, mode_idx);
	}
}
#endif /*  WL_RU_NUMRATES */

/** True if BLOB format with given major version supported
 * Made separate function to avoid ROM invalidation (albeit it is always wrong idea to put
 * CLM to ROM)
 * \param[in] BLOB major version
 * Returns TRUE if given major BLOB format version supported
 */
static bool
is_blob_version_supported(uint32 format_major)
{
	return (format_major <= FORMAT_VERSION_MAJOR) &&
		(format_major >= FORMAT_MIN_COMPAT_MAJOR);
}

/** Verifies that BLOB's flags and flags2 field are consistent with environment
 * in which ClmAPI was compiled
 * It is expected that when this function is called data_dsc_t::flags and
 * data_dsc_t::flags2 fields are already filled in
 * \param[in] ds_id Identifier of CLM data set
 * Returns CLM_RESULT_OK if no problems were found, CLM_RESULT_ERR otherwise
 */
static clm_result_t
check_data_flags_compatibility(data_source_id_t ds_id)
{
	data_dsc_t *ds = get_data_sources(ds_id);
	uint32 registry_flags = ds->registry_flags;
	uint32 registry_flags2 = ds->registry_flags2;
	if ((registry_flags & CLM_REGISTRY_FLAG_NUM_RATES_MASK) &&
		((registry_flags & CLM_REGISTRY_FLAG_NUM_RATES_MASK) !=
		((WL_NUMRATES << CLM_REGISTRY_FLAG_NUM_RATES_SHIFT) &
		CLM_REGISTRY_FLAG_NUM_RATES_MASK)))
	{
		/* BLOB was compiled for WL_NUMRATES different from one, ClmAPI
		 * was compiled for
		 */
		return CLM_RESULT_ERR;
	}
	/* Unknown flags present. May never happen for regularly released
	 * ClmAPI sources, but can for ClmAPI with patched-in features from
	 * more recent BLOB formats
	 */
	if ((registry_flags & ~(uint32)CLM_REGISTRY_FLAG_ALL) ||
		(registry_flags2 & ~(uint32)CLM_REGISTRY_FLAG2_ALL))
	{
		/* BLOB contains flags ClmAPI is unaware of */
		return CLM_RESULT_ERR;
	}
#ifdef WL_RU_NUMRATES
	if ((registry_flags2 & CLM_REGISTRY_FLAG2_HE_RU_FIXED) &&
		((registry_flags2 & CLM_REGISTRY_FLAG2_NUM_RU_RATES_MASK) !=
		((WL_RU_NUMRATES << CLM_REGISTRY_FLAG2_NUM_RU_RATES_SHIFT) &
		CLM_REGISTRY_FLAG2_NUM_RU_RATES_MASK)))
	{
		/* Number of RU rates doesn't match one that BLOB was compiled
		 * for
		 */
		return CLM_RESULT_ERR;
	}
#endif /* WL_RU_NUMRATES */
	return CLM_RESULT_OK;
}

/** Fills-in fields in data_dsc_t that related to layout of region record
 * \param[in] ds_id Identifier of CLM data set
 * Returns CLM_RESULT_OK if no problems were found, CLM_RESULT_ERR otherwise
 */
static clm_result_t
fill_region_record_data_fields(data_source_id_t ds_id)
{
	data_dsc_t *ds = get_data_sources(ds_id);
	uint32 registry_flags = ds->registry_flags;
	uint32 registry_flags2 = ds->registry_flags2;
	ds->reg_rev16_idx = -1;
	ds->reg_loc10_idx = -1;
	ds->reg_loc12_idx = -1;
	ds->reg_loc14_idx = -1;
	ds->reg_loc12_6g_idx = -1;
	ds->reg_loc16_6g_idx = -1;
	ds->reg_flags_idx = -1;
	ds->reg_flags_2_idx = -1;
	ds->loc_idx_mask = 0xFFu;
	if (registry_flags & CLM_REGISTRY_FLAG_CD_REGIONS) {
		int32 idx = 0;
		int32 extra_loc_idx_bytes =
			(registry_flags & CLM_REGISTRY_FLAG_CD_LOC_IDX_BYTES_MASK) >>
			CLM_REGISTRY_FLAG_CD_LOC_IDX_BYTES_SHIFT;
		/* Indicates format used by shims that extends
		 * clm_country_rev_definition10_fl_t but do not use index-based
		 * implementation of CC/rev references
		 */
		bool loc12_flag_swap =
			!!(registry_flags & CLM_REGISTRY_FLAG_REGION_LOC_12_FLAG_SWAP);
		if (registry_flags2 & CLM_REGISTRY_FLAG2_6GHZ) {
			idx += 2;
		}
		ds->loc_idx_mask = (1u << (8u + 2u * extra_loc_idx_bytes)) - 1u;
		ds->reg_rev16_idx =
			(registry_flags & CLM_REGISTRY_FLAG_CD_16_BIT_REV) ? idx++ : -1;
		ds->reg_loc10_idx = (extra_loc_idx_bytes-- > 0) ? idx++ : -1;
		if (loc12_flag_swap) {
			ds->reg_flags_idx = idx++;
			ds->reg_loc12_idx = idx++;
			ds->loc_idx_mask = 0xFFFu;
		} else {
			ds->reg_loc12_idx = (extra_loc_idx_bytes-- > 0) ? idx++ : -1;
			ds->reg_loc14_idx = (extra_loc_idx_bytes-- > 0) ? idx++ : -1;
			ds->reg_flags_idx = idx++;
			if (registry_flags2 & CLM_REGISTRY_FLAG2_6GHZ) {
				if (ds->reg_loc10_idx >= 0) {
					ds->reg_loc12_6g_idx = idx++;
				}
				if (ds->reg_loc14_idx >= 0) {
					ds->reg_loc16_6g_idx = idx++;
				}
			}
		}
		ds->country_rev_rec_len =
			OFFSETOF(clm_country_rev_definition_cd10_t, extra) + idx;
		ds->ccrev_format =
			loc12_flag_swap ? CCREV_FORMAT_CC_REV :
			((registry_flags & CLM_REGISTRY_FLAG_CD_16_BIT_REGION_INDEX)
			? CCREV_FORMAT_CC_IDX16 : CCREV_FORMAT_CC_IDX8);
	} else if (registry_flags & CLM_REGISTRY_FLAG_COUNTRY_10_FL) {
		ds->loc_idx_mask = 0x3FFu;
		ds->reg_loc10_idx = 0;
		ds->reg_flags_idx = 1;
		ds->country_rev_rec_len = sizeof(clm_country_rev_definition10_fl_t);
		ds->ccrev_format = CCREV_FORMAT_CC_REV;
	} else {
		ds->country_rev_rec_len = sizeof(clm_country_rev_definition_t);
		ds->ccrev_format = CCREV_FORMAT_CC_REV;
	}
	if (registry_flags & CLM_REGISTRY_FLAG_REGION_FLAG_2) {
		ds->reg_flags_2_idx = ds->country_rev_rec_len++ -
			OFFSETOF(clm_country_rev_definition_cd10_t, extra);
		ds->scr_idx_4 = FALSE;
	} else if (ds->registry_flags2 &CLM_REGISTRY_FLAG2_INDEXED_COUNTRY_FLAGS) {
		ds->scr_idx_4 = FALSE;
	}
	return CLM_RESULT_OK;
}

/** Fills-in fields in data_dsc_t that points to various collections inside BLOB
 * \param[in] ds_id Identifier of CLM data set
 * Returns CLM_RESULT_OK if no problems were found, CLM_RESULT_ERR otherwise
 */
static clm_result_t
fill_pointer_data_fields(data_source_id_t ds_id)
{
	static const struct band_bw_fields_dsc {
		clm_band_t band;
		clm_bandwidth_t bw;
		uint32 main_rates_field_offset;
		uint32 ext_rates_field_offset;
		uint32 ext4_rates_field_offset;
		uint32 he_rates_field_offset;
		uint32 main_rates_indices_field_offset;
		uint32 ext_rates_indices_field_offset;
		uint32 ext4_rates_indices_field_offset;
		uint32 he_rates_indices_field_offset;
		uint32 channel_ranges_field_offset;
		uint32 ext_ru_rates_field_offset;
		uint32 ext_ru_rates_indices_field_offset;
	} band_bw_fields[] = {
		{CLM_BAND_2G, CLM_BW_20,
		OFFSETOF(clm_registry_t, locale_rate_sets_2g_20m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_2g_20m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_2g_20m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_2g_20m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_2g_20m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_2g_20m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_2g_20m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_2g_20m),
		OFFSETOF(clm_registry_t, channel_ranges_2g_20m), 0u, 0u},
		{CLM_BAND_2G, CLM_BW_40,
		OFFSETOF(clm_registry_t, locale_rate_sets_2g_40m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_2g_40m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_2g_40m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_2g_40m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_2g_40m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_2g_40m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_2g_40m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_2g_40m),
		OFFSETOF(clm_registry_t, channel_ranges_2g_40m), 0u, 0u},
		{CLM_BAND_5G, CLM_BW_20,
		OFFSETOF(clm_registry_t, locale_rate_sets_5g_20m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_5g_20m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_5g_20m),
		OFFSETOF(clm_registry_t, locale_rate_sets_he),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_5g_20m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_5g_20m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_5g_20m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_5g_20m),
		OFFSETOF(clm_registry_t, channel_ranges_20m), 0u, 0u},
		{CLM_BAND_5G, CLM_BW_40,
		OFFSETOF(clm_registry_t, locale_rate_sets_5g_40m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_5g_40m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_5g_40m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_5g_40m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_5g_40m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_5g_40m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_5g_40m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_5g_40m),
		OFFSETOF(clm_registry_t, channel_ranges_40m), 0u, 0u},
		{CLM_BAND_5G, CLM_BW_80,
		OFFSETOF(clm_registry_t, locale_rate_sets_5g_80m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_5g_80m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_5g_80m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_5g_80m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_5g_80m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_5g_80m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_5g_80m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_5g_80m),
		OFFSETOF(clm_registry_t, channel_ranges_80m), 0u, 0u},
		{CLM_BAND_5G, CLM_BW_80_80,
		OFFSETOF(clm_registry_t, locale_rate_sets_5g_80m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_5g_80m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_5g_80m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_5g_80m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_5g_80m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_5g_80m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_5g_80m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_5g_80m),
		OFFSETOF(clm_registry_t, channel_ranges_80m), 0u, 0u},
		{CLM_BAND_5G, CLM_BW_160,
		OFFSETOF(clm_registry_t, locale_rate_sets_5g_160m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_5g_160m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_5g_160m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_5g_160m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_5g_160m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_5g_160m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_5g_160m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_5g_160m),
		OFFSETOF(clm_registry_t, channel_ranges_160m),
		OFFSETOF(clm_registry_t, locale_ext_ru_rate_sets_5g_160m),
		OFFSETOF(clm_registry_t, locale_ext_ru_rate_sets_index_5g_160m)},
#ifdef WL_BAND6G
		{CLM_BAND_6G, CLM_BW_20,
		OFFSETOF(clm_registry_t, locale_rate_sets_6g_20m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_6g_20m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_6g_20m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_6g_20m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_6g_20m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_6g_20m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_6g_20m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_6g_20m),
		OFFSETOF(clm_registry_t, channel_ranges_6g_20m), 0u, 0u},
		{CLM_BAND_6G, CLM_BW_40,
		OFFSETOF(clm_registry_t, locale_rate_sets_6g_40m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_6g_40m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_6g_40m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_6g_40m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_6g_40m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_6g_40m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_6g_40m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_6g_40m),
		OFFSETOF(clm_registry_t, channel_ranges_6g_40m), 0u, 0u},
		{CLM_BAND_6G, CLM_BW_80,
		OFFSETOF(clm_registry_t, locale_rate_sets_6g_80m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_6g_80m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_6g_80m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_6g_80m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_6g_80m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_6g_80m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_6g_80m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_6g_80m),
		OFFSETOF(clm_registry_t, channel_ranges_6g_80m), 0u, 0u},
		{CLM_BAND_6G, CLM_BW_80_80,
		OFFSETOF(clm_registry_t, locale_rate_sets_6g_80m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_6g_80m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_6g_80m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_6g_80m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_6g_80m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_6g_80m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_6g_80m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_6g_80m),
		OFFSETOF(clm_registry_t, channel_ranges_6g_80m), 0u, 0u},
		{CLM_BAND_6G, CLM_BW_160,
		OFFSETOF(clm_registry_t, locale_rate_sets_6g_160m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_6g_160m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_6g_160m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_6g_160m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_6g_160m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_6g_160m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_6g_160m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_6g_160m),
		OFFSETOF(clm_registry_t, channel_ranges_6g_160m),
		OFFSETOF(clm_registry_t, locale_ext_ru_rate_sets_6g_160m),
		OFFSETOF(clm_registry_t, locale_ext_ru_rate_sets_index_6g_160m)},
#ifdef CLM_320_MHZ_SUPPORTED
		{CLM_BAND_6G, CLM_BW_320,
		OFFSETOF(clm_registry_t, locale_rate_sets_6g_320m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_6g_320m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_6g_320m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_6g_320m),
		OFFSETOF(clm_registry_t, locale_rate_sets_index_6g_320m),
		OFFSETOF(clm_registry_t, locale_ext_rate_sets_index_6g_320m),
		OFFSETOF(clm_registry_t, locale_ext4_rate_sets_index_6g_320m),
		OFFSETOF(clm_registry_t, locale_he_rate_sets_index_6g_320m),
		OFFSETOF(clm_registry_t, channel_ranges_6g_320m),
		OFFSETOF(clm_registry_t, locale_ext_ru_rate_sets_6g_320m),
		OFFSETOF(clm_registry_t, locale_ext_ru_rate_sets_index_6g_320m)},
#endif /* CLM_320_MHZ_SUPPORTED */
#endif /* WL_BAND6G */
	};
	data_dsc_t *ds = get_data_sources(ds_id);
	uint32 registry_flags = ds->registry_flags;
	uint32 registry_flags2 = ds->registry_flags2;
	const struct band_bw_fields_dsc *fd;
	if (registry_flags & CLM_REGISTRY_FLAG_REGREV_REMAP) {
		ds->regrev_remap = (const clm_regrev_cc_remap_set_t *)GET_DATA(ds_id, regrev_remap);
	}
	for (fd = band_bw_fields; fd < (band_bw_fields + ARRAYSIZE(band_bw_fields)); ++fd) {
		fill_band_bw_field((void *)ds->chan_ranges_band_bw, fd->band, fd->bw, ds_id,
			fd->channel_ranges_field_offset, TRUE,
			(registry_flags2 & CLM_REGISTRY_FLAG2_PER_BAND_BW_RANGES) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BW_RS) != 0u);
		fill_band_bw_field((void *)ds->rate_sets_bw, fd->band, fd->bw, ds_id,
			fd->main_rates_field_offset, TRUE,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BAND_RATES) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BW_RS) != 0u);
		fill_band_bw_field((void *)ds->ext_rate_sets_bw, fd->band, fd->bw, ds_id,
			fd->ext_rates_field_offset,
			(registry_flags & CLM_REGISTRY_FLAG_EXT_RATE_SETS) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BAND_RATES) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BW_RS) != 0u);
		fill_band_bw_field((void *)ds->ext4_rate_sets_bw, fd->band, fd->bw, ds_id,
			fd->ext4_rates_field_offset,
			(registry_flags2 & CLM_REGISTRY_FLAG2_EXT4) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BAND_RATES) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BW_RS) != 0u);
		fill_band_bw_field((void *)ds->he_rate_sets_bw, fd->band, fd->bw, ds_id,
			fd->he_rates_field_offset,
			(registry_flags2 & CLM_REGISTRY_FLAG2_HE_LIMITS) != 0u,
			(registry_flags2 & CLM_REGISTRY_FLAG2_RU_SETS_PER_BW_BAND) != 0u,
			FALSE);
		fill_band_bw_field((void *)ds->rate_sets_indices_bw, fd->band, fd->bw, ds_id,
			fd->main_rates_indices_field_offset,
			(registry_flags2 & CLM_REGISTRY_FLAG2_RATE_SET_INDEX) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BAND_RATES) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BW_RS) != 0u);
		fill_band_bw_field((void *)ds->ext_rate_sets_indices_bw, fd->band, fd->bw, ds_id,
			fd->ext_rates_indices_field_offset,
			(registry_flags & CLM_REGISTRY_FLAG_EXT_RATE_SETS) &&
			(registry_flags2 & CLM_REGISTRY_FLAG2_RATE_SET_INDEX),
			(registry_flags & CLM_REGISTRY_FLAG_PER_BAND_RATES) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BW_RS) != 0u);
		fill_band_bw_field((void *)ds->ext4_rate_sets_indices_bw, fd->band, fd->bw, ds_id,
			fd->ext4_rates_indices_field_offset,
			(registry_flags2 & CLM_REGISTRY_FLAG2_EXT4) &&
			(registry_flags2 & CLM_REGISTRY_FLAG2_RATE_SET_INDEX),
			(registry_flags & CLM_REGISTRY_FLAG_PER_BAND_RATES) != 0u,
			(registry_flags & CLM_REGISTRY_FLAG_PER_BW_RS) != 0u);
		fill_band_bw_field((void *)ds->he_rate_sets_indices_bw, fd->band, fd->bw, ds_id,
			fd->he_rates_indices_field_offset,
			(registry_flags2 & CLM_REGISTRY_FLAG2_HE_LIMITS) &&
			(registry_flags2 & CLM_REGISTRY_FLAG2_RATE_SET_INDEX),
			(registry_flags2 & CLM_REGISTRY_FLAG2_RU_SETS_PER_BW_BAND) != 0u,
			FALSE);
		fill_band_bw_field((void *)ds->ext_ru_rate_sets_bw, fd->band, fd->bw, ds_id,
			fd->ext_ru_rates_field_offset,
			(registry_flags2 & CLM_REGISTRY_FLAG2_EXT_RU) && (fd->bw >= CLM_BW_160),
			TRUE, FALSE);
		fill_band_bw_field((void *)ds->ext_ru_rate_sets_indices_bw, fd->band, fd->bw, ds_id,
			fd->ext_ru_rates_indices_field_offset,
			(registry_flags2 & CLM_REGISTRY_FLAG2_EXT_RU) && (fd->bw >= CLM_BW_160),
			TRUE, FALSE);
	}
	if (registry_flags2 & CLM_REGISTRY_FLAG2_HE_LIMITS) {
#if defined(WL_RU_NUMRATES)
		if (!(registry_flags2 & CLM_REGISTRY_FLAG2_HE_RU_FIXED)) {
			/* Old-style HE limits not supported with new bcmwifi_rates.h */
			return CLM_RESULT_ERR;
		}
#elif defined(WL_NUM_HE_RT)
		if ((registry_flags2 & (CLM_REGISTRY_FLAG2_HE_RU_FIXED |
			CLM_REGISTRY_FLAG2_HE_SU_ORDINARY)) || (ds_id != DS_MAIN))
		{
			/* New-style HE-limits and incremental operation
			 * not supported with old bcmwifi_rates.h
			 */
			return CLM_RESULT_ERR;
		}
		ds->he_rate_dscs = (const clm_he_rate_dsc_t *)GET_DATA(ds_id, he_rates);
#else
		/* HE limits not supported with very old style bcmwifi_rates.h */
		return CLM_RESULT_ERR;
#endif /* else WL_NUM_HE_RT */
	}
	return CLM_RESULT_OK;
}

/** Initializes given CLM data source
 * \param[in] header Header of CLM data
 * \param[in] ds_id Identifier of CLM data set
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if data address is
 * zero or if CLM data tag is absent at given address or if major number of CLM
 * data format version is not supported by CLM API
 */
static clm_result_t
clm_data_source_init(const clm_data_header_t *header, data_source_id_t ds_id)
{
	data_dsc_t *ds = get_data_sources(ds_id);
	if (header) {
		bool has_registry_flags = TRUE;
		clm_result_t err;
		if (strncmp(header->header_tag, CLM_HEADER_TAG, sizeof(header->header_tag))) {
			return CLM_RESULT_ERR;
		}
		if ((header->format_major == 5) && (header->format_minor == 1)) {
			has_registry_flags = FALSE;
		} else if (!is_blob_version_supported(header->format_major)) {
			return CLM_RESULT_ERR;
		}
		ds->scr_idx_4 = header->format_major <= 17;
		ds->relocation =
			(uintptr)((const uint8 *)header - (const uint8 *)header->self_pointer);
		ds->data = (const clm_registry_t*)relocate_ptr(ds_id, header->data);
		ds->registry_flags = has_registry_flags ? ds->data->flags : 0u;
		ds->registry_flags2 = (ds->registry_flags & CLM_REGISTRY_FLAG_REGISTRY_FLAGS2)
			? ds->data->flags2 : 0u;
		ds->header = header;

		err = check_data_flags_compatibility(ds_id);
		if (err != CLM_RESULT_OK) {
			return err;
		}

		err = fill_region_record_data_fields(ds_id);
		if (err != CLM_RESULT_OK) {
			return err;
		}

		err = fill_pointer_data_fields(ds_id);
		if (err != CLM_RESULT_OK) {
			return err;
		}

	} else {
		ds->relocation = 0u;
		ds->data = NULL;
	}
	try_fill_valid_channel_combs(ds_id);
	if (ds_id == DS_MAIN) {
		try_fill_valid_channel_combs(DS_INC);
	}
	fill_rate_types();
#ifdef WL_RU_NUMRATES
	fill_ru_rate_info();
#endif /* WL_RU_NUMRATES */
	return CLM_RESULT_OK;
}

/** True if two given CC/Revs are equal
 * \param[in] cc_rev1 First CC/Rev
 * \param[in] cc_rev2 Second CC/Rev
 * \return True if two given CC/Revs are equal
 */
static bool
cc_rev_equal(const clm_cc_rev4_t *cc_rev1, const clm_cc_rev4_t *cc_rev2)
{
	return (cc_rev1->cc[0u] == cc_rev2->cc[0u]) && (cc_rev1->cc[1u] == cc_rev2->cc[1u]) &&
		(cc_rev1->rev == cc_rev2->rev);
}

/** True if two given CCs are equal
 * \param[in] cc1 First CC
 * \param[in] cc2 Second CC
 * \return True if two given CCs are equal
 */
static bool
cc_equal(const char *cc1, const char *cc2)
{
	return (cc1[0u] == cc2[0u]) && (cc1[1u] == cc2[1u]);
}

/** Copies CC
 * \param[out] to Destination CC
 * \param[in] from Source CC
 */
static void
copy_cc(char *to, const char *from)
{
	to[0u] = from[0u];
	to[1u] = from[1u];
}

/** Returns country definition by index
 * \param[in] ds_id Data set to look in
 * \param[in] idx Country definition index
 * \param[out] num_countries Optional output parameter - number of regions
 * \return Country definition address, NULL if data set contains no countries
 * or index is out of range
 */
static const clm_country_rev_definition_cd10_t *
get_country_def_by_idx(data_source_id_t ds_id, uint32 idx, int32 *num_countries)
{
	return (const clm_country_rev_definition_cd10_t *)
		GET_ITEM(ds_id, countries, clm_country_rev_definition_set_t,
		get_data_sources(ds_id)->country_rev_rec_len, idx, num_countries);
}

/** Performs 8->16 regrev remap
 * This function assumes that remap is needed - i.e. CLM data set uses 8 bit
 * regrevs and has CLM_REGISTRY_FLAG_REGREV_REMAP flag set
 * \param[in] ds_id Data set from which CC/rev was retrieved
 * \param[in,out] ccrev CC/rev to remap
 */
static void
remap_regrev(data_source_id_t ds_id, clm_cc_rev4_t *ccrev)
{
	const clm_regrev_cc_remap_set_t *regrev_remap_set = get_data_sources(ds_id)->regrev_remap;
	const clm_regrev_cc_remap_t *cc_remap;
	const clm_regrev_cc_remap_t *cc_remap_end;

	uint32 num_regrevs;
	const clm_regrev_regrev_remap_t *regrev_regrev_remap;
	/* Fail if function called for a blob that does not define remaps */
	ASSERT(regrev_remap_set != NULL);
	cc_remap = (const clm_regrev_cc_remap_t *)relocate_ptr(ds_id, regrev_remap_set->cc_remaps);
	cc_remap_end = cc_remap + regrev_remap_set->num;
	/* Fail on nonempty remapped CCs' set with NULL set pointer (which
	 * means that ClmCompiler has a bug)
	 */
	ASSERT((cc_remap != NULL) || (cc_remap_end == cc_remap));
	for (; cc_remap < cc_remap_end;	++cc_remap)
	{
		if (cc_equal(ccrev->cc, cc_remap->cc)) {
			break;
		}
	}
	if (cc_remap >= cc_remap_end) {
		return;
	}
	/* Fail on empty remap descriptors' set when there is a reference into
	 * it (which means that ClmCompiler has a bug)
	 */
	ASSERT(regrev_remap_set->regrev_remaps != NULL);
	regrev_regrev_remap = (const clm_regrev_regrev_remap_t *)relocate_ptr(ds_id,
		regrev_remap_set->regrev_remaps) + cc_remap->index;
	for (num_regrevs = cc_remap[1u].index - cc_remap->index; num_regrevs--;
		++regrev_regrev_remap)
	{
		if (regrev_regrev_remap->r8 == ccrev->rev) {
			ccrev->rev = (uint16)regrev_regrev_remap->r16l +
				((uint16)regrev_regrev_remap->r16h << 8u);
			break;
		}
	}
}

/** Reads CC/rev from region (country) record
 * \param[in] ds_id Identifier of CLM data set
 * \param[in] country Region record to read from
 * \param[out] result Buffer for result
 */
static void
get_country_ccrev(data_source_id_t ds_id,
	const clm_country_rev_definition_cd10_t *country,
	clm_cc_rev4_t *result)
{
	data_dsc_t *ds = get_data_sources(ds_id);
	copy_cc(result->cc, country->cc_rev.cc);
	result->rev = country->cc_rev.rev;
	if (ds->reg_rev16_idx >= 0) {
		result->rev += ((uint16)country->extra[ds->reg_rev16_idx]) << 8u;
	} else if (result->rev == (CLM_DELETED_MAPPING & 0xFFu)) {
		result->rev = CLM_DELETED_MAPPING;
	} else if (ds->regrev_remap) {
		remap_regrev(ds_id, result);
	}
}

/** Reads CC/rev from given memory address
 * CC/rev in memory may be in form of clm_cc_rev4_t, 8-bit index, 16-bit index
 * \param[in] ds_id Identifier of CLM data set
 * \param[out] result Buffer for result
 * \param[in] raw_ccrev Address of raw CC/rev or vector of CC/revs
 * \param[in] raw_ccrev_idx Index in vector of CC/revs
 */
static void
get_ccrev(data_source_id_t ds_id, clm_cc_rev4_t *result, const void *raw_ccrev,
	uint32 raw_ccrev_idx)
{
	data_dsc_t *ds = get_data_sources(ds_id);
	const clm_cc_rev_t *plain_ccrev = NULL;
	/* Determining storage format */
	switch (ds->ccrev_format) {
	case CCREV_FORMAT_CC_REV:
		/* Stored as plain clm_cc_rev_t */
		plain_ccrev = (const clm_cc_rev_t *)raw_ccrev + raw_ccrev_idx;
		break;
	case CCREV_FORMAT_CC_IDX8:
	case CCREV_FORMAT_CC_IDX16:
		{
			/* Stored as 8-bit or 16-bit index */
			int32 idx = (ds->ccrev_format == CCREV_FORMAT_CC_IDX8)
				? *((const uint8 *)raw_ccrev + raw_ccrev_idx)
				: *((const uint16 *)raw_ccrev + raw_ccrev_idx);
			int32 num_countries;
			const clm_country_rev_definition_cd10_t *country =
				get_country_def_by_idx(ds_id, idx, &num_countries);
			/* Index to region table or to extra_ccrevs? */
			if (country) {
				/* Index to region table */
				get_country_ccrev(ds_id, country, result);
			} else {
				/* Index to extra_ccrev */
				const void *ccrev =
					GET_ITEM(ds_id, extra_ccrevs, clm_cc_rev_set_t,
					(ds->reg_rev16_idx >= 0) ? sizeof(clm_cc_rev4_t)
					: sizeof(clm_cc_rev_t),
					idx - num_countries, NULL);
				/* Fail on empty extra CC/rev despite there is
				 * a reference into it (which means that
				 * ClmCompiler has a bug)
				 */
				ASSERT(ccrev != NULL);
				/* What format extra_ccrev has? */
				if (ds->reg_rev16_idx >= 0) {
					*result = *(const clm_cc_rev4_t *)ccrev;
				} else {
					/* clm_cc_rev_t structures (8-bit
					 * rev)
					 */
					plain_ccrev = (const clm_cc_rev_t *)ccrev;
				}
			}
		}
		break;
	}
	if (plain_ccrev) {
		copy_cc(result->cc, plain_ccrev->cc);
		result->rev = plain_ccrev->rev;
		if (result->rev == (CLM_DELETED_MAPPING & 0xFFu)) {
			result->rev = CLM_DELETED_MAPPING;
		} else if (ds->regrev_remap) {
			remap_regrev(ds_id, result);
		}
	}
}

/** Retrieves aggregate data by aggregate index
 * \param[in] ds_id Identifier of CLM data set
 * \param[in] idx Aggregate index
 * \param[out] result Buffer for result
 * \param[out] num_aggregates Optional output parameter - number of aggregates
    in data set
 * \return TRUE if index is in valid range
 */
static bool
get_aggregate_by_idx(data_source_id_t ds_id, uint32 idx, aggregate_data_t *result,
	int32 *num_aggregates)
{
	bool maps_indices = get_data_sources(ds_id)->ccrev_format != CCREV_FORMAT_CC_REV;
	const void *p =
		GET_ITEM(ds_id, aggregates, clm_aggregate_cc_set_t,
		maps_indices ? sizeof(clm_aggregate_cc16_t) : sizeof(clm_aggregate_cc_t),
		idx, num_aggregates);
	if (!p) {
		return FALSE;
	}
	if (maps_indices) {
		result->def_reg = ((const clm_aggregate_cc16_t *)p)->def_reg;
		result->num_regions = ((const clm_aggregate_cc16_t *)p)->num_regions;
		result->regions = relocate_ptr(ds_id, ((const clm_aggregate_cc16_t *)p)->regions);
	} else {
		get_ccrev(ds_id, &result->def_reg,
			&((const clm_aggregate_cc_t *)p)->def_reg, 0u);
		result->num_regions = ((const clm_aggregate_cc_t *)p)->num_regions;
		result->regions = relocate_ptr(ds_id, ((const clm_aggregate_cc_t *)p)->regions);
	}
	return TRUE;
}

/** Looks for given aggregation in given data set
 * \param[in] ds_id Identifier of CLM data set
 * \param[in] cc_rev Aggregation's default region CC/rev
 * \param[out] idx Optional output parameter - index of aggregation in set
 * \param[out] result Output parameter - buffer for aggregate data
 * \return TRUE if found
 */
static bool
get_aggregate(data_source_id_t ds_id, const clm_cc_rev4_t *cc_rev, uint32 *idx,
	aggregate_data_t *result)
{
	uint32 i;
	/* Making copy because *cc_rev may be part of *result */
	clm_cc_rev4_t target = *cc_rev;
	for (i = 0u; get_aggregate_by_idx(ds_id, i, result, NULL); ++i) {
		if (cc_rev_equal(&result->def_reg, &target)) {
			if (idx) {
				*idx = i;
			}
			return TRUE;
		}
	}
	return FALSE;
}

/** Looks for mapping with given CC in given aggregation
 * \param[in] ds_id Identifier of CLM data set aggregation belongs to
 * \param[in] agg Aggregation to look in
 * \param[in] cc CC to look for
 * \param[out] result Optional buffer for resulted mapping
 * \return TRUE if found
 */
static bool
get_mapping(data_source_id_t ds_id, const aggregate_data_t *agg,
	const ccode_t cc, clm_cc_rev4_t *result)
{
	const uint8 *mappings =	agg ? (const uint8 *)agg->regions : NULL;
	clm_cc_rev4_t ccrev_buf;
	int32 i;
	if (!mappings) {
		return FALSE;
	}
	if (!result) {
		result = &ccrev_buf;
	}
	for (i = 0; i < agg->num_regions; ++i) {
		get_ccrev(ds_id, result, mappings, (uint32)i);
		if (cc_equal(cc, result->cc)) {
			return TRUE;
		}
	}
	return FALSE;
}

/** Reads locale index from region record
 * \param[in] ds_id Identifier of CLM data set region record belongs to
 * \param[in] country_definition Region definition record
 * \param[in] loc_type Locale type
 * \return Locale index or one of CLM_LOC_... special indices
 */
static uint32
loc_idx(data_source_id_t ds_id,
	const clm_country_rev_definition_cd10_t *country_definition,
	uint32 loc_type)
{
	uint32 ret;
	data_dsc_t *ds = get_data_sources(ds_id);
#ifdef WL_BAND6G
	if ((loc_type_dscs[loc_type].band == CLM_BAND_6G) &&
		(!(ds->registry_flags2 & CLM_REGISTRY_FLAG2_6GHZ)))
	{
		/* Locale index byte (ret initial value) might be outside the
		 * structure. Avoiding reading it
		 */
		return CLM_LOC_NONE;
	}
#endif /* WL_BAND6G */
	/* Sanity check avoiding way to do 'ret = country_definition->locales[loc_type];' */
	ret = ((const uint8 *)country_definition)
		[OFFSETOF(clm_country_rev_definition_cd10_t, locales) + loc_type];
#ifdef WL_BAND6G
	if (loc_type_dscs[loc_type].band == CLM_BAND_6G) {
		if (ds->reg_loc12_6g_idx >= 0) {
			ret |= ((uint32)country_definition->extra[ds->reg_loc12_6g_idx] <<
				((CLM_LOC_IDX_NUM - loc_type + 2u) * 4u)) & 0xF00u;
			if (ds->reg_loc16_6g_idx >= 0) {
				ret |= ((uint32)country_definition->extra[ds->reg_loc16_6g_idx] <<
					((CLM_LOC_IDX_NUM - loc_type + 2u) * 4u + 4u)) & 0xF000u;
			}
		}
		ret &= ds->loc_idx_mask;
	} else
#endif /* WL_BAND6G */
	{
		if (ds->reg_loc10_idx >= 0) {
			ret |= ((uint32)country_definition->extra[ds->reg_loc10_idx] <<
				((CLM_LOC_IDX_NUM - loc_type)*2u)) & 0x300u;
			if (ds->reg_loc12_idx >= 0) {
				ret |= ((uint32)country_definition->extra[ds->reg_loc12_idx] <<
					((CLM_LOC_IDX_NUM - loc_type) * 2u + 2u)) & 0xC00u;
				if (ds->reg_loc14_idx >= 0) {
					ret |= ((uint32)country_definition->extra[ds->reg_loc14_idx]
						<< ((CLM_LOC_IDX_NUM - loc_type) * 2u + 4u)) &
						0x3000u;
				}
			}
		}
	}
	if (ret == (CLM_LOC_NONE & ds->loc_idx_mask)) {
		ret = CLM_LOC_NONE;
	} else if (ret == (CLM_LOC_SAME & ds->loc_idx_mask)) {
		ret = CLM_LOC_SAME;
	} else if (ret == (CLM_LOC_DELETED & ds->loc_idx_mask)) {
		ret = CLM_LOC_DELETED;
	}
	return ret;
}

/** True if given country definition marked as deleted
 * \param[in] ds_id Identifier of CLM data set country definition belongs to
 * \param[in] country_definition Country definition structure
 * \return True if given country definition marked as deleted
 */
static bool
country_deleted(data_source_id_t ds_id,
	const clm_country_rev_definition_cd10_t *country_definition)
{
	return loc_idx(ds_id, country_definition, 0u) == CLM_LOC_DELETED;
}

/** Looks up for definition of given country (region) in given CLM data set
 * \param[in] ds_id Data set id to look in
 * \param[in] cc_rev Region CC/rev to look for
 * \param[out] idx Optional output parameter: index of found country definition
 * \return Address of country definition or NULL
 */
static const clm_country_rev_definition_cd10_t *
get_country_def(data_source_id_t ds_id, const clm_cc_rev4_t *cc_rev, uint32 *idx)
{
	uint32 i;
	const clm_country_rev_definition_cd10_t *ret;
	for (i = 0u; (ret = get_country_def_by_idx(ds_id, i, NULL)) != NULL; ++i) {
		clm_cc_rev4_t region_ccrev;
		get_country_ccrev(ds_id, ret, &region_ccrev);
		if (cc_rev_equal(&region_ccrev, cc_rev)) {
			if (idx) {
				*idx = i;
			}
			return ret;
		}
	}
	return NULL;
}

/** Finds subchannel rule for given main channel and fills channel table for it
 * \param[out] actual_table Table to fill. Includes channel numbers only for
 * bandwidths included in subchannel rule
 * \param[out] power_inc Power increment to apply
 * \param[in] full_table Full subchannel table to take channel numbers from
 * \param[in] limits_type Limits type (subchannel ID)
 * \param[in] channel Main channel
 * \param[in] ranges Channel ranges' table
 * \param[in] comb_set Comb set for main channel's bandwidth
 * \param[in] main_rules Array of main channel subchannel rules (each rule
 * pertinent to range of main channels)
 * \param[in] num_main_rules Number of main channel subchannel rules
 * \param[in] num_subchannels Number of subchannels in rule
 * (CLM_DATA_SUB_CHAN_MAX_... constant)
 * \param[in] registry_flags Registry flags for data set that contains
 * subchannel rules
 */
static void
fill_actual_subchan_table(uint8 actual_table[CLM_BW_NUM], clm_power_t *power_inc,
	uint8 full_table[CLM_BW_NUM], uint32 limits_type, uint32 channel,
	const clm_channel_range_t *ranges, const clm_channel_comb_set_t *comb_set,
	const clm_sub_chan_region_rules_t *region_rules, const int8 *increments,
	uint32 num_subchannels, uint32 registry_flags)
{
	/* Rule pointer as character pointer (to simplify address arithmetic)
	 */
	const uint8 *r;
	uint32 bw_data_len = (registry_flags & CLM_REGISTRY_FLAG_SUBCHAN_RULES_INC)
		? sizeof(clm_sub_chan_rule_inc_t) : sizeof(uint8);
	uint32 chan_rule_len = CLM_SUB_CHAN_RULES_IDX + (bw_data_len * num_subchannels);
	uint32 rule_index = 0u;

	/* Fail nonempty subchannel rules when there are no channel range
	 * descriptors for main channel bandwidth (which means that ClmCompiler
	 * has a bug)
	 */
	ASSERT((region_rules->num == 0u) || (ranges != NULL));

	/* Loop over subchannel rules */
	for (r = (const uint8 *)region_rules->channel_rules; rule_index < region_rules->num;
		r += chan_rule_len, ++rule_index)
	{
		/* Did we find rule for range that contains given main
		 * channel?
		 */
		if (channel_in_range(channel, ranges + r[CLM_SUB_CHAN_RANGE_IDX], comb_set, 0u)) {
			/* Rule found - now let's fill the table */

			/* Loop index, actual type is clm_bandwidth_t */
			uint32 bw_idx;
			/* Subchannel rule (cell in 'Rules' page) */
			const clm_sub_chan_rule_inc_t *sub_chan_rule =
				(const clm_sub_chan_rule_inc_t *)
				(r + CLM_SUB_CHAN_RULES_IDX +
				(limits_type - 1u) * bw_data_len);
			/* Probing all possible bandwidths */
			for (bw_idx = 0u; bw_idx < CLM_BW_NUM; ++bw_idx) {
				/* If bandwidth included to rule */
				if (get_bandwidth_traits(bw_idx)->sc_mask & sub_chan_rule->bw) {
					/* Copy channel number for this
					 * bandwidth from full table
					 */
					actual_table[bw_idx] = full_table[bw_idx];
				}
			}
			*power_inc = (registry_flags & CLM_REGISTRY_FLAG_SUBCHAN_RULES_INC) ?
				sub_chan_rule->inc :
				(increments ? increments[rule_index * num_subchannels
				+ (limits_type - 1u)] : 0);
			return; /* All done, no need to look for more rules */
		}
	}
}

/** Translates given flags according to given flag translation descriptor set
 * \param[in] flags Flags to translate
 * \param[in] flag_set_id ID of flag translation set
 * \return Logical OR if 'to' flags, corespondent to 'from' flags set in 'flags'
 */
static uint32
BCMRAMFN(translate_flags)(uint32 flags, flag_translation_set_id_t flag_set_id)
{
	uint32 ret = 0u;
	const flag_translation_set_t *ftr_set = flag_translation_sets + flag_set_id;
	const flag_translation_t *p, *end;
	ASSERT(((uint32)flag_set_id < (uint32)FTS_NUM) &&
		((uint32)ARRAYSIZE(flag_translation_sets) == (uint32)FTS_NUM));
	p = flag_translation_sets[flag_set_id].set;
	end = p + flag_translation_sets[flag_set_id].num;
	for (p = ftr_set->set, end = p + ftr_set->num; p < end; ++p) {
		if (flags & p->from) {
			ret |= p->to;
		}
	}
	return ret;
}

/** Translates device category and regulatory destination fields of second flag
 * byte of regulatory/TX record
 * \param[in] flags2 Second flags byte
 * \param[out] dev_cat Optional output parameter: device category
 * \param[out] reg_dest Optional output parameter: regulatory destination
 * \return TRUE if field values were found valid
 */
static bool
get_dev_cat_reg_dest(uint8 flags2, clm_device_category_t *dev_cat,
	clm_regulatory_limit_dest_t *reg_dest)
{
	if (dev_cat) {
		switch (flags2 & CLM_DATA_FLAG2_DEVICE_CATEGORY_MASK) {
		case CLM_DATA_FLAG2_DEVICE_CATEGORY_LP:
			*dev_cat = CLM_DEVICE_CATEGORY_LP;
			break;
		case CLM_DATA_FLAG2_DEVICE_CATEGORY_VLP:
			*dev_cat = CLM_DEVICE_CATEGORY_VLP;
			break;
		case CLM_DATA_FLAG2_DEVICE_CATEGORY_SP:
			*dev_cat = CLM_DEVICE_CATEGORY_SP;
			break;
		default:
			return FALSE;
		}
	}
	if (reg_dest) {
		switch (flags2 & CLM_DATA_FLAG2_REGULATORY_LIMIT_DEST_MASK) {
		case CLM_DATA_FLAG2_REGULATORY_LIMIT_DEST_CLIENT:
			*reg_dest = CLM_REGULATORY_LIMIT_DEST_CLIENT;
			break;
		case CLM_DATA_FLAG2_REGULATORY_LIMIT_DEST_LOCAL:
			*reg_dest = CLM_REGULATORY_LIMIT_DEST_LOCAL;
			break;
		case CLM_DATA_FLAG2_REGULATORY_LIMIT_DEST_SUBORDINATE:
			*reg_dest = CLM_REGULATORY_LIMIT_DEST_SUBORDINATE;
			break;
		default:
			return FALSE;
		}
	}
	return TRUE;
}

clm_result_t
clm_init(const struct clm_data_header *header)
{
	return clm_data_source_init(header, DS_MAIN);
}

clm_result_t
clm_set_inc_data(const struct clm_data_header *header)
{
	return clm_data_source_init(header, DS_INC);
}

clm_result_t
clm_iter_init(int32 *iter)
{
	if (iter) {
		*iter = CLM_ITER_NULL;
		return CLM_RESULT_OK;
	}
	return CLM_RESULT_ERR;
}

clm_result_t
clm_limits_params_init(clm_limits_params_t *params)
{
	if (!params) {
		return CLM_RESULT_ERR;
	}
	params->bw = CLM_BW_20;
	params->antenna_idx = 0;
	params->sar = 0x7F;
	params->other_80_80_chan = 0u;
	params->device_category = CLM_DEVICE_CATEGORY_LEGACY;
	params->power_shift = 0;
	params->punctured_subchan_bw = CLM_BW_NUM;
	return CLM_RESULT_OK;
}

clm_result_t
clm_country_iter(clm_country_t *country, ccode_t cc, uint32 *rev)
{
	data_source_id_t ds_id;
	uint32 idx;
	clm_result_t ret = CLM_RESULT_OK;
	if (!country || !cc || !rev) {
		return CLM_RESULT_ERR;
	}
	if (*country == CLM_ITER_NULL) {
		ds_id = DS_INC;
		idx = 0u;
	} else {
		iter_unpack(*country, &ds_id, &idx);
		++idx;
	}
	for (;;) {
		int32 num_countries;
		const clm_country_rev_definition_cd10_t *country_definition =
			get_country_def_by_idx(ds_id, idx, &num_countries);
		clm_cc_rev4_t country_ccrev;
		if (!country_definition) {
			if (ds_id == DS_INC) {
				ds_id = DS_MAIN;
				idx = 0u;
				continue;
			} else {
				ret = CLM_RESULT_NOT_FOUND;
				idx = (num_countries >= 0) ? (uint32)num_countries : 0u;
				break;
			}
		}
		get_country_ccrev(ds_id, country_definition, &country_ccrev);
		if (country_deleted(ds_id, country_definition)) {
			++idx;
			continue;
		}
		if ((ds_id == DS_MAIN) && get_data_sources(DS_INC)->data) {
			int32 i, num_inc_countries;
			const clm_country_rev_definition_cd10_t *inc_country_definition;
			for (i = 0;
				(inc_country_definition =
				get_country_def_by_idx(DS_INC, i, &num_inc_countries)) != NULL;
				++i)
			{
				clm_cc_rev4_t inc_country_ccrev;
				get_country_ccrev(DS_INC, inc_country_definition,
					&inc_country_ccrev);
				if (cc_rev_equal(&country_ccrev, &inc_country_ccrev)) {
					break;
				}
			}
			if (i < num_inc_countries) {
				++idx;
				continue;
			}
		}
		copy_cc(cc, country_ccrev.cc);
		*rev = country_ccrev.rev;
		break;
	}
	iter_pack(country, ds_id, idx);
	return ret;
}

clm_result_t
clm_country_lookup(const ccode_t cc, uint32 rev, clm_country_t *country)
{
	uint32 ds_idx;
	clm_cc_rev4_t cc_rev;
	if (!cc || !country) {
		return CLM_RESULT_ERR;
	}
	copy_cc(cc_rev.cc, cc);
	cc_rev.rev = (regrev_t)rev;
	for (ds_idx = 0u; ds_idx < DS_NUM; ++ds_idx) {
		uint32 idx;
		const clm_country_rev_definition_cd10_t *country_definition =
			get_country_def((data_source_id_t)ds_idx, &cc_rev, &idx);
		if (!country_definition) {
			continue;
		}
		if (country_deleted((data_source_id_t)ds_idx, country_definition)) {
			return CLM_RESULT_NOT_FOUND;
		}
		iter_pack(country, (data_source_id_t)ds_idx, idx);
		return CLM_RESULT_OK;
	}
	return CLM_RESULT_NOT_FOUND;
}

clm_result_t
clm_country_def(const clm_country_t country, clm_country_locales_t *locales)
{
	data_source_id_t ds_id;
	uint32 idx;
	uint32 loc_type;
	const clm_country_rev_definition_cd10_t *country_definition;
	const clm_country_rev_definition_cd10_t *main_country_definition = NULL;
	int32 flags_idx;
	data_dsc_t* ds;
	if (!locales) {
		return CLM_RESULT_ERR;
	}
	iter_unpack(country, &ds_id, &idx);
	country_definition = get_country_def_by_idx(ds_id, idx, NULL);
	if (!country_definition) {
		return CLM_RESULT_NOT_FOUND;
	}
	locales->main_loc_data_bitmask = 0u;
	for (loc_type = 0u; loc_type < ARRAYSIZE(loc_type_dscs); ++loc_type) {
		data_source_id_t locale_ds_id = ds_id;
		const uint8 *loc_def;
		uint32 locale_idx = loc_idx(locale_ds_id, country_definition, loc_type);
		if (locale_idx == CLM_LOC_SAME) {
			if (!main_country_definition) {
				clm_cc_rev4_t country_ccrev;
				get_country_ccrev(ds_id, country_definition, &country_ccrev);
				main_country_definition =
					get_country_def(DS_MAIN, &country_ccrev, NULL);
			}
			locale_ds_id = DS_MAIN;
			locale_idx = main_country_definition
				? loc_idx(locale_ds_id, main_country_definition, loc_type)
				: CLM_LOC_NONE;
		}
		if (locale_idx == CLM_LOC_NONE) {
			loc_def = NULL;
		} else {
			bool is_base = loc_type_dscs[loc_type].flavor == BH_BASE;
			loc_def = (const uint8 *)get_data(locale_ds_id,
				loc_type_dscs[loc_type].loc_def_field_offset);
			while (locale_idx--) {
				uint32 tx_rec_len;
				if (is_base) {
					loc_def = skip_base_header(loc_def,
						get_data_sources(locale_ds_id), NULL, NULL);
				}
				for (;;) {
					uint8 flags = *loc_def++;
					if (flags & CLM_DATA_FLAG_FLAG2) {
						uint8 flags2 = *loc_def++;
						if (flags2 & CLM_DATA_FLAG2_FLAG3) {
							++loc_def;
						}
					}
					tx_rec_len = CLM_LOC_DSC_TX_REC_LEN +
						((flags &
						CLM_DATA_FLAG_PER_ANT_MASK) >>
						CLM_DATA_FLAG_PER_ANT_SHIFT);
					if (!(flags & CLM_DATA_FLAG_MORE)) {
						break;
					}
					loc_def += 1u + tx_rec_len * (uint32)(*loc_def);
				}
				loc_def += 1u + tx_rec_len * (uint32)(*loc_def);
			}
		}
		*(const uint8 **)
			((uint8 *)locales + loc_type_dscs[loc_type].def_field_offset) = loc_def;
		if (locale_ds_id == DS_MAIN) {
			locales->main_loc_data_bitmask |= (1u << loc_type);
		}
	}
	ds = get_data_sources(ds_id);
	flags_idx = ds->reg_flags_idx;
	if (ds->registry_flags2 & CLM_REGISTRY_FLAG2_INDEXED_COUNTRY_FLAGS) {
		const clm_region_flags_t* flag_set =
			(const clm_region_flags_t*)GET_DATA(ds_id, region_flags);
		const uint8* flags = (const uint8*)relocate_ptr(ds_id, flag_set->flags) +
			(country_definition->extra[flags_idx] * flag_set->reg_flag_len);
		locales->country_flags = flags[0];
		locales->country_flags_2 = (flag_set->reg_flag_len > 1) ? flags[1] : 0;
		locales->country_flags_3 = (flag_set->reg_flag_len > 2) ? flags[2] : 0;
	} else {
		locales->country_flags =
			(flags_idx >= 0) ? country_definition->extra[flags_idx] : 0u;
		flags_idx = ds->reg_flags_2_idx;
		locales->country_flags_2 =
			(flags_idx >= 0) ? country_definition->extra[flags_idx] : 0u;
		locales->country_flags_3 = 0;
	}
	locales->computed_flags = (uint8)ds_id;
	return CLM_RESULT_OK;
}

clm_result_t
clm_country_channels(const clm_country_locales_t *locales, clm_band_t band,
	clm_channels_t *valid_channels, clm_channels_t *restricted_channels)
{
	clm_channels_t dummy_valid_channels;
	locale_data_t loc_data;

	if (!locales || ((uint32)band >= (uint32)CLM_BAND_NUM)) {
		return CLM_RESULT_ERR;
	}
	if (!restricted_channels && !valid_channels) {
		return CLM_RESULT_OK;
	}
	if (!valid_channels) {
		valid_channels = &dummy_valid_channels;
	}
	if (!get_loc_def(locales, compose_loc_type[band][BH_BASE], &loc_data)) {
		return CLM_RESULT_ERR;
	}
	if (loc_data.base_hdr) {
		get_channels(valid_channels, loc_data.valid_channels,
			loc_data.base_hdr[CLM_LOC_DSC_CHANNELS_IDX],
			loc_data.chan_ranges_bw[CLM_BW_20],
			loc_data.combs[CLM_BW_20]);
		get_channels(restricted_channels, loc_data.restricted_channels,
			loc_data.base_hdr[CLM_LOC_DSC_RESTRICTED_IDX],
			loc_data.chan_ranges_bw[CLM_BW_20],
			loc_data.combs[CLM_BW_20]);
		if (restricted_channels) {
			uint32 i;
			for (i = 0u; i < ARRAYSIZE(restricted_channels->bitvec); ++i) {
				restricted_channels->bitvec[i] &= valid_channels->bitvec[i];
			}
		}
	} else {
		get_channels(valid_channels, NULL, 0u, NULL, NULL);
		get_channels(restricted_channels, NULL, 0u, NULL, NULL);
	}
	return CLM_RESULT_OK;
}

/** Retrieve part of country information, contained in regulatory part of its
 * base locale
 * \param[in] base_loc_data Base locale data
 * \param[in] band Base locale band
 * \param[out] info Country information being filled
 * \return TRUE if no problem with data, FALSE if there is some problem
 *
 */
static bool
regulatory_country_information(const locale_data_t *base_loc_data, clm_band_t band,
	clm_country_info_t *info)
{
	uint8 base_flags;
	if (!base_loc_data->base_hdr) {
		return TRUE;
	}
	base_flags = base_loc_data->base_hdr[CLM_LOC_DSC_FLAGS_IDX];
	switch (band) {
#ifdef WL_BAND6G
	case CLM_BAND_6G:
		info->c2c = c2c_translation[base_flags & CLM_DATA_FLAG_C2C_MASK];
		if (base_flags & CLM_DATA_FLAG_CBP_FCC) {
			info->flags |= CLM_FLAG_CBP_FCC;
		}
		break;
#endif /* WL_BAND6G */
	case CLM_BAND_5G:
		switch (base_flags & CLM_DATA_FLAG_DFS_MASK) {
		case CLM_DATA_FLAG_DFS_NONE:
			info->flags |= CLM_FLAG_DFS_NONE;
			break;
		case CLM_DATA_FLAG_DFS_EU:
			info->flags |= CLM_FLAG_DFS_EU;
			break;
		case CLM_DATA_FLAG_DFS_US:
			info->flags |= CLM_FLAG_DFS_US;
			break;
		case CLM_DATA_FLAG_DFS_TW:
			info->flags |= CLM_FLAG_DFS_TW;
			break;
		case CLM_DATA_FLAG_DFS_UK:
			info->flags |= CLM_FLAG_DFS_UK;
			break;
		case CLM_DATA_FLAG_DFS_JP:
			info->flags |= CLM_FLAG_DFS_JP;
			break;
		}
		break;
	default:
		break;
	}
	if (base_loc_data->reg_limits) {
		info->reg_limit_types |= 1u << CLM_REGULATORY_LIMIT_TYPE_CHANNEL;
	}
	if (base_loc_data->psd_limits) {
		info->reg_limit_types |= 1u << CLM_REGULATORY_LIMIT_TYPE_PSD;
	}
	if (base_loc_data->ds->registry_flags2 & CLM_REGISTRY_FLAG2_DEVICE_CATEGORIES) {
		uint32 rdt;
		const uint8 *reg_limits[CLM_REGULATORY_LIMIT_TYPE_NUM];
		/* Nonconstant array initializer is nonstandard extension :( */
		reg_limits[0u] = base_loc_data->reg_limits;
		reg_limits[1u] = base_loc_data->psd_limits;
		for (rdt = 0u; rdt < ARRAYSIZE(reg_limits); ++rdt) {
			uint8 flags;
			const uint8 *p = reg_limits[rdt];
			if (!p) {
				continue;
			}
			do {
				clm_device_category_t dev_cat = CLM_DEVICE_CATEGORY_LEGACY;
				clm_regulatory_limit_dest_t reg_dest =
					CLM_REGULATORY_LIMIT_DEST_LEGACY;
				flags = *p++;
				if (flags & CLM_DATA_FLAG_FLAG2) {
					uint8 flags2 = *p++;
					if (flags2 & CLM_DATA_FLAG2_FLAG3) {
						++p;
					}
					if (!get_dev_cat_reg_dest(flags2, &dev_cat, &reg_dest)) {
						return FALSE;
					}
				}
				info->device_categories |= 1u << dev_cat;
				info->reg_limit_destinations |= 1u << reg_dest;
				p += 1u + CLM_LOC_DSC_REG_REC_LEN * (uint32)(*p);
			} while (flags & CLM_DATA_FLAG_MORE);
		}
	} else {
		if (base_loc_data->reg_limits || base_loc_data->psd_limits) {
			info->device_categories |= 1u << CLM_DEVICE_CATEGORY_LEGACY;
			info->reg_limit_destinations |= 1u << CLM_REGULATORY_LIMIT_DEST_CLIENT;
		}
	}
	return TRUE;
}

static void get_rate_encoding_traits(clm_rate_code_type_t rct, uint32 bw_idx,
	const locale_data_t *loc_data, uint32 *base_rate, const uint8 **rate_sets,
	const uint16 **rate_sets_index)
{
	switch (rct) {
	case RCT_MAIN:
		*base_rate = 0u;
		*rate_sets = loc_data->rate_sets_bw[bw_idx];
		*rate_sets_index = loc_data->rate_sets_indices_bw[bw_idx];
		break;
	case RCT_EXT:
		*base_rate = BASE_EXT_RATE;
		*rate_sets = loc_data->ext_rate_sets_bw[bw_idx];
		*rate_sets_index = loc_data->ext_rate_sets_indices_bw[bw_idx];
		break;
	case RCT_EXT4:
#if defined(CLM_EXT4_RATES_SUPPORTED)
		*base_rate = BASE_EXT4_RATE;
		*rate_sets = loc_data->ext4_rate_sets_bw[bw_idx];
		*rate_sets_index = loc_data->ext4_rate_sets_indices_bw[bw_idx];
#else /* CLM_EXT4_RATES_SUPPORTED */
		ASSERT(0);
#endif /* else CLM_EXT4_RATES_SUPPORTED */
		break;
	case RCT_HE:
		*base_rate = 0u;
		*rate_sets = loc_data->he_rate_sets_bw[bw_idx];
		*rate_sets_index = loc_data->he_rate_sets_indices_bw[bw_idx];
		break;
	case RCT_EXT_RU: /* Extended RCT code */
#if defined(CLM_MRU_RATES_SUPPORTED)
		*base_rate = BASE_EXT_RU_RATE;
		*rate_sets = loc_data->ext_ru_rate_sets_bw[bw_idx];
		*rate_sets_index = loc_data->ext_ru_rate_sets_indices_bw[bw_idx];
#else /* CLM_MRU_RATES_SUPPORTED */
		ASSERT(0);
#endif /* else CLM_MRU_RATES_SUPPORTED */
		break;
	default:
		ASSERT(0);
		break;
	}
}

/** Retrieve part of country information, contained in TX data of its locales
 * \param[in] locales Region locales
 * \param[in] band Band to retrieve information for
 * \param[out] info Country information being filled
 * \return TRUE if no problem with data, FALSE if there is some problem
 *
 */
static bool
tx_country_information(const clm_country_locales_t *locales, clm_band_t band,
	clm_country_info_t *info)
{
	uint32 base_ht_idx;
	for (base_ht_idx = 0u; base_ht_idx < BH_NUM; ++base_ht_idx) {
		uint8 flags, flags2, flags3;
		locale_data_t loc_data;
		const uint8 *tx_rec;

		if (!get_loc_def(locales, compose_loc_type[band][base_ht_idx], &loc_data)) {
			return FALSE;
		}
		tx_rec = loc_data.tx_limits;

		if (!tx_rec) {
			continue;
		}
		do {
			uint32 num_rec, tx_rec_len;
			bool eirp;
			uint8 bw_idx;
			uint32 bw_flag_mask = 0u;
			const uint8 *rate_sets = NULL;
			const uint16 *rate_sets_index = NULL;
			uint32 base_rate = 0u;
			clm_device_category_t dev_cat = CLM_DEVICE_CATEGORY_LEGACY;
			clm_rate_code_type_t rct;

			flags = *tx_rec++;
			flags2 = (flags & CLM_DATA_FLAG_FLAG2) ? *tx_rec++ : 0u;
			flags3 = (flags2 & CLM_DATA_FLAG2_FLAG3) ? *tx_rec++ : 0u;
			tx_rec_len = CLM_LOC_DSC_TX_REC_LEN + ((flags &
				CLM_DATA_FLAG_PER_ANT_MASK) >> CLM_DATA_FLAG_PER_ANT_SHIFT);
			num_rec = *tx_rec++;
			if (!num_rec) {
				continue;
			}
			bw_idx = get_tx_rec_bw(flags);
			if ((bw_idx == CLM_BW_NUM) ||
				(flags2 &
				(CLM_DATA_FLAG2_WIDTH_EXT | CLM_DATA_FLAG2_OUTER_BW_MASK)))
			{
				tx_rec += tx_rec_len * num_rec;
				continue;
			}
			if (bw_idx == CLM_BW_40) {
				bw_flag_mask = CLM_FLAG_NO_40MHZ;
			} else if (bw_idx == CLM_BW_80) {
				bw_flag_mask = CLM_FLAG_NO_80MHZ;
			} else if (bw_idx == CLM_BW_80_80) {
				bw_flag_mask = CLM_FLAG_NO_80_80MHZ;
			} else if (bw_idx == CLM_BW_160) {
				bw_flag_mask = CLM_FLAG_NO_160MHZ;
#ifdef CLM_320_MHZ_SUPPORTED
			} else if (bw_idx == CLM_BW_320) {
				bw_flag_mask = CLM_FLAG_NO_320MHZ;
#endif /* CLM_320_MHZ_SUPPORTED */
			}
			if (tx_rec_len != CLM_LOC_DSC_TX_REC_LEN) {
				info->flags |= CLM_FLAG_PER_ANTENNA;
			}
			eirp = (flags & CLM_DATA_FLAG_MEAS_MASK) == CLM_DATA_FLAG_MEAS_EIRP;
			rct = get_rate_code_type(flags2, flags3);
			if (rct >= RCT_MIN_RU) {
				info->flags |= CLM_FLAG_HE;
				tx_rec += num_rec * tx_rec_len;
				continue;
			}
			get_rate_encoding_traits(rct, bw_idx, &loc_data, &base_rate, &rate_sets,
				&rate_sets_index);
			if (!get_dev_cat_reg_dest(flags2, &dev_cat, NULL)) {
				return FALSE;
			}
			info->device_categories |= 1u << dev_cat;
			/* Fail on absent rate set table for a bandwidth when
			 * TX limit table is nonempty (which means that
			 * ClmCompiler has a bug)
			 */
			ASSERT((rate_sets != NULL) || (num_rec == 0u));
			for (; num_rec--; tx_rec += tx_rec_len) {
				const uint8 *rates =
					get_byte_string(rate_sets, tx_rec[CLM_LOC_DSC_RATE_IDX],
					rate_sets_index);
				uint32 num_rates = *rates++;
				/* Check for a non-disabled power before
				 * clearing NO_bw flag
				 */
				if ((uint8)CLM_DISABLED_POWER ==
					(uint8)tx_rec[CLM_LOC_DSC_POWER_IDX])
				{
					continue;
				}
				if (bw_flag_mask) {
					info->flags &= ~bw_flag_mask;
					/* clearing once should be enough */
					bw_flag_mask = 0u;
				}
				while (num_rates--) {
					uint32 rate_idx = base_rate + (*rates++);
					switch (get_rate_type(rate_idx)) {
					case RT_DSSS:
						if (eirp) {
							info->flags |= CLM_FLAG_HAS_DSSS_EIRP;
						}
						break;
					case RT_OFDM:
						if (eirp) {
							info->flags |= CLM_FLAG_HAS_OFDM_EIRP;
						}
						break;
					case RT_MCS:
						info->flags &= ~CLM_FLAG_NO_MIMO;
						break;
					case RT_SU:
						info->flags &= ~CLM_FLAG_NO_MIMO;
						info->flags |= CLM_FLAG_HE;
						break;
#ifdef CLM_EHT_RATES_SUPPORTED
					case RT_EHT:
						info->flags |= CLM_FLAG_EHT;
						break;
#endif /* CLM_EHT_RATES_SUPPORTED */
					default:
						ASSERT(FALSE);
						break;
					}
				}
			}
		} while (flags & CLM_DATA_FLAG_MORE);
	}
	return TRUE;
}

clm_result_t
clm_country_information(const clm_country_locales_t *locales, clm_band_t band,
	clm_country_info_t *info)
{
	locale_data_t base_loc_data;
	data_dsc_t *ds;
	if (!locales || !info || ((uint32)band >= (uint32)CLM_BAND_NUM)) {
		return CLM_RESULT_ERR;
	}
	bzero(info, sizeof(*info));
	info->flags = (uint32)(CLM_FLAG_DFS_NONE | CLM_FLAG_NO_40MHZ | CLM_FLAG_NO_80MHZ |
		CLM_FLAG_NO_80_80MHZ | CLM_FLAG_NO_160MHZ | CLM_FLAG_NO_MIMO |
		CLM_FLAG_NO_240MHZ | CLM_FLAG_NO_320MHZ | CLM_FLAG_NO_160_160MHZ);
	ds = get_data_sources((data_source_id_t)(locales->computed_flags & COUNTRY_FLAGS_DS_MASK));

	if (!get_loc_def(locales, compose_loc_type[band][BH_BASE], &base_loc_data)) {
		return CLM_RESULT_ERR;
	}
	if (!regulatory_country_information(&base_loc_data, band, info) ||
		!tx_country_information(locales, band, info))
	{
		return CLM_RESULT_ERR;
	}
	info->flags |= translate_flags(base_loc_data.base_hdr
		? base_loc_data.base_hdr[CLM_LOC_DSC_FLAGS_IDX] : 0u, FTS_BASE) |
		translate_flags(locales->country_flags, FTS_COUNTRY_FLAG_1) |
		translate_flags(locales->country_flags_2, FTS_COUNTRY_FLAG_2);
	if ((locales->country_flags & CLM_DATA_FLAG_REG_DSA_2) &&
		(ds->registry_flags2 & CLM_REGISTRY_FLAG2_DSA_2))
	{
		info->flags |= CLM_FLAG_DSA_2;
	}
#ifdef CLM_MRU_RATES_SUPPORTED
	if (locales->country_flags_3 & CLM_DATA_FLAG_3_REG_MRU) {
		info->flags |= CLM_FLAG_MRU;
	}
#endif /* CLM_MRU_RATES_SUPPORTED */
	return CLM_RESULT_OK;
}

clm_result_t
clm_country_flags(const clm_country_locales_t *locales, clm_band_t band, unsigned long *ret_flags)
{
	clm_country_info_t info;
	clm_result_t ret = clm_country_information(locales, band, &info);
	if (ret != CLM_RESULT_OK) {
		return ret;
	}
	if (ret_flags == NULL) {
		return CLM_RESULT_ERR;
	}
	*ret_flags = (unsigned long)info.flags;
	return CLM_RESULT_OK;
}

clm_result_t
clm_country_advertised_cc(const clm_country_t country, ccode_t advertised_cc)
{
	data_source_id_t ds_id;
	uint32 idx, ds_idx;
	const clm_country_rev_definition_cd10_t *country_def;
	clm_cc_rev4_t cc_rev;

	if (!advertised_cc) {
		return CLM_RESULT_ERR;
	}
	iter_unpack(country, &ds_id, &idx);
	country_def = get_country_def_by_idx(ds_id, idx, NULL);
	if (!country_def) {
		return CLM_RESULT_ERR;
	}
	get_country_ccrev(ds_id, country_def, &cc_rev);
	for (ds_idx = 0u; ds_idx < DS_NUM; ++ds_idx) {
		uint32 adv_cc_idx;
		const clm_advertised_cc_t *adv_cc;
		for (adv_cc_idx = 0u;
			(adv_cc = (const clm_advertised_cc_t *)GET_ITEM(ds_idx,
			advertised_ccs, clm_advertised_cc_set_t,
			sizeof(clm_advertised_cc_t), adv_cc_idx, NULL)) != NULL;
			++adv_cc_idx)
		{
			uint32 num_aliases, alias_idx;
			const void *alias = (const void *)relocate_ptr(ds_idx, adv_cc->aliases);
			if (adv_cc->num_aliases == CLM_DELETED_NUM) {
				continue;
			}
			num_aliases = (uint32)adv_cc->num_aliases;
			if ((ds_idx == DS_MAIN) && get_data_sources(DS_INC)->data) {
				uint32 inc_adv_cc_idx;
				const clm_advertised_cc_t *inc_adv_cc;
				for (inc_adv_cc_idx = 0u;
					(inc_adv_cc =
					(const clm_advertised_cc_t *)GET_ITEM(DS_INC,
					advertised_ccs, clm_advertised_cc_set_t,
					sizeof(clm_advertised_cc_t), inc_adv_cc_idx,
					NULL)) != NULL;
					++inc_adv_cc_idx)
				{
					if (cc_equal(adv_cc->cc, inc_adv_cc->cc)) {
						break;
					}
				}
				if (inc_adv_cc) {
					continue;
				}
			}
			for (alias_idx = 0u; alias_idx < num_aliases; ++alias_idx) {
				clm_cc_rev4_t alias_ccrev;
				get_ccrev((data_source_id_t)ds_idx, &alias_ccrev, alias, alias_idx);
				if (cc_rev_equal(&alias_ccrev, &cc_rev)) {
					copy_cc(advertised_cc, adv_cc->cc);
					return CLM_RESULT_OK;
				}
			}
		}
	}
	copy_cc(advertised_cc, cc_rev.cc);
	return CLM_RESULT_OK;
}

#if !defined(WLC_CLMAPI_PRE7) || defined(BCMROMBUILD)

/** Precomputes country (region) related data
 * \param[in] locales Region locales
 * \param[in] device_category Device category
 * \param[out] country_data Country-related data
 */
static void
get_country_data(const clm_country_locales_t *locales,
	clm_device_category_t device_category, country_data_v3_t *country_data)
{
	data_source_id_t ds_id =
		(data_source_id_t)(locales->computed_flags & COUNTRY_FLAGS_DS_MASK);
	data_dsc_t *ds = get_data_sources(ds_id);
	/* Index of region subchannel rules */
	int32 rules_idx;
	bzero(country_data, sizeof(*country_data));
	/* Computing subchannel rules index */
	if (ds->scr_idx_4) {
		/* Deprecated 4-bit noncontiguous index field in first region
		 * flag byte
		 */
		rules_idx = (int32)remove_extra_bits(
			locales->country_flags & CLM_DATA_FLAG_REG_SC_RULES_MASK_4,
			CLM_DATA_FLAG_REG_SC_RULES_EXTRA_BITS_4);
	} else if ((ds->registry_flags & CLM_REGISTRY_FLAG_REGION_FLAG_2) ||
		(ds->registry_flags2 & CLM_REGISTRY_FLAG2_INDEXED_COUNTRY_FLAGS))
	{
		/* New 3+5=8-bit index located in lower 3 and lower 5 bits of
		 * first and second region flag bytes
		 */
		rules_idx = (int32)((locales->country_flags & CLM_DATA_FLAG_REG_SC_RULES_MASK) |
			((locales->country_flags_2 & CLM_DATA_FLAG_2_REG_SC_RULES_MASK)
			<< CLM_DATA_FLAG_REG_SC_RULES_MASK_WIDTH));
	} else {
		/* Original 3-bit index */
		rules_idx = (int32)(locales->country_flags & CLM_DATA_FLAG_REG_SC_RULES_MASK);
	}
	/* Making index 0-based */
	rules_idx -= CLM_SUB_CHAN_IDX_BASE;
	if (rules_idx >= 0) {
		/* Information for shoveling region rules from BLOB to
		 * country_data (band/bandwidth specific)
		 */
		static const struct rules_dsc {
			/** Band */
			clm_band_t band;

			/** Bandwidth */
			clm_bandwidth_t bw;

			/** Registry flags ('registry_flags' field) that must be
			 * set for field to be in BLOB
			 */
			uint32 registry_flags;

			/** Registry flags ('registry_flags2' field) that must be
			 * set for field to be in BLOB
			 */
			uint32 registry_flags2;

			/** Offset to pointer to clm_sub_chan_rules_set_t
			 * structure in BLOB
			 */
			uint32 registry_offset;

			/** For per-device-category subchannel rules - size of rule set
			 * structure. 0 for single-device-category rules
			 */
			uint32 dc_set_struct_size;
		} rules_dscs[] = {
			{CLM_BAND_2G, CLM_BW_40,
			CLM_REGISTRY_FLAG_SUB_CHAN_RULES | CLM_REGISTRY_FLAG_SUBCHAN_RULES_40, 0,
			OFFSETOF(clm_registry_t, sub_chan_rules_2g_40m), 0},
			{CLM_BAND_5G, CLM_BW_40,
			CLM_REGISTRY_FLAG_SUB_CHAN_RULES | CLM_REGISTRY_FLAG_SUBCHAN_RULES_40, 0,
			OFFSETOF(clm_registry_t, sub_chan_rules_5g_40m), 0},
			{CLM_BAND_5G, CLM_BW_80,
			CLM_REGISTRY_FLAG_SUB_CHAN_RULES, 0,
			OFFSETOF(clm_registry_t, sub_chan_rules_80), 0},
			{CLM_BAND_5G, CLM_BW_160,
			CLM_REGISTRY_FLAG_SUB_CHAN_RULES | CLM_REGISTRY_FLAG_160MHZ, 0,
			OFFSETOF(clm_registry_t, sub_chan_rules_160), 0},
#ifdef WL_BAND6G
			{CLM_BAND_6G, CLM_BW_40,
			CLM_REGISTRY_FLAG_SUB_CHAN_RULES | CLM_REGISTRY_FLAG_SUBCHAN_RULES_40,
			CLM_REGISTRY_FLAG2_6GHZ,
			OFFSETOF(clm_registry_t, sub_chan_rules_6g_40),
			sizeof(clm_sub_chan_rules_set_40_t)},
			{CLM_BAND_6G, CLM_BW_80,
			CLM_REGISTRY_FLAG_SUB_CHAN_RULES, CLM_REGISTRY_FLAG2_6GHZ,
			OFFSETOF(clm_registry_t, sub_chan_rules_6g_80),
			sizeof(clm_sub_chan_rules_set_80_t)},
			{CLM_BAND_6G, CLM_BW_160,
			CLM_REGISTRY_FLAG_SUB_CHAN_RULES | CLM_REGISTRY_FLAG_160MHZ,
			CLM_REGISTRY_FLAG2_6GHZ,
			OFFSETOF(clm_registry_t, sub_chan_rules_6g_160),
			sizeof(clm_sub_chan_rules_set_160_t)},
#ifdef CLM_320_MHZ_SUPPORTED
			{CLM_BAND_6G, CLM_BW_320,
			CLM_REGISTRY_FLAG_SUB_CHAN_RULES,
			CLM_REGISTRY_FLAG2_6GHZ | CLM_REGISTRY_FLAG2_320MHZ,
			OFFSETOF(clm_registry_t, sub_chan_rules_6g_320),
			sizeof(clm_sub_chan_rules_set_320_t)},
#endif /* CLM_320_MHZ_SUPPORTED */
#endif /* WL_BAND6G */
		};
		/* Offset in BLOB to this region's rules from
		 * clm_sub_chan_rules_set_t::region_rules
		 */
		uint32 region_rules_offset = rules_idx *
			((ds->registry_flags & CLM_REGISTRY_FLAG_SUBCHAN_RULES_INC_SEPARATE) ?
			sizeof(clm_sub_chan_region_rules_inc_t) :
			sizeof(clm_sub_chan_region_rules_t));
		/* Index in rules_dscs. Corresponds to band/bandwidth */
		uint32 ri;
		for (ri = 0u; ri < ARRAYSIZE(rules_dscs); ++ri) {
			/* Address of current rules_dsc structure */
			const struct rules_dsc *rds = rules_dscs + ri;

			/* Index in sub_chan... fields of country_data_v3_t */
			uint32 bb40_idx = get_bb40_idx(rds->band, rds->bw);

			/* Address of top-level clm_sub_chan_rules_set_t for
			 * current band/bandwidth in BLOB
			 */
			const clm_sub_chan_rules_set_t *blob_rule_set;

			/* Address of this region's
			 * clm_sub_chan_region_rules_inc_t structure in BLOB
			 */
			const clm_sub_chan_region_rules_inc_t *blob_region_rules;

			ASSERT(bb40_idx != ~0u);

			/* Can BLOB have subchannel rules for current
			 * band/bandwidth?
			 */
			if (((ds->registry_flags & rds->registry_flags) != rds->registry_flags) ||
				((ds->registry_flags2 & rds->registry_flags2) !=
				rds->registry_flags2))
			{
				continue;
			}
			blob_rule_set = (const clm_sub_chan_rules_set_t *)get_data(ds_id,
				rds->registry_offset);
			/* Does BLOB actually have subchannel rules for current
			 * band/bandwidth?
			 */
			if (!blob_rule_set) {
				continue;
			}
			if (ds->registry_flags2 & CLM_REGISTRY_FLAG2_PER_DC_SC_RULES) {
				blob_rule_set = (const clm_sub_chan_rules_set_t *)
					((const uint8 *)blob_rule_set +
					dev_cat_translate[device_category] *
					rds->dc_set_struct_size);
			}
			if ((rules_idx >= (int32)blob_rule_set->num)) {
				continue;
			}
			blob_region_rules = (const clm_sub_chan_region_rules_inc_t *)
				((const uint8 *)relocate_ptr(ds_id, blob_rule_set->region_rules) +
				region_rules_offset);

			country_data->sub_chan_channel_rules[bb40_idx].num = blob_region_rules->num;
			country_data->sub_chan_channel_rules[bb40_idx].channel_rules =
				relocate_ptr(ds_id, blob_region_rules->channel_rules);

			if (ds->registry_flags & CLM_REGISTRY_FLAG_SUBCHAN_RULES_INC_SEPARATE) {
				country_data->sub_chan_increments[bb40_idx] =
					(const int8 *)relocate_ptr(ds_id,
					blob_region_rules->increments);
			}
		}
	}
	country_data->chan_ranges_band_bw = ds->chan_ranges_band_bw;
}

/** Fills subchannel table that maps bandwidth to subchannel numbers
 * \param[out] subchannels Subchannel table being filled
 * \param[in] channel Main channel number
 * \param[in] bw Main channel bandwidth (actual type is clm_bandwidth_t)
 * \param[in] limits_type Limits type (subchannel ID)
 * \return TRUE if bandwidth/limits type combination is valid
 */
static bool
fill_full_subchan_table(uint8 subchannels[CLM_BW_NUM], uint32 channel, int32 bw,
	clm_limits_type_t limits_type)
{
	/* Bitmask of path to subccahhel */
	uint8 path;

	/* Bitmask of current position in path */
	uint8 path_mask;

	get_subchan_path(limits_type, &path, &path_mask);

	/* Emptying the map */
	bzero(subchannels, sizeof(uint8) * CLM_BW_NUM);
	for (;;) {
		/* Setting channel number for current bandwidth */
		subchannels[bw] = (uint8)channel;

		/* Is path over? */
		if ((path_mask >>= 1u) == 0u) {
			return TRUE; /* Yes - success */
		}

		/* Descenting one subchannel level */

		/* Chosing half-bandwidth */
		bw = get_bandwidth_traits(bw)->half_bw;
		if (bw == CLM_BW_NUM) {
			return FALSE;	/* No such bandwidth */
		}

		/* Chosing subchannel number at half-bandwidth */
		if (path & path_mask) {
			channel += get_bandwidth_traits(bw)->half_stride;
		} else {
			channel -= get_bandwidth_traits(bw)->half_stride;
		}
	}
}

/* Preliminary observations.
 * Every power in *limits is a minimum over values from several sources:
 * - For main (full) channel - over per-channel-range limits from limits that
 *   contain this channel. There can be legitimately more than one such limit:
 *   e.g. one EIRP and another Conducted (this was legal up to certain moment,
 *   not legal now but may become legal again in future)
 * - For subchannels it might be minimum over several enclosing channels (e.g.
 *   for 20-in-80 it may be minimum over corresponding 20MHz main (full)
 *   channel and 40MHz enclosing main (full) channel). Notice that all
 *   enclosing channels have different channel numbers (e.g. for 36LL it is
 *   40MHz enclosing channel 38 and 80MHz enclosing channel 42)
 * - 2.4GHz 20-in-40 channels also take power targets for DSSS rates from 20MHz
 *   channel data (even though other limits are taken from enclosing 40MHz
 *   channel)
 *
 * So in general resulting limit is a minimum of up to 3 channels (one per
 * bandwidth) and these channels have different numbers!
 * 'bw_to_chan' vector contains mapping from bandwidths to channel numbers.
 * Bandwidths not used in limit computation are mapped to 0.
 * 20-in-40 DSSS case is served by 'channel_dsss' variable, that when nonzero
 * contains number of channel where from DSSS limits shall be taken.
 *
 * 'chan_offsets' mapping is initially computed to derive all these channel
 * numbers from main channel number, main channel bandwidth and power limit
 * type (i.e. subchannel ID)  is computed. Also computation of chan_offsets is
 * used to determine if bandwidth/limits_type pair is valid.
*/
/** Adjusts power, retrieved from CLM BLOB
 * \param[in] blob_power Power value retrieved from BLOB
 * \param[in] tx_flags First byte of TX record group's flags
 * \param[in] params clm_[ru_]limits() additional parameters structure
 * \param[in]ant_gain Antenna gain
 * \param[in]power_inc Power increment from subchannel rule
 * \param[in]power_shift_inc Effective qdbm power shift - difference between
 * BLOB power shift and of parameters' power shift
 * \param[out] qdbm_cp resulting power value, without subchannel rule increment
 * applied. 8 bit
 * \param[out,optional] qdbm_inc_cp resulting power value, with subchannel rule
 * increment applied. 8 bit
 */
static void adjust_tx_power(int8 blob_power, uint8 tx_flags,
	const clm_limits_params_t *params, int32 ant_gain, clm_power_t power_inc,
	int32 power_shift_inc, clm_power_t *qdbm_cp, clm_power_t *qdbm_inc_cp)
{
	int32 qdbm, qdbm_inc, sar;
	if ((uint8)blob_power == (uint8)CLM_DISABLED_POWER) {
		*qdbm_cp = (clm_power_t)blob_power;
		if (qdbm_inc_cp) {
			*qdbm_inc_cp = (clm_power_t)blob_power;
		}
		return;
	}
	qdbm_inc = qdbm = blob_power;
	if ((tx_flags & CLM_DATA_FLAG_MEAS_MASK) == CLM_DATA_FLAG_MEAS_EIRP) {
		qdbm -= (clm_power_t)ant_gain;
		qdbm_inc = qdbm;
	}
	qdbm += power_shift_inc;
	sar = params->sar - params->power_shift;
	if (qdbm > sar) {
		qdbm = sar;
	}
	if (qdbm > MAX_TX_POWER) {
		*qdbm_cp = (clm_power_t)MAX_TX_POWER;
	} else if (qdbm < (clm_power_t)(uint8)MIN_TX_POWER) {
		*qdbm_cp = (clm_power_t)(uint8)CLM_DISABLED_POWER;
	} else {
		*qdbm_cp = (clm_power_t)qdbm;
	}
	if (!qdbm_inc_cp) {
		return;
	}
	qdbm_inc += power_shift_inc + power_inc;
	if (qdbm_inc > MAX_TX_POWER) {
		*qdbm_inc_cp = (clm_power_t)MAX_TX_POWER;
	} else if (qdbm_inc < (clm_power_t)(uint8)MIN_TX_POWER) {
		*qdbm_inc_cp = (clm_power_t)(uint8)CLM_DISABLED_POWER;
	} else {
		*qdbm_inc_cp = (clm_power_t)qdbm_inc;
	}
}

/** Processing normal (non-OFDMA) rate limits of single TX rate group
 * In parameter names below, 'tx_' prefix means that it is characteristic of
 * given rate group
 * \param[in,out] per_rate_limits Per-rate power limits being computed
 * \param[in]tx_rec_len Length of records in group
 * \param[in]tx_num_rec Number of recortds in group
 * \param[in]tx_flags First flag byte for group
 * \param[in]tx_bw Bandwidth for channels in group
 * \param[in]tx_power_idx Index of power byte in records of group
 * \param[in]tx_base_rate Base rate for records in group
 * \param[in]tx_channel_for_bw Channel in groups' bandwidth that corresponds to
 * channel in question (i.e. channel in question itself or containing channel
 * in group)
 * \param[in]tx_comb_set_for_bw Comb set for bandwidth of channels in group
 * \param[in]tx_rate_sets List of rate sets for channels in group
 * \param[in]tx_rate_sets_index NULL or index or rate sets for channels in group
 * \param[in]tx_channel_rangesList of channel ranges for channels in group
 * \param[out]valid_channel True if any powers were found
 * \param[in]params clm_[ru_]limits() parameters
 * \param[in]ant_gain Antenna gain
 * \param[in]power_inc Power increment from subchannel rule
 * \param[in]comb_sets Comb sets for all bandwidths
 * \param[in]channel_dsss Channel for DSSS rates of 20-in-40 power targets (0
 * if irrelevant)
 * \param[in]power_shift_inc Effective qdbm power shift - difference between
 * BLOB power shift and of parameters' power shift
 */
static void BCM_NOINLINE
process_tx_rate_group(clm_power_t *per_rate_limits, const uint8 *tx_rec, uint32 tx_rec_len,
	uint32 tx_num_rec, uint8 tx_flags, clm_bandwidth_t tx_bw, uint32 tx_power_idx,
	uint32 tx_base_rate, uint32 tx_channel_for_bw,
	const clm_channel_comb_set_t *tx_comb_set_for_bw, const uint8 *tx_rate_sets,
	const uint16 *tx_rate_sets_index, const clm_channel_range_t *tx_channel_ranges,
	bool *valid_channel, const clm_limits_params_t *params, int32 ant_gain,
	clm_power_t power_inc, const clm_channel_comb_set_t *const* comb_sets,
	uint32 channel_dsss, int32 power_shift_inc)
{
	/* Nothing interesting can be found in this group */
	if ((!(tx_channel_for_bw || (channel_dsss && (tx_bw == CLM_BW_20)))) ||
		((tx_power_idx >= tx_rec_len) && *valid_channel))
	{
		return;
	}
	for (; tx_num_rec--; tx_rec += tx_rec_len)	{
		/* Channel range for current transmission power record */
		const clm_channel_range_t *range =
			tx_channel_ranges + tx_rec[CLM_LOC_DSC_RANGE_IDX];

		/* Power targets for current transmission power record
		 * - original and incremented per subchannel rule
		 */
		clm_power_t qdbm, qdbm_inc;

		/* Per-antenna record without a limit for given antenna index?
		 */
		if (tx_power_idx >= tx_rec_len) {
			/* At least check if channel is valid */
			if (!*valid_channel && tx_channel_for_bw &&
				channel_in_range(tx_channel_for_bw, range, tx_comb_set_for_bw,
				params->other_80_80_chan) &&
				((uint8)tx_rec[CLM_LOC_DSC_POWER_IDX] != (uint8)CLM_DISABLED_POWER))
			{
				*valid_channel = TRUE;
				/* Return to skip the rest of group */
				return;
			}
		}
		adjust_tx_power(tx_rec[tx_power_idx], tx_flags, params, ant_gain,
			power_inc, power_shift_inc, &qdbm, &qdbm_inc);

		/* If this record related to channel for this bandwidth? */
		if (tx_channel_for_bw &&
			channel_in_range(tx_channel_for_bw, range, tx_comb_set_for_bw,
			params->other_80_80_chan))
		{
			/* Rate indices  for current records' rate set */
			const uint8 *rates = get_byte_string(tx_rate_sets,
				tx_rec[CLM_LOC_DSC_RATE_IDX], tx_rate_sets_index);
			uint32 num_rates = *rates++;
			/* Loop over this TX power record's rate indices */
			while (num_rates--) {
				uint32 rate_idx = *rates++ + tx_base_rate;
				clm_power_t *pp = per_rate_limits + rate_idx;
				/* Chosing minimum (if CLF_SCR_MIN) or latest power */
				if ((!channel_dsss || (get_rate_type(rate_idx) != RT_DSSS)) &&
					((*pp == (clm_power_t)(uint8)UNSPECIFIED_POWER) ||
					((*pp > qdbm_inc) &&
					(*pp != (clm_power_t)(uint8)CLM_DISABLED_POWER))))
				{
					*pp = qdbm_inc;
				}
			}
			if ((uint8)qdbm_inc != (uint8)CLM_DISABLED_POWER) {
				*valid_channel = TRUE;
			}
		}
		/* If this rule related to 20-in-something DSSS channel? */
		if (channel_dsss && (tx_bw == CLM_BW_20) &&
			channel_in_range(channel_dsss, range, comb_sets[CLM_BW_20], 0))
		{
			/* Same as above */
			const uint8 *rates = get_byte_string(tx_rate_sets,
				tx_rec[CLM_LOC_DSC_RATE_IDX], tx_rate_sets_index);
			uint32 num_rates = *rates++;
			while (num_rates--) {
				uint32 rate_idx = *rates++ + tx_base_rate;
				clm_power_t *pp = per_rate_limits + rate_idx;
				if (get_rate_type(rate_idx) == RT_DSSS) {
					*pp = qdbm;
				}
			}
		}
	}
}

#ifdef WL_RU_NUMRATES
/** Finds out what RU rate(s) are to be generated by given rate in BLOB
 * RU rates generate themselves (if any), HE0 rates generate derived RU rates,
 * rest generates nothing
 * \param[in, out] default_derivation On input - contains rate from BLOB (value
 * from clm_rates_t or clm_ru_rates_t i.e. with base rate already added). On
 * output may be used as buffer for single-rate derivation
 * \param[in] limits_type - Channel/subchannel
 * \param[in] tx_bw Bandwidth from record group header
 * \param[in] params Parameter structure from clm_ru_limits()
 * \param[in] rct Rate encoding type
 * \param[in] tx_bw_traits Bandwidth traits for tx_bw
 * \param[in] derive_mru True to derive all possible MRU rates from HE0, false
 * to only derive full-channel RU rate from HE0
 * \param[out] ru_min_bw Minimum bandwidth for derived RU rates
 * \param[out] derivations Vector of derivation descriptors
 * \param[out] num_derivatons Length of verctor of derivation descriptors
 * \param[out] overwrite True to overwrite existing rate (if any), false to
 * compute minimum
 * \param[out] has_he_limit Become true once HE0 rate found
 */
static void
get_ru_derivations(ru_derivation_t *default_derivation,
	clm_limits_type_t limits_type, clm_bandwidth_t tx_bw,
	const clm_limits_params_t *params, clm_rate_code_type_t rct,
	const bandwidth_traits_t *tx_bw_traits, bool derive_mru,
	clm_bandwidth_t *ru_min_bw, const ru_derivation_t **derivations,
	uint32 *num_derivations, uint32 *derivation_rate_offset, bool *overwrite,
	bool *has_he_limit)
{
	uint32 rate_idx = default_derivation->rate;
	*derivations = default_derivation;
	*num_derivations = 1;
	*derivation_rate_offset = 0;
	*overwrite = FALSE;
	*ru_min_bw = CLM_BW_NUM;
	if (rct >= RCT_MIN_RU) {
		/* OFDMA rate (no derivation needed). What kind? */
		*ru_min_bw = get_ru_rate_min_bw(rate_idx);
		if (*ru_min_bw == CLM_BW_NUM) {
			/* UB/LUB. Ignored for subchannels */
			if ((limits_type == CLM_LIMITS_TYPE_CHANNEL) && (tx_bw == params->bw)) {
				*ru_min_bw = tx_bw_traits->active;
				*overwrite = TRUE;
			} else {
				*num_derivations = 0;
			}
		}
		return;
	}
	/* SU rate. Only HE0 rates are of interest */
	if ((get_rate_type(rate_idx) != RT_SU) ||
		(get_rate_type(rate_idx - 1u) == RT_SU))
	{
		/* Not a HE0 rate */
		*num_derivations = 0;
		return;
	}
	if (tx_bw == params->bw) {
		/* HE0 rate for whole channel - this makes RU limits legitimate */
		*has_he_limit = TRUE;
	}
	/* RU bandwidth is a bandwidth of HE0 it will be derived from */
	*ru_min_bw = tx_bw_traits->active;
	/* HE0 rate - considering derivation */
#ifdef CLM_MRU_RATES_SUPPORTED
	if (derive_mru) {
		/* MRU derivation: full-channel and non-full-channel RU rates */
		*derivations = tx_bw_traits->ru_derivations;
		*num_derivations = tx_bw_traits->num_ru_derivations;
	} else
#endif /* CLM_MRU_RATES_SUPPORTED */
	if (tx_bw_traits->channel_ru_rate != WL_RU_NUMRATES) {
		/* RU derivation: only full-channel RU rate, if any */
		default_derivation->rate = tx_bw_traits->channel_ru_rate;
	} else {
		/* RU derivation, no full-channel rate for given bandwidth */
		*num_derivations = 0;
	}
	*derivation_rate_offset = get_he_tx_mode(rate_idx);
}

/** Processing OFDMA rate limits of single TX rate group
 * In parameter names below, 'tx_' prefix means that it is characteristic of
 * given rate group
 * \param[in,out] per_rate_limits Per-rate power limits being computed
 * \param[in]tx_rec_len Length of records in group
 * \param[in]tx_num_rec Number of recortds in group
 * \param[in]tx_flags First flag byte for group
 * \param[in]tx_flags2 Second flag byte for group
 * \param[in]tx_flags3 Third flag byte for group
 * \param[in]tx_bw Bandwidth for channels in group
 * \param[in]tx_power_idx Index of power byte in records of group
 * \param[in]tx_base_rate Base rate for records in group
 * \param[in]tx_channel_for_bw Channel in groups' bandwidth that corresponds to
 * channel in question (i.e. channel in question itself or containing channel
 * in group)
 * \param[in]tx_comb_set_for_bw Comb set for bandwidth of channels in group
 * \param[in]tx_rate_sets List of rate sets for channels in group
 * \param[in]tx_rate_sets_index NULL or index or rate sets for channels in group
 * \param[in]tx_channel_ranges List of channel ranges for channels in group
 * \param[out]has_he_limit True if ther eis HE0 power for main channel
 * \param[in]params clm_[ru_]limits() parameters
 * \param[in]ant_gain Antenns gain
 * \param[in]limits_type Limits type (channel/subchannel)
 * \param[in] bw_to_chan Maps bandwidths (elements of clm_bw_t enum) from
 * subchannel bandwidth to whole channel bandwidth to correspondent channel
 * numbers, all other bandwidths - to zero
 * \param[in]power_shift_inc Effective qdbm power shift - difference between
 * BLOB power shift and of parameters' power shift
 * \param[in]derive_mru Perform MRU derivation
 */
static void BCM_NOINLINE
process_tx_ru_rate_group(clm_power_t *per_rate_limits, const uint8 *tx_rec,
	uint32 tx_rec_len, uint32 tx_num_rec, uint8 tx_flags, uint8 tx_flags2,
	uint8 tx_flags3, clm_bandwidth_t tx_bw, uint32 tx_power_idx,
	uint32 tx_base_rate, uint32 tx_channel_for_bw,
	const clm_channel_comb_set_t *tx_comb_set_for_bw, const uint8 *tx_rate_sets,
	const uint16 *tx_rate_sets_index, const clm_channel_range_t *tx_channel_ranges,
	bool *has_he_limit, const clm_limits_params_t *params, int32 ant_gain,
	clm_limits_type_t limits_type, uint8 bw_to_chan[CLM_BW_NUM],
	int32 power_shift_inc, bool derive_mru)
{
	/** For each bandwidth specifies first (SS1, unexpanded) RU rate with such
	 * minimum bandwidth that is equivalent to some HE0 rate
	 */
	clm_bandwidth_t whole_channel_bw = get_bandwidth_traits(params->bw)->active;
	clm_bandwidth_t subchannel_bw = get_subchan_bw(limits_type, params->bw);
	int32 half_subchan_width = get_bandwidth_traits(subchannel_bw)->half_stride;
	/* Bounds of (sub) channel in terms of channel numbers */
	int32 left_bound = (int32)bw_to_chan[subchannel_bw] - half_subchan_width;
	int32 right_bound = (int32)bw_to_chan[subchannel_bw] + half_subchan_width;
	/* Channel traits for TX bandwidth */
	const bandwidth_traits_t *tx_bw_traits = get_bandwidth_traits(tx_bw);
	/** Half-width of channels in group in terms of channel numbers */
	int32 tx_half_width = tx_bw_traits->half_stride;
	int32 min_intersection_width = (tx_half_width < half_subchan_width) ?
		(2 * tx_half_width) : 2 * (half_subchan_width);
	clm_rate_code_type_t rct = get_rate_code_type(tx_flags2, tx_flags3);
	if ((!tx_channel_for_bw && (tx_bw > params->bw)) || (tx_power_idx >= tx_rec_len)) {
		/* Nothing interesting can be found in this group */
		return;
	}
	/* Checking if it is RU subchannel record group from higher bandwidth
	 * channel - skip it if so
	 */
	if (rct >= RCT_MIN_RU) {
		clm_bandwidth_t outer_bw = get_outer_bw(tx_flags2, tx_flags3, tx_bw);
		if ((outer_bw == CLM_BW_80_80) ? (params->bw != CLM_BW_80_80) :
			(outer_bw > whole_channel_bw))
		{
			return;
		}
	}
	for (; tx_num_rec--; tx_rec += tx_rec_len)	{
		/* Channel range for current transmission power record */
		const clm_channel_range_t *range =
			tx_channel_ranges + tx_rec[CLM_LOC_DSC_RANGE_IDX];
		/* Range end channel number */
		int32 range_end = (tx_bw == CLM_BW_80_80) ? range->start : range->end;

		/* Power targets for current transmission power record */
		clm_power_t qdbm;
		adjust_tx_power(tx_rec[tx_power_idx], tx_flags, params, ant_gain, 0,
			power_shift_inc, &qdbm, NULL);

		/* Record may be for range that is wider than or same width as
		 * (sub)channel - in this case 'tx_channel_for_bw' is nonzero
		 * and test by subchannel table performed, or it may be narrower
		 * than (sub) channel - in this case intersection test is
		 * performed
		 */
		if (tx_channel_for_bw
			? channel_in_range(tx_channel_for_bw, range, tx_comb_set_for_bw,
			params->other_80_80_chan)
			: ranges_intersect(left_bound, right_bound,
				(int32)range->start - tx_half_width,
				(int32)range_end + tx_half_width, min_intersection_width))
		{
			/* Rate indices  for current records' rate set */
			const uint8 *rates = get_byte_string(tx_rate_sets,
				tx_rec[CLM_LOC_DSC_RATE_IDX], tx_rate_sets_index);
			uint32 num_rates = *rates++;
			/* Loop over this TX power record's rate indices */
			while (num_rates--) {
				ru_derivation_t default_derivation = {*rates++ + tx_base_rate, 0};
				const ru_derivation_t *derivations;
				uint32 num_derivations;
				bool overwrite;
				clm_bandwidth_t ru_min_bw;
				uint32 derivation_rate_offset;
				get_ru_derivations(&default_derivation, limits_type, tx_bw, params,
					rct, tx_bw_traits, derive_mru, &ru_min_bw, &derivations,
					&num_derivations, &derivation_rate_offset, &overwrite,
					has_he_limit);
				if (ru_min_bw > subchannel_bw) {
					/* RU rate wider than (sub)channel - skip */
					continue;
				}
				for (; num_derivations--; ++derivations) {
					clm_power_t *pp = per_rate_limits + derivations->rate +
						derivation_rate_offset;
					clm_power_t adjusted_limit =
						(qdbm == (clm_power_t)(uint8)CLM_DISABLED_POWER) ?
						qdbm : (clm_power_t)(qdbm - derivations->backoff);
					if (overwrite) {
						*pp = adjusted_limit;
						continue;
					}
					/* If limit for rate was not yet specified or if
					 * recod's channel range completely covers the
					 * channel - overwriting limit for rate (assuming
					 * that rate groups in BLOB sorted appropriately),
					 * otherwise writing minimum value
					 */
					if ((*pp == (clm_power_t)(uint8)UNSPECIFIED_POWER) ||
						(((range->start - tx_half_width) <= left_bound) &&
						((range_end + tx_half_width) >= right_bound)) ||
						((*pp > adjusted_limit) &&
						(*pp != (clm_power_t)(uint8)CLM_DISABLED_POWER)))
					{
						*pp = adjusted_limit;
					}
				}
			}
		}
	}
}
#endif /* WL_RU_NUMRATES */

/** Compute power limits for one locale (base or HT)
 * This function is formerly a part of clm_limits(), that become separate
 * function in process of functions' size limiting action. Thus the above
 * comment is actually related to both clm_limits() (that prepares various
 * parameters and does the postprocessing) and compute_locale_limits() (that
 * performs actual computation of power limits)
 * \param[out] per_rate_limits Per-rate power limits being computed
 * \param[out] valid_channel TRUE if there is at least one enabled power limit
 * \param[in] params Miscellaneous power computation parameters, passed to
 * clm_limits()
 * \param[in] ant_gain Antenna gain in dBi (for use in EIRP limits computation)
 * \param[in] limits_type Subchannel to get limits for
 * \param[in] loc_data Locale-specific helper data, precomputed in clm_limits()
 * \param[in] channel_dsss Channel for DSSS rates of 20-in-40 power targets (0
 * if irrelevant)
 * \param[in] power_inc  Power increment from subchannel rule
 * \param[in] bw_to_chan Maps bandwidths (elements of clm_bw_t enum) used in
 * subchannel rule to channel numbers. Bandwidths not used in subchannel rule
 * mapped to zero
 * \param[in] ru_limits TRUE means that RU limits (for rates defined in
 * clm_ru_rates_t) shall be retrieved, FALSE means that normal limits (for
 * rates defined in clm_ru_rates_t) shall be retrieved
 * \param[in]power_shift_inc Effective qdbm power shift - difference between
 * BLOB power shift and of parameters' power shift
 * \param[in]derive_mru Perform MRU derivation
 */
static void
compute_locale_limits(clm_power_t *per_rate_limits, bool *valid_channel,
	const clm_limits_params_t *params, int32 ant_gain, clm_limits_type_t limits_type,
	const locale_data_t *loc_data, uint32 channel_dsss, clm_power_t power_inc,
	uint8 bw_to_chan[CLM_BW_NUM], bool ru_limits, int32 power_shift_inc,
	bool derive_mru)
{
	/** Transmission power records' sequence for current locale */
	const uint8 *tx_rec = loc_data->tx_limits;

	/** Channel combs for given band - vector indexed by channel bandwidth
	 */
	const clm_channel_comb_set_t *const* comb_sets = loc_data->combs;

	/** CLM_DATA_FLAG_ flags for current group of TX power records */
	uint8 tx_flags, tx_flags2, tx_flags3;

	/* Loop over all groups of TX power records */
	do {
		/** Number of records in group */
		uint32 tx_num_rec;

		/** Bandwidth of records in group */
		clm_bandwidth_t tx_bw;

		/** Channel combs for bandwidth used in group.
		 * NULL for 80+80 channel
		 */
		const clm_channel_comb_set_t *tx_comb_set_for_bw;

		/** Channel number to look for bandwidth used in this group */
		uint32 tx_channel_for_bw;

		/** Length of TX power records in current group */
		uint32 tx_rec_len;

		/** Index of TX power inside TX power record */
		uint32 tx_power_idx;

		/** Base address for channel ranges for bandwidth used in
		 * current group
		 */
		const clm_channel_range_t *tx_channel_ranges;

		/** Sequence of rate sets' definition for bw used in this group
		 */
		const uint8 *tx_rate_sets = NULL;

		/** Index of sequence of rate sets' definition for bw used in
		 * this group
		 */
		const uint16 *tx_rate_sets_index = NULL;

		/** Base value for rates in current rate set */
		uint32 tx_base_rate = 0u;

		/** Device category of TX group */
		clm_device_category_t device_category = CLM_DEVICE_CATEGORY_NUM;

		/** Rate encoding */
		clm_rate_code_type_t rct;

		tx_flags = *tx_rec++;
		tx_flags2 = (tx_flags & CLM_DATA_FLAG_FLAG2) ? *tx_rec++ : 0u;
		tx_flags3 = (tx_flags2 & CLM_DATA_FLAG2_FLAG3) ? *tx_rec++ : 0u;
		tx_rec_len = CLM_LOC_DSC_TX_REC_LEN +
			((tx_flags & CLM_DATA_FLAG_PER_ANT_MASK) >> CLM_DATA_FLAG_PER_ANT_SHIFT);
		tx_num_rec = *tx_rec++;
		rct = get_rate_code_type(tx_flags2, tx_flags3);
		(void)get_dev_cat_reg_dest(tx_flags2, &device_category, NULL);
		ASSERT(device_category != CLM_DEVICE_CATEGORY_NUM);
		tx_bw = get_tx_rec_bw(tx_flags);
		if ((tx_bw != CLM_BW_NUM) && tx_num_rec &&
			(!(tx_flags2 & CLM_DATA_FLAG2_WIDTH_EXT)) &&
			(ru_limits || (rct < RCT_MIN_RU)) &&
			(device_category == params->device_category))
		{
			tx_comb_set_for_bw = comb_sets[tx_bw];
			tx_channel_for_bw = bw_to_chan[tx_bw];
			tx_power_idx = (tx_rec_len == CLM_LOC_DSC_TX_REC_LEN)
				? CLM_LOC_DSC_POWER_IDX
				: antenna_power_offsets[params->antenna_idx];
			tx_channel_ranges = loc_data->chan_ranges_bw[tx_bw];
			get_rate_encoding_traits(rct, tx_bw, loc_data,
				&tx_base_rate, &tx_rate_sets, &tx_rate_sets_index);
			/* Fail on absent rate set table for a bandwidth when
			 * TX limit table is nonempty (which means that
			 * ClmCompiler has a bug)
			 */
			ASSERT(tx_rate_sets != NULL);
			/* Fail on absent channel range table for a bandwidth
			 * when TX limit table is nonempty (which means that
			 * ClmCompiler has a bug)
			 */
			ASSERT(tx_channel_ranges != NULL);
			if (!(tx_rate_sets && tx_channel_ranges)) {
				/* This should never happen (as it means ClmCompiler bug that
				 * should have been caught by some other means), but it makes
				 * Coverity happy
				 */
				return;
			}
#ifdef WL_RU_NUMRATES
			if (ru_limits) {
				process_tx_ru_rate_group(per_rate_limits, tx_rec, tx_rec_len,
					tx_num_rec, tx_flags, tx_flags2, tx_flags3, tx_bw,
					tx_power_idx, tx_base_rate, tx_channel_for_bw,
					tx_comb_set_for_bw, tx_rate_sets, tx_rate_sets_index,
					tx_channel_ranges, valid_channel, params, ant_gain,
					limits_type, bw_to_chan, power_shift_inc, derive_mru);
			} else
#else /* WL_RU_NUMRATES */
			(void)limits_type;
			(void)derive_mru;
#endif /* else WL_RU_NUMRATES */
			{
				process_tx_rate_group(per_rate_limits, tx_rec, tx_rec_len,
					tx_num_rec, tx_flags, tx_bw, tx_power_idx, tx_base_rate,
					tx_channel_for_bw, tx_comb_set_for_bw, tx_rate_sets,
					tx_rate_sets_index, tx_channel_ranges, valid_channel,
					params, ant_gain, power_inc, comb_sets, channel_dsss,
					power_shift_inc);
			}
		}
		tx_rec += tx_num_rec * tx_rec_len;
	} while (tx_flags & CLM_DATA_FLAG_MORE);
}

/** Common part of clm_limits() and clm_ru_limits
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[in] channel Channel number (main channel if subchannel limits output
 * is required)
 * \param[in] ant_gain Antenna gain in quarter dBm (used if limit is given in
 * EIRP terms)
 * \param[in] limits_type Subchannel to get limits for
 * \param[in] params Other parameters
 * \param[out] per_rate_limits Buffer for per-rate limits
 * \param[in] num_limits Length of buffer for per-rate limits
 * \param[in] ru_limits TRUE means that RU limits (for rates defined in
 * clm_ru_rates_t) shall be retrieved, FALSE means that normal limits (for
 * rates defined in clm_rates_t) shall be retrieved
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` is
 * null or contains invalid information, or if any other input parameter
 * (except channel) has invalid value, CLM_RESULT_NOT_FOUND if channel has
 * invalid value
 */
static clm_result_t
compute_clm_limits(const clm_country_locales_t *locales, clm_band_t band,
	uint32 channel, int32 ant_gain, clm_limits_type_t limits_type,
	const clm_limits_params_t *params, clm_power_t *per_rate_limits,
	uint32 num_limits, bool ru_limits)
{
	/** Loop variable. Points first to base than to HT locale */
	uint32 base_ht_idx;

	/** Channel for DSSS rates of 20-in-40 power targets (0 if
	 * irrelevant)
	 */
	uint32 channel_dsss = 0u;

	/** Become true if at least one power target was found */
	bool valid_channel = FALSE;

	/** Maps bandwidths to subchannel numbers */
	uint8 subchannels[CLM_BW_NUM];

	/** Country (region) precomputed data */
	country_data_v3_t country_data;

	/** Per-bandwidth channel comb sets taken from same data source as
	 * country definition
	 */
	const clm_channel_comb_set_t *country_comb_sets;

	/* Data set descriptor */
	data_dsc_t *ds;

	/** Traits of (outer) channel bandwidth */
	const bandwidth_traits_t *bw_traits;

	/* Simple validity check */
	if (!locales || !per_rate_limits || !num_limits || (num_limits > WL_NUMRATES) ||
		((uint32)band >= (uint32)CLM_BAND_NUM) || !params ||
		!is_valid_bandwidth(params->bw) ||
		((uint32)limits_type >= (uint32)CLM_LIMITS_TYPE_NUM) ||
		((uint32)params->antenna_idx >= (uint32)WL_TX_CHAINS_MAX) ||
		((uint32)params->device_category >= (uint32)CLM_DEVICE_CATEGORY_NUM))
	{
		return CLM_RESULT_ERR;
	}
	bw_traits = get_bandwidth_traits(params->bw);

	/* Fills bandwidth->channel number map */
	if (!fill_full_subchan_table(subchannels, channel, bw_traits->active, limits_type)) {
		/** bandwidth/limits_type pair is invalid */
		return CLM_RESULT_ERR;
	}
	subchannels[params->bw] = subchannels[bw_traits->active];

	ds = get_data_sources((data_source_id_t)(locales->computed_flags & COUNTRY_FLAGS_DS_MASK));

	if ((band == CLM_BAND_2G) && (params->bw != CLM_BW_20) && subchannels[CLM_BW_20] &&
		!ru_limits)
	{
		/* 20-in-something, 2.4GHz. Channel to take DSSS limits from */
		channel_dsss = subchannels[CLM_BW_20];
	}
	memset(per_rate_limits, (uint8)UNSPECIFIED_POWER, num_limits);

	/* Obtaining precomputed country data */
	get_country_data(locales, params->device_category, &country_data);

	/* Obtaining comb sets pertinent to data source that contains
	* country
	*/
	country_comb_sets = ds->valid_channel_combs[band];

	/* For base then HT locales do */
	for (base_ht_idx = 0u; base_ht_idx < BH_NUM; ++base_ht_idx) {
		/** Same as subchannels, but only has nonzeros for bandwidths,
		 * used by current subchannel rule
		 */
		uint8 bw_to_chan[CLM_BW_NUM];

		/** Power increment from subchannel rule */
		clm_power_t power_inc = 0;

		/** Effective qdbm power shift - difference between BLOB power
		 * shift and of parameters' power shift
		 */
		int32 power_shift_inc;

		/** Properties of currently traversed locale */
		locale_data_t loc_data;

		if (!get_loc_def(locales, compose_loc_type[band][base_ht_idx], &loc_data)) {
			return CLM_RESULT_ERR;
		}

		/* No transmission power records or lookin for HE0 or HE limits
		 * and locale is base - nothing to do for this locale
		 */
		if ((!loc_data.tx_limits) || (ru_limits && (base_ht_idx == BH_BASE))) {
			continue;
		}

		power_shift_inc =
			((loc_data.ds->registry_flags2 & CLM_REGISTRY_FLAG2_POWER_SHIFT) ?
			loc_data.ds->data->tx_power_shift : 0) - params->power_shift;

		/* Now computing 'bw_to_chan' - bandwidth to channel map that
		 * determines which channels will be used for limit computation
		 * RU limits require full subchannel table
		 */
		/* Preset to 'no bandwidths' */
		bzero(bw_to_chan, sizeof(bw_to_chan));
		if (!ru_limits) {
			if (limits_type == CLM_LIMITS_TYPE_CHANNEL) {
				/* Main channel case - bandwidth specified as
				 * parameter, channel specified as parameter
				 */
				bw_to_chan[params->bw] = (uint8)channel;
			} else if (params->bw != CLM_BW_20) {
				/* Index in country_data's [BB40_NUM] fields */
				uint32 bb40_idx = get_bb40_idx(band, bw_traits->active);
				ASSERT(bb40_idx != ~0u);

				if ((params->bw == CLM_BW_40) &&
					!country_data.sub_chan_channel_rules[bb40_idx].
					channel_rules)
				{
					/* Default for 20-in-40: use limit from 40MHz rule */
					bw_to_chan[CLM_BW_40] = (uint8)channel;
				} else {
					fill_actual_subchan_table(bw_to_chan, &power_inc,
						subchannels, limits_type, channel,
						country_data.chan_ranges_band_bw[band]
						[bw_traits->active],
						&country_comb_sets[bw_traits->active],
						&country_data.sub_chan_channel_rules[bb40_idx],
						country_data.sub_chan_increments[bb40_idx],
						bw_traits->num_subchannels, ds->registry_flags);
					if (params->bw != bw_traits->active) {
						bw_to_chan[params->bw] =
							bw_to_chan[bw_traits->active];
						bw_to_chan[bw_traits->active] = 0u;
					}
				}
			}
			/* bw_to_chan computed */
		}
		compute_locale_limits(per_rate_limits, &valid_channel, params, ant_gain,
			limits_type, &loc_data, channel_dsss, power_inc,
			ru_limits ? subchannels : bw_to_chan, ru_limits, power_shift_inc,
			(locales->country_flags_3 & CLM_DATA_FLAG_3_REG_MRU) != 0);
	}
	if (valid_channel) {
		/* Converting CLM_DISABLED_POWER and UNSPECIFIED_POWER to
		 * WL_RATE_DISABLED
		 */
		clm_power_t *pp = per_rate_limits, *end = pp + num_limits;
		/* Subchannel with complex subchannel rule with all rates
		 * disabled might be falsely detected as valid. For default
		 * (zero) antenna assume that channel is invalid and will look
		 * for refutation
		 */
		if (params->antenna_idx == 0) {
			valid_channel = FALSE;
		}
		do {
			if ((*pp == (clm_power_t)(uint8)CLM_DISABLED_POWER) ||
				(*pp == (clm_power_t)(uint8)UNSPECIFIED_POWER))
			{
				*pp = (clm_power_t)WL_RATE_DISABLED;
			} else {
				/* Found enabled rate - channel is valid */
				valid_channel = TRUE;
			}
		} while (++pp < end);
	} else if (ru_limits) {
		memset(per_rate_limits, (uint8)WL_RATE_DISABLED, num_limits);
	}
	return valid_channel ? CLM_RESULT_OK : CLM_RESULT_NOT_FOUND;
}

#ifdef WL11BE
/** If params refers to punctured bandwidth - modify parameters to point to
 * its base bandwidth
 * \param[in] band Band
 * \param[in] limits_type Subchannel to get limits for
 * \param[out] punctured_bw_traits Output parameter. NULL or points to
 * descriptor of punctured bandwidth to use for results' modification
 * \param[in, out] params Pointer to pointer to parameter structure passed to
 * clm_limits() or clm_ru_limits(). If parameters do not need to be modified -
 * returned as is, otherwise params_buf will contain modified parameters and
 * this pointer will be returned pointing to it
 * \param[out] params_buf Buffer for modified parameters (if needed)
 * \return CLM_RESULT_OK if parameters are consistent, CLM_RESULT_NOT_FOUND if
 * anything punctured requested for band other than 6GHz, CLM_RESULT_ERR if
 * parameters are inconsistent
 */
static clm_result_t
check_punctured_bw(clm_band_t band, clm_limits_type_t limits_type,
	const bandwidth_traits_t **punctured_bw_traits,
	const clm_limits_params_t **params, clm_limits_params_t *params_buf)
{
	const bandwidth_traits_t *outer_bw_traits;
	/* Checking parameters' individual validity */
	if (!params || ((uint32)(**params).bw >= (uint32)CLM_BW_NUM) ||
		((uint32)(**params).punctured_subchan_bw > (uint32)CLM_BW_NUM) ||
		((uint32)limits_type >= CLM_LIMITS_TYPE_NUM))
	{
		/* Bad parameters */
		return CLM_RESULT_ERR;
	}
	/* If outer bandwidth is punctured - it should be replaced with its nonpunctured base */
	outer_bw_traits = get_bandwidth_traits((**params).bw);
	if (outer_bw_traits->punctured_base_bw != CLM_BW_NUM) {
		/* Yes, outer bandwidth is punctured */
		*params_buf = **params;
		params_buf->bw = outer_bw_traits->punctured_base_bw;
		*params = params_buf;
	}
	/* If inner bandwidth is punctured - punctured_bw_traits must be returned */
	*punctured_bw_traits = NULL;
	if ((**params).punctured_subchan_bw != CLM_BW_NUM) {
		/* Inner bandwidth may be punctured */
		/** Nonpunctured bandwidth for given outer channel bandwidth and subchannel ID */
		clm_bandwidth_t nonpunctured_inner_bw = get_subchan_bw(limits_type, (**params).bw);
		const bandwidth_traits_t *inner_bw_traits;
		if (nonpunctured_inner_bw == CLM_BW_NUM) {
			/** Outer channel bandwidth do not have subchannel with such an ID */
			return CLM_RESULT_ERR;
		}
		/* Finding information about inner (presumably punctured) bandwidth */
		inner_bw_traits = get_bandwidth_traits((**params).punctured_subchan_bw);
		/* Is inner bandwidth really punctured? */
		if (inner_bw_traits->punctured_base_bw != CLM_BW_NUM) {
			/* Inner bandwidth punctured */
			*punctured_bw_traits = inner_bw_traits;
			/* Is it consistent with outer channel and subchannel ID? */
			if (inner_bw_traits->punctured_base_bw != nonpunctured_inner_bw) {
				/* Not consistent! */
				return CLM_RESULT_ERR;
			}
		} else {
			/* Inner bandwidth not punctured. But is it valid? */
			if (inner_bw_traits->active != nonpunctured_inner_bw) {
				/* Not consistent! */
				return CLM_RESULT_ERR;
			}
		}
	}
	/* Outer or inner bandwidth may only be punctured for 6GHz band */
	if ((outer_bw_traits->punctured_base_bw != CLM_BW_NUM) || *punctured_bw_traits) {
#ifdef WL_BAND6G
		if (band != CLM_BAND_6G) {
			return CLM_RESULT_NOT_FOUND;
		}
#else /* WL_BAND6G */
		return CLM_RESULT_NOT_FOUND;
#endif /* else WL_BAND6G */
	}
	return CLM_RESULT_OK;
}
#endif /* WL11BE */

clm_result_t
clm_limits(const clm_country_locales_t *locales, clm_band_t band, uint32 channel,
	int32 ant_gain, clm_limits_type_t limits_type, const clm_limits_params_t *params,
	clm_power_limits_t *limits)
{
#ifdef WL11BE
	const bandwidth_traits_t *punctured_bw_traits;
	clm_limits_params_t params_buf;
#endif /* WL11BE */
	clm_result_t ret;
	if (!limits) {
		return CLM_RESULT_ERR;
	}
#if defined(WL11BE)
	ret = check_punctured_bw(band, limits_type, &punctured_bw_traits, &params, &params_buf);
	if (ret != CLM_RESULT_OK) {
		return ret;
	}
#endif /* defined(WL11BE) */
	ret = compute_clm_limits(locales, band, channel, ant_gain, limits_type, params,
		limits->limit, ARRAYSIZE(limits->limit), FALSE);
#ifdef WL11BE
	if ((ret == CLM_RESULT_OK) && punctured_bw_traits) {
		/* Punctured channel - decrement limits by backoff value */
		clm_power_t *pp = limits->limit, *end = pp + ARRAYSIZE(limits->limit);
		clm_power_t backoff = (clm_power_t)punctured_bw_traits->punctured_su_backoff;
		/* Limit so small, that values below will not fit the range and
		 * hence should be disabled
		 */
		clm_power_t min_power = (clm_power_t)(uint8)MIN_TX_POWER + backoff;
		do {
			if (*pp == (clm_power_t)WL_RATE_DISABLED) {
				continue;
			}
			if (*pp < min_power) {
				*pp = (clm_power_t)WL_RATE_DISABLED;
			} else {
				*pp -= backoff;
			}
		} while (++pp < end);
	}
#endif /* WL11BE */
	return ret;
}

#ifdef WL_RU_NUMRATES
clm_result_t
clm_ru_limits(const clm_country_locales_t *locales, clm_band_t band, uint32 channel,
	int32 ant_gain,
	clm_limits_type_t limits_type, const clm_limits_params_t *params,
	clm_ru_power_limits_t *limits)
{
#ifdef WL11BE
	const bandwidth_traits_t *punctured_bw_traits;
	clm_limits_params_t params_buf;
#endif /* WL11BE */
	clm_result_t ret;
	if (!limits) {
		return CLM_RESULT_ERR;
	}
#if defined(WL11BE)
	ret = check_punctured_bw(band, limits_type, &punctured_bw_traits, &params, &params_buf);
	if (ret != CLM_RESULT_OK) {
		return ret;
	}
#endif /* else defined(WL11BE) */
	ret = compute_clm_limits(locales, band, channel, ant_gain, limits_type, params,
		limits->limit, ARRAYSIZE(limits->limit), TRUE);
#ifdef WL11BE
	if ((ret == CLM_RESULT_OK) && punctured_bw_traits) {
		/* Punctured channel - drop impossible limits */
		clm_power_t *pp = limits->limit;
		uint32 ru_idx;
		for (ru_idx = 0u; ru_idx < ARRAYSIZE(limits->limit); ++pp, ++ru_idx) {
			const clm_he_rate_dsc_t *dummy;
			wl_he_rate_type_t ru_rate_type;
			if (*pp == (clm_power_t)WL_RATE_DISABLED) {
				continue;
			}
			get_ru_rate_info(ru_idx, &dummy, &ru_rate_type);
			if (!(punctured_bw_traits->punctured_rate_type_mask & (1 << ru_rate_type)))
			{
				*pp = (clm_power_t)WL_RATE_DISABLED;
			}
		}
	}
#endif /* WL11BE */
	return ret;
}
#endif /* WL_RU_NUMRATES */

#if defined(WL_RU_NUMRATES) || defined(WL_NUM_HE_RT)
clm_result_t
clm_he_limit_params_init(clm_he_limit_params_t *params)
{
	if (!params) {
		return CLM_RESULT_ERR;
	}
	params->he_rate_type = WL_HE_RT_SU;
	params->tx_mode = WL_TX_MODE_NONE;
	params->nss = 1u;
	params->chains = 1u;
	params->device_category = CLM_DEVICE_CATEGORY_LEGACY;
	return CLM_RESULT_OK;
}
#endif /* defined(WL_RU_NUMRATES) || defined(WL_NUM_HE_RT) */

#if defined(WL_RU_NUMRATES)
clm_result_t
clm_available_he_limits(const clm_country_locales_t *locales, clm_band_t band,
	clm_bandwidth_t bandwidth, uint32 channel, clm_limits_type_t limits_type,
	clm_available_he_limits_result_t *result)
{
	/* Buffers for all kinds of rates' mass retrieval we may need. Union
	 * because they'll be used sequentiuially (if at all)
	 */
	union {
		clm_power_t ru_limits[WL_RU_NUMRATES];
		clm_power_t su_limits[WL_NUMRATES];
	} limits;
	clm_limits_params_t limit_params;
	clm_result_t ret = CLM_RESULT_NOT_FOUND;
	uint32 rt_idx;
	clm_device_category_t dev_cat;
	if (!result || !locales) {
		return CLM_RESULT_ERR;
	}
	bzero(result, sizeof(*result));
	if (bandwidth != get_bandwidth_traits(bandwidth)->active) {
		return CLM_RESULT_NOT_FOUND;
	}
	for (dev_cat = 0; dev_cat < CLM_DEVICE_CATEGORY_NUM; ++dev_cat) {
		clm_result_t err;
		if (
#if defined(WL_BAND6G)
			(band != CLM_BAND_6G) &&
#endif /* defined(WL_BAND6G) */
			(dev_cat != CLM_DEVICE_CATEGORY_LEGACY))
		{
			continue;
		}
		err = clm_limits_params_init(&limit_params);
		if (err != CLM_RESULT_OK) {
			return err;
		}
		limit_params.bw = bandwidth;
		limit_params.device_category = dev_cat;
		/* First retrieve HE rates - they are part of normal rates */
		err = compute_clm_limits(locales, band, channel, 0, limits_type, &limit_params,
			limits.su_limits, ARRAYSIZE(limits.su_limits), FALSE);
		if (err == CLM_RESULT_ERR) {
			return err;
		}
		if (err == CLM_RESULT_NOT_FOUND) {
			continue;
		}
		ret = CLM_RESULT_OK;
		/* ... and looking for SU limits among them */
		for (rt_idx = 0u; rt_idx < CLM_NUM_RU_RATE_MODES; ++rt_idx) {
			const clm_he_rate_dsc_t *su_rate_dsc = get_tx_mode_info(rt_idx);
			wl_tx_mode_t tx_mode;
			if (limits.su_limits[get_su_base_rate(rt_idx)] ==
				(clm_power_t)WL_RATE_DISABLED)
			{
				continue;
			}
			result->rate_type_mask |= (1u << WL_HE_RT_SU);
			result->nss_mask |= (1u << su_rate_dsc->nss);
			result->chains_mask |= (1u << su_rate_dsc->chains);
			if (su_rate_dsc->flags & CLM_HE_RATE_FLAG_TXBF) {
				tx_mode = WL_TX_MODE_TXBF;
			} else if ((su_rate_dsc->nss == 1u) && (su_rate_dsc->chains > 1u)) {
				tx_mode = WL_TX_MODE_CDD;
			} else {
				tx_mode = WL_TX_MODE_NONE;
			}
			result->tx_mode_mask |= 1u << tx_mode;
		}
		/* Now retrieving RU limits ... */
		err = compute_clm_limits(locales, band, channel, 0, limits_type, &limit_params,
			limits.ru_limits, ARRAYSIZE(limits.ru_limits), TRUE);
		if (err == CLM_RESULT_ERR) {
			return err;
		}
		if (err != CLM_RESULT_OK) {
			return result->rate_type_mask ? CLM_RESULT_OK : CLM_RESULT_NOT_FOUND;
		}
		/* ... and looking which RU are defined */
		for (rt_idx = 0u; rt_idx < WL_RU_NUMRATES; ++rt_idx) {
			const clm_he_rate_dsc_t *he_rate_dsc;
			wl_he_rate_type_t ru_rate_type;
			wl_tx_mode_t tx_mode;
			if (limits.ru_limits[rt_idx] == (clm_power_t)WL_RATE_DISABLED) {
				continue;
			}
			/* Fail on absent HE rate table when there is enabled
			 * HE rate (which means that ClmCompiler has a bug)
			 */
			get_ru_rate_info((clm_ru_rates_t)rt_idx, &he_rate_dsc, &ru_rate_type);
			result->rate_type_mask |= (1u << ru_rate_type);
			result->nss_mask |= (1u << he_rate_dsc->nss);
			result->chains_mask |= (1u << he_rate_dsc->chains);
			if (he_rate_dsc->flags & CLM_HE_RATE_FLAG_TXBF) {
				tx_mode = WL_TX_MODE_TXBF;
			} else if ((he_rate_dsc->nss == 1u) && (he_rate_dsc->chains > 1u)) {
				tx_mode = WL_TX_MODE_CDD;
			} else {
				tx_mode = WL_TX_MODE_NONE;
			}
			result->tx_mode_mask |= 1u << tx_mode;
		}
	}
	return ret;
}

clm_result_t
clm_he_limit(const clm_country_locales_t *locales, clm_band_t band,
	clm_bandwidth_t bandwidth, uint32 channel, int32 ant_gain,
	clm_limits_type_t limits_type, const clm_he_limit_params_t *params,
	clm_he_limit_result_t *result)
{
	union {
		clm_power_t ru_limits[WL_RU_NUMRATES];
		clm_power_t su_limits[WL_NUMRATES];
	} limits;
	clm_limits_params_t limit_params;
	clm_result_t err;
	uint32 rt_idx;
	/* Checking validity of parameters that will not be checked in compute_clm_limits() */
	if (!params || !result ||
		((uint32)params->he_rate_type >= (uint32)WL_NUM_HE_RT) ||
		((uint32)params->chains >= (uint32)(WL_TX_CHAINS_MAX + 1u)) ||
		((uint32)params->nss >= (uint32)(WL_TX_CHAINS_MAX + 1u)) ||
		(params->nss > params->chains) ||
		((uint32)params->tx_mode >= (uint32)WL_NUM_TX_MODES) ||
		(params->tx_mode == WL_TX_MODE_STBC) ||
		((params->chains == 1u) && (params->tx_mode != WL_TX_MODE_NONE)) ||
		((params->tx_mode == WL_TX_MODE_CDD) && (params->nss != 1u)) ||
		((uint32)params->device_category >= (uint32)CLM_DEVICE_CATEGORY_NUM))
	{
		return CLM_RESULT_ERR;
	}
	result->limit = (clm_power_t)WL_RATE_DISABLED;
	err = clm_limits_params_init(&limit_params);
	if (err != CLM_RESULT_OK) {
		return err;
	}
	limit_params.bw = bandwidth;
	limit_params.device_category = params->device_category;
	err = compute_clm_limits(locales, band, channel, ant_gain, limits_type, &limit_params,
		(clm_power_t *)&limits,
		(params->he_rate_type == WL_HE_RT_SU) ? WL_NUMRATES : WL_RU_NUMRATES,
		(params->he_rate_type != WL_HE_RT_SU));
	if (err != CLM_RESULT_OK) {
		return err;
	}
	/* Choosing HE rate descriptor (in he_rate_descriptors[]) that matches
	 * all rate parameters except rate type
	 */
	for (rt_idx = 0u; rt_idx < CLM_NUM_RU_RATE_MODES; ++rt_idx) {
		const clm_he_rate_dsc_t *he_rate_dsc = get_tx_mode_info(rt_idx);
		if ((params->nss == he_rate_dsc->nss) &&
			(params->chains == he_rate_dsc->chains) &&
			((params->tx_mode == WL_TX_MODE_TXBF) ==
			((he_rate_dsc->flags & CLM_HE_RATE_FLAG_TXBF) != 0u)))
		{
			break;
		}
	}
	if (rt_idx == CLM_NUM_RU_RATE_MODES) {
		/* Rate with requested parameters not found - exiting */
		return CLM_RESULT_NOT_FOUND;
	}
	if (params->he_rate_type == WL_HE_RT_SU) {
		/* For SU rates - rate index in limits.su_limits
		 * contained in su_base_rates[]
		 */
		result->limit = limits.su_limits[get_su_base_rate(rt_idx)];
	} else {
		/* For RU rates rate index in limits.ru_limits computed
		 * by combining rate type and index within rate type,
		 * found above
		 */
		result->limit = limits.ru_limits[rt_idx + (params->he_rate_type - 1u) *
			CLM_NUM_RU_RATE_MODES];
	}
	return (result->limit == (clm_power_t)WL_RATE_DISABLED) ? CLM_RESULT_NOT_FOUND :
		CLM_RESULT_OK;
}

clm_result_t
clm_get_ru_rate_params(clm_ru_rates_t ru_rate, clm_he_limit_params_t *params)
{
	const clm_he_rate_dsc_t *he_rate_dsc;
	if (!params || ((uint32)ru_rate >= (uint32)WL_RU_NUMRATES)) {
		return CLM_RESULT_ERR;
	}
	get_ru_rate_info(ru_rate, &he_rate_dsc, &(params->he_rate_type));
	params->chains = (uint32)he_rate_dsc->chains;
	params->nss = (uint32)he_rate_dsc->nss;
	if (he_rate_dsc->flags & CLM_HE_RATE_FLAG_TXBF) {
		params->tx_mode = WL_TX_MODE_TXBF;
	} else if ((he_rate_dsc->nss == 1u) && (he_rate_dsc->chains > 1u)) {
		params->tx_mode = WL_TX_MODE_CDD;
	} else {
		params->tx_mode = WL_TX_MODE_NONE;
	}
	return CLM_RESULT_OK;
}
#elif defined(WL_NUM_HE_RT)

/** Scans TX power limits and calls given callback for HE limits, defined for
 * given channel. Helper function for clm_available_he_limits() and
 * clm_he_limit()
 * \param[in] locales Country (region) locales' information
 * \param[in] band Band of channel in question
 * \param[in] bandwidth Bandwidth of channel in question (of main channel if
 * subchannel is requested). If CLM_BW_NUM then neitehr channel nor bandwidth
 * is checked (i.e. 'fnc' is called for all HE TX power records
 * \param[in] channel Channel number of channel in question (main channel if
 * subchannel is requested). Ignored if 'bandwidth' is CLM_BW_NUM
 * \param[in] limits_type Subchannel in question. Ignored if 'bandwidth' is
 * CLM_BW_NUM
 * \param[in] min_bw
 * \param[in] use_subchan_rules
 * \param[in] ignore_ru_subchannels
 * \param[in] device_category Device category to look in,
 * CLM_DEVICE_CATEGORY_NUM to look in all
 * \param[in] fnc Function to call for TX power record with HE limits of channel
 * in question. Arguments of this function: 'limit' - power limit for current
 * TX record, 'tx_flags' - address of first flag byte of TX records group,
 * 'rates' - sequence of HE rate indices, 'num_rates' - number of rate indices,
 * 'rate_dscs' - array of HE rate descriptors, 'chan_bw' - Bandwidth of channel
 * to which current TX power record belongs to, 'is_main_bw' - TRUE if TX power
 * record related to main channel bandwidth, FALSE if to narrower bandwidth,
 * 'full_coverage' - TRUE if TX record completely covers requested subchannel,
 * 'arg' - argument, passed to scan_he_limits(). Last return value of this
 * function is used as return value of scan_he_limits()
 * \param[in, out] arg - Argument, passed to callback function
 * Returms CLM_RESULT_ERR if some parameters (except channel) were obviously
 * wrong, CLM_RESULT_OK otherwise
 */
static clm_result_t
scan_he_limits(const clm_country_locales_t *locales, clm_band_t band,
	clm_bandwidth_t bandwidth, uint32 channel, clm_limits_type_t limits_type,
	clm_bandwidth_t min_bw, bool use_subchan_rules, bool ignore_ru_subchannels,
	clm_device_category_t device_category,
	void (*fnc) (clm_power_t /* limit */, const uint8 * /* tx_flags */,
		const uint8 * /* rates */, uint32 /* num_rates */,
		const clm_he_rate_dsc_t * /* rate_dscs */, clm_bandwidth_t /* chan_bw */,
		bool /* is_main_bw */, bool /* full_coverage */, void * /* arg */),
	void *arg)
{
	data_dsc_t *ds;
	locale_data_t ht_loc_data; /* Helper information about locale being traversed */
	/* Mapping of bandwidths (from subchannel up to to main channel) to channel numbers */
	uint8 subchannels[CLM_BW_NUM];
	uint8 bw_to_chan[CLM_BW_NUM]; /* actually used subset of 'subchannels' */
	clm_power_t power_inc = 0; /* Power increment from subchannel rule */
	const uint8 *tx_rec; /* Pointer in TX power records' group */
	clm_bandwidth_t subchannel_bw; /* Bandwidth of requested (sub)channel */
	int32 half_subchan_width; /* Half width of requested (sub)channel in channel numbers */
	int32 left_bound, right_bound; /* Bounds of (sub) channel in terms of channel numbers */
	const bandwidth_traits_t *bw_traits;

	/* Parameter sanity check, filling in 'ht_loc_data' */
	if (!locales || !fnc || ((uint32)band >= (uint32)CLM_BAND_NUM) ||
		!is_valid_bandwidth(bandwidth) ||
		((uint32)limits_type >= (uint32)CLM_LIMITS_TYPE_NUM) ||
		!get_loc_def(locales, compose_loc_type[band][BH_HT], &ht_loc_data))
	{
		return CLM_RESULT_ERR;
	}
	ds = get_data_sources((data_source_id_t)(locales->computed_flags & COUNTRY_FLAGS_DS_MASK));
	bandwidth = get_bandwidth_traits(bandwidth)->active;
	bw_traits = get_bandwidth_traits(bandwidth);
	tx_rec = ht_loc_data.tx_limits;
	if (!tx_rec) {
		return CLM_RESULT_NOT_FOUND; /* Locale has no TX power records */
	}
	if (!fill_full_subchan_table(subchannels, channel, bandwidth, limits_type)) {
		/* Filling full subchannel table for given main channel */
		return CLM_RESULT_ERR;
	}
	if (use_subchan_rules) {
		bzero(bw_to_chan, sizeof(bw_to_chan));
		/* Computing 'bw_to_chan' and 'power_inc' */
		if (limits_type == CLM_LIMITS_TYPE_CHANNEL) {
			/* Main channel case - given channel is the only subchannel */
			bw_to_chan[bandwidth] = (uint8)channel;
		} else {
			country_data_v3_t country_data;
			uint32 bb40_idx = get_bb40_idx(band, bandwidth);
			get_country_data(locales, device_category, &country_data);
			if ((bandwidth == CLM_BW_40) &&
				!country_data.sub_chan_channel_rules[bb40_idx].channel_rules)
			{
				/* Default is to use 40 for 20-in-40 */
				bw_to_chan[CLM_BW_40] = (uint8)channel;
			} else if (bw_traits->num_subchannels) {
				fill_actual_subchan_table(bw_to_chan, &power_inc,
					subchannels, limits_type, channel,
					country_data.chan_ranges_band_bw[band][bandwidth],
					&(ds->valid_channel_combs[band][bandwidth]),
					&country_data.sub_chan_channel_rules[bb40_idx],
					country_data.sub_chan_increments[bb40_idx],
					bw_traits->num_subchannels, ds->registry_flags);
			}
		}
	} else {
		(void)memcpy_s(bw_to_chan, sizeof(bw_to_chan), subchannels, sizeof(subchannels));
	}
	subchannel_bw = get_subchan_bw(limits_type, bandwidth);
	half_subchan_width = get_bandwidth_traits(subchannel_bw)->half_stride;
	left_bound = (int32)bw_to_chan[subchannel_bw] - half_subchan_width;
	right_bound = (int32)bw_to_chan[subchannel_bw] + half_subchan_width;
	for (;;) {
		const uint8 *tx_flags = tx_rec;
		uint8 flags = *tx_rec++;
		uint8 flags2 = (flags & CLM_DATA_FLAG_FLAG2) ? *tx_rec++ : 0u;
		uint8 flags3 = (flags2 & CLM_DATA_FLAG2_FLAG3) ? *tx_rec++ : 0u;
		uint32 tx_rec_len = CLM_LOC_DSC_TX_REC_LEN +
			((flags & CLM_DATA_FLAG_PER_ANT_MASK) >> CLM_DATA_FLAG_PER_ANT_SHIFT);
		uint32 num_rec = *tx_rec++;
		clm_bandwidth_t pg_bw = get_tx_rec_bw(flags);
		clm_rate_code_type_t rct = get_rate_code_type(flags2, flags3);
		clm_bandwidth_t outer_bw;
		const clm_channel_comb_set_t *comb_set_for_bw;
		int32 channel_for_bw;
		const clm_channel_range_t *ranges_for_bw;
		clm_device_category_t dc;

		if (pg_bw != CLM_BW_NUM) {
			outer_bw = get_outer_bw(flags2, flags3, pg_bw);
			comb_set_for_bw = ht_loc_data.combs[pg_bw];
			channel_for_bw = (int32)bw_to_chan[pg_bw];
			ranges_for_bw = ht_loc_data.chan_ranges_bw[pg_bw];
		}

		if (device_category != CLM_DEVICE_CATEGORY_NUM) {
			if (!get_dev_cat_reg_dest(flags2, &dc, NULL)) {
				return CLM_RESULT_ERR;
			}
		} else {
			dc = device_category;
		}

		if ((pg_bw == CLM_BW_NUM) ||(flags2 & CLM_DATA_FLAG2_WIDTH_EXT) ||
			(rct < RCT_MIN_RU) ||
			(ignore_ru_subchannels && (flags2 & CLM_DATA_FLAG2_OUTER_BW_MASK)) ||
			(get_outer_bw(flags2, flags3, CLM_BW_20) > bandwidth) ||
			(!channel_for_bw && ((pg_bw >= subchannel_bw) || (pg_bw < min_bw))) ||
			(dc != device_category))
		{
			tx_rec += tx_rec_len * num_rec;
		} else {
			int32 half_tx_width = get_bandwidth_traits(pg_bw)->half_stride;
			int32 min_intersection_width = (half_tx_width < half_subchan_width) ?
				(2 * half_tx_width) : (2 * half_subchan_width);
			for (; num_rec--; tx_rec += tx_rec_len) { /* Loop over TX records group */
				const uint8 *rates; /* Rate indices  for current rate set */
				clm_power_t power; /* Power for current record */
				const clm_channel_range_t *range =
					ranges_for_bw + tx_rec[CLM_LOC_DSC_RANGE_IDX];
				int32 range_end = (pg_bw == CLM_BW_80_80) ? range->start :
					range->end;
				if (!(channel_for_bw
					? channel_in_range(channel_for_bw, range, comb_set_for_bw,
					0u)
					: ranges_intersect(left_bound, right_bound,
					(int32)range->start - half_tx_width,
					range_end + half_tx_width,
					min_intersection_width)))
				{
					continue; /* Channel not in range - skip this record */
				}
				rates = get_byte_string(ht_loc_data.he_rate_sets_bw[pg_bw],
					tx_rec[CLM_LOC_DSC_RATE_IDX],
					ht_loc_data.he_rate_sets_indices_bw[pg_bw]);
				power = (clm_power_t)tx_rec[CLM_LOC_DSC_POWER_IDX];
				if ((uint8)power != (uint8)CLM_DISABLED_POWER) {
					power += power_inc;
				}
				fnc(power, tx_flags, rates + 1u, *rates, ds->he_rate_dscs, outer_bw,
					outer_bw == bandwidth,
					((range->start - half_tx_width) <= left_bound) &&
					((range_end + half_tx_width) >= right_bound), arg);
			}
		}
		if (!(flags & CLM_DATA_FLAG_MORE)) {
			break;
		}
	}
	return CLM_RESULT_OK;
}

/** scan_he_limits() callback for clm_available_he_limits()
 * \param[in] limit HE power limit (ignored)
 * \param[in] tx_flags Flags of group of HE power records (ignored)
 * \param[in] rates HE rate indices
 * \param[in] num_rates Number of HE rate indices
 * \param[in] rate_dscs HE rate descriptors
 * \param[in] chan_bw Bandwidth of channel to which current TX power record
 * belongs to
 * \param[in] is_main_bw TRUE if TX power record related to main channel
 * bandwidth, FALSE if to narrower bandwidth
 * \param[in] full_coverage - TRUE if TX record completely covers requested
 * subchannel
 * \param[in] arg 'result' parameter, passed to clm_available_he_limits()
 * Returns CLM_RESULT_OK
 */
static void
available_he_limits_scan_fnc(clm_power_t limit, const uint8 *tx_flags,
	const uint8 *rates, uint32 num_rates, const clm_he_rate_dsc_t *rate_dscs,
	clm_bandwidth_t chan_bw, bool is_main_bw, bool full_coverage, void *arg)
{
	clm_available_he_limits_result_t *result = (clm_available_he_limits_result_t *)arg;
	wl_he_rate_type_t ru_equivalent = get_bandwidth_traits(chan_bw)->ru_rate_type;
	(void)limit;
	(void)tx_flags;
	(void)full_coverage;
	while (num_rates--) {
		const clm_he_rate_dsc_t *rate_dsc = &rate_dscs[*rates++];
		wl_tx_mode_t tx_mode;
		if (rate_dsc->flags & CLM_HE_RATE_FLAG_TXBF) {
			tx_mode = WL_TX_MODE_TXBF;
		} else if ((rate_dsc->nss == 1u) && (rate_dsc->chains > 1u)) {
			tx_mode = WL_TX_MODE_CDD;
		} else {
			tx_mode = WL_TX_MODE_NONE;
		}
		result->tx_mode_mask |= (1u << tx_mode);
		result->rate_type_mask |= (1u << rate_dsc->rate_type);
		result->nss_mask |= (1u << rate_dsc->nss);
		result->chains_mask |= (1u << rate_dsc->chains);
		if (rate_dsc->rate_type == WL_HE_RT_SU) {
			if (ru_equivalent != WL_NUM_HE_RT) {
				result->rate_type_mask |= (1u << ru_equivalent);
			}
			if (is_main_bw) {
				result->rate_type_mask |= (1u << WL_HE_RT_SU);
			}
		} else {
			result->rate_type_mask |= (1u << rate_dsc->rate_type);
		}
	}
}

clm_result_t
clm_available_he_limits(const clm_country_locales_t *locales, clm_band_t band,
	clm_bandwidth_t bandwidth, uint32 channel, clm_limits_type_t limits_type,
	clm_available_he_limits_result_t *result)
{
	clm_result_t ret;
	if (!result) {
		return CLM_RESULT_ERR;
	}
	bzero(result, sizeof(*result));
	ret = scan_he_limits(locales, band, bandwidth, channel, limits_type,
		CLM_BW_20, FALSE, FALSE, CLM_DEVICE_CATEGORY_NUM,
		available_he_limits_scan_fnc, result);
	if (ret == CLM_RESULT_OK) {
		if (!(result->rate_type_mask & (1u << WL_HE_RT_SU))) {
			bzero(result, sizeof(*result));
			ret = CLM_RESULT_NOT_FOUND;
		} else if (limits_type != CLM_LIMITS_TYPE_CHANNEL) {
			result->rate_type_mask &= ~(1u << WL_HE_RT_UB);
			result->rate_type_mask &= ~(1u << WL_HE_RT_LUB);
		}
	}
	return ret;
}

/** Argument structure for he_limit_scan_fnc */
typedef struct he_limit_scan_arg {
	/** clm_he_limit() 'params' - describes rate to look for */
	const clm_he_limit_params_t *params;
	/** clm_he_limit() result */
	clm_he_limit_result_t *result;
	/* Antenna gain */
	clm_power_t ant_gain;

	/** Ignore coverage and bandwidth, always compute minimum limit */
	bool ignore_coverage;

	/**  Maximum outer bandwidth (CLM spreadsheet page, used for limit
	 * computation - may be different fromchannel range bandwidth for
	 * power, specified for subchannels) used in limit computation so far.
	 * Initialized to -2, set to -1 for RU242-996 limit derived from HE
	 * limit
	 */
	int32 max_outer_bw;

	/** TRUE if at highest outer bandwidth so far the whole (sub)channel
	 * was covered. FALSE means that limit shall be computed as minimum of
	 * all pertinent limits
	 */
	bool full_coverage;

	/** HE limit found on main bandwidth */
	bool he_found;
} he_limit_scan_arg_t;

/** scan_he_limits() callback for clm_he_limit()
 * \param[in] limit HE power limit
 * \param[in] tx_flags Flags of group of HE power records
 * \param[in] rates HE rate indices
 * \param[in] num_rates Number of HE rate indices
 * \param[in] rate_dscs HE rate descriptors
 * \param[in] chan_bw Bandwidth of channel to which current TX power record
 * belongs to
 * \param[in] is_main_bw TRUE if TX power record related to main channel
 * bandwidth, FALSE if to narrower bandwidth
 * \param[in] full_coverage - TRUE if TX record completely covers requested
 * subchannel
 * \param[in] arg Address of he_limit_scan_arg_t structure
 */
static void
he_limit_scan_fnc(clm_power_t limit, const uint8 *tx_flags,
	const uint8 *rates, uint32 num_rates, const clm_he_rate_dsc_t *rate_dscs,
	clm_bandwidth_t chan_bw, bool is_main_bw, bool full_coverage, void *arg)
{
	he_limit_scan_arg_t *hlsa = (he_limit_scan_arg_t *)arg;
	const clm_he_limit_params_t *params = hlsa->params;
	clm_he_limit_result_t *result = hlsa->result;
	const bandwidth_traits_t *bw_traits = get_bandwidth_traits(chan_bw);
	if (((*tx_flags & CLM_DATA_FLAG_MEAS_MASK) == CLM_DATA_FLAG_MEAS_EIRP) &&
		(limit != (clm_power_t)(uint32)CLM_DISABLED_POWER))
	{
		limit -= hlsa->ant_gain;
	}
	while (num_rates--) {
		const clm_he_rate_dsc_t *rate_dsc = &rate_dscs[*rates++];
		wl_he_rate_type_t ru_equivalent = (wl_he_rate_type_t)WL_NUM_HE_RT;
		int32 outer_bw = (int32)chan_bw;
		if (rate_dsc->rate_type == WL_HE_RT_SU) {
			if (is_main_bw) {
				hlsa->he_found = TRUE;
			}
			ru_equivalent = bw_traits->ru_rate_type;
			outer_bw = -1;
		}
		if (((params->he_rate_type == (wl_he_rate_type_t)rate_dsc->rate_type) ||
			(params->he_rate_type == ru_equivalent)) &&
			(params->nss == rate_dsc->nss) && (params->chains == rate_dsc->chains) &&
			((params->tx_mode == WL_TX_MODE_TXBF) ==
			((rate_dsc->flags & CLM_HE_RATE_FLAG_TXBF) != 0)))
		{
			if (!hlsa->ignore_coverage) {
				if (hlsa->full_coverage && (hlsa->max_outer_bw > outer_bw)) {
					continue;
				}
				if ((hlsa->max_outer_bw == outer_bw) ||
					((hlsa->max_outer_bw > outer_bw) && !hlsa->full_coverage))
				{
					full_coverage = FALSE;
				}
			}
			if ((full_coverage && !hlsa->ignore_coverage) ||
				(result->limit == (clm_power_t)(uint32)UNSPECIFIED_POWER) ||
				((result->limit != (clm_power_t)(uint32)CLM_DISABLED_POWER) &&
				(limit < result->limit)))
			{
				result->limit = limit;
			}
			if (!hlsa->ignore_coverage) {
				if (full_coverage || (outer_bw > hlsa->max_outer_bw)) {
					hlsa->max_outer_bw = outer_bw;
				}
				hlsa->full_coverage = full_coverage;
			}
		}
	}
}

clm_result_t
clm_he_limit(const clm_country_locales_t *locales, clm_band_t band,
	clm_bandwidth_t bandwidth, uint32 channel, int32 ant_gain, clm_limits_type_t limits_type,
	const clm_he_limit_params_t *params, clm_he_limit_result_t *result)
{
	he_limit_scan_arg_t arg;
	clm_result_t ret;
	clm_bandwidth_t subchannel_bw, ru_bw;
	bool is_ru_ul;
	if (!params || !result || !is_valid_bandwidth(bandwidth) ||
		((uint32)params->he_rate_type >= (uint32)WL_NUM_HE_RT) ||
		((uint32)(params->chains - 1u) >= (uint32)WL_TX_CHAINS_MAX) ||
		((uint32)(params->nss - 1u) >= (uint32)WL_TX_CHAINS_MAX) ||
		(params->nss > params->chains) ||
		((uint32)params->tx_mode >= (uint32)WL_NUM_TX_MODES) ||
		((uint32)params->device_category >= (uint32)CLM_DEVICE_CATEGORY_NUM))
	{
		return CLM_RESULT_ERR;
	}
	subchannel_bw = get_subchan_bw(limits_type, bandwidth);
	ru_bw = get_min_ru_bw(params->he_rate_type);
	is_ru_ul = (ru_bw != CLM_BW_NUM) && (params->he_rate_type != WL_HE_RT_SU);
	if ((ru_bw == CLM_BW_NUM) ? (limits_type != CLM_LIMITS_TYPE_CHANNEL)
		: (ru_bw > subchannel_bw))
	{
		result->limit = (clm_power_t)WL_RATE_DISABLED;
		return CLM_RESULT_NOT_FOUND;
	}
	arg.params = params;
	arg.result = result;
	arg.ant_gain = (clm_power_t)ant_gain;
	arg.max_outer_bw = -2;
	arg.full_coverage = FALSE;
	arg.ignore_coverage = !is_ru_ul;
	arg.he_found = FALSE;
	result->limit = (clm_power_t)(uint32)UNSPECIFIED_POWER;
	ret = scan_he_limits(locales, band, bandwidth, channel, limits_type,
		is_ru_ul ? ru_bw : subchannel_bw, (params->he_rate_type == WL_HE_RT_SU),
		!is_ru_ul, params->device_category, he_limit_scan_fnc, &arg);
	if ((result->limit == (clm_power_t)(uint32)UNSPECIFIED_POWER) ||
		((params->he_rate_type != WL_HE_RT_SU) && !arg.he_found))
	{
		result->limit = (clm_power_t)WL_RATE_DISABLED;
		ret = CLM_RESULT_NOT_FOUND;
	}
	return ret;
}
#endif /* WL_NUM_HE_RT */

/** Retrieves information about channels with valid power limits for locales of
 * some region
 * \param[in] locales Country (region) locales' information
 * \param[out] valid_channels Valid 5GHz channels
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` is
 * null or contains invalid information, CLM_RESULT_NOT_FOUND if channel has
 * invalid value
 */
clm_result_t
clm_valid_channels_5g(const clm_country_locales_t *locales,
	clm_channels_t *channels20, clm_channels_t *channels4080)
{
	/** Loop variable */
	uint32 base_ht_idx;

	/* Check pointers' validity */
	if (!locales || !channels20 || !channels4080) {
		return CLM_RESULT_ERR;
	}
	/* Clearing output parameters */
	bzero(channels20, sizeof(*channels20));
	bzero(channels4080, sizeof(*channels4080));

	/* For base then HT locales do */
	for (base_ht_idx = 0u; base_ht_idx < BH_NUM; ++base_ht_idx) {
		/** Properties of currently tracversed locale */
		locale_data_t loc_data;

		/** Channel combs for given band - vector indexed by channel
		 * bandwidth
		 */
		const clm_channel_comb_set_t *const* comb_sets;

		/** Transmission power records' sequence for current locale */
		const uint8 *tx_rec;

		/** CLM_DATA_FLAG_ flags for current group */
		uint8 flags, flags2;

		if (!get_loc_def(locales, compose_loc_type[CLM_BAND_5G][base_ht_idx], &loc_data)) {
			return CLM_RESULT_ERR;
		}

		comb_sets = loc_data.combs;
		tx_rec = loc_data.tx_limits;

		/* No transmission power records - nothing to do for this
		 * locale
		 */
		if (!tx_rec) {
			continue;
		}

		/* Loop over all TX records' groups */
		do {
			/** Number of records in group */
			uint32 num_rec;
			/* Bandwidth of records in group */
			clm_bandwidth_t pg_bw;
			/* Channel combs for bandwidth used in group */
			const clm_channel_comb_set_t *comb_set_for_bw;
			/* Length of TX power records in current group */
			uint32 tx_rec_len;
			/* Vector of channel ranges' definition */
			const clm_channel_range_t *ranges;
			clm_channels_t *channels;

			flags = *tx_rec++;
			flags2 = (flags & CLM_DATA_FLAG_FLAG2) ? *tx_rec++ : 0u;
			if (flags2 & CLM_DATA_FLAG2_FLAG3) {
				++tx_rec;
			}
			tx_rec_len = CLM_LOC_DSC_TX_REC_LEN +
				((flags & CLM_DATA_FLAG_PER_ANT_MASK) >>
				CLM_DATA_FLAG_PER_ANT_SHIFT);
			num_rec = *tx_rec++;
			if (flags2 & (CLM_DATA_FLAG2_WIDTH_EXT | CLM_DATA_FLAG2_OUTER_BW_MASK)) {
				tx_rec += num_rec * tx_rec_len;
				continue;
			}
			pg_bw = get_tx_rec_bw(flags);
			if ((pg_bw == CLM_BW_80_80) || (pg_bw == CLM_BW_NUM)) {
				tx_rec += num_rec * tx_rec_len;
				continue;
			}
			if (pg_bw == CLM_BW_20) {
				channels = channels20;
			} else {
				channels = channels4080;
			}

			ranges = loc_data.chan_ranges_bw[pg_bw];
		        /* Fail on absent range table when there is nonempty
			 * limits table for current bandwidth (which means that
			 * ClmCompiler has a bug)
			 */
			ASSERT((ranges != NULL) || (num_rec == 0u));

			/* Loop over all records in group */

			comb_set_for_bw = comb_sets[pg_bw];

			for (; num_rec--; tx_rec += tx_rec_len)
			{

				/* Channel range for current transmission power
				 * record
				 */
				const clm_channel_range_t *range = ranges +
					tx_rec[CLM_LOC_DSC_RANGE_IDX];

				uint32 num_combs;
				const clm_channel_comb_set_t *combs = loc_data.combs[pg_bw];
				const clm_channel_comb_t *comb;
				/* Fail on absent combs table for present
				 * bandwidth (which means that ClmCompiler has
				 * a bug)
				 */
				ASSERT(combs != NULL);
				comb = combs->set;
				/* Fail on NULL pointer to nonempty comb set
				 * (which means that ClmCompiler has a bug)
				 */
				ASSERT((comb != NULL) || (combs->num == 0u));

				/* Check for a non-disabled power before
				 * clearing NO_bw flag
				 */
				if ((uint8)CLM_DISABLED_POWER ==
					(uint8)tx_rec[CLM_LOC_DSC_POWER_IDX])
				{
					continue;
				}

				for (num_combs = comb_set_for_bw->num; num_combs--; ++comb) {
					uint32 chan;
					for (chan = comb->first_channel; chan <= comb->last_channel;
						chan += comb->stride) {
						if (chan && channel_in_range(chan, range,
							comb_set_for_bw, 0u))
						{
							channels->bitvec[chan / 8u] |=
								(uint8)(1u << (chan % 8u));
						}
					}
				}
			}
		} while (flags & CLM_DATA_FLAG_MORE);
	}
	return CLM_RESULT_OK;
}

clm_result_t
clm_channels_params_init(clm_channels_params_t *params)
{
	if (!params) {
		return CLM_RESULT_ERR;
	}
	params->bw = CLM_BW_20;
	params->this_80_80 = 0u;
	params->add = 0;
	params->device_categories = ~0u;
	return CLM_RESULT_OK;
}

/** Adds to given buffer globally valid channels (i.e. channels used in some
 * region) - implementation of clm_valid_channels() for locales == NULL
 * \param[in] band Band of channels
 * \param[in] params Other parameters (bandwidth, etc)
 * \param[in,out] channels Bit vector of channels. This function only adds bits
 * to buffer, caller expected to clear it if needed
 * Returns CLM_RESULT_NOT_FOUND if no channels were found, CLM_RESULT_ERR if
 * something is wrong (e.g. if params->this_80_80 not defined), CLM_OK
 * otherwise
 */
static clm_result_t
get_all_valid_channels(clm_band_t band, const clm_channels_params_t *params,
	clm_channels_t *channels)
{
	data_source_id_t ds_id;
	clm_result_t ret = CLM_RESULT_NOT_FOUND;
	bool invalid_pair = (params->bw == CLM_BW_80_80);
	for (ds_id = (data_source_id_t)0; ds_id < DS_NUM; ++ds_id) {
		const clm_channel_comb_t *comb;
		const clm_channel_comb_set_t *comb_set =
			&get_data_sources(ds_id)->valid_channel_combs[band]
			[get_bandwidth_traits(params->bw)->active];
		for (comb = comb_set->set; comb < comb_set->set + comb_set->num; ++comb) {
			uint32 chan;
			ret = CLM_RESULT_OK;
			for (chan = comb->first_channel; chan <= comb->last_channel;
				chan += comb->stride)
			{
				if (params->bw == CLM_BW_80_80) {
					if (chan == params->this_80_80) {
						invalid_pair = FALSE;
					}
					if ((chan >= (params->this_80_80 - comb->stride)) &&
						(chan <= (params->this_80_80 + comb->stride)))
					{
						continue;
					}
				}
				channels->bitvec[chan / 8u] |= (uint8) (1u << (chan % 8u));
			}
		}
	}
	return invalid_pair ? CLM_RESULT_ERR : ret;
}

clm_result_t
clm_valid_channels(const clm_country_locales_t *locales, clm_band_t band,
	const clm_channels_params_t *params, clm_channels_t *channels)
{
	/** Index for looping over base and HT locales */
	uint32 base_ht_idx;
	/** Return value */
	clm_result_t ret = CLM_RESULT_NOT_FOUND;
#if defined(WL11BE)
	/** Bandwidth traits */
	const bandwidth_traits_t *bandwidth_traits;
#if defined(WL_BAND6G)
	/** Parameters structure to use for punctured channel */
	clm_channels_params_t punctured_params;
#endif /* defined(WL_BAND6G) */
#endif /* defined(WL11BE) */

	/* Check parameters' validity */
	if (!channels || !params ||
		((uint32)band >= (uint32)CLM_BAND_NUM) || !is_valid_bandwidth(params->bw) ||
		((params->this_80_80 != 0u) != (params->bw == CLM_BW_80_80)))
	{
		return CLM_RESULT_ERR;
	}
	if (params->add == 0) {
		/* Clearing output parameters */
		bzero(channels, sizeof(*channels));
	}
#ifdef WL11BE
	bandwidth_traits = get_bandwidth_traits(params->bw);
	if (bandwidth_traits->punctured_base_bw != CLM_BW_NUM) {
#ifdef WL_BAND6G
		if (band != CLM_BAND_6G) {
			/* Punctured bandwidths are 6G-only */
			return CLM_RESULT_NOT_FOUND;
		}
		/* Switching to base bandwidth */
		punctured_params = *params;
		punctured_params.bw = bandwidth_traits->punctured_base_bw;
		params = &punctured_params;
#else /* WL_BAND6G */
		/* Punctured bandwidths are 6G-only */
		return CLM_RESULT_NOT_FOUND;
#endif /* else WL_BAND6G */
	}
#endif /* WL11BE */
	if (!locales) {
		return get_all_valid_channels(band, params, channels);
	}

	/* For base then HT locales do */
	for (base_ht_idx = 0u; base_ht_idx < BH_NUM; ++base_ht_idx) {
		/** Properties of currently traversed locale */
		locale_data_t loc_data;

		/** Channel combs for given band - vector indexed by channel
		 * bandwidth
		 */
		const clm_channel_comb_set_t *const* comb_sets;

		/** Transmission power records' sequence for current locale */
		const uint8 *tx_rec;

		/** CLM_DATA_FLAG_ flags for current group */
		uint8 flags, flags2;

		if (!get_loc_def(locales, compose_loc_type[band][base_ht_idx], &loc_data)) {
			return CLM_RESULT_ERR;
		}
		comb_sets = loc_data.combs;
		tx_rec = loc_data.tx_limits;

		/* No transmission power records - nothing to do for this
		 * locale
		 */
		if (!tx_rec) {
			continue;
		}

		/* Loop over all TX record groups */
		do {
			/** Number of records in group */
			uint32 num_rec;
			/** Bandwidth of records in subsgroupequence */
			clm_bandwidth_t pg_bw;
			/** Channel combs for bandwidth used in group */
			const clm_channel_comb_set_t *comb_set_for_bw;
			/** Length of TX power records in current group */
			uint32 tx_rec_len;
			/** Vector of channel ranges' definition */
			const clm_channel_range_t *ranges;
			/* Device category for record group */
			clm_device_category_t device_category;

			flags = *tx_rec++;
			flags2 = (flags & CLM_DATA_FLAG_FLAG2) ? *tx_rec++ : 0u;
			if (flags2 & CLM_DATA_FLAG2_FLAG3) {
				++tx_rec;
			}
			num_rec = *tx_rec++;
			tx_rec_len = CLM_LOC_DSC_TX_REC_LEN +
				((flags & CLM_DATA_FLAG_PER_ANT_MASK) >>
				CLM_DATA_FLAG_PER_ANT_SHIFT);
			if (flags2 & (CLM_DATA_FLAG2_WIDTH_EXT | CLM_DATA_FLAG2_OUTER_BW_MASK)) {
				tx_rec += num_rec * tx_rec_len;
				continue;
			}
			pg_bw = get_tx_rec_bw(flags);
			/* If not bandwidth we are looking for - skip to next
			 * group of TX records
			 */
			if (pg_bw != params->bw) {
				tx_rec += num_rec * tx_rec_len;
				continue;
			}
			if (!get_dev_cat_reg_dest(flags2, &device_category, NULL)) {
				return CLM_RESULT_ERR;
			}
			if (!((1u << device_category) & params->device_categories)) {
				tx_rec += num_rec * tx_rec_len;
				continue;
			}
			ranges = loc_data.chan_ranges_bw[pg_bw];
			/* Fail on absent range set for nonempty limit table
			 * for current bandwidth (which means that ClmCompiler
			 * has a bug)
			 */
			ASSERT((ranges != NULL) || (num_rec == 0u));
			comb_set_for_bw = comb_sets[pg_bw];
			/* Fail on NULL pointer to nonempty comb set (which
			 * means that ClmCompiler has a bug)
			 */
			ASSERT((comb_set_for_bw->set != NULL) || (comb_set_for_bw->num == 0u));
			/* Loop over all records in subgroup */
			for (; num_rec--; tx_rec += tx_rec_len)
			{
				/** Channel range for current transmission power
				 * record
				 */
				const clm_channel_range_t *range = ranges +
					tx_rec[CLM_LOC_DSC_RANGE_IDX];

				uint32 num_combs;
				const clm_channel_comb_t *comb;

				/* Skip disabled power record */
				if ((uint8)CLM_DISABLED_POWER ==
					(uint8)tx_rec[CLM_LOC_DSC_POWER_IDX])
				{
					continue;
				}
				/* 80+80 ranges are special - they are channel
				 * pairs
				 */
				if (params->this_80_80) {
					if (range->start == params->this_80_80) {
						channels->bitvec[range->end / 8u] |=
							(uint8)(1u << (range->end % 8u));
						ret = CLM_RESULT_OK;
					}
					continue;
				}
				/* Normal enabled range - loop over all
				 * channels in it
				 */
				for (num_combs = comb_set_for_bw->num, comb = comb_set_for_bw->set;
					num_combs--; ++comb)
				{
					uint8 chan;
					if ((range->end < comb->first_channel) ||
						(range->start > comb->last_channel))
					{
						continue;
					}
					for (chan = comb->first_channel;
						chan <= comb->last_channel;
						chan += comb->stride)
					{
						if (channel_in_range(chan, range,
							comb_set_for_bw, 0u))
						{
							channels->bitvec[chan / 8u] |=
								(uint8)(1u << (chan % 8u));
							ret = CLM_RESULT_OK;
						}
					}
				}
			}
		} while (flags & CLM_DATA_FLAG_MORE);
	}
	return ret;
}
#endif /* !WLC_CLMAPI_PRE7 || BCMROMBUILD */

clm_result_t
clm_regulatory_limits_params_init(clm_regulatory_limits_params_t *params)
{
	if (!params) {
		return CLM_RESULT_ERR;
	}
	params->bw = CLM_BW_20;
	params->dest = CLM_REGULATORY_LIMIT_DEST_CLIENT;
	params->limit_type = CLM_REGULATORY_LIMIT_TYPE_CHANNEL;
	params->device_category = CLM_DEVICE_CATEGORY_LEGACY;
	params->ant_gain = 0;
	return CLM_RESULT_OK;
}

clm_result_t
clm_regulatory_limits(const clm_country_locales_t *locales, clm_band_t band,
	const clm_regulatory_limits_params_t *params, clm_regulatory_power_limits_t *limits)
{
	locale_data_t loc_data;
	uint8 flags, flags2;
	const uint8 *record;
	const clm_channel_range_t *ranges;
	const clm_channel_comb_set_t *comb_set;
	bool qdbm = FALSE;
	bool headerless = FALSE;
	clm_result_t ret = CLM_RESULT_NOT_FOUND;
	int8 backoff = 0;
#ifdef WL11BE
	const bandwidth_traits_t *bandwidth_traits;
	clm_regulatory_limits_params_t punctured_params;
#endif /* WL11BE */
	if (!params) {
		return CLM_RESULT_ERR;
	}
	if (!params || !locales || !limits || ((uint32)band >= (uint32)CLM_BAND_NUM) ||
		!is_valid_bandwidth(params->bw) ||
		((uint32)params->dest >= (uint32)CLM_REGULATORY_LIMIT_DEST_NUM) ||
		((uint32)params->device_category >= (uint32)CLM_DEVICE_CATEGORY_NUM) ||
		!get_loc_def(locales, compose_loc_type[band][BH_BASE], &loc_data))
	{
		return CLM_RESULT_ERR;
	}
	memset(limits->limits, (uint8)WL_RATE_DISABLED, sizeof(limits->limits));
#ifdef WL11BE
	bandwidth_traits = get_bandwidth_traits(params->bw);
	if (bandwidth_traits->punctured_base_bw != CLM_BW_NUM) {
#ifdef WL_BAND6G
		if (band != CLM_BAND_6G) {
			/* Punctured bandwidths are 6G-only */
			return CLM_RESULT_NOT_FOUND;
		}
		/* Switching to base bandwidth of punctured channel */
		punctured_params = *params;
		punctured_params.bw = bandwidth_traits->punctured_base_bw;
		params = &punctured_params;
		if (params->limit_type == CLM_REGULATORY_LIMIT_TYPE_CHANNEL) {
			/* Backoff channel power limits */
			backoff = (int8)(bandwidth_traits->punctured_su_backoff + QDBM_IN_DBM - 1) /
				QDBM_IN_DBM;
		}
#else /* WL_BAND6G */
		/* Punctured bandwidths are 6G-only */
		return CLM_RESULT_NOT_FOUND;
#endif /* else WL_BAND6G */
	}
#endif /* WL11BE */
	if (!loc_data.base_hdr) {
		return CLM_RESULT_NOT_FOUND;
	}
	ranges = loc_data.chan_ranges_bw[params->bw];
	comb_set = loc_data.combs[params->bw];
	switch (params->limit_type) {
	case CLM_REGULATORY_LIMIT_TYPE_CHANNEL:
		record = loc_data.reg_limits;
		if (!(loc_data.ds->registry_flags2 & CLM_REGISTRY_FLAG2_DEVICE_CATEGORIES)) {
			headerless = TRUE;
		}
		break;
	case CLM_REGULATORY_LIMIT_TYPE_PSD:
		record = loc_data.psd_limits;
		if (!(loc_data.ds->registry_flags2 & CLM_REGISTRY_FLAG2_PSD_DBM)) {
			qdbm = TRUE;
		}
		break;
	default:
		return CLM_RESULT_ERR;
	}
	if (!record) {
		return CLM_RESULT_NOT_FOUND;
	}
	do {
		uint32 num_rec;
		clm_device_category_t device_category;
		clm_regulatory_limit_dest_t limit_dest;
		clm_power_t ant_gain;

		if (headerless) {
			flags = CLM_DATA_FLAG_WIDTH_20 | CLM_DATA_FLAG_MEAS_EIRP;
			flags2 = 0u;
		} else {
			flags = *record++;
			flags2 = (flags & CLM_DATA_FLAG_FLAG2) ? (*record++) : 0u;
			if (flags2 & CLM_DATA_FLAG2_FLAG3) {
				++record;
			}
		}
		num_rec = *record++;
		if (get_tx_rec_bw(flags) != params->bw) {
			record += CLM_LOC_DSC_REG_REC_LEN * num_rec;
			continue;
		}
		if (!get_dev_cat_reg_dest(flags2, &device_category, &limit_dest)) {
			return CLM_RESULT_ERR;
		}
		if ((device_category != params->device_category) || (limit_dest != params->dest)) {
			record += CLM_LOC_DSC_REG_REC_LEN * num_rec;
			continue;
		}
		ant_gain = ((flags & CLM_DATA_FLAG_MEAS_MASK) == CLM_DATA_FLAG_MEAS_EIRP) ?
			(clm_power_t)params->ant_gain : 0;
		for (; num_rec--; record += CLM_LOC_DSC_REG_REC_LEN)
		{
			/** Channel range for current transmission power
			 * record
			 */
			const clm_channel_range_t *range = ranges + record[CLM_LOC_DSC_RANGE_IDX];

			uint32 num_combs;
			const clm_channel_comb_t *comb;

			clm_power_t limit = (clm_power_t)record[CLM_LOC_DSC_POWER_IDX];

			/* Skip disabled power record */
			if ((uint8)CLM_DISABLED_POWER == (uint8)limit) {
				continue;
			}
			if (qdbm) {
				limit /= QDBM_IN_DBM;
			}
			limit -= ant_gain + backoff;
			/* Normal enabled range - loop over all
			 * channels in it
			 */
			for (num_combs = comb_set->num, comb = comb_set->set; num_combs--; ++comb) {
				uint8 chan;
				if ((range->end < comb->first_channel) ||
					(range->start > comb->last_channel))
				{
					continue;
				}
				for (chan = comb->first_channel; chan <= comb->last_channel;
					chan += comb->stride)
				{
					if (channel_in_range(chan, range, comb_set, 0u)) {
						limits->limits[chan] = limit;
						ret = CLM_RESULT_OK;
					}
				}
			}
		}
	} while (flags & CLM_DATA_FLAG_MORE);
	return ret;
}

clm_result_t
clm_regulatory_limit(const clm_country_locales_t *locales, clm_band_t band, uint32 channel,
	int32 *limit)
{
	clm_result_t ret;
	clm_regulatory_power_limits_t reg_limits;
	clm_regulatory_limits_params_t reg_limits_params;
	if (channel >= (uint32)ARRAYSIZE(reg_limits.limits)) {
		return CLM_RESULT_ERR;
	}
	*limit = WL_RATE_DISABLED;
	(void)clm_regulatory_limits_params_init(&reg_limits_params);
	reg_limits_params.bw = CLM_BW_20;
	reg_limits_params.dest = CLM_REGULATORY_LIMIT_DEST_CLIENT;
	reg_limits_params.device_category = CLM_DEVICE_CATEGORY_LEGACY;
	reg_limits_params.limit_type = CLM_REGULATORY_LIMIT_TYPE_CHANNEL;
	reg_limits_params.ant_gain = 0;
	ret = clm_regulatory_limits(locales, band, &reg_limits_params, &reg_limits);
	if (ret != CLM_RESULT_OK) {
		return ret;
	}
	if ((uint32)reg_limits.limits[channel] == (uint32)WL_RATE_DISABLED) {
		return CLM_RESULT_NOT_FOUND;
	}
	*limit = reg_limits.limits[channel];
	return CLM_RESULT_OK;
}

clm_result_t
clm_psd_limit_params_init(clm_psd_limit_params_t *params)
{
	if (!params) {
		return CLM_RESULT_ERR;
	}
	params->bw = CLM_BW_20;
	params->device_category = CLM_DEVICE_CATEGORY_LEGACY;
	return CLM_RESULT_OK;
}

clm_result_t
clm_psd_limit(const clm_country_locales_t *locales, clm_band_t band, uint32 channel,
	int32 ant_gain, const clm_psd_limit_params_t *params, clm_power_t *psd_limit)
{
	clm_result_t ret;
	clm_regulatory_power_limits_t reg_limits;
	clm_regulatory_limits_params_t reg_limits_params;
	if (!params || !psd_limit || (channel >= (uint32)ARRAYSIZE(reg_limits.limits))) {
		return CLM_RESULT_ERR;
	}
	*psd_limit = (clm_power_t)(uint8)WL_RATE_DISABLED;
	(void)clm_regulatory_limits_params_init(&reg_limits_params);
	reg_limits_params.bw = params->bw;
	reg_limits_params.dest = CLM_REGULATORY_LIMIT_DEST_CLIENT;
	reg_limits_params.device_category = params->device_category;
	reg_limits_params.limit_type = CLM_REGULATORY_LIMIT_TYPE_PSD;
	reg_limits_params.ant_gain = ant_gain;
	ret = clm_regulatory_limits(locales, band, &reg_limits_params, &reg_limits);
	if (ret != CLM_RESULT_OK) {
		return ret;
	}
	if ((uint32)reg_limits.limits[channel] == (uint32)WL_RATE_DISABLED) {
		return CLM_RESULT_NOT_FOUND;
	}
	*psd_limit = reg_limits.limits[channel] * QDBM_IN_DBM;
	return CLM_RESULT_OK;
}

clm_result_t
clm_agg_country_iter(clm_agg_country_t *agg, ccode_t cc, uint32 *rev)
{
	data_source_id_t ds_id;
	uint32 idx;
	clm_result_t ret = CLM_RESULT_OK;
	if (!agg || !cc || !rev) {
		return CLM_RESULT_ERR;
	}
	if (*agg == CLM_ITER_NULL) {
		ds_id = DS_INC;
		idx = 0u;
	} else {
		iter_unpack(*agg, &ds_id, &idx);
		++idx;
	}
	for (;;) {
		int32 num_aggregates;
		aggregate_data_t aggregate_data, inc_aggregate_data;
		if (!get_aggregate_by_idx(ds_id, idx, &aggregate_data, &num_aggregates)) {
			if (ds_id == DS_INC) {
				ds_id = DS_MAIN;
				idx = 0u;
				continue;
			} else {
				ret = CLM_RESULT_NOT_FOUND;
				idx = (num_aggregates >= 0) ? (uint32)num_aggregates : 0u;
				break;
			}
		}
		if (aggregate_data.num_regions == CLM_DELETED_NUM) {
			++idx;
			continue;
		}
		if ((ds_id == DS_MAIN) && get_aggregate(DS_INC,
			&aggregate_data.def_reg, NULL, &inc_aggregate_data)) {
			++idx;
			continue;
		}
		copy_cc(cc, aggregate_data.def_reg.cc);
		*rev = aggregate_data.def_reg.rev;
		break;
	}
	iter_pack(agg, ds_id, idx);
	return ret;
}

clm_result_t
clm_agg_map_iter(const clm_agg_country_t agg, clm_agg_map_t *map, ccode_t cc, uint32 *rev)
{
	data_source_id_t ds_id, mapping_ds_id;
	uint32 agg_idx, mapping_idx;
	aggregate_data_t aggregate_data;
	clm_cc_rev4_t mapping = {"", 0u};

	if (!map || !cc || !rev) {
		return CLM_RESULT_ERR;
	}
	iter_unpack(agg, &ds_id, &agg_idx);
	get_aggregate_by_idx(ds_id, agg_idx, &aggregate_data, NULL);
	if (*map == CLM_ITER_NULL) {
		mapping_idx = 0u;
		mapping_ds_id = ds_id;
	} else {
		iter_unpack(*map, &mapping_ds_id, &mapping_idx);
		++mapping_idx;
	}
	for (;;) {
		aggregate_data_t cur_agg_data;
		bool found = TRUE;
		if (mapping_ds_id == ds_id) {
			cur_agg_data = aggregate_data;
		} else {
			found = get_aggregate(mapping_ds_id, &aggregate_data.def_reg, NULL,
				&cur_agg_data);
		}
		if (found) {
			if (mapping_idx < (uint32)cur_agg_data.num_regions) {
				get_ccrev(mapping_ds_id, &mapping, cur_agg_data.regions,
					mapping_idx);
			} else {
				found = FALSE;
			}
		}
		if (!found) {
			if (mapping_ds_id == DS_MAIN) {
				iter_pack(map, mapping_ds_id, mapping_idx);
				return CLM_RESULT_NOT_FOUND;
			}
			mapping_ds_id = DS_MAIN;
			mapping_idx = 0u;
			continue;
		}
		if (mapping.rev == CLM_DELETED_MAPPING) {
			++mapping_idx;
			continue;
		}
		if ((ds_id == DS_INC) && (mapping_ds_id == DS_MAIN) &&
			get_mapping(DS_INC, &aggregate_data, mapping.cc, NULL))
		{
			++mapping_idx;
			continue;
		}
		copy_cc(cc, mapping.cc);
		*rev = mapping.rev;
		break;
	}
	iter_pack(map, mapping_ds_id, mapping_idx);
	return CLM_RESULT_OK;
}

clm_result_t
clm_agg_country_lookup(const ccode_t cc, uint32 rev, clm_agg_country_t *agg)
{
	uint32 ds_idx;
	clm_cc_rev4_t cc_rev;
	if (!cc || !agg) {
		return CLM_RESULT_ERR;
	}
	copy_cc(cc_rev.cc, cc);
	cc_rev.rev = (regrev_t)rev;
	for (ds_idx = 0u; ds_idx < DS_NUM; ds_idx++) {
		uint32 agg_idx;
		aggregate_data_t agg_data;
		if (get_aggregate((data_source_id_t)ds_idx, &cc_rev, &agg_idx, &agg_data)) {
			if (agg_data.num_regions == CLM_DELETED_NUM) {
				return CLM_RESULT_NOT_FOUND;
			}
			iter_pack(agg, (data_source_id_t)ds_idx, agg_idx);
			return CLM_RESULT_OK;
		}
	}
	return CLM_RESULT_NOT_FOUND;
}

clm_result_t
clm_agg_country_map_lookup(const clm_agg_country_t agg, const ccode_t target_cc, uint32 *rev)
{
	data_source_id_t ds_id;
	uint32 aggregate_idx;
	aggregate_data_t aggregate_data;
	clm_cc_rev4_t mapping;

	if (!target_cc || !rev) {
		return CLM_RESULT_ERR;
	}
	iter_unpack(agg, &ds_id, &aggregate_idx);
	get_aggregate_by_idx(ds_id, aggregate_idx, &aggregate_data, NULL);
	for (;;) {
		if (get_mapping(ds_id, &aggregate_data, target_cc, &mapping)) {
			if (mapping.rev == CLM_DELETED_MAPPING) {
				return CLM_RESULT_NOT_FOUND;
			}
			*rev = mapping.rev;
			return CLM_RESULT_OK;
		}
		if (ds_id == DS_MAIN) {
			return CLM_RESULT_NOT_FOUND;
		}
		ds_id = DS_MAIN;
		if (!get_aggregate(DS_MAIN, &aggregate_data.def_reg, NULL, &aggregate_data)) {
			return CLM_RESULT_NOT_FOUND;
		}
	}
}

const char*
clm_get_base_app_version_string(void)
{
	return clm_get_string(CLM_STRING_TYPE_APPS_VERSION,
		CLM_STRING_SOURCE_BASE);
}

const char*
clm_get_inc_app_version_string(void)
{
	return clm_get_string(CLM_STRING_TYPE_APPS_VERSION,
		CLM_STRING_SOURCE_INCREMENTAL);
}

const char*
clm_get_string(clm_string_type_t string_type,
	clm_string_source_t string_source)
{
	data_source_id_t ds_id =
		((string_source == CLM_STRING_SOURCE_BASE) ||
		((string_source == CLM_STRING_SOURCE_EFFECTIVE) &&
		!get_data_sources(DS_INC)->data))
		? DS_MAIN : DS_INC;
	data_dsc_t *ds = get_data_sources(ds_id);
	const clm_data_header_t *header = ds->header;
	const char* ret = NULL;
	const char* def_value = "";
	/* field_len value shall be long enough to compare with default value.
	 * As default value is "", 1 is enough
	 */
	uint32 field_len = 1u;
	/* NULL if data source is invalid or absent */
	if (((uint32)string_source >= (uint32)CLM_STRING_SOURCE_NUM) || !ds->data) {
		return NULL;
	}
	switch (string_type) {
	case CLM_STRING_TYPE_DATA_VERSION:
		ret = (ds->registry_flags2 & CLM_REGISTRY_FLAG2_DATA_VERSION_STRINGS)
			? (const char *)GET_DATA(ds_id, clm_version) : header->clm_version;
		break;
	case CLM_STRING_TYPE_COMPILER_VERSION:
		ret = header->compiler_version;
		break;
	case CLM_STRING_TYPE_GENERATOR_VERSION:
		ret = header->generator_version;
		break;
	case CLM_STRING_TYPE_APPS_VERSION:
		if (ds->data->flags & CLM_REGISTRY_FLAG_APPS_VERSION) {
			ret = (ds->registry_flags2 & CLM_REGISTRY_FLAG2_DATA_VERSION_STRINGS)
				? (const char *)GET_DATA(ds_id, apps_version)
				: header->apps_version;
			def_value = CLM_APPS_VERSION_NONE_TAG;
			field_len = sizeof(header->apps_version);
		}
		break;
	case CLM_STRING_TYPE_USER_STRING:
		if (ds->data->flags & CLM_REGISTRY_FLAG_USER_STRING) {
			ret = (const char*)relocate_ptr(ds_id, ds->data->user_string);
		}
		break;
	default:
		return NULL;
	}
	/* If string equals default value - will return NULL */
	if (ret && !strncmp(ret, def_value, field_len)) {
		ret = NULL;
	}
	return ret;
}

int32
clm_max_power_shift(void)
{
	uint32 ds_idx;
	int32 ret = 1000; /* Big seed to compute minimum */
	for (ds_idx = 0u; ds_idx < DS_NUM; ++ds_idx) {
		data_dsc_t *ds = get_data_sources(ds_idx);
		if ((ds->registry_flags2 & CLM_REGISTRY_FLAG2_POWER_SHIFT) &&
			(ds->data->tx_power_shift < ret))
		{
			ret = ds->data->tx_power_shift;
		}
	}
	return (ret == 1000) ? 0 : ret;
}
