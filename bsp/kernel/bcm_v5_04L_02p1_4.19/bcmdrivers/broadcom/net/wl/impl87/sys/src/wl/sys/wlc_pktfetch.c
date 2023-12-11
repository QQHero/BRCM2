/*
 * Routines related to the uses of Pktfetch in WLC
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
 * $Id: wlc_pktfetch.c 804620 2021-11-04 18:56:37Z $
 */

/**
 * There are use cases which require the complete packet to be consumed in-dongle, and thus
 * neccessitates a mechanism (PktFetch) that can DMA down data blocks from the Host memory to dongle
 * memory, and thus recreate the full ethernet packet. Example use cases include:
 *     EAPOL frames
 *     SW TKIP
 *     WAPI
 *
 * This is a generic module which takes a Host address, block size and a pre-specified local buffer
 * (lbuf) and guarantees that the Host block will be fetched into device memory.
 */

#include <wlc_cfg.h>
#include <wlc_types.h>
#include <bcmwpa.h>
#include <hnd_cplt.h>
#include <phy_utils_api.h>
#include <wlc.h>
#include <wlc_pub.h>
#include <wlc_pktfetch.h>
#include <wlc_scb.h>
#include <wlc_ampdu_rx.h>
#include <wlc_keymgmt.h>
#include <wlc_bsscfg.h>
#include <d11_cfg.h>
#include <wlc_bmac.h>
#if defined(STS_XFER_PHYRXS)
#include <wlc_sts_xfer.h>
#endif /* STS_XFER_PHYRXS */
#include <rte_fetch.h>
#if defined(BCMPCIEDEV)
#include <wlc_tx.h>
#include <wlc_wnm.h>
#endif /* BCMPCIEDEV */

#if defined(BCMSPLITRX)
#include <wlc_rx.h>

#if !(defined(BCMHWA) && defined(HWA_RXDATA_BUILD)) || !defined(BME_PKTFETCH)
static bool
wlc_pktfetch_checkpkt(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct wlc_frminfo *f, struct dot11_llc_snap_header *lsh, uint body_len)
{
	bool retval;
	uint16 ether_type = ntoh16(lsh->type);

	retval = LLC_SNAP_HEADER_CHECK(lsh) &&
		(EAPOL_PKTFETCH_REQUIRED(ether_type) ||
#ifdef WLTDLS
			WLTDLS_PKTFETCH_REQUIRED(wlc, ether_type) ||
#endif
#ifdef WLNDOE
			NDOE_PKTFETCH_REQUIRED(wlc, lsh, f->pbody, body_len) ||
#endif
#ifdef BCMWAPI_WAI
			WAPI_PKTFETCH_REQUIRED(ether_type) ||
#endif
#ifdef WOWLPF
			WOWLPF_ACTIVE(wlc->pub) ||
#endif
#ifdef WL_TBOW
			WLTBOW_PKTFETCH_REQUIRED(wlc, bsscfg, ether_type) ||
#endif
#ifdef ICMP
			ICMP_PKTFETCH_REQUIRED(wlc, ether_type) ||
#endif
			FALSE);

	return retval;
}
#endif /* !(defined(BCMHWA) && defined(HWA_RXDATA_BUILD)) || !defined(BME_PKTFETCH) */

#if defined(BCMHWA) && defined(HWA_RXDATA_BUILD)
bool
wlc_pktfetch_required(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct wlc_frminfo *f,	wlc_key_info_t *key_info, bool skip_iv)
{
	/* All remaining checks are valid only if its a RX frag with host content */
	if (PKTFRAGUSEDLEN(wlc->osh, f->p) == 0)
		return FALSE;

	return hwa_rxdata_fhr_is_pktfetch(ltoh16(D11RXHDR_GE129_ACCESS_VAL(f->rxh, filtermap16)));
}

#else /* ! (BCMHWA && HWA_RXDATA_BUILD) */

static bool
wlc_pktfetch_required_mode4(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, uint16 *ether_type_offset,
		struct wlc_frminfo *f)
{
	uint16 ether_type = ntoh16(*ether_type_offset);

	BCM_REFERENCE(f);

	if (EAPOL_PKTFETCH_REQUIRED(ether_type) ||
#ifdef WLTDLS
			WLTDLS_PKTFETCH_REQUIRED(wlc, ether_type) ||
#endif
#ifdef WLNDOE
			(NDOE_ENAB(wlc->pub) && (ether_type == ETHER_TYPE_IPV6) &&
			NDOE_PKTFETCH_REQUIRED_MODE4(wlc,
			((uint8 *)ether_type_offset + ETHER_TYPE_LEN),
			f->pbody, f->body_len)) ||
#endif
#ifdef BCMWAPI_WAI
			WAPI_PKTFETCH_REQUIRED(ether_type) ||
#endif
#ifdef WOWLPF
			WOWLPF_ACTIVE(wlc->pub) ||
#endif
#ifdef WL_TBOW
			WLTBOW_PKTFETCH_REQUIRED(wlc, bsscfg, ether_type) ||
#endif
#ifdef ICMP
			ICMP_PKTFETCH_REQUIRED(wlc, ether_type) ||
#endif
			FALSE) {
				return TRUE;
			}
	return FALSE;
}

bool
wlc_pktfetch_required(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
	struct wlc_frminfo *f,	wlc_key_info_t *key_info, bool skip_iv)
{
	struct dot11_llc_snap_header *lsh;
	uint16* ether_type = NULL;
	uchar* f_body = f->pbody;

	/* All remaining checks are valid only if its a RX frag with host content */
	if (PKTFRAGUSEDLEN(wlc->osh, f->p) == 0)
		return FALSE;

	/* In  MODE-4  need to check for  HdrType of conversion( Type1/Type2) to fetch correct
	 *    EtherType
	 */
	if (SPLITRX_DYN_MODE4(wlc->osh, f->p)) {
		bool conv_type;

		if (f->isamsdu) {
			f_body -= HDRCONV_PAD;
		}

		conv_type = RXHDR_GET_CONV_TYPE(&f->wrxh->rxhdr, wlc);
		if (conv_type) {
			ether_type = (unsigned short*)(f_body + ETHER_TYPE_2_OFFSET);
		} else {
			ether_type = (unsigned short*)(f_body + ETHER_TYPE_1_OFFSET);
		}
		return wlc_pktfetch_required_mode4(wlc, bsscfg, ether_type, f);
	}

	/* If IV needs to be accounted for, move past IV len */
	lsh = (struct dot11_llc_snap_header *)(f_body + (skip_iv ? key_info->iv_len : 0));

	/* For AMSDU packets packet starts after subframe header */
	if (f->isamsdu)
		lsh = (struct dot11_llc_snap_header*)((char *)lsh + ETHER_HDR_LEN);

	return (wlc_pktfetch_checkpkt(wlc, bsscfg, f, lsh, f->body_len));
}

#endif /* ! (BCMHWA && HWA_RXDATA_BUILD) */
#endif /* BCMSPLITRX */

#if defined(BCMPCIEDEV)
/**
 * Per packet key check for SW TKIP MIC requirement, code largely borrowed from wlc_sendpkt.
 * BCMPCIEDEV specific.
 */
static bool
wlc_sw_tkip_mic_enab(wlc_info_t *wlc, wlc_if_t *wlcif, wlc_bsscfg_t *bsscfg, struct lbuf *lb)
{
	struct scb *scb = NULL;
	struct ether_header *eh;
	struct ether_addr *dst;
#ifdef WDS
	struct ether_addr *wds = NULL;
#endif
	wlc_key_info_t key_info;
	wlc_key_t *key = NULL;
	enum wlc_bandunit bandunit;
	bool tkip_enab = FALSE;

	/* WLDPT, WLTDLS, IAPP cases currently not handled */

	/* Get dest. */
	eh = (struct ether_header*) PKTDATA(wlc->osh, lb);

#ifdef WDS
	if (wlcif && wlcif->type == WLC_IFTYPE_WDS) {
		scb = wlcif->u.scb;
		wds = &scb->ea;
	}

	if (wds)
		dst = wds;
	else
#endif /* WDS */
	if (BSSCFG_AP(bsscfg)) {
#ifdef WLWNM_AP
		/* Do the WNM processing */
		if (WLWNM_ENAB(wlc->pub) && wlc_wnm_dms_amsdu_on(wlc, bsscfg) &&
		    WLPKTTAGSCBGET(lb) != NULL) {
			dst = &(WLPKTTAGSCBGET(lb)->ea);
		} else
#endif /* WLWNM_AP */
		dst = (struct ether_addr*)eh->ether_dhost;
	} else {
		dst = bsscfg->BSS ? &bsscfg->BSSID : (struct ether_addr*)eh->ether_dhost;
	}

	/* Get key */
	bandunit = CHSPEC_BANDUNIT(bsscfg->current_bss->chanspec);

	/* Class 3 (BSS) frame */
	if (TRUE &&
#ifdef WDS
		!wds &&
#endif
		bsscfg->BSS && !ETHER_ISMULTI(dst)) {
		scb = wlc_scbfindband(wlc, bsscfg, dst, bandunit);
	}
	/* Class 1 (IBSS/DPT) or 4 (WDS) frame */
	else {
		if (!ETHER_ISMULTI(dst))
			scb = wlc_scblookupband(wlc, bsscfg, dst, bandunit);
	}

	key = wlc_keymgmt_get_tx_key(wlc->keymgmt, scb, bsscfg, &key_info);

	if (scb && (key_info.algo == CRYPTO_ALGO_OFF)) {
		WL_INFORM(("wl%d: %s: key_info algo is off, use bss tx key instead\n",
			WLCWLUNIT(wlc), __FUNCTION__));
		key = wlc_keymgmt_get_bss_tx_key(wlc->keymgmt, bsscfg, FALSE, &key_info);
	}

	if (key == NULL)
		WL_ERROR(("wl%d: %s: key is NULL!\n", WLCWLUNIT(wlc), __FUNCTION__));

	/* If security algo is TKIP and MIC key is in HW, or MFP */
	if (((key_info.algo == CRYPTO_ALGO_TKIP) && (ETHER_ISMULTI(dst) ||
		!(WLC_KEY_MIC_IN_HW(&key_info)) ||
		(scb && wlc_is_packet_fragmented(wlc, scb, (void *)lb)))) ||
		WLC_KEY_SW_ONLY(&key_info)) {
		tkip_enab = TRUE;
	}
	return tkip_enab;
}

bool
wlc_tx_pktfetch_required(wlc_info_t *wlc, wlc_if_t *wlcif, wlc_bsscfg_t *bsscfg,
	struct lbuf *lb, wl_arp_info_t * arpi)
{

