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
/* cfg_ts48_pre_sync_fifo_disable:  - 0: New mode. Transfer TS48 using FIFO. 1: Legacy mode.  Tra */
/*                                 nsferupper TS48 bits between clock domains.                    */
/* cfg_ts48_pre_sync_fifo_load_rate:  - Number of clock ticks between consecutive writes to the T */
/*                                   S48 FIFO.                                                    */
/* cfg_tod_pps_clear:  - Allow 1PPS pulse to clear the counter if set.  If not set, the 1PPSpulse */
/*                     will have no effect on the TS48.                                           */
/* cfg_ts48_read:  - The TS48 will be captured on the rising edge of this signal.                 */
/* cfg_ts48_offset:  - The lower 10-bits of the timestamp, to be applied aftersynchronizing to th */
/*                  e 250 MHz domain.                                                             */
/* cfg_ts48_enable:  - All TS48 config signals will be sampled on the rising edge of thissignal.  */
/*                   To change any of the other configuration fields, softwaremust clear and asse */
/*                  rt this bit again.                                                            */
/* cfg_ts48_mac_select:  - This field selects the MAC that the timestamp comes from.0: EPON1: 10G */
/*                       EPON2: GPON3: NGPON4: Active Ethernet5-7: Reserved                       */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cfg_ts48_pre_sync_fifo_disable;
    uint8_t cfg_ts48_pre_sync_fifo_load_rate;
    bdmf_boolean cfg_tod_pps_clear;
    bdmf_boolean cfg_ts48_read;
    uint16_t cfg_ts48_offset;
    bdmf_boolean cfg_ts48_enable;
    uint8_t cfg_ts48_mac_select;
} tod_config_0;


/**************************************************************************************************/
/* cfg_tod_load_ns:  - When this bit is set, hardware will update the internal nanosecondcounter, */
/*                   cfg_tod_ns[31:0], when the local MPCP time equalscfg_tod_mpcp[31:0]. Softwar */
/*                  e should set this bit and wait untilhardware clears it before setting it agai */
/*                  n.                                                                            */
/* cfg_tod_epon_read:  - When this bit is set, hardware will latch the internal ts48, ns, andseco */
/*                    nds counters. Software should set this bit and wait untilhardware clears it */
/*                     before setting it again. Once hardware hascleared the bit, the timers are  */
/*                    available to be read.                                                       */
/* cfg_tod_epon_read_sel:  - Select the block to read the timers from.  0: Reserved. 1: 1G EPON.2 */
/*                        :10G EPON. 3: AE. This field should not be changed whilecfg_tod_read is */
/*                         set.                                                                   */
/* cfg_tod_load:  - The rising edge will be latched, and cfg_tod_seconds will be loadedon the nex */
/*               t 1PPS pulse or when the next second rolls over.                                 */
/* cfg_tod_seconds:  - Number of seconds to be loaded.                                            */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cfg_tod_load_ns;
    bdmf_boolean cfg_tod_epon_read;
    uint8_t cfg_tod_epon_read_sel;
    bdmf_boolean cfg_tod_load;
    uint32_t cfg_tod_seconds;
} tod_config_1;

bdmf_error_t ag_drv_tod_config_0_set(const tod_config_0 *config_0);
bdmf_error_t ag_drv_tod_config_0_get(tod_config_0 *config_0);
bdmf_error_t ag_drv_tod_config_1_set(const tod_config_1 *config_1);
bdmf_error_t ag_drv_tod_config_1_get(tod_config_1 *config_1);
bdmf_error_t ag_drv_tod_msb_get(uint16_t *ts48_wan_read_msb);
bdmf_error_t ag_drv_tod_lsb_get(uint32_t *ts48_wan_read_lsb);
bdmf_error_t ag_drv_tod_config_2_set(uint16_t cfg_tx_offset, uint16_t cfg_rx_offset);
bdmf_error_t ag_drv_tod_config_2_get(uint16_t *cfg_tx_offset, uint16_t *cfg_rx_offset);
bdmf_error_t ag_drv_tod_config_3_set(uint16_t cfg_ref_offset);
bdmf_error_t ag_drv_tod_config_3_get(uint16_t *cfg_ref_offset);
bdmf_error_t ag_drv_tod_status_0_get(uint16_t *ts16_tx_read, uint16_t *ts16_rx_read);
bdmf_error_t ag_drv_tod_status_1_get(uint16_t *ts16_rx_synce_read, uint16_t *ts16_ref_synce_read);
bdmf_error_t ag_drv_tod_status_2_get(uint16_t *ts16_mac_tx_read, uint16_t *ts16_mac_rx_read);
bdmf_error_t ag_drv_tod_ns_set(uint32_t cfg_tod_ns);
bdmf_error_t ag_drv_tod_ns_get(uint32_t *cfg_tod_ns);
bdmf_error_t ag_drv_tod_mpcp_set(uint32_t cfg_tod_mpcp);
bdmf_error_t ag_drv_tod_mpcp_get(uint32_t *cfg_tod_mpcp);

#ifdef USE_BDMF_SHELL
enum
{
    cli_tod_config_0,
    cli_tod_config_1,
    cli_tod_msb,
    cli_tod_lsb,
    cli_tod_config_2,
    cli_tod_config_3,
    cli_tod_status_0,
    cli_tod_status_1,
    cli_tod_status_2,
    cli_tod_ns,
    cli_tod_mpcp,
};

int bcm_tod_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_tod_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

