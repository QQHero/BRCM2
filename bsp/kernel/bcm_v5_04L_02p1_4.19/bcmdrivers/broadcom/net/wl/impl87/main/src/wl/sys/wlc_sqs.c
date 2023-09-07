/*
 * Single stage Queuing and Scheduling module
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
 * $Id: wlc_sqs.c 806786 2022-01-04 02:01:02Z $
 */
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11_cfg.h>
#include <wl_export.h>
#include <wlc_types.h>
#include <wlc.h>
#include <wlc_keymgmt.h>
#include <wlc_scb.h>
#include <wlc_ampdu.h>
#include <wlc_nar.h>
#include <wlc_amsdu.h>
#include <wlc_sqs.h>
#include <wlc_wlfc.h>
#include <wlc_hw_priv.h>
#if defined(BCMDBG)
#include <wlc_dump.h>
#endif
#ifdef WLTAF
#include <wlc_taf.h>
#endif
#include <wlc_apps.h>

#define SQS_LAZY_FETCH_WATERMARK	128
#define SQS_LAZY_FETCH_DELAYCNT		3

#if defined(BCMDBG)
static int wlc_sqs_dump(void *ctx, struct bcmstrbuf *b);
static int wlc_sqs_dump_clear(void* ctx);
static void __wlc_sqs_scb_dump(wlc_info_t* wlc, struct scb* scb,
	scb_sqs_t *scb_sqs, struct bcmstrbuf *b);
static void wlc_sqs_scb_dump(void *ctx, struct scb *scb, struct bcmstrbuf *b);
#endif

/** File scoped SQS Global object: Fast acecss to SQS module and SQS Cubbies. */
static struct wlc_sqs_global {
	wlc_sqs_info_t  *wlc_sqs;
} wlc_sqs_global = {
	.wlc_sqs = (wlc_sqs_info_t *)NULL,
};

/** File scoped SQS Module Global Pointer. */
#define WLC_SQS_G		((wlc_sqs_info_t*)(wlc_sqs_global.wlc_sqs))

/* SQS scb cubby */
#define SCB_SQS_CUBBY_LOC(sqs, scb) \
	((scb_sqs_t**) SCB_CUBBY((scb), (sqs)->scb_hdl))

#define SCB_SQS(sqs, scb)     (*SCB_SQS_CUBBY_LOC((sqs), (scb)))

static inline uint16 wlc_sqs_pull_packets_cb(uint16 ringid, uint16 request_cnt, bool sqs_force);

static inline bool wlc_sqs_flow_ring_status_cb(uint16 ringid);
#define SQS_FLRING_ACTIVE(scb, prio)	\
	({\
		wlc_sqs_flow_ring_status_cb(SCB_HOST_RINGID((scb), (prio))); \
	})

static int  wlc_sqs_scb_init(void *ctx, struct scb *scb);
static void wlc_sqs_scb_deinit(void *ctx, struct scb *scb);
static uint wlc_sqs_scb_secsz(void *ctx, scb_t *scb);

static uint16 wlc_sqs_pull_packets(wlc_info_t* wlc, scb_sqs_t * scb_sqs, struct scb* scb,
	uint8 tid, uint16 request_cnt, bool sqs_force);
#ifdef WLTAF
static uint16 wlc_sqs_real_pkt_cnt(wlc_info_t* wlc, struct scb* scb, int prio);
void *
wlc_sqs_taf_get_handle(wlc_info_t* wlc)
{
	return &wlc_sqs_global.wlc_sqs;
}

void *
wlc_sqs_taf_get_scb_info(void *sqsh, struct scb* scb)
{
	wlc_sqs_info_t *sqs_info = (wlc_sqs_info_t *)sqsh;

	ASSERT(scb);
	ASSERT(sqs_info);

	return (scb) ? (void*)SCB_SQS(sqs_info, scb) : NULL;
}

void *
wlc_sqs_taf_get_scb_tid_info(void *scb_h, int tid)
{
	return TAF_PARAM(tid);
}

