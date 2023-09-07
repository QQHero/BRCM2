/* 
* <:copyright-BRCM:2002:proprietary:standard
* 
*    Copyright (c) 2002 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

#ifndef OVERHEAD_H
#define OVERHEAD_H

#ifndef BCM_BASIC_TYPES_H
#include "bcm_BasicTypes.h"
#endif

#define NR_OF_MESSAGE_TYPES 2
/*Use this to determine the required buffer size for overhead messaging.
 *Input: the max. message length*/
/*The actual buffer size must be 3 bytes more than the max. message length*/
/*An extra 4 bytes are added to store segmentation information */
/*FIXME: Temp. +3, should be +2 after HDLC fix*/
#define CALC_RX_OVERHEAD_BUF_LEN(msgLen) ((msgLen) + 7)
#define CALC_TX_OVERHEAD_BUF_LEN(msgLen) (msgLen)

#define SET_MGR_STATE(MgrState,newState,prio)   {(MgrState)[(prio)] = (newState);}
#define SET_SRV_STATE(SrvState,newState,prio)   {(SrvState)[(prio)] = (newState);}           
          
#define MAXTIME_HIGH_PRIORITY 400
#define MAXTIME_MEDIUM_PRIORITY 800
#define MAXTIME_LOW_PRIORITY 1000

#define MGR_COUNTER_READ_COMMAND                    0x05
#define MGR_COUNTER_READ_COMMAND_SPECIFIC_TYPE_STD   0x1
#define MGR_COUNTER_READ_REPLY_COMMAND_STD          0x81
#define MGR_COUNTER_READ_COMMAND_SPECIFIC_TYPE_FIRE  0xF
#define MGR_COUNTER_READ_REPLY_COMMAND_FIRE         0x8F
#define COUNTERMGR_REQUEST_MSG_LENGTH                  4
#define COUNTERMGR_REPLY_MSG_LENGTH_STD               32 /* basic PMS_TC counters */
#define COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_LP           8 /* FEC and CRC for LP1 */
#define COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_FIRE        12 /* rtx, rtx_c, rtx_uc */
#define COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_GINP        20 /* rtx_c, rtx_uc, leftrs, errFreeBits, minEftr */
#define COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_GINP_REV     4 /* rtx */
#ifdef G998_4_R18
#define COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_GINP_R18     4 /* maxEftr */
#else
#define COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_GINP_R18     0 /* temp dummy define */
#endif
#define COUNTERMGR_REPLY_MSG_LENGTH_ATM_BEARER        16 /* 4 ATM counters */
#define COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_VECTOR       4 /* 4 bytes downstream counters */
#define COUNTERMGR_REPLY_MSG_MAX_LENGTH MAX((  COUNTERMGR_REPLY_MSG_LENGTH_STD \
                                             + COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_LP \
                                             + COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_GINP \
                                             + COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_GINP_REV \
                                             + COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_GINP_R18 \
                                             + COUNTERMGR_REPLY_MSG_LENGTH_ATM_BEARER \
                                             + COUNTERMGR_REPLY_MSG_LENGTH_EXTRA_VECTOR), \
                                             COUNTERMGR_REPLY_MSG_LENGTH_GFAST)

#define COUNTERMGR_REPLY_MSG_LENGTH_GFAST         68   /* expects ANDEFTR counters by default */

#define MGR_CLEAREOC_READ_COMMAND   0x08
#define MGR_CLEAREOC_SUBCOMMAND     0x01
#define MGR_DATAGRAM_COMMAND        0x0a

#define CLEAREOCMGR_REPLY_MSG_LENGTH 4
#ifdef GFAST_SUPPORT
#define TRANSPARENTEOCMGR_MAX_MSG_LENGTH DATAGRAM_MAX_BUFFER_GFAST+4
#else
#define TRANSPARENTEOCMGR_MAX_MSG_LENGTH CLEAREOC_MAX_BUFFER_IMPL+4
#endif
#define MAX_SOFTWAREDOWNLOAD_SIZE_US 256

/* ADSL = 5, VDSL = 4, high res NTR = 6 */ 
#define TESTPARAMETERS_MAX_REQUEST_MSG_SIZE 6 
/* #define TESTPARAMETERS_MAX_REQUEST_MSG_SIZE 28  */  /* If PTP is enabled, the max tx msg size is 28
                                                          However, the TX buff will overflow into the RX buff.
                                                          Since the RX buff is not used together witht he TX, and because the TX is large enough,
                                                          we can get away with it */

#define GFAST_OLRMGR_REQUEST_MSG_LENGTH_TX      1024
#ifdef GFAST212_SUPPORT
#define GFAST_OLRMGR_REQUEST_MSG_LENGTH_RX      4096   /* Needed to support requests containing both NOI and DOI */
#else
#define GFAST_OLRMGR_REQUEST_MSG_LENGTH_RX      2048   /* Needed to support requests containing both NOI and DOI */
#endif

