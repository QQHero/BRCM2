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

#ifndef _GPON_GEARBOX_STATUS_AG_H_
#define _GPON_GEARBOX_STATUS_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr:  - TBD              */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv:  - TBD                              */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt:  - TBD                          */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt:  - TBD                         */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode:  - TBD               */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel:  - TBD                         */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout:  - TBD                       */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode:  - TBD                    */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode;
} gpon_gearbox_status_gearbox_prbs_control_0;

bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_status_get(uint32_t *cr_rd_data_clx);
bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_set(const gpon_gearbox_status_gearbox_prbs_control_0 *gearbox_prbs_control_0);
bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_control_0_get(gpon_gearbox_status_gearbox_prbs_control_0 *gearbox_prbs_control_0);
bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_set(bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, uint32_t cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);
bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_control_1_get(bdmf_boolean *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_en, uint8_t *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_mode, uint32_t *cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_1_timer_val);
bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_status_0_get(uint32_t *gpon_gearbox_prbs_stat_0_vector);
bdmf_error_t ag_drv_gpon_gearbox_status_gearbox_prbs_status_1_get(uint8_t *gpon_gearbox_prbs_stat_1_vector);

#ifdef USE_BDMF_SHELL
enum
{
    cli_gpon_gearbox_status_gearbox_status,
    cli_gpon_gearbox_status_gearbox_prbs_control_0,
    cli_gpon_gearbox_status_gearbox_prbs_control_1,
    cli_gpon_gearbox_status_gearbox_prbs_status_0,
    cli_gpon_gearbox_status_gearbox_prbs_status_1,
};

int bcm_gpon_gearbox_status_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_gpon_gearbox_status_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

