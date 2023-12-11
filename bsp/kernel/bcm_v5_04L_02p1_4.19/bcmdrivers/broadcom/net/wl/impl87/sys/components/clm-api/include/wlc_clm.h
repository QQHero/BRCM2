/*
 * API for accessing CLM data
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
 * $Id: wlc_clm.h 821810 2020-06-17 12:41:05Z $
 */

#ifndef _WLC_CLM_H_
#define _WLC_CLM_H_

#ifdef _MSC_VER
	#pragma warning(push, 3)
#endif /* _MSC_VER */
#include <bcmwifi_rates.h>
#include <bcmwifi_channels.h>
#ifdef _MSC_VER
	#pragma warning(pop)
#endif /* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************
* API CONSTANTS	*
*****************
*/

/** Module constants */
enum clm_const {
	/** Initial ('before begin') value for iterator. It is guaranteed that
	 * iterator 'pointing' at some valid object it is not equal to this
	 * value
	 */
	CLM_ITER_NULL = 0
};

/** Frequency band identifiers */
typedef enum clm_band {
	/** 2.4HGz band */
	CLM_BAND_2G = 0,

	/** 5GHz band */
	CLM_BAND_5G = 1,

#ifdef WL_BAND6G
	/** 6GHz band */
	CLM_BAND_6G = 2,
#endif /* WL_BAND6G */

	/** Number of band identifiers */
	CLM_BAND_NUM
} clm_band_t;

/** Channel bandwidth identifiers */
typedef enum clm_bandwidth {
	/** 20MHz channel */
	CLM_BW_20	= 0,

	/** 40MHz channel */
	CLM_BW_40	= 1,

	/** 80MHz channel */
	CLM_BW_80	= 2,

	/** 160MHz channel */
	CLM_BW_160	= 3,

	/** 80+80MHz channel */
	CLM_BW_80_80	= 4,
#ifdef WL11BE
	/** 240MHz channel (punctured 6GHz 320MHz) */
	CLM_BW_240	= 5,

	/** 320MHz channel */
	CLM_BW_320	= 6,

	/** 160+160MHz channel */
	CLM_BW_160_160	= 7,

	/** 60MHz channel (punctured 6GHz 80MHz) */
	CLM_BW_60	= 8,

	/** 12MHz channel (punctured 6GHz 160MHz) */
	CLM_BW_120	= 9,

	/** 140MHz channel (punctured 6GHz 160MHz) */
	CLM_BW_140	= 10,

	/** 200MHz channel (punctured 6GHz 320MHz) */
	CLM_BW_200	= 11,

	/** 280MHz channel (punctured 6GHz 320MHz) */
	CLM_BW_280	= 12,
#endif /* WL11BE */
	/** Number of channel bandwidth identifiers */
	CLM_BW_NUM
} clm_bandwidth_t;

/** Return codes for API functions */
typedef enum clm_result {
	/** No error */
	CLM_RESULT_OK		 = 0,

	/** Invalid parameters */
	CLM_RESULT_ERR		 = 1,

	/** Lookup failed (something was not found) */
	CLM_RESULT_NOT_FOUND	= -1
} clm_result_t;

#if defined(WLC_CLMAPI_PRE7) && !defined(BCM4334A0SIM_4334B0) && !defined(BCMROMBUILD)
/** Which 20MHz channel to use as extension in 40MHz operation */
typedef enum clm_ext_chan {
	/** Lower channel is extension, upper is control */
	CLM_EXT_CHAN_LOWER = -1,

	/** Upper channel is extension, lower is control */
	CLM_EXT_CHAN_UPPER =  1,

	/** Neither of the above (use for 20MHz operation) */
	CLM_EXT_CHAN_NONE  =  0
} clm_ext_chan_t;
#endif

