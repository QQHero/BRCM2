/**
 * +--------------------------------------------------------------------------+
 * wlc_csimon.c
 *
 * Implementation of Channel State Information Monitor (CSIMON)
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_csimon.c 810648 2022-04-12 05:26:05Z $
 *
 * vim: set ts=4 noet sw=4 tw=80:
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * +--------------------------------------------------------------------------+
 */

/**
 * Theory of operation:
 * ====================
 *
 * The consumer side of the Channel State Information Monitor
 * (CSIMON) includes reading or consuming the CSI
 * records/structures from the circular ring in the host memory
 * and making them available to the user-space application. A
 * timer is invoked periodically to consume the record and send
 * over a netlink socket or write to a file.
 *
 * In case of a netlink socket, the consumer multicasts each
 * record over to a multicast group. The user application needs
 * to become a member of that group to receive the CSI record.
 *
 * In case of a file output, the consumer writes the CSI records
 * in a file /var/csimon sequentially as they are read/consumed
 * from the circular ring. The file has a size limit. Once the
 * limit is reached, the next records are not copied and they
 * are essentially dropped. So it is user's responsibility to
 * copy the file out of the AP and remove it. The consumer will
 * then create a new file of the same name and start populating
 * the next CSI records.
 *
 * CSI record generation:
 * The driver sends a null QoS frame to each client in the
 * CSIMON client list periodically. When a client responds with
 * an ACK, the PHY layer computes the CSI(H-matrix) based on its
 * PHY preamble. The CSI is stored in SVMP memory. During
 * TxStatus processing, the driver recognizes the Null frame's
 * ACK and copies the CSI report from the SVMP memory to the
 * host memory in the circular ring of the CSI records. In
 * addition to the CSI report, it also copies additional
 * information, called CSI header, like RSSI, MAC address, as a
 * part of the overall CSI record.
 *
 */
#include <osl.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <bcmpcie.h>
#include <sbtopcie.h>
#if defined(DONGLEBUILD)
#include <bcmhme.h>
#include <rte_isr.h>
#include <sbtopcie.h>
#else /* ! DONGLEBUILD */
#include <linux/netlink.h>
#endif /* ! DONGLEBUILD */
#include <bcm_ring.h>
#include <pcicfg.h>
#include <wl_dbg.h>
#include <wl_export.h>
#include <wlc.h>
#include <wlc_types.h>
#include <wlc_scb.h>
#include <wlc_rspec.h>
#include <wlc_stf.h>
#include <wlc_dump.h>
#include <wlc_csimon.h>
#include <wlc_hrt.h>
/* Compile time pick either hndm2m or sb2pcie based transfer of CSI structure */
#ifndef DONGLEBUILD
#define CSIMON_M2MCPY_BUILD        /* Asynchronous transfer with callback */
#else
#define CSIMON_M2MCPY_BUILD        /* Asynchronous transfer with callback */
#ifndef CSIMON_M2MCPY_BUILD
#define CSIMON_S2PCPY_BUILD        /* Synchronous transfer, no callback */
#endif  /* ! CSIMON_M2MCPY_BUILD */
#endif  /* ! DONGLEBUILD */

#if !defined(CSIMON_M2MCPY_BUILD) && !defined(CSIMON_S2PCPY_BUILD)
#error "Must define a xfer mechanism"
#endif

//#define CSIMON_FILE_BUILD
//#define SINGLE_M2M_XFER /* FD mode: single M2M transfer: sysmem to host mem */
//#define CSIMON_PER_STA_TIMER /* Per Station Timer Implementation */

#if  ((CSIMON_REPORT_SIZE + CSIMON_HEADER_SIZE) > CSIMON_RING_ITEM_SIZE)
#error "CSIMON header + report cannot be greater than CSIMON item size"
#endif

/* wlc access macros */
#define WLC(x) ((x)->wlc)
#define WLCPUB(x) ((x)->wlc->pub)
#define WLCUNIT(x) ((x)->wlc->pub->unit)

/* CSIMON Info xfer macro */
#define CSIxfer(x) (x)->csimon_info->xfer

/* CSIMON local ring manipulation */
#define CSIMON_RING_IDX2ELEM(ring_base, idx) \
	(((wlc_csimon_rec_t *)(ring_base)) + (idx))

/* 6Mbps ofdm rate for 2.4GHz QoS Null frame */
#define	CSIMON_NULL_FRAME_RSPEC	(OFDM_RSPEC(WLC_RATE_6M))

#if defined(DONGLEBUILD)
/* SBTOPCIE service for programmed IO access into Host Memory Extension */
typedef csimon_ipc_hme_t csimon_ipc_s2p_t;
#endif /* DONGLEBUILD */

/**
 * +--------------------------------------------------------------------------+
 *  Section: Definitions and Declarations
 * +--------------------------------------------------------------------------+
 */

#define C_CSI_IDX_NUM  2 // Number of SVMP memory-resident CSI reports

/* M_CSI_STATUS */
typedef enum
{
	C_CSI_IDX_NBIT          = 0,    // CSI index
	C_CSI_IDX_LB            = 4,
	C_CSI_ARM_NBIT          = 5     // ARM CSI capture for next RX frame
} eCsiStatusRegBitDefinitions;

#define C_CSI_IDX_BSZ          NBITS(C_CSI_IDX)
#define C_CSI_IDX_MASK         0x1F

/* C_CSI_VSEQN_POS */
typedef enum
{
	C_CSI_SEQN_NBIT         = 0,    // CSI index
	C_CSI_SEQN_LB           = 14,
	C_CSI_VLD_NBIT          = 15    // Record is valid
} eCsiValidSeqNBitDefinitions;
#define C_CSI_SEQN_BSZ         NBITS(C_CSI_SEQN)

/** csi info block: M_CSI_BLKS */
typedef enum
{
	C_CSI_ADDRL_POS         = 0,    // MAC ADDR LOW
	C_CSI_ADDRM_POS         = 1,    // MAC ADDR MED
	C_CSI_ADDRH_POS         = 2,    // MAC ADDR HI
	C_CSI_VSEQN_POS         = 3,    // Frame seq number
	C_CSI_RXTSFL_POS        = 4,    // CSI RX-TSF-time: lower 16 bits
	C_CSI_RXTSFML_POS       = 5,    // CSI RX-TSF-time: higher 16 bits
	C_CSI_RSSI0_POS         = 6,    // Ant0, Ant1 RSSI
	C_CSI_RSSI1_POS         = 7,    // Ant2, Ant3 RSSI
	C_CSI_BLK_WSZ
} eCsiInfoWordDefinitions;

#define C_CSI_BLK_SZ           (C_CSI_BLK_WSZ * 2)
#define M_CSI_MACADDRL(wlc)    (M_CSI_BLKS(wlc) + (C_CSI_ADDRL_POS * 2))
#define M_CSI_MACADDRM(wlc)    (M_CSI_BLKS(wlc) + (C_CSI_ADDRM_POS * 2))
#define M_CSI_MACADDRH(wlc)    (M_CSI_BLKS(wlc) + (C_CSI_ADDRH_POS * 2))
#define M_CSI_VSEQN(wlc)       (M_CSI_BLKS(wlc) + (C_CSI_VSEQN_POS * 2))
#define M_CSI_RXTSFL(wlc)      (M_CSI_BLKS(wlc) + (C_CSI_RXTSFL_POS * 2))
#define M_CSI_RXTSFML(wlc)     (M_CSI_BLKS(wlc) + (C_CSI_RXTSFML_POS * 2))
#define M_CSI_RSSI0(wlc)       (M_CSI_BLKS(wlc) + (C_CSI_RSSI0_POS * 2))
#define M_CSI_RSSI1(wlc)       (M_CSI_BLKS(wlc) + (C_CSI_RSSI1_POS * 2))

/** M2M callback data used by the callback function after the M2M xfer */
typedef struct csimon_m2m_cb_data {
	scb_t   *scb;		/**< pointer to the SCB */
	uint16  seqn;		/**< Sequence number in SHM - has valid bit */
	uint16  svmp_rpt_idx;	/**< CSI Report index in SVMP */
} csimon_m2m_cb_data_t;

union wlc_csimon_hdr {
	 uint8 u8[CSIMON_HEADER_SIZE];
	 struct {
	     uint32 format_id;		/**< Id/version of the CSI report format */
	     uint16 client_ea[3];	/**< client MAC address - 3 16-bit words */
	     uint16 bss_ea[3];		/**< BSS MAC address - 3 16-bit words */
	     chanspec_t chanspec;	/**< band, channel, bandwidth */
	     uint8 txstreams;		/**< number of tx spatial streams */
	     uint8 rxstreams;		/**< number of rx spatial streams */
	     uint32 report_ts;		/**< CSI Rx TSF timer timestamp */
	     uint32 assoc_ts;		/**< TSF timer timestamp at association time */
	     uint8 rssi[4];		/**< RSSI for each rx chain */
	 };
} __csimon_aligned;
typedef union wlc_csimon_hdr wlc_csimon_hdr_t;

struct wlc_csimon_rec {
	wlc_csimon_hdr_t csimon_hdr;
	uint8 csi_data[CSIMON_RING_ITEM_SIZE - CSIMON_HEADER_SIZE];
} __csimon_aligned;
typedef struct wlc_csimon_rec wlc_csimon_rec_t;

typedef struct csimon_xfer {

	/* Runtime State */
	uint32	           svmp_rd_idx;          /* Index of the SVMP CSI report */
	bcm_ring_t         local_ring;           /* ring of wlc_csimon_hdr_t or rec_t */
#if !defined(DONGLEBUILD)
	wlc_csimon_rec_t  *local_ring_base;      /* aligned local buffer */
	uint32             local_wr_idx;         /* Local write index for run-time use */
#else /* DONGLEBUILD */
	wlc_csimon_hdr_t  *local_ring_base;      /* aligned local buffer */
	bcm_ring_t         host_ring;            /* current write and read index */
	csimon_ipc_s2p_t  *s2p;                  /* HME memory mapped for SB2PCIE PIO */
	csimon_ipc_hme_t  *hme;                  /* hme_haddr64 lo32 for offset calc */
	haddr64_t          hme_haddr64;          /* csimon_ipc_hme */
#endif /* DONGLEBUILD */
	wlc_csimon_hdr_t  *local_ring_base_orig; /* unaligned version of above */
	dmaaddr_t          pa_orig;              /* physical address for buffer */
	uint32             alloced;              /* allocated buffersize */

#ifdef CSIMON_M2MCPY_BUILD
	m2m_dd_key_t       m2m_dd_csi;           /* M2M_DD_CSI usr registered key */
#endif
	/* Statistics */
	uint32             bytes;                /* total bytes transferred */
	uint32             xfers;                /* CSI records transferred */
	uint32             drops;                /* CSI records dropped */
	uint32             m2m_drops;            /* CSI records dropped:M2M error */
	uint32             host_rng_drops;       /* CSI records dropped:host ring full */
	uint32             fails;                /* CSI failures like m2m xfer */

} csimon_xfer_t;

struct wlc_csimon_sta {
	struct ether_addr ea;       /**< MAC addr - also in SCB if associated */
	uint16 timeout;             /**< CSI timeout(ms) for STA */
	uint32 assoc_ts;            /**< client association timestamp */
#ifdef CSIMON_PER_STA_TIMER
	struct wl_timer *timer;    /**< per-sta CSI monitor timer */
#else
	uint32 prev_time;        /**< null data time reference */
	uint32 delta_time;       /**< only For Debug Dump */
#endif /* CSIMON_PER_STA_TIMER */
	/**< Callback data for M2M DMA xfer of the CSI data */
	csimon_m2m_cb_data_t m2m_cb_data;
	/**< run time boolean indicating if M2M copy is in progress */
	bool m2mcpy_busy;

	/**< Null frames sent */
	uint32 null_frm_cnt;
	/**< Null frames acknowledged */
	uint32 null_frm_ack_cnt;
	/**< Number of CSI records successfully xferred by M2M DMA */
	uint32 m2mxfer_cnt;
	/**< Null frames not successfully acked */
	uint32 ack_fail_cnt;
	/**< The application is not reading the reports fast enough */
	uint32 rec_ovfl_cnt;
	/**< SVMP to host memory xfer failures */
	uint32 xfer_fail_cnt;
	/**< Both the CSI records/reports in mac memory were invalid */
	uint32 rpt_invalid_cnt;
	/**< Single CSI record/report in mac memory was invalid */
	uint32 rpt_invalid_once_cnt;
#ifdef BCM_CSIMON_AP
	uint8   SSID[DOT11_MAX_SSID_LEN];
	uint32  ssid_len;
#endif
	scb_t *scb;                             /**< back pointer to SCB */
};

struct wlc_csimon_info {
	/**< current number of clients doing CSI */
	uint8                num_clients;
#ifdef BCM_CSIMON_AP
	uint8                num_responder_aps;
#endif
	/**< CSIMON xfer related info */
	csimon_xfer_t        xfer;
	/**< Info about the clients doing CSI */
	wlc_csimon_sta_t     sta_info[CSIMON_MAX_STA];
	/**< CSIMON state like enabled/disabled and counters */
	csimon_state_t       state;
#ifndef CSIMON_PER_STA_TIMER
	wlc_hrt_to_t     *csi_timer;         /**< per-radio CSI monitor timer */
#endif
#if !defined(DONGLEBUILD)
	/**< Watchdog timeout(ms) for sending the CSIMON reports */
	uint16               wd_timeout;
	/**< Watchdog timer for sending the CSIMON reports */
	struct wl_timer     *wd_timer;
	/**< Netlink socket structure */
	struct sock         *nl_sock;
	/**< Netlink socket id */
	int                  nl_sock_id;
#endif /* !DONGLEBUILD */
	/**< M2M core interrupt name */
	char				irqname[32];
	/**< Back pointer to wlc */
	wlc_info_t          *wlc;
};

/* IOVar table */
enum {
	  IOV_CSIMON = 0, /* Enable/disable; Add/delete monitored STA's MAC addr */
	  IOV_CSIMON_STATE = 1, /* CSIMON enabled/disabled and counters */
	  IOV_LAST
};

static const bcm_iovar_t csimon_iovars[] = {
	{"csimon", IOV_CSIMON,
	(IOVF_SET_UP), 0, IOVT_BUFFER, sizeof(wlc_csimon_sta_config_t)
	},
	{"csimon_state", IOV_CSIMON_STATE,
	(0), 0, IOVT_BUFFER, sizeof(csimon_state_t)
	},
	{NULL, 0, 0, 0, 0, 0 }
};

static int8 wlc_csimon_sta_find(wlc_csimon_info_t *csimon_ctxt,
                                const struct ether_addr *ea);
static int wlc_csimon_doiovar(void *hdl, uint32 actionid, void *p, uint plen,
             void *a, uint alen, uint vsize, struct wlc_if *wlcif);

/* Ring macro used for NIC mode */
#define CSIMON_RING_IDX2ELEM(ring_base, idx) \
	    (((wlc_csimon_rec_t *)(ring_base)) + (idx))

#ifdef CSIMON_PER_STA_TIMER
/**
 * +--------------------------------------------------------------------------+
 *  Section: Functional Interface
 * +--------------------------------------------------------------------------+
 */
void // Add/start client-based timer
wlc_csimon_timer_add(wlc_info_t *wlc, struct scb *scb)
{
	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(scb);

	wl_add_timer(wlc->wl, scb->csimon->timer, scb->csimon->timeout, TRUE);

} // wlc_csimon_timer_add()

