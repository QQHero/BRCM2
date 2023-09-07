
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

#ifndef LINEMGR_H
#define LINEMGR_H

#ifndef ATU_C
#define ATU_C
#endif
#ifndef VDSL
#define VDSL
#endif

#ifndef BCM_BASIC_TYPES_H
#include "bcm_BasicTypes.h"
#endif
#ifndef BCM_HMI_LINE_MSG_H
#include "bcm_hmiLineMsg.h"
#endif
#ifndef LINEMESSAGEDEF_H
#include "LineMessageDef.h"
#endif

#include "bcm_DrvXface.h"

uint16 bswap16(const uint16 arg);

#define RUNNING_STATES      (LINE_ITU_HANDSHAKE | LINE_TRAINING | LINE_SHOWTIME)
#define IDLE_STATES         (LINE_IDLE_NOT_CONFIGURED | LINE_IDLE_CONFIGURED)
#define ALL_STATES           0xFF
#define MIN_IS_MAX           0xFFU

/* status control field meaning */
#define GINP_REPORT_LOCAL_NE_VALUES     0x1
#define GINP_REPORT_LOCAL_FE_VALUES     0x2
#ifdef ATU_C
#define CONSTRAINED_ATTNDR_US           0x4
#define CONSTRAINED_ATTNDR_DS           0x8
#endif

typedef struct LineMgr LineMgr;

/* function that handle a line HpiService, return a pointer to the reply
   message. If the service does not send any reply data the return pointer
   will be NULL and the reply message will be allocated by the caller. In this
   case *sizep is not filled in
*/
typedef void *LineHpiService(LineMgr *me,  /* pointer to the line Mgr */
                             void    *msgp,/* pointer to the message to handle */
                             uint16  *sizep,/*size of the reply message is
                                              allocated */
                             int     *resultp,/* result of the service */
                             uint16  hmiVersion);

typedef struct LineHpiServiceInfo LineHpiServiceInfo;
struct LineHpiServiceInfo {
  LineHpiService *handler;
  int16           size;
  uint8           minSize;
  uint8           allowedState;
};

#define NR_PHY_BANDS                    5
typedef struct BandsDescriptor BandsDescriptor;
struct BandsDescriptor
{
  uint8         noOfToneGroups;
  uint8         noCheck;                /* bypass validation logic (should only be used in lineTest and loopback modes) */
  ToneGroup     toneGroups[NR_PHY_BANDS];
};

#define MAX_NR_RFI_BANDS 32
typedef struct RfiBands RfiBands;
struct RfiBands
{
  uint8  nrOfRfiBands; 
  uint8  nrOfRfiGaps;  
  ToneGroup toneGroups[MAX_NR_RFI_BANDS]; /* max tot = 32 (gfast build) or 24 */
};

#define VDSL_MSG_MAX_NO_OF_TONE_GROUPS_IN_REF_PSD_DESC     5

typedef struct VdslMsgReferencePsdDescriptor VdslMsgReferencePsdDescriptor;
struct VdslMsgReferencePsdDescriptor
{
  uint8  noOfToneGroups;
  uint8  pboMethod;
  /*  
      This field controls the method used to apply UPBO (only meaningful if
      the noOfToneGroups is larger than 0.
      The default upstream power backoff method applied follows G993.2 ammendment 1. 
      - bit 0 : reserved
      - bit 1 : if set, the upstream PBO is computed according to the
        FEXT-optimized  UPBO method, G993.2, ammendment 2
      - bit 2 : if set, the upstream PBO is computed according to the
        FEXT-optimized UPBO method, without collaboration from the CPE,
        allowing to use this method with a CPE that doesn't support ammendment
        2. This requires a retrain to install a new reference PSD that yields
        the upstream PSD as desired for that loop.   
  */
  PboSetting pboSetting[VDSL_MSG_MAX_NO_OF_TONE_GROUPS_IN_REF_PSD_DESC];
};


