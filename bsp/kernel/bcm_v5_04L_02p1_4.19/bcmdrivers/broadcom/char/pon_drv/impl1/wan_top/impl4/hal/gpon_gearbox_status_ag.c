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
#include "gpon_gearbox_status_ag.h"

bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_status_get(uint32_t *cr_rd_data_clx)
{
    uint32_t reg_gearbox_status;

#ifdef VALIDATE_PARMS
    if(!cr_rd_data_clx)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON_GEARBOX_STATUS, GEARBOX_STATUS, reg_gearbox_status);

    *cr_rd_data_clx = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_STATUS, CR_RD_DATA_CLX, reg_gearbox_status);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_set(const gpon_gearbox_status_gearbox_prbs_control_0 *gearbox_prbs_control_0)
{
    uint32_t reg_gearbox_prbs_control_0=0;

#ifdef VALIDATE_PARMS
    if(!gearbox_prbs_control_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr >= _1BITS_MAX_VAL_) ||
       (gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv >= _1BITS_MAX_VAL_) ||
       (gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt >= _5BITS_MAX_VAL_) ||
       (gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt >= _5BITS_MAX_VAL_) ||
       (gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode >= _1BITS_MAX_VAL_) ||
       (gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel >= _3BITS_MAX_VAL_) ||
       (gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout >= _5BITS_MAX_VAL_) ||
       (gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode >= _2BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_gearbox_prbs_control_0 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_SIG_PRBS_STATUS_CLR, reg_gearbox_prbs_control_0, gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr);
    reg_gearbox_prbs_control_0 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_INV, reg_gearbox_prbs_control_0, gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv);
    reg_gearbox_prbs_control_0 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_OOL_CNT, reg_gearbox_prbs_control_0, gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt);
    reg_gearbox_prbs_control_0 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_LOCK_CNT, reg_gearbox_prbs_control_0, gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt);
    reg_gearbox_prbs_control_0 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_ERR_CNT_BURST_MODE, reg_gearbox_prbs_control_0, gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode);
    reg_gearbox_prbs_control_0 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_MODE_SEL, reg_gearbox_prbs_control_0, gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel);
    reg_gearbox_prbs_control_0 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_EN_TIMEOUT, reg_gearbox_prbs_control_0, gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout);
    reg_gearbox_prbs_control_0 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_EN_TIMER_MODE, reg_gearbox_prbs_control_0, gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode);

    RU_REG_WRITE(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, reg_gearbox_prbs_control_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_get(gpon_gearbox_status_gearbox_prbs_control_0 *gearbox_prbs_control_0)
{
    uint32_t reg_gearbox_prbs_control_0;

#ifdef VALIDATE_PARMS
    if(!gearbox_prbs_control_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, reg_gearbox_prbs_control_0);

    gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_SIG_PRBS_STATUS_CLR, reg_gearbox_prbs_control_0);
    gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_INV, reg_gearbox_prbs_control_0);
    gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_OOL_CNT, reg_gearbox_prbs_control_0);
    gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_LOCK_CNT, reg_gearbox_prbs_control_0);
    gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_ERR_CNT_BURST_MODE, reg_gearbox_prbs_control_0);
    gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_MODE_SEL, reg_gearbox_prbs_control_0);
    gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_EN_TIMEOUT, reg_gearbox_prbs_control_0);
    gearbox_prbs_control_0->cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_0_EN_TIMER_MODE, reg_gearbox_prbs_control_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_set(bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, uint32_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val)
{
    uint32_t reg_gearbox_prbs_control_1=0;

#ifdef VALIDATE_PARMS
    if((cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en >= _1BITS_MAX_VAL_) ||
       (cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode >= _2BITS_MAX_VAL_) ||
       (cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val >= _20BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_gearbox_prbs_control_1 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_1_EN, reg_gearbox_prbs_control_1, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en);
    reg_gearbox_prbs_control_1 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_1_MODE, reg_gearbox_prbs_control_1, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode);
    reg_gearbox_prbs_control_1 = RU_FIELD_SET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_1_TIMER_VAL, reg_gearbox_prbs_control_1, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);

    RU_REG_WRITE(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1, reg_gearbox_prbs_control_1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_get(bdmf_boolean *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, uint8_t *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, uint32_t *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val)
{
    uint32_t reg_gearbox_prbs_control_1;

#ifdef VALIDATE_PARMS
    if(!cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en || !cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode || !cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1, reg_gearbox_prbs_control_1);

    *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_1_EN, reg_gearbox_prbs_control_1);
    *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_1_MODE, reg_gearbox_prbs_control_1);
    *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_RG_PRBS_CHK_CTRL_1_TIMER_VAL, reg_gearbox_prbs_control_1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_status_0_get(uint32_t *gpon_gearbox_prbs_stat_0_vector)
{
    uint32_t reg_gearbox_prbs_status_0;

#ifdef VALIDATE_PARMS
    if(!gpon_gearbox_prbs_stat_0_vector)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_STATUS_0, reg_gearbox_prbs_status_0);

    *gpon_gearbox_prbs_stat_0_vector = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_STATUS_0, GPON_GEARBOX_PRBS_STAT_0_VECTOR, reg_gearbox_prbs_status_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_status_1_get(uint8_t *gpon_gearbox_prbs_stat_1_vector)
{
    uint32_t reg_gearbox_prbs_status_1;

#ifdef VALIDATE_PARMS
    if(!gpon_gearbox_prbs_stat_1_vector)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_STATUS_1, reg_gearbox_prbs_status_1);

    *gpon_gearbox_prbs_stat_1_vector = RU_FIELD_GET(0, GPON_GEARBOX_STATUS, GEARBOX_PRBS_STATUS_1, GPON_GEARBOX_PRBS_STAT_1_VECTOR, reg_gearbox_prbs_status_1);

    return BDMF_ERR_OK;
}