void // Delete/free client-based timer
wlc_csimon_timer_del(wlc_info_t *wlc, struct scb *scb)
{
	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(scb);

	wl_free_timer(wlc->wl, scb->csimon->timer);
	scb->csimon->timer = NULL;
} // wlc_csimon_timer_del()

bool //  Check if client-based timer is allocated
wlc_csimon_timer_isnull(struct scb *scb)
{
	CSIMON_ASSERT(scb);

	if (scb->csimon->timer == NULL) {
		return TRUE;
	}
	return FALSE;
} // wlc_csimon_timer_isnull()
#endif /* CSIMON_PER_STA_TIMER */
#if !defined(DONGLEBUILD)
void // Delete/free the NIC-mode watchdog timer
wlc_csimon_wd_timer_del(wlc_info_t *wlc)
{
	CSIMON_ASSERT(wlc);
	if (wlc->csimon_info->wd_timer != NULL) {
		wl_free_timer(wlc->wl, wlc->csimon_info->wd_timer);
		wlc->csimon_info->wd_timer = NULL;
	}
}
#endif /* ! DONGLEBUILD */

void // Set the association timestamp as a reference
wlc_csimon_assocts_set(wlc_info_t *wlc, struct scb *scb)
{
		uint32 tsf_l;

		CSIMON_ASSERT(wlc);
		CSIMON_ASSERT(scb);

		/* association timestamp as a reference - using TSF register */
		wlc_read_tsf(wlc, &tsf_l, NULL);
		scb->csimon->assoc_ts = tsf_l;
} // wlc_csimon_assocts_set()

bool // Check if CSIMON feature is enabled and STA is a member of the STA list
wlc_csimon_enabled(wlc_info_t *wlc, scb_t *scb)
{
	bool retval = FALSE;
	int idx = -1;

	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(scb);

	if (CSIMON_ENAB(wlc->pub)) {
		/* Also check if the STA is member of the STA list */
		if (scb && ((idx = wlc_csimon_sta_find(wlc->csimon_info, &scb->ea)) >= 0)) {
			CSIMON_DEBUG("wl%d:Found STA in the list at idx %d "
						 "for SCB DA "MACF"\n", wlc->pub->unit, idx,
						 ETHER_TO_MACF(scb->ea));
			retval = TRUE;
		} else {
			CSIMON_DEBUG("wl%d:csimon_enable %d STA in the list at idx %d "
			             "for SCB DA "MACF"\n", wlc->pub->unit,
			             CSIMON_ENAB(wlc->pub), idx, ETHER_TO_MACF(scb->ea));
		}
	}

	return retval;
} // wlc_csimon_enabled()

#if defined(DONGLEBUILD)
int pciedev_csimon_dump(void) // Invoked from the command line
{
	printf("Use 'wl -i <wlX> dump csimon'.\n");

	return BCME_OK;
} // pciedev_csimon_dump()
#endif /* DONGLEBUILD */

int // CSIMON SCB Initialization
wlc_csimon_scb_init(wlc_info_t *wlc, scb_t *scb)
{
	int idx;

	if (!wlc || !scb) {
		return BCME_BADARG;
	}
	idx = 0;

	if (wlc_csimon_enabled(wlc, scb)) {
		/* Get the index of this client in the CSIMON_STA list */
		idx = wlc_csimon_sta_find(wlc->csimon_info, &scb->ea);
		 /* Set the pointer to the appropriate CSIMON_STA */
		if (idx >= 0) {
			scb->csimon = &(wlc->csimon_info->sta_info[idx]);
			/* Set the back pointer in the CSIMON_STA */
			wlc->csimon_info->sta_info[idx].scb = scb;
			CSIMON_DEBUG("wl%d:Found STA in the list at idx %d "
						 "for SCB DA "MACF"\n", wlc->pub->unit, idx,
						 ETHER_TO_MACF(scb->ea));
		} else { /* should not happen if wlc_csimon_enabled returns true */
			CSIMON_DEBUG("wl%d:NOT Found STA in the list at idx %d "
						 "for SCB DA "MACF"\n", wlc->pub->unit, idx,
						 ETHER_TO_MACF(scb->ea));
			return BCME_NOTFOUND;
		}
#ifdef CSIMON_PER_STA_TIMER
		/* Initialize the per-client timer */
		scb->csimon->timer = wl_init_timer(wlc->wl, wlc_csimon_scb_timer,
		                                   scb, "csimon");
		if (!(scb->csimon->timer)) {
			WL_ERROR(("wl%d: csimon timer init failed for "MACF"\n",
			          wlc->pub->unit, ETHER_TO_MACF(scb->ea)));
			return BCME_NORESOURCE;
		}
		CSIMON_DEBUG("wl%d:Init csimon timer %p timeout %u "
				"for SCB DA "MACF"\n", wlc->pub->unit, scb->csimon->timer,
				scb->csimon->timeout, ETHER_TO_MACF(scb->ea));
#endif
	}
	return BCME_OK;
}

#ifdef CSIMON_PER_STA_TIMER
void
wlc_csimon_scb_timer(void *arg)
{
	scb_t *scb = (scb_t *)arg;
	wlc_info_t *wlc;
	ratespec_t rate_override = 0;
	CSIMON_ASSERT(scb);

	wlc = scb->bsscfg->wlc;

	if (BAND_2G(wlc->band->bandtype)) {
		/* use the lowest non-BPHY rate for 2G QoS Null frame */
		rate_override = CSIMON_NULL_FRAME_RSPEC;
	}
	if (CSIMON_ENAB(wlc->pub) &&
#ifdef BCM_CSIMON_AP
	(!(wlc_csimon_ap_ssid_len_check(scb))) &&
#endif
	TRUE) {
		/* Send null frame for CSI Monitor (Assoc-STA case) */
		if (!wlc_sendnulldata(wlc, scb->bsscfg, &scb->ea, rate_override,
			WLF_CSI_NULL_PKT, PRIO_8021D_VO, NULL, NULL)) {
				WL_ERROR(("wl%d.%d: %s: wlc_sendnulldata failed for DA "MACF"\n",
						wlc->pub->unit, WLC_BSSCFG_IDX(scb->bsscfg),
					__FUNCTION__, ETHER_TO_MACF(scb->ea)));
		} else {
			CSIMON_DEBUG("wl%d: !!!!!!!!sent null frame for DA "MACF"\n",
				wlc->pub->unit, ETHER_TO_MACF(scb->ea));
				wlc->csimon_info->state.null_frm_cnt++;
				scb->csimon->null_frm_cnt++;
		}
	}
#ifdef BCM_CSIMON_AP
	if ((CSIMON_ENAB(wlc->pub) &&
		wlc_csimon_ap_ssid_len_check(scb))) {
		/* Send PROBE REQ frame for CSI Monitor (AP to AP case) */
		/* GET THE da MAC address from csimon struct */
		wlc_sendprobe(wlc, scb->bsscfg, scb->csimon->SSID, scb->csimon->ssid_len,
				0, NULL, &(wlc->primary_bsscfg->cur_etheraddr),
				&scb->csimon->ea, &scb->csimon->ea,
				wlc_lowest_basic_rspec(wlc, &wlc->band->hw_rateset),
				NULL,
				FALSE);
		CSIMON_DEBUG("wl%d: !!!!!!!!sent a probe req frame for DA "MACF" with SSID:%s\n",
				wlc->pub->unit, ETHER_TO_MACF(scb->csimon->ea), scb->csimon->SSID);
		wlc->csimon_info->state.null_frm_cnt++;
		scb->csimon->null_frm_cnt++;
	}
#endif /* BCM_CSIMON_AP */
	/* delete old CSI timer */
	wl_del_timer(scb->bsscfg->wlc->wl, scb->csimon->timer);

	/* add new CSI timer */
	wl_add_timer(scb->bsscfg->wlc->wl, scb->csimon->timer,
	             scb->csimon->timeout, TRUE);
	CSIMON_DEBUG("wl%d: TIMEOUT: added csi timer for %d ms SCB "MACF" scb %p\n",
			wlc->pub->unit, scb->csimon->timeout, ETHER_TO_MACF(scb->ea), scb);
}
#endif /* CSIMON_PER_STA_TIMER */

#if !defined(DONGLEBUILD)

#ifdef CSIMON_FILE_BUILD
#include <linux/fs.h>

static int
wlc_csimon_write(wlc_csimon_info_t *csimon, void * csi_buf, ssize_t csi_buf_len)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	ssize_t         ret;
	struct file     *file;
	loff_t          offset;
	int             flags       = O_RDWR | O_CREAT | O_LARGEFILE | O_SYNC;
	const char      *filename   = CSIMON_FILENAME;

	ASSERT(csimon != NULL);
	ASSERT(csi_buf != NULL);

	file = filp_open(filename, flags, 0600);
	if (IS_ERR(file)) {
		WL_ERROR(("%s filp_open(%s) failed\n", __FUNCTION__, filename));
		return PTR_ERR(file);
	}

	offset = default_llseek(file, 0, SEEK_END); /* seek to EOF */
	if (offset < 0) {
		ret = (ssize_t)offset;
		goto exit1;
	}

	if (offset >= CSIMON_FILESIZE) {    /* csimon file is bounded, drop */
		ret = BCME_NORESOURCE;
		goto exit1;
	}

	ret = kernel_write(file, csi_buf, csi_buf_len, &offset);
	if (ret != csi_buf_len) {
		WL_ERROR(("%s kernel_write %s failed: %zd\n",
			__FUNCTION__, filename, ret));
		if (ret > 0)
			ret = BCME_ERROR;
		goto exit1;
	}

	ret = BCME_OK;

exit1:
	filp_close(file, NULL);
	return ret;

#else   /* LINUX_VERSION_CODE < 4 */
	return BCME_OK;
#endif	/* LINUX_VERSION_CODE < 4 */

} // wlc_csimon_write()

#else /* ! CSIMON_FILE_BUILD */

static int // send the CSI record over netlink socket to the user application
wlc_csimon_netlink_send(wlc_csimon_info_t *csimon, void * csi_buf, ssize_t csi_buf_len)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlmh = NULL;
	int ret;

	CSIMON_ASSERT(csimon);
	CSIMON_ASSERT(csimon->wlc);

	if (csimon->nl_sock == NULL) {
		WL_ERROR(("%s: nl mcast not done as nl_sock is NULL\n", __FUNCTION__));
		return BCME_NORESOURCE;
	}

	/* Allocate an SKB and its data area */
	skb = alloc_skb(NLMSG_SPACE(csi_buf_len), GFP_KERNEL);
	if (skb == NULL) {
		WL_ERROR(("Allocation failure!\n"));
		return BCME_NORESOURCE;
	}
	skb_put(skb, NLMSG_SPACE(csi_buf_len));

	/* Fill in the netlink msg header that is at skb->data */
	nlmh = (struct nlmsghdr *)skb->data;
	nlmh->nlmsg_len = NLMSG_SPACE(csi_buf_len);
	nlmh->nlmsg_pid = 0; /* kernel */
	nlmh->nlmsg_flags = 0;

	/* Copy the CSI record into the netlink SKB structure */
	/* XXX Can we avoid this copy inside kernel space? Apparently mmap netlink
	 * could do but not sure if we can use it in this case where we have the
	 * CSI records in HME. Also the mmap netlink support has been removed from
	 * the latest Linux kernels.
	 */
	memcpy(NLMSG_DATA(nlmh), csi_buf, csi_buf_len);
	{
		struct timespec64 ts;
		ktime_get_real_ts64(&ts);
		CSIMON_DEBUG("wl%d: nl multicast at %lu.%lu for skb %p nlmh %p \n",
		  csimon->wlc->pub->unit, (ulong)ts.tv_sec, ts.tv_nsec/1000, skb, nlmh);
	}

	/* Send the CSI record over netlink multicast */
	ret = netlink_broadcast(csimon->nl_sock, skb, 0, CSIMON_GRP_BIT, GFP_KERNEL);
	if (ret < 0) {
		WL_ERROR(("%s: nl mcast failed %d sock %p skb %p\n", __FUNCTION__,
		           ret, csimon->nl_sock, skb));
		return ret;
	}
	else {
		CSIMON_DEBUG("wl%d: nl mcast successful ret %d socket %p skb %p\n",
		           csimon->wlc->pub->unit, ret, csimon->nl_sock, skb);
	}

	return BCME_OK;

#else   /* LINUX_VERSION_CODE < 4 */
	return BCME_OK;
#endif	/* LINUX_VERSION_CODE < 4 */
}

#endif /* ! CSIMON_FILE_BUILD */

static void // Watchdog-like timer for CSIMON to transfer CSI records to the user-space
wlc_csimon_wd_timer(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t *)arg;
	int elem_idx = 0;
	wlc_csimon_rec_t *elem;

	CSIMON_ASSERT(wlc);

	/* Transfer the CSI records present in the host memory ring */
	while ((elem_idx = bcm_ring_cons(&CSIxfer(wlc).local_ring, CSIMON_LOCAL_RING_DEPTH))
	            != BCM_RING_EMPTY) {
		elem = CSIMON_RING_IDX2ELEM(CSIxfer(wlc).local_ring_base, elem_idx);
		OSL_CACHE_INV((void*)(elem), CSIMON_RING_ITEM_SIZE);

		/* print on console if enabled */
		//__wlc_csimon_rpt_print_console((uint16 *)(elem->data));

#ifdef CSIMON_FILE_BUILD
		/* Transfer to a file */
		if (wlc_csimon_write(wlc->csimon_info, elem, CSIMON_RING_ITEM_SIZE) != BCME_OK) {
			wlc->csimon_info->state.usr_xfer_fail_cnt++;
		}
		else {
			wlc->csimon_info->state.usrxfer_cnt++;
		}
#elif !defined(CSIMON_M2M_CONSOLE_PRINT) /* ! CSIMON_FILE_BUILD */
		/* Transfer over netlink socket */
		if (wlc_csimon_netlink_send(wlc->csimon_info, (void *) elem,
		    CSIMON_RING_ITEM_SIZE)) {
			wlc->csimon_info->state.usr_xfer_fail_cnt++;
			WL_ERROR(("%s: CSIMON drops with nl send %d\n", __FUNCTION__,
			           wlc->csimon_info->state.usr_xfer_fail_cnt));
		}
		else {
			wlc->csimon_info->state.usrxfer_cnt++;
		}
#endif /* ! CSIMON_FILE_BUILD */
	}

	/* delete old wd timer */
	wl_del_timer(wlc->wl, wlc->csimon_info->wd_timer);

	/* add new wd timer */
	wl_add_timer(wlc->wl, wlc->csimon_info->wd_timer, wlc->csimon_info->wd_timeout, TRUE);
	CSIMON_DEBUG("wl%d: TIMEOUT: added wd timer for %d ms \n",
			wlc->pub->unit, wlc->csimon_info->wd_timeout);
}
#endif /* !DONGLEBUILD */

