/*
 * Common interface for Unified Interrupt Controller module.
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
 */

#include <hnduic.h>
#include <hnduic_regs.h>
#include <hndsoc.h>

#include <wl_dbg.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <bcmutils.h>

#define UIC_PRINT(args)		printf args
#define UIC_ERROR(args)		printf args
#ifdef BCMDBG
#define UIC_INFORM(args)	UIC_PRINT(args)
#else
#define UIC_INFORM(args)
#endif /* BCMDBG */

#define UIC_GROUP_COUNT		10
#define UIC_SUBGROUP_COUNT	8
#define UIC_REGS_PER_GROUP	4
#define UIC_SUBGROUPS_PER_REG	8

#define UIC_SIMULATE		/* Enable UIC core simulation */

/*
 *
 * Private data structures
 *
 */

struct uic_context {
	osl_t	*osh;		/* OSL handle */
	si_t	*sih;		/* SI handle */
	uint32	intmask;	/* Cached UIC interrupt mask */
};

static uic_context_t *g_uic_context = NULL;

#ifdef BCMDBG
static const char* group_strings[] = {
	"CC", "PMU", "M2M", "HWA0", "HWA1", "PCIE0", "PCIE1", "PCIE2", "MAC0", "MAC1"
};
#endif /* BCMDBG */

#ifdef UIC_SIMULATE
static uint32 _uic_sim_regs[128] = { 0x0 };
#endif /* UIC_SIMULATE */

/*
 * Internal functions
 */

/**
 * UIC hardware availability check
 *
 * @param sih		SI handle.
 * @return		TRUE if UIC is available.
*/

static bool
BCMATTACHFN(is_uic_available)(si_t *sih)
{
#ifndef UIC_SIMULATE
	return BCM6715_CHIP(sih->chip);
#else
	return TRUE;
#endif /* UIC_SIMULATE */
}

/**
 * Write UIC register.
 *
 * @param uic		Pointer to UIC context.
 * @param address	Register address relative to UIC base.
 * @param value		Value to write.
 */

static INLINE void
write_register(uic_context_t *uic, uint32 address, uint32 value)
{
	ASSERT(address >= gci_uic_UIC_CFG);
	ASSERT(address <= gci_uic_UIC_group_mapped_sel9_3);

#ifndef UIC_SIMULATE
	W_REG(uic->osh, (uint32*)(SI_ENUM_BASE(uic->sih) + address), value);
#else
	_uic_sim_regs[(address - gci_uic_UIC_CFG) / sizeof(uint32)] = value;
#endif /* UIC_SIMULATE */
}

/**
 * Read register.
 *
 * @param uic		Pointer to UIC context.
 * @param address	Register address relative to UIC base.
 * @return		Value.
 */

static INLINE uint32
read_register(uic_context_t *uic, uint32 address)
{
	ASSERT(address >= gci_uic_UIC_CFG);
	ASSERT(address <= gci_uic_UIC_group_mapped_sel9_3);

#ifndef UIC_SIMULATE
	return R_REG(uic->osh, (uint32*)(SI_ENUM_BASE(uic->sih) + address));
#else
	if (address == gci_uic_UIC_CFG) {
		_uic_sim_regs[0] |= (UIC_GROUP_COUNT << gci_uic_UIC_CFG_GroupNumInUIC_SHIFT);
		_uic_sim_regs[0] &= ~gci_uic_UIC_CFG_CFG_Start_MASK;
	}
	return _uic_sim_regs[(address - gci_uic_UIC_CFG) / sizeof(uint32)];
#endif /* UIC_SIMULATE */
}

/**
 * Modify register.
 *
 * Uses read-modify-write to modify register bits indicated by a mask.
 *
 * @param uic		Pointer to UIC context.
 * @param address	Register address relative to UIC base.
 * @param value		Value.
 * @param mask		Mask.
 */

static void
BCMATTACHFN(modify_register)(uic_context_t *uic, uint32 address, uint32 value, uint32 mask)
{
	ASSERT((value & ~mask) == 0);

	write_register(uic, address, (read_register(uic, address) & ~mask) | (value & mask));
}

/**
 * Get subgroup mapping register address.
 *
 * @param group		Source group.
 * @param bit		Source bit.
 * @return		Subgroup mapping register address relative to UIC base address.
 */

