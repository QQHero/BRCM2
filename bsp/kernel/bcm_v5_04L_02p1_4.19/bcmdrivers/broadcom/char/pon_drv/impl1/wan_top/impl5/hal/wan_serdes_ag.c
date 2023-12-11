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

#include "rdp_common.h"
#include "drivers_common_ag.h"
#include "wan_serdes_ag.h"

bdmf_error_t ag_drv_wan_serdes_wan_serdes_pll_ctl_set(const wan_serdes_wan_serdes_pll_ctl *wan_serdes_pll_ctl)
{
    uint32_t reg_pll_ctl=0;

#ifdef VALIDATE_PARMS
    if(!wan_serdes_pll_ctl)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((wan_serdes_pll_ctl->cfg_pll0_refin_en >= _1BITS_MAX_VAL_) ||
       (wan_serdes_pll_ctl->cfg_pll0_refout_en >= _1BITS_MAX_VAL_) ||
       (wan_serdes_pll_ctl->cfg_pll0_lcref_sel >= _1BITS_MAX_VAL_) ||
       (wan_serdes_pll_ctl->cfg_pll1_refin_en >= _1BITS_MAX_VAL_) ||
       (wan_serdes_pll_ctl->cfg_pll1_refout_en >= _1BITS_MAX_VAL_) ||
       (wan_serdes_pll_ctl->cfg_pll1_lcref_sel >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_pll_ctl = RU_FIELD_SET(0, WAN_SERDES, PLL_CTL, CFG_PLL0_REFIN_EN, reg_pll_ctl, wan_serdes_pll_ctl->cfg_pll0_refin_en);
    reg_pll_ctl = RU_FIELD_SET(0, WAN_SERDES, PLL_CTL, CFG_PLL0_REFOUT_EN, reg_pll_ctl, wan_serdes_pll_ctl->cfg_pll0_refout_en);
    reg_pll_ctl = RU_FIELD_SET(0, WAN_SERDES, PLL_CTL, CFG_PLL0_LCREF_SEL, reg_pll_ctl, wan_serdes_pll_ctl->cfg_pll0_lcref_sel);
    reg_pll_ctl = RU_FIELD_SET(0, WAN_SERDES, PLL_CTL, CFG_PLL1_REFIN_EN, reg_pll_ctl, wan_serdes_pll_ctl->cfg_pll1_refin_en);
    reg_pll_ctl = RU_FIELD_SET(0, WAN_SERDES, PLL_CTL, CFG_PLL1_REFOUT_EN, reg_pll_ctl, wan_serdes_pll_ctl->cfg_pll1_refout_en);
    reg_pll_ctl = RU_FIELD_SET(0, WAN_SERDES, PLL_CTL, CFG_PLL1_LCREF_SEL, reg_pll_ctl, wan_serdes_pll_ctl->cfg_pll1_lcref_sel);

    RU_REG_WRITE(0, WAN_SERDES, PLL_CTL, reg_pll_ctl);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_wan_serdes_pll_ctl_get(wan_serdes_wan_serdes_pll_ctl *wan_serdes_pll_ctl)
{
    uint32_t reg_pll_ctl;

#ifdef VALIDATE_PARMS
    if(!wan_serdes_pll_ctl)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_SERDES, PLL_CTL, reg_pll_ctl);

    wan_serdes_pll_ctl->cfg_pll0_refin_en = RU_FIELD_GET(0, WAN_SERDES, PLL_CTL, CFG_PLL0_REFIN_EN, reg_pll_ctl);
    wan_serdes_pll_ctl->cfg_pll0_refout_en = RU_FIELD_GET(0, WAN_SERDES, PLL_CTL, CFG_PLL0_REFOUT_EN, reg_pll_ctl);
    wan_serdes_pll_ctl->cfg_pll0_lcref_sel = RU_FIELD_GET(0, WAN_SERDES, PLL_CTL, CFG_PLL0_LCREF_SEL, reg_pll_ctl);
    wan_serdes_pll_ctl->cfg_pll1_refin_en = RU_FIELD_GET(0, WAN_SERDES, PLL_CTL, CFG_PLL1_REFIN_EN, reg_pll_ctl);
    wan_serdes_pll_ctl->cfg_pll1_refout_en = RU_FIELD_GET(0, WAN_SERDES, PLL_CTL, CFG_PLL1_REFOUT_EN, reg_pll_ctl);
    wan_serdes_pll_ctl->cfg_pll1_lcref_sel = RU_FIELD_GET(0, WAN_SERDES, PLL_CTL, CFG_PLL1_LCREF_SEL, reg_pll_ctl);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_wan_serdes_temp_ctl_get(uint16_t *wan_temperature_read)
{
    uint32_t reg_temp_ctl;

#ifdef VALIDATE_PARMS
    if(!wan_temperature_read)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_SERDES, TEMP_CTL, reg_temp_ctl);

    *wan_temperature_read = RU_FIELD_GET(0, WAN_SERDES, TEMP_CTL, WAN_TEMPERATURE_READ, reg_temp_ctl);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_wan_serdes_pram_ctl_set(bdmf_boolean pram_go, bdmf_boolean pram_we, uint16_t pram_address)
{
    uint32_t reg_pram_ctl=0;

#ifdef VALIDATE_PARMS
    if((pram_we >= _1BITS_MAX_VAL_) ||
       (pram_go >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_pram_ctl = RU_FIELD_SET(0, WAN_SERDES, PRAM_CTL, PRAM_ADDRESS, reg_pram_ctl, pram_address);
    reg_pram_ctl = RU_FIELD_SET(0, WAN_SERDES, PRAM_CTL, PRAM_WE, reg_pram_ctl, pram_we);
    reg_pram_ctl = RU_FIELD_SET(0, WAN_SERDES, PRAM_CTL, PRAM_GO, reg_pram_ctl, pram_go);

    RU_REG_WRITE(0, WAN_SERDES, PRAM_CTL, reg_pram_ctl);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_wan_serdes_pram_ctl_get(bdmf_boolean *pram_go, bdmf_boolean *pram_we, uint16_t *pram_address)
{
    uint32_t reg_pram_ctl;

#ifdef VALIDATE_PARMS
    if(!pram_address || !pram_we || !pram_go)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_SERDES, PRAM_CTL, reg_pram_ctl);

    *pram_address = RU_FIELD_GET(0, WAN_SERDES, PRAM_CTL, PRAM_ADDRESS, reg_pram_ctl);
    *pram_we = RU_FIELD_GET(0, WAN_SERDES, PRAM_CTL, PRAM_WE, reg_pram_ctl);
    *pram_go = RU_FIELD_GET(0, WAN_SERDES, PRAM_CTL, PRAM_GO, reg_pram_ctl);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_wan_serdes_pram_val_low_set(uint32_t val)
{
    uint32_t reg_pram_val_low=0;

#ifdef VALIDATE_PARMS
#endif

    reg_pram_val_low = RU_FIELD_SET(0, WAN_SERDES, PRAM_VAL_LOW, VAL, reg_pram_val_low, val);

    RU_REG_WRITE(0, WAN_SERDES, PRAM_VAL_LOW, reg_pram_val_low);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_wan_serdes_pram_val_low_get(uint32_t *val)
{
    uint32_t reg_pram_val_low;

#ifdef VALIDATE_PARMS
    if(!val)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_SERDES, PRAM_VAL_LOW, reg_pram_val_low);

    *val = RU_FIELD_GET(0, WAN_SERDES, PRAM_VAL_LOW, VAL, reg_pram_val_low);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_wan_serdes_pram_val_high_set(uint32_t val)
{
    uint32_t reg_pram_val_high=0;

#ifdef VALIDATE_PARMS
#endif

    reg_pram_val_high = RU_FIELD_SET(0, WAN_SERDES, PRAM_VAL_HIGH, VAL, reg_pram_val_high, val);

    RU_REG_WRITE(0, WAN_SERDES, PRAM_VAL_HIGH, reg_pram_val_high);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_wan_serdes_pram_val_high_get(uint32_t *val)
{
    uint32_t reg_pram_val_high;

#ifdef VALIDATE_PARMS
    if(!val)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_SERDES, PRAM_VAL_HIGH, reg_pram_val_high);

    *val = RU_FIELD_GET(0, WAN_SERDES, PRAM_VAL_HIGH, VAL, reg_pram_val_high);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_top_osr_ctrl_set(const wan_serdes_top_osr_ctrl *top_osr_ctrl)
{
    uint32_t reg_cr_wan_top_wan_misc_serdes_oversample_ctrl=0;

#ifdef VALIDATE_PARMS
    if(!top_osr_ctrl)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((top_osr_ctrl->top_osr_control_cfg_gpon_rx_clk >= _2BITS_MAX_VAL_) ||
       (top_osr_ctrl->top_osr_control_txfifo_rd_legacy_mode >= _1BITS_MAX_VAL_) ||
       (top_osr_ctrl->top_osr_control_txlbe_ser_en >= _1BITS_MAX_VAL_) ||
       (top_osr_ctrl->top_osr_control_txlbe_ser_init_val >= _3BITS_MAX_VAL_) ||
       (top_osr_ctrl->top_osr_control_txlbe_ser_order >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_cr_wan_top_wan_misc_serdes_oversample_ctrl = RU_FIELD_SET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, CFG_GPON_RX_CLK, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl, top_osr_ctrl->top_osr_control_cfg_gpon_rx_clk);
    reg_cr_wan_top_wan_misc_serdes_oversample_ctrl = RU_FIELD_SET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, TXFIFO_RD_LEGACY_MODE, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl, top_osr_ctrl->top_osr_control_txfifo_rd_legacy_mode);
    reg_cr_wan_top_wan_misc_serdes_oversample_ctrl = RU_FIELD_SET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, TXLBE_SER_EN, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl, top_osr_ctrl->top_osr_control_txlbe_ser_en);
    reg_cr_wan_top_wan_misc_serdes_oversample_ctrl = RU_FIELD_SET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, TXLBE_SER_INIT_VAL, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl, top_osr_ctrl->top_osr_control_txlbe_ser_init_val);
    reg_cr_wan_top_wan_misc_serdes_oversample_ctrl = RU_FIELD_SET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, TXLBE_SER_ORDER, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl, top_osr_ctrl->top_osr_control_txlbe_ser_order);

    RU_REG_WRITE(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_serdes_top_osr_ctrl_get(wan_serdes_top_osr_ctrl *top_osr_ctrl)
{
    uint32_t reg_cr_wan_top_wan_misc_serdes_oversample_ctrl;

#ifdef VALIDATE_PARMS
    if(!top_osr_ctrl)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl);

    top_osr_ctrl->top_osr_control_cfg_gpon_rx_clk = RU_FIELD_GET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, CFG_GPON_RX_CLK, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl);
    top_osr_ctrl->top_osr_control_txfifo_rd_legacy_mode = RU_FIELD_GET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, TXFIFO_RD_LEGACY_MODE, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl);
    top_osr_ctrl->top_osr_control_txlbe_ser_en = RU_FIELD_GET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, TXLBE_SER_EN, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl);
    top_osr_ctrl->top_osr_control_txlbe_ser_init_val = RU_FIELD_GET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, TXLBE_SER_INIT_VAL, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl);
    top_osr_ctrl->top_osr_control_txlbe_ser_order = RU_FIELD_GET(0, WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL, TXLBE_SER_ORDER, reg_cr_wan_top_wan_misc_serdes_oversample_ctrl);

    return BDMF_ERR_OK;
}

#ifdef USE_BDMF_SHELL
typedef enum
{
    bdmf_address_pll_ctl,
    bdmf_address_temp_ctl,
    bdmf_address_pram_ctl,
    bdmf_address_pram_val_low,
    bdmf_address_pram_val_high,
    bdmf_address_cr_wan_top_wan_misc_serdes_oversample_ctrl,
}
bdmf_address;

static int bcm_wan_serdes_cli_set(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_wan_serdes_wan_serdes_pll_ctl:
    {
        wan_serdes_wan_serdes_pll_ctl wan_serdes_pll_ctl = { .cfg_pll0_refin_en=parm[1].value.unumber, .cfg_pll0_refout_en=parm[2].value.unumber, .cfg_pll0_lcref_sel=parm[3].value.unumber, .cfg_pll1_refin_en=parm[4].value.unumber, .cfg_pll1_refout_en=parm[5].value.unumber, .cfg_pll1_lcref_sel=parm[6].value.unumber};
        err = ag_drv_wan_serdes_wan_serdes_pll_ctl_set(&wan_serdes_pll_ctl);
        break;
    }
    case cli_wan_serdes_wan_serdes_pram_ctl:
        err = ag_drv_wan_serdes_wan_serdes_pram_ctl_set(parm[1].value.unumber, parm[2].value.unumber, parm[3].value.unumber);
        break;
    case cli_wan_serdes_wan_serdes_pram_val_low:
        err = ag_drv_wan_serdes_wan_serdes_pram_val_low_set(parm[1].value.unumber);
        break;
    case cli_wan_serdes_wan_serdes_pram_val_high:
        err = ag_drv_wan_serdes_wan_serdes_pram_val_high_set(parm[1].value.unumber);
        break;
    case cli_wan_serdes_top_osr_ctrl:
    {
        wan_serdes_top_osr_ctrl top_osr_ctrl = { .top_osr_control_cfg_gpon_rx_clk=parm[1].value.unumber, .top_osr_control_txfifo_rd_legacy_mode=parm[2].value.unumber, .top_osr_control_txlbe_ser_en=parm[3].value.unumber, .top_osr_control_txlbe_ser_init_val=parm[4].value.unumber, .top_osr_control_txlbe_ser_order=parm[5].value.unumber};
        err = ag_drv_wan_serdes_top_osr_ctrl_set(&top_osr_ctrl);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

int bcm_wan_serdes_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_wan_serdes_wan_serdes_pll_ctl:
    {
        wan_serdes_wan_serdes_pll_ctl wan_serdes_pll_ctl;
        err = ag_drv_wan_serdes_wan_serdes_pll_ctl_get(&wan_serdes_pll_ctl);
        bdmf_session_print(session, "cfg_pll0_refin_en = %u (0x%x)\n", wan_serdes_pll_ctl.cfg_pll0_refin_en, wan_serdes_pll_ctl.cfg_pll0_refin_en);
        bdmf_session_print(session, "cfg_pll0_refout_en = %u (0x%x)\n", wan_serdes_pll_ctl.cfg_pll0_refout_en, wan_serdes_pll_ctl.cfg_pll0_refout_en);
        bdmf_session_print(session, "cfg_pll0_lcref_sel = %u (0x%x)\n", wan_serdes_pll_ctl.cfg_pll0_lcref_sel, wan_serdes_pll_ctl.cfg_pll0_lcref_sel);
        bdmf_session_print(session, "cfg_pll1_refin_en = %u (0x%x)\n", wan_serdes_pll_ctl.cfg_pll1_refin_en, wan_serdes_pll_ctl.cfg_pll1_refin_en);
        bdmf_session_print(session, "cfg_pll1_refout_en = %u (0x%x)\n", wan_serdes_pll_ctl.cfg_pll1_refout_en, wan_serdes_pll_ctl.cfg_pll1_refout_en);
        bdmf_session_print(session, "cfg_pll1_lcref_sel = %u (0x%x)\n", wan_serdes_pll_ctl.cfg_pll1_lcref_sel, wan_serdes_pll_ctl.cfg_pll1_lcref_sel);
        break;
    }
    case cli_wan_serdes_wan_serdes_temp_ctl:
    {
        uint16_t wan_temperature_read;
        err = ag_drv_wan_serdes_wan_serdes_temp_ctl_get(&wan_temperature_read);
        bdmf_session_print(session, "wan_temperature_read = %u (0x%x)\n", wan_temperature_read, wan_temperature_read);
        break;
    }
    case cli_wan_serdes_wan_serdes_pram_ctl:
    {
        uint16_t pram_address;
        bdmf_boolean pram_we;
        bdmf_boolean pram_go;
        err = ag_drv_wan_serdes_wan_serdes_pram_ctl_get(&pram_address, &pram_we, &pram_go);
        bdmf_session_print(session, "pram_address = %u (0x%x)\n", pram_address, pram_address);
        bdmf_session_print(session, "pram_we = %u (0x%x)\n", pram_we, pram_we);
        bdmf_session_print(session, "pram_go = %u (0x%x)\n", pram_go, pram_go);
        break;
    }
    case cli_wan_serdes_wan_serdes_pram_val_low:
    {
        uint32_t val;
        err = ag_drv_wan_serdes_wan_serdes_pram_val_low_get(&val);
        bdmf_session_print(session, "val = %u (0x%x)\n", val, val);
        break;
    }
    case cli_wan_serdes_wan_serdes_pram_val_high:
    {
        uint32_t val;
        err = ag_drv_wan_serdes_wan_serdes_pram_val_high_get(&val);
        bdmf_session_print(session, "val = %u (0x%x)\n", val, val);
        break;
    }
    case cli_wan_serdes_top_osr_ctrl:
    {
        wan_serdes_top_osr_ctrl top_osr_ctrl;
        err = ag_drv_wan_serdes_top_osr_ctrl_get(&top_osr_ctrl);
        bdmf_session_print(session, "top_osr_control_cfg_gpon_rx_clk = %u (0x%x)\n", top_osr_ctrl.top_osr_control_cfg_gpon_rx_clk, top_osr_ctrl.top_osr_control_cfg_gpon_rx_clk);
        bdmf_session_print(session, "top_osr_control_txfifo_rd_legacy_mode = %u (0x%x)\n", top_osr_ctrl.top_osr_control_txfifo_rd_legacy_mode, top_osr_ctrl.top_osr_control_txfifo_rd_legacy_mode);
        bdmf_session_print(session, "top_osr_control_txlbe_ser_en = %u (0x%x)\n", top_osr_ctrl.top_osr_control_txlbe_ser_en, top_osr_ctrl.top_osr_control_txlbe_ser_en);
        bdmf_session_print(session, "top_osr_control_txlbe_ser_init_val = %u (0x%x)\n", top_osr_ctrl.top_osr_control_txlbe_ser_init_val, top_osr_ctrl.top_osr_control_txlbe_ser_init_val);
        bdmf_session_print(session, "top_osr_control_txlbe_ser_order = %u (0x%x)\n", top_osr_ctrl.top_osr_control_txlbe_ser_order, top_osr_ctrl.top_osr_control_txlbe_ser_order);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

static int bcm_wan_serdes_cli_test(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_test_method m = parm[0].value.unumber;
    bdmf_error_t err = BDMF_ERR_OK;

    {
        wan_serdes_wan_serdes_pll_ctl wan_serdes_pll_ctl = {.cfg_pll0_refin_en=gtmv(m, 1), .cfg_pll0_refout_en=gtmv(m, 1), .cfg_pll0_lcref_sel=gtmv(m, 1), .cfg_pll1_refin_en=gtmv(m, 1), .cfg_pll1_refout_en=gtmv(m, 1), .cfg_pll1_lcref_sel=gtmv(m, 1)};
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_pll_ctl_set( %u %u %u %u %u %u)\n", wan_serdes_pll_ctl.cfg_pll0_refin_en, wan_serdes_pll_ctl.cfg_pll0_refout_en, wan_serdes_pll_ctl.cfg_pll0_lcref_sel, wan_serdes_pll_ctl.cfg_pll1_refin_en, wan_serdes_pll_ctl.cfg_pll1_refout_en, wan_serdes_pll_ctl.cfg_pll1_lcref_sel);
        if(!err) ag_drv_wan_serdes_wan_serdes_pll_ctl_set(&wan_serdes_pll_ctl);
        if(!err) ag_drv_wan_serdes_wan_serdes_pll_ctl_get( &wan_serdes_pll_ctl);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_pll_ctl_get( %u %u %u %u %u %u)\n", wan_serdes_pll_ctl.cfg_pll0_refin_en, wan_serdes_pll_ctl.cfg_pll0_refout_en, wan_serdes_pll_ctl.cfg_pll0_lcref_sel, wan_serdes_pll_ctl.cfg_pll1_refin_en, wan_serdes_pll_ctl.cfg_pll1_refout_en, wan_serdes_pll_ctl.cfg_pll1_lcref_sel);
        if(err || wan_serdes_pll_ctl.cfg_pll0_refin_en!=gtmv(m, 1) || wan_serdes_pll_ctl.cfg_pll0_refout_en!=gtmv(m, 1) || wan_serdes_pll_ctl.cfg_pll0_lcref_sel!=gtmv(m, 1) || wan_serdes_pll_ctl.cfg_pll1_refin_en!=gtmv(m, 1) || wan_serdes_pll_ctl.cfg_pll1_refout_en!=gtmv(m, 1) || wan_serdes_pll_ctl.cfg_pll1_lcref_sel!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint16_t wan_temperature_read=gtmv(m, 10);
        if(!err) ag_drv_wan_serdes_wan_serdes_temp_ctl_get( &wan_temperature_read);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_temp_ctl_get( %u)\n", wan_temperature_read);
    }
    {
        uint16_t pram_address=gtmv(m, 16);
        bdmf_boolean pram_we=gtmv(m, 1);
        bdmf_boolean pram_go=gtmv(m, 1);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_pram_ctl_set( %u %u %u)\n", pram_address, pram_we, pram_go);
        if(!err) ag_drv_wan_serdes_wan_serdes_pram_ctl_set(pram_address, pram_we, pram_go);
        if(!err) ag_drv_wan_serdes_wan_serdes_pram_ctl_get( &pram_address, &pram_we, &pram_go);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_pram_ctl_get( %u %u %u)\n", pram_address, pram_we, pram_go);
        if(err || pram_address!=gtmv(m, 16) || pram_we!=gtmv(m, 1) || pram_go!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint32_t val=gtmv(m, 32);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_pram_val_low_set( %u)\n", val);
        if(!err) ag_drv_wan_serdes_wan_serdes_pram_val_low_set(val);
        if(!err) ag_drv_wan_serdes_wan_serdes_pram_val_low_get( &val);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_pram_val_low_get( %u)\n", val);
        if(err || val!=gtmv(m, 32))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint32_t val=gtmv(m, 32);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_pram_val_high_set( %u)\n", val);
        if(!err) ag_drv_wan_serdes_wan_serdes_pram_val_high_set(val);
        if(!err) ag_drv_wan_serdes_wan_serdes_pram_val_high_get( &val);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_wan_serdes_pram_val_high_get( %u)\n", val);
        if(err || val!=gtmv(m, 32))
            return err ? err : BDMF_ERR_IO;
    }
    {
        wan_serdes_top_osr_ctrl top_osr_ctrl = {.top_osr_control_cfg_gpon_rx_clk=gtmv(m, 2), .top_osr_control_txfifo_rd_legacy_mode=gtmv(m, 1), .top_osr_control_txlbe_ser_en=gtmv(m, 1), .top_osr_control_txlbe_ser_init_val=gtmv(m, 3), .top_osr_control_txlbe_ser_order=gtmv(m, 1)};
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_top_osr_ctrl_set( %u %u %u %u %u)\n", top_osr_ctrl.top_osr_control_cfg_gpon_rx_clk, top_osr_ctrl.top_osr_control_txfifo_rd_legacy_mode, top_osr_ctrl.top_osr_control_txlbe_ser_en, top_osr_ctrl.top_osr_control_txlbe_ser_init_val, top_osr_ctrl.top_osr_control_txlbe_ser_order);
        if(!err) ag_drv_wan_serdes_top_osr_ctrl_set(&top_osr_ctrl);
        if(!err) ag_drv_wan_serdes_top_osr_ctrl_get( &top_osr_ctrl);
        if(!err) bdmf_session_print(session, "ag_drv_wan_serdes_top_osr_ctrl_get( %u %u %u %u %u)\n", top_osr_ctrl.top_osr_control_cfg_gpon_rx_clk, top_osr_ctrl.top_osr_control_txfifo_rd_legacy_mode, top_osr_ctrl.top_osr_control_txlbe_ser_en, top_osr_ctrl.top_osr_control_txlbe_ser_init_val, top_osr_ctrl.top_osr_control_txlbe_ser_order);
        if(err || top_osr_ctrl.top_osr_control_cfg_gpon_rx_clk!=gtmv(m, 2) || top_osr_ctrl.top_osr_control_txfifo_rd_legacy_mode!=gtmv(m, 1) || top_osr_ctrl.top_osr_control_txlbe_ser_en!=gtmv(m, 1) || top_osr_ctrl.top_osr_control_txlbe_ser_init_val!=gtmv(m, 3) || top_osr_ctrl.top_osr_control_txlbe_ser_order!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    return err;
}

static int bcm_wan_serdes_cli_address(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    uint32_t i;
    uint32_t j;
    uint32_t index1_start=0;
    uint32_t index1_stop;
    uint32_t index2_start=0;
    uint32_t index2_stop;
    bdmfmon_cmd_parm_t * bdmf_parm;
    const ru_reg_rec * reg;
    const ru_block_rec * blk;
    const char * enum_string = bdmfmon_enum_parm_stringval(session, 0, parm[0].value.unumber);

    if(!enum_string)
        return BDMF_ERR_INTERNAL;

    switch (parm[0].value.unumber)
    {
    case bdmf_address_pll_ctl : reg = &RU_REG(WAN_SERDES, PLL_CTL); blk = &RU_BLK(WAN_SERDES); break;
    case bdmf_address_temp_ctl : reg = &RU_REG(WAN_SERDES, TEMP_CTL); blk = &RU_BLK(WAN_SERDES); break;
    case bdmf_address_pram_ctl : reg = &RU_REG(WAN_SERDES, PRAM_CTL); blk = &RU_BLK(WAN_SERDES); break;
    case bdmf_address_pram_val_low : reg = &RU_REG(WAN_SERDES, PRAM_VAL_LOW); blk = &RU_BLK(WAN_SERDES); break;
    case bdmf_address_pram_val_high : reg = &RU_REG(WAN_SERDES, PRAM_VAL_HIGH); blk = &RU_BLK(WAN_SERDES); break;
    case bdmf_address_cr_wan_top_wan_misc_serdes_oversample_ctrl : reg = &RU_REG(WAN_SERDES, CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL); blk = &RU_BLK(WAN_SERDES); break;
    default :
        return BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    if((bdmf_parm = bdmfmon_find_named_parm(session,"index1")))
    {
        index1_start = bdmf_parm->value.unumber;
        index1_stop = index1_start + 1;
    }
    else
        index1_stop = blk->addr_count;
    if((bdmf_parm = bdmfmon_find_named_parm(session,"index2")))
    {
        index2_start = bdmf_parm->value.unumber;
        index2_stop = index2_start + 1;
    }
    else
        index2_stop = reg->ram_count + 1;
    if(index1_stop > blk->addr_count)
    {
        bdmf_session_print(session, "index1 (%u) is out of range (%u).\n", index1_stop, blk->addr_count);
        return BDMF_ERR_RANGE;
    }
    if(index2_stop > (reg->ram_count + 1))
    {
        bdmf_session_print(session, "index2 (%u) is out of range (%u).\n", index2_stop, reg->ram_count + 1);
        return BDMF_ERR_RANGE;
    }
    if(reg->ram_count)
        for (i = index1_start; i < index1_stop; i++)
        {
            bdmf_session_print(session, "index1 = %u\n", i);
            for (j = index2_start; j < index2_stop; j++)
                bdmf_session_print(session, 	 "(%5u) 0x%lX\n", j, (blk->addr[i] + reg->addr + j));
        }
    else
        for (i = index1_start; i < index1_stop; i++)
            bdmf_session_print(session, "(%3u) 0x%lX\n", i, blk->addr[i]+reg->addr);
    return 0;
}

bdmfmon_handle_t ag_drv_wan_serdes_cli_init(bdmfmon_handle_t driver_dir)
{
    bdmfmon_handle_t dir;

    if ((dir = bdmfmon_dir_find(driver_dir, "wan_serdes"))!=NULL)
        return dir;
    dir = bdmfmon_dir_add(driver_dir, "wan_serdes", "wan_serdes", BDMF_ACCESS_ADMIN, NULL);

    {
        static bdmfmon_cmd_parm_t set_wan_serdes_pll_ctl[]={
            BDMFMON_MAKE_PARM("cfg_pll0_refin_en", "cfg_pll0_refin_en", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_pll0_refout_en", "cfg_pll0_refout_en", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_pll0_lcref_sel", "cfg_pll0_lcref_sel", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_pll1_refin_en", "cfg_pll1_refin_en", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_pll1_refout_en", "cfg_pll1_refout_en", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_pll1_lcref_sel", "cfg_pll1_lcref_sel", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_wan_serdes_pram_ctl[]={
            BDMFMON_MAKE_PARM("pram_address", "pram_address", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("pram_we", "pram_we", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("pram_go", "pram_go", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_wan_serdes_pram_val_low[]={
            BDMFMON_MAKE_PARM("val", "val", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_wan_serdes_pram_val_high[]={
            BDMFMON_MAKE_PARM("val", "val", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_top_osr_ctrl[]={
            BDMFMON_MAKE_PARM("top_osr_control_cfg_gpon_rx_clk", "top_osr_control_cfg_gpon_rx_clk", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("top_osr_control_txfifo_rd_legacy_mode", "top_osr_control_txfifo_rd_legacy_mode", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("top_osr_control_txlbe_ser_en", "top_osr_control_txlbe_ser_en", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("top_osr_control_txlbe_ser_init_val", "top_osr_control_txlbe_ser_init_val", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("top_osr_control_txlbe_ser_order", "top_osr_control_txlbe_ser_order", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="wan_serdes_pll_ctl", .val=cli_wan_serdes_wan_serdes_pll_ctl, .parms=set_wan_serdes_pll_ctl },
            { .name="wan_serdes_pram_ctl", .val=cli_wan_serdes_wan_serdes_pram_ctl, .parms=set_wan_serdes_pram_ctl },
            { .name="wan_serdes_pram_val_low", .val=cli_wan_serdes_wan_serdes_pram_val_low, .parms=set_wan_serdes_pram_val_low },
            { .name="wan_serdes_pram_val_high", .val=cli_wan_serdes_wan_serdes_pram_val_high, .parms=set_wan_serdes_pram_val_high },
            { .name="top_osr_ctrl", .val=cli_wan_serdes_top_osr_ctrl, .parms=set_top_osr_ctrl },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "set", "set", bcm_wan_serdes_cli_set,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_cmd_parm_t set_default[]={
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="wan_serdes_pll_ctl", .val=cli_wan_serdes_wan_serdes_pll_ctl, .parms=set_default },
            { .name="wan_serdes_temp_ctl", .val=cli_wan_serdes_wan_serdes_temp_ctl, .parms=set_default },
            { .name="wan_serdes_pram_ctl", .val=cli_wan_serdes_wan_serdes_pram_ctl, .parms=set_default },
            { .name="wan_serdes_pram_val_low", .val=cli_wan_serdes_wan_serdes_pram_val_low, .parms=set_default },
            { .name="wan_serdes_pram_val_high", .val=cli_wan_serdes_wan_serdes_pram_val_high, .parms=set_default },
            { .name="top_osr_ctrl", .val=cli_wan_serdes_top_osr_ctrl, .parms=set_default },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "get", "get", bcm_wan_serdes_cli_get,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_test_method[] = {
            { .name="low" , .val=bdmf_test_method_low },
            { .name="mid" , .val=bdmf_test_method_mid },
            { .name="high" , .val=bdmf_test_method_high },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "test", "test", bcm_wan_serdes_cli_test,
            BDMFMON_MAKE_PARM_ENUM("method", "low: 0000, mid: 1000, high: 1111", enum_table_test_method, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_address[] = {
            { .name="PLL_CTL" , .val=bdmf_address_pll_ctl },
            { .name="TEMP_CTL" , .val=bdmf_address_temp_ctl },
            { .name="PRAM_CTL" , .val=bdmf_address_pram_ctl },
            { .name="PRAM_VAL_LOW" , .val=bdmf_address_pram_val_low },
            { .name="PRAM_VAL_HIGH" , .val=bdmf_address_pram_val_high },
            { .name="CR_WAN_TOP_WAN_MISC_SERDES_OVERSAMPLE_CTRL" , .val=bdmf_address_cr_wan_top_wan_misc_serdes_oversample_ctrl },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "address", "address", bcm_wan_serdes_cli_address,
            BDMFMON_MAKE_PARM_ENUM("method", "method", enum_table_address, 0),
            BDMFMON_MAKE_PARM("index2", "onu_id/alloc_id/port_id/etc...", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
    }
    return dir;
}
#endif /* USE_BDMF_SHELL */

