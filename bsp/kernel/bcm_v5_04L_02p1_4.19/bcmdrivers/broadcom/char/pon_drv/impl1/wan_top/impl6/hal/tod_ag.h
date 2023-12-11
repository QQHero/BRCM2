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
/* cfg_ts48_mac_select: TS48_MAC_SELECT - This field selects the MAC that the timestamp comes fro */
/*                      m.0: EPON1: 10GEPON2: GPON3: NGPON/XGPON4: Active EthernetOthers: reserve */
/*                      dReset value is 0x0.                                                      */
/* cfg_ts48_enable: TS48_ENABLE - All TS48 config signals will be sampled on the rising edge of t */
/*                  hissignal. To change any of the other configuration fields, softwaremust clea */
/*                  r and assert this bit again.Reset value is 0x0.                               */
/* cfg_ts48_offset: TS48_OFFSET - The lower 10-bits of the timestamp, to be applied aftersynchron */
/*                  izing to the 250 MHz domain.Reset value is 0x0.                               */
/* cfg_ts48_read: TS48_READ - The TS48 will be captured on the rising edge of this signal.Reset v */
/*                alue is 0x0.                                                                    */
/**************************************************************************************************/
typedef struct
{
    uint8_t cfg_ts48_mac_select;
    bdmf_boolean cfg_ts48_enable;
    uint16_t cfg_ts48_offset;
    bdmf_boolean cfg_ts48_read;
} tod_config_0;


/**************************************************************************************************/
/* rempty_gpon: rempty_gpon - TBD                                                                 */
/* rempty_epon: rempty_epon - TBD                                                                 */
/* rempty_ngpon: rempty_ngpon - TBD                                                               */
/* rempty_10g_epon: rempty_10g_epon - TBD                                                         */
/* rempty_ae: rempty_ae - TBD                                                                     */
/* rempty_rx: rempty_rx - TBD                                                                     */
/* rempty_tx: rempty_tx - TBD                                                                     */
/* rempty_ref: rempty_ref - TBD                                                                   */
/* rempty_mac_rx: rempty_mac_rx - TBD                                                             */
/* rempty_mac_tx: rempty_mac_tx - TBD                                                             */
/* wfull_gpon: wfull_gpon - TBD                                                                   */
/* wfull_epon: wfull_epon - TBD                                                                   */
/* wfull_ngpon: wfull_ngpon - TBD                                                                 */
/* wfull_10g_epon: wfull_10g_epon - TBD                                                           */
/* wfull_ae: wfull_ae - TBD                                                                       */
/* wfull_rx: wfull_rx - TBD                                                                       */
/* wfull_tx: wfull_tx - TBD                                                                       */
/* wfull_ref: wfull_ref - TBD                                                                     */
/* wfull_mac_rx: wfull_mac_rx - TBD                                                               */
/* wfull_mac_tx: wfull_mac_tx - TBD                                                               */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean rempty_gpon;
    bdmf_boolean rempty_epon;
    bdmf_boolean rempty_ngpon;
    bdmf_boolean rempty_10g_epon;
    bdmf_boolean rempty_ae;
    bdmf_boolean rempty_rx;
    bdmf_boolean rempty_tx;
    bdmf_boolean rempty_ref;
    bdmf_boolean rempty_mac_rx;
    bdmf_boolean rempty_mac_tx;
    bdmf_boolean wfull_gpon;
    bdmf_boolean wfull_epon;
    bdmf_boolean wfull_ngpon;
    bdmf_boolean wfull_10g_epon;
    bdmf_boolean wfull_ae;
    bdmf_boolean wfull_rx;
    bdmf_boolean wfull_tx;
    bdmf_boolean wfull_ref;
    bdmf_boolean wfull_mac_rx;
    bdmf_boolean wfull_mac_tx;
} tod_tod_fifo_status;

bdmf_error_t ag_drv_tod_config_0_set(const tod_config_0 *config_0);
bdmf_error_t ag_drv_tod_config_0_get(tod_config_0 *config_0);
bdmf_error_t ag_drv_tod_msb_get(uint16_t *ts48_wan_read_msb);
bdmf_error_t ag_drv_tod_lsb_get(uint32_t *ts48_wan_read_lsb);
bdmf_error_t ag_drv_tod_tod_config_2_set(uint16_t tx_offset, uint16_t rx_offset);
bdmf_error_t ag_drv_tod_tod_config_2_get(uint16_t *tx_offset, uint16_t *rx_offset);
bdmf_error_t ag_drv_tod_tod_config_3_set(bdmf_boolean ts48_fifo_dis, uint8_t ts48_fifo_ld_rate, uint16_t ref_offset);
bdmf_error_t ag_drv_tod_tod_config_3_get(bdmf_boolean *ts48_fifo_dis, uint8_t *ts48_fifo_ld_rate, uint16_t *ref_offset);
bdmf_error_t ag_drv_tod_tod_status_0_get(uint16_t *ts16_tx_read, uint16_t *ts16_rx_read);
bdmf_error_t ag_drv_tod_tod_status_1_get(uint16_t *ts16_rx_synce_read, uint16_t *ts16_ref_synce_read);
bdmf_error_t ag_drv_tod_tod_status_2_get(uint16_t *ts16_mac_tx_read, uint16_t *ts16_mac_rx_read);
bdmf_error_t ag_drv_tod_tod_fifo_status_set(const tod_tod_fifo_status *tod_fifo_status);
bdmf_error_t ag_drv_tod_tod_fifo_status_get(tod_tod_fifo_status *tod_fifo_status);

#ifdef USE_BDMF_SHELL
enum
{
    cli_tod_config_0,
    cli_tod_msb,
    cli_tod_lsb,
    cli_tod_tod_config_2,
    cli_tod_tod_config_3,
    cli_tod_tod_status_0,
    cli_tod_tod_status_1,
    cli_tod_tod_status_2,
    cli_tod_tod_fifo_status,
};

int bcm_tod_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_tod_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