/** Flags */
typedef enum clm_flags {
	/* DFS-RELATED COUNTRY (REGION) FLAGS */
	/** Common DFS rules */
	CLM_FLAG_DFS_NONE	= 0x00000000,

	/** EU DFS rules */
	CLM_FLAG_DFS_EU		= 0x00000001,

	/** US (FCC) DFS rules */
	CLM_FLAG_DFS_US		= 0x00000002,

	/** TW DFS rules */
	CLM_FLAG_DFS_TW		= 0x00000003,

	/** UK DFS rules */
	CLM_FLAG_DFS_UK		= 0x00000080,

	/** JP DFS rules */
	CLM_FLAG_DFS_JP		= 0x00000081,

	/** Mask of DFS-related flags */
	CLM_FLAG_DFS_MASK	= 0x00000083,

	/** FiltWAR1 flag from CLM XML */
	CLM_FLAG_FILTWAR1	= 0x00000004,

	/** Beamforming allowed */
	CLM_FLAG_TXBF		= 0x00000008,

	/** Region has per-antenna power targets */
	CLM_FLAG_PER_ANTENNA	= 0x00000010,

	/** Region is EDCRS-EU-compliant */
	CLM_FLAG_EDCRS_EU	= 0x00000040,

	/** Limit peak power during PAPD calibration */
	CLM_FLAG_LO_GAIN_NBCAL	= 0x00000100,

	/** China Spur WAR2 flag from CLM XML */
	CLM_FLAG_CHSPRWAR2	= 0x00000200,

	/** PSD limits present */
	CLM_FLAG_PSD		= 0x00000400,

	/** Region is compliant with 2018 RED (Radio Equipment Directive), that
	 * limits frame burst duration and maybe something else
	 */
	CLM_FLAG_RED_EU		= 0x00000800,

	/** HE limits present */
	CLM_FLAG_HE		= 0x00001000,

	/** Dynamic SAR Averaging with normal averaging window (60 seconds).
	 * Dynamic SAR Averaging allows SAR to be above threshold sometimes, if
	 * in average it is below threshold
	 */
	CLM_FLAG_DSA		= 0x00002000,

	/** Dynamic SAR Averaging with longer averaging window (360 seconds,
	 * used in Canada). Dynamic SAR Averaging allows SAR to be above
	 * threshold sometimes, if in average it is below threshold
	 */
	CLM_FLAG_DSA_2		= 0x00004000,

	/** EHT limits present */
	CLM_FLAG_EHT = 0x00008000,

	/** MRU limits present */
	CLM_FLAG_MRU = 0x00010000,

	/** FCC Contention Based Protocol for incumbent 6GHz devices required */
	CLM_FLAG_CBP_FCC = 0x00020000,

	/* DEBUGGING FLAGS (ALLOCATED AS TOP BITS) */

	/** No 80MHz channels */
	CLM_FLAG_NO_80MHZ	= 0x80000000,

	/** No 40MHz channels */
	CLM_FLAG_NO_40MHZ	= 0x40000000,

	/** No MCS rates */
	CLM_FLAG_NO_MIMO	= 0x20000000,

	/** Has DSSS rates that use EIRP limits */
	CLM_FLAG_HAS_DSSS_EIRP	= 0x10000000,

	/* HAS OFDM RATES THAT USE EIRP LIMITS */
	CLM_FLAG_HAS_OFDM_EIRP	= 0x08000000,

	/** No 160MHz channels */
	CLM_FLAG_NO_160MHZ	= 0x04000000,

	/** No 80+80MHz channels */
	CLM_FLAG_NO_80_80MHZ	= 0x02000000,

	/** No 240MHz channels */
	CLM_FLAG_NO_240MHZ	= 0x01000000u,

	/** No 320MHz channels */
	CLM_FLAG_NO_320MHZ	= 0x00800000u,

	/** No 160+160MHz channels */
	CLM_FLAG_NO_160_160MHZ	= 0x00400000u
} clm_flags_t;

/** Type of limits to output in clm_limits() */
typedef enum clm_limits_type {
	/** Limit for main channel */
	CLM_LIMITS_TYPE_CHANNEL		= 0,

	/** Limit for L-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_L	= 1,

	/** Limit for U-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_U	= 2,

	/** Limit for LL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LL	= 3,

	/** Limit for LU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LU	= 4,

	/** Limit for UL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_UL	= 5,

	/** Limit for UU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_UU	= 6,

	/** Limit for LLL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LLL	= 7,

	/** Limit for LLU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LLU	= 8,

	/** Limit for LUL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LUL	= 9,

	/** Limit for LUU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LUU	= 10,

	/** Limit for ULL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_ULL	= 11,

	/** Limit for ULU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_ULU	= 12,

	/** Limit for UUL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_UUL	= 13,

	/** Limit for UUU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_UUU	= 14,

	/** Limit for LLL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LLLL	= 15,

	/** Limit for LLU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LLLU	= 16,

	/** Limit for LUL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LLUL	= 17,

	/** Limit for LUU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LLUU	= 18,

	/** Limit for ULL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LULL	= 19,

	/** Limit for ULU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LULU	= 20,

	/** Limit for UUL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LUUL	= 21,

	/** Limit for UUU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_LUUU	= 22,

	/** Limit for LLL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_ULLL	= 23,

	/** Limit for LLU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_ULLU	= 24,

	/** Limit for LUL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_ULUL	= 25,

	/** Limit for LUU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_ULUU	= 26,

	/** Limit for ULL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_UULL	= 27,

	/** Limit for ULU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_UULU	= 28,

	/** Limit for UUL-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_UUUL	= 29,

	/** Limit for UUU-subchannel */
	CLM_LIMITS_TYPE_SUBCHAN_UUUU	= 30,

	CLM_LIMITS_TYPE_NUM
} clm_limits_type_t;

/** Strings stored in CLM data */
typedef enum clm_string_type {
	/** CLM data (spreadsheet) version */
	CLM_STRING_TYPE_DATA_VERSION		= 0,

	/** ClmCompiler version */
	CLM_STRING_TYPE_COMPILER_VERSION	= 1,

	/** Name and version of program that generated XML */
	CLM_STRING_TYPE_GENERATOR_VERSION	= 2,

	/** Engineering version of CLM data */
	CLM_STRING_TYPE_APPS_VERSION		= 3,

	/** User string */
	CLM_STRING_TYPE_USER_STRING		= 4,

	/** Number of enum members */
	CLM_STRING_TYPE_NUM
} clm_string_type_t;

