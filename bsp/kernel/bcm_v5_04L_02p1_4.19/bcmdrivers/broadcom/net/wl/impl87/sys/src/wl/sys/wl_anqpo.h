/*
 * ANQP Offload
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
 * $Id: wl_anqpo.h 661822 2016-09-27 12:31:10Z $
 */

/**
 * XXX Hot Spot related.
 * ANQP Offload is a feature requested by Olympic which allows the dongle to perform ANQP
 * queries (using 802.11u GAS) to devices and have the ANQP response returned to the host using an
 * event notification. Any query using ANQP such as hotspot and service discovery may be sent using
 * the offload.
 *
 * Twiki: [OffloadsPhase2]
 */

#ifndef _wl_anqpo_h_
#define _wl_anqpo_h_

#include <wlc_cfg.h>
#include <d11.h>
#include <wlc_types.h>
#include <wl_gas.h>

typedef struct wl_anqpo_info wl_anqpo_info_t;

/*
 * Initialize anqpo private context.
 * Returns a pointer to the anqpo private context, NULL on failure.
 */
extern wl_anqpo_info_t *wl_anqpo_attach(wlc_info_t *wlc, wl_gas_info_t *gas);

/* Cleanup anqpo private context */
extern void wl_anqpo_detach(wl_anqpo_info_t *anqpo);

/* initialize on scan start */
extern void wl_anqpo_scan_start(wl_anqpo_info_t *anqpo);

/* deinitialize on scan stop */
extern void wl_anqpo_scan_stop(wl_anqpo_info_t *anqpo);

/* process scan result */
extern void wl_anqpo_process_scan_result(wl_anqpo_info_t *anqpo,
	wlc_bss_info_t *bi, uint8 *ie, uint32 ie_len, int8 cfg_idx);
#endif /* _wl_anqpo_h_ */