	if (WSEC_ENABLED(bsscfg->wsec) && (WLPKTFLAG_MFP(WLPKTTAG(lb)) ||
	   (WSEC_TKIP_ENABLED(bsscfg->wsec) && wlc_sw_tkip_mic_enab(wlc, wlcif, bsscfg, lb))))

		return TRUE;

	if (ARPOE_ENAB(wlc->pub)) {
		if (arpi) {
			if (wl_arp_send_pktfetch_required(arpi, lb))
				return TRUE;
		}
	}

	if (ntoh16_ua((const void *)(PKTDATA(wlc->osh, lb) + ETHER_TYPE_OFFSET))
		== ETHER_TYPE_802_1X) {
		return TRUE;
	}

	return FALSE;
}

/*
 * Cancel all pending fetch requests
 * Requests could be waiting @
 *
 * 1. packet fetch queue @ rte_pktfetch layer
 * 2. Fetch request queue @ rte_fetch layer
 * 3. Fetch completion queue at pciedev bus layer
 *
 * This routine deletes all pending requests in rte layer.
 * Then cancels all pending fetch requests dispatched to bus layer.
 */

void
wlc_pktfetch_queue_flush(wlc_info_t * wlc)
{
	hnd_cancel_fetch_requests();
}
#endif /* BCMPCIEDEV */

#if !defined(BME_PKTFETCH)
#include <rte_pktfetch.h>

/*
 * HND_PKTFETCH - Asynchronous PktFetch:
 * This method used HNDRTE fetch service to fetch the payload from host to dongle buffer.
 *
 * On WLAN driver request for PktFetch, hnd pktfetch will save the packet context
 * pktfetch_info::ctx and schedule an HND fetch request.
 * On Fetch completion (DMA done callback), HNDRTE fetch service will invoke the registered
 * hnd pktfetch completion callback (pktfetch_info::cb).
 * In hnd pktfetch completion callback, original packet is freed and the new fetched packet will be
 * handed off to WLAN driver for Rx packet processing.
 *
 * In hnd_pktfetch PKTFREE callback (hnd_lbuf_free_cb), pending PktFetch requests will be
 * dispatched.
 */

#if defined(BCMSPLITRX)

#define PKTFETCH_FLAG_AMSDU_SUBMSDUS  0x01 /* AMSDU non-first MSDU"S */

typedef struct wlc_eapol_pktfetch_ctx {
	wlc_info_t *wlc;
	wlc_frminfo_t f;
	bool ampdu_path;
	bool ordered;
	struct pktfetch_info *pinfo;
	bool promisc;
	uint32 scb_assoctime;
	uint32 flags;
	struct ether_addr ea; /* Ethernet address of SCB */
} wlc_eapol_pktfetch_ctx_t;

static void wlc_recvdata_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctxt, bool cancelled);
static void wlc_recreate_frameinfo(wlc_info_t *wlc, void *lbuf, void *lfrag,
	wlc_frminfo_t *fold, wlc_frminfo_t *fnew);

/* Look up scb from frminfo, required for Rx pktfetch cases.
 * Similar to scb lookup in wlc_recvdata
 */
static void
wlc_pktfetch_get_scb(wlc_info_t *wlc, wlc_frminfo_t *f,
	wlc_bsscfg_t **bsscfg, struct scb **scb, bool promisc_frame, uint32 ctx_assoctime)
{
	struct ether_addr *bssid;
	enum wlc_bandunit bandunit;
	struct dot11_header *h = f->h;

	bandunit = CHSPEC_BANDUNIT(D11RXHDR_ACCESS_VAL(f->rxh, wlc->pub->corerev, RxChan));

	if (f->wds) {
		/* WDS frames */
		*scb = wlc_scbbssfindband(wlc, &h->a1, &h->a2,
			bandunit, bsscfg);
	} else {
		if ((f->fc & (FC_FROMDS | FC_TODS)) == 0) {
			/* IBSS case */
			bssid = &h->a3;
			*scb = wlc_scbibssfindband(wlc, &h->a2, bandunit, bsscfg);
		} else {
			if (f->fc & FC_TODS) {
				bssid = &h->a1;
			} else {
				bssid = &h->a2;
			}
#ifdef PSTA
			if (PSTA_ENAB(wlc->pub) && !f->ismulti)
				*bsscfg = wlc_bsscfg_find_by_hwaddr_bssid(wlc, &h->a1, bssid);
#endif
		}

		/* broadcast or multicast frames */
		if (*bsscfg == NULL)
			*bsscfg = wlc_bsscfg_find_by_bssid(wlc, bssid);

		/* Get scb info */
		if (*bsscfg && *scb == NULL)
			*scb = wlc_scbfindband(wlc, *bsscfg, &h->a2, bandunit);

	}

	/* Additional sanity for derived scb
	 * Make sure saved assoc time is same as derived scb assoc time
	 */
	if (*scb) {
		if ((*scb)->assoctime != ctx_assoctime)
			*scb = NULL;
	}
} /* wlc_pktfetch_get_scb */

int
wlc_recvdata_schedule_pktfetch(wlc_info_t *wlc, struct scb *scb,
	wlc_frminfo_t *f, bool promisc_frame, bool ordered, bool amsdu_msdus)
{
	wlc_eapol_pktfetch_ctx_t *ctx = NULL;
	struct pktfetch_info *pinfo = NULL;
	d11rxhdr_t *rxh;
	uint32 copycount = (wlc->pub->tunables->copycount << 2);
	uint headroom;
	int ret = BCME_OK;
	uint8 pad;

	pinfo = MALLOCZ(wlc->osh, sizeof(struct pktfetch_info) + sizeof(wlc_eapol_pktfetch_ctx_t));
	if (!pinfo) {
		WL_ERROR((WLC_MALLOC_ERR, WLCWLUNIT(wlc), __FUNCTION__,
			(int)sizeof(struct pktfetch_info), MALLOCED(wlc->osh)));
		ret = BCME_NOMEM;
		goto error;
	}

	pinfo->ctx_count = (sizeof(wlc_eapol_pktfetch_ctx_t) + sizeof(void*) - 1) / sizeof(void*);
	ctx = (wlc_eapol_pktfetch_ctx_t *) (pinfo + 1);
	ctx->wlc = wlc;

	/* are we using the existing packets wrxh or borrowed rxh */
	/* if f->wrxh is outside the headeroom, copy it in to current packet */
	if ((uchar *)f->wrxh <= (uchar *)PKTDATA(wlc->osh, f->p))
	{
		uint32 diff = 0;

		diff = (uchar *)PKTDATA(wlc->osh, f->p) - (uchar *)f->wrxh;
		WL_INFORM(("header adjust: diff: %d, data: %p, wrxh: %p, pktheadroom %d\n",
			diff, PKTDATA(wlc->osh, f->p), f->wrxh, PKTHEADROOM(wlc->osh, f->p)));
		if (diff <= PKTHEADROOM(wlc->osh, f->p)) {
			WL_INFORM(("rxheader in the packet\n"));
			goto wrxh_inline;
		}
	}
	/* wlc->datafiforxoff is the receive offset set in DMA engine (dma_attach_ext) */
	/* Check if mode4 is not enabled */
	if (PKTHEADROOM(wlc->osh, f->p) < wlc->datafiforxoff) {
		WL_ERROR(("no headroom to push the header\n"));
		OSL_SYS_HALT();
		ret = BCME_BUFTOOSHORT;
		goto error;
	}

	rxh = (d11rxhdr_t *)((uchar *)f->p + (LBUFFRAGSZ + WLC_RXHDR_LEN));
	pad = RXHDR_GET_PAD_LEN(rxh, wlc);
	headroom = PKTHEADROOM(wlc->osh, f->p);

	/* Copy rx header */
	PKTPUSH(wlc->osh, f->p, headroom);
	memcpy(PKTDATA(wlc->osh, f->p), (uchar *)f->wrxh, WL_RXHDR_LEN(wlc->pub->corerev));
	WL_INFORM(("f->wrsxh switching from %p to new %p\n", f->wrxh, PKTDATA(wlc->osh, f->p)));
	f->wrxh = (wlc_d11rxhdr_t *) PKTDATA(wlc->osh, f->p);
	f->rxh = &f->wrxh->rxhdr;
	PKTPULL(wlc->osh, f->p, headroom);

	if (pad) {
		wlc_rxhdr_set_pad_present(f->rxh, wlc);
	} else {
		wlc_rxhdr_clear_pad_present(f->rxh, wlc);
	}

wrxh_inline:
	memcpy(&ctx->f, f, sizeof(wlc_frminfo_t));
	if ((f->subtype == FC_SUBTYPE_QOS_DATA) &&
		SCB_AMPDU(scb) && wlc_scb_ampdurx_on_tid(scb, f->prio) &&
		!f->ismulti && !promisc_frame) {
		ctx->ampdu_path = TRUE;
	}
	/* Block processing of Classify FIFO (RX-FIFO2) when packet fetch is scheduled.
	 * This is done to ensure we don`t do any out of order procesing of management
	 * frames when packet fetch is not yet completed.
	 * One such example is processing of RX-DEAUTH frame from P2P-GO when EAP-FAILURE
	 * is being fetched from host
	 */
	wlc_bmac_classify_fifo_suspend(wlc->hw);

	ctx->ordered = ordered;
	ctx->promisc = promisc_frame;
	ctx->pinfo = pinfo;
	ctx->scb_assoctime = scb->assoctime;
	memcpy(&ctx->ea, &scb->ea,  sizeof(struct ether_addr));

	/* Headroom does not need to be > PKTRXFRAGSZ */
	pinfo->osh = wlc->osh;
	pinfo->headroom = PKTRXFRAGSZ;

	/* In case of RXMODE 3, host has full pkt including RxStatus (WL_HWRXOFF_AC bytes)
	 * Host buffer addresses in the lfrag points to start of this full pkt
	 * So, pktfetch host_offset needs to be set to (copycount + HW_RXOFF)
	 * so that only necessary (remaining) data if pktfetched from Host.
	*/
	if (SPLITRX_DYN_MODE3(wlc->osh, f->p)) {
		pinfo->host_offset = copycount + wlc->d11rxoff;
		PKTSETFRAGTOTLEN(wlc->osh, f->p,
			PKTFRAGTOTLEN(wlc->osh, f->p) + copycount + wlc->d11rxoff);
	}
	/* In RXMODE4 host has full 802.3 pkt starting from WL_HWRXOFF_AC.
	*  So to recreate orginal pkt dongle has to get the full payload excluding
	* WL_HWRXOFF_AC + Erhernet Header, as that part will be present in lfrag
	*/
	else if (SPLITRX_DYN_MODE4(wlc->osh, f->p)) {
		bool conv_type;

		conv_type = RXHDR_GET_CONV_TYPE(f->rxh, wlc);
		if (conv_type) {
			/* This is a type-2 conversion so hostoffset should be
			*  rxoffset + Ethernet Hdr + 2 bytes pad.
			*  This 2 bytes pad is absorbed into DOT3HDR_OFFSET[14 + 2].
			*/
			/* Its already converted header to 4 byte status */
			pinfo->host_offset = (wlc->datafiforxoff + DOT3HDR_OFFSET);
			PKTSETFRAGTOTLEN(wlc->osh, f->p, PKTFRAGTOTLEN(wlc->osh, f->p) +
					wlc->datafiforxoff + DOT3HDR_OFFSET);
		}
		else {
			/* This is Type-1 conversion so hostoffset should be
			* rxoffset + Ethernet Hdr + LLC hdr
			*/
			pinfo->host_offset = (wlc->datafiforxoff + DOT3HDR_OFFSET + LLC_HDR_LEN);
			PKTSETFRAGTOTLEN(wlc->osh, f->p, PKTFRAGTOTLEN(wlc->osh, f->p) +
					wlc->datafiforxoff + DOT3HDR_OFFSET + LLC_HDR_LEN);
		}
	} else {
		pinfo->host_offset = 0;
	}

	pinfo->lfrag = f->p;
	pinfo->cb = wlc_recvdata_pktfetch_cb;
	ctx->flags = 0;
	if (amsdu_msdus)
		ctx->flags = PKTFETCH_FLAG_AMSDU_SUBMSDUS;

#ifdef BCMPCIEDEV
	if (BCMPCIEDEV_ENAB()) {
		ret = hnd_pktfetch(pinfo);
		if (ret != BCME_OK) {
			wlc_bmac_classify_fifo_resume(wlc->hw, TRUE);
			WL_ERROR(("%s: pktfetch request rejected\n", __FUNCTION__));
			goto error;
		}
	}
#endif /* BCMPCIEDEV */
	return ret;

error:
	if (pinfo)
		MFREE(wlc->osh, pinfo, sizeof(struct pktfetch_info) +
			sizeof(wlc_eapol_pktfetch_ctx_t));

	if (f->p)
		PKTFREE(wlc->osh, f->p, FALSE);

	return ret;
}

