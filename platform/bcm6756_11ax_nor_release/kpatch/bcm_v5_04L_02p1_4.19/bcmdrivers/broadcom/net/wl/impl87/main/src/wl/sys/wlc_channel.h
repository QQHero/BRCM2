
/*
 * Common interface to channel definitions.
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
 * $Id: wlc_channel.h 806838 2022-01-05 16:38:14Z $
 */

#ifndef _WLC_CHANNEL_H_
#define _WLC_CHANNEL_H_

#include <typedefs.h>
#include <bcmwifi_channels.h>
#include <bcmwifi_rclass.h>
#include <wlc_ppr.h>
#include <wlc_clm.h>
#include <wlc_msfblob.h>

#define WLC_TXPWR_DB_FACTOR 4 /**< conversion for phy txpwr cacluations that use .25 dB units */

struct wlc_info;

#define MAX_CLMVER_STRLEN 128

#define MAXRCLIST_REG    2
#define WLC_RCLIST_80    0
#define WLC_RCLIST_160    1

#define WLC_VALID_CHANSPECS_WANT_80P80    0xF0

/*
 * CHSPEC_IS40()/CHSPEC_IS40_UNCOND
 * Use the conditional form of this macro, CHSPEC_IS40(), in code that is checking
 * chanspecs that have already been cleaned for an operational bandwidth supported by the
 * driver compile, such as the current radio channel or the currently associated BSS's
 * chanspec.
 * Use the unconditional form of this macro, CHSPEC_IS40_UNCOND(), in code that is
 * checking chanspecs that may not have a bandwidth supported as an operational bandwidth
 * by the driver compile, such as chanspecs that are specified in incoming ioctls or
 * chanspecs parsed from received packets.
 */
#define CHSPEC_IS40_UNCOND(chspec)    (((chspec) & WL_CHANSPEC_BW_MASK) == WL_CHANSPEC_BW_40)
#ifdef CHSPEC_IS40
#undef CHSPEC_IS40
#endif
#define CHSPEC_IS40(chspec)    CHSPEC_IS40_UNCOND(chspec)

#define CHSPEC_IS80_UNCOND(chspec)    (((chspec) & WL_CHANSPEC_BW_MASK) == WL_CHANSPEC_BW_80)
#ifdef CHSPEC_IS80
#undef CHSPEC_IS80
#endif
#ifdef WL11AC
#define CHSPEC_IS80(chspec)    CHSPEC_IS80_UNCOND(chspec)
#else /* WL11AC */
#define CHSPEC_IS80(chspec)    0
#endif /* WL11AC */

#define CHSPEC_IS8080_UNCOND(chspec)    (((chspec) & WL_CHANSPEC_BW_MASK) == WL_CHANSPEC_BW_8080)
#ifdef CHSPEC_IS8080
#undef CHSPEC_IS8080
#endif
#ifdef WL11AC
#define CHSPEC_IS8080(chspec)    CHSPEC_IS8080_UNCOND(chspec)
#else /* WL11AC */
#define CHSPEC_IS8080(chspec)    0
#endif /* WL11AC */

#define CHSPEC_IS160_UNCOND(chspec)    (((chspec) & WL_CHANSPEC_BW_MASK) == WL_CHANSPEC_BW_160)
#ifdef CHSPEC_IS160
#undef CHSPEC_IS160
#endif
#ifdef WL11AC
#define CHSPEC_IS160(chspec)    CHSPEC_IS160_UNCOND(chspec)
#else /* WL11AC */
#define CHSPEC_IS160(chspec)    0
#endif /* WL11AC */

#define WLC_CHAN_COEXIST(c1, c2) (wf_chspec_coexist(c1, c2))

/* Max IE len for parsing Supported Operating Classes */
#define MAXRCLISTSIZE    37

#define MAXREGCLASS    136
#define MAXRCTBL    28

/* +1 to compensate for bit0 of bitvec */
#define MAXRCVEC    CEIL((MAXREGCLASS + 1), NBBY)

