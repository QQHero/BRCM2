/*
 * MBSS module
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
 * $Id: wlc_mbss.c 810655 2022-04-12 06:08:44Z $
 */

/**
 * @file
 * @brief
 * Twiki: [MBSSIDSupport]
 */

#include "wlc_cfg.h"

#ifdef MBSS

#include <osl.h>
#include <wlioctl.h>
#include <siutils.h>
#include <bcmutils.h>
#include <bcmendian.h>

#include "wlc_rate.h"
#include "wl_dbg.h"
#include "wlc.h"
#include "wlc_mbss.h"
#include "wlc_pub.h"
#include "wlc_dbg.h"
#include "wlc_bsscfg.h"
#include "wlc_bmac.h"
#include "wlc_ap.h"
#include "wlc_tx.h"
#include "wlc_tso.h"
#include "wlc_scan.h"
#include "wlc_apps.h"
#include "wlc_stf.h"
#include <wlc_hw.h>
#include <wlc_rspec.h>
#include "wlc_pcb.h"
#include "wlc_dump.h"
#include "wlc_ie_mgmt.h"
#include "wlc_ie_mgmt_types.h"
#include "wlc_fbt.h"
#include <wlc_he.h>
#include <wlc_rrm.h>
#include <wlc_akm_ie.h>
#include <wlc_modesw.h>
#define WLC_MBSS_BSSCFG_IDX_INVALID	-1

