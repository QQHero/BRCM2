/*
 * HND Packet Pager packet buffer definitions.
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

#ifndef _hnd_pp_lbuf_h_
#define	_hnd_pp_lbuf_h_

#include <typedefs.h>
#include <hnd_lbuf_cmn.h>

#ifdef BCMPKTIDMAP
#error "BCMPKTIDMAP is not support in PKTPGR mode"
#endif

// Forward declarations
struct lbuf;

// +--------------------------------------------------------------------------+
//  Layout of a Packet known to the Packet Pager
// +--------------------------------------------------------------------------+

// In RxLfrag, a portion of the unused Lfrag.FRAGINFO may be used as data buffer
#define HWA_PP_PKTDATABUF_WORDS     32
#define HWA_PP_PKTDATABUF_BYTES     (HWA_PP_PKTDATABUF_WORDS * NBU32)

#define HWA_PP_PKTCONTEXT_WORDS     32
#define HWA_PP_PKTCONTEXT_BYTES     (HWA_PP_PKTCONTEXT_WORDS * NBU32)

#define HWA_PP_PKTCONTROL_WORDS     8
#define HWA_PP_PKTCONTROL_BYTES     (HWA_PP_PKTCONTROL_WORDS * NBU32)

// Repurpose latest 3 words of databuffer for LBF_TX_FRAG
// head_int:4B end_int:4B
// headroom:2B poolstate:4b/poolid:4b or 1B, RSVD_FIELD:1B

// HNDPQP needs 3 x 4B = 12B hbm_pkt { u32 page, u32 next, u32 link }
// PROP_TXSTATUS needs 16B { fc_tlv[8]:8B, u16 rd_idx[0..2]:3 x 2B, fc_seq:2B }
#define HWA_PP_PKTDATABUF_TXFRAG_RESV_WORDS	(3 + 4) // LBF_TX_FRAG + MAX(HNDPQP, PROP_TXSTATUS)

#define HWA_PP_PKTDATABUF_TXFRAG_RESV_BYTES \
	(HWA_PP_PKTDATABUF_TXFRAG_RESV_WORDS * NBU32)
#define HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES \
	(HWA_PP_PKTDATABUF_BYTES - HWA_PP_PKTDATABUF_TXFRAG_RESV_BYTES)

typedef struct hwa_pp_lbuf_control {
	uint16 pkt_mapid;                           // HW: slot index in HostPktPool
	union {
		uint16 flowid;                          // SW: e.g. CFP flowid
		uint16 fifo_idx;                        // SW: WLCCFP: FIFO index
	};
	struct lbuf *next;                          // HW: frame(MPDU) fragment list
	struct lbuf *link;                          // HW: frame(MPDU) list
	uint32 flags;                               // SW: lbuf flags
	uchar  *data;                               // HW: start of data
	uint16 len;                                 // HW: length of data buffer
	uint8  ifid;                                // HW/SW: interface
	uint8  prio;                                // HW/SW: packet prio

	union {
		uint32 misc[2];                         // SW: RX and TxStatus path
		struct {                                // SW
			union {
				// rx completion ID used for AMPDU reordering
				uint16 rxcpl_id;
				uint16 dma_index;               // index into DMA ring
			};
			// offset to beginning of data in 4-byte words
			uint8  dataOff;
			uint8  txstatus;
			uint32 RSVD_FIELD;
		};
		struct {                                //
			uint32 info             :  4;       // HW: Host field for future use
			uint32 flowid_override  : 12;       // HW: Host flowid override
			uint32 RSVD_FIELD       : 16;       //

			uint32 copy             :  1;       // HW: COPY as-is bit
			uint32 flags            :  7;       // HW: flags
			uint32 RSVD_FIELD       :  8;       //
			uint32 RSVD_FIELD       : 12;       //
			uint32 sada_miss        :  1;       // HW: SADA Lkup Miss
			uint32 flow_miss        :  1;       // HW: flowid LUT / SADA miss
			uint32 copy_asis        :  1;       // HW: copy as-is
			uint32 audit_fail       :  1;       // HW: audit failure
		} txpost;                               // HW: HWA3a facing layout

		struct {                                //
			uint8  num_desc;                    // HW: see hwa_txfifo_pkt
			uint8  RSVD_FIELD;                  //
			uint16 amsdu_total_len;             // HW: see hwa_txfifo_pkt

			uint32 txdma_flags;                 // HW: see hwa_txfifo_pkt
		} txfifo;                               // HW: HWA3b facing layout
	};
} hwa_pp_lbuf_control_t;

#define HWA_PP_PKTTAG_WORDS         8
#define HWA_PP_PKTTAG_BYTES         (HWA_PP_PKTTAG_WORDS * NBU32)

typedef union hwa_pp_lbuf_pkttag {
	uint8   u8[HWA_PP_PKTTAG_BYTES];
	uint32 u32[HWA_PP_PKTTAG_WORDS];
} hwa_pp_lbuf_pkttag_t;

// 64 Bytes FragInfo, allowing 4-in-1 Host fragments for TxLfrag
#define HWA_PP_FRAGINFO_WORDS       16
#define HWA_PP_FRAGINFO_BYTES       (HWA_PP_FRAGINFO_WORDS * NBU32)

#define HWA_PP_PKT_FRAGINFO_MAX     4

typedef struct hwa_pp_lbuf_fraginfo {
	uint8  frag_num;                            // SW: total number of fragments
	uint8  flags;                               // SW: lbuf flags extension
	uint16 frag_len;                            // SW: misc: total length of fragments
	union {                                     //
		uint16 rx_misc[2];                      // SW: unused in RX(PhyRx?)
		struct {                                //
			uint16 flowring_id;             // HW: Flowring Id
			// Stale value in non-PROP_TXSTATUS builds; SW shouldn't use rd_idx.
			// Can be re-purposed
			uint16 rd_idx;			// HW: Flowring RD index
		} tx;                                   // HW: used in TX
	};                                          //

	union {
		uint32 host_pktid[HWA_PP_PKT_FRAGINFO_MAX];
		// RX only has one FRAGINFO so we can repurpose FRAGINFO1..3.
		// TX may use all 4 FRAGINFO, we cannot repurpose FRAGINFO1..3.
		// Since TX only need PKTFRAGSZ size of data buffer, so we repurpose
		// last 7 words of data_buffer.

		// for LBF_RX_FRAG
		struct {
			uint32 RSVD_FIELD[HWA_PP_PKT_FRAGINFO_MAX-2];	//host_pktid[0..1]
			// Repurpose host_pktid[2..3] for LBF_RX_FRAG
			uchar  *head;	//host_pktid[2]
			uchar  *end;	//host_pktid[3]
		};
	};

	union {
		uint16 host_datalen[HWA_PP_PKT_FRAGINFO_MAX];

		// for LBF_RX_FRAG
		struct {
			uint16 RSVD_FIELD[HWA_PP_PKT_FRAGINFO_MAX-2];	// host_datalen[0..1]:
			uint16 RSVD_FIELD;			// host_datalen[2]
			// host_datalen[3]
			uint8  RSVD_FIELD;
#if defined(BCMDBG_POOL)
			uint8  poolstate : 4;	/**< BCMDBG_POOL stats collection */
			uint8  poolid    : 4;	/**< pktpool ID */