/* Regulatory Class list */
#define MAXRCLIST    3
#define WLC_RCLIST_20    0
#define WLC_RCLIST_40L    1
#define WLC_RCLIST_40U    2

/* Global Regulatory Class list */
#define MAXGBLRCLIST    3
#define WLC_GBL_RCLIST_20    0
#define WLC_GBL_RCLIST_40L    1
#define WLC_GBL_RCLIST_40U    2

#define MAXRCGBLLIST_REG    2
#define WLC_RCGBLLIST_80    0
#define WLC_RCGBLLIST_160    1

#define GLOBAL_OP_CLASS_SEL 0x01
#define NON_GLOBAL_OP_CLASS_SEL 0x02

#define MAXRCBW    10

/** regulatory bitvec */
typedef struct {
    uint8    vec[MAXRCVEC];    /**< bitvec of regulatory class */
    uint8    count;        /**< number of bits set in vec */
} rcvec_t;

/** channel & regulatory class pair */
typedef struct {
    uint8 chan;        /**< channel */
    uint8 rclass;        /**< regulatory class */
} chan_rc_t;

/** regclass_t */
typedef struct {
    uint8 len;        /**< number of entry */
    chan_rc_t rctbl[MAXRCTBL];    /**< regulatory class table */
} rcinfo_t;

/** regclass and bandwidth */
typedef struct {
    uint8 rclass;
    uint16 bw;
} rcbw_t;

typedef struct {
    uint8 cnt;
    rcbw_t rcbwtbl[MAXRCBW];
} rcbwinfo_t;

/* Regulatory limits of TPE (Transmit Power Envelope) element */
typedef struct clm_tpe_regulatory_limits {
    signed char limit[(CLM_BW_160 - CLM_BW_20) + 1][CLM_REGULATORY_LIMIT_TYPE_NUM];
} clm_tpe_regulatory_limits_t;

/* maxpwr mapping to 5GHz band channels:
 * maxpwr[0] - channels [34-48]
 * maxpwr[1] - channels [52-60]
 * maxpwr[2] - channels [62-64]
 * maxpwr[3] - channels [100-140]
 * maxpwr[4] - channels [149-165]
 */
#define BAND_5G_PWR_LVLS    5 /**< 5 power levels for 5G */

/* power level in group of 2.4GHz band channels:
 * maxpwr[0] - CCK  channels [1]
 * maxpwr[1] - CCK  channels [2-10]
 * maxpwr[2] - CCK  channels [11-14]
 * maxpwr[3] - OFDM channels [1]
 * maxpwr[4] - OFDM channels [2-10]
 * maxpwr[5] - OFDM channels [11-14]
 */

/* macro to get 2.4 GHz channel group index for tx power */
#define CHANNEL_POWER_IDX_2G_CCK(c) (((c) < 2) ? 0 : (((c) < 11) ? 1 : 2)) /* cck index */
#define CHANNEL_POWER_IDX_2G_OFDM(c) (((c) < 2) ? 3 : (((c) < 11) ? 4 : 5)) /* ofdm index */

/* macros to get 5 GHz channel group index for tx power */
#define CH_PWR_IDX_5G_MAX    4
#define CHANNEL_POWER_IDX_5G(c) \
    (((c) < 52) ? 0 : (((c) < 62) ? 1 :(((c) < 100) ? 2 : (((c) < 149) ? 3 : \
                CH_PWR_IDX_5G_MAX))))

#define DIS_CH_GRP_MASK_ALL ((1 << ((CH_PWR_IDX_5G_MAX) + 1)) - 1)
/* invalidate if all channel groups are masked to be disabled */
#define IS_DIS_CH_GRP_VALID(G) (((G) & (DIS_CH_GRP_MASK_ALL))  != (DIS_CH_GRP_MASK_ALL))

