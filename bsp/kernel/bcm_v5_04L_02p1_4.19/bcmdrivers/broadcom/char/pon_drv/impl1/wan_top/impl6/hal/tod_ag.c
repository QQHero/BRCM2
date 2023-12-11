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
    if((config_0->cfg_ts48_mac_select >= _3BITS_MAX_VAL_) ||
       (config_0->cfg_ts48_enable >= _1BITS_MAX_VAL_) ||
       (config_0->cfg_ts48_offset >= _10BITS_MAX_VAL_) ||
       (config_0->cfg_ts48_read >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_MAC_SELECT, reg_config_0, config_0->cfg_ts48_mac_select);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_ENABLE, reg_config_0, config_0->cfg_ts48_enable);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_OFFSET, reg_config_0, config_0->cfg_ts48_offset);
    reg_config_0 = RU_FIELD_SET(0, TOD, CONFIG_0, CFG_TS48_READ, reg_config_0, config_0->cfg_ts48_read);

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

    config_0->cfg_ts48_mac_select = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_MAC_SELECT, reg_config_0);
    config_0->cfg_ts48_enable = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_ENABLE, reg_config_0);
    config_0->cfg_ts48_offset = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_OFFSET, reg_config_0);
    config_0->cfg_ts48_read = RU_FIELD_GET(0, TOD, CONFIG_0, CFG_TS48_READ, reg_config_0);

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