/** Source (location) of string stored in CLM data */
typedef enum clm_string_source {
	/** String stored in base data */
	CLM_STRING_SOURCE_BASE		= 0,

	/** String stored in incremental data */
	CLM_STRING_SOURCE_INCREMENTAL	= 1,

	/** If incremental data present - string stored in it otherwise in base
	 * data
	 */
	CLM_STRING_SOURCE_EFFECTIVE	= 2,

	/** Number of enum members */
	CLM_STRING_SOURCE_NUM
} clm_string_source_t;

/** Device category */
typedef enum clm_device_category {
	/** Low power */
	CLM_DEVICE_CATEGORY_LP	= 0,

	/** VLP Power */
	CLM_DEVICE_CATEGORY_VLP	= 1,

	/** SP Power */
	CLM_DEVICE_CATEGORY_SP	= 2,

	/** Number of device categories */
	CLM_DEVICE_CATEGORY_NUM,

	/** Legacy device category - value to use for non-6GHz bands */
	CLM_DEVICE_CATEGORY_LEGACY = CLM_DEVICE_CATEGORY_LP
} clm_device_category_t;

/** Power types (operation modes) */
typedef enum clm_regulatory_limit_type {
	/** Whole-channel power limit */
	CLM_REGULATORY_LIMIT_TYPE_CHANNEL	= 0,

	/** PSD limit */
	CLM_REGULATORY_LIMIT_TYPE_PSD		= 1,

	/** Number of power limits */
	CLM_REGULATORY_LIMIT_TYPE_NUM
} clm_regulatory_limit_type_t;

/** Regulatory limit destination */
typedef enum clm_regulatory_limit_dest {
	/** Regulatory limit for client stations */
	CLM_REGULATORY_LIMIT_DEST_CLIENT	= 0,

	/** Regulatory limit for local use (may be higher than client for
	 * LP AP)
	 */
	CLM_REGULATORY_LIMIT_DEST_LOCAL		= 1,

	/** Regulatory limit for local use subordinate devices */
	CLM_REGULATORY_LIMIT_DEST_SUBORDINATE	= 2,

	/** Number of regulatory limit roles */
	CLM_REGULATORY_LIMIT_DEST_NUM,

	/** Legacy regulatory limit destination */
	CLM_REGULATORY_LIMIT_DEST_LEGACY = CLM_REGULATORY_LIMIT_DEST_CLIENT
} clm_regulatory_limit_dest_t;

/** Rules for using 6GHz LP power in presumably VLP places */
typedef enum clm_c2c {
	/** No rules */
	CLM_C2C_NONE = 0,

	/** EU rules */
	CLM_C2C_EU = 1,

	/** US rules */
	CLM_C2C_US = 2,

	/** Number of currently supported rules */
	CLM_C2C_NUM
} clm_c2c_t;

/*****************
* API DATA TYPES *
******************
*/

/** Country Code: a two byte code, usually ASCII
 * Note that 'worldwide' country code is now "ww", not "\0\0"
 */
typedef char ccode_t[2];

/** Channel set */
typedef struct clm_channels {
	/** Bit vector, indexed by channel numbers */
	uint8 bitvec[(MAXCHANNEL + 7) / 8];
} clm_channels_t;

/** Power in quarter of dBm units */
typedef int8 clm_power_t;

/** Per-TX-rate power limits */
typedef struct clm_power_limits {
	/** Per-rate limits (WL_RATE_DISABLED for disabled rates) */
	clm_power_t limit[WL_NUMRATES];
} clm_power_limits_t;

#ifdef WL_RU_NUMRATES
/** Per-OFDMA-rate-group power limits */
typedef struct clm_ru_power_limits {
	/** Per-rate-group limits (WL_RATE_DISABLED for disabled rates) */
	clm_power_t limit[WL_RU_NUMRATES];
} clm_ru_power_limits_t;
#endif /* WL_RU_NUMRATES */

/* ITERATORS - TOKENS THAT REPRESENT VARIOUS ITEMS IN THE CLM */

/** Country (region) definition */
typedef int32 clm_country_t;

/** Locale definition */
typedef int32 clm_locale_t;

/** Aggregate definition */
typedef int32 clm_agg_country_t;

/** Definition of mapping inside aggregation */
typedef int32 clm_agg_map_t;

