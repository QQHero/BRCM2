/*
   Copyright (c) 2015 Broadcom
   All Rights Reserved

    <:label-BRCM:2015:DUAL/GPL:standard

    Unless you and Broadcom execute a separate written software license
    agreement governing use of this software, this software is licensed
    to you under the terms of the GNU General Public License version 2
    (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
    with the following added to such license:

       As a special exception, the copyright holders of this software give
       you permission to link this software with independent modules, and
       to copy and distribute the resulting executable under terms of your
       choice, provided that you also meet, for each linked independent
       module, the terms and conditions of the license of that module.
       An independent module is a module which is not derived from this
       software.  The special exception does not apply to any modifications
       of the software.

    Not withstanding the above, under no circumstances may you combine
    this software in any way with any other Broadcom software provided
    under a license other than the GPL, without Broadcom's express prior
    written consent.

:>
*/

#include "ru.h"

#if RU_INCLUDE_FIELD_DB
/******************************************************************************
 * Field: QEGPHY__REVISION_REVISION
 ******************************************************************************/
const ru_field_rec QEGPHY__REVISION_REVISION_FIELD =
{
    "REVISION",
#if RU_INCLUDE_DESC
    "REVISION",
    "Quad GPHY implements its own revision control register. Refer to Quad GPHY register specification.",
#endif
    QEGPHY__REVISION_REVISION_FIELD_MASK,
    0,
    QEGPHY__REVISION_REVISION_FIELD_WIDTH,
    QEGPHY__REVISION_REVISION_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__REVISION_RESERVED0
 ******************************************************************************/
const ru_field_rec QEGPHY__REVISION_RESERVED0_FIELD =
{
    "RESERVED0",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    QEGPHY__REVISION_RESERVED0_FIELD_MASK,
    0,
    QEGPHY__REVISION_RESERVED0_FIELD_WIDTH,
    QEGPHY__REVISION_RESERVED0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY_TEST_CNTRL_PHY_TEST_EN
 ******************************************************************************/
const ru_field_rec QEGPHY_TEST_CNTRL_PHY_TEST_EN_FIELD =
{
    "PHY_TEST_EN",
#if RU_INCLUDE_DESC
    "PHY_TEST_EN",
    "Enables EGPHY test mode within chip testmux:"
    "0 : EGPHY is not in test mode."
    "1 : EGPHY is in test mode."
    "Note:This bit is combined with PHY_SEL."
    "",
#endif
    QEGPHY_TEST_CNTRL_PHY_TEST_EN_FIELD_MASK,
    0,
    QEGPHY_TEST_CNTRL_PHY_TEST_EN_FIELD_WIDTH,
    QEGPHY_TEST_CNTRL_PHY_TEST_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY_TEST_CNTRL_RESERVED0
 ******************************************************************************/
const ru_field_rec QEGPHY_TEST_CNTRL_RESERVED0_FIELD =
{
    "RESERVED0",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    QEGPHY_TEST_CNTRL_RESERVED0_FIELD_MASK,
    0,
    QEGPHY_TEST_CNTRL_RESERVED0_FIELD_WIDTH,
    QEGPHY_TEST_CNTRL_RESERVED0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_IDDQ_BIAS
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_IDDQ_BIAS_FIELD =
{
    "IDDQ_BIAS",
#if RU_INCLUDE_DESC
    "IDDQ_BIAS",
    "Power down BIAS. When 1b1. the internal bias is put into iddq mode."
    "The energy_det output is not valid when this input is set."
    "Requires HW reset(see bit 8 of this register) to bring EGPHY back from power down.",
#endif
    QEGPHY__CNTRL_IDDQ_BIAS_FIELD_MASK,
    0,
    QEGPHY__CNTRL_IDDQ_BIAS_FIELD_WIDTH,
    QEGPHY__CNTRL_IDDQ_BIAS_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_EXT_PWR_DOWN
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_EXT_PWR_DOWN_FIELD =
{
    "EXT_PWR_DOWN",
#if RU_INCLUDE_DESC
    "EXT_PWR_DOWN",
    "When any of bits is set, corresponding EGPHY AFE is powered down."
    "When 4b1111 and force_dll_en=0, quad EGPHY DLL is also powered down."
    "Requires SW reset to bring EGPHY back from power down."
    "",
#endif
    QEGPHY__CNTRL_EXT_PWR_DOWN_FIELD_MASK,
    0,
    QEGPHY__CNTRL_EXT_PWR_DOWN_FIELD_WIDTH,
    QEGPHY__CNTRL_EXT_PWR_DOWN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_FORCE_DLL_EN
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_FORCE_DLL_EN_FIELD =
{
    "FORCE_DLL_EN",
#if RU_INCLUDE_DESC
    "FORCE_DLL_EN",
    "Force DLL on",
#endif
    QEGPHY__CNTRL_FORCE_DLL_EN_FIELD_MASK,
    0,
    QEGPHY__CNTRL_FORCE_DLL_EN_FIELD_WIDTH,
    QEGPHY__CNTRL_FORCE_DLL_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_IDDQ_GLOBAL_PWR
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_IDDQ_GLOBAL_PWR_FIELD =
{
    "IDDQ_GLOBAL_PWR",
#if RU_INCLUDE_DESC
    "IDDQ_GLOBAL_PWR",
    "Enables isolation cells for quad EGPHY power gating.",
#endif
    QEGPHY__CNTRL_IDDQ_GLOBAL_PWR_FIELD_MASK,
    0,
    QEGPHY__CNTRL_IDDQ_GLOBAL_PWR_FIELD_WIDTH,
    QEGPHY__CNTRL_IDDQ_GLOBAL_PWR_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_CK25_DIS
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_CK25_DIS_FIELD =
{
    "CK25_DIS",
#if RU_INCLUDE_DESC
    "CK25_DIS",
    "Disable 25 MHz clock to quad EGPHY."
    "This bit should be set in quad EGPHY power down mode only when MDIO access is not required.",
#endif
    QEGPHY__CNTRL_CK25_DIS_FIELD_MASK,
    0,
    QEGPHY__CNTRL_CK25_DIS_FIELD_WIDTH,
    QEGPHY__CNTRL_CK25_DIS_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_PHY_RESET
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_PHY_RESET_FIELD =
{
    "PHY_RESET",
#if RU_INCLUDE_DESC
    "PHY_RESET",
    "Quad EGPHY system reset. Must be held high for at leaset 60 us while system clock is running."
    "After reset de-assertion no EGPHY activity should occur for 20 us.",
#endif
    QEGPHY__CNTRL_PHY_RESET_FIELD_MASK,
    0,
    QEGPHY__CNTRL_PHY_RESET_FIELD_WIDTH,
    QEGPHY__CNTRL_PHY_RESET_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_RESERVED0
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_RESERVED0_FIELD =
{
    "RESERVED0",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    QEGPHY__CNTRL_RESERVED0_FIELD_MASK,
    0,
    QEGPHY__CNTRL_RESERVED0_FIELD_WIDTH,
    QEGPHY__CNTRL_RESERVED0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_PHY_PHYAD
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_PHY_PHYAD_FIELD =
{
    "PHY_PHYAD",
#if RU_INCLUDE_DESC
    "PHY_PHYAD",
    "Quad EGPHY base PHY address."
    "EGPHYs within quad EGPHY are addressed as base address + offset where offset=0,1,2,3.",
#endif
    QEGPHY__CNTRL_PHY_PHYAD_FIELD_MASK,
    0,
    QEGPHY__CNTRL_PHY_PHYAD_FIELD_WIDTH,
    QEGPHY__CNTRL_PHY_PHYAD_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_PLL_REFCLK_SEL
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_PLL_REFCLK_SEL_FIELD =
{
    "PLL_REFCLK_SEL",
#if RU_INCLUDE_DESC
    "PLL_REFCLK_SEL",
    "These bits are used to select the reference clock source to EGPHY:"
    "00 : i_pll_refclk[0]."
    "01 : i_pll_refclk[1]."
    "10 : i_pll_refclk[2]."
    "11 : TVCO."
    "Note: Do note change these bits from their default value before consulting with Broadcom."
    "",
#endif
    QEGPHY__CNTRL_PLL_REFCLK_SEL_FIELD_MASK,
    0,
    QEGPHY__CNTRL_PLL_REFCLK_SEL_FIELD_WIDTH,
    QEGPHY__CNTRL_PLL_REFCLK_SEL_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_PLL_SEL_DIV5
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_PLL_SEL_DIV5_FIELD =
{
    "PLL_SEL_DIV5",
#if RU_INCLUDE_DESC
    "PLL_SEL_DIV5",
    "These bits are used to select the frequency of the reference clock source to EGPHY:"
    "00 : 25MHz."
    "01 : 54MHz."
    "10 : 50MHz."
    "11 : 40MHz."
    "Note: Do note change these bits from their default value before consulting with Broadcom."
    "",
#endif
    QEGPHY__CNTRL_PLL_SEL_DIV5_FIELD_MASK,
    0,
    QEGPHY__CNTRL_PLL_SEL_DIV5_FIELD_WIDTH,
    QEGPHY__CNTRL_PLL_SEL_DIV5_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_PLL_CLK125_250_SEL
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_PLL_CLK125_250_SEL_FIELD =
{
    "PLL_CLK125_250_SEL",
#if RU_INCLUDE_DESC
    "PLL_CLK125_250_SEL",
    "This bit is used to select the EGPHY PLL output clock frequency:"
    "0 : 125MHz."
    "1 : 250MHz."
    "Note: Do note change this bit from its default value before consulting with Broadcom."
    "",
#endif
    QEGPHY__CNTRL_PLL_CLK125_250_SEL_FIELD_MASK,
    0,
    QEGPHY__CNTRL_PLL_CLK125_250_SEL_FIELD_WIDTH,
    QEGPHY__CNTRL_PLL_CLK125_250_SEL_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__CNTRL_RESERVED1
 ******************************************************************************/
const ru_field_rec QEGPHY__CNTRL_RESERVED1_FIELD =
{
    "RESERVED1",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    QEGPHY__CNTRL_RESERVED1_FIELD_MASK,
    0,
    QEGPHY__CNTRL_RESERVED1_FIELD_WIDTH,
    QEGPHY__CNTRL_RESERVED1_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: QEGPHY__STATUS_ENERGY_DET_MASKED
 ******************************************************************************/
const ru_field_rec QEGPHY__STATUS_ENERGY_DET_MASKED_FIELD =
{
    "ENERGY_DET_MASKED",
#if RU_INCLUDE_DESC
    "ENERGY_DET_MASKED",
    "Filtered Energy Detect.",
#endif
    QEGPHY__STATUS_ENERGY_DET_MASKED_FIELD_MASK,
    0,
    QEGPHY__STATUS_ENERGY_DET_MASKED_FIELD_WIDTH,
    QEGPHY__STATUS_ENERGY_DET_MASKED_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: QEGPHY__STATUS_ENERGY_DET_APD
 ******************************************************************************/
const ru_field_rec QEGPHY__STATUS_ENERGY_DET_APD_FIELD =
{
    "ENERGY_DET_APD",
#if RU_INCLUDE_DESC
    "ENERGY_DET_APD",
    "Filtered Energy Detect in Auto-Power Down mode.",
#endif
    QEGPHY__STATUS_ENERGY_DET_APD_FIELD_MASK,
    0,
    QEGPHY__STATUS_ENERGY_DET_APD_FIELD_WIDTH,
    QEGPHY__STATUS_ENERGY_DET_APD_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: QEGPHY__STATUS_PLL_LOCK
 ******************************************************************************/
const ru_field_rec QEGPHY__STATUS_PLL_LOCK_FIELD =
{
    "PLL_LOCK",
#if RU_INCLUDE_DESC
    "PLL_LOCK",
    "When 1b1 indicates that Quad EGPHY DLL is locked.",
#endif
    QEGPHY__STATUS_PLL_LOCK_FIELD_MASK,
    0,
    QEGPHY__STATUS_PLL_LOCK_FIELD_WIDTH,
    QEGPHY__STATUS_PLL_LOCK_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: QEGPHY__STATUS_RECOVERED_CLK_LOCK
 ******************************************************************************/
const ru_field_rec QEGPHY__STATUS_RECOVERED_CLK_LOCK_FIELD =
{
    "RECOVERED_CLK_LOCK",
#if RU_INCLUDE_DESC
    "RECOVERED_CLK_LOCK",
    "When 1b1 indicates that recovered clock is locked.",
#endif
    QEGPHY__STATUS_RECOVERED_CLK_LOCK_FIELD_MASK,
    0,
    QEGPHY__STATUS_RECOVERED_CLK_LOCK_FIELD_WIDTH,
    QEGPHY__STATUS_RECOVERED_CLK_LOCK_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: QEGPHY__STATUS_GPHY_TEST_STATUS
 ******************************************************************************/
const ru_field_rec QEGPHY__STATUS_GPHY_TEST_STATUS_FIELD =
{
    "GPHY_TEST_STATUS",
#if RU_INCLUDE_DESC
    "GPHY_TEST_STATUS",
    "EGPHY test status. When 1b1 indicates that EGPHY is in test mode."
    "",
#endif
    QEGPHY__STATUS_GPHY_TEST_STATUS_FIELD_MASK,
    0,
    QEGPHY__STATUS_GPHY_TEST_STATUS_FIELD_WIDTH,
    QEGPHY__STATUS_GPHY_TEST_STATUS_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: QEGPHY__STATUS_RESERVED0
 ******************************************************************************/
const ru_field_rec QEGPHY__STATUS_RESERVED0_FIELD =
{
    "RESERVED0",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    QEGPHY__STATUS_RESERVED0_FIELD_MASK,
    0,
    QEGPHY__STATUS_RESERVED0_FIELD_WIDTH,
    QEGPHY__STATUS_RESERVED0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

#endif /* RU_INCLUDE_FIELD_DB */

/******************************************************************************
 * Register: QEGPHY__REVISION
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *QEGPHY__REVISION_FIELDS[] =
{
    &QEGPHY__REVISION_REVISION_FIELD,
    &QEGPHY__REVISION_RESERVED0_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec QEGPHY__REVISION_REG = 
{
    "_REVISION",
#if RU_INCLUDE_DESC
    "QEGPHY_REVISION Register",
    "Revision control register",
#endif
    QEGPHY__REVISION_REG_OFFSET,
    0,
    0,
    38,
#if RU_INCLUDE_ACCESS
    ru_access_rw,
#endif
#if RU_INCLUDE_FIELD_DB
    2,
    QEGPHY__REVISION_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Register: QEGPHY_TEST_CNTRL
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *QEGPHY_TEST_CNTRL_FIELDS[] =
{
    &QEGPHY_TEST_CNTRL_PHY_TEST_EN_FIELD,
    &QEGPHY_TEST_CNTRL_RESERVED0_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec QEGPHY_TEST_CNTRL_REG = 
{
    "TEST_CNTRL",
#if RU_INCLUDE_DESC
    "TEST_CNTRL Register",
    "Test control register",
#endif
    QEGPHY_TEST_CNTRL_REG_OFFSET,
    0,
    0,
    39,
#if RU_INCLUDE_ACCESS
    ru_access_rw,
#endif
#if RU_INCLUDE_FIELD_DB
    2,
    QEGPHY_TEST_CNTRL_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Register: QEGPHY__CNTRL
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *QEGPHY__CNTRL_FIELDS[] =
{
    &QEGPHY__CNTRL_IDDQ_BIAS_FIELD,
    &QEGPHY__CNTRL_EXT_PWR_DOWN_FIELD,
    &QEGPHY__CNTRL_FORCE_DLL_EN_FIELD,
    &QEGPHY__CNTRL_IDDQ_GLOBAL_PWR_FIELD,
    &QEGPHY__CNTRL_CK25_DIS_FIELD,
    &QEGPHY__CNTRL_PHY_RESET_FIELD,
    &QEGPHY__CNTRL_RESERVED0_FIELD,
    &QEGPHY__CNTRL_PHY_PHYAD_FIELD,
    &QEGPHY__CNTRL_PLL_REFCLK_SEL_FIELD,
    &QEGPHY__CNTRL_PLL_SEL_DIV5_FIELD,
    &QEGPHY__CNTRL_PLL_CLK125_250_SEL_FIELD,
    &QEGPHY__CNTRL_RESERVED1_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec QEGPHY__CNTRL_REG = 
{
    "_CNTRL",
#if RU_INCLUDE_DESC
    "QEGPHY_CNTRL Register",
    "Control register",
#endif
    QEGPHY__CNTRL_REG_OFFSET,
    0,
    0,
    40,
#if RU_INCLUDE_ACCESS
    ru_access_rw,
#endif
#if RU_INCLUDE_FIELD_DB
    12,
    QEGPHY__CNTRL_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Register: QEGPHY__STATUS
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *QEGPHY__STATUS_FIELDS[] =
{
    &QEGPHY__STATUS_ENERGY_DET_MASKED_FIELD,
    &QEGPHY__STATUS_ENERGY_DET_APD_FIELD,
    &QEGPHY__STATUS_PLL_LOCK_FIELD,
    &QEGPHY__STATUS_RECOVERED_CLK_LOCK_FIELD,
    &QEGPHY__STATUS_GPHY_TEST_STATUS_FIELD,
    &QEGPHY__STATUS_RESERVED0_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec QEGPHY__STATUS_REG = 
{
    "_STATUS",
#if RU_INCLUDE_DESC
    "QEGPHY_STATUS Register",
    "Status register",
#endif
    QEGPHY__STATUS_REG_OFFSET,
    0,
    0,
    41,
#if RU_INCLUDE_ACCESS
    ru_access_read,
#endif
#if RU_INCLUDE_FIELD_DB
    6,
    QEGPHY__STATUS_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Block: QEGPHY
 ******************************************************************************/
static const ru_reg_rec *QEGPHY_REGS[] =
{
    &QEGPHY__REVISION_REG,
    &QEGPHY_TEST_CNTRL_REG,
    &QEGPHY__CNTRL_REG,
    &QEGPHY__STATUS_REG,
};

unsigned long QEGPHY_ADDRS[] =
{
    0x82db2200,
};

const ru_block_rec QEGPHY_BLOCK = 
{
    "QEGPHY",
    QEGPHY_ADDRS,
    1,
    4,
    QEGPHY_REGS
};

/* End of file WAN_TOP_QEGPHY.c */
