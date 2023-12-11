/*
 * Proxd internal interface - burst manager
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
 * $Id: pdburst.c 810932 2022-04-19 20:14:14Z $
 */

#include <wlc_cfg.h>

#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <802.11.h>
#include <bcmevent.h>
#include <wlioctl.h>
#include <bcmwifi_channels.h>

#include <osl.h>
#include <wl_dbg.h>
#include <siutils.h>

#include <wlc_pub.h>
#include <wlc_rate.h>
#include <wlc_key.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wl_export.h>
#include <d11.h>
#include <wlc_cfg.h>
#include <wlc_hrt.h>
#include <wlc_rate.h>
#include <wlc_key.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scan_utils.h>
#include <wl_export.h>
#include <wlc_assoc.h>
#include <wlc_bmac.h>
#include <wlc_hw.h>
#include <wlc_hw_priv.h>
#include <hndpmu.h>
#include <wlc_pcb.h>
#include <wlc_lq.h>

#include <wlc_pdsvc.h>
#include <wlc_pddefs.h>
#include <wlc_pdmthd.h>

#include "pdsvc.h"
#include "pdftm.h"
#include "pdburst.h"
#include "pdftmpvt.h"

#include <phy_rxgcrs_api.h>
#include <phy_tof_api.h>
#include <phy_utils_api.h>
#include <phy_utils_reg.h>
#include <wlc_phyreg_ac.h>

#ifdef WL_RANGE_SEQ
#include <phy_ac_misc.h>
#endif /* WL_RANGE_SEQ */
#ifdef WL_PROXD_PHYTS_DEBUG
#include <wlc_macdbg.h>
#endif

/* #define TOF_DEBUG_UCODE 1 */
/* #define TOF_DEBUG_TIME */
/* #define TOF_DEBUG_TIME2 */
/* #define TOF_PROFILE */
/* #define TOF_KVALUE_CAL */

#define TOF_DFREE_SCAN		1
#define TOF_DFREE_TXDONE	2
#define TOF_DFREE_PWR		4
#define TOF_DFREE_INSVCCB	8
#define TOF_RX_PROCESS_MASK	1

#define TOF_43694_AVB_TXS       20000
#define TOF_43694_AVB_TXL       200000
#define TOF_43694_AVB_TXS_FIX   150000
#define TOF_43694_AVB_TXL_FIX   -10000

#define PHY_RXFE_DEBUG2_160IN160   0xffff
#define PHY_RXFE_DEBUG2_80IN80     0x0f0f
#define PHY_RXFE_DEBUG2_40IN40     0x0303
#define PHY_RXFE_DEBUG2_DEFAULT    0x0000

#if defined(TOF_DBG) || defined(TOF_COLLECT)
#include <sbchipc.h>
#endif /* TOF_DBG || TOF_COLLECT */

#ifdef TOF_PROFILE
#define TOF_DEBUG_TIME2
#endif

#define TOF_VER			1
#ifdef TOF_DEBUG_FTM
#define MDBG(x)  printf x /* dbg messages for non-timing sensetive code  */
#else
#define MDBG(x)
#endif /* TOF_DEBUG_FTM */

#ifdef WL_RANGE_SEQ
#ifdef FTM_SESSION_CHANSWT
#define BURST_NOCHANSWT(flags)		((flags) & WL_PROXD_SESSION_FLAG_NOCHANSWT)
#else
#define BURST_NOCHANSWT(flags)		TRUE
#endif /* FTM_SESSION_CHANSWT */
#endif /* WL_RANGE_SEQ */

#define BURST_HZ_PICO 1000 /* nano second resolution */
#define BURST_ERR_PICO (BURST_HZ_PICO >> 1) /* 0.5 nano sec */

struct pdburst_sm;
typedef struct pdburst_sm	pdburst_sm_t;

#define TS_METHOD_SAMPCAP 0x1u
#define TS_METHOD_AVB 0x2u
#define TS_METHOD_SEQ 0x4u

typedef struct ftmts {
	uint64	t1;
	uint64	t2;
	uint64	t3;
	uint64	t4;
	uint32	k;
	int32	gd;
	int32	adj;
	uint32	rspec;
	wl_proxd_rssi_t	rssi;
	bool	discard;
	uint8	tx_id;
	uint32	avbrx;
	uint32	avbtx;
	wl_proxd_snr_t  snr;
	wl_proxd_bitflips_t  bitflips;
	wl_proxd_phy_error_t tof_phy_error;
	uint8	tsmethod;
} ftmts_t;

typedef	struct tof_tslist {
	uint16	tscnt;
	ftmts_t	*tslist;
} tof_tslist_t;

struct pdburst;
typedef struct pdburst	pdburst_t;
#ifdef WL_RANGE_SEQ
#define MAX_COLLECT_COUNT	2
#else
#define MAX_COLLECT_COUNT	5
#endif /* WL_RANGE_SEQ */

typedef struct pdburst_collect {
	bool				remote_request;
	bool				remote_collect;
	int16				remote_cnt;
	int16				collect_cnt;
	int16				collect_size;
	uint32				*collect_buf;
	pdburst_t *			pdburstp;
	wl_proxd_collect_header_t	*collect_header;
	wl_proxd_collect_data_t		*collect;
	wl_proxd_collect_info_t		collect_info;
	uint32				*chan;
	uint8				ri_rr[FTM_TPK_RI_RR_LEN_SECURE_2_0];
	pdburst_config_t		*configp;
	wl_proxd_collect_method_t	collect_method;
} pdburst_collect_t;

#ifdef TOF_COLLECT
static pdburst_collect_t *pdburst_collect = NULL;
#endif /* TOF_COLLECT */

enum  {
	TOF_SEQ_NONE		= 0,
	TOF_SEQ_STARTED		= 1,
	TOF_SEQ_DONE		= 2,
	TOF_SEQ_LAST		= 3
};

typedef int8 pdburst_seq_state_t;

/* TOF method data object */
struct pdburst {
	void				*ctx; /* session ctx */
	const pdburst_callbacks_t	*svc_funcs;
	wlc_bsscfg_t			*bsscfg;
	wlc_info_t			*wlc;
	pdburst_sm_t			*sm;
	uint8				txcnt;
	uint8				rxcnt;
	uint8				measurecnt;
	uint8				totalfrmcnt;
	uint16				frame_type_cnt[FRAME_TYPE_NUM];
	uint8				adj_type_cnt[TOF_ADJ_TYPE_NUM];
	struct wl_timer			*timer;
	wlc_hrt_to_t			*duration_timer;
	wlc_hrt_to_t			*ftm_tx_timer;
	bool				ftm_tx_timer_active;
	bool				duration_timer_active;
	bool				timeractive;
	bool				caldone;
	uint16				tofcmd;
	tof_tslist_t			tof_tslist;
	uint64				tx_t1;
	uint64				tx_t4;
	int32				distance;
	uint32				meanrtt;
	uint64				Tq;
	uint32				oldavbrx;
	uint32				oldavbtx;
	uint32				chipnum;
	uint32				chiprev;
	uint16				shmemptr;
	struct ether_addr		allow_mac;
	int32				var3;
	wlc_bsscfg_t			*scanbsscfg;
	uint8				noscanengine;
	bool				lastburst;
	int8				rssi; /* RSSI of the last FTM packet */
	pdburst_config_t		*configp;
	wl_proxd_params_tof_tune_t	*tunep;
	uint8				phyver;
	uint8				frmcnt;
	int8				avgrssi;
	uint8				scanestarted;
	uint8				smstoped;
	bool				destroyed;
	uint8				delayfree;
	bool				seq_en;
	int32				seq_len;
	uint32				flags;
	uint16				sdrtt;
	ftmts_t				*lasttsp; /* previous burst time stamps */
	bool				seq_started;
	pdburst_collect_t		*collectp;
	tof_pbuf_t			tof_pbuf[TOF_PROFILE_BUF_SIZE];
	bool				channel_dumped;
	uint16				result_flags;

	/* num measurement frames recv from peer (ignoring retries) */
	uint8				num_meas;
	uint8				core;
	pdburst_seq_state_t		seq_state;
	uint8				*mf_buf;
	uint16				mf_buf_len;
	uint8				*mf_stats_buf;
	uint16				mf_stats_buf_len;
	bool				randmac_in_use;
	uint32				ftm_rx_rspec; /* valid only only initiator */
	uint32				ack_rx_rspec;
	uint8				is_valid_ts;
	uint64				phy_deaf_start;
	bool				deferred;
	uint8				pad[2];
	uint64				start_time;
	bool				avb_active;
	bool				meas_mode;
	bool				tof_reset;
	uint16				rxclass;
	uint16				rxclass_saved;
	bool				phyts_setup;
	bool				sampcap_method; /* samplecapture method for timestamping */
	bool				fallback_avb; /* fallback to avb method for timestamping */
	bool				phyts_meas; /* Filtered measurements are phyts or not */
	cint16				*sc_mem_ptr;
	uint32				sc_mem_size;
};

#define BURST_SEQ_EN(_bp) ((_bp)->seq_en != 0)
#define BURST_IS_VHTACK(_bp) (((_bp)->flags & WL_PROXD_SESSION_FLAG_VHTACK) != 0)
#define BURST_IS_HEACK(_bp) (((_bp)->flags & WL_PROXD_SESSION_FLAG_HEACK) != 0)

#ifdef WL_RANGE_SEQ
#define BURST_SEQ_EN_SET(_bp, v) ((_bp)->seq_en = (v))
#define BURST_FLAG_SEQ_EN(_bp) (((_bp)->flags & WL_PROXD_SESSION_FLAG_SEQ_EN) != 0)
#endif /* WL_RANGE_SEQ */
#define BURST_HWADJ_EN(_tp) ((_tp)->hw_adj != 0)
#define BURST_SWADJ_EN(_tp) ((_tp)->sw_adj != 0)

/* RSSI Proximity state machine parameters */
struct pdburst_sm {
	uint8			tof_mode;
	uint8			tof_txcnt;
	uint8			tof_rxcnt;
	uint8			tof_state;
	struct ether_addr	tof_peerea;
	struct ether_addr	tof_selfea;
	pdburst_t		*tof_obj;
	wl_proxd_status_t	tof_reason;
	uint8			tof_retrycnt;
	uint8			tof_txpktcnt;
	bool			tof_txvht;
	bool			tof_rxvht;
	uint16			phyctl0;
	uint16			phyctl1;
	uint16			phyctl2;
	uint16			lsig;
	uint16			vhta0;
	uint16			vhta1;
	uint16			vhta2;
	uint16			vhtb0;
	uint16			vhtb1;
	uint16			ampductl;
	uint16			ampdudlim;
	uint16			ampdulen;
	uint8			tof_legacypeer;
	uint8			tof_followup;
	uint8			tof_dialog;
	uint32			toa; /* timestamp of arrival of FTM or ACK */
	uint32			tod; /* timestamp of departure of FTM or ACK */
	uint16			htsig0;
	uint16			htsig1;
	uint16			hesig0;
	uint16			hesig1;
	uint16			hesig2;
	uint16			hesigext0;
	uint16			hesigext1;
};

enum tof_type {
	TOF_TYPE_REQ_END		= 0,
	TOF_TYPE_REQ_START		= 1,
	TOF_TYPE_MEASURE_END		= 2,
	TOF_TYPE_MEASURE		= 3,
	TOF_TYPE_COLLECT_REQ_END	= 4,
	TOF_TYPE_COLLECT_REQ_START	= 5,
	TOF_TYPE_COLLECT_DATA_END	= 6,
	TOF_TYPE_COLLECT_DATA		= 7,
	TOF_TYPE_LAST			= 8
};

enum tof_event {
	TOF_EVENT_WAKEUP	= 0,
	TOF_EVENT_RXACT		= 1,
	TOF_EVENT_TMO		= 2,
	TOF_EVENT_ACKED		= 3,
	TOF_EVENT_NOACK		= 4,
	TOF_EVENT_WAITMTMO	= 5,
	TOF_EVENT_COLLECT_REQ	= 6,
	TOF_EVENT_LAST		= 7
};

enum tof_ret {
	TOF_RET_SLEEP	= 0,
	TOF_RET_ALIVE	= 1,
	TOF_RET_IGNORE	= 2,
	TOF_RET_END		= 3
};

typedef struct pdburst_data {
	uint8			tof_type;
	int8			tof_rssi;
	uint32			tof_rspec;
	struct ether_addr	tof_srcea;
	struct ether_addr	tof_dstea;
} pdburst_data_t;

#ifdef TOF_COLLECT_REMOTE

#include<packed_section_start.h>

/* Declarations of collect debug header */
typedef	BWL_PRE_PACKED_STRUCT pdburst_collect_frm {
	uint8	category;
	uint8	OUI[3];
	uint8	type;		/* BRCM_FTM_VS_AF_TYPE */
	uint8	subtype;	/* BRCM_FTM_VS_COLLECT_SUBTYPE */
	uint8	tof_type;	/* packet type */
	uint8	index;		/* data index: 0 is header, others is data */
	uint16	length;		/* data length */
	uint8	data[1];	/* collect data */
} BWL_POST_PACKED_STRUCT pdburst_collect_frm_t;

#include<packed_section_end.h>

#endif /* TOF_COLLECT_REMOTE */

#define PROTOCB(_burst, _func, _args) (((_burst)->svc_funcs && \
	(_burst)->svc_funcs->_func) ? (*(_burst)->svc_funcs->_func)_args :\
	BCME_UNSUPPORTED)

#ifdef TOF_DEBUG_TIME2
static uint32 tsf_start, tsf_hi, tsf_scanstart, tsf_txreq, tsf_rxack;
static uint32 tsf_rxm, tsf_tmo, tsf_lastm, tsf_confirm;
#endif

static int pdburst_send(pdburst_sm_t *sm, struct ether_addr *da, uint8 type);

static int pdburst_sm(pdburst_sm_t *sm, int event, const pdburst_tsinfo_t *protp,
	int paramlen, pdburst_data_t *datap);
static void pdburst_measure(pdburst_t *tofobj, int cmd);
static int pdburst_confirmed(pdburst_sm_t *sm, wl_proxd_status_t reason);
static int pdburst_target_done(pdburst_sm_t *sm, wl_proxd_status_t reason);
static void
pdburst_mf_stats_init_event(
	wl_proxd_event_t * event,
	wl_proxd_session_id_t sid);
static void pdburst_send_mf_stats_event(
	pdburst_t * burstp);
#if defined(TOF_COLLECT) || defined(TOF_COLLECT_REMOTE)
static void
pdburst_collect_prep_header(pdburst_collect_t *collectp,
	wl_proxd_collect_header_t *header);
#endif /* TOF_COLLECT || TOF_COLLECT_REMOTE */
#ifdef TOF_COLLECT
static int pdburst_collect_generic_event(pdburst_t *burstp);
static int pdburst_collect_event(pdburst_t *burstp);
#endif /* TOF_COLLECT */

#ifdef TOF_COLLECT_INLINE
typedef enum tof_collect_inline_type
{
	TOF_COLLECT_INLINE_HEADER = 1,
	TOF_COLLECT_INLINE_FRAME_INFO = 2,
	TOF_COLLECT_INLINE_FRAME_INFO_CHAN_DATA = 3,
	TOF_COLLECT_INLINE_RESULTS = 4,
	TOF_COLLECT_INLINE_MAX
} tof_collect_inline_type_t;

#define TOF_COLLECT_INLINE_HEADER_INFO_VER		TOF_COLLECT_INLINE_HEADER_INFO_VER_1
#define TOF_COLLECT_INLINE_RESULTS_VER		TOF_COLLECT_INLINE_RESULTS_VER_1
#define TOF_COLLECT_INLINE_FRAME_INFO_VER		TOF_COLLECT_INLINE_FRAME_INFO_VER_2

#endif /* TOF_COLLECT_INLINE */

#ifdef WL_PROXD_UCODE_TSYNC
static void pdburst_clear_ucode_ack_block(wlc_info_t *wlc);
#endif /* WL_PROXD_UCODE_TSYNC */

/* convert tof frame type to pdburst frame type */
pdburst_frame_type_t pdburst_get_ftype(uint8 type)
{
	pdburst_frame_type_t ftype;
	ASSERT(type <= TOF_TYPE_MEASURE);
	if (type > TOF_TYPE_MEASURE) {
		WL_ERROR(("%s()- Unknown tof type %d\n", __FUNCTION__, type));
	}

	if (type == TOF_TYPE_REQ_START || type == TOF_TYPE_REQ_END) {
		ftype = PDBURST_FRAME_TYPE_REQ;
	} else {
		ftype = PDBURST_FRAME_TYPE_MEAS; /* both TOF_TYPE_MEASURE/MEASURE_END */
	}
	return ftype;
}