bdmf_error_t ag_drv_tod_tod_config_2_set(uint16_t tx_offset, uint16_t rx_offset)
{
    uint32_t reg_config_2=0;

#ifdef VALIDATE_PARMS
    if((rx_offset >= _10BITS_MAX_VAL_) ||
       (tx_offset >= _10BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_2 = RU_FIELD_SET(0, TOD, CONFIG_2, RX_OFFSET, reg_config_2, rx_offset);
    reg_config_2 = RU_FIELD_SET(0, TOD, CONFIG_2, TX_OFFSET, reg_config_2, tx_offset);

    RU_REG_WRITE(0, TOD, CONFIG_2, reg_config_2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_tod_config_2_get(uint16_t *tx_offset, uint16_t *rx_offset)
{
    uint32_t reg_config_2;

#ifdef VALIDATE_PARMS
    if(!rx_offset || !tx_offset)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, CONFIG_2, reg_config_2);

    *rx_offset = RU_FIELD_GET(0, TOD, CONFIG_2, RX_OFFSET, reg_config_2);
    *tx_offset = RU_FIELD_GET(0, TOD, CONFIG_2, TX_OFFSET, reg_config_2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_tod_config_3_set(bdmf_boolean ts48_fifo_dis, uint8_t ts48_fifo_ld_rate, uint16_t ref_offset)
{
    uint32_t reg_config_3=0;

#ifdef VALIDATE_PARMS
    if((ref_offset >= _10BITS_MAX_VAL_) ||
       (ts48_fifo_ld_rate >= _5BITS_MAX_VAL_) ||
       (ts48_fifo_dis >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_config_3 = RU_FIELD_SET(0, TOD, CONFIG_3, REF_OFFSET, reg_config_3, ref_offset);
    reg_config_3 = RU_FIELD_SET(0, TOD, CONFIG_3, TS48_FIFO_LD_RATE, reg_config_3, ts48_fifo_ld_rate);
    reg_config_3 = RU_FIELD_SET(0, TOD, CONFIG_3, TS48_FIFO_DIS, reg_config_3, ts48_fifo_dis);

    RU_REG_WRITE(0, TOD, CONFIG_3, reg_config_3);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_tod_config_3_get(bdmf_boolean *ts48_fifo_dis, uint8_t *ts48_fifo_ld_rate, uint16_t *ref_offset)
{
    uint32_t reg_config_3;

#ifdef VALIDATE_PARMS
    if(!ref_offset || !ts48_fifo_ld_rate || !ts48_fifo_dis)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, CONFIG_3, reg_config_3);

    *ref_offset = RU_FIELD_GET(0, TOD, CONFIG_3, REF_OFFSET, reg_config_3);
    *ts48_fifo_ld_rate = RU_FIELD_GET(0, TOD, CONFIG_3, TS48_FIFO_LD_RATE, reg_config_3);
    *ts48_fifo_dis = RU_FIELD_GET(0, TOD, CONFIG_3, TS48_FIFO_DIS, reg_config_3);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_tod_status_0_get(uint16_t *ts16_tx_read, uint16_t *ts16_rx_read)
{
    uint32_t reg_status_0;

#ifdef VALIDATE_PARMS
    if(!ts16_rx_read || !ts16_tx_read)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, STATUS_0, reg_status_0);

    *ts16_rx_read = RU_FIELD_GET(0, TOD, STATUS_0, TS16_RX_READ, reg_status_0);
    *ts16_tx_read = RU_FIELD_GET(0, TOD, STATUS_0, TS16_TX_READ, reg_status_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_tod_status_1_get(uint16_t *ts16_rx_synce_read, uint16_t *ts16_ref_synce_read)
{
    uint32_t reg_status_1;

#ifdef VALIDATE_PARMS
    if(!ts16_ref_synce_read || !ts16_rx_synce_read)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, STATUS_1, reg_status_1);

    *ts16_ref_synce_read = RU_FIELD_GET(0, TOD, STATUS_1, TS16_REF_SYNCE_READ, reg_status_1);
    *ts16_rx_synce_read = RU_FIELD_GET(0, TOD, STATUS_1, TS16_RX_SYNCE_READ, reg_status_1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_tod_status_2_get(uint16_t *ts16_mac_tx_read, uint16_t *ts16_mac_rx_read)
{
    uint32_t reg_status_2;

#ifdef VALIDATE_PARMS
    if(!ts16_mac_rx_read || !ts16_mac_tx_read)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, STATUS_2, reg_status_2);

    *ts16_mac_rx_read = RU_FIELD_GET(0, TOD, STATUS_2, TS16_MAC_RX_READ, reg_status_2);
    *ts16_mac_tx_read = RU_FIELD_GET(0, TOD, STATUS_2, TS16_MAC_TX_READ, reg_status_2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_tod_fifo_status_set(const tod_tod_fifo_status *tod_fifo_status)
{
    uint32_t reg_fifo_status=0;

#ifdef VALIDATE_PARMS
    if(!tod_fifo_status)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((tod_fifo_status->rempty_gpon >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_epon >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_ngpon >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_10g_epon >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_ae >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_rx >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_tx >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_ref >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_mac_rx >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->rempty_mac_tx >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_gpon >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_epon >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_ngpon >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_10g_epon >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_ae >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_rx >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_tx >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_ref >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_mac_rx >= _1BITS_MAX_VAL_) ||
       (tod_fifo_status->wfull_mac_tx >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_GPON, reg_fifo_status, tod_fifo_status->rempty_gpon);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_EPON, reg_fifo_status, tod_fifo_status->rempty_epon);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_NGPON, reg_fifo_status, tod_fifo_status->rempty_ngpon);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_10G_EPON, reg_fifo_status, tod_fifo_status->rempty_10g_epon);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_AE, reg_fifo_status, tod_fifo_status->rempty_ae);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_RX, reg_fifo_status, tod_fifo_status->rempty_rx);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_TX, reg_fifo_status, tod_fifo_status->rempty_tx);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_REF, reg_fifo_status, tod_fifo_status->rempty_ref);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_MAC_RX, reg_fifo_status, tod_fifo_status->rempty_mac_rx);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, REMPTY_MAC_TX, reg_fifo_status, tod_fifo_status->rempty_mac_tx);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_GPON, reg_fifo_status, tod_fifo_status->wfull_gpon);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_EPON, reg_fifo_status, tod_fifo_status->wfull_epon);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_NGPON, reg_fifo_status, tod_fifo_status->wfull_ngpon);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_10G_EPON, reg_fifo_status, tod_fifo_status->wfull_10g_epon);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_AE, reg_fifo_status, tod_fifo_status->wfull_ae);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_RX, reg_fifo_status, tod_fifo_status->wfull_rx);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_TX, reg_fifo_status, tod_fifo_status->wfull_tx);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_REF, reg_fifo_status, tod_fifo_status->wfull_ref);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_MAC_RX, reg_fifo_status, tod_fifo_status->wfull_mac_rx);
    reg_fifo_status = RU_FIELD_SET(0, TOD, FIFO_STATUS, WFULL_MAC_TX, reg_fifo_status, tod_fifo_status->wfull_mac_tx);

    RU_REG_WRITE(0, TOD, FIFO_STATUS, reg_fifo_status);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_tod_tod_fifo_status_get(tod_tod_fifo_status *tod_fifo_status)
{
    uint32_t reg_fifo_status;

#ifdef VALIDATE_PARMS
    if(!tod_fifo_status)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, TOD, FIFO_STATUS, reg_fifo_status);

    tod_fifo_status->rempty_gpon = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_GPON, reg_fifo_status);
    tod_fifo_status->rempty_epon = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_EPON, reg_fifo_status);
    tod_fifo_status->rempty_ngpon = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_NGPON, reg_fifo_status);
    tod_fifo_status->rempty_10g_epon = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_10G_EPON, reg_fifo_status);
    tod_fifo_status->rempty_ae = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_AE, reg_fifo_status);
    tod_fifo_status->rempty_rx = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_RX, reg_fifo_status);
    tod_fifo_status->rempty_tx = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_TX, reg_fifo_status);
    tod_fifo_status->rempty_ref = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_REF, reg_fifo_status);
    tod_fifo_status->rempty_mac_rx = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_MAC_RX, reg_fifo_status);
    tod_fifo_status->rempty_mac_tx = RU_FIELD_GET(0, TOD, FIFO_STATUS, REMPTY_MAC_TX, reg_fifo_status);
    tod_fifo_status->wfull_gpon = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_GPON, reg_fifo_status);
    tod_fifo_status->wfull_epon = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_EPON, reg_fifo_status);
    tod_fifo_status->wfull_ngpon = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_NGPON, reg_fifo_status);
    tod_fifo_status->wfull_10g_epon = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_10G_EPON, reg_fifo_status);
    tod_fifo_status->wfull_ae = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_AE, reg_fifo_status);
    tod_fifo_status->wfull_rx = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_RX, reg_fifo_status);
    tod_fifo_status->wfull_tx = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_TX, reg_fifo_status);
    tod_fifo_status->wfull_ref = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_REF, reg_fifo_status);
    tod_fifo_status->wfull_mac_rx = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_MAC_RX, reg_fifo_status);
    tod_fifo_status->wfull_mac_tx = RU_FIELD_GET(0, TOD, FIFO_STATUS, WFULL_MAC_TX, reg_fifo_status);

    return BDMF_ERR_OK;
}