static void
wlc_recvdata_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctxt, bool cancelled)
{
	struct pktfetch_info *pinfo = ctxt;
	wlc_eapol_pktfetch_ctx_t *ctx = (wlc_eapol_pktfetch_ctx_t *) (pinfo + 1);
	wlc_info_t *wlc = ctx->wlc;
	struct scb *scb = NULL;
	osl_t *osh = wlc->osh;
	wlc_frminfo_t f;
	bool ampdu_path = ctx->ampdu_path;
	bool ordered = ctx->ordered;
	bool promisc = ctx->promisc;
	wlc_bsscfg_t *bsscfg = NULL;

	ASSERT(pinfo->ctx_count ==
		(sizeof(wlc_eapol_pktfetch_ctx_t) + sizeof(void*) - 1) / sizeof(void*));

	if (cancelled) {
		/* lbuf could be NULL */
		if (lbuf) {
			PKTFREE(osh, lbuf, FALSE);
		}

		PKTFREE(osh, orig_lfrag, FALSE);
		WL_ERROR(("%s: Cancel Rx pktfetch for lfrag@%p type [%s]\n",
			__FUNCTION__, orig_lfrag, (lbuf) ? "IN_FETCH" : "IN_PKTFETCH"));
		goto done;
	}

	/* Replicate frameinfo buffer */
	memcpy(&f, &ctx->f, sizeof(wlc_frminfo_t));
	if (!(ctx->flags & PKTFETCH_FLAG_AMSDU_SUBMSDUS))
		wlc_recreate_frameinfo(wlc, lbuf, orig_lfrag, &ctx->f, &f);
	else {
		int err = 0;
		bsscfg = wlc_bsscfg_find(wlc, WLPKTTAGBSSCFGGET(orig_lfrag), &err);
		if (bsscfg)
			scb = wlc_scbfind(wlc, bsscfg, &ctx->ea);
		/* Subsequent subframe fetches */
		PKTPUSH(osh, lbuf, PKTLEN(osh, orig_lfrag));
		/* Copy the original lfrag data  */
		memcpy(PKTDATA(osh, lbuf), PKTDATA(osh, orig_lfrag), PKTLEN(osh, orig_lfrag));
		f.p = lbuf;
		/* Set length of the packet */
		PKTSETLEN(wlc->osh, f.p, PKTLEN(osh, orig_lfrag) +
			PKTFRAGTOTLEN(wlc->osh, orig_lfrag));
	}

	/* Copy PKTTAG */
	memcpy(WLPKTTAG(lbuf), WLPKTTAG(orig_lfrag), sizeof(wlc_pkttag_t));
	/* reset pkt next info old one needs to un chain, new one needs to be chained */
	PKTSETNEXT(osh, lbuf, PKTNEXT(osh, orig_lfrag));
	PKTSETNEXT(osh, orig_lfrag, NULL);

#if defined(STS_XFER_PHYRXS)
	ASSERT(WLPKTTAG(orig_lfrag)->phyrxs_seqid == STS_XFER_PHYRXS_SEQID_INVALID);
#else /* ! STS_XFER_PHYRXS */
	WLPKTTAG(orig_lfrag)->phystsbuf_idx = 0;
#endif /* ! STS_XFER_PHYRXS */

	/* Cleanup first before dispatch */
	PKTFREE(osh, orig_lfrag, FALSE);

	/* Extract scb info */
	if (!scb && !(ctx->flags & PKTFETCH_FLAG_AMSDU_SUBMSDUS))
		wlc_pktfetch_get_scb(wlc, &f, &bsscfg, &scb, promisc, ctx->scb_assoctime);

	if (scb == NULL) {
		/* Unable to acquire SCB: Clean up */
		WL_INFORM(("%s: Unable to acquire scb!\n", __FUNCTION__));
		PKTFREE(osh, lbuf, FALSE);
	} else {
		/*
		 * It is possible that the chip might have gone down into low power state
		* after scheduling pktfetch, so now make sure specific power islands are powered up
		*/
		if ((ctx->flags & PKTFETCH_FLAG_AMSDU_SUBMSDUS)) {
			/* non first MSDU/suframe and if its 802.1x frame then fetch come here */
			if (wlc_process_eapol_frame(wlc, scb->bsscfg, scb, &f, f.p)) {
				WL_INFORM(("Processed fetched msdu %p\n", f.p));
				/* We have consumed the pkt drop and continue; */
				PKTFREE(osh, f.p, FALSE);
			} else {
				f.da = (struct ether_addr *)PKTDATA(wlc->osh, f.p);
				f.wds = FALSE;

				/* Call sendup as frames are in Ethernet format */
				wlc_recvdata_sendup_msdus(wlc, scb, &f);
			}
		} else if (ampdu_path) {
			if (ordered)
				wlc_recvdata_ordered(wlc, scb, &f);
			else
				wlc_ampdu_recvdata(wlc->ampdu_rx, scb, &f);
		} else {
			wlc_recvdata_ordered(wlc, scb, &f);
		}
	}

done:

	/* Resume processing of Classify FIFO (FIFO2, Management frames) once packet fetching is
	 * completed
	 */
	wlc_bmac_classify_fifo_resume(wlc->hw, FALSE);

	MFREE(osh, pinfo, sizeof(struct pktfetch_info) + sizeof(wlc_eapol_pktfetch_ctx_t));
}

/* Recreates the frameinfo buffer based on offsets of the pulled packet */
static void
wlc_recreate_frameinfo(wlc_info_t *wlc, void *lbuf, void *lfrag,
	wlc_frminfo_t *fold, wlc_frminfo_t *fnew)
{
	osl_t *osh = wlc->osh;
	int16 offset_from_start;
	int16 offset = 0;
	uint8 pad = 0;
	uint8 dot11_offset = 0;
	unsigned char llc_hdr[8] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e};
	uint16* ether_type = NULL;

	/* During recvd frame processing, Pkt Data pointer was offset by some bytes
	 * Need to restore it to start
	 */
	offset_from_start = PKTDATA(osh, lfrag) - (uchar*)fold->wrxh;
	PKTPUSH(osh, lfrag, offset_from_start);

	/* lfrag data pointer is now at start of pkt.
	 * Push up lbuf data pointer by PKTLEN of lfrag
	 */
	if (SPLITRX_DYN_MODE4(osh, lfrag)) {
		uchar* fold_pbody = fold->pbody;
		bool conv_type;
		uint16 sflen = 0;
		uint16 insert_len = 0;
		uint32 copycount = (wlc->pub->tunables->copycount << 2);

		if (fold->isamsdu) {
			fold_pbody -= HDRCONV_PAD;
		}

		conv_type = RXHDR_GET_CONV_TYPE(&fold->wrxh->rxhdr, wlc);
		if (conv_type) {
			ether_type = (unsigned short*)(fold_pbody + ETHER_TYPE_2_OFFSET);
		} else {
			ether_type = (unsigned short*)(fold_pbody + ETHER_TYPE_1_OFFSET);
			/* Copy the LLC header for type 1 */
			memcpy(llc_hdr, ((uchar *)ether_type - LLC_START_OFFSET_LEN),
					LLC_START_OFFSET_LEN);
		}
		llc_hdr[6] = (uint8)*ether_type;
		llc_hdr[7] = (uint8)((*ether_type) >> 8);

		/* 802.11 hdr offset */
		dot11_offset = ((uchar*)fold->pbody) - (PKTDATA(osh, lfrag) + offset_from_start);

		insert_len = dot11_offset + LLC_HDR_LEN + offset_from_start;

		if (WLPKTTAG(lfrag)->flags & WLF_HWAMSDU) {
			/* Consider sub frame header and pad */
			insert_len += ETHER_HDR_LEN + HDRCONV_PAD;

			/* Calculate sub frame length */
			sflen = hton16(PKTLEN(osh, lbuf) + LLC_HDR_LEN);

			/* Adjust total length */
			fnew->totlen -= (copycount - (HDRCONV_PAD + ETHER_HDR_LEN + LLC_HDR_LEN));
		}

		PKTPUSH(osh, lbuf, insert_len);

		/* Copy the lfrag data starting from RxStatus (wlc_d11rxhdr_t) */
		offset = 0;
		memcpy(PKTDATA(osh, lbuf), PKTDATA(osh, lfrag), offset_from_start);
		offset += offset_from_start;

		/* Copy pad */
		if (WLPKTTAG(lfrag)->flags & WLF_HWAMSDU) {
			pad = HDRCONV_PAD;
			offset += HDRCONV_PAD;
			wlc_rxhdr_set_pad_present(fnew->rxh, wlc);
		}

		/* Copy 802.11 hdr */
		memcpy((PKTDATA(osh, lbuf) + offset), (PKTDATA(osh, lfrag) + offset),
			dot11_offset);
		offset += dot11_offset;

		/* Copy sub frame header */
		if (WLPKTTAG(lfrag)->flags & WLF_HWAMSDU) {
			memcpy((PKTDATA(osh, lbuf) + offset), fold->pbody, ETHER_TYPE_OFFSET);
			offset += ETHER_TYPE_OFFSET;
			memcpy((PKTDATA(osh, lbuf) + offset), &sflen, ETHER_TYPE_LEN);
			offset += ETHER_TYPE_LEN;
		}

		/* Copy llc header */
		memcpy((PKTDATA(osh, lbuf) + offset), llc_hdr, LLC_HDR_LEN);
	} else {
		PKTPUSH(osh, lbuf, PKTLEN(osh, lfrag));
		/* Copy the lfrag data starting from RxStatus (wlc_d11rxhdr_t) */
		memcpy(PKTDATA(osh, lbuf), PKTDATA(osh, lfrag), PKTLEN(osh, lfrag));
	}

	/* wrxh and rxh pointers: both have same address */
	fnew->wrxh = (wlc_d11rxhdr_t *)PKTDATA(osh, lbuf);
	fnew->rxh = &fnew->wrxh->rxhdr;

	/* Restore data pointer of lfrag and lbuf */
	PKTPULL(osh, lfrag, offset_from_start);
	PKTPULL(osh, lbuf, offset_from_start);

	/* Calculate offset of dot11_header from original lfrag
	 * and apply the same to lbuf frameinfo
	 */
	offset = (((uchar*)fold->h) - PKTDATA(osh, lfrag)) + pad;
	fnew->h = (struct dot11_header *)(PKTDATA(osh, lbuf) + offset);

	/* Calculate the offset of Packet body pointer
	 * from original lfrag and apply to lbuf frameinfo
	 */
	offset = (((uchar*)fold->pbody) - PKTDATA(osh, lfrag)) + pad;
	fnew->pbody = (uchar*)(PKTDATA(osh, lbuf) + offset);

	fnew->p = lbuf;
}