/** Locales (transmission rules) for a country (region) */
typedef struct clm_country_locales {
	/** Pointer to 2.4GHz base locale */
	const uint8 *locale_2G;

	/** Pointer to 5GHz base locale */
	const uint8 *locale_5G;

	/** Pointer to 2.4GHz HT locale */
	const uint8 *locale_2G_HT;

	/** Pointer to 5GHz HT locale */
	const uint8 *locale_5G_HT;

	/** Flags from country record */
	uint8 country_flags;

	/** Computed country flags */
	uint8 computed_flags;

	/** Second byte of flags from country record */
	uint8 country_flags_2;

	/** Third byte of flags from country record */
	uint8 country_flags_3;

	/** Bitmask, ordered by CLM_LOC_IDX_... constants, with '1' for
	 * locales, contained in main (base) data source, '0' in incremental
	 * data source
	 */
	uint32 main_loc_data_bitmask;

#ifdef WL_BAND6G
	/** Pointer to 6GHz base locale (802.11a SISO) */
	const uint8 *locale_6G;

	/** Pointer to 6GHz HT locale */
	const uint8 *locale_6G_HT;
#endif /* WL_BAND6G */
} clm_country_locales_t;

/** Parameters that refine clm_limits() output data
 * To use this structure one shall use clm_limits_params_init() to reset
 * parameters from this structure to default state, then change only parameters
 * relevant to task, leaving all others in default states
 */
typedef struct clm_limits_params {
	/** Channel bandwidth. Default is CLM_BW_20 (20MHz) */
	clm_bandwidth_t bw;

	/** SAR limit in quarter dBm. Default is 0x7F (no SAR limit) */
	int32 sar;

	/** 0-based antenna index (0 .. WL_TX_CHAINS_MAX-1). This parameter
	 * only affects rates for which per-antenna power limits specified.
	 * If 2 limits specified - they'll be returned for antenna indices 0
	 * and 1, WL_RATE_DISABLED will be returned for antenna index 3. If 3
	 * limits specified - they'll be returned for corresponded antenna
	 * indices. Default is 0
	 */
	int32 antenna_idx;

	/** For 80+80 channel it is other channel in pair (not one for which
	 * power limit is requested)
	 */
	uint32 other_80_80_chan;

	/** Device category */
	clm_device_category_t device_category;

	/** Constant to add to powers (other than WL_RATE_DISABLED) to compute
	 * true power. For use in situations when otherwise resulted power will
	 * not fit [-31.5, 31.75] range. Expressed in qdB units.
	 */
	int32 power_shift;

	/** CLM_BW_NUM or bandwidth for punctured subchannel. Ignored if
	 * limits_type is CLM_LIMITS_TYPE_CHANNEL. If limits_type is not
	 * CLM_LIMITS_TYPE_CHANNEL then value should be either CLM_BW_NUM
	 * (means subchannel is not punctured) or it should not contradict
	 * limits_type.
	 * E.g. if bw is CLM_BW_280 and limits_type is CLM_LIMITS_TYPE_SUBCHAN_L,
	 * then punctured_sub_chan_bw may be CLM_BW_NUM (means nonpunctured
	 * CLM_BW_160), CLM_BW_160, CLM_BW_140, CLM_BW_120. But it can't be
	 * CLM_BW_200 or CLM_BW_60, because this does not agree with
	 * limits_type of CLM_LIMITS_TYPE_SUBCHAN_L.
	 * Default is CLM_BW_NUM
	 */
	clm_bandwidth_t punctured_subchan_bw;
} clm_limits_params_t;

/** Input parameters for clm_valid_channels() */
typedef struct clm_channels_params {
	/** Bandwidth of channels to look for. Default is CLM_BW_20 */
	clm_bandwidth_t bw;

	/** If nonzero - one channel in 80+80 channel pair, in this case
	 * function returns other 80MHz channels that may be used in pair with
	 * it
	 */
	uint32 this_80_80;

	/** If nonzero - clm_valid_channels will skip clearing channel vector clm_channels_t
	 * and adds valid CLM channels for given bandwidth bw
	 */
	int32 add;

	/** Bitmask of device categories - combination of (1 << CLM_DEVICE_CATEGORY_...) values.
	 * Validity not checked, i.e. ~0u is allowed (and is default)
	 */
	uint32 device_categories;
} clm_channels_params_t;

/** Input parameters for clm_psd_limit() */
typedef struct clm_psd_limit_params {
	/** Bandwidth of channels to look for. Default is CLM_BW_20 */
	clm_bandwidth_t bw;

	/** Device category */
	clm_device_category_t device_category;
} clm_psd_limit_params_t;

#if defined(WL_RU_NUMRATES) || defined(WL_NUM_HE_RT)
/** Result struct for clm_available_he_limits() */
typedef struct clm_available_he_limits_result {
	/** Bitmask, corresponded to available members of wl_he_rate_type enum */
	uint32 rate_type_mask;

	/** Bitmask, corresponded to available members of wl_tx_nss enum */
	uint32 nss_mask;

	/** Bitmask, corresponded to available members of wl_tx_chains enum */
	uint32 chains_mask;

	/** Bitmask, corresponded to available members of wl_tx_mode enum */
	uint32 tx_mode_mask;
} clm_available_he_limits_result_t;

/** Additional parameters for clm_he_limit() */
typedef struct clm_he_limit_params {
	/** Desired rate type */
	wl_he_rate_type_t he_rate_type;

	/** Desired TX mode */
	wl_tx_mode_t tx_mode;

	/** Desired number of spatial streams */
	uint32 nss;

	/** Desired number of TX chains */
	uint32 chains;

	/** Device category */
	clm_device_category_t device_category;
} clm_he_limit_params_t;

