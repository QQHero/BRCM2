/*
 * Implementation of wlc_key operations
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
 * $Id: km_key.c 808142 2022-02-10 01:30:35Z $
 */

#include "km_key_pvt.h"
#include <wlc_txc.h>
#include <wlc_rx.h>
#include <wlc_wnm.h>

#if defined(BCM43684_HDRCONVTD_ETHER_LEN_WAR) && defined(BCMPCIEDEV)
#include <sbtopcie.h> // sbtopcie_pio ether_type to host
#include <wlc_pktfetch.h>
#endif /* BCM43684_HDRCONVTD_ETHER_LEN_WAR */

/* internal interface */

struct key_algo_entry {
	key_algo_t algo;
	key_algo_init_t init;
};
typedef struct key_algo_entry key_algo_entry_t;

#define KAE(algo, fn) {CRYPTO_ALGO_##algo, km_key_##fn##_init}

static const key_algo_entry_t key_algo_entries[] = {
	KAE(NONE, none),
	KAE(WEP1, wep),
	KAE(WEP128, wep),
	KAE(AES_CCM, aes),
	KAE(AES_OCB_MSDU, aes),
	KAE(AES_OCB_MPDU, aes),
	KAE(AES_CCM256, aes),
	KAE(AES_GCM, aes),
	KAE(AES_GCM256, aes),
#ifdef MFP
	KAE(BIP, aes),
	KAE(BIP_CMAC256, aes),
	KAE(BIP_GMAC, aes),
	KAE(BIP_GMAC256, aes),
#endif /* MFP */
#ifndef LINUX_CRYPTO
	KAE(TKIP, tkip),
#else
	KAE(TKIP, tkip_linux),
#endif /* LINUX_CRYPTO */
#ifdef BCMWAPI_WPI
	KAE(SMS4, wapi),
#endif /* BCMWAPI_WPI */
};

#undef KAE

static const key_algo_entry_t*
key_find_algo_entry(key_algo_t algo)
{
	size_t i;
	for (i = 0; i < ARRAYSIZE(key_algo_entries); ++i) {
		const key_algo_entry_t* ae = &key_algo_entries[i];
		if (ae->algo == algo)
			return ae;
	}
	return NULL;
}

#if defined(BCMDBG)
static void
km_key_info_dump(wlc_key_t *key, key_info_t *key_info, struct bcmstrbuf *b)
{
	char eabuf[ETHER_ADDR_STR_LEN];
	bcm_bprintf(b, "\tidx: %03d id: %01d hw idx %02d\n", key_info->key_idx,
		key_info->key_id, key->hw_idx);
	bcm_bprintf(b, "\talgo: %d (%s) hw algo: %d\n", key_info->algo,
		wlc_keymgmt_get_algo_name(KEY_KM(key), key_info->algo), key_info->hw_algo);
	bcm_bprintf(b, "\taddr: %s\n", bcm_ether_ntoa(&key_info->addr, eabuf));
	bcm_bprintf(b, "\tflags: 0x%08x\n", key_info->flags);
	bcm_bprintf(b, "\tlen: key %d, iv %d icv %d\n", key_info->key_len,
		key_info->iv_len, key_info->icv_len);
}
#endif

enum km_key_pkt_type {
	KM_KEY_PKT_DOT11 = 0,
	KM_KEY_PKT_ETHER = 1
};
typedef enum km_key_pkt_type km_key_pkt_type_t;

/* update algo, hw/key idx that depends on corerev.
 */
static void
km_key_update_txd(wlc_key_t *key, void *pkt,
	const struct dot11_header *hdr, uint8 *body, d11txhdr_t *txd)
{
	uint16 mch;
	BCM_REFERENCE(pkt);
	BCM_REFERENCE(hdr);
	BCM_REFERENCE(body);

	KM_DBG_ASSERT(txd != NULL);
	if (KEY_COREREV_GE128(key)) {
		ASSERT(RATELINKMEM_ENAB(key->wlc->pub) == TRUE);
		/* nothing can be done here, link_entry should be ok already */
	} else if (KEY_COREREV_GE40(key)) {
		d11txh_cache_common_t *cache_info;
		d11actxh_t *ac_txh = &txd->rev40;
		d11actxh_cache_t *d11ac_cache_info;
		d11ac_cache_info = WLC_TXD_CACHE_INFO_GET(ac_txh, KEY_PUB(key)->corerev);
		cache_info = &d11ac_cache_info->common;

		cache_info->BssIdEncAlg |= key->info.hw_algo << D11AC_ENCRYPT_ALG_SHIFT;
		cache_info->KeyIdx = (uint8)KM_KEY_HW_IDX_TO_SLOT(key, key->hw_idx);
	} else {
		d11txh_pre40_t *txh = &txd->pre40;
		mch = ltoh16(txh->MacTxControlHigh);
		mch &= ~(TXC_SECKEY_MASK | TXC_SECTYPE_MASK);
		mch |= (key->hw_idx << TXC_SECKEY_SHIFT) & TXC_SECKEY_MASK;
		mch |= (key->info.hw_algo << TXC_SECTYPE_SHIFT) & TXC_SECTYPE_MASK;
		txh->MacTxControlHigh = htol16(mch);
	}
}

/* public interface */
int
km_key_create(keymgmt_t *km, const key_info_t *key_info, wlc_key_t **out_key)
{
	const key_algo_entry_t *ae;
	wlc_key_t *key = NULL;
	int err = BCME_OK;
	wlc_info_t *wlc;

	wlc = wlc_keymgmt_get_wlc(km);
	KM_DBG_ASSERT(km != NULL && wlc != NULL &&
		key_info != NULL && out_key != NULL);

	ae = key_find_algo_entry(key_info->algo);
	if (ae == NULL) {
		err = BCME_UNSUPPORTED;
		goto done;
	}

	key = (wlc_key_t *)MALLOCZ(wlc->osh, sizeof(wlc_key_t));
	if (key == NULL) {
		err = BCME_NOMEM;
		goto done;
	}

	key->magic = KEY_MAGIC;
	key->wlc = wlc;
	key->info = *key_info;

	key->info.flags |= WLC_KEY_FLAG_VALID;

	key->hw_idx = WLC_KEY_INDEX_INVALID;

	/* depending on type - fill in rx/tx by calling init func */
	err = ae->init(key);
	if (err != BCME_OK)
		goto done;

done:
	if (err != BCME_OK) {
		if (key != NULL) {
			MFREE(wlc->osh, key, sizeof(wlc_key_t));
			key = NULL;
		}
	}

	*out_key = key;

	KEY_LOG(("wl%d: %s: exit with status %d key %p\n", WLCWLUNIT(wlc),
		__FUNCTION__, err, key));
	return err;
}