#define IS_5G_CH_GRP_DISABLED(wlc, ch) \
    ((((wlc)->pub->_dis_ch_grp_conf | (wlc)->pub->_dis_ch_grp_user) & \
        (1 << CHANNEL_POWER_IDX_5G(ch))) != 0)

#define WLC_MAXPWR_TBL_SIZE        6 /**< max of BAND_5G_PWR_LVLS and 6 for 2.4 GHz */
#define WLC_MAXPWR_MIMO_TBL_SIZE    14 /**< max of BAND_5G_PWR_LVLS and 14 for 2.4 GHz */

/** locale channel and power info. */
typedef struct {
    uint32    valid_channels;
    uint8    radar_channels;        /**< List of radar sensitive channels */
    uint8    restricted_channels;    /**< List of channels used only if APs are detected */
    int8    maxpwr[WLC_MAXPWR_TBL_SIZE];    /**< Max tx pwr in qdBm for each sub-band */
    int8    pub_maxpwr[BAND_5G_PWR_LVLS];    /**< Country IE advertised max tx pwr in dBm
                         * per sub-band
                         */
    uint8    flags;
} locale_info_t;

/* bits for locale_info flags */
#define WLC_PEAK_CONDUCTED    0x0000 /**< Peak for locals */
#define WLC_EIRP        0x0001 /**< Flag for EIRP */
#define WLC_DFS_TPC        0x0002 /**< Flag for DFS TPC */
#define WLC_NO_80MHZ        0x0004 /**< Flag for No 80MHz channels */
#define WLC_NO_40MHZ        0x0008 /**< Flag for No MIMO 40MHz */
#define WLC_NO_MIMO        0x0010 /**< Flag for No MIMO, 20 or 40 MHz */
#define WLC_RADAR_TYPE_EU    0x0020 /**< Flag for EU */
#define WLC_TXBF        0x0040 /**< Flag for Tx beam forming */
#define WLC_FILT_WAR        0x0080 /**< Flag for apply filter war */
#define WLC_NO_160MHZ        0x0100 /**< Flag for No 160MHz channels */
#define WLC_EDCRS_EU        0x0200  /**< Use EU post-2015 energy detect */
/* LO_GAIN_NBCAL flag used in branches 9.30 and 9.44 for 43430 has value set as 0x200. */
/* Value to be changed to 0x400 to match Trunk */
#define WLC_LO_GAIN_NBCAL    0x0400 /* Flag for restricting power during cal */
#define WLC_RADAR_TYPE_UK    0x0800 /**< Flag for UK */
#define WLC_RADAR_TYPE_JP    0x1000 /**< Flag for JP */
#define WLC_CBP_FCC             0x8000 /**< FCC 6GHz Contention Based Protocol */
#define WLC_DFS_FCC        WLC_DFS_TPC /**< Flag for DFS FCC */
#define WLC_DFS_EU        (WLC_DFS_TPC | WLC_RADAR_TYPE_EU) /**< Flag for DFS EU */
#define WLC_DFS_UK        (WLC_DFS_TPC | WLC_RADAR_TYPE_UK) /**< Flag for DFS UK */
#define WLC_DFS_JP        (WLC_DFS_TPC | WLC_RADAR_TYPE_JP) /**< Flag for DFS JP */
#define WLC_RADAR_TYPE_FLAGS \
    (WLC_DFS_TPC | WLC_RADAR_TYPE_EU | WLC_RADAR_TYPE_UK | WLC_RADAR_TYPE_JP)

#define ISDFS_EU(fl)        (((fl) & WLC_RADAR_TYPE_FLAGS) == WLC_DFS_EU)
#define ISDFS_UK(fl)        (((fl) & WLC_RADAR_TYPE_FLAGS) == WLC_DFS_UK)
#define ISDFS_JP(fl)        (((fl) & WLC_RADAR_TYPE_FLAGS) == WLC_DFS_JP)

/**
 * locale per-channel tx power limits for MIMO frames
 * maxpwr arrays are index by channel for 2.4 GHz limits, and
 * by sub-band for 5 GHz limits using CHANNEL_POWER_IDX_5G(channel)
 */