#else  /* ! BCMDBG_POOL byte access is faster than 4bit access */
			uint8  poolid;			/**< entire byte is used for performance */
#endif /* ! BCMDBG_POOL */
		};
	};

	dma64addr_t data_buf_haddr64[HWA_PP_PKT_FRAGINFO_MAX];
	// Rx: 24 Bytes data_buf_haddr64[1..3] memory can be used for data_buffer
} hwa_pp_lbuf_fraginfo_t;

// Packet Pager compliant Lbuf Context structure
struct lbuf {
	union {
		uint8   u8[HWA_PP_PKTCONTEXT_BYTES];
		uint32 u32[HWA_PP_PKTCONTEXT_WORDS];
		struct {
		    hwa_pp_lbuf_control_t  control;         // Lbuf Control Header
		    hwa_pp_lbuf_pkttag_t   pkttag;          // Lbuf Packet Tage
		    hwa_pp_lbuf_fraginfo_t fraginfo;        // Lbuf Host Fragment Info
		};
	};
};

typedef struct lbuf hwa_pp_lbuf_context_t;

// Set the LBUFSZ equal to LBUFFRAGSZ.  Because we need head, end pointers we
// only can use the space in hwa_pp_lbuf_fraginfo_t
#define	LBP(lb)               ((struct lbuf *)(lb))
#define	LBUFSZ                ((int)sizeof(struct lbuf))
#define	LBFP(lb)              LBP(lb)
#define LBUFFRAGSZ            LBUFSZ
// Reserve 7 words for MGMT pkt when allocate from heap
#define	LBUFMGMTSZ            ((int)sizeof(struct lbuf) + HWA_PP_PKTDATABUF_TXFRAG_RESV_BYTES)

