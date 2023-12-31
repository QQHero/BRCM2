/*
 * Broadcom 802.11 Networking Device Driver Configuration file for 6715b0
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
 * $Id: $
 *
 * wl driver tunables for 6715b0 RTE dev
 *
 */

#define    D11CONF        0
#define    D11CONF2    0
#define    D11CONF3    0
#define    D11CONF4    0
#define    D11CONF5    0x00000010    /* D11 Core Rev 132 */
#define ACCONF        0
#define ACCONF5        0x00000004    /* AC-phy rev 130 */
#define D11CONF_MINOR   0x4     /* MINOR rev 2 */

#define HECAP_DONGLE    0x1

#ifdef WL_NTXD
#define NTXD        WL_NTXD
#else
#define NTXD        128
#endif
#ifdef WL_NTXD_BCMC
#define NTXD_BCMC    WL_NTXD_BCMC
#else
#define NTXD_BCMC    128
#endif
#ifdef WL_NRXD
#define NRXD        WL_NRXD
#else
#define NRXD        128
#endif
#ifdef WL_NRXBUFPOST
#define NRXBUFPOST    WL_NRXBUFPOST
#else
#define NRXBUFPOST    64
#endif

#define WLC_DATAHIWAT    50        /* NIC: 50 */
#define WLC_AMPDUDATAHIWAT    128    /* NIC: 128 */
#define RXBND        48
#ifdef WLC_NRMAX_BSS
#define WLC_MAX_UCODE_BSS    WLC_NRMAX_BSS    /* Max BSS supported */
#define WLC_MAXBSSCFG    WLC_NRMAX_BSS
#else
#define WLC_MAX_UCODE_BSS    8    /* Max BSS supported */
#define WLC_MAXBSSCFG    9
#endif
#define WLC_MAXDPT    1
#define WLC_MAXTDLS    5

#define MAXSCB        96 /* max # of SCB that can be supported based on memory available */

#define DEFMAXSCB    75 /* default value of max # of STAs allowed to join */
#define AIDMAPSZ    32

#ifndef AMPDU_RX_BA_DEF_WSIZE
#define AMPDU_RX_BA_DEF_WSIZE    64 /* Default value to be overridden for dongle */
#endif

#define PKTCBND            RXBND
#define PKTCBND_AC3X3        RXBND
#define NTXD_LARGE_AC3X3    NTXD
#define NRXD_LARGE_AC3X3    NRXD
#define RXBND_LARGE_AC3X3    RXBND
#define NRXBUFPOST_LARGE_AC3X3    NRXBUFPOST
#define NRXBUFPOST_CLASSIFIED_FIFO    16

#ifdef WL_NTXD_LFRAG
#define NTXD_LFRAG        WL_NTXD_LFRAG
#else
#define NTXD_LFRAG        512
#endif

/* IE MGMT tunables */
#define MAXIEREGS        8
#define MAXVSIEBUILDCBS        112
#define MAXIEPARSECBS        166
#define MAXVSIEPARSECBS        70

/* Module and cubby tunables */
#define MAXBSSCFGCUBBIES    64    /* max number of cubbies in bsscfg container */
#define WLC_MAXMODULES        95    /* max #  wlc_module_register() calls */

/* Customize/increase pre-allocated notification memory pool size */
/* Maximum number of notification servers */
#define MAX_NOTIF_SERVERS 64
/* Maximum number of notification clients */
#define MAX_NOTIF_CLIENTS 80
