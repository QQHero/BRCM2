/*
 * +--------------------------------------------------------------------------+
 *
 * Broadcom Full Dongle SiliconBackplane address Translation to a PCIE Memory
 * address space. pcie_core.h defines a set of translation windows in SB space.
 * Each window is mapped into a window of equal size over PCIE (DDR).
 * When an SB address falls into a  translation window, the upper N bits are
 * replaced by the N bits configured in the sbtopcie translation register and
 * read/write transaction is transferred to the PCIe RC.
 *
 * PCIe RC may perform additional match and translate.
 *
 * Exported Interface Namespace: sbtopcie as in sbtopcie.h, pcie_core.h
 * Implementation Namespace: s2p (system bus or silicon backplane) to pcie mem
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

#include <osl.h>
#include <typedefs.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <bcmlibarm.h>
#include <pcie_core.h>
#include <sbtopcie.h>
#include <rte_cons.h>               // hnd_cons_add_cmd()

#ifndef DONGLEBUILD
#error  "S2P only applies to dongle mode"
#endif

#define S2P_VERSION                 "v1.0"

#define S2P_NOOP                    do { /* no-op */ } while(0)

#define S2P_CALL_FMT                " [@%08x]"
#define S2P_CALL_SITE               (uint32)(__builtin_return_address(0))

// In Generic System Services, auto restore a shared window back to System reset
// #define S2P_RESET_BUILD

// #define S2P_DEBUG_BUILD
#if defined(S2P_DEBUG_BUILD)
#define S2P_ASSERT(expr)            ASSERT(expr)
#define S2P_DEBUG(expr)             expr
#define S2P_REGOP(expr)             expr
// #define S2P_REGOP(expr)          S2P_NOOP
#define S2P_RANGE_AUDIT(_sysp_, _u64_, _sz_) \
({ \
	if (__s2p_range((_sysp_), U64_TO_LO32(_u64_), _sz_) == SBTOPCIE_ADDR_INV) \
	{ \
		printf("S2P: %s window overflow lo 0x%08x hi 0x%08x bytes %u" \
			S2P_CALL_FMT "\n", \
			 __FUNCTION__, U64_TO_LO32(_u64_), U64_TO_HI32(_u64_), \
			(uint32)(_sz_), S2P_CALL_SITE); \
		ASSERT(0); \
		return; \
	} \
})
#else  /* ! S2P_DEBUG_BUILD */
#define S2P_ASSERT(expr)            S2P_NOOP
#define S2P_DEBUG(expr)             S2P_NOOP
#define S2P_REGOP(expr)             S2P_NOOP
#define S2P_RANGE_AUDIT(s, u, n)    S2P_NOOP
#endif /* ! S2P_DEBUG_BUILD */

/**
 * +--------------------------------------------------------------------------+
 * XXX
 *
 *  SBTOPCIE_BACKPLANE_REGION_SIZE = 128 MB
 *
 *  PCIECOREREV CCI BACKPLANE   WINDOWS::SIZE MB   CONFIG_MASK  CHIPS
 *     >= 24     Y  0x20000000      4  ::  32 MB    0xFE000000  43684, 6715
 *     <  24     Y  0x20000000      2  ::  64 MB    0xFC000000  4366
 *     <  24     N  0x08000000      2  ::  64 MB    0xFC000000  43602
 *
 *  Translation WINDOW SIZE   = 128 MB / (Number of Windows)
 *  Translation CONFIG MASK   = 0xFFFFFFFF ^ (WINDOW_SIZE - 1)
 *
 *  SBTOPCIE window reg::lo32 = (haddr64.lo & TRANS_MASK)
 *  Remap OFFSET MASK         = ((WINDOW_SIZE - 1)
 *  Dongle translated address = window base + (haddr64.lo & (WINDOW_SIZE - 1))
 * +--------------------------------------------------------------------------+
 */
#define S2P_WINDOW_SIZE(windows)    (SBTOPCIE_BACKPLANE_REGION_SIZE / (windows))
#define S2P_OFFSET_MASK(windows)    (S2P_WINDOW_SIZE(windows) - 1)
#define S2P_CONFIG_MASK(windows)    ((~0U) ^ (S2P_OFFSET_MASK(windows)))