/* forward function declarations */
static int wlc_mbss_doiovar(void *hdl, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif);
static int wlc_mbss_up(void *hdl);
static int wlc_spt_init(wlc_info_t *wlc, wlc_spt_t *spt, int count, int len);
static void wlc_spt_deinit(wlc_info_t *wlc, wlc_spt_t *spt, int pkt_free_force);
static void mbss_ucode_set(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
static void wlc_mbss16_write_beacon(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
static void wlc_mbss16_write_prbrsp(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool suspend);
static void wlc_mbss16_setup(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
static void wlc_mbss16_updssid_len(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
static void bcmc_fid_shm_commit(wlc_bsscfg_t *bsscfg);
static void wlc_mbss16_updssid(wlc_info_t *wlc, wlc_bsscfg_t *cfg);
static void wlc_bsscfg_disablemulti(wlc_info_t *wlc);
static void wlc_mbss_bcmc_free_cb(wlc_info_t *wlc, void *pkt, uint txs);
#ifdef WL_MBSSID
static uint wlc_mbss_calc_mbssid_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_mbss_write_mbssid_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_mbss_calc_mbssid_ie_len_all(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_mbss_write_mbssid_ie_all(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_calc_max_bss_indicator(uint number);
static uint wlc_mbssid_per_mbss_ie_calc(void *ctx, wlc_iem_cbparm_t *cbparm,
	wlc_iem_calc_data_t *data, uint16 *tlv_len);
static uint wlc_mbssid_calc_append_subelement(void *ctx, wlc_iem_calc_data_t *data,
	wlc_iem_tag_t tag, uint16 *tlv_len);
static uint wlc_mbss_calc_nontrans_mbssid_subelement_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static uint wlc_mbss_calc_mbssid_ie_len_local(void *ctx, wlc_iem_calc_data_t *data);
static uint wlc_mbss_calc_nontrans_bssid_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static uint wlc_mbss_calc_multibssid_index_ie_len(wlc_iem_calc_data_t *data);
static int wlc_mbssid_per_mbss_ie(void *ctx, wlc_iem_build_data_t *data,
	wlc_iem_calc_data_t *calc_data, uint16 *tlv_len);
static int wlc_mbssid_append_subelement(void *ctx, wlc_iem_build_data_t *data,
	wlc_iem_calc_data_t *calc_data, wlc_iem_tag_t tag,
	uint16 *tlv_len, uint16 *per_mbss_len);
static int wlc_mbss_update_add_mbssie_and_subelement(void *ctx, wlc_iem_build_data_t *data,
	wlc_iem_calc_data_t *calc_data, uint16 *tlv_len, uint16 *per_mbss_len);
static uint wlc_mbss_build_nontrans_bssid_subelement_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_mbss_build_nontrans_bssid_cap_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_mbss_build_multibssid_index_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_mbss_write_mbssid_ie_local(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_mbss_calc_multibssid_noninheritance_ie(void *ctx, wlc_iem_calc_data_t *data);
static uint wlc_mbss_build_multibssid_noninheritance_ie(void *ctx, wlc_iem_build_data_t *data);
static uint wlc_mbss_calc_multibssid_configuration_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_mbss_write_mbssid_configuration_ie(void *ctx, wlc_iem_build_data_t *data);
#endif /* WL_MBSSID */

/* MBSS wlc fields */
struct wlc_mbss_info {
	wlc_info_t	*wlc;			/* pointer to main wlc structure */
	int		cfgh;			/* bsscfg cubby handle */
	struct ether_addr vether_base;		/* Base virtual MAC addr when user
						 * doesn't provide one
						 */
	int8		hw2sw_idx[WLC_MAXBSSCFG]; /* Map from uCode index to software index */
	uint32		last_tbtt_us;		/* Timestamp of TBTT time */
	int8		beacon_bssidx;		/* Track start config to rotate order of beacons */
	int		bcast_next_start;	/* For rotating probe responses to bcast requests */
	uint16		pervap_cck_bitmap;
	int16		mbss16_beacon_count;	/* Number of beacons configured in last tbtt */
#ifdef WL_MBSSID
	int16           max_bss_indicator;
#endif /* WL_MBSSID */
	uint32		skip_bcntpl_upd;	/* counter to track if bcn tpl skipped
						 * as close to tbtt
						 */
};

/* MBSS debug counters */
typedef struct wlc_mbss_cnt {
	uint32		prb_resp_alloc_fail;  /* Failed allocation on probe response */
	uint32		prb_resp_tx_fail;     /* Failed to TX probe response */
	uint32		prb_resp_retrx_fail;  /* Failed to TX probe response */
	uint32		prb_resp_retrx;       /* Retransmit suppressed probe resp */
	uint32		prb_resp_ttl_expy;    /* Probe responses suppressed due to TTL expry */
	uint32		bcn_tx_failed;	      /* Failed to TX beacon frame */

	uint32		mc_fifo_max;          /* Max number of BC/MC pkts in DMA queues */
	uint32		bcmc_count;           /* Total number of pkts sent thru BC/MC fifo */

	/* Timing and other measurements for PS transitions */
	uint32		ps_trans;	      /* Total number of transitions started */
} wlc_mbss_cnt_t;

/* bsscfg cubby */
typedef struct {
	wlc_pkt_t	probe_template;	/* Probe response master packet, including PLCP */
	bool		prb_modified;	/* Ucode version: push to shm if true */
	wlc_spt_t	*bcn_template;	/* Beacon DMA template */
	int8		_ucidx;		/* the uCode index of this bsscfg,
					 * assigned at wlc_bsscfg_up()
					 */
	uint32		mc_fifo_pkts;	/* Current number of BC/MC pkts sent to DMA queues */
	uint32		prb_ttl_us;     /* Probe rsp time to live since req. If 0, disabled */
	wlc_mbss_cnt_t  *cnt;		/* MBSS debug counters */
	uint16		capability;	/* MBSSID copy of capability field */
	uint16		bcn_len;	/* beacon length programmed by host driver */
} bss_mbss_info_t;

static int wlc_mbss_info_init(void *hdl, wlc_bsscfg_t *cfg);
static void wlc_mbss_info_deinit(void *hdl, wlc_bsscfg_t *cfg);
#if defined(BCMDBG)
static void wlc_mbss_bsscfg_dump(void *hdl, wlc_bsscfg_t *cfg, struct bcmstrbuf *b);
static int wlc_mbss_dump(void *ctx, struct bcmstrbuf *b);
#endif

/* bsscfg specific info access accessor */
#define MBSS_BSSCFG_CUBBY_LOC(mbss, cfg) ((bss_mbss_info_t **)BSSCFG_CUBBY((cfg), (mbss)->cfgh))
#define MBSS_BSSCFG_CUBBY(mbss, cfg)	 (*(MBSS_BSSCFG_CUBBY_LOC(mbss, cfg)))
#define BSS_MBSS_INFO(mbss, cfg)	 MBSS_BSSCFG_CUBBY(mbss, cfg)

#define EADDR_TO_UC_IDX(eaddr, mask)	((eaddr).octet[5] & (mask))

#define BCN0_TPL_BASE_ADDR	(D11REV_GE(wlc->pub->corerev, 40) ? \
				 D11AC_T_BCN0_TPL_BASE : D11_T_BCN0_TPL_BASE)

#define SHM_MBSS_WORD_OFFSET_TO_ADDR(x, n)	(M_MBSSID_BLK(x) + ((n) * 2))

#define SHM_MBSS_BC_FID0(x)	SHM_MBSS_WORD_OFFSET_TO_ADDR(x, 5)
#define SHM_MBSS_BC_FID1(x)	SHM_MBSS_WORD_OFFSET_TO_ADDR(x, 6)
#define SHM_MBSS_BC_FID2(x)	SHM_MBSS_WORD_OFFSET_TO_ADDR(x, 7)
#define SHM_MBSS_BC_FID3(x)	SHM_MBSS_WORD_OFFSET_TO_ADDR(x, 8)

/* SSID lengths are encoded, two at a time in 16-bits */
#define SHM_MBSS_SSID_LEN0(x)	SHM_MBSS_WORD_OFFSET_TO_ADDR(x, 10)
#define SHM_MBSS_SSID_LEN1(x)	SHM_MBSS_WORD_OFFSET_TO_ADDR(x, 11)

/* New for ucode template based mbss */

/* Uses uCode (HW) BSS config IDX */
#define SHM_MBSS_SSID_ADDR(x, idx)	((idx) == 0 ? M_SSID(x) : \
		(M_MBS_SSID_1(x) + (0x10 * (idx - 1))))

/* Uses uCode (HW) BSS config IDX */
#define SHM_MBSS_BC_FID_ADDR16(x, ucidx)	(M_MBS_BCFID_BLK(x) + (2 * ucidx))

#define MBSS_PRS_BLKS_START(wlc, n)   (BCN0_TPL_BASE_ADDR +	\
				       ((n) *	\
				        (D11REV_GE((wlc)->pub->corerev, 40) ? \
				         (wlc)->pub->bcn_tmpl_len :	\
				         TPLBLKS_PER_BCN((wlc)->pub->corerev) * \
				         TXFIFO_SIZE_UNIT((wlc)->pub->corerev))))

/*
 * Conversion between HW and SW BSS indexes.  HW is (currently) based on lower
 * bits of BSSID/MAC address.  SW is based on allocation function.
 * BSS does not need to be up, so caller should check if required.  No error checking.
 */
#define WLC_BSSCFG_HW2SW_IDX(wlc, mbss, ucidx)	\
	(MBSS_SUPPORT((wlc)->pub) ? (int)(mbss)->hw2sw_idx[ucidx] : 0)

/* used for extracting ucidx from macaddr */
#define WLC_MBSS_UCIDX_MASK(d11corerev)	(WLC_MAX_AP_BSS(d11corerev) - 1)

/*
 * Software packet template (spt) structure; for beacons and (maybe) probe responses.
 */
#define WLC_SPT_COUNT_MAX 2

/* Turn on define to get stats on SPT */
/* #define WLC_SPT_DEBUG */

struct wlc_spt
{
	uint32		in_use_bitmap;	/* Bitmap of entries in use by DMA */
	wlc_pkt_t	pkts[WLC_SPT_COUNT_MAX];	/* Pointer to array of pkts */
	int		latest_idx;	/* Most recently updated entry */
	uint8		*tim_ie;	/* Pointer to start of TIM IE in current packet */
	bool		bcn_modified;	/* Ucode versions, TRUE: push out to shmem */

#if defined(WLC_SPT_DEBUG)
	uint32		tx_count;	/* Debug: Number of times tx'd */
	uint32		suppressed;	/* Debug: Number of times supressed */
#endif /* WLC_SPT_DEBUG */
};

/* In the case of 2 templates, return index of one not in use; -1 if both in use */
#define SPT_PAIR_AVAIL(spt) \
	(((spt)->in_use_bitmap == 0) || ((spt)->in_use_bitmap == 0x2) ? 0 : \
	((spt)->in_use_bitmap == 0x1) ? 1 : -1)

/* Is the given pkt index in use */
#define SPT_IN_USE(spt, idx) (((spt)->in_use_bitmap & (1 << (idx))) != 0)

#define SPT_LATEST_PKT(spt) ((spt)->pkts[(spt)->latest_idx])

/* iovar table */
enum {
	IOV_BCN_ROTATE,	/* enable/disable beacon rotation */
	IOV_MBSS_IGN_MAC_VALID,
	IOV_MBSS,
	IOV_PREF_TRANSMIT_BSS
};

static const bcm_iovar_t mbss_iovars[] = {
	{"bcn_rotate", IOV_BCN_ROTATE, 0, 0, IOVT_BOOL, 0},	/* enable/disable beacon rotation */
	{"mbss_ign_mac_valid", IOV_MBSS_IGN_MAC_VALID,
	(IOVF_SET_DOWN), 0, IOVT_BOOL, 0
	},
	{"mbss", IOV_MBSS, IOVF_SET_DOWN, 0, IOVT_BOOL, 0},	/* enable/disable MBSS */
	{"pref_transmit_bss", IOV_PREF_TRANSMIT_BSS,
	IOVF_SET_DOWN, IOVT_BOOL, IOVT_UINT8, 0},
	{NULL, 0, 0, 0, 0, 0}
};

typedef uint (*wlc_nontrans_bss_ie_check)(wlc_bsscfg_t *trans_cfg, wlc_bsscfg_t *nontrans_cfg);

typedef struct nontrans_bcnprb {
	wlc_iem_tag_t tag;
	wlc_nontrans_bss_ie_check fn;
} nontrans_bcnprb_t;

#define BEACONING_ENABLED(wlc, cfg) (!MBSSID_ENAB(wlc->pub, wlc->band->bandtype) || \
		((MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && BSSCFG_IS_MAIN_AP(cfg))))

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

wlc_mbss_info_t *
BCMATTACHFN(wlc_mbss_attach)(wlc_info_t *wlc)
{
	wlc_mbss_info_t *mbss;
#ifdef WL_MBSSID
	uint16 bcnfstbmp = (
		FT2BMP(FC_BEACON) |
		FT2BMP(FC_PROBE_RESP));
#endif /* WL_MBSSID */

	ASSERT(wlc != NULL);

	/* Allocate info structure */
	if ((mbss = (wlc_mbss_info_t *)MALLOCZ(wlc->osh, sizeof(wlc_mbss_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	mbss->wlc = wlc;

	/* reserve cubby in the bsscfg container for per-bsscfg private data */
	if ((mbss->cfgh = wlc_bsscfg_cubby_reserve(wlc, sizeof(bss_mbss_info_t *),
		wlc_mbss_info_init, wlc_mbss_info_deinit, NULL, mbss)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* register module */
	if (wlc_module_register(wlc->pub, mbss_iovars, "mbss",
		mbss, wlc_mbss_doiovar, NULL, wlc_mbss_up, NULL)) {
		WL_ERROR(("wl%d: MBSS module register failed\n", wlc->pub->unit));
		goto fail;
	}

#ifdef WL_MBSSID
	if (wlc_iem_add_build_fn_mft(wlc->iemi, bcnfstbmp, DOT11_MNG_MULTIPLE_BSSID_ID,
		wlc_mbss_calc_mbssid_ie_len, wlc_mbss_write_mbssid_ie, mbss) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, mbssid in bcn\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	if (wlc_iem_add_build_fn(wlc->iemi, FC_BEACON, DOT11_MNG_MULTIPLE_BSSID_ID,
		wlc_mbss_calc_mbssid_ie_len_all, wlc_mbss_write_mbssid_ie_all, mbss) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, mbssid in bcn when wl down\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
	if (wlc_iem_add_build_fn_mft(wlc->iemi, bcnfstbmp, DOT11_MNG_MBSSID_CONFIG_ID,
			wlc_mbss_calc_multibssid_configuration_ie_len,
			wlc_mbss_write_mbssid_configuration_ie, mbss) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, mbssid configuration in bcn\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif /* WL_MBSSID */

#if defined(BCMDBG)
	/* debug dump */
	wlc_dump_register(wlc->pub, "mbss", wlc_mbss_dump, mbss);
#endif

	wlc->pub->_mbss_support = TRUE;
	wlc->pub->tunables->maxucodebss = WLC_MAX_AP_BSS(wlc->pub->corerev);

	if (wlc_pcb_fn_set(wlc->pcb, 0, WLF2_PCB1_MBSS_BCMC, wlc_mbss_bcmc_free_cb) != BCME_OK) {
		WL_ERROR(("wl%d: %s wlc_pcb_fn_set() failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	mbss->mbss16_beacon_count = -1;	 /* Init, marking No tbtt done yet */
#ifdef WL_MBSSID
	mbss->max_bss_indicator =  wlc_calc_max_bss_indicator(wlc_ap_get_maxbss_count(wlc->ap));
#endif /* WL_MBSSID */
	return mbss;

fail:
	MODULE_DETACH(mbss, wlc_mbss_detach);

	return NULL;
}

void
BCMATTACHFN(wlc_mbss_detach)(wlc_mbss_info_t *mbss)
{
	if (mbss != NULL) {
		ASSERT(mbss->wlc != NULL && mbss->wlc->pub != NULL);

		mbss->wlc->pub->_mbss_support = FALSE;

		/* unregister */
		wlc_module_unregister(mbss->wlc->pub, "mbss", mbss);

		/* free info structure */
		MFREE(mbss->wlc->osh, mbss, sizeof(wlc_mbss_info_t));
	}
}

/* bsscfg cubby */
static int
wlc_mbss_info_init(void *hdl, wlc_bsscfg_t *cfg)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)hdl;
	wlc_info_t *wlc = mbss->wlc;
	bss_mbss_info_t **pbmi = MBSS_BSSCFG_CUBBY_LOC(mbss, cfg);
	bss_mbss_info_t *bmi;
	int err;

	/*
	 * Allocate the MBSS BSSCFG cubby if MBSS is supported.
	 * Note: This would occur even if MBSS is not enabled when wlc_bsscfg_init() is called,
	 * because it may become enabled later (via IOVAR), and therefore the cubby needs to exist.
	 */
	if (!MBSS_SUPPORT(wlc->pub))
		return BCME_OK;

	ASSERT(*pbmi == NULL);

	/* allocate memory and point bsscfg cubby to it */
	if ((bmi = MALLOCZ(wlc->osh, sizeof(*bmi))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
			wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		err = BCME_NOMEM;
		goto fail;
	}
	*pbmi = bmi;

	/* allocate bcn_template */
	if ((bmi->bcn_template = (wlc_spt_t *) MALLOCZ(wlc->osh, sizeof(wlc_spt_t))) == NULL) {
		err = BCME_NOMEM;
		goto fail;
	}

#ifdef WLCNT
	if ((bmi->cnt = (wlc_mbss_cnt_t *) MALLOCZ(wlc->osh, sizeof(wlc_mbss_cnt_t))) == NULL) {
		err = BCME_NOMEM;
		goto fail;
	}
#endif	/* WLCNT */

	return BCME_OK;

fail:
	wlc_mbss_info_deinit(hdl, cfg);

	return err;
}

static void
wlc_mbss_info_deinit(void *hdl, wlc_bsscfg_t *cfg)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)hdl;
	wlc_info_t *wlc = mbss->wlc;
	bss_mbss_info_t **pbmi = MBSS_BSSCFG_CUBBY_LOC(mbss, cfg);
	bss_mbss_info_t *bmi = *pbmi;

	if (bmi == NULL)
		return;

#ifdef WLCNT
	if (bmi->cnt)
		MFREE(wlc->osh, bmi->cnt, sizeof(*(bmi->cnt)));
#endif	/* WLCNT */

	if (bmi->bcn_template) {
		wlc_spt_deinit(wlc, bmi->bcn_template, FALSE);
		MFREE(wlc->osh, bmi->bcn_template, sizeof(*(bmi->bcn_template)));
	}

	MFREE(wlc->osh, bmi, sizeof(*bmi));
	*pbmi = NULL;
}

/* bsscfg cubby accessors */
wlc_pkt_t
wlc_mbss_get_probe_template(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bss_mbss_info_t *bmi;

	ASSERT(cfg != NULL);
	if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && !BSSCFG_IS_MAIN_AP(cfg)) {
		return NULL;
	}

	bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	ASSERT(bmi != NULL);

	return bmi->probe_template;
}

wlc_spt_t *
wlc_mbss_get_bcn_spt(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bss_mbss_info_t *bmi;

	ASSERT(cfg != NULL);

	bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	ASSERT(bmi != NULL);

	return bmi->bcn_template;
}

wlc_pkt_t
wlc_mbss_get_bcn_template(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_spt_t *bcn_template = wlc_mbss_get_bcn_spt(wlc, cfg);

	ASSERT(bcn_template != NULL);

	return SPT_LATEST_PKT(bcn_template);
}

void
wlc_mbss_set_bcn_tim_ie(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint8 *ie)
{
	wlc_spt_t *bcn_template = wlc_mbss_get_bcn_spt(wlc, cfg);

	ASSERT(bcn_template != NULL);

	bcn_template->tim_ie = ie;
}

uint32
wlc_mbss_get_bcmc_pkts_sent(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bss_mbss_info_t *bmi;

	ASSERT(cfg != NULL);

	bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	ASSERT(bmi != NULL);

	return bmi->mc_fifo_pkts;
}

/** Mark all but the primary cfg as disabled */
static void
wlc_bsscfg_disablemulti(wlc_info_t *wlc)
{
	int i;
	wlc_bsscfg_t * bsscfg;

	/* iterate along the ssid cfgs */
	for (i = 1; i < WLC_MAXBSSCFG; i++)
		if ((bsscfg = WLC_BSSCFG(wlc, i)))
			wlc_bsscfg_disable(wlc, bsscfg);
}

static int
wlc_mbss_doiovar(void *hdl, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, struct wlc_if *wlcif)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)hdl;
	wlc_info_t *wlc = mbss->wlc;
	int32 int_val = 0;
	bool bool_val;
	uint32 *ret_uint_ptr;
	int err = BCME_OK;
	bool curstat;

	if (plen >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	bool_val = (int_val != 0) ? TRUE : FALSE;

	ret_uint_ptr = (uint32 *)arg;

	switch (actionid) {
	case IOV_GVAL(IOV_MBSS):
		*ret_uint_ptr = (wlc->pub->_mbss_mode != 0) ? TRUE : FALSE;
		break;

	case IOV_SVAL(IOV_MBSS):
		curstat = (wlc->pub->_mbss_mode != 0);

		/* No change requested */
		if (curstat == bool_val)
			break;

		if (!MBSS_SUPPORT(wlc->pub)) {
			err = BCME_UNSUPPORTED;
			break;
		}

		/* Reject if insufficient template memory */
		if (wlc_bmac_ucodembss_hwcap(wlc->hw) == FALSE) {
			err = BCME_NORESOURCE;
			break;
		}

		if (curstat) {
			/* if turning off mbss, disable extra bss configs */
			wlc_bsscfg_disablemulti(wlc);
			wlc_bmac_set_defmacintmask(wlc->hw, MI_DTIM_TBTT, ~MI_DTIM_TBTT);
			wlc->pub->_mbss_mode = 0;
		}
		else {
			wlc_bmac_set_defmacintmask(wlc->hw, MI_DTIM_TBTT, MI_DTIM_TBTT);
			wlc->pub->_mbss_mode = MBSS_ENABLED;

			if (ETHER_ISNULLADDR(&wlc->mbss->vether_base)) {
				bcopy(&wlc->pub->cur_etheraddr,
					&wlc->mbss->vether_base, ETHER_ADDR_LEN);
				wlc->mbss->vether_base.octet[5] =
					(wlc->mbss->vether_base.octet[5] &
						~(WLC_MBSS_UCIDX_MASK(wlc->pub->corerev))) |
					((wlc->mbss->vether_base.octet[5] + 1) &
						WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
				/* force locally administered address */
				ETHER_SET_LOCALADDR(&wlc->mbss->vether_base);
			}
		}
		break;

	case IOV_GVAL(IOV_PREF_TRANSMIT_BSS):
		*ret_uint_ptr = wlc->pub->_pref_transmit_bss;
		break;

	case IOV_SVAL(IOV_PREF_TRANSMIT_BSS):
		if (int_val >= WLC_MAXBSSCFG) {
			err = BCME_BADARG;
		     break;
		}
		wlc->pub->_pref_transmit_bss = int_val;
		break;

	case IOV_GVAL(IOV_MBSS_IGN_MAC_VALID):
		*ret_uint_ptr = wlc->pub->_mbss_ign_mac_valid ? TRUE : FALSE;
		break;

	case IOV_SVAL(IOV_MBSS_IGN_MAC_VALID):
		wlc->pub->_mbss_ign_mac_valid = bool_val;
		break;

	default:
		err = BCME_UNSUPPORTED;
	}

	return err;
}

/* Write the base MAC/BSSID into shared memory.  For MBSS, the MAC and BSSID
 * are required to be the same.
 */
static int
wlc_write_mbss_basemac(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	uint16 mac_l;
	uint16 mac_m;
	uint16 mac_h;
	struct ether_addr *addr;

	ASSERT(MBSS_SUPPORT(wlc->pub));

	if (((BSSCFG_IS_PRIMARY(cfg)) && (wlc->aps_associated == 0)) ||
		ETHER_ISNULLADDR(&wlc->mbss->vether_base)) {
		addr = &wlc->pub->cur_etheraddr;
		mac_l = addr->octet[0] | (addr->octet[1] << 8);
		mac_m = addr->octet[2] | (addr->octet[3] << 8);
		mac_h = addr->octet[4] | (addr->octet[5] << 8);
	} else {
		if (MBSS_IGN_MAC_VALID(wlc->pub)) {
			addr = &cfg->cur_etheraddr;
		} else
			addr = &wlc->mbss->vether_base;
		mac_l = addr->octet[0] | (addr->octet[1] << 8);
		mac_m = addr->octet[2] | (addr->octet[3] << 8);
		/* Mask low bits of BSSID base */
		mac_h = addr->octet[4] |
			((addr->octet[5] & ~(WLC_MBSS_UCIDX_MASK(wlc->pub->corerev))) << 8);
	}

	wlc_write_shm(wlc, M_MBS_BSSID0(wlc), mac_l);
	wlc_write_shm(wlc, M_MBS_BSSID1(wlc), mac_m);
	wlc_write_shm(wlc, M_MBS_BSSID2(wlc), mac_h);

	return BCME_OK;
}

static int
wlc_mbss_up(void *hdl)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)hdl;
	wlc_info_t *wlc = mbss->wlc;
	uint8 i;

	(void)wlc;

	ASSERT(MBSS_SUPPORT(wlc->pub));

	if (!MBSS_ENAB(wlc->pub)) {
		return BCME_OK;
	}

	/* Initialize the HW to SW BSS configuration index map */
	for (i = 0; i < WLC_MAXBSSCFG; i++) {
		mbss->hw2sw_idx[i] = WLC_MBSS_BSSCFG_IDX_INVALID;
	}
	return BCME_OK;
}

/*
 * Allocate and set up a software packet template
 * @param count The number of packets to use; must be <= WLC_SPT_COUNT_MAX
 * @param len The length of the packets to be allocated
 *
 * Returns 0 on success, < 0 on error.
 */

static int
wlc_spt_init(wlc_info_t *wlc, wlc_spt_t *spt, int count, int len)
{
	int idx;
	int tso_hdr_overhead = ((wlc->toe_bypass || D11REV_GE(wlc->pub->corerev, 128)) ?
		0 : sizeof(d11ac_tso_t));

	/* Pad for header overhead */
	len += tso_hdr_overhead;

	/* Pad for PLCP */
	if (D11REV_GE(wlc->pub->corerev, 128))
		len += D11_PHY_HDR_LEN;
	else /* Pad for txh */
		len += D11_TXH_LEN_EX(wlc);

	ASSERT(spt != NULL);
	ASSERT(count <= WLC_SPT_COUNT_MAX);

	for (idx = 0; idx < count; idx++) {
		if (spt->pkts[idx] == NULL &&
		    (spt->pkts[idx] = PKTGET(wlc->osh, len, TRUE)) == NULL) {
			return -1;
		}
	}

	spt->latest_idx = -1;

	return 0;
}

/*
 * Clean up a software template object;
 * if pkt_free_force is TRUE, will not check if the pkt is in use before freeing.
 * Note that if "in use", the assumption is that some other routine owns
 * the packet and will free appropriately.
 */

static void
wlc_spt_deinit(wlc_info_t *wlc, wlc_spt_t *spt, int pkt_free_force)
{
	int idx;

	if (spt != NULL) {
		for (idx = 0; idx < WLC_SPT_COUNT_MAX; idx++) {
			if (spt->pkts[idx] != NULL) {
				if (pkt_free_force || !SPT_IN_USE(spt, idx)) {
					PKTFREE(wlc->osh, spt->pkts[idx], TRUE);
					spt->pkts[idx] = NULL;
				} else {
					WLPKTFLAG_BSS_DOWN_SET(WLPKTTAG(spt->pkts[idx]), TRUE);
				}
			}
		}
		bzero(spt, sizeof(*spt));
	}
}

static void
mbss_ucode_set(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bool cur_val, new_val;

	/* Assumes MBSS_EN has same value in all cores */
	cur_val = ((wlc_mhf_get(wlc, MHF1, WLC_BAND_AUTO) & MHF1_MBSS_EN) != 0);
	new_val = (MBSS_ENAB(wlc->pub) != 0);

	if (cur_val != new_val) {
		wlc_suspend_mac_and_wait(wlc);
		/* enable MBSS in uCode */
		WL_MBSS(("%s MBSS mode\n", new_val ? "Enabling" : "Disabling"));
		(void)wlc_mhf(wlc, MHF1, MHF1_MBSS_EN, new_val ? MHF1_MBSS_EN : 0, WLC_BAND_ALL);
		wlc_enable_mac(wlc);
	}
}

/* Generate a MAC address for the MBSS AP BSS config */
int
wlc_bsscfg_macgen(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	int ii;
	bool collision = FALSE;
	int cfg_idx = WLC_BSSCFG_IDX(cfg);
	struct ether_addr newmac;
	wlc_bsscfg_t *bsscfg;
#ifdef BCMDBG
	char eabuf[ETHER_ADDR_STR_LEN];
#endif /* BCMDBG */

	ASSERT(MBSS_SUPPORT(wlc->pub));
	ASSERT(!ETHER_ISNULLADDR(&mbss->vether_base));

	bcopy(&mbss->vether_base, &newmac, ETHER_ADDR_LEN);

	/* determine the lower bits according the index of vif
	 * which could avoid collision
	 */
	newmac.octet[5] =
		(newmac.octet[5] & ~(WLC_MBSS_UCIDX_MASK(wlc->pub->corerev))) |
		((newmac.octet[5] + cfg_idx - 1) & WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));

	/* checks for collisions with other configs
	 */
	FOREACH_BSS(wlc, ii, bsscfg) {
		if ((bsscfg == cfg) ||
		    (ETHER_ISNULLADDR(&bsscfg->cur_etheraddr))) {
			continue;
		}
		if (EADDR_TO_UC_IDX(bsscfg->cur_etheraddr,
			WLC_MBSS_UCIDX_MASK(wlc->pub->corerev)) ==
		    EADDR_TO_UC_IDX(newmac, WLC_MBSS_UCIDX_MASK(wlc->pub->corerev))) {
			collision = TRUE;
			break;
		}
	}

	if (collision == TRUE) {
		WL_MBSS(("wl%d.%d: wlc_bsscfg_macgen couldn't generate MAC address\n",
		         wlc->pub->unit, cfg_idx));

		return BCME_BADADDR;
	}
	else {
		bcopy(&newmac, &cfg->cur_etheraddr, ETHER_ADDR_LEN);
		WL_MBSS(("wl%d.%d: wlc_bsscfg_macgen assigned MAC %s\n",
		         wlc->pub->unit, cfg_idx,
		         bcm_ether_ntoa(&cfg->cur_etheraddr, eabuf)));
		return BCME_OK;
	}
}

#define BCN_TEMPLATE_COUNT 2
int
wlc_mbss_beacon_init(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wlc_mbss_info_t *mbss)
{
	bss_mbss_info_t *bmi;
	bss_mbss_info_t *bmi_loop;
	wlc_bsscfg_t *cfg_loop;
	uint16 cfgidx;
	int result = 0;
	int  bcn_tmpl_len;
	int idx = 0;

	if (!cfg) {
		return -1;
	}
	bmi = BSS_MBSS_INFO(mbss, cfg);
	ASSERT(bmi != NULL);
#ifdef WL_MBSSID
		if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
			uint16 count = 0;
			bool start_count = FALSE;
			uint16 id = 0;
			if (wlc->pub->_pref_transmit_bss != WLC_INVALID_BSS_IDX) {
				for (cfgidx = 0; cfgidx < WLC_MAXBSSCFG; cfgidx++) {
					id = cfgidx + wlc->pub->_pref_transmit_bss;
					if (id >= WLC_MAXBSSCFG) {
						id = id - WLC_MAXBSSCFG;
					}
					cfg_loop = wlc->bsscfg[id];
					if (!cfg_loop) {
						count ++;
						continue;
					}
					bmi_loop = BSS_MBSS_INFO(mbss, cfg_loop);
					bmi_loop->_ucidx = count;
					count++;
				}
			} else {
				for (cfgidx = 0; cfgidx < WLC_MAXBSSCFG; cfgidx++) {
					cfg_loop = wlc->bsscfg[cfgidx];
					if (!cfg_loop) {
						if (start_count) {
							count ++;
						}
						continue;
					}
					bmi_loop = BSS_MBSS_INFO(mbss, cfg_loop);
					if (cfg_loop && BSSCFG_STA(cfg_loop)) {
						continue;
					}
					if (BSSCFG_AP(cfg_loop)) {
						if (BSSCFG_IS_MAIN_AP(cfg_loop)) {
							start_count = TRUE;
						}
					}
					if (start_count) {
						bmi_loop->_ucidx = count;
						count++;
					}
				}
			}
		} else
#endif /* WL_MBSSID */
		{
			/* Set the uCode index of this config */
			bmi->_ucidx = EADDR_TO_UC_IDX(cfg->cur_etheraddr,
					WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
			ASSERT(bmi->_ucidx <= WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
		}

	bcn_tmpl_len = wlc->pub->bcn_tmpl_len;
	if (BEACONING_ENABLED(wlc, cfg)) {
		/* Allocate DMA space for beacon software template */
		result = wlc_spt_init(wlc, bmi->bcn_template, BCN_TEMPLATE_COUNT, bcn_tmpl_len);
		if (result != BCME_OK) {
			WL_ERROR(("wl%d.%d: %s: unable to allocate beacon templates",
					wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));
			return result;
		}
		/* Set the BSSCFG index in the packet tag for beacons */
		for (idx = 0; idx < BCN_TEMPLATE_COUNT; idx++) {
			WLPKTTAGBSSCFGSET(bmi->bcn_template->pkts[idx], WLC_BSSCFG_IDX(cfg));
		}
	}
	mbss->hw2sw_idx[bmi->_ucidx] = WLC_BSSCFG_IDX(cfg);

	/* Make sure that our SSID is in the correct uCode
	 * SSID slot in shared memory
	 */
	wlc_shm_ssid_upd(wlc, cfg);

	wlc_mbss_bcmc_reset(wlc, cfg);

	cfg->flags &= ~(WLC_BSSCFG_SW_BCN | WLC_BSSCFG_SW_PRB);
	cfg->flags &= ~(WLC_BSSCFG_HW_BCN | WLC_BSSCFG_HW_PRB);
	if (BEACONING_ENABLED(wlc, cfg)) {
		if (!MBSS_ENAB(wlc->pub)) {
			cfg->flags |= (WLC_BSSCFG_SW_BCN | WLC_BSSCFG_SW_PRB);
		} else {
			cfg->flags |= (WLC_BSSCFG_HW_BCN | WLC_BSSCFG_HW_PRB);
		}
	}
	return 0;

}

static int
mbss_bsscfg_up(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	int result = 0;
	int idx = 0;
	wlc_bsscfg_t *bsscfg;
#ifdef WL_MBSSID
	uint8 mbssid_enab;
#endif /* WL_MBSSID */

	ASSERT(MBSS_SUPPORT(wlc->pub));

	/* Assumes MBSS is enabled for this BSS config herein */

	/* Set pre TBTT interrupt timer to 10 ms for now; will be shorter */
	wlc_write_shm(wlc, M_MBS_PRETBTT(wlc), wlc_ap_get_pre_tbtt(wlc->ap));

	/* if the BSS configs hasn't been given a user defined address or
	 * the address is duplicated, we'll generate our own.
	 */
	if (!BSSCFG_IS_PRIMARY(cfg)) {
		FOREACH_BSS(wlc, idx, bsscfg) {
			if (bsscfg == cfg)
				continue;
			if (bcmp(&bsscfg->cur_etheraddr, &cfg->cur_etheraddr, ETHER_ADDR_LEN) == 0)
				break;
		}
	}

	if (BSSCFG_IS_MAIN_AP(cfg)) {
		if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
			if (!MBSSID_ALLOWED(wlc->pub)) {
				WL_ERROR(("[%s] mbssid is not allowed corerev [%d] minor[%d] \n",
					__FUNCTION__, wlc->pub->corerev,
					wlc->pub->corerev_minor));
					ASSERT(0);
			}
			wlc_mhf(wlc, MHF6, MHF6_MBSSID, MHF6_MBSSID, WLC_BAND_ALL);
			wlc->pub->bcn_tmpl_len = D11_MBSSID_BCN_TMPL_LEN;
			wlc_write_shm(wlc, M_BCN_TPLBLK_BSZ(wlc), wlc->pub->bcn_tmpl_len);
		} else {
			if (D11REV_GE(wlc->pub->corerev, 40)) {
				if (wlc->pub->bcn_tmpl_len != D11AC_BCN_TMPL_LEN) {
					wlc->pub->bcn_tmpl_len = D11AC_BCN_TMPL_LEN;
					wlc_write_shm(wlc, M_BCN_TPLBLK_BSZ(wlc),
						wlc->pub->bcn_tmpl_len);
				}
			} else {
				if (wlc->pub->bcn_tmpl_len != BCN_TMPL_LEN) {
					wlc->pub->bcn_tmpl_len = BCN_TMPL_LEN;
					wlc_write_shm(wlc, M_BCN_TPLBLK_BSZ(wlc),
						wlc->pub->bcn_tmpl_len);
				}
			}
		}
	}

	if (ETHER_ISNULLADDR(&cfg->cur_etheraddr) ||
		((idx < WLC_MAXBSSCFG) && (!BSSCFG_IS_PRIMARY(cfg)) &&
		!MBSS_IGN_MAC_VALID(wlc->pub))) {
		result = wlc_bsscfg_macgen(wlc, cfg);
		if (result != BCME_OK) {
			WL_ERROR(("wl%d.%d: %s: unable to generate MAC address\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));
			return result;
		}
	}

	wlc_mbss_beacon_init(wlc, cfg, mbss);
#ifdef WL_MBSSID
	mbssid_enab = MBSSID_ENAB(wlc->pub, wlc->band->bandtype);
	wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_MBSSID, mbssid_enab);
	wlc_bsscfg_set_ext_cap(cfg, DOT11_EXT_CAP_COMPLETE_NONTXBSSID_PROF, mbssid_enab);
#endif /* WL_MBSSID */

	return result;
}

/**
 * This function is used to reinitialize the bcmc firmware/ucode interface for a certain bsscfg.
 * Only to be called when there is no bcmc traffic pending in hw fifos.
 *
 * For MBSS, for each bss, every DTIM the last transmitted bcmc fid is set in SHM.
 * Ucode will only transmit up to and including this fid even if more bcmc packets are added later.
 * When there is no more bcmc traffic pending for a certain bss:
 * 1) bcmc_fid and bcmc_fid_shm are set to INVALIDFID, in case last traffic was flushed shm needs to
 *    be updated as well.
 * 2) mc_fifo_pkts is set to 0.
 * 3) a ps off transition can be marked as complete so that the PS bit for the bcmc scb of that bss
 *    is properly set.
 */
void
wlc_mbss_bcmc_reset(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	uint fid_addr = 0;
	bss_mbss_info_t *bmi;

	cfg->bcmc_fid = INVALIDFID;
	cfg->bcmc_fid_shm = INVALIDFID;

	ASSERT(MBSS_SUPPORT(wlc->pub));

	bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	if (bmi == NULL) {
		return;
	}

	WL_MBSS(("wl%d.%d: %s: resetting fids %d, %d; mc pkts %d\n", wlc->pub->unit,
		WLC_BSSCFG_IDX(cfg), __FUNCTION__, cfg->bcmc_fid, cfg->bcmc_fid_shm,
		bmi->mc_fifo_pkts));

	bmi->mc_fifo_pkts = 0;

	if (wlc->pub->hw_up) {
		/* Let's also update the shm */
		fid_addr = SHM_MBSS_BC_FID_ADDR16(wlc, bmi->_ucidx);
		wlc_write_shm((wlc), fid_addr, INVALIDFID);
	}
}

int
wlc_mbss_bsscfg_up(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	int ret = BCME_OK;

	if (MBSS_ENAB(wlc->pub)) {
		if ((ret = mbss_bsscfg_up(wlc, cfg)) != BCME_OK)
			return ret;

		/* XXX JFH - This is redundant right now, we're also
		 * writing the MAC in wlc_BSSinit() but we want this
		 * to be done prior to enabling MBSS per George
		 * We should probably be using wlc_set_mac() here..?
		 */
		wlc_write_mbss_basemac(wlc, cfg);
	}
	mbss_ucode_set(wlc, cfg);

	return ret;
}

void wlc_mbss_spt_deinit(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	if (BEACONING_ENABLED(wlc, cfg)) {
		wlc_spt_deinit(wlc, bmi->bcn_template, FALSE);
	}
}

void
wlc_mbss_bsscfg_down(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	if (MBSS_SUPPORT(wlc->pub)) {
		uint clear_len = FALSE;
		wlc_bsscfg_t *bsscfg;
		bss_mbss_info_t *bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
		uint8 ssidlen = cfg->SSID_len;
		uint i;

		ASSERT(bmi != NULL);

		if (bmi->bcn_template) {
			wlc_spt_deinit(wlc, bmi->bcn_template, FALSE);
		}
		if (bmi->probe_template != NULL) {
			PKTFREE(wlc->osh, bmi->probe_template, TRUE);
			bmi->probe_template = NULL;
		}

		/* If we clear ssid length of all bsscfgs while doing
		 * a wl down the ucode can get into a state where it
		 * will keep searching  for non-zero ssid length thereby
		 * causing mac_suspend_and_wait messages. To avoid that
		 * we will prevent clearing the ssid len of at least one BSS.
		 * If all BBS go down and no beacons are being sent, the ssid
		 * entire ssid length table will be reset to all 0's when
		 * beaconing is restarted.  See wlc_shm_ssid_upd() and
		 * the mbss16_beacon_count in wlc_mbss_info_t
		 */
		FOREACH_BSS(wlc, i, bsscfg) {
			if (bsscfg->up) {
				clear_len = TRUE;
				break;
			}
		}
		if (clear_len) {
			cfg->SSID_len = 0;

			/* update uCode shared memory */
			wlc_shm_ssid_upd(wlc, cfg);
			cfg->SSID_len = ssidlen;

			/* Clear the HW index */
			wlc->mbss->hw2sw_idx[bmi->_ucidx] = WLC_MBSS_BSSCFG_IDX_INVALID;
		}
	}
}

#ifdef BCMDBG
void
wlc_mbss_dump_spt_pkt_state(wlc_info_t *wlc, wlc_bsscfg_t *cfg, int i)
{
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(wlc->mbss, cfg);

	ASSERT(bmi != NULL);

	/* Dump SPT pkt state */
	if (bmi->bcn_template != NULL) {
		int j;
		wlc_spt_t *spt = bmi->bcn_template;

		for (j = 0; j < WLC_SPT_COUNT_MAX; j++) {
			if (spt->pkts[j] != NULL) {
				wlc_pkttag_t *tag = WLPKTTAG(spt->pkts[j]);

				WL_ERROR(("bss[%d] spt[%d]=%p in_use=%d flags=%08x flags2=%04x\n",
				          i, j, OSL_OBFUSCATE_BUF(spt->pkts[j]),
				          SPT_IN_USE(bmi->bcn_template, j),
				          tag->flags, tag->flags2));
			}
		}
	}
}
#endif /* BCMDBG */

/* Under MBSS, this routine handles all TX dma done packets from the ATIM fifo. */
void
wlc_mbss_dotxstatus(wlc_info_t *wlc, tx_status_t *txs, void *pkt, uint16 fc,
                    wlc_pkttag_t *pkttag, uint supr_status)
{
	wlc_bsscfg_t *bsscfg = NULL;
	int bss_idx;
	bool free_pkt = FALSE;
	bss_mbss_info_t *bmi;

	ASSERT(MBSS_SUPPORT(wlc->pub));

	bss_idx = (int)(WLPKTTAG_BSSIDX_GET(pkttag));
#if defined(BCMDBG) /* Verify it's a reasonable index */
	if ((bss_idx < 0) || (bss_idx >= WLC_MAXBSSCFG) ||
	    (wlc->bsscfg[bss_idx] == NULL)) {
		WL_ERROR(("%s: bad BSS idx\n", __FUNCTION__));
		ASSERT(0);
		return;
	}
#endif /* BCMDBG */

	/* For probe resp, this is really only used for counters */
	ASSERT(bss_idx < WLC_MAXBSSCFG);
	bsscfg = wlc->bsscfg[bss_idx];
	ASSERT(bsscfg != NULL);

	bmi = BSS_MBSS_INFO(wlc->mbss, bsscfg);
	ASSERT(bmi != NULL);

	/* Being in the ATIM fifo, it must be a beacon or probe response */
	switch (fc & FC_KIND_MASK) {
	case FC_PROBE_RESP:
		/* Requeue suppressed probe response if due to TBTT */
		if (supr_status == TX_STATUS_SUPR_TBTT) {
			int txerr;

			WLCNTINCR(bmi->cnt->prb_resp_retrx);
			txerr = wlc_bmac_dma_txfast(wlc, TX_ATIM_FIFO, pkt, TRUE);

			if (txerr < 0) {
				WL_MBSS(("Failed to retransmit suppressed probe resp for bss %d\n",
					WLC_BSSCFG_IDX(bsscfg)));
				WLCNTINCR(bmi->cnt->prb_resp_retrx_fail);
				free_pkt = TRUE;
			}
		} else {
			free_pkt = TRUE;
			if (supr_status == TX_STATUS_SUPR_EXPTIME) {
				WLCNTINCR(bmi->cnt->prb_resp_ttl_expy);
			}
		}
		break;
	case FC_BEACON:
		if (supr_status != TX_STATUS_SUPR_NONE)
			WL_ERROR(("%s: Suppressed Beacon frame = 0x%x\n", __FUNCTION__,
			          supr_status));

		if (WLPKTFLAG_BSS_DOWN_GET(pkttag)) { /* Free the pkt since BSS is gone */
			WL_MBSS(("BSSCFG down on bcn done\n"));
			WL_ERROR(("%s: in_use_bitmap = 0x%x pkt: %p\n", __FUNCTION__,
			          bmi->bcn_template->in_use_bitmap, OSL_OBFUSCATE_BUF(pkt)));
			free_pkt = TRUE;
			break; /* All done */
		}
		ASSERT(bsscfg->up);
		/* Assume only one beacon in use at a time */
		bmi->bcn_template->in_use_bitmap = 0;
#if defined(WLC_SPT_DEBUG) && defined(BCMDBG)
		if (supr_status != TX_STATUS_SUPR_NONE) {
			bmi->bcn_template->suppressed++;
		}
#endif /* WLC_STP_DEBUG && BCMDBG */
		break;
	default: /* Bad packet type for ATIM fifo */
		WL_ERROR(("Bad Packet Type. TX done ATIM packet neither BCN or PRB"));
		ASSERT(0);
		break;
	}

	if (supr_status != TX_STATUS_SUPR_NONE) {
		WLCNTINCR(wlc->pub->_cnt->atim_suppress_count);
	}

	if (free_pkt) {
		PKTFREE(wlc->osh, pkt, TRUE);
	}
}

void
wlc_mbss_dotxstatus_mcmx(wlc_info_t *wlc, wlc_bsscfg_t *cfg, tx_status_t *txs)
{
	bss_mbss_info_t *bmi;

	ASSERT(MBSS_ENAB(wlc->pub));

	bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	ASSERT(bmi != NULL);

	bmi->mc_fifo_pkts--; /* Decrement mc fifo counter */

	/* Check if this was last frame uCode knew about */
	if (D11_TXFID_GET_SEQ(wlc, cfg->bcmc_fid_shm) == D11_TXFID_GET_SEQ(wlc, txs->frameid)) {
		cfg->bcmc_fid_shm = INVALIDFID;
	}
}

void
wlc_mbss_update_beacon(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	uint16 *bcn;
	int idx;
	wlc_pkt_t pkt;
	ratespec_t bcn_rspec;
	wlc_bss_info_t *current_bss = cfg->current_bss;
	int len = wlc->pub->bcn_tmpl_len;
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(wlc->mbss, cfg);

	ASSERT(MBSS_BCN_ENAB(wlc, cfg));
	ASSERT(bmi != NULL);

	if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && !BSSCFG_IS_MAIN_AP(cfg)) {
		return;
	}
	/* Find a non-inuse buffer */
	if ((idx = SPT_PAIR_AVAIL(bmi->bcn_template)) == -1) {
		WL_ERROR(("Beacon template not available"));
		ASSERT(0);
		return;
	}

	pkt = bmi->bcn_template->pkts[idx];
	ASSERT(pkt != NULL);

	if (D11REV_GE(wlc->pub->corerev, 128)) {
		PKTPULL(wlc->osh, pkt, D11_PHY_HDR_LEN);
	} else {
		/* account for tso_hdr before txh */
#ifdef WLTOEHW
		uint16 tso_hdr_size;
		d11ac_tso_t *tso_hdr;

		/* Pull off space so that d11hdrs below works */
		tso_hdr = (d11ac_tso_t *) PKTDATA(wlc->osh, pkt);
		tso_hdr_size = (uint16) (wlc->toe_bypass ? 0 : wlc_tso_hdr_length(tso_hdr));
		PKTPULL(wlc->osh, pkt, tso_hdr_size);
#endif /* WLTOEHW */

		PKTPULL(wlc->osh, pkt, D11_TXH_LEN_EX(wlc));
	}

	/* Use the lowest basic rate for beacons if no user specified bcn rate */
	bcn_rspec =
		wlc_ap_forced_bcn_rspec_get(wlc) ? wlc_ap_forced_bcn_rspec_get(wlc) :
		wlc_lowest_basic_rspec(wlc, &current_bss->rateset);
	if (CHSPEC_IS6G(wlc->chanspec) &&
		((wlc->lpi_mode == AUTO && wlc->stf->psd_limit_indicator) ||
		(wlc->lpi_mode == ON))) {
		bcn_rspec &= ~WL_RSPEC_BW_MASK;
		bcn_rspec |= CHSPECBW_TO_RSPECBW(CHSPEC_BW(wlc->chanspec));
	}
	ASSERT(wlc_valid_rate(wlc, bcn_rspec,
	                      CHSPEC_BANDTYPE(current_bss->chanspec),
	                      TRUE));
	wlc->bcn_rspec = bcn_rspec;

	bcn = (uint16 *)PKTDATA(wlc->osh, pkt);
	wlc_bcn_prb_template(wlc, FC_BEACON, bcn_rspec, cfg, bcn, &len);

	PKTSETLEN(wlc->osh, pkt, len);
	wlc_write_hw_bcnparams(wlc, cfg, bcn, len, bcn_rspec, FALSE);
	if (D11REV_GE(wlc->pub->corerev, 128)) {
		/* insert PLCP */
		uint8 *plcp = PKTPUSH(wlc->osh, pkt, D11_PHY_HDR_LEN);
		bzero(plcp, D11_PHY_HDR_LEN);
		wlc_compute_plcp(wlc, SCB_BSSCFG(wlc->band->hwrs_scb), bcn_rspec,
			len + DOT11_FCS_LEN, FC_BEACON, plcp);

		if (RSPEC_ISHE(bcn_rspec)) {
			//clear b8-b13 BSS COLOR for HE PLCP
			*(uint32 *) plcp &= ~HE_SIGA_BSS_COLOR_MASK_(HE_FORMAT_SU);
		}
	} else {
		wlc_d11hdrs(wlc, pkt, wlc->band->hwrs_scb, FALSE, 0, 0,
			TX_ATIM_FIFO, 0, NULL, NULL, bcn_rspec);
	}
	/* Indicate most recently updated index */
	bmi->bcn_template->latest_idx = idx;
	bmi->bcn_template->bcn_modified = TRUE;
}

/* Select SSID length register based on HW index */
#define _MBSS_SSID_LEN_SELECT(wlc, idx) \
	(MBSS_ENAB(wlc->pub) ? \
	(((idx) == 0 || (idx) == 1) ? SHM_MBSS_SSID_LEN0(wlc) : \
	 SHM_MBSS_SSID_LEN1(wlc)) : M_SSID_BYTESZ(wlc))

/* Use to access a specific SSID length */
#define WLC_MBSS_SSID_LEN_GET(wlc, idx, out_val) \
	do { \
		out_val = wlc_read_shm(wlc, _MBSS_SSID_LEN_SELECT(wlc, idx)); \
		if ((idx) % 2) \
			out_val = ((out_val) >> 8) & 0xff; \
		else \
			out_val = (out_val) & 0xff; \
	} while (0)

/* Internal macro to set SSID length register values properly */
#define _MBSS_SSID_LEN_SET(idx, reg_val, set_val) \
	do { \
		if ((idx) % 2) { \
			(reg_val) &= ~0xff00; \
			(reg_val) |= ((set_val) & 0xff) << 8; \
		} else { \
			(reg_val) &= ~0xff; \
			(reg_val) |= (set_val) & 0xff; \
		} \
	} while (0)

#if defined(BCMDBG)
/* Get the SSID for the indicated (idx) bsscfg from SHM Return the length */
static void
wlc_shm_ssid_get(wlc_info_t *wlc, int idx, wlc_ssid_t *ssid)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	int i;
	int base;
	uint16 tmpval;
	int ucode_idx;
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(mbss, wlc->bsscfg[idx]);

	ASSERT(bmi != NULL);

	ucode_idx = bmi->_ucidx;

	if (MBSS_ENAB(wlc->pub)) {
		base = SHM_MBSS_SSIDSE_BASE_ADDR(wlc) + (ucode_idx * SHM_MBSS_SSIDSE_BLKSZ(wlc));
		wlc_bmac_copyfrom_objmem(wlc->hw, base, &ssid->SSID_len,
		                       SHM_MBSS_SSIDLEN_BLKSZ, OBJADDR_SRCHM_SEL);
		/* search mem length field is always little endian */
		ssid->SSID_len = ltoh32(ssid->SSID_len);
		base += SHM_MBSS_SSIDLEN_BLKSZ;
		wlc_bmac_copyfrom_objmem(wlc->hw, base, ssid->SSID,
		                       SHM_MBSS_SSID_BLKSZ, OBJADDR_SRCHM_SEL);
		return;
	}

	WLC_MBSS_SSID_LEN_GET(wlc, ucode_idx, ssid->SSID_len);
	base = SHM_MBSS_SSID_ADDR(wlc, ucode_idx);
	for (i = 0; i < DOT11_MAX_SSID_LEN; i += 2) {
		tmpval = wlc_read_shm(wlc, base + i);
		ssid->SSID[i] = tmpval & 0xFF;
		ssid->SSID[i + 1] = tmpval >> 8;
	}
}

/* Set this definition to 1 for additional verbosity */
#define BSSCFG_EXTRA_VERBOSE 1

#define SHOW_SHM(wlc, bf, addr, name) do { \
		uint16 tmpval; \
		tmpval = wlc_read_shm((wlc), (addr)); \
		bcm_bprintf(bf, "%15s     offset: 0x%04x (0x%04x)     0x%04x (%6d)\n", \
			name, addr / 2, addr, tmpval, tmpval); \
	} while (0)

static void
wlc_mbss_bsscfg_dump(void *hdl, wlc_bsscfg_t *cfg, struct bcmstrbuf *b)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)hdl;
	wlc_info_t *wlc = mbss->wlc;
	bss_mbss_info_t *bmi;
	int bsscfg_idx;

	ASSERT(cfg != NULL);

	bmi = BSS_MBSS_INFO(mbss, cfg);
	bsscfg_idx = WLC_BSSCFG_IDX(cfg);

	if (bmi == NULL) {
		return;
	}

	bcm_bprintf(b, "PS trans %u.\n", WLCNTVAL(bmi->cnt->ps_trans));
#if defined(WLC_SPT_DEBUG)
	if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && BSSCFG_IS_MAIN_AP(cfg)) {
		bcm_bprintf(b, "BCN: bcn tx cnt %u. bcn suppressed %u\n",
			bmi->bcn_template->tx_count, bmi->bcn_template->suppressed);
	}
#endif /* WLC_SPT_DEBUG */
	bcm_bprintf(b,
		"PrbResp: soft-prb-resp %s. alloc_fail %d, tx_fail %d\n",
		SOFTPRB_ENAB(cfg) ? "enabled" : "disabled",
		WLCNTVAL(bmi->cnt->prb_resp_alloc_fail),
		WLCNTVAL(bmi->cnt->prb_resp_tx_fail));
	bcm_bprintf(b, "PrbResp: TBTT suppressions %d. TTL expires %d. retrx fail %d.\n",
		WLCNTVAL(bmi->cnt->prb_resp_retrx), WLCNTVAL(bmi->cnt->prb_resp_ttl_expy),
		WLCNTVAL(bmi->cnt->prb_resp_retrx_fail));
	if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && BSSCFG_IS_MAIN_AP(cfg)) {
		bcm_bprintf(b, "BCN: soft-bcn %s. bcn in use bmap 0x%x. bcn fail %u\n",
				SOFTBCN_ENAB(cfg) ? "enabled" : "disabled",
				bmi->bcn_template->in_use_bitmap,
				WLCNTVAL(bmi->cnt->bcn_tx_failed));
		bcm_bprintf(b, "BCN: HW MBSS %s. bcn in use bmap 0x%x. bcn fail %u\n",
				HWBCN_ENAB(cfg) ? "enabled" : "disabled",
				bmi->bcn_template->in_use_bitmap,
				WLCNTVAL(bmi->cnt->bcn_tx_failed));
	}
	bcm_bprintf(b, "PRB: HW MBSS %s.\n",
		HWPRB_ENAB(cfg) ? "enabled" : "disabled");
	bcm_bprintf(b, "MC pkts in fifo %u. Max %u\n", bmi->mc_fifo_pkts,
		WLCNTVAL(bmi->cnt->mc_fifo_max));
	if (wlc->clk) {
		char ssidbuf[SSID_FMT_BUF_LEN];
		wlc_ssid_t ssid;
		uint16 shm_fid;

		shm_fid = wlc_read_shm(wlc, SHM_MBSS_BC_FID_ADDR16(wlc, bmi->_ucidx));
		bcm_bprintf(b, "bcmc_fid 0x%x. bcmc_fid_shm 0x%x. shm last fid 0x%x. "
			"bcmc TX pkts %u\n", cfg->bcmc_fid, cfg->bcmc_fid_shm, shm_fid,
			WLCNTVAL(bmi->cnt->bcmc_count));
		wlc_shm_ssid_get(wlc, bsscfg_idx, &ssid);
		if (ssid.SSID_len > DOT11_MAX_SSID_LEN) {
			WL_ERROR(("Warning: Invalid MBSS ssid length %d for BSS %d\n",
				ssid.SSID_len, bsscfg_idx));
			ssid.SSID_len = DOT11_MAX_SSID_LEN;
		}
		wlc_format_ssid(ssidbuf, ssid.SSID, ssid.SSID_len);
		bcm_bprintf(b, "MBSS: ucode idx %d; shm ssid >%s< of len %d bcn_len %d\n",
			bmi->_ucidx, ssidbuf, ssid.SSID_len, bmi->bcn_len);

	} else {
		bcm_bprintf(b, "Core clock disabled, not dumping SHM info\n");
	}
}

static int
wlc_mbss_dump(void *ctx, struct bcmstrbuf *b)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	int idx;
	wlc_bsscfg_t *bsscfg;

	if (!MBSS_SUPPORT(wlc->pub)) {
		bcm_bprintf(b, "MBSS not supported\n");
		return BCME_OK;
	}

	bcm_bprintf(b, "MBSS Build.  MBSS is %s. SW MBSS MHF band 0: %s; band 1: %s\n",
		MBSS_ENAB(wlc->pub) ? "enabled" : "disabled",
		(wlc_bmac_mhf_get(wlc->hw, MHF1, WLC_BAND_2G) & MHF1_MBSS_EN) ? "set" : "clear",
		(wlc_bmac_mhf_get(wlc->hw, MHF1, WLC_BAND_5G) & MHF1_MBSS_EN) ? "set" : "clear");
	bcm_bprintf(b, "Pkts suppressed from ATIM:  %d. Bcn Tmpl not ready/done %d/%d "
		"nobcnupd %d\n",
		WLCNTVAL(wlc->pub->_cnt->atim_suppress_count),
		WLCNTVAL(wlc->pub->_cnt->bcn_template_not_ready),
		WLCNTVAL(wlc->pub->_cnt->bcn_template_not_ready_done),
		wlc->mbss->skip_bcntpl_upd);

	bcm_bprintf(b, "Late TBTT counter %d\n",
		WLCNTVAL(wlc->pub->_cnt->late_tbtt_dpc));
	if (BSSCFG_EXTRA_VERBOSE && wlc->clk) {
		bcm_bprintf(b, "MBSS shared memory offsets and values:\n");
		SHOW_SHM(wlc, b, M_MBS_BSSID0(wlc), "BSSID0");
		SHOW_SHM(wlc, b, M_MBS_BSSID1(wlc), "BSSID1");
		SHOW_SHM(wlc, b, M_MBS_BSSID2(wlc), "BSSID2");
		SHOW_SHM(wlc, b, M_MBS_NBCN(wlc), "BCN_COUNT");
		SHOW_SHM(wlc, b, SHM_MBSS_BC_FID0(wlc), "BC_FID0");
		SHOW_SHM(wlc, b, SHM_MBSS_BC_FID1(wlc), "BC_FID1");
		SHOW_SHM(wlc, b, SHM_MBSS_BC_FID2(wlc), "BC_FID2");
		SHOW_SHM(wlc, b, SHM_MBSS_BC_FID3(wlc), "BC_FID3");
		SHOW_SHM(wlc, b, M_MBS_PRETBTT(wlc), "PRE_TBTT");
		SHOW_SHM(wlc, b, SHM_MBSS_SSID_LEN0(wlc), "SSID_LEN0");
		SHOW_SHM(wlc, b, SHM_MBSS_SSID_LEN1(wlc), "SSID_LEN1");
		SHOW_SHM(wlc, b, M_HOST_FLAGS(wlc), "M_HOST1");
		SHOW_SHM(wlc, b, M_HOST_FLAGS2(wlc), "M_HOST2");
	}

	FOREACH_BSS(wlc, idx, bsscfg) {
		bcm_bprintf(b, "\n-------- BSSCFG %d (%p) --------\n", idx, bsscfg);
		wlc_mbss_bsscfg_dump(mbss, bsscfg, b);
	}

	return BCME_OK;
}
#endif

/* Use to set a specific SSID length */
static void
wlc_mbss_ssid_len_set(wlc_info_t *wlc, int idx, uint8 in_val)
{
	uint16 tmp_val;

	tmp_val = wlc_read_shm(wlc, _MBSS_SSID_LEN_SELECT(wlc, idx));
	_MBSS_SSID_LEN_SET(idx, tmp_val, in_val);
	wlc_write_shm(wlc, _MBSS_SSID_LEN_SELECT(wlc, idx), tmp_val);
}

void
wlc_mbss_shm_ssid_upd(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint16 *base)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	int max_trans_bss = 0;

	ASSERT(MBSS_ENAB(wlc->pub));

	if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
		max_trans_bss = 1;
	} else {
		max_trans_bss = WLC_MAX_AP_BSS(wlc->pub->corerev);
	}

	/* Update based on uCode index of BSS */
	if (MBSS_ENAB(wlc->pub)) {
		wlc_mbss16_updssid(wlc, cfg);
		/* tell ucode where to find the probe responses */
		if (D11REV_GE(wlc->pub->corerev, 40)) {
			wlc_write_shm(wlc, M_MBS_PRS_TPLPTR(wlc),
				MBSS_PRS_BLKS_START(wlc, max_trans_bss));
		} else { /* for corerev >= 16 the value is in multiple of 4 */
			wlc_write_shm(wlc, M_MBS_PRS_TPLPTR(wlc),
				MBSS_PRS_BLKS_START(wlc, max_trans_bss) >> 2);
		}

		wlc_write_shm(wlc, SHM_MBSS_BC_FID1(wlc), WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
	} else {
		int uc_idx;
		bss_mbss_info_t *bmi = BSS_MBSS_INFO(mbss, cfg);

		ASSERT(bmi != NULL);

		uc_idx = bmi->_ucidx;

		*base = SHM_MBSS_SSID_ADDR(wlc, uc_idx);	/* Update base addr for ssid */
		wlc_mbss_ssid_len_set(wlc, uc_idx, cfg->SSID_len);
	}
}