typedef struct {
    int8    maxpwr20[WLC_MAXPWR_MIMO_TBL_SIZE];    /**< tx 20 MHz power limits, qdBm units */
    int8    maxpwr40[WLC_MAXPWR_MIMO_TBL_SIZE];    /**< tx 40 MHz power limits, qdBm units */
    uint8    flags;
} locale_mimo_info_t;

/* dfs setting */
#define WLC_DFS_RADAR_CHECK_INTERVAL    150    /**< radar check interval in ms */
#define WLC_DFS_CAC_TIME_USE_DEFAULTS    -1    /**< use default CAC times */
#define WLC_DFS_CAC_TIME_SEC_DEFAULT    60    /**< default CAC time in seconds */
#define WLC_DFS_CAC_TIME_SEC_DEF_EUWR    600    /**< default CAC time for european weather radar */
#define WLC_DFS_CAC_TIME_SEC_MAX    1000    /**< max CAC time in second */
#define WLC_DFS_NOP_SEC_DEFAULT        (1800 + 60)    /**< in second.
                             * plus 60 is a margin to ensure a mininum
                             * of 1800 seconds
                             */
#define WLC_DFS_CSA_MSEC    800    /**< mininum 800ms of csa process */
#define WLC_DFS_CSA_BEACONS    8    /**< minimum 8 beacons for csa */
#define WLC_CHANBLOCK_FOREVER   0xffffffff  /* special define for specific nop requirements */

#ifdef SLAVE_RADAR
#define WLC_RADAR_NOTIFICATION_TIMEOUT    40    /* timeout in ms */
#define WLC_RADAR_REPORT_RETRY_LIMIT    3
#endif /* SLAVE_RADAR */

/** Country names and abbreviations with locale defined from ISO 3166 */
struct country_info {
    const uint8 locale_2G;        /**< 2.4G band locale */
    const uint8 locale_5G;        /**< 5G band locale */
    const uint8 locale_mimo_2G;    /**< 2.4G mimo info */
    const uint8 locale_mimo_5G;    /**< 5G mimo info */
};

typedef struct country_info country_info_t;

typedef struct wlc_cm_info wlc_cm_info_t;

extern wlc_cm_info_t *wlc_channel_mgr_attach(struct wlc_info *wlc);
extern void wlc_channel_mgr_detach(wlc_cm_info_t *wlc_cm);

extern int wlc_valid_countrycode(wlc_cm_info_t *wlc_cmi, const char *country_abbrev,
    const char *ccode, uint regrev);
extern int wlc_set_countrycode(wlc_cm_info_t *wlc_cm, const char* ccode, uint regrev);
extern int wlc_set_countrycode_rev(
    wlc_cm_info_t *wlc_cm, const char* ccode, uint regrev);

extern const char* wlc_channel_country_abbrev(wlc_cm_info_t *wlc_cm);
extern const char* wlc_channel_ccode(wlc_cm_info_t *wlc_cm);
extern const char *wlc_channel_srom_ccode(wlc_cm_info_t *wlc_cm);
extern uint wlc_channel_regrev(wlc_cm_info_t *wlc_cm);
extern uint wlc_channel_srom_regrev(wlc_cm_info_t *wlc_cm);
extern uint16 wlc_channel_locale_flags(wlc_cm_info_t *wlc_cm);
extern uint16 wlc_channel_locale_flags_in_band(wlc_cm_info_t *wlc_cm, enum wlc_bandunit bandunit);
uint wlc_channel_country(wlc_cm_info_t *wlc_cmi);
void wlc_channel_setcountry(wlc_cm_info_t *wlc_cmi, clm_country_t country);

/* Specify channel types for get_first and get_next channel routines */
#define    CHAN_TYPE_ANY        0    /**< Dont care */
#define    CHAN_TYPE_CHATTY    1    /**< Normal, non-radar channel */
#define    CHAN_TYPE_QUIET        2    /**< Radar channel */

