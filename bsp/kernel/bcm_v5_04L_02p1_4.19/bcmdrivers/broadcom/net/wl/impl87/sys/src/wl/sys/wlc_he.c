/*
 * 802.11ax HE (High Efficiency) STA signaling and d11 h/w manipulation.
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
 * $Id: wlc_he.c 810668 2022-04-12 09:18:49Z $
 */

#ifdef WL11AX

/* XXX: Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <802.11.h>
#include <802.11ax.h>
#include <wl_dbg.h>
#include <wlc_types.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_mgmt_vs.h>
#include <wlc_ie_helper.h>
#include <wlc_dump.h>
#include <wlc_iocv_cmd.h>
#include <wlc_bsscfg.h>
#include <wlc_rate.h>
#include <wlc_rspec.h>
#include <wlc_scb.h>
#include <wlc_mbss.h>
#include <wlc_musched.h>
#include <wlc_ulmu.h>
#include <wlc_he.h>
#include <wlc_assoc.h>
#include <phy_hecap_api.h>
#include <wlc_ap.h>
#include <wlc_dbg.h>
#include <wlc_stf.h>
#include <wlc_ratelinkmem.h>
#include <wlc_pcb.h>
#include <wlc_txbf.h>
#include <wlc_vht.h>
#include <wlc_scb_ratesel.h>
#include <wlc_apps.h>
#include <bcmdevs.h>
#ifdef TESTBED_AP_11AX
#include <wlc_test.h>
#endif /* TESTBED_AP_11AX */
#include <wlc_mutx.h>
#include <wlc_txmod.h>
#include <wlc_ampdu_cmn.h>
#include <wlc_ampdu_rx.h>
#include <wlc_ampdu.h>
#include <wlc_amsdu.h>
#include <wlc_scan.h>
#include <wlc_wnm.h>
#include <wlc_hw_priv.h>
#include <wlc_qoscfg.h>
#include <wlc_event_utils.h>
#include <phy_chanmgr_api.h>

#define HE_PPET_MAX_RUCOUNT_2G	2	/* 242 and 484, no support for 996 and 2x996 */
#define HE_PPET_MAX_RUCOUNT	4	/* 242, 484, 996 and 2x996 */
#define HE_MAX_PPET_SIZE	(HE_PPE_THRESH_LEN(HE_CAP_MCS_MAP_NSS_MAX, HE_PPET_MAX_RUCOUNT))
#define HE_MAX_HTC_CODES	16

#define HE_NEXT_HTC_IDX(idx)	(((idx) + 1) == HE_MAX_HTC_CODES ? 0 : (idx) + 1)
#define HE_INFO_STR_SZ		128

#define HE_OP_IE_MAX_LEN	(sizeof(he_op_ie_t) - TLV_HDR_LEN)

#define HE_BSSCOLOR_APIDX		HE_BSSCOLOR_IDX1
#define HE_BSSCOLOR_STAIDX		HE_BSSCOLOR_IDX0
#define HE_BSSCOLOR_MAX_VALUE		0x3f

#define HE_CC_AP_DETECT_ENAB		(1 << 0)
#define HE_CC_AP_REPORT_HANDLER_ENAB	(1 << 1)
#define HE_CC_AUTO_ENAB			(1 << 2)
#define HE_CC_EVENT_ENAB		(1 << 3)
#define HE_CC_STA_DETECT_ENAB		(1 << 4)
#define HE_CC_LOCAL_AP_DETECT_TIMEOUT	10	/* Timeout for colors found in beacon */
#define HE_CC_LOCAL_AP_DETECT_MAX_DELAY	5	/* Random delay in secs before triggering change */
#define HE_CC_REPORT_DETECT_TIMEOUT	120	/* Timeout for colors found in collision report */
#define HE_CC_REPORT_DETECT_DELAY	3	/* Fixed delay in secs before triggering change */
#define HE_CC_DETECT_BACKOFF_INIT	2	/* secs. initial delay befor first collision rep */
#define HE_CC_DETECT_BACKOFF_DEFAULT	60	/* secs. Min. time in between collision report */
#define HE_CC_CLS_DETECT_THRSHLD	2	/* Min. diff in count (per second) to detect coll */
#define HE_CC_REMOTE_COLL_CNT_INIT	120	/* Seconds a remote coll count remains valid */
#define HE_CC_LOCAL_COLL_CNT_INIT	15	/* Seconds a local coll count remains valid */
#define HE_CC_EVENT_DELAY		2	/* Seconds after coll report results in event */

#define HE_MUEDCA_TIMER_UNIT		8	/* Convert MU EDCA timer in 8 TUs */
#define MUEDCA_TIMER_US                 1000    /* Time in micro seconds */
#define M_ULTX_HOLDTM_H_MASK            0xffff0000 /* upper 16 bits of timeout value in micro sec */
#define M_ULTX_HOLDTM_H_SHIFT           16
#define M_ULTX_HOLDTM_L_MASK            0x0000ffff /* lower 16 bits of timeout value in micro sec */
#define M_ULTX_HOLDTM_L_SHIFT           0

#define	WLC_AIFS_AC_BK			0
#define	WLC_AIFS_AC_BE			1
#define	WLC_AIFS_AC_VI			2
#define	WLC_AIFS_AC_VO			3

const uint8 wlc_ac_aifs_update[AC_COUNT] = {
	WLC_AIFS_AC_BE,
	WLC_AIFS_AC_BK,
	WLC_AIFS_AC_VI,
	WLC_AIFS_AC_VO,
};

/* variable length PPE Thresholds field description */
typedef struct wlc_he_ppet {
	uint8 ppe_ths_len;
	uint8 ppe_ths[HE_MAX_PPET_SIZE];
} wlc_he_ppet_t;

typedef struct wlc_he_color_collision {
	bool ap_detect_enab;
	bool ap_report_handler_enab;
	bool auto_enab;
	bool event_enab;
	bool sta_detect_enab;
	uint8 local_collision_detect_cnt;
	uint8 remote_collision_detect_cnt;
	uint8 create_event_cnt;
	uint8 detected_colors_total;
	uint8 detected_colors_cnt[HE_BSSCOLOR_MAX_VALUE + 1];
	uint8 detect_report_cnt;
	uint8 detect_local_cnt;
	uint8 detect_prev_color;
	uint16 detect_backoff_init_cnt;
	uint16 detect_backoff_cnt;
	uint16 prev_cls_cnt;
} wlc_he_color_collision_t;

typedef struct wlc_he_obss_nbwru {
	uint ref_timestamp;
	bool obss_bcn_monitor_active;	/* OBSS beacon monitoring is active */
	bool discard_nbwru_tf;		/* Discard HE trigger frames with 26RU */
} wlc_he_obss_nbwru_t;

/* module info */
struct wlc_he_info {
	wlc_info_t *wlc;
	int scbh;
	int cfgh;

	/* HE capabilities of this node being advertised in outgoing mgmt frames.
	 * Do not modify this while processing incoming mgmt frames.
	 */
	he_mac_cap_t def_mac_cap;	/* default HE MAC capabilities of this node */
	he_phy_cap_t def_phy_cap;	/* default HE PHY capabilities of this node */
	uint16 def_6g_caps_info;	/* default 6G band capabilities of this node */
	wlc_he_rateset_t he_rateset;	/* Supported MCS per NSS per BW for tx/rx */
	uint he_ratesetsz;		/* size of rateset for OP and CAP ie */
	wlc_he_ppet_t ppe_ths_2g;	/* PPE Thresholds of this node during 2G operation */
	wlc_he_ppet_t ppe_ths_5g6g;	/* PPE Thresholds of this node for otherband operation */
	uint8 he_format;		/* range extension support */
	bool bsscolor_disable;		/* BSS coloring disabled? */
	uint8 bsscolor;			/* BSS color to be used in AP mode */
	uint8 update_bsscolor_counter;	/* BSS color running update counter, beacon countdowns */
	uint8 bsscolor_counter;		/* BSS color initial beacon counter */
	uint8 new_bsscolor;		/* New BSS color to be used */
	wlc_rateset_t rateset_filter;	/* externally configured rateset limit. */
	uint8 dynfrag;			/* Current dynamic fragment level */
	uint8 ppet;			/* PPET supported as specified by PHY */
	uint8 ppet_override;		/* PPET mode, auto or override to 0, 8 or 16 us */
	bool bsr_supported;		/* Testbed AP configurable. A-BSR support */
	bool testbed_mode;		/* Testbed AP active */
	bool muedca_blocked;		/* Testbed mode default is no muedca */
	bool sr_enabled;		/* Spatial Reuse IE enabled */
	uint8 sr_control;		/* Spatial Reuse SR control value */
	he_non_srg_t non_srg;		/* Non-SRG Information */
	he_srg_t srg;			/* SRG Information */
	wlc_he_color_collision_t cc;	/* Color Collision detection and handler information */
	wlc_he_obss_nbwru_t nbwru;	/* Conditions for not responding with an HE TB PPDU */
	bool sta_insert_dummy_htc;	/* Insert dummy HTC as STA in all QoS frames */
};

/* debug */
#define WL_HE_ERR(x)	WL_ERROR(x)
#ifdef BCMDBG
#define WL_HE_INFO(x)	WL_INFORM(x)
#else
#define WL_HE_INFO(x)
#endif

typedef struct {
	uint32 codes[HE_MAX_HTC_CODES];
	he_htc_cb_fn_t cb_fn[HE_MAX_HTC_CODES];
	uint8 rd_idx;		/* read index in codes to be transmitted */
	uint8 wr_idx;		/* write index in codes to be transmitted */
	uint8 outstanding;	/* currently HTC code being transmitted? */
} scb_he_htc_t;

/* scb cubby */
/* TODO: compress the structure as much as possible */
typedef struct {
	uint32 flags;		/* converted HE flags */
	uint32 ppe_nss[HE_CAP_MCS_MAP_NSS_MAX]; /* PPE Thresholds of this node */
	uint32 min_ppe_nss;
	he_mac_cap_t mac_cap;	/* capabilities info in the HE Cap IE */
	he_phy_cap_t phy_cap;	/* HE PHY capabilities of this node */
	bool ppet_valid;	/* ppet information in ppe_nss is valid */
	bool htc_retry_required; /* need to resend HTC code */
	uint8 trig_mac_pad_dur; /* in usec */
	uint8 send_htc_retries;	/* counter for HTC retries */
	scb_he_htc_t htc;	/* Tx HTC information */
	uint16 bw80_tx_mcs_nss;	/* HE Tx mcs nss set BW 80Mhz, supported/capabilities */
	uint16 bw80_rx_mcs_nss;	/* HE Rx mcs nss set BW 80Mhz, supported/capabilities  */
	uint16 bw160_tx_mcs_nss; /* HE Tx mcs nss set BW 160Mhz, supported/capabilities */
	uint16 bw160_rx_mcs_nss; /* HE Rx mcs nss set BW 160Mhz, supported/capabilities */
	uint16 bw80p80_tx_mcs_nss; /* HE Tx mcs nss set BW 80p80Mhz, supported/capabilities */
	uint16 bw80p80_rx_mcs_nss; /* HE Tx mcs nss set BW 80p80Mhz, supported/capabilities */
	uint16 omi_pmq;		/* OMI code as received through PMQ */
	uint16 omi_htc;		/* OMI code as received by HTC+ */
	uint16 omi_lm;		/* OMI code to be stored in linkmem */
	bool omi_recvd;		/* OMI code received */
	uint8 multi_tid_agg_num;
} scb_he_t;

/** per BSS info */
typedef struct {
	he_mu_ac_param_t sta_mu_edca[AC_COUNT];	/* EDCA Parameter set be used by STAs */
	uint16 rts_thresh;		/* AP config for duration of RTS in usec */
	uint8 pe_duration;		/* Default PE Duration */
	uint8 edca_update_count;	/* EDCA Parameter Set Update Count, for STA mode only */
	uint8 bsscolor;			/* BSS color to be used in STA mode */
	bool bsscolor_disable;		/* BSS coloring disabled? for Rx */
	bool omc_cap;			/* HE MAC Capability OM Control. 1 for AP; 0 for STA */
} bss_he_info_t;

/* cubby access macros */
#define SCB_HE_CUBBY(hei, scb)		(scb_he_t **)SCB_CUBBY(scb, (hei)->scbh)
#define SCB_HE(hei, scb)		*SCB_HE_CUBBY(hei, scb)

/* handy macros to access bsscfg cubby & data */
#define BSS_HE_INFO_LOC(hei, cfg)	(bss_he_info_t **)BSSCFG_CUBBY(cfg, (hei)->cfgh)
#define BSS_HE_INFO(hei, cfg)		*BSS_HE_INFO_LOC(hei, cfg)

/* features mask */
#define WL_HE_MAX_ALLOWED_FEATURES	(WL_HE_FEATURES_5G | WL_HE_FEATURES_2G | \
					WL_HE_FEATURES_DLOMU | WL_HE_FEATURES_ULOMU | \
					WL_HE_FEATURES_DLMMU | WL_HE_FEATURES_ULMMU | \
					WL_HE_FEATURES_ULMU_STA | WL_HE_FEATURES_ERSU_RX)
/* default features */
#ifndef WL_HE_FEATURES_DEFAULT
#define WL_HE_FEATURES_DEFAULT		(WL_HE_FEATURES_2G | WL_HE_FEATURES_5G | \
					WL_HE_FEATURES_DLOMU | WL_HE_FEATURES_ULOMU | \
					WL_HE_FEATURES_DLMMU | WL_HE_FEATURES_ULMU_STA)
#endif /* WL_HE_FEATURES_DEFAULT */

/* Max. MSDU fragments */
#define WLC_HE_MAX_MSDU_FRAG_ENC	1 /* Max. MSDU fragments receivable by
					* HE STA to be 2^(HE_MAC_MSDU_FRAG_ENC) -1
					*/
/* Max. TID in an AMPDU */
#define WLC_HE_MAX_TID_IN_AMPDU		1

#ifndef HE_DEFAULT_PE_DURATION
#define HE_DEFAULT_PE_DURATION		4	/* in units of 4us */
#endif

#define HE_BW_RX_RASETSZ		2
#define HE_BW_TX_RASETSZ		2
#define HE_BW_RASETSZ			(HE_BW_TX_RASETSZ + HE_BW_RX_RASETSZ)
#define HE_BW80_RX_RATESET_OFFSET	0
#define HE_BW80_TX_RATESET_OFFSET	2
#define HE_BW160_RX_RATESET_OFFSET	4
#define HE_BW160_TX_RATESET_OFFSET	6
#define HE_BW80P80_RX_RATESET_OFFSET	8
#define HE_BW80P80_TX_RATESET_OFFSET	10
#define HE_MIN_HE_RATESETSZ		HE_BW_RASETSZ

#define HE_RTS_DUR_THRESHOLD_32USEC_SHIFT	5 /* convert usec <-> 32usec unit */

#define HE_INITIAL_BSSCOLOR_UPDATE_CNT	10

#define HE_MUEDCA_DEFAULT_TIMER		8
#define HE_MUEDCA_INVALID_UPDATE_COUNT	0xff

#define HE_LOWEST_QAM1024_MCS_IDX	10

#define HE_SEND_HTC_RETRY_LIMIT		3

/* Macro to expand a ppet (ppet8 + ppet16 combo) to all used RUs consuming a total of 24 bits */
#define HE_EXPAND_PPET_ALL_RU(ppet)			\
	(((ppet) << (6 * HE_PPE_THRESH_NSS_RU_FSZ)) |	\
	((ppet) << (4 * HE_PPE_THRESH_NSS_RU_FSZ)) |	\
	((ppet) << (2 * HE_PPE_THRESH_NSS_RU_FSZ)) |	\
	(ppet))

/* The HT control field in non-wds is located at (FC|DUR|A1|A2|A3|SEQ|QOS|HTC) 2+2+6+6+6+2+2=26 */
#define HTC_CONTROL_OFFSET	26
#define HTC_SIZEOFA4		6
#define HTC_IDENTITY_MASK	0x3		/* See 9.2.4.6 */
#define HTC_IDENTITY_HE		0x3
#define HTC_CODE_SIZE		32		/* HTC code is 32 bits */
#define HTC_VARIANT_SZ		2		/* HTC code variant is 2 bits. */
#define HTC_CONTROL_ID_SZ	4		/* HTC code HE control ID is 4 bits. */

#define HTC_CONTROL_ID_TRS	0		/* HTC Control ID Triggered response scheduling */
#define HTC_CONTROL_TRS_SZ	26
#define HTC_CONTROL_ID_OM	1		/* HTC Control ID Operating mode */
#define HTC_CONTROL_OM_SZ	12
#define HTC_CONTROL_ID_HLA	2		/* HTC Control ID HE link adaptation */
#define HTC_CONTROL_HLA_SZ	26
#define HTC_CONTROL_ID_BSR	3		/* HTC Control ID Buffer status report */
#define HTC_CONTROL_BSR_SZ	26
#define HTC_CONTROL_ID_UPH	4		/* HTC Control ID UL power headroom */
#define HTC_CONTROL_UPH_SZ	8
#define HTC_CONTROL_ID_BQR	5		/* HTC Control ID Bandwidth query report */
#define HTC_CONTROL_BQR_SZ	10
#define HTC_CONTROL_ID_CAS	6		/* HTC Control ID Command and status */
#define HTC_CONTROL_CAS_SZ	8
#define HTC_CONTROL_ID_ONES	15		/* HTC Control ID ONES */
#define HTC_CONTROL_ONES_SZ	26

/* See 9.2.4.6a.2 OM Control: */
#define HE_OMI_ENCODE(rx, tx, bw, er_su, dl_res, ul_dis, ul_data_dis) \
	(((rx) << HTC_OM_CONTROL_RX_NSS_OFFSET) | \
	((tx) << HTC_OM_CONTROL_TX_NSTS_OFFSET) | \
	((bw) << HTC_OM_CONTROL_CHANNEL_WIDTH_OFFSET) | \
	((er_su) << HTC_OM_CONTROL_ER_SU_DISABLE_OFFSET) | \
	((dl_res) << HTC_OM_CONTROL_DL_MUMIMO_RESOUND_OFFSET) | \
	((ul_dis) << HTC_OM_CONTROL_UL_MU_DISABLE_OFFSET) | \
	((ul_data_dis) << HTC_OM_CONTROL_UL_MU_DATA_DISABLE_OFFSET))

#define HE_TXRATE_UPDATE_MASK	(HTC_OM_CONTROL_RX_NSS_MASK | HTC_OM_CONTROL_CHANNEL_WIDTH_MASK)

/* default mu edca AIFSN setting */
#define HE_MUEDCA_AC_BE_ACI	(EDCF_AC_BE_ACI_STA & ~EDCF_AIFSN_MASK) /* 0x00 */
#define HE_MUEDCA_AC_BK_ACI	(EDCF_AC_BK_ACI_STA & ~EDCF_AIFSN_MASK) /* 0x20 */
#define HE_MUEDCA_AC_VI_ACI	(EDCF_AC_VI_ACI_STA & ~EDCF_AIFSN_MASK) /* 0x40 */
#define HE_MUEDCA_AC_VO_ACI	(EDCF_AC_VO_ACI_STA & ~EDCF_AIFSN_MASK) /* 0x60 */

static const he_mu_ac_param_t default_mu_edca_sta[AC_COUNT] = {
	{ HE_MUEDCA_AC_BE_ACI, EDCF_AC_BE_ECW_STA, HE_MUEDCA_DEFAULT_TIMER },
	{ HE_MUEDCA_AC_BK_ACI, EDCF_AC_BK_ECW_STA, HE_MUEDCA_DEFAULT_TIMER },
	{ HE_MUEDCA_AC_VI_ACI, EDCF_AC_VI_ECW_STA, HE_MUEDCA_DEFAULT_TIMER },
	{ HE_MUEDCA_AC_VO_ACI, EDCF_AC_VO_ECW_STA, HE_MUEDCA_DEFAULT_TIMER }
};

/* local declarations */

/* capabilities */
static void wlc_he_ap_maccap_init(wlc_he_info_t *hei, he_mac_cap_t *ap_mac_cap);
static void wlc_he_sta_maccap_init(wlc_he_info_t *hei, he_mac_cap_t *sta_mac_cap);
static void wlc_he_maccap_init(wlc_he_info_t *hei);
static void wlc_he_ap_phycap_init(wlc_he_info_t *hei, he_phy_cap_t *ap_phy_cap);
static void wlc_he_sta_phycap_init(wlc_he_info_t *hei, he_phy_cap_t *sta_phy_cap);

/* wlc module */
static int wlc_he_wlc_up(void *ctx);
static int wlc_he_wlc_down(void *ctx);
static int wlc_he_doiovar(void *context, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif);
static void wlc_he_watchdog(void *ctx);
#if defined(BCMDBG)
static int wlc_he_dump(void *ctx, struct bcmstrbuf *b);
#endif
static void BCMFASTPATH wlc_he_htc_pkt_freed(wlc_info_t *wlc, uint txs, void* arg);

/* bsscfg module */
static int wlc_he_bss_init(void *ctx, wlc_bsscfg_t *cfg);
static void wlc_he_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);
#if defined(BCMDBG)
static void wlc_he_bss_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b);
#endif
static void wlc_he_bsscfg_state_upd(void *ctx, bsscfg_state_upd_data_t *evt);
static void wlc_he_assoc_state_change_cb(void *ctx, bss_assoc_state_data_t *notif_data);
#if defined(STA)
static void wlc_he_disassoc_state_change_cb(void *ctx, bss_disassoc_notif_data_t *notif_data);
static void wlc_he_chansw_upd_cb(void *arg, wlc_chansw_notif_data_t *data);
#endif

/* scb cubby */
static int wlc_he_scb_init(void *ctx, scb_t *scb);
static void wlc_he_scb_deinit(void *ctx, scb_t *scb);
static uint wlc_he_scb_secsz(void *, scb_t *);
#if defined(BCMDBG)
static void wlc_he_scb_dump(void *ctx, scb_t *scb, struct bcmstrbuf *b);
#endif

/* IE mgmt */
static uint wlc_he_calc_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_he_write_cap_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_he_calc_op_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_he_write_op_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_he_calc_muedca_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_he_write_muedca_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_he_calc_color_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_he_write_color_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_he_calc_sr_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_he_write_sr_ie(void *ctx, wlc_iem_build_data_t *data);
static void wlc_he_parse_cap_ie(wlc_he_info_t *hei, scb_t *scb, he_cap_ie_t *cap);
static int wlc_he_assoc_parse_cap_ie(void *ctx, wlc_iem_parse_data_t *data);
#ifdef STA
static void wlc_he_read_6ghz_op_ie(he_op_6ghz_op_info_t *_6ghz_op_info, wlc_bss_info_t *bi);
static void
wlc_he_op_ie_read_optional_fields(wlc_info_t *wlc, he_op_ie_t *op_ie, wlc_bss_info_t *bi);
#endif /* STA */
static int wlc_he_parse_op_ie(void *ctx, wlc_iem_parse_data_t *data);
static int wlc_he_parse_muedca_ie(void *ctx, wlc_iem_parse_data_t *data);
static int wlc_he_scan_parse_cap_ie(void *ctx, wlc_iem_parse_data_t *data);
static int wlc_he_scan_parse_op_ie(void *ctx, wlc_iem_parse_data_t *data);
/* The HE 6G band cap IE contains fields that are also present in VHT IEs */
static uint wlc_he_calc_6g_caps_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_he_write_6g_caps_ie(void *ctx, wlc_iem_build_data_t *data);
static void wlc_he_parse_6g_caps_ie(wlc_he_info_t *hei, scb_t *scb,
	he_6g_band_caps_ie_t *ie);
static int wlc_he_assoc_parse_6g_caps_ie(void *ctx, wlc_iem_parse_data_t *data);

/* misc */
static bool wlc_he_hw_cap(wlc_info_t *wlc);
static void wlc_he_configure_bsscolor_phy(wlc_he_info_t *hei, wlc_bsscfg_t *cfg);
static void wlc_he_color_collision_process_beacon_probe(wlc_he_info_t *hei, wlc_bsscfg_t *bsscfg,
	he_op_ie_t* op_ie);
#ifdef STA
static void wlc_he_send_htc_indication(wlc_info_t *wlc, wlc_bsscfg_t *cfg, scb_t *scb);
#endif /* STA */

#ifdef HERTDBG
static void wlc_he_print_rateset(wlc_he_rateset_t *he_rateset);
#endif /* HERTDBG */

static void wlc_he_color_collision_event(wlc_he_info_t *hei, wl_color_event_t *color_event);

/* iovar table */
enum {
	IOV_HE = 0,
	IOV_LAST
};

static const bcm_iovar_t he_iovars[] = {
	{"he", IOV_HE, 0, 0, IOVT_BUFFER, 0},
	{NULL, 0, 0, 0, 0, 0}
};