static void
wlc_mbss_bcmc_free_cb(wlc_info_t *wlc, void *pkt, uint txs)
{
	int err;
	wlc_bsscfg_t *cfg;
	uint16 frameid, pkt_seq;

	ASSERT(MBSS_ENAB(wlc->pub));

	cfg = wlc_bsscfg_find(wlc, WLPKTTAGBSSCFGGET(pkt), &err);

	/* if bsscfg or scb are stale or bad, then ignore this pkt for acctg purposes */
	if (!err && cfg) {
		frameid = wlc_get_txh_frameid(wlc, pkt);
		pkt_seq = D11_TXFID_GET_SEQ(wlc, frameid);

		/* Check if this was the last frame added to the BCMC fifo */
		if ((cfg->bcmc_fid != INVALIDFID) &&
			(D11_TXFID_GET_SEQ(wlc, cfg->bcmc_fid) == pkt_seq)) {
			cfg->bcmc_fid = INVALIDFID;
		}

		/* Check if this was last frame uCode knew about */
		if ((cfg->bcmc_fid_shm != INVALIDFID) &&
			(D11_TXFID_GET_SEQ(wlc, cfg->bcmc_fid_shm) == pkt_seq)) {
			cfg->bcmc_fid_shm = INVALIDFID;
			WL_MBSS(("wl%d.%d: BCMC packet freed that was not accounted for, fid: %d\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(cfg), frameid));
		}
	}
}

void
wlc_mbss_txq_update_bcmc_counters(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *p)
{
	bss_mbss_info_t *bmi;

	ASSERT(MBSS_ENAB(wlc->pub));

	bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	ASSERT(bmi != NULL);

	bmi->mc_fifo_pkts++;
	WLF2_PCB1_REG(p, WLF2_PCB1_MBSS_BCMC);

#ifdef WLCNT
	if (bmi->mc_fifo_pkts > bmi->cnt->mc_fifo_max)
		bmi->cnt->mc_fifo_max = bmi->mc_fifo_pkts;
	bmi->cnt->bcmc_count++;
#endif /* WLCNT */
}

void
wlc_mbss_increment_ps_trans_cnt(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	bss_mbss_info_t *bmi;

	ASSERT(MBSS_SUPPORT(wlc->pub));

	bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	ASSERT(bmi != NULL);

	WLCNTINCR(bmi->cnt->ps_trans);
}

static void
wlc_mbss16_setup(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	uint8 *bcn;
	void *pkt;
	uint16 tim_offset;
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(wlc->mbss, cfg);

	ASSERT(bmi != NULL);

	/* find the TIM elt offset in the bcn template, push to shm */
	pkt = SPT_LATEST_PKT(bmi->bcn_template);
	bcn = (uint8 *)(PKTDATA(wlc->osh, pkt));
	tim_offset = (uint16)(bmi->bcn_template->tim_ie - bcn);
	/* we want it less the actual ssid length.
	 * SSID length is 0 for closed network.
	 */
	if ((!cfg->closednet && !cfg->nobcnssid) ||
		!BSSCFG_IS_EMPTY_SSID_SUPPORTED_ON_CLOSED_NW(cfg)) {
		tim_offset -= cfg->SSID_len;
	}
	if (D11REV_GE(wlc->pub->corerev, 128)) {
		tim_offset += (D11AC_PHY_HDR_LEN - D11_PHY_HDR_LEN);
		/* add additional 4 bytes to accommodate bcnlen in template memory */
		tim_offset += 4;
	} else if (D11REV_GE(wlc->pub->corerev, 40)) {
		/* not sure what do to for 4360... */
		tim_offset -= D11_TXH_LEN_EX(wlc);
		tim_offset += 8;
	}
	else {
		/* and less the D11_TXH_LEN too */
		tim_offset -= D11_TXH_LEN;
	}

	wlc_write_shm(wlc, M_TIMBPOS_INBEACON(wlc), tim_offset);

}

/* Write the BSSCFG's beacon template into HW */
static void
wlc_mbss16_write_beacon(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	wlc_pkt_t pkt;
	uint32 ucidx;
	int start;
	uint32 len;
	uint8 * pt;
	d11actxh_t *txh = NULL;
	d11actxh_rate_t* local_rate_info;
	uint8 phy_hdr[D11AC_PHY_HDR_LEN];
	int  bcn_tmpl_len;
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(mbss, cfg);
	uint16 tso_hdr_size;

	ASSERT(bmi != NULL);

	bcn_tmpl_len = wlc->pub->bcn_tmpl_len;

	bzero(phy_hdr, sizeof(phy_hdr));

	ucidx = bmi->_ucidx;
	ASSERT(ucidx != (uint32)WLC_MBSS_BSSCFG_IDX_INVALID);

	ASSERT(bmi->bcn_template->latest_idx >= 0);
	ASSERT(bmi->bcn_template->latest_idx < WLC_SPT_COUNT_MAX);

	pkt = SPT_LATEST_PKT(bmi->bcn_template);
	ASSERT(pkt != NULL);

	start = BCN0_TPL_BASE_ADDR + (ucidx * bcn_tmpl_len);
	if (D11REV_GE(wlc->pub->corerev, 128)) {
		uint8 offset = wlc_template_plcp_offset(wlc, wlc->bcn_rspec);

		pt = (uint8 *) PKTDATA(wlc->osh, pkt);
		bcopy(pt, &phy_hdr[offset], D11_PHY_HDR_LEN);
		pt += D11_PHY_HDR_LEN;
		len = PKTLEN(wlc->osh, pkt) - D11_PHY_HDR_LEN;

	} else if (D11REV_GE(wlc->pub->corerev, 40)) {
		uint8 offset = wlc_template_plcp_offset(wlc, wlc->bcn_rspec);

		/* Get pointer TX header and build the phy header */
		pt = (uint8 *) PKTDATA(wlc->osh, pkt);
#ifdef WLTOEHW
		/* Skip overhead */
		tso_hdr_size = WLC_TSO_HDR_LEN(wlc, (d11ac_tso_t*)pt);
		pt += tso_hdr_size;
#else
		tso_hdr_size = 0;
#endif /* WLTOEHW */
		len = PKTLEN(wlc->osh, pkt) - tso_hdr_size;

		txh = (d11actxh_t *) pt;
		local_rate_info = WLC_TXD_RATE_INFO_GET(txh, wlc->pub->corerev);

		bcopy(local_rate_info[0].plcp,
		      &phy_hdr[offset], D11_PHY_HDR_LEN);

		/* Now get the MAC frame */
		pt += D11_TXH_LEN_EX(wlc);
		len -= D11_TXH_LEN_EX(wlc);
	}
	else {
		pt = ((uint8 *)(PKTDATA(wlc->osh, pkt)) + D11_TXH_LEN);
		len = PKTLEN(wlc->osh, pkt) - D11_TXH_LEN;
	}

	bmi->bcn_len = len;
	WL_MBSS(("wl%d: %s: bcn_modified on bsscfg %d len:%d idx:%d\n",
			wlc->pub->unit, __FUNCTION__, WLC_BSSCFG_IDX(cfg), len, ucidx));
	if (len > bcn_tmpl_len) {
		WL_ERROR(("wl%d.%d: %s: buffer too short len[%d] > bcn_tmpl_len[%d] \n",
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__, len, bcn_tmpl_len));
	}
	ASSERT(len <= bcn_tmpl_len);

	if (D11REV_GE(wlc->pub->corerev, 128)) {
		/* bcn len */
		len += D11AC_PHY_HDR_LEN;
		/* Update beacon length, within first 4 bytes for beacon template memory */
		wlc_write_template_ram(wlc, start, sizeof(len), &len);

		start += sizeof(uint32);
		/* Write the phy header (PLCP) first */
		wlc_write_template_ram(wlc, start, D11AC_PHY_HDR_LEN, phy_hdr);

		start += D11AC_PHY_HDR_LEN;
		len -= D11AC_PHY_HDR_LEN;
		/* Now, write the remaining beacon frame */
		wlc_write_template_ram(wlc, start, (len + 3) & (~3), pt);
	} else if (D11REV_GE(wlc->pub->corerev, 40)) {
		/* bcn len */
		wlc_write_shm(wlc, SHM_MBSS_BCNLEN0(wlc) + (2 * ucidx), len + D11AC_PHY_HDR_LEN);

		/* Write the phy header (PLCP) first */
		wlc_write_template_ram(wlc, start, D11AC_PHY_HDR_LEN, phy_hdr);

		/* Now, write the remaining beacon frame */
		wlc_write_template_ram(wlc, start + D11AC_PHY_HDR_LEN, (len + 3) & (~3), pt);

	} else {
		/* bcn len */
		wlc_write_shm(wlc, SHM_MBSS_BCNLEN0(wlc) + (2 * ucidx), len);

		wlc_write_template_ram(wlc, start, (len + 3) & (~3), pt);
	}

	/* Push the ssidlen out to shm after bcnlen pushed */
	wlc_mbss16_updssid_len(wlc, cfg);

	bmi->bcn_template->bcn_modified = FALSE;
}

/* Committing FID to SHM; move driver's value to bcmc_fid_shm */
static void
bcmc_fid_shm_commit(wlc_bsscfg_t *bsscfg)
{
	bsscfg->bcmc_fid_shm = bsscfg->bcmc_fid;
	bsscfg->bcmc_fid = INVALIDFID;
}

/* Assumes SW beaconing active */
#define BSS_BEACONING(cfg) ((cfg) && BSSCFG_AP(cfg) && (cfg)->up)

/* MBSS16 MI_TBTT and MI_DTIM_TBTT handler */
int
wlc_mbss16_tbtt(wlc_info_t *wlc, uint32 macintstatus)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	bool dtim;
	int cfgidx;
	int ucidx;
	wlc_bsscfg_t *cfg = NULL;
	uint16 beacon_count = 0;
	uint16 dtim_map = 0;
	uint32 time_left_for_next_tbtt_us; /* time left for next TBTT */
	uint32 tsf_timerlow_us; /* current tfs */
	uint32 beacon_period_us; /* beacon period in us */
	bool ucode_bcn_suspend = FALSE; /* Indicates whether driver have to suspend ucode
					 * for beaconing.
					 */
	bool ucode_bcn_suspended = FALSE; /* Indicates whether ucode is in suspended
					   * state for beaconing.
					   */
	bss_mbss_info_t *bmi;
	uint16 update_count = 0;
	uint32 beacon_offset_us, tsf_h, tsf_l;

	ASSERT(MBSS_SUPPORT(wlc->pub));
	BCM_REFERENCE(tsf_timerlow_us);

	dtim = ((macintstatus & MI_DTIM_TBTT) != 0);

#ifdef RADIO_PWRSAVE
	if (!dtim && wlc_radio_pwrsave_bcm_cancelled(wlc->ap)) {
		wlc_write_shm(wlc, M_MBS_NBCN(wlc), 0);
		WL_INFORM(("wl%d: radio pwrsave skipping bcn.\n", wlc->pub->unit));
		return 0;
	}
#endif /* RADIO_PWRSAVE */

	/* Calculating the time left for next TBTT */
	beacon_period_us = wlc->default_bss->beacon_period * DOT11_TU_TO_US;
	/*
	 *	||	      |		||
	 *	||============|=========||
	 *	||	      |<-- D -->||
	 *	 A	      C		B
	 * A - last tbtt TSF
	 * B - Next tbtt TSF
	 * C - offset since last TBTT
	 * D - pre-tbtt threshold
	 * (B - A) - beacon interval (BI)
	 * (BI - C) <= D, check if pre-TBTT interrupt is close to TBTT
	 */
	wlc_read_tsf(wlc, &tsf_l, &tsf_h);
	/* offset to last tbtt */
	beacon_offset_us = wlc_calc_tbtt_offset(wlc->default_bss->beacon_period, tsf_h, tsf_l);
	tsf_timerlow_us = R_REG(wlc->osh, D11_TSFTimerLow(wlc));
	time_left_for_next_tbtt_us = beacon_period_us - beacon_offset_us;

	if (time_left_for_next_tbtt_us <= MBSS_PRE_TBTT_MIN_THRESH_us) {
		WL_MBSS(("wl%d: %s: pre-tbtt interrupt fired close to tbtt %d "
			"tsf:%08x_%08x offset:%08x cnt:%d\n",
			wlc->pub->unit, __FUNCTION__, time_left_for_next_tbtt_us,
			tsf_h, tsf_l, beacon_offset_us, wlc->mbss->skip_bcntpl_upd));
		wlc->mbss->skip_bcntpl_upd++;
		ucode_bcn_suspend = TRUE;
	}

	/* Traverse the bsscfg's
	 * create a count of "active" bss's
	 *
	 * if we're at a DTIM:
	 * create a DTIM map,  push "last" bc/mc fid's to shm
	 *
	 * if a beacon has been modified push to shm
	 */
	for (cfgidx = 0; cfgidx < WLC_MAXBSSCFG; cfgidx++) {
		cfg = wlc->bsscfg[cfgidx];
		if (!BSS_BEACONING(cfg))
			continue;

		bmi = BSS_MBSS_INFO(mbss, cfg);
		ASSERT(bmi != NULL);

		if (BEACONING_ENABLED(wlc, cfg)) {
			if (bmi->bcn_template->latest_idx < 0) {
				continue;
			}
			ASSERT(bmi->bcn_template->latest_idx < WLC_SPT_COUNT_MAX);
			++beacon_count;
		}

		ucidx = bmi->_ucidx;
		ASSERT(ucidx != WLC_MBSS_BSSCFG_IDX_INVALID);
		/* Update BCMC flag in the beacon. */
		if (dtim && (cfg->bcmc_fid != INVALIDFID)) {
			uint fid_addr;

			dtim_map |= NBITVAL(ucidx);
			fid_addr = SHM_MBSS_BC_FID_ADDR16(wlc, ucidx);
			if (ucode_bcn_suspend && !ucode_bcn_suspended) {
				ucode_bcn_suspended = TRUE;
				/* Suspending ucode from beaconing for next TBTT
				* due to we are updating bcmc_fid in shared memory
				* very close to next TBTT
				*/
			}
			wlc_write_shm((wlc), fid_addr, cfg->bcmc_fid);
			bcmc_fid_shm_commit(cfg);
		}

		if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && !BSSCFG_IS_MAIN_AP(cfg)) {
			continue;
		}
		if (bmi->bcn_template->bcn_modified == TRUE) {
			if (ucode_bcn_suspend) {
				ucode_bcn_suspended = TRUE;
				/* Suspending ucode from beaconing for next TBTT
				* due to the beacon template is going to be updated
				* in shared memory very close to next TBTT
				*/
				continue;
			}

			/* Update the HW beacon template */
			wlc_mbss16_write_beacon(wlc, cfg);
			update_count++;

			/* Update beacon count tracker with intermediate count.
			 * Note: mbss16_beacon_count must be updated _after_
			 * the call to wlc_mbss16_write_beacon() since that
			 * function is checking mbss16_beacon_count for 0
			 * in order to clear the ssid_len.
			 */
			mbss->mbss16_beacon_count = beacon_count;
		}
		if (update_count == 1) {
			wlc_mbss16_setup(wlc, cfg);
		}
	} /* cfgidx loop */

	if (ucode_bcn_suspended == TRUE) {
		return 0;
	}

#ifdef WL_MBSSID
	if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
		wlc_bsscfg_t *bsscfg;
		uint32 start;
		uint16 val;
		int i;

		wlc_write_shm(wlc, M_MBS_NBCN(wlc), beacon_count ? 1 : 0);
		wlc_write_shm(wlc, M_MBS_BSSIDNUM(wlc), WLC_MAXBSSCFG);
		FOREACH_UP_AP(wlc, i, bsscfg) {
			if (!BSSCFG_IS_MAIN_AP(bsscfg)) {
				bmi = BSS_MBSS_INFO(mbss, bsscfg);
				ucidx = bmi->_ucidx;
				/* Clear the ssid len field of all MBSSes except main bsscfg.
				 * Len is store per 2 ucindexes in the ssidlen_blk, address
				 * goes also per 2, so add ucidc with lower bit cleared to
				 * base address, then read, clear and write back
				 */
				start = M_MBS_SSIDLEN_BLK(wlc) + (ucidx & 0xFE);
				val = wlc_read_shm(wlc, start);
				if (ucidx & 0x01) {
					val &= 0xff;

				} else {
					val &= 0xff00;
				}
				wlc_write_shm(wlc, start, val);
			}
		}
	} else
