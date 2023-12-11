
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

#ifdef VECTORING
#undef VECTORING
#endif
#ifdef GFAST_SUPPORT
#undef GFAST_SUPPORT
#endif
#ifdef ATU_R
#undef ATU_R
#endif

#include <linux/string.h> /* for memset() */
#ifndef LINEMGR_H
#include "LineMgr.h"
#endif
#ifndef COREMESSAGEDEF_H
#include "CoreMessageDef.h"
#endif
#include "Overhead.h"
#include "DiagDef.h"
#include "bcm_DrvXface.h"
#include "DslCommonDef.h"

extern int printk(const char *fmt, ...);
#ifdef ADSLDRV_LITTLE_ENDIAN
extern typeLayout GfastRFIconfigLayout[];
extern typeLayout GfastConfigLayout[];
extern typeLayout DtaConfigLayout[];
extern typeLayout DtaRequestLayout[];
extern typeLayout LineIntroduceErrorsMsgReqLayout[];
extern typeLayout SetLineLogicalFrameConfigMsgReqLayout[];
#endif
extern void *releaseRequestAllocateReply(void *requestPtr, uint16 size);
extern void XdslDrvHpiPost(void *replyPtr, uint16 usedSize, int result);
extern void lineIRQ_setStatus(uint32 lineId, uint32 newAlarms);

static int GetSelectedProtol(int modType, int xdslMode);
static uint8 GetSelectedProfile(unsigned int vdsl2Profile);

#define hpi_post  XdslDrvHpiPost

#define INFINITE_MAX_MARGIN_HMI ((int16)0xffff)
#define INFINITE_MAX_MARGIN_FW  (62*256)
#define FIX88_TO_DEC31(x) ((((int32)(x))*10+128)>>8)

void GfastConfig_convertToHMIformat(GfastConfig *lConf)
{

  PhysicalLayerConfigGfast *phyConfig = &lConf->phyConfig;
  LinkConfig *linkConfp = &lConf->phyConfig.linkConfig[0];
  RmcConfig  *rmcConfig = &lConf->rmcConfig[0];
  int dir;

  phyConfig->maxRxPowerUp          = FIX88_TO_DEC31(phyConfig->maxRxPowerUp);
  phyConfig->forceElectricalLength = FIX88_TO_DEC31(phyConfig->forceElectricalLength);

  /* convert fix8_8 to dec3_1 (used on itf) */
  for (dir=0;dir<2;dir++,linkConfp++, rmcConfig++)
  {
    /* convert dec3_1 (used on itf) toward fix8_8 (used internally) */
    linkConfp->targetMarginDB       = FIX88_TO_DEC31(linkConfp->targetMarginDB);
    if (linkConfp->maxMarginDB == INFINITE_MAX_MARGIN_FW)
      linkConfp->maxMarginDB        = INFINITE_MAX_MARGIN_HMI;
    else
      linkConfp->maxMarginDB        = FIX88_TO_DEC31(linkConfp->maxMarginDB);
    linkConfp->upShiftNoiseMargin   = FIX88_TO_DEC31(linkConfp->upShiftNoiseMargin);
    linkConfp->downShiftNoiseMargin = FIX88_TO_DEC31(linkConfp->downShiftNoiseMargin);
    linkConfp->minMarginDB          = FIX88_TO_DEC31(linkConfp->minMarginDB);
    linkConfp->maxTxPower           = FIX88_TO_DEC31(linkConfp->maxTxPower);
    rmcConfig->targetMarginRmc      = FIX88_TO_DEC31(rmcConfig->targetMarginRmc);
    rmcConfig->minMarginRmc         = FIX88_TO_DEC31(rmcConfig->minMarginRmc);
  }
}

uint16 bswap16(const uint16 arg)
{
  return ((arg<<8) | (arg>>8));
}

/***************************************************************************/
/*****************     local FUNCTIONS declaration   ***********************/
/***************************************************************************/

static LineHpiService LineGetPhyLayerConfig;
static LineHpiService LineGetTestConfig;
static LineHpiService LineGetStatus;
static LineHpiService LineGetFeatures;
static LineHpiService LineGetPtmCounters;
static LineHpiService LineGetConfigGFAST;
static LineHpiService LineSetGfastRFIconfig;
static LineHpiService LineGetGfastRFIconfig;
static LineHpiService LineChangeMds;
static LineHpiService LineSetDtaConfig;
static LineHpiService LineGetDtaConfig;
static LineHpiService LineUpdateTestParams;
static LineHpiService LineGetBandPlanVDSL;
static LineHpiService LineGetOamVector;
static LineHpiService LineGetPsdVDSL;
static LineHpiService LineSendOverheadMessage;
static LineHpiService LineGetOverheadMessage;
static LineHpiService LineGetOverheadStatus;
static LineHpiService LineIntroduceErrors;
static LineHpiService LineGetCPEvendorInfo;
static LineHpiService LineGetExtraConfig;

static LineHpiService LineGetCarrierGains;
static LineHpiService LineGetBitAllocation;
static LineHpiService LineGetSnr;
static LineHpiService LineGetRmcBitAlloc;

static LineHpiService LineSetTrafficConfig;
static LineHpiService LineSetExtraConfig;
static LineHpiService LineSetPhyLayerConfig;
static LineHpiService LineStart;
static LineHpiService LineStop;
static LineHpiService LineSetConfigGFAST;
static LineHpiService LineSetLogicalFrameConfig;

#define SERVICE_SIZE  0x82

#ifndef BCM63158_SRC
#define BCM63158_SRC
#endif
//default phyConfigGfast
#define LIMIT_MASK_G9700 \
  {     \
    5, {0,0,0}, \
    {{43,-660},{579,-660},{580,-740},{2047,-770},{4095,-800}} \
  }

#define START_TONE_DS   43
#define STOP_TONE_DS    4095

#define START_TONE_US   43
#define STOP_TONE_US    4095

#define TG_6500_NS      1378 //  6.5 mus

#ifdef GFAST_EC_TA
#define TG_1_PRIME_INIT 0
#else
#define TG_1_PRIME_INIT TG_6500_NS
#endif

#define DEC31_TO_FIX88(x) ((((int32)(x))*6554+128)>>8)

#define targetMarginDBDs  (6 << 8)                  /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define targetMarginDBUs  (6<<8)                    /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define maxMarginDB       INFINITE_MAX_MARGIN_FW    /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define minMarginDB       (0<<8)                    /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define rateAdaptationFlags  2                     // /*Bitmap: bit 0 = Rate Adaptation through Retrain.
                                                    //     bit 1 = SRA
                                                    //    bit 2 = SOS (SRA bit must be enabled as well) */
#define phyRdeltaMarginDB    0                     //   /* unit is (0.1 dB) on itf but internally 1/256 dB.
                                                    //   if phyR is enabled, targetMargin is lowered by this amount */
#define downShiftNoiseMarginDelta  (3<<8)          /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define downShiftNoiseMarginDs  (targetMarginDBDs-downShiftNoiseMarginDelta)          /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define downShiftNoiseMarginUs  (targetMarginDBUs-downShiftNoiseMarginDelta)          /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define minimumDownshiftRAInterval  20
#define upShiftNoiseMarginDelta  (3<<8)            /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define upShiftNoiseMarginDs  (targetMarginDBDs+upShiftNoiseMarginDelta)          /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define upShiftNoiseMarginUs  (targetMarginDBUs+upShiftNoiseMarginDelta)          /* unit is (0.1 dB) on itf but internally 1/256 dB */
#define minimumUpshiftRAInterval  20
#define maxTxPower               (8<<8)             // /* Rx Pwr in US, Tx Pwr in DS. Unit is (0.1 dBm) on itf but internally 1/256 dBm */
#define riTimeThreshold           10                // uint8 riTimeThreshold;  /* retrain time in case of high BER event - range 5-31 sec*/
#define losPersistency            5                 // uint8 losPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 1-20 */
#define lomPersistency            2                 // uint8 lomPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 2-20 */
#define lorPer                    2                 // uint8 lorPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 1-20 */
#define lowETRthreshold           0                 // uint8 lowETRthreshold;  /* allowed time with ETR below ETRmin after FRA (range 1-30 sec, 0 disables the condition) */
#define lowANDEFTRthreshold       0                 // uint32 lowANDEFTRthreshold;  /* unit is 1kbps (range 0- NDRmax, 0 disables the condition) */
#define targetMarginRmc           (12<<8)             //  fix8_8 targetMarginRmc;
#define minMarginRmc              (6<<8)              //  fix8_8 minMarginRmc;    // trigger for RPA
#define maxBi                     4                 //  uint8  maxBi;
#ifdef ATU_C
#define RMCRlorPersistencyTrigDS  2                 // unit is 50 ms; if 0 the use of RMCR procedure is not allowed range 1-20
#define RMCRlorPersistencyTrigUS  2                 // unit is 50 ms; if 0 the use of RMCR procedure is not allowed range 1-20
#else
#define RMCRlorPersistencyTrigDS  0                 // unit is 50 ms; if 0 the use of RMCR procedure is not allowed range 1-20
#define RMCRlorPersistencyTrigUS  0                 // unit is 50 ms; if 0 the use of RMCR procedure is not allowed range 1-20
#endif
#ifdef GFAST_EC_ENABLE_IN_SYSTEM
#define FRA_TIME                  0                 //uint8  FRA_TIME;      // monitoring time  range 1 - Msf
#else
#define FRA_TIME                  8                 //uint8  FRA_TIME;      // monitoring time  range 1 - Msf
#endif
#define FRA_NTONES                50                //uint8  FRA_NTONES;    // % degraded tones range 1-100
#define FRA_RTX_UC                150               //uint16 FRA_RTX_UC;    // rtx_uc anomalies range 1-1023
#define FRA_VENDISC                1                //uint8  FRA_VENDISC;   // enables vendor disc method
#ifdef GFAST_EC_ENABLE_IN_SYSTEM
#define RTX_TEST_MODE   1
#else
#define RTX_TEST_MODE   0
#endif


GfastConfigInternal gfastConfigInternalDefault =
{
    /* PhysicalLayerConfigGfast */
    {
        LIMIT_MASK_G9700,     //PsdDescriptorDn                limitingMaskDn;
        LIMIT_MASK_G9700,     //PsdDescriptorUp                limitingMaskUp;
        { /* bandPlanDn */    //BandsDescriptor            bandPlanDn;
            1,0,{{STOP_TONE_DS,START_TONE_DS}}
        },
        { /* bandPlanUp */   //BandsDescriptor            bandPlanUp;
            1,0,{{STOP_TONE_US,START_TONE_US}}
        },
        { /* rfi bands */    //RfiBands                       rfiBands;
#if 0  //use rfi
            2,0, {  {200, 180}, {840, 800}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}    }
#else
            0,0, {  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}    }
#endif
        },
        { /* carrier mask DS bands */    //RfiBands                       carMaskDn;
            0,0, {  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}    }
        },
        { /* carrier mask US bands */    //RfiBands                       carMaskUp;
            0,0, {  {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}    }
        },
        /* IAR bands*/  /* 12 bits bitmask following coding as in G998.4 table 11.70.8 */
#ifndef BCM63158_SRC   //use IAR
        0x70,
#else
        0x0,
#endif
        { /* usPboDescriptor */            //VdslMsgReferencePsdDescriptor  usPboDescriptor;
            1,0, { {-40*256,0,0}, {0,0,0},{0,0,0},{0,0,0} }
        },
        BCM_GFAST_PROFILE_MASK_106a|BCM_GFAST_PROFILE_MASK_106b|BCM_GFAST_PROFILE_MASK_212a,                //uint8          enableProfileBitmap;    /* bit0 = 106a, bit1 = 212a, bit2 = 106b, bit3 = 106c, bit4 = 212c */
        BCM_MGFAST_PROFILE_MASK_P424a|BCM_MGFAST_PROFILE_MASK_P424d,                                 //uint8          enableProfileBitmapMgfast;
#ifndef GFAST_EC_SUPPORT        
        DEC31_TO_FIX88(-1),                                //int16          forceElectricalLength;
#else
        DEC31_TO_FIX88(-4),                                //int16          forceElectricalLength;
#endif
                                                            /*
                                                             >=0:  use forced Electrical Length
                                                             -4:   use cpe elen
                                                             -3:   use co  elen
                                                             -2:   use min(co_elen,cpe_elen)
                                                             -1:   use max(co_elen,cpe_elen) (default)
                                                            */
        1,                                 //uint8          retrainAllowed
                                                             /*
                                                               0: Not retrain allowed for PSD optimization
                                                               1: Retrain allow in O-PRM/R-PRM for PSD optimization
                                                               2: Force retrain in O-PRM/R-PRM for debug
                                                               3: Retrain allow in O-PRM/R-PRM for PSD optimization, but only if CPE supports extended timeouts
                                                             */
        0,                                 //uint8          reserved
        0,                                 //int16          maxRxPowerUp;           /* unit is (0.1 dB) on itf but internally 1/256 dB */
        0,                                 //uint16         maxToneUsed;            /* GFAST US and DS bandplans will be clipped according to this tone -  active when maxUsedTone>0 */
        TG_1_PRIME_INIT,                   //int16          initialTgPrime;         /* initial value of Tg1' that the CPE should install, -1 to let the FW decide */
        -1, // GFAST_NSYM_DS,                     //int8           m_ds;                   /* nr of active frames in the DS TDD slot [2..M_DS], -1 means fill all available space */
        -1, // GFAST_NSYM_TDD- GFAST_NSYM_DS-1,   //int8           m_us;                   /* nr of active frames in the US TDD slot [2..M_US], -1 means fill all available space, but is not implemented */
        1,                                 //uint8          soc_redundancy;         /* DS soc repetion rate in ChannelDiscovery 0 = minimal/1 = max (m_ds/4) */
        {0,0,0},          /* reservedA[3]             */
        /*  LinkConfig                     linkConfig[2]; */
        {
            {
                targetMarginDBUs,           //int16  targetMarginDB; /* unit is (0.1 dB) on itf but internally 1/256 dB */
                maxMarginDB,                //int16  maxMarginDB;    /* unit is (0.1 dB) on itf but internally 1/256 dB */
                minMarginDB,                //int16  minMarginDB;    /* unit is (0.1 dB) on itf but internally 1/256 dB */
                rateAdaptationFlags,        //int8   rateAdaptationFlags;
                phyRdeltaMarginDB,          //uint8  phyRdeltaMarginDB; /* uint is (0.1 dB) on interface but internally 1/256 dB.
                downShiftNoiseMarginUs,     //int16  downShiftNoiseMargin; /* unit is (0.1 dB)  on itf but internally 1/256 dB */
                minimumDownshiftRAInterval, //uint16 minimumDownshiftRAInterval;
                upShiftNoiseMarginUs,       //int16  upShiftNoiseMargin; /* unit is (0.1 dB)  on itf but internally
                                            //                          1/256 dB */
                minimumUpshiftRAInterval,   //uint16 minimumUpshiftRAInterval;
                maxTxPower                  //int16  maxTxPower;      /* Rx Pwr in US, Tx Pwr in DS. Unit is (0.1 dBm) on itf but internally 1/256 dBm */,
            },
            {
                targetMarginDBDs,           //int16  targetMarginDB; /* unit is (0.1 dB) on itf but internally 1/256 dB */
                maxMarginDB,                //int16  maxMarginDB;    /* unit is (0.1 dB) on itf but internally 1/256 dB */
                minMarginDB,                //int16  minMarginDB;    /* unit is (0.1 dB) on itf but internally 1/256 dB */
                rateAdaptationFlags,        //int8   rateAdaptationFlags;
                phyRdeltaMarginDB,          //uint8  phyRdeltaMarginDB; /* uint is (0.1 dB) on interface but internally 1/256 dB.
                downShiftNoiseMarginDs,     //int16  downShiftNoiseMargin; /* unit is (0.1 dB)  on itf but internally 1/256 dB */
                minimumDownshiftRAInterval, //uint16 minimumDownshiftRAInterval;
                upShiftNoiseMarginDs,       //int16  upShiftNoiseMargin; /* unit is (0.1 dB)  on itf but internally
                                            //                          1/256 dB */
                minimumUpshiftRAInterval,   //uint16 minimumUpshiftRAInterval;
                maxTxPower                  //int16  maxTxPower;      /* Rx Pwr in US, Tx Pwr in DS. Unit is (0.1 dBm) on itf but internally 1/256 dBm */,
            }
        },
        {0,0,0,0},                         // int8  reservedB[4];
#ifdef ATU_R
        {0, 0},                            // uint8 rmcrConfig[2];     /* Unit is 50 ms, 0 means RMC-R is disabled */
#else
       {RMCRlorPersistencyTrigUS, RMCRlorPersistencyTrigDS},                            // uint8 rmcrConfig[2];     /* Unit is 50 ms, 0 means RMC-R is disabled */
#endif
        /*  ReinitConfig                   reinitConfig[2]; */
        {
            {
                0,                          // uint8 riPolicy;         /* reserved for G.fast */
                riTimeThreshold,            // uint8 riTimeThreshold;  /* retrain time in case of high BER event - range 5-31 sec*/
                losPersistency,             // uint8 losPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 1-20 */
                lomPersistency,             // uint8 lomPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 2-20 */
                lorPer,                     // uint8 lomPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 1-20 */
                lowETRthreshold,            // uint8 lowETRthreshold;  /* allowed time with ETR below ETRmin after FRA (range 1-30 sec, 0 disables the condition) */
                {0,0},                      //reserved[2];
                lowANDEFTRthreshold,        // uint32 lowANDEFTRthreshold;  /* unit is 1kbps (range 0- NDRmax, 0 disables the condition) */
            },
            {
                0,                          // uint8 riPolicy;         /* reserved for G.fast */
                riTimeThreshold,            // uint8 riTimeThreshold;  /* retrain time in case of high BER event - range 5-31 sec*/
                losPersistency,             // uint8 losPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 1-20 */
                lomPersistency,             // uint8 lomPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 2-20 */
                lorPer,                     // uint8 lomPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 1-20 */
                lowETRthreshold,            // uint8 lowETRthreshold;  /* allowed time with ETR below ETRmin after FRA (range 1-30 sec, 0 disables the condition) */
                {0,0},                      //reserved[2];
                lowANDEFTRthreshold,        // uint32 lowANDEFTRthreshold;  /* unit is 1kbps (range 0- NDRmax, 0 disables the condition) */
            },
        },
    },
    /* TrafficConfigGfast */
    {
        {
            2000000,                     //uint32  NDRmax;         // only useful value...
            96,                          //uint32  ETRmin;         // Used to abort init if it can't be achieved
            0,                           //uint32  GDRmax;         // Gamma Data Rate
            0,                           //uint32  GDRmin;         //
            10000,                       //uint16  maxDelay;       // unit is us - range 0-63000 (63ms)
            10,                           //uint16  INPmin;         // unit is 1/2 symbol - range 0-1040 by step of 2
            0,                           //uint8   INPmin_rein;    // unit is 1/2 symbol - range 0-126 by step of 2
            0,                           //uint8   shineRatio;     // Assumed fraction of NDR necessary to correct shine - range 0-100 (0.1).
            0,                           //uint8   iatRein;        // 0-3 for [100, 120, 300, 360] Hz
            0,                           //uint8   minRSoverhead;  // minimum R/N. Unit is 1/256th - range 0-64 by step of 8
            RTX_TEST_MODE,               //uint8   rtxTestMode;    // accelerated MTBE testing
            1,                           //uint8   tpsTcTestMode;  // disables dummy DTU generation
            0,                           //uint8   enableIdleSymbol
            0,                           //uint8   disableBackpressure
            {0,0,0,0},                   //uint8   reserved[4];
        },
        {
            2000000,                     //uint32  NDRmax;         // only useful value...
            96,                          //uint32  ETRmin;         // Used to abort init if it can't be achieved
            0,                           //uint32  GDRmax;         // Gamma Data Rate
            0,                           //uint32  GDRmin;         //
            10000,                       //uint16  maxDelay;       // unit is us - range 0-63000 (63ms)
            10,                           //uint16  INPmin;         // unit is 1/2 symbol - range 0-1040 by step of 2
            0,                           //uint8   INPmin_rein;    // unit is 1/2 symbol - range 0-126 by step of 2
            0,                           //uint8   shineRatio;     // Assumed fraction of NDR necessary to correct shine - range 0-100 (0.1).
            0,                           //uint8   iatRein;        // 0-3 for [100, 120, 300, 360] Hz
            0,                           //uint8   minRSoverhead;  // minimum R/N. Unit is 1/256th - range 0-64 by step of 8
            RTX_TEST_MODE,               //uint8   rtxTestMode;    // accelerated MTBE testing
            1,                           //uint8   tpsTcTestMode;  // disables dummy DTU generation
            0,                           //uint8   enableIdleSymbol
            0,                           //uint8   disableBackpressure
            {0,0,0,0},                   //uint8   reserved[4];
        }
    },
    /* RmcConfig */
    {
        {
            targetMarginRmc,                //  fix8_8 targetMarginRmc;
            minMarginRmc,                   //  fix8_8 minMarginRmc;    // trigger for RPA
            maxBi,                          //  uint8  maxBi;
            0,                              // uint8 minimum startTone for the RMC alloc - encoded as real tone index divided by 8
        },
        {
            targetMarginRmc,                //  fix8_8 targetMarginRmc;
            minMarginRmc,                   //  fix8_8 minMarginRmc;    // trigger for RPA
            maxBi,                          //  uint8  maxBi;
            0,                              // uint8 minimum startTone for the RMC alloc - encoded as real tone index divided by 8
        }
    },
    /* FraConfig */
    {
        {
            FRA_TIME,                       //uint8  FRA_TIME;      // monitoring time  range 1 - Msf
            FRA_NTONES,                     //uint8  FRA_NTONES;    // % degraded tones range 1-100
            FRA_RTX_UC,                     //uint16 FRA_RTX_UC;    // rtx_uc anomalies range 1-1023
            FRA_VENDISC,                    //uint8  FRA_VENDISC;   // enables vendor disc method
            0,                              //uint8  reserved;
        },
        {
            FRA_TIME,                       //uint8  FRA_TIME;      // monitoring time  range 1 - Msf
            FRA_NTONES,                     //uint8  FRA_NTONES;    // % degraded tones range 1-100
            FRA_RTX_UC,                     //uint16 FRA_RTX_UC;    // rtx_uc anomalies range 1-1023
            FRA_VENDISC,                    //uint8  FRA_VENDISC;   // enables vendor disc method
            0,                              //uint8  reserved;
        }
    }
};