/**
 * AccessType RW=2b00, PrefetchEn=1b1, WriteBurstEn=1b1
 * SBTOPCIE Service supports AccessType Memory. No support for IO, CFG0, CFG1
 */
#define S2P_ACCESS_ATTRIBUTES \
		BCM_SBF(SBTOPCIE_ACCESS_TYPE_MEM, SBTOPCIE_ACCESS_TYPE) | \
		BCM_SBIT(SBTOPCIE_PREF_ENABLE) | \
		BCM_SBIT(SBTOPCIE_WRITE_BURST)

// +--------------------------------------------------------------------------+

typedef union s2p_ptr {
	uint8             * p8;
	uint16            * p16;
	uint32            * p32;
	uint64            * p64;
	uintptr             uptr;
	void              * vptr;
} s2p_ptr_t;

typedef struct s2p_reg {        // Registers not listed in order
	volatile uint32   * lo32;   //   Pointer to window lower register
	volatile uint32   * hi32;   //   Pointer to window upper register
} s2p_reg_t;

typedef union s2p_val {         // Values in registers
	uint64              u64;
	struct {
		uint32          lo32;   //   Value in window lower register
		uint32          hi32;   //   Value in window upper register
	};
} s2p_val_t;

typedef struct s2p_win s2p_win_t;

typedef struct s2p_usr {        // User configuration
	uint8               uidx;   //   User id
	uint8               mode;   //   Window mode - allocation policy
	uint8               widx;   //   Window index
	uint8               enab;   //   Enabled
	uint32              stat;   //   Read/Write access statistics
	s2p_val_t           vals;   //   Values to use in registers
} s2p_usr_t;

typedef struct s2p_win {        // Translation window
	uint32              base;   //   Window base address in SB space
	s2p_usr_t         * usrp;   //   Window assigned to user: private
	s2p_reg_t           regs;   //   Pointers to window registers
	s2p_val_t           vals;   //   Values currently in window register
} s2p_win_t;

typedef struct s2p_sys {        // System service
	s2p_win_t           win[SBTOPCIE_WIN_MAX]; // Current window configuration
	s2p_usr_t           usr[SBTOPCIE_USR_MAX]; // User configurations
	uint32              cfg;    //   Mask haddr to configure translation reg
	uint32              map;    //   Offset mask for remapping to window base
	uint32              len;    //   Window size in bytes: 32 MB or 64 MB
	uint8               tot;    //   Total translation windows
	osl_t             * osh;
	sbpcieregs_t      * regs;
} s2p_sys_t;

// +--------------------------------------------------------------------------+
//  S2P Accessor Macros
//  0th user and window is owned by the S2P System Service
// +--------------------------------------------------------------------------+
#define S2P_SYS()               (&s2p_sys_g)
#define S2P_USR(_sysp_, _uidx_) (&((_sysp_)->usr[_uidx_]))
#define S2P_WIN(_sysp_, _widx_) (&((_sysp_)->win[_widx_]))
#define S2P_USR_SYS(_sysp_)     S2P_USR((_sysp_), SBTOPCIE_USR_SYS)
#define S2P_WIN_SYS(_sysp_)     S2P_WIN((_sysp_), SBTOPCIE_WIN_SYS)

#define S2P_MAP_HADDR_64(_sysp_, _winp_, _haddr_u64_) \
	(uintptr)((_winp_)->base + \
	          (U64_TO_LO32(_haddr_u64_) & S2P_OFFSET_MASK((_sysp_)->tot)))

// +--------------------------------------------------------------------------+
//  S2P Globals
// +--------------------------------------------------------------------------+

s2p_sys_t s2p_sys_g                           = { .tot = 0 }; // Global

const char * s2p_usr_name[SBTOPCIE_USR_MAX]   =
	{ "SYS", "RNR", "AIP", "PQP", "SWP", "CSI", "UN6", "UN7" };

const char * s2p_mode_name[SBTOPCIE_MODE_MAX] = { "Shared ", "Private" };

