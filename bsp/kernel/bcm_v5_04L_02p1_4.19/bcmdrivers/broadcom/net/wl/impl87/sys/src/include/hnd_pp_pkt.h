/*
 * HND Packet Pager packet operation primitives
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
 * $Id: $
 */

#ifndef _hnd_pp_pkt_h_
#define _hnd_pp_pkt_h_

#include <typedefs.h>
#include <osl_decl.h>
#include <hnd_pp_lbuf.h>
#include <hnd_pktid.h>

/* the largest reasonable packet buffer driver uses for ethernet MTU */
#define	PKTBUFSZ	(MAXPKTBUFSZ - LBUFSZ)        /**< per packet, in [bytes] */
#define	PKTFRAGSZ	(MAXPKTFRAGSZ - LBUFFRAGSZ)   /**< per packet, in [bytes] */
#define	PKTRXFRAGSZ	(MAXPKTRXFRAGSZ - LBUFFRAGSZ) /**< per packet, in [bytes] */

#define PKT_HEADROOM_DEFAULT 0

#include <hwa_export.h>

/* packet primitives */
void * hnd_pkt_get(osl_t *osh, uint len, bool send);
/** len: [nbytes] to allocate in dongle memory */
#define PKTGETLF(osh, len, send, lbuf_type)	\
	(void *)hnd_pkt_alloc(osh, len, lbuf_type)
/** allocates packet in dongle memory. len: [nbytes] to allocate. */
#define PKTGET(osh, len, send)		(void *)hnd_pkt_get(osh, len, send)
#define PKTSWAPD11BUF(osh, p)		(BCME_OK)
/* PULL size of data to reset len */
#define PKTBUFFREE(osh, p)		lfbufpool_free_buf(osh, p)
#define PKTBUFEARLYFREE(osh, p) lfbufpool_early_free_buf(osh, p)
void lfbufpool_early_free_buf(osl_t *osh, void *p);
void lfbufpool_free_buf(osl_t *osh, void *p);
#define PKTBINDD11BUF(osh, p)		(BCME_OK)
#define PKTGETHEADER(osh, data, size, lbuf_type) \
	(void *)hnd_pkt_get_header((osh), (data), (size), lbuf_type)

/** allocates packet in dongle memory. len: [nbytes] to allocate. */
#define PKTALLOC(osh, len, lbuf_type) \
	(void *)hnd_pkt_alloc(osh, len, lbuf_type)
#define PKTFREE(osh, p, send)	hnd_pkt_free(osh, p, send)
#ifdef HNDPQP
#define PQP_PKTFREE(osh, p)	hnd_pqp_pkt_free(osh, p)
#endif /* HNDPQP */
#define PKTPUT(osh, p)			hnd_pkt_put(osh, p)
#define	PKTDATA(osh, lb)		({BCM_REFERENCE(osh); LBP(lb)->control.data;})
#define	PKTSETDATA(osh, lb, x)	({BCM_REFERENCE(osh); LBP(lb)->control.data = ((uchar *)x);})
#define PKTADJDATA(osh, lb, buf, sz, h) \
	({BCM_REFERENCE(osh); lb_adj_data(LBP(lb), buf, sz, h);})

#define	PKTLEN(osh, lb)			({BCM_REFERENCE(osh); LBP(lb)->control.len;})
#define	PKTHEAD(osh, lb)		({BCM_REFERENCE(osh); LB_HEAD(LBP(lb));})
#define	PKTEND(osh, lb)			({BCM_REFERENCE(osh); LB_END(LBP(lb));})
#define	PKTHEADROOM(osh, lb) \
	({BCM_REFERENCE(osh); (LBP(lb)->control.data - lb_head(LBP(lb)));})
#define	PKTTAILROOM(osh, lb) \
	({ \
	BCM_REFERENCE(osh); \
	(lb_end(LBP(lb)) - (LBP(lb)->control.data + LBP(lb)->control.len)); \
	})
#define	PKTSETLEN(osh, lb, len)		({BCM_REFERENCE(osh); lb_setlen(LBP(lb), (len));})
#define	PKTPUSH(osh, lb, bytes)		({BCM_REFERENCE(osh); lb_push(LBP(lb), (bytes));})
#define	PKTPULL(osh, lb, bytes)		({BCM_REFERENCE(osh); lb_pull(LBP(lb), (bytes));})
#define PKTDUP(osh, p)			hnd_pkt_dup((osh), (p))
#define	PKTTAG(lb)			((void *)(&(LBP(lb))->pkttag))
#define PKTTAGCLR(lb)		lb_clear_pkttag(LBP(lb))
/* PKTPRIO save priority in control.flags */
#define	PKTPRIO(lb)			lb_pri(LBP(lb))
#define	PKTSETPRIO(lb, x)		lb_setpri(LBP(lb), (x))
#define PKTSHARED(lb)			(lb_isclone(LBP(lb)))
#define PKTALLOCED(osh)		    (((osl_t *)osh)->cmn->pktalloced)