LineMgr               gLineMgr[LINES_PER_DEV];

static void* DummyMsgHandler(LineMgr *me, void *msgp, uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  *resultp = HPI_SUCCESS;
  return 0;
}

static LineHpiServiceInfo  services[SERVICE_SIZE] =
{
  { /*  0 */ NULL,                                0,                                0,                                   0},
  { /*  1 */ NULL, /* line reset -- no generic function */   sizeof(uint32),        0,                                   ALL_STATES},
  { /*  2 */ LineSetTrafficConfig,                sizeof(LineTrafficConfig),        MIN_IS_MAX,                          IDLE_STATES},
  { /*  3 */ LineGetTrafficConfig,                0,                                0,                                   ~LINE_TEST_STATE},
  { /*  4 */ LineSetPhyLayerConfig,               sizeof(PhysicalLayerConfig),      MIN(254,sizeof(PhysicalLayerConfig)),IDLE_STATES},
  { /*  5 */ LineGetPhyLayerConfig,               0,                                0,                                   ~LINE_TEST_STATE},
  { /*  6 */ LineSetTestConfig,                   sizeof(TestConfigMessage),        MIN_IS_MAX,                          IDLE_STATES | RUNNING_STATES},
  { /*  7 */ LineGetTestConfig,                   0,                                0,                                   ~LINE_TEST_STATE},
  { /*  8 */ LineStart,                           0,                                0,                                   LINE_IDLE_CONFIGURED | RUNNING_STATES},
  { /*  9 */ LineStop,                            0,                                0,                                   ALL_STATES},
  { /*  A */ LineGetTrafficCounter,               0,                                0,                                   LINE_IDLE_CONFIGURED | RUNNING_STATES},
  { /*  B */ LineGetStatus,                       sizeof(uint32),                   0,                                   ALL_STATES,},
  { /*  C */ LineGetFeatures,                     0,                                0,                                   ALL_STATES,},
  { /*  D */ DummyMsgHandler,                     0,                                0,                                   IDLE_STATES},
  { /*  E */ NULL,                                0,                                0,                                   0,},
  { /*  F */ NULL,                                0,                                0,                                   0,},
  { /* 10 */ NULL,                                0,                                0,                                   0,},
  { /* 11 */ NULL,                                0,                                0,                                   0,},
  { /* 12 */ NULL,                                0,                                0,                                   0,},
  { /* 13 */ NULL,                                0,                                0,                                   0,},
  { /* 14 */ NULL,                                0,                                0,                                   0,},
  { /* 15 */ NULL,                                0,                                0,                                   0,},
  { /* 16 */ NULL,                                0,                                0,                                   0,},
  { /* 17 */ NULL,                                0,                                0,                                   0,},
  { /* 18 */ LineGetBandPlanVDSL,                 0,                                0,                                   LINE_SHOWTIME|LINE_IDLE_DM_COMPLETE|LINE_TEST_STATE},
  { /* 19 */ NULL,                                0,                                0,                                   0,},
  { /* 1A */ LineGetOamVector,                    5,                                MIN_IS_MAX,                          LINE_SHOWTIME|LINE_IDLE_DM_COMPLETE},
  { /* 1B */ LineGetPtmCounters,                  0,                                0,                                   ALL_STATES},
  { /* 1C */ LineGetPsdVDSL,                      0,                                0,                                   LINE_SHOWTIME|LINE_IDLE_DM_COMPLETE|LINE_TEST_STATE},
  { /* 1D */ NULL,                                0,                                0,                                   0,},
  { /* 1E */ LineSetConfigGFAST,                  sizeof(GfastConfig),           MIN_IS_MAX,                          IDLE_STATES},
  { /* 1F */ LineGetConfigGFAST,                  0,                                0,                                   ~LINE_TEST_STATE},
  { /* 20 */ NULL,                                0,                                0,                                   0},
  { /* 21 */ NULL,                                0,                                0,                                   0},
  { /* 22 */ LineSetExtraConfig,                  sizeof(ExtraConfig),              2*sizeof(GinpConfig),                IDLE_STATES},
  { /* 23 */ LineGetExtraConfig,                  0,                                0,                                   ~LINE_TEST_STATE},
  { /* 24 */ NULL,                                0,                                0,                                   0},
  { /* 25 */ NULL,                                0,                                0,                                   0},
  { /* 26 */ NULL,                                0,                                0,                                   0},
  { /* 27 */ NULL,                                0,                                0,                                   0},
  { /* 28 */ NULL,                                0,                                0,                                   0},
  { /* 29 */ NULL,                                0,                                0,                                   0},
  { /* 2A */ NULL,                                0,                                0,                                   0},
  { /* 2B */ NULL,                                0,                                0,                                   0},
  { /* 2C */ NULL,                                0,                                0,                                   0},
  { /* 2D */ NULL,                                0,                                0,                                   0},
  { /* 2E */ NULL,                                0,                                0,                                   0},
  { /* 2F */ LineGetBitAllocation,                sizeof(BSGrequest),               MIN_IS_MAX,                          LINE_SHOWTIME},
  { /* 30 */ NULL,                                0,                                0,                                   0},
  { /* 31 */ LineGetTrafficPeriodCounter,         sizeof(PeriodIdentification),     MIN_IS_MAX,                          ALL_STATES},
  { /* 32 */ NULL,                                0,                                0,                                   0 },
  { /* 33 */ LineGetCPEvendorInfo,                0,                                0,                                   ALL_STATES},
  { /* 34 */ NULL,                                0,                                0,                                   0 },
  { /* 35 */ NULL,                                0,                                0,                                   0},
  { /* 36 */ NULL,                                0,                                0,                                   0 },
  { /* 37 */ LineGetSnr,                          sizeof(BSGrequest),               MIN_IS_MAX,                          LINE_SHOWTIME|LINE_IDLE_DM_COMPLETE},
  { /* 38 */ LineSendOverheadMessage,             sizeof(HmiOverheadMessage),       MIN(254,sizeof(HmiOverheadMessage)), LINE_SHOWTIME},
  { /* 39 */ LineGetOverheadMessage,              sizeof(uint8),                    0,                                   LINE_SHOWTIME},
  { /* 3A */ LineGetOverheadStatus,               0,                                0,                                   LINE_SHOWTIME},
  { /* 3B */ NULL,                                0,                                0,                                   0 },
  { /* 3C */ NULL,                                0,                                0,                                   0 },
  { /* 3D */ NULL,                                0,                                0,                                   0 },
  { /* 3E */ NULL,                                0,                                0,                                   0 },
  { /* 3F */ NULL,                                0,                                0,                                   0 },
  { /* 40 */ LineUpdateTestParams,                sizeof(uint8),                    sizeof(uint8),                       LINE_SHOWTIME},
  { /* 41 */ NULL,                                0,                                0,                                   0 },
  { /* 42 */ NULL,                                0,                                0,                                   0 },
  { /* 43 */ LineGetCarrierGains,                 sizeof(BSGrequest),               MIN_IS_MAX,                          LINE_SHOWTIME},
  { /* 44 */ NULL,                                0,                                0,                                   0 },
  { /* 45 */ LineIntroduceErrors,                 sizeof(IntroduceErrorsRequest),   MIN_IS_MAX,                         LINE_SHOWTIME },
  { /* 46 */ NULL,                                0,                                0,                                   0 },
  { /* 47 */ NULL,                                0,                                0,                                   0 },
  { /* 48 */ NULL,                                0,                                0,                                   0 },
  { /* 49 */ NULL,                                0,                                0,                                   0 },
  { /* 4A */ NULL,                                0,                                0,                                   0 },
  { /* 4B */ NULL,                                0,                                0,                                   0 },
  { /* 4C */ NULL,                                0,                                0,                                   0 },
  { /* 4D */ NULL,                                0,                                0,                                   0 },
  { /* 4E */ NULL,                                0,                                0,                                   0 },
  { /* 4F */ NULL,                                0,                                0,                                   0 },
  { /* 50 */ NULL,                                0,                                0,                                   0 },
  { /* 51 */ NULL,                                0,                                0,                                   0 },
  { /* 52 */ NULL,                                0,                                0,                                   0 },
  { /* 53 */ DummyMsgHandler,                     0,                                0,                                   LINE_TEST_STATE},
  { /* 54 */ DummyMsgHandler,                     0,                                0,                                   LINE_SHOWTIME},
  { /* 55 */ DummyMsgHandler,                     0,                                0,                                   ALL_STATES},
  { /* 56 */ NULL,                                0,                                0,                                   0 },
  { /* 57 */ DummyMsgHandler,                     0,                                0,                                   0 },
  { /* 58 */ DummyMsgHandler,                     0,                                0,                                   ALL_STATES},
  { /* 59 */ DummyMsgHandler,                     0,                                0,                                   ALL_STATES},
  { /* 5A */ NULL,                                0,                                0,                                   0 },
  { /* 5B */ NULL,                                0,                                0,                                   0 },
  { /* 5C */ NULL,                                0,                                0,                                   0 },
  { /* 5D */ NULL,                                0,                                0,                                   0 },
  { /* 5E */ NULL,                                0,                                0,                                   0 },
  { /* 5F */ NULL,                                0,                                0,                                   0 },
  { /* 60 */ NULL,                                0,                                0,                                   0 },
  { /* 61 */ NULL,                                0,                                0,                                   0 },
  { /* 62 */ NULL,                                0,                                0,                                   0 },
  { /* 63 */ NULL,                                0,                                0,                                   0 },
  { /* 64 */ NULL,                                0,                                0,                                   0 },
  { /* 65 */ NULL,                                0,                                0,                                   0 },
  { /* 66 */ NULL,                                0,                                0,                                   0 },
  { /* 67 */ NULL,                                0,                                0,                                   0 },
  { /* 68 */ NULL,                                0,                                0,                                   0},
  { /* 69 */ NULL,                                0,                                0,                                   0},
  { /* 6A */ NULL,                                0,                                0,                                   0},
  { /* 6B */ NULL,                                0,                                0,                                   0},
  { /* 6C */ NULL,                                0,                                0,                                   0},
  { /* 6D */ NULL,                                0,                                0,                                   0,},
  { /* 6E */ NULL,                                0,                                0,                                   0,},
  { /* 6F */ DummyMsgHandler,                     0,                                0,                                   ALL_STATES},
  { /* 70 */ DummyMsgHandler,                     0,                                0,                                   ALL_STATES},
  { /* 71 */ NULL,                                0,                                0,                                   0,},
  { /* 72 */ DummyMsgHandler,                     0,                                0,                                   ALL_STATES},
  { /* 73 */ NULL,                                0,                                0,                                   0,},
  { /* 74 */ LineGetRmcBitAlloc,                  sizeof(BSGrequest),               MIN_IS_MAX,                          LINE_SHOWTIME},
  { /* 75 */ NULL,                                0,                                0,                                   0,},
  { /* 76 */ NULL,                                0,                                0,                                   0,},
  { /* 77 */ NULL,                                0,                                0,                                   0,},
  { /* 78 */ NULL,                                0,                                0,                                   0,},
  { /* 79 */ NULL,                                0,                                0,                                   0,},
  { /* 7A */ LineSetLogicalFrameConfig,           sizeof(LogicalFrameCfg),          MIN_IS_MAX,                          ALL_STATES},
  { /* 7B */ LineSetGfastRFIconfig,               sizeof(GfastRFIconfig),           MIN_IS_MAX,                          IDLE_STATES},
  { /* 7C */ LineGetGfastRFIconfig,               0,                                0,                                  ~LINE_TEST_STATE},
  { /* 7D */ NULL,                                0,                                0,                                   0,},
  { /* 7E */ LineChangeMds,                       sizeof(DtaRequest),               MIN_IS_MAX,                          ALL_STATES},
  { /* 7F */ LineSetDtaConfig,                    sizeof(DtaConfig),                MIN_IS_MAX,                          ALL_STATES},
  { /* 80 */ LineGetDtaConfig,                    0,                                MIN_IS_MAX,                          ALL_STATES},
  { /* 81 */ NULL,                                0,                                0,                                   0,},
};


/***************************************************************************/
/********************     FUNCTIONS definitions   **************************/
/***************************************************************************/

// BitsGainsSnrReporting.c
static void DiagPrintBSGrequest(BSGrequest *req)
{
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "downstream=%d partId=%d startTone=%d nTones=%d log2M=%d includeRi=%d isDoi=%d isBackupRmc=%d",
    req->downstream, req->partId,
    req->startTone, req->nTones,
    req->log2M, req->includeRi,
    req->isDoi, req->isBackupRmc);
}

static void clampToZero(int8 *array, int length)
{
  int i;
  for(i=0; i<length; i++)
    array[i] = (array[i] < 0 ? 0 : array[i]);
}

void* LineGetSnr(LineMgr *me,
                 void    *msgp,
                 uint16  *sizep,
                 int     *resultp,
                 uint16   hmiVersion)
{
  LineGetSnr_MsgRep *rep;
  BSGrequest        *reqData   = (BSGrequest *)msgp;
  Bool              marginRequest, varianceFlag;
  int               i, startTone, endTone, maxToneNumber, sliceLen = BGS_BUFF_LEN;
  fix8_8            *pSnr, *pRepSnr;
  adslMibInfo       *pMibInfo;
  char              oidStr[] = { kOidAdslPrivate, kOidAdslPrivSNR};
  long              dataLen = 0;
  int               virtualToneOffset = 0;
  
  pSnr  = (fix8_8 *) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);

  DiagPrintBSGrequest(reqData);
  
  marginRequest = (reqData->downstream & 0x40) ? 1: 0;
  varianceFlag  = (reqData->downstream & 0x80) ? 1: 0;
  reqData->downstream &= ~0xc0; /* clear margin and variance flags */
  
  if(marginRequest || varianceFlag) {
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetSnr: Need to reply to marginRequest=%d, varianceFlag=%d", marginRequest, varianceFlag);
    *resultp = HPI_NOT_POSSIBLE_IN_CURRENT_STATE;
    return NULL;
  }
  
  maxToneNumber = dataLen >> 2;
  dataLen   = sizeof(adslMibInfo);
  pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &dataLen);
  
  if(reqData->downstream == DS_DIRECTION) {
    pSnr += maxToneNumber;  /* FE data */
    virtualToneOffset = pMibInfo->dsNegBandPlan.toneGroups[0].startTone;
  }
  else
    virtualToneOffset = pMibInfo->usNegBandPlan.toneGroups[0].startTone;

  if (reqData->startTone==0 && reqData->nTones==0)
    startTone = virtualToneOffset + reqData->partId*BGS_BUFF_LEN;
  else {
    startTone = reqData->startTone;
    sliceLen  = reqData->nTones;
  }
  
  if( (startTone >= maxToneNumber) || (sliceLen > BGS_BUFF_LEN) || (reqData->log2M != 0) ) {
    *resultp = HPI_INVALID_REQUEST_DATA;
    return NULL;
  }
  
  rep = releaseRequestAllocateReply(msgp,sizeof(*rep));
  pRepSnr = rep->data.snr;
  memset(pRepSnr, 0, sizeof rep->data.snr);
  sliceLen = MIN(maxToneNumber-startTone, sliceLen);
  endTone = startTone + sliceLen;

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetSnr: nMaxTone=%d, toneIndex=%d sliceLen=%d", maxToneNumber, startTone, sliceLen);
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "Before: %04x %04x %04x %04x %04x %04x %04x %04x",
    pSnr[startTone+0], pSnr[startTone+1], pSnr[startTone+2], pSnr[startTone+3],
    pSnr[startTone+4], pSnr[startTone+5], pSnr[startTone+6], pSnr[startTone+7]);
  for(i = startTone; i < endTone; i++) {
    if(pSnr[i] > 0)
      *pRepSnr = bswap16((pSnr[i]+8) >> 4);
    pRepSnr++;
  }
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "After:  %04x %04x %04x %04x %04x %04x %04x %04x",
    rep->data.snr[0], rep->data.snr[1], rep->data.snr[2], rep->data.snr[3],
    rep->data.snr[4], rep->data.snr[5], rep->data.snr[6], rep->data.snr[7]);
//  DiagDumpData(0, DIAG_DSL_CLIENT, &rep->data.snr[0], sizeof(*rep), kDiagDbgDataFormatHex | kDiagDbgDataSize16);

  *resultp = HPI_SUCCESS;
  *sizep   = sizeof(*rep);
  return rep;
}

void* LineGetCarrierGains(LineMgr *me,
                          void    *msgp,
                          uint16  *sizep,
                          int     *resultp, uint16 hmiVersion)
{
  LineGetCarrierGains_MsgRep *rep;
  LineGetCarrierGains_MsgReq *req = (LineGetCarrierGains_MsgReq*)msgp;
  BSGrequest *reqData = &req->data;
  int               i, startTone, endTone, maxToneNumber, sliceLen = BGS_BUFF_LEN;
  fix8_8            *pGi, *pRepGi;
  char              oidStr[] = { kOidAdslPrivate, kOidAdslPrivGain};
  long              dataLen = 0;
  adslMibInfo       *pMibInfo;
  long              mibLen;
  int               bandPlanStartTone = 0;

  pGi  = (fix8_8 *) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);
  mibLen    = sizeof(adslMibInfo);
  pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);

  DiagPrintBSGrequest(reqData);
  
  maxToneNumber= dataLen >> 2;
  if(reqData->downstream == DS_DIRECTION) {
    pGi += maxToneNumber;  /* FE data */
    bandPlanStartTone = pMibInfo->dsNegBandPlan.toneGroups[0].startTone;
  }
  else
    bandPlanStartTone = pMibInfo->usNegBandPlan.toneGroups[0].startTone;

  if (reqData->startTone==0 && reqData->nTones==0)
    startTone = bandPlanStartTone + reqData->partId*BGS_BUFF_LEN;
  else {
    startTone = reqData->startTone;
    sliceLen  = reqData->nTones;
  }
  
  if( (startTone >= maxToneNumber) || (sliceLen > BGS_BUFF_LEN) || (reqData->log2M != 0) ) {
    *resultp = HPI_INVALID_REQUEST_DATA;
    return NULL;
  }
  
  rep = releaseRequestAllocateReply(msgp,sizeof(*rep));
  pRepGi = rep->data.gi;
  memset(pRepGi, 0, sizeof rep->data.gi);

  sliceLen = MIN(maxToneNumber-startTone, sliceLen);
  endTone = startTone + sliceLen;
  
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetCarrierGains: nMaxTone=%d, toneIndex=%d sliceLen=%d", maxToneNumber, startTone, sliceLen);
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "Before: %04x %04x %04x %04x %04x %04x %04x %04x",
    pGi[startTone], pGi[startTone+1], pGi[startTone+2], pGi[startTone+3], pGi[startTone+4], pGi[startTone+5], pGi[startTone+6],pGi[startTone+7]);
  for(i = startTone; i < endTone; i++) {
    if(pGi[i])
      *pRepGi = pGi[i] << 4;
    pRepGi++;
  }
  
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "After: %04x %04x %04x %04x %04x %04x %04x %04x",
    rep->data.gi[0], rep->data.gi[1], rep->data.gi[2], rep->data.gi[3],
    rep->data.gi[4], rep->data.gi[5], rep->data.gi[6], rep->data.gi[7]);
  
  *resultp = HPI_SUCCESS;
  *sizep = sizeof(*rep);
  return rep;
}

void* LineGetBitAllocation(LineMgr *me,
                           void    *msgp,
                           uint16  *sizep,
                           int     *resultp,
                           uint16   hmiVersion)
{
  LineGetBitAllocation_MsgRep   *rep;
  BSGrequest                    *req    = (BSGrequest *)msgp;
  int                          direction;
  int                          toneBlock;
  int                          startTone;
  int                          nTones;
  int                          maxToneNumber = 0;
  int                          isDoi;
  
  uint8 *pBitAlloc;
  char  oidStr[] = { kOidAdslPrivate, 0 };
  long  dataLen = 0;
  adslMibInfo         *pMibInfo;
  long                mibLen;
  int                 bandPlanStartTone = 0;
  
  DiagPrintBSGrequest(req);
  if(*sizep != 0)
  {
    direction = req->downstream;
    toneBlock = req->partId;
    startTone = req->startTone;
    nTones    = MIN(req->nTones, BGS_BUFF_LEN * 2);
    isDoi     = (req->isDoi) && XdslDrvIsGfastMod(me->lineId);
  }
  else
  {
    *resultp = HPI_INVALID_REQUEST_DATA;
    return NULL;
  }

  oidStr[1] = isDoi ? kOidAdslPrivDoiBitAlloc: kOidAdslPrivBitAlloc;

  pBitAlloc  = (uint8*) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);

  rep = releaseRequestAllocateReply(msgp,sizeof(*rep));
  memset( rep->data.bitAllocTable, 0, sizeof rep->data.bitAllocTable);
  maxToneNumber= dataLen >> 1;
  
  mibLen    = sizeof(adslMibInfo);
  pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);

  if(direction == DS_DIRECTION) {
    pBitAlloc += maxToneNumber;  /* FE data */
    bandPlanStartTone = pMibInfo->dsNegBandPlan.toneGroups[0].startTone;
  }
  else
    bandPlanStartTone = pMibInfo->usNegBandPlan.toneGroups[0].startTone;
  
  /* if both startTone and nTones are set to 0, reset them based on toneBlock input */
  if ((startTone == 0) && (nTones == 0))
  {
    startTone = bandPlanStartTone + toneBlock * BGS_BUFF_LEN *2;
    nTones    = MIN(maxToneNumber-startTone,BGS_BUFF_LEN *2);
  }

  if (startTone < maxToneNumber)
  {
    memcpy(rep->data.bitAllocTable,&pBitAlloc[startTone], nTones);
  }
  
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetBitAllocation: nMaxTone=%d, toneIndex=%d nTone=%d", maxToneNumber, startTone, nTones);

  clampToZero((int8 *) rep->data.bitAllocTable, sizeof(rep->data.bitAllocTable)); 

  *resultp = HPI_SUCCESS;
  *sizep   = sizeof(*rep);
  return rep;
}

