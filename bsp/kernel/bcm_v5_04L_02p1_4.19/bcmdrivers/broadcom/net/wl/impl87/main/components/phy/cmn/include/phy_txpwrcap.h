/*
 * TxPowerCap module internal interface (to other PHY modules).
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
 * $Id: phy_txpwrcap.h 785011 2020-03-10 22:27:57Z $
 */

#ifndef _phy_txpwrcap_h_
#define _phy_txpwrcap_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_txpwrcap_info phy_txpwrcap_info_t;

#ifdef WLC_TXPWRCAP

#define TXPWRCAP_CELLSTATUS_ON 1
#define TXPWRCAP_CELLSTATUS_OFF 0
#define TXPWRCAP_CELLSTATUS_NBIT 0
#define TXPWRCAP_CELLSTATUS_MASK (1<<TXPWRCAP_CELLSTATUS_NBIT)
#define TXPWRCAP_CELLSTATUS_FORCE_MASK 0x2
#define TXPWRCAP_CELLSTATUS_FORCE_UPD_MASK 0x4
#define TXPWRCAP_CELLSTATUS_WCI2_NBIT 4
#define TXPWRCAP_CELLSTATUS_WCI2_MASK (1<<TXPWRCAP_CELLSTATUS_WCI2_NBIT)

#define TXPOWERCAP_MAX_QDB	(127)
#define TXPOWERCAP_MAX_ANT_PER_CORE	(2)

/* attach/detach */
phy_txpwrcap_info_t *phy_txpwrcap_attach(phy_info_t *pi);
void phy_txpwrcap_detach(phy_txpwrcap_info_t *ri);

/* ******** interface for Txpowercap module ******** */
int8 phy_txpwrcap_tbl_get_max_percore(phy_info_t *pi, uint8 core);

#ifdef WLC_SW_DIVERSITY
void wlc_phy_txpwrcap_to_shm(wlc_phy_t *pih, uint16 tx_ant, uint16 rx_ant);
#endif /* WLC_SW_DIVERSITY */
#endif /* WLC_TXPWRCAP */

#endif /* _phy_txpwrcap_h_ */
