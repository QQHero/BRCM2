/*
 * +--------------------------------------------------------------------------+
 *
 * Generic Broadcom Home Networking Division (HND) SBTOPCIE module.
 *
 * Dongle access to PCIE Memory using SBTOPCIE Translation Window Service
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
 * +--------------------------------------------------------------------------+
 */

#ifndef __sbtopcie_h_included__
#define __sbtopcie_h_included__

#ifdef DONGLEBUILD

/**
 * +--------------------------------------------------------------------------+
 *  SBTOPCIE Service manages the multiple translation windows as shared or
 *  private. An application registered with a shared allocation mode, needs to
 *  explicitly switch the translation window before any access is made in the
 *  run to completion execution thread. An application may register a private
 *  window when switching before use paradigm cannot be established or the
 *  overhead for switching is non acceptable. A Host Memory Offload with Access
 *  in Place (AIP) needs to be private.
 *
 * sbtopcie.h is not PCIe IPC revision control
 * +--------------------------------------------------------------------------+
 */
/** Window Mode: allocation policy used in sbtopcie_setup() */
#define SBTOPCIE_MODE_SHARED    (0) // Window 0 is shared
#define SBTOPCIE_MODE_PRIVATE   (1) // Window is private to a user
#define SBTOPCIE_MODE_MAX       (2)

/**
 * +--------------------------------------------------------------------------+
 *  Users with Private Window
 *  - Runner software doorbell. Need not be private(*)
 *  - HMO Access In Place: Must always be private.
 *  - Packet Queue Pager: Need not be private(*). Switch before use.
 *  - HMO Text Segment SW_PAGING: Private at boot. Released hme_link_pcie_ipc().
 * +--------------------------------------------------------------------------+
 */
#define SBTOPCIE_USR_SYS        (0) // X: System [read, write, copy, print]
#define SBTOPCIE_USR_RNR        (1) // P* Runner wakeup software doorbell
#define SBTOPCIE_USR_AIP        (2) // P: HMO Data Access In Place
#define SBTOPCIE_USR_PQP        (3) // P* Packet Queue Pager
#define SBTOPCIE_USR_SWP        (4) // P: HMO SW_PAGING Text Segment
#define SBTOPCIE_USR_CSI        (5) // S: CSIMON feature
#define SBTOPCIE_USR_UN6        (6) // X: Future
#define SBTOPCIE_USR_UN7        (7) // X: Future
#define SBTOPCIE_USR_MAX        (8)

/** A maximum of 4 translation windows are supported in current WLAN SoCs */
#define SBTOPCIE_WIN_SYS        (0) // System owned window for shared mapping
#define SBTOPCIE_WIN_MAX        (4)

/** Direction of a PCIE_MEM acces/copy */
#define SBTOPCIE_DIR_H2D        (0) // Host to Dongle Accesses: PIO, Copy
#define SBTOPCIE_DIR_D2H        (1) // Dongle to Host Accesses: PIO, Copy

/** Return value for error conditions */
#define SBTOPCIE_ERROR          (~0U)
#define SBTOPCIE_ADDR_INV       ((uintptr)SBTOPCIE_ERROR)

#define SBTOPCIE_WIN_04MB       (22) // 04 MByte Window : 1 << 22
#define SBTOPCIE_WIN_08MB       (23) // 08 MByte Window : 1 << 23
#define SBTOPCIE_WIN_16MB       (24) // 16 MByte Window : 1 << 24
#define SBTOPCIE_WIN_32MB       (25) // 32 MByte Window : 1 << 25

/** Debug dump sbtopcie service */
void    sbtopcie_dump(void);

/**
 * Initialize sbtopcie service.
 * Service initialized and ready to use when the PCIE CORE driver is attached
 * to the backplane.
 */
int     BCMATTACHFN(sbtopcie_init)(osl_t * osh,
            uint32 corerev, uint32 coreid, uint32 chip, volatile void * regs);

/**
 *  Setup a user's sbtopcie window.
 *  - usr       : User Id as specified in SBTOPCIE_USR_###
 *  - haddr_u64 : 64bit physical address of a host memory region to be mapped
 *  - size      : size of the host memory intended to be accessed by user
 *  - mode      : Windiow allocation mode - shared or private
 *
 *  Returns:
 *  - uintptr which is a backplane address corresponding to the 64 bit host
 *    physical address. Applications may directly access the host memory using
 *    the returned 32 bit uintptr.
 *  - SBTOPCIE_ADDR_INV on error, e.g. range cannot fit in a window
 *
 *  Caution:
 *   In shared mode, upon setup, an explicit switch is required.
 */