/** Result structure for clm_he_limit() */
typedef struct clm_he_limit_result {
	/** Requested HE limit */
	clm_power_t limit;
} clm_he_limit_result_t;

#endif /* defined(WL_RU_NUMRATES) || defined(WL_NUM_HE_RT) */

/** Country (CLM Region) information */
typedef struct clm_country_info {
	/** Country flags */
	uint32 flags;

	/** Supported device categories - bitmask of
	 * (1 << CLM_DEVICE_CATEGORY_...) values
	 */
	uint32 device_categories;

	/** Supported regulatory limit destinations - bitmask of
	 * (1 << CLM_REGULATORY_LIMIT_DEST_...) values
	 */
	uint32 reg_limit_destinations;

	/** Supported regulatory limit types - bitmask of
	 * (1 << CLM_REGULATORY_LIMIT_TYPE_...) values
	 */
	uint32 reg_limit_types;

	/** C2C rules for 6GHz LP operation */
	clm_c2c_t c2c;
} clm_country_info_t;

/** Additional parameters for clm_regulatory_limits */
typedef struct clm_regulatory_limits_params {
	/** Channel bandwidth */
	clm_bandwidth_t bw;

	/** Limit type (whole channel, PSD, etc.) */
	clm_regulatory_limit_type_t limit_type;

	/** Device category */
	clm_device_category_t device_category;

	/** Regulatory limit destination */
	clm_regulatory_limit_dest_t dest;

	/** Antenna gain - might be needed for retrieval of local limits */
	int32 ant_gain;
} clm_regulatory_limits_params_t;

/** Regulatory power limits */
typedef struct clm_regulatory_power_limits {
	/** Per-channel regulatory limits of requested type. Unit is dBm or
	 * dBm/MHz (whole dBs, not qdBs)
	 */
	int8 limits[MAXCHANNEL];
} clm_regulatory_power_limits_t;

/* forward declaration for CLM header data structure used in clm_init()
 * struct clm_data_header is defined in clm_data.h
 */
struct clm_data_header;

/***************
* API ROUTINES *
****************
*/

/** Provides main CLM data to the CLM access API
 * Must be called before any access APIs are used
 * \param[in] data Header of main CLM data. Only one main CLM data source may
 * be set
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if given address is
 * nonzero and CLM data tag is absent at given address or major number of CLM
 * data format version is not supported by CLM API
 */
extern clm_result_t clm_init(const struct clm_data_header *data);

/** Provides incremental CLM data to the CLM access API
 * This call is optional
 * \param[in] data Header of incremental CLM data. No more than one incremental
 * CLM data source may be set
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if given address is
 * non zero and CLM data tag is absent at given address or major number of CLM
 * data format version is not supported by CLM API
 */
extern clm_result_t clm_set_inc_data(const struct clm_data_header *data);

/** Initializes iterator before iteration via of clm_..._iter()
 * May be done manually - by assigning CLM_ITER_NULL
 * \param[out] iter Iterator to be initialized (type is clm_country_t,
 * clm_locale_t, clm_agg_country_t, clm_agg_map_t)
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if null pointer
 * passed
 */
extern clm_result_t clm_iter_init(int32 *iter);

/** Resets given clm_limits() parameters structure to defaults
 * \param[out] params Address of parameters' structure to reset
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if null pointer
 * passed
 */
extern clm_result_t clm_limits_params_init(clm_limits_params_t *params);

/** Performs one iteration step over set of countries (regions)
 * Looks up first/next country (region)
 * \param[in,out] country Iterator token. Shall be initialized with
 * clm_iter_init() before iteration begin. After successful call iterator token
 * 'points' to same region as returned cc/rev
 * \param[out] cc Country (region) code
 * \param[out] rev Country (region) revision
 * \return CLM_RESULT_OK if first/next country was found, CLM_RESULT_ERR if any
 * of passed pointers was null, CLM_RESULT_NOT_FOUND if first/next country was
 * not found (iteration completed)
 */
extern clm_result_t clm_country_iter(clm_country_t *country, ccode_t cc,
	uint32 *rev);

/** Looks up for country (region) with given country code and revision
 * \param[in] cc Country code to look for
 * \param[in] rev Country (region) revision to look for
 * \param[out] country Iterator that 'points' to found country
 * \return CLM_RESULT_OK if country was found, CLM_RESULT_ERR if any of passed
 * pointers was null, CLM_RESULT_NOT_FOUND if required country was not found
 */
extern clm_result_t clm_country_lookup(const ccode_t cc, uint32 rev,
	clm_country_t *country);

/** Retrieves locale definitions of given country (regions)
 * \param[in] country Iterator that 'points' to country (region) information
 * \param[out] locales Locales' information for given country
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales`
 * pointer is null, CLM_RESULT_NOT_FOUND if given country iterator does not
 * point to valid country (region) definition
 */
extern clm_result_t clm_country_def(const clm_country_t country,
	clm_country_locales_t *locales);