// Packet Pager manages Lbuf packets in Dongle and Host Packet Pools
// Packet Pager compliant Lbuf Context structure and Data buffer
typedef union hwa_pp_lbuf {
	uint8   u8[HWA_PP_PKTCONTEXT_BYTES + HWA_PP_PKTDATABUF_BYTES];
	uint32 u32[HWA_PP_PKTCONTEXT_WORDS + HWA_PP_PKTDATABUF_WORDS];
	struct {
		hwa_pp_lbuf_context_t context;
		union {
			uint8             data_buffer[HWA_PP_PKTDATABUF_BYTES];
			// TX may use all 4 FRAGINFO, we cannot repurpose above FRAGINFO 1..3.
			// Since TX only need PKTFRAGSZ size of data buffer, so we repurpose
			// lastest 7 words of data_buffer here.

			// for LBF_TX_FRAG, 7 words
			struct {
				// Repurpose 7 words of databuffer for LBF_TX_FRAG
				// MGMT pkt also need these fields for processing
				union {
					uchar	  *txreset; // zero from here for 7 words
					uchar	  *head;
				};
				uchar	      *end;
				uint16	      headroom;
#if defined(BCMDBG_POOL)
				uint8	      poolstate : 4;
				uint8	      poolid	: 4;
#else
				uint8	      poolid;
#endif /* ! BCMDBG_POOL */
				uint8	      RSVD_FIELD;

				union {
#if defined(PROP_TXSTATUS)
					struct {
						union {
							struct {
								uint8 key_seq[6];
								uint8 pktflags;
								uint8 RSVD_FIELD;
							};
							/* storage for WLFC TLV */
							uint8 fc_tlv[FC_TLV_SIZE];
						};
						uint16	rd_idx[HWA_PP_PKT_FRAGINFO_MAX - 1];
						uint16	fc_seq;
					};
#endif /* PROP_TXSTATUS */
#if defined(HNDPQP)
					struct { // HNDPQP: Packet's Host BM context
						union {
							uint32 page;
							struct {
								uint16  id; // HBM pkt ID
								// next HBM seg's tail HBM id
								uint16  seg_id;
							};
						};
						uintptr next; // Host BM PKTNEXT list
						uintptr link; // Host BM PKTLINK list
						uint32 RSVD_FIELD;
					} hbm_pkt;
#endif /* HNDPQP */
					uint32 RSVD_FIELD[4];
				};
				uint8   databuf_txfrag[HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES];
			};
		};
	};
} hwa_pp_lbuf_t;

#define	PPLBUF(lb)                  ((hwa_pp_lbuf_t *)(lb))
#define HWA_PP_LBUF_SZ              (sizeof(hwa_pp_lbuf_t))