/* API to get the chanspec from OTA rspec & configured chanspec
* Basically...configured BW and over the air frame/ack BW can be different.
* so we update the BW of configured chanspec with OTA ones
*/
static chanspec_t
pdburst_get_chspec_from_rspec(pdburst_t *burstp, ratespec_t rspec)
{
	chanspec_t config_chanspec = burstp->configp->chanspec;
	chanspec_t chanspec = 0;

	if ((rspec & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_160MHZ) {
		chanspec = (config_chanspec & ~(WL_CHANSPEC_BW_MASK)) |
			WL_CHANSPEC_BW_160;
	} else if ((rspec & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_80MHZ) {
		chanspec = (config_chanspec & ~(WL_CHANSPEC_BW_MASK)) |
			WL_CHANSPEC_BW_80;
	} else if ((rspec & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_40MHZ) {
		chanspec = (config_chanspec & ~(WL_CHANSPEC_BW_MASK)) |
			WL_CHANSPEC_BW_40;
	} else if ((rspec & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_20MHZ) {
		chanspec = (config_chanspec & ~(WL_CHANSPEC_BW_MASK)) |
			WL_CHANSPEC_BW_20;
	}

	return chanspec;
}

#ifdef WL_PROXD_UCODE_TSYNC
/* API to derive ACK rspec from TXStatus ack block..
* Refer below confluence for details
* http://confluence.broadcom.com/display/WLAN/NewUcodeInterfaceForProxdFeature
*/
static ratespec_t
pdburst_get_tgt_ackrspec(pdburst_t *burstp, uint16 ucodeack)
{
	uint8 bw, type, leg_rate;
	ratespec_t ackrspec = 0;
	ucodeack = (ucodeack & FTM_TXSTATUS_ACK_RSPEC_BLOCK_MASK);
	bw = (ucodeack >> FTM_TXSTATUS_ACK_RSPEC_BW_SHIFT) &
		FTM_TXSTATUS_ACK_RSPEC_BW_MASK;

	if (bw == FTM_TXSTATUS_ACK_RSPEC_BW_20) {
		ackrspec |= WL_RSPEC_BW_20MHZ;
	} else if (bw == FTM_TXSTATUS_ACK_RSPEC_BW_40) {
		ackrspec |= WL_RSPEC_BW_40MHZ;
	} else if (bw == FTM_TXSTATUS_ACK_RSPEC_BW_80) {
		ackrspec |= WL_RSPEC_BW_80MHZ;
	} else if (bw == FTM_TXSTATUS_ACK_RSPEC_BW_160) {
		ackrspec |= WL_RSPEC_BW_160MHZ;
	}

	type = (ucodeack >> FTM_TXSTATUS_ACK_RSPEC_TYPE_SHIFT) &
		FTM_TXSTATUS_ACK_RSPEC_TYPE_MASK;

	leg_rate = (ucodeack >> FTM_TXSTATUS_ACK_RSPEC_RATE_SHIFT) &
		FTM_TXSTATUS_ACK_RSPEC_RATE_MASK;
#ifdef TOF_DEBUG_UCODE
	FTM_ERR(("TXS: ucodeack 0x%04x bw 0x%02x type 0x%02x leg_rate 0x%02x\n",
		ucodeack, bw, type, leg_rate));
#endif
	if (type == FTM_TXSTATUS_ACK_RSPEC_TYPE_LEG) {
		if (FTM_TXSTATUS_ACK_RSPEC_RATE_6M(leg_rate)) {
			ackrspec |= WLC_RATE_6M;
		} else { /* any non-6mps is fine */
			ackrspec |= WLC_RATE_24M;
		}
	} else if (type == FTM_TXSTATUS_ACK_RSPEC_TYPE_HT) {
		/* TODO..add non-mcs0 support */
		ackrspec |= HT_RSPEC(0);
	} else if (type == FTM_TXSTATUS_ACK_RSPEC_TYPE_VHT) {
		ackrspec |= VHT_RSPEC(FTM_MCS, FTM_NSS);
	} else if (type == FTM_TXSTATUS_ACK_RSPEC_TYPE_HE) {
		ackrspec |= HE_RSPEC(FTM_MCS, FTM_NSS);
	} else if (type == FTM_TXSTATUS_ACK_RSPEC_TYPE_CCK) {
		ASSERT(0);
	}

#ifdef TOF_DEBUG_UCODE
	FTM_ERR(("\n ackrspec 0x%08x\n", ackrspec));
#endif

	return ackrspec;
}
#endif /* WL_PROXD_UCODE_TSYNC */

/* API to get subband index */
static uint8
pdburst_get_subband_idx(pdburst_t *burstp, ratespec_t rspec)
{
	wlc_info_t *wlc = burstp->wlc;
	uint8 idx = 0;
	chanspec_t chanspec = pdburst_get_chspec_from_rspec(burstp, rspec);
	chanspec_t radio_chanspec = phy_utils_get_chanspec(WLC_PI(wlc));

	if (CHSPEC_BW(chanspec) !=
		CHSPEC_BW(radio_chanspec)) {
		/* initiator and target bandwidth is different */
		if (CHSPEC_IS160(radio_chanspec)) {
			if (CHSPEC_IS80(chanspec)) {
				idx = WL_PROXD_160M_80M;
			} else if (CHSPEC_IS40(chanspec)) {
				idx = WL_PROXD_160M_40M;
			} else {
				idx = WL_PROXD_160M_20M;
			}
		} else if (CHSPEC_IS80(radio_chanspec)) {
			/* target is 80 */
			if (CHSPEC_IS40(chanspec)) {
				idx = WL_PROXD_80M_40M;
			} else {
				idx = WL_PROXD_80M_20M;
			}
		} else if (CHSPEC_IS40(radio_chanspec)) {
			if (CHSPEC_IS20(chanspec)) {
				idx = WL_PROXD_40M_20M;
			}
		}
	}

	return idx;
}

/* get K value */
static uint32
pdburst_get_kval(pdburst_t *burstp, bool initiator, bool seq_en, bool sampcap_en)
{
	wlc_info_t *wlc = burstp->wlc;
	uint32 k = 0u;
	ratespec_t ftm_ratespec, ackrspec, rspec;
	chanspec_t ftm_chanspec;
	chanspec_t ackchanspec = 0;
	uint8 bw_idx = 0, flags = 0;
	uint16 rate_idx = 0;
	chanspec_t radio_chanspec;
	uint32 *kip, *ktp;

	/* FTM config values */
	ftm_ratespec = burstp->configp->ratespec;
	ftm_chanspec = burstp->configp->chanspec;
	radio_chanspec = phy_utils_get_chanspec(WLC_PI(wlc));

	BCM_REFERENCE(ftm_chanspec);
	BCM_REFERENCE(ackchanspec);
	FTM_TRACE(("Radio-chanpec %x, burst-config-chanspec %x\n",
		radio_chanspec, ftm_chanspec));

	/* overwrite with actual OTA values & derive ack ratespec/chanspec */
	if (initiator) {
		if ((burstp->flags & WL_PROXD_SESSION_FLAG_ONE_WAY) ||
			(burstp->sm->tof_legacypeer == TOF_LEGACY_AP)) {
			return 0;
		}
		ftm_ratespec = burstp->ftm_rx_rspec;
		ftm_chanspec = pdburst_get_chspec_from_rspec(burstp, ftm_ratespec);
		if (RSPEC_ISHE(ftm_ratespec)) {
			ackrspec = HE_RSPEC(FTM_MCS, FTM_NSS) | (ftm_ratespec & WL_RSPEC_BW_MASK);
		} else if (BURST_IS_VHTACK(burstp) && RSPEC_ISVHT(ftm_ratespec)) {
			ackrspec = VHT_RSPEC(FTM_MCS, FTM_NSS) | (ftm_ratespec & WL_RSPEC_BW_MASK);
		} else if (RSPEC_ISHT(ftm_ratespec)) {
			ackrspec = HT_RSPEC(0) | (ftm_ratespec & WL_RSPEC_BW_MASK);
		} else {
			/* We will transmit ACK same as the FTM frame and upto 24Mbps */
			uint8 leg_ack_rate = (ftm_ratespec & WL_RSPEC_LEGACY_RATE_MASK);
			if (leg_ack_rate > WLC_RATE_24M) {
				leg_ack_rate = WLC_RATE_24M;
			}
			ackrspec = leg_ack_rate | (ftm_ratespec & WL_RSPEC_BW_MASK);
		}
		if (!sampcap_en) {
			k = burstp->tunep->Ki;
		}
		rspec = ftm_ratespec;
	} else {
		ackchanspec = pdburst_get_chspec_from_rspec(burstp,
			burstp->ack_rx_rspec);
		ackrspec = burstp->ack_rx_rspec;
		if (!sampcap_en) {
			k = burstp->tunep->Kt;
		}
		rspec = burstp->ack_rx_rspec;
	}

	bw_idx = pdburst_get_subband_idx(burstp, rspec);
	rate_idx = proxd_get_ratespec_idx(ftm_ratespec, ackrspec);
	if (!k) {
#ifdef WL_PROXD_PHYTS
		/* Update flag to get AVB/SEQ or PHYTS kvalue */
		/* Also, override lower bits for bw_idx */
		if (sampcap_en) {
			flags = WL_PROXD_PHTS_MASK;
		} else if (seq_en) {
			flags = WL_PROXD_SEQEN;
		} else
#endif /* WL_PROXD_PHYTS */
		{
			flags = 0u;
		}
		flags = flags | bw_idx;
		if (initiator) {
			kip = &k;
			ktp = NULL;
		} else {
			kip = NULL;
			ktp = &k;
		}
		wlc_phy_kvalue(WLC_PI(wlc), radio_chanspec, rate_idx, kip, ktp, flags);

		if (initiator) {
			FTM_TRACE(("ftm-cspec:%x k:%d ftm-rspec:%x idx:%d rate-idx:%d\n",
				ftm_chanspec, k, ftm_ratespec, bw_idx, rate_idx));
		} else {
			FTM_TRACE(("ack-cspec:%x cspec:%x k:%d ack-rspec:%x idx:%d rate-idx:%d\n",
				ackchanspec, radio_chanspec, k, ackrspec, bw_idx, rate_idx));
		}
	} else {
		if (initiator) {
			FTM_TRACE(("ftm-cspec:%x k:%d ftm-rspec:%x idx:%d rate-idx:%d\n",
				ftm_chanspec, k, ftm_ratespec, bw_idx, rate_idx));
		} else {
			FTM_TRACE(("ack-cspec:%x cspec:%x k:%d ack-rspec:%x idx:%d rate-idx:%d\n",
				ackchanspec, radio_chanspec, k, ackrspec, bw_idx, rate_idx));
		}
	}

	return k;
}

/* Get total frame count */
static uint8 pdburst_total_framecnt(pdburst_t *burstp)
{
	if (BURST_SEQ_EN(burstp))
		return 0;
	if (burstp->tunep->totalfrmcnt)
		return burstp->tunep->totalfrmcnt;
	if (burstp->totalfrmcnt)
		return burstp->totalfrmcnt;
	return 0;
}

/* FTM ACK phy ctl initialization */
static void
pdburst_tof_ack_phyctl_init(pdburst_t *burstp, uint8 dot11_bw)
{
	pdburst_sm_t *sm = burstp->sm;
	ratespec_t ratespec = burstp->configp->ratespec;
	wlc_info_t *wlc	= burstp->wlc;

	if (D11REV_GE(burstp->wlc->hw->corerev, 80)) {
		if (RSPEC_ISHE(ratespec)) {
			sm->phyctl0 = PHY_TXC_FT_HE | (HE_FMT_HESU << D11_HEFMT_SHIFT);
		} else if (RSPEC_ISVHT(ratespec)) {
			sm->phyctl0 = PHY_TXC_FT_VHT;
		} else if (RSPEC_ISHT(ratespec)) {
			sm->phyctl0 = PHY_TXC_FT_HT;
		}

		sm->phyctl0 = sm->phyctl0 | D11_REV80_PHY_TXC_NON_SOUNDING;
		sm->phyctl1 = dot11_bw & PHY_TXC1_BW_MASK;
		sm->phyctl2 = (1u << (burstp->core));
	} else {
		sm->phyctl0 = (dot11_bw << 14u) | 7u | (1u << (burstp->core + 6u));
		sm->phyctl1 = 0;
		sm->phyctl2 = 0;
	}

#ifdef TOF_DEBUG_UCODE
	TOF_PRINTF(("sm->phyctl0 0x%08x, sm->phyctl1 0x%08x, sm->phyctl2 0x%08x, dot11_bw 0x%x\n",
		sm->phyctl0, sm->phyctl1, sm->phyctl2, dot11_bw));
#endif

	/* Update BW and core information in ctl1 and ctl2 by default */
	wlc_write_shm(wlc, M_TOF_PHYCTL1(wlc), sm->phyctl1);
	wlc_write_shm(wlc, M_TOF_PHYCTL2(wlc), sm->phyctl2);
}

/* FTM ACK he sig params initialization */
static void
pdburst_tof_ack_he_init(pdburst_t *burstp, int dot11_bw)
{
	pdburst_sm_t *sm = burstp->sm;
	wlc_info_t *wlc	= burstp->wlc;
	int he_sig_a1 = 0, he_sig_a2 = 0;

	burstp->flags |= WL_PROXD_SESSION_FLAG_HEACK;

	he_sig_a1 = HE_SIGA_FORMAT_HE_SU | HE_SIGA_RESERVED_PLCP0
		| ((dot11_bw << HESU_SIGA_BW_SHIFT) & HESU_SIGA_BW_MASK)
		|  HE_SIGA_2x_LTF_GI_1_6us_VAL;
	he_sig_a2 = HE_SIGA_RESERVED_PLCP1; // Reserved bit required

	sm->hesig0 = (he_sig_a1 & 0xffff);
	sm->hesig1 = ((he_sig_a2 & 0xff) << 8) | ((he_sig_a1 >> 16) & 0xff);
	sm->hesig2 = (he_sig_a2 & 0xfff);

#ifdef TOF_DEBUG_UCODE
	TOF_PRINTF(("sm->hesig0 0x%08x, sm->hesig1 0x%08x, sm->hesig2 0x%08x, dot11_bw 0x%x\n",
		sm->hesig0, sm->hesig1, sm->hesig2, dot11_bw));
#endif

	wlc_write_shm(wlc, M_TOF_HE_PHYCTL0(wlc), sm->phyctl0);
	wlc_write_shm(wlc, M_TOF_HE_PHYCTL1(wlc), sm->phyctl1);
	wlc_write_shm(wlc, M_TOF_HE_PHYCTL2(wlc), sm->phyctl2);
	wlc_write_shm(wlc, M_TOF_HESIG0(wlc), sm->hesig0);
	wlc_write_shm(wlc, M_TOF_HESIG1(wlc), sm->hesig1);
	wlc_write_shm(wlc, M_TOF_HESIG2(wlc), sm->hesig2);
}

/* FTM ACK vht sig params initialization */
static void pdburst_tof_ack_vht_init(pdburst_t *burstp, int dot11_bw)
{
	pdburst_sm_t *sm = burstp->sm;
	wlc_info_t *wlc	= burstp->wlc;
	int vht_sig_a1 = 0, vht_sig_a2 = 0;

	vht_sig_a1 = dot11_bw | (1u << 2u) | (63u << 4u) | (1u << 23u);
	vht_sig_a2 = (1u << 9u);
	sm->vhta0 = (vht_sig_a1 & 0xffffu);
	sm->vhta1 = ((vht_sig_a2 & 0xffu) << 8u) | ((vht_sig_a1 >> 16u) & 0xffu);
	sm->vhta2 = ((vht_sig_a2 >> 8u) & 0xffffu);

#ifdef TOF_DEBUG_UCODE
	TOF_PRINTF(("sm->vhta0 0x%08x, sm->vhta1 0x%08x, sm->vhta2 0x%08x, dot11_bw 0x%x\n",
		sm->vhta0, sm->vhta1, sm->vhta2, dot11_bw));
	TOF_PRINTF(("vht_sig_a1 0x%08x, vht_sig_a2 0x%08x\n",
		vht_sig_a1, vht_sig_a2));
#endif

	wlc_write_shm(wlc, M_TOF_PHYCTL0(wlc), sm->phyctl0);
	wlc_write_shm(wlc, M_TOF_PHYCTL1(wlc), sm->phyctl1);
	wlc_write_shm(wlc, M_TOF_PHYCTL2(wlc), sm->phyctl2);
	wlc_write_shm(wlc, M_TOF_VHTA0(wlc), sm->vhta0);
	wlc_write_shm(wlc, M_TOF_VHTA1(wlc), sm->vhta1);
	wlc_write_shm(wlc, M_TOF_VHTA2(wlc), sm->vhta2);
}

#define TOF_HT_ACK_LEN		0x12u
#define TOF_HT_SIG1_SHIFT	8u

/* FTM ACK ht sig params initialization */
static void
pdburst_tof_ack_ht_init(pdburst_t *burstp, int dot11_bw)
{
	pdburst_sm_t *sm = burstp->sm;
	wlc_info_t *wlc	= burstp->wlc;

	sm->htsig0 = ((dot11_bw << HT_SIG1_CBW_SHIFT) & HT_SIG1_CBW)
			| (TOF_HT_ACK_LEN << HT_SIG1_HT_LENGTH_SHIFT);
	sm->htsig1 = (HT_SIG2_SMOOTHING | HT_SIG2_NOT_SOUNDING |
		HT_SIG2_RESERVED | HT_SIG2_AGGREGATION) << TOF_HT_SIG1_SHIFT;

#ifdef TOF_DEBUG_UCODE
	TOF_PRINTF(("sm->htsig0 0x%08x, sm->htsig1 0x%08x, dot11_bw 0x%x\n",
		sm->htsig0, sm->htsig1, dot11_bw));
#endif

	wlc_write_shm(wlc,  M_TOF_HT_PHYCTL0(wlc), sm->phyctl0);
	wlc_write_shm(wlc,  M_TOF_HT_PHYCTL1(wlc), sm->phyctl1);
	wlc_write_shm(wlc,  M_TOF_HT_PHYCTL2(wlc), sm->phyctl2);
	wlc_write_shm(wlc,  M_TOF_HTSIG0(wlc), sm->htsig0);
	wlc_write_shm(wlc,  M_TOF_HTSIG1(wlc), sm->htsig1);
	wlc_write_shm(wlc,  M_TOF_HTSIG2(wlc), 0x0);
}

/* FTM ACK setup */
static void
pdburst_tof_ack_init(pdburst_t *burstp)
{
	chanspec_t chanspec = burstp->configp->chanspec;
	ratespec_t ratespec = burstp->configp->ratespec;
	int	dot11_bw;

	if (burstp->ftm_rx_rspec != 0) {
		chanspec = pdburst_get_chspec_from_rspec(burstp, burstp->ftm_rx_rspec);
		ratespec = burstp->ftm_rx_rspec;
	}

	if (CHSPEC_IS160(chanspec)) {
		dot11_bw = D11_REV80_PHY_TXC_BW_160MHZ;
	} else if (CHSPEC_IS80(chanspec)) {
		dot11_bw = D11_REV80_PHY_TXC_BW_80MHZ;
	} else if (CHSPEC_IS40(chanspec)) {
		dot11_bw = D11_REV80_PHY_TXC_BW_40MHZ;
	} else if (CHSPEC_IS20(chanspec)) {
		dot11_bw = D11_REV80_PHY_TXC_BW_20MHZ;
	} else {
		TOF_PRINTF(("pdburst_tof_ack_init: Unsupported BW\n"));
		return;
	}

	pdburst_tof_ack_phyctl_init(burstp, dot11_bw);

	if (RSPEC_ISHE(ratespec)) {
		pdburst_tof_ack_he_init(burstp, dot11_bw);
	} else if (RSPEC_ISVHT(ratespec)) {
		pdburst_tof_ack_vht_init(burstp, dot11_bw);
	} else if (RSPEC_ISHT(ratespec)) {
		pdburst_tof_ack_ht_init(burstp, dot11_bw);
	} else {
		return;
	}
}

#ifdef TOF_SEQ_DBG
int gTofSeqCnt = 0;
#endif

/* Called when entering/exiting tof measurement mode */
static void pdburst_hw(pdburst_t *burstp, bool enter, bool tx)
{
	wlc_info_t *wlc = burstp->wlc;

#ifdef TOF_SEQ_DBG
	gTofSeqCnt = 0;
#endif

	burstp->meas_mode = enter;
	wlc_phy_tof(WLC_PI(wlc), enter, tx, burstp->tunep->hw_adj, BURST_SEQ_EN(burstp),
		(int)burstp->core, burstp->tunep->emu_delay);
}

/* Retrieve the channel information */
#define IS_HE(FT) (FT == FT_HE)
static int
pdburst_rtd_adj_retrieve_channel(pdburst_t *burstp, tof_rtd_adj_params_t *params,
		int frame_bw, int frame_type,  int *n_out, bool *hw_adj_en,
		bool *sw_adj_en, uint32* p_collect_data)
{
	int adj_err = BCME_OK;
	wlc_info_t *wlc = burstp->wlc;

	if (BURST_SEQ_EN(burstp)) {
		uint32 chan_size = 0;
		uint32* tmpR = NULL;
		int32* tmpH = NULL;

		frame_bw = (frame_bw & 0xffff);
		chan_size = (K_TOF_COLLECT_CHAN_SIZE >> (2 - frame_bw));
		TOF_PRINTF(("frame_bw = %d, chan_size = %d.\n", frame_bw, chan_size));
		if (!burstp->collectp) {
			TOF_PRINTF(("Allocating temp mem ----->\n"));
			tmpR = (uint32*)MALLOCZ(burstp->wlc->osh,
					(PHY_CORE_MAX + 1) * chan_size * sizeof(uint32));
			if (tmpR == NULL) {
				TOF_PRINTF(("*** Malloc error for channel estimates.*** \n"));
				return BCME_NOMEM;
			}
		} else {
			tmpR = (uint32 *) (burstp->collectp->chan);
		}
		if (tmpR != NULL) {
			adj_err = phy_tof_chan_freq_response_api(WLC_PI(burstp->wlc),
				chan_size, CORE0_K_TOF_H_BITS, tmpH, NULL, tmpR,
				TRUE, 1, TRUE, FALSE, IS_HE(frame_type));
			burstp->channel_dumped = TRUE;
			if (adj_err == BCME_NOCLK) {
				TOF_PRINTF(("wlc_hw clk %d\n", burstp->wlc->hw->clk));
			}
		}

		if (!burstp->collectp) {
			MFREE(burstp->wlc->osh, tmpR,
				(PHY_CORE_MAX + 1) * chan_size * sizeof(uint32));
			tmpR = NULL;
		}

#ifdef TOF_DBG_SEQ_PHY_SEC
		TOF_PRINTF(("%s : channel dump \n", __FUNCTION__));
#endif /* TOF_DBG_SEQ_PHY_SEC */
		if (adj_err) {
			TOF_PRINTF(("error = %d\n", adj_err));
		}
		*hw_adj_en = FALSE;
		*sw_adj_en = FALSE;
	} else {
		if (*hw_adj_en) {
			/* For HW adjust method */
			adj_err = wlc_phy_chan_mag_sqr_impulse_response(WLC_PI(wlc),
				frame_type, params->w_len, params->w_offset, CHNSM_K_TOF_H_BITS,
				params->H, (int *)&params->gd_ns, p_collect_data, burstp->shmemptr);
			if (adj_err == BCME_OK) {
				params->w_ext = params->H;
#ifdef TOF_COLLECT
				n_out = params->w_len + K_TOF_COLLECT_H_PAD;
#endif
			} else {
				*hw_adj_en = FALSE;
			}
		} else if (*sw_adj_en) {
			/* For SW adjust method */
			adj_err = phy_tof_chan_freq_response_api(WLC_PI(wlc), params->buf_len,
				CORE0_K_TOF_H_BITS, params->H, NULL, p_collect_data,
				TRUE, 1, FALSE, params->tdcs_en, IS_HE(frame_type));
			if (adj_err == BCME_OK) {
#ifdef TOF_COLLECT
				n_out = params->buf_len;
#endif
			} else {
				*sw_adj_en = FALSE;
			}
		}
	}
	return adj_err;
}

/* Compute the buffer length */
static void
pdburst_rtd_adj_buf_len(pdburst_t *burstp,  tof_rtd_adj_params_t *params, uint16 nfft)
{
	wlc_info_t *wlc = burstp->wlc;
	chanspec_t radio_chanspec = phy_utils_get_chanspec(WLC_PI(wlc));

	params->buf_len_extra = 0;
	/* It is used for allocating memory for channel/LTFs */
	if (BURST_SEQ_EN(burstp)) {
		/* Allocate twice the memory for two windows */
		params->buf_len = 2 * nfft;
#ifdef TOF_SEQ_20MHz_BW_512IFFT
		/* Allocate extra memory for 512IFFT */
		if (CHSPEC_IS20(radio_chanspec)) {
			params->buf_len_extra = 384;
		}
#endif /* TOF_SEQ_20MHz_BW_512IFFT */
	} else {
		if (CHSPEC_IS160(radio_chanspec))
			params->buf_len = 512;
		else if (CHSPEC_IS80(radio_chanspec))
			params->buf_len = 256;
		else if (CHSPEC_IS40(radio_chanspec))
			params->buf_len = 128;
		else {
			params->buf_len = 64;
		}
	}
	params->buf_len_extra +=  16;
}

/* Sampling time(Ts): In q3 format U(6,3) */
#define TS_20MHZ_NS (50u << 3)	/* 50ns */
#define TS_40MHZ_NS (TS_20MHZ_NS >> 1) /* 25ns */
#define TS_80MHZ_NS (TS_20MHZ_NS >> 2) /* 12.5ns */
#define TS_160MHZ_NS (TS_20MHZ_NS >> 3) /* 6.25ns */

static void
pdburst_rtd_adj_upd_h_ts(tof_rtd_adj_params_t *params)
{
	int bw = params->bw;

	if (bw == TOF_BW_160MHZ) {
		params->h_ts = TS_160MHZ_NS;
	} else if (bw == TOF_BW_80MHZ) {
		params->h_ts = TS_80MHZ_NS;
	} else if (bw == TOF_BW_40MHZ) {
		params->h_ts = TS_40MHZ_NS;
	} else if (bw == TOF_BW_20MHZ) {
		params->h_ts = TS_20MHZ_NS;
	} else {
		TOF_PRINTF(("Unsupported bw\n"));
	}
}
static void
pdburst_rtd_adj_upd_fp_params(pdburst_t *burstp, tof_rtd_adj_params_t *params, int frame_bw)
{
	wlc_info_t *wlc = burstp->wlc;

	/* First path detection parameters */
	params->w_len = burstp->tunep->w_len[frame_bw];
	params->w_offset = burstp->tunep->w_offset[frame_bw];
	if (BURST_SEQ_EN(burstp)) {
		if (frame_bw == TOF_BW_20MHZ_INDEX_V2) {
			if (CHSPEC_IS5G(phy_utils_get_chanspec(WLC_PI(wlc)))) {
				params->thresh_scale[0] = burstp->tunep->seq_5g20.N_tx_scale;
			params->thresh_log2[0] = burstp->tunep->seq_5g20.N_tx_log2;
			params->thresh_scale[1] = burstp->tunep->seq_5g20.N_rx_scale;
			params->thresh_log2[1] = burstp->tunep->seq_5g20.N_rx_log2;
			params->w_len = burstp->tunep->seq_5g20.w_len;
			params->w_offset = burstp->tunep->seq_5g20.w_offset;
			} else {
				params->thresh_scale[0] = burstp->tunep->seq_2g20.N_tx_scale;
				params->thresh_log2[0] = burstp->tunep->seq_2g20.N_tx_log2;
				params->thresh_scale[1] = burstp->tunep->seq_2g20.N_rx_scale;
				params->thresh_log2[1] = burstp->tunep->seq_2g20.N_rx_log2;
				params->w_len = burstp->tunep->seq_2g20.w_len;
				params->w_offset = burstp->tunep->seq_2g20.w_offset;
			}
		} else {
			int i;
			for (i = 0; i <  2; i++) {
				params->thresh_scale[i] = burstp->tunep->N_scale[i + TOF_BW_NUM_V2];
				params->thresh_log2[i] = burstp->tunep->N_log2[i + TOF_BW_NUM_V2];
			}
		}
	} else {
		if (frame_bw == TOF_BW_20MHZ_INDEX_V2 && !RSPEC_ISVHT(burstp->configp->ratespec) &&
			CHSPEC_IS2G(phy_utils_get_chanspec(WLC_PI(wlc)))) {
			/* 2g/20M channels without VHT rate */
			params->thresh_log2[1] = burstp->tunep->N_log2_2g;
			params->thresh_scale[1] = burstp->tunep->N_scale_2g;
		} else {
			params->thresh_log2[1] = burstp->tunep->N_log2[frame_bw];
			params->thresh_scale[1] = burstp->tunep->N_scale[frame_bw];
		}
	}
}

#define LOG2_NFFT_BASE 6
static void
pdburst_rtd_adj_upd_info(pdburst_t *burstp, tof_rtd_adj_params_t *params, int frame_bw,
		int frame_type)
{
	wlc_info_t *wlc = burstp->wlc;
	chanspec_t chanspec;

	if (burstp->sm->tof_mode == WL_PROXD_MODE_INITIATOR) {
		chanspec = pdburst_get_chspec_from_rspec(burstp, burstp->ftm_rx_rspec);
	} else {
		chanspec = pdburst_get_chspec_from_rspec(burstp, burstp->ack_rx_rspec);
	}

	if (chanspec == phy_utils_get_chanspec(WLC_PI(wlc))) {
		/* initiator and target use same chanspec */
		params->subband = PRXS_SUBBAND_20LL;
	} else {
		params->subband = ((uint32)frame_bw) >> (16 + PRXS1_ACPHY_SUBBAND_SHIFT_GEN2);
	}
	frame_bw = frame_bw & 0xffff;
	params->bw = (BANDWIDTH_BASE << frame_bw);
	params->nfft = (NFFT_BASE << frame_bw);
	params->log2_nfft = (LOG2_NFFT_BASE + frame_bw);
	pdburst_rtd_adj_upd_h_ts(params);
	params->gd = 0;
	params->gd_ns = 0;
	params->adj_ns = 0;
	params->gd_shift = !BURST_SEQ_EN(burstp);
	params->p_A = NULL;
	params->w_ext = NULL;	/* No hardware gd/channel smoothing data */
	params->tdcs_en = TOF_DEFAULT_TDCS_EN;
	params->longwin_en = TOF_DEFAULT_LONGWIN_EN;

	pdburst_rtd_adj_upd_fp_params(burstp, params, frame_bw);
}

/* Compute timestamps using sample-capture method */
#ifdef WL_PROXD_PHYTS
/* Uncomment below to use 4s mode */
/* #define SC_RXFARROW_MODE */
#ifdef WL_PROXD_PHYTS_DEBUG
/* Sampcap of the last ftm is available in burstp */
#define TS_CNT_DBG_MAX 10u

pdburst_t phyts_burstp_dbg;
ftmts_t phyts_ftmts_dbg[TS_CNT_DBG_MAX];
uint8 phyts_ftmts_idx = 0u;
#endif /* WL_PROXD_PHYTS_DEBUG */

/* Compute timestamps using sample-capture method */
static int
pdburst_phyts(pdburst_t *burstp, uint32 *ftm_ts_ps, uint32 *ack_ts_ps)
{
	int ret = BCME_OK;
	wlc_info_t *wlc = burstp->wlc;
	cint16 *sc_mem_ptr = NULL;
	uint32 sc_mem_size;

#ifdef WL_PROXD_PHYTS_DEBUG
	int16 vali, valq;
	uint32 sc_idx;
	uint32 nonzero_idx = 0;
	cint16 *sc_mem_save_dbg = NULL;
	uint32 sc_mem_size_dbg = (wlc->hw->sc_buf_info->sz);
#ifdef SC_RXFARROW_MODE
	/* As 4s mode skips alternate samples when reading */
	sc_mem_size_dbg = (sc_mem_size_dbg >> 1u);
#endif /* SC_RXFARROW_MODE */

	if (burstp->phyts_setup != TRUE) {
		ret = BCME_ERROR;
		FTM_ERR(("wl%u: pdburst_phyts: phyts_setup failed\n",
			wlc->pub->unit));
		goto done;
	}

	/* BM needs to be pre-allocated */
	if (sc_mem_size_dbg == 0) {
		ret = BCME_NOMEM;
		goto done;
	}

	sc_mem_save_dbg = (cint16 *)MALLOCZ(wlc->osh, sc_mem_size_dbg * sizeof(cint16));
	if (sc_mem_save_dbg == NULL) {
		FTM_ERR(("PHYTS_DBG: Not enough sc memory\n"));
		ret = BCME_NOMEM;
		goto done;
	}
#endif /* WL_PROXD_PHYTS_DEBUG */

	/* Allocate memory for sample capture read buffer */
	ret = phy_tof_phyts_get_sc_read_buf_sz_api(WLC_PI(wlc), &sc_mem_size);
	if (ret != BCME_OK) {
		FTM_ERR(("wl%u: pdburst_phyts: sc read buf size get failed\n",
			wlc->pub->unit));
		/* Fallback to AVB method */
		burstp->fallback_avb = TRUE;
		goto done;
	}

	sc_mem_ptr = (cint16 *)MALLOCZ(wlc->osh, sc_mem_size * sizeof(cint16));
	if (sc_mem_ptr == NULL) {
		FTM_ERR(("wl%u: pdburst_phyts: sc buf malloc failed\n",
			wlc->pub->unit));
		/* Fallback to AVB method */
		burstp->fallback_avb = TRUE;
		goto done;
	}

	bzero(sc_mem_ptr, sizeof(cint16) * sc_mem_size);
	burstp->sc_mem_ptr = sc_mem_ptr;
	burstp->sc_mem_size = sc_mem_size;
	FTM_TRACE(("wl%u: pdburst_phyts: sc_mem_ptr %p sc_size= %d\n",
		wlc->pub->unit, sc_mem_ptr, sc_mem_size));

#ifdef WL_PROXD_PHYTS_DEBUG
	TOF_PRINTF(("wl%u: pdburst_phyts: sc_mem_save_dbg %p sc_mem_size_dbg= %d\n",
		wlc->pub->unit, sc_mem_save_dbg, sc_mem_size_dbg));
	/* Read sample-capture data */
	ret = phy_tof_phyts_read_sc_api(WLC_PI(wlc), sc_mem_save_dbg);
	if (ret != BCME_OK) {
		FTM_ERR(("wl%u: pdburst_phyts: sc read failed\n",
			wlc->pub->unit));
		/* Fallback to AVB method */
		burstp->fallback_avb = TRUE;
		goto done;
	} else {

		for (sc_idx = 0u; sc_idx < sc_mem_size_dbg; sc_idx++) {
			vali = sc_mem_save_dbg[sc_idx].i;
			valq = sc_mem_save_dbg[sc_idx].q;
			/* only uncomment when full samp cap debug is needed.
			if (burstp->rxcnt <= 1 && burstp->txcnt <= 1)
				wlc_macdbg_dtrace_log_str(wlc->macdbg, NULL,
					"%s: index: %u; %d + i * %d",
					__FUNCTION__, sc_idx, vali, valq);
			*/
			if (vali != 0 && valq != 0)
				nonzero_idx = sc_idx;
		}
		TOF_PRINTF(("wl%u: pdburst_phyts: LAST NON-ZERO idx = %u\n",
			wlc->pub->unit, nonzero_idx));
	}
#endif /* WL_PROXD_PHYTS_DEBUG */

	/* Packet detection */
	ret = phy_tof_phyts_pkt_detect_api(WLC_PI(wlc), sc_mem_ptr);
	if (ret != BCME_OK) {
		FTM_ERR(("wl%u: pdburst_phyts: pkt detection failed\n",
			wlc->pub->unit));
		/* Fallback to AVB method */
		burstp->fallback_avb = TRUE;
		goto done;
	}

	/* Find approriate LTF window and Match filtering to timestamp FTM packet */
	ret = phy_tof_phyts_mf_api(WLC_PI(wlc), sc_mem_ptr, ftm_ts_ps, 0u);
	if (ret != BCME_OK) {
		FTM_ERR(("wl%u: pdburst_phyts: FTM Ts failed; fallback to avb\n",
			wlc->pub->unit));
		/* Fallback to AVB method */
		burstp->fallback_avb = TRUE;
		goto done;
	}

	/* Find approriate LTF window and Match filtering to timestamp ACK packet */
	ret = phy_tof_phyts_mf_api(WLC_PI(wlc), sc_mem_ptr, ack_ts_ps, 1u);
	if (ret != BCME_OK) {
		FTM_ERR(("wl%u: pdburst_phyts: ACK Ts failed, don't fallback\n",
			wlc->pub->unit));
		/* Don't fallback to AVB method, as time reserved might not be enough */
		burstp->fallback_avb = FALSE;
	}

	/* Enable sample capture for next FTM/reset phy-deaf */
	/* If fallbacked to AVB method, then this will happen after */
	/* reading the channel memory in the AVB path */
	pdburst_measure(burstp, TOF_RX);

done:
	if (sc_mem_ptr != NULL) {
		MFREE(wlc->osh, sc_mem_ptr, sc_mem_size * sizeof(cint16));
	}
#ifdef WL_PROXD_PHYTS_DEBUG
	if (sc_mem_save_dbg != NULL) {
		MFREE(wlc->osh, sc_mem_save_dbg, sc_mem_size_dbg * sizeof(cint16));
	}
#endif /* WL_PROXD_PHYTS_DEBUG */
	return ret;
}
#endif /* WL_PROXD_PHYTS */

/* Adjustment to the timestamp */
static int
pdburst_rtd_adj(pdburst_t *burstp, int frame_type, int frame_bw, int cfo,
	int32 *gd, int32 *adj, bool initiator, uint8 ts_id)
{
	int ret = BCME_OK;
	wlc_info_t *wlc = burstp->wlc;
	tof_rtd_adj_params_t params;

	int32 *H_buf = NULL;
	int status = BCME_OK;

	bool seq_en = BURST_SEQ_EN(burstp);
	bool hw_adj_en = BURST_HWADJ_EN(burstp->tunep);
	bool sw_adj_en = BURST_SWADJ_EN(burstp->tunep);
	uint8* ri_rr = NULL;
	uint32* p_collect_data = NULL;
	int32 *chan_data = NULL;
	uint32 chan_data_len = 0u;
	int n_out = 0;

#ifdef TOF_DEBUG
	TOF_PRINTF(("func %s burstp %p frame_type %x bw %x cfo %x sw_adj %x hw_adj %x\n",
		__FUNCTION__, OSL_OBFUSCATE_BUF(burstp),  frame_type, frame_bw,  cfo,
			sw_adj_en,	hw_adj_en));
	TOF_PRINTF(("pdburst_rtd_adj() seq_started %d num_meas %d tof_txcnt %d "
		"seq_en %x gd %x adj %x initiator %x frame_type %d\n",
		burstp->seq_started, burstp->num_meas, burstp->sm->tof_txcnt,
		seq_en, *gd,  *adj,  initiator, frame_type));
#endif
	if (BURST_SEQ_EN(burstp) &&
		(!burstp->seq_started) &&
		((burstp->num_meas % TOF_DEFAULT_FTMCNT_SEQ == 1) || /* initiator */
		!(burstp->sm->tof_txcnt % TOF_DEFAULT_FTMCNT_SEQ  ))) { /* target */
		burstp->seq_started = TRUE;
		burstp->result_flags |= WL_PROXD_LTFSEQ_STARTED;
		pdburst_measure(burstp, initiator? TOF_RX : TOF_RESET);
		return BCME_OK;
	}

	/* Update the information in params */
	pdburst_rtd_adj_upd_info(burstp, &params, frame_bw, frame_type);
	/* Allocate memory for buffering channel/LTFs */
	pdburst_rtd_adj_buf_len(burstp, &params, params.nfft);

	H_buf = (int32 *)MALLOCZ(wlc->osh, (params.buf_len
		+ params.buf_len_extra) * sizeof(cint32));
	if (H_buf == NULL) {
		ret = BCME_NOMEM;
		goto done;
	} else {
		params.H = H_buf;
	}

#ifdef TOF_COLLECT_INLINE
	if (TOF_COLLECT_INLINE_ENAB(wlc->pub)) {
		/* chan_data allocation status is checked in the place
		 * where it is used because of multiple collection paths
		 */
		chan_data_len = params.nfft * sizeof(cint32);
		chan_data = (int32 *)MALLOCZ(wlc->osh, chan_data_len);

		if (chan_data == NULL) {
			ret = BCME_NOMEM;
			goto done;
		}
	}
#endif /* TOF_COLLECT_INLINE */

#ifdef TOF_COLLECT
	if (burstp->collectp) {
		p_collect_data = burstp->collectp->collect_buf;
		ri_rr = burstp->collectp->ri_rr;
			}
#endif

	*gd = *adj = 0;

	/* Retrieve the required channel information */
	ret = pdburst_rtd_adj_retrieve_channel(burstp, &params, frame_bw, frame_type,
		&n_out, &hw_adj_en, &sw_adj_en, p_collect_data);
		if (ret != BCME_OK) {
		goto done;
	}

#ifdef TOF_DBG_SEQ
		/* In debug mode, this prevents the Sequence Triggering again and */
		/* overwriting the sample capture buffer on the reception of next frame */
		burstp->seq_started = FALSE;
		/*
			if(burstp->wlc->clk)
				wlc_write_shm(burstp->wlc, burstp->shmemptr + M_TOF_UCODE_SET, 0);
		*/
#endif

	pdburst_measure(burstp, initiator? TOF_RX : TOF_RESET);

#ifdef WL_PROXD_PHYTS
	if (!initiator && burstp->sampcap_method) {
		/* Enable back sample capture */
		phy_tof_phyts_enable_sc_api(WLC_PI(wlc));
	}
#endif /* WL_PROXD_PHYTS */

	if (burstp->seq_en) {
		int adj1;
		adj1 = (int)pdburst_get_kval(burstp, initiator, TRUE, FALSE);

		/* retreive bit flip and SNR threshold from tunep */
		params.bitflip_thresh = burstp->tunep->bitflip_thresh;
		params.snr_thresh = burstp->tunep->snr_thresh;

		status = wlc_phy_seq_ts(WLC_PI(wlc), params.buf_len, H_buf,
			(initiator ? 0 : 1), cfo, adj1, (void*)&params,
			&params.adj_ns, &burstp->seq_len, p_collect_data,
			ri_rr, burstp->tunep->smooth_win_en);
		if (status) {
			seq_en = FALSE;
		}
		if (!initiator && burstp->seq_state == TOF_SEQ_DONE) {
			burstp->seq_started = FALSE;
		}
#ifdef TOF_COLLECT
		n_out = 2*(params.buf_len + K_TOF_COLLECT_H_PAD);
#endif
	} else if (hw_adj_en || sw_adj_en) {
		if (tof_rtd_adj(wlc, &params, chan_data, &chan_data_len) != BCME_OK) {
#ifdef TOF_DEBUG_TIME
			TOF_PRINTF(("$$$ tof_rtd_adj failed $$$\n"));
#endif
			hw_adj_en = FALSE;
			sw_adj_en = FALSE;
		}
	}

	if (seq_en)
		burstp->adj_type_cnt[TOF_ADJ_SEQ]++;
	else if (hw_adj_en)
		burstp->adj_type_cnt[TOF_ADJ_HARDWARE]++;
	else if (sw_adj_en)
		burstp->adj_type_cnt[TOF_ADJ_SOFTWARE]++;

	*gd = params.gd_ns;
	*adj = params.adj_ns;

#ifdef TOF_COLLECT
	if (p_collect_data) {
		burstp->collectp->collect_info.nfft =  n_out;
		if (BURST_SEQ_EN(burstp)) {
			burstp->collectp->collect_info.type = TOF_ADJ_SEQ;
		} else if (BURST_HWADJ_EN(burstp->tunep)) {
			burstp->collectp->collect_info.type = TOF_ADJ_HARDWARE;
		} else if (BURST_SWADJ_EN(burstp->tunep)) {
			burstp->collectp->collect_info.type = TOF_ADJ_SOFTWARE;
		}
		burstp->collectp->collect_info.gd_adj_ns = *gd;
		burstp->collectp->collect_info.gd_h_adj_ns = *adj;
	}
#endif /* TOF_COLLECT */

	ret = (hw_adj_en || sw_adj_en || seq_en) ? BCME_OK : BCME_ERROR;

done:
#ifdef TOF_COLLECT_INLINE
	if (TOF_COLLECT_INLINE_ENAB(burstp->wlc->pub)) {
		if (chan_data && chan_data_len && (ret == BCME_OK)) {
			pdburst_collect_inline_frame_info_chan_data(burstp,
				(uint32 *)chan_data, chan_data_len);
		}
		if (chan_data != NULL) {
			MFREE(wlc->osh, chan_data, chan_data_len);
		}
	}
#endif /* TOF_COLLECT_INLINE */

	if (H_buf != NULL) {
		MFREE(wlc->osh, H_buf, (params.buf_len + params.buf_len_extra) * sizeof(cint32));
	}

	return ret;
}

/* transmit command to ucode */
static bool pdburst_cmd(wlc_info_t *wlc, uint shmemptr, uint16 cmd)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	d11regs_t *regs = wlc_hw->regs;
	int i = 0;

	if (!wlc_hw->clk) {
		return FALSE;
	}

	/* Wait until last command completes */
	while ((R_REG(wlc_hw->osh, D11_MACCOMMAND_ALTBASE(regs, wlc->regoffsets)) & MCMD_TOF) &&
		(i < TOF_MCMD_TIMEOUT)) {
		OSL_DELAY(1);
		i++;
	}

	if (R_REG(wlc_hw->osh, D11_MACCOMMAND_ALTBASE(regs, wlc->regoffsets)) & MCMD_TOF) {
		FTM_ERR(("TOF ucode cmd timeout; maccommand: 0x%p tof_cmf %x\n",
			D11_MACCOMMAND_ALTBASE(regs, wlc->regoffsets), cmd));
		return FALSE;
	}

	wlc_write_shm(wlc, shmemptr + M_TOF_CMD_OFFSET(wlc), cmd);

	W_REG(wlc_hw->osh, D11_MACCOMMAND_ALTBASE(regs, wlc->regoffsets), MCMD_TOF);

#ifdef TOF_DEBUG
	TOF_PRINTF(("cmd = %x c1f %x c7f %x\n", cmd, wlc_read_shm(wlc, (0xc1f * 2)),
		wlc_read_shm(wlc, (0xc7f * 2))));
	for (i = 0; i < 10; i++) {
		TOF_PRINTF(("TOF_DBG%d %02x  \n", i+1, wlc_read_shm(wlc, ((0x888 + i) * 2))));
	}
#endif

	if (cmd & TOF_RX) {
		/* Inform ucode to process FTM meas frames on initiator */
		wlc_update_shm(wlc, shmemptr + M_TOF_FLAGS_OFFSET(wlc),
			1 << TOF_RX_FTM_NBIT, 1 << TOF_RX_FTM_NBIT);
	}
	else {
		wlc_update_shm(wlc, shmemptr + M_TOF_FLAGS_OFFSET(wlc),
			0 << TOF_RX_FTM_NBIT, 1 << TOF_RX_FTM_NBIT);
	}

	return TRUE;
}

/* send measurement cmd to ucode */
static void pdburst_measure(pdburst_t *burstp, int cmd)
{
	wlc_info_t *wlc = burstp->wlc;
	uint16 tof_cmd = TOF_RESET;
#ifdef TOF_DEBUG_TIME
	uint64 phy_deaf_end;
	uint32 ftm_sep;
	uint32 diff;
#endif /* TOF_DEBUG_TIME */

	ASSERT(wlc->hw != NULL);
	if (wlc->hw == NULL) {
		return;
	}

	if ((!wlc->hw->clk) || (wlc->pub->hw_off)) {
		WL_ERROR(("pdburst_measure() clk %d , hw_off %d\n",
			wlc->hw->clk, wlc->pub->hw_off));
		return;
	}

	burstp->tofcmd = 0xdead;

	if (BURST_SEQ_EN(burstp) && burstp->seq_started) {
		tof_cmd |= (1 << TOF_SEQ_SHIFT);
	}

	if (cmd == TOF_RX) {
		tof_cmd |= TOF_RX;
		if ((burstp->configp != NULL) && !BURST_SEQ_EN(burstp)) {
			if (RSPEC_ISHE(burstp->configp->ratespec)) {
				tof_cmd |= (1u << TOF_HE_ACK_SHIFT);
			} else if (RSPEC_ISVHT(burstp->configp->ratespec) &&
				BURST_IS_VHTACK(burstp)) {
				tof_cmd |= (1u << TOF_VHT_ACK_SHIFT);
			}
			/* For HT use legacy ACK only for now */
		}
	}

	if (pdburst_cmd(wlc, burstp->shmemptr, tof_cmd)) {
		burstp->tofcmd = tof_cmd;
#if defined(TOF_SEQ_DBG)
		if (BURST_SEQ_EN(burstp)) {
			gTofSeqCnt++;
			if (gTofSeqCnt > 1)
				cmd = 0;
		}
#endif
		phy_tof_cmd(WLC_PI(wlc), BURST_SEQ_EN(burstp), burstp->tunep->emu_delay);
	}
	else {
		phy_tof_cmd(WLC_PI(wlc), FALSE, 0);
	}

	if (cmd == TOF_RESET && burstp->tofcmd != 0xdead) {
		burstp->tof_reset = TRUE;
	}
	else {
		burstp->tof_reset = 0;
	}

#ifdef WL_PROXD_PHYTS
	if ((cmd == TOF_RX) && (burstp->sampcap_method)) {
		/* Enable back sample capture */
		phy_tof_phyts_enable_sc_api(WLC_PI(wlc));
	}
#endif /* WL_PROXD_PHYTS */

#ifdef TOF_DEBUG_TIME
	/* phy is undefed with phy_tof_cmd() above */
	phy_deaf_end = OSL_SYSUPTIME_US();

	ftm_sep = burstp->configp ? FTM_INTVL2USEC(&burstp->configp->ftm_sep) : 0;
	diff = phy_deaf_end - burstp->phy_deaf_start;
	if (ftm_sep && burstp->phy_deaf_start &&
		(burstp->sm->tof_mode == WL_PROXD_MODE_INITIATOR)) {
		if (diff > ftm_sep) {
			WL_ERROR(("ftp-phydeaf: phy is deaf for(%uus) for more than "
				"sep intv(%dus)\n", diff, ftm_sep));
		}
		TOF_PRINTF(("ftm-phydeaf: phy is deaf from %u to %u(%uus), sep intv(%dus)\n",
			(uint32)burstp->phy_deaf_start, (uint32)phy_deaf_end, diff, ftm_sep));
	}

	/* reset phy deaf start time */
	burstp->phy_deaf_start = 0;
#endif /* TOF_DEBUG_TIME */
}

/* Get AVB time stamp */
static void pdburst_avbtime(pdburst_t *burstp, uint32 *tx, uint32 *rx)
{
	wlc_info_t *wlc = burstp->wlc;

	if (PROXD_ENAB_UCODE_TSYNC(wlc->pub)) {
		pdburst_sm_t *sm = burstp->sm;
		*tx = sm->tod;
		*rx = sm->toa;
	} else {
		wlc_get_avb_timestamp(wlc->hw, tx, rx);
	}
}

/* Check whether FTM exchange is successful */
#define TOF_RSP_TIMEOUT 200u
static bool
pdburst_tof_success(pdburst_t *burstp, bool acked, uint16 id)
{
	wlc_info_t *wlc = burstp->wlc;
	bool tof_success = FALSE;
	uint8 valid_ts = TRUE;
	uint16 rspcmd = 0u, i = 0u;

	if (!wlc->hw->clk) {
		return FALSE;
	}

	if (PROXD_ENAB_UCODE_TSYNC(wlc->pub) == TRUE) {
		valid_ts = burstp->is_valid_ts;
	}

	if ((acked == TRUE) && (valid_ts == TRUE)) {
		for (i = 0; i < TOF_RSP_TIMEOUT; i++) {
			rspcmd = wlc_read_shm(wlc, burstp->shmemptr + M_TOF_RSP_OFFSET(wlc));
			if ((rspcmd & TOF_RSP_MASK) == TOF_SUCCESS) {
				tof_success = TRUE;
				break;
			}
			OSL_DELAY(1);
		}
	}
	if (!tof_success) {
		FTM_ERR(("Proxd Failed id %d rspcmd 0x%x tofcmd 0x%x\n",
			id, rspcmd, burstp->tofcmd));
	}

#ifdef TOF_COLLECT
	if (burstp->collectp && burstp->collectp->collect) {
		burstp->collectp->collect_info.tof_cmd = burstp->tofcmd;
		burstp->collectp->collect_info.tof_rsp = rspcmd;
		burstp->collectp->collect_info.tof_id = id;
		burstp->collectp->collect_info.nfft = 0;
		burstp->collectp->collect_info.type = TOF_ADJ_NONE;
	}
#endif /* TOF_COLLECT */

	return tof_success;
}

/* Update tof info from rxstatus and based on rspec */
static int
pdburst_get_tof_info(pdburst_t *burstp, wlc_phy_tof_info_t *tof_info, bool initiator)
{
	wlc_info_t *wlc = burstp->wlc;
	wlc_phy_tof_info_type_t tof_info_mask;
	ratespec_t rspec;
	uint32 subband;
	int expected_frame_bw = 0;
	int ret = BCME_OK;

	/* Fields to be read from rxstatus */
	tof_info_mask = (WLC_PHY_TOF_INFO_TYPE_FRAME_TYPE |
		WLC_PHY_TOF_INFO_TYPE_FRAME_BW | WLC_PHY_TOF_INFO_TYPE_CFO |
			WLC_PHY_TOF_INFO_TYPE_RSSI);

	ret = wlc_phy_tof_info(WLC_PI(wlc), tof_info, tof_info_mask, burstp->core);
	if (ret != BCME_OK) {
		tof_info->info_mask = WLC_PHY_TOF_INFO_TYPE_NONE;
		tof_info->frame_type = -1;
		tof_info->frame_bw = -1;
		goto done;
	}

	if (tof_info->info_mask & WLC_PHY_TOF_INFO_TYPE_FRAME_BW) {
		subband = ((uint32)(tof_info->frame_bw)) & 0xffff0000;
		tof_info->frame_bw = tof_info->frame_bw & 0xffff;
	} else {
		subband = 0;
		tof_info->frame_bw = -1;
	}

	if (!BURST_SEQ_EN(burstp)) {
		/* On the initiator side, for the legacy case, use the BW  */
		/* from the rxstatus itself, as legacy header doesn't have */
		/* bw information, and rspec always hold 20MHz bandwidth   */
		if (initiator) {
			rspec = burstp->ftm_rx_rspec;
			/* Update the rspec BW information for legacy */
			if ((rspec & WL_RSPEC_ENCODING_MASK) == WL_RSPEC_ENCODE_RATE) {
				if (tof_info->frame_bw == TOF_BW_160MHZ_INDEX_V2) {
					rspec = (rspec & (~(WL_RSPEC_BW_MASK))) |
						WL_RSPEC_BW_160MHZ;
				} else if (tof_info->frame_bw == TOF_BW_80MHZ_INDEX_V2) {
					rspec = (rspec & (~(WL_RSPEC_BW_MASK))) | WL_RSPEC_BW_80MHZ;
				} else if (tof_info->frame_bw == TOF_BW_40MHZ_INDEX_V2) {
					rspec = (rspec & (~(WL_RSPEC_BW_MASK))) | WL_RSPEC_BW_40MHZ;
				} else {
					rspec = (rspec & (~(WL_RSPEC_BW_MASK))) | WL_RSPEC_BW_20MHZ;
				}
			}
			burstp->ftm_rx_rspec = rspec;
		} else {
			rspec = burstp->ack_rx_rspec;
		}
		/* Update frame_bw from rspec for non-legacy cases */
		if ((rspec & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_160MHZ) {
			tof_info->frame_bw = TOF_BW_160MHZ_INDEX_V2;
		} else if ((rspec & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_80MHZ) {
			tof_info->frame_bw = TOF_BW_80MHZ_INDEX_V2;
		} else if ((rspec & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_40MHZ) {
			tof_info->frame_bw = TOF_BW_40MHZ_INDEX_V2;
		} else if ((rspec & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_20MHZ) {
			tof_info->frame_bw = TOF_BW_20MHZ_INDEX_V2;
		}

		if (CHSPEC_IS160(burstp->configp->chanspec))
			expected_frame_bw = TOF_BW_160MHZ_INDEX_V2;
		else if (CHSPEC_IS80(burstp->configp->chanspec))
			expected_frame_bw = TOF_BW_80MHZ_INDEX_V2;
		else if (CHSPEC_IS40(burstp->configp->chanspec))
			expected_frame_bw = TOF_BW_40MHZ_INDEX_V2;
		else
			expected_frame_bw = TOF_BW_20MHZ_INDEX_V2;

		if (tof_info->frame_bw != expected_frame_bw) {
			FTM_ERR(("%s: RX frame_bw 0x%x != expected_frame_bw 0x%x\n",
			 __FUNCTION__, tof_info->frame_bw, expected_frame_bw));
		}

		tof_info->frame_bw = (tof_info->frame_bw | subband);

		/* Update frame_type from rspec */
		if (RSPEC_ISHE(rspec)) {
			tof_info->frame_type = FT_HE;
		} else if (RSPEC_ISVHT(rspec)) {
			tof_info->frame_type = FT_VHT;
		} else if (RSPEC_ISHT(rspec)) {
			tof_info->frame_type = FT_HT;
		} else if (RSPEC_ISLEGACY(rspec)) {
			tof_info->frame_type = FT_OFDM;
		}
	}
	burstp->frame_type_cnt[tof_info->frame_type]++;

#ifdef TOF_COLLECT
	if (burstp->collectp && burstp->collectp->collect) {
		 burstp->collectp->collect_info.tof_frame_type
			= (uint8)(tof_info->frame_type);
		burstp->collectp->collect_info.tof_frame_bw =
			(uint8)(tof_info->frame_bw);
		burstp->collectp->collect_info.tof_rssi =
			(int8)(tof_info->rssi);
		burstp->collectp->collect_info.tof_cfo =
			(int32)(tof_info->cfo);
	}
#endif

done:
	return ret;
}

static int
pdburst_read_avb_timestamps(pdburst_t *burstp, uint32 *avbtx, uint32 *avbrx)
{
	wlc_info_t *wlc = burstp->wlc;
	uint32 clkst, macctrl1;
	int ret = BCME_OK;

	pdburst_avbtime(burstp, avbtx, avbrx);
	if ((*avbrx == burstp->oldavbrx) && (*avbtx == burstp->oldavbtx)) {
		wlc_get_avb_timer_reg(wlc->hw, &clkst, &macctrl1);
		wlc_enable_avb_timer(wlc->hw, TRUE);
		FTM_ERR(("Clkst %x Macctrl1 %x Restart AVB timer\n",
			clkst, macctrl1));
	} else {
		burstp->oldavbrx = *avbrx;
		burstp->oldavbtx = *avbtx;
	}

#ifdef TOF_COLLECT
	if (burstp->collectp && burstp->collectp->collect) {
		burstp->collectp->collect_info.tof_avb_rxl = (uint16)(*avbrx & 0xffff);
		burstp->collectp->collect_info.tof_avb_rxh =
			(uint16)((*avbrx >> 16) & 0xffff);
		burstp->collectp->collect_info.tof_avb_txl = (uint16)(*avbtx & 0xffff);
		burstp->collectp->collect_info.tof_avb_txh =
			(uint16)((*avbtx >> 16) & 0xffff);
	}
#endif /* TOF_COLLECT */

	return ret;
}

#ifdef WL_PROXD_PHYTS
static bool
pdburst_sampcap_is_fullbw(pdburst_t *burstp, bool initiator)
{
	bool ret = TRUE;

	if (initiator) {
		ret = (pdburst_get_subband_idx(burstp, burstp->ftm_rx_rspec) == 0);
	} else {
		ret = (pdburst_get_subband_idx(burstp, burstp->ack_rx_rspec) == 0);
	}

	return ret;
}

/* Check conditions to enable sample capture */
#define PHYTS_RSSI_THRESH_DBM -90 /* TBD */
static bool
pdburst_sampcap_enab(pdburst_t *burstp)
{
	pdftm_session_t *sn = (pdftm_session_t *)burstp->ctx;
	bool initiator = burstp->flags & WL_PROXD_SESSION_FLAG_INITIATOR;
	bool enab_phyts, is_fullbw, is_valid_rssi;

	ASSERT(FTM_VALID_SESSION(sn));

	is_fullbw = pdburst_sampcap_is_fullbw(burstp, initiator);
	enab_phyts = is_fullbw && (BURST_IS_VHTACK(burstp) ||
		BURST_IS_HEACK(burstp));
	is_valid_rssi = (initiator) ? (burstp->rssi > PHYTS_RSSI_THRESH_DBM) : 1u;
	/* Conditionally enable Samp-cap method */
	if ((!FTM_AVB_TS(sn->ftm->config->flags)) &&
		(!(burstp->flags & WL_PROXD_SESSION_FLAG_ONE_WAY)) &&
		enab_phyts && is_valid_rssi) {
		burstp->sampcap_method = TRUE;
		burstp->fallback_avb = FALSE;
	} else {
		burstp->sampcap_method = FALSE;
		burstp->fallback_avb = TRUE;
	}

	return burstp->sampcap_method;
}
#endif /* WL_PROXD_PHYTS */

/* Get measurement results */
static int
pdburst_measure_results(pdburst_t *burstp, uint64 *tx, uint64 *rx,
	uint32 *k, int32 *gd, int32 *adj, uint16 id, bool acked,
	wlc_phy_tof_info_t *tof_info, bool *discard, bool initiator,
	uint32 *avbrxp, uint32 *avbtxp)
{
	wlc_info_t *wlc = burstp->wlc;
	uint32 avbrx, avbtx;
	uint32 delta;
	int32 h_adj = 0;
#ifdef WL_PROXD_PHYTS
	uint32 ftm_ts_ps, ack_ts_ps, kval;
#endif /* WL_PROXD_PHYTS */
	uint32 ki, kt;
	int ret = BCME_OK;

	if (!pdburst_tof_success(burstp, acked, id)) {
		pdburst_measure(burstp, initiator? TOF_RX : TOF_RESET);
		goto done;
	}

	/* tof success */
	ret = pdburst_get_tof_info(burstp, tof_info, initiator);
	if (ret != BCME_OK) {
		goto done;
	}

#ifdef TOF_PROFILE
	wlc_read_tsf(burstp->wlc, &tsf_lastm, &tsf_hi);
	tsf_lastm -= tsf_start;
	TOF_PRINTF(("BEFORE_PROCESSING TIME = 0x%0x\n", tsf_lastm));
#endif /* TOF_PROFILE */

	/* Default */
	burstp->sampcap_method = FALSE;
	burstp->fallback_avb = TRUE;
#ifdef WL_PROXD_PHYTS
	/* Decide whether to run sample capture method */
	if (pdburst_sampcap_enab(burstp)) {

		wlc_phy_tof_info(WLC_PI(burstp->wlc), tof_info,
			WLC_PHY_TOF_INFO_TYPE_NONE, burstp->core);

		/* Run sample capture based phy timestamping method */
		ret = pdburst_phyts(burstp, &ftm_ts_ps, &ack_ts_ps);
		if ((ret != BCME_OK) && (!burstp->fallback_avb)) {
			goto done;
		}
	}
#endif /* WL_PROXD_PHYTS */

	/* For SEQ and AVB method */
	if (burstp->fallback_avb) {
		/* AVB timestamps not required for SEQ method */
		if (!BURST_SEQ_EN(burstp)) {
			ret = pdburst_read_avb_timestamps(burstp, &avbtx, &avbrx);
			FTM_TRACE(("AVB: tx %u rx %u id %x\n", avbtx, avbrx, id));
			if (ret != BCME_OK) {
				goto done;
			}
		}
		if (BURST_SEQ_EN(burstp) || burstp->tunep->hw_adj || burstp->tunep->sw_adj) {
			ret = pdburst_rtd_adj(burstp, tof_info->frame_type,
				tof_info->frame_bw, tof_info->cfo,
				gd, adj, initiator, id);
			if (ret != BCME_OK) {
				return BCME_ERROR;
			}
		} else {
			*gd = *adj = 0;
			pdburst_measure(burstp, initiator? TOF_RX : TOF_RESET);
			burstp->adj_type_cnt[TOF_ADJ_NONE]++;
		}

		h_adj = *adj;
	}
#ifdef TOF_PROFILE
	wlc_read_tsf(burstp->wlc, &tsf_lastm, &tsf_hi);
	tsf_lastm -= tsf_start;
	TOF_PRINTF(("AFTER_PROCESSING TIME = 0x%0x\n", tsf_lastm));
#endif

#ifdef WL_PROXD_PHYTS
	if (burstp->sampcap_method && !(burstp->fallback_avb)) {
		/* Get kval, kval also in ps */
		kval = pdburst_get_kval(burstp, initiator, FALSE, TRUE);
		*k = kval;

		/* Ts are in ps */
		if (initiator) {
			/* Get kvalue -> PHYTS kvalues are in ps */
			*tx = (ack_ts_ps - (ftm_ts_ps - kval));
			*rx = 0u;
		} else {
			*tx = 0u;
			*rx = ((ack_ts_ps - kval) - ftm_ts_ps);
		}
		FTM_TRACE(("Sampcap: Init:%u tx:%llu rx:%llu\n", initiator, *tx, *rx));
	} else
#endif /* WL_PROXD_PHYTS */
	{
		if (BURST_SEQ_EN(burstp)) {
			if (initiator) {
				*tx = h_adj;
				*rx = 0;
			} else {
				*tx = 0;
				*rx = h_adj;
			}
		} else {
			*avbrxp = avbrx;
			*avbtxp = avbtx;
			if (initiator) {
				ki = pdburst_get_kval(burstp, TRUE, FALSE, FALSE);
				*k = ki;
				delta = (uint32)(TOF_TICK_TO_NS(((avbtx-avbrx) & 0xffff),
					burstp->Tq));
				if (D11REV_IS(wlc->pub->corerev, 129)) {
					/* Inaccurate firmware AVB TX value from 436x4 */
					if (ki && (delta < TOF_43694_AVB_TXS)) {
						delta = TOF_43694_AVB_TXS_FIX;
					}
				}
				*rx = TOF_TICK_TO_NS(avbrx, burstp->Tq);
				*tx = *rx + delta;
				*rx = (*rx + h_adj) - ki;
#ifdef TOF_KVALUE_CAL
				TOF_PRINTF(("dTraw %d\n", (delta - h_adj)));
#endif
				TOF_PRINTF(("id %d, avbtx-avbrx %u, gd %d, h_adj %d, dif %d,"
					" delta %u, dTraw %d, T2 %llu, T3 %llu (%lld)\n",
					id, avbtx-avbrx, *gd, h_adj, (*gd - h_adj), delta,
					(delta - h_adj), *rx, *tx, (*tx - *rx)));
			} else {
				/*
					T4 = T1(avb timer) + Delta(avbrx-avbtx)
						+ h_adj - K (target k value)
					Tq is a shift factor to keep avb timer integer
					calculation accuracy
				*/
				kt = pdburst_get_kval(burstp, FALSE, FALSE, FALSE);
				*k = kt;
				delta = TOF_TICK_TO_NS(((avbrx-avbtx) & 0xffff), burstp->Tq);
				if (D11REV_IS(wlc->pub->corerev, 129)) {
					/* Inaccurate firmware AVB TX value from 436x4 */
					if (kt && (delta > TOF_43694_AVB_TXL)) {
						delta = TOF_43694_AVB_TXL_FIX;
					}
				}
				*tx = TOF_TICK_TO_NS(avbtx, burstp->Tq);
#ifdef TOF_KVALUE_CAL
				TOF_PRINTF(("dTraw %d\n", (delta + h_adj)));
#endif
				if (delta + h_adj > kt)
					*rx = *tx + delta + h_adj - kt;
				else {
					*rx = *tx;
					FTM_ERR(("K %d is bigger than delta %d\n", kt,
						delta + h_adj));
				}
				TOF_PRINTF(("id %d, avbtx %u, avbrx %u, avbrx-avbtx %u, gd %d,"
					" h_adj %d, dif %d, delta %u, dTraw %d, T1 %llu,"
					" T4 %llu (%lld)\n",
					id, avbtx, avbrx, (avbrx-avbtx), *gd, h_adj, (*gd - h_adj),
					delta, (delta + h_adj), *tx, *rx, (*rx - *tx)));
			}
		}
	}
done:
	return ret;
}

/* reset TOF state */
static void pdburst_reset(pdburst_sm_t *sm, uint8 mode, wl_proxd_status_t reason)
{
	pdburst_t *burstp = sm->tof_obj;

	if (reason != WL_PROXD_E_NOTSTARTED) {
		pdburst_measure(burstp, TOF_RESET);
		pdburst_hw(burstp, FALSE, FALSE);
#ifdef WL_RANGE_SEQ
		BURST_SEQ_EN_SET(burstp, FALSE);
#else
		burstp->seq_en = FALSE;
#endif
	}

	sm->tof_mode = mode;
	sm->tof_state = TOF_STATE_IDLE;
	sm->tof_txcnt = 0;
	sm->tof_rxcnt = 0;
	sm->tof_reason = reason;
	sm->tof_legacypeer = TOF_LEGACY_UNKNOWN;
	sm->tof_txvht = FALSE;
	sm->tof_rxvht = FALSE;

	if (reason != WL_PROXD_E_NOACK && reason != WL_PROXD_E_TIMEOUT)
		sm->tof_retrycnt = 0;

	if (burstp->ftm_tx_timer_active)
	{
		wlc_hrt_del_timeout(burstp->ftm_tx_timer);
		burstp->ftm_tx_timer_active = FALSE;
	}

	if (mode == WL_PROXD_MODE_INITIATOR) {
		bcopy(&burstp->configp->peer_mac, &sm->tof_peerea, ETHER_ADDR_LEN);
		if (burstp->timeractive) {
			burstp->timeractive = FALSE;
			wl_del_timer(burstp->wlc->wl, burstp->timer);
		}
		burstp->tof_tslist.tscnt = 0;
		burstp->frmcnt = 0;
	} else {
		bcopy(&ether_null, &sm->tof_peerea, ETHER_ADDR_LEN);
	}
}

static void
pdburst_excursion_complete(void *ctx,	int status, wlc_bsscfg_t *cfg)
{

	pdburst_t *burstp = (pdburst_t *)ctx;
#ifdef TOF_DEBUG_TIME
	TOF_PRINTF(("$$$$$  %s\n", __FUNCTION__));
#endif
	ASSERT(burstp != NULL);
	ASSERT(cfg != NULL);
	ASSERT(burstp->wlc != NULL);

	/* proximity duration time expired */
	if (burstp->scanestarted) {
		burstp->scanestarted = FALSE;
	}

	burstp->delayfree &= ~TOF_DFREE_SCAN;
	if (!burstp->delayfree && burstp->destroyed) {
#ifdef TOF_DEBUG_TIME
		TOF_PRINTF(("delayfree burstp %p scan complete\n", OSL_OBFUSCATE_BUF(burstp)));
#endif
		MFREE(burstp->wlc->osh, burstp, sizeof(pdburst_t));
		return;
	}

	if (burstp->smstoped)
		return;

#ifdef TOF_DEBUG_TIME2
	wlc_read_tsf(burstp->wlc, &tsf_tmo, &tsf_hi);
	tsf_tmo -= tsf_start;
#endif

	if (burstp->sm->tof_state != TOF_STATE_ICONFIRM)
		pdburst_sm(burstp->sm, TOF_EVENT_TMO, NULL, 0, NULL);
}

/* TOF stay power on using bsscfg state flag to stop mpc */
static void pdburst_pwron(pdburst_t* burstp, bool up)
{
	proxd_power(burstp->wlc, PROXD_PWR_ID_BURST, up);
}

/* TOF initiator timeout function */
static void
pdburst_duration_expired_cb(void *ctx)
{
	pdburst_t *burstp = (pdburst_t *)ctx;
	wlc_bsscfg_t *cfg;

	ASSERT(burstp != NULL);

	FTM_ERR(("wl%d: %s: burst EXPIRED %p\n",
		burstp->wlc->pub->unit, __FUNCTION__, burstp));
	cfg = burstp->bsscfg;

	wlc_hrt_del_timeout(burstp->duration_timer);
	burstp->duration_timer_active = FALSE;
	pdburst_excursion_complete(ctx, 0, cfg);
}

/* TOF target timeout function */
static void
pdburst_duration_expired_target(void *ctx)
{
	pdburst_t *burstp = (pdburst_t *)ctx;
	pdburst_sm_t *sm;
#ifdef TOF_DEBUG_TIME
	uint64 curtsf = 0;
#endif

	ASSERT(burstp != NULL);
	if (burstp) {
		FTM_ERR(("wl%d: %s: burst EXPIRED %p\n",
			burstp->wlc->pub->unit, __FUNCTION__, burstp));
#ifdef TOF_DEBUG_TIME
		FTM_GET_TSF(wlc_ftm_get_handle(burstp->wlc), curtsf);
		TOF_PRINTF(("*** %s: BURST EXPIRED TIME = %u.%u \n", __FUNCTION__,
			FTM_LOG_TSF_ARG(curtsf)));
#endif
		sm = burstp->sm;
		burstp->duration_timer_active = FALSE;
		if (sm) {
			pdburst_reset(sm, sm->tof_mode, WL_PROXD_E_TIMEOUT);
			pdburst_target_done(sm, WL_PROXD_E_TIMEOUT);
		}
	}
}

/* force phy rx classification */
static void pdburst_phy_rxclass_war(pdburst_t* burstp, chanspec_t chanspec)
{
	wlc_info_t *wlc = burstp->wlc;
	chanspec_t radio_chanspec = phy_utils_get_chanspec(WLC_PI(wlc));
	bool need_set = FALSE;

	if (CHSPEC_IS160(chanspec) && (radio_chanspec == chanspec)) {
		if (burstp->rxclass != PHY_RXFE_DEBUG2_160IN160) {
			need_set = TRUE;
			burstp->rxclass = PHY_RXFE_DEBUG2_160IN160;
		} else {
			/* rx class already set */
			goto done;
		}
	} else if (CHSPEC_IS80(chanspec) && (radio_chanspec == chanspec)) {
		if (burstp->rxclass != PHY_RXFE_DEBUG2_80IN80) {
			need_set = TRUE;
			burstp->rxclass = PHY_RXFE_DEBUG2_80IN80;
		} else {
			/* rx class already set */
			goto done;
		}
	} else if (CHSPEC_IS40(chanspec) && (radio_chanspec == chanspec)) {
		if (burstp->rxclass != PHY_RXFE_DEBUG2_40IN40) {
			need_set = TRUE;
			burstp->rxclass = PHY_RXFE_DEBUG2_40IN40;
		} else {
			/* rx class already set */
			goto done;
		}
	} else if (burstp->rxclass != PHY_RXFE_DEBUG2_DEFAULT) {
		/* put back the original value when the burst is done */
		phy_utils_write_phyreg(WLC_PI(wlc),
			ACPHY_RxFrontEndDebug2(wlc->pub->corerev),
			burstp->rxclass_saved);
		burstp->rxclass = PHY_RXFE_DEBUG2_DEFAULT;
		burstp->rxclass_saved = PHY_RXFE_DEBUG2_DEFAULT;
	}

	if (need_set) {
		/* only read the original value once per burst */
		if (burstp->rxclass_saved == PHY_RXFE_DEBUG2_DEFAULT) {
			burstp->rxclass_saved = phy_utils_read_phyreg(WLC_PI(wlc),
					ACPHY_RxFrontEndDebug2(wlc->pub->corerev));
		}
		/* force phy rx classification */
		phy_utils_write_phyreg(WLC_PI(wlc),
			ACPHY_RxFrontEndDebug2(wlc->pub->corerev),
			burstp->rxclass);
	}

done:
	return;
}

/* activate the scan engine */
static void pdburst_activate_pm(pdburst_t* burstp)
{
	wlc_info_t *wlc;

	wlc = burstp->wlc;

	ASSERT(wlc != NULL);

	burstp->txcnt = 0;
	burstp->rxcnt = 0;

	/* Enable AVB timer in case it is turned off when CLK is off */
	burstp->avb_active = TRUE;
	wlc_enable_avb_timer(wlc->hw, TRUE);
}

/* deactivate the scan engine */
static void pdburst_deactivate_pm(pdburst_t* burstp)
{
	if (burstp && burstp->avb_active) {
		burstp->avb_active = FALSE;
		wlc_enable_avb_timer(burstp->wlc->hw, FALSE);
		pdburst_phy_rxclass_war(burstp, 0);
	}

/* cleanup some resources since burst is done, may be more time before destroy */
#ifdef WL_PROXD_PHYTS
	/* Clean up sample-capture(loopback) setup */
	if (burstp && burstp->phyts_setup) {
		if (phy_tof_phyts_setup_api(WLC_PI(burstp->wlc),
			FALSE, PHYTS_ROLE_NONE) != BCME_OK) {
			FTM_ERR(("%s phy_tof_phyts_setup_api failed \n", __FUNCTION__));
		}
		burstp->phyts_setup = FALSE;
	}
#endif /* WL_PROXD_PHYTS */
}

static uint8 pdburst_get_ftm_cnt(pdburst_t* burstp)
{

	if (burstp->flags & WL_PROXD_SESSION_FLAG_MBURST_FOLLOWUP) {
		if (burstp->configp->num_ftm > 1) {
			if (burstp->tx_t1)
				return (burstp->configp->num_ftm);
			else
				return (burstp->configp->num_ftm - 1);
		}
	} else {
		if (burstp->configp->num_ftm > 1)
			return (burstp->configp->num_ftm - 1);
	}

	if (BURST_SEQ_EN(burstp))
		return burstp->tunep->ftm_cnt[TOF_BW_SEQTX_INDEX_V2];

	if (CHSPEC_IS160(burstp->configp->chanspec))
		return burstp->tunep->ftm_cnt[TOF_BW_160MHZ_INDEX_V2];

	if (CHSPEC_IS80(burstp->configp->chanspec))
		return burstp->tunep->ftm_cnt[TOF_BW_80MHZ_INDEX_V2];

	if (CHSPEC_IS40(burstp->configp->chanspec))
		return burstp->tunep->ftm_cnt[TOF_BW_40MHZ_INDEX_V2];

	return burstp->tunep->ftm_cnt[TOF_BW_20MHZ_INDEX_V2];
}

/* update TOF parameters from rxed frame */
static int
pdburst_rx_tof_params(pdburst_t *burstp, pdburst_frame_type_t ftype,
	const uint8 *body, int body_len, ratespec_t rspec)
{
	int err = BCME_OK;
	pdburst_session_info_t bsi;
	ftm_vs_req_params_t req_params;
	ftm_vs_seq_params_t seq_params;
	ftm_vs_sec_params_t sec_params;
	ftm_vs_meas_info_t meas_info;
	uint8 ri_rr[FTM_TPK_RI_RR_LEN_SECURE_2_0];
	ftm_vs_timing_params_t timing_params;
	wlc_phy_tof_secure_2_0_t  tof_sec_params;
	uint16 ri_rr_len = 0;

	bzero((wlc_phy_tof_secure_2_0_t *)&tof_sec_params, sizeof(tof_sec_params));
	bzero(ri_rr, sizeof(ri_rr));

	burstp->measurecnt = burstp->configp->num_ftm - 1;
	/* burstp->configp->ratespec = rspec; */

	bzero(&bsi, sizeof(bsi));
	/* initialize local variables with 0s */
	bzero(&req_params, sizeof(req_params));
	bzero(&seq_params, sizeof(seq_params));
	bzero(&sec_params, sizeof(sec_params));
	bzero(&meas_info, sizeof(meas_info));
	bzero(&timing_params, sizeof(timing_params));

	tof_sec_params.start_seq_time =
		TIMING_TLV_START_SEQ_TIME;
	tof_sec_params.delta_time_tx2rx =
		TIMING_TLV_DELTA_TIME_TX2RX;

	switch (ftype) {
	case PDBURST_FRAME_TYPE_REQ:
		bsi.vs_req_params = &req_params;
		bsi.vs_seq_params = &seq_params;
		break;
	case PDBURST_FRAME_TYPE_MEAS:
		bsi.vs_sec_params = &sec_params;
		bsi.vs_timing_params = &timing_params;
		bsi.vs_meas_info = &meas_info;
		break;
	default:
		err = BCME_UNSUPPORTED;
		goto done;
	}

	err = PROTOCB(burstp, vs_rx, (burstp->ctx, ftype, body, body_len, &bsi));
	if (err != BCME_OK) {
		/* clear vendor options if peer is not bcm */
		if (err == WL_PROXD_E_NOT_BCM) {
			pdburst_session_info_t info;
			bzero(&info, sizeof(info));
			err = PROTOCB(burstp, set_session_info, (burstp->ctx, &info));
			burstp->flags &= ~(WL_PROXD_SESSION_FLAG_SEQ_EN |
				WL_PROXD_SESSION_FLAG_VHTACK | WL_PROXD_SESSION_FLAG_SECURE);
		}
		goto done;
	}

	/* session is already updated in vs rx. update burst */
	if (FTM_VS_PARAMS_VALID(&req_params)) {
		burstp->seq_en = (req_params.flags & FTM_VS_REQ_F_SEQ_EN) ? TRUE : FALSE;
		burstp->totalfrmcnt = req_params.totfrmcnt;
		burstp->flags |= (req_params.flags & FTM_VS_REQ_F_VHTACK) ?
			WL_PROXD_SESSION_FLAG_VHTACK : 0;
		burstp->flags |= (req_params.flags & FTM_VS_REQ_F_SECURE) ?
			WL_PROXD_SESSION_FLAG_SECURE : 0;
	}

	/* Further TLVs not present/required for AVB */
	if (!BURST_SEQ_EN(burstp))
		goto done;

	if (burstp->flags & WL_PROXD_SESSION_FLAG_SECURE) {
		if (!BURST_SEQ_EN(burstp) || !FTM_VS_PARAMS_VALID(&sec_params))
			goto done;
		if (bsi.vs_sec_params->flags &
			FTM_VS_SEC_F_RANGING_2_0) {
			memcpy(ri_rr, bsi.vs_sec_params->ri,
				FTM_TPK_RI_PHY_LEN_SECURE_2_0);
			memcpy((ri_rr +
				FTM_TPK_RI_PHY_LEN_SECURE_2_0),
				bsi.vs_sec_params->rr,
				FTM_TPK_RR_PHY_LEN_SECURE_2_0);
			ri_rr_len = FTM_TPK_RI_RR_LEN_SECURE_2_0;
			tof_sec_params.start_seq_time =
			bsi.vs_timing_params->start_seq_time;
			tof_sec_params.delta_time_tx2rx =
			bsi.vs_timing_params->delta_time_tx2rx;
		} else if ((bsi.vs_sec_params->flags &
			FTM_VS_SEC_F_VALID)
			== FTM_VS_SEC_F_VALID) {
			memcpy(ri_rr, bsi.vs_sec_params->ri, FTM_VS_TPK_RI_LEN);
			memcpy((ri_rr + FTM_VS_TPK_RI_LEN), bsi.vs_sec_params->rr,
				(FTM_TPK_RI_RR_LEN - FTM_VS_TPK_RI_LEN));
			ri_rr_len = FTM_TPK_RI_RR_LEN;
		}
		err = phy_tof_set_ri_rr(WLC_PI(burstp->wlc), (uint8 *)ri_rr, ri_rr_len,
			burstp->core, TRUE, TRUE, tof_sec_params);
		prhex("phy_ri", (uint8 *)bsi.vs_sec_params->ri,
			((bsi.vs_sec_params->flags &
			FTM_VS_SEC_F_RANGING_2_0) ?
			FTM_VS_TPK_RI_LEN_SECURE_2_0 :
			FTM_VS_TPK_RI_LEN));
		prhex("phy_rr", (uint8 *)bsi.vs_sec_params->rr,
			((bsi.vs_sec_params->flags &
			FTM_VS_SEC_F_RANGING_2_0) ?
			FTM_VS_TPK_RR_LEN_SECURE_2_0 :
			FTM_VS_TPK_RR_LEN));

	} else {
		err = phy_tof_set_ri_rr(WLC_PI(burstp->wlc), NULL, FTM_TPK_RI_RR_LEN,
				burstp->core, TRUE, FALSE, tof_sec_params);
	}

done:
	return err;
}

static uint8 pdburst_rx_measurecnt(pdburst_t* burstp, const pdburst_tsinfo_t *protp)
{
	tof_tslist_t *listp = &burstp->tof_tslist;
	ftmts_t * list = listp->tslist;

	if (protp && list && protp->ts_id > 0) {
		/* Not the first measurement packet */
#ifdef TOF_DEBUG_TIME
		if (burstp->flags & WL_PROXD_SESSION_FLAG_MBURST_FOLLOWUP)
			TOF_PRINTF(("Comp txid %d tsid %d\n", list[listp->tscnt].tx_id,
				protp->ts_id));
#endif /* TOF_DEBUG_TIME */
		if (list[listp->tscnt].tx_id == protp->ts_id)
			return listp->tscnt + 1;
		/* This is retransmission */
		return listp->tscnt;
	}

	return 0;
}

static pdburst_seq_state_t
get_next_seq_state(int num_meas)
{
	return (pdburst_seq_state_t)(num_meas % TOF_SEQ_LAST);
}

/* Process rxed action frames */
static int pdburst_process_rx_frame(pdburst_t* burstp, const wlc_d11rxhdr_t *wrxh,
	const uint8 *body, int body_len, uint32 rspec, const pdburst_tsinfo_t *tsinfo)
{
	pdburst_sm_t *sm;
	wlc_info_t *wlc;
	int ret;
	pdburst_data_t data;
	const d11rxhdr_t *rxh;
	uint32 chan_size;
	int32 *tmpH;
	uint32* tmpR;
	int32 adj_err;
	uint8 mes_cnt;

	ASSERT(burstp != NULL);
	ASSERT(burstp->sm != NULL);
	ASSERT(burstp->wlc != NULL);

#ifndef TOF_COLLECT_REMOTE
	if (burstp->smstoped || !burstp->ctx || !burstp->svc_funcs)
		return 0;
#endif /* TOF_COLLECT */

	rxh = &wrxh->rxhdr;
	sm = burstp->sm;
	wlc = burstp->wlc;

#ifdef TOF_DEBUG_TIME
	if (burstp->sm->tof_mode == WL_PROXD_MODE_INITIATOR) {
		/* log action farme rx time to compute tottal deaf interval
		 * (ucode already made phy deaf on receivig FTM measurement frame).
		 */
		burstp->phy_deaf_start = OSL_SYSUPTIME_US();
	}

	if (tsinfo) {
		TOF_PRINTF(("pdburst_process_rx_frame: measurement dialog %d "
			"followup %d rxStatus1 0x%x ", tsinfo->tx_id, tsinfo->ts_id,
			D11RXHDR_ACCESS_VAL(rxh, wlc->pub->corerev, RxStatus1)));
		if (D11REV_GE(wlc->pub->corerev, 129))
			TOF_PRINTF(("rxStatus3 0x%x\n", D11RXHDR_GE129_ACCESS_VAL(rxh, RxStatus3)));
		else
			TOF_PRINTF(("MuRate 0x%x\n", D11RXHDR_LT80_ACCESS_VAL(rxh, MuRate)));
	} else {
		TOF_PRINTF(("pdburst_process_rx_frame: request\n"));
	}
#endif /* TOF_DEBUG_TIME */

	if (!tsinfo)
	{
		/* FTM Request */
#ifdef TOF_COLLECT_REMOTE
		if (*body == DOT11_ACTION_CAT_VS && *(body+5) == BRCM_FTM_VS_COLLECT_SUBTYPE) {
			pdburst_collect_frm_t *tof_hdr = (pdburst_collect_frm_t *)body;
			data.tof_type = tof_hdr->tof_type;
			tsinfo = (const pdburst_tsinfo_t *)body;
		}
		else
#endif /* TOF_COLLECT */
		{
			data.tof_type = TOF_TYPE_REQ_START;
#ifndef WL_RANGE_SEQ
			if (burstp->sm->tof_mode == WL_PROXD_MODE_TARGET) {
				burstp->seq_state = TOF_SEQ_NONE;
			}
#endif /* !WL_RANGE_SEQ */
		}
	} else if ((D11REV_GE(wlc->pub->corerev, 129) &&
			(D11RXHDR_GE129_ACCESS_VAL(rxh,	RxStatus3) & htol16(RXS_TOFINFO))) ||
		(D11REV_LT(wlc->pub->corerev, 129) && (D11RXHDR_ACCESS_VAL(rxh,
			wlc->pub->corerev, RxStatus1) & htol16(RXS_TOFINFO)))) {
		/* FTM measurement packet */
		if ((tsinfo->ts_id == 0) && (burstp->seq_en)) {
			uint8 frame_bw;
			if (CHSPEC_IS80(burstp->configp->chanspec)) {
				frame_bw = 2;
			} else {
				frame_bw = 0;
			}
			TOF_PRINTF(("frame_bw = %d\n", frame_bw));
			chan_size = (K_TOF_COLLECT_CHAN_SIZE) >> (2 - frame_bw);
			tmpH = NULL;
			if (!burstp->collectp) {
				TOF_PRINTF(("Allocating temp mem ----->\n"));
				tmpR = (uint32*)MALLOCZ(burstp->wlc->osh,
						(PHY_CORE_MAX+1)*chan_size* sizeof(uint32));
				if (tmpR == NULL) {
					TOF_PRINTF(("*** Malloc error for channel estimates."
								"*** \n"));
					return BCME_NOMEM;
				}
			} else {
				tmpR = (uint32 *) (burstp->collectp->chan);
			}

			adj_err = phy_tof_chan_freq_response(WLC_PI(burstp->wlc),
					chan_size, CORE0_K_TOF_H_BITS, tmpH, NULL, tmpR,
					FALSE, 1, TRUE);
			burstp->channel_dumped = TRUE;

			if (burstp->collectp) {
				burstp->collectp->collect_info.num_max_cores =
					phy_tof_num_cores(WLC_PI(burstp->wlc));
			}

			if ((tmpR != NULL) && !burstp->collectp) {
				MFREE(burstp->wlc->osh, tmpR,
						(PHY_CORE_MAX+1)*chan_size*sizeof(uint32));
			}
			TOF_PRINTF(("Channel dump : adj_err -> %d\n", adj_err));
			if (burstp->flags & WL_PROXD_SESSION_FLAG_SECURE) {
				if (burstp->tunep->core == 0xff) {
					phy_tof_core_select(WLC_PI(burstp->wlc),
							burstp->tunep->acs_gdv_thresh,
							burstp->tunep->acs_gdmm_thresh,
							burstp->tunep->acs_rssi_thresh,
							burstp->tunep->acs_delta_rssi_thresh,
							&(burstp->core),
							burstp->tunep->core_mask);
				}
			}
		}
		if (BURST_SEQ_EN(burstp)) {
			burstp->seq_state = get_next_seq_state(burstp->num_meas);
		}
		mes_cnt = pdburst_rx_measurecnt(burstp, tsinfo);
		burstp->num_meas = mes_cnt + 1; /* because meas_cnt starts from 0 */
#ifdef TOF_DEBUG_TIME
		TOF_PRINTF(("%s: num_meas %d mes_cnt %d measurecnt %d tx_id %d ts_id %d \n",
			__FUNCTION__, burstp->num_meas, mes_cnt,
		burstp->measurecnt, tsinfo->tx_id, tsinfo->ts_id));
#endif
		if (tsinfo->tx_id) {
			if (mes_cnt >= burstp->measurecnt) {
				data.tof_type = TOF_TYPE_MEASURE_END;
			} else {
				data.tof_type = TOF_TYPE_MEASURE;
			}
		} else {
			data.tof_type = TOF_TYPE_MEASURE_END;
		}
		data.tof_rssi = wrxh->rssi;

		if (BURST_SEQ_EN(burstp)) {
			if (burstp->sm->tof_mode == WL_PROXD_MODE_INITIATOR) {
				if (burstp->num_meas % TOF_DEFAULT_FTMCNT_SEQ == 1) {
					/* call the vs_rx cb */
#ifdef WL_RANGE_SEQ
					ret = pdburst_rx_tof_params(burstp,
							PDBURST_FRAME_TYPE_MEAS,
							body, body_len, rspec);
#endif /* WL_RANGE_SEQ */
					/* It was found that the phy's classsifier register
					** that the phy is deaf at time, on the reception of a ftm
					** measurement frame # 2. Since the driver control's the
					** undeaf'ing of the phy, this change was made
					*/
#ifdef WL_RANGE_SEQ
					proxd_undeaf_phy(burstp->wlc, TRUE);
					if (ret != BCME_OK) {
						return ret;
					}
#endif /* WL_RANGE_SEQ */
				} else if (burstp->num_meas % TOF_DEFAULT_FTMCNT_SEQ == 0) {
					/* changes from another RB
					** needs to be added
					*/
					burstp->measurecnt = burstp->configp->num_ftm - 1;
					/* 3rd measurement frame from target */
#ifdef WL_RANGE_SEQ
					ret = pdburst_rx_tof_params(burstp,
						PDBURST_FRAME_TYPE_MEAS_END,
						body, body_len, rspec);
					if (ret != BCME_OK) {
						FTM_ERR(("pdburst_rx_tof_params:"
							" Returned error %d\n", ret));
						return ret;
					}
#else
					if (FTM_BSSCFG_SECURE(wlc_ftm_get_handle(wlc),
						burstp->bsscfg)) {
					/* call the vs_rx cb to process the mf-buf or other TLVs */
					}
#endif /* WL_RANGE_SEQ */
				}
			}
		}
	} else {
		FTM_ERR(("rxh->RxStatus1 %x\n",
			D11RXHDR_ACCESS_VAL(rxh, wlc->pub->corerev, RxStatus1)));
		return 0;
	}

	bcopy(&burstp->configp->peer_mac, &data.tof_srcea, ETHER_ADDR_LEN);
	data.tof_rspec = rspec;
	++burstp->rxcnt;

	if (RSPEC_ISVHT(rspec) &&
		sm->tof_state != TOF_STATE_IDLE && sm->tof_state != TOF_STATE_ICONFIRM)
		sm->tof_rxvht = TRUE;

	ret = pdburst_sm(sm, TOF_EVENT_RXACT, tsinfo, sizeof(*tsinfo), &data);
	/* burstp and sm may be freed by pdburst_sm() */
	if (ret == TOF_RET_END || sm->tof_mode == WL_PROXD_MODE_TARGET)
		return 0;

	if (ret == TOF_RET_ALIVE) {
		if (burstp->duration_timer_active) {
			wlc_hrt_del_timeout(burstp->duration_timer);
			wlc_hrt_add_timeout(burstp->duration_timer,
				FTM_INTVL2USEC(&burstp->configp->timeout),
				pdburst_duration_expired_cb, (void *)burstp);
		}
	} else if (ret == TOF_RET_SLEEP) {
		if (burstp->scanestarted) {
			wlc_scan_abort_ex(wlc->scan, burstp->scanbsscfg,
				WLC_E_STATUS_ABORT);
			burstp->scanestarted = FALSE;
		}
	}

	return 0;
}

/* action frame tx complete callback */
void pdburst_tx_complete(wlc_info_t *wlc, uint txstatus, void *arg)
{
	pdburst_t *burstp = (pdburst_t *)arg;
	pdburst_sm_t *sm;
	int err = BCME_OK;

	if (!burstp)
		return;

#ifdef TOF_DEBUG_TIME
	TOF_PRINTF(("pdburst_tx_complete: burstp %p, %s \n",
		OSL_OBFUSCATE_BUF(burstp),
		((txstatus & TX_STATUS_ACK_RCV)?"ACK recvd":"NO ACK")));
#endif

	if (burstp->smstoped && (burstp->delayfree & TOF_DFREE_TXDONE)) {
		/* pkt txed but not processed because of burst timeout */
		proxd_undeaf_phy(wlc, (txstatus & TX_STATUS_ACK_RCV));
		if (FTM_BSSCFG_SECURE(wlc_ftm_get_handle(burstp->wlc), burstp->bsscfg)) {
			phy_rxgcrs_sel_classifier(WLC_PI(burstp->wlc),
				TOF_CLASSIFIER_BPHY_ON_OFDM_ON);
		}
	}
	burstp->delayfree &= ~TOF_DFREE_TXDONE;
	if (!burstp->delayfree && burstp->destroyed) {
#ifdef TOF_DEBUG_TIME
		TOF_PRINTF(("delayfree burstp %p after tx_complete\n", OSL_OBFUSCATE_BUF(burstp)));
#endif
		MFREE(burstp->wlc->osh, burstp, sizeof(pdburst_t));
		return;
	}

#ifndef TOF_COLLECT_REMOTE
	if (burstp->smstoped || !burstp->ctx || !burstp->svc_funcs)
		return;
#endif /* TOF_COLLECT_REMOTE */

	ASSERT(burstp->sm != NULL);

	sm = burstp->sm;
	if (sm == NULL) {
		ASSERT((burstp->ctx != NULL) && burstp->ftm_tx_timer_active);
		return;
	}
	err = (txstatus & TX_STATUS_ACK_RCV) ? BCME_OK : WL_PROXD_E_NOACK;

	(void)PROTOCB(burstp, tx_done, (burstp->ctx, err));

	if (err != BCME_OK) {
		FTM_ERR(("pdburst_tx_complete: ACK was lost txstat:0x%x, pkt:%d, retry:%d\n",
			txstatus, burstp->sm->tof_txcnt, burstp->sm->tof_retrycnt));

		if ((burstp->ftm_tx_timer_active) && (sm->tof_mode == WL_PROXD_MODE_TARGET)) {
			wlc_hrt_del_timeout(burstp->ftm_tx_timer);
			burstp->ftm_tx_timer_active = FALSE;
			FTM_ERR(("%s:ERROR: ftm[%d] OnHrtTimer TX is pending, cancelled\n",
				__FUNCTION__, burstp->sm->tof_txcnt));
		}
		/* reset ucode state */
		pdburst_measure(burstp, TOF_RESET);
		pdburst_sm(burstp->sm, TOF_EVENT_NOACK, NULL, 0, NULL);
	} else {
		pdburst_sm(burstp->sm, TOF_EVENT_ACKED, NULL, 0, NULL);
	}
}

static int
pdburst_init_bsi(pdburst_t *burstp, pdburst_frame_type_t ftype,
	ftm_vs_req_params_t *req, ftm_vs_seq_params_t *seq,
	ftm_vs_sec_params_t *sec, ftm_vs_mf_buf_t *mf,
	ftm_vs_meas_info_t *meas_info,
	ftm_vs_timing_params_t *timing_params,
	pdburst_session_info_t *bsi)
{
	int err = BCME_OK;
	wlc_phy_tof_info_t tof_info;

	BCM_REFERENCE(mf);

	bzero(bsi, sizeof(*bsi));

	if (ftype == PDBURST_FRAME_TYPE_REQ) {
		bsi->vs_req_params = req;
		bzero(req, sizeof(*req));
		req->flags = FTM_VS_F_VALID;
		req->totfrmcnt = pdburst_total_framecnt(burstp);
		/* other req params are generated or taken from session */
	}

	if (ftype == PDBURST_FRAME_TYPE_MEAS) {
		/* meas info */
		bzero(meas_info, sizeof(*meas_info));
		bsi->vs_meas_info = meas_info;
		meas_info->flags = FTM_VS_MEAS_F_VALID;
		if (get_next_seq_state(burstp->sm->tof_txcnt) == TOF_SEQ_DONE) {
			/* read and set phy error code here */
			bzero(&tof_info, sizeof(tof_info));
			err = wlc_phy_tof_info(WLC_PI(burstp->wlc), &tof_info,
				WLC_PHY_TOF_INFO_TYPE_PHYERROR, burstp->core);
			if (err != BCME_OK)
				goto done;
			bsi->vs_meas_info->phy_err = tof_info.tof_phy_error;
		}
	}

	if (BURST_SEQ_EN(burstp)) {
		bsi->vs_seq_params = seq;
		bzero(seq, sizeof(*seq));
		seq->flags = FTM_VS_F_VALID;
	}

	if (burstp->flags & WL_PROXD_SESSION_FLAG_SECURE) {
		bsi->vs_sec_params = sec;
		bzero(sec, sizeof(*sec));
		sec->flags = FTM_VS_F_VALID;
		/* other sec params are generated or taken from session */
		bsi->vs_timing_params = timing_params;
	}

	/* MF buf goes in last measure frame of each measurement */
	if (get_next_seq_state(burstp->sm->tof_txcnt) == TOF_SEQ_DONE) {
		bsi->vs_mf_buf_data = burstp->mf_buf;
		bsi->vs_mf_buf_data_max = burstp->mf_buf_len;
		bsi->vs_mf_buf_data_len = burstp->mf_buf_len;
		bsi->vs_mf_buf = mf;
		mf->flags = FTM_VS_F_VALID;
	}

	BCM_REFERENCE(mf);
done:
	return err;
}

/* TOF action frame send function */
static int pdburst_send(pdburst_sm_t *sm, struct ether_addr *da, uint8 type)
{
	wlc_info_t *wlc;
	pdburst_t* burstp = sm->tof_obj;
	ratespec_t rate_override;
	uint16 durid = 60;
	pkcb_fn_t fn = NULL;
	int ret = BCME_ERROR;
	uint16 fc_type;
	pdburst_tsinfo_t tsinfo;
	wlc_bsscfg_t *bsscfg;
	uint8* pbody;
	wlc_pkttag_t *pkttag;
	void *pkt = NULL;
	uint pkt_len;
	uint ftm_len, vs_ie_len = 0;
	pdburst_frame_type_t ftype;
	int txstatus = BCME_OK;
	pdburst_session_info_t bsi;
	ftm_vs_req_params_t req_params;
	ftm_vs_seq_params_t seq_params;
	ftm_vs_sec_params_t sec_params;
	ftm_vs_meas_info_t meas_info;
	ftm_vs_mf_buf_t mf_buf;
	ftm_vs_timing_params_t timing_params;
	wlcband_t *band = NULL;
#ifdef TOF_DEBUG_TIME
	char eabuf[32];
	bcm_ether_ntoa(da, eabuf);
	TOF_PRINTF(("send type %d, %s %d %d\n", type, eabuf, sm->tof_dialog, sm->tof_followup));
#endif

	bzero(&req_params, sizeof(req_params));
	bzero(&seq_params, sizeof(seq_params));
	bzero(&sec_params, sizeof(sec_params));
	bzero(&meas_info, sizeof(meas_info));
	bzero(&mf_buf, sizeof(mf_buf));
	bzero(&timing_params, sizeof(timing_params));

	wlc = burstp->wlc;

#if defined(TOF_PROFILE)
	wlc_read_tsf(wlc, &tsf_lastm, &tsf_hi);
	tsf_lastm -= tsf_start;
#endif
	if (!burstp->svc_funcs) {
		ret =  WL_PROXD_E_PROTO;
		goto done;
	}

	if (burstp->bsscfg) {
		bsscfg = burstp->bsscfg;
	} else {
		if (!(bsscfg = wlc_bsscfg_find_by_hwaddr(wlc, &sm->tof_selfea))) {
#ifdef TOF_DEBUG_TIME
			FTM_ERR(("%s()wl%d: Can't find BSSCFG from matching selfea " MACF "\n",
				__FUNCTION__, wlc->pub->unit,
				ETHERP_TO_MACF(&sm->tof_selfea)));
#endif
			return BCME_ERROR;
		}
	}

	if (!BURST_SEQ_EN(burstp) && (type == TOF_TYPE_MEASURE || type == TOF_TYPE_MEASURE_END)) {
#ifdef TOF_DEBUG_TIME
		TOF_PRINTF(("*** rate spec 0x%08x\n", burstp->configp->ratespec));
#endif
		band = wlc->bandstate[CHSPEC_BANDUNIT(burstp->configp->chanspec)];
		if (RSPEC_ACTIVE(band->rspec_override)) {
			rate_override = band->rspec_override;
		} else {
			rate_override = burstp->configp->ratespec;
		}
	} else {
		rate_override = PROXD_DEFAULT_TX_RATE;
		/* rate_override = 0x02030009; */
	}

	if ((rate_override & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_UNSPECIFIED) {
		if (CHSPEC_IS160(burstp->configp->chanspec))
			rate_override |= WL_RSPEC_BW_160MHZ;
		else if (CHSPEC_IS80(burstp->configp->chanspec))
			rate_override |= WL_RSPEC_BW_80MHZ;
		else if (CHSPEC_IS40(burstp->configp->chanspec))
			rate_override |= WL_RSPEC_BW_40MHZ;
		else if (CHSPEC_IS20(burstp->configp->chanspec))
			rate_override |= WL_RSPEC_BW_20MHZ;
	}

	if (RSPEC_ISVHT(rate_override))
		sm->tof_txvht = TRUE;

	ftype = pdburst_get_ftype(type);
	if (ftype == PDBURST_FRAME_TYPE_REQ) {
		/*   about to send FTM request, set the req flag here.
		 **  Also initialize the num_meas to 0 here.
		*/
		burstp->result_flags |= WL_PROXD_REQUEST_SENT;
		burstp->num_meas = 0;
		burstp->seq_state = TOF_SEQ_NONE;
	}
	else if (ftype != PDBURST_FRAME_TYPE_MEAS) {
		FTM_ERR(("Unknown TOF pkt type	%d\n", type));
		ret = WL_PROXD_E_FRAME_TYPE;
		goto done;
	}
	/* avoid vs_ie for AVB responder */
	if (BURST_SEQ_EN(burstp) || (ftype == PDBURST_FRAME_TYPE_REQ)) {
		ret = pdburst_init_bsi(burstp, ftype, &req_params, &seq_params,
			&sec_params, &mf_buf, &meas_info, &timing_params, &bsi);
		if (ret != BCME_OK)
			goto done;
	}

	ftm_len = PROTOCB(burstp, get_frame_len, (burstp->ctx, ftype, &fc_type));
	/* avoid vs_ie for AVB responder */
	if (BURST_SEQ_EN(burstp) || (ftype == PDBURST_FRAME_TYPE_REQ)) {
		vs_ie_len +=
			PROTOCB(burstp, vs_get_frame_len, (burstp->ctx, ftype, &bsi));
	}

	/* get allocation of action frame */
	pkt = proxd_alloc_action_frame(wlc->pdsvc_info, bsscfg, da,
		&burstp->configp->cur_ether_addr, &burstp->configp->bssid,
		ftm_len + vs_ie_len,
		&pbody, (burstp->flags & WL_PROXD_SESSION_FLAG_SECURE) ?
		DOT11_ACTION_CAT_PDPA: DOT11_ACTION_CAT_PUBLIC, 0);
	if (pkt == NULL) {
		ret = BCME_NOMEM;
		goto done;
	}

	pkttag = WLPKTTAG(pkt);
	if (sm->tof_retrycnt && !CHSPEC_IS5G(burstp->configp->chanspec)) {
		pkttag->flags |= WLF_USERTS;
	}
	pkttag->shared.packetid = burstp->configp->chanspec;
	WLPKTTAGBSSCFGSET(pkt, bsscfg->_idx);

	bzero(&tsinfo, sizeof(tsinfo));
	if (type == TOF_TYPE_REQ_START || type == TOF_TYPE_REQ_END)
	{
		fn = pdburst_tx_complete;

#ifdef TOF_PROFILE
		TOF_PRINTF(("EVENT = %d, TIME = 0x%0x\n", (type+10), tsf_lastm));
#endif
		if (wlc->hw->clk) {
			wlc_update_shm(burstp->wlc, burstp->shmemptr + M_TOF_FLAGS_OFFSET(wlc),
				1 << TOF_RX_FTM_NBIT, 1 << TOF_RX_FTM_NBIT);
		}
	} else if (type == TOF_TYPE_MEASURE_END || type == TOF_TYPE_MEASURE) {
		int nextFrames;

		fn = pdburst_tx_complete;
		nextFrames = pdburst_total_framecnt(burstp);

		if (nextFrames)
			nextFrames = nextFrames- sm->tof_txpktcnt;
		else
			nextFrames = burstp->measurecnt - sm->tof_txcnt;

		if (burstp->sm->tof_legacypeer == TOF_LEGACY_AP)
			nextFrames--;

		durid = wlc_compute_frame_dur(wlc, CHSPEC_BANDTYPE(bsscfg->current_bss->chanspec),
				rate_override, WLC_LONG_PREAMBLE, 0);
		if (nextFrames > 0 && burstp->tunep->rsv_media)
			durid += burstp->tunep->rsv_media;

		if (wlc->hw->clk) {
			wlc_write_shm(wlc, burstp->shmemptr + M_TOF_DOT11DUR_OFFSET(wlc), durid);
		}

		/* mark measurement pkt with special packet ID to identify it later */
		pkttag->shared.packetid |= (PROXD_FTM_PACKET_TAG |
			PROXD_MEASUREMENT_PKTID);
		burstp->delayfree |= TOF_DFREE_TXDONE;

		tsinfo.toa_err = BURST_ERR_PICO;
		tsinfo.tod_err = BURST_ERR_PICO;
		tsinfo.toa = burstp->tx_t4;
		tsinfo.tod = burstp->tx_t1;
		tsinfo.tx_id = sm->tof_dialog;
		tsinfo.ts_id = sm->tof_followup;

		if (type == TOF_TYPE_MEASURE || type == TOF_TYPE_MEASURE_END) {
			burstp->seq_state = get_next_seq_state(burstp->sm->tof_txcnt);
		}
		pdburst_measure(burstp, TOF_RESET);
#ifdef TOF_PROFILE
		TOF_PRINTF(("EVENT = %d, TOKEN=%d FOLLOW_TOKEN=%d TIME = 0x%0x\n", (type+10),
			tsinfo.tx_id, tsinfo.ts_id, tsf_lastm));
#endif
	}

	ret = PROTOCB(burstp, prep_tx, (burstp->ctx, ftype, pkt, pbody,
		(ftm_len + vs_ie_len), &ftm_len, &tsinfo));
	if (ret != BCME_OK) {
		goto done;
	}
	/* avoid vs_ie for AVB responder */
	if (BURST_SEQ_EN(burstp) || (ftype == PDBURST_FRAME_TYPE_REQ)) {
		ret = PROTOCB(burstp, vs_prep_tx, (burstp->ctx,
			ftype, pbody + ftm_len, vs_ie_len, &vs_ie_len, &bsi));
		if (ret != BCME_OK) {
			goto done;
		}
	}
	if (fn) {
		wlc_pcb_fn_register(wlc->pcb, fn, burstp, pkt);
	}

	if (burstp->scanestarted)
		txstatus = WL_PROXD_E_SCAN_INPROCESS;

	/* adjust packet length based on data actually added */
	pkt_len = PKTLEN(wlc->osh, pkt);
	PKTSETLEN(wlc->osh, pkt, pkt_len);

	if (proxd_tx(wlc->pdsvc_info, pkt, bsscfg, rate_override, txstatus)) {
		burstp->txcnt++;
	} else {
		ret = BCME_TXFAIL;
		pkt = NULL; /* will be freed by proxd_tx... */
	}

done:
	if (ret != BCME_OK) {
		if (pkt)
			PKTFREE(wlc->osh, pkt, FALSE);
		FTM_ERR(("wl%d: %s: status %d\n",
			wlc->pub->unit, __FUNCTION__, ret));
	}

	return ret;
}

static bool
pdburst_ts_valid(pdburst_t *burstp, ftmts_t *toftsp, int16 meas_idx, int32 *dT)
{
	int32 k;
	bool ret = TRUE;

	/* For SEQ case, for sample number 0,3,6, etc(SETUP frames), skip dT updation */
	if (BURST_SEQ_EN(burstp)) {
		if ((meas_idx % TOF_DEFAULT_FTMCNT_SEQ == 0) ||
			(meas_idx % TOF_DEFAULT_FTMCNT_SEQ == 2) ||
			(toftsp->discard)) {
			ret = FALSE;
		} else {
			if (*dT < -(burstp->seq_len/2)) {
				*dT += burstp->seq_len;
			} else if (*dT > (burstp->seq_len/2)) {
				*dT -= burstp->seq_len;
			}
			ret = TRUE;
		}
	} else {
		if (toftsp->tsmethod == TS_METHOD_SEQ) {
			 k = 0u;
		} else {
			k = toftsp->k;
		}
		/* For lower bound on RTT, allow -ve measurements that are within tolerance */
		/* Any RTT value below MINVAL is considered as zero measurement */
		if (*dT <= (int32)TOF_RTT_MINVAL_PS) {
			*dT = 0;
			ret = FALSE;
		}
		if (!(burstp->flags & WL_PROXD_SESSION_FLAG_ONE_WAY)) {
			/* Ignore upper bound on RTT during K-value calibration-phase of AVB */
			/* were k values are zero, otherwise discard */
			if (toftsp->tsmethod == TS_METHOD_AVB) {
				if ((k != 0u) && (*dT > (int32)TOF_RTT_MAXVAL_PS)) {
					ret = FALSE;
				}
			} else if (*dT > (int32)TOF_RTT_MAXVAL_PS) {
				ret = FALSE;
			} else if ((toftsp->t3 - toftsp->t2) <= 0) {
				/* Filtering wrong/out-of-bound measurements for T3, T2 */
				ret = FALSE;
			}
		}
		/* Filtering wrong/out-of-bound measurements for T4,T1 */
		if ((toftsp->t4 - toftsp->t1) <= 0) {
			ret = FALSE;
		}

		/* check for phyts calibration phase */
		if ((toftsp->k == 0) && burstp->phyts_setup) {
			/* brcm target fallback to avb */
			if (!(toftsp->t1 == 0 && toftsp->t4 != 0)) {
				ret = FALSE;
			}
			/* initiator fallback to avb */
			if (toftsp->tsmethod != TS_METHOD_SAMPCAP) {
				ret = FALSE;
			}
		}

		/* Filtering AVB measurements based on gd/hadj values */
		if (toftsp->tsmethod == TS_METHOD_AVB) {
			/* Valid gd(weight across all the taps), hadj (gd of firstpath) */
			/* has the below property */
			/* diff (gd - hadj) value will be bounded,  0 =< (gd - hadj) <= 200ns */
			if (((toftsp->gd - toftsp->adj) < TOF_AVB_GD_HADJ_DIF_LOW_LIMIT_NS) ||
				((toftsp->gd - toftsp->adj) > TOF_AVB_GD_HADJ_DIF_HIGH_LIMIT_NS)) {
				ret = FALSE;
			}
		}
	}

	return ret;
}

static int
pdburst_analyze_results_update_ftmp(pdburst_t *burstp, wl_proxd_rtt_sample_t *ftmp,
	ftmts_t *toftsp, int32 dT, wl_proxd_status_t reason)
{
	int ret = BCME_OK;

	ftmp->version = WL_PROXD_RTT_SAMPLE_VERSION_2;
	ftmp->length = sizeof(*ftmp) - OFFSETOF(wl_proxd_rtt_sample_t, id);
	ftmp->id = toftsp->tx_id;
	ftmp->rssi = toftsp->rssi;
	ftmp->ratespec = toftsp->rspec;
	ftmp->snr = toftsp->snr;
	if (toftsp->t1 == 0 && toftsp->t4 != 0) {
		ftmp->flags |= WL_PROXD_RTT_SAMPLE_TGT_PHYTS;
	}
	ftmp->status = (toftsp->discard) ? WL_PROXD_E_INVALIDMEAS : reason;
	ftmp->flags |= (toftsp->discard ? WL_PROXD_RTT_SAMPLE_DISCARD : 0);
	ftmp->flags |= ((toftsp->tsmethod == TS_METHOD_SAMPCAP) ?
		WL_PROXD_RTT_SAMPLE_INIT_PHYTS : 0);

	ftmp->coreid = burstp->core;
	ftmp->chanspec = burstp->configp->chanspec;
	FTM_INIT_INTVL(&ftmp->rtt, dT, WL_PROXD_TMU_PICO_SEC);
	ftmp->tof_phy_error = toftsp->tof_phy_error;

	TOF_PRINTF(("(%d)T1_L %u T2_L %u T3_L %u T4_L %u Delta(ps) %d "
		"T4-T1 %lld T3-T2 %lld avbtx-avbrx %d "
		"rssi %d gd %d hadj %d dif %d %s core %d TS_mode %u\n",
		ftmp->id, (uint32)toftsp->t1, (uint32)toftsp->t2,
		(uint32)toftsp->t3, (uint32)toftsp->t4, dT,
		(toftsp->t4 - toftsp->t1), (toftsp->t3 - toftsp->t2),
		(toftsp->avbtx - toftsp->avbrx), toftsp->rssi,
		toftsp->gd, toftsp->adj, toftsp->gd - toftsp->adj,
		toftsp->discard ? "discard" : "",
		ftmp->coreid, toftsp->tsmethod));

	if (BURST_SEQ_EN(burstp)) {
		wl_proxd_snr_t tof_target_snr = 0;
		wl_proxd_bitflips_t tof_target_bitflips = 0;

		ftmp->bitflips = toftsp->bitflips;
		ftmp->tof_tgt_phy_error = ftm_vs_tgt_snr_bitfips(burstp->ctx,
			burstp->tunep->snr_thresh, burstp->tunep->bitflip_thresh,
			&tof_target_snr, &tof_target_bitflips);
		ftmp->tof_tgt_snr = tof_target_snr;
		ftmp->tof_tgt_bitflips = tof_target_bitflips;
		FTM_TRACE(("snr %d bitflips %d tof_phy_error %x tof_tgt_phy_error %x "
				"tof_target_snr = %d tof_target_bitflips = %d\n",
				ftmp->snr, ftmp->bitflips,
				ftmp->tof_phy_error, ftmp->tof_tgt_phy_error, ftmp->tof_tgt_snr,
				ftmp->tof_tgt_bitflips));
	}

	return ret;
}

static int
pdburst_analyze_results_ftm(pdburst_t *burstp, wl_proxd_rtt_sample_t *ftmp,
		tof_tslist_t *listp, pdburst_results_t *res,
		int32 *dT_arr, uint8 *validts_cnt, wl_proxd_status_t reason)
{
	ftmts_t *toftsp = &listp->tslist[0];
	uint8 ftm_idx;
	uint16 decimal_fac;
	int32 dT = 0;
	int ret = BCME_OK;
	uint8 *dT_method_arr = (uint8 *)(dT_arr + listp->tscnt);

	res->avg_rtt.rssi = 0;
	*validts_cnt = 0u;
	ftm_idx = 0u;
	while (ftm_idx < listp->tscnt) {
		/* RTT computation */
		if (BURST_SEQ_EN(burstp)) {
			dT = (((uint32)(toftsp->t4 - toftsp->t1)) % burstp->seq_len) -
				(uint32)(toftsp->t3 - toftsp->t2);
		} else {
			dT = (int32)((toftsp->t4 - toftsp->t1) - (toftsp->t3 - toftsp->t2));
		}
		/* Resolution of sampcap - 1ps, avb - 1ns, seq - 0.1ns */
		decimal_fac = (toftsp->tsmethod == TS_METHOD_SAMPCAP) ? (1u) :
			((toftsp->tsmethod == TS_METHOD_AVB) ? (1000u) : (100u));
		/* dT is converted into pico sec, independent of ts method */
		dT = dT * decimal_fac;
		/* Ts validity check */
		if (pdburst_ts_valid(burstp, toftsp, ftm_idx, &dT)) {
			dT_arr[*validts_cnt] = dT;
			dT_method_arr[*validts_cnt] = toftsp->tsmethod;
			ftmp->distance = dT;
			(*validts_cnt)++;
		} else {
			toftsp->discard = TRUE;
			ftmp->flags |= WL_PROXD_RTT_SAMPLE_DISCARD;
			ftmp->status =  WL_PROXD_E_INVALIDMEAS;
		}

		/* Update ftmp */
		ret = pdburst_analyze_results_update_ftmp(burstp, ftmp, toftsp, dT, reason);
		if (ret != BCME_OK) {
			goto done;
		}
		/* Average rssi */
		res->avg_rtt.rssi += ftmp->rssi;

#ifdef WL_PROXD_PHYTS_DEBUG
		if (phyts_ftmts_idx < TS_CNT_DBG_MAX) {
			memcpy(&phyts_ftmts_dbg[phyts_ftmts_idx],
				toftsp, sizeof(ftmts_t));
			FTM_ERR(("TS DBG Collect idx %d\n", phyts_ftmts_idx));
			phyts_ftmts_idx++;
		} else {
			phyts_ftmts_idx = 0u;
		}
#endif /* WL_PROXD_PHYTS_DEBUG */
		/* Process next ftm */
		ftm_idx++;
		toftsp++;
		ftmp++;
	}
	TOF_PRINTF(("Valid TS Cnt %d\n", *validts_cnt));
	/* Compute average RSSI */
	res->avg_rtt.rssi = res->avg_rtt.rssi * (-10)/listp->tscnt;
	if ((res->avg_rtt.rssi%10) >= 5) {
		res->avg_rtt.rssi = -res->avg_rtt.rssi/10 - 1;
	} else {
		res->avg_rtt.rssi = -res->avg_rtt.rssi/10;
	}
	burstp->avgrssi = res->avg_rtt.rssi;
	/* Update avg results */
	res->avg_rtt.ratespec = listp->tslist[0].rspec;
	res->avg_rtt.chanspec = burstp->configp->chanspec;
	res->avg_rtt.version = WL_PROXD_RTT_SAMPLE_VERSION_2;
	res->avg_rtt.length = sizeof(wl_proxd_rtt_sample_t) -
			OFFSETOF(wl_proxd_rtt_sample_t, id);

done:
	return ret;
}

#ifdef WL_PROXD_OUTLIER_FILTERING
/* Median Absolute Deviatin (MAD) Scaling Factor */
/* 1.25 (5/4) -> 78% CDF point, lower value more percentage */
#define MAD_SCALE 5u
#define MAD_SHIFT 2u
#define MAD_RANGE 5u
/* Bound is decided as (Median - (MAD_RANGE*MAD)) to (Median + (MAD_RANGE*MAD)) */

static int32
pdburst_filter_outliers(pdburst_t* burstp, int32 *arr, uint8 *n)
{
	int i, j;
	int32 median, mad;
	int32 *local_arr;
	int32 lower_bound, upper_bound;
	int16 cnt = *n;
	uint16 cnt_u;

	if (cnt > 2) {
		cnt_u = (uint16) cnt;
		local_arr = (int32 *)MALLOCZ(burstp->wlc->osh, sizeof(int32) * cnt);
		/* If memory not allocated, just return the average value without */
		/* any outlier filtering */
		if (local_arr != NULL) {
			/* Sort array in ascending order */
			wlc_pdsvc_sortasc(arr, cnt_u);
			/* Median of the set */
			median = wlc_pdsvc_median(arr, cnt_u);
			/* Absolute deviation from median */
			for (i = 0; i < cnt; i++) {
				local_arr[i] = ABS((signed)(arr[i] - median));
			}
			/* Sort the absolute deviation and then find the median */
			/* Also known as Median Absolute Deviation (MAD) */
			/* MAD = b * Median(abs(xi - Mi)) */
			wlc_pdsvc_sortasc(local_arr, cnt_u);
			mad = wlc_pdsvc_median(local_arr, cnt_u);
			mad = MAD_SCALE * mad >> MAD_SHIFT;
			if (mad == 0) {
				mad = 1;
			}
			lower_bound = median - (MAD_RANGE * mad);
			upper_bound = median + (MAD_RANGE * mad);
			TOF_PRINTF(("Mad=%d, Lower/Upper Limit =%d/%d\n", mad, lower_bound,
				upper_bound));
			/* Filter any outliers out of these bounds */
			j = 0;
			for (i = 0; i < cnt; i++) {
				if (arr[i] >= lower_bound && arr[i] <= upper_bound) {
					local_arr[j] = arr[i];
					j++;
				}
			}
			/* Copy the filtered measurements and free the local_arr */
			memcpy(arr, local_arr, sizeof(int32) * j);
			MFREE(burstp->wlc->osh, local_arr, sizeof(int32) * cnt);
			*n = j;
			cnt = *n;
		}
	}

	return wlc_pdsvc_average(arr, cnt);
}
#endif /* WL_PROXD_OUTLIER_FILTERING */

#ifdef WL_PROXD_PHYTS
/* If the burst has a mix of +ve and -ve measurements then */
/* filter the -ve measurements */
static int
pdburst_filter_neg_measurements(int32 *dT_arr, uint8 *n)
{
	bool pos_dist = FALSE, neg_dist = FALSE;
	uint8 dT_idx = 0, dT_idx_fil = 0;
	int ret = BCME_OK;

	for (dT_idx = 0; dT_idx < *n; dT_idx++) {
		if (dT_arr[dT_idx] > 0) {
			pos_dist = TRUE;
			break;
		}
	}
	for (dT_idx = 0; dT_idx < *n; dT_idx++) {
		if (dT_arr[dT_idx] < 0) {
			neg_dist = TRUE;
			break;
		}
	}
	if (pos_dist && neg_dist) {
		for (dT_idx = 0; dT_idx < *n; dT_idx++) {
			if (dT_arr[dT_idx] > TOF_RTT_VALIDVAL_PS) {
				dT_arr[dT_idx_fil] = dT_arr[dT_idx];
				dT_idx_fil++;
			}
		}
		*n = dT_idx_fil;
	}

	return ret;
}
/* Filter PHYTS and non-PHYTS measurements */
static int
pdburst_filter_measurements(pdburst_t* burstp, int32 *dT_arr, uint8 *dT_method_arr, uint8 *n)
{
	int32 *dT_arr_1 = NULL;
	uint8 mem_size = *n;
	uint8 dT_idx = 0, phyts_cnt = 0, non_phyts_cnt = 0;
	int ret = BCME_OK;

	/* Avoid filtering if valid measurements are less */
	if (*n < 2u) {
		goto done;
	}
	/* Filter PHYTS and non-PHYTS measurements */
	dT_arr_1 = (int32 *)MALLOCZ(burstp->wlc->osh, sizeof(int32) * mem_size);
	if (dT_arr_1 == NULL) {
		ret = BCME_NOMEM;
		goto done;
	}
	while (dT_idx < *n) {
		if (dT_method_arr[dT_idx] == TS_METHOD_SAMPCAP) {
			dT_arr[phyts_cnt] = dT_arr[dT_idx];
			FTM_TRACE(("pts%d meas%d\n", phyts_cnt, dT_arr[phyts_cnt]));
			phyts_cnt++;
		} else {
			dT_arr_1[non_phyts_cnt] = dT_arr[dT_idx];
			FTM_TRACE(("nonpts%d meas%d\n", non_phyts_cnt, dT_arr_1[non_phyts_cnt]));
			non_phyts_cnt++;
		}
		dT_idx++;
	}
	/* Decide Ts technique based on majority */
	if (phyts_cnt >= non_phyts_cnt) {
		burstp->phyts_meas = TRUE;
		*n = phyts_cnt;
	} else {
		burstp->phyts_meas = FALSE;
		*n = non_phyts_cnt;
		memcpy(dT_arr, dT_arr_1, non_phyts_cnt * sizeof(int32));
	}
	ret = pdburst_filter_neg_measurements(dT_arr, n);
done:
	if (dT_arr_1 != NULL) {
		MFREE(burstp->wlc->osh, dT_arr_1, sizeof(int32) * mem_size);
	}
	return ret;
}
#endif /* WL_PROXD_PHYTS */

/* calculate the distance based on measure results */
/* STD base value PHYTS 1ns, AVB 5.6ns */
#define STD_DECIMALDIGITS 1u
#define STD_BASEVAL_PHYTS (240u * (10u * STD_DECIMALDIGITS))
#define STD_BASEVAL_AVB (1000u * (10u * STD_DECIMALDIGITS))
#define STD_BASEVAL(phyts_meas) ((phyts_meas) ? STD_BASEVAL_PHYTS : STD_BASEVAL_AVB)
static int
pdburst_analyze_results(pdburst_t* burstp, tof_tslist_t *listp,
	pdburst_results_t *res, int32 *difavg, wl_proxd_status_t reason)
{
	uint8 num_valid_rtt;
	int32 *dT_arr = NULL;
	int32 avg_rtt = 0;
	uint32 sigma;
	uint16 dT_arr_size = 0;
	wl_proxd_rtt_sample_t *ftmp = &res->rtt[0];
	int ret = BCME_OK;

	if (listp && listp->tscnt && listp->tslist && ftmp) {
		/* reserve space to store corresponding timestamp method (uint8) */
		/* along with delta value (dT) = ((int32) + (uint8)) x n */
		dT_arr_size = (sizeof(int32) + sizeof(uint8)) * listp->tscnt;
		dT_arr = (int32 *)MALLOCZ(burstp->wlc->osh, dT_arr_size);
		if (dT_arr == NULL) {
			ret = BCME_NOMEM;
			goto done;
		}
		ret = pdburst_analyze_results_ftm(burstp, ftmp, listp, res, dT_arr,
			&num_valid_rtt, reason);
		if (ret != BCME_OK) {
			goto done;
		}
		/* Update number of valid measurements */
		res->num_valid_rtt = num_valid_rtt;
		burstp->frmcnt = num_valid_rtt;
#ifdef WL_PROXD_PHYTS
		if (num_valid_rtt > 1u) {
			ret = pdburst_filter_measurements(burstp, dT_arr,
				(uint8 *)(dT_arr + listp->tscnt), &num_valid_rtt);
			if (ret != BCME_OK) {
				goto done;
			}
		}
#endif /* WL_PROXD_PHYTS */
#ifdef WL_PROXD_OUTLIER_FILTERING
		avg_rtt = pdburst_filter_outliers(burstp, dT_arr, &num_valid_rtt);
#else
		avg_rtt = wlc_pdsvc_average(dT_arr, num_valid_rtt);
#endif /* WL_PROXD_OUTLIER_FILTERING  */
		if (avg_rtt < 0) {
			avg_rtt = -avg_rtt;
		}
		burstp->meanrtt = avg_rtt;
		FTM_INIT_INTVL(&res->avg_rtt.rtt, avg_rtt, WL_PROXD_TMU_PICO_SEC);
		/* Compute SD on the filtered valid measurements */
		sigma = wlc_pdsvc_deviation(dT_arr, avg_rtt, num_valid_rtt, STD_DECIMALDIGITS);
#ifdef WL_PROXD_PHYTS
		/* Cap the sigma value to a minimum base value, based on the std */
		/* computated on population set in an ideal environment */
		sigma = (sigma > STD_BASEVAL(burstp->phyts_meas)) ?
			(sigma) : (STD_BASEVAL(burstp->phyts_meas));
#endif /* WL_PROXD_PHYTS */
		/* sigma is in 0.1ps resolution, convert it to 0.1ns resolution */
		res->sd_rtt = sigma/1000u;
		burstp->sdrtt = sigma/1000u;

		FTM_TRACE(("FiltValCnt %d RTT(ps) %d std(ps) %d.%d avgrssi(dBm) %d\n",
			num_valid_rtt, avg_rtt, sigma/10, sigma%10, burstp->avgrssi));
		/* Consider only (T4-T1) for ONE WAY RTT */
		if (burstp->flags & WL_PROXD_SESSION_FLAG_ONE_WAY) {
			ret = TOF_PS_TO_256THMETER_ONEWAY(avg_rtt);
		} else {
			ret = TOF_PS_TO_256THMETER(avg_rtt);
		}
	} else {
		res->avg_rtt.rssi = 0;
		ret = BCME_ERROR;
	}

#ifdef WL_PROXD_PHYTS_DEBUG
	memcpy(&phyts_burstp_dbg, burstp, sizeof(pdburst_t));
#endif /* WL_PROXD_PHYTS_DEBUG */

done:
	if (dT_arr != NULL) {
		MFREE(burstp->wlc->osh, dT_arr, dT_arr_size);
	}
	return ret;
}

#ifdef TOF_COLLECT_REMOTE
/* generate TOF events */
static void pdburst_event(pdburst_sm_t *sm, uint8 eventtype)
{
	pdburst_t* burstp = sm->tof_obj;

	if (burstp->collectp && eventtype == WLC_E_PROXD_START) {
		burstp->collectp->collect_cnt = 0;
	}
}
#endif /* TOF_COLLECT_REMOTE */

/* TOF get report results and state machine goes to CONFIRM state */
static void pdburst_report_done(pdburst_sm_t *sm, wl_proxd_status_t reason)
{
	pdburst_t* burstp = sm->tof_obj;

	sm->tof_state = TOF_STATE_ICONFIRM;

	pdburst_deactivate_pm(burstp);

	if (burstp->flags & WL_PROXD_SESSION_FLAG_NETRUAL) {
		pdburst_reset(sm, WL_PROXD_MODE_TARGET, WL_PROXD_E_NOTSTARTED);
	}

	MDBG(("TS:%d should be in pwr_down now\n", get_usts(burstp->wlc)));

}

static int pdburst_target_done(pdburst_sm_t *sm, wl_proxd_status_t reason)
{
	pdburst_t* burstp = sm->tof_obj;

#ifdef TOF_DEBUG_TIME
	switch (reason) {
		case WL_PROXD_E_OK:
			TOF_PRINTF(("OK\n"));
			break;
		case WL_PROXD_E_TIMEOUT:
			TOF_PRINTF(("TIMEOUT\n"));
			break;
		case WL_PROXD_E_NOACK:
			TOF_PRINTF(("NOACK\n"));
			break;
		case WL_PROXD_E_INVALIDMEAS:
			TOF_PRINTF(("INVALIDMEAS\n"));
			break;
		case WL_PROXD_E_CANCELED:
			TOF_PRINTF(("ABORT\n"));
			break;
		default:
			TOF_PRINTF(("ERROR\n"));
			break;
	}
#endif /* TOF_DEBUG_TIME */

#ifdef WL_RANGE_SEQ
	if (!burstp) {
		ASSERT(0);
		/* No point proceeding further if burstp is invalid */
		OSL_SYS_HALT();
	}
#endif /* WL_RANGE_SEQ */

	phy_tof_cmd(WLC_PI(burstp->wlc), FALSE, 0);
	if (FTM_BSSCFG_SECURE(wlc_ftm_get_handle(burstp->wlc), burstp->bsscfg)) {
		phy_rxgcrs_sel_classifier(WLC_PI(burstp->wlc),
			TOF_CLASSIFIER_BPHY_ON_OFDM_ON);
	}

	if (burstp->ftm_tx_timer_active) {
		wlc_hrt_del_timeout(burstp->ftm_tx_timer);
		burstp->ftm_tx_timer_active = FALSE;
	}

	/* stop HRT duration timer if it is running */
	if (burstp->duration_timer_active) {
		wlc_hrt_del_timeout(burstp->duration_timer);
		burstp->duration_timer_active = FALSE;
	}

	if (burstp->collectp != NULL && burstp->collectp->configp != NULL) {
		memcpy(burstp->collectp->configp, burstp->configp, sizeof(*burstp->configp));
	}
#ifdef TOF_COLLECT
#ifdef WL_RANGE_SEQ
	if (burstp->collectp) {
#else
	if (burstp && burstp->collectp) {
#endif
		(void)pdburst_collect_event(burstp);
#ifdef WL_RANGE_SEQ
		burstp->collectp->collect_cnt = 0;
#endif
	}
#endif /* TOF_COLLECT */

	/*
	*In burst_done call sequence, pdburst_destroy is called
	*in case of single burst or last burst which will FREE the
	*burstp struct. So any access to burstp
	*should be done before burst_done
	*/
	burstp->delayfree |= TOF_DFREE_INSVCCB;
	(void)PROTOCB(burstp, done, (burstp->ctx, NULL));
	burstp->delayfree &= ~TOF_DFREE_INSVCCB;
	if (burstp->destroyed && !burstp->delayfree) {
		MFREE(burstp->wlc->osh, burstp, sizeof(pdburst_t));
	} else {
		burstp->smstoped = TRUE;
	}
	return TOF_RET_SLEEP;
}

static int pdburst_result(pdburst_t* burstp, wl_proxd_status_t reason)
{
	tof_tslist_t *listp = &burstp->tof_tslist;
	int32 dif = 0;
	int distance;
	pdburst_results_t *res;
	int len = OFFSETOF(pdburst_results_t, rtt);
#ifdef TOF_DEBUG_TIME2
	uint32 t;
#endif

	len += sizeof(wl_proxd_rtt_sample_t)*listp->tscnt;

	res = (pdburst_results_t *)MALLOCZ(burstp->wlc->osh, len);
	if (res == NULL) {
		return BCME_NOMEM;
	}

	distance = pdburst_analyze_results(burstp, listp, res, &dif, reason);
	if (distance == BCME_ERROR) {
		res->dist_unit = PD_DIST_UNKNOWN;
	} else {
		res->dist_unit = PD_DIST_1BY256M;
	}

	res->num_rtt = listp->tscnt;
	res->status = reason;
	if (!res->num_rtt && reason == WL_PROXD_E_NOACK)
		res->flags = WL_PROXD_RESULT_FLAG_FATAL;
	else {
		res->flags = WL_PRXOD_RESULT_FLAG_NONE;
		if (BURST_IS_HEACK(burstp) && RSPEC_ISHE(burstp->configp->ratespec)) {
			res->flags |= WL_PROXD_RESULT_FLAG_HEACK; /* with he ack */
		} else if (BURST_IS_VHTACK(burstp) && RSPEC_ISVHT(burstp->configp->ratespec)) {
			res->flags |= WL_PROXD_RESULT_FLAG_VHTACK; /* with vhtack */
		}
	}

	res->flags |= burstp->result_flags;
	res->num_meas = burstp->num_meas;

	if (burstp->frmcnt == 0 && burstp->sm->tof_reason == WL_PROXD_E_OK) {
		burstp->sm->tof_reason = WL_PROXD_E_INVALIDMEAS;
		res->status = WL_PROXD_E_INVALIDMEAS;
	}
#ifdef TOF_DEBUG_TIME
	if (distance == BCME_ERROR) {
		TOF_PRINTF(("Distance -1 meter\n"));
	} else {
		TOF_PRINTF(("Distance %d.%04d meter\n", (distance >> 8),
			(((distance & 0xff) >> 4) * 625)));
	}
#endif

	burstp->distance = distance;
	res->avg_dist = distance;

#ifdef TOF_DEBUG_TIME2
	wlc_read_tsf(burstp->wlc, &t, &tsf_hi);
	t = t -tsf_start;
	TOF_PRINTF(("Scan %d Txreq %d Rxack %d 1stM %d ", tsf_scanstart, tsf_txreq,
		tsf_rxack, tsf_rxm));
	TOF_PRINTF(("lastM %d Confirm %d Event %d tmo %d\n", tsf_lastm, tsf_confirm, t, tsf_tmo));
#endif

	if (!(burstp->delayfree & TOF_DFREE_TXDONE)) {
		/* De-register callback at the end of every burst */
		wlc_pcb_fn_find(burstp->wlc->pcb, pdburst_tx_complete, burstp, TRUE);
	}

	burstp->delayfree |= TOF_DFREE_INSVCCB;
	(void)PROTOCB(burstp, done, (burstp->ctx, res));
	burstp->delayfree &= ~TOF_DFREE_INSVCCB;
	MFREE(burstp->wlc->osh, res, len);

	return BCME_OK;
}

void pdburst_cancel(pdburst_t* burstp, wl_proxd_status_t reason)
{
	ASSERT(burstp != NULL);
	(void)pdburst_confirmed(burstp->sm, reason);
}

/* TOF get final results and state machine goes to CONFIRM state */
static int pdburst_confirmed(pdburst_sm_t *sm, wl_proxd_status_t reason)
{
	pdburst_t* burstp = sm->tof_obj;
#ifdef WL_RANGE_SEQ
	wlc_info_t *wlc;
#else
	wlc_info_t *wlc = burstp->wlc;
#endif
	int tofret = TOF_RET_END;

#ifdef WL_RANGE_SEQ
	if (!burstp) {
		ASSERT(0);
		/* No point proceeding further if burstp is invalid */
		OSL_SYS_HALT();
	}
	wlc = burstp->wlc;
#endif
	pdburst_measure(burstp, TOF_RESET);
	if (burstp->smstoped)
		return tofret;

#ifdef TOF_DEBUG_TIME2
	wlc_read_tsf(burstp->wlc, &tsf_confirm, &tsf_hi);
	tsf_confirm = tsf_confirm - tsf_start;
#endif

	sm->tof_state = TOF_STATE_ICONFIRM;
	if (burstp->collectp != NULL && burstp->collectp->configp != NULL) {
		memcpy(burstp->collectp->configp, burstp->configp, sizeof(*burstp->configp));
	}

	pdburst_deactivate_pm(burstp);

	sm->tof_reason = reason;
	pdburst_hw(burstp, FALSE, FALSE);

#ifdef TOF_DEBUG_TIME
	switch (reason) {
		case WL_PROXD_E_OK:
			TOF_PRINTF(("OK\n"));
			break;
		case WL_PROXD_E_TIMEOUT:
			TOF_PRINTF(("TIMEOUT\n"));
			break;
		case WL_PROXD_E_NOACK:
			TOF_PRINTF(("NOACK\n"));
			break;
		case WL_PROXD_E_INVALIDMEAS:
			TOF_PRINTF(("INVALIDMEAS\n"));
			break;
		case WL_PROXD_E_CANCELED:
			TOF_PRINTF(("ABORT\n"));
			break;
		default:
			TOF_PRINTF(("ERROR\n"));
			break;
	}
#endif /* TOF_DEBUG_TIME */
	if (burstp->timeractive) {
		burstp->timeractive = FALSE;
		wl_del_timer(burstp->wlc->wl, burstp->timer);
	}

	pdburst_send_mf_stats_event(burstp);

#ifdef TOF_COLLECT_REMOTE
	if (burstp->collectp && burstp->collectp->remote_request) {
		/* Remote collect data section is done */
		burstp->collectp->remote_request = FALSE;
		return tofret;
	}
#endif /* TOF_COLLECT_REMOTE */

#ifdef TOF_COLLECT
#ifdef WL_RANGE_SEQ
	if (burstp->collectp) {
#else
	if (burstp && burstp->collectp) {
#endif
		(void)pdburst_collect_event(burstp);
#ifdef WL_RANGE_SEQ
		burstp->collectp->collect_cnt = 0;
#endif
	}
#endif /* TOF_COLLECT */

	if (burstp->svc_funcs && burstp->svc_funcs->done && burstp->configp) {
		(void)pdburst_result(burstp, reason);
		pdburst_pwron(burstp, FALSE);
		ftm_vs_reset_target_side_data(burstp->ctx);
		if (burstp->destroyed && !burstp->delayfree) {
#ifdef TOF_DEBUG_TIME
			TOF_PRINTF(("delayfree burstp %p confirmed\n", OSL_OBFUSCATE_BUF(burstp)));
#endif
			MFREE(wlc->osh, burstp, sizeof(pdburst_t));
		}
	}

	if (reason != WL_PROXD_E_OK) {
		FTM_ERR(("%s: reason %d\n", __FUNCTION__, reason));
	}
	return tofret;
}

#ifdef TOF_COLLECT
static int pdburst_collect_mem(wlc_info_t *wlc, pdburst_collect_t **collect, bool alloc)
{

	int i;
	pdburst_collect_t *collectp = *collect;

	if (alloc) {
		collectp = MALLOCZ(wlc->osh, sizeof(pdburst_collect_t));
		if (!collectp) {
			FTM_ERR(("MALLOC failed %s\n", __FUNCTION__));
			*collect = NULL;
			return BCME_NOMEM;
		}
		collectp->collect_cnt = 0;
		collectp->collect_size = MAX_COLLECT_COUNT;
		collectp->collect = MALLOCZ(wlc->osh, collectp->collect_size *
			sizeof(wl_proxd_collect_data_t));
		if (collectp->collect) {
			/* initialize wl_proxd_collect_data_t version and length */
			for (i = 0; i < collectp->collect_size; i++) {
				collectp->collect[i].version = WL_PROXD_COLLECT_DATA_VERSION_3;
				collectp->collect[i].len = sizeof(wl_proxd_collect_data_t) -
					OFFSETOF(wl_proxd_collect_data_t, info);
			}
		}
#ifdef WL_RANGE_SEQ
		collectp->collect_buf = MALLOCZ(wlc->osh, K_TOF_COLLECT_HRAW_SIZE_20MHZ *
#else
		collectp->collect_buf = MALLOCZ(wlc->osh, K_TOF_COLLECT_HRAW_SIZE_80MHZ *
#endif
			sizeof(uint32));
		collectp->collect_header = MALLOCZ(wlc->osh, sizeof(wl_proxd_collect_header_t));
		collectp->chan = MALLOCZ(wlc->osh,
				(PHY_CORE_MAX + 1) * K_TOF_COLLECT_CHAN_SIZE * sizeof(uint32));
		collectp->configp = MALLOCZ(wlc->osh, sizeof(*collectp->configp));
#ifdef WL_RANGE_SEQ
		collectp->collect_method |= WL_PROXD_COLLECT_METHOD_TYPE_EVENT;
#endif /* WL_RANGE_SEQ */
	}

	if (!alloc || !collectp->collect || !collectp->collect_buf ||
		!collectp->collect_header || !collectp->chan || !collectp->configp) {
		if (collectp->collect) {
			MFREE(wlc->osh, collectp->collect, (collectp->collect_size) *
				sizeof(wl_proxd_collect_data_t));
		}
		if (collectp->collect_buf) {
			MFREE(wlc->osh, collectp->collect_buf,
#ifdef WL_RANGE_SEQ
				K_TOF_COLLECT_HRAW_SIZE_20MHZ * sizeof(uint32));
#else
				K_TOF_COLLECT_HRAW_SIZE_80MHZ * sizeof(uint32));
#endif /* WL_RANGE_SEQ */
		}
		if (collectp->collect_header) {
			MFREE(wlc->osh, collectp->collect_header,
				sizeof(wl_proxd_collect_header_t));
		}
		if (collectp->chan) {
			MFREE(wlc->osh, collectp->chan,
				(PHY_CORE_MAX + 1) * K_TOF_COLLECT_CHAN_SIZE * sizeof(uint32));
		}

		if (collectp->configp) {
			MFREE(wlc->osh, collectp->configp,
				sizeof(*collectp->configp));
		}
		collectp->collect_size = 0;
#ifdef WL_RANGE_SEQ
		MFREE(wlc->osh, collectp, sizeof(pdburst_collect_t));
#endif
		if (alloc) {
			FTM_ERR(("%s: MALLOC failed\n", __FUNCTION__));
		}
		*collect = NULL;
		return BCME_NOMEM;
	}

	*collect = collectp;
	return BCME_OK;
}
#endif /* TOF_COLLECT */
#ifdef TOF_COLLECT_REMOTE
static int pdburst_collect_send(pdburst_sm_t *sm, struct ether_addr *da, uint8 type)
{
	wlc_info_t *wlc;
	pdburst_t* burstp = sm->tof_obj;
	pdburst_collect_t *collectp = burstp->collectp;
	ratespec_t rate_override;
	pkcb_fn_t fn = NULL;
	int ret = BCME_ERROR;
	wlc_bsscfg_t *bsscfg;
	uint8* pbody;
	void *pkt;
	int len;
	pdburst_collect_frm_t *tof_hdr;
	wl_action_frame_t *af;
	TOF_PRINTF(("send type %d, " MACF " %d %d\n", type, ETHERP_TO_MACF(da), sm->tof_dialog,
		sm->tof_followup));

	if (!burstp->svc_funcs || !collectp)
		return BCME_ERROR;

	wlc = burstp->wlc;

	if (burstp->bsscfg) {
		bsscfg = burstp->bsscfg;
	} else {
		bsscfg = wlc_bsscfg_find_by_hwaddr(wlc, &sm->tof_selfea);
	}

	rate_override = PROXD_DEFAULT_TX_RATE;

	if ((rate_override & WL_RSPEC_BW_MASK) == WL_RSPEC_BW_UNSPECIFIED) {
		if (CHSPEC_IS160(burstp->configp->chanspec))
			rate_override |= WL_RSPEC_BW_160MHZ;
		else if (CHSPEC_IS80(burstp->configp->chanspec))
			rate_override |= WL_RSPEC_BW_80MHZ;
		else if (CHSPEC_IS40(burstp->configp->chanspec))
			rate_override |= WL_RSPEC_BW_40MHZ;
		else if (CHSPEC_IS20(burstp->configp->chanspec))
			rate_override |= WL_RSPEC_BW_20MHZ;
	}

	if ((af = (wl_action_frame_t *)MALLOCZ(wlc->osh, WL_WIFI_AF_PARAMS_SIZE)) == NULL)
		return BCME_NOMEM;

	tof_hdr = (pdburst_collect_frm_t *)af->data;
	len = sizeof(pdburst_collect_frm_t);
	if (type == TOF_TYPE_COLLECT_DATA_END || type == TOF_TYPE_COLLECT_DATA) {
		wl_proxd_collect_query_t query;

		bzero(&query, sizeof(query));
		query.method = htol32(PROXD_TOF_METHOD);

		if (collectp->remote_cnt == 0) {
			query.request = PROXD_COLLECT_QUERY_HEADER;
			ret = pdburst_collection(burstp->wlc, collectp, &query,
				tof_hdr->data, ACTION_FRAME_SIZE, &tof_hdr->length);
		} else {
			if (!collectp->collect ||
				collectp->remote_cnt > collectp->collect_cnt ||
				collectp->remote_cnt > collectp->collect_size) {
				MFREE(wlc->osh, af, WL_WIFI_AF_PARAMS_SIZE);
				return BCME_ERROR;
			}

			query.request = PROXD_COLLECT_QUERY_DATA;
			query.index = collectp->remote_cnt - 1;
			ret = pdburst_collection(burstp->wlc, collectp, &query,
				tof_hdr->data, ACTION_FRAME_SIZE, &tof_hdr->length);
		}

		if (ret != BCME_OK || tof_hdr->length <= 0) {
			MFREE(wlc->osh, af, WL_WIFI_AF_PARAMS_SIZE);
			return BCME_ERROR;
		}
		len	+= tof_hdr->length - 1;
	}

	/* get allocation of action frame */
	if ((pkt = proxd_alloc_action_frame(wlc->pdsvc_info, bsscfg, da,
		&burstp->configp->cur_ether_addr, &burstp->configp->bssid, len,
		&pbody, DOT11_ACTION_CAT_VS, BRCM_FTM_VS_AF_TYPE)) == NULL) {
		return BCME_NOMEM;
	}

	/* copy action frame payload */
	tof_hdr->tof_type = type;
	tof_hdr->category = DOT11_ACTION_CAT_VS;
	memcpy(tof_hdr->OUI, BRCM_PROP_OUI, DOT11_OUI_LEN);
	tof_hdr->type = BRCM_FTM_VS_AF_TYPE;
	tof_hdr->subtype = BRCM_FTM_VS_COLLECT_SUBTYPE;
	if (type == TOF_TYPE_COLLECT_DATA_END || type == TOF_TYPE_COLLECT_DATA) {
		if (type == TOF_TYPE_COLLECT_DATA && collectp->collect &&
			collectp->remote_cnt < collectp->collect_cnt)
			tof_hdr->tof_type = TOF_TYPE_COLLECT_DATA;
		else
			tof_hdr->tof_type = TOF_TYPE_COLLECT_DATA_END;
		tof_hdr->index = collectp->remote_cnt;
	}
	bcopy(af->data, pbody, len);

	fn = pdburst_tx_complete;
	wlc_pcb_fn_register(wlc->pcb, fn, burstp, pkt);

	if (proxd_tx(wlc->pdsvc_info, pkt, bsscfg, rate_override, 0)) {
		burstp->txcnt++;
		ret = BCME_OK;
	} else {
		ret = BCME_TXFAIL;
		FTM_ERR(("%s tx failed\n", __FUNCTION__));
	}

	MFREE(wlc->osh, af, WL_WIFI_AF_PARAMS_SIZE);
	return ret;
}
#endif /* TOF_COLLECT_REMOTE */

#ifdef TOF_COLLECT
/* TOF collects debug data */
static void pdburst_collect_data(void* collect, int index, bool isTwenty)
{
	pdburst_collect_t * collectp = collect;
	int collect_h_size = 0;
	if (collectp) {
#ifdef WL_RANGE_SEQ
		pdburst_t *pdburstp = collectp->pdburstp;
		wl_proxd_collect_data_t *p_collect, *p_collect_end;
		uint32 *p_buf = collectp->collect_buf;
		int n = 0, n_total = collectp->collect_info.nfft;
#endif /* WL_RANGE_SEQ */

		if (isTwenty) {
			collect_h_size = K_TOF_COLLECT_H_SIZE_20MHZ;
		} else {
			collect_h_size = K_TOF_COLLECT_H_SIZE_80MHZ;
		}

#ifdef WL_RANGE_SEQ
		if (pdburstp && BURST_SEQ_EN(pdburstp) && !index) {
			/* Ignore toast first measurement */
			return;
		}
#else
		wl_proxd_collect_data_t *p_collect, *p_collect_z, *p_collect_end;
		uint32 *p_buf = collectp->collect_buf;
		int n = 0, n_total = collectp->collect_info.nfft;
#endif /* WL_RANGE_SEQ */
		/* Only advance log if id changed */
		p_collect = collectp->collect + collectp->collect_cnt;
#ifndef WL_RANGE_SEQ
		if (collectp->collect_cnt > 0) {
			p_collect_z = p_collect - 1;
			while ((p_collect_z != collectp->collect) && (p_collect_z->info.index)) {
				p_collect_z--;
			}
			if (p_collect_z->info.tof_id == collectp->collect_info.tof_id)
				p_collect = p_collect_z;
		}
#endif /* !WL_RANGE_SEQ */
		p_collect_end = collectp->collect + collectp->collect_size;
		collectp->collect_cnt = (p_collect - collectp->collect);

		collectp->collect_info.index = 0;

#ifdef WL_RANGE_SEQ
		/* The p_collect->H allocated here is for 20MHz only.
		 * The collect_h_size could be more for 80MHz.
		 */
		if (isTwenty) {
#endif
		while (p_collect < p_collect_end) {
			collectp->collect_info.nfft = n_total - n;
			if (collectp->collect_info.nfft > collect_h_size)
				collectp->collect_info.nfft = collect_h_size;
			bcopy((void*)&collectp->collect_info, (void*)&p_collect->info,
				sizeof(wl_proxd_collect_info_t));
			bcopy((void*)(p_buf + n), (void*)&p_collect->H,
				collect_h_size*sizeof(uint32));
			p_collect++;
			collectp->collect_info.index++;
			n += collect_h_size;
			if (n >= n_total)
				break;
		}
#ifdef WL_RANGE_SEQ
		}
#endif

#if defined(TOF_COLLECT)
		memcpy((void *)&(p_collect - 1)->ri_rr, (void *)&collectp->ri_rr,
			FTM_TPK_RI_RR_LEN_SECURE_2_0);
#ifndef WL_PROXD_LOG_OPT
		prhex(NULL, (void *)&collectp->ri_rr,
				FTM_TPK_RI_RR_LEN_SECURE_2_0);
#endif /* WL_PROXD_LOG_OPT */
#endif /* TOF_COLLECT */

#ifndef WL_RANGE_SEQ
		uint32 chan_size = (K_TOF_COLLECT_CHAN_SIZE) >>
			(2 - collectp->collect_info.tof_frame_bw);
		memcpy((void *)&(p_collect - 1)->chan, (void *)&collectp->chan,
			(collectp->collect_info.num_max_cores + 1)*chan_size*sizeof(uint32));
#ifdef TOF_DBG
		prhex("TOF_COLLECT ri_rr",
			(uint8 *)&((p_collect - 1)->ri_rr[0]), FTM_TPK_RI_RR_LEN_SECURE_2_0);
#endif /* TOF_DBG */
#endif /* !WL_RANGE_SEQ */
		collectp->collect_cnt += collectp->collect_info.index;
		collectp->remote_collect = FALSE;
	}
}
#endif /* TOF_COLLECT */

#ifdef TOF_COLLECT_REMOTE
/* initiator gets collect debug data */
static int pdburst_initiator_get_collect_data(pdburst_collect_t * collectp,
	pdburst_collect_frm_t *tof_hdr)
{
	/* TOF_PRINTF(("index %d collect size %d\n", tof_hdr->index, collectp->collect_size)); */

	if (tof_hdr->index == 0 && collectp->collect_header) {
		bcopy(tof_hdr->data, collectp->collect_header, tof_hdr->length);
		collectp->collect_cnt = 0;
		collectp->remote_collect = TRUE;
	} else if (collectp->collect && (tof_hdr->index <= collectp->collect_size)) {
		bcopy(tof_hdr->data, collectp->collect + (tof_hdr->index - 1), tof_hdr->length);
		collectp->collect_cnt = tof_hdr->index;
		collectp->remote_collect = TRUE;
	}

	return (!collectp->collect || tof_hdr->index >= collectp->collect_size);
}
#endif /* TOF_COLLECT_REMOTE */

#if defined(TOF_COLLECT) || defined(TOF_COLLECT_REMOTE)
/* wl proxd_collect function */
int pdburst_collection(wlc_info_t *wlc, void *collectptr, wl_proxd_collect_query_t *query,
	void *buff, int len, uint16 *reqLen)
{
	pdburst_collect_t* collectp = collectptr;
	pdburst_t *burstp = NULL;
	pdburst_sm_t *sm = NULL;

	ASSERT(buff != NULL);

	/* collectp == NULL is IOVAR call */
	if (!pdburst_collect && query->request != PROXD_COLLECT_GET_STATUS &&
		query->request != PROXD_COLLECT_SET_STATUS) {
		return BCME_NOTREADY;
	}

	if (!collectp)
		collectp = pdburst_collect;

	if (collectp) {
		burstp = collectp->pdburstp;
		if (burstp)
			sm = burstp->sm;
	}

	switch (query->request) {
		case PROXD_COLLECT_GET_STATUS:
		case PROXD_COLLECT_SET_STATUS:
		{
			wl_proxd_collect_query_t *reply;

			*reqLen = sizeof(wl_proxd_collect_query_t);
			if (len < sizeof(wl_proxd_collect_query_t))
				return BCME_BUFTOOSHORT;

			reply = (wl_proxd_collect_query_t *)buff;
			bzero(reply, sizeof(wl_proxd_collect_query_t));

			if (query->request == PROXD_COLLECT_GET_STATUS) {
				if (pdburst_collect) {
					reply->status = 1;
					reply->remote = pdburst_collect->remote_collect;
					TOF_PRINTF(("status 1\n"));
				} else {
					reply->status = 0;
					reply->remote = FALSE;
				}
				if (!sm) {
					reply->mode = WL_PROXD_MODE_INITIATOR;
					reply->busy = FALSE;
				} else {
					reply->mode = sm->tof_mode;
					reply->busy = ((sm->tof_mode == WL_PROXD_MODE_TARGET) ?
						(sm->tof_state > TOF_STATE_IDLE) : TRUE) &&
						(sm->tof_state < TOF_STATE_ICONFIRM);
				}
			} else {
				if (query->status) {
					if (!pdburst_collect) {
						if (pdburst_collect_mem(wlc, &pdburst_collect,
							TRUE)) {
							FTM_ERR(("MALLOC failed %s\n",
								__FUNCTION__));
							return BCME_NOMEM;
						}
						/* store the bitmask */
						pdburst_collect->collect_method = query->status;
					}
				} else {
					if (pdburst_collect) {
						pdburst_collect_mem(wlc, &pdburst_collect, FALSE);
					}
				}
			}
			break;
		}

		case PROXD_COLLECT_QUERY_HEADER:
		{
			wl_proxd_collect_header_t *reply;

			*reqLen = sizeof(wl_proxd_collect_header_t);
			if (len < sizeof(wl_proxd_collect_header_t))
				return BCME_BUFTOOSHORT;

			reply = (wl_proxd_collect_header_t *)buff;

			if (!burstp || !collectp) {
				bzero(reply, sizeof(wl_proxd_collect_header_t));
				return BCME_OK;
			}

			if (collectp->remote_collect && collectp->collect_header) {
				bcopy(collectp->collect_header, reply,
					sizeof(wl_proxd_collect_header_t));
			} else {
				bzero(reply, sizeof(wl_proxd_collect_header_t));
				pdburst_collect_prep_header(collectp, reply);
			}
			break;
		}

		case PROXD_COLLECT_QUERY_DATA:
		{
			wl_proxd_collect_data_t *reply;
			wl_proxd_collect_data_t *collect;
			int size;
			int collect_h_size;

			if (!burstp) {
				ASSERT(0);
				return BCME_ERROR;
			}

			if (CHSPEC_IS20(burstp->configp->chanspec)) {
				collect_h_size = K_TOF_COLLECT_H_SIZE_20MHZ;
			} else {
				collect_h_size = K_TOF_COLLECT_H_SIZE_80MHZ;
			}

			if (!collectp->collect)
				return BCME_ERROR;

			if (query->index >= (uint16)collectp->collect_cnt ||
				query->index >= (uint16)collectp->collect_size)
				return BCME_RANGE;

			collect = collectp->collect + query->index;
			size = sizeof(wl_proxd_collect_data_t) -
				(collect_h_size - collect->info.nfft)* sizeof(uint32);

			*reqLen = (uint16)size;
			if (len < size)
				return BCME_BUFTOOSHORT;

			reply = (wl_proxd_collect_data_t *)buff;
			bcopy(collect, reply, size);
			break;
		}

		case PROXD_COLLECT_QUERY_DEBUG:
			return BCME_ERROR;

#ifdef TOF_COLLECT_REMOTE
		case PROXD_COLLECT_REMOTE_REQUEST:
		{
			if (!burstp)
				return BCME_NOTREADY;
			if (burstp->sm->tof_mode == WL_PROXD_MODE_INITIATOR) {
				if (burstp->sm->tof_state != TOF_STATE_IDLE &&
					burstp->sm->tof_state != TOF_STATE_ICONFIRM) {
					return BCME_BUSY;
				}

				if (!collectp->collect) {
					return BCME_ERROR;
				}

				collectp->collect_cnt = 0;
				bzero(collectp->collect, (collectp->collect_size) *
					sizeof(wl_proxd_collect_data_t));

				collectp->remote_request = TRUE;
				pdburst_start(burstp);
			} else
				return  BCME_UNSUPPORTED;
			break;
		}
#endif /* TOF_COLLECT_REMOTE */

		case PROXD_COLLECT_DONE:
			if (burstp) {
				burstp->collectp = NULL;
				pdburst_destroy(&burstp);
				collectp->pdburstp = NULL;
			}
#ifndef TOF_COLLECT_REMOTE
			collectp->collect_cnt = 0;
			bzero(collectp->collect, (collectp->collect_size) *
				sizeof(wl_proxd_collect_data_t));
#endif /* !TOF_COLLECT_REMOTE */
			break;

		default:
			return BCME_UNSUPPORTED;
	}

	return BCME_OK;
}
#endif /* TOF_COLLECT || TOF_COLLECT_REMOTE */

#ifdef TOF_COLLECT_REMOTE
static int pdburst_sm_initiator_collect_wait(pdburst_sm_t *sm, int event, pdburst_data_t *datap,
	pdburst_tsinfo_t *protp)
{
	pdburst_t* burstp = sm->tof_obj;
	wlc_info_t *wlc = burstp->wlc;
	int ret = TOF_RET_ALIVE;

	switch (event) {
		case TOF_EVENT_RXACT:
			/* Initiator Receive FTMs */
			if (datap->tof_type == TOF_TYPE_COLLECT_DATA ||
				datap->tof_type == TOF_TYPE_COLLECT_DATA_END) {
				bool endRx = FALSE;

				if (burstp->timeractive) {
					wl_del_timer(wlc->wl, burstp->timer);
					if (datap->tof_type == TOF_TYPE_COLLECT_DATA)
						wl_add_timer(wlc->wl, burstp->timer,
							FTM_INTVL2MSEC(&burstp->configp->timeout),
							FALSE);
					else
						burstp->timeractive = FALSE;
				}
				++sm->tof_rxcnt;
				endRx = pdburst_initiator_get_collect_data(burstp->collectp,
					(pdburst_collect_frm_t *)protp);
				if (datap->tof_type == TOF_TYPE_COLLECT_DATA) {
					if (endRx) {
						pdburst_collect_send(sm, &datap->tof_srcea,
							TOF_TYPE_COLLECT_REQ_END);
						pdburst_confirmed(sm, WL_PROXD_E_OK);
						ret = TOF_RET_SLEEP;
					}
				} else {
					pdburst_confirmed(sm, WL_PROXD_E_OK);
					ret = TOF_RET_SLEEP;
				}
			} else {
				FTM_ERR(("Initiator(%d) got unexpected type %d\n",
					sm->tof_state, datap->tof_type));
				ret = TOF_RET_IGNORE;
			}
			break;

		case TOF_EVENT_NOACK:
			/* REQ is NOT acked */
			++sm->tof_retrycnt;
			if (sm->tof_retrycnt > burstp->configp->ftm_req_retries) {
				pdburst_confirmed(sm, WL_PROXD_E_NOACK);
				ret = TOF_RET_SLEEP;
			} else {
				pdburst_collect_send(sm, &sm->tof_peerea,
					TOF_TYPE_COLLECT_REQ_START);
			}
			break;

		case TOF_EVENT_ACKED:
			sm->tof_retrycnt = 0;
			if (FTM_INTVL2MSEC(&burstp->configp->timeout)) {
				wl_add_timer(wlc->wl, burstp->timer,
					FTM_INTVL2MSEC(&burstp->configp->timeout), FALSE);
				burstp->timeractive = TRUE;
			}
			break;

		case TOF_EVENT_WAITMTMO:
			pdburst_confirmed(sm, WL_PROXD_E_TIMEOUT);
			ret = TOF_RET_SLEEP;
			break;

		default:
			ret = TOF_RET_IGNORE;
			break;
	}
	return ret;
}

static int pdburst_sm_target_collect_wait(pdburst_sm_t *sm, int event)
{
	pdburst_t* burstp = sm->tof_obj;
	int ret = TOF_RET_ALIVE;

	if (event == TOF_EVENT_ACKED || event == TOF_EVENT_NOACK) {
		if (event == TOF_EVENT_ACKED) {
			sm->tof_retrycnt = 0;
			sm->tof_txcnt++;
			sm->tof_followup = sm->tof_dialog;
			burstp->collectp->remote_cnt++;
		} else {
			sm->tof_retrycnt++;
		}

		if (sm->tof_retrycnt > burstp->configp->ftm_retries ||
			burstp->collectp->remote_cnt > burstp->collectp->collect_cnt) {
			/* Remote collect is done, free buffer */
			burstp->collectp = NULL;
			pdburst_destroy(&burstp);
			ret = TOF_RET_SLEEP;
		} else {
			pdburst_collect_send(sm, &sm->tof_peerea,
				(burstp->collectp->collect && burstp->collectp->remote_cnt <
					burstp->collectp->collect_cnt)?
					TOF_TYPE_COLLECT_DATA : TOF_TYPE_COLLECT_DATA_END);
			ret = TOF_RET_ALIVE;
		}
	}
	return ret;
}
#endif /* TOF_COLLECT_REMOTE */

static bool pdburst_initiator_measure_frame(pdburst_t* burstp, uint8 type)
{
	if (type == TOF_TYPE_MEASURE)
		return TRUE;
	if (burstp->lastburst)
		return FALSE;

	if (burstp->flags & WL_PROXD_SESSION_FLAG_MBURST_FOLLOWUP)
		return TRUE;
	return FALSE;
}

static void pdburst_initiator_save_last_frame(pdburst_t* burstp, uint8 type, uint64 t2,
	uint64 t3, int32 gd, int32 adj, int8 rssi, ratespec_t rspec, bool discard, uint8 txid)
{
	if (!(burstp->flags & WL_PROXD_SESSION_FLAG_MBURST_FOLLOWUP))
		return;

	if (burstp->lasttsp && !burstp->lastburst) {
		/* Save last frame measurement t2/t3 for next burst */
		burstp->tx_t1 = t2;
		burstp->lasttsp->t1 = 0;
		burstp->lasttsp->t4 = 0;
		burstp->lasttsp->t3 = t3;
		burstp->lasttsp->t2 = t2;
		burstp->lasttsp->gd = gd;
		burstp->lasttsp->adj = adj;
		burstp->lasttsp->rssi = rssi;
		burstp->lasttsp->rspec = rspec;
		burstp->lasttsp->discard = discard;
		burstp->lasttsp->tx_id = txid;
#ifdef TOF_DEBUG_TIME
		TOF_PRINTF(("Save txid %d t2 %d\n", txid, burstp->lasttsp->t2));
#endif
	}
}

static void pdburst_initiator_restore_last_frame(pdburst_t* burstp, ftmts_t *tsp)
{
	if (burstp->flags & WL_PROXD_SESSION_FLAG_MBURST_FOLLOWUP) {
		if (burstp->lasttsp && tsp) {
			memcpy(tsp, burstp->lasttsp, sizeof(ftmts_t));
#ifdef TOF_DEBUG_TIME
			TOF_PRINTF(("Restore timestamp txid %d\n", tsp->tx_id));
#endif /* TOF_DEBUG_TIME */
		}
	}
}

/* initiator gets AVB time stamp */
static int pdburst_initiator_get_ts(pdburst_sm_t *sm, const pdburst_tsinfo_t *protp,
	int rssi, uint32 rspec, uint8 type)
{
	pdburst_t* burstp = sm->tof_obj;
	tof_tslist_t *listp = &burstp->tof_tslist;
	ftmts_t * list = listp->tslist;
	uint64 t3, t2;
	int32 gd, adj;
	uint32 k;
	bool discard = FALSE;
	uint8 measurecnt;
	wlc_phy_tof_info_t tof_info;
	wlc_phy_tof_info_type_t temp_mask;
	chanspec_t ftm_chanspec;
	uint32 avbrx = 0;
	uint32 avbtx = 0;

	bzero(&tof_info, sizeof(tof_info));

	burstp->ftm_rx_rspec = rspec;

	 /* The responding STA *should* transmit Fine Timing Measurement frames with the
	  * format and bandwidth it indicated. (but might not)
	  */
	ftm_chanspec = pdburst_get_chspec_from_rspec(burstp, rspec);
	proxd_update_tunep_values(burstp->tunep, ftm_chanspec, BURST_IS_VHTACK(burstp));

	pdburst_phy_rxclass_war(burstp, ftm_chanspec);

	measurecnt = pdburst_rx_measurecnt(burstp, protp);
#ifdef TOF_DEBUG_TIME
	TOF_PRINTF(("%s burstp->seq_started %d seq_en %d num_meas %d measurecnt %d  "
	"type %d ts_id %d tx_id %d seq_state %d\n",
	__FUNCTION__, burstp->seq_started, burstp->seq_en,
	 burstp->num_meas, measurecnt, type, protp->ts_id, protp->tx_id, burstp->seq_state));
#endif
	if (pdburst_initiator_measure_frame(burstp, type)) {
		if (pdburst_measure_results(burstp, &t3, &t2, &k, &gd, &adj,
			(uint16)protp->ts_id, TRUE, &tof_info, &discard, TRUE,
			&avbrx, &avbtx)) {
			t2 = 0;
			t3 = 0;
			k = 0;
			gd = 0;
			adj = 0;
			discard = TRUE;
		}
	}

	if (BURST_SEQ_EN(burstp)) {
		if (burstp->seq_state == TOF_SEQ_DONE) {
			burstp->seq_started = FALSE;
			WL_ERROR(("seq_started is set to %d\n", burstp->seq_started));
			proxd_undeaf_phy(burstp->wlc, TRUE);
			pdburst_measure(burstp, TOF_RX);
		}

		temp_mask =  (WLC_PHY_TOF_INFO_TYPE_SNR | WLC_PHY_TOF_INFO_TYPE_BITFLIPS);
		wlc_phy_tof_info(WLC_PI(burstp->wlc), &tof_info, temp_mask, burstp->core);

		if (!(burstp->num_meas % TOF_DEFAULT_FTMCNT_SEQ)) {
			burstp->seq_started = FALSE;
			TOF_PRINTF(("seq_started is set to %d\n", burstp->seq_started));
			proxd_undeaf_phy(burstp->wlc, TRUE);
			pdburst_measure(burstp, TOF_RX);
		}
	}

	if (measurecnt <= burstp->measurecnt && list) {
		/* Get t2, t3 */
		if (measurecnt < burstp->measurecnt && type == TOF_TYPE_MEASURE) {
#ifdef TOF_COLLECT
			pdburst_collect_data(burstp->collectp, protp->ts_id,
					CHSPEC_IS20(burstp->configp->chanspec));
#endif /* TOF_COLLECT */
#ifdef WL_PROXD_PHYTS
			if (burstp->sampcap_method && !(burstp->fallback_avb)) {
				/* Save the information of timestamping method */
				list[measurecnt].tsmethod = TS_METHOD_SAMPCAP;
				list[measurecnt].gd = 0u;
				list[measurecnt].adj = 0u;
			} else
#endif /* WL_PROXD_PHYTS */
			{
				if (BURST_SEQ_EN(burstp)) {
					list[measurecnt].tsmethod = TS_METHOD_SEQ;
				} else {
					list[measurecnt].tsmethod = TS_METHOD_AVB;
				}
				list[measurecnt].gd = gd;
				list[measurecnt].adj = adj;
			}
			list[measurecnt].t3 = t3;
			list[measurecnt].t2 = t2;
			list[measurecnt].k = k;
			list[measurecnt].rssi = rssi;
			list[measurecnt].rspec = rspec;
			list[measurecnt].discard = discard;
			list[measurecnt].avbrx = avbrx;
			list[measurecnt].avbtx = avbtx;
			list[measurecnt].tx_id = protp->tx_id;
			if (tof_info.info_mask & WLC_PHY_TOF_INFO_TYPE_RSSI) {
				/* list[measurecnt].rssi = tof_info.rssi; */
			}
			if (tof_info.info_mask & WLC_PHY_TOF_INFO_TYPE_SNR) {
				list[measurecnt].snr = tof_info.snr;
			}
			if (tof_info.info_mask & WLC_PHY_TOF_INFO_TYPE_BITFLIPS) {
				list[measurecnt].bitflips = tof_info.bitflips;
			}

			if (tof_info.info_mask & WLC_PHY_TOF_INFO_TYPE_PHYERROR) {
				list[measurecnt].tof_phy_error = tof_info.tof_phy_error;
			}
		} else {
			/* last measurement */
			pdburst_initiator_save_last_frame(burstp, type, t2, t3, gd, adj, rssi,
				rspec, discard, protp->tx_id);
		}
		if (measurecnt) {
#ifdef WL_PROXD_PHYTS
			if (list[measurecnt - 1u].tsmethod == TS_METHOD_SAMPCAP) {
				/* Use ps resolution */
				list[measurecnt - 1u].t1 = protp->tod;
				list[measurecnt - 1u].t4 = protp->toa;
			} else
#endif /* WL_PROXD_PHYTS */
			{
			/* convert to nano sec - resolution here. 0.5 ns error */
			list[measurecnt-1].t1 = (uint32)pdftm_div64(protp->tod, BURST_HZ_PICO);
			list[measurecnt-1].t4 = (uint32)pdftm_div64((protp->toa +
				BURST_ERR_PICO), BURST_HZ_PICO);
			}
			if (measurecnt > listp->tscnt)
				listp->tscnt = measurecnt;
		}
	}
	return BCME_OK;
}

static bool pdburst_target_timestamp_last_frame(pdburst_t * burstp, uint8 totalfcnt)
{
	bool ret = TRUE;
	if (burstp->sm->tof_txcnt >= burstp->measurecnt ||
		(totalfcnt && burstp->sm->tof_txpktcnt >= totalfcnt)) {
		/* last measurement frame */
		if (!(burstp->flags & WL_PROXD_SESSION_FLAG_MBURST_FOLLOWUP) || burstp->lastburst) {
			burstp->sm->tof_retrycnt = 0;
			burstp->sm->tof_txcnt++;
			burstp->sm->tof_followup = burstp->sm->tof_dialog;
			ret = FALSE;
		}
	}
	return ret;
}

/* target gets time stamp */
static int pdburst_target_get_ts(pdburst_sm_t *sm, bool acked)
{
	pdburst_t* burstp = sm->tof_obj;
	uint64 t1, t4;
	int32 gd, adj;
	uint32 k;
	bool discard = FALSE;
	uint16 id;
	wlc_phy_tof_info_t tof_info;
	wlc_phy_tof_info_type_t tof_info_mask;
	uint32 avbrx = 0;
	uint32 avbtx = 0;

	bzero(&tof_info, sizeof(tof_info));

	if (sm->tof_legacypeer == TOF_LEGACY_AP) {
		tof_tslist_t *listp = &burstp->tof_tslist;
		id = listp->tscnt;
	} else {
		id = sm->tof_txcnt;
	}

	if (pdburst_measure_results(burstp, &t1, &t4, &k, &gd, &adj, id, acked,
		&tof_info, &discard, FALSE, &avbrx, &avbtx)) {
		t1 = 0;
		t4 = 0;
		k = 0;
		gd = 0;
		adj = 0;
		discard = TRUE;
	}

	if (BURST_SEQ_EN(burstp)) {
		tof_info_mask =  (WLC_PHY_TOF_INFO_TYPE_SNR | WLC_PHY_TOF_INFO_TYPE_BITFLIPS
						| WLC_PHY_TOF_INFO_TYPE_RSSI);
		wlc_phy_tof_info(WLC_PI(burstp->wlc), &tof_info, tof_info_mask, burstp->core);
	}

#ifdef TOF_COLLECT
	if (sm->tof_legacypeer != TOF_LEGACY_AP) {
		pdburst_collect_data(burstp->collectp, sm->tof_followup,
				CHSPEC_IS20(burstp->configp->chanspec));
	}
#endif /* TOF_COLLECT */

	if (!acked || discard)
		return BCME_ERROR;

	if (sm->tof_legacypeer == TOF_LEGACY_AP) {
		tof_tslist_t *listp = &burstp->tof_tslist;
		ftmts_t * list = listp->tslist;

#ifdef TOF_COLLECT
		pdburst_collect_data(burstp->collectp, burstp->collectp->collect_cnt,
				CHSPEC_IS20(burstp->configp->chanspec));
#endif /* TOF_COLLECT */
		if (list && listp->tscnt < burstp->measurecnt) {
#ifdef WL_PROXD_PHYTS
			if (burstp->sampcap_method && !(burstp->fallback_avb)) {
				list[listp->tscnt].tsmethod = TS_METHOD_SAMPCAP;
				list[listp->tscnt].gd = 0u;
				list[listp->tscnt].adj = 0u;
			} else
#endif /* WL_PROXD_PHYTS */
			{
				if (BURST_SEQ_EN(burstp)) {
					list[listp->tscnt].tsmethod = TS_METHOD_SEQ;
				} else {
					list[listp->tscnt].tsmethod = TS_METHOD_AVB;
				}
				list[listp->tscnt].gd = gd;
				list[listp->tscnt].adj = adj;
			}
			list[listp->tscnt].t1 = t1;
			list[listp->tscnt].t4 = t4;
			list[listp->tscnt].k = k;
			list[listp->tscnt].avbrx = avbrx;
			list[listp->tscnt].avbtx = avbtx;
			if (tof_info.info_mask & WLC_PHY_TOF_INFO_TYPE_RSSI)
				list[listp->tscnt].rssi = tof_info.rssi;
			if (tof_info.info_mask & WLC_PHY_TOF_INFO_TYPE_SNR)
				list[listp->tscnt].snr = tof_info.snr;
			if (tof_info.info_mask & WLC_PHY_TOF_INFO_TYPE_BITFLIPS)
				list[listp->tscnt].bitflips = tof_info.bitflips;
			list[listp->tscnt].rspec = burstp->ack_rx_rspec;
			list[listp->tscnt].discard = discard;
			list[listp->tscnt].tx_id = sm->tof_dialog;
			listp->tscnt++;
		}
		burstp->tx_t1 = t1;
		burstp->tx_t4 = t4;
	} else {
#ifdef WL_PROXD_PHYTS
		if (burstp->sampcap_method && !(burstp->fallback_avb)) {
			burstp->tx_t1 = t1;
			burstp->tx_t4 = t4;
		} else
#endif /* WL_PROXD_PHYTS */
		{
			/* convert into PICO second */
			burstp->tx_t1 = t1 * BURST_HZ_PICO;
			burstp->tx_t4 = t4 * BURST_HZ_PICO;
		}
	}
	return BCME_OK;
}

/* TOF timeout function */
static void pdburst_timer(void *arg)
{
	pdburst_t* burstp = (pdburst_t *)arg;

	burstp->timeractive = FALSE;
	if (burstp->smstoped)
		return;

	if (burstp->sm->tof_mode == WL_PROXD_MODE_INITIATOR)
		pdburst_sm(burstp->sm, TOF_EVENT_WAITMTMO, NULL, 0, NULL);
}

/* delay certain time before txing the next measurement packet */
static void pdburst_ftm_tx_timer(void *ctx)
{
	pdburst_t *burstp = (pdburst_t *)ctx;
	pdburst_sm_t *sm;
	uint8 type = TOF_TYPE_MEASURE;
	int totalfcnt;
	int err = BCME_OK;

	ASSERT(burstp != NULL);
	sm = burstp->sm;

	if (sm->tof_mode == WL_PROXD_MODE_INITIATOR)
	{
		burstp->ftm_tx_timer_active = FALSE;
		if (burstp->flags & WL_PROXD_SESSION_FLAG_ONE_WAY) {
			err = pdburst_send(sm, &sm->tof_peerea, TOF_TYPE_MEASURE);
		} else {
			err = pdburst_send(sm, &sm->tof_peerea, TOF_TYPE_REQ_START);
		}
		if (err == BCME_OK) {
			pdburst_measure(burstp, TOF_RX);
		} else if (err != BCME_TXFAIL) {
			/* stop burst for errors other than txfail
			* In txfail, retries will be attempted in pcb context
			*/
			FTM_ERR(("pdburst_ftm_tx_timer: pdburst_send err %d\n", err));
			pdburst_confirmed(sm, WL_PROXD_E_ERROR);
		}
		return;
	}
	if (!burstp->caldone && !burstp->smstoped) {
		/* Wait for calculation done */
		wlc_hrt_add_timeout(burstp->ftm_tx_timer, 100, pdburst_ftm_tx_timer, ctx);
		return;
	}
	wlc_hrt_del_timeout(burstp->ftm_tx_timer);
	burstp->ftm_tx_timer_active = FALSE;
	sm->tof_txpktcnt++;
	if (!(++sm->tof_dialog))
		sm->tof_dialog = 1;
	totalfcnt = pdburst_total_framecnt(burstp);
	if (sm->tof_txcnt >= burstp->measurecnt ||
		(totalfcnt && sm->tof_txpktcnt >= totalfcnt)) {
		type = TOF_TYPE_MEASURE_END;

		/* Last burst measurement dialog = 0 */
		if (burstp->lastburst) {
			sm->tof_dialog = 0;
		}
	}

	if (pdburst_send(sm, &burstp->sm->tof_peerea, type) != BCME_OK) {
		FTM_ERR(("%s: ERROR in pdburst_send\n", __FUNCTION__));
	}
}

/* Use one way RTT */
static void pdburst_start_oneway(pdburst_t *burstp, pdburst_sm_t *sm)
{
	sm->tof_state = TOF_STATE_ILEGACY;
	sm->tof_legacypeer = TOF_LEGACY_AP;
	sm->tof_txcnt = 0;
	sm->tof_txpktcnt = 1;
	++sm->tof_dialog;
	if (!sm->tof_dialog)
		sm->tof_dialog = 1;
	pdburst_send(sm, &sm->tof_peerea, TOF_TYPE_MEASURE);
}

static void pdburst_decide_oneway(pdburst_t *burstp, pdburst_sm_t *sm)
{
	/* Disable one-way RTT when it is set as auto-mode */
	sm->tof_legacypeer = TOF_NONLEGACY_AP;
}
static int pdburst_sm_legacy(pdburst_sm_t *sm, int event)
{
	pdburst_t* burstp = sm->tof_obj;
	int ret = TOF_RET_SLEEP;
	uint8 totalfcnt;

	if (event == TOF_EVENT_ACKED || event == TOF_EVENT_NOACK) {
		if (event == TOF_EVENT_ACKED) {
			if (pdburst_target_get_ts(sm, TRUE) == BCME_OK) {
				sm->tof_retrycnt = 0;
				sm->tof_txcnt++;
				sm->tof_followup = sm->tof_dialog;
			} else
				sm->tof_retrycnt++;
		} else {
			sm->tof_retrycnt++;
			pdburst_target_get_ts(sm, FALSE);
		}

		totalfcnt = pdburst_total_framecnt(burstp);
		if (sm->tof_retrycnt > burstp->configp->ftm_retries) {
			pdburst_confirmed(sm, WL_PROXD_E_NOACK);
		} else if (sm->tof_txcnt >= burstp->measurecnt) {
			pdburst_confirmed(sm, WL_PROXD_E_OK);
		} else if (sm->tof_txpktcnt >= totalfcnt && totalfcnt) {
			pdburst_confirmed(sm, WL_PROXD_E_CANCELED);
		} else {
			sm->tof_txpktcnt++;
			sm->tof_dialog++;
			if (!sm->tof_dialog)
				sm->tof_dialog = 1;
			pdburst_send(sm, &sm->tof_peerea, TOF_TYPE_MEASURE);
			ret = TOF_RET_ALIVE;
		}
	}
	return ret;
}

static int pdburst_sm_target_wait(pdburst_sm_t *sm, int event, pdburst_data_t *datap)
{
	pdburst_t* burstp = sm->tof_obj;
	int ret = TOF_RET_ALIVE;
	uint8 totalfcnt;

	switch (event) {
		case TOF_EVENT_ACKED:
		case TOF_EVENT_NOACK:
			if (sm->tof_txcnt <= burstp->measurecnt) {
				if (burstp->ftm_tx_timer_active) {
					wlc_hrt_del_timeout(burstp->ftm_tx_timer);
					burstp->ftm_tx_timer_active = FALSE;
				}
				wlc_hrt_add_timeout(burstp->ftm_tx_timer,
						FTM_INTVL2USEC(&burstp->configp->ftm_sep),
						pdburst_ftm_tx_timer, (void *)burstp);
				burstp->ftm_tx_timer_active = TRUE;
				burstp->caldone = FALSE;
			}
			totalfcnt = pdburst_total_framecnt(burstp);
			if (event == TOF_EVENT_ACKED) {
#if defined(TOF_PROFILE)
				wlc_read_tsf(burstp->wlc, &tsf_lastm, &tsf_hi);
				tsf_lastm -= tsf_start;
				TOF_PRINTF(("TIME_BEFORE_GET_TS = 0x%0x\n", tsf_lastm));
#endif
				if (pdburst_target_timestamp_last_frame(burstp, totalfcnt) &&
					pdburst_target_get_ts(sm, TRUE) == BCME_OK) {
					sm->tof_retrycnt = 0;
					sm->tof_txcnt++;
					sm->tof_followup = sm->tof_dialog;
				} else {
					sm->tof_retrycnt++;
				}
#if defined(TOF_PROFILE)
				wlc_read_tsf(burstp->wlc, &tsf_lastm, &tsf_hi);
				tsf_lastm -= tsf_start;
				TOF_PRINTF(("TIME_AFTER_GET_TS = 0x%0x\n", tsf_lastm));
#endif
			} else {
				sm->tof_retrycnt++;
				OSL_DELAY(1);
				pdburst_target_get_ts(sm, FALSE);
			}
			burstp->caldone = TRUE;
			if (sm->tof_retrycnt > burstp->configp->ftm_retries) {
				FTM_ERR(("Too many retries, ftm:%d, stopped\n", sm->tof_txcnt));
				sm->tof_retrycnt = 0;
				pdburst_reset(sm, sm->tof_mode, WL_PROXD_E_NOACK);
				ret = pdburst_target_done(sm, WL_PROXD_E_NOACK);
			} else if (sm->tof_txcnt > burstp->measurecnt) {
				if (event == TOF_EVENT_ACKED) {
					/*  TARGET ftms completed OK */
					sm->tof_retrycnt = 0;
					pdburst_reset(sm, sm->tof_mode, WL_PROXD_E_OK);
					ret = pdburst_target_done(sm, WL_PROXD_E_OK);
				}
			} else if (sm->tof_txpktcnt >= totalfcnt && totalfcnt) {
				sm->tof_retrycnt = 0;
				pdburst_reset(sm, sm->tof_mode, WL_PROXD_E_CANCELED);
				ret = pdburst_target_done(sm, WL_PROXD_E_CANCELED);
			}
			break;

		case TOF_EVENT_RXACT:
			/* target received FTMR */
			if (datap->tof_type == TOF_TYPE_REQ_START) {
				/* Rxed start because client resets */
				sm->tof_txcnt = 0;
				sm->tof_retrycnt = 0;
				pdburst_send(sm, &sm->tof_peerea, TOF_TYPE_MEASURE);
			} else {
				FTM_ERR(("Unknown TOF pkt type:%d\n", datap->tof_type));
				ret = TOF_RET_IGNORE;
			}
			break;

		default:
			ret = TOF_RET_IGNORE;
			break;
	}
	return ret;
}

static int pdburst_sm_initiator_wait(pdburst_sm_t *sm, int event, pdburst_data_t *datap,
	const pdburst_tsinfo_t *protp)
{
	pdburst_t* burstp = sm->tof_obj;
	wlc_info_t *wlc = burstp->wlc;
	int ret = TOF_RET_ALIVE;

	switch (event) {
		case TOF_EVENT_RXACT:
			/* Initiator Rxed Packet */
			if (datap->tof_type == TOF_TYPE_MEASURE ||
				datap->tof_type == TOF_TYPE_MEASURE_END) {
				/* Rxed measure packet */
				if (sm->tof_legacypeer == TOF_LEGACY_UNKNOWN) {
					/* First Measurement Packet */
#ifdef TOF_DEBUG_TIME2
					wlc_read_tsf(burstp->wlc, &tsf_rxm, &tsf_hi);
					tsf_rxm -= tsf_start;
#endif
					sm->tof_legacypeer = TOF_NONLEGACY_AP;
				}
#ifdef TOF_DEBUG_TIME2
				wlc_read_tsf(burstp->wlc, &tsf_lastm, &tsf_hi);
				tsf_lastm -= tsf_start;
#endif
				if (burstp->timeractive) {
					wl_del_timer(wlc->wl, burstp->timer);
					if (datap->tof_type == TOF_TYPE_MEASURE)
						wl_add_timer(wlc->wl, burstp->timer,
							FTM_INTVL2MSEC(&burstp->configp->timeout),
							FALSE);
					else
						burstp->timeractive = FALSE;
				}
				++sm->tof_rxcnt;
				pdburst_initiator_get_ts(sm, protp, datap->tof_rssi,
					datap->tof_rspec, datap->tof_type);
				if (datap->tof_type == TOF_TYPE_MEASURE) {
					if (pdburst_rx_measurecnt(burstp, protp) >=
						burstp->measurecnt) {
						ret = pdburst_confirmed(sm, WL_PROXD_E_OK);
					}
				} else {
					ret = pdburst_confirmed(sm, WL_PROXD_E_OK);
				}
			} else {
				FTM_ERR(("Initiator(%d) got unexpected type %d\n",
					sm->tof_state, datap->tof_type));
				ret = TOF_RET_IGNORE;
			}
			break;

		case TOF_EVENT_NOACK:
			/* REQ is NOT acked */
			++sm->tof_retrycnt;
			if (sm->tof_retrycnt > burstp->configp->ftm_req_retries) {
				pdburst_confirmed(sm, WL_PROXD_E_NOACK);
				ret = TOF_RET_SLEEP;
			} else {
				wlc_hrt_add_timeout(burstp->ftm_tx_timer,
					TOF_REQ_START_RETRY_DUR, pdburst_ftm_tx_timer,
					(void *)burstp);
				burstp->ftm_tx_timer_active = TRUE;
				ret = TOF_RET_SLEEP;
			}
			break;

		case TOF_EVENT_ACKED:
			/* initiator rxed FTMR ack */
			burstp->result_flags |= WL_PROXD_REQUEST_ACKED;
			sm->tof_retrycnt = 0;
			if (burstp->flags & WL_PROXD_SESSION_FLAG_ONE_WAY) {
				pdburst_start_oneway(burstp, sm);
			} else if (FTM_INTVL2MSEC(&burstp->configp->timeout)) {
				wl_add_timer(wlc->wl, burstp->timer,
					FTM_INTVL2MSEC(&burstp->configp->timeout), FALSE);
				burstp->timeractive = TRUE;
				pdburst_decide_oneway(burstp, sm);
			}
			if (burstp->ftm_tx_timer_active) {
				wlc_hrt_del_timeout(burstp->ftm_tx_timer);
				burstp->ftm_tx_timer_active = FALSE;
			}
#ifdef TOF_DEBUG_TIME2
			wlc_read_tsf(burstp->wlc, &tsf_rxack, &tsf_hi);
			tsf_rxack -= tsf_start;
#endif
			break;

		case TOF_EVENT_WAITMTMO:
			/* Wait for measurement pkt timeout */
			if (sm->tof_legacypeer == TOF_LEGACY_UNKNOWN) {
				pdburst_start_oneway(burstp, sm);
			} else if (sm->tof_legacypeer == TOF_NONLEGACY_AP) {
				/* AP stoped txing measurement */
				ret = pdburst_confirmed(sm, WL_PROXD_E_TIMEOUT);
			}
			break;

		default:
			ret = TOF_RET_IGNORE;
			break;
	}
	return ret;
}

static int pdburst_sm_idle(pdburst_sm_t *sm, int event, pdburst_data_t *datap)
{
	pdburst_t* burstp = sm->tof_obj;
	int ret = TOF_RET_ALIVE;

	switch (event) {
		case TOF_EVENT_WAKEUP:
			if (sm->tof_mode == WL_PROXD_MODE_INITIATOR) {
#ifdef TOF_DEBUG_TIME2
				wlc_read_tsf(burstp->wlc, &tsf_txreq, &tsf_hi);
				tsf_txreq -= tsf_start;
#endif
				if (burstp->flags & WL_PROXD_SESSION_FLAG_SEQ_EN) {
					burstp->seq_en = TRUE;
				} else {
					burstp->seq_en = FALSE;
				}
				burstp->measurecnt = pdburst_get_ftm_cnt(burstp);
				pdburst_hw(burstp, TRUE, FALSE);

				if (BURST_SEQ_EN(burstp)) {
					FTM_ERR(("Invalid config SEQ mode %d\n", sm->tof_mode));
					ASSERT(0);
				}

				/* Only needed on side which sends acks */
				/* Uses the TDC scheme for ACK */
				pdburst_tof_ack_init(burstp);

				wlc_hrt_add_timeout(burstp->ftm_tx_timer, TOF_REQ_START_DUR,
					pdburst_ftm_tx_timer, (void *)burstp);
				burstp->ftm_tx_timer_active = TRUE;
				sm->tof_state = TOF_STATE_IWAITM;
			} else if (sm->tof_mode != WL_PROXD_MODE_TARGET) {
				FTM_ERR(("Invalid mode %d\n", sm->tof_mode));
				ret = TOF_RET_SLEEP;
			}
			break;
#ifdef TOF_COLLECT_REMOTE
		case TOF_EVENT_COLLECT_REQ:
			if (sm->tof_mode == WL_PROXD_MODE_INITIATOR) {
				pdburst_event(sm, WLC_E_PROXD_COLLECT_START);
				pdburst_collect_send(sm, &sm->tof_peerea,
					TOF_TYPE_COLLECT_REQ_START);
				sm->tof_state = TOF_STATE_IWAITCL;
				sm->tof_retrycnt = 0;
				ret = TOF_RET_ALIVE;
			} else {
				ret = TOF_RET_SLEEP;
				FTM_ERR(("Invalid mode %d\n", sm->tof_mode));
			}
			break;
#endif /* TOF_COLLECT_REMOTE */
		case TOF_EVENT_RXACT:
			if (datap->tof_type == TOF_TYPE_REQ_START) {
				/* Rxed measure request packet */
				if (sm->tof_mode == WL_PROXD_MODE_TARGET) {
					pdburst_hw(burstp, TRUE, TRUE);
					bcopy(&datap->tof_srcea, &sm->tof_peerea, ETHER_ADDR_LEN);
					sm->tof_state = TOF_STATE_TWAITM;
					sm->tof_txpktcnt = 1;
					sm->tof_dialog++;
					if (!sm->tof_dialog)
						sm->tof_dialog = 1;
					pdburst_send(sm, &sm->tof_peerea, TOF_TYPE_MEASURE);
				} else {
					ret = TOF_RET_IGNORE;
				}
			}
#ifdef TOF_COLLECT_REMOTE
			else if (datap->tof_type == TOF_TYPE_COLLECT_REQ_START) {
				/* Rxed collect request packet */
				if (sm->tof_mode == WL_PROXD_MODE_TARGET && burstp->collectp) {
					bcopy(&datap->tof_srcea, &sm->tof_peerea, ETHER_ADDR_LEN);
					bcopy(&datap->tof_dstea, &sm->tof_selfea, ETHER_ADDR_LEN);
					sm->tof_state = TOF_STATE_TWAITCL;
					sm->tof_retrycnt = 0;
					burstp->collectp->remote_cnt = 0;
					pdburst_collect_send(sm, &sm->tof_peerea,
						TOF_TYPE_COLLECT_DATA);
					pdburst_event(sm, WLC_E_PROXD_COLLECT_START);
				} else {
					ret = TOF_RET_IGNORE;
				}
			}
#endif /* TOF_COLLECT_REMOTE */
			else {
				ret = TOF_RET_IGNORE;
			}
			break;
		default:
			ret = TOF_RET_IGNORE;
			break;
	}
	return ret;
}

/* TOF state machine */
static int pdburst_sm(pdburst_sm_t *sm, int event, const pdburst_tsinfo_t *protp,
	int paramlen, pdburst_data_t *datap)
{
	pdburst_t* burstp;
	int ret = TOF_RET_IGNORE;

	ASSERT(event < TOF_EVENT_LAST);
	ASSERT(sm != NULL);
	burstp = sm->tof_obj;
#if defined(TOF_PROFILE)
	wlc_read_tsf(burstp->wlc, &tsf_lastm, &tsf_hi);
	tsf_lastm -= tsf_start;

	if (protp) {
#ifdef TOF_PROFILE
		TOF_PRINTF(("EVENT = %d, TOKEN=%d FOLLOW_TOKEN=%d TIME = 0x%0x\n",
			event, protp->tx_id, protp->ts_id, tsf_lastm));
#endif

	} else {
#ifdef TOF_PROFILE
		TOF_PRINTF(("EVENT = %d, TIME = 0x%0x\n", event, tsf_lastm));
#endif
	}
#endif /* TOF_PROFILE */

	if (sm->tof_mode == WL_PROXD_MODE_DISABLE)
		return TOF_RET_SLEEP;

	if (event == TOF_EVENT_TMO && sm->tof_state != TOF_STATE_ICONFIRM) {
		pdburst_confirmed(sm, WL_PROXD_E_TIMEOUT);
		return TOF_RET_SLEEP;
	}

	if (event == TOF_EVENT_RXACT) {
		ASSERT(datap != NULL);

		if (sm->tof_mode == WL_PROXD_MODE_TARGET) {
			if (bcmp(&ether_bcast, &burstp->allow_mac, ETHER_ADDR_LEN) &&
				bcmp(&datap->tof_srcea, &burstp->allow_mac, ETHER_ADDR_LEN)) {
				return TOF_RET_IGNORE;
			}
		}
	}

	switch (sm->tof_state) {
		case TOF_STATE_IDLE:
			ret = pdburst_sm_idle(sm, event, datap);
			break;

		case TOF_STATE_IWAITM:
			ret = pdburst_sm_initiator_wait(sm, event, datap, protp);
			break;

		case TOF_STATE_ILEGACY:
			ret = pdburst_sm_legacy(sm, event);
			break;

		case TOF_STATE_TWAITM:
			ret = pdburst_sm_target_wait(sm, event, datap);
			break;

#ifdef TOF_COLLECT_REMOTE
		case TOF_STATE_IWAITCL:
			ret = pdburst_sm_initiator_collect_wait(sm, event, datap, protp);
			break;

		case TOF_STATE_TWAITCL:
			ret = pdburst_sm_target_collect_wait(sm, event);
			break;
#endif /* TOF_COLLECT_REMOTE */
		case TOF_STATE_IREPORT:
			if (event == TOF_EVENT_ACKED) {
				pdburst_report_done(sm, WL_PROXD_E_OK);
			} else {
				pdburst_report_done(sm, WL_PROXD_E_NOACK);
			}
			ret = TOF_RET_SLEEP;
			break;

		case TOF_STATE_ICONFIRM:
			if (event == TOF_EVENT_ACKED) {
				ret = TOF_RET_SLEEP;
			} else if (event == TOF_EVENT_NOACK) {
				ret = TOF_RET_SLEEP;
			}
			break;

		default:
			ASSERT(0);
			break;
	}

	return ret;
}

static int pdburst_init_tslist(pdburst_t * burstp, tof_tslist_t *listp, int list_cnt)
{
	if (!listp->tslist || (burstp->measurecnt != list_cnt)) {
		/* The measure counts changed */
		if (listp->tslist)
			MFREE(burstp->wlc->osh, listp->tslist, burstp->measurecnt *
				sizeof(ftmts_t));
		listp->tslist = MALLOCZ(burstp->wlc->osh, list_cnt * sizeof(ftmts_t));
		if (listp->tslist)
			burstp->measurecnt = list_cnt;
		else {
			burstp->measurecnt = 0;
			return BCME_NOMEM;
		}
		if (!burstp->lasttsp) {
			burstp->lasttsp = MALLOCZ(burstp->wlc->osh, sizeof(ftmts_t));
			if (!burstp->lasttsp)
			{
				MFREE(burstp->wlc->osh, listp->tslist, list_cnt * sizeof(ftmts_t));
				return BCME_NOMEM;
			}
		} else {
			pdburst_initiator_restore_last_frame(burstp, listp->tslist);
		}
	}
	listp->tscnt = 0;
	return BCME_OK;
}

static int pdburst_target_init(pdburst_t * burstp, const pdburst_params_t *params)
{
	int err = BCME_OK;
	if (!burstp->mf_buf) {
		if (!(burstp->mf_buf = MALLOCZ(burstp->wlc->osh, MF_BUF_MAX_LEN))) {
			err = BCME_NOMEM;
			goto done;
		}
	}
	burstp->mf_buf_len = MF_BUF_MAX_LEN;

	pdburst_reset(burstp->sm, WL_PROXD_MODE_TARGET, WL_PROXD_E_NOTSTARTED);
	burstp->sm->tof_dialog = params->dialog;
	if (burstp->flags & WL_PROXD_SESSION_FLAG_MBURST_FOLLOWUP) {
		if (burstp->tx_t1) {
			burstp->sm->tof_followup = params->dialog;
#ifdef TOF_DEBUG_TIME
			TOF_PRINTF(("followup token %d\n", burstp->sm->tof_followup));
#endif
		}
	} else {
		burstp->tx_t1 = 0;
		burstp->tx_t4 = 0;
		burstp->sm->tof_followup = 0;
	}

	if (params->req) {
		err = pdburst_rx_tof_params(burstp, PDBURST_FRAME_TYPE_REQ,
			params->req, params->req_len, params->req_rspec);
		if (err != BCME_OK)
			goto done;

		proxd_update_tunep_values(burstp->tunep, burstp->configp->chanspec,
			BURST_IS_VHTACK(burstp));
	}

done:
	return err;
}

static int pdburst_get_session_info(pdburst_t * burstp, const pdburst_params_t *params,
	pdburst_session_info_t *info)
{
	int err;

	err = PROTOCB(burstp, get_session_info, (burstp->ctx, info));
	if (err != BCME_OK) {
		goto done;
	}

	GCC_DIAGNOSTIC_PUSH_SUPPRESS_CAST();
	burstp->configp = (pdburst_config_t *)params->config;
	GCC_DIAGNOSTIC_POP();

	burstp->bsscfg = params->bsscfg;
	burstp->flags |= params->flags;
	burstp->lastburst = (info->flags & PDBURST_SESSION_FLAGS_LAST_BURST);
	if (info->flags & PDBURST_SESSION_FLAGS_MBURST_FOLLOWUP) {
		burstp->flags |= WL_PROXD_SESSION_FLAG_MBURST_FOLLOWUP;
	}
	/* note: seq, vht ack and secure flags are always in sync */
done:
	return err;
}

/* external interface */

pdburst_t*
pdburst_create(wlc_info_t *wlc, void *ctx, const pdburst_callbacks_t *callbacks)
{
	pdburst_t * burstp;

	ASSERT(wlc != NULL);
#ifdef TOF_DEBUG_TIME
	TOF_PRINTF(("pdburst_create\n"));
#endif
	burstp = MALLOCZ(wlc->osh, sizeof(pdburst_t));
	if (burstp != NULL) {
		burstp->wlc = wlc;
		burstp->ctx = ctx;
		burstp->chipnum = CHIPID(wlc->pub->sih->chip);
		burstp->chiprev = wlc->pub->sih->chiprev;

		burstp->svc_funcs = callbacks;
		burstp->phyver = wlc->band->phyrev;
		burstp->tunep = proxd_get_tunep(wlc, &burstp->Tq);

		burstp->sm = MALLOCZ(wlc->osh, sizeof(pdburst_sm_t));
		if (burstp->sm) {
			burstp->sm->tof_obj = burstp;
		} else {
			FTM_ERR(("Create tofpd obj failed\n"));
			goto err;
		}

		if ((burstp->duration_timer = wlc_hrt_alloc_timeout(wlc->hrti)) == NULL) {
			FTM_ERR(("wl%d: %s: wlc_hrt_alloc_timeout failed\n",
				wlc->pub->unit, __FUNCTION__));
			goto err;
		}

		if ((burstp->ftm_tx_timer = wlc_hrt_alloc_timeout(wlc->hrti)) == NULL) {
			FTM_ERR(("wl%d: %s: ftm_tx_timer hrt tmr alloc failed \n",
				wlc->pub->unit, __FUNCTION__));
			goto err;
		}
		burstp->ftm_tx_timer_active = FALSE;
		bcopy(&ether_bcast, &burstp->allow_mac, ETHER_ADDR_LEN);

		if (!(burstp->timer = wl_init_timer(wlc->wl, pdburst_timer,
			burstp, "pdburst"))) {
			FTM_ERR(("Create pdburst timer failed\n"));
			goto err;
		}

		/* Reset state machine */
		burstp->smstoped = TRUE;
		/* Get TOF shared memory address */
		burstp->shmemptr = wlc_read_shm(wlc, M_TOF_BLK_PTR(wlc)) << 1;
#ifdef TOF_COLLECT
		if (pdburst_collect) {
			ASSERT(burstp != pdburst_collect->pdburstp);
			if (pdburst_collect->pdburstp) {
				/* When we create new burst, free the previous one if it is not
				** collected yet. Then link collect data to the new burst.
				** collected yet. Then link collect data to the new burst.
				*/
				pdburst_collect->pdburstp->collectp = NULL;
				pdburst_destroy(&pdburst_collect->pdburstp);
			}
			burstp->collectp = pdburst_collect;
			pdburst_collect->pdburstp = burstp;
			burstp->tunep->minDT = -1;
			burstp->tunep->maxDT = -1;
		}
#endif  /* TOF_COLLECT */
		burstp->core = (burstp->tunep->core == 255) ? 0 : burstp->tunep->core;

	} else {
		FTM_ERR(("wl:%d %s MALLOC failed malloced %d bytes\n", wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->pub->osh)));
	}

	return (burstp);
err:
	if (burstp) {
		if (burstp->duration_timer != NULL) {
			wlc_hrt_free_timeout(burstp->duration_timer);
			burstp->duration_timer = NULL;
		}

		if (burstp->ftm_tx_timer != NULL) {
			wlc_hrt_free_timeout(burstp->ftm_tx_timer);
			burstp->ftm_tx_timer = NULL;
		}

		if (burstp->sm) {
			MFREE(wlc->osh, burstp->sm, sizeof(pdburst_sm_t));
			burstp->sm = NULL;
		}

		if (burstp->timer) {
			wl_free_timer(wlc->wl, burstp->timer);
			burstp->timer = NULL;
		}

		MFREE(wlc->osh, burstp, sizeof(pdburst_t));
	}

	return NULL;
}

static int
pdburst_resolve_initator_mac_addr(pdburst_t *burstp)
{
	int err = BCME_OK;
	struct ether_addr *mac = &burstp->configp->cur_ether_addr;

	ASSERT(burstp->sm->tof_mode == WL_PROXD_MODE_INITIATOR);
	if (ETHER_ISNULLADDR(mac)) {
		struct ether_addr *rand_mac;
		rand_mac = wlc_proxd_get_randmac(burstp->wlc->pdsvc_info, burstp->bsscfg);
		if (rand_mac) {
			memcpy(mac, rand_mac, ETHER_ADDR_LEN);
			burstp->randmac_in_use = TRUE;
		} else {
		if (burstp->deferred) {
			/* Burst Deferred/Resched case
			 * Do nothing as mac addr is already
			 * resolved while initial burst setup
			 */
			goto done;
		}
			/* in case WL_RANDMAC is undefined */
			memcpy(mac, &burstp->bsscfg->cur_etheraddr, ETHER_ADDR_LEN);
			err = BCME_OK;
		}
	}
done:
	return err;
}

void
pdftm_set_burst_deferred(pdftm_t *ftm, pdftm_session_t *sn)
{
	pdburst_t *burst;
	pdftm_session_state_t *sst;

	sst = sn->ftm_state;
	ASSERT(sst);
	burst = sst->burst;
	ASSERT(burst);

	burst->deferred = TRUE;
	return;
}

int
pdburst_init(pdburst_t *burstp, const pdburst_params_t *params)
{
	pdburst_session_info_t info;
	int err = BCME_OK;

	if (!burstp) {
		err = BCME_BADARG;
		goto done;
	}

	/* note: do this first */
	err = pdburst_get_session_info(burstp, params, &info);
	if (err != BCME_OK)
		goto done;

	memcpy(&burstp->sm->tof_selfea, burstp->bsscfg ? &burstp->bsscfg->cur_etheraddr :
		&ether_null, ETHER_ADDR_LEN);

	if (burstp->flags & WL_PROXD_SESSION_FLAG_INITIATOR) {
		tof_tslist_t *listp = &burstp->tof_tslist;
		uint8 list_cnt = pdburst_get_ftm_cnt(burstp);

		burstp->sm->tof_mode = WL_PROXD_MODE_INITIATOR;
		proxd_update_tunep_values(burstp->tunep, burstp->configp->chanspec,
			BURST_IS_VHTACK(burstp));
		if (pdburst_init_tslist(burstp, listp, list_cnt)) {
			burstp->smstoped = TRUE;
			err = BCME_NOMEM;
			goto done;
		}
	} else if (burstp->flags & WL_PROXD_SESSION_FLAG_TARGET) {
		err = pdburst_target_init(burstp, params);
	} else {
			err = BCME_UNSUPPORTED;
			goto done;
	}

	if (BURST_IS_VHTACK(burstp)) {
		info.flags |= PDBURST_SESSION_FLAGS_VHTACK;
		err = PROTOCB(burstp, set_session_info, (burstp->ctx, &info));
	}

done:
	return err;
}

int
pdburst_start(pdburst_t *burstp)
{
	pdburst_sm_t *sm;
	int err = BCME_OK;
	wlc_info_t *wlc;
#ifdef WL_PROXD_PHYTS
	pdftm_session_t *sn;
	uint8 phyts_role = 0u;
	bool enab_phyts;
#endif /* WL_PROXD_PHYTS */

	ASSERT(burstp != NULL);
	ASSERT(burstp->sm != NULL);
	ASSERT(burstp->wlc != NULL);

	sm = burstp->sm;
	wlc = burstp->wlc;

#ifdef TOF_DEBUG_TIME
	TOF_PRINTF(("%s mode %d\n", __FUNCTION__, sm->tof_mode));
#endif
#ifdef TOF_DEBUG_TIME2
	wlc_read_tsf(burstp->wlc, &tsf_start, &tsf_hi);
	tsf_scanstart = tsf_txreq = tsf_rxack =
	tsf_rxm = tsf_tmo = tsf_lastm = 0;
#endif
	burstp->smstoped = FALSE;
	burstp->distance = 0;
	burstp->meanrtt = 0;
	burstp->sdrtt = 0;
	bzero((void*)burstp->frame_type_cnt, sizeof(burstp->frame_type_cnt));
	bzero((void*)burstp->adj_type_cnt, sizeof(burstp->adj_type_cnt));

#ifdef TOF_COLLECT_DEBUG
	burstp->debug_cnt = 0;
#endif

#ifdef WL_PROXD_PHYTS
	sn = (pdftm_session_t *)burstp->ctx;
	if (burstp->flags & WL_PROXD_SESSION_FLAG_INITIATOR) {
		phyts_role = PHYTS_ROLE_INITIATOR;
	} else if (burstp->flags & WL_PROXD_SESSION_FLAG_TARGET) {
		phyts_role = PHYTS_ROLE_TARGET;
	}

	/* VHTACK is not known yet at burst start for non-BRCM initiators */
	enab_phyts = (BURST_IS_VHTACK(burstp) || (phyts_role == PHYTS_ROLE_TARGET) ||
		RSPEC_ISHE(burstp->configp->ratespec));
	if ((!FTM_AVB_TS(sn->ftm->config->flags)) &&
		(!(burstp->flags & WL_PROXD_SESSION_FLAG_ONE_WAY)) &&
		enab_phyts) {

		/* pass coreid to phy first */
		wlc_phy_tof_info_t tof_info;
		bzero(&tof_info, sizeof(tof_info));
		wlc_phy_tof_info(WLC_PI(burstp->wlc), &tof_info,
			WLC_PHY_TOF_INFO_TYPE_NONE, burstp->core);

		/* Enable sample-capture method */
		burstp->phyts_setup = TRUE;
		if ((err = phy_tof_phyts_setup_api(WLC_PI(burstp->wlc),
			TRUE, phyts_role)) != BCME_OK) {
			burstp->fallback_avb = TRUE;
			FTM_ERR(("%s phy_tof_phyts_setup_api failed %d\n", __FUNCTION__, err));
		}
	}
#endif /* WL_PROXD_PHYTS */

	if (sm->tof_mode == WL_PROXD_MODE_INITIATOR) {
		err = pdburst_resolve_initator_mac_addr(burstp);
		if (err != BCME_OK)
			goto done;
		pdburst_reset(sm, sm->tof_mode, WL_PROXD_E_NOTSTARTED);
		bcopy(&burstp->configp->peer_mac, &sm->tof_peerea, ETHER_ADDR_LEN);
		if (ETHER_ISNULLADDR(&burstp->configp->peer_mac)) {
			burstp->smstoped = TRUE;
			return BCME_BADADDR;
		}
		FTM_GET_TSF(wlc_ftm_get_handle(burstp->wlc), burstp->start_time);
		pdburst_activate_pm(burstp);
		(void) pdburst_sm(burstp->sm, TOF_EVENT_WAKEUP, NULL, 0, NULL);
	} else {
		/* start hrt timer for duration */
		wlc_hrt_add_timeout(burstp->duration_timer,
			(FTM_INTVL2USEC(&burstp->configp->duration) -
				FTM_TX_OVRHD_BURST_DURATION_US),
			pdburst_duration_expired_target, (void *)burstp);
		FTM_GET_TSF(wlc_ftm_get_handle(burstp->wlc), burstp->start_time);
#ifdef TOF_DEBUG_TIME
		TOF_PRINTF(("%s: BURST START TIME = %u.%u \n",
			__FUNCTION__, FTM_LOG_TSF_ARG(burstp->start_time)));
#endif
		burstp->duration_timer_active = TRUE;
		pdburst_process_rx_frame(burstp, NULL, NULL, 0, burstp->configp->ratespec, NULL);
		burstp->avb_active = TRUE;
		wlc_enable_avb_timer(wlc->hw, TRUE);
	}
	pdburst_phy_rxclass_war(burstp, burstp->configp->chanspec);
done:
#ifdef TOF_DEBUG_UCODE
	TOF_PRINTF(("%s err %d\n", __FUNCTION__, err));
#endif
	return err;
}

int
pdburst_suspend(pdburst_t *burstp)
{
	pdburst_sm_t *sm = NULL;

#ifdef TOF_DEBUG_TIME
	TOF_PRINTF(("%s\n", __FUNCTION__));
#endif

	if (burstp)
		sm = burstp->sm;

	if (sm && !burstp->smstoped) {
		if (sm->tof_mode == WL_PROXD_MODE_INITIATOR) {
			pdburst_confirmed(sm, WL_PROXD_E_CANCELED);
			burstp->smstoped = TRUE;
			pdburst_pwron(burstp, FALSE);
		} else {
			pdburst_hw(burstp, FALSE, FALSE);
			pdburst_target_done(sm, WL_PROXD_E_CANCELED);
		}
	}

	return BCME_OK;
}

int
pdburst_rx(pdburst_t *burstp, wlc_bsscfg_t *bsscfg, const dot11_management_header_t *hdr,
	const uint8 *body, int body_len, const wlc_d11rxhdr_t *wrxh, ratespec_t rspec,
	const pdburst_tsinfo_t *tsinfo)
{
	/* This is rxed measurement packet */
	if (burstp) {
		burstp->bsscfg = bsscfg;
#ifdef WL_PROXD_PHYTS
		burstp->rssi = wrxh->rssi;
#endif /* WL_PROXD_PHYTS */
		return pdburst_process_rx_frame(burstp, wrxh, body, body_len, rspec, tsinfo);
	}
#ifdef TOF_COLLECT_REMOTE
	else {
		if (pdburst_collect && pdburst_collect->pdburstp)
			return pdburst_process_rx_frame(pdburst_collect->pdburstp, wrxh, body,
				body_len, rspec, tsinfo);
		return BCME_DATA_NOTFOUND;
	}
#endif /* TOF_COLLECT_REMOTE */
	return BCME_OK;
}

void
pdburst_destroy(pdburst_t **in_burst)
{
	/* remove create method */
	pdburst_t *burstp;
#ifdef TOF_DEBUG_TIME
	TOF_PRINTF(("%s\n", __FUNCTION__));
#endif

	if (!in_burst)
		goto done;

	burstp = *in_burst;
	*in_burst = NULL;
	if (burstp) {
#ifdef WL_PROXD_PHYTS
		/* Clean up sample-capture(loopback) setup */
		if (burstp->phyts_setup) {
			if (phy_tof_phyts_setup_api(WLC_PI(burstp->wlc),
				FALSE, PHYTS_ROLE_NONE) != BCME_OK) {
				FTM_ERR(("%s phy_tof_phyts_setup_api failed \n", __FUNCTION__));
			}
			burstp->phyts_setup = FALSE;
		}
#endif /* WL_PROXD_PHYTS */
		burstp->smstoped = TRUE;
		burstp->ctx = NULL;
		burstp->configp = NULL;
		burstp->svc_funcs = NULL;
#ifdef TOF_COLLECT
		if (burstp->collectp) {
			if (burstp->collectp->collect_method &
				WL_PROXD_COLLECT_METHOD_TYPE_IOVAR) {
				if (pdburst_collect ||
					(burstp->sm->tof_state == TOF_STATE_ICONFIRM)) {
					/* wait collect to finish, then destroy */
					if (FTM_BSSCFG_SECURE(wlc_ftm_get_handle(burstp->wlc),
						burstp->bsscfg)) {
						phy_rxgcrs_sel_classifier(
							WLC_PI(burstp->wlc),
							TOF_CLASSIFIER_BPHY_ON_OFDM_ON);
					}
					goto done;
				}
			} else {
				/* Non-IOVAR so no need to retain collect buffers */
				pdburst_collect_mem(burstp->wlc, &pdburst_collect, FALSE);
				if (FTM_BSSCFG_SECURE(wlc_ftm_get_handle(burstp->wlc),
					burstp->bsscfg)) {
					phy_rxgcrs_sel_classifier(
						WLC_PI(burstp->wlc),
						TOF_CLASSIFIER_BPHY_ON_OFDM_ON);
				}
				burstp->collectp = NULL;
			}
		}
#endif /* TOF_COLLECT */
		if (burstp->randmac_in_use) {
			wlc_proxd_release_randmac(burstp->wlc->pdsvc_info, burstp->bsscfg);
			burstp->randmac_in_use = FALSE;
		}

		if (burstp->start_time) {
			burstp->start_time = 0;
#ifdef WL_PROXD_UCODE_TSYNC
			if (PROXD_ENAB_UCODE_TSYNC(burstp->wlc->pub)) {
				pdburst_clear_ucode_ack_block(burstp->wlc);
			}
#endif /* WL_PROXD_UCODE_TSYNC */
			pdburst_deactivate_pm(burstp);

			if (!burstp->tof_reset) {
				pdburst_measure(burstp, TOF_RESET);
			}
			phy_tof_cmd(WLC_PI(burstp->wlc), FALSE, 0);
			if (burstp->meas_mode) {
				pdburst_hw(burstp, FALSE, FALSE);
			}
			if (FTM_BSSCFG_SECURE(wlc_ftm_get_handle(burstp->wlc),
				burstp->bsscfg)) {
				phy_rxgcrs_sel_classifier(WLC_PI(burstp->wlc),
					TOF_CLASSIFIER_BPHY_ON_OFDM_ON);
			}
		}
#ifdef WL_RANGE_SEQ
		if (!BURST_NOCHANSWT(burstp->flags) && burstp->noscanengine)
#endif
		pdburst_pwron(burstp, FALSE);

		if (burstp->timeractive) {
			burstp->timeractive = FALSE;
			wl_del_timer(burstp->wlc->wl, burstp->timer);
		}
		if (burstp->duration_timer != NULL) {
			wlc_hrt_free_timeout(burstp->duration_timer);
			burstp->duration_timer_active = FALSE;
			burstp->duration_timer = NULL;
		}

		if (burstp->ftm_tx_timer != NULL) {
			wlc_hrt_free_timeout(burstp->ftm_tx_timer);
			burstp->ftm_tx_timer_active = FALSE;
			burstp->ftm_tx_timer = NULL;
		}

		if (burstp->sm) {
			MFREE(burstp->wlc->osh, burstp->sm, sizeof(pdburst_sm_t));
			burstp->sm = NULL;
		}

		if (burstp->timer) {
			wl_free_timer(burstp->wlc->wl, burstp->timer);
			burstp->timer = NULL;
		}

		if (burstp->tof_tslist.tslist) {
			MFREE(burstp->wlc->osh, burstp->tof_tslist.tslist,
				burstp->measurecnt * sizeof(ftmts_t));
			burstp->tof_tslist.tslist = NULL;
		}

		if (burstp->lasttsp) {
			MFREE(burstp->wlc->osh, burstp->lasttsp, sizeof(ftmts_t));
			burstp->lasttsp = NULL;
		}

		if (burstp->mf_buf) {
			MFREE(burstp->wlc->osh, burstp->mf_buf, burstp->mf_buf_len);
			burstp->mf_buf = NULL;
			burstp->mf_buf_len = 0;
		}

		if (burstp->mf_stats_buf) {
			MFREE(burstp->wlc->osh, burstp->mf_stats_buf, burstp->mf_stats_buf_len);
			burstp->mf_stats_buf = NULL;
			burstp->mf_stats_buf_len = 0;
		}
		if (!(burstp->delayfree & TOF_DFREE_TXDONE)) {
			/* No measurement frame is pending, remove callback */
			wlc_pcb_fn_find(burstp->wlc->pcb, pdburst_tx_complete, burstp, TRUE);
		}
		/* mark burst first as destroyed - even when freeing */
		burstp->destroyed = TRUE;
		if (!burstp->delayfree)
			MFREE(burstp->wlc->osh, burstp, sizeof(pdburst_t));
	}

done:
	return;
}

void
pdburst_dump(const pdburst_t *burst, struct bcmstrbuf *b)
{
}
#if defined(TOF_COLLECT) || defined(TOF_COLLECT_REMOTE)
static void
pdburst_collect_prep_header(pdburst_collect_t *collectp,
	wl_proxd_collect_header_t *header)
{
	ratespec_t ackrspec;
	pdburst_t *burstp;
	wlc_info_t *wlc;

	if (!collectp || !header)
		return;

	if (!collectp->pdburstp)
		return;

	burstp = collectp->pdburstp;
	wlc = burstp->wlc;
	header->total_frames = (uint16)collectp->collect_cnt;
	if (CHSPEC_IS160(collectp->configp->chanspec)) {
		header->nfft = 512;
		header->bandwidth = 160;
	} else if (CHSPEC_IS80(collectp->configp->chanspec)) {
		header->nfft = 256;
		header->bandwidth = 80;
	} else if (CHSPEC_IS40(collectp->configp->chanspec)) {
		header->nfft = 128;
		header->bandwidth = 40;
	} else if (CHSPEC_IS20(collectp->configp->chanspec)) {
		header->nfft = 64;
		header->bandwidth = 20;
	} else {
		header->nfft = 0;
		header->bandwidth = 10;
	}

	header->channel = CHSPEC_CHANNEL(collectp->configp->chanspec);
	header->chanspec = burstp->configp->chanspec;
	header->fpfactor = burstp->Tq;
	header->fpfactor_shift = TOF_SHIFT;
	memcpy((void*)&header->params, (void*)burstp->tunep,
		sizeof(wl_proxd_params_tof_tune_t));
	if (!BURST_IS_VHTACK(burstp) ||
		!RSPEC_ISVHT(burstp->configp->ratespec)) {
		/* no vhtack */
		ackrspec = LEGACY_RSPEC(PROXD_DEFAULT_TX_RATE);
	} else {
		ackrspec = LEGACY_RSPEC(PROXD_DEFAULT_TX_RATE) |
			WL_RSPEC_ENCODE_VHT;
	}
	wlc_phy_kvalue(WLC_PI(wlc), burstp->configp->chanspec,
		proxd_get_ratespec_idx(burstp->configp->ratespec,
			ackrspec),
		&header->params.Ki, &header->params.Kt,
		((collectp->collect_info.type == TOF_ADJ_SEQ) ?
			WL_PROXD_SEQEN : 0));
	header->distance = burstp->distance;
	header->meanrtt = burstp->meanrtt;
	header->modertt = 0;
	header->medianrtt = 0;
	header->sdrtt = burstp->sdrtt;
	si_pmu_fvco_pllreg(wlc->hw->sih, NULL, &header->clkdivisor);
	header->clkdivisor &= PMU1_PLL0_PC1_M1DIV_MASK;
	header->chipnum = burstp->chipnum;
	header->chiprev = burstp->chiprev;
	header->phyver = wlc->band->phyrev;

	memcpy(&header->localMacAddr, &burstp->sm->tof_selfea,
		ETHER_ADDR_LEN);
	if (ETHER_ISNULLADDR(&burstp->sm->tof_peerea))
		memcpy(&header->remoteMacAddr, &collectp->configp->peer_mac,
			ETHER_ADDR_LEN);
	else
		memcpy(&header->remoteMacAddr, &burstp->sm->tof_peerea,
			ETHER_ADDR_LEN);
}
#endif /* TOF_COLLECT || TOF_COLLECT_REMOTE */

#if defined(TOF_COLLECT) || defined(TOF_COLLECT_INLINE)
static void
pdburst_collect_init_event(wl_proxd_event_t *event,
	wl_proxd_session_id_t sid)
{
	event->version = htol16(WL_PROXD_API_VERSION);
	event->len = htol16(OFFSETOF(wl_proxd_event_t, tlvs));
	event->type = htol16(WL_PROXD_EVENT_COLLECT);
	event->method = htol16(WL_PROXD_METHOD_FTM);
	event->sid = htol16(sid);
	bzero(event->pad, sizeof(event->pad));
}
#endif

#ifdef TOF_COLLECT
static int pdburst_collect_event(pdburst_t *burstp)
{
	int ret = BCME_OK;
	if (burstp && burstp->collectp) {
		if (burstp->collectp->collect_method & WL_PROXD_COLLECT_METHOD_TYPE_EVENT) {
			ret = pdburst_collect_generic_event(burstp);
		}
	}
	return ret;
}

static int pdburst_collect_generic_event(pdburst_t *burstp)
{
	int i;
	int ret = BCME_OK;
	pdftm_session_t *sn = NULL;
	wl_proxd_session_id_t sid = 0;
	pdburst_collect_t *collectp = NULL;
	uint16 event_len = 0;
	uint16 tlv_len = 0;
	uint16 buf_size = 0;
	uint8 *event_buf = NULL;
	wl_proxd_event_t *event = NULL;
	wl_proxd_tlv_t *tlv = NULL;
	wl_proxd_collect_event_data_t *cop = NULL;
	wl_proxd_collect_data_t *collect = NULL;
	uint32 *outp = NULL;
#ifdef WL_PROXD_SEQ
	wlc_phy_tof_info_t tof_info;
#endif /* WL_PROXD_SEQ */

	if (!burstp || !burstp->collectp) {
		return BCME_NOTREADY;
	}

	collectp = burstp->collectp;
	if (!collectp->collect_cnt) {
		return BCME_NOTREADY;
	}

	if (!CHSPEC_IS20(collectp->configp->chanspec)) {
		/* only supports 20MHZ */
		return BCME_UNSUPPORTED;
	}

	buf_size = sizeof(wl_proxd_event_t) + sizeof(wl_proxd_tlv_t) +
		sizeof(wl_proxd_collect_event_data_t);
	if ((event_buf = (uint8 *)MALLOCZ(burstp->wlc->osh, buf_size)) == NULL) {
		FTM_ERR(("wl:%d %s MALLOC failed malloced %d bytes\n", burstp->wlc->pub->unit,
			__FUNCTION__, MALLOCED(burstp->wlc->pub->osh)));
		return BCME_NOMEM;
	}

	event = (wl_proxd_event_t *)event_buf;
	tlv = (wl_proxd_tlv_t *)event->tlvs;
	cop = (wl_proxd_collect_event_data_t *)tlv->data;

	sn = (pdftm_session_t *)burstp->ctx;
	sid = (sn != NULL)? sn->sid : WL_PROXD_SESSION_ID_GLOBAL;

	/* calculate lengths */
	tlv_len = sizeof(wl_proxd_collect_event_data_t);
	event_len = OFFSETOF(wl_proxd_event_t, tlvs) + OFFSETOF(wl_proxd_tlv_t, data) + tlv_len;

	/* initialize event/tlv header info */
	bzero(event_buf, sizeof(event_buf));
	pdburst_collect_init_event(event, sid);
	event->type = WL_PROXD_EVENT_COLLECT;
	event->len = event_len;
	tlv->id = WL_PROXD_TLV_ID_COLLECT_DATA;
	tlv->len = tlv_len;
	int collect_h_size;

	if (CHSPEC_IS20(burstp->configp->chanspec)) {
		collect_h_size = K_TOF_COLLECT_H_SIZE_20MHZ;
#ifndef WL_RANGE_SEQ
	} else {
		collect_h_size = K_TOF_COLLECT_H_SIZE_80MHZ;
	}
#endif /* !WL_RANGE_SEQ */
	if (collectp->collect_cnt && collectp->collect_cnt <= collectp->collect_size) {
		for (i = 0; i < collectp->collect_cnt; i++) {
			collect = collectp->collect + i;
			if (collect) {
#ifdef WL_RANGE_SEQ
				outp = (!i)? cop->H_LB : cop->H_RX;
				(void)memcpy_s(outp, collect_h_size * sizeof(uint32),
					collect->H, (collect_h_size * sizeof(uint32)));
#else
				outp = (i)? cop->H_LB : cop->H_RX;
				memcpy(outp, collect->H,
					(collect_h_size * sizeof(uint32)));
#endif
			}
		}
	}
#ifdef WL_RANGE_SEQ
	} else {
		collect_h_size = K_TOF_COLLECT_H_SIZE_80MHZ;
	}
	(void)memcpy_s(cop->ri_rr, sizeof(collectp->ri_rr),
		collectp->ri_rr, sizeof(collectp->ri_rr));
#else
	memcpy(cop->ri_rr, collectp->ri_rr, sizeof(collectp->ri_rr));
#endif /* WL_RANGE_SEQ */
	/* query phy for phy error mask */
#ifdef WL_PROXD_SEQ
	bzero(&tof_info, sizeof(tof_info));
	wlc_phy_tof_info(WLC_PI(burstp->wlc), &tof_info,
		WLC_PHY_TOF_INFO_TYPE_ALL, burstp->core);
	cop->phy_err_mask = tof_info.tof_phy_error;
#else /* WL_PROXD_SEQ */
	cop->phy_err_mask = 0;
#endif /* WL_PROXD_SEQ */

	cop->version = WL_PROXD_COLLECT_EVENT_DATA_VERSION_MAX;
#ifdef WL_RANGE_SEQ
	cop->length = sizeof(*cop) - OFFSETOF(wl_proxd_collect_event_data_t, H_LB);
#else
	cop->length = sizeof(*cop) - OFFSETOF(*cop, H_LB);
#endif /* WL_RANGE_SEQ */
	proxd_send_event(burstp->wlc->pdsvc_info, burstp->bsscfg, WL_PROXD_E_OK,
		&burstp->sm->tof_selfea, event, event->len);

#ifdef WL_RANGE_SEQ
	MFREE(collectp->pdburstp->wlc->osh, event_buf, buf_size);
#else
	MFREE(burstp->wlc->osh, event_buf, buf_size);
#endif
	return ret;
}

#endif /* TOF_COLLECT */

uint8 ftm_vs_get_tof_txcnt(void *burst)
{
	pdburst_t *pdburstp = (pdburst_t *)burst;
	if ((pdburstp) && (pdburstp->sm)) {
		TOF_PRINTF(("ftm_vs_get_tof_txcnt(), tof_txcnt %d\n", pdburstp->sm->tof_txcnt));
		return pdburstp->sm->tof_txcnt;
	}
	return 0;
}

uint8 *
ftm_vs_init_mf_stats_buf(void *burstp)
{
	pdburst_t *pdburstp = (pdburst_t *)burstp;
	ASSERT(pdburstp != NULL);
	if (!pdburstp->mf_stats_buf) {
		if (!(pdburstp->mf_stats_buf =
			MALLOCZ(pdburstp->wlc->osh, MF_BUF_MAX_LEN))) {
			WL_ERROR(("ftm_vs_init_mf_stats_buf()"
				"MALLOC of mf_stats_buf data failed\n"));
			return NULL;
		}
		pdburstp->mf_stats_buf_len = MF_BUF_MAX_LEN;
	}
	return (uint8*)pdburstp->mf_stats_buf;
}

static void
pdburst_mf_stats_init_event(wl_proxd_event_t *event,
	wl_proxd_session_id_t sid)
{
	event->version = htol16(WL_PROXD_API_VERSION);
	event->len = htol16(OFFSETOF(wl_proxd_event_t, tlvs));
	event->type = htol16(WL_PROXD_EVENT_MF_STATS);
	event->method = htol16(WL_PROXD_METHOD_FTM);
	event->sid = htol16(sid);
	bzero(event->pad, sizeof(event->pad));
}

void pdburst_send_mf_stats_event(pdburst_t *burstp)
{
	pdftm_session_t *sn;
	wl_proxd_session_id_t sid;
	uint16 buf_len  = 0, buf_size = 0;
	uint8 *event_buf = NULL;
	wl_proxd_event_t *event = NULL;
	wl_proxd_tlv_t *tlv = NULL;

	ASSERT(burstp != NULL);
	if (!burstp) {
		return;
	}

	sn = (pdftm_session_t *)burstp->ctx;
	sid = (sn != NULL)? sn->sid : WL_PROXD_SESSION_ID_GLOBAL;

	buf_len = burstp->mf_stats_buf_len;
	buf_size = sizeof(wl_proxd_event_t) + sizeof(wl_proxd_tlv_t) + buf_len;

	event_buf = (uint8 *)MALLOCZ(burstp->wlc->osh, buf_size);
	if (!event_buf) {
		WL_ERROR(("MALLOC failed %s\n", __FUNCTION__));
		return;
	}

	event = (wl_proxd_event_t *)event_buf;
	pdburst_mf_stats_init_event(event, sid);
	event->len = OFFSETOF(wl_proxd_event_t, tlvs) + OFFSETOF(wl_proxd_tlv_t, data) + buf_len;

	tlv = (wl_proxd_tlv_t *)event->tlvs;
	tlv->id = WL_PROXD_TLV_ID_MF_STATS_DATA;
	tlv->len = buf_len;

	memcpy(tlv->data, burstp->mf_stats_buf, buf_len);
	proxd_send_event(burstp->wlc->pdsvc_info, burstp->bsscfg, WL_PROXD_E_OK,
		&burstp->sm->tof_selfea, event, event->len);

	MFREE(burstp->wlc->osh, event_buf, buf_size);
	return;
}

void ftm_vs_update_mf_stats_len(void *burstp, uint16 len)
{
	pdburst_t *pdburstp = (pdburst_t *)burstp;
	ASSERT(pdburstp != NULL);
	if ((pdburstp) && (pdburstp->mf_stats_buf)) {
		pdburstp->mf_stats_buf_len = len;
	}
}
#ifdef WL_PROXD_UCODE_TSYNC
/* For SHM definitions refer below confluence
* http://confluence.broadcom.com/display/WLAN/NewUcodeInterfaceForProxdFeature
*/
#define FTM_TIMESTAMP_SHIFT		16
#define TOF_AVB_ACK_BLOCK_SIZE		6
#define TOF_AVB_ACK_BLOCK_MASK		0x8000
#define TXS_ACK_INDEX_SHIFT		3
#define RXH_ACK_INDEX_SHIFT		8
#define RXH_ACK_INDEX_SHIFT_REV_GE80	12
#define ACK_BLOCK_SIZE			3
#define NUM_UCODE_ACK_TS_BLKS		4

void
pdburst_process_tx_rx_status(wlc_info_t *wlc, tx_status_t *txs,
	d11rxhdr_t *rxh, struct ether_addr *peer)
{
	uint32 ftm_avb, ack_avb;
	uint32 off_set;
	uint16 ack_avb_blk[ACK_BLOCK_SIZE];
	int i;
	uint8 index;
	pdburst_sm_t *sm = NULL;
	pdburst_t *p = NULL;
	uint8 rxh_ack_shift;
	uint8 err_at = 0;
	pdftm_session_t *sn = NULL;
	chanspec_t chanspec;

#ifdef TOF_DEBUG_UCODE
	TOF_PRINTF(("%s: tx_rx_status\n",
		(txs ? "TXS" : "RXS")));
#endif

	/* check if the peer session is still in a valid state for txrx */
	sn = FTM_SESSION_FOR_PEER(wlc_ftm_get_handle(wlc), peer, WL_PROXD_SESSION_FLAG_NONE);
	if (!sn || sn->state > WL_PROXD_SESSION_STATE_BURST ||
	    sn->state < WL_PROXD_SESSION_STATE_STARTED) {
		err_at = 1;
		goto done;
	}

	if (txs) {
		ftm_avb = TX_STATUS_MACTXS_ACK_MAP2(txs);
		index = (TX_STATUS_MACTXS_S5(txs) >> TXS_ACK_INDEX_SHIFT) & 0x07;
	} else {

		if (D11REV_GE(wlc->pub->corerev, 80)) {
			rxh_ack_shift = RXH_ACK_INDEX_SHIFT_REV_GE80;
		} else {
			rxh_ack_shift = RXH_ACK_INDEX_SHIFT;
		}

		if (D11REV_GE(wlc->pub->corerev, 129)) {
			/*
			Note: for non-FTM frames, AvbRxTimeH is reused for uplink
			rate info on AX chips
			*/
			ftm_avb = (D11RXHDR_GE129_ACCESS_VAL(rxh, AvbRxTimeL) |
				(D11RXHDR_GE129_ACCESS_VAL(rxh, UlRtInfo) <<
				FTM_TIMESTAMP_SHIFT));

			index = (D11RXHDR_GE129_ACCESS_VAL(rxh, RxStatus3) >> rxh_ack_shift) & 0x07;
		} else {
			ftm_avb = (D11RXHDR_LT80_ACCESS_VAL(rxh, AvbRxTimeL) |
				(D11RXHDR_LT80_ACCESS_VAL(rxh, AvbRxTimeH) <<
				FTM_TIMESTAMP_SHIFT));

			index = (D11RXHDR_LT80_ACCESS_VAL(rxh, MuRate) >> rxh_ack_shift) & 0x07;
		}
	}
	FTM_TRACE(("%s: AVB FTMTS : %u tof_avb_block index %d\n",
		(txs ? "TXS" : "RXS"), ftm_avb, index));

	if (D11REV_GE(wlc->pub->corerev, 80)) {
		if (index == 0x04) { /* invalid index 0x04..check */
			FTM_ERR(("No available block, so ack ts not valid..ignore\n"));
			err_at = 2;
			goto done;
		}
	}
	off_set = index * TOF_AVB_ACK_BLOCK_SIZE + M_TOF_AVB_BLK(wlc->hw);

	if (!wlc->hw->clk) {
		err_at = 3;
		goto done;
	}
	/* Wait until ack time stamps are updated/valid */
	i = 0;
	while (!((wlc_bmac_read_shm(wlc->hw, off_set)) & TOF_AVB_ACK_BLOCK_MASK) &&
		(i < TOF_ACK_TS_TIMEOUT)) {
		OSL_DELAY(1);
		i++;
	}

	for (i = 0; i < ACK_BLOCK_SIZE; i++) {
		ack_avb_blk[i] = wlc_bmac_read_shm(wlc->hw, off_set + 2*i);
	}

	/* IMP: reset the first word in ACK block so that UCODE
	* can reuse it. Note that we have only 4 ACK blocks
	*/
	wlc_bmac_write_shm(wlc->hw, off_set, 0x0000);

	if (txs && !(txs->status.was_acked)) {
		FTM_ERR(("TXS: packet not acked, so ack ts will not be valid..ignore\n"));
		err_at = 4;
		goto done;
	}

	FTM_TRACE(("%s: avb1 0x%04x avb2 0x%04x avb3 0x%04x\n", (txs ? "TXS" : "RXS"),
		ack_avb_blk[0], ack_avb_blk[1], ack_avb_blk[2]));

	if (sn->ftm_state) {
		p = sn->ftm_state->burst;
	}

	if (!p) {
		FTM_ERR(("%s:burst/ssn not created or already freed\n", __FUNCTION__));
		err_at = 6;
		goto done;
	}

	p->is_valid_ts = FALSE;

	if (!(ack_avb_blk[0] & TOF_AVB_ACK_BLOCK_MASK)) {
		FTM_ERR(("%s: ack field not valid \n", (txs ? "TXS" : "RXS")));
		FTM_ERR(("%s: avb1 0x%04x avb2 0x%04x avb3 0x%04x\n", (txs ? "TXS" : "RXS"),
			ack_avb_blk[0], ack_avb_blk[1], ack_avb_blk[2]));
		/* skip this avb error if we are trying to do phyts first */
		if (p->phyts_setup != TRUE) {
			wlc_bmac_update_shm(wlc->hw, M_TOF_FLAGS(wlc),
				0 << TOF_AVB_ACK_ON_NBIT, 1 << TOF_AVB_ACK_ON_NBIT);
			err_at = 5;
			goto done;
		}
	}

	sm = p->sm;
	if (!sm) {
		FTM_ERR(("TXS: Invalid state machine..some thing wrong\n"));
		err_at = 7;
		goto done;
	}
	ack_avb = ((ack_avb_blk[1]) & 0xFFFF) |
		((ack_avb_blk[2] & 0xFFFF) << FTM_TIMESTAMP_SHIFT);
	if (txs) {
		sm->tod = ftm_avb;
		sm->toa = ack_avb;
		p->ack_rx_rspec = pdburst_get_tgt_ackrspec(p, ack_avb_blk[0]);
		chanspec = pdburst_get_chspec_from_rspec(p, p->ack_rx_rspec);

		/* target: handle vhtack automatically */
		if (RSPEC_ISHE(p->ack_rx_rspec)) {
			p->flags |= WL_PROXD_SESSION_FLAG_HEACK;
			p->flags &= ~(WL_PROXD_SESSION_FLAG_VHTACK);
		} else if (RSPEC_ISVHT(p->ack_rx_rspec)) {
			p->flags |= WL_PROXD_SESSION_FLAG_VHTACK;
			p->flags &= ~(WL_PROXD_SESSION_FLAG_HEACK);
		} else {
			p->flags &= ~(WL_PROXD_SESSION_FLAG_VHTACK |
				WL_PROXD_SESSION_FLAG_HEACK);
		}
		proxd_update_tunep_values(p->tunep, chanspec,
		      BURST_IS_VHTACK(p));

	} else {
		sm->tod = ack_avb;
		sm->toa = ftm_avb;
	}

	FTM_TRACE(("\n%s: AVB FTMTS : %u ACKTS : %u tof_avb_block index %d VHTACK %d HEACK %d\n",
		(txs ? "TXS" : "RXS"), ftm_avb, ack_avb, index, BURST_IS_VHTACK(p),
		BURST_IS_HEACK(p)));
	p->is_valid_ts = TRUE;

done:
	if (err_at != 0) {
		FTM_ERR(("pdburst_process_tx_rx_status: Failed err_at %d peer " MACF "\n",
			err_at, ETHERP_TO_MACF(peer)));
	}
	return;

}

/* Called when destroying the burst.
* It can happen by the time a FTM measurement is received in
* ucode, burst might be stopping. This case randmac will clear
* the multi_mac and ATM entries which results in tossing the
* received frame. So clear/reset all ACK SHM blocks
*/
static void
pdburst_clear_ucode_ack_block(wlc_info_t *wlc)
{
	uint8 index = 0;
	uint16 off_set = 0;

	if (!wlc->hw->clk) {
		return;
	}

	for (index = 0; index < NUM_UCODE_ACK_TS_BLKS; index++) {
		off_set = index * TOF_AVB_ACK_BLOCK_SIZE +
			M_TOF_AVB_BLK(wlc->hw);
			wlc_bmac_write_shm(wlc->hw, off_set, 0x0000);
	}
}
#endif /* WL_PROXD_UCODE_TSYNC */