typedef struct PhysicalLayerConfigGfastInternal PhysicalLayerConfigGfastInternal;
struct PhysicalLayerConfigGfastInternal
{
  PsdDescriptorGfast             limitingMaskDn;
  PsdDescriptorGfast             limitingMaskUp;
  BandsDescriptor                bandPlanDn;
  BandsDescriptor                bandPlanUp;
  RfiBands                       rfiBands;
  RfiBands                       carMaskDn;
  RfiBands                       carMaskUp;
  uint16                         iarBands;               /* 12 bits bitmask following coding as in G998.4 table 11.70.8 */
  VdslMsgReferencePsdDescriptor  usPboDescriptor;
  uint8                          enableProfileBitmap;    /* bit0 = 106a, bit1 = 212a, bit2 = 106b, bit3 = 106c, bit4 = 212c */
  uint8                          vectorFlag;             /* bitmap. bit0 = reserved, bit1 = (internal VCE) precoder will be calculated, but it will not be installed */
  int16                          forceElectricalLength;
  uint8                          retrainAllowed;         /*
                                                           0: Not retrain allowed for PSD optimization
                                                           1: Retrain allow in O-PRM/R-PRM for PSD optimization
                                                           2: Force retrain in O-PRM/R-PRM for debug
                                                           3: Retrain allow in O-PRM/R-PRM for PSD optimization, but only if CPE supports extended timeouts
                                                         */
  uint8                          reserved;
  int16                          maxRxPowerUp;           /* unit is (0.1 dB) on itf but internally 1/256 dB */
  uint16                         maxToneUsed;            /* GFAST US and DS bandplans will be clipped according to this tone -  active when maxUsedTone>0 */
  int16                          initialTgPrime;         /* initial value of Tg1' that the CPE should install, -1 to let the FW decide */
  int8                           m_ds;                   /* nr of active frames in the DS TDD slot [2..M_DS], -1 means fill all available space */
  int8                           m_us;                   /* nr of active frames in the US TDD slot [2..M_US], -1 means fill all available space */
  uint8                          soc_redundancy;         /* DS soc repetition rate in ChannelDiscovery 0 = minimal/1 = max (m_ds/4) */
  uint8                          reservedA[3];
  LinkConfig                     linkConfig[2];
  int8                           reservedB[4];
  uint8                          rmcrConfig[2];          /* Unit is 50 ms, 0 means RMC-R is disabled */
  ReinitConfig                   reinitConfig[2];
}; // sizeof = 800
typedef struct GfastConfigInternal
{
  PhysicalLayerConfigGfastInternal phyConfig;
  TrafficConfigGfast       trafficConfig[2];
  RmcConfig                rmcConfig[2];
  FraConfig                fraConfig[2];
} GfastConfigInternal;

#define HMI_BUFFER_SIZE 1030 /* correct size (1024 + 2 + 4)*/

typedef struct HmiBufferMgr
{
  uint64 lockTime;
  uint8 localBuffer[HMI_BUFFER_SIZE];
  Bool  bufferState;
  uint8 lineId;
} HmiBufferMgr;


struct LineMgr
{
  uint8                 lineId;
  uint8                 lineState;
  uint8                 updateTestParams;
  uint8                 keepOam;
  LineStatus            lineStatus;
  LineTrafficConfig     lineTrafficConfig;
  PhysicalLayerConfig   linePhysicalLayerConfig;
  TestConfigMessage     lineTestConfig;
  LineExtraConfig       lineExtraConfig;
  HmiOverheadMessage    overHeadMsgReq;
  GfastConfig           gfastConfig;
  GfastRFIconfig        gFastRFIConfig;
  DtaConfig             dtaConfig;
  DtaRequest            dtaReq;
  LineCounters          lineCounters;
  uint32                lastRead;
  OverheadClient        overheadClient;
  HmiBufferMgr          hmiEocBuf;
  Bool                  taskToBeProcessed;
  HmiPeriodCounters     prev15MinCounters;
  uint32                eventTimeLogTimeStamp[EVENT_TIME_LOG_LENGTH];
};

LineHpiService LineGetTrafficCounter;
LineHpiService LineGetTrafficPeriodCounter;
LineHpiService LineGetTrafficConfig;
LineHpiService LineSetTestConfig;
LineHpiService LineGoToL3;
LineHpiService LineGetSignalMeasurement;
LineHpiService StartSignalGenerationAndMeasurement;

#define lineMgrStatus(me)   (me->lineStatus)
void LineMgr_init(void);
void LineMgr_changeState(unsigned char lineId, int state);
void LineMgr_LogShowtimeEventTimeStamp(unsigned char lineId, ShowtimeEvent event);
LineMgr* getLineMgr(int line);

#endif