static int wlc_he_cmd_enab(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_features(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_bsscolor(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_rtsdurthresh(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_peduration(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_muedca(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_dynfrag(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_ppet(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_htc(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_axmode(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_omi(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_fragtx(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_sr(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_defcap(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_color_collision(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_trigger_color_event(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);

#ifdef TESTBED_AP_11AX
static int wlc_he_cmd_bsr(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
static int wlc_he_cmd_testbed(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif);
#endif /* TESTBED_AP_11AX */

static int
wlc_he_cmd_cap(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;

	*result = wlc_he_hw_cap(wlc);

	*rlen = sizeof(*result);

	return BCME_OK;
}

uint8
wlc_get_heformat(wlc_info_t *wlc)
{
	return wlc->hei->he_format;
}

uint8
wlc_get_hebsscolor(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bss_he_info_t *bhei;

	bhei = BSS_HE_INFO(wlc->hei, cfg);

	return bhei->bsscolor;
}

bool
wlc_he_get_bsscolor_disable(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bss_he_info_t *bhei;
	bhei = BSS_HE_INFO(wlc->hei, cfg);
	return bhei->bsscolor_disable;
}

static int
wlc_he_cmd_range_ext(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;

	if (set) {
		if (!wlc_he_hw_cap(wlc)) {
			return BCME_UNSUPPORTED;
		}
		if ((*params >= HE_FORMAT_MAX) || (*params == HE_106_TONE_RANGE_EXT)) {
			return BCME_RANGE;
		}

		/* bit mask 0x00 --> range extension disabled
		 * bit mask 0x01 --> 106 tone range extension is supported
		 * bit mask 0x10 --> 242 tone range extension is supported
		 * bit mask 0x11 --> 106 and 242 tone range extension is supported
		 * 4369A0 only supports 242 tone so use h format 1 to represent 242 when
		 * IOVAR issue wl he range ext 2
		 */
		if (*params == HE_242_TONE_RANGE_EXT) {
			hei->he_format = HE_FORMAT_RANGE_EXT;
		}
		else {
			hei->he_format = HE_FORMAT_SU;
		}
	}
	else {

		*result = hei->he_format;

		/* HE format HE_FORMAT_RANGE_EXT represents the  242 tone
		 * hence return HE_242_TONE_RANGE_EXT
		 */
		if (*result == HE_FORMAT_RANGE_EXT)
			*result = HE_242_TONE_RANGE_EXT;

		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

/*  HE cmds  */
static const wlc_iov_cmd_t he_cmds[] = {
	{ WL_HE_CMD_ENAB, 0 | IOVF_SET_DOWN, IOVT_UINT8, wlc_he_cmd_enab },
	{ WL_HE_CMD_FEATURES, IOVF_SET_DOWN, IOVT_UINT32, wlc_he_cmd_features },
	{ WL_HE_CMD_BSSCOLOR, 0, IOVT_BUFFER, wlc_he_cmd_bsscolor },
	{ WL_HE_CMD_CAP, 0, IOVT_UINT8, wlc_he_cmd_cap },
	{ WL_HE_CMD_RANGE_EXT, 0, IOVT_UINT8, wlc_he_cmd_range_ext },
	{ WL_HE_CMD_RTSDURTHRESH, 0, IOVT_UINT16, wlc_he_cmd_rtsdurthresh },
	{ WL_HE_CMD_PEDURATION, 0, IOVT_UINT8, wlc_he_cmd_peduration },
	{ WL_HE_CMD_MUEDCA, 0, IOVT_BUFFER, wlc_he_cmd_muedca },
	{ WL_HE_CMD_DYNFRAG, IOVF_SET_DOWN, IOVT_UINT8, wlc_he_cmd_dynfrag },
	{ WL_HE_CMD_PPET, 0, IOVT_UINT8, wlc_he_cmd_ppet },
	{ WL_HE_CMD_HTC, 0, IOVT_UINT32, wlc_he_cmd_htc },
	{ WL_HE_CMD_AXMODE, 0, IOVT_BOOL, wlc_he_cmd_axmode },
	{ WL_HE_CMD_OMI, 0, IOVT_BUFFER, wlc_he_cmd_omi },
	{ WL_HE_CMD_FRAGTX, 0, IOVT_UINT8, wlc_he_cmd_fragtx },
	{ WL_HE_CMD_SR, 0, IOVT_BUFFER, wlc_he_cmd_sr },
	{ WL_HE_CMD_DEFCAP, IOVF_SET_DOWN, IOVT_BUFFER, wlc_he_cmd_defcap },
	{ WL_HE_CMD_COLOR_COLLISION, 0, IOVT_UINT8, wlc_he_cmd_color_collision },
	{ WL_HE_CMD_TRIGGER_COLOR_EVENT, 0, IOVT_BUFFER, wlc_he_cmd_trigger_color_event },
#ifdef TESTBED_AP_11AX
	{ WL_HE_CMD_BSR_SUPPORT, IOVF_SET_DOWN, IOVT_BOOL, wlc_he_cmd_bsr },
	{ WL_HE_CMD_TESTBED, IOVF_SET_DOWN, IOVT_BOOL, wlc_he_cmd_testbed },
#endif /* TESTBED_AP_11AX */
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
*/
#include <wlc_patch.h>

/* ======== attach/detach ======== */

wlc_he_info_t *
BCMATTACHFN(wlc_he_attach)(wlc_info_t *wlc)
{
	uint16 build_capfstbmp =
		FT2BMP(FC_BEACON) |
		FT2BMP(FC_PROBE_RESP) |
#ifdef STA
		FT2BMP(FC_ASSOC_REQ) |
		FT2BMP(FC_REASSOC_REQ) |
#endif
#ifdef AP
		FT2BMP(FC_ASSOC_RESP) |
		FT2BMP(FC_REASSOC_RESP) |
#endif
		FT2BMP(FC_PROBE_REQ) |
		0;
	uint16 build_opfstbmp =
		FT2BMP(FC_BEACON) |
		FT2BMP(FC_PROBE_RESP) |
#ifdef AP
		FT2BMP(FC_ASSOC_RESP) |
		FT2BMP(FC_REASSOC_RESP) |
#endif
		0;
	uint16 build_muedcafstbmp = (
#ifdef AP
		FT2BMP(FC_BEACON) |
		FT2BMP(FC_PROBE_RESP) |
		FT2BMP(FC_ASSOC_RESP) |
		FT2BMP(FC_REASSOC_RESP) |
#endif /* AP */
		0);
	uint16 build_colorfstbmp = (
#ifdef AP
		FT2BMP(FC_BEACON) |
		FT2BMP(FC_PROBE_RESP) |
		FT2BMP(FC_ASSOC_RESP) |
		FT2BMP(FC_REASSOC_RESP) |
#endif /* AP */
		0);
	uint16 build_spatialreusefstbmp = (
#ifdef AP
		FT2BMP(FC_BEACON) |
		FT2BMP(FC_PROBE_RESP) |
		FT2BMP(FC_ASSOC_RESP) |
		FT2BMP(FC_REASSOC_RESP) |
#endif /* AP */
		0);

	uint16 parse_capfstbmp =
#ifdef AP
		FT2BMP(FC_ASSOC_REQ) |
		FT2BMP(FC_REASSOC_REQ) |
#endif
#ifdef STA
		FT2BMP(FC_ASSOC_RESP) |
		FT2BMP(FC_REASSOC_RESP) |
#endif
		FT2BMP(FC_PROBE_REQ) |
		0;
	uint16 parse_opfstbmp =
#ifdef STA
		FT2BMP(FC_BEACON) |
		FT2BMP(FC_ASSOC_RESP) |
		FT2BMP(FC_REASSOC_RESP) |
#endif
		0;

	uint16 parse_muedcafstbmp = (
#ifdef STA
		FT2BMP(FC_BEACON) |
		FT2BMP(FC_ASSOC_RESP) |
		FT2BMP(FC_REASSOC_RESP) |
#endif /* STA */
		0);
	uint16 scanfstbmp =
		FT2BMP(WLC_IEM_FC_SCAN_BCN) |
		FT2BMP(WLC_IEM_FC_SCAN_PRBRSP |
		0);
	wlc_he_info_t *hei;
	scb_cubby_params_t cubby_params;
	bsscfg_cubby_params_t cfg_cubby_params;
	uint8 random;

	/* allocate private module info */
	if ((hei = MALLOCZ(wlc->osh, sizeof(*hei))) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	hei->wlc = wlc;

	if (!wlc_he_hw_cap(wlc)) {
		WL_INFORM(("wl%d: %s: HE functionality disabled \n", wlc->pub->unit, __FUNCTION__));
		wlc->pub->_he_enab = FALSE;
		return hei;
	}

	/* reserve some space in scb for private data */
	bzero(&cubby_params, sizeof(cubby_params));

	cubby_params.context = hei;
	cubby_params.fn_init = wlc_he_scb_init;
	cubby_params.fn_deinit = wlc_he_scb_deinit;
	cubby_params.fn_secsz = wlc_he_scb_secsz;
#if defined(BCMDBG)
	cubby_params.fn_dump = wlc_he_scb_dump;
#endif

	if ((hei->scbh = wlc_scb_cubby_reserve_ext(wlc, sizeof(scb_he_t *), &cubby_params)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve_ext() failed\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	/* register IE mgmt callbacks - calc/build */

	/* bcn/prbreq/prbrsp/assocreq/reassocreq/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, build_capfstbmp, DOT11_MNG_HE_CAP_ID,
			wlc_he_calc_cap_ie_len, wlc_he_write_cap_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, he cap ie\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	/* bcn/prbreq/prbrsp/assocreq/reassocreq/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, build_capfstbmp, DOT11_MNG_HE_6G_BAND_CAPS_ID,
			wlc_he_calc_6g_caps_ie_len,
			wlc_he_write_6g_caps_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, ext he cap ie\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	/* bcn/prbrsp/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, build_opfstbmp, DOT11_MNG_HE_OP_ID,
			wlc_he_calc_op_ie_len, wlc_he_write_op_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, he op ie\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}
	/* bcn/prbrsp/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, build_muedcafstbmp, DOT11_MNG_MU_EDCA_ID,
			wlc_he_calc_muedca_ie_len, wlc_he_write_muedca_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, he mu-edca ie\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* bcn/prbrsp/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, build_colorfstbmp, DOT11_MNG_COLOR_CHANGE_ID,
			wlc_he_calc_color_ie_len, wlc_he_write_color_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, he color change ie\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	/* bcn/prbrsp/assocresp/reassocresp */
	if (wlc_iem_add_build_fn_mft(wlc->iemi, build_spatialreusefstbmp, DOT11_MNG_SRPS_ID,
			wlc_he_calc_sr_ie_len, wlc_he_write_sr_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, he spatial reuse ie\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register IE mgmt callbacks - parse */

	/* assocreq/reassocreq/assocresp/reassocresp */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, parse_capfstbmp, DOT11_MNG_HE_CAP_ID,
		wlc_he_assoc_parse_cap_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, he cap ie\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	/* assocreq/reassocreq/assocresp/reassocresp */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, parse_capfstbmp, DOT11_MNG_HE_6G_BAND_CAPS_ID,
		wlc_he_assoc_parse_6g_caps_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, he ext cap ie\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	/* bcn/assocresp/reassocresp */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, parse_opfstbmp, DOT11_MNG_HE_OP_ID,
		wlc_he_parse_op_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, he op ie\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}
	/* bcn/assocresp/reassocresp */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, parse_muedcafstbmp, DOT11_MNG_MU_EDCA_ID,
		wlc_he_parse_muedca_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, he mu-edca ie\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}
	/* bcn/prbrsp in scan */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, scanfstbmp, DOT11_MNG_HE_CAP_ID,
		wlc_he_scan_parse_cap_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, cap ie in scan\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}
	/* bcn/prbrsp in scan */
	if (wlc_iem_add_parse_fn_mft(wlc->iemi, scanfstbmp, DOT11_MNG_HE_OP_ID,
		wlc_he_scan_parse_op_ie, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_parse_fn failed, op ie in scan\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	/* reserve space for bss data */
	bzero(&cfg_cubby_params, sizeof(cfg_cubby_params));

	cfg_cubby_params.context = hei;
	cfg_cubby_params.fn_init = wlc_he_bss_init;
	cfg_cubby_params.fn_deinit = wlc_he_bss_deinit;
#if defined(BCMDBG)
	cfg_cubby_params.fn_dump = wlc_he_bss_dump;
#endif

	hei->cfgh = wlc_bsscfg_cubby_reserve_ext(wlc, sizeof(bss_he_info_t), &cfg_cubby_params);
	if (hei->cfgh < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve_ext failed\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	/* bsscfg state change callback */
	if (wlc_bsscfg_state_upd_register(wlc, wlc_he_bsscfg_state_upd, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_state_upd_register failed\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	/* register assoc state change callback */
	if (wlc_bss_assoc_state_register(wlc, wlc_he_assoc_state_change_cb, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bss_assoc_state_register failed\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}
#if defined(STA)
	/* register disassoc state change callback */
	if (wlc_bss_disassoc_notif_register(wlc, wlc_he_disassoc_state_change_cb, hei) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bss_assoc_state_register failed\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}
	if ((wlc_chansw_notif_register(wlc, wlc_he_chansw_upd_cb, wlc)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_chansw_notif_register failed\n", wlc->pub->unit,
		__FUNCTION__));
		goto fail;
	}
#endif /* STA */

	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, he_iovars, "he", hei, wlc_he_doiovar, wlc_he_watchdog,
		wlc_he_wlc_up, wlc_he_wlc_down)) {
		WL_ERROR(("wl%d: %s: wlc_module_register failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG)
	/* debug dump */
	wlc_dump_register(wlc->pub, "he", wlc_he_dump, hei);
#endif

	wlc->pub->_he_enab = TRUE;

	WLC_HE_FEATURES_SET(wlc->pub, WL_HE_FEATURES_DEFAULT);
	/* ULOFDMA as STA not supported until 43684c0 */
	if (D11REV_LE(wlc->pub->corerev, 129) && D11MINORREV_LT(wlc->pub->corerev_minor, 2)) {
		WLC_HE_FEATURES_CLR(wlc->pub, WL_HE_FEATURES_ULMU_STA);
	}
	/* 6705 (same chipid as 43692) and 43692 do not support MU */
	if (CHIPID(si_chipid(wlc->pub->sih)) == BCM43692_CHIP_ID) {
		WLC_HE_FEATURES_CLR(wlc->pub, WL_HE_FEATURES_DLMMU);
	}

	/* update FIFOs because we enable MU */
	wlc_hw_update_nfifo(wlc->hw);

	/* Initialize coloring number */
	wlc_getrand(wlc, &random, 1);
	hei->bsscolor = (random % HE_BSSCOLOR_MAX_VALUE) + 1;
	hei->bsscolor_disable = FALSE;
	hei->bsscolor_counter = HE_INITIAL_BSSCOLOR_UPDATE_CNT;

	/* support dynamic fragmentation level 2 now for AX chip */
	hei->dynfrag = HE_MAC_FRAG_NOSUPPORT;
	wlc->pub->_he_fragtx = 0;

	/* Default mode at startup for PPET override is auto */
	hei->ppet_override = WL_HE_PPET_AUTO;

#ifdef TESTBED_AP_11AX
	/* By default BSR support is cleared for testbed AP. */
	hei->bsr_supported = FALSE;
#else
	/* By default BSR support is set. */
	hei->bsr_supported = TRUE;
#endif /* TESTBED_AP_11AX */

	return hei;

fail:
	wlc_he_detach(hei);
	return NULL;
}

void
BCMATTACHFN(wlc_he_detach)(wlc_he_info_t *hei)
{
	wlc_info_t *wlc;

	if (hei == NULL) {
		return;
	}

	wlc = hei->wlc;
#if defined(STA)
	(void)wlc_chansw_notif_unregister(wlc, wlc_he_chansw_upd_cb, wlc);
	(void)wlc_bss_disassoc_notif_unregister(wlc, wlc_he_disassoc_state_change_cb, hei);
#endif
	(void)wlc_bss_assoc_state_unregister(wlc, wlc_he_assoc_state_change_cb, hei);
	wlc_module_unregister(wlc->pub, "he", hei);

	MFREE(wlc->osh, hei, sizeof(*hei));
}

static void
wlc_he_cp_rateset_to_wlc(wlc_he_rateset_t *he_rateset, wlc_rateset_t *rateset)
{
	rateset->he_bw80_tx_mcs_nss = he_rateset->bw80_tx_mcs_nss;
	rateset->he_bw80_rx_mcs_nss = he_rateset->bw80_rx_mcs_nss;
	rateset->he_bw160_tx_mcs_nss = he_rateset->bw160_tx_mcs_nss;
	rateset->he_bw160_rx_mcs_nss = he_rateset->bw160_rx_mcs_nss;
	rateset->he_bw80p80_tx_mcs_nss = he_rateset->bw80p80_tx_mcs_nss;
	rateset->he_bw80p80_rx_mcs_nss = he_rateset->bw80p80_rx_mcs_nss;
}

static void
wlc_he_cp_he_rateset_to_bi_sup(wlc_bss_info_t *bi, wlc_rateset_t *rateset)
{
	bi->he_sup_bw80_tx_mcs = rateset->he_bw80_tx_mcs_nss;
	bi->he_sup_bw80_rx_mcs = rateset->he_bw80_rx_mcs_nss;
	bi->he_sup_bw160_tx_mcs = rateset->he_bw160_tx_mcs_nss;
	bi->he_sup_bw160_rx_mcs = rateset->he_bw160_rx_mcs_nss;
	bi->he_sup_bw80p80_tx_mcs = rateset->he_bw80p80_tx_mcs_nss;
	bi->he_sup_bw80p80_rx_mcs = rateset->he_bw80p80_rx_mcs_nss;
}

static void
wlc_he_init_bi_rateset_to_none(wlc_bss_info_t *bi)
{
	bi->he_neg_bw80_tx_mcs = HE_CAP_MAX_MCS_NONE_ALL;
	bi->he_neg_bw80_rx_mcs = HE_CAP_MAX_MCS_NONE_ALL;
	bi->he_neg_bw160_tx_mcs = HE_CAP_MAX_MCS_NONE_ALL;
	bi->he_neg_bw160_rx_mcs = HE_CAP_MAX_MCS_NONE_ALL;
	bi->he_neg_bw80p80_tx_mcs = HE_CAP_MAX_MCS_NONE_ALL;
	bi->he_neg_bw80p80_rx_mcs = HE_CAP_MAX_MCS_NONE_ALL;
}

static void
wlc_he_remove_1024qam(wlc_info_t *wlc, uint8 nstreams, uint16 *he_mcs_nss)
{
	uint8 nss, mcs_code;
	for (nss = 1; nss <= nstreams; nss++) {
		mcs_code = HE_CAP_MAX_MCS_NSS_GET_MCS(nss, *he_mcs_nss);
		if (mcs_code == HE_CAP_MAX_MCS_0_11) {
			HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_0_9, *he_mcs_nss);
		}
	}
}

/*
 * Intersect HE Cap mcsmaps with that of peer's and
 * update rateset with BW, Tx and Rx limitations
 */
static void
wlc_he_intersect_txrxmcsmaps(wlc_he_info_t *hei, scb_t *scb, bool bw_160,
		bool isomi, uint txlimit, uint rxlimit)
{
	scb_he_t *sh;
	wlc_rateset_t *scb_rs;
	uint nss;
	uint16 txmcsmap80, rxmcsmap80, txmcsmap160, rxmcsmap160, txmcsmap80p80, rxmcsmap80p80;

	sh = SCB_HE(hei, scb);
	ASSERT(sh != NULL);

	/* First copy the ratesets, apply possible BW limitations */
	scb_rs = &scb->rateset;

	/* Update tx and rx mcs maps based on intersection of Our cap with SCB's.
	 * Intersection of our Tx and SCB's Rx to get our Tx; Our Rx and SCB's Tx to get our Rx.
	 */
	txmcsmap80 = wlc_he_rateset_intersection(hei->he_rateset.bw80_tx_mcs_nss,
			sh->bw80_rx_mcs_nss);
	rxmcsmap80 = wlc_he_rateset_intersection(hei->he_rateset.bw80_rx_mcs_nss,
			sh->bw80_tx_mcs_nss);
	if (bw_160) {
		txmcsmap160 = wlc_he_rateset_intersection(hei->he_rateset.bw160_tx_mcs_nss,
				sh->bw160_rx_mcs_nss);
		rxmcsmap160 = wlc_he_rateset_intersection(hei->he_rateset.bw160_rx_mcs_nss,
				sh->bw160_tx_mcs_nss);
		txmcsmap80p80 = wlc_he_rateset_intersection(hei->he_rateset.bw80p80_tx_mcs_nss,
				sh->bw80p80_rx_mcs_nss);
		rxmcsmap80p80 = wlc_he_rateset_intersection(hei->he_rateset.bw80p80_rx_mcs_nss,
				sh->bw80p80_tx_mcs_nss);
	} else {
		txmcsmap160 = HE_CAP_MAX_MCS_NONE_ALL;
		rxmcsmap160 = HE_CAP_MAX_MCS_NONE_ALL;
		txmcsmap80p80 = HE_CAP_MAX_MCS_NONE_ALL;
		rxmcsmap80p80 = HE_CAP_MAX_MCS_NONE_ALL;
	}

	/*
	 * Update mcs maps again based on intersection of previous result and
	 * default rateset if band default rateset had changed by user.
	 */
	if (hei->wlc->defrateset_override) {
		enum wlc_bandunit bandunit = hei->wlc->band->bandunit;
		wlc_rateset_t *def_rs = &hei->wlc->bandstate[bandunit]->defrateset;

		txmcsmap80 = wlc_he_rateset_intersection(txmcsmap80, def_rs->he_bw80_tx_mcs_nss);
		rxmcsmap80 = wlc_he_rateset_intersection(rxmcsmap80, def_rs->he_bw80_rx_mcs_nss);
		if (bw_160) {
			txmcsmap160 = wlc_he_rateset_intersection(txmcsmap160,
				def_rs->he_bw160_tx_mcs_nss);
			rxmcsmap160 = wlc_he_rateset_intersection(rxmcsmap160,
				def_rs->he_bw160_rx_mcs_nss);
			txmcsmap80p80 = wlc_he_rateset_intersection(txmcsmap80p80,
				def_rs->he_bw80p80_tx_mcs_nss);
			rxmcsmap80p80 = wlc_he_rateset_intersection(rxmcsmap80p80,
				def_rs->he_bw80p80_rx_mcs_nss);
		}
	}

	/* Now apply NSS txlimit to get final tx rateset */
	for (nss = txlimit + 1; nss <= HE_CAP_MCS_MAP_NSS_MAX; nss++) {
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, txmcsmap80);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, txmcsmap160);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, txmcsmap80p80);
	}

	if (!isomi) {
		/* Now apply the rx chain limits */
		for (nss = rxlimit + 1; nss <= HE_CAP_MCS_MAP_NSS_MAX; nss++) {
			HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rxmcsmap80);
			HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rxmcsmap160);
			HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rxmcsmap80p80);
		}
	}
	scb_rs->he_bw80_tx_mcs_nss = txmcsmap80;
	scb_rs->he_bw80_rx_mcs_nss = rxmcsmap80;
	scb_rs->he_bw160_tx_mcs_nss = txmcsmap160;
	scb_rs->he_bw160_rx_mcs_nss = rxmcsmap160;
	scb_rs->he_bw80p80_tx_mcs_nss = txmcsmap80p80;
	scb_rs->he_bw80p80_rx_mcs_nss = rxmcsmap80p80;

	WL_RATE(("wl%d: %s txmcsmap80 0x%x rxmcsmap80 0x%x txmcsmap160 0x%x rxmcsmap160 0x%x"
			" txmcsmap80p80 0x%x rxmcsmap80p80 0x%x\n",
			hei->wlc->pub->unit, __FUNCTION__, txmcsmap80, rxmcsmap80, txmcsmap160,
			rxmcsmap160, txmcsmap80p80, rxmcsmap80p80));
}

/**
 * initialize the given rateset with HE defaults based upon band type.
 */
static void
wlc_he_init_rateset(wlc_he_info_t *hei, wlc_rateset_t *rateset, enum wlc_bandunit bandunit,
                    wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc =  hei->wlc;
	uint nss;
	uint rxstreams = wlc_stf_rxstreams_get(wlc);

	BCM_REFERENCE(cfg);

	if (!HE_ENAB_BAND(wlc->pub, wlc->bandstate[bandunit]->bandtype)) {
		wlc_rateset_he_none_all(rateset);
		return;
	}

	wlc_he_cp_rateset_to_wlc(&hei->he_rateset, rateset);
	/* In the 2G band only 80Mhz rateset is possible/allowed */
	if (bandunit == BAND_2G_INDEX) {
		rateset->he_bw160_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
		rateset->he_bw160_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
		rateset->he_bw80p80_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
		rateset->he_bw80p80_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	}
	/* Now apply the rx and tx chain limits */
	for (nss = rxstreams + 1; nss <= HE_CAP_MCS_MAP_NSS_MAX; nss++) {
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw80_rx_mcs_nss);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw160_rx_mcs_nss);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw80p80_rx_mcs_nss);
	}
	for (nss = WLC_BITSCNT(wlc->stf->txchain) + 1; nss <= HE_CAP_MCS_MAP_NSS_MAX; nss++) {
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw80_tx_mcs_nss);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw160_tx_mcs_nss);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw80p80_tx_mcs_nss);
	}
	/* Apply possible external limit (wl rateset) */
	rateset->he_bw80_tx_mcs_nss = wlc_he_rateset_intersection(rateset->he_bw80_tx_mcs_nss,
		hei->rateset_filter.he_bw80_tx_mcs_nss);
	rateset->he_bw80_rx_mcs_nss = wlc_he_rateset_intersection(rateset->he_bw80_rx_mcs_nss,
		hei->rateset_filter.he_bw80_rx_mcs_nss);
	rateset->he_bw160_tx_mcs_nss = wlc_he_rateset_intersection(rateset->he_bw160_tx_mcs_nss,
		hei->rateset_filter.he_bw160_tx_mcs_nss);
	rateset->he_bw160_rx_mcs_nss = wlc_he_rateset_intersection(rateset->he_bw160_rx_mcs_nss,
		hei->rateset_filter.he_bw160_rx_mcs_nss);
	rateset->he_bw80p80_tx_mcs_nss = wlc_he_rateset_intersection(rateset->he_bw80p80_tx_mcs_nss,
		hei->rateset_filter.he_bw80p80_tx_mcs_nss);
	rateset->he_bw80p80_rx_mcs_nss = wlc_he_rateset_intersection(rateset->he_bw80p80_rx_mcs_nss,
		hei->rateset_filter.he_bw80p80_rx_mcs_nss);
}

/**
 * Apply OMI settings on rateset from scb hei and update the scb rateset. OMI params are based upon
 * received params: BW is of type DOT11_OPER_MODE_.. omi_rxnss inidicates the remote Rx caps and
 * determines our tx nss, where 0 means 1 nss and max is 7 (indicating 8 nss).
 *
 */
static void
wlc_he_apply_omi(wlc_info_t *wlc, scb_t *scb, uint8 omi_bw, uint8 omi_rxnss, bool update_vht)
{
	wlc_he_info_t *hei = wlc->hei;
	uint8 oper_mode;

	/* Intersect and update rateset */
	wlc_he_intersect_txrxmcsmaps(hei, scb, (omi_bw == DOT11_OPER_MODE_160MHZ) ? TRUE : FALSE,
		TRUE, omi_rxnss + 1, 0);

	/* This is all what is needed within HE. The BW limitation of 20 or 40 in the final
	 * rate table will be determined by wlc_scb_ratesel_init. The BW limitation is stored
	 * in (VHT) oper_mode info (even if received through HTC) and gets applied by that setting
	 */
	if (update_vht) {
		/* Determine matching oper_mode for VHT and lower rates. */
		oper_mode = DOT11_D8_OPER_MODE(0, omi_rxnss + 1, 0,
			(omi_bw == DOT11_OPER_MODE_160MHZ) ?  1 : 0,
			(omi_bw == DOT11_OPER_MODE_160MHZ) ? DOT11_OPER_MODE_80MHZ : omi_bw);
		wlc_vht_update_scb_oper_mode(wlc->vhti, scb, oper_mode);
	}
	/* no need to init ratesel as VHT module will do that for us */
}

/**
 * Returns whether or not the capabilities should be limited to 80Mhz operation. This depends on
 * a number of settings from wlc and bsscfg. Return true if limitation should be applied.
 */
static bool
wlc_he_bw80_limited(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	if (BAND_2G(wlc->band->bandtype)) {
		return TRUE;
	}
	/* No 160MHz and 80p80 capability advertisement if
	 * - the locale does not allow 160MHz or
	 * - the band is not configured to support it or
	 * - chanspec was not 160MHz or 80+80Mhz for AP/IBSS
	 */
	if ((wlc_channel_locale_flags(wlc->cmi) & WLC_NO_160MHZ) ||
	    (!WL_BW_CAP_160MHZ(wlc->band->bw_cap))) {
		return TRUE;
	}
	if ((cfg != NULL) && (!cfg->BSS) && !BSSCFG_IS_TDLS(cfg)) {
		return TRUE;
	}

	return FALSE;
}

/**
 * initialize the HE rates for the def_rateset and hw_rateset.
 */
static void
wlc_he_default_ratesets(wlc_he_info_t *hei, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc =  hei->wlc;
	wlc_rateset_t *rateset;
	enum wlc_bandunit bandunit;

	FOREACH_WLC_BAND(wlc, bandunit) {
		rateset = &wlc->bandstate[bandunit]->defrateset;
		wlc_he_init_rateset(hei, rateset, bandunit, cfg);
		rateset = &wlc->bandstate[bandunit]->hw_rateset;
		wlc_he_init_rateset(hei, rateset, bandunit, cfg);
	}
}

/**
 * Update the mcs maps, as an external influence (like tx/rx chain config) may require so.
 */
void
wlc_he_update_mcs_cap(wlc_he_info_t *hei)
{
	wlc_info_t *wlc =  hei->wlc;
	uint8 idx;
	wlc_bsscfg_t *cfg;

	wlc_he_default_ratesets(hei, NULL);
	FOREACH_UP_AP(wlc, idx, cfg) {
		wlc_bss_info_t *bi = cfg->current_bss;
		wlc_he_init_rateset(hei, &bi->rateset, wlc->band->bandunit, cfg);
		wlc_he_cp_he_rateset_to_bi_sup(bi,
			&wlc->bandstate[wlc->band->bandunit]->hw_rateset);
	}
	/* update beacon/probe resp for AP */
	if (wlc->pub->up && AP_ENAB(wlc->pub) && wlc->pub->associated) {
		wlc_update_beacon(wlc);
		wlc_update_probe_resp(wlc, TRUE);
	}
}

/**
 * Set a rateset limit. This will be applied when calculating rateset supported.
 */
void
wlc_he_set_rateset_filter(wlc_he_info_t *hei, wlc_rateset_t *rateset)
{
	hei->rateset_filter = *rateset;
}

/**
 * Retreieve the default/unfiltered by HW supported rateset
 */
void
wlc_he_default_rateset(wlc_he_info_t *hei, wlc_rateset_t *rateset)
{
	wlc_he_cp_rateset_to_wlc(&hei->he_rateset, rateset);
}

/**
 * Initialize supported HE capabilities, this function is called from wlc.c from wlc_attach. It
 * should initialize all HE capabilites supported by the device. Stored in HE information store
 * in the wlc struct.
 */
int
BCMATTACHFN(wlc_he_init_defaults)(wlc_he_info_t *hei)
{
	wlc_info_t *wlc;
	uint32 channel_width_set;
	uint16 def_6g_caps_info = 0;

	wlc = hei->wlc;

	/* No limit on external configured rateset */
	hei->rateset_filter.he_bw80_tx_mcs_nss = HE_CAP_MAX_MCS_MAP_0_11_NSS4;
	hei->rateset_filter.he_bw80_rx_mcs_nss = HE_CAP_MAX_MCS_MAP_0_11_NSS4;
	hei->rateset_filter.he_bw160_tx_mcs_nss = HE_CAP_MAX_MCS_MAP_0_11_NSS4;
	hei->rateset_filter.he_bw160_rx_mcs_nss = HE_CAP_MAX_MCS_MAP_0_11_NSS4;
	hei->rateset_filter.he_bw80p80_tx_mcs_nss = HE_CAP_MAX_MCS_MAP_0_11_NSS4;
	hei->rateset_filter.he_bw80p80_rx_mcs_nss = HE_CAP_MAX_MCS_MAP_0_11_NSS4;

	setbits((uint8 *)&def_6g_caps_info, sizeof(def_6g_caps_info),
		HE_6G_CAP_MIN_MPDU_START_SPC_IDX, HE_6G_CAP_MIN_MPDU_START_SPC_FSZ,
		0); /* no AMPDU density restriction for local node */
	setbits((uint8 *)&def_6g_caps_info, sizeof(def_6g_caps_info),
		HE_6G_CAP_MAX_AMPDU_LEN_EXP_IDX, HE_6G_CAP_MAX_AMPDU_LEN_EXP_FSZ,
		wlc_ampdu_get_rx_factor(wlc));
	setbits((uint8 *)&def_6g_caps_info, sizeof(def_6g_caps_info),
		HE_6G_CAP_SM_PWR_SAVE_IDX, HE_6G_CAP_SM_PWR_SAVE_FSZ,
		HT_CAP_MIMO_PS_OFF);
	setbits((uint8 *)&def_6g_caps_info, sizeof(def_6g_caps_info),
		HE_6G_CAP_MAX_MPDU_LEN_IDX, HE_6G_CAP_MAX_MPDU_LEN_FSZ,
		(D11REV_LT(wlc->pub->corerev, 132) ? VHT_CAP_MPDU_MAX_8K : VHT_CAP_MPDU_MAX_11K));
	/* HE extended capabilities element */
	hei->def_6g_caps_info = def_6g_caps_info;

	/* Populate HE MCS-NSS-BW cap rateset */
	phy_hecap_get_rateset(WLC_PI(wlc), &hei->he_rateset);

	/* Remove 1024QAM cap if needed */
	if (BCM1024QAM_DSAB(wlc)) {
		wlc_he_remove_1024qam(wlc, wlc->stf->txstreams,
			&hei->he_rateset.bw80_tx_mcs_nss);
		wlc_he_remove_1024qam(wlc, wlc->stf->txstreams,
			&hei->he_rateset.bw160_tx_mcs_nss);
		wlc_he_remove_1024qam(wlc, wlc->stf->txstreams,
			&hei->he_rateset.bw80p80_tx_mcs_nss);
		wlc_he_remove_1024qam(wlc, wlc->stf->rxstreams,
			&hei->he_rateset.bw80_rx_mcs_nss);
		wlc_he_remove_1024qam(wlc, wlc->stf->rxstreams,
			&hei->he_rateset.bw160_rx_mcs_nss);
		wlc_he_remove_1024qam(wlc, wlc->stf->rxstreams,
			&hei->he_rateset.bw80p80_rx_mcs_nss);
	}

#ifdef HERTDBG
	wlc_he_print_rateset(&hei->he_rateset);
#endif /* HERTDBG */

	/* Initialize band-specific default HE rateset */
	wlc_he_default_ratesets(hei, NULL);

	/* Initialize MAC Capabilities */
	wlc_he_maccap_init(hei);

	hei->ppet = phy_hecap_get_ppet(WLC_PI(wlc));

	/* Initialize PHY Capabilities */
	phy_hecap_fill_phycap_info(WLC_PI(wlc), &hei->def_phy_cap);

	/* Determine the rateset size based upon PHY capabilities. Min is 4 */
	hei->he_ratesetsz = HE_MIN_HE_RATESETSZ; /* BW 80 TxRx always encoded */
	/* From std: The Rx HE-MCS Map 160 MHz subfield is present if B2 of the Channel Width Set
	 * subfield of the HE PHY Capabilities Information field is set to 1
	 */
	channel_width_set = getbits((uint8*)&hei->def_phy_cap, sizeof(hei->def_phy_cap),
		HE_PHY_CH_WIDTH_SET_IDX, HE_PHY_CH_WIDTH_SET_FSZ);
	if (channel_width_set & HE_PHY_CH_WIDTH_5G_160)
		hei->he_ratesetsz += HE_BW_RASETSZ; /* BW 160 TxRx encoded */
	/* From std: The Rx HE-MCS Map 80+80 MHz subfield is present if B3 of the Channel Width Set
	 * subfield of the HE PHY Capabilities Information field is set to 1
	 */
	if (channel_width_set & HE_PHY_CH_WIDTH_5G_80P80) {
		/* 80p80 is only allowed if 160 is supported: */
		ASSERT(hei->he_ratesetsz == (HE_BW_RASETSZ + HE_BW_RASETSZ));
		hei->he_ratesetsz += HE_BW_RASETSZ; /* BW 80p80 TxRx encoded */
	}

	return BCME_OK;
} /* wlc_he_init_defaults */

static void
wlc_he_fill_ppe_thresholds_band(wlc_he_info_t *hei, wlc_he_ppet_t *ppe_ths, uint8 ppet,
	uint8 max_nss_supported, uint8 max_rucount)
{
	uint nss, ru_index;
	uint8 ppet8;
	uint8 ppet16;
	uint8 ru_bitmap = NBITMASK(max_rucount);

	if (ppet == WL_HE_PPET_0US) {
		ppe_ths->ppe_ths_len = 0;
		return;
	}

	ppe_ths->ppe_ths_len = HE_PPE_THRESH_LEN(max_nss_supported, max_rucount);
	memset(ppe_ths->ppe_ths, 0, HE_MAX_PPET_SIZE);

	/* Store total nss (-1) in IE */
	setbits(ppe_ths->ppe_ths, ppe_ths->ppe_ths_len, HE_NSSM1_IDX, HE_NSSM1_LEN,
		max_nss_supported - 1);
	/* Store ru bitmask */
	setbits(ppe_ths->ppe_ths, ppe_ths->ppe_ths_len, HE_RU_INDEX_MASK_IDX, HE_RU_INDEX_MASK_LEN,
		ru_bitmap);

	if (ppet == WL_HE_PPET_8US) {
		ppet8 = HE_CONST_IDX_BPSK;
		ppet16 = HE_CONST_IDX_NONE;
	} else { /* WL_HE_PPET_16US */
		ppet8 = HE_CONST_IDX_NONE;
		ppet16 = HE_CONST_IDX_BPSK;
	}

	for (nss = 0; nss < max_nss_supported; nss++) {
		for (ru_index = 0; ru_index < max_rucount; ru_index++) {
			setbits(ppe_ths->ppe_ths, ppe_ths->ppe_ths_len,
				HE_PPET16_BIT_OFFSET(max_rucount, nss, ru_index),
				HE_PPE_THRESH_NSS_RU_FSZ, ppet16);
			setbits(ppe_ths->ppe_ths, ppe_ths->ppe_ths_len,
				HE_PPET8_BIT_OFFSET(max_rucount, nss, ru_index),
				HE_PPE_THRESH_NSS_RU_FSZ, ppet8);
		}
	}
}

static void
wlc_he_fill_ppe_thresholds(wlc_he_info_t *hei)
{
	wlc_info_t *wlc = hei->wlc;
	uint8 max_nss_supported;
	uint8 ppet;

	/* Take the override value if set */
	if (hei->ppet_override == WL_HE_PPET_AUTO) {
		ppet = hei->ppet;
	} else {
		ppet = hei->ppet_override;
	}

	/* take rxchain to specify remote what ppet values per nss to use. */
	max_nss_supported = wlc_stf_rxstreams_get(wlc);

	/* 2G band */
	wlc_he_fill_ppe_thresholds_band(hei, &hei->ppe_ths_2g, ppet, max_nss_supported,
		HE_PPET_MAX_RUCOUNT_2G);
	/* other bands */
	wlc_he_fill_ppe_thresholds_band(hei, &hei->ppe_ths_5g6g, ppet, max_nss_supported,
		HE_PPET_MAX_RUCOUNT);
}

/* get current cap info this node uses */
uint8
wlc_he_get_bfe_ndp_recvstreams(wlc_he_info_t *hei)
{
	uint8 bfe_sts = 0;

	bfe_sts = getbits((uint8*)&hei->def_phy_cap, sizeof(hei->def_phy_cap),
		HE_PHY_BEAMFORMEE_STS_BELOW80MHZ_IDX, HE_PHY_BEAMFORMEE_STS_BELOW80MHZ_FSZ);

	return bfe_sts;
}

/* get my bfr sts */
uint8
wlc_he_get_bfr_ndp_sts(wlc_he_info_t *hei, bool is_bw160)
{
	uint8 bfr_sts;

	if (is_bw160) {
		bfr_sts = getbits((uint8*)&hei->def_phy_cap, sizeof(hei->def_phy_cap),
			HE_PHY_SOUND_DIM_ABOVE80MHZ_IDX, HE_PHY_SOUND_DIM_ABOVE80MHZ_FSZ);
	} else {
		bfr_sts = getbits((uint8*)&hei->def_phy_cap, sizeof(hei->def_phy_cap),
			HE_PHY_SOUND_DIM_BELOW80MHZ_IDX, HE_PHY_SOUND_DIM_BELOW80MHZ_FSZ);
	}

	return bfr_sts;
}

/* get my mu feedback ng16 phy cap */
uint8
wlc_he_get_phy_mu_feedback_ng16(wlc_he_info_t *hei)
{
	return getbits((uint8*)&hei->def_phy_cap, sizeof(hei->def_phy_cap),
		HE_PHY_MU_FEEDBACK_NG16_SUPPORT_IDX, HE_PHY_MU_FEEDBACK_NG16_SUPPORT_FSZ);
}

static void
wlc_he_ap_maccap_init(wlc_he_info_t *hei, he_mac_cap_t *ap_mac_cap)
{
	wlc_info_t *wlc = hei->wlc;
	uint8 frag_cap;

	memcpy(ap_mac_cap, &hei->def_mac_cap, sizeof(*ap_mac_cap));

	frag_cap = wlc_he_get_dynfrag(wlc);
	/* b3-b4: Fragmentation Support field */
	setbits((uint8 *)ap_mac_cap, sizeof(*ap_mac_cap), HE_MAC_FRAG_SUPPORT_IDX,
		HE_MAC_FRAG_SUPPORT_FSZ, frag_cap);
	if ((frag_cap == HE_MAC_FRAG_ONE_PER_AMPDU) || (frag_cap == HE_MAC_FRAG_MULTI_PER_AMPDU)) {
		/* b5-b7: Max. Number of fragmented MSDUs */
		setbits((uint8 *)ap_mac_cap, sizeof(*ap_mac_cap), HE_MAC_MAX_MSDU_FRAGS_IDX,
			HE_MAC_MAX_MSDU_FRAGS_FSZ, WLC_HE_MAX_MSDU_FRAG_ENC);

		/* bit 8-9: Min. payload size of first fragment */
		setbits((uint8 *)ap_mac_cap, sizeof(*ap_mac_cap), HE_MAC_MIN_FRAG_SIZE_IDX,
			HE_MAC_MIN_FRAG_SIZE_FSZ, HE_MAC_MINFRAG_NO_RESTRICT);
	} else {
		/* b5-b7: Max. Number of fragmented MSDUs */
		setbits((uint8 *)ap_mac_cap, sizeof(*ap_mac_cap), HE_MAC_MAX_MSDU_FRAGS_IDX,
			HE_MAC_MAX_MSDU_FRAGS_FSZ, 0);

		/* bit 8-9: Min. payload size of first fragment */
		setbits((uint8 *)ap_mac_cap, sizeof(*ap_mac_cap), HE_MAC_MIN_FRAG_SIZE_IDX,
			HE_MAC_MIN_FRAG_SIZE_FSZ, 0);
	}

	/* b10-b11: Trigger frame mac padding duration is 0 for AP */

	/* b19: AP can receive A-BSR support */
	if (hei->bsr_supported) {
		setbit((uint8 *)ap_mac_cap, HE_MAC_A_BSR_IDX);
	}

	/* b24: Group Addressed Multi-STA Block-Ack in DL MU Support, is 0 for AP */

	/* b25: OM Control Support, always 1 for AP */
	setbit((uint8 *)ap_mac_cap, HE_MAC_OM_CONTROL_SUPPORT_IDX);

	/* b26: OFDMA RA Support */

	/* b26: AP supports sending Trigger to allocate random RUs */

	/* b29: A-MSDU Fragmentation Support. */
	if ((frag_cap == HE_MAC_FRAG_ONE_PER_AMPDU) || (frag_cap == HE_MAC_FRAG_MULTI_PER_AMPDU)) {
		setbit((uint8 *)ap_mac_cap, HE_MAC_AMSDU_FRAG_SUPPORT_IDX);
	} else {
		clrbit((uint8 *)ap_mac_cap, HE_MAC_AMSDU_FRAG_SUPPORT_IDX);
	}

	/* b31: Rx Control Frame to MultiBSS is 0 for AP. */

	/* b34: BQR receive Support. */

	/* b44: OM Ctrl UL MU Data Disable RX, supported. */
	setbit((uint8 *)ap_mac_cap, HE_MAC_OM_UL_MU_DATA_DISABLE_IDX);
}

static void
wlc_he_sta_maccap_init(wlc_he_info_t *hei, he_mac_cap_t *sta_mac_cap)
{
	wlc_info_t *wlc = hei->wlc;
	uint8 frag_cap;

	memcpy(sta_mac_cap, &hei->def_mac_cap, sizeof(*sta_mac_cap));

	frag_cap = wlc_he_get_dynfrag(wlc);
	/* b3-b4: Fragmentation Support field */
	setbits((uint8 *)sta_mac_cap, sizeof(*sta_mac_cap), HE_MAC_FRAG_SUPPORT_IDX,
		HE_MAC_FRAG_SUPPORT_FSZ, frag_cap);
	if ((frag_cap == HE_MAC_FRAG_ONE_PER_AMPDU) || (frag_cap == HE_MAC_FRAG_MULTI_PER_AMPDU)) {
		/* b5-b7: Max. Number of fragmented MSDUs */
		setbits((uint8 *)sta_mac_cap, sizeof(*sta_mac_cap), HE_MAC_MAX_MSDU_FRAGS_IDX,
			HE_MAC_MAX_MSDU_FRAGS_FSZ, WLC_HE_MAX_MSDU_FRAG_ENC);

		/* bit 8-9: Min. payload size of first fragment */
		setbits((uint8 *)sta_mac_cap, sizeof(*sta_mac_cap), HE_MAC_MIN_FRAG_SIZE_IDX,
			HE_MAC_MIN_FRAG_SIZE_FSZ, HE_MAC_MINFRAG_NO_RESTRICT);
	} else {
		/* b5-b7: Max. Number of fragmented MSDUs */
		setbits((uint8 *)sta_mac_cap, sizeof(*sta_mac_cap), HE_MAC_MAX_MSDU_FRAGS_IDX,
			HE_MAC_MAX_MSDU_FRAGS_FSZ, 0);

		/* bit 8-9: Min. payload size of first fragment */
		setbits((uint8 *)sta_mac_cap, sizeof(*sta_mac_cap), HE_MAC_MIN_FRAG_SIZE_IDX,
			HE_MAC_MIN_FRAG_SIZE_FSZ, 0);
	}

	/* b10-b11: Trigger frame mac padding duration, set to max for moment, not 100% sure what
	 * was found as safe value, initially 16usec was not even enough...
	 */
	setbits((uint8 *)sta_mac_cap, sizeof(*sta_mac_cap), HE_MAC_TRG_PAD_DUR_IDX,
		HE_MAC_TRG_PAD_DUR_IDX, HE_MAC_TRG_PAD_DUR_16);

	/* b19: STA supports sending A-control with AMPDU BSR  */

	/* b24: Group Addressed Multi-STA Block-Ack in DL MU Support, not supported in a0 */

	/* b25: OM Control Support. Disabled for STA */
	clrbit((uint8 *)sta_mac_cap, HE_MAC_OM_CONTROL_SUPPORT_IDX);

	/* b26: OFDMA RA Support */

	/* b29: A-MSDU Fragmentation Support. */
	if ((frag_cap == HE_MAC_FRAG_ONE_PER_AMPDU) || (frag_cap == HE_MAC_FRAG_MULTI_PER_AMPDU)) {
		setbit((uint8 *)sta_mac_cap, HE_MAC_AMSDU_FRAG_SUPPORT_IDX);
	} else {
		clrbit((uint8 *)sta_mac_cap, HE_MAC_AMSDU_FRAG_SUPPORT_IDX);
	}

	/* b31: Rx Control Frame to MultiBSS. */

	/* b34: BQR transmit Support. */

	/* b36: NDP Feedback Report Support */

	/* b38: A-MSDU In A-MPDU Support */
}

static void
BCMATTACHFN(wlc_he_maccap_init)(wlc_he_info_t *hei)
{
	wlc_info_t *wlc = hei->wlc;
	he_mac_cap_t *mac_cap = &hei->def_mac_cap;
	uint8 frag_cap;
	bzero(mac_cap, sizeof(*mac_cap));

	/* Initialize common HE MAC capabilities for STA & AP */

	/* b0: +HTC-HE Support */
	if (D11REV_GE(wlc->pub->corerev, 129)) {
		setbit((uint8 *)mac_cap, HE_MAC_HTC_HE_SUPPORT_IDX);
	}

	frag_cap = wlc_he_get_dynfrag(wlc);
	/* b3-b4: Fragmentation Support field */
	setbits((uint8 *)mac_cap, sizeof(*mac_cap), HE_MAC_FRAG_SUPPORT_IDX,
		HE_MAC_FRAG_SUPPORT_FSZ, frag_cap);

	if ((frag_cap == HE_MAC_FRAG_ONE_PER_AMPDU) || (frag_cap == HE_MAC_FRAG_MULTI_PER_AMPDU)) {
		/* b5-b7: Max. Number of fragmented MSDUs */
		setbits((uint8 *)mac_cap, sizeof(*mac_cap), HE_MAC_MAX_MSDU_FRAGS_IDX,
			HE_MAC_MAX_MSDU_FRAGS_FSZ, WLC_HE_MAX_MSDU_FRAG_ENC);

		/* bit 8-9: Min. payload size of first fragment */
		setbits((uint8 *)mac_cap, sizeof(*mac_cap), HE_MAC_MIN_FRAG_SIZE_IDX,
			HE_MAC_MIN_FRAG_SIZE_FSZ, HE_MAC_MINFRAG_NO_RESTRICT);
	} else {
		/* b5-b7: Max. Number of fragmented MSDUs */
		setbits((uint8 *)mac_cap, sizeof(*mac_cap), HE_MAC_MAX_MSDU_FRAGS_IDX,
			HE_MAC_MAX_MSDU_FRAGS_FSZ, 0);

		/* bit 8-9: Min. payload size of first fragment */
		setbits((uint8 *)mac_cap, sizeof(*mac_cap), HE_MAC_MIN_FRAG_SIZE_IDX,
			HE_MAC_MIN_FRAG_SIZE_FSZ, 0);
	}

	/* bit 12-14: Max. TIDs that can be aggregated in an AMPDU */
	setbits((uint8 *)mac_cap, sizeof(*mac_cap), HE_MAC_MULTI_TID_AGG_IDX,
		HE_MAC_MULTI_TID_AGG_FSZ, (WLC_HE_MAX_TID_IN_AMPDU - 1));

	/* bit 15-16: HE Link Adaptation Capable: not supported for a0 */

	/* b17: Support Rx. of multi-TID A-MPDU under all ack context */

	/* b18: UMRS support */

	/* b20: See above, TWT support related. */

	/* b21: Supports Rx. of Multi-STA BA with 32-bit BlockAck bitmap */

	/* b22: STA supports participating in an MU Cascading sequence */

	/* b23: Ack-enabled Multi-TID Aggregation Support */

	/* b27-28: Max AMPDU Length Exponent. Setting it to 2 as this is supported by all HW */
	setbits((uint8 *)mac_cap, sizeof(*mac_cap), HE_MAC_MAX_AMPDU_LEN_EXP_IDX,
		HE_MAC_MAX_AMPDU_LEN_EXP_FSZ, 2);

	/* b29: A-MSDU Fragmentation Support. */
	if ((frag_cap == HE_MAC_FRAG_ONE_PER_AMPDU) || (frag_cap == HE_MAC_FRAG_MULTI_PER_AMPDU)) {
		setbit((uint8 *)mac_cap, HE_MAC_AMSDU_FRAG_SUPPORT_IDX);
	} else {
		clrbit((uint8 *)mac_cap, HE_MAC_AMSDU_FRAG_SUPPORT_IDX);
	}

	/* b30: Flexible TWT Schedule Support. */

	/* b32: BSRP A-MPDU Aggregation. */

	/* b33: QTP Support. */

	/* b35: SR Responder. */

	/* b37: OPS Support */

	/* b38: A-MSDU In A-MPDU Support */

	/* b42: HE Subchannel Selective Transmission Support (TWT) */
}

static void
wlc_he_ap_phycap_init(wlc_he_info_t *hei, he_phy_cap_t *ap_phy_cap)
{
	memcpy(ap_phy_cap, &hei->def_phy_cap, sizeof(*ap_phy_cap));

	/* b12: device class. Reserved for AP */
	clrbit((uint8 *)ap_phy_cap, HE_PHY_DEVICE_CLASS_IDX);

	/* b22-b23: Only Full BW UL MU-MIMO Rx. */
	if (!WLC_HE_FEATURES_ULMMU(hei->wlc->pub)) {
		clrbit((uint8 *)ap_phy_cap, HE_PHY_FULL_BW_UL_MU_IDX);
	}

	/* b56-b58: TXBF feedback with Trigger frame */

	/* b60: DL MU-MIMO on Partial BW. Reserved for AP */
	clrbit((uint8 *)ap_phy_cap, HE_PHY_DL_MU_MIMO_PART_BW_IDX);

}

static void
wlc_he_sta_phycap_init(wlc_he_info_t *hei, he_phy_cap_t *sta_phy_cap)
{
	memcpy(sta_phy_cap, &hei->def_phy_cap, sizeof(*sta_phy_cap));

	/* b22-b23: Only Full BW UL MU-MIMO Tx. */

	/* b33: MU Beamformer. valid only for AP */
	clrbit((uint8 *)sta_phy_cap, HE_PHY_MU_BEAMFORMER_IDX);

	/* b56-b58: TXBF feedback with Trigger frame */

}

/* ======== iovar dispatch ======== */

/* FIXME: integrate with iovar subcommand infrastructure
 * which is being designed/implemented...
 */

static int
wlc_he_cmd_enab(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;

	if (set) {
		if (!wlc_he_hw_cap(wlc) || !N_ENAB(wlc->pub)) {
			return BCME_UNSUPPORTED;
		}
		wlc->pub->_he_enab = *(uint8 *)params;
		if (!wlc->pub->_he_enab) {
			wlc_twt_disable(wlc);
		}
	} else {
		*result = wlc->pub->_he_enab;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

static int
wlc_he_cmd_features(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;

	if (set) {
		uint32 features = *(uint32 *)params;
		bool update_fifos = FALSE;

		if (*(int32 *)params == -1) {
			/* Reset to default value */
			features = WL_HE_FEATURES_DEFAULT;
			/* ULOFDMA as STA not supported until 43684c0 */
			if (D11REV_LE(wlc->pub->corerev, 129) &&
				D11MINORREV_LT(wlc->pub->corerev_minor, 2)) {
				features &= ~WL_HE_FEATURES_ULMU_STA;
			}
			/* 6705 (same chipid as 43692) and 43692 do not support MU */
			if (CHIPID(si_chipid(wlc->pub->sih)) == BCM43692_CHIP_ID) {
				features &= ~WL_HE_FEATURES_DLMMU;
			}
		}
		if (features & (~WL_HE_MAX_ALLOWED_FEATURES)) {
			return BCME_UNSUPPORTED;
		}
		if (((features & (WL_HE_FEATURES_ULOMU | WL_HE_FEATURES_DLOMU)) != 0) &&
			((features & (WL_HE_FEATURES_5G | WL_HE_FEATURES_2G)) == 0)) {
			return BCME_EPERM; /* not a valid combination */
		}
		if ((features & WL_HE_FEATURES_ULMMU) != 0 &&
			(!getbits(wlc->hei->def_phy_cap, sizeof(wlc->hei->def_phy_cap),
				HE_PHY_FULL_BW_UL_MU_IDX, HE_PHY_FULL_BW_UL_MU_FSZ))) {
			return BCME_UNSUPPORTED;
		}
#if !defined(BCMDBG) && !defined(WLTEST)
		if (((features & WL_HE_FEATURES_ULOMU) != 0) &&
			((features & WL_HE_FEATURES_DLOMU) == 0)) {
			return BCME_EPERM; /* not a valid combination */
		}
#endif

		if (D11REV_LE(wlc->pub->corerev, 129) &&
			D11MINORREV_LT(wlc->pub->corerev_minor, 2) &&
			((features & WL_HE_FEATURES_ULMU_STA) != 0)) {
			/* not supported until 43684c0 */
			return BCME_UNSUPPORTED;
		}

		/* BCM6705 (same chipid as 43692) and 43692 do not support MU */
		if ((CHIPID(si_chipid(wlc->pub->sih)) == BCM43692_CHIP_ID) &&
			((features & WL_HE_FEATURES_DLMMU) != 0)) {
			return BCME_UNSUPPORTED;
		}

		/* Only 6715 supports ER SU */
		if ((features & WL_HE_FEATURES_ERSU_RX) &&
			(!BCM6715_CHIP(si_chipid(wlc->pub->sih)))) {

			return BCME_UNSUPPORTED;
		}

#ifdef WLTAF
		/* For 6715, if TAF UL is enabled, ULMMU cannot be enabled */
		if ((features & WL_HE_FEATURES_ULMMU) &&
			BCM6715_CHIP(si_chipid(wlc->pub->sih)) &&
			wlc_taf_ul_enabled(wlc->taf_handle)) {

			return BCME_UNSUPPORTED;
		}
#endif /* WLTAF */

		update_fifos = ((features & WL_HE_FEATURES_DLOMU) !=
			(wlc->pub->he_features & WL_HE_FEATURES_DLOMU));
		wlc->pub->he_features = features;
		if (update_fifos) {
			/* The number of active TX FIFOs may have changed */
			wlc_hw_update_nfifo(wlc->hw);
		}
	} else {
		*(uint32 *)result = wlc->pub->he_features;
		*rlen = sizeof(*(uint32 *)result);
	}

	return BCME_OK;
}

static int
wlc_he_configure_new_bsscolor(wlc_he_info_t *hei, uint8 new_color, uint8 disabled,
	uint8 switch_count)
{
	wlc_info_t *wlc = hei->wlc;
	wlc_he_color_collision_t *cc = &hei->cc;
	int idx;
	wlc_bsscfg_t *bsscfg;
	bss_he_info_t *bhei;
	bool bss_up;

	if ((hei->update_bsscolor_counter) && (switch_count) && !(disabled))
		return BCME_BUSY;

	/* Clear some of the color collision counters as we are handling color update/change */
	cc->remote_collision_detect_cnt = 0;
	cc->local_collision_detect_cnt = 0;
	cc->detect_report_cnt = 0;
	cc->detect_local_cnt = 0;

	/* Special case, when switch_count is set to 0 then force update of color at once. This
	 * allows for testing colors in all modes (pkteng).
	 */
	if (switch_count == 0) {
		hei->bsscolor_disable = disabled;
		hei->new_bsscolor = new_color;
		hei->bsscolor = new_color;
		wlc_he_configure_bsscolor_phy(hei, NULL);
		FOREACH_BSS(wlc, idx, bsscfg) {
			bhei = BSS_HE_INFO(hei, bsscfg);
			bhei->bsscolor = new_color;
			bhei->bsscolor_disable = disabled;
			if (BSSCFG_AP(bsscfg)) {
				wlc_bss_update_beacon(wlc, bsscfg, FALSE);
				wlc_bss_update_probe_resp(wlc, bsscfg, TRUE);
			}
			if (RATELINKMEM_ENAB(wlc->pub)) {
				wlc_ratelinkmem_update_link_entry_all(wlc, bsscfg, FALSE,
					FALSE /* clr_txbf_stats=0 in mreq */);
			}
		}
	} else {
		hei->bsscolor_counter = switch_count;
		hei->bsscolor_disable = FALSE;
		bss_up = FALSE;
		FOREACH_UP_AP(wlc, idx, bsscfg) {
			bss_up = TRUE;
			break;
		}
		if (disabled) {
			hei->bsscolor_disable = TRUE;
			if (bss_up) {
				wlc_he_configure_bsscolor_phy(hei, NULL);
				wlc_update_beacon(hei->wlc);
				wlc_update_probe_resp(hei->wlc, TRUE);
			}
		} else if (bss_up) {
			hei->update_bsscolor_counter = switch_count;
			hei->new_bsscolor = new_color;
		} else {
			hei->new_bsscolor = new_color;
			hei->bsscolor = new_color;
		}
		wlc_he_configure_bsscolor_phy(hei, wlc->main_ap_bsscfg);
	}
	return BCME_OK;
}

/** called on './wl he bsscolor' */
static int
wlc_he_cmd_bsscolor(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	bss_he_info_t *bhei;
	wl_he_bsscolor_t *bsscolor;
	int ret_code = BCME_OK;

	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);

	if (set) {
		bsscolor = (wl_he_bsscolor_t *)params;
		if (!BSSCFG_AP(cfg) && bsscolor->switch_count) {
			ret_code = BCME_NOTAP;
		} else if (!wlc_he_hw_cap(wlc)) {
			ret_code = BCME_UNSUPPORTED;
		} else if (bsscolor->color > HE_BSSCOLOR_MAX_VALUE) {
			ret_code = BCME_BADARG;
		} else if ((bsscolor->color != hei->bsscolor) ||
			(bsscolor->disabled != hei->bsscolor_disable) ||
			(bsscolor->switch_count == 0)) {
			ret_code = wlc_he_configure_new_bsscolor(hei, bsscolor->color,
				bsscolor->disabled, bsscolor->switch_count);
		}
	} else {
		bsscolor = (wl_he_bsscolor_t *)result;
		if (BSSCFG_AP(cfg)) {
			bsscolor->color = hei->bsscolor;
			bsscolor->disabled = hei->bsscolor_disable;
		} else {
			bhei = BSS_HE_INFO(hei, cfg);
			bsscolor->color = bhei->bsscolor;
			bsscolor->disabled = bhei->bsscolor_disable;
		}
		bsscolor->switch_count = hei->bsscolor_counter;
		*rlen = sizeof(*bsscolor);
	}

	return ret_code;
}

static int
wlc_he_cmd_rtsdurthresh(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	bss_he_info_t *bhei;
	uint16 new_val = *(uint16 *)params;

	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);
	bhei = BSS_HE_INFO(hei, cfg);

	if (set) {
		if (!BSSCFG_AP(cfg)) {
			return BCME_NOTAP;
		}
		if (!wlc_he_hw_cap(wlc)) {
			return BCME_UNSUPPORTED;
		}
		if (new_val != bhei->rts_thresh) {
			if ((new_val >> HE_RTS_DUR_THRESHOLD_32USEC_SHIFT) >=
				HE_RTS_THRES_DISABLED) {
				new_val = BCM_UINT16_MAX;
			}
			bhei->rts_thresh = new_val;
			/* update beacon and probe response templates */
			if (cfg->up) {
				wlc_bss_update_beacon(wlc, cfg, FALSE);
				wlc_bss_update_probe_resp(wlc, cfg, TRUE);
			}
		}
	} else {
		if (bhei->rts_thresh == BCM_UINT16_MAX) {
			*(uint16 *)result = HE_RTS_THRES_DISABLED;
		}
		else {
			*(uint16 *)result = bhei->rts_thresh;
		}
		*rlen = sizeof(*(uint16*)result);
	}

	return BCME_OK;
}

static int
wlc_he_cmd_peduration(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	bss_he_info_t *bhei;
	uint8 new_val = *(uint8 *)params;

	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);
	bhei = BSS_HE_INFO(hei, cfg);

	if (set) {
		if (!BSSCFG_AP(cfg)) {
			return BCME_NOTAP;
		}
		if (!wlc_he_hw_cap(wlc)) {
			return BCME_UNSUPPORTED;
		}
		if (new_val != bhei->pe_duration) {
			bhei->pe_duration = new_val;
			/* update beacon and probe response templates */
			if (cfg->up) {
				wlc_bss_update_beacon(wlc, cfg, FALSE);
				wlc_bss_update_probe_resp(wlc, cfg, TRUE);
			}
		}
	} else {
		*result = bhei->pe_duration;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

static int
wlc_he_doiovar(void *ctx, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	int err = BCME_OK;

	BCM_REFERENCE(vsize);

	switch (actionid) {
	case IOV_GVAL(IOV_HE):
		err = wlc_iocv_iov_cmd_proc(wlc, ctx, he_cmds, ARRAYSIZE(he_cmds),
			FALSE, params, plen, arg, alen, wlcif);
		break;
	case IOV_SVAL(IOV_HE):
		err = wlc_iocv_iov_cmd_proc(wlc, ctx, he_cmds, ARRAYSIZE(he_cmds),
			TRUE, params, plen, arg, alen, wlcif);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* ======== wlc module hooks ========= */

/* wlc up/init callback */
static int
wlc_he_wlc_up(void *ctx)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	phy_info_t *pi = WLC_PI(wlc);
	wlc_bsscolor_t bsscolor;

	/* Tell the phy to not use any color filtering for STA idx */
	bsscolor.disable = TRUE;
	bsscolor.color = 0;
	bsscolor.index = HE_BSSCOLOR_STAIDX;
	bsscolor.staid[STAID0] = HE_STAID_BSS_BCAST;
	bsscolor.staid[STAID1] = HE_STAID_MBSS_BCAST;
	bsscolor.staid[STAID2] = HE_STAID_BSS_BCAST;
	bsscolor.staid[STAID3] = 0;
	phy_hecap_write_bsscolor(pi, &bsscolor);

	hei->sta_insert_dummy_htc = (HE_ULMU_STA_ENAB(wlc->pub) &&
		D11REV_LT(wlc->pub->corerev, 130));

	return BCME_OK;
}

static int
wlc_he_wlc_down(void *ctx)
{
	wlc_he_info_t *hei = ctx;
	wlc_he_color_collision_t *cc = &hei->cc;

	/* If there were any color updates configured then apply them now */
	if (hei->update_bsscolor_counter) {
		hei->update_bsscolor_counter = 0;
		hei->bsscolor = hei->new_bsscolor;

		cc->remote_collision_detect_cnt = 0;
		cc->local_collision_detect_cnt = 0;
		cc->detect_report_cnt = 0;
		cc->detect_local_cnt = 0;
	}

	return BCME_OK;
}

#if defined(BCMDBG)
/* debug dump */
static int
wlc_he_dump(void *ctx, struct bcmstrbuf *b)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;

	bcm_bprintf(b, "HE Enab: %d Features: 0x%x\n", HE_ENAB(wlc->pub), wlc->pub->he_features);

	return BCME_OK;
}
#endif

/* ======== bsscfg module hooks ======== */

/* bsscfg cubby */
static int
wlc_he_bss_init(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc = hei->wlc;
	bss_he_info_t **pbhei = BSS_HE_INFO_LOC(hei, cfg);
	bss_he_info_t *bhei = BSS_HE_INFO(hei, cfg);

	if ((bhei = MALLOCZ(wlc->osh, sizeof(*bhei))) == NULL) {
		WL_ERROR(("wl%d: %s: mem alloc failed, allocated %d bytes\n", wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->osh)));
		return BCME_NOMEM;
	}

	*pbhei = bhei;

	bhei->pe_duration = HE_DEFAULT_PE_DURATION;
	bhei->rts_thresh = BSSCFG_AP(cfg) ? 0 /* always enabled  for AP */ :
		BCM_UINT16_MAX;	/* (-1) always disabled by default for STA */
	bhei->edca_update_count = HE_MUEDCA_INVALID_UPDATE_COUNT;
	memcpy(&bhei->sta_mu_edca, &default_mu_edca_sta, sizeof(bhei->sta_mu_edca));
	bhei->bsscolor = hei->bsscolor;
	bhei->bsscolor_disable = hei->bsscolor_disable;

	return BCME_OK;
} /* wlc_he_bss_init */

static void
wlc_he_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc = hei->wlc;
	bss_he_info_t **pbhei = BSS_HE_INFO_LOC(hei, cfg);
	bss_he_info_t *bhei = BSS_HE_INFO(hei, cfg);

	if (bhei == NULL) {
		return;
	}

	MFREE(wlc->osh, bhei, sizeof(*bhei));
	*pbhei = NULL;
}

#if defined(BCMDBG)
static void
wlc_he_bss_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	bss_he_info_t *bhei = BSS_HE_INFO(hei, cfg);

	bcm_bprintf(b, "Bsscolor %d (%s)\n", bhei->bsscolor, bhei->bsscolor_disable ? "disabled" :
		"enabled");
}
#endif

/**
 * bsscfg state change callback, will be invoked when a bsscfg changes its state (enable/disable/
 * up/down).
 */
static void
wlc_he_bsscfg_state_upd(void *ctx, bsscfg_state_upd_data_t *evt)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc =  hei->wlc;
	wlc_bsscfg_t *cfg = evt->cfg;
	bss_he_info_t *bhei;

	if (cfg->up) {
		wlc_bss_info_t *bi = cfg->current_bss;

		/* TWT requester support in extended capabilities. */
		if (wlc_twt_req_cap(wlc, cfg)) {
			wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_TWT_REQUESTER, TRUE);
		} else {
			wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_TWT_REQUESTER, FALSE);
		}

		/* TWT responder support in extended capabilities. */
		if (wlc_twt_resp_cap(wlc, cfg) && BSS_HE_ENAB_BAND(wlc, wlc->band->bandtype, cfg)) {
			wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_TWT_RESPONDER, TRUE);
		} else {
			wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_TWT_RESPONDER, FALSE);
		}

		/* Initialize band-specific default HE rateset */
		wlc_he_default_ratesets(hei, cfg);

		if (BSSCFG_AP(cfg) || BSSCFG_IBSS(cfg)) {
			bi->flags3 &= ~(WLC_BSS3_HE);

			if (BSS_HE_ENAB_BAND(wlc, wlc->band->bandtype, cfg)) {
				bi->flags3 |= WLC_BSS3_HE;
				/* BSS color collision report requires setting EVENT capability. */
				wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_EVENT, TRUE);
			}
		}

		/* calculate PPET based on nss */
		wlc_he_fill_ppe_thresholds(hei);

		bhei = BSS_HE_INFO(hei, cfg);
		bhei->bsscolor = hei->bsscolor;
		if (BSSCFG_AP(cfg)) {
			wlc_he_init_rateset(hei, &bi->rateset, wlc->band->bandunit, cfg);
			wlc_he_cp_he_rateset_to_bi_sup(bi,
				&wlc->bandstate[wlc->band->bandunit]->hw_rateset);
			wlc_he_init_bi_rateset_to_none(bi);
			bhei->bsscolor_disable = hei->bsscolor_disable;
		} else {
			/* Make sure we start with bss coloring disabled, till we actually
			 * associate. Needed for 6G, where all frames are 11ax
			 */
			bhei->bsscolor_disable = TRUE;
		}
		wlc_he_configure_bsscolor_phy(hei, cfg);
	}
}

#ifdef STA
/* PCB function for the HTC containing frame */
static void
wlc_he_send_htc_indication_complete(wlc_info_t *wlc, uint txstatus, void *arg)
{
	scb_t *scb;
	wlc_bsscfg_t *bsscfg;
	scb_he_t *he_scb;

	bsscfg = (wlc_bsscfg_t*) arg;
	ASSERT(bsscfg != NULL);

	if (!(scb = wlc_scbfind(wlc, bsscfg, &bsscfg->BSSID))) {
		return;
	}

	if (!(he_scb = SCB_HE(wlc->hei, scb))) {
		return;
	}

	he_scb->htc_retry_required = FALSE;
	if (txstatus & TX_STATUS_ACK_RCV) {
		/* acked */
		he_scb->send_htc_retries = 0;
	} else if (he_scb->send_htc_retries++ < HE_SEND_HTC_RETRY_LIMIT) {
		/* Need to retry HTC code, but if got here because of a FIFO flush (pktdrop), then
		 * sending it immediately won't help as it will get dropped immediately again
		 * Therefore postpone to next WD
		 */
		he_scb->htc_retry_required = TRUE;
	} else {
		/* reach retry limit */
		WL_ERROR(("wl%d: %s: Failed to send OMI indication. Reached retry limit %d\n",
			wlc->pub->unit, __FUNCTION__, he_scb->send_htc_retries));
		he_scb->send_htc_retries = 0;
	}
}

static int
wlc_he_send_htc_indication_cb(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, void *data)
{
	wlc_pcb_fn_register(wlc->pcb, wlc_he_send_htc_indication_complete, cfg, pkt);
	return BCME_OK;
}

/** send a HTC indication (based on link's omi_lm configuration) and retry if needed */
static void
wlc_he_send_htc_indication(wlc_info_t *wlc, wlc_bsscfg_t *cfg, scb_t *scb)
{
	wlc_he_info_t *hei = wlc->hei;
	scb_he_t *he_scb;
	uint32 htc;

	ASSERT(scb);
	ASSERT(cfg);
	if (SCB_HE_CAP(scb) && BSSCFG_STA(cfg)) {
		he_scb = SCB_HE(hei, scb);
		htc = HE_OMI_ENCODE(HTC_OM_CONTROL_TX_NSTS(he_scb->omi_lm),
			HTC_OM_CONTROL_RX_NSS(he_scb->omi_lm),
			HTC_OM_CONTROL_CHANNEL_WIDTH(he_scb->omi_lm),
			HTC_OM_CONTROL_ER_SU_DISABLE(he_scb->omi_lm), 0,
			HTC_OM_CONTROL_UL_MU_DISABLE(he_scb->omi_lm), 0);
		htc = (htc << HTC_CONTROL_ID_SZ) | HTC_CONTROL_ID_OM;
		htc = (htc << HTC_VARIANT_SZ) | HTC_IDENTITY_HE;

		wlc_he_htc_send_code(wlc, scb, htc, NULL);
		wlc_sendnulldata(wlc, cfg, &cfg->BSSID, 0, 0,
			PRIO_8021D_BE, wlc_he_send_htc_indication_cb, NULL);
	}
}

/** STA function to send ULMU enable/disable OMI indication to AP */
void
wlc_he_omi_ulmu_disable(wlc_he_info_t *hei, wlc_bsscfg_t *cfg, bool ulmu_disable)
{
	wlc_info_t *wlc = hei->wlc;
	scb_t *scb;
	scb_he_t *he_scb;

	ASSERT(BSSCFG_STA(cfg));

	if (!HE_ULMU_STA_ENAB(wlc->pub)) {
		/* don't allow ulmu */
		ulmu_disable = TRUE;
	}

	if (!(scb = wlc_scbfind(wlc, cfg, &cfg->BSSID))) {
		return;
	}
	he_scb = SCB_HE(hei, scb);
	if (SCB_HE_CAP(scb) && (he_scb != NULL) && (he_scb->flags & SCB_HE_OMI) &&
		(HTC_OM_CONTROL_UL_MU_DISABLE(he_scb->omi_lm) != ulmu_disable)) {
		he_scb->omi_lm &= ~(1 << HTC_OM_CONTROL_UL_MU_DISABLE_OFFSET);
		he_scb->omi_lm |= (ulmu_disable << HTC_OM_CONTROL_UL_MU_DISABLE_OFFSET);

		he_scb->send_htc_retries = 0;
		he_scb->htc_retry_required = FALSE;
		wlc_he_send_htc_indication(wlc, cfg, scb);
	}
}
#endif /* STA */

/*
 * send OMI indication to peers because of change RX configuration
 * new_rxnss has to be pre-decremented by caller.
 */
void
wlc_he_upd_omi_rxnss(wlc_he_info_t *hei, uint8 new_rxnss)
{
	wlc_info_t *wlc = hei->wlc;
	int idx;
	wlc_bsscfg_t *cfg;
	struct scb *scb;
	scb_he_t *he_scb;

	ASSERT(new_rxnss < WLC_BITSCNT(wlc->stf->hw_rxchain));

	/* inform all our connected-to APs */
#ifdef STA
	FOREACH_AS_STA(wlc, idx, cfg) {
		if ((scb = wlc_scbfind(wlc, cfg, &cfg->BSSID)) != NULL) {
			he_scb = SCB_HE(hei, scb);
			if ((he_scb == NULL) || !SCB_HE_CAP(scb) || !(he_scb->flags & SCB_HE_OMI))
				continue;
			/* If we are not changing the number of chains then no need to send OMI */
			if (HTC_OM_CONTROL_RX_NSS(he_scb->omi_lm) == new_rxnss) {
				continue;
			}
			he_scb->omi_lm &= ~HTC_OM_CONTROL_RX_NSS_MASK;
			he_scb->omi_lm |= (new_rxnss << HTC_OM_CONTROL_RX_NSS_OFFSET);
			he_scb->send_htc_retries = 0;
			he_scb->htc_retry_required = FALSE;
			wlc_he_send_htc_indication(wlc, cfg, scb);
		}
	}
#endif /* STA */
	/* inform all our associated STAs */
	FOREACH_UP_AP(wlc, idx, cfg) {
		scb_iter_t scbiter;
		uint omi_bw = (CHSPEC_BW(cfg->current_bss->chanspec) - WL_CHANSPEC_BW_20) >>
			WL_CHANSPEC_BW_SHIFT; /* convert to DOT11_OPER_MODE_xxMHz range */
		uint32 htc = HE_OMI_ENCODE(new_rxnss, (wlc->stf->txstreams - 1), omi_bw,
			!HE_ERSU_RX_ENAB(wlc->pub), 0, 0, 0);
		htc = (htc << HTC_CONTROL_ID_SZ) | HTC_CONTROL_ID_OM;
		htc = (htc << HTC_VARIANT_SZ) | HTC_IDENTITY_HE;
		FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
			he_scb = SCB_HE(hei, scb);
			if ((he_scb == NULL) || !SCB_HE_CAP(scb) || !(he_scb->flags & SCB_HE_OMI))
				continue;
			/* best effort attempt, no retries */
			wlc_he_htc_send_code(wlc, scb, htc, NULL);
			/* Send NULL data frame to make sure HTC gets sent... */
			wlc_sendnulldata(wlc, scb->bsscfg, &scb->ea, 0, 0, PRIO_8021D_BE,
				NULL, NULL);
		}
	}
}

/** send OMI indication to peers because of change TX configuration */
void
wlc_he_upd_omi_txnsts(wlc_he_info_t *hei, uint8 new_txnsts)
{
	wlc_info_t *wlc = hei->wlc;
	int idx;
	wlc_bsscfg_t *cfg;

	ASSERT(new_txnsts < WLC_BITSCNT(wlc->stf->hw_txchain));
	/* inform all our connected-to APs (for their triggers) */
	FOREACH_AS_STA(wlc, idx, cfg) {
#ifdef STA
		struct scb *scb;
		scb_he_t *he_scb;

		if ((scb = wlc_scbfind(wlc, cfg, &cfg->BSSID)) != NULL) {
			he_scb = SCB_HE(hei, scb);
			if ((he_scb == NULL) || !SCB_HE_CAP(scb) || !(he_scb->flags & SCB_HE_OMI))
				continue;
			/* If we are not changing the number of chains then no need to send OMI */
			if (HTC_OM_CONTROL_TX_NSTS(he_scb->omi_lm) == new_txnsts) {
				continue;
			}
			he_scb->omi_lm &= ~HTC_OM_CONTROL_TX_NSTS_MASK;
			he_scb->omi_lm |= (new_txnsts << HTC_OM_CONTROL_TX_NSTS_OFFSET);
			he_scb->send_htc_retries = 0;
			he_scb->htc_retry_required = FALSE;
			wlc_he_send_htc_indication(wlc, cfg, scb);
		}
#endif /* STA */
	}
	/* for TX no need to inform associated STAs */
}

#if defined(STA)
/**
 * disassoc state change callback, will be invoked when a link association state changes
 */
static void
wlc_he_disassoc_state_change_cb(void *ctx, bss_disassoc_notif_data_t *notif_data)
{
	uint16 val;
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg = notif_data->cfg;

	if (!BSSCFG_STA(cfg) || !HE_ULMU_STA_ENAB(wlc->pub)) {
		return;
	}
	/*
	 * disable beacon promiscous mode at disassoc
	 * Also clear MHF6_AVOID_HETB_RU26
	 */
	if (notif_data->state == DAN_ST_DISASSOC_CMPLT) {
		if (wlc->hei->nbwru.obss_bcn_monitor_active) {
			wlc->hei->nbwru.obss_bcn_monitor_active = FALSE;
			wlc_mac_bcn_promisc_update(wlc->hei->wlc,
				BCNMISC_AXSTA_OBSS_NBWRU, FALSE);
#if defined(BCMDBG)
			WL_HE_ERR(("wl%d: %s: Clearing BCNMISC\n", wlc->pub->unit, __FUNCTION__));
#endif
		}
		if (!wlc->hei->nbwru.discard_nbwru_tf)
			return;
		wlc->hei->nbwru.discard_nbwru_tf = FALSE;
		if (!wlc->hw->clk)
			return;
		val = wlc_read_shm(wlc, M_HOST_FLAGS6(wlc->hw));
		wlc_write_shm(wlc, M_HOST_FLAGS6(wlc->hw),
				val & ~MHF6_AVOID_HETB_RU26);
#if defined(BCMDBG)
		WL_HE_INFO(("wl%d: %s: Clearing MHF6_AVOID_HETB_RU26\n",
			wlc->pub->unit, __FUNCTION__));
#endif
	}
}

static void
wlc_he_chansw_upd_cb(void *arg, wlc_chansw_notif_data_t *data)
{
	wlc_info_t *wlc = (wlc_info_t *)arg;
	chanspec_t new_chspec = data->new_chanspec;
	chanspec_t old_chspec = data->old_chanspec;
	wlc_bsscfg_t *icfg;
	int idx;
	scb_t* scb;
	scb_he_t *he_scb;

	if (SCAN_IN_PROGRESS(wlc->scan)) {
		return;
	}

	WL_HE_INFO(("wl%d: %s: old chanspec 0x%x new chanspec 0x%x\n", wlc->pub->unit,
		__FUNCTION__, old_chspec, new_chspec));
	if ((CHSPEC_BW(new_chspec) != CHSPEC_BW(old_chspec)) ||
		(CHSPEC_BAND(new_chspec) != CHSPEC_BAND(old_chspec))) {
		/* upon BAND or BW switch (for an 'up' STA) inform AP of updated OMI */
		FOREACH_AS_STA(wlc, idx, icfg) {
			scb = wlc_scbfind(wlc, icfg, &icfg->BSSID);
			if (scb == NULL)
				continue;
			he_scb = SCB_HE(wlc->hei, scb);
			if ((he_scb == NULL) || !SCB_HE_CAP(scb) || !(he_scb->flags & SCB_HE_OMI))
				continue;
			wlc_he_send_htc_indication(wlc, icfg, scb);
		}
	}
}
#endif /* STA */

/**
 * assoc state change callback, will be invoked when a link association state changes
 */
static void
wlc_he_assoc_state_change_cb(void *ctx, bss_assoc_state_data_t *notif_data)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg = notif_data->cfg;
	bss_he_info_t *bhei = BSS_HE_INFO(hei, cfg);

	if (!HE_ENAB(wlc->pub)) {
		return;
	}
	if (notif_data->type == AS_ASSOCIATION ||
		notif_data->type == AS_ROAM) {

		if (notif_data->state == AS_JOIN_ADOPT) {
			/* Make sure phy gets updated with staid info, color config function */
			wlc_he_configure_bsscolor_phy(hei, cfg);
#ifdef STA
			if (!HE_ULMU_STA_ENAB(wlc->pub)) {
				wlc_he_omi_ulmu_disable(hei, cfg, TRUE);
			} else if (!SCAN_IN_PROGRESS(wlc->scan) &&
					wlc_radar_chanspec(wlc->cmi, wlc->home_chanspec)) {
					wlc_mac_bcn_promisc_update(hei->wlc,
						BCNMISC_AXSTA_OBSS_NBWRU, TRUE);
					hei->nbwru.obss_bcn_monitor_active = TRUE;
#if defined(BCMDBG)
		WL_HE_INFO(("wl%d: %s: Setting BCNMISC\n", wlc->pub->unit, __FUNCTION__));
#endif
			}
#endif /* STA */
		}
	}
	/* Lost association as STA, on IDLE event re-initialize MU-EDCA and coloring. */
	if (BSSCFG_STA(cfg) && (notif_data->state == AS_IDLE)) {
		/* Reset MU-EDCA */
		memcpy(&bhei->sta_mu_edca, &default_mu_edca_sta, sizeof(bhei->sta_mu_edca));
		/* Make sure the first MU EDCA parameter set is consumed */
		bhei->edca_update_count = HE_MUEDCA_INVALID_UPDATE_COUNT;
		/* Make sure we start with bss coloring disabled, till we actually
		 * associate. Needed for 6G, where all frames are 11ax
		 */
		bhei->bsscolor_disable = TRUE;
		wlc_he_configure_bsscolor_phy(hei, cfg);
	}
}

/* ======== scb cubby ======== */

static int
wlc_he_scb_init(void *ctx, scb_t *scb)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	scb_he_t **psh = SCB_HE_CUBBY(hei, scb);

	ASSERT(*psh == NULL);

	*psh = wlc_scb_sec_cubby_alloc(wlc, scb, wlc_he_scb_secsz(ctx, scb));

	return BCME_OK;
}

static void
wlc_he_scb_deinit(void *ctx, scb_t *scb)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	scb_he_t **psh = SCB_HE_CUBBY(hei, scb);
	scb_he_t *sh = SCB_HE(hei, scb);

	/* Memory not allocated for SCB return */
	if (!sh) {
		return;
	}

	wlc_scb_sec_cubby_free(wlc, scb, sh);
	*psh = NULL;
}

static uint
wlc_he_scb_secsz(void *ctx, scb_t *scb)
{
	uint size = 0;
	if (scb && !SCB_INTERNAL(scb)) {
		size = sizeof(scb_he_t);
	}
	return size;
}

#if defined(BCMDBG)
static const bcm_bit_desc_t scb_mac_cap[] =
{
	{HE_MAC_HTC_HE_SUPPORT_IDX, "HTC"},
	{HE_MAC_TWT_REQ_SUPPORT_IDX, "TWTReq"},
	{HE_MAC_TWT_RESP_SUPPORT_IDX, "TWTResp"},
	{HE_MAC_UL_MU_RESP_SCHED_IDX, "UL-MU-Resp"},
	{HE_MAC_A_BSR_IDX, "A-BSR"},
	{HE_MAC_BCAST_TWT_SUPPORT_IDX, "BcstTWT"},
	{HE_MAC_OM_CONTROL_SUPPORT_IDX, "OMI"},
	{HE_MAC_UL_2X996_TONE_RU_IDX, "UL-2x996"}
};

static const bcm_bit_desc_t scb_phy_cap[] =
{
	{HE_PHY_DEVICE_CLASS_IDX, "Class-A"},
	{HE_PHY_LDPC_PYLD_IDX, "LDPC"},
	{HE_PHY_SU_BEAMFORMER_IDX, "SU-BFR"},
	{HE_PHY_SU_BEAMFORMEE_IDX, "SU-BFE"},
	{HE_PHY_MU_BEAMFORMER_IDX, "MU-BFR"},
	{HE_PHY_TX_1024_QAM_LESS_242_TONE_RU_IDX, "Tx1024Q"},
	{HE_PHY_RX_1024_QAM_LESS_242_TONE_RU_IDX, "Rx1024Q"}
};

static void
wlc_he_scb_dump(void *ctx, scb_t *scb, struct bcmstrbuf *b)
{
	char heinfostr[HE_INFO_STR_SZ];
	wlc_he_info_t *hei = ctx;
	scb_he_t *sh = SCB_HE(hei, scb);
	wlc_rateset_t *rateset;

	if (sh == NULL) {
		return;
	}

	if (!SCB_HE_CAP(scb)) {
		bcm_bprintf(b, "     NOT HE capable");
		return;
	}

	bcm_format_octets(scb_mac_cap, ARRAYSIZE(scb_mac_cap), (const uint8 *)&sh->mac_cap,
		sizeof(sh->mac_cap), heinfostr, sizeof(heinfostr));
	bcm_bprintf(b, "     he_mac_cap: %s ", heinfostr);
	bcm_bprhex(b, NULL, TRUE, (const uint8 *)&sh->mac_cap, sizeof(sh->mac_cap));
	bcm_format_octets(scb_phy_cap, ARRAYSIZE(scb_phy_cap), (const uint8 *)&sh->phy_cap,
		sizeof(sh->phy_cap), heinfostr, sizeof(heinfostr));
	bcm_bprintf(b, "     he_phy_cap: %s ", heinfostr);
	bcm_bprhex(b, NULL, TRUE, (const uint8 *)&sh->phy_cap, sizeof(sh->phy_cap));
	rateset = &scb->rateset;
	bcm_bprintf(b, "     Tx 80=0x%04x, Rx 80=0x%04x, Tx 160=0x%04x, Rx 160=0x%04x, Flag=0x%x\n",
		rateset->he_bw80_tx_mcs_nss, rateset->he_bw80_rx_mcs_nss,
		rateset->he_bw160_tx_mcs_nss, rateset->he_bw160_rx_mcs_nss,
		sh->flags);
}
#endif

/* ======== IE mgmt ======== */

/* figure out the length of the value in the TLV i.e. the length without the TLV header. */
static uint8
_wlc_he_calc_cap_ie_len(wlc_he_info_t *hei, wlc_bsscfg_t *cfg)
{
	wlc_he_ppet_t *ppe_ths;
	/* The length of the HE cap IE is dependent on the bandwidth. By the time the defaults
	 * get determined it is not possible to know for which band. This is how it works:
	 * The minimum support is bw80, on 2.4G that is also the max. On 5G the max is
	 * determined by the capabilities of device which is precalculated.
	 */
	ppe_ths = BAND_2G(hei->wlc->band->bandtype) ? &hei->ppe_ths_2g : &hei->ppe_ths_5g6g;
	if (wlc_he_bw80_limited(hei->wlc, cfg)) {
		return (sizeof(he_cap_ie_t) - TLV_HDR_LEN + HE_MIN_HE_RATESETSZ +
			ppe_ths->ppe_ths_len);
	} else {
		return (sizeof(he_cap_ie_t) - TLV_HDR_LEN + hei->he_ratesetsz +
			ppe_ths->ppe_ths_len);
	}
}

static uint
wlc_he_max_mcs_idx(uint16 mcs_nss_set)
{
	uint8 nss;
	uint8 mcs_code;
	uint max_mcs_idx = 0;

	for (nss = 1; nss <= HE_CAP_MCS_MAP_NSS_MAX; nss++) {
		mcs_code = HE_CAP_MAX_MCS_NSS_GET_MCS(nss, mcs_nss_set);

		if (mcs_code == HE_CAP_MAX_MCS_NONE) {
			/* continue to next stream */
			continue;
		}
		if (HE_MAX_MCS_TO_INDEX(mcs_code) > max_mcs_idx) {
			max_mcs_idx = HE_MAX_MCS_TO_INDEX(mcs_code);
		}
	}
	return max_mcs_idx;
}

/** HE capabilities element */
static uint
wlc_he_calc_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;

	if (!data->cbparm->he)
		return 0;

	return TLV_HDR_LEN + _wlc_he_calc_cap_ie_len(hei, data->cfg);
}

static void
wlc_he_fill_mac_cap(wlc_info_t *wlc, wlc_bsscfg_t *cfg, he_mac_cap_t *mac_cap)
{
	/* TWT requester support ? */
	if (wlc_twt_req_cap(wlc, cfg)) {
		setbit((uint8 *)mac_cap, HE_MAC_TWT_REQ_SUPPORT_IDX);
	}

	/* TWT responder support ? */
	if (wlc_twt_resp_cap(wlc, cfg)) {
		setbit((uint8 *)mac_cap, HE_MAC_TWT_RESP_SUPPORT_IDX);
	}

	/* Broadcast TWT support ? */
	if (wlc_twt_bcast_cap(wlc, cfg)) {
		setbit((uint8 *)mac_cap, HE_MAC_BCAST_TWT_SUPPORT_IDX);
	}
}

static uint32
wlc_he_fill_phy_cap(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool bw80_limited, he_phy_cap_t *phy_cap)
{
	uint32 channel_width_set;
	uint8 bfe;
	//wlc_disable_bfe_for_smth(..., TRUE) means checking for HE
	bfe = wlc_disable_bfe_for_smth(wlc->txbf, cfg, TRUE) ? 0
		: wlc_txbf_get_bfe_cap(wlc->txbf, cfg);
	setbits((uint8 *)phy_cap, sizeof(*phy_cap),
		HE_PHY_SOUND_DIM_BELOW80MHZ_IDX, HE_PHY_SOUND_DIM_BELOW80MHZ_FSZ,
		wlc->stf->op_txstreams - 1);

	/* Init bfe and bfr cap */
	setbits((uint8 *)phy_cap, sizeof(*phy_cap),
		HE_PHY_SU_BEAMFORMEE_IDX, HE_PHY_SU_BEAMFORMEE_FSZ, 0);
	setbits((uint8 *)phy_cap, sizeof(*phy_cap),
		HE_PHY_SU_BEAMFORMER_IDX, HE_PHY_SU_BEAMFORMER_FSZ, 0);
	setbits((uint8 *)phy_cap, sizeof(*phy_cap),
		HE_PHY_MU_BEAMFORMER_IDX, HE_PHY_MU_BEAMFORMER_FSZ, 0);
#ifdef WL_BEAMFORMING
	if (TXBF_ENAB(wlc->pub)) {
		if ((bfe & TXBF_HE_SU_BFE_CAP) > 0) {
			setbit((uint8 *)phy_cap, HE_PHY_SU_BEAMFORMEE_IDX);
		} else {
			clrbit((uint8 *)phy_cap, HE_PHY_SU_BEAMFORMEE_IDX);
			clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
				HE_PHY_BEAMFORMEE_STS_BELOW80MHZ_IDX,
				HE_PHY_BEAMFORMEE_STS_BELOW80MHZ_FSZ);
			clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
				HE_PHY_BEAMFORMEE_STS_ABOVE80MHZ_IDX,
				HE_PHY_BEAMFORMEE_STS_ABOVE80MHZ_FSZ);
			clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
				HE_PHY_MAX_NC_IDX, HE_PHY_MAX_NC_FSZ);
		}
		if (wlc_txbf_get_bfr_cap(wlc->txbf) & TXBF_HE_SU_BFR_CAP) {
			setbit((uint8 *)phy_cap, HE_PHY_SU_BEAMFORMER_IDX);
		} else {
			clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
				HE_PHY_SOUND_DIM_BELOW80MHZ_IDX,
				HE_PHY_SOUND_DIM_BELOW80MHZ_FSZ);
			clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
				HE_PHY_SOUND_DIM_ABOVE80MHZ_IDX,
				HE_PHY_SOUND_DIM_ABOVE80MHZ_FSZ);
		}
		if (wlc_txbf_get_bfr_cap(wlc->txbf) & TXBF_HE_MU_BFR_CAP) {
			if (!BSSCFG_STA(cfg)) {
				setbit((uint8 *)phy_cap, HE_PHY_MU_BEAMFORMER_IDX);
			}
			setbit((uint8 *)phy_cap, HE_PHY_TRG_SU_BFM_FEEDBACK_IDX);
		}
		/* for AP, indicates rx of partial/full bw CQI feedback, triggered mode */
		if (BSSCFG_AP(cfg) && wlc_txbf_get_bfr_cap(wlc->txbf) & TXBF_HE_CQI_BFR_CAP) {
			setbit((uint8 *)phy_cap, HE_PHY_TRG_CQI_FEEDBACK_IDX);
		}
		/* for STA, indicates tx of partial/full bw CQI feedback, triggered mode */
		if (BSSCFG_STA(cfg) && wlc_txbf_get_bfe_cap(wlc->txbf, cfg) & TXBF_HE_CQI_BFE_CAP) {
			setbit((uint8 *)phy_cap, HE_PHY_TRG_CQI_FEEDBACK_IDX);
		}
		/* since high resolution (cb=1) is prefered (and mandatory), mark that as BFE we
		 * don't support MU codebook with low resolution (cb=0)
		 */
		clrbit((uint8 *)phy_cap, HE_PHY_MU_CODEBOOK_SUPPORT_IDX);
	} else