/* checksum primitives */
#define PKTSUMNEEDED(lb)		lb_sumneeded(LBP(lb))
#define PKTSETSUMNEEDED(lb, x)		lb_setsumneeded(LBP(lb), (x))
#define PKTSUMGOOD(lb)			lb_sumgood(LBP(lb))
#define PKTSETSUMGOOD(lb, x)		lb_setsumgood(LBP(lb), (x))

/* message trace primitives */
#define PKTMSGTRACE(lb)			lb_msgtrace(LBP(lb))
#define PKTSETMSGTRACE(lb, x)		lb_setmsgtrace(LBP(lb), (x))

#define PKTDATAOFFSET(lb)		lb_dataoff(LBP(lb))
#define PKTSETDATAOFFSET(lb, dataOff)	lb_setdataoff(LBP(lb), dataOff)

#define PKTPTR(lb)			(lb)
#define PKTID(lb)			({BCM_REFERENCE(lb); 0;})
#define PKTSETID(lb, id)		({BCM_REFERENCE(lb); BCM_REFERENCE(id);})
#define	PKTNEXT(osh, lb)		({BCM_REFERENCE(osh); LBP(lb)->control.next;})
#define	PKTSETNEXT(osh, lb, x)		({BCM_REFERENCE(osh); (LBP(lb)->control.next = LBP(x));})
#define	PKTLINK(lb)			(LBP(lb)->control.link)
#define	PKTSETLINK(lb, x)		(LBP(lb)->control.link = LBP(x))
#define PKTFREELIST(lb)			PKTLINK(lb)
#define PKTSETFREELIST(lb, x)		PKTSETLINK((lb), (x))

/* packet pool */
#define PKTSETPOOL(osh, lb, x, y) \
	({BCM_REFERENCE(osh); lb_setpool(LBP(lb), (x), (y));})
#define PKTSETPOOL_TX(osh, lb, x, y) \
	({BCM_REFERENCE(osh); lb_setpool_tx(LBP(lb), (x), (y));})
#define PKTSETPOOL_RX(osh, lb, x, y) \
	({BCM_REFERENCE(osh); lb_setpool_rx(LBP(lb), (x), (y));})
#define PKTPOOL(osh, lb)		({BCM_REFERENCE(osh); lb_pool(LBP(lb));})
#ifdef BCMDBG_POOL
#define PKTPOOLSTATE(lb)		lb_poolstate(LBP(lb))
#define PKTPOOLSETSTATE(lb, s)		lb_setpoolstate(LBP(lb), (s))
#endif

#define PKTALTINTF(lb)			lb_altinterface(LBP(lb))
#define PKTSETALTINTF(lb, x)		lb_setaltinterface(LBP(lb), (x))

/* BCM_DMAPAD not supported if DMA bulk processing is enabled. */
#if defined(DMA_BULK_PKTLIST) && defined(BCM_DMAPAD)
#error "DMA BULK Processing (DMA_BULK_PKTLIST) incompatible with (BCM_DMAPAD) feature"
#endif

/* This seems to be in use only for SDIO devices.  Useless, remove it. */
//#define PKTDMAPAD(osh, lb)		({BCM_REFERENCE(osh); (LBP(lb)->control.dmapad);})
//#define PKTSETDMAPAD(osh, lb, pad)	({BCM_REFERENCE(osh); (LBP(lb)->control.dmapad = (pad));})
#define PKTDMAPAD(osh, lb)		do {} while (0)

#define PKTDMAIDX(osh, lb)		({BCM_REFERENCE(osh); (LBP(lb)->control.dma_index);})
#define PKTSETDMAIDX(osh, lb, idx) \
	({BCM_REFERENCE(osh); (LBP(lb)->control.dma_index = (idx));})

#define PKTRXCPLID(osh, lb)		({BCM_REFERENCE(osh); (LBP(lb)->control.rxcpl_id);})
#define PKTSETRXCPLID(osh, lb, id) \
	({BCM_REFERENCE(osh); (LBP(lb)->control.rxcpl_id = (id));})
#define PKTRESETRXCPLID(osh, lb)	PKTSETRXCPLID(osh, lb, 0)

#define PKTSETNODROP(osh, lb)		({BCM_REFERENCE(osh); lb_setnodrop(LBP(lb));})
#define PKTNODROP(osh, lb)		({BCM_REFERENCE(osh); lb_nodrop(LBP(lb));})

