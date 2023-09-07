/*
 * +--------------------------------------------------------------------------+
 *  HND Packet Queue Pager (PQP) Service
 *
 *  Copyright 2022 Broadcom
 *
 *  This program is the proprietary software of Broadcom and/or
 *  its licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.  IF YOU HAVE NO
 *  AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 *  WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 *  THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1. This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use
 *  all reasonable efforts to protect the confidentiality thereof, and to
 *  use this information only in connection with your use of Broadcom
 *  integrated circuit products.
 *
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *  REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 *  OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *  DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *  NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *  ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *  CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *  OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 *  BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 *  SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 *  IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *  IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 *  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 *  OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 *  NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *  <<Broadcom-WL-IPTag/Proprietary:>>
 *
 *  $Id$
 *
 * vim: set ts=4 noet sw=4 tw=80:
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * +--------------------------------------------------------------------------+
 */

#ifndef __hnd_pqp_h_included__
#define __hnd_pqp_h_included__

#include <typedefs.h>

#define PQP_USE_MGMT_LOCAL      /* Use local management buffer */

#ifndef PQP_REQ_MAX
#define PQP_REQ_MAX     256     /* 256 paging requests: 10 KBytes */
#endif

/**
 * +--------------------------------------------------------------------------+
 *  Host Memory Extension:
 *  Total memory = (PQP_PKT_SZ * PQP_PKT_MAX) + (PQP_LCL_SZ * PQP_LCL_MAX)
 *               = (256 Bytes  *  8192 Items) + (2048 Bytes *   256 Items)
 *               = (2 MBytes) + (512 KBytes)
 *               = 2.5 MBytes
 *
 * When BCMHWAPP is defined, HME LCLPKT is managed by HWA/PP driver
 * - See HME_LCLPKT_BUILD
 *
 * +--------------------------------------------------------------------------+
 */
#ifndef PQP_PKT_SZ
#define PQP_PKT_SZ      HWA_PP_LBUF_SZ     /* TxLfrag Context + D11 buffer */
#endif
#ifndef PQP_PKT_MAX
#define PQP_PKT_MAX     HWA_HOST_PKTS_MAX    /* Total packets (w/ D11) in host */
#endif

#define PQP_HME_SZ      (PQP_PKT_MAX * PQP_PKT_SZ)

/**
 * +--------------------------------------------------------------------------+
 *  PQP Theory of Opearation:
 *
 * hnd_pktq.h defines a single precedence packet queue, namely pktq_prec_t and
 * referred here to as "pktq". A pktq specifies the head packet, the tail packet
 * and number of packets held in the queue. In addition, other attributes like
 * maximum holding capacity and queue operation statistics.
 *
 * Packet Queue Pager, allows all packets held in a pktq to be offloaded
 * to Host Memory using "PAGE-OUT" operation and subsequently retrieved back
 * into Dongle Memory via a "PAGE-IN" operation. Upon a PAGE-IN, the packet may
 * occupy a different memory location and hence each packet's pointers will need
 * to be relocated, specifically Lbuf::<head, end, data, next, link>.
 * Pointers in a PKTTAG are not relocated.
 * Upon a PAGE-OUT, the pktq is deemed managed by the PQP. On a PAGE_OUT the
 * dongle resident packets are released to the free pool. pktq::<head,tail>
 * must not be accessed until a subsequent PAGE-IN has completed.
 * Moreover, the pktq::<head,tail> would likely point to packets in a different
 * location upon PAGE-IN.
 *
 * A "D11 PACKET is a 802.11 MPDU" i.e. a PKTNEXT linked list of Lfrags(MSDUs).
 * A QUEUE is essentially a Power Save Queue (or a Suppression PSQ) represented
 *    as a PKTLINK linked list of MPDUs with MSDUs encoded in a PKTNEXT linked
 *    list of Lfrags (lfrag_info may hold more than one MSDU's host info).
 *
 * Packets managed by PSQ and SuprPSQ are 802.11 formatted.
 *    PSQ      = D11 Packets are extracted from Common Queue. The PSQ is
 *               built in entirety before page-out. i.e. No append to tail.
 *    SuprPSQ  = D11 Packets suppressed via TxStatus. On each TxStatus, a list
 *               suppressed D11 packets are appended to the SuprPSQ.
 *               Unlike a PSQ, a SuprPSQ may be incrementally constructed with
 *               append to tail of a new set of D11 packets.
 *    On exit of PowerSave, all D11 packets in PSQ are appended to the tail of
 *    SupprPSQ, and the joined queue is treated as the PSQ.
 *
 * Each PSQ and SuprPSQ are individually (independently) tracked by PQP and
 * consume one PQP Request resource. A PQP Request is not exported.
 *
 * +--------------------------------------------------------------------------+
 *
 *  Packet is used to refer to a MPDU (List of Lbufs linked via PKTNEXT)
 *  Packet Storage Memory: DBM (Dongle Buffer Memory), HBM (Host Buffer Memory)
 *
 *  WL APPS configures it's OSH, Packet Callback and Queue callback.
 *  - Packet Callback (pgo_pcb) is invoked by PQP on the successful page-out of
 *    a Packet. WL APPS is responsible for freeing the DBM packet. As the packet
 *    still lives in HBM, only packet storage may be freed.
 *  - Queue Callback (pgi_qcb) is invoked by PQP when a page-in request could
 *    not be synchronously fullfilled due to a dongle resource depletion.
 *
 *  On a PS indication, WL-APPS extracts packets from CmnQ and placed into PSQs.
 *  Upon composing a PSQ, it may be paged out to recover DBM Lbuf storage. On
 *  TxStatus suppression, suppressed packets are accumulated into SupprPSQ. Upon
 *  each TxStatus handling, a SupprPSQ may be incrementally paged out. Upon
 *  handling of all TxStatus suppressions, PSQ is joined to tail of SupprPSQ.
 *  Joining empties PSQ.
 *
 *  WL APPS may request a page-in of a subset of packets held in a pqp_pktq.
 *  PQP will return the requested n_pkts based on availability of DBM resources.
 *  If resources are unavailable, asynchronously the DBM resources will be
 *  allocated and the WL APPS Queue Callback will be invoked.
 *
 * +--------------------------------------------------------------------------+
 */