extern chanvec_t *wlc_quiet_chanvec_get(wlc_cm_info_t *wlc_cm);
extern chanvec_t *wlc_valid_chanvec_get(wlc_cm_info_t *wlc_cm, enum wlc_bandunit bandunit);
extern void wlc_quiet_channels_reset(wlc_cm_info_t *wlc_cm);
extern bool wlc_quiet_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec);
extern void wlc_set_quiet_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec);
extern void wlc_clr_quiet_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec);
extern void wlc_set_quiet_chanspec_exclude(wlc_cm_info_t *wlc_cm,
        chanspec_t chspec, chanspec_t chspec_exclude);

#define VALID_40CHANSPEC_IN_BAND(wlc, bandunit) wlc_valid_40chanspec_in_band((wlc)->cmi, bandunit)
#define VALID_80CHANSPEC_IN_BAND(wlc, bandunit) wlc_valid_80chanspec_in_band((wlc)->cmi, bandunit)
#define VALID_80CHANSPEC(_wlc, _chanspec) wlc_valid_80chanspec((_wlc), (_chanspec))

#define VALID_8080CHANSPEC_IN_BAND(wlc, bandunit) \
    wlc_valid_8080chanspec_in_band((wlc)->cmi, bandunit)
#define VALID_8080CHANSPEC(_wlc, _chanspec) wlc_valid_8080chanspec((_wlc), (_chanspec))

#define VALID_160CHANSPEC_IN_BAND(wlc, bandunit) wlc_valid_160chanspec_in_band((wlc)->cmi, bandunit)
#define VALID_160CHANSPEC(_wlc, _chanspec) wlc_valid_160chanspec((_wlc), (_chanspec))

extern bool wlc_valid_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec);
extern bool wlc_valid_chanspec_db(wlc_cm_info_t *wlc_cm, chanspec_t chspec);
extern bool wlc_valid_channel20(wlc_cm_info_t *wlc_cm, chanspec_t chspec, bool current_bu);
extern bool
wlc_valid_channel20_in_band(wlc_cm_info_t *wlc_cm, enum wlc_bandunit bandunit, uint val);
extern bool wlc_valid_40chanspec_in_band(wlc_cm_info_t *wlc_cm, enum wlc_bandunit bandunit);
extern bool wlc_valid_80chanspec_in_band(wlc_cm_info_t *wlc_cm, enum wlc_bandunit bandunit);
extern bool wlc_valid_80chanspec(struct wlc_info *wlc, chanspec_t chanspec);
extern bool wlc_valid_8080chanspec_in_band(wlc_cm_info_t *wlc_cm, enum wlc_bandunit bandunit);
extern bool wlc_valid_8080chanspec(struct wlc_info *wlc, chanspec_t chanspec);
extern bool wlc_valid_160chanspec_in_band(wlc_cm_info_t *wlc_cm, enum wlc_bandunit bandunit);
extern bool wlc_valid_160chanspec(struct wlc_info *wlc, chanspec_t chanspec);
extern uint8*
wlc_channel_get_valid_channels_vec(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit);

extern chanspec_t wlc_default_chanspec(wlc_cm_info_t *wlc_cm, bool hw_fallback);
extern chanspec_t wlc_next_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t cur_ch, int type, bool db);
extern chanspec_t wlc_default_chanspec_by_band(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit);
extern bool wlc_radar_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chanspec);
extern bool wlc_restricted_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec);
extern bool wlc_channel_clm_restricted_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec);
extern void wlc_clr_restricted_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chspec);
extern bool wlc_has_restricted_chanspec(wlc_cm_info_t *wlc_cm);
extern chanspec_t wlc_channel_next_2gchanspec(wlc_cm_info_t *wlc_cmi, chanspec_t cur_chanspec);
extern chanspec_t wlc_channel_rand_chanspec(wlc_info_t *wlc, bool exclude_current,
        bool radar_detected);