#if defined(PKTC) || defined(PKTC_DONGLE)
static void wlc_sendup_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctxt, bool cancelled);

bool
wlc_sendup_chain_pktfetch_required(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, void *p,
		uint16 body_offset)
{
	uint body_len;
	struct dot11_llc_snap_header *lsh;

	/* For AMSDU pkts, packet starts after subframe header */
	if (WLPKTTAG(p)->flags & WLF_HWAMSDU)
		body_offset += ETHER_HDR_LEN;

	lsh = (struct dot11_llc_snap_header *) (PKTDATA(wlc->osh, p) + body_offset);
	body_len = PKTLEN(wlc->osh, p) - body_offset;
	BCM_REFERENCE(body_len);

	if (PKTFRAGUSEDLEN(wlc->osh, p) > 0) {
		return (wlc_pktfetch_checkpkt(wlc, bsscfg, p, lsh, body_len));
	}
	return FALSE;
}

void
wlc_sendup_schedule_pktfetch(wlc_info_t *wlc, void *pkt, uint32 body_offset)
{
	struct pktfetch_info *pinfo = NULL;
	int ctx_count = 2;	/* No. of ctx variables needed to be saved */
	uint32 copycount = (wlc->pub->tunables->copycount << 2);

	pinfo = MALLOCZ(wlc->osh, (sizeof(struct pktfetch_info) + ctx_count*sizeof(void*)));
	if (!pinfo) {
		WL_ERROR(("%s: Out of mem: Unable to alloc pktfetch ctx!\n", __FUNCTION__));
		goto error;
	}

	/* Fill up context */
	pinfo->ctx_count = ctx_count;
	pinfo->ctx[0] = (void *)wlc;
	pinfo->ctx[1] = (void *)body_offset;
	/* Fill up pktfetch info */
	/* In case of RXMODE 3, host has full pkt including RxStatus (WL_HWRXOFF_AC bytes)
	 * Host buffer addresses in the lfrag points to start of this full pkt
	 * So, pktfetch host_offset needs to be set to (copycount + HW_RXOFF)
	 * so that only necessary (remaining) data if pktfetched from Host.
	*/
	if (SPLITRX_DYN_MODE3(wlc->osh, pkt)) {
		pinfo->host_offset = copycount + wlc->d11rxoff;
		PKTSETFRAGTOTLEN(wlc->osh, pkt,
			PKTFRAGTOTLEN(wlc->osh, pkt) + copycount + wlc->d11rxoff);
	}
	else
		pinfo->host_offset = 0;

	pinfo->osh = wlc->osh;
	pinfo->headroom = PKTRXFRAGSZ;

	/* if key processing done, make headroom to save body_offset */
	if (WLPKTTAG(pkt)->flags & WLF_RX_KM)
		pinfo->headroom += PKTBODYOFFSZ;
	pinfo->lfrag = (void*)pkt;
	pinfo->cb = wlc_sendup_pktfetch_cb;
	pinfo->next = NULL;
	if (hnd_pktfetch(pinfo) != BCME_OK) {
		WL_ERROR(("%s: pktfetch request rejected\n", __FUNCTION__));
		goto error;
	}

	return;

error:

	if (pinfo)
		MFREE(wlc->osh, pinfo, sizeof(struct pktfetch_info) + ctx_count*sizeof(void*));

	if (pkt)
		PKTFREE(wlc->osh, pkt, FALSE);
}

static void
wlc_sendup_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctxt, bool cancelled)
{
	wlc_info_t *wlc;
	uint lcl_len;
	struct pktfetch_info *pinfo = (struct pktfetch_info *) ctxt;
	uint32 body_offset;

	ASSERT(pinfo->ctx_count == 2);
	/* Retrieve contexts */
	wlc = (wlc_info_t *)pinfo->ctx[0];
	body_offset = (uint32)pinfo->ctx[1];

	lcl_len = PKTLEN(wlc->osh, orig_lfrag);
	PKTPUSH(wlc->osh, lbuf, lcl_len);
	memcpy(PKTDATA(wlc->osh, lbuf), PKTDATA(wlc->osh, orig_lfrag), lcl_len);

	/* append body_offset if key management has been processed */
	if (WLPKTTAG(orig_lfrag)->flags & WLF_RX_KM) {
		PKTPUSH(wlc->osh, lbuf, PKTBODYOFFSZ);
		*((uint32 *)PKTDATA(wlc->osh, lbuf)) = body_offset;
	}
	/* Copy wl pkttag area */
	wlc_pkttag_info_move(wlc, orig_lfrag, lbuf);
	PKTSETIFINDEX(wlc->osh, lbuf, PKTIFINDEX(wlc->osh, orig_lfrag));
	PKTSETPRIO(lbuf, PKTPRIO(orig_lfrag));

	PKTSETNEXT(wlc->osh, lbuf, PKTNEXT(wlc->osh, orig_lfrag));
	PKTSETNEXT(wlc->osh, orig_lfrag, NULL);

	/* Free the original pktfetch_info and generic ctx  */
	MFREE(wlc->osh, pinfo, sizeof(struct pktfetch_info)+ pinfo->ctx_count*sizeof(void *));

#if defined(STS_XFER_PHYRXS)
	ASSERT(WLPKTTAG(orig_lfrag)->phyrxs_seqid == STS_XFER_PHYRXS_SEQID_INVALID);
#else /* ! STS_XFER_PHYRXS */
	WLPKTTAG(orig_lfrag)->phystsbuf_idx = 0;
#endif /* ! STS_XFER_PHYRXS */

	PKTFREE(wlc->osh, orig_lfrag, TRUE);

	/* Mark this lbuf has pktfetched lbuf pkt */
	PKTSETPKTFETCHED(wlc->osh, lbuf);

	wlc_sendup_chain(wlc, lbuf);
}
#endif /* PKTC || PKTC_DONGLE */

#if defined(BCMPCIEDEV)

static void wlc_rx_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctx, bool cancelled);

/**
 * For e.g. ftp packets in NATOE, it is necessary to transfer the full rx packet
 * from host memory into CPU RAM, so firmware can parse packet contents before
 * forwarding for transmission.
 */
int
wlc_rx_pktfetch(wlc_info_t *wlc, wlc_if_t *wlcif, struct lbuf *lb)
{
	struct pktfetch_info *pinfo = NULL;
	int ctx_count = 4;	/* No. of ctx variables needed to be saved */
	/* intention is to store ID - bsscfg Instance ID - to track delete before fetch cb */

	uint32 bsscfg_id;
	wlc_bsscfg_t *bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);

	osl_t * osh = wlc->osh;
	int ret = BCME_OK;

	if (bsscfg) {
		bsscfg_id = bsscfg->ID;
	} else {
		WL_ERROR(("%s: bsscfg is NULL!\n", __FUNCTION__));
		ret = BCME_ERROR;
		goto error;
	}

	pinfo = MALLOCZ(osh, sizeof(struct pktfetch_info) + ctx_count*sizeof(void*));
	if (!pinfo) {
		WL_ERROR(("%s: Out of mem: Unable to alloc pktfetch ctx!\n", __FUNCTION__));
		ret = BCME_NOMEM;
		goto error;
	}

	/* Fill up context */
	pinfo->ctx_count = ctx_count;
	pinfo->ctx[0] = (void *)wlc;
	pinfo->ctx[1] = (void *)wlcif;
	pinfo->ctx[2] = (void *)bsscfg;
	pinfo->ctx[3] = (void *)bsscfg_id;

	/* Fill up pktfetch info */
	/* Headroom does not need to be > PKTRXFRAGSZ */
	pinfo->osh = osh;
	pinfo->headroom = PKTRXFRAGSZ;
	pinfo->lfrag = (void*)lb;
	pinfo->cb = wlc_rx_pktfetch_cb;

	pinfo->host_offset = 0;

	ret = hnd_pktfetch(pinfo);
	if (ret != BCME_OK) {
		WL_ERROR(("%s: pktfetch request rejected\n", __FUNCTION__));
		goto error;
	}

	return ret;