bool
wlc_sqs_taf_release(void* sqsh, void* scbh, void* tidh, bool force,
	taf_scheduler_public_t* taf)
{
	wlc_sqs_info_t *sqs_info = (wlc_sqs_info_t *)sqsh;
	scb_sqs_t * scb_sqs = (scb_sqs_t*)scbh;
	struct scb* scb;
	int prio = (int)tidh;
	wlc_info_t *wlc = WLC_SQS_WLC(sqs_info);
	int32 taf_pkt_time_units;
	int32 taf_pkt_units_to_fill;
	uint32 virtual_release = 0;
	int32 max_virtual_release;
	int32 real_pkts = 0;
	uint16 pktbytes;
	int32 margin;

	TAF_ASSERT(taf->how == TAF_RELEASE_LIKE_IAS);
	TAF_ASSERT(prio >= 0 && prio < NUMPRIO);
	TAF_ASSERT(scb_sqs);

	/* Initialize */
	scb = SCB_SQS_SCB(scb_sqs);

	if (taf->ias.is_ps_mode) {
		/* regardless set emptied flag as the available traffic (ie in PS) is none
		 * so this is effectively empty
		 */
		taf->ias.was_emptied = TRUE;
		taf->complete = TAF_REL_COMPLETE_PS;
		if (SCB_SQS_V_PKTS(scb_sqs, prio)) {
			wlc_apps_pvb_update(wlc, scb);
		}
		/* do not do virtal scheduling in ps state so return */
		return FALSE;
	}

#ifdef HNDPQP
	/* If PSQ is not empty and SCB is PS off,
	 * it means PQP PGI wait for resource to complete the transaction.
	 * Block the release until PQP PGI is finished.
	 */
	if (wlc_apps_psq_len(wlc, scb)) {
		taf->ias.was_emptied = TRUE;
		taf->complete = TAF_REL_COMPLETE_PS;
		return FALSE;
	}
#endif /* HNDPQP */

	taf_pkt_units_to_fill = taf->ias.time_limit_units -
		(taf->ias.actual.released_units + taf->ias.virtual.released_units +
		taf->ias.pending.released_units + taf->ias.total.released_units);

	if ((taf->ias.released_units_limit > 0) &&
		(taf_pkt_units_to_fill > taf->ias.released_units_limit)) {

		taf_pkt_units_to_fill = taf->ias.released_units_limit;
	}

	if (taf_pkt_units_to_fill <= 0) {
		WL_ERROR(("%s: taf_pkt_units_to_fill = %d, actual.released_units = %u, "
			"virtual.released_units = %u, pending.released_units = %u, "
			"total.released_units %u, time_limit_units %u\n",
			__FUNCTION__, taf_pkt_units_to_fill, taf->ias.actual.released_units,
			taf->ias.virtual.released_units, taf->ias.pending.released_units,
			taf->ias.total.released_units, taf->ias.time_limit_units));
		TAF_ASSERT(!(taf_pkt_units_to_fill <= 0));

		taf->complete = TAF_REL_COMPLETE_ERR;

		return FALSE;
	}

	pktbytes = taf->ias.estimated_pkt_size_mean;

	taf_pkt_time_units = TAF_PKTBYTES_TO_UNITS(pktbytes, taf->ias.pkt_rate,
		taf->ias.byte_rate);

	if (taf_pkt_time_units < taf->ias.min_mpdu_dur_units) {
		taf_pkt_time_units = taf->ias.min_mpdu_dur_units;
	}

	if (taf_pkt_time_units == 0) {
		WL_ERROR(("%s: taf_pkt_time_units = %d, pktbytes = %u, pkt_rate = %u, "
			  "byte_rate = %u \n", __FUNCTION__, taf_pkt_time_units, pktbytes,
			  taf->ias.pkt_rate, taf->ias.byte_rate));
		taf_pkt_time_units = 1;
	}

	max_virtual_release = (taf_pkt_units_to_fill / taf_pkt_time_units);

	if (max_virtual_release * taf_pkt_time_units < taf_pkt_units_to_fill) {
		/* round up */
		max_virtual_release++;
	}

	max_virtual_release = MIN(max_virtual_release, 4095);

	margin = 1 + ((max_virtual_release * taf->ias.margin) >> 8);

	real_pkts = wlc_sqs_real_pkt_cnt(wlc, scb, prio);

	if (max_virtual_release + margin - real_pkts > 0) {
		max_virtual_release -= real_pkts;
	} else {
		WL_TAFF(wlc, "enough real packets exist (have %u, require %u, "
			"margin %u)\n", real_pkts, max_virtual_release, margin);

		taf->ias.pending.released_units += taf_pkt_time_units * max_virtual_release;
		taf->ias.pending.release += max_virtual_release;
		max_virtual_release = 0;
		margin = 0;
	}

	if (max_virtual_release + margin > 0) {
		virtual_release = wlc_sqs_pull_packets(wlc, scb_sqs, scb, prio,
			(uint16)(margin + max_virtual_release), FALSE);

	} else {
		/* pull a single packet to keep state machine moving */
		taf->ias.virtual.release +=
			wlc_sqs_pull_packets(wlc, scb_sqs, scb, prio, 1, FALSE);
	}

	if (virtual_release) {
		uint32 vrel_units;
		uint32 rpend_units = taf_pkt_time_units * real_pkts;

		/* account for real packets already available */
		taf->ias.pending.release += real_pkts;
		taf->ias.pending.released_units += rpend_units;

		if (virtual_release > max_virtual_release) {
			/* only account for what we need, not the extra margin */
			vrel_units = taf_pkt_time_units * max_virtual_release;
		} else {
			vrel_units = taf_pkt_time_units * virtual_release;
		}
		taf->ias.virtual.release += virtual_release;
		taf->ias.virtual.released_units += vrel_units;

		if (taf->ias.released_units_limit &&
				((rpend_units + vrel_units) > taf->ias.released_units_limit)) {
			/* report back if over-scheduled, as window may need to be extended */
			taf->ias.extend.units += (rpend_units + vrel_units) -
				taf->ias.released_units_limit;
		}

		if (taf->ias.traffic_count_available < virtual_release) {
			WL_ERROR(("%s: virtual_release %u, traffic_count_available %u\n",
				__FUNCTION__, virtual_release, taf->ias.traffic_count_available));
			TAF_ASSERT(!(taf->ias.traffic_count_available < virtual_release));
		}
		taf->ias.traffic_count_available -= virtual_release;

		if (taf->ias.traffic_count_available == 0) {
			taf->ias.was_emptied = TRUE;

			taf->complete = TAF_REL_COMPLETE_EMPTIED;

		} else if (virtual_release >= max_virtual_release) {
			taf->ias.was_emptied = FALSE;

			taf->complete = (taf_pkt_units_to_fill == taf->ias.released_units_limit) ?
				TAF_REL_COMPLETE_REL_LIMIT : TAF_REL_COMPLETE_TIME_LIMIT;

		} else {
			/* SQS gave less than what we ask; for IAS purposes, this should be
			 * treated as if "emptied" because we can't get more
			 */
			taf->ias.was_emptied = TRUE;
			taf->complete = TAF_REL_COMPLETE_RESTRICTED;
		}
	} else if (max_virtual_release + margin > 0) {

		taf->ias.was_emptied = TRUE;
		taf->complete = TAF_REL_COMPLETE_RESTRICTED;

		return FALSE;
	} else {
		taf->ias.was_emptied = (taf->ias.traffic_count_available == 0);
		taf->complete = (taf_pkt_units_to_fill == taf->ias.released_units_limit) ?
				TAF_REL_COMPLETE_REL_LIMIT : TAF_REL_COMPLETE_TIME_LIMIT;
	}
	return TRUE;
}