int
km_key_destroy(wlc_key_t **in_key)
{
	wlc_key_t *key;
	int err;

	KM_DBG_ASSERT(in_key != NULL && KEY_VALID(*in_key));
	key = *in_key;

	KM_RESET_KEY_CACHE(KEY_KM(key));

	/* clear key before destroying */
	(void)wlc_key_set_data(key, key->info.algo, NULL, 0);

	km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_KEY_DELETE,
		NULL /* bsscfg */, NULL /* scb */, key, NULL /* pkt */);

	/* note: caller key address is cleared after notification */
	*in_key = NULL;

	KEY_ALGO_CB(err, key->algo_impl.cb->destroy, (key));
	if (err != BCME_OK && err != BCME_UNSUPPORTED) {
		KEY_LOG(("wl%d: %s: error %d destroying key@%p algo impl\n",
			KEY_WLUNIT(key), __FUNCTION__, err, key));
	}
	key->magic = KEY_BAD_MAGIC;
	MFREE(KEY_OSH(key), key, sizeof(wlc_key_t));
	return BCME_OK;
}

#if defined(BCMDBG)
void
km_key_dump(wlc_key_t *key, struct bcmstrbuf *b)
{
	int err;
	KM_DBG_ASSERT(KEY_VALID(key));

	if (!KEY_VALID(key)) {
		bcm_bprintf(b, "key not valid\n");
		return;
	}

	bcm_bprintf(b, "begin key@%p dump:\n", key);
	bcm_bprintf(b, "\twlc: %p\n", KEY_WLC(key));
	km_key_info_dump(key, &key->info, b);
	bcm_bprintf(b, "\texp: %d\n", key->exp);
	KEY_ALGO_CB(err, key->algo_impl.cb->dump, (key, b));
	if (err != BCME_OK && err != BCME_UNSUPPORTED) {
		KEY_LOG(("wl%d: %s: error %d dumping key@%p algo impl\n",
			KEY_WLUNIT(key), __FUNCTION__, err, key));
	}
	bcm_bprintf(b, "end key@%p dump:\n", key);
}
#endif

void
wlc_key_get_info(const wlc_key_t *key, key_info_t *key_info)
{
	if (KEY_VALID(key)) {
		if (key_info != NULL)
			*key_info = key->info;
	} else {
		if (key_info) {
			memset(key_info, 0, sizeof(key_info_t));
			key_info->key_idx = WLC_KEY_INDEX_INVALID;
			key_info->key_id = WLC_KEY_ID_INVALID;
		}
	}
}

/* get pkt info and ptr to body */
static uint8*
km_key_dot11_pkt_get_body(struct dot11_header *hdr, int pkt_len,
	const d11rxhdr_t *rxh, key_pkt_info_t *pkt_info)
{
	int body_off;
	uint16 fc;

	memset(pkt_info, 0, sizeof(*pkt_info));
	fc =  pkt_info->fc = ltoh16(hdr->fc);

	body_off = DOT11_A3_HDR_LEN + KM_KEY_PKT_DOT11_A4_LEN(fc);
	if (KM_KEY_FC_TYPE_QOS_DATA(fc)) {
		pkt_info->flags |= KEY_PKT_QC_PRESENT;
		pkt_info->qc = ltoh16_ua((uint8 *)hdr + body_off);
		body_off += DOT11_QOS_LEN;
		if (KM_KEY_FC_TYPE_HTC(fc)) {
			body_off += DOT11_HTC_LEN;
			pkt_info->flags |= KEY_PKT_HTC_PRESENT;
		}
	}

	/* note: recv path already checked for short frames */
	KM_DBG_ASSERT(pkt_len >= body_off);

	pkt_info->body_len = pkt_len - body_off;
	return (uint8 *)hdr + body_off;
}