// +--------------------------------------------------------------------------+
//  S2P Internal Helper Functions
// +--------------------------------------------------------------------------+
static INLINE void // Program registers only if value to be written is different
__s2p_wr_regs(s2p_sys_t * sysp, s2p_win_t * winp, uint64 haddr_u64)
{
	uint32 v32;
	v32 = (U64_TO_LO32(haddr_u64) & S2P_CONFIG_MASK(sysp->tot));
	if (winp->vals.lo32 != v32) {
		winp->vals.lo32  = v32;
		v32 = v32 | S2P_ACCESS_ATTRIBUTES;
		W_REG(sysp->osh, winp->regs.lo32, v32);
		S2P_REGOP(printf("S2P: W_REG LO<%p,%08x>\n", winp->regs.lo32, v32));
	}
	v32 = U64_TO_HI32(haddr_u64);
	if (winp->vals.hi32 != v32) {
		winp->vals.hi32  = v32;
		W_REG(sysp->osh, winp->regs.hi32, v32);
		S2P_REGOP(printf("S2P: W_REG HI<%p,%08x>\n", winp->regs.hi32, v32));
	}

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7A__)
	__asm__ __volatile__("dsb");
	__asm__ __volatile__("isb");
#endif /* __ARM_ARCH_7M__ || __ARM_ARCH_7R__ || __ARM_ARCH_7A__ */

}   // __s2p_wr_regs()

static INLINE void // Configure a translation window and attach to user
__s2p_win_config(s2p_sys_t * sysp, s2p_win_t * winp, uint64 haddr_u64,
                 s2p_usr_t * usrp)
{
	S2P_ASSERT(winp->base != 0U); // S2P service has been attached
	__s2p_wr_regs(sysp, winp, haddr_u64);
	winp->usrp      = usrp;
}   // __s2p_win_config()

static INLINE void // Configure a user and attach to window
__s2p_usr_config(s2p_sys_t * sysp, s2p_usr_t * usrp, uint64 haddr_u64,
                 uint8 mode, int widx)
{
	usrp->mode      = mode;
	usrp->widx      = widx;
	usrp->stat      = 0U;
	usrp->vals.u64  = haddr_u64;
}   // __s2p_usr_config()

static INLINE int // Excluding WIN_SYS, find a window not attached to a user
__s2p_win_allocate(s2p_sys_t * sysp)
{
	int         widx;
	s2p_win_t * winp;
	for (widx = SBTOPCIE_WIN_SYS + 1; widx < sysp->tot; widx++) {
		winp = S2P_WIN(sysp, widx);
		if (winp->usrp == S2P_USR_SYS(sysp)) {
			return widx;
		}
	}
	return BCME_NORESOURCE;
}   // __s2p_win_allocate()

static INLINE void
__s2p_win_sys_reset(s2p_sys_t * sysp)
{
#if defined(S2P_RESET_BUILD)
	uint64      z64  = 0ULL;
	s2p_usr_t * usrp = S2P_USR_SYS(sysp);
	s2p_win_t * winp = S2P_WIN_SYS(sysp);

	__s2p_win_config(sysp, winp, z64, usrp);
#endif /* S2P_RESET_BUILD */
}   // __s2p_win_sys_reset()

static INLINE uintptr // Returns the window configuration for a range or ~0U
__s2p_range(s2p_sys_t * sysp, uint32 haddr_lo32, size_t bytes)
{
	uint32  mask    = S2P_CONFIG_MASK(sysp->tot);
	uint32  win_bgn = (haddr_lo32 & mask);
	uint32  win_end = ((haddr_lo32 + (uint32)bytes) & mask);
	return (win_bgn == win_end) ? (uintptr)win_bgn : SBTOPCIE_ADDR_INV;
}   // __s2p_range()

// +--------------------------------------------------------------------------+
//  Exported Functional Interface
// +--------------------------------------------------------------------------+