/** Retrieves information about valid and restricted channels for locales of
 * some region
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[out] valid_channels Valid 20MHz channels (optional parameter)
 * \param[out] restricted_channels Restricted channels (optional parameter)
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales`
 * pointer is null, or band ID is invalid or `locales` contents is invalid
 */
extern clm_result_t clm_country_channels(const clm_country_locales_t *locales,
	clm_band_t band, clm_channels_t *valid_channels,
	clm_channels_t *restricted_channels);

/** Retrieves flags associated with given country (region) for given band
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[out] flags Flags associated with given country (region) for given
 * band
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` or
 * `flags` pointer is null, or band ID is invalid or `locales` contents is
 * invalid
 *
 * This API is being deprecated in favor of clm_country_information()
 */
extern clm_result_t clm_country_flags(const clm_country_locales_t *locales,
	clm_band_t band, unsigned long *flags);

/** Retrieves country (region) information for given band
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[out] info Buffer for country information
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` or
 * `info` pointer is null, or band ID is invalid or `locales` contents is
 * invalid
 */
extern clm_result_t clm_country_information(const clm_country_locales_t *locales,
	clm_band_t band, clm_country_info_t *info);

/** Retrieves advertised country code for country (region) pointed by given
 * iterator
 * \param[in] country Iterator that points to country (region) in question
 * \param[out] advertised_cc Advertised CC for given region
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `country` or
 * `advertised_cc` is null, or if `country` not `points` t a valid country
 * (region) definition
 */
extern clm_result_t clm_country_advertised_cc(const clm_country_t country,
	ccode_t advertised_cc);

#if defined(WLC_CLMAPI_PRE7) && !defined(BCM4334A0SIM_4334B0) && !defined(BCMROMBUILD)
/* This version required for ROM compatibility */
extern clm_result_t clm_limits(const clm_country_locales_t *locales,
	clm_band_t band, uint32 channel, clm_bandwidth_t bw, int32 ant_gain,
	int32 sar, clm_ext_chan_t extension_channel, clm_power_limits_t *limits,
	clm_power_limits_t *bw20in40_limits);
#else
/** Retrieves the power limits on the given band/(sub)channel/bandwidth using
 * the given antenna gain
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[in] channel Channel number (main channel if subchannel limits output
 * is required)
 * \param[in] ant_gain Antenna gain in quarter dBm (used if limit is given in
 * EIRP terms)
 * \param[in] limits_type Subchannel to get limits for
 * \param[in] params Other parameters
 * \param[out] limits Limits for given above parameters
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` is
 * null or contains invalid information, or if any other input parameter
 * (except channel) has invalid value, CLM_RESULT_NOT_FOUND if channel has
 * invalid value
 */
extern clm_result_t clm_limits(const clm_country_locales_t *locales,
	clm_band_t band, uint32 channel, int32 ant_gain, clm_limits_type_t limits_type,
	const clm_limits_params_t *params, clm_power_limits_t *limits);

#ifdef WL_RU_NUMRATES
/** Retrieves OFDMA power limits on the given band/(sub)channel/bandwidth using
 * the given antenna gain
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[in] channel Channel number (main channel if subchannel limits output
 * is required)
 * \param[in] ant_gain Antenna gain in quarter dBm (used if limit is given in
 * EIRP terms)
 * \param[in] limits_type Subchannel to get limits for
 * \param[in] params Other parameters
 * \param[out] limits Limits for given above parameters
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` is
 * null or contains invalid information, or if any other input parameter
 * (except channel) has invalid value, CLM_RESULT_NOT_FOUND if channel has
 * invalid value or has no OFDMA limits
 */
extern clm_result_t clm_ru_limits(const clm_country_locales_t *locales,
	clm_band_t band, uint32 channel, int32 ant_gain, clm_limits_type_t limits_type,
	const clm_limits_params_t *params, clm_ru_power_limits_t *limits);

/** Returns attributes of given RU rates in form of clm_he_limit_params_t
 * May be used fo rbridging clm_he_limit()->clm_ru_limits() transition
 * \param[in] ru_rate Rate go get parameters for
 * \param[out] params Structure for attributes of given rate
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if rate code is out
 * of range or 'params' is NULL
 */
extern clm_result_t clm_get_ru_rate_params(clm_ru_rates_t ru_rate,
	clm_he_limit_params_t *params);
#endif /* WL_RU_NUMRATES */

/** Retrieves information about channels with valid power limits for locales of
 * some region. This function is deprecated. It is being superseded by
 * clm_valid_channels()
 * \param[in] locales Country (region) locales' information
 * \param[out] valid_channels Valid 5GHz channels
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` is
 * null or contains invalid information, CLM_RESULT_NOT_FOUND if channel has
 * invalid value
 */
extern clm_result_t clm_valid_channels_5g(const clm_country_locales_t *locales,
	clm_channels_t *channels20, clm_channels_t *channels4080);

/** Resets given clm_valid_channels() parameters structure to defaults
 * \param[out] params Address of parameters' structure to reset
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if null pointer
 * passed
 */