#if defined(CSIMON_S2PCPY_BUILD)
static INLINE int // Transfer a CSI structure to Host using synchronous copy
__wlc_csimon_xfer_s2pcpy(wlc_info_t *wlc, scb_t *scb, void * csi_hdr, int csi_len,
                         uint32 *svmp_addr, void *xfer_scb)
{
	int i, wr_idx;
	uint16 hdr_len;
	csimon_ring_elem_t * s2p_elem;

	CSIMON_ASSERT(csi_len <= CSIMON_RING_ITEM_SIZE);
	CSIMON_ASSERT((csi_len % sizeof(int)) == 0); // 32b copies

	sbtopcie_switch(SBTOPCIE_USR_CSI); // activate User CSI's sbtopcie window

	/* Lazilly refresh the host updated read index */
	if (bcm_ring_is_full(&CSIxfer(wlc).host_ring, CSIMON_RING_ITEMS_MAX)) {
		CSIxfer(wlc).host_ring.read = // refresh and try again
			(int)CSIxfer(wlc).s2p->preamble.read_idx.u32;
		if (bcm_ring_is_full(&CSIxfer(wlc).host_ring, CSIMON_RING_ITEMS_MAX)) {
			CSIxfer(wlc).drops++;
			CSIxfer(wlc).host_rng_drops++;
			wlc->csimon_info->state.rec_ovfl_cnt++;
			scb->csimon->rec_ovfl_cnt++;
			return BCME_NORESOURCE;
		}
	}

	/* Determine the destination in host circular ring */
	wr_idx   = bcm_ring_prod(&CSIxfer(wlc).host_ring, CSIMON_RING_ITEMS_MAX);
	s2p_elem = CSIMON_TABLE_IDX2ELEM(CSIxfer(wlc).s2p->table, wr_idx);

	/* s2p copy in 32bit pio operations */

	CSIxfer(wlc).xfers += 1;
	CSIxfer(wlc).bytes += csi_len;

	/* First copy the CSI header from dongle sysmem */
	hdr_len = CSIMON_HEADER_SIZE / (sizeof(int));
	for (i = 0; i < hdr_len; ++i) {
		*(((int*)s2p_elem) + i) = *(((int*)csi_hdr) + i);
	}
	/* Now copy the CSI report from SVMP */
	csi_len  = (csi_len - CSIMON_HEADER_SIZE) / sizeof(int);
	for (i = 0; i < csi_len; ++i) {
		*(((int*)s2p_elem) + i) = *(((int*)svmp_addr) + i);
	}

	/* Post the updated write index to the host ring preamble */
	CSIxfer(wlc).s2p->preamble.write_idx.u32 = CSIxfer(wlc).host_ring.write;

	scb->csimon->m2mxfer_cnt++;
	wlc->csimon_info->state.m2mxfer_cnt++;

	return BCME_OK;

}  // __wlc_csimon_xfer_s2pcpy()

#endif /* CSIMON_S2PCPY_BUILD */

#if defined(CSIMON_M2MCPY_BUILD)
/*
 * Transfer CSI record to Host using asynchronous copy. A CSI record consists of
 * a fixed width CSI header that contains the channel number, MAC address, TSF,
 * RSSI etc. The CSI record also has a CSI report (H-matrix data) that is
 * transferred to the host without peeking into it. The CSI report could have
 * variable length based on phy bandwidth, number of Rx antenna, and decimation
 * factor but here we transfer a fixed length data from SVMP memory. It is the
 * user application's responsibility to pick correct length of the report based
 * on the report metadata inside the first 32 bytes of the report.
 *
 * csi_hdr:		Sysmem resident CSI header that includes MAC addr, RSSI
 * csi_len:		Total length of data to be transferred; it includes CSI
 *			header of CSIMON_HEADER_LENGTH length and CSI report
 * svmp_addr_u32:	Address of the CSI report resident in SVMP memory
 * xfer_arg:		Callback function to be called after the data transfer
 *			is complete
 *
 */
#if defined(DONGLEBUILD)
static INLINE int
__wlc_csimon_xfer_m2mcpy(wlc_info_t *wlc, scb_t *scb, void * csi_hdr,
                         int csi_len, uint32 *svmp_addrp_u32, void *xfer_arg)
{
	int wr_idx;
	m2m_dd_cpy_t cpy_key;
	csimon_ring_elem_t *elem;
#ifdef SINGLE_M2M_XFER
	dma64addr_t xfer_src, xfer_dst;
#else /* ! SINGLE_M2M_XFER */
	dma64addr_t xfer_src_sv, xfer_dst_sv, xfer_src_sm, xfer_dst_sm;
#endif /* ! SINGLE_M2M_XFER */

	CSIMON_ASSERT(csi_len <= CSIMON_RING_ITEM_SIZE);
	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(wlc->csimon_info);
	CSIMON_ASSERT(scb);
	CSIMON_ASSERT(scb->csimon);

	/* Lazilly refresh the host updated read index */
	if (bcm_ring_is_full(&CSIxfer(wlc).host_ring, CSIMON_RING_ITEMS_MAX)) {
		sbtopcie_switch(SBTOPCIE_USR_CSI); // activate CSI's sbtopcie window
		CSIxfer(wlc).host_ring.read = // refresh and try again
			(int)CSIxfer(wlc).s2p->preamble.read_idx.u32;
		if (bcm_ring_is_full(&CSIxfer(wlc).host_ring, CSIMON_RING_ITEMS_MAX)) {
			CSIxfer(wlc).drops++;
			CSIxfer(wlc).host_rng_drops++;
			wlc->csimon_info->state.rec_ovfl_cnt++;
			scb->csimon->rec_ovfl_cnt++;
			return BCME_NORESOURCE;
		}
	}
	CSIMON_DEBUG("osh %p key %d\n", wlc->osh, CSIxfer(wlc).m2m_dd_csi);

	if (m2m_dd_avail(wlc->osh, CSIxfer(wlc).m2m_dd_csi) < 2) {
		wlc->csimon_info->state.xfer_fail_cnt++;
		scb->csimon->xfer_fail_cnt++;
		WL_ERROR(("CSIMON: Failure %u no m2m_dd_avail\n",
		          wlc->csimon_info->state.xfer_fail_cnt));
		return BCME_NORESOURCE;
	}

	/* Determine the destination in host circular ring */
	wr_idx = bcm_ring_prod(&CSIxfer(wlc).host_ring, CSIMON_RING_ITEMS_MAX);
	elem   = CSIMON_TABLE_IDX2ELEM(CSIxfer(wlc).hme->table, wr_idx);

#ifdef SINGLE_M2M_XFER

	xfer_src.hiaddr = 0U;
	xfer_src.loaddr = (uint32)((uintptr)csi_hdr);

	xfer_dst.hiaddr = HADDR64_HI(CSIxfer(wlc).hme_haddr64);
	xfer_dst.loaddr = (uint32)((uintptr)(elem));
	cpy_key	= m2m_dd_xfer(wlc->osh, CSIxfer(wlc).m2m_dd_csi,
	                      &xfer_src, &xfer_dst, csi_len,
	                      xfer_arg, M2M_DD_XFER_COMMIT | M2M_DD_XFER_RESUME);
	if (cpy_key == M2M_INVALID) {
		wlc->csimon_info->state.xfer_fail_cnt++;
		scb->csimon->xfer_fail_cnt++;
		WL_ERROR(("CSIMON: Failure %u in m2m_dd_xfer\n",
		          wlc->csimon_info->state.xfer_fail_cnt));
		CSIMON_ASSERT(0);
		return BCME_ERROR;
	} else {
		CSIxfer(wlc).xfers += 1;
		CSIxfer(wlc).bytes += csi_len;
	}
	CSIMON_DEBUG("xfer sys mem 0x%x to elem idx %d host 0x%x len %u"
			" xfers %u\n", xfer_src.loaddr,
			wr_idx, xfer_dst.loaddr, csi_len, CSIxfer(wlc).xfers);

#else /* ! SINGLE_M2M_XFER */

	/* src_sv is csi report in SVMP memory */
	xfer_src_sv.hiaddr = 0U;
	xfer_src_sv.loaddr = (uint32)((uintptr)svmp_addrp_u32);

	/* dst_sv is element at index wr_idx plus an offset in host circular ring */
	xfer_dst_sv.hiaddr = HADDR64_HI(CSIxfer(wlc).hme_haddr64);
	xfer_dst_sv.loaddr = (uint32)((uintptr)elem) + CSIMON_HEADER_SIZE;

	/* src_sm is csi data in dongle system memory */
	xfer_src_sm.hiaddr = 0U;
	xfer_src_sm.loaddr = (uint32)((uintptr)csi_hdr);

	/* dst_sm is element at index wr_idx in host circular ring */
	xfer_dst_sm.hiaddr = HADDR64_HI(CSIxfer(wlc).hme_haddr64);
	xfer_dst_sm.loaddr = (uint32)((uintptr)elem);

	CSIMON_DEBUG("xfer SVMP mem 0x%x to idx %d host 0x%x len %u\n",
			xfer_src_sv.loaddr, wr_idx,
	        xfer_dst_sv.loaddr, (csi_len - CSIMON_HEADER_SIZE));

	cpy_key = m2m_dd_xfer(wlc->osh, CSIxfer(wlc).m2m_dd_csi,
	                      &xfer_src_sv, &xfer_dst_sv,
	                      (csi_len - CSIMON_HEADER_SIZE), 0U, 0U);
	if (cpy_key == M2M_INVALID) {
		wlc->csimon_info->state.xfer_fail_cnt++;
		scb->csimon->xfer_fail_cnt++;
		WL_ERROR(("CSIMON: Failure %u in first m2m_dd_xfer\n",
		          wlc->csimon_info->state.xfer_fail_cnt));
	        CSIMON_ASSERT(0);
		return BCME_ERROR;
	}
	cpy_key = m2m_dd_xfer(wlc->osh, CSIxfer(wlc).m2m_dd_csi,
	                      &xfer_src_sm, &xfer_dst_sm, CSIMON_HEADER_SIZE,
	                      xfer_arg, M2M_DD_XFER_COMMIT | M2M_DD_XFER_RESUME);

	if (cpy_key == M2M_INVALID) {
		wlc->csimon_info->state.xfer_fail_cnt++;
		scb->csimon->xfer_fail_cnt++;
		WL_ERROR(("CSIMON: Failure %u in 2nd m2m_dd_xfer\n",
		          wlc->csimon_info->state.xfer_fail_cnt));
		CSIMON_ASSERT(0);
		return BCME_ERROR;
	} else {
		CSIxfer(wlc).xfers += 1;
		CSIxfer(wlc).bytes += csi_len;
	}
	CSIMON_DEBUG("xfer sys mem 0x%x to host 0x%x len %u xfers %u\n",
	             xfer_src_sm.loaddr, xfer_dst_sm.loaddr,
	             CSIMON_HEADER_SIZE, CSIxfer(wlc).xfers);
#endif /* ! SINGLE_M2M_XFER */

	return BCME_OK;
} // __wlc_csimon_xfer_m2mcpy()

#else /* ! DONGLEBUILD */

static INLINE int
__wlc_csimon_xfer_m2mcpy(wlc_info_t *wlc, scb_t *scb, void * csi_rec,
                         int csi_len, uint32 *svmp_addrp_u32, void *xfer_arg)
{
	m2m_dd_cpy_t cpy_key;
	dma64addr_t xfer_src, xfer_dst;
	uintptr xfer_dst_u;

	CSIMON_ASSERT(csi_len <= CSIMON_RING_ITEM_SIZE);
	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(wlc->csimon_info);
	CSIMON_ASSERT(scb);
	CSIMON_ASSERT(scb->csimon);

	if (m2m_dd_avail(wlc->osh, CSIxfer(wlc).m2m_dd_csi) < 1) {
		wlc->csimon_info->state.xfer_fail_cnt++;
		scb->csimon->xfer_fail_cnt++;
		WL_ERROR(("CSIMON: Failure %u no m2m_dd_avail\n",
		          wlc->csimon_info->state.xfer_fail_cnt));
		return BCME_NORESOURCE;
	}

	/* Determine the destination in local circular ring */
	xfer_src.hiaddr = 0U;
	xfer_src.loaddr = (uint32)((uintptr)svmp_addrp_u32);

	xfer_dst_u = (uintptr)VIRT_TO_PHYS((void *)((uintptr)csi_rec +
	                                     CSIMON_HEADER_SIZE));
#if defined(__ARM_ARCH_7A__)
	xfer_dst.hiaddr = 0U;
#else
	xfer_dst.hiaddr = (uint32)(((uint64)xfer_dst_u) >> 32);
#endif
	xfer_dst.loaddr = (uint32)(xfer_dst_u);

	cpy_key	= m2m_dd_xfer(wlc->osh, CSIxfer(wlc).m2m_dd_csi, &xfer_src,
	                      &xfer_dst, (csi_len - CSIMON_HEADER_SIZE),
	                      xfer_arg,
	                      M2M_DD_XFER_COMMIT | M2M_DD_XFER_RESUME);
	if (cpy_key == M2M_INVALID) {
		wlc->csimon_info->state.xfer_fail_cnt++;
		scb->csimon->xfer_fail_cnt++;
		WL_ERROR(("CSIMON: Failure %u in m2m_dd_xfer\n",
		          wlc->csimon_info->state.xfer_fail_cnt));
		CSIMON_ASSERT(0);
		return BCME_ERROR;
	}
	CSIMON_DEBUG("xfer SVMP mem 0x%08x local hi 0x%08x lo 0x%08x csi_rec %p len"
				 " %u\n", xfer_src.loaddr, xfer_dst.hiaddr, xfer_dst.loaddr,
				 csi_rec, csi_len - CSIMON_HEADER_SIZE);

	return BCME_OK;
} // __wlc_csimon_xfer_m2mcpy()
#endif /* ! DONGLEBUILD */

#endif /* CSIMON_M2MCPY_BUILD */

static INLINE int // Transfer a CSI record to host DDR
__wlc_csimon_xfer(wlc_info_t *wlc, scb_t *scb, void * csi_hdr, int csi_len,
                  uint32 *svmp_addrp, void *cb_data)
{
#if   defined(CSIMON_S2PCPY_BUILD)
	return __wlc_csimon_xfer_s2pcpy(wlc, scb, csi_hdr, csi_len, svmp_addrp, cb_data);
#elif defined(CSIMON_M2MCPY_BUILD)
	return __wlc_csimon_xfer_m2mcpy(wlc, scb, csi_hdr, csi_len, svmp_addrp,
	                                cb_data);
#else
	return BCME_ERROR;
#endif
} // wlc_csimon_xfer()

static void	// Clear the client level statistics
wlc_csimon_sta_stats_clr(wlc_csimon_info_t *ctxt, int8 idx)
{
	if (idx < 0 || idx >= CSIMON_MAX_STA) {
		WL_ERROR(("Incorrect client index %d for deletion of stats\n", idx));
		return;
	}
	ctxt->sta_info[idx].null_frm_cnt = 0;
	ctxt->sta_info[idx].null_frm_ack_cnt = 0;
	ctxt->sta_info[idx].m2mxfer_cnt = 0;
	ctxt->sta_info[idx].ack_fail_cnt = 0;
	ctxt->sta_info[idx].rec_ovfl_cnt = 0;
	ctxt->sta_info[idx].xfer_fail_cnt = 0;
	ctxt->sta_info[idx].rpt_invalid_cnt = 0;
	ctxt->sta_info[idx].rpt_invalid_once_cnt = 0;
	return;
} // wlc_csimon_sta_stats_clr()