int wlc_key_prep_tx_mpdu(wlc_key_t *key, void *pkt, d11txhdr_t *txd)
{
	int err;
	struct dot11_header *hdr;
	uint8 *body;
	int body_len = 0;
	key_pkt_info_t pkt_info;
	uint16 fc;
	uint16 fk;

	KM_ASSERT(KEY_VALID(key));

	KEY_LOG(("wl%d: %s: enter pkt@%p key idx %d algo %d[%s] flags 0x%08x\n",
		KEY_WLUNIT(key), __FUNCTION__, pkt, key->info.key_idx, key->info.algo,
		wlc_keymgmt_get_algo_name(KEY_KM(key), key->info.algo), key->info.flags));

	/* check if the key is valid for tx */
	if (!KEY_TX_ENABLED(key) || (key->info.algo == CRYPTO_ALGO_OFF)) {
		err = (key->info.algo == CRYPTO_ALGO_OFF) ? BCME_OK : BCME_NOTENABLED;
		goto done;
	}
#if defined(BCMDBG)
	else if (key->info.flags & (WLC_KEY_FLAG_GROUP | WLC_KEY_FLAG_MGMT_GROUP)) {
		scb_t *scb = WLPKTTAGSCBGET(pkt);
		KM_DBG_ASSERT(scb != NULL);
		if (scb != NULL) {
			wlc_bsscfg_t *bsscfg = SCB_BSSCFG(scb);
			BCM_REFERENCE(bsscfg);
			KM_DBG_ASSERT(bsscfg != NULL);
			KM_DBG_ASSERT(BSSCFG_AP(bsscfg) || KM_BSSCFG_IS_IBSS(bsscfg) ||
				(key->info.algo == CRYPTO_ALGO_WEP1) ||
				(key->info.algo == CRYPTO_ALGO_WEP128));
		}
	}
#endif /* BCMDBG */

	/* Map remaining unmapped pkt contents to cover the cases where
	 * we receive hw decrypted frame and have to send it using sw encr
	 */
	PKTCTFMAP(KEY_OSH(key), pkt);

	hdr = KEY_MPDU_HDR(key, pkt);

	/* debug: dump pkt */
	KEY_LOG_DUMP_PKT(__FUNCTION__, key, pkt);

	body = km_key_dot11_pkt_get_body(hdr, KEY_PKT_LEN(key, pkt), NULL /* no rxh */, &pkt_info);
	body_len = pkt_info.body_len;

	fc  = pkt_info.fc;
	KM_ASSERT((FC_TYPE(fc) == FC_TYPE_MNG) || FC_TYPE(fc) == FC_TYPE_DATA);

	/* set protected frame bit for data frames, robust uc mgmt
	 * frames and mgmt auth frames. 802.11/2012 8.2.4.1.9, but
	 * not for mc mgmt frames
	 */
	fk = fc & FC_KIND_MASK;
	if ((FC_TYPE(fc) == FC_TYPE_DATA) || ((FC_TYPE(fc) == FC_TYPE_MNG) &&
			(!ETHER_ISMULTI(&hdr->a1) || (fk == FC_AUTH)))) {
		fc |= FC_WEP;
		hdr->fc = htol16(fc);

		/* HW encryption, default */
		if (RATELINKMEM_ENAB(KEY_PUB(key)) && txd) {
			d11txh_rev128_t *rev128_txh = &txd->rev128;
			if (WLC_KEY_IN_HW(&key->info)) {
				rev128_txh->MacControl_0 |= D11REV128_TXC_ENC;
			} else { /* Since SW encryption, avoid HW encryptor */
				rev128_txh->MacControl_0 &= ~D11REV128_TXC_ENC;
			}
		}

		/* ensure we have iv */
		if (body_len < key->info.iv_len) {
			KEY_LOG(("wl%d: %s: packet body of len %d has no space for iv\n",
				KEY_WLUNIT(key), __FUNCTION__, body_len));
			err = BCME_BUFTOOSHORT;
			goto done;
		}
	}

	/* Set 'use_sw_uc_mfp' to 1 to enable SW encryption for UC mgmt frames.
	 * Always enable SW encryption for a unicast deauth frame as it can be pending
	 * on the hw fifo when the scb and its keys are freed.
	 */
	if (SW_MFP_ENAB(KEY_PUB(key)) || (FC_TYPE(fc) == FC_TYPE_MNG && fk == FC_DEAUTH)) {
		/* if protecting mgmt packet, mark it as MFP and use sw/ enc */
		if (FC_TYPE(fc) == FC_TYPE_MNG && fk != FC_AUTH) {
			WLPKTTAG(pkt)->flags |= WLF_MFP;
			KEY_ERR(("wl%d: %s: choosing SW encryption for MGMT frames\n",
				KEY_WLUNIT(key), __FUNCTION__));
			/* set wep disable bit as SW enc is used */
			if (RATELINKMEM_ENAB(KEY_PUB(key)) && txd) {
				d11txh_rev128_t *rev128_txh = &txd->rev128;
				rev128_txh->MacControl_0 &= ~D11REV128_TXC_ENC;
			}
			txd = NULL;
		}
	}

	/* dispatch to algo impl which for h/w encr only updates iv */
	KEY_ALGO_CB(err, key->algo_impl.cb->tx_mpdu, (key, pkt, hdr, body, body_len, txd));
	if (err != BCME_OK)
		goto done;

#if defined(BCMDBG)
	/* ICV error generation support */
	if (key->info.flags & WLC_KEY_FLAG_GEN_ICV_ERR) {
		if (FC_TYPE(fc) != FC_TYPE_MNG || !ETHER_ISMULTI(&hdr->a1)) {
			key->info.flags &= ~WLC_KEY_FLAG_GEN_ICV_ERR;
			body[body_len] = ~body[body_len];
		}
	}
#endif /* BCMDBG */

	/* update pkt len except for mc mgmt frames unless linux crypto or hw enc
	 * if hw enc, update txd(txh) - hw/algo, key idx.
	 */
	if (!WLC_KEY_IN_HW(&key->info) || txd == NULL) {
		if (FC_TYPE(fc) != FC_TYPE_MNG || !ETHER_ISMULTI(&hdr->a1)) {
			if (!(WLC_KEY_IS_LINUX_CRYPTO(&key->info)))
				PKTSETLEN(KEY_OSH(key), pkt,
					PKTLEN(KEY_OSH(key), pkt) + key->info.icv_len);
		}
	} else {
		km_key_update_txd(key, pkt, hdr, body, txd);
	}

	/* note: algorithm dependent mic may have been added to the body of the packet.
	 * if body_len is needed beyond this point it must be adjusted appropriately.
	 * body_len = key->info.icv_len + mic_size
	 */

	KEY_LOG_DUMP_PKT(__FUNCTION__, key, pkt);

done:
	KEY_LOG(("wl%d: %s: exit status %d\n", KEY_WLUNIT(key), __FUNCTION__, err));
	return err;
}

int
wlc_key_fill_link_entry(wlc_key_t *key, scb_t *scb, struct d11linkmem_entry *link_entry)
{
	int err = BCME_OK;
	uint16 amt_idx;
	KM_ASSERT(KEY_VALID(key));

	KEY_LOG(("wl%d: %s: enter key idx %d algo %d[%s] flags 0x%08x\n",
		KEY_WLUNIT(key), __FUNCTION__, key->info.key_idx, key->info.algo,
		wlc_keymgmt_get_algo_name(KEY_KM(key), key->info.algo), key->info.flags));

	/* check if the key is valid for tx */
	if (!KEY_TX_ENABLED(key) || (key->info.algo == CRYPTO_ALGO_OFF)) {
		err = (key->info.algo == CRYPTO_ALGO_OFF) ? BCME_OK : BCME_NOTENABLED;
		goto done;
	}

	link_entry->EncryptInfo = (key->info.hw_algo & D11_REV128_ENCRYPTALGO_MASK);
	link_entry->EncryptInfo |= ((KM_KEY_HW_IDX_TO_SLOT(key, key->hw_idx) <<
		D11_REV128_KEYIDX_SHIFT) & D11_REV128_KEYIDX_MASK);
	link_entry->EncryptInfo |= ((KM_KEY_TKIP_TSC_TTAK_IDX(key, key->hw_idx) <<
			D11_REV128_TKIPPHASE1KEYIDX_SHIFT) & D11_REV128_TKIPPHASE1KEYIDX_MASK);

	/* update amt defkey if bss is using wep and defkey is inuse */
	amt_idx = wlc_keymgmt_get_scb_amt_idx(KEY_KM(key), scb);

	if (scb && WSEC_WEP_ENABLED(SCB_BSSCFG(scb)->wsec) &&
	  ((key->info.algo == CRYPTO_ALGO_WEP1) || (key->info.algo == CRYPTO_ALGO_WEP128))) {
		km_amt_defkey_update(KEY_KM(key), amt_idx);
	}

done:
	KEY_LOG(("wl%d: %s: exit status %d\n", KEY_WLUNIT(key), __FUNCTION__, err));
	return err;
}

int
wlc_key_prep_tx_msdu(wlc_key_t *key, void *pkt, size_t frag_length, uint8 prio)
{
	int err;
	struct ether_header *hdr = NULL;
	uint8 *body = NULL;
	int body_len = 0;

	KM_DBG_ASSERT(KEY_VALID(key));

	KEY_LOG(("wl%d: %s: enter pkt@%p key idx %d\n", KEY_WLUNIT(key),
		__FUNCTION__, pkt, key->info.key_idx));

	/* check if the key is valid for tx */
	if (!KEY_TX_ENABLED(key)) {
		err = (key->info.algo == CRYPTO_ALGO_OFF) ? BCME_OK : BCME_NOTENABLED;
		goto done;
	}

	KEY_LOG_DUMP_PKT(__FUNCTION__, key, pkt);

	if (key->algo_impl.cb->tx_msdu != NULL) {
		hdr = KEY_MSDU_HDR(key, pkt);
		body = ((uint8 *)hdr) + ETHER_HDR_LEN;
		body_len = KEY_PKT_LEN(key, pkt) - ETHER_HDR_LEN;
	}

	KEY_ALGO_CB(err, key->algo_impl.cb->tx_msdu, (key, pkt, hdr,
		body, body_len, frag_length, prio));
	/* some algorithms don't support tx msdu callback */
	if (err == BCME_UNSUPPORTED)
		err = BCME_OK;

done:
	KEY_LOG(("wl%d: %s: exit with status %d\n", KEY_WLUNIT(key), __FUNCTION__, err));
	return err;
}

