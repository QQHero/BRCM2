/*
<:copyright-BRCM:2012:proprietary:standard 

   Copyright (c) 2012 Broadcom 
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>

*/
/***********************************************************************/
/*                                                                     */
/*   MODULE:  6813_common.h                                           */
/*   DATE:    12/09/19                                                 */
/*   PURPOSE: Register definition used by assembly for BCM6813        */
/*                                                                     */
/***********************************************************************/

/* BOOTROM inclusions */
#ifndef __BCM6813_COMMON_H
#define __BCM6813_COMMON_H

#if defined (_BOOTROM_)
#include "bcm_sbi_header.h"

#else

#define CFG_CHIP_SRAM				0x82000000	
#define CFG_CHIP_SRAM_SIZE			(1024*128)
#endif

#define ETH_PHY_TOP_BASE		0x83400000
#define ETH_PHY_TOP_REG_BASE		0x83700000
/*
#####################################################################
# Memory Control Registers
#####################################################################
*/

#define MEMC_GLB_VERS                           0x00000000 /* MC Global Version Register */
#define MEMC_GLB_GCFG                           0x00000004 /* MC Global Configuration Register */
#define MEMC_GLB_GCFG_GCFG_DRAM_EN              (1<<31)
#define MEMC_GLB_GCFG_MEM_INIT_DONE             (1<<8)

#define MEMC_GLB_FSBL_STATE            		0x10      /* Firmware state scratchpad */
#define MEMC_GLB_FSBL_DRAM_SIZE_SHIFT		0
#define MEMC_GLB_FSBL_DRAM_SIZE_MASK		(0xf << MEMC_GLB_FSBL_DRAM_SIZE_SHIFT)

/* uart init usage */
#define UART0DR          			0x0
#define UART0RSR         			0x4
#define UART0FR          			0x18
#define UART0ILPR        			0x20
#define UART0IBRD        			0x24
#define UART0FBRD        			0x28
#define UART0LCR_H       			0x2c
#define UART0CR          			0x30
#define UART0IFLS        			0x34
#define UART0IMSC        			0x38
#define UART0IRIS        			0x3c
#define UART0IMIS        			0x40
#define UART0ICR         			0x44
#define UART0DMACR       			0x48

#define LCR_FIFOEN				0x10
#define LCR_PAREN				0x02
#define LCR_8SYM_NOPAR_ONE_STOP_FIFOEN_CFG	0x70 /*(BIT8SYM|FIFOEN|NOPAR|ONESTOP)*/
#define LCR_8SYM_NOPAR_ONE_STOP_NOFIFO_CFG	0x60 /*(BIT8SYM|FIFODIS|NOPAR|ONESTOP)*/
#define CR_TXE					0x100
#define CR_RXE					0x200
#define CR_EN					0x1
#define FR_TXFE					0x80
#define FR_RXFF					0x40
#define FR_TXFF					0x20
#define FR_RXFE					0x10
#define FR_BUSY					0x04

/*
#####################################################################
# OTP Control / Status Registers
#####################################################################
*/
#define JTAG_OTP_GENERAL_CTRL_0                 0x00
#define JTAG_OTP_GENERAL_CTRL_0_START           (1 << 0)
#define JTAG_OTP_GENERAL_CTRL_0_PROG_EN         (1 << 21)
#define JTAG_OTP_GENERAL_CTRL_0_ACCESS_MODE     (2 << 22)

#define JTAG_OTP_GENERAL_CTRL_1                 0x04
#define JTAG_OTP_GENERAL_CTRL_1_CPU_MODE        (1 << 0)

#define JTAG_OTP_GENERAL_CTRL_2                 0x08

#define JTAG_OTP_GENERAL_CTRL_3                 0x10

#define JTAG_OTP_GENERAL_STATUS_0               0x18

#define JTAG_OTP_GENERAL_STATUS_1               0x20
#define JTAG_OTP_GENERAL_STATUS_1_CMD_DONE      (1 << 1)