#endif /* WL_BEAMFORMING */
	{
		clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
			HE_PHY_BEAMFORMEE_STS_BELOW80MHZ_IDX, HE_PHY_BEAMFORMEE_STS_BELOW80MHZ_FSZ);
		clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
			HE_PHY_BEAMFORMEE_STS_ABOVE80MHZ_IDX, HE_PHY_BEAMFORMEE_STS_ABOVE80MHZ_FSZ);
		clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
			HE_PHY_SOUND_DIM_BELOW80MHZ_IDX, HE_PHY_SOUND_DIM_BELOW80MHZ_FSZ);
		clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
			HE_PHY_SOUND_DIM_ABOVE80MHZ_IDX, HE_PHY_SOUND_DIM_ABOVE80MHZ_FSZ);
		clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
			HE_PHY_MAX_NC_IDX, HE_PHY_MAX_NC_FSZ);
	}
	if (bw80_limited) {
		clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
			HE_PHY_BEAMFORMEE_STS_ABOVE80MHZ_IDX, HE_PHY_BEAMFORMEE_STS_ABOVE80MHZ_FSZ);
		clrbits((uint8 *)phy_cap, sizeof(*phy_cap),
			HE_PHY_SOUND_DIM_ABOVE80MHZ_IDX, HE_PHY_SOUND_DIM_ABOVE80MHZ_FSZ);
	}

	/* Phy capabilities b1-b7 Channel Width Set is dependent on band */
	channel_width_set = getbits((uint8 *)phy_cap, sizeof(*phy_cap),
		HE_PHY_CH_WIDTH_SET_IDX, HE_PHY_CH_WIDTH_SET_FSZ);
	if (BAND_2G(wlc->band->bandtype)) {
		channel_width_set &= ~(HE_PHY_CH_WIDTH_5G_80 | HE_PHY_CH_WIDTH_5G_160 |
			HE_PHY_CH_WIDTH_5G_80P80 | HE_PHY_CH_WIDTH_5G_242TONE);
		if (!(WL_BW_CAP_40MHZ(wlc->band->bw_cap))) {
			channel_width_set &= ~(HE_PHY_CH_WIDTH_2G_40);
		}
	} else {
		channel_width_set &= ~(HE_PHY_CH_WIDTH_2G_40 | HE_PHY_CH_WIDTH_2G_242TONE);
		if (bw80_limited) {
			channel_width_set &= ~(HE_PHY_CH_WIDTH_5G_160 | HE_PHY_CH_WIDTH_5G_80P80);
			/* If limited to 20Mhz only then report this (if we are non-AP). */
			if (BSSCFG_STA(cfg) && (WL_BW_CAP_20MHZ(wlc->band->bw_cap)) &&
				!(WL_BW_CAP_40MHZ(wlc->band->bw_cap))) {
				channel_width_set &= ~(HE_PHY_CH_WIDTH_5G_80);
			}
		}
	}
	setbits((uint8 *)phy_cap, sizeof(*phy_cap), HE_PHY_CH_WIDTH_SET_IDX,
		HE_PHY_CH_WIDTH_SET_FSZ, channel_width_set);

	return channel_width_set;
}