int
wlc_key_rx_mpdu(wlc_key_t *key, void *pkt, d11rxhdr_t *rxh)
{
	struct dot11_header *hdr;
	uint8 *body;
	int body_len = 0;
	uint16 fc;
	uint16 qc;
	bool hwdec = TRUE;
	int err = BCME_OK;
	key_pkt_info_t pkt_info;
	uint16 *RxStatus1;
	uint8 icv_len = 0;
	void *last_pkt = NULL;

	KM_DBG_ASSERT(KEY_VALID(key) && rxh != NULL);

	RxStatus1 = D11RXHDR_ACCESS_REF(rxh, KEY_PUB(key)->corerev, RxStatus1);

	KEY_LOG(("wl%d: %s: enter pkt@%p rxs %04x DECATMPT %d DECERR %d\n",
		KEY_WLUNIT(key), __FUNCTION__, pkt,
		*RxStatus1, ((*RxStatus1 & RXS_DECATMPT) ? 1 : 0),
		((*RxStatus1 & RXS_DECERR) ? 1 : 0)));

	hdr = KEY_MPDU_HDR(key, pkt);

	KEY_LOG_DUMP_PKT(__FUNCTION__, key, pkt);

	body = km_key_dot11_pkt_get_body(hdr, KEY_PKT_LEN(key, pkt), rxh, &pkt_info);
	body_len = pkt_info.body_len;

	fc  = pkt_info.fc;
	KM_ASSERT((FC_TYPE(fc) == FC_TYPE_MNG) ||
		(FC_TYPE(fc) == FC_TYPE_DATA));
	qc = pkt_info.qc;

	/* check if the key is valid for rx */
	if (!KEY_RX_ENABLED(key)) {
		err = BCME_NOTENABLED;
		goto done;
	}

	/* wep must be set except for  bc/mc mgmt frames */
	if (!(fc & FC_WEP)) {
		if ((FC_TYPE(fc) != FC_TYPE_MNG) || !ETHER_ISMULTI(&hdr->a1)) {
			err = BCME_UNSUPPORTED;
			goto done;
		}
	}

	if (FC_TYPE(fc) != FC_TYPE_MNG || !ETHER_ISMULTI(&hdr->a1)) {
		last_pkt = pkt;
		if ((qc & QOS_AMSDU_MASK))
			last_pkt = pktlast(KEY_OSH(key), pkt);
		/* SPLIT RX: icv is in the host */
		if (PKTISHDRCONVTD(KEY_OSH(key), last_pkt) &&
				((qc & QOS_AMSDU_MASK) || D11REV_GE(KEY_PUB(key)->corerev, 130))) {
			/* XXX SWWLAN-187559 WAR
			 * [43684] Header-converted and AMSDU there is no ICV
			 * CRBCAD11MAC-4623
			 * [corerev>=130] Header-converted then MIC/ICV is trimmed
			 */
		} else {
			icv_len = key->info.icv_len;
		}
	}

	if (body_len < (key->info.iv_len + icv_len)) {
		err = BCME_BUFTOOSHORT;
		goto done;
	}

	/* handle h/w decrypted */
#ifdef BCMWAPI_WPI
	if ((*RxStatus1 & RXS_DECATMPT) &&
		(key->info.algo == CRYPTO_ALGO_SMS4) &&
		(key->info.flags & WLC_KEY_FLAG_DEFAULT_BSS)) {
		*RxStatus1 &= ~RXS_DECATMPT;
	}
#endif /* BCMWAPI_WPI */

#ifdef BCMDBG
	/* mfp currently done in s/w */
	if (WLPKTFLAG_MFP(WLPKTTAG(pkt)))
		KM_DBG_ASSERT(!(*RxStatus1 & RXS_DECATMPT));
#endif /* BCMDBG */

	if ((FC_TYPE(fc) == FC_TYPE_MNG) && (*RxStatus1 & RXS_DECATMPT) &&
		((fc & FC_KIND_MASK) == FC_AUTH) &&
		KM_WEP_ALGO(key->info.algo) && !WLC_KEY_IS_DEFAULT_BSS(&key->info)) {
		err = km_key_wep_rx_defkey_fixup(key, body, body_len);
		if (err != BCME_OK)
			goto done;
		*RxStatus1 &= ~(RXS_DECATMPT | RXS_DECERR);
	}

	/* algorithm specific processing */
	if (KEY_SW_ENC_DEC(key, pkt, rxh)) {
		hwdec = FALSE;
		if (WLPKTTAG(pkt)->flags & (WLF_AMSDU | WLF_RX_KM)) {
			KEY_LOG(("wl%d: %s: no sw support for A-MSDU deagg or ICV is trimmed!"
				"-  key idx %d, pkt flags %d\n", KEY_WLUNIT(key), __FUNCTION__,
				key->info.key_idx, WLPKTTAG(pkt)->flags));
			err = BCME_UNSUPPORTED;
		}
	} else if (*RxStatus1 &	RXS_DECERR) {
		err = BCME_DECERR;
		/* XXX WAR for CRWLDOT11M-3109:
		 * Hardware reports decryption error when there is zero payload in the last
		 * fragment. Driver tries to recover with SW decryption and MIC calculation.
		 */
		if (D11REV_LT(KEY_PUB(key)->corerev, 128) && key->info.algo == CRYPTO_ALGO_TKIP &&
			body_len <= (DOT11_IV_TKIP_LEN + TKIP_MIC_SIZE + DOT11_ICV_LEN)) {
			hwdec = FALSE;
		}
	}

	if (err == BCME_OK || err == BCME_DECERR) {
		/* ensure we have the full frame if required for s/w decrypt or mic */
		if ((PKTFRAGUSEDLEN(KEY_OSH(key), pkt) > 0) &&
			(!hwdec || ((WLC_KEY_ALGO_HAS_MIC(key->info.algo) &&
			!WLC_KEY_MIC_IN_HW(&key->info))))) {
			err = BCME_DATA_NOTFOUND;
			goto done;
		}

		pkt_info.status = err;
		pkt_info.flags |= hwdec ? KEY_PKT_HWDEC : 0;
		pkt_info.flags |= (KEY_PKT_PKTC_FIRST | KEY_PKT_PKTC_LAST);
		if (WLPKTTAG(pkt)->flags & WLF_RX_PKTC_NOTFIRST)
			pkt_info.flags &= ~KEY_PKT_PKTC_FIRST;
		if (WLPKTTAG(pkt)->flags & WLF_RX_PKTC_NOTLAST)
			pkt_info.flags &= ~KEY_PKT_PKTC_LAST;

		KEY_ALGO_CB(err, key->algo_impl.cb->rx_mpdu,
			(key, pkt, hdr, body, body_len, &pkt_info));
	}

	/* if algorithm does not support rx/decrypt, signal an error.
	 * also, per spec increment wep error counts
	 */
	if (err == BCME_UNSUPPORTED) {
		WLCNTINCR(KEY_CNT(key)->wepundec);
		if (ETHER_ISMULTI(&hdr->a1))
			WLCNTINCR(KEY_CNT(key)->wepundec_mcst);
	} else {
		*RxStatus1 |= RXS_DECATMPT; /* attempted h/w or s/w */
		if (!hwdec) {
			KEY_PUB(key)->swdecrypt++;
		}
		if (err == BCME_DECERR || err == BCME_MICERR)
			*RxStatus1 |= RXS_DECERR;
	}

	/* strip off icv */
	if (!(WLC_KEY_IS_LINUX_CRYPTO(&key->info))) {

		KM_ASSERT(!(WLPKTTAG(pkt)->flags & WLF_RX_KM));

		if (FC_TYPE(fc) != FC_TYPE_MNG || !ETHER_ISMULTI(&hdr->a1)) {
			/* SPLIT RX: icv is in the host */
			if (icv_len == 0) {
				/* XXX SWWLAN-187559 WAR
				 * [43684] Header-converted and AMSDU there is no ICV
				 * CRBCAD11MAC-4623
				 * [corerev>=130] Header-converted then MIC/ICV is trimmed
				 */
			} else if (last_pkt) {
				PKTFRAG_TRIM_TAILBYTES(KEY_OSH(key), last_pkt, key->info.icv_len,
					TAIL_BYTES_TYPE_ICV);
#if defined(BCM43684_HDRCONVTD_ETHER_LEN_WAR) && defined(BCMPCIEDEV)
				/* Correct ether lenght for type 1 conversion */
				if (PKTISHDRCONVTD(KEY_OSH(key), last_pkt) &&
					!RXHDR_GET_CONV_TYPE(rxh, key->wlc))
				{
					struct ether_header *eh;
					uint16 ether_type;

					eh = (struct ether_header *)(body + key->info.iv_len + 2);
					ether_type = ntoh16(eh->ether_type);
					if (ether_type < ETHER_TYPE_MIN) {
						uint64 haddr_u64 = PKTFRAGDATA_LO(KEY_OSH(key),
							last_pkt, LB_FRAG1) + ETHER_TYPE_1_OFFSET;

						ether_type -= key->info.icv_len;
						ether_type = hton16(ether_type);

						sbtopcie_pio((uintptr)&ether_type, haddr_u64,
							ETHER_TYPE_LEN, SBTOPCIE_DIR_D2H);
					}
				}
#endif /* BCM43684_HDRCONVTD_ETHER_LEN_WAR && BCMPCIEDEV */
			}

			WLPKTTAG(pkt)->flags |= WLF_RX_KM;
		}
	}

	KEY_LOG_DUMP_PKT(__FUNCTION__, key, pkt);

done:
	if (err != BCME_OK) {
		if (fc & FC_WEP) {
			WLCNTINCR(KEY_CNT(key)->rxundec);
			if (ETHER_ISMULTI(&hdr->a1))
				WLCNTINCR(KEY_CNT(key)->rxundec_mcst);

			/* notify keymgmt of error */
			switch (err) {
			case BCME_DATA_NOTFOUND:
				km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_NEED_PKTFETCH,
					NULL, NULL, key, pkt);
				break;
			case BCME_DECERR: /* fall through */
			case BCME_MICERR: /* mpdu MIC error is treated as a decrypt error */
				km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_DECRYPT_ERROR,
					NULL, NULL, key, pkt);
				break;
			case BCME_BUFTOOSHORT:
			case BCME_BADLEN:
				WLCNTINCR(KEY_CNT(key)->rxrunt);
				/* fall through */
			default:
				km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_DECODE_ERROR,
					NULL, NULL, key, pkt);
				break;
			}
		} else { /*  unencrypted */
			scb_t *scb = WLPKTTAGSCBGET(pkt);
			KM_DBG_ASSERT(scb != NULL);
			if (!km_allow_unencrypted(KEY_KM(key), &key->info, scb,
				hdr, qc, body, body_len, pkt)) {
				KEY_LOG_DECL(char eabuf[ETHER_ADDR_STR_LEN]);

				WLCNTINCR(KEY_CNT(key)->rxbadproto);
				WLCNTINCR(KEY_CNT(key)->wepexcluded);
				if (ETHER_ISMULTI(&hdr->a1))
					WLCNTINCR(KEY_CNT(key)->wepexcluded_mcst);

				KEY_LOG(("wl%d.%d: %s: disallowed unencrypted frame from %s\n",
					KEY_WLUNIT(key), WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
					__FUNCTION__, bcm_ether_ntoa(&hdr->a2, eabuf)));
				if (SCB_LEGACY_WDS(scb)) {
					uint32 wpa_auth;
					wlc_bsscfg_t *bsscfg = NULL;
					bsscfg = SCB_BSSCFG(scb);
					KM_DBG_ASSERT(bsscfg != NULL);
					wpa_auth = bsscfg->WPA_auth;
					if ((wpa_auth != WPA_AUTH_DISABLED) &&
#ifdef BCMWAPI_WPI
							(!IS_WAPI_AUTH(wpa_auth)) &&
#endif /* BCMWAPI_WPI */
							(key->info.algo != CRYPTO_ALGO_OFF)) {
						KM_DBG_ASSERT(pkt != NULL);
						km_null_key_deauth(KEY_KM(key),
							WLPKTTAGSCBGET(pkt), pkt);
					}
				}
			} else {
				err = BCME_OK;
			}
		}
	} else {
		WLCNTINCR(KEY_CNT(key)->decsuccess);
		if (ETHER_ISMULTI(&hdr->a1))
			WLCNTINCR(KEY_CNT(key)->decsuccess_mcst);
	}

	KEY_LOG(("wl%d: %s: exit status %d rxs %04x DECATMPT %d DECERR %d wep %d\n",
		KEY_WLUNIT(key), __FUNCTION__, err, *RxStatus1,
		((*RxStatus1 & RXS_DECATMPT) ? 1 : 0),
		((*RxStatus1 & RXS_DECERR) ? 1 : 0),
		((fc & FC_WEP) ? 1 : 0)));
	return err;
}