#ifdef USE_BDMF_SHELL
typedef enum
{
    bdmf_address_gearbox_status,
    bdmf_address_gearbox_prbs_control_0,
    bdmf_address_gearbox_prbs_control_1,
    bdmf_address_gearbox_prbs_status_0,
    bdmf_address_gearbox_prbs_status_1,
}
bdmf_address;

static int bcm_gpon_gearbox_status_cli_set(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_gpon_gearbox_status_gearbox_prbs_control_0:
    {
        gpon_gearbox_status_gearbox_prbs_control_0 gearbox_prbs_control_0 = { .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr=parm[1].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv=parm[2].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt=parm[3].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt=parm[4].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode=parm[5].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel=parm[6].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout=parm[7].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode=parm[8].value.unumber};
        err = ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_set(&gearbox_prbs_control_0);
        break;
    }
    case cli_gpon_gearbox_status_gearbox_prbs_control_1:
        err = ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_set(parm[1].value.unumber, parm[2].value.unumber, parm[3].value.unumber);
        break;
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

int bcm_gpon_gearbox_status_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_gpon_gearbox_status_gearbox_status:
    {
        uint32_t cr_rd_data_clx;
        err = ag_drv_gpon_gearbox_status_gearbox_status_get(&cr_rd_data_clx);
        bdmf_session_print(session, "cr_rd_data_clx = %u (0x%x)\n", cr_rd_data_clx, cr_rd_data_clx);
        break;
    }
    case cli_gpon_gearbox_status_gearbox_prbs_control_0:
    {
        gpon_gearbox_status_gearbox_prbs_control_0 gearbox_prbs_control_0;
        err = ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_get(&gearbox_prbs_control_0);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr = %u (0x%x)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv = %u (0x%x)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt = %u (0x%x)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt = %u (0x%x)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode = %u (0x%x)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel = %u (0x%x)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout = %u (0x%x)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode = %u (0x%x)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode);
        break;
    }
    case cli_gpon_gearbox_status_gearbox_prbs_control_1:
    {
        bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en;
        uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode;
        uint32_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val;
        err = ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_get(&cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, &cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, &cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en = %u (0x%x)\n", cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode = %u (0x%x)\n", cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val = %u (0x%x)\n", cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);
        break;
    }
    case cli_gpon_gearbox_status_gearbox_prbs_status_0:
    {
        uint32_t gpon_gearbox_prbs_stat_0_vector;
        err = ag_drv_gpon_gearbox_status_gearbox_prbs_status_0_get(&gpon_gearbox_prbs_stat_0_vector);
        bdmf_session_print(session, "gpon_gearbox_prbs_stat_0_vector = %u (0x%x)\n", gpon_gearbox_prbs_stat_0_vector, gpon_gearbox_prbs_stat_0_vector);
        break;
    }
    case cli_gpon_gearbox_status_gearbox_prbs_status_1:
    {
        uint8_t gpon_gearbox_prbs_stat_1_vector;
        err = ag_drv_gpon_gearbox_status_gearbox_prbs_status_1_get(&gpon_gearbox_prbs_stat_1_vector);
        bdmf_session_print(session, "gpon_gearbox_prbs_stat_1_vector = %u (0x%x)\n", gpon_gearbox_prbs_stat_1_vector, gpon_gearbox_prbs_stat_1_vector);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

static int bcm_gpon_gearbox_status_cli_test(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_test_method m = parm[0].value.unumber;
    bdmf_error_t err = BDMF_ERR_OK;

    {
        uint32_t cr_rd_data_clx=gtmv(m, 32);
        if(!err) ag_drv_gpon_gearbox_status_gearbox_status_get( &cr_rd_data_clx);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gearbox_status_gearbox_status_get( %u)\n", cr_rd_data_clx);
    }
    {
        gpon_gearbox_status_gearbox_prbs_control_0 gearbox_prbs_control_0 = {.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt=gtmv(m, 5), .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt=gtmv(m, 5), .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel=gtmv(m, 3), .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout=gtmv(m, 5), .cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode=gtmv(m, 2)};
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_set( %u %u %u %u %u %u %u %u)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode);
        if(!err) ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_set(&gearbox_prbs_control_0);
        if(!err) ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_get( &gearbox_prbs_control_0);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_get( %u %u %u %u %u %u %u %u)\n", gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout, gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode);
        if(err || gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr!=gtmv(m, 1) || gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv!=gtmv(m, 1) || gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt!=gtmv(m, 5) || gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt!=gtmv(m, 5) || gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode!=gtmv(m, 1) || gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel!=gtmv(m, 3) || gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout!=gtmv(m, 5) || gearbox_prbs_control_0.cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode!=gtmv(m, 2))
            return err ? err : BDMF_ERR_IO;
    }
    {
        bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en=gtmv(m, 1);
        uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode=gtmv(m, 2);
        uint32_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val=gtmv(m, 20);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_set( %u %u %u)\n", cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);
        if(!err) ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_set(cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);
        if(!err) ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_get( &cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, &cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, &cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_get( %u %u %u)\n", cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);
        if(err || cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en!=gtmv(m, 1) || cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode!=gtmv(m, 2) || cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val!=gtmv(m, 20))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint32_t gpon_gearbox_prbs_stat_0_vector=gtmv(m, 32);
        if(!err) ag_drv_gpon_gearbox_status_gearbox_prbs_status_0_get( &gpon_gearbox_prbs_stat_0_vector);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gearbox_status_gearbox_prbs_status_0_get( %u)\n", gpon_gearbox_prbs_stat_0_vector);
    }
    {
        uint8_t gpon_gearbox_prbs_stat_1_vector=gtmv(m, 2);
        if(!err) ag_drv_gpon_gearbox_status_gearbox_prbs_status_1_get( &gpon_gearbox_prbs_stat_1_vector);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gearbox_status_gearbox_prbs_status_1_get( %u)\n", gpon_gearbox_prbs_stat_1_vector);
    }
    return err;
}

static int bcm_gpon_gearbox_status_cli_address(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
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
    case bdmf_address_gearbox_status : reg = &RU_REG(GPON_GEARBOX_STATUS, GEARBOX_STATUS); blk = &RU_BLK(GPON_GEARBOX_STATUS); break;
    case bdmf_address_gearbox_prbs_control_0 : reg = &RU_REG(GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_0); blk = &RU_BLK(GPON_GEARBOX_STATUS); break;
    case bdmf_address_gearbox_prbs_control_1 : reg = &RU_REG(GPON_GEARBOX_STATUS, GEARBOX_PRBS_CONTROL_1); blk = &RU_BLK(GPON_GEARBOX_STATUS); break;
    case bdmf_address_gearbox_prbs_status_0 : reg = &RU_REG(GPON_GEARBOX_STATUS, GEARBOX_PRBS_STATUS_0); blk = &RU_BLK(GPON_GEARBOX_STATUS); break;
    case bdmf_address_gearbox_prbs_status_1 : reg = &RU_REG(GPON_GEARBOX_STATUS, GEARBOX_PRBS_STATUS_1); blk = &RU_BLK(GPON_GEARBOX_STATUS); break;
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

bdmfmon_handle_t ag_drv_gpon_gearbox_status_cli_init(bdmfmon_handle_t driver_dir)
{
    bdmfmon_handle_t dir;

    if ((dir = bdmfmon_dir_find(driver_dir, "gpon_gearbox_status"))!=NULL)
        return dir;
    dir = bdmfmon_dir_add(driver_dir, "gpon_gearbox_status", "gpon_gearbox_status", BDMF_ACCESS_ADMIN, NULL);

    {
        static bdmfmon_cmd_parm_t set_gearbox_prbs_control_0[]={
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_gearbox_prbs_control_1[]={
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val", "cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="gearbox_prbs_control_0", .val=cli_gpon_gearbox_status_gearbox_prbs_control_0, .parms=set_gearbox_prbs_control_0 },
            { .name="gearbox_prbs_control_1", .val=cli_gpon_gearbox_status_gearbox_prbs_control_1, .parms=set_gearbox_prbs_control_1 },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "set", "set", bcm_gpon_gearbox_status_cli_set,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_cmd_parm_t set_default[]={
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="gearbox_status", .val=cli_gpon_gearbox_status_gearbox_status, .parms=set_default },
            { .name="gearbox_prbs_control_0", .val=cli_gpon_gearbox_status_gearbox_prbs_control_0, .parms=set_default },
            { .name="gearbox_prbs_control_1", .val=cli_gpon_gearbox_status_gearbox_prbs_control_1, .parms=set_default },
            { .name="gearbox_prbs_status_0", .val=cli_gpon_gearbox_status_gearbox_prbs_status_0, .parms=set_default },
            { .name="gearbox_prbs_status_1", .val=cli_gpon_gearbox_status_gearbox_prbs_status_1, .parms=set_default },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "get", "get", bcm_gpon_gearbox_status_cli_get,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_test_method[] = {
            { .name="low" , .val=bdmf_test_method_low },
            { .name="mid" , .val=bdmf_test_method_mid },
            { .name="high" , .val=bdmf_test_method_high },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "test", "test", bcm_gpon_gearbox_status_cli_test,
            BDMFMON_MAKE_PARM_ENUM("method", "low: 0000, mid: 1000, high: 1111", enum_table_test_method, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_address[] = {
            { .name="GEARBOX_STATUS" , .val=bdmf_address_gearbox_status },
            { .name="GEARBOX_PRBS_CONTROL_0" , .val=bdmf_address_gearbox_prbs_control_0 },
            { .name="GEARBOX_PRBS_CONTROL_1" , .val=bdmf_address_gearbox_prbs_control_1 },
            { .name="GEARBOX_PRBS_STATUS_0" , .val=bdmf_address_gearbox_prbs_status_0 },
            { .name="GEARBOX_PRBS_STATUS_1" , .val=bdmf_address_gearbox_prbs_status_1 },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "address", "address", bcm_gpon_gearbox_status_cli_address,
            BDMFMON_MAKE_PARM_ENUM("method", "method", enum_table_address, 0),
            BDMFMON_MAKE_PARM("index2", "onu_id/alloc_id/port_id/etc...", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
    }
    return dir;
}
#endif /* USE_BDMF_SHELL */

