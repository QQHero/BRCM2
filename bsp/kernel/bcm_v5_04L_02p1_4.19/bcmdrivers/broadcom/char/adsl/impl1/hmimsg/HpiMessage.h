
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

/* this file defines the low level HPI message layout  */ 

#ifndef HPIMESSAGE_H
#define HPIMESSAGE_H

#ifndef SUPPORT_HMI
#include "common/types.h"
#endif

#ifdef SUPPORT_HMI

#define HPI_MSG_SIZE      1216
#define HPI_MSG_PENDING   1

/* Here are the possible line States, now defined as bit mask
   These states are also reported through HPI to the modem controller.
   The index of first bit != 0 (minus 1) gives the state as seen by the
   controller.
 */
#define LINE_IDLE_RESET            0xFF /* default state after reset */
                                        /* externally visible as: */
#define LINE_IDLE_NOT_CONFIGURED   0x01 /* 0 */
#define LINE_IDLE_CONFIGURED       0x02 /* 1 */
#define LINE_IDLE_DM_COMPLETE      0x42 /* 7 */

#define LINE_ITU_HANDSHAKE         0x04 /* 2 */
#define LINE_ANSI_HANDSHAKE        0x84 /* 2 */

#define LINE_TRAINING              0x08 /* 3 */
#define LINE_ANA_EXCHANGE          0x88 /* 3 */
#define LINE_DM_TRAINING           0x48 /* 6 */
#define LINE_DM_ANA_EXCHANGE       0xC8 /* 6 */

#define LINE_SHOWTIME              0x10 /* 4 */
#define LINE_SHOWTIME_WITH_TRAFFIC 0x90 /* 4 */
#define LINE_SHOWTIME_L2           0x50 /* 8 */

#define LINE_TEST_STATE            0x20 /* 5 */

/* Moved here from CoreMessageDef.h */
#define INT_IDLE_RESET_IDX                0
#define INT_SRA_NE_RATE_CHANGE_IDX        1
#define INT_SOS_NE_RATE_CHANGE_IDX        2
#define INT_INIT_FAIL_IDX                 3
#define INT_REINIT_IDX                    4
#define INT_VECT_STATE_CHANGE_IDX         5
#define INT_NE_15MIN_THRESHOLD_IDX        6
#define INT_CLEAR_EOC_TX_FAILED_IDX       7

#define INT_FE_15MIN_THRESHOLD_IDX        9
#define INT_PREV_15MIN_RETROFIT_IDX       10
#define INT_VECT_EOC_FAILED_IDX           11
#define INT_LINE_STATE_CHANGE_IDX         12
#define INT_RPA_NE_IDX                    13
#define INT_RPA_FE_IDX                    14
#define INT_SOS_RATE_CHANGE_IDX           15    /* HMI < ((12<<8)+4) - No discrimination between NE and FE */
#define INT_SOS_FE_RATE_CHANGE_IDX        INT_SOS_RATE_CHANGE_IDX
#define INT_FAILURE_STATE_CHANGE_IDX      16
#define INT_EOC_MSG_RECEIVED_IDX          17
#define INT_HMI_TRANSACTION_FINISHED_IDX  18
#define INT_CLEAR_EOC_RX_IDX              19
#define INT_CPE_LOP_IDX                   20
#define INT_AUTO_RESET                    21
#define INT_CLEAR_EOC_TX_IDX              22
#define INT_BITSWAP_NE_IDX                23
#define INT_BITSWAP_FE_IDX                24
#define INT_15MIN_PERIOD_IDX              25
#define INT_24H_PERIOD_IDX                26
#define INT_ATM_BONDING_RATE_CHANGE_IDX   27
#define INT_TIGA_IDX                      28
#define INT_SRA_RATE_CHANGE_IDX           29    /* HMI < ((12<<8)+4) - No discrimination between NE and FE */
#define INT_SRA_FE_RATE_CHANGE_IDX        INT_SRA_RATE_CHANGE_IDX
#ifdef ATU_R
  #define INT_ALIGNMENT_ERROR_IDX         30   /* BCM6519 non-ncoMaster line has an offset outside the [-6144,2048] sample window of the BCM65800 */
#else
  #define INT_NO_UPBO_SUPPORT             30   /* signals that the CPE doesn't support US PBO correctly */
#endif
#define INT_BONDING_INCONSISTENT_CONFIG   31

