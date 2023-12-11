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
/* cr_xgwan_top_wan_misc_onu2g_pmd_status_sel: ONU2G_PMD_STATUS_SEL - Not used in this chip.Reset */
/*                                              value is 0x0.                                     */
/* cr_xgwan_top_wan_misc_refin_en: REFIN_EN - Not used in this chip.Reset value is 0x0.           */
/* cr_xgwan_top_wan_misc_refout_en: REFOUT_EN - Not used in this chip.Reset value is 0x0.         */
/* cr_xgwan_top_wan_misc_mdio_mode: MDIO_MODE - MDIO transaction indicator needs to be asserted i */
/*                                  n a configurationwhere an external mdio controller is trying  */
/*                                  to access the internalPMD registers directly.Reset value is 0 */
/*                                  x0.                                                           */
/* cr_xgwan_top_wan_misc_mdio_fast_mode: MDIO_FAST_MODE - Debug bit that disables the 32-bit prea */
/*                                       mble to allow mdio frames torun at 2x speed.Reset value  */
/*                                       is 0x0.                                                  */
/* cr_xgwan_top_wan_misc_epon_tx_fifo_off_ld: EPON_TX_FIFO_OFF_LD - Load vlue in cr_xgwan_top_wan */
/*                                            _misc_epon_tx_fifo_off.                             */
/* cr_xgwan_top_wan_misc_onu2g_phya: ONU2G_PHYA - Strap input for selecting the port address to d */
/*                                   ecode on the mdiotransaction.Reset value is 0x0.             */
/* cr_xgwan_top_wan_misc_epon_gbox_pon_rx_width_mode: EPON_GBOX_PON_RX_WIDTH_MODE - cr_wan_top_wa */
/*                                                    n_misc_wan_top_misc_0_epon_gbox_pon_rx_widt */
/*                                                    h_mode                                      */
/* cr_xgwan_top_wan_misc_epon_gbox_ae_2p5_full_rate_mode: EPON_GBOX_AE_2P5_FULL_RATE_MODE - cr_wa */
/*                                                        n_top_wan_misc_wan_top_misc_0_epon_ae_2 */
/*                                                        p5_full_rate_mode                       */
/* cr_xgwan_top_wan_misc_pmd_lane_mode: PMD_LANE_MODE - Reserved mode bus for lane. Mode bus for  */
/*                                      lane used by the PCS tocommunicate lane info to PMD. This */
/*                                       bus should only be written towhen the lane is in reset s */
/*                                      ince the firmware will only read thisafter coming out of  */
/*                                      reset. This signal will be latched to a lanebased registe */
/*                                      r during core_dp_rstb. Asynchronous signal to the PMDRese */
/*                                      t value is 0x0.                                           */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cr_xgwan_top_wan_misc_onu2g_pmd_status_sel;
    bdmf_boolean cr_xgwan_top_wan_misc_refin_en;
    bdmf_boolean cr_xgwan_top_wan_misc_refout_en;
    bdmf_boolean cr_xgwan_top_wan_misc_mdio_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_mdio_fast_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_epon_tx_fifo_off_ld;
    uint8_t cr_xgwan_top_wan_misc_onu2g_phya;
    bdmf_boolean cr_xgwan_top_wan_misc_epon_gbox_pon_rx_width_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_epon_gbox_ae_2p5_full_rate_mode;
    uint16_t cr_xgwan_top_wan_misc_pmd_lane_mode;
} misc_misc_0;


