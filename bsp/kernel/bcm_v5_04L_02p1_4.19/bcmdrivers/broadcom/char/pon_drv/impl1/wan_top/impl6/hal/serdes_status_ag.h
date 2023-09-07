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

#ifndef _SERDES_STATUS_AG_H_
#define _SERDES_STATUS_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* pmd_pll0_lock: PMD_PLL0_LOCK - Assertion of this signal indicates that the pll has achieved lo */
/*                ck.Reset value is 0x0.                                                          */
/* pmd_rx_lock_0_invert: PMD_RX_LOCK_0_INVERT - Assertion of this signal indicates that the pll h */
/*                       as not achievedlock.Reset value is 0x0.                                  */
/* pmd_rx_clk_vld_0: PMD_RX_CLK_VLD_0 - Receive clock valid.Reset value is 0x0.                   */
/* pmd_tx_clk_vld: PMD_TX_CLK_VLD - Transmit clock valid.Reset value is 0x0.                      */
/* pmd_rx_lock_0: PMD_RX_LOCK_0 - Receive PMD lock. WHen this signal is low, the receiver isacqui */
/*                ring lock. During this period, the phase of the receive clockand alignment of d */
/*                ata are not reliable.Reset value is 0x0.                                        */
/* pmd_energy_detect_0: PMD_ENERGY_DETECT_0 - EEE energy detect.Reset value is 0x0.               */
/* pmd_signal_detect_0: PMD_SIGNAL_DETECT_0 - Signal detect status from the analog. This signal i */
/*                      s not related toany interface clock or data validity.Reset value is 0x0.  */
/* pmi_lp_acknowledge: PMI_LP_ACKNOWLEDGE - Ack response back from the RMIC slave indicating that */
/*                      the write orread transaction is complete. This signal is driven in the re */
/*                     gistersblocks clock domain and should be treated as an asynchronous inputb */
/*                     y the master.Reset value is 0x0.                                           */
/* pmi_lp_error: PMI_LP_ERROR - Error response from RMIC slave indicating an address error whichm */
/*               eans that either the block address does not exist or that the deviddid not match */
/*                the strap value. The ack signal indicates that thetransaction is complete and t */
/*               he error signal indicates that therewas an address error with this transaction.  */
/*               This signal is assertedalong with the ack signal and should be treated an asynch */
/*               ronoussignal the same way as the ack signal.Reset value is 0x0.                  */
/* o_laser_burst_en: O_LASER_BURST_EN - If set, the SERDES is attempting to enable the laser. The */
/*                    actualstate of the laser also depends on the laser output enable.Reset valu */
/*                   e is 0x0.                                                                    */
/* pmd_pll1_lock: PMD_PLL1_LOCK - Assertion of this signal indicates that the pll has achieved lo */
/*                ck.Reset value is 0x0.                                                          */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean pmd_pll0_lock;
    bdmf_boolean pmd_rx_lock_0_invert;
    bdmf_boolean pmd_rx_clk_vld_0;
    bdmf_boolean pmd_tx_clk_vld;
    bdmf_boolean pmd_rx_lock_0;
    bdmf_boolean pmd_energy_detect_0;
    bdmf_boolean pmd_signal_detect_0;
    bdmf_boolean pmi_lp_acknowledge;
    bdmf_boolean pmi_lp_error;
    bdmf_boolean o_laser_burst_en;
    bdmf_boolean pmd_pll1_lock;
} serdes_status_serdes_status_status;

bdmf_error_t ag_drv_serdes_status_serdes_status_status_get(serdes_status_serdes_status_status *serdes_status_status);

#ifdef USE_BDMF_SHELL
enum
{
    cli_serdes_status_serdes_status_status,
};

int bcm_serdes_status_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_serdes_status_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

