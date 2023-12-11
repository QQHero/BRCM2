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
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_sig_prbs_status_clr:  - Active high 1 cy */
/*                                                                 %s%-100scle clear signal, use  */
/*                                                                 %s%-100sthe common clear signa */
/*                                                                 %s%-100sl forthe lock_lost_lh  */
/*                                                                 %s%-100sand the error counter  */
/*                                                                 %s%-100sas counter MSB will be */
/*                                                                 %s%-100s in thesame address as */
/*                                                                 %s%-28s LOCK_LOST_LH.          */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_inv:  - 0: Do nothing.  1: Invert all da */
/*                                                            ta bits.                            */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_ool_cnt:  - Specifies the number of cons */
/*                                                                ecutive valid clock cycles with */
/*                                                                 1 ormore bit errors before PRB */
/*                                                                S checker goes out of PRBS lock */
/*                                                                 state.  0indicates that PRBS w */
/*                                                                ill go out of lock as soon as i */
/*                                                                t gets the firstclock with any  */
/*                                                                error.  Likewise 31 indicates t */
/*                                                                hat PRBS will go outof lock as  */
/*                                                                soon as it gets the 32 consecut */
/*                                                                ive clocks with 1 or more.      */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_lock_cnt:  - Specifies the number of con */
/*                                                                 secutive valid clock cycles wi */
/*                                                                 thout anyerror before PRBS che */
/*                                                                 cker goes into PRBS lock state */
/*                                                                 . Valid valuesare 0 to 31 wher */
/*                                                                 e 0 indicate that PRBS will lo */
/*                                                                 ck as soon as it getsthe first */
/*                                                                  clock with no error. Likewise */
/*                                                                  31 indicates that PRBS willlo */
/*                                                                 ck as soon as it gets the 32 c */
/*                                                                 onsecutive clocks with no erro */
/*                                                                 %s%-36sr.                      */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_err_cnt_burst_mode:  - 0: will count in  */
/*                                                                 %s%-36sbit mode.  1: will coun */
/*                                                                 %s%-29st in burst mode.        */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_mode_sel:  - 3'd0: PRBS73'd1: PRBS93'd2: */
/*                                                                  PRBS113'd3: PRBS153'd4: PRBS2 */
/*                                                                 33'd5: PRBS313'd6: PRBS58 (1 + */
/*                                                                 %s%-36s x^39 + x^58)3'd7: Rese */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timeout:  - prbs_chk_en timeout value */
/*                                                                 %s, range 0 to 31 which maps t */
/*                                                                 %s%-34so 0 to 448.             */
/* cr_xgwan_top_wan_misc_gpon_gearbox_rg_prbs_chk_ctrl_0_en_timer_mode:  - 2'b0X: disable prbs_ch */
/*                                                                 %s%-3k_en timer.  2'b10: user  */
/*                                                                 %s%-3heartbeat_toggle_1us.2'b1 */
/*                                                                 %s%-31: use heartbeat_toggle_1 */
/*                                                                 %s%-3ms.  Actually, both heart */
/*                                                                 %s%-3beat modesare the same, a */
/*                                                                 %s%-3s the count of cycles is  */
/*                                                                 %s%-3determined by counter val */
/*                                                                 %s%-31suefield in chk_ctrl_1.  */
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

#ifdef USE_BDMF_SHELL
enum
{
    cli_gpon_gearbox_status_gearbox_status,
    cli_gpon_gearbox_status_gearbox_prbs_control_0,
    cli_gpon_gearbox_status_gearbox_prbs_control_1,
};

int bcm_gpon_gearbox_status_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_gpon_gearbox_status_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