#endif /* WL_MBSSID */
	{
		wlc_write_shm(wlc, M_MBS_NBCN(wlc), beacon_count);
		wlc_write_shm(wlc, M_MBS_BSSIDNUM(wlc), beacon_count);
	}

	/* Update beacon count tracker with final count.
	 * This could be resetting to 0 if no BSS are beaconing
	 */
	mbss->mbss16_beacon_count = beacon_count;

	if (dtim) {
		wlc_write_shm(wlc, M_MBS_PIO_BCBMP(wlc), dtim_map);

	}
	return 0;
}

/* Write the BSSCFG's probe response template into HW, suspend MAC if requested */
static void
wlc_mbss16_write_prbrsp(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool suspend)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	wlc_pkt_t pkt;
	uint32 ucidx;
	int start;
	uint16 len;
	uint8 * pt;
	int  bcn_tmpl_len = wlc->pub->bcn_tmpl_len;
	bss_mbss_info_t *bmi;

	ASSERT(MBSS_SUPPORT(wlc->pub));

	bmi = BSS_MBSS_INFO(mbss, cfg);
	ASSERT(bmi != NULL);

	ucidx = bmi->_ucidx;
	ASSERT(ucidx != (uint32)WLC_MBSS_BSSCFG_IDX_INVALID);

	pkt = bmi->probe_template;
	ASSERT(pkt != NULL);

	WL_MBSS(("%s: wl%d.%d %smodified %d\n", __FUNCTION__, wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
	         suspend ? "w/suspend " : "", bmi->prb_modified));

	/* probe response */
	if (bmi->prb_modified == TRUE) {
		if (suspend)
			wlc_suspend_mac_and_wait(wlc);

		start = MBSS_PRS_BLKS_START(wlc, WLC_MAX_AP_BSS(wlc->pub->corerev)) +
		        (ucidx * bcn_tmpl_len);

		if (D11REV_GE(wlc->pub->corerev, 40)) {
#ifdef WLTOEHW
			uint16 tso_hdr_size;
			d11ac_tso_t *tso_hdr;

			tso_hdr = (d11ac_tso_t *)PKTDATA(wlc->osh, pkt);
			tso_hdr_size = (uint16) (wlc->toe_bypass ? 0 :
			                         wlc_tso_hdr_length(tso_hdr));
			PKTPULL(wlc->osh, pkt, tso_hdr_size);
#endif /* WLTOEHW */
			pt = ((uint8 *)(PKTDATA(wlc->osh, pkt)) + (D11_TXH_LEN_EX(wlc)));
			len = PKTLEN(wlc->osh, pkt) - (D11_TXH_LEN_EX(wlc));
		}
		else {
			pt = ((uint8 *)(PKTDATA(wlc->osh, pkt)) + D11_TXH_LEN);
			len = PKTLEN(wlc->osh, pkt) - D11_TXH_LEN;
		}

		ASSERT(len <= bcn_tmpl_len);

		wlc_write_template_ram(wlc, start, (len + 3) & (~3), pt);
		/* probe response len */
		wlc_write_shm(wlc, M_MBS_PRSLEN_BLK(wlc) + (2 * ucidx), len);

		bmi->prb_modified = FALSE;

		if (suspend)
			wlc_enable_mac(wlc);
	}
}