static int // Dump all the CSIMON counters: per-module and per-client
wlc_csimon_dump(void *ctx, bcmstrbuf_t *b)
{
	wlc_csimon_info_t *c_info = (wlc_csimon_info_t *)ctx;
	int idx;

	bcm_bprintf(b, "CSI Monitor: %u\n", CSIMON_ENAB(WLCPUB(c_info)));
	bcm_bprintf(b, "Number of clients: %u\n\n", c_info->num_clients);
	bcm_bprintf(b, "Aggregate Statistics\n------------------------\n");

#if defined(DONGLEBUILD)
	bcm_bprintf(b, "null_frm_cnt %u xfer_to_ddr_cnt %u ack_fail_cnt %u\n"
	            "rec_ovfl_cnt %u xfer_to_ddr_fail_cnt %u\n\n",
	            c_info->state.null_frm_cnt, c_info->state.m2mxfer_cnt,
	            c_info->state.ack_fail_cnt,
	            c_info->state.rec_ovfl_cnt, c_info->state.xfer_fail_cnt);
#else /* ! DONGLEBUILD */
	bcm_bprintf(b, "null_frm_cnt %u xfer_to_usr_cnt %u xfer_to_ddr_cnt %u ack_fail_cnt %u\n"
	            "rec_ovfl_cnt %u xfer_to_ddr_fail_cnt %u xfer_to_usr_fail_cnt %u\n\n",
	            c_info->state.null_frm_cnt, c_info->state.usrxfer_cnt,
	            c_info->state.m2mxfer_cnt, c_info->state.ack_fail_cnt,
	            c_info->state.rec_ovfl_cnt, c_info->state.xfer_fail_cnt,
	            c_info->state.usr_xfer_fail_cnt);
#endif /* ! DONGLEBUILD */
	bcm_bprintf(b, "Per-client Statistics\n------------------------\n");
	for (idx = 0; idx < CSIMON_MAX_STA; idx++) {
		if (bcmp(&ether_null, &c_info->sta_info[idx].ea, ETHER_ADDR_LEN) == 0) {
			continue;
		}
		bcm_bprintf(b, MACF"\t monitor_interval %d milliseconds\n",
		            ETHER_TO_MACF(c_info->sta_info[idx].ea),
		            c_info->sta_info[idx].timeout);
#if defined(CSIMON_DEBUG_DUMP)
		bcm_bprintf(b, "null_frm_cnt %u xfer_to_ddr_cnt %u delta_time %u \n"
		"ack_fail_cnt %u rec_ovfl_cnt %u xfer_to_ddr_fail_cnt %u rpt_invalid_cnt %u \n"
		"rpt_invaild_once_cnt %u \n\n",
		c_info->sta_info[idx].null_frm_cnt, c_info->sta_info[idx].m2mxfer_cnt,
		c_info->sta_info[idx].delta_time, c_info->sta_info[idx].ack_fail_cnt,
		c_info->sta_info[idx].rec_ovfl_cnt, c_info->sta_info[idx].xfer_fail_cnt,
		c_info->sta_info[idx].rpt_invalid_cnt, c_info->sta_info[idx].rpt_invalid_once_cnt);
#else /* ! CSIMON_DEBUG_DUMP */
		bcm_bprintf(b, "null_frm_cnt %u xfer_to_ddr_cnt %u ack_fail_cnt %u\n"
		 "rec_ovfl_cnt %u xfer_to_ddr_fail_cnt %u\n\n",
		 c_info->sta_info[idx].null_frm_cnt, c_info->sta_info[idx].m2mxfer_cnt,
		 c_info->sta_info[idx].ack_fail_cnt, c_info->sta_info[idx].rec_ovfl_cnt,
		 c_info->sta_info[idx].xfer_fail_cnt);
#endif /* ! CSIMON_DEBUG_DUMP */
	}

	bcm_bprintf(b, "More Details\n------------------------\n");
#if defined(DONGLEBUILD)
	sbtopcie_switch(SBTOPCIE_USR_CSI); // activate user CSI's sbtopcie window
	bcm_bprintf(b, "Version:" CSIMON_VRP_FMT
		"\nHOST ADDR" HADDR64_FMT " IPC HME %p S2P %p"
		"\nHOST TABLE HME %p S2P %p WR %u RD %u\nDNGL RING %p WR %u RD %u\n",
		CSIMON_VRP_VAL(CSIMON_VERSIONCODE), HADDR64_VAL(c_info->xfer.hme_haddr64),
		c_info->xfer.hme, c_info->xfer.s2p,
		c_info->xfer.hme->table, c_info->xfer.s2p->table,
		c_info->xfer.host_ring.write, c_info->xfer.host_ring.read,
		c_info->xfer.local_ring_base,
		c_info->xfer.local_ring.write, c_info->xfer.local_ring.read);
#else /* ! DONGLEBUILD */
	bcm_bprintf(b, "Version:" CSIMON_VRP_FMT "\nLOCAL RING %p WR %u RD %u\n",
		CSIMON_VRP_VAL(CSIMON_VERSIONCODE), c_info->xfer.local_ring_base,
		c_info->xfer.local_ring.write, c_info->xfer.local_ring.read);
#ifndef CSIMON_FILE_BUILD
	bcm_bprintf(b, "Netlink subsystem for CSIMON: %d\n", c_info->nl_sock_id);
#endif
#endif /* ! DONGLEBUILD */

	return BCME_OK;
} // wlc_csimon_dump()

static int // Clear all the CSIMON counters: per-module and per-client
wlc_csimon_dump_clr(void *ctx)
{
	wlc_csimon_info_t *c_info = (wlc_csimon_info_t *)ctx;
	int idx;

	/* Clear the aggregate statistics */
	c_info->state.null_frm_cnt = 0;
	c_info->state.m2mxfer_cnt = 0;
	c_info->state.usrxfer_cnt = 0;
	c_info->state.ack_fail_cnt = 0;
	c_info->state.rec_ovfl_cnt = 0;
	c_info->state.xfer_fail_cnt = 0;
	c_info->state.usr_xfer_fail_cnt = 0;

	/* Now clear the client level statistics */
	for (idx = 0; idx < CSIMON_MAX_STA; idx++) {
		if (bcmp(&ether_null, &c_info->sta_info[idx].ea, ETHER_ADDR_LEN) == 0) {
			continue;
		}
		wlc_csimon_sta_stats_clr(c_info, idx);
	}

	return BCME_OK;
} // wlc_csimon_dump_clr()

wlc_csimon_info_t * // Attach the CSIMON module
BCMATTACHFN(wlc_csimon_attach)(wlc_info_t *wlc)
{
	int                  i;
	wlc_csimon_info_t   *csimon_ctxt = NULL;
	uint32               mem_size;
	uint32               align_bits = 6;	/* 64-Byte alignment */
	uint32               alloced;
	void                *va;
	uint32               offset;
#if defined(DONGLEBUILD)
	/* Dongle sysmem-resident ring element holds CSI header */
	mem_size = CSIMON_HEADER_SIZE * CSIMON_LOCAL_RING_DEPTH;
#else /* ! DONGLEBUILD */
	/* NIC host memory resident whole CSI record */
	mem_size = CSIMON_RING_ITEM_SIZE * CSIMON_LOCAL_RING_DEPTH;
#endif /* ! DONGLEBUILD */

	if (!wlc) return NULL;

#if defined(DONGLEBUILD)
	{   /* Overwrite default HME configuration for HME_USER_CSIMON */
		uint32 hme_pages, bound_bits, hme_flags, item_size, items_max;
		hme_pages  = PCIE_IPC_HME_PAGES(CSIMON_IPC_HME_BYTES);
		bound_bits = SBTOPCIE_WIN_32MB; /* buffer must fit into 32MB region */
		hme_flags  = PCIE_IPC_HME_FLAG_ALIGNED  | PCIE_IPC_HME_FLAG_BOUNDARY
		           | PCIE_IPC_HME_FLAG_SBTOPCIE | PCIE_IPC_HME_FLAG_DMA_XFER;
		item_size  = 0U; items_max  = 0U;
		hme_attach_mgr(HME_USER_CSIMON, hme_pages, /* overwrite default(s) */
		               item_size, items_max, HME_MGR_NONE,
		               align_bits, bound_bits, hme_flags);
	}
#endif /* DONGLEBUILD */

	/* Memory allocation */
	csimon_ctxt = (wlc_csimon_info_t*)MALLOCZ(wlc->pub->osh,
			sizeof(wlc_csimon_info_t));
	if (csimon_ctxt == NULL) {
		WL_ERROR(("wl%d: %s: csimon_ctxt MALLOCZ failed; total "
			"mallocs %d bytes\n", wlc->pub->unit,
			__FUNCTION__, MALLOCED(wlc->osh)));
		return NULL;
	}
	csimon_ctxt->wlc = wlc;

	if (D11REV_LT(wlc->pub->corerev, 129)) {
		WL_ERROR(("wl%d: %s: Not enabling CSIMON support for D11 rev %d\n",
			wlc->pub->unit, __FUNCTION__, wlc->pub->corerev));
		return csimon_ctxt;
	}

	/* STA list for handling multiple STAs for CSI */
	csimon_ctxt->num_clients = 0;
#ifdef BCM_CSIMON_AP
	csimon_ctxt->num_responder_aps = 0;
#endif /* BCM_CSIMON_AP */
	for (i = 0; i < CSIMON_MAX_STA; i++) {
		memset(&csimon_ctxt->sta_info[i], 0, sizeof(wlc_csimon_sta_t));
	}

#if !defined(DONGLEBUILD)
	csimon_ctxt->wd_timeout = CSIMON_NIC_POLL_MSEC;
	/* Initialize the CSIMON watchdog timer */
	csimon_ctxt->wd_timer = wl_init_timer(wlc->wl, wlc_csimon_wd_timer,
	                                      wlc, "csimon_wd");
	if (!(csimon_ctxt->wd_timer)) {
		WL_ERROR(("wl%d: csimon wd timer init failed \n", wlc->pub->unit));
		MFREE(wlc->pub->osh, csimon_ctxt, sizeof(wlc_csimon_info_t));
		return NULL;
	}
	CSIMON_DEBUG("wl%d:Init csimon wd timer %p timeout %u\n", wlc->pub->unit,
	              csimon_ctxt->wd_timer, csimon_ctxt->wd_timeout);
#endif /* ! DONGLEBUILD */

	/* Memory for CSI headers for FD and whole CSI records for NIC mode */
	align_bits = 6;	/* 64-Byte alignment */
	va = DMA_ALLOC_CONSISTENT(wlc->osh, mem_size, align_bits, &alloced,
		&csimon_ctxt->xfer.pa_orig, NULL);
	if (va == NULL) {
		WL_ERROR(("wl%d: %s: csimon local_ring_mem alloc failed; size %u "
			"align_bits %u depth %u\n",
			wlc->pub->unit, __FUNCTION__, mem_size, align_bits,
			CSIMON_LOCAL_RING_DEPTH));
#if !defined(DONGLEBUILD)
		wl_free_timer(wlc->wl, csimon_ctxt->wd_timer);
#endif /* ! DONGLEBUILD */
		MFREE(wlc->pub->osh, csimon_ctxt, sizeof(wlc_csimon_info_t));
		return NULL;
	}
	/* fix up alignment if needed */
	offset = ROUNDUP(PHYSADDRLO(csimon_ctxt->xfer.pa_orig), (1 << align_bits)) -
		PHYSADDRLO(csimon_ctxt->xfer.pa_orig);
	/* make sure we don't cross our boundaries */
	ASSERT(mem_size + offset <= alloced);

	csimon_ctxt->xfer.local_ring_base_orig = va;
	csimon_ctxt->xfer.local_ring_base = (void*)((uintptr)va + offset);
	csimon_ctxt->xfer.alloced = alloced;
#ifdef CSIMON_M2MCPY_BUILD
	csimon_ctxt->xfer.m2m_dd_csi = M2M_INVALID;
#endif /* CSIMON_M2MCPY_BUILD */

#ifndef CSIMON_PER_STA_TIMER
	/* Initialize per-radio CSI Timer */
	csimon_ctxt->csi_timer = wlc_hrt_alloc_timeout(wlc->hrti);
	if (!(csimon_ctxt->csi_timer)) {
		WL_ERROR(("wl%d: per radio csimon timer init failed \n", wlc->pub->unit));
		MFREE(wlc->pub->osh, csimon_ctxt->csi_timer, sizeof(wlc_csimon_info_t));
		return NULL;
	}
#endif
	/* Register module */
	if (wlc_module_register(
			    wlc->pub,
			    csimon_iovars,
			    "csimon",
			    csimon_ctxt,
			    wlc_csimon_doiovar,
			    NULL,
			    NULL,
			    NULL)) {
		WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
		          WLCUNIT(csimon_ctxt), __FUNCTION__));
#if !defined(DONGLEBUILD)
		wl_free_timer(wlc->wl, csimon_ctxt->wd_timer);
#endif /* ! DONGLEBUILD */
		DMA_FREE_CONSISTENT(wlc->osh, va, alloced, csimon_ctxt->xfer.pa_orig, NULL);
		MFREE(wlc->pub->osh, csimon_ctxt, sizeof(wlc_csimon_info_t));
		return NULL;
	}

	/* Register csimon dump function */
	wlc_dump_add_fns(wlc->pub, "csimon", wlc_csimon_dump, wlc_csimon_dump_clr,
	                 csimon_ctxt);

	printf("CSIMON module registered\n");
#if !defined(DONGLEBUILD)
#ifndef CSIMON_FILE_BUILD
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	{
		int nl_id = 0;

		/* There is a netlink socket per radio */
		if (wlc->pub->unit == 0) {
			nl_id = NETLINK_CSIMON0;
		} else if (wlc->pub->unit == 1) {
			nl_id = NETLINK_CSIMON1;
		} else { /* Third radio */
			nl_id = NETLINK_CSIMON2;
		}
		csimon_ctxt->nl_sock_id = nl_id;
		csimon_ctxt->nl_sock = netlink_kernel_create(&init_net, nl_id, NULL);
		if (csimon_ctxt->nl_sock == NULL) {
			csimon_ctxt->nl_sock_id = -1;
			WL_ERROR(("wl%d: Failed creating netlink: nl_id %d\n",
			           wlc->pub->unit, nl_id));
		} else {
			CSIMON_DEBUG("wl%d: Created netlink: nl_id %d\n",
			           wlc->pub->unit, nl_id);
		}
	}
#endif	/* LINUX_VERSION_CODE < 4 */
#endif /* ! CSIMON_FILE_BUILD */
#endif /* !DONGLEBUILD */
	return csimon_ctxt;
} // wlc_csimon_attach()