uintptr sbtopcie_setup(uint32 usr, uint64 haddr_u64, size_t size, uint32 mode);

/**
 * Release sbtopcie window owned by a user. Users with private windows allocated
 * must release the window resource, if no further accesses are needed.
 */
void    sbtopcie_release(uint32 usr);

/**
 *  Translate a PCIE memory address into a SB managed window.
 *  Returns
 *  - uintptr which is a backplane address corresponding to the 64 bit
 *    host physical address. Applications may directly access the host memory
 *    using * the returned 32 bit uintptr.
 *  - SBTOPCIE_ADDR_INV on error, e.g. user has not been registered.
 *
 *  Caution:
 *   In shared mode, upon setup, an explicit switch is required.
 */
uintptr sbtopcie_translate(uint32 usr, uint64 haddr_u64);

/**
 * Test whether a host address range is in user's translation window.
 * User must be registered.
 */
bool    sbtopcie_valid(uint32 usr, uint64 haddr_u64, size_t bytes);

/**
 * Switch the system window to a specified user.
 * A user with a shared window allocation, must switch the window before use.
 */
void    sbtopcie_switch(uint32 usr);

/** Restore the system window to a specified user */
void    sbtopcie_restore(uint32 usr); // for debug only

/**
 * +--------------------------------------------------------------------------+
 *  Generic System Services that may be invoked by unregistered users.
 *
 *  Service will:
 *  - switch the system window
 *  - translate the 64 bit host memory into a backplane address
 *  - perform the requested operation: PIO, Copy 32b words, Copy bytes, Print
 * +--------------------------------------------------------------------------+
 */

/** Programmed IO access 1 Byte, 2 Byte or 4 Byte using sbtopcie */
void    sbtopcie_pio(uintptr daddr_uptr, uint64 haddr_u64, size_t pio_size,
                     uint32 dir);

/** Copy 32bit words using sbtopcie - assumes 32b alignment */
void    sbtopcie_cpy32(uintptr daddr_uptr, uint64 haddr_u64, uint32 words,
                       uint32 dir); // in units of 4 Byte words
/** Copy bytes using sbtopcie */
void    sbtopcie_cpy(uintptr daddr_uptr, uint64 haddr_u64, size_t bytes,
                     uint32 dir);

/** Print Host Memory using sbtopcie */
void    sbtopcie_print(uint64 haddr_u64, size_t bytes);

#endif /* DONGLEBUILD */

#endif /* __sbtopcie_h_included__ */
/*
 * +--------------------------------------------------------------------------+
 *
 * Generic Broadcom Home Networking Division (HND) SBTOPCIE module.
 *
 * Dongle access to PCIE Memory using SBTOPCIE Translation Window Service
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
 * +--------------------------------------------------------------------------+
 */

#ifndef __sbtopcie_h_included__
#define __sbtopcie_h_included__

#ifdef DONGLEBUILD

/**
 * +--------------------------------------------------------------------------+
 *  SBTOPCIE Service manages the multiple translation windows as shared or
 *  private. An application registered with a shared allocation mode, needs to
 *  explicitly switch the translation window before any access is made in the
 *  run to completion execution thread. An application may register a private
 *  window when switching before use paradigm cannot be established or the
 *  overhead for switching is non acceptable. A Host Memory Offload with Access
 *  in Place (AIP) needs to be private.
 * +--------------------------------------------------------------------------+
 */
/** Window Mode: allocation policy used in sbtopcie_setup() */
#define SBTOPCIE_MODE_SHARED    (0) // Window 0 is shared
#define SBTOPCIE_MODE_PRIVATE   (1) // Window is private to a user
#define SBTOPCIE_MODE_MAX       (2)

/**
 * +--------------------------------------------------------------------------+
 *  Users with Private Window
 *  - Runner software doorbell. Need not be private(*)
 *  - HMO Access In Place: Must always be private.
 *  - Packet Queue Pager: Need not be private(*). Switch before use.
 *  - HMO Text Segment SW_PAGING: Private at boot. Released hme_link_pcie_ipc().
 * +--------------------------------------------------------------------------+
 */
#define SBTOPCIE_USR_SYS        (0) // X: System [read, write, copy, print]
#define SBTOPCIE_USR_RNR        (1) // P* Runner wakeup software doorbell
#define SBTOPCIE_USR_AIP        (2) // P: HMO Data Access In Place
#define SBTOPCIE_USR_PQP        (3) // P* Packet Queue Pager
#define SBTOPCIE_USR_SWP        (4) // P: HMO SW_PAGING Text Segment
#define SBTOPCIE_USR_CSI        (5) // S: CSIMON feature
#define SBTOPCIE_USR_UN6        (6) // X: Future
#define SBTOPCIE_USR_UN7        (7) // X: Future
#define SBTOPCIE_USR_MAX        (8)