void* LineGetRmcBitAlloc(LineMgr *me,
                         void    *msgp,
                         uint16  *sizep,
                         int     *resultp, uint16 hmiVersion)
{
  BSGrequest                  *req      = (BSGrequest *)msgp;
  LineGetRmcBitAlloc_MsgRep   *rep;
  int                         direction = req->downstream;
  int                         startTone = req->startTone;
  int                         nTones    = MIN(req->nTones, RMC_BUFF_LEN);
  char                        oidStr[]  = { kOidAdslPrivate, 0 };
  long                        dataLen   = 0;
  int                         dataOffset = 0;
  uint8                       *pRmcBitAlloc;
  uint16                      *pRts;

  DiagPrintBSGrequest(req);

  if (req->log2M != 0)
  {
    *resultp = HPI_INVALID_REQUEST_DATA;
    return NULL;
  }

  if (req->partId != -1)
  {
    startTone = (req->partId==0) ? 0 : ((req->partId==1) ? RMC_BUFF_LEN : 2*RMC_BUFF_LEN);
    nTones    = (req->partId==0) ? RMC_BUFF_LEN : ((req->partId==1) ? MIN(MAX_RMC_TONES_HMI-RMC_BUFF_LEN, RMC_BUFF_LEN) : MAX(MAX_RMC_TONES_HMI-2*RMC_BUFF_LEN, 0));
  }

  rep = releaseRequestAllocateReply(msgp,sizeof(*rep));

  oidStr[1] = kOidAdslPrivGetRmcBitAlloc;
  pRmcBitAlloc = (uint8*) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);
  if(direction == DS_DIRECTION) {
    dataOffset = dataLen >> 1;  /* FE data */
  }
  pRmcBitAlloc += dataOffset;
  memcpy(rep->data.bi, &pRmcBitAlloc[startTone], nTones);

  oidStr[1] = kOidAdslPrivGetRts;
  pRts = (uint16*) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);
  pRts += dataOffset;
  memcpy(rep->data.rts, &pRts[startTone], 2*nTones);

  dataLen = sizeof(rep->data.nTonesRmc);
  oidStr[1] = kOidAdslPrivGetnToneRmc;
  dataLen = AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), (char *)&rep->data.nTonesRmc, &dataLen);

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetRmcBitAlloc: nTonesRmc=%d nTones=%d startTone=%d", rep->data.nTonesRmc, nTones, startTone);

  *resultp = HPI_SUCCESS;
  *sizep   = sizeof(*rep);
  return rep;
}

void *LineSetTrafficConfig(LineMgr *me,
                           void    *msgp,
                           uint16  *sizep, int *resultp, uint16 hmiVersion)
{
  LineSetTrafficConfig_MsgReq *req    = msgp;
  LineTrafficConfig           *lConf  = &req->data;
  int                          result = HPI_SUCCESS;

  printk("%s: size = %d\n", __FUNCTION__, *sizep);
  if (me->lineState & (LINE_IDLE_CONFIGURED | LINE_IDLE_NOT_CONFIGURED)) {
    me->lineTrafficConfig = *lConf;
  }
  else {
    result = HPI_NOT_POSSIBLE_IN_CURRENT_STATE;
  }
  
  /************** state independent late  actions ****************/
  *resultp = result;

  return NULL;
}

void* LineSetExtraConfig(LineMgr *me,
                         void    *msgp,
                         uint16  *sizep,
                         int     *resultp, uint16 hmiVersion)
{
  LineSetExtraConfig_MsgReq *req            = msgp;
  ExtraConfig               *extraConfig    = &req->data;
  int                        result         = HPI_SUCCESS;

  printk("%s: size = %d\n", __FUNCTION__, *sizep);
  me->lineExtraConfig = *extraConfig;

  *resultp = result;

  return NULL;
}



void *LineSetPhyLayerConfig(LineMgr *me,
                            void    *msgp,
                            uint16  *sizep,
                            int     *resultp, uint16 hmiVersion)
{
  LineSetPhyLayerConfig_MsgReq *req   = msgp;
  PhysicalLayerConfig          *lConf = &req->data;
  int                          result = HPI_SUCCESS;

  printk("%s: size = %d\n", __FUNCTION__, *sizep);
  me->linePhysicalLayerConfig = *lConf;

  /************** state independent late  actions ****************/
  *resultp = result;
  return NULL;
}

#ifdef ADSLDRV_LITTLE_ENDIAN

/* This function works on an array of byte, according to a layout information
 * which delimits the different fields (short, int, long) to be processed */
int bigEndianByteSwap(uint8 *data, typeLayout *layout)
{
  uint8 tmpBytes[8];            /* max 64 bits supported */
  int i, j;
  uint8 *origin = data;
  
  if (layout == NULL || data == NULL)
    return 0; /* It does not make sense to byteswap null layouts */

  for (i=0; layout[i].size != 0; i++) {
    
    if (layout[i].layout != NULL) {
      for (j=0; j<layout[i].size; j++) 
        data += bigEndianByteSwap(data, layout[i].layout);
    }

    else if (layout[i].size == 1)
      data++;
    
    else if (layout[i].size <= 8) {
      for (j=0; j<layout[i].size; j++) 
        tmpBytes[j] = data[j];
        
      for (j=layout[i].size; j--; ) 
        *data++ = tmpBytes[j];
    }

    else {
      printk("%s: Does not support object sizes larger than 8 bytes\n", __FUNCTION__);
    }
  }
  return data-origin;
}
#endif

void *LineSetConfigGFAST(LineMgr *me,
                         void    *msgp,
                         uint16  *sizep,
                         int     *resultp,
                         uint16   hmiVersion)
{
  LineSetConfigGFAST_MsgReq *req   = msgp;
  GfastConfig               *lConf = &req->data;
  
  me->gfastConfig = *lConf;
  *resultp = HPI_SUCCESS;

  if (lConf->phyConfig.enableProfileBitmap & BCM_GFAST_PROFILE_MASK_106a)
    adslCoreCfgProfile[me->lineId].vdslParam &= ~kGfastProfile106aDisable;
  else
    adslCoreCfgProfile[me->lineId].vdslParam |= kGfastProfile106aDisable;

  if (lConf->phyConfig.enableProfileBitmap & BCM_GFAST_PROFILE_MASK_212a)
    adslCoreCfgProfile[me->lineId].vdslParam &= ~kGfastProfile212aDisable;
  else
    adslCoreCfgProfile[me->lineId].vdslParam |= kGfastProfile212aDisable;

  if (lConf->phyConfig.enableProfileBitmap & BCM_GFAST_PROFILE_MASK_106b)
    adslCoreCfgProfile[me->lineId].vdslParam &= ~kGfastProfile106bDisable;
  else
    adslCoreCfgProfile[me->lineId].vdslParam |= kGfastProfile106bDisable;

  if (lConf->phyConfig.enableProfileBitmap & BCM_GFAST_PROFILE_MASK_106c)
    adslCoreCfgProfile[me->lineId].vdslParam &= ~kGfastProfile106cDisable;
  else
    adslCoreCfgProfile[me->lineId].vdslParam |= kGfastProfile106cDisable;

  if (lConf->phyConfig.enableProfileBitmap & BCM_GFAST_PROFILE_MASK_212c)
    adslCoreCfgProfile[me->lineId].vdslParam &= ~kGfastProfile212cDisable;
  else
    adslCoreCfgProfile[me->lineId].vdslParam |= kGfastProfile212cDisable;

  if (lConf->phyConfig.enableProfileBitmapMgfast & BCM_MGFAST_PROFILE_MASK_P424a)
    adslCoreCfgProfile[me->lineId].vdslParam1 &= (~kMgfastProfileP424aDisable >> 16);
  else
    adslCoreCfgProfile[me->lineId].vdslParam1 |= (kMgfastProfileP424aDisable >> 16);

  if (lConf->phyConfig.enableProfileBitmapMgfast & BCM_MGFAST_PROFILE_MASK_P424d)
    adslCoreCfgProfile[me->lineId].vdslParam1 &= (~kMgfastProfileP424dDisable >> 16);
  else
    adslCoreCfgProfile[me->lineId].vdslParam1 |= (kMgfastProfileP424dDisable >> 16);

  if (lConf->phyConfig.enableProfileBitmapMgfast & BCM_MGFAST_PROFILE_MASK_Q424a)
    adslCoreCfgProfile[me->lineId].vdslParam1 &= (~kMgfastProfileQ424cDisable >> 16);
  else
    adslCoreCfgProfile[me->lineId].vdslParam1 |= (kMgfastProfileQ424cDisable >> 16);

  if (lConf->phyConfig.enableProfileBitmapMgfast & BCM_MGFAST_PROFILE_MASK_Q424d)
    adslCoreCfgProfile[me->lineId].vdslParam1 &= (~kMgfastProfileQ424dDisable >> 16);
  else
    adslCoreCfgProfile[me->lineId].vdslParam1 |= (kMgfastProfileQ424dDisable >> 16);

  printk("%s: sizep = %d profileBitmap=0x%X vdslParam=0x%08X profileBitmapMgfast=0x%X vdslParam1=0x%08X\n",
    __FUNCTION__, *sizep, lConf->phyConfig.enableProfileBitmap, adslCoreCfgProfile[me->lineId].vdslParam,
    lConf->phyConfig.enableProfileBitmapMgfast, adslCoreCfgProfile[me->lineId].vdslParam1);

  BcmAdslCoreUpdateConnectionParam(me->lineId);

  XdslDrvSendHmiCmd(0, kXDslSetConfigGfast, sizeof(GfastConfig), lConf, GfastConfigLayout);
  return NULL;
}

void LineMgrStatus_partialReset(LineStatus *lineStatus)
{
  /* reset some data from previous init */
  LinkStatus *linkStatus = &lineStatus->linkStatus[0];
  memset(&linkStatus[0],0,sizeof(LinkStatus));
  memset(&linkStatus[1],0,sizeof(LinkStatus));

  lineStatus->selectedProtocol = -1;
  lineStatus->vdslProfile = 0;
  lineStatus->actualElectricalLength = 0;
  lineStatus->us0Mask = 0;
  lineStatus->lr_actMode = 0;
  memset(&lineStatus->perBandStatus,0,sizeof(PerBandStatus));
  memset(&lineStatus->tpsTcType,0,2*sizeof(uint8));
}

void *LineStart(LineMgr *me, void *msgp, uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  /* (hidden) flags to enable/disable certain steps:
   * 0x1 = don't reset retrain controller */
  int flags = 0;
  if (msgp)
    flags = *((int*) msgp);

  printk("%s: sizep = %d\n", __FUNCTION__, *sizep);
  *resultp = HPI_SUCCESS;
  XdslDrvConnectionStart(me->lineId);
  LineMgr_changeState(0, LINE_ITU_HANDSHAKE);
  if (!(flags & 0x1))
    LineMgrStatus_partialReset(&me->lineStatus);

  return NULL;
}

void *LineStop(LineMgr *me, void *msgp, uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  printk("%s: sizep = %d\n", __FUNCTION__, *sizep);
  //XdslDrvConnectionStop(me->lineId);
  XdslDrvSetL3Mode(me->lineId);
  XdslDrvSleepWithWakeupEvent(3000);
  LineMgr_changeState(me->lineId, LINE_IDLE_CONFIGURED);

  *resultp = HPI_SUCCESS;
  return NULL;
}

// LineTriggers.c

static uint8 CheckIfServiceIsAllowedInCurrentLineState(uint8 allowedState)
{
  LineMgr * me = getLineMgr(0);
  uint8 lineState = me->lineState;

  return ((lineState & allowedState) &&
          /* use strict equality for DM_COMPLETE, else L2 and idle config will match the above test as well */
          (allowedState != LINE_IDLE_DM_COMPLETE ||
          lineState == LINE_IDLE_DM_COMPLETE) &&
          /* same for showtime or DM_COMPLETE */
          (allowedState != (LINE_SHOWTIME|LINE_IDLE_DM_COMPLETE) ||
          (lineState & LINE_SHOWTIME) || lineState == LINE_IDLE_DM_COMPLETE));
}

#define DISABLE_RX_BITSWAP  0x1
#define DISABLE_TX_BITSWAP  0x2

#define DISABLE_TRELLIS     0x1
#define DISABLE_RS          0x2

extern void BcmAdslCoreConfigure(unsigned char lineId, adslCfgProfile *pAdslCfg);
void HmiMgr_init(LineMgr *me);
void HmiMgr_messageReceived(OverheadClient* overheadClient, ErrorType errorType);

void LineInitOvhMsg(unsigned char lineId)
{
  LineMgr *lineMgr = getLineMgr(lineId);
  HmiMgr_init(lineMgr);
}

void LineInitShowtimeLogEvents(unsigned char lineId)
{
  LineMgr *lineMgr = getLineMgr(lineId);
  memset((void *)&lineMgr->eventTimeLogTimeStamp[0], 0, sizeof(lineMgr->eventTimeLogTimeStamp));
  memset((void *)&lineMgr->lineStatus.lastRetrainInfo.eventTimeLog[0], 0xFF, sizeof(lineMgr->lineStatus.lastRetrainInfo.eventTimeLog));
}

void LineMgr_LogShowtimeEventTimeStamp(unsigned char lineId, ShowtimeEvent event)
{
  adslMibInfo *pMibInfo;
  long        mibLen;
  LineMgr *lineMgr = getLineMgr(lineId);

  mibLen    = sizeof(adslMibInfo);
  pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);
  BcmCoreDpcSyncEnter(SYNC_RX);
  lineMgr->eventTimeLogTimeStamp[event] = pMibInfo->adslPerfData.adslSinceLinkTimeElapsed;
  BcmCoreDpcSyncExit(SYNC_RX);
  switch(event)
  {
    case SHOWTIME_RX_BS_APPLIED:
      lineIRQ_setStatus(lineId, INT_BITSWAP_NE_MASK);
      break;
    case SHOWTIME_RX_SRA_APPLIED:
      lineIRQ_setStatus(lineId, INT_SRA_NE_RATE_CHANGE_MASK);
      break;
    case SHOWTIME_RX_RPA_APPLIED:
      lineIRQ_setStatus(lineId, INT_RPA_NE_MASK);
      break;
    case SHOWTIME_RX_FRA_APPLIED:
      lineIRQ_setStatus(lineId, INT_SOS_NE_RATE_CHANGE_MASK);
      break;
    case SHOWTIME_TX_BS_APPLIED:
      lineIRQ_setStatus(lineId, INT_BITSWAP_FE_MASK);
      break;
    case SHOWTIME_TX_SRA_APPLIED:
      lineIRQ_setStatus(lineId, INT_SRA_FE_RATE_CHANGE_MASK);
      break;
    case SHOWTIME_TX_TIGA_APPLIED:
      lineIRQ_setStatus(lineId, INT_TIGA_MASK);
      break;
    case SHOWTIME_TX_RPA_APPLIED:
      lineIRQ_setStatus(lineId, INT_RPA_FE_MASK);
      break;
    case SHOWTIME_TX_FRA_APPLIED:
      lineIRQ_setStatus(lineId, INT_SOS_FE_RATE_CHANGE_MASK);
    default:
      break;
  }

}

int LineStateInTraining(unsigned char lineId)
{
  LineMgr *lineMgr = getLineMgr(lineId);
  return (LINE_TRAINING == lineMgr->lineState);
}
void LineStateSet(unsigned char lineId, unsigned char state)
{
  LineMgr *lineMgr = getLineMgr(lineId);
  lineMgr->lineState = state;
}

void LineMgr_changeState(unsigned char lineId, int state)
{
  LineMgr *lineMgr = getLineMgr(lineId);

  if(LINE_SHOWTIME == state) {
    LineInitOvhMsg(lineId);
    LineInitShowtimeLogEvents(lineId);
    XdslDrvSetGfastTestMode(lineId,
      lineMgr->gfastConfig.trafficConfig[0].rtxTestMode,
      lineMgr->gfastConfig.trafficConfig[1].rtxTestMode,
      lineMgr->gfastConfig.trafficConfig[0].tpsTcTestMode,
      lineMgr->gfastConfig.trafficConfig[1].tpsTcTestMode);
  }
  lineMgr->lineState = state;
  lineIRQ_setStatus(lineId, INT_LINE_STATE_CHANGE_MASK);
}

static void XdslGetTestConfig(uint8 lineId, TestConfigMessage *pLineTestConfig)
{
  uint32 disableProtocolBitmap = (uint32)-1;  /* Disable everything */
  uint8 disableCoding = 0;
  uint8 disableBitSwap = 0;
  uint32 dslCfgParam = adslCoreCfgProfile[lineId].adslAnnexAParam;
  uint32 demodCapValue = adslCoreCfgProfile[lineId].adslDemodCapValue;
  uint32 adsl2Param = adslCoreCfgProfile[lineId].adsl2Param;

  
  if(kAdslCfgModAny == (dslCfgParam & kAdslCfgModMask)) {
    if(adsl2Param & kAdsl2CfgAnnexMOnly)
      disableProtocolBitmap &= ~LINE_MODE_MASK_992_5_M;
    else {
      disableProtocolBitmap &= ~(LINE_MODE_MASK_992_1_A | LINE_MODE_MASK_992_2_A | LINE_MODE_MASK_ANSI | LINE_MODE_MASK_992_3_A |\
        LINE_MODE_MASK_992_3_L1 | LINE_MODE_MASK_992_5_A | LINE_MODE_MASK_993_5 | LINE_MODE_MASK_9701 | LINE_MODE_MASK_9711);
      if(adsl2Param & kAdsl2CfgAnnexMEnabled)
        disableProtocolBitmap &= ~LINE_MODE_MASK_992_5_M;
    }
  }
  else {
    if(dslCfgParam & kAdslCfgModGdmtOnly)
      disableProtocolBitmap &= ~LINE_MODE_MASK_992_1_A;
    if(dslCfgParam & kAdslCfgModGliteOnly)
      disableProtocolBitmap &= ~LINE_MODE_MASK_992_2_A;
    if(dslCfgParam & kAdslCfgModT1413Only)
      disableProtocolBitmap &= ~LINE_MODE_MASK_ANSI;
    if(dslCfgParam & kAdslCfgModAdsl2Only)
      disableProtocolBitmap &= ~LINE_MODE_MASK_992_3_A;
    if(adsl2Param & kAdsl2CfgReachExOn)
      disableProtocolBitmap &= ~LINE_MODE_MASK_992_3_L1;
    if(adsl2Param & kAdsl2CfgAnnexMEnabled)
      disableProtocolBitmap &= ~LINE_MODE_MASK_992_5_M;
    if(dslCfgParam & kAdslCfgModAdsl2pOnly)
      disableProtocolBitmap &= ~LINE_MODE_MASK_992_5_A;
    if(dslCfgParam & kDslCfgModVdsl2Only)
      disableProtocolBitmap &= ~LINE_MODE_MASK_993_5;
    if(dslCfgParam & kDslCfgModGfastOnly)
      disableProtocolBitmap &= ~LINE_MODE_MASK_9701;
    if(dslCfgParam & kDslCfgModMgfastOnly)
      disableProtocolBitmap &= ~LINE_MODE_MASK_9711;
  }
  
  if(0 == (demodCapValue & kXdslBitSwapEnabled))
    disableBitSwap |= DISABLE_RX_BITSWAP;
  if(0 == (demodCapValue & kXdslTrellisEnabled))
    disableCoding |= DISABLE_TRELLIS;
  
  pLineTestConfig->disableProtocolBitmap = disableProtocolBitmap;
  pLineTestConfig->disableBitSwap = disableBitSwap;
  pLineTestConfig->disableCoding = disableCoding;
}