int
wlc_key_rx_msdu(wlc_key_t *key, void *pkt, d11rxhdr_t *rxh)
{
	int err;
	struct ether_header *hdr;
	uint8 *body;
	int body_len = 0;
	key_pkt_info_t pkt_info;
	bool hwdec;
	uint16 RxStatus2;
	bool is_amsdu;

	KM_ASSERT(KEY_VALID(key));

	RxStatus2 = D11RXHDR_ACCESS_VAL(rxh, KEY_PUB(key)->corerev, RxStatus2);
	is_amsdu = RXHDR_GET_AMSDU(rxh, key->wlc);

	KEY_LOG(("wl%d: %s: enter pkt@%p rxs status1 0x%04x  status2 0x%04x\n",
		KEY_WLUNIT(key), __FUNCTION__, pkt,
		D11RXHDR_ACCESS_VAL(rxh, KEY_PUB(key)->corerev, RxStatus1),
		RxStatus2));

	/* check if the key is valid for rx */
	if (!KEY_RX_ENABLED(key)) {
		err = BCME_NOTENABLED;
		goto done;
	}

	hdr = KEY_MSDU_HDR(key, pkt);

	KEY_LOG_DUMP_PKT(__FUNCTION__, key, pkt);

	body = ((uint8 *)hdr) + ETHER_HDR_LEN;
	body_len = KEY_PKT_LEN(key, pkt) - ETHER_HDR_LEN;

	/* algorithm specific processing */
	hwdec = WLC_KEY_MIC_IN_HW(&key->info) && key->info.algo == CRYPTO_ALGO_TKIP &&
		(RxStatus2 & RXS_TKMICATMPT);
#ifndef IGNORE_MIC_ERROR_43684A0
	if (hwdec && (RxStatus2 & RXS_TKMICERR))
		pkt_info.status = BCME_MICERR;
	else
#endif /* IGNORE_MIC_ERROR_43684A0 */
		pkt_info.status = BCME_OK;

	pkt_info.flags = hwdec ? KEY_PKT_HWDEC : 0;

	/* Update pkt_info with amsdu details */
	pkt_info.qc = (is_amsdu ? (1 << QOS_AMSDU_SHIFT) : 0);

	KEY_ALGO_CB(err, key->algo_impl.cb->rx_msdu, (key, pkt, hdr, body, body_len, &pkt_info));
	if (err == BCME_OK) {
		WLCNTINCR(KEY_CNT(key)->decsuccess);
		if (ETHER_ISMULTI(&hdr->ether_dhost[0]))
			WLCNTINCR(KEY_CNT(key)->decsuccess_mcst);
	}

#if defined(BCMDBG)
	if (err != BCME_UNSUPPORTED) {
		KEY_LOG_DUMP_PKT(__FUNCTION__, key, pkt);
	}
#endif

done:
	/* check error and notify keymgmt */
	switch (err) {
	case BCME_OK:
		break;
	case BCME_BUFTOOSHORT:
	case BCME_BADLEN:
		WLCNTINCR(KEY_CNT(key)->rxrunt);
		break;
	case BCME_MICERR:
		km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_MSDU_MIC_ERROR, NULL, NULL, key, pkt);
		break;
	case BCME_UNSUPPORTED: /* rx_msdu is optional, mask error if unsupported */
		err = BCME_OK;
		break;
	default:
		km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_DECODE_ERROR, NULL, NULL, key, pkt);
		break;
	}

	KEY_LOG(("wl%d: %s: exit status %d\n", KEY_WLUNIT(key), __FUNCTION__, err));
	return err;
}