// Below are specified in 4B words
// Only a single dma64addr_t needed for RxLfrag, re-purposed for databuffer
#define HWA_PP_RXLFRAG_DATABUF_OFFSET_WORDS \
	(HWA_PP_PKTCONTEXT_WORDS - \
	 ((sizeof(dma64addr_t) * (HWA_PP_PKT_FRAGINFO_MAX - 1)) / NBU32))
#define HWA_PP_RXLFRAG_DATABUF_OFFSET	(HWA_PP_RXLFRAG_DATABUF_OFFSET_WORDS * NBU32)

#define HWA_PP_RXLFRAG_DATABUF_LEN_WORDS ((HWA_PP_LBUF_SZ / NBU32) - \
	HWA_PP_RXLFRAG_DATABUF_OFFSET_WORDS)
#define HWA_PP_RXLFRAG_DATABUF_LEN	(HWA_PP_RXLFRAG_DATABUF_LEN_WORDS * NBU32)

/* Request HW to copy dasa_type 14B to data_offset unit is word(32-bit) for TxLfrag */
#ifndef HWA_ETHER_TYPE_2_OFFSET
#define HWA_ETHER_TYPE_2_OFFSET		14
#endif

#define HWA_TXPOST_DATA_OFFSET_WORDS \
	(ROUNDUP((HWA_PP_PKTDATABUF_TXFRAG_RESV_BYTES + PKTFRAGSZ - HWA_ETHER_TYPE_2_OFFSET), \
	NBU32) / NBU32)
#define HWA_TXPOST_DATA_OFFSET_BYTES	(HWA_TXPOST_DATA_OFFSET_WORDS * NBU32)
#define HWA_TXPOST_ETHTYPE_OFFSET_BYTES	(HWA_TXPOST_DATA_OFFSET_BYTES + HWA_ETHER_ADDR_LEN*2)

/* Total maximum packet buffer size including lbuf header */
#define MAXPKTBUFSZ (MAXPKTDATABUFSZ + LBUFSZ)

/* 36 bytes of PSM descriptor + 50 bytes of d11 header */
#ifndef MAXPKTFRAGDATABUFSZ
#define MAXPKTFRAGDATABUFSZ 86
#endif

/* Total maximum Tx packet buffer size including lbuf_frag header */
#define MAXPKTFRAGSZ	(MAXPKTFRAGDATABUFSZ + LBUFFRAGSZ)

/* 12bytes [RPH] + 12bytes SWRxstatus + 40bytesHWRxstatus + 14bytes plcp
 *  + 4bytes pad + 50bytes d11 hdr + 16 bytes copy count  =  148
 */
#ifndef MAXPKTRXFRAGDATABUFSZ
#define MAXPKTRXFRAGDATABUFSZ 148
#endif

#define MAXPKTRXFRAGSZ  (MAXPKTRXFRAGDATABUFSZ + LBUFFRAGSZ)

#define LB_FRAG1			0
#define LB_FRAG2			1
#define LB_FRAG3			2
#define LB_FRAG4			3

/* prototypes */
extern struct lbuf *lb_init(struct lbuf *lb, enum lbuf_type lb_type, uint lb_sz);

#if defined(BCMDBG_MEMFAIL)
extern struct lbuf *lb_alloc_header(void *data, uint size, enum lbuf_type lbuf_type,
	void *call_site);
extern struct lbuf *lb_alloc(uint size, enum lbuf_type lbuf_type, void *call_site);
extern struct lbuf *lb_clone(struct lbuf *lb, int offset, int len, void *call_site);
extern struct lbuf *lb_dup(struct lbuf *lb, void *call_site);
#else
extern struct lbuf *lb_alloc_header(void *data, uint size, enum lbuf_type lbuf_type);
extern struct lbuf *lb_alloc(uint size, enum lbuf_type lbtype);
extern struct lbuf *lb_clone(struct lbuf *lb, int offset, int len);
extern struct lbuf *lb_dup(struct lbuf *lb);
#endif

