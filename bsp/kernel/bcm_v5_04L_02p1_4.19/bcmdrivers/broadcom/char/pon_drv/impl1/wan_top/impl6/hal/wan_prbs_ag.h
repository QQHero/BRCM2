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

#ifndef _WAN_PRBS_AG_H_
#define _WAN_PRBS_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* en_timer_mode: EN_TIMER_MODE - 0x - disable prbs_chk_en timer                                  */
/*                                   10 - use heartbeat_toggle_1us                                */
/*                                        11 - use heartbeat_toggle_1msActually both heartbeat mo */
/*                des are the same, as the count of cycles is determined by counter value field i */
/*                n chk_ctrl_1 register.                                                          */
/* en_timeout: EN_TIMEOUT - prbs_chk_en timeout value, range 0 to 31 which maps to 0 to 448       */
/* mode_sel: MODE_SEL - 3d0 -> PRBS 73d1 -> PRBS 9                                                */
/*                     3d2 -> PRBS 11                                                         3d3 */
/*            -> PRBS 15                                                         3d4 -> PRBS 23   */
/*                                                                  3d5 -> PRBS 31                */
/*                                                3d6 -> PRBS 58 (1 + x^39 + x^58)                */
/*                                               3d7 -> reserved                                  */
/* err_cnt_burst_mode: ERR_CNT_BURST_MODE - 0 will count in bit mode (default)1 will count in bur */
/*                     st mode                                                                    */
/* lock_cnt: LOCK_CNT - Specifies the number of consecutive valid clock cycles without any error  */
/*           before PRBS checker goes into PRBS lock state. Valid values are 0 to 31 where 0 indi */
/*           cate that PRBS will lock as soon as it gets the first clock with no error. Likewise  */
/*           31 indicates that PRBS will lock as soon as it gets the 32 consecutive clocks with n */
/*           o error.                                                                             */
/* ool_cnt: OOL_CNT - Specifies the number of consecutive valid clock cycles with 1 or more bit e */
/*          rrors before PRBS checker goes out of PRBS lock state.0 indicate that PRBS will go ou */
/*          t of lock as soon as it gets the first clock with any error.Likewise 31 indicates tha */
/*          t PRBS will go out of lock as soon as it gets the 32 consecutive clocks with 1 or mor */
/*          e                                                                                     */
/* inv: INV - 1 will invert all the data bits.                                                    */
/* sig_prbs_status_clr: SIG_PRBS_STATUS_CLR - Active high 1 cycle clear signal, use the common cl */
/*                      ear signal for the lock_lost_lh and the error counter as counter MSB will */
/*                       be in the same address as LOCK_LOST_LH                                   */
/**************************************************************************************************/
typedef struct
{
    uint8_t en_timer_mode;
    uint8_t en_timeout;
    uint8_t mode_sel;
    bdmf_boolean err_cnt_burst_mode;
    uint8_t lock_cnt;
    uint8_t ool_cnt;
    bdmf_boolean inv;
    bdmf_boolean sig_prbs_status_clr;
} wan_prbs_wan_prbs_chk_ctrl_0;

bdmf_error_t ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_set(const wan_prbs_wan_prbs_chk_ctrl_0 *wan_prbs_chk_ctrl_0);
bdmf_error_t ag_drv_wan_prbs_wan_prbs_chk_ctrl_0_get(wan_prbs_wan_prbs_chk_ctrl_0 *wan_prbs_chk_ctrl_0);
bdmf_error_t ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_set(bdmf_boolean prbs_chk_en, uint8_t prbs_chk_mode, uint32_t prbs_timer_val);
bdmf_error_t ag_drv_wan_prbs_wan_prbs_chk_ctrl_1_get(bdmf_boolean *prbs_chk_en, uint8_t *prbs_chk_mode, uint32_t *prbs_timer_val);
bdmf_error_t ag_drv_wan_prbs_wan_prbs_status_0_get(bdmf_boolean *lock_lost_lh, uint32_t *err_cnt);
bdmf_error_t ag_drv_wan_prbs_wan_prbs_status_1_get(bdmf_boolean *any_err, bdmf_boolean *lock);

#ifdef USE_BDMF_SHELL
enum
{
    cli_wan_prbs_wan_prbs_chk_ctrl_0,
    cli_wan_prbs_wan_prbs_chk_ctrl_1,
    cli_wan_prbs_wan_prbs_status_0,
    cli_wan_prbs_wan_prbs_status_1,
};

int bcm_wan_prbs_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_wan_prbs_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

