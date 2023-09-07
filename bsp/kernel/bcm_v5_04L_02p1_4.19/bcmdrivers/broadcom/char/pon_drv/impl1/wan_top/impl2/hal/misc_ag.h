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

#ifndef _MISC_AG_H_
#define _MISC_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* cr_xgwan_top_wan_misc_pmd_lane_mode:  - Reserved mode bus for lane. Mode bus for lane used by  */
/*                                      the PCS tocommunicate lane info to PMD. This bus should o */
/*                                      nly be written towhen the lane is in reset since the firm */
/*                                      ware will only read thisafter coming out of reset.  This  */
/*                                      signal will be latched to a lanebased register during cor */
/*                                      e_dp_rstb. Asynchronous signal to the PMD                 */
/* cr_xgwan_top_wan_misc_epon_tx_fifo_off:  - TXFIFO OFF LOAD signal for EPON's gearbox.          */
/* cr_xgwan_top_wan_misc_onu2g_phya:  - Strap input for selecting the port address to decode on t */
/*                                   he mdiotransaction.                                          */
/* cr_xgwan_top_wan_misc_epon_tx_fifo_off_ld:  - Load vlue in cr_xgwan_top_wan_misc_epon_tx_fifo_ */
/*                                            off.                                                */
/* cr_xgwan_top_wan_misc_mdio_fast_mode:  - Debug bit that disables the 32-bit preamble to allow  */
/*                                       mdio frames torun at 2x speed.                           */
/* cr_xgwan_top_wan_misc_mdio_mode:  - MDIO transaction indicator needs to be asserted in a confi */
/*                                  gurationwhere an external mdio controller is trying to access */
/*                                   the internalPMD registers directly.                          */
/* cr_xgwan_top_wan_misc_refout_en:  - Not used in this chip.                                     */
/* cr_xgwan_top_wan_misc_refin_en:  - Not used in this chip.                                      */
/* cr_xgwan_top_wan_misc_onu2g_pmd_status_sel:  - Not used in this chip.                          */
/**************************************************************************************************/
typedef struct
{
    uint16_t cr_xgwan_top_wan_misc_pmd_lane_mode;
    uint8_t cr_xgwan_top_wan_misc_epon_tx_fifo_off;
    uint8_t cr_xgwan_top_wan_misc_onu2g_phya;
    bdmf_boolean cr_xgwan_top_wan_misc_epon_tx_fifo_off_ld;
    bdmf_boolean cr_xgwan_top_wan_misc_mdio_fast_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_mdio_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_refout_en;
    bdmf_boolean cr_xgwan_top_wan_misc_refin_en;
    bdmf_boolean cr_xgwan_top_wan_misc_onu2g_pmd_status_sel;
} misc_misc_0;


