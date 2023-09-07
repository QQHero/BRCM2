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
#include "force_lbe_control_ag.h"

bdmf_error_t ag_drv_force_lbe_control_force_lbe_control_control_set(const force_lbe_control_force_lbe_control_control *force_lbe_control_control)
{
    uint32_t reg_control=0;

#ifdef VALIDATE_PARMS
    if(!force_lbe_control_control)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((force_lbe_control_control->cfg_force_lbe >= _1BITS_MAX_VAL_) ||
       (force_lbe_control_control->cfg_force_lbe_value >= _1BITS_MAX_VAL_) ||
       (force_lbe_control_control->cfg_force_lbe_oe >= _1BITS_MAX_VAL_) ||
       (force_lbe_control_control->cfg_force_lbe_oe_value >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_control = RU_FIELD_SET(0, FORCE_LBE_CONTROL, CONTROL, CFG_FORCE_LBE, reg_control, force_lbe_control_control->cfg_force_lbe);
    reg_control = RU_FIELD_SET(0, FORCE_LBE_CONTROL, CONTROL, CFG_FORCE_LBE_VALUE, reg_control, force_lbe_control_control->cfg_force_lbe_value);
    reg_control = RU_FIELD_SET(0, FORCE_LBE_CONTROL, CONTROL, CFG_FORCE_LBE_OE, reg_control, force_lbe_control_control->cfg_force_lbe_oe);
    reg_control = RU_FIELD_SET(0, FORCE_LBE_CONTROL, CONTROL, CFG_FORCE_LBE_OE_VALUE, reg_control, force_lbe_control_control->cfg_force_lbe_oe_value);

    RU_REG_WRITE(0, FORCE_LBE_CONTROL, CONTROL, reg_control);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_force_lbe_control_force_lbe_control_control_get(force_lbe_control_force_lbe_control_control *force_lbe_control_control)
{
    uint32_t reg_control;

#ifdef VALIDATE_PARMS
    if(!force_lbe_control_control)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, FORCE_LBE_CONTROL, CONTROL, reg_control);

    force_lbe_control_control->cfg_force_lbe = RU_FIELD_GET(0, FORCE_LBE_CONTROL, CONTROL, CFG_FORCE_LBE, reg_control);
    force_lbe_control_control->cfg_force_lbe_value = RU_FIELD_GET(0, FORCE_LBE_CONTROL, CONTROL, CFG_FORCE_LBE_VALUE, reg_control);
    force_lbe_control_control->cfg_force_lbe_oe = RU_FIELD_GET(0, FORCE_LBE_CONTROL, CONTROL, CFG_FORCE_LBE_OE, reg_control);
    force_lbe_control_control->cfg_force_lbe_oe_value = RU_FIELD_GET(0, FORCE_LBE_CONTROL, CONTROL, CFG_FORCE_LBE_OE_VALUE, reg_control);

    return BDMF_ERR_OK;
}

#ifdef USE_BDMF_SHELL
typedef enum
{
    bdmf_address_control,
}
bdmf_address;

static int bcm_force_lbe_control_cli_set(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_force_lbe_control_force_lbe_control_control:
    {
        force_lbe_control_force_lbe_control_control force_lbe_control_control = { .cfg_force_lbe=parm[1].value.unumber, .cfg_force_lbe_value=parm[2].value.unumber, .cfg_force_lbe_oe=parm[3].value.unumber, .cfg_force_lbe_oe_value=parm[4].value.unumber};
        err = ag_drv_force_lbe_control_force_lbe_control_control_set(&force_lbe_control_control);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

int bcm_force_lbe_control_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_force_lbe_control_force_lbe_control_control:
    {
        force_lbe_control_force_lbe_control_control force_lbe_control_control;
        err = ag_drv_force_lbe_control_force_lbe_control_control_get(&force_lbe_control_control);
        bdmf_session_print(session, "cfg_force_lbe = %u (0x%x)\n", force_lbe_control_control.cfg_force_lbe, force_lbe_control_control.cfg_force_lbe);
        bdmf_session_print(session, "cfg_force_lbe_value = %u (0x%x)\n", force_lbe_control_control.cfg_force_lbe_value, force_lbe_control_control.cfg_force_lbe_value);
        bdmf_session_print(session, "cfg_force_lbe_oe = %u (0x%x)\n", force_lbe_control_control.cfg_force_lbe_oe, force_lbe_control_control.cfg_force_lbe_oe);
        bdmf_session_print(session, "cfg_force_lbe_oe_value = %u (0x%x)\n", force_lbe_control_control.cfg_force_lbe_oe_value, force_lbe_control_control.cfg_force_lbe_oe_value);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

static int bcm_force_lbe_control_cli_test(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_test_method m = parm[0].value.unumber;
    bdmf_error_t err = BDMF_ERR_OK;

    {
        force_lbe_control_force_lbe_control_control force_lbe_control_control = {.cfg_force_lbe=gtmv(m, 1), .cfg_force_lbe_value=gtmv(m, 1), .cfg_force_lbe_oe=gtmv(m, 1), .cfg_force_lbe_oe_value=gtmv(m, 1)};
        if(!err) bdmf_session_print(session, "ag_drv_force_lbe_control_force_lbe_control_control_set( %u %u %u %u)\n", force_lbe_control_control.cfg_force_lbe, force_lbe_control_control.cfg_force_lbe_value, force_lbe_control_control.cfg_force_lbe_oe, force_lbe_control_control.cfg_force_lbe_oe_value);
        if(!err) ag_drv_force_lbe_control_force_lbe_control_control_set(&force_lbe_control_control);
        if(!err) ag_drv_force_lbe_control_force_lbe_control_control_get( &force_lbe_control_control);
        if(!err) bdmf_session_print(session, "ag_drv_force_lbe_control_force_lbe_control_control_get( %u %u %u %u)\n", force_lbe_control_control.cfg_force_lbe, force_lbe_control_control.cfg_force_lbe_value, force_lbe_control_control.cfg_force_lbe_oe, force_lbe_control_control.cfg_force_lbe_oe_value);
        if(err || force_lbe_control_control.cfg_force_lbe!=gtmv(m, 1) || force_lbe_control_control.cfg_force_lbe_value!=gtmv(m, 1) || force_lbe_control_control.cfg_force_lbe_oe!=gtmv(m, 1) || force_lbe_control_control.cfg_force_lbe_oe_value!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    return err;
}

static int bcm_force_lbe_control_cli_address(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
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
    case bdmf_address_control : reg = &RU_REG(FORCE_LBE_CONTROL, CONTROL); blk = &RU_BLK(FORCE_LBE_CONTROL); break;
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

bdmfmon_handle_t ag_drv_force_lbe_control_cli_init(bdmfmon_handle_t driver_dir)
{
    bdmfmon_handle_t dir;

    if ((dir = bdmfmon_dir_find(driver_dir, "force_lbe_control"))!=NULL)
        return dir;
    dir = bdmfmon_dir_add(driver_dir, "force_lbe_control", "force_lbe_control", BDMF_ACCESS_ADMIN, NULL);

    {
        static bdmfmon_cmd_parm_t set_force_lbe_control_control[]={
            BDMFMON_MAKE_PARM("cfg_force_lbe", "cfg_force_lbe", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_force_lbe_value", "cfg_force_lbe_value", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_force_lbe_oe", "cfg_force_lbe_oe", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_force_lbe_oe_value", "cfg_force_lbe_oe_value", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="force_lbe_control_control", .val=cli_force_lbe_control_force_lbe_control_control, .parms=set_force_lbe_control_control },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "set", "set", bcm_force_lbe_control_cli_set,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_cmd_parm_t set_default[]={
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="force_lbe_control_control", .val=cli_force_lbe_control_force_lbe_control_control, .parms=set_default },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "get", "get", bcm_force_lbe_control_cli_get,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_test_method[] = {
            { .name="low" , .val=bdmf_test_method_low },
            { .name="mid" , .val=bdmf_test_method_mid },
            { .name="high" , .val=bdmf_test_method_high },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "test", "test", bcm_force_lbe_control_cli_test,
            BDMFMON_MAKE_PARM_ENUM("method", "low: 0000, mid: 1000, high: 1111", enum_table_test_method, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_address[] = {
            { .name="CONTROL" , .val=bdmf_address_control },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "address", "address", bcm_force_lbe_control_cli_address,
            BDMFMON_MAKE_PARM_ENUM("method", "method", enum_table_address, 0),
            BDMFMON_MAKE_PARM("index2", "onu_id/alloc_id/port_id/etc...", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
    }
    return dir;
}
#endif /* USE_BDMF_SHELL */