extern void lb_free(struct lbuf *lb);
#ifdef HNDPQP
extern void pqp_lb_free(struct lbuf *lb);
#endif /* HNDPQP */
extern void lb_resetpool(struct lbuf *lb, uint16 len);

#ifdef BCMDBG
extern void lb_dump(void);
#endif

typedef bool (*lbuf_free_cb_t)(void* arg, void* p);
extern void lbuf_free_register(lbuf_free_cb_t cb, void* arg);

typedef void (*lbuf_free_global_cb_t)(struct lbuf *lb);
extern void lbuf_free_cb_set(lbuf_free_global_cb_t lbuf_free_cb);
extern void lb_clear_pkttag(struct lbuf *lb);

/* Attach data buffer to lbuf */
extern void lb_set_buf(struct lbuf *lb, void *buf, uint size);
extern void lb_set_txbuf(struct lbuf *lb, void *buf, uint size);

/* head, end in fraginfo field or data buffer */
#define LB_HEAD_F(lb) (LBFP(lb)->fraginfo.head)
#define LB_END_F(lb) (LBFP(lb)->fraginfo.end)
#define LB_HEAD_B(lb) (PPLBUF(lb)->head)
#define LB_END_B(lb) (PPLBUF(lb)->end)
#define LB_HEAD(lb) ((LBFP(lb)->control.flags & (LBF_TX_FRAG | LBF_MGMT_TX_PKT)) ? \
	LB_HEAD_B(lb) : LB_HEAD_F(lb))
#define LB_END(lb)  ((LBFP(lb)->control.flags & (LBF_TX_FRAG | LBF_MGMT_TX_PKT)) ? \
	LB_END_B(lb) : LB_END_F(lb))

/* poolstate, poolid in fraginfo field or data buffer */
#if defined(BCMDBG_POOL)
#define LB_POOLSTATE(lb) ((LBFP(lb)->control.flags & (LBF_TX_FRAG | LBF_MGMT_TX_PKT)) ? \
	PPLBUF(lb)->poolstate : LBFP(lb)->fraginfo.poolstate)
#endif
#define LB_POOL(lb) ((LBFP(lb)->control.flags & (LBF_TX_FRAG | LBF_MGMT_TX_PKT)) ? \
	PPLBUF(lb)->poolid : LBFP(lb)->fraginfo.poolid)

/* GNU macro versions avoid the -fno-inline used in ROM builds. */
#define lb_sane(lb)  TRUE

#define lb_set_head(lb, head) ({ \
	if (LBFP(lb)->control.flags & (LBF_TX_FRAG | LBF_MGMT_TX_PKT)) { \
		LB_HEAD_B(lb) = (head); \
	} else { \
		LB_HEAD_F(lb) = (head); \
	} \
})

#define lb_set_end(lb, end) ({ \
	if (LBFP(lb)->control.flags & (LBF_TX_FRAG | LBF_MGMT_TX_PKT)) { \
		LB_END_B(lb) = (end); \
	} else { \
		LB_END_F(lb) = (end); \
	} \
})

#define lb_head(lb) ({ \
	ASSERT(lb_sane(lb)); \
	LB_HEAD(lb); \
})

#define lb_end(lb) ({ \
	ASSERT(lb_sane(lb)); \
	LB_END(lb); \
})

#define lb_push(lb, _len) ({ \
	uint __len = (_len); \
	ASSERT(lb_sane(lb)); \
	ASSERT(((lb)->control.data - __len) >= LB_HEAD(lb)); \
	(lb)->control.data -= __len; \
	(lb)->control.len += __len; \
	(lb)->control.data; \
})

#define lb_pull(lb, _len) ({ \
	uint __len = (_len); \
	ASSERT(lb_sane(lb)); \
	ASSERT(__len <= (lb)->control.len); \
	(lb)->control.data += __len; \
	(lb)->control.len -= __len; \
	(lb)->control.data; \
})