static void XdslSetTestConfig(uint8 lineId, TestConfigMessage *pLineTestConfig)
{
  uint32 disableProtocolBitmap = pLineTestConfig->disableProtocolBitmap;
  uint32 *pDslCfgParam = &adslCoreCfgProfile[lineId].adslAnnexAParam;
  int32 *pDemodCapValue = &adslCoreCfgProfile[lineId].adslDemodCapValue;
  int32 *pAdsl2Param = &adslCoreCfgProfile[lineId].adsl2Param;
  
  adslCoreCfgProfile[lineId].adslDemodCapMask |= (kXdslBitSwapEnabled | kXdslTrellisEnabled);

  if(disableProtocolBitmap & LINE_MODE_MASK_992_1_A)
    *pDslCfgParam &= ~kAdslCfgModGdmtOnly;
  else
    *pDslCfgParam |= kAdslCfgModGdmtOnly;
  if(disableProtocolBitmap & LINE_MODE_MASK_992_2_A)
    *pDslCfgParam &= ~kAdslCfgModGliteOnly;
  else
    *pDslCfgParam |= kAdslCfgModGliteOnly;
  if(disableProtocolBitmap & LINE_MODE_MASK_ANSI)
    *pDslCfgParam &= ~kAdslCfgModT1413Only;
  else
    *pDslCfgParam |= kAdslCfgModT1413Only;
  if(disableProtocolBitmap & LINE_MODE_MASK_992_3_A)
    *pDslCfgParam &= ~kAdslCfgModAdsl2Only;
  else
    *pDslCfgParam |= kAdslCfgModAdsl2Only;
  if(disableProtocolBitmap & LINE_MODE_MASK_992_3_L1)
    *pAdsl2Param &= ~kAdsl2CfgReachExOn;
  else
    *pAdsl2Param |= kAdsl2CfgReachExOn;
  if(disableProtocolBitmap & LINE_MODE_MASK_992_5_M)
    *pAdsl2Param &= ~kAdsl2CfgAnnexMEnabled;
  else
    *pAdsl2Param |= kAdsl2CfgAnnexMEnabled;
  if(disableProtocolBitmap & LINE_MODE_MASK_992_5_A)
    *pDslCfgParam &= ~kAdslCfgModAdsl2pOnly;
  else
    *pDslCfgParam |= kAdslCfgModAdsl2pOnly;
  if(disableProtocolBitmap & LINE_MODE_MASK_993_5)
    *pDslCfgParam &= ~kDslCfgModVdsl2Only;
  else
    *pDslCfgParam |= kDslCfgModVdsl2Only;
  if(disableProtocolBitmap & LINE_MODE_MASK_9701)
    *pDslCfgParam &= ~kDslCfgModGfastOnly;
  else
    *pDslCfgParam |= kDslCfgModGfastOnly;
  if(disableProtocolBitmap & LINE_MODE_MASK_9711)
    *pDslCfgParam &= ~kDslCfgModMgfastOnly;
  else
    *pDslCfgParam |= kDslCfgModMgfastOnly;

  if(pLineTestConfig->disableBitSwap & DISABLE_RX_BITSWAP)
    *pDemodCapValue &= ~kXdslBitSwapEnabled;
  else
    *pDemodCapValue |= kXdslBitSwapEnabled;

  if(pLineTestConfig->disableCoding & DISABLE_TRELLIS)
    *pDemodCapValue &= ~kXdslTrellisEnabled;
  else
    *pDemodCapValue |= kXdslTrellisEnabled;
  
  BcmAdslCoreUpdateConnectionParam(lineId);
}

static void XdslGetGfastConfig(uint8 lineId, GfastConfig *pGfastConfig)
{
  uint8 enableProfileBitmap = 0;

  if(!(adslCoreCfgProfile[lineId].vdslParam & kGfastProfile106aDisable))
    enableProfileBitmap |= BCM_GFAST_PROFILE_MASK_106a;
  if(!(adslCoreCfgProfile[lineId].vdslParam & kGfastProfile212aDisable))
    enableProfileBitmap |= BCM_GFAST_PROFILE_MASK_212a;
  if(!(adslCoreCfgProfile[lineId].vdslParam & kGfastProfile106bDisable))
    enableProfileBitmap |= BCM_GFAST_PROFILE_MASK_106b;
  if(!(adslCoreCfgProfile[lineId].vdslParam & kGfastProfile106cDisable))
    enableProfileBitmap |= BCM_GFAST_PROFILE_MASK_106c;
  if(!(adslCoreCfgProfile[lineId].vdslParam & kGfastProfile212cDisable))
    enableProfileBitmap |= BCM_GFAST_PROFILE_MASK_212c;

  pGfastConfig->phyConfig.enableProfileBitmap = enableProfileBitmap;

  printk("%s: vdslParam=0x%08X enableProfileBitmap=0x%X\n",
    __FUNCTION__, adslCoreCfgProfile[lineId].vdslParam, enableProfileBitmap);
}

static void cpyBackGfastCfg(GfastConfig *cfgHMI,GfastConfigInternal *cfgInternal)
{
  PhysicalLayerConfigGfastInternal  *phyCfgInternal = &cfgInternal->phyConfig;
  PhysicalLayerConfigGfast          *phyCfgHMI      = &cfgHMI->phyConfig;

//  memset(cfgHMI,0,sizeof(GfastConfigHMI));
  phyCfgHMI->limitingMaskDn=phyCfgInternal->limitingMaskDn;
  phyCfgHMI->limitingMaskUp=phyCfgInternal->limitingMaskUp;

  phyCfgHMI->bandPlanDn = phyCfgInternal->bandPlanDn.toneGroups[0];
  phyCfgHMI->bandPlanUp = phyCfgInternal->bandPlanUp.toneGroups[0];

  memcpy(&phyCfgHMI->pboPsdUp,&phyCfgInternal->usPboDescriptor,sizeof(UsPboDescriptorGfast));
  phyCfgHMI->enableProfileBitmap   =phyCfgInternal->enableProfileBitmap;
  phyCfgHMI->enableProfileBitmapMgfast =phyCfgInternal->vectorFlag;
  phyCfgHMI->forceElectricalLength =phyCfgInternal->forceElectricalLength;
  phyCfgHMI->maxRxPowerUp          =phyCfgInternal->maxRxPowerUp;
  phyCfgHMI->retrainAllowed        =phyCfgInternal->retrainAllowed;
  phyCfgHMI->maxUsedTone           =phyCfgInternal->maxToneUsed;
  phyCfgHMI->initialTgPrime        =phyCfgInternal->initialTgPrime;
  phyCfgHMI->Sds                  =phyCfgInternal->m_ds;
  phyCfgHMI->Sus                  =phyCfgInternal->m_us;
  phyCfgHMI->soc_redundancy        =phyCfgInternal->soc_redundancy;
  memcpy(phyCfgHMI->reinitConfig,phyCfgInternal->reinitConfig,2*sizeof(ReinitConfig));
  memcpy(phyCfgHMI->linkConfig,phyCfgInternal->linkConfig,2*sizeof(LinkConfig));
  memcpy(phyCfgHMI->rmcrLorTrigger,phyCfgInternal->rmcrConfig,2);
  memcpy(cfgHMI->trafficConfig,cfgInternal->trafficConfig,2*sizeof(TrafficConfigGfast));
  memcpy(cfgHMI->rmcConfig,    cfgInternal->rmcConfig,2*sizeof(RmcConfig));
  memcpy(cfgHMI->fraConfig,    cfgInternal->fraConfig,2*sizeof(FraConfig));
}

void LineMgr_init(void)
{
  uint8 lineId = 0;
  LineMgr *lineMgr = getLineMgr(lineId);
  LineStatus  *pLineStatus = &lineMgr->lineStatus;
  LineTrafficConfig *pLineTrafficConfig = &lineMgr->lineTrafficConfig;
  PhysicalLayerConfig *pLinePhysicalLayerConfig = &lineMgr->linePhysicalLayerConfig;
  PhysicalLayerConfigGfastInternal *phyCfgInternal = &gfastConfigInternalDefault.phyConfig;

  memset(lineMgr, 0, sizeof(LineMgr));
  
  /* Populate default GfastRFIConfig */
  memcpy(&lineMgr->gFastRFIConfig.rfiNotches, &phyCfgInternal->rfiBands, sizeof(RfiBandsGfast));
  lineMgr->gFastRFIConfig.iarBands  = phyCfgInternal->iarBands;
  memcpy(&lineMgr->gFastRFIConfig.carMaskDn, &phyCfgInternal->carMaskDn, sizeof(CarMaskGfast));
  memcpy(&lineMgr->gFastRFIConfig.carMaskUp, &phyCfgInternal->carMaskUp, sizeof(CarMaskGfast));
  /* Populate default GfastConfig */
  cpyBackGfastCfg(&lineMgr->gfastConfig, &gfastConfigInternalDefault);
  GfastConfig_convertToHMIformat(&lineMgr->gfastConfig);

  pLineStatus->state = LINE_STATUS_IDL_NCON;
  pLineStatus->selectedProtocol = LINE_SEL_PROT_NONE;
  pLineStatus->tpsTcType[B0] = pLineStatus->tpsTcType[B1] = 0x10; /* PTM(Gfast) */
  pLineStatus->timingStatus = -1; /* -1 -> No information from the CPE */

  pLineTrafficConfig->bearer[0].isEnabled = 1;

  pLinePhysicalLayerConfig->optionFlags |= 0x02; /* NTR support disabled */

  XdslGetTestConfig(lineId, &lineMgr->lineTestConfig);

  XdslGetGfastConfig(lineId, &lineMgr->gfastConfig);

  LineMgr_changeState(lineId, LINE_IDLE_RESET);

  HmiMgr_init(lineMgr);
}

void LineMgrSendGfastConfig(uint8 lineId)
{
  LineMgr *lineMgr = getLineMgr(lineId);

  XdslDrvSendHmiCmd(lineId, kXDslSetHmiCoreConfig, sizeof(XdslCoreConfig), &xdslCoreCfg, NULL);
  XdslDrvSendHmiCmd(lineId, kXDslSetConfigGfast, sizeof(GfastConfig), &lineMgr->gfastConfig, GfastConfigLayout);
  XdslDrvSendHmiCmd(lineId, kXDslSetRfiConfigGfast, sizeof(GfastRFIconfig), &lineMgr->gFastRFIConfig, GfastRFIconfigLayout);
}

void LineHPIserver(void *msgp, uint16 size, uint16 commandId, uint16 hmiVersion)
{
  int      result;
  LineMgr *me      = NULL;
  void    *repMsgp = NULL;

  me = getLineMgr(0);

  if (me->lineState == LINE_IDLE_RESET)
  {
    /* the Core is not yet configured or the current line is not enabled in the current config */
    /* send reply invalid request */
    result = HPI_NOT_POSSIBLE_IN_CURRENT_STATE;
  }
  else
  {
    if (commandId == LINE_RESET)
    {
      int keepOam = (*(int32*)(msgp));

      if (size < sizeof(uint32) || keepOam == 0)
          me->keepOam = 0;
      else
          me->keepOam = 1;

      /* immediately post the reply because line reset should not return !!! */
      repMsgp = releaseRequestAllocateReply(msgp,0);
      result = HPI_SUCCESS;
      size = 0;
      XdslDrvConnectionStop(0);
      XdslDrvSleepWithWakeupEvent(3000);
      XdslDrvConnectionStart(0);
      LineMgr_changeState(0, LINE_ITU_HANDSHAKE);
      goto rep_send;
    }
    else
    {
      int idx       = commandId &  0xFF;
      int serviceId = commandId & ~0xFF;

      /* do a minimum check to verify whether the commandId is correct */
      if ((idx < SERVICE_SIZE)            &&
          (services[idx].handler != NULL) &&
          ((serviceId == LINE_SERVICE_ID) ||(serviceId == LINE_DEBUG_ID))
         )
      {
        int minSize      = services[idx].minSize;
        int expectedSize = services[idx].size;

        if(minSize == MIN_IS_MAX)
          minSize = expectedSize;

        if (DummyMsgHandler == services[idx].handler) {
          minSize = size;
          expectedSize = size;
          if (0 == services[idx].allowedState) {
            result = HPI_SUCCESS;
            repMsgp = releaseRequestAllocateReply(msgp,size);
            goto rep_send;
          }
        }

        if( (size <= expectedSize) && (size >= minSize))
        {
          /* pad with zeros the message if it's shorter than expected size */
          memset(((uint8 *)msgp)+size,0,expectedSize-size);
          
          if (CheckIfServiceIsAllowedInCurrentLineState(services[idx].allowedState))
            /* call the function through a lookup table */
            repMsgp = (services[idx].handler)(me, msgp, &size, &result, hmiVersion);
          else
            result = HPI_NOT_POSSIBLE_IN_CURRENT_STATE;
        }
        else
        {
            result = HPI_WRONG_HEADER_FORMAT;
#if 1
            DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineHPIserver: message 0x%X, size = %d, expectedSize = %d, minSize = %d",
              commandId, size, expectedSize, minSize);
            printk("LineHPIserver: message 0x%X, size = %d, expectedSize = %d, minSize = %d\n", commandId, size, expectedSize, minSize);
#endif
        }

      }
      else
      {
#if 1
        DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineHPIserver: non supported Line HPI message 0x%X", commandId);
        printk("LineHPIserver: non supported Line HPI message 0x%X\n", commandId);
#endif
        result = HPI_UNKNOWN_COMMAND_ID;
      }
    }
  }

  if (repMsgp == NULL)
  {
    size    = 0;
    repMsgp = releaseRequestAllocateReply(msgp,size);
  }
rep_send:
  hpi_post(repMsgp, size, result);

#if 0
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineHPIserver: reply res=%d size=%d", result, size);
  if ( (size != 0) && (commandId != GET_LINE_STATUS) )
    DiagDumpData(0, DIAG_DSL_CLIENT, repMsgp, size, kDiagDbgDataFormatHex | kDiagDbgDataSize8);
#endif
}


void *LineSetTestConfig(LineMgr *me,
                           void *msgp,
                         uint16 *sizep, int *resultp, uint16 hmiVersion)
{

  LineSetTestConfig_MsgReq  *req    = msgp;
  TestConfigMessage         *lConf  = &req->data;
  int                        result = HPI_SUCCESS;

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineSetTestConfig: size=%d", result, *sizep);
  me->lineTestConfig = *lConf;
  XdslSetTestConfig(me->lineId, lConf);

  *resultp = result;

  return NULL;
}


static void *LineGetCPEvendorInfo(LineMgr *me,
                              void *msgp,
                            uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  adslMibInfo *pMibInfo;
  long        mibLen;
  LineGetCPEvendorInfo_MsgRep  *rep;
  int size = sizeof(*rep);
  mibLen    = sizeof(adslMibInfo);
  pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);
  rep = releaseRequestAllocateReply(msgp,size);
  memset((void *)rep, 0, size);
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetCPEvendorInfo: Fix me! Populate the response");
  memcpy(&rep->data.hsVendorInfo.countryCode[0], &pMibInfo->xdslAtucPhys.adslVendorID[0], 2);
  memcpy(&rep->data.hsVendorInfo.providerCode[0], &pMibInfo->xdslAtucPhys.adslVendorID[2], 4);
  memcpy(&rep->data.hsVendorInfo.vendorInfo[0], &pMibInfo->xdslAtucPhys.adslVendorID[6], 2);
#if 0
#define CPE_VENDOR_INFO_HS_AVAIL   1
#define CPE_VENDOR_INFO_ANSI_AVAIL 2
#define CPE_VENDOR_INFO_BCM_AVAIL  4
#define CPE_VENDOR_INFO_3RD_PARTY_AVAIL  8
#endif
  if(XdslDrvIsVendorIdReceived(me->lineId))
    rep->data.availInfo = CPE_VENDOR_INFO_HS_AVAIL;
  *sizep = size;
  *resultp = HPI_SUCCESS;

  return rep;
}

void *LineGetTrafficConfig(LineMgr *me,
                           void    *msgp,
                           uint16  *sizep,
                           int     *resultp, uint16 hmiVersion)
{
  int result = HPI_SUCCESS;
  LineGetTrafficConfig_MsgRep *rep;
  rep = releaseRequestAllocateReply(msgp,sizeof(LineGetTrafficConfig_MsgRep));
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetTrafficConfig: size = %ld", sizeof(*rep));
  rep->data = me->lineTrafficConfig;
  *sizep = sizeof(*rep);
  *resultp = result;

  return rep;
}


static void *LineGetPhyLayerConfig(LineMgr *me,
                                   void    *msgp,
                                   uint16  *sizep,
                                   int     *resultp, uint16 hmiVersion)
{
  int result = HPI_SUCCESS;
  LineGetPhyLayerConfig_MsgRep *rep;

  rep = releaseRequestAllocateReply(msgp, sizeof(*rep));
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetPhyLayerConfig: size = %ld", sizeof(*rep));
  rep->data = me->linePhysicalLayerConfig;
  *sizep = sizeof(*rep);
  *resultp = result;
  
  return rep;
}

static void *LineGetBandPlanVDSL(LineMgr *me,void    *msgp,uint16  *sizep,int     *resultp, uint16 hmiVersion)
{
  LineGetBandPlanVDSL_MsgRep *rep;
  BandPlanDescriptor  bandPlan;
  adslMibInfo         *pMibInfo;
  long                mibLen;
  int                 nToneGroup;
  
  mibLen    = sizeof(adslMibInfo);
  pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);
  
  rep = releaseRequestAllocateReply(msgp, sizeof(*rep));
  
  memset((void *)&bandPlan, 0, sizeof(bandPlan));
  memcpy((void *)&bandPlan, &pMibInfo->usNegBandPlan, sizeof(pMibInfo->usNegBandPlan));
  rep->data.bandPlanUp = bandPlan;
  memset((void *)&bandPlan, 0, sizeof(bandPlan));
  memcpy((void *)&bandPlan, &pMibInfo->dsNegBandPlan, sizeof(pMibInfo->dsNegBandPlan));
  rep->data.bandPlanDn = bandPlan;

  nToneGroup = pMibInfo->dsNegBandPlanDiscovery.noOfToneGroups;
  rep->data.supportedSetBoundsDn.endTone    = pMibInfo->dsNegBandPlanDiscovery.toneGroups[nToneGroup-1].endTone;
  rep->data.supportedSetBoundsDn.startTone  = pMibInfo->dsNegBandPlanDiscovery.toneGroups[0].startTone;
  nToneGroup = pMibInfo->usNegBandPlanDiscovery.noOfToneGroups;
  rep->data.supportedSetBoundsUp.endTone    = pMibInfo->usNegBandPlanDiscovery.toneGroups[nToneGroup-1].endTone;
  rep->data.supportedSetBoundsUp.startTone  = pMibInfo->usNegBandPlanDiscovery.toneGroups[0].startTone;
  nToneGroup = pMibInfo->dsNegBandPlan.noOfToneGroups;
  rep->data.medleySetBoundsDn.endTone       = pMibInfo->dsNegBandPlan.toneGroups[nToneGroup-1].endTone;
  rep->data.medleySetBoundsDn.startTone     = pMibInfo->dsNegBandPlan.toneGroups[0].startTone;
  nToneGroup = pMibInfo->usNegBandPlan.noOfToneGroups;
  rep->data.medleySetBoundsUp.endTone       = pMibInfo->usNegBandPlan.toneGroups[nToneGroup-1].endTone;
  rep->data.medleySetBoundsUp.startTone     = pMibInfo->usNegBandPlan.toneGroups[0].startTone;

  *sizep = sizeof(*rep);
  *resultp = HPI_SUCCESS;

  return rep;
}

static void *LineGetPsdVDSL(LineMgr *me,void    *msgp,uint16  *sizep,int     *resultp, uint16 hmiVersion)
{
  LineGetPsdVDSL_MsgRep *rep;
  int         i;
  char        oidStr[]  = { kOidAdslPrivate, kOidAdslPrivGetMrefPsdInfo};
  long        dataLen   = 0;
  MrefPsd     *pMrefPsd = (MrefPsd *) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);

  rep = releaseRequestAllocateReply(msgp, sizeof(*rep));
  *resultp = HPI_SUCCESS;
  
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetPsdVDSL: dsDescriptor.n=%d, usDescriptor.n=%d", pMrefPsd->dsDescriptor.n, pMrefPsd->usDescriptor.n);

  memset( &rep->data, 0, sizeof(rep->data) );
  rep->data.dsDescriptor.reserved = BCM_MAX_DS_MREF_PSD_BP;
  rep->data.dsDescriptor.n        = (pMrefPsd->dsDescriptor.n < BCM_MAX_DS_MREF_PSD_BP) ? pMrefPsd->dsDescriptor.n: BCM_MAX_DS_MREF_PSD_BP;
  for(i = 0; i < rep->data.dsDescriptor.n; i ++) {
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "DS bk %d tone %d psd %d", i, pMrefPsd->dsDescriptor.bp[i].toneIndex, pMrefPsd->dsDescriptor.bp[i].psd);
    rep->data.dsDescriptor.bp[i].toneIndex  = pMrefPsd->dsDescriptor.bp[i].toneIndex;
    rep->data.dsDescriptor.bp[i].psd        = pMrefPsd->dsDescriptor.bp[i].psd;
  }
  
  rep->data.usDescriptor.reserved = BCM_MAX_US_MREF_PSD_BP;
  rep->data.usDescriptor.n        = (pMrefPsd->usDescriptor.n < BCM_MAX_US_MREF_PSD_BP) ? pMrefPsd->usDescriptor.n: BCM_MAX_US_MREF_PSD_BP;
  for(i = 0; i < rep->data.usDescriptor.n; i ++) {
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "US bk %d tone %d psd %d", i, pMrefPsd->usDescriptor.bp[i].toneIndex, pMrefPsd->usDescriptor.bp[i].psd);
    rep->data.usDescriptor.bp[i].toneIndex  = pMrefPsd->usDescriptor.bp[i].toneIndex;
    rep->data.usDescriptor.bp[i].psd        = pMrefPsd->usDescriptor.bp[i].psd;
  }

  *sizep = sizeof(*rep);

  return rep;
}

static void *LineGetConfigGFAST(LineMgr *me,
                                void    *msgp,
                                uint16  *sizep,
                                int     *resultp, uint16 hmiVersion)
{
  int result = HPI_SUCCESS;
  LineGetConfigGFAST_MsgRep *rep;

  rep = releaseRequestAllocateReply(msgp, sizeof(*rep));
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetConfigGFAST: sizep = %ld", sizeof(*rep));
  rep->data = me->gfastConfig;
  *sizep = sizeof(*rep);
  *resultp = result;

  return rep;
}

static void *LineSetGfastRFIconfig(LineMgr *me,
                                   void    *msgp,
                                   uint16  *sizep,
                                   int     *resultp,
                                   uint16   hmiVersion)
{
  LineSetGfastRFIconfig_MsgReq  *req   = msgp;
  GfastRFIconfig                *lConf = &req->data;
  int result = HPI_SUCCESS;
  me->gFastRFIConfig = *lConf;
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineSetGfastRFIconfig: sizep = %d", *sizep);
  XdslDrvSendHmiCmd(0, kXDslSetRfiConfigGfast, sizeof(GfastRFIconfig), lConf, GfastRFIconfigLayout);

  *resultp = result;

  return NULL;
}