int // Initialize the global CSIMON object
wlc_csimon_init(wlc_info_t *wlc)
{
#if defined(DONGLEBUILD)
	uint64              hme_addr_u64;
	csimon_preamble_t * s2p_preamble;
#endif

	printf("CSIMON: " CSIMON_VRP_FMT " Initialization\n",
		CSIMON_VRP_VAL(CSIMON_VERSIONCODE));

	bcm_ring_init(&CSIxfer(wlc).local_ring);

#if defined(DONGLEBUILD)
	if (CSIxfer(wlc).hme != (csimon_ipc_hme_t *)NULL) {
		printf("CSIMON: already initialized ...\n");
		return BCME_OK;
	}

	bcm_ring_init(&CSIxfer(wlc).host_ring);

	/* Fetch the HME host address from the HME service for CSIMON user */
	CSIxfer(wlc).hme_haddr64    = hme_haddr64(HME_USER_CSIMON);
	CSIMON_ASSERT(!HADDR64_IS_ZERO(CSIxfer(wlc).hme_haddr64));
	CSIxfer(wlc).hme = (csimon_ipc_hme_t *) /* for use in offset computations */
	                   ((uintptr)HADDR64_LO(CSIxfer(wlc).hme_haddr64));

	HADDR64_TO_U64(CSIxfer(wlc).hme_haddr64, hme_addr_u64); // cast to uint64
	/* Setup a shared SB2PCIE window for user CSI in SBTOPCIE service */
	CSIxfer(wlc).s2p = (csimon_ipc_s2p_t *) /* map hme into S2P PIO window */
		sbtopcie_setup(SBTOPCIE_USR_CSI, hme_addr_u64, sizeof(csimon_ipc_s2p_t),
		               SBTOPCIE_MODE_SHARED);
	if ((uintptr)(CSIxfer(wlc).s2p) == SBTOPCIE_ERROR) {
		printf("CSIMON: sbtopcie_setup failure\n");
		ASSERT(0);
		return BCME_ERROR;
	}

	/* Switch shared window to user CSI */
	sbtopcie_switch(SBTOPCIE_USR_CSI);

	/* Setup preamble in HME using S2P PIO */
	s2p_preamble                = &(CSIxfer(wlc).s2p->preamble);
	s2p_preamble->version_code  = CSIMON_VERSIONCODE;
	s2p_preamble->elem_size     = CSIMON_RING_ITEM_SIZE;
	s2p_preamble->table_daddr32 = (uintptr) CSIxfer(wlc).hme->table;

	CSIMON_DEBUG("wl%d: versioncode %d ring elem size %d\n", wlc->pub->unit,
	             s2p_preamble->version_code, s2p_preamble->elem_size);
#endif /* DONGLEBUILD */

#ifdef CSIMON_M2MCPY_BUILD
	if (CSIxfer(wlc).m2m_dd_csi != M2M_INVALID) {
		printf("CSIMON: M2M usr already registered ...\n");
		return BCME_OK;
	}

	CSIxfer(wlc).m2m_dd_csi = m2m_dd_usr_register(wlc->osh, M2M_DD_CSI,
	                            M2M_DD_CH0, &CSIxfer(wlc), wlc_csimon_m2m_dd_done_cb,
	                            NULL, 0); // No wake cb, threshold
	CSIMON_DEBUG("wl%d: m2m_dd_key %d\n", wlc->pub->unit, CSIxfer(wlc).m2m_dd_csi);
#endif /* CSIMON_M2MCPY_BUILD */
	return BCME_OK;
}  // wlc_csimon_init()

void // Detach the CSIMON module
BCMATTACHFN(wlc_csimon_detach)(wlc_csimon_info_t *ci)
{
	wlc_info_t *wlc;
#ifdef CSIMON_PER_STA_TIMER
	int idx;
#endif

	CSIMON_ASSERT(ci);

	wlc = ci->wlc;
	CSIMON_ASSERT(wlc);

	/* Free the timers */
#if !defined(DONGLEBUILD)
	if (ci->wd_timer != NULL) {
		wl_free_timer(wlc->wl, ci->wd_timer);
	}
#endif /* ! DONGLEBUILD */
#ifdef CSIMON_PER_STA_TIMER
	for (idx = 0; idx < CSIMON_MAX_STA; idx++) {
		if (ci->sta_info[idx].timer != NULL) {
			wl_free_timer(wlc->wl, ci->sta_info[idx].timer);
		}
	}
#else
	/* Free up the per-radio timer */
	if (ci->csi_timer) {
		wlc_hrt_free_timeout(ci->csi_timer);
	}
#endif /* CSIMON_PER_STA_TIMER */

	if (ci->xfer.local_ring_base_orig) {
		/* Free the local memory for CSI headers/records */
		DMA_FREE_CONSISTENT(wlc->osh, (void *)ci->xfer.local_ring_base_orig,
				ci->xfer.alloced, ci->xfer.pa_orig, NULL);
	}
	/* Unregister the module */
	wlc_module_unregister(wlc->pub, "csimon", ci);

	/* Free the CSIMON data structure memory */
	MFREE(wlc->osh, ci, sizeof(wlc_csimon_info_t));

}  // wlc_csimon_detach()

static INLINE void // Reset valid bit in ucode SHM to indicate report has been read
__wlc_csimon_shm_update(wlc_info_t *wlc, uint16 seqn, uint16 svmp_rpt_idx)
{
	seqn &= ~ (1 << C_CSI_VLD_NBIT);
	wlc_write_shm(wlc, (M_CSI_VSEQN(wlc) + svmp_rpt_idx*C_CSI_BLK_SZ), seqn);
	CSIMON_DEBUG("wl%d: Reset valid bit: 0x%x to shmem 0x%x \n", wlc->pub->unit,
	             seqn, (M_CSI_VSEQN(wlc) + svmp_rpt_idx*C_CSI_BLK_SZ));
} // wlc_csimon_shm_update()

//#define CSIMON_PIO_CONSOLE_PRINT 1

static INLINE void // Print the CSI 'report' to the console
__wlc_csimon_rpt_print_console(wlc_info_t *wlc, wlc_csimon_rec_t *csimon_rec,
                               uint32 svmp_offset, uint16 seqn, uint16 rpt_idx)
{
#ifdef CSIMON_PIO_CONSOLE_PRINT
	uint16 mem_len = CSIMON_REPORT_SIZE / sizeof(uint16); /* 16-bit words */
	int i, j;
	uint num_col = 16, mem_to_dump = 256;
	uint16 rpt_words[(CSIMON_RING_ITEM_SIZE-CSIMON_HEADER_SIZE)/sizeof(uint16)];

	/* Copy the CSI report from SVMP memory to local memory */
	wlc_svmp_mem_read(wlc->hw, svmp_offset, mem_len, rpt_words);
	printf("wl%d:copied SVMP Offset 0x%x into local mem 0x%p\n",
			wlc->pub->unit, svmp_offset, rpt_words);

	/* Dump the report */
	for (i = 0; i < (mem_to_dump / num_col); i++) {
		for (j = 0; j < num_col; j++) {
			printf("0x%04x\t", rpt_words[i * num_col + j]);
		}
		printf("\n");
	}
#endif /* CSIMON_PIO_CONSOLE_PRINT */
} // __wlc_csimon_rpt_print_console()

static INLINE void // Read CSI header parameters from ucode shared memory
__wlc_read_shm_csimon_hdr(wlc_info_t *wlc, wlc_csimon_hdr_t *csimon_hdr, uint8 idx)
{
	uint32 tsf_l, tsf_h, rssi0, rssi1;

	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(csimon_hdr);

	/* MAC Address */
	csimon_hdr->client_ea[0] = wlc_read_shm(wlc,
	                               (M_CSI_MACADDRL(wlc) + idx*C_CSI_BLK_SZ));
	csimon_hdr->client_ea[1] = wlc_read_shm(wlc,
	                               (M_CSI_MACADDRM(wlc) + idx*C_CSI_BLK_SZ));
	csimon_hdr->client_ea[2] = wlc_read_shm(wlc,
	                               (M_CSI_MACADDRH(wlc) + idx*C_CSI_BLK_SZ));

	/* Time stamp using TSF register values */
	tsf_l = (uint32)wlc_read_shm(wlc, (M_CSI_RXTSFL(wlc) + idx*C_CSI_BLK_SZ));
	tsf_h = (uint32)wlc_read_shm(wlc, (M_CSI_RXTSFML(wlc) + idx*C_CSI_BLK_SZ));
	csimon_hdr->report_ts = tsf_l | (tsf_h << 16);

	/* RSSI for up to 4 antennae */
	rssi0 = (uint32)wlc_read_shm(wlc, (M_CSI_RSSI0(wlc) + idx*C_CSI_BLK_SZ));
	rssi1 = (uint32)wlc_read_shm(wlc, (M_CSI_RSSI1(wlc) + idx*C_CSI_BLK_SZ));
	csimon_hdr->rssi[0] = rssi0;
	csimon_hdr->rssi[1] = rssi0 >> 8;
	csimon_hdr->rssi[2] = rssi1;
	csimon_hdr->rssi[3] = rssi1 >> 8;

	CSIMON_DEBUG("wl%d:EA low 0x%x middle 0x%x high 0x%x\n"
	              "\tTSF low 0x%x hi 0x%x timestamp %u\n"
	              "\trssi0 0x%x rssi1 0x%x RSSI[0] 0x%0x RSSI[1] 0x%0x RSSI[2]"
		      " 0x%0x RSSI[3] 0x%0x\n",
		      wlc->pub->unit, csimon_hdr->client_ea[0],
		      csimon_hdr->client_ea[1], csimon_hdr->client_ea[2],
		      tsf_l, tsf_h, csimon_hdr->report_ts,
		      rssi0, rssi1, csimon_hdr->rssi[0], csimon_hdr->rssi[1],
		      csimon_hdr->rssi[2], csimon_hdr->rssi[3]);
} // __wlc_read_shm_csimon_hdr()

static INLINE int // Build the CSIMON header
__wlc_csimon_hdr_build(wlc_info_t * wlc, struct scb *scb,
                     wlc_csimon_hdr_t *csimon_hdr, uint16 *seqn)
{
	uint32 rd_idx;
	uint16 seq_no;
	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(scb);
	CSIMON_ASSERT(csimon_hdr);
	CSIMON_ASSERT(seqn);

	/* First set the CSI report format id/version. This should be a supporting
	 * MAC/PHY.
	 */
	if (D11REV_GE(wlc->pub->corerev, 129)) {
		((wlc_csimon_hdr_t *)csimon_hdr)->format_id = CSI_REPORT_FORMAT_ID;
	} else {
		WL_ERROR(("wl%d: CSIMON not supported on this radio chip\n",
				wlc->pub->unit));
		return BCME_EPERM;
	}
	CSIMON_DEBUG("D11 core rev %d \n", wlc->pub->corerev);
	CSIMON_DEBUG("seq no loc 0x%x\n", M_CSI_VSEQN(wlc));

	/* Get shmem params for CSI header as a part of overall CSI record */

	/* Get the sequence number that indicates if the CSI record is valid */
	seq_no = wlc_read_shm(wlc, (M_CSI_VSEQN(wlc) +
	                      CSIxfer(wlc).svmp_rd_idx*C_CSI_BLK_SZ));

	/* Note that the following code section works well for ping-pong buffers
	 * employed by the ucode. If that changes, this sections should be revisited.
	 */
	if (seq_no & (1 << C_CSI_VLD_NBIT)) {
		/* Now read the header parameters from SHM */
		__wlc_read_shm_csimon_hdr(wlc, csimon_hdr, CSIxfer(wlc).svmp_rd_idx);
		CSIMON_DEBUG("wl%d:***************svmp_rd_idx %u\n",
				wlc->pub->unit, CSIxfer(wlc).svmp_rd_idx);
	} else {  // Try the other SHM record
		CSIMON_DEBUG("wl%d:valid bit NOT SET; try next record\n",
				wlc->pub->unit);
		rd_idx =
			(CSIxfer(wlc).svmp_rd_idx + 1) % CSIMON_SVMP_RING_DEPTH;
		seq_no = wlc_read_shm(wlc, (M_CSI_VSEQN(wlc) + rd_idx*C_CSI_BLK_SZ));
		if (seq_no & (1 << C_CSI_VLD_NBIT)) {
			CSIxfer(wlc).svmp_rd_idx = rd_idx;
			__wlc_read_shm_csimon_hdr(wlc, csimon_hdr, rd_idx);
			CSIMON_DEBUG("wl%d:==================svmp_rd_idx %u\n",
					wlc->pub->unit, CSIxfer(wlc).svmp_rd_idx);
			scb->csimon->rpt_invalid_once_cnt++;
		} else {
			/* Increment a failure cnt as both the CSI records are invalid */
			scb->csimon->rpt_invalid_cnt++;
			WL_ERROR(("wl%d: Both SVMP CSI records invalid: idx %u seq_no 0x%x\n",
					wlc->pub->unit, CSIxfer(wlc).svmp_rd_idx, seq_no));
			return BCME_NOTREADY;
		}
	}
	*seqn = seq_no;
	/* Number of tx rx streams, chanspec with channel, bandwidth */
	/* The number of tx streams are typically ignored by the user-space app.
	   Here we were getting the streams of the AP radio whereas the app needs to
	   know those of the STA. But anyways the CSI is captured at the
	   L-LTF and the H-matrix is collapsed to 1 Tx stream.
	 */
	csimon_hdr->txstreams = 1; //wlc->stf->op_txstreams;
	csimon_hdr->rxstreams = wlc->stf->op_rxstreams;
	csimon_hdr->chanspec = scb->bsscfg->current_bss->chanspec;

	/* BSSID and association timestamp */
	csimon_hdr->bss_ea[0] = scb->bsscfg->BSSID.octet[0] |
	                        scb->bsscfg->BSSID.octet[1] << 8;
	csimon_hdr->bss_ea[1] = scb->bsscfg->BSSID.octet[2] |
	                        scb->bsscfg->BSSID.octet[3] << 8;
	csimon_hdr->bss_ea[2] = scb->bsscfg->BSSID.octet[4] |
	                        scb->bsscfg->BSSID.octet[5] << 8;

	csimon_hdr->assoc_ts = scb->csimon->assoc_ts;
	CSIMON_DEBUG("wl%d: Format id %u Tx streams 0x%x Rx streams 0x%x chanspec "
				 "0x%0x\n", wlc->pub->unit, csimon_hdr->format_id,
				 csimon_hdr->txstreams, csimon_hdr->rxstreams,
				 csimon_hdr->chanspec);
	CSIMON_DEBUG("wl%d: Assoc TS %u BSS[0] 0x%x BSS[1] 0x%x BSS[2] 0x%x\n",
			wlc->pub->unit, csimon_hdr->assoc_ts, csimon_hdr->bss_ea[0],
			csimon_hdr->bss_ea[1], csimon_hdr->bss_ea[2]);

	return BCME_OK;
}

//#define CSIMON_HEADER_ONLY