error:
	if (pinfo) {
		MFREE(osh, pinfo, sizeof(struct pktfetch_info) + ctx_count*sizeof(void *));
	}

	if (lb) {
		PKTFREE(osh, lb, FALSE);
	}
	return ret;
}

static void
wlc_rx_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctx, bool cancelled)
{
	wlc_info_t *wlc;
	wlc_if_t *wlcif;
	osl_t *osh;
	uint32 bsscfg_ID;
	wlc_bsscfg_t *bsscfg = NULL;
	struct pktfetch_info *pinfo = ctx;

	ASSERT(pinfo->ctx_count == 4);

	/* Retrieve contexts */
	wlc = (wlc_info_t *)pinfo->ctx[0];
	wlcif = (wlc_if_t *)pinfo->ctx[1];
	bsscfg = (wlc_bsscfg_t *)pinfo->ctx[2];
	bsscfg_ID = (uint32)pinfo->ctx[3];

	osh = pinfo->osh;

	if (bsscfg != wlc_bsscfg_find_by_ID(wlc, bsscfg_ID)) {
		/* Drop the pkt, bsscfg not valid */
		WL_ERROR(("wl%d: Bsscfg %p Was freed during pktfetch, Drop the rqst \n",
			WLCWLUNIT(wlc), bsscfg));
		PKTFREE(osh, lbuf, FALSE);
		goto done;
	}

	/* Subsequent subframe fetches */
	PKTPUSH(osh, lbuf, PKTLEN(osh, orig_lfrag));

	/* Copy the original lfrag data  */
	memcpy(PKTDATA(osh, lbuf), PKTDATA(osh, orig_lfrag), PKTLEN(osh, orig_lfrag));

	/* Copy PKTTAG */
	memcpy(WLPKTTAG(lbuf), WLPKTTAG(orig_lfrag), sizeof(wlc_pkttag_t));

	PKTSETNEXT(osh, lbuf, NULL);

	wl_sendup(wlc->wl, wlcif->wlif, lbuf, 1);

done:
	PKTFREE(osh, orig_lfrag, FALSE);

	MFREE(osh, pinfo, sizeof(struct pktfetch_info)+ pinfo->ctx_count*sizeof(void *));
}

#endif /* BCMPCIEDEV */
#endif /* BCMSPLITRX */

#if defined(BCMPCIEDEV)
static void wlc_tx_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctx, bool cancelled);

/**
 * For e.g. 802.1x packets, it is necessary to transfer the full packet from host memory into CPU
 * RAM, so firmware can parse packet contents before transmission.
 */
void
wlc_tx_pktfetch(wlc_info_t *wlc, struct lbuf *lb, void *src, void *dev,
	wlc_bsscfg_t *bsscfg)
{
	struct pktfetch_info *pinfo = NULL;
	int ctx_count = 5;	/* No. of ctx variables needed to be saved */
	/* intention is to store ID - bsscfg Instance ID - to track delete before fetch cb */
	uint32 bsscfg_id = bsscfg->ID;

	pinfo = MALLOCZ(wlc->osh, sizeof(struct pktfetch_info) + ctx_count*sizeof(void*));
	if (!pinfo) {
		WL_ERROR(("%s: Out of mem: Unable to alloc pktfetch ctx!\n", __FUNCTION__));
		goto error;
	}

	/* Fill up context */
	pinfo->ctx_count = ctx_count;
	pinfo->ctx[0] = (void *)wlc;
	pinfo->ctx[1] = (void *)src;
	pinfo->ctx[2] = (void *)dev;
	pinfo->ctx[3] = (void *)bsscfg;
	pinfo->ctx[4] = (void *)bsscfg_id;

	/* Fill up pktfetch info */
#ifdef BCM_DHDHDR
	pinfo->host_offset = (-DOT11_LLC_SNAP_HDR_LEN);
#else
	pinfo->host_offset = 0;
#endif

	/* Need headroom of atleast 224 for TXOFF/amsdu headroom
	 * Rounded to 256
	 */
	pinfo->headroom = PKTFETCH_DEFAULT_HEADROOM;
	pinfo->lfrag = (void*)lb;
	pinfo->cb = wlc_tx_pktfetch_cb;
	pinfo->next = NULL;
	pinfo->osh = wlc->osh;
	if (hnd_pktfetch(pinfo) != BCME_OK) {
		//WL_ERROR(("%s: pktfetch request rejected\n", __FUNCTION__));
		goto error;
	}

	return;

error:
	if (pinfo)
		MFREE(wlc->osh, pinfo, sizeof(struct pktfetch_info) + ctx_count*sizeof(void*));

	if (lb)
		PKTFREE(wlc->osh, lb, TRUE);

}

/** Packet fetch callback. BCMPCIEDEV specific */
static void
wlc_tx_pktfetch_cb(void *lbuf, void *orig_lfrag, void *ctx, bool cancelled)
{
	wlc_info_t *wlc;
	struct pktfetch_info *pinfo = (struct pktfetch_info *)ctx;
	void *src, *dev;
	uint32 bsscfg_ID;
	wlc_bsscfg_t *bsscfg = NULL;

	ASSERT(pinfo->ctx_count == 5);
	/* Retrieve contexts */
	wlc = (wlc_info_t*)pinfo->ctx[0];
	src = (void *)pinfo->ctx[1];
	dev = (void *)pinfo->ctx[2];
	bsscfg = (wlc_bsscfg_t *)pinfo->ctx[3];
	bsscfg_ID = (uint32)pinfo->ctx[4];

#if defined(BCM_DHDHDR) && defined(DONGLEBUILD)
	if (PKTISTXFRAG(wlc->osh, orig_lfrag)) {
		if (lbuf && lbuf != orig_lfrag)
			ASSERT(0);
	} else
#endif
	{
		PKTSETNEXT(wlc->osh, orig_lfrag, lbuf);
	}

	/* Handle the pktfetch cancelled case */
	if (cancelled) {
		/* lbuf could be NULL */
		PKTFREE(wlc->osh, orig_lfrag, TRUE);
		WL_ERROR(("%s: Cancel Tx pktfetch for lfrag@%p type [%s]\n",
			__FUNCTION__, orig_lfrag, (lbuf) ? "IN_FETCH" : "IN_PKTFETCH"));
		goto done;
	}

	PKTSETFRAGTOTLEN(wlc->osh, orig_lfrag, 0);
	PKTSETFRAGLEN(wlc->osh, orig_lfrag, LB_FRAG1, 0);

	/* When BCM_DHDHDR is enabled, all tx packets that need to be fetched will
	 * include llc snap 8B header at start of lbuf.
	 * So we can do PKTSETFRAGTOTNUM here as well.
	 */
	PKTSETFRAGTOTNUM(wlc->osh, orig_lfrag, 0);

	/* The hnd_pktfetch_dispatch may get lbuf from PKTALLOC and the pktalloced counter
	 * will be increased by 1, later in the wl_send the PKTFROMNATIVE will increase 1 again
	 * for !lb_pool lbuf. (dobule increment)
	 * Here do PKTTONATIVE to decrease it before wl_send.
	 */
	if (lbuf && !PKTPOOL(wlc->osh, lbuf)) {
		PKTTONATIVE(wlc->osh, lbuf);
	}

	if (bsscfg != wlc_bsscfg_find_by_ID(wlc, bsscfg_ID)) {
		/* Drop the pkt, bsscfg not valid */
		WL_ERROR(("wl%d: Bsscfg %p Was freed during pktfetch, Drop the rqst \n",
			wlc->pub->unit, bsscfg));
		PKTFREE(wlc->osh, orig_lfrag, TRUE);
	} else {
		wl_send_cb(wlc->wl, src, dev, orig_lfrag);
	}

done:
	/* Free the original pktfetch_info and generic ctx  */
	MFREE(wlc->osh, pinfo, sizeof(struct pktfetch_info) + pinfo->ctx_count*sizeof(void *));
}

#endif /* BCMPCIEDEV */

#else /* BME_PKTFETCH */

#include <hndbme.h>

/*
 * ------------------------------------------------------------------------------------------------
 *  BME_PKTFETCH - Synchronous PktFetch
 *
 *  In Split Rx/Tx modes, dongle lfrag contains only part of the frame required for WLAN processing.
 *  - Tx packet: 802.11 Header
 *  - Rx packet: 802.11 header + part of payload (wlc_tunables_t::copycount)
 *  But in some uses cases (like EAPOL, SW TKIP, ARPOE, NATOE) WLAN driver requires complete packet
 *  in the dongle and has to be fetched from the host.
 *
 *  Synchronous PktFetch uses Byte Move Engine (BME) user BME_USR_H2D to perform Host DDR to
 *  Dongle SysMem DMA.
 *
 *  On demand, BME_PKTFETCH will initiate a DMA transfer and poll for the DMA completion.
 * ------------------------------------------------------------------------------------------------
 */

#if !defined(BCMHME) || !defined(HNDBME)
/* BME_USR_H2D is registered by HME module */
#error "BME_PKTFETCH is dependent on BCMHME and HNDBME"
#endif

#define PKTFETCH_MIN_MEM_FOR_HEAPALLOC		(16 * 1024)

#if defined(BCM_DHDHDR) && defined(DONGLEBUILD)
/* Set default pktfetch headroom to 256:
 * greater than TXOFF + amsdu headroom requirements
 */
#define PKTFETCH_DEFAULT_HEADROOM	256
#else
#define PKTFETCH_DEFAULT_HEADROOM	0
#endif /* BCM_DHDHDR && DONGLEBUILD */

/* ------------------------------------------------------------------------------------------------
 * Section: Packet Fetch using BME service Initialization and User Configuration API
 * ------------------------------------------------------------------------------------------------
 */

/* BME_PKFETCH module statistics */
#if defined(BCMDBG)
#include <wlc_dump.h>
#define BME_PKTFETCH_STATS
#endif

#if defined(BME_PKTFETCH_STATS)
typedef struct bme_pktfetch_stats
{
	uint32 tot_pkts;
	uint32 rx_pkts;
	uint32 tx_pkts;
	uint32 error;
} bme_pktfetch_stats_t;

#define BME_PKTFETCH_STATS_INC(_cntr)		((_cntr)++);

static int wlc_bme_pktfetch_dump(void *ctx, struct bcmstrbuf *b);
static int wlc_bme_pktfetch_dump_clr(void *ctx);

#else /* ! BME_PKTFETCH_STATS */
#define BME_PKTFETCH_STATS_INC(_cntr)
#endif /* ! BME_PKTFETCH_STATS */