void // Debug dump sbtopcie service
sbtopcie_dump(void)
{
	int widx, uidx;
	s2p_sys_t * sysp = S2P_SYS();

	printf("S2P: %s cfg<0x%08x> map<0x%08x> len<%u MB> tot<%u>\n",
		S2P_VERSION, sysp->cfg, sysp->map,
		(sysp->len >> 20) /* in MB */, sysp->tot);

	printf("\twin. baseaddr s2p_regl s2p_regh s2p_regv addrlo32 addrhi32\n");
	for (widx = 0; widx < SBTOPCIE_WIN_MAX; widx++) {
		s2p_win_t * winp = S2P_WIN(sysp, widx);
		printf("\t%3u. %08x %p %p %08x %08x %08x\n", widx, (int)winp->base,
			winp->regs.lo32, winp->regs.hi32,
			R_REG(sysp->osh, winp->regs.lo32),
			(int)winp->vals.lo32, (int)winp->vals.hi32);
	}
	printf("\tUsr. mode enab widx addrlo32 addrhi32 rdwr_stats\n");
	for (uidx = 0; uidx < SBTOPCIE_USR_MAX; uidx++) {
		s2p_usr_t * usrp = S2P_USR(sysp, uidx);
		printf("\t%s. %s %u %4u %08x %08x %-10u\n",
			s2p_usr_name[usrp->uidx], s2p_mode_name[usrp->mode], usrp->enab,
			usrp->widx, (int)usrp->vals.lo32, (int)usrp->vals.hi32,
			usrp->stat);
	}
}   // sbtopcie_dump()

static void // hnd_cons_add_cmd()
s2p_cmd(void * arg, int argc, char * argv[])
{
	char *p = argv[1];
	if (argc < 2)  goto cmd_help;
	if (*p != '-') goto cmd_help;

	switch (*++p) {
		case 'd': sbtopcie_dump(); return;
		default : break;
	}

cmd_help: printf("s2p -[d|h]\n");
}   // s2p_cmd()

int // Initialize sbtopcie service
BCMATTACHFN(sbtopcie_init)(osl_t *osh,
	uint32 corerev, uint32 coreid, uint32 chip, volatile void * regs)
{
	uint32         win_base;
	sbpcieregs_t * sbpcieregs;
	uint8          widx, uidx;
	s2p_sys_t    * sysp   = S2P_SYS();
	s2p_usr_t    * usrp   = S2P_USR(sysp, 0);
	s2p_win_t    * winp   = S2P_WIN(sysp, 0);

	if (sysp->tot) // singleton attach
		return sysp->tot;

	memset(sysp, 0, sizeof(s2p_sys_t));

	sbpcieregs            = (sbpcieregs_t*)regs;
	sysp->osh             = osh;
	sysp->regs            = (sbpcieregs_t*)regs;

	if (PCIECOREREV(corerev) >= 24) {
		sysp->tot         = 4;  //  4 windows
		if (BCM43684_CHIP(chip) || BCM6715_CHIP(chip)) {
			win_base      = SBTOPCIE_BACKPLANE_BASE_CCI;
		} else { // Unsupported chip
			S2P_DEBUG(printf("S2P: pcie corerev >= 24 and ! CCI backplane\n"));
			return BCME_UNSUPPORTED;
		}
	} else {
		sysp->tot         = 2;  //  2 windows
		if (BCM4365_CHIP(chip)) {
			win_base      = SBTOPCIE_BACKPLANE_BASE_CCI;
		} else {
			win_base      = SBTOPCIE_BACKPLANE_BASE;
		}
	}

	ASSERT(sysp->tot <= SBTOPCIE_WIN_MAX);

	sysp->len             = S2P_WINDOW_SIZE(sysp->tot);
	sysp->map             = S2P_OFFSET_MASK(sysp->tot);
	sysp->cfg             = S2P_CONFIG_MASK(sysp->tot);

	/* sbtopcie registers are not arranged in order */
	(winp+0)->regs.lo32   = &sbpcieregs->sbtopcie0;       // 0x100
	(winp+0)->regs.hi32   = &sbpcieregs->sbtopcie0upper;  // 0x10C
	(winp+1)->regs.lo32   = &sbpcieregs->sbtopcie1;       // 0x104
	(winp+1)->regs.hi32   = &sbpcieregs->sbtopcie1upper;  // 0x110
	(winp+2)->regs.lo32   = &sbpcieregs->sbtopcie2;       // 0x0F0
	(winp+2)->regs.hi32   = &sbpcieregs->sbtopcie2upper;  // 0x0F4
	(winp+3)->regs.lo32   = &sbpcieregs->sbtopcie3;       // 0x0F8
	(winp+3)->regs.hi32   = &sbpcieregs->sbtopcie3upper;  // 0x0FC

	for (widx = 0; widx < sysp->tot; widx++) {
		winp              = S2P_WIN(sysp, widx);
		winp->base        = win_base;
		winp->usrp        = S2P_USR_SYS(sysp);
		winp->vals.u64    = 0ULL;

		win_base         += sysp->len;
	}

	for (uidx = 0; uidx < SBTOPCIE_USR_MAX; uidx++) {
		usrp              = S2P_USR(sysp, uidx);
		usrp->uidx        = uidx;
		usrp->mode        = SBTOPCIE_MODE_SHARED;
		usrp->widx        = SBTOPCIE_WIN_SYS;
		usrp->enab        = FALSE;
		usrp->stat        = 0U;
		usrp->vals.u64    = 0ULL;
	}

	hnd_cons_add_cmd("s2p", s2p_cmd, NULL);

	printf("S2P Service Initialization\n"
		   "S2P: corerev %u coreid %u sbpcieregs %p windows %u\n",
		   corerev, coreid, regs, sysp->tot);

	return sysp->tot;
}   // sbtopcie_init()