#define GFAST_OLRMGR_REPLY_MSG_LENGTH           48
#define GFAST_OLR_RESP_MSG_SIZE                 5

#define GFAST_PWRMGR_REQUEST_MSG_LENGTH         5
#define GFAST_PWRMGR_REPLY_MSG_LENGTH           5

#define GFAST_NTRMGR_MSG_LENGTH                 8
#define GFAST_TODMGR_MSG_LENGTH                 8

#define GFAST_DIAGMGR_REQUEST_MSG_LENGTH        4
#define GFAST_DIAGMGR_REPLY_MSG_LENGTH          5

#define GFAST_PSUPDATEMGR_REQUEST_MSG_LENGTH    8+PROBE_SEQUENCE_LEN/4
#define GFAST_PSUPDATEMGR_REPLY_MSG_LENGTH      8

#define BACKUP_NSCR                             512
#define GFAST_BACKUPRMC_REQUEST_MSG_LENGTH      11+2*BACKUP_NSCR
#define GFAST_BACKUPRMC_REPLY_MSG_LENGTH        8

#define GFAST_TIMESYNCMGR_REQUEST_MSG_LENGTH    30
#define GFAST_TIMESYNCMGR_REPLY_MSG_LENGTH      30

#define GFAST_VECTORFBMGR_REQUEST_MSG_LENGTH    100
#define GFAST_VECTORFBMGR_REPLY_MSG_LENGTH      1024

#define VDSL_SINGLE_READ_COMMAND_BASE_LEN 44 
#define VDSL_SINGLE_READ_GINP_EXT          8
#define VDSL_SINGLE_READ_COMMAND_LEN (VDSL_SINGLE_READ_COMMAND_BASE_LEN+VDSL_SINGLE_READ_GINP_EXT)

#define NON_STANDARD_FACILITY_COMMAND   0x3f

#ifdef GFAST_SUPPORT
#define NSF_SUPPORT 1 //we support the NSF command on gfast FW (ikanos jpn feature)
#define TESTPARAMETERS_MAX_REPLY_MSG_SIZE (4*512+6) /* = MAX_TX_TESTPAR_MSG_SIZE of the CPE; no segmentation */
#else
#define TESTPARAMETERS_MAX_REPLY_MSG_SIZE VDSL_SINGLE_READ_COMMAND_LEN
#endif /* GFAST_SUPPORT */

#define INVENTORY_LIST_MSG_LENGTH 65

#define MGR_TESTPARAMETERS_READ_COMMAND 0x81
#define SINGLE_READ_COMMAND        0x01
#define NEXT_MULTIPLE_READ_COMMAND 0x03
#define MULTIPLE_READ_COMMAND      0x04
#define BLOCK_READ_COMMAND         0x05
#define VECTOR_BLOCK_READ_COMMAND  0x06
#define SCALAR_READ_COMMAND        0x07

#define TIME_SYNC_COMMAND          0x51

#define TESTPARAMETERS_NACK        0x80
#define SINGLE_READ_COMMAND_ACK	   0x81
#define MULTIPLE_READ_COMMAND_ACK  0x82
#define BLOCK_READ_COMMAND_ACK     0x84
#define VECTOR_READ_COMMAND_ACK    0x86
#define SCALAR_READ_COMMAND_ACK    0x87

#define SINGLE_READ_ADSL_HLOG_PS 0x01
#define SINGLE_READ_ADSL_QLN_PS  0x03
#define SINGLE_READ_ADSL_SNR_PS  0x04

#define LOOP_ATTENUATION         0x21
#define SIGNAL_ATTENUATION       0x22
#define SNR_MARGIN               0x23
#define ATTAINABLE_NET_DATA_RATE 0x24
#define NE_ACTATP                0x25
#define FE_ACTATP                0x26
#define FE_ACTINP                0x27
#define FE_SNR_MARGIN_ROC        0x28
#define SINGLE_READ_XDSL_NTR     0x29
#define RTX_FE_ACT_INP_SHINE     0x41
#define RTX_FE_ACT_INP_REIN      0x42
#define RTX_NE_ACT_ETR           0x43
#define RTX_NE_ACT_DELAY         0x44
#define SEND_PTP_INFO            0xFE 
#define VDSL2_TEST_PARAMETER_ID  0xFF