#define PKTSETTYPEEVENT(osh, lb)	({BCM_REFERENCE(osh); lb_settypeevent(LBP(lb));})
#define PKTTYPEEVENT(osh, lb)		({BCM_REFERENCE(osh); lb_typeevent(LBP(lb));})

#define PKT_SET_DOT3(osh, lb)		({BCM_REFERENCE(osh); lb_set_dot3_pkt(LBP(lb));})
#define PKT_DOT3(osh, lb)		({BCM_REFERENCE(osh); lb_dot3_pkt(LBP(lb));})

#define PKTLIST_DUMP(osh, buf)		BCM_REFERENCE(osh)

#define PKTFRMNATIVE(osh, lb)		((void *)(hnd_pkt_frmnative((osh), (lb))))
#define PKTTONATIVE(osh, p)		((struct lbuf *)(hnd_pkt_tonative((osh), (p))))

#define PKTSET80211(lb)			lb_set80211pkt(LBP(lb))
#define PKT80211(lb)			lb_80211pkt(LBP(lb))

#ifdef WL_MONITOR
#define PKTSETMON(lb)			lb_setmonpkt(LBP(lb))
#define PKTMON(lb)			lb_monpkt(LBP(lb))
#endif /* WL_MONITOR */

#define PKTIFINDEX(osh, lb)		(LBP(lb)->control.ifid)
#define PKTSETIFINDEX(osh, lb, idx)	(LBP(lb)->control.ifid = (idx))

#define PKTFLOWIDOVERRIDE(osh, lb)		(LBP(lb)->control.txpost.flowid_override)
#define PKTSETFLOWIDOVERRIDE(osh, lb, idx)	(LBP(lb)->control.txpost.flowid_override = (idx))

/* These macros used to set/get a 32-bit value in the pkttag */
#define PKTTAG_SET_VALUE(lb, val)	(*((uint32*)PKTTAG(lb)) = (uint32)val)
#define PKTTAG_GET_VALUE(lb)		(*((uint32*)PKTTAG(lb)))

#define PKTISBUFALLOC(osh, lb)		({BCM_REFERENCE(osh); lb_is_buf_alloc(LBP(lb));})
#define PKTSETBUFALLOC(osh, lb)		({BCM_REFERENCE(osh); lb_set_buf_alloc(LBP(lb));})
#define PKTRESETBUFALLOC(osh, lb)	({BCM_REFERENCE(osh); lb_reset_buf_alloc(LBP(lb));})

#define PKTISRXCORRUPTED(osh, lb)	({BCM_REFERENCE(osh); lb_is_rx_corrupted(LBP(lb));})
#define PKTSETRXCORRUPTED(osh, lb)	({BCM_REFERENCE(osh); lb_set_rx_corrupted(LBP(lb));})
#define PKTRESETRXCORRUPTED(osh, lb)	({BCM_REFERENCE(osh); lb_reset_rx_corrupted(LBP(lb));})

/* +===== start hwa related definitions =====+ */
#define PKTINIT(osh, lb, type, sz)	({BCM_REFERENCE(osh); lb_init(LBP(lb), type, sz);})

#define PKTISHWAPKT(osh, lb)		({BCM_REFERENCE(osh); lb_is_hwa_pkt(LBP(lb));})
#define PKTSETHWAPKT(osh, lb)		({BCM_REFERENCE(osh); lb_set_hwa_pkt(LBP(lb));})
#define PKTRESETHWAPKT(osh, lb)		({BCM_REFERENCE(osh); lb_reset_hwa_pkt(LBP(lb));})

#define PKTISHWA3BPKT(osh, lb)		({BCM_REFERENCE(osh); lb_is_hwa_3bpkt(LBP(lb));})
#define PKTSETHWA3BPKT(osh, lb)		({BCM_REFERENCE(osh); lb_set_hwa_3bpkt(LBP(lb));})
#define PKTRESETHWA3BPKT(osh, lb)	({BCM_REFERENCE(osh); lb_reset_hwa_3bpkt(LBP(lb));})

#define PKTISHWAHOSTREORDER(osh, lb)	({BCM_REFERENCE(osh); lb_is_hwa_hostreorder(LBP(lb));})
#define PKTSETHWAHOSTREORDER(osh, lb)	({BCM_REFERENCE(osh); lb_set_hwa_hostreorder(LBP(lb));})
#define PKTRESETHWAHOSTREORDER(osh, lb) \
	({BCM_REFERENCE(osh); lb_reset_hwa_hostreorder(LBP(lb));})

#define PKTISMGMTTXPKT(osh, lb)		({BCM_REFERENCE(osh); lb_is_mgmt_tx_pkt(LBP(lb));})
#define PKTSETMGMTTXPKT(osh, lb)	({BCM_REFERENCE(osh); lb_set_mgmt_tx_pkt(LBP(lb));})
#define PKTRESETMGMTTXPKT(osh, lb)	({BCM_REFERENCE(osh); lb_reset_mgmt_tx_pkt(LBP(lb));})

