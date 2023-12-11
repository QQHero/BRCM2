/*
 * HND Packet Pager packet buffer routines.
 *
 * No caching,
 * Just a thin packet buffering data structure layer atop malloc/free .
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

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <hnd_pp_lbuf.h>
#include <hwa_export.h>
#ifdef HNDPQP
#include <hnd_pqp.h>
#endif

#if defined(BCMDBG) || defined(BCMDBG_MEMFAIL)
#define LBUF_MSG(x) printf x
#else
#define LBUF_MSG(x)
#endif

#define LBUF_BIN_SIZE	6

#if defined(BCMLFRAG) && !defined(BCMLFRAG_DISABLED)
bool _bcmlfrag = TRUE;
#else
bool _bcmlfrag = FALSE;
#endif

typedef struct {
	lbuf_free_cb_t cb;
	void* arg;
} lbuf_freeinfo_t;

static lbuf_freeinfo_t lbuf_info;

static lbuf_freeinfo_t* lbuf_info_get(void);

static lbuf_freeinfo_t*
BCMRAMFN(lbuf_info_get)(void)
{
	return (&lbuf_info);
}

void lbuf_free_register(lbuf_free_cb_t cb, void* arg)
{
	lbuf_freeinfo_t *lbuf_infoptr;

	lbuf_infoptr = lbuf_info_get();

	lbuf_infoptr->cb = cb;
	lbuf_infoptr->arg = arg;
}

static const uint lbsize[LBUF_BIN_SIZE] = {
	MAXPKTBUFSZ >> 4,
	MAXPKTBUFSZ >> 3,
	MAXPKTBUFSZ >> 2,
	MAXPKTBUFSZ >> 1,
	MAXPKTBUFSZ,
	4096 + LBUFSZ		/* ctrl queries on bus can be 4K */
};

#if !defined(BCMPKTPOOL) || defined(BCMDBG)
static uint lbuf_hist[LBUF_BIN_SIZE] = {0};
#endif
static uint lbuf_allocfail = 0;

#if defined(BCMPCIEDEV) && !defined(BME_PKTFETCH)
static lbuf_free_global_cb_t lbuf_free_cb = NULL;
#endif /* BCMPCIEDEV && !BME_PKTFETCH */

void
lb_set_buf(struct lbuf *lb, void *buf, uint size)
{
	if (lb->control.flags & LBF_TX_FRAG) {
		LB_HEAD_B(lb) = (uchar *)buf;
		LB_END_B(lb) = LB_HEAD_B(lb) + size;
	} else {
		LB_HEAD_F(lb) = (uchar *)buf;
		LB_END_F(lb) = LB_HEAD_F(lb) + size;
	}
	lb->control.data = (uchar *)buf;
	lb->control.len = size;
}

void
lb_set_txbuf(struct lbuf *lb, void *buf, uint size)
{
	LB_HEAD_B(lb) = (uchar *)buf;
	LB_END_B(lb) = LB_HEAD_B(lb) + size;
	lb->control.data = (uchar *)buf;
	lb->control.len = size;
}

/* Set flags and size according to pkt type */
static void
set_lbuf_params(enum lbuf_type lbuf_type, uint32* flags, uint* lbufsz)
{
	switch (lbuf_type) {
	case lbuf_basic:
		/* legacy pkt */
		*flags = 0;
		*lbufsz = LBUFSZ;
		break;
	case lbuf_frag:
		/* tx frag */
		*flags = LBF_TX_FRAG;
		*lbufsz = LBUFFRAGSZ;
		break;
	case lbuf_rxfrag:
		/* rx frag */
		*flags = 0;
		*lbufsz = LBUFFRAGSZ;
		break;
	case lbuf_mgmt_tx:
		/* legacy mgmt pkt */
		*flags = (LBF_HWA_PKT | LBF_MGMT_TX_PKT);
		*lbufsz = LBUFMGMTSZ;
		break;
	default:
		break;
	}
}

struct lbuf *
lb_init(struct lbuf *lb, enum lbuf_type lb_type, uint lb_sz)
{
	return NULL;
}