static void *LineGetGfastRFIconfig(LineMgr *me,
                                void    *msgp,
                                uint16  *sizep,
                                int     *resultp, uint16 hmiVersion)
{
  int result = HPI_SUCCESS;
  LineGetGfastRFIconfig_MsgRep *rep;

  rep = releaseRequestAllocateReply(msgp, sizeof(*rep));
  rep->data = me->gFastRFIConfig;
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetGfastRFIconfig: sizep = %ld", sizeof(*rep));
  *sizep = sizeof(*rep);
  *resultp = result;

  return rep;
}

static void* LineChangeMds(LineMgr *me, void *msgp, uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  DtaRequest    *dtaReq        =  &((( LineChangeMds_MsgReq*)msgp)->data);

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineChangeMds: sizep = %d", *sizep);
  *resultp = HPI_SUCCESS;
  me->dtaReq = *dtaReq;
  
  XdslDrvSendHmiCmd(0, kXDslSetHmiChangeMds, sizeof(DtaRequest), dtaReq, DtaRequestLayout);

  return 0;
}
static void* LineSetDtaConfig(LineMgr *me, void *msgp, uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  adslMibInfo             *pMibInfo;
  long                    mibLen;
  LineSetDtaConfig_MsgReq *req   = msgp;
  DtaConfig               *dtaConfig = &req->data;

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineSetDtaConfig: sizep = %d", *sizep);
  me->dtaConfig = *dtaConfig;
  *resultp = HPI_SUCCESS;
  XdslDrvSendHmiCmd(0, kXDslSetHmiDtaConfig, sizeof(DtaConfig), dtaConfig, DtaConfigLayout);
  if(0 == me->dtaConfig.dtaCounter) {
    mibLen    = sizeof(adslMibInfo);
    pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);
    memset((void *)&pMibInfo->gfastDta.lastMds[0], 0, sizeof(pMibInfo->gfastDta.lastMds));
    pMibInfo->gfastDta.lastMdsIdx = 0;
    pMibInfo->gfastDta.dtaEventCnt = 0;
  }
  return NULL;
}

static void* LineGetDtaConfig(LineMgr *me, void *msgp, uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  adslMibInfo             *pMibInfo;
  long                    mibLen;
  LineGetDtaConfig_MsgRep *rep  = releaseRequestAllocateReply(msgp, sizeof(LineGetDtaConfig_MsgRep));
  DtaConfig               *dtaConfig = &rep->data;

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetDtaConfig: sizep = %ld", sizeof(*rep));
  *dtaConfig = me->dtaConfig;
  *sizep = sizeof(*rep);
  *resultp = HPI_SUCCESS;
  
  mibLen    = sizeof(adslMibInfo);
  pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);
  memset((void *)&dtaConfig->lastMds[0], 0, sizeof(dtaConfig->lastMds));
  if(0 != pMibInfo->gfastDta.dtaEventCnt) {
    int i, lastMdsMaxCnt, lastMdsIdx = pMibInfo->gfastDta.lastMdsIdx;
    dtaConfig->dtaCounter = pMibInfo->gfastDta.dtaEventCnt;
    lastMdsMaxCnt = (dtaConfig->dtaCounter < MAX_LAST_Mds) ? dtaConfig->dtaCounter: MAX_LAST_Mds;
    for(i = 0; i < lastMdsMaxCnt; i++) {
      lastMdsIdx--;
      if(lastMdsIdx < 0)
        lastMdsIdx = MAX_LAST_Mds - 1;
      dtaConfig->lastMds[i] = pMibInfo->gfastDta.lastMds[lastMdsIdx];
    }
  }
  else
    dtaConfig->dtaCounter = 0;
  
  return rep;
}

static void *LineGetTestConfig(LineMgr *me,
                        void    *msgp,
                        uint16  *sizep,
                        int     *resultp, uint16 hmiVersion)
{
  LineGetTestConfig_MsgRep *rep;
  rep = releaseRequestAllocateReply(msgp, sizeof(LineGetTestConfig_MsgRep));

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetTestConfig: sizep = %ld", sizeof(*rep));
  XdslGetTestConfig(me->lineId, &me->lineTestConfig);
  rep->data = me->lineTestConfig;
  *sizep = sizeof(*rep);
  *resultp = HPI_SUCCESS;

  return rep;
}

typedef struct Int2ExtStateMap {
  uint8 intState;
  uint8 extState;
} Int2ExtStateMap;

#define LINE_EXT_STATE_IDLE_NOT_CONFIGURED      0
#define LINE_EXT_STATE_IDLE_CONFIGURED          1
#define LINE_EXT_STATE_RUNNING_ACTIVATION       2
#define LINE_EXT_STATE_RUNNING_INITIALIZATION   3
#define LINE_EXT_STATE_RUNNING_SHOWTIME_L0      4
#define LINE_EXT_STATE_TEST_MODE                5
#define LINE_EXT_STATE_RUNNING_DM_INIT          6
#define LINE_EXT_STATE_IDLE_DM_COMPLETE         7
#define LINE_EXT_STATE_RUNNING_SHOWTIME_L2      8

#define LINE_EXT_STATE_INVALID                  9

static Int2ExtStateMap int2ExtStateMap[] = {
  { LINE_IDLE_NOT_CONFIGURED, LINE_EXT_STATE_IDLE_NOT_CONFIGURED},
  { LINE_IDLE_CONFIGURED, LINE_EXT_STATE_IDLE_CONFIGURED},
  { LINE_IDLE_DM_COMPLETE, LINE_EXT_STATE_IDLE_DM_COMPLETE},
  { LINE_ITU_HANDSHAKE, LINE_EXT_STATE_RUNNING_ACTIVATION},
  { LINE_TRAINING, LINE_EXT_STATE_RUNNING_INITIALIZATION},
  { LINE_DM_TRAINING, LINE_EXT_STATE_RUNNING_DM_INIT},
  { LINE_SHOWTIME, LINE_EXT_STATE_RUNNING_SHOWTIME_L0},
  { LINE_SHOWTIME_L2, LINE_EXT_STATE_RUNNING_SHOWTIME_L2},
  { LINE_TEST_STATE, LINE_EXT_STATE_TEST_MODE}
};

static int GetSelectedProtol(int modType, int xdslMode)
{
  int sp = -1;
  
  switch (modType)
  {
  case kAdslModGdmt:
    sp = 0;
    break;
  case kAdslModGlite:
    sp = 3;
    break;
  case kAdslModAnnexI:
    sp = 21;
    break;
  case kAdslModAdsl2:
    if(xdslMode == kAdslTypeAnnexA)
      sp = 16;
    else if(xdslMode == kAdslTypeAnnexB)
      sp = 17;
    else if(xdslMode == kAdslTypeAnnexI)
      sp = 18;
    break;
  case kAdslModAdsl2p:
    if(xdslMode == kAdslTypeAnnexA)
      sp = 24;
    else if(xdslMode == kAdslTypeAnnexB)
      sp = 25;
    else if(xdslMode == kAdslTypeAnnexI)
      sp = 26;
    break;
  case kAdslModReAdsl2:
    sp = 22;
    break;
  case kVdslModVdsl2:
    sp = 7;
    break;
  case kXdslModGfast:
    sp = 11;
    break;
  case kAdslModT1413:
    sp = 30;
    break;
  case kXdslModMgfast:
    sp = LINE_SEL_PROT_9711;
    break;
  default:
    sp = -1;
  }
  
  return sp;
}

static uint8 GetSelectedProfile(unsigned int vdsl2Profile)
{
  uint8 res = 0;

  switch(vdsl2Profile) {
    case kVdslProfile8a:
      res = 1;
      break;
    case kVdslProfile8b:
      res = 2;
      break;
    case kVdslProfile8c:
      res = 3;
      break;
    case kVdslProfile8d:
      res = 4;
      break;
    case kVdslProfile12a:
      res = 5;
      break;
    case kVdslProfile12b:
      res = 6;
      break;
    case kVdslProfile17a:
      res = 7;
      break;
    case kVdslProfile30a:
      res = 8;
      break;
    case kVdslProfile35b:
      res = 9;
      break;
    case kGfastProfile106a:
      res = 1;
      break;
    case kGfastProfile212a:
      res = 2;
      break;
    case kGfastProfile106b:
      res = 3;
      break;
    case kGfastProfile106c:
      res = 4;
      break;
    case kGfastProfile212c:
      res = 5;
      break;
    case kMgfastProfileP424a:
      res = BCM_SEL_MGFAST_PROFILE_P424a;
      break;
    case kMgfastProfileP424d:
      res = BCM_SEL_MGFAST_PROFILE_P424d;
      break;
    case kMgfastProfileQ424c:
      res = BCM_SEL_MGFAST_PROFILE_Q424a;
      break;
    case kMgfastProfileQ424d:
      res = BCM_SEL_MGFAST_PROFILE_Q424d;
      break;
  }
  return res;
}


/* Copy from SoftDsl.h to avoid user defined types conflict with other header files */
#ifdef __GNUC__
#define	ALIGN_PACKED __attribute__ ((packed))
#else
#define	ALIGN_PACKED
#endif

#if defined(WINNT) || defined(LINUX_DRIVER) || defined(__KERNEL__)
#define DIAG_OR_HOST_BUILD
#endif

typedef struct UDenNum16 UDenNum16;
struct UDenNum16
{
  unsigned short num; /*numerator*/
  unsigned short denom; /*denominator*/
};

typedef struct FramerDeframerOptions FramerDeframerOptions;
struct FramerDeframerOptions
{
  UDenNum16             S;          /* S== number of PMD symbols over which the FEC data frame spans (=1 for G.dmt fast path, <=1 for ADSL2 fast path) */
  unsigned short        D;          /* interleaving depth: =1 for fast path */
  unsigned short        N;          /* RS codeword size*/
  unsigned short        L16;
  union {
    struct {
      unsigned short        B;          /* nominal total of each bearer's octets per Mux Data Frame (Slightly redundant)*/
      unsigned short        U;          /* VDSL2: Number of OH sub-frames per OH frame */
      unsigned char         I;          /* VDSL2: Interleaver block length */
    } ALIGN_PACKED;
    struct { /* Gfast */
      unsigned short        Lrmc;       /* RMC bits in RC symbol */
      unsigned short        Ldoi;       /* L in DOI; Bdoi = floor(Ldoi/8) */
      unsigned char         Rrmc;       /* R in RMC symbol */
    } ALIGN_PACKED;
  };
  unsigned char         R;          /* RS codeword overhead bytes */
  unsigned char         M;          /* ADSL2 only. Number of Mux frames per FEC Data frame.*/
  union {
    struct {
      unsigned char         T;          /* ADSL2: Number of Mux frames per sync octet*/
                                        /* VDSL2: Number of Mux data frames in an OH sub-frame */
      unsigned char         G;          /* VDSL2: Notional number of OH bytes in an OH sub-frame - actual number of bytes in any sub-frame may 1 be greater than this */
      unsigned char         F;          /* VDSL2: Number of OH frames in an OH superframe */
    } ALIGN_PACKED;
    struct { /* Gfast */
      unsigned char         Mf;         /* common for DS and US */
      unsigned char         Msf;        /* common for DS and US */
      unsigned char         Drmc;       /* RMC symbol offset  */
    } ALIGN_PACKED;
  };
  unsigned char         codingType; /* codingType associated with this option */
  union {
    struct {
      unsigned char         fireEnabled;/* bitmap flagging fire support for this direction and/or for reverse direction
                                     * No more used - Kept in the structure for backward compatibility with Diags */
      unsigned char         fireRxQueueOld;/* length of the retransmission queue in Rx direction */
    } ALIGN_PACKED;
    struct { /* Gfast */
      unsigned char         MNDSNOI;    /* Min Data Symbols in NOI */
      unsigned char         ackWindowShift;
    } ALIGN_PACKED;
  };
#ifdef _MSC_VER
#pragma pack(pop)
#endif
  unsigned char         tpsTcOptions; /* result of the pmsTcNegotiation */
  unsigned char         delay;      /* actual delay incurred on this latency path in [ms] */
  unsigned char         INP;        /* actual INP guaranteed on this latency path in [symbol/2] */
  unsigned char         b1;
  unsigned char         ovhType;
  unsigned char         path;
  unsigned char         ahifChanId[2];
  unsigned char         tmType[2];
  unsigned char         fireTxQueue;/* length of the retransmission queue in Tx direction */
  unsigned char         phyRrrcBits;/* number of bits in the retransmission return channel */
  unsigned char         ginpFraming;/* 0 if G.inp is not active, 1 or 2 (framingType) if active
                                     * W = ginpFraming-1 */
  unsigned char         INPrein;    /* actual INP guaranteed on this latency path against rein noise in [symbol/2] */
  unsigned char         Q;          /* G.Inp: Number of RS CW per DTU (PhyR & G.Inp) */
  unsigned char         V;          /* G.Inp: Number of padding bytes per DTU */
  unsigned char         QrxBuffer;  /* G.Inp: Number of DTUs of the rx queue effectively bufferized */
  unsigned int          ETR_kbps;   /* G.Inp: ETR - Expected throughput in kbits/sec */
  unsigned short        INPshine;   /* G.Inp: For G.Inp the INP is 2 bytes and supports values up to 204.7 0.1 symbols which won't fit in existing INP. We use Q1 format
                                       to be the same as the existing INP and the driver will limit to 204.7 when reporting to the CO */
  unsigned int          L;
#if defined(GFAST_SUPPORT) || defined(DIAG_OR_HOST_BUILD)
  unsigned int          maxMemory;  /* Maximum available nominal memory at NE for the current lp */
  unsigned int          ndr;        /* Net Data Rate */
  unsigned short        Ldr;        /* Number of bearer bits per symbol during RMC symbol */
  unsigned char         Nret;       /* Maximum number of retransmission - fireRxQueue = Nret * fireTxQueue */
  unsigned int          etru;
  unsigned int          Lmax;       /* Maximum possible L (SRA upshift) */
  unsigned int          Lmin;       /* Minimum possible L (SRA downshift) */
  unsigned int          ETRminEoc;  /* ETR_min_eoc, see table 9-21 in corrigendum 1 */
#if defined(GFAST_DTA_SUPPORTED) || defined(DIAG_OR_HOST_BUILD)
  unsigned char         dtaMmax;    /* Mds Max when dta is enabled */
#endif
  unsigned short        fireRxQueue;/* length of the retransmission queue in Rx direction */
#endif
};

void LineUpdateBearerLatencyInfo(unsigned char lineId, FramerDeframerOptions *param, int dir)
{
  LineMgr *lineMgr = getLineMgr(lineId);
  LineStatus *pLineStatus = &lineMgr->lineStatus;
//  LinkStatus *pLinkStatus = (US_DIRECTION == dir)? &pLineStatus->linkStatus[RX_DIRECTION]: &pLineStatus->linkStatus[TX_DIRECTION];
  LinkStatus *pLinkStatus = &pLineStatus->linkStatus[dir];
  BearerLatencyInfo *pBearerLatencyInfo = &pLinkStatus->bearerLatencyInfo[0];
  
  memset((void *)pBearerLatencyInfo, 0, sizeof(BearerLatencyInfo));
  
  pBearerLatencyInfo->phyLatencyIndex = param->path;
  
  pBearerLatencyInfo->actualBitRate = param->ndr;
  pBearerLatencyInfo->etru          = param->etru;
  
  pBearerLatencyInfo->L             = param->L;
  pBearerLatencyInfo->D             = param->D;
  pBearerLatencyInfo->B             = param->B;
  pBearerLatencyInfo->N             = param->N;
  pBearerLatencyInfo->R             = param->R;
  pBearerLatencyInfo->I             = param->I;
  pBearerLatencyInfo->M             = param->M;
  
  pBearerLatencyInfo->T             = param->T;
  pBearerLatencyInfo->G             = param->G;
  pBearerLatencyInfo->U             = param->U;
  
  pBearerLatencyInfo->Q             = param->Q;
  pBearerLatencyInfo->V             = param->V;
  pBearerLatencyInfo->Nret          = param->Nret;
  pBearerLatencyInfo->ginpFraming   = param->ginpFraming;
  pBearerLatencyInfo->rtxTxQueue    = param->fireTxQueue;
  pBearerLatencyInfo->phyRrrcBits   = param->phyRrrcBits;
  pBearerLatencyInfo->rtxRxQueue    = param->QrxBuffer;
  
  pBearerLatencyInfo->INP           = param->INP;
  pBearerLatencyInfo->tpsTcOptions  = param->tpsTcOptions;
  pBearerLatencyInfo->INPrein       = param->INPrein;
  pBearerLatencyInfo->delay         = param->delay;
  pBearerLatencyInfo->codingType    = param->codingType;
  
  pBearerLatencyInfo->maxMemory     = param->maxMemory;
  pBearerLatencyInfo->Lmax          = param->Lmax;
  pBearerLatencyInfo->Lmin          = param->Lmax;
  
  pBearerLatencyInfo->gdr           = param->ndr;
  pBearerLatencyInfo->Ldr           = param->Ldr;
  pBearerLatencyInfo->dtaMmax       = param->M;
  pBearerLatencyInfo->actMmax       = param->M;
}
static void LineUpdateLinkStatusRx(LinkStatus *pLinkStatus, xdslPhysEntry *pXdslPhys, int16 outputPwrUS)
{
  pLinkStatus->snrMargin    = (int16)pXdslPhys->adslCurrSnrMgn;
  pLinkStatus->outputPower  = outputPwrUS;
#if 0
  uint16 electricalLength;      /* kl0: attenuation at 1 MHz */
  uint32 actualLineBitRate;     /* ATM net data rate of all bearers plus all
                                 * overhead information */
#endif
  pLinkStatus->attainableBitRate  = (uint32)pXdslPhys->adslCurrAttainableRate/1000;
  pLinkStatus->attEtr             = pXdslPhys->attnETR;
#if 0
  uint32 eftr;                  /* Error-Free Throughput Rate (G998.4 only) */
  uint32 aggAchNDR;             /* Aggregate Achievable NDR (G998.4 only):
                                 * maximum aggregate rate achievable by the
                                 * transceiver */
#endif
  pLinkStatus->attNDRmethod       = pXdslPhys->attnDrMethod;
  pLinkStatus->attNDRactDelay     = pXdslPhys->attnDrDelay;
  pLinkStatus->attNDRactInp       = pXdslPhys->attnDrInp;
#if 0
  uint8  attNDRactInpRein;      /* Actual INP against REIN noise used in the
                                 * computation of ATTNDR */
  uint8  MSGc;                  /* nb octets in message-based portion of the
                                 * overhead framing structure (ADSL2 only) */
  uint8  MSGlp;                 /* ID of latency path carrying the
                                 * message-based overhead */
#endif
  pLinkStatus->snrMode            = (uint8)pXdslPhys->SNRmode;
#if 0
  uint8  raMode;                /* The 4 LSB give the actual RA mode:
                                 * 1: fixed rate mode
                                 * 2: adaptive rate mode at init
                                 * 3: dynamic rate adaptation
                                 * 4: SOS enabled  */
  uint8  rtxMode;               /* Bits 2:0 give the actual RTX mode
                                 * according to G997.1 encoding:
                                 * 1: RTX in use
                                 * 2: RTX not in use, due to RTX_MODE=FORBIDDEN
                                 * 3: RTX not in use, not supported by XTU-C
                                 * 4: RTX not in use, not supported by XTU-R
                                 * 5: RTX not in use, not supported by both XTUs */
  uint8  riPolicy;              /* 0: Default:
                                 * 1: Number of consecutive SES that triggers the Showtime exit transition  */
  int16  loopDelay;             /* Current loop group delay, only measured in VDSL
                                 * US: delay as measured on RMSG1
                                 *      = actual loop delay
                                 * DS: delay as measured on R-P-TRAINING1
                                 *     = residual after application of timing advance
                                 * expressed in samples at 35.328MHz */
  uint32 currentFailures;       /* current failure state */
  uint32 changedFailures;       /* set of failures changed since last read */
#endif
  pLinkStatus->snrMarginROC     = (int16)pXdslPhys->snrmRoc;
#if 0
  uint16 rateLossROC;           /* Rate loss due to ROC */
  uint32 attainableGDR;         /* Gamma Data Rate */
  uint32 andEftr;               /* All-NOI Data Error Free Throughput Rate */
  uint16 autoSensingMetric;     /* metric used in GFAST->VDSL automoding, 0xffff if NA */
  uint8  reserved1[7];
  uint8  ttr;                   /* NOI duration */
  uint8  ta;                    /* NOI-DOI gap */
  uint8  tBudget;               /* max allowed transmission time */
#endif
}