#define lb_setlen(lb, _len) ({ \
	uint __len = (_len); \
	ASSERT(lb_sane(lb)); \
	ASSERT((lb)->control.data + __len <= LB_END(lb)); \
	(lb)->control.len = (__len); \
})

#define lb_pri(lb) ({ \
	ASSERT(lb_sane(lb)); \
	((lb)->control.flags & LBF_PRI); \
})

#define lb_setpri(lb, pri) ({ \
	uint _pri = (pri); \
	ASSERT(lb_sane(lb)); \
	ASSERT((_pri & LBF_PRI) == _pri); \
	(lb)->control.flags = ((lb)->control.flags & ~LBF_PRI) | (_pri & LBF_PRI); \
})

#define lb_sumneeded(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_SUM_NEEDED) != 0); \
})

#define lb_setsumneeded(lb, summed) ({ \
	ASSERT(lb_sane(lb)); \
	if (summed) \
		(lb)->control.flags |= LBF_SUM_NEEDED; \
	else \
		(lb)->control.flags &= ~LBF_SUM_NEEDED; \
})

#define lb_sumgood(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_SUM_GOOD) != 0); \
})

#define lb_setsumgood(lb, summed) ({ \
	ASSERT(lb_sane(lb)); \
	if (summed) \
		(lb)->control.flags |= LBF_SUM_GOOD; \
	else \
		(lb)->control.flags &= ~LBF_SUM_GOOD; \
})

#define lb_msgtrace(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_MSGTRACE) != 0); \
})

#define lb_setmsgtrace(lb, set) ({ \
	ASSERT(lb_sane(lb)); \
	if (set) \
		(lb)->control.flags |= LBF_MSGTRACE; \
	else \
		(lb)->control.flags &= ~LBF_MSGTRACE; \
})

#define lb_dataoff(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.dataOff; \
})

#define lb_setdataoff(lb, _dataOff) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.dataOff = _dataOff; \
})

#define lb_isclone(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_CLONE) != 0); \
})

#define lb_is_txfrag(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_TX_FRAG) != 0); \
})

#define lb_set_txfrag(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_TX_FRAG; \
})

#define lb_reset_txfrag(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_TX_FRAG; \
})

#define lb_is_rxfrag(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_RX_FRAG) != 0); \
})

#define lb_set_rxfrag(lb) ({ \
	(lb)->control.flags |= LBF_RX_FRAG; \
})
#define lb_reset_rxfrag(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_RX_FRAG; \
})
#define lb_is_frag(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & (LBF_TX_FRAG | LBF_RX_FRAG)) != 0); \
})

/** Cache Flow Processing lbuf macros */
#define lb_is_cfp(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_CFP_PKT) != 0); \
})

#define lb_get_cfp_flowid(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flowid; \
})

#define lb_set_cfp_flowid(lb, cfp_flowid) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_CFP_PKT; \
	(lb)->control.flowid = (cfp_flowid); \
})

#define lb_clr_cfp_flowid(lb, cfp_flowid) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_CFP_PKT; \
	(lb)->control.flowid = (cfp_flowid); \
})

#define lb_isptblk(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_PTBLK) != 0); \
})

#define lb_nodrop(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_PKT_NODROP) != 0); \
})

#define lb_setnodrop(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_PKT_NODROP; \
})

#define lb_typeevent(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_PKT_EVENT) != 0); \
})

#define lb_settypeevent(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_PKT_EVENT; \
})

#define lb_dot3_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_DOT3_PKT) != 0); \
})

#define lb_set_dot3_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_DOT3_PKT; \
})

#define lb_ischained(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_CHAINED) != 0); \
})

#define lb_setchained(lb) ({ \
	ASSERT(lb_sane(lb)); \
	((lb)->control.flags |= LBF_CHAINED); \
})

#define lb_clearchained(lb) ({ \
	ASSERT(lb_sane(lb)); \
	((lb)->control.flags &= ~LBF_CHAINED); \
})

#define lb_has_metadata(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_METADATA) != 0); \
})