int // Copy to host DDR STA CSI record: report from SVMP + header in driver
wlc_csimon_record_copy(wlc_info_t *wlc, scb_t *scb)
{
	uint32 svmp_offset;		/* Offset of csi_rpt0 or csi_rpt1 in SVMP memory */
	uint32 *svmp_addrp_u32;         /* Actual address of the csi_rpt in SVMP memory */
	uint16 seqn = 0;		/* Seq no field from SHM - managed by ucode */
	int curr_wr_idx, next_wr_idx;   /* current and next write indices */
	int ret = BCME_OK;
	wlc_csimon_hdr_t *csimon_hdr;
	void *csimon_unit;
#if !defined(DONGLEBUILD)
	wlc_csimon_rec_t *csimon_rec;
	BCM_REFERENCE(csimon_rec);
#endif /* ! DONGLEBUILD */

	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(scb);

	BCM_REFERENCE(svmp_addrp_u32);
	BCM_REFERENCE(curr_wr_idx);
	BCM_REFERENCE(next_wr_idx);
	BCM_REFERENCE(csimon_hdr);
	BCM_REFERENCE(csimon_unit);

	scb->csimon->null_frm_ack_cnt++;

#ifdef CSIMON_PIO_CONSOLE_PRINT
	/* Print the CSI report to console - use for debugging. */
	svmp_offset = VASIP_SHARED_OFFSET(wlc->hw, csi_rpt0);
	__wlc_csimon_rpt_print_console(wlc, NULL, svmp_offset, seqn,
	                               CSIxfer(wlc).svmp_rd_idx);
	return ret;
#endif /* CSIMON_PIO_CONSOLE_PRINT */

	if ((curr_wr_idx = bcm_ring_prod_pend(&CSIxfer(wlc).local_ring,
	                   &next_wr_idx, CSIMON_LOCAL_RING_DEPTH)) ==
	                   BCM_RING_FULL) {
		CSIxfer(wlc).drops++;
#if !defined(DONGLEBUILD)
		/* For NIC mode, this ring has the whole CSI records */
		wlc->csimon_info->state.rec_ovfl_cnt++;
		scb->csimon->rec_ovfl_cnt++;
#endif /* ! DONGLEBUILD */
		return BCME_NORESOURCE;
	}
#if defined(DONGLEBUILD)
	csimon_hdr = CSIxfer(wlc).local_ring_base + curr_wr_idx;
	csimon_unit = csimon_hdr;
#else /* ! DONGLEBUILD */
	/* Cache local write index to be used in the xfer completion callback */
	CSIxfer(wlc).local_wr_idx = next_wr_idx;
	csimon_rec = CSIxfer(wlc).local_ring_base + curr_wr_idx;
	csimon_hdr = &csimon_rec->csimon_hdr;
	csimon_unit = csimon_rec;

	CSIMON_DEBUG("local ring base %p local curr wr idx %d csimon_rec VA %p\n",
		CSIxfer(wlc).local_ring_base, curr_wr_idx, csimon_rec);
#endif /* ! DONGLEBUILD */

	ret = __wlc_csimon_hdr_build(wlc, scb, csimon_hdr, &seqn);

#if defined(CSIMON_HEADER_ONLY) && !defined(DONGLEBUILD) // For debugging only
	if (ret == BCME_OK) {
		/* Indicate/lie to the ucode that the CSI report has been copied */
		__wlc_csimon_shm_update(wlc, seqn, CSIxfer(wlc).svmp_rd_idx);

		CSIxfer(wlc).svmp_rd_idx = (CSIxfer(wlc).svmp_rd_idx + 1) %
		                           CSIMON_SVMP_RING_DEPTH;
		/* Commit the local ring write index and lie - increment the xfer cnt */
		bcm_ring_prod_done(&CSIxfer(wlc).local_ring, CSIxfer(wlc).local_wr_idx);
		scb->csimon->m2mxfer_cnt++;
		wlc->csimon_info->state.m2mxfer_cnt++;
	}
	return ret;
#else /* ! (CSIMON_HEADER_ONLY && !DONGLEBUILD) */
	if (ret != BCME_OK) {
		return ret;
	}
#endif /* ! (CSIMON_HEADER_ONLY && !DONGLEBUILD) */

	/* Transfer the whole CSI record that includes CSI header and CSI report
	 * (H-matrix). The CSI header consists of MAC addr, RSSI etc.
	 */

	/* Source address of the CSI report in SVMP memory */
	svmp_offset = VASIP_SHARED_OFFSET(wlc->hw, csi_rpt0);
	if (CSIxfer(wlc).svmp_rd_idx == 1) {
		svmp_offset = VASIP_SHARED_OFFSET(wlc->hw, csi_rpt1);
	}

	svmp_addrp_u32 = wlc_vasip_addr_int(wlc->hw, svmp_offset);

	/* Fill in the callback data that is needed after M2M xfer is done */
	scb->csimon->m2m_cb_data.scb = scb;
	scb->csimon->m2m_cb_data.seqn = seqn;
	scb->csimon->m2m_cb_data.svmp_rpt_idx = CSIxfer(wlc).svmp_rd_idx;

	ret = __wlc_csimon_xfer(wlc, scb, csimon_unit, CSIMON_RING_ITEM_SIZE,
	                      svmp_addrp_u32, (void*)(&(scb->csimon->m2m_cb_data)));
	if (ret == BCME_OK) {
		scb->csimon->m2mcpy_busy = TRUE;
#if defined(DONGLEBUILD)
		/* Commit the local ring (CSI header) write index, now */
		bcm_ring_prod_done(&CSIxfer(wlc).local_ring, next_wr_idx);
#endif /* DONGLEBUILD */
	}
	else {
		/* Invalidate the SHM record so that ucode can fetch the next CSI */
		__wlc_csimon_shm_update(wlc, seqn, CSIxfer(wlc).svmp_rd_idx);
	}
	CSIxfer(wlc).svmp_rd_idx = (CSIxfer(wlc).svmp_rd_idx + 1) %
	                           CSIMON_SVMP_RING_DEPTH;

	return ret;
}   // wlc_csimon_record_copy()

void // Handle Null frame ack failure
wlc_csimon_ack_failure_process(wlc_info_t *wlc, scb_t *scb)
{
	CSIMON_ASSERT(wlc);
	CSIMON_ASSERT(wlc->csimon_info);
	CSIMON_ASSERT(scb);
	CSIMON_ASSERT(scb->csimon);

	/* Increment the client level and CSIMON level failure counters */
	scb->csimon->ack_fail_cnt++;
	wlc->csimon_info->state.ack_fail_cnt++;

	WL_ERROR(("%s: csimon ack failure; %u for "MACF" \n", __FUNCTION__,
	          scb->csimon->ack_fail_cnt, ETHER_TO_MACF(scb->ea)));

} // wlc_csimon_ack_failure_process()

#if !defined(DONGLEBUILD)
//#define CSIMON_M2M_CONSOLE_PRINT
static INLINE void // Print the 'm2m xfer'ed CSI report' and header to console
__wlc_csimon_rec_console_print(wlc_info_t *wlc, wlc_csimon_rec_t *csimon_rec)
{
#ifdef CSIMON_M2M_CONSOLE_PRINT
	int i, j;
	uint num_col = 8, mem_to_dump = 64;
	uint32 *rpt_words = (uint32 *)csimon_rec;

	/* Dump the record */
	printf("CSI record at %p\n", csimon_rec);
	for (i = 0; i < (mem_to_dump / num_col); i++) {
		for (j = 0; j < num_col; j++) {
			printf("0x%08x\t", rpt_words[i * num_col + j]);
		}
		printf("\n");
	}
#endif /* CSIMON_M2M_CONSOLE_PRINT */
} // __wlc_csimon_rec_console_print()
#endif /* ! DONGLEBUILD */

void // function callback at the completion of the DD-based M2M xfer
wlc_csimon_m2m_dd_done_cb(void *usr_cbdata,
    dma64addr_t *xfer_src, dma64addr_t *xfer_dst, int xfer_len, void *xfer_arg)
{
	csimon_xfer_t *csimon = (csimon_xfer_t *)usr_cbdata;
	csimon_m2m_cb_data_t *m2m_cb_data;
	wlc_info_t *wlc;
	uint16 seqn, svmp_rpt_idx;

	BCM_REFERENCE(csimon);
	m2m_cb_data = (csimon_m2m_cb_data_t *)(xfer_arg);
	CSIMON_ASSERT(m2m_cb_data->scb);
	CSIMON_ASSERT(m2m_cb_data->scb->bsscfg);
	wlc = m2m_cb_data->scb->bsscfg->wlc;
	CSIMON_ASSERT(wlc);
	seqn = m2m_cb_data->seqn;
	svmp_rpt_idx = m2m_cb_data->svmp_rpt_idx;
	BCM_REFERENCE(svmp_rpt_idx);
	BCM_REFERENCE(seqn);

#if defined(DONGLEBUILD)
	sbtopcie_switch(SBTOPCIE_USR_CSI); // activate User CSI's sbtopcie window

	/* Post the updated write index given that the xfer is successful */
	CSIxfer(wlc).s2p->preamble.write_idx.u32 = CSIxfer(wlc).host_ring.write;

	/* CSI header ring item is consumed */
	bcm_ring_cons(&CSIxfer(wlc).local_ring, CSIMON_LOCAL_RING_DEPTH);
#else /* ! DONGLEBUILD */
	/* Commit the local ring write index, now */
	bcm_ring_prod_done(&CSIxfer(wlc).local_ring, CSIxfer(wlc).local_wr_idx);
#endif /* ! DONGLEBUILD */

	m2m_cb_data->scb->csimon->m2mcpy_busy = FALSE;
	m2m_cb_data->scb->csimon->m2mxfer_cnt++;
	wlc->csimon_info->state.m2mxfer_cnt++;
	CSIMON_DEBUG("wl%d:xfer cnt %u\n", wlc->pub->unit,
			m2m_cb_data->scb->csimon->m2mxfer_cnt);

	/* Indicate to the ucode that the CSI report has been copied */
	__wlc_csimon_shm_update(wlc, seqn, svmp_rpt_idx);

#if !defined(DONGLEBUILD) && defined(CSIMON_M2M_CONSOLE_PRINT)
	/* Print CSI report (partially) to the console for quick verification */
	int elem_idx;
	wlc_csimon_rec_t *elem;

	elem_idx = bcm_ring_cons(&CSIxfer(wlc).local_ring, CSIMON_LOCAL_RING_DEPTH);
	elem = CSIMON_RING_IDX2ELEM(CSIxfer(wlc).local_ring_base, elem_idx);
	OSL_CACHE_INV((void*)(elem), CSIMON_RING_ITEM_SIZE);
	CSIMON_DEBUG("wl%d:rd idx %d csimon_rec %p\n", wlc->pub->unit,
			elem_idx, elem);
	__wlc_csimon_rec_console_print(wlc, elem);

	/* Transfer over netlink socket */
	if (wlc_csimon_netlink_send(wlc->csimon_info, (void *) elem,
		CSIMON_RING_ITEM_SIZE)) {
		wlc->csimon_info->state.usr_xfer_fail_cnt++;
		WL_ERROR(("%s: CSIMON drops with nl send %d\n", __FUNCTION__,
		          wlc->csimon_info->state.usr_xfer_fail_cnt));
	}
	else {
		wlc->csimon_info->state.usrxfer_cnt++;
	}
#endif /* ! DONGLEBUILD && CSIMON_M2M_CONSOLE_PRINT */

} // wlc_csimon_m2m_dd_done_cb()

//-------------- IOVAR and client/station list management --------------------//

static INLINE int8 // Return the idx of the STA with given MAC address in the MAC list
wlc_csimon_sta_find(wlc_csimon_info_t *csimon_ctxt, const struct ether_addr *ea)
{
	int i;

	CSIMON_ASSERT(csimon_ctxt);
	CSIMON_ASSERT(ea);

	for (i = 0; i < CSIMON_MAX_STA; i++) {
		if (bcmp(ea, &csimon_ctxt->sta_info[i].ea, ETHER_ADDR_LEN) == 0)
			return i;
	}

	return -1;
}
#ifndef CSIMON_PER_STA_TIMER
#define CSIMON_PER_RADIO_TIMER_TIMEOUT_MSEC  1
static INLINE int wlc_csimon_sendnull_or_probereq_frame(scb_t *scb)
{
	wlc_info_t *wlc;
	ratespec_t rate_override = 0;
	CSIMON_ASSERT(scb);

	wlc = scb->bsscfg->wlc;

	if (BAND_2G(wlc->band->bandtype)) {
		/* use the lowest non-BPHY rate for 2G QoS Null frame */
		rate_override = CSIMON_NULL_FRAME_RSPEC;
	}
	if (CSIMON_ENAB(wlc->pub) &&
#ifdef BCM_CSIMON_AP
	(!(wlc_csimon_ap_ssid_len_check(scb))) &&
#endif
	TRUE) {
		/* Send null frame for CSI Monitor (Assoc-STA case) */
		if (!wlc_sendnulldata(wlc, scb->bsscfg, &scb->ea, rate_override,
			WLF_CSI_NULL_PKT, PRIO_8021D_VO, NULL, NULL)) {
				CSIMON_DEBUG("wl%d.%d: %s: wlc_sendnulldata failed for DA "MACF"\n",
						wlc->pub->unit, WLC_BSSCFG_IDX(scb->bsscfg),
					__FUNCTION__, ETHER_TO_MACF(scb->ea));
			return BCME_ERROR;
		} else {
			CSIMON_DEBUG("wl%d: !!!!!!!!sent null frame for DA "MACF"\n",
				wlc->pub->unit, ETHER_TO_MACF(scb->ea));
				wlc->csimon_info->state.null_frm_cnt++;
				scb->csimon->null_frm_cnt++;
		}
	}
#ifdef BCM_CSIMON_AP
	if ((CSIMON_ENAB(wlc->pub) &&
		wlc_csimon_ap_ssid_len_check(scb))) {
		/* Send PROBE REQ frame for CSI Monitor (AP to AP case) */
		/* GET THE da MAC address from csimon struct */
		wlc_sendprobe(wlc, scb->bsscfg, scb->csimon->SSID, scb->csimon->ssid_len,
				0, NULL, &(wlc->primary_bsscfg->cur_etheraddr),
				&scb->csimon->ea, &scb->csimon->ea,
				wlc_lowest_basic_rspec(wlc, &wlc->band->hw_rateset),
				NULL,
				FALSE);
		CSIMON_DEBUG("wl%d: !!!!!!!!sent a probe req frame for DA "MACF" with SSID:%s\n",
				wlc->pub->unit, ETHER_TO_MACF(scb->csimon->ea), scb->csimon->SSID);
		wlc->csimon_info->state.null_frm_cnt++;
		scb->csimon->null_frm_cnt++;
	}
#endif /* BCM_CSIMON_AP */
	return BCME_OK;
}

static void // CSIMON per-radio Timer
wlc_csimon_timer(void *arg)
{
	wlc_csimon_info_t *csi_ctxt = (wlc_csimon_info_t *) arg;
	scb_t *scb;
	wlc_info_t *wlc;
	int8 idx;
	uint32 current_time, delta;

	CSIMON_ASSERT(csi_ctxt);
	wlc = csi_ctxt->wlc;
	CSIMON_ASSERT(wlc);

	/* Find The CSIMON configured STAs in list */
	for (idx = 0; idx < CSIMON_MAX_STA; idx++)
	{
		if (bcmp(&ether_null, &csi_ctxt->sta_info[idx].ea, ETHER_ADDR_LEN) == 0) {
			continue;
		}
#ifdef BCM_CSIMON_AP
		/* CSIMON_AP ensure this is AP AP case by checking ssid_len */
		if ((CSIMON_ENAB(wlc->pub) && (csi_ctxt->sta_info[idx].ssid_len != 0))) {
			scb = wlc->band->hwrs_scb;
			CSIMON_DEBUG("wl%d: csimon found scb for a AP AP case "MACF"\n",
				wlc->pub->unit, ETHER_TO_MACF(csi_ctxt->sta_info[idx].ea));
		} else
#endif
		{
			scb = wlc_scbapfind(wlc, &csi_ctxt->sta_info[idx].ea);
			CSIMON_DEBUG("wl%d: csimon found scb for a Assoc STA case "MACF"\n",
			wlc->pub->unit, ETHER_TO_MACF(csi_ctxt->sta_info[idx].ea));
		}
		if (scb != NULL) {
			current_time = OSL_SYSUPTIME();
			if (csi_ctxt->sta_info[idx].prev_time == 0) {
				CSIMON_DEBUG("wl%d: First time Hit STAId:%d "MACF"\n",
				wlc->pub->unit, idx, ETHER_TO_MACF(csi_ctxt->sta_info[idx].ea));
				if (wlc_csimon_sendnull_or_probereq_frame(scb) != BCME_ERROR) {
					CSIMON_DEBUG("wl%d: !!!!sent null frame for DA "MACF" \n",
							wlc->pub->unit, ETHER_TO_MACF(scb->ea));
					csi_ctxt->sta_info[idx].delta_time = current_time;
					csi_ctxt->sta_info[idx].prev_time = current_time;
				}
				break;
			}
			delta = current_time > csi_ctxt->sta_info[idx].prev_time ?
				(current_time - (csi_ctxt->sta_info[idx].prev_time)) :
				((uint32)~0 -current_time + csi_ctxt->sta_info[idx].prev_time + 1);

			if (delta > csi_ctxt->sta_info[idx].timeout) {
				CSIMON_DEBUG("wl%d: STAid:%d sending QOS NULL FRAME FOR "MACF"\n",
					wlc->pub->unit, idx, ETHER_TO_MACF(scb->ea));
				if (wlc_csimon_sendnull_or_probereq_frame(scb) != BCME_ERROR) {
				CSIMON_DEBUG("wl%d: !!!!!!!!sent null frame for DA "MACF" \n",
						wlc->pub->unit, ETHER_TO_MACF(scb->ea));
				csi_ctxt->sta_info[idx].delta_time = delta;
				/*	current_time - csi_ctxt->sta_info[idx].prev_time; */
				csi_ctxt->sta_info[idx].prev_time = current_time;
				}
			}
		}
	}
	wlc_hrt_del_timeout(csi_ctxt->csi_timer);
	wlc_hrt_add_timeout(csi_ctxt->csi_timer,
			CSIMON_PER_RADIO_TIMER_TIMEOUT_MSEC * DOT11_TU_TO_US,
			wlc_csimon_timer, csi_ctxt);
}
#endif /* CSIMON_PER_STA_TIMER */