#ifdef USE_BDMF_SHELL
typedef enum
{
    bdmf_address_config_0,
    bdmf_address_msb,
    bdmf_address_lsb,
    bdmf_address_config_2,
    bdmf_address_config_3,
    bdmf_address_status_0,
    bdmf_address_status_1,
    bdmf_address_status_2,
    bdmf_address_fifo_status,
}
bdmf_address;

static int bcm_tod_cli_set(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_tod_config_0:
    {
        tod_config_0 config_0 = { .cfg_ts48_mac_select=parm[1].value.unumber, .cfg_ts48_enable=parm[2].value.unumber, .cfg_ts48_offset=parm[3].value.unumber, .cfg_ts48_read=parm[4].value.unumber};
        err = ag_drv_tod_config_0_set(&config_0);
        break;
    }
    case cli_tod_tod_config_2:
        err = ag_drv_tod_tod_config_2_set(parm[1].value.unumber, parm[2].value.unumber);
        break;
    case cli_tod_tod_config_3:
        err = ag_drv_tod_tod_config_3_set(parm[1].value.unumber, parm[2].value.unumber, parm[3].value.unumber);
        break;
    case cli_tod_tod_fifo_status:
    {
        tod_tod_fifo_status tod_fifo_status = { .rempty_gpon=parm[1].value.unumber, .rempty_epon=parm[2].value.unumber, .rempty_ngpon=parm[3].value.unumber, .rempty_10g_epon=parm[4].value.unumber, .rempty_ae=parm[5].value.unumber, .rempty_rx=parm[6].value.unumber, .rempty_tx=parm[7].value.unumber, .rempty_ref=parm[8].value.unumber, .rempty_mac_rx=parm[9].value.unumber, .rempty_mac_tx=parm[10].value.unumber, .wfull_gpon=parm[11].value.unumber, .wfull_epon=parm[12].value.unumber, .wfull_ngpon=parm[13].value.unumber, .wfull_10g_epon=parm[14].value.unumber, .wfull_ae=parm[15].value.unumber, .wfull_rx=parm[16].value.unumber, .wfull_tx=parm[17].value.unumber, .wfull_ref=parm[18].value.unumber, .wfull_mac_rx=parm[19].value.unumber, .wfull_mac_tx=parm[20].value.unumber};
        err = ag_drv_tod_tod_fifo_status_set(&tod_fifo_status);
        break;
    }
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
        bdmf_session_print(session, "cfg_ts48_mac_select = %u (0x%x)\n", config_0.cfg_ts48_mac_select, config_0.cfg_ts48_mac_select);
        bdmf_session_print(session, "cfg_ts48_enable = %u (0x%x)\n", config_0.cfg_ts48_enable, config_0.cfg_ts48_enable);
        bdmf_session_print(session, "cfg_ts48_offset = %u (0x%x)\n", config_0.cfg_ts48_offset, config_0.cfg_ts48_offset);
        bdmf_session_print(session, "cfg_ts48_read = %u (0x%x)\n", config_0.cfg_ts48_read, config_0.cfg_ts48_read);
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
    case cli_tod_tod_config_2:
    {
        uint16_t rx_offset;
        uint16_t tx_offset;
        err = ag_drv_tod_tod_config_2_get(&rx_offset, &tx_offset);
        bdmf_session_print(session, "rx_offset = %u (0x%x)\n", rx_offset, rx_offset);
        bdmf_session_print(session, "tx_offset = %u (0x%x)\n", tx_offset, tx_offset);
        break;
    }
    case cli_tod_tod_config_3:
    {
        uint16_t ref_offset;
        uint8_t ts48_fifo_ld_rate;
        bdmf_boolean ts48_fifo_dis;
        err = ag_drv_tod_tod_config_3_get(&ref_offset, &ts48_fifo_ld_rate, &ts48_fifo_dis);
        bdmf_session_print(session, "ref_offset = %u (0x%x)\n", ref_offset, ref_offset);
        bdmf_session_print(session, "ts48_fifo_ld_rate = %u (0x%x)\n", ts48_fifo_ld_rate, ts48_fifo_ld_rate);
        bdmf_session_print(session, "ts48_fifo_dis = %u (0x%x)\n", ts48_fifo_dis, ts48_fifo_dis);
        break;
    }
    case cli_tod_tod_status_0:
    {
        uint16_t ts16_rx_read;
        uint16_t ts16_tx_read;
        err = ag_drv_tod_tod_status_0_get(&ts16_rx_read, &ts16_tx_read);
        bdmf_session_print(session, "ts16_rx_read = %u (0x%x)\n", ts16_rx_read, ts16_rx_read);
        bdmf_session_print(session, "ts16_tx_read = %u (0x%x)\n", ts16_tx_read, ts16_tx_read);
        break;
    }
    case cli_tod_tod_status_1:
    {
        uint16_t ts16_ref_synce_read;
        uint16_t ts16_rx_synce_read;
        err = ag_drv_tod_tod_status_1_get(&ts16_ref_synce_read, &ts16_rx_synce_read);
        bdmf_session_print(session, "ts16_ref_synce_read = %u (0x%x)\n", ts16_ref_synce_read, ts16_ref_synce_read);
        bdmf_session_print(session, "ts16_rx_synce_read = %u (0x%x)\n", ts16_rx_synce_read, ts16_rx_synce_read);
        break;
    }
    case cli_tod_tod_status_2:
    {
        uint16_t ts16_mac_rx_read;
        uint16_t ts16_mac_tx_read;
        err = ag_drv_tod_tod_status_2_get(&ts16_mac_rx_read, &ts16_mac_tx_read);
        bdmf_session_print(session, "ts16_mac_rx_read = %u (0x%x)\n", ts16_mac_rx_read, ts16_mac_rx_read);
        bdmf_session_print(session, "ts16_mac_tx_read = %u (0x%x)\n", ts16_mac_tx_read, ts16_mac_tx_read);
        break;
    }
    case cli_tod_tod_fifo_status:
    {
        tod_tod_fifo_status tod_fifo_status;
        err = ag_drv_tod_tod_fifo_status_get(&tod_fifo_status);
        bdmf_session_print(session, "rempty_gpon = %u (0x%x)\n", tod_fifo_status.rempty_gpon, tod_fifo_status.rempty_gpon);
        bdmf_session_print(session, "rempty_epon = %u (0x%x)\n", tod_fifo_status.rempty_epon, tod_fifo_status.rempty_epon);
        bdmf_session_print(session, "rempty_ngpon = %u (0x%x)\n", tod_fifo_status.rempty_ngpon, tod_fifo_status.rempty_ngpon);
        bdmf_session_print(session, "rempty_10g_epon = %u (0x%x)\n", tod_fifo_status.rempty_10g_epon, tod_fifo_status.rempty_10g_epon);
        bdmf_session_print(session, "rempty_ae = %u (0x%x)\n", tod_fifo_status.rempty_ae, tod_fifo_status.rempty_ae);
        bdmf_session_print(session, "rempty_rx = %u (0x%x)\n", tod_fifo_status.rempty_rx, tod_fifo_status.rempty_rx);
        bdmf_session_print(session, "rempty_tx = %u (0x%x)\n", tod_fifo_status.rempty_tx, tod_fifo_status.rempty_tx);
        bdmf_session_print(session, "rempty_ref = %u (0x%x)\n", tod_fifo_status.rempty_ref, tod_fifo_status.rempty_ref);
        bdmf_session_print(session, "rempty_mac_rx = %u (0x%x)\n", tod_fifo_status.rempty_mac_rx, tod_fifo_status.rempty_mac_rx);
        bdmf_session_print(session, "rempty_mac_tx = %u (0x%x)\n", tod_fifo_status.rempty_mac_tx, tod_fifo_status.rempty_mac_tx);
        bdmf_session_print(session, "wfull_gpon = %u (0x%x)\n", tod_fifo_status.wfull_gpon, tod_fifo_status.wfull_gpon);
        bdmf_session_print(session, "wfull_epon = %u (0x%x)\n", tod_fifo_status.wfull_epon, tod_fifo_status.wfull_epon);
        bdmf_session_print(session, "wfull_ngpon = %u (0x%x)\n", tod_fifo_status.wfull_ngpon, tod_fifo_status.wfull_ngpon);
        bdmf_session_print(session, "wfull_10g_epon = %u (0x%x)\n", tod_fifo_status.wfull_10g_epon, tod_fifo_status.wfull_10g_epon);
        bdmf_session_print(session, "wfull_ae = %u (0x%x)\n", tod_fifo_status.wfull_ae, tod_fifo_status.wfull_ae);
        bdmf_session_print(session, "wfull_rx = %u (0x%x)\n", tod_fifo_status.wfull_rx, tod_fifo_status.wfull_rx);
        bdmf_session_print(session, "wfull_tx = %u (0x%x)\n", tod_fifo_status.wfull_tx, tod_fifo_status.wfull_tx);
        bdmf_session_print(session, "wfull_ref = %u (0x%x)\n", tod_fifo_status.wfull_ref, tod_fifo_status.wfull_ref);
        bdmf_session_print(session, "wfull_mac_rx = %u (0x%x)\n", tod_fifo_status.wfull_mac_rx, tod_fifo_status.wfull_mac_rx);
        bdmf_session_print(session, "wfull_mac_tx = %u (0x%x)\n", tod_fifo_status.wfull_mac_tx, tod_fifo_status.wfull_mac_tx);
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
        tod_config_0 config_0 = {.cfg_ts48_mac_select=gtmv(m, 3), .cfg_ts48_enable=gtmv(m, 1), .cfg_ts48_offset=gtmv(m, 10), .cfg_ts48_read=gtmv(m, 1)};
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_0_set( %u %u %u %u)\n", config_0.cfg_ts48_mac_select, config_0.cfg_ts48_enable, config_0.cfg_ts48_offset, config_0.cfg_ts48_read);
        if(!err) ag_drv_tod_config_0_set(&config_0);
        if(!err) ag_drv_tod_config_0_get( &config_0);
        if(!err) bdmf_session_print(session, "ag_drv_tod_config_0_get( %u %u %u %u)\n", config_0.cfg_ts48_mac_select, config_0.cfg_ts48_enable, config_0.cfg_ts48_offset, config_0.cfg_ts48_read);
        if(err || config_0.cfg_ts48_mac_select!=gtmv(m, 3) || config_0.cfg_ts48_enable!=gtmv(m, 1) || config_0.cfg_ts48_offset!=gtmv(m, 10) || config_0.cfg_ts48_read!=gtmv(m, 1))
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
        uint16_t rx_offset=gtmv(m, 10);
        uint16_t tx_offset=gtmv(m, 10);
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_config_2_set( %u %u)\n", rx_offset, tx_offset);
        if(!err) ag_drv_tod_tod_config_2_set(rx_offset, tx_offset);
        if(!err) ag_drv_tod_tod_config_2_get( &rx_offset, &tx_offset);
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_config_2_get( %u %u)\n", rx_offset, tx_offset);
        if(err || rx_offset!=gtmv(m, 10) || tx_offset!=gtmv(m, 10))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint16_t ref_offset=gtmv(m, 10);
        uint8_t ts48_fifo_ld_rate=gtmv(m, 5);
        bdmf_boolean ts48_fifo_dis=gtmv(m, 1);
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_config_3_set( %u %u %u)\n", ref_offset, ts48_fifo_ld_rate, ts48_fifo_dis);
        if(!err) ag_drv_tod_tod_config_3_set(ref_offset, ts48_fifo_ld_rate, ts48_fifo_dis);
        if(!err) ag_drv_tod_tod_config_3_get( &ref_offset, &ts48_fifo_ld_rate, &ts48_fifo_dis);
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_config_3_get( %u %u %u)\n", ref_offset, ts48_fifo_ld_rate, ts48_fifo_dis);
        if(err || ref_offset!=gtmv(m, 10) || ts48_fifo_ld_rate!=gtmv(m, 5) || ts48_fifo_dis!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint16_t ts16_rx_read=gtmv(m, 16);
        uint16_t ts16_tx_read=gtmv(m, 16);
        if(!err) ag_drv_tod_tod_status_0_get( &ts16_rx_read, &ts16_tx_read);
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_status_0_get( %u %u)\n", ts16_rx_read, ts16_tx_read);
    }
    {
        uint16_t ts16_ref_synce_read=gtmv(m, 16);
        uint16_t ts16_rx_synce_read=gtmv(m, 16);
        if(!err) ag_drv_tod_tod_status_1_get( &ts16_ref_synce_read, &ts16_rx_synce_read);
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_status_1_get( %u %u)\n", ts16_ref_synce_read, ts16_rx_synce_read);
    }
    {
        uint16_t ts16_mac_rx_read=gtmv(m, 16);
        uint16_t ts16_mac_tx_read=gtmv(m, 16);
        if(!err) ag_drv_tod_tod_status_2_get( &ts16_mac_rx_read, &ts16_mac_tx_read);
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_status_2_get( %u %u)\n", ts16_mac_rx_read, ts16_mac_tx_read);
    }
    {
        tod_tod_fifo_status tod_fifo_status = {.rempty_gpon=gtmv(m, 1), .rempty_epon=gtmv(m, 1), .rempty_ngpon=gtmv(m, 1), .rempty_10g_epon=gtmv(m, 1), .rempty_ae=gtmv(m, 1), .rempty_rx=gtmv(m, 1), .rempty_tx=gtmv(m, 1), .rempty_ref=gtmv(m, 1), .rempty_mac_rx=gtmv(m, 1), .rempty_mac_tx=gtmv(m, 1), .wfull_gpon=gtmv(m, 1), .wfull_epon=gtmv(m, 1), .wfull_ngpon=gtmv(m, 1), .wfull_10g_epon=gtmv(m, 1), .wfull_ae=gtmv(m, 1), .wfull_rx=gtmv(m, 1), .wfull_tx=gtmv(m, 1), .wfull_ref=gtmv(m, 1), .wfull_mac_rx=gtmv(m, 1), .wfull_mac_tx=gtmv(m, 1)};
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_fifo_status_set( %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u)\n", tod_fifo_status.rempty_gpon, tod_fifo_status.rempty_epon, tod_fifo_status.rempty_ngpon, tod_fifo_status.rempty_10g_epon, tod_fifo_status.rempty_ae, tod_fifo_status.rempty_rx, tod_fifo_status.rempty_tx, tod_fifo_status.rempty_ref, tod_fifo_status.rempty_mac_rx, tod_fifo_status.rempty_mac_tx, tod_fifo_status.wfull_gpon, tod_fifo_status.wfull_epon, tod_fifo_status.wfull_ngpon, tod_fifo_status.wfull_10g_epon, tod_fifo_status.wfull_ae, tod_fifo_status.wfull_rx, tod_fifo_status.wfull_tx, tod_fifo_status.wfull_ref, tod_fifo_status.wfull_mac_rx, tod_fifo_status.wfull_mac_tx);
        if(!err) ag_drv_tod_tod_fifo_status_set(&tod_fifo_status);
        if(!err) ag_drv_tod_tod_fifo_status_get( &tod_fifo_status);
        if(!err) bdmf_session_print(session, "ag_drv_tod_tod_fifo_status_get( %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u)\n", tod_fifo_status.rempty_gpon, tod_fifo_status.rempty_epon, tod_fifo_status.rempty_ngpon, tod_fifo_status.rempty_10g_epon, tod_fifo_status.rempty_ae, tod_fifo_status.rempty_rx, tod_fifo_status.rempty_tx, tod_fifo_status.rempty_ref, tod_fifo_status.rempty_mac_rx, tod_fifo_status.rempty_mac_tx, tod_fifo_status.wfull_gpon, tod_fifo_status.wfull_epon, tod_fifo_status.wfull_ngpon, tod_fifo_status.wfull_10g_epon, tod_fifo_status.wfull_ae, tod_fifo_status.wfull_rx, tod_fifo_status.wfull_tx, tod_fifo_status.wfull_ref, tod_fifo_status.wfull_mac_rx, tod_fifo_status.wfull_mac_tx);
        if(err || tod_fifo_status.rempty_gpon!=gtmv(m, 1) || tod_fifo_status.rempty_epon!=gtmv(m, 1) || tod_fifo_status.rempty_ngpon!=gtmv(m, 1) || tod_fifo_status.rempty_10g_epon!=gtmv(m, 1) || tod_fifo_status.rempty_ae!=gtmv(m, 1) || tod_fifo_status.rempty_rx!=gtmv(m, 1) || tod_fifo_status.rempty_tx!=gtmv(m, 1) || tod_fifo_status.rempty_ref!=gtmv(m, 1) || tod_fifo_status.rempty_mac_rx!=gtmv(m, 1) || tod_fifo_status.rempty_mac_tx!=gtmv(m, 1) || tod_fifo_status.wfull_gpon!=gtmv(m, 1) || tod_fifo_status.wfull_epon!=gtmv(m, 1) || tod_fifo_status.wfull_ngpon!=gtmv(m, 1) || tod_fifo_status.wfull_10g_epon!=gtmv(m, 1) || tod_fifo_status.wfull_ae!=gtmv(m, 1) || tod_fifo_status.wfull_rx!=gtmv(m, 1) || tod_fifo_status.wfull_tx!=gtmv(m, 1) || tod_fifo_status.wfull_ref!=gtmv(m, 1) || tod_fifo_status.wfull_mac_rx!=gtmv(m, 1) || tod_fifo_status.wfull_mac_tx!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
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
    case bdmf_address_msb : reg = &RU_REG(TOD, MSB); blk = &RU_BLK(TOD); break;
    case bdmf_address_lsb : reg = &RU_REG(TOD, LSB); blk = &RU_BLK(TOD); break;
    case bdmf_address_config_2 : reg = &RU_REG(TOD, CONFIG_2); blk = &RU_BLK(TOD); break;
    case bdmf_address_config_3 : reg = &RU_REG(TOD, CONFIG_3); blk = &RU_BLK(TOD); break;
    case bdmf_address_status_0 : reg = &RU_REG(TOD, STATUS_0); blk = &RU_BLK(TOD); break;
    case bdmf_address_status_1 : reg = &RU_REG(TOD, STATUS_1); blk = &RU_BLK(TOD); break;
    case bdmf_address_status_2 : reg = &RU_REG(TOD, STATUS_2); blk = &RU_BLK(TOD); break;
    case bdmf_address_fifo_status : reg = &RU_REG(TOD, FIFO_STATUS); blk = &RU_BLK(TOD); break;
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
            BDMFMON_MAKE_PARM("cfg_ts48_mac_select", "cfg_ts48_mac_select", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_ts48_enable", "cfg_ts48_enable", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_ts48_offset", "cfg_ts48_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cfg_ts48_read", "cfg_ts48_read", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_tod_config_2[]={
            BDMFMON_MAKE_PARM("rx_offset", "rx_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("tx_offset", "tx_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_tod_config_3[]={
            BDMFMON_MAKE_PARM("ref_offset", "ref_offset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("ts48_fifo_ld_rate", "ts48_fifo_ld_rate", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("ts48_fifo_dis", "ts48_fifo_dis", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_tod_fifo_status[]={
            BDMFMON_MAKE_PARM("rempty_gpon", "rempty_gpon", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_epon", "rempty_epon", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_ngpon", "rempty_ngpon", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_10g_epon", "rempty_10g_epon", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_ae", "rempty_ae", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_rx", "rempty_rx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_tx", "rempty_tx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_ref", "rempty_ref", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_mac_rx", "rempty_mac_rx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("rempty_mac_tx", "rempty_mac_tx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_gpon", "wfull_gpon", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_epon", "wfull_epon", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_ngpon", "wfull_ngpon", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_10g_epon", "wfull_10g_epon", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_ae", "wfull_ae", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_rx", "wfull_rx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_tx", "wfull_tx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_ref", "wfull_ref", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_mac_rx", "wfull_mac_rx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("wfull_mac_tx", "wfull_mac_tx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="config_0", .val=cli_tod_config_0, .parms=set_config_0 },
            { .name="tod_config_2", .val=cli_tod_tod_config_2, .parms=set_tod_config_2 },
            { .name="tod_config_3", .val=cli_tod_tod_config_3, .parms=set_tod_config_3 },
            { .name="tod_fifo_status", .val=cli_tod_tod_fifo_status, .parms=set_tod_fifo_status },
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
            { .name="msb", .val=cli_tod_msb, .parms=set_default },
            { .name="lsb", .val=cli_tod_lsb, .parms=set_default },
            { .name="tod_config_2", .val=cli_tod_tod_config_2, .parms=set_default },
            { .name="tod_config_3", .val=cli_tod_tod_config_3, .parms=set_default },
            { .name="tod_status_0", .val=cli_tod_tod_status_0, .parms=set_default },
            { .name="tod_status_1", .val=cli_tod_tod_status_1, .parms=set_default },
            { .name="tod_status_2", .val=cli_tod_tod_status_2, .parms=set_default },
            { .name="tod_fifo_status", .val=cli_tod_tod_fifo_status, .parms=set_default },
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
            { .name="MSB" , .val=bdmf_address_msb },
            { .name="LSB" , .val=bdmf_address_lsb },
            { .name="CONFIG_2" , .val=bdmf_address_config_2 },
            { .name="CONFIG_3" , .val=bdmf_address_config_3 },
            { .name="STATUS_0" , .val=bdmf_address_status_0 },
            { .name="STATUS_1" , .val=bdmf_address_status_1 },
            { .name="STATUS_2" , .val=bdmf_address_status_2 },
            { .name="FIFO_STATUS" , .val=bdmf_address_fifo_status },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "address", "address", bcm_tod_cli_address,
            BDMFMON_MAKE_PARM_ENUM("method", "method", enum_table_address, 0),
            BDMFMON_MAKE_PARM("index2", "onu_id/alloc_id/port_id/etc...", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
    }
    return dir;
}
#endif /* USE_BDMF_SHELL */