/* return -1, 0, or + 1 based on whether kseq < seq or equal or > */
static int
km_key_seq_cmp(wlc_key_t *key, const uint8 *seq, size_t seq_len,
	wlc_key_seq_id_t seq_id, bool tx)
{
	uint8 key_seq[KM_KEY_MAX_DATA_LEN];
	int key_seq_len;
	key_seq_len = wlc_key_get_seq(key, key_seq, sizeof(key_seq), seq_id, tx);
	KM_DBG_ASSERT(key_seq_len >= 0 && (uint)key_seq_len <= sizeof(key_seq));

	seq_len = (size_t)MIN(key_seq_len, (int)seq_len);
	if (km_key_seq_less(key_seq, seq, seq_len)) {
		return -1;
	} else if (!memcmp(key_seq, seq, seq_len)) {
		return 0;
	} else {
		return 1;
	}
}

int
wlc_key_get_seq(wlc_key_t *key, uint8 *buf, size_t buf_len,
    key_seq_id_t seq_id, bool tx)
{
	size_t data_len = 0;
	int err;

	KM_DBG_ASSERT(KEY_VALID(key));
	KEY_ALGO_CB(err,  key->algo_impl.cb->get_data,
		(key, buf, buf_len, &data_len, WLC_KEY_DATA_TYPE_SEQ, seq_id, tx));

	if (err == BCME_OK)
		return (int)data_len;
	else
		return -((int)data_len);
}

int
wlc_key_set_seq(wlc_key_t *key, const uint8 *seq, size_t seq_len,
    key_seq_id_t seq_id, bool tx)
{
	int err = BCME_OK;
	key_seq_id_t ins;
	key_seq_id_t ins_begin;
	key_seq_id_t ins_end;

	KM_DBG_ASSERT(KEY_VALID(key));

	/* allow only one tx seq, and for rx, setting the seq can be selected */
	if (tx || WLC_KEY_IS_MGMT_GROUP(&key->info))
		seq_id = 0;

	KEY_RESOLVE_SEQ(key, seq_id, tx, ins_begin, ins_end);
	for (ins = ins_begin; ins < ins_end; ++ins) {
		/* ensure seq > key seq */
		if (km_key_seq_cmp(key, seq, seq_len, ins, tx) > 0) {
			err = BCME_REPLAY;
			break;
		}

		KEY_ALGO_CB(err, key->algo_impl.cb->set_data,
			(key, seq, seq_len, WLC_KEY_DATA_TYPE_SEQ, ins, tx));

		if (err != BCME_OK) {
			break;
		}
#ifdef BRCMAPIVTW
		if (WLC_KEY_USE_IVTW(&key->info)) {
			err = KM_SET_IVTW(KEY_KM(key), &key->info, ins, seq, seq_len);
			if (err != BCME_OK) {
				break;
			}
		}
#endif /* BRCMAPIVTW */

	}

	KEY_LOG(("wl%d: %s: key@%p key_idx 0x%04x seq len %d status %d\n",
		KEY_WLUNIT(key), __FUNCTION__, key, key->info.key_idx, (int)seq_len, err));
	return err;
}