#define lb_set_has_metadata(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_METADATA; \
})
#define lb_reset_has_metadata(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_METADATA; \
})

#define lb_has_heapbuf(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_HEAPBUF) != 0); \
})

#define lb_set_heapbuf(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_HEAPBUF; \
})

#define lb_clr_heapbuf(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_HEAPBUF; \
})

#define lb_set_norxcpl(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_NORXCPL; \
})

#define lb_clr_norxcpl(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_NORXCPL; \
})

#define lb_need_rxcpl(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_NORXCPL) == 0); \
})

#define lb_altinterface(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_ALT_INTF) != 0); \
})

#define lb_set_pktfetched(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_PKTFETCHED; \
})

#define lb_reset_pktfetched(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_PKTFETCHED; \
})

#define lb_is_pktfetched(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_PKTFETCHED) != 0); \
})

#define lb_set_frwd_pkt(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_FRWD_PKT; \
})

#define lb_reset_frwd_pkt(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_FRWD_PKT; \
})

#define lb_is_frwd_pkt(lb) ({	\
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_FRWD_PKT) != 0); \
})

#define lb_setaltinterface(lb, set) ({ \
	ASSERT(lb_sane(lb)); \
	if (set) { \
		(lb)->control.flags |= LBF_ALT_INTF; \
	} else { \
		(lb)->control.flags &= ~LBF_ALT_INTF; \
	} \
})

#define lb_is_hwa_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_HWA_PKT) != 0); \
})

#define lb_set_hwa_pkt(lb) ({ \
	(lb)->control.flags |= LBF_HWA_PKT; \
})

#define lb_reset_hwa_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_HWA_PKT; \
})

#define lb_is_buf_alloc(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_BUF_ALLOC) != 0); \
})

#define lb_set_buf_alloc(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_BUF_ALLOC; \
})

#define lb_reset_buf_alloc(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_BUF_ALLOC; \
})

#define lb_is_rx_corrupted(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_RX_CORRUPTED) != 0); \
})

#define lb_set_rx_corrupted(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_RX_CORRUPTED; \
})

#define lb_reset_rx_corrupted(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_RX_CORRUPTED; \
})

#define lb_is_hwa_3bpkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_HWA_3BPKT) != 0); \
})

#define lb_set_hwa_3bpkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_HWA_3BPKT; \
})

#define lb_reset_hwa_3bpkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_HWA_3BPKT; \
})

#define lb_is_mgmt_tx_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_MGMT_TX_PKT) != 0); \
})

#define lb_data_is_local(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & (LBF_MGMT_TX_PKT | LBF_BUF_ALLOC)) != 0); \
})

#define lb_set_mgmt_tx_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_MGMT_TX_PKT; \
})

#define lb_reset_mgmt_tx_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_MGMT_TX_PKT; \
})

#define lb_is_hwa_hostreorder(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_HWA_HOSTREORDER) != 0); \
})

#define lb_set_hwa_hostreorder(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_HWA_HOSTREORDER; \
})

#define lb_reset_hwa_hostreorder(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_HWA_HOSTREORDER; \
})

/* HME data */
#define lb_has_hme_data(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_HME_DATA) != 0); \
})

#define lb_set_hme_data(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_HME_DATA; \
})

#define lb_reset_hme_data(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_HME_DATA; \
})

/* DDBM packet */
#define lb_is_hwa_ddbm_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_HWA_DDBM_PKT) != 0); \
})

#define lb_set_hwa_ddbm_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_HWA_DDBM_PKT; \
})

#define lb_reset_hwa_ddbm_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_HWA_DDBM_PKT; \
})

/* SWPQP packet */
#define lb_is_swpqp_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_SWPQP_PKT) != 0); \
})

#define lb_set_swpqp_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_SWPQP_PKT; \
})

#define lb_reset_swpqp_pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags &= ~LBF_SWPQP_PKT; \
})

