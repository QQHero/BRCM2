/**
 * -----------------------------------------------------------------------------
 * Generic Broadcom Home Networking Division (HND) Mailbox Pager Revision 1
 * Mailbox in Host Memory Extension with Paging of mail.
 *
 * Revision 2: HW assisted Page-In.
 * - Autonomous policy, multi-slot consumer page-in.
 * - Non-MLO and MLO mailboxes.
 * - MAP consumes from AAP's MLO Mailboxes as well as self.
 *
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
 * $Id$
 *
 * vim: set ts=4 noet sw=4 tw=80:
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * -----------------------------------------------------------------------------
 */

#ifndef __hndmbx_h_included__
#define __hndmbx_h_included__

#include <typedefs.h>

/** MBX well known User IDs. */
#define MBX_USR_MACTXS          (0) // MAC to Driver TxStatus Mailbox
#define MBX_USR_PHYRXS          (1) // PHY to Driver PhyRxStatus Mailbox
#define MBX_USR_MBXREQ          (2) // Future use (Peer MLO AP request mbox)
#define MBX_USR_MBXRSP          (3) // Future use (Peer MLO AP response mbox)
#define MBX_USR_TOTAL           (4)

/** Roundup the size of each mailbox to 4 KBytes */
#define MBX_HME_BYTES(bytes)    ROUNDUP((bytes), 4096)

#ifndef MBX_USR_MACTXS_BYTES
// No of packages/TxStatus unit x TxStatus Ring depth
#define MBX_USR_MACTXS_BYTES    \
	MBX_HME_BYTES(TX_STATUS_MACTXS_BYTES * TX_STATUS_MACTXS_RING_DEPTH)
#endif
#ifndef MBX_USR_PHYRXS_BYTES
// d11phyrxsts_t x PhyRx Status Ring depth
#define MBX_USR_PHYRXS_BYTES    \
	MBX_HME_BYTES(PHYRX_STATUS_BYTES * PHYRX_STATUS_RING_DEPTH)
#endif
#ifndef MBX_USR_MBXREQ_BYTES
#define MBX_USR_MBXREQ_BYTES    (0) // Future use
#endif
#ifndef MBX_USR_MBXRSP_BYTES
#define MBX_USR_MBXRSP_BYTES    (0) // Future use
#endif

/** Host Memory Extension service: mailbox memory sizing */
#define MBX_HME_BYTES_MAX       (MBX_USR_MACTXS_BYTES + MBX_USR_PHYRXS_BYTES \
	                            + MBX_USR_MBXREQ_BYTES + MBX_USR_MBXRSP_BYTES)

/** Maximum mailbox page-in slots for consumer side. (Tested for 2) */
#define MBX_SLOTS_MAX           (2) // Rev1: max 2 ping-pong slots

/** Mailbox ping-pong slot offset: current slot and alternate slot */
#define MBX_SLOT_CUR            (0) // current slot tracked by iterator
#define MBX_SLOT_ALT            (1) // alternate slot to current

/** Index of a message in HME mailbox. ~0 is treated as IDLE or INVALID */
#define MBX_MSG_IDLE            ((uint16)(~0))

/** Engine index in BME_USR_H2D engine set. ~0 is treated as IDLE or INVALID */
#define MBX_BME_IDLE            ((int)(~0))

/** Initiate page-in of an element from producer ring into consumer ping-pong */
int     mbx_xfer(uint32 usr_idx, uint16 msg_idx, uint16 slot_offset);

/** Determine whether a xfer is in progress, returing msg_idx or MBX_MSG_IDLE */
uint16  mbx_page(uint32 usr_idx, uint16 slot_offset);

/** Sync page-in DMA completion returning message in current slot */
void  * mbx_sync(uint32 usr_idx, uint16 slot_offset);

/** Iterate to next slot, sync on next xfer if ongoing/ */
void  * mbx_iter(uint32 usr_idx);   // iteration over a slot, vacates the slot

/** Release all engines and vacate slots */
void    mbx_done(uint32 usr_idx);

/** Query number of pagein DMA transfers busy (i.e. not synced) */
uint32  mbx_busy(uint32 usr_idx);

/** Query number of messages occupying slots */
uint32  mbx_used(uint32 usr_idx);

/** Query number of messages occupying slots and ready to consume */
uint32  mbx_msgs(uint32 usr_idx);

/** Initialize Mailbox Service */
int    mbx_init(si_t * sih, osl_t * osh);

/** Register a user with Mailbox service after HME has linked with PCIe IPC */
int    mbx_register_user(osl_t * osh, uint32 usr_idx, uint16 msg_len,
		uint16 msg_num, haddr64_t * haddr64);

/** Dump Mailbox Service and all users */
void   mbx_dump(bool verbose);

/**  Mailbox Service console command utility */
void   mbx_cmd(void *arg, int argc, char *argv[]);

#endif /* __hndmbx_h_included__ */