/* if a CMD is added, update testParameterIds array accordingly (TestParametersMgr.c) */
#define CMD_LOOP_ATTENUATION         (1<<0)
#define CMD_SIGNAL_ATTENUATION       (1<<1)
#define CMD_SNR_MARGIN               (1<<2)
#define CMD_ATTAINABLE_NET_DATA_RATE (1<<3)
#define CMD_NE_ACTATP                (1<<4)
#define CMD_SINGLE_READ_XDSL_NTR     (1<<5)
#define CMD_RTX_FE_ACT_INP_SHINE     (1<<6)
#define CMD_RTX_FE_ACT_INP_REIN      (1<<7)
#define CMD_RTX_NE_ACT_ETR           (1<<8)
#define CMD_RTX_NE_ACT_DELAY         (1<<9)
#define CMD_VDSL2_TEST_PARAMETER_ID  (1<<10)
#define CMD_FE_SNR_MARGIN_ROC        (1<<11)
#define CMD_SEND_PTP_INFO            (1<<12) 
#define CMD_FE_ACTINP                (1<<13)

#define MGR_INM_FACILITY_COMMAND    0x89
#ifdef INM
 #define MGR_INM_FACILITY_RESPONSE   MGR_INM_FACILITY_COMMAND

 #define READ_INM_COUNTERS_COMMAND   0x02
 #define SET_INM_PARAMETERS_COMMAND  0x03
 #define READ_INM_PARAMETERS_COMMAND 0x04
 #define INM_ACK_RESPONSE            0x80
 #define INM_NACK_RESPONSE           0x81
 #define INM_COUNTERS_RESPONSE       0x82
 #define INM_PARAMETERS_RESPONSE     0x84

 #define READ_INM_COUNTERS_SIZE         2
 #define INM_VDSL_PARAMETERS_SIZE       7
 #define INM_GFAST_PARAMETERS_SIZE      8
 #define READ_INM_PARAMETERS_SIZE       2

 #define INM_ACK_SIZE                   3
 #define INM_NACK_SIZE                  2
 #define INM_COUNTERS_SIZE            107

 #define MAX_INPEQ_MODE                 3
 #define INM_INPEQ_MODE_ACCEPTED      0x80
 #define INM_INPEQ_MODE_NOT_SUPPORTED 0x81
#ifdef GFAST_SUPPORT
 #define INM_MAX_REQUEST_MSG_SIZE     INM_GFAST_PARAMETERS_SIZE
#else
 #define INM_MAX_REQUEST_MSG_SIZE     INM_VDSL_PARAMETERS_SIZE
#endif
 #define INM_MAX_REPLY_MSG_SIZE       INM_COUNTERS_SIZE

 #define MGR_LOWPRIO_NSF_FACILITY_COMMAND  0xBF
 #define MGR_LOWPRIO_NSF_FACILITY_RESPONSE MGR_LOWPRIO_NSF_FACILITY_COMMAND

 #define LOWPRIO_NSF_COMMAND_ID            0x01
 #define LOWPRIO_NSF_ACK_RESPONSE          0x80
 #define LOWPRIO_NSF_NACK_RESPONSE         0x81
 #define LOWPRIO_NSF_RESP_SIZE             14

 #define LOWPRIO_NSF_MAX_REQUEST_MSG_SIZE  14
 #define SET_INM_ISDD_SENSITIVITY          0x01
 #define READ_INM_ISDD_SENSITIVITY         0x02
 #define REPORT_INM_ISDD_SENSITIVITY       0x82

#endif

#ifdef VECTORING
/* High priority vectoring command */
#define VECTORMGR_HIGH_CMD_ID              0x18
#define VECTORMGR_START_DUMP_CMD_ID        0x01
#define VECTORMGR_START_DUMP_MSG_LENGTH    (4+7+5*8)

/* Normal priority vectoring command */
#define VECTORMGR_NORM_CMD_ID              0x11
#define VECTORMGR_UPDATE_PILOT_CMD_ID      0x01
#define VECTORMGR_UPDATE_PILOT_MSG_LENGTH  (4+1+64)
#define VECTORMGR_UPDATE_PILOT_FDPS_CMD_ID      0x02
#define VECTORMGR_UPDATE_PILOT_FDPS_MSG_LENGTH  (4+1+64+8+7*64)

/* Proprietary vectoring command */
#define VECTORMGR_PROPRIETARY_CMD_ID       0x0F
#define VECTORMGR_SET_MODE_CMD_ID          0x04
#define VECTORMGR_SET_MODE_MSG_LENGTH      (4+sizeof(VectorMode))

#define VECTORMGR_ACK_RESPONSE             0x80
#define VECTORMGR_NACK_RESPONSE            0x81

typedef union VectorMgrEocBufferCmd VectorMgrEocBufferCmd;
union VectorMgrEocBufferCmd
{
  uint8 startDumpCmd[VECTORMGR_START_DUMP_MSG_LENGTH];
  uint8 setModeCmd[VECTORMGR_SET_MODE_MSG_LENGTH];
  uint8 updatePilotCmd[VECTORMGR_UPDATE_PILOT_MSG_LENGTH];
  uint8 updatePilotFdpsCmd[VECTORMGR_UPDATE_PILOT_FDPS_MSG_LENGTH];
};
typedef union VectorMgrEocBufferResponse VectorMgrEocBufferResponse;
union VectorMgrEocBufferResponse
{
  uint8 ack[4+4];
};

