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

#ifndef _GPON_AG_H_
#define _GPON_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset: SW_RESET_TXPG_RESET - Tx Pattern Gener */
/*                                                         ator reset control                     */
/* cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset: SW_RESET_TXFIFO_RESET - Tx FIFO rese */
/*                                                           t control                            */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted: FIFO_CFG_0_CLEAR_TXFIFO_DR */
/*                                                                 %s%-IFTED - If 1, the TXFIFO_D */
/*                                                                 %s%-RIFTED status bit resets t */
/*                                                                 %s%-32so 0.Reset value is 0.   */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx: FIFO_CFG_0_LOOPBACK_RX - If 1, the  */
/*                                                            output of Rx FIFO is looped back to */
/*                                                             the input of Tx FIFO. In this case */
/*                                                            , the SATA PHY Tx data rate is the  */
/*                                                            same as the Rx data rate regardless */
/*                                                             of whether Gen2 or Gen3 is selecte */
/*                                                            d.Reset value is 0.                 */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision: FIFO_CFG_0_CLEAR_TXFIFO_ */
/*                                                                 %s%-41COLLISION - If 1, the TX */
/*                                                                 %s%-41FIFO_COLLISION status bi */
/*                                                                 %s%-41t resets to 0.Reset valu */
/*                                                                 %s%-30se is 0.                 */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv: FIFO_CFG_0_TX_WR_PTR_ADV - Advanc */
/*                                                              e Tx FIFO write pointer by 1 loca */
/*                                                              tion (8 Tx bits). The pointer is  */
/*                                                              adjusted on every 0 to 1 transiti */
/*                                                              on in this register field.Reset v */
/*                                                              alue is 0.                        */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly: FIFO_CFG_0_TX_WR_PTR_DLY - Delay  */
/*                                                              Tx FIFO write pointer by 1 locati */
/*                                                              on (8 Tx bits). The pointer is ad */
/*                                                              justed on every 0 to 1 transition */
/*                                                               in this register field.Reset val */
/*                                                              ue is 0.                          */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv: FIFO_CFG_0_TX_BIT_INV - This bit ena */
/*                                                           bles logically inversion of every Tx */
/*                                                            bit.Reset value is 0.               */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max: FIFO_CFG_0_TX_PTR_DIST_MAX - Ma */
/*                                                                ximum distance allowed between  */
/*                                                                the Tx FIFO write and read poin */
/*                                                                ters. The TXFIFO_DRIFTED status */
/*                                                                 bit is asserted if TX_POINTER_ */
/*                                                                DISTANCE goes above this maximu */
/*                                                                m value.Reset value is 3.       */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback: FIFO_CFG_0_ASYM_LOOPBACK - GPON a */
/*                                                              symetric loopback                 */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min: FIFO_CFG_0_TX_PTR_DIST_MIN - Mi */
/*                                                                nimum distance allowed between  */
/*                                                                the Tx FIFO write and read poin */
/*                                                                ters. The TXFIFO_DRIFTED status */
/*                                                                 bit is asserted if TX_POINTER_ */
/*                                                                DISTANCE goes below this minimu */
/*                                                                m value.Reset value is 1.       */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order: FIFO_CFG_0_TX_20BIT_ORDER - This */
/*                                                                field changes the bit order of  */
/*                                                               the 20-bit Tx data exiting the T */
/*                                                               x FIFO to SATA PHY.0: Bit  0 is  */
/*                                                               transmitted first1: Bit 19 is tr */
/*                                                               ansmitted firstReset value is 1. */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order: FIFO_CFG_0_TX_8BIT_ORDER - This f */
/*                                                              ield changes the bit order of the */
/*                                                               8-bit Tx data entering the Tx FI */
/*                                                              FO.0: No changes1: Tx data is rev */
/*                                                              ersed from [7:0] to [0:7]Reset va */
/*                                                              lue is 0.                         */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order: FIFO_CFG_0_RX_16BIT_ORDER - This */
/*                                                                field changes the bit order of  */
/*                                                               the 16-bit Rx data exiting the R */
/*                                                               x FIFO to GPON MAC.0: No changes */
/*                                                               1: Rx data is reversed from [15: */
/*                                                               0] to [0:15]Reset value is 0.    */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order: FIFO_CFG_0_TXLBE_BIT_ORDER - TB */
/*                                                                D                               */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel: FIFO_STATUS_SEL - TBD                      */
/* cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel: PTG_STATUS1_SEL - TBD                      */
/* cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel: PTG_STATUS2_SEL - TBD                      */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txpg_reset;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_sw_reset_txfifo_reset;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_drifted;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_loopback_rx;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_clear_txfifo_collision;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_adv;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_wr_ptr_dly;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_bit_inv;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_max;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_asym_loopback;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_ptr_dist_min;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_20bit_order;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_txlbe_bit_order;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_fifo_status_sel;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status1_sel;
    bdmf_boolean cr_xgwan_top_wan_misc_gpon_gearbox_ptg_status2_sel;
} gpon_gpon_gearbox_0;