void
wlc_mbss_update_probe_resp(wlc_info_t *wlc, wlc_bsscfg_t *cfg, bool suspend)
{
	uint8 *pbody;
	wlc_pkt_t pkt;
	int len = wlc->pub->bcn_tmpl_len;
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(wlc->mbss, cfg);

	ASSERT(bmi != NULL);

	/* Probe response template includes everything from the PLCP header on */
	if ((pkt = bmi->probe_template) == NULL) {
		pkt = wlc_frame_get_mgmt(wlc, FC_PROBE_RESP, &ether_null,
		                         &cfg->cur_etheraddr, &cfg->BSSID, len, &pbody);
		if (pkt == NULL) {
			WL_ERROR(("Could not allocate SW probe template\n"));
			return;
		}
		bmi->probe_template = pkt;
	} else {
		/* Pull back PLCP and TX headers since wlc_d11hdrs puts them back */
		PKTPULL(wlc->osh, pkt, D11_TXH_LEN_EX(wlc));
		/* PKTDATA is now at start of D11 hdr; find packet body */
		pbody = (uint8 *)PKTDATA(wlc->osh, pkt) + DOT11_MGMT_HDR_LEN;
	}

	/* At this point, PKTDATA is at start of D11 hdr; pbody past D11 hdr */

	/* Generate probe body */
	wlc_bcn_prb_body(wlc, FC_PROBE_RESP, cfg, pbody, &len, NULL);

	/* Set the length and set up the D11, PLCP and transmit headers */
	PKTSETLEN(wlc->osh, pkt, len + DOT11_MGMT_HDR_LEN);
	wlc_d11hdrs(wlc, pkt, wlc->band->hwrs_scb, FALSE, 0, 0, TX_ATIM_FIFO, 0, NULL, NULL, 0);

	bmi->prb_modified = TRUE;

	if (HWPRB_ENAB(cfg)) {
		/* Update HW template for MBSS16 */
		wlc_mbss16_write_prbrsp(wlc, cfg, suspend);
	}
}