uintptr // Setup a user's sbtopcie window
sbtopcie_setup(uint32 uidx, uint64 haddr_u64, size_t size, uint32 mode)
{
	int         widx;
	s2p_win_t * winp;
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR(sysp, uidx);
	uintptr     daddr_uptr = SBTOPCIE_ADDR_INV;

	S2P_ASSERT((uidx < SBTOPCIE_USR_MAX) && (mode < SBTOPCIE_MODE_MAX));
	printf("S2P: setup %s lo 0x%08x hi 0x%08x size %u %s " S2P_CALL_FMT "\n",
	       s2p_usr_name[uidx], U64_TO_LO32(haddr_u64), U64_TO_HI32(haddr_u64),
	       (uint32)size, s2p_mode_name[mode], S2P_CALL_SITE);

	S2P_ASSERT(sysp->tot != 0); // not yet attached
	S2P_ASSERT((usrp->enab == 0) || (usrp->mode == mode));

	if (__s2p_range(sysp, U64_TO_LO32(haddr_u64), size) == SBTOPCIE_ADDR_INV) {
		printf("S2P: window overflow failure" S2P_CALL_FMT "\n",
			S2P_CALL_SITE);
		S2P_ASSERT(0);
		goto setup_failure;
	}

	/* Assign a window to a user based on requested mode */
	if (mode == SBTOPCIE_MODE_SHARED) {
		widx = SBTOPCIE_WIN_SYS;
		winp = S2P_WIN(sysp, widx);
		goto setup_win_done; // SBTOPCIE_WIN_SYS is reserved for shared mode
	}

	if (usrp->enab == TRUE) {
		widx = usrp->widx; // reuse previously allocated window
	} else {
		widx = __s2p_win_allocate(sysp); // allocate a new window
		if (widx <= SBTOPCIE_WIN_SYS) {
			printf("S2P: window resource allocation failure\n");
			sbtopcie_dump();
			goto setup_failure;
		}
	}

	ASSERT((widx > SBTOPCIE_WIN_SYS) && (widx < SBTOPCIE_WIN_MAX));

	winp     = S2P_WIN(sysp, widx);

	// Configure a translation window and attach to user
	__s2p_win_config(sysp, winp, haddr_u64, usrp);

setup_win_done:

	// Configure a user and attach to window. widx could be SBTOPCIE_WIN_SYS
	__s2p_usr_config(sysp, usrp, haddr_u64, mode, widx);
	usrp->enab = TRUE;

	// Translate by adding the haddr offset in a 32MB window to the SB window
	daddr_uptr = S2P_MAP_HADDR_64(sysp, winp, haddr_u64);

	usrp->stat++;

setup_failure:
	return daddr_uptr;

}   // sbtopcie_setup()