extern clm_result_t clm_channels_params_init(clm_channels_params_t *params);

/** Retrieves information about certain channels with valid power limits for
 * locales of some region
 * \param[in] locales Country (region) locales' information. NULL means that
 * all channels from all countries should be retrieved
 * \param[in] band Band of channels being requested
 * \param[in] params Other parameters of channels being requested
 * \param[out] channels Country's (region's) channels that match given criteria
 * \return CLM_RESULT_OK if some channels were found, CLM_RESULT_NOT_FOUND if
 * no matching channels were found, CLM_RESULT_ERR if parameters has invalid
 * values
 */
extern clm_result_t clm_valid_channels(const clm_country_locales_t *locales,
	clm_band_t band, const clm_channels_params_t *params,
	clm_channels_t *channels);
#endif /* WLC_CLMAPI_PRE7 && !BCM4334A0SIM_4334B0 && !BCMROMBUILD */

/** Retrieves maximum regulatory power for given channel
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[in] channel Channel number
 * \param[out] limit Regulatory power limit in dBm (!NOT qdBm!)
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` is
 * null or contains invalid information, if `limit` is null or if any other
 * input parameter (except channel) has invalid value, CLM_RESULT_NOT_FOUND if
 * regulatory power limit not defined for given channel
 *
 * This API is being deprecated in favor of clm_regulatory_limits()
 */
extern clm_result_t clm_regulatory_limit(const clm_country_locales_t *locales,
	clm_band_t band, uint32 channel, int32 *limit);

/** Resets given clm_regulatory_limits() parameters structure to defaults
 * \param[out] params Address of parameters' structure to reset
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if null pointer
 * passed
 */
extern clm_result_t clm_regulatory_limits_params_init(
	clm_regulatory_limits_params_t *params);

/** Retrieves regulatory limits for all channels of given given country
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[in] params Additional parameters
 * \param[out] limits Resulted limits
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if 'locales',
 * 'params' or limits is null, or if 'locales', 'bans' or 'params' contains
 * invalid information, if `limit` is null or if any other input parameter
 * (except channel) has invalid value, CLM_RESULT_NOT_FOUND if no results were
 * found
 */
extern clm_result_t clm_regulatory_limits(const clm_country_locales_t *locales,
	clm_band_t band, const clm_regulatory_limits_params_t *params,
	clm_regulatory_power_limits_t *limits);

/** Resets given clm_psd_limit() parameters structure to defaults
* \param[out] params Address of parameters' structure to reset
* \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if null pointer
* passed
*/
extern clm_result_t clm_psd_limit_params_init(clm_psd_limit_params_t *params);

/** Retrieves PSD limit for given channel
 * \param[in] locales Country (region) locales' information
 * \param[in] band Frequency band
 * \param[in] channel Channel number
 * \param[in] ant_gain Antenna gain in quarters of dBm. Used if PSD limit
 * specified as EIRP
 * \param[in] params Other parameters of limit being requested
 * \param[out] psd_limit PSD limit in qdBm/MHz
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if `locales` is
 * null or contains invalid information, if `psd_limit` is null or if any other
 * input parameter (except channel) has invalid value, CLM_RESULT_NOT_FOUND if
 * PSD limit not defined for given channel
 *
 * This API is re-deprecated. This time in favor of clm_regulatory_limits
 */
extern clm_result_t clm_psd_limit(const clm_country_locales_t *locales,
	clm_band_t band, uint32 channel, int32 ant_gain,
	const clm_psd_limit_params_t *params, clm_power_t *psd_limit);

/* Temporary alias for smoot transition to new signature. Will be removed */
#define clm_psd_limit_new clm_psd_limit

#if defined(WL_RU_NUMRATES) || defined(WL_NUM_HE_RT)
/** Determines what kinds of HE limits available for given channel
 * \param[in] locales Country (region) locales' information
 * \param[in] band Band of requested channel
 * \param[in] bandwidth Channel bandwidth (main channel bandwidth if
 * subchannel is requested)
 * \param[in] channel Channel number (main channel number if subchannel is
 * requested)
 * \param[in] limits_type Subchannel to get limits for
 * \param[out] result Structure, containing information about available HE
 * limits
 * \return CLM_RESULT_OK if some kind of HE limits was found,
 * CLM_RESULT_NOT_FOUND if no HE limits was found, CLM_RESULT_ERR if some
 * pointer is null or some other parameter is out of range
 */
extern clm_result_t clm_available_he_limits(const clm_country_locales_t *locales,
	clm_band_t band, clm_bandwidth_t bandwidth, uint32 channel,
	clm_limits_type_t limits_type, clm_available_he_limits_result_t *result);

/** Resets given clm_he_limit() parameters structure to defaults
 * \param[out] params Address of parameters' structure to reset
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if null pointer
 * passed
 */
extern clm_result_t clm_he_limit_params_init(clm_he_limit_params_t *params);