static void
wlc_mbss16_updssid(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	uint32 start;
	uint32 ssidlen = cfg->SSID_len;
	uint32 swplen;
	bss_mbss_info_t *bmi;
	int8 ucidx;
	uint8 ssidbuf[DOT11_MAX_SSID_LEN];

	ASSERT(MBSS_SUPPORT(wlc->pub));

	bmi = BSS_MBSS_INFO(mbss, cfg);
	ASSERT(bmi != NULL);

	ucidx = bmi->_ucidx;
	ASSERT((ucidx >= 0) && (ucidx <= WLC_MBSS_UCIDX_MASK(wlc->pub->corerev)));

	/* push SSID length 0 for hidden network */
	if (D11REV_GE(wlc->pub->corerev, 128) &&
		(cfg->closednet || cfg->nobcnssid)) {
		ssidlen = 0;
	}
	/* push ssid, ssidlen out to ucode Search Engine */
	start = SHM_MBSS_SSIDSE_BASE_ADDR(wlc) + (ucidx * SHM_MBSS_SSIDSE_BLKSZ(wlc));
	/* search mem length field is always little endian */
	swplen = htol32(ssidlen);
	/* invent new function like wlc_write_shm using OBJADDR_SRCHM_SEL */
	wlc_bmac_copyto_objmem(wlc->hw, start, &swplen, SHM_MBSS_SSIDLEN_BLKSZ, OBJADDR_SRCHM_SEL);

	bzero(ssidbuf, DOT11_MAX_SSID_LEN);
	bcopy(cfg->SSID, ssidbuf, cfg->SSID_len);

	start += SHM_MBSS_SSIDLEN_BLKSZ;
	wlc_bmac_copyto_objmem(wlc->hw, start, ssidbuf, SHM_MBSS_SSID_BLKSZ, OBJADDR_SRCHM_SEL);

	/* push ssidlen out to shm
	 * XXX: again? Already sent to search engine ...
	 * XXX: awkward rmw sequence
	 */

	/* Defer pushing the ssidlen out to shm until the beacon len is pushed.
	 * Except ssidlen is 0, push ssidlen 0 imply disabling beaconing on that
	 * BSS.
	 */
	if (ssidlen == 0)
		wlc_mbss16_updssid_len(wlc, cfg);
}

static void
wlc_mbss16_updssid_len(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	uint32 start;
	uint16 val;
	uint32 ssidlen = cfg->SSID_len;
	bss_mbss_info_t *bmi;
	int8 ucidx;
	int8 i;

	ASSERT(MBSS_SUPPORT(wlc->pub));

	/*
	 * If there were not beacons sent in a previous tbtt then we need to
	 * make sure that all of the ssid_len values is shm are reset to 0.
	 * When brining down the last bss, the last ssid_len is not reset to 0
	 * to avoid a race issue with the ucode and nbcn count.  Please refer to
	 * the code in wlc_mbss_bsscfg_down()
	 */
	if (wlc->mbss->mbss16_beacon_count == 0) {
		for (i = 0; i < WLC_MAX_AP_BSS(wlc->pub->corerev); i += 2) {
			start = M_MBS_SSIDLEN_BLK(wlc) + i;
			wlc_write_shm(wlc, start, 0);
		}
	}

	bmi = BSS_MBSS_INFO(wlc->mbss, cfg);
	ASSERT(bmi != NULL);

	ucidx = bmi->_ucidx;
	ASSERT((ucidx >= 0) && (ucidx <= WLC_MBSS_UCIDX_MASK(wlc->pub->corerev)));

	/* push SSID length 0 for hidden network */
	if (D11REV_GE(wlc->pub->corerev, 128) &&
		(cfg->closednet || cfg->nobcnssid)) {
		ssidlen = 0;
	}
	/* push ssidlen out to shm */
	start = M_MBS_SSIDLEN_BLK(wlc) + (ucidx & 0xFE);
	val = wlc_read_shm(wlc, start);
	/* set bit indicating closed net if appropriate */
	if (cfg->up && (cfg->closednet || cfg->nobcnssid)) {
		ssidlen |= SHM_MBSS_CLOSED_NET(wlc);
	}

	if (ucidx & 0x01) {
		val &= 0xff;
		val |= ((uint8)ssidlen << 8);

	} else {
		val &= 0xff00;
		val |= (uint8)ssidlen;
	}

	wlc_write_shm(wlc, start, val);
}

/*
 * Updates SHM for closed net
 * Must be called for enabled network only
 */
void
wlc_mbss16_upd_closednet(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	uint32 start;
	bss_mbss_info_t *bmi;
	int8 ucidx;
	uint16 val;
	uint8 shift = 0;

	ASSERT(MBSS_SUPPORT(wlc->pub));

	bmi = BSS_MBSS_INFO(mbss, cfg);
	ASSERT(bmi != NULL);

	ucidx = bmi->_ucidx;
	ASSERT((ucidx >= 0) && (ucidx <= WLC_MBSS_UCIDX_MASK(wlc->pub->corerev)));

	if (ucidx & 0x01)
		shift = 8;
	start = M_MBS_SSIDLEN_BLK(wlc) + (ucidx & 0xFE);
	val = wlc_read_shm(wlc, start);
	val = val & ~(SHM_MBSS_CLOSED_NET(wlc) << shift);
	if (cfg->closednet || cfg->nobcnssid)
		val = val | (SHM_MBSS_CLOSED_NET(wlc) << shift);
	wlc_write_shm(wlc, start, val);
}