struct lbuf *
#if defined(BCMDBG_MEMFAIL)
lb_alloc_header(void *data, uint size, enum lbuf_type lbuf_type, void* call_site)
#else
lb_alloc_header(void *data, uint size, enum lbuf_type lbuf_type)
#endif
{
	printf("%s: HWA doesn't handle it!\n", __FUNCTION__);
	ASSERT(0);

	lbuf_allocfail++;
	return NULL;
}

struct lbuf *
#if defined(BCMDBG_MEMFAIL)
lb_alloc(uint size, enum lbuf_type lbuf_type, void *call_site)
#else
/**
 * @param size        size of packet buffer to allocate in dongle memory in [bytes]
 * @param lbuf_type   e.g. 'lbuf_frag'
 */
lb_alloc(uint size, enum lbuf_type lbuf_type)
#endif
{
	uchar * head;
	struct lbuf *lb;
	uint tot;     /**< [bytes] of dongle memory to allocate for lbuf+pktbuffer */
	uint lbufsz;  /**< LBUFFRAGSZ or LBUFSZ */
	uint end_off;
	uint32 flags;
	uint align_bits;

	tot = 0;
	flags = 0;
	lbufsz = 0; /* assumes e.g. LBUFFRAGSZ */
	align_bits = 0;

	/* get lbuf params based on type */
	set_lbuf_params(lbuf_type, &flags, &lbufsz);

#ifdef HNDPQP
	// Check PQP HBM credit
	if ((lbuf_type == lbuf_mgmt_tx) && (pqp_hbm_avail() <= 0)) {
		LBUF_MSG(("lb_alloc: There is no available PQP HBM credit(%d)\n",
			pqp_hbm_avail()));
		goto error;
	}
#endif /* HNDPQP */

#ifdef BCMPKTPOOL
	/* Don't roundup size if PKTPOOL enabled */
	tot = lbufsz + ROUNDUP(size, sizeof(int));
	if (tot > lbsize[ARRAYSIZE(lbsize)-1]) {
		LBUF_MSG(("lb_alloc: size too big (%u); alloc failed;\n",
		       (lbufsz + size)));
		goto error;
	}
#else
	{
		int i;
		for (i = 0; i < ARRAYSIZE(lbsize); i++) {
			if ((lbufsz + ROUNDUP(size, sizeof(int))) <= lbsize[i]) {
				tot = lbsize[i];
				lbuf_hist[i]++;
				break;
			}
		}
	}
#endif /* BCMPKTPOOL */

	if (tot == 0) {
		LBUF_MSG(("lb_alloc: size too big (%u); alloc failed;\n",
		       (lbufsz + size)));
		goto error;
	}

	/* fastdma requeirement */
#if HWACONF_GE(131)
	align_bits = 3; /* 8 bytes */
#endif

#if defined(BCMDBG_MEMFAIL)
	if ((lb = MALLOC_ALIGN_CALLSITE(OSH_NULL, tot, align_bits, call_site)) == NULL) {
		LBUF_MSG(("lb_alloc: size (%u); alloc failed; called from 0x%p\n", (lbufsz + size),
			call_site));
		goto error;
	}
#else
	if ((lb = MALLOC_ALIGN(OSH_NULL, tot, align_bits)) == NULL) {
		LBUF_MSG(("lb_alloc: size (%u); alloc failed;\n", (lbufsz + size)));
		goto error;
	}
#endif

	bzero(lb, lbufsz);
	ASSERT(ISALIGNED(lb, sizeof(int)));

	// Set head and end for non-DDBM.
	head = (uchar*)((uchar*)lb + lbufsz);
	end_off = (tot - lbufsz);
	lb->control.data = (head + end_off) - ROUNDUP(size, sizeof(int));
	lb->control.len = size;
	lb->control.flags = flags;
	lb_set_head(lb, head);
	lb_set_end(lb, head + end_off);

#ifdef HNDPQP
	// Update PQP HBM credit
	if (lbuf_type == lbuf_mgmt_tx) {
		pqp_hbm_avail_sub(1);
	}
#endif /* HNDPQP */

	return (lb);

error:
	lbuf_allocfail++;
	return NULL;
} /* lb_alloc */