/**************************************************************************************************/
/* cfg_apm_mux_sel_1:  - 0: Select MUX 0 output for wan_rbc_for_apm. 1: ntr_sync_pulse forwan_rbc */
/*                    _for_apm.                                                                   */
/* cfg_apm_mux_sel_0:  - 0: Select ncoProgClk for MUX 0 output.  1: Select ncoClk8KHz for MUX0 ou */
/*                    tput.                                                                       */
/* cfgngponrxclk:  - 0: Selects divide-by-2 clock divider for mac_rx_clk.  1: Selectsdivide-by-4  */
/*                clock divider for mac_rx_clk. 2: Selects divide-by-1clock divider. 3: Unused.   */
/* cfgngpontxclk:  - 0: Selects divide-by-2 clock divider for mac_tx_clk.  1: Selectsdivide-by-4  */
/*                clock divider for mac_tx_clk. 2: Selects divide-by-1clock divider. 3: Unused.   */
/* cfgactiveethernet2p5:  - 0: Selects divide-by-2 clock divider for clkRbc125. 1: Selectsdivide- */
/*                       by-1 clock divider for clkRbc125.                                        */
/* cr_xgwan_top_wan_misc_pmd_rx_osr_mode:  - Oversample mode for rx lane. Asynchronous signal to  */
/*                                        the PMD. 0 -OSR1. 1 - OSR2.                             */
/* cr_xgwan_top_wan_misc_pmd_tx_mode:  - EEE tx mode function for lane. Asynchronous signal to th */
/*                                    e PMD.                                                      */
/* cr_xgwan_top_wan_misc_pmd_tx_osr_mode:  - Oversample mode for tx lane. Asynchronous signal to  */
/*                                        the PMD. 0 -OSR1.  1 - OSR2.                            */
/* cr_xgwan_top_wan_misc_pmd_tx_disable:  - Pmd_tx_disable is asserted to squelch the transmit si */
/*                                       gnal for lane.                                           */
/* cr_xgwan_top_wan_misc_pmd_ln_rx_h_pwrdn:  - Lane RX power down. Minimum assertion time: 25 com */
/*                                          clk period.                                           */
/* cr_xgwan_top_wan_misc_pmd_ln_tx_h_pwrdn:  - Lane TX power down. Minimum assertion time: 25 com */
/*                                          clk period.                                           */
/* cr_xgwan_top_wan_misc_pmd_ext_los:  - External Loss of signal. LOS = 1. Signal presence = 0.   */
/* cr_xgwan_top_wan_misc_pmd_por_h_rstb:  - PMD main reset, resets registers, data path for entir */
/*                                       e coreincluding all lanes. Active Low. Minimum assertion */
/*                                        time: 25 comclkperiod.                                  */
/* cr_xgwan_top_wan_misc_pmd_core_1_dp_h_rstb:  - Core reset for datapath for all lanes and corre */
/*                                             sponding PLL. Doesnot reset registers. Active Low. */
/*                                              Minimum assertion time: 25 comclkperiod.          */
/* cr_xgwan_top_wan_misc_pmd_core_0_dp_h_rstb:  - Core reset for datapath for all lanes and corre */
/*                                             sponding PLL. Doesnot reset registers. Active Low. */
/*                                              Minimum assertion time: 25 comclkperiod.          */
/* cr_xgwan_top_wan_misc_pmd_ln_h_rstb:  - Lane reset registers and data path. Active Low. Minimu */
/*                                      m assertiontime: 25 comclk period.                        */
/* cr_xgwan_top_wan_misc_pmd_ln_dp_h_rstb:  - Lane datapath reset, does not reset registers. Acti */
/*                                         ve Low. Minimumassertion time: 25 comclk period.       */
/* cr_xgwan_top_wan_misc_pmd_rx_mode:  - EEE rx mode function for lane. Asynchronous signal to th */
/*                                    e PMD                                                       */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cfg_apm_mux_sel_1;
    bdmf_boolean cfg_apm_mux_sel_0;
    uint8_t cfgngponrxclk;
    uint8_t cfgngpontxclk;
    bdmf_boolean cfgactiveethernet2p5;
    uint8_t cr_xgwan_top_wan_misc_pmd_rx_osr_mode;
    uint8_t cr_xgwan_top_wan_misc_pmd_tx_mode;
    uint8_t cr_xgwan_top_wan_misc_pmd_tx_osr_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_tx_disable;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ln_rx_h_pwrdn;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ln_tx_h_pwrdn;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ext_los;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_por_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_core_1_dp_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_core_0_dp_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ln_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ln_dp_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_rx_mode;
} misc_misc_2;


