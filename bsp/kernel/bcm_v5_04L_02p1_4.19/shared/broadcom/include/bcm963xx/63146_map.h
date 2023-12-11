/*
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom 
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
/*   MODULE:  63146_map.h                                               */
/*   DATE:    10/07/19                                                 */
/*   PURPOSE: Define the proprietary hardware blocks/subblocks for     */
/*            BCM63146                                                  */
/*                                                                     */
/***********************************************************************/
#ifndef __BCM63146_MAP_H
#define __BCM63146_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bcmtypes.h"
#include "63146_common.h"
#include "63146_map_part.h"
/* For the proprietary blocks that needs be mapped in the linux, the base 
   address still define in the map_part.h. The detail block register definition
   must be defined in this file with the C structure below */
#define BROM_BASE                   (PERF_BASE + 0x00000600)  /* bootrom registers */
#define BROM_GEN_BASE               BROM_BASE
#define BROM_SEC_BASE               (PERF_BASE + 0x00000620)  /* bootrom secure registers */
#define BROM_SEC1_BASE              BROM_SEC_BASE

/* For the proprietary blocks does not need be mapped in linux at all */
#define BOOTROM_BASE                0xfff00000
#define BOOTROM_SIZE                0x20000
#define PERF_SRAM_BASE              0xfff80000
#define PERF_SRAM_SIZE              0x8000
#define RDP_SRAM_BASE               0x82000000
#define RDP_SRAM_SIZE               0x30000     /* 192KB */

#define BOOTLUT_SIZE                0x1000
#if defined (_BOOTROM_)
#define SOTP_OFFSET                 0x5000
#define FSR_OFFSET                  0x5300
#define SOTP_BASE                   (PERF_PHYS_BASE+SOTP_OFFSET)
#define FSR_BASE                    (PERF_PHYS_BASE+FSR_OFFSET)
#endif
/*
#####################################################################
# BIU config Registers
#####################################################################
*/
#define TS0_CTRL_CNTCR             0x1000

#ifndef __ASSEMBLER__

#if defined(__KERNEL__) && !defined(MODULE)
#error "PRIVATE FILE INCLUDED IN KERNEL"
#endif


typedef struct BootBase {
    uint32 general_secbootcfg;
#define BOOTROM_CRC_DONE                (1 << 31)
#define BOOTROM_CRC_FAIL                (1 << 30)
    uint32 general_boot_crc_low;
    uint32 general_boot_crc_high;
} BootBase;

#define BOOTBASE ((volatile BootBase * const) BROM_BASE)

typedef struct BootSec {
    uint32 AccessCtrl;
    uint32 AccessRangeChk[4];
} BootSec;

#define BOOTSECURE ((volatile BootSec * const) BROM_SEC_BASE)

/* SOTP defs */
#define KP_PORTAL_STATUS_KEY_LOADED_SHIFT	24
#define KP_PORTAL_STATUS_KEY_LOADED_MASK	1
#define KP_PORTAL_STATUS_READY_SHIFT		21
#define KP_PORTAL_STATUS_READY_MASK		1
#define KP_PORTAL_STATUS_DESC_VALID_SHIFT	20
#define KP_PORTAL_STATUS_DESC_VALID_MASK	1
#define	KP_PORTAL_VALID	(	(KP_PORTAL_STATUS_KEY_LOADED_MASK<<KP_PORTAL_STATUS_KEY_LOADED_SHIFT)	|\
				(KP_PORTAL_STATUS_READY_MASK<<KP_PORTAL_STATUS_READY_SHIFT)		|\
 				(KP_PORTAL_STATUS_DESC_VALID_MASK<<KP_PORTAL_STATUS_DESC_VALID_SHIFT)	)
#define KP_PORTAL_STATUS_ECC_DATA_SHIFT	23
#define KP_PORTAL_STATUS_ECC_DATA_MASK	1
#define KP_PORTAL_STATUS_ECC_DESC_SHIFT	22
#define KP_PORTAL_STATUS_ECC_DESC_MASK	1
#define KP_PORTAL_ECC_DATA_ERR	(KP_PORTAL_STATUS_ECC_DATA_MASK<<KP_PORTAL_STATUS_ECC_DATA_SHIFT)
#define KP_PORTAL_ECC_DESC_ERR	(KP_PORTAL_STATUS_ECC_DESC_MASK<<KP_PORTAL_STATUS_ECC_DESC_SHIFT)
#define	KP_PORTAL_KEY_LOADED 	(KP_PORTAL_STATUS_KEY_LOADED_MASK<<KP_PORTAL_STATUS_KEY_LOADED_SHIFT)
typedef struct KeyPortal {
	uint32 key_kp0_pac;                   	/* 0x00       */
	uint32 key_kp0_otp_cmd;                 /* 0x04       */
	uint32 key_kp0_otp_status;              /* 0x08       */
	uint32 key_kp0_rd_desc;                 /* 0x0c       */
	uint32 key_kp0_pgm_desc;                /* 0x10       */
	uint32 key_kp0_portal_status;           /* 0x14       */
	uint32 key_kp0_rsvd0;                   /* 0x18       */
	uint32 key_kp0_rsvd1;                   /* 0x1c       */
	uint32 key_kp0_data[8];
	uint32 reg_gap[16];
	uint32 key_kp1_pac; 			/*0x80*/                  	
	uint32 key_kp1_otp_cmd;                   
	uint32 key_kp1_otp_status;               
	uint32 key_kp1_rd_desc;                 
	uint32 key_kp1_pgm_desc;               
	uint32 key_kp1_portal_status;         
	uint32 key_kp1_rsvd0;                
	uint32 key_kp1_rsvd1;               
	uint32 key_kp1_data[8];
} SecKeyObj,key_portal_t;

typedef struct FsrPortal {
#define FSR_FP_OTP_CMD_READ_DATA		0x2
#define FSR_FP_OTP_CMD_READ_OPCODE_MASK		0xf
#define FSR_FP_OTP_CMD_READ_OPCODE_SHIFT	0
#define FSR_FP_OTP_CMD_WORD_SEL_MASK		0xfff
#define FSR_FP_OTP_CMD_WORD_SEL_SHIFT		4
	uint32 fp_otp_cmd;                 	/* 0x0       */
#define FSR_FP_OTP_STATUS_ACTIVE_MASK		0x1	
#define FSR_FP_OTP_STATUS_ACTIVE_SHIFT		31	
#define FSR_FP_OTP_STATUS_ERR_MASK		0xff
#define FSR_FP_OTP_STATUS_ERR_SHIFT		0	
	uint32 fp_otp_status;              	/* 0x04       */
	uint32 fp_data;                 	/* 0x08      */
	uint32 fp_rd_ecc;                	/* 0x0c       */
#define FSR_FP_PORTAL_STATUS_READY_MASK		0x1
#define FSR_FP_PORTAL_STATUS_READY_SHIFT	21
#define FSR_FP_PORTAL_STATUS_DESC_VALID_MASK	0x1
#define FSR_FP_PORTAL_STATUS_DESC_VALID_SHIFT	20
	uint32 fp_portal_status;           /* 0x10       */
	uint32 fp_rsvd[3];
} fsr_portal_t;

#define FSR_PORTAL_0 ((volatile fsr_portal_t * const) FSR_BASE)

#endif


#ifdef __cplusplus
}
#endif

#endif