static int // Start CSI monitoring for all the stations in the list
wlc_csimon_enable_all_stations(wlc_csimon_info_t *ctxt)
{
	int8 idx;
	uint32 tsf_l;
	scb_t *scb;
#ifdef BCM_CSIMON_AP
	wlc_info_t *wlc;
#endif
	CSIMON_ASSERT(ctxt);
#ifdef BCM_CSIMON_AP
	wlc = ctxt->wlc;
#endif
	CSIMON_DEBUG("wl%d: Enabling all %d stations\n", ctxt->wlc->pub->unit,
	             ctxt->num_clients);

	/* Start CSIMON timer for the STAs in list */
	for (idx = 0; idx < CSIMON_MAX_STA; idx++) {
		if (bcmp(&ether_null, &ctxt->sta_info[idx].ea, ETHER_ADDR_LEN) == 0) {
			continue;
		}
#ifdef BCM_CSIMON_AP
		/* CSIMON_AP ensure this is AP AP case by checking ssid_len */
		if (CSIMON_ENAB_AP(WLCPUB(ctxt)) && (ctxt->sta_info[idx].ssid_len != 0)) {
			scb = wlc->band->hwrs_scb;
			CSIMON_DEBUG("wl%d: csimon found scb for a AP AP case "MACF"\n",
				ctxt->wlc->pub->unit, ETHER_TO_MACF(ctxt->sta_info[idx].ea));
		} else
#endif
		{
			scb = wlc_scbapfind(ctxt->wlc, &ctxt->sta_info[idx].ea);
			CSIMON_DEBUG("wl%d: csimon found scb for a Assoc STA case "MACF"\n",
			ctxt->wlc->pub->unit, ETHER_TO_MACF(ctxt->sta_info[idx].ea));
		}

		if ((scb && SCB_ASSOCIATED(scb)) ||
#ifdef BCM_CSIMON_AP
		((CSIMON_ENAB_AP(WLCPUB(ctxt))) && (ctxt->sta_info[idx].ssid_len != 0))||
#endif
		TRUE) {
			scb->csimon = &ctxt->sta_info[idx];
			/* CSI Monitoring start timestamp as a reference - TSF reg */
#ifdef BCM_CSIMON_AP
			/* CSIMON_AP tsf_l is zero for a unsoliciated (AP to AP)case */
			if ((CSIMON_ENAB_AP(WLCPUB(ctxt))) && (ctxt->sta_info[idx].ssid_len != 0)) {
				tsf_l = 0;
			} else
#endif
			{
				wlc_read_tsf(ctxt->wlc, &tsf_l, NULL);
			}
			ctxt->sta_info[idx].assoc_ts = tsf_l;
#ifdef CSIMON_PER_STA_TIMER
			/* Initialize the per-STA timer */
			ctxt->sta_info[idx].timer = wl_init_timer(ctxt->wlc->wl,
			                               wlc_csimon_scb_timer, scb, "csimon");
			if (!(ctxt->sta_info[idx].timer)) {
				WL_ERROR(("wl%d: csimon timer init failed for "MACF"\n",
				  ctxt->wlc->pub->unit, ETHER_TO_MACF(ctxt->sta_info[idx].ea)));
				return BCME_NORESOURCE;
			}
			/* Start the per-STA timer */
			wl_add_timer(ctxt->wlc->wl, ctxt->sta_info[idx].timer,
			             ctxt->sta_info[idx].timeout, TRUE);
			CSIMON_DEBUG("wl%d: started CSI timer for SCB DA "MACF" assocTS %u\n",
					ctxt->wlc->pub->unit,
					ETHER_TO_MACF(ctxt->sta_info[idx].ea), tsf_l);
#endif /* CSIMON_PER_STA_TIMER */
		}
	}
#ifndef CSIMON_PER_STA_TIMER
	/* Start the per-radio Hrt timer */
	wlc_hrt_add_timeout(ctxt->csi_timer,
			CSIMON_PER_RADIO_TIMER_TIMEOUT_MSEC * DOT11_TU_TO_US,
			wlc_csimon_timer, ctxt);
	CSIMON_DEBUG("wl%d: started per-radio HRT Timer with timeout %d\n",
			ctxt->wlc->pub->unit, CSIMON_PER_RADIO_TIMER_TIMEOUT_MSEC);
#endif /* CSIMON_PER_STA_TIMER */
	return BCME_OK;
}

static void // Stop CSI monitoring all the stations in the list
wlc_csimon_disable_all_stations(wlc_csimon_info_t *ctxt)
{
#ifdef CSIMON_PER_STA_TIMER
	int8 idx;
#endif /* CSIMON_PER_STA_TIMER */

	CSIMON_ASSERT(ctxt);

	CSIMON_DEBUG("wl%d: Disabling all %d stations\n", ctxt->wlc->pub->unit, ctxt->num_clients);
#ifdef CSIMON_PER_STA_TIMER
	for (idx = 0; idx < CSIMON_MAX_STA; idx++) {
		/* Stop/Free STA timer */
		if (ctxt->sta_info[idx].timer != NULL) {
			wl_del_timer(ctxt->wlc->wl, ctxt->sta_info[idx].timer);
			wl_free_timer(ctxt->wlc->wl, ctxt->sta_info[idx].timer);
			ctxt->sta_info[idx].timer = NULL;
		}
	}
#else
	CSIMON_DEBUG("wl%d: Deleting per-radio timer\n", ctxt->wlc->pub->unit);
	wlc_hrt_del_timeout(ctxt->csi_timer);
#endif /* CSIMON_PER_STA_TIMER */
}

static void // Clear all monitored stations from the list
wlc_csimon_delete_all_stations(wlc_csimon_info_t *ctxt)
{
	int8 idx;

	CSIMON_ASSERT(ctxt);

	CSIMON_DEBUG("wl%d: Deleting all %d stations\n", ctxt->wlc->pub->unit,
	             ctxt->num_clients);
	for (idx = 0; idx < CSIMON_MAX_STA; idx++) {
		if (ETHER_ISNULLADDR(&(ctxt->sta_info[idx].ea)))
			continue;
#ifdef CSIMON_PER_STA_TIMER
		/* Stop/Free STA timer */
		if (ctxt->sta_info[idx].timer != NULL) {
			wl_del_timer(ctxt->wlc->wl, ctxt->sta_info[idx].timer);
			wl_free_timer(ctxt->wlc->wl, ctxt->sta_info[idx].timer);
			ctxt->sta_info[idx].timer = NULL;
		}
#else
		ctxt->sta_info[idx].prev_time  = 0;
		ctxt->sta_info[idx].delta_time = 0;
#endif /* CSIMON_PER_STA_TIMER */

#ifdef BCM_CSIMON_AP
		if (ctxt->sta_info[idx].ssid_len != 0) {
			/* This is AP AP case need to handle */
			ctxt->num_responder_aps--;
		}
#endif
		bcopy(&ether_null, &ctxt->sta_info[idx].ea, ETHER_ADDR_LEN);
		ctxt->num_clients--;
	}
}

static int // Copy configured MAC addresses to the output maclist
wlc_csimon_sta_maclist_get(wlc_csimon_info_t *ctxt, struct maclist *ml)
{
	uint i, j;

	if (ctxt == NULL)
		return BCME_UNSUPPORTED;

	CSIMON_ASSERT(ml != NULL);

	/* ml->count contains maximum number of MAC addresses ml can carry. */
	for (i = 0, j = 0; i < CSIMON_MAX_STA && j < ml->count; i++) {
		if (!ETHER_ISNULLADDR(&ctxt->sta_info[i].ea)) {
			bcopy(&ctxt->sta_info[i].ea, &ml->ea[j], ETHER_ADDR_LEN);
			j++;
		}
	}
	/* return the number of copied MACs to the maclist */
	ml->count = j;

	return BCME_OK;
}

static int // List the clients being monitored by CSIMON module with get iovar
wlc_csimon_process_get_cmd_options(wlc_csimon_info_t *ctxt, void* param,
	int paramlen, void* bptr, int len)
{
	int err = BCME_ERROR;
	struct maclist *maclist;

	maclist = (struct maclist *) bptr;

	if (len < (int)(sizeof(maclist->count) +
			(ctxt->num_clients * sizeof(*(maclist->ea))))) {
		return BCME_BUFTOOSHORT;
	}
	err = wlc_csimon_sta_maclist_get(ctxt, maclist);
	return err;
}

/*
 * Process commands passed via cfg->cmd parameter
 *
 * CSIMON_CFG_CMD_ENB command enables CSI Monitor feature at runtime
 *
 * CSIMON_CFG_CMD_DSB command disables CSI Monitor feature at runtime
 *
 * CSIMON_CFG_CMD_ADD command adds given MAC address of the STA into the STA
 *
 * CSIMON_CFG_CMD_ADD_AP command adds given MAC address of the responder AP
 * list and start capturing CSI if associated. The address must be valid unicast
 *
 * CSIMON_CFG_CMD_DEL command removes given MAC address of the STA from the STA
 * list and stops capturing CSI. The address must be valid unicast or broadcast.
 * The broadcast mac address specification clears all stations in the list.
 *
 * Return values:
 * BCME_UNSUPPORTED - feature is not supported
 * BCME_BADARG - invalid argument
 * BCME_NORESOURCE - no more entry in the STA list
 * BCME_ERROR - feature is supported but not enabled
 * BCME_NOTFOUND - entry not found
 * BCME_OK - success
 */
