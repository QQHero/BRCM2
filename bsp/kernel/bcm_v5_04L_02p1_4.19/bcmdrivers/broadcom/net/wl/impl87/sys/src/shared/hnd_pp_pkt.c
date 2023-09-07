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

#include <typedefs.h>
#include <osl.h>
#include <hnd_pp_lbuf.h>
#include <hnd_pktpool.h>
#include <hnd_pp_pkt.h>
#include <bcmutils.h>
#include <hwa_export.h>

#if defined(BCMDBG_MEMFAIL)
#define LB_ALLOC_HEADER(data, len, lbuf_type)	lb_alloc_header(data, len, lbuf_type, CALL_SITE)
#define LB_ALLOC(len, type)			lb_alloc(len, type, CALL_SITE)
#define LB_CLONE(p, offset, len)		lb_clone(p, offset, len, CALL_SITE)
#define LB_DUP(p)				lb_dup(p, CALL_SITE)
#else
#define LB_ALLOC_HEADER(data, len, lbuf_type)	lb_alloc_header(data, len, lbuf_type)
#define LB_ALLOC(len, type)			lb_alloc(len, type)
#define LB_CLONE(p, offset, len)		lb_clone(p, offset, len)
#define LB_DUP(p)				lb_dup(p)
#endif

static uint32 pktget_failed_but_alloced_by_pktpool = 0;
static uint32 pktget_failed_not_alloced_by_pktpool = 0;

/**
 * Allocates an empty packet in dongle memory. When this fails, it attempts to get a packet from the
 * 'shared' packet pool.
 *
 * @param osh   OS specific handle
 * @param len   length in [bytes] of packet buffer to allocate in dongle memory
 */
void *
hnd_pkt_get_header(osl_t *osh, void *data, uint len, enum lbuf_type lbuf_type)
{
	void *pkt;

	pkt = (void *)LB_ALLOC_HEADER(data, len, lbuf_type);

	if (pkt)
		osh->cmn->pktalloced++;
	return pkt;
}

/**
 * Allocates an empty packet in dongle memory. When this fails, it attempts to get a packet from the
 * 'shared' packet pool.
 *
 * @param osh   OS specific handle
 * @param len   length in [bytes] of packet buffer to allocate in dongle memory
 */
void *
hnd_pkt_get(osl_t *osh, uint len, bool send)
{
	void *pkt;
	enum lbuf_type type = lbuf_basic;

	if (send) {
		type = lbuf_mgmt_tx;
	}

	pkt = hnd_pkt_alloc(osh, len, type);

	// Mgmt packet "lbuf_mgmt_tx" allocation is only from HEAP.
	if (pkt == NULL && type == lbuf_basic) {
		/* fastdma requeirement */
#if HWACONF_GE(131)
		return pkt; // Give up
#endif
		if (POOL_ENAB(SHARED_POOL) && (len <= pktpool_max_pkt_bytes(SHARED_POOL))) {
			pkt = pktpool_get(SHARED_POOL);
			if (pkt) {
				PKTSETLEN(osh, pkt, len);
				pktget_failed_but_alloced_by_pktpool++;
			} else {
				pktget_failed_not_alloced_by_pktpool++;
			}
		}
	}

	return pkt;
}

/**
 * Allocates an empty packet in dongle memory.
 *
 * @param osh   OS specific handle
 * @param len   length in [bytes] of packet buffer to allocate in dongle memory
 * @param type  e.g. 'lbuf_frag'
 */
void *
hnd_pkt_alloc(osl_t *osh, uint len, enum lbuf_type type)
{
	void *pkt;

#if defined(MEM_LOWATLIMIT)
	if ((OSL_MEM_AVAIL() < MEM_LOWATLIMIT) && len >= PKTBUFSZ) {
		return (void *)NULL;
	}
#endif
	pkt = (void *)LB_ALLOC(len, type);

	if (pkt)
		osh->cmn->pktalloced++;
	return pkt;
}