void // Release the sbtopcie window owned by a user
sbtopcie_release(uint32 uidx)
{
	uint64      z64  = 0ULL;
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR(sysp, uidx);
	s2p_win_t * winp;

	S2P_ASSERT((uidx < SBTOPCIE_USR_MAX) && (usrp->enab == TRUE));
	S2P_DEBUG(printf("S2P: release %s" S2P_CALL_FMT "\n",
	                 s2p_usr_name[uidx], S2P_CALL_SITE));

	S2P_ASSERT(usrp->widx < SBTOPCIE_WIN_MAX);

	if (usrp->enab == FALSE) return;

	winp = S2P_WIN(sysp, usrp->widx); // may be the system window
	__s2p_win_config(sysp, winp, z64, S2P_USR_SYS(sysp));
	__s2p_usr_config(sysp, usrp, z64, SBTOPCIE_MODE_SHARED, SBTOPCIE_WIN_SYS);
	usrp->enab = FALSE; // user released window and disabled

}   // sbtopcie_release()

uintptr // Translate a PCIE Memory address into a SB managed window
sbtopcie_translate(uint32 uidx, uint64 haddr_u64)
{
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR(sysp, uidx);
	s2p_win_t * winp;
	uintptr     daddr_uptr = SBTOPCIE_ADDR_INV;

	S2P_ASSERT((uidx < SBTOPCIE_USR_MAX) && (usrp->enab == TRUE));

	if (usrp->enab == FALSE) {
		S2P_ASSERT(usrp->enab == TRUE);
		goto translate_failure;
	}
	if (usrp->vals.hi32 != U64_TO_HI32(haddr_u64)) {
		S2P_ASSERT(usrp->vals.hi32 == U64_TO_HI32(haddr_u64));
		goto translate_failure;
	}

	winp    = S2P_WIN(sysp, usrp->widx);

	// Translate by adding the haddr offset in a 32MB window to the SB window
	daddr_uptr = S2P_MAP_HADDR_64(sysp, winp, haddr_u64);
	usrp->stat++;

translate_failure:

	S2P_DEBUG(printf("S2P: translate %s lo 0x%08x hi 0x%08x daddr32 0x%08x"
			         S2P_CALL_FMT "\n", s2p_usr_name[uidx],
			         U64_TO_LO32(haddr_u64), U64_TO_HI32(haddr_u64),
			         (uint32)daddr_uptr, S2P_CALL_SITE));

	return daddr_uptr;

}   // sbtopcie_translate()

bool // Test whether a host address is in user's window
sbtopcie_valid(uint32 uidx, uint64 haddr_u64, size_t bytes)
{
	bool        is_valid = FALSE;
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR(sysp, uidx);
	uintptr     base;

	S2P_ASSERT(uidx < SBTOPCIE_USR_MAX);

	base = (uintptr)(usrp->vals.lo32 & S2P_CONFIG_MASK(sysp->tot));

	// Check whether an address falls in the configured translation window
	if (usrp->enab == 0) goto not_valid;
	if (usrp->vals.hi32 != U64_TO_HI32(haddr_u64)) goto not_valid;
	if (__s2p_range(sysp, U64_TO_LO32(haddr_u64), bytes) != base) goto not_valid;

	is_valid = TRUE;

not_valid:

	S2P_DEBUG(
		printf("S2P: valid %s lo 0x%08x hi 0x%08x %s lo 0x%08x hi 0x%08x "
			         "bytes %u" S2P_CALL_FMT "\n",
			         s2p_usr_name[uidx], usrp->vals.lo32, usrp->vals.hi32,
			         is_valid ? "~~" : "!~",
			         U64_TO_LO32(haddr_u64), U64_TO_HI32(haddr_u64),
			         (uint32)bytes, S2P_CALL_SITE));
	usrp->stat++;

	return is_valid;

}   // sbtopcie_valid()