#define PKTDATAISLOCAL(osh, lb)		({BCM_REFERENCE(osh); lb_data_is_local(LBP(lb));})

#define PKTRESETFLAGS(osh, lb)		(LBP(lb)->control.flags = 0)

/* +===== end hwa related definitions =====+ */

#define PKTCOPYFLAGS(osh, lb1, lb2) \
	({BCM_REFERENCE(osh); LBP(lb1)->control.flags = LBP(lb2)->control.flags;})
#define PKTFLAGS(lb)				(LBP(lb)->control.flags)

/* Lbuf with fraginfo */
/**
 * A fraginfo is an array of host memory fragments. One packet on the host may need more than one
 * fraginfo, so fraginfo can be chained. This means that e.g. PKTFRAGTOTLEN() is not guaranteed to
 * return the sum of all fragments of a host packet, use the functions for this instead.
 */
#define PKTFRAGPKTID(osh, lb)		(LBFP(lb)->fraginfo.host_pktid[0])
#define PKTSETFRAGPKTID(osh, lb, id)	(LBFP(lb)->fraginfo.host_pktid[0] = (id))
#define PKTHOSTPKTID(lb, fi)		(LBFP(lb)->fraginfo.host_pktid[fi])
#define PKTSETHOSTPKTID(lb, fi, id)	(LBFP(lb)->fraginfo.host_pktid[fi] = (id))

#define PKTSAVEHEADROOM(osh, lb, sz)	(PPLBUF(lb)->headroom = (sz))
#define PKTGETHEADROOM(osh, lb)		(PPLBUF(lb)->headroom)

#define PKTHMELEN(osh, lb)	(LBFP(lb)->fraginfo.host_datalen[LB_FRAG3])
#define PKTSETHMELEN(osh, lb, len) (LBFP(lb)->fraginfo.host_datalen[LB_FRAG3] = (len))
#define PKTHME_LO(osh, lb)	(LBFP(lb)->fraginfo.data_buf_haddr64[LB_FRAG3].lo)
#define PKTSETHME_LO(osh, lb, addr) \
	(LBFP(lb)->fraginfo.data_buf_haddr64[LB_FRAG3].lo = (addr))
#define PKTHME_HI(osh, lb)	(LBFP(lb)->fraginfo.data_buf_haddr64[LB_FRAG3].hi)
#define PKTSETHME_HI(osh, lb, addr) \
	(LBFP(lb)->fraginfo.data_buf_haddr64[LB_FRAG3].hi = (addr))

/* Total number of MSDUs in current TxLfrag/RxLfrag */
#define PKTFRAGTOTNUM(osh, lb)		(LBFP(lb)->fraginfo.frag_num)
#define PKTSETFRAGTOTNUM(osh, lb, tot)	(LBFP(lb)->fraginfo.frag_num = (tot))

/* In tx path, total fragments len */
#define PKTFRAGTOTLEN(osh, lb)		(LBFP(lb)->fraginfo.frag_len)
#define PKTSETFRAGTOTLEN(osh, lb, len)	(LBFP(lb)->fraginfo.frag_len = (len))

/* In tx path, host address/len op */
#define	PKTFRAGPUSH(osh, lb, ix, bytes) \
	({BCM_REFERENCE(osh); lb_fragpush(LBP(lb), (ix), (bytes));})
#define	PKTFRAGPULL(osh, lb, ix, bytes) \
	({BCM_REFERENCE(osh); lb_fragpull(LBP(lb), (ix), (bytes));})

#define PKTFRAGFLOWRINGID(osh, lb)	(LBFP(lb)->fraginfo.tx.flowring_id)
#define PKTSETFRAGFLOWRINGID(osh, lb, ring) \
	(LBFP(lb)->fraginfo.tx.flowring_id = (ring))

/* in rx path, reuse totlen as used len */
#define PKTFRAGUSEDLEN(osh, lb)		(LBFP(lb)->fraginfo.frag_len)
#define PKTSETFRAGUSEDLEN(osh, lb, len)	(LBFP(lb)->fraginfo.frag_len = (len))
#define PKTFRAGLEN(osh, lb, ix)		(LBFP(lb)->fraginfo.host_datalen[ix])
#define PKTSETFRAGLEN(osh, lb, ix, len) (LBFP(lb)->fraginfo.host_datalen[ix] = (len))
#define PKTFRAGDATA_LO(osh, lb, ix)	(LBFP(lb)->fraginfo.data_buf_haddr64[ix].lo)
#define PKTSETFRAGDATA_LO(osh, lb, ix, addr) \
	(LBFP(lb)->fraginfo.data_buf_haddr64[ix].lo = (addr))
