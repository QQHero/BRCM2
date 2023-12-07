/*
 * MSDU aggregation protocol source file
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_amsdu.c 810891 2022-04-19 05:55:11Z $
 */

/**
 * C preprocessor flags used within this file:
 * WLAMSDU       : if defined, enables support for AMSDU reception
 * WLAMSDU_TX    : if defined, enables support for AMSDU transmission
 * BCMDBG_AMSDU  : enable AMSDU debug/dump facilities
 * PKTC          : packet chaining support for NIC driver model
 * PKTC_DONGLE   : packet chaining support for dongle firmware
 * WLOVERTHRUSTER: TCP throughput enhancing feature
 * PROP_TXSTATUS : transmit flow control related feature
 * BCM_GMAC3     : Atlas (4709) router chip specific Ethernet interface
 */

/* XXX: Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>

#if !defined(WLAMSDU) && !defined(WLAMSDU_TX)
#error "Neither WLAMSDU nor WLAMSDU_TX is defined"
#endif

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <bcmdevs.h>
#include <802.1d.h>
#include <802.11.h>
#include <wlioctl.h>
#include <bcmwpa.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_phy_hal.h>
#include <wlc_frmutil.h>
#include <wlc_pcb.h>
#include <wlc_scb_ratesel.h>
#include <wlc_rate_sel.h>
#include <wlc_amsdu.h>
#ifdef PROP_TXSTATUS
#include <wlc_wlfc.h>
#endif
#include <wlc_ampdu.h>
#if defined(WLOVERTHRUSTER)
/* headers to enable TCP ACK bypass for Overthruster */
#include <wlc_ampdu.h>
#include <ethernet.h>
#include <bcmip.h>
#include <bcmtcp.h>
#endif /* WLOVERTHRUSTER */
#ifdef PSTA
#include <wlc_psta.h>
#endif
#ifdef WL11AC
#include <wlc_vht.h>
#endif /* WL11AC */
#include <wlc_ht.h>
#include <wlc_tx.h>
#include <wlc_rx.h>
#include <wlc_bmac.h>
#include <wlc_pktc.h>
#include <wlc_dump.h>
#include <wlc_hw.h>
#include <wlc_ratelinkmem.h>
#include <wlc_dbg.h>

#ifdef DONGLEBUILD
#include <hnd_cplt.h>
#endif
#if defined(BCMSPLITRX)
#include <wlc_pktfetch.h>
#endif
#include <d11_cfg.h>
#include <wlc_log.h>

#ifdef WL_MU_RX
#include <wlc_murx.h>
#endif /* WL_MU_RX */

#ifdef PKTC_TBL
#include <wl_pktc.h>
#endif

#ifdef WLCFP
#include <wlc_cfp.h>
#endif /* WLCFP */
#ifdef HNDPQP
#include <hnd_pqp.h>
#endif

/* default values for tunables, iovars */
#define AMSDU_MAX_MSDU_PKTLEN	VHT_MAX_AMSDU	/**< max pkt length to be aggregated */
#define AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB(wlc)	\
	D11REV_LT((wlc)->pub->corerev, 132)	/**< use ht agg limits for vht */
#define AMSDU_AGGBYTES_MIN	500		/**< the lowest aggbytes allowed */
#define MAX_TX_SUBFRAMES_LIMIT	16		/**< the highest aggsf allowed */

#define MAX_RX_SUBFRAMES	100		/**< max A-MSDU rx size/smallest frame bytes */
#ifndef BCMSIM
#define MAX_TX_SUBFRAMES	14		/**< max num of MSDUs in one A-MSDU */
#else
#define MAX_TX_SUBFRAMES	5		/**< for linuxsim testing */
#endif
#define AMSDU_RX_SUBFRAMES_BINS	8		/**< number of counters for amsdu subframes */

#ifndef MAX_TX_SUBFRAMES_ACPHY
#define MAX_TX_SUBFRAMES_ACPHY	2		/**< max num of MSDUs in one A-MSDU */
#endif
#ifndef MAX_TX_SUBFRAMES_AXPHY
#define MAX_TX_SUBFRAMES_AXPHY	8		/**< max num of MSDUs in one A-MSDU */
#endif

/* Statistics */

/* Number of length bins in length histogram */
#ifdef WL11AC
#define AMSDU_LENGTH_BINS	12
#else
#define AMSDU_LENGTH_BINS	8
#endif

/* Number of bytes in length represented by each bin in length histogram */
#define AMSDU_LENGTH_BIN_BYTES	1024

/* SW RX private states */
#define WLC_AMSDU_DEAGG_IDLE	0	/**< idle */
#define WLC_AMSDU_DEAGG_FIRST	1	/**< deagg first frame received */
#define WLC_AMSDU_DEAGG_LAST	3	/**< deagg last frame received */

#define AMSDU_OFF_RATE_THRESH	20000	/* Threshold for 1 sf aggregation, in Kbps */
#define AMSDU_ON_RATE_THRESH	40000	/* Threshold to restore aggsf, in Kbps */

#ifdef WLCNT
#define	WLC_AMSDU_CNT_VERSION	4	/**< current version of wlc_amsdu_cnt_t */

/** block ack related stats */
typedef struct wlc_amsdu_cnt {
	/* Transmit aggregation counters */
	uint16 version;               /**< WLC_AMSDU_CNT_VERSION */
	uint16 length;                /**< length of entire structure */

	uint32 agg_passthrough;       /**< num of MSDU pass through w/o A-MSDU agg */
	uint32 agg_amsdu;             /**< num of A-MSDU released */
	uint32 agg_msdu;              /**< num of MSDU aggregated in A-MSDU */
	uint32 agg_stop_tailroom;     /**< num of MSDU aggs stopped for lack of tailroom */
	uint32 agg_stop_sf;           /**< num of MSDU aggs stopped for sub-frame count limit */
	uint32 agg_stop_len;          /**< num of MSDU aggs stopped for byte length limit */
	uint32 agg_stop_tcpack;       /**< num of MSDU aggs stopped for encountering a TCP ACK */
	uint32 agg_stop_suppr;        /**< num of MSDU aggs stopped for suppressed packets */

	/* Receive Deaggregation counters */
	uint32 deagg_msdu;           /**< MSDU of deagged A-MSDU(in ucode) */
	uint32 deagg_amsdu;          /**< valid A-MSDU deagged(exclude bad A-MSDU) */
	uint32 deagg_badfmt;         /**< MPDU is bad */
	uint32 deagg_wrongseq;       /**< MPDU of one A-MSDU doesn't follow sequence */
	uint32 deagg_badsflen;       /**< MPDU of one A-MSDU has length mismatch */
	uint32 deagg_badsfalign;     /**< MPDU of one A-MSDU is not aligned to 4 byte boundary */
	uint32 deagg_badtotlen;      /**< A-MSDU tot length doesn't match summation of all sfs */
	uint32 deagg_openfail;       /**< A-MSDU deagg open failures */
	uint32 deagg_swdeagglong;    /**< A-MSDU sw_deagg doesn't handle long pkt */
	uint32 deagg_flush;          /**< A-MSDU deagg flush; deagg errors may result in this */
	uint32 tx_padding_in_tail;   /**< 4Byte pad was placed in tail of packet */
	uint32 tx_padding_in_head;   /**< 4Byte pad was placed in head of packet */
	uint32 tx_padding_no_pad;    /**< 4Byte pad was not needed (4B aligned or last in agg) */
	uint32 agg_amsdu_bytes_l;    /**< num of total msdu bytes successfully transmitted */
	uint32 agg_amsdu_bytes_h;
	uint32 deagg_amsdu_bytes_l;  /**< AMSDU bytes deagg successfully */
	uint32 deagg_amsdu_bytes_h;
} wlc_amsdu_cnt_t;
#endif	/* WLCNT */

typedef struct {
	/* tx counters */
	uint32 tx_msdu_histogram[MAX_TX_SUBFRAMES_LIMIT]; /**< mpdus per amsdu histogram */
	uint32 tx_length_histogram[AMSDU_LENGTH_BINS]; /**< amsdu length histogram */
	/* rx counters */
	uint32 rx_msdu_histogram[AMSDU_RX_SUBFRAMES_BINS]; /**< mpdu per amsdu rx */
	uint32 rx_length_histogram[AMSDU_LENGTH_BINS]; /**< amsdu rx length */
} amsdu_dbg_t;

/** iovar table */
enum {
	IOV_AMSDU_SIM,

	IOV_AMSDU_AGGSF,      /**< num of subframes in one A-MSDU for all tids */
	IOV_AMSDU_AGGBYTES,   /**< num of bytes in one A-MSDU for all tids */

	IOV_AMSDU_BLOCK,      /**< block amsdu agg */
	IOV_AMSDU_DEAGGDUMP,  /**< dump deagg pkt */
	IOV_AMSDU_COUNTERS,   /**< dump A-MSDU counters */
	IOV_AMSDUNOACK,
	IOV_AMSDU,            /**< Enable/disable A-MSDU, GET returns current state */
	IOV_RX_AMSDU_IN_AMPDU,
	IOV_VAP_AMSDU,
	IOV_VAP_AMSDU_TX_ENABLE,
	IOV_VAP_AMSDU_RX_MAX,
	IOV_VAP_RX_AMSDU_IN_AMPDU,
	IOV_VAP_RX_AMSDU_HWDAGG_DIS,
	IOV_SPP_AMSDU_CAP,
	IOV_AMSDU_TID,              /* enable/disable per-tid amsdu */
	IOV_AMSDU_MAX_MSDU_NUM     /**< max # of MSDUs in an A-MSDU, DOT11_EXT_CAP_NUM_MSDU */
};

/* Policy of allowed AMSDU priority items */
static const bool amsdu_scb_txpolicy[NUMPRIO] = {
	TRUE,  /* 0 BE - Best-effortBest-effort */
	FALSE, /* 1 BK - Background */
	FALSE, /* 2 None = - */
	FALSE, /* 3 EE - Excellent-effort */
	FALSE, /* 4 CL - Controlled Load */
	FALSE, /* 5 VI - Video */
	FALSE, /* 6 VO - Voice */
	FALSE, /* 7 NC - Network Control */
};

static const bcm_iovar_t amsdu_iovars[] = {
	{"amsdu", IOV_AMSDU, (IOVF_SET_DOWN), 0, IOVT_BOOL, 0},
	{"rx_amsdu_in_ampdu", IOV_RX_AMSDU_IN_AMPDU, (0), 0, IOVT_BOOL, 0},
	{"amsdu_noack", IOV_AMSDUNOACK, (0), 0, IOVT_BOOL, 0},
	{"amsdu_aggsf", IOV_AMSDU_AGGSF, (0), 0, IOVT_UINT16, 0},
	{"amsdu_aggbytes", IOV_AMSDU_AGGBYTES, (0), 0, IOVT_UINT32, 0},
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	{"amsdu_aggblock", IOV_AMSDU_BLOCK, (0), 0, IOVT_BOOL, 0},
#ifdef WLCNT
	{"amsdu_counters", IOV_AMSDU_COUNTERS, (0), 0, IOVT_BUFFER, sizeof(wlc_amsdu_cnt_t)},
#endif /* WLCNT */
#endif /* BCMDBG */
	{"amsdu_tid", IOV_AMSDU_TID, (0), 0, IOVT_BUFFER, sizeof(struct amsdu_tid_control)},
#ifdef WL_SPP_AMSDU
	{"spp_amsdu_cap", IOV_SPP_AMSDU_CAP, (0), 0, IOVT_UINT8, 0},
#endif /* WL_SPP_AMSDU */
	{"amsdu_max_msdu_num", IOV_AMSDU_MAX_MSDU_NUM, (IOVF_SET_DOWN), 0, IOVT_UINT8, 0},
	{NULL, 0, 0, 0, 0, 0}
};

typedef struct amsdu_deagg {
	int amsdu_deagg_state;     /**< A-MSDU deagg statemachine per device */
	void *amsdu_deagg_p;       /**< pointer to first pkt buffer in A-MSDU chain */
	void *amsdu_deagg_ptail;   /**< pointer to last pkt buffer in A-MSDU chain */
	uint16  first_pad;         /**< front padding bytes of A-MSDU first sub frame */
	bool chainable;
} amsdu_deagg_t;

/** default/global settings for A-MSDU module */
typedef struct amsdu_ami_policy {
	bool amsdu_agg_enable[NUMPRIO];		/**< TRUE:agg allowed, FALSE:agg disallowed */
	uint amsdu_max_agg_bytes[NUMPRIO];	/**< Maximum allowed payload bytes per A-MSDU */
	uint8 amsdu_max_sframes;		/**< Maximum allowed subframes per A-MSDU */
} amsdu_ami_policy_t;

/** principal amsdu module local structure per device instance */
struct amsdu_info {
	wlc_info_t *wlc;             /**< pointer to main wlc structure */
	wlc_pub_t *pub;              /**< public common code handler */
	int scb_handle;              /**< scb cubby handle to retrieve data from scb */

	/* RX datapath bits */
	uint mac_rcvfifo_limit;    /**< max rx fifo in bytes */
	uint amsdu_rx_mtu;         /**< amsdu MTU, depend on rx fifo limit */
	bool amsdu_rxcap_big;        /**< TRUE: rx big amsdu capable (HT_MAX_AMSDU) */
	/* rx: streams per device */
	amsdu_deagg_t *amsdu_deagg;  /**< A-MSDU deagg */

	/* TX datapath bits */
	bool amsdu_agg_block;        /**< global override: disable amsdu tx */
	amsdu_ami_policy_t txpolicy; /**< Default ami release policy per prio */

	bool amsdu_deagg_pkt;        /**< dump deagg pkt BCMINTERNAL */
	wlc_amsdu_cnt_t *cnt;        /**< counters/stats WLCNT */
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	amsdu_dbg_t *amdbg;
#endif
};

/* Per-prio SCB A-MSDU policy variables */
typedef struct {
	/*
	 * Below are the private limits as negotiated during association
	 */
	/** max (V)HT AMSDU bytes (negotiated) that remote node can receive */
	uint amsdu_agg_bytes_max;
	bool amsdu_agg_enable;		/**< TRUE:agg allowed, FALSE:agg disallowed */
} amsdu_scb_txpolicy_t;

/** per scb cubby info */
typedef struct scb_amsduinfo {
	/*
	 * This contains the default attach time values as well as any
	 * protocol dependent limits.
	 */
	amsdu_scb_txpolicy_t scb_txpolicy[NUMPRIO];
	uint16 mtu_pref;
	/** Maximum allowed number of subframes per A-MSDU that remote node can receive */
	uint8 amsdu_aggsf;
	uint8 amsdu_max_sframes;	/**< Maximum allowed subframes per A-MSDU */
	/** set when ratesel mem was updated, max tx amsdu agg len then has to be (re)determined */
	bool agglimit_upd;
} scb_amsdu_t;

#define SCB_AMSDU_CUBBY(ami, scb) (scb_amsdu_t *)SCB_CUBBY((scb), (ami)->scb_handle)

/* A-MSDU general */
static int wlc_amsdu_doiovar(void *hdl, uint32 actionid,
        void *p, uint plen, void *arg, uint alen, uint val_size, struct wlc_if *wlcif);

static void wlc_amsdu_mtu_init(amsdu_info_t *ami);
static int wlc_amsdu_down(void *hdl);
static int wlc_amsdu_up(void *hdl);

#ifdef WLAMSDU_TX
static uint32 wlc_amsdu_tx_scb_max_agg_len_upd(amsdu_info_t *ami, struct scb *scb);
static void wlc_amsdu_tx_scb_agglimit_upd(amsdu_info_t *ami, struct scb *scb);
#ifndef WLAMSDU_TX_DISABLED
static int wlc_amsdu_tx_scb_init(void *cubby, struct scb *scb);
static void wlc_amsdu_tx_scb_state_upd(void *ctx, scb_state_upd_data_t *notif_data);
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
static void wlc_amsdu_tx_dump_scb(void *ctx, struct scb *scb, struct bcmstrbuf *b);
#else
#define wlc_amsdu_tx_dump_scb NULL
#endif

#endif /* !WLAMSDU_TX_DISABLED */
#endif /* WLAMSDU_TX */

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
static int wlc_amsdu_dump(void *ctx, struct bcmstrbuf *b);
#endif

/* A-MSDU deaggregation */
static bool wlc_amsdu_deagg_open(amsdu_info_t *ami, int fifo, void *p,
	struct dot11_header *h, uint32 pktlen);
static bool wlc_amsdu_deagg_verify(amsdu_info_t *ami, uint16 fc, void *h);
static void wlc_amsdu_deagg_flush(amsdu_info_t *ami, int fifo);
static int wlc_amsdu_tx_attach(amsdu_info_t *ami, wlc_info_t *wlc);

#if (defined(BCMDBG) || defined(BCMDBG_AMSDU)) && defined(WLCNT)
void wlc_amsdu_dump_cnt(amsdu_info_t *ami, struct bcmstrbuf *b);
#endif	/* defined(BCMDBG) && defined(WLCNT) */

#ifdef WL11K_ALL_MEAS
void wlc_amsdu_get_stats(wlc_info_t *wlc, rrm_stat_group_11_t *g11)
{
	ASSERT(wlc);
	ASSERT(g11);

	g11->txamsdu = wlc->ami->cnt->agg_amsdu;
	g11->amsdufail = wlc->ami->cnt->deagg_openfail;
	g11->amsduretry = 0; /* Not supported */
	g11->amsduretries = 0; /* Not supported */
	g11->txamsdubyte_h = wlc->ami->cnt->agg_amsdu_bytes_h;
	g11->txamsdubyte_l = wlc->ami->cnt->agg_amsdu_bytes_l;
	g11->amsduackfail = wlc->_amsdu_noack;
	g11->rxamsdu = wlc->ami->cnt->deagg_amsdu;
	g11->rxamsdubyte_h = wlc->ami->cnt->deagg_amsdu_bytes_h;
	g11->rxamsdubyte_l = wlc->ami->cnt->deagg_amsdu_bytes_l;
}
#endif /* WL11K_ALL_MEAS */

#ifdef WLAMSDU_TX

static void
wlc_amsdu_set_scb_default_txpolicy(amsdu_info_t *ami, scb_amsdu_t *scb_ami)
{
	uint i;

	scb_ami->amsdu_max_sframes = ami->txpolicy.amsdu_max_sframes;

	for (i = 0; i < NUMPRIO; i++) {
		amsdu_scb_txpolicy_t *scb_txpolicy = &scb_ami->scb_txpolicy[i];

		scb_txpolicy->amsdu_agg_enable = ami->txpolicy.amsdu_agg_enable[i];
	}
}

/** called by IOV */
static void
wlc_amsdu_set_scb_default_txpolicy_all(amsdu_info_t *ami)
{
	struct scb *scb;
	struct scb_iter scbiter;

	FOREACHSCB(ami->wlc->scbstate, &scbiter, scb) {
		wlc_amsdu_set_scb_default_txpolicy(ami, SCB_AMSDU_CUBBY(ami, scb));
		wlc_amsdu_tx_scb_aggsf_upd(ami, scb);
	}

	/* Update frag threshold on A-MSDU parameter changes */
	wlc_amsdu_agglimit_frag_upd(ami);
}

#endif /* WLAMSDU_TX */

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
#ifdef WLCNT
static int
wlc_amsdu_dump_clr(void *ctx)
{
	amsdu_info_t *ami = ctx;

	bzero(ami->cnt, sizeof(*ami->cnt));
	bzero(ami->amdbg, sizeof(*ami->amdbg));

	return BCME_OK;
}
#else
#define wlc_amsdu_dump_clr NULL
#endif
#endif

