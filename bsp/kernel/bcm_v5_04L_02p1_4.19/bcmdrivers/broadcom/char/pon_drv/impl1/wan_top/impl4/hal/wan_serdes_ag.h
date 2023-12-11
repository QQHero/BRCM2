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

#ifndef _WAN_SERDES_AG_H_
#define _WAN_SERDES_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* cfg_pll1_lcref_sel:  - 0 - select pll1_lcref.  1 - select pll0_lcref.                          */
/* cfg_pll1_refout_en:  - Enables SERDES to drive the pll1_refout pin.  0 - output is hiZ.  1- ou */
/*                     tput is pad_pll1_refclk.                                                   */
/* cfg_pll1_refin_en:  - Reference select. 0 - select pad_pll1_refclkp/n.  1 - selectpll1_lcrefp/ */
/*                    n.                                                                          */
/* cfg_pll0_lcref_sel:  - 0 - select pll0_lcref.  1 - select pll1_lcref.                          */
/* cfg_pll0_refout_en:  - Enables SERDES to drive the pll0_refout pin.  0 - output is hiZ.  1- ou */
/*                     tput is pad_pll0_refclk.                                                   */
/* cfg_pll0_refin_en:  - Reference select. 0 - select pad_pll0_refclkp/n.  1 - selectpll0_lcrefp/ */
/*                    n.                                                                          */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cfg_pll1_lcref_sel;
    bdmf_boolean cfg_pll1_refout_en;
    bdmf_boolean cfg_pll1_refin_en;
    bdmf_boolean cfg_pll0_lcref_sel;
    bdmf_boolean cfg_pll0_refout_en;
    bdmf_boolean cfg_pll0_refin_en;
} wan_serdes_pll_ctl;


/**************************************************************************************************/
/* cfg_pram_go:  - Perform pRAM operation.  This field is only valid for the B0 orbeyond.  Softwa */
/*              re sets and hardware clears this bit.  Do not writeto this register if this bit i */
/*              s set.                                                                            */
/* cfg_pram_we:  - Program RAM write strobe.                                                      */
/* cfg_pram_cs:  - Program RAM chip select. This field is only valid for the A0 versionof the chi */
/*              p.                                                                                */
/* cfg_pram_ability:  - Ability to support parallel bus interface to access program RAM.  0- not  */
/*                   supported. 1 - supported. This field is only valid for the A0version of the  */
/*                   chip.                                                                        */
/* cfg_pram_datain:  - DEPRECATED. Use the data field in WAN_SERDES_PRAM_CTL_2/3.                 */
/* cfg_pram_addr:  - Program RAM address.                                                         */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cfg_pram_go;
    bdmf_boolean cfg_pram_we;
    bdmf_boolean cfg_pram_cs;
    bdmf_boolean cfg_pram_ability;
    uint8_t cfg_pram_datain;
    uint16_t cfg_pram_addr;
} wan_serdes_pram_ctl;

bdmf_error_t ag_drv_wan_serdes_pll_ctl_set(const wan_serdes_pll_ctl *pll_ctl);
bdmf_error_t ag_drv_wan_serdes_pll_ctl_get(wan_serdes_pll_ctl *pll_ctl);
bdmf_error_t ag_drv_wan_serdes_temp_ctl_get(uint16_t *wan_temperature_data);
bdmf_error_t ag_drv_wan_serdes_pram_ctl_set(const wan_serdes_pram_ctl *pram_ctl);
bdmf_error_t ag_drv_wan_serdes_pram_ctl_get(wan_serdes_pram_ctl *pram_ctl);
bdmf_error_t ag_drv_wan_serdes_pram_ctl_2_set(uint32_t cfg_pram_datain_0);
bdmf_error_t ag_drv_wan_serdes_pram_ctl_2_get(uint32_t *cfg_pram_datain_0);
bdmf_error_t ag_drv_wan_serdes_pram_ctl_3_set(uint32_t cfg_pram_datain_1);
bdmf_error_t ag_drv_wan_serdes_pram_ctl_3_get(uint32_t *cfg_pram_datain_1);

#ifdef USE_BDMF_SHELL
enum
{
    cli_wan_serdes_pll_ctl,
    cli_wan_serdes_temp_ctl,
    cli_wan_serdes_pram_ctl,
    cli_wan_serdes_pram_ctl_2,
    cli_wan_serdes_pram_ctl_3,
};

int bcm_wan_serdes_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_wan_serdes_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