/**************************************************************************************************/
/* cr_xgwan_top_wan_misc_pmd_rx_mode: PMD_RX_MODE - EEE rx mode function for lane. Asynchronous s */
/*                                    ignal to the PMDReset value is 0x0.                         */
/* cr_xgwan_top_wan_misc_pmd_ln_dp_h_rstb: PMD_LN_DP_H_RSTB - Lane datapath reset, does not reset */
/*                                          registers. Active Low. Minimumassertion time: 25 comc */
/*                                         lk period.Reset value is 0x0.                          */
/* cr_xgwan_top_wan_misc_pmd_ln_h_rstb: PMD_LN_H_RSTB - Lane reset registers and data path. Activ */
/*                                      e Low. Minimum assertiontime: 25 comclk period.Reset valu */
/*                                      e is 0x0.                                                 */
/* cr_xgwan_top_wan_misc_pmd_core_0_dp_h_rstb: PMD_CORE_0_DP_H_RSTB - Core reset for datapath for */
/*                                              all lanes and corresponding PLL. Doesnot reset re */
/*                                             gisters. Active Low. Minimum assertion time: 25 co */
/*                                             mclkperiod.Reset value is 0x0.                     */
/* cr_xgwan_top_wan_misc_pmd_core_1_dp_h_rstb: PMD_CORE_1_DP_H_RSTB - Core reset for datapath for */
/*                                              all lanes and corresponding PLL. Doesnot reset re */
/*                                             gisters. Active Low. Minimum assertion time: 25 co */
/*                                             mclkperiod.Reset value is 0x0.                     */
/* cr_xgwan_top_wan_misc_pmd_por_h_rstb: PMD_POR_H_RSTB - PMD main reset, resets registers, data  */
/*                                       path for entire coreincluding all lanes. Active Low. Min */
/*                                       imum assertion time: 25 comclkperiod.Reset value is 0x0. */
/* cr_xgwan_top_wan_misc_pmd_ext_los: PMD_EXT_LOS - External Loss of signal. LOS = 1. Signal pres */
/*                                    ence = 0.Reset value is 0x0.                                */
/* cr_xgwan_top_wan_misc_pmd_ln_tx_h_pwrdn: PMD_LN_TX_H_PWRDN - Lane TX power down. Minimum asser */
/*                                          tion time: 25 comclk period.Reset value is 0x0.       */
/* cr_xgwan_top_wan_misc_pmd_ln_rx_h_pwrdn: PMD_LN_RX_H_PWRDN - Lane RX power down. Minimum asser */
/*                                          tion time: 25 comclk period.Reset value is 0x0.       */
/* cr_xgwan_top_wan_misc_pmd_tx_disable: PMD_TX_DISABLE - Pmd_tx_disable is asserted to squelch t */
/*                                       he transmit signal for lane.Reset value is 0x0.          */
/* cr_xgwan_top_wan_misc_pmd_tx_osr_mode: PMD_TX_OSR_MODE - Oversample mode for tx lane. Asynchro */
/*                                        nous signal to the PMD. 0 -OSR1. 1 - OSR2.Reset value i */
/*                                        s 0x0.                                                  */
/* cr_xgwan_top_wan_misc_pmd_tx_mode: PMD_TX_MODE - EEE tx mode function for lane. Asynchronous s */
/*                                    ignal to the PMD.Reset value is 0x0.                        */
/* cr_xgwan_top_wan_misc_pmd_rx_osr_mode: PMD_RX_OSR_MODE - Oversample mode for rx lane. Asynchro */
/*                                        nous signal to the PMD. 0 -OSR1. 1 - OSR2.Reset value i */
/*                                        s 0x0.                                                  */
/* cfgactiveethernet2p5: cfgActiveEthernet2p5 - 0: Selects divide-by-2 clock divider for clkRbc12 */
/*                       5. 1: Selectsdivide-by-1 clock divider for clkRbc125.Reset value is 0x0. */
/* cfgngpontxclk: cfgNgponTxClk - 0: Selects divide-by-2 clock divider for mac_tx_clk. 1: Selects */
/*                divide-by-4 clock divider for mac_tx_clk. 2: Selects divide-by-1clock divider.  */
/*                3: Unused.Reset value is 0x0.                                                   */
/* cfgngponrxclk: cfgNgponRxClk - 0: Selects divide-by-2 clock divider for mac_rx_clk. 1: Selects */
/*                divide-by-4 clock divider for mac_rx_clk. 2: Selects divide-by-1clock divider.  */
/*                3: Unused.Reset value is 0x0.                                                   */
/* cfg_apm_mux_sel_0: CFG_APM_MUX_SEL_0 - 0: Select ncoProgClk for MUX 0 output. 1: Select ncoClk */
/*                    8KHz for MUX0 output.Reset value is 0x0.                                    */
/* cfg_apm_mux_sel_1: CFG_APM_MUX_SEL_1 - 0: Select MUX 0 output for wan_rbc_for_apm. 1: ntr_sync */
/*                    _pulse forwan_rbc_for_apm.Reset value is 0x0.                               */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_rx_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ln_dp_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ln_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_core_0_dp_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_core_1_dp_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_por_h_rstb;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ext_los;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ln_tx_h_pwrdn;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_ln_rx_h_pwrdn;
    bdmf_boolean cr_xgwan_top_wan_misc_pmd_tx_disable;
    uint8_t cr_xgwan_top_wan_misc_pmd_tx_osr_mode;
    uint8_t cr_xgwan_top_wan_misc_pmd_tx_mode;
    uint8_t cr_xgwan_top_wan_misc_pmd_rx_osr_mode;
    bdmf_boolean cfgactiveethernet2p5;
    uint8_t cfgngpontxclk;
    uint8_t cfgngponrxclk;
    bdmf_boolean cfg_apm_mux_sel_0;
    bdmf_boolean cfg_apm_mux_sel_1;
} misc_misc_2;