#define PKTFRAGDATA_HI(osh, lb, ix)	(LBFP(lb)->fraginfo.data_buf_haddr64[ix].hi)
#define PKTSETFRAGDATA_HI(osh, lb, ix, addr) \
	(LBFP(lb)->fraginfo.data_buf_haddr64[ix].hi = (addr))

#define PKTFLAGSEXT(osh, lb)		(LBP(lb)->fraginfo.flags)
#define PKTRESETFLAGSEXT(osh, lb)	(LBP(lb)->fraginfo.flags = 0)

// SET
#define PKTSETFIFO0INT(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags |= LB_FIFO0_INT;})
#define PKTSETFIFO1INT(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags |= LB_FIFO1_INT;})
#define PKTSETHDRCONVTD(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags |= LB_HDR_CONVERTED;})
#define PKTSETTXSPROCESSED(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags |= LB_TXS_PROCESSED;})
#define PKTSETTXTSINSERTED(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags |= LB_TXTS_INSERTED;})
#define PKTSETTXSHOLD(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags |= LB_TXS_HOLD;})

// RESET
#define PKTRESETFIFO0INT(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags &= ~LB_FIFO0_INT;})
#define PKTRESETFIFO1INT(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags &= ~LB_FIFO1_INT;})
#define PKTRESETHDRCONVTD(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags &= ~LB_HDR_CONVERTED;})
#define PKTRESETTXSPROCESSED(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags &= ~LB_TXS_PROCESSED;})
#define PKTRESETTXTSINSERTED(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags &= ~LB_TXTS_INSERTED;})
#define PKTRESETTXSHOLD(osh, lb) \
	({BCM_REFERENCE(osh); LBFP(lb)->fraginfo.flags &= ~LB_TXS_HOLD;})

// ISSET
#define PKTISFIFO0INT(osh, lb) \
	({BCM_REFERENCE(osh); (LBFP(lb)->fraginfo.flags & LB_FIFO0_INT) ? 1 : 0;})
#define PKTISFIFO1INT(osh, lb) \
	({BCM_REFERENCE(osh); (LBFP(lb)->fraginfo.flags & LB_FIFO1_INT) ? 1 : 0;})
#define PKTISHDRCONVTD(osh, lb) \
	({BCM_REFERENCE(osh); (LBFP(lb)->fraginfo.flags & LB_HDR_CONVERTED) ? 1 : 0;})
#define PKTISTXSPROCESSED(osh, lb) \
	({BCM_REFERENCE(osh); (LBFP(lb)->fraginfo.flags & LB_TXS_PROCESSED) ? 1 : 0;})
#define PKTISTXTSINSERTED(osh, lb) \
	({BCM_REFERENCE(osh); (LBFP(lb)->fraginfo.flags & LB_TXTS_INSERTED) ? 1 : 0;})
#define PKTISTXSHOLD(osh, lb) \
	({BCM_REFERENCE(osh); (LBFP(lb)->fraginfo.flags & LB_TXS_HOLD) ? 1 : 0;})

#if defined(PROP_TXSTATUS)
// PKTFLAGS
#define PKTFRAGPKTFLAGS(osh, lb) \
	({BCM_REFERENCE(osh); (PPLBUF(lb)->pktflags & 0xF0) >> 4;})
#define PKTFRAGRESETPKTFLAGS(osh, lb) \
	({BCM_REFERENCE(osh); PPLBUF(lb)->pktflags &= 0xF;})
#define PKTFRAGSETPKTFLAGS(osh, lb, flags) \
	({BCM_REFERENCE(osh); PPLBUF(lb)->pktflags |= (flags << 4);})

/* WLFC SEQ */
#define PKTWLFCSEQ(osh, lb)	(PPLBUF(lb)->fc_seq)
#define PKTSETWLFCSEQ(osh, lb, seq) \
	({BCM_REFERENCE(osh); PPLBUF(lb)->fc_seq = (seq);})

/* FC TLV */
#define PKTFRAGFCTLV(osh, lb)	(PPLBUF(lb)->fc_tlv)

#endif /* PROP_TXSTATUS */

/* TxStatus */
#define PKTFRAGTXSTATUS(osh, lb)  (LBP(lb)->control.txstatus)
#define PKTFRAGSETTXSTATUS(osh, lb, txs) \
	({BCM_REFERENCE(osh); LBP(lb)->control.txstatus = (txs);})