static int
wlc_he_write_cap_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc = hei->wlc;
	he_cap_ie_t *cap = (he_cap_ie_t *)data->buf;
	wlc_bsscfg_t *cfg = data->cfg;
	bss_he_info_t *bhei = BSS_HE_INFO(hei, cfg);
	wlc_rateset_t *rateset;
	uint32 channel_width_set;
	uint offset;
	uint8 *bw_info;
	uint8 *ptr;
	uint mcs_tmp;
	uint max_mcs_rx;
	uint max_mcs_tx;
	wlc_he_ppet_t *ppe_ths;

	/* sanity check */
	STATIC_ASSERT(sizeof(hei->def_mac_cap) == SIZE_OF(he_cap_ie_t, mac_cap));
	STATIC_ASSERT(sizeof(hei->def_phy_cap) == SIZE_OF(he_cap_ie_t, phy_cap));

	/* IE header */
	cap->id = DOT11_MNG_ID_EXT_ID;
	cap->id_ext = EXT_MNG_HE_CAP_ID;
	cap->len = _wlc_he_calc_cap_ie_len(hei, cfg);
	offset = sizeof(cap->id) + sizeof(cap->id_ext) + sizeof(cap->len);

	/* Initializing default HE MAC & PHY Capabilities
	 * Set other bits to add new capabilities
	 */
	if (BSSCFG_AP(cfg)) {
		wlc_he_ap_maccap_init(hei, &cap->mac_cap);
		wlc_he_ap_phycap_init(hei, &cap->phy_cap);
		bhei->omc_cap = TRUE;
	} else {
		wlc_he_sta_maccap_init(hei, &cap->mac_cap);
		wlc_he_sta_phycap_init(hei, &cap->phy_cap);
		bhei->omc_cap = FALSE;
	}

	wlc_he_fill_mac_cap(wlc, cfg, &cap->mac_cap);

	channel_width_set = wlc_he_fill_phy_cap(wlc, cfg, wlc_he_bw80_limited(wlc, cfg),
		&cap->phy_cap);

	offset += (uint)(sizeof(he_mac_cap_t) + sizeof(he_phy_cap_t));

	/* HE TX-RX MCS-NSS support */
	rateset = &wlc->bandstate[wlc->band->bandunit]->hw_rateset;
	ptr = (uint8 *)cap + offset;
	bw_info = ptr;
	setbits(bw_info, HE_BW_RASETSZ, HE_BW80_RX_RATESET_OFFSET * NBBY,
		HE_BW_RX_RASETSZ * NBBY, rateset->he_bw80_rx_mcs_nss);
	setbits(bw_info, HE_BW_RASETSZ, HE_BW80_TX_RATESET_OFFSET * NBBY,
		HE_BW_TX_RASETSZ * NBBY, rateset->he_bw80_tx_mcs_nss);
	max_mcs_rx = wlc_he_max_mcs_idx(rateset->he_bw80_rx_mcs_nss);
	max_mcs_tx = wlc_he_max_mcs_idx(rateset->he_bw80_tx_mcs_nss);
	ptr += HE_BW_RASETSZ;
	if (channel_width_set & HE_PHY_CH_WIDTH_5G_160) {
		setbits(bw_info, 2 * HE_BW_RASETSZ, HE_BW160_RX_RATESET_OFFSET * NBBY,
			HE_BW_RX_RASETSZ * NBBY, rateset->he_bw160_rx_mcs_nss);
		setbits(bw_info, 2 * HE_BW_RASETSZ, HE_BW160_TX_RATESET_OFFSET * NBBY,
			HE_BW_TX_RASETSZ * NBBY, rateset->he_bw160_tx_mcs_nss);
		ptr += HE_BW_RASETSZ;
		mcs_tmp = wlc_he_max_mcs_idx(rateset->he_bw160_rx_mcs_nss);
		if (mcs_tmp > max_mcs_rx) {
			max_mcs_rx = mcs_tmp;
		}
		mcs_tmp = wlc_he_max_mcs_idx(rateset->he_bw160_tx_mcs_nss);
		if (mcs_tmp > max_mcs_tx) {
			max_mcs_tx = mcs_tmp;
		}
	}
	if (channel_width_set & HE_PHY_CH_WIDTH_5G_80P80) {
		setbits(bw_info, HE_BW_RASETSZ, HE_BW80P80_RX_RATESET_OFFSET * NBBY,
			HE_BW_RX_RASETSZ * NBBY, rateset->he_bw80p80_rx_mcs_nss);
		setbits(bw_info, HE_BW_RASETSZ, HE_BW80P80_TX_RATESET_OFFSET * NBBY,
			HE_BW_TX_RASETSZ * NBBY, rateset->he_bw80p80_tx_mcs_nss);
		ptr += HE_BW_RASETSZ;
		mcs_tmp = wlc_he_max_mcs_idx(rateset->he_bw80p80_rx_mcs_nss);
		if (mcs_tmp > max_mcs_rx) {
			max_mcs_rx = mcs_tmp;
		}
		mcs_tmp = wlc_he_max_mcs_idx(rateset->he_bw80p80_tx_mcs_nss);
		if (mcs_tmp > max_mcs_tx) {
			max_mcs_tx = mcs_tmp;
		}
	}

	if (max_mcs_rx < HE_LOWEST_QAM1024_MCS_IDX) {
		clrbits((uint8 *)&cap->phy_cap, sizeof(cap->phy_cap),
			HE_PHY_RX_1024_QAM_LESS_242_TONE_RU_IDX,
			HE_PHY_RX_1024_QAM_LESS_242_TONE_RU_FSZ);
	}
	if (max_mcs_tx < HE_LOWEST_QAM1024_MCS_IDX) {
		clrbits((uint8 *)&cap->phy_cap, sizeof(cap->phy_cap),
			HE_PHY_TX_1024_QAM_LESS_242_TONE_RU_IDX,
			HE_PHY_TX_1024_QAM_LESS_242_TONE_RU_FSZ);
	}

	/* Initialize default HE PPE Thresholds field */
	ppe_ths = BAND_2G(hei->wlc->band->bandtype) ? &hei->ppe_ths_2g : &hei->ppe_ths_5g6g;
	if (ppe_ths->ppe_ths_len > 0) {
		setbit((uint8*)&cap->phy_cap, HE_PHY_PPE_THRESH_PRESENT_IDX);
		/* Nominal packet padding is reserved if PPE Thresholds presents */
		setbits((uint8*)&cap->phy_cap, sizeof(cap->phy_cap),
			HE_PHY_NOMINAL_PACKET_PADDING_IDX, HE_PHY_NOMINAL_PACKET_PADDING_FSZ,
			HE_PHY_NOMINAL_PKT_PADDING_RESERVED);
		memcpy(ptr, ppe_ths->ppe_ths, ppe_ths->ppe_ths_len);
	}

#ifdef TESTBED_AP_11AX
	/* When AP is set for testbed mode then some features are to be disabled by default */
	if (hei->testbed_mode) {
		clrbit((uint8 *)&cap->mac_cap, HE_MAC_OM_UL_MU_DATA_DISABLE_IDX);

		clrbit((uint8 *)&cap->phy_cap, HE_PHY_SU_CODEBOOK_SUPPORT_IDX);
		clrbit((uint8 *)&cap->phy_cap, HE_PHY_MU_CODEBOOK_SUPPORT_IDX);
		clrbit((uint8 *)&cap->phy_cap, HE_PHY_SU_FEEDBACK_NG16_SUPPORT_IDX);
		clrbit((uint8 *)&cap->phy_cap, HE_PHY_UL_MU_PYLD_IDX);
		clrbit((uint8 *)&cap->phy_cap, HE_PHY_NDP_4x_LTF_3_2_GI_RX_IDX);

		clrbits((uint8 *)&cap->mac_cap, sizeof(cap->mac_cap), HE_MAC_MAX_AMPDU_LEN_EXP_IDX,
			HE_MAC_MAX_AMPDU_LEN_EXP_FSZ);
	}
#endif /* TESTBED_AP_11AX */

	return BCME_OK;
}

static int
wlc_he_parse_cap_ppe_thresh(wlc_he_info_t *hei, scb_he_t *sh, uint8 *cap_ptr, uint length)
{
	wlc_info_t *wlc = hei->wlc;
	uint nss, ru, num_ru, nss_idx;
	uint ru_r_idx, ru_w_idx;
	uint32 current_ppet16;
	uint32 current_ppet8;
	uint32 ppet16;
	uint32 ppet8;

	BCM_REFERENCE(wlc);

	/* cap_ptr points to PPE field, read nss(m1) and ru bitmap */
	nss = getbits(cap_ptr, 1, HE_NSSM1_IDX, HE_NSSM1_LEN) + 1;
	ru = getbits(cap_ptr, 1, HE_RU_INDEX_MASK_IDX, HE_RU_INDEX_MASK_LEN);
	num_ru = bcm_bitcount((uint8*) &ru, 1);
	if (HE_PPE_THRESH_LEN(nss, num_ru) > length) {
		WL_HE_INFO(("wl%d: %s: Invalid PPE threshold Cap IE len %d, %d\n",
			wlc->pub->unit, __FUNCTION__, HE_PPE_THRESH_LEN(nss, num_ru), length));
		return BCME_ERROR;
	} else if (nss > HE_CAP_MCS_MAP_NSS_MAX) {
		WL_HE_INFO(("wl%d: %s: Invalid PPE_NUM_NSS %d\n", wlc->pub->unit, __FUNCTION__,
			nss));
		return BCME_ERROR;
	} else if (num_ru > HE_MAX_RU_COUNT) {
		WL_HE_INFO(("wl%d: %s: Invalid PPE_NUM_RU %d\n", wlc->pub->unit, __FUNCTION__, ru));
		return BCME_ERROR;
	}

	/* JIRA SWWLAN-178834 has pdf attached explaining format to store info. We store info in
	 * the format such that it can be used by ucode and only requires a copy. JIRA SWWLAN-194366
	 * provides additional information on how to parse the PPET values.
	 */
	sh->min_ppe_nss = -1;
	for (nss_idx = 0; nss_idx < nss; nss_idx++) {
		for (ru_r_idx = 0, ru_w_idx = 0; ru_w_idx < HE_MAX_RU_COUNT; ru_w_idx++) {
			if ((ru & NBITVAL(ru_w_idx)) != 0) {
				/* determine if the supplied value by remote sta indicates
				 * higher ppet value. This is quite difficult due to way the
				 * ppet values are encoded, see specification and JIRA
				 */
				current_ppet16 = sh->ppe_nss[nss_idx] >>
					(ru_w_idx * HE_PPE_THRESH_NSS_RU_FSZ * 2);
				current_ppet16 &= HE_PPE_THRESH_NSS_RU_MASK;
				current_ppet8 = sh->ppe_nss[nss_idx] >>
					((ru_w_idx * 2) + 1) * HE_PPE_THRESH_NSS_RU_FSZ;
				current_ppet8 &= HE_PPE_THRESH_NSS_RU_MASK;
				/* combined read of PPET16 & PPET8 */
				ppet16 = getbits(cap_ptr, length,
					HE_PPET16_BIT_OFFSET(num_ru, nss_idx, ru_r_idx),
					2 * HE_PPE_THRESH_NSS_RU_FSZ);
				ru_r_idx++;
				ppet8 = ppet16 >> HE_PPE_THRESH_NSS_RU_FSZ;
				ppet16 &= HE_PPE_THRESH_NSS_RU_MASK;
				if (ppet16 < current_ppet16) {
					current_ppet16 = ppet16;
				}
				if (ppet8 < current_ppet8) {
					current_ppet8 = ppet8;
				}
				if ((current_ppet8 >= current_ppet16) &&
				    (current_ppet8 != HE_CONST_IDX_NONE)) {
					/* invalid combo created. Move ppet8 index below ppet16
					 * or if ppet16 = 0 then set ppet8 to none
					 */
					if (current_ppet16 == HE_CONST_IDX_BPSK) {
						current_ppet8 = HE_CONST_IDX_NONE;
					} else {
						current_ppet8 = current_ppet16 - 1;
					}

				}
				sh->ppe_nss[nss_idx] &= ~(((HE_PPE_THRESH_NSS_RU_MASK <<
					HE_PPE_THRESH_NSS_RU_FSZ) | HE_PPE_THRESH_NSS_RU_MASK) <<
					(ru_w_idx * HE_PPE_THRESH_NSS_RU_FSZ * 2));
				sh->ppe_nss[nss_idx] |= (((current_ppet8 <<
					HE_PPE_THRESH_NSS_RU_FSZ) | current_ppet16) <<
					(ru_w_idx * HE_PPE_THRESH_NSS_RU_FSZ * 2));

				if (sh->min_ppe_nss > sh->ppe_nss[nss_idx]) {
					sh->min_ppe_nss = sh->ppe_nss[nss_idx];
				}
			}
		}
		ASSERT(ru_r_idx == num_ru);
	}

	return BCME_OK;
}

/**
 * Extract he rateset from capabilities ie
 * @param[in] cap   HE capability element in network endianness
 */
static int
wlc_he_parse_rateset_cap_ie(wlc_info_t *wlc, he_cap_ie_t *cap, wlc_he_rateset_t *he_rateset)
{
	uint min_ie_size = sizeof(*cap) - TLV_HDR_LEN + HE_MIN_HE_RATESETSZ;
	uint32 channel_width_set;
	uint8 *bw_info;

	he_rateset->bw80_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	he_rateset->bw80_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	he_rateset->bw160_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	he_rateset->bw160_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	he_rateset->bw80p80_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	he_rateset->bw80p80_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;

	if (cap->len < min_ie_size) {
		return BCME_ERROR;
	}
	channel_width_set = getbits((uint8*)&cap->phy_cap, sizeof(cap->phy_cap),
		HE_PHY_CH_WIDTH_SET_IDX, HE_PHY_CH_WIDTH_SET_FSZ);
	bw_info = (uint8*)cap->phy_cap + HE_PHY_CAP_INFO_SIZE;

	he_rateset->bw80_rx_mcs_nss = getbits(bw_info, HE_BW_RASETSZ,
		HE_BW80_RX_RATESET_OFFSET * NBBY, HE_BW_RX_RASETSZ * NBBY);
	he_rateset->bw80_tx_mcs_nss = getbits(bw_info, HE_BW_RASETSZ,
		HE_BW80_TX_RATESET_OFFSET * NBBY, HE_BW_TX_RASETSZ * NBBY);

	if ((!(channel_width_set & HE_PHY_CH_WIDTH_5G_160)) || (BAND_2G(wlc->band->bandtype))) {
		return BCME_OK;
	}
	min_ie_size += HE_BW_RASETSZ;
	if (cap->len < min_ie_size) {
		return BCME_ERROR;
	}

	he_rateset->bw160_rx_mcs_nss = getbits(bw_info, 2 * HE_BW_RASETSZ,
		HE_BW160_RX_RATESET_OFFSET * NBBY, HE_BW_RX_RASETSZ * NBBY);
	he_rateset->bw160_tx_mcs_nss = getbits(bw_info, 2 * HE_BW_RASETSZ,
		HE_BW160_TX_RATESET_OFFSET * NBBY, HE_BW_TX_RASETSZ * NBBY);

	if (!(channel_width_set & HE_PHY_CH_WIDTH_5G_80P80)) {
		return BCME_OK;
	}
	min_ie_size += HE_BW_RASETSZ;
	if (cap->len < min_ie_size) {
		return BCME_ERROR;
	}

	he_rateset->bw80p80_rx_mcs_nss = getbits(bw_info, 3 * HE_BW_RASETSZ,
		HE_BW80P80_RX_RATESET_OFFSET * NBBY, HE_BW_RX_RASETSZ * NBBY);
	he_rateset->bw80p80_tx_mcs_nss = getbits(bw_info, 3 * HE_BW_RASETSZ,
		HE_BW80P80_TX_RATESET_OFFSET * NBBY, HE_BW_TX_RASETSZ * NBBY);

	return BCME_OK;
}

/**
 * Called on reception of a DOT11_MNG_HE_CAP_ID ie to process it.
 * @param[in] cap   HE capability element in network endianness
 */
static void
wlc_he_parse_cap_ie(wlc_he_info_t *hei, scb_t *scb, he_cap_ie_t *cap)
{
	scb_he_t *sh;
	wlc_info_t *wlc = hei->wlc;
	const uint fixed_ie_size = sizeof(*cap) - TLV_HDR_LEN + HE_MIN_HE_RATESETSZ;
	wlc_bsscfg_t *cfg = SCB_BSSCFG(scb);
	chanspec_t chspec;
	wlc_bss_info_t *bi = cfg->target_bss;
	wlc_he_rateset_t he_rateset;
	wlc_rateset_t *rateset;
	wlc_rateset_t *our_rateset;
	uint32 channel_width_set;
	uint mcs_nss_setsz;
	uint8 *data;
	uint32 default_ppe_nss;
	uint nss;
	uint16 omi_rx_nss;
	uint16 omi_bw;
	uint16 omi_tx_nss;
	uint8 oper_mode;
	uint legacy_omi_bw;
	uint bw;
	uint txstreams = WLC_BITSCNT(wlc->stf->txchain);
	uint rxstreams = wlc_stf_rxstreams_get(wlc);
	uint flags3 = scb->flags3;
	uint flags2 = scb->flags2;
	uint flags = scb->flags;

	/* If not associated, should use wlc->chanspec */
	chspec = (SCB_ASSOCIATED(scb) || SCB_LEGACY_WDS(scb)) ?
			cfg->current_bss->chanspec : wlc->chanspec;

	/* Clear HE flag */
	scb->flags2 &= ~SCB2_HECAP;
	rateset = &scb->rateset;
	rateset->he_bw80_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	rateset->he_bw80_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	rateset->he_bw160_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	rateset->he_bw160_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	rateset->he_bw80p80_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	rateset->he_bw80p80_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;

	sh = SCB_HE(hei, scb);
	ASSERT(sh != NULL);

	bzero(&sh->mac_cap, sizeof(sh->mac_cap));
	bzero(&sh->phy_cap, sizeof(sh->phy_cap));

	if ((cap == NULL) || (!BSS_HE_ENAB_BAND(wlc, wlc->band->bandtype, cfg))) {
		return;
	}

	if (cap->len < fixed_ie_size) {
		WL_HE_INFO(("wl%d: %s: Invalid CAP IE len %d\n", wlc->pub->unit, __FUNCTION__,
			cap->len));
		return;
	}

	memcpy(&sh->mac_cap, &cap->mac_cap, sizeof(sh->mac_cap));
	memcpy(&sh->phy_cap, &cap->phy_cap, sizeof(sh->phy_cap));

	/* Set HE capability if HE MAC CAP IE is not empty */
	SCB_SET_HE_CAP(scb);

	/* Dynamic fragment support */
	/* We support Level 2 */
	if (getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap), HE_MAC_FRAG_SUPPORT_IDX,
		HE_MAC_FRAG_SUPPORT_FSZ) >= HE_MAC_FRAG_ONE_PER_AMPDU) {
		sh->flags |= SCB_HE_DYNFRAG_RXEN;
	} else {
		sh->flags &= (~SCB_HE_DYNFRAG_RXEN);
	}

	/* HE HTC support */
	if (D11REV_GE(wlc->pub->corerev, 129)) {
		if (getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap), HE_MAC_HTC_HE_SUPPORT_IDX,
			HE_MAC_HTC_HE_SUPPORT_FSZ)) {
			sh->flags |= SCB_HE_HTC_CAP;
		}
	}

	/* UL 2x996 RU support */
	if (getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap), HE_MAC_UL_2X996_TONE_RU_IDX,
		HE_MAC_UL_2X996_TONE_RU_FSZ)) {
		sh->flags |= SCB_HE_UL2x996;
	}

	if (TWT_ENAB(wlc->pub)) {
		wlc_twt_scb_set_cap(wlc->twti, scb, WLC_TWT_CAP_TWT_REQ_SUPPORT,
			isset((uint8 *)&sh->mac_cap, HE_MAC_TWT_REQ_SUPPORT_IDX));
		wlc_twt_scb_set_cap(wlc->twti, scb, WLC_TWT_CAP_BCAST_TWT_SUPPORT,
			isset((uint8 *)&sh->mac_cap, HE_MAC_BCAST_TWT_SUPPORT_IDX));
		wlc_twt_scb_set_cap(wlc->twti, scb, WLC_TWT_CAP_TWT_RESP_SUPPORT,
			isset((uint8 *)&sh->mac_cap, HE_MAC_TWT_RESP_SUPPORT_IDX));
	}

	sh->trig_mac_pad_dur = (uint8)getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap),
		HE_MAC_TRG_PAD_DUR_IDX, HE_MAC_TRG_PAD_DUR_FSZ);
	sh->trig_mac_pad_dur <<= 3; /* convert from 8 usec unit to usec */

	sh->multi_tid_agg_num = (uint8)getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap),
		HE_MAC_MULTI_TID_AGG_IDX, HE_MAC_MULTI_TID_AGG_FSZ) + 1;

	/* LDPC coding in payload */
	if (getbits((uint8 *)&sh->phy_cap, sizeof(sh->phy_cap),
		HE_PHY_LDPC_PYLD_IDX, HE_PHY_LDPC_PYLD_FSZ)) {
		sh->flags |= SCB_HE_LDPCCAP;
	}

	/* Check if the peer can Rx 1024 QAM in <242 tone RU */
	if (getbits((uint8*)&hei->def_phy_cap, sizeof(hei->def_phy_cap),
		HE_PHY_TX_1024_QAM_LESS_242_TONE_RU_IDX,
		HE_PHY_TX_1024_QAM_LESS_242_TONE_RU_FSZ) &&
		getbits((uint8 *)&sh->phy_cap, sizeof(sh->phy_cap),
			HE_PHY_RX_1024_QAM_LESS_242_TONE_RU_IDX,
			HE_PHY_RX_1024_QAM_LESS_242_TONE_RU_FSZ)) {
		sh->flags |= SCB_HE_DL_QAM1024_LT242;
	}
	/* Check if the peer can Tx 1024 QAM in <242 tone RU */
	if (getbits((uint8*)&hei->def_phy_cap, sizeof(hei->def_phy_cap),
		HE_PHY_RX_1024_QAM_LESS_242_TONE_RU_IDX,
		HE_PHY_RX_1024_QAM_LESS_242_TONE_RU_FSZ) &&
		getbits((uint8 *)&sh->phy_cap, sizeof(sh->phy_cap),
			HE_PHY_TX_1024_QAM_LESS_242_TONE_RU_IDX,
			HE_PHY_TX_1024_QAM_LESS_242_TONE_RU_FSZ)) {
		sh->flags |= SCB_HE_UL_QAM1024_LT242;
	}

	/* Check device class */
	if (getbits((uint8 *)&sh->phy_cap, sizeof(sh->phy_cap),
		HE_PHY_DEVICE_CLASS_IDX, HE_PHY_DEVICE_CLASS_FSZ)) {
		sh->flags |= SCB_HE_DEVICE_CLASS;
	}

	/* Check if remote is BSR capable */
	if (isset((uint8 *)&sh->mac_cap, HE_MAC_A_BSR_IDX)) {
		sh->flags |= SCB_HE_BSR_CAPABLE;
	}

	/* Check if 80IN160 capable */
	if (getbits((uint8 *)&sh->phy_cap, sizeof(sh->phy_cap),
		HE_PHY_80M_IN_160MOR80P80_PPDU_IDX,
		HE_PHY_80M_IN_160MOR80P80_PPDU_FSZ)) {
		sh->flags |= SCB_HE_80IN160;
	}

	/**
	 * TX-RX HE-MCS support:
	 */
	if (wlc_he_parse_rateset_cap_ie(wlc, cap, &he_rateset)) {
		WL_HE_INFO(("wl%d: %s: Parsing CAP IE HE rateset failed\n", wlc->pub->unit,
			__FUNCTION__));
		return;
	}

	if (!(sh->flags & SCB_HE_DL_QAM1024_LT242) && wlc_musched_wfa20in80_enab(wlc->musched)) {
		/* remove 1024qam rate to tx dl ofdma to the user for the wfa WAR */
		wlc_he_remove_1024qam(wlc, txstreams, &he_rateset.bw80_rx_mcs_nss);
		wlc_he_remove_1024qam(wlc, txstreams, &he_rateset.bw160_rx_mcs_nss);
		wlc_he_remove_1024qam(wlc, txstreams, &he_rateset.bw80p80_rx_mcs_nss);
		wlc_he_remove_1024qam(wlc, rxstreams, &he_rateset.bw80_tx_mcs_nss);
		wlc_he_remove_1024qam(wlc, rxstreams, &he_rateset.bw160_tx_mcs_nss);
		wlc_he_remove_1024qam(wlc, rxstreams, &he_rateset.bw80p80_tx_mcs_nss);
	}

	/* Two reasons for getting here: assoc_req or assoc_resp. The req needs intersection
	 * but the response should in theory be good. However, it is easier and safer to intersect.
	 */
	our_rateset = &wlc->bandstate[wlc->band->bandunit]->hw_rateset;
	/* In rateset it is stored from 'our' perspective. So our tx becomes intersection of
	 * our tx capability and remote rx capability.
	 */
	rateset->he_bw80_tx_mcs_nss = wlc_he_rateset_intersection(he_rateset.bw80_rx_mcs_nss,
		our_rateset->he_bw80_tx_mcs_nss);
	rateset->he_bw80_rx_mcs_nss = wlc_he_rateset_intersection(he_rateset.bw80_tx_mcs_nss,
		our_rateset->he_bw80_rx_mcs_nss);
	rateset->he_bw160_tx_mcs_nss = wlc_he_rateset_intersection(he_rateset.bw160_rx_mcs_nss,
		our_rateset->he_bw160_tx_mcs_nss);
	rateset->he_bw160_rx_mcs_nss = wlc_he_rateset_intersection(he_rateset.bw160_tx_mcs_nss,
		our_rateset->he_bw160_rx_mcs_nss);
	rateset->he_bw80p80_tx_mcs_nss = wlc_he_rateset_intersection(he_rateset.bw80p80_rx_mcs_nss,
		our_rateset->he_bw80p80_tx_mcs_nss);
	rateset->he_bw80p80_rx_mcs_nss = wlc_he_rateset_intersection(he_rateset.bw80p80_tx_mcs_nss,
		our_rateset->he_bw80p80_rx_mcs_nss);
	/* Apply BW limitation */
	if (wlc_he_bw80_limited(wlc, cfg)) {
		rateset->he_bw160_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
		rateset->he_bw160_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
		rateset->he_bw80p80_tx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
		rateset->he_bw80p80_rx_mcs_nss = HE_CAP_MAX_MCS_NONE_ALL;
	}
	/* Now apply the rx and tx chain limits */
	for (nss = rxstreams + 1; nss <= HE_CAP_MCS_MAP_NSS_MAX; nss++) {
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw80_rx_mcs_nss);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw160_rx_mcs_nss);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw80p80_rx_mcs_nss);
	}
	for (nss = txstreams + 1; nss <= HE_CAP_MCS_MAP_NSS_MAX; nss++) {
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw80_tx_mcs_nss);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw160_tx_mcs_nss);
		HE_MCS_MAP_SET_MCS_PER_SS(nss, HE_CAP_MAX_MCS_NONE, rateset->he_bw80p80_tx_mcs_nss);
	}

#ifdef HERTDBG
	WL_PRINT(("HE-Rate Cap IE mcs map\n"));
	WL_PRINT(("BW80    Remote Rx: %04x, Our Tx: %04x, intersect: %04x\n",
		he_rateset.bw80_rx_mcs_nss, hei->he_rateset.bw80_tx_mcs_nss,
		rateset->he_bw80_tx_mcs_nss));
	WL_PRINT(("BW80    Remote Tx: %04x, Our Rx: %04x, intersect: %04x\n",
		he_rateset.bw80_tx_mcs_nss, hei->he_rateset.bw80_rx_mcs_nss,
		rateset->he_bw80_rx_mcs_nss));
	WL_PRINT(("BW160   Remote Rx: %04x, Our Tx: %04x, intersect: %04x\n",
		he_rateset.bw160_rx_mcs_nss, hei->he_rateset.bw160_tx_mcs_nss,
		rateset->he_bw160_tx_mcs_nss));
	WL_PRINT(("BW160   Remote Tx: %04x, Our Rx: %04x, intersect: %04x\n",
		he_rateset.bw160_tx_mcs_nss, hei->he_rateset.bw160_rx_mcs_nss,
		rateset->he_bw160_rx_mcs_nss));
	WL_PRINT(("BW80P80 Remote Rx: %04x, Our Tx: %04x, intersect: %04x\n",
		he_rateset.bw80p80_rx_mcs_nss, hei->he_rateset.bw80p80_tx_mcs_nss,
		rateset->he_bw80p80_tx_mcs_nss));
	WL_PRINT(("BW80P80 Remote Tx: %04x, Our Rx: %04x, intersect: %04x\n",
		he_rateset.bw80p80_tx_mcs_nss, hei->he_rateset.bw80p80_rx_mcs_nss,
		rateset->he_bw80p80_rx_mcs_nss));
