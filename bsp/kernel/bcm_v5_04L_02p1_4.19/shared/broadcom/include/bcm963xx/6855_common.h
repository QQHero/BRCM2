/*
<:copyright-BRCM:2020:proprietary:standard 

   Copyright (c) 2020 Broadcom 
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
/*   MODULE:  6855_common.h                                           */
/*   PURPOSE: Register definition used by assembly for BCM6855        */
/*                                                                     */
/***********************************************************************/

/* BOOTROM inclusions */
#ifndef __BCM6855_MAP_COMMON_H
#define __BCM6855_MAP_COMMON_H

#if defined (_BOOTROM_)
#include "bcm_sbi_header.h"

#else

#define CFG_CHIP_SRAM				0x82000000	
#define CFG_CHIP_SRAM_SIZE			(1024*256)
#endif

#define SPIFLASH_PHYS_BASE          0xffd00000  
#define SPIFLASH_SIZE               0x100000    
#define NANDFLASH_PHYS_BASE         0xffe00000  
#define NANDFLASH_SIZE              0x100000    

/*
#####################################################################
# Memory Control Registers
#####################################################################
*/

#define MEMC_GLB_VERS                              0x00000000 /* MC Global Version Register */
#define MEMC_GLB_GCFG                              0x00000004 /* MC Global Configuration Register */
#define MEMC_GLB_GCFG_GCFG_DRAM_EN                 (1<<31)
#define MEMC_GLB_GCFG_MEM_INIT_DONE                (1<<8)

#define MEMC_GLB_FSBL_STATE            0x10      /* Firmware state scratchpad */
#define MEMC_GLB_FSBL_DRAM_SIZE_SHIFT          0
#define MEMC_GLB_FSBL_DRAM_SIZE_MASK           (0xf << MEMC_GLB_FSBL_DRAM_SIZE_SHIFT)

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

/* row 8 */
#define OTP_CPU_CORE_CFG_ROW			8
#define OTP_CPU_CORE_CFG_SHIFT			28
#define OTP_CPU_CORE_CFG_MASK			(0x7 << OTP_CPU_CORE_CFG_SHIFT)

/* row 17 */
#define OTP_BRCM_BTRM_BOOT_ENABLE_ROW           17
#define OTP_BRCM_BTRM_BOOT_ENABLE_SHIFT         3
#define OTP_BRCM_BTRM_BOOT_ENABLE_MASK          (1 << OTP_BRCM_BTRM_BOOT_ENABLE_SHIFT)

/* row 18 */
#define OTP_CUST_BTRM_BOOT_ENABLE_ROW           18
#define OTP_CUST_BTRM_BOOT_ENABLE_SHIFT         15
#define OTP_CUST_BTRM_BOOT_ENABLE_MASK          (7 << OTP_CUST_BTRM_BOOT_ENABLE_SHIFT)

/* row 19 */
#define OTP_CUST_BTRM_UART_DISABLE_ROW          19
#define OTP_CUST_BTRM_UART_DISABLE_SHIFT        29
#define OTP_CUST_BTRM_UART_DISABLE_MASK         (1 << OTP_CUST_BTRM_UART_DISABLE_SHIFT)

/* row 23 */
#define OTP_CUST_MFG_MRKTID_ROW                 23
#define OTP_CUST_MFG_MRKTID_SHIFT               0
#define OTP_CUST_MFG_MRKTID_MASK                (0xffff << OTP_CUST_MFG_MRKTID_SHIFT)

/* row 34 */
#define OTP_BRCM_ENFORCE_BINIT_ROW 		17
#define OTP_BRCM_ENFORCE_BINIT_SHIFT		13
#define OTP_BRCM_ENFORCE_BINIT_MASK		(1 << OTP_BRCM_ENFORCE_BINIT_SHIFT)

/* row 26 - 32 hmid public hash*/
#define OTP_BOOT_SW_0                 		26
#define OTP_BOOT_SW_0_SHIFT                     0
#define OTP_BOOT_SW_0_MASK                	(0xffffffff << OTP_BOOT_SW_0_SHIFT)


#define OTP_READ_TIMEOUT_CNTR                   0x100000

/*
#####################################################################
# GIC reigsters
#####################################################################
*/
#define GICD_CTLR_OFFSET        0x0
#define	GICD_TYPER_OFFSET       0x4
#define GICD_IGROUPR0_OFFSET    0x80
#define GICD_IGROUPR8_OFFSET    0xA0
#define GICD_PRIORITY_OFFSET    0x400
#define GICC_CTLR_OFFSET        0x0
#define GICC_PMR_OFFSET         0x4
#define GICC_BPR_OFFSET         0x8

/*
#####################################################################
# GPIO Control Registers
#####################################################################
*/

#define GPIO_DATA                  (GPIO_BASE + 0x20)
#define GP_OFFSET                  0x68

/***************************************************************************
 *MEMC_SRAM_REMAP - "MC non-secure address remapping to internal SRAM"
***************************************************************************/

#define MEMC_SRAM_REMAP_CONTROL                    0x00000020 /* SRAM Remap Control */
#define MEMC_SRAM_REMAP_INIT                       0x00000028 /* SRAM Remap Initialization */
#define MEMC_SRAM_REMAP_LOG_INFO_0                 0x0000002c /* SRAM Remap Log Info 0 */
#define MEMC_SRAM_REMAP_LOG_INFO_1                 0x00000030 /* SRAM Remap Log Info 1 */

/* SOTP*/
#define SOTP_PAC_CTRL			0x0
#define	PSRAM_BLKF_PHYS_BASE        0x82d9a800
#define	PSRAM_BLKF_PHYS_BASE_SIZE   0x3c0
#define	PSRAM_BLOCK_FUNC            PSRAM_BLKF_PHYS_BASE
#define PSRAM_BLKF_CFG_CTRL		0x0 
#define PSRAM_BLKF_CFG_SCRM_SEED	0x4
#define PSRAM_BLKF_CFG_SCRM_ADDR	0x8
#define RNG_CTRL_0			0x0
#define RNG_SOFT_RESET			0x4
#define RNG_TOTAL_BIT_COUNT		0xC
#define RNG_TOTAL_BIT_COUNT_TSH		0x10
#define RNG_FIFO_DATA			0x20
#define RNG_FIFO_COUNT			0x24

#endif
