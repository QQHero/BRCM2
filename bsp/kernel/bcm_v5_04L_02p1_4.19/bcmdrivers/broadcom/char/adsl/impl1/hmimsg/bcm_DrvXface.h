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

#ifndef _BCM_DRV_XFACE_H
#define _BCM_DRV_XFACE_H

#include "AdslMibDef.h"
#include "../AdslCore.h"

typedef enum ShowtimeEvent {
    SHOWTIME_NE_DEFECT        = 0x0,
    SHOWTIME_FE_DEFECT        = 0x1,
    SHOWTIME_NE_FAILURE       = 0x2,
    SHOWTIME_FE_FAILURE       = 0x3,
    SHOWTIME_COUNTER_RESP     = 0x4,
    SHOWTIME_TESTPARAM_RESP   = 0x5,
    SHOWTIME_OH_MESSAGE       = 0x6,
    SHOWTIME_TX_BS_APPLIED    = 0x7,
    SHOWTIME_TX_BS_REJECTED   = 0x8,
    SHOWTIME_RX_BS_APPLIED    = 0x9,
    SHOWTIME_RX_BS_FAILED     = 0xA,
    SHOWTIME_L0_L2_TRANS      = 0xB,
    SHOWTIME_L2_POWER_TRIM    = 0xC,
    SHOWTIME_L2_L0_TRANS      = 0xD,
    SHOWTIME_US_MARGIN_LOW    = 0xE,
    SHOWTIME_DS_MARGIN_LOW    = 0xF,
    SHOWTIME_EMERG_FEQ        = 0x10,
    SHOWTIME_TX_SRA_APPLIED   = 0x11,
    SHOWTIME_TX_SRA_REJECTED  = 0x12,
    SHOWTIME_RX_SRA_APPLIED   = 0x13,
    SHOWTIME_RX_SRA_FAILED    = 0x14,
    SHOWTIME_INM_RESP         = 0x15,
    SHOWTIME_VECTOR_RESP      = 0x16,
    SHOWTIME_TX_SOS_APPLIED   = 0x17, /* SHOWTIME_TX_FRA_APPLIED */
    SHOWTIME_TX_SOS_REJECTED  = 0x18, /* SHOWTIME_TX_FRA_REJECTED */
    SHOWTIME_RX_SOS_APPLIED   = 0x19, /* SHOWTIME_RX_FRA_APPLIED */
    SHOWTIME_RX_SOS_FAILED    = 0x1A, /* SHOWTIME_RX_FRA_FAILED */
    SHOWTIME_DLE_TX_MUTE      = 0x1B,
    SHOWTIME_DLE_TX_UNMUTE    = 0x1C,
    SHOWTIME_TX_TIGA_APPLIED  = 0x1D,
    SHOWTIME_TX_RPA_APPLIED   = 0x1E,
    SHOWTIME_TX_RPA_REJECTED  = 0x1F,
    SHOWTIME_RX_RPA_APPLIED   = 0x20,
    SHOWTIME_RX_RPA_FAILED    = 0x21,
    SHOWTIME_SS_DLE_TX_MUTE   = 0x22,
    SHOWTIME_SS_DLE_TX_UNMUTE = 0x23,
}  ShowtimeEvent; 

#define SHOWTIME_TX_FRA_APPLIED         SHOWTIME_TX_SOS_APPLIED
#define SHOWTIME_TX_FRA_REJECTED        SHOWTIME_TX_SOS_REJECTED
#define SHOWTIME_RX_FRA_APPLIED         SHOWTIME_RX_SOS_APPLIED
#define SHOWTIME_RX_FRA_FAILED          SHOWTIME_RX_SOS_FAILED

void BcmXdslNotifyShowtimeEvent(unsigned char lineId, ShowtimeEvent event);


#define kXDslSetConfigGfast                      301
#define kXDslSetRfiConfigGfast                   302
#define kXDslSetHmiCoreConfig                    303
#define kXDslSetHmiDtaConfig                     306
#define kXDslSetHmiChangeMds                     307
#define kXDslSetLineIntroduceErrors              308
#define kXDslSetLogicalFrameConfig               309

/* should be identical to msgCoreConfig in SoftDsl.h */
typedef struct {
  unsigned char                repetitionRate;
  unsigned char                reInitTimeOut;              /* Timeout for CPE detection in [s] */
  unsigned char                gfast_tdd_params_Mds;       /* US/DS split, valid range [2,32] */
  unsigned char                gfast_tdd_params_Mf;        /* currently hardwired to 36       */
  unsigned char                gfast_tdd_params_Msf;       /* currently hardwired to 8        */
  unsigned char                gfast_cyclic_extension_m;   /* Lcp = gfast_cyclic_extension_m*N/64. currently hardwired to 10 */
  unsigned char                enableDtaFraming;          /* if set, the DTA compatible framing (RMC position early in the TDD frame) is enabled. */
  unsigned char                reserved;
} XdslCoreConfig;

extern Bool gSharedMemAllocFromUserContext;
extern adslCfgProfile adslCoreCfgProfile[];
extern XdslCoreConfig xdslCoreCfg;

extern void BcmAdslCoreUpdateConnectionParam(unsigned char lineId);
extern void BcmXdslCoreSendHmiConfig(unsigned char lineId, int configId, void *data, int dataLen);

typedef unsigned char MessageCommand;
typedef unsigned char ErrorType;

#define NO_ERROR  (0)
#define NO_BUFFER (1)
#define TIMEOUT   (2)
#define CRC_FAULT (4)

#define MAX_NUM_MSG_CMD 1

/* Buffer type */
typedef struct Buffer Buffer;
struct Buffer
{
  unsigned char   *data;
  unsigned short  length;
};

/* indicates which buffer is used */
typedef Buffer BufferDescription;

typedef struct BufferPair BufferPair;
struct BufferPair
{
  BufferDescription buffer1;
  BufferDescription buffer2;
};

typedef struct OverheadClient OverheadClient;
typedef void (*MessageReceived)(OverheadClient* overheadClient, ErrorType errorType);

struct OverheadClient
{
  BufferPair                 bufferPair;
  MessageReceived            messageReceived;
  MessageCommand             messageCommand[MAX_NUM_MSG_CMD];
  unsigned short             responseBufferSize;
};


unsigned long XdslDrvGetTime(void);
int XdslDrvIsXdsl2Mod(unsigned char lineId);
int XdslDrvIsGfastMod(unsigned char lineId);
int XdslDrvIsVendorIdReceived(unsigned char lineId);
int XdslCoreG997SendHmiEocData(unsigned char lineId, OverheadClient* overheadClient);
void XdslDrvConnectionStart(unsigned char lineId);
void XdslDrvConnectionStop(unsigned char lineId);
void XdslDrvSetTestMode(unsigned char lineId, int testMode);
void XdslDrvSetL3Mode(unsigned char lineId);
void XdslDrvSendHmiCmd(unsigned char lineId, int cmd, int size, void *data, void *layout);
void XdslDrvSetGfastTestMode(
  unsigned char lineId,
  unsigned char rtxTestModeDs,
  unsigned char rtxTestModeUs,
  unsigned char tpsTcTestModeDs,
  unsigned char tpsTcTestModeUs);
void XdslDrvIntroduceGfastErrors(unsigned char lineId, unsigned int testMode);
void XdslDrvSleep(unsigned int timeMs);
void XdslDrvSleepWithWakeupEvent(unsigned int timeMs);
#endif