int
wlc_mbss_validate_mac(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct ether_addr *addr)
{
	struct ether_addr temp, local_vetherbase, primary_addr;
	int ucidx;
	int ii;
	wlc_bsscfg_t *bsscfg;

	ASSERT(BSSCFG_AP(cfg));
	ASSERT(!ETHER_ISNULLADDR(&wlc->mbss->vether_base));

	/* XXX JFH - TO DO!!! - We need to treat a NULL MAC
	 * as a way to clear/reset the mac addresses for
	 * bss configs.
	 */

	/* Has the primary config's address been set? */
	if (ETHER_ISNULLADDR(&wlc->primary_bsscfg->cur_etheraddr))
		return BCME_BADADDR;

	if (MBSS_IGN_MAC_VALID(wlc->pub)) {
		WL_PRINT(("%s: wl%d.%d ignore mac validation\n", __FUNCTION__,
			wlc->pub->unit, WLC_BSSCFG_IDX(cfg)));
		return BCME_OK;
	}

	/* verify that the upper bits of the address
	 * match our base
	 */
	bcopy(&wlc->mbss->vether_base, &local_vetherbase, ETHER_ADDR_LEN);
	local_vetherbase.octet[5] &= ~(WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
	bcopy(addr, &temp, ETHER_ADDR_LEN);
	temp.octet[5] &= ~(WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
	if (bcmp(&temp, &local_vetherbase, ETHER_ADDR_LEN)) {
		/* new format of VIF addr,
		 * check if the first and applicable
		 */
		bcopy(&wlc->pub->cur_etheraddr, &primary_addr, ETHER_ADDR_LEN);
		primary_addr.octet[5] &= ~(WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
		ETHER_SET_LOCALADDR(&primary_addr);

		if (bcmp(&primary_addr, &local_vetherbase, ETHER_ADDR_LEN)) {
			/* vether_base different from the primary addr means that vether_base is
			 * regenerated by previous input VIF addr and not derived from the primary
			 * intf in system startup
			 * the input addr is not the first so not applicable
			 */
			return BCME_BADADDR;
		} else {
			/* the first input VIF addr, check if index conflicts
			 */
			if (EADDR_TO_UC_IDX(*addr, WLC_MBSS_UCIDX_MASK(wlc->pub->corerev)) ==
			    EADDR_TO_UC_IDX(wlc->primary_bsscfg->cur_etheraddr,
					WLC_MBSS_UCIDX_MASK(wlc->pub->corerev))) {
				return BCME_BADADDR;
			}

			/* regen vether_base according to the input VIF addr
			 */
			temp.octet[5] |= ((wlc->primary_bsscfg->cur_etheraddr.octet[5] + 1) &
				WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
			bcopy(&temp, &wlc->mbss->vether_base, ETHER_ADDR_LEN);
			return BCME_OK;
		}
	}

	/* verify that there isn't a
	 * collision with any other configs.
	 */
	ucidx = EADDR_TO_UC_IDX(*addr, WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));

	FOREACH_BSS(wlc, ii, bsscfg) {
		if ((bsscfg == cfg) ||
		    (ETHER_ISNULLADDR(&bsscfg->cur_etheraddr))) {
			continue;
		}
		if (ucidx == EADDR_TO_UC_IDX(bsscfg->cur_etheraddr,
		    WLC_MBSS_UCIDX_MASK(wlc->pub->corerev))) {
			return BCME_BADADDR;
		}
	}

	/* make sure the index is in bound */
	if (MBSS_ENAB(wlc->pub) &&
	    ((uint32)AP_BSS_UP_COUNT(wlc) >= (uint32)WLC_MAX_AP_BSS(wlc->pub->corerev))) {
		return BCME_BADADDR;
	}

	return BCME_OK;
}

void
wlc_mbss_reset_mac(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	int ii;
	wlc_bsscfg_t *bsscfg;

	/* regardless of a clash, every time the user sets
	 * the primary config's cur_etheraddr, we will clear all
	 * all of the secondary config ethernet addresses.  If we
	 * don't do this, we'll have to prevent the user from
	 * configuring a MAC for the primary that collides(ucidx)
	 * with secondary configs.  this is way easier and is
	 * documented this way in the IOCTL/IOVAR manual.
	 */
	FOREACH_BSS(wlc, ii, bsscfg) {
		if (BSSCFG_AP(bsscfg))
			bcopy(&ether_null, &bsscfg->cur_etheraddr, ETHER_ADDR_LEN);
	}

	/* also re-generate the base address for MBSS */
	bcopy(&wlc->pub->cur_etheraddr,
		&wlc->mbss->vether_base, ETHER_ADDR_LEN);
	wlc->mbss->vether_base.octet[5] =
		(wlc->mbss->vether_base.octet[5] &
			~(WLC_MBSS_UCIDX_MASK(wlc->pub->corerev))) |
		((wlc->mbss->vether_base.octet[5] + 1) &
			WLC_MBSS_UCIDX_MASK(wlc->pub->corerev));
	/* force locally administered address */
	ETHER_SET_LOCALADDR(&wlc->mbss->vether_base);
}

void
wlc_mbss_set_mac(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_write_mbss_basemac(wlc, cfg);
}

void
wlc_mbss_record_time(wlc_info_t *wlc)
{
	wlc->mbss->last_tbtt_us = R_REG(wlc->osh, D11_TSFTimerLow(wlc));
}

/* return ucode index for BSS */
int
wlc_mbss_bss_idx(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
	wlc_mbss_info_t *mbss = wlc->mbss;
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(mbss, cfg);

	ASSERT(bmi != NULL);

	return bmi->_ucidx;
}

#ifdef WL_MBSSID
/* Multiple BSSID Ind */

#define WLC_MBSS_MBSSID_TLVLEN		3	/* TLV LEN 2 + MAX BSSID Indicator + subelements */
#define WLC_MBSS_MBSSID_SUBELEMENT	2	/* ID + length (+variable data) */
#define WLC_MBSS_MBSSID_SUBELEMENT_ID0	0	/* Nontransmitted BSSID Profile */
#define WLC_MBSS_MBSSID_NBC_LEN		4	/* Nontransmitted BSSID Capability element */
#define WLC_MBSS_MBSSID_IDX_BCN_LEN	5	/* Multiple BSSID-Index element lenght beacon */
#define WLC_MBSS_MBSSID_IDX_PRB_LEN	3	/* Multiple BSSID-Index element lenght proberesp */
#define WLC_MBSS_MBSSID_IE_FIXED_LEN	3	/* Multiple BSSID-IE element length  */
#define WLC_MBSS_MBSSID_NONINHERITANCE_FIXED_LEN	5	/* non inheritance IE fixed len  */

static uint
wlc_mbss_calc_mbssid_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	wlc_bsscfg_t *main_bsscfg = data->cfg;
	wlc_bsscfg_t *bsscfg;
	int total_mbssid_len, len;
	int i;
	uint16 tlv_len;
	wlc_iem_calc_data_t data_local;

	if (!MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
		return BCME_OK;
	}
	if (!BSSCFG_IS_MAIN_AP(main_bsscfg)) {
		return BCME_OK;
	}
	bcopy(data, &data_local, sizeof(wlc_iem_calc_data_t));
	len = wlc_mbss_calc_mbssid_ie_len_local(ctx, data);
	total_mbssid_len = len;
	tlv_len = len;
	FOREACH_UP_AP(wlc, i, bsscfg) {
		if (bsscfg != main_bsscfg) {
			data_local.cfg = bsscfg;
			memset(bsscfg->mbssid_tag_list, 0, sizeof(wlc_mbssid_tag_list_t));
			wlc_iem_fill_mbssid_tag_list(wlc, main_bsscfg, bsscfg, &data_local);
			total_mbssid_len +=
				wlc_mbssid_per_mbss_ie_calc(ctx, data_local.cbparm,
					&data_local, &tlv_len);
		}
	}

	return total_mbssid_len;

}

/* Calculate mbssid IE len for all BSSes. Used for mbssid beacon_len iovar when driver is down */
static uint
wlc_mbss_calc_mbssid_ie_len_all(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	wlc_bsscfg_t *main_bsscfg = data->cfg;
	wlc_bsscfg_t *bsscfg;
	int total_mbssid_len, len;
	int i;
	uint16 tlv_len;
	wlc_iem_calc_data_t data_local;

	if (wlc->clk || !MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
		return BCME_OK;
	}

	bcopy(data, &data_local, sizeof(wlc_iem_calc_data_t));
	len = wlc_mbss_calc_mbssid_ie_len_local(ctx, data);
	total_mbssid_len = len;
	tlv_len = len;
	FOREACH_AP(wlc, i, bsscfg) {
		if (bsscfg != main_bsscfg) {
			data_local.cfg = bsscfg;
			memset(bsscfg->mbssid_tag_list, 0, sizeof(wlc_mbssid_tag_list_t));
			wlc_iem_fill_mbssid_tag_list(wlc, main_bsscfg, bsscfg, &data_local);
			total_mbssid_len +=
				wlc_mbssid_per_mbss_ie_calc(ctx, data_local.cbparm,
					&data_local, &tlv_len);
		}
	}

	return total_mbssid_len;
}

static uint
wlc_mbssid_per_mbss_ie_calc(void *ctx, wlc_iem_cbparm_t *cbparm,
	wlc_iem_calc_data_t *data, uint16 *tlv_len)
{
	wlc_bsscfg_t *bsscfg = data->cfg;
	uint16 total_mbss_len = 0, len, i;
	wlc_mbssid_tag_list_t *mbssid_tag_list = bsscfg->mbssid_tag_list;

	len = wlc_mbss_calc_nontrans_mbssid_subelement_ie_len(ctx, data);
	/* tlv limit reached - mbss ie tag is included */
	if ((*tlv_len + len - 2) > TLV_BODY_LEN_MAX)
	{
		*tlv_len  =  wlc_mbss_calc_mbssid_ie_len_local(ctx, data);
		total_mbss_len += *tlv_len;
	}
	*tlv_len += len;
	total_mbss_len += len;
	data->tag = DOT11_MNG_NONTRANS_BSSID_CAP_ID;
	total_mbss_len += wlc_mbssid_calc_append_subelement(ctx, data, data->tag, tlv_len);
	data->tag = DOT11_MNG_SSID_ID;
	total_mbss_len += wlc_mbssid_calc_append_subelement(ctx, data, data->tag, tlv_len);
	data->tag = DOT11_MNG_MULTIPLE_BSSID_IDX_ID;
	total_mbss_len += wlc_mbssid_calc_append_subelement(ctx, data, data->tag, tlv_len);

	ASSERT(mbssid_tag_list->tags_len <= MAX_TAG_MBSSID_IE);
	ASSERT(mbssid_tag_list->non_inherit_len <= MAX_TAG_MBSSID_IE);
	ASSERT(mbssid_tag_list->non_inherit_ext_len <= MAX_TAG_MBSSID_IE);

	for (i = 0; i < mbssid_tag_list->tags_len; i ++) {
		data->tag = mbssid_tag_list->tag[i];
		len = wlc_mbssid_calc_append_subelement(ctx,
			data, mbssid_tag_list->tag[i], tlv_len);
		total_mbss_len += len;
	}
	if (mbssid_tag_list->non_inherit_ext_len ||
		mbssid_tag_list->non_inherit_len) {
		len = wlc_mbssid_calc_append_subelement(ctx,
			data, DOT11_MNG_MBSSID_NON_INHERITANCE_ID, tlv_len);
		total_mbss_len += len;
	}

	return total_mbss_len;
}

static uint wlc_mbssid_calc_append_subelement(void *ctx, wlc_iem_calc_data_t *data,
	wlc_iem_tag_t tag, uint16 *tlv_len)
{	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	uint16 len;
	wlc_iem_cbe_t build_cb;
	if (tag == DOT11_MNG_NONTRANS_BSSID_CAP_ID) {
		len = wlc_mbss_calc_nontrans_bssid_cap_ie_len(ctx, data);
	} else if (tag == DOT11_MNG_MULTIPLE_BSSID_IDX_ID) {
		len = wlc_mbss_calc_multibssid_index_ie_len(data);
	} else if (tag == DOT11_MNG_MBSSID_NON_INHERITANCE_ID) {
		len = wlc_mbss_calc_multibssid_noninheritance_ie(ctx, data);
	} else {
		wlc_iem_get_tag_build_calc_cb(wlc, tag, &build_cb, FC_BEACON);
		if (build_cb.calc == NULL) {
			return 0;
		}
		len = build_cb.calc(build_cb.ctx, data);
	}

	if ((*tlv_len + len - 2) > TLV_BODY_LEN_MAX) {
		len +=  wlc_mbss_calc_mbssid_ie_len_local(ctx, data);
		len += wlc_mbss_calc_nontrans_mbssid_subelement_ie_len(ctx, data);
		*tlv_len = len;
	} else {
		*tlv_len += len;
	}
	return len;
}

static uint
wlc_mbss_calc_nontrans_mbssid_subelement_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	return TLV_HDR_LEN;
}

static uint
wlc_mbss_calc_mbssid_ie_len_local(void *ctx, wlc_iem_calc_data_t *data)
{
	return WLC_MBSS_MBSSID_IE_FIXED_LEN;
}

static uint
wlc_mbss_calc_nontrans_bssid_cap_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	wlc_bsscfg_t *bsscfg = data->cfg;
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(wlc->mbss, bsscfg);

	return TLV_HDR_LEN + sizeof(bmi->capability);
}

static uint
wlc_mbss_calc_multibssid_index_ie_len(wlc_iem_calc_data_t *data)
{
	if (data->ft == FC_BEACON) {
		return WLC_MBSS_MBSSID_IDX_BCN_LEN;
	} else {
		return  WLC_MBSS_MBSSID_IDX_PRB_LEN;
	}

}
static uint
wlc_mbss_calc_multibssid_noninheritance_ie(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_bsscfg_t *bsscfg = data->cfg;

	return WLC_MBSS_MBSSID_NONINHERITANCE_FIXED_LEN +
		bsscfg->mbssid_tag_list->non_inherit_len +
		bsscfg->mbssid_tag_list->non_inherit_ext_len;
}
static uint
wlc_mbss_calc_multibssid_configuration_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	if (!MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
		return BCME_OK;
	}

	return sizeof(dot11_mbssid_configuration_ie_t);
}

static int
wlc_mbss_write_mbssid_configuration_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	wlc_bsscfg_t *bsscfg = data->cfg;
	dot11_mbssid_configuration_ie_t *mbssid_configuaration_ie;
	int bss_count = 0;
	int i = 0;

	mbssid_configuaration_ie = (dot11_mbssid_configuration_ie_t *)data->buf;

	if (!MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
		return BCME_OK;
	}

	FOREACH_UP_AP(wlc, i, bsscfg) {
		bss_count ++;
	}

	mbssid_configuaration_ie->id = DOT11_MNG_ID_EXT_ID;
	mbssid_configuaration_ie->ext_id = EXT_MNG_MBSSID_CONFIG_ID;
	mbssid_configuaration_ie->len = 3;
	mbssid_configuaration_ie->bssid_count = bss_count;
	mbssid_configuaration_ie->periodicity = 1;

	return BCME_OK;
}

