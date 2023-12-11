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

#ifndef _NGPON_GEARBOX_AG_H_
#define _NGPON_GEARBOX_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* cfngpongboxrxfifordptr:  - Value for RX output RIFO read pointer.                              */
/* cfngpongboxrxptrautolddis:  - Disable pointer auto-load going into lock for output FIFO.       */
/* cfngpongboxrxmaxbadk:  - Number of bad KChar to go out of lock.                                */
/* cfngpongboxrxfrmk28only:  - Use only K28.5 for framing.                                        */
/* cfngpongboxrxmaxgoodk:  - Number of good KChar in a row for lock.                              */
/* cfngpongboxrxfrchunt:  - Force 10b framer to go to HUNT state on rising edge.                  */
/* cfngpongboxrxoutdataflip:  - Bitwise flip 32b output data.                                     */
/* cfngpongboxrxfrcmuxsel:  - Force mux select to value in cfNGponGboxRxFrcMuxVal.                */
/* cfngpongboxrxfrcmuxval:  - Value that will be forced to mux select when cfNGponGboxRxFrcMuxSel */
/*                         is asserted.                                                           */
/* cfngpongboxrx20bdataflip:  - Bitwise flip RX 20b gearbox data.                                 */
/* cfngpongboxrxserdataflip:  - Bitwise flip RX 16b data from SERDES.                             */
/* cfngpongboxrxserdatainv:  - Bitwise invert RX 16b data from SERDES.                            */
/* cfngpongboxrxfifoptrld:  - Load value for FIFO read pointer.  Write pointer will be loaded to0 */
/*                         .                                                                      */
/* cfngpongboxrxswsynchold:  - When set, synchronization will be held indefinitely.               */
/* cfngpongboxrxmode:  - 0. 8B/10B decoder mode operating at 777 MHz.  1. Pass through modeoperat */
/*                    ing at 622 MHz.                                                             */
/* cfngpongboxrxen:  - Synchronous enable for RX gearbox.                                         */
/* cfngpongboxrstn:  - Asynchronous, active-low, software reset for gearbox.                      */
/**************************************************************************************************/
typedef struct
{
    uint8_t cfngpongboxrxfifordptr;
    bdmf_boolean cfngpongboxrxptrautolddis;
    uint8_t cfngpongboxrxmaxbadk;
    bdmf_boolean cfngpongboxrxfrmk28only;
    uint8_t cfngpongboxrxmaxgoodk;
    bdmf_boolean cfngpongboxrxfrchunt;
    bdmf_boolean cfngpongboxrxoutdataflip;
    bdmf_boolean cfngpongboxrxfrcmuxsel;
    uint8_t cfngpongboxrxfrcmuxval;
    bdmf_boolean cfngpongboxrx20bdataflip;
    bdmf_boolean cfngpongboxrxserdataflip;
    bdmf_boolean cfngpongboxrxserdatainv;
    bdmf_boolean cfngpongboxrxfifoptrld;
    bdmf_boolean cfngpongboxrxswsynchold;
    bdmf_boolean cfngpongboxrxmode;
    bdmf_boolean cfngpongboxrxen;
    bdmf_boolean cfngpongboxrstn;
} ngpon_gearbox_rx_ctl_0;


/**************************************************************************************************/
/* cfngpongboxtxfifodatardptr:  - Value for TX data FIFO read pointer.  Steps of 2 x txClk (622 M */
/*                             Hz),jumps of 32 bits.                                              */
/* cfngpongboxtxfifovldoff:  - Value for TX valid FIFO offset.  Steps of txClk (622 MHz), jumps o */
/*                          f16 bits.  1 to 15 are advances valid vs data, valid comes out ahead. */
/*                          1=1 clock, 2=2 clocks, 3=3 clocks...  31 to 16 are advances validvs d */
/*                          ata, valid comes out behind.  31=1 clock, 30=2 clocks, 29=3clocks...  */
/* cfngpongboxtxservldflip:  - Flip TX data valid endian on 32b input.                            */
/* cfngpongboxtxserdataflip:  - Flip TX data endian on 32b input.                                 */
/* cfngpongboxtxservldinv:  - Bitwise invert TX 4b valid to SERDES.                               */
/* cfngpongboxtxserdatainv:  - Bitwise invert TX 16b data to SERDES.                              */
/* cfngpongboxtxfifovldptrld:  - Load only the offset for TX valid FIFO pointer.  This is an offs */
/*                            etfrom the data read pointer.                                       */
/* cfngpongboxtxfifoptrld:  - Load value for TX data FIFO read pointer and valid read pointeroffs */
/*                         et.  Data/valid write will be loaded to 0.                             */
/* cfngpongboxtxen:  - Synchronous enable for TX gearbox.                                         */
/**************************************************************************************************/
typedef struct
{
    uint8_t cfngpongboxtxfifodatardptr;
    uint8_t cfngpongboxtxfifovldoff;
    bdmf_boolean cfngpongboxtxservldflip;
    bdmf_boolean cfngpongboxtxserdataflip;
    bdmf_boolean cfngpongboxtxservldinv;
    bdmf_boolean cfngpongboxtxserdatainv;
    bdmf_boolean cfngpongboxtxfifovldptrld;
    bdmf_boolean cfngpongboxtxfifoptrld;
    bdmf_boolean cfngpongboxtxen;
} ngpon_gearbox_tx_ctl;


