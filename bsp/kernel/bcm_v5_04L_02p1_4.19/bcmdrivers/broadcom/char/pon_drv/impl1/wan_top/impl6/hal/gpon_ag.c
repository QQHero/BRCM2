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
#include "gpon_ag.h"

bdmf_error_t ag_drv_gpon_gpon_gearbox_0_set(const gpon_gpon_gearbox_0 *gpon_gearbox_0)
{
    uint32_t reg_gearbox_0=0;

#ifdef VALIDATE_PARMS
    if(!gpon_gearbox_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max >= _5BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min >= _5BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel >= _1BITS_MAX_VAL_) ||
       (gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel >= _1BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_SW_RESET_TXPG_RESET, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_SW_RESET_TXFIFO_RESET, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_CLEAR_TXFIFO_DRIFTED, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_LOOPBACK_RX, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_CLEAR_TXFIFO_COLLISION, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_WR_PTR_ADV, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_WR_PTR_DLY, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_BIT_INV, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_PTR_DIST_MAX, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_ASYM_LOOPBACK, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_PTR_DIST_MIN, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_20BIT_ORDER, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_8BIT_ORDER, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_RX_16BIT_ORDER, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TXLBE_BIT_ORDER, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_STATUS_SEL, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_PTG_STATUS1_SEL, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel);
    reg_gearbox_0 = RU_FIELD_SET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_PTG_STATUS2_SEL, reg_gearbox_0, gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel);

    RU_REG_WRITE(0, GPON, GEARBOX_0, reg_gearbox_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gpon_gearbox_0_get(gpon_gpon_gearbox_0 *gpon_gearbox_0)
{
    uint32_t reg_gearbox_0;

#ifdef VALIDATE_PARMS
    if(!gpon_gearbox_0)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON, GEARBOX_0, reg_gearbox_0);

    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_SW_RESET_TXPG_RESET, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_SW_RESET_TXFIFO_RESET, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_CLEAR_TXFIFO_DRIFTED, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_LOOPBACK_RX, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_CLEAR_TXFIFO_COLLISION, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_WR_PTR_ADV, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_WR_PTR_DLY, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_BIT_INV, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_PTR_DIST_MAX, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_ASYM_LOOPBACK, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_PTR_DIST_MIN, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_20BIT_ORDER, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TX_8BIT_ORDER, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_RX_16BIT_ORDER, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_0_TXLBE_BIT_ORDER, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_STATUS_SEL, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_PTG_STATUS1_SEL, reg_gearbox_0);
    gpon_gearbox_0->cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel = RU_FIELD_GET(0, GPON, GEARBOX_0, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_PTG_STATUS2_SEL, reg_gearbox_0);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gpon_gearbox_pattern_cfg1_set(const gpon_gpon_gearbox_pattern_cfg1 *gpon_gearbox_pattern_cfg1)
{
    uint32_t reg_gearbox_pattern_cfg1=0;

#ifdef VALIDATE_PARMS
    if(!gpon_gearbox_pattern_cfg1)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((gpon_gearbox_pattern_cfg1->pg_mode >= _3BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_gearbox_pattern_cfg1 = RU_FIELD_SET(0, GPON, GEARBOX_PATTERN_CFG1, PG_MODE, reg_gearbox_pattern_cfg1, gpon_gearbox_pattern_cfg1->pg_mode);
    reg_gearbox_pattern_cfg1 = RU_FIELD_SET(0, GPON, GEARBOX_PATTERN_CFG1, HEADER, reg_gearbox_pattern_cfg1, gpon_gearbox_pattern_cfg1->header);
    reg_gearbox_pattern_cfg1 = RU_FIELD_SET(0, GPON, GEARBOX_PATTERN_CFG1, PAYLOAD, reg_gearbox_pattern_cfg1, gpon_gearbox_pattern_cfg1->payload);
    reg_gearbox_pattern_cfg1 = RU_FIELD_SET(0, GPON, GEARBOX_PATTERN_CFG1, FILLER, reg_gearbox_pattern_cfg1, gpon_gearbox_pattern_cfg1->filler);

    RU_REG_WRITE(0, GPON, GEARBOX_PATTERN_CFG1, reg_gearbox_pattern_cfg1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gpon_gearbox_pattern_cfg1_get(gpon_gpon_gearbox_pattern_cfg1 *gpon_gearbox_pattern_cfg1)
{
    uint32_t reg_gearbox_pattern_cfg1;

#ifdef VALIDATE_PARMS
    if(!gpon_gearbox_pattern_cfg1)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON, GEARBOX_PATTERN_CFG1, reg_gearbox_pattern_cfg1);

    gpon_gearbox_pattern_cfg1->pg_mode = RU_FIELD_GET(0, GPON, GEARBOX_PATTERN_CFG1, PG_MODE, reg_gearbox_pattern_cfg1);
    gpon_gearbox_pattern_cfg1->header = RU_FIELD_GET(0, GPON, GEARBOX_PATTERN_CFG1, HEADER, reg_gearbox_pattern_cfg1);
    gpon_gearbox_pattern_cfg1->payload = RU_FIELD_GET(0, GPON, GEARBOX_PATTERN_CFG1, PAYLOAD, reg_gearbox_pattern_cfg1);
    gpon_gearbox_pattern_cfg1->filler = RU_FIELD_GET(0, GPON, GEARBOX_PATTERN_CFG1, FILLER, reg_gearbox_pattern_cfg1);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gpon_gearbox_pattern_cfg2_set(uint8_t gap_size, uint8_t burst_size)
{
    uint32_t reg_gearbox_pattern_cfg2=0;

#ifdef VALIDATE_PARMS
#endif

    reg_gearbox_pattern_cfg2 = RU_FIELD_SET(0, GPON, GEARBOX_PATTERN_CFG2, BURST_SIZE, reg_gearbox_pattern_cfg2, burst_size);
    reg_gearbox_pattern_cfg2 = RU_FIELD_SET(0, GPON, GEARBOX_PATTERN_CFG2, GAP_SIZE, reg_gearbox_pattern_cfg2, gap_size);

    RU_REG_WRITE(0, GPON, GEARBOX_PATTERN_CFG2, reg_gearbox_pattern_cfg2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gpon_gearbox_pattern_cfg2_get(uint8_t *gap_size, uint8_t *burst_size)
{
    uint32_t reg_gearbox_pattern_cfg2;

#ifdef VALIDATE_PARMS
    if(!burst_size || !gap_size)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON, GEARBOX_PATTERN_CFG2, reg_gearbox_pattern_cfg2);

    *burst_size = RU_FIELD_GET(0, GPON, GEARBOX_PATTERN_CFG2, BURST_SIZE, reg_gearbox_pattern_cfg2);
    *gap_size = RU_FIELD_GET(0, GPON, GEARBOX_PATTERN_CFG2, GAP_SIZE, reg_gearbox_pattern_cfg2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gpon_gearbox_2_set(const gpon_gpon_gearbox_2 *gpon_gearbox_2)
{
    uint32_t reg_gearbox_2=0;

#ifdef VALIDATE_PARMS
    if(!gpon_gearbox_2)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
    if((gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer >= _5BITS_MAX_VAL_) ||
       (gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer >= _5BITS_MAX_VAL_) ||
       (gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc >= _3BITS_MAX_VAL_) ||
       (gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc >= _4BITS_MAX_VAL_))
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_RANGE), BDMF_ERR_RANGE);
        return BDMF_ERR_RANGE;
    }
#endif

    reg_gearbox_2 = RU_FIELD_SET(0, GPON, GEARBOX_2, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_1_TX_RD_POINTER, reg_gearbox_2, gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer);
    reg_gearbox_2 = RU_FIELD_SET(0, GPON, GEARBOX_2, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_1_TX_WR_POINTER, reg_gearbox_2, gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer);
    reg_gearbox_2 = RU_FIELD_SET(0, GPON, GEARBOX_2, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_TX_VLD_DELAY_CYC, reg_gearbox_2, gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc);
    reg_gearbox_2 = RU_FIELD_SET(0, GPON, GEARBOX_2, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_CONFIG_BURST_DELAY_CYC, reg_gearbox_2, gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc);

    RU_REG_WRITE(0, GPON, GEARBOX_2, reg_gearbox_2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gpon_gearbox_2_get(gpon_gpon_gearbox_2 *gpon_gearbox_2)
{
    uint32_t reg_gearbox_2;

#ifdef VALIDATE_PARMS
    if(!gpon_gearbox_2)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON, GEARBOX_2, reg_gearbox_2);

    gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer = RU_FIELD_GET(0, GPON, GEARBOX_2, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_1_TX_RD_POINTER, reg_gearbox_2);
    gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer = RU_FIELD_GET(0, GPON, GEARBOX_2, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_FIFO_CFG_1_TX_WR_POINTER, reg_gearbox_2);
    gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc = RU_FIELD_GET(0, GPON, GEARBOX_2, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_TX_VLD_DELAY_CYC, reg_gearbox_2);
    gpon_gearbox_2->cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc = RU_FIELD_GET(0, GPON, GEARBOX_2, CR_XGWAN_TOP_WAN_MISC_GPON_GEARBOX_CONFIG_BURST_DELAY_CYC, reg_gearbox_2);

    return BDMF_ERR_OK;
}

bdmf_error_t ag_drv_gpon_gpon_gearbox_status_get(uint32_t *cr_rd_data_clx)
{
    uint32_t reg_gearbox_status;

#ifdef VALIDATE_PARMS
    if(!cr_rd_data_clx)
    {
        bdmf_trace("ERROR driver %s:%u| err=%s (%d)\n", __FILE__, __LINE__, bdmf_strerror(BDMF_ERR_PARM), BDMF_ERR_PARM);
        return BDMF_ERR_PARM;
    }
#endif

    RU_REG_READ(0, GPON, GEARBOX_STATUS, reg_gearbox_status);

    *cr_rd_data_clx = RU_FIELD_GET(0, GPON, GEARBOX_STATUS, CR_RD_DATA_CLX, reg_gearbox_status);

    return BDMF_ERR_OK;
}

#ifdef USE_BDMF_SHELL
typedef enum
{
    bdmf_address_gearbox_0,
    bdmf_address_gearbox_pattern_cfg1,
    bdmf_address_gearbox_pattern_cfg2,
    bdmf_address_gearbox_2,
    bdmf_address_gearbox_status,
}
bdmf_address;

static int bcm_gpon_cli_set(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_gpon_gpon_gearbox_0:
    {
        gpon_gpon_gearbox_0 gpon_gearbox_0 = { .cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset=parm[1].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset=parm[2].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted=parm[3].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx=parm[4].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision=parm[5].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv=parm[6].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly=parm[7].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv=parm[8].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max=parm[9].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback=parm[10].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min=parm[11].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order=parm[12].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order=parm[13].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order=parm[14].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order=parm[15].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel=parm[16].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel=parm[17].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel=parm[18].value.unumber};
        err = ag_drv_gpon_gpon_gearbox_0_set(&gpon_gearbox_0);
        break;
    }
    case cli_gpon_gpon_gearbox_pattern_cfg1:
    {
        gpon_gpon_gearbox_pattern_cfg1 gpon_gearbox_pattern_cfg1 = { .pg_mode=parm[1].value.unumber, .header=parm[2].value.unumber, .payload=parm[3].value.unumber, .filler=parm[4].value.unumber};
        err = ag_drv_gpon_gpon_gearbox_pattern_cfg1_set(&gpon_gearbox_pattern_cfg1);
        break;
    }
    case cli_gpon_gpon_gearbox_pattern_cfg2:
        err = ag_drv_gpon_gpon_gearbox_pattern_cfg2_set(parm[1].value.unumber, parm[2].value.unumber);
        break;
    case cli_gpon_gpon_gearbox_2:
    {
        gpon_gpon_gearbox_2 gpon_gearbox_2 = { .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer=parm[1].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer=parm[2].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc=parm[3].value.unumber, .cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc=parm[4].value.unumber};
        err = ag_drv_gpon_gpon_gearbox_2_set(&gpon_gearbox_2);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

int bcm_gpon_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_error_t err = BDMF_ERR_OK;

    switch(parm[0].value.unumber)
    {
    case cli_gpon_gpon_gearbox_0:
    {
        gpon_gpon_gearbox_0 gpon_gearbox_0;
        err = ag_drv_gpon_gpon_gearbox_0_get(&gpon_gearbox_0);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel = %u (0x%x)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel);
        break;
    }
    case cli_gpon_gpon_gearbox_pattern_cfg1:
    {
        gpon_gpon_gearbox_pattern_cfg1 gpon_gearbox_pattern_cfg1;
        err = ag_drv_gpon_gpon_gearbox_pattern_cfg1_get(&gpon_gearbox_pattern_cfg1);
        bdmf_session_print(session, "pg_mode = %u (0x%x)\n", gpon_gearbox_pattern_cfg1.pg_mode, gpon_gearbox_pattern_cfg1.pg_mode);
        bdmf_session_print(session, "header = %u (0x%x)\n", gpon_gearbox_pattern_cfg1.header, gpon_gearbox_pattern_cfg1.header);
        bdmf_session_print(session, "payload = %u (0x%x)\n", gpon_gearbox_pattern_cfg1.payload, gpon_gearbox_pattern_cfg1.payload);
        bdmf_session_print(session, "filler = %u (0x%x)\n", gpon_gearbox_pattern_cfg1.filler, gpon_gearbox_pattern_cfg1.filler);
        break;
    }
    case cli_gpon_gpon_gearbox_pattern_cfg2:
    {
        uint8_t burst_size;
        uint8_t gap_size;
        err = ag_drv_gpon_gpon_gearbox_pattern_cfg2_get(&burst_size, &gap_size);
        bdmf_session_print(session, "burst_size = %u (0x%x)\n", burst_size, burst_size);
        bdmf_session_print(session, "gap_size = %u (0x%x)\n", gap_size, gap_size);
        break;
    }
    case cli_gpon_gpon_gearbox_2:
    {
        gpon_gpon_gearbox_2 gpon_gearbox_2;
        err = ag_drv_gpon_gpon_gearbox_2_get(&gpon_gearbox_2);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer = %u (0x%x)\n", gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer = %u (0x%x)\n", gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc = %u (0x%x)\n", gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc);
        bdmf_session_print(session, "cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc = %u (0x%x)\n", gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc);
        break;
    }
    case cli_gpon_gpon_gearbox_status:
    {
        uint32_t cr_rd_data_clx;
        err = ag_drv_gpon_gpon_gearbox_status_get(&cr_rd_data_clx);
        bdmf_session_print(session, "cr_rd_data_clx = %u (0x%x)\n", cr_rd_data_clx, cr_rd_data_clx);
        break;
    }
    default:
        err = BDMF_ERR_NOT_SUPPORTED;
        break;
    }
    return err;
}

static int bcm_gpon_cli_test(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    bdmf_test_method m = parm[0].value.unumber;
    bdmf_error_t err = BDMF_ERR_OK;

    {
        gpon_gpon_gearbox_0 gpon_gearbox_0 = {.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max=gtmv(m, 5), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min=gtmv(m, 5), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel=gtmv(m, 1), .cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel=gtmv(m, 1)};
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_0_set( %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel);
        if(!err) ag_drv_gpon_gpon_gearbox_0_set(&gpon_gearbox_0);
        if(!err) ag_drv_gpon_gpon_gearbox_0_get( &gpon_gearbox_0);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_0_get( %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u)\n", gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel, gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel);
        if(err || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max!=gtmv(m, 5) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min!=gtmv(m, 5) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel!=gtmv(m, 1) || gpon_gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel!=gtmv(m, 1))
            return err ? err : BDMF_ERR_IO;
    }
    {
        gpon_gpon_gearbox_pattern_cfg1 gpon_gearbox_pattern_cfg1 = {.pg_mode=gtmv(m, 3), .header=gtmv(m, 8), .payload=gtmv(m, 8), .filler=gtmv(m, 8)};
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_pattern_cfg1_set( %u %u %u %u)\n", gpon_gearbox_pattern_cfg1.pg_mode, gpon_gearbox_pattern_cfg1.header, gpon_gearbox_pattern_cfg1.payload, gpon_gearbox_pattern_cfg1.filler);
        if(!err) ag_drv_gpon_gpon_gearbox_pattern_cfg1_set(&gpon_gearbox_pattern_cfg1);
        if(!err) ag_drv_gpon_gpon_gearbox_pattern_cfg1_get( &gpon_gearbox_pattern_cfg1);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_pattern_cfg1_get( %u %u %u %u)\n", gpon_gearbox_pattern_cfg1.pg_mode, gpon_gearbox_pattern_cfg1.header, gpon_gearbox_pattern_cfg1.payload, gpon_gearbox_pattern_cfg1.filler);
        if(err || gpon_gearbox_pattern_cfg1.pg_mode!=gtmv(m, 3) || gpon_gearbox_pattern_cfg1.header!=gtmv(m, 8) || gpon_gearbox_pattern_cfg1.payload!=gtmv(m, 8) || gpon_gearbox_pattern_cfg1.filler!=gtmv(m, 8))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint8_t burst_size=gtmv(m, 8);
        uint8_t gap_size=gtmv(m, 8);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_pattern_cfg2_set( %u %u)\n", burst_size, gap_size);
        if(!err) ag_drv_gpon_gpon_gearbox_pattern_cfg2_set(burst_size, gap_size);
        if(!err) ag_drv_gpon_gpon_gearbox_pattern_cfg2_get( &burst_size, &gap_size);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_pattern_cfg2_get( %u %u)\n", burst_size, gap_size);
        if(err || burst_size!=gtmv(m, 8) || gap_size!=gtmv(m, 8))
            return err ? err : BDMF_ERR_IO;
    }
    {
        gpon_gpon_gearbox_2 gpon_gearbox_2 = {.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer=gtmv(m, 5), .cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer=gtmv(m, 5), .cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc=gtmv(m, 3), .cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc=gtmv(m, 4)};
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_2_set( %u %u %u %u)\n", gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc);
        if(!err) ag_drv_gpon_gpon_gearbox_2_set(&gpon_gearbox_2);
        if(!err) ag_drv_gpon_gpon_gearbox_2_get( &gpon_gearbox_2);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_2_get( %u %u %u %u)\n", gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc, gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc);
        if(err || gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer!=gtmv(m, 5) || gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer!=gtmv(m, 5) || gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc!=gtmv(m, 3) || gpon_gearbox_2.cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc!=gtmv(m, 4))
            return err ? err : BDMF_ERR_IO;
    }
    {
        uint32_t cr_rd_data_clx=gtmv(m, 32);
        if(!err) ag_drv_gpon_gpon_gearbox_status_get( &cr_rd_data_clx);
        if(!err) bdmf_session_print(session, "ag_drv_gpon_gpon_gearbox_status_get( %u)\n", cr_rd_data_clx);
    }
    return err;
}

static int bcm_gpon_cli_address(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
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
    case bdmf_address_gearbox_0 : reg = &RU_REG(GPON, GEARBOX_0); blk = &RU_BLK(GPON); break;
    case bdmf_address_gearbox_pattern_cfg1 : reg = &RU_REG(GPON, GEARBOX_PATTERN_CFG1); blk = &RU_BLK(GPON); break;
    case bdmf_address_gearbox_pattern_cfg2 : reg = &RU_REG(GPON, GEARBOX_PATTERN_CFG2); blk = &RU_BLK(GPON); break;
    case bdmf_address_gearbox_2 : reg = &RU_REG(GPON, GEARBOX_2); blk = &RU_BLK(GPON); break;
    case bdmf_address_gearbox_status : reg = &RU_REG(GPON, GEARBOX_STATUS); blk = &RU_BLK(GPON); break;
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

bdmfmon_handle_t ag_drv_gpon_cli_init(bdmfmon_handle_t driver_dir)
{
    bdmfmon_handle_t dir;

    if ((dir = bdmfmon_dir_find(driver_dir, "gpon"))!=NULL)
        return dir;
    dir = bdmfmon_dir_add(driver_dir, "gpon", "gpon", BDMF_ACCESS_ADMIN, NULL);

    {
        static bdmfmon_cmd_parm_t set_gpon_gearbox_0[]={
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset", "cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset", "cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel", "cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel", "cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_gpon_gearbox_pattern_cfg1[]={
            BDMFMON_MAKE_PARM("pg_mode", "pg_mode", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("header", "header", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("payload", "payload", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("filler", "filler", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_gpon_gearbox_pattern_cfg2[]={
            BDMFMON_MAKE_PARM("burst_size", "burst_size", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("gap_size", "gap_size", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_cmd_parm_t set_gpon_gearbox_2[]={
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer", "cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc", "cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc", "cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="gpon_gearbox_0", .val=cli_gpon_gpon_gearbox_0, .parms=set_gpon_gearbox_0 },
            { .name="gpon_gearbox_pattern_cfg1", .val=cli_gpon_gpon_gearbox_pattern_cfg1, .parms=set_gpon_gearbox_pattern_cfg1 },
            { .name="gpon_gearbox_pattern_cfg2", .val=cli_gpon_gpon_gearbox_pattern_cfg2, .parms=set_gpon_gearbox_pattern_cfg2 },
            { .name="gpon_gearbox_2", .val=cli_gpon_gpon_gearbox_2, .parms=set_gpon_gearbox_2 },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "set", "set", bcm_gpon_cli_set,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_cmd_parm_t set_default[]={
            BDMFMON_PARM_LIST_TERMINATOR
        };
        static bdmfmon_enum_val_t selector_table[] = {
            { .name="gpon_gearbox_0", .val=cli_gpon_gpon_gearbox_0, .parms=set_default },
            { .name="gpon_gearbox_pattern_cfg1", .val=cli_gpon_gpon_gearbox_pattern_cfg1, .parms=set_default },
            { .name="gpon_gearbox_pattern_cfg2", .val=cli_gpon_gpon_gearbox_pattern_cfg2, .parms=set_default },
            { .name="gpon_gearbox_2", .val=cli_gpon_gpon_gearbox_2, .parms=set_default },
            { .name="gpon_gearbox_status", .val=cli_gpon_gpon_gearbox_status, .parms=set_default },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "get", "get", bcm_gpon_cli_get,
            BDMFMON_MAKE_PARM_SELECTOR("purpose", "purpose", selector_table, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_test_method[] = {
            { .name="low" , .val=bdmf_test_method_low },
            { .name="mid" , .val=bdmf_test_method_mid },
            { .name="high" , .val=bdmf_test_method_high },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "test", "test", bcm_gpon_cli_test,
            BDMFMON_MAKE_PARM_ENUM("method", "low: 0000, mid: 1000, high: 1111", enum_table_test_method, 0));
    }
    {
        static bdmfmon_enum_val_t enum_table_address[] = {
            { .name="GEARBOX_0" , .val=bdmf_address_gearbox_0 },
            { .name="GEARBOX_PATTERN_CFG1" , .val=bdmf_address_gearbox_pattern_cfg1 },
            { .name="GEARBOX_PATTERN_CFG2" , .val=bdmf_address_gearbox_pattern_cfg2 },
            { .name="GEARBOX_2" , .val=bdmf_address_gearbox_2 },
            { .name="GEARBOX_STATUS" , .val=bdmf_address_gearbox_status },
            BDMFMON_ENUM_LAST
        };
        BDMFMON_MAKE_CMD(dir, "address", "address", bcm_gpon_cli_address,
            BDMFMON_MAKE_PARM_ENUM("method", "method", enum_table_address, 0),
            BDMFMON_MAKE_PARM("index2", "onu_id/alloc_id/port_id/etc...", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL));
    }
    return dir;
}
#endif /* USE_BDMF_SHELL */