void
hnd_pkt_free(osl_t *osh, void *p, bool send)
{
	struct lbuf *nlbuf;

	if (send) {
		if (osh->tx_fn) /* set using PKTFREESETCB() */
			osh->tx_fn(osh->tx_ctx, p, 0);
	} else {
		if (osh->rx_fn) /* set using PKTFREESETRXCB() */ {
			osh->rx_fn(osh->rx_ctx, p);
		}
	}

	for (nlbuf = (struct lbuf *)p; nlbuf; nlbuf = PKTNEXT(osh, nlbuf)) {
		if (!lb_pool(nlbuf) && !PKTISHWADDMBPKT(osh, nlbuf) &&
			(!PKTISHWAPKT(osh, nlbuf) || (PKTISMGMTTXPKT(osh, nlbuf) &&
			!PKTHASPQPHEAPPOOL(osh, nlbuf)))) {
			ASSERT(osh->cmn->pktalloced > 0);
			osh->cmn->pktalloced--;
		}
	}

	lb_free((struct lbuf *)p);
}
#ifdef HNDPQP
void
hnd_pqp_pkt_free(osl_t *osh, void *p)
{
	struct lbuf *nlbuf;

	for (nlbuf = (struct lbuf *)p; nlbuf; nlbuf = PKTNEXT(osh, nlbuf)) {
		if (!lb_pool(nlbuf) && !PKTISHWADDMBPKT(osh, nlbuf) &&
			(!PKTISHWAPKT(osh, nlbuf) || (PKTISMGMTTXPKT(osh, nlbuf) &&
			!PKTHASPQPHEAPPOOL(osh, nlbuf)))) {
			ASSERT(osh->cmn->pktalloced > 0);
			osh->cmn->pktalloced--;
		}
	}

	pqp_lb_free((struct lbuf *)p);
}
#endif /* HNDPQP */
// Only for Mgmt "lbuf_mgmt_tx" packet.
void
hnd_pkt_put(osl_t *osh, void *p)
{
	struct lbuf *lbuf = (struct lbuf *)p;

	ASSERT(!PKTNEXT(osh, lbuf));
	ASSERT(!lb_pool(lbuf));
	ASSERT(PKTISHWAPKT(osh, lbuf));
	ASSERT(PKTISMGMTTXPKT(osh, lbuf));
	ASSERT(osh->cmn->pktalloced > 0);

	osh->cmn->pktalloced--;

	// 3:TX-HEAP_LBUF(mgmt from heap_lbuf, phase 1 free at PAGEOUT_LOCAL_RESP).
	lbuf->control.data = (uchar *)0xdeadbeef;

	MFREE(OSH_NULL, lbuf, LB_END(lbuf) - LB_HEAD(lbuf));
}

void *
hnd_pkt_clone(osl_t *osh, void *p, int offset, int len)
{
	void *pkt;

	pkt = (void *)LB_CLONE(p, offset, len);

	if (pkt)
		osh->cmn->pktalloced++;

	return pkt;
}

void *
hnd_pkt_dup(osl_t *osh, void *p)
{
	void *pkt;

	if ((pkt = (void *)LB_DUP((struct lbuf *)p)))
		osh->cmn->pktalloced++;

	return pkt;
}

void *
hnd_pkt_frmnative(osl_t *osh, struct lbuf *lbuf)
{
	struct lbuf *nlbuf;

	for (nlbuf = lbuf; nlbuf; nlbuf = PKTNEXT(osh, nlbuf)) {
		if (!lb_pool(nlbuf) && !PKTISHWAPKT(osh, nlbuf))
			osh->cmn->pktalloced++;
	}

	return ((void *)lbuf);
}

struct lbuf *
hnd_pkt_tonative(osl_t *osh, void *p)
{
	struct lbuf *nlbuf;

	for (nlbuf = (struct lbuf *)p; nlbuf; nlbuf = PKTNEXT(osh, nlbuf)) {
		if (!lb_pool(nlbuf) && !PKTISHWAPKT(osh, nlbuf)) {
			ASSERT(osh->cmn->pktalloced > 0);
			osh->cmn->pktalloced--;
		}
	}

	return ((struct lbuf *)p);
}