/** Retrieves HE power limit of given type
 * \param[in] locales Country (region) locales' information
 * \param[in] band Channel band
 * \param[in] bandwidth Channel bandwidth (main channel bandwidth if
 * subchannel is requested)
 * \param[in] channel Channel number (main channel if subchannel limit is
 * required)
 * \param[in] ant_gain Antenna gain in quarter dBm (used if limit is given in
 * EIRP terms)
 * \param[in] limits_type Subchannel to get limits for
 * \param[in] params Other parameters
 * \param[out] limits Limit for given above parameters
 * \return CLM_RESULT_OK in case of success, CLM_RESULT_ERR if some parameter
 * is null or out of range, CLM_RESULT_NOT_FOUND if requested limit not found
 */
extern clm_result_t clm_he_limit(const clm_country_locales_t *locales,
	clm_band_t band, clm_bandwidth_t bandwidth, uint32 channel, int32 ant_gain,
	clm_limits_type_t limits_type, const clm_he_limit_params_t *params,
	clm_he_limit_result_t *result);
#endif /* defined(WL_RU_NUMRATES) || defined(WL_NUM_HE_RT) */

/** Performs one iteration step over set of aggregations. Looks up first/next
 * aggregation
 * \param[in,out] agg Iterator token. Shall be initialized with clm_iter_init()
 * before iteration begin. After successful call iterator token 'points' to
 * same aggregation as returned cc/rev
 * \param[out] cc Aggregation's default country (region) code
 * \param[out] rev Aggregation's default country (region) revision
 * \return CLM_RESULT_OK if first/next aggregation was found, CLM_RESULT_ERR if
 * any of passed pointers was null, CLM_RESULT_NOT_FOUND if first/next
 * aggregation was not found (iteration completed)
 */
extern clm_result_t clm_agg_country_iter(clm_agg_country_t *agg, ccode_t cc,
	uint32 *rev);

/** Performs one iteration step over sequence of aggregation's mappings
 * Looks up first/next mapping
 * \param[in] agg Aggregation whose mappings are being iterated
 * \param[in,out] map Iterator token. Shall be initialized with clm_iter_init()
 * before iteration begin. After successful call iterator token 'points' to
 * same mapping as returned cc/rev
 * \param[out] cc Mapping's country code
 * \param[out] rev Mapping's region revision
 * \return CLM_RESULT_OK if first/next mapping was found, CLM_RESULT_ERR if any
 * of passed pointers was null or if aggregation iterator does not 'point' to
 * valid aggregation, CLM_RESULT_NOT_FOUND if first/next mapping was not found
 * (iteration completed)
 */
extern clm_result_t clm_agg_map_iter(const clm_agg_country_t agg,
	clm_agg_map_t *map, ccode_t cc, uint32 *rev);

/** Looks up for aggregation with given default country code and revision
 * \param[in] cc Default country code of aggregation being looked for
 * \param[in] rev Default region revision of aggregation being looked for
 * \param[out] agg Iterator that 'points' to found aggregation
 * \return CLM_RESULT_OK if aggregation was found, CLM_RESULT_ERR if any of
 * passed pointers was null, CLM_RESULT_NOT_FOUND if required aggregation was
 * not found
 */
extern clm_result_t clm_agg_country_lookup(const ccode_t cc, uint32 rev,
	clm_agg_country_t *agg);

/** Looks up for mapping with given country code among mappings of given
* aggregation
 * \param[in] agg Aggregation whose mappings are being looked up
 * \param[in] target_cc Country code of mapping being looked up
 * \param[out] rev Country (region) revision of found mapping
 * \return CLM_RESULT_OK if mapping was found, CLM_RESULT_ERR if any of passed
 * pointers was null or aggregation iterator does not 'point' to valid
 * aggregation, CLM_RESULT_NOT_FOUND if required aggregation was not found
 */
extern clm_result_t clm_agg_country_map_lookup(const clm_agg_country_t agg,
	const ccode_t target_cc, uint32 *rev);

/** Returns base data app version string
 * \return Pointer to version if it's present and not the vanilla string.
 * NULL if version is not present or unchanged from default.
 */
extern const char *clm_get_base_app_version_string(void);

/** Returns incremental data app version string
 * \return Pointer to version if it's present and not the vanilla string.
 * NULL if version is not present or unchanged from default.
 */
extern const char *clm_get_inc_app_version_string(void);

/** Returns string stored in CLM data
 * \param[in] string_type Type of string
 * \param[in] string_source Location of string (base or incremental CLM data)
 * \param[out] rev Country (region) revision of found mapping
 * \return Pointer to requested string. NULL if string is absent in CLM data or
 * unchanged from default
 */
extern const char *clm_get_string(clm_string_type_t string_type,
	clm_string_source_t string_source);

/** 0 or maximum power shift (in qdBM) for data contained in CLM BLOB
 * \return 0 if power shift not required (no limits above 31.75dBm in BLOB)
 * otherwise maximum power shift (value for clm_limits_params_t::power_shift)
 * that woud not affect (disable) lowest limits
 */
extern int32 clm_max_power_shift(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WLC_CLM_H_ */