#endif /* HERTDBG */

	/* Capability of the BSS */
	if (BSSCFG_STA(cfg)) {
		bi->he_neg_bw80_tx_mcs = rateset->he_bw80_tx_mcs_nss;
		bi->he_neg_bw80_rx_mcs = rateset->he_bw80_rx_mcs_nss;
		bi->he_neg_bw160_tx_mcs = rateset->he_bw160_tx_mcs_nss;
		bi->he_neg_bw160_rx_mcs = rateset->he_bw160_rx_mcs_nss;
		bi->he_neg_bw80p80_tx_mcs = rateset->he_bw80p80_tx_mcs_nss;
		bi->he_neg_bw80p80_rx_mcs = rateset->he_bw80p80_rx_mcs_nss;
		wlc_he_cp_he_rateset_to_bi_sup(bi,
			&wlc->bandstate[wlc->band->bandunit]->hw_rateset);
		wlc_rateset_he_cp(&bi->rateset, rateset);
	}
	/* Copy the MCS sets for all BWs to sh. They are needed to be able to apply OMI */
	sh->bw80_tx_mcs_nss = he_rateset.bw80_tx_mcs_nss;
	sh->bw80_rx_mcs_nss = he_rateset.bw80_rx_mcs_nss;
	sh->bw160_tx_mcs_nss = he_rateset.bw160_tx_mcs_nss;
	sh->bw160_rx_mcs_nss = he_rateset.bw160_rx_mcs_nss;
	sh->bw80p80_tx_mcs_nss = he_rateset.bw80p80_tx_mcs_nss;
	sh->bw80p80_rx_mcs_nss = he_rateset.bw80p80_rx_mcs_nss;

	/* Initialize OMI values with capabilities, so no limit gets accidently applied */
	if (getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap),
		HE_MAC_OM_CONTROL_SUPPORT_IDX, HE_MAC_OM_CONTROL_SUPPORT_FSZ)) {
		sh->flags |= SCB_HE_OMI;
		if (getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap),
			HE_MAC_OM_UL_MU_DATA_DISABLE_IDX, HE_MAC_OM_UL_MU_DATA_DISABLE_FSZ)) {
			sh->flags |= SCB_HE_OMI_UL_MU_DATA_DIS;
		}
	}

	/* parse and apply channel width indication */
	channel_width_set = getbits((uint8*)&sh->phy_cap, sizeof(sh->phy_cap),
		HE_PHY_CH_WIDTH_SET_IDX, HE_PHY_CH_WIDTH_SET_FSZ);
	scb->flags3 &= ~(SCB3_IS_160|SCB3_IS_80_80);
	scb->flags &= ~(SCB_IS40);
	if (channel_width_set & HE_PHY_CH_WIDTH_5G_80) {
		/* implies 40MHz too */
		scb->flags |= SCB_IS40;
		if (channel_width_set & HE_PHY_CH_WIDTH_5G_160) {
			scb->flags3 |= SCB3_IS_160;
		}
		if (channel_width_set & HE_PHY_CH_WIDTH_5G_80P80) {
			scb->flags3 |= (SCB3_IS_80_80);
		}
	}
	if (channel_width_set & HE_PHY_CH_WIDTH_2G_40) {
		scb->flags |= SCB_IS40;
	}
	/* If the STA indicates support for anything higher then 20Mhz then 242TONE is reserved (0)
	 * but it means that 242TONE is supported, so we set the flag in that case.
	 */
	if ((scb->flags & SCB_IS40) ||
		(channel_width_set & (HE_PHY_CH_WIDTH_2G_242TONE | HE_PHY_CH_WIDTH_5G_242TONE))) {

		sh->flags |= SCB_HE_DL_242TONE;
	}
	/* For BW take the rx set. The Tx set BW limitation may be different but will
	 * get applied through rate table, it is unclear at this point if ucode needs
	 * BW limitation for Tx?
	 */
	if ((sh->bw80p80_rx_mcs_nss != HE_CAP_MAX_MCS_NONE_ALL) ||
	    (sh->bw160_rx_mcs_nss != HE_CAP_MAX_MCS_NONE_ALL)) {
		omi_bw = DOT11_OPER_MODE_160MHZ;
	} else {
		/* Max is 80, but in 2G it is 40 or 20, use chanspec to determine */
		omi_bw = DOT11_OPER_MODE_80MHZ;
		if (BAND_2G(wlc->band->bandtype)) {
			if ((CHSPEC_IS40(chspec)) &&
			    (channel_width_set & HE_PHY_CH_WIDTH_2G_40)) {
				omi_bw = DOT11_OPER_MODE_40MHZ;
			} else {
				omi_bw = DOT11_OPER_MODE_20MHZ;
			}
		} else {
			/* 5G can be limited to 20 (or 40) only. If STA, apply BW_CAP. If AP
			 * then observe channel_width and bw_cap and chanspec.
			 */
			if (BSSCFG_AP(cfg)) {
				if (CHSPEC_IS20(chspec) ||
					!(channel_width_set & HE_PHY_CH_WIDTH_5G_80)) {
					omi_bw = DOT11_OPER_MODE_20MHZ;
					sh->flags |= SCB_HE_5G_20MHZ_ONLY;
				} else if (CHSPEC_IS40(chspec)) {
					omi_bw = DOT11_OPER_MODE_40MHZ;
				}
			} else {
				if (CHSPEC_IS20(chspec)) {
					omi_bw = DOT11_OPER_MODE_20MHZ;
				} else if (CHSPEC_IS40(chspec)) {
					omi_bw = DOT11_OPER_MODE_40MHZ;
				}
			}
		}
	}

	/* Determine omi rx nss, derive from tx mcs nss. Omi rx is remote rx, which is our tx */
	omi_rx_nss = HE_MAX_SS_SUPPORTED(rateset->he_bw80_tx_mcs_nss);
	if ((HE_MAX_SS_SUPPORTED(rateset->he_bw160_tx_mcs_nss)) > omi_rx_nss)
		omi_rx_nss = HE_MAX_SS_SUPPORTED(rateset->he_bw160_tx_mcs_nss);

	if ((HE_MAX_SS_SUPPORTED(rateset->he_bw80p80_tx_mcs_nss)) > omi_rx_nss)
		omi_rx_nss = HE_MAX_SS_SUPPORTED(rateset->he_bw80p80_tx_mcs_nss);
	ASSERT(omi_rx_nss > 0);
	omi_rx_nss -= 1;

	omi_tx_nss = HE_MAX_SS_SUPPORTED(rateset->he_bw80_rx_mcs_nss);
	if ((HE_MAX_SS_SUPPORTED(rateset->he_bw160_rx_mcs_nss)) > omi_tx_nss)
		omi_tx_nss = HE_MAX_SS_SUPPORTED(rateset->he_bw160_rx_mcs_nss);

	if ((HE_MAX_SS_SUPPORTED(rateset->he_bw80p80_rx_mcs_nss)) > omi_tx_nss)
		omi_tx_nss = HE_MAX_SS_SUPPORTED(rateset->he_bw80p80_rx_mcs_nss);
	ASSERT(omi_tx_nss > 0);
	omi_tx_nss -= 1;

	sh->omi_htc = sh->omi_lm = HE_OMI_ENCODE(omi_rx_nss, omi_tx_nss, omi_bw,
		!HE_ERSU_RX_ENAB(wlc->pub), 0, 0, 0);
	if ((flags3 != scb->flags3) || (flags2 != scb->flags2) || (flags != scb->flags)) {
		/* need to reinit ratesel before determining link BW */
		wlc_scb_ratesel_init(wlc, scb);
	}

	bw = wlc_scb_ratesel_get_link_bw(wlc, scb);
	if (bw == BW_160MHZ) {
		legacy_omi_bw = DOT11_OPER_MODE_160MHZ;
	} else if (bw == BW_80MHZ) {
		legacy_omi_bw = DOT11_OPER_MODE_80MHZ;
	} else if (bw == BW_40MHZ) {
		legacy_omi_bw = DOT11_OPER_MODE_40MHZ;
	} else {
		legacy_omi_bw = DOT11_OPER_MODE_20MHZ;
	}

	if (legacy_omi_bw <= omi_bw) {
		omi_bw = legacy_omi_bw;
	} else {
		/* Apply the current settings in oper mode. This will make sure the determined BW
		 * will be applied by scb_ratesel
		 */
		oper_mode = DOT11_D8_OPER_MODE(0, omi_rx_nss + 1, 0,
			(omi_bw == DOT11_OPER_MODE_160MHZ) ?  1 : 0,
			(omi_bw == DOT11_OPER_MODE_160MHZ) ? DOT11_OPER_MODE_80MHZ : omi_bw);
		wlc_vht_update_scb_oper_mode(wlc->vhti, scb, oper_mode);
	}

	/* OMI HE ctrl frame, LSB -> MSB: RxNss (3b) | BW (2b) | UL_MU_dis (1b) | TxNSS (3b) |
	 * ER SU Disable (1b) | DL MU_MIMO re-sound (1b) | UL MU Data Dsiable. Note: not all
	 * fields are written. Encoding is copying HTC omi, use those defines!
	 */
	sh->omi_htc = sh->omi_lm = HE_OMI_ENCODE(omi_rx_nss, omi_tx_nss, omi_bw,
		!HE_ERSU_RX_ENAB(wlc->pub), 0, 0, 0);
	/* pmqdata updates only lower 5bits of RXNSS | BW */
	sh->omi_pmq = (sh->omi_htc & HE_TXRATE_UPDATE_MASK);

	/* Parse PPET values only once. */
	if (sh->ppet_valid == FALSE) {
		/* Only parse PPET information once, if not available set it to 0 us */
		sh->ppet_valid = TRUE;
		/* First check Nominal Packet Padding */
		default_ppe_nss = getbits((uint8 *)&sh->phy_cap, sizeof(sh->phy_cap),
			HE_PHY_NOMINAL_PACKET_PADDING_IDX,
			HE_PHY_NOMINAL_PACKET_PADDING_FSZ);
		STATIC_ASSERT(WL_HE_PPET_0US == HE_PHY_NOMINAL_PKT_PADDING_0_US);
		STATIC_ASSERT(WL_HE_PPET_8US == HE_PHY_NOMINAL_PKT_PADDING_8_US);
		STATIC_ASSERT(WL_HE_PPET_16US == HE_PHY_NOMINAL_PKT_PADDING_16_US);
		if (default_ppe_nss != HE_PHY_NOMINAL_PKT_PADDING_RESERVED) {
			/* If valid, default to max of own limitation and peer's nominal setting */
			default_ppe_nss = MAX(default_ppe_nss, hei->ppet);
		} else {
			/* Else pick own limitation as default */
			default_ppe_nss = hei->ppet;
		}
		/* Initialize ppet values with default, then parse IE (if available)
		 * to adjust to possible higher value.
		 */
		if (default_ppe_nss == WL_HE_PPET_0US) {
			default_ppe_nss = (HE_CONST_IDX_NONE << HE_PPE_THRESH_NSS_RU_FSZ) |
				HE_CONST_IDX_NONE;
		} else if (default_ppe_nss == WL_HE_PPET_8US) {
			default_ppe_nss = (HE_CONST_IDX_BPSK << HE_PPE_THRESH_NSS_RU_FSZ) |
				HE_CONST_IDX_NONE;
		} else { /* 16 usec */
			default_ppe_nss = (HE_CONST_IDX_NONE << HE_PPE_THRESH_NSS_RU_FSZ) |
				HE_CONST_IDX_BPSK;
		}
		default_ppe_nss = HE_EXPAND_PPET_ALL_RU(default_ppe_nss);
		for (nss = 0; nss < ARRAYSIZE(sh->ppe_nss); nss++) {
			sh->ppe_nss[nss] = default_ppe_nss;
		}
		sh->min_ppe_nss = default_ppe_nss;
		/* First check if PPET IE is available */
		if (isset(&sh->phy_cap, HE_PHY_PPE_THRESH_PRESENT_IDX)) {
			/* need to skip var supported HE-MCS and NSS Set field */
			mcs_nss_setsz = 0;
			if ((channel_width_set & HE_PHY_CH_WIDTH_5G_160) &&
			    BAND_5G6G(wlc->band->bandtype))
				mcs_nss_setsz += HE_BW_RASETSZ;
			if ((channel_width_set & HE_PHY_CH_WIDTH_5G_80P80) &&
			    BAND_5G6G(wlc->band->bandtype))
				mcs_nss_setsz += HE_BW_RASETSZ;

			if (cap->len <= fixed_ie_size + mcs_nss_setsz) {
				WL_HE_INFO(("wl%d: %s: Invalid CAP IE len (PPE) %d\n",
					wlc->pub->unit, __FUNCTION__, cap->len));
			} else {
				data = (uint8*)cap->phy_cap + HE_PHY_CAP_INFO_SIZE +
					HE_MIN_HE_RATESETSZ;
				wlc_he_parse_cap_ppe_thresh(hei, sh, data + mcs_nss_setsz,
					cap->len - (fixed_ie_size + mcs_nss_setsz));
			}
		}
	}

#ifdef WL_BEAMFORMING
	/* beamforming caps */
	if (TXBF_ENAB(wlc->pub)) {
		uint8 su_bfe, su_bfr, mu_bfr, cqi;
		WL_HE_INFO(("wl%d %s ie %d len %d id_ext %d\n", wlc->pub->unit, __FUNCTION__,
			cap->id, cap->len, cap->id_ext));

		su_bfr = getbits((uint8 *)&cap->phy_cap, sizeof(cap->phy_cap),
			HE_PHY_SU_BEAMFORMER_IDX, HE_PHY_SU_BEAMFORMER_FSZ);
		su_bfe = getbits((uint8 *)&cap->phy_cap, sizeof(cap->phy_cap),
			HE_PHY_SU_BEAMFORMEE_IDX, HE_PHY_SU_BEAMFORMEE_FSZ);
		mu_bfr = getbits((uint8 *)&cap->phy_cap, sizeof(cap->phy_cap),
			HE_PHY_MU_BEAMFORMER_IDX, HE_PHY_MU_BEAMFORMER_FSZ);
		cqi = getbits((uint8 *)&cap->phy_cap, sizeof(cap->phy_cap),
			HE_PHY_TRG_CQI_FEEDBACK_IDX, HE_PHY_TRG_CQI_FEEDBACK_FSZ);

		/* peer should be SU beamformer as well to become MU beamformer */
		if (su_bfr) {
			sh->flags |= SCB_HE_SU_BFR;
			if (mu_bfr)
				sh->flags |= SCB_HE_MU_BFR;
		}

		/* non-AP STA has to support SU_BFE. For HE, SU_BFE=1 implies MU_BFE=1 */
		if (su_bfe) {
			 sh->flags |= SCB_HE_SU_MU_BFE;
		}

		if (cqi) {
			sh->flags |= SCB_HE_CQI_BFE;
		}
		wlc_txbf_scb_state_upd(hei->wlc->txbf, scb, (uint8 *) &cap->phy_cap,
			sizeof(cap->phy_cap), TXBF_CAP_TYPE_HE);
	}
#endif /*  WL_BEAMFORMING */

	if (AMPDU_ENAB(wlc->pub)) {
		uint8 rxfactor_ext = getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap),
			HE_MAC_MAX_AMPDU_LEN_EXP_IDX, HE_MAC_MAX_AMPDU_LEN_EXP_FSZ);
		wlc_ampdu_scb_set_params(wlc, scb, -1, -1, rxfactor_ext);
	}

	/* Trigger ratelinkmem update to make sure all values get programmed */
	if (RATELINKMEM_ENAB(wlc->pub)) {
		wlc_ratelinkmem_upd_lmem_int(wlc, scb, TRUE /* clr_txbf_sts=1 in mreq */);
	}
} /* wlc_he_parse_cap_ie */

/** called on reception of assocreq/reassocreq/assocresp/reassocresp */
static int
wlc_he_assoc_parse_cap_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_he_info_t *hei = ctx;
	scb_t *scb = wlc_iem_parse_get_assoc_bcn_scb(data);

	BCM_REFERENCE(hei);

	if (scb == NULL)
		return BCME_OK;

	switch (data->ft) {
#ifdef AP
	case FC_ASSOC_REQ:
	case FC_REASSOC_REQ: {
		wlc_iem_ft_pparm_t *ftpparm = data->pparm->ft;

		wlc_he_parse_cap_ie(hei, scb, (he_cap_ie_t *)data->ie);

		ftpparm->assocreq.he_cap_ie = data->ie;
		break;
	}
#endif /* AP */
#ifdef STA
	case FC_ASSOC_RESP:
	case FC_REASSOC_RESP: {
		wlc_he_parse_cap_ie(hei, scb, (he_cap_ie_t *)data->ie);
		break;
	}
#endif /* STA */
	default:
		break;
	}

	return BCME_OK;
}

static int
wlc_he_scan_parse_cap_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	he_cap_ie_t *cap;
	wlc_iem_ft_pparm_t *ftpparm;
	wlc_bss_info_t *bi;
	const uint fixed_ie_size = sizeof(*cap) - TLV_HDR_LEN + HE_MIN_HE_RATESETSZ;
	wlc_he_rateset_t he_rateset;
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;

	BCM_REFERENCE(wlc);

	ftpparm = data->pparm->ft;
	bi = ftpparm->scan.result;

	bi->flags3 &= ~(WLC_BSS3_HE);

	if ((cap = (he_cap_ie_t *)data->ie) == NULL) {
		return BCME_OK;
	}

	if (cap->len < fixed_ie_size) {
		WL_HE_INFO(("wl%d: %s: Invalid CAP IE len %d\n", wlc->pub->unit, __FUNCTION__,
			cap->len));
		return BCME_OK;
	}

	if (wlc_he_parse_rateset_cap_ie(wlc, cap, &he_rateset)) {
		WL_HE_INFO(("wl%d: %s: Parsing CAP IE HE rateset failed\n", wlc->pub->unit,
			__FUNCTION__));
		return BCME_OK;
	}

	wlc_he_cp_rateset_to_wlc(&he_rateset, &bi->rateset);
	wlc_he_cp_he_rateset_to_bi_sup(bi, &bi->rateset);
	wlc_he_init_bi_rateset_to_none(bi);

	/* Mark the BSS as HE capable */
	bi->flags3 |= WLC_BSS3_HE;

	return BCME_OK;
}

/**
 * When the user sets the rx factor for this local node using e.g. the wl utility, the emitted
 * capabilities have to change.
 */
void
wlc_he_update_ampdu_cap(wlc_he_info_t *hei, uint8 rx_factor, uint8 mpdu_density)
{
	WL_TRACE(("wl%d: %s(%04x)\n", hei->wlc->pub->unit, __FUNCTION__, rx_factor));

	ASSERT(mpdu_density < 8);
	setbits((uint8 *)&hei->def_6g_caps_info, sizeof(hei->def_6g_caps_info),
		HE_6G_CAP_MIN_MPDU_START_SPC_IDX, HE_6G_CAP_MIN_MPDU_START_SPC_FSZ, mpdu_density);
	setbits((uint8 *)&hei->def_6g_caps_info, sizeof(hei->def_6g_caps_info),
		HE_6G_CAP_MAX_AMPDU_LEN_EXP_IDX, HE_6G_CAP_MAX_AMPDU_LEN_EXP_FSZ, rx_factor);
}

/** HE 6GHz band capabilities element */
static uint
wlc_he_calc_6g_caps_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc = hei->wlc;

	if (!data->cbparm->he)
		return 0;

	if (wlc->band->bandunit != BAND_6G_INDEX)
		return 0;

	return sizeof(he_6g_band_caps_ie_t);
}

/** HE 6GHz band capabilities element, only to be transmitted in the 6GHz band */
static int
wlc_he_write_6g_caps_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t *wlc = hei->wlc;
	he_6g_band_caps_ie_t *ie = (he_6g_band_caps_ie_t *)data->buf;

	if (wlc->band->bandunit != BAND_6G_INDEX)
		return BCME_OK;

	ie->id = DOT11_MNG_ID_EXT_ID;
	ie->id_ext = EXT_MNG_HE_6G_BAND_CAPS_ID;
	ie->len = sizeof(he_6g_band_caps_ie_t) - TLV_HDR_LEN;
	ie->caps_info = hei->def_6g_caps_info;

	setbits((uint8 *)&ie->caps_info, sizeof(ie->caps_info),
		HE_6G_CAP_SM_PWR_SAVE_IDX, HE_6G_CAP_SM_PWR_SAVE_FSZ,
		wlc->stf->mimops_mode);

	return BCME_OK;
}

/** called on reception of a DOT11_MNG_HE_CAP_ID ie to process it */
static void
wlc_he_parse_6g_caps_ie(wlc_he_info_t *hei, scb_t *scb, he_6g_band_caps_ie_t *ie)
{
	wlc_info_t *wlc = hei->wlc;
	uint8 density;
	uint8 max_exp;

	if (ie == NULL) {
		return;
	}

	if (ie->len < sizeof(he_6g_band_caps_ie_t) - TLV_HDR_LEN) {
		WL_HE_INFO(("wl%d: %s: Invalid EXT CAP IE len %d\n", wlc->pub->unit, __FUNCTION__,
			ie->len));
		return;
	}

	max_exp = getbits((uint8*)&ie->caps_info, sizeof(ie->caps_info),
		HE_6G_CAP_MAX_AMPDU_LEN_EXP_IDX, HE_6G_CAP_MAX_AMPDU_LEN_EXP_FSZ);
	density = getbits((uint8*)&ie->caps_info, sizeof(ie->caps_info),
		HE_6G_CAP_MIN_MPDU_START_SPC_IDX, HE_6G_CAP_MIN_MPDU_START_SPC_FSZ);

	wlc_ampdu_scb_set_params(wlc, scb, density, max_exp, -1);

	if (AMPDU_ENAB(wlc->pub)) {
		scb->flags |= SCB_AMPDUCAP;
	}
} /* wlc_he_parse_6g_caps_ie */

/** called on reception of assocreq/reassocreq/assocresp/reassocresp */
static int
wlc_he_assoc_parse_6g_caps_ie(void *ctx, wlc_iem_parse_data_t *data)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
#ifdef STA
	wlc_iem_ft_pparm_t *ftpparm = data->pparm->ft;
#endif /* STA */
	scb_t *scb;
	BCM_REFERENCE(hei);

	scb = wlc_iem_parse_get_assoc_bcn_scb(data);
	if (scb == NULL)
		return BCME_OK;

	wlc_he_parse_6g_caps_ie(hei, scb, (he_6g_band_caps_ie_t *)data->ie);
#ifdef STA
	ftpparm->assocreq.he_6g_caps_ie = data->ie;
#endif /* STA */

	return BCME_OK;
} /* wlc_he_assoc_parse_6g_caps_ie */

/** HE operation element */
static uint
wlc_he_calc_op_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	uint len;
	wlc_bsscfg_t *cfg = data->cfg;

	if (!data->cbparm->he)
		return 0;

	if (!BSSCFG_AP(cfg) && !BSSCFG_IBSS(cfg))
		return 0;

	len = TLV_HDR_LEN + HE_OP_IE_MIN_LEN;

	if (BSSCFG_AP(cfg) && !MBSSID_ENAB(cfg->wlc->pub, cfg->wlc->band->bandtype) &&
		MBSS_ENAB(cfg->wlc->pub) && (AP_BSS_UP_COUNT(cfg->wlc) > 1)) {
			len += HE_OP_COHOST_BSS_SIZE;
	}

	if (CHSPEC_IS6G(cfg->current_bss->chanspec)) {
		len += HE_OP_6GHZ_OP_INFO_SIZE;
	}

	return len;
}

/** AP/IBSS centric, called on transmit of bcn/prbrsp/assocresp/reassocresp */
static int
wlc_he_write_op_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg = data->cfg;
	chanspec_t chspec = cfg->current_bss->chanspec;
	he_op_ie_t *op = (he_op_ie_t *)data->buf;
	bss_he_info_t *bhei = BSS_HE_INFO(hei, cfg);
	uint16 mcs_nss;
	/** TLV size [bytes], for building optional field(s) */
	uint tlv_byte_sz = HE_OP_IE_MIN_LEN + TLV_HDR_LEN;
#ifdef BAND6G
	int8 regulatory_info = 0;
#endif

	BCM_REFERENCE(wlc);

	if (!BSSCFG_AP(cfg) || (bhei == NULL)) {
		return BCME_OK;
	}

	/* IE header */
	op->id = DOT11_MNG_ID_EXT_ID;
	op->id_ext = EXT_MNG_HE_OP_ID;
	op->len = HE_OP_IE_MIN_LEN;

	/* BSS Color value */
	setbits((uint8 *)&op->color, sizeof(op->color), HE_OP_BSS_COLOR_IDX, HE_OP_BSS_COLOR_FSZ,
		hei->bsscolor);

	/* The color is updated when the "global" color has changed or when color disable field
	 * gets cleared, i.e. disable goes from TRUE => FALSE. The latter requires color update as
	 * well, as this is the place where the color gets programmed to phy, but only when
	 * bsscolor_counter reaches 1 which is the moment the switch occurs.
	 */
	if ((bhei->bsscolor != hei->bsscolor) ||
	    ((bhei->bsscolor_disable) && (!hei->bsscolor_disable) &&
	     (hei->update_bsscolor_counter == 1))) {
		bhei->bsscolor = hei->bsscolor;
		/* find all scbs for this bsscfg and get ratelinkmem updated */
		if (RATELINKMEM_ENAB(wlc->pub)) {
			wlc_ratelinkmem_update_link_entry_all(wlc, cfg, FALSE,
				FALSE /* clr_txbf_stats=0 in mreq */);
		}
		/* Configure phy rx filter */
		bhei->bsscolor_disable = hei->bsscolor_disable;
		wlc_he_configure_bsscolor_phy(hei, cfg);
	}
	/* BSS Color disabled */
	bhei->bsscolor_disable = hei->bsscolor_disable;
	if ((hei->bsscolor_disable) || (hei->update_bsscolor_counter > 1)) {
		setbit(&op->color, HE_OP_BSS_COLOR_DISABLED_IDX);
	}

	/* Default PE Duration */
	setbits((uint8 *)&op->parms, sizeof(op->parms), HE_OP_DEF_PE_DUR_IDX, HE_OP_DEF_PE_DUR_FSZ,
		bhei->pe_duration);

	/* HE duration based RTS Threshold */
	setbits((uint8 *)&op->parms, sizeof(op->parms), HE_OP_HE_DUR_RTS_THRESH_IDX,
		HE_OP_HE_DUR_RTS_THRESH_FSZ,
		(bhei->rts_thresh >> HE_RTS_DUR_THRESHOLD_32USEC_SHIFT));

	/* 242-tone HE ER SU PPDU reception support */
	setbits((uint8 *)&op->parms, sizeof(op->parms), HE_OP_ER_SU_DISABLE_IDX,
		HE_OP_ER_SU_DISABLE_FSZ, !HE_ERSU_RX_ENAB(wlc->pub));

	/**
	 * XXX: Basic HE MCS NSS set
	 * Implementation needed. intersection of all HE capable STAs associated to this
	 * AP and all MBSS APs of the mcs nss capabilities. Odly it doesnt define the tx or rx
	 * rate. It doesnt seem to be of much importance for AP. Initially setting to AP cap
	 * should suffice. Can be updated later for higher complexity. Needs to be stored in HEI
	 * and upated by handlers for station add/remove, so it wont have to be calculated every
	 * beacon.
	 */
	/**
	 * During PF1 Intel found our default not acceptable. We used he_bw80_tx_mcs_nss, and they
	 * saw that as minimum and couldnt fullfill that. So for now we set it to really bare
	 * minimum, which is NSS1*MCS7
	 */
	mcs_nss =
		(HE_CAP_MAX_MCS_0_7 << HE_CAP_MAX_MCS_NSS_GET_SS_IDX(1)) |
		(HE_CAP_MAX_MCS_NONE << HE_CAP_MAX_MCS_NSS_GET_SS_IDX(2)) |
		(HE_CAP_MAX_MCS_NONE << HE_CAP_MAX_MCS_NSS_GET_SS_IDX(3)) |
		(HE_CAP_MAX_MCS_NONE << HE_CAP_MAX_MCS_NSS_GET_SS_IDX(4)) |
		(HE_CAP_MAX_MCS_NONE << HE_CAP_MAX_MCS_NSS_GET_SS_IDX(5)) |
		(HE_CAP_MAX_MCS_NONE << HE_CAP_MAX_MCS_NSS_GET_SS_IDX(6)) |
		(HE_CAP_MAX_MCS_NONE << HE_CAP_MAX_MCS_NSS_GET_SS_IDX(7)) |
		(HE_CAP_MAX_MCS_NONE << HE_CAP_MAX_MCS_NSS_GET_SS_IDX(8));
	op->basic_mcs_nss_set = htol16(mcs_nss);

	if (BSSCFG_AP(cfg) && !MBSSID_ENAB(wlc->pub, wlc->band->bandtype) &&
		MBSS_ENAB(wlc->pub) && (AP_BSS_UP_COUNT(cfg->wlc) > 1)) {
		he_op_cohost_bss_t *cobss_exp = (he_op_cohost_bss_t*) ((uint8*)op + tlv_byte_sz);
		uint max_mbss = WLC_MAX_AP_BSS(wlc->pub->corerev);
		setbits((uint8 *)&op->parms, sizeof(op->parms), HE_OP_CO_LOCATED_BSS_IDX,
			HE_OP_CO_LOCATED_BSS_FSZ, 1);
		*cobss_exp[0] = (max_mbss > 8) ? 4 /* 2^4=16 */ :
			(max_mbss > 4) ? 3 /* 2^3=8 */ : 2 /* 2^2=4 */;
		op->len += HE_OP_COHOST_BSS_SIZE;
		tlv_byte_sz += HE_OP_COHOST_BSS_SIZE;
	}

	if (CHSPEC_IS6G(chspec)) {
		he_op_6ghz_op_info_t *_6ghz_op_info = (he_op_6ghz_op_info_t*)
							((uint8*)op + tlv_byte_sz);
		enum he_6ghz_op_ctrl_bw bw;
		uint8 ccfs0 = 0, ccfs1 = 0;

		setbit((uint8 *)&op->parms, HE_OP_6G_OP_IE_PRESENT_IDX);
		_6ghz_op_info->pri_chan = wf_chspec_primary20_chan(chspec);

		switch (CHSPEC_BW(chspec)) {
		case WL_CHANSPEC_BW_160:
			ccfs0 = wf_chspec_primary80_channel(chspec);
			ccfs1 = CHSPEC_CHANNEL(chspec);
			bw = HE_6GHZ_OP_CTRL_BW8080_160;
			break;
		case WL_CHANSPEC_BW_8080:
			ccfs0 = wf_chspec_primary80_channel(chspec);
			ccfs1 = wf_chspec_secondary80_channel(chspec);
			bw = HE_6GHZ_OP_CTRL_BW8080_160;
			break;
		case WL_CHANSPEC_BW_80:
			bw = HE_6GHZ_OP_CTRL_BW80;
			break;
		case WL_CHANSPEC_BW_40:
			bw = HE_6GHZ_OP_CTRL_BW40;
			break;
		case WL_CHANSPEC_BW_20:
		default:
			bw = HE_6GHZ_OP_CTRL_BW20;
			break;
		}

		setbits((uint8 *)&_6ghz_op_info->control, sizeof(_6ghz_op_info->control),
		        HE_6GHZ_OP_CTRL_CHANN_WIDTH_IDX, HE_6GHZ_OP_CTRL_CHANN_WIDTH_FSZ,
			bw);

#ifdef BAND6G
		regulatory_info = wlc_update_regulatory_info(wlc, chspec);
		if (regulatory_info >= 0) {
			_6ghz_op_info->control |= regulatory_info << 3;
		} else {
			WL_ERROR(("wl%d: %s: Error fetching regulatory info \n",
					wlc->pub->unit, __FUNCTION__));
		}
#endif /* BAND6G */
		if ((wlc->lpi_mode == AUTO && wlc->stf->psd_limit_indicator) ||
			(wlc->lpi_mode == ON)) {
			setbits((uint8 *)&_6ghz_op_info->control, sizeof(_6ghz_op_info->control),
			HE_6GHZ_OP_CTRL_DUP_BCN_IDX, HE_6GHZ_OP_CTRL_DUP_BCN_FSZ,
				TRUE);
		}

		if (ccfs0 == 0) {
			ccfs0 = CHSPEC_CHANNEL(chspec);
		}
		_6ghz_op_info->cntr_fq_seg0 = ccfs0;
		_6ghz_op_info->cntr_fq_seg1 = ccfs1;

		op->len += HE_OP_6GHZ_OP_INFO_SIZE;
	}

	return BCME_OK;
} /* wlc_he_write_op_ie */

#ifdef STA

/**
 * Reads the (optional) 6ghz operation information field contained in the HE operation IE
 *
 * @param[in]  _6ghz_op_info   Pointer to 'raw' received data
 * @param[out] bi              Information distilled from IE is written to Bss Information
 */
static void
wlc_he_read_6ghz_op_ie(he_op_6ghz_op_info_t *_6ghz_op_info, wlc_bss_info_t *bi)
{
	enum he_6ghz_op_ctrl_bw bw;
	uint8 ctl_chan, center_chan, ccfs0, ccfs1;
	chanspec_bw_t chspec_bw;
	chanspec_t chanspec;
	uint16 bss_bw_flag = 0;
	uint16 bss_bw_flag2 = 0;

	bw = getbits((uint8 *)&_6ghz_op_info->control, sizeof(_6ghz_op_info->control),
	             HE_6GHZ_OP_CTRL_CHANN_WIDTH_IDX, HE_6GHZ_OP_CTRL_CHANN_WIDTH_FSZ);

	ctl_chan = _6ghz_op_info->pri_chan;
	center_chan = _6ghz_op_info->cntr_fq_seg0;

	switch (bw) {
	default:
		/* invalid bw so assume 20MHz */
		center_chan = ctl_chan;
		/* fall-through */
	case HE_6GHZ_OP_CTRL_BW20:
		chspec_bw = WL_CHANSPEC_BW_20;
		break;
	case HE_6GHZ_OP_CTRL_BW40:
		chspec_bw = WL_CHANSPEC_BW_40;
		bss_bw_flag = WLC_BSS_40MHZ;
		break;
	case HE_6GHZ_OP_CTRL_BW80:
		chspec_bw = WL_CHANSPEC_BW_80;
		bss_bw_flag2 = WLC_BSS2_80MHZ;
		break;
	case HE_6GHZ_OP_CTRL_BW8080_160:
		ccfs0 = center_chan;
		ccfs1 = _6ghz_op_info->cntr_fq_seg1;
		if (ABS((int)(ccfs1 - ccfs0)) == CH_40MHZ_APART) {
			chspec_bw = WL_CHANSPEC_BW_160;
			bss_bw_flag2 = WLC_BSS2_160MHZ;
			center_chan = ccfs1;
		} else {
			chspec_bw = WL_CHANSPEC_BW_8080;
			bss_bw_flag2 = WLC_BSS2_8080MHZ;
		}
		break;
	}

	if (chspec_bw == WL_CHANSPEC_BW_8080)
		chanspec = wf_create_8080MHz_chspec(ctl_chan, ccfs0, ccfs1, WL_CHANSPEC_BAND_6G);
	else
		chanspec = wf_create_chspec(ctl_chan, center_chan, chspec_bw, WL_CHANSPEC_BAND_6G);

	if (chanspec != INVCHANSPEC) {
		bi->chanspec = chanspec;
		bi->flags |= bss_bw_flag;
		bi->flags2 |= bss_bw_flag2;
	}
} /* wlc_he_read_6ghz_op_ie */

/** The HE operation IE was extended in a later rev of the .11ax standard with optional fields */
static void
wlc_he_op_ie_read_optional_fields(wlc_info_t *wlc, he_op_ie_t *op_ie, wlc_bss_info_t *bi)
{
	uchar *p = (uchar*)op_ie;              /**< to facilitate pointer arithmetic */
	/** TLV size [bytes], for optional field parsing */
	uint tlv_byte_sz = HE_OP_IE_MIN_LEN + TLV_HDR_LEN;

	if (getbits((uint8 *)&op_ie->parms, sizeof(op_ie->parms),
		HE_OP_VHT_OP_INFO_PRESENT_IDX, HE_OP_VHT_OP_INFO_PRESENT_FSZ) == 1) {
		if (tlv_byte_sz + sizeof(he_op_vht_op_info_t) > op_ie->len + TLV_HDR_LEN) {
			WL_HE_INFO(("wl%d: %s: VHT_OP_INFO: Invalid OP IE len %d\n",
			           wlc->pub->unit, __FUNCTION__, op_ie->len));
			return;
		}

		tlv_byte_sz += sizeof(he_op_vht_op_info_t);
	}

	if (getbits((uint8 *)&op_ie->parms, sizeof(op_ie->parms),
		HE_OP_CO_LOCATED_BSS_IDX, HE_OP_CO_LOCATED_BSS_FSZ) == 1) {
		if (tlv_byte_sz + sizeof(he_op_cohost_bss_t) > op_ie->len + TLV_HDR_LEN) {
			WL_HE_INFO(("wl%d: %s: CO_LOCATED: Invalid OP IE len %d\n",
			           wlc->pub->unit, __FUNCTION__, op_ie->len));
			return;
		}

		tlv_byte_sz += sizeof(he_op_cohost_bss_t);
	}

	if (getbits((uint8 *)&op_ie->parms, sizeof(op_ie->parms),
		HE_OP_6G_OP_IE_PRESENT_IDX, HE_OP_6G_OP_IE_PRESENT_FSZ) == 1) {
		if (tlv_byte_sz + sizeof(he_op_6ghz_op_info_t) > op_ie->len + TLV_HDR_LEN) {
			WL_HE_INFO(("wl%d: %s: 6G_OP: Invalid OP IE len %d\n",
			           wlc->pub->unit, __FUNCTION__, op_ie->len));
			return;
		}

		wlc_he_read_6ghz_op_ie((he_op_6ghz_op_info_t*)(p + tlv_byte_sz), bi);
		tlv_byte_sz += sizeof(he_op_6ghz_op_info_t);
	}

	if (tlv_byte_sz != op_ie->len + TLV_HDR_LEN) {
		WL_HE_INFO(("wl%d: %s: remaining bytes %d/%d\n",
		            wlc->pub->unit, __FUNCTION__, tlv_byte_sz, op_ie->len));
	}
} /* wlc_he_op_ie_read_optional_fields */