/* BME_PKTFETCH module info */
typedef struct wlc_bme_pktfetch {
	osl_t		*osh;		// RTE osh
	wlc_info_t	*wlc;
	struct pktpool	*pktpool;	// Shared pktpool
	int             h2d_bme_key;    // BME_USR_H2D bme key
#if defined(BME_PKTFETCH_STATS)
	bme_pktfetch_stats_t	stats;
#endif /* BME_PKTFETCH_STATS */
	uint8           init;           // init state: TRUE | FALSE
} wlc_bme_pktfetch_t;

/* ------------------------------------------------------------------------------------------------
 * Section: Globals
 * ------------------------------------------------------------------------------------------------
 */
wlc_bme_pktfetch_t bme_pktfetch_g;

#define WLC_BME_PKTFETCH()		(&bme_pktfetch_g) // singleton global

/**
 * ------------------------------------------------------------------------------------------------
 *  BME_PKTFETCH Initialization API
 *
 *  Initialize Mailbox service with the Run Time Environment (RTOS).
 *  HME service registers BME user BME_USR_H2D and BME_PKTFETCH acquires
 *  BME_USR_H2D bme_key for packet fetching.
 *
 *  Caveats:
 *  BME service and HME services must be initialized before BME_PKTFETCH service.
 * ------------------------------------------------------------------------------------------------
 */
int
BCMINITFN(wlc_bme_pktfetch_init)(wlc_info_t *wlc)
{
	wlc_bme_pktfetch_t *bme_pktfetch;

	bme_pktfetch = WLC_BME_PKTFETCH();

	if (bme_pktfetch->init) {
	       WL_ERROR(("%s: BME_PKTFETCH is already initialized\n", __FUNCTION__));
	       return BCME_OK;
	}

	bme_pktfetch->wlc = wlc;
	bme_pktfetch->h2d_bme_key = bme_get_key(wlc->osh, BME_USR_H2D);
	bme_pktfetch->pktpool = SHARED_POOL;
	bme_pktfetch->init = TRUE;

#if defined(BME_PKTFETCH_STATS)
	wlc_dump_add_fns(wlc->pub, "bme_pktfetch", wlc_bme_pktfetch_dump,
		wlc_bme_pktfetch_dump_clr, (void *) bme_pktfetch);
#endif /* BME_PKTFETCH_STATS */

	WL_TRACE(("%s: H2D BME key 0x%08x\n", __FUNCTION__, bme_pktfetch->h2d_bme_key));

	return BCME_OK;

} /* wlc_bme_pktfetch_init() */

#if defined(BME_PKTFETCH_STATS)
/** Dump BME_PKTFETCH stats */
static int
wlc_bme_pktfetch_dump(void *ctx, struct bcmstrbuf *b)
{
	wlc_bme_pktfetch_t *bme_pktfetch = (wlc_bme_pktfetch_t *) ctx;
	bme_pktfetch_stats_t *stats = &bme_pktfetch->stats;

	bcm_bprintf(b, "BME_PKFETCH: tot_pkts[%d] rx_pkts[%d] tx_pkts[%d] error[%d]\n",
		stats->tot_pkts, stats->rx_pkts, stats->tx_pkts, stats->error);
	return BCME_OK;
} /* wlc_bme_pktfetch_dump() */

/** Clear BME_PKTFETCH stats */
static int
wlc_bme_pktfetch_dump_clr(void *ctx)
{
	wlc_bme_pktfetch_t *bme_pktfetch = (wlc_bme_pktfetch_t *) ctx;

	memset((void*)&bme_pktfetch->stats, 0, sizeof(bme_pktfetch_stats_t));
	return BCME_OK;
} /* wlc_bme_pktfetch_dump_clr() */
#endif /* BME_PKTFETCH_STATS */

#if defined(BCMSPLITRX) || defined(BCMPCIEDEV)
/**
 * ------------------------------------------------------------------------------------------------
 *  Allocate a new packet from shared pool and initiate a BME transfer of host packet data to
 *  newly allocated packet.
 *  host_offset - Shift the Host data/len by offset
 *  copy_dngl: If set, dongle data from the original packet is copied to newly allocated packet
 *
 *  Return: Fetched packet
 * ------------------------------------------------------------------------------------------------
 */
static void *
_bme_pktfetch_rx(wlc_info_t *wlc, void *pkt, int host_offset, bool copy_dngl)
{
	wlc_bme_pktfetch_t *bme_pktfetch;
	osl_t	*osh;
	void	*fetch_pkt;
	uint64	msg_src64, msg_dst64;
	int	bme_eng_idx;
	uint32	host_pktlen;
	int16	headroom;

	osh = wlc->osh;
	bme_pktfetch = WLC_BME_PKTFETCH();

	WL_TRACE(("wl%d: %s: ENTERo host_offset[%d] copy_dngl[%s]\n", wlc->pub->unit, __FUNCTION__,
		host_offset, (copy_dngl ? "TRUE" : "FALSE")));

	ASSERT(bme_pktfetch->h2d_bme_key != ~0U);
	ASSERT(PKTISRXFRAG(osh, pkt));

	/* Allocate a packet from shared pktpool */
	fetch_pkt = pktpool_get(bme_pktfetch->pktpool);

	/* If no free packets in shared pktpool then allocate a packet from dongle memory */
	if ((fetch_pkt == NULL) &&
		((OSL_MEM_AVAIL() < PKTFETCH_MIN_MEM_FOR_HEAPALLOC) ||
			((fetch_pkt = PKTALLOC(osh, MAXPKTDATABUFSZ, lbuf_basic)) == NULL))) {

		BME_PKTFETCH_STATS_INC(bme_pktfetch->stats.error);

		WL_ERROR(("wl%d: %s: PKTALLOC failed; OSL_MEM_AVAIL %d\n",
			wlc->pub->unit, __FUNCTION__, OSL_MEM_AVAIL()));
		return NULL;
	}

	headroom = PKTRXFRAGSZ; // Headroom does not need to be > PKTRXFRAGSZ
	host_pktlen = PKTFRAGTOTLEN(osh, pkt) - host_offset;

	/* Get Host (src) buffer address and unset Hi bit if set */
	msg_src64 = (((uint64)(PKTFRAGDATA_HI(osh, pkt, LB_FRAG1) & 0x7fffffff)) << 32) |
		(PKTFRAGDATA_LO(osh, pkt, LB_FRAG1) + host_offset);

	/* Get Dongle (dst) buffer address */
	msg_dst64 = (uint64)((uint32)PKTDATA(osh, fetch_pkt) + headroom);

	WL_INFORM(("%s: BME copy msg_src64[0x%x:%x] msg_dst64[0x%x:%x] msg_len[%d]\n", __FUNCTION__,
		(uint32)(msg_src64 >> 32), (uint32)msg_src64,
		(uint32)(msg_dst64 >> 32), (uint32)msg_dst64, host_pktlen));

	/* Initiate BME (DMA) copy */
	bme_eng_idx = bme_copy64(wlc->osh, bme_pktfetch->h2d_bme_key,
			msg_src64, msg_dst64, host_pktlen);

	BME_PKTFETCH_STATS_INC(bme_pktfetch->stats.rx_pkts);
	BME_PKTFETCH_STATS_INC(bme_pktfetch->stats.tot_pkts);

	/* Mark new packet as pktfetched lbuf pkt */
	PKTSETPKTFETCHED(wlc->osh, fetch_pkt);
	PKTSETIFINDEX(osh, fetch_pkt, PKTIFINDEX(osh, pkt));
	PKTSETLEN(osh, fetch_pkt, (headroom + host_pktlen));

	if (copy_dngl) {
		uint8	*memcpy_dst;
		uint16	dngl_pktlen;

		/* Copy the Dongle pkt data to new dongle buffer */
		dngl_pktlen = PKTLEN(osh, pkt);
		memcpy_dst = PKTDATA(osh, fetch_pkt) + (headroom - dngl_pktlen);
		memcpy(memcpy_dst, PKTDATA(osh, pkt), dngl_pktlen);

		PKTPULL(osh, fetch_pkt, (headroom - dngl_pktlen));
	} else {
		/* Caller will manage the dongle packet data. */
		PKTPULL(osh, fetch_pkt, headroom);
	}

	/* Copy PKTTAG */
	memcpy(WLPKTTAG(fetch_pkt), WLPKTTAG(pkt), sizeof(wlc_pkttag_t));

	PKTSETNEXT(osh, fetch_pkt, NULL);

	/* Ensure the BME copy is completed */
	bme_sync_eng(wlc->osh, bme_eng_idx);

	WL_INFORM(("wl%d: %s: Host pktdata is fetched from orig_pkt[%p] to new fetch_pkt[%p]\n",
		wlc->pub->unit, __FUNCTION__, pkt, fetch_pkt));

	/* Return new fetch_pkt to the caller */
	return fetch_pkt;
} /* _bme_pktfetch_rx() */
#endif /* BCMSPLITRX || BCMPCIEDEV */

#if defined(BCMSPLITRX)
/**
 * ------------------------------------------------------------------------------------------------
 *  Fetch the hostdata for a receive packet from slow path (wlc_recv()) and
 *  recreate the frame info.
 *
 *  Return
 *	BCME_OK: On PktFetch success
 *		- f->p points to newly allocated packet with complete data.
 *		- Original packet is freed.
 *	bcm error: On PktFetch failure
 *		- f->p points to original packet.
 *		- Caller has to own the packet.
 * ------------------------------------------------------------------------------------------------
 */