int
wlc_key_set_data(wlc_key_t *key, wlc_key_algo_t algo,
    const uint8 *data, size_t data_len)
{
	const key_algo_entry_t *ae;
	int err;
#if defined(STA) && defined(WLWNM)
	wlc_info_t *wlc;
#endif /* STA && WLWNM */

	KM_DBG_ASSERT(KEY_VALID(key));
	KM_ASSERT(data_len == 0 || data != NULL);

	KEY_LOG(("wl%d: %s: enter key@%p key idx 0x%04x algo %d[%s] data len %d\n",
		KEY_WLUNIT(key), __FUNCTION__, key, key->info.key_idx, algo,
		wlc_keymgmt_get_algo_name(KEY_KM(key), algo), (int)data_len));
#if defined(STA) && defined(WLWNM)
	wlc = key->wlc;
	/* Dont install new keys until STA exits WNM sleep mode */
	if ((err = wlc_wnm_is_wnmsleeping(wlc)) != BCME_OK) {
		goto done;
	}
#endif /* STA && WLWNM */
	/* algo must be supported, and key must have a valid key index */
	if (!km_algo_is_supported(KEY_KM(key), algo) ||
		(key->info.key_idx == WLC_KEY_INDEX_INVALID)) {
		err = BCME_UNSUPPORTED;
		goto done;
	}

	/* null data is treated as changing to algo 'none' */
	if (!data_len) {
		algo = CRYPTO_ALGO_NONE;
	}

	/* handle algorithm change. */
	if (key->info.algo != algo) {
		/* reset key to clear any data - any h/w data associated with
		 * the key is cleared when keymgmt notification is handled.
		 * destroy key algorithm specific data
		 * update the algo and init algo implementation
		 */
		ae = key_find_algo_entry(algo);
		if (!ae) {
			err = BCME_UNSUPPORTED;
			goto done;
		}

		KEY_ALGO_CB(err, key->algo_impl.cb->set_data,
			(key, NULL, 0, WLC_KEY_DATA_TYPE_KEY, 0, TRUE));
		if (err != BCME_OK && err != BCME_UNSUPPORTED) {
			KEY_LOG(("wl%d: %s: error %d setting data for key@%p algo impl\n",
				KEY_WLUNIT(key), __FUNCTION__, err, key));
		}

		/* this notification is for clearing external state (e.g. hw key data) if any */
		km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_KEY_UPDATE,
			NULL /* bsscfg */, NULL /* scb */, key, NULL /* pkt */);

		KEY_ALGO_CB(err, key->algo_impl.cb->destroy, (key));
		if (err != BCME_OK && err != BCME_UNSUPPORTED) {
			KEY_LOG(("wl%d: %s: error %d destroying key@%p algo impl\n",
				KEY_WLUNIT(key), __FUNCTION__, err, key));
		}

		key->info.algo = algo;
		key->info.hw_algo = wlc_keymgmt_algo_to_hw_algo(KEY_KM(key), algo);
		err = ae->init(key);
		KM_DBG_ASSERT(err == BCME_OK);
		if (err != BCME_OK)
			goto done;
	}
	if (data && (data_len > 0)) {
		uint8 key_data[KM_KEY_MAX_DATA_LEN];
		size_t key_data_len = 0;

		err = km_key_get_data(key, key_data, KM_KEY_MAX_DATA_LEN, &key_data_len,
				WLC_KEY_DATA_TYPE_KEY, 0, TRUE);
		if (err == BCME_OK) {
			if ((key_data_len == data_len) && !memcmp(key_data, data, key_data_len)) {
				goto done;
			}
		}
	}
	KEY_ALGO_CB(err, key->algo_impl.cb->set_data,
		(key, data, data_len, WLC_KEY_DATA_TYPE_KEY, 0, TRUE));
	if (err != BCME_OK) {
		if (err == BCME_UNSUPPORTED) {
			err = BCME_OK;
		} else if (err == BCME_REPLAY) {
			err = BCME_OK;
			goto done; /* need to skip notification that resets ivtw */
		} else {
			goto done;
		}
	}

#ifdef GTK_RESET
	if (!WLC_KEY_IS_AP(&key->info) && WLC_KEY_IS_GROUP(&key->info))
		key->info.flags |= WLC_KEY_FLAG_GTK_RESET;
#endif
	key->exp = 0; /* new data, so clear expiration */
	km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_KEY_UPDATE,
		NULL /* bsscfg */, NULL /* scb */, key, NULL /* pkt */);

done:
	KEY_LOG(("wl%d: %s: exit status %d\n", KEY_WLUNIT(key), __FUNCTION__, err));
	return err;
}

/* Reset data and any other state for the key. Algorithm changes to none . */
int
wlc_key_reset(wlc_key_t *key)
{
	return wlc_key_set_data(key, key->info.algo, NULL, 0);
}

/* internal to keymgmt implementation - used to access key data. in
 * debug builds, the data is made available via wlc_key_get_data...
 */
int km_key_get_data(wlc_key_t *key, uint8* data, size_t data_size,
    size_t *data_len, wlc_key_data_type_t data_type, int instance, bool tx)
{
	int err;

	STATIC_ASSERT(sizeof(key->info.flags) == sizeof(uint32));

	KM_DBG_ASSERT(KEY_VALID(key));

	switch (data_type) {
	case WLC_KEY_DATA_TYPE_FLAGS:
		if (data_len)
			*data_len = sizeof(key->info.flags);
		if (data_size < sizeof(key->info.flags)) {
			err = BCME_BUFTOOSHORT;
			break;
		}
		htol32_ua_store(key->info.flags, data);
		err = BCME_OK;
		break;
	default:
		KEY_ALGO_CB(err, key->algo_impl.cb->get_data,
			(key, data, data_size, data_len, data_type, instance, tx));
		break;
	}

	return err;
}

void
km_key_set_flags(wlc_key_t *key, wlc_key_flags_t flags)
{
	KM_DBG_ASSERT(KEY_VALID(key));

	flags = (key->info.flags & ~KM_KEY_SETTABLE_FLAGS) |
		(flags & KM_KEY_SETTABLE_FLAGS);

	if (flags != key->info.flags) {
		bool do_notify = KM_KEY_FLAGS_NOTIFY_MASK(key->info.flags) !=
			KM_KEY_FLAGS_NOTIFY_MASK(flags);
		key->info.flags = flags;
		if (do_notify)
			km_notify(KEY_KM(key), WLC_KEYMGMT_NOTIF_KEY_UPDATE,
				NULL /* bsscfg */, NULL /* scb */, key, NULL /* pkt */);
	}
}