/* if set, lb_free() skips de-alloc
 * Keep the below as macros for now, as PKTPOOL macros are not defined here
 */
#define lb_setpool_tx(lb, set, _pool) ({ \
	if (set) { \
		ASSERT(POOLID(_pool) <= PKTPOOL_MAXIMUM_ID); \
		PPLBUF(lb)->poolid = POOLID(_pool); \
	} else { \
		PPLBUF(lb)->poolid = PKTPOOL_INVALID_ID; \
	} \
})

#define lb_setpool_rx(lb, set, _pool) ({ \
	if (set) { \
		ASSERT(POOLID(_pool) <= PKTPOOL_MAXIMUM_ID); \
		LBFP(lb)->fraginfo.poolid = POOLID(_pool); \
	} else { \
		LBFP(lb)->fraginfo.poolid = PKTPOOL_INVALID_ID; \
	} \
})

#define lb_setpool(lb, set, _pool) ({ \
	if (set) { \
		ASSERT(POOLID(_pool) <= PKTPOOL_MAXIMUM_ID); \
		if (LBFP(lb)->control.flags & (LBF_TX_FRAG | LBF_MGMT_TX_PKT)) { \
			PPLBUF(lb)->poolid = POOLID(_pool); \
		} else { \
			LBFP(lb)->fraginfo.poolid = POOLID(_pool); \
		} \
	} else { \
		if (LBFP(lb)->control.flags & (LBF_TX_FRAG | LBF_MGMT_TX_PKT)) { \
			PPLBUF(lb)->poolid = PKTPOOL_INVALID_ID; \
		} else { \
			LBFP(lb)->fraginfo.poolid = PKTPOOL_INVALID_ID; \
		} \
	} \
})

#define lb_getpool(lb) ({ \
	ASSERT(lb_sane(lb)); \
	ASSERT(LB_POOL(lb) <= PKTPOOL_MAXIMUM_ID); \
	PKTPOOL_ID2PTR(LB_POOL(lb)); \
})

#define lb_pool(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(LB_POOL(lb) != PKTPOOL_INVALID_ID); \
})

#ifdef BCMDBG_POOL
#define lb_poolstate(lb) ({ \
	ASSERT(lb_sane(lb)); \
	ASSERT(LB_POOLSTATE(lb) != PKTPOOL_INVALID_ID); \
	LB_POOLSTATE(lb); \
})

#define lb_setpoolstate(lb, state) ({ \
	ASSERT(lb_sane(lb)); \
	ASSERT(LB_POOL(lb) != PKTPOOL_INVALID_ID); \
	LB_POOLSTATE(lb) = (state); \
})
#endif

#define lb_80211pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_80211_PKT) != 0); \
})

#define lb_set80211pkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_80211_PKT; \
})

#ifdef WL_MONITOR
#define lb_monpkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(((lb)->control.flags & LBF_MON_PKT) != 0); \
})

#define lb_setmonpkt(lb) ({ \
	ASSERT(lb_sane(lb)); \
	(lb)->control.flags |= LBF_MON_PKT; \
})
#endif /* WL_MONITOR */

#define lb_fragpush(lb, ix, _len) ({ \
	uint __len = (_len); \
	(lb)->fraginfo.data_buf_haddr64[ix].lo -= __len; \
	(lb)->fraginfo.host_datalen[(ix)] += __len; \
})

#define lb_fragpull(lb, ix, _len) ({ \
	uint __len = (_len); \
	ASSERT(__len <= (lb)->control.len); \
	(lb)->fraginfo.data_buf_haddr64[ix].lo += __len; \
	(lb)->fraginfo.host_datalen[(ix)] -= __len; \
})

#define lb_adj_data(lb, buf, sz, h) ({ \
	LB_HEAD_B(lb) = (uchar *)(buf); \
	LB_END_B(lb) = LB_HEAD_B(lb) + (sz); \
	(lb)->control.data = LB_HEAD_B(lb) + (h); \
})
#endif	/* _hnd_pp_lbuf_h_ */