int
wlc_bme_pktfetch_recvdata(wlc_info_t *wlc, wlc_frminfo_t *f, bool amsdu_msdus)
{
	osl_t	*osh;
	void	*fetch_pkt, *pkt;
	uint32	copycount;
	int16	host_offset;
	int	ret;
	bool	copy_dngl;

	osh = wlc->osh;
	pkt = f->p;
	host_offset = 0;
	copycount = (wlc->pub->tunables->copycount << 2);
	ret = BCME_OK;

	WL_TRACE(("wl%d: %s: ENTER amsdu_msdus[%s] SPLITRX_DYN_MODE3[%d] SPLITRX_DYN_MODE4[%d]\n",
		wlc->pub->unit, __FUNCTION__, (amsdu_msdus ? "TRUE" : "FALSE"),
		SPLITRX_DYN_MODE3(osh, pkt), SPLITRX_DYN_MODE4(osh, pkt)));
	ASSERT(PKTISRXFRAG(osh, pkt));

	/* In case of RXMODE 3, host has full pkt including RxStatus (WL_HWRXOFF_AC bytes)
	 * Host buffer addresses in the lfrag points to start of this full pkt
	 * So, pktfetch host_offset needs to be set to (copycount + HW_RXOFF)
	 * so that only necessary (remaining) data if pktfetched from Host.
	*/
	if (SPLITRX_DYN_MODE3(osh, pkt)) {
		host_offset = copycount + wlc->d11rxoff;
		PKTSETFRAGUSEDLEN(osh, pkt, PKTFRAGUSEDLEN(osh, pkt) + host_offset);
	}
	/* In RXMODE4 host has full 802.3 pkt starting from WL_HWRXOFF_AC.
	*  So to recreate orginal pkt dongle has to get the full payload excluding
	* WL_HWRXOFF_AC + Erhernet Header, as that part will be present in lfrag
	*/
	else if (SPLITRX_DYN_MODE4(osh, pkt)) {

		if (RXHDR_GET_CONV_TYPE(f->rxh, wlc)) {
			/* This is a type-2 conversion so hostoffset should be
			*  rxoffset + Ethernet Hdr + 2 bytes pad.
			*  This 2 bytes pad is absorbed into DOT3HDR_OFFSET[14 + 2].
			*/
			/* Its already converted header to 4 byte status */
			host_offset = (wlc->datafiforxoff + DOT3HDR_OFFSET);
		} else {
			/* This is Type-1 conversion so hostoffset should be
			* rxoffset + Ethernet Hdr + LLC hdr
			*/
			host_offset = (wlc->datafiforxoff + DOT3HDR_OFFSET + LLC_HDR_LEN);
		}

		PKTSETFRAGUSEDLEN(osh, pkt, PKTFRAGUSEDLEN(osh, pkt) + host_offset);
	}

	copy_dngl = amsdu_msdus || !SPLITRX_DYN_MODE4(osh, pkt);

	/* Schedule and sync BME transfer of host packet data */
	if ((fetch_pkt = _bme_pktfetch_rx(wlc, pkt, host_offset, copy_dngl)) == NULL) {
		ret = BCME_ERROR;
		goto error;
	}

	if (amsdu_msdus) {
		/* Return fetched packet */
		f->p = fetch_pkt;
		f->da = (struct ether_addr *)PKTDATA(wlc->osh, f->p);
		f->wds = FALSE;

#if defined(BCMDBG)
		/* AMSDU deagg should not access following fields; For sanity marking as NULL */
		f->wrxh = NULL;
		f->rxh = NULL;
		f->pbody = NULL;
		f->h = NULL;
#endif /* BCMDBG */

		ret = BCME_OK;
	} else {
		int16 offset = 0;
		uint8 pad = 0;
		bool wrxh_inline = FALSE;

		/* In RXMODE4, WLAN header is converted (802.11 to 802.3) so recreate the
		 * dongle packet with WLAN header (with AMSDU subframe header if applicable).
		 */
		if (SPLITRX_DYN_MODE4(osh, pkt)) {
			uint8	*fpkt_pbody = f->pbody;
			uint16	sflen = 0;
			uint16	insert_len = 0;
			uint8	llc_hdr[8] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e};
			uint16	*ether_type = NULL;
			uint8	dot11_offset = 0;

			if (f->isamsdu) {
				fpkt_pbody -= HDRCONV_PAD;
			}

			if (RXHDR_GET_CONV_TYPE(f->rxh, wlc)) {
				ether_type = (unsigned short*)(fpkt_pbody + ETHER_TYPE_2_OFFSET);
			} else {
				ether_type = (unsigned short*)(fpkt_pbody + ETHER_TYPE_1_OFFSET);
				/* Copy the LLC header for type 1 */
				memcpy(llc_hdr, ((uint8 *)ether_type - LLC_START_OFFSET_LEN),
						LLC_START_OFFSET_LEN);
			}

			llc_hdr[6] = (uint8)*ether_type;
			llc_hdr[7] = (uint8)((*ether_type) >> 8);

			/* 802.11 hdr + LLC SNAP header offset */
			dot11_offset = ((uint8 *)f->pbody) - PKTDATA(osh, pkt);
			insert_len = dot11_offset + LLC_HDR_LEN;

			/* AMSDU sub frame */
			if (WLPKTTAG(pkt)->flags & WLF_HWAMSDU) {
				/* Consider sub frame header and pad */
				insert_len += ETHER_HDR_LEN + HDRCONV_PAD;

				/* Calculate sub frame length */
				sflen = hton16(PKTLEN(osh, fetch_pkt) + LLC_HDR_LEN);

				/* Adjust total length */
				f->totlen -=
					(copycount - (HDRCONV_PAD + ETHER_HDR_LEN + LLC_HDR_LEN));

				pad = HDRCONV_PAD;
				offset += HDRCONV_PAD;
				wlc_rxhdr_set_pad_present(f->rxh, wlc);
			}

			/* Reserve room for WLAN header in new fetch_pkt */
			PKTPUSH(osh, fetch_pkt, insert_len);

			/* Copy 802.11 hdr and pad (if present) */
			memcpy((PKTDATA(osh, fetch_pkt) + offset),
				(PKTDATA(osh, pkt) + offset), dot11_offset);
			offset += dot11_offset;

			/* Copy sub frame header */
			if (WLPKTTAG(pkt)->flags & WLF_HWAMSDU) {
				memcpy((PKTDATA(osh, fetch_pkt) + offset),
					f->pbody, ETHER_TYPE_OFFSET);
				offset += ETHER_TYPE_OFFSET;
				memcpy((PKTDATA(osh, fetch_pkt) + offset), &sflen, ETHER_TYPE_LEN);
				offset += ETHER_TYPE_LEN;
			}

			/* Copy llc header */
			memcpy((PKTDATA(osh, fetch_pkt) + offset), llc_hdr, LLC_HDR_LEN);
		}

		/* Are we using the existing packets wrxh or borrowed rxh */
		if ((uchar *)f->wrxh <= (uchar *)PKTDATA(osh, pkt)) {
			uint32 diff = 0;

			diff = (uchar *)PKTDATA(osh, pkt) - (uchar *)f->wrxh;
			WL_INFORM(("header adjust: diff: %d, data: %p, wrxh: %p, pktheadroom %d\n",
				diff, PKTDATA(osh, pkt), f->wrxh, PKTHEADROOM(osh, pkt)));
			if (diff <= PKTHEADROOM(osh, pkt)) {
				WL_INFORM(("rxheader in the packet\n"));
				wrxh_inline = TRUE;
			}
		}

		/* Original packet will be freed so if f->wrxh is inline with PKTDATA then
		 * copy it to the new fetch_pkt.
		 */
		if (wrxh_inline) {
			int16 wrxh_offset;

			wrxh_offset = PKTDATA(osh, pkt) - (uchar *)f->wrxh;

			if (PKTHEADROOM(osh, fetch_pkt) < wrxh_offset) {

				BME_PKTFETCH_STATS_INC((WLC_BME_PKTFETCH())->stats.error);

				WL_ERROR(("wl%d: %s: No headroom in fetch_pkt to copy RxStatus;"
					"headroom[%d] wrxh_offset[%d]\n", wlc->pub->unit,
					__FUNCTION__, PKTHEADROOM(osh, fetch_pkt), wrxh_offset));
				OSL_SYS_HALT();
				/* Free newly allocated packet and return original packet */
				PKTFREE(osh, fetch_pkt, FALSE);
				ret = BCME_BUFTOOSHORT;
				goto error;
			}

			ASSERT(wrxh_offset >= WL_RXHDR_LEN(wlc->pub->corerev));
			/* Make room in new fetch_pkt for RxStatus */
			PKTPUSH(osh, pkt, wrxh_offset);
			PKTPUSH(osh, fetch_pkt, wrxh_offset);

			/* Copy RxStatus (wlc_d11rxhdr_t) to new fetch_pkt */
			memcpy(PKTDATA(osh, fetch_pkt), PKTDATA(osh, pkt), wrxh_offset);

			/* wrxh and rxh pointers: both have same address */
			f->wrxh = (wlc_d11rxhdr_t *)PKTDATA(osh, fetch_pkt);
			f->rxh = &f->wrxh->rxhdr;

			/* Restore data pointers */
			PKTPULL(osh, pkt, wrxh_offset);
			PKTPULL(osh, fetch_pkt, wrxh_offset);
		}

		/* Calculate offset of dot11_header from orig pkt and apply the same to fetch_pkt */
		offset = (((uchar *)f->h) - PKTDATA(osh, pkt)) + pad;
		f->h = (struct dot11_header *)(PKTDATA(osh, fetch_pkt) + offset);

		/* Calculate the offset of pkt body pointer from orig pkt and apply to fetch_pkt */
		offset = (((uchar *)f->pbody) - PKTDATA(osh, pkt)) + pad;
		f->pbody = (uchar *)(PKTDATA(osh, fetch_pkt) + offset);

		/* Return fetched packet */
		f->p = fetch_pkt;
	}

	/* Link AMSDU subframes to new packet */
	PKTSETNEXT(wlc->osh, fetch_pkt, PKTNEXT(osh, pkt));
	PKTSETNEXT(osh, pkt, NULL);

#if defined(STS_XFER_PHYRXS)
	ASSERT(WLPKTTAG(pkt)->phyrxs_seqid == STS_XFER_PHYRXS_SEQID_INVALID);
#else /* ! STS_XFER_PHYRXS */
	WLPKTTAG(pkt)->phystsbuf_idx = 0;
#endif /* ! STS_XFER_PHYRXS */

	/* Free original packet */
	PKTFREE(wlc->osh, pkt, FALSE);

error:
	return ret;
} /* wlc_bme_pktfetch_recvdata() */

#if defined(PKTC) || defined(PKTC_DONGLE)
/**
 * ------------------------------------------------------------------------------------------------
 *  Fetch the hostdata for a receive packet from PKTC path.
 *
 *  Return
 *	BCME_OK: On PktFetch success
 *		- pkt_p points to newly allocated packet with complete data.
 *		- Original packet is freed.
 *	bcm error: On PktFetch failure
 *		- pkt_p points to original packet.
 *		- Caller has to own the packet.
 *
 *  Caveats:
 *	In PKTC path, PktFetch for Header converted packets (RXMODE4) is not supported.
 * ------------------------------------------------------------------------------------------------
 */