void // Switch the system window to a specified user that was setup as shared
sbtopcie_switch(uint32 uidx)
{
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR(sysp, uidx);
	s2p_win_t * winp = S2P_WIN_SYS(sysp);

	S2P_ASSERT((uidx < SBTOPCIE_USR_MAX) && (usrp->enab == TRUE));
	S2P_DEBUG(printf("S2P: switch %s" S2P_CALL_FMT "\n",
	                 s2p_usr_name[uidx], S2P_CALL_SITE));

	ASSERT(usrp->mode == SBTOPCIE_MODE_SHARED);
	S2P_ASSERT(usrp->widx == SBTOPCIE_WIN_SYS);

	__s2p_win_config(sysp, winp, usrp->vals.u64, usrp);

	usrp->stat++;
}   // sbtopcie_switch()

void // Restore the system window
sbtopcie_restore(uint32 uidx)
{
	uint64      z64  = 0ULL;
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR(sysp, uidx);
	s2p_win_t * winp = S2P_WIN_SYS(sysp);

	S2P_ASSERT((uidx < SBTOPCIE_USR_MAX) && (usrp->enab == TRUE));
	S2P_ASSERT(usrp->mode == SBTOPCIE_MODE_SHARED);
	S2P_ASSERT(usrp->widx == SBTOPCIE_WIN_SYS);
	S2P_DEBUG(printf("S2P: restore %s" S2P_CALL_FMT "\n",
	                 s2p_usr_name[uidx], S2P_CALL_SITE));

	__s2p_win_config(sysp, winp, z64, S2P_USR_SYS(sysp));

	usrp->stat++;
}   // sbtopcie_restore()

// +--------------------------------------------------------------------------+
//  Generic System Service for unregistered users
// +--------------------------------------------------------------------------+
void // Programmed IO access 1 Byte, 2 Byte or 4 Byte using sbtopcie
sbtopcie_pio(uintptr daddr_uptr, uint64 haddr_u64, size_t pio_size, uint32 dir)
{
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR_SYS(sysp);
	s2p_win_t * winp = S2P_WIN_SYS(sysp);

	uintptr     sb_uptr;  // remap haddr_u64 into sbtopcie window
	s2p_ptr_t   src, dst; // *(dst.p##) = *(src.p##)

	S2P_RANGE_AUDIT(sysp, haddr_u64, pio_size);

	sb_uptr      = S2P_MAP_HADDR_64(sysp, winp, haddr_u64);

	if (dir == SBTOPCIE_DIR_H2D) {
		src.uptr = sb_uptr;
		dst.uptr = daddr_uptr;
	} else {
		src.uptr = daddr_uptr;
		dst.uptr = sb_uptr;
	}

	S2P_DEBUG(printf("S2P: pio *(%p) = *(%p) %u bytes" S2P_CALL_FMT "\n",
	                 dst.vptr, src.vptr, (uint32)pio_size, S2P_CALL_SITE));

	__s2p_win_config(sysp, winp, haddr_u64, usrp);

	switch (pio_size) {
		case 1 : *(dst.p8)  = *(src.p8);  break;
		case 2 : *(dst.p16) = *(src.p16); break;
		case 4 : *(dst.p32) = *(src.p32); break;
		case 8 : *(dst.p64) = *(src.p64); break; // PCIE WRAP ?
		default: ASSERT(0);               break;
	}

	__s2p_win_sys_reset(sysp); // __s2p_win_config z64

	usrp->stat++;

}   // sbtopcie_pio()