static void LineUpdateLinkStatusTx(LinkStatus *pLinkStatus, xdslFullPhysEntry *pXdslPhys, int16 outputPwrDS)
{
  pLinkStatus->snrMargin    = (int16)pXdslPhys->adslCurrSnrMgn;
  pLinkStatus->outputPower  = outputPwrDS;
#if 0
  uint16 electricalLength;      /* kl0: attenuation at 1 MHz */
  uint32 actualLineBitRate;     /* ATM net data rate of all bearers plus all
                                 * overhead information */
#endif
  pLinkStatus->attainableBitRate  = (uint32)pXdslPhys->adslCurrAttainableRate/1000;
  pLinkStatus->attEtr             = pXdslPhys->attnETR;
#if 0
  uint32 eftr;                  /* Error-Free Throughput Rate (G998.4 only) */
  uint32 aggAchNDR;             /* Aggregate Achievable NDR (G998.4 only):
                                 * maximum aggregate rate achievable by the
                                 * transceiver */
#endif
  pLinkStatus->attNDRmethod       = pXdslPhys->attnDrMethod;
  pLinkStatus->attNDRactDelay     = pXdslPhys->attnDrDelay;
  pLinkStatus->attNDRactInp       = pXdslPhys->attnDrInp;
#if 0
  uint8  attNDRactInpRein;      /* Actual INP against REIN noise used in the
                                 * computation of ATTNDR */
  uint8  MSGc;                  /* nb octets in message-based portion of the
                                 * overhead framing structure (ADSL2 only) */
  uint8  MSGlp;                 /* ID of latency path carrying the
                                 * message-based overhead */
#endif
  pLinkStatus->snrMode            = (uint8)pXdslPhys->SNRmode;
#if 0
  uint8  raMode;                /* The 4 LSB give the actual RA mode:
                                 * 1: fixed rate mode
                                 * 2: adaptive rate mode at init
                                 * 3: dynamic rate adaptation
                                 * 4: SOS enabled  */
  uint8  rtxMode;               /* Bits 2:0 give the actual RTX mode
                                 * according to G997.1 encoding:
                                 * 1: RTX in use
                                 * 2: RTX not in use, due to RTX_MODE=FORBIDDEN
                                 * 3: RTX not in use, not supported by XTU-C
                                 * 4: RTX not in use, not supported by XTU-R
                                 * 5: RTX not in use, not supported by both XTUs */
  uint8  riPolicy;              /* 0: Default:
                                 * 1: Number of consecutive SES that triggers the Showtime exit transition  */
  int16  loopDelay;             /* Current loop group delay, only measured in VDSL
                                 * US: delay as measured on RMSG1
                                 *      = actual loop delay
                                 * DS: delay as measured on R-P-TRAINING1
                                 *     = residual after application of timing advance
                                 * expressed in samples at 35.328MHz */
  uint32 currentFailures;       /* current failure state */
  uint32 changedFailures;       /* set of failures changed since last read */
#endif
  pLinkStatus->snrMarginROC     = (int16)pXdslPhys->snrmRoc;
#if 0
  uint16 rateLossROC;           /* Rate loss due to ROC */
  uint32 attainableGDR;         /* Gamma Data Rate */
  uint32 andEftr;               /* All-NOI Data Error Free Throughput Rate */
  uint16 autoSensingMetric;     /* metric used in GFAST->VDSL automoding, 0xffff if NA */
  uint8  reserved1[7];
  uint8  ttr;                   /* NOI duration */
  uint8  ta;                    /* NOI-DOI gap */
  uint8  tBudget;               /* max allowed transmission time */
#endif
}

/* BasicTypes.h */
#define SPECIAL_SNRMARGIN_VALUE (-512)
#define SPECIAL_ATTENUATION_VALUE (1023)

#define SPECIAL_ACTATP_VALUE (-512)

static void *LineGetStatus(LineMgr *me,
                           void    *msgp,
                           uint16  *sizep,
                           int     *resultp, uint16 hmiVersion)
{
  LineGetStatus_MsgRep *rep;
  LineGetStatus_MsgReq *req = msgp;
  uint32      statusControl = req->data; //.statusControl;
  LineStatus *lineStatusp   = &lineMgrStatus(me);
  int         lineStateStart, lineStateEnd;
  LinkStatus  *pLinkStatus;
  int         i;
  adslMibInfo *pMibInfo;
  long        mibLen;

  if (*sizep < sizeof(uint32))
    statusControl = 0;

  rep = releaseRequestAllocateReply(msgp,sizeof(LineGetStatus_MsgRep));

  if(statusControl) {
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetStatus: statusControl = 0x%08X", statusControl);
  }
  
  do
  {
    lineStateStart = me->lineState;
    for (i=0; i<sizeof(int2ExtStateMap)/sizeof(Int2ExtStateMap); i++)
    {
      if (int2ExtStateMap[i].intState == lineStateStart)
      {
        lineStatusp->state = int2ExtStateMap[i].extState;
        break;
      }
    }
    /* if init has not started, we do not update the status */
    if ((lineStateStart & (LINE_TRAINING|LINE_SHOWTIME))||(lineStateStart==LINE_IDLE_DM_COMPLETE)) {
      if (lineStateStart & LINE_SHOWTIME)
        lineStatusp->subState = 1;
      lineStatusp->farendDetected = TRUE;

      mibLen    = sizeof(adslMibInfo);
      pMibInfo  = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);
      lineStatusp->selectedProtocol = GetSelectedProtol(pMibInfo->adslConnection.modType,
                                        pMibInfo->xdslInfo.xdslMode >> kXdslModeAnnexShift);
      if(XdslDrvIsGfastMod(me->lineId))
        lineStatusp->gfastProfile = GetSelectedProfile(pMibInfo->vdslInfo[0].vdsl2Profile);
      else if(kVdslModVdsl2 == pMibInfo->adslConnection.modType)
        lineStatusp->vdslProfile = GetSelectedProfile(pMibInfo->vdslInfo[0].vdsl2Profile);

      lineStatusp->hsMds  = pMibInfo->xdslInfo.dirInfo[DS_DIRECTION].lpInfo[0].M;
      lineStatusp->hsMus  = pMibInfo->xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].M;

      /* RX path - index 0 */
      pLinkStatus = &lineStatusp->linkStatus[RX_DIRECTION];
      LineUpdateLinkStatusRx(pLinkStatus, &pMibInfo->xdslPhys, (int16)pMibInfo->xdslAtucPhys.adslCurrOutputPwr);
      /* overwrite loop attenuation / signal attenuation / SNR margin in perBandStatus */
      lineStatusp->perBandStatus.LATN[0]  = pMibInfo->perbandDataDs[0].adslCurrAtn;
      lineStatusp->perBandStatus.SATN[0]  = pMibInfo->perbandDataDs[0].adslSignalAttn;
      lineStatusp->perBandStatus.SNRM[0]  = pMibInfo->xdslAtucPhys.adslCurrSnrMgn;
      lineStatusp->perBandStatus.TXPWR[0] = pLinkStatus->outputPower;
      lineStatusp->perBandStatus.ELE[0] = (pMibInfo->xdslPhys.UPBOkle+5)/10;
      for (i = 1; i < 5; i++) {
        lineStatusp->perBandStatus.LATN[i]  = 1023;
        lineStatusp->perBandStatus.SATN[i]  = 1023;
        lineStatusp->perBandStatus.SNRM[i]  = -512;
        lineStatusp->perBandStatus.TXPWR[i] = -1280;
        lineStatusp->perBandStatus.ELE[i] = -512;
      }

      pLinkStatus->electricalLength = (pMibInfo->xdslPhys.UPBOkle+5)/10;
      pLinkStatus->actualLineBitRate = pMibInfo->xdslInfo.dirInfo[RX_DIRECTION].aggrDataRate;
      pLinkStatus->eftr = pMibInfo->adslStat.ginpStat.cntDS.minEFTR;
      pLinkStatus->andEftr = pMibInfo->adslStat.ginpStat.cntDS.minEFTR;
      pLinkStatus->aggAchNDR = pMibInfo->adslStat.ginpStat.cntDS.minEFTR + pMibInfo->adslStat.ginpStat.cntUS.minEFTR;
      pLinkStatus->attainableGDR = pMibInfo->xdslInfo.dirInfo[RX_DIRECTION].aggrDataRate;

      pLinkStatus->bearerLatencyInfo[0].actualBitRate = pMibInfo->xdslInfo.dirInfo[RX_DIRECTION].lpInfo[0].dataRate;
      pLinkStatus->bearerLatencyInfo[1].actualBitRate = pMibInfo->xdslInfo.dirInfo[RX_DIRECTION].lpInfo[1].dataRate;
      pLinkStatus->bearerLatencyInfo[0].idr = (pMibInfo->gfastDta.dtaFlags != 0) ? 
        pMibInfo->gfastDta.usCurRate :
        pMibInfo->xdslInfo.dirInfo[RX_DIRECTION].lpInfo[0].dataRate;
      
      /* TX path - index 1 */
      pLinkStatus = &lineStatusp->linkStatus[TX_DIRECTION];
      LineUpdateLinkStatusTx(pLinkStatus, &pMibInfo->xdslAtucPhys, (int16)pMibInfo->xdslPhys.adslCurrOutputPwr);
      /* overwrite loop attenuation / signal attenuation / SNR margin in perBandStatus */
      lineStatusp->perBandStatus.LATN[5]  = pMibInfo->perbandDataUs[0].adslCurrAtn;
      lineStatusp->perBandStatus.SATN[5]  = pMibInfo->perbandDataUs[0].adslSignalAttn;
      lineStatusp->perBandStatus.SNRM[5]  = pMibInfo->xdslPhys.adslCurrSnrMgn;
      lineStatusp->perBandStatus.TXPWR[5] = pLinkStatus->outputPower;
      lineStatusp->perBandStatus.ELE[5]   = (pMibInfo->xdslPhys.UPBOkleCpe+5)/10;
      for (i = 6; i < 9; i++) {
          lineStatusp->perBandStatus.LATN[i]  = 1023;
          lineStatusp->perBandStatus.SATN[i]  = 1023;
          lineStatusp->perBandStatus.SNRM[i]  = -512;
          lineStatusp->perBandStatus.TXPWR[i] = -1280;
          lineStatusp->perBandStatus.ELE[i]   = -512;
      }
      lineStatusp->perBandStatus.LATN[9]  = 0;
      lineStatusp->perBandStatus.SATN[9]  = 0;
      lineStatusp->perBandStatus.SNRM[9]  = 0;
      lineStatusp->perBandStatus.TXPWR[9] = 0;
      lineStatusp->perBandStatus.ELE[9]   = 0;

      pLinkStatus->electricalLength = (pMibInfo->xdslPhys.UPBOkleCpe+5)/10;
      pLinkStatus->actualLineBitRate = pMibInfo->xdslInfo.dirInfo[TX_DIRECTION].aggrDataRate;
      pLinkStatus->eftr = pMibInfo->adslStat.ginpStat.cntUS.minEFTR;
      pLinkStatus->andEftr = pMibInfo->adslStat.ginpStat.cntUS.minEFTR;
      pLinkStatus->aggAchNDR = pMibInfo->adslStat.ginpStat.cntDS.minEFTR + pMibInfo->adslStat.ginpStat.cntUS.minEFTR;
      pLinkStatus->attainableGDR = pMibInfo->xdslInfo.dirInfo[TX_DIRECTION].aggrDataRate;

      pLinkStatus->bearerLatencyInfo[0].actualBitRate = pMibInfo->xdslInfo.dirInfo[TX_DIRECTION].lpInfo[0].dataRate;
      pLinkStatus->bearerLatencyInfo[1].actualBitRate = pMibInfo->xdslInfo.dirInfo[TX_DIRECTION].lpInfo[1].dataRate;
      pLinkStatus->bearerLatencyInfo[0].idr = (pMibInfo->gfastDta.dtaFlags != 0) ? 
        pMibInfo->gfastDta.dsCurRate :
        pMibInfo->xdslInfo.dirInfo[TX_DIRECTION].lpInfo[0].dataRate;

      lineStatusp->lastRetrainInfo.uptime  = pMibInfo->adslPerfData.perfSinceShowTime.adslAS;
      lineStatusp->lastRetrainInfo.lastTransmittedState  = pMibInfo->adslDiag.ldLastStateUS;
      lineStatusp->lastRetrainInfo.lastRetrainCause = pMibInfo->adslPerfData.lastRetrainReason;

      if(me->gfastConfig.phyConfig.forceElectricalLength < 0) {
        if(-1 == me->gfastConfig.phyConfig.forceElectricalLength)
          lineStatusp->actualElectricalLength = MAX(lineStatusp->linkStatus[RX_DIRECTION].electricalLength, lineStatusp->linkStatus[TX_DIRECTION].electricalLength);
        else if(-2 == me->gfastConfig.phyConfig.forceElectricalLength)
          lineStatusp->actualElectricalLength = MIN(lineStatusp->linkStatus[RX_DIRECTION].electricalLength, lineStatusp->linkStatus[TX_DIRECTION].electricalLength);
        else if(-3 == me->gfastConfig.phyConfig.forceElectricalLength)
          lineStatusp->actualElectricalLength = lineStatusp->linkStatus[RX_DIRECTION].electricalLength;
        else if(-4 == me->gfastConfig.phyConfig.forceElectricalLength)
          lineStatusp->actualElectricalLength = lineStatusp->linkStatus[TX_DIRECTION].electricalLength;
        else
          lineStatusp->actualElectricalLength = me->gfastConfig.phyConfig.forceElectricalLength;  /* Should never go here */
      }
      else {
        if(1 == me->gfastConfig.phyConfig.forceElectricalLength)
          lineStatusp->actualElectricalLength = lineStatusp->linkStatus[TX_DIRECTION].electricalLength;
        else if(2 == me->gfastConfig.phyConfig.forceElectricalLength)
          lineStatusp->actualElectricalLength = lineStatusp->linkStatus[RX_DIRECTION].electricalLength;
        else if(3 == me->gfastConfig.phyConfig.forceElectricalLength)
          lineStatusp->actualElectricalLength = MIN(lineStatusp->linkStatus[RX_DIRECTION].electricalLength, lineStatusp->linkStatus[TX_DIRECTION].electricalLength);
        else
          lineStatusp->actualElectricalLength = me->gfastConfig.phyConfig.forceElectricalLength;  /* Should never go here */
      }
    }
    memcpy(&rep->data,lineStatusp,sizeof(LineStatus));
    for(i=0; i < EVENT_TIME_LOG_LENGTH; i++) {
      if(me->eventTimeLogTimeStamp[i] != 0) {
        rep->data.lastRetrainInfo.eventTimeLog[i] = rep->data.lastRetrainInfo.uptime - me->eventTimeLogTimeStamp[i];
        me->eventTimeLogTimeStamp[i] = 0;
      }
    }
    lineStateEnd = me->lineState;
  }
  while(lineStateStart != lineStateEnd);

  *sizep = sizeof(LineGetStatus_MsgRep);
  *resultp = HPI_SUCCESS;

  return rep;
}

static void *LineGetFeatures(LineMgr *me, void *msgp, uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  char        oidStr[]  = { kOidAdslPrivate, kOidAdslPrivGetLineFeatures};
  long        dataLen   = 0;
  LineGetFeatures_MsgRep *rep = releaseRequestAllocateReply(msgp,sizeof(LineGetFeatures_MsgRep));

  LineFeatureInfos     *pLineFeatureInfos = (LineFeatureInfos *) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetFeatures: dataLen=%d", dataLen);
  *sizep = sizeof(*rep);
  memset((void *)rep, 0, *sizep);
  rep->data.supportedStandards[0].bitMap = pLineFeatureInfos[0].supportedStandards;
  rep->data.supportedStandards[1].bitMap = pLineFeatureInfos[1].supportedStandards;
  rep->data.supportedOptions[0].gfast = pLineFeatureInfos[0].supportedOptions;
  rep->data.supportedOptions[1].gfast = pLineFeatureInfos[1].supportedOptions;

  rep->data.enabledStandards.bitMap = pLineFeatureInfos[0].enabledStandards;
  
  rep->data.enabledOptions[0].gfast = pLineFeatureInfos[0].enabledOptions;
  rep->data.enabledOptions[1].gfast = pLineFeatureInfos[1].enabledOptions;
  
  *resultp = HPI_SUCCESS;
  
  return rep;
}

static void *LineGetPtmCounters(LineMgr *me,
                                void    *msgp,
                                uint16  *sizep,
                                int     *resultp, uint16 hmiVersion)
{
  LineGetPtmCounters_MsgRep *rep;
  PtmCounters               *pPtmCounters;
  adslMibInfo               *pMibInfo;
  long                      mibLen;

  rep = releaseRequestAllocateReply(msgp,sizeof(*rep));

  mibLen   = sizeof(adslMibInfo);
  pMibInfo = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);

  pPtmCounters = &rep->data;
  memset(pPtmCounters, 0, sizeof(PtmCounters));
  BcmCoreDpcSyncEnter(SYNC_RX);
  pPtmCounters->ptmCounters[0][0].ptm_rx_packet_count = pMibInfo->atmStat2lp[0].rcvStat.cntCellData;
  pPtmCounters->ptmCounters[0][0].ptm_tx_packet_count = pMibInfo->atmStat2lp[0].xmtStat.cntCellData;
  BcmCoreDpcSyncExit(SYNC_RX);

  *sizep = sizeof(*rep);
  *resultp = HPI_SUCCESS;

  return rep;
}

static void CopyDerivedSecCounters(DerivedSecCounters *pDerivedCounters, adslPerfCounters *pPerfCounters)
{
  pDerivedCounters->FECS  = pPerfCounters->adslFECs;   /* FEC second */
  pDerivedCounters->LOSS  = pPerfCounters->adslLOSS;   /* LOS second */
  pDerivedCounters->ES    = pPerfCounters->adslESs;     /* errored second: CRC-8 error or LOS or SEF (or LPR) */
  pDerivedCounters->SES   = pPerfCounters->adslSES;    /* severely errored second : idem as above but CRC-8 > 18 */
  pDerivedCounters->UAS   = pPerfCounters->adslUAS;    /* unavailable second : more then 10 consecutive SES */
  pDerivedCounters->AS    = pPerfCounters->adslAS;     /* available second = elapsed time - UAS_L */
//  pDerivedCounters->LOFS  = pPerfCounters->xxx;   /* LOF second */
  pDerivedCounters->LPRS  = pPerfCounters->adslLprs;   /* LOL second in NE instance, LPR second in FE instance */
  pDerivedCounters->LORS  = pPerfCounters->xdslLORS;   /* LOR seconds - G.fast only */
}

static void CopyAdslAnomCounters(AdslAnomNeFeCounters *pAdslAnomCounters, adslConnectionDataStat *pConnDataStat, rtxCounters *pRtxCounters)
{
  pAdslAnomCounters->CV                     = pConnDataStat->cntSFErr;  /* CRC-8 count */
  pAdslAnomCounters->FEC                    = pConnDataStat->cntRSCor;  /* FEC corrections */
  pAdslAnomCounters->uncorrectableCodeword  = pConnDataStat->cntRSUncor;  /* codewords that could not be corrected */
  pAdslAnomCounters->rtxCW                  = pRtxCounters->rtx_tx;     /* number of codewords retransmitted */
  pAdslAnomCounters->rtxCorrectedCW         = pRtxCounters->rtx_c;      /* number of CW corrected through retransmission */
  pAdslAnomCounters->rtxUncorrectedCW       = pRtxCounters->rtx_uc;     /* number of CW left uncorrected after retransmission */
}