static void
lb_free_one(struct lbuf *lb)
{
	ASSERT(lb_sane(lb));

	/* It's difficult to implement CLONE, see lb_clone. */
	ASSERT(!lb_isclone(lb));

	/* No refcnt attribute anymore. */

	/* Who will use lb_pool ?  lb_pool means a lb is from SW pool,
	 * i.e. SHARED_POOL, SHARED_FRAG_POOL, RESV_FRAG_POOL, SHARED_RXFRAG_POOL
	 * We also have buffer/data pool,
	 * i.e. D3_LFRAG_BUF_POOL, D11_LFRAG_BUF_POOL
	 * One D11 buffer size is PKTFRAGSZ(86) bytes, the PP_TxLfrag
	 * can provide 128 bytes, so we don't need D11 pool anymore.
	 *
	 * RX:
	 *       1. RXFIFO-2, SHARE_LBUF
	 *       2. Rx pktfetch use PP_LBUF as input and a
	 *           SHARE_LBUF/HEAP_LBUF + PP_LBUF as output
	 *            wlc_recvdata_pktfetch_cb()/wlc_bme_pktfetch_recvdata() will free PP_LBUF and
	 *            keep using SHARE_LBUF/HEAP_LBUF.
	 *            See
	 *            - rte_pktfetch.c::hnd_pktfetch_dispatch()
	 *            - wlc_pktfetch.c::_bme_pktfetch_rx()
	 * TX:
	 *       1. Tx pktfetch keep using original PP_LBUF but SW will allocate
	 *            a new big HEAP_BUFFER for it.
	 *            See
	 *            - rte_pktfetch.c::hnd_pktfetch_dispatch()
	 *            - wlc_pktfetch.c::_bme_pktfetch_tx()
	 * NOTE: In PKTPGR platform, we only have SW SHARED_POOL,
	 * no FRAG, RESV_FRAG and RXFRAG pool and D3, D11 buffer pool.
	 * One D11 buffer size is PKTFRAGSZ(86) bytes, the PP_TxLfrag
	 * can provide 128 bytes. So we don't need D11 pool.
	 *
	 * If SW need to allocate RxLfrag or TxLfrag from PKTPGR through
	 * HWA_PP_PAGEMGR_ALLOC_RX_RPH and HWA_PP_PAGEMGR_ALLOC_TX.
	 */
	/* CASES:
	 * 1:RX-SHARED_LBUF(Rx Mgmt/Ctrl or Rx pktfetch)
	 */
	if (lb_pool(lb)) {
		pktpool_t *pool = lb_getpool(lb);

		ASSERT(pool);
		ASSERT(LB_POOL(lb) == POOLID(pool));

		// It must be from SHARED_POOL when lb is from pool.

		lb_resetpool(lb, pool->max_pkt_bytes);
		pktpool_free(pool, lb); /* put back to pool */
	} else {
		/* lb is not from lb_pool, it can be from PP_LBUF or heap.
		 * We don't use D3 and D11 buffer pool, we only use SHARED_POOL
		 * One D11 buffer size is PKTFRAGSZ(86) bytes, the PP_TxLfrag can
		 * provide 128 bytes. So we don't need D11 pool.
		 * Check if PP_LBUF.
		 * Check if HEAP_LBUF.  HEAP_LBUF will use continous data buffer.
		 * So just free HEAP_LBUF.
		 * RX:
		 *       1. Rx pktfetch use PP_LBUF as input and a
		 *            SHARE_LBUF/HEAP_LBUF + PP_LBUF as output
		 *            wlc_recvdata_pktfetch_cb will free PP_LBUF and
		 *            keep using SHARE_LBUF/HEAP_LBUF.
		 *       So here we take care HEAP_LBUF or PP_LBUF for RX.
		 * TX:
		 *       1. Tx pktfetch keep using original PP_LBUF but SW will
		 *            allocate a new big HEAP_BUFFER for it.
		 *       So here we take case PP_LBUF.
		 */

		/* CASES:
		 * 1:RX-PP_LBUF(normal rx pkt)
		 * 2:RX-HEAP_LBUF(Rx pktfetch pkt)
		 * 3:TX-HEAP_LBUF(mgmt from heap_lbuf).
		 * 4:TX-PP_LBUF(normal tx pkt)
		 * 5:TX-PP_LBUF-HEAP_BUFFER(tx pktfetch)
		 */
		// Say, PP_LBUF must have be PKTISHWAPKT flag.
		if (PKTISHWAPKT(OSH_NULL, lb)) {
			// 1:RX-PP_LBUF(normal rx pkt)
			// 3:TX-HEAP_LBUF(mgmt from heap_lbuf).
			// 4:TX-PP_LBUF(normal tx pkt)
			// 5:TX-PP_LBUF-HEAP_BUFFER(tx pktfetch)
			if (PKTISMGMTTXPKT(OSH_NULL, lb)) {
#ifdef HNDPQP
				// Update PQP HBM credit
				pqp_hbm_avail_add(1);
#endif /* HNDPQP */

				// 3:TX-HEAP_LBUF(mgmt from heap_lbuf).
				if (PKTISHWADDMBPKT(OSH_NULL, lb)) {
					// HEAP_LBUF from ddbm, it was dropped in PQP.
					// Reset the flag and free it.
					PKTRESETHWADDMBPKT(OSH_NULL, lb);
					// free TX-PP_LBUF part
					hwa_pktpgr_free_tx(hwa_dev,
						PPLBUF(lb), PPLBUF(lb), 1);
				} else if (PKTHASPQPHEAPPOOL(OSH_NULL, lb)) {
					// HEAP_LBUF from pqp pool, free it.
					PKTRESETPQPHEAPPOOL(OSH_NULL, lb);
					lfbufpool_free(lb);
				} else {
					// Free DDBM if the mgmt pkt still owns it.
					// It usually happen during partial shadow reclaim.
					// Pkt is freed in wlc_low_txq_account().
					if (PKTMGMTDDBMPKT(OSH_NULL, lb)) {
						// free TX-PP_LBUF part
						hwa_pktpgr_free_tx(hwa_dev,
							PPLBUF(PKTMGMTDDBMPKT(OSH_NULL, lb)),
							PPLBUF(PKTMGMTDDBMPKT(OSH_NULL, lb)), 1);
						PKTRESETMGMTDDBMPKT(OSH_NULL, lb);
					}

					// HEAP_LBUF from memory use continous lbuf+data buffer
					// so just free HEAP_LBUF.
					lb->control.data = (uchar *)0xdeadbeef;
					MFREE(OSH_NULL, lb, LB_END(lb) - LB_HEAD(lb));
				}
			} else if (PKTISTXFRAG(OSH_NULL, lb)) {
				// 4:TX-PP_LBUF(normal tx pkt)
				// 5:TX-PP_LBUF-HEAP_BUFFER(tx pktfetch)

				// free HEAP_BUFFER part
				if (PKTISBUFALLOC(OSH_NULL, lb)) {
					if (PKTHASHEAPBUF(OSH_NULL, lb)) {
						MFREE(OSH_NULL,
							PKTHEAD(OSH_NULL, lb),
							MAXPKTDATABUFSZ);
						PKTRESETHEAPBUF(OSH_NULL, lb);
					} else {
						// HEAP_LBUF from pool, free it.
						lfbufpool_free(PKTHEAD(OSH_NULL, lb));
					}
					PKTRESETBUFALLOC(OSH_NULL, lb);
				}

				// TX-PP_LBUF-HEAP_BUFFER(tx pktfetch)
				if (PKTHASHMEDATA(OSH_NULL, lb)) {
					hwa_pktpgr_hmedata_free(hwa_dev, lb);
					PKTRESETHMEDATA(OSH_NULL, lb);
				}

#ifdef HNDPQP
				// Update PQP HBM credit
				pqp_hbm_avail_add(1);
#endif /* HNDPQP */

				// Don't need to clean SW fields because
				// pktpgr_free doesn't sync DDBM to HDBM.
				// The DDBM SW fields will be overwrited
				// by other HBM when we do PAGEIN.
				// So we don't need to clean SW fields here.
				// Instead, we need to reset/fixup them in
				// PAGEIN RESP.

				// free TX-PP_LBUF part
				hwa_pktpgr_free_tx(hwa_dev,
					PPLBUF(lb), PPLBUF(lb), 1);
			} else {
				// 1:RX-PP_LBUF(normal rx pkt)

				// free RPH part if any
				if (PKTISRXFRAG(OSH_NULL, lb)) {
					hwa_pktpgr_free_rph(hwa_dev,
						RPH_HOSTPKTID(lb),
						RPH_HOSTDATALEN(lb),
						RPH_HOSTADDR64(lb));
				}

				// free RX-PP_LBUF part
				hwa_pktpgr_free_rx(hwa_dev,
					PPLBUF(lb), PPLBUF(lb), 1);
			}
		} else {
			// 2:RX-HEAP_LBUF(Rx pktfetch pkt)
			// HEAP_LBUF use continous lbuf+data buffer
			// so just free HEAP_LBUF.
			lb->control.data = (uchar *)0xdeadbeef;
			MFREE(OSH_NULL, lb, LB_END(lb) - LB_HEAD(lb));
		}
	}
}