static int
wlc_csimon_sta_config(wlc_csimon_info_t *ctxt, wlc_csimon_sta_config_t *cfg)
{
	int8 idx;
	int ret;
	wlc_info_t *wlc;

	if (ctxt == NULL)
		return BCME_UNSUPPORTED;

	CSIMON_ASSERT(cfg);

	wlc = ctxt->wlc;

	CSIMON_DEBUG("wl%d stations %d \n", wlc->pub->unit, ctxt->num_clients);
	switch (cfg->cmd) {
	case CSIMON_CFG_CMD_DSB:
		if (CSIMON_ENAB(WLCPUB(ctxt))) {
			/* Disable all existing STAs including deleting timer */
			wlc_csimon_disable_all_stations(ctxt);
#ifdef BCM_CSIMON_AP
			/* Disable CSI Monitor feature */
			WLCPUB(ctxt)->_csimon_ap = FALSE;
#endif
			WLCPUB(ctxt)->_csimon = FALSE;

			CSIMON_DEBUG("wl%d %d stations cfg_cmd %d\n", wlc->pub->unit,
			              ctxt->num_clients, cfg->cmd);
#if !defined(DONGLEBUILD)
			wl_del_timer(wlc->wl, ctxt->wd_timer);
			CSIMON_DEBUG("wl%d deleted wd Timer %d ms\n", wlc->pub->unit,
			             ctxt->wd_timeout);
#endif /* ! DONGLEBUILD */
		}
		break;
	case CSIMON_CFG_CMD_ENB:
		/* CSIMON supported on D11 core rev 129 and above */
		if (!D11REV_GE(wlc->pub->corerev, 129)) {
			WL_ERROR(("wl%d: CSI Monitor not supported on this radio chip! \n",
				WLCUNIT(ctxt)));
			return BCME_EPERM;
		}
		if (!CSIMON_ENAB(WLCPUB(ctxt))) {
			/* Enable CSI Monitor feature */
			WLCPUB(ctxt)->_csimon = TRUE;
#ifdef BCM_CSIMON_AP
			WLCPUB(ctxt)->_csimon_ap = TRUE;
#endif
			/* Enable all existing STAs including starting timer */
			if ((ret = wlc_csimon_enable_all_stations(ctxt)) != BCME_OK)
				return ret;
			CSIMON_DEBUG("wl%d stations %d cfg_cmd %d\n", wlc->pub->unit,
			             ctxt->num_clients, cfg->cmd);
#if !defined(DONGLEBUILD)
			wl_add_timer(wlc->wl, ctxt->wd_timer, ctxt->wd_timeout, TRUE);
			CSIMON_DEBUG("wl%d added tmr %d ms\n", wlc->pub->unit,
			             ctxt->wd_timeout);
#endif /* ! DONGLEBUILD */

		}
		break;
	case CSIMON_CFG_CMD_ADD:
		/* CSIMON supported on D11 core rev 129 and above */
		if (!D11REV_GE(wlc->pub->corerev, 129)) {
			WL_ERROR(("wl%d: CSI Monitor not supported on this radio chip! \n",
				WLCUNIT(ctxt)));
			return BCME_EPERM;
		}
		/* The MAC address must be a valid unicast address */
		if (ETHER_ISNULLADDR(&(cfg->ea)) || ETHER_ISMULTI(&(cfg->ea))) {
			WL_ERROR(("wl%d: %s: Invalid MAC address.\n",
				WLCUNIT(ctxt), __FUNCTION__));
			return BCME_BADARG;
		}
		/* Search existing entry in the list */
		idx = wlc_csimon_sta_find(ctxt, &cfg->ea);
		if (idx < 0) {
			/* Search free entry in the list */
			idx = wlc_csimon_sta_find(ctxt, &ether_null);
			if (idx < 0) {
				WL_ERROR(("wl%d:%s: CSIMON MAC list is full (%u clients) and "
				          "can't add ["MACF"] \n", WLCUNIT(ctxt), __FUNCTION__,
				          CSIMON_MAX_STA, ETHERP_TO_MACF(&cfg->ea)));
				return BCME_NORESOURCE;
			}
			/* Add MAC address to the list */
			bcopy(&cfg->ea, &ctxt->sta_info[idx].ea, ETHER_ADDR_LEN);
			ctxt->num_clients++;
			CSIMON_DEBUG("Added " MACF " to the list at idx %d; clients %d \n",
			             ETHERP_TO_MACF(&cfg->ea), idx, ctxt->num_clients);
		}

		/* Set/Update monitor interval */
		if (cfg->monitor_interval) {
			CSIMON_DEBUG("wl%d prev_int %d upd_int %d\n", wlc->pub->unit,
			             ctxt->sta_info[idx].timeout, cfg->monitor_interval);
			ctxt->sta_info[idx].timeout = cfg->monitor_interval;
		} else {
			ctxt->sta_info[idx].timeout = CSIMON_DEFAULT_TIMEOUT_MSEC;
		}

		/* Start sending the null frames */
		if (CSIMON_ENAB(WLCPUB(ctxt))) {
			scb_t *scb;
			uint32 tsf_l;

			scb = wlc_scbapfind(wlc, &ctxt->sta_info[idx].ea);
			CSIMON_DEBUG("scb %p idx %d\n", scb, idx);
			if (scb && SCB_ASSOCIATED(scb)) {
				scb->csimon = &ctxt->sta_info[idx];
			/* CSI Monitoring start timestamp as a reference - TSF reg */
				wlc_read_tsf(wlc, &tsf_l, NULL);
				ctxt->sta_info[idx].assoc_ts = tsf_l;
#ifdef CSIMON_PER_STA_TIMER
			/* Free up any existing timer */
				if (ctxt->sta_info[idx].timer) {
					wl_free_timer(wlc->wl, ctxt->sta_info[idx].timer);
					ctxt->sta_info[idx].timer = NULL;
					CSIMON_DEBUG("Freed timer %p\n", ctxt->sta_info[idx].timer);
				}
				/* Initialize per-STA timer */
				ctxt->sta_info[idx].timer = wl_init_timer(wlc->wl,
				                           wlc_csimon_scb_timer, scb, "csimon");
				if (!(ctxt->sta_info[idx].timer)) {
					WL_ERROR(("wl%d: csimon timer init failed for "MACF"\n",
					  wlc->pub->unit, ETHER_TO_MACF(cfg->ea)));
					return BCME_NORESOURCE;
				}
				/* Start per-STA timer */
				wl_add_timer(wlc->wl, ctxt->sta_info[idx].timer,
				             ctxt->sta_info[idx].timeout, TRUE);
				CSIMON_DEBUG("wl%d: started CSI timer for SCB DA "MACF
				             " assocTS %u\n", wlc->pub->unit,
				             ETHER_TO_MACF(cfg->ea), tsf_l);
#endif /* CSIMON_PER_STA_TIMER */
			}
		}
		CSIMON_DEBUG("wl%d stations %d cfg_cmd %d\n", wlc->pub->unit,
		             ctxt->num_clients, cfg->cmd);
		break;
#ifdef BCM_CSIMON_AP
	case CSIMON_CFG_CMD_ADD_AP:
		/* CSIMON supported on D11 core rev 129 and above */
		if (!D11REV_GE(wlc->pub->corerev, 129)) {
			WL_ERROR(("wl%d: CSI Monitor not supported on this radio chip! \n",
				WLCUNIT(ctxt)));
			return BCME_EPERM;
		}
		/* The MAC address must be a valid unicast address */
		if (ETHER_ISNULLADDR(&(cfg->ea)) || ETHER_ISMULTI(&(cfg->ea))) {
			WL_ERROR(("wl%d: %s: Invalid MAC address.\n",
				WLCUNIT(ctxt), __FUNCTION__));
			return BCME_BADARG;
		}
		/* Search existing entry in the list */
		idx = wlc_csimon_sta_find(ctxt, &cfg->ea);
		if (idx < 0) {
			/* Search free entry in the list */
			idx = wlc_csimon_sta_find(ctxt, &ether_null);
			if (idx < 0) {
				WL_ERROR(("wl%d:%s: CSIMON MAC list is full (%u clients) and "
				          "can't add ["MACF"] \n", WLCUNIT(ctxt), __FUNCTION__,
				          CSIMON_MAX_STA, ETHERP_TO_MACF(&cfg->ea)));
				return BCME_NORESOURCE;
			}
			if (ctxt->num_responder_aps <  CSIMON_MAX_RESPONDER_AP) {
				/* Add MAC address to the list */
				bcopy(&cfg->ea, &ctxt->sta_info[idx].ea, ETHER_ADDR_LEN);
				ctxt->num_clients++;
				ctxt->num_responder_aps++;
				CSIMON_DEBUG("Added " MACF " to the list at idx %d;"
						"AP AP clients %d \n", ETHERP_TO_MACF(&cfg->ea),
						idx, ctxt->num_responder_aps);
			} else {
				WL_ERROR(("wl%d:%s: CSIMON AP MAC list is full (%u clients) and "
				          "can't add ["MACF"] \n", WLCUNIT(ctxt), __FUNCTION__,
				          CSIMON_MAX_RESPONDER_AP, ETHERP_TO_MACF(&cfg->ea)));
				return BCME_NORESOURCE;
			}
		}

		/* SET the SSID details required to send a prb req message */
		strncpy((char *)ctxt->sta_info[idx].SSID, (char *)cfg->SSID, cfg->ssid_len);
		ctxt->sta_info[idx].ssid_len = cfg->ssid_len;
		/* Set/Update monitor interval */
		if (cfg->monitor_interval) {
			CSIMON_DEBUG("wl%d prev_int %d upd_int %d\n", wlc->pub->unit,
			             ctxt->sta_info[idx].timeout, cfg->monitor_interval);
			ctxt->sta_info[idx].timeout = cfg->monitor_interval;
		} else {
			ctxt->sta_info[idx].timeout = CSIMON_DEFAULT_TIMEOUT_MSEC;
		}
		/* Start sending the probe req frames */
		if (CSIMON_ENAB(WLCPUB(ctxt)) && CSIMON_ENAB_AP(WLCPUB(ctxt))) {
			scb_t *scb;
			uint32 tsf_l = 0;
			scb = wlc->band->hwrs_scb;
			CSIMON_DEBUG("scb %p idx %d\n", scb, idx);
			if (scb) {
				scb->csimon = &ctxt->sta_info[idx];
				/* assoc_ts is zero as this is for unsoliciated AP to AP case */
				ctxt->sta_info[idx].assoc_ts = tsf_l;
#ifdef CSIMON_PER_STA_TIMER
				/* Free up any existing timer */
				if (ctxt->sta_info[idx].timer) {
					wl_free_timer(wlc->wl, ctxt->sta_info[idx].timer);
					ctxt->sta_info[idx].timer = NULL;
					CSIMON_DEBUG("Freed timer %p\n", ctxt->sta_info[idx].timer);
				}
				/* Initialize per-STA timer */
				ctxt->sta_info[idx].timer = wl_init_timer(wlc->wl,
				                           wlc_csimon_scb_timer, scb, "csimon");
				if (!(ctxt->sta_info[idx].timer)) {
					WL_ERROR(("wl%d: csimon timer init failed for "MACF"\n",
					  wlc->pub->unit, ETHER_TO_MACF(cfg->ea)));
					return BCME_NORESOURCE;
				}
				CSIMON_DEBUG("Init timer %p\n", ctxt->sta_info[idx].timer);
				/* Start per-STA timer */
				wl_add_timer(wlc->wl, ctxt->sta_info[idx].timer,
				             ctxt->sta_info[idx].timeout, TRUE);
				CSIMON_DEBUG("wl%d: started CSI timer for SCB DA "MACF
				             " assocTS %u\n", wlc->pub->unit,
				             ETHER_TO_MACF(cfg->ea), tsf_l);
#endif /* CSIMON_PER_STA_TIMER */

			}
		}
		CSIMON_DEBUG("wl%d stations %d cfg_cmd %d\n", wlc->pub->unit,
		             ctxt->num_clients, cfg->cmd);
		break;
#endif /* BCM_CSIMON_AP */
	case CSIMON_CFG_CMD_DEL:
		if (!CSIMON_ENAB(WLCPUB(ctxt)) && !(ctxt->num_clients)) {
			WL_ERROR(("wl%d: %s: Feature is not enabled\n",
			         WLCUNIT(ctxt), __FUNCTION__));
			return BCME_ERROR;
		}
		/* Broadcast mac address indicates to clear all stations in the list */
		if (ETHER_ISBCAST(&(cfg->ea))) {
			wlc_csimon_delete_all_stations(ctxt);
		} else {
			/* Search specified MAC address in the list */
			idx = wlc_csimon_sta_find(ctxt, &cfg->ea);
			if (idx < 0) {
				WL_ERROR(("wl%d: %s: Entry not found\n",
					WLCUNIT(ctxt), __FUNCTION__));
				return BCME_NOTFOUND;
			}
#ifdef CSIMON_PER_STA_TIMER
			/* Stop sending null frames to this station */
			if (ctxt->sta_info[idx].timer != NULL) {
				wl_free_timer(wlc->wl, ctxt->sta_info[idx].timer);
				ctxt->sta_info[idx].timer = NULL;
				CSIMON_DEBUG("wl%d: freed CSI timer for SCB DA "MACF"\n",
				             wlc->pub->unit, ETHER_TO_MACF(cfg->ea));
			}
#else
			ctxt->sta_info[idx].prev_time  = 0;
			ctxt->sta_info[idx].delta_time = 0;
#endif /* CSIMON_PER_STA_TIMER */
			wlc_csimon_sta_stats_clr(ctxt, idx);
			bcopy(&ether_null, &ctxt->sta_info[idx].ea, ETHER_ADDR_LEN);
			ctxt->num_clients--;
#ifdef BCM_CSIMON_AP
			if (ctxt->sta_info[idx].ssid_len != 0) {
				/* This is AP AP case need to handle */
				ctxt->num_responder_aps--;
			}
#endif
			CSIMON_DEBUG("wl%d stations %d cfg_cmd %d tmr %p\n", wlc->pub->unit,
			            ctxt->num_clients, cfg->cmd, ctxt->sta_info[idx].timer);
		}
		break;
	case CSIMON_CFG_CMD_RSTCNT:
		ctxt->state.null_frm_cnt = 0;
		ctxt->state.ack_fail_cnt = 0;
		ctxt->state.m2mxfer_cnt = 0;
		ctxt->state.usrxfer_cnt = 0;
		ctxt->state.rec_ovfl_cnt = 0;
		ctxt->state.xfer_fail_cnt = 0;
		ctxt->state.usr_xfer_fail_cnt = 0;
		break;
	default:
		return BCME_BADARG;
	}
	return BCME_OK;
}

static int // CSIMON IOVAR
wlc_csimon_doiovar(
	void                *hdl,
	uint32              actionid,
	void                *p,
	uint                plen,
	void                *a,
	uint                 alen,
	uint                 vsize,
	struct wlc_if       *wlcif)
{
	wlc_csimon_info_t	*csimon_ctxt = hdl;
	int			err = BCME_OK;

	BCM_REFERENCE(vsize);
	BCM_REFERENCE(wlcif);

	switch (actionid) {
	case IOV_GVAL(IOV_CSIMON):
		/* Process csimon command */
		err = wlc_csimon_process_get_cmd_options(csimon_ctxt, p, plen,
		                                         a, alen);
		break;
	case IOV_SVAL(IOV_CSIMON):
		{
			wlc_csimon_sta_config_t *csimon_cfg = (wlc_csimon_sta_config_t *)a;

			if (csimon_cfg->version != CSIMON_STACONFIG_VER) {
				return BCME_VERSION;
			}
			if ((alen >= CSIMON_STACONFIG_LENGTH) &&
				(csimon_cfg->length >= CSIMON_STACONFIG_LENGTH)) {
				/* Set CSIMON or STA parameter */
				err = wlc_csimon_sta_config(csimon_ctxt, csimon_cfg);
			} else {
				return BCME_BUFTOOSHORT;
			}
			break;
		}
	case IOV_GVAL(IOV_CSIMON_STATE):
		{
			csimon_state_t *state = (csimon_state_t *)a;
			csimon_ctxt->state.version = CSIMON_CNTR_VER;
			csimon_ctxt->state.length = sizeof(csimon_ctxt->state);
			if (alen < csimon_ctxt->state.length)
				return BCME_BUFTOOSHORT;
			*state = csimon_ctxt->state;
			state->enabled = CSIMON_ENAB(csimon_ctxt->wlc->pub);
			break;
		}
	default:
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
} // wlc_csimon_doiovar()

char * // Construct and return name of M2M IRQ
wlc_csimon_irqname(wlc_info_t *wlc, void *btparam)
{
	wlc_csimon_info_t *wlc_csimon = wlc->csimon_info;
	BCM_REFERENCE(btparam);

#if defined(CONFIG_BCM_WLAN_DPDCTL)
	if (btparam != NULL) {
		/* bustype = PCI, even embedded 2x2AX devices have virtual pci underneath */
		snprintf(wlc_csimon->irqname, sizeof(wlc_csimon->irqname),
			"wlpcie:%s, wlan_%d_m2m", pci_name(btparam), wlc->pub->unit);
	} else
#endif /* CONFIG_BCM_WLAN_DPDCTL */
	{
		snprintf(wlc_csimon->irqname, sizeof(wlc_csimon->irqname), "wlan_%d_m2m",
			wlc->pub->unit);
	}

	return wlc_csimon->irqname;
}

#if !defined(DONGLEBUILD)
bool BCMFASTPATH // ISR for M2M interrupts
wlc_csimon_isr(wlc_info_t *wlc, bool *wantdpc)
{
	void *cbdata = (void *)m2m_dd_eng_get(wlc->osh,
	   CSIxfer(wlc).m2m_dd_csi);
	return m2m_dd_isr(cbdata, wantdpc);
}

void BCMFASTPATH // Worklet/DPC for M2M interrupts
wlc_csimon_worklet(wlc_info_t *wlc)
{
	void *cbdata = (void *)m2m_dd_eng_get(wlc->osh,
	   CSIxfer(wlc).m2m_dd_csi);
	m2m_dd_worklet(cbdata);
}

uint // M2M core bitmap in oobselouta30 reg
wlc_csimon_m2m_si_flag(wlc_info_t *wlc)
{
	return m2m_dd_si_flag_get(wlc->osh);
}

void // Disable M2M interrupts
wlc_csimon_intrsoff(wlc_info_t *wlc)
{
	void *m2m_eng;

	CSIMON_ASSERT(wlc);

	/* Get the M2MDMA engine pointer */
	m2m_eng = (void *)m2m_dd_eng_get(wlc->osh,
	   CSIxfer(wlc).m2m_dd_csi);
	CSIMON_ASSERT(m2m_eng);

	/* Disable channel-based M2M interrupts for CSIMON */
	m2m_intrsoff(wlc->osh, m2m_eng);

	WL_TRACE(("wl%d: %s: disabled intrs m2m eng %p m2m_usr %d\n", wlc->pub->unit,
	          __FUNCTION__, m2m_eng, CSIxfer(wlc).m2m_dd_csi));

} /* wlc_csimon_intrsoff() */

void // Enable M2M interrupts
wlc_csimon_intrson(wlc_info_t *wlc)
{
	void *m2m_eng;

	CSIMON_ASSERT(wlc);

	/* Get the M2MDMA engine pointer */
	m2m_eng = (void *)m2m_dd_eng_get(wlc->osh,
	   CSIxfer(wlc).m2m_dd_csi);
	CSIMON_ASSERT(m2m_eng);

	/* Enable channel-based M2M interrupts for CSIMON */
	m2m_intrson(wlc->osh, m2m_eng);

	WL_TRACE(("wl%d: %s: enabled intrs m2m eng %p m2m_usr %d\n", wlc->pub->unit,
	          __FUNCTION__, m2m_eng, CSIxfer(wlc).m2m_dd_csi));

} /* wlc_csimon_intrson() */

#endif /* ! DONGLEBUILD */

#ifdef BCM_CSIMON_AP
/* csimon wrapper function to check ssid_len */
bool
wlc_csimon_ap_ssid_len_check(struct scb *scb)
{
	if (scb == NULL)
		return FALSE;

	if (scb->csimon != NULL && scb->csimon->ssid_len)
		return TRUE;

	return FALSE;
}
#endif