extern clm_limits_type_t clm_chanspec_to_limits_type(chanspec_t chspec);
#ifdef WL11AC
extern clm_limits_type_t clm_get_enclosing_subchan(clm_limits_type_t ctl_subchan, uint lvl);
#endif /* WL11AC */

extern void wlc_channel_reg_limits(wlc_cm_info_t *wlc_cm,
    chanspec_t chanspec, ppr_t *txpwr, ppr_ru_t *ru_txpwr);
extern void wlc_channel_set_chanspec(wlc_cm_info_t *wlc_cm, chanspec_t chanspec);
extern int wlc_channel_set_txpower_limit(wlc_cm_info_t *wlc_cm, uint8 local_constraint_qdbm);
extern uint8 wlc_rclass_extch_get(wlc_cm_info_t *wlc_cm, uint8 rclass);
extern bool wlc_is_edcrs_eu(struct wlc_info *wlc);
extern bool wlc_is_cbp_fcc(wlc_info_t *wlc);

#ifdef RADAR
extern bool wlc_is_dfs_eu_uk(struct wlc_info *wlc);
extern bool wlc_is_european_weather_radar_channel(struct wlc_info *wlc, chanspec_t chanspec);
#endif

extern clm_result_t wlc_country_lookup_direct(const char* ccode, uint regrev,
    clm_country_t *country);
extern clm_result_t wlc_country_lookup(struct wlc_info *wlc, const char* ccode, uint regrev,
    clm_country_t *country);
clm_result_t wlc_locale_get_channels(clm_country_locales_t *locales, clm_band_t band,
    chanvec_t *valid_channels, chanvec_t *restricted_channels);
extern clm_result_t wlc_get_locale(clm_country_t country, clm_country_locales_t *locales);
extern clm_result_t wlc_get_flags(clm_country_locales_t *locales, clm_band_t band, uint16 *flags);
#if defined(STA) && defined(WL11D)
clm_result_t wlc_country_lookup_ext(struct wlc_info *wlc, const char* ccode,
    clm_country_t *country);
#endif
extern bool wlc_japan(struct wlc_info *wlc);
extern clm_country_t wlc_get_country(struct wlc_info *wlc);
extern int wlc_get_channels_in_country(struct wlc_info *wlc, void *arg);
extern int wlc_get_country_list(struct wlc_info *wlc, void *arg);
/* Get 6GHz regulatory max power from CLM (TPE tab) for Transmit Power Envelope (TPE) element */
extern int wlc_get_6g_tpe_reg_max_power(wlc_cm_info_t *wlc_cmi,
    const clm_country_locales_t *locales,
    clm_tpe_regulatory_limits_t *limits, clm_regulatory_limit_dest_t limit_dest,
    clm_device_category_t device_category, chanspec_t chanspec);
extern int8 wlc_get_reg_max_power_for_channel_ex(wlc_cm_info_t *wlc_cm,
    clm_country_locales_t *locales, chanspec_t chspec, bool external);
extern int8 wlc_get_reg_max_power_for_channel(wlc_cm_info_t *wlc_cm, chanspec_t chspec,
    bool external);
extern void wlc_get_valid_chanspecs(wlc_cm_info_t *wlc_cm, wl_uint32_list_t *list,
    enum wlc_bandunit bu_req, uint8 bwmask, const char *abbrev);
extern bool wlc_valid_chanspec_cntry(wlc_cm_info_t *wlc_cm, const char *country_abbrev,
    chanspec_t    home_chanspec);

extern uint8 wlc_get_regclass(wlc_cm_info_t *wlc_cm, chanspec_t chanspec);
extern int wlc_regclass_get_band(wlc_cm_info_t *wlc_cmi, uint8 rclass, chanspec_band_t *band);
extern void wlc_dump_rclist(const char *name, uint8 *rclist, uint8 rclen, struct bcmstrbuf *b);
extern bool wlc_channel_get_chanvec(struct wlc_info *wlc, const char* country_abbrev,
    int bandtype, chanvec_t *channels);