#define INT_IDLE_RESET_MASK               (1<<INT_IDLE_RESET_IDX)
#define INT_SRA_NE_RATE_CHANGE_MASK       (1<<INT_SRA_NE_RATE_CHANGE_IDX)
#define INT_SOS_NE_RATE_CHANGE_MASK       (1<<INT_SOS_NE_RATE_CHANGE_IDX)
#define INT_INIT_FAIL_MASK                (1<<INT_INIT_FAIL_IDX)
#define INT_REINIT_MASK                   (1<<INT_REINIT_IDX)
#define INT_VECT_STATE_CHANGE_MASK        (1<<INT_VECT_STATE_CHANGE_IDX)
#define INT_NE_15MIN_THRESHOLD_MASK       (1<<INT_NE_15MIN_THRESHOLD_IDX)
#define INT_FE_15MIN_THRESHOLD_MASK       (1<<INT_FE_15MIN_THRESHOLD_IDX)
#define INT_PREV_15MIN_RETROFIT_MASK      (1<<INT_PREV_15MIN_RETROFIT_IDX)
#define INT_FAILURE_STATE_CHANGE_MASK     (1<<INT_FAILURE_STATE_CHANGE_IDX)
#define INT_EOC_MSG_RECEIVED_MASK         (1<<INT_EOC_MSG_RECEIVED_IDX)
#define INT_HMI_TRANSACTION_FINISHED_MASK (1<<INT_HMI_TRANSACTION_FINISHED_IDX)
#define INT_CLEAR_EOC_TX_FAILED_MASK      (1<<INT_CLEAR_EOC_TX_FAILED_IDX)
#define INT_VECT_EOC_FAILED_MASK          (1<<INT_VECT_EOC_FAILED_IDX)
#define INT_CLEAR_EOC_RX_MASK             (1<<INT_CLEAR_EOC_RX_IDX)
#define INT_CPE_LOP_MASK                  (1<<INT_CPE_LOP_IDX)
#define INT_AUTO_RESET_MASK               (1<<INT_AUTO_RESET)
#define INT_CLEAR_EOC_TX_MASK             (1<<INT_CLEAR_EOC_TX_IDX)
#define INT_BITSWAP_NE_MASK               (1<<INT_BITSWAP_NE_IDX)
#define INT_BITSWAP_FE_MASK               (1<<INT_BITSWAP_FE_IDX)
#define INT_15MIN_PERIOD_MASK             (1<<INT_15MIN_PERIOD_IDX)
#define INT_24H_PERIOD_MASK               (1<<INT_24H_PERIOD_IDX)
#define INT_ATM_BONDING_RATE_CHANGE_MASK  (1<<INT_ATM_BONDING_RATE_CHANGE_IDX)
#define INT_RPA_NE_MASK                   (1<<INT_RPA_NE_IDX)
#define INT_RPA_FE_MASK                   (1<<INT_RPA_FE_IDX)
#define INT_TIGA_MASK                     (1<<INT_TIGA_IDX)
#define INT_SRA_RATE_CHANGE_MASK          (1<<INT_SRA_RATE_CHANGE_IDX)
#define INT_SOS_RATE_CHANGE_MASK          (1<<INT_SOS_RATE_CHANGE_IDX)
#define INT_SRA_FE_RATE_CHANGE_MASK       (1<<INT_SRA_FE_RATE_CHANGE_IDX)
#define INT_SOS_FE_RATE_CHANGE_MASK       (1<<INT_SOS_FE_RATE_CHANGE_IDX)
#define INT_LINE_STATE_CHANGE_MASK        (1<<INT_LINE_STATE_CHANGE_IDX)
#if defined(VDSL) && defined(ATU_R)
#define INT_ATUC_NOT_VDSL_MASK            (1<<INT_ATUC_NOT_VDSL)
#endif

#define INT_15MIN_THRESHOLD_MASK (INT_NE_15MIN_THRESHOLD_MASK  \
                                | INT_FE_15MIN_THRESHOLD_MASK)

#elif 1

#if defined(ATU_C)
/* on mikeno/tacana the buffer mgr is in dmem so we have more space in the top page of smem */
#if defined(BOOT_HPI_GEOMETRY)
#  define HPI_MSG_SIZE      4072
#  define HPI_MSG_PENDING   2
#elif defined(NOISE_GENERATOR)
#  define HPI_MSG_SIZE      4188
#  define HPI_MSG_PENDING   2
#elif (CHIPSET >= KOS_DEV) && !defined(VDSL)
#  define HPI_MSG_SIZE      (512+256)
#  define HPI_MSG_PENDING   9
#elif (CHIPSET >= TACANA_DEV)
#  define HPI_MSG_SIZE      (512+256)
#if (CHIPSET >= KOS_DEV)
#  define HPI_MSG_PENDING   17
#else
#  define HPI_MSG_PENDING   15
#endif
#else
#  define HPI_MSG_SIZE      (512+256)
#  define HPI_MSG_PENDING   11
#endif

#else /* ATU_R */

#if (PLATFORM==LINUX)
 #define HPI_MSG_SIZE      512
 #define HPI_MSG_PENDING   13
#else
 #define HPI_MSG_SIZE      1216
 #define HPI_MSG_PENDING   4
#endif

#endif /*ATU_C*/

#elif defined(BOOT_HPI_GEOMETRY)

#define HPI_MSG_SIZE      4072
#define HPI_MSG_PENDING   4

#else

/* geometry used for testing */
#define HPI_MSG_SIZE      1024*2 /* 1024*2 */
#define HPI_MSG_PENDING   3

#endif