#ifdef HNDPQP
static void
pqp_lb_free_one(struct lbuf *lb)
{
	ASSERT(lb_sane(lb));

	/* It's difficult to implement CLONE, see lb_clone. */
	ASSERT(!lb_isclone(lb));

	// This PP_LBUF from PQP must have be PKTISHWAPKT and PKTISTXFRAG flag.
	ASSERT(PKTISHWAPKT(OSH_NULL, lb) && PKTISTXFRAG(OSH_NULL, lb));
	if (PKTISHWAPKT(OSH_NULL, lb) && PKTISTXFRAG(OSH_NULL, lb)) {
		// free HEAP_BUFFER part
		// There is no hmedata because the local buffer was kept in sysmem
		// when there is no LCL resource.
		// Don't put heap buffer back to sysmem.
		if (PKTISBUFALLOC(OSH_NULL, lb) && PKTHASHMEDATA(OSH_NULL, lb)) {
			if (PKTHASHEAPBUF(OSH_NULL, lb)) {
				MFREE(OSH_NULL,
					PKTHEAD(OSH_NULL, lb),
					MAXPKTDATABUFSZ);
				PKTRESETHEAPBUF(OSH_NULL, lb);
			} else {
				// HEAP_LBUF from pool, free it.
				lfbufpool_free(PKTHEAD(OSH_NULL, lb));
			}
			PKTRESETBUFALLOC(OSH_NULL, lb);
		}

		// Don't need to clean SW fields because
		// pktpgr_free doesn't sync DDBM to HDBM.
		// The DDBM SW fields will be overwrited
		// by other HBM when we do PAGEIN.
		// So we don't need to clean SW fields here.
		// Instead, we need to reset/fixup them in
		// PAGEIN RESP.

		// When pqplbuf_use_rsv_ddbm is set, enqueue DDBM to pqplbufpool if resource is low.
		if (!hwa_pktpgr_pqplbufpool_enq(hwa_dev, PPLBUF(lb))) {
			// free TX-PP_LBUF part
			hwa_pktpgr_free_tx(hwa_dev,
				PPLBUF(lb), PPLBUF(lb), 1);
		}
	}
}