typedef struct lbuf      pqp_pkt_t;  /* hnd_lbuf.h, hnd_pp_lbuf.h */
typedef struct pktq_prec pqp_pktq_t; /* hnd_pktq.h */

typedef enum pqp_policy
{
	PQP_PREPEND     =  0,
	PQP_APPEND      =  1,
	PQP_POLICY_MAX  =  2
} pqp_policy_t;

// Packet availability thresholds for iterating a Page Out and In
#define PQP_PGO_HBM_THRESH          (8) // used for hme_pkt and hme_lcl free_cnt
#define PQP_PGI_DBM_THRESH          (8)
#define PQP_PGI_DBM_THRESH_RSV      (2)
#define PQP_PGI_MEM_THRESH          (32 * 1024) // 32 KBytes
#define PQP_PGI_BUF_THRESH          (2)

/**
 * +--------------------------------------------------------------------------+
 *  WLAN AP PowerSave Callback handler for freeing packets on pageout and
 *  when a pagein resumes after a stall.
 *
 *  ctx is parameter associated with a pqp_pktq. E.g. scb owning the pqp_pktq
 *
 *  pgo_pcb: PACKET Callback invoked on PAGE-OUT per D11 PACKET.
 *  pgi_qcb: QUEUE  Callback invoked on a stalled PAGE-IN of Queue.
 * +--------------------------------------------------------------------------+
 */
typedef struct pqp_cb {
	osl_t       * osh;      /* wl apps osh */
	void        * ctx;      /* hndpktq_t context, e.g. scb */
	pqp_pkt_t   * pqp_pkt;  /* PCB: dbm pkt paged out and to be freed */
	pqp_pktq_t  * pqp_pktq; /* QCB: pagein resumed and appended to pqp_pktq */
} pqp_cb_t;

typedef int (*pqp_cb_fn_t)(pqp_cb_t * cb);

/**
 * +--------------------------------------------------------------------------+
 *  PQP Functional Interface (Service Debug and Service Initialization)
 * +--------------------------------------------------------------------------+
 *
 *  + pqp_dump()    : Debug dump the internal PQP runtime state
 *  + pqp_stats()   : Debug dump the internal PQP runtime statistics
 *
 *  + pqp_fini()    : Detach the PQP subsystem
 *  + pqp_init()    : Initialize the PQP subsystem with max-queues capability
 *                    Invoked in rte.c after <hme, bme> services are initialized
 *
 *  + pqp_bind_hme(): Bind to HME segments: specify storage requirement in pages
 *                    Invoked in rte.c, after pqp_init()
 *  + pqp_link_hme(): Link to HME segments: fetch storage addresses
 *                    Invoked in bcmhme.c
 *
 * +--------------------------------------------------------------------------+
 */