/*
 * This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

/** attach function for receive AMSDU support */
amsdu_info_t *
BCMATTACHFN(wlc_amsdu_attach)(wlc_info_t *wlc)
{
	amsdu_info_t *ami;
	if (!(ami = (amsdu_info_t *)MALLOCZ(wlc->osh, sizeof(amsdu_info_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	ami->wlc = wlc;
	ami->pub = wlc->pub;

	if (!(ami->amsdu_deagg = (amsdu_deagg_t *)MALLOCZ(wlc->osh,
		sizeof(amsdu_deagg_t) * RX_FIFO_NUMBER))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

#ifdef WLCNT
	if (!(ami->cnt = (wlc_amsdu_cnt_t *)MALLOCZ(wlc->osh, sizeof(wlc_amsdu_cnt_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
#endif /* WLCNT */

	/* register module */
	if (wlc_module_register(ami->pub, amsdu_iovars, "amsdu", ami, wlc_amsdu_doiovar,
		NULL, wlc_amsdu_up, wlc_amsdu_down)) {
		WL_ERROR(("wl%d: %s: wlc_module_register failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	if (!(ami->amdbg = (amsdu_dbg_t *)MALLOCZ(wlc->osh, sizeof(amsdu_dbg_t)))) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	wlc_dump_add_fns(ami->pub, "amsdu", wlc_amsdu_dump, wlc_amsdu_dump_clr, ami);
#endif

	if (wlc_amsdu_tx_attach(ami, wlc) < 0) {
		WL_ERROR(("wl%d: %s: Error initing the amsdu tx\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	wlc_amsdu_mtu_init(ami);

	/* to be compatible with spec limit */
	if (wlc->pub->tunables->nrxd < MAX_RX_SUBFRAMES) {
		WL_ERROR(("NRXD %d is too small to fit max amsdu rxframe\n",
		          (uint)wlc->pub->tunables->nrxd));
	}

	/* Set to 3 for 8 MSDU in AMSDU maximum. 802.11-2016, 9.4.2.27, bit63-64 */
	wlc->pub->_amsdu_max_msdu_num = 3;
	return ami;
fail:
	MODULE_DETACH(ami, wlc_amsdu_detach);
	return NULL;
} /* wlc_amsdu_attach */

/** attach function for transmit AMSDU support */
static int
BCMATTACHFN(wlc_amsdu_tx_attach)(amsdu_info_t *ami, wlc_info_t *wlc)
{
#ifdef WLAMSDU_TX
#ifndef WLAMSDU_TX_DISABLED
	uint i;
	uint max_agg;
	scb_cubby_params_t cubby_params;
	int err;

	if (WLCISACPHY(wlc->band)) {
		if (AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB(wlc))
			max_agg = HT_MAX_AMSDU;
		else
			max_agg = VHT_MAX_AMSDU;
	} else {
		max_agg = HT_MAX_AMSDU;
	}

	/* reserve cubby in the scb container for per-scb private data */
	bzero(&cubby_params, sizeof(cubby_params));

	cubby_params.context = ami;
	cubby_params.fn_init = wlc_amsdu_tx_scb_init;
	cubby_params.fn_deinit = NULL;
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	cubby_params.fn_dump = wlc_amsdu_tx_dump_scb;
#endif
	ami->scb_handle = wlc_scb_cubby_reserve_ext(wlc, sizeof(scb_amsdu_t), &cubby_params);

	if (ami->scb_handle < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve failed\n",
		          wlc->pub->unit, __FUNCTION__));
		return -1;
	}

	/* Add client callback to the scb state notification list */
	if ((err = wlc_scb_state_upd_register(wlc, wlc_amsdu_tx_scb_state_upd, ami)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: unable to register callback %p\n",
		          wlc->pub->unit, __FUNCTION__, wlc_amsdu_tx_scb_state_upd));

		return -1;
	}

	WLCNTSET(ami->cnt->version, WLC_AMSDU_CNT_VERSION);
	WLCNTSET(ami->cnt->length, sizeof(*(ami->cnt)));

	/* init tunables */

	if (EMBEDDED_2x2AX_CORE(wlc->pub->sih->chip)) {
		ami->txpolicy.amsdu_max_sframes = MAX_TX_SUBFRAMES_AXPHY;
	} else if (WLCISACPHY(wlc->band)) {
		ami->txpolicy.amsdu_max_sframes = MAX_TX_SUBFRAMES_ACPHY;
	} else {
		ami->txpolicy.amsdu_max_sframes = MAX_TX_SUBFRAMES;
	}

	/* DMA: leave empty room for DMA descriptor table */
	if (ami->txpolicy.amsdu_max_sframes >
		(uint)(wlc->pub->tunables->ntxd/3)) {
		WL_ERROR(("NTXD %d is too small to fit max amsdu txframe\n",
			(uint)wlc->pub->tunables->ntxd));
		ASSERT(0);
	}

#if defined(BCMHWA)
	HWA_PKTPGR_EXPR({
		if ((hwa_pktpgr_multi_txlfrag(hwa_dev) == 0) &&
			(ami->txpolicy.amsdu_max_sframes > 4)) {
			WL_ERROR(("%s: Cannot support more then %d subframes in one A-MSDU\n",
				__FUNCTION__, ami->txpolicy.amsdu_max_sframes));
			ami->txpolicy.amsdu_max_sframes = 4;
		}
	});
#endif

	for (i = 0; i < NUMPRIO; i++) {
		uint fifo_size;

		ami->txpolicy.amsdu_agg_enable[i] = amsdu_scb_txpolicy[i];

		/* set agg_bytes_limit to standard maximum if hw fifo allows
		 *  this value can be changed via iovar or fragthreshold later
		 *  it can never exceed hw fifo limit since A-MSDU is not streaming
		 */
		fifo_size = wlc->xmtfifo_szh[prio2fifo[i]];
		/* blocks to bytes */
		fifo_size = fifo_size * TXFIFO_SIZE_UNIT(wlc->pub->corerev);
		ami->txpolicy.amsdu_max_agg_bytes[i] = MIN(max_agg, fifo_size);
	}

	wlc->pub->_amsdu_tx_support = TRUE;
#endif /* !WLAMSDU_TX_DISABLED */
#endif /* WLAMSDU_TX */

	return 0;
} /* wlc_amsdu_tx_attach */

void
BCMATTACHFN(wlc_amsdu_detach)(amsdu_info_t *ami)
{
	if (!ami)
		return;

	wlc_amsdu_down(ami);

	wlc_module_unregister(ami->pub, "amsdu", ami);

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	if (ami->amdbg) {
		MFREE(ami->pub->osh, ami->amdbg, sizeof(amsdu_dbg_t));
		ami->amdbg = NULL;
	}
#endif

#ifdef WLCNT
	if (ami->cnt)
		MFREE(ami->pub->osh, ami->cnt, sizeof(wlc_amsdu_cnt_t));
#endif /* WLCNT */
	MFREE(ami->pub->osh, ami->amsdu_deagg, sizeof(amsdu_deagg_t) * RX_FIFO_NUMBER);
	MFREE(ami->pub->osh, ami, sizeof(amsdu_info_t));
}

#ifndef DONGLEBUILD
static uint
wlc_rcvfifo_limit_get(wlc_info_t *wlc)
{
	uint rcvfifo;

	/* determine rx fifo. no register/shm, it's hardwired in RTL */

	if (D11REV_GE(wlc->pub->corerev, 40)) {
		rcvfifo = wlc_bmac_rxfifosz_get(wlc->hw);
	} else if (D11REV_GE(wlc->pub->corerev, 22)) {
		/* XXX it should have been D11REV_GE(wlc->pub->corerev, 16) but all chips
		 * with revid older than 22 are EOL'd so start from revid 22.
		 */
		rcvfifo = ((wlc->machwcap & MCAP_RXFSZ_MASK) >> MCAP_RXFSZ_SHIFT) * 512;
	} else {
		/* EOL'd chips or chips that don't exist yet */
		ASSERT(0);
		return 0;
	}
	return rcvfifo;
} /* wlc_rcvfifo_limit_get */
#endif /* !DONGLEBUILD */

static void
BCMATTACHFN(wlc_amsdu_mtu_init)(amsdu_info_t *ami)
{
	wlc_info_t *wlc = ami->wlc;
#ifdef DONGLEBUILD
	ami->amsdu_rxcap_big = FALSE;
#else /* DONGLEBUILD */
	ami->mac_rcvfifo_limit = wlc_rcvfifo_limit_get(wlc);
	if (D11REV_GE(wlc->pub->corerev, 31) && D11REV_LE(wlc->pub->corerev, 38))
		ami->amsdu_rxcap_big = FALSE;
	else
		ami->amsdu_rxcap_big =
		        ((int)(ami->mac_rcvfifo_limit - wlc->hwrxoff - 100) >= HT_MAX_AMSDU);
#endif /* DONGLEBUILD */

	ami->amsdu_rx_mtu = ami->amsdu_rxcap_big ? HT_MAX_AMSDU : HT_MIN_AMSDU;

	/* For A/C enabled chips only */
	if (WLCISACPHY(wlc->band) &&
	    ami->amsdu_rxcap_big &&
	    ((int)(ami->mac_rcvfifo_limit - wlc->hwrxoff - 100) >= VHT_MAX_AMSDU)) {
		ami->amsdu_rx_mtu = VHT_MAX_AMSDU;
	}
	WL_AMSDU(("%s:ami->amsdu_rx_mtu=%d\n", __FUNCTION__, ami->amsdu_rx_mtu));
}

bool
wlc_amsdu_is_rxmax_valid(amsdu_info_t *ami)
{
	if (wlc_amsdu_mtu_get(ami) < HT_MAX_AMSDU) {
		return TRUE;
	} else {
		return FALSE;
	}
}

uint
wlc_amsdu_mtu_get(amsdu_info_t *ami)
{
	return ami->amsdu_rx_mtu;
}

/** Returns TRUE or FALSE. AMSDU tx is optional, sw can turn it on or off even HW supports */
bool
wlc_amsdu_tx_cap(amsdu_info_t *ami)
{
#if defined(WLAMSDU_TX)
	if (AMSDU_TX_SUPPORT(ami->pub))
		return (TRUE);
#endif
	return (FALSE);
}

/** Returns TRUE or FALSE. AMSDU rx is mandatory for NPHY */
bool
wlc_amsdurx_cap(amsdu_info_t *ami)
{
	return (TRUE);
}

#ifdef WLAMSDU_TX

/** WLAMSDU_TX specific function to enable/disable AMSDU transmit */
int
wlc_amsdu_set(amsdu_info_t *ami, bool on)
{
	wlc_info_t *wlc = ami->wlc;

	WL_AMSDU(("wlc_amsdu_set val=%d\n", on));

	if (on) {
		if (!N_ENAB(wlc->pub)) {
			WL_AMSDU(("wl%d: driver not nmode enabled\n", wlc->pub->unit));
			return BCME_UNSUPPORTED;
		}
		if (!wlc_amsdu_tx_cap(ami)) {
			WL_AMSDU(("wl%d: device not amsdu capable\n", wlc->pub->unit));
			return BCME_UNSUPPORTED;
		} else if (AMPDU_ENAB(wlc->pub) &&
		           D11REV_LT(wlc->pub->corerev, 40)) {
			/* AMSDU + AMPDU ok for core-rev 40+ with AQM */
			WL_AMSDU(("wl%d: A-MSDU not supported with AMPDU on d11 rev %d\n",
			          wlc->pub->unit, wlc->pub->corerev));
			return BCME_UNSUPPORTED;
		}
	}

	/* This controls AMSDU agg only, AMSDU deagg is on by default per spec */
	wlc->pub->_amsdu_tx = on;
	wlc_update_brcm_ie(wlc);

	/* tx descriptors should be higher -- AMPDU max when both AMSDU and AMPDU set */
	wlc_set_default_txmaxpkts(wlc);

	return (0);
} /* wlc_amsdu_set */

#ifndef WLAMSDU_TX_DISABLED

/** WLAMSDU_TX specific function, initializes each priority for a remote party (scb) */
static int
wlc_amsdu_tx_scb_init(void *context, struct scb *scb)
{
	uint i;
	amsdu_info_t *ami = (amsdu_info_t *)context;
	scb_amsdu_t *scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	amsdu_scb_txpolicy_t *scb_txpolicy;

	WL_AMSDU_EX(scb, ("scb %p\n", OSL_OBFUSCATE_BUF(scb)));

	ASSERT(scb_ami);

	/* Setup A-MSDU SCB policy defaults */
	for (i = 0; i < NUMPRIO; i++) {
		scb_txpolicy = &scb_ami->scb_txpolicy[i];

		memset(scb_txpolicy, 0, sizeof(*scb_txpolicy));

		/*
		 * These are negotiated defaults, this is not a public block
		 * The public info is rebuilt each time an iovar is used which
		 * would destroy the negotiated values here, so this is made private.
		 */
		scb_txpolicy->amsdu_agg_bytes_max = HT_MAX_AMSDU;
	}
	scb_ami->amsdu_aggsf = ami->txpolicy.amsdu_max_sframes;

	/* Init the public part with the global ami defaults */
	wlc_amsdu_set_scb_default_txpolicy(ami, scb_ami);

	return 0;
}

/* Callback function invoked when a STA's association state changes.
 * Inputs:
 *   ctx -        amsdu_info_t structure
 *   notif_data - information describing the state change
 */
static void
wlc_amsdu_tx_scb_state_upd(void *ctx, scb_state_upd_data_t *notif_data)
{
	amsdu_info_t *ami = (amsdu_info_t*) ctx;
	scb_t *scb;

	ASSERT(notif_data != NULL);

	scb = notif_data->scb;
	ASSERT(scb != NULL);

	if (SCB_ASSOCIATED(scb)) {
		wlc_amsdu_tx_scb_aggsf_upd(ami, scb);
	}
}
#endif /* !WLAMSDU_TX_DISABLED */
#endif /* WLAMSDU_TX */

/** handle AMSDU related items when going down */
static int
wlc_amsdu_down(void *hdl)
{
	int fifo;
	amsdu_info_t *ami = (amsdu_info_t *)hdl;

	WL_AMSDU(("wlc_amsdu_down: entered\n"));

	/* Flush the deagg Q, there may be packets there */
	for (fifo = 0; fifo < RX_FIFO_NUMBER; fifo++)
		wlc_amsdu_deagg_flush(ami, fifo);

	return 0;
}

static int
wlc_amsdu_up(void *hdl)
{
	/* limit max size pkt ucode lets through to what we use for dma rx descriptors */
	/* else rx of amsdu can cause dma rx errors and potentially impact performance */
	amsdu_info_t *ami = (amsdu_info_t *)hdl;
	wlc_info_t *wlc = ami->wlc;
	hnddma_t *di;
	uint16 rxbufsz;
	uint16 rxoffset;

	di = WLC_HW_DI(wlc, 0);
	dma_rxparam_get(di, &rxoffset, &rxbufsz);
	rxbufsz =  rxbufsz - rxoffset;

	wlc_write_shm(wlc, M_MAXRXFRM_LEN(wlc), (uint16)rxbufsz);
	if (D11REV_GE(wlc->pub->corerev, 65)) {
		W_REG(wlc->osh, D11_DAGG_LEN_THR(wlc), (uint16)rxbufsz);
	}

	return BCME_OK;
}

/** handle AMSDU related iovars */
static int
wlc_amsdu_doiovar(void *hdl, uint32 actionid,
	void *p, uint plen, void *a, uint alen, uint val_size, struct wlc_if *wlcif)
{
	amsdu_info_t *ami = (amsdu_info_t *)hdl;
	int32 int_val = 0;
	int32 *pval = (int32 *)a;
#if defined(WL_SPP_AMSDU)
	wlc_bsscfg_t *bsscfg;
	int32 result = 0;
#endif
	bool bool_val;
	int err = 0;
	wlc_info_t *wlc;
#ifdef WLAMSDU_TX
	uint i; /* BCM_REFERENCE() may be more convienient but we save
		 * 1 stack var if the feature is not used.
		*/
#endif

	BCM_REFERENCE(wlcif);
	BCM_REFERENCE(alen);

	if (plen >= (int)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	bool_val = (int_val != 0) ? TRUE : FALSE;
	wlc = ami->wlc;
	ASSERT(ami == wlc->ami);
#if defined(WL_SPP_AMSDU)
	bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
	ASSERT(bsscfg != NULL);
#endif

	switch (actionid) {
	case IOV_GVAL(IOV_AMSDU):
		int_val = wlc->pub->_amsdu_tx;
		bcopy(&int_val, a, val_size);
		break;

#ifdef WLAMSDU_TX
	case IOV_SVAL(IOV_AMSDU):
		if (AMSDU_TX_SUPPORT(wlc->pub))
			err = wlc_amsdu_set(ami, bool_val);
		else
			err = BCME_UNSUPPORTED;
		break;
#endif

	case IOV_GVAL(IOV_AMSDUNOACK):
		*pval = (int32)wlc->_amsdu_noack;
		break;

	case IOV_SVAL(IOV_AMSDUNOACK):
		wlc->_amsdu_noack = bool_val;
		break;

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	case IOV_GVAL(IOV_AMSDU_BLOCK):
		*pval = (int32)ami->amsdu_agg_block;
		break;

	case IOV_SVAL(IOV_AMSDU_BLOCK):
		ami->amsdu_agg_block = bool_val;
		break;

#ifdef WLCNT
	case IOV_GVAL(IOV_AMSDU_COUNTERS):
		bcopy(&ami->cnt, a, sizeof(ami->cnt));
		break;
#endif /* WLCNT */
#endif /* BCMDBG */

#ifdef WLAMSDU_TX
	case IOV_GVAL(IOV_AMSDU_AGGBYTES):
		if (AMSDU_TX_SUPPORT(wlc->pub)) {
			/* TODO, support all priorities ? */
			*pval = (int32)ami->txpolicy.amsdu_max_agg_bytes[PRIO_8021D_BE];
		} else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_AMSDU_AGGBYTES):
		if (AMSDU_TX_SUPPORT(wlc->pub)) {
			struct scb *scb;
			struct scb_iter scbiter;
			uint32 uint_val = (uint)int_val;

			if (WLCISACPHY(wlc->band) && uint_val > VHT_MAX_AMSDU) {
				err = BCME_RANGE;
				break;
			}
			if (!WLCISACPHY(wlc->band) && (uint_val > (uint)HT_MAX_AMSDU)) {
				err = BCME_RANGE;
				break;
			}

			if (uint_val < AMSDU_AGGBYTES_MIN) {
				err = BCME_RANGE;
				break;
			}

			for (i = 0; i < NUMPRIO; i++) {
				uint fifo_size;

				fifo_size = wlc->xmtfifo_szh[prio2fifo[i]];
				/* blocks to bytes */
				fifo_size = fifo_size * TXFIFO_SIZE_UNIT(wlc->pub->corerev);
				ami->txpolicy.amsdu_max_agg_bytes[i] =
						MIN(uint_val, fifo_size);
			}
			/* update amsdu agg bytes for ALL scbs */
			FOREACHSCB(wlc->scbstate, &scbiter, scb) {
				wlc_amsdu_tx_scb_agglimit_upd(ami, scb);
			}
		} else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_GVAL(IOV_AMSDU_AGGSF):
		if (AMSDU_TX_SUPPORT(wlc->pub)) {
			/* TODO, support all priorities ? */
			*pval = (int32)ami->txpolicy.amsdu_max_sframes;
		} else {
			err = BCME_UNSUPPORTED;
		}
		break;

	case IOV_SVAL(IOV_AMSDU_AGGSF):
		if (AMSDU_TX_SUPPORT(wlc->pub)) {

			if ((int_val > MAX_TX_SUBFRAMES_LIMIT) ||
			    (int_val > wlc->pub->tunables->ntxd/2) ||
			    (int_val < 1)) {
				err = BCME_RANGE;
				break;
			}

#if defined(BCMHWA)
			HWA_PKTPGR_EXPR({
				if ((hwa_pktpgr_multi_txlfrag(hwa_dev) == 0) && (int_val > 4)) {
					WL_ERROR(("%s: Cannot support more then %d subframes "
						"in one A-MSDU\n", __FUNCTION__, int_val));
					err = BCME_RANGE;
					break;
				}
			});
#endif
			ami->txpolicy.amsdu_max_sframes = (uint8)int_val;

			/* Update the scbs */
			wlc_amsdu_set_scb_default_txpolicy_all(ami);
		} else
			err = BCME_UNSUPPORTED;
		break;
#endif /* WLAMSDU_TX */

#ifdef WLAMSDU

	case IOV_GVAL(IOV_RX_AMSDU_IN_AMPDU):
		int_val = (int8)(wlc->_rx_amsdu_in_ampdu);
		bcopy(&int_val, a, val_size);
		break;

	case IOV_SVAL(IOV_RX_AMSDU_IN_AMPDU):
		if (bool_val && D11REV_LT(wlc->pub->corerev, 40)) {
			WL_AMSDU(("wl%d: Not supported < corerev (40)\n", wlc->pub->unit));
			err = BCME_UNSUPPORTED;
		} else {
			wlc->_rx_amsdu_in_ampdu = bool_val;
		}

		break;

	case IOV_GVAL(IOV_AMSDU_TID):
		{
			struct amsdu_tid_control *amsdu_tid = (struct amsdu_tid_control *)p;

			if (amsdu_tid->tid >= NUMPRIO) {
				err = BCME_BADARG;
				break;
			}
			amsdu_tid->enable = ami->txpolicy.amsdu_agg_enable[amsdu_tid->tid];
			bcopy(amsdu_tid, a, sizeof(*amsdu_tid));
		}
		break;
	case IOV_SVAL(IOV_AMSDU_TID):
		{
			struct amsdu_tid_control *amsdu_tid = (struct amsdu_tid_control *)a;
			if (amsdu_tid->tid >= NUMPRIO) {
				err = BCME_BADARG;
				break;
			}
			ami->txpolicy.amsdu_agg_enable[amsdu_tid->tid] = amsdu_tid->enable
				? TRUE : FALSE;
		}
		break;

	case IOV_GVAL(IOV_AMSDU_MAX_MSDU_NUM):
		*pval = (int32)wlc->pub->_amsdu_max_msdu_num;
		break;
	case IOV_SVAL(IOV_AMSDU_MAX_MSDU_NUM):
		if ((int_val > 3) || (int_val < 0)) {
			err = BCME_RANGE;
			break;
		}

		wlc->pub->_amsdu_max_msdu_num = int_val;
		break;
#endif /* WLAMSDU */
#ifdef WL_SPP_AMSDU
	case IOV_SVAL(IOV_SPP_AMSDU_CAP):

		if (D11REV_LT(wlc->pub->corerev, 129)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		bsscfg->flags2 &= ~(WLC_BSSCFG_FL2_RSN_CAP_SPPC |
			WLC_BSSCFG_FL2_RSN_CAP_SPPR);

		if (int_val) {
			if (int_val & SPP_AMSDU_CAPABLE) {
				bsscfg->flags2 |= WLC_BSSCFG_FL2_RSN_CAP_SPPC;
			}
			if (int_val & SPP_AMSDU_REQUIRED) {
				bsscfg->flags2 |= WLC_BSSCFG_FL2_RSN_CAP_SPPR;
			}
		}
		break;
	case IOV_GVAL(IOV_SPP_AMSDU_CAP):

		if (BSSCFG_IS_RSN_CAP_SPPC(bsscfg) || BSSCFG_IS_RSN_CAP_SPPR(bsscfg)) {
			if (BSSCFG_IS_RSN_CAP_SPPC(bsscfg)) {
				result |= SPP_AMSDU_CAPABLE;
			}
			if (BSSCFG_IS_RSN_CAP_SPPR(bsscfg)) {
				result |= SPP_AMSDU_REQUIRED;
			}
		} else {
			result = SPP_AMSDU_DISABLED;
		}

		if (pval != NULL)
			*pval = result;
		break;
#endif /* WL_SPP_AMSDU */

	default:
		err = BCME_UNSUPPORTED;
	}

	return err;
} /* wlc_amsdu_doiovar */

#ifdef WLAMSDU_TX
/**
 * Helper function that updates the per-SCB A-MSDU state.
 *
 * @param ami			A-MSDU module handle.
 * @param scb			scb pointer.
 * @param amsdu_agg_enable[]	array of enable states, one for each priority.
 *
 */
static void
wlc_amsdu_tx_scb_agglimit_upd_all(amsdu_info_t *ami, struct scb *scb, bool amsdu_agg_enable[])
{
	uint i;
	amsdu_scb_txpolicy_t *scb_txpolicy = &(SCB_AMSDU_CUBBY(ami, scb))->scb_txpolicy[0];

	for (i = 0; i < NUMPRIO; i++) {
		scb_txpolicy[i].amsdu_agg_enable = amsdu_agg_enable[i];
	}

	wlc_amsdu_tx_scb_agglimit_upd(ami, scb);
}

/**
 * WLAMSDU_TX specific function.
 * called from fragthresh changes ONLY: update agg bytes limit, toss buffered A-MSDU
 * This is expected to happen very rarely since user should use very standard 802.11 fragthreshold
 *  to "disabled" fragmentation when enable A-MSDU. We can even ignore that. But to be
 *  full spec compliant, we reserve this capability.
 *   ??? how to inform user the requirement that not changing FRAGTHRESHOLD to screw up A-MSDU
 */
void
wlc_amsdu_agglimit_frag_upd(amsdu_info_t *ami)
{
	uint i;
	wlc_info_t *wlc = ami->wlc;
	struct scb *scb;
	struct scb_iter scbiter;
	bool frag_disabled = FALSE;
	bool amsdu_agg_enable[NUMPRIO];
	uint16 fragthresh;

	WL_AMSDU(("wlc_amsdu_agg_limit_upd\n"));

	if (!AMSDU_TX_SUPPORT(wlc->pub))
		return;

	for (i = 0; i < NUMPRIO; i++) {
		fragthresh = wlc->fragthresh[WME_PRIO2AC(i)];
		frag_disabled = (fragthresh == DOT11_MAX_FRAG_LEN);

		if (!frag_disabled && (fragthresh < ami->txpolicy.amsdu_max_agg_bytes[i])) {
			ami->txpolicy.amsdu_max_agg_bytes[i] = fragthresh;

			WL_AMSDU(("wlc_amsdu_agg_frag_upd: amsdu_aggbytes[%d] = %d due to frag!\n",
				i, ami->txpolicy.amsdu_max_agg_bytes[i]));
		} else if (frag_disabled || (fragthresh > ami->txpolicy.amsdu_max_agg_bytes[i])) {
			uint max_agg;
			uint fifo_size;
			if (WLCISACPHY(wlc->band)) {
				if (AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB(wlc))
					max_agg = HT_MAX_AMSDU;
				else
					max_agg = VHT_MAX_AMSDU;
			} else {
				max_agg = HT_MAX_AMSDU;
			}
			fifo_size = wlc->xmtfifo_szh[prio2fifo[i]];
			/* blocks to bytes */
			fifo_size = fifo_size * TXFIFO_SIZE_UNIT(wlc->pub->corerev);

			if (frag_disabled &&
				ami->txpolicy.amsdu_max_agg_bytes[i] == MIN(max_agg, fifo_size)) {
				/*
				 * Copy the A-MSDU  enable state over to
				 * properly initialize amsdu_agg_enable[]
				 */
				amsdu_agg_enable[i] = ami->txpolicy.amsdu_agg_enable[i];
				/* Nothing else to be done. */
				continue;
			}
#ifdef BCMDBG
			if (fragthresh > MIN(max_agg, fifo_size)) {
				WL_AMSDU(("wl%d:%s: MIN(max_agg=%d, fifo_sz=%d)=>amsdu_max_agg\n",
					wlc->pub->unit, __FUNCTION__, max_agg, fifo_size));
			}
#endif /* BCMDBG */
			ami->txpolicy.amsdu_max_agg_bytes[i] = MIN(fifo_size, max_agg);
			/* if frag not disabled, then take into account the fragthresh */
			if (!frag_disabled) {
				ami->txpolicy.amsdu_max_agg_bytes[i] =
					MIN(ami->txpolicy.amsdu_max_agg_bytes[i], fragthresh);
			}
		}
		amsdu_agg_enable[i] = ami->txpolicy.amsdu_agg_enable[i] &&
			(ami->txpolicy.amsdu_max_agg_bytes[i] > AMSDU_AGGBYTES_MIN);

		if (!amsdu_agg_enable[i]) {
			WL_AMSDU(("wlc_amsdu_agg_frag_upd: fragthresh is too small for AMSDU %d\n",
				i));
		}
	}

	/* update all scb limit */
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		wlc_amsdu_tx_scb_agglimit_upd_all(ami, scb, amsdu_agg_enable);
	}
} /* wlc_amsdu_agglimit_frag_upd */

/**
 * WLAMSDU_TX specific function.
 * called when ratelink memory -> ratesel.
 * driver need to update amsdu_agg_bytes_max value.
 */
void BCMFASTPATH
wlc_amsdu_scb_agglimit_calc(amsdu_info_t *ami, struct scb *scb)
{
	scb_amsdu_t *scb_ami;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	scb_ami->agglimit_upd = TRUE;
}

static void
wlc_amsdu_tx_scb_agglimit_upd(amsdu_info_t *ami, struct scb *scb)
{
	uint i;
	scb_amsdu_t *scb_ami;
	uint16 mtu_pref = 0;
	uint16 max_agg_len = 0;
	WL_MSG_MACFILTER_PFX_DECL;

	WL_MSG_MACFILTER_PFX_INIT(scb);

	if (!SCB_AMSDU(scb) || SCB_MARKED_DEL(scb) || SCB_DEL_IN_PROGRESS(scb)) {
		/* no action needed */
		return;
	}

	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	mtu_pref = scb_ami->mtu_pref;
	max_agg_len = wlc_amsdu_tx_scb_max_agg_len_upd(ami, scb);
	mtu_pref = MIN(mtu_pref, max_agg_len);
#ifdef BCMDBG
	WL_AMSDU_EX_PFX(scb, "scb->mtu_pref %d\n", mtu_pref);
#endif

	for (i = 0; i < NUMPRIO; i++) {
		WL_AMSDU_EX_PFX(scb, "scb_txpolicy[%d].agg_bytes_max: old = %d", i,
			scb_ami->scb_txpolicy[i].amsdu_agg_bytes_max);
#ifndef BCMSIM
		scb_ami->scb_txpolicy[i].amsdu_agg_bytes_max =
			MIN(mtu_pref, ami->txpolicy.amsdu_max_agg_bytes[i]);
#else
		/* BCMSIM has limited 2K buffer size */
		scb_ami->scb_txpolicy[i].amsdu_agg_bytes_max = AMSDU_MAX_MSDU_PKTLEN;
#endif
		WL_AMSDU_EX_NPFX(scb, " new = %d\n",
			scb_ami->scb_txpolicy[i].amsdu_agg_bytes_max);
	}
}

void
wlc_amsdu_tx_scb_set_max_agg_size(amsdu_info_t *ami, struct scb *scb, uint16 n_bytes)
{
	scb_amsdu_t *scb_ami;

	if (!SCB_AMSDU(scb) || SCB_MARKED_DEL(scb) || SCB_DEL_IN_PROGRESS(scb)) {
		/* no action needed */
		return;
	}

	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	if (scb_ami->mtu_pref == n_bytes) {
		return;
	}

	scb_ami->mtu_pref = n_bytes;
	wlc_amsdu_tx_scb_agglimit_upd(ami, scb);
}

/* Check if AMSDU is enabled for given tid */
bool
wlc_amsdu_chk_priority_enable(amsdu_info_t *ami, uint8 tid)
{
	return ami->txpolicy.amsdu_agg_enable[tid];
}

uint8 BCMFASTPATH
wlc_amsdu_scb_max_sframes(amsdu_info_t *ami, struct scb *scb)
{
	scb_amsdu_t *scb_ami;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);

	return scb_ami->amsdu_max_sframes;
}

uint32 BCMFASTPATH
wlc_amsdu_scb_max_agg_len(amsdu_info_t *ami, struct scb *scb, uint8 tid)
{
	scb_amsdu_t *scb_ami;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);

	/* If rate changed, driver need to recalculate
	 * the max aggregation length for vht and he rate.
	 */
	if (scb_ami->agglimit_upd) {
		wlc_amsdu_tx_scb_agglimit_upd(ami, scb);
		scb_ami->agglimit_upd = FALSE;
	}

	return scb_ami->scb_txpolicy[tid].amsdu_agg_bytes_max;
}

void
wlc_amsdu_max_agg_len_upd(amsdu_info_t *ami)
{
	struct scb *scb;
	struct scb_iter scbiter;

	/* update amsdu agg bytes for ALL scbs */
	FOREACHSCB(ami->wlc->scbstate, &scbiter, scb) {
		wlc_amsdu_tx_scb_agglimit_upd(ami, scb);
	}
}

static uint32 BCMFASTPATH
wlc_amsdu_tx_scb_max_agg_len_upd(amsdu_info_t *ami, struct scb *scb)
{
	wlc_info_t *wlc;
	uint8 nss, mcs, i;
	ratespec_t rspec;
	uint32 max_agg;
	ratesel_txparams_t ratesel_rates;
	const wlc_rlm_rate_store_t *rstore = NULL;
	bool updated = TRUE;
	WL_MSG_MACFILTER_PFX_DECL;

	WL_MSG_MACFILTER_PFX_INIT(scb);

	wlc = ami->wlc;

	if (WLCISACPHY(wlc->band)) {
		if (AMSDU_VHT_USE_HT_AGG_LIMITS_ENAB(wlc))
			max_agg = HT_MAX_AMSDU;
		else
			max_agg = VHT_MAX_AMSDU;
	} else {
		max_agg = HT_MIN_AMSDU;
		goto done;
	}

	if (RSPEC_ACTIVE(wlc->band->rspec_override)) {
		ratesel_rates.num = 1;
		ratesel_rates.rspec[0] = wlc->band->rspec_override;
	} else {
		uint16 flag0 = 0, flag1; /* as passed in para but not use it at caller */

		/* Force trigger ratemem update */
		ratesel_rates.num = 4; /* enable multi fallback rate */
		ratesel_rates.ac = 0; /* Use priority BE for reference */
		updated = wlc_scb_ratesel_gettxrate(wlc->wrsi, scb, &flag0, &ratesel_rates, &flag1);
		if (RATELINKMEM_ENAB(wlc->pub) && !updated) {
			uint16 rate_idx;

			rate_idx = wlc_ratelinkmem_get_scb_rate_index(wlc, scb);
			rstore = wlc_ratelinkmem_retrieve_cur_rate_store(wlc, rate_idx, TRUE);
			if (rstore == NULL) {
				WL_ERROR(("wl%d: %s, no rstore for scb: %p ("MACF", idx: %d)\n",
					wlc->pub->unit, __FUNCTION__, scb,
					ETHER_TO_MACF(scb->ea), rate_idx));
				wlc_scb_ratesel_init(wlc, scb);
				updated = wlc_scb_ratesel_gettxrate(wlc->wrsi, scb, &flag0,
					&ratesel_rates, &flag1);
				ASSERT(updated);
			}
		}
	}

	/* XXX
	 * Without the below for loop, the driver would hit "suppressed by continuous AGG0" if
	 * amsdu_aggsf >= 3 when using rate mcs0. To resolve the issue, the driver was modified to
	 * change max amsdu aggregation length dynamically, on a tx rateset change. RB:157173.
	 */
	for (i = 0; i < ratesel_rates.num; i++) {
		if (RATELINKMEM_ENAB(wlc->pub) && !updated) {
			rspec = WLC_RATELINKMEM_GET_RSPEC(rstore, i);
		} else {
			rspec = ratesel_rates.rspec[i];
		}

		if (RSPEC_ISHE(rspec)) {
			mcs = RSPEC_HE_MCS(rspec);
			nss = RSPEC_HE_NSS(rspec);
		} else if (RSPEC_ISVHT(rspec)) {
			mcs = RSPEC_VHT_MCS(rspec);
			nss = RSPEC_VHT_NSS(rspec);
		} else {
			max_agg = HT_MIN_AMSDU;
			WL_AMSDU_EX_PFX(scb, "ht or legacy rate\n");
			break;
		}

		if (RSPEC_IS20MHZ(rspec) && (mcs == 0) && (nss == 1)) {
			WL_AMSDU_EX_PFX(scb, "Rate:mcs=0 nss=1 bw=20Mhz\n");
			max_agg = MIN(max_agg, HT_MIN_AMSDU);
			break;
		}
	}

done:
	WL_AMSDU_EX_PFX(scb, "Update max_agg_len=%d\n", max_agg);

	return max_agg;
} /* wlc_amsdu_tx_scb_max_agg_len_upd */

#if defined(HWA_PKTPGR_BUILD)

/* Fixup AMSDU in single TxLfrag */
void
wlc_amsdu_single_txlfrag_fixup(wlc_info_t *wlc, void *head)
{
	osl_t *osh;
	void *pn = NULL;
	void *p1, *next, *p, *txlfrag;
	uint8 pkt_count, free_pkt_count, fi;
	uint32 len;

	HWA_PKT_DUMP_EXPR(hwa_txpost_dump_pkt(head, NULL,
		"++single_txlfrag_fixup++", 0, FALSE));

	// Setup local
	osh = wlc->osh;
	fi = 1;
	pkt_count = 0;
	free_pkt_count = 0;
	p1 = PKTNEXT(osh, head);
	PKTSETNEXT(osh, head, NULL);
	txlfrag = head;

	// Should not have link
	ASSERT(PKTLINK(txlfrag) == NULL);

	p = p1;
	while (p) {
		pn = p;
		next = PKTNEXT(osh, p);
		len = PKTFRAGLEN(osh, p, LB_FRAG1);
		PKTSETHOSTPKTID(txlfrag, fi, PKTFRAGPKTID(osh, p));
		PKTSETFRAGLEN(osh, txlfrag, fi, len);

		// Sum of all host length in host_datalen.
		PKTSETFRAGTOTLEN(osh, txlfrag, (PKTFRAGTOTLEN(osh, txlfrag) + len));

		PKTSETFRAGDATA_HI(osh, txlfrag, fi, PKTFRAGDATA_HI(osh, p, LB_FRAG1));
		PKTSETFRAGDATA_LO(osh, txlfrag, fi, PKTFRAGDATA_LO(osh, p, LB_FRAG1));
#if defined(PROP_TXSTATUS)
		PKTSETRDIDX(txlfrag, (fi-1), PKTFRAGRINGINDEX(osh, p));
#endif /* PROP_TXSTATUS */

		// Link free list
		PKTSETLINK(p, next);
		PKTSETNEXT(osh, p, NULL);
		HWA_PKT_DUMP_EXPR(hwa_txpost_dump_pkt(p, NULL,
			"single_txlfrag_fixup", fi, TRUE));

		pkt_count++;
		free_pkt_count++;
		fi++;

		p = next;

		// Use new txlfrag
		if (p && fi == HWA_PP_PKT_FRAGINFO_MAX) {
			// Set current txlfrag Frag Num
			PKTSETFRAGTOTNUM(osh, txlfrag, (PKTFRAGTOTNUM(osh, txlfrag) + pkt_count));
			pkt_count = 0;
			fi = 1;
			// Link txlfrags by next
			PKTSETNEXT(osh, txlfrag, p);
			// New txlfrag
			txlfrag = p;

			// Should not have link
			ASSERT(PKTLINK(txlfrag) == NULL);

			p = PKTNEXT(osh, txlfrag);
			PKTSETNEXT(osh, txlfrag, NULL);

			// Add to free list
			PKTSETLINK(pn, p);
		}
	}

	if (pkt_count) {
		PKTSETFRAGTOTNUM(osh, txlfrag, (PKTFRAGTOTNUM(osh, txlfrag) + pkt_count));
	}
#ifdef PROP_TXSTATUS
	WL_TRACE(("%s: txlfrag[seq<0x%x>, rindex<%d,%d,%d,%d>]\n",
		__FUNCTION__, (WLPKTTAG(txlfrag)->seq & WL_SEQ_AMSDU_SUPPR_MASK),
		PKTFRAGRINGINDEX(osh, txlfrag), PKTRDIDX(txlfrag, 0),
		PKTRDIDX(txlfrag, 1), PKTRDIDX(txlfrag, 2)));
#endif /* PROP_TXSTATUS */

	if (free_pkt_count) {
		HWA_PKT_DUMP_EXPR(hwa_txpost_dump_pkt(p1, NULL,
			"free:single_txlfrag_fixup", 0, FALSE));
		hwa_pktpgr_free_tx(hwa_dev, PPLBUF(p1), PPLBUF(pn), free_pkt_count);
#ifdef HNDPQP
		// Update PQP HBM credit
		pqp_hbm_avail_add(free_pkt_count);
#endif /* HNDPQP */
	}

	// Mark stop index
	if (fi < HWA_PP_PKT_FRAGINFO_MAX) {
		PKTSETHOSTPKTID(txlfrag, fi, 0);
	}

	HWA_PKT_DUMP_EXPR(hwa_txpost_dump_pkt(head, NULL,
		"--single_txlfrag_fixup--", 0, FALSE));
}

#endif /* HWA_PKTPGR_BUILD */

/**
 * Dynamic adjust amsdu_aggsf based on STA's bandwidth
 */
void
wlc_amsdu_tx_scb_aggsf_upd(amsdu_info_t *ami, struct scb *scb)
{
	wlc_info_t *wlc;
	scb_amsdu_t *scb_ami;
	uint8 bw, aggsf_cfp, aggsf;
	ratespec_t rspec;
	uint32 current_rate;

	wlc = ami->wlc;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	bw = wlc_scb_ratesel_get_link_bw(wlc, scb);

	rspec = wlc_scb_ratesel_get_primary(wlc, scb, NULL);
	current_rate = wf_rspec_to_rate(rspec);

	if (current_rate < AMSDU_OFF_RATE_THRESH) {
		if ((scb_ami->amsdu_aggsf > 1) ||
			(scb_ami->amsdu_max_sframes > 1)) {
			// Set max sf to 1.
			scb_ami->amsdu_aggsf = 1;
			scb_ami->amsdu_max_sframes = 1;
		}
		return;
	} else if ((scb_ami->amsdu_aggsf == 1) &&
		(scb_ami->amsdu_max_sframes == 1)) {

		if (current_rate < AMSDU_ON_RATE_THRESH) {
			return;
		}
	}

	/* For CFP path, limit the max amsdu_aggsf based on STA's bandwidth.
	 * 80Mhz: 4, 40Mhz: 2, 20Mhz: 2
	 */
	if (bw == BW_160MHZ) {
		aggsf_cfp = ami->txpolicy.amsdu_max_sframes;
	} else if (bw == BW_80MHZ) {
		aggsf_cfp = MIN(ami->txpolicy.amsdu_max_sframes, 4);
	} else {
		aggsf_cfp = MIN(ami->txpolicy.amsdu_max_sframes, 2);
	}

#if defined(BCMHWA)
	HWA_PKTPGR_EXPR({
		if ((hwa_pktpgr_multi_txlfrag(hwa_dev) == 0) && (aggsf_cfp > 4)) {
			WL_ERROR(("%s: Cannot support more then %d subframes in one A-MSDU\n",
				__FUNCTION__, aggsf_cfp));
			aggsf_cfp = 4;
		}
	});
#endif

	/* XXX: For legacy path, there is throughput regression for mfgtest driver
	 * when amsdu_aggsf > 4
	 */
	aggsf = MIN(aggsf_cfp, 4);

	/* XXX: The amsdu_max_sframes affects wlc_ampdu_taf_release, for pktpgr mode
	 * set aggsf equal to aggsf_cfp to have best tput.
	 * For PROPTXSTATUS enabled driver, suppression packet will use the mechanism of
	 * flow ring rollback. If using different amsdu aggsf setting for CFP and legacy path
	 * [aggsf=8 for CFP and aggsf=4 for legacy mode].
	 * When this 5 in1 amsdu gets suppressed and comes back, we try to reuse
	 * the saved d11 sequence number for the whole amsdu. But in legacy path,
	 * we dont allow aggregation of more than 4. So what really happens is
	 * we send one amsdu with 4 msdus and next one with a duplicate d11 sequence number
	 * resulting in RX side dropping the frame.
	 * Use the same amsdu_aggsf setting for both path to eliminate this problem.
	 */
#if defined(BCMHWA)
	HWA_TXPOST_EXPR(aggsf = aggsf_cfp);
#endif

	scb_ami->amsdu_aggsf = aggsf_cfp;
	scb_ami->amsdu_max_sframes = aggsf;
}

uint32 BCMFASTPATH
wlc_amsdu_scb_aggsf(amsdu_info_t *ami, struct scb *scb, uint8 tid)
{
	scb_amsdu_t *scb_ami;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);

	return scb_ami->amsdu_aggsf;
}

/* count sub frames of a mpdu */
uint32 BCMFASTPATH
wlc_amsdu_msdu_cnt(osl_t *osh, void *p)
{
	uint32 cnt;

	for (cnt = 0; p; p = PKTNEXT(osh, p)) {
#if defined(HWA_PKTPGR_BUILD)
		cnt += PKTFRAGTOTNUM(osh, p);
#else
		cnt++;
#endif
	}

	return cnt;
}
#endif /* WLAMSDU_TX */

/**
 * We should not come here typically!!!!
 * if we are here indicates we received a corrupted packet which is tagged as AMSDU by the ucode.
 * So flushing invalid AMSDU chain. When anything other than AMSDU is received when AMSDU state is
 * not idle, flush the collected intermediate amsdu packets.
 */
void BCMFASTPATH
wlc_amsdu_flush(amsdu_info_t *ami)
{
	int fifo;
	amsdu_deagg_t *deagg;

	for (fifo = 0; fifo < RX_FIFO_NUMBER; fifo++) {
		deagg = &ami->amsdu_deagg[fifo];
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_IDLE)
			wlc_amsdu_deagg_flush(ami, fifo);
	}
}

/**
 * return FALSE if filter failed
 *   caller needs to toss all buffered A-MSDUs and p
 *   Enhancement: in case of out of sequences, try to restart to
 *     deal with lost of last MSDU, which can occur frequently due to fcs error
 *   Assumes receive status is in host byte order at this point.
 *   PKTDATA points to start of receive descriptor when called.
 */
void * BCMFASTPATH
wlc_recvamsdu(amsdu_info_t *ami, wlc_d11rxhdr_t *wrxh, void *p, uint16 *padp, bool chained_sendup)
{
	osl_t *osh;
	amsdu_deagg_t *deagg;
	uint aggtype;
	int fifo;
	uint16 pad;                     /* Number of bytes of pad */
	wlc_info_t *wlc = ami->wlc;
	d11rxhdr_t *rxh;

	/* packet length starting at 802.11 mac header (first frag) or eth header (others) */
	uint32 pktlen;

	uint32 pktlen_w_plcp;           /* packet length starting at PLCP */
	struct dot11_header *h;
#ifdef BCMDBG
	int msdu_cnt = -1;              /* just used for debug */
#endif
	wlc_rx_pktdrop_reason_t toss_reason; /* must be set if tossing */

#if !defined(PKTC) && !defined(PKTC_DONGLE)
	BCM_REFERENCE(chained_sendup);
#endif
	osh = ami->pub->osh;

	ASSERT(padp != NULL);

	rxh = &wrxh->rxhdr;
	aggtype = RXHDR_GET_AGG_TYPE(rxh, wlc);
	pad = RXHDR_GET_PAD_LEN(rxh, wlc);

#ifdef BCMDBG
	msdu_cnt = RXHDR_GET_MSDU_COUNT(rxh, wlc);
#endif  /* BCMDBG */

	/* PKTDATA points to rxh. Get length of packet w/o rxh, but incl plcp */
	pktlen = pktlen_w_plcp = PKTLEN(osh, p) - (wlc->hwrxoff + pad);

#ifdef DONGLEBUILD
	/* XXX Manage memory pressure by right sizing the packet since there
	 * can be lots of subframes that are going to be chained together
	 * pending the reception of the final subframe with its FCS for
	 * MPDU.  Note there is a danger of a "low memory deadlock" if you
	 * exhaust the available supply of packets by chaining subframes and
	 * as result you can't rxfill the wlan rx fifo and thus receive the
	 * final AMSDU subframe or even another MPDU to invoke a AMSDU cleanup
	 * action.  Also you might be surprised this can happen, to the extreme,
	 * if you receive a normal, but corrupt, MPDU which looks like a ASMDU
	 * to ucode because it releases frames to the FIFO/driver before it sees
	 * the CRC, i.e. ucode AMSDU deagg.  A misinterpted MPDU with lots of
	 * zero data can look like a large number of 16 byte minimum ASMDU
	 * subframes.  This is more than a theory, this actually happened repeatedly
	 * in open, congested air.
	 */
	if (PKTISRXFRAG(osh, p)) {
		/* for splitRX enabled case, pkt clonning is not valid */
		/* since part of the packet is in host */
	} else {
		uint headroom;
		void *dup_p;
		uint16 pkt_len = 0;          /* packet length, including rxh */

		/* Make virtually an identical copy of the original packet with the same
		 * headroom and data.  Only the tailroom will be diffent, i.e the packet
		 * is right sized.
		 *
		 * What about making sure we have the same alignment if necessary?
		 * If you can't get a "right size" packets,just continue
		 * with the current one. You don't have much choice
		 * because the rest of the AMSDU could be in the
		 * rx dma ring and/or FIFO and be ack'ed already by ucode.
		 */
		pkt_len = PKTLEN(osh, p);

		if (pkt_len < wlc->pub->tunables->amsdu_resize_buflen) {
			headroom = PKTHEADROOM(osh, p);
			if ((dup_p = PKTGET(osh, headroom + pkt_len,
				FALSE)) != NULL) {
#ifdef BCMPCIEDEV
				if (BCMPCIEDEV_ENAB()) {
					rxcpl_info_t *p_rxcpl_info = bcm_alloc_rxcplinfo();
					if (p_rxcpl_info == NULL) {
						WL_ERROR(("%s:  RXCPLID not free\n", __FUNCTION__));
						/* try to send an error to host */
						PKTFREE(osh, dup_p, FALSE);
						ASSERT(p_rxcpl_info);
						goto skip_amsdu_resize;
					}
					PKTSETRXCPLID(osh, dup_p, p_rxcpl_info->rxcpl_id.idx);
				}
#endif /* BCMPCIEDEV */
				PKTPULL(osh, dup_p, headroom);
				bcopy(PKTDATA(osh, p) - headroom, PKTDATA(osh, dup_p) - headroom,
					PKTLEN(osh, p) + headroom);
				PKTFREE(osh, p, FALSE);
				p = dup_p;
			} else {
#ifdef BCMPCIEDEV
				/*
				 * For splitrx cases this is fatal if no more packets are posted
				 * to classified fifo
				*/
				if (BCMPCIEDEV_ENAB() &&
					(dma_rxactive(WLC_HW_DI(wlc, PKT_CLASSIFY_FIFO)) == 0)) {
					WL_ERROR(("%s: AMSDU resizing packet alloc failed \n",
						__FUNCTION__));
					ASSERT(0);
				}
#endif /* BCMPCIEDEV */
			}
		}
	}
#ifdef BCMPCIEDEV
skip_amsdu_resize:
#endif
#endif /* DONGLEBUILD */
	fifo = (D11RXHDR_ACCESS_VAL(&wrxh->rxhdr, ami->pub->corerev, fifo));
	ASSERT((fifo < RX_FIFO_NUMBER));
	deagg = &ami->amsdu_deagg[fifo];

	WLCNTINCR(ami->cnt->deagg_msdu);

	WL_AMSDU(("%s: aggtype %d, msdu count %d\n", __FUNCTION__, aggtype, msdu_cnt));

	h = (struct dot11_header *)(PKTDATA(osh, p) + wlc->hwrxoff + pad +
		D11_PHY_RXPLCP_LEN(wlc->pub->corerev));

	switch (aggtype) {
	case RXS_AMSDU_FIRST:
		/* PKTDATA starts with PLCP */
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_IDLE) {
			WL_AMSDU(("%s: wrong A-MSDU deagg sequence, cur_state=%d\n",
				__FUNCTION__, deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			wlc_amsdu_deagg_flush(ami, fifo);
			/* keep this valid one and reset to improve throughput */
		}

		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_FIRST;

		/* Store the frontpad value of the first subframe */
		deagg->first_pad = *padp;

		if (!wlc_amsdu_deagg_open(ami, fifo, p, h, pktlen_w_plcp)) {
			toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_OPEN;
			goto abort;
		}

		/* Packet length w/o PLCP */
		pktlen = pktlen_w_plcp - D11_PHY_RXPLCP_LEN(wlc->pub->corerev);

		WL_AMSDU(("%s: first A-MSDU buffer\n", __FUNCTION__));
		break;

	case RXS_AMSDU_INTERMEDIATE:
		/* PKTDATA starts with subframe header */
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_FIRST) {
			WL_AMSDU_ERROR(("%s: wrong A-MSDU deagg sequence, cur_state=%d, agg=%d\n",
				__FUNCTION__, deagg->amsdu_deagg_state, aggtype));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_SEQ;
			goto abort;
		}

#ifdef ASSERT
		/* intermediate frames should have 2 byte padding if wlc->hwrxoff is aligned
		* on mod 4 address
		*/
		if ((wlc->hwrxoff % 4) == 0) {
			ASSERT(pad != 0);
		} else {
			ASSERT(pad == 0);
		}
#endif /* ASSERT */

		if ((pktlen) < ETHER_HDR_LEN) {
			WL_AMSDU_ERROR(("%s: rxrunt, agg=%d\n", __FUNCTION__, aggtype));
			WLCNTINCR(ami->pub->_cnt->rxrunt);
			toss_reason = RX_PKTDROP_RSN_RUNT_FRAME;
			goto abort;

		}

		ASSERT(deagg->amsdu_deagg_ptail);
		PKTSETNEXT(osh, deagg->amsdu_deagg_ptail, p);
		deagg->amsdu_deagg_ptail = p;
		WL_AMSDU(("%s:   mid A-MSDU buffer\n", __FUNCTION__));
		break;
	case RXS_AMSDU_LAST:
		/* PKTDATA starts with last subframe header */
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_FIRST) {
			WL_AMSDU_ERROR(("%s: wrong A-MSDU deagg sequence, cur_state=%d\n",
				__FUNCTION__, deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_SEQ;
			goto abort;
		}

		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_LAST;

#ifdef ASSERT
		/* last frame should have 2 byte padding if wlc->hwrxoff is aligned
		* on mod 4 address
		*/
		if ((wlc->hwrxoff % 4) == 0) {
			ASSERT(pad != 0);
		} else {
			ASSERT(pad == 0);
		}
#endif /* ASSERT */

		if ((pktlen) < (ETHER_HDR_LEN + (PKTISRXFRAG(osh, p) ? 0 :  DOT11_FCS_LEN))) {
			WL_AMSDU_ERROR(("%s: rxrunt\n", __FUNCTION__));
			WLCNTINCR(ami->pub->_cnt->rxrunt);
			toss_reason = RX_PKTDROP_RSN_RUNT_FRAME;
			goto abort;
		}

		ASSERT(deagg->amsdu_deagg_ptail);
		PKTSETNEXT(osh, deagg->amsdu_deagg_ptail, p);
		deagg->amsdu_deagg_ptail = p;
		WL_AMSDU(("%s: last A-MSDU buffer\n", __FUNCTION__));
		break;

	case RXS_AMSDU_N_ONE:
		/* this frame IS AMSDU, checked by caller */

		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_IDLE) {
			WL_AMSDU(("%s: wrong A-MSDU deagg sequence, cur_state=%d\n",
				__FUNCTION__, deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			wlc_amsdu_deagg_flush(ami, fifo);

			/* keep this valid one and reset to improve throughput */
		}

		ASSERT((deagg->amsdu_deagg_p == NULL) && (deagg->amsdu_deagg_ptail == NULL));
		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_LAST;

		/* Store the frontpad value of this single subframe */
		deagg->first_pad = *padp;

		if (!wlc_amsdu_deagg_open(ami, fifo, p, h, pktlen_w_plcp)) {
			toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_OPEN;
			goto abort;
		}

		/* Packet length w/o PLCP */
		pktlen = pktlen_w_plcp - D11_PHY_RXPLCP_LEN(wlc->pub->corerev);

		break;

	default:
		/* can't be here */
		ASSERT(0);
		toss_reason = RX_PKTDROP_RSN_BAD_AGGTYPE;
		goto abort;
	}

	/* Note that pkttotlen now includes the length of the rxh for each frag */
	WL_AMSDU(("%s: add one more A-MSDU buffer %d bytes, accumulated %d bytes\n",
		__FUNCTION__, pktlen_w_plcp, pkttotlen(osh, deagg->amsdu_deagg_p)));

	if (deagg->amsdu_deagg_state == WLC_AMSDU_DEAGG_LAST) {
		void *pp = deagg->amsdu_deagg_p;
#ifdef WL11K
		uint tot_len = pkttotlen(osh, pp);
#endif
		deagg->amsdu_deagg_p = deagg->amsdu_deagg_ptail = NULL;
		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_IDLE;

		/* ucode/hw deagg happened */

/* XXX This will ASSERT() in wlc_sendup_chain
 * if PKTC or PKTC_DONGLE is not defined
 */
		WLPKTTAG(pp)->flags |= WLF_HWAMSDU;

		/* First frame has fully defined Receive Frame Header,
		 * handle it to normal MPDU process.
		 */
		WLCNTINCR(ami->pub->_cnt->rxfrag);
		WLCNTINCR(ami->cnt->deagg_amsdu);
#ifdef WL11K
		WLCNTADD(ami->cnt->deagg_amsdu_bytes_l, tot_len);
		if (ami->cnt->deagg_amsdu_bytes_l < tot_len)
			WLCNTINCR(ami->cnt->deagg_amsdu_bytes_h);
#endif

#if defined(WL_MU_RX) && defined(WLCNT) && (defined(BCMDBG) || defined(WLDUMP) || \
	defined(BCMDBG_MU))

		/* Update the murate to the plcp
		 * last rxhdr has murate information
		 * plcp is in the first packet
		 */
		if (MU_RX_ENAB(wlc) && wlc_murx_active(wlc->murx)) {
			wlc_d11rxhdr_t *frag_wrxh;   /* rx status of an AMSDU frag in chain */
			d11rxhdr_t *_rxh;
			uchar *plcp;
			uint16 pad_cnt;
			frag_wrxh = (wlc_d11rxhdr_t *)PKTDATA(osh, pp);
			_rxh = &frag_wrxh->rxhdr;

			/* Check for short or long format */
			pad_cnt = RXHDR_GET_PAD_LEN(_rxh, wlc);
			plcp = (uchar *)(PKTDATA(osh, pp) + wlc->hwrxoff + pad_cnt);

			wlc_bmac_upd_murate(wlc, &wrxh->rxhdr, plcp);
		}
#endif /* WL_MU_RX */

#if defined(PKTC) || defined(PKTC_DONGLE)
		/* if chained sendup, return back head pkt and front padding of first sub-frame */
		/* if unchained wlc_recvdata takes pkt till bus layer */
		if (chained_sendup == TRUE) {
			*padp = deagg->first_pad;
			deagg->first_pad = 0;
			return (pp);
		} else
#endif
		{
			/* Strip rxh from all amsdu frags in amsdu chain before send up */
			void *np = pp;
			uint16 pad_cnt;
			wlc_d11rxhdr_t *frag_wrxh = NULL;   /* Subframe rxstatus */
			d11rxhdr_t *_rxh;

			/* Loop through subframes to remove rxstatus */
			while (np) {
				frag_wrxh = (wlc_d11rxhdr_t*) PKTDATA(osh, np);
				_rxh = &frag_wrxh->rxhdr;
				pad_cnt = RXHDR_GET_PAD_LEN(_rxh, wlc);

				PKTPULL(osh, np, wlc->hwrxoff + pad_cnt);
				np = PKTNEXT(osh, np);
			}
			ASSERT(frag_wrxh);
			wlc_recvdata(wlc, osh, frag_wrxh, pp);
		}
		deagg->first_pad = 0;
	}

	/* all other cases needs no more action, just return */
	return  NULL;

abort:
	wlc_amsdu_deagg_flush(ami, fifo);
	RX_PKTDROP_COUNT(wlc, NULL, toss_reason);
	PKTFREE(osh, p, FALSE);
	return  NULL;
} /* wlc_recvamsdu */

/** return FALSE if A-MSDU verification failed */
static bool BCMFASTPATH
wlc_amsdu_deagg_verify(amsdu_info_t *ami, uint16 fc, void *h)
{
	bool is_wds;
	uint16 *pqos;
	uint16 qoscontrol;

	BCM_REFERENCE(ami);

	/* it doesn't make sense to aggregate other type pkts, toss them */
	if ((fc & FC_KIND_MASK) != FC_QOS_DATA) {
		WL_AMSDU(("wlc_amsdu_deagg_verify fail: fc 0x%x is not QoS data type\n", fc));
		return FALSE;
	}

	is_wds = ((fc & (FC_TODS | FC_FROMDS)) == (FC_TODS | FC_FROMDS));
	pqos = (uint16*)((uchar*)h + (is_wds ? DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN));
	qoscontrol = ltoh16_ua(pqos);

	if (qoscontrol & QOS_AMSDU_MASK)
		return TRUE;

	WL_AMSDU_ERROR(("%s fail: qos field 0x%x\n", __FUNCTION__, *pqos));
	return FALSE;
}

/**
 * Start a new AMSDU receive chain. Verifies that the frame is a data frame
 * with QoS field indicating AMSDU, and that the frame is long enough to
 * include PLCP, 802.11 mac header, QoS field, and AMSDU subframe header.
 * Inputs:
 *   ami    - AMSDU state
 *   fifo   - queue on which current frame was received
 *   p      - first frag in a sequence of AMSDU frags. PKTDATA(p) points to
 *            start of receive descriptor
 *   h      - start of ethernet header in received frame
 *   pktlen - frame length, starting at PLCP
 *
 * Returns:
 *   TRUE if new AMSDU chain is started
 *   FALSE otherwise
 */
static bool BCMFASTPATH
wlc_amsdu_deagg_open(amsdu_info_t *ami, int fifo, void *p, struct dot11_header *h, uint32 pktlen)
{
	osl_t *osh = ami->pub->osh;
	amsdu_deagg_t *deagg = &ami->amsdu_deagg[fifo];
	uint16 fc;

	BCM_REFERENCE(osh);

	if (pktlen < (uint32)(D11_PHY_RXPLCP_LEN(ami->pub->corerev) + DOT11_MAC_HDR_LEN
		+ DOT11_QOS_LEN + ETHER_HDR_LEN)) {
		WL_AMSDU(("%s: rxrunt\n", __FUNCTION__));
		WLCNTINCR(ami->pub->_cnt->rxrunt);
		goto fail;
	}

	fc = ltoh16(h->fc);

	if (!wlc_amsdu_deagg_verify(ami, fc, h)) {
		WL_AMSDU(("%s: AMSDU verification failed, toss\n", __FUNCTION__));
		WLCNTINCR(ami->cnt->deagg_badfmt);
		goto fail;
	}

	/* explicitly test bad src address to avoid sending bad deauth */
	if ((ETHER_ISNULLADDR(&h->a2) || ETHER_ISMULTI(&h->a2))) {
		WL_AMSDU(("%s: wrong address 2\n", __FUNCTION__));
		WLCNTINCR(ami->pub->_cnt->rxbadsrcmac);
		goto fail;
	}

	deagg->amsdu_deagg_p = p;
	deagg->amsdu_deagg_ptail = p;
	return TRUE;

fail:
	WLCNTINCR(ami->cnt->deagg_openfail);
	return FALSE;
} /* wlc_amsdu_deagg_open */

static void BCMFASTPATH
wlc_amsdu_deagg_flush(amsdu_info_t *ami, int fifo)
{
	amsdu_deagg_t *deagg = &ami->amsdu_deagg[fifo];
	WL_AMSDU(("%s\n", __FUNCTION__));

	if (deagg->amsdu_deagg_p)
		PKTFREE(ami->pub->osh, deagg->amsdu_deagg_p, FALSE);

	deagg->first_pad = 0;
	deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_IDLE;
	deagg->amsdu_deagg_p = deagg->amsdu_deagg_ptail = NULL;
#ifdef WLCNT
	WLCNTINCR(ami->cnt->deagg_flush);
#endif /* WLCNT */
}

#if defined(PKTC) || defined(PKTC_DONGLE)
static void
wlc_amsdu_to_dot11(amsdu_info_t *ami, struct scb *scb, struct dot11_header *hdr, void *pkt)
{
	osl_t *osh;

	/* ptr to 802.3 or eh in incoming pkt */
	struct ether_header *eh;
	struct dot11_llc_snap_header *lsh;

	/* ptr to 802.3 or eh in new pkt */
	struct ether_header *neh;
	struct dot11_header *phdr;

	wlc_bsscfg_t *cfg = scb->bsscfg;
	BCM_REFERENCE(cfg);
	osh = ami->pub->osh;

	/* If we discover an ethernet header, replace it by an 802.3 hdr + SNAP header */
	eh = (struct ether_header *)PKTDATA(osh, pkt);

	if (ntoh16(eh->ether_type) > ETHER_MAX_LEN) {
		neh = (struct ether_header *)PKTPUSH(osh, pkt, DOT11_LLC_SNAP_HDR_LEN);

		/* Avoid constructing 802.3 header as optimization.
		 * 802.3 header(14 bytes) is going to be overwritten by the 802.11 header.
		 * This will save writing 14-bytes for every MSDU.
		 */

		/* Construct LLC SNAP header */
		lsh = (struct dot11_llc_snap_header *)
			((char *)neh + ETHER_HDR_LEN);
		lsh->dsap = 0xaa;
		lsh->ssap = 0xaa;
		lsh->ctl = 0x03;
		lsh->oui[0] = 0;
		lsh->oui[1] = 0;
		lsh->oui[2] = 0;
		/* The snap type code is already in place, inherited from the ethernet header that
		 * is now overlaid.
		 */
	}
	else {
		neh = (struct ether_header *)PKTDATA(osh, pkt);
	}

	if (BSSCFG_AP(cfg)) {
		/* Force the 802.11 a2 address to be the ethernet source address */
		bcopy((char *)neh->ether_shost,
			(char *)&hdr->a2, ETHER_ADDR_LEN);
	} else {
		if (BSSCFG_IBSS(cfg)) {
			/* Force the 802.11 a3 address to be the ethernet source address
			 * IBSS has BSS as a3, so leave a3 alone for win7+
			 */
			bcopy((char *)neh->ether_shost,
				(char *)&hdr->a3, ETHER_ADDR_LEN);
		}
	}

	/* Replace the 802.3 header, if present, by an 802.11 header. The original 802.11 header
	 * was appended to the frame along with the receive data needed by Microsoft.
	 */
	phdr = (struct dot11_header *)
		PKTPUSH(osh, pkt, DOT11_A3_HDR_LEN - ETHER_HDR_LEN);

	bcopy((char *)hdr, (char *)phdr, DOT11_A3_HDR_LEN);

	/* Clear all frame control bits except version, type, data subtype & from-ds/to-ds */
	phdr->fc = htol16(ltoh16(phdr->fc) & (FC_FROMDS | FC_TODS | FC_TYPE_MASK |
		(FC_SUBTYPE_MASK & ~QOS_AMSDU_MASK) | FC_PVER_MASK));
} /* wlc_amsdu_to_dot11 */

#endif /* PKTC || PKTC_DONGLE */

/**
 * Security Vulnerability: SW WAR for AMSDU deaggregation attack.
 * Validate AMSDU subframe header.
 */
bool
wlc_amsdu_validate_sf_hdr(wlc_info_t *wlc, struct scb *scb, struct ether_header *eh)
{
	wlc_bsscfg_t * bsscfg;

	ASSERT(scb != NULL);
	bsscfg = SCB_BSSCFG(scb);

	/* Check for AMSDU deagg attack only if STA is not SPP capable. */
	if (SCB_SPP_AMSDU(scb)) {
		return TRUE;
	}

	/* If AMSDU subframe header matches with LLC-SNAP header and there is no signaling
	 * integrity for A-MSDU (SPP) then this could be an AMSDU deagg attack.
	 */

	/* For A4 data (WDS/DWDS/MAP), DA/SA will not match with RA/TA so check only
	 * LLCP-SNAP signature.
	 * For AP mode, DA must not look like LLC-SNAP hdr.
	 */
	if (SCB_A4_DATA(scb) || BSSCFG_AP(bsscfg)) {
		return (!rfc894chk(wlc, (struct dot11_llc_snap_header *)eh));
	} else {
		if (ETHER_ISMULTI(eh->ether_dhost)) {
			return TRUE;
		}
		/* For STA, the DA (if not multicast) must match its own MAC addr.
		 * This condition is stronger than the LLC-SNAP signature check so
		 * no need of LLC-SNAP check.
		 */
		return (eacmp(eh->ether_dhost, &bsscfg->cur_etheraddr) == 0);
	}
}

#ifdef BCMWAPI_WPI
#define AMSDU_WAPI_WAI_HDR(pbody, _off, _len) \
	(bcmp(&WAPI_WAI_SNAP_HDR[DOT11_LLC_SNAP_HDR_LEN - _len], ((uint8*)pbody + _off), \
	  (_len)) == 0)
#endif /* BCMWAPI_WPI */

#define AMSDU_KM_DOT1X_HDR(pbody, _off, _len) \
	(bcmp(&wlc_802_1x_hdr[DOT11_LLC_SNAP_HDR_LEN - _len], ((uint8*)pbody + _off), \
	(_len)) == 0)
bool
wlc_amsdu_validate_sf_encryption(wlc_info_t *wlc, struct scb *scb, void *p, uint32 len,
	uint16 fc)
{
	bool hdr_conv = PKTISHDRCONVTD(wlc->osh, p);
	uint8 *pbody = (uint8*) PKTDATA(wlc->osh, p);
	uint8 cmp_len = (hdr_conv ? ETHER_TYPE_LEN : DOT11_LLC_SNAP_HDR_LEN);
	uint8 offset = (hdr_conv ? (ETHER_ADDR_LEN * 2) : ETHER_HDR_LEN);
	uint32 wpa_auth;

	if ((fc & (FC_TODS|FC_FROMDS)) == (FC_TODS|FC_FROMDS)) { /* wds ? */
		wpa_auth = scb->bsscfg->WPA_auth;
	} else {
		wpa_auth = scb->WPA_auth;
	}

	if (!(fc & FC_WEP) && WSEC_ENABLED(scb->bsscfg->wsec) &&
		(wpa_auth != WPA_AUTH_DISABLED) &&
		scb->bsscfg->wsec_restrict) {

		if (len < (cmp_len + offset)) {
			return FALSE;
		}
#ifdef BCMWAPI_WPI
		if (IS_WAPI_AUTH(wpa_auth) && (!AMSDU_WAPI_WAI_HDR(pbody, offset, cmp_len))) {
			return FALSE;
		}
#endif /* BCMWAPI_WPI */
		if (!AMSDU_KM_DOT1X_HDR(pbody, offset, cmp_len)) {
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * Called when the 'WLF_HWAMSDU' flag is set in the PKTTAG of a received frame.
 * A-MSDU decomposition: break A-MSDU(chained buffer) to individual buffers
 *
 *    | 80211 MAC HEADER | subFrame 1 |
 *			               --> | subFrame 2 |
 *			                                 --> | subFrame 3... |
 * where, 80211 MAC header also includes QOS and/or IV fields
 *        f->p, at function entry, has PKTDATA() pointing at the start of the 802.11 mac header.
 *        f->pbody points to beginning of subFrame 1,
 *        f->totlen is the total body len(chained, after mac/qos/iv header) w/o icv and FCS
 *
 *        each subframe is in the form of | 8023hdr | body | pad |
 *                subframe other than the last one may have pad bytes
*/
void
wlc_amsdu_deagg_hw(amsdu_info_t *ami, struct scb *scb, struct wlc_frminfo *f)
{
	osl_t *osh;
	wlc_info_t *wlc = ami->wlc;
	void *sf[MAX_RX_SUBFRAMES], *newpkt;
	struct ether_header *eh;
	uint32 body_offset, sflen = 0, len = 0;
	uint num_sf = 0, i;
	int resid;
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	uint32 amsdu_bytes = 0;
#endif /* BCMDBG || BCMDBG_AMSDU */
	wlc_rx_pktdrop_reason_t toss_reason; /* must be set if tossing */
	uint16 fc = ltoh16(f->fc);
	WL_MSG_MACFILTER_PFX_DECL;

	ASSERT(WLPKTTAG(f->p)->flags & WLF_HWAMSDU);
	osh = ami->pub->osh;

	/* strip mac header, move to start from A-MSDU body */
	body_offset = (uint)(f->pbody - (uchar*)PKTDATA(osh, f->p));
	PKTPULL(osh, f->p, body_offset);

	WL_MSG_MACFILTER_PFX_INIT(scb);
	WL_AMSDU_EX_PFX(scb, "body_len(exclude icv and FCS) %d\n", f->totlen);

	resid = f->totlen;
	newpkt = f->p;

	/* break chained AMSDU into N independent MSDU */
	while (newpkt != NULL) {
		/* there must be a limit to stop in order to prevent memory/stack overflow */
		if (num_sf >= MAX_RX_SUBFRAMES) {
			WL_AMSDU_ERROR(("%s: more than %d MSDUs !\n", __FUNCTION__, num_sf));
			break;
		}

		/* each subframe is 802.3 frame */
		eh = (struct ether_header*) PKTDATA(osh, newpkt);

		/* Check for AMSDU deagg attack. */
		if ((num_sf == 0) && !wlc_amsdu_validate_sf_hdr(wlc, scb, eh)) {
			WL_AMSDU_ERROR(("%s: AMSDU deagg attack; Invalid AMSDU\n", __FUNCTION__));
			WL_AMSDU_ERROR(("%s: bsscfg mode: %s SCB_A4 %d\n", __FUNCTION__,
				BSSCFG_AP(SCB_BSSCFG(scb)) ? "AP" : "STA", SCB_A4_DATA(scb)));
			prhex("Dump pkt", (uint8 *)eh, 32);
			WLCNTINCR(ami->cnt->deagg_badfmt);
			toss_reason = RX_PKTDROP_RSN_BAD_PROTO;
			goto toss;
		}

		len = PKTLEN(osh, newpkt) + PKTFRAGUSEDLEN(osh, newpkt);

		/* Drop all unencrypted non-eapol data frames if key is required.
		 * Take exception for NAN which accept unencrypted multicast data
		 */
		if (!wlc_amsdu_validate_sf_encryption(wlc, scb, newpkt, len, fc)) {
			WL_AMSDU_ERROR(("%s: sf body is not encrypted properly!\n", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_badfmt);
			toss_reason = RX_PKTDROP_RSN_BAD_PROTO;
			goto toss;
		}

#ifdef BCMPCIEDEV
		if (BCMPCIEDEV_ENAB() && PKTISHDRCONVTD(osh, newpkt)) {

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
			amsdu_bytes += (uint16)PKTFRAGUSEDLEN(osh, newpkt);
#endif /* BCMDBG || BCMDBG_AMSDU */

			/* Skip header conversion */
		        goto skip_conv;
		}
#endif /* BCMPCIEDEV */

		sflen = ntoh16(eh->ether_type) + ETHER_HDR_LEN;

		if ((((uintptr)eh + (uint)ETHER_HDR_LEN) % 4)  != 0) {
			WL_AMSDU_ERROR(("%s: sf body is not 4 bytes aligned!\n", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_badsfalign);
			toss_reason = RX_PKTDROP_RSN_DEAGG_UNALIGNED;
			goto toss;
		}

		/* last MSDU: has FCS, but no pad, other MSDU: has pad, but no FCS */
		if (len != (PKTNEXT(osh, newpkt) ? ROUNDUP(sflen, 4) : sflen)) {

			/* XXX: In 6878, ucode is passsing incorrect length for a single MSDU in
			 * an AMSDU (aggtype - RXS_AMSDU_N_ONE) i.e., there are two trailing
			 * bytes.
			 * SW WAR: Stripping the extra two bytes at tail.
			 */
			if (D11REV_IS(wlc->pub->corerev, 61) && (num_sf == 0) &&
				(PKTNEXT(osh, newpkt) == NULL) && ((len - sflen) == 2)) {
				WL_AMSDU(("wl%d: %s: 6878 WAR: Strip exta two bytes at tail\n",
					wlc->pub->unit, __FUNCTION__));
				PKTSETLEN(osh, newpkt, (PKTLEN(osh, newpkt) - 2));
			} else {

				WL_AMSDU_ERROR(("%s: len mismatch buflen %d sflen %d, sf %d\n",
					__FUNCTION__, len, sflen, num_sf));
#ifdef RX_DEBUG_ASSERTS
				WL_ERROR(("%s : LEN mismatch : head pkt %p new pkt %p"
					"FC 0x%02x seq 0x%02x \n",
					__FUNCTION__, f->p, newpkt, f->fc, f->seq));
				/* Dump previous RXS info */
				wlc_print_prev_rxs(wlc);
				ASSERT(0);
#endif /* RX_DEBUG_ASSERTS */
				WLCNTINCR(ami->cnt->deagg_badsflen);
				toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_SF_LEN;
				goto toss;
			}
		}

		/* strip trailing optional pad */
		if (PKTFRAGUSEDLEN(osh, newpkt)) {
			/* set total length to sflen */
			PKTSETFRAGUSEDLEN(osh, newpkt, (sflen - PKTLEN(osh, newpkt)));
		} else {
			PKTSETLEN(osh, newpkt, sflen);
		}

		{
			/* convert 8023hdr to ethernet if necessary */
			wlc_8023_etherhdr(wlc, osh, newpkt);
		}

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
		amsdu_bytes += (uint16)PKTLEN(osh, newpkt) +
				(uint16)PKTFRAGUSEDLEN(osh, newpkt);
#endif /* BCMDBG || BCMDBG_AMSDU */

#ifdef BCMPCIEDEV
skip_conv:
#endif /* BCMPCIEDEV */
		/* propagate prio, NO need to transfer other tags, it's plain stack packet now */
		PKTSETPRIO(newpkt, f->prio);

		WL_AMSDU_EX_PFX(scb, "deagg MSDU buffer %d, frame %d\n", len, sflen);

		sf[num_sf] = newpkt;
		num_sf++;
		newpkt = PKTNEXT(osh, newpkt);

		resid -= len;
	}

	if (resid != 0) {
		ASSERT(0);
		WLCNTINCR(ami->cnt->deagg_badtotlen);
		toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_SF_LEN;
		goto toss;
	}

	/* cut the chain: set PKTNEXT to NULL */
	for (i = 0; i < num_sf; i++)
		PKTSETNEXT(osh, sf[i], NULL);

	/* toss the remaining MSDU, which we couldn't handle */
	if (newpkt != NULL) {
		WL_AMSDU_ERROR(("%s: toss MSDUs > %d !\n", __FUNCTION__, num_sf));
		PKTFREE(osh, newpkt, FALSE);
	}

	/* forward received data in host direction */
	for (i = 0; i < num_sf; i++) {
		struct ether_addr * ea;

		WL_AMSDU_EX_PFX(scb, "sendup subframe %d\n", i);

		ea = (struct ether_addr *) PKTDATA(osh, sf[i]);
		f->da = ea;

		eh = (struct ether_header*)PKTDATA(osh, sf[i]);
		if ((ntoh16(eh->ether_type) == ETHER_TYPE_802_1X)) {
#if defined(BCMSPLITRX)
			if ((BCMSPLITRX_ENAB()) && i &&
				(PKTFRAGUSEDLEN(osh, sf[i]) > 0)) {
				/* Fetch susequent subframes */
				f->p = sf[i];
#if defined(BME_PKTFETCH)
				if (wlc_bme_pktfetch_recvdata(wlc, f, TRUE) != BCME_OK) {
					WL_ERROR(("wl%d: %s: pktfetch failed\n",
						wlc->pub->unit, __FUNCTION__));
					PKTFREE(osh, f->p, FALSE);
					continue;
				}

				/* Full payload is fetched to a new dongle packet and
				 * freed original packet.
				 * Continue Rx processing with new packet...
				 */
				sf[i] = f->p;
				// eh = (struct ether_header*)PKTDATA(osh, sf[i]);
#else /* ! BME_PKTFETCH */
				wlc_recvdata_schedule_pktfetch(wlc, scb, f, FALSE, TRUE, TRUE);
				continue;
#endif /* ! BME_PKTFETCH */
			}
#endif /* BCMSPLITRX */
			/* Call process EAP frames */
			if (wlc_process_eapol_frame(wlc, scb->bsscfg, scb, f, sf[i])) {
				/* We have consumed the pkt drop and continue; */
				WL_AMSDU_EX_PFX(scb, "Processed First fetched msdu %p\n",
					(void *)sf[i]);
				PKTFREE(osh, sf[i], FALSE);
				continue;
			}
		}

		f->p = sf[i];
		wlc_recvdata_sendup_msdus(wlc, scb, f);
	}

	WL_AMSDU_EX_PFX(scb, "this A-MSDU has %d MSDU, done\n", num_sf);

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	WLCNTINCR(ami->amdbg->rx_msdu_histogram[MIN((num_sf - 1), AMSDU_RX_SUBFRAMES_BINS-1)]);
	WLCNTINCR(ami->amdbg->rx_length_histogram[MIN(amsdu_bytes/AMSDU_LENGTH_BIN_BYTES,
			AMSDU_LENGTH_BINS-1)]);
#endif /* BCMDBG */

#ifdef WLSCB_HISTO
	/* Increment musdu count from 2nd sub-frame onwards
	 * as 1st sub-frame is counted as mpdu
	 */
	if (num_sf) {
#if defined(WLCFP)
		/* If CFP is enabled, amsdu already acounted, skip here */
		if (!CFP_RCB_ENAB(wlc->cfp))
#endif /* WLCFP */
		{
			WLSCB_HISTO_RX_INC_RECENT(scb, (num_sf-1));
		}
	}
#endif /* WLSCB_HISTO */

	return;

toss:
	RX_PKTDROP_COUNT(wlc, scb, toss_reason);
#ifndef WL_PKTDROP_STATS
	wlc_log_unexpected_rx_frame_log_80211hdr(wlc, f->h, toss_reason);
#endif
	/*
	 * toss the whole A-MSDU since we don't know where the error starts
	 *  e.g. a wrong subframe length for mid frame can slip through the ucode
	 *       and the only syptom may be the last MSDU frame has the mismatched length.
	 */
	for (i = 0; i < num_sf; i++)
		sf[i] = NULL;

	WL_AMSDU_EX_PFX(scb, "tossing amsdu in deagg -- error seen\n");
	PKTFREE(osh, f->p, FALSE);
} /* wlc_amsdu_deagg_hw */

#if defined(PKTC) || defined(PKTC_DONGLE)
/** Packet chaining (pktc) specific AMSDU receive function */
int32 BCMFASTPATH
wlc_amsdu_pktc_deagg_hw(amsdu_info_t *ami, void **pp, wlc_rfc_t *rfc, uint16 *index,
                        bool *chained_sendup, struct scb *scb, uint16 sec_offset, uint16 fc)
{
	osl_t *osh;
	wlc_info_t *wlc = ami->wlc;
	void *newpkt, *head, *tail, *tmp_next;
	struct ether_header *eh;
	uint16 sflen = 0, len = 0;
	uint16 num_sf = 0;
	int resid = 0;
	uint8 *da;
	struct dot11_header hdr_copy;
	char *start;
	wlc_pkttag_t * pkttag  = NULL;
	wlc_bsscfg_t *bsscfg = NULL;
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	uint16 amsdu_bytes = 0;
#endif /* BCMDBG || BCMDBG_AMSDU */
	wlc_rx_pktdrop_reason_t toss_reason; /* must be set if tossing */
	WL_MSG_MACFILTER_PFX_DECL;

	pkttag = WLPKTTAG(*pp);
	ASSERT(pkttag->flags & WLF_HWAMSDU);

	osh = ami->pub->osh;
	newpkt = tail = head = *pp;
	resid = pkttotlen(osh, head);

	/* converted frame doesnt have FCS bytes */
	if (!PKTISHDRCONVTD(osh, head))
		resid -= DOT11_FCS_LEN;

	WL_MSG_MACFILTER_PFX_INIT(scb);
	bsscfg = SCB_BSSCFG(scb);

	if (bsscfg->wlcif->if_flags & WLC_IF_PKT_80211) {
		ASSERT((sizeof(hdr_copy) + sec_offset) < (uint16)PKTHEADROOM(osh, newpkt));
		start = (char *)(PKTDATA(osh, newpkt) - sizeof(hdr_copy) - sec_offset);

		/* Save the header before being overwritten */
		bcopy((char *)start + sizeof(rx_ctxt_t), (char *)&hdr_copy, DOT11_A3_HDR_LEN);

	}

	/* insert MSDUs in to current packet chain */
	while (newpkt != NULL) {
		/* strip off FCS in last MSDU */
		if (PKTNEXT(osh, newpkt) == NULL)
		{
			PKTFRAG_TRIM_TAILBYTES(osh, newpkt,
					DOT11_FCS_LEN, TAIL_BYTES_TYPE_FCS);
		}

		/* there must be a limit to stop in order to prevent memory/stack overflow */
		if (num_sf >= MAX_RX_SUBFRAMES) {
			WL_AMSDU_ERROR(("%s: more than %d MSDUs !\n", __FUNCTION__, num_sf));
			break;
		}

		/* Frame buffer still points to the start of the receive descriptor. For each
		 * MPDU in chain, move pointer past receive descriptor.
		 */
		if ((WLPKTTAG(newpkt)->flags & WLF_HWAMSDU) == 0) {
			wlc_d11rxhdr_t *wrxh;
			uint pad;

			/* determine whether packet has 2-byte pad */
			wrxh = (wlc_d11rxhdr_t*) PKTDATA(osh, newpkt);
			pad = RXHDR_GET_PAD_LEN(&wrxh->rxhdr, wlc);

			PKTPULL(osh, newpkt, wlc->hwrxoff + pad);
			resid -= (wlc->hwrxoff + pad);
		}

		/* each subframe is 802.3 frame */
		eh = (struct ether_header *)PKTDATA(osh, newpkt);

		/* Check for AMSDU deagg attack. */
		if ((num_sf == 0) && !wlc_amsdu_validate_sf_hdr(wlc, scb, eh)) {
			WL_AMSDU_ERROR(("%s: AMSDU deagg attack; Invalid AMSDU\n", __FUNCTION__));
			WL_AMSDU_ERROR(("%s: bsscfg mode: %s SCB_A4 %d\n", __FUNCTION__,
				BSSCFG_AP(SCB_BSSCFG(scb)) ? "AP" : "STA", SCB_A4_DATA(scb)));
			prhex("Dump pkt", (uint8 *)eh, 32);
			WLCNTINCR(ami->cnt->deagg_badfmt);
			toss_reason = RX_PKTDROP_RSN_BAD_PROTO;
			goto toss;
		}

		len = (uint16)PKTLEN(osh, newpkt) + (uint16)PKTFRAGUSEDLEN(osh, newpkt);

		/* Drop all unencrypted non-eapol data frames if key is required.
		 * Take exception for NAN which accept unencrypted multicast data
		 */
		if (!wlc_amsdu_validate_sf_encryption(wlc, scb, newpkt, len, fc)) {
			WL_AMSDU_ERROR(("%s: sf body is not encrypted properly!\n", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_badfmt);
			toss_reason = RX_PKTDROP_RSN_BAD_PROTO;
			goto toss;
		}

#ifdef BCMPCIEDEV
		if (BCMPCIEDEV_ENAB() && PKTISHDRCONVTD(osh, newpkt)) {

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
			amsdu_bytes += (uint16)PKTFRAGUSEDLEN(osh, newpkt);
#endif /* BCMDBG || BCMDBG_AMSDU */

			/* Allready converted */
			goto skip_conv;
		}
#endif /* BCMPCIEDEV */

		sflen = NTOH16(eh->ether_type) + ETHER_HDR_LEN;

		if ((((uintptr)eh + (uint)ETHER_HDR_LEN) % 4) != 0) {
			WL_AMSDU_ERROR(("%s: sf body is not 4b aligned!\n", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_badsfalign);
			toss_reason = RX_PKTDROP_RSN_DEAGG_UNALIGNED;
			goto toss;
		}

		/* last MSDU: has FCS, but no pad, other MSDU: has pad, but no FCS */
		if (len != (PKTNEXT(osh, newpkt) ? ROUNDUP(sflen, 4) : sflen)) {
			WL_AMSDU_ERROR(("%s: len mismatch buflen %d sflen %d, sf %d\n",
				__FUNCTION__, len, sflen, num_sf));
			WLCNTINCR(ami->cnt->deagg_badsflen);
			toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_SF_LEN;
			goto toss;
		}

		/* strip trailing optional pad */
		if (PKTFRAGUSEDLEN(osh, newpkt)) {
			PKTSETFRAGUSEDLEN(osh, newpkt, (sflen - PKTLEN(osh, newpkt)));
		} else {
			PKTSETLEN(osh, newpkt, sflen);
		}

		if (bsscfg->wlcif->if_flags & WLC_IF_PKT_80211) {
			/* convert 802.3 to 802.11 */
			wlc_amsdu_to_dot11(ami, scb, &hdr_copy, newpkt);
			if (*pp != newpkt) {
				wlc_pkttag_t * new_pkttag = WLPKTTAG(newpkt);

				new_pkttag->rxchannel = pkttag->rxchannel;
				new_pkttag->pktinfo.misc.rssi = pkttag->pktinfo.misc.rssi;
				new_pkttag->rspec = pkttag->rspec;
			}

		} else {
			/* convert 8023hdr to ethernet if necessary */
			wlc_8023_etherhdr(wlc, osh, newpkt);
		}

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
		amsdu_bytes += (uint16)PKTLEN(osh, newpkt) +
				(uint16)PKTFRAGUSEDLEN(osh, newpkt);
#endif /* BCMDBG || BCMDBG_AMSDU */

#ifdef BCMPCIEDEV
skip_conv:
#endif /* BCMPCIEDEV */

		eh = (struct ether_header *)PKTDATA(osh, newpkt);
#ifdef PSTA
		if (BSSCFG_STA(rfc->bsscfg) &&
		    PSTA_IS_REPEATER(wlc) &&
		    TRUE)
			da = (uint8 *)&rfc->ds_ea;
		else
#endif
			da = eh->ether_dhost;

		if (ETHER_ISNULLDEST(da)) {
			toss_reason = RX_PKTDROP_RSN_BAD_MAC_DA;
			goto toss;
		}

		if (*chained_sendup) {
			/* Init DA for the first valid packet of the chain */
			if (!PKTC_HDA_VALID(wlc->pktc_info)) {
				eacopy((char*)(da), PKTC_HDA(wlc->pktc_info));
				PKTC_HDA_VALID_SET(wlc->pktc_info, TRUE);
			}

			*chained_sendup = !ETHER_ISMULTI(da) &&
				!eacmp(PKTC_HDA(wlc->pktc_info), da) &&
#if defined(PKTC_TBL)
#if defined(BCM_PKTFWD)
				wl_pktfwd_match(da, rfc->wlif_dev) &&
#else
#if !defined(WL_EAP_WLAN_ONLY_UL_PKTC)
				/* XXX: With uplink chaining controlled by rfc da and scb till
					the wlan exit, the following check is not needed for this
					feature
				*/
				PKTC_TBL_FN_CMP(wlc->pub->pktc_tbl, da, rfc->wlif_dev) &&
#endif /* !WL_EAP_WLAN_ONLY_UL_PKTC */
#endif /* BCM_PKTFWD */
#endif /* PKTC_TBL */
				((eh->ether_type == HTON16(ETHER_TYPE_IP)) ||
				(eh->ether_type == HTON16(ETHER_TYPE_IPV6)));
		}

		WL_AMSDU_EX_PFX(scb, "deagg MSDU buffer %d, frame %d\n",
		          len, sflen);

		/* remove from AMSDU chain and insert in to MPDU chain. skip
		 * the head MSDU since it is already in chain.
		 */
		tmp_next = PKTNEXT(osh, newpkt);
		if (num_sf > 0) {
			/* remove */
			PKTSETNEXT(osh, head, tmp_next);
			PKTSETNEXT(osh, newpkt, NULL);
			/* insert */
			PKTSETCLINK(newpkt, PKTCLINK(tail));
			PKTSETCLINK(tail, newpkt);
			tail = newpkt;
			/* set prio */
			PKTSETPRIO(newpkt, PKTPRIO(head));
			WLPKTTAGSCBSET(newpkt, rfc->scb);
		}

		*pp = newpkt;
		PKTCADDLEN(head, len);
		PKTSETCHAINED(osh, newpkt);

		num_sf++;
		newpkt = tmp_next;
		resid -= len;
	}

	if (resid != 0) {
		ASSERT(0);
		WLCNTINCR(ami->cnt->deagg_badtotlen);
		toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_LEN;
		goto toss;
	}

	/* toss the remaining MSDU, which we couldn't handle */
	if (newpkt != NULL) {
		WL_AMSDU_ERROR(("%s: toss MSDUs > %d !\n", __FUNCTION__, num_sf));
		PKTFREE(osh, newpkt, FALSE);
	}

	WL_AMSDU_EX_PFX(scb, "this A-MSDU has %d MSDUs, done\n", num_sf);

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	WLCNTINCR(ami->amdbg->rx_msdu_histogram[MIN((num_sf - 1), AMSDU_RX_SUBFRAMES_BINS-1)]);
	WLCNTINCR(ami->amdbg->rx_length_histogram[MIN(amsdu_bytes/AMSDU_LENGTH_BIN_BYTES,
			AMSDU_LENGTH_BINS-1)]);
#endif /* BCMDBG || BCMDBG_AMSDU */

#ifdef WLSCB_HISTO
	/* Increment musdu count from 2nd sub-frame onwards
	 * as 1st sub-frame is counted as mpdu
	 */
	if (num_sf) {
#if defined(WLCFP)
		/* If CFP is enabled, amsdu already acounted, skip here */
		if (!CFP_RCB_ENAB(wlc->cfp))
#endif /* WLCFP */
		{
			WLSCB_HISTO_RX_INC_RECENT(rfc->scb, (num_sf-1));
		}
	}
#endif /* WLSCB_HISTO */

	(*index) += num_sf;

	return BCME_OK;

toss:
	RX_PKTDROP_COUNT(wlc, scb, toss_reason);
	/*
	 * toss the whole A-MSDU since we don't know where the error starts
	 *  e.g. a wrong subframe length for mid frame can slip through the ucode
	 *       and the only syptom may be the last MSDU frame has the mismatched length.
	 */
	if (PKTNEXT(osh, head)) {
		PKTFREE(osh, PKTNEXT(osh, head), FALSE);
		PKTSETNEXT(osh, head, NULL);
	}

	if (head != tail) {
		while ((tmp_next = PKTCLINK(head)) != NULL) {
			PKTSETCLINK(head, PKTCLINK(tmp_next));
			PKTSETCLINK(tmp_next, NULL);
			PKTCLRCHAINED(osh, tmp_next);
			PKTFREE(osh, tmp_next, FALSE);
			if (tmp_next == tail) {
				/* assign *pp to head so that wlc_sendup_chain
				 * does not try to free tmp_next again
				 */
				*pp = head;
				break;
			}
		}
	}

	WL_AMSDU_EX_PFX(scb, "tossing amsdu in deagg -- error seen\n");
	return BCME_ERROR;
} /* wlc_amsdu_pktc_deagg_hw */
#endif /* PKTC */

#ifdef WLAMSDU_SWDEAGG
/**
 * A-MSDU sw deaggregation - for testing only due to lower performance to align payload.
 *
 *    | 80211 MAC HEADER | subFrame 1 | subFrame 2 | subFrame 3 | ... |
 * where, 80211 MAC header also includes WDS and/or QOS and/or IV fields
 *        f->pbody points to beginning of subFrame 1,
 *        f->body_len is the total length of all sub frames, exclude ICV and/or FCS
 *
 *        each subframe is in the form of | 8023hdr | body | pad |
 *                subframe other than the last one may have pad bytes
*/
/*
 * Note: This headroom calculation comes out to 10 byte.
 * Arithmetically, this amounts to two 4-byte blocks plus
 * 2. 2 bytes are needed anyway to achieve 4-byte alignment.
 */
#define HEADROOM  DOT11_A3_HDR_LEN-ETHER_HDR_LEN
void
wlc_amsdu_deagg_sw(amsdu_info_t *ami, struct scb *scb, struct wlc_frminfo *f)
{
	wlc_info_t *wlc = ami->wlc;
	osl_t *osh;
	struct ether_header *eh;
	struct ether_addr *ea;
	uchar *data;
	void *newpkt;
	int resid;
	uint16 body_offset, sflen, len;
	void *orig_p;
	uint16 num_sf = 0;
	WL_MSG_MACFILTER_PFX_DECL;

	BCM_REFERENCE(wlc);

	osh = ami->pub->osh;
	WL_MSG_MACFILTER_PFX_INIT(scb);

	/* all in one buffer, no chain */
	ASSERT(PKTNEXT(osh, f->p) == NULL);

	/* throw away mac header all together, start from A-MSDU body */
	body_offset = (uint)(f->pbody - (uchar*)PKTDATA(osh, f->p));
	PKTPULL(osh, f->p, body_offset);
	ASSERT(f->pbody == (uchar *)PKTDATA(osh, f->p));
	data = f->pbody;
	resid = f->totlen;
	orig_p = f->p;

	WL_AMSDU_EX_PFX(scb, "body_len(exclude ICV and FCS) %d\n", resid);

	/* loop over orig unpacking and copying frames out into new packet buffers */
	while (resid > 0) {
		if (resid < ETHER_HDR_LEN + DOT11_LLC_SNAP_HDR_LEN)
			break;

		/* each subframe is 802.3 frame */
		eh = (struct ether_header*) data;

		/* Check for AMSDU deagg attack. */
		if ((num_sf == 0) && !wlc_amsdu_validate_sf_hdr(wlc, scb, eh)) {
			WL_AMSDU_ERROR(("%s: AMSDU deagg attack; Invalid AMSDU\n", __FUNCTION__));
			WL_AMSDU_ERROR(("%s: bsscfg mode: %s SCB_A4 %d\n", __FUNCTION__,
				BSSCFG_AP(SCB_BSSCFG(scb)) ? "AP" : "STA", SCB_A4_DATA(scb)));
			prhex("Dump pkt", (uint8 *)eh, 32);
			WLCNTINCR(ami->cnt->deagg_badfmt);
			RX_PKTDROP_COUNT(wlc, scb, RX_PKTDROP_RSN_BAD_PROTO);
			goto done;
		}

		sflen = ntoh16(eh->ether_type) + ETHER_HDR_LEN;

		/* swdeagg is mainly for testing, not intended to support big buffer.
		 *  there are also the 2K hard limit for rx buffer we posted.
		 *  We can increase to 4K, but it wastes memory and A-MSDU often goes
		 *  up to 8K. HW deagg is the preferred way to handle large A-MSDU.
		 */
		if (sflen > ETHER_MAX_DATA + DOT11_LLC_SNAP_HDR_LEN + ETHER_HDR_LEN) {
			WL_AMSDU_ERROR(("%s: unexpected long pkt, toss!", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_swdeagglong);
			RX_PKTDROP_COUNT(wlc, scb, RX_PKTDROP_RSN_LONG_PKT);
			goto done;
		}

		/*
		 * Alloc new rx packet buffer, add headroom bytes to
		 * achieve 4-byte alignment and to allow for changing
		 * the hdr from 802.3 to 802.11 (EXT STA only)
		 */
		if ((newpkt = PKTGET(osh, sflen + HEADROOM, FALSE)) == NULL) {
			WL_ERROR(("wl: %s: pktget error\n", __FUNCTION__));
			WLCNTINCR(ami->pub->_cnt->rxnobuf);
			RX_PKTDROP_COUNT(wlc, scb, RX_PKTDROP_RSN_NO_BUF);
			goto done;
		}
		PKTPULL(osh, newpkt, HEADROOM);
		/* copy next frame into new rx packet buffer, pad bytes are dropped */
		bcopy(data, PKTDATA(osh, newpkt), sflen);
		PKTSETLEN(osh, newpkt, sflen);

		/* convert 8023hdr to ethernet if necessary */
		wlc_8023_etherhdr(wlc, osh, newpkt);

		ea = (struct ether_addr *) PKTDATA(osh, newpkt);

		/* transfer prio, NO need to transfer other tags, it's plain stack packet now */
		PKTSETPRIO(newpkt, f->prio);

		f->da = ea;
		f->p = newpkt;
		wlc_recvdata_sendup_msdus(wlc, scb, f);
		num_sf++;

		/* account padding bytes */
		len = ROUNDUP(sflen, 4);

		WL_AMSDU_EX_PFX(scb, "deagg one frame datalen=%d, buflen %d\n",
			sflen, len);

		data += len;
		resid -= len;

		/* last MSDU doesn't have pad, may overcount */
		if (resid < -4) {
			WL_AMSDU_ERROR(("wl: %s: error: resid %d\n", __FUNCTION__, resid));
			break;
		}
	}

done:
	/* all data are copied, free the original amsdu frame */
	PKTFREE(osh, orig_p, FALSE);
} /* wlc_amsdu_deagg_sw */
#endif /* WLAMSDU_SWDEAGG */

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
static void
wlc_amsdu_dump_ami_table(amsdu_info_t *ami, struct bcmstrbuf *b)
{
	bcm_bprintf(b, "\n");
	wlc_print_dump_table(b, "TxMSDUdens", ami->amdbg->tx_msdu_histogram,
		NULL, NULL, MAX_TX_SUBFRAMES_LIMIT, 8, TABLE_NONAME);

	wlc_print_dump_table(b, "TxAMSDU Len", ami->amdbg->tx_length_histogram,
		NULL, NULL, AMSDU_LENGTH_BINS, AMSDU_LENGTH_BINS, TABLE_LEN);

	bcm_bprintf(b, "\n");
	wlc_print_dump_table(b, "RxMSDUdens", ami->amdbg->rx_msdu_histogram,
		NULL, NULL, AMSDU_RX_SUBFRAMES_BINS, 8, TABLE_NONAME);

	wlc_print_dump_table(b, "RxAMSDU Len", ami->amdbg->rx_length_histogram,
		NULL, NULL, AMSDU_LENGTH_BINS, AMSDU_LENGTH_BINS, TABLE_LEN);
}

static int
wlc_amsdu_dump(void *ctx, struct bcmstrbuf *b)
{
	amsdu_info_t *ami = ctx;
	uint i;

	bcm_bprintf(b, "amsdu_agg_block %d amsdu_rx_mtu %d rcvfifo_limit %d amsdu_rxcap_big %d\n",
		ami->amsdu_agg_block, ami->amsdu_rx_mtu,
		ami->mac_rcvfifo_limit, ami->amsdu_rxcap_big);

	for (i = 0; i < RX_FIFO_NUMBER; i++) {
		amsdu_deagg_t *deagg = &ami->amsdu_deagg[i];
		bcm_bprintf(b, "%d amsdu_deagg_state %d\n", i, deagg->amsdu_deagg_state);
	}

	for (i = 0; i < NUMPRIO; i++) {
		bcm_bprintf(b, "%d agg_allowprio %d agg_bytes_limit %d agg_sf_limit %d",
			i, ami->txpolicy.amsdu_agg_enable[i],
			ami->txpolicy.amsdu_max_agg_bytes[i],
			ami->txpolicy.amsdu_max_sframes);
		bcm_bprintf(b, "\n");
	}

#ifdef WLCNT
	wlc_amsdu_dump_cnt(ami, b);
#endif

	wlc_amsdu_dump_ami_table(ami, b);

	return 0;
} /* wlc_amsdu_dump */

#ifdef WLCNT
void
wlc_amsdu_dump_cnt(amsdu_info_t *ami, struct bcmstrbuf *b)
{
	wlc_amsdu_cnt_t *cnt = ami->cnt;

	bcm_bprintf(b, "agg_passthrough %u\n", cnt->agg_passthrough);
	bcm_bprintf(b, "agg_amsdu %u\n", cnt->agg_amsdu);
	bcm_bprintf(b, "agg_msdu %u\n", cnt->agg_msdu);
	bcm_bprintf(b, "agg_stop_tailroom %u\n", cnt->agg_stop_tailroom);
	bcm_bprintf(b, "agg_stop_sf %u\n", cnt->agg_stop_sf);
	bcm_bprintf(b, "agg_stop_len %u\n", cnt->agg_stop_len);
	bcm_bprintf(b, "agg_stop_tcpack %u\n", cnt->agg_stop_tcpack);
	bcm_bprintf(b, "agg_stop_suppr %u\n", cnt->agg_stop_suppr);
	bcm_bprintf(b, "deagg_msdu %u\n", cnt->deagg_msdu);
	bcm_bprintf(b, "deagg_amsdu %u\n", cnt->deagg_amsdu);
	bcm_bprintf(b, "deagg_badfmt %u\n", cnt->deagg_badfmt);
	bcm_bprintf(b, "deagg_wrongseq %u\n", cnt->deagg_wrongseq);
	bcm_bprintf(b, "deagg_badsflen %u\n", cnt->deagg_badsflen);
	bcm_bprintf(b, "deagg_badsfalign %u\n", cnt->deagg_badsfalign);
	bcm_bprintf(b, "deagg_badtotlen %u\n", cnt->deagg_badtotlen);
	bcm_bprintf(b, "deagg_openfail %u\n", cnt->deagg_openfail);
	bcm_bprintf(b, "deagg_swdeagglong %u\n", cnt->deagg_swdeagglong);
	bcm_bprintf(b, "deagg_flush %u\n", cnt->deagg_flush);
	bcm_bprintf(b, "tx_padding_in_tail %u\n", cnt->tx_padding_in_tail);
	bcm_bprintf(b, "tx_padding_in_head %u\n", cnt->tx_padding_in_head);
	bcm_bprintf(b, "tx_padding_no_pad %u\n", cnt->tx_padding_no_pad);
}
#endif	/* WLCNT */

#if defined(WLAMSDU_TX) && !defined(WLAMSDU_TX_DISABLED)
static void
wlc_amsdu_tx_dump_scb(void *ctx, struct scb *scb, struct bcmstrbuf *b)
{
	amsdu_info_t *ami = (amsdu_info_t *)ctx;
	scb_amsdu_t *scb_amsdu = SCB_AMSDU_CUBBY(ami, scb);
	amsdu_scb_txpolicy_t *amsdupolicy;
	/* Not allocating as a static or const, only needed during scope of the dump */
	char ac_name[NUMPRIO][3] = {"BE", "BK", "--", "EE", "CL", "VI", "VO", "NC"};
	uint i;

	if (!AMSDU_TX_SUPPORT(ami->pub) || !scb_amsdu || !SCB_AMSDU(scb))
		return;

	bcm_bprintf(b, "AMSDU-MTU pref:%d\n", scb_amsdu->mtu_pref);
	for (i = 0; i < NUMPRIO; i++) {

		amsdupolicy = &scb_amsdu->scb_txpolicy[i];
		if (amsdupolicy->amsdu_agg_enable == FALSE) {
			continue;
		}

		/* add \t to be aligned with other scb stuff */
		bcm_bprintf(b, "\tAMSDU scb prio: %s(%d)\n", ac_name[i], i);

#if defined(WLCFP)
		bcm_bprintf(b, "\tamsdu_aggsf_max_cfp %d", scb_amsdu->amsdu_aggsf);
#endif
		bcm_bprintf(b, " amsdu_aggsf_max %d", scb_amsdu->amsdu_max_sframes);
		bcm_bprintf(b, " amsdu_agg_bytes_max %d amsdu_agg_enable %d\n",
			amsdupolicy->amsdu_agg_bytes_max, amsdupolicy->amsdu_agg_enable);

		bcm_bprintf(b, "\n");
	}
}
#endif /* WLAMSDU_TX && !WLAMSDU_TX_DISABLED */
#endif

#ifdef WLCFP
/* Return a opaque pointer to amsdu scb cubby
 * This will be store in tcb block for an easy access during fast TX path
 * members of the cubby wont be touched
 */
void*
wlc_cfp_get_amsdu_cubby(wlc_info_t *wlc, struct scb *scb)
{
	ASSERT(scb);
	ASSERT(wlc->ami);
	return (SCB_AMSDU(scb) ? SCB_AMSDU_CUBBY(wlc->ami, scb) : NULL);
}

/**
 *   CFP equivalent of wlc_recvamsdu.
 *
 *   Maintains AMSDU deagg state machine.
 *   Check if subframes are chainable.
 *   Returns NULL for intermediate subframes.
 *   Return head frame on last subframe.
 *
 *   Assumes receive status is in host byte order at this point.
 *   PKTDATA points to start of receive descriptor when called.
 *
 *   CAUTION : With CFP_REVLT80_UCODE_WAR, SCB will be NULL for corerev < 128
 */
void * BCMFASTPATH
wlc_cfp_recvamsdu(amsdu_info_t *ami, wlc_d11rxhdr_t *wrxh, void *p, bool* chainable,
	struct scb *scb)
{
	osl_t *osh;
	amsdu_deagg_t *deagg;
	uint aggtype;
	int fifo;
#ifdef BCMDBG_ASSERT
	uint16 pad;                     /* Number of bytes of pad */
#endif
	wlc_info_t *wlc = ami->wlc;
	d11rxhdr_t *rxh;
	wlc_rx_pktdrop_reason_t toss_reason; /* must be set if tossing */
	WL_MSG_MACFILTER_PFX_DECL;

#if defined(BCMSPLITRX)
	uint16 filtermap;
#endif /* BCMSPLITRX */

#if defined(BCMDBG) || defined(WLSCB_HISTO)
	int msdu_cnt = -1;              /* just used for debug */
#endif  /* BCMDBG | WLSCB_HISTO */

	osh = ami->pub->osh;

	ASSERT(chainable != NULL);

	rxh = &wrxh->rxhdr;

#if defined(DONGLEBUILD)
	aggtype = (D11RXHDR_GE129_ACCESS_VAL(rxh, mrxs) & RXSS_AGGTYPE_MASK) >>
		RXSS_AGGTYPE_SHIFT;
#else /* ! DONGLEBUILD */
	aggtype = RXHDR_GET_AGG_TYPE(rxh, wlc);
#endif /* ! DONGLEBUILD */

#ifdef BCMDBG_ASSERT
	pad = RXHDR_GET_PAD_LEN(rxh, wlc);
#endif

#if defined(BCMDBG) || defined(WLSCB_HISTO)
	msdu_cnt = RXHDR_GET_MSDU_COUNT(rxh, wlc);
#endif  /* BCMDBG | WLSCB_HISTO */

	/* Retrieve per fifo deagg info */
	fifo = (D11RXHDR_GE129_ACCESS_VAL(rxh, fifo));
	ASSERT((fifo < RX_FIFO_NUMBER));
	deagg = &ami->amsdu_deagg[fifo];

	WLCNTINCR(ami->cnt->deagg_msdu);

	WL_MSG_MACFILTER_PFX_INIT(scb);
	WL_AMSDU_EX_PFX(scb, "aggtype %d, msdu count %d\n", aggtype, msdu_cnt);

	/* Decode the aggtype */
	switch (aggtype) {
	case RXS_AMSDU_FIRST:
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_IDLE) {
			WL_AMSDU_EX_PFX(scb, "wrong A-MSDU deagg sequence, cur_state=%d\n",
				deagg->amsdu_deagg_state);
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			wlc_amsdu_deagg_flush(ami, fifo);
			/* keep this valid one and reset to improve throughput */
		}

		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_FIRST;
		deagg->amsdu_deagg_p = p;
		deagg->amsdu_deagg_ptail = p;
		deagg->chainable = TRUE;

		WL_AMSDU_EX_PFX(scb, "first A-MSDU buffer\n");
		break;

	case RXS_AMSDU_INTERMEDIATE:
		/* PKTDATA starts with subframe header */
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_FIRST) {
			WL_AMSDU_ERROR(("%s: wrong A-MSDU deagg sequence, cur_state=%d\n",
				__FUNCTION__, deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_SEQ;
			goto abort;
		}

#ifdef BCMDBG_ASSERT
		/* intermediate frames should have 2 byte padding if wlc->hwrxoff is aligned
		* on mod 4 address
		*/
		if ((wlc->hwrxoff % 4) == 0) {
			ASSERT(pad != 0);
		} else {
			ASSERT(pad == 0);
		}
#endif /* ASSERT */

#if defined(BCMSPLITRX)
		/* Do chainable checks only for non-head subframes;
		 * head frame is checked by parent fn
		 */
		/* PKTCLASS from HWA 2.a */
		/* NOTE: Only PKTCLASS_AMSDU_DA_MASK and
		 * PKTCLASS_AMSDU_SA_MASK are valid in non-first MSDU.
		 * So we don't need to check pktclass A1,A2 and FC for non-first
		 * sub frame.
		 */

		/* Filtermap from HWA 2.a */
		filtermap = ltoh16(D11RXHDR_GE129_ACCESS_VAL(rxh, filtermap16));
#if defined(BCMHWA) && defined(HWA_RXDATA_BUILD)
		filtermap = hwa_rxdata_fhr_unchainable(filtermap);
#endif

		deagg->chainable &= (filtermap == 0);
#else /* ! BCMSPLITRX */
		deagg->chainable &= TRUE;
#endif /* ! BCMSPLITRX */

		ASSERT(deagg->amsdu_deagg_ptail);
		PKTSETNEXT(osh, deagg->amsdu_deagg_ptail, p);
		deagg->amsdu_deagg_ptail = p;
		WL_AMSDU_EX_PFX(scb, "mid A-MSDU buffer\n");
		break;
	case RXS_AMSDU_LAST:
		/* PKTDATA starts with last subframe header */
		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_FIRST) {
			WL_AMSDU_ERROR(("%s: wrong A-MSDU deagg sequence, cur_state=%d\n",
				__FUNCTION__, deagg->amsdu_deagg_state));
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			toss_reason = RX_PKTDROP_RSN_BAD_DEAGG_SEQ;
			goto abort;
		}

		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_LAST;

#ifdef ASSERT
		/* last frame should have 2 byte padding if wlc->hwrxoff is aligned
		* on mod 4 address
		*/
		if ((wlc->hwrxoff % 4) == 0) {
			ASSERT(pad != 0);
		} else {
			ASSERT(pad == 0);
		}
#endif /* ASSERT */

#if defined(BCMSPLITRX)
		/* Do chainable checks only for non-head subframes;
		 * head frame is checked by parent fn
		 */

		/* PKTCLASS from HWA 2.a */
		/* NOTE: Only PKTCLASS_AMSDU_DA_MASK and
		 * PKTCLASS_AMSDU_SA_MASK are valid in non-first MSDU.
		 * So we don't need to check pktclass A1,A2 and FC for non-first
		 * sub frame.
		 */

		/* Filtermap from HWA 2.a */
		filtermap = ltoh16(D11RXHDR_GE129_ACCESS_VAL(rxh, filtermap16));
#if defined(BCMHWA) && defined(HWA_RXDATA_BUILD)
		filtermap = hwa_rxdata_fhr_unchainable(filtermap);
#endif

		deagg->chainable &= (filtermap == 0);
#else /* ! BCMSPLITRX */
		deagg->chainable &= TRUE;
#endif /* ! BCMSPLITRX */

		ASSERT(deagg->amsdu_deagg_ptail);
		PKTSETNEXT(osh, deagg->amsdu_deagg_ptail, p);
		deagg->amsdu_deagg_ptail = p;
		WL_AMSDU_EX_PFX(scb, "last A-MSDU buffer\n");
		break;

	case RXS_AMSDU_N_ONE:
		/* this frame IS AMSDU, checked by caller */

		if (deagg->amsdu_deagg_state != WLC_AMSDU_DEAGG_IDLE) {
			WL_AMSDU_EX_PFX(scb, "wrong A-MSDU deagg sequence, cur_state=%d\n",
				deagg->amsdu_deagg_state);
			WLCNTINCR(ami->cnt->deagg_wrongseq);
			wlc_amsdu_deagg_flush(ami, fifo);

			/* keep this valid one and reset to improve throughput */
		}

		ASSERT((deagg->amsdu_deagg_p == NULL) && (deagg->amsdu_deagg_ptail == NULL));
		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_LAST;
		deagg->amsdu_deagg_p = p;
		deagg->amsdu_deagg_ptail = p;
		deagg->chainable = TRUE;

		break;

	default:
		/* can't be here */
		ASSERT(0);
		toss_reason = RX_PKTDROP_RSN_BAD_AGGTYPE;
		goto abort;
	}

	/* Note that pkttotlen now includes the length of the rxh for each frag */
	WL_AMSDU_EX_PFX(scb, "add one more A-MSDU buffer  accumulated %d bytes\n",
		pkttotlen(osh, deagg->amsdu_deagg_p));

	if (deagg->amsdu_deagg_state == WLC_AMSDU_DEAGG_LAST) {
		void *pp = deagg->amsdu_deagg_p;
#if defined(WL11K)
		uint tot_len = pkttotlen(osh, pp);
#endif /* WL11K */

		deagg->amsdu_deagg_p = deagg->amsdu_deagg_ptail = NULL;
		deagg->amsdu_deagg_state = WLC_AMSDU_DEAGG_IDLE;

		*chainable = deagg->chainable;
		deagg->chainable = TRUE;

		/* ucode/hw deagg happened */
/* XXX This will ASSERT() in wlc_sendup_chain()
 * if PKTC or PKTC_DONGLE is not defined
 */
		WLPKTTAG(pp)->flags |= WLF_HWAMSDU;

		/* First frame has fully defined Receive Frame Header,
		 * handle it to normal MPDU process.
		 */
		WLCNTINCR(ami->pub->_cnt->rxfrag);
		WLCNTINCR(ami->cnt->deagg_amsdu);

#if defined(WL11K)
		WLCNTADD(ami->cnt->deagg_amsdu_bytes_l, tot_len);
		if (ami->cnt->deagg_amsdu_bytes_l < tot_len)
			WLCNTINCR(ami->cnt->deagg_amsdu_bytes_h);
#endif /* WL11K */

		deagg->first_pad = 0;
#ifdef WLSCB_HISTO
		/* At the end of AMSDU de-agg update rx counters
		 * for each AMSDU, counter is updated once
		 */
#if defined(CFP_REVLT80_UCODE_WAR)
		/* For corerev < 128, SCB stats are updated in wlc_cfp_rxframe */
		if (scb)
#endif /* CFP_REVLT80_UCODE_WAR */
		{
			WLSCB_HISTO_RX(scb, (WLPKTTAG(p))->rspec, msdu_cnt);
		}
#endif /* WLSCB_HISTO */
		return (pp);
	}

	/* all other cases needs no more action, just return */
	return  NULL;
abort:
	ASSERT(deagg->amsdu_deagg_ptail);
	ASSERT(PKTNEXT(osh, deagg->amsdu_deagg_ptail) == NULL);
	PKTSETNEXT(osh, deagg->amsdu_deagg_ptail, NULL);
	wlc_amsdu_deagg_flush(ami, fifo);
	RX_PKTDROP_COUNT(wlc, scb, toss_reason);
	PKTFREE(osh, p, FALSE);
	return  NULL;
} /* wlc_cfp_recvamsdu */

#if !defined(BCMDBG) && !defined(BCMDBG_AMSDU)
/* Wrapper to update ampsdu counter at the end of entire release loop
 * This routine doesn't update histogram. So should be called when
 * debug option is not enabled
 */
void
wlc_cfp_amsdu_tx_counter_upd(wlc_info_t *wlc, uint32 tot_nmsdu, uint32 tot_namsdu)
{
	amsdu_info_t *ami = wlc->ami;

	WLCNTADD(ami->cnt->agg_amsdu, tot_namsdu);
	WLCNTADD(ami->cnt->agg_msdu, tot_nmsdu);
}
#endif /* !BCMDBG && !BCMDBG_AMSDU */

/* amsdu counter update for CFP Tx path. most of the counters
 * do not make sense for CFP amsdu path. Updating only few
 */
void
wlc_cfp_amsdu_tx_counter_histogram_upd(wlc_info_t *wlc, uint32 nmsdu, uint32 totlen)
{
	amsdu_info_t *ami = wlc->ami;

	WLCNTINCR(ami->cnt->agg_amsdu);
	WLCNTADD(ami->cnt->agg_msdu, nmsdu);

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	/* update statistics histograms */
	ami->amdbg->tx_msdu_histogram[(nmsdu - 1)]++;
	ami->amdbg->tx_length_histogram[MIN(totlen / AMSDU_LENGTH_BIN_BYTES,
		AMSDU_LENGTH_BINS-1)]++;
#endif /* BCMDBG || BCMDBG_AMSDU */
}

#if defined(DONGLEBUILD)
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
void
wlc_cfp_amsdu_rx_histogram_upd(wlc_info_t *wlc, uint16 msdu_count, uint16 amsdu_bytes)
{
	amsdu_info_t *ami = wlc->ami;

	WLCNTINCR(ami->amdbg->rx_msdu_histogram[MIN((msdu_count - 1), AMSDU_RX_SUBFRAMES_BINS-1)]);
	WLCNTINCR(ami->amdbg->rx_length_histogram[MIN(amsdu_bytes/AMSDU_LENGTH_BIN_BYTES,
			AMSDU_LENGTH_BINS-1)]);
}
#endif /* BCMDBG || BCMDBG_AMSDU */

#else /* ! DONGLEBUILD */
/**
 *   CFP equivalent of wlc_amsdu_pktc_deagg_hw.
 *
 *   Called when the 'WLF_HWAMSDU' flag is set in the PKTTAG of a received frame.
 *   A-MSDU decomposition:
 *
 *    | 80211 MAC HEADER | subFrame 1 |
 *			               --> | subFrame 2 |
 *			                                 --> | subFrame 3... |
 *   each subframe is in the form of | 8023hdr | body | pad |
 *   subframe other than the last one may have pad bytes
 *
 *   Convert each subframe from 8023hdr to ethernet form
 */
int32 BCMFASTPATH
wlc_cfp_amsdu_deagg_hw(amsdu_info_t *ami, void *p, uint32 *index, struct scb *scb, uint16 fc)
{
	osl_t			* osh;
	wlc_info_t		* wlc = ami->wlc;
	void			* newpkt;
	void			* prev_pkt;
	struct ether_header	* eh;
	int			rem_len;
	uint16			sflen = 0;
	uint16			len = 0;
	uint16			num_sf = 0;
#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	uint16			amsdu_bytes = 0;
#endif /* BCMDBG || BCMDBG_AMSDU */
	WL_MSG_MACFILTER_PFX_DECL;

	WL_MSG_MACFILTER_PFX_INIT(scb);
	osh = ami->pub->osh;
	newpkt = p;
	rem_len = pkttotlen(osh, p) - DOT11_FCS_LEN;

	while (newpkt != NULL) {
		/* strip off FCS in last MSDU */
		if (PKTNEXT(osh, newpkt) == NULL) {
			PKTFRAG_TRIM_TAILBYTES(osh, newpkt, DOT11_FCS_LEN, TAIL_BYTES_TYPE_FCS);
		}

		/* there must be a limit to stop to prevent stack overflow */
		if (num_sf >= MAX_RX_SUBFRAMES) {
			WL_AMSDU_ERROR(("%s: more than %d MSDUs !\n", __FUNCTION__, num_sf));
			break;
		}

		/* Frame buffer still points to the start of the receive descriptor.
		 * For each MPDU in chain, move pointer past receive descriptor.
		 */
		if ((WLPKTTAG(newpkt)->flags & WLF_HWAMSDU) == 0) {
			wlc_d11rxhdr_t *wrxh;
			uint pad;

			/* determine whether packet has 2-byte pad */
			wrxh = (wlc_d11rxhdr_t*) PKTDATA(osh, newpkt);
			pad = RXHDR_GET_PAD_LEN(&wrxh->rxhdr, wlc);

			PKTPULL(osh, newpkt, wlc->hwrxoff + pad);
			rem_len -= (wlc->hwrxoff + pad);
		}

		/* each subframe is 802.3 frame */
		eh = (struct ether_header*) PKTDATA(osh, newpkt);

		/* Check for AMSDU deagg attack. */
		if ((num_sf == 0) && !wlc_amsdu_validate_sf_hdr(wlc, scb, eh)) {
			WL_AMSDU_ERROR(("%s: AMSDU deagg attack; Invalid AMSDU\n", __FUNCTION__));
			WL_AMSDU_ERROR(("%s: bsscfg mode: %s SCB_A4 %d\n", __FUNCTION__,
				BSSCFG_AP(SCB_BSSCFG(scb)) ? "AP" : "STA", SCB_A4_DATA(scb)));
			prhex("Dump pkt", (uint8 *)eh, 32);
			WLCNTINCR(ami->cnt->deagg_badfmt);
			goto abort;
		}

		len = (uint16)PKTLEN(osh, newpkt);

		/* Drop all unencrypted non-eapol data frames if key is required.
		 * Take exception for NAN which accept unencrypted multicast data
		 */
		if (!wlc_amsdu_validate_sf_encryption(wlc, scb, newpkt, len, fc)) {
			WL_AMSDU_ERROR(("%s: sf body is not encrypted properly!\n", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_badfmt);
			goto abort;
		}

#ifdef BCMPCIEDEV
		ASSERT(!PKTISHDRCONVTD(osh, newpkt));
#endif /* BCMPCIEDEV */

		sflen = NTOH16(eh->ether_type) + ETHER_HDR_LEN;

		if ((((uintptr)eh + (uint)ETHER_HDR_LEN) % 4) != 0) {
			WL_AMSDU_ERROR(("%s: sf body is not 4b aligned!\n", __FUNCTION__));
			WLCNTINCR(ami->cnt->deagg_badsfalign);
			goto abort;
		}

		/* last MSDU: has FCS, but no pad, other MSDU: has pad, but no FCS */
		if (len != (PKTNEXT(osh, newpkt) ? ROUNDUP(sflen, 4) : sflen)) {
			WL_AMSDU_ERROR(("%s: len mismatch buflen %d sflen %d, sf %d\n",
				__FUNCTION__, len, sflen, num_sf));
			WLCNTINCR(ami->cnt->deagg_badsflen);
			goto abort;
		}

		PKTSETLEN(osh, newpkt, sflen);

		/* convert 8023hdr to ethernet if necessary */
		wlc_8023_etherhdr(wlc, osh, newpkt);
		WL_AMSDU_EX_PFX(scb, "deagg MSDU buffer %d, frame %d\n", len, sflen);

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
		amsdu_bytes += PKTLEN(osh, newpkt);
#endif /* BCMDBG || BCMDBG_AMSDU */

		num_sf++;
		rem_len -= len;
		prev_pkt = newpkt;
		newpkt = PKTNEXT(osh, newpkt);
	}

	if (rem_len != 0) {
		ASSERT(0);
		WLCNTINCR(ami->cnt->deagg_badtotlen);
		goto abort;
	}

	/* toss the remaining MSDU, which we couldn't handle */
	if (newpkt != NULL) {
		WL_AMSDU_ERROR(("%s: toss MSDUs > %d !\n", __FUNCTION__, num_sf));
		PKTSETNEXT(osh, prev_pkt, NULL);
		PKTFREE(osh, newpkt, FALSE);
	}

	WL_AMSDU_EX_PFX(scb, "this A-MSDU has %d MSDUs, done\n", num_sf);

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	WLCNTINCR(ami->amdbg->rx_msdu_histogram[MIN((num_sf - 1), AMSDU_RX_SUBFRAMES_BINS-1)]);
	WLCNTINCR(ami->amdbg->rx_length_histogram[MIN(amsdu_bytes/AMSDU_LENGTH_BIN_BYTES,
			AMSDU_LENGTH_BINS-1)]);
#endif /* BCMDBG || BCMDBG_AMSDU */

	(*index) += num_sf;
	return BCME_OK;

abort:
	/* Packet had to be freed by Caller */
	return BCME_ERROR;
}
#endif /* ! DONGLEBUILD */

#endif /* WLCFP */

/* This function is used when AMSDU agg is done at release time.
 *
 * Subframes are pulled out from AMPDU or NAR prec queues.
 * If can't fit in, should make sure its enqueued back to the prec queues.
 */
uint8 BCMFASTPATH
wlc_amsdu_agg_attempt(amsdu_info_t *ami, struct scb *scb, struct pktq *pktq, void *p,
	uint8 prec, uint8 tid)
{
	osl_t *osh = ami->pub->osh;
	wlc_info_t *wlc = ami->wlc;
	int32 pad, lastpad = 0;
	uint8 i = 0;
	uint32 totlen = 0;
	scb_amsdu_t *scb_ami;
	void *p1 = NULL;
	bool pad_at_head = FALSE;
	void *n, *pkt_last;
#if defined(BCMHWA)
	HWA_PKTPGR_EXPR(void *head = p);
#endif

	BCM_REFERENCE(wlc);

	ASSERT(!(WLF2_PCB1(p) & WLF2_PCB1_NAR));
	ASSERT(!(WLPKTTAG(p)->flags & WLF_AMSDU));

#ifdef BCMDBG
	if (ami->amsdu_agg_block || !wlc->pub->_amsdu_tx) {
		WLCNTINCR(ami->cnt->agg_passthrough);
		return 0;
	}
#endif /* BCMDBG */

#if defined(BCMHWA)
	/* PKTPGR cannot do AMSDU in fragment packet.
	 * SW WAR: In PKTPGR no AMSDU for fragments.
	 */
	HWA_PKTPGR_EXPR({
		if (PKTNEXT(osh, p)) {
			return 0;
		}
	});
#endif

#if defined(BCM_DHDHDR) && defined(DONGLEBUILD)
	/* Skip AMSDU agg for PKTFETCHED packet */
	if (PKTISPKTFETCHED(osh, p))
		return 0;
#endif

	/* Get the next packet from ampdu/nar precedence Queue */
	n = pktqprec_peek(pktq, prec);

	/* Return if no packets available to do AMSDU agg */
	if (n == NULL) {
		return 0;
	}

	pad = 0;
	i = 1;
	scb_ami = SCB_AMSDU_CUBBY(ami, scb);
	totlen = pkttotlen(osh, p);

#ifdef WLAMSDU_TX
	/* If rate changed, driver need to recalculate
	 * the max aggregation length for vht and he rate.
	 */
	if (scb_ami->agglimit_upd) {
		wlc_amsdu_tx_scb_agglimit_upd(ami, scb);
		scb_ami->agglimit_upd = FALSE;
	}
#endif /* WLAMSDU_TX */

	/* msdu's in packet chain 'n' are aggregated while in this loop */
	while (n != NULL) {
#if defined(BCMHWA)
		/* PKTPGR cannot do AMSDU in fragment packet.
		 * SW WAR: In PKTPGR no AMSDU for fragments.
		 * PKTPGR will fixup AMSDU in single TxLfrag.
		 * It expect all msdu should have the same flowid.
		 * So don't do amsdu agg if flowid of peek_pkt is different.
		 */
		HWA_PKTPGR_EXPR({
			if (PKTNEXT(wlc->osh, n) || (PKTFRAGFLOWRINGID(PKT_OSH_NA, p) !=
				PKTFRAGFLOWRINGID(PKT_OSH_NA, n))) {
				break;
			}
		});
#endif /* BCMHWA */

#ifdef WLOVERTHRUSTER
		if (OVERTHRUST_ENAB(wlc->pub)) {
			/* Check if the next subframe is possibly TCP ACK */
			if (pkttotlen(osh, n) <= TCPACKSZSDU) {
				WLCNTINCR(ami->cnt->agg_stop_tcpack);
				break;
			}
		}
#endif /* WLOVERTHRUSTER */

#if defined(BCM_DHDHDR) && defined(DONGLEBUILD)
		/* Skip AMSDU agg for PKTFETCHED packet */
		if (PKTISPKTFETCHED(osh, n))
			break;
#endif

		if (i >= scb_ami->amsdu_max_sframes) {
			WLCNTINCR(ami->cnt->agg_stop_sf);
			break;
		} else if ((totlen + pkttotlen(osh, n))
			>= scb_ami->scb_txpolicy[tid].amsdu_agg_bytes_max) {
			WLCNTINCR(ami->cnt->agg_stop_len);
			break;
		}
#if defined(PROP_TXSTATUS) && defined(BCMPCIEDEV)
		else if (BCMPCIEDEV_ENAB() && PROP_TXSTATUS_ENAB(wlc->pub) &&
			WLFC_GET_REUSESEQ(wlfc_query_mode(wlc->wlfc))) {
			/*
			 * This comparison does the following:
			 *  - For suppressed pkts in AMSDU, the mask should be same
			 *      so re-aggregate them
			 *  - For new pkts, pkttag->seq is zero so same
			 *  - Prevents suppressed pkt from being aggregated with non-suppressed pkt
			 *  - Prevents suppressed MPDU from being aggregated with suppressed MSDU
			 */
			if ((WLPKTTAG(p)->seq & WL_SEQ_AMSDU_SUPPR_MASK) !=
				(WLPKTTAG(n)->seq & WL_SEQ_AMSDU_SUPPR_MASK)) {
				WLCNTINCR(ami->cnt->agg_stop_suppr);
				break;
			}
		}
#endif /* PROP_TXSTATUS && BCMPCIEDEV */

		/* Padding of A-MSDU sub-frame to 4 bytes */
		pad = (uint)((-(int)(pkttotlen(osh, p) - lastpad)) & 3);

		if (i == 1) {
			/* For first msdu init ether header (amsdu header) */
#if defined(BCM_DHDHDR) && defined(DONGLEBUILD)
			if (PKTISTXFRAG(osh, p)) {
				/* When BCM_DHDHDR enabled the DHD host driver will prepare the
				 * dot3_mac_llc_snap_header, now we adjust ETHER_HDR_LEN bytes
				 * in host data addr to include the ether header 14B part.
				 * The ether header 14B in dongle D3_BUFFER is going to be used as
				 * amsdu header.
				 * Now, first msdu has all DHDHDR 22B in host.
				 */
				PKTSETFRAGDATA_LO(osh, p, LB_FRAG1,
					PKTFRAGDATA_LO(osh, p, LB_FRAG1) - ETHER_HDR_LEN);
				PKTSETFRAGLEN(osh, p, LB_FRAG1,
					PKTFRAGLEN(osh, p, LB_FRAG1) + ETHER_HDR_LEN);
				PKTSETFRAGTOTLEN(osh, p,
					PKTFRAGTOTLEN(osh, p) + ETHER_HDR_LEN);
			}
#endif /* BCM_DHDHDR && DONGLEBUILD */
			WLPKTTAG(p)->flags |= WLF_AMSDU;
		}

#if defined(BCM_DHDHDR) && defined(DONGLEBUILD)
		/* For second msdu and later */
		if (PKTISTXFRAG(osh, n)) {
			/* Before we free the second msdu D3_BUFFER we have to include the
			 * ether header 14B part in host.
			 * Now, second msdu has all DHDHDR 22B in host.
			 */
			PKTSETFRAGDATA_LO(osh, n, LB_FRAG1,
				PKTFRAGDATA_LO(osh, n, LB_FRAG1) - ETHER_HDR_LEN);
			PKTSETFRAGLEN(osh, n, LB_FRAG1,
				PKTFRAGLEN(osh, n, LB_FRAG1) + ETHER_HDR_LEN);
			PKTSETFRAGTOTLEN(osh, n,
				PKTFRAGTOTLEN(osh, n) + ETHER_HDR_LEN);

			/* Free the second msdu D3_BUFFER, we don't need it */
			PKTBUFEARLYFREE(osh, n);
		}
#endif /* BCM_DHDHDR && DONGLEBUILD */

		/* Add padding to next pkt (at headroom) if current is a lfrag */
		if (BCMLFRAG_ENAB() && PKTISTXFRAG(osh, p)) {
			/*
			 * Store the padding value. We need this to be accounted for, while
			 * calculating the padding for the subsequent packet.
			 * For example, if we need padding of 3 bytes for the first packet,
			 * we add those 3 bytes padding in the head of second packet. Now,
			 * to check if padding is required for the second packet, we should
			 * calculate the padding without considering these 3 bytes that
			 * we have already put in.
			 */
			lastpad = pad;

			if (pad) {
#if defined(BCM_DHDHDR) && defined(DONGLEBUILD)
				/* pad data will be the garbage at the end of host data */
				PKTSETFRAGLEN(osh, p, LB_FRAG1,
					PKTFRAGLEN(osh, p, LB_FRAG1) + pad);
				PKTSETFRAGTOTLEN(osh, p, PKTFRAGTOTLEN(osh, p) + pad);
				totlen += pad;
				lastpad = 0;
#else
				/*
				 * Let's just mark that padding is required at the head of next
				 * packet. Actual padding needs to be done after the next sdu
				 * header has been copied from the current sdu.
				 */
				pad_at_head = TRUE;

#ifdef WLCNT
				WLCNTINCR(ami->cnt->tx_padding_in_head);
#endif /* WLCNT */
#endif /* BCM_DHDHDR && DONGLEBUILD */
			}
		} else if (pad) {
			pkt_last = pktlast(osh, p);
			if (PKTTAILROOM(osh, pkt_last) < pad) {
#ifdef WLCNT
				WLCNTINCR(ami->cnt->agg_stop_tailroom);
#endif /* WLCNT */
				break;
			}
			PKTSETLEN(osh, pkt_last, PKTLEN(osh, pkt_last) + pad);
			totlen += pad;
#ifdef WLCNT
			WLCNTINCR(ami->cnt->tx_padding_in_tail);
#endif /* WLCNT */
		}

#ifdef WLCNT
		if (pad == 0) {
			WLCNTINCR(ami->cnt->tx_padding_no_pad);
		}
#endif /* WLCNT */

		/* Consumes MSDU from head of chain 'n' and append this MSDU to chain 'p' */
		p1 = pktq_pdeq(pktq, prec);
		ASSERT(n == p1);

		PKTSETNEXT(osh, pktlast(osh, p), p1);

		totlen += pkttotlen(osh, p1);
		i++;

		if (pad_at_head == TRUE) {
			/* If padding was required for the previous packet, apply it here */
			PKTPUSH(osh, p1, pad);
			totlen += pad;
			pad_at_head = FALSE;
		}

		p = n;
		/* Peek the next packet on the ampdu/nar precedence queue */
		n = pktqprec_peek(pktq, prec);
	} /* while */

	WLCNTINCR(ami->cnt->agg_amsdu);
	WLCNTADD(ami->cnt->agg_msdu, i);

#ifdef WL11K
	WLCNTADD(ami->cnt->agg_amsdu_bytes_l, totlen);
	if (ami->cnt->agg_amsdu_bytes_l < totlen)
		WLCNTINCR(ami->cnt->agg_amsdu_bytes_h);
#endif

#if defined(BCMDBG) || defined(BCMDBG_AMSDU)
	/* update statistics histograms */
	ami->amdbg->tx_msdu_histogram[MIN(i-1,
		MAX_TX_SUBFRAMES_LIMIT-1)]++;
	ami->amdbg->tx_length_histogram[MIN(totlen / AMSDU_LENGTH_BIN_BYTES,
		AMSDU_LENGTH_BINS-1)]++;
#endif /* BCMDBG || BCMDBG_AMSDU */

#if defined(BCMHWA)
	/* Fixup AMSDU in single TxLfrag */
	HWA_PKTPGR_EXPR(wlc_amsdu_single_txlfrag_fixup(wlc, head));
#endif /* BCMHWA */

	return i - 1;
} /* wlc_amsdu_agg_attempt */

#ifdef	WLCFP
#if defined(BCM_DHDHDR) && defined(DONGLEBUILD)
/* With DHDHDR enabled, amsdu subframe header is fully formed at host.
 * So update host frag length and start address to account for
 * ETHER_HDR_LEN bytes length in host.
 *
 */
void
wlc_amsdu_subframe_insert(wlc_info_t* wlc, void* head)
{
	uint16 cnt = 0;
	void* pkt = head;

	while (pkt) {
		ASSERT(PKTISTXFRAG(wlc->osh, pkt));
		PKTSETFRAGDATA_LO(wlc->osh, pkt, LB_FRAG1,
			PKTFRAGDATA_LO(wlc->osh, pkt, LB_FRAG1) - ETHER_HDR_LEN);
		PKTSETFRAGLEN(wlc->osh, pkt, LB_FRAG1,
			PKTFRAGLEN(wlc->osh, pkt, LB_FRAG1) + ETHER_HDR_LEN);
		PKTSETFRAGTOTLEN(wlc->osh, pkt,
			PKTFRAGTOTLEN(wlc->osh, pkt) + ETHER_HDR_LEN);

		if (cnt != 0) {
			/* Free the D3_BUFFER for non head subframe
			 * D11 headers are required only for head subframe.
			 * Free up local storage for all other subframes.
			 */
			PKTBUFEARLYFREE(wlc->osh, pkt);
		}

		cnt++;
		pkt = PKTNEXT(wlc->osh, pkt);
	}
}
#endif /* BCM_DHDHDR && DONGLEBUILD */
#endif /* WLCFP */