/* RX FRAG */
#define PKTISRXFRAG(osh, lb)		({BCM_REFERENCE(osh); lb_is_rxfrag(LBP(lb));})
#define PKTSETRXFRAG(osh, lb)		({BCM_REFERENCE(osh); lb_set_rxfrag(LBP(lb));})
#define PKTRESETRXFRAG(osh, lb)		({BCM_REFERENCE(osh); lb_reset_rxfrag(LBP(lb));})
#define PKTISPKTFETCHED(osh, lb)	({BCM_REFERENCE(osh); lb_is_pktfetched(LBP(lb));})
#define PKTSETPKTFETCHED(osh, lb)	({BCM_REFERENCE(osh); lb_set_pktfetched(LBP(lb));})
#define PKTRESETPKTFETCHED(osh, lb)	({BCM_REFERENCE(osh); lb_reset_pktfetched(LBP(lb));})

/* TX FRAG */
#define PKTISTXFRAG(osh, lb)		({BCM_REFERENCE(osh); lb_is_txfrag(LBP(lb));})
#define PKTSETTXFRAG(osh, lb)		({BCM_REFERENCE(osh); lb_set_txfrag(LBP(lb));})
#define PKTRESETTXFRAG(osh, lb)		({BCM_REFERENCE(osh); lb_reset_txfrag(LBP(lb));})

/** Cache Flow Processing Packet Macros */
#define PKTISCFP(lb)                ({lb_is_cfp(LBP(lb));})
#define PKTGETCFPFLOWID(lb)         ({lb_get_cfp_flowid(LBP(lb));})
#define PKTSETCFPFLOWID(lb, cfp_flowid) ({lb_set_cfp_flowid(LBP(lb), cfp_flowid);})
#define PKTCLRCFPFLOWID(lb, cfp_flowid) ({lb_clr_cfp_flowid(LBP(lb), cfp_flowid);})

/* FIFO index storage access */
#define PKTGETFIFOIDX(lb)			({lb_get_fifo_idx(LBP(lb));})
#define PKTSETFIFOIDX(lb, fifo)		({lb_set_fifo_idx(LBP(lb), (fifo));})
#define PKTCLRFIFOIDX(lb, fifo)		({lb_clr_fifo_idx(LBP(lb), (fifo));})

/* Forwarded pkt indication */
#define PKTISFRWDPKT(osh, lb)		({BCM_REFERENCE(osh); lb_is_frwd_pkt(LBP(lb));})
#define PKTSETFRWDPKT(osh, lb)		({BCM_REFERENCE(osh); lb_set_frwd_pkt(LBP(lb));})
#define PKTRESETFRWDPKT(osh, lb)	({BCM_REFERENCE(osh); lb_reset_frwd_pkt(LBP(lb));})

/* Need Rx completion used for AMPDU reordering */
#define PKTNEEDRXCPL(osh, lb)		({BCM_REFERENCE(osh); lb_need_rxcpl(LBP(lb));})
#define PKTSETNORXCPL(osh, lb)		({BCM_REFERENCE(osh); lb_set_norxcpl(LBP(lb));})
#define PKTRESETNORXCPL(osh, lb)	({BCM_REFERENCE(osh); lb_clr_norxcpl(LBP(lb));})

/* TX FRAG is using heap allocated buffer */
#define PKTHASHEAPBUF(osh, lb)		({BCM_REFERENCE(osh); lb_has_heapbuf(LBP(lb));})
#define PKTSETHEAPBUF(osh, lb)		({BCM_REFERENCE(osh); lb_set_heapbuf(LBP(lb));})
#define PKTRESETHEAPBUF(osh, lb)	({BCM_REFERENCE(osh); lb_clr_heapbuf(LBP(lb));})

#define PKTFRAGRINGINDEX(osh, lb)	(LBFP(lb)->fraginfo.tx.rd_idx)

#if defined(PROP_TXSTATUS)
/* flow read index storage access */
#define PKTFRAGSETRINGINDEX(osh, lb, idx) \
	(LBFP(lb)->fraginfo.tx.rd_idx = (idx))

/* AMSDU in single TxLfrag, fi < 3 */
#define PKTRDIDX(lb, fi)		(PPLBUF(lb)->rd_idx[fi])
#define PKTSETRDIDX(lb, fi, id)		(PPLBUF(lb)->rd_idx[fi] = (id))
#endif /* PROP_TXSTATUS */

#define PKTISFRAG(osh, lb)		({BCM_REFERENCE(osh); lb_is_frag(LBP(lb));})

/* Should not use */
#define PKTFRAGISCHAINED(osh, i)	({ASSERT(0); (FALSE);})

/* packet has metadata */
#define PKTHASMETADATA(osh, lb) \
	({BCM_REFERENCE(osh); lb_has_metadata((struct lbuf *)lb);})