static INLINE uint32*
get_group_address(uic_group_t group, uint bit)
{
	ASSERT(group < UIC_GROUP_COUNT);
	ASSERT(bit < 32);

	uint32 *address = (uint32*)gci_uic_UIC_group_mapped_sel0_0;
	return address + group * UIC_REGS_PER_GROUP + (bit / 8);
}

#ifdef BCMDBG

/**
 * Get source group as a string.
 *
 * @param source	Source group index.
 * @return		Source group name.
 */

static INLINE const char*
group_to_string(uic_group_t group)
{
	ASSERT(group < ARRAYSIZE(group_strings);

	return group_strings[group];
}

/**
 * Dump subgroup mappings.
 *
 * @param uic		Pointer to UIC context.
 */

static void
dump_mappings(uic_context_t *uic)
{
	uint* address;
	uint group, subgroup, reg, bit = 0, assigned_subgroup;
	uint8 mappings[UIC_GROUP_COUNT][UIC_SUBGROUP_COUNT] = {{ 0 }};
	uint32 value;

	/* Print subgroup mappings per source group */

	UIC_PRINT(("%-10s %-5s %29s\n", "SRC_GROUP", "BIT31", "BIT0"));

	for (group = 0; group < UIC_GROUP_COUNT; group++) {
		UIC_PRINT(("U%02u:%-6s", group, group_to_string(group)));
		address = get_group_address(group, 31);
		for (reg = 0; reg < UIC_REGS_PER_GROUP; reg++, address--) {
			UIC_PRINT((" "));
			value = read_register(uic, (uint32)address);
			for (subgroup = 0; subgroup < UIC_SUBGROUPS_PER_REG; subgroup++) {
				if (value & 0x80000000) {
					assigned_subgroup = (value >> 28) & 0x7;
					mappings[group][assigned_subgroup]++;
					UIC_PRINT(("%1d", assigned_subgroup));
				} else {
					UIC_PRINT(("."));
				}
				value <<= 4;
			}
		}
		UIC_PRINT(("\n"));
	}

	/* Print subgroup to intstatus register mappings */

	UIC_PRINT(("\n%3s %-10s %8s\n", "BIT", "SRC_GROUP", "SUBGROUP"));

	bit = 0;
	for (group = 0; group < UIC_GROUP_COUNT; group++) {
		for (subgroup = 0; subgroup < UIC_SUBGROUP_COUNT; subgroup++) {
			if (mappings[group][subgroup] != 0) {
				UIC_PRINT(("%-3u U%02u:%-6s %8u\n", bit++,
					group, group_to_string(group), subgroup));
			}
		}
	}
}

/**
 * Dump source group interrupt status and UIC interrupt status.
 *
 * @param uic		Pointer to UIC context.
 */

static void
dump_status(uic_context_t *uic)
{
	uint group;
	uint32 status;

	BCM_REFERENCE(uic);

	for (group = 0; group < UIC_GROUP_COUNT; group++) {
		status = uic_get_group_interrupts(group);
		if (status) {
			UIC_PRINT(("%s:0x%08x ", group_to_string(group), status));
		}
	}

	UIC_PRINT(("UIC:0x%08x\n", uic_get_interrupts()));
}

#endif /* BCMDBG */

/**
 * Configure subgroup mappings for a source group
 *
 * @param uic		Pointer to UIC context.
 * @param group		Source group.
 * @param mappings3	Mappings for bits 24..31
 * @param mappings2	Mappings for bits 16..23
 * @param mappings1	Mappings for bits 8..15
 * @param mappings0	Mappings for bits 0..7
 *
 * Source bits assigned to subgroups 0 through 7 are mapped to UIC intstatus bits, source bits
 * assigned to subgroup 8 (or up) are not mapped. Each mapping byte encodes two 4-bit subgroup
 * assignments.
 *
 * @note The meaning of bit 0x8 (mapped versus unmapped) is inverted compared to the encoding
 * used by UIC hardware, allowing easier subgroup assignment by callers.
 */

static void
BCMATTACHFN(uic_map)(uic_context_t *uic, uic_group_t group,
	uint32 mappings3, uint32 mappings2, uint32 mappings1, uint32 mappings0)
{
	uint32 *address = get_group_address(group, 0);

	write_register(uic, (uint32)address++, mappings0 ^ 0x88888888);
	write_register(uic, (uint32)address++, mappings1 ^ 0x88888888);
	write_register(uic, (uint32)address++, mappings2 ^ 0x88888888);
	write_register(uic, (uint32)address++, mappings3 ^ 0x88888888);
}

/**
 * Configure UIC for 6715
 *
 * @param uic		Pointer to UIC context.
 */

static void
BCMATTACHFN(uic_map_interrupts_6715)(uic_context_t *uic)
{
	/*
	              --- Source group                  Target bit in UIC_INTSTATUS --------
	             |                                                                      |
	             V                                                                      V
	*/

	uic_map(uic, UIC_GROUP_CC,      0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 0 */
	uic_map(uic, UIC_GROUP_PMU,     0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 1 */
	uic_map(uic, UIC_GROUP_M2M,     0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 2 */
	uic_map(uic, UIC_GROUP_HWA0,    0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 3 */
	uic_map(uic, UIC_GROUP_HWA1,    0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 4 */
	uic_map(uic, UIC_GROUP_PCIE0,   0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 5 */
	uic_map(uic, UIC_GROUP_PCIE1,   0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 6 */
	uic_map(uic, UIC_GROUP_PCIE2,   0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 7 */
	uic_map(uic, UIC_GROUP_MAC0,    0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 8 */
	uic_map(uic, UIC_GROUP_MAC1,    0x00000000, 0x00000000, 0x00000000, 0x00000000); /* 9 */

	/*
	                                         ^           ^           ^           ^
	                                         |           |           |           |
	        Subgroup mapping bits 31..24 ----            |           |           |
	        Subgroup mapping bits 23..16 ----------------            |           |
	        Subgroup mapping bits 15..8  ----------------------------            |
	        Subgroup mapping bits  7..0  ----------------------------------------

	The subgroup mapping values specify how each bit in the source interrupt status register
	should be assigned to one of nine (0..8) subgroups. Two subgroup mappings are encoded per
	byte of the mapping values, each taking up 4 bits.

	For subgroups 0 through 7, all bits from a source group assigned to the same subgroup are
	ORed together and the resulting value is copied to one target bit in the UIC intstatus
	register. Source bits assigned to subgroup 8 are ignored.

	The order in which the resulting bits are assigned to the target bits in the UIC intstatus
	register is fixed. The number of target bits in the UIC intstatus register assigned to
	each source group is equal to the number of subgroups to which source bits are mapped.
	This means that changes to (the number of) subgroup mappings for a source group may result
	in target bits associated to other cores to be shifted in the UIC intstatus register. The
	following pseudocode illustrates how mapping is done:

		foreach source group from UIC_GROUP_CC to UIC_GROUP_MAC1:
			foreach subgroup mapping for that source group, from 0 to 7:
				if any source bits mapped to that subgroup:
					allocate one bit in UIC intstatus register

	Example:

		uic_map(uic, UIC_GROUP_CC,  0x33333333, 0x22222222, 0x11111111, 0x00000000);
		uic_map(uic, UIC_GROUP_M2M, 0x88888888, 0x88888888, 0x00000000, 0x88888888);

		UIC_INTSTATUS:
			- bit 0 = OR(CC intstatus bits 0..7)
			- bit 1 = OR(CC intstatus bits 8..15)
			- bit 2 = OR(CC intstatus bits 16..23)
			- bit 3 = OR(CC intstatus bits 24..31)
			- bit 4 = OR(M2M intstatus bits 8..15)
	*/
}

/*
 * Export functions
 */

/**
 * Initialize UIC driver.
 *
 * @param sih		SI handle.
 * @param osh		OS handle.
 * @return		BCME_OK on success or UIC not available.
 */

int
BCMATTACHFN(uic_init)(si_t *sih, osl_t *osh)
{
	uint32 value;
	uic_context_t *uic;

	ASSERT(g_uic_context == NULL);

	if (!is_uic_available(sih)) {
		UIC_INFORM(("%s: UIC not available\n", __FUNCTION__));
		return BCME_OK;
	}

	UIC_INFORM(("%s: initializing UIC\n", __FUNCTION__));

	uic = (uic_context_t*)MALLOCZ(osh, sizeof(uic_context_t));
	if (uic == NULL) {
		UIC_ERROR(("%s: out of mem, malloced %d bytes\n", __FUNCTION__, MALLOCED(osh)));
		return BCME_NORESOURCE;
	}

	uic->sih = sih;
	uic->osh = osh;
	g_uic_context = uic;

	/* Enable module, enable all groups, no unmapped groups */
	write_register(uic,
		gci_uic_UIC_CFG, gci_uic_UIC_CFG_ModuleEn_MASK |
		gci_uic_UIC_CFG_IPIntStatusEn_MASK);

	/* Start configuration and wait for config done */
	modify_register(uic,
		gci_uic_UIC_CFG, gci_uic_UIC_CFG_CFG_Start_MASK,
		gci_uic_UIC_CFG_CFG_Start_MASK);
	do {
		OSL_DELAY(5);
		value = read_register(uic, gci_uic_UIC_CFG);
	} while (value & gci_uic_UIC_CFG_CFG_Start_MASK);

	/* Check number of groups */
	ASSERT((value & gci_uic_UIC_CFG_GroupNumInUIC_MASK) >>
		 gci_uic_UIC_CFG_GroupNumInUIC_SHIFT == UIC_GROUP_COUNT);

#ifndef UIC_SIMULATE
	if (BCM6715_CHIP(sih->chip)) {
		uic_map_interrupts_6715(uic);
	} else {
		ASSERT(0);
	}
#else
	uic_map_interrupts_6715(uic);
#endif /* UIC_SIMULATE */

	/* Disable all interrupts */
	uic_enable_interrupts(0x0, 0xffffffff);

	UIC_INFORM(("%s: initialized, %u groups\n", __FUNCTION__,
		(value & gci_uic_UIC_CFG_GroupNumInUIC_MASK) >>
		gci_uic_UIC_CFG_GroupNumInUIC_SHIFT));

	return BCME_OK;
}

/**
 * Deinitialize UIC driver.
 */

void
BCMATTACHFN(uic_deinit)(void)
{
	UIC_PRINT(("%s\n", __FUNCTION__));

	if (g_uic_context != NULL) {

		/* Disable all interrupts */
		uic_enable_interrupts(0x0, 0xffffffff);

		MFREE(g_uic_context->osh, g_uic_context, sizeof(uic_context_t));
		g_uic_context = NULL;
	}
}

/**
 * Interrupt control for UIC interrupts.
 *
 * @param enable_mask	Bitmap of UIC interrupts to enable.
 * @param disable_mask	Bitmap of UIC interrupts to disable.
 */

void
uic_enable_interrupts(uint32 enable_mask, uint32 disable_mask)
{
	uic_context_t *uic = g_uic_context;

	ASSERT(uic != NULL);

	uic->intmask &= ~disable_mask;
	uic->intmask |=  enable_mask;

	UIC_INFORM(("%s: new mask 0x%08x\n", __FUNCTION__, uic->intmask));
	write_register(uic, gci_uic_UIC_INTMASK, uic->intmask);
}

/**
 * Get UIC interrupt status.
 *
 * @return		UIC interrupt status.
 */

INLINE uint32
uic_get_interrupts(void)
{
	uic_context_t *uic = g_uic_context;

	ASSERT(uic != NULL);

	return read_register(uic, gci_uic_UIC_INTSTATUS);
}

/**
 * Get source group interrupt status.
 *
 * @param group		Source group.
 * @return		Source group interrupt status.
 */

INLINE uint32
uic_get_group_interrupts(uic_group_t group)
{
	uic_context_t *uic = g_uic_context;

	ASSERT(uic != NULL);
	ASSERT(group < UIC_GROUP_COUNT);

	return read_register(uic, gci_uic_UIC_INTSTATUS0 + (group * sizeof(uint32)));
}

void
uic_cmd(void *arg, int argc, char *argv[])
{
#ifdef BCMDBG
	if (g_uic_context == NULL) {
		UIC_PRINT(("UIC not initialized\n"));
		return;
	}

	printf_suppress_timestamp(TRUE);

	if (argc > 1) {
		if (!strcmp(argv[1], "dump")) {
			dump_mappings(g_uic_context);
		} else if (!strcmp(argv[1], "status")) {
			dump_status(g_uic_context);
		}
	}

	printf_suppress_timestamp(FALSE);
#endif /* BCMDBG */
}