#define VECTORMGR_REQUEST_MSG_LENGTH       (sizeof(VectorMgrEocBufferCmd))
#define VECTORMGR_REPLY_MSG_LENGTH         (sizeof(VectorMgrEocBufferResponse))

#endif /* VECTORING */

#if defined(VECTORING) && defined(ATU_R)
#define NORM_PRIO_SERVER_RX_BUFFER_NO_VECTOR    MAX(INVENTORY_LIST_MSG_LENGTH,COUNTERMGR_REPLY_MSG_MAX_LENGTH)
#define NORM_PRIO_SERVER_RX_BUFFER              MAX(NORM_PRIO_SERVER_RX_BUFFER_NO_VECTOR,VECTORMGR_REQUEST_MSG_LENGTH)
#else
#define NORM_PRIO_SERVER_RX_BUFFER              MAX(INVENTORY_LIST_MSG_LENGTH,COUNTERMGR_REPLY_MSG_MAX_LENGTH)
#endif

#define MGR_ADSL2EOC_READ_COMMAND     0x41
#define ADSL2EOC_ACK                  0x80
#define ADSL2EOC_REQUEST_MSG_SIZE        4
#define ADSL2EOC_REPLY_MSG_SIZE          4
#define PERFORM_SELF_TEST_COMMAND     0x01
#define UPDATE_TESTPARAMETERS_COMMAND 0x02
#define START_CORRUPT_TX_CRC          0x03
#define END_CORRUPT_TX_CRC            0x04
#define START_CORRUPT_RX_CRC          0x05
#define END_CORRUPT_RX_CRC            0x06
#define ENTER_RTX_TEST_MODE           0x07
#define LEAVE_RTX_TEST_MODE           0x08

#define GFAST_PERFORM_SELF_TEST       0x01
#define GFAST_UPDATE_TEST_PARAM       0x02
#define GFAST_ENTER_RTX_TEST_MODE     0x03
#define GFAST_LEAVE_RTX_TEST_MODE     0x04
#define GFAST_ENTER_TPS_TEST_MODE     0x05
#define GFAST_LEAVE_TPS_TEST_MODE     0x06

#define NSF_MEDIUM_PRIO               0x3F
#define NSF_LOW_PRIO                  0xBF
#define OLR_COMMAND                   0x01
#define PMM_COMMAND                   0x07
#define PMM_SIMPLE_REQUEST            0x01
#define PMM_L3_STATE                  0x03
#define VECTORFEEDBACK_COMMAND        0x18
#define L3_COMMAND                    0x09
#define NTR_COMMAND                   0x50
#define TOD_COMMAND                   0x52
#define TIMESYNC_COMMAND              0x51
#define PROBESEQUPDATE_COMMAND        0x11
#define BACKUPRMC_COMMAND             0x57


#define COMMAND_UNIMPORTANT 0x00

/* used by HmiMgr, High prio messages (OLR) are not allowed */
  #define determinePriority(messageCommand)                          \
     ((    ((messageCommand) == (MGR_TESTPARAMETERS_READ_COMMAND))   \
        || ((messageCommand) == (MGR_INM_FACILITY_COMMAND))          \
        || ((messageCommand) == (NSF_LOW_PRIO))) ? (LOW) : (MEDIUM))


/*TEMP_LOCAL and TEMP_EXTERNAL indicate the default allocation scheme is temporarily overruled. We roll back to the default as soon
 *as the corresponding buffer is transmitted. Currently applies to transmit buffers only.*/
typedef enum BufferAllocation {LOCAL_ALLOCATION, EXTERNAL_ALLOCATION, TEMP_LOCAL_ALLOCATION, TEMP_EXTERNAL_ALLOCATION,CUSTOM_ALLOCATION} BufferAllocation;
typedef enum Priority {HIGH = 0, MEDIUM = 1, LOW = 2, NEARHIGH = 3} Priority;
typedef enum MessageType {COMMAND = 0, RESPONSE = 1} MessageType;

#define LAST_SEGMENT (0)
#define NOT_LAST_SEGMENT (1)
#define ACK_MESSAGE (2)

/* max buffersize to be returned by HDLC */
#define SIZE_RX_BUFFER 1024

#ifdef GFAST_SUPPORT
#define EOC_NUMBER_OF_PRIORITIES 4
#else
#define EOC_NUMBER_OF_PRIORITIES 3
#endif

#endif /*OVERHEAD*/
