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
#include "wan_prbs_ag.h"

bdmf_error_t ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_set(const wan_prbs_wan_prbs_chk_ctrl_0 *wan_prbs_chk_ctrl_0)
{
    uint32_t reg_chk_ctrl_0=0;

#ifdef VALIDATE_PARMS
    if(!wan_prbs_chk_ctrl_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((wan_prbs_chk_ctrl_0->en_timer_mode >= _2BITS_MAX_VAL_) ||
       (wan_prbs_chk_ctrl_0->en_timeout >= _5BITS_MAX_VAL_) ||
       (wan_prbs_chk_ctrl_0->mode_sel >= _3BITS_MAX_VAL_) ||
       (wan_prbs_chk_ctrl_0->err_cnt_burst_mode >= _1BITS_MAX_VAL_) ||
       (wan_prbs_chk_ctrl_0->lock_cnt >= _5BITS_MAX_VAL_) ||
       (wan_prbs_chk_ctrl_0->ool_cnt >= _5BITS_MAX_VAL_) ||
       (wan_prbs_chk_ctrl_0->inv >= _1BITS_MAX_VAL_) ||
       (wan_prbs_chk_ctrl_0->sig_prbs_status_clr >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_chk_ctrl_0 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_0, EN_TIMER_MODE, reg_chk_ctrl_0, wan_prbs_chk_ctrl_0->en_timer_mode);
    reg_chk_ctrl_0 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_0, EN_TIMEOUT, reg_chk_ctrl_0, wan_prbs_chk_ctrl_0->en_timeout);
    reg_chk_ctrl_0 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_0, MODE_SEL, reg_chk_ctrl_0, wan_prbs_chk_ctrl_0->mode_sel);
    reg_chk_ctrl_0 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_0, ERR_CNT_BURST_MODE, reg_chk_ctrl_0, wan_prbs_chk_ctrl_0->err_cnt_burst_mode);
    reg_chk_ctrl_0 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_0, LOCK_CNT, reg_chk_ctrl_0, wan_prbs_chk_ctrl_0->lock_cnt);
    reg_chk_ctrl_0 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_0, OOL_CNT, reg_chk_ctrl_0, wan_prbs_chk_ctrl_0->ool_cnt);
    reg_chk_ctrl_0 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_0, INV, reg_chk_ctrl_0, wan_prbs_chk_ctrl_0->inv);
    reg_chk_ctrl_0 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_0, SIG_PRBS_STATUS_CLR, reg_chk_ctrl_0, wan_prbs_chk_ctrl_0->sig_prbs_status_clr);

    RU_REG_WRITE(0, WAN_PRBS, CHK_CTRL_0, reg_chk_ctrl_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_get(wan_prbs_wan_prbs_chk_ctrl_0 *wan_prbs_chk_ctrl_0)
{
    uint32_t reg_chk_ctrl_0;

#ifdef VALIDATE_PARMS
    if(!wan_prbs_chk_ctrl_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_PRBS, CHK_CTRL_0, reg_chk_ctrl_0);

    wan_prbs_chk_ctrl_0->en_timer_mode = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_0, EN_TIMER_MODE, reg_chk_ctrl_0);
    wan_prbs_chk_ctrl_0->en_timeout = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_0, EN_TIMEOUT, reg_chk_ctrl_0);
    wan_prbs_chk_ctrl_0->mode_sel = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_0, MODE_SEL, reg_chk_ctrl_0);
    wan_prbs_chk_ctrl_0->err_cnt_burst_mode = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_0, ERR_CNT_BURST_MODE, reg_chk_ctrl_0);
    wan_prbs_chk_ctrl_0->lock_cnt = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_0, LOCK_CNT, reg_chk_ctrl_0);
    wan_prbs_chk_ctrl_0->ool_cnt = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_0, OOL_CNT, reg_chk_ctrl_0);
    wan_prbs_chk_ctrl_0->inv = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_0, INV, reg_chk_ctrl_0);
    wan_prbs_chk_ctrl_0->sig_prbs_status_clr = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_0, SIG_PRBS_STATUS_CLR, reg_chk_ctrl_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_set(bdmf_boolean prbs_chk_en, uint8_t prbs_chk_mode, uint32_t prbs_timer_val)
{
    uint32_t reg_chk_ctrl_1=0;

#ifdef VALIDATE_PARMS
    if((prbs_timer_val >= _20BITS_MAX_VAL_) ||
       (prbs_chk_mode >= _2BITS_MAX_VAL_) ||
       (prbs_chk_en >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_chk_ctrl_1 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_1, PRBS_TIMER_VAL, reg_chk_ctrl_1, prbs_timer_val);
    reg_chk_ctrl_1 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_1, PRBS_CHK_MODE, reg_chk_ctrl_1, prbs_chk_mode);
    reg_chk_ctrl_1 = RU_FIELD_SET(0, WAN_PRBS, CHK_CTRL_1, PRBS_CHK_EN, reg_chk_ctrl_1, prbs_chk_en);

    RU_REG_WRITE(0, WAN_PRBS, CHK_CTRL_1, reg_chk_ctrl_1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_get(bdmf_boolean *prbs_chk_en, uint8_t *prbs_chk_mode, uint32_t *prbs_timer_val)
{
    uint32_t reg_chk_ctrl_1;

#ifdef VALIDATE_PARMS
    if(!prbs_timer_val || !prbs_chk_mode || !prbs_chk_en)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_PRBS, CHK_CTRL_1, reg_chk_ctrl_1);

    *prbs_timer_val = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_1, PRBS_TIMER_VAL, reg_chk_ctrl_1);
    *prbs_chk_mode = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_1, PRBS_CHK_MODE, reg_chk_ctrl_1);
    *prbs_chk_en = RU_FIELD_GET(0, WAN_PRBS, CHK_CTRL_1, PRBS_CHK_EN, reg_chk_ctrl_1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_prbs_wan_prbs_status_0_get(bdmf_boolean *lock_lost_lh, uint32_t *err_cnt)
{
    uint32_t reg_status_0;

#ifdef VALIDATE_PARMS
    if(!err_cnt || !lock_lost_lh)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_PRBS, STATUS_0, reg_status_0);

    *err_cnt = RU_FIELD_GET(0, WAN_PRBS, STATUS_0, ERR_CNT, reg_status_0);
    *lock_lost_lh = RU_FIELD_GET(0, WAN_PRBS, STATUS_0, LOCK_LOST_LH, reg_status_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_wan_prbs_wan_prbs_status_1_get(bdmf_boolean *any_err, bdmf_boolean *lock)
{
    uint32_t reg_status_1;

#ifdef VALIDATE_PARMS
    if(!lock || !any_err)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, WAN_PRBS, STATUS_1, reg_status_1);

    *lock = RU_FIELD_GET(0, WAN_PRBS, STATUS_1, LOCK, reg_status_1);
    *any_err = RU_FIELD_GET(0, WAN_PRBS, STATUS_1, ANY_ERR, reg_status_1);

    return BDMF_ERR_OK;
}

#ifdef USE_BDMF_SHELL
typedef enum
{
    bdmf_address_chk_ctrl_0,
    bdmf_address_chk_ctrl_1,
    bdmf_address_status_0,
    bdmf_address_status_1,
}
bdmf_address;

static int bcm_wan_prbs_cli_set(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_wan_prbs_wan_prbs_chk_ctrl_0:
    {
        wan_prbs_wan_prbs_chk_ctrl_0 wan_prbs_chk_ctrl_0 = { .en_timer_mode=parm[1].value.unumber, .en_timeout=parm[2].value.unumber, .mode_sel=parm[3].value.unumber, .err_cnt_burst_mode=parm[4].value.unumber, .lock_cnt=parm[5].value.unumber, .ool_cnt=parm[6].value.unumber, .inv=parm[7].value.unumber, .sig_prbs_status_clr=parm[8].value.unumber};
        err = ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_set(&wan_prbs_chk_ctrl_0);
        break;
    }
    case cli_wan_prbs_wan_prbs_chk_ctrl_1:
        err = ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_set(parm[1].value.unumber, parm[2].value.unumber, parm[3].value.unumber);
        break;
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

int bcm_wan_prbs_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_wan_prbs_wan_prbs_chk_ctrl_0:
    {
        wan_prbs_wan_prbs_chk_ctrl_0 wan_prbs_chk_ctrl_0;
        err = ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_get(&wan_prbs_chk_ctrl_0);
        bdmf_session_print(session, "en_timer_mode = %u (0x%x)\n", wan_prbs_chk_ctrl_0.en_timer_mode, wan_prbs_chk_ctrl_0.en_timer_mode);
        bdmf_session_print(session, "en_timeout = %u (0x%x)\n", wan_prbs_chk_ctrl_0.en_timeout, wan_prbs_chk_ctrl_0.en_timeout);
        bdmf_session_print(session, "mode_sel = %u (0x%x)\n", wan_prbs_chk_ctrl_0.mode_sel, wan_prbs_chk_ctrl_0.mode_sel);
        bdmf_session_print(session, "err_cnt_burst_mode = %u (0x%x)\n", wan_prbs_chk_ctrl_0.err_cnt_burst_mode, wan_prbs_chk_ctrl_0.err_cnt_burst_mode);
        bdmf_session_print(session, "lock_cnt = %u (0x%x)\n", wan_prbs_chk_ctrl_0.lock_cnt, wan_prbs_chk_ctrl_0.lock_cnt);
        bdmf_session_print(session, "ool_cnt = %u (0x%x)\n", wan_prbs_chk_ctrl_0.ool_cnt, wan_prbs_chk_ctrl_0.ool_cnt);
        bdmf_session_print(session, "inv = %u (0x%x)\n", wan_prbs_chk_ctrl_0.inv, wan_prbs_chk_ctrl_0.inv);
        bdmf_session_print(session, "sig_prbs_status_clr = %u (0x%x)\n", wan_prbs_chk_ctrl_0.sig_prbs_status_clr, wan_prbs_chk_ctrl_0.sig_prbs_status_clr);
        break;
    }
    case cli_wan_prbs_wan_prbs_chk_ctrl_1:
    {
        uint32_t prbs_timer_val;
        uint8_t prbs_chk_mode;
        bdmf_boolean prbs_chk_en;
        err = ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_get(&prbs_timer_val, &prbs_chk_mode, &prbs_chk_en);
        bdmf_session_print(session, "prbs_timer_val = %u (0x%x)\n", prbs_timer_val, prbs_timer_val);
        bdmf_session_print(session, "prbs_chk_mode = %u (0x%x)\n", prbs_chk_mode, prbs_chk_mode);
        bdmf_session_print(session, "prbs_chk_en = %u (0x%x)\n", prbs_chk_en, prbs_chk_en);
        break;
    }
    case cli_wan_prbs_wan_prbs_status_0:
    {
        uint32_t err_cnt;
        bdmf_boolean lock_lost_lh;
        err = ag_drv_wan_prbs_wan_prbs_status_0_get(&err_cnt, &lock_lost_lh);
        bdmf_session_print(session, "err_cnt = %u (0x%x)\n", err_cnt, err_cnt);
        bdmf_session_print(session, "lock_lost_lh = %u (0x%x)\n", lock_lost_lh, lock_lost_lh);
        break;
    }
    case cli_wan_prbs_wan_prbs_status_1:
    {
        bdmf_boolean lock;
        bdmf_boolean any_err;
        err = ag_drv_wan_prbs_wan_prbs_status_1_get(&lock, &any_err);
        bdmf_session_print(session, "lock = %u (0x%x)\n", lock, lock);
        bdmf_session_print(session, "any_err = %u (0x%x)\n", any_err, any_err);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

static int bcm_wan_prbs_cli_test(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_test_method m = parm[0].value.unumber;
    bdmf_error_t err = BDMF_ERR_OK;

    {
        wan_prbs_wan_prbs_chk_ctrl_0 wan_prbs_chk_ctrl_0 = {.en_timer_mode=gtmv(m, 2), .en_timeout=gtmv(m, 5), .mode_sel=gtmv(m, 3), .err_cnt_burst_mode=gtmv(m, 1), .lock_cnt=gtmv(m, 5), .ool_cnt=gtmv(m, 5), .inv=gtmv(m, 1), .sig_prbs_status_clr=gtmv(m, 1)};
        if(!err) bdmf_session_print(session, "ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_set( %u %u %u %u %u %u %u %u)\n", wan_prbs_chk_ctrl_0.en_timer_mode, wan_prbs_chk_ctrl_0.en_timeout, wan_prbs_chk_ctrl_0.mode_sel, wan_prbs_chk_ctrl_0.err_cnt_burst_mode, wan_prbs_chk_ctrl_0.lock_cnt, wan_prbs_chk_ctrl_0.ool_cnt, wan_prbs_chk_ctrl_0.inv, wan_prbs_chk_ctrl_0.sig_prbs_status_clr);
        if(!err) ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_set(&wan_prbs_chk_ctrl_0);
        if(!err) ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_get( &wan_prbs_chk_ctrl_0);
        if(!err) bdmf_session_print(session, "ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_get( %u %u %u %u %u %u %u %u)\n", wan_prbs_chk_ctrl_0.en_timer_mode, wan_prbs_chk_ctrl_0.en_timeout, wan_prbs_chk_ctrl_0.mode_sel, wan_prbs_chk_ctrl_0.err_cnt_burst_mode, wan_prbs_chk_ctrl_0.lock_cnt, wan_prbs_chk_ctrl_0.ool_cnt, wan_prbs_chk_ctrl_0.inv, wan_prbs_chk_ctrl_0.sig_prbs_status_clr);
        if(err || wan_prbs_chk_ctrl_0.en_timer_mode!=gtmv(m, 2) || wan_prbs_chk_ctrl_0.en_timeout!=gtmv(m, 5) || wan_prbs_chk_ctrl_0.mode_sel!=gtmv(m, 3) || wan_prbs_chk_ctrl_0.err_cnt_burst_mode!=gtmv(m, 1) || wan_prbs_chk_ctrl_0.lock_cnt!=gtmv(m, 5) || wan_prbs_chk_ctrl_0.ool_cnt!=gtmv(m, 5) || wan_prbs_chk_ctrl_0.inv!=gtmv(m, 1) || wan_prbs_chk_ctrl_0.sig_prbs_status_clr!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint32_t prbs_timer_val=gtmv(m, 20);
        uint8_t prbs_chk_mode=gtmv(m, 2);
        bdmf_boolean prbs_chk_en=gtmv(m, 1);
        if(!err) bdmf_session_print(session, "ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_set( %u %u %u)\n", prbs_timer_val, prbs_chk_mode, prbs_chk_en);
        if(!err) ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_set(prbs_timer_val, prbs_chk_mode, prbs_chk_en);
        if(!err) ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_get( &prbs_timer_val, &prbs_chk_mode, &prbs_chk_en);
        if(!err) bdmf_session_print(session, "ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_get( %u %u %u)\n", prbs_timer_val, prbs_chk_mode, prbs_chk_en);
        if(err || prbs_timer_val!=gtmv(m, 20) || prbs_chk_mode!=gtmv(m, 2) || prbs_chk_en!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint32_t err_cnt=gtmv(m, 31);
        bdmf_boolean lock_lost_lh=gtmv(m, 1);
        if(!err) ag_drv_wan_prbs_wan_prbs_status_0_get( &err_cnt, &lock_lost_lh);
        if(!err) bdmf_session_print(session, "ag_drv_wan_prbs_wan_prbs_status_0_get( %u %u)\n", err_cnt, lock_lost_lh);
    }
    {
        bdmf_boolean lock=gtmv(m, 1);
        bdmf_boolean any_err=gtmv(m, 1);
        if(!err) ag_drv_wan_prbs_wan_prbs_status_1_get( &lock, &any_err);
        if(!err) bdmf_session_print(session, "ag_drv_wan_prbs_wan_prbs_status_1_get( %u %u)\n", lock, any_err);
    }
    return err;
}

static int bcm_wan_prbs_cli_address(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
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
    case bdmf_address_chk_ctrl_0 : reg = &RU_REG(WAN_PRBS, CHK_CTRL_0); blk = &RU_BLK(WAN_PRBS); break;
    case bdmf_address_chk_ctrl_1 : reg = &RU_REG(WAN_PRBS, CHK_CTRL_1); blk = &RU_BLK(WAN_PRBS); break;
    case bdmf_address_status_0 : reg = &RU_REG(WAN_PRBS, STATUS_0); blk = &RU_BLK(WAN_PRBS); break;
    case bdmf_address_status_1 : reg = &RU_REG(WAN_PRBS, STATUS_1); blk = &RU_BLK(WAN_PRBS); break;
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

bdmfmon_handle_t ag_drv_wan_prbs_cli_init(bdmfmon_handle_t driver_dir)
{
    bdmfmon_handle_t dir;

    if ((dir = bdmfmon_dir_find(driver_dir, "wan_prbs"))!=NULL)
        return dir;
    dir = bdmfmon_dir_add(driver_dir, "wan_prbs", "wan_prbs", BDMF_ACCESS_ADMIN, NULL);

    {
        static bdmfmon_cmd_parm_t set_wan_prbs_chk_ctrl_0[]={
            BDMFMON_MAKE_PARM("en_timer_mode", "en_timer_mode", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("en_timeout", "en_timeout", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("mode_sel", "mode_sel", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("err_cnt_burst_mode", "err_cnt_burst_mode", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("lock_cnt", "lock_cnt", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("ool_cnt", "ool_cnt", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("inv", "inv", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("sig_prbs_status_clr", "sig_prbs_status_clr", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_wan_prbs_chk_ctrl_1[]={
            BDMFMON_MAKE_PARM("prbs_timer_val", "prbs_timer_val", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("prbs_chk_mode", "prbs_chk_mode", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("prbs_chk_en", "prbs_chk_en", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="wan_prbs_chk_ctrl_0", .val=cli_wan_prbs_wan_prbs_chk_ctrl_0, .parms=set_wan_prbs_chk_ctrl_0 },
            { .name="wan_prbs_chk_ctrl_1", .val=cli_wan_prbs_wan_prbs_chk_ctrl_1, .parms=set_wan_prbs_chk_ctrl_1 },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "set", "set", bcm_wan_prbs_cli_set,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_cmd_parm_t set_default[]={
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="wan_prbs_chk_ctrl_0", .val=cli_wan_prbs_wan_prbs_chk_ctrl_0, .parms=set_default },
            { .name="wan_prbs_chk_ctrl_1", .val=cli_wan_prbs_wan_prbs_chk_ctrl_1, .parms=set_default },
            { .name="wan_prbs_status_0", .val=cli_wan_prbs_wan_prbs_status_0, .parms=set_default },
            { .name="wan_prbs_status_1", .val=cli_wan_prbs_wan_prbs_status_1, .parms=set_default },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "get", "get", bcm_wan_prbs_cli_get,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_test_method[] = {
            { .name="low" , .val=bdmf_test_method_low },
            { .name="mid" , .val=bdmf_test_method_mid },
            { .name="high" , .val=bdmf_test_method_high },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "test", "test", bcm_wan_prbs_cli_test,
            BDMFMON_MAKE_PARM_ENUM("method", "low: 0000, mid: 1000, high: 1111", enum_table_test_method, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_address[] = {
            { .name="CHK_CTRL_0" , .val=bdmf_address_chk_ctrl_0 },
            { .name="CHK_CTRL_1" , .val=bdmf_address_chk_ctrl_1 },
            { .name="STATUS_0" , .val=bdmf_address_status_0 },
            { .name="STATUS_1" , .val=bdmf_address_status_1 },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "address", "address", bcm_wan_prbs_cli_address,
            BDMFMON_MAKE_PARM_ENUM("method", "method", enum_table_address, 0),
            BDMFMON_MAKE_PARM("index2", "onu_id/alloc_id/port_id/etc...", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
    }
    return dir;
}
#endif /* USE_BDMF_SHELL */

