/*
 * 802.11ax uplink MU scheduler and statistics module.
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_ulmu.c 808799 2022-02-28 06:53:59Z $
 */

#ifdef WL_ULMU

/* Enable to turn on driver triggered UTXD mode
 * #define ULMU_DRV
 */
#define ULMU_DRV

#ifndef BYTE_INTERFACE
#define ULMU_NO_R2C
#endif

/* Enable to turn on infinite triggering
 * #define TAF_INF_UL_TRIGGER
 */

/* XXX: Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlioctl.h>
#include <802.11.h>
#include <wl_dbg.h>
#include <wlc_types.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_dbg.h>
#include <wlc_dump.h>
#include <wlc_iocv_cmd.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>
#include <wlc_ulmu.h>
#include <wlc_ht.h>
#include <wlc_he.h>
#include <wlc_scb_ratesel.h>
#include <wlc_bmac.h>
#include <wlc_hw_priv.h>
#include <wlc_ratelinkmem.h>
#include <wlc_lq.h>
#include <wlc_ampdu_cmn.h>
#include <wlc_ampdu_rx.h>
#include <wlc_txbf.h>
#if defined(TESTBED_AP_11AX) || defined(BCMDBG)
#include <wlc_tx.h>
#endif /* TESTBED_AP_11AX || BCMDBG */
#include <wlc_txcfg.h>
#include <wlc_mutx.h>
#include <wlc_fifo.h>
#include <wlc_musched.h>
#ifdef WLTAF
#include <wlc_taf.h>
#endif
#include <wlc_macdbg.h>
#include <wlc_qoscfg.h>

/* forward declaration */
#define ULMU_TRIG_USRCNT_MAX	8
#define ULMU_TRIG_USRCNT_DEF	8
#define ULMU_SCHPOS_INVLD	(-1)
#define ULMU_TXDUR_MAX		5380
#define ULMU_MCTL_DURCFG_AVG	0
#define ULMU_MCTL_DURCFG_MAX	1
#define ULMU_MCTL_DURCFG_MIN	2
#define ULMU_RMEMIDX_FIRST	AMT_IDX_ULOFDMA_RSVD_START
#define ULMU_RMEMIDX_LAST	(ULMU_RMEMIDX_FIRST + (AMT_IDX_ULOFDMA_RSVD_SIZE - 1))
#define ULMU_USRCNT_MIN		2
#define ULMU_MAX_ULC_SZ		32 /* max number of ULC resources */
#define ULMU_MAX_UTXD_PER_STA	8 /* max allowed utxd per SCB */

#define ULMU_TRIGGER_SF		6 /* 64 byte trigger unit */
#define ULMU_TRIGGER_INT_1US	(1)	/* 1 us disables trigger delay by ucode */
#define ULMU_TRIGGER_INT_AUTO	((uint16) -1) /* auto trigger interval scheme */

#define EVICT_CLIENT		FALSE
#define ADMIT_CLIENT		TRUE

/* special WFA test settings */
#define ULMU_TRIGGER_INTVL_WFA	(10000) /* For WFA test fix trigger interval at 2000us */
#define ULMU_TRIGGER_BURST_WFA	1
#define ULMU_TRIGGER_MAXTW_WFA	1

#define ULSTS_FRAMEID_SHIFT		7
#define ULSTS_UTXD_PAD			4

#define ULMU_ACC_BUFSZ_MIN	(1 << ULMU_TRIGGER_SF) /* 64 byte trigger unit */
#define ULMU_ACC_BUFSZ_MAX	(16 * 1024 * 1024) /* 16MB */
#define ULMU_DRV_DEFAULT_BUFSZ	(2 * 1024 * 1024) /* 2MB */

#define ULMU_RXMON_BYTECNT_INIT_TIME (50) /* 50 ms worth of bytes */
#define ULMU_RXMON_FILL_THRSH (115) /* fill threshold, 90 % of 128 */
#define ULMU_RXMON_RAVG_EXP (8)
#define ULMU_RXMON_EVICT_THRSH (20)
#define ULMU_RXMON_RATE_MULTIPLIER (1)
#define ULMU_TRIG_WAIT_START_THRSH (1)
#define ULMU_TRIG_WAIT_MS (15) /* ms before sending next trigger after qos null rx */
#define ULMU_ZERO_BYTES_THRSH (1)

/* utxd commands for global update */
#define ULMU_UTXD_GLBUPD	((D11_UCTX_CMD_GLBUPD << D11_UCTX_CMD_TYPE_SHIFT) & \
				D11_UCTX_CMD_TYPE_MASK)

/* utxd commands for user update */
#define ULMU_UTXD_USRUPD	((D11_UCTX_CMD_USRUPD << D11_UCTX_CMD_TYPE_SHIFT) & \
				D11_UCTX_CMD_TYPE_MASK)
#define ULMU_UTXD_USRUPD_TRFC	(ULMU_UTXD_USRUPD | D11_UCTXCMD_USRUPD_TRFC_MASK)
#define ULMU_UTXD_USRUPD_RATE	(ULMU_UTXD_USRUPD | D11_UCTXCMD_USRUPD_RATE_MASK)
#define ULMU_UTXD_USRADD_ALL	(ULMU_UTXD_USRUPD | (D11_UCTXCMD_USRUPD_TRFC_MASK | \
				D11_UCTXCMD_USRUPD_RATE_MASK))
#ifdef ULMU_NO_R2C
#define ULMU_UTXD_USRUPD_R2C	(ULMU_UTXD_USRUPD_TRFC)
#else
#define ULMU_UTXD_USRUPD_R2C	(ULMU_UTXD_USRUPD_TRFC | D11_UCTXCMD_USRUPD_R2C_MASK | \
				D11_UCTXCMD_USRUPD_RFSH_MASK)
#endif

/* utxd commands for user delete */
#define ULMU_UTXD_USRDEL	((D11_UCTX_CMD_USRDEL << D11_UCTX_CMD_TYPE_SHIFT) & \
				D11_UCTX_CMD_TYPE_MASK)
#define ULMU_UTXD_USRDEL_HARD	ULMU_UTXD_USRDEL
#define ULMU_UTXD_USRDEL_SOFT	(ULMU_UTXD_USRDEL | D11_UCTXCMD_USRDEL_SOFT_MASK)

/* eviction reason codes */
typedef enum {
	EVICT_REASON_PPS,
	EVICT_REASON_RXMON,
	EVICT_REASON_MINUSERS,
	EVICT_REASON_BWUPD,
	EVICT_REASON_TRIG_STALL,
	EVICT_REASON_NUM_TYPES
} ulmu_evict_types;

const char* ulmu_evict_reason_names[EVICT_REASON_NUM_TYPES] = {
	"pps",
	"rxmon",
	"minusers",
	"bwupd",
	"trigstall",
};

typedef enum {
	UTXD_TYPE_GLBUPD,
	UTXD_TYPE_USRADD_ALL,
	UTXD_TYPE_USRUPD_RATE,
	UTXD_TYPE_USRUPD_DATA,
	UTXD_TYPE_USRDEL,
	UTXD_NUM_TYPES
} ulmu_utxd_types;

const char* ulmu_utxd_type_names[UTXD_NUM_TYPES] = {
	"glbupd",
	"usradd",
	"rate",
	"data",
	"usrdel"
};

#define ULMU_UTXDQ_PKTS_MAX	64

/* pkt pool macros */
#define ULMU_UTXD_POOL_SIZE		ULMU_UTXDQ_PKTS_MAX
#define ULMU_UTXD_BUF_SIZE		192
#define ULMU_UTXD_MAX_REF_CNT	2
#ifdef WLTAF
#define ULMU_UTXD_FREE(ulmu, tag, p) \
	({ \
		ASSERT(tag->ref_cnt); \
		tag->ref_cnt--; \
		if (tag->ref_cnt == 0) { \
			PKTFREE(ulmu->wlc->osh, p, TRUE); \
			ulmu->utxd_alloc_cnt--; \
		} \
	})
#define ULMU_GET_TAG_FROM_UTXD_PKT(osh, p) \
	(wlc_ulpkttag_t*) ((uint8 *)PKTDATA(osh, p) - WLULPKTTAGLEN)
#endif

#define ULMU_NUM_MPDU_DLFT		128
#define ULMU_NUM_MPDU_THRSH		64
#define ULMU_MLEN_KBSZ			3	// mlen in KB unit
#define ULMU_BUFSZ_DFLT			(ULMU_NUM_MPDU_DLFT*ULMU_MLEN_KBSZ)	// in KB unit
#define ULMU_BUFSZ_THRSH		(ULMU_NUM_MPDU_THRSH*ULMU_MLEN_KBSZ)	// in KB unit
#define ULMU_AGGN_HEADROOM		0	// headroom for trigger dur est. unit is # mpdus
#define ULMU_STOPTRIG_QNULL_THRSH	3	// # of qnull frames to stop trigger
#define ULMU_DURSZ_DFLT			1000

#define ULMU_LEGACY_MODE		0
#define ULMU_UTXD_MODE			1
#define ULMU_MLEN_INIT			1460
#define ULMU_ON				1
#define ULMU_OFF			0

#define ULMU_IS_UTXD(mode)		((mode) == ULMU_UTXD_MODE)

#define UTXD_SYNC_FIRST_SHIFT		0	// first utxd
#define UTXD_SYNC_FIRST_MASK		0x01
#define UTXD_SYNC_LAST_SHIFT		1	// last utxd
#define UTXD_SYNC_LAST_MASK		0x02
#define UTXD_SYNC_USER_CNT_SHIFT	4	// user count
#define UTXD_SYNC_USER_CNT_MASK		0xF0

#define UTXD_SYNC_FIRST_GET(f)		(((f) & UTXD_SYNC_FIRST_MASK) >> \
					UTXD_SYNC_FIRST_SHIFT)
#define UTXD_SYNC_FIRST_SET(f, x)	do {\
	(f) &= ~UTXD_SYNC_FIRST_MASK; \
	(f) |= ((x) << UTXD_SYNC_FIRST_SHIFT) & UTXD_SYNC_FIRST_MASK; \
	} while (0)
#define UTXD_SYNC_LAST_GET(f)		(((f) & UTXD_SYNC_LAST_MASK) >> \
						UTXD_SYNC_LAST_SHIFT)
#define UTXD_SYNC_LAST_SET(f, x)	do {\
		(f) &= ~UTXD_SYNC_LAST_MASK; \
		(f) |= ((x) << UTXD_SYNC_LAST_SHIFT) & UTXD_SYNC_LAST_MASK; \
		} while (0)
#define UTXD_SYNC_USER_CNT_GET(f)		(((f) & UTXD_SYNC_USER_CNT_MASK) >> \
							UTXD_SYNC_USER_CNT_SHIFT)
#define UTXD_SYNC_USER_CNT_SET(f, x)	do {\
			(f) &= ~UTXD_SYNC_USER_CNT_MASK; \
			(f) |= ((x) << UTXD_SYNC_USER_CNT_SHIFT) & UTXD_SYNC_USER_CNT_MASK; \
			} while (0)

#define ULMU_FLAGS_USCHED_SHIFT		0	// ucode ul scheculer
#define ULMU_FLAGS_USCHED_MASK		0x0001
#define ULMU_FLAGS_HBRNT_SHIFT		1	// hibernate mode
#define ULMU_FLAGS_HBRNT_MASK		0x0002
#define ULMU_FLAGS_AUTOULC_SHIFT	2	// auto ulc
#define ULMU_FLAGS_AUTOULC_MASK		0x0004
#define ULMU_FLAGS_DSCHED_SHIFT		3	// driver-based scheduler
#define ULMU_FLAGS_DSCHED_MASK		0x0008

#define ULMU_FLAGS_HBRNT_GET(f)		(((f) & ULMU_FLAGS_HBRNT_MASK) >> \
					ULMU_FLAGS_HBRNT_SHIFT)
#define ULMU_FLAGS_HBRNT_SET(f, x)	do {\
	(f) &= ~ULMU_FLAGS_HBRNT_MASK; \
	(f) |= ((x) << ULMU_FLAGS_HBRNT_SHIFT) & ULMU_FLAGS_HBRNT_MASK; \
	} while (0)
#define ULMU_FLAGS_USCHED_GET(f)	(((f) & ULMU_FLAGS_USCHED_MASK) >> \
					ULMU_FLAGS_USCHED_SHIFT)
#define ULMU_FLAGS_USCHED_SET(f, x)	do {\
	(f) &= ~ULMU_FLAGS_USCHED_MASK; \
	(f) |= ((x) << ULMU_FLAGS_USCHED_SHIFT) & ULMU_FLAGS_USCHED_MASK; \
	} while (0)
#define ULMU_FLAGS_AUTOULC_GET(f)	(((f) & ULMU_FLAGS_AUTOULC_MASK) >> \
					ULMU_FLAGS_AUTOULC_SHIFT)
#define ULMU_FLAGS_AUTOULC_SET(f, x)	do {\
	(f) &= ~ULMU_FLAGS_AUTOULC_MASK; \
	(f) |= ((x) << ULMU_FLAGS_AUTOULC_SHIFT) & ULMU_FLAGS_AUTOULC_MASK; \
	} while (0)
#define ULMU_FLAGS_DSCHED_GET(f)	(((f) & ULMU_FLAGS_DSCHED_MASK) >> \
					ULMU_FLAGS_DSCHED_SHIFT)
#define ULMU_FLAGS_DSCHED_SET(f, x)	do {\
	(f) &= ~ULMU_FLAGS_DSCHED_MASK; \
	(f) |= ((x) << ULMU_FLAGS_DSCHED_SHIFT) & ULMU_FLAGS_DSCHED_MASK; \
	} while (0)

#define OPERAND_SHIFT			4
#define ULMU_TRIG_FRMTY			16 // trigger frame types
#define ULMU_SCB_STATS_NUM		16
#define MAX_USRHIST_PERLINE		8

#define ULMU_SCB_IDLEPRD		10
#define ULMU_SCB_RX_PKTCNT_THRSH	100

#define ULMU_SCB_MINRSSI		(-110)

#define ULMMU_TAGID_STRT		16
#define ULMU_DUMP_MMU			1
#define ULMU_DUMP_OMU			2
#define ULMU_DUMP_BOTH			(ULMU_DUMP_MMU | ULMU_DUMP_OMU)

/* This approach avoids overflow of a 32 bits integer */
#define PERCENT(total, part) (((total) > 10000) ? \
	((part) / ((total) / 100)) : \
	(((part) * 100) / (total)))

#define ULMU_SCB_LOWM		40
#define ULMU_SCB_HIWM		80
#define ULMU_SCB_QNCNT_THRSH	50
#define ULMU_SCB_FLCNT_THRSH	30

#define ULMU_QS_UPSCALE_FACTOR	8
#define ULMU_QS_EMA_ALPHA	8
#define ULMU_QS_EMA(new, cur, alpha)	\
	do {				\
		new -= new >> alpha;	\
		new += cur >> alpha;	\
	} while (0);

#define D11AX_RUTYPE_BW20_BASE (D11AX_RU242_TYPE - BW_20MHZ)
#define D11AX_BW2RUTYPE(x) ((x) + D11AX_RUTYPE_BW20_BASE)

typedef enum {
	ULMU_SCB_INIT = 0,
	ULMU_SCB_ADMT = 1,
	ULMU_SCB_EVCT = 2
} ulmuScbAdmitStates;

typedef enum {
	ULMU_ADMIT_AUTO = 0,
	ULMU_ADMIT_ALWAYS = 1,
	ULMU_ADMIT_TWT_ONLY = 2,
	ULMU_ADMIT_ALLNSTS = 3
} ulmuAdmitStates;

typedef struct ulmu_gstats {
	uint32 usrhist[ULMU_TRIG_USRCNT_MAX];	/* histogram of N_usr in trigger */

	/* Histogram of non-qosnull data frames received for N_usr trigger.
	 * Meaning, N*usrhist[N-1] = usrDataFrmhist[N-1] + (qosnull+fail count for N-usr trigger
	 */
	uint32 usrDataFrmhist[ULMU_TRIG_USRCNT_MAX];
	uint32 lcnt[AMPDU_MAX_HE];		/* total lfifo cnt per mcs and nss */
	uint32 gdfcscnt[AMPDU_MAX_HE];		/* total good FCS cnt per mcs and nss */
	uint32 tx_cnt[MUSCHED_RU_TYPE_NUM];	/* total tx cnt per ru size */
	uint32 txsucc_cnt[MUSCHED_RU_TYPE_NUM];	/* succ tx cnt per ru size */
	uint8 ru_idx_use_bmap[MUSCHED_RU_BMP_ROW_SZ][MUSCHED_RU_BMP_COL_SZ];
} ulmu_gstats_t;

/* Driver uplink info */
typedef struct ulmu_drv_counters {
	uint32 nutxd;			/* num posted utxd */

	/* Callbacks and completions */
	uint32 nutxd_status;		/* num trigger frame status */
	uint32 ncallback;		/* num callback fn invocation */
	uint32 threshold_callback;	/* bytelimit callback */
	uint32 multi_callback;		/* multi callback threshold */
	uint32 spurious_callback;	/* cb was not expected */
	uint32 no_callback;		/* tx status, no cb invoked */
	uint32 no_request;		/* txstatus with no pending utxd request */
	uint32 zero_bytes;		/* txstatus, zero bytes recvd */
	uint32 invalid_rate;		/* txstatus, unknown rate */
	uint32 overrun;			/* txstatus, more ul rel units than requested */

	uint32 fid_mismatch;		/* Fid from trig status does not match request */

	/* Byte counts */
	uint32 nbytes_triggered;	/* utxd byte request */
	uint32 nbytes_received;		/* bytes received for current request */

	/* us counts */
	uint32 nus_triggered;		/* utxd us request */
	uint32 nus_received;		/* us received for current request */

	/* UTXD request termination reasons */
	uint32 stop_byte_limit;		/* request terminated, bytelimit reached */
	uint32 stop_qos_null;		/* request terminated, QoS NULL recvd */
	uint32 stop_timeout;		/* request terminated, rxtimeout */
	uint32 stop_suppress;		/* request terminated, suppressed */
	uint32 stop_trigcnt;		/* request terminated, trig count exceeded */
	uint32 stop_watchdog;		/* request terminated, watchdog fired */
	uint32 stop_evicted;		/* request terminated, peer evicted */
} ulmu_drv_counters_t;

#ifdef WLTAF
/* TAF UL stats */
typedef struct ulmu_taf_stats {
	/* counters */
	uint32 nutxd; /* number of utxd sent */
	uint32 nupd; /* number of trigger req sent */
	uint32 nqosnull; /* number of qos null */
	uint32 ntimeout; /* number of timeouts */
	uint32 nsuppress; /* number of supress */

	/* Byte counts */
	uint64 bytes_rel;	/* total bytes released */
	uint64 bytes_trigd;	/* total bytes triggered */
	uint64 bytes_recvd; /* total bytes recvd */
	uint32 rxmon_min; /* min byte cnt reported by rxmon */
	uint32 rxmon_max; /* max byte cnt reported by rxmon */

	/* duration */
	uint64 usec_trigd; /*  total duration triggered */
	uint64 usec_txdur; /*  total actual tx duration */
	uint32 usec_qosnull; /* total qosnull duration */
	uint32 usec_timeout; /* total timeout duration */
	uint32 usec_suppress; /* total suppress duration */

	/* trigger wait cumulatives */
	uint32 trig_wait_cnt; /* number of times it entered trigger wait mode */
	uint32 trig_wait_cum_retry_cnt; /* total retries */
	uint32 trig_wait_cum_ms; /* total wait time */

	/* evaluated stats */

	/* rxmon byte cnt */
	uint32 rxmon_min_bytes; /* min byte cnt reported by rxmon */
	uint32 rxmon_max_bytes; /* max byte cnt reported by rxmon */

	/* averages */
	/* avg bytes released by TAF per trigger req */
	uint32 relbytes_per_utxd;
	/* avg duration released by TAF per trigger req */
	uint32 reldur_per_utxd;
	/* avg. bytes actually recvd per trigger req sent by TAF */
	uint32 rxbytes_per_utxd;
	/* avg. padded bytes per trigger req sent by TAF */
	uint32 padded_bytes_per_trig;
	/* avg. duration actually scheduled by ucode per trigger req by TAF */
	uint32 txdur_per_utxd;
	/* number of qos null recvd out of total triggers */
	uint32 qosnull_per_trig;
	/* number of timeout recvd out of total triggers */
	uint32 timeout_per_trig;
	/* number of suppress recvd out of total triggers */
	uint32 suppress_per_trig;

	/* efficiency */
	/* what % out of total bytes actually recvd is released by TAF */
	uint64 relbytes;
	/* what % out of total duration actually scheduled by ucode is released by TAF */
	uint32 reldur;
	/* % airtime unused due to qos nulls out of total airtime scheduled by ucode */
	uint32 qosnull_airtime;
	/* % airtime unused due to suppress out of total airtime scheduled by ucode */
	uint32 suppress_airtime;
	/* % airtime unused due to timeouts out of total airtime scheduled by ucode */
	uint32 timeout_airtime;
	/* % airtime padded out of total airtime scheduled by ucode */
	uint32 padded_airtime;
	/* % airtime unused , including due to qos nulls, out of total airtime scheduled by ucode */
	uint32 unused_airtime;

	/* trigger wait averages */
	uint32 avg_trig_wait_retries; /* avg retries */
	uint32 avg_trig_wait_time; /* avg wait time between trigger wait mode ON  and OFF */
} ulmu_taf_stats_t;

#define ULMU_SCB_IS_ULMMU(ulmu_scb)	(ulmu_scb->tagid >= ULMMU_TAGID_STRT)
#define ULMU_SCB_STATS(ulmu_scb)	(ULMU_SCB_IS_ULMMU(ulmu_scb) ? \
		ulmu_scb->scb_mmu_stats : \
		ulmu_scb->scb_omu_stats)
#define ULMU_TAF_SCB_STATS(ulmu_scb)	(ULMU_SCB_IS_ULMMU(ulmu_scb) ? \
		(&(ulmu_scb)->scb_mmu_stats->taf_stats) : \
		(&(ulmu_scb)->scb_omu_stats->taf_stats))

#if defined(BCMDBG_DUMP_UMSCHED)
static int wlc_ulmu_taf_stats_upd(wlc_ulmu_info_t *ulmu, scb_t *scb);
static int wlc_ulmu_taf_stats_reset(wlc_ulmu_info_t *ulmu, scb_t *scb);
static void wlc_ulmu_dump_bulk_commit_stats(wlc_ulmu_info_t *ulmu, bcmstrbuf_t *b);
static void wlc_ulmu_taf_print_scb_stats(wlc_ulmu_info_t *ulmu, scb_t *scb, bcmstrbuf_t *b);
static void wlc_ulmu_dump_utxd_in_transit_stats(wlc_ulmu_info_t *ulmu, bcmstrbuf_t *b);
#endif /* BCMDBG_DUMP_UMSCHED */

#endif /* WLTAF */

typedef struct ulmu_rssi_stats {
	int16 min_rssi; /* min rssi */
	int16 max_rssi; /* max rssi */
	int32 avg_rssi; /* avg rssi */
} ulmu_rssi_stats_t;
typedef struct ulmu_stats {
	uint32 trigtp_cnt[ULMU_TRIG_FRMTY];	/* placeholder cnts for trigger types */
	uint32 qncnt;				/* total qos-null cnts */
	uint32 agglen;				/* sum mpdu len */
	uint32 nupd;				/* total txstatus upd */
	uint32 nfail;				/* counter for lcnt = 0 */
	uint32 nadmit;				/* admission counter */
	uint32 admit_dur;			/* total admission time */
	uint32 nbadfcs;				/* counter for badfcs > 0 */
	uint32 nvldrssi;			/* counter for rssi being valid */
	uint32 txop;				/* avg TXOP */
	uint32 sum_lcnt;			/* sum of lfifo cnts */
	uint32 lcnt[AMPDU_MAX_HE];		/* total lfifo cnt per mcs and nss */
	uint32 gdfcscnt[AMPDU_MAX_HE];		/* total good FCS cnt per mcs and nss */
	ulmu_rssi_stats_t rssi_stats;
	uint32 tx_cnt[MUSCHED_RU_TYPE_NUM];	/* total tx cnt per ru size */
	uint32 txsucc_cnt[MUSCHED_RU_TYPE_NUM];	/* succ tx cnt per ru size */
	uint32 tx_nsscnt[MAX_STREAMS_SUPPORTED];	/* total tx cnt per nss */
	uint32 txsucc_nsscnt[MAX_STREAMS_SUPPORTED];	/* successful tx cnt per nss */
	uint8 ru_idx_use_bmap[MUSCHED_RU_BMP_ROW_SZ][MUSCHED_RU_BMP_COL_SZ];
	int32 cur_qsz;				/* cur queue size from QosControl field */
	int32 avg_qsz;				/* avg queue size from QosControl field */
#ifdef ULMU_DRV
	ulmu_drv_counters_t	drv_counters;	/* Driver interface counterblock */
#endif
#ifdef WLTAF
	ulmu_taf_stats_t	taf_stats;	/* Driver interface counterblock */
#endif
	uint8 evict_hist[EVICT_REASON_NUM_TYPES]; /* eviction histogram */
} ulmu_stats_t;

typedef struct ulmu_drv_config {
	uint32	drv_requested_bytes;		/* Global driver requested byte count */
	uint32	drv_watchdog;			/* Global driver watchdog threshold */
#ifdef BCMDBG
	uint32	drv_autotrigger;		/* Auto trigger flag, enables retriggering */
#endif
} ulmu_drv_config_t;

/* module info */
typedef struct wlc_ulmu_info {
	wlc_info_t *wlc;
	uint16	flags;
	int	scbh;
	int16	policy;
	uint8	maxn[D11_REV128_BW_SZ];	/* maximum number of users triggered for ul ofdma tx */
	uint16	num_usrs;		/* number of users admitted to ul ofdma scheduler */
	uint16	txlmt;
	scb_t	*scb_list[ULMU_USRCNT_MAX];
	bool	is_start;
	int	frameid;
	int8	mode;
	uint8	ulc_sz;
	d11ulo_trig_txcfg_t txd;
	ulmu_gstats_t *omu_gstats;
	ulmu_gstats_t *mmu_gstats;
	int16	num_scb_ulstats;
	uint8	min_ulofdma_usrs;	/* minimum number of users to start ul ofdma */
	uint16	csthr0;			/* min lsig len to not clear CS for basic/brp trig */
	uint16	csthr1;			/* min lsig len to not clear CS for all other trig */
	struct	spktq utxdq;		/* utxd q in SW */
	uint16	dl_weight;		/* used for manual setting of ratio dl/ul */
	uint16	ul_weight;		/* used for manual setting of ratio dl/ul */
	uint16	qnull_thrsh;		/* # consecutive rx qnull to stop trigger */
	uint16	g_tagid;		/* global tagid for grouping */

	/* admit / evict params */
	int	min_rssi;
	uint32	rx_pktcnt_thrsh;   /* threshold of num of pkts per second to enable ul-ofdma */
	uint16	idle_prd;
	bool	ban_nonbrcm_160sta; /* No ulofdma admission for non-brcm 160Mhz-capable sta */
	bool	brcm_only;		/* Only admit brcm client for ulofdma */
	uint8	always_admit;		/* 0: use additional admit/evict params like
					 * min_rssi, rx_pktcnt_thrsh, wlc_scb_ampdurx_on();
					 * 1: always admit ulofdma capable station;
					 * 2: admit trig-enab twt users only
					 * 3: admit all NSTS STAs
					 */
	uint16	g_ucfg;			/* global ucfg info */
	/* driver UTXD trigger global configuration */
	ulmu_drv_config_t drv_gconfig;
	bool	is_run2complete;
#ifdef WLTAF
	uint8	bulk_commit;
	uint8	zero_bytes_thrsh;
	uint8	trig_wait_start_thrsh;
	uint8	max_aggn_release;
	uint32	trig_wait_thrsh;
	uint32	trig_wait_us;			/* retrigger wait time after qos null */
	uint16	evict_thrsh;
	uint16	rxmon_qlen;
	uint16  admit_delay;
	uint16  qlen_init_time;
	uint16	qlen_thrsh_cnt;
	uint16 nmpdu_headroom;
	uint16 qlen_incr_wait;
	uint16 qlen_incr_amount;
	uint16 qlen_incr_factor;
	uint16 qlen_incr_over;
	uint16 qlen_incr_over_phy;
	uint16 qlen_incr_over_deci;
	uint8 headroom_aggn;

	uint32	utxd_alloc_cnt;
	uint32 bulk_commit_nuser[ULMU_TRIG_USRCNT_MAX];
	uint32 nbulk_commit;
	uint8 utxd_in_transit[UTXD_NUM_TYPES];
#endif /* WLTAF */
	bool	per_bw_admn;			/* flag to control per-BW admission */
	uint8	drv_ulrtctl;	/* uses driver UL rate control (default on) */
} wlc_ulmu_info_t;

#define ULMU_DRV_REQUEST_LISTSZ	8

/* Data structure for UTXD request accmulator */
typedef struct utxd_accmulator {
	uint32 bytes_completed;			/* Number of bytes received */
#ifndef BYTE_INTERFACE
	uint32 delta_uscount;			/* Delta us to next threshold */
	uint32 next_uscount;			/* Next us count target */
	uint32 overrun_us;			/* Extra us delivered beyond req */
#else
	uint32 delta_bytecount;			/* Delta bytes to next threshold */
	uint32 next_bytecount;			/* Next bytecount target */
	uint32 overrun_bytes;			/* Extra bytes delivered beyond req */
#endif
	uint32 overrun;				/* Num times us overrun */
	uint32 npkts;				/* Num MPDU per AMPDU */
	uint32 duration;
	uint32 timeout;				/* Num times timeout occured */
} utxd_accmulator_t;

/* Structure of individual linked list elements */
struct trigger_ctx_entry {
	/* Control info */
	uint32 frameid;				/* Request cookie */
	uint32 sequence_number;			/* UTXD request sequence number */
	uint8 slot_number;			/* Storage location slot */
	uint8 state;				/* Processing state of current entry */

	/* UTXD accmulator */
	utxd_accmulator_t accmulator;

	/* Copy of UTXD request */
	packet_trigger_info_t info;
};

enum {
	ULMU_REQ_INACTIVE = 0,
	ULMU_REQ_PENDING = 1,
	ULMU_REQ_ACTIVE = 2,
	ULMU_REQ_COMPLETED = 3
};

/* Circular linked list of pending UTXD requests */
typedef struct trigger_ctx_list {
	uint8 len;			/* Pending trigger requests */
	uint8 headidx;			/* Circular req list, head 'pointer' */
	uint8 tailidx;			/* Circular req list, tail 'pointer' */
	uint32 next_sequence_number;	/* Next sequence for trigger request */
	trigger_ctx_entry_t list[ULMU_DRV_REQUEST_LISTSZ];
} trigger_ctx_list_t;

/* Interface per SCB accmulation metadata */
typedef struct ulmu_drv_scbinfo {
	trigger_ctx_entry_t *current_req;	/* Current active UTXD req */
	trigger_ctx_list_t context_list;	/* Pending UTXD request storage */
	uint32	nutxd;				/* Number of UTXDs posted */
	uint32	qosnull_count;			/* Number of Qos NULLs seen */
	uint32	prev_nutxd_status;		/* Previous UTXD status complete count */
	uint32	nutxd_status;			/* Current UTXD complete count */
	uint32	watchdog_count;			/* Number of watchdog events */
} ulmu_drv_scbinfo_t;

/* Ucode trigger status report block */
typedef struct ulmu_drv_utxd_status {
	ratespec_t rate;			/* Ratespec used */
	int    tid;
	uint32 mcs;				/* MCS index */
	uint32 nss;				/* num spatial streams */
	uint32 qn_consec_count;			/* Qos NULL count */
	uint32 qn_total_count;
	uint32 ru_idx;				/* RU index */
	uint32 triggered_bytes;			/* Number of bytes received */
	uint32 frameid;				/* Frameid of requesting UTXD */
	uint32 npkts;				/* Num MPDU per AMPDU */
	uint32 duration;
	uint32 return_status;			/* Raw trigger status from frame */
} ulmu_drv_utxd_status_t;

enum {
	ULMU_TRIG_WAIT_OFF = 0,
	ULMU_TRIG_WAIT_ENTERING = 1,
	ULMU_TRIG_WAIT_ON = 2
};

/* scb cubby */
typedef struct scb_ulmu {
	wlc_ulmu_info_t *ulmu;	/* backlink to  ulmu */
	scb_t	*scb_bl;	/**< backlink to current scb */
	int8 schpos;		/* HEMU UL scheduler pos */
	/* For UL OFDMA */
	bool rmem_upd;		/* if need to update ul_rmem */
	uint16 rmemidx;		/* ULC ratemem idx */
	d11ulmu_rmem_t *ul_rmem;
	uint16 lmemidx;		/* UL global linkmem idx */
	uint16 ucfg;		/* per scb ucfg info */
	ulmu_stats_t *scb_omu_stats; /* pointer to UL OFDMA stats */
	ulmu_stats_t *scb_mmu_stats; /* pointer to UL MUMIMO stats */
	int  trigcnt;
	uint16 ucfg1;
	uint16 aggn;
	uint16 peak_mlen;
	uint16 mlen;
	uint16 rel_cnt;		/* expect size, i.e., number of us/bytes to trigger */
	uint16 tagid;		/* tagid for grouping */

	/* admit / evict params */
	uint32	last_rx_pkts;
	uint32	start_ts;	/* timestamp of start of SU bytes rx */
	bool	try_admit;	/* flag to check if the admission attempt is in progress */
	uint8	idle_cnt;
	uint8	qnullonly_cnt;	/* consective number of qosnull only */
	uint8	fail_cnt;	/* consective number of qosnull only */
	uint8	state;		/* 0: init, 1: admit, 2: evict */
	uint8	lowwm;
	bool	rx_data;
	uint8	reserved;
	/* scheduler driver interface uplink info */
	ulmu_drv_scbinfo_t *ulinfo;
	/* bytes requested to be received */
	uint32 tgs_exprecv_bytes;
	/* bytes tgs_recv_bytes */
	uint32 tgs_recv_bytes;
	uint32 su_recv_bytes;
	uint32 su_recv_dur;	/* time duration (us) during which the su_recv_bytes was captured */
	uint32 acc_bufsz;	/* accumulated bytes */
	uint32 mlenma;		/* sum mpdu len */
	uint32 aggnma;		/* sum mpdu len */

#ifdef WLTAF
	uint32 pend_trig_cnt; /* pending trigger cnt */
	uint32 rate; /* rate at which bytes are being received */

	uint32 prev_read_ts; /* timestamp of last time byte cnt was read */
	uint8 rxmon_idle_cnt;
	uint8 zero_bytes_cnt;
	uint8 trig_wait_start_cnt;
	uint8 trig_wait_state;
	bool emptied;
	uint16 pktq_len;
	uint16 avg_pkt_size;
	uint16 peak_pkt_size;
	uint16 peak_aggn;
	uint16 trig_wait_retry_cnt; /* counter for how many retries */
	uint16 trig_tot_wait_ms;
	uint32 trig_wait_start_ts;
	uint32 trig_ts; /* timestamp of last time the byte cnt was updated */
	uint32 total_compl_utxd_dur;
	uint32 qlen_incr_prev_ts;
	bool trig_stall;
#endif /* WLTAF */
	uint32 excess_trigger_time;
} scb_ulmu_t;

#define C_BFIGEN_HESER_NBIT	15

/* cubby access macros */
#define SCB_ULMU_CUBBY(ulmu, scb)	((scb_ulmu_t **)SCB_CUBBY(scb, (ulmu)->scbh))
#define SCB_ULMU(ulmu, scb)		(*SCB_ULMU_CUBBY(ulmu, scb))

/* local declarations */

/* wlc module */
static int wlc_ulmu_wlc_init(void *ctx);

static int wlc_ulmu_doiovar(void *context, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, wlc_if_t *wlcif);

#if defined(BCMDBG_DUMP_UMSCHED) /* for ul ofdma scheduler */
static int wlc_ulmu_dump(void *ctx, bcmstrbuf_t *b);
static int wlc_ulmu_dump_clr(void *ctx);
static void wlc_ulmu_print_stats(scb_ulmu_t* ulmu_scb,
	scb_t *scb, bcmstrbuf_t *b);
static void wlc_ulmu_dump_gstats(ulmu_gstats_t* gstats,
	bcmstrbuf_t *b, char* dump_type);
static void wlc_ulmu_ru_stats(ulmu_stats_t* ul_stats,
	scb_t *scb, bcmstrbuf_t *b, bool is_160);
static void wlc_ulmu_ru_gstats(ulmu_gstats_t* gstats,
	bcmstrbuf_t *b, bool is_160);
#endif /* BCMDBG_DUMP_UMSCHED */

static int wlc_ulmu_scb_stats_init(wlc_ulmu_info_t *ulmu, scb_t *scb);
static void wlc_ulmu_scb_state_upd(void *ctx, scb_state_upd_data_t *notif_data);
static void wlc_ulmu_bsscfg_state_upd(void *ctx, bsscfg_state_upd_data_t *notif_data);
static void wlc_ulmu_set_ulpolicy(wlc_ulmu_info_t *ulmu, int16 ulsch_policy);
static int wlc_ulmu_get_ulpolicy(wlc_ulmu_info_t *ulmu);
static bool wlc_ulmu_scb_is_ulofdma(wlc_ulmu_info_t *ulmu, scb_t *scb);

/* scheduler admit control */
static int wlc_ulmu_wlc_deinit(void *ctx);

/* ul ofdma scheduler */
static void wlc_ulmu_cfg_commit(wlc_ulmu_info_t* ulmu);
static void wlc_ulmu_csreq_commit(wlc_ulmu_info_t* ulmu);
uint16 wlc_ulmu_scb_get_rmemidx(wlc_info_t *wlc, scb_t *scb);
static int wlc_ulmu_maxn_set(wlc_ulmu_info_t *ulmu);

/* scb cubby */
static int wlc_ulmu_scb_init(void *ctx, scb_t *scb);
static void wlc_ulmu_scb_deinit(void *ctx, scb_t *scb);
static uint wlc_ulmu_scb_secsz(void *ctx, scb_t *);
#if defined(DUMP_SCB_ULMU)
static void wlc_ulmu_scb_dump(void *ctx, scb_t *scb, bcmstrbuf_t *b);
#endif /* DUMP_SCB_ULMU */
static void wlc_ulmu_scb_sched_init(wlc_ulmu_info_t *ulmu, scb_t *scb, scb_ulmu_t *ulmu_scb);
static bool wlc_ulmu_scb_eligible(wlc_ulmu_info_t *ulmu, scb_t *scb);
static int wlc_ulmu_prep_utxd(wlc_ulmu_info_t *ulmu, scb_t *scb, uint16 cmd,
	void *pkt, uint32 *fid);

/* ul ofdma scb cubby */
static int8 wlc_ulmu_scb_lkup(wlc_ulmu_info_t *ulmu, scb_t *scb);
static int8 wlc_ulmu_scb_onaddr_lkup(wlc_ulmu_info_t *ulmu,
	struct ether_addr *ea, scb_t **pp_scb);
static void wlc_ulmu_fill_rt(wlc_ulmu_info_t *ulmu, scb_t *scb,
	d11ulmu_txd_t *utxd, ratespec_t *rspec, uint8 *mcs, uint8 *nss);
static void wlc_ulmu_fill_utxd(wlc_ulmu_info_t *ulmu, d11ulmu_txd_t *utxd, scb_t *scb);
static void wlc_ulmu_watchdog(void *mi);
/* dump */
static void wlc_ulmu_dump_ulofdma(wlc_ulmu_info_t* ulmu, bcmstrbuf_t *b, bool verbose);

#ifdef ULMU_DRV
static void wlc_ulmu_drv_ulmu_attach(wlc_ulmu_info_t* ulmu);
static void wlc_ulmu_drv_ulmu_detach(wlc_ulmu_info_t* ulmu);
static void wlc_ulmu_drv_watchdog(wlc_ulmu_info_t *ulmu);
static void wlc_ulmu_drv_set_watchdog(wlc_ulmu_info_t *ulmu, uint32 timeout);
static uint32 wlc_ulmu_drv_get_watchdog(wlc_ulmu_info_t *ulmu);
static void wlc_ulmu_drv_set_bytecount(wlc_ulmu_info_t *ulmu, uint32 bytecount);
static uint32 wlc_ulmu_drv_get_bytecount(wlc_ulmu_info_t *ulmu);
static void wlc_ulmu_drv_watchdog(wlc_ulmu_info_t *ulmu);
static void wlc_ulmu_drv_del_usr(wlc_info_t *wlc, scb_t *scb, scb_ulmu_t *ulmu_scb);

#ifdef BCMDBG
static void wlc_ulmu_drv_set_autotrigger(wlc_ulmu_info_t *ulmu, uint32 iter);
static uint32 wlc_ulmu_drv_get_autotrigger(wlc_ulmu_info_t *ulmu);
static bool wlc_ulmu_drv_check_trigger(ulmu_drv_scbinfo_t *ulinfo, uint32 fid);

/* TAF Stubs */
static void wlc_ulmu_drv_trigger_scb(wlc_ulmu_info_t* ulmu, uint16 iter);
#ifndef BYTE_INTERFACE
static int wlc_ulmu_drv_trigger_callback(wlc_info_t *wlc, scb_t *scb,
	packet_trigger_info_t *ti,
	void *arg, uint32 status_code, uint32 duration);
#else
static int wlc_ulmu_drv_trigger_callback(wlc_info_t *wlc, scb_t *scb,
	packet_trigger_info_t *ti,
	void *arg, uint32 status_code, uint32 bytes_consumed,
	uint32 avg_pktsz, uint32 duration);
#endif
#endif /* BCMDBG */

#if defined(BCMDBG_DUMP_UMSCHED)
static void wlc_ulmu_drv_print_scb_stats(scb_ulmu_t* ulmu_scb,
	scb_t *scb, bcmstrbuf_t *b);
static void wlc_ulmu_print_eviction_hist(scb_ulmu_t* ulmu_scb, bcmstrbuf_t *b);
#endif /* defined(BCMDBG_DUMP_UMSCHED) */

#else /* !ULMU_DRV */

#define wlc_ulmu_drv_print_scb_stats(ulmu_scb, scb, b)
#define wlc_ulmu_drv_watchdog(ulmu)

static int wlc_ulmu_release_bytes(wlc_ulmu_info_t *ulmu, scb_t *scb, uint16 bufsize);

#endif /* ULMU_DRV */

static bool wlc_ulmu_drv_ucautotrigger(wlc_ulmu_info_t *ulmu, scb_t *scb);
static void wlc_ulmu_scb_reqbytes_eval(wlc_ulmu_info_t *ulmu, scb_t *scb);
#ifdef WLTAF
static uint32 wlc_ulmu_scb_reqbytes_get(wlc_ulmu_info_t *ulmu, scb_ulmu_t* ulmu_scb, uint32 cur_ts);
static void wlc_ulmu_taf_enable(wlc_ulmu_info_t *ulmu, scb_t *scb, bool enable);
#endif
static bool wlc_ulmu_bump_bytecnt(wlc_ulmu_info_t *ulmu, scb_ulmu_t* ulmu_scb);
static void wlc_ulmu_utxd_stats_upd(wlc_ulmu_info_t *ulmu, uint32 cmd, bool incr);

static bool wlc_ulmu_admit_client(wlc_ulmu_info_t *ulmu, scb_t *scb, bool admit);
static bool wlc_ulmu_admit_ready(wlc_ulmu_info_t *ulmu, scb_t *scb);

#define UTXD_PEND_STATS_INCR(ulmu, cmd) wlc_ulmu_utxd_stats_upd(ulmu, cmd, TRUE)
#define UTXD_PEND_STATS_DECR(ulmu, cmd) wlc_ulmu_utxd_stats_upd(ulmu, cmd, FALSE)

/* iovar table */
enum {
	IOV_UMUSCHEDULER	= 0,
	IOV_LAST
};

static const bcm_iovar_t ulmu_iovars[] = {
	{"umsched", IOV_UMUSCHEDULER, 0, 0, IOVT_BUFFER, 0},
	{NULL, 0, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
*/
#include <wlc_patch.h>

/* ======== iovar dispatch ======== */
static int
wlc_ulmu_cmd_get_dispatch(wlc_ulmu_info_t *ulmu, wl_musched_cmd_params_t *params,
	char *outbuf, int outlen)
{
	int err = BCME_OK;
	wlc_info_t *wlc;
	bcmstrbuf_t bstr;
	int i, bw;
	uint16 uval16;
	uint offset;
	int8 direction; /* indicates 1:dl 2:ul 3:bi */

	BCM_REFERENCE(i);
	BCM_REFERENCE(offset);
	BCM_REFERENCE(direction);

	bcm_binit(&bstr, outbuf, outlen);
	wlc = ulmu->wlc;

	if (D11REV_LE(wlc->pub->corerev, 128))
		return BCME_UNSUPPORTED;

	if (!ulmu) {
		bcm_bprintf(&bstr, "Ul OFDMA scheduler is not initilized\n");
		return BCME_ERROR;
	}

	if (!strncmp(params->keystr, "maxn", strlen("maxn"))) {
		bcm_bprintf(&bstr, "maxN ");
		for (i = 0; i < D11_REV128_BW_SZ; i++) {
			bw = 20 << i;
			bcm_bprintf(&bstr, "bw%d: %d ", bw, ulmu->maxn[i]);
		}
		bcm_bprintf(&bstr, "\n");
#if defined(WL_PSMX)
		if (wlc->clk) {
			offset = MX_ULOMAXN_BLK(wlc);
			for (i = 0; i < D11_REV128_BW_SZ; i++) {
				bw = 20 << i;
				uval16 = wlc_read_shmx(wlc, offset+(i*2));
				if (uval16 != ulmu->maxn[i]) {
					bcm_bprintf(&bstr, "shmx bw%d: %d ", bw, uval16);
				}
			}
			bcm_bprintf(&bstr, "\n");
		}
#endif
	} else if (!strncmp(params->keystr, "maxclients", strlen("maxclients"))) {
		bcm_bprintf(&bstr, "max num of admitted clients: %d\n",
		wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA));
	} else if (!strncmp(params->keystr, "start", strlen("start"))) {
		bcm_bprintf(&bstr, "%d\n", ulmu->is_start);
	} else if (!strncmp(params->keystr, "ban_nonbrcm_160sta", strlen("ban_nonbrcm_160sta"))) {
		bcm_bprintf(&bstr, "ban nonbrcm 160sta: %d\n", ulmu->ban_nonbrcm_160sta);
	} else if (!strncmp(params->keystr, "brcm_only", strlen("brcm_only"))) {
		bcm_bprintf(&bstr, "brcm only: %d\n", ulmu->brcm_only);
#ifdef ULMU_DRV
	} else if (!strncmp(params->keystr, "drv_requested_kbytes",
		strlen("drv_requested_kbytes"))) {
		bcm_bprintf(&bstr, "%d\n", (wlc_ulmu_drv_get_bytecount(ulmu) >> 10));
	} else if (!strncmp(params->keystr, "drv_watchdog", strlen("drv_watchdog"))) {
		bcm_bprintf(&bstr, "%d\n", wlc_ulmu_drv_get_watchdog(ulmu));
#endif
	} else if (!strncmp(params->keystr, "dl_weight", strlen("dl_weight"))) {
		if (wlc->clk) {
			ulmu->dl_weight = wlc_read_shm(wlc, M_TXTRIGWT0_VAL(wlc));
		}
		bcm_bprintf(&bstr, "%d\n", ulmu->dl_weight);
	} else if (!strncmp(params->keystr, "ul_weight", strlen("ul_weight"))) {
		if (wlc->clk) {
			ulmu->ul_weight = wlc_read_shm(wlc, M_TXTRIGWT1_VAL(wlc));
		}
		bcm_bprintf(&bstr, "%d\n", ulmu->ul_weight);
#ifdef WL_ULRT_DRVR
	} else if (!strncmp(params->keystr, "drv_ulrtctl", strlen("drv_ulrtctl"))) {
		bcm_bprintf(&bstr, "%x\n", ulmu->drv_ulrtctl);
#endif
#ifdef WLTAF
	} else if (!strncmp(params->keystr, "wait_cnt",
		sizeof("wait_cnt") - 1)) {
		bcm_bprintf(&bstr, "%d\n", ulmu->trig_wait_start_thrsh);
	} else if (!strncmp(params->keystr, "evict_thrsh",
		sizeof("evict_thrsh") - 1)) {
		bcm_bprintf(&bstr, "%d\n", ulmu->evict_thrsh);
	} else if (!strncmp(params->keystr, "zero_bytes_thrsh",
		sizeof("zero_bytes_thrsh") - 1)) {
		bcm_bprintf(&bstr, "%d\n", ulmu->zero_bytes_thrsh);
	} else if (!strncmp(params->keystr, "trig_wait_ms",
		sizeof("trig_wait_ms") - 1)) {
		bcm_bprintf(&bstr, "%d\n", ulmu->trig_wait_us / 1000);
	} else if (!strncmp(params->keystr, "rxmon_qlen", sizeof("rxmon_qlen") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->rxmon_qlen);
	} else if (!strncmp(params->keystr, "qlen_thrsh_cnt", sizeof("qlen_thrsh_cnt") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->qlen_thrsh_cnt);
	} else if (!strncmp(params->keystr, "qlen_incr_wait", sizeof("qlen_incr_wait") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->qlen_incr_wait);
	} else if (!strncmp(params->keystr, "qlen_incr_amount", sizeof("qlen_incr_amount") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->qlen_incr_amount);
	} else if (!strncmp(params->keystr, "qlen_incr_factor", sizeof("qlen_incr_factor") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->qlen_incr_factor);
	} else if (!strncmp(params->keystr, "qlen_incr_over_phy",
			sizeof("qlen_incr_over_phy") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->qlen_incr_over_phy);
	} else if (!strncmp(params->keystr, "qlen_incr_over_deci",
			sizeof("qlen_incr_over_deci") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->qlen_incr_over_deci);
	} else if (!strncmp(params->keystr, "qlen_incr_over", sizeof("qlen_incr_over") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->qlen_incr_over);
	} else if (!strncmp(params->keystr, "headroom_aggn", sizeof("headroom_aggn") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->headroom_aggn);
	} else if (!strncmp(params->keystr, "max_aggn_release", sizeof("max_aggn_release") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->max_aggn_release);
	} else if (!strncmp(params->keystr, "admit_delay", sizeof("admit_delay") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->admit_delay);
	} else if (!strncmp(params->keystr, "qlen_init_time", sizeof("qlen_init_time") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->qlen_init_time);
#endif /* WLTAF */
	} else if (!strncmp(params->keystr, "per_bw_admn", sizeof("per_bw_admn") - 1)) {
			bcm_bprintf(&bstr, "%d\n", ulmu->per_bw_admn);
	} else {
		wlc_ulmu_dump_ulofdma(ulmu, &bstr, WL_MUSCHED_FLAGS_VERBOSE(params));
	}

	return err;
}

static void
wlc_ulmu_start(wlc_ulmu_info_t *ulmu, bool enable)
{
	wlc_info_t *wlc = ulmu->wlc;

	ASSERT(wlc != NULL);

	/* Set host flag to enable UL OFDMA */
	wlc_bmac_mhf(wlc->hw, MXHF0, MXHF0_ULOFDMA,
		enable ? MXHF0_ULOFDMA : 0, WLC_BAND_ALL);
	/* Set host flag to send trigger status to driver. It is on by default */
	wlc_bmac_mhf(wlc->hw, MHF5, MHF5_TRIGTXS,
		enable ? MHF5_TRIGTXS : 0, WLC_BAND_ALL);
}

static int
wlc_ulmu_cmd_set_dispatch(wlc_ulmu_info_t *ulmu, wl_musched_cmd_params_t *params)
{
	int err = BCME_OK;
	wlc_info_t *wlc;
	uint offset;
	int i, start, end;
	int16 val16;
	scb_t *scb = NULL;
	scb_ulmu_t* ulmu_scb;
	int8 schpos;
	bool upd = TRUE;
	d11ulo_trig_txcfg_t *txd;
	uint16 max_ulofdma_usrs;

	BCM_REFERENCE(i);
	BCM_REFERENCE(offset);
	BCM_REFERENCE(txd);

	if (!ulmu) {
		return BCME_ERROR;
	}

	wlc = ulmu->wlc;
	if (D11REV_LE(wlc->pub->corerev, 128)) {
		return BCME_UNSUPPORTED;
	}

	/* Don't allow the set iovar if he feature for ulmu is not set */
	if (!HE_ULOMU_ENAB(wlc->pub) && !HE_ULMMU_ENAB(wlc->pub)) {
		return BCME_EPERM;
	}

	txd = &ulmu->txd;
	max_ulofdma_usrs = wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA);

	val16 = params->vals[0];

	if (!strncmp(params->keystr, "start", strlen("start"))) {
		if (!HE_ULOMU_ENAB(wlc->pub)) {
			return BCME_EPERM;
		} else {
			ulmu->is_start = val16 ? TRUE : FALSE;
			wlc_ulmu_start(ulmu, ulmu->is_start);
		}
	} else if (!strncmp(params->keystr, "fb", strlen("fb"))) {
		if ((val16 < 0) || (val16 > 1)) {
			return BCME_RANGE;
		}
		wlc_ulmu_fburst_set(ulmu, val16 ? TRUE : FALSE);
	} else if (!strncmp(params->keystr, "mctl_mode", strlen("mctl_mode"))) {
		txd->macctl &= ~D11_ULOTXD_MACTL_MODE_NBIT;
		txd->macctl |= ((val16 == 0 ? 0 : 1) << D11_ULOTXD_MACTL_MODE_NBIT);
	} else if (!strncmp(params->keystr, "mctl_ptype", strlen("mctl_ptype"))) {
		txd->macctl &= ~D11_ULOTXD_MACTL_PTYPE_MASK;
		txd->macctl |= ((val16 << D11_ULOTXD_MACTL_PTYPE_SHIFT) &
			D11_ULOTXD_MACTL_PTYPE_MASK);
	} else if (!strncmp(params->keystr, "mctl", strlen("mctl"))) {
		txd->macctl = val16;
	} else if (!strncmp(params->keystr, "txcnt", strlen("txcnt"))) {
		txd->txcnt = val16;
	} else if (!strncmp(params->keystr, "interval", strlen("interval"))) {
		txd->interval = val16;
	} else if (!strncmp(params->keystr, "burst", strlen("burst"))) {
		if ((val16 > txd->maxtw)) {
			return BCME_RANGE;
		}
		txd->burst = val16;
	} else if (!strncmp(params->keystr, "maxn", strlen("maxn"))) {
		if (val16 > ULMU_TRIG_USRCNT_MAX ||
			val16 > max_ulofdma_usrs) {
			return BCME_RANGE;
		}
		val16 = params->vals[0] >= 0 ? params->vals[0] : 0;

		if (params->bw == -1) {
			start = 0;
			end = D11_REV128_BW_SZ-1;
		} else {
			start = end = params->bw;
		}
		for (i = start; i <= end; i++) {
			ulmu->maxn[i] = MIN(val16, wlc_txcfg_ofdma_maxn_upperbound(wlc, i));
		}

		wlc_ulmu_maxn_set(ulmu);
	} else if (!strncmp(params->keystr, "maxdur", strlen("maxdur"))) {
		if (val16 > ULMU_TXDUR_MAX || val16 <= 0) {
			return BCME_RANGE;
		}
		txd->maxdur = val16;
	} else if (!strncmp(params->keystr, "maxtw", strlen("maxtw"))) {
		if ((val16 != -1) && (val16 < txd->burst)) {
			return BCME_RANGE;
		}
		txd->maxtw = val16;
	} else if (!strncmp(params->keystr, "minidle", strlen("minidle"))) {
		txd->minidle = val16;
	} else if (!strncmp(params->keystr, "txlowat0", strlen("txlowat0"))) {
		txd->txlowat0 = val16;
	} else if (!strncmp(params->keystr, "txlowat1", strlen("txlowat1"))) {
		txd->txlowat1 = val16;
	} else if (!strncmp(params->keystr, "rxlowat0", strlen("rxlowat0"))) {
		txd->rxlowat0 = val16;
	} else if (!strncmp(params->keystr, "rxlowat1", strlen("rxlowat1"))) {
		txd->rxlowat1 = val16;
	} else if (!strncmp(params->keystr, "autorate", strlen("autorate"))) {
		if (val16 != 1 && val16 != 0) {
			return BCME_RANGE;
		}
		ulmu->g_ucfg &= (val16 == 0) ? (uint16) -1 :
			~D11_ULOTXD_UCFG_FIXRT_MASK;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if (WL_MUSCHED_FLAGS_MACADDR(params) && (schpos !=
				wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb))) {
				/* only update the scb with given ether addr matched */
					continue;
			}
			if ((scb = ulmu->scb_list[schpos]) == NULL) {
				continue;
			}
			if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
				continue;
			}
			ulmu_scb->ucfg &= (val16 == 0) ? (uint16) -1 :
				~D11_ULOTXD_UCFG_FIXRT_MASK;
		}
	} else if (!strncmp(params->keystr, "mcs", strlen("mcs"))) {
		if (!(val16 >= 0 && val16 <= 11)) {
			return BCME_RANGE;
		}
		D11_ULOTXD_UCFG_SET_MCS(ulmu->g_ucfg, val16);
		ulmu->g_ucfg |= D11_ULOTXD_UCFG_FIXRT_MASK;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if (WL_MUSCHED_FLAGS_MACADDR(params) && (schpos !=
				wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb))) {
				/* only update the scb with given ether addr matched */
					continue;
			}
			if ((scb = ulmu->scb_list[schpos]) == NULL) {
				continue;
			}
			if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
				continue;
			}
			D11_ULOTXD_UCFG_SET_MCS(ulmu_scb->ucfg, val16);
			ulmu_scb->ucfg |= D11_ULOTXD_UCFG_FIXRT_MASK;
		}
	} else if (!strncmp(params->keystr, "nss", strlen("nss"))) {
		if (val16 < 1 || val16 > 4) {
			return BCME_RANGE;
		}
		D11_ULOTXD_UCFG_SET_NSS(ulmu->g_ucfg, val16 - 1);
		ulmu->g_ucfg |= D11_ULOTXD_UCFG_FIXRT_MASK;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if (WL_MUSCHED_FLAGS_MACADDR(params) && (schpos !=
				wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb))) {
				/* only update the scb with given ether addr matched */
					continue;
			}
			if ((scb = ulmu->scb_list[schpos]) == NULL) {
				continue;
			}
			if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
				continue;
			}
			D11_ULOTXD_UCFG_SET_NSS(ulmu_scb->ucfg, val16 - 1);
			ulmu_scb->ucfg |= D11_ULOTXD_UCFG_FIXRT_MASK;
		}
	} else if (!strncmp(params->keystr, "trssi", strlen("trssi"))) {
		D11_ULOTXD_UCFG_SET_TRSSI(ulmu->g_ucfg, val16);
		ulmu->g_ucfg |= D11_ULOTXD_UCFG_FIXRSSI_MASK;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if (WL_MUSCHED_FLAGS_MACADDR(params) && (schpos !=
				wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb))) {
				/* only update the scb with given ether addr matched */
					continue;
			}
			if ((scb = ulmu->scb_list[schpos]) == NULL) {
				continue;
			}
			if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
				continue;
			}
			D11_ULOTXD_UCFG_SET_TRSSI(ulmu_scb->ucfg, val16);
			ulmu_scb->ucfg |= D11_ULOTXD_UCFG_FIXRSSI_MASK;
		}
	} else if (!strncmp(params->keystr, "cpltf", strlen("cpltf"))) {
		if ((val16 == DOT11_HETB_1XLTF_1U6S_GI && !WLC_HE_FEATURES_ULMMU(wlc->pub)) ||
			val16 >= DOT11_HETB_RSVD_LTF_GI) {
			return BCME_RANGE;
		}
		txd->txctl &= ~D11_ULOTXD_TXCTL_CPF_MASK;
		txd->txctl |= ((val16 << D11_ULOTXD_TXCTL_CPF_SHIFT) &
			D11_ULOTXD_TXCTL_CPF_MASK);
	} else if (!strncmp(params->keystr, "nltf", strlen("nltf"))) {
		if ((val16 <= DOT11_HETB_1XHELTF_NLTF || val16 >= DOT11_HETB_RSVD_NLTF)) {
			return BCME_RANGE;
		}
		txd->txctl &= ~D11_ULOTXD_TXCTL_NLTF_MASK;
		txd->txctl |= ((val16 << D11_ULOTXD_TXCTL_NLTF_SHIFT) &
			D11_ULOTXD_TXCTL_NLTF_MASK);
	} else if (!strncmp(params->keystr, "txlmt", strlen("txlmt"))) {
		ulmu->txlmt = val16;
	} else if (!strncmp(params->keystr, "mmlen", strlen("mmlen"))) {
		txd->mmlen = val16;
	} else if (!strncmp(params->keystr, "mlen", strlen("mlen"))) {
		txd->mlen = val16;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if (WL_MUSCHED_FLAGS_MACADDR(params) &&	(schpos !=
				wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb))) {
					/* only update the scb with given ether addr matched */
					continue;
				}
			if ((scb = ulmu->scb_list[schpos]) == NULL) {
				continue;
			}
			if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
				continue;
			}
			ulmu_scb->mlen = val16;
			if (val16) {
				ulmu_scb->ucfg1 |= D11_ULOTXD_UCFG1_FIXMLEN_MASK;
			} else {
				ulmu_scb->ucfg1 &= ~D11_ULOTXD_UCFG1_FIXMLEN_MASK;
			}
		}
	} else if (!strncmp(params->keystr, "aggn", strlen("aggn"))) {
		txd->aggnum = val16;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if (WL_MUSCHED_FLAGS_MACADDR(params) &&	(schpos !=
				wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb))) {
					/* only update the scb with given ether addr matched */
					continue;
				}
			if ((scb = ulmu->scb_list[schpos]) == NULL) {
				continue;
			}
			if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
				continue;
			}
			ulmu_scb->aggn = val16;
			if (val16) {
				ulmu_scb->ucfg1 |= D11_ULOTXD_UCFG1_FIXAGGN_MASK;
			} else {
				ulmu_scb->ucfg1 &= ~D11_ULOTXD_UCFG1_FIXAGGN_MASK;
			}
		}
	} else if (!strncmp(params->keystr, "ban_nonbrcm_160sta", strlen("ban_nonbrcm_160sta"))) {
		if (wlc->pub->up) {
			return BCME_NOTDOWN;
		}
		ulmu->ban_nonbrcm_160sta = (val16 != 0) ? TRUE : FALSE;
	} else if (!strncmp(params->keystr, "brcm_only", strlen("brcm_only"))) {
		if (wlc->pub->up) {
			return BCME_NOTDOWN;
		}
		ulmu->brcm_only = (val16 != 0) ? TRUE : FALSE;
	} else if (!strncmp(params->keystr, "minulusers", strlen("minulusers"))) {
		if (wlc->pub->up) {
			return BCME_NOTDOWN;
		}
		if (val16 >= 0 && (val16 <= (uint8) -1)) {
			ulmu->min_ulofdma_usrs = val16;
		} else {
			return BCME_RANGE;
		}
	} else if (!strncmp(params->keystr, "csthr0", strlen("csthr0"))) {
		ulmu->csthr0 = val16;
	} else if (!strncmp(params->keystr, "csthr1", strlen("csthr1"))) {
		ulmu->csthr1 = val16;
	} else if (!strncmp(params->keystr, "mode", strlen("mode"))) {
		if (wlc->pub->up) {
			return BCME_NOTDOWN;
		}
		ulmu->mode = val16;
		txd->macctl &= ~D11_ULOTXD_MACTL_UTXD_MASK;
		txd->macctl |= (val16 ? 1 : 0) << D11_ULOTXD_MACTL_UTXD_SHIFT;
	} else if (!strncmp(params->keystr, "qnullthrsh", strlen("qnullthrsh"))) {
		upd = FALSE;
		if (wlc->pub->up) {
			return BCME_NOTDOWN;
		}
		if (val16 <= 0) {
			return BCME_BADARG;
		}
		ulmu->qnull_thrsh = val16;
	} else if (!strncmp(params->keystr, "tagid", strlen("tagid"))) {
		if (!WLC_HE_FEATURES_ULMMU(wlc->pub) && (val16 >= ULMMU_TAGID_STRT)) {
			return BCME_RANGE;
		}
		ulmu->g_tagid = val16;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if (WL_MUSCHED_FLAGS_MACADDR(params) && (schpos !=
				wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb))) {
				/* only update the scb with given ether addr matched */
					continue;
			}
			if ((scb = ulmu->scb_list[schpos]) == NULL) {
				continue;
			}
			if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
				continue;
			}
			ulmu_scb->tagid = val16;
			if (ULMU_SCB_IS_ULMMU(ulmu_scb)) {
				SCB_ULMMU_ENABLE(scb);
			} else {
				SCB_ULMMU_DISABLE(scb);
			}
			ulmu_scb->ucfg |= D11_ULOTXD_UCFG_FIXTAGID_MASK;
			wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRADD_ALL, NULL, NULL);
			err = wlc_ulmu_post_utxd(ulmu);
		}
	} else if (!strncmp(params->keystr, "mixbw", strlen("mixbw"))) {
		if (val16) {
			txd->macctl |= D11_ULOTXD_MACTL_MIXEDBW_MASK;
		} else {
			txd->macctl &= ~D11_ULOTXD_MACTL_MIXEDBW_MASK;
		}
#if defined(BCMDBG)
	} else if (!strncmp(params->keystr, "autoulc", strlen("autoulc"))) {
		if (wlc->pub->up) {
			return BCME_NOTDOWN;
		}
		val16 = val16 ? ULMU_ON : ULMU_OFF;
		ULMU_FLAGS_AUTOULC_SET(ulmu->flags, val16);
	} else if (!strncmp(params->keystr, "usched", strlen("usched"))) {
		val16 = val16 ? ULMU_ON : ULMU_OFF;
		ULMU_FLAGS_USCHED_SET(ulmu->flags, val16);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_USCHED_MASK;
		txd->macctl1 |= (val16 << D11_ULOTXD_MACTL1_USCHED_SHIFT);
	} else if (!strncmp(params->keystr, "hibernate", strlen("hibernate"))) {
		val16 = val16 ? ULMU_ON : ULMU_OFF;
		ULMU_FLAGS_HBRNT_SET(ulmu->flags, val16);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_HBRNT_MASK;
		txd->macctl1 |= (val16 << D11_ULOTXD_MACTL1_HBRNT_SHIFT);
	} else if (!strncmp(params->keystr, "dsched", strlen("dsched"))) {
		if (wlc->pub->up) {
			return BCME_NOTDOWN;
		}
		val16 = val16 ? ULMU_ON : ULMU_OFF;
		ULMU_FLAGS_DSCHED_SET(ulmu->flags, val16);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_DSCHED_MASK;
		txd->macctl1 |= (val16 << D11_ULOTXD_MACTL1_DSCHED_SHIFT);
	} else if (!strncmp(params->keystr, "post", strlen("post"))) {
		upd = FALSE;
		for (i = 0; i < val16; i++) {
			if (wlc_ulmu_prep_utxd(ulmu, NULL, ULMU_UTXD_USRADD_ALL,
				NULL, NULL) != BCME_OK) {
				return BCME_ERROR;
			}
		}
		err = wlc_ulmu_post_utxd(ulmu);
	} else if (!strncmp(params->keystr, "enable", strlen("enable"))) {
		upd = FALSE;
		if (WL_MUSCHED_FLAGS_MACADDR(params) && (ULMU_SCHPOS_INVLD !=
			wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb))) {
			if (wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRADD_ALL,
				NULL, NULL) != BCME_OK) {
				return BCME_ERROR;
			}
			err = wlc_ulmu_post_utxd(ulmu);
		}
	} else if (!strncmp(params->keystr, "pos", strlen("pos"))) {
		/* Debug purpose: force a UL OFDMA user in given pos of list */
		upd = FALSE;
		if (WL_MUSCHED_FLAGS_MACADDR(params) && (ULMU_SCHPOS_INVLD !=
			wlc_ulmu_scb_onaddr_lkup(ulmu, &params->ea, &scb)) && scb) {
			if ((ulmu_scb = SCB_ULMU(ulmu, scb)) != NULL) {
				ulmu_scb->rmemidx = ULMU_RMEMIDX_FIRST + val16;
				if (wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRADD_ALL,
					NULL, NULL) != BCME_OK) {
					return BCME_ERROR;
				}
				err = wlc_ulmu_post_utxd(ulmu);
			}
		}
	} else if (!strncmp(params->keystr, "test", strlen("test"))) {
		upd = FALSE;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if (((scb = ulmu->scb_list[schpos]) != NULL) &&
				((ulmu_scb = SCB_ULMU(ulmu, scb)) != NULL)) {
#ifndef BYTE_INTERFACE
				ulmu_scb->rel_cnt = ULMU_DURSZ_DFLT;
#else
				ulmu_scb->rel_cnt = ULMU_BUFSZ_DFLT;
#endif
				if (wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRADD_ALL,
					NULL, NULL) != BCME_OK) {
					return BCME_ERROR;
				}
			}
		}
		wlc_ulmu_post_utxd(ulmu);
	} else if (!strncmp(params->keystr, "ulc_sz", strlen("ulc_sz"))) {
		upd = FALSE;
		if (wlc->pub->up) {
			return BCME_NOTDOWN;
		}
		if ((val16 > ULMU_MAX_ULC_SZ) || (val16 <= 0)) {
			return BCME_BADARG;
		}
		ulmu->ulc_sz = val16;
	} else if (!strncmp(params->keystr, "reclaim", strlen("reclaim"))) {
		upd = FALSE;
#if defined(BCMHWA)
		/* Stop pageout ring before reclaim all utxd */
		HWA_PKTPGR_EXPR(hwa_txfifo_disable_prep(WL_HWA_DEVP(wlc), 0));
#endif /* BCMHWA */
		wlc_ulmu_reclaim_utxd(wlc, NULL);
#if defined(BCMHWA)
		/* Resume pageout ring to posting tx packets after reclaim is done */
		HWA_PKTPGR_EXPR(hwa_txfifo_enable(WL_HWA_DEVP(wlc), 0, TRUE));
#endif /* BCMHWA */
#endif /* defined(BCMDBG) */
#ifdef ULMU_DRV
#ifdef BCMDBG
	} else if (!strncmp(params->keystr, "drv_trig_test", strlen("drv_trig_test"))) {
		if (val16 < 0) {
			val16 = -val16;
			wlc_ulmu_drv_set_autotrigger(ulmu, val16);
		} else {
			wlc_ulmu_drv_set_autotrigger(ulmu, 0);
		}

		if (val16 > 0) {
			wlc_ulmu_drv_trigger_scb(ulmu, val16);
		}

#endif /* BCMDBG */
	} else if (!strncmp(params->keystr, "drv_requested_kbytes",
			strlen("drv_requested_kbytes"))) {
		/* XXX limitation of this ioctl interface,
		 * operates on 16bit values
		 */
		wlc_ulmu_drv_set_bytecount(ulmu, (uint32)(val16 << 10));
	} else if (!strncmp(params->keystr, "drv_watchdog",
			strlen("drv_watchdog"))) {
		wlc_ulmu_drv_set_watchdog(ulmu, (uint32)val16);
#endif /* ULMU_DRV */
	} else if (!strncmp(params->keystr, "minrssi", strlen("minrssi"))) {
		ulmu->min_rssi = val16;
	} else if (!strncmp(params->keystr, "pktthrsh", strlen("pktthrsh"))) {
		ulmu->rx_pktcnt_thrsh = val16;
	} else if (!strncmp(params->keystr, "idleprd", strlen("idleprd"))) {
		ulmu->idle_prd = val16;
	} else if (!strncmp(params->keystr, "always_admit", strlen("always_admit"))) {
		if ((uint16)val16 > 3) {
			return BCME_RANGE;
		}
		ulmu->always_admit = val16;
		if (wlc_taf_ul_enabled(wlc->taf_handle)) {
			if (ulmu->always_admit) {
				ulmu->evict_thrsh = -1;
			} else {
				ulmu->evict_thrsh = ULMU_RXMON_EVICT_THRSH;
			}
		}
	} else if (!strncmp(params->keystr, "dl_weight", strlen("dl_weight"))) {
		ulmu->dl_weight = val16;
		if (wlc->clk) {
			wlc_write_shm(wlc, M_TXTRIGWT0_VAL(wlc), val16);
		} else {
			err = BCME_NOCLK;
		}
	} else if (!strncmp(params->keystr, "ul_weight", strlen("ul_weight"))) {
		ulmu->ul_weight = val16;
		if (wlc->clk) {
			wlc_write_shm(wlc, M_TXTRIGWT1_VAL(wlc), val16);
		} else {
			err = BCME_NOCLK;
		}
#ifdef WL_ULRT_DRVR
	} else if (!strncmp(params->keystr, "drv_ulrtctl", strlen("drv_ulrtctl"))) {
		/* 0 = ucode_ratesel
		 * 1 = driver_ratesel
		 * 3 = driver_ratesel + SP
		 */
		if (val16 == 0 || val16 == 1 || val16 == 3)
			ulmu->drv_ulrtctl = (uint8)val16;
		else
			return BCME_RANGE;
#endif
#ifdef WLTAF
	} else if (!strncmp(params->keystr, "evict_thrsh", sizeof("evict_thrsh") - 1)) {
		if (val16 >= 0 && (val16 <= (uint8) -1)) {
			ulmu->evict_thrsh = val16;
		} else {
			return BCME_RANGE;
		}
	} else if (!strncmp(params->keystr, "wait_cnt", sizeof("wait_cnt") - 1)) {
		if (val16 >= 0 && (val16 <= (uint8) -1)) {
			ulmu->trig_wait_start_thrsh = val16;
		} else {
			return BCME_RANGE;
		}
	} else if (!strncmp(params->keystr, "zero_bytes_thrsh", sizeof("zero_bytes_thrsh") - 1)) {
		if (val16 >= 0 && (val16 <= (uint8) -1)) {
			ulmu->zero_bytes_thrsh = val16;
		} else {
			return BCME_RANGE;
		}
	} else if (!strncmp(params->keystr, "trig_wait_ms", sizeof("trig_wait_ms") - 1)) {
		if (val16 >= 0 && (val16 <= (uint8) -1)) {
			ulmu->trig_wait_us = val16 * 1000; /* convert to us */
		} else {
			return BCME_RANGE;
		}
#endif /* WLTAF */
	} else if (!strncmp(params->keystr, "wfa_test", strlen("wfa_test"))) {
		if (val16 != 0) {
			/* For WFA HE ul ofdma cert Reason for it is WFA test requires continuous
			 * trigger mode and fix MCS at 7. This mode bypasses any other smart ul
			 * ofdma schedulers
			 */
			ULMU_FLAGS_USCHED_SET(ulmu->flags, ULMU_ON);
			ULMU_FLAGS_DSCHED_SET(ulmu->flags, ULMU_OFF);
			ULMU_FLAGS_HBRNT_SET(ulmu->flags, ULMU_OFF);
			ulmu->txd.macctl1 = (ULMU_ON << D11_ULOTXD_MACTL1_DSCHED_SHIFT) |
				(ULMU_ON << D11_ULOTXD_MACTL1_HBRNT_SHIFT);
			ulmu->txd.macctl &= ~D11_ULOTXD_MACTL_DURCFG_MASK;
			ulmu->txd.macctl |= (ULMU_MCTL_DURCFG_MAX << D11_ULOTXD_MACTL_DURCFG_SHIFT);
			ulmu->txd.interval = ULMU_TRIGGER_INTVL_WFA;
			ulmu->rx_pktcnt_thrsh = 0;
			ulmu->always_admit = ULMU_ADMIT_ALWAYS;
			ulmu->txd.burst = ULMU_TRIGGER_BURST_WFA;
			ulmu->txd.maxtw = ULMU_TRIGGER_MAXTW_WFA;
			ulmu->is_run2complete = FALSE;
			wlc_ulmu_fburst_set(ulmu, FALSE);
		} else {
			/* resume to regular mode */
#ifdef WLTAF
			wlc_ulmu_sw_trig_enable(wlc, wlc_taf_ul_enabled(wlc->taf_handle));
#else /* WLTAF */
			wlc_ulmu_sw_trig_enable(wlc, 0);
#endif /* WLTAF */
		}
#ifdef WLTAF
	} else if (!strncmp(params->keystr, "rxmon_qlen", sizeof("rxmon_qlen") - 1)) {
			ulmu->rxmon_qlen = val16;
	} else if (!strncmp(params->keystr, "qlen_thrsh_cnt", sizeof("qlen_thrsh_cnt") - 1)) {
			ulmu->qlen_thrsh_cnt = val16;
	} else if (!strncmp(params->keystr, "qlen_incr_wait", sizeof("qlen_incr_wait") - 1)) {
			ulmu->qlen_incr_wait = val16;
	} else if (!strncmp(params->keystr, "qlen_incr_amount", sizeof("qlen_incr_amount") - 1)) {
			ulmu->qlen_incr_amount = val16;
	} else if (!strncmp(params->keystr, "qlen_incr_factor", sizeof("qlen_incr_factor") - 1)) {
			ulmu->qlen_incr_factor = val16;
	} else if (!strncmp(params->keystr, "qlen_incr_over_phy",
			sizeof("qlen_incr_over_phy") - 1)) {
			ulmu->qlen_incr_over_phy = val16;
	} else if (!strncmp(params->keystr, "qlen_incr_over_deci",
			sizeof("qlen_incr_over_deci") - 1)) {
			ulmu->qlen_incr_over_deci = val16;
	} else if (!strncmp(params->keystr, "headroom_aggn",
			sizeof("headroom_aggn") - 1)) {
			ulmu->headroom_aggn = val16;
	} else if (!strncmp(params->keystr, "qlen_incr_over", sizeof("qlen_incr_over") - 1)) {
			ulmu->qlen_incr_over = val16;
	} else if (!strncmp(params->keystr, "max_aggn_release", sizeof("max_aggn_release") - 1)) {
			ulmu->max_aggn_release = MIN(val16, 64);
	} else if (!strncmp(params->keystr, "admit_delay", sizeof("admit_delay") - 1)) {
			ulmu->admit_delay = val16;
	} else if (!strncmp(params->keystr, "qlen_init_time", sizeof("qlen_init_time") - 1)) {
			ulmu->qlen_init_time = val16;
#endif /* WLTAF */
	} else if (!strncmp(params->keystr, "per_bw_admn", sizeof("per_bw_admn") - 1)) {
			ulmu->per_bw_admn = val16;
	} else {
		upd = FALSE;
		err = BCME_BADARG;
	}

	if (upd && wlc->pub->up) {
		wlc_ulmu_cfg_commit(ulmu);
		wlc_ulmu_csreq_commit(ulmu);
		wlc_ulmu_maxn_set(ulmu);
	}

	return err;
} /* wlc_ulmu_cmd_set_dispatch */

static int
wlc_ulmu_doiovar(void *ctx, uint32 actionid,
	void *params, uint plen, void *arg, uint alen, uint vsize, wlc_if_t *wlcif)
{
	int err = BCME_OK;
	int32 *ret_int_ptr;
	wlc_ulmu_info_t *ulmu = ctx;
	wlc_info_t *wlc = ulmu->wlc;
	wl_musched_cmd_params_t musched_cmd_params;
	int32 int_val = 0;
	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;

	BCM_REFERENCE(vsize);
	BCM_REFERENCE(wlc);
	BCM_REFERENCE(ret_int_ptr);

	if (D11REV_LT(wlc->pub->corerev, 129)) return BCME_ERROR;

	bcopy(params, &musched_cmd_params, sizeof(wl_musched_cmd_params_t));
	/* convenience int and bool vals for first 8 bytes of buffer */
	if (plen >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));
	switch (actionid) {
	case IOV_GVAL(IOV_UMUSCHEDULER):
		err = wlc_ulmu_cmd_get_dispatch(ulmu, &musched_cmd_params, arg, alen);
		break;
	case IOV_SVAL(IOV_UMUSCHEDULER):
		err = wlc_ulmu_cmd_set_dispatch(ulmu, params);
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
} /* wlc_ulmu_doiovar */

/* ======== wlc module hooks ========= */

/* wlc init callback */
static int
wlc_ulmu_wlc_init(void *ctx)
{
	wlc_ulmu_info_t *ulmu = ctx;
	int err = BCME_OK;
	wlc_info_t *wlc = ulmu->wlc;

	if (!HE_ENAB(wlc->pub)) {
		return BCME_OK;
	}

	wlc_ulmu_set_ulpolicy(ulmu, ulmu->policy);

	/* start or stop UL scheduler, depending on value of is_start */
	if (HE_ULOMU_ENAB(wlc->pub) && ulmu->is_start) {
		wlc_ulmu_start(ulmu, TRUE);
	} else {
		wlc_ulmu_start(ulmu, FALSE);
	}

	wlc->ulmu->txd.chanspec = wlc->home_chanspec;
	wlc_ulmu_cfg_commit(ulmu);
	wlc_ulmu_csreq_commit(ulmu);
	wlc_ulmu_maxn_set(ulmu);

	wlc_write_shm(wlc, M_TXTRIGWT0_VAL(wlc), ulmu->dl_weight);
	wlc_write_shm(wlc, M_TXTRIGWT1_VAL(wlc), ulmu->ul_weight);

#ifdef WLTAF
	wlc_taf_set_ulofdma_maxn(wlc, &ulmu->maxn);
#endif

	ulmu->qlen_thrsh_cnt = 0;
	ulmu->qlen_incr_wait = 10;
	ulmu->qlen_incr_amount = 3;
	ulmu->qlen_incr_factor = 5;
	ulmu->qlen_incr_over = 28;
	ulmu->qlen_incr_over_phy = 0;
	ulmu->qlen_incr_over_deci = 40;
	ulmu->max_aggn_release = 0;
	ulmu->admit_delay = 500;
	ulmu->qlen_init_time = ULMU_RXMON_BYTECNT_INIT_TIME;

	return err;
} /* wlc_ulmu_wlc_init */

/* wlc deinit callback */
static int
wlc_ulmu_wlc_deinit(void *ctx)
{
	wlc_ulmu_info_t *ulmu = ctx;
	void *p;

	if (ulmu == NULL) {
		return BCME_ERROR;
	}

	while ((p = spktq_deq(&ulmu->utxdq))) {
		PKTFREE(ulmu->wlc->osh, p, TRUE);
	}
	WL_ULO(("%s utxd pending %d\n", __FUNCTION__, TXPKTPENDGET(ulmu->wlc, ULMU_TRIG_FIFO)));

	return BCME_OK;
}

/**
 * ul-ofdma admission control
 *
 * Registered as ulmu module watchdog callback. called once per second.
 * Iterates through each scb and see if there is a ul-ofdma user that needs to be
 * evicted. Does not admit ul-ofdma users.
 *
 * @param ulmu		handle to ulmu_info context
 * @return		none
 */
static void
wlc_ulmu_watchdog(void *mi)
{
	wlc_ulmu_info_t *ulmu = (wlc_ulmu_info_t*) mi;
	wlc_info_t *wlc = ulmu->wlc;
	scb_t *scb;
	scb_iter_t scbiter;
	int admit_cnt = 0, tot_admit_cnt = 0;
	scb_ulmu_t* ulmu_scb;
	uint32 last_rx_pkts;
	ulmu_stats_t *ulstats;
	uint8 bw = 0, scb_bw = 0;
	uint16 num_users = ulmu->num_usrs;

	/* bypass scb admit check/change to prevent sending utxd packet during driver mute/reinit */
	if (wlc->hw->mute_ap || wlc->hw->reinit || wlc->down_override) {
		return;
	}

	do {
		admit_cnt = 0;

		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			if (!SCB_ULOFDMA_ADM(scb)) {
				continue;
			}

			ulmu_scb = SCB_ULMU(ulmu, scb);
			/* Clients could never have been admitted without ulmu_scb */
			ASSERT(ulmu_scb);
			scb_bw = wlc_scb_ratesel_get_ulmu_link_bw(wlc, scb) - 1;
			if (ulmu->per_bw_admn &&
				(bw != scb_bw)) {
				continue;
			}

			last_rx_pkts = ulmu_scb->last_rx_pkts;
			ulmu_scb->last_rx_pkts = scb->scb_stats.rx_ucast_pkts;
			ulmu_scb->rx_data =
				(((uint32)scb->scb_stats.rx_ucast_pkts - (uint32)last_rx_pkts) >=
				ulmu->rx_pktcnt_thrsh);
#ifdef WLTAF
			/* evict if rate, byte cnt are 0 and no utxd in flight */
			if (wlc_taf_ul_enabled(wlc->taf_handle) &&
				!ulmu->always_admit &&
				!ulmu_scb->acc_bufsz && !ulmu_scb->rate &&
				!ulmu_scb->pend_trig_cnt) {
				ulmu_scb->rx_data = FALSE;
			}
#endif
			admit_cnt++;
			tot_admit_cnt++;
		}

		if (admit_cnt < ulmu->min_ulofdma_usrs) { /* then evict all clients */
			/* then evict all clients */
			if (admit_cnt) {
				WL_ULO(("wl%d: %s: evict ALL count %d < %d\n", wlc->pub->unit,
					__FUNCTION__, admit_cnt, ulmu->min_ulofdma_usrs));
				FOREACHSCB(wlc->scbstate, &scbiter, scb) {
					if (!SCB_ULOFDMA_ADM(scb)) {
						continue;
					}
					scb_bw = wlc_scb_ratesel_get_ulmu_link_bw(wlc, scb) - 1;
					if (ulmu->per_bw_admn &&
						(bw != scb_bw)) {
						continue;
					}

					wlc_ulmu_admit_client(ulmu, scb, EVICT_CLIENT);
					ulmu_scb = SCB_ULMU(ulmu, scb);
					if ((ulstats = ulmu_scb->scb_omu_stats) != NULL) {
						ulstats->evict_hist[EVICT_REASON_MINUSERS]++;
					}
				}
				if (ulmu->per_bw_admn) {
					continue;
				}
			}
			if (!ulmu->per_bw_admn) {
				return;
			}
		}
		bw++;
	} while (ulmu->per_bw_admn &&
		(bw < CHSPEC_BW(wlc->home_chanspec) - 1) &&
		(tot_admit_cnt < num_users));

	if ((ulmu->always_admit != ULMU_ADMIT_ALWAYS) && ulmu->num_usrs) {
		/* eviction */
		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			if (!SCB_ULOFDMA_ADM(scb)) {
				continue;
			}
			ulmu_scb = SCB_ULMU(ulmu, scb);
			ASSERT(ulmu_scb);

			ulstats = ulmu_scb->scb_omu_stats;

			ulmu_scb->idle_cnt++;

			if (ulmu_scb->rx_data) {
				ulmu_scb->idle_cnt = 0;
			}

			/* Do not evict trigger-enabled twt user */
			if (wlc_twt_scb_is_trig_enab(wlc->twti, scb)) {
				continue;
			}

			/* for TAF UL, don't wait for ULMU_SCB_IDLEPRD */
			if ((wlc_taf_ul_enabled(wlc->taf_handle) && ulmu_scb->idle_cnt) ||
				(wlc_taf_ul_enabled(wlc->taf_handle) && ulmu_scb->trig_stall) ||
				(ulmu_scb->idle_cnt >= ulmu->idle_prd)) {
#ifdef WLTAF
				WL_TAFF(wlc,
				MACF" trig_wait_start_cnt %d trig_wait_state %d zero_bytes_cnt %d "
					"rxmon_idle_cnt %d idle_cnt %d\n",
					ETHER_TO_MACF(scb->ea), ulmu_scb->trig_wait_start_cnt,
					ulmu_scb->trig_wait_state, ulmu_scb->zero_bytes_cnt,
					ulmu_scb->rxmon_idle_cnt, ulmu_scb->idle_cnt);
#endif
				WL_ULO(("wl%d: %s: evict "MACF" idle_cnt %d >= %d\n",
					wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea),
					ulmu_scb->idle_cnt, ulmu->idle_prd));
				if (!wlc_ulmu_admit_client(ulmu, scb, EVICT_CLIENT)) {
					wlc_ulmu_oper_state_upd(ulmu, scb, ULMU_SCB_INIT);
				}

				/* update eviction histogram */
				if (!ulmu_scb->rx_data) {
					if (ulstats) {
						WLCNTINCR(ulstats->evict_hist[EVICT_REASON_PPS]);
					}
				} else if (ulmu_scb->trig_stall) {
					if (ulstats) {
						ulstats->evict_hist[EVICT_REASON_TRIG_STALL]++;
					}
					ulmu_scb->trig_stall = FALSE;
				}
			}

			/* reset trigger stall check flag  */
			if (wlc_taf_ul_enabled(wlc->taf_handle) &&
				!ulmu->always_admit) {
				 ulmu_scb->trig_stall = TRUE;
			}
		}
	}

	/* Driver UTXD stall check */
	wlc_ulmu_drv_watchdog(ulmu);
} /* wlc_ulmu_watchdog */

#if defined(WL11AX) && defined(WL_PSMX)
void
wlc_ulmu_chanspec_upd(wlc_info_t *wlc)
{
	if (!wlc->ulmu || D11REV_LT(wlc->pub->corerev, 128) || !HE_ULOMU_ENAB(wlc->pub)) {
		return;
	}

	if (!wlc->clk) {
		return;
	}

	if (!PSMX_ENAB(wlc->pub)) {
		return;
	}

	if (wlc->ulmu->txd.chanspec == wlc->home_chanspec) {
		return;
	}

	wlc_bmac_suspend_macx_and_wait(wlc->hw);
	wlc->ulmu->txd.chanspec = wlc->home_chanspec;
	wlc_write_shmx(wlc, MX_TRIG_TXCFG(wlc) +
		OFFSETOF(d11ulo_trig_txcfg_t, chanspec), wlc->home_chanspec);
	wlc_bmac_enable_macx(wlc->hw);
}
#endif /* defined(WL11AX) && defined(WL_PSMX) */

static void
wlc_ulmu_dump_ulofdma(wlc_ulmu_info_t *ulmu, bcmstrbuf_t *b, bool verbose)
{
	int8 schpos;
	scb_t *scb;
	d11ulo_trig_txcfg_t *txd = &ulmu->txd;
	wlc_info_t *wlc = ulmu->wlc;
	scb_ulmu_t* ulmu_scb;
	uint8 mcs, nss;
	uint16 max_ulofdma_usrs = wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA);
	int i;
	char ulmu_policy_str[][20] = {
		"DISABLE",
		"AUTO",
	};
	uint ul_policy_idx = MIN((uint) wlc_ulmu_get_ulpolicy(ulmu),
		ARRAYSIZE(ulmu_policy_str)-1);
	uint offset;
	uint16 val16 = 0;
	BCM_REFERENCE(offset);
	BCM_REFERENCE(val16);

	bcm_bprintf(b, "UL OFDMA admitted %d maxclients %d minulusers %d "
		"fb %d mode %d brcm_only %d ban_nonbrcm_160sta %d\n",
		ulmu->num_usrs, max_ulofdma_usrs, ulmu->min_ulofdma_usrs,
		(txd->mctl0 & D11AC_TXC_MBURST) ? 1 : 0, ulmu->mode,
		ulmu->brcm_only, ulmu->ban_nonbrcm_160sta);
	bcm_bprintf(b, "maxn ");
		for (i = 0; i < D11_REV128_BW_SZ; i++) {
			bcm_bprintf(b, "bw%d: %d ", 20 << i, ulmu->maxn[i]);
		}
	bcm_bprintf(b, "trigger: %s ", (ulmu->is_start && (ulmu->num_usrs >=
			ulmu->min_ulofdma_usrs)) ? "ON" : "OFF");
	bcm_bprintf(b, "drv_ulrtctl: %x\n", ulmu->drv_ulrtctl);

	bcm_bprintf(b, "Scheduler parameters\n"
		"  mctl 0x%04x mctl1 0x%04x txcnt %d burst %d maxtw %d\n"
		"  interval %d maxdur %d minidle %d cpltf %d nltf %d\n"
		"  txlmt 0x%x txlowat0 %d txlowat1 %d rxlowat0 %d rxlowat1 %d\n"
		"  mlen %d mmlen %d aggn %d csthr0 %d csthr1 %d tagid %d\n"
		"  ulc_sz %d autoulc %d usched %d  dsched %d hibernate %d mixbw %d\n"
		"  minrssi %d pktthrsh %d idleprd %d qnullthrsh %d always_admit %d\n",
		txd->macctl, txd->macctl1, txd->txcnt, txd->burst, txd->maxtw,
		txd->interval, txd->maxdur, txd->minidle,
		(txd->txctl & D11_ULOTXD_TXCTL_CPF_MASK),
		((txd->txctl & D11_ULOTXD_TXCTL_NLTF_MASK) >> D11_ULOTXD_TXCTL_NLTF_SHIFT),
		ulmu->txlmt, txd->txlowat0, txd->txlowat1, txd->rxlowat0, txd->rxlowat1,
		txd->mlen, txd->mmlen, txd->aggnum, ulmu->csthr0, ulmu->csthr1, ulmu->g_tagid,
		ulmu->ulc_sz, ULMU_FLAGS_AUTOULC_GET(ulmu->flags),
		ULMU_FLAGS_USCHED_GET(ulmu->flags), ULMU_FLAGS_DSCHED_GET(ulmu->flags),
		ULMU_FLAGS_HBRNT_GET(ulmu->flags),
		(txd->macctl & D11_ULOTXD_MACTL_MIXEDBW_MASK) >> D11_ULOTXD_MACTL_MIXEDBW_SHIFT,
		ulmu->min_rssi, ulmu->rx_pktcnt_thrsh, ulmu->idle_prd, ulmu->qnull_thrsh,
		ulmu->always_admit);

#if defined(BCMDBG)
	/* This block of info is for internal debugging purpose */
#if defined(WL_PSMX)
	if (wlc->clk) {
		offset = MX_TRIG_TXCFG(wlc) + OFFSETOF(d11ulo_trig_txcfg_t, tgintvl);
		val16 = wlc_read_shmx(wlc, offset);
	}
#endif /* defined(WL_PSMX) */
	bcm_bprintf(b, "  tgintvl %d\n", val16);
#endif /* BCMDBG */

	bcm_bprintf(b, "UL policy: %s (%d) ",
		ulmu_policy_str[ul_policy_idx], wlc_ulmu_get_ulpolicy(ulmu));
	for (i = 0; i < D11_REV128_BW_SZ; i++) {
		bcm_bprintf(b, "bw%d: %d ", 20 << i, ulmu->maxn[i]);
	}
	bcm_bprintf(b, "\n");
	bcm_bprintf(b, "num_usrs: %d num_pendutxd: %d num_utxd_alloc: %d ",
		ulmu->num_usrs, TXPKTPENDGET(wlc, ULMU_TRIG_FIFO), ulmu->utxd_alloc_cnt);
	if (ulmu->num_usrs) {
		bcm_bprintf(b, "List of admitted clients:");
	}
	bcm_bprintf(b, "\n");
	for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
		if ((scb = ulmu->scb_list[schpos]) == NULL) {
			continue;
		}
		ulmu_scb = SCB_ULMU(ulmu, scb);
		bcm_bprintf(b, "%s:", ULMU_SCB_IS_ULMMU(ulmu_scb) ? "MMU":"OMU");
		bcm_bprintf(b, " "MACF" (aid 0x%x) idx %d rmem %d lmem %d tagid %d ulmu_allow %d",
			ETHER_TO_MACF(scb->ea), scb->aid,
			schpos,
			wlc_ulmu_scb_get_rmemidx(wlc, scb),
			ulmu_scb->lmemidx,
			ulmu_scb->tagid,
			wlc_he_get_ulmu_allow(wlc->hei, scb));
		if (ulmu_scb->ucfg & D11_ULOTXD_UCFG_FIXRT_MASK) {
			mcs = D11_ULOTXD_UCFG_GET_MCS(ulmu_scb->ucfg);
			nss = D11_ULOTXD_UCFG_GET_NSS(ulmu_scb->ucfg) + 1;
			bcm_bprintf(b, " fixed rate %dx%d\n", mcs, nss);
		} else {
			bcm_bprintf(b, "\n");
		}
	}
}

#if defined(BCMDBG_DUMP_UMSCHED)
static void
wlc_ulmu_dump_ul_stats(wlc_ulmu_info_t *ulmu, bcmstrbuf_t *b, uint8 dump_type)
{
	scb_iter_t scbiter;
	scb_t *scb;
	scb_ulmu_t* ulmu_scb;
	uint scb_bw;
	wlc_info_t *wlc = ulmu->wlc;
	ulmu_stats_t* scb_stats;

	/* 1. dump MU histogram and global PER */
	/* 2. dump UL RU stats */
	if (dump_type & ULMU_DUMP_OMU) {
		bcm_bprintf(b, "UL OFDMA stats:");
		wlc_ulmu_dump_gstats(ulmu->omu_gstats, b, "ULOMU");
		wlc_ulmu_ru_gstats(ulmu->omu_gstats, b,
			CHSPEC_IS160(ulmu->wlc->chanspec));

		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			if (!scb || !SCB_ASSOCIATED(scb) || !SCB_HE_CAP(scb)) {
				continue;
			}
			ulmu_scb = SCB_ULMU(ulmu, scb);
			scb_stats = ulmu_scb ? ulmu_scb->scb_omu_stats : NULL;
			if (ulmu_scb && scb_stats) {
				scb_bw = wlc_scb_ratesel_get_link_bw(wlc, scb);
				wlc_ulmu_ru_stats(scb_stats, scb, b,
					(scb_bw == BW_160MHZ));
			}
		}
	}
	if ((dump_type & ULMU_DUMP_MMU) && ulmu->mmu_gstats) {
		bcm_bprintf(b, "UL MUMIMO stats:");
		wlc_ulmu_dump_gstats(ulmu->mmu_gstats, b, "ULMMU");
		wlc_ulmu_ru_gstats(ulmu->mmu_gstats, b,
			CHSPEC_IS160(ulmu->wlc->chanspec));

		FOREACHSCB(wlc->scbstate, &scbiter, scb) {
			if (!scb || !SCB_ASSOCIATED(scb) || !SCB_HE_CAP(scb)) {
				continue;
			}
			ulmu_scb = SCB_ULMU(ulmu, scb);
			scb_stats = ulmu_scb ? ulmu_scb->scb_mmu_stats : NULL;
			if (ulmu_scb && scb_stats) {
				scb_bw = wlc_scb_ratesel_get_link_bw(wlc, scb);
				wlc_ulmu_ru_stats(scb_stats, scb, b,
					(scb_bw == BW_160MHZ));
			}
		}
	}

	wlc_ulmu_dump_utxd_in_transit_stats(ulmu, b);

#ifdef WLTAF
	wlc_ulmu_dump_bulk_commit_stats(ulmu, b);
#endif
}

int
wlc_ulmu_rustats_upd(wlc_ulmu_info_t *ulmu, scb_t *scb, tx_status_t *txs)
{
	scb_ulmu_t* ulmu_scb = SCB_ULMU(ulmu, scb);
	wlc_info_t *wlc;
	uint8 ruidx, upper, tx_cnt, txsucc_cnt;
	uint8 rualloc_b12, primary80, ulmm;
	ru_type_t ru_type;
	ulmu_gstats_t *gstats;
	ulmu_stats_t* scb_stats;

	if (ulmu_scb == NULL) {
		return BCME_ERROR;
	}

	wlc = ulmu->wlc;
	ruidx = TGTXS_RUIDX(TX_STATUS_MACTXS_S4(txs));
	rualloc_b12 = TGTXS_RUALLOCLSB(TX_STATUS_MACTXS_S4(txs));
	primary80 = (CHSPEC_SB(wlc->chanspec) > 4)? 1: 0;
	upper = primary80 ^ rualloc_b12;
	ulmm = TGTXS_ULMM(TX_STATUS_MACTXS_S5(txs));
	tx_cnt = TGTXS_LCNT(TX_STATUS_MACTXS_S5(txs));
	txsucc_cnt = TGTXS_GDFCSCNT(TX_STATUS_MACTXS_S5(txs));
	gstats = ulmm ? ulmu->mmu_gstats : ulmu->omu_gstats;
	scb_stats = ulmm ? ulmu_scb->scb_mmu_stats : ulmu_scb->scb_omu_stats;

	if (ruidx > HE_MAX_2x996_TONE_RU_INDX) {
		WL_ERROR(("wl%d: %s: Invalid ru type. ru idx %d txs\n"
			"  %08X %08X | %08X %08X | %08X %08X || %08X %08X\n",
			ulmu->wlc->pub->unit, __FUNCTION__, ruidx,
			TX_STATUS_MACTXS_S1(txs), TX_STATUS_MACTXS_S2(txs),
			TX_STATUS_MACTXS_S3(txs), TX_STATUS_MACTXS_S4(txs),
			TX_STATUS_MACTXS_S5(txs), TX_STATUS_MACTXS_ACK_MAP1(txs),
			TX_STATUS_MACTXS_ACK_MAP2(txs), TX_STATUS_MACTXS_S8(txs)));
		ASSERT(0);
		return BCME_ERROR;
	} else {
		ru_type = wf_he_ruidx_to_ru_type(ruidx);
	}

	if (gstats == NULL) {
		return BCME_ERROR;
	}

	gstats->ru_idx_use_bmap[upper][ruidx / 8] |= 1 << (ruidx % 8);
	if (tx_cnt > 0) {
		WLCNTADD(gstats->tx_cnt[ru_type], tx_cnt);
		WLCNTADD(gstats->txsucc_cnt[ru_type], txsucc_cnt);
	}

	if (ulmu_scb && scb_stats) {
		scb_stats->ru_idx_use_bmap[upper][ruidx / 8] |= 1 << (ruidx % 8);
		if (tx_cnt > 0) {
			WLCNTADD(scb_stats->tx_cnt[ru_type], tx_cnt);
			WLCNTADD(scb_stats->txsucc_cnt[ru_type], txsucc_cnt);
		}
	}

	return BCME_OK;
} /* wlc_ulmu_rustats_upd */

static void
wlc_ulmu_ru_gstats(ulmu_gstats_t* gstats, bcmstrbuf_t *b, bool is_160)
{
	int i, k, cnt;
	uint32 total = 0;

	for (i = 0; i < MUSCHED_RU_TYPE_NUM; i++) {
		total += gstats->tx_cnt[i];
	}

	if (!total) {
		return;
	}
	bcm_bprintf(b, "UL RU stats: ");
	cnt = 0;
	for (i = 0; i < MUSCHED_RU_BMP_ROW_SZ; i++) {
		if (!is_160 && i > 0) {
			continue;
		}
		for (k = 0; k <= MUSCHED_RU_IDX_NUM; k++) {
			if ((cnt > 0) && (cnt % 16 == 0)) {
				bcm_bprintf(b, "\n");
			}
			if (getbits(gstats->ru_idx_use_bmap[i], MUSCHED_RU_BMP_COL_SZ,
				k, 1)) {
				bcm_bprintf(b, "%d%s ", k, i == 0 ? "" : "s");
				cnt++;
			}
		}
	}
	bcm_bprintf(b, "\n");

	wlc_print_dump_table(b, "    RX", gstats->tx_cnt, "   PER", gstats->txsucc_cnt,
		MUSCHED_RU_TYPE_NUM, MUSCHED_RU_TYPE_NUM, TABLE_RU);
} /* wlc_ulmu_ru_gstats */

static void
wlc_ulmu_dump_gstats(ulmu_gstats_t* gstats, bcmstrbuf_t *b, char *dump_type)
{
	int i, last, num_perline;
	uint32 total = 0;

	for (i = 0, total = 0, last = 0; i < ULMU_TRIG_USRCNT_MAX; i++) {
		total += gstats->usrhist[i];
		if (gstats->usrhist[i]) last = i;
	}
	if (total) {
		num_perline = ULMU_TRIG_USRCNT_MAX < MAX_USRHIST_PERLINE?
			ULMU_TRIG_USRCNT_MAX: MAX_USRHIST_PERLINE;
		last = num_perline * (last/num_perline + 1) - 1;
		bcm_bprintf(b, "  (TOT: %d)\n", total);
		bcm_bprintf(b, "  HIST %s Trigger:    ", dump_type);
		for (i = 0; i <= last; i++) {
			bcm_bprintf_val_pcent(b, gstats->usrhist[i],
				(gstats->usrhist[i] * 100) / total, DEFAULT_PADDING);
			if ((i % num_perline) == (num_perline - 1) && i != last)
				bcm_bprintf(b, "\n       : ");
		}
		bcm_bprintf(b, "\n");
		bcm_bprintf(b, "  HIST %s Rx AMPDU:  ", dump_type);
		for (i = 0; i <= last; i++) {
			bcm_bprintf_val_pcent(b, gstats->usrDataFrmhist[i], gstats->usrhist[i] > 0 ?
				(gstats->usrDataFrmhist[i] * 100) / (gstats->usrhist[i]*(i+1)) : 0,
				DEFAULT_PADDING);
			if ((i % num_perline) == (num_perline - 1) && i != last)
				bcm_bprintf(b, "\n       : ");
		}
	}
	bcm_bprintf(b, "\n");

	wlc_print_dump_table(b, " RX HE", gstats->lcnt,
		" HE PER", gstats->gdfcscnt, AMPDU_MAX_HE, MAX_HE_RATES, TABLE_MCS);
} /* wlc_ulmu_dump_gstats */

static void
wlc_ulmu_print_stats(scb_ulmu_t* ulmu_scb, scb_t *scb, bcmstrbuf_t *b)
{
	ulmu_stats_t *ul_stats = ULMU_SCB_STATS(ulmu_scb);
	int i, last;
	uint32 total = 0, tmp_aggnum, tmp_agglen, tmp_txop;
	uint32 ngoodfcs, nposlcnt;
	int32 tmp;
	uint32 avg_admit_dur;

	if (!ul_stats) {
		return;
	}

	for (i = 0, total = 0, last = 0; i < AMPDU_MAX_HE; i++) {
		total += ul_stats->lcnt[i];
		if (ul_stats->lcnt[i]) last = i;
	}

	if (!total) {
		return;
	}

	bcm_bprintf(b, ""MACF" (aid %d):\n",
		ETHER_TO_MACF(scb->ea), scb->aid);
	last = MAX_HE_RATES * (last/MAX_HE_RATES + 1) - 1;

	wlc_print_dump_table(b, " RX HE", ul_stats->lcnt,
		" HE PER", ul_stats->gdfcscnt, AMPDU_MAX_HE, MAX_HE_RATES, TABLE_MCS);

	if (ul_stats->rssi_stats.min_rssi < 0) {
		tmp = (-ul_stats->rssi_stats.min_rssi) >> OPERAND_SHIFT;
		bcm_bprintf(b, "  RSSI : min(-%d.%d) ", tmp >> 2, (tmp & 0x3) * 25);
	} else {
		tmp = ul_stats->rssi_stats.min_rssi >> OPERAND_SHIFT;
		bcm_bprintf(b, "  RSSI : min(%d.%d) ", tmp >> 2, (tmp & 0x3) * 25);
	}
	if (ul_stats->rssi_stats.max_rssi < 0) {
		tmp = (-ul_stats->rssi_stats.max_rssi) >> OPERAND_SHIFT;
		bcm_bprintf(b, "max(-%d.%d) ", tmp >> 2, (tmp & 0x3) * 25);
	} else {
		tmp = ul_stats->rssi_stats.max_rssi >> OPERAND_SHIFT;
		bcm_bprintf(b, "max(%d.%d) ", tmp >> 2, (tmp & 0x3) * 25);
	}
	ngoodfcs = ul_stats->nupd - ul_stats->nbadfcs;
	nposlcnt = ul_stats->nupd - ul_stats->nfail;
	if (ul_stats->nvldrssi > 0) {
		tmp = ul_stats->rssi_stats.avg_rssi;
		if (tmp < 0) {
			tmp = (-tmp / (int32) ul_stats->nvldrssi) >> OPERAND_SHIFT;
			bcm_bprintf(b, "avg(-%d.%d)\n", tmp >> 2, (tmp & 0x3) * 25);
		} else {
			tmp = (tmp / (int32) ul_stats->nvldrssi) >> OPERAND_SHIFT;
			bcm_bprintf(b, "avg(%d.%d)\n", tmp >> 2, (tmp & 0x3) * 25);
		}
	} else {
		bcm_bprintf(b, "avg(%d.%d)\n", 0, 0);
	}
	bcm_bprintf(b, "  total: qosnull %d ", ul_stats->qncnt);
	bcm_bprintf(b, "lcnt %d ", ul_stats->sum_lcnt);
	bcm_bprintf(b, "nfail %d ", ul_stats->nfail);
	bcm_bprintf(b, "nupd %d ", ul_stats->nupd);
	bcm_bprintf(b, "nadmit %d", ul_stats->nadmit);
	wlc_ulmu_print_eviction_hist(ulmu_scb, b);

	if (nposlcnt) {
		tmp_aggnum = ul_stats->sum_lcnt / nposlcnt;
	} else {
		tmp_aggnum = 0;
	}
	if (ngoodfcs) {
		tmp_agglen = ul_stats->agglen / ngoodfcs;
		tmp_txop = ((ul_stats->txop / ngoodfcs) / 3) >> OPERAND_SHIFT;
	} else {
		tmp_agglen = 0;
		tmp_txop = 0;
	}
	avg_admit_dur = ul_stats->nadmit ? ul_stats->admit_dur / ul_stats->nadmit : 0;
	bcm_bprintf(b, "  avg  : aggnum %d agglen %d txdur %d admit_dur %d ms\n",
		tmp_aggnum, tmp_agglen, tmp_txop, avg_admit_dur);

	wlc_ulmu_drv_print_scb_stats(ulmu_scb, scb, b);

	bcm_bprintf(b, "  avg rxmpduperampdu %d rxmpdulen %d avg_qsz %d cur_qsz %d\n",
		ulmu_scb->aggnma >> ULMU_NF_AGGN,
		ulmu_scb->mlenma >> ULMU_NF_AGGLEN,
		ul_stats->avg_qsz >> ULMU_QS_UPSCALE_FACTOR, ul_stats->cur_qsz);
	bcm_bprintf(b, "  tgs bytes recvd %d exp %d\n",
		ulmu_scb->tgs_recv_bytes,
		ulmu_scb->tgs_exprecv_bytes);

#ifdef WLTAF
	wlc_ulmu_taf_print_scb_stats(ulmu_scb->ulmu, scb, b);
#endif
} /* wlc_ulmu_print_stats */

static void
wlc_ulmu_ru_stats(ulmu_stats_t* ul_stats, scb_t *scb,
	bcmstrbuf_t *b, bool is_160)
{
	int i, k, cnt;
	uint32 total = 0;

	for (i = 0; i < MUSCHED_RU_TYPE_NUM; i++) {
		total += ul_stats->tx_cnt[i];
	}

	if (!total) {
		return;
	}

	bcm_bprintf(b, ""MACF" (aid %d): RU ",
		ETHER_TO_MACF(scb->ea), scb->aid);
	cnt = 0;
	for (i = 0; i < MUSCHED_RU_BMP_ROW_SZ; i++) {
		if (!is_160 && i > 0) {
			continue;
		}
		for (k = 0; k <= MUSCHED_RU_IDX_NUM; k++) {
			if ((cnt > 0) && (cnt % 16 == 0)) {
				bcm_bprintf(b, "\n");
			}
			if (getbits(ul_stats->ru_idx_use_bmap[i], MUSCHED_RU_BMP_COL_SZ,
				k, 1)) {
				bcm_bprintf(b, "%d%s ", k, i == 0 ? "" : "s");
				cnt++;
			}
		}
	}
	bcm_bprintf(b, "\n");
	wlc_print_dump_table(b, "    RX", ul_stats->tx_cnt, "   PER", ul_stats->txsucc_cnt,
		MUSCHED_RU_TYPE_NUM, MUSCHED_RU_TYPE_NUM, TABLE_RU);
	wlc_print_dump_table(b, "    RX", ul_stats->tx_nsscnt,
		"    PER", ul_stats->txsucc_nsscnt, 4, 4, TABLE_NSS);
} /* wlc_ulmu_ru_stats */

/* debug dump for ul ofdma scheduler */
static int
wlc_ulmu_dump(void *ctx, bcmstrbuf_t *b)
{
	wlc_ulmu_info_t *ulmu = ctx;
	wlc_info_t *wlc = ulmu->wlc;
	scb_iter_t scbiter;
	scb_t *scb;
	scb_ulmu_t* ulmu_scb;
	uint8 dump_type = ULMU_DUMP_BOTH;
	char *args = ulmu->wlc->dump_args;

	if (args) {
		char* p = bcmstrtok(&args, " ", 0);

		if (p && (p[0] == '-') && (p[1] == 'u')) {
			struct ether_addr ea;

			p = bcmstrtok(&args, " ", 0);
			bcm_ether_atoe(p, &ea);
			scb = wlc_scbapfind(wlc, &ea);

			if (!scb || !SCB_ASSOCIATED(scb) || !SCB_HE_CAP(scb)) {
				return BCME_OK;
			}
			ulmu_scb = SCB_ULMU(ulmu, scb);
			if (ulmu_scb) {
				wlc_ulmu_print_stats(ulmu_scb, scb, b);
			}
			return BCME_OK;
		} else if (p && (p[0] == '-') && (p[1] == 'a')) {

			/* dump per user PER info */
			FOREACHSCB(wlc->scbstate, &scbiter, scb) {
				if (!scb || !SCB_ASSOCIATED(scb) || !SCB_HE_CAP(scb)) {
					continue;
				}
				ulmu_scb = SCB_ULMU(ulmu, scb);
				if (ulmu_scb) {
					wlc_ulmu_print_stats(ulmu_scb, scb, b);
				}
			}
			return BCME_OK;
		} else if (p && (p[0] == '-') && (p[1] == 'm')) {
				dump_type = ULMU_DUMP_MMU;
		} else if (p && (p[0] == '-') && (p[1] == 'o')) {
				dump_type = ULMU_DUMP_OMU;
		} else if (p && (p[0] == '-') && (p[1] == 'h')) {

			bcm_bprintf(b, "dump umsched optional params (space separated): ");
			bcm_bprintf(b, "[-h -u -m -o -a]\n");
			bcm_bprintf(b, "-h\t(this) help output\n");
			bcm_bprintf(b, "-u <addr>\tStats dump for <addr>\n");
			bcm_bprintf(b, "-m\tStats dump for OMU only. Default is both.\n");
			bcm_bprintf(b, "-o\tStats dump for MMU only. Default is both.\n");
			bcm_bprintf(b, "-a\tStats dump for all admitted STAs excluding the "
				"common info\n");
			return BCME_OK;
		}
	}

	if (HE_ULOMU_ENAB(ulmu->wlc->pub)) {
		wlc_ulmu_dump_ulofdma(ulmu, b, TRUE);
		wlc_ulmu_dump_ul_stats(ulmu, b, dump_type);
	}

	return BCME_OK;
} /* wlc_ulmu_dump */

static int
wlc_ulmu_dump_clr(void *ctx)
{
	scb_iter_t scbiter;
	scb_t *scb;
	scb_ulmu_t* ulmu_scb;
	wlc_info_t *wlc;

	wlc_ulmu_info_t *ulmu = ctx;
	BCM_REFERENCE(ulmu);
	wlc = ulmu->wlc;

	/* clear global ulmu_gstats */
	memset(ulmu->omu_gstats, 0, sizeof(ulmu_gstats_t));
	if (ulmu->mmu_gstats) {
		memset(ulmu->mmu_gstats, 0, sizeof(ulmu_gstats_t));
	}

	/* clear ulmu_stats for scb */
	FOREACHSCB(wlc->scbstate, &scbiter, scb) {
		if (!scb || !SCB_ASSOCIATED(scb) || !SCB_HE_CAP(scb)) {
			continue;
		}
		ulmu_scb = SCB_ULMU(ulmu, scb);
		if (ulmu_scb && ulmu_scb->scb_omu_stats) {
			memset(ulmu_scb->scb_omu_stats, 0, sizeof(ulmu_stats_t));
			ulmu_scb->scb_omu_stats->rssi_stats.min_rssi =
				PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
			ulmu_scb->scb_omu_stats->rssi_stats.max_rssi =
				-PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
#ifdef WLTAF
			wlc_ulmu_taf_stats_reset(ulmu, scb);
#endif
		}
		if (ulmu_scb && ulmu_scb->scb_mmu_stats) {
			memset(ulmu_scb->scb_mmu_stats, 0, sizeof(ulmu_stats_t));
			ulmu_scb->scb_mmu_stats->rssi_stats.min_rssi =
				PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
			ulmu_scb->scb_mmu_stats->rssi_stats.max_rssi =
				-PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
#ifdef WLTAF
			wlc_ulmu_taf_stats_reset(ulmu, scb);
#endif
		}
#ifdef WL_ULRT_DRVR
		wlc_scb_ratesel_ulrt_dump_clr(wlc->wrsi, scb);
#endif
	}

	return BCME_OK;
} /* wlc_ulmu_dump_clr */

#endif /* BCMDBG_DUMP_UMSCHED */

#ifdef ULMU_DRV
#define ULSTS_STOP_TIMEOUT		0x001
#define ULSTS_STOP_QOSNULL		0x002
#define ULSTS_STOP_BYTECOUNT		0x004
#define ULSTS_STOP_TRIGCNT		0x008
#define ULSTS_STOP_SUPP			0x010

/* Driver ULMU UTXD STOP reason macros */
#define ULMU_DRV_STOP_BYTES(s)		((s) & ULSTS_STOP_BYTECOUNT)
#define ULMU_DRV_STOP_QOSNULL(s)	((s) & ULSTS_STOP_QOSNULL)
#define ULMU_DRV_STOP_TIMEOUT(s)	((s) & ULSTS_STOP_TIMEOUT)
#define ULMU_DRV_STOP_SUPPRESS(s)	((s) & ULSTS_STOP_SUPP)
#define ULMU_DRV_STOP_TRIGCNT(s)	((s) & ULSTS_STOP_TRIGCNT)
#define ULMU_DRV_STOP(s)		((s) >= ULMU_STATUS_COMPLETE)

#define DRV_SCB_COUNTERS(ulmu_scb) (ULMU_SCB_IS_ULMMU(ulmu_scb) ? \
		(&(ulmu_scb)->scb_mmu_stats->drv_counters) : \
		(&(ulmu_scb)->scb_omu_stats->drv_counters))

/* Driver uplink module attach init */
static void
wlc_ulmu_drv_ulmu_attach(wlc_ulmu_info_t *ulmu)
{
	ASSERT(ulmu);

	ulmu->drv_gconfig.drv_requested_bytes = ULMU_DRV_DEFAULT_BUFSZ;
	ulmu->drv_gconfig.drv_watchdog = 0;

#ifdef BCMDBG
	ulmu->drv_gconfig.drv_autotrigger = 0;
#endif
}

/* Driver uplink module attach de-init */
static void
wlc_ulmu_drv_ulmu_detach(wlc_ulmu_info_t *ulmu)
{
	ASSERT(ulmu);

	ulmu->drv_gconfig.drv_requested_bytes = 0;
	ulmu->drv_gconfig.drv_watchdog = 0;

#ifdef BCMDBG
	ulmu->drv_gconfig.drv_autotrigger = 0;
#endif
}

/**
 * Init/deinit context tracking data structures related to one remote node, and add it to the list
 * of remote nodes.
 */
static int
wlc_ulmu_drv_req_list_init(wlc_info_t *wlc, scb_t *scb, ulmu_drv_scbinfo_t *ulinfo)
{
	trigger_ctx_list_t *list;
	trigger_ctx_entry_t *cur = NULL;
	int n;

	ASSERT(ulinfo);

	list = &ulinfo->context_list;
	list->headidx = 0;
	list->tailidx = 0;
	list->next_sequence_number = 1;

	/* Connect up the next pointers into a circular linked list */
	for (n = 0, cur = &list->list[0]; n < ULMU_DRV_REQUEST_LISTSZ; n++, cur++) {
		cur->slot_number = n;
	}

	WL_ULO(("%s: SCB:"MACF" List initialized (len = %d, start seq = %d)\n",
		__FUNCTION__, ETHER_TO_MACF(scb->ea),
		ULMU_DRV_REQUEST_LISTSZ, list->next_sequence_number));

	return BCME_OK;
} /* wlc_ulmu_drv_req_list_init */

/* Save trigger request for accumulation */
static trigger_ctx_entry_t *
wlc_ulmu_drv_save_trigger_context(scb_t *scb, ulmu_drv_scbinfo_t *ulinfo,
	packet_trigger_info_t *in_data)
{
	trigger_ctx_list_t *list;
	trigger_ctx_entry_t *entry;

	STATIC_ASSERT(sizeof(list->list[0].info) == sizeof(*in_data));
	ASSERT(ulinfo);

	list = &ulinfo->context_list;

	if (list->len > 0) {
		uint8 newhead = MODINC(list->headidx, ULMU_DRV_REQUEST_LISTSZ);

		if (newhead == list->tailidx) {
			ASSERT(list->len == ULMU_DRV_REQUEST_LISTSZ);
			return NULL;
		}
		list->headidx = newhead;
	} else {
		ASSERT(list->headidx == list->tailidx);
	}
	entry = &list->list[list->headidx];
	memcpy(&entry->info, in_data, sizeof(*in_data));

	entry->sequence_number = list->next_sequence_number;
	list->next_sequence_number++;
	list->len++;

	WL_ULO(("%s: SCB:"MACF" nlen=%d seq=%d slot=%d\n",
		__FUNCTION__, ETHER_TO_MACF(scb->ea),
		list->len, entry->sequence_number, entry->slot_number));

	return entry;
} /* wlc_ulmu_drv_save_trigger_context */

/* Returns next saved trigger context */
static trigger_ctx_entry_t *
wlc_ulmu_drv_get_next_trigger(scb_t *scb, ulmu_drv_scbinfo_t *ulinfo)
{
	trigger_ctx_list_t *list;
	trigger_ctx_entry_t *entry;

	ASSERT(ulinfo);

	list = &ulinfo->context_list;

	if (list->len == 0) {
		ASSERT(list->headidx == list->tailidx);
		return NULL;
	}

	entry = &list->list[list->tailidx];

	WL_ULO(("%s: SCB:"MACF" len=%d seq=%d slot=%d\n",
		__FUNCTION__, ETHER_TO_MACF(scb->ea),
		list->len, entry->sequence_number, entry->slot_number));

	return entry;
}

/* Returns next saved trigger context */
static bool
wlc_ulmu_drv_check_trigger(ulmu_drv_scbinfo_t *ulinfo, uint32 fid)
{
	trigger_ctx_list_t *list;
	trigger_ctx_entry_t *entry;
	int i, idx;

	ASSERT(ulinfo);

	list = &ulinfo->context_list;

	if (list->len == 0) {
		ASSERT(list->headidx == list->tailidx);
		WL_ERROR(("%s: fid 0x%x not found, list empty\n", __FUNCTION__, fid));
		return FALSE;
	}

	for (i = 0, idx = list->tailidx; ; i++, idx = MODINC(idx, ULMU_DRV_REQUEST_LISTSZ)) {
		entry = &list->list[idx];
		if (entry->frameid == fid) {
			return TRUE;
		}
		if (idx == list->headidx)
			break;
		ASSERT(i < list->len);
	}

	WL_ERROR(("%s: fid 0x%x not found, list len = %d\n", __FUNCTION__, fid, list->len));
	for (i = 0, idx = list->tailidx; i < list->len;	i++,
		idx = MODINC(idx, ULMU_DRV_REQUEST_LISTSZ)) {
		WL_ULO(("[i%d:0x%x]", idx, list->list[idx].frameid));
	}
	WL_ULO(("\n"));

	return FALSE;
}

/* Free and release current pending trigger */
static void
wlc_ulmu_drv_free_trigger(scb_t *scb, ulmu_drv_scbinfo_t *ulinfo)
{
	trigger_ctx_list_t *list;
	trigger_ctx_entry_t *entry;

	ASSERT(ulinfo);

	list = &ulinfo->context_list;

	if (list->len == 0) {
		ASSERT(list->headidx == list->tailidx);
		return;
	}

	entry = &list->list[list->tailidx];
	entry->state = ULMU_REQ_INACTIVE;
	entry->frameid = 0;

	/* Clear UTXD accmulator info */
	memset(&entry->accmulator, 0, sizeof(entry->accmulator));
	memset(&entry->info, 0, sizeof(entry->info));

	list->len--;
	if (list->len > 0) {
		list->tailidx = MODINC(list->tailidx, ULMU_DRV_REQUEST_LISTSZ);
	} else {
		ASSERT(list->headidx == list->tailidx);
	}

	WL_ULO(("%s: SCB:"MACF" seq=%d nlen=%dd slot=%d->%d\n",
		__FUNCTION__, ETHER_TO_MACF(scb->ea),
		entry->sequence_number,	list->len,
		entry->slot_number, list->list[list->tailidx].slot_number));
}

/* De-initialize scb uplink context */
static void
wlc_ulmu_drv_scb_deinit(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	ulmu_drv_scbinfo_t *ulinfo;
	scb_ulmu_t *ulmu_scb;

	ASSERT(ulmu);
	ASSERT(scb);

	ulmu_scb = SCB_ULMU(ulmu, scb);

	if (!ulmu_scb) {
		return;
	}

	ulinfo = ulmu_scb->ulinfo;

	if (ulinfo == NULL) {
		return;
	}

	wlc_ulmu_drv_del_usr(ulmu->wlc, scb, ulmu_scb);

	MFREE(ulmu->wlc->osh, ulinfo, sizeof(*ulinfo));
	WL_ULO(("%s: SCB:"MACF" \n", __FUNCTION__, ETHER_TO_MACF(scb->ea)));
}

/** Initialize scb uplink context */
static int
wlc_ulmu_drv_scb_init(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	ulmu_drv_scbinfo_t *ulinfo; /* related to one remote node */
	scb_ulmu_t *ulmu_scb;
	int ret = BCME_OK;

	ASSERT(!SCB_INTERNAL(scb));
	ASSERT(ulmu);
	ASSERT(scb);

	ulmu_scb = SCB_ULMU(ulmu, scb);
	ASSERT(ulmu_scb);

	if (ulmu_scb->ulinfo == NULL) {
		ulinfo = MALLOCZ(ulmu->wlc->osh, sizeof(*ulinfo));
		if (ulinfo == NULL) {
			WL_ULO(("%s: SCB:"MACF" Failed to allocate %d bytes for ulinfo\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), (int)sizeof(*ulinfo)));
			return BCME_NOMEM;
		}

		ret = wlc_ulmu_drv_req_list_init(ulmu->wlc, scb, ulinfo);

		if (ret != BCME_OK) {
			MFREE(ulmu->wlc->osh, ulinfo, sizeof(*ulinfo));
			ulmu_scb->ulinfo = NULL;
		} else {
			ulmu_scb->ulinfo = ulinfo;
		}
	}

	WL_ULO(("%s: SCB:"MACF" \n", __FUNCTION__, ETHER_TO_MACF(scb->ea)));

	return ret;
} /* wlc_ulmu_drv_scb_init */

/* Get globally configured bytecount */
static uint32
wlc_ulmu_drv_get_bytecount(wlc_ulmu_info_t *ulmu)
{
	ASSERT(ulmu);

	return ulmu->drv_gconfig.drv_requested_bytes;
}

/*
 * Set global target bytecount for the UTXD request
 * This is a stub to integrate with future RXMONITOR queue depth functionality
 */
static void
wlc_ulmu_drv_set_bytecount(wlc_ulmu_info_t *ulmu, uint32 bytecount)
{
	ASSERT(ulmu);
	ulmu->drv_gconfig.drv_requested_bytes = bytecount;
}

/* Get watchdog timeout in seconds */
static uint32
wlc_ulmu_drv_get_watchdog(wlc_ulmu_info_t *ulmu)
{
	ASSERT(ulmu);

	return ulmu->drv_gconfig.drv_watchdog;
}

/* Set watchdog timeout in seconds */
static void
wlc_ulmu_drv_set_watchdog(wlc_ulmu_info_t *ulmu, uint32 timeout)
{
	ASSERT(ulmu);
	ulmu->drv_gconfig.drv_watchdog = timeout;
}

#ifndef BYTE_INTERFACE
/* Calculate next us count target used for threshold reporting */
static uint32
wlc_ulmu_drv_next_uscount(uint32 maxcount, uint32 curr_count, uint32 delta)
#else
/* Calculate next byte count target used for threshold reporting */
static uint32
wlc_ulmu_drv_next_bytecount(uint32 maxcount, uint32 curr_count, uint32 delta)
#endif
{
	uint32 newcount;

	/*
	 * (curr_count/delta) is the integral number
	 * of delta slots, add 1 to get to the next
	 * slot value that is (n+1) slots fo delta_bytes
	 */
	newcount = delta ? (1 + (curr_count/delta)) * delta : 1;
	newcount = (newcount < maxcount) ? newcount : maxcount;
	WL_ULO(("%s: max:%d cur:%d new:%d delta:%d\n",
		__FUNCTION__, maxcount, curr_count, newcount, delta));

	return (newcount);
}

/* Apply default config info if caller did not specified them */
static void
wlc_ulmu_drv_request_default_fixups(packet_trigger_info_t *ti)
{
	if (ti->qos_null_threshold == 0) {
		ti->qos_null_threshold = ULMU_QOSNULL_LIMIT;
	}

	if (ti->failed_request_threshold == 0) {
		ti->failed_request_threshold = ULMU_TIMEOUT_LIMIT;
	}

	if (ti->callback_reporting_threshold == 0) {
		ti->callback_reporting_threshold = ULMU_CB_THRESHOLD;
	}
}

/* Init startstate of UTXD request */
static void
wlc_ulmu_drv_ulinfo_init(ulmu_drv_scbinfo_t *ulinfo, trigger_ctx_entry_t *entry)
{
	packet_trigger_info_t *ti;
	utxd_accmulator_t *accum;

	ASSERT(ulinfo);
	ASSERT(entry);

	ti = &entry->info;
	accum = &entry->accmulator;

	/* Apply default request fixups */
	wlc_ulmu_drv_request_default_fixups(ti);

	memset(accum, 0, sizeof(*accum));

	entry->state = ULMU_REQ_PENDING;
#ifndef BYTE_INTERFACE
	/* Calculate next airtime threshold */
	accum->delta_uscount =
		((ti->trigger_us * ti->callback_reporting_threshold)/100);

	accum->next_uscount =
		wlc_ulmu_drv_next_uscount(
			ti->trigger_us,
			accum->duration,
			accum->delta_uscount);
#else
	/* Calculate next byte threshold */
	accum->delta_bytecount =
		((ti->trigger_bytes * ti->callback_reporting_threshold)/100);

	accum->next_bytecount =
		wlc_ulmu_drv_next_bytecount(
			ti->trigger_bytes,
			accum->bytes_completed,
			accum->delta_bytecount);
#endif /* BYTE_INTERFACE */
}

static void
wlc_ulmu_drv_del_usr(wlc_info_t *wlc, scb_t *scb, scb_ulmu_t *ulmu_scb)
{
	ulmu_drv_scbinfo_t *ulinfo;
	trigger_ctx_entry_t *entry;
	packet_trigger_info_t *ti;

	ASSERT(ulmu_scb);

	ulinfo = ulmu_scb->ulinfo;
	ASSERT(ulinfo);

	entry = (ulinfo->current_req) ? (ulinfo->current_req) :
		wlc_ulmu_drv_get_next_trigger(scb, ulinfo);

	/* Flush pending UTXD requests */
	while (entry) {
		ti = &entry->info;
		if (ti->callback_function) {
			/* If current UTXD is being processed, complete and return
			 * ULMU_STATUS_EVICTED code
			 */
			if (ulinfo->current_req) {
#ifdef BYTE_INTERFACE
				uint32 avglen;
#endif
				utxd_accmulator_t *accum = &entry->accmulator;

#ifdef BYTE_INTERFACE
				avglen = (accum->npkts) ?
					(accum->bytes_completed/accum->npkts) : 0;
#endif
#ifndef BYTE_INTERFACE
				ti->callback_function(wlc, scb, ti, ti->callback_parameter,
					ULMU_STATUS_EVICTED, accum->duration);
#else
				ti->callback_function(wlc, scb, ti, ti->callback_parameter,
					ULMU_STATUS_EVICTED, accum->bytes_completed,
					avglen, accum->duration);
#endif

				/*
				 * From this point on, txtrigger statues
				 * will be dropped for UTXD requests
				 * already queued to ucode
				 */
				ulinfo->current_req = NULL;
			} else {
#ifndef BYTE_INTERFACE
				ti->callback_function(wlc, scb, ti, ti->callback_parameter,
					ULMU_STATUS_EVICTED, 0);
#else
				ti->callback_function(wlc, scb, ti, ti->callback_parameter,
					ULMU_STATUS_EVICTED, 0, 0, 0);
#endif
			}
		}
		entry->state = ULMU_REQ_INACTIVE;
		wlc_ulmu_drv_free_trigger(scb, ulinfo);
		entry = wlc_ulmu_drv_get_next_trigger(scb, ulinfo);
	}

	ASSERT(ulinfo->context_list.len == 0);
	ASSERT(ulinfo->context_list.headidx == ulinfo->context_list.tailidx);

	/* Clear ulinfo counters */
	ulinfo->nutxd = 0;
	ulinfo->qosnull_count = 0;
	ulinfo->prev_nutxd_status = 0;
	ulinfo->nutxd_status = 0;
	ulinfo->watchdog_count = 0;

	/* Reset start sequence number for UTXD requests */
	ulinfo->context_list.next_sequence_number = 1;

	WL_ULO(("%s: SCB:"MACF"\n", __FUNCTION__, ETHER_TO_MACF(scb->ea)));
} /* wlc_ulmu_drv_del_usr */

static void
wlc_ulmu_drv_add_usr(scb_ulmu_t* ulmu_scb)
{
	ASSERT(ulmu_scb);
	ASSERT(ulmu_scb->ulinfo);
	ASSERT(ulmu_scb->ulinfo->context_list.len == 0);
	ASSERT(ulmu_scb->ulinfo->context_list.headidx == ulmu_scb->ulinfo->context_list.tailidx);
}

/* Enqueue next UTXD, save context for accmulation and callback */
static int
wlc_ulmu_drv_queue_utxd(wlc_ulmu_info_t *ulmu,
	scb_t *scb, packet_trigger_info_t *ti_data)
{
	scb_ulmu_t* ulmu_scb;
	ulmu_drv_scbinfo_t *ulinfo;
	trigger_ctx_entry_t *entry;
	uint16 upd_mode;
	uint32 fid;

	ASSERT(ulmu);
	ASSERT(scb);
	ASSERT(ti_data);

	ulmu_scb = SCB_ULMU(ulmu, scb);
	ASSERT(ulmu_scb);

	ulinfo = ulmu_scb->ulinfo;
	ASSERT(ulinfo);

	/* Save the trigger request for accmulation processing */
	entry = wlc_ulmu_drv_save_trigger_context(scb, ulinfo, ti_data);

	if (!entry) {
		return BCME_ERROR;
	}

	wlc_ulmu_drv_ulinfo_init(ulinfo, entry);

	ulinfo->nutxd++;
	WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->nutxd);
	WLCNTADD(DRV_SCB_COUNTERS(ulmu_scb)->nbytes_triggered,
		entry->info.trigger_bytes);
#ifndef BYTE_INTERFACE
		WLCNTADD(DRV_SCB_COUNTERS(ulmu_scb)->nus_triggered,
			entry->info.trigger_us);
#endif

	/* Queue the UTXD to ucode, using run to complete mode if run2complete is set */
	upd_mode = ulmu->is_run2complete ? ULMU_UTXD_USRUPD_R2C : ULMU_UTXD_USRUPD_TRFC;
#ifndef BYTE_INTERFACE
		ulmu_scb->rel_cnt = entry->info.trigger_us;
#else
		ulmu_scb->rel_cnt = (entry->info.trigger_bytes >> ULMU_TRIGGER_SF);
#endif

	if (wlc_ulmu_prep_utxd(ulmu, scb, upd_mode, ti_data->pkt, &fid) != BCME_OK) {
		return BCME_ERROR;
	}
	entry->frameid = fid;

	WL_INFORM(("%s: SCB:"MACF" fid=0x%x request_us=%d seqno=%d slot=%d post=%d\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea),
			entry->frameid,
#ifndef BYTE_INTERFACE
			entry->info.trigger_us, entry->sequence_number,
#else
			entry->info.trigger_bytes, entry->sequence_number,
#endif

			entry->slot_number,
			ti_data->post_utxd));

	return ((ti_data->post_utxd) ? wlc_ulmu_post_utxd(ulmu) : BCME_OK);
} /* wlc_ulmu_drv_queue_utxd */

static int
wlc_ulmu_drv_packet_trigger_request(wlc_info_t *wlc, scb_t *scb,
	wlc_ulmu_trigger_info_t *trigger_info)
{
	wlc_ulmu_info_t *ulmu;
	scb_ulmu_t* ulmu_scb;

	ASSERT(wlc);
	ASSERT(scb);
	ASSERT(trigger_info);

	ulmu = wlc->ulmu;
	ASSERT(ulmu);

	ulmu_scb = SCB_ULMU(ulmu, scb);
	ASSERT(ulmu_scb);

	if (!ULMU_IS_UTXD(ulmu->mode)) {
		WL_ULO(("%s: SCB:"MACF" UTXD mode not enabled\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return BCME_DISABLED;
	}

	/* RXMONITOR admits the user, do not proceed unless client is already admitted */
	if (ulmu_scb->state != ULMU_SCB_ADMT) {
		WL_ULO(("%s: SCB:"MACF" not admitted for uplink\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return BCME_NOT_ADMITTED;
	}

	/* XXX Current ucode has a max of 64MBytes for the request bytesize
	 * Call to uCode routine is in kBytes hence the right shift of 10bits
	 */
	if ((KB(trigger_info->trigger_type.packet_trigger.trigger_bytes)) >
			(uint32)(0xFFFF)) {
		return BCME_BUFTOOLONG;
	}

	/* Post utxd */
	return wlc_ulmu_drv_queue_utxd(ulmu, scb, &trigger_info->trigger_type.packet_trigger);
} /* wlc_ulmu_drv_packet_trigger_request */

int
wlc_ulmu_drv_trigger_request(wlc_info_t *wlc, scb_t *scb,
	int trigger_type, wlc_ulmu_trigger_info_t *trigger_info)
{
	int ret = BCME_OK;

	ASSERT(wlc);
	ASSERT(scb);
	ASSERT(trigger_info);

	switch (trigger_type) {
		case ULMU_PACKET_TRIGGER:
			ret = wlc_ulmu_drv_packet_trigger_request(wlc, scb, trigger_info);
			break;

		case ULMU_TWT_TRIGGER:
		default:
			ret = BCME_ERROR;
	}

	return ret;
}

static void
wlc_ulmu_drv_process_utxd_status(wlc_ulmu_info_t *ulmu, scb_t *scb, ulmu_drv_utxd_status_t *status)
{
	scb_ulmu_t* ulmu_scb;
	ulmu_drv_scbinfo_t *ulinfo;
	packet_trigger_info_t *ti;
	uint32 status_code;
	uint32 return_code = ULMU_STATUS_INPROGRESS;
	trigger_ctx_entry_t *entry;
	utxd_accmulator_t *accum;
	bool doCallback;
	bool fm = FALSE;
#ifdef ULMU_NO_R2C
	uint32 txs_duration = status->duration;
#endif
	wlc_taf_info_t * taf_info = ulmu->wlc->taf_handle;
	/* simple approx for QOS NULL frame length */
	const uint32 qos_null_len = DOT11_MAC_HDR_LEN + DOT11_FCS_LEN;

	BCM_REFERENCE(fm);
	BCM_REFERENCE(doCallback);

	ASSERT(ulmu);
	ASSERT(status);
	status_code = status->return_status;

	ulmu_scb = SCB_ULMU(ulmu, scb);
	if (!SCB_ULOFDMA_ADM(scb) || !ulmu_scb ||
	    (SCB_MARKED_DEL(scb) || SCB_DEL_IN_PROGRESS(scb))) {
		return;
	}

	ASSERT(ulmu_scb);

	ulinfo = ulmu_scb->ulinfo;
	ASSERT(ulinfo);

	if (wlc_ulmu_drv_check_trigger(ulinfo, status->frameid) == FALSE) {
		return;
	}

	if (!ULMU_DRV_STOP_TIMEOUT(status_code) && !ULMU_DRV_STOP_SUPPRESS(status_code)) {
		taf_rxpkt_stats_t stats;

		memset(&stats, 0, sizeof(stats));

		stats.aggn = status->npkts;
		stats.total_agglen = status->triggered_bytes;
		stats.total_qos_null_len = qos_null_len * status->qn_total_count;
		stats.qos_nulls = status->qn_total_count;

		if (status->npkts == status->qn_total_count) {
			stats.qos_null_only = status->qn_total_count;
		}

		/* add received packets */
		wlc_taf_rxpkt_status(taf_info, scb, status->tid, 0, &stats,
			TAF_RXPKT_STATUS_UPDATE_BYTE_PKT_RATE_TRIG_STATUS);
	}

	/* These counters contain the same data
	 * counters->nutxd_status is used for dump info and can be cleared
	 * ulinfo->nutxd_status is the internal copy for the watchdog and
	 * is not cleared when dump clear is called.
	 */
	WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->nutxd_status);
	ulinfo->nutxd_status++;

#ifdef ULMU_NO_R2C
	if (!ulinfo->current_req) {
		ulinfo->current_req = wlc_ulmu_drv_get_next_trigger(scb, ulinfo);
		if (!ulinfo->current_req) {

			WL_ERROR(("%s: SCB:"MACF" No more UTXDs in Q , fid 0x%x dur %d\n",
					__FUNCTION__, ETHER_TO_MACF(scb->ea),
					status->frameid, txs_duration));
			return;
		}

	}

	entry = ulinfo->current_req;

	ti = &entry->info;
	accum = &entry->accmulator;

	if ((status->frameid < ulinfo->current_req->frameid) &&
		((ulinfo->current_req->frameid - status->frameid) <= ulmu_scb->pend_trig_cnt)) {
		// wrap around handling
		ulmu_scb->excess_trigger_time += txs_duration;

		WL_INFORM(("%s: SCB:"MACF" Not processing txs , fid(u/t)=(0x%x/0x%x) fm:%d seq=%d "
			"slot=%d rc=%d(0x%x) rate=(%d/%d) excess_dur %d\n", __FUNCTION__,
			ETHER_TO_MACF(scb->ea),
			entry->frameid, status->frameid, fm, entry->sequence_number,
			entry->slot_number, return_code, status_code,
			status->mcs, status->nss, txs_duration));

		return;
	}

	while (status->frameid != ulinfo->current_req->frameid) {
		doCallback = (ti->callback_function != NULL);
		ASSERT(doCallback);

		if (accum->duration >= ti->trigger_us) {
			return_code = ULMU_STATUS_COMPLETE;

			WL_INFORM(("%s: SCB:"MACF" utxd excess time fid 0x%x = %d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), entry->frameid,
				accum->duration - ti->trigger_us));

			ulmu_scb->excess_trigger_time += accum->duration - ti->trigger_us;

			WL_INFORM(("%s: SCB:"MACF" fid(u/t)=(0x%x/0x%x) fm:%d seq=%d slot=%d "
				"rc=%d(0x%x) rate=(%d/%d) dur=(%d/%d/%d)\n", __FUNCTION__,
				ETHER_TO_MACF(scb->ea),
				entry->frameid, status->frameid, fm, entry->sequence_number,
				entry->slot_number, return_code, status_code,
				status->mcs, status->nss, accum->duration, status->duration,
				ti->trigger_us));

			ti->callback_function(ulmu->wlc, scb, ti,
				ti->callback_parameter, ULMU_STATUS_COMPLETE, accum->duration);

			WL_INFORM(("%s: SCB:"MACF" utxd completed fid 0x%x\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), entry->frameid));

			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->ncallback);
			wlc_ulmu_drv_free_trigger(scb, ulinfo);
			ulinfo->current_req = NULL;
		} else if ((accum->duration + txs_duration) >= ti->trigger_us) {
			/* complete the utxd */
			return_code = ULMU_STATUS_COMPLETE;

			txs_duration -= ti->trigger_us - accum->duration;
			accum->duration = ti->trigger_us;

			WL_INFORM(("%s: SCB:"MACF" fid(u/t)=(0x%x/0x%x) fm:%d seq=%d slot=%d "
				"rc=%d(0x%x) rate=(%d/%d) dur=(%d/%d/%d)\n", __FUNCTION__,
				ETHER_TO_MACF(scb->ea),
				entry->frameid, status->frameid, fm, entry->sequence_number,
				entry->slot_number, return_code, status_code,
				status->mcs, status->nss, accum->duration, status->duration,
				ti->trigger_us));

			ti->callback_function(ulmu->wlc, scb, ti,
				ti->callback_parameter, ULMU_STATUS_COMPLETE, accum->duration);

			WL_INFORM(("%s: SCB:"MACF" utxd completed fid 0x%x\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), entry->frameid));

			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->ncallback);
			wlc_ulmu_drv_free_trigger(scb, ulinfo);
			ulinfo->current_req = NULL;
		} else if (ULMU_DRV_STOP_BYTES(status_code) ||
			ULMU_DRV_STOP_QOSNULL(status_code) ||
			ULMU_DRV_STOP_TIMEOUT(status_code) ||
			ULMU_DRV_STOP_SUPPRESS(status_code)) {
			if (ULMU_DRV_STOP_BYTES(status_code)) {
				return_code = ULMU_STATUS_COMPLETE;
			} else if (ULMU_DRV_STOP_QOSNULL(status_code)) {
				return_code = ULMU_STATUS_QOSNULL;
			} else if (ULMU_DRV_STOP_TIMEOUT(status_code)) {
				return_code = ULMU_STATUS_TIMEOUT;
			} else if (ULMU_DRV_STOP_SUPPRESS(status_code)) {
				return_code = ULMU_STATUS_SUPPRESS;
			}

			WL_INFORM(("%s: SCB:"MACF" fid(u/t)=(0x%x/0x%x) fm:%d seq=%d slot=%d "
				"rc=%d(0x%x) rate=(%d/%d) dur=(%d/%d/%d)\n", __FUNCTION__,
				ETHER_TO_MACF(scb->ea),
				entry->frameid, status->frameid, fm, entry->sequence_number,
				entry->slot_number, return_code, status_code,
				status->mcs, status->nss, accum->duration, status->duration,
				ti->trigger_us));

			ti->callback_function(ulmu->wlc, scb, ti,
				ti->callback_parameter, return_code, accum->duration);

			WL_INFORM(("%s: SCB:"MACF" utxd completed fid 0x%x\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), entry->frameid));

			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->ncallback);
			wlc_ulmu_drv_free_trigger(scb, ulinfo);
			ulinfo->current_req = NULL;
		} else {
			break;
		}

		if (ulinfo->current_req == NULL) {
			ulinfo->current_req = wlc_ulmu_drv_get_next_trigger(scb, ulinfo);
			if (!ulinfo->current_req) {
				if (txs_duration) {
					WL_ERROR(("%s: SCB:"MACF" No more UTXD left, utxd excess "
						"time  %d\n", __FUNCTION__,
						ETHER_TO_MACF(scb->ea), txs_duration));
					ulmu_scb->excess_trigger_time += txs_duration;
				}

				return;
			}

			entry = ulinfo->current_req;

			ti = &entry->info;
			accum = &entry->accmulator;
		}
	}

	if (ulinfo->current_req) {
		/* Check for the STOP conditions, callback invoked if any of the below is met */
		if (ULMU_DRV_STOP_BYTES(status_code)) {
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_byte_limit);
			return_code = ULMU_STATUS_COMPLETE;
		} else if (ulmu_scb->state != ULMU_SCB_ADMT) {
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_evicted);
			return_code = ULMU_STATUS_NOTADMITTED;
		} else if (ULMU_DRV_STOP_QOSNULL(status_code)) {
		/* uCode terminates the UTXD request and
		 * indicates status 0x2
		 */
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_qos_null);
			return_code = ULMU_STATUS_QOSNULL;
		} else if (ULMU_DRV_STOP_TIMEOUT(status_code)) {
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_timeout);
			return_code = ULMU_STATUS_TIMEOUT;
		} else if (ULMU_DRV_STOP_SUPPRESS(status_code)) {
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_suppress);
			return_code = ULMU_STATUS_SUPPRESS;
		} else if (ULMU_DRV_STOP_TRIGCNT(status_code)) {
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_trigcnt);
			return_code = ULMU_STATUS_TRIGCNT;
		} else if (txs_duration) {
			accum->duration += txs_duration;
			return_code = ULMU_STATUS_INPROGRESS;
			WL_INFORM(("%s: SCB:"MACF" utxd partially completed fid 0x%x\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), entry->frameid));
		} else {
			return;
		}

		WL_INFORM(("%s: SCB:"MACF" fid(u/t)=(0x%x/0x%x) fm:%d seq=%d slot=%d rc=%d(0x%x) "
			"rate=(%d/%d) dur=(%d/%d/%d)\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea),
			entry->frameid, status->frameid, fm, entry->sequence_number,
			entry->slot_number, return_code, status_code,
			status->mcs, status->nss, accum->duration, status->duration,
			ti->trigger_us));

		ti->callback_function(ulmu->wlc, scb, ti,
			ti->callback_parameter, return_code, accum->duration);

		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->ncallback);

		if (return_code != ULMU_STATUS_INPROGRESS) {
			WL_INFORM(("%s: SCB:"MACF" utxd completed fid 0x%x\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), entry->frameid));

			wlc_ulmu_drv_free_trigger(scb, ulinfo);
			ulinfo->current_req = NULL;
		}

	}
#else

	if (ulinfo->current_req == NULL) {
		ulinfo->current_req = wlc_ulmu_drv_get_next_trigger(scb, ulinfo);
		if (ulinfo->current_req) {
			 ulinfo->current_req->state = ULMU_REQ_ACTIVE;
		}
	}

	/* Spurious status. Ignore.
	 * ulmu_scb->bufsize = 0 is used to suppress
	 * the unwanted autonomous triggering in ucode
	 */
	if (ulinfo->current_req == NULL) {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->no_request);
		/* No more saved uplink requests */
		WL_ERROR(("%s: SCB:"MACF" Missing trigger, rc=0x%x qn=%d "
			"fid=0x%x cp=%d pkts=%d rate=(%d/%d)\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea),
			status->return_status, status->qn_count,
			status->frameid, status->triggered_bytes,
			status->npkts, status->mcs, status->nss));
		return;
	}

	entry = ulinfo->current_req;

	ti = &entry->list;

	doCallback = (ti->callback_function != NULL);
	ASSERT(doCallback);

	accum = &entry->accmulator;
	accum->bytes_completed += status->triggered_bytes;
	accum->npkts += status->npkts;
	accum->duration += status->duration;

	if (status->triggered_bytes == 0) {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->zero_bytes);
	}

	if (status->frameid != entry->frameid) {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->fid_mismatch);
		if (entry->next) {
			WL_ERROR(("SCB: "MACF" fid mismatch cnt %d txs fid %x utxd fid %x next "
				"utxd fid %x\n",
				ETHER_TO_MACF(scb->ea), DRV_SCB_COUNTERS(ulmu_scb)->fid_mismatch,
				status->frameid, entry->frameid, entry->next->frameid));
		} else {
			WL_ERROR(("SCB: "MACF" fid mismatch cnt %d txs fid %x utxd fid %x next "
				"utxd fid NULL\n",
				ETHER_TO_MACF(scb->ea), DRV_SCB_COUNTERS(ulmu_scb)->fid_mismatch,
				status->frameid, entry->frameid));
		}
		fm = TRUE;
	} else {
		if (entry->next) {
			WL_INFORM(("SCB: "MACF" NO fid mismatch cnt %d txs fid %x utxd fid %x "
				"next utxd fid %x\n",
				ETHER_TO_MACF(scb->ea), DRV_SCB_COUNTERS(ulmu_scb)->fid_mismatch,
				status->frameid, entry->frameid, entry->next->frameid));
		} else {
			WL_INFORM(("SCB: "MACF" NO fid mismatch cnt %d txs fid %x utxd fid %x "
				"next utxd fid NULL\n",
				ETHER_TO_MACF(scb->ea), DRV_SCB_COUNTERS(ulmu_scb)->fid_mismatch,
				status->frameid, entry->frameid));
		}
	}

	WLCNTADD(DRV_SCB_COUNTERS(ulmu_scb)->nbytes_received,
		status->triggered_bytes);

	/* Check for the STOP conditions, callback invoked if any of the below is met */
	if (ULMU_DRV_STOP_BYTES(status_code)) {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_byte_limit);
		return_code = ULMU_STATUS_COMPLETE;
	} else if (ulmu_scb->state != ULMU_SCB_ADMT) {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_evicted);
		return_code = ULMU_STATUS_NOTADMITTED;
	} else if (ULMU_DRV_STOP_QOSNULL(status_code)) {
	/* uCode terminates the UTXD request and
	 * indicates status 0x2
	 */
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_qos_null);
		return_code = ULMU_STATUS_QOSNULL;
	} else if (ULMU_DRV_STOP_TIMEOUT(status_code)) {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_timeout);
		return_code = ULMU_STATUS_TIMEOUT;
	} else if (ULMU_DRV_STOP_SUPPRESS(status_code)) {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_suppress);
		return_code = ULMU_STATUS_SUPPRESS;
	} else if (ULMU_DRV_STOP_TRIGCNT(status_code)) {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->stop_trigcnt);
		return_code = ULMU_STATUS_TRIGCNT;
#ifndef BYTE_INTERFACE
	} else if (accum->duration >= accum->next_uscount) {
#else
	} else if (accum->bytes_completed >= accum->next_bytecount) {
#endif
		/* Calculate next threshold if multi-callback */
		if (ti->multi_callback) {
#ifndef BYTE_INTERFACE
			accum->next_uscount = wlc_ulmu_drv_next_uscount(
				ti->trigger_us,
				accum->duration,
				accum->delta_uscount);
#else
			accum->next_bytecount = wlc_ulmu_drv_next_bytecount(
				ti->trigger_bytes,
				accum->bytes_completed,
				accum->delta_bytecount);
#endif
			return_code = ULMU_STATUS_INPROGRESS;
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->multi_callback);
		} else {
			return_code = ULMU_STATUS_THRESHOLD;
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->threshold_callback);
			/* Set next threshold to above requested in trigger */
#ifndef BYTE_INTERFACE
			accum->next_uscount = (uint32)(-1);
#else
			accum->next_bytecount = (uint32)(-1);
#endif
		}
	} else {
		doCallback = FALSE;
#ifndef BYTE_INTERFACE
		if (accum->duration >= ti->trigger_us) {
			accum->overrun++;
			accum->overrun_us += status->duration;
		}
#else
		if (accum->bytes_completed >= ti->trigger_bytes) {
			accum->overrun++;
			accum->overrun_bytes += status->triggered_bytes;
		}
#endif
	}

	if (ULMU_DRV_STOP(return_code)) {
		ulinfo->current_req->state = ULMU_REQ_COMPLETED;
	}

	if (doCallback) {
#ifdef BYTE_INTERFACE
		uint32 avglen = (accum->npkts) ?
			(accum->bytes_completed/accum->npkts) : 0;
#endif
		WL_ULO(("%s: SCB:"MACF" Invoke callback cb:0x%p\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea),
			ti->callback_function));

#ifndef BYTE_INTERFACE
		ti->callback_function(ulmu->wlc, scb, ti,
			ti->callback_parameter, return_code, accum->duration);
#else
		ti->callback_function(ulmu->wlc, scb, ti,
			ti->callback_parameter, return_code, accum->bytes_completed,
			avglen, accum->duration);
#endif

		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->ncallback);
	} else {
		WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->no_callback);
	}

	WL_ULO(("%s: SCB:"MACF" fid(u/t)=(0x%x/0x%x) fm:%d seq=%d slot=%d rc=%d(0x%x) rate=(%d/%d)"
			" bytes(rq=%d cp=%d  ac=%d nx=%d) qn=%d cb(%d/%d)\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea),
			entry->frameid, status->frameid, fm, entry->sequence_number,
			entry->slot_number, return_code, status_code,
			status->mcs, status->nss, ti->trigger_bytes, status->triggered_bytes,
#ifndef BYTE_INTERFACE
			accum->bytes_completed, accum->next_uscount,
#else
			accum->bytes_completed, accum->next_bytecount,
#endif
			status->qn_count, doCallback, ti->multi_callback));

	/* UTXD request termination condition, invalidate current request pointer */
	if (ULMU_DRV_STOP(return_code)) {
		if (accum->overrun) {
			WLCNTINCR(DRV_SCB_COUNTERS(ulmu_scb)->overrun);
		}
		WL_ULO(("%s: SCB:"MACF" UTXD complete. fid(u/t)=(0x%x/0x%x) "
			"Seqno=%d  Overruns=%d (%d bytes)\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea),
			entry->frameid, status->frameid, entry->sequence_number,
#ifndef BYTE_INTERFACE
			accum->overrun, accum->overrun_us));
#else
			accum->overrun, accum->overrun_bytes));
#endif
		wlc_ulmu_drv_free_trigger(scb, ulinfo);
		ulinfo->current_req = NULL;
	}
#endif /* ULMU_NO_R2C */
} /* wlc_ulmu_drv_process_utxd_status */

static void
wlc_ulmu_drv_watchdog(wlc_ulmu_info_t *ulmu)
{
	uint max_ulofdma_usrs;
	uint schpos;

	ASSERT(ulmu);

	if (wlc_ulmu_drv_get_watchdog(ulmu) == 0) {
		return;
	}

	max_ulofdma_usrs = wlc_txcfg_max_clients_get(ulmu->wlc->txcfg, ULOFDMA);

	for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
		scb_t *scb = ulmu->scb_list[schpos];

		if (scb) {
			scb_ulmu_t *ulmu_scb = SCB_ULMU(ulmu, scb);
			ulmu_drv_scbinfo_t *ulinfo;
			ulmu_stats_t* scb_stats;
			packet_trigger_info_t *ti;
			bool clear_watchdog = TRUE;

			if (!ulmu_scb) {
				continue;
			}
			scb_stats = ULMU_SCB_STATS(ulmu_scb);
			ulinfo = ulmu_scb->ulinfo;
			ASSERT(ulinfo);

			if (ulmu_scb->state != ULMU_SCB_ADMT) {
				continue;
			}

			if (!ulinfo->current_req) {
				continue;
			}

			ti = &ulinfo->current_req->info;

			if (ti->watchdog_timeout == 0) {
				continue;
			}

			WL_ULO(("%s: SCB:"MACF" watchdog_count=%d. curr_utxd=%d prev_utxd=%d\n",
					__FUNCTION__, ETHER_TO_MACF(scb->ea),
					ulinfo->watchdog_count,
					ulinfo->nutxd_status, ulinfo->prev_nutxd_status));

			if (ulinfo->nutxd_status == ulinfo->prev_nutxd_status) {
				ulinfo->watchdog_count++;
				clear_watchdog = FALSE;
				WL_ULO(("%s: SCB:"MACF" watchdog_count=%d.\n",
					__FUNCTION__, ETHER_TO_MACF(scb->ea),
					ulinfo->watchdog_count));
			} else {
				ulinfo->prev_nutxd_status = ulinfo->nutxd_status;
			}

			if (ulinfo->watchdog_count >= ti->watchdog_timeout) {
				/* Terminate UTXD request if the watchdog
				 * count threshold is reached
				 */
				ulinfo->current_req->state = ULMU_REQ_COMPLETED;
				if (ti->callback_function) {
					utxd_accmulator_t *accum;
#ifdef BYTE_INTERFACE
					uint32 avglen;
#endif

					accum = &ulinfo->current_req->accmulator;
#ifdef BYTE_INTERFACE
					avglen = (accum->npkts) ?
						(accum->bytes_completed/accum->npkts) : 0;
#endif

					ti->callback_function(ulmu->wlc,
						scb, ti, ti->callback_parameter,
#ifndef BYTE_INTERFACE
						ULMU_STATUS_WATCHDOG, accum->duration);
#else
						ULMU_STATUS_WATCHDOG, accum->bytes_completed,
						avglen, accum->duration);
#endif

					WL_ULO(("%s: SCB:"MACF" Terminating UTXD request,"
						"watchdog_count=%d.\n",
						__FUNCTION__, ETHER_TO_MACF(scb->ea),
						ulinfo->watchdog_count));
				}

				/* if ulinfo->watchdog_count was incremented
				 * clear_watchdog will be FALSE
				 */
				clear_watchdog = TRUE;
				WLCNTINCR(scb_stats->drv_counters.stop_watchdog);
				wlc_ulmu_drv_free_trigger(scb, ulinfo);
				ulinfo->current_req = NULL;
			}

			if (clear_watchdog) {
				ulinfo->watchdog_count = 0;
			}
		}
	}
} /* wlc_ulmu_drv_watchdog */

#ifdef BCMDBG
/* Apply global user confgured overides when making new UTXD request */
static void
wlc_ulmu_drv_apply_iovar_settings(wlc_ulmu_info_t *ulmu, wlc_ulmu_trigger_info_t *trigger_info)
{
	packet_trigger_info_t *ti;

	ASSERT(ulmu);
	ASSERT(trigger_info);

	ti = &trigger_info->trigger_type.packet_trigger;

	ti->trigger_bytes = wlc_ulmu_drv_get_bytecount(ulmu);
	ti->watchdog_timeout = wlc_ulmu_drv_get_watchdog(ulmu);
}

/* Prepare a generic trigger request */
static void
wlc_ulmu_drv_prepare_trigger_request(wlc_ulmu_info_t *ulmu,
		wlc_ulmu_trigger_info_t *ti_data, ulmu_cb_t callback_function)
{
	packet_trigger_info_t *ti;

	ASSERT(ti_data);

	/* Apply IOVAR override values */
	wlc_ulmu_drv_apply_iovar_settings(ulmu, ti_data);
	ti = &ti_data->trigger_type.packet_trigger;

	ti->callback_function = callback_function;
	ti->callback_parameter = (void *)ti;
	ti->callback_reporting_threshold = ULMU_CB_TEST_THRESHOLD;
	ti->multi_callback = FALSE;
	ti->qos_null_threshold = ULMU_QOSNULL_LIMIT;
	ti->failed_request_threshold = ULMU_TIMEOUT_LIMIT;
}

/* Get auto trigger state */
static uint32
wlc_ulmu_drv_get_autotrigger(wlc_ulmu_info_t *ulmu)
{
	ASSERT(ulmu);

	return ulmu->drv_gconfig.drv_autotrigger;
}

/* Set trigger auto state */
static void
wlc_ulmu_drv_set_autotrigger(wlc_ulmu_info_t *ulmu, uint32 iter)
{
	ASSERT(ulmu);
	ulmu->drv_gconfig.drv_autotrigger = iter;
}

/* TAF callback stub, this stub queues 1 UTXD per callback */
#ifndef BYTE_INTERFACE
static int wlc_ulmu_drv_trigger_callback(wlc_info_t *, scb_t *,
	packet_trigger_info_t *ti,
	void *, uint32, uint32);
#else
static int wlc_ulmu_drv_trigger_callback(wlc_info_t *, scb_t *,
	packet_trigger_info_t *ti,
	void *, uint32, uint32, uint32, uint32);
#endif

#ifndef BYTE_INTERFACE
static int
wlc_ulmu_drv_trigger_callback(wlc_info_t *wlc, scb_t *scb,
	packet_trigger_info_t *ti,
	void *arg, uint32 status_code, uint32 duration)
#else
static int
wlc_ulmu_drv_trigger_callback(wlc_info_t *wlc, scb_t *scb,
	packet_trigger_info_t *ti,
	void *arg, uint32 status_code, uint32 bytes_consumed,
	uint32 avg_pktsz, uint32 duration)
#endif
{
	wlc_ulmu_info_t *ulmu;
	bool newTrigger = FALSE;
	uint32 iter;

	ASSERT(wlc);
	ASSERT(scb);
	ASSERT(arg);

	ulmu = wlc->ulmu;

	if (ti == NULL) {
			WL_ULO(("%s: SCB:"MACF". ti is NULL\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea)));
			return BCME_ERROR;
	}

#ifndef BYTE_INTERFACE
	switch (status_code) {
		case ULMU_STATUS_INPROGRESS:
			WL_ULO(("%s: SCB:"MACF" Inprog. %d%% dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				ti->callback_reporting_threshold, duration));
			break;
		case ULMU_STATUS_THRESHOLD:
			WL_ULO(("%s: SCB:"MACF" Thres. %d%% dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				ti->callback_reporting_threshold, duration));
			break;
		case ULMU_STATUS_COMPLETE:
			WL_ULO(("%s: SCB:"MACF" Done. dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_QOSNULL:
			WL_ULO(("%s: SCB:"MACF" ABRT:QN. dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_TIMEOUT:
			WL_ULO(("%s: SCB:"MACF" ABRT:TO. dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_TRIGCNT:
			WL_ULO(("%s: SCB:"MACF" ABRT:TC. dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_WATCHDOG:
			WL_ULO(("%s: SCB:"MACF" ABRT:WD. dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_EVICTED:
			WL_ULO(("%s: SCB:"MACF" ABRT:EV. dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), duration));
			/* User got kicked out of UL schedule , no new trigger */
			break;
		case ULMU_STATUS_NOTADMITTED:
			WL_ULO(("%s: SCB:"MACF" ABRT:NA. dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), duration));
			/* User got kicked out of UL schedule , no new trigger */
			break;
		default:
			WL_ULO(("%s: SCB:"MACF" UNK:%d. dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), status_code, duration));
			break;
	}
#else
	switch (status_code) {
		case ULMU_STATUS_INPROGRESS:
			WL_ULO(("%s: SCB:"MACF" Inprog. %d%% bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				ti->callback_reporting_threshold,
				bytes_consumed, avg_pktsz, duration));
			break;
		case ULMU_STATUS_THRESHOLD:
			WL_ULO(("%s: SCB:"MACF" Thres. %d%% bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				ti->callback_reporting_threshold,
				bytes_consumed, avg_pktsz, duration));
			break;
		case ULMU_STATUS_COMPLETE:
			WL_ULO(("%s: SCB:"MACF" Done. bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				bytes_consumed, avg_pktsz, duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_QOSNULL:
			WL_ULO(("%s: SCB:"MACF" ABRT:QN. bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				bytes_consumed, avg_pktsz, duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_TIMEOUT:
			WL_ULO(("%s: SCB:"MACF" ABRT:TO. bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				bytes_consumed, avg_pktsz, duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_TRIGCNT:
			WL_ULO(("%s: SCB:"MACF" ABRT:TC. bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				bytes_consumed, avg_pktsz, duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_WATCHDOG:
			WL_ULO(("%s: SCB:"MACF" ABRT:WD. bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				bytes_consumed, avg_pktsz, duration));
			newTrigger = TRUE;
			break;
		case ULMU_STATUS_EVICTED:
			WL_ULO(("%s: SCB:"MACF" ABRT:EV. bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				bytes_consumed, avg_pktsz, duration));
			/* User got kicked out of UL schedule , no new trigger */
			break;
		case ULMU_STATUS_NOTADMITTED:
			WL_ULO(("%s: SCB:"MACF" ABRT:NA. bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				bytes_consumed, avg_pktsz, duration));
			/* User got kicked out of UL schedule , no new trigger */
			break;
		default:
			WL_ULO(("%s: SCB:"MACF" UNK:%d. bytes=%d avglen=%d dur=%d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), status_code,
				bytes_consumed, avg_pktsz, duration));
			break;
	}
#endif /* BYTE_INTERFACE */

	iter = wlc_ulmu_drv_get_autotrigger(ulmu);
	newTrigger &= (iter > 0);

	if (newTrigger) {
		wlc_ulmu_trigger_info_t trigger_info;
		uint32 n;
		int rc;

		/* Prepare trigger request */
		wlc_ulmu_drv_prepare_trigger_request(ulmu, &trigger_info,
			wlc_ulmu_drv_trigger_callback);

		for (n = 0; n < iter; n++) {

			rc = wlc_ulmu_drv_trigger_request(ulmu->wlc, scb,
					ULMU_PACKET_TRIGGER, &trigger_info);

			if (rc == BCME_OK) {
				WL_ULO(("%s: SCB:"MACF" triggered.\n",
					__FUNCTION__, ETHER_TO_MACF(scb->ea)));
			} else {
				WL_ULO(("%s: SCB:"MACF" error rc=0x%x.\n",
					__FUNCTION__, ETHER_TO_MACF(scb->ea), rc));
				break;
			}
		}
		return (rc);
	} else {
		return BCME_OK;
	}
} /* wlc_ulmu_drv_trigger_callback */

/* TAF Stub */
static void
wlc_ulmu_drv_trigger_scb(wlc_ulmu_info_t *ulmu, uint16 niter)
{
	uint max_ulofdma_usrs;
	uint schpos;
	wlc_ulmu_trigger_info_t trigger_info;
	int rc, iter = 0;

	if (niter < 1) {
		return;
	};

	ASSERT(ulmu);
	max_ulofdma_usrs = wlc_txcfg_max_clients_get(ulmu->wlc->txcfg, ULOFDMA);

	/* Prepare trigger request */
	wlc_ulmu_drv_prepare_trigger_request(ulmu, &trigger_info,
		wlc_ulmu_drv_trigger_callback);

	rc = BCME_OK;
	do {
		iter++;
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			scb_t *scb = ulmu->scb_list[schpos];

			if (scb) {
				rc = wlc_ulmu_drv_trigger_request(ulmu->wlc, scb,
					ULMU_PACKET_TRIGGER, &trigger_info);
				WL_ULO(("%s: SCB:"MACF" triggered.\n",
					__FUNCTION__, ETHER_TO_MACF(scb->ea)));
				if (rc != BCME_OK) {
					break;
				} /* if rc... */
			} /* if (scb)... */
		} /* for(...) */
	} while ((iter < niter) && (rc == BCME_OK));
} /* wlc_ulmu_drv_trigger_scb */

/* UTXD Request list debugging routines */
/* Return current active UTXD request pointer */
trigger_ctx_entry_t *
wlc_ulmu_active_utxd_request(wlc_info_t *wlc, scb_t *scb)
{
	scb_ulmu_t *ulmu_scb;

	if (!wlc || !wlc->ulmu || !scb) {
		return NULL;
	}

	ulmu_scb = SCB_ULMU(wlc->ulmu, scb);

	if (!ulmu_scb) {
		return NULL;
	}

	ASSERT(ulmu_scb->ulinfo);

	return ulmu_scb->ulinfo->current_req;
}
#endif /* BCMDBG */

#if defined(BCMDBG_DUMP_UMSCHED)
static void
wlc_ulmu_drv_print_scb_stats(scb_ulmu_t *ulmu_scb, scb_t *scb, bcmstrbuf_t *b)
{
	ulmu_drv_counters_t *counters;
	ulmu_drv_scbinfo_t *ulinfo;

	ASSERT(ulmu_scb);
	ASSERT(scb);
	ASSERT(b);
	ASSERT(ULMU_SCB_STATS(ulmu_scb));

	counters = DRV_SCB_COUNTERS(ulmu_scb);
	ASSERT(counters);

	ulinfo = ulmu_scb->ulinfo;
	ASSERT(ulinfo);

	bcm_bprintf(b, "\nDriver trigger stats summary:\n");
	bcm_bprintf(b, "  nutxd=%d npendutxd=%d ntxststus=%d ncallback=%d "
		"npending=%d fid_mismatch=%d\n",
		counters->nutxd, SCB_UTXD_INFLT_VAL(scb), counters->nutxd_status,
		counters->ncallback, ulinfo->context_list.len, counters->fid_mismatch);
	bcm_bprintf(b, "  bytes: triggered=%d received=%d zero=%d ovr=%d\n",
		counters->nbytes_triggered, counters->nbytes_received,
		counters->zero_bytes, counters->overrun);
	bcm_bprintf(b, "Driver UTXD completion reasons:\n");
	bcm_bprintf(b, "  bytelimit=%d qosnull=%d timeout=%d suppress=%d"
		" trigcnt=%d watchdog=%d evicted=%d\n",
		counters->stop_byte_limit, counters->stop_qos_null,
		counters->stop_timeout, counters->stop_suppress,
		counters->stop_trigcnt, counters->stop_watchdog, counters->stop_evicted);
	bcm_bprintf(b, "Driver Callback counts:\n");
	bcm_bprintf(b, "  thres=%d mult=%d spur=%d nocb=%d noreq=%d\n",
		counters->threshold_callback, counters->multi_callback,
		counters->spurious_callback, counters->no_callback, counters->no_request);

	if (ulinfo->current_req) {
		packet_trigger_info_t *ti = &ulinfo->current_req->info;

		bcm_bprintf(b, "Settings:\n");
		bcm_bprintf(b, "  bytes=%d threshold(cb=%d qn=%d tout=%d) wd=%d mc=%d\n",
			ti->trigger_bytes, ti->callback_reporting_threshold,
			ti->qos_null_threshold, ti->failed_request_threshold,
			ti->watchdog_timeout, ti->multi_callback);
	};
	bcm_bprintf(b, "\n");
}
#endif /* BCMDBG_DUMP_UMSCHED */

#else

#define wlc_ulmu_drv_print_scb_stats(ulmu_scb, scb, b)
#define wlc_ulmu_drv_scb_init(ulmu, scb)	(BCME_OK)
#define wlc_ulmu_drv_scb_deinit(ulmu, scb)
#define wlc_ulmu_drv_ulmu_attach(ulmu)
#define wlc_ulmu_drv_ulmu_detach(ulmu)
#define wlc_ulmu_drv_add_usr(ulmu_scb)
#define wlc_ulmu_drv_del_usr(wlc, scb, ulmu_scb)

#ifndef BYTE_INTERFACE
static int
wlc_ulmu_release_bytes(wlc_ulmu_info_t *ulmu, scb_t *scb, uint16 us)
#else
static int
wlc_ulmu_release_bytes(wlc_ulmu_info_t *ulmu, scb_t *scb, uint16 bufsize)
#endif
{
	int ret;
	scb_ulmu_t* ulmu_scb = SCB_ULMU(ulmu, scb);

#ifndef BYTE_INTERFACE
	ulmu_scb->rel_cnt = us; // in KB unit
#else
	ulmu_scb->rel_cnt = bufsize; // in KB unit
#endif

	if (wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRUPD_TRFC, NULL, NULL) != BCME_OK) {
		return BCME_ERROR;
	}
	ret = wlc_ulmu_post_utxd(ulmu);

	return ret;
}

#endif /* ULMU_DRV */

static bool
wlc_ulmu_drv_ucautotrigger(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	bool ret = TRUE;
	int err;
	scb_ulmu_t* ulmu_scb = SCB_ULMU(ulmu, scb);

	BCM_REFERENCE(ulmu_scb);
	ASSERT(ulmu_scb);

#ifndef BYTE_INTERFACE
	SCB_ULMU(ulmu, scb)->rel_cnt = ULMU_DURSZ_DFLT;
#else
	SCB_ULMU(ulmu, scb)->rel_cnt = ULMU_BUFSZ_DFLT;
#endif

	if (ULMU_FLAGS_DSCHED_GET(ulmu->flags) == ULMU_ON) {
		err = wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRUPD_RATE, NULL, NULL);
	} else {
		err = wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRADD_ALL, NULL, NULL);
	}
	if (err != BCME_OK) {
		return FALSE;
	}

	if ((err = wlc_ulmu_post_utxd(ulmu)) != BCME_OK) {
		ret = FALSE;
		WL_ERROR(("wl%d: %s: fail to post utxd for "MACF" err %d schpos %d "
			"state 0x%x into ul ofdma list ulmu_state:%d\n",
			ulmu->wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea), err,
			ulmu_scb->schpos, scb->state, ulmu_scb->state));
	} else {
		WL_ULO(("wl%d: %s: add sta "MACF" schpos %d state 0x%x "
			"into ul ofdma list ulmu_state:%d\n",
			ulmu->wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea),
			ulmu_scb->schpos, scb->state, ulmu_scb->state));
	}

	return ret;
} /* wlc_ulmu_drv_ucautotrigger */

static void
wlc_ulmu_stats_upd_queue_size(uint16 qos_field, ulmu_stats_t *ulstats)
{
	uint8 qs_uv = 0, qs_sf = 0, tid = 0;
	qs_sf = QOS_QS_SF(qos_field);
	qs_uv = QOS_QS_UV(qos_field);
	tid = QOS_TID(qos_field);
	BCM_REFERENCE(tid);

	if ((qos_field & QOS_QS_PRESENT_MASK) == 0) {
		/* queue_size info is not present */
		return;
	}

	/* The formula for qsz calculation is in 11ax spec D6.0
	 * 9.2.4.5.6 Queue Size subfield, Eq (9-0a)
	 */
	if (qs_sf == 0) {
		ulstats->cur_qsz = 16 * qs_uv;
	} else if (qs_sf == 1) {
		ulstats->cur_qsz = 1024 + 256 * qs_uv;
	} else if (qs_sf == 2) {
		ulstats->cur_qsz = 17408 + 2048 * qs_uv;
	} else if (qs_sf == 3 && qs_uv < 63) {
		ulstats->cur_qsz = 148480 + 32768 * qs_uv;
		/* Note that qs>2147328, if the qs_sf is 3 qs_uv is 62 */
	} else {
		/* Unspecified or Unknown */
		ulstats->cur_qsz = -1;
		return;
	}

	ULMU_QS_EMA(ulstats->avg_qsz, (ulstats->cur_qsz << ULMU_QS_UPSCALE_FACTOR),
		ULMU_QS_UPSCALE_FACTOR);
}

int
wlc_ulmu_stats_upd(wlc_ulmu_info_t *ulmu, scb_t *scb, tx_status_t *txs)
{
	uint8 nusrs, rx_cnt, rxsucc_cnt;
	uint8 mcs, nss, rateidx, ulmm, last, reason;
	int32 phyrssi;
	uint32 txop, qncnt, agglen;
	scb_ulmu_t* ulmu_scb;
	ulmu_stats_t *ulstats;
	uint32 cur_mlen = 0;
	ru_type_t ru_type;
	ratespec_t rspec;
	uint32 triggered_bytes;
	uint32 cmi01, cmi12;
	ulmu_gstats_t* gstats;
	uint16 qos_field;
	uint32 ru_rate;
#ifdef WLTAF
	uint32 usec_txdur;
#endif
	uint32 payload_txtime;

	ulmu_scb = SCB_ULMU(ulmu, scb);

	if (ulmu_scb == NULL) {
		return BCME_ERROR;
	}

	if ((ulstats = ULMU_SCB_STATS(ulmu_scb)) == NULL) {
		return BCME_ERROR;
	}

	wlc_macdbg_dtrace_log_utxs(ulmu->wlc->macdbg, scb, txs);

	cmi01 = TGTXS_CMNINFOP1(TX_STATUS_MACTXS_S2(txs));
	cmi12 = TGTXS_CMNINFOP2(TX_STATUS_MACTXS_S3(txs));
	cmi01 = (cmi12 << 16) | cmi01;
	/* 3*txop = ((lsig + 5) << 2) + 51 */
	txop = (((TGTXS_LSIG(TX_STATUS_MACTXS_S2(txs)) + 5) << 2) + 51) << OPERAND_SHIFT;
	mcs = TGTXS_MCS(TX_STATUS_MACTXS_S4(txs));
	nss = TGTXS_NSS(TX_STATUS_MACTXS_S4(txs));
	rateidx = nss * MAX_HE_RATES + mcs;
	ASSERT(rateidx < AMPDU_MAX_HE);
	qncnt = TGTXS_QNCNT(TX_STATUS_MACTXS_ACK_MAP1(txs));
	agglen = TGTXS_AGGLEN(TX_STATUS_MACTXS_ACK_MAP1(txs));
	phyrssi = TGTXS_PHYRSSI(TX_STATUS_MACTXS_S8(txs));
	ASSERT(phyrssi < PHYRSSI_2SCOMPLEMENT);
	phyrssi = (phyrssi - ((phyrssi >= PHYRSSI_SIGN_MASK) <<
		PHYRSSI_2SCOMPLEMENT_SHIFT)) << OPERAND_SHIFT;
	nusrs = TGTXS_NUSR(TX_STATUS_MACTXS_S1(txs));
	ASSERT(nusrs > 0);
	ulmm = TGTXS_ULMM(TX_STATUS_MACTXS_S5(txs));
	last = TGTXS_LAST(TX_STATUS_MACTXS_S5(txs));
	rx_cnt = TGTXS_LCNT(TX_STATUS_MACTXS_S5(txs));
	rxsucc_cnt = TGTXS_GDFCSCNT(TX_STATUS_MACTXS_S5(txs));
	gstats = ulmm ? ulmu->mmu_gstats : ulmu->omu_gstats;
	qos_field = TGTXS_QOS(TX_STATUS_MACTXS_ACK_MAP2(txs));

	if (gstats == NULL) {
		return BCME_ERROR;
	}

	/* update ul ofdma global stats */
	if (rx_cnt > 0) {
		WLCNTADD(gstats->lcnt[rateidx], rx_cnt);
		WLCNTADD(gstats->gdfcscnt[rateidx], rxsucc_cnt);
	}
	if (last == 1)
		WLCNTADD(gstats->usrhist[nusrs-1], 1);

	/* update ul ofdma per user stats */
	WLCNTADD(ulstats->nupd, 1);

	if (rx_cnt == 0) {
		WLCNTADD(ulstats->nfail, 1);
		ulmu_scb->fail_cnt++;
	} else {
		WLCNTADD(ulstats->sum_lcnt, rx_cnt);
		WLCNTADD(ulstats->lcnt[rateidx], rx_cnt);
		WLCNTADD(ulstats->gdfcscnt[rateidx], rxsucc_cnt);
		WLCNTADD(ulstats->tx_nsscnt[nss], rx_cnt);
		WLCNTADD(ulstats->txsucc_nsscnt[nss], rxsucc_cnt);
		ulmu_scb->fail_cnt = 0;
	}

	if (rxsucc_cnt == 0) {
		WLCNTADD(ulstats->nbadfcs, 1);
	} else {
		/* skip rssi update if it is a pre-defined corrupted value */
		if (phyrssi != (ULMU_PHYRSSI_MAX << OPERAND_SHIFT)) {
			WLCNTADD(ulstats->nvldrssi, 1);
			if (ulstats->rssi_stats.min_rssi > phyrssi) {
				ulstats->rssi_stats.min_rssi = phyrssi;
			}

			if (ulstats->rssi_stats.max_rssi < phyrssi) {
				ulstats->rssi_stats.max_rssi = phyrssi;
			}
			WLCNTADD(ulstats->rssi_stats.avg_rssi, phyrssi);
			WLCNTADD(ulstats->txop, txop);
			WLCNTADD(ulstats->agglen, agglen);
			WLCNTADD(ulstats->qncnt, qncnt);
		}

		if (rxsucc_cnt == qncnt) {
			ulmu_scb->qnullonly_cnt++;
		} else {
			ulmu_scb->qnullonly_cnt = 0;
		}

		wlc_ulmu_stats_upd_queue_size(qos_field, ulstats);
	}

	if ((rxsucc_cnt != 0) && (rxsucc_cnt > qncnt))
		WLCNTADD(gstats->usrDataFrmhist[nusrs-1], 1);

	rspec = HE_RSPEC(mcs, nss+1);
	rspec |= HE_GI_TO_RSPEC(WL_RSPEC_HE_2x_LTF_GI_1_6us);
	ru_type = wf_he_ruidx_to_ru_type(TGTXS_RUIDX(TX_STATUS_MACTXS_S4(txs)));
	reason = TGTXS_REASON(TX_STATUS_MACTXS_S1(txs));

	payload_txtime = (TGTXS_LSIG(TX_STATUS_MACTXS_S2(txs)) * 8 / 6) -
		D11TRIGCI_NLTF(cmi01)*2*7 - 20 - 16;

	ASSERT(payload_txtime > 0);

	usec_txdur = payload_txtime >>
		(D11AX_BW2RUTYPE(wlc_scb_ratesel_get_ulmu_link_bw(ulmu->wlc, scb)) -  ru_type);

	ru_rate = wf_he_rspec_ru_type_to_rate(rspec, ru_type);

	triggered_bytes = (payload_txtime * ru_rate)/8000;

	WL_INFORM((""MACF" tgsts su_dur %d dur %d %dx%d qncnt %d agglen %d rx_cnt %d rxsucc_cnt %d"
		" reason 0x%x fid 0x%x ru %d bw %d kbps %d triggerd_bytes %d pend_trig_cnt %d\n",
		ETHER_TO_MACF(scb->ea),
		usec_txdur,
		payload_txtime, mcs, nss,
		qncnt, agglen, rx_cnt, rxsucc_cnt, reason,
		TGTXS_REMKB(TX_STATUS_MACTXS_ACK_MAP2(txs)) >> ULSTS_FRAMEID_SHIFT,
		ru_type, D11TRIGCI_BW(cmi01), ru_rate,
		triggered_bytes, ulmu_scb->pend_trig_cnt));

#ifdef ULMU_DRV
	if ((ulmu->mode == ULMU_UTXD_MODE) &&
		(ULMU_FLAGS_DSCHED_GET(ulmu->flags) == ULMU_ON)) {
		ulmu_drv_utxd_status_t status;

		status.tid = TAF_UL_PRIO;
		status.triggered_bytes = agglen;
		status.npkts = rxsucc_cnt;
		status.duration = usec_txdur;
		status.frameid =
			TGTXS_REMKB(TX_STATUS_MACTXS_ACK_MAP2(txs)) >> ULSTS_FRAMEID_SHIFT;
		status.ru_idx = TGTXS_RUIDX(TX_STATUS_MACTXS_S4(txs));
		status.mcs = mcs;
		status.nss = nss + 1; /* NSS in uCode is zero based */
		status.qn_consec_count = ulmu_scb->qnullonly_cnt;
		status.qn_total_count = qncnt;
		status.return_status = reason;

		/* Update the utxd stats and schedule the next UL trigger */
		wlc_ulmu_drv_process_utxd_status(ulmu, scb, &status);
	}
#if defined(WLTAF) && defined(BCMDBG)
	else {
		taf_rxpkt_stats_t stats;

		memset(&stats, 0, sizeof(stats));

		stats.total_qos_null_len = (DOT11_MAC_HDR_LEN + DOT11_FCS_LEN) * qncnt;
		stats.qos_nulls = qncnt;

		/*  qos null count */
		wlc_taf_rxpkt_status(ulmu->wlc->taf_handle, scb, TAF_UL_PRIO, 0, &stats,
			TAF_RXPKT_STATUS_UPDATE_QOS_NULL);
	}
#endif
#else
	if (reason) {
		WL_INFORM(("wl%d: %s: Stop trigger for "MACF" reason %x\n", ulmu->wlc->pub->unit,
			__FUNCTION__, ETHER_TO_MACF(scb->ea), reason));
		if (ULMU_FLAGS_DSCHED_GET(ulmu->flags) == ULMU_ON) {
#ifndef BYTE_INTERFACE
			wlc_ulmu_release_bytes(ulmu, scb, ULMU_DURSZ_DFLT);
#else
			wlc_ulmu_release_bytes(ulmu, scb, ULMU_BUFSZ_DFLT);
#endif
		}
	}

	if (ULMU_IS_UTXD(ulmu->mode) && ULMU_FLAGS_DSCHED_GET(ulmu->flags) == ULMU_ON) {
		if (TGTXS_REMKB(TX_STATUS_MACTXS_ACK_MAP2(txs)) < ULMU_BUFSZ_THRSH) {
			WL_ULO(("wl%d: %s:  "MACF" remaining %d \n", ulmu->wlc->pub->unit,
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				TGTXS_REMKB(TX_STATUS_MACTXS_ACK_MAP2(txs))));
#ifndef BYTE_INTERFACE
			wlc_ulmu_release_bytes(ulmu, scb, ULMU_DURSZ_DFLT);
#else
			wlc_ulmu_release_bytes(ulmu, scb, ULMU_BUFSZ_DFLT);
#endif
		}
	}
#endif /* ULMU_DRV */

	if (rxsucc_cnt && rxsucc_cnt != qncnt) {
		uint16 cur_aggn = rx_cnt - qncnt;
		cur_aggn = (cur_aggn << ULMU_NF_AGGN);
		if (ulmu_scb->aggnma != 0) {
			ULMU_MOVING_AVG(&ulmu_scb->aggnma, cur_aggn,
				ULMU_EMA_ALPHA);
		} else {
			ulmu_scb->aggnma = cur_aggn;
		}
	}

	if (rxsucc_cnt > qncnt) {
		cur_mlen = (agglen << ULMU_NF_AGGLEN) / (rxsucc_cnt - qncnt);
		if (ulmu_scb->mlenma != 0) {
			ULMU_MOVING_AVG(&ulmu_scb->mlenma, cur_mlen, ULMU_EMA_ALPHA);
		} else {
			ulmu_scb->mlenma = cur_mlen;
		}
	}

	ulmu_scb->tgs_exprecv_bytes = wf_he_rspec_ru_type_to_rate(rspec, ru_type);
	/* compute expected bytes in trigger status based on lsig
	 * i.e., exp_bytes = (phy_rate * ((lsig * 8) / 6)) / (1000 * 8)
	 * i.e., exp_bytes = (phy_rate * lsig / (1000 * 6), /1000 is to convert phyrate
	 * KBPS to BPS
	 */
	ulmu_scb->tgs_exprecv_bytes = (ulmu_scb->tgs_exprecv_bytes *
		TGTXS_LSIG(TX_STATUS_MACTXS_S2(txs))) / (1000 * 6);
	if (ulmu_scb->tgs_exprecv_bytes) {
		wlc_ulmu_scb_reqbytes_eval(ulmu, scb);
	}

#ifdef WLTAF
	/* update stats */
	WLCNTINCR(ULMU_TAF_SCB_STATS(ulmu_scb)->nupd);

	WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->bytes_trigd, triggered_bytes);
	WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->bytes_recvd, agglen);
	WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->usec_txdur, usec_txdur);

	if (ULMU_DRV_STOP_QOSNULL(TGTXS_REASON(TX_STATUS_MACTXS_S1(txs)))) {
		WLCNTINCR(ULMU_TAF_SCB_STATS(ulmu_scb)->nqosnull);
		WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->usec_qosnull, usec_txdur);
	}

	if (ULMU_DRV_STOP_TIMEOUT(TGTXS_REASON(TX_STATUS_MACTXS_S1(txs)))) {
		WLCNTINCR(ULMU_TAF_SCB_STATS(ulmu_scb)->ntimeout);
		WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->usec_timeout, usec_txdur);
	}
	if (ULMU_DRV_STOP_SUPPRESS(TGTXS_REASON(TX_STATUS_MACTXS_S1(txs)))) {
		WLCNTINCR(ULMU_TAF_SCB_STATS(ulmu_scb)->nsuppress);
		WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->usec_suppress, usec_txdur);
	}

#endif /* WLTAF */

	return BCME_OK;
} /* wlc_ulmu_stats_upd */

static void
wlc_ulmu_scb_state_upd(void *ctx, scb_state_upd_data_t *notif_data)
{
	wlc_ulmu_info_t *ulmu = (wlc_ulmu_info_t *) ctx;
	wlc_info_t *wlc = ulmu->wlc;
	scb_t *scb;
	wlc_bsscfg_t *bsscfg;
	uint8 oldstate;
	scb_ulmu_t *ulmu_scb;
	ASSERT(notif_data);

	scb = notif_data->scb;
	ASSERT(scb);
	oldstate = notif_data->oldstate;
	bsscfg = scb->bsscfg;

	BCM_REFERENCE(wlc);
	BCM_REFERENCE(oldstate);

	ulmu_scb = SCB_ULMU(ulmu, scb);

	if (D11REV_LT(wlc->pub->corerev, 129) ||
		!SCB_HE_CAP(scb) || !BSSCFG_AP(bsscfg) ||
#ifndef WL11AX
		/* force to return if WL11AX compilation flag is off */
		TRUE ||
#endif /* WL11AX */
		SCB_INTERNAL(scb) || !ulmu_scb || !HE_ULOMU_ENAB(wlc->pub)) {
		return;
	}

	if ((!WSEC_ENABLED(bsscfg->wsec) && !(oldstate & ASSOCIATED) &&
		SCB_AUTHENTICATED(scb) && SCB_ASSOCIATED(scb)) ||
		(WSEC_ENABLED(bsscfg->wsec) && SCB_AUTHENTICATED(scb) &&
		SCB_ASSOCIATED(scb) && SCB_AUTHORIZED(scb) && !(oldstate & AUTHORIZED))) {
		/* if always_admit then try admit else let the rx status trigger the admission */
		if (ulmu->always_admit &&
			wlc_ulmu_admit_ready(ulmu, scb)) {
			WL_ULO(("%s: admitting "MACF"\n", __FUNCTION__,
				ETHER_TO_MACF(scb->ea)));
		}
	} else if ((oldstate & ASSOCIATED) && !(SCB_ASSOCIATED(scb) && SCB_AUTHENTICATED(scb))) {
		// Associated -> disassoc or deauth
		wlc_ulmu_admit_client(ulmu, scb, EVICT_CLIENT);
		wlc_ulmu_oper_state_upd(ulmu, scb, ULMU_SCB_INIT);
	} else {
		/* pass */
	}
} /* wlc_ulmu_scb_state_upd */

/* link bw update callback */
void
wlc_ulmu_scb_bw_upd(wlc_info_t *wlc, scb_t *scb, uint8 bw)
{
	uint8 bw_idx;
	ulmu_stats_t *ulstats;
	scb_ulmu_t *ulmu_scb = SCB_ULMU(wlc->ulmu, scb);

	/* Map bw, type BW_xxx (1..4), for exampel BW_80MHZ to idx 0..3 */
	bw_idx = bw - 1;
	/* evict if maxn is 0 for this link bw */
	if (SCB_ULOFDMA_ADM(scb) && (wlc->ulmu->maxn[bw_idx] == 0)) {
		WL_ULO(("wl%d: %s: SCB "MACF" bw update, new bw : %d\n",
			wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea), bw));

		wlc_ulmu_admit_client(wlc->ulmu, scb, EVICT_CLIENT);
		if ((ulstats = ulmu_scb->scb_omu_stats) != NULL) {
			WLCNTINCR(ulstats->evict_hist[EVICT_REASON_BWUPD]);
		}
	}
}

static void
wlc_ulmu_set_ulpolicy(wlc_ulmu_info_t *ulmu, int16 policy)
{
	if (policy > ULMU_POLICY_MAX) {
		ulmu->policy = ULMU_POLICY_AUTO;
	} else {
		ulmu->policy = policy;
	}
}

static int
wlc_ulmu_get_ulpolicy(wlc_ulmu_info_t *ulmu)
{
	return ulmu->policy;
}

/**
 * A new remote node has entered the scene. To keep track of its ulmu state, a data structure is
 * allocated, initialized and added to the linked list of remode nodes.
 */
static int
wlc_ulmu_scb_init(void *ctx, scb_t *scb)
{
	wlc_ulmu_info_t *ulmu = ctx;
	wlc_info_t *wlc = ulmu->wlc;
	scb_ulmu_t **psh = SCB_ULMU_CUBBY(ulmu, scb);
	scb_ulmu_t *ulmu_scb;

	ASSERT(*psh == NULL);

	*psh = wlc_scb_sec_cubby_alloc(wlc, scb, wlc_ulmu_scb_secsz(ctx, scb));

	if (*psh != NULL) {
		ulmu_scb = SCB_ULMU(ulmu, scb);
		ASSERT(ulmu_scb == *psh);

		wlc_ulmu_scb_sched_init(ulmu, scb, ulmu_scb);
	}

	return BCME_OK;
} /* wlc_ulmu_scb_init */

static void
wlc_ulmu_scb_deinit(void *ctx, scb_t *scb)
{
	wlc_info_t *wlc;
	int i;
	scb_ulmu_t **psh;
	scb_ulmu_t *sh;
	uint16 max_ulofdma_usrs;
	wlc_ulmu_info_t *ulmu = ctx;
	ASSERT(ulmu);

	wlc = ulmu->wlc;
	psh = SCB_ULMU_CUBBY(ulmu, scb);
	sh = SCB_ULMU(ulmu, scb);
	max_ulofdma_usrs = wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA);

	/* Memory not allocated for scb, return */
	if (!sh) {
		return;
	}

	for (i = 0; i < max_ulofdma_usrs; i++) {
		if (ulmu->scb_list[i] == scb) {
			ASSERT(ulmu->num_usrs >= 1);
			ulmu->num_usrs--;
			ulmu->scb_list[i] = NULL;
		}
	}

	/* process any pending utxd status and regular tx status */
	wlc_bmac_service_txstatus(wlc->hw);

	wlc_ulmu_drv_scb_deinit(ulmu, scb);

	if (sh->scb_omu_stats != NULL) {
		MFREE(wlc->osh, sh->scb_omu_stats, sizeof(ulmu_stats_t));
		--ulmu->num_scb_ulstats;
		ASSERT(ulmu->num_scb_ulstats >= 0);
	}
	if (sh->scb_mmu_stats != NULL) {
		MFREE(wlc->osh, sh->scb_mmu_stats, sizeof(ulmu_stats_t));
		--ulmu->num_scb_ulstats;
		ASSERT(ulmu->num_scb_ulstats >= 0);
	}

	if (sh->ul_rmem != NULL) {
		MFREE(wlc->osh, sh->ul_rmem, sizeof(d11ulmu_rmem_t));
		sh->ul_rmem = NULL;
	}

	wlc_scb_sec_cubby_free(wlc, scb, sh);
	*psh = NULL;
} /* wlc_ulmu_scb_deinit */

static uint
wlc_ulmu_scb_secsz(void *ctx, scb_t *scb)
{
	wlc_ulmu_info_t *ulmu = ctx;
	wlc_info_t *wlc = ulmu->wlc;
	if (scb && !SCB_INTERNAL(scb) && HE_ULOMU_ENAB(wlc->pub)) {
		return sizeof(scb_ulmu_t);
	} else {
		return 0;
	}
}

#if defined(DUMP_SCB_ULMU)
static void
wlc_ulmu_scb_dump(void *ctx, scb_t *scb, bcmstrbuf_t *b)
{
	wlc_ulmu_info_t *ulmu = ctx;
	scb_ulmu_t *sh = SCB_ULMU(ulmu, scb);

	if (sh == NULL) {
		return;
	}

	bcm_bprintf(b, "     UL schpos %d\n", sh->schpos);
}
#endif /* DUMP_SCB_ULMU */

static int
wlc_ulmu_scb_stats_init(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	scb_ulmu_t *ulmu_scb;
	wlc_info_t *wlc;
	ulmu_scb = SCB_ULMU(ulmu, scb);

	wlc = ulmu->wlc;

	ASSERT(!SCB_INTERNAL(scb));
	ASSERT(ulmu_scb != NULL);

	if (ulmu_scb->scb_omu_stats != NULL) {
		/* already have stats, should we clear them? */
		ulmu_scb->scb_omu_stats->rssi_stats.min_rssi =
			PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
		ulmu_scb->scb_omu_stats->rssi_stats.max_rssi =
			-PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
	} else if ((ulmu_scb->scb_omu_stats =
		MALLOCZ(ulmu->wlc->osh, sizeof(ulmu_stats_t))) != NULL) {
		++ulmu->num_scb_ulstats;
		ulmu_scb->scb_omu_stats->rssi_stats.min_rssi =
			PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
		ulmu_scb->scb_omu_stats->rssi_stats.max_rssi =
			-PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
	} else {
		WL_ERROR(("wl%d: %s: Fail to get scb_omu_stats STA "MACF"\n",
			wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea)));
		return BCME_NOMEM;
	}
	if (HE_ULMMU_ENAB(wlc->pub)) {
		if (ulmu_scb->scb_mmu_stats != NULL) {
			/* already have stats, should we clear them? */
			ulmu_scb->scb_mmu_stats->rssi_stats.min_rssi =
				PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
			ulmu_scb->scb_mmu_stats->rssi_stats.max_rssi =
				-PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
		} else if ((ulmu_scb->scb_mmu_stats =
			MALLOCZ(ulmu->wlc->osh, sizeof(ulmu_stats_t))) != NULL) {
			++ulmu->num_scb_ulstats;
			ulmu_scb->scb_mmu_stats->rssi_stats.min_rssi =
				PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
			ulmu_scb->scb_mmu_stats->rssi_stats.max_rssi =
				-PHYRSSI_2SCOMPLEMENT << OPERAND_SHIFT;
		} else {
			WL_ERROR(("wl%d: %s: Fail to get scb_mmu_stats STA "MACF"\n",
				wlc->pub->unit, __FUNCTION__,
				ETHER_TO_MACF(scb->ea)));
			return BCME_NOMEM;
		}
	}

	return BCME_OK;
} /* wlc_ulmu_scb_stats_init */

static void
wlc_ulmu_scb_sched_init(wlc_ulmu_info_t *ulmu, scb_t *scb, scb_ulmu_t *ulmu_scb)
{
	ASSERT(!SCB_INTERNAL(scb));
	ASSERT(ulmu_scb != NULL);

	memset(ulmu_scb, 0, sizeof(*ulmu_scb));

	ulmu_scb->schpos = ULMU_SCHPOS_INVLD;
	ulmu_scb->trigcnt = ulmu->txd.txcnt;
	ulmu_scb->scb_bl = scb;	/* save the scb back link */
	ulmu_scb->ulmu = ulmu;	/* save the ulmu back link */
	ulmu_scb->acc_bufsz = ULMU_ACC_BUFSZ_MIN;
	ulmu_scb->aggnma = wlc_ampdu_rx_get_ba_max_rx_wsize(ulmu->wlc->ampdu_rx);
	ulmu_scb->mlenma = ULMU_MLEN_INIT;
	ulmu_scb->try_admit = FALSE;
}

/* Function to get empty slot in ul_usr_list
 * If there is empty slot return the index
 * Otherwise, return -1
 */
static int8
wlc_ulmu_ulofdma_get_empty_schpos(wlc_ulmu_info_t *ulmu)
{
	int8 schpos;
	wlc_info_t *wlc = ulmu->wlc;
	uint16 max_ulofdma_usrs = wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA);

	if (ulmu->num_usrs >= max_ulofdma_usrs) {
		return ULMU_SCHPOS_INVLD;
	}

	for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
		if (ulmu->scb_list[schpos] == NULL) {
			return schpos;
		}
	}

	return ULMU_SCHPOS_INVLD;
}

/* Function to find if a given scb in ul_usr_list
 * If yes, then return the schpos; Otherwise, return -1
 */
static int8
wlc_ulmu_scb_lkup(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	int8 i, schpos = ULMU_SCHPOS_INVLD;
	wlc_info_t *wlc = ulmu->wlc;

	for (i = 0; i < wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA); i++) {
		if (ulmu->scb_list[i] == scb) {
			schpos = i;
			break;
		}
	}

	return schpos;
}

/* Function to find if a given ether addr in ul_usr_list
 * If yes, then return the schpos and set scb pointer; Otherwise, return -1 and NULL scb pointer
 */
static int8
wlc_ulmu_scb_onaddr_lkup(wlc_ulmu_info_t *ulmu, struct ether_addr *ea,
	scb_t **pp_scb)
{
	int8 idx, schpos = ULMU_SCHPOS_INVLD;
	wlc_bsscfg_t *bsscfg;
	wlc_info_t *wlc = ulmu->wlc;
	char eabuf[ETHER_ADDR_STR_LEN];
	scb_t *scb = NULL;

	BCM_REFERENCE(eabuf);

	FOREACH_BSS(wlc, idx, bsscfg) {
		if ((scb = wlc_scbfind(wlc, bsscfg, ea))) {
			schpos = wlc_ulmu_scb_lkup(ulmu, scb);
			break;
		}
	}
	*pp_scb = scb;

	WL_ULO(("wl%d: %s: addr %s schpos %d scb %p\n",
		wlc->pub->unit, __FUNCTION__,
		scb ? bcm_ether_ntoa(&scb->ea, eabuf) : "null", schpos, scb));
	return schpos;
}

/** Add the new user to ul ofdma scheduler. Return TRUE if added; otherwise FALSE */
static bool
wlc_ulmu_ulofdma_add_usr(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	scb_ulmu_t *ulmu_scb;
	wlc_info_t *wlc = ulmu->wlc;
	bool ret = FALSE;
	d11ulmu_rmem_t *rmem;
	int i;
	uint8 nss;
	int8 schpos;
	ulmu_stats_t *ulstats;
#ifdef WLTAF
	uint32 taf_kbits_rate;
	taf_rxpkt_info_t pkt_info;
#endif

	STATIC_ASSERT((ULMU_RMEMIDX_FIRST >= AMT_IDX_RLM_RSVD_SIZE) &&
		(ULMU_RMEMIDX_FIRST < AMT_IDX_SIZE_11AX));
	STATIC_ASSERT((ULMU_RMEMIDX_LAST >= AMT_IDX_RLM_RSVD_SIZE) &&
		(ULMU_RMEMIDX_LAST < AMT_IDX_SIZE_11AX));
	STATIC_ASSERT(ULMU_RMEMIDX_FIRST <= ULMU_RMEMIDX_LAST);
	STATIC_ASSERT(ULMU_RMEMIDX_LAST < AMT_IDX_DLOFDMA_RSVD_START);

	if (ulmu->num_usrs >= wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA)) {
			WL_ULO(("wl%d: %s: num_usrs exceeded(%d) STA "MACF"\n",
			wlc->pub->unit, __FUNCTION__, ulmu->num_usrs,
			ETHER_TO_MACF(scb->ea)));
		return ret;
	}

	if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
		WL_ULO(("wl%d: %s: Fail to get ulmu scb cubby STA "MACF"\n",
			wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea)));
		return ret;
	}

	ASSERT(ulmu_scb->ul_rmem == NULL);
	schpos = wlc_ulmu_ulofdma_get_empty_schpos(ulmu);
	if (schpos == ULMU_SCHPOS_INVLD) {
		WL_ULO(("wl%d: %s: Invalid schpos STA "MACF"\n",
			wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea)));
		return ret;
	}

	if ((ulstats = ulmu_scb->scb_omu_stats) == NULL) {
		return ret;
	}

	if (!(rmem = MALLOCZ(wlc->osh, sizeof(d11ulmu_rmem_t)))) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->osh)));
		return ret;
	}

	if (SCB_UTXD_INFLT_VAL(scb) != 0) {
		WL_ERROR(("wl%d: %s: SCB "MACF" Admission fail, %d utxd pending\n",
			ulmu->wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea), SCB_UTXD_INFLT_VAL(scb)));
		MFREE(wlc->osh, rmem, sizeof(d11ulmu_rmem_t));
		return ret;
	}

	ulmu_scb->schpos = schpos;
	ulmu_scb->rmemidx = ULMU_RMEMIDX_FIRST + schpos;

	SCB_ULOFDMA_ENABLE(scb);

	nss = HE_MAX_SS_SUPPORTED(scb->rateset.he_bw80_tx_mcs_nss); /* 1-based NSS */

	/* populate rate block to ucfg: autorate by default */
	D11_ULOTXD_UCFG_SET_MCSNSS(ulmu_scb->ucfg, 0x17);
	/* init ucfg's nss to supported max NSS */
	D11_ULOTXD_UCFG_SET_NSS(ulmu_scb->ucfg, nss == 0 ? 0 : nss-1);

	D11_ULOTXD_UCFG_SET_TRSSI(ulmu_scb->ucfg, ULMU_OFDMA_TRSSI_INIT);

	/* check if need to override with global fixed settings */
	if ((ulmu->g_ucfg & D11_ULOTXD_UCFG_FIXRT_MASK)) {
		D11_ULOTXD_UCFG_SET_MCSNSS(ulmu_scb->ucfg,
			D11_ULOTXD_UCFG_GET_MCSNSS(ulmu->g_ucfg));
		ulmu_scb->ucfg |= D11_ULOTXD_UCFG_FIXRT_MASK;
	}

	if ((ulmu->g_ucfg & D11_ULOTXD_UCFG_FIXRSSI_MASK)) {
		D11_ULOTXD_UCFG_SET_TRSSI(ulmu_scb->ucfg,
			D11_ULOTXD_UCFG_GET_TRSSI(ulmu->g_ucfg));
		ulmu_scb->ucfg |= D11_ULOTXD_UCFG_FIXRSSI_MASK;
	}

	ulmu_scb->tagid = ulmu->g_tagid;

	if (ULMU_SCB_IS_ULMMU(ulmu_scb)) {
		SCB_ULMMU_ENABLE(scb);
	}

	/* TODO: now just use hardcoded bw80 mcsmap */
	for (i = 0; i < ARRAYSIZE(rmem->mcsbmp); i++) {
		rmem->mcsbmp[i] = HE_MAX_MCS_TO_MCS_MAP(
			(((scb->rateset.he_bw80_tx_mcs_nss >> (i*2)) & 0x3)));
	}
	D11_ULORMEM_RTCTL_SET_MCS(rmem->rtctl,
		HE_MAX_MCS_TO_INDEX(HE_MCS_MAP_TO_MAX_MCS(rmem->mcsbmp[0])));
	D11_ULORMEM_RTCTL_SET_NSS(rmem->rtctl, nss == 0 ? 0 : nss-1);
	WL_ERROR(("wl%d: %s: rtctl 0x%x mcsmap 0x%x sta addr "MACF"\n",
		wlc->pub->unit, __FUNCTION__,
		rmem->rtctl, scb->rateset.he_bw80_tx_mcs_nss,
		ETHER_TO_MACF(scb->ea)));
	rmem->aggnma = (64 << 4); // XXX: get the BAwin from linkentry
	rmem->mlenma[0] = 0;
	rmem->mlenma[1] = 0x30; // AMSDU byte size 3072 << 10

	ulmu->scb_list[schpos] = scb;
	ulmu_scb->ul_rmem = rmem;
	ulmu_scb->rmem_upd = TRUE;
	ulmu->num_usrs++;
	ret = TRUE;
	WLCNTINCR(ulstats->nadmit);

	if (ULMU_IS_UTXD(ulmu->mode)) {
		ret = wlc_ulmu_drv_ucautotrigger(ulmu, scb);
	}

	ulmu_scb->state = ULMU_SCB_ADMT;
#ifdef WLTAF
	/* the following will 'get' the Rx byte rate in Kbits/second */
	wlc_taf_rxpkt_status(ulmu->wlc->taf_handle, scb, TAF_UL_PRIO, 0,
		&taf_kbits_rate, TAF_RXPKT_STATUS_GET_BYTE_RATE);

	ulmu_scb->rate = taf_kbits_rate >> 3;
	ulmu_scb->rate = ulmu_scb->rate ? ulmu_scb->rate : 1; /* roundup to 1 if 0 */
	WL_TAFF(wlc, " SCB:"MACF" admitted: rate %d\n", ETHER_TO_MACF(scb->ea), ulmu_scb->rate);

	wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, 0, &pkt_info,
		TAF_RXPKT_STATUS_GET_PKT_INFO);
	ulmu_scb->avg_pkt_size = pkt_info.size.average ? pkt_info.size.average : 1024;
	ulmu_scb->peak_pkt_size = pkt_info.size.peak ? pkt_info.size.peak : ulmu_scb->avg_pkt_size;
	ulmu_scb->peak_aggn = pkt_info.aggn.peak;

	/* init the rx mon byte cnt to su recv byte rate x 50 ms  */
	ulmu_scb->acc_bufsz = ulmu_scb->rate * ulmu->qlen_init_time;
	ulmu_scb->acc_bufsz =
		LIMIT_TO_RANGE(ulmu_scb->acc_bufsz, ULMU_ACC_BUFSZ_MIN, ULMU_ACC_BUFSZ_MAX);

	ulmu_scb->rxmon_idle_cnt = 0;
	ulmu_scb->zero_bytes_cnt = 0;
	ulmu_scb->trig_wait_state = ULMU_TRIG_WAIT_OFF;
	ulmu_scb->trig_stall = FALSE;
	ulmu_scb->trig_wait_start_cnt = 0;
	ulmu_scb->trig_ts = wlc_read_usec_timer(ulmu->wlc);

	ulmu_scb->total_compl_utxd_dur = 0;

#else
	ulmu_scb->acc_bufsz = ulmu_scb->su_recv_bytes;
#endif /* WLTAF */
	wlc_ulmu_drv_add_usr(ulmu_scb);

#if defined WLTAF && defined ULMU_DRV
	if (wlc_taf_ul_enabled(wlc->taf_handle)) {
		WL_TAFF(ulmu->wlc, " SCB:"MACF" recv_bytes %d dur %d rxmon cnt %d\n",
			ETHER_TO_MACF(scb->ea), ulmu_scb->su_recv_bytes,
			ulmu_scb->su_recv_dur, ulmu_scb->acc_bufsz);

		wlc_ulmu_taf_enable(ulmu, scb, TRUE);
	}
#endif

	return ret;
} /* wlc_ulmu_ulofdma_add_usr */

/* Remove a given user from ul ofdma scheduler. Return TRUE if removed; otherwise FALSE */
bool
wlc_ulmu_del_usr(wlc_ulmu_info_t *ulmu, scb_t *scb, bool is_bss_up)
{
	wlc_info_t *wlc = ulmu->wlc;
	bool ret = FALSE;
	scb_ulmu_t *ulmu_scb;
	int8 schpos;

	if (SCB_INTERNAL(scb)) {
		return ret;
	}

	if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
		WL_ULO(("wl%d: %s: Fail to get ulmu scb cubby STA "MACF"\n",
			wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea)));
		return ret;
	}

	schpos = wlc_ulmu_scb_lkup(ulmu, scb);
	if (schpos == ULMU_SCHPOS_INVLD) {
		return ret;
	}

	ulmu_scb->state = ULMU_SCB_EVCT;
	if (is_bss_up && ULMU_IS_UTXD(ulmu->mode)) {
		if (wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRDEL, NULL, NULL) != BCME_OK) {
			return BCME_ERROR;
		}
		wlc_ulmu_post_utxd(ulmu);
	}
	SCB_ULMU_DISABLE(scb);
	ulmu_scb->schpos = ULMU_SCHPOS_INVLD;
	ulmu->scb_list[schpos] = NULL;
	ASSERT(ulmu->num_usrs >= 1);
	ulmu->num_usrs--;
	if (ulmu_scb->ul_rmem != NULL) {
		MFREE(wlc->osh, ulmu_scb->ul_rmem, sizeof(d11ulmu_rmem_t));
		ulmu_scb->ul_rmem = NULL;
	}
	WL_ERROR(("wl%d: %s: Remove sta "MACF" schpos %d state %x from ul ofdma list\n",
		wlc->pub->unit, __FUNCTION__,
		ETHER_TO_MACF(scb->ea), schpos, scb->state));
	ret = TRUE;

	/* clear the rx byte cnt  */
	ulmu_scb->acc_bufsz = 0;
	ulmu_scb->try_admit = FALSE;
#ifdef WLTAF
	ulmu_scb->rate = 0;
#endif

#if defined WLTAF && defined ULMU_DRV
	if (wlc_taf_ul_enabled(wlc->taf_handle)) {
		WL_TAFF(ulmu->wlc,
			"Evicting "MACF" trig_wait_start_cnt %d trig_wait_state %d "
			"zero_bytes_cnt %d rxmon_idle_cnt %d\n",
			ETHER_TO_MACF(scb->ea), ulmu_scb->trig_wait_start_cnt,
			ulmu_scb->trig_wait_state, ulmu_scb->zero_bytes_cnt,
			ulmu_scb->rxmon_idle_cnt);

		wlc_ulmu_taf_enable(ulmu, scb, FALSE);
	}
#endif /* defined WLTAF && defined ULMU_DRV */

	wlc_ulmu_drv_del_usr(ulmu->wlc, scb, ulmu_scb);
	return ret;
} /* wlc_ulmu_del_usr */

/* Determine if a STA is eligible to be admitted into ul ofdma list */
static bool
wlc_ulmu_scb_eligible(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	bool ret;
	wlc_info_t* wlc = ulmu->wlc;
	ret = (HE_ULOMU_ENAB(wlc->pub) && !SCB_IS_UNUSABLE(scb) && !SCB_INTERNAL(scb) &&
		SCB_HE_CAP(scb) && SCB_AUTHENTICATED(scb) && SCB_ASSOCIATED(scb) &&
		(SCB_IS_BRCM(scb) || (!ulmu->brcm_only)) &&
		(!ulmu->ban_nonbrcm_160sta || !wlc_he_is_nonbrcm_160sta(wlc->hei, scb)) &&
		(!WSEC_ENABLED(SCB_BSSCFG(scb)->wsec) ||
		(WSEC_ENABLED(SCB_BSSCFG(scb)->wsec) && SCB_AUTHORIZED(scb))) &&
		BSSCFG_AP(SCB_BSSCFG(scb)) &&
		wlc_he_get_ulmu_allow(wlc->hei, scb) &&
		(ulmu->always_admit != ULMU_ADMIT_TWT_ONLY ||
		wlc_twt_scb_is_trig_enab(wlc->twti, scb)) &&
		((wlc_he_get_omi_tx_nsts(wlc->hei, scb) <= MUMAX_NSTS_ALLOWED) ||
		ulmu->always_admit == ULMU_ADMIT_ALLNSTS) &&
		(wlc->ulmu->maxn[wlc_scb_ratesel_get_link_bw(wlc, scb) - 1]));
	if (ret) {
		WL_INFORM(("wl%d: %s: ul ofdma capable STA "MACF" rssi %d "
			"state 0x%x txnss %d ulmu_disabled %d\n",
			wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea),
			wlc_lq_rssi_get(wlc, SCB_BSSCFG(scb), scb), scb->state,
			wlc_he_get_omi_tx_nsts(wlc->hei, scb),
			!wlc_he_get_ulmu_allow(wlc->hei, scb)));
	}

	return ret;
} /* wlc_ulmu_scb_eligible */

/**
 * Function to try to add or delete a usr from ULOFDMA usr list
 * If a user is successfully added or deleted, return TRUE
 * otherwise FALSE
 *
 * @param[in] scb  The respective user
 * @param admit    EVICT_CLIENT(FALSE) or ADMIT_CLIENT(TRUE)
 */
static bool
wlc_scbulmu_set_ulofdma(wlc_ulmu_info_t *ulmu, scb_t *scb, bool admit)
{
	scb_ulmu_t *ulmu_scb;
	wlc_info_t *wlc;
	bool ret = FALSE;

	wlc = ulmu->wlc;
	BCM_REFERENCE(wlc);

	if (SCB_INTERNAL(scb)) {
		return ret;
	}

	if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
		WL_ULO(("wl%d: %s: Fail to get ulmu scb cubby STA "MACF"\n",
			wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return ret;
	}

	if (wlc_ulmu_scb_is_ulofdma(ulmu, scb) == admit) {
		return ret;
	}

	if (admit) {
		int local_ret = wlc_ulmu_scb_stats_init(ulmu, scb);
		if (local_ret == BCME_OK) {
			local_ret = wlc_ulmu_drv_scb_init(ulmu, scb);
		}

		ulmu_scb->lmemidx = wlc_ratelinkmem_get_scb_link_index(wlc, scb);
		if ((local_ret == BCME_OK) && wlc_ulmu_scb_eligible(ulmu, scb)) {
			ret = wlc_ulmu_ulofdma_add_usr(ulmu, scb);
		} else if (SCB_HE_CAP(scb) && HE_ULOMU_ENAB(wlc->pub) &&
			BSSCFG_AP(SCB_BSSCFG(scb))) {
			WL_ULO(("wl%d: %s: Fail to enable ul ofdma STA "MACF" "
				"max clients %d num clients %d txnss %d ulmu_disabled %d\n",
				wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea),
				wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA),
				ulmu->num_usrs, wlc_he_get_omi_tx_nsts(wlc->hei, scb),
				!wlc_he_get_ulmu_allow(wlc->hei, scb)));

			SCB_ULMU_DISABLE(scb);
			ulmu_scb->schpos = ULMU_SCHPOS_INVLD;
			ulmu_scb->state = ULMU_SCB_EVCT;
#if defined WLTAF && defined ULMU_DRV
			if (wlc_taf_ul_enabled(wlc->taf_handle)) {
				wlc_ulmu_taf_enable(ulmu, scb, ulmu_scb->state == ULMU_SCB_ADMT);
			}
#endif /* defined WLTAF && defined ULMU_DRV */
		} else {
			/* pass */
		}
	} else {
		ret = wlc_ulmu_del_usr(ulmu, scb, TRUE);
		WL_ULO(("wl%d: %s: Disable ul ofdma STA "MACF" state 0x%x ret:%d\n",
			wlc->pub->unit, __FUNCTION__, ETHER_TO_MACF(scb->ea), scb->state, ret));
	}

	return ret;
} /* wlc_scbulmu_set_ulofdma */

uint16
wlc_ulmu_scb_get_rmemidx(wlc_info_t *wlc, scb_t *scb)
{
	scb_ulmu_t* ulmu_scb;

	if (scb == NULL || wlc->ulmu == NULL ||
		((ulmu_scb = SCB_ULMU(wlc->ulmu, scb)) == NULL)) {
		WL_ERROR(("wl%d: %s: Fail to get ulmu scb cubby STA "MACF"\n",
			wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea)));
		return D11_RATE_LINK_MEM_IDX_INVALID;
	}

	return ((ulmu_scb->schpos == ULMU_SCHPOS_INVLD) ?
		D11_RATE_LINK_MEM_IDX_INVALID :	ulmu_scb->rmemidx);
}

static void
wlc_ulmu_csreq_commit(wlc_ulmu_info_t* ulmu)
{
	wlc_info_t *wlc = ulmu->wlc;
	wlc_write_shm(wlc, M_HETB_CSTHRSH_LO(wlc), ulmu->csthr0);
	wlc_write_shm(wlc, M_HETB_CSTHRSH_HI(wlc), ulmu->csthr1);
}

static int
wlc_ulmu_utxd_reinit(wlc_ulmu_info_t* ulmu)
{
	int schpos, err, num_usrs = 0;
	wlc_info_t *wlc;
	scb_t *scb;
	uint16 max_ulofdma_usrs;

	err = BCME_OK;

	if (!ulmu) {
		return err;
	}
	wlc = ulmu->wlc;
	max_ulofdma_usrs = wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA);

	if (!HE_ULOMU_ENAB(wlc->pub) || !ULMU_IS_UTXD(ulmu->mode)) {
		return err;
	}

	for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
		if ((scb = ulmu->scb_list[schpos]) == NULL) {
			continue;
		}
		num_usrs++;
		if (wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRADD_ALL, NULL, NULL) != BCME_OK) {
			return BCME_ERROR;
		}
	}

	if (num_usrs) {
		if (wlc_ulmu_prep_utxd(ulmu, NULL, ULMU_UTXD_GLBUPD, NULL, NULL) != BCME_OK) {
			return BCME_ERROR;
		}
		if ((err = wlc_ulmu_post_utxd(ulmu)) != BCME_OK) {
			err = BCME_ERROR;
			WL_ERROR(("wl%d: %s: fail to post global config utxd\n",
				wlc->pub->unit, __FUNCTION__));
		}
	}

	return err;
} /* wlc_ulmu_utxd_reinit */

static void
wlc_ulmu_bsscfg_state_upd(void *ctx, bsscfg_state_upd_data_t *notif_data)
{
	wlc_ulmu_info_t *ulmu = (wlc_ulmu_info_t *) ctx;
	wlc_bsscfg_t *cfg = notif_data->cfg;

	if (!BSSCFG_AP(cfg)) {
		return;
	}

	if (notif_data->old_up && !cfg->up) {
		/* up -> down */
		wlc_info_t *wlc = ulmu->wlc;
		scb_iter_t scbiter;
		scb_t *scb;

		/* remove scb of this bsscfg from ulmu list to prevent utxd scheduling */
		FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
			wlc_ulmu_del_usr(ulmu, scb, TRUE);
		}

#if defined(BCMHWA)
		/* Stop pageout ring before reclaim all utxd */
		HWA_PKTPGR_EXPR(hwa_txfifo_disable_prep(WL_HWA_DEVP(wlc), 0));
#endif /* BCMHWA */
		/* reclaim utxd to update inflight fifo count */
		wlc_ulmu_reclaim_utxd(wlc, NULL);
#if defined(BCMHWA)
		/* Resume pageout ring to posting tx packets after reclaim is done */
		HWA_PKTPGR_EXPR(hwa_txfifo_enable(WL_HWA_DEVP(wlc), 0, TRUE));
#endif /* BCMHWA */

	} else if (notif_data->old_up && cfg->up) {
		/* reinit */
		wlc_ulmu_utxd_reinit(ulmu);
	}
}

/* Function to commit ulofdma scheduler changes
 * 1. populate ul ofdma trig txcfg block
 * 2. update the ratemem block for the newly added user
 */
static void
wlc_ulmu_cfg_commit(wlc_ulmu_info_t* ulmu)
{
	int schpos, idx;
	wlc_info_t *wlc = ulmu->wlc;
	d11ulo_trig_txcfg_t *txd = &ulmu->txd;
	scb_t *scb;
	uint16 lmem_idx;
	uint16 rmem_idx;
	uint16 *ptr;
	uint offset;
	scb_ulmu_t* ulmu_scb;
	uint16 max_ulofdma_usrs = wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA);

	BCM_REFERENCE(ptr);
	BCM_REFERENCE(offset);
	BCM_REFERENCE(ulmu_scb);

	if (!RATELINKMEM_ENAB(wlc->pub)) {
		WL_ERROR(("wl%d: %s: Fail to fill up ul ofdma txd. ratelinkmem_enab %x\n",
			wlc->pub->unit, __FUNCTION__, RATELINKMEM_ENAB(wlc->pub)));
		return;
	}

	for (schpos = 0, idx = 0; schpos < max_ulofdma_usrs; schpos++) {
		if ((scb = ulmu->scb_list[schpos]) == NULL) {
			continue;
		}
		if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
			continue;
		}
		lmem_idx = ulmu_scb->lmemidx;
		rmem_idx = wlc_ulmu_scb_get_rmemidx(wlc, scb);
		if ((lmem_idx == D11_RATE_LINK_MEM_IDX_INVALID) ||
			(rmem_idx == D11_RATE_LINK_MEM_IDX_INVALID)) {
			WL_ERROR(("wl%d: %s: Fail to fill up ul ofdma txd. schpos %d addr "MACF"\n",
				wlc->pub->unit, __FUNCTION__, schpos,
				ETHER_TO_MACF(scb->ea)));
			ASSERT(0);
			return;
		}
		txd->rlmem[idx] = ((rmem_idx << 8) | lmem_idx);
		txd->ucfg[idx] = ulmu_scb->ucfg;
		idx++;
	}

	ASSERT(idx == ulmu->num_usrs);

	for (; idx < D11_ULOFDMA_MAX_NUSERS; idx++) {
		txd->rlmem[idx] = 0;
	}

	/* Don't enable UL OFDMA if the number of users is less than minimum */
	if (ulmu->num_usrs < ulmu->min_ulofdma_usrs) {
		txd->nvld = 0;
	} else {
		txd->nvld = ulmu->num_usrs;
	}

	/* set init bit */
	txd->nvld |= ((ulmu->num_usrs) && ulmu->is_start) ? (1 << 15) : 0;

	/* set min users to start ul ofdma */
	txd->minn = ulmu->min_ulofdma_usrs;

#if defined(WL_PSMX)
	/* suspend psmx */
	wlc_bmac_suspend_macx_and_wait(wlc->hw);

	if (ulmu->mode == ULMU_LEGACY_MODE) {
		/* update rate mem for the usr got changed */
		for (schpos = 0; schpos < max_ulofdma_usrs; schpos++) {
			if ((scb = ulmu->scb_list[schpos]) != NULL) {
				ulmu_scb = SCB_ULMU(ulmu, scb);
				if (!ulmu_scb || !ulmu_scb->rmem_upd) {
					continue;
				}
				wlc_ratelinkmem_write_rucfg(wlc,
					(uint8*)ulmu_scb->ul_rmem,
					sizeof(d11ulmu_rmem_t),
					ulmu_scb->rmemidx);
				ulmu_scb->rmem_upd = FALSE;
			}
		}

		/* copy the info to MX_TRIG_TXCFG block */
		for (offset = MX_TRIG_TXCFG(wlc), ptr = (uint16 *) &ulmu->txd;
			offset < MX_TRIG_TXCFG(wlc) + sizeof(d11ulo_trig_txcfg_t);
			offset += 2, ptr++) {
			wlc_write_shmx(wlc, offset, *ptr);
		}
	} else {
		/* selectively copy the info to MX_TRIG_TXCFG block */
		for (offset = MX_TRIG_TXCFG(wlc), ptr = (uint16 *) &ulmu->txd;
			offset <= MX_TRIG_TXCFG(wlc) + OFFSETOF(d11ulo_trig_txcfg_t, aggnum);
			offset += 2, ptr++) {
			wlc_write_shmx(wlc, offset, *ptr);
		}
		for (offset = MX_TRIG_TXCFG(wlc) + OFFSETOF(d11ulo_trig_txcfg_t, ucfg),
			ptr = (uint16 *) ((uint8 *) &ulmu->txd +
				OFFSETOF(d11ulo_trig_txcfg_t, ucfg));
			offset < MX_TRIG_TXCFG(wlc) + sizeof(d11ulo_trig_txcfg_t);
			offset += 2, ptr++) {
			wlc_write_shmx(wlc, offset, *ptr);
		}
		offset = MX_TRIG_TXCFG(wlc) + OFFSETOF(d11ulo_trig_txcfg_t, mctl0);
		ptr = (uint16 *) ((uint8 *)&ulmu->txd+OFFSETOF(d11ulo_trig_txcfg_t, mctl0));
		wlc_write_shmx(wlc, offset, *ptr);

		offset = MX_TRIG_TXCFG(wlc) + OFFSETOF(d11ulo_trig_txcfg_t, chanspec);
		ptr = (uint16 *) ((uint8 *)&ulmu->txd+OFFSETOF(d11ulo_trig_txcfg_t, chanspec));
		wlc_write_shmx(wlc, offset, *ptr);

		offset = MX_TRIG_TXCFG(wlc) + OFFSETOF(d11ulo_trig_txcfg_t, minn);
		ptr = (uint16 *) ((uint8 *)&ulmu->txd+OFFSETOF(d11ulo_trig_txcfg_t, minn));
		wlc_write_shmx(wlc, offset, *ptr);

		wlc_write_shmx(wlc, MX_ULO_QNULLTHRSH(wlc), ulmu->qnull_thrsh);

		wlc_write_shmx(wlc, MX_ULC_NUM(wlc), ulmu->ulc_sz);
	}

	/* write trigger txlmt value */
	wlc_write_shmx(wlc, MX_TRIG_TXLMT(wlc), ulmu->txlmt);

	wlc_bmac_enable_macx(wlc->hw);
#endif /* defined(WL_PSMX) */
} /* wlc_ulmu_cfg_commit */

/**
 * admit / evict a ul-ofdma user
 *
 * Called from admission control component / omi transition
 *
 * @param wlc		handle to wlc_info context
 * @param scb		the client/user to admit or evict
 * @param admit		true to admit, false to evict
 * @return		true if successful, false otherwise
 */
static bool
wlc_ulmu_admit_client(wlc_ulmu_info_t *ulmu, scb_t *scb, bool admit)
{
	bool ret;

	if ((ret = wlc_scbulmu_set_ulofdma(ulmu, scb, admit)) == TRUE) {
		/* if a user has been added or deleted, update the scheduler block */
		if (ULMU_IS_UTXD(ulmu->mode)) {
			if (wlc_ulmu_prep_utxd(ulmu, NULL, ULMU_UTXD_GLBUPD,
				NULL, NULL) != BCME_OK) {
				return BCME_ERROR;
			}
			wlc_ulmu_post_utxd(ulmu);
		}
		wlc_ulmu_cfg_commit(ulmu);
	} else {
		/* no change on ul ofdma user list, pass */
	}

	return ret;
}

static bool
wlc_ulmu_scb_is_ulofdma(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	if (ulmu->policy == ULMU_POLICY_DISABLE ||
		SCB_INTERNAL(scb)) {
		return FALSE;
	}

	return (SCB_ULOFDMA_ADM(scb));
}

void
wlc_ulmu_ul_rspec_upd(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	scb_ulmu_t *ulmu_scb = NULL;

	if (ulmu && !HE_ULOMU_ENAB(ulmu->wlc->pub)) {
		WL_ERROR(("%s: ULMU Feature NOT enabled (%d)\n",
			__FUNCTION__, HE_ULOMU_ENAB(ulmu->wlc->pub)));
		return;
	}

	if (scb == NULL || (ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
		WL_ERROR(("%s: SCB or ulmu_scb is NULL(%p/%p)\n", __FUNCTION__, scb, ulmu_scb));
		return;
	}
	ASSERT(ulmu_scb);
	if (ulmu_scb->state != ULMU_SCB_ADMT) {
		WL_ERROR(("%s: User "MACF" is not admitted\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return;
	}

	if (ULMU_IS_UTXD(ulmu->mode)) {
		if (wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRUPD_RATE, NULL, NULL) != BCME_OK) {
			WL_ERROR(("%s: Prepare UTXD failed\n", __FUNCTION__));
			return;
		}
		wlc_ulmu_post_utxd(ulmu);
	} else {
		wlc_ulmu_cfg_commit(ulmu);
	}
}

bool
wlc_ulmu_is_ulrt_fixed(wlc_ulmu_info_t *ulmu)
{
	return (ulmu->g_ucfg & D11_ULOTXD_UCFG_FIXRT_MASK) ? TRUE : FALSE;
}

#ifdef WL_ULRT_DRVR
bool
wlc_ulmu_is_drv_rtctl_en(wlc_ulmu_info_t *ulmu)
{
	return ULMU_DRVR_GET_RT(ulmu->drv_ulrtctl);
}

bool
wlc_ulmu_is_drv_spctl_en(wlc_ulmu_info_t *ulmu)
{
	return ULMU_DRVR_GET_SP(ulmu->drv_ulrtctl);
}
#endif

void
wlc_ulmu_ul_nss_upd(wlc_ulmu_info_t *ulmu, scb_t *scb, uint8 tx_nss)
{
	scb_ulmu_t* ulmu_scb;

	if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
		return;
	}

	ulmu_scb->rmem_upd = TRUE;
	D11_ULORMEM_RTCTL_SET_INIT(ulmu_scb->ul_rmem->rtctl, 0);
	D11_ULORMEM_RTCTL_SET_NSS(ulmu_scb->ul_rmem->rtctl, tx_nss);
	D11_ULOTXD_UCFG_SET_NSS(ulmu_scb->ucfg, tx_nss);

	if (ULMU_IS_UTXD(ulmu->mode)) {
		if (wlc_ulmu_prep_utxd(ulmu, scb, ULMU_UTXD_USRUPD_RATE, NULL, NULL) != BCME_OK) {
			return;
		}
		wlc_ulmu_post_utxd(ulmu);
	} else {
		wlc_ulmu_cfg_commit(ulmu);
	}
}

static void
wlc_ulmu_validate_rspec(wlc_ulmu_info_t *ulmu, scb_t *scb, d11ulmu_txd_t *utxd,
	ratespec_t *rspec, uint8 *mcs, uint8 *nss)
{
	wlc_info_t *wlc = ulmu->wlc;
	uint8 omi_nss;

	if (*rspec == ULMU_RSPEC_INVD) {
		*mcs = HE_MAX_MCS_TO_INDEX(HE_MCS_MAP_TO_MAX_MCS(utxd->mcsbmp[0]));
		*nss = HE_MAX_SS_SUPPORTED(scb->rateset.he_bw80_tx_mcs_nss);
		WL_INFORM(("%s: invalid rspec [%x] replace with [%x]\n", __FUNCTION__,
			*rspec, HE_RSPEC(*mcs, *nss)));
		*rspec = HE_RSPEC(*mcs, *nss);
	} else {
		omi_nss = wlc_he_get_omi_tx_nsts(wlc->hei, scb); /* 1-based NSS */
		omi_nss = MIN(omi_nss, MAX_STREAMS_SUPPORTED);
		*mcs = RSPEC_HE_MCS(*rspec);
		*nss = RSPEC_HE_NSS(*rspec); /* 1-based NSS */

		if (*nss <= 0 || *nss > omi_nss) {
			WL_ERROR(("Invalid NSS %d, rspec %x; override NSS %d\n",
				*nss, *rspec, omi_nss));
			*nss = omi_nss;
		}
		if (!ULMU_DRVR_GET_SP(ulmu->drv_ulrtctl) && (*nss != omi_nss)) {
			WL_ULO(("wl%d: %s: txnss %d omi txnss %d mismatch\n",
				wlc->pub->unit, __FUNCTION__, *nss, omi_nss));
			*nss = omi_nss;
		}
	}
}

static void
wlc_ulmu_fill_rt(wlc_ulmu_info_t *ulmu, scb_t *scb, d11ulmu_txd_t *utxd,
	ratespec_t *rspec, uint8 *mcs, uint8 *nss)
{
	wlc_info_t *wlc = ulmu->wlc;
	ratespec_t rt_rspec;
	uint8 rt_mcs, rt_nss, epoch;
	uint i;
	uint16 is_sp = 0;

	utxd->rtctl = 0;
	rt_rspec = wlc_scb_ratesel_get_ulrt_txr(wlc->wrsi, scb, 0);
	epoch = D11_ULORMEM_RTCTL_GET_EPOCH(rt_rspec);
	wlc_ulmu_validate_rspec(ulmu, scb, utxd, &rt_rspec, &rt_mcs, &rt_nss);

	*mcs = rt_mcs;
	*nss = rt_nss;
	*rspec = rt_rspec;
	rt_nss = (rt_nss == 0) ? 0 : rt_nss-1;
	D11_ULORMEM_RTCTL_SET_NSS(utxd->rtctl, rt_nss);
	D11_ULORMEM_RTCTL_SET_MCS(utxd->rtctl, rt_mcs);

	BCM_REFERENCE(i);
	BCM_REFERENCE(is_sp);
#ifdef WL_ULRT_DRVR
	if (!wlc_ulmu_is_drv_rtctl_en(ulmu))
		return;

	/* driver ratesel need to set the epoch */
	D11_ULORMEM_RTCTL_SET_EPOCH(utxd->rtctl, epoch);

	D11_ULORMEM_ORT_SET_MCS(utxd->rt0, rt_mcs);
	D11_ULORMEM_ORT_SET_NSS(utxd->rt0, rt_nss);
	is_sp = wlc_scb_ratesel_is_start_sp(wlc->wrsi, scb);
	D11_ULORMEM_ORT_SET_SP(utxd->rt0, is_sp);

	/* set to disable ucode rate control */
	D11_ULORMEM_RTCTL_SET_URTOFF(utxd->rtctl, 1);

	for (i = 1; i < RATESEL_MFBR_NUM; i++) {
		rt_rspec = wlc_scb_ratesel_get_ulrt_txr(wlc->wrsi, scb, i);
		wlc_ulmu_validate_rspec(ulmu, scb, utxd, &rt_rspec, &rt_mcs, &rt_nss);
		switch (i) {
			case 1:
				D11_ULORMEM_ORT_SET_MCS(utxd->rt1, rt_mcs);
				D11_ULORMEM_ORT_SET_NSS(utxd->rt1, (rt_nss == 0) ? 0 : rt_nss-1);
				break;
			case 2:
				D11_ULORMEM_ORT_SET_MCS(utxd->rt2, rt_mcs);
				D11_ULORMEM_ORT_SET_NSS(utxd->rt2, (rt_nss == 0) ? 0 : rt_nss-1);
				break;
			case 3:
				D11_ULORMEM_ORT_SET_MCS(utxd->rt3, rt_mcs);
				D11_ULORMEM_ORT_SET_NSS(utxd->rt3, (rt_nss == 0) ? 0 : rt_nss-1);
				break;
			default:
				ASSERT(0);
		}
	}
	/* set the last bit */
	if (utxd->rt0 == utxd->rt1)
		D11_ULORMEM_ORT_SET_LAST(utxd->rt0, 1);
	else if (utxd->rt1 == utxd->rt2)
		D11_ULORMEM_ORT_SET_LAST(utxd->rt1, 1);
	else if (utxd->rt2 == utxd->rt3)
		D11_ULORMEM_ORT_SET_LAST(utxd->rt2, 1);
	else
		D11_ULORMEM_ORT_SET_LAST(utxd->rt3, 1);

	WL_ULO(("%s:%s utxd-rt[0..3] last-bit [%d %d %d %d] "
		"nss:mcs [%02x %02x %02x %02x] epoch %d\n",
		__FUNCTION__, (is_sp ? "(SP)" : ""),
		D11_ULORMEM_ORT_GET_LAST(utxd->rt0),
		D11_ULORMEM_ORT_GET_LAST(utxd->rt1),
		D11_ULORMEM_ORT_GET_LAST(utxd->rt2),
		D11_ULORMEM_ORT_GET_LAST(utxd->rt3),
		(utxd->rt0 & D11_ULORMEM_ORT_HE_MASK),
		(utxd->rt1 & D11_ULORMEM_ORT_HE_MASK),
		(utxd->rt2 & D11_ULORMEM_ORT_HE_MASK),
		(utxd->rt3 & D11_ULORMEM_ORT_HE_MASK), epoch));
#endif /* WL_ULRT_DRVR */
}

static void
wlc_ulmu_fill_utxd(wlc_ulmu_info_t *ulmu, d11ulmu_txd_t *utxd, scb_t *scb)
{
	wlc_info_t *wlc = ulmu->wlc;
	uint8 mcs = 0, nss = 0;
	scb_ulmu_t* ulmu_scb;
	int i;
	uint16 aggnum = 0, mlen = 0, ucfg, trssi;
	ratespec_t rspec = 0;
	bool taf_ul_enabled = wlc_taf_ul_enabled(wlc->taf_handle);

	BCM_REFERENCE(mlen);
	BCM_REFERENCE(aggnum);

	if (!RATELINKMEM_ENAB(wlc->pub)) {
		WL_ERROR(("wl%d: %s: Fail to fill up ul ofdma utxd. ratelinkmem_enab %x\n",
			wlc->pub->unit, __FUNCTION__, RATELINKMEM_ENAB(wlc->pub)));
		return;
	}

	if (!scb) {
		return;
	}

	if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
		return;
	}

	utxd->glmem = ulmu_scb->lmemidx;
	if (ULMU_FLAGS_AUTOULC_GET(ulmu->flags) == ULMU_ON) {
		utxd->urmem = (uint16) -1;
	} else {
		utxd->urmem = wlc_ulmu_scb_get_rmemidx(wlc, scb);
	}
	utxd->rel_cnt = ulmu_scb->rel_cnt;

	utxd->trigcnt = ulmu_scb->trigcnt;
	ucfg = ulmu_scb->ucfg;

	/* TODO: now just use hardcoded bw80 mcsmap */
	for (i = 0; i < ARRAYSIZE(utxd->mcsbmp); i++) {
		utxd->mcsbmp[i] = HE_MAX_MCS_TO_MCS_MAP(
			(((scb->rateset.he_bw80_tx_mcs_nss >> (i*2)) & 0x3)));
	}
	wlc_ulmu_fill_rt(ulmu, scb, utxd, &rspec, &mcs, &nss);
	trssi = wlc_scb_ratesel_get_ulrt_trssi(wlc->wrsi, scb);
	D11_ULOTXD_UCFG_SET_TRSSI(ucfg, trssi);

	if (ulmu_scb->state == ULMU_SCB_EVCT) {
		aggnum = wlc_ampdu_rx_get_ba_max_rx_wsize(wlc->ampdu_rx);
		mlen = ULMU_MLEN_INIT;
	} else if (ulmu_scb->state == ULMU_SCB_ADMT || ulmu_scb->state == ULMU_SCB_INIT) {
		aggnum = MIN((ulmu_scb->aggnma >> ULMU_NF_AGGN) + ULMU_AGGN_HEADROOM,
			wlc_ampdu_rx_get_ba_max_rx_wsize(wlc->ampdu_rx));
		mlen = ulmu_scb->mlenma;
	} else {
		ASSERT(0);
	}
	ulmu_scb->ucfg = ucfg;
	/* 160MHz STAs that can't do RU68, (eg 4389) remain as UL OFDMA */
	if ((wlc_scb_ratesel_get_link_bw(wlc, scb) == BW_160MHZ) &&
		!SCB_HE_UL2x996_CAP(wlc->hei, scb)) {
		ulmu_scb->tagid = 0;
		utxd->tagid = 0;
	}
	if (ulmu_scb->ucfg & D11_ULOTXD_UCFG_FIXTAGID_MASK) {
		utxd->tagid = ulmu_scb->tagid;
	}

	if (taf_ul_enabled) {
		if (ulmu_scb->aggn) {
			utxd->aggn = ulmu_scb->aggn;
		} else {
			uint32 max_wsize =
				wlc_ampdu_rx_get_scb_ba_rx_wsize(wlc->ampdu_rx, scb, ALLPRIO);
			utxd->aggn = MIN(ulmu_scb->peak_aggn + ulmu->headroom_aggn, max_wsize);
		}
	} else {
		utxd->aggn = aggnum;
	}

	if (taf_ul_enabled) {
		if (ulmu_scb->mlen) {
			utxd->mlen = ulmu_scb->mlen;
			utxd->peak_mlen = ulmu_scb->mlen;
		} else {
			int32 diff = ulmu_scb->peak_pkt_size - ulmu_scb->avg_pkt_size;

			utxd->peak_mlen = ulmu_scb->peak_pkt_size;

			if (diff < 0) {
				/* this is wrong, likely average is unreliable so use peak */
				utxd->mlen = ulmu_scb->peak_pkt_size;

				WL_INFORM(("wl%d: %s: avg pkt size greater than peak (%u/%u)\n",
					wlc->pub->unit, __FUNCTION__,
					ulmu_scb->avg_pkt_size, ulmu_scb->peak_pkt_size));

			} else if (diff < (ulmu_scb->peak_pkt_size >> 3)) {
				/* avg and peak are close (within ~12%), so use peak */
				utxd->mlen = ulmu_scb->peak_pkt_size;

			} else {
				utxd->mlen = ulmu_scb->avg_pkt_size;
			}
		}
	} else {
		utxd->mlen = mlen;
		utxd->peak_mlen = mlen;
	}

	utxd->ucfg = ucfg;

	WL_ULO(("wl%d: %s: lmem %d cmd 0x%x fid 0x%x rspec 0x%x mcs %d nss %d\n"
		"aggn %d mlen %d us %d ucfg 0x%x rssi %d state %d rtctl 0x%x tagid %d\n",
		wlc->pub->unit, __FUNCTION__, utxd->glmem, utxd->utxdcmd,
		utxd->frameid >> ULSTS_FRAMEID_SHIFT,
		rspec, mcs, nss, utxd->aggn, utxd->mlen, utxd->rel_cnt, ucfg,
		wlc_lq_rssi_get(wlc, SCB_BSSCFG(scb), scb),
		ulmu_scb->state, utxd->rtctl, utxd->tagid));
} /* wlc_ulmu_fill_utxd */

static int
wlc_ulmu_prep_utxd(wlc_ulmu_info_t *ulmu, scb_t *scb, uint16 cmd, void *pkt, uint32 *fid)
{
	wlc_info_t *wlc = ulmu->wlc;
	void *p;
	osl_t *osh;
	d11ulmu_txd_t *utxd;
	uint32 ts;
	int pad_sz = 4;

	osh = wlc->osh;

	if (!pkt) {
#ifdef UTXD_POOL
		p = pktpool_get(wlc->pub->pktpool_utxd);

#else
		p = PKTGET(osh, ULMU_UTXD_BUF_SIZE, TRUE);
#endif /* UTXD_POOL */
		if (!p) {
			WL_ERROR(("wl%d: %s: pktget error\n",
				wlc->pub->unit, __FUNCTION__));
			return BCME_NOMEM;
		}
		ASSERT(ISALIGNED((uintptr)PKTDATA(osh, p), sizeof(uint32)));

#if defined(BCMHWA) && defined(HWA_PKT_MACRO)
		PKTSETHWAPKT(osh, p);
		PKTSETMGMTTXPKT(osh, p);
#endif /* BCMHWA && HWA_PKT_MACRO */

#ifdef WLTAF
		/* reserve tx headroom offset */
		PKTPULL(osh, p, WLULPKTTAGLEN);
#endif
	} else {
		p = pkt;
	}

	PKTSETLEN(osh, p, pad_sz + sizeof(d11ulmu_txd_t));
	memset((uint8 *)PKTDATA(osh, p), 0, pad_sz + sizeof(d11ulmu_txd_t));

	if (!scb) {
		/* global update utxd will not have scb associated, use HW_SCB */
		scb = wlc->band->hwrs_scb;
		ASSERT(scb);
	}

	WLPKTTAGSCBSET(p, scb);
	ASSERT(WLPKTTAGSCBGET(p) != NULL);
	WLPKTTAG(p)->flags |= WLF_UTXD;

	ulmu->utxd_alloc_cnt++;

	/* construct the utxd packet */
	utxd = (d11ulmu_txd_t*) ((uint8 *)PKTDATA(osh, p) + pad_sz);
	utxd->frameid = ((ulmu->frameid++ << D11_REV128_TXFID_SEQ_SHIFT) &
		D11_REV128_TXFID_SEQ_MASK) |
		((TX_FIFO_SU_OFFSET(wlc->pub->corerev) + ULMU_TRIG_FIFO)
		& D11_REV128_TXFID_FIFO_MASK);
	utxd->chanspec = wlc->home_chanspec;
	utxd->utxdcmd = cmd;
	ts = R_REG(wlc->osh, D11_TSFTimerLow(wlc));
	utxd->ts_l = ts;
	utxd->ts_h = (ts >> 16);
	utxd->tagid = ulmu->g_tagid;
	utxd->trigci = (DOT11_HETB_2XLTF_1U6S_GI << D11_ULOTXD_TXCTL_CPF_SHIFT) |
		(DOT11_HETB_2XHELTF_NLTF << D11_ULOTXD_TXCTL_NLTF_SHIFT);
	utxd->mctl0 = ulmu->txd.mctl0;
	/* populate per user info */
	wlc_ulmu_fill_utxd(ulmu, utxd, scb);

	wlc_macdbg_dtrace_log_utxd(wlc->macdbg, scb, utxd);

	if (WL_ULMU_ON() && WL_PRHDRS_ON()) {
		prhex("utxd", PKTDATA(osh, p), 40);
	}
	spktq_enq(&ulmu->utxdq, p);

	if (scb) {
		SCB_UTXD_INFLT_INCR(scb);
	}

	if (fid) {
		*fid = (utxd->frameid >> 7);
	}

	UTXD_PEND_STATS_INCR(ulmu, cmd);

	return BCME_OK;
} /* wlc_ulmu_prep_utxd */

int
wlc_ulmu_post_utxd(wlc_ulmu_info_t *ulmu)
{
	wlc_info_t *wlc = ulmu->wlc;
	wlc_txq_info_t *qi = wlc->active_queue;		/* Active Queue */
	uint16 cnt;
	uint8 user_cnt;
	void *p;
	d11ulmu_txd_t *utxd;
	int pad_sz = 4;
	uint16 cmd;

	if (!HE_ULOMU_ENAB(wlc->pub) || !ULMU_IS_UTXD(ulmu->mode)) {
		WL_ERROR(("wl%d: %s: ulmu_enab %d ulmu mode %d\n",
			wlc->pub->unit, __FUNCTION__, HE_ULOMU_ENAB(wlc->pub), ulmu->mode));
		return BCME_ERROR;
	}

	/* get user count in a bulk commit, it is same as number of utxd in q */
	user_cnt = cnt = spktq_n_pkts(&ulmu->utxdq);

	if (!cnt) {
		return BCME_ERROR;
	}

	WL_INFORM(("%s: OPEN user_cnt %d\n", __FUNCTION__, user_cnt));

	do {
		/* Pull a utxd from q, set the sync field, put it back in the q.
		 * Head is the first utxd that will reach ucode.
		 */
		p = spktq_deq(&ulmu->utxdq);
		if (!p) {
			return BCME_ERROR;
		}

		utxd = (d11ulmu_txd_t*) ((uint8 *)PKTDATA(wlc->osh, p) + pad_sz);

		if ((user_cnt > 1) && (cnt == user_cnt)) {
			/* head, first utxd has first =1, last = 0 */
			UTXD_SYNC_FIRST_SET(utxd->sync, ULMU_ON);
			UTXD_SYNC_LAST_SET(utxd->sync, ULMU_OFF);
		} else if ((user_cnt > 1) && (cnt != 1)) {
			/* in-between utxd has first =0, last = 0 */
			UTXD_SYNC_FIRST_SET(utxd->sync, ULMU_OFF);
			UTXD_SYNC_LAST_SET(utxd->sync, ULMU_OFF);
		} else if ((user_cnt > 1) && (cnt == 1)) {
			/* tail, last utxd has first =0, last = 1 */
			UTXD_SYNC_FIRST_SET(utxd->sync, ULMU_OFF);
			UTXD_SYNC_LAST_SET(utxd->sync, ULMU_ON);
		} else if (user_cnt == 1) {
			/* this is the only utxd in q, has first =1, last = 1 */
			UTXD_SYNC_FIRST_SET(utxd->sync, ULMU_ON);
			UTXD_SYNC_LAST_SET(utxd->sync, ULMU_ON);
		} else {
			/* unhandled case, should never be here */
			ASSERT(0);
		}

		UTXD_SYNC_USER_CNT_SET(utxd->sync, user_cnt);

		cmd = utxd->utxdcmd;

		WL_INFORM(("%s: fid: 0x%x cmd: %d sync: 0x%x\n",
			__FUNCTION__, (utxd->frameid >> 7), utxd->utxdcmd, utxd->sync));

		/* put the utxd back into the q */
		spktq_enq(&ulmu->utxdq, p);
	} while (--cnt);

	WL_INFORM(("%s: CLOSE\n", __FUNCTION__));

	/* update user cnt stats */
	if ((cmd == ULMU_UTXD_USRUPD_R2C) || (cmd == ULMU_UTXD_USRUPD_TRFC)) {
		WLCNTINCR(ulmu->bulk_commit_nuser[user_cnt - 1]);
		WLCNTINCR(ulmu->nbulk_commit);
	}

	wlc_low_txq_enq_list(wlc,
		qi->low_txq, ULMU_TRIG_FIFO, &ulmu->utxdq, user_cnt);

	/* Post packets to fifo */
	txq_hw_fill_utxd(wlc->txqi, qi->low_txq);

	return BCME_OK;
} /* wlc_ulmu_post_utxd */

int
wlc_ulmu_reclaim_utxd(wlc_info_t *wlc, tx_status_t *txs)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	wlc_ulmu_info_t *ulmu = wlc->ulmu;
	void *txp;
	int ncons, cnt = 0;
	scb_t *scb;
	int utxd_range = HNDDMA_RANGE_TRANSMITTED;

	if (!ULMU_IS_UTXD(ulmu->mode)) {
		return cnt;
	}

	if (txs) {
		ncons = (txs->status.raw_bits & TX_STATUS40_NCONS) >>
			TX_STATUS40_NCONS_SHIFT;
		if (ncons == 0) {
			WL_ERROR(("wl%d: %s: ncons %d raw_bits %x s1 %x\n",
				wlc->pub->unit, __FUNCTION__,
				ncons, txs->status.raw_bits, TX_STATUS_MACTXS_S1(txs)));
			ASSERT("ncons == 0");
		}
	} else {
		/* reclaim all utxd in hwfifo */
		ncons = -1;
		utxd_range = HNDDMA_RANGE_ALL;
#if defined(BCMHWA)
		/* Before reclaim all utxd, make sure all packets in HW shadow are page-ined */
		HWA_PKTPGR_EXPR({
			// Pagein packets in HW shadow to SW shadow.
			while (hwa_pktpgr_txfifo_shadow_reclaim(WL_HWA_DEVP(wlc),
				WLC_HW_MAP_TXFIFO(wlc, ULMU_TRIG_FIFO), FALSE)) {
			}

			// Remove specific packets in PageOut Req Q to SW shadow.
			hwa_pktpgr_pageout_ring_shadow_reclaim(WL_HWA_DEVP(wlc),
				WLC_HW_MAP_TXFIFO(wlc, ULMU_TRIG_FIFO));
		});
#endif /* BCMHWA */
	}

	while (((cnt < ncons) || (ncons == -1)) &&
		(txp = wlc_bmac_dma_getnexttxp(wlc, ULMU_TRIG_FIFO, utxd_range))) {
		d11ulmu_txd_t *utxd =
			(d11ulmu_txd_t*) ((uint8 *)PKTDATA(wlc->osh, txp) + ULSTS_UTXD_PAD);

		UTXD_PEND_STATS_DECR(ulmu, utxd->utxdcmd);

		BCM_REFERENCE(utxd);
		WL_ULO(("wl%d: %s: UL OFDMA try freeing idx %d txp %p fid 0x%x cmd %d sync 0x%x\n",
			wlc->pub->unit, __FUNCTION__,  cnt, txp,
			(utxd->frameid >> ULSTS_FRAMEID_SHIFT), utxd->utxdcmd, utxd->sync));

		scb = WLPKTTAGSCBGET(txp);

#ifdef WLTAF
		/* if valid scb and data utxd, decr alloc ref count */
		if (scb && (utxd->utxdcmd == (ulmu->is_run2complete ?
			ULMU_UTXD_USRUPD_R2C : ULMU_UTXD_USRUPD_TRFC))) {
			wlc_ulpkttag_t *tag = ULMU_GET_TAG_FROM_UTXD_PKT(ulmu->wlc->osh, txp);

			ULMU_UTXD_FREE(ulmu, tag, txp);
		} else
#endif /* WLTAF */
		{
			PKTFREE(wlc_hw->osh, txp, TRUE);
			ulmu->utxd_alloc_cnt--;
		}

		wlc_txfifo_complete(wlc, scb, ULMU_TRIG_FIFO, 1);

		/* decrement per-scb pending utxd cnt */
		if (scb) {
			SCB_UTXD_INFLT_DECR(scb);
		}

		cnt++;
	}

	WL_ULO(("wl%d: %s: UL OFDMA freed cnt %d expect ncons %d raw_bits 0x%x fid 0x%x\n",
		wlc->pub->unit, __FUNCTION__, cnt, ncons,
		txs ? txs->status.raw_bits : 0,
		txs ? TX_STATUS_MACTXS_S1(txs) >> (TX_STATUS_FID_SHIFT + ULSTS_FRAMEID_SHIFT) : 0));

	return cnt;
} /* wlc_ulmu_reclaim_utxd */

void
wlc_ulmu_fburst_set(wlc_ulmu_info_t *ulmu, bool enable)
{
	d11ulo_trig_txcfg_t *txd = &ulmu->txd;
	txd->mctl0 &= ~D11AC_TXC_MBURST;
	txd->mctl0 |= (enable ? D11AC_TXC_MBURST: 0);
}

void
wlc_ulmu_maxn_uplimit_set(wlc_ulmu_info_t *ulmu, uint16 val)
{
	int bw;
	if (!ulmu) {
		return;
	}

	for (bw = 0; bw < D11_REV128_BW_SZ; bw++) {
		ulmu->maxn[bw] = MIN(ulmu->maxn[bw], val);
	}
}

static int
wlc_ulmu_maxn_set(wlc_ulmu_info_t *ulmu)
{
#if defined(WL_PSMX)
	wlc_info_t *wlc;
	uint offset;
	int bw;

	wlc = ulmu->wlc;
	if (!wlc->clk) {
		return BCME_NOCLK;
	}

	offset = MX_ULOMAXN_BLK(wlc);
	for (bw = 0; bw < D11_REV128_BW_SZ; bw++) {
		/* ucode does not support maxn=0 */
		wlc_write_shmx(wlc, offset+(bw*2), MAX(ulmu->maxn[bw], 1));
	}
#endif /* defined(WL_PSMX) */

#ifdef WLTAF
	wlc_taf_set_ulofdma_maxn(wlc, &ulmu->maxn);
#endif

	return BCME_OK;
}

void
wlc_ulmu_oper_state_upd(wlc_ulmu_info_t* ulmu, scb_t *scb, uint8 state)
{
	scb_ulmu_t* ulmu_scb;

	if (scb == NULL || ulmu == NULL ||
		((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL)) {
		WL_ERROR(("wl%d: %s: Fail to get ulmu scb cubby STA "MACF"\n",
			ulmu->wlc->pub->unit, __FUNCTION__,
			ETHER_TO_MACF(scb->ea)));
		return;
	}

	ulmu_scb->state = state;
	switch (state) {
		case ULMU_SCB_INIT:
			ulmu_scb->idle_cnt = 0;
			ulmu_scb->last_rx_pkts = 0;
			ulmu_scb->qnullonly_cnt = 0;
			break;

		case ULMU_SCB_ADMT:
		case ULMU_SCB_EVCT:
			/* do nothing for now */
			break;

		default:
			break;
	}
#ifdef WLTAF
	if (wlc_taf_ul_enabled(ulmu->wlc->taf_handle)) {
		wlc_ulmu_taf_enable(ulmu, scb, (state == ULMU_SCB_ADMT));
	}
#endif
}

static void
wlc_ulmu_scb_reqbytes_eval(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	scb_ulmu_t* ulmu_scb;
	uint16 wm;

	if (scb == NULL || ulmu == NULL ||
		((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL)) {
		return;
	}

	BCM_REFERENCE(wm);
	ulmu_scb->tgs_recv_bytes = (ulmu_scb->mlenma >> ULMU_NF_AGGLEN) *
		(ulmu_scb->aggnma >> ULMU_NF_AGGN);

	wm = PERCENT(ulmu_scb->tgs_exprecv_bytes, ulmu_scb->tgs_recv_bytes);
	if (wm <= ULMU_SCB_LOWM ||
		ulmu_scb->qnullonly_cnt >= ULMU_SCB_QNCNT_THRSH ||
		ulmu_scb->fail_cnt >= ULMU_SCB_FLCNT_THRSH) {
		ulmu_scb->tgs_recv_bytes >>= 1;
		ulmu_scb->lowwm = TRUE;
	} else if (ulmu_scb->lowwm == TRUE &&
		wm >= ULMU_SCB_HIWM) {
		ulmu_scb->tgs_recv_bytes += ULMU_MLEN_INIT;
	} else {
		ulmu_scb->lowwm = FALSE;
	}
}

#ifdef WLTAF
/* Get the predicted bytes that can be requested
 * Inputs:
 *   mu_info  - ulmu info pointer
 *   scb      - STA in question
 * Returns:
 *   non-zero value in bytes that can be requested to post utxd
 */
static uint32
wlc_ulmu_scb_reqbytes_get(wlc_ulmu_info_t *ulmu, scb_ulmu_t* ulmu_scb, uint32 cur_ts)
{
	uint64 delta_bytecnt;
	uint32 delta;
	scb_t *scb;
	uint32 byte_cnt = 0;

	BCM_REFERENCE(scb);
	scb = ulmu_scb->scb_bl;

	byte_cnt = ulmu_scb->acc_bufsz;

	if (ulmu_scb->trig_wait_state == ULMU_TRIG_WAIT_ENTERING) {
		byte_cnt = 0;
	} else if ((ulmu_scb->trig_wait_state == ULMU_TRIG_WAIT_ON) &&
		((cur_ts - ulmu_scb->trig_wait_start_ts) < ulmu->trig_wait_us)) {
		byte_cnt = 0;
	}
	else {
		/* delta time in us */
		delta = (cur_ts - ulmu_scb->prev_read_ts);
		ulmu_scb->prev_read_ts = cur_ts;

		/* fix up approximation that (>> 10) is actually dividing by 1024 instead of 1000 */
		delta_bytecnt = (((uint64)ulmu_scb->rate) * 99) >> 12;
		delta_bytecnt += (uint64)ulmu_scb->rate;
		delta_bytecnt *= (uint64)delta;
		delta_bytecnt = (delta_bytecnt >> 10);

		ulmu_scb->acc_bufsz += (uint32)(delta_bytecnt);
		ulmu_scb->acc_bufsz = byte_cnt = LIMIT_TO_MAX(ulmu_scb->acc_bufsz,
			ULMU_ACC_BUFSZ_MAX);

		WL_TAFN(ulmu->wlc, MACF" qlen update: rate %d, delta_bytecnt %d, byte cnt %d\n",
			ETHER_TO_MACF(scb->ea), ulmu_scb->rate,
			(uint32)delta_bytecnt, ulmu_scb->acc_bufsz);

		if (ulmu_scb->trig_wait_state == ULMU_TRIG_WAIT_ON) {
			ulmu_scb->trig_wait_retry_cnt++;
			ulmu_scb->trig_tot_wait_ms +=
				(cur_ts - ulmu_scb->trig_wait_start_ts) / 1000;
			WL_INFORM((""MACF" trig wait mode: ON : retry_cnt %d rate %d qlen %d "
				"trig_wait %d ms total_wait %d ms\n",
				ETHER_TO_MACF(scb->ea), ulmu_scb->trig_wait_retry_cnt,
				ulmu_scb->rate >> 7,
				ulmu_scb->acc_bufsz, (cur_ts - ulmu_scb->trig_wait_start_ts) / 1000,
				ulmu_scb->trig_tot_wait_ms));
		}

		if ((ulmu->qlen_thrsh_cnt > 0) &&
			(ulmu_scb->acc_bufsz < (ulmu_scb->avg_pkt_size * ulmu->qlen_thrsh_cnt)) &&
			(ulmu_scb->pend_trig_cnt > 0)) {

			WL_TAFF(ulmu->wlc, MACF" qlen threshold %u, %u, %u\n",
				ETHER_TO_MACF(scb->ea), ulmu->qlen_thrsh_cnt,
				ulmu_scb->acc_bufsz / ulmu_scb->avg_pkt_size,
				ulmu_scb->pend_trig_cnt);

			byte_cnt = 0;
		}
	}

	if (ulmu->rxmon_qlen) {
		ulmu_scb->acc_bufsz = byte_cnt = ulmu->rxmon_qlen << 10;
	}

	/* update max byte cnt stats */
	WLCNTSET(ULMU_TAF_SCB_STATS(ulmu_scb)->rxmon_max,
		MAX(ULMU_TAF_SCB_STATS(ulmu_scb)->rxmon_max, ulmu_scb->acc_bufsz));

	return byte_cnt;
} /* wlc_ulmu_scb_reqbytes_get */
#endif /* WLTAF */

void
wlc_ulmu_scb_reqbytes_decr(wlc_ulmu_info_t *ulmu, scb_t *scb, uint32 cnt)
{
	scb_ulmu_t* ulmu_scb;

	if (scb == NULL || ulmu == NULL ||
		((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL)) {
		return;
	}

	ulmu_scb->acc_bufsz = (ulmu_scb->acc_bufsz > cnt) ?
		(ulmu_scb->acc_bufsz - cnt) : 0;

	/* update rxmon stats */
	WLCNTSET(ULMU_TAF_SCB_STATS(ulmu_scb)->rxmon_min,
		MIN(ULMU_TAF_SCB_STATS(ulmu_scb)->rxmon_min, ulmu_scb->acc_bufsz));

	WL_ULO(("wl%d: %s: "MACF" rate %d release %d byte cnt %d\n", ulmu->wlc->pub->unit,
		__FUNCTION__, ETHER_TO_MACF(scb->ea), ulmu_scb->rate, cnt,
		ulmu_scb->acc_bufsz));

}

/* ======== attach/detach ======== */

wlc_ulmu_info_t *
BCMATTACHFN(wlc_ulmu_attach)(wlc_info_t *wlc)
{
	wlc_ulmu_info_t *ulmu;
	scb_cubby_params_t cubby_params;

	/* allocate private module info */
	if ((ulmu = MALLOCZ(wlc->osh, sizeof(*ulmu))) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	if ((ulmu->omu_gstats = MALLOCZ(wlc->osh, sizeof(ulmu_gstats_t))) == NULL) {
		WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->osh)));
		goto fail;
	}
	if (D11REV_GE(wlc->pub->corerev, 132)) {
		if ((ulmu->mmu_gstats = MALLOCZ(wlc->osh, sizeof(ulmu_gstats_t))) == NULL) {
			WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes\n", wlc->pub->unit,
				__FUNCTION__, MALLOCED(wlc->osh)));
			goto fail;
		}
	}
	ulmu->wlc = wlc;

	/* HE is attached before this module, so can rely on HE_ENAB here */
	if (!HE_ENAB(wlc->pub)) {
		WL_INFORM(("wl%d: %s: MU scheduler disabled\n", wlc->pub->unit,	__FUNCTION__));
		return ulmu;
	}

	ulmu->policy = ULMU_POLICY_AUTO;
	/* Init ulmu sched */
	wlc_txcfg_ulofdma_maxn_init(wlc, ulmu->maxn);

	ulmu->mode = ULMU_UTXD_MODE;
	ULMU_FLAGS_USCHED_SET(ulmu->flags, ULMU_ON);
	ULMU_FLAGS_HBRNT_SET(ulmu->flags, ULMU_ON);
	ULMU_FLAGS_AUTOULC_SET(ulmu->flags, ULMU_ON);
	ULMU_FLAGS_DSCHED_SET(ulmu->flags, ULMU_OFF);

	ulmu->txd.macctl = ((0 << D11_ULOTXD_MACTL_MODE_NBIT) |
		(ulmu->mode << D11_ULOTXD_MACTL_UTXD_SHIFT) |
		(0 << D11_ULOTXD_MACTL_FIXDUR_NBIT) |
		(HE_TRIG_TYPE_BASIC_FRM << D11_ULOTXD_MACTL_PTYPE_SHIFT) |
		((1 << AC_BK) << D11_ULOTXD_MACTL_ACBMP_SHIFT) |
		(ULMU_MCTL_DURCFG_AVG << D11_ULOTXD_MACTL_DURCFG_SHIFT) |
		(0 << D11_ULOTXD_MACTL_MIXEDBW_SHIFT)); /* 0x0020 */
	ulmu->txd.macctl1 = ((ULMU_FLAGS_HBRNT_GET(ulmu->flags) <<
		D11_ULOTXD_MACTL1_HBRNT_SHIFT) |
		(ULMU_FLAGS_USCHED_GET(ulmu->flags) << D11_ULOTXD_MACTL1_USCHED_SHIFT) |
		(ULMU_FLAGS_DSCHED_GET(ulmu->flags) << D11_ULOTXD_MACTL1_DSCHED_SHIFT));
	ulmu->txd.maxdur = ULMU_TXDUR_MAX;
	ulmu->txd.burst = 2;
	ulmu->txd.maxtw = 5;
	ulmu->txd.txcnt = (uint16) -1;
	ulmu->txd.interval = (uint16) -1; /* Turn on auto trigger interval scheme */
	ulmu->txd.minidle = 100; /* Init to 100us */
	ulmu->txd.rxlowat0 = 3;
	ulmu->txd.rxlowat1 = 30;
	wlc_ulmu_fburst_set(ulmu, WLC_HT_GET_FRAMEBURST(wlc->hti));
	ulmu->txd.txctl = (DOT11_HETB_2XLTF_1U6S_GI << D11_ULOTXD_TXCTL_CPF_SHIFT) |
		(DOT11_HETB_2XHELTF_NLTF << D11_ULOTXD_TXCTL_NLTF_SHIFT);
	ulmu->txlmt = 0x206;
	ulmu->min_ulofdma_usrs = ULMU_USRCNT_MIN;
	ulmu->qnull_thrsh = ULMU_STOPTRIG_QNULL_THRSH;
	ulmu->csthr0 = 76;
	ulmu->csthr1 = 418;
	ulmu->g_tagid = 0;
	ulmu->ulc_sz = ULMU_MAX_ULC_SZ;
	ulmu->rx_pktcnt_thrsh = ULMU_SCB_RX_PKTCNT_THRSH;
	ulmu->idle_prd = ULMU_SCB_IDLEPRD;
	ulmu->min_rssi = ULMU_SCB_MINRSSI;
	ulmu->always_admit = ULMU_ADMIT_AUTO;
#ifdef WLTAF
#ifdef ULMU_NO_R2C
	ulmu->is_run2complete = FALSE;
#else
	ulmu->is_run2complete = TRUE;
#endif
	ulmu->evict_thrsh = ULMU_RXMON_EVICT_THRSH;
	ulmu->trig_wait_start_thrsh = ULMU_TRIG_WAIT_START_THRSH;
	ulmu->trig_wait_us = ULMU_TRIG_WAIT_MS * 1000;
	ulmu->zero_bytes_thrsh = ULMU_ZERO_BYTES_THRSH;
	ulmu->headroom_aggn = ULMU_AGGN_HEADROOM;
	ulmu->per_bw_admn = TRUE; /* enable per-BW admission feature by default */
#endif /* WLTAF */
	wlc_ulmu_drv_ulmu_attach(ulmu);

	spktq_init(&ulmu->utxdq, ULMU_UTXDQ_PKTS_MAX);

	ulmu->is_start = TRUE;
	ulmu->brcm_only = FALSE;
	ulmu->ban_nonbrcm_160sta = FALSE;
#ifdef WL_ULRT_DRVR
	ulmu->drv_ulrtctl = (ULMU_DRVR_RT_MASK);
#endif

	wlc->ulmu = ulmu;
	D11_ULOTXD_UCFG_SET_MCSNSS(ulmu->g_ucfg, 0x17); /* auto rate by default */
	/* register module up/down, watchdog, and iovar callbacks */
	if (wlc_module_register(wlc->pub, ulmu_iovars, "ulmu", ulmu,
		wlc_ulmu_doiovar, wlc_ulmu_watchdog, wlc_ulmu_wlc_init, wlc_ulmu_wlc_deinit)) {
		WL_ERROR(("wl%d: %s: wlc_module_register failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* reserve some space in scb for private data */
	bzero(&cubby_params, sizeof(cubby_params));

	cubby_params.context = ulmu;
	cubby_params.fn_init = wlc_ulmu_scb_init;
	cubby_params.fn_deinit = wlc_ulmu_scb_deinit;
	cubby_params.fn_secsz = wlc_ulmu_scb_secsz;
#if defined(DUMP_SCB_ULMU)
	cubby_params.fn_dump = wlc_ulmu_scb_dump;
#endif /* DUMP_SCB_ULMU */

	if ((ulmu->scbh =
		wlc_scb_cubby_reserve_ext(wlc, sizeof(scb_ulmu_t *), &cubby_params)) < 0) {
		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve_ext() failed\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	if (wlc_scb_state_upd_register(wlc, wlc_ulmu_scb_state_upd, ulmu) != BCME_OK) {
		WL_ERROR(("wl%d: %s: unable to register callback wlc_ulmu_scb_state_upd\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	if ((wlc_bsscfg_state_upd_register(wlc, wlc_ulmu_bsscfg_state_upd, ulmu)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_bsscfg_state_upd_register failed\n",
		          wlc->pub->unit, __FUNCTION__));
			goto fail;
	}

	/* debug dump */
#if defined(BCMDBG_DUMP_UMSCHED)
	wlc_dump_add_fns(wlc->pub, "umsched", wlc_ulmu_dump, wlc_ulmu_dump_clr, ulmu);
#endif /* BCMDBG_DUMP_UMSCHED */

	return ulmu;

fail:
	wlc_ulmu_detach(ulmu);
	return NULL;
} /* wlc_ulmu_attach */

void
BCMATTACHFN(wlc_ulmu_detach)(wlc_ulmu_info_t *ulmu)
{
	wlc_info_t *wlc;
	void *p;

	if (ulmu == NULL) {
		return;
	}

	wlc = ulmu->wlc;

	wlc_scb_state_upd_unregister(wlc, wlc_ulmu_scb_state_upd, ulmu);

	wlc_ulmu_drv_ulmu_detach(ulmu);

	wlc_bsscfg_state_upd_unregister(wlc, wlc_ulmu_bsscfg_state_upd, ulmu);

	wlc_module_unregister(wlc->pub, "ulmu", ulmu);

	while ((p = spktq_deq(&ulmu->utxdq))) {
		PKTFREE(wlc->osh, p, TRUE);
	}
	spktq_deinit(&ulmu->utxdq);

	if (ulmu->omu_gstats != NULL) {
		MFREE(wlc->osh, ulmu->omu_gstats, sizeof(ulmu_gstats_t));
	}
	if (ulmu->mmu_gstats != NULL) {
		MFREE(wlc->osh, ulmu->mmu_gstats, sizeof(ulmu_gstats_t));
	}
	MFREE(wlc->osh, ulmu, sizeof(wlc_ulmu_info_t));

	wlc->ulmu = NULL;
}

int
wlc_ulmu_sw_trig_enable(wlc_info_t *wlc, uint32 enable)
{
	wlc_ulmu_info_t *ulmu = wlc->ulmu;
	d11ulo_trig_txcfg_t *txd = &ulmu->txd;

	if (wlc->pub->up) {
		return BCME_NOTDOWN;
	}

	if (enable == 1) {
		/* check if UL OFDMA is enabled */
		if (!HE_ULOMU_ENAB(wlc->pub)) {
			printf("wl%d: %s: HE UL-OFDMA not enabled\n",
				wlc->pub->unit, __FUNCTION__);
			return BCME_UNSUPPORTED;
		}

		/* umsched usched 0 */
		ULMU_FLAGS_USCHED_SET(ulmu->flags, ULMU_OFF);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_USCHED_MASK;
		txd->macctl1 |= (ULMU_OFF << D11_ULOTXD_MACTL1_USCHED_SHIFT);

		/* umsched dsched 1 */
		ULMU_FLAGS_DSCHED_SET(ulmu->flags, ULMU_ON);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_DSCHED_MASK;
		txd->macctl1 |= (ULMU_ON << D11_ULOTXD_MACTL1_DSCHED_SHIFT);

		/* umsched hibernate 0 */
		ULMU_FLAGS_HBRNT_SET(ulmu->flags, ULMU_OFF);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_HBRNT_MASK;
		txd->macctl1 |= (ULMU_OFF << D11_ULOTXD_MACTL1_HBRNT_SHIFT);

		/* disable pairing by ucode */
#ifdef ULMU_NO_R2C
		txd->macctl &= ~D11_ULOTXD_MACTL_DURCFG_MASK;
		txd->macctl |= ULMU_MCTL_DURCFG_MIN << D11_ULOTXD_MACTL_DURCFG_SHIFT;
#else
		txd->macctl &= ~D11_ULOTXD_MACTL_DURCFG_MASK;
		txd->macctl |= ULMU_MCTL_DURCFG_MAX << D11_ULOTXD_MACTL_DURCFG_SHIFT;
#endif

		/* disable trigger delay by ucode */
		txd->interval = ULMU_TRIGGER_INT_1US;
#ifdef ULMU_NO_R2C
		ulmu->is_run2complete = FALSE;
#else
		ulmu->is_run2complete = TRUE;
#endif

#ifndef BYTE_INTERFACE
		/* airtime interface */
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_AIRTIME_MASK;
		txd->macctl1 |= (ULMU_ON << D11_ULOTXD_MACTL1_AIRTIME_SHIFT);
#else
		/* byte interface */
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_AIRTIME_MASK;
		txd->macctl1 |= (ULMU_OFF << D11_ULOTXD_MACTL1_AIRTIME_SHIFT);
#endif
	} else if (enable == 0) {

		/* umsched usched 1 */
		ULMU_FLAGS_USCHED_SET(ulmu->flags, ULMU_ON);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_USCHED_MASK;
		txd->macctl1 |= (ULMU_ON << D11_ULOTXD_MACTL1_USCHED_SHIFT);

		/* umsched dsched 0 */
		ULMU_FLAGS_DSCHED_SET(ulmu->flags, ULMU_OFF);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_DSCHED_MASK;
		txd->macctl1 |= (ULMU_OFF << D11_ULOTXD_MACTL1_DSCHED_SHIFT);

		/* umsched hibernate 1 */
		ULMU_FLAGS_HBRNT_SET(ulmu->flags, ULMU_ON);
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_HBRNT_MASK;
		txd->macctl1 |= (ULMU_ON << D11_ULOTXD_MACTL1_HBRNT_SHIFT);

		/* enable pairing by ucode, use avg policy */
		txd->macctl &= ~D11_ULOTXD_MACTL_DURCFG_MASK;
		txd->macctl |= ULMU_MCTL_DURCFG_AVG << D11_ULOTXD_MACTL_DURCFG_SHIFT;
		/* Turn on auto trigger interval scheme */
		txd->interval = ULMU_TRIGGER_INT_AUTO;
		ulmu->is_run2complete = FALSE;

		/* byte interface */
		txd->macctl1 &= ~D11_ULOTXD_MACTL1_AIRTIME_MASK;
		txd->macctl1 |= (ULMU_OFF << D11_ULOTXD_MACTL1_AIRTIME_SHIFT);
	} else {
		return BCME_RANGE;
	}

	return BCME_OK;
} /* wlc_ulmu_sw_trig_enable */

#ifdef WLTAF
#if defined(BCMDBG_DUMP_UMSCHED)
static int
wlc_ulmu_taf_stats_upd(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	scb_ulmu_t *ulmu_scb = SCB_ULMU(ulmu, scb);
	ulmu_taf_stats_t *stats;
	uint32 relbytes_per_utxd, rxbytes_per_utxd, padded_per_trig, reldur_per_utxd,
		txdur_per_utxd, padding, relbytes, reldur, unused;

	if (!ulmu_scb)
		return BCME_ERROR;

	stats = ULMU_TAF_SCB_STATS(ulmu_scb);
	ASSERT(stats);

	if (stats->nutxd) {
		/* rxmon min bytes */
		WLCNTSET(stats->rxmon_min_bytes, stats->rxmon_min);

		/* rxmon max bytes */
		WLCNTSET(stats->rxmon_max_bytes, stats->rxmon_max);

		/* avg release bytes */
		relbytes_per_utxd = wlc_uint64_div((uint64)stats->bytes_rel, (uint64)stats->nutxd);
		WLCNTSET(stats->relbytes_per_utxd, relbytes_per_utxd);

		/* avg recvd bytes per utxd */
		rxbytes_per_utxd = wlc_uint64_div((uint64)stats->bytes_recvd, (uint64)stats->nutxd);
		WLCNTSET(stats->rxbytes_per_utxd, rxbytes_per_utxd);

		/* avg padded bytes per trigger */
		padded_per_trig =
			wlc_uint64_div((uint64)stats->bytes_trigd - (uint64)stats->bytes_recvd,
				(uint64)stats->nupd);
		WLCNTSET(stats->padded_bytes_per_trig, padded_per_trig);

		/* avg released duration per utxd */
		reldur_per_utxd = wlc_uint64_div((uint64)stats->usec_trigd, (uint64)stats->nutxd);
		WLCNTSET(stats->reldur_per_utxd, reldur_per_utxd);

		/* avg tx duration per utxd */
		txdur_per_utxd = wlc_uint64_div((uint64)stats->usec_txdur, (uint64)stats->nutxd);
		WLCNTSET(stats->txdur_per_utxd, txdur_per_utxd);
	}

	if (stats->nupd) {
		/* update qos null and timeout related stats */
		WLCNTSET(stats->qosnull_per_trig, (stats->nqosnull * 100) / stats->nupd);
		WLCNTSET(stats->timeout_per_trig, (stats->ntimeout * 100) / stats->nupd);
		WLCNTSET(stats->suppress_per_trig, (stats->nsuppress * 100) / stats->nupd);
	}

	if (stats->bytes_recvd) {
		relbytes = wlc_uint64_div((uint64)stats->bytes_recvd * 100,
			(uint64)stats->bytes_rel);
		WLCNTSET(stats->relbytes, relbytes);
	}

	if (stats->usec_txdur) {
		reldur = wlc_uint64_div((uint64)stats->usec_txdur  * 100,
			(uint64)stats->usec_trigd);
		WLCNTSET(stats->reldur, reldur);
	}

	if (stats->usec_trigd) {
		/* update qos null and timeout related stats */
		WLCNTSET(stats->qosnull_airtime,
			wlc_uint64_div((uint64)stats->usec_qosnull * 100,
				(uint64)stats->usec_txdur));
		WLCNTSET(stats->timeout_airtime,
			wlc_uint64_div((uint64)stats->usec_timeout * 100,
				(uint64)stats->usec_txdur));
		WLCNTSET(stats->suppress_airtime,
			wlc_uint64_div((uint64)stats->usec_suppress * 100,
				(uint64)stats->usec_txdur));
	}

	if (stats->bytes_trigd) {
		/* padded airtime */
		padding =
			wlc_uint64_div(((uint64)stats->bytes_trigd -
				(uint64)stats->bytes_recvd) * 100,
				(uint64)stats->bytes_trigd);

		padding -= stats->qosnull_airtime - stats->timeout_airtime;
		WLCNTSET(stats->padded_airtime, padding);

		/* unused airtime */
		unused = padding + stats->qosnull_airtime;
		WLCNTSET(stats->unused_airtime, unused);
	}

	if (stats->trig_wait_cnt) {
		WLCNTSET(stats->avg_trig_wait_retries,
			stats->trig_wait_cum_retry_cnt/stats->trig_wait_cnt);

		WLCNTSET(stats->avg_trig_wait_time,
			stats->trig_wait_cum_ms/stats->trig_wait_cnt);
	}

#ifdef TAF_UL_STATS_CLEAR_ON_READ
	WLCNTSET(stats->rxmon_min, UINT32_MAX);
	WLCNTSET(stats->rxmon_max, 0);
	WLCNTSET(stats->bytes_rel, 0);
	WLCNTSET(stats->usec_trigd, 0);
	WLCNTSET(stats->usec_txdur, 0);
	WLCNTSET(stats->bytes_trigd, 0);
	WLCNTSET(stats->bytes_recvd, 0);
	WLCNTSET(stats->nqosnull, 0);
	WLCNTSET(stats->ntimeout, 0);
	WLCNTSET(stats->nsuppress, 0);
	WLCNTSET(stats->usec_qosnull, 0);
	WLCNTSET(stats->usec_timeout, 0);
	WLCNTSET(stats->usec_suppress, 0);
	WLCNTSET(stats->nutxd, 0);
	WLCNTSET(stats->nupd, 0);
#endif /* TAF_UL_STATS_CLEAR_ON_READ */

	return BCME_OK;
} /* wlc_ulmu_taf_stats_upd */

static int
wlc_ulmu_taf_stats_reset(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	scb_ulmu_t *ulmu_scb = SCB_ULMU(ulmu, scb);
	ulmu_taf_stats_t *stats;

	if (!ulmu_scb)
		return BCME_ERROR;

	stats = ULMU_TAF_SCB_STATS(ulmu_scb);
	ASSERT(stats);

	/* clear TAF UL stats */
	memset(stats, 0, sizeof(ulmu_taf_stats_t));
	WLCNTSET(stats->rxmon_min, UINT32_MAX);

	ulmu_scb->excess_trigger_time = 0;

	memset(ulmu->bulk_commit_nuser, 0, sizeof(ulmu->bulk_commit_nuser));

	ulmu->nbulk_commit = 0;

	return BCME_OK;
}

static void
wlc_ulmu_dump_bulk_commit_stats(wlc_ulmu_info_t *ulmu, bcmstrbuf_t *b)
{
	int i;
	int n = ulmu->nbulk_commit;

	if (n) {
		bcm_bprintf(b, "\n");
		bcm_bprintf(b, "Bulk commit user stats  ");

		for (i = 0; i < ULMU_TRIG_USRCNT_MAX; i++) {
			bcm_bprintf(b, "%d: %d%%    ",
				i + 1, (ulmu->bulk_commit_nuser[i] * 100)/n);
		}

		bcm_bprintf(b, "\n");
	}
}

static void
wlc_ulmu_taf_print_scb_stats(wlc_ulmu_info_t *ulmu, scb_t *scb, bcmstrbuf_t *b)
{
	scb_ulmu_t *ulmu_scb = SCB_ULMU(ulmu, scb);
	ulmu_taf_stats_t *stats;

	ASSERT(ulmu_scb);
	ASSERT(scb);
	ASSERT(b);
	ASSERT(ULMU_SCB_STATS(ulmu_scb));

	stats = ULMU_TAF_SCB_STATS(ulmu_scb);
	ASSERT(stats);

	/* update stats */
	wlc_ulmu_taf_stats_upd(ulmu, scb);

	bcm_bprintf(b, "\nTAF stats summary:\n");
	if (stats->nutxd) {
		uint32 x = stats->nupd * 10;
		uint32 y = x / stats->nutxd;
		if (((x * 10) / stats->nutxd) % 10 > 5) {
			y++; /* round up */
		}
		bcm_bprintf(b, " nupd:nutxd=%d.%d : 1\n",
			y / 10, y % 10);
	}
	bcm_bprintf(b, " req_bytes_per_utxd=%d req_dur_per_utxd=%d\n",
		stats->relbytes_per_utxd, stats->reldur_per_utxd);
	bcm_bprintf(b, " actual_rxbytes_per_utxd=%d actual_txdur_per_utxd=%d"
		" padded_bytes_per_trig=%d\n",
		stats->rxbytes_per_utxd, stats->txdur_per_utxd, stats->padded_bytes_per_trig);
	bcm_bprintf(b, " qosnull_cnt=%d(%d%%) timeout_cnt=%d(%d%%) suppress_cnt=%d(%d%%)\n",
		stats->nqosnull, stats->qosnull_per_trig, stats->ntimeout, stats->timeout_per_trig,
		stats->nsuppress, stats->suppress_per_trig);

	bcm_bprintf(b, "TAF trigger accuracy:\n");
	bcm_bprintf(b, " released_bytes(recvd/requested)=%d%%\n", (int)stats->relbytes);
	bcm_bprintf(b, " released_dur(recvd/requested)=%d%%\n", stats->reldur);
	bcm_bprintf(b, " qosnull_airtime=%d%% padded_airtime=%d%% total_unused=%d%%\n",
		stats->qosnull_airtime, stats->padded_airtime, stats->unused_airtime);
	bcm_bprintf(b, " timeout_airtime=%d%% suppress_airtime=%d%%\n",
		stats->timeout_airtime, stats->suppress_airtime);

	bcm_bprintf(b, "RX monitor stats:\n");
	bcm_bprintf(b, " rxmon_min_bytes=%d rxmon_max_bytes=%d rate=%d pend_trig_cnt %d "
		"excess_trigger_time %d avg_pkt_sz %d peak_aggn %d\n",
		stats->rxmon_min_bytes, stats->rxmon_max_bytes,
		(ulmu_scb->rate *  8)/1000, ulmu_scb->pend_trig_cnt, ulmu_scb->excess_trigger_time,
		ulmu_scb->avg_pkt_size, ulmu_scb->peak_aggn);

	bcm_bprintf(b, "Trigger wait stats:\n");
	bcm_bprintf(b, " trig_wait_cnt=%d avg_trig_wait_retries=%d avg_trig_wait_time=%d\n",
		stats->trig_wait_cnt, stats->avg_trig_wait_retries, stats->avg_trig_wait_time);

	bcm_bprintf(b, "\n");
} /* wlc_ulmu_taf_print_scb_stats */

static void
wlc_ulmu_dump_utxd_in_transit_stats(wlc_ulmu_info_t *ulmu, bcmstrbuf_t *b)
{
	int i;
	int n = ulmu->nbulk_commit;

	if (n) {
		bcm_bprintf(b, "\n");
		bcm_bprintf(b, "UTXD in transit stats  ");

		for (i = 0; i < UTXD_NUM_TYPES; i++) {
			bcm_bprintf(b, "%s: %d    ",
				ulmu_utxd_type_names[i], ulmu->utxd_in_transit[i]);
		}

		bcm_bprintf(b, "\n");
	}
}

static void
wlc_ulmu_print_eviction_hist(scb_ulmu_t* ulmu_scb, bcmstrbuf_t *b)
{
	ulmu_stats_t *ul_stats = ULMU_SCB_STATS(ulmu_scb);
	int i;
	int n = ul_stats->nadmit;

	if (n) {
		bcm_bprintf(b, " Evict Hist [");

		for (i = 0; i < EVICT_REASON_NUM_TYPES; i++) {
			bcm_bprintf(b, "%s: %d  ",
				ulmu_evict_reason_names[i], ul_stats->evict_hist[i]);
		}
		bcm_bprintf(b, "]");
		bcm_bprintf(b, "\n");
	}
}
#endif /* BCMDBG_DUMP_UMSCHED */

#ifdef TAF_INF_UL_TRIGGER /* define it for infinite triggering */
static uint32 BCMFASTPATH
wlc_ulmu_rxbyte_cnt(scb_ulmu_t * ulmu_scb)
{
	/* rx monitor byte cnt API, return some large value for now */
	return 0xA00000;
}
#endif /* TAF_INF_UL_TRIGGER */

void * BCMFASTPATH
wlc_ulmu_taf_get_scb_info(void *ulmuh, scb_t *scb)
{
	wlc_ulmu_info_t *ulmu = (wlc_ulmu_info_t *)ulmuh;

	return (ulmu && scb) ? (void*)SCB_ULMU(ulmu, scb) : NULL;
}

void * BCMFASTPATH
wlc_ulmu_taf_get_scb_tid_info(void *scb_h, int tid)
{
	return 0;
}

bool BCMFASTPATH
wlc_ulmu_taf_update(void *ulmuh, void *scbh, void *tidh, uint32 ts)
{
	wlc_ulmu_info_t *ulmu = (wlc_ulmu_info_t *)ulmuh;
	scb_ulmu_t * ulmu_scb;
	bool update;

	TAF_ASSERT(ulmu && scbh);

	if (ulmu->qlen_incr_wait == 0 || ulmu->qlen_incr_amount == 0) {
		return FALSE;
	}
	ulmu_scb = (scb_ulmu_t *)scbh;

	if ((ts - ulmu_scb->qlen_incr_prev_ts) < (ulmu->qlen_incr_wait << 10)) {
		return FALSE;
	}

	/* if rate is 0 and no pending triggers, don't bump so that we can evict */
	if (!ulmu_scb->rate && !ulmu_scb->pend_trig_cnt) {
		return FALSE;
	}

	ulmu_scb->qlen_incr_prev_ts = ts;

	update = wlc_ulmu_bump_bytecnt(ulmu, ulmu_scb);

	WL_TAFN(ulmu->wlc, "update "MACF" %u\n", ETHER_TO_MACF(ulmu_scb->scb_bl->ea), update);

	return update;
}

uint16 BCMFASTPATH
wlc_ulmu_taf_get_qlen(void *ulmuh, void *scbh, void *tidh, uint32 ts)
{
	scb_ulmu_t * ulmu_scb = (scb_ulmu_t *)scbh;
	wlc_ulmu_info_t *ulmu = (wlc_ulmu_info_t *)ulmuh;
	scb_t *scb = ulmu_scb->scb_bl;
	uint32 byte_cnt;
	uint32 taf_kbits_rate;
	wlc_info_t* wlc = ulmu->wlc;
	taf_rxpkt_info_t pkt_info;

	wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, 0, &pkt_info,
		TAF_RXPKT_STATUS_GET_PKT_INFO);

	ulmu_scb->avg_pkt_size = pkt_info.size.average;
	ulmu_scb->peak_pkt_size = pkt_info.size.peak;
	ulmu_scb->peak_aggn = pkt_info.aggn.peak;

	ASSERT(ulmu_scb->avg_pkt_size && ulmu_scb->peak_pkt_size && ulmu_scb->peak_aggn);

	/* the following will 'get' the Rx byte rate in Kbits/second */
	wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, 0,
		&taf_kbits_rate, TAF_RXPKT_STATUS_GET_BYTE_RATE);

	ulmu_scb->rate = taf_kbits_rate >> 3;

	byte_cnt = wlc_ulmu_scb_reqbytes_get(ulmu, ulmu_scb, ts);

	if (ulmu_scb->avg_pkt_size && byte_cnt) {
		if (!ulmu_scb->pend_trig_cnt && !ulmu_scb->rate) {
			/* if rate is 0 and no pending utxd, round up to 1 pkt so that
			 * we can send a trigger out. Otherwise, qlen will not drop to 0
			  * and we will not evict.
			 */
			ulmu_scb->pktq_len = MAX(byte_cnt / ulmu_scb->avg_pkt_size, 1);
		} else {
			ulmu_scb->pktq_len = byte_cnt / ulmu_scb->avg_pkt_size;
		}
	} else {
		ulmu_scb->pktq_len = 0;
	}

	WL_TAFF(wlc, MACF" qlen %u, byte cnt %u\n",
		ETHER_TO_MACF(scb->ea), ulmu_scb->pktq_len, byte_cnt);

	return ulmu_scb->pktq_len;
}

uint32 wlc_ulmu_pend_trig_cnt(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	scb_ulmu_t* ulmu_scb = SCB_ULMU(ulmu, scb);

	return ulmu_scb->pend_trig_cnt;
}

static void
wlc_ulmu_scb_rxmon_rate_eval(wlc_ulmu_info_t *ulmu, scb_t *scb, uint32 status_code)
{
	scb_ulmu_t* ulmu_scb;
	ulmu_stats_t *ulstats;

	if (scb == NULL || ulmu == NULL ||
		((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL)) {
		return;
	}

	/* Do not evict trigger-enabled twt user */
	if (wlc_twt_scb_is_trig_enab(ulmu->wlc->twti, scb)) {
		return;
	}

	if (status_code == ULMU_STATUS_QOSNULL) {
		/* check and enter trigger wait mode */
		if (ulmu->trig_wait_us) {
			if (ulmu_scb->trig_wait_state == ULMU_TRIG_WAIT_OFF) {
				if (ulmu_scb->pend_trig_cnt) {
					ulmu_scb->trig_wait_state = ULMU_TRIG_WAIT_ENTERING;
					WL_TAFF(ulmu->wlc, MACF" trig wait mode: ENTERING : "
						"rate %d qlen %d\n", ETHER_TO_MACF(scb->ea),
						ulmu_scb->rate >> 7,
						ulmu_scb->acc_bufsz);

				} else {
					ulmu_scb->trig_wait_state = ULMU_TRIG_WAIT_ON;
					ulmu_scb->trig_wait_start_ts =
						wlc_read_usec_timer(ulmu->wlc);

					WL_TAFF(ulmu->wlc, MACF" trig wait mode: ON : rate %d "
						"qlen %d\n",
						ETHER_TO_MACF(scb->ea), ulmu_scb->rate >> 7,
						ulmu_scb->acc_bufsz);
				}
			} else if (ulmu_scb->trig_wait_state == ULMU_TRIG_WAIT_ENTERING) {
				ulmu_scb->trig_wait_state = ULMU_TRIG_WAIT_ON;
				ulmu_scb->trig_wait_start_ts = wlc_read_usec_timer(ulmu->wlc);

				WL_TAFF(ulmu->wlc, MACF" trig wait mode: ON :"
					" rate %d qlen %d\n",
					ETHER_TO_MACF(scb->ea), ulmu_scb->rate >> 7,
					ulmu_scb->acc_bufsz);
			} else if (ulmu_scb->trig_wait_state == ULMU_TRIG_WAIT_ON) {
				ulmu_scb->trig_wait_start_ts = wlc_read_usec_timer(ulmu->wlc);
			}
		}

		if (ulmu_scb->trig_wait_state == ULMU_TRIG_WAIT_ON) {
			/* qos null recvd, reset the qlen */
			WL_TAFF(ulmu->wlc, MACF" resetting qlen (%u) to 0\n",
				ETHER_TO_MACF(scb->ea), ulmu_scb->acc_bufsz);

			ulmu_scb->acc_bufsz = 0;
		}

		ulmu_scb->acc_bufsz = 0;
	} else if (ulmu->trig_wait_us && (ulmu_scb->trig_wait_state != ULMU_TRIG_WAIT_OFF)) {
		ulmu_scb->trig_wait_state = ULMU_TRIG_WAIT_OFF;

		WL_TAFF(ulmu->wlc, MACF" trig wait mode: OFF: retry_cnt %d tot_wait_ms %d "
			"rate %d qlen %d\n",
			ETHER_TO_MACF(scb->ea), ulmu_scb->trig_wait_retry_cnt,
			ulmu_scb->trig_tot_wait_ms, ulmu_scb->rate >> 7,
			ulmu_scb->acc_bufsz);

		WLCNTINCR(ULMU_TAF_SCB_STATS(ulmu_scb)->trig_wait_cnt);
		WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->trig_wait_cum_retry_cnt,
			ulmu_scb->trig_wait_retry_cnt);
		WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->trig_wait_cum_ms,
			ulmu_scb->trig_tot_wait_ms);

		ulmu_scb->trig_wait_retry_cnt = 0;
		ulmu_scb->trig_tot_wait_ms = 0;
	}

	/* evict if rate, byte cnt are 0 and no utxd in flight */
	if (!ulmu->always_admit &&
		!ulmu_scb->acc_bufsz && !ulmu_scb->rate && !ulmu_scb->pend_trig_cnt &&
		status_code != ULMU_STATUS_EVICTED) {
		WL_ULO((
			"Evicting "MACF" trig_wait_start_cnt %d trig_wait_state %d "
			"zero_bytes_cnt %d rxmon_idle_cnt %d\n",
			ETHER_TO_MACF(scb->ea), ulmu_scb->trig_wait_start_cnt,
			ulmu_scb->trig_wait_state, ulmu_scb->zero_bytes_cnt,
			ulmu_scb->rxmon_idle_cnt));

		wlc_ulmu_admit_client(ulmu, scb, EVICT_CLIENT);
		if ((ulstats = ulmu_scb->scb_omu_stats) != NULL) {
			WLCNTINCR(ulstats->evict_hist[EVICT_REASON_RXMON]);
		}
	}

	WL_ULO((""MACF" trig_wait_start_cnt %d trig_wait_state %d "
		"zero_bytes_cnt %d rxmon_idle_cnt %d\n",
		ETHER_TO_MACF(scb->ea), ulmu_scb->trig_wait_start_cnt,
		ulmu_scb->trig_wait_state, ulmu_scb->zero_bytes_cnt,
		ulmu_scb->rxmon_idle_cnt));
} /* wlc_ulmu_scb_rxmon_rate_eval */

#ifndef BYTE_INTERFACE
static int
wlc_ulmu_trig_req_cb(wlc_info_t * wlc, scb_t *scb,
	packet_trigger_info_t *ti, void *p, uint32 status_code, uint32 duration)
#else
static int
wlc_ulmu_trig_req_cb(wlc_info_t * wlc, scb_t *scb,
	packet_trigger_info_t *ti, void *p, uint32 status_code, uint32 bytes_consumed,
	uint32 avg_pktsz, uint32 duration)
#endif
{
	wlc_ulmu_info_t *ulmu = wlc->ulmu;
	scb_ulmu_t *ulmu_scb;
	wlc_ulpkttag_t *tag;
	d11ulmu_txd_t *utxd;

#if defined(HWA_PKTPGR_BUILD)
	// Driver will do PKT pointers adjustment after BME copy
	// For trigger status, it will access the local packet.
	// Need to restore data, head, and end pointer if this packet is not adjusted.
	if (PKTNEEDLOCALFIXUP(wlc->osh, p)) {
		PKTADJDATA(wlc->osh, p, PKTPPBUFFERP(p),
			PKTLEN(wlc->osh, p), PKTGETHEADROOM(wlc->osh, p));
		PKTRESETLOCALFIXUP(ulmu->wlc->osh, p);
	}
#endif /* HWA_PKTPGR_BUILD */

#ifdef DONGLEBUILD
	ASSERT(PKTDATA(wlc->osh, p) != (uint8 *)0xdeadbeef);
	if (PKTDATA(wlc->osh, p) == (uint8 *)0xdeadbeef) {
		return BCME_ERROR;
	}
#endif /* DONGLEBUILD */

	BCM_REFERENCE(tag);
	tag = ULMU_GET_TAG_FROM_UTXD_PKT(wlc->osh, p);

	utxd = (d11ulmu_txd_t*) ((uint8 *)PKTDATA(wlc->osh, p) + ULSTS_UTXD_PAD);

	if (utxd->utxdcmd != (ulmu->is_run2complete ?
		ULMU_UTXD_USRUPD_R2C : ULMU_UTXD_USRUPD_TRFC)) {
		return BCME_ERROR;
	}

	TAF_ASSERT(tag->index <= TAF_MAX_PKT_INDEX);

	/*
	 * Return if we are not set up to handle the tx status for this scb.
	 */
	if (!scb) { /* SCB check */
		WL_ERROR(("wl%d: %s: null scb \n",
				WLCWLUNIT(wlc), __FUNCTION__));
		TAF_ASSERT(0);
		return BCME_ERROR;
	}

	/* check for feature not enabled */
	if (!SCB_ULOFDMA_ADM(scb) && (status_code != ULMU_STATUS_EVICTED)) {
		WL_ERROR(("wl%d: SCB:"MACF" %s: ul ofdma feature not enabled status: %d\n",
				WLCWLUNIT(wlc), ETHER_TO_MACF(scb->ea), __FUNCTION__, status_code));
	}

	ulmu_scb = SCB_ULMU(ulmu, scb);
	if (!ulmu_scb) {
		WL_ERROR(("wl%d: SCB:"MACF" %s: null ulmu_scb\n",
				WLCWLUNIT(wlc), ETHER_TO_MACF(scb->ea), __FUNCTION__));
		TAF_ASSERT(0);
		return BCME_ERROR;
	}

	if (!WLCNTVAL(ulmu_scb->pend_trig_cnt)) {
		return BCME_ERROR;
	}

	/* check for admission */
	if ((ulmu_scb->state != ULMU_SCB_ADMT) && (status_code != ULMU_STATUS_EVICTED)) {
		WL_ERROR(("wl%d: SCB:"MACF" %s: not admitted state %d status: %d\n",
				WLCWLUNIT(wlc), ETHER_TO_MACF(scb->ea), __FUNCTION__,
				ulmu_scb->state, status_code));
	}

	if ((status_code == ULMU_STATUS_INPROGRESS) ||
		(status_code == ULMU_STATUS_THRESHOLD)) {
		uint32 units, completed_units;
		/* update partial completes */
#ifndef BYTE_INTERFACE
		completed_units = wlc_uint64_div((uint64)tag->req_trig_units *
			(uint64)duration,
			(uint64)tag->req_trig_us);
#else
		completed_units = wlc_uint64_div((uint64)tag->req_trig_units *
			(uint64)bytes_consumed,
			(uint64)tag->req_trig_bytes);
#endif
		completed_units = LIMIT_TO_MAX(completed_units, tag->req_trig_units);
		units = completed_units - tag->compl_trig_units;
		tag->units = units;
		tag->compl_trig_units = completed_units;

#ifndef BYTE_INTERFACE
		WL_TAFF(wlc, MACF" index %d status %d completed dur %d"
			" req units %d flag %d units %d completed_units %d\n",
			ETHER_TO_MACF(scb->ea), tag->index, status_code,
			duration, tag->req_trig_units, tag->flags3, tag->units, completed_units);
#else
		WL_TAFF(wlc, MACF" index %d status %d bytes consumed %d avg_pktsz %d dur %d"
			" req units %d flag %d units %d completed_units %d\n",
			ETHER_TO_MACF(scb->ea), tag->index, status_code, bytes_consumed, avg_pktsz,
			duration, tag->req_trig_units, tag->flags3, tag->units, completed_units);
#endif

		wlc_taf_txpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, tag,
			TAF_TXPKT_STATUS_TRIGGER_COMPLETE);

		if (!wlc_taf_scheduler_blocked(wlc->taf_handle)) {
			wlc_taf_schedule(wlc->taf_handle, TAF_UL_PRIO, scb, FALSE);
		}

		return BCME_OK;
	}

	tag->units = tag->req_trig_units - tag->compl_trig_units;

	if (status_code == ULMU_STATUS_SUPPRESS) {
		wlc_taf_txpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, tag,
			TAF_TXPKT_STATUS_UL_SUPPRESSED);
	} else {
		wlc_taf_txpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, tag,
			TAF_TXPKT_STATUS_TRIGGER_COMPLETE);
	}

	if (status_code == ULMU_STATUS_QOSNULL || status_code == ULMU_STATUS_TIMEOUT ||
		status_code == ULMU_STATUS_SUPPRESS) {
		taf_rxpkt_stats_t stats;

		memset(&stats, 0, sizeof(stats));

		/* feed 0 into rx monitor to keep the time base of the filter moving */
		wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, -1, &stats,
			TAF_RXPKT_STATUS_UPDATE_BYTE_PKT_RATE_TRIG_STATUS);
	}

	WLCNTDECR(ulmu_scb->pend_trig_cnt);
	ULTRIGPENDDEC(wlc);

	WL_TAFF(wlc, MACF" index %d status %d completed dur %d"
		" req units %d flag %d units %d"
		" trig_wait_state %d trig_pend_total %d trig_pend_this %d\n",
		ETHER_TO_MACF(scb->ea), tag->index, status_code,
		duration, tag->req_trig_units, tag->flags3, tag->units,
		ulmu_scb->trig_wait_state, ULTRIGPENDTOT(wlc), WLCNTVAL(ulmu_scb->pend_trig_cnt));

	wlc_ulmu_scb_rxmon_rate_eval(ulmu, scb, status_code);

	ULMU_UTXD_FREE(ulmu, tag, p);

	if (!wlc_taf_scheduler_blocked(wlc->taf_handle)) {
		wlc_taf_schedule(wlc->taf_handle, TAF_UL_PRIO, scb, FALSE);
	}

	return BCME_OK;
} /* wlc_ulmu_trig_req_cb */

#ifndef BYTE_INTERFACE
static int BCMFASTPATH
wlc_ulmu_send_trigger(wlc_ulmu_info_t* ulmu, scb_t *scb, int bytes,
	int us, void *p, bool bulk_commit)
#else
static int BCMFASTPATH
wlc_ulmu_send_trigger(wlc_ulmu_info_t* ulmu, scb_t *scb, int bytes,
	void *p, bool bulk_commit)
#endif
{
	wlc_ulmu_trigger_info_t trig_info;
	scb_ulmu_t* ulmu_scb;
	int ret = BCME_OK;
	wlc_ulpkttag_t *tag;

	BCM_REFERENCE(trig_info);
	if (scb == NULL || ulmu == NULL ||
		((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL)) {
		return BCME_BADARG;
	}

	tag = ULMU_GET_TAG_FROM_UTXD_PKT(ulmu->wlc->osh, p);

	if (tag->index > TAF_MAX_PKT_INDEX) {
		WL_ERROR(("wl%d: %s: "MACF" index %d units %d flag %d\n",
			WLCWLUNIT(ulmu->wlc), __FUNCTION__,
			ETHER_TO_MACF(scb->ea), tag->index, tag->units,
			tag->flags3));
		TAF_ASSERT(tag->index <= TAF_MAX_PKT_INDEX);
	}

	WL_TAFF(ulmu->wlc, MACF" index %d units %d flag %d bulk_commit %d\n",
		ETHER_TO_MACF(scb->ea), tag->index, tag->units, tag->flags3, bulk_commit);

	if (bytes) {
		TAF_ASSERT(tag != NULL);

		trig_info.trigger_type.packet_trigger.trigger_bytes = bytes;
#ifndef BYTE_INTERFACE
		trig_info.trigger_type.packet_trigger.trigger_us = us;
#endif
		trig_info.trigger_type.packet_trigger.callback_function =
			wlc_ulmu_trig_req_cb;
		trig_info.trigger_type.packet_trigger.callback_parameter =
			(void *)p;
		trig_info.trigger_type.packet_trigger.callback_reporting_threshold =
			ULMU_CB_TEST_THRESHOLD;
		trig_info.trigger_type.packet_trigger.multi_callback = TRUE;
		trig_info.trigger_type.packet_trigger.qos_null_threshold =
			ULMU_QOSNULL_LIMIT;
		trig_info.trigger_type.packet_trigger.failed_request_threshold =
			ULMU_TIMEOUT_LIMIT;
		if (bulk_commit) {
			trig_info.trigger_type.packet_trigger.post_utxd = FALSE;
		} else {
			trig_info.trigger_type.packet_trigger.post_utxd = TRUE;
		}
		trig_info.trigger_type.packet_trigger.pkt = p;

#ifdef ULMU_DRV
		ret = wlc_ulmu_drv_trigger_request(ulmu->wlc, scb, ULMU_PACKET_TRIGGER, &trig_info);
		if (ret != BCME_OK)
			return ret;
#endif
		WLCNTINCR(ulmu_scb->pend_trig_cnt);
		ULTRIGPENDINC(ulmu->wlc);

		WL_TAFF(ulmu->wlc, MACF" pend_trig_cnt %d\n",
			ETHER_TO_MACF(scb->ea), ulmu_scb->pend_trig_cnt);

		wlc_ulmu_scb_reqbytes_decr(ulmu, scb, bytes);
		return ret;
	} else {
		return BCME_BADLEN;
	}
} /* wlc_ulmu_send_trigger */

bool
wlc_ulmu_taf_bulk(void* ulmuh, int tid, bool open)
{
	wlc_ulmu_info_t *ulmu = (wlc_ulmu_info_t *)ulmuh;

	BCM_REFERENCE(ulmu);

	if (!ulmu->num_usrs) {
		return FALSE;
	}

	if (open && tid == TAF_UL_PRIO) {
		/* open window for TAF_UL_PRIO only */
		WL_TAFN(ulmu->wlc, "opened scheduling\n");

		/*
		 * everything released from now, that scheduling is opened,
		 * should be held pending
		 */
		ulmu->bulk_commit = 1;
		/* return TRUE to indicate we accepted the open request */
		return TRUE;
	} else if (open) {
		/* TID other than TAF_UL_PRIO are not supported */
		return FALSE;
	}

	/* Up to this point in the code, the open request was handled.
	 * From here on, this means the request is to close scheduling when this function
	 * is called a second time at the end of the release window.
	 */

	/* should not get here with TID we did not accept to open */
	TAF_ASSERT(tid == TAF_UL_PRIO);

	/*
	 * everything that was pending can now be finally committed
	 */
	if (ulmu->bulk_commit > 1) {
		wlc_ulmu_post_utxd(ulmu);
	}
	ulmu->bulk_commit = 0;

	WL_TAFF(ulmu->wlc, "closed scheduling\n");

	/* return TRUE to indicate we did close the session */
	return TRUE;
} /* wlc_ulmu_taf_bulk */

//#define USE_TAF_DUR

bool wlc_ulmu_taf_release(void* ulmuh, void* scbh, void* tidh, bool force,
	taf_scheduler_public_t* taf)
{
	wlc_ulmu_info_t *ulmu = (wlc_ulmu_info_t *)ulmuh;
	wlc_info_t* wlc = ulmu->wlc;
	int nreleased = 0;
	scb_ulmu_t * ulmu_scb = (scb_ulmu_t *)scbh;
	scb_t * scb;
	wlc_ulpkttag_t *tag = NULL;
	int ret = BCME_OK;
	uint32 taf_pkt_time_units;
	uint32 taf_units_to_fill;
	uint32 taf_units_to_rel;
	uint32 taf_units_avail_to_rel;
	void *p;
	osl_t *osh = wlc->osh;
	uint32 avg_pkt_sz;
	uint32 pkt_cnt;
	uint32 partial_count;

	if (!ulmu_scb) {
		WL_ERROR(("wl%d: %s: no cubby!\n",  WLCWLUNIT(wlc), __FUNCTION__));
		TAF_ASSERT(0);
		taf->complete = TAF_REL_COMPLETE_ERR;
		return FALSE;
	}

	if (taf->how != TAF_RELEASE_LIKE_IAS) {
		WL_ERROR(("wl%d: %s: !TAF_RELEASE_LIKE_IAS\n",  WLCWLUNIT(wlc), __FUNCTION__));
		TAF_ASSERT(0);
		taf->complete = TAF_REL_COMPLETE_ERR;
		return FALSE;
	}

	if (wlc->down_override)
		return FALSE;

	avg_pkt_sz = ulmu_scb->avg_pkt_size;
	scb = ulmu_scb->scb_bl;
	if (scb == NULL) {
		WL_ERROR(("wl%d: %s: SCB is null\n", WLCWLUNIT(wlc), __FUNCTION__));
		TAF_ASSERT(0);
		taf->complete = TAF_REL_COMPLETE_ERR;
		return FALSE;
	}

	if (!SCB_ULOFDMA_ADM(scb) || ulmu_scb->state != ULMU_SCB_ADMT) {
		WL_ERROR(("wl%d: %s: SCB:"MACF" not admitted for uplink, scb state %d\n",
				WLCWLUNIT(wlc), __FUNCTION__, ETHER_TO_MACF(scb->ea),
				ulmu_scb->state));
		taf->ias.was_emptied = TRUE;
		taf->complete = TAF_REL_COMPLETE_EMPTIED;
		TAF_ASSERT(0);
		return FALSE;
	}

	if (taf->ias.is_ps_mode) {
		/* regardless set emptied flag as the available traffic (ie in PS) is none
		 * so this is effectively empty
		 */
		taf->ias.was_emptied = TRUE;

		taf->complete = TAF_REL_COMPLETE_PS;

		/* need to cancel UTXDs??  */
		return FALSE;
	}

	if ((pkt_cnt = ulmu_scb->pktq_len) == 0) {
		taf->ias.was_emptied = TRUE;
		ulmu_scb->emptied = TRUE;
		taf->complete = TAF_REL_COMPLETE_EMPTIED;

		return FALSE;
	}

	WL_TAFN(wlc,
		MACF" rate: pkt %d byte %d release units: actual %d "
		"total %d rel_limit %d time_limit %d\n",
		ETHER_TO_MACF(scb->ea), taf->ias.pkt_rate, taf->ias.byte_rate,
		taf->ias.actual.released_units, taf->ias.total.released_units,
		taf->ias.released_units_limit, taf->ias.time_limit_units);

	taf_pkt_time_units =
		TAF_PKTBYTES_TO_UNITS((uint16)(avg_pkt_sz), taf->ias.pkt_rate, taf->ias.byte_rate);

	if (taf_pkt_time_units < taf->ias.min_mpdu_dur_units) {
		taf_pkt_time_units = taf->ias.min_mpdu_dur_units;
	}

	if (taf_pkt_time_units == 0) {
		WL_INFORM(("%s: taf_pkt_time_units = %d, pktbytes = %u, pkt_rate = %u, "
			  "byte_rate = %u \n", __FUNCTION__, taf_pkt_time_units,
			  avg_pkt_sz, taf->ias.pkt_rate, taf->ias.byte_rate));
		taf_pkt_time_units = 1;
	}
	WL_TAFN(wlc, MACF" taf_pkt_time_units = %d, pktbytes = %u, pkt_rate = %u, "
		"byte_rate = %u\n",
		ETHER_TO_MACF(scb->ea), taf_pkt_time_units, avg_pkt_sz,
		taf->ias.pkt_rate, taf->ias.byte_rate);

	taf_units_to_fill = taf->ias.time_limit_units - taf->ias.total.released_units;

	if ((taf->ias.released_units_limit > 0) &&
		(taf_units_to_fill >
		(taf->ias.released_units_limit - taf->ias.actual.released_units))) {

		taf_units_to_fill = taf->ias.released_units_limit - taf->ias.actual.released_units;
	}

	if (taf_units_to_fill <= 0) {
		WL_ERROR(("%s: taf_pkt_units_to_fill = %d, actual.released_units = %u, "
			"total.released_units %u, time_limit_units %u\n",
			__FUNCTION__, taf_units_to_fill, taf->ias.actual.released_units,
			taf->ias.total.released_units, taf->ias.time_limit_units));
		TAF_ASSERT(!(taf_units_to_fill <= 0));

		taf->complete = TAF_REL_COMPLETE_ERR;
		return FALSE;
	}
	WL_TAFN(wlc, MACF" taf_pkt_units_to_fill = %d, actual.released_units = %u, "
		"total.released_units %u, time_limit_units %u\n",
		ETHER_TO_MACF(scb->ea), taf_units_to_fill, taf->ias.actual.released_units,
		taf->ias.total.released_units, taf->ias.time_limit_units);

	taf_units_avail_to_rel = pkt_cnt * taf_pkt_time_units;

	if (taf_units_avail_to_rel > taf_units_to_fill) {
		taf_units_to_rel = taf_units_to_fill;

		taf->ias.was_emptied = FALSE;

		taf->complete = (taf_units_to_rel == taf->ias.released_units_limit) ?
			TAF_REL_COMPLETE_REL_LIMIT : TAF_REL_COMPLETE_TIME_LIMIT;
	} else {
		if (!taf->ias.force || taf->ias.released_units_limit == 0) {
			taf_units_to_rel = taf_units_avail_to_rel;
		} else {
			taf_units_to_rel = taf_units_to_fill;
		}
		taf->ias.was_emptied = TRUE;
		ulmu_scb->emptied = TRUE;
		taf->complete = TAF_REL_COMPLETE_EMPTIED;
	}

	nreleased = (taf_units_to_rel / taf_pkt_time_units);
	partial_count = nreleased;

	while (partial_count) {
		uint32 partial_release_pkts = partial_count;
		uint32 partial_release_units;

		if (ulmu->max_aggn_release && partial_release_pkts > ulmu->max_aggn_release) {
			partial_release_pkts = ulmu->max_aggn_release;
		}

		partial_release_units = partial_release_pkts * taf_pkt_time_units;

		WL_TAFN(wlc, MACF" nrel/partial %u:%u, partial pkts %u\n",
			ETHER_TO_MACF(scb->ea),
			nreleased, partial_count, partial_release_pkts);
#ifdef UTXD_POOL
		p = pktpool_get(wlc->pub->pktpool_utxd);
#else
		p = PKTGET(osh, ULMU_UTXD_BUF_SIZE, TRUE);
#endif
		if (!p) {
			WL_ERROR(("wl%d: %s: pktget error\n",
				wlc->pub->unit, __FUNCTION__));
			return 0;
		}
		ASSERT(ISALIGNED((uintptr)PKTDATA(osh, p), sizeof(uint32)));

#if defined(BCMHWA) && defined(HWA_PKT_MACRO)
		PKTSETHWAPKT(osh, p);
		PKTSETMGMTTXPKT(osh, p);
#endif /* BCMHWA && HWA_PKT_MACRO */

		memset((uint8 *)PKTDATA(osh, p), 0, WLULPKTTAGLEN);

		/* construct the tag */
		tag = (wlc_ulpkttag_t*)PKTDATA(wlc->osh, p);
		tag->ref_cnt = ULMU_UTXD_MAX_REF_CNT;

		tag->index = taf->ias.index;
		tag->units = partial_release_units;
		tag->flags3 = WLF3_TAF_TAGGED;
		tag->req_trig_bytes = partial_release_pkts * avg_pkt_sz;
		tag->req_trig_us = TAF_UNITS_TO_MICROSEC(partial_release_units);
		tag->req_trig_units = partial_release_units;
		tag->compl_trig_units = 0;
		tag->trig_ts = taf->ias.timestamp;

		PKTPULL(osh, p, WLULPKTTAGLEN);

		WL_TAFN(wlc, MACF" %u: idx %d, req bytes %d, req_dur %d, units %d\n",
			ETHER_TO_MACF(scb->ea), tag->trig_ts, tag->index,
			tag->req_trig_bytes, tag->req_trig_us, tag->units);

		if (ulmu->bulk_commit) {
#ifndef BYTE_INTERFACE
			ret = wlc_ulmu_send_trigger(ulmu, scb, tag->req_trig_bytes,
				tag->req_trig_us, p, TRUE);
#else
			ret = wlc_ulmu_send_trigger(ulmu, scb, tag->req_trig_bytes, p, TRUE);
#endif
			if (ret == BCME_OK) {
				ulmu->bulk_commit++;
			}
		} else {
#ifndef BYTE_INTERFACE
			ret = wlc_ulmu_send_trigger(ulmu, scb, tag->req_trig_bytes,
				tag->req_trig_us, p, FALSE);
#else
			ret = wlc_ulmu_send_trigger(ulmu, scb, tag->req_trig_bytes, p, FALSE);
#endif
		}

		if (ret != BCME_OK) {
			/* already called PKTFREE in wlc_ulmu_post_utxd()
			 * when wlc_bmac_dma_txfast() return BCME_BUSY error
			 */
			if (ret != BCME_BUSY) {
				PKTFREE(osh, p, TRUE);
			}
			taf->ias.was_emptied = TRUE;
			taf->complete = TAF_REL_COMPLETE_ERR;
			WL_INFORM(("wl%d: %s: "MACF" trigger request failed with error %d "
				"pend_trig_cnt %d\n",
				WLCWLUNIT(wlc), __FUNCTION__, ETHER_TO_MACF(scb->ea), ret,
				ulmu_scb->pend_trig_cnt));

			return FALSE;
		}

		/* clear any trigger stall condition */
		ulmu_scb->trig_stall = FALSE;

		taf->ias.actual.released_bytes += partial_release_pkts * avg_pkt_sz;
		taf->ias.actual.released_units += partial_release_units;
		taf->ias.actual.release += partial_release_pkts;

		partial_count -= partial_release_pkts;

		/* update stats */
		WLCNTINCR(ULMU_TAF_SCB_STATS(ulmu_scb)->nutxd);
		WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->bytes_rel, tag->req_trig_bytes);
#ifndef BYTE_INTERFACE
		WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->usec_trigd, tag->req_trig_us);
#else
		WLCNTADD(ULMU_TAF_SCB_STATS(ulmu_scb)->usec_trigd,
			TAF_UNITS_TO_MICROSEC(partial_release_units));
#endif

		WL_TAFF(wlc, MACF" released %u, bytes %d, pkt_cnt %d, index %d, "
			"peak_aggn %u, avg pkt %u, "
			"units %d, dur %d, total_pend_trig %d, pend_trig_cnt %d\n",
			ETHER_TO_MACF(scb->ea), partial_release_pkts, tag->req_trig_bytes, pkt_cnt,
			taf->ias.index, ulmu_scb->peak_aggn, avg_pkt_sz, tag->req_trig_units,
			tag->req_trig_us, ULTRIGPENDTOT(ulmu->wlc),
			WLCNTVAL(ulmu_scb->pend_trig_cnt));
	}

	return nreleased > 0;
} /* wlc_ulmu_taf_release */

static bool wlc_ulmu_bump_bytecnt(wlc_ulmu_info_t *ulmu, scb_ulmu_t* ulmu_scb)
{
	wlc_info_t *wlc = ulmu->wlc;
	scb_t *scb = ulmu_scb->scb_bl;
	bool bumped = FALSE;

	uint32 taf_kbits_sched_rate;
	uint32 taf_kbits_rate;
	uint32 taf_kbits_phy = 0;
	int32  taf_decipkts = 0;

	if (!SCB_ULOFDMA_ADM(scb) || ulmu_scb->state == ULMU_SCB_INIT) {
		TAF_ASSERT(0);
		return FALSE;
	}

	if (!ulmu_scb->emptied) {
		WL_TAFF(wlc, MACF" not emptied\n", ETHER_TO_MACF(scb->ea));
		return FALSE;
	}

	if (ulmu_scb->trig_wait_state != ULMU_TRIG_WAIT_OFF) {
		WL_TAFF(wlc, MACF" experiencing qosnull\n", ETHER_TO_MACF(scb->ea));
		return FALSE;
	}

	/* the following will 'get' the Rx scheduling rate in Kbits/second */
	wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, 0,
		(void *)&taf_kbits_sched_rate, TAF_RXPKT_STATUS_GET_SCHED_RATE);

	taf_kbits_rate = ulmu_scb->rate << 3;

	/* the following will 'get' the Rx phy rate in Kbits/second */
	wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, 0,
		(void *)&taf_kbits_phy, TAF_RXPKT_STATUS_GET_PHY_RATE);

	if (taf_kbits_sched_rate <= taf_kbits_rate) {

		bumped = TRUE;

	} else if ((taf_kbits_sched_rate - taf_kbits_rate) <
		((taf_kbits_rate * ulmu->qlen_incr_over) >> 8)) {

		bumped = TRUE;

	} else if (ulmu->qlen_incr_over_phy && ((taf_kbits_sched_rate - taf_kbits_rate) <
		((taf_kbits_phy * ulmu->qlen_incr_over_phy) >> 8))) {

		bumped = TRUE;

	} else if (ulmu->qlen_incr_over_deci > 0) {
		/* the following will 'get' the deci-pkts overscheduling rate */
		wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, 0,
			(void *)&taf_decipkts, TAF_RXPKT_STATUS_GET_SCHED_DPKT_OVER);

		if (taf_decipkts < ulmu->qlen_incr_over_deci) {
			bumped = TRUE;
		}
	}

	if (bumped) {
		uint32 est_traffic_ms;

		if (taf_kbits_phy == 0) {
			WL_TAFF(wlc, MACF" phy rate 0\n", ETHER_TO_MACF(scb->ea));
			return FALSE;
		}

		est_traffic_ms = (ulmu_scb->acc_bufsz * 8) / taf_kbits_phy;

		if (est_traffic_ms < (ulmu->qlen_incr_wait + 10)) {
			uint32 byte_bump_A = (taf_kbits_phy * ulmu->qlen_incr_amount) >> 5;
			uint32 byte_bump_B = (ulmu_scb->peak_aggn * ulmu_scb->avg_pkt_size *
				ulmu->qlen_incr_factor);
			uint32 byte_bump = MIN(byte_bump_A, byte_bump_B);

			ulmu_scb->acc_bufsz += byte_bump;

			WL_TAFF(wlc, MACF" bumped %u bytes (%u, %u, %u, %u, %d, a %u, b %u), "
				"new qlen %u\n",
				ETHER_TO_MACF(scb->ea), byte_bump, est_traffic_ms,
				taf_kbits_rate, taf_kbits_sched_rate, taf_kbits_phy, taf_decipkts,
				byte_bump_A, byte_bump_B,
				ulmu_scb->acc_bufsz / ulmu_scb->avg_pkt_size);
		} else {
			bumped = FALSE;
			WL_TAFF(wlc, MACF" enough; not bumped (%u, %u, %u, %u, %d), qlen %u\n",
				ETHER_TO_MACF(scb->ea), est_traffic_ms,
				taf_kbits_rate, taf_kbits_sched_rate, taf_kbits_phy, taf_decipkts,
				ulmu_scb->acc_bufsz / ulmu_scb->avg_pkt_size);
		}
	} else {
		WL_TAFF(wlc, MACF" not bumped (%u, %u, %u, %d), qlen %u\n",
			ETHER_TO_MACF(scb->ea),
			taf_kbits_rate, taf_kbits_sched_rate, taf_kbits_phy, taf_decipkts,
			ulmu_scb->acc_bufsz / ulmu_scb->avg_pkt_size);
	}
	ulmu_scb->emptied = FALSE;
	return bumped;

} /* wlc_ulmu_bump_bytecnt */

static void
wlc_ulmu_taf_enable(wlc_ulmu_info_t *ulmu, scb_t *scb, bool enable)
{
	wlc_info_t * wlc;

	if (!scb || !ulmu) {
		return;
	}

	wlc = ulmu->wlc;

	if (enable) {
		wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NO_SOURCE, TAF_PARAM(TRUE),
			TAF_SCBSTATE_MU_UL_OFDMA);

		wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_UL, NULL,
			TAF_SCBSTATE_SOURCE_ENABLE);

		wlc_taf_link_state(ulmu->wlc->taf_handle, scb, TAF_UL_PRIO, TAF_UL,
			TAF_LINKSTATE_ACTIVE);

	} else {
		wlc_taf_link_state(wlc->taf_handle, scb, TAF_UL_PRIO, TAF_UL,
			TAF_LINKSTATE_NOT_ACTIVE);

		wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_UL, NULL,
			TAF_SCBSTATE_SOURCE_DISABLE);

		wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NO_SOURCE, TAF_PARAM(FALSE),
			TAF_SCBSTATE_MU_UL_OFDMA);
	}
} /* wlc_ulmu_taf_enable */
#endif /* WLTAF */

/* set ul ofdma admission params for TWT setting */
void
wlc_ulmu_twt_params(wlc_ulmu_info_t *ulmu, bool on)
{

	WL_ULO(("wl%d: %s: on is %d\n",
		ulmu->wlc->pub->unit, __FUNCTION__, on));
	if (on) {
		/* For TWT
		 * - admit even 1 user as ul ofdma
		 * - set the trigger interval to 1ms
		 * - guarantee to tx at least 1 trigger per interval
		 * - tx max 2 triggers per interval
		 */
		ulmu->min_ulofdma_usrs = 1;
		ulmu->is_start = TRUE;

#ifdef TESTBED_AP_11AX
		/* Restore the settings which got updated by wfa_test IOVAR */
		ulmu->txd.macctl1 = (ULMU_OFF << D11_ULOTXD_MACTL1_DSCHED_SHIFT) |
			(ULMU_ON << D11_ULOTXD_MACTL1_HBRNT_SHIFT |
			ULMU_ON << D11_ULOTXD_MACTL1_USCHED_SHIFT);
		ulmu->txd.interval = ULMU_TRIGGER_INTVL_WFA;
		ulmu->rx_pktcnt_thrsh = 0;
		ulmu->txd.burst = 2;
		ulmu->txd.maxtw = 5;
		ulmu->txd.interval = -1;
#endif /* TESTBED_AP_11AX */

		wlc_ulmu_start(ulmu, ulmu->is_start);
	} else {
		/* there is no trig based twt user so restore default settings */
		ulmu->min_ulofdma_usrs = ULMU_USRCNT_MIN;
	}
	if (ulmu->wlc->pub->up) {
		wlc_ulmu_cfg_commit(ulmu);
	}
}

static bool
wlc_ulmu_sta_admittable(wlc_ulmu_info_t *ulmu, scb_t *scb, uint32 delay)
{
	wlc_info_t *wlc = ulmu->wlc;
	scb_ulmu_t *ulmu_scb = SCB_ULMU(ulmu, scb);
#ifdef WLTAF
	uint32 pps = 0;
#endif

	ASSERT(ulmu_scb);

	if (ulmu->always_admit) {
		return TRUE;
	}

	/* All TWT trigger enabled SCBs should be admitted */
	if (wlc_twt_scb_is_trig_enab(wlc->twti, scb)) {
		return TRUE;
	}
	/* Should this STA be tried to admit? */
	if (!ulmu_scb->try_admit) {
		return FALSE;
	}
	if (!wlc_ulmu_scb_eligible(ulmu, scb)) {
		return FALSE;
	}

#ifdef WLTAF
	if (wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, 0,
		(void *)&pps, TAF_RXPKT_STATUS_GET_RX_PKT_RATE)) {
		if (pps < ulmu->rx_pktcnt_thrsh) {
			return FALSE;
		}

		if (delay < ulmu->admit_delay * 1000) {
			return FALSE;
		}
	} else {
		if (!ulmu_scb->rx_data) {

			return FALSE;
		}
	}
#else
	if (!ulmu_scb->rx_data) {

		return FALSE;
	}
#endif /* WLTAF */

	if (!wlc_scb_ampdurx_on(scb) && ulmu->rx_pktcnt_thrsh) {
		return FALSE;
	}

	return TRUE;
}

static bool
wlc_ulmu_admit_ready(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	wlc_info_t *wlc = ulmu->wlc;
	scb_t *tmpscb;
	scb_iter_t scbiter;
	int admit_cnt = 0;
	scb_ulmu_t *ulmu_scb, *tmp_ulmu_scb;
	ulmu_stats_t *ul_stats;
	uint32 ts;
#ifdef WLTAF
	uint32 pps = 0;
	bool taf_pps = FALSE;
#endif
	uint8 bw = wlc_scb_ratesel_get_ulmu_link_bw(wlc, scb);

	if ((ulmu_scb = SCB_ULMU(ulmu, scb)) == NULL) {
		return FALSE;
	}

	if (!wlc_ulmu_scb_eligible(ulmu, scb)) {
		WL_INFORM(("%s: SCB:"MACF" admit fail: not eligible\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea)));
		return FALSE;
	}

	if (ulmu->num_usrs >= wlc_txcfg_max_clients_get(wlc->txcfg, ULOFDMA)) {
		/* reset admit check flag */
		ulmu_scb->try_admit = FALSE;
		WL_INFORM(("%s: SCB:"MACF" admit fail: max users %d already admitted\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea), ulmu->num_usrs));
		return FALSE;
	}

	ts = wlc_read_usec_timer(wlc);

	if (wlc_twt_scb_is_trig_enab(wlc->twti, scb) ||
		ulmu->always_admit) {
		/* If not yet admitted then continue, otherwise no update */
		if (ulmu_scb->state != ULMU_SCB_ADMT) {
			goto skip_time_pkt_check;
		}
		return FALSE;
	}

#ifdef WLTAF
	taf_pps = wlc_taf_rxpkt_status(wlc->taf_handle, scb, TAF_UL_PRIO, 0,
			(void *)&pps, TAF_RXPKT_STATUS_GET_RX_PKT_RATE);

	if (taf_pps) {
		if (pps < ulmu->rx_pktcnt_thrsh) {
			/* reset admit check flag */
			ulmu_scb->try_admit = FALSE;
			WL_INFORM(("%s: SCB:"MACF" not admit ready: pps %d below threshold\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), pps));

			return FALSE;
		}
	}
#endif

	if (!ulmu_scb->try_admit) {
		ulmu_scb->start_ts = ts;
		ulmu_scb->try_admit = TRUE;
#ifdef WLTAF
		if (!taf_pps) {
			wlc_ampdu_ulmu_reqbytes_get(wlc->ampdu_rx, scb); /* reset recv_bytes */
			ulmu_scb->last_rx_pkts =
				scb->scb_stats.rx_ucast_pkts; /* refresh pkt counter */
		}
#else
		wlc_ampdu_ulmu_reqbytes_get(wlc->ampdu_rx, scb); /* reset recv_bytes */
		ulmu_scb->last_rx_pkts = scb->scb_stats.rx_ucast_pkts; /* refresh pkt counter */
#endif /* WLTAF */
		WL_INFORM(("%s: SCB:"MACF" admit check start ts %d\n", __FUNCTION__,
			ETHER_TO_MACF(scb->ea), ulmu_scb->start_ts));
		return FALSE;
	}

#ifdef WLTAF
	if (taf_pps) {
		/* check for admission delay time */
		if (ts - ulmu_scb->start_ts < (ulmu->admit_delay * 1000)) {
			WL_INFORM(("%s: SCB:"MACF" admit fail: admit delay not yet reached %d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), admit_cnt));
			return FALSE;
		}
	} else {
		if ((scb->scb_stats.rx_ucast_pkts - ulmu_scb->last_rx_pkts) <
			ulmu->rx_pktcnt_thrsh) {
			WL_INFORM(("%s: SCB:"MACF" rx pkt cnt %d not above threshold\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea),
				scb->scb_stats.rx_ucast_pkts - ulmu_scb->last_rx_pkts));
			return FALSE;
		}

		/* reset admit check if last pkt arrived more than 1 second earlier */
		if (ts - ulmu_scb->start_ts > US_PER_SECOND) {
			 ulmu_scb->start_ts = ts;
			/* reset admit check flag */
			ulmu_scb->try_admit = FALSE;
			WL_INFORM(("%s: SCB:"MACF" admit fail: time expired %d\n",
				__FUNCTION__, ETHER_TO_MACF(scb->ea), admit_cnt));
			return FALSE;
		}
	}
#else
	if ((scb->scb_stats.rx_ucast_pkts - ulmu_scb->last_rx_pkts) <
		ulmu->rx_pktcnt_thrsh) {
		WL_INFORM(("%s: SCB:"MACF" rx pkt cnt %d not above threshold\n", __FUNCTION__,
			ETHER_TO_MACF(scb->ea),
			scb->scb_stats.rx_ucast_pkts - ulmu_scb->last_rx_pkts));
		return FALSE;
	}

	/* reset admit check if last pkt arrived more than 1 second earlier */
	if (ts - ulmu_scb->start_ts > US_PER_SECOND) {
		 ulmu_scb->start_ts = ts;
		/* reset admit check flag */
		ulmu_scb->try_admit = FALSE;
		WL_INFORM(("%s: SCB:"MACF" admit fail: time expired %d\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea), admit_cnt));
		return FALSE;
	}
#endif /* WLTAF */

skip_time_pkt_check:
	/* Skip loop check if new SCB still won't make enough users for the BSS */

	/* count total users that can be admitted or are already admitted */
	FOREACHSCB(wlc->scbstate, &scbiter, tmpscb) {
		if (ulmu->per_bw_admn && (bw != wlc_scb_ratesel_get_ulmu_link_bw(wlc, tmpscb))) {
			continue;
		}

		/* Already admitted? Count immediately */
		if (SCB_ULOFDMA_ADM(tmpscb)) {
			admit_cnt++;
			continue;
		}
		tmp_ulmu_scb = SCB_ULMU(ulmu, tmpscb);
		if (!tmp_ulmu_scb) {
			continue;
		}
#ifdef WLTAF
		if (!taf_pps) {
			/* Update try_admit. */
			if (tmp_ulmu_scb->try_admit &&
				((ts - tmp_ulmu_scb->start_ts) > US_PER_SECOND)) {
				tmp_ulmu_scb->try_admit = FALSE;
			}
			/* update rx_data */
			tmp_ulmu_scb->rx_data =
				((tmpscb->scb_stats.rx_ucast_pkts - tmp_ulmu_scb->last_rx_pkts) >=
				ulmu->rx_pktcnt_thrsh);
		}
#else
		/* Update try_admit. */
		if (tmp_ulmu_scb->try_admit && ((ts - tmp_ulmu_scb->start_ts) > US_PER_SECOND)) {
			tmp_ulmu_scb->try_admit = FALSE;
		}
		/* update rx_data */
		tmp_ulmu_scb->rx_data =
			((tmpscb->scb_stats.rx_ucast_pkts - tmp_ulmu_scb->last_rx_pkts) >=
			ulmu->rx_pktcnt_thrsh);
#endif /* WLTAF */
		if (wlc_ulmu_sta_admittable(ulmu, tmpscb, (ts - tmp_ulmu_scb->start_ts))) {
			admit_cnt++;
		}
	}

	if (admit_cnt < ulmu->min_ulofdma_usrs) {
#ifndef WLTAF
		/* reset admit check flag */
		ulmu_scb->try_admit = FALSE;
#endif
		WL_INFORM(("%s: SCB:"MACF" admit fail: low admit cnt %d\n",
			__FUNCTION__, ETHER_TO_MACF(scb->ea), admit_cnt));
		return FALSE;
	}

	/* admission */
	FOREACHSCB(wlc->scbstate, &scbiter, tmpscb) {
		tmp_ulmu_scb = SCB_ULMU(ulmu, tmpscb);
		if (!tmp_ulmu_scb) {
			continue;
		}

		if (ulmu->per_bw_admn && (bw != wlc_scb_ratesel_get_ulmu_link_bw(wlc, tmpscb))) {
			continue;
		}

		if (SCB_ULOFDMA_ADM(tmpscb)) {
			continue;
		}

		if (!wlc_ulmu_sta_admittable(ulmu, tmpscb, (ts - tmp_ulmu_scb->start_ts))) {
			continue;
		}

		tmp_ulmu_scb->su_recv_dur = (ts - tmp_ulmu_scb->start_ts);
		tmp_ulmu_scb->su_recv_bytes = wlc_ampdu_ulmu_reqbytes_get(wlc->ampdu_rx, tmpscb);
		tmp_ulmu_scb->try_admit = FALSE;
		WL_INFORM(("%s: SCB:"MACF" ready for admit, start %d end %d dur %d link_bw %d\n",
			__FUNCTION__, ETHER_TO_MACF(tmpscb->ea), tmp_ulmu_scb->start_ts, ts,
			tmp_ulmu_scb->su_recv_dur, bw));

		/* let's admit if not admitted yet */
		if (!wlc_ulmu_admit_client(ulmu, tmpscb, ADMIT_CLIENT)) {
			WL_ERROR(("%s: SCB:"MACF" admission failed\n",
				__FUNCTION__, ETHER_TO_MACF(tmpscb->ea)));
			wlc_ulmu_oper_state_upd(ulmu, tmpscb, ULMU_SCB_INIT);
		} else {
			/* update admission time stats */
			ul_stats = ULMU_SCB_STATS(tmp_ulmu_scb);
			WLCNTADD(ul_stats->admit_dur,
				ROUNDUP(tmp_ulmu_scb->su_recv_dur, 1000) / 1000);
		}
	}

	return TRUE;
} /* wlc_ulmu_admit_ready */

/* wlc_ulmu_alloc_fifo() is used by DWDS clients to allocate uplink OFDMA FIFO.
 * In case of re-association use existing FIFO.
 * If there are less than 4 FIFOs available, evict all MU clients to free FIFO.
 * Restriction from Ucode:
 *	For run time optimization in micro code, driver has to make sure that
 *	4 FIFOs allocated for ULMU falls in the same range of 16 FIFO blocks
 *	within MU FIFO space. Meaning, all 4 FIFOs will be in the range of
 *	[0-15] or [16-31] or [32-47] or [48-63] ...
 *	Shared memory write will include a 16-bit bitmap and bank number
 *	indicating which block of 16 FIFOs includes ULMU FIFOs.
 *	This determistic behaviour require eviction of all MU clients on repeater's
 *	AP interface.
*/
void
wlc_ulmu_alloc_fifo(wlc_info_t *wlc, scb_t *scb)
{
	int i;
	uint16 fifo_bitmap = 0;
	uint16 block = 0;
	wlc_bsscfg_t *bsscfg = scb->bsscfg;

	/* In case of Reassociation we can keep previously allocated FIFO
	 * Check only AC_BE FIFO. It will be sufficient
	 */
	if (wlc_fifo_isMU(wlc->fifo, scb, AC_BE))
		return;

	/* Evict all mu clients to free FIFOs */
	wlc_mutx_evict_all_muclients(wlc->mutx);
	ASSERT(wlc_fifo_global_remaining(wlc->fifo) >= AC_COUNT);

	for (i = 0; i < AC_COUNT; i++) {
		wlc_fifo_alloc(wlc->fifo, scb, WL_MU_ULOFDMA, i);
	}

	/* Create FIFO bitmap and write it in shared memory */
	block = wlc_fifo_sta_ulo_bitmap(wlc->fifo, scb, &fifo_bitmap);

	if (wlc->clk) {
		edcf_acparam_t *acp = &bsscfg->wme->wme_param_ie.acparam[AC_BK];
		wlc_write_shmx(wlc, MX_ULOFIFO_BMP(wlc), fifo_bitmap);
		wlc_write_shmx(wlc, MX_ULOFIFO_BASE(wlc), block);
		wlc_write_shm(wlc, M_ULTX_ACMASK(wlc), 1);

		/* WAR JIRA: BCAWLAN-223309. Applies to DWDS station with ULOFDMA cap.
		* Because of limited number of FFQs ucode has to mux all ULOFDMA traffic from
		* DWDS's station interface to a single AC which is AC_BK. To address throughput
		* drop in majority of testcases we need to replace AC param of AC_BK with param
		* of AC_BE.
		*/
		wlc->ul_ofdma_repeater_ac_param_mod = TRUE;
		wlc_edcf_acp_set_one(wlc, bsscfg, acp, TRUE);
	}
	return;
} /* wlc_ulmu_alloc_fifo */

static void
wlc_ulmu_utxd_stats_upd(wlc_ulmu_info_t *ulmu, uint32 cmd, bool incr)
{
	switch (cmd) {
		case ULMU_UTXD_GLBUPD:
			ulmu->utxd_in_transit[UTXD_TYPE_GLBUPD] =
				incr ? (ulmu->utxd_in_transit[UTXD_TYPE_GLBUPD] + 1):
				(ulmu->utxd_in_transit[UTXD_TYPE_GLBUPD] - 1);
			break;
		case ULMU_UTXD_USRADD_ALL:
			ulmu->utxd_in_transit[UTXD_TYPE_USRADD_ALL] =
				incr ? (ulmu->utxd_in_transit[UTXD_TYPE_USRADD_ALL] + 1) :
				(ulmu->utxd_in_transit[UTXD_TYPE_USRADD_ALL] - 1);
			break;
		case ULMU_UTXD_USRUPD_RATE:
			ulmu->utxd_in_transit[UTXD_TYPE_USRUPD_RATE] =
				incr ? (ulmu->utxd_in_transit[UTXD_TYPE_USRUPD_RATE] + 1) :
				(ulmu->utxd_in_transit[UTXD_TYPE_USRUPD_RATE] - 1);
			break;
		case ULMU_UTXD_USRUPD_R2C:
			ulmu->utxd_in_transit[UTXD_TYPE_USRUPD_DATA] =
				incr ? (ulmu->utxd_in_transit[UTXD_TYPE_USRUPD_DATA] + 1) :
				(ulmu->utxd_in_transit[UTXD_TYPE_USRUPD_DATA] - 1);
			break;
		case ULMU_UTXD_USRDEL:
			ulmu->utxd_in_transit[UTXD_TYPE_USRDEL] =
				incr ? (ulmu->utxd_in_transit[UTXD_TYPE_USRDEL] + 1) :
				(ulmu->utxd_in_transit[UTXD_TYPE_USRDEL] - 1);
			break;
		default:
			WL_ERROR(("wlc_ulmu_utxd_stats_upd: unrecognised utxd command type: %d\n",
				cmd));
			break;
	}
}

/*
 * wlc_ulmu_evaluate_admission - Use this function to (re-) evaluate the admission of a specific
 * SCB. When SCB is admitted it is checked if still eligible, if not then it is evicted. If it is
 * currently not admitted then see if it is eligble, if so, then try admission. This function can
 * for example be called when OMI has been received with an updated ULMU configuration.
 */
void
wlc_ulmu_evaluate_admission(wlc_ulmu_info_t *ulmu, scb_t *scb)
{
	/* Is it currently admitted? */
	if (SCB_ULOFDMA_ADM(scb)) {
		if (!wlc_ulmu_scb_eligible(ulmu, scb)) {
			/* Remove this SCB at once, if additional evictions are needed then this
			 * will happen in the WD routine.
			 */
			wlc_ulmu_admit_client(ulmu, scb, EVICT_CLIENT);
		}
	} else {
		/* Use the existing function admit ready, to do complete evaluation of this and
		 * other SCBs to see if admission should take place.
		 */
		wlc_ulmu_admit_ready(ulmu, scb);
	}
}

#endif /* WL_ULMU */