uint16
wlc_sqs_taf_get_scb_tid_pkts(void* sqsh, void *scbh, void *tidh, uint32 ts)
{
	scb_sqs_t * scb_sqs = (scb_sqs_t*)scbh;
	struct scb* scb;
	int prio = (int)tidh;
	int tot_pkts = 0;

	ASSERT((prio >= 0) && (prio < NUMPRIO));

	if (scb_sqs) {
		scb = SCB_SQS_SCB(scb_sqs);
		ASSERT(scb);

		/* Return virtual count if flow ring is active */
		if (SQS_FLRING_ACTIVE(scb, prio)) {
			tot_pkts = SCB_SQS_V_PKTS(scb_sqs, prio);
		}
	}
	return tot_pkts;
}
#endif /* WLTAF */

/** SQS module attach handler */
wlc_sqs_info_t *
BCMATTACHFN(wlc_sqs_attach)(wlc_info_t *wlc)
{
	wlc_sqs_info_t *sqs_info = NULL;
	scb_cubby_params_t sqs_scb_cubby_params;

	if ((sqs_info = MALLOCZ(wlc->osh, sizeof(wlc_sqs_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		         wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}

	/* Setup a global pointer to SQS module */
	wlc_sqs_global.wlc_sqs = sqs_info; /* WLC_SQS_G */
	sqs_info->wlc = wlc;

	/* Register SQS module up/down, watchdog, and iovar callbacks */
	//if (wlc_module_register(wlc->pub, sqs_iovars, "sqs", sqs_info,
	//		wlc_sqs_doiovar, wlc_sqs_watchdog, NULL, NULL) != BCME_OK) {
	if (wlc_module_register(wlc->pub, NULL, "sqs", sqs_info,
			NULL, NULL, NULL, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#if defined(BCMDBG)
	/* Register a Dump utility for SQS */
	wlc_dump_add_fns(wlc->pub, "sqs", wlc_sqs_dump, wlc_sqs_dump_clear, (void *)sqs_info);
#endif

	/* Reserve a SQS cubby in the SCB */
	bzero(&sqs_scb_cubby_params, sizeof(sqs_scb_cubby_params));
	sqs_scb_cubby_params.context = sqs_info;
	sqs_scb_cubby_params.fn_init = wlc_sqs_scb_init;
	sqs_scb_cubby_params.fn_deinit = wlc_sqs_scb_deinit;
	sqs_scb_cubby_params.fn_secsz = wlc_sqs_scb_secsz;
#if defined(BCMDBG)
	sqs_scb_cubby_params.fn_dump = wlc_sqs_scb_dump;
#endif
	sqs_info->scb_hdl = wlc_scb_cubby_reserve_ext(wlc, sizeof(scb_sqs_t *),
		&sqs_scb_cubby_params);

	if (sqs_info->scb_hdl < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	return sqs_info;

fail:
	MODULE_DETACH(sqs_info, wlc_sqs_detach);
	return NULL;
}

/** SQS module detach handler */
void
BCMATTACHFN(wlc_sqs_detach)(wlc_sqs_info_t *sqs_info)
{
	wlc_info_t *wlc;

	if (sqs_info == NULL)
		return;

	wlc = sqs_info->wlc;

	/* Unregister the module */
	wlc_module_unregister(wlc->pub, "sqs", sqs_info);

	/* Free up the SQS module memory */
	MFREE(wlc->osh, sqs_info, WLC_SQS_SIZE);

	/* Reset the global SQS module */
	wlc_sqs_global.wlc_sqs = NULL; /* WLC_SQS_G */
}

/* SCB sqs init handler */
static int
wlc_sqs_scb_init(void *ctx, struct scb *scb)
{
	wlc_sqs_info_t *sqs_info;
	wlc_info_t *wlc;
	scb_sqs_t **scb_sqs_ptr;
	scb_sqs_t *scb_sqs;

	/* Initialize */
	sqs_info = (wlc_sqs_info_t*)ctx;
	wlc = WLC_SQS_WLC(sqs_info);

	/* Allocate the SCB cubby */
	scb_sqs_ptr = SCB_SQS_CUBBY_LOC(sqs_info, scb);
	scb_sqs = wlc_scb_sec_cubby_alloc(wlc, scb,
		wlc_sqs_scb_secsz(ctx, scb));

	/* Alloc the SCB CFP cubby */
	if (scb_sqs == NULL) {
		goto done;
	}

	 /* Store SCB CFP cubby pointer */
	*scb_sqs_ptr = scb_sqs;

	SCB_SQS_SCB(scb_sqs) = scb;
	SCB_SQS_INFO(scb_sqs) = sqs_info;

	WLC_SQS_DEBUG(("%s  : SQS cubby %p size %d scb %p "
		"EA "MACF" \n", __FUNCTION__, scb_sqs, (int) SCB_SQS_SIZE, scb,
		 ETHER_TO_MACF(scb->ea)));
done:
	return BCME_OK;
}

/* SCB sqs deinit handler */
static void
wlc_sqs_scb_deinit(void *ctx, struct scb *scb)
{
	wlc_sqs_info_t *sqs_info = (wlc_sqs_info_t*)ctx;
	scb_sqs_t** scb_sqs_ptr = SCB_SQS_CUBBY_LOC(sqs_info, scb);
	scb_sqs_t *scb_sqs = *scb_sqs_ptr;
	wlc_info_t * wlc;
	uint8 prio;
	uint16 v2r_pkts;

	/* Initialize */
	wlc = WLC_SQS_WLC(sqs_info);

	if (scb_sqs == NULL)
		return;

	/* Decrement the global V2R count if SCB is getting deleted */
	for (prio = 0; prio < NUMPRIO; prio++) {
		v2r_pkts = SCB_SQS_V2R_PKTS(scb_sqs, prio);

		if (v2r_pkts) {
			ASSERT(WLC_SQS_V2R_INTRANSIT(sqs_info) >= v2r_pkts);
			WLC_SQS_V2R_INTRANSIT(sqs_info) -= v2r_pkts;
		}
	}

	/* Zero out the SCB SQS cubby contents */
	memset(scb_sqs, 0, sizeof(scb_sqs_t));

	/* Release the SCB CFP cubby memory */
	wlc_scb_sec_cubby_free(wlc, scb, scb_sqs);

	/* Set the SCB CFP cubby pointer to NULL */
	*scb_sqs_ptr = NULL;
}

static uint
wlc_sqs_scb_secsz(void *ctx, scb_t *scb)
{
	if (scb && !SCB_HWRS(scb)) {
		return SCB_SQS_SIZE;
	} else {
		return 0;
	}
}

#if defined(BCMDBG)

/** SCB CFP cubby dump utility */
static void
wlc_sqs_scb_dump(void *ctx, struct scb *scb, struct bcmstrbuf *b)
{
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;
	wlc_info_t* wlc;

	ASSERT(scb);

	 /* Initialization */
	sqs_info = (wlc_sqs_info_t*)ctx;
	scb_sqs  = SCB_SQS(sqs_info, scb);
	wlc = WLC_SQS_WLC(WLC_SQS_G);

	if (scb_sqs == NULL) {
		return;
	}

	/* Dump per client info */
	__wlc_sqs_scb_dump(wlc, scb, scb_sqs, b);

	return;

}
/* SQS dump clear utility */
static int
wlc_sqs_dump_clear(void* ctx)
{
	wlc_sqs_info_t *sqs_info = (wlc_sqs_info_t*)ctx;
	struct scb_iter scbiter;
	struct scb *scb;
	scb_sqs_t * scb_sqs;
	wlc_info_t* wlc;
	uint8 prio;

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);

	/* Iterate the SCBs */
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		scb_sqs = SCB_SQS(sqs_info, scb);
		ASSERT(scb_sqs);

		/* Max V packets */
		for (prio = 0; prio < NUMPRIO; prio++) {
			SCB_SQS_V_PKTS_MAX(scb_sqs, prio) = SCB_SQS_V_PKTS(scb_sqs, prio);
		}

		/* Cumulative counter */
		for (prio = 0; prio < NUMPRIO; prio++) {
			SCB_SQS_CUM_V_PKTS(scb_sqs, prio) = SCB_SQS_V_PKTS(scb_sqs, prio);
		}
	}
	return 0;
}

/* SQS dump utility */
static int
wlc_sqs_dump(void *ctx, struct bcmstrbuf *b)
{
	uint16 flowid_i;
	wlc_sqs_info_t *sqs_info = (wlc_sqs_info_t*)ctx;
	struct scb *scb;
	scb_sqs_t * scb_sqs;
	wlc_info_t* wlc;

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);

	for (flowid_i = 0; flowid_i < SCB_FLOWID_INVALID; flowid_i++) {
		scb = wlc_scb_flowid_lookup(wlc, flowid_i);

		 if (scb == NULL)
			continue;

		scb_sqs  = SCB_SQS(sqs_info, scb);

		if (scb_sqs == NULL)
			continue;

		/* Dump per client info */
		__wlc_sqs_scb_dump(wlc, scb, scb_sqs, b);

	}
	return 0;
}

static void
__wlc_sqs_scb_dump(wlc_info_t* wlc, struct scb* scb,
	scb_sqs_t *scb_sqs, struct bcmstrbuf *b)
{
	uint8 prio;

	ASSERT(scb_sqs);

	bcm_bprintf(b, "Link "MACF" [flowid: %d]:\n", ETHERP_TO_MACF(&scb->ea),
		SCB_FLOWID(scb));
	bcm_bprintf(b, "\tTotal \t\t\t ::V pkts:%u  V2R pkts:%u\n",
		SCB_SQS_V_PKTS_TOT(scb_sqs), SCB_SQS_V2R_PKTS_TOT(scb_sqs));
	bcm_bprintf(b, "\tV Pkts \t\t\t ::");
	for (prio = 0; prio < NUMPRIO; prio++) {
		bcm_bprintf(b, "%u ", SCB_SQS_V_PKTS(scb_sqs, prio));
	}
	bcm_bprintf(b, "\n");
	bcm_bprintf(b, "\tActive \t\t\t ::");
	for (prio = 0; prio < NUMPRIO; prio++) {
		bcm_bprintf(b, "%u ", SQS_FLRING_ACTIVE(scb, prio));
	}
	bcm_bprintf(b, "\n");

	bcm_bprintf(b, "\tMax V Pkts \t\t ::");
	for (prio = 0; prio < NUMPRIO; prio++) {
		bcm_bprintf(b, "%u ", SCB_SQS_V_PKTS_MAX(scb_sqs, prio));
	}

	bcm_bprintf(b, "\n");
	bcm_bprintf(b, "\tCumulative  V Pkts \t ::");
	for (prio = 0; prio < NUMPRIO; prio++) {
		bcm_bprintf(b, "%u ", SCB_SQS_CUM_V_PKTS(scb_sqs, prio));
	}

	bcm_bprintf(b, "\n");
	bcm_bprintf(b, "\tV2R Pkts \t\t ::");
	for (prio = 0; prio < NUMPRIO; prio++) {
		bcm_bprintf(b, "%u ", SCB_SQS_V2R_PKTS(scb_sqs, prio));
	}
	bcm_bprintf(b, "\n");
}
#endif

/* Register a callback function to pull packets from BUS */
void
wlc_sqs_pull_packets_register(pkt_pull_cb_t cb, void* arg)
{
	wlc_sqs_info_t *sqs_info = WLC_SQS_G;

	ASSERT(sqs_info);

	/* Save the fn pointer and arg */
	WLC_SQS_PKTPULL_CB_FN(sqs_info) = cb;
	WLC_SQS_PKTPULL_CB_ARG(sqs_info) = arg;

}

/* Callback function to get Flow ring status from BUS layer */
void
wlc_sqs_flowring_status_register(flow_ring_status_cb_t cb, void* arg)
{
	wlc_sqs_info_t *sqs_info = WLC_SQS_G;

	ASSERT(sqs_info);

	/* Save the fn pointer and arg */
	WLC_SQS_FLRING_STS_CB_FN(sqs_info) = cb;
	WLC_SQS_FLRING_STS_CB_ARG(sqs_info) = arg;

}

/* Register a callback function register a End of pull set from TAF */
void
wlc_sqs_eops_rqst_register(eops_rqst_cb_t cb, void* arg)
{
	wlc_sqs_info_t *sqs_info = WLC_SQS_G;

	ASSERT(sqs_info);

	/* Save the fn pointer and arg */
	WLC_EOPS_RQST_CB_FN(sqs_info) = cb;
	WLC_EOPS_RQST_CB_ARG(sqs_info) = arg;
}

/* Invoke the callback function registered by BUS layer */
static inline uint16
wlc_sqs_pull_packets_cb(uint16 ringid, uint16 request_cnt, bool sqs_force)
{
	wlc_sqs_info_t *sqs_info = WLC_SQS_G;

	ASSERT(sqs_info);

	/* Make sure calbacks are registered */
	ASSERT(WLC_SQS_PKTPULL_CB_FN(sqs_info));
	ASSERT(WLC_SQS_PKTPULL_CB_ARG(sqs_info));

	/* Call back */
	return ((WLC_SQS_PKTPULL_CB_FN(sqs_info))((WLC_SQS_PKTPULL_CB_ARG(sqs_info)),
		ringid, request_cnt, sqs_force));
}

/* Invoke the callback function registered by BUS layer */
static inline int
wlc_sqs_eops_rqst_cb(void)
{
	wlc_sqs_info_t *sqs_info = WLC_SQS_G;

	ASSERT(sqs_info);

	/* Make sure calbacks are registered */
	ASSERT(WLC_EOPS_RQST_CB_FN(sqs_info));
	ASSERT(WLC_EOPS_RQST_CB_ARG(sqs_info));

	/* Call back */
	return ((WLC_EOPS_RQST_CB_FN(sqs_info))(WLC_EOPS_RQST_CB_ARG(sqs_info)));
}

/* Invoke the callback function registered by BUS layer */
static inline bool
wlc_sqs_flow_ring_status_cb(uint16 ringid)
{
	wlc_sqs_info_t *sqs_info = WLC_SQS_G;

	ASSERT(sqs_info);

	/* Make sure calbacks are registered */
	ASSERT(WLC_SQS_FLRING_STS_CB_FN(sqs_info));
	ASSERT(WLC_SQS_FLRING_STS_CB_ARG(sqs_info));

	if (ringid == SCB_HOST_RINGID_INVALID)
		return FALSE;

	/* Call back */
	return ((WLC_SQS_FLRING_STS_CB_FN(sqs_info))((WLC_SQS_FLRING_STS_CB_ARG(sqs_info)),
		ringid));
}

/* Virtual packets for multiple ACs of a given flow */
uint16
wlc_sqs_vpkts_multi_ac(struct scb* scb, uint8 tid_bmp)
{
	wlc_info_t *wlc;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;
	uint8  prio;
	uint16 tot_pkts = 0;

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb_sqs = SCB_SQS(sqs_info, scb);

	if (!scb_sqs)
		return 0;

	for (prio = 0; prio < NUMPRIO; prio++) {
		if ((tid_bmp & (0x1 << prio)) == 0)
			continue;
		tot_pkts += SCB_SQS_V_PKTS(scb_sqs, prio);
	}
	return tot_pkts;
}
/* Total virtual packets for a flow across all priorities */
uint16
wlc_sqs_vpkts_tot(struct scb* scb)
{
	wlc_info_t *wlc;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb_sqs = SCB_SQS(sqs_info, scb);

	if (!scb_sqs)
		return 0;

	return SCB_SQS_V_PKTS_TOT(scb_sqs);
}

/* Virtual packets for a given flow & priority */
uint16
wlc_sqs_vpkts(uint16 flowid, uint8 prio)
{
	wlc_info_t *wlc;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;
	struct scb* scb;

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb = wlc_scb_flowid_lookup(wlc, flowid);

	ASSERT(scb);

	scb_sqs = SCB_SQS(sqs_info, scb);

	if (!scb_sqs)
		return 0;
	ASSERT(prio < NUMPRIO);

	return SCB_SQS_V_PKTS(scb_sqs, prio);
}

/* V2R packets for multiple ACs of a given flow */
uint16
wlc_sqs_v2r_pkts_multi_ac(struct scb* scb, uint8 tid_bmp)
{
	wlc_info_t *wlc;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;
	uint8  prio;
	uint16 tot_pkts = 0;

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb_sqs = SCB_SQS(sqs_info, scb);

	if (!scb_sqs)
		return 0;

	for (prio = 0; prio < NUMPRIO; prio++) {
		if ((tid_bmp & (0x1 << prio)) == 0)
			continue;
		tot_pkts += SCB_SQS_V2R_PKTS(scb_sqs, prio);
	}
	return tot_pkts;
}
/* Total v2r packets for a flow across all priorities */
uint16
wlc_sqs_v2r_pkts_tot(struct scb* scb)
{
	wlc_info_t *wlc;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb_sqs = SCB_SQS(sqs_info, scb);

	if (!scb_sqs)
		return 0;

	return SCB_SQS_V2R_PKTS_TOT(scb_sqs);
}

/* v2r packets for a given scb and priority */
uint16
wlc_sqs_v2r_pkts(uint16 flowid, uint8 prio)
{
	wlc_info_t *wlc;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;
	struct scb* scb;

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb = wlc_scb_flowid_lookup(wlc, flowid);
	ASSERT(scb);

	scb_sqs = SCB_SQS(sqs_info, scb);

	ASSERT(scb_sqs);
	ASSERT(prio < NUMPRIO);

	return SCB_SQS_V2R_PKTS(scb_sqs, prio);
}

void
wlc_sqs_v2r_enqueue(uint16 flowid, uint8 prio, uint16 v2r_count)
{
	wlc_info_t *wlc;
	struct scb *scb;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;

	ASSERT_SCB_FLOWID(flowid);
	ASSERT(WLC_SQS_G);

	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb = wlc_scb_flowid_lookup(wlc, flowid);

	ASSERT(scb);
	ASSERT(prio < NUMPRIO);

	scb_sqs = SCB_SQS(sqs_info, scb);

	/* Sanity check for the virtual packets */
	ASSERT(SCB_SQS_V_PKTS(scb_sqs, prio) >= v2r_count);
	SCB_SQS_V_PKTS(scb_sqs, prio) -= v2r_count;
	SCB_SQS_V_PKTS_TOT(scb_sqs) -= v2r_count;

	/* V2R per scb-tid */
	SCB_SQS_V2R_PKTS(scb_sqs, prio) += v2r_count;	/* Per prio */
	SCB_SQS_V2R_PKTS_TOT(scb_sqs)	+= v2r_count;	/* Per SCB */
	WLC_SQS_V2R_INTRANSIT(sqs_info) += v2r_count;	/* Per Radio */

	/* Catch any overflow errors */
	ASSERT(WLC_SQS_V2R_INTRANSIT(sqs_info) >=
		SCB_SQS_V2R_PKTS(scb_sqs, prio));

	WLC_SQS_DEBUG(("%s : Cur flowid %d v2r %d  V pkts %d \n",
		__FUNCTION__, flowid, SCB_SQS_V2R_PKTS(scb_sqs, prio),
		SCB_SQS_V_PKTS(scb_sqs, prio)));
}

void
wlc_sqs_v2r_dequeue(uint16 flowid, uint8 prio, uint16 pkt_count, bool sqs_force)
{

	wlc_info_t *wlc;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;
	struct scb* scb;

	ASSERT_SCB_FLOWID(flowid);
	ASSERT(WLC_SQS_G);
	ASSERT(prio < NUMPRIO);

	/* Initialize */
	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb = wlc_scb_flowid_lookup(wlc, flowid);
	ASSERT(scb);

	scb_sqs = SCB_SQS(sqs_info, scb);

	/* Decrement total outstanding V2R request in system */
	ASSERT(WLC_SQS_V2R_INTRANSIT(sqs_info) >= pkt_count);
	ASSERT(SCB_SQS_V2R_PKTS(scb_sqs, prio) >= pkt_count);
	ASSERT(SCB_SQS_V2R_PKTS_TOT(scb_sqs) >= pkt_count);

	SCB_SQS_V2R_PKTS(scb_sqs, prio) -= pkt_count;	/* Per Prio */
	SCB_SQS_V2R_PKTS_TOT(scb_sqs)	-= pkt_count;	/* Per SCB */
	WLC_SQS_V2R_INTRANSIT(sqs_info) -= pkt_count;	/* Per Radio */

#ifdef WLTAF
	/* check if TAF is enabled and not in bypass state */
	if ((!SCB_INTERNAL(scb)) && wlc_taf_in_use(wlc->taf_handle) && !sqs_force) {
		wlc_taf_pkts_dequeue(wlc->taf_handle, scb,
			prio, pkt_count);
	}
#endif /* WLTAF */

	WLC_SQS_DEBUG(("%s : cur flowid %d v2r %d \n",
		__FUNCTION__, flowid, SCB_SQS_V2R_PKTS(scb_sqs, prio)));
}

/**
 * wlc_sqs_vpkts_rewind() - Rewind the fetch_ptr and revert/increase the vpkts.
 */
void
wlc_sqs_vpkts_rewind(uint16 flowid, uint8 prio, uint16 count)
{
	wlc_info_t *wlc;
	struct scb *scb;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;

	ASSERT_SCB_FLOWID(flowid);
	ASSERT(WLC_SQS_G);
	ASSERT(prio < NUMPRIO);

	/* Initialilze */
	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb = wlc_scb_flowid_lookup(wlc, flowid);

	ASSERT(scb);

	scb_sqs = SCB_SQS(sqs_info, scb);

	if (!scb_sqs)
		return;

	/* Increment virtual counters */
	SCB_SQS_V_PKTS(scb_sqs, prio) += count;
	SCB_SQS_V_PKTS_TOT(scb_sqs) += count;

	/* Store the max stats */
	SCB_SQS_V_PKTS_MAX(scb_sqs, prio) = MAX(SCB_SQS_V_PKTS_MAX(scb_sqs, prio),
		SCB_SQS_V_PKTS(scb_sqs, prio));

#ifdef WLTAF
	if ((!SCB_INTERNAL(scb)) && wlc_taf_in_use(wlc->taf_handle)) {
		wlc_taf_sched_state(wlc->taf_handle, scb, prio, count,
			TAF_SQSHOST, TAF_SCHED_STATE_REWIND);
	}
#endif

#ifdef PKTQ_LOG
	wlc_ampdu_sqs_pktq_log(wlc, scb, prio, TRUE,
		SCB_SQS_V_PKTS(scb_sqs, prio), count);
#endif /* PKTQ_LOG */

}

/**
 * wlc_sqs_vpkts_forward() - Forward the fetch_ptr and decrease the vpkts.
 */
void
wlc_sqs_vpkts_forward(uint16 flowid, uint8 prio, uint16 count)
{
	wlc_info_t *wlc;
	struct scb *scb;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;

	ASSERT_SCB_FLOWID(flowid);
	ASSERT(WLC_SQS_G);
	ASSERT(prio < NUMPRIO);

	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb = wlc_scb_flowid_lookup(wlc, flowid);

	ASSERT(scb);

	scb_sqs = SCB_SQS(sqs_info, scb);
	ASSERT(SCB_SQS_V_PKTS(scb_sqs, prio) >= count);

	SCB_SQS_V_PKTS(scb_sqs, prio) -= count;
	SCB_SQS_V_PKTS_TOT(scb_sqs) -= count;
}

/**
 * wlc_sqs_vpkts_enqueue () - admit virtual packets into a precedence Queue.
 * There is no drop threshold on a precedence queue. Host performs push into
 * a flowring and is responsible for all host side traffic management.
 *
 * @param cfp_flowid	CFP flow ID
 * @param prio		Packet priority
 * @paarm v_pkts	Virtual packets added in current iteration
 */
void
wlc_sqs_vpkts_enqueue(uint16 flowid, uint8 prio, uint16 v_pkts)
{
	wlc_info_t *wlc;
	struct scb *scb;
	wlc_sqs_info_t *sqs_info;
	scb_sqs_t * scb_sqs;

	ASSERT_SCB_FLOWID(flowid);
	ASSERT(WLC_SQS_G);

	wlc = WLC_SQS_WLC(WLC_SQS_G);
	sqs_info = wlc->sqs;
	scb = wlc_scb_flowid_lookup(wlc, flowid);

	ASSERT(scb);
	ASSERT(prio < NUMPRIO);

	scb_sqs = SCB_SQS(sqs_info, scb);

	ASSERT(scb_sqs);

	SCB_SQS_V_PKTS(scb_sqs, prio) += v_pkts;
	SCB_SQS_V_PKTS_TOT(scb_sqs) += v_pkts;
	SCB_SQS_CUM_V_PKTS(scb_sqs, prio) += v_pkts;

	/* Store the max stats */
	SCB_SQS_V_PKTS_MAX(scb_sqs, prio) = MAX(SCB_SQS_V_PKTS_MAX(scb_sqs, prio),
		SCB_SQS_V_PKTS(scb_sqs, prio));

	WLC_SQS_DEBUG(("%s : cur flowid %d V %d \n",
		__FUNCTION__, flowid, SCB_SQS_V_PKTS(scb_sqs, prio)));
#ifdef WLTAF
	/* check if TAF is enabled and not in bypass state */
	if ((!SCB_INTERNAL(scb)) && wlc_taf_in_use(wlc->taf_handle)) {
		wlc_taf_pkts_enqueue(wlc->taf_handle, scb,
			prio, TAF_SQSHOST, v_pkts);
	}
#endif

#ifdef PKTQ_LOG
	wlc_ampdu_sqs_pktq_log(wlc, scb, prio, FALSE,
		SCB_SQS_V_PKTS(scb_sqs, prio), v_pkts);
#endif /* PKTQ_LOG */

}

/* Fetch packets for PS mode stations outside of TAF scheduler */
uint16
wlc_sqs_psmode_pull_packets(wlc_info_t* wlc, struct scb* scb,
	uint8 tid_bmp, uint16 request_cnt)
{
	uint8 prio;
	scb_sqs_t * scb_sqs;
	uint16 ret = 0;

	ASSERT(scb);

	scb_sqs = SCB_SQS(wlc->sqs, scb);

	if (!scb_sqs)
		return 0;

	for (prio = 0; prio < NUMPRIO; prio++) {
		if ((tid_bmp & (0x1 << prio)) == 0)
			continue;
		ret += wlc_sqs_pull_packets(wlc, scb_sqs, scb, prio, request_cnt, TRUE);
	}

	return ret;
}

/* Submit request to convert virtual to real packets */
static uint16
wlc_sqs_pull_packets(wlc_info_t* wlc, scb_sqs_t * scb_sqs,  struct scb* scb,
	uint8 tid, uint16 request_cnt, bool sqs_force)
{
	uint16 ringid;
	int v2r_request = request_cnt;
	uint16 ret = 0;

	ASSERT(scb);

	ASSERT(scb_sqs);
	ASSERT(!SCB_INTERNAL(scb));

	v2r_request = MIN(SCB_SQS_V_PKTS(scb_sqs, tid), v2r_request);

	if (v2r_request == 0)
		return ret;

	ASSERT(v2r_request >= 0);

	/* Get Host flowring id */
	ringid = SCB_HOST_RINGID(scb, tid);

	ASSERT_SCB_HOST_RINGID(ringid);

	WLC_SQS_DEBUG(("%s : SQS V2R submit : flowid %d cnt %d \n",
		__FUNCTION__, SCB_FLOWID(scb),
		v2r_request));

	/* V2R request from BUS Layer [sqs_v2r_request]  */
	ret = wlc_sqs_pull_packets_cb(ringid, v2r_request, sqs_force);

	/* Return successfully submitted v2r count */
	return ret;
}
#ifdef WLTAF
/** TAF evaluation entry point. */
void
wlc_sqs_taf_txeval_trigger(void)
{
	WLC_SQS_DEBUG(("%s : Kick the TAF scheduler \n", __FUNCTION__));

#ifdef WLTAF
	/* check if TAF is enabled and not in bypass state */
	if (wlc_taf_in_use(WLC_SQS_WLC(WLC_SQS_G)->taf_handle)) {
		wlc_taf_schedule(WLC_SQS_WLC(WLC_SQS_G)->taf_handle, TAF_SQS_TRIGGER_TID, NULL,
			FALSE);
	}
#endif
}

/* TAF End of pull set request
 * Triggered by TAF packet evaluation routine at the end of a schedule cycle.
 *
 * 1. If there are pending V2R requests, submit a request to BUS layer
 * 	to mark a PENDING_RESPONSE signal.
 * 2. If EoPS request raised multiple times without a new V2R submission,
 * 	EoPS request is dropped.
 * 3. If no V2R, send EoPS response immediately
 */
void
wlc_sqs_eops_rqst(void)
{
	int ret = -1;
	wlc_sqs_info_t *sqs_info = WLC_SQS_G;
	ASSERT(sqs_info);

	WLC_SQS_DEBUG(("%s : Trigger EoPS request \n", __FUNCTION__));

	if (WLC_SQS_V2R_INTRANSIT(sqs_info)) {
		/* Submit a end of pull pull set request to BUS layer */
		ret = wlc_sqs_eops_rqst_cb();

		if (ret == BCME_OK) {
			/* Outstanding EoPS request count */
			WLC_SQS_EOPS_INTRANSIT(sqs_info)++;
		}
	} else {
		/* Outstanding EoPS request count */
		WLC_SQS_EOPS_INTRANSIT(sqs_info)++;

		/* No outstanding V2r requests. Send the EOPS response */
		wlc_sqs_eops_response();
	}
}

/* TAF End of pull set request */
void
wlc_sqs_eops_response(void)
{
	wlc_sqs_info_t *sqs_info = WLC_SQS_G;

	ASSERT(sqs_info);

	WLC_SQS_DEBUG(("%s : EoPS response \n", __FUNCTION__));

	/* Outstanding EoPS request count */
	ASSERT(WLC_SQS_EOPS_INTRANSIT(sqs_info));

	WLC_SQS_EOPS_INTRANSIT(sqs_info)--;

#ifdef WLTAF
	/* check if TAF is enabled and not in bypass state */
	if (wlc_taf_in_use(WLC_SQS_WLC(sqs_info)->taf_handle)) {
		/* TAF EOPS response callback */
		wlc_taf_v2r_complete(WLC_SQS_WLC(sqs_info)->taf_handle);
	}
#endif /* WLTAF */
}

/* Return the real packets enqueued for a given SCB in AMPDU & NAR Qs. */
static
uint16 wlc_sqs_real_pkt_cnt(wlc_info_t* wlc, struct scb* scb, int prio)
{
	uint16 real_pkts = 0;

	ASSERT(prio < NUMPRIO);

	real_pkts += wlc_scb_ampdu_n_pkts(wlc, scb, prio);
	real_pkts += wlc_scb_nar_n_pkts(wlc->nar_handle, scb, prio);

	return real_pkts;
}
#endif /* WLTAF */