int
wlc_bme_pktfetch_sendup(wlc_info_t *wlc, void **pkt_p)
{
	void *fetch_pkt;
	int host_offset = 0;
	int ret;

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));
	ASSERT(PKTISRXFRAG(wlc->osh, *pkt_p));

	/* Fill up pktfetch info */
	/* In case of RXMODE 3, host has full pkt including RxStatus (WL_HWRXOFF_AC bytes)
	 * Host buffer addresses in the lfrag points to start of this full pkt
	 * So, pktfetch host_offset needs to be set to (copycount + HW_RXOFF)
	 * so that only necessary (remaining) data will be pktfetched from Host.
	*/
	if (SPLITRX_DYN_MODE3(wlc->osh, *pkt_p)) {
		host_offset = (wlc->pub->tunables->copycount << 2) + wlc->d11rxoff;
		PKTSETFRAGUSEDLEN(wlc->osh, *pkt_p, PKTFRAGUSEDLEN(wlc->osh, *pkt_p) + host_offset);
	}

	/* PktFetch for Header converted packets (RXMODE4) in PKTC Rx path is not supported. */
	ASSERT(!SPLITRX_DYN_MODE4(wlc->osh, *pkt_p));

	/* Schedule and sync BME transfer of host packet data */
	if ((fetch_pkt = _bme_pktfetch_rx(wlc, *pkt_p, host_offset, TRUE)) != NULL) {

		PKTSETPRIO(fetch_pkt, PKTPRIO(*pkt_p));

		/* Link AMSDU subframes to new packet */
		PKTSETNEXT(wlc->osh, fetch_pkt, PKTNEXT(wlc->osh, *pkt_p));
		PKTSETNEXT(wlc->osh, *pkt_p, NULL);

#if defined(STS_XFER_PHYRXS)
		ASSERT(WLPKTTAG(*pkt_p)->phyrxs_seqid == STS_XFER_PHYRXS_SEQID_INVALID);
#else /* ! STS_XFER_PHYRXS */
		WLPKTTAG(*pkt_p)->phystsbuf_idx = 0;
#endif /* ! STS_XFER_PHYRXS */

		/* Free original packet and return fetch_pkt */
		PKTFREE(wlc->osh, *pkt_p, FALSE);
		*pkt_p = fetch_pkt;

		ret = BCME_OK;
	} else {
		/* Return the original packet to caller */
		ret = BCME_ERROR;
	}

	return ret;
} /* wlc_bme_pktfetch_sendup() */
#endif /* PKTC || PKTC_DONGLE */
#endif /* BCMSPLITRX */

#if defined(BCMPCIEDEV)
/**
 * ------------------------------------------------------------------------------------------------
 *  L2/L3 forwarding features (E.g., FTP packets in NATOE) need Full packet data for parsing.
 *  Fetch the hostdata for a receive packet
 *
 *  Return
 *	BCME_OK: On PktFetch success
 *		- pkt_p points to newly allocated packet with complete data.
 *		- Original packet is freed.
 *	bcm error: On PktFetch failure
 *		- pkt_p points to original packet.
 *		- Caller has to own the packet.
 * ------------------------------------------------------------------------------------------------
 */
int
wlc_bme_pktfetch_rx(wlc_info_t *wlc, void **pkt_p)
{
	void *fetch_pkt;
	int ret;

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));
	ASSERT(PKTISRXFRAG(wlc->osh, *pkt_p));

	/* Schedule and sync BME transfer of host packet data */
	if ((fetch_pkt = _bme_pktfetch_rx(wlc, *pkt_p, 0, TRUE)) != NULL) {

		PKTSETNEXT(wlc->osh, fetch_pkt, NULL);
		ASSERT(PKTNEXT(wlc->osh, *pkt_p) == NULL);

		/* Free original packet and return fetch_pkt */
		PKTFREE(wlc->osh, *pkt_p, FALSE);
		*pkt_p = fetch_pkt;

		ret = BCME_OK;
	} else {
		/* Return the original packet to caller */
		ret = BCME_ERROR;
	}

	return ret;
} /* wlc_bme_pktfetch_rx() */

/**
 * ------------------------------------------------------------------------------------------------
 *  Fetch the hostdata for Transmit packets (like EAPOL, TKIP)
 *  Return BCME_OK: On PktFetch success
 *
 *  Caveats:
 *	Even after fetching the host data, Dongle packet still carries Host buffer and is
 *	released after sending Tx Status in PKTFREE callback.
 * ------------------------------------------------------------------------------------------------
 */
int
wlc_bme_pktfetch_tx(wlc_info_t *wlc, void *pkt)
{
	wlc_bme_pktfetch_t *bme_pktfetch;
	osl_t	*osh;
	uint64	msg_src64, msg_dst64;
	int	bme_eng_idx;
	uint32	host_pktlen;
#if defined(BCM_DHDHDR)
	uint8	*heap_buf;
	int16	headroom;
#endif /* BCM_DHDHDR */

	osh = wlc->osh;
	bme_pktfetch = WLC_BME_PKTFETCH();

	WL_TRACE(("wl%d: %s: ENTER\n", wlc->pub->unit, __FUNCTION__));

	ASSERT(bme_pktfetch->h2d_bme_key != ~0U);
	ASSERT(PKTISTXFRAG(osh, pkt));

#if defined(BCM_DHDHDR)
	/* Dongle frag buffer is released and a large buffer from heap memory is attached to pkt. */
	{
		int16	host_offset;

		/* Allocate a buffer from heap memory. */
		if ((OSL_MEM_AVAIL() < PKTFETCH_MIN_MEM_FOR_HEAPALLOC) ||
			((heap_buf = hnd_malloc(MAXPKTDATABUFSZ)) == NULL)) {

			BME_PKTFETCH_STATS_INC((WLC_BME_PKTFETCH())->stats.error);

			WL_ERROR(("wl%d: %s: Heap buffer alloc failed; OSL_MEM_AVAIL %d\n",
				wlc->pub->unit, __FUNCTION__, OSL_MEM_AVAIL()));
			return BCME_NOMEM;
		}

		headroom = PKTFETCH_DEFAULT_HEADROOM;
		host_offset = -(DOT11_LLC_SNAP_HDR_LEN);

		host_pktlen = PKTFRAGTOTLEN(osh, pkt) - host_offset;

		/* Get Host (src) buffer address and unset Hi bit if set */
		msg_src64 = (((uint64)(PKTFRAGDATA_HI(osh, pkt, LB_FRAG1) & 0x7fffffff)) << 32) |
			(PKTFRAGDATA_LO(osh, pkt, LB_FRAG1) + host_offset);

		/* Get Dongle (dst) buffer address */
		msg_dst64 = (uint64)((uint32)heap_buf + headroom);
	}
#else /* ! BCM_DHDHDR */
	/* Allocate and fetch host pktdata to a new dongle pkt and link new pkt to original pkt */
	{
		void *fetch_pkt;

		/* Allocate a packet from dongle memory */
		if ((OSL_MEM_AVAIL() < PKTFETCH_MIN_MEM_FOR_HEAPALLOC) ||
			((fetch_pkt = PKTALLOC(osh, MAXPKTDATABUFSZ, lbuf_basic)) == NULL)) {

			BME_PKTFETCH_STATS_INC((WLC_BME_PKTFETCH())->stats.error);

			WL_ERROR(("wl%d: %s: PKTALLOC failed; OSL_MEM_AVAIL %d\n",
				wlc->pub->unit, __FUNCTION__, OSL_MEM_AVAIL()));
			return BCME_NOMEM;
		}

		/* Link fetch_pkt to orig dongle packet */
		PKTSETNEXT(osh, pkt, fetch_pkt);
		PKTSETLEN(osh, fetch_pkt, PKTFRAGTOTLEN(osh, pkt));
		PKTSETIFINDEX(osh, fetch_pkt, PKTIFINDEX(osh, pkt));

		host_pktlen = PKTFRAGTOTLEN(osh, pkt);

		/* Get Host (src) buffer address and unset Hi bit if set */
		msg_src64 = (((uint64)(PKTFRAGDATA_HI(osh, pkt, LB_FRAG1) & 0x7fffffff)) << 32) |
			PKTFRAGDATA_LO(osh, pkt, LB_FRAG1);

		/* Get Dongle (dst) buffer address */
		msg_dst64 = (uint64)(PKTDATA(osh, fetch_pkt));
	}
#endif /* ! BCM_DHDHDR */

	WL_INFORM(("%s: BME copy msg_src64[0x%x:%x] msg_dst64[0x%x:%x] msg_len[%d]\n", __FUNCTION__,
		(uint32)(msg_src64 >> 32), (uint32)msg_src64,
		(uint32)(msg_dst64 >> 32), (uint32)msg_dst64, host_pktlen));

	/* Initiate BME (DMA) copy */
	bme_eng_idx = bme_copy64(wlc->osh, bme_pktfetch->h2d_bme_key,
			msg_src64, msg_dst64, host_pktlen);

	BME_PKTFETCH_STATS_INC(bme_pktfetch->stats.tx_pkts);
	BME_PKTFETCH_STATS_INC(bme_pktfetch->stats.tot_pkts);

#if defined(BCM_DHDHDR)
	{
		uint8	*memcpy_dst;
		uint32	dngl_pktlen;

		dngl_pktlen = PKTLEN(osh, pkt);

		/* Copy the Dongle pkt data to new dongle heap buffer */
		memcpy_dst = heap_buf + (headroom - dngl_pktlen);
		memcpy(memcpy_dst, PKTDATA(osh, pkt), dngl_pktlen);

		/* Release dongle frag buffer */
		PKTBUFEARLYFREE(osh, pkt);

		/* Link new heap buffer to the packet */
		PKTSETBUF(osh, pkt, heap_buf, MAXPKTDATABUFSZ);
		PKTSETBUFALLOC(osh, pkt);
		PKTSETHEAPBUF(osh, pkt);

		PKTSETLEN(osh, pkt, (headroom + host_pktlen));
		PKTPULL(osh, pkt, (headroom - dngl_pktlen));
	}
#endif /* BCM_DHDHDR */

	/* Mark the pkt as PKTFETCHED */
	PKTSETPKTFETCHED(osh, pkt);

	/* Reset FRAG parameters but PKTFRAGPKTID() is still valid.
	 * Host buffer is released after sending Tx Status in PKTFREE callback.
	 */
	PKTSETFRAGTOTLEN(wlc->osh, pkt, 0);
	PKTSETFRAGLEN(wlc->osh, pkt, LB_FRAG1, 0);

	/* When BCM_DHDHDR is enabled, all tx packets that need to be fetched will
	 * include llc snap 8B header at start of lbuf.
	 * So we can do PKTSETFRAGTOTNUM here as well.
	 */
	PKTSETFRAGTOTNUM(wlc->osh, pkt, 0);

	/* Ensure the BME copy is completed */
	bme_sync_eng(wlc->osh, bme_eng_idx);

	return BCME_OK;
} /* wlc_bme_pktfetch_tx() */
#endif /* BCMPCIEDEV */

#endif /* BME_PKTFETCH */