/**************************************************************************************************/
/* cr_xgwan_top_wan_misc_epon_tx_fifo_off_hi:  - Additional higher order TXFIFO OFF LOAD bits for */
/*                                             EPON's gearbox:cr_xgwan_top_wan_misc_epon_tx_fifo_ */
/*                                            off_hi[5:3]cr_xgwan_top_wan_misc_epon_tx_fifo_off[2 */
/*                                            :0]                                                 */
/* cr_xgwan_top_wan_misc_epon_gbox_ae_2p5_full_rate_mode:  - 0: All other modes 1: Explicit 2.5G  */
/*                                                        Full Rate Serdes Mode                   */
/* cr_xgwan_top_wan_misc_epon_gbox_pon_rx_width_mode:  - 0: 10b mode, 1G Operation 1: 20b mode, 2 */
/*                                                    G Operation                                 */
/* cr_xgwan_top_wan_misc_wan_cfg_epon_debug_sel:  - Not used.                                     */
/* cr_xgwan_top_wan_misc_wan_cfg_wan_debug_sel:  - This field selects the block that the 40-bit d */
/*                                              ebug bus comesfrom.0: GPON1: EPON2: NGPON_03: SER */
/*                                              DES4: NGPON_15: BBH RX6: BBH TX                   */
/* cr_xgwan_top_wan_misc_wan_cfg_ntr_sync_period_sel:  - Selects APM clock generation division.   */
/* cr_xgwan_top_wan_misc_wan_cfg_laser_oe:  - 0: Output enable for laser is disabled.  1: Output  */
/*                                         enable for laseris enabled.                            */
/* cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select:  - The lower bit selects the speed, and th */
/*                                                     e upper bit selects thetechnology. 0: EPON */
/*                                                     . 1: 10G EPON. 2: GPON. 3: NGPON.          */
/* cr_xgwan_top_wan_misc_wan_cfg_laser_mode:  - Bit 0 selects the speed, and bit 1 selects the te */
/*                                           chnology.  0: EPON.1: 10G EPON. 2: GPON. 3: NGPON. 4 */
/*                                            (or higher): Disable laser.                         */
/* cr_xgwan_top_wan_misc_wan_cfg_laser_invert:  - 0: Normal operation.  1: Invert laser enable.   */
/* cr_xgwan_top_wan_misc_wan_cfg_mem_reb:  - REB going to WAN memories.                           */
/**************************************************************************************************/
typedef struct
{
    uint8_t cr_xgwan_top_wan_misc_epon_tx_fifo_off_hi;
    bdmf_boolean cr_xgwan_top_wan_misc_epon_gbox_ae_2p5_full_rate_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_epon_gbox_pon_rx_width_mode;
    uint8_t cr_xgwan_top_wan_misc_wan_cfg_epon_debug_sel;
    uint8_t cr_xgwan_top_wan_misc_wan_cfg_wan_debug_sel;
    uint8_t cr_xgwan_top_wan_misc_wan_cfg_ntr_sync_period_sel;
    bdmf_boolean cr_xgwan_top_wan_misc_wan_cfg_laser_oe;
    uint8_t cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select;
    uint8_t cr_xgwan_top_wan_misc_wan_cfg_laser_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_wan_cfg_laser_invert;
    bdmf_boolean cr_xgwan_top_wan_misc_wan_cfg_mem_reb;
} misc_misc_3;

bdmf_error_t ag_drv_misc_misc_0_set(const misc_misc_0 *misc_0);
bdmf_error_t ag_drv_misc_misc_0_get(misc_misc_0 *misc_0);
bdmf_error_t ag_drv_misc_misc_1_set(uint16_t cr_xgwan_top_wan_misc_pmd_core_1_mode, uint16_t cr_xgwan_top_wan_misc_pmd_core_0_mode);
bdmf_error_t ag_drv_misc_misc_1_get(uint16_t *cr_xgwan_top_wan_misc_pmd_core_1_mode, uint16_t *cr_xgwan_top_wan_misc_pmd_core_0_mode);
bdmf_error_t ag_drv_misc_misc_2_set(const misc_misc_2 *misc_2);
bdmf_error_t ag_drv_misc_misc_2_get(misc_misc_2 *misc_2);
bdmf_error_t ag_drv_misc_misc_3_set(const misc_misc_3 *misc_3);
bdmf_error_t ag_drv_misc_misc_3_get(misc_misc_3 *misc_3);

#ifdef USE_BDMF_SHELL
enum
{
    cli_misc_misc_0,
    cli_misc_misc_1,
    cli_misc_misc_2,
    cli_misc_misc_3,
};

int bcm_misc_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_misc_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