void pqp_dump(bool verbose);
void pqp_stats(void);
void pqp_cmd(void * arg, int argc, char * argv[]); // hnd_cons_add_cmd()

int  BCMATTACHFN(pqp_fini)(osl_t *osh);
int  BCMATTACHFN(pqp_init)(osl_t *osh, int pqp_req_max);

int  pqp_bind_hme(void);
int  pqp_link_hme(void);

/**
 * +--------------------------------------------------------------------------+
 *  PQP Service Runtime Operational Interface
 * +--------------------------------------------------------------------------+
 */

/** Query PQP state, and whether a pktq is managed by PQP */
int  pqp_dbm_cnt(pqp_pktq_t *pqp_pktq);
int  pqp_hbm_cnt(pqp_pktq_t *pqp_pktq);
int  pqp_pkt_cnt(pqp_pktq_t *pqp_pktq);

bool pqp_owns(pqp_pktq_t *pqp_pktq);

/** Dump the pktq state */
void pqp_pktq_dump(pqp_pktq_t * pqp_pktq);

/**  AP Power Save configures callbacks for pageout and pagein */
int  pqp_config(osl_t *wl_osh, void * dev,
                pqp_cb_fn_t pgo_pcb, pqp_cb_fn_t pgi_qcb);

/**  PQP hbm credits operation */
int  pqp_hbm_avail(void);
void pqp_hbm_avail_add(int delta);
void pqp_hbm_avail_sub(int delta);

/**
 * +--------------------------------------------------------------------------+
 *   Page out list of packets in pqp_pktq_t by appending ot prepending to PQP
 *   managed list, if any. User context to be returned via page out packet
 *   and page in queue callbacks.
 * +--------------------------------------------------------------------------+
 */
int  pqp_pgo(pqp_pktq_t *pqp_pktq, pqp_policy_t pqp_policy, void *cb_ctx);
int  pqp_pgo_wake(void); // private: backdoor wakeup stalled PGO

/**
 * +--------------------------------------------------------------------------+
 *   Join two pqp_pktq "A" and "B"
 *   - pqp_pktq "B" may be prepended or appended to pktq "A"
 *   - pqp_pktq "B" is reset after joining.
 *   - Joined queue is returned in pqp_pktq
 * +--------------------------------------------------------------------------+
 */
int  pqp_join(pqp_pktq_t *pqp_pktq_A, pqp_pktq_t *pqp_pktq_B,
              pqp_policy_t pqp_policy, void *cb_ctx);

/**
 * +--------------------------------------------------------------------------+
 *   Page in packets from head of PQP managed list and append to list of packets
 *   in pqp_pktq. Queue callback invoked for at least qcb_min_thresh number of
 *   packets paged in from a requested n_pkts.
 *     cont_pkts - minimum number of packets for a qcb callback resumption
 *     fill_pkts - requested number of packets to fill into packet queue
 *
 *   While fill_pkts are requested, DBM resources may not be available. PQP will
 *   invoke the queue callback to resume application, when at least cont_pkts
 *   worth of packet are available.
 *
 *   Queue callback is only invoked to resume WL APPS, when a pqp_pgi() was
 *   stalled. On return of a pqp_pgi(), WLA APPS may consume any packets
 *   returned into the pktq as such pktq owned packets are external to PQP.
 * +--------------------------------------------------------------------------+
 */
int  pqp_pgi(pqp_pktq_t *pqp_pktq, int cont_pkts, int fill_pkts);
int  pqp_pgi_wake(void); // private: backdoor wakeup stalled PGI
void pqp_pgi_spl_set(bool pgi_spl);

/*
 * +--------------------------------------------------------------------------+
 * XXX Do we need an explicit pqp_clr() API to handle a station disassociation?
 *
 * Current design is to perform a pqp_pgi() and retrieve all packets, so
 * appropriate callbacks are invoked with a dongle resident PKTTAG
 * +--------------------------------------------------------------------------+
 */

#endif /* __hnd_pqp_h_included__ */