static int
wlc_mbss_write_mbssid_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	wlc_bsscfg_t *main_bsscfg = data->cfg;
	wlc_bsscfg_t *bsscfg;
	int i;
	wlc_iem_build_data_t data_local;
	wlc_iem_calc_data_t calc_data;
	uint16 tlv_len;
	uint16 len;
	dot11_mbssid_ie_t *mbssid_ie;
	int ret;

	if (!MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
		return BCME_OK;
	}

	if (!BSSCFG_IS_MAIN_AP(main_bsscfg)) {
		return BCME_OK;
	}
	bzero(&calc_data, sizeof(wlc_iem_calc_data_t));
	calc_data.cbparm = data->cbparm;
	calc_data.cfg = data->cfg;
	calc_data.ft = data->ft;
	calc_data.tag = data->tag;

	bcopy(data, &data_local, sizeof(wlc_iem_build_data_t));

	len = wlc_mbss_calc_mbssid_ie_len_local(ctx, &calc_data);
	if (len > data_local.buf_len) {
		return BCME_BUFTOOSHORT;
	}
	wlc_mbss_write_mbssid_ie_local(ctx, &data_local);
	data_local.buf += len;
	data_local.buf_len -= len;
	tlv_len = len;

	/* iterate along the ssid cfgs, skip main bsscfg */
	FOREACH_UP_AP(wlc, i, bsscfg) {
		if (bsscfg != main_bsscfg) {
			data_local.cfg = bsscfg;
			calc_data.cfg = bsscfg;
			ret = wlc_mbssid_per_mbss_ie(ctx, &data_local, &calc_data, &tlv_len);
			if (ret != BCME_OK) {
				return ret;
			}
		}
	}
	/* len field in the mbss ie tag - last tlv */
	mbssid_ie  = (dot11_mbssid_ie_t *)(data_local.buf - tlv_len);
	mbssid_ie->len  = tlv_len - TLV_HDR_LEN;

	return BCME_OK;
}

/* No-op mbssid write function to be used when driver is down for beacon_len iovar */
static int
wlc_mbss_write_mbssid_ie_all(void *ctx, wlc_iem_build_data_t *data)
{
	BCM_REFERENCE(ctx);
	BCM_REFERENCE(data);
	return BCME_OK;
}

static int
wlc_mbssid_per_mbss_ie(void *ctx, wlc_iem_build_data_t *data,
		wlc_iem_calc_data_t *calc_data, uint16 *tlv_len)
{
	wlc_bsscfg_t *bsscfg = data->cfg;
	uint16 per_mbss_len = 0, len, len1, i;
	int ret;
	dot11_mbssid_nontrans_subelement_t *subelement;
	dot11_mbssid_ie_t *mbssid_ie;
	wlc_mbssid_tag_list_t *mbssid_tag_list = bsscfg->mbssid_tag_list;

	ASSERT(mbssid_tag_list);

	len = wlc_mbss_calc_nontrans_mbssid_subelement_ie_len(ctx, calc_data);
	if (len > data->buf_len) {
		return BCME_BUFTOOSHORT;
	}
	if ((*tlv_len + len + wlc_mbss_calc_nontrans_bssid_cap_ie_len(ctx, calc_data)
		- TLV_HDR_LEN) > TLV_BODY_LEN_MAX)
	{
		/* len field assignment in the mbss ie tag */
		mbssid_ie  = (dot11_mbssid_ie_t *)(data->buf - *tlv_len);
		mbssid_ie->len  = *tlv_len - TLV_HDR_LEN;

		len1 = wlc_mbss_calc_mbssid_ie_len_local(ctx, calc_data);
		if (len1 > data->buf_len) {
			return BCME_BUFTOOSHORT;
		}
		wlc_mbss_write_mbssid_ie_local(ctx, data);
		data->buf += len1;
		data->buf_len -= len1;
		*tlv_len = len1;
	}

	wlc_mbss_build_nontrans_bssid_subelement_ie(ctx, data);
	data->buf += len;
	data->buf_len -= len;
	*tlv_len += len;
	per_mbss_len = len;

	data->tag = DOT11_MNG_NONTRANS_BSSID_CAP_ID;
	ret = wlc_mbssid_append_subelement(ctx, data, calc_data, data->tag, tlv_len, &per_mbss_len);
	if (ret != BCME_OK) {
		return ret;
	}
	data->tag = DOT11_MNG_SSID_ID;
	ret = wlc_mbssid_append_subelement(ctx, data, calc_data, data->tag, tlv_len, &per_mbss_len);
	if (ret != BCME_OK) {
		return ret;
	}
	data->tag = DOT11_MNG_MULTIPLE_BSSID_IDX_ID;
	ret = wlc_mbssid_append_subelement(ctx, data, calc_data, data->tag, tlv_len, &per_mbss_len);
	if (ret != BCME_OK) {
		return ret;
	}

	ASSERT(mbssid_tag_list->tags_len <= MAX_TAG_MBSSID_IE);
	ASSERT(mbssid_tag_list->non_inherit_len <= MAX_TAG_MBSSID_IE);
	ASSERT(mbssid_tag_list->non_inherit_ext_len <= MAX_TAG_MBSSID_IE);

	for (i = 0; i < mbssid_tag_list->tags_len; i ++) {
		data->tag = mbssid_tag_list->tag[i];
		ret = wlc_mbssid_append_subelement(ctx, data, calc_data,
			data->tag, tlv_len, &per_mbss_len);
		if (ret != BCME_OK) {
			return ret;
		}
	}

	if (mbssid_tag_list->non_inherit_ext_len ||
		mbssid_tag_list->non_inherit_len) {
		ret = wlc_mbssid_append_subelement(ctx, data, calc_data,
			DOT11_MNG_MBSSID_NON_INHERITANCE_ID, tlv_len, &per_mbss_len);
	}

	/* len field assignment in the mbss sub element tag */
	subelement = (dot11_mbssid_nontrans_subelement_t *)(data->buf - per_mbss_len);
	subelement->len = per_mbss_len - TLV_HDR_LEN;
	return BCME_OK;
}

static int
wlc_mbssid_append_subelement(void *ctx, wlc_iem_build_data_t *data,
	wlc_iem_calc_data_t *calc_data,  wlc_iem_tag_t tag,
	uint16 *tlv_len, uint16 *per_mbss_len)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	uint16 len;
	int ret;
	wlc_iem_cbe_t build_cb;

	if (tag == DOT11_MNG_NONTRANS_BSSID_CAP_ID) {
		len = wlc_mbss_calc_nontrans_bssid_cap_ie_len(ctx, calc_data);
	} else if (tag == DOT11_MNG_MULTIPLE_BSSID_IDX_ID) {
		len = wlc_mbss_calc_multibssid_index_ie_len(calc_data);
	} else if (tag == DOT11_MNG_MBSSID_NON_INHERITANCE_ID) {
		len = wlc_mbss_calc_multibssid_noninheritance_ie(ctx, calc_data);
	} else {
		wlc_iem_get_tag_build_calc_cb(wlc, tag, &build_cb, FC_BEACON);
		if (build_cb.calc == NULL) {
			return BCME_OK;
		}
		len = build_cb.calc(build_cb.ctx, calc_data);
	}
	if (len > data->buf_len) {
		return BCME_BUFTOOSHORT;
	}

	if (*tlv_len - TLV_HDR_LEN  + len > TLV_BODY_LEN_MAX) {

		ret = wlc_mbss_update_add_mbssie_and_subelement(ctx, data,
			calc_data, tlv_len, per_mbss_len);
		if (ret != BCME_OK) {
			return ret;
		}
	}
	*tlv_len += len;
	*per_mbss_len += len;

	if (tag == DOT11_MNG_NONTRANS_BSSID_CAP_ID) {
		wlc_mbss_build_nontrans_bssid_cap_ie(ctx, data);
	} else if (tag == DOT11_MNG_MULTIPLE_BSSID_IDX_ID) {
		wlc_mbss_build_multibssid_index_ie(ctx, data);
	} else if (tag == DOT11_MNG_MBSSID_NON_INHERITANCE_ID) {
		wlc_mbss_build_multibssid_noninheritance_ie(ctx, data);
	} else {
		build_cb.build(build_cb.ctx, data);
	}
	data->buf += len;
	data->buf_len -= len;

	return BCME_OK;
}

static int
wlc_mbss_update_add_mbssie_and_subelement(void *ctx, wlc_iem_build_data_t *data,
	wlc_iem_calc_data_t *calc_data, uint16 *tlv_len, uint16 *per_mbss_len)
{
	uint16 len;
	dot11_mbssid_nontrans_subelement_t *subelement;
	dot11_mbssid_ie_t *mbssid_ie;

	/* len field assignment in the mbss ie tag */
	mbssid_ie  = (dot11_mbssid_ie_t *)(data->buf - *tlv_len);
	mbssid_ie->len  = *tlv_len - TLV_HDR_LEN;

	/* len field assignment in the mbss sub element tag */
	subelement = (dot11_mbssid_nontrans_subelement_t *)(data->buf - *per_mbss_len);
	subelement->len = *per_mbss_len - TLV_HDR_LEN;

	len =  wlc_mbss_calc_mbssid_ie_len_local(ctx, calc_data);
	if (len > data->buf_len) {
		return BCME_BUFTOOSHORT;
	}
	wlc_mbss_write_mbssid_ie_local(ctx, data);
	data->buf += len;
	data->buf_len -= len;
	*tlv_len = len;

	len = wlc_mbss_calc_nontrans_mbssid_subelement_ie_len(ctx, calc_data);
	if (len > data->buf_len) {
		return BCME_BUFTOOSHORT;
	}
	wlc_mbss_build_nontrans_bssid_subelement_ie(ctx, data);
	data->buf += len;
	data->buf_len -= len;
	*tlv_len += len;
	*per_mbss_len = len;

	return BCME_OK;
}

static uint
wlc_mbss_build_nontrans_bssid_subelement_ie(void *ctx, wlc_iem_build_data_t *data)
{
	dot11_mbssid_nontrans_subelement_t *subelement;
	subelement = (dot11_mbssid_nontrans_subelement_t *)data->buf;
	subelement->id = 0;
	return BCME_OK;
}

static uint
wlc_mbss_build_nontrans_bssid_cap_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	wlc_bsscfg_t *bsscfg = data->cfg;
	bss_mbss_info_t *bmi = BSS_MBSS_INFO(wlc->mbss, bsscfg);
	uint16 capability;

	if (BSSCFG_AP(bsscfg)) {
		capability = DOT11_CAP_ESS;
#ifdef WL11K
		if (WL11K_ENAB(wlc->pub) && wlc_rrm_enabled(wlc->rrm_info, bsscfg))
			capability |= DOT11_CAP_RRM;
#endif
	}
#ifdef WLP2P
	else if (BSS_P2P_DISC_ENAB(wlc, bsscfg))
		capability = 0;
#endif
	else
		capability = DOT11_CAP_IBSS;
	if (WSEC_ENABLED(bsscfg->wsec) && bsscfg->wsec_restrict) {
		capability |= DOT11_CAP_PRIVACY;
	}
	/* Advertise short preamble capability if we have not been forced long AND we
	 * are not an APSTA
	 */
	if (!APSTA_ENAB(wlc->pub) && BAND_2G(wlc->band->bandtype) &&
		(bsscfg->current_bss->capability & DOT11_CAP_SHORT))
		capability |= DOT11_CAP_SHORT;
	if (wlc->band->gmode && wlc->shortslot)
		capability |= DOT11_CAP_SHORTSLOT;
	if (BSS_WL11H_ENAB(wlc, bsscfg) && BSSCFG_AP(bsscfg))
		capability |= DOT11_CAP_SPECTRUM;

	bmi->capability = htol16(capability);

	if (bcm_write_tlv_safe(DOT11_MNG_NONTRANS_BSSID_CAP_ID, &bmi->capability,
			sizeof(bmi->capability), data->buf, data->buf_len) == data->buf) {
		return BCME_BUFTOOSHORT;
	}
	return BCME_OK;

}

static uint
wlc_mbss_build_multibssid_index_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	wlc_info_t *wlc = mbss->wlc;
	dot11_mbssid_index_ie_t *mbssid_index_ie;
	wlc_bsscfg_t *bsscfg = data->cfg;
	bss_mbss_info_t *bmi;
	bmi = BSS_MBSS_INFO(mbss, bsscfg);
	mbssid_index_ie = (dot11_mbssid_index_ie_t *)data->buf;
	if (data->ft == FC_BEACON) {
		mbssid_index_ie->id = DOT11_MNG_MULTIPLE_BSSID_IDX_ID;
		mbssid_index_ie->len = WLC_MBSS_MBSSID_IDX_BCN_LEN - TLV_HDR_LEN;
		mbssid_index_ie->bssid_index = bmi->_ucidx;
		mbssid_index_ie->dtim_period = bsscfg->associated ?
			bsscfg->current_bss->dtim_period :
			wlc->default_bss->dtim_period;
		mbssid_index_ie->dtim_count = (uint8)wlc_ap_getdtim_count(wlc, bsscfg);
	} else {
		mbssid_index_ie->id = DOT11_MNG_MULTIPLE_BSSID_IDX_ID;
		mbssid_index_ie->len = WLC_MBSS_MBSSID_IDX_PRB_LEN - TLV_HDR_LEN;
		mbssid_index_ie->bssid_index = bmi->_ucidx;
	}
	return BCME_OK;
}

static uint
wlc_mbss_write_mbssid_ie_local(void *ctx, wlc_iem_build_data_t *data)
{
	dot11_mbssid_ie_t *mbssid_ie;
	wlc_mbss_info_t *mbss = (wlc_mbss_info_t *)ctx;
	mbssid_ie = (dot11_mbssid_ie_t *)data->buf;
	mbssid_ie->id = DOT11_MNG_MULTIPLE_BSSID_ID;
	mbssid_ie->maxbssid_indicator = mbss->max_bss_indicator;
	return BCME_OK;
}

static uint
wlc_mbss_build_multibssid_noninheritance_ie(void *ctx, wlc_iem_build_data_t *data)
{
	wlc_bsscfg_t *bsscfg = data->cfg;
	dot11_mbssid_non_inheritance_t *non_inheritance_ie =
		(dot11_mbssid_non_inheritance_t *)data->buf;
	uint8 len = 0;
	int i;

	non_inheritance_ie->id = DOT11_MNG_ID_EXT_ID;
	non_inheritance_ie->ext_id = EXT_MNG_MBSSID_NON_INHERITANCE_ID;
	if (bsscfg->mbssid_tag_list->non_inherit_len) {
		non_inheritance_ie->var_list[len++] =
			bsscfg->mbssid_tag_list->non_inherit_len;
		for (i = 0; i < bsscfg->mbssid_tag_list->non_inherit_len; i++)	{
			non_inheritance_ie->var_list[len++] =
				bsscfg->mbssid_tag_list->non_inherit[i];
		}
	} else {
		non_inheritance_ie->var_list[len++] = 0;
	}

	if (bsscfg->mbssid_tag_list->non_inherit_ext_len) {
		non_inheritance_ie->var_list[len++] =
			bsscfg->mbssid_tag_list->non_inherit_ext_len;
		for (i = 0; i < bsscfg->mbssid_tag_list->non_inherit_ext_len; i++) {
			non_inheritance_ie->var_list[len++] =
				bsscfg->mbssid_tag_list->non_inherit_ext[i];
		}
	} else {
		non_inheritance_ie->var_list[len++] = 0;
	}
	non_inheritance_ie->len = len + 1;  /* length of the IE +1 is to add the ext_id */
	return BCME_OK;
}

/* helper function similar to log2(n) */
static uint wlc_calc_max_bss_indicator(uint number)
{
	int i = 0;

	while (i < 32) {
		if (number & 1) {
			return i;
		}
		i++;
		number = number >> 1;
	}

	return BCME_OK;
}
#endif /* WL_MBSSID */
#endif /* MBSS */