#if defined(_BOOTROM_) 
/* row 9 */
#define OTP_CPU_CLOCK_FREQ_ROW			9
#define OTP_CPU_CLOCK_FREQ_SHIFT		0
#define OTP_CPU_CLOCK_FREQ_MASK			(0x7 << OTP_CPU_CLOCK_FREQ_SHIFT)

/* row 8 */
#define OTP_CPU_CORE_CFG_ROW			8
#define OTP_CPU_CORE_CFG_SHIFT			28
#define OTP_CPU_CORE_CFG_MASK			(0x1 << OTP_CPU_CORE_CFG_SHIFT) // 0=dual cores, 1=single core

/* row 13 */
#define OTP_BRCM_BTRM_PRODUCTION_MODE_ROW       13
#define OTP_BRCM_BTRM_PRODUCTION_MODE_SHIFT    	0
#define OTP_BRCM_BTRM_PRODUCTION_MODE_MASK      (1 << OTP_BRCM_BTRM_PRODUCTION_MODE_SHIFT)
/* row 13 */
#define OTP_BRCM_BTRM_BOOT_ENABLE_ROW           13
#define OTP_BRCM_BTRM_BOOT_ENABLE_SHIFT        	2
#define OTP_BRCM_BTRM_BOOT_ENABLE_MASK          (1 << OTP_BRCM_BTRM_BOOT_ENABLE_SHIFT)

/* row 14 */
#define OTP_CUST_BTRM_BOOT_ENABLE_ROW           14
#define OTP_CUST_BTRM_BOOT_ENABLE_SHIFT        	28
#define OTP_CUST_BTRM_BOOT_ENABLE_MASK          (1 << OTP_CUST_BTRM_BOOT_ENABLE_SHIFT)

/* row 14 */
#define OTP_CUST_BTRM_UART_DISABLE_ROW         	14
#define OTP_CUST_BTRM_UART_DISABLE_SHIFT       	0
#define OTP_CUST_BTRM_UART_DISABLE_MASK         (1 << OTP_CUST_BTRM_UART_DISABLE_SHIFT)

/* row 14 */
#define OTP_CUST_BTRM_MSG_DISABLE_ROW         	14
#define OTP_CUST_BTRM_MSG_DISABLE_SHIFT       	27
#define OTP_CUST_BTRM_MSG_DISABLE_MASK          (1 << OTP_CUST_BTRM_MSG_DISABLE_SHIFT)
/* row 29 */
#define OTP_CUST_MFG_MRKTID_ROW                 29
#define OTP_CUST_MFG_MRKTID_SHIFT               0
#define OTP_CUST_MFG_MRKTID_MASK                (0xffff << OTP_CUST_MFG_MRKTID_SHIFT)
#endif


#define OTP_READ_TIMEOUT_CNTR                   0x100000

/*
#####################################################################
# GIC reigsters
#####################################################################
*/
#define GICD_CTLR				0x0
#define	GICD_TYPER				0x4
#define GICD_IGROUPR0				0x80
#define GICD_IGROUPR8				0xA0
#define GICD_PRIORITY				0x400
#define GICC_CTLR				0x0
#define GICC_PMR				0x4
#define GICC_BPR				0x8

/*
#####################################################################
# GPIO Control Registers
#####################################################################
*/

#define GPIO_DATA				(GPIO_BASE + 0x20)
#define GP_OFFSET				0x68

/* SOTP*/



#define	PSRAM_BLKF_PHYS_BASE			0x828a2800
#define	PSRAM_BLKF_PHYS_BASE_SIZE		0x2c0
#define	PSRAM_BLOCK_FUNC			PSRAM_BLKF_PHYS_BASE
#define PSRAM_BLKF_CFG_CTRL			0x0 
#define PSRAM_BLKF_CFG_SCRM_SEED		0x4
#define PSRAM_BLKF_CFG_SCRM_ADDR		0x8
#define RNG_CTRL_0				0x0
#define RNG_SOFT_RESET				0x4
#define RNG_TOTAL_BIT_COUNT			0xC
#define RNG_TOTAL_BIT_COUNT_TSH			0x10
#define RNG_FIFO_DATA				0x20
#define RNG_FIFO_COUNT				0x24

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
