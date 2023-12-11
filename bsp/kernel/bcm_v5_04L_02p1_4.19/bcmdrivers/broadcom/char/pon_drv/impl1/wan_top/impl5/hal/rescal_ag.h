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

#ifndef _RESCAL_AG_H_
#define _RESCAL_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* ctrl: CTRL - Connects to i_rescal_ctrl.Reset value is 0x0.                                     */
/* pwrdn: PWRDN - Connects to i_pwrdn.Reset value is 0x1.                                         */
/* diag_on: DIAG_ON - Connects to i_diag_on.Reset value is 0x0.                                   */
/* rstb: RSTB - Connects to i_rstb.Reset value is 0x0.                                            */
/**************************************************************************************************/
typedef struct
{
    uint16_t ctrl;
    bdmf_boolean pwrdn;
    bdmf_boolean diag_on;
    bdmf_boolean rstb;
} rescal_rescal_cfg;


/**************************************************************************************************/
/* valid: VALID - Connects to o_valid.Reset value is 0x0.                                         */
/* comp: COMP - Connects to o_rescalcomp.Reset value is 0x0.                                      */
/* state: STATE - Connects to o_rescal_state.Reset value is 0x0.                                  */
/* ctrl_dfs: CTRL_DFS - Connects to o_rescal_ctrl_dfs.Reset value is 0x0.                         */
/* prev_comp_cnt: PREV_COMP_CNT - Connects to o_prev_comp_cnt.Reset value is 0x0.                 */
/* pon: PON - Connects to o_pon.Reset value is 0x0.                                               */
/* done: DONE - Connects to o_done.Reset value is 0x0.                                            */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean valid;
    bdmf_boolean comp;
    uint8_t state;
    uint16_t ctrl_dfs;
    uint8_t prev_comp_cnt;
    uint8_t pon;
    bdmf_boolean done;
} rescal_rescal_status_0;

bdmf_error_t ag_drv_rescal_rescal_cfg_set(const rescal_rescal_cfg *rescal_cfg);
bdmf_error_t ag_drv_rescal_rescal_cfg_get(rescal_rescal_cfg *rescal_cfg);
bdmf_error_t ag_drv_rescal_rescal_status_0_get(rescal_rescal_status_0 *rescal_status_0);
bdmf_error_t ag_drv_rescal_rescal_status1_get(uint8_t *curr_comp_cnt);

#ifdef USE_BDMF_SHELL
enum
{
    cli_rescal_rescal_cfg,
    cli_rescal_rescal_status_0,
    cli_rescal_rescal_status1,
};

int bcm_rescal_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_rescal_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

