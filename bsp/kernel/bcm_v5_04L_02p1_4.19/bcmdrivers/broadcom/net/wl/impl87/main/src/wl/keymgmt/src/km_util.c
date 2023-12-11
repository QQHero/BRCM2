/*
 * Key Management Module Implementation - utilities
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
 * $Id: km_util.c 796662 2021-03-10 04:28:57Z $
 */

#include "km_pvt.h"
#include "km_hw_impl.h"
#include "km_key_pvt.h"

#ifdef BCMWAPI_WPI
#include <wlc_wapi.h>
#endif /* BCMWAPI_WPI */
#include <wlc_rx.h>

#define KM_IS_WDS(_fc) (((_fc) & (FC_TODS | FC_FROMDS)) == \
	    (FC_TODS | FC_FROMDS))

/* 802.1X LLC header, * DSAP/SSAP/CTL = AA:AA:03
 * OUI = 00:00:00
 * Ethertype = 0x888e (802.1X Port Access Entity)
 */
#define KM_DOT1X_HDR(_pbody, _off, _len) \
	(bcmp(&wlc_802_1x_hdr[(_off)], (_pbody), (_len)) == 0)

scb_t*
km_find_scb(keymgmt_t *km, wlc_bsscfg_t *bsscfg, const struct ether_addr *addr,
	bool create)
{
	scb_t *scb = NULL;
	enum wlc_bandunit first_bandunit, bandunit;
	wlc_info_t *wlc;

	wlc = km->wlc;

	if (addr == NULL || KM_ADDR_IS_BCMC(addr))
		goto done;

	first_bandunit = (bsscfg->associated) ?
		CHSPEC_BANDUNIT(bsscfg->current_bss->chanspec) :
		wlc->band->bandunit;

	FOREACH_WLC_BAND_STARTING_FROM(wlc, first_bandunit, bandunit) {
		scb = wlc_scbfindband(wlc, bsscfg, addr, bandunit);
		if (scb != NULL) {
			break;
		}
	}
	if ((scb != NULL) || !create)
		goto done;

	/* need to create it */
	scb = wlc_scblookupband(wlc, bsscfg, addr, first_bandunit);

done:
	if (create && (scb == NULL))
		KM_ERR(("wl%d.%d: %s: out of scbs\n", KM_UNIT(km),
			WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
	return scb;
}

wlc_key_t*
km_find_key(keymgmt_t *km, wlc_bsscfg_t *bsscfg, const struct ether_addr *addr,
	wlc_key_id_t key_id, wlc_key_flags_t key_flags, wlc_key_info_t *key_info)
{
	wlc_key_t *key;
	scb_t *scb;
	wlc_key_info_t tmp_ki;

	KM_DBG_ASSERT(KM_VALID(km));

	if (!key_info)
		key_info = &tmp_ki;

	if (KM_ADDR_IS_BCMC(addr)) {
		key = wlc_keymgmt_get_bss_key(km, (BSSCFG_PSTA(bsscfg) ?
			KM_DEFAULT_BSSCFG(km) : bsscfg), key_id, key_info);
		goto done;
	}

	scb = km_find_scb(km, bsscfg, addr, TRUE);
	if (scb == NULL) {
		key = km->null_key;
		wlc_key_get_info(key, key_info);
		goto done;
	}

	key = wlc_keymgmt_get_scb_key(km, scb, key_id, key_flags, key_info);

	/* if scb key index has not been allocated, allocate it now */
	if (!KM_VALID_KEY_IDX(km, key_info->key_idx)) {
		km_scb_reset(km, scb);
		key = wlc_keymgmt_get_scb_key(km, scb, key_id, key_flags, key_info);
	}

done:
	KM_DBG_ASSERT(key != NULL);
	return key;
}

void
km_sync_scb_wsec(keymgmt_t *km, scb_t *scb, wlc_key_algo_t key_algo)
{
	KM_DBG_ASSERT(KM_VALID(km));
	switch (key_algo) {
		case CRYPTO_ALGO_WEP1:
		case CRYPTO_ALGO_WEP128:
			scb->wsec = WEP_ENABLED;
			break;

		case CRYPTO_ALGO_TKIP:
			scb->wsec = TKIP_ENABLED;
			break;

		/* Enable AES for both CCM and GCM modes w/ 128 or 256 bit keys */
		case CRYPTO_ALGO_AES_CCM:
		case CRYPTO_ALGO_AES_OCB_MSDU:
		case CRYPTO_ALGO_AES_OCB_MPDU:
		case CRYPTO_ALGO_AES_CCM256:
		case CRYPTO_ALGO_AES_GCM:
		case CRYPTO_ALGO_AES_GCM256:
			scb->wsec = AES_ENABLED;
			break;

#ifdef BCMWAPI_WPI
		case CRYPTO_ALGO_SMS4:
			scb->wsec = SMS4_ENABLED;
			break;
#endif /* BCMWAPI_WPI */

		/* not valid, ignore
		 * case CRYPTO_ALGO_BIP:
		 * case CRYPTO_ALGO_BIP_CMAC256:
		 * case CRYPTO_ALGO_BIP_GMAC:
		 * case CRYPTO_ALGO_BIP_GMAC256:
		 * case CRYPTO_ALGO_PMK:
		 * case CRYPTO_ALGO_NALG:
		 */
		default:
			scb->wsec = 0;
	}
}

wlc_info_t*
wlc_keymgmt_get_wlc(keymgmt_t * km)
{
	return KM_VALID(km) ? km->wlc : NULL;
}

wlc_bsscfg_t*
wlc_keymgmt_get_bsscfg(keymgmt_t *km, wlc_key_index_t key_idx)
{
	km_pvt_key_t *km_pvt_key;

	KM_DBG_ASSERT(KM_VALID(km));

	if (!KM_VALID_KEY_IDX(km, key_idx))
		return NULL;

	km_pvt_key = &km->keys[key_idx];
	if (!KM_VALID_KEY(km_pvt_key))
		return NULL;

	return (km_pvt_key->flags & KM_FLAG_SCB_KEY) ?
		SCB_BSSCFG(km_pvt_key->u.scb) : km_pvt_key->u.bsscfg;
}

scb_t*
wlc_keymgmt_get_scb(keymgmt_t *km, wlc_key_index_t key_idx)
{
	km_pvt_key_t *km_pvt_key;

	KM_DBG_ASSERT(KM_VALID(km));

	if (!KM_VALID_KEY_IDX(km, key_idx))
		return NULL;

	km_pvt_key = &km->keys[key_idx];
	if (!KM_VALID_KEY(km_pvt_key))
		return NULL;

	return (km_pvt_key->flags & KM_FLAG_SCB_KEY) ?
		km_pvt_key->u.scb : NULL;
}

wlc_key_t*
wlc_keymgmt_get_key_by_addr(keymgmt_t *km,
	wlc_bsscfg_t *bsscfg, const struct ether_addr *addr,
	wlc_key_flags_t key_flags, wlc_key_info_t *key_info)
{
	wlc_key_id_t key_id;
	wlc_key_t *key;

	KM_DBG_ASSERT(KM_VALID(km));

	key = km->null_key;
	if (bsscfg == NULL || addr == NULL)
		goto done;

	if (KM_ADDR_IS_BCMC(addr)) {
		key_id = wlc_keymgmt_get_bss_tx_key_id(km, bsscfg,
			(key_flags & WLC_KEY_FLAG_MGMT_GROUP));
	} else {
		key_id = WLC_KEY_ID_PAIRWISE;
	}

	if (key_id != WLC_KEY_ID_INVALID)
		key = km_find_key(km, bsscfg, addr, key_id, key_flags, NULL);

done:
	KM_DBG_ASSERT(key != NULL);
	wlc_key_get_info(key, key_info);
	return key;
}

wlc_key_index_t
wlc_keymgmt_get_key_index(keymgmt_t *km, wlc_key_t *key)
{
	wlc_key_info_t key_info;

	KM_DBG_ASSERT(KM_VALID(km));

	wlc_key_get_info(key, &key_info);
	KM_DBG_ASSERT(key_info.key_idx == WLC_KEY_INDEX_INVALID ||
		KM_VALID_KEY_IDX(km, key_info.key_idx));
	return key_info.key_idx;
}

wlc_key_t*
wlc_keymgmt_get_key(keymgmt_t *km, wlc_key_index_t key_idx,
	wlc_key_info_t *key_info)
{
	km_pvt_key_t *km_pvt_key;
	wlc_key_t *key;

	KM_DBG_ASSERT(KM_VALID(km));

	key = km->null_key;
	if (KM_VALID_KEY_IDX(km, key_idx)) {
		km_pvt_key = &km->keys[key_idx];
		if (KM_VALID_KEY(km_pvt_key)) {
			key = km_pvt_key->key;
		}
	}

	wlc_key_get_info(key, key_info);
	return key;
}

bool
km_allow_unencrypted(keymgmt_t *km, const wlc_key_info_t *key_info,
	scb_t *scb, const struct dot11_header *hdr, uint16 qc,
	uint8 *body, int body_len, void *pkt)
{
	bool allow = FALSE;
	wlc_bsscfg_t *bsscfg;

	BCM_REFERENCE(km);

	KM_DBG_ASSERT(key_info != NULL && scb != NULL &&
		hdr != NULL && body != NULL);

	bsscfg = SCB_BSSCFG(scb);

	do {
		uint16 seq;
		bool frag0;
		uint16 fc;
		uint16 fk;
		uint32 wpa_auth;
		bool more_frag;
		uint16 eth_type = 0;
		bool is_llc = TRUE;
		uint16 llc_off = 0;

		if (!bsscfg->wsec || !bsscfg->wsec_restrict) {
			allow = TRUE;
			break;
		}

		seq = ltoh16(hdr->seq);
		frag0 = ((seq & FRAGNUM_MASK) == 0);
		seq &= ~FRAGNUM_MASK;

		fc = ltoh16(hdr->fc);
		fk = (fc & FC_KIND_MASK);
		BCM_REFERENCE(fk);

		more_frag = (fc & FC_MOREFRAG) != 0;

		/* multicast MSDUs and MMPDUs are never fragmented */
		if (ETHER_ISMULTI(&hdr->a1)) {
			if (!frag0 || more_frag) {
				break; /* more fragments or non-initial fragment */
			}
		}

		if (FC_TYPE(fc) == FC_TYPE_MNG) {
#ifdef MFP
			/* deauth and disassoc always need protection when MFP is enabled.
			 * mfp module depends on these being disallowed as part of keymgmt checks
			 * for its operation. Action frames are handled by mfp module w/ robust
			 * category checks
			 */
			if (KM_SCB_MFP(scb) && KM_BSSCFG_ASSOCIATED(bsscfg)) {
				if (fk == FC_DEAUTH || fk == FC_DISASSOC)
					break;
			}
#endif
			allow = TRUE;
			break;
		}

		if (FC_TYPE(fc) != FC_TYPE_DATA) {
			KM_DBG_ASSERT(FC_TYPE(fc) == FC_TYPE_DATA);
			break;
		}

		if (KM_IS_WDS(fc)) {
			wpa_auth = bsscfg->WPA_auth;
		} else {
			wpa_auth = scb->WPA_auth;
		}

		if (wpa_auth == WPA_AUTH_DISABLED) {
			allow = TRUE;
			break;
		}

		/* note re. hdr conversion: without it, match entire snap hdr
		 * and the following ether type, otherwise match just ether type
		 * Although hdr conversion may be supported by hw, certain frames such
		 * as EAPOL, come here w/o hdr conversion because of pktfetch into dongle
		 * after the frame ether type is determined, and that certain other frames
		 * that may need to be filtered come here with hdr conversion.
		 */
		if (frag0) {
			bool hconv = PKTISHDRCONVTD(KM_OSH(km), pkt);
			bool is_amsdu = (qc & QOS_AMSDU_MASK) != 0;

			/* non amsdu hdr converted frames have a pad | DA | SA,
			 * but amsdu has no pad even if hdr converted.
			 */
			if (!is_amsdu) {
				llc_off += hconv ? (HDRCONV_PAD + ETHER_HDR_LEN) : 0;
			} else {
				llc_off += ETHER_HDR_LEN;
			}

			/* eth_type not used for llc frames - incl. non amsdu w/o hdr conversion */
			if (hconv || is_amsdu) {
				if (body_len < llc_off) {
					break; /* missing eth type or llc len */
				}

				eth_type = ntoh16_ua(body + llc_off - ETHER_TYPE_LEN);
				is_llc = (eth_type < ETHER_TYPE_MIN);
			}
		}
		else {
			/* check if scb frag state exists for this non-initial frag */
			uint16 scb_seq;
			scb_seq = scb->seqctl[QOS_PRIO(qc)] & ~FRAGNUM_MASK;

			if (seq != scb_seq) {
				break; /* disallow */
			}
		}

		/* block clear traffic if key exists, otherwise allow specific frames */
#ifdef BCMWAPI_WPI
		/* For WAPI, allow only WAI if there is no key */
		if (IS_WAPI_AUTH(wpa_auth)) {
			if (key_info->algo != CRYPTO_ALGO_OFF) {
				break;
			}

			/* no key, allow first frag WAI or frag w/ current WAI state only */
			if (frag0) {
				if (is_llc) {
					if (body_len >= (llc_off + DOT11_LLC_SNAP_HDR_LEN)) {
						allow = WAPI_WAI_SNAP(body + llc_off);
					}
				} else {
					allow = (eth_type == WAPI_WAI_ETHER_TYPE);
				}
			} else {
				allow = ((scb->flags2 & SCB2_WAIHDR) != 0);
			}
			break;
		}
#endif /* BCMWAPI_WPI */

		/* WPA */
		if ((key_info->algo != CRYPTO_ALGO_OFF)) {
			/* allow generic IBSS only when there is a key */
			if (!bsscfg->BSS && (bsscfg->type == BSSCFG_TYPE_GENERIC)) {
				wlc_bsscfg_type_t bss_type;

				wlc_bsscfg_type_get(bsscfg->wlc, bsscfg, &bss_type);
				allow = (bss_type.subtype == BSSCFG_GENERIC_IBSS);
			}
		} else { /* no key */
			/* allow first frag 1x or frag w/ current 1x state only */
			if (frag0) {
				if (is_llc) {
					if (body_len >= (llc_off + DOT11_LLC_SNAP_HDR_LEN)) {
						allow = KM_DOT1X_HDR(body + llc_off,
							0, DOT11_LLC_SNAP_HDR_LEN);
					}
				} else {
					allow = (eth_type == ETHER_TYPE_802_1X);
				}
			} else {
				allow =  ((scb->flags & SCB_8021XHDR) != 0);
			}
		}
	} while (0);

	return allow;
}

void km_null_key_deauth(keymgmt_t *km, scb_t *scb, void *pkt)
{
#if defined(AP)
	wlc_bsscfg_t *bsscfg;
	struct dot11_header *hdr;
	wlc_info_t *wlc;

	/* scb can be null for unexpected encrypted frames */
	if (scb == NULL)
		return;

	wlc = km->wlc;
	bsscfg = SCB_BSSCFG(scb);
	KM_DBG_ASSERT(bsscfg != NULL);

	hdr = (struct dot11_header *)PKTDATA(KM_OSH(km), pkt);

	/* 802.11i D5.0 8.4.10.1 Illegal data transfer */
	if (!ETHER_ISMULTI(&hdr->a1) && BSSCFG_AP(bsscfg) && SCB_WDS(scb) &&
		SCB_AUTHORIZED(scb) && (bsscfg->WPA_auth != WPA_AUTH_DISABLED)) {
		/* pairwise key is out of sync with peer, send deauth */
		if (!(scb->flags & SCB_DEAUTH)) {
			/* Use the cur_etheraddr of the BSSCFG that this WDS
			 * interface is tied to as our BSSID.  We can't use the
			 * BSSCFG's BSSID because the BSSCFG may not be "up" (yet).
			 */
			KM_ASSOC(("wl%d.%d: %s: send deauth to "MACF" with reason %d\n",
				wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
				ETHER_TO_MACF(scb->ea), DOT11_RC_AUTH_INVAL));
			wlc_senddeauth(wlc, bsscfg, scb, &scb->ea,
				&bsscfg->cur_etheraddr, &bsscfg->cur_etheraddr,
				DOT11_RC_AUTH_INVAL, TRUE);
			wlc_scb_disassoc_cleanup(wlc, scb);
			wlc_scb_clearstatebit(wlc, scb, AUTHORIZED);
			wlc_deauth_complete(wlc, bsscfg, WLC_E_STATUS_SUCCESS, &scb->ea,
				DOT11_RC_AUTH_INVAL, 0);
			scb->flags |= SCB_DEAUTH;
		}
	}
#endif /* AP */
}

wlc_key_hw_index_t
km_get_hw_idx(keymgmt_t *km, wlc_key_index_t key_idx)
{
	wlc_key_hw_index_t hw_idx = WLC_KEY_INDEX_INVALID;

	KM_DBG_ASSERT(KM_VALID(km));
	if (KM_VALID_KEY_IDX(km, key_idx)) {
		km_pvt_key_t *km_pvt_key;
		km_pvt_key = &km->keys[key_idx];
		if (KM_VALID_KEY(km_pvt_key))
			hw_idx = wlc_key_get_hw_idx(km_pvt_key->key);
	}

	return hw_idx;
}

/* public interface */
int BCMFASTPATH
wlc_keymgmt_recvdata(keymgmt_t *km, wlc_frminfo_t *f)
{
	int err;
	wlc_bsscfg_t *bsscfg = NULL;
	scb_t *scb = NULL;
	wlc_key_info_t *key_info;
	wlc_key_t *key;
	wlc_key_id_t key_id;

	KM_DBG_ASSERT(KM_VALID(km));
	KM_ASSERT(f != NULL && f->p != NULL);

	key_info = &f->key_info;

#if defined(PKTC) || defined(PKTC_DONGLE)
	/* fast path lookup */
	{
		wlc_key_hw_index_t hw_idx;
		uint16 RxStatus1 = D11RXHDR_ACCESS_VAL(f->rxh,
			KM_COREREV(km), RxStatus1);

		hw_idx = KM_RXS_SECKINDX(km, (RxStatus1 & KM_RXS_SECKINDX_MASK(km))
						>> RXS_SECKINDX_SHIFT);
		if ((RxStatus1 & RXS_DECATMPT) &&
			(hw_idx == km->key_cache->hw_idx)) {
			key = km->key_cache->key;
			*key_info = *km->key_cache->key_info;
			goto have_key;
		}
	}
#endif /* PKTC || PKTC_DONGLE */

	scb = WLPKTTAGSCBGET(f->p);
	KM_ASSERT(scb != NULL);

	bsscfg = SCB_BSSCFG(scb);
	KM_DBG_ASSERT(bsscfg != NULL);

	if (!f->rx_wep) {
		key_id = WLC_KEY_ID_PAIRWISE;
	} else {
		key_id =  KM_PKT_KEY_ID(f->pbody);
#ifdef BCMWAPI_WPI
		if (scb->wsec & SMS4_ENABLED)
			key_id = KM_PKT_WAPI_KEY_ID(f->pbody);
#endif
	}

	if (!f->rx_wep) {
		key = wlc_keymgmt_get_scb_key(km, scb, key_id, WLC_KEY_FLAG_NONE, key_info);
	} else if (!f->ismulti) { /* pairwise key if available bss key */
		key = wlc_keymgmt_get_scb_key(km, scb, key_id, WLC_KEY_FLAG_NONE, key_info);
		if (key_info->algo == CRYPTO_ALGO_OFF)
			key = wlc_keymgmt_get_bss_key(km, bsscfg, key_id, key_info);
	} else {
#if defined(IBSS_PEER_GROUP_KEY)
		if (KM_IBSS_PGK_ENABLED(km) && BSSCFG_IBSS(bsscfg) &&
			(bsscfg->WPA_auth != WPA_AUTH_DISABLED) &&
			(bsscfg->WPA_auth != WPA_AUTH_NONE)) {
			key = wlc_keymgmt_get_scb_key(km, scb, key_id,
				WLC_KEY_FLAG_IBSS_PEER_GROUP, key_info);
		} else
#endif /* defined(IBSS_PEER_GROUP_KEY) */
		{
			key = wlc_keymgmt_get_bss_key(km, bsscfg, key_id, key_info);
		}
	}

#if defined(PKTC) || defined(PKTC_DONGLE)
have_key:
#endif /* PKTC || PKTC_DONGLE */

	f->key = key;

	KM_LOG(("wl%d.%d: %s: key_lookup: "
		"key for %scast frame key id %d, key idx %d, algo %d\n",
		KM_UNIT(km), WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
		(f->ismulti ? "multi" : "uni"), key_info->key_id,
		key_info->key_idx, key_info->algo));

	/* receive mpdu; if s/w decryption is required it will handled
	 * by wlc_key_rx_mpdu. Any required mic checks are deferred to sdu processing.
	 */
	err = wlc_key_rx_mpdu(key, f->p, f->rxh);
	if (err != BCME_OK) {
		KM_LOG(("wl%d.%d: %s: error %d on receive\n",
			KM_UNIT(km), WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, err));
		goto done;
	}

#if defined(PKTC) || defined(PKTC_DONGLE)
	/* update key cache */
	if (!f->ismulti && (km->key_cache->key != key) &&
			!WLC_KEY_SMS4_PREV_KEY(key_info)) {
		km_key_update_key_cache(key, km->key_cache);
	}
#endif /* PKTC || PKTC_DONGLE */

	/* adjust frame body and length unless not wep or algo off */
	if (!f->rx_wep || (key_info->algo == CRYPTO_ALGO_OFF))
		goto done;

	/* rx stripped icv, update frame info */
	f->pbody += key_info->iv_len;
	f->body_len -= (key_info->iv_len);
	f->totlen -= (key_info->iv_len);
	if (PKTISHDRCONVTD(KM_OSH(km), f->p) &&
		((f->isamsdu) || D11REV_GE(KEY_PUB(key)->corerev, 130))) {
		/* XXX SWWLAN-187559 WAR
		 * [43684] Header-converted and AMSDU there is no ICV
		 * CRBCAD11MAC-4623
		 * [corerev>=130] Header-converted then MIC/ICV is trimmed
		 */
	} else {
		/* XXX Dont see MIC bytes decremented any where.
		 * Probably due to AMSDU disabled in TKIP mode.
		 * But will be a problem if a thridparty transmits AMSDU in TKIP mode
		 */
		f->body_len -= key_info->icv_len;
		f->totlen -= key_info->icv_len;
	}

	f->len = PKTLEN(KM_OSH(km), f->p);

done:
	return err;
}

bool
km_needs_hw_key(keymgmt_t *km, km_pvt_key_t *km_pvt_key, wlc_key_info_t *key_info)
{
	bool needs = FALSE;

	KM_DBG_ASSERT(KM_VALID(km));

	do {
		if (km->flags & KM_FLAG_WLC_DOWN)
			break;

		if (!KM_VALID_KEY(km_pvt_key) || !key_info) /* invalid keys */
			break;

		if (WLC_KEY_IN_HW(key_info)) /* already has it */
			break;

		if (key_info->algo == CRYPTO_ALGO_NONE)	/* no real crypto */
			break;

		if (km_algo_is_swonly(km, key_info->algo)) /* h/w support disabled */
			break;

		if (key_info->hw_algo == WSEC_ALGO_OFF)	/* no h/w support */
			break;

		if (km_pvt_key->flags & KM_FLAG_SWONLY_KEY) /* s/w only key */
			break;

		if (WLC_KEY_IS_LINUX_CRYPTO(key_info)) /* external crypto */
			break;

		/* note: group keys on AP used only for tx, no rx is expected. */
		if (!WLC_KEY_RX_ALLOWED(key_info) && !WLC_KEY_TX_ALLOWED(key_info))
			break;

#ifdef MFP
		if (WLC_KEY_IS_MGMT_GROUP(key_info))
			break;
#endif

		if (km_pvt_key->flags & KM_FLAG_BSS_KEY) {
			wlc_bsscfg_t *bsscfg = km_pvt_key->u.bsscfg;

			if (!KM_BSSCFG_UP(bsscfg))
				break;

			if (BSSCFG_PSTA(bsscfg)) /* non-primary psta group key */
				break;

			if (BSSCFG_STA(bsscfg)) {
				/* default bss tkip group keys - no space for phase1.
				 * non-default bss tkip group keys need ucode support
				 */
				if (key_info->algo == CRYPTO_ALGO_TKIP)
					break;
				if (!WLC_KEY_IS_DEFAULT_BSS(key_info)) {
					/* non sta group keys in non-default bss */
					if (!WLC_KEY_ID_IS_STA_GROUP(key_info->key_id))
						break;
				}
			}
			/* AP group keys need hw keys, for tx */
		}

#ifdef BCMWAPI_WPI
		/* no h/w support for prev keys */
		if (km_pvt_key->flags & KM_FLAG_SCB_KEY) {
			km_scb_t *scb_km = KM_SCB(km, km_pvt_key->u.scb);
			if (key_info->key_idx == scb_km->prev_key_idx)
				break;
		}
#endif

		/* TKIP: use SW keys for BCMC traffic */
		if (KM_HW_COREREV_GE128(km) && (key_info->algo == CRYPTO_ALGO_TKIP) &&
			(km_pvt_key->flags & KM_FLAG_BSS_KEY))
			break;

		/* IBSS group keys are tx only - h/w support is needed */

		needs = TRUE;
	} while (0);

	KM_LOG(("wl%d: %s: key with key idx %d - needs hw key: %d\n",
		KM_UNIT(km), __FUNCTION__, (key_info != NULL ? key_info->key_idx :
		WLC_KEY_INDEX_INVALID), needs));

	return needs;
}

void
km_init_pvt_key(keymgmt_t *km, km_pvt_key_t *km_pvt_key, wlc_key_algo_t algo,
	wlc_key_t *key, km_flags_t flags, wlc_bsscfg_t *bsscfg, scb_t *scb)
{
	KM_DBG_ASSERT(key != NULL);

	/* mark the key as valid and allocated */
	flags |= KM_FLAG_VALID_KEY | KM_FLAG_IDX_ALLOC;

	km_pvt_key->key =  key;
	km_pvt_key->flags =  flags;
	km_pvt_key->key_algo = algo;

	if (flags & KM_FLAG_BSS_KEY) {
		KM_DBG_ASSERT(bsscfg != NULL);
		km_pvt_key->u.bsscfg = bsscfg;
	} else {
		KM_DBG_ASSERT(scb != NULL);
		km_pvt_key->u.scb = scb;
	}

	km_notify(km, WLC_KEYMGMT_NOTIF_KEY_UPDATE,
        NULL /* bsscfg */, NULL /* scb */, key, NULL /* pkt */);
}

#if defined(BCMDBG)
void
km_get_hw_idx_key_info(keymgmt_t *km, wlc_key_hw_index_t hw_idx,
    wlc_key_info_t *key_info)
{
	wlc_key_t *key;
	int i;

	KM_DBG_ASSERT(KM_VALID(km));
	KM_DBG_ASSERT(key_info != NULL);

	key = km->null_key;
	for (i = 0; i < km->max_keys; ++i) {
		if (hw_idx == km_get_hw_idx(km, (wlc_key_index_t)i)) {
			key = km->keys[i].key;
			break;
		}
	}

	wlc_key_get_info(key, key_info);
}
#endif

/* XXX when processing a rx'd unicast ucode tries a pairwise key match first
 * based on the frame's TA and then falls back to a default key indicated by
 * the key index field in IV field of the frame if no pairwise key match is
 * found. For multicast/broadcast frames the ucode uses the key index field
 * in the frames' IV anyway to find the key entry.
 */
/* determine if default keys are valid for rx unicast */
bool
km_rxucdefkeys(keymgmt_t *km)
{
	bool defkeys = FALSE;
	wlc_info_t *wlc;
	wlc_bsscfg_t *bsscfg;

	KM_DBG_ASSERT(KM_VALID(km));

	wlc = km->wlc;

	BCM_REFERENCE(wlc);

	bsscfg = KM_DEFAULT_BSSCFG(km);
	KM_DBG_ASSERT(bsscfg != NULL);

	/* WPA must have uc keys except ibss/wpa none - def keys not valid for rx */
	if (bsscfg->WPA_auth != WPA_AUTH_DISABLED) {
		if (!bsscfg->BSS && (bsscfg->WPA_auth == WPA_AUTH_NONE))
			defkeys = TRUE;
		goto done;
	}

	if (AP_ENAB(wlc->pub) && bsscfg->eap_restrict)
		goto done;

	/* sw keys not in hw are present */
	if (km->stats.num_sw_keys != km->stats.num_hw_keys)
		goto done;

#ifdef BCMWAPI_WPI
	 if (AP_ENAB(wlc->pub) && WAPI_WAI_RESTRICT(wlc, bsscfg))
		goto done;
#endif /* BCMWAPI_WPI */

	/* multi SSID */
	if ((APSTA_ENAB(wlc->pub) || BSSCFG_AP(bsscfg)) && (km->stats.num_bss_up > 1))
		goto done;

	/* one or more default bss keys are wep */
	if (km->stats.num_def_bss_wep != 0)
		defkeys = TRUE;
done:
	KM_LOG(("wl%d: %s: defkeys %d\n", KM_UNIT(km), __FUNCTION__, defkeys));
	return defkeys;
}

/* Update AMT info DEFKEY to all the amt entries using default key
 */
void
km_amt_defkey_clearall(keymgmt_t *km)
{
	km_amt_idx_t amt_idx;
	uint16 val;

	/* update with clearing amt entries with default keys with attribute AMT_ATTR_DEFKEY */
	for (amt_idx = 0; km_hw_amt_idx_valid(km->hw, amt_idx); ++amt_idx) {
		val = wlc_read_amtinfo_by_idx(km->wlc, amt_idx);

		val &= ~NBITVAL(C_ADDR_DEFKEY_NBIT);
		wlc_write_amtinfo_by_idx(km->wlc, amt_idx, val);
	}
}

/* Update AMT info DEFKEY to all the amt entries using default key
 */
void
km_amt_defkey_update(keymgmt_t *km, km_amt_idx_t amt_idx)
{
	uint16 val = 0;
	bool mhf_defkeys = km_rxucdefkeys(km);

	/* update amt entry on whether default key is valid */
	if (km_hw_amt_idx_valid(km->hw, amt_idx)) {
		val = wlc_read_amtinfo_by_idx(km->wlc, amt_idx);
		if (mhf_defkeys) {
			val |= NBITVAL(C_ADDR_DEFKEY_NBIT);
		} else {
			val &= ~NBITVAL(C_ADDR_DEFKEY_NBIT);
		}
		wlc_write_amtinfo_by_idx(km->wlc, amt_idx, val);
	}
	KM_LOG(("wl%d: %s: amt_idx %d amt_info 0x%02x\n", KM_UNIT(km), __FUNCTION__,
		amt_idx, val));
}

bool
km_is_replay(keymgmt_t *km, wlc_key_info_t *key_info, int ins,
	uint8 *key_seq, uint8 *rx_seq, size_t seq_len)
{
	bool replay = FALSE;
	BCM_REFERENCE(ins);

	KM_DBG_ASSERT(KM_VALID(km) && key_info != NULL);

	if (WLC_KEY_CHECK_REPLAY(key_info)) {
#ifdef BRCMAPIVTW
		KM_DBG_ASSERT(km->ivtw != NULL);
		if (WLC_KEY_USE_IVTW(key_info))
			replay = km_ivtw_is_replay(km->ivtw, key_info,
				ins, key_seq, rx_seq, seq_len);
		else
#endif /* BRCMAPIVTW */
			replay = !km_key_seq_less(key_seq, rx_seq, seq_len);
	}

#ifdef GTK_RESET
	/* one shot deal to ignore replay error after gtk is reset */
	if (WLC_KEY_IS_GROUP(key_info) && (key_info->flags & WLC_KEY_FLAG_GTK_RESET)) {
		replay = FALSE;
		key_info->flags &= ~WLC_KEY_FLAG_GTK_RESET;
	}
#endif /* GTK_RESET */

	return replay;
}

#ifdef BRCMAPIVTW
void
km_update_ivtw(keymgmt_t *km, wlc_key_info_t *key_info, int ins,
	uint8 *rx_seq, size_t seq_len, bool chained)
{
	KM_DBG_ASSERT(KM_VALID(km) && key_info != NULL);
	KM_DBG_ASSERT(WLC_KEY_USE_IVTW(key_info));
	KM_DBG_ASSERT(km->ivtw != NULL);
	km_ivtw_update(km->ivtw, key_info, ins, rx_seq, seq_len, chained);
}

int
km_set_ivtw(keymgmt_t *km, wlc_key_info_t *key_info, int ins,
	const uint8 *rx_seq, size_t seq_len)
{
	KM_DBG_ASSERT(KM_VALID(km) && key_info != NULL);
	KM_DBG_ASSERT(WLC_KEY_USE_IVTW(key_info));
	KM_DBG_ASSERT(km->ivtw != NULL);
	return (km_ivtw_set(km->ivtw, key_info, ins, rx_seq, seq_len));
}
#endif /* BRCMAPIVTW */

size_t
km_get_max_keys(keymgmt_t *km)
{
	KM_DBG_ASSERT(KM_VALID(km));
	return km->max_keys;
}

wlc_key_algo_t
wlc_keymgmt_hw_algo_to_algo(keymgmt_t *km, wlc_key_hw_algo_t algo)
{
	KM_DBG_ASSERT(KM_VALID(km));
	return km_hw_hw_algo_to_algo(km->hw, algo);
}

wlc_key_hw_algo_t
wlc_keymgmt_algo_to_hw_algo(keymgmt_t *km, wlc_key_algo_t algo)
{
	KM_DBG_ASSERT(KM_VALID(km));
	return km_hw_algo_to_hw_algo(km->hw, algo);
}

bool
km_algo_is_supported(wlc_keymgmt_t *km, wlc_key_algo_t algo)
{
	KM_DBG_ASSERT(KM_VALID(km));
	KM_DBG_ASSERT(algo < KM_SIZE_BITS(km_algo_mask_t));
	return !(km->algo_unsup & (1 << algo));
}

bool
km_algo_is_swonly(wlc_keymgmt_t *km, wlc_key_algo_t algo)
{
	KM_DBG_ASSERT(KM_VALID(km));
	KM_DBG_ASSERT(algo < KM_SIZE_BITS(km_algo_mask_t));
	return (km->algo_swonly & (1 << algo));
}

bool
km_wsec_allows_algo(keymgmt_t *km, uint32 wsec, wlc_key_algo_t algo)
{
	bool allows = FALSE;

	(void)km;
	switch (algo) {
		case CRYPTO_ALGO_OFF:
			allows = TRUE;
			break;

		case CRYPTO_ALGO_WEP1:
		case CRYPTO_ALGO_WEP128:
			if (wsec & WEP_ENABLED)
				allows = TRUE;
			break;

		case CRYPTO_ALGO_TKIP:
			if (wsec & TKIP_ENABLED)
				allows = TRUE;
			break;

		case CRYPTO_ALGO_AES_CCM:
		case CRYPTO_ALGO_AES_CCM256:
		case CRYPTO_ALGO_AES_OCB_MSDU:
		case CRYPTO_ALGO_AES_OCB_MPDU:
		case CRYPTO_ALGO_AES_GCM:
		case CRYPTO_ALGO_AES_GCM256:
#ifdef MFP
		case CRYPTO_ALGO_BIP:
		case CRYPTO_ALGO_BIP_CMAC256:
		case CRYPTO_ALGO_BIP_GMAC:
		case CRYPTO_ALGO_BIP_GMAC256:
#endif /* MFP */
			if (wsec & AES_ENABLED)
				allows = TRUE;
			break;

#ifdef BCMWAPI_WPI
		case CRYPTO_ALGO_SMS4:
			if (wsec & SMS4_ENABLED)
				allows = TRUE;
			break;
#endif /* BCMWAPI_WPI */

		/* not handled by wsec setting
		 * case CRYPTO_ALGO_PMK:
		 * case CRYPTO_ALGO_NALG:
		*/
		default:
			break;
	}

	return allows;
}

#if defined(BCMDBG) || defined(WLMSG_WSEC)

#define CASE(x) case WLC_KEYMGMT_NOTIF_##x: return #x
const char*
wlc_keymgmt_notif_name(wlc_keymgmt_notif_t notif)
{
	switch (notif) {
	CASE(NONE);
	CASE(WLC_UP);
	CASE(WLC_DOWN);
	CASE(BSS_UP);
	CASE(BSS_DOWN);
	CASE(BSS_CREATE);
	CASE(BSS_DESTROY);
	CASE(BSS_WSEC_CHANGED);
	CASE(SCB_CREATE);
	CASE(SCB_DESTROY);
	CASE(KEY_UPDATE);
	CASE(KEY_DELETE);
	CASE(M1_RX);
	CASE(M4_TX);
	CASE(WOWL);
	CASE(SCB_BSSCFG_CHANGED);
	CASE(DECODE_ERROR);
	CASE(DECRYPT_ERROR);
	CASE(MSDU_MIC_ERROR);
	CASE(TKIP_CM_REPORTED);
	CASE(OFFLOAD);
	CASE(BSSID_UPDATE);
	CASE(WOWL_MICERR);
	CASE(NEED_PKTFETCH);
	default:
		break;
	}
	return "unknown";
}
#undef CASE

const char*
wlc_keymgmt_get_algo_name(keymgmt_t *km, wlc_key_algo_t algo)
{
	const char *name = "unknown";

	(void)km;
	switch (algo) {
		case CRYPTO_ALGO_OFF: name = "off"; break;
		case CRYPTO_ALGO_WEP1: name = "wep1"; break;
		case CRYPTO_ALGO_WEP128: name = "wep128"; break;
		case CRYPTO_ALGO_TKIP: name = "tkip"; break;
		case CRYPTO_ALGO_AES_CCM: name = "aes-ccm"; break;
		case CRYPTO_ALGO_AES_CCM256: name = "aes-ccm-256"; break;
		case CRYPTO_ALGO_AES_GCM: name = "aes-gcm"; break;
		case CRYPTO_ALGO_AES_GCM256: name = "aes-gcm-256"; break;
		case CRYPTO_ALGO_AES_OCB_MSDU: name = "aes-ocb-msdu"; break;
		case CRYPTO_ALGO_AES_OCB_MPDU: name = "aes-ocb-mpdu"; break;
#ifdef BCMWAPI_WPI
		case CRYPTO_ALGO_SMS4: name = "sms4"; break;
#endif /* BCMWAPI_WPI */
		case CRYPTO_ALGO_BIP: name = "bip"; break;
		case CRYPTO_ALGO_BIP_CMAC256: name = "bip-cmac-256"; break;
		case CRYPTO_ALGO_BIP_GMAC: name = "bip-gmac"; break;
		case CRYPTO_ALGO_BIP_GMAC256: name = "bip-gmac-256"; break;
		case CRYPTO_ALGO_PMK: name = "pmk"; break;
		case CRYPTO_ALGO_NALG: name = "nalg"; break;
		default:
			break;
	}
	return name;
}

const char*
wlc_keymgmt_get_hw_algo_name(keymgmt_t *km, wlc_key_hw_algo_t hw_algo, int mode)
{
	const char *name = "unknown";
	bool rev40plus;

	KM_DBG_ASSERT(KM_VALID(km));
	rev40plus = KM_COREREV_GE40(km);

	if (rev40plus) {
		switch (hw_algo) {
		case WSEC_ALGO_OFF: name = "off"; break;
		case WSEC_ALGO_WEP1: name = "wep1"; break;
		case WSEC_ALGO_TKIP: name = "tkip"; break;

		case WSEC_ALGO_WEP128: name = "wep128"; break;
		case WSEC_ALGO_AES_LEGACY: name = "aes-legacy"; break;
		case WSEC_ALGO_AES:
			switch (mode) {
			case AES_MODE_NONE: name = "aes-*"; break;
			case AES_MODE_CCM: name = "aes-ccm"; break;
			case AES_MODE_OCB_MSDU: name = "aes-ocb-msdu"; break;
			case AES_MODE_OCB_MPDU: name = "aes-ocb-mpdu"; break;
			case AES_MODE_CMAC: name = "aes-cmac"; break;
			case AES_MODE_GCM: name = "aes-gcm"; break;
			case AES_MODE_GMAC: name = "aes-gmac"; break;
			default: break;
			}
			break;

		case WSEC_ALGO_SMS4_DFT_2005_09_07: /* fall through */
		case WSEC_ALGO_SMS4: name = "sms4"; break;
		case WSEC_ALGO_NALG: name = "nalg"; break;
		case WSEC_ALGO_GCM256: name = "aes-gcm256"; break;
		default: break;
		}
	} else {
		switch (hw_algo) {
		case WSEC_ALGO_OFF: name = "off"; break;
		case WSEC_ALGO_WEP1: name = "wep1"; break;
		case WSEC_ALGO_TKIP: name = "tkip"; break;
		case D11_PRE40_WSEC_ALGO_AES:
			switch (mode) {
			case AES_MODE_NONE: name = "aes-*"; break;
			case AES_MODE_CCM: name = "aes-ccm"; break;
			case AES_MODE_OCB_MSDU: name = "aes-ocb-msdu"; break;
			case AES_MODE_OCB_MPDU: name = "aes-ocb-mpdu"; break;
			case AES_MODE_CMAC: name = "aes-cmac"; break;
			case AES_MODE_GCM: name = "aes-gcm"; break;
			case AES_MODE_GMAC: name = "aes-gmac"; break;
			default: break;
			}
			break;
		case D11_PRE40_WSEC_ALGO_WEP128: name = "wep128"; break;
		case D11_PRE40_WSEC_ALGO_AES_LEGACY: name = "aes-legacy"; break;
		case D11_PRE40_WSEC_ALGO_SMS4: name = "sms4"; break;
		case D11_PRE40_WSEC_ALGO_NALG: name = "nalg"; break;
		default: break;
		}
	}
	return name;
}
#endif

#ifdef BCMDBG
static unsigned
__h2i(int c)
{
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= '0' && c <= '9')
		return c - '0';
	else
		return 0;
}

uint8
km_hex2int(uchar lo, uchar hi)
{
	return (uint8)(__h2i(hi) << 4 | __h2i(lo));
}
#endif /* BCMDBG */

void
wlc_keymgmt_amt_reserve(wlc_keymgmt_t *km, uint8  amt_idx, size_t count, bool reserve)
{
	km_hw_amt_reserve(km->hw, amt_idx, count, reserve);
}

/* to allocate dummy key entry - used in stamon */
int
wlc_keymgmt_alloc_amt(wlc_keymgmt_t *km)
{
	km_amt_idx_t  amt_idx;

	if ((amt_idx = km_hw_amt_find_and_resrv(km->hw)) != KM_HW_AMT_IDX_INVALID) {
		return amt_idx;
	}

	return BCME_NORESOURCE;
}

void
wlc_keymgmt_free_amt(wlc_keymgmt_t *km, int *amt_idx)
{
	KM_ASSERT(KM_VALID(km));
	KM_DBG_ASSERT(amt_idx != NULL);

	km_hw_amt_release(km->hw, amt_idx);
}