/**************************************************************************************************/
/* ngpontxgboxfifovldptrcol:  - Pointer collision.                                                */
/* ngponrxgboxstate:  - Framer state.                                                             */
/* ngpontxgboxfifodataptrcol:  - Pointer collision.                                               */
/* ngponrxgboxkcnt:  - Number of KChar.                                                           */
/* ngponrxgboxfifoptrdelta:  - Pointer delta.                                                     */
/* ngponrxgboxsyncacq:  - 10b sync acquired.                                                      */
/* ngponrxgboxfifoptrcol:  - FIFO pointer collision.                                              */
/* ngponrxgboxcodeerrcntstat:  - Line errors.                                                     */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean ngpontxgboxfifovldptrcol;
    uint8_t ngponrxgboxstate;
    bdmf_boolean ngpontxgboxfifodataptrcol;
    uint8_t ngponrxgboxkcnt;
    uint8_t ngponrxgboxfifoptrdelta;
    bdmf_boolean ngponrxgboxsyncacq;
    bdmf_boolean ngponrxgboxfifoptrcol;
    uint16_t ngponrxgboxcodeerrcntstat;
} ngpon_gearbox_status;

bdmf_error_t ag_drv_ngpon_gearbox_rx_ctl_0_set(const ngpon_gearbox_rx_ctl_0 *rx_ctl_0);
bdmf_error_t ag_drv_ngpon_gearbox_rx_ctl_0_get(ngpon_gearbox_rx_ctl_0 *rx_ctl_0);
bdmf_error_t ag_drv_ngpon_gearbox_rx_ctl_1_set(uint32_t cfngpongboxrxmaxtimercnt);
bdmf_error_t ag_drv_ngpon_gearbox_rx_ctl_1_get(uint32_t *cfngpongboxrxmaxtimercnt);
bdmf_error_t ag_drv_ngpon_gearbox_rx_ctl_2_set(uint16_t cfngpongboxrxk28d5rdp, uint16_t cfngpongboxrxk28d5rdn);
bdmf_error_t ag_drv_ngpon_gearbox_rx_ctl_2_get(uint16_t *cfngpongboxrxk28d5rdp, uint16_t *cfngpongboxrxk28d5rdn);
bdmf_error_t ag_drv_ngpon_gearbox_rx_ctl_3_set(uint16_t cfngpongboxrxd5d7rdp, uint16_t cfngpongboxrxd5d7rdn);
bdmf_error_t ag_drv_ngpon_gearbox_rx_ctl_3_get(uint16_t *cfngpongboxrxd5d7rdp, uint16_t *cfngpongboxrxd5d7rdn);
bdmf_error_t ag_drv_ngpon_gearbox_tx_ctl_set(const ngpon_gearbox_tx_ctl *tx_ctl);
bdmf_error_t ag_drv_ngpon_gearbox_tx_ctl_get(ngpon_gearbox_tx_ctl *tx_ctl);
bdmf_error_t ag_drv_ngpon_gearbox_status_set(const ngpon_gearbox_status *status);
bdmf_error_t ag_drv_ngpon_gearbox_status_get(ngpon_gearbox_status *status);

#ifdef USE_BDMF_SHELL
enum
{
    cli_ngpon_gearbox_rx_ctl_0,
    cli_ngpon_gearbox_rx_ctl_1,
    cli_ngpon_gearbox_rx_ctl_2,
    cli_ngpon_gearbox_rx_ctl_3,
    cli_ngpon_gearbox_tx_ctl,
    cli_ngpon_gearbox_status,
};

int bcm_ngpon_gearbox_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_ngpon_gearbox_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