void
pqp_lb_free(struct lbuf *lb)
{
	struct lbuf *next;
	while (lb) {
		ASSERT(PKTLINK(lb) == NULL);
		next = PKTNEXT(OSH_NULL, lb);

		pqp_lb_free_one(lb);
		lb = next;
	}
}
#endif /* HNDPQP */

void
lb_free(struct lbuf *lb)
{
	struct lbuf *next;
	bool snarf;

	while (lb) {
		ASSERT(PKTLINK(lb) == NULL);

		next = PKTNEXT(OSH_NULL, lb);

		if (BCMLFRAG_ENAB()) {

			lbuf_freeinfo_t *lbuf_infoptr;

			lbuf_infoptr = lbuf_info_get();
			/* if a tx frag or rx frag , go to callback functions before free up */
			/* pciedev_lbuf_callback */
			if (lbuf_infoptr->cb) {
				PKTSETNEXT(OSH_NULL, lb, NULL);
				snarf = lbuf_infoptr->cb(lbuf_infoptr->arg, lb);
				if (snarf) {
					lb = next;
					continue;
				}
			}
		}

#if defined(BCMPCIEDEV) && !defined(BME_PKTFETCH)
		if (BCMPCIEDEV_ENAB()) {
			/* rte_pktfetch.c::hnd_lbuf_free_cb() */
			if (lbuf_free_cb != NULL)
				(lbuf_free_cb)(lb);
		}
#endif /* BCMPCIEDEV && !BME_PKTFETCH */

		lb_free_one(lb);
		lb = next;
	}
}