/**************************************************************************************************/
/* cr_xgwan_top_wan_misc_wan_cfg_mem_reb: MEM_REB - REB going to WAN memories0 - REB asserted1 -  */
/*                                        REB de-asserted                                         */
/* cr_xgwan_top_wan_misc_wan_cfg_laser_invert: laser_invert - Selects laser mode0 - Do not invert */
/*                                              laser_en1 - Invert laser_en                       */
/* cr_xgwan_top_wan_misc_wan_cfg_laser_mode: laser_mode - Selects laser mode0 - Use 1G EPON laser */
/*                                           _en1 - Use 10G EPON laser_en2 - Use GPON laser_en3 - */
/*                                            User NGPON/XGPON laser_en                           */
/* cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select_hs: wan_interface_select_hs - Select WAN mo */
/*                                                        de.  Used together with bit [5], wan_in */
/*                                                        terface_select.[6] [5]0    0 : EPON0    */
/*                                                         1 : 10G EPON1    0 : GPON1    1 : NGPO */
/*                                                        N/XGPON                                 */
/* cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select: wan_interface_select - Selects which WAN i */
/*                                                     nterface will be used.0 - EPON/10GEPON1 -  */
/*                                                     GPON/NGPON/XGPON                           */
/* cr_xgwan_top_wan_misc_wan_cfg_laser_oe: laser_oe - Laser output enable0 - Output not enabled1  */
/*                                         - Output enabled                                       */
/* cr_xgwan_top_wan_misc_wan_cfg_ntr_sync_period_sel: ntr_sync_period_sel - Selects APM clock gen */
/*                                                    eration division                            */
/* cr_xgwan_top_wan_misc_wan_cfg_wan_debug_sel: WAN_DEBUG_SEL - Selects WAN debug bus output      */
/* cr_xgwan_top_wan_misc_epon_debug_sel: EPON_DEBUG_SEL - Selects EPON debug bus output           */
/* cr_xgwan_top_wan_misc_epon_tx_fifo_off: EPON_TX_FIFO_OFF - TXFIFO OFF LOAD signal for EPONs ge */
/*                                         arbox.Reset value is 0x0.                              */
/* cr_xgwan_top_wan_misc_epon_power_zone_sel: EPON_POWER_ZONE_SEL - EPON power zone select.       */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cr_xgwan_top_wan_misc_wan_cfg_mem_reb;
    bdmf_boolean cr_xgwan_top_wan_misc_wan_cfg_laser_invert;
    uint8_t cr_xgwan_top_wan_misc_wan_cfg_laser_mode;
    bdmf_boolean cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select_hs;
    bdmf_boolean cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select;
    bdmf_boolean cr_xgwan_top_wan_misc_wan_cfg_laser_oe;
    uint8_t cr_xgwan_top_wan_misc_wan_cfg_ntr_sync_period_sel;
    uint8_t cr_xgwan_top_wan_misc_wan_cfg_wan_debug_sel;
    uint8_t cr_xgwan_top_wan_misc_epon_debug_sel;
    uint8_t cr_xgwan_top_wan_misc_epon_tx_fifo_off;
    bdmf_boolean cr_xgwan_top_wan_misc_epon_power_zone_sel;
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