/** A maximum of 4 translation windows are supported in current WLAN SoCs */
#define SBTOPCIE_WIN_SYS        (0) // System owned window for shared mapping
#define SBTOPCIE_WIN_MAX        (4)

/** Direction of a PCIE_MEM acces/copy */
#define SBTOPCIE_DIR_H2D        (0) // Host to Dongle Accesses: PIO, Copy
#define SBTOPCIE_DIR_D2H        (1) // Dongle to Host Accesses: PIO, Copy

/** Return value for error conditions */
#define SBTOPCIE_ERROR          (~0U)
#define SBTOPCIE_ADDR_INV       ((uintptr)SBTOPCIE_ERROR)

/** Debug dump sbtopcie service */
void    sbtopcie_dump(void);

/**
 * Initialize sbtopcie service.
 * Service initialized and ready to use when the PCIE CORE driver is attached
 * to the backplane.
 */
int     BCMATTACHFN(sbtopcie_init)(osl_t * osh,
            uint32 corerev, uint32 coreid, uint32 chip, volatile void * regs);

/**
 *  Setup a user's sbtopcie window.
 *  - usr       : User Id as specified in SBTOPCIE_USR_###
 *  - haddr_u64 : 64bit physical address of a host memory region to be mapped
 *  - size      : size of the host memory intended to be accessed by user
 *  - mode      : Windiow allocation mode - shared or private
 *
 *  Returns:
 *  - uintptr which is a backplane address corresponding to the 64 bit host
 *    physical address. Applications may directly access the host memory using
 *    the returned 32 bit uintptr.
 *  - SBTOPCIE_ADDR_INV on error, e.g. range cannot fit in a window
 *
 *  Caution:
 *   In shared mode, upon setup, an explicit switch is required.
 */
uintptr sbtopcie_setup(uint32 usr, uint64 haddr_u64, size_t size, uint32 mode);

/**
 * Release sbtopcie window owned by a user. Users with private windows allocated
 * must release the window resource, if no further accesses are needed.
 */
void    sbtopcie_release(uint32 usr);

/**
 *  Translate a PCIE memory address into a SB managed window.
 *  Returns
 *  - uintptr which is a backplane address corresponding to the 64 bit
 *    host physical address. Applications may directly access the host memory
 *    using * the returned 32 bit uintptr.
 *  - SBTOPCIE_ADDR_INV on error, e.g. user has not been registered.
 *
 *  Caution:
 *   In shared mode, upon setup, an explicit switch is required.
 */
uintptr sbtopcie_translate(uint32 usr, uint64 haddr_u64);

/**
 * Test whether a host address range is in user's translation window.
 * User must be registered.
 */
bool    sbtopcie_valid(uint32 usr, uint64 haddr_u64, size_t bytes);

/**
 * Switch the system window to a specified user.
 * A user with a shared window allocation, must switch the window before use.
 */
void    sbtopcie_switch(uint32 usr);

/** Restore the system window to a specified user */
void    sbtopcie_restore(uint32 usr); // for debug only

/**
 * +--------------------------------------------------------------------------+
 *  Generic System Services that may be invoked by unregistered users.
 *
 *  Service will:
 *  - switch the system window
 *  - translate the 64 bit host memory into a backplane address
 *  - perform the requested operation: PIO, Copy 32b words, Copy bytes, Print
 * +--------------------------------------------------------------------------+
 */

/** Programmed IO access 1 Byte, 2 Byte or 4 Byte using sbtopcie */
void    sbtopcie_pio(uintptr daddr_uptr, uint64 haddr_u64, size_t pio_size,
                     uint32 dir);

/** Copy 32bit words using sbtopcie - assumes 32b alignment */
void    sbtopcie_cpy32(uintptr daddr_uptr, uint64 haddr_u64, uint32 words,
                       uint32 dir); // in units of 4 Byte words
/** Copy bytes using sbtopcie */
void    sbtopcie_cpy(uintptr daddr_uptr, uint64 haddr_u64, size_t bytes,
                     uint32 dir);

/** Print Host Memory using sbtopcie */
void    sbtopcie_print(uint64 haddr_u64, size_t bytes);

#endif /* DONGLEBUILD */

#endif /* __sbtopcie_h_included__ */
