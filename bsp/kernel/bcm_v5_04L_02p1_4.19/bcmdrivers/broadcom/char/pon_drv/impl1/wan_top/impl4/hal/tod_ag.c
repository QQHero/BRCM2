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
#include "tod_ag.h"

bdmf_error_t ag_drv_tod_config_0_set(const tod_config_0 *config_0)
{
    uint32_t reg_config_0=0;

#ifdef VALIDATE_PARMS
    if(!config_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((config_0->tod_read_busy >= _1BITS_MAX_VAL_) ||
       (config_0->cfg_ts48_pre_sync_fifo_disable >= _1BITS_MAX_VAL_) ||
       (config_0->cfg_ts48_pre_sync_fifo_load_rate >= _5BITS_MAX_VAL_) ||
       (config_0->cfg_tod_pps_clear >= _1BITS_MAX_VAL_) ||
       (config_0->cfg_ts48_read >= _1BITS_MAX_VAL_) ||
       (config_0->cfg_ts48_offset >= _10BITS_MAX_VAL_) ||
       (config_0->cfg_ts48_mac_select >= _3BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, TOD_READ_BUSY, reg_config_0, config_0->tod_read_busy);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_PRE_SYNC_FIFO_DISABLE, reg_config_0, config_0->cfg_ts48_pre_sync_fifo_disable);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_PRE_SYNC_FIFO_LOAD_RATE, reg_config_0, config_0->cfg_ts48_pre_sync_fifo_load_rate);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TOD_PPS_CLEAR, reg_config_0, config_0->cfg_tod_pps_clear);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_READ, reg_config_0, config_0->cfg_ts48_read);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_OFFSET, reg_config_0, config_0->cfg_ts48_offset);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_MAC_SELECT, reg_config_0, config_0->cfg_ts48_mac_select);

    RU_REG_WRITE(0, TOD, CONFIG_0, reg_config_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_0_get(tod_config_0 *config_0)
{
    uint32_t reg_config_0;

#ifdef VALIDATE_PARMS
    if(!config_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, CONFIG_0, reg_config_0);

    config_0->tod_read_busy = RU_FIELD_GET(0, TOD, CONFIG_0, TOD_READ_BUSY, reg_config_0);
    config_0->cfg_ts48_pre_sync_fifo_disable = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_PRE_SYNC_FIFO_DISABLE, reg_config_0);
    config_0->cfg_ts48_pre_sync_fifo_load_rate = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_PRE_SYNC_FIFO_LOAD_RATE, reg_config_0);
    config_0->cfg_tod_pps_clear = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TOD_PPS_CLEAR, reg_config_0);
    config_0->cfg_ts48_read = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_READ, reg_config_0);
    config_0->cfg_ts48_offset = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_OFFSET, reg_config_0);
    config_0->cfg_ts48_mac_select = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_MAC_SELECT, reg_config_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_1_set(bdmf_boolean cfg_tod_load_ts48_offset, bdmf_boolean cfg_tod_load, uint32_t cfg_tod_seconds)
{
    uint32_t reg_config_1=0;

#ifdef VALIDATE_PARMS
    if((cfg_tod_load_ts48_offset >= _1BITS_MAX_VAL_) ||
       (cfg_tod_load >= _1BITS_MAX_VAL_) ||
       (cfg_tod_seconds >= _19BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_1 = RU_FIELD_SET(0, TOD, CONFIG_1, CFG_TOD_LOAD_TS48_OFFSET, reg_config_1, cfg_tod_load_ts48_offset);
    reg_config_1 = RU_FIELD_SET(0, TOD, CONFIG_1, CFG_TOD_LOAD, reg_config_1, cfg_tod_load);
    reg_config_1 = RU_FIELD_SET(0, TOD, CONFIG_1, CFG_TOD_SECONDS, reg_config_1, cfg_tod_seconds);

    RU_REG_WRITE(0, TOD, CONFIG_1, reg_config_1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_1_get(bdmf_boolean *cfg_tod_load_ts48_offset, bdmf_boolean *cfg_tod_load, uint32_t *cfg_tod_seconds)
{
    uint32_t reg_config_1;

#ifdef VALIDATE_PARMS
    if(!cfg_tod_load_ts48_offset || !cfg_tod_load || !cfg_tod_seconds)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, CONFIG_1, reg_config_1);

    *cfg_tod_load_ts48_offset = RU_FIELD_GET(0, TOD, CONFIG_1, CFG_TOD_LOAD_TS48_OFFSET, reg_config_1);
    *cfg_tod_load = RU_FIELD_GET(0, TOD, CONFIG_1, CFG_TOD_LOAD, reg_config_1);
    *cfg_tod_seconds = RU_FIELD_GET(0, TOD, CONFIG_1, CFG_TOD_SECONDS, reg_config_1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_2_set(uint16_t cfg_tx_offset, uint16_t cfg_rx_offset)
{
    uint32_t reg_config_2=0;

#ifdef VALIDATE_PARMS
    if((cfg_tx_offset >= _10BITS_MAX_VAL_) ||
       (cfg_rx_offset >= _10BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_2 = RU_FIELD_SET(0, TOD, CONFIG_2, CFG_TX_OFFSET, reg_config_2, cfg_tx_offset);
    reg_config_2 = RU_FIELD_SET(0, TOD, CONFIG_2, CFG_RX_OFFSET, reg_config_2, cfg_rx_offset);

    RU_REG_WRITE(0, TOD, CONFIG_2, reg_config_2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_2_get(uint16_t *cfg_tx_offset, uint16_t *cfg_rx_offset)
{
    uint32_t reg_config_2;

#ifdef VALIDATE_PARMS
    if(!cfg_tx_offset || !cfg_rx_offset)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, CONFIG_2, reg_config_2);

    *cfg_tx_offset = RU_FIELD_GET(0, TOD, CONFIG_2, CFG_TX_OFFSET, reg_config_2);
    *cfg_rx_offset = RU_FIELD_GET(0, TOD, CONFIG_2, CFG_RX_OFFSET, reg_config_2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_3_set(uint16_t cfg_ref_offset)
{
    uint32_t reg_config_3=0;

#ifdef VALIDATE_PARMS
    if((cfg_ref_offset >= _10BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_3 = RU_FIELD_SET(0, TOD, CONFIG_3, CFG_REF_OFFSET, reg_config_3, cfg_ref_offset);

    RU_REG_WRITE(0, TOD, CONFIG_3, reg_config_3);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_3_get(uint16_t *cfg_ref_offset)
{
    uint32_t reg_config_3;

#ifdef VALIDATE_PARMS
    if(!cfg_ref_offset)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, CONFIG_3, reg_config_3);

    *cfg_ref_offset = RU_FIELD_GET(0, TOD, CONFIG_3, CFG_REF_OFFSET, reg_config_3);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_4_set(uint32_t cfg_tod_1pps_ns_offset)
{
    uint32_t reg_config_4=0;

#ifdef VALIDATE_PARMS
    if((cfg_tod_1pps_ns_offset >= _30BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_4 = RU_FIELD_SET(0, TOD, CONFIG_4, CFG_TOD_1PPS_NS_OFFSET, reg_config_4, cfg_tod_1pps_ns_offset);

    RU_REG_WRITE(0, TOD, CONFIG_4, reg_config_4);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_4_get(uint32_t *cfg_tod_1pps_ns_offset)
{
    uint32_t reg_config_4;

#ifdef VALIDATE_PARMS
    if(!cfg_tod_1pps_ns_offset)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, CONFIG_4, reg_config_4);

    *cfg_tod_1pps_ns_offset = RU_FIELD_GET(0, TOD, CONFIG_4, CFG_TOD_1PPS_NS_OFFSET, reg_config_4);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_5_set(bdmf_boolean cfg_tod_load_ns_offset, uint32_t cfg_tod_ns_offset)
{
    uint32_t reg_config_5=0;

#ifdef VALIDATE_PARMS
    if((cfg_tod_load_ns_offset >= _1BITS_MAX_VAL_) ||
       (cfg_tod_ns_offset >= _30BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_5 = RU_FIELD_SET(0, TOD, CONFIG_5, CFG_TOD_LOAD_NS_OFFSET, reg_config_5, cfg_tod_load_ns_offset);
    reg_config_5 = RU_FIELD_SET(0, TOD, CONFIG_5, CFG_TOD_NS_OFFSET, reg_config_5, cfg_tod_ns_offset);

    RU_REG_WRITE(0, TOD, CONFIG_5, reg_config_5);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_config_5_get(bdmf_boolean *cfg_tod_load_ns_offset, uint32_t *cfg_tod_ns_offset)
{
    uint32_t reg_config_5;

#ifdef VALIDATE_PARMS
    if(!cfg_tod_load_ns_offset || !cfg_tod_ns_offset)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, CONFIG_5, reg_config_5);

    *cfg_tod_load_ns_offset = RU_FIELD_GET(0, TOD, CONFIG_5, CFG_TOD_LOAD_NS_OFFSET, reg_config_5);
    *cfg_tod_ns_offset = RU_FIELD_GET(0, TOD, CONFIG_5, CFG_TOD_NS_OFFSET, reg_config_5);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_msb_get(uint16_t *ts48_wan_read_msb)
{
    uint32_t reg_msb;

#ifdef VALIDATE_PARMS
    if(!ts48_wan_read_msb)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, MSB, reg_msb);

    *ts48_wan_read_msb = RU_FIELD_GET(0, TOD, MSB, TS48_WAN_READ_MSB, reg_msb);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_lsb_get(uint32_t *ts48_wan_read_lsb)
{
    uint32_t reg_lsb;

#ifdef VALIDATE_PARMS
    if(!ts48_wan_read_lsb)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, LSB, reg_lsb);

    *ts48_wan_read_lsb = RU_FIELD_GET(0, TOD, LSB, TS48_WAN_READ_LSB, reg_lsb);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_ts64_msb_get(uint32_t *ts64_wan_read_msb)
{
    uint32_t reg_ts64_msb;

#ifdef VALIDATE_PARMS
    if(!ts64_wan_read_msb)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, TS64_MSB, reg_ts64_msb);

    *ts64_wan_read_msb = RU_FIELD_GET(0, TOD, TS64_MSB, TS64_WAN_READ_MSB, reg_ts64_msb);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_ts64_lsb_get(uint32_t *ts64_wan_read_lsb)
{
    uint32_t reg_ts64_lsb;

#ifdef VALIDATE_PARMS
    if(!ts64_wan_read_lsb)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, TS64_LSB, reg_ts64_lsb);

    *ts64_wan_read_lsb = RU_FIELD_GET(0, TOD, TS64_LSB, TS64_WAN_READ_LSB, reg_ts64_lsb);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_status_0_get(uint16_t *ts16_ref_synce_read)
{
    uint32_t reg_status_0;

#ifdef VALIDATE_PARMS
    if(!ts16_ref_synce_read)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, STATUS_0, reg_status_0);

    *ts16_ref_synce_read = RU_FIELD_GET(0, TOD, STATUS_0, TS16_REF_SYNCE_READ, reg_status_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_status_1_get(uint16_t *ts16_mac_tx_read, uint16_t *ts16_mac_rx_read)
{
    uint32_t reg_status_1;

#ifdef VALIDATE_PARMS
    if(!ts16_mac_tx_read || !ts16_mac_rx_read)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, STATUS_1, reg_status_1);

    *ts16_mac_tx_read = RU_FIELD_GET(0, TOD, STATUS_1, TS16_MAC_TX_READ, reg_status_1);
    *ts16_mac_rx_read = RU_FIELD_GET(0, TOD, STATUS_1, TS16_MAC_RX_READ, reg_status_1);

    return BDMF_ERR_OK;
}

#ifdef USE_BDMF_SHELL
typedef enum
{
    bdmf_address_config_0,
    bdmf_address_config_1,
    bdmf_address_config_2,
    bdmf_address_config_3,
    bdmf_address_config_4,
    bdmf_address_config_5,
    bdmf_address_msb,
    bdmf_address_lsb,
    bdmf_address_ts64_msb,
    bdmf_address_ts64_lsb,
    bdmf_address_status_0,
    bdmf_address_status_1,
}
bdmf_address;

static int bcm_tod_cli_set(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_tod_config_0:
    {
        tod_config_0 config_0 = { .tod_read_busy=parm[1].value.unumber, .cfg_ts48_pre_sync_fifo_disable=parm[2].value.unumber, .cfg_ts48_pre_sync_fifo_load_rate=parm[3].value.unumber, .cfg_tod_pps_clear=parm[4].value.unumber, .cfg_ts48_read=parm[5].value.unumber, .cfg_ts48_offset=parm[6].value.unumber, .cfg_ts48_mac_select=parm[7].value.unumber};
        err = ag_drv_tod_config_0_set(&config_0);
        break;
    }
    case cli_tod_config_1:
        err = ag_drv_tod_config_1_set(parm[1].value.unumber, parm[2].value.unumber, parm[3].value.unumber);
        break;
    case cli_tod_config_2:
        err = ag_drv_tod_config_2_set(parm[1].value.unumber, parm[2].value.unumber);
        break;
    case cli_tod_config_3:
        err = ag_drv_tod_config_3_set(parm[1].value.unumber);
        break;
    case cli_tod_config_4:
        err = ag_drv_tod_config_4_set(parm[1].value.unumber);
        break;
    case cli_tod_config_5:
        err = ag_drv_tod_config_5_set(parm[1].value.unumber, parm[2].value.unumber);
        break;
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

int bcm_tod_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_tod_config_0:
    {
        tod_config_0 config_0;
        err = ag_drv_tod_config_0_get(&config_0);
        bdmf_session_print(session, "tod_read_busy = %u (0x%x)\n", config_0.tod_read_busy, config_0.tod_read_busy);
        bdmf_session_print(session, "cfg_ts48_pre_sync_fifo_disable = %u (0x%x)\n", config_0.cfg_ts48_pre_sync_fifo_disable, config_0.cfg_ts48_pre_sync_fifo_disable);
        bdmf_session_print(session, "cfg_ts48_pre_sync_fifo_load_rate = %u (0x%x)\n", config_0.cfg_ts48_pre_sync_fifo_load_rate, config_0.cfg_ts48_pre_sync_fifo_load_rate);
        bdmf_session_print(session, "cfg_tod_pps_clear = %u (0x%x)\n", config_0.cfg_tod_pps_clear, config_0.cfg_tod_pps_clear);
        bdmf_session_print(session, "cfg_ts48_read = %u (0x%x)\n", config_0.cfg_ts48_read, config_0.cfg_ts48_read);
        bdmf_session_print(session, "cfg_ts48_offset = %u (0x%x)\n", config_0.cfg_ts48_offset, config_0.cfg_ts48_offset);
        bdmf_session_print(session, "cfg_ts48_mac_select = %u (0x%x)\n", config_0.cfg_ts48_mac_select, config_0.cfg_ts48_mac_select);
        break;
    }
    case cli_tod_config_1:
    {
        bdmf_boolean cfg_tod_load_ts48_offset;
        bdmf_boolean cfg_tod_load;
        uint32_t cfg_tod_seconds;
        err = ag_drv_tod_config_1_get(&cfg_tod_load_ts48_offset, &cfg_tod_load, &cfg_tod_seconds);
        bdmf_session_print(session, "cfg_tod_load_ts48_offset = %u (0x%x)\n", cfg_tod_load_ts48_offset, cfg_tod_load_ts48_offset);
        bdmf_session_print(session, "cfg_tod_load = %u (0x%x)\n", cfg_tod_load, cfg_tod_load);
        bdmf_session_print(session, "cfg_tod_seconds = %u (0x%x)\n", cfg_tod_seconds, cfg_tod_seconds);
        break;
    }
    case cli_tod_config_2:
    {
        uint16_t cfg_tx_offset;
        uint16_t cfg_rx_offset;
        err = ag_drv_tod_config_2_get(&cfg_tx_offset, &cfg_rx_offset);
        bdmf_session_print(session, "cfg_tx_offset = %u (0x%x)\n", cfg_tx_offset, cfg_tx_offset);
        bdmf_session_print(session, "cfg_rx_offset = %u (0x%x)\n", cfg_rx_offset, cfg_rx_offset);
        break;
    }
    case cli_tod_config_3:
    {
        uint16_t cfg_ref_offset;
        err = ag_drv_tod_config_3_get(&cfg_ref_offset);
        bdmf_session_print(session, "cfg_ref_offset = %u (0x%x)\n", cfg_ref_offset, cfg_ref_offset);
        break;
    }
    case cli_tod_config_4:
    {
        uint32_t cfg_tod_1pps_ns_offset;
        err = ag_drv_tod_config_4_get(&cfg_tod_1pps_ns_offset);
        bdmf_session_print(session, "cfg_tod_1pps_ns_offset = %u (0x%x)\n", cfg_tod_1pps_ns_offset, cfg_tod_1pps_ns_offset);
        break;
    }
    case cli_tod_config_5:
    {
        bdmf_boolean cfg_tod_load_ns_offset;
        uint32_t cfg_tod_ns_offset;
        err = ag_drv_tod_config_5_get(&cfg_tod_load_ns_offset, &cfg_tod_ns_offset);
        bdmf_session_print(session, "cfg_tod_load_ns_offset = %u (0x%x)\n", cfg_tod_load_ns_offset, cfg_tod_load_ns_offset);
        bdmf_session_print(session, "cfg_tod_ns_offset = %u (0x%x)\n", cfg_tod_ns_offset, cfg_tod_ns_offset);
        break;
    }
    case cli_tod_msb:
    {
        uint16_t ts48_wan_read_msb;
        err = ag_drv_tod_msb_get(&ts48_wan_read_msb);
        bdmf_session_print(session, "ts48_wan_read_msb = %u (0x%x)\n", ts48_wan_read_msb, ts48_wan_read_msb);
        break;
    }
    case cli_tod_lsb:
    {
        uint32_t ts48_wan_read_lsb;
        err = ag_drv_tod_lsb_get(&ts48_wan_read_lsb);
        bdmf_session_print(session, "ts48_wan_read_lsb = %u (0x%x)\n", ts48_wan_read_lsb, ts48_wan_read_lsb);
        break;
    }
    case cli_tod_ts64_msb:
    {
        uint32_t ts64_wan_read_msb;
        err = ag_drv_tod_ts64_msb_get(&ts64_wan_read_msb);
        bdmf_session_print(session, "ts64_wan_read_msb = %u (0x%x)\n", ts64_wan_read_msb, ts64_wan_read_msb);
        break;
    }
    case cli_tod_ts64_lsb:
    {
        uint32_t ts64_wan_read_lsb;
        err = ag_drv_tod_ts64_lsb_get(&ts64_wan_read_lsb);
        bdmf_session_print(session, "ts64_wan_read_lsb = %u (0x%x)\n", ts64_wan_read_lsb, ts64_wan_read_lsb);
        break;
    }
    case cli_tod_status_0:
    {
        uint16_t ts16_ref_synce_read;
        err = ag_drv_tod_status_0_get(&ts16_ref_synce_read);
        bdmf_session_print(session, "ts16_ref_synce_read = %u (0x%x)\n", ts16_ref_synce_read, ts16_ref_synce_read);
        break;
    }
    case cli_tod_status_1:
    {
        uint16_t ts16_mac_tx_read;
        uint16_t ts16_mac_rx_read;
        err = ag_drv_tod_status_1_get(&ts16_mac_tx_read, &ts16_mac_rx_read);
        bdmf_session_print(session, "ts16_mac_tx_read = %u (0x%x)\n", ts16_mac_tx_read, ts16_mac_tx_read);
        bdmf_session_print(session, "ts16_mac_rx_read = %u (0x%x)\n", ts16_mac_rx_read, ts16_mac_rx_read);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

static int bcm_tod_cli_test(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_test_method m = parm[0].value.unumber;
    bdmf_error_t err = BDMF_ERR_OK;

    {
        tod_config_0 config_0 = {.tod_read_busy=gtmv(m, 1), .cfg_ts48_pre_sync_fifo_disable=gtmv(m, 1), .cfg_ts48_pre_sync_fifo_load_rate=gtmv(m, 5), .cfg_tod_pps_clear=gtmv(m, 1), .cfg_ts48_read=gtmv(m, 1), .cfg_ts48_offset=gtmv(m, 10), .cfg_ts48_mac_select=gtmv(m, 3)};
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_0_set( %u %u %u %u %u %u %u)\n", config_0.tod_read_busy, config_0.cfg_ts48_pre_sync_fifo_disable, config_0.cfg_ts48_pre_sync_fifo_load_rate, config_0.cfg_tod_pps_clear, config_0.cfg_ts48_read, config_0.cfg_ts48_offset, config_0.cfg_ts48_mac_select);
        if(!err) ag_drv_tod_config_0_set(&config_0);
        if(!err) ag_drv_tod_config_0_get( &config_0);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_0_get( %u %u %u %u %u %u %u)\n", config_0.tod_read_busy, config_0.cfg_ts48_pre_sync_fifo_disable, config_0.cfg_ts48_pre_sync_fifo_load_rate, config_0.cfg_tod_pps_clear, config_0.cfg_ts48_read, config_0.cfg_ts48_offset, config_0.cfg_ts48_mac_select);
        if(err || config_0.tod_read_busy!=gtmv(m, 1) || config_0.cfg_ts48_pre_sync_fifo_disable!=gtmv(m, 1) || config_0.cfg_ts48_pre_sync_fifo_load_rate!=gtmv(m, 5) || config_0.cfg_tod_pps_clear!=gtmv(m, 1) || config_0.cfg_ts48_read!=gtmv(m, 1) || config_0.cfg_ts48_offset!=gtmv(m, 10) || config_0.cfg_ts48_mac_select!=gtmv(m, 3))
            return err ? err : BDMF_ERR_IO;
    }
    {
        bdmf_boolean cfg_tod_load_ts48_offset=gtmv(m, 1);
        bdmf_boolean cfg_tod_load=gtmv(m, 1);
        uint32_t cfg_tod_seconds=gtmv(m, 19);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_1_set( %u %u %u)\n", cfg_tod_load_ts48_offset, cfg_tod_load, cfg_tod_seconds);
        if(!err) ag_drv_tod_config_1_set(cfg_tod_load_ts48_offset, cfg_tod_load, cfg_tod_seconds);
        if(!err) ag_drv_tod_config_1_get( &cfg_tod_load_ts48_offset, &cfg_tod_load, &cfg_tod_seconds);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_1_get( %u %u %u)\n", cfg_tod_load_ts48_offset, cfg_tod_load, cfg_tod_seconds);
        if(err || cfg_tod_load_ts48_offset!=gtmv(m, 1) || cfg_tod_load!=gtmv(m, 1) || cfg_tod_seconds!=gtmv(m, 19))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint16_t cfg_tx_offset=gtmv(m, 10);
        uint16_t cfg_rx_offset=gtmv(m, 10);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_2_set( %u %u)\n", cfg_tx_offset, cfg_rx_offset);
        if(!err) ag_drv_tod_config_2_set(cfg_tx_offset, cfg_rx_offset);
        if(!err) ag_drv_tod_config_2_get( &cfg_tx_offset, &cfg_rx_offset);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_2_get( %u %u)\n", cfg_tx_offset, cfg_rx_offset);
        if(err || cfg_tx_offset!=gtmv(m, 10) || cfg_rx_offset!=gtmv(m, 10))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint16_t cfg_ref_offset=gtmv(m, 10);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_3_set( %u)\n", cfg_ref_offset);
        if(!err) ag_drv_tod_config_3_set(cfg_ref_offset);
        if(!err) ag_drv_tod_config_3_get( &cfg_ref_offset);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_3_get( %u)\n", cfg_ref_offset);
        if(err || cfg_ref_offset!=gtmv(m, 10))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint32_t cfg_tod_1pps_ns_offset=gtmv(m, 30);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_4_set( %u)\n", cfg_tod_1pps_ns_offset);
        if(!err) ag_drv_tod_config_4_set(cfg_tod_1pps_ns_offset);
        if(!err) ag_drv_tod_config_4_get( &cfg_tod_1pps_ns_offset);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_4_get( %u)\n", cfg_tod_1pps_ns_offset);
        if(err || cfg_tod_1pps_ns_offset!=gtmv(m, 30))
            return err ? err : BDMF_ERR_IO;
    }
    {
        bdmf_boolean cfg_tod_load_ns_offset=gtmv(m, 1);
        uint32_t cfg_tod_ns_offset=gtmv(m, 30);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_5_set( %u %u)\n", cfg_tod_load_ns_offset, cfg_tod_ns_offset);
        if(!err) ag_drv_tod_config_5_set(cfg_tod_load_ns_offset, cfg_tod_ns_offset);
        if(!err) ag_drv_tod_config_5_get( &cfg_tod_load_ns_offset, &cfg_tod_ns_offset);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_5_get( %u %u)\n", cfg_tod_load_ns_offset, cfg_tod_ns_offset);
        if(err || cfg_tod_load_ns_offset!=gtmv(m, 1) || cfg_tod_ns_offset!=gtmv(m, 30))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint16_t ts48_wan_read_msb=gtmv(m, 16);
        if(!err) ag_drv_tod_msb_get( &ts48_wan_read_msb);
        if(!err) bdmf_session_print(session, "ag_drv_tod_msb_get( %u)\n", ts48_wan_read_msb);
    }
    {
        uint32_t ts48_wan_read_lsb=gtmv(m, 32);
        if(!err) ag_drv_tod_lsb_get( &ts48_wan_read_lsb);
        if(!err) bdmf_session_print(session, "ag_drv_tod_lsb_get( %u)\n", ts48_wan_read_lsb);
    }
    {
        uint32_t ts64_wan_read_msb=gtmv(m, 32);
        if(!err) ag_drv_tod_ts64_msb_get( &ts64_wan_read_msb);
        if(!err) bdmf_session_print(session, "ag_drv_tod_ts64_msb_get( %u)\n", ts64_wan_read_msb);
    }
    {
        uint32_t ts64_wan_read_lsb=gtmv(m, 32);
        if(!err) ag_drv_tod_ts64_lsb_get( &ts64_wan_read_lsb);
        if(!err) bdmf_session_print(session, "ag_drv_tod_ts64_lsb_get( %u)\n", ts64_wan_read_lsb);
    }
    {
        uint16_t ts16_ref_synce_read=gtmv(m, 16);
        if(!err) ag_drv_tod_status_0_get( &ts16_ref_synce_read);
        if(!err) bdmf_session_print(session, "ag_drv_tod_status_0_get( %u)\n", ts16_ref_synce_read);
    }
    {
        uint16_t ts16_mac_tx_read=gtmv(m, 16);
        uint16_t ts16_mac_rx_read=gtmv(m, 16);
        if(!err) ag_drv_tod_status_1_get( &ts16_mac_tx_read, &ts16_mac_rx_read);
        if(!err) bdmf_session_print(session, "ag_drv_tod_status_1_get( %u %u)\n", ts16_mac_tx_read, ts16_mac_rx_read);
    }
    return err;
}

static int bcm_tod_cli_address(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
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
    case bdmf_address_config_0 : reg = &RU_REG(TOD, CONFIG_0); blk = &RU_BLK(TOD); break;
    case bdmf_address_config_1 : reg = &RU_REG(TOD, CONFIG_1); blk = &RU_BLK(TOD); break;
    case bdmf_address_config_2 : reg = &RU_REG(TOD, CONFIG_2); blk = &RU_BLK(TOD); break;
    case bdmf_address_config_3 : reg = &RU_REG(TOD, CONFIG_3); blk = &RU_BLK(TOD); break;
    case bdmf_address_config_4 : reg = &RU_REG(TOD, CONFIG_4); blk = &RU_BLK(TOD); break;
    case bdmf_address_config_5 : reg = &RU_REG(TOD, CONFIG_5); blk = &RU_BLK(TOD); break;
    case bdmf_address_msb : reg = &RU_REG(TOD, MSB); blk = &RU_BLK(TOD); break;
    case bdmf_address_lsb : reg = &RU_REG(TOD, LSB); blk = &RU_BLK(TOD); break;
    case bdmf_address_ts64_msb : reg = &RU_REG(TOD, TS64_MSB); blk = &RU_BLK(TOD); break;
    case bdmf_address_ts64_lsb : reg = &RU_REG(TOD, TS64_LSB); blk = &RU_BLK(TOD); break;
    case bdmf_address_status_0 : reg = &RU_REG(TOD, STATUS_0); blk = &RU_BLK(TOD); break;
    case bdmf_address_status_1 : reg = &RU_REG(TOD, STATUS_1); blk = &RU_BLK(TOD); break;
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

bdmfmon_handle_t ag_drv_tod_cli_init(bdmfmon_handle_t driver_dir)
{
    bdmfmon_handle_t dir;

    if ((dir = bdmfmon_dir_find(driver_dir, "tod"))!=NULL)
        return dir;
    dir = bdmfmon_dir_add(driver_dir, "tod", "tod", BDMF_ACCESS_ADMIN, NULL);

    {
        static bdmfmon_cmd_parm_t set_config_0[]={
            BDMFMON_MAKE_PARM("tod_read_busy", "tod_read_busy", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_ts48_pre_sync_fifo_disable", "cfg_ts48_pre_sync_fifo_disable", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_ts48_pre_sync_fifo_load_rate", "cfg_ts48_pre_sync_fifo_load_rate", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_tod_pps_clear", "cfg_tod_pps_clear", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_ts48_read", "cfg_ts48_read", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_ts48_offset", "cfg_ts48_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_ts48_mac_select", "cfg_ts48_mac_select", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_config_1[]={
            BDMFMON_MAKE_PARM("cfg_tod_load_ts48_offset", "cfg_tod_load_ts48_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_tod_load", "cfg_tod_load", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_tod_seconds", "cfg_tod_seconds", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_config_2[]={
            BDMFMON_MAKE_PARM("cfg_tx_offset", "cfg_tx_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_rx_offset", "cfg_rx_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_config_3[]={
            BDMFMON_MAKE_PARM("cfg_ref_offset", "cfg_ref_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_config_4[]={
            BDMFMON_MAKE_PARM("cfg_tod_1pps_ns_offset", "cfg_tod_1pps_ns_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_config_5[]={
            BDMFMON_MAKE_PARM("cfg_tod_load_ns_offset", "cfg_tod_load_ns_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_tod_ns_offset", "cfg_tod_ns_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="config_0", .val=cli_tod_config_0, .parms=set_config_0 },
            { .name="config_1", .val=cli_tod_config_1, .parms=set_config_1 },
            { .name="config_2", .val=cli_tod_config_2, .parms=set_config_2 },
            { .name="config_3", .val=cli_tod_config_3, .parms=set_config_3 },
            { .name="config_4", .val=cli_tod_config_4, .parms=set_config_4 },
            { .name="config_5", .val=cli_tod_config_5, .parms=set_config_5 },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "set", "set", bcm_tod_cli_set,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_cmd_parm_t set_default[]={
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="config_0", .val=cli_tod_config_0, .parms=set_default },
            { .name="config_1", .val=cli_tod_config_1, .parms=set_default },
            { .name="config_2", .val=cli_tod_config_2, .parms=set_default },
            { .name="config_3", .val=cli_tod_config_3, .parms=set_default },
            { .name="config_4", .val=cli_tod_config_4, .parms=set_default },
            { .name="config_5", .val=cli_tod_config_5, .parms=set_default },
            { .name="msb", .val=cli_tod_msb, .parms=set_default },
            { .name="lsb", .val=cli_tod_lsb, .parms=set_default },
            { .name="ts64_msb", .val=cli_tod_ts64_msb, .parms=set_default },
            { .name="ts64_lsb", .val=cli_tod_ts64_lsb, .parms=set_default },
            { .name="status_0", .val=cli_tod_status_0, .parms=set_default },
            { .name="status_1", .val=cli_tod_status_1, .parms=set_default },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "get", "get", bcm_tod_cli_get,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_test_method[] = {
            { .name="low" , .val=bdmf_test_method_low },
            { .name="mid" , .val=bdmf_test_method_mid },
            { .name="high" , .val=bdmf_test_method_high },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "test", "test", bcm_tod_cli_test,
            BDMFMON_MAKE_PARM_ENUM("method", "low: 0000, mid: 1000, high: 1111", enum_table_test_method, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_address[] = {
            { .name="CONFIG_0" , .val=bdmf_address_config_0 },
            { .name="CONFIG_1" , .val=bdmf_address_config_1 },
            { .name="CONFIG_2" , .val=bdmf_address_config_2 },
            { .name="CONFIG_3" , .val=bdmf_address_config_3 },
            { .name="CONFIG_4" , .val=bdmf_address_config_4 },
            { .name="CONFIG_5" , .val=bdmf_address_config_5 },
            { .name="MSB" , .val=bdmf_address_msb },
            { .name="LSB" , .val=bdmf_address_lsb },
            { .name="TS64_MSB" , .val=bdmf_address_ts64_msb },
            { .name="TS64_LSB" , .val=bdmf_address_ts64_lsb },
            { .name="STATUS_0" , .val=bdmf_address_status_0 },
            { .name="STATUS_1" , .val=bdmf_address_status_1 },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "address", "address", bcm_tod_cli_address,
            BDMFMON_MAKE_PARM_ENUM("method", "method", enum_table_address, 0),
            BDMFMON_MAKE_PARM("index2", "onu_id/alloc_id/port_id/etc...", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
    }
    return dir;
}
#endif /* USE_BDMF_SHELL */