static void CopyLineCountersDifference(LineCounters *pDest, LineCounters *pCur, LineCounters *pPrev)
{
  memset(pDest, 0, sizeof(LineCounters));
  /* DerivedSecCounters */
  pDest->derivedCounts[0].FECS = pCur->derivedCounts[0].FECS  - pPrev->derivedCounts[0].FECS;
  pDest->derivedCounts[0].LOSS = pCur->derivedCounts[0].LOSS  - pPrev->derivedCounts[0].LOSS;
  pDest->derivedCounts[0].ES   = pCur->derivedCounts[0].ES    - pPrev->derivedCounts[0].ES;
  pDest->derivedCounts[0].SES  = pCur->derivedCounts[0].SES   - pPrev->derivedCounts[0].SES;
  pDest->derivedCounts[0].UAS  = pCur->derivedCounts[0].UAS   - pPrev->derivedCounts[0].UAS;
  pDest->derivedCounts[0].AS   = pCur->derivedCounts[0].AS    - pPrev->derivedCounts[0].AS;
  pDest->derivedCounts[0].LOFS = pCur->derivedCounts[0].LOFS  - pPrev->derivedCounts[0].LOFS;
  pDest->derivedCounts[0].LPRS = pCur->derivedCounts[0].LPRS  - pPrev->derivedCounts[0].LPRS;
  pDest->derivedCounts[0].LORS = pCur->derivedCounts[0].LORS  - pPrev->derivedCounts[0].LORS;

  pDest->derivedCounts[1].FECS = pCur->derivedCounts[1].FECS  - pPrev->derivedCounts[1].FECS;
  pDest->derivedCounts[1].LOSS = pCur->derivedCounts[1].LOSS  - pPrev->derivedCounts[1].LOSS;
  pDest->derivedCounts[1].ES   = pCur->derivedCounts[1].ES    - pPrev->derivedCounts[1].ES;
  pDest->derivedCounts[1].SES  = pCur->derivedCounts[1].SES   - pPrev->derivedCounts[1].SES;
  pDest->derivedCounts[1].UAS  = pCur->derivedCounts[1].UAS   - pPrev->derivedCounts[1].UAS;
  pDest->derivedCounts[1].AS   = pCur->derivedCounts[1].AS    - pPrev->derivedCounts[1].AS;
  pDest->derivedCounts[1].LOFS = pCur->derivedCounts[1].LOFS  - pPrev->derivedCounts[1].LOFS;
  pDest->derivedCounts[1].LPRS = pCur->derivedCounts[1].LPRS  - pPrev->derivedCounts[1].LPRS;
  pDest->derivedCounts[1].LORS = pCur->derivedCounts[1].LORS  - pPrev->derivedCounts[1].LORS;

  /* AdslAnomCounters */
  pDest->adslAnomalyCounts[0].ne.CV                    = pCur->adslAnomalyCounts[0].ne.CV                     - pPrev->adslAnomalyCounts[0].ne.CV;
  pDest->adslAnomalyCounts[0].ne.FEC                   = pCur->adslAnomalyCounts[0].ne.FEC                    - pPrev->adslAnomalyCounts[0].ne.FEC;
  pDest->adslAnomalyCounts[0].ne.uncorrectableCodeword = pCur->adslAnomalyCounts[0].ne.uncorrectableCodeword  - pPrev->adslAnomalyCounts[0].ne.uncorrectableCodeword;
  pDest->adslAnomalyCounts[0].ne.rtxCW                 = pCur->adslAnomalyCounts[0].ne.rtxCW                  - pPrev->adslAnomalyCounts[0].ne.rtxCW;
  pDest->adslAnomalyCounts[0].ne.rtxCorrectedCW        = pCur->adslAnomalyCounts[0].ne.rtxCorrectedCW         - pPrev->adslAnomalyCounts[0].ne.rtxCorrectedCW;
  pDest->adslAnomalyCounts[0].ne.rtxUncorrectedCW      = pCur->adslAnomalyCounts[0].ne.rtxUncorrectedCW       - pPrev->adslAnomalyCounts[0].ne.rtxUncorrectedCW;

  pDest->adslAnomalyCounts[0].fe.CV                    = pCur->adslAnomalyCounts[0].fe.CV                     - pPrev->adslAnomalyCounts[0].fe.CV;
  pDest->adslAnomalyCounts[0].fe.FEC                   = pCur->adslAnomalyCounts[0].fe.FEC                    - pPrev->adslAnomalyCounts[0].fe.FEC;
  pDest->adslAnomalyCounts[0].fe.uncorrectableCodeword = pCur->adslAnomalyCounts[0].fe.uncorrectableCodeword  - pPrev->adslAnomalyCounts[0].fe.uncorrectableCodeword;
  pDest->adslAnomalyCounts[0].fe.rtxCW                 = pCur->adslAnomalyCounts[0].fe.rtxCW                  - pPrev->adslAnomalyCounts[0].fe.rtxCW;
  pDest->adslAnomalyCounts[0].fe.rtxCorrectedCW        = pCur->adslAnomalyCounts[0].fe.rtxCorrectedCW         - pPrev->adslAnomalyCounts[0].fe.rtxCorrectedCW;
  pDest->adslAnomalyCounts[0].fe.rtxUncorrectedCW      = pCur->adslAnomalyCounts[0].fe.rtxUncorrectedCW       - pPrev->adslAnomalyCounts[0].fe.rtxUncorrectedCW;

  /* HmiPerfCounters */
  pDest->perfCounts[0].ptm.p0.ptm_rx_packet_count = pCur->perfCounts[0].ptm.p0.ptm_rx_packet_count - pPrev->perfCounts[0].ptm.p0.ptm_rx_packet_count;
  pDest->perfCounts[0].ptm.p0.ptm_tx_packet_count = pCur->perfCounts[0].ptm.p0.ptm_tx_packet_count - pPrev->perfCounts[0].ptm.p0.ptm_tx_packet_count;

  pDest->failedFullInitCount      = pCur->failedFullInitCount   - pPrev->failedFullInitCount;
  pDest->fullInitCount            = pCur->fullInitCount         - pPrev->fullInitCount;
  pDest->reInitCount              = pCur->reInitCount           - pPrev->reInitCount;
  pDest->fastInitCounter          = pCur->fastInitCounter       - pPrev->fastInitCounter;
  pDest->failedFastInitCounter    = pCur->failedFastInitCounter - pPrev->failedFastInitCounter;

#if 0 /* Updated after CopyLineCountersDifference */
  pDest->elapsedTimeSinceLastRead = pCur->elapsedTimeSinceLastRead;
  pDest->suspectFlagNE            = pCur->suspectFlagNE;
  pDest->suspectFlagFE            = pCur->suspectFlagFE;
  pDest->upTime                   = pCur->upTime;
#endif

  /* GinpCounters */
  pDest->ginpCounters[NE].LEFTRS      = pCur->ginpCounters[NE].LEFTRS       - pPrev->ginpCounters[NE].LEFTRS;
  pDest->ginpCounters[NE].minEFTR     = pCur->ginpCounters[NE].minEFTR;
  pDest->ginpCounters[NE].errFreeBits = pCur->ginpCounters[NE].errFreeBits  - pPrev->ginpCounters[NE].errFreeBits;

  pDest->ginpCounters[FE].LEFTRS      = pCur->ginpCounters[FE].LEFTRS       - pPrev->ginpCounters[FE].LEFTRS;
  pDest->ginpCounters[FE].minEFTR     = pCur->ginpCounters[FE].minEFTR;
  pDest->ginpCounters[FE].errFreeBits = pCur->ginpCounters[FE].errFreeBits  - pPrev->ginpCounters[FE].errFreeBits;

  /* GfastCounters */
  pDest->gfastCounters[NE].ANDEFTRmin = pCur->gfastCounters[NE].ANDEFTRmin;
  pDest->gfastCounters[NE].ANDEFTRmax = pCur->gfastCounters[NE].ANDEFTRmax;
  pDest->gfastCounters[NE].ANDEFTRsum = pCur->gfastCounters[NE].ANDEFTRsum  - pPrev->gfastCounters[NE].ANDEFTRsum;
  pDest->gfastCounters[NE].ANDEFTRDS  = pCur->gfastCounters[NE].ANDEFTRDS   - pPrev->gfastCounters[NE].ANDEFTRDS;
  pDest->gfastCounters[NE].LANDEFTRS  = pCur->gfastCounters[NE].LANDEFTRS   - pPrev->gfastCounters[NE].LANDEFTRS;

  pDest->gfastCounters[FE].ANDEFTRmin = pCur->gfastCounters[FE].ANDEFTRmin;
  pDest->gfastCounters[FE].ANDEFTRmax = pCur->gfastCounters[FE].ANDEFTRmax;
  pDest->gfastCounters[FE].ANDEFTRsum = pCur->gfastCounters[FE].ANDEFTRsum  - pPrev->gfastCounters[FE].ANDEFTRsum;
  pDest->gfastCounters[FE].ANDEFTRDS  = pCur->gfastCounters[FE].ANDEFTRDS   - pPrev->gfastCounters[FE].ANDEFTRDS;
  pDest->gfastCounters[FE].LANDEFTRS  = pCur->gfastCounters[FE].LANDEFTRS   - pPrev->gfastCounters[FE].LANDEFTRS;

}

/* Copy from AlarmPerfMon.c */
void *LineGetTrafficCounter(LineMgr *me,
                            void    *msgp,
                            uint16  *sizep,
                            int     *resultp, uint16 hmiVersion)
{
  LineGetCounters_MsgRep *rep;
  LineCounters          *pCurCounters, lineCounters;
  adslMibInfo           *pMibInfo;
  long                  mibLen;
  rtxCounters           rxtCounters;

  rep = releaseRequestAllocateReply(msgp,sizeof(LineGetCounters_MsgRep));
  *sizep = sizeof(*rep);
  msgp = rep;
  *resultp = HPI_SUCCESS;

  mibLen   = sizeof(adslMibInfo);
  pMibInfo = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);
  pCurCounters = &lineCounters;
  memset(pCurCounters, 0, sizeof(LineCounters));

  BcmCoreDpcSyncEnter(SYNC_RX);

  /* DerivedSecCounters */
  CopyDerivedSecCounters(&pCurCounters->derivedCounts[NE], &pMibInfo->adslPerfData.perfTotal);
  CopyDerivedSecCounters(&pCurCounters->derivedCounts[FE], &pMibInfo->adslTxPerfTotal);
  pCurCounters->derivedCounts[FE].AS   = pCurCounters->derivedCounts[NE].AS;

  /* AdslAnomCounters */
  rxtCounters = pMibInfo->rtxCounterData.cntDS.perfTotal;
  rxtCounters.rtx_tx  = pMibInfo->rtxCounterData.cntUS.perfTotal.rtx_tx;
  CopyAdslAnomCounters(&pCurCounters->adslAnomalyCounts[0].ne, &pMibInfo->xdslStatSincePowerOn[0].rcvStat, &rxtCounters);
  rxtCounters = pMibInfo->rtxCounterData.cntUS.perfTotal;
  rxtCounters.rtx_tx  = pMibInfo->rtxCounterData.cntDS.perfTotal.rtx_tx;
  CopyAdslAnomCounters(&pCurCounters->adslAnomalyCounts[0].fe, &pMibInfo->xdslStatSincePowerOn[0].xmtStat, &rxtCounters);
  /* HmiPerfCounters: PTM counters */
  pCurCounters->perfCounts[0].ptm.p0.ptm_rx_packet_count = pMibInfo->atmStat2lp[0].rcvStat.cntCellData;
  pCurCounters->perfCounts[0].ptm.p0.ptm_tx_packet_count = pMibInfo->atmStat2lp[0].xmtStat.cntCellData;

  pCurCounters->failedFullInitCount  = pMibInfo->adslPerfData.failTotal.adslInitErr;
  pCurCounters->fullInitCount        = pMibInfo->adslPerfData.failTotal.adslRetr;
#if 0
  pCurCounters->reInitCount; /* incremented each time we start a new training within 20s after a showtime drop */
#endif
  pCurCounters->fastInitCounter        = pMibInfo->adslPerfData.failTotal.xdslFastRetr;
  pCurCounters->failedFastInitCounter  = pMibInfo->adslPerfData.failTotal.xdslFastInitErr;
  
  /* pCurCounters->reserved[3] */
  
  /* GinpCounters */
  pCurCounters->ginpCounters[NE].LEFTRS      = pMibInfo->adslStatSincePowerOn.ginpStat.cntUS.LEFTRS; /* N/A for G.fast */
  pCurCounters->ginpCounters[NE].minEFTR     = pMibInfo->adslStatSincePowerOn.ginpStat.cntUS.minEFTR;
  pCurCounters->ginpCounters[NE].errFreeBits = pMibInfo->adslStatSincePowerOn.ginpStat.cntUS.errFreeBits;
  pCurCounters->ginpCounters[FE].LEFTRS      = pMibInfo->adslStatSincePowerOn.ginpStat.cntDS.LEFTRS; /* N/A for G.fast */
  pCurCounters->ginpCounters[FE].minEFTR     = pMibInfo->adslStatSincePowerOn.ginpStat.cntDS.minEFTR;
  pCurCounters->ginpCounters[FE].errFreeBits = pMibInfo->adslStatSincePowerOn.ginpStat.cntDS.errFreeBits;
  
  /* GfastCounters */
  pCurCounters->gfastCounters[NE].ANDEFTRmin = pMibInfo->adslStat.gfastStat.rxANDEFTRmin;
  pCurCounters->gfastCounters[NE].ANDEFTRmax = pMibInfo->adslStat.gfastStat.rxANDEFTRmax;
  pCurCounters->gfastCounters[NE].ANDEFTRsum = pMibInfo->adslStat.gfastStat.rxANDEFTRsum;
  pCurCounters->gfastCounters[NE].ANDEFTRDS  = pMibInfo->adslStat.gfastStat.rxANDEFTRDS;
  pCurCounters->gfastCounters[NE].LANDEFTRS  = pMibInfo->adslStat.gfastStat.rxLANDEFTRS;
  pCurCounters->gfastCounters[FE].ANDEFTRmin = pMibInfo->adslStat.gfastStat.txANDEFTRmin/1000;        /* Floor of txANDEFTRmin */
  pCurCounters->gfastCounters[FE].ANDEFTRmax = (pMibInfo->adslStat.gfastStat.txANDEFTRmax+999)/1000;  /* Ceiling of txANDEFTRmax */
  pCurCounters->gfastCounters[FE].ANDEFTRsum = pMibInfo->adslStat.gfastStat.txANDEFTRsum;
  pCurCounters->gfastCounters[FE].ANDEFTRDS  = pMibInfo->adslStat.gfastStat.txANDEFTRDS;
  pCurCounters->gfastCounters[FE].LANDEFTRS  = pMibInfo->adslStat.gfastStat.txLANDEFTRS;
  
  CopyLineCountersDifference(&rep->data, pCurCounters, &me->lineCounters);
  rep->data.elapsedTimeSinceLastRead = pMibInfo->adslPerfData.adslSinceDrvStartedTimeElapsed - me->lastRead;
  me->lastRead = pMibInfo->adslPerfData.adslSinceDrvStartedTimeElapsed;
  if (rep->data.elapsedTimeSinceLastRead > 3600)
  {
    rep->data.suspectFlagNE |= BCM_SUSPECT_BECAUSE_TOO_LONG_PERIOD;
    rep->data.suspectFlagFE |= BCM_SUSPECT_BECAUSE_TOO_LONG_PERIOD;
  }
  rep->data.upTime = pMibInfo->adslPerfData.adslSinceLinkTimeElapsed;  /* nr of seconds already in showtime */
  me->lineCounters = *pCurCounters;

  BcmCoreDpcSyncExit(SYNC_RX);

  return msgp;
}

static void LineGetCurrent15MinutesCounter(LineMgr *me, HmiPeriodCounters *destp, adslMibInfo *pMibInfo)
{
  rtxCounters       rxtCounters;
  adslConnectionDataStat  connectionDataStat;
  
  /* DerivedSecCounters */
  CopyDerivedSecCounters(&destp->derivedCounts[NE], &pMibInfo->adslPerfData.perfCurr15Min);
  CopyDerivedSecCounters(&destp->derivedCounts[FE], &pMibInfo->adslTxPerfCur15Min);
  destp->derivedCounts[FE].AS   = destp->derivedCounts[NE].AS;

  /* AdslAnomCounters */
  rxtCounters = pMibInfo->rtxCounterData.cntDS.perfCurr15Min;
  rxtCounters.rtx_tx  = pMibInfo->rtxCounterData.cntUS.perfCurr15Min.rtx_tx;
  memset(&connectionDataStat, 0, sizeof(adslConnectionDataStat));
  connectionDataStat.cntSFErr   = pMibInfo->xdslChanPerfData[0].perfCurr15Min.adslChanUncorrectBlks;
  connectionDataStat.cntRSCor   = pMibInfo->xdslChanPerfData[0].perfCurr15Min.adslChanCorrectedBlks;
  connectionDataStat.cntRSUncor = pMibInfo->xdslChanPerfData[0].perfCurr15Min.rxUncorrectableCW;
  CopyAdslAnomCounters(&destp->adslAnomalyCounts[0].ne, &connectionDataStat, &rxtCounters);
  rxtCounters = pMibInfo->rtxCounterData.cntUS.perfCurr15Min;
  rxtCounters.rtx_tx  = pMibInfo->rtxCounterData.cntDS.perfCurr15Min.rtx_tx;
  memset(&connectionDataStat, 0, sizeof(adslConnectionDataStat));
  connectionDataStat.cntSFErr   = pMibInfo->xdslChanPerfData[0].perfCurr15Min.adslChanTxCRC;
  connectionDataStat.cntRSCor   = pMibInfo->xdslChanPerfData[0].perfCurr15Min.adslChanTxFEC;
  connectionDataStat.cntRSUncor = pMibInfo->xdslChanPerfData[0].perfCurr15Min.txUncorrectableCW;
  CopyAdslAnomCounters(&destp->adslAnomalyCounts[0].fe, &connectionDataStat, &rxtCounters);

  /* HmiPerfCounters: PTM counters */
  destp->perfCounts[0].ptm.p0.ptm_rx_packet_count = pMibInfo->atmStatCur15Min2lp[0].rcvStat.cntCellData;
  destp->perfCounts[0].ptm.p0.ptm_tx_packet_count = pMibInfo->atmStatCur15Min2lp[0].xmtStat.cntCellData;

  destp->failedFullInitCount  = pMibInfo->adslPerfData.failCur15Min.adslInitErr;
  destp->fullInitCount        = pMibInfo->adslPerfData.failCur15Min.adslRetr;
#if 0
  destp->reInitCount; /* incremented each time we start a new training within 20s after a showtime drop */
#endif
  destp->fastInitCounter        = pMibInfo->adslPerfData.failCur15Min.xdslFastRetr;
  destp->failedFastInitCounter  = pMibInfo->adslPerfData.failCur15Min.xdslFastInitErr;

  /* GinpCounters */
  destp->ginpCounters[NE].LEFTRS      = pMibInfo->ginpCur15MinCntrs[0].LEFTRS; /* N/A for G.fast */
  destp->ginpCounters[NE].minEFTR     = pMibInfo->ginpCur15MinCntrs[0].minEFTR;
  destp->ginpCounters[NE].errFreeBits = pMibInfo->ginpCur15MinCntrs[0].errFreeBits;
  destp->ginpCounters[FE].LEFTRS      = pMibInfo->ginpCur15MinCntrs[1].LEFTRS; /* N/A for G.fast */
  destp->ginpCounters[FE].minEFTR     = pMibInfo->ginpCur15MinCntrs[1].minEFTR;
  destp->ginpCounters[FE].errFreeBits = pMibInfo->ginpCur15MinCntrs[1].errFreeBits;

  /* GfastCounters */
  destp->gfastCounters[NE].ANDEFTRmin = pMibInfo->gfastCur15MinCntrs.txANDEFTRmin/1000;        /* Floor of txANDEFTRmin */
  destp->gfastCounters[NE].ANDEFTRmax = (pMibInfo->gfastCur15MinCntrs.txANDEFTRmax+999)/1000;  /* Ceiling of txANDEFTRmax */
  destp->gfastCounters[NE].ANDEFTRsum = pMibInfo->gfastCur15MinCntrs.txANDEFTRsum;
  destp->gfastCounters[NE].ANDEFTRDS  = pMibInfo->gfastCur15MinCntrs.txANDEFTRDS;
  destp->gfastCounters[NE].LANDEFTRS  = pMibInfo->gfastCur15MinCntrs.txLANDEFTRS;
  destp->gfastCounters[FE].ANDEFTRmin = pMibInfo->gfastCur15MinCntrs.rxANDEFTRmin;
  destp->gfastCounters[FE].ANDEFTRmax = pMibInfo->gfastCur15MinCntrs.rxANDEFTRmax;
  destp->gfastCounters[FE].ANDEFTRsum = pMibInfo->gfastCur15MinCntrs.rxANDEFTRsum;
  destp->gfastCounters[FE].ANDEFTRDS  = pMibInfo->gfastCur15MinCntrs.rxANDEFTRDS;
  destp->gfastCounters[FE].LANDEFTRS  = pMibInfo->gfastCur15MinCntrs.rxLANDEFTRS;

  destp->upTime              = pMibInfo->adslPerfData.adslSinceLinkTimeElapsed;  /* nr of seconds already in showtime */
  destp->elapsedTimeIn15min  = pMibInfo->adslPerfData.adslPerfCurr15MinTimeElapsed;
  destp->elapsedTimeIn24hour = pMibInfo->adslPerfData.adslPerfCurr1DayTimeElapsed;
}

void LineCpyCurrent15MinCounter(unsigned char lineId)
{
  adslMibInfo   *pMibInfo;
  long          mibLen;
  LineMgr *lineMgr = getLineMgr(lineId);
  mibLen   = sizeof(adslMibInfo);
  pMibInfo = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);

  BcmCoreDpcSyncEnter(SYNC_RX);
  memset((void *)&lineMgr->prev15MinCounters, 0, sizeof(lineMgr->prev15MinCounters));
  LineGetCurrent15MinutesCounter(lineMgr, &lineMgr->prev15MinCounters, pMibInfo);
  lineMgr->prev15MinCounters.periodDuration = 900;
  BcmCoreDpcSyncExit(SYNC_RX);
}

void *LineGetTrafficPeriodCounter(LineMgr *me,
                                  void    *msgp,
                                  uint16  *sizep,
                                  int     *resultp, uint16 hmiVersion)
{
  LineGetPeriodCounters_MsgRep *rep;
  LineGetPeriodCounters_MsgReq *req = msgp;
  HmiPeriodCounters *destp;
  uint32            periodId;
  adslMibInfo       *pMibInfo;
  long              mibLen;

  periodId  = req->data.periodIdentification;
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetTrafficPeriodCounter: periodId = %d", periodId);

  if (periodId > 1)
  {
    *resultp = HPI_INVALID_REQUEST_DATA;
    return NULL;
  }

  *sizep = hmiVersion > (10<<8) ? sizeof(*rep) : sizeof(*rep) - 2*sizeof(GinpCounters);
  rep = releaseRequestAllocateReply(msgp,sizeof(LineGetPeriodCounters_MsgRep));
  msgp = rep;
  *resultp = HPI_SUCCESS;

  destp = &rep->data;
  mibLen   = sizeof(adslMibInfo);
  pMibInfo = (void *) AdslCoreGetObjectValue (0, NULL, 0, NULL, &mibLen);
  
  BcmCoreDpcSyncEnter(SYNC_RX);
  if(periodId == 0) {
    memset(destp, 0, sizeof(HmiPeriodCounters));
    LineGetCurrent15MinutesCounter(me, destp, pMibInfo);
  }
  else {
    *destp = me->prev15MinCounters;
    destp->elapsedTimeIn15min  = pMibInfo->adslPerfData.adslPerfCurr15MinTimeElapsed;
    destp->elapsedTimeIn24hour = pMibInfo->adslPerfData.adslPerfCurr1DayTimeElapsed;
  }
  BcmCoreDpcSyncExit(SYNC_RX);
  
  destp->periodIdentification = periodId;
  if (periodId == 0)
  {
    destp->periodDuration = destp->elapsedTimeIn15min;
  }
  
  return msgp;
}

void* LineGetExtraConfig(LineMgr *me,
                            void    *msgp,
                            uint16  *sizep,
                            int     *resultp, uint16 hmiVersion)
{
  LineGetExtraConfig_MsgRep *rep;
  rep = releaseRequestAllocateReply(msgp,sizeof(*rep));

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetExtraConfig: size = %ld", sizeof(*rep));
  rep->data = me->lineExtraConfig;

  *sizep = sizeof(*rep);
  *resultp = HPI_SUCCESS;

  return rep;
}