#define PKTSETHASMETADATA(osh, lb) \
	({BCM_REFERENCE(osh); lb_set_has_metadata((struct lbuf *)lb);})
#define PKTRESETHASMETADATA(osh, lb)\
	({BCM_REFERENCE(osh); lb_reset_has_metadata((struct lbuf *)lb);})

#define PKTSETBUF(osh, lb, buf, n) \
	({BCM_REFERENCE(osh); lb_set_buf((struct lbuf *)lb, buf, n);})
#define PKTSETTXBUF(osh, lb, buf, n) \
	({BCM_REFERENCE(osh); lb_set_txbuf((struct lbuf *)lb, buf, n);})
#define PKTSETTXBUFRANGE(osh, lb, buf, n) ({ \
	BCM_REFERENCE(osh); \
	PPLBUF(lb)->head = (uchar *)(buf); \
	PPLBUF(lb)->end = PPLBUF(lb)->head + (n); \
})
#define PKTSETRXBUFRANGE(osh, lb, buf, n) ({ \
	BCM_REFERENCE(osh); \
	LBFP(lb)->fraginfo.head = (uchar *)(buf); \
	LBFP(lb)->fraginfo.end = LBFP(lb)->fraginfo.head + (n); \
})

#ifdef PKTC_DONGLE
#define	PKTCSETATTR(s, f, p, b)	BCM_REFERENCE(s)
#define	PKTCCLRATTR(s)		BCM_REFERENCE(s)
#define	PKTCGETATTR(s)		({BCM_REFERENCE(s); 0;})
#define	PKTCCNT(lb)		({BCM_REFERENCE(lb); 0;})
#define	PKTCLEN(lb)		({BCM_REFERENCE(lb); 0;})
#define	PKTCGETFLAGS(lb)	({BCM_REFERENCE(lb); 0;})
#define	PKTCSETFLAGS(lb, f)	BCM_REFERENCE(lb)
#define	PKTCCLRFLAGS(lb)	BCM_REFERENCE(lb)
#define	PKTCFLAGS(lb)		({BCM_REFERENCE(lb); 0;})
#define	PKTCSETCNT(lb, c)	BCM_REFERENCE(lb)
#define	PKTCINCRCNT(lb)		BCM_REFERENCE(lb)
#define	PKTCADDCNT(lb, c)	BCM_REFERENCE(lb)
#define	PKTCSETLEN(lb, l)	BCM_REFERENCE(lb)
#define	PKTCADDLEN(lb, l)	BCM_REFERENCE(lb)
#define	PKTCSETFLAG(lb, fb)	BCM_REFERENCE(lb)
#define	PKTCCLRFLAG(lb, fb)	BCM_REFERENCE(lb)
#define	PKTCLINK(lb)		PKTLINK(lb)
#define	PKTSETCLINK(lb, x)	PKTSETLINK(lb, x)
#define	FOREACH_CHAINED_PKT(lb, nlb) \
	for (; (lb) != NULL; (lb) = (nlb)) \
		if ((nlb) = PKTCLINK(lb), PKTSETCLINK((lb), NULL), 1)
#define	PKTCFREE(osh, lb, send) \
	do {			 \
		void *nlb;	 \
		ASSERT((lb) != NULL);	   \
		FOREACH_CHAINED_PKT((lb), nlb) {	\
			PKTFREE((osh), (struct lbuf *)(lb), (send));	\
		}					\
	} while (0)
#define PKTCENQTAIL(h, t, p) \
	do {		     \
		if ((t) == NULL) {		\
			(h) = (t) = (p);	\
		} else {			\
			PKTSETCLINK((t), (p));	\
			(t) = (p);		\
		}				\
	} while (0)
#endif /* PKTC_DONGLE */

#ifdef PKTC_TX_DONGLE
#define	PKTSETCHAINED(o, lb)	({BCM_REFERENCE(o); lb_setchained(LBP(lb));})
#define	PKTISCHAINED(lb)	lb_ischained(LBP(lb))
#define	PKTCLRCHAINED(o, lb)	({BCM_REFERENCE(o); lb_clearchained(LBP(lb));})
#endif /* PKTC_TX_DONGLE */

#define PKTPPBUFFERP(lb)		((void *)(PPLBUF(lb)->databuf_txfrag))

#define RPH_HOSTPKTID(lb)           (LBFP(lb)->fraginfo.host_pktid[0])
#define RPH_HOSTDATALEN(lb)         (LBFP(lb)->fraginfo.host_datalen[0])
#define RPH_HOSTADDR64(lb)          (LBFP(lb)->fraginfo.data_buf_haddr64[0])

