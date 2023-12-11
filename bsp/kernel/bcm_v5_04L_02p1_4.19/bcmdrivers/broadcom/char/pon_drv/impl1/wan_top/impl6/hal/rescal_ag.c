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
#include "rescal_ag.h"

bdmf_error_t ag_drv_rescal_rescal_cfg_set(const rescal_rescal_cfg *rescal_cfg)
{
    uint32_t reg_cfg=0;

#ifdef VALIDATE_PARMS
    if(!rescal_cfg)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((rescal_cfg->ctrl >= _13BITS_MAX_VAL_) ||
       (rescal_cfg->pwrdn >= _1BITS_MAX_VAL_) ||
       (rescal_cfg->diag_on >= _1BITS_MAX_VAL_) ||
       (rescal_cfg->rstb >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_cfg = RU_FIELD_SET(0, RESCAL, CFG, CTRL, reg_cfg, rescal_cfg->ctrl);
    reg_cfg = RU_FIELD_SET(0, RESCAL, CFG, PWRDN, reg_cfg, rescal_cfg->pwrdn);
    reg_cfg = RU_FIELD_SET(0, RESCAL, CFG, DIAG_ON, reg_cfg, rescal_cfg->diag_on);
    reg_cfg = RU_FIELD_SET(0, RESCAL, CFG, RSTB, reg_cfg, rescal_cfg->rstb);

    RU_REG_WRITE(0, RESCAL, CFG, reg_cfg);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_rescal_rescal_cfg_get(rescal_rescal_cfg *rescal_cfg)
{
    uint32_t reg_cfg;

#ifdef VALIDATE_PARMS
    if(!rescal_cfg)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, RESCAL, CFG, reg_cfg);

    rescal_cfg->ctrl = RU_FIELD_GET(0, RESCAL, CFG, CTRL, reg_cfg);
    rescal_cfg->pwrdn = RU_FIELD_GET(0, RESCAL, CFG, PWRDN, reg_cfg);
    rescal_cfg->diag_on = RU_FIELD_GET(0, RESCAL, CFG, DIAG_ON, reg_cfg);
    rescal_cfg->rstb = RU_FIELD_GET(0, RESCAL, CFG, RSTB, reg_cfg);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_rescal_rescal_status_0_get(rescal_rescal_status_0 *rescal_status_0)
{
    uint32_t reg_status_0;

#ifdef VALIDATE_PARMS
    if(!rescal_status_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, RESCAL, STATUS_0, reg_status_0);

    rescal_status_0->valid = RU_FIELD_GET(0, RESCAL, STATUS_0, VALID, reg_status_0);
    rescal_status_0->comp = RU_FIELD_GET(0, RESCAL, STATUS_0, COMP, reg_status_0);
    rescal_status_0->state = RU_FIELD_GET(0, RESCAL, STATUS_0, STATE, reg_status_0);
    rescal_status_0->ctrl_dfs = RU_FIELD_GET(0, RESCAL, STATUS_0, CTRL_DFS, reg_status_0);
    rescal_status_0->prev_comp_cnt = RU_FIELD_GET(0, RESCAL, STATUS_0, PREV_COMP_CNT, reg_status_0);
    rescal_status_0->pon = RU_FIELD_GET(0, RESCAL, STATUS_0, PON, reg_status_0);
    rescal_status_0->done = RU_FIELD_GET(0, RESCAL, STATUS_0, DONE, reg_status_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_rescal_rescal_status1_get(uint8_t *curr_comp_cnt)
{
    uint32_t reg_status1;

#ifdef VALIDATE_PARMS
    if(!curr_comp_cnt)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, RESCAL, STATUS1, reg_status1);

    *curr_comp_cnt = RU_FIELD_GET(0, RESCAL, STATUS1, CURR_COMP_CNT, reg_status1);

    return BDMF_ERR_OK;
}

#ifdef USE_BDMF_SHELL
typedef enum
{
    bdmf_address_cfg,
    bdmf_address_status_0,
    bdmf_address_status1,
}
bdmf_address;

static int bcm_rescal_cli_set(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_rescal_rescal_cfg:
    {
        rescal_rescal_cfg rescal_cfg = { .ctrl=parm[1].value.unumber, .pwrdn=parm[2].value.unumber, .diag_on=parm[3].value.unumber, .rstb=parm[4].value.unumber};
        err = ag_drv_rescal_rescal_cfg_set(&rescal_cfg);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

int bcm_rescal_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_rescal_rescal_cfg:
    {
        rescal_rescal_cfg rescal_cfg;
        err = ag_drv_rescal_rescal_cfg_get(&rescal_cfg);
        bdmf_session_print(session, "ctrl = %u (0x%x)\n", rescal_cfg.ctrl, rescal_cfg.ctrl);
        bdmf_session_print(session, "pwrdn = %u (0x%x)\n", rescal_cfg.pwrdn, rescal_cfg.pwrdn);
        bdmf_session_print(session, "diag_on = %u (0x%x)\n", rescal_cfg.diag_on, rescal_cfg.diag_on);
        bdmf_session_print(session, "rstb = %u (0x%x)\n", rescal_cfg.rstb, rescal_cfg.rstb);
        break;
    }
    case cli_rescal_rescal_status_0:
    {
        rescal_rescal_status_0 rescal_status_0;
        err = ag_drv_rescal_rescal_status_0_get(&rescal_status_0);
        bdmf_session_print(session, "valid = %u (0x%x)\n", rescal_status_0.valid, rescal_status_0.valid);
        bdmf_session_print(session, "comp = %u (0x%x)\n", rescal_status_0.comp, rescal_status_0.comp);
        bdmf_session_print(session, "state = %u (0x%x)\n", rescal_status_0.state, rescal_status_0.state);
        bdmf_session_print(session, "ctrl_dfs = %u (0x%x)\n", rescal_status_0.ctrl_dfs, rescal_status_0.ctrl_dfs);
        bdmf_session_print(session, "prev_comp_cnt = %u (0x%x)\n", rescal_status_0.prev_comp_cnt, rescal_status_0.prev_comp_cnt);
        bdmf_session_print(session, "pon = %u (0x%x)\n", rescal_status_0.pon, rescal_status_0.pon);
        bdmf_session_print(session, "done = %u (0x%x)\n", rescal_status_0.done, rescal_status_0.done);
        break;
    }
    case cli_rescal_rescal_status1:
    {
        uint8_t curr_comp_cnt;
        err = ag_drv_rescal_rescal_status1_get(&curr_comp_cnt);
        bdmf_session_print(session, "curr_comp_cnt = %u (0x%x)\n", curr_comp_cnt, curr_comp_cnt);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

static int bcm_rescal_cli_test(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_test_method m = parm[0].value.unumber;
    bdmf_error_t err = BDMF_ERR_OK;

    {
        rescal_rescal_cfg rescal_cfg = {.ctrl=gtmv(m, 13), .pwrdn=gtmv(m, 1), .diag_on=gtmv(m, 1), .rstb=gtmv(m, 1)};
        if(!err) bdmf_session_print(session, "ag_drv_rescal_rescal_cfg_set( %u %u %u %u)\n", rescal_cfg.ctrl, rescal_cfg.pwrdn, rescal_cfg.diag_on, rescal_cfg.rstb);
        if(!err) ag_drv_rescal_rescal_cfg_set(&rescal_cfg);
        if(!err) ag_drv_rescal_rescal_cfg_get( &rescal_cfg);
        if(!err) bdmf_session_print(session, "ag_drv_rescal_rescal_cfg_get( %u %u %u %u)\n", rescal_cfg.ctrl, rescal_cfg.pwrdn, rescal_cfg.diag_on, rescal_cfg.rstb);
        if(err || rescal_cfg.ctrl!=gtmv(m, 13) || rescal_cfg.pwrdn!=gtmv(m, 1) || rescal_cfg.diag_on!=gtmv(m, 1) || rescal_cfg.rstb!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    {
        rescal_rescal_status_0 rescal_status_0 = {.valid=gtmv(m, 1), .comp=gtmv(m, 1), .state=gtmv(m, 3), .ctrl_dfs=gtmv(m, 13), .prev_comp_cnt=gtmv(m, 4), .pon=gtmv(m, 4), .done=gtmv(m, 1)};
        if(!err) ag_drv_rescal_rescal_status_0_get( &rescal_status_0);
        if(!err) bdmf_session_print(session, "ag_drv_rescal_rescal_status_0_get( %u %u %u %u %u %u %u)\n", rescal_status_0.valid, rescal_status_0.comp, rescal_status_0.state, rescal_status_0.ctrl_dfs, rescal_status_0.prev_comp_cnt, rescal_status_0.pon, rescal_status_0.done);
    }
    {
        uint8_t curr_comp_cnt=gtmv(m, 6);
        if(!err) ag_drv_rescal_rescal_status1_get( &curr_comp_cnt);
        if(!err) bdmf_session_print(session, "ag_drv_rescal_rescal_status1_get( %u)\n", curr_comp_cnt);
    }
    return err;
}

static int bcm_rescal_cli_address(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
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
    case bdmf_address_cfg : reg = &RU_REG(RESCAL, CFG); blk = &RU_BLK(RESCAL); break;
    case bdmf_address_status_0 : reg = &RU_REG(RESCAL, STATUS_0); blk = &RU_BLK(RESCAL); break;
    case bdmf_address_status1 : reg = &RU_REG(RESCAL, STATUS1); blk = &RU_BLK(RESCAL); break;
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

bdmfmon_handle_t ag_drv_rescal_cli_init(bdmfmon_handle_t driver_dir)
{
    bdmfmon_handle_t dir;

    if ((dir = bdmfmon_dir_find(driver_dir, "rescal"))!=NULL)
        return dir;
    dir = bdmfmon_dir_add(driver_dir, "rescal", "rescal", BDMF_ACCESS_ADMIN, NULL);

    {
        static bdmfmon_cmd_parm_t set_rescal_cfg[]={
            BDMFMON_MAKE_PARM("ctrl", "ctrl", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("pwrdn", "pwrdn", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("diag_on", "diag_on", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rstb", "rstb", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="rescal_cfg", .val=cli_rescal_rescal_cfg, .parms=set_rescal_cfg },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "set", "set", bcm_rescal_cli_set,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_cmd_parm_t set_default[]={
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="rescal_cfg", .val=cli_rescal_rescal_cfg, .parms=set_default },
            { .name="rescal_status_0", .val=cli_rescal_rescal_status_0, .parms=set_default },
            { .name="rescal_status1", .val=cli_rescal_rescal_status1, .parms=set_default },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "get", "get", bcm_rescal_cli_get,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_test_method[] = {
            { .name="low" , .val=bdmf_test_method_low },
            { .name="mid" , .val=bdmf_test_method_mid },
            { .name="high" , .val=bdmf_test_method_high },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "test", "test", bcm_rescal_cli_test,
            BDMFMON_MAKE_PARM_ENUM("method", "low: 0000, mid: 1000, high: 1111", enum_table_test_method, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_address[] = {
            { .name="CFG" , .val=bdmf_address_cfg },
            { .name="STATUS_0" , .val=bdmf_address_status_0 },
            { .name="STATUS1" , .val=bdmf_address_status1 },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "address", "address", bcm_rescal_cli_address,
            BDMFMON_MAKE_PARM_ENUM("method", "method", enum_table_address, 0),
            BDMFMON_MAKE_PARM("index2", "onu_id/alloc_id/port_id/etc...", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
    }
    return dir;
}
#endif /* USE_BDMF_SHELL */