#endif /* STA */

/**
 * This function is e.g. called by wlc_rx.c on beacon reception. It copies the TLV into a
 * caller-provided structure and calls other necessary handling (color collision)
 *
 * @param[in]  hei
 * @param[in]  tlvs         src
 * @param[in]  tlvs_len
 * @param[out] op_ie        dst
 * @return     pointer to op_ie
 */
he_op_ie_t *
wlc_he_process_op_ie(wlc_he_info_t *hei, wlc_bsscfg_t *bsscfg, uint8 *tlvs, int tlvs_len,
	he_op_ie_t* op_ie)
{
	bcm_tlv_t *op_ie_tlv;

	BCM_REFERENCE(hei);

	op_ie_tlv = bcm_find_tlv_ext(tlvs, tlvs_len, DOT11_MNG_ID_EXT_ID, EXT_MNG_HE_OP_ID);
	if (op_ie_tlv) {
		if (op_ie_tlv->len >= HE_OP_IE_MIN_LEN && op_ie_tlv->len <= HE_OP_IE_MAX_LEN) {
			bcopy(op_ie_tlv, op_ie, op_ie_tlv->len + TLV_HDR_LEN);
			wlc_he_color_collision_process_beacon_probe(hei, bsscfg, op_ie);
			return op_ie;
		} else {
			WL_HE_INFO(("wl%d: %s: std len %d does not match %d\n",
			               hei->wlc->pub->unit, __FUNCTION__, op_ie_tlv->len,
			               HE_OP_IE_MIN_LEN));
		}
	}

	return NULL;
}

/**
 * OBSS beacon's extendeted capability element processing routine. This function is called when
 * an OBSS beacon is received. Check if all the conditions in  26.5.3.2 are met,
 * then set bit 8 in M_HOST_FLAGS6.
 * When this bit is set ucode will discard HE trigger frames with 26 tone RU alloation.
 *
 * @param hei		handle to wlc_he_info_t context
 * @param tlvs		pointer to extended capability element in tlv format.
 * @param tlvs_len	length of the extended capability element in tlv format.
 * @return		none
 */

void
wlc_he_process_obssbcn_extcap_elm(wlc_he_info_t *hei, uint8 *tlvs, int tlvs_len)
{
	uint16 val;
	wlc_info_t *wlc = hei->wlc;
	dot11_extcap_ie_t *extcap_ie_tlv;

	if ((!BSSCFG_STA(wlc->primary_bsscfg) || !HE_ULMU_STA_ENAB(wlc->pub))) {
		return;
	}

	extcap_ie_tlv = (dot11_extcap_ie_t *)tlvs;
	if (extcap_ie_tlv && (tlvs_len != 0)) {
		dot11_extcap_t *cap = (dot11_extcap_t*)extcap_ie_tlv->cap;
		if (extcap_ie_tlv->len >= DOT11_EXTCAP_LEN_OBSS_NBWRU_TOLERANCE) {
			if (isset(cap->extcap, DOT11_EXT_CAP_OBSS_NBWRU_TOLERANCE)) {
				return;
			}
		}
	}

	hei->nbwru.ref_timestamp = wlc->pub->now;
	wlc->hei->nbwru.discard_nbwru_tf = TRUE;
	if (!wlc->hw->clk) {
		return;
	}
	val = wlc_read_shm(wlc, M_HOST_FLAGS6((wlc->hw)));
	wlc_write_shm(wlc, M_HOST_FLAGS6((wlc->hw)),
			val | MHF6_AVOID_HETB_RU26);
#if defined(BCMDBG)
		WL_HE_INFO(("wl%d: %s: Setting MHF6_AVOID_HETB_RU26\n",
			wlc->pub->unit, __FUNCTION__));
#endif
	return;
}
/**
 * Called in a non-scanning scenario, on reception of a beacon, assoc resp or reassoc resp
 * containing a DOT11_MNG_HE_OP_ID element.
 */
static int
wlc_he_parse_op_ie(void *ctx, wlc_iem_parse_data_t *data)
{
#ifdef STA
	wlc_he_info_t *hei = ctx;
	scb_t *scb;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	bss_he_info_t *bhei;
	he_op_ie_t *op_ie;
	uint16 he_RTSThresh;
	uint8 bsscolor;
	bool bsscolor_disable;
	bool trigger_ratelinkmem;
	uint8 pe_duration;

	if (!HE_ENAB(wlc->pub)) {
		return BCME_OK;
	}

	if (data->ie == NULL) {
		return BCME_OK;
	}

	scb = wlc_iem_parse_get_assoc_bcn_scb(data);
	if (scb == NULL)
		return BCME_OK;

	if (!SCB_HE_CAP(scb)) {
		return BCME_OK;
	}

	cfg = SCB_BSSCFG(scb);
	bhei = BSS_HE_INFO(hei, cfg);
	op_ie = (he_op_ie_t *)data->ie;

	if (op_ie->len < HE_OP_IE_MIN_LEN) {
		WL_HE_INFO(("wl%d: %s: Invalid OP IE len %d\n", wlc->pub->unit, __FUNCTION__,
			op_ie->len));
		return BCME_OK;
	}

	/* bss color change should only happen for initial parse, otherwise AP needs to follow
	 * special procedure. So for now if color changes, just accept it and program it to PHY
	 */
	bsscolor = (uint8)getbits((uint8 *)&op_ie->color, sizeof(op_ie->color), HE_OP_BSS_COLOR_IDX,
		HE_OP_BSS_COLOR_FSZ);
	/* Disable BSScolor indication */
	bsscolor_disable = (isset((uint8 *)&op_ie->color, HE_OP_BSS_COLOR_DISABLED_IDX)) ? TRUE :
		FALSE;

	/* Dealing with color changes is difficult. There is an color disable bit to
	 * indicate if the coloring is used at all. Problem is that this bit is set when
	 * color change happens and if the a color change IE is present as well then the
	 * color should still be used. So here is a little "trick" to deal with all this:
	 * We start with color 0 and disabled true. When disabled is recvd, then only color
	 * is updated, and ratelinkmem is triggered to consume the new color. So tx will
	 * follow the color. When enabled is recvd true then color is updated, ratelinkmem
	 * is triggered, and phy is updated. When disabled is recvd and color was enabled
	 * then we just disable color on the phy side. So basically it is not that hard. The
	 * recvd color is used for tx. Disabled/enabled is used to program color (or not) to
	 * phy.
	 */
	trigger_ratelinkmem = FALSE;
	if (bhei->bsscolor != bsscolor) {
		bhei->bsscolor = bsscolor;
		/* trigger ratelinkmem */
		trigger_ratelinkmem = TRUE;
		/* Set bhei->bsscolor_disable to !bsscolor_disable to trigger update */
		bhei->bsscolor_disable = !bsscolor_disable;
	}
	if (bhei->bsscolor_disable != bsscolor_disable) {
		bhei->bsscolor_disable = bsscolor_disable;
		wlc_he_configure_bsscolor_phy(hei, cfg);
	}

	/* Default PE Duration */
	pe_duration = (uint8)getbits((uint8 *)&op_ie->parms, sizeof(op_ie->parms),
		HE_OP_DEF_PE_DUR_IDX, HE_OP_DEF_PE_DUR_FSZ);
	if (bhei->pe_duration != pe_duration) {
		bhei->pe_duration = pe_duration;
		phy_hecap_write_pe_dur(WLC_PI(wlc), pe_duration);
	}

	/* TWT required */
	if (TWT_ENAB(wlc->pub)) {
		wlc_twt_set_twt_required(wlc->twti, cfg,
			(isset((uint8 *)&op_ie->parms, HE_OP_TWT_REQD_IDX)) ? 1 : 0);
		/* This means we have to join a BCAST TWT or create individual TWT
		 * if TWT is supported.
		 */
	}

	/* HE Duration based RTS Threshold */
	he_RTSThresh = (uint16)getbits((uint8 *)&op_ie->parms, sizeof(op_ie->parms),
		HE_OP_HE_DUR_RTS_THRESH_IDX, HE_OP_HE_DUR_RTS_THRESH_FSZ);
	if (he_RTSThresh >= HE_RTS_THRES_DISABLED) {
		/* max value means mode is disabled */
		he_RTSThresh = BCM_UINT16_MAX; /* set max duration */
	} else {
		he_RTSThresh <<= HE_RTS_DUR_THRESHOLD_32USEC_SHIFT; /* convert to usec */
	}

	/* Only a non-zero value means, change the previous threshold */
	if (he_RTSThresh && (bhei->rts_thresh != he_RTSThresh)) {
		bhei->rts_thresh = he_RTSThresh;
		trigger_ratelinkmem = TRUE;
	}

	/**
	 * Basic HE MCS-NSS set, this information is of no use at the moment. It defines the
	 * intersection of all STAs associated with AP. or intersection of all STAs in case of
	 * IBSS, latter seems rather impossible. Since intersection can only go down, and on
	 * case of removal of one the limited stas it is almost impossible to determine that
	 * intersection can be recalculated. It requires tracking of capabilities of all STAs
	 * which are part of BSS and continous calculation of intersection. IBSS not supported at
	 * this point.
	 */

	if (trigger_ratelinkmem) {
		if (RATELINKMEM_ENAB(wlc->pub)) {
			wlc_ratelinkmem_update_link_entry(wlc, scb);
		}
	}
#else
	BCM_REFERENCE(ctx);
	BCM_REFERENCE(data);
#endif /* STA */

	return BCME_OK;
} /* wlc_he_parse_op_ie */

#ifdef STA
/**
 * This function is not invoked via the IEM callback mechanism. It is used when (pre)scanning, or
 * e.g. after beacon reception, it has be determined if the BSS bandwidth needs to be changed.
 */
chanspec_t
wlc_he_chanspec(wlc_he_info_t *hei, he_op_ie_t *op_ie, chanspec_t chspec)
{
	wlc_info_t *wlc = hei->wlc;
	wlc_bss_info_t bi;

	if (op_ie->len < HE_OP_IE_MIN_LEN) {
		WL_HE_INFO(("wl%d: %s: Invalid OP IE len %d\n", wlc->pub->unit, __FUNCTION__,
				op_ie->len));
		return chspec;
	}

	bi.chanspec = 0;
	wlc_he_op_ie_read_optional_fields(wlc, op_ie, &bi);

	if (wf_chspec_valid(bi.chanspec)) {
		chspec = bi.chanspec;
	}

	return chspec;
}
#endif /* STA */

/**
 * Called during scanning on reception of beacon or probe response. Distills information from the
 * DOT11_MNG_HE_OP_ID ('d291) element.
 */
static int
wlc_he_scan_parse_op_ie(void *ctx, wlc_iem_parse_data_t *data)
{
#ifdef STA
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_iem_ft_pparm_t *ftpparm = data->pparm->ft;
	wlc_bss_info_t *bi = ftpparm->scan.result;
	he_op_ie_t *op_ie;

	if (!HE_ENAB(wlc->pub)) {
		return BCME_OK;
	}

	if (data->ie == NULL) {
		return BCME_OK;
	}

	ASSERT(ftpparm->bcn.scb != NULL);

	op_ie = (he_op_ie_t *)data->ie;

	if (op_ie->len < HE_OP_IE_MIN_LEN) {
		WL_HE_INFO(("wl%d: %s: Invalid OP IE len %d\n", wlc->pub->unit, __FUNCTION__,
			op_ie->len));
		return BCME_OK;
	}

	wlc_he_op_ie_read_optional_fields(wlc, op_ie, bi);
#else
	BCM_REFERENCE(ctx);
	BCM_REFERENCE(data);
#endif /* STA */

	return BCME_OK;
} /* wlc_he_scan_parse_op_ie */

/* ======== misc ========= */

/* is the hardware HE capable */
static bool
wlc_he_hw_cap(wlc_info_t *wlc)
{
	return WLC_HE_CAP_PHY(wlc);
}

static bool
wlc_he_handle_6g_caps_dynamics(wlc_he_info_t *hei, scb_t *scb, he_6g_band_caps_ie_t *he_6g_caps_ie)
{
	wlc_info_t *wlc = hei->wlc;
	bool mimops_enabled = FALSE;
	bool mimops_rtsmode = FALSE;
	uint8 sm_pwr_control;

#ifdef WLAMSDU_TX
	bool amsdu_enabled = AMSDU_TX_ENAB(wlc->pub);
#endif /* WLAMSDU_TX */

	ASSERT(he_6g_caps_ie);

	sm_pwr_control = getbits((uint8*)&he_6g_caps_ie->caps_info,
		sizeof(he_6g_caps_ie->caps_info),
		HE_6G_CAP_SM_PWR_SAVE_IDX, HE_6G_CAP_SM_PWR_SAVE_FSZ);

#ifdef WLAMSDU_TX
	if (amsdu_enabled) {
		uint8 max_txmpdu_len; /**< max mpdu size the remote node can receive */
		uint16 max_amsdu_tx_size = HT_MIN_AMSDU;
		uint16 caps = he_6g_caps_ie->caps_info;
		max_txmpdu_len = getbits((uint8 *)&caps, sizeof(caps),
			HE_6G_CAP_MAX_MPDU_LEN_IDX, HE_6G_CAP_MAX_MPDU_LEN_FSZ);

		switch (max_txmpdu_len) {
			case VHT_CAP_MPDU_MAX_11K:
				max_amsdu_tx_size = VHT_MAX_MPDU;
				break;
			case VHT_CAP_MPDU_MAX_8K:
				max_amsdu_tx_size = HT_MAX_AMSDU;
				break;
			case VHT_CAP_MPDU_MAX_4K:
				max_amsdu_tx_size = HT_MIN_AMSDU;
				break;
		}

		scb->flags |= SCB_AMSDUCAP;
		/* update AMSDU agg bytes */
		wlc_amsdu_tx_scb_set_max_agg_size(wlc->ami, scb, max_amsdu_tx_size);
	}
#endif /* WLAMSDU_TX */
	switch (sm_pwr_control) {
		case HT_CAP_MIMO_PS_ON :
			mimops_enabled = TRUE;
			break;
		case HT_CAP_MIMO_PS_RTS :
			mimops_enabled = TRUE;
			mimops_rtsmode = TRUE;
			break;
		case HT_CAP_MIMO_PS_OFF :
			break;
		default:
			WL_ERROR(("wl%d: %s, incorrect psmode\n",
			          wlc->pub->unit, __FUNCTION__));
			break;
	}

	return wlc_stf_mimops_on_info_received(wlc, scb, FALSE, mimops_enabled, mimops_rtsmode);
}

/**
 * update scb using the cap and op contents
 * @param[in] capie   HE capability element in network endianness
 * @param[in] opie    HE operations element in network endianness
 */
void
wlc_he_update_scb_state(wlc_he_info_t *hei, enum wlc_bandunit band, scb_t *scb, he_cap_ie_t *capie,
	he_op_ie_t *opie, he_6g_band_caps_ie_t *he_6g_caps_ie)
{
	wlc_info_t *wlc = hei->wlc;
	bool reinit_ratesel = FALSE;
	bool hecap = SCB_HE_CAP(scb);

	if ((band == WLC_BAND_6G) && (he_6g_caps_ie == NULL)) {
		capie = NULL; /* need both, force to NULL */
		scb->flags3 &= ~(SCB3_IS_160|SCB3_IS_80_80);
		scb->flags &= ~(SCB_IS40);
		reinit_ratesel = TRUE;
	}

	if (he_6g_caps_ie != NULL) {
		wlc_he_parse_6g_caps_ie(hei, scb, he_6g_caps_ie);
		if (wlc_he_handle_6g_caps_dynamics(hei, scb, he_6g_caps_ie)) {
			reinit_ratesel = TRUE;
		}
	}

	wlc_he_parse_cap_ie(hei, scb, capie);

	if (hecap && !SCB_HE_CAP(scb)) {
		/* if HE CAP got disabled, we need to reinit */
		reinit_ratesel = TRUE;
	}

	if (reinit_ratesel) {
		/* need to reinit ratesel */
		wlc_scb_ratesel_init(wlc, scb);
	}
} /* wlc_he_update_scb_state */

/**
 * A beacon was received. Update the information about the remote node that transmitted the beacon.
 */
void
wlc_he_bcn_scb_upd(wlc_he_info_t *hei, enum wlc_bandunit band, struct scb *scb, he_cap_ie_t *capie,
	he_op_ie_t *opie, he_6g_band_caps_ie_t *he_6g_caps)
{
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	wlc_bss_info_t *bss;  /**< the current bss */

	ASSERT(scb != NULL);
	cfg = SCB_BSSCFG(scb);
	ASSERT(cfg != NULL);
	bss = cfg->current_bss;

	/* Make sure to verify AP also advertises WMM IE before updating HE state */

	if ((capie != NULL) && ((band != WLC_BAND_6G) || (he_6g_caps != NULL)) && cfg->associated &&
		(bss->flags3 & WLC_BSS3_HE) && (bss->flags & WLC_BSS_WME)) {
		if (BSSCFG_IBSS(cfg) ||
			(AP_ACTIVE(wlc) && (scb != NULL) && SCB_LEGACY_WDS(scb))) {
			/* full processing required on each RX beacon for IBSS and WDS */
#ifdef WLP2P
			/* XXX some vendors don't keep SM PS mode in their beacons
			 * in sync with that in the notification action frames
			 * so in order to pass P2P Interop TP v0.39 case 7.1.7
			 * we are forced to ignore the mode in beacons.
			 * But, If peer device is from BRCM then, we should not Ignore
			 * SMPS mode in beacon.
			 */
			if (P2P_CLIENT(hei->wlc, cfg) && !SCB_IS_BRCM(scb))
				scb->flags2 |= SCB2_IGN_SMPS;
#endif
			wlc_he_update_scb_state(hei, band, scb, capie, opie, he_6g_caps);
#ifdef WLP2P
			scb->flags2 &= ~SCB2_IGN_SMPS;
#endif
		} else if (band == WLC_BAND_6G) {
			ASSERT(he_6g_caps != NULL);
			/* for STA only need to process 6GHZ specific IE each RX beacon */
			if (wlc_he_handle_6g_caps_dynamics(hei, scb, he_6g_caps)) {
				/* need to reinit ratesel */
				wlc_scb_ratesel_init(wlc, scb);
			}
		}
	} else if (SCB_HE_CAP(scb)) {
		wlc_he_update_scb_state(hei, band, scb, NULL, NULL, NULL);
	}
}

uint8
wlc_he_get_dynfrag(wlc_info_t *wlc)
{
	return wlc->hei->dynfrag;
}

uint32
wlc_he_get_peer_caps(wlc_he_info_t *hei, scb_t *scb)
{
	scb_he_t *sh = (scb_he_t *)SCB_HE(hei, scb);

	if (!sh) {
		return 0;
	} else {
		return sh->flags;
	}
}

/**
 * Update capabilities with additional caps, only to be used by packet engine.
 */
void
wlc_he_add_peer_caps(wlc_he_info_t *hei, struct scb *scb, uint32 cap)
{
	scb_he_t *sh = (scb_he_t *)SCB_HE(hei, scb);

	if (sh) {
		sh->flags |= cap;
	}
}

uint8
wlc_he_scb_get_bfr_nr(wlc_he_info_t *hei, scb_t *scb)
{
	scb_he_t *sh =
		(scb_he_t *)SCB_HE(hei, scb);
	if (!sh) {
		return 0;
	} else {
		return getbits((uint8*)&sh->phy_cap, sizeof(sh->phy_cap),
			HE_PHY_SOUND_DIM_BELOW80MHZ_IDX, HE_PHY_SOUND_DIM_BELOW80MHZ_FSZ);
	}
} /* wlc_he_scb_get_bfr_nr */

static void
wlc_he_configure_bsscolor_phy(wlc_he_info_t *hei, wlc_bsscfg_t *cfg)
{
	wlc_info_t *wlc = hei->wlc;
	bss_he_info_t *bhei;
	phy_info_t *pi = WLC_PI(wlc);
	wlc_bsscolor_t bsscolor;
	uint16 bsscolor_shm;

	if (!wlc->pub->up) {
		return;
	}

	if (cfg == NULL) {
		bsscolor.index = HE_BSSCOLOR_APIDX;
		/* Disable BSSColor filtering */
		bsscolor.disable = TRUE;
		/* Use some valid number for color */
		bsscolor.color = 1;
	} else {
		if (BSSCFG_AP(cfg)) {
			if ((hei->bsscolor_disable) || (hei->update_bsscolor_counter > 1)) {
				bsscolor.disable = TRUE;
			} else {
				bsscolor.disable = FALSE;
			}
			bsscolor.color = hei->bsscolor;
			bsscolor.index = HE_BSSCOLOR_APIDX;
			bsscolor_shm = wlc_read_shm(wlc, M_MY_BSSCOLOR(wlc->hw));

			/* clear AP BSS color value and disable status */
			bsscolor_shm &= ~(D11_HE_AP_BSSCOLOR_MASK | D11_HE_AP_BSSCOLOR_DIS_MASK);

			/* update AP BSS color value */
			bsscolor_shm |= (bsscolor.color << D11_HE_AP_BSSCOLOR_SHIFT);

			/* update AP BSS color disable status
			 * uCode calculates TXOP (HE-SIG-A2) when operating on 6GHz and when
			 * BSS Color Disable is set false.
			 */
			bsscolor_shm |= (bsscolor.disable << D11_HE_AP_BSSCOLOR_DIS_SHIFT);

			wlc_write_shm(wlc, M_MY_BSSCOLOR(wlc->hw), bsscolor_shm);

			WL_HE_INFO(("wl%d: %s: signal uCode BSS Color %d Disable %d countdown %d\n",
					wlc->pub->unit, __FUNCTION__,
					bsscolor.color, bsscolor.disable,
					hei->update_bsscolor_counter));
		} else {
			bhei = BSS_HE_INFO(hei, cfg);
			/* Disable BSSColor filtering ? */
			bsscolor.disable = bhei->bsscolor_disable;
			bsscolor.color = bhei->bsscolor;
			bsscolor.index = HE_BSSCOLOR_STAIDX;
			/* Write AID as STAID */
			bsscolor.staid[STAID0] = HE_STAID_BSS_BCAST;
			bsscolor.staid[STAID1] = HE_STAID_MBSS_BCAST;
			bsscolor.staid[STAID2] = HE_STAID_BSS_BCAST;
			bsscolor.staid[STAID3] = (cfg->AID & HE_STAID_MASK);
		}
	}

	phy_hecap_write_bsscolor(pi, &bsscolor);
}

void
wlc_he_ap_tbtt(wlc_he_info_t *hei)
{
	if (hei->update_bsscolor_counter == 0) {
		return;
	}
	hei->update_bsscolor_counter--;
	if (hei->update_bsscolor_counter == 1) {
		hei->bsscolor = hei->new_bsscolor;
	}
	wlc_update_beacon(hei->wlc);
	wlc_update_probe_resp(hei->wlc, TRUE);
}

static uint wlc_he_calc_muedca_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_he_info_t *hei = ctx;

	if (!data->cbparm->he) {
		return 0;
	}
	if (!BSSCFG_AP(data->cfg)) {
		return 0;
	}
	if (hei->muedca_blocked) {
		return 0;
	}

	return sizeof(he_muedca_ie_t);
}

static int wlc_he_write_muedca_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_he_info_t *hei = ctx;
	wlc_bsscfg_t *cfg = data->cfg;
	bss_he_info_t *bhei = BSS_HE_INFO(hei, cfg);
	he_muedca_ie_t *muedca_ie;
	uint8 aci;

	if (hei->muedca_blocked) {
		return BCME_OK;
	}

	ASSERT(BSSCFG_AP(cfg));

	muedca_ie = (he_muedca_ie_t *)data->buf;

	muedca_ie->id = DOT11_MNG_ID_EXT_ID;
	muedca_ie->id_ext = EXT_MNG_MU_EDCA_ID;
	muedca_ie->len = sizeof(*muedca_ie) - TLV_HDR_LEN;

	muedca_ie->mu_qos_info = cfg->edca_update_count;

	for (aci = AC_BE; aci < AC_COUNT; aci++) {
		muedca_ie->ac_param[aci].aci_aifsn = bhei->sta_mu_edca[aci].aci_aifsn;
		muedca_ie->ac_param[aci].ecw_min_max = bhei->sta_mu_edca[aci].ecw_min_max;
		muedca_ie->ac_param[aci].muedca_timer = bhei->sta_mu_edca[aci].muedca_timer;
	}

	return BCME_OK;
}

/* color IE functions. color IE is added when the bss color is changing. A down counter in the IE
 * is to be updated for every beacond, and once reached 0 the color is to be 'used' and on the next
 * beacon the color IE Is to be removed. This is from the specification, explaining difference of
 * counter values between Beacons and non - Beacons:
 * A value of 0 indicates that the switch occurs at the current TBTT if the element is carried in a
 * Beacon frame or the next TBTT following the frame that carried the element if the frame is not a
 * Beacon frame.
 * The internal counter used folows a slightly different usage: 0 Means no update is going on at
 * all. 1 means, last beacon with color IE and and on next beacon the color IE will be removed.
 * On 1 also the color is updated. Other values means that the color change is approaching.
 * The reason for this method is that the color IE needs removal. So on the beacon where the color
 * change happens the color IE should be sent out with counter value 0, on the next beacon the
 * color IE should not be there anymore. To make this easy for software we use an off by one
 * downcounter.
 */
static uint
wlc_he_calc_color_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_he_info_t *hei = ctx;

	if (!data->cbparm->he)
		return 0;
	if ((data->ft == FC_BEACON) && (hei->update_bsscolor_counter == 0))
		return 0;
	if ((data->ft != FC_BEACON) && (hei->update_bsscolor_counter < 2))
		return 0;

	return sizeof(he_colorchange_ie_t);
}

static int
wlc_he_write_color_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_he_info_t *hei = ctx;
	he_colorchange_ie_t *colorchange_ie;

	colorchange_ie = (he_colorchange_ie_t *)data->buf;

	colorchange_ie->id = DOT11_MNG_ID_EXT_ID;
	colorchange_ie->id_ext = EXT_MNG_COLOR_CHANGE_ID;
	colorchange_ie->len = sizeof(*colorchange_ie) - TLV_HDR_LEN;
	/* If this is for Beacon frame then use counter - 1, otherwise use counter - 2 */
	if (data->ft == FC_BEACON) {
		colorchange_ie->countdown = hei->update_bsscolor_counter - 1;
	} else {
		colorchange_ie->countdown = hei->update_bsscolor_counter - 2;
	}
	colorchange_ie->newcolor = hei->new_bsscolor;

	return BCME_OK;
}

/**
 * Spatial reuse IE.
 */
static uint wlc_he_calc_sr_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_he_info_t *hei = ctx;
	uint size;

	if (!data->cbparm->he) {
		return 0;
	}

	if (!hei->sr_enabled) {
		return 0;
	}

	size = sizeof(he_spatial_reuse_ie_t);
	if (isset(&hei->sr_control, HE_SR_CONTROL_NON_SRG_OFFSET_PRESENT_IDX)) {
		size += sizeof(he_non_srg_t);
	}
	if (isset(&hei->sr_control, HE_SR_CONTROL_SRG_INFORMATION_PRESENT_IDX)) {
		size += sizeof(he_srg_t);
	}

	return size;
}

static int wlc_he_write_sr_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_he_info_t *hei = ctx;
	he_spatial_reuse_ie_t *spatial_reuse_ie;
	uint8 *buf;

	if (!hei->sr_enabled) {
		return BCME_OK;
	}

	spatial_reuse_ie = (he_spatial_reuse_ie_t *)data->buf;

	spatial_reuse_ie->id = DOT11_MNG_ID_EXT_ID;
	spatial_reuse_ie->id_ext = EXT_MNG_SRPS_ID;
	spatial_reuse_ie->len = sizeof(*spatial_reuse_ie) - TLV_HDR_LEN;
	spatial_reuse_ie->sr_control = hei->sr_control;

	buf = (uint8 *)&spatial_reuse_ie[1];
	if (isset(&hei->sr_control, HE_SR_CONTROL_NON_SRG_OFFSET_PRESENT_IDX)) {
		memcpy(buf, &hei->non_srg, sizeof(he_non_srg_t));
		spatial_reuse_ie->len += sizeof(he_non_srg_t);
		buf += sizeof(he_non_srg_t);
	}
	if (isset(&hei->sr_control, HE_SR_CONTROL_SRG_INFORMATION_PRESENT_IDX)) {
		memcpy(buf, &hei->srg, sizeof(he_srg_t));
		spatial_reuse_ie->len += sizeof(he_srg_t);
	}

	return BCME_OK;

}

static int
wlc_he_parse_muedca_ie(void *ctx, wlc_iem_parse_data_t *data)
{
#ifdef STA
	wlc_he_info_t *hei = ctx;
	scb_t *scb = NULL;
	wlc_bsscfg_t *bsscfg;
	bss_he_info_t *bhei;
	he_muedca_ie_t *muedca;
	uint8 edca_update_count;
	uint8 aci;
	wlc_info_t *wlc = hei->wlc;
	uint32 muedca_timer_us;
	uint8 muedca_timer_max = 0;
	uint8 ac_mask = 0;
	edcf_acparam_t *acp;
	uint16  wme_vi_aifs, muedca_vi_aifs;

	if (data->ie == NULL) {
		return BCME_OK;
	}

	scb = wlc_iem_parse_get_assoc_bcn_scb(data);
	if (scb == NULL)
		return BCME_OK;

	if (!SCB_HE_CAP(scb)) {
		return BCME_OK;
	}

	bsscfg = scb->bsscfg;
	bhei = BSS_HE_INFO(hei, bsscfg);
	muedca = (he_muedca_ie_t *)data->ie;
	edca_update_count = (muedca->mu_qos_info & WME_QI_AP_COUNT_MASK) >> WME_QI_AP_COUNT_SHIFT;
	if (edca_update_count == bhei->edca_update_count) {
		return BCME_OK;
	}

	bhei->edca_update_count = edca_update_count;

	ac_mask = wlc_bmac_read_shm(wlc->hw, M_ULTX_ACMASK(wlc));
	/* Now inform Ucode about the new MU EDCA params */
	for (aci = AC_BE; aci < AC_COUNT; aci++) {
		if  (!(bhei->sta_mu_edca[aci].aci_aifsn & EDCF_AIFSN_MASK) &&
			((muedca->ac_param[aci].aci_aifsn & EDCF_AIFSN_MASK) != 0))
			ac_mask = ac_mask & ~(1 << wlc_ac_aifs_update[aci]);
		else if (((bhei->sta_mu_edca[aci].aci_aifsn & EDCF_AIFSN_MASK) != 0) &&
			!(muedca->ac_param[aci].aci_aifsn & EDCF_AIFSN_MASK))
			ac_mask = ac_mask | (1 << wlc_ac_aifs_update[aci]);

		bhei->sta_mu_edca[aci].aci_aifsn = muedca->ac_param[aci].aci_aifsn;
		bhei->sta_mu_edca[aci].ecw_min_max = muedca->ac_param[aci].ecw_min_max;
		bhei->sta_mu_edca[aci].muedca_timer = muedca->ac_param[aci].muedca_timer;
		if (muedca->ac_param[aci].muedca_timer > muedca_timer_max)
			muedca_timer_max = muedca->ac_param[aci].muedca_timer;
	}

	/* WAR to pass cert test 5.64 */
	acp = &bsscfg->wme->wme_param_ie.acparam[AC_VI];
	wme_vi_aifs = acp->ACI & EDCF_AIFSN_MASK;
	muedca_vi_aifs = bhei->sta_mu_edca[AC_VI].aci_aifsn & EDCF_AIFSN_MASK;
	if (BSSCFG_STA(bsscfg) && (wme_vi_aifs == 2) && (muedca_vi_aifs == 2)) {
		acp->ACI |= 3;
		wlc_edcf_acp_apply(wlc, bsscfg, TRUE);
	}

	wlc_bmac_write_shm(wlc->hw, M_ULTX_ACMASK(wlc), ac_mask);
	muedca_timer_us = (uint32)(muedca_timer_max * HE_MUEDCA_TIMER_UNIT * MUEDCA_TIMER_US);
	/* ucode has single value for all ACs */
	/* Suspend MAC */
	wlc_suspend_mac_and_wait(wlc);
	wlc_bmac_write_shm(wlc->hw, M_ULTX_HOLDTM_L(wlc),
		((muedca_timer_us & M_ULTX_HOLDTM_L_MASK) >> M_ULTX_HOLDTM_L_SHIFT));
	wlc_bmac_write_shm(wlc->hw, M_ULTX_HOLDTM_H(wlc),
		((muedca_timer_us & M_ULTX_HOLDTM_H_MASK) >> M_ULTX_HOLDTM_H_SHIFT));
	/* Resume MAC */
	wlc_enable_mac(wlc);

#endif /* STA */

	return BCME_OK;
}

static int
wlc_he_cmd_muedca(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	wl_he_muedca_v1_t *muedca;
	uint8 aci;
	bss_he_info_t *bhei;

	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);
	bhei = BSS_HE_INFO(hei, cfg);

	if (set) {
		muedca = (wl_he_muedca_v1_t *)params;
		if (muedca->version != WL_HE_VER_1)
			return BCME_UNSUPPORTED;

		for (aci = AC_BE; aci < AC_COUNT; aci++) {
			bhei->sta_mu_edca[aci].aci_aifsn = muedca->ac_param_sta[aci].aci_aifsn;
			bhei->sta_mu_edca[aci].ecw_min_max = muedca->ac_param_sta[aci].ecw_min_max;
			bhei->sta_mu_edca[aci].muedca_timer =
				muedca->ac_param_sta[aci].muedca_timer;
		}
		cfg->edca_update_count++;
		cfg->edca_update_count &= WME_QI_AP_COUNT_MASK;
		hei->muedca_blocked = FALSE;
		if (BSSCFG_AP(cfg)) {
			if (wlc->pub->up) {
				wlc_update_beacon(wlc);
				wlc_update_probe_resp(wlc, TRUE);
			}
		}
	} else {
		muedca = (wl_he_muedca_v1_t *)result;
		muedca->version = WL_HE_VER_1;
		muedca->length = sizeof(*muedca) - 4;

		for (aci = AC_BE; aci < AC_COUNT; aci++) {
			muedca->ac_param_sta[aci].aci_aifsn = bhei->sta_mu_edca[aci].aci_aifsn;
			muedca->ac_param_sta[aci].ecw_min_max = bhei->sta_mu_edca[aci].ecw_min_max;
			muedca->ac_param_sta[aci].muedca_timer =
				bhei->sta_mu_edca[aci].muedca_timer;
		}
		*rlen = sizeof(*muedca);
	}

	return BCME_OK;

}