#define PKTSETHMEDATA(osh, lb)		({BCM_REFERENCE(osh); lb_set_hme_data(LBP(lb));})
#define PKTHASHMEDATA(osh, lb)		({BCM_REFERENCE(osh); lb_has_hme_data(LBP(lb));})
#define PKTRESETHMEDATA(osh, lb)	({BCM_REFERENCE(osh); lb_reset_hme_data(LBP(lb));})

#define PKTISHWADDMBPKT(osh, lb)		({BCM_REFERENCE(osh); lb_is_hwa_ddbm_pkt(LBP(lb));})
#define PKTSETHWADDMBPKT(osh, lb)	({BCM_REFERENCE(osh); lb_set_hwa_ddbm_pkt(LBP(lb));})
#define PKTRESETHWADDMBPKT(osh, lb)	({BCM_REFERENCE(osh); lb_reset_hwa_ddbm_pkt(LBP(lb));})

#define	PKTMAPID(lb)				(LBP(lb)->control.pkt_mapid)

#define PKTLOCAL(osh, lb)		(LBFP(lb)->fraginfo.host_pktid[0])
#define PKTSETLOCAL(osh, lb, x)		(LBFP(lb)->fraginfo.host_pktid[0] = (uintptr)(x))
#define PKTRESETLOCAL(osh, lb)		(LBFP(lb)->fraginfo.host_pktid[0] = 0)

/* MGMT TX PKT flag information of lfrag */
#define LB_MGMT_TX_LOCALFIXUP		0x01
#define LB_MGMT_TX_PQPHEAPPOOL		0x02

#define PKTNEEDLOCALFIXUP(osh, lb) \
	(LBFP(lb)->fraginfo.host_datalen[LB_FRAG4] & LB_MGMT_TX_LOCALFIXUP)
#define PKTSETLOCALFIXUP(osh, lb) \
	(LBFP(lb)->fraginfo.host_datalen[LB_FRAG4] |= LB_MGMT_TX_LOCALFIXUP)
#define PKTRESETLOCALFIXUP(osh, lb) \
	(LBFP(lb)->fraginfo.host_datalen[LB_FRAG4] &= ~(LB_MGMT_TX_LOCALFIXUP))

#define PKTHASPQPHEAPPOOL(osh, lb) \
	(LBFP(lb)->fraginfo.host_datalen[LB_FRAG4] & LB_MGMT_TX_PQPHEAPPOOL)
#define PKTSETPQPHEAPPOOL(osh, lb) \
	(LBFP(lb)->fraginfo.host_datalen[LB_FRAG4] |= LB_MGMT_TX_PQPHEAPPOOL)
#define PKTRESETPQPHEAPPOOL(osh, lb) \
	(LBFP(lb)->fraginfo.host_datalen[LB_FRAG4] &= ~(LB_MGMT_TX_PQPHEAPPOOL))

#define PKTMGMTDDBMPKT(osh, lb)		(LBFP(lb)->fraginfo.host_pktid[LB_FRAG4])
#define PKTSETMGMTDDBMPKT(osh, lb, x)	(LBFP(lb)->fraginfo.host_pktid[LB_FRAG4] = (uintptr)(x))
#define PKTRESETMGMTDDBMPKT(osh, lb)	(LBFP(lb)->fraginfo.host_pktid[LB_FRAG4] = 0)

#define PKTISSWPQPPKT(osh, lb)		({BCM_REFERENCE(osh); lb_is_swpqp_pkt(LBP(lb));})
#define PKTSETSWPQPPKT(osh, lb)		({BCM_REFERENCE(osh); lb_set_swpqp_pkt(LBP(lb));})
#define PKTRESETSWPQPPKT(osh, lb)	({BCM_REFERENCE(osh); lb_reset_swpqp_pkt(LBP(lb));})

extern void * hnd_pkt_frmnative(osl_t *osh, struct lbuf *lb);
extern struct lbuf * hnd_pkt_tonative(osl_t *osh, void *p);
extern void * hnd_pkt_get_header(osl_t *osh, void *data, uint len, enum lbuf_type lbuf_type);
extern void * hnd_pkt_alloc(osl_t *osh, uint len, enum lbuf_type lbuf_type);
extern void hnd_pkt_free(osl_t *osh, void *p, bool send);
#ifdef HNDPQP
extern void hnd_pqp_pkt_free(osl_t *osh, void *p);
#endif /* HNDPQP */
extern void hnd_pkt_put(osl_t *osh, void *p);
extern void * hnd_pkt_dup(osl_t *osh, void *p);
extern void * hnd_pkt_clone(osl_t *osh, void *p, int offset, int len);
extern uchar * hnd_pkt_params(osl_t *osh, void **p, uint32 *len, uint32 *fragix,
	uint16 *ftot, uint8* fbuf, uint32 *lo_addr, uint32 *hi_addr);

#endif /* _hnd_pp_pkt_h_ */