extern void wlc_dump_clmver(wlc_cm_info_t *wlc_cm, struct bcmstrbuf *b);

extern bool wlc_channel_sarenable_get(wlc_cm_info_t *wlc_cm);
extern void wlc_channel_sarenable_set(wlc_cm_info_t *wlc_cm, bool state);
#ifdef WL_SARLIMIT
extern void wlc_channel_sar_init(wlc_cm_info_t *wlc_cm);
extern void wlc_channel_sarlimit_subband(wlc_cm_info_t *wlc_cm, chanspec_t chanspec, uint8 *sar);
extern int wlc_channel_sarlimit_get(wlc_cm_info_t *wlc_cm, sar_limit_t *sar);
extern int wlc_channel_sarlimit_set(wlc_cm_info_t *wlc_cm, sar_limit_t *sar);
#else
#define wlc_channel_sar_init(a) do { } while (0)
#define wlc_channel_sarlimit_subband(a, b, c) do { } while (0)
#define wlc_channel_sarlimit_get(a, b) (0)
#define wlc_channel_sarlimit_set(a, b) (0)
#endif /* WL_SARLIMIT */
extern void wlc_channel_update_txpwr_limit(struct wlc_info *wlc);
extern void wlc_channel_apply_band_chain_limits(struct wlc_info *wlc,
    int band, int8 band_max, int num_chains, int *txcpd_power_offset, int *tx_chain_offset);
extern int8 wlc_channel_max_tx_power_for_band(struct wlc_info *wlc, int band, int8 *min);
extern void wlc_channel_tx_power_target_min_max(struct wlc_info *wlc,
    chanspec_t chanspec, int *min_pwr, int *max_pwr);
extern void wlc_channel_set_tx_power(struct wlc_info *wlc,
    int band, int num_chains, int *txcpd_power_offset, int *tx_chain_offset);

#ifdef WLC_TXPWRCAP
extern void wlc_channel_txcap_cellstatus_cb(wlc_cm_info_t *wlc_cm, int cellstatus);
#endif
#ifdef WL_AP_CHAN_CHANGE_EVENT
extern void wlc_channel_send_chan_event(wlc_info_t *wlc, wlc_bsscfg_t *cfg,
        wl_chan_change_reason_t reason,    chanspec_t chanspec);
#endif /* WL_AP_CHAN_CHANGE_EVENT */
#ifdef WL_GLOBAL_RCLASS
extern uint8 wlc_sta_supports_global_rclass(uint8 *rclass_bitmap);
#endif /* WL_GLOBAL_RCLASS */
extern uint8 wlc_sta_supported_bands_from_rclass(bcmwifi_rclass_type_t rc_type,
    uint8 *rclass_bitmap);
extern bcmwifi_rclass_type_t wlc_channel_get_cur_rclass(wlc_info_t *wlc);
extern void wlc_update_rcinfo(wlc_cm_info_t *wlc_cmi, bool use_global);
extern bool wlc_is_ccode_lockdown(struct wlc_info *wlc);
extern uint8 *wlc_write_wide_bw_chan_ie(chanspec_t chspec, uint8 *cp, int buflen);
#ifdef BGDFS_2G
extern bool wlc_channel_2g_dfs_chan(wlc_cm_info_t *wlc_cmi, chanspec_t chspec);
extern int wlc_channel_set_phy_radar_detect(wlc_cm_info_t *wlc_cmi);
#endif /* BGDFS_2G */
#ifdef TD_EASYMESH_SUPPORT
extern bool wlc_eu_code(struct wlc_info *wlc);
extern bool wlc_us_code(struct wlc_info *wlc);
#endif
extern void wlc_channels_init_ext(wlc_cm_info_t *wlc_cmi);
#endif /* _WLC_CHANNEL_H */