void // Copy 32bit words using sbtopcie
sbtopcie_cpy32(uintptr daddr_uptr, uint64 haddr_u64, uint32 words, uint32 dir)
{
	uint32      idx;
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR_SYS(sysp);
	s2p_win_t * winp = S2P_WIN_SYS(sysp);

	uintptr     sb_uptr;  // remap haddr_u64 into sbtopcie window
	s2p_ptr_t   src, dst; // *(dst.p##) = *(src.p##)

	S2P_RANGE_AUDIT(sysp, haddr_u64, words * sizeof(uint32));

	sb_uptr      = S2P_MAP_HADDR_64(sysp, winp, haddr_u64);

	if (dir ==  SBTOPCIE_DIR_H2D) { // src[host] >>  dst[dngl]
		src.uptr = sb_uptr;         // src[host] via sbtopcie map of haddr_u64
		dst.uptr = daddr_uptr;      // dst[dngl]
	} else {                        // src[dngl] >> dst[host]
		src.uptr = daddr_uptr;      // src[dngl]
		dst.uptr = sb_uptr;         // dst[host] via sbtopcie map of haddr_u64
	}

	S2P_DEBUG(printf("S2P: cpy32 *(%p) = *(%p) x %u words" S2P_CALL_FMT "\n",
	                 dst.vptr, src.vptr, words, S2P_CALL_SITE));

	__s2p_win_config(sysp, winp, haddr_u64, usrp);

	// if words is a multiple of 8, use ldmia, stmia with 8 registers.
	//    rte_memcpy32(dst.vptr, src.vptr, words >> 3) bcmlibarm.h
	for (idx = 0; idx < words; idx++) {
		// 4-Byte aligned copy, of 4-Byte words
		dst.p32[idx] = src.p32[idx];
	}

	__s2p_win_sys_reset(sysp); // __s2p_win_config z64

	usrp->stat++;

}   // sbtopcie_cpy32()

void // Copy bytes using sbtopcie
sbtopcie_cpy(uintptr daddr_uptr, uint64 haddr_u64, size_t bytes, uint32 dir)
{
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR_SYS(sysp);
	s2p_win_t * winp = S2P_WIN_SYS(sysp);

	uintptr     sb_uptr;  // remap haddr_u64 into sbtopcie window
	s2p_ptr_t   src, dst; // *(dst.p##) = *(src.p##)

	S2P_RANGE_AUDIT(sysp, haddr_u64, bytes);

	sb_uptr      = S2P_MAP_HADDR_64(sysp, winp, haddr_u64);

	if (dir ==  SBTOPCIE_DIR_H2D) { // src[host] >> dst[dngl]
		src.uptr = sb_uptr;         // src[host] via sbtopcie map
		dst.uptr = daddr_uptr;      // dst[dngl[
	} else {                        // src[dngl] >> dst[host]
		src.uptr = daddr_uptr;      // src[dngl]
		dst.uptr = sb_uptr;         // dst[host] via sbtopcie map
	}

	S2P_DEBUG(printf("S2P: cpy *(%p) = *(%p) x %u bytes" S2P_CALL_FMT "\n",
	                 dst.vptr, src.vptr, (uint32)bytes, S2P_CALL_SITE));

	__s2p_win_config(sysp, winp, haddr_u64, usrp);

	memcpy((void *)dst.uptr, (void *)src.uptr, bytes);

	__s2p_win_sys_reset(sysp); // __s2p_win_config z64

	usrp->stat++;

}   // sbtopcie_cpy()

void // Print Host Memory using sbtopcie
sbtopcie_print(uint64 haddr_u64, size_t bytes)
{
	s2p_sys_t * sysp = S2P_SYS();
	s2p_usr_t * usrp = S2P_USR_SYS(sysp);
	s2p_win_t * winp = S2P_WIN_SYS(sysp);

	uint32      i;
	uint8     * sb_mem; // map host memory haddr_u64 into sbtopcie window

	S2P_RANGE_AUDIT(sysp, haddr_u64, bytes);

	sb_mem           = (uint8*)S2P_MAP_HADDR_64(sysp, winp, haddr_u64);

	__s2p_win_config(sysp, winp, haddr_u64, usrp);

	printf("S2P: Host Mem Dump @ lo 0x%08x hi 0x%08x sb_mem %p bytes %u :",
		U64_TO_LO32(haddr_u64), U64_TO_HI32(haddr_u64), sb_mem, (uint32)bytes);

	for (i = 0; i < bytes; ++i)
	{
		if ((i % 16) == 0)
			printf("\n\t%3u.\t", i);
		printf("%02x ", sb_mem[i]);
	}
	printf("\n");

	__s2p_win_sys_reset(sysp); // __s2p_win_config z64

	usrp->stat++;

}   // sbtopcie_print()