struct lbuf *
#if defined(BCMDBG_MEMFAIL)
lb_dup(struct lbuf *lb, void *call_site)
#else
lb_dup(struct lbuf *lb)
#endif
{
	struct lbuf *lb_new;

#if defined(BCMDBG_MEMFAIL)
	if (!(lb_new = lb_alloc(lb->control.len, lbuf_basic, call_site)))
#else
	if (!(lb_new = lb_alloc(lb->control.len, lbuf_basic)))
#endif
		return (NULL);

	bcopy(lb->control.data, lb_new->control.data, lb->control.len);

	return (lb_new);
}

// NOTE: It's difficult to implement CLONE, because the cloned
// packet need to reference data segment of original packet but
// the original packet may be in HDBM or DDBM.
// Can we replace clone as duplicate.
struct lbuf *
#if defined(BCMDBG_MEMFAIL)
lb_clone(struct lbuf *lb, int offset, int len, void *call_site)
#else
lb_clone(struct lbuf *lb, int offset, int len)
#endif
{
	uchar *src_data;
	struct lbuf *lb_new;

	ASSERT(offset >= 0 && len >= 0);

	if (offset < 0 || len < 0)
		return (NULL);

#if defined(BCMDBG_MEMFAIL)
	if (!(lb_new = lb_alloc(len, lbuf_basic, call_site)))
#else
	if (!(lb_new = lb_alloc(len, lbuf_basic)))
#endif
	{
		LBUF_MSG(("lb_clone: alloc failed; called from 0x%p\n", CALL_SITE));
		return (NULL);
	}

	/* dup's data extent is a subset of the current data of lb */
	ASSERT(lb->control.data + offset + len <= lb_end(lb));

	src_data = lb->control.data + offset;
	bcopy(src_data, lb_new->control.data, len);
	lb_new->control.flags |= (LBF_HWA_PKT | LBF_MGMT_TX_PKT);

	return (lb_new);
}

/*
 * reset lbuf before recycling to pool
 *      PRESERVE                        FIX-UP
 *      fraginfo.poolid                 fraginfo.poolstate
 *      offsets | head, end
 *                                      data
 *                                      len, flags
 *                                      reset all other fields
 */
void
lb_resetpool(struct lbuf *lb, uint16 len)
{
	ASSERT(lb_sane(lb));
	ASSERT(lb_pool(lb));

#ifdef BCMDBG_POOL
	lb_setpoolstate(lb, POOL_IDLE); /* fraginfo.poolstate */
#endif

	lb->control.data = lb_end(lb) - ROUNDUP(len, sizeof(int));
	lb->control.len = len;
	if (lb->control.flags & LBF_TX_FRAG) {
		lb->control.flags = lb->control.flags & (LBF_TX_FRAG | LBF_RX_FRAG | LBF_HEAPBUF);
	} else {
		lb->control.flags = lb->control.flags & (LBF_TX_FRAG | LBF_RX_FRAG);
	}

	lb->control.dataOff = 0;
	lb->control.ifid = 0;
	lb->control.next = NULL;
	lb->control.link = NULL;
	bzero(&lb->pkttag, sizeof(lb->pkttag));

#ifdef WLCFP
	/* Reset CFP Flowid
	 * Both 0 and CFP_FLOWID_INVALID are considered invalid for CFP flows
	 */
	lb->control.flowid = 0;
#endif
}

void
lb_clear_pkttag(struct lbuf *lb)
{
	bzero(&lb->pkttag, sizeof(lb->pkttag));
}

void
lbuf_free_cb_set(lbuf_free_global_cb_t cb)
{
#if defined(BCMPCIEDEV) && !defined(BME_PKTFETCH)
	if (BCMPCIEDEV_ENAB()) {
		lbuf_free_cb = cb;
	}
#endif /* BCMPCIEDEV && !BME_PKTFETCH */
}

#ifdef BCMDBG
void
lb_dump(void)
{
	uint i;
	LBUF_MSG(("allocfail %d\n", lbuf_allocfail));
	for (i = 0; i < LBUF_BIN_SIZE; i++) {
		LBUF_MSG(("bin[%d] %d ", i, lbuf_hist[i]));
	}
	LBUF_MSG(("\n"));
}
#endif	/* BCMDBG */
