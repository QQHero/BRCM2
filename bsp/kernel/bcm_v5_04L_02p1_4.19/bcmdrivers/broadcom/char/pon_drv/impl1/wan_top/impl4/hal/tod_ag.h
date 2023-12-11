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

#ifndef _TOD_AG_H_
#define _TOD_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* tod_read_busy:  - Indicates TOD read is in progress.  Deassertive value indicatesvalid values  */
/*                at WAN_TOD/WAN_TOD_TS64 registers.                                              */
/* cfg_ts48_pre_sync_fifo_disable:  - 0: New mode. Transfer TS48 using FIFO. 1: Legacy mode.  Tra */
/*                                 nsferupper TS48 bits between clock domains.                    */
/* cfg_ts48_pre_sync_fifo_load_rate:  - Number of clock ticks between consecutive writes to the T */
/*                                   S48 FIFO.                                                    */
/* cfg_tod_pps_clear:  - Allows 1PPS pulse to load cfg_tod_1pps_ns_offset into nanosecondcounter. */
/*                      If not set, the 1PPS pulse will have no effect on theTS48.                */
/* cfg_ts48_read:  - Arm the reading of the TS48/TS64 timestamps.  Values are valid atthe deasser */
/*                tion of tod_read_busy                                                           */
/* cfg_ts48_offset:  - The TS48 offset value.In legacy, GPON mode (cfg_ts48_pre_sync_fifo_disable */
/*                   = 1), therising edge ofTS48/TS64's bit[9] loads cfg_ts48_offset[8:0] into th */
/*                  e lower 9 bitsof the synchronized TS48/TS64.In the new mode, the timestamp is */
/*                   transfer to the 250 MHz clockdomain via an asynchronousFIFO.  The offset is  */
/*                  added to the output of the FIFO.  Thecfg_ts48_offset[8] is the signbit, allow */
/*                  ing +/- adjustment to the timestamp value. It is signextended to make theoffs */
/*                  et 48-bits.In AE mode, the offset is added to the current TS48 value andloadi */
/*                  ng it back into AE TS48.Loading is accomplished by setting the cfg_tod_load_t */
/*                  s48_offset bit.The sign extension ofcfg_ts48_offset[8] also applies.          */
/* cfg_ts48_mac_select:  - This field selects the MAC that the timestamp comes from.2: GPON4: Act */
/*                      ive Ethernet0,1,3,5,6,7: Reserved                                         */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean tod_read_busy;
    bdmf_boolean cfg_ts48_pre_sync_fifo_disable;
    uint8_t cfg_ts48_pre_sync_fifo_load_rate;
    bdmf_boolean cfg_tod_pps_clear;
    bdmf_boolean cfg_ts48_read;
    uint16_t cfg_ts48_offset;
    uint8_t cfg_ts48_mac_select;
} tod_config_0;

bdmf_error_t ag_drv_tod_config_0_set(const tod_config_0 *config_0);
bdmf_error_t ag_drv_tod_config_0_get(tod_config_0 *config_0);
bdmf_error_t ag_drv_tod_config_1_set(bdmf_boolean cfg_tod_load_ts48_offset, bdmf_boolean cfg_tod_load, uint32_t cfg_tod_seconds);
bdmf_error_t ag_drv_tod_config_1_get(bdmf_boolean *cfg_tod_load_ts48_offset, bdmf_boolean *cfg_tod_load, uint32_t *cfg_tod_seconds);
bdmf_error_t ag_drv_tod_config_2_set(uint16_t cfg_tx_offset, uint16_t cfg_rx_offset);
bdmf_error_t ag_drv_tod_config_2_get(uint16_t *cfg_tx_offset, uint16_t *cfg_rx_offset);
bdmf_error_t ag_drv_tod_config_3_set(uint16_t cfg_ref_offset);
bdmf_error_t ag_drv_tod_config_3_get(uint16_t *cfg_ref_offset);
bdmf_error_t ag_drv_tod_config_4_set(uint32_t cfg_tod_1pps_ns_offset);
bdmf_error_t ag_drv_tod_config_4_get(uint32_t *cfg_tod_1pps_ns_offset);
bdmf_error_t ag_drv_tod_config_5_set(bdmf_boolean cfg_tod_load_ns_offset, uint32_t cfg_tod_ns_offset);
bdmf_error_t ag_drv_tod_config_5_get(bdmf_boolean *cfg_tod_load_ns_offset, uint32_t *cfg_tod_ns_offset);
bdmf_error_t ag_drv_tod_msb_get(uint16_t *ts48_wan_read_msb);
bdmf_error_t ag_drv_tod_lsb_get(uint32_t *ts48_wan_read_lsb);
bdmf_error_t ag_drv_tod_ts64_msb_get(uint32_t *ts64_wan_read_msb);
bdmf_error_t ag_drv_tod_ts64_lsb_get(uint32_t *ts64_wan_read_lsb);
bdmf_error_t ag_drv_tod_status_0_get(uint16_t *ts16_ref_synce_read);
bdmf_error_t ag_drv_tod_status_1_get(uint16_t *ts16_mac_tx_read, uint16_t *ts16_mac_rx_read);

#ifdef USE_BDMF_SHELL
enum
{
    cli_tod_config_0,
    cli_tod_config_1,
    cli_tod_config_2,
    cli_tod_config_3,
    cli_tod_config_4,
    cli_tod_config_5,
    cli_tod_msb,
    cli_tod_lsb,
    cli_tod_ts64_msb,
    cli_tod_ts64_lsb,
    cli_tod_status_0,
    cli_tod_status_1,
};

int bcm_tod_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_tod_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