/**************************************************************************************************/
/* pg_mode: PG_MODE - Pattern generator modes:0: Pattern generator disabled. GPON MAC has control */
/*           of Tx output and burst enable.1: Generate repetitive Tx bursts. Each burst consists  */
/*          of 1 header byte and1 or more payload bytes. Filler bytes are placed between Tx burst */
/*          s.2: Reserved3: Reserved4: Generate PRBS7 pattern5: Generate PRBS15 pattern6: Generat */
/*          e PRBS23 pattern7: Generate PRBS31 patternMode 0 is for GPON normal operation.  Mode  */
/*          1 is for laser burst enable calibration.Reset value is 0.                             */
/* header: HEADER - 8-bit pattern to placed at the start of every Tx burst when PG_MODE is 1.Rese */
/*         t value is 0.                                                                          */
/* payload: PAYLOAD - 8-bit pattern to placed after the HEADER byte in every Tx burst when PG_MOD */
/*          E is 1.Reset value is 0.                                                              */
/* filler: FILLER - 8-bit pattern to placed between Tx bursts when PG_MODE is 1.Reset value is 0. */
/**************************************************************************************************/
typedef struct
{
    uint8_t pg_mode;
    uint8_t header;
    uint8_t payload;
    uint8_t filler;
} gpon_gpon_gearbox_pattern_cfg1;


/**************************************************************************************************/
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer: FIFO_CFG_1_TX_RD_POINTER - Initia */
/*                                                              l value to be loaded into Tx FIFO */
/*                                                               read pointer whenTXFIFO_REET is  */
/*                                                              asserted. Legal values are 0 to 3 */
/*                                                              1.Reset value is 0x1c.            */
/* cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer: FIFO_CFG_1_TX_WR_POINTER - Initia */
/*                                                              l value to be loaded into Tx FIFO */
/*                                                               write pointer whenTXFIFO_RESET i */
/*                                                              s asserted. Legal values are 0 to */
/*                                                               19.Reset value is 0x0.           */
/* cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc: TX_VLD_DELAY_CYC - Offset of tx valid sig */
/*                                                      nal towards SERDES                        */
/* cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc: CONFIG_BURST_DELAY_CYC - TBD        */
/**************************************************************************************************/
typedef struct
{
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_rd_pointer;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_1_tx_wr_pointer;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_tx_vld_delay_cyc;
    uint8_t cr_xgwan_top_wan_misc_gpon_gearbox_config_burst_delay_cyc;
} gpon_gpon_gearbox_2;

bdmf_error_t ag_drv_gpon_gpon_gearbox_0_set(const gpon_gpon_gearbox_0 *gpon_gearbox_0);
bdmf_error_t ag_drv_gpon_gpon_gearbox_0_get(gpon_gpon_gearbox_0 *gpon_gearbox_0);
bdmf_error_t ag_drv_gpon_gpon_gearbox_pattern_cfg1_set(const gpon_gpon_gearbox_pattern_cfg1 *gpon_gearbox_pattern_cfg1);
bdmf_error_t ag_drv_gpon_gpon_gearbox_pattern_cfg1_get(gpon_gpon_gearbox_pattern_cfg1 *gpon_gearbox_pattern_cfg1);
bdmf_error_t ag_drv_gpon_gpon_gearbox_pattern_cfg2_set(uint8_t gap_size, uint8_t burst_size);
bdmf_error_t ag_drv_gpon_gpon_gearbox_pattern_cfg2_get(uint8_t *gap_size, uint8_t *burst_size);
bdmf_error_t ag_drv_gpon_gpon_gearbox_2_set(const gpon_gpon_gearbox_2 *gpon_gearbox_2);
bdmf_error_t ag_drv_gpon_gpon_gearbox_2_get(gpon_gpon_gearbox_2 *gpon_gearbox_2);
bdmf_error_t ag_drv_gpon_gpon_gearbox_status_get(uint32_t *cr_rd_data_clx);

#ifdef USE_BDMF_SHELL
enum
{
    cli_gpon_gpon_gearbox_0,
    cli_gpon_gpon_gearbox_pattern_cfg1,
    cli_gpon_gpon_gearbox_pattern_cfg2,
    cli_gpon_gpon_gearbox_2,
    cli_gpon_gpon_gearbox_status,
};

int bcm_gpon_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_gpon_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