#define HPI_HEADER_LENGTH   6 /* number of uint32 inside the HPI header */
#define HPI_SERVICE_PAYLOAD_SIZE (HPI_MSG_SIZE - HPI_HEADER_LENGTH*4)
/* The have two bytes offset between the request and reply queues. This
   creates a gap between queue entries, used to check that no writing out of
   bound takes place.
*/
#define HPI_QUEUE_OFFSET         2
#define HPI_MSG_PAD_SIZE_LONG   ((HPI_QUEUE_OFFSET+7)>>3)

typedef struct HpiMsgHeader HpiMsgHeader;
struct HpiMsgHeader
{
  uint32 sourceRef;  /* transparently sent back in the reply header */
  uint32 requestRef; /* transparently sent back in the reply header */
  uint16 portNumber; /* least significant bytes is assumed to remain lower
                        than 16. This would allow (if ever necessary) to
                        make a difference between HPI over AAL5 and IP over
                        AAL5 */
  uint16 len; /* length of the message data */
  uint8  replyAddress[5]; /* ATM header to be used for the reply in case
                             the request is sent through ATM */
  int8   result;  /* dispatching result */
  uint16 buildId; /* only valid in the reply correspond to the build id of the
                     SW running in the santorini */
  uint16 commandId; /* command to be executed */
  int16  errorCode;  /* could be reused for other purposes */
};


typedef struct HpiDefaultMessage HpiDefaultMessage;
struct HpiDefaultMessage
{
  HpiMsgHeader header;
  uint32       data[HPI_SERVICE_PAYLOAD_SIZE/4];
  /* Next field is needed to avoid trashing the header of the next message in the queue when
     posting a reply, since request and reply queues overlap with some offset
     and we need to keep uint64 alignment.
  */
  uint64       pad[HPI_MSG_PAD_SIZE_LONG];
};

/* with each commandId, 2 types are associated :
     - 1 type which identify which C structure is associated with the request
     - 1 type which identify which C structure is associated with the reply

     For each command, a data layout should uniquely identify the exact data
     layout to be used by the HPI client.

     Some protocol could be implemented on top of the HPI. In this case, the
     payload data will be split into an additional header and a protocol
     payload.

     The destination should be used for further routing inside the
     firepath. Each destination should be associated with     
*/

/* common operation results */
#define HPI_SUCCESS                 0
#define UNAVAILABLE_PORTNUMBER      1
#define REQUEST_STILL_PENDING       2



#define concat(a,b) a ## b

/* In order to indicate the types - messageId association, a macro should be
   used : (next macro should be moved to somewhere else) */


#ifdef DATA_PARSING

#ifndef NEW_DATA_PARSING
#define HPI_SERV_DEF(Name, CommandId, RequestType, ReplyType) \
   typedef struct concat(Name,_MsgReq) concat(Name,_MsgReq); \
   struct concat(Name,_MsgReq) { RequestType data; }; \
   typedef struct concat(Name,_MsgRep) concat(Name,_MsgRep); \
   struct concat(Name,_MsgRep) { ReplyType data; };\
   new Msg Def  ++  Name ++  CommandId ++ RequestType ++ ReplyType;
#else /* DATA_PARSING with struct-info */
/* type definition to be used when no parameters are expected */
typedef uint32  DUMMY;
#define HPI_SERV_DEF(Name, CommandId, RequestType, ReplyType) \
    struct MgsDef_##Name { \
        char commandId[CommandId];\
        RequestType request;\
        ReplyType reply;\
    };\
   typedef struct concat(Name,_MsgReq) concat(Name,_MsgReq); \
   struct concat(Name,_MsgReq) { RequestType data; }; \
   typedef struct concat(Name,_MsgRep) concat(Name,_MsgRep); \
   struct concat(Name,_MsgRep) { ReplyType data; };
#endif
#else



/* type definition to be used when no parameters are expected */
typedef uint32  DUMMY;

#define HPI_SERV_DEF(Name, CommandId, RequestType, ReplyType) \
   typedef struct concat(Name,_MsgReq) concat(Name,_MsgReq); \
   struct concat(Name,_MsgReq) { RequestType data; }; \
   typedef struct concat(Name,_MsgRep) concat(Name,_MsgRep); \
   struct concat(Name,_MsgRep) { ReplyType data; };

#endif
/* the above macro should:
   - define two types:  <Name>_MsgReq and <Name>_MsgRep
     those are structures with
        - service payload

   => the receiving application should always check whether enough data has
      been received
*/

/* reserved service results */
#define HPI_UNKNOWN_COMMAND_ID       -1   /* the received command id is not
                                             offered by the application */
#define HPI_WRONG_HEADER_FORMAT      -3   /* the HPI headers are not consistent */
#define HPI_INVALID_REQUEST_DATA     -4   /* the request data is not valid or
                                             not consistent */
#define HPI_NOT_POSSIBLE_IN_CURRENT_STATE -5 /* the request cannot be
                                                performed in the current
                                                state:
                                                e.g. try to change the config
                                                while running, try to get info
                                                which is not yet available,
                                                ... */
#define HPI_RESOURCES_ALREADY_IN_USE -6 /* Some ressources needed to accept
                                           the message are in use => the
                                           message is rejected */

#endif