/** called on './wl he dynfrag' */
static int
wlc_he_cmd_dynfrag(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);

	ASSERT(cfg != NULL);
	BCM_REFERENCE(cfg);

	if (set) {
		uint8 newfrag = *params;

		/* support dynamic fragmentation level 2 for AX chip */
		if (newfrag > HE_MAC_FRAG_ONE_PER_AMPDU) {
			return BCME_BADARG;
		}

		if (hei->dynfrag != newfrag) {
			hei->dynfrag = newfrag;
			if (BSSCFG_AP(cfg) && wlc->pub->up) {
				wlc_update_beacon(wlc);
				wlc_update_probe_resp(wlc, TRUE);
			}
		}
	} else {
		*result = hei->dynfrag;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

/** called on './wl he ppet' */
static int
wlc_he_cmd_ppet(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen, bool set,
		wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);

	ASSERT(cfg != NULL);
	BCM_REFERENCE(cfg);

	if (set) {
		uint8 newppet = *params;

		if (hei->ppet_override != newppet) {
			hei->ppet_override = newppet;
			wlc_he_fill_ppe_thresholds(hei);
			/* Update the beacons to override the new value */
			if (BSSCFG_AP(cfg) && wlc->pub->up) {
				wlc_update_beacon(wlc);
				wlc_update_probe_resp(wlc, TRUE);
			}
			/* trigger ratelinkmem for all scbs to get the override configured */
			if (RATELINKMEM_ENAB(wlc->pub)) {
				wlc_ratelinkmem_update_link_entry_all(wlc, cfg, FALSE,
					FALSE /* clr_txbf_stats=0 in mreq */);
			}
		}
	} else {
		*result = hei->ppet_override;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

/** called on './wl he axmode' */
static int
wlc_he_cmd_axmode(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen, bool set,
		wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);

	ASSERT(cfg != NULL);
	BCM_REFERENCE(cfg);

	if (set) {
		uint8 newaxmode = *params;

		if (cfg->up) {
			return BCME_NOTDOWN;
		}
		if (newaxmode != 0) {
			cfg->flags &= ~WLC_BSSCFG_HE_DISABLE;
		} else
			cfg->flags |= WLC_BSSCFG_HE_DISABLE;

	} else {
		*result = ((cfg->flags & WLC_BSSCFG_HE_DISABLE) != 0 ?
				FALSE : TRUE);
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

void
wlc_he_set_ldpc_cap(wlc_he_info_t *hei, bool enab)
{
	if (enab) {
		setbit(&hei->def_phy_cap, HE_PHY_LDPC_PYLD_IDX);
	} else {
		clrbit(&hei->def_phy_cap, HE_PHY_LDPC_PYLD_IDX);
	}
} /* wlc_he_set_ldpc_cap */

/** Compute and fill the link mem entry (Rev 128) content, called by wlc_tx_fill_link_entry. */
void
wlc_he_fill_link_entry(wlc_he_info_t *hei, wlc_bsscfg_t *cfg, scb_t *scb,
	d11linkmem_entry_t* link_entry)
{
	scb_he_t *sh;
	bss_he_info_t *bhei;
	uint32 ppet_nss;
	uint16 dynfragpara;
	uint16 heppdu160;
	uint8 omi_bw, bw;

	ASSERT(link_entry != NULL);
	ASSERT(scb != NULL);
	ASSERT(cfg != NULL);
	ASSERT(SCB_HE_CAP(scb));

	bhei = BSS_HE_INFO(hei, cfg);
	if (!bhei) {
		return;
	}
	sh = SCB_HE(hei, scb);
	/* sh might already freed in wlc_he_scb_deinit */
	if (!sh) {
		return;
	}

	link_entry->RtsDurThresh = htol16(bhei->rts_thresh);

	if (hei->ppet_override == WL_HE_PPET_AUTO && sh->ppet_valid) {
		ppet_nss = sh->min_ppe_nss;
	} else {
		/* If there is a ppet override configured then we apply it here */
		uint override = (hei->ppet_override == WL_HE_PPET_AUTO) ?
			/* no valid peer info, pick own limitation as default */
			hei->ppet : hei->ppet_override;
		if (override == WL_HE_PPET_0US) {
			ppet_nss = HE_EXPAND_PPET_ALL_RU((HE_CONST_IDX_NONE <<
				HE_PPE_THRESH_NSS_RU_FSZ) | HE_CONST_IDX_NONE);
		} else if (override == WL_HE_PPET_8US) {
			ppet_nss = HE_EXPAND_PPET_ALL_RU((HE_CONST_IDX_BPSK <<
				HE_PPE_THRESH_NSS_RU_FSZ) | HE_CONST_IDX_NONE);
		} else { /* 16 us */
			ppet_nss = HE_EXPAND_PPET_ALL_RU((HE_CONST_IDX_NONE <<
				HE_PPE_THRESH_NSS_RU_FSZ) | HE_CONST_IDX_BPSK);
		}
	}
	link_entry->PPET_AmpMinDur &= ~D11_REV128_PPETX_MASK;
	link_entry->PPET_AmpMinDur |= htol32(ppet_nss);
	 /* no Multi-TID yet */
	link_entry->MultiTIDAggBitmap = 0;
	link_entry->MultiTIDAggNum = sh->multi_tid_agg_num;

	link_entry->BssColor_valid |= bhei->bsscolor & D11_REV128_COLOR_MASK;

	link_entry->OMI = sh->omi_lm;
	/* make sure omi_bw within chanspec bw */
	bw = wlc_scb_ratesel_get_link_bw(hei->wlc, scb);
	if (bw == BW_160MHZ) {
		omi_bw = DOT11_OPER_MODE_160MHZ;
	} else if (bw == BW_80MHZ) {
		omi_bw = DOT11_OPER_MODE_80MHZ;
	} else if (bw == BW_40MHZ) {
		omi_bw = DOT11_OPER_MODE_40MHZ;
	} else {
		omi_bw = DOT11_OPER_MODE_20MHZ;
	}
	link_entry->OMI &= ~HTC_OM_CONTROL_CHANNEL_WIDTH_MASK;
	link_entry->OMI |= ((omi_bw << HTC_OM_CONTROL_CHANNEL_WIDTH_OFFSET)
		& HTC_OM_CONTROL_CHANNEL_WIDTH_MASK);
	sh->omi_lm = link_entry->OMI;

	link_entry->OMI |= ((sh->flags & SCB_HE_DL_QAM1024_LT242 ? 1 : 0) <<
		C_LTX_OMI_DLQ1024_NBIT);
	link_entry->OMI |= ((sh->flags & SCB_HE_UL_QAM1024_LT242 ? 1 : 0) <<
		C_LTX_OMI_ULQ1024_NBIT);

	link_entry->OMI |= ((((sh->flags & SCB_HE_BSR_CAPABLE) && (hei->bsr_supported)) ? 1 : 0) <<
		C_LTX_OMI_BSR_NBIT);
	/* Set ldpc cap bit */
	if (hei->wlc->stf->ldpc != OFF &&
		((SCB_HE_CAP(scb) && SCB_HE_LDPC_CAP(hei, scb)) ||
		(SCB_VHT_CAP(scb) && SCB_VHT_LDPC_CAP(hei->wlc->vhti, scb))))
		link_entry->OMI |= 1 << C_LTX_OMI_LDPC_NBIT;
	/* Initiate the OMISTAT field using the lower 12 bits of OMI filed */
	link_entry->OMIStat = htol16(link_entry->OMI & D11_LTX_OMISTAT_MASK);
	link_entry->OMI = htol16(link_entry->OMI);
	link_entry->UmrxStats0 = 0;

	/* Dynamic Fragment configuration */
	if (hei->dynfrag != HE_MAC_FRAG_NOSUPPORT) {
		link_entry->StaID_IsAP |= htol16(D11_REV128_STAID_DEFRAG);
	}
	if (SCB_HE_FRAGRX_CAP(hei, scb) &&
		(HE_SU_FRAGTX_ENAB(hei->wlc->pub) || HE_MU_FRAGTX_ENAB(hei->wlc->pub))) {
		// the min length should at least be 1
		dynfragpara = (1 << C_LTX_FRAGTXEN_NBIT);
		switch (getbits((uint8 *)&sh->mac_cap, sizeof(sh->mac_cap),
			HE_MAC_MIN_FRAG_SIZE_IDX, HE_MAC_MIN_FRAG_SIZE_FSZ)) {

			case 1 :
				dynfragpara |= 128;
				break;
			case 2 :
				dynfragpara |= 256;
				break;
			case 3 :
				dynfragpara |= 512;
				break;
			default :
				dynfragpara |= 1;
				break;
		}
		link_entry->fragTx_minru |= htol16(dynfragpara);
	}
	/* AP BW == 160 && (STA BW == 160 || (STA BW == 80 && 80IN160)) */
	heppdu160 = CHSPEC_IS_BW_160_WIDE(cfg->current_bss->chanspec) &&
		((HTC_OM_CONTROL_CHANNEL_WIDTH(sh->omi_lm) == DOT11_OPER_MODE_160MHZ) ||
		((HTC_OM_CONTROL_CHANNEL_WIDTH(sh->omi_lm) == DOT11_OPER_MODE_80MHZ) &&
		(sh->flags & SCB_HE_80IN160))) ? 1: 0;
	link_entry->BFIConfig1 |= heppdu160 << C_LTX_PPDU160_NBIT;
	link_entry->BFIConfig1 |= wlc_stf_is_scb_dynamic_smps(hei->wlc, scb) << C_LTX_DYSMPS_NBIT;
	link_entry->BFIConfig1 |= ((sh->flags & SCB_HE_UL2x996) ? 1 : 0) << C_LTX_UL2x996_NBIT;
}

#ifdef HERTDBG
static void wlc_he_print_rateset(wlc_he_rateset_t *he_rateset)
{
	/* Print the MCS cap map for all bandwidths */
	WL_PRINT(("HE-Rate rateset:\n"));
	WL_PRINT(("\t80Mhz     Tx:%04x Rx:%04x\n", he_rateset->bw80_tx_mcs_nss,
		he_rateset->bw80_rx_mcs_nss));
	WL_PRINT(("\t80p80Mhz  Tx:%04x Rx:%04x\n", he_rateset->bw80p80_tx_mcs_nss,
		he_rateset->bw80p80_rx_mcs_nss));
	WL_PRINT(("\t160Mhz    Tx:%04x Rx:%04x\n", he_rateset->bw160_tx_mcs_nss,
		he_rateset->bw160_rx_mcs_nss));
}
#endif /* HERTDBG */

uint16
wlc_he_get_scb_omi(wlc_he_info_t *hei, struct scb *scb)
{
	scb_he_t *he_scb;

	he_scb = SCB_HE(hei, scb);
	if (he_scb) {
		return he_scb->omi_lm;
	} else {
		return 0;
	}
}

/* Function to check if a given scb can do ul ofdma data */
bool
wlc_he_get_ulmu_allow(wlc_he_info_t *hei, struct scb *scb)
{
	scb_he_t *he_scb;
	bool ret = FALSE;

	he_scb = SCB_HE(hei, scb);
	if (he_scb && SCB_HE_CAP(scb) &&
		(!HTC_OM_CONTROL_UL_MU_DISABLE(he_scb->omi_lm) &&
		!HTC_OM_CONTROL_UL_MU_DATA_DISABLE(he_scb->omi_lm))) {
		ret = TRUE;
	}
	return ret;
}

/* Function to check if a given scb can be admitted for dlmmu */
bool
wlc_he_get_dlmmu_allow(wlc_he_info_t *hei, struct scb *scb)
{
	scb_he_t *he_scb;
	bool ret = FALSE;

	he_scb = SCB_HE(hei, scb);
	if (he_scb && SCB_HE_CAP(scb) &&
		(!HTC_OM_CONTROL_UL_MU_DISABLE(he_scb->omi_lm))) {
		ret = TRUE;
	}
	return ret;
}

/* Return TX NSS (1-base, i.e. returns 1 for 1-stream) */
uint8
wlc_he_get_omi_tx_nsts(wlc_he_info_t *hei, scb_t *scb)
{
	scb_he_t *sh;
	sh = SCB_HE(hei, scb);
	return (HTC_OM_CONTROL_TX_NSTS(sh->omi_lm) + 1);
}

/** Return configured OMI BW in BW_XXX format range */
uint8
wlc_he_get_omi_bw(wlc_he_info_t *hei, scb_t *scb)
{
	scb_he_t *he_scb;
	he_scb = SCB_HE(hei, scb);
	if (he_scb) {
		/* Add 1 to convert from OPER_MODE_xxx to BW_xxx format */
		return HTC_OM_CONTROL_CHANNEL_WIDTH(he_scb->omi_lm) + 1;
	}
	return 0;
}

/** Return whether we have received valid OMI info from peer */
bool
wlc_he_received_valid_omi(wlc_he_info_t *hei, scb_t *scb)
{
	scb_he_t *he_scb;
	he_scb = SCB_HE(hei, scb);
	if (he_scb) {
		return he_scb->omi_recvd;
	}
	return FALSE;
}

/**
 * Process omi_data as received with HTC. The data will only contain the actual omi_data (12 bits)
 * and be loaded in omi_data, already CPU translated, use mask/shift operations to extract
 * individual fields. Store the received function and see if it matches against last received PMQ
 * version. If so then we can start applying it. Otherwise store it and wait for tx drain complete
 * trigger.
 */
static void
wlc_he_htc_process_omi(wlc_info_t* wlc, scb_t *scb, d11rxhdr_t *rxh, uint32 omi_data)
{
	scb_he_t *he_scb;
	bool rate_change, nss_change, ulmu_dis_change, bw_change;
	uint8 ps_omi, new_omi_bw;
	uint8 bw, rx_nss, tx_nss;
	bool dlmmu_dis_change;
	bool first_omi;

	BCM_REFERENCE(new_omi_bw);
	he_scb = SCB_HE(wlc->hei, scb);

	{
		/* When you get OMI value from HTC field if its BW is larger than BW from
		 * RXSTAT of the same pkt, change the OMI BW field to the one from RXSTAT
		 */
		uint8 omi_bw, chanspec_bw;
		uint16 chanspec = WLC_RX_CHSPEC(wlc->pub->corerev, rxh);
		/* make sure the new omi_bw does not exceed chanspec's bw */
		omi_bw = HTC_OM_CONTROL_CHANNEL_WIDTH(omi_data);
		if (CHSPEC_IS160(chanspec)) {
			chanspec_bw = DOT11_OPER_MODE_160MHZ;
		} else if (CHSPEC_IS8080(chanspec)) {
			chanspec_bw = DOT11_OPER_MODE_8080MHZ;
		} else if (CHSPEC_IS80(chanspec)) {
			chanspec_bw = DOT11_OPER_MODE_80MHZ;
		} else if (CHSPEC_IS40(chanspec)) {
			chanspec_bw = DOT11_OPER_MODE_40MHZ;
		} else {
			chanspec_bw = DOT11_OPER_MODE_20MHZ;
		}
		new_omi_bw = MIN(omi_bw, chanspec_bw);

		ASSERT(((CHSPEC_BW(SCB_BSSCFG(scb)->current_bss->chanspec) - WL_CHANSPEC_BW_20) >>
				WL_CHANSPEC_BW_SHIFT) >= chanspec_bw);

		if (omi_bw != new_omi_bw) {
			/* Override the omi_data with proper bw info */
			omi_data &= ~HTC_OM_CONTROL_CHANNEL_WIDTH_MASK;
			omi_data |= ((new_omi_bw << HTC_OM_CONTROL_CHANNEL_WIDTH_OFFSET)
				& HTC_OM_CONTROL_CHANNEL_WIDTH_MASK);

			WL_ERROR(("wl%d.%d "MACF": %s: recv omi_bw %d new_omi_bw %d "
				"chanspec_bw %d chanspec 0x%x raw omi 0x%x\n",
				wlc->pub->unit,	WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
				ETHER_TO_MACF(scb->ea),	__FUNCTION__, omi_bw, new_omi_bw,
				chanspec_bw, SCB_BSSCFG(scb)->current_bss->chanspec, omi_data));
		}
	}

	WL_PS_EX(scb, ("omi_pmq: 0x%x omi_htc: 0x%x -> 0x%x\n",
		he_scb->omi_pmq, he_scb->omi_htc, omi_data));

	if (he_scb->omi_htc == omi_data) {
		/* Nothing to update */
		return;
	}
	first_omi = (he_scb->omi_recvd == FALSE);

	WL_TRACE(("wl%d.%d %s: Rx OMI from HTC+: BW=%d, Rx NSS=%d, Tx NSTS=%d, UL MU Disable=%d, "
		"ER SU Disable=%d, DL MUMIMO Resound=%d, UL MU Data Disable=%d\n", wlc->pub->unit,
		WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
		HTC_OM_CONTROL_CHANNEL_WIDTH(omi_data), HTC_OM_CONTROL_RX_NSS(omi_data),
		HTC_OM_CONTROL_TX_NSTS(omi_data), HTC_OM_CONTROL_UL_MU_DISABLE(omi_data),
		HTC_OM_CONTROL_ER_SU_DISABLE(omi_data), HTC_OM_CONTROL_DL_MUMIMO_RESOUND(omi_data),
		HTC_OM_CONTROL_UL_MU_DATA_DISABLE(omi_data)));

	rate_change = ((omi_data & HE_TXRATE_UPDATE_MASK) !=
		(he_scb->omi_htc & HE_TXRATE_UPDATE_MASK));

	nss_change = ((omi_data & HTC_OM_CONTROL_TX_NSTS_MASK) !=
		(he_scb->omi_htc & HTC_OM_CONTROL_TX_NSTS_MASK));

	ulmu_dis_change = (((omi_data & HTC_OM_CONTROL_UL_MU_DISABLE_MASK) !=
		(he_scb->omi_htc & HTC_OM_CONTROL_UL_MU_DISABLE_MASK)) ||
		((omi_data & HTC_OM_CONTROL_UL_MU_DATA_DISABLE_MASK) !=
		(he_scb->omi_htc & HTC_OM_CONTROL_UL_MU_DATA_DISABLE_MASK)));

	dlmmu_dis_change = ((omi_data & HTC_OM_CONTROL_UL_MU_DISABLE_MASK) !=
		(he_scb->omi_htc & HTC_OM_CONTROL_UL_MU_DISABLE_MASK));

	bw_change = ((omi_data & HTC_OM_CONTROL_CHANNEL_WIDTH_MASK) !=
		(he_scb->omi_htc & HTC_OM_CONTROL_CHANNEL_WIDTH_MASK));

	/* Store received htc omi,
	 * because omi_htc has to be equal to omi_pmq to get SCB out of PS.
	 */
	he_scb->omi_htc = (uint16)omi_data;

	if (bw_change || first_omi) {
		/* invoke bw change callback to ulmu, need to remap OPER_MODE BW to internal val */
		STATIC_ASSERT((DOT11_OPER_MODE_20MHZ + 1) == BW_20MHZ);
		STATIC_ASSERT((DOT11_OPER_MODE_40MHZ + 1) == BW_40MHZ);
		STATIC_ASSERT((DOT11_OPER_MODE_80MHZ + 1) == BW_80MHZ);
		STATIC_ASSERT((DOT11_OPER_MODE_160MHZ + 1) == BW_160MHZ);
		wlc_ulmu_scb_bw_upd(wlc, scb, (new_omi_bw + 1));

#ifdef TESTBED_AP_11AX
		/* Update packet engine configuration. PKT engine calls are safe since if not
		 * programmed, then command will be ignored.
		 */
		update_pkt_eng_ulbw(wlc, scb->aid, HTC_OM_CONTROL_CHANNEL_WIDTH(omi_data));
#endif /* TESTBED_AP_11AX */
	}

	/* Adjust rx_nss and tx nsts with STF limitation */
	rx_nss = HTC_OM_CONTROL_RX_NSS(omi_data);
	if (rx_nss >= WLC_BITSCNT(wlc->stf->txchain)) {
		rx_nss = WLC_BITSCNT(wlc->stf->txchain) - 1;
		omi_data &= ~HTC_OM_CONTROL_RX_NSS_MASK;
		omi_data |= (rx_nss << HTC_OM_CONTROL_RX_NSS_OFFSET);
	}
	tx_nss = HTC_OM_CONTROL_TX_NSTS(omi_data);
	if (tx_nss >= wlc_stf_rxstreams_get(wlc)) {
		tx_nss = wlc_stf_rxstreams_get(wlc) - 1;
		omi_data &= ~HTC_OM_CONTROL_TX_NSTS_MASK;
		omi_data |= (tx_nss << HTC_OM_CONTROL_TX_NSTS_OFFSET);
	}
	he_scb->omi_lm = (uint16)omi_data;
	he_scb->omi_recvd = TRUE;

	if (rate_change || first_omi) {
		bw = HTC_OM_CONTROL_CHANNEL_WIDTH(omi_data);
		wlc_he_apply_omi(wlc, scb, bw, rx_nss, TRUE);
	}

	if (RATELINKMEM_ENAB(wlc->pub)) {
		wlc_ratelinkmem_update_link_entry(wlc, scb);
	}

	if (!BSSCFG_AP(SCB_BSSCFG(scb))) {
		/* rest of functionality is only applicable for AP mode, for
		 * STA simply exit here
		 */
		return;
	}
	if (nss_change) {
		if (HE_ULOMU_ENAB(wlc->pub) && SCB_HE_CAP(scb) && SCB_ULOFDMA_ADM(scb)) {
			wlc_ulmu_ul_nss_upd(wlc->ulmu, scb, tx_nss);
		}
#ifdef TESTBED_AP_11AX
		update_pkt_eng_ulnss(wlc, scb->aid, tx_nss);
#endif /* TESTBED_AP_11AX */
	}

	if (ulmu_dis_change) {
		WL_MUTX(("wl%d: %s: change ul ofdma STA "MACF" admission to %d\n",
			wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea),
			wlc_he_get_ulmu_allow(wlc->hei, scb)));
		wlc_ulmu_evaluate_admission(wlc->ulmu, scb);
#ifdef TESTBED_AP_11AX
		update_pkt_eng_ulstate(wlc, !HTC_OM_CONTROL_UL_MU_DISABLE(omi_data));
#endif /* TESTBED_AP_11AX */
	}
	if (dlmmu_dis_change || bw_change) {
#ifdef WL_MU_TX
		if (SCB_HEMMU(scb) && dlmmu_dis_change) {
			wlc_mutx_process_dlhemmu_dis_change(wlc->mutx, scb);
		} else if (SCB_DLOFDMA_ADM(scb) &&
			!wlc_musched_scb_isdlofdma_eligible(wlc->musched, scb)) {
			wlc_mutx_evict_dlofdma_client(wlc->mutx, scb);
		}
#endif
	}

	/* PS ON if omi_htc != omi_pmq.
	 *	- Nothing will be done if already in PS
	 * PS OFF if omi_htc == omi_pmq.
	 *	- Nothing will be done if already out of PS.
	 *	- If still draining txfifo, PS off will be pending
	 */
	if ((he_scb->omi_htc & HE_TXRATE_UPDATE_MASK) == he_scb->omi_pmq) {
		ps_omi = PS_SWITCH_OFF;
		wlc_apps_ps_requester(wlc, scb, 0, PS_SWITCH_OMI);
	} else {
		ps_omi = PS_SWITCH_OMI;
		wlc_apps_ps_requester(wlc, scb, PS_SWITCH_OMI, 0);
	}
	WL_PS_EX(scb, ("ps_omi 0x%x\n", ps_omi));
	wlc_apps_process_ps_switch(wlc, scb, ps_omi);
}

/**
 * This function is to be called by VHT module when AF frame containing oper_mode info.
 */
void
wlc_he_recv_af_oper_mode(wlc_info_t* wlc, scb_t *scb, uint8 oper_mode)
{
	scb_he_t *he_scb;
	uint8 omi_bw;
	uint8 omi_rx_nss;
	uint8 bw;
	uint8 rx_nss;

	/* Ignore info if the Rx NSS Type is 1 (no usefull information for us) */
	if (DOT11_OPER_MODE_RXNSS_TYPE(oper_mode)) {
		return;
	}

	omi_bw = DOT11_OPER_MODE_160_8080(oper_mode) ?
		DOT11_OPER_MODE_160MHZ : DOT11_OPER_MODE_CHANNEL_WIDTH(oper_mode);
	omi_rx_nss = DOT11_OPER_MODE_RXNSS(oper_mode) - 1;

	/* track that we recevied oper_mode so we next time we receive HT OMI we for sure apply */
	he_scb = SCB_HE(wlc->hei, scb);

	bw = HTC_OM_CONTROL_CHANNEL_WIDTH(he_scb->omi_lm);
	rx_nss = HTC_OM_CONTROL_RX_NSS(he_scb->omi_lm);

	if ((omi_bw != bw) || (omi_rx_nss != rx_nss)) {
		/* Tx nss is not defined in legacy operaring mode AF, dont update, but if the ucode
		 * really needs the field then we should program max nss based upon capabilities
		 * or use the rx value (need to discuss)
		 */
		he_scb->omi_lm &= ~HE_TXRATE_UPDATE_MASK;
		he_scb->omi_lm |= omi_bw << HTC_OM_CONTROL_CHANNEL_WIDTH_OFFSET;
		he_scb->omi_lm |= omi_rx_nss << HTC_OM_CONTROL_RX_NSS_OFFSET;
		wlc_he_apply_omi(wlc, scb, omi_bw, omi_rx_nss, FALSE);
		if (RATELINKMEM_ENAB(wlc->pub)) {
			wlc_ratelinkmem_update_link_entry(wlc, scb);
		}
	}
}

/**
 * Parse a received HTC code. The HTC code to be parsed should include the Variant bits, but the
 * variant has to be HE (will not be verified by this function), and it should be stored as it was
 * was received over the air, such that getbit(s) can be used to extract the information.
 */
static void
wlc_he_parse_htcode(wlc_info_t* wlc, scb_t *scb, d11rxhdr_t *rxh, uint8 *htc_code)
{
	uint32 read_idx;
	uint8 control_id;
	uint32 info_sz;
	uint32 control_data;

	/* Skip variant which is two bits */
	read_idx = HTC_VARIANT_SZ;

	/* Loop over all the encoded control IDs */
	while (read_idx < (HTC_CODE_SIZE - HTC_CONTROL_ID_SZ)) {
		control_id = getbits(htc_code, HTC_CODE_SIZE, read_idx, HTC_CONTROL_ID_SZ);
		read_idx += HTC_CONTROL_ID_SZ;
		switch (control_id) {
		case HTC_CONTROL_ID_TRS :
			/* Note: if this is the first HTC code then it is valid TRS, but if it
			 * is second one then it cant fit (TRS size is 26), but in that case it
			 * is supposed to be padding. Padding is made up by all 0. Length
			 * validation will pick up padding, so we set length TRS_SZ, and keep
			 * checks as simple as possible, since it will work correctly.
			 */
			info_sz = HTC_CONTROL_TRS_SZ;
			break;
		case HTC_CONTROL_ID_OM :
			info_sz = HTC_CONTROL_OM_SZ;
			break;
		case HTC_CONTROL_ID_HLA :
			info_sz = HTC_CONTROL_HLA_SZ;
			break;
		case HTC_CONTROL_ID_BSR :
			info_sz = HTC_CONTROL_BSR_SZ;
			break;
		case HTC_CONTROL_ID_UPH :
			info_sz = HTC_CONTROL_UPH_SZ;
			break;
		case HTC_CONTROL_ID_BQR :
			info_sz = HTC_CONTROL_BQR_SZ;
			break;
		case HTC_CONTROL_ID_CAS :
			info_sz = HTC_CONTROL_CAS_SZ;
			break;
		case HTC_CONTROL_ID_ONES :
			info_sz = HTC_CONTROL_ONES_SZ;
			break;
		default:
			/* Unhandled code. Problem: we cannot continue parsing as the the info
			 * size is unknown. Set sz to 0, so parsing gets skipped
			 */
			info_sz = 0;
			break;
		}
		/* Verify the info_sz to be valid (not too large) */
		if ((info_sz > (HTC_CODE_SIZE - read_idx)) || (info_sz == 0)) {
			break;
		}
		control_data = getbits(htc_code, HTC_CODE_SIZE, read_idx, info_sz);
		read_idx += info_sz;
		/* Now parse the different control IDs for which we have support */
		switch (control_id) {
		case HTC_CONTROL_ID_OM :
			wlc_he_htc_process_omi(wlc, scb, rxh, control_data);
			break;
		case HTC_CONTROL_ID_UPH :
			break; /* handled by ucode, ignore */
		case HTC_CONTROL_ID_ONES :
			break; /* dummy content, ignore */
		default:
			WL_INFORM(("Unhandled HTC %d, data 0x%03x\n", control_id, control_data));
			break;
		}

	}
}

/**
 * any received AMSDU/AMPDU/MPDU should call this function so HTC information can
 * extracted from header and data can be used.
 */
void
wlc_he_htc_recv(wlc_info_t* wlc, scb_t *scb, d11rxhdr_t *rxh, struct dot11_header *hdr)
{
	uint16 fc;
	uint8 *htc;
	uint32 offset;
	scb_he_t *he_scb;

	if (!HE_ENAB(wlc->pub))
		return;

	he_scb = SCB_HE(wlc->hei, scb);
	if (!he_scb) {
		return;
	}

	/* Determine if header includes HTC field. See 9.2.4.1.10. It needs to be HT or higher
	 * type frame, then it it has to be QOS frame, if both conditions true then fc.order bit
	 * specifies whether or not the HTC field is included/appended.
	 */
	fc = ltoh16(hdr->fc);
	if ((D11PPDU_FT(rxh, wlc->pub->corerev) < FT_HT) ||
	    !(FC_SUBTYPE_ANY_QOS(FC_SUBTYPE(fc))) || !(fc & FC_ORDER)) {
		return;
	}

	/* Check if we are dealing with HE HTC code */
	offset = HTC_CONTROL_OFFSET;
	if ((fc & (FC_TODS | FC_FROMDS)) == (FC_TODS | FC_FROMDS)) {
		offset += HTC_SIZEOFA4;
	}

	htc = (uint8 *)hdr;
	htc += offset;
	if ((htc[0] & HTC_IDENTITY_MASK) == HTC_IDENTITY_HE) {
		/* Yes, it is a HE HTC code */
		wlc_he_parse_htcode(wlc, scb, rxh, htc);
	}
}

/* Initialize he params in rtc */
void
wlc_he_htc_rtc_init(wlc_tx_rtc_t *rtc)
{
	wlc_info_t *wlc;
	scb_t *scb;
	ratespec_t rspec;

	wlc = rtc->wlc;
	scb = rtc->scb;

	rtc->he_htc_enab = FALSE;
	rtc->he_htc_force = FALSE;
	rtc->htc = NULL;
	rtc->htc_code = 0;

	rspec = wlc_scb_ratesel_get_primary(wlc, scb, NULL /* pkt */);
	/* can't do HTC on legacy rate frames */
	if (!RSPEC_ISLEGACY(rspec) && HE_ENAB(wlc->pub)) {
		scb_he_t *he_scb;
		he_scb = SCB_HE(wlc->hei, scb);
		if (he_scb && (he_scb->flags & SCB_HE_HTC_CAP)) {
			scb_he_htc_t *htc;
			htc = &he_scb->htc;
			if (BSSCFG_STA(SCB_BSSCFG(scb)) && wlc->hei->sta_insert_dummy_htc) {
				rtc->he_htc_enab = TRUE;
				rtc->he_htc_force = TRUE;
				rtc->htc = htc;
				rtc->htc_code = ALLONES_32;
			}
			if (htc->rd_idx != htc->wr_idx) {
				uint8 *data;

				rtc->he_htc_enab = TRUE;
				rtc->htc = htc;

				data = (uint8 *)&(rtc->htc_code);
				data[0] = (htc->codes[htc->rd_idx] | HTC_IDENTITY_HE) & 0xff;
				data[1] = (htc->codes[htc->rd_idx] >> 8) & 0xff;
				data[2] = (htc->codes[htc->rd_idx] >> 16) & 0xff;
				data[3] = (htc->codes[htc->rd_idx] >> 24) & 0xff;
			}
		}
	}
}

/* Register packet callback */
bool
wlc_he_htc_rtc_tx(wlc_tx_rtc_t *rtc, void *pkt)
{
	scb_he_htc_t *htc;
	bool ret = rtc->he_htc_force;

	htc = rtc->htc;

	ASSERT(rtc->he_htc_enab == TRUE);

	if (htc->outstanding) {
		return ret;
	}
	if (rtc->htc_code == ALLONES_32) {
		ASSERT(rtc->he_htc_force);
		return TRUE;
	}

	/* Register the callback function for TXSTATUS processing
	 * Do not check any return value here.. In case of failure
	 * wlc_pcb_fn_register itself will cause ASSERT
	 */
	wlc_pcb_fn_register(rtc->wlc->pcb, wlc_he_htc_pkt_freed, rtc->scb, pkt);
	htc->outstanding++;

	return TRUE;
}

/**
 * if there is a HTC+ code to transmit return true; Note: *htc_code will never be 0 in that case
 */
bool
wlc_he_htc_tx(wlc_info_t* wlc, scb_t *scb, void *pkt, uint32 *htc_code)
{
	scb_he_htc_t *htc;
	scb_he_t *he_scb;
	uint8 *data;
	ratespec_t rspec;
	bool ret = FALSE;

	if (!HE_ENAB(wlc->pub))
		goto done;

	he_scb = SCB_HE(wlc->hei, scb);
	if (!he_scb || !(he_scb->flags & SCB_HE_HTC_CAP)) {
		goto done;
	}

	rspec = wlc_scb_ratesel_get_primary(wlc, scb, pkt);
	if (RSPEC_ISLEGACY(rspec)) {
		/* can't do HTC on legacy rate frames */
		goto done;
	}

	if (BSSCFG_STA(SCB_BSSCFG(scb)) && wlc->hei->sta_insert_dummy_htc) {
		/* always need to include HTC, even if dummy */
		*htc_code = ALLONES_32;
		ret = TRUE;
	}

	htc = &he_scb->htc;

	if (htc->outstanding) {
		goto done;
	}

	if (htc->rd_idx == htc->wr_idx) {
		goto done;
	}

	/* Register the callback function for TXSTATUS processing */
	if (wlc_pcb_fn_register(wlc->pcb, wlc_he_htc_pkt_freed, scb, pkt)) {
		WL_ERROR(("wl%d: %s : HTC pcb registration failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto done; /* keep dummy if needed */
	}

	data = (uint8 *)htc_code;
	data[0] = (htc->codes[htc->rd_idx] | HTC_IDENTITY_HE) & 0xff;
	data[1] = (htc->codes[htc->rd_idx] >> 8) & 0xff;
	data[2] = (htc->codes[htc->rd_idx] >> 16) & 0xff;
	data[3] = (htc->codes[htc->rd_idx] >> 24) & 0xff;

	htc->outstanding++;
	ret = TRUE;
done:
	return ret;
}

/**
 * Called upon packet free, when packet contains HTC code. If success then allow to go to next
 * code (if any in array of codes to transmit). If no success, then set code to be retried on
 * next packet.
 */
static void BCMFASTPATH
wlc_he_htc_pkt_freed(wlc_info_t *wlc, uint txs, void* arg)
{
	struct scb *scb = (struct scb*) arg;
	scb_he_t *he_scb;
	scb_he_htc_t *htc;

	ASSERT(HE_ENAB(wlc->pub));

	if (scb == NULL)
		return;

	he_scb = SCB_HE(wlc->hei, scb);

	if (!he_scb) {
		return;
	}
	htc = &he_scb->htc;

	if ((txs & TX_STATUS_MASK) == TX_STATUS_ACK_RCV) {
		ASSERT(htc->rd_idx != htc->wr_idx);
		ASSERT(htc->outstanding == 1);
		if (htc->cb_fn[htc->rd_idx]) {
			htc->cb_fn[htc->rd_idx](wlc, scb, htc->codes[htc->rd_idx]);
		}
		htc->rd_idx = HE_NEXT_HTC_IDX(htc->rd_idx);
	}
	htc->outstanding--;
}

/**
 * Add a htc code to queue of htc codes to be transmitted.
 */
void
wlc_he_htc_send_code(wlc_info_t *wlc, scb_t *scb, uint32 htc_code, he_htc_cb_fn_t cb_fn)
{
	scb_he_t *he_scb;
	scb_he_htc_t *htc;

	ASSERT(HE_ENAB(wlc->pub));

	he_scb = SCB_HE(wlc->hei, scb);

	if (!he_scb || !(he_scb->flags & SCB_HE_HTC_CAP)) {
		return;
	}

	htc = &he_scb->htc;

	/* Check if there is room to send next HTC code */
	if (HE_NEXT_HTC_IDX(htc->wr_idx) != htc->rd_idx) {
		htc->codes[htc->wr_idx] = htc_code;
		htc->cb_fn[htc->wr_idx] = cb_fn;
		htc->wr_idx = HE_NEXT_HTC_IDX(htc->wr_idx);
	} else {
		WL_ERROR(("%s No empty slot to store new HTC code, overwriting last (%d)\n",
			__FUNCTION__, htc->wr_idx));
		htc->codes[htc->wr_idx] = htc_code;
		htc->cb_fn[htc->wr_idx] = cb_fn;
	}
}

/* returns TRUE if the HTC code in this packet is a real rather than a 'dummy' (ONES) code */
bool
wlc_he_pkt_has_real_htc(wlc_info_t* wlc, scb_t *scb, void *pkt)
{
	bool ret = FALSE;
	scb_he_t *he_scb;
	scb_he_htc_t *htc;

	/* we only get called for packets that have HTC/order bit set */

	ASSERT(HE_ENAB(wlc->pub));

	if (!wlc->hei->sta_insert_dummy_htc || /* if we don't insert dummies, it must be real */
		!BSSCFG_STA(SCB_BSSCFG(scb))) { /* if this is not STA iface, it must be real */
		return TRUE;
	}

	he_scb = SCB_HE(wlc->hei, scb);
	ASSERT(he_scb != NULL);
	ASSERT(he_scb->flags & SCB_HE_HTC_CAP);

	htc = &he_scb->htc;
	if (htc->outstanding > 0) {
		/* check if this packet caused the 'outstanding' */
		ret = wlc_pcb_pkt_is_registered(wlc->pcb, wlc_he_htc_pkt_freed, scb, pkt);
	} /* else must be dummy */

	return ret;
}

static int
wlc_he_cmd_htc(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen, bool set,
	wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	scb_iter_t scbiter;
	scb_t *scb;

	if (set) {
		uint32 htc_code = *(uint32 *)params;

		FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
			wlc_he_htc_send_code(wlc, scb, htc_code, NULL);
		}
	} else {
		return BCME_UNSUPPORTED;
	}

	return BCME_OK;
}

static void
wlc_he_tx_omi_callback(wlc_info_t *wlc, struct scb *scb, uint32 htc_omi)
{
	scb_he_t *he_scb;
	uint16 omi;
	uint8 tx_nsts;
	uint8 bw;
	uint8 rx_nss;

	he_scb = SCB_HE(wlc->hei, scb);

	omi = (htc_omi >> (HTC_CONTROL_ID_SZ + HTC_VARIANT_SZ));
	tx_nsts = HTC_OM_CONTROL_TX_NSTS(omi);
	rx_nss = HTC_OM_CONTROL_RX_NSS(omi);
	bw = HTC_OM_CONTROL_CHANNEL_WIDTH(omi);

	/* Update linkmem with "inverted" data */
	he_scb->omi_lm = HE_OMI_ENCODE(tx_nsts, rx_nss, bw, HTC_OM_CONTROL_ER_SU_DISABLE(omi),
		HTC_OM_CONTROL_DL_MUMIMO_RESOUND(omi), HTC_OM_CONTROL_UL_MU_DISABLE(omi),
		HTC_OM_CONTROL_UL_MU_DATA_DISABLE(omi));
	/* Update Tx rate table */
	wlc_he_apply_omi(wlc, scb, bw, tx_nsts, TRUE);
	if (RATELINKMEM_ENAB(wlc->pub)) {
		wlc_ratelinkmem_update_link_entry(wlc, scb);
	}
}

static int
wlc_he_cmd_omi(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen, bool set,
	wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	wl_he_omi_t *omi = (wl_he_omi_t *)params;
	struct ether_addr *addr;
	scb_t *scb;
	scb_he_t *he_scb;
	uint32 htc;
	bool use_htc_callback;
	uint8 lm_tx_nsts;
	uint8 lm_bw;
	int ul_mu_data_disable;

	if (!cfg) {
		return BCME_BADARG;
	}
	/* ignore the user supplied peer address for infra STA and use BSSID instead */
	addr = (BSSCFG_STA(cfg) && cfg->BSS) ? &cfg->BSSID : &omi->peer;
	/* lookup the scb */
	scb = wlc_scbfind(wlc, cfg, addr);
	if (!scb) {
		return BCME_BADARG;
	}

	he_scb = SCB_HE(wlc->hei, scb);

	if (!he_scb) {
		return BCME_UNSUPPORTED;
	}

	if (set) {
#ifdef TESTBED_AP_11AX
		/* for testbed AP there is an override needed for only txnsts. This should not
		 * be communicated to remote, only result in generation of new txrate tables
		 * with new txnsts (and possibly bw, but test does not require that).
		 */
		if (omi->tx_override) {
			he_scb->omi_lm = HE_OMI_ENCODE(omi->tx_nsts, omi->rx_nss,
				omi->channel_width, omi->er_su_disable,
				omi->dl_mumimo_resound, omi->ul_mu_disable,
				omi->ul_mu_data_disable);
			wlc_he_apply_omi(wlc, scb, omi->channel_width, omi->tx_nsts, TRUE);
			if (RATELINKMEM_ENAB(wlc->pub)) {
				wlc_ratelinkmem_update_link_entry(wlc, scb);
			}
			return BCME_OK;
		}
#endif /* TESTBED_AP_11AX */
		if (!(he_scb->flags & SCB_HE_OMI)) {
			return BCME_UNSUPPORTED;
		}
		if (he_scb->flags & SCB_HE_OMI_UL_MU_DATA_DIS) {
			ul_mu_data_disable = omi->ul_mu_data_disable;
		} else {
			ul_mu_data_disable = 0;
		}
		/* The paramaters for tx and rx are about to be updated. If tx params get
		 * updated then all tx frames be suppressed, htc transmission (using null frame),
		 * then tx rateset update, linkmem update and finally tx unblock. Found improved
		 * method. Limit the rateset to min of current and new (if needed). send htc, once
		 * sent (callback) update rateset to new set. This will make sure packet will
		 * always be sent with "valid" tx set and gives best tput and easiest handling.
		 */
		use_htc_callback = TRUE;
		lm_tx_nsts = HTC_OM_CONTROL_RX_NSS(he_scb->omi_lm);
		lm_bw = HTC_OM_CONTROL_CHANNEL_WIDTH(he_scb->omi_lm);
		if ((lm_tx_nsts > omi->tx_nsts) || (lm_bw > omi->channel_width)) {
			/* Update Tx rate table to temporarily minium value. If this value
			 * is the same as what is configured then dont set callback for htc
			 * but do all the updating now and be done with it.
			 */
			if ((lm_tx_nsts >= omi->tx_nsts) && (lm_bw >= omi->channel_width)) {
				/* Update linkmem */
				he_scb->omi_lm = HE_OMI_ENCODE(omi->tx_nsts, omi->rx_nss,
					omi->channel_width, omi->er_su_disable,
					omi->dl_mumimo_resound, omi->ul_mu_disable,
					ul_mu_data_disable);
				wlc_he_apply_omi(wlc, scb, omi->channel_width, omi->tx_nsts, TRUE);
				if (RATELINKMEM_ENAB(wlc->pub)) {
					wlc_ratelinkmem_update_link_entry(wlc, scb);
				}
				use_htc_callback = FALSE;
			} else {
				wlc_he_apply_omi(wlc, scb,
					lm_bw > omi->channel_width ? omi->channel_width : lm_bw,
					lm_tx_nsts > omi->tx_nsts ? omi->tx_nsts : lm_tx_nsts,
					TRUE);
			}
		}

		/* Create HTC code, note: it will go out new rateset, which is wrong. We should
		 * use a null frame at bw20 and 1x1 to be safe. TBD!
		 */
		htc = HE_OMI_ENCODE(omi->rx_nss, omi->tx_nsts, omi->channel_width,
			omi->er_su_disable, omi->dl_mumimo_resound, omi->ul_mu_disable,
			ul_mu_data_disable);
		htc = (htc << HTC_CONTROL_ID_SZ) | HTC_CONTROL_ID_OM;
		htc = (htc << HTC_VARIANT_SZ) | HTC_IDENTITY_HE;
		wlc_he_htc_send_code(wlc, scb, htc,
			use_htc_callback ? wlc_he_tx_omi_callback : NULL);
		/* Send NULL data frame to make sure HTC gets sent... */
		wlc_sendnulldata(wlc, cfg, &scb->ea, 0, 0, PRIO_8021D_BE, NULL, NULL);
	} else {
		omi = (wl_he_omi_t *)result;
		memcpy(&omi->peer, addr, sizeof(omi->peer));
		omi->rx_nss = HTC_OM_CONTROL_TX_NSTS(he_scb->omi_lm);
		omi->channel_width = HTC_OM_CONTROL_CHANNEL_WIDTH(he_scb->omi_lm);
		omi->tx_nsts = HTC_OM_CONTROL_RX_NSS(he_scb->omi_lm);
		omi->er_su_disable = HTC_OM_CONTROL_ER_SU_DISABLE(he_scb->omi_lm);
		omi->dl_mumimo_resound = HTC_OM_CONTROL_DL_MUMIMO_RESOUND(he_scb->omi_lm);
		omi->ul_mu_disable = HTC_OM_CONTROL_UL_MU_DISABLE(he_scb->omi_lm);
		omi->ul_mu_data_disable = HTC_OM_CONTROL_UL_MU_DATA_DISABLE(he_scb->omi_lm);
		omi->tx_override = 0;
	}

	return BCME_OK;
}

/**
 * wlc_he_omi_pmq_code is to be called when a PMQ interrupt with OMI information is received.
 */
bool
wlc_he_omi_pmq_code(wlc_info_t *wlc, scb_t *scb, uint8 rx_nss, uint8 bw)
{
	scb_he_t *he_scb;

	if (!HE_ENAB(wlc->pub)) {
		return FALSE;
	}

	he_scb = SCB_HE(wlc->hei, scb);
	if (!he_scb) {
		return FALSE;
	}

	he_scb->omi_pmq = HE_OMI_ENCODE(rx_nss, 0, bw, 0, 0, 0, 0);

	WL_PS_EX(scb, ("omi_htc 0x%x omi_pmq 0x%x\n", he_scb->omi_htc, he_scb->omi_pmq));

	/* return whether to suppress */
	return ((he_scb->omi_htc & HE_TXRATE_UPDATE_MASK) != he_scb->omi_pmq);
}

/** called on './wl he fragtx' */
static int
wlc_he_cmd_fragtx(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;

	if (set) {
		uint8 fragtx = *params;

		/* fragtx : now support bit 0 and bit 1 for SU/MU traffic respectively */
		if (fragtx > 0x3) {
			return BCME_BADARG;
		}

		wlc->pub->_he_fragtx = fragtx;

		if (RATELINKMEM_ENAB(wlc->pub)) {
			wlc_ratelinkmem_update_link_entry_all(wlc, NULL, FALSE,
				FALSE /* clr_txbf_stats=0 in mreq */);
		}
	} else {
		*result = wlc->pub->_he_fragtx;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

/** called from wlc_apps_omi_waitpmq_war as a WAR */
void
wlc_he_omi_pmq_reset(wlc_info_t *wlc, scb_t *scb)
{
	scb_he_t *he_scb;

	if (!HE_ENAB(wlc->pub)) {
		return;
	}

	he_scb = SCB_HE(wlc->hei, scb);
	if (!he_scb) {
		return;
	}

	/* pmqdata updates only lower 5bits of RXNSS | BW */
	he_scb->omi_pmq = (he_scb->omi_htc & HE_TXRATE_UPDATE_MASK);
}

static int
wlc_he_cmd_sr(void *ctx, uint8 *params, uint16 plen, uint8 *result,
	uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wlc_bsscfg_t *cfg;
	wl_he_sr_v1_t *sr;

	cfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(cfg != NULL);
	BCM_REFERENCE(cfg);

	if (set) {
		sr = (wl_he_sr_v1_t *)params;

		if (sr->version != WL_HE_SR_VER_1) {
			return BCME_BADARG;
		}
		hei->sr_enabled = sr->enabled;
		hei->sr_control = 0;
		if (sr->src & SRC_PSR_DIS) {
			setbit(&hei->sr_control, HE_SR_CONTROL_SRP_DISALLOWED_IDX);
		}
		if (sr->src & SRC_NON_SRG_OBSS_PD_SR_DIS) {
			setbit(&hei->sr_control, HE_SR_CONTROL_NON_SRG_OBSS_PD_SR_DISALLOWED_IDX);
		}
		if (sr->src & SRC_NON_SRG_OFFSET_PRESENT) {
			setbit(&hei->sr_control, HE_SR_CONTROL_NON_SRG_OFFSET_PRESENT_IDX);
			hei->non_srg[0] = sr->non_srg_offset;
		}
		if (sr->src & SRC_SRG_INFORMATION_PRESENT) {
			setbit(&hei->sr_control, HE_SR_CONTROL_SRG_INFORMATION_PRESENT_IDX);
			memcpy(&hei->srg, sr->srg, sizeof(hei->srg));
		}
		if (sr->src & SRC_HESIGA_SPATIAL_REUSE_VALUE15_ALLOWED) {
			setbit(&hei->sr_control,
				HE_SR_CONTROL_HESIGA_SPATIAL_REUSE_VALUE15_ALLOWED_IDX);
		}

		if (BSSCFG_AP(cfg)) {
			if (wlc->pub->up) {
				wlc_update_beacon(wlc);
				wlc_update_probe_resp(wlc, TRUE);
			}
		}
	} else {
		sr = (wl_he_sr_v1_t *)result;
		sr->version = WL_HE_SR_VER_1;
		sr->length = sizeof(*sr) - 4;

		sr->src = 0;
		if (isset(&hei->sr_control, HE_SR_CONTROL_SRP_DISALLOWED_IDX)) {
			sr->src |= SRC_PSR_DIS;
		}
		if (isset(&hei->sr_control, HE_SR_CONTROL_NON_SRG_OBSS_PD_SR_DISALLOWED_IDX)) {
			sr->src |= SRC_NON_SRG_OBSS_PD_SR_DIS;
		}
		if (isset(&hei->sr_control, HE_SR_CONTROL_NON_SRG_OFFSET_PRESENT_IDX)) {
			sr->src |= SRC_NON_SRG_OFFSET_PRESENT;
		}
		if (isset(&hei->sr_control, HE_SR_CONTROL_SRG_INFORMATION_PRESENT_IDX)) {
			sr->src |= SRC_SRG_INFORMATION_PRESENT;
		}
		if (isset(&hei->sr_control,
			HE_SR_CONTROL_HESIGA_SPATIAL_REUSE_VALUE15_ALLOWED_IDX)) {

			sr->src |= SRC_HESIGA_SPATIAL_REUSE_VALUE15_ALLOWED;
		}
		sr->non_srg_offset = hei->non_srg[0];
		memcpy(sr->srg, &hei->srg, sizeof(sr->srg));

		sr->enabled = hei->sr_enabled;
		*rlen = sizeof(*sr);
	}

	return BCME_OK;
}

static int
wlc_he_cmd_defcap(void *ctx, uint8 *params, uint16 plen, uint8 *result,
        uint16 *rlen, bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_info_t *wlc = hei->wlc;
	wl_he_defcap_t *defcap = (wl_he_defcap_t *)result;
	wl_he_defcap_t *pars = (wl_he_defcap_t *)params;
	wlcband_t *orig_band;
	enum wlc_bandunit bu;
	wlc_bsscfg_t *cfg;
	bool cfg_isap;
	bool bw80_limited;

	if (set) {
		return BCME_UNSUPPORTED;
	} else {
		if (*rlen < sizeof(*defcap) || plen < sizeof(*defcap)) {
			return BCME_BUFTOOSHORT;
		}
		bu = wlc_bandtype2bandunit_noassert(pars->band);
		if (bu == MAXBANDS) {
			return BCME_NOTFOUND;
		}
		if (pars->bsscfg_type != BSSCFG_TYPE_GENERIC) {
			return BCME_BADARG;
		}

		ASSERT(wlcif->type == WLC_IFTYPE_BSS);
		cfg = wlcif->u.bsscfg;
		cfg_isap = cfg->_ap;

		switch (pars->bsscfg_subtype) {
		case BSSCFG_GENERIC_STA:
			defcap->bsscfg_subtype = BSSCFG_GENERIC_STA;
			wlc_he_sta_maccap_init(hei, &defcap->mac_cap);
			wlc_he_sta_phycap_init(hei, &defcap->phy_cap);
			cfg->_ap = FALSE;
			break;
		case BSSCFG_GENERIC_AP:
			defcap->bsscfg_subtype = BSSCFG_GENERIC_AP;
			wlc_he_ap_maccap_init(hei, &defcap->mac_cap);
			wlc_he_ap_phycap_init(hei, &defcap->phy_cap);
			cfg->_ap = TRUE;
			break;
		default:
			return BCME_BADARG;
		}

		defcap->version = WL_HE_DEFCAP_VER_1;
		defcap->length = sizeof(*defcap);
		defcap->band = pars->band;
		defcap->bsscfg_type = BSSCFG_TYPE_GENERIC;

		orig_band = wlc->band;
		wlc->band = wlc->bandstate[bu];
		bw80_limited = !wlc->pub->phy_bw160_capable ||
			(wlc_channel_locale_flags(wlc->cmi) & WLC_NO_160MHZ);

		wlc_he_fill_mac_cap(wlc, cfg, &defcap->mac_cap);
		wlc_he_fill_phy_cap(wlc, cfg, bw80_limited, &defcap->phy_cap);

		wlc->band = orig_band;
		cfg->_ap = cfg_isap;
		wlc_set_wake_ctrl(wlc);

		*rlen = sizeof(*defcap);
	}
	return BCME_OK;
}

#ifdef TESTBED_AP_11AX
static int
wlc_he_cmd_bsr(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen, bool set,
	wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;

	if (set) {
		hei->bsr_supported = *params;
	} else {
		*result = hei->bsr_supported;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

static int
wlc_he_cmd_testbed(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen, bool set,
	wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;

	if (set) {
		hei->testbed_mode = (*params) ? TRUE : FALSE;
		hei->muedca_blocked = (*params) ? TRUE : FALSE;
	} else {
		*result = hei->testbed_mode;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}
#endif /* TESTBED_AP_11AX */

/*
 * Update the HE rateset of SCB based on the max NSS.
 */
void
wlc_he_upd_scb_rateset_mcs(wlc_he_info_t *hei, struct scb *scb, uint8 link_bw)
{
	wlc_info_t *wlc =  hei->wlc;
	wlc_bsscfg_t *cfg = SCB_BSSCFG(scb);
	uint txstreams, rxstreams, peer_nss, omi_rx_nss;
	scb_he_t *he_scb;
	bss_he_info_t *bhei = BSS_HE_INFO(hei, cfg);

	/* For Tx chain limit, use min of op_txsreams and Peer's NSS from OMN;
	 * If Peer is in OMN, use the RXNSS of OMN as the Tx chains.
	 */
	if (SCB_HE_CAP(scb)) {
		he_scb = SCB_HE(hei, scb);
		ASSERT(he_scb != NULL);

		peer_nss = HE_MAX_SS_SUPPORTED(he_scb->bw80_rx_mcs_nss);
		if (link_bw == BW_160MHZ) {
			peer_nss = MAX(peer_nss, HE_MAX_SS_SUPPORTED(he_scb->bw160_rx_mcs_nss));
		}
		omi_rx_nss = peer_nss;

		/* Rely on OMC only when we are OMC capable, else fallback to VHT OMN */
		if (bhei->omc_cap) {
			omi_rx_nss = HTC_OM_CONTROL_RX_NSS(he_scb->omi_lm) + 1;
		} else if (SCB_VHT_CAP(scb) && wlc_vht_is_omn_enabled(wlc->vhti, scb)) {
			omi_rx_nss = wlc_vht_get_scb_oper_nss(wlc->vhti, scb);
		} else if (WLC_STF_GET_SCB_MIMOPS_ENAB(wlc->stf, scb) &&
			!WLC_STF_GET_SCB_MIMOPS_RTS_ENAB(wlc->stf, scb)) {
			/* peer signals MIMOPS but not RTS activated MIMOPS, so limit to 1 stream */
			omi_rx_nss = 1;
		}
		peer_nss = MIN(peer_nss, omi_rx_nss);
		/* update OMI */
		he_scb->omi_lm &= ~HTC_OM_CONTROL_RX_NSS_MASK;
		he_scb->omi_lm |= (peer_nss - 1) << HTC_OM_CONTROL_RX_NSS_OFFSET;
		ASSERT(peer_nss == HTC_OM_CONTROL_RX_NSS(he_scb->omi_lm) + 1);

		WL_RATE(("wl%d.%d: %s AP %d STA %d omc_cap %d omi_rx_nss %d peer_nss %d\n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__, BSSCFG_AP(cfg),
			BSSCFG_STA(cfg), bhei->omc_cap, omi_rx_nss, peer_nss));
	} else if (SCB_VHT_CAP(scb)) {
		peer_nss = wlc_vht_get_scb_oper_nss(wlc->vhti, scb);
	} else {
		return;
	}

	txstreams = MIN(wlc->stf->op_txstreams, peer_nss);
	rxstreams = MIN(wlc->stf->op_rxstreams, wlc_stf_rxstreams_get(wlc));

	WL_RATE(("wl%d.%d: %s op_txstreams %d txstreams %d op_rxstreams %d rxstreams %d\n",
		wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__, wlc->stf->op_txstreams,
		txstreams, wlc->stf->op_rxstreams, rxstreams));

	/* Intersect and update rateset */
	wlc_he_intersect_txrxmcsmaps(hei, scb, !wlc_he_bw80_limited(wlc, cfg),
		FALSE, txstreams, rxstreams);
}

bool
wlc_he_is_nonbrcm_160sta(wlc_he_info_t *hei, scb_t *scb)
{
	uint32 channel_width_set;
	scb_he_t *sh;

	sh = SCB_HE(hei, scb);
	ASSERT(sh != NULL);

	channel_width_set = getbits((uint8*)&sh->phy_cap, sizeof(sh->phy_cap),
		HE_PHY_CH_WIDTH_SET_IDX, HE_PHY_CH_WIDTH_SET_FSZ);

	if (!SCB_IS_BRCM(scb) &&
		(channel_width_set & (HE_PHY_CH_WIDTH_2G_40 | HE_PHY_CH_WIDTH_5G_80 |
			HE_PHY_CH_WIDTH_5G_160))) {
		WL_INFORM(("%s : "MACF" likely non brcm bw160 sta\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return TRUE;
	}

	return FALSE;
}

static int
wlc_he_cmd_color_collision(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wlc_he_color_collision_t *cc = &hei->cc;

	if (set) {
		cc->ap_detect_enab = ((*(uint8 *)params) & HE_CC_AP_DETECT_ENAB) ? TRUE : FALSE;
		cc->ap_report_handler_enab = ((*(uint8 *)params) & HE_CC_AP_REPORT_HANDLER_ENAB) ?
			TRUE : FALSE;
		cc->sta_detect_enab = ((*(uint8 *)params) & HE_CC_STA_DETECT_ENAB) ? TRUE : FALSE;
		cc->auto_enab = ((*(uint8 *)params) & HE_CC_AUTO_ENAB) ? TRUE : FALSE;
		cc->event_enab = ((*(uint8 *)params) & HE_CC_EVENT_ENAB) ? TRUE : FALSE;
		if ((cc->ap_detect_enab) || ((cc->sta_detect_enab))) {
			/* Enable promisc beacons */
			wlc_mac_bcn_promisc_update(hei->wlc, BCNMISC_COLOR_COLLISION, TRUE);
		} else {
			/* Restore the default setting for promisc beacons */
			wlc_mac_bcn_promisc_update(hei->wlc, BCNMISC_COLOR_COLLISION, FALSE);
		}
		cc->prev_cls_cnt = wlc_read_shm(hei->wlc, M_COCLS_CNT(hei->wlc->hw));
		if ((cc->event_enab) && (!cc->create_event_cnt)) {
			cc->create_event_cnt = HE_CC_EVENT_DELAY;
		}
	} else {
		*result = cc->ap_detect_enab ? HE_CC_AP_DETECT_ENAB : 0;
		*result |= cc->ap_report_handler_enab ? HE_CC_AP_REPORT_HANDLER_ENAB : 0;
		*result |= cc->auto_enab ? HE_CC_AUTO_ENAB : 0;
		*result |= cc->event_enab ? HE_CC_EVENT_ENAB : 0;
		*result |= cc->sta_detect_enab ? HE_CC_STA_DETECT_ENAB : 0;
		*rlen = sizeof(*result);
	}

	return BCME_OK;
}

static int
wlc_he_cmd_trigger_color_event(void *ctx, uint8 *params, uint16 plen, uint8 *result, uint16 *rlen,
	bool set, wlc_if_t *wlcif)
{
	wlc_he_info_t *hei = ctx;
	wl_color_event_t color_event;

	wlc_he_color_collision_event(hei, &color_event);

	if (!set) {
		if (*rlen < sizeof(color_event) || plen < sizeof(color_event)) {
			return BCME_BUFTOOSHORT;
		}

		memcpy(result, &color_event, sizeof(color_event));
		*rlen = sizeof(color_event);
	}
	return BCME_OK;
}

/**
 * HE color (collision) event, generate mac event with detected colors.
 *
 * @param hei		handle to wlc_he_info_t context.
 * @return		none
 */
static void
wlc_he_color_collision_event(wlc_he_info_t *hei, wl_color_event_t *color_event)
{
	wlc_he_color_collision_t *cc = &hei->cc;
	uint idx, color;

	memset(color_event, 0, sizeof(*color_event));

	color_event->own_color = hei->bsscolor;
	color_event->collision |= cc->local_collision_detect_cnt ?
		WL_COLOR_COLLISION_DETECT_LOCAL : 0;
	color_event->collision |= cc->remote_collision_detect_cnt ?
		WL_COLOR_COLLISION_DETECT_REMOTE : 0;

	idx = 0;
	for (color = 1; color <= WL_COLOR_MAX_VALUE; color++) {
		if (cc->detected_colors_cnt[color]) {
			color_event->colors[idx] = color;
			idx++;
		}
	}
	wlc_bss_mac_event(hei->wlc, NULL, WLC_E_COLOR, NULL, 0, 0, 0, color_event,
		sizeof(*color_event));
}

/**
 * HE color collision add found color with specified timeout.
 *
 * @param hei		handle to wlc_he_info_t context.
 * @param color		color to be added as in use.
 * @param timeout	timeout to use in seconds for the newly detected in use color.
 * @return		none
 */
static void
wlc_he_color_collision_add(wlc_he_info_t *hei, uint8 color, uint8 timeout)
{
	wlc_he_color_collision_t *cc = &hei->cc;

	ASSERT(timeout > 0);
	ASSERT(color < ARRAYSIZE(cc->detected_colors_cnt));

	if (!cc->detected_colors_cnt[color]) {
		cc->detected_colors_total++;
		if ((cc->event_enab) && (!cc->create_event_cnt)) {
			cc->create_event_cnt = HE_CC_EVENT_DELAY;
		}
	}
	if (timeout > cc->detected_colors_cnt[color]) {
		cc->detected_colors_cnt[color] = timeout;
	}
}

/**
 * HE color collision beacon and probe response processing routine. This function is called when
 * HE op IE is found in beacon or probe response. Register the color found in the OP ie as in use.
 *
 * @param hei		handle to wlc_he_info_t context
 * @param op_ie		pointer to HE operation IE of the received beacon/probe response.
 * @return		none
 */
static void
wlc_he_color_collision_process_beacon_probe(wlc_he_info_t *hei, wlc_bsscfg_t *bsscfg,
	he_op_ie_t* op_ie)
{
	wlc_info_t *wlc = hei->wlc;
	wlc_he_color_collision_t *cc = &hei->cc;
	uint8 bsscolor = 0;
	uint8 random;
#ifdef STA
	int idx;
	wlc_bsscfg_t *cfg;
	bss_he_info_t *bhei;
	scb_t *scb;
#endif /* STA */

	/* If the Operation IE parsing was due to beacon or probe response received while scanning
	 * then the value should be ignored. Only beacons and probe responses which are received
	 * in normal operation indicate colors which should not be used.
	 */
	if (SCAN_IN_PROGRESS(hei->wlc->scan)) {
		return;
	}

	if ((cc->ap_detect_enab) || (cc->sta_detect_enab)) {
		bsscolor = (uint8)getbits((uint8 *)&op_ie->color, sizeof(op_ie->color),
			HE_OP_BSS_COLOR_IDX, HE_OP_BSS_COLOR_FSZ);
		wlc_he_color_collision_add(hei, bsscolor, HE_CC_LOCAL_AP_DETECT_TIMEOUT);
	}
#ifdef STA
	if (cc->sta_detect_enab) {
		/* inform all our connected-to APs */
		FOREACH_AS_STA(wlc, idx, cfg) {
			if (cc->detect_backoff_cnt) {
				continue;
			}
			if (cfg == bsscfg) {
				continue;
			}
			if (isclr(cfg->ext_cap, DOT11_EXT_CAP_EVENT)) {
				continue;
			}
			bhei = BSS_HE_INFO(hei, cfg);
			if (bhei->bsscolor != bsscolor) {
				continue;
			}
			if ((scb = wlc_scbfind(wlc, cfg, &cfg->BSSID)) != NULL) {
				uint8 colors[DOT11_EVENT_COLOR_COLLISION_REPORT_LEN];
				int i;

				memset(colors, 0, sizeof(colors));
				for (i = 1; i < ARRAYSIZE(cc->detected_colors_cnt); i++) {
					if (cc->detected_colors_cnt[i]) {
						setbit(colors, i);
					}
				}
				/* Use exponential backoff for the reporting */
				if (bsscolor == cc->detect_prev_color) {
					if (cc->detect_backoff_init_cnt ==
						HE_CC_DETECT_BACKOFF_INIT) {

						cc->detect_backoff_init_cnt =
							HE_CC_DETECT_BACKOFF_DEFAULT;
					} else if (cc->detect_backoff_init_cnt < 32768) {
						cc->detect_backoff_init_cnt *= 2;
					}
				} else {
					/* If it is the first time and device is STA then use
					 * initial backoff of 2 secs to make sure we gather the
					 * colors from the neighbourhood APs.
					 */
					cc->detect_backoff_init_cnt = HE_CC_DETECT_BACKOFF_INIT;
				}
				cc->detect_backoff_cnt = cc->detect_backoff_init_cnt;
				cc->detect_prev_color = bsscolor;
				if (cc->detect_backoff_init_cnt != HE_CC_DETECT_BACKOFF_INIT) {
					wlc_wnm_send_color_collision_frame(wlc, cfg, scb, colors);
				}
			}
		}
	}
#endif /* STA */

	if (cc->ap_detect_enab) {
		if ((wlc->main_ap_bsscfg != NULL) && (bsscolor == hei->bsscolor)) {
			cc->local_collision_detect_cnt = HE_CC_LOCAL_COLL_CNT_INIT;
			if ((cc->auto_enab) && (!cc->detect_local_cnt)) {

				wlc_getrand(hei->wlc, &random, 1);
				cc->detect_local_cnt = 2 +
					(random % HE_CC_LOCAL_AP_DETECT_MAX_DELAY);
			}
		}
	}
}

/**
 * HE color collision report provided by WNM. This function is called when a collision report is
 * received.
 *
 * @param hei		handle to wlc_he_info_t context
 * @param colors	pointer to color bitmap as received in report.
 * @return		none
 */
void
wlc_he_color_collision_process_report(wlc_he_info_t *hei,
	uint8 colors[DOT11_EVENT_COLOR_COLLISION_REPORT_LEN])
{
	wlc_he_color_collision_t *cc = &hei->cc;
	int color;

	if (!cc->ap_report_handler_enab) {
		return;
	}

	for (color = 1; color < ARRAYSIZE(cc->detected_colors_cnt); color++) {
		if (isset(colors, color)) {
			wlc_he_color_collision_add(hei, color, HE_CC_REPORT_DETECT_TIMEOUT);
		}
	}

	/* The standard is not 100% clear on what a non-AP STA should report. It does not tell
	 * if the color which collided should be reported as well. However since the report is to
	 * be sent on collision we should mark it as reported.
	 */
	wlc_he_color_collision_add(hei, hei->bsscolor, HE_CC_REPORT_DETECT_TIMEOUT);
	cc->remote_collision_detect_cnt = HE_CC_REMOTE_COLL_CNT_INIT;

	/* Do not immediately change color, but give it a small delay to get possibly more color
	 * reports from other STAs
	 */
	if (cc->auto_enab && !cc->detect_report_cnt) {
		cc->detect_report_cnt = HE_CC_REPORT_DETECT_DELAY;
	}
}

/**
 * HE color collision watchdog handler
 *
 * @param ctx		handle to wlc_he_info_t context
 * @return		none
 */
static void
wlc_he_color_collision_wd(wlc_he_info_t *hei)
{
	wlc_he_color_collision_t *cc = &hei->cc;
	wl_color_event_t color_event;
	int color, j;
	bool change_color = FALSE;
	uint8 random;
	uint16 cls_cnt;

	if (!cc->ap_detect_enab && !cc->sta_detect_enab && !cc->auto_enab &&
		!cc->ap_report_handler_enab && !cc->event_enab) {
		return;
	}

	if (cc->create_event_cnt) {
		cc->create_event_cnt--;
		if (!cc->create_event_cnt) {
			wlc_he_color_collision_event(hei, &color_event);
		}
	}

	if (cc->remote_collision_detect_cnt) {
		cc->remote_collision_detect_cnt--;
	}

	if (cc->local_collision_detect_cnt) {
		cc->local_collision_detect_cnt--;
	}

	/* Handle ucode OBSS color collision detection count */
	if ((cc->ap_detect_enab) && (hei->wlc->main_ap_bsscfg != NULL)) {
		cls_cnt = wlc_read_shm(hei->wlc, M_COCLS_CNT(hei->wlc->hw));
		if (cls_cnt != cc->prev_cls_cnt) {
			if (((cls_cnt - cc->prev_cls_cnt) >= HE_CC_CLS_DETECT_THRSHLD) &&
				(!hei->bsscolor_disable) && (hei->update_bsscolor_counter < 2)) {
				wlc_he_color_collision_add(hei, hei->bsscolor,
					HE_CC_LOCAL_AP_DETECT_TIMEOUT);
				cc->local_collision_detect_cnt = HE_CC_LOCAL_COLL_CNT_INIT;
				if ((cc->auto_enab) && (!cc->detect_local_cnt)) {
					wlc_getrand(hei->wlc, &random, 1);
					cc->detect_local_cnt = 1 +
						(random % HE_CC_LOCAL_AP_DETECT_MAX_DELAY);
				}
			}
			cc->prev_cls_cnt = cls_cnt;
		}
	}

	if (cc->detect_backoff_cnt) {
		cc->detect_backoff_cnt--;
	}

	if (!cc->detected_colors_total) {
		cc->detect_report_cnt = 0;
		cc->detect_local_cnt = 0;
		return;
	}

	cc->detected_colors_total = 0;
	for (color = 1; color < ARRAYSIZE(cc->detected_colors_cnt); color++) {
		if (cc->detected_colors_cnt[color]) {
			cc->detected_colors_cnt[color]--;
			if (cc->detected_colors_cnt[color]) {
				cc->detected_colors_total++;
				if (color == hei->bsscolor) {
					if ((cc->detect_report_cnt == 1) ||
						(cc->detect_local_cnt == 1)) {
						change_color = 1;
					}
				}
			} else {
				if ((cc->event_enab) && (!cc->create_event_cnt)) {
					cc->create_event_cnt = HE_CC_EVENT_DELAY;
				}
			}
		}
	}

	/* When all colors are in use then skip color change */
	if ((change_color) && (cc->detected_colors_total < HE_BSSCOLOR_MAX_VALUE)) {
		cc->detect_report_cnt = 0;
		cc->detect_local_cnt = 0;
		/* pick a random color from the available colors. Use rand max color and sweep
		 * through array to find next available color. But, start at random index
		 * otherwise the lower colors have more chance to getting used.
		 */
		wlc_getrand(hei->wlc, &random, 1);
		color = (random % HE_BSSCOLOR_MAX_VALUE) + 1;
		while (cc->detected_colors_cnt[color]) {
			color++;
			if (color > HE_BSSCOLOR_MAX_VALUE) {
				color = 1;
			}
		}
		wlc_getrand(hei->wlc, &random, 1);
		j = (random % HE_BSSCOLOR_MAX_VALUE) + 1;
		while (j) {
			color++;
			if (color > HE_BSSCOLOR_MAX_VALUE) {
				color = 1;
			}
			while (cc->detected_colors_cnt[color]) {
				color++;
				if (color > HE_BSSCOLOR_MAX_VALUE) {
					color = 1;
				}
			}
			j--;
		}
		WL_HE_INFO(("%s Color collission detected, picking new color %d\n", __FUNCTION__,
			color));
		/* color = the color to pick. */
		wlc_he_configure_new_bsscolor(hei, color, hei->bsscolor_disable,
			hei->bsscolor_counter);
	}
	if (cc->detect_report_cnt) {
		cc->detect_report_cnt--;
	}
	if (cc->detect_local_cnt) {
		cc->detect_local_cnt--;
	}
}

/**
 * HETB RU26 chanspec change call back
 * If channel changes to non-DFS, stop monitoring
 * obss beacons and clear MHF6_AVOID_HETB_RU26
 * If channel changes to DFS, start monitoring
 * obss beacons
 *
 * @param wlc		pointer to wlc_info_t
 * @return		none
 */

void
wlc_he_chanspec_upd(wlc_info_t *wlc)
{
	uint16 val;
	if (!HE_ULMU_STA_ENAB(wlc->pub) || !STA_ACTIVE(wlc) ||
		SCAN_IN_PROGRESS(wlc->scan))
		return;
	if (!wlc_radar_chanspec(wlc->cmi, wlc->home_chanspec)) {
		/* Channel changed to non-DFS */
		if (wlc->hei->nbwru.obss_bcn_monitor_active) {
			wlc->hei->nbwru.obss_bcn_monitor_active = FALSE;
			wlc_mac_bcn_promisc_update(wlc->hei->wlc,
				BCNMISC_AXSTA_OBSS_NBWRU, FALSE);
#if defined(BCMDBG)
			WL_HE_INFO(("wl%d: %s: Clearing BCNMISC\n",
					wlc->pub->unit, __FUNCTION__));
#endif
		}
		if (!wlc->hei->nbwru.discard_nbwru_tf)
			return;
		wlc->hei->nbwru.discard_nbwru_tf = FALSE;
		if (!wlc->hw->clk)
			return;
		val = wlc_read_shm(wlc, M_HOST_FLAGS6(wlc->hw));
		wlc_write_shm(wlc, M_HOST_FLAGS6(wlc->hw),
				val & ~MHF6_AVOID_HETB_RU26);
#if defined(BCMDBG)
		WL_HE_INFO(("wl%d: %s: Clearing MHF6_AVOID_HETB_RU26\n",
			wlc->pub->unit, __FUNCTION__));
#endif
	} else {
		/* Channel changed to DFS */
		if (wlc->hei->nbwru.obss_bcn_monitor_active)
		      return;

		wlc->hei->nbwru.obss_bcn_monitor_active = TRUE;
		wlc_mac_bcn_promisc_update(wlc,	BCNMISC_AXSTA_OBSS_NBWRU, TRUE);
#if defined(BCMDBG)
		WL_HE_INFO(("wl%d: %s: Setting BCNMISC\n", wlc->pub->unit, __FUNCTION__));
#endif
	}
}

/**
 * HETB RU26 intolerance watchdog handler
 *
 * @param ctx		handle to wlc_he_info_t context
 * @return		none
 */
static void
wlc_he_avoid_hetb_ru26_wd(wlc_he_info_t *hei)
{
	uint elapsed;
	uint nowtime = 0;
	uint16 val;

	nowtime = hei->wlc->pub->now;
	elapsed = nowtime - hei->nbwru.ref_timestamp;
	/* Clear MHF6_AVOID_HETB_RU26 if last OBSS beacon with NBWRU intolerance
	 * was received more than DOT11_OBSS_NBRU_TOLR_TIME seconds ago.
	 */
	if (elapsed >= DOT11_OBSS_NBRU_TOLR_TIME) {
		hei->nbwru.discard_nbwru_tf = FALSE;
		if (!hei->wlc->hw->clk)
			return;
		val = wlc_read_shm(hei->wlc, M_HOST_FLAGS6(hei->wlc->hw));
		wlc_write_shm(hei->wlc, M_HOST_FLAGS6(hei->wlc->hw),
			val & ~MHF6_AVOID_HETB_RU26);
#if defined(BCMDBG)
		WL_HE_INFO(("wl%d: %s: Clearing MHF6_AVOID_HETB_RU26\n",
			hei->wlc->pub->unit, __FUNCTION__));
#endif
	}
}

/**
 * HE watchdog handler
 *
 * Registered as HE module watchdog callback. called once per second.
 * In use by bss color collision detection and handler
 *
 * @param ctx		handle to wlc_he_info_t context
 * @return		none
 */
static void
wlc_he_watchdog(void *ctx)
{
	wlc_he_info_t *hei = (wlc_he_info_t *)ctx;
	wlc_info_t* wlc = hei->wlc;
	int idx;
	wlc_bsscfg_t *cfg;

	wlc_he_color_collision_wd(hei);
	if (hei->nbwru.discard_nbwru_tf == TRUE) {
		wlc_he_avoid_hetb_ru26_wd(hei);
	}
	FOREACH_AS_STA(wlc, idx, cfg) {
#ifdef STA
		struct scb *scb;
		scb_he_t *he_scb;
		if ((scb = wlc_scbfind(wlc, cfg, &cfg->BSSID)) != NULL) {
			he_scb = SCB_HE(hei, scb);
			if ((he_scb == NULL) || !SCB_HE_CAP(scb) || !(he_scb->flags & SCB_HE_OMI))
				continue;
			if (he_scb->htc_retry_required == TRUE) {
				ASSERT(he_scb->send_htc_retries &&
					he_scb->send_htc_retries <= HE_SEND_HTC_RETRY_LIMIT);
				he_scb->htc_retry_required = FALSE;
				wlc_he_send_htc_indication(wlc, cfg, scb);
			}
		}
#endif /* STA */
	}
}

#endif /* WL11AX */
