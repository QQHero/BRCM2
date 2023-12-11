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
 * Field: SGMII__REV_SERDES_REV
 ******************************************************************************/
const ru_field_rec SGMII__REV_SERDES_REV_FIELD =
{
    "SERDES_REV",
#if RU_INCLUDE_DESC
    "SERDES_REV",
    "Single Serdes Revision Control Register",
#endif
    SGMII__REV_SERDES_REV_FIELD_MASK,
    0,
    SGMII__REV_SERDES_REV_FIELD_WIDTH,
    SGMII__REV_SERDES_REV_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__REV_RESERVED0
 ******************************************************************************/
const ru_field_rec SGMII__REV_RESERVED0_FIELD =
{
    "RESERVED0",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    SGMII__REV_RESERVED0_FIELD_MASK,
    0,
    SGMII__REV_RESERVED0_FIELD_WIDTH,
    SGMII__REV_RESERVED0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_IDDQ
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_IDDQ_FIELD =
{
    "IDDQ",
#if RU_INCLUDE_DESC
    "IDDQ",
    "IDDQ Enable. Powers down SERDES analog front end and turn off all clocks. MDIO is not operational",
#endif
    SGMII__CTRL_IDDQ_FIELD_MASK,
    0,
    SGMII__CTRL_IDDQ_FIELD_WIDTH,
    SGMII__CTRL_IDDQ_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_PWRDWN
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_PWRDWN_FIELD =
{
    "PWRDWN",
#if RU_INCLUDE_DESC
    "PWRDWN",
    "Power Down Enable. Powers down SERDES analog front end and turn off all clocks except the reference clock. MDIO is operational",
#endif
    SGMII__CTRL_PWRDWN_FIELD_MASK,
    0,
    SGMII__CTRL_PWRDWN_FIELD_WIDTH,
    SGMII__CTRL_PWRDWN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_RESERVED0
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_RESERVED0_FIELD =
{
    "RESERVED0",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    SGMII__CTRL_RESERVED0_FIELD_MASK,
    0,
    SGMII__CTRL_RESERVED0_FIELD_WIDTH,
    SGMII__CTRL_RESERVED0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_RESET_PLL
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_RESET_PLL_FIELD =
{
    "RESET_PLL",
#if RU_INCLUDE_DESC
    "RESET_PLL",
    "Active high PLL reset. Resets PLL and SERDES digital logic. Must be held high for at least 1ms after IDDQ or PWRDWN are de-asserted",
#endif
    SGMII__CTRL_RESET_PLL_FIELD_MASK,
    0,
    SGMII__CTRL_RESET_PLL_FIELD_WIDTH,
    SGMII__CTRL_RESET_PLL_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_RESET_MDIOREGS
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_RESET_MDIOREGS_FIELD =
{
    "RESET_MDIOREGS",
#if RU_INCLUDE_DESC
    "RESET_MDIOREGS",
    "Active high single MDIO register reset. Must be held high for at least 1ms after IDDQ or PWRDWN are de-asserted",
#endif
    SGMII__CTRL_RESET_MDIOREGS_FIELD_MASK,
    0,
    SGMII__CTRL_RESET_MDIOREGS_FIELD_WIDTH,
    SGMII__CTRL_RESET_MDIOREGS_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_SERDES_RESET
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_SERDES_RESET_FIELD =
{
    "SERDES_RESET",
#if RU_INCLUDE_DESC
    "SERDES_RESET",
    "Active high single SERDES system reset. Must be held high for at least 1ms after IDDQ or PWRDWN are de-asserted",
#endif
    SGMII__CTRL_SERDES_RESET_FIELD_MASK,
    0,
    SGMII__CTRL_SERDES_RESET_FIELD_WIDTH,
    SGMII__CTRL_SERDES_RESET_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_RESERVED1
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_RESERVED1_FIELD =
{
    "RESERVED1",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    SGMII__CTRL_RESERVED1_FIELD_MASK,
    0,
    SGMII__CTRL_RESERVED1_FIELD_WIDTH,
    SGMII__CTRL_RESERVED1_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_SERDES_PRTAD
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_SERDES_PRTAD_FIELD =
{
    "SERDES_PRTAD",
#if RU_INCLUDE_DESC
    "SERDES_PRTAD",
    "Single SERDES PHY address for Clause 22, port address for Clause 45",
#endif
    SGMII__CTRL_SERDES_PRTAD_FIELD_MASK,
    0,
    SGMII__CTRL_SERDES_PRTAD_FIELD_WIDTH,
    SGMII__CTRL_SERDES_PRTAD_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_SERDES_DEVAD
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_SERDES_DEVAD_FIELD =
{
    "SERDES_DEVAD",
#if RU_INCLUDE_DESC
    "SERDES_DEVAD",
    "Single SERDES device address for Clause 45",
#endif
    SGMII__CTRL_SERDES_DEVAD_FIELD_MASK,
    0,
    SGMII__CTRL_SERDES_DEVAD_FIELD_WIDTH,
    SGMII__CTRL_SERDES_DEVAD_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_MDIO_ST
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_MDIO_ST_FIELD =
{
    "MDIO_ST",
#if RU_INCLUDE_DESC
    "MDIO_ST",
    "Single SERDES MDIO Clause. 1b1 - Clause 22: 1b0 - Clause 45; Note: SERDES MDIO clause setting must match MDIO master MDIO clause setting",
#endif
    SGMII__CTRL_MDIO_ST_FIELD_MASK,
    0,
    SGMII__CTRL_MDIO_ST_FIELD_WIDTH,
    SGMII__CTRL_MDIO_ST_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_SERDES_TEST_EN
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_SERDES_TEST_EN_FIELD =
{
    "SERDES_TEST_EN",
#if RU_INCLUDE_DESC
    "SERDES_TEST_EN",
    "When set single SERDES MDIO is controlled by a MDIO master connected to chip pins. Debug only function",
#endif
    SGMII__CTRL_SERDES_TEST_EN_FIELD_MASK,
    0,
    SGMII__CTRL_SERDES_TEST_EN_FIELD_WIDTH,
    SGMII__CTRL_SERDES_TEST_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_LINK_DOWN_TX_DIS
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_LINK_DOWN_TX_DIS_FIELD =
{
    "LINK_DOWN_TX_DIS",
#if RU_INCLUDE_DESC
    "LINK_DOWN_TX_DIS",
    "When set HW will automatically disable (mask) serdes input GMII Tx interface when the link is down.  When cleared, GMII TX interface is always enabled.",
#endif
    SGMII__CTRL_LINK_DOWN_TX_DIS_FIELD_MASK,
    0,
    SGMII__CTRL_LINK_DOWN_TX_DIS_FIELD_WIDTH,
    SGMII__CTRL_LINK_DOWN_TX_DIS_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_SERDES_PRTAD_BCST
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_SERDES_PRTAD_BCST_FIELD =
{
    "SERDES_PRTAD_BCST",
#if RU_INCLUDE_DESC
    "SERDES_PRTAD_BCST",
    "Single SERDES PHY broadcast address for Clause 22, port address for Clause 45",
#endif
    SGMII__CTRL_SERDES_PRTAD_BCST_FIELD_MASK,
    0,
    SGMII__CTRL_SERDES_PRTAD_BCST_FIELD_WIDTH,
    SGMII__CTRL_SERDES_PRTAD_BCST_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_SERDES_REFCLK_SEL
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_SERDES_REFCLK_SEL_FIELD =
{
    "SERDES_REFCLK_SEL",
#if RU_INCLUDE_DESC
    "SERDES_REFCLK_SEL",
    "SGMII Serdes ref clock select. Encoded as:"
    "3b000 - Internal CML refclk"
    "3b001 - External XTAL"
    "3b010 - XTAL bypass/External refclk"
    "3b011 - XTAL bypass/External refclk with 100 Ohm termination"
    "1XX - Internal single-ended CMOS refclk",
#endif
    SGMII__CTRL_SERDES_REFCLK_SEL_FIELD_MASK,
    0,
    SGMII__CTRL_SERDES_REFCLK_SEL_FIELD_WIDTH,
    SGMII__CTRL_SERDES_REFCLK_SEL_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__CTRL_RESERVED2
 ******************************************************************************/
const ru_field_rec SGMII__CTRL_RESERVED2_FIELD =
{
    "RESERVED2",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    SGMII__CTRL_RESERVED2_FIELD_MASK,
    0,
    SGMII__CTRL_RESERVED2_FIELD_WIDTH,
    SGMII__CTRL_RESERVED2_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_LINK_STATUS
 ******************************************************************************/
const ru_field_rec SGMII__STAT_LINK_STATUS_FIELD =
{
    "LINK_STATUS",
#if RU_INCLUDE_DESC
    "LINK_STATUS",
    "Link Status. When 1b1 indicates that link is up",
#endif
    SGMII__STAT_LINK_STATUS_FIELD_MASK,
    0,
    SGMII__STAT_LINK_STATUS_FIELD_WIDTH,
    SGMII__STAT_LINK_STATUS_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_RX_SIGDET
 ******************************************************************************/
const ru_field_rec SGMII__STAT_RX_SIGDET_FIELD =
{
    "RX_SIGDET",
#if RU_INCLUDE_DESC
    "RX_SIGDET",
    "Rx Signal Detect. When 1b1 indicates presence of the signal on Rx pins",
#endif
    SGMII__STAT_RX_SIGDET_FIELD_MASK,
    0,
    SGMII__STAT_RX_SIGDET_FIELD_WIDTH,
    SGMII__STAT_RX_SIGDET_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_RXSEQDONE1G
 ******************************************************************************/
const ru_field_rec SGMII__STAT_RXSEQDONE1G_FIELD =
{
    "RXSEQDONE1G",
#if RU_INCLUDE_DESC
    "RXSEQDONE1G",
    "When 1b1 indicates that bit alignment is achieved",
#endif
    SGMII__STAT_RXSEQDONE1G_FIELD_MASK,
    0,
    SGMII__STAT_RXSEQDONE1G_FIELD_WIDTH,
    SGMII__STAT_RXSEQDONE1G_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_SGMII
 ******************************************************************************/
const ru_field_rec SGMII__STAT_SGMII_FIELD =
{
    "SGMII",
#if RU_INCLUDE_DESC
    "SGMII",
    "SGMII Mode",
#endif
    SGMII__STAT_SGMII_FIELD_MASK,
    0,
    SGMII__STAT_SGMII_FIELD_WIDTH,
    SGMII__STAT_SGMII_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_SYNC_STATUS
 ******************************************************************************/
const ru_field_rec SGMII__STAT_SYNC_STATUS_FIELD =
{
    "SYNC_STATUS",
#if RU_INCLUDE_DESC
    "SYNC_STATUS",
    "When 1b1 indicates that SERDES achieved symbol alignment",
#endif
    SGMII__STAT_SYNC_STATUS_FIELD_MASK,
    0,
    SGMII__STAT_SYNC_STATUS_FIELD_WIDTH,
    SGMII__STAT_SYNC_STATUS_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_PLL_LOCK
 ******************************************************************************/
const ru_field_rec SGMII__STAT_PLL_LOCK_FIELD =
{
    "PLL_LOCK",
#if RU_INCLUDE_DESC
    "PLL_LOCK",
    "When 1b1 indicates that single SERDES PLL is locked",
#endif
    SGMII__STAT_PLL_LOCK_FIELD_MASK,
    0,
    SGMII__STAT_PLL_LOCK_FIELD_WIDTH,
    SGMII__STAT_PLL_LOCK_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_DEB_SIG_DETECT
 ******************************************************************************/
const ru_field_rec SGMII__STAT_DEB_SIG_DETECT_FIELD =
{
    "DEB_SIG_DETECT",
#if RU_INCLUDE_DESC
    "DEB_SIG_DETECT",
    "Filtered external or internal signal detect. Selection is made via Serdes APD Control Register.",
#endif
    SGMII__STAT_DEB_SIG_DETECT_FIELD_MASK,
    0,
    SGMII__STAT_DEB_SIG_DETECT_FIELD_WIDTH,
    SGMII__STAT_DEB_SIG_DETECT_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_APD_STATE
 ******************************************************************************/
const ru_field_rec SGMII__STAT_APD_STATE_FIELD =
{
    "APD_STATE",
#if RU_INCLUDE_DESC
    "APD_STATE",
    "APD state machine state. State encoding is as following:"
    "3b000 -- IDLE"
    "3b001 -- SD_OFF"
    "3b010 -- RST_ON"
    "3b011 -- RST_OFF"
    "3b100 -- SD_ON"
    ""
    "Debug only."
    "",
#endif
    SGMII__STAT_APD_STATE_FIELD_MASK,
    0,
    SGMII__STAT_APD_STATE_FIELD_WIDTH,
    SGMII__STAT_APD_STATE_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_EXT_SIGDET
 ******************************************************************************/
const ru_field_rec SGMII__STAT_EXT_SIGDET_FIELD =
{
    "EXT_SIGDET",
#if RU_INCLUDE_DESC
    "EXT_SIGDET",
    "Non-filtered signal detect from the pin that is optical transceiver. Filtering must be done by SW."
    "Note: Please consult optical transceiver datasheet for the signal polarity.",
#endif
    SGMII__STAT_EXT_SIGDET_FIELD_MASK,
    0,
    SGMII__STAT_EXT_SIGDET_FIELD_WIDTH,
    SGMII__STAT_EXT_SIGDET_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_MOD_DEF0
 ******************************************************************************/
const ru_field_rec SGMII__STAT_MOD_DEF0_FIELD =
{
    "MOD_DEF0",
#if RU_INCLUDE_DESC
    "MOD_DEF0",
    "When 1b1 indicates presence of the optical module.",
#endif
    SGMII__STAT_MOD_DEF0_FIELD_MASK,
    0,
    SGMII__STAT_MOD_DEF0_FIELD_WIDTH,
    SGMII__STAT_MOD_DEF0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__STAT_RESERVED0
 ******************************************************************************/
const ru_field_rec SGMII__STAT_RESERVED0_FIELD =
{
    "RESERVED0",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    SGMII__STAT_RESERVED0_FIELD_MASK,
    0,
    SGMII__STAT_RESERVED0_FIELD_WIDTH,
    SGMII__STAT_RESERVED0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_read
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_APD_EN
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_APD_EN_FIELD =
{
    "APD_EN",
#if RU_INCLUDE_DESC
    "APD_EN",
    "Auto-Power-Down Enable. When set serdes is powered up and down by HW state machine based on the state of the Signal Detect.",
#endif
    SGMII__APD_CTRL_APD_EN_FIELD_MASK,
    0,
    SGMII__APD_CTRL_APD_EN_FIELD_WIDTH,
    SGMII__APD_CTRL_APD_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_APD_CYCLE_EN
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_APD_CYCLE_EN_FIELD =
{
    "APD_CYCLE_EN",
#if RU_INCLUDE_DESC
    "APD_CYCLE_EN",
    "APD Power Cycle Enable. When set HW state machine will periodically power up serdes testing signal detect (in case where signal was previously lost and as a result serdes was powered down). Works in conjunction with APD_EN.",
#endif
    SGMII__APD_CTRL_APD_CYCLE_EN_FIELD_MASK,
    0,
    SGMII__APD_CTRL_APD_CYCLE_EN_FIELD_WIDTH,
    SGMII__APD_CTRL_APD_CYCLE_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_SD_SEL
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_SD_SEL_FIELD =
{
    "SD_SEL",
#if RU_INCLUDE_DESC
    "SD_SEL",
    "Signal Detect Select.  0  External Signal Detect (from the chip pin) is used 1  Internal Signal detect (from the on-chip serdes is used).  Note: Internal Signal Detect should be used ONLY when APD_CYCLE_EN = 1.",
#endif
    SGMII__APD_CTRL_SD_SEL_FIELD_MASK,
    0,
    SGMII__APD_CTRL_SD_SEL_FIELD_WIDTH,
    SGMII__APD_CTRL_SD_SEL_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_INV_SD
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_INV_SD_FIELD =
{
    "INV_SD",
#if RU_INCLUDE_DESC
    "INV_SD",
    "When set, Signal Detect is inverted for HW use.  Useful in case of external fiber modules that provide Loss of Signal instead of the Signal Detect.",
#endif
    SGMII__APD_CTRL_INV_SD_FIELD_MASK,
    0,
    SGMII__APD_CTRL_INV_SD_FIELD_WIDTH,
    SGMII__APD_CTRL_INV_SD_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_PWRDWN_EN
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_PWRDWN_EN_FIELD =
{
    "PWRDWN_EN",
#if RU_INCLUDE_DESC
    "PWRDWN_EN",
    "When this bit is set APD state machine asserts PWRDWN signal when the signal is lost.",
#endif
    SGMII__APD_CTRL_PWRDWN_EN_FIELD_MASK,
    0,
    SGMII__APD_CTRL_PWRDWN_EN_FIELD_WIDTH,
    SGMII__APD_CTRL_PWRDWN_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_IDDQ_EN
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_IDDQ_EN_FIELD =
{
    "IDDQ_EN",
#if RU_INCLUDE_DESC
    "IDDQ_EN",
    "When this bit is set APD state machine asserts IDDQ signal when the signal is lost.",
#endif
    SGMII__APD_CTRL_IDDQ_EN_FIELD_MASK,
    0,
    SGMII__APD_CTRL_IDDQ_EN_FIELD_WIDTH,
    SGMII__APD_CTRL_IDDQ_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_RSTB_PLL_EN
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_RSTB_PLL_EN_FIELD =
{
    "RSTB_PLL_EN",
#if RU_INCLUDE_DESC
    "RSTB_PLL_EN",
    "When this bit is set APD state machine asserts RSTB_PLL signal when the signal is lost.",
#endif
    SGMII__APD_CTRL_RSTB_PLL_EN_FIELD_MASK,
    0,
    SGMII__APD_CTRL_RSTB_PLL_EN_FIELD_WIDTH,
    SGMII__APD_CTRL_RSTB_PLL_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_RSTB_MDIOREGS_EN
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_RSTB_MDIOREGS_EN_FIELD =
{
    "RSTB_MDIOREGS_EN",
#if RU_INCLUDE_DESC
    "RSTB_MDIOREGS_EN",
    "When this bit is set APD state machine asserts RSTB_MDIOREGS signal when the signal is lost.",
#endif
    SGMII__APD_CTRL_RSTB_MDIOREGS_EN_FIELD_MASK,
    0,
    SGMII__APD_CTRL_RSTB_MDIOREGS_EN_FIELD_WIDTH,
    SGMII__APD_CTRL_RSTB_MDIOREGS_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_RSTB_HW_EN
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_RSTB_HW_EN_FIELD =
{
    "RSTB_HW_EN",
#if RU_INCLUDE_DESC
    "RSTB_HW_EN",
    "When this bit is set APD state machine asserts RSTB_HW signal when the signal is lost.",
#endif
    SGMII__APD_CTRL_RSTB_HW_EN_FIELD_MASK,
    0,
    SGMII__APD_CTRL_RSTB_HW_EN_FIELD_WIDTH,
    SGMII__APD_CTRL_RSTB_HW_EN_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_SD_INTR_SRC_SEL
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_SD_INTR_SRC_SEL_FIELD =
{
    "SD_INTR_SRC_SEL",
#if RU_INCLUDE_DESC
    "SD_INTR_SRC_SEL",
    "Selects signal detect used to generate interrupt. 0:  debounced signal detect is used ; 1:  non-filtered signal detect is used.",
#endif
    SGMII__APD_CTRL_SD_INTR_SRC_SEL_FIELD_MASK,
    0,
    SGMII__APD_CTRL_SD_INTR_SRC_SEL_FIELD_WIDTH,
    SGMII__APD_CTRL_SD_INTR_SRC_SEL_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_RESERVED0
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_RESERVED0_FIELD =
{
    "RESERVED0",
#if RU_INCLUDE_DESC
    "",
    "",
#endif
    SGMII__APD_CTRL_RESERVED0_FIELD_MASK,
    0,
    SGMII__APD_CTRL_RESERVED0_FIELD_WIDTH,
    SGMII__APD_CTRL_RESERVED0_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_CTRL_DEBOUNCE_TIME
 ******************************************************************************/
const ru_field_rec SGMII__APD_CTRL_DEBOUNCE_TIME_FIELD =
{
    "DEBOUNCE_TIME",
#if RU_INCLUDE_DESC
    "DEBOUNCE_TIME",
    "De-bounce time specified in 1ms units.  Signal Detect on-off and off-on transitions are de-bounced by a time specified in this registers.",
#endif
    SGMII__APD_CTRL_DEBOUNCE_TIME_FIELD_MASK,
    0,
    SGMII__APD_CTRL_DEBOUNCE_TIME_FIELD_WIDTH,
    SGMII__APD_CTRL_DEBOUNCE_TIME_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_FSM_CTRL_SD_OFF_TIME
 ******************************************************************************/
const ru_field_rec SGMII__APD_FSM_CTRL_SD_OFF_TIME_FIELD =
{
    "SD_OFF_TIME",
#if RU_INCLUDE_DESC
    "SD_OFF_TIME",
    "Signal Detect OFF Timeout specified in 1ms units.  When APD_CYCLE_EN=1, APD state machine will keep serdes powered down for a time specified in this register before it powers it up and try to detect presence of the signal on the wire/fiber.",
#endif
    SGMII__APD_FSM_CTRL_SD_OFF_TIME_FIELD_MASK,
    0,
    SGMII__APD_FSM_CTRL_SD_OFF_TIME_FIELD_WIDTH,
    SGMII__APD_FSM_CTRL_SD_OFF_TIME_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

/******************************************************************************
 * Field: SGMII__APD_FSM_CTRL_SD_ON_TIME
 ******************************************************************************/
const ru_field_rec SGMII__APD_FSM_CTRL_SD_ON_TIME_FIELD =
{
    "SD_ON_TIME",
#if RU_INCLUDE_DESC
    "SD_ON_TIME",
    "Signal Detect ON Timeout specified in 1ms units.  When APD_CYCLE_EN=1, APD state machine will keep serdes powered up for a time specified in this register before it eventually powers it down (in case where the signal is not detected on the wire/fiber).",
#endif
    SGMII__APD_FSM_CTRL_SD_ON_TIME_FIELD_MASK,
    0,
    SGMII__APD_FSM_CTRL_SD_ON_TIME_FIELD_WIDTH,
    SGMII__APD_FSM_CTRL_SD_ON_TIME_FIELD_SHIFT,
#if RU_INCLUDE_ACCESS
    ru_access_rw
#endif
};

#endif /* RU_INCLUDE_FIELD_DB */

/******************************************************************************
 * Register: SGMII__REV
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *SGMII__REV_FIELDS[] =
{
    &SGMII__REV_SERDES_REV_FIELD,
    &SGMII__REV_RESERVED0_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec SGMII__REV_REG = 
{
    "_REV",
#if RU_INCLUDE_DESC
    "SGMII_REV Register",
    "Serdes Revision Control Register",
#endif
    SGMII__REV_REG_OFFSET,
    0,
    0,
    50,
#if RU_INCLUDE_ACCESS
    ru_access_read,
#endif
#if RU_INCLUDE_FIELD_DB
    2,
    SGMII__REV_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Register: SGMII__CTRL
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *SGMII__CTRL_FIELDS[] =
{
    &SGMII__CTRL_IDDQ_FIELD,
    &SGMII__CTRL_PWRDWN_FIELD,
    &SGMII__CTRL_RESERVED0_FIELD,
    &SGMII__CTRL_RESET_PLL_FIELD,
    &SGMII__CTRL_RESET_MDIOREGS_FIELD,
    &SGMII__CTRL_SERDES_RESET_FIELD,
    &SGMII__CTRL_RESERVED1_FIELD,
    &SGMII__CTRL_SERDES_PRTAD_FIELD,
    &SGMII__CTRL_SERDES_DEVAD_FIELD,
    &SGMII__CTRL_MDIO_ST_FIELD,
    &SGMII__CTRL_SERDES_TEST_EN_FIELD,
    &SGMII__CTRL_LINK_DOWN_TX_DIS_FIELD,
    &SGMII__CTRL_SERDES_PRTAD_BCST_FIELD,
    &SGMII__CTRL_SERDES_REFCLK_SEL_FIELD,
    &SGMII__CTRL_RESERVED2_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec SGMII__CTRL_REG = 
{
    "_CTRL",
#if RU_INCLUDE_DESC
    "SGMII_CTRL Register",
    "Single Serdes Control Register",
#endif
    SGMII__CTRL_REG_OFFSET,
    0,
    0,
    51,
#if RU_INCLUDE_ACCESS
    ru_access_rw,
#endif
#if RU_INCLUDE_FIELD_DB
    15,
    SGMII__CTRL_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Register: SGMII__STAT
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *SGMII__STAT_FIELDS[] =
{
    &SGMII__STAT_LINK_STATUS_FIELD,
    &SGMII__STAT_RX_SIGDET_FIELD,
    &SGMII__STAT_RXSEQDONE1G_FIELD,
    &SGMII__STAT_SGMII_FIELD,
    &SGMII__STAT_SYNC_STATUS_FIELD,
    &SGMII__STAT_PLL_LOCK_FIELD,
    &SGMII__STAT_DEB_SIG_DETECT_FIELD,
    &SGMII__STAT_APD_STATE_FIELD,
    &SGMII__STAT_EXT_SIGDET_FIELD,
    &SGMII__STAT_MOD_DEF0_FIELD,
    &SGMII__STAT_RESERVED0_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec SGMII__STAT_REG = 
{
    "_STAT",
#if RU_INCLUDE_DESC
    "SGMII_STAT Register",
    "Single Serdes Status Register",
#endif
    SGMII__STAT_REG_OFFSET,
    0,
    0,
    52,
#if RU_INCLUDE_ACCESS
    ru_access_read,
#endif
#if RU_INCLUDE_FIELD_DB
    11,
    SGMII__STAT_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Register: SGMII__APD_CTRL
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *SGMII__APD_CTRL_FIELDS[] =
{
    &SGMII__APD_CTRL_APD_EN_FIELD,
    &SGMII__APD_CTRL_APD_CYCLE_EN_FIELD,
    &SGMII__APD_CTRL_SD_SEL_FIELD,
    &SGMII__APD_CTRL_INV_SD_FIELD,
    &SGMII__APD_CTRL_PWRDWN_EN_FIELD,
    &SGMII__APD_CTRL_IDDQ_EN_FIELD,
    &SGMII__APD_CTRL_RSTB_PLL_EN_FIELD,
    &SGMII__APD_CTRL_RSTB_MDIOREGS_EN_FIELD,
    &SGMII__APD_CTRL_RSTB_HW_EN_FIELD,
    &SGMII__APD_CTRL_SD_INTR_SRC_SEL_FIELD,
    &SGMII__APD_CTRL_RESERVED0_FIELD,
    &SGMII__APD_CTRL_DEBOUNCE_TIME_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec SGMII__APD_CTRL_REG = 
{
    "_APD_CTRL",
#if RU_INCLUDE_DESC
    "SGMII_APD_CTRL Register",
    "Single Serdes APD Control Register",
#endif
    SGMII__APD_CTRL_REG_OFFSET,
    0,
    0,
    53,
#if RU_INCLUDE_ACCESS
    ru_access_rw,
#endif
#if RU_INCLUDE_FIELD_DB
    12,
    SGMII__APD_CTRL_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Register: SGMII__APD_FSM_CTRL
 ******************************************************************************/
#if RU_INCLUDE_FIELD_DB
static const ru_field_rec *SGMII__APD_FSM_CTRL_FIELDS[] =
{
    &SGMII__APD_FSM_CTRL_SD_OFF_TIME_FIELD,
    &SGMII__APD_FSM_CTRL_SD_ON_TIME_FIELD,
};

#endif /* RU_INCLUDE_FIELD_DB */

const ru_reg_rec SGMII__APD_FSM_CTRL_REG = 
{
    "_APD_FSM_CTRL",
#if RU_INCLUDE_DESC
    "SGMII_APD_FSM_CTRL Register",
    "Single Serdes APD FSM Control Register",
#endif
    SGMII__APD_FSM_CTRL_REG_OFFSET,
    0,
    0,
    54,
#if RU_INCLUDE_ACCESS
    ru_access_rw,
#endif
#if RU_INCLUDE_FIELD_DB
    2,
    SGMII__APD_FSM_CTRL_FIELDS
#endif /* RU_INCLUDE_FIELD_DB */
};

/******************************************************************************
 * Block: SGMII
 ******************************************************************************/
static const ru_reg_rec *SGMII_REGS[] =
{
    &SGMII__REV_REG,
    &SGMII__CTRL_REG,
    &SGMII__STAT_REG,
    &SGMII__APD_CTRL_REG,
    &SGMII__APD_FSM_CTRL_REG,
};

unsigned long SGMII_ADDRS[] =
{
    (PON_DRV_BASE+0x280),
};

const ru_block_rec SGMII_BLOCK = 
{
    "SGMII",
    SGMII_ADDRS,
    1,
    5,
    SGMII_REGS
};

/* End of file WAN_TOP_SGMII.c */