static void *LineGetOamVector(LineMgr *me,
                              void    *msgp,
                              uint16  *sizep,
                              int     *resultp, uint16 hmiVersion)
{
  LineGetOamVector_MsgReq *req = (LineGetOamVector_MsgReq*) msgp;
  uint8 *req_bff = req->data.req;
  uint32 testParameterId  = req_bff[0];
  int i, g, j, realIndex;
  adslMibInfo *pMibInfo;
  char        oidStr[]  = { kOidAdslPrivate, 0};
  long        dataLen   = 0;
  
  LineGetOamVector_MsgRep *rep;
  uint8 *rep_bff;
  uint32 startToneGroupIx = (req_bff[1]<<8)+req_bff[2];
  uint32 stopToneGroupIx  = (req_bff[3]<<8)+req_bff[4];
  int len = stopToneGroupIx-startToneGroupIx+1; /* nr of items */
  
  rep = releaseRequestAllocateReply(msgp,sizeof(*rep));
  rep_bff = rep->data.rep;
  
  *resultp = HPI_SUCCESS;
  *sizep = sizeof(*rep);
  
  if ((testParameterId&0xf) == 1) len*=2; /* hlog has 2 bytes per entry */
  
  if ((startToneGroupIx>stopToneGroupIx)||(stopToneGroupIx>511)||(len>(sizeof(rep->data.rep)-4)))
  {
    *resultp = HPI_INVALID_REQUEST_DATA;
    return NULL;
  }

  dataLen    = sizeof(adslMibInfo);
  pMibInfo  = (void *) AdslCoreGetObjectValue (me->lineId, NULL, 0, NULL, &dataLen);
  
  g=pMibInfo->gFactors.Gfactor_SUPPORTERCARRIERSus;
  i=startToneGroupIx;

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetOamVector: startToneGroupIx=%d, stopToneGroupIx=%d len=%d", startToneGroupIx, stopToneGroupIx, len);
  
  rep_bff[0] = pMibInfo->gFactors.Gfactor_SUPPORTERCARRIERSus;
  rep_bff[1] = pMibInfo->gFactors.Gfactor_SUPPORTERCARRIERSds;
  rep_bff[2] = pMibInfo->gFactors.Gfactor_MEDLEYSETus;
  rep_bff[3] = pMibInfo->gFactors.Gfactor_MEDLEYSETds;

  dataLen = 0;
  switch(testParameterId)
  {
    case 0:
      /* dummy request to fetch grouping factors */
      return rep;
    case 1:    /* HlogUs (ALWAYS - state 7 or 4) */
    {
      int16 *pHlog, *pReply = (int16 *)&rep_bff[4];
      oidStr[1] = kOidAdslPrivChanCharLog;
      pHlog = (int16 *) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);
#if 1
      realIndex = i*g;
      if(realIndex < pMibInfo->usNegBandPlanDiscovery.toneGroups[0].endTone) {
        int printLen = ((stopToneGroupIx*g) < pMibInfo->usNegBandPlanDiscovery.toneGroups[0].endTone) ?
          len: (pMibInfo->usNegBandPlanDiscovery.toneGroups[0].endTone - (stopToneGroupIx*g)) << 1;
        DiagDumpData(0, DIAG_DSL_CLIENT, &pHlog[realIndex], printLen, kDiagDbgDataFormatHex | kDiagDbgDataSize8);
      }
#endif

      for (j=0;j<pMibInfo->usNegBandPlanDiscovery.noOfToneGroups;j++)
      {
        for(;i<=stopToneGroupIx;i++) {
          realIndex = i*g;
          if(realIndex >  pMibInfo->usNegBandPlanDiscovery.toneGroups[j].endTone )
            break;
          if(realIndex <  pMibInfo->usNegBandPlanDiscovery.toneGroups[j].startTone )
            *pReply++ = bswap16(0x3FF);
          else
            *pReply++ = bswap16((uint16)(((int)(6*16-pHlog[realIndex])*5) >> 3));
        }
      }
      for(;i<=stopToneGroupIx;i++)
        *pReply++ = bswap16(0x3FF);
      break;
    }
    case 3:    /* QlnDs (ALWAYS - state 7 or 4) */
    case 5:    /* Aln */
    {
      int16 *pQln;
      uint8 *pReply = &rep_bff[4];
      if(testParameterId == 3)
        oidStr[1] = kOidAdslPrivQuietLineNoise;
      else
        oidStr[1] = kOidAdslPrivActiveLineNoise;
      pQln = (int16 *) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);
#if 1
      realIndex = i*g;
      if(realIndex < pMibInfo->usNegBandPlanDiscovery.toneGroups[0].endTone) {
        int printLen = ((stopToneGroupIx*g) < pMibInfo->usNegBandPlanDiscovery.toneGroups[0].endTone) ?
          len: pMibInfo->usNegBandPlanDiscovery.toneGroups[0].endTone - (stopToneGroupIx*g);
        DiagDumpData(0, DIAG_DSL_CLIENT, &pQln[realIndex], printLen, kDiagDbgDataFormatHex | kDiagDbgDataSize8);
      }
#endif
      for (j=0;j<pMibInfo->usNegBandPlanDiscovery.noOfToneGroups;j++)
      {
        for(;i<=stopToneGroupIx;i++) {
          realIndex = i*g;
          if(realIndex >  pMibInfo->usNegBandPlanDiscovery.toneGroups[j].endTone )
            break;
          if(realIndex <  pMibInfo->usNegBandPlanDiscovery.toneGroups[j].startTone )
            *pReply++ = 255;
          else
            *pReply++ = (uint8)(pQln[realIndex] >> 3);
        }
      }
      for(;i<=stopToneGroupIx;i++)
        *pReply++ = 255;
      break;
    }
    default:
      *resultp = HPI_INVALID_REQUEST_DATA;
      return NULL;
  }
  DiagDumpData(0, DIAG_DSL_CLIENT, &rep_bff[0], (len+4), kDiagDbgDataFormatHex | kDiagDbgDataSize8);
  *resultp = HPI_SUCCESS;
  *sizep = sizeof(*rep);
  
  return rep;

}

static void* LineIntroduceErrors(LineMgr *me,
                                 void    *msgp,
                                 uint16  *sizep,
                                 int     *resultp, uint16 hmiVersion)
{
  LineIntroduceErrorsMsgReq *req = (LineIntroduceErrorsMsgReq*)msgp;
  DiagStrPrintf(me->lineId, DIAG_DSL_CLIENT, "LineIntroduceErrors: errorType = 0x%X", req->errorType);
  *resultp = HPI_SUCCESS;

  switch (req->errorType) {
  case BCM_ERROR_TYPE_RTX_TEST_ON:
    XdslDrvIntroduceGfastErrors(me->lineId, GFAST_START_RTX_TESTMODE);
    break;
  case BCM_ERROR_TYPE_RTX_TEST_OFF:
    XdslDrvIntroduceGfastErrors(me->lineId, GFAST_STOP_RTX_TESTMODE);
    break;
  case BCM_ERROR_TYPE_TPS_TEST_ON:
    XdslDrvIntroduceGfastErrors(me->lineId, GFAST_START_TPS_TESTMODE);
    break;
  case BCM_ERROR_TYPE_TPS_TEST_OFF:
    XdslDrvIntroduceGfastErrors(me->lineId, GFAST_STOP_TPS_TESTMODE);
    break;
  default:
    XdslDrvSendHmiCmd(me->lineId, kXDslSetLineIntroduceErrors, sizeof(LineIntroduceErrorsMsgReq), req, LineIntroduceErrorsMsgReqLayout);
    //*resultp = HPI_UNKNOWN_COMMAND_ID;
    break;
  }

  return NULL;
}

static void* LineSetLogicalFrameConfig(LineMgr *me, void *msgp, uint16 *sizep, int *resultp, uint16 hmiVersion)
{
  LogicalFrameCfg                  *lfcReq = &((( LineSetLogicalFrameConfig_MsgReq*)msgp)->data);
  LineSetLogicalFrameConfig_MsgRep *rep    = releaseRequestAllocateReply(msgp, sizeof(LineSetLogicalFrameConfig_MsgRep));
  LogicalFrameCfg                  *lfcRep = &rep->data;
  int                              dir     = lfcReq->direction;

  char            oidStr[] = { kOidAdslPrivate, kOidAdslPrivGetLogicalFrameInfo};
  long            dataLen  = 0;
  LogicalFrameCfg *pLogicalFrameCfg = (LogicalFrameCfg *) AdslCoreGetObjectValue (me->lineId, oidStr, sizeof(oidStr), NULL, &dataLen);

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineSetLogicalFrameConfig: sizep=%d dataLen=%d", sizeof(*rep), dataLen);

  *resultp = HPI_SUCCESS;

  if (lfcReq->applyNow != -1) {
    memcpy(&pLogicalFrameCfg[dir], lfcReq, sizeof(LogicalFrameCfg));         /* Set command */
    XdslDrvSendHmiCmd(me->lineId, kXDslSetLogicalFrameConfig, sizeof(LogicalFrameCfg), lfcReq, SetLineLogicalFrameConfigMsgReqLayout);
  }
  memcpy(lfcRep,&pLogicalFrameCfg[dir], sizeof(LogicalFrameCfg));

  *sizep = sizeof(*rep);
  return rep;
}

#define FREE    0
#define PENDING 1
#define LOCKED  2

#define TIMEOUT_AFE  (5000) /* 5s timeout */

Bool setBufferState(LineMgr *me, int state) {
  /* possible transitions: 
   * (LOCKED -> FREE)           : On error (abort)
   * (LOCKED -> PENDING)        : On message arrival
   * (PENDING -> PENDING)       : Part of the message has been fetched by HMI (timeout refresh)
   * (PENDING -> FREE)          : When message has been fully fetched by HMI 
   * (FREE -> LOCKED)           : Scheduling a message to be sent
   * (PENDING -> LOCKED)        : When timeout has expired.
   * Not allowed: (FREE -> FREE), (FREE -> PENDING) 
   *
   * Returns True for successfull (anything -> LOCKED) and (LOCKED -> FREE)
   * */
  Bool retVal = FALSE;
  
  switch (state) {
    case LOCKED:
      if((me->hmiEocBuf.bufferState == FREE) ||
         ((me->hmiEocBuf.bufferState == PENDING) && ((me->hmiEocBuf.lockTime + TIMEOUT_AFE) < XdslDrvGetTime()))) {
        me->hmiEocBuf.bufferState = LOCKED;
        me->overheadClient.bufferPair.buffer1.data = &me->hmiEocBuf.localBuffer[0];
        me->overheadClient.bufferPair.buffer2.data = &me->hmiEocBuf.localBuffer[0];
        retVal = TRUE;
      }
      break;
    case FREE:
    case PENDING:
      if((me->hmiEocBuf.bufferState != FREE)
         && !((me->hmiEocBuf.bufferState == PENDING) && ((me->hmiEocBuf.lockTime+TIMEOUT_AFE) < XdslDrvGetTime()))) {
        /* We are only allowed to send an IRQ when current state is LOCKED */
        retVal = me->hmiEocBuf.bufferState == LOCKED ? TRUE: FALSE;
        me->hmiEocBuf.bufferState = state;
        if (state == PENDING) {
          me->hmiEocBuf.lockTime = XdslDrvGetTime();
        }
        break;
      }
      break;
  }
  return retVal;
}

void HmiBufferMgr_init(LineMgr *me)
{
  me->hmiEocBuf.bufferState = FREE;
}

void HmiMgr_init(LineMgr *me)
{
//  MessageCommand messageCommand = 0;  /*not yet know at this instance, changes everytime we want to send a message*/
  
  me->taskToBeProcessed = FALSE;
  memset(&me->overheadClient, 0, sizeof(OverheadClient));
  me->overheadClient.messageReceived = HmiMgr_messageReceived;
  me->overheadClient.responseBufferSize = HMI_BUFFER_SIZE;
  HmiBufferMgr_init(me);
}

Bool HmiMgr_txFilterMessage(LineMgr *me, uint8* message, uint16 messageLength)
{
  MessageCommand messageCommand = (MessageCommand)message[0];
  if (messageLength < 2)
    return TRUE; /* wrong message, too short */
  
  {
    uint8 testParameter = message[1];

    switch (messageCommand)
    {
        case MGR_COUNTER_READ_COMMAND: /* not allowed, we implement a Mgr ourselves */
          return TRUE;
        case MGR_TESTPARAMETERS_READ_COMMAND:
          if (testParameter == SINGLE_READ_COMMAND)
          {
            if ( messageLength < 3 )
              return TRUE;
            else
            {
              uint32 specificTestParameter = message[2];
              if  ((specificTestParameter == SIGNAL_ATTENUATION) 
                   ||(specificTestParameter == SNR_MARGIN) 
                   ||(specificTestParameter == ATTAINABLE_NET_DATA_RATE))
                return TRUE;
            }
          }
          break;
#if 0//def INM /* Don't filter these INM commands. Let the far end reject them if it dislikes them */
        case MGR_INM_FACILITY_COMMAND:
          {
            uint8 inmCommand = message[1];

            switch (inmCommand)
            {
              case READ_INM_COUNTERS_COMMAND:
                if (messageLength != READ_INM_COUNTERS_SIZE)
                  return TRUE;
                break;
              case READ_INM_PARAMETERS_COMMAND:
                if (messageLength != READ_INM_PARAMETERS_SIZE)
                  return TRUE;
                break;
              case SET_INM_PARAMETERS_COMMAND:
                DBPrint(TRC_DETAIL, LOC_ALL, "SET_INM_PARAMETERS_COMMAND: Expected message length = %d, actual = %d\n", SET_INM_PARAMETERS_SIZE, messageLength);
                if (messageLength != SET_INM_PARAMETERS_SIZE)
                  return TRUE;
                break;
              default:
                return TRUE;
            }
          }
          break;
#endif
        case MGR_ADSL2EOC_READ_COMMAND:
          if (testParameter == UPDATE_TESTPARAMETERS_COMMAND)
            return TRUE;
          break;
        case OLR_COMMAND:
        case PMM_COMMAND:
          return TRUE;
    };
  }
  /* message must not be filtered */
  return FALSE;
}

void HmiMgr_releaseBufferIfAllocated(LineMgr *me)
{
  /* will only release the buffer if it was LOCKED */ 
  if (setBufferState(me, FREE))
  {
    lineIRQ_setStatus(me->lineId, INT_HMI_TRANSACTION_FINISHED_MASK);
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "Hmi buffer released by line %d: \n", me->lineId);
  }
}

Bool HmiMgr_allocateBuffer(LineMgr *me, uint8* message, uint16 messageLength)
{
  if (!me->taskToBeProcessed && setBufferState(me, LOCKED)) {
      Buffer* buffer1 = &me->overheadClient.bufferPair.buffer1;
      Buffer* buffer2 = &me->overheadClient.bufferPair.buffer2;
      /* + address field + control field, transmit message length is smaller than receive message length 
         so we can add the +2 without causing an out of bound write */
      buffer1->length = messageLength + 2;
      buffer2->length = HMI_BUFFER_SIZE;

      memcpy(&buffer1->data[2],message,messageLength);
      /* HmiMgr can send different commands, set the command type once it is known */
      me->overheadClient.messageCommand[0] = (MessageCommand)buffer1->data[2];
      me->taskToBeProcessed = TRUE;
      return TRUE;
  }
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "HmiMgr_allocateBuffer returned 0 (last task not yet scheduled = %d)", me->taskToBeProcessed);
  return FALSE;
}

void HmiMgr_messageReceived(OverheadClient* overheadClient, ErrorType errorType)
{
  LineMgr *lineMgr = getLineMgr(0);
  switch (errorType)
  {
      case NO_ERROR:
        setBufferState(lineMgr, PENDING);
        DiagStrPrintf(0, DIAG_DSL_CLIENT,"Message received in HmiMgr\n");
        /* no processing of the message must be done, buffer will be copied
         * when service LineGetOverheadMessage is called */
        /* informing the OverheadProcessScheduler to continue occurs in the OverheadClient */
        break;
        /* what to do when a message was received but no buffer was available? */
      case TIMEOUT:
        /* once the message is received release the buffer and inform the overheadProcessScheduler */
        /* release the buffer */
        /* don't want to retry previous request, continue scheduler */
        DiagStrPrintf(0, DIAG_DSL_CLIENT,"Timeout in HmiMgr\n");
        lineMgr->taskToBeProcessed = FALSE;
        lineMgr->overheadClient.bufferPair.buffer2.length = 0;
        /* Intentional fall through */
      default:
        setBufferState(lineMgr, FREE);
        DiagStrPrintf(0, DIAG_DSL_CLIENT,"Hmi buffer released, Error %d", errorType);
        break;
  };
  lineIRQ_setStatus(lineMgr->lineId, INT_HMI_TRANSACTION_FINISHED_MASK);
}

void HmiMgr_getOverheadReply(LineMgr *me, uint8* message, uint16* messageLength, uint8 partId)
{
  int beginPosition = 0; 
  int length = 0;
  Buffer* buffer2 = &me->overheadClient.bufferPair.buffer2;

  /*DBPrint(TRC_ERROR, LOC_ALL, "Line %d: getMessage (%d)\n", line, i);*/
  if (buffer2->length > 2)
  {
    beginPosition = partId * 256 + 2;
    length = ((int16)buffer2->length - beginPosition > 256) ? 256 
      : buffer2->length - beginPosition;
  }

  /* start reading before the end of the message */
  if (length > 0)
  {
    memcpy(message,&buffer2->data[beginPosition],length);
    *messageLength = length;
  }
  else 
  {
    *messageLength = 0;
  }
  if (((int16)buffer2->length - beginPosition - length) <= 0)
    setBufferState(me, FREE);
  else
    setBufferState(me, PENDING);
}

Bool HmiMgr_isMessageAvailable(LineMgr *me)
{
  Bool retCode = FALSE;
  if (me->hmiEocBuf.bufferState != LOCKED) {
    /* if locked --> no reply is available yet */
    retCode = TRUE;
  }
  return retCode;
}

static void* LineSendOverheadMessage(LineMgr *me,
                                     void    *msgp,
                                     uint16  *sizep,
                                     int     *resultp, uint16 hmiVersion)
{
  LineSendOverheadMessage_MsgReq *req = (LineSendOverheadMessage_MsgReq*)msgp;

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineSendOverheadMessage: cmdLend=%d", req->data.length);
  DiagDumpData(0, DIAG_DSL_CLIENT, &req->data.message[0], req->data.length, kDiagDbgDataFormatHex | kDiagDbgDataSize8);
  if ((me->lineState & LINE_SHOWTIME) && XdslDrvIsXdsl2Mod(me->lineId))
  {
    if( (req->data.length <= HMIOVERHEAD_MAX_BUFFER)
        && (!HmiMgr_txFilterMessage(me,req->data.message,req->data.length)))
    {
      if(HmiMgr_allocateBuffer(me,req->data.message,req->data.length)) 
      {
        if(0 == XdslCoreG997SendHmiEocData(me->lineId, &me->overheadClient)) {
          *resultp = HPI_RESOURCES_ALREADY_IN_USE;
          setBufferState(me, FREE);
        }
        else {
          *resultp = HPI_SUCCESS;
          me->taskToBeProcessed = FALSE;
        }
      }
      else
        *resultp = HPI_RESOURCES_ALREADY_IN_USE;
    } 
    else
      *resultp = HPI_INVALID_REQUEST_DATA;
  }
  else
    *resultp = HPI_NOT_POSSIBLE_IN_CURRENT_STATE;
  
  if(*resultp != HPI_SUCCESS)
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineSendOverheadMessage: error=%d", *resultp);

  return NULL;
}

static void* LineGetOverheadMessage(LineMgr *me,
                                    void    *msgp,
                                    uint16  *sizep,
                                    int     *resultp, uint16 hmiVersion)
{
  LineGetOverheadMessage_MsgRep *rep;
  uint8 partId =  *((uint8 *)msgp);

  if ((me->lineState & LINE_SHOWTIME) && XdslDrvIsXdsl2Mod(me->lineId)) {
    rep = releaseRequestAllocateReply(msgp,sizeof(*rep));

    HmiMgr_getOverheadReply(me,rep->data.message,&rep->data.length,partId);
    *resultp = HPI_SUCCESS;
    *sizep = sizeof(*rep);
    
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetOverheadMessage: msgLen=%d partId=%d", rep->data.length,partId);

    return rep;
  }
  else
    *resultp = HPI_NOT_POSSIBLE_IN_CURRENT_STATE;
  
  if(*resultp != HPI_SUCCESS)
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineGetOverheadMessage: error=%d", *resultp);

  return NULL;
}

static void* LineGetOverheadStatus(LineMgr *me,
                                   void    *msgp,
                                   uint16  *sizep,
                                   int     *resultp, uint16 hmiVersion)
{
  LineGetOverheadStatus_MsgRep *rep;
  Bool messageReceived;

  if( XdslDrvIsXdsl2Mod(me->lineId) ) {
    rep = releaseRequestAllocateReply(msgp, sizeof(*rep));
    messageReceived = HmiMgr_isMessageAvailable(me);
    rep->data.overheadStatus = (messageReceived ? 0 : 1);
    *resultp = HPI_SUCCESS;
    *sizep = sizeof(*rep);
    
    return rep;
  }
  else
    *resultp = HPI_NOT_POSSIBLE_IN_CURRENT_STATE;

  return NULL;
}

static void* LineUpdateTestParams(LineMgr *me,
                                  void    *msgp,
                                  uint16  *sizep,
                                  int     *resultp, uint16 hmiVersion)
{
  LineUpdateTestParams_MsgRep *rep;
  LineUpdateTestParams_MsgReq *req = (LineUpdateTestParams_MsgReq*)msgp;
  uint32 command = req->data;
  me->updateTestParams |= 3;
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "LineUpdateTestParams: command = 0x%X(0x%X)\n", command, me->updateTestParams);
  rep = releaseRequestAllocateReply(msgp,sizeof(*rep));
  rep->data = me->updateTestParams;
  *resultp  = HPI_SUCCESS;
  *sizep    = sizeof(*rep);

  return rep;
}

LineMgr* getLineMgr(int line) {
  return &gLineMgr[line];
}