int
wlc_key_set_flags(wlc_key_t *key, wlc_key_flags_t flags)
{
	int err = BCME_OK;

	KM_DBG_ASSERT(KEY_VALID(key));

	if (key->info.key_idx == WLC_KEY_INDEX_INVALID) {
		err = BCME_UNSUPPORTED;
		goto done;
	}

	flags &= WLC_KEY_SETTABLE_FLAGS;
	flags |= (key->info.flags & ~WLC_KEY_SETTABLE_FLAGS);
	km_key_set_flags(key, flags);

done:
	return err;
}

int
wlc_key_get_data(wlc_key_t *key, uint8* data, size_t data_size,
	size_t *data_len)
{
	return km_key_get_data(key, data, data_size, data_len,
		WLC_KEY_DATA_TYPE_KEY, 0, TRUE);
}

int
wlc_key_get_data_ex(wlc_key_t *key, uint8* data, size_t data_size,
    size_t *data_len, key_data_type_t data_type, int instance, bool tx)
{
	return km_key_get_data(key, data, data_size, data_len, data_type,
		instance, tx);
}

int
wlc_key_set_data_ex(wlc_key_t *key, uint8* data, size_t data_len,
    key_data_type_t data_type, int instance, bool tx)
{
	int err;

	KM_DBG_ASSERT(KEY_VALID(key));

	switch (data_type) {
	case WLC_KEY_DATA_TYPE_KEY:
		err = wlc_key_set_data(key, key->info.algo, data, data_len);
		break;
	default:
		KEY_ALGO_CB(err, key->algo_impl.cb->set_data,
			(key, data, data_len, data_type, instance, tx));
		break;
	}

	return err;
}

size_t
wlc_key_pn_to_seq(uint8 *seq, size_t seq_size, uint16 lo, uint32 hi)
{
	const size_t seq_len = sizeof(lo) + sizeof(hi);

	if (seq_size < seq_len)
		return 0;

	memset(seq, 0, seq_size);
	htol16_ua_store(lo, seq);
	htol32_ua_store(hi, seq + sizeof(lo));
	return seq_len;
}

#if defined(BCMDBG) || defined(WLMSG_WSEC)
const char*
wlc_key_get_data_type_name(wlc_key_data_type_t data_type)
{
	const char *name = "none";
	switch (data_type) {
	case WLC_KEY_DATA_TYPE_KEY: name = "key"; break;
	case WLC_KEY_DATA_TYPE_SEQ: name = "seq"; break;
	case WLC_KEY_DATA_TYPE_TKHASH_P1: name = "tkip-hash-phase1"; break;
	case WLC_KEY_DATA_TYPE_MIC_KEY_FROM_DS: name = "mic-key-from-ds"; break;
	case WLC_KEY_DATA_TYPE_MIC_KEY_TO_DS: name = "mic-key-to-ds"; break;
	case WLC_KEY_DATA_TYPE_FLAGS: name = "flags"; break;
	case WLC_KEY_DATA_TYPE_MIC_KEY: name = "mic-key"; break;
	case WLC_KEY_DATA_TYPE_TKIP_TK: name = "key"; break;
	default:
		break;
	}
	return name;
}
#endif

#ifdef BCMWAPI_WPI
/* XXX expiration is currently supported only for WAPI. this can be enabled
 * in general, if necessary. to do that some more work may be needed
 */
wlc_key_expiration_t
wlc_key_get_expiration(wlc_key_t *key)
{
	KM_DBG_ASSERT(KEY_VALID(key));
	return key->exp;
}

wlc_key_expiration_t
wlc_key_set_expiration(wlc_key_t *key, wlc_key_expiration_t exp)
{
	KM_DBG_ASSERT(KEY_VALID(key));
	KM_SWAP(wlc_key_expiration_t, exp, key->exp);
	return exp;
}
#endif /* BCMWAPI_WPI */

bool
km_key_seq_less(const uint8 *seq1, const uint8* seq2, size_t seq_len)
{
	size_t i;
	bool less = FALSE;
	for (i = seq_len; i > 0; --i) {
		if (seq1[i-1] < seq2[i-1]) {
			less = TRUE;
			break;
		} else if (seq1[i-1] > seq2[i-1])
			break;
	}
	return less;
}

bool
wlc_key_seq_less(wlc_key_t *key, const uint8 *seq, size_t seq_len,
	wlc_key_seq_id_t seq_id, bool tx)
{
	return km_key_seq_cmp(key, seq, seq_len, seq_id, tx) < 0;
}

int
wlc_key_advance_seq(wlc_key_t *key, const uint8 *seq, size_t seq_len,
	wlc_key_seq_id_t seq_id, bool tx)
{
	int err;

	err = wlc_key_set_seq(key, seq, seq_len, seq_id, tx);

	/* if iv rollback ignore, because caller asked to advance */
	if (err == BCME_REPLAY) {
		err = BCME_OK;
	}

	return err;
}

wlc_key_hw_index_t
wlc_key_get_hw_idx(wlc_key_t *key)
{
	KM_DBG_ASSERT(KEY_VALID(key));
	return key->hw_idx;
}

wlc_key_hw_index_t
km_key_set_hw_idx(wlc_key_t *key, wlc_key_hw_index_t hw_idx, bool hw_mic)
{
	KM_DBG_ASSERT(KEY_VALID(key));

	key->info.flags &= ~(WLC_KEY_FLAG_IN_HW | WLC_KEY_FLAG_HW_MIC);
	if (hw_idx != WLC_KEY_INDEX_INVALID) {
		key->info.flags |= WLC_KEY_FLAG_IN_HW;
		if (hw_mic)
			key->info.flags |= WLC_KEY_FLAG_HW_MIC;
	}

	KM_SWAP(wlc_key_hw_index_t, hw_idx, key->hw_idx);

	/* h/w index for a key changed. reset keymgmt key cache */
	KM_RESET_KEY_CACHE(KEY_KM(key));

	return hw_idx;
}

#if defined(PKTC) || defined(PKTC_DONGLE)
void
km_key_update_key_cache(wlc_key_t *key, km_key_cache_t *kc)
{
	/* note: can't use KEY_VALID - called during attach */
	KM_DBG_ASSERT((key != NULL));
	KM_DBG_ASSERT(kc != NULL);

	memset(kc, 0, sizeof(*kc));
	kc->hw_idx = key->hw_idx;
	kc->key = key;
	kc->key_info = &key->info;
}
#endif /* PKTC || PKTC_DONGLE */

int km_key_set_key_id(wlc_key_t *key, wlc_key_id_t key_id)
{
	int err = BCME_OK;

	KM_DBG_ASSERT(KEY_VALID(key));

	/* allow only pairwise key id to be set to support dynamic 802.1x/wep */
	if (!WLC_KEY_ID_IS_DEFAULT(key_id) ||
			!WLC_KEY_IS_PAIRWISE(&key->info)) {
		err = BCME_UNSUPPORTED;
		goto done;
	}

	key->info.key_id = key_id;
done:
	return err;
}
