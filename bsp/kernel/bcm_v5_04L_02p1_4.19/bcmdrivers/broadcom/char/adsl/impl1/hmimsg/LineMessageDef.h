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


#ifndef LINEMESSAGEDEF_H
#define LINEMESSAGEDEF_H

#ifdef SUPPORT_HMI

#ifndef HPIMESSAGE_H
#include "HpiMessage.h"
#endif

#else

#ifndef BASICTYPES_H
#include "platform/BasicTypes.h"
#endif

#ifndef HPIMESSAGE_H
#include "platform/HpiMessage.h"
#endif

#ifndef LINETYPES_H
#include "lineMgr/LineTypes.h"
#endif

#ifndef PSDINFO_H
#include "common/PsdInfo.h"
#endif

#ifndef STDINFO_H
#include "common/StdInfo.h"
#endif

#ifndef TSSI_H
#ifndef MATLAB_MEX
#include "common/tssi.h"
#endif
#endif

#ifndef DEF_TYPES_H
#include "algo/DefTypes.h"
#endif

#ifndef CORE_H
#ifndef MATLAB_MEX
#include "genericCore/Core.h"
#endif
#endif

#ifndef HsNegotiation_h
#include "handshake/HsNegotiation.h"
#endif

#ifndef VECTORING_ALGO_H
#include "algo/VectoringAlgo.h"
#endif

#ifdef GFAST_SUPPORT
#include "gfast/GfastTypes.h"
#include "gfast/DtaController.h"
#ifndef DTA_MGR_H
#include "gfast/DtaMgr.h"
#endif
#endif

#ifndef ALGOTYPES_H
#include "algo/AlgoTypes.h"
#endif

#endif  /* SUPPORT_HMI */

/***********************************************************************/
/******************  Line Configuration Services    ********************/
/***********************************************************************/


/* after a line reset, the line goes idle,
   The following actions are taken by the modem:
    - forget all past events (=> all counters are reset)
    - go into a default config mode (forget all previous configurations)
    - go to idle state


*/

/* LineStart Error Codes */
#define LINE_NO_PROTOCOL_AVAILABLE     0x1
#define LINE_INVALID_ATTNDR_ITLV_SPLIT 0x2
#define LINE_INVALID_DELAY             0x3
#define LINE_INVALID_VDSL_ECHO_CONFIG  0x4

#define LINE_SERVICE_ID 0x100
#define LINE_DEBUG_ID   0x400


#define CLEAREOC_MAX_BUFFER_IMPL    486
#define CLEAREOC_MAX_BUFFER         486
#define CLEAREOC_MAX_BUFFER_GFAST   510
#define DATAGRAM_MAX_BUFFER_GFAST  1018
#define DATAGRAM_MAX_SEGMENT_GFAST  700

#define LINE_RESET                (0x1 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineReset, LINE_RESET,uint32, DUMMY)


#define LINE_SET_TRAFFIC_CONFIG   (0x2 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetTrafficConfig,
             LINE_SET_TRAFFIC_CONFIG,
             LineTrafficConfig,
             DUMMY)

#define LINE_GET_TRAFFIC_CONFIG   (0x3 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetTrafficConfig,
             LINE_GET_TRAFFIC_CONFIG,
             DUMMY,
             LineTrafficConfig)


#define LINE_SET_PHY_LAYER_CONFIG (0x4 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetPhyLayerConfig,
             LINE_SET_PHY_LAYER_CONFIG,
             PhysicalLayerConfig,
             DUMMY)

#define LINE_GET_PHY_LAYER_CONFIG (0x5 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPhyLayerConfig,
             LINE_GET_PHY_LAYER_CONFIG,
             DUMMY,
             PhysicalLayerConfig)


#define LINE_SET_TEST_CONFIG      (0x6 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetTestConfig,
             LINE_SET_TEST_CONFIG,
             TestConfigMessage,
             DUMMY)

#define LINE_GET_TEST_CONFIG      (0x7 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetTestConfig,
             LINE_GET_TEST_CONFIG,
             DUMMY,
             TestConfigMessage)


#define LINE_START (0x8 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineStart, LINE_START, DUMMY, DUMMY)
/*
   - Is ignored if the line is not in idle state
   - LineStart is resting all internal counters.
*/

#define LINE_STOP (0x9 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineStop, LINE_STOP, DUMMY, DUMMY)

#ifndef SUPPORT_HMI
/* LineCounters struct is the internal type, it doesn't contain the reserved[16] field */
typedef struct LineTrafficCounters LineTrafficCounters;
struct LineTrafficCounters
{
  DerivedSecCounters adslDerivCounts[2];/* NE/FE */
  AdslAnomCounters   adslAnomCounts[2]; /* B0/B1 */
  AtmPerfCounters    atmPerfCounters[2];

  uint16   failedFullInitCount;       /* incremented each time an exception is
                                         reported in a non showtime state while in full init */
  uint8    fullInitCount;             /* Gfast [g997.2 ($7.7.13)/g9701 ($11.3.1.5)]: incremented each time FTU-O transitions from O-SILENT to O-INIT/HS state.
                                         xdsl [g997.1 ($7.2.1.3.1)]: incremented at each initialization attempt (successful and failed) */
  uint8    reInitCount;               /* incremented each time we start a new training
                                         within 20s after a showtime drop */
  uint16   fastInitCounter;           /* incremented each time training state is entered from showtime */
  uint16   failedFastInitCounter;     /* incremented each time an exception is
                                         reported in a non showtime state while in fast init */
  uint16   elapsedTimeSinceLastRead;  /* number of seconds since last read action */
  uint8    suspectFlagNE;             /* the suspect flag is activated either
                                         - the first time PM is read after a
                                           Santorini reset or line reset.
                                         - when the elapsed time between two
                                           reads is bigger than 1 hour.    */
  uint8    suspectFlagFE;             /* the suspect flag is activated either
                                         - the first time PM is read after a
                                           Santorini reset or line reset.
                                         - when the elapsed time between two
                                           reads is bigger than 1 hour.
                                         - the FE statistics are not reliable
                                           because of a IB were not reliable at
                                           some point in time during the last
                                           monitoring period.
                                      */
  uint32   upTime;                    /* nr of seconds already in showtime */
  uint32   reserved[3];               /* introduced to match the periodCounter length.*/
  GinpCounters ginpCounters[2];       /* NE/FE */
  GfastRtxCounters gfastCounters[2];
};
#endif

#define LINE_GET_TRAFFIC_COUNTERS (0xa  | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetCounters, LINE_GET_TRAFFIC_COUNTERS, DUMMY, LineCounters)

#define LINE_GET_STATUS (0xb | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetStatus, LINE_GET_STATUS, uint32, LineStatus)

#define LINE_SHOWTIME_UNIT_TEST (0x6d | LINE_SERVICE_ID)
HPI_SERV_DEF(ShowtimeUnitTest, LINE_SHOWTIME_UNIT_TEST, LineStatus,DUMMY)

#define LINE_GET_FEATURES (0xc | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetFeatures, LINE_GET_FEATURES, DUMMY, LineFeatures)

#if defined(VDSL) && defined(ATU_C)
#define LINE_SET_PHY_LAYER_CONFIG_VDSL (0xD | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetPhyConfigVDSL,
             LINE_SET_PHY_LAYER_CONFIG_VDSL,
             PhysicalLayerConfigVDSL,
             DUMMY)
#endif

#define LINE_GET_PHY_LAYER_CONFIG_VDSL (0xE | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPhyConfigVDSL,
             LINE_GET_PHY_LAYER_CONFIG_VDSL,
             DUMMY,
             PhysicalLayerConfigVDSL)
 

#if defined(SUPPORT_HMI) || defined(GFAST_SUPPORT) 

typedef struct AfeCalibrationStruct AfeCalibrationStruct;
struct AfeCalibrationStruct 
{
  uint16 setCalibration;
  uint16 reserved;
  uint16 calib[4];
  int16  dcoffset[4];
};


#define LINE_SET_AFE_CAL (0x1D | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetAfeCal,
             LINE_SET_AFE_CAL,
             AfeCalibrationStruct,
             AfeCalibrationStruct)


#define LINE_SET_CONFIG_GFAST (0x1E | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetConfigGFAST,
             LINE_SET_CONFIG_GFAST,
             GfastConfig,
             DUMMY)

#define LINE_GET_CONFIG_GFAST (0x1F | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetConfigGFAST,
             LINE_GET_CONFIG_GFAST,
             DUMMY,
             GfastConfig)

#define LINE_SET_GFAST_RFI_CONFIG (0x7B | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetGfastRFIconfig,
             LINE_SET_GFAST_RFI_CONFIG,
             GfastRFIconfig,
             DUMMY)

#define LINE_GET_GFAST_RFI_CONFIG (0x7C | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetGfastRFIconfig,
             LINE_GET_GFAST_RFI_CONFIG,
             DUMMY,
             GfastRFIconfig)

#ifndef SUPPORT_HMI
typedef struct GfastSchedInfo GfastSchedInfo;
struct GfastSchedInfo
{
  GlobalTddSettings gts;
  #ifdef MONITOR_TDD_SCHEDULING
  uint16 schedMon[MF_MAX];
  uint16 schedAvg[MF_MAX];
  uint16 schedDelta[MF_MAX];
  #endif
  uint16 schedMonTx;
  int16  distDeadLineTx;
};

#define LINE_GET_SCHED_INFO (0x7D | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetSchedInfo,
             LINE_GET_SCHED_INFO,
             DUMMY,
             GfastSchedInfo)
#endif
#endif  /* SUPPORT_HMI */

typedef struct 
{
  uint64 afeCtrlWriteValue; // will be written once to AFEB_0_WR_CTRL at
                            // afeTimeOffset after HWA has started 
  uint64 afeTimeOffset;         
  
} AfeWriteRequestAtOffset;
  
typedef struct TimeDomainCaptureReq TimeDomainCaptureReq;
struct TimeDomainCaptureReq
{
  AfeWriteRequestAtOffset cmd[16];
  uint32                  nbOfCmd;
};


#define LINE_CAPTURE_TIME_DOMAIN_SAMPLES (0xF | LINE_SERVICE_ID)
HPI_SERV_DEF(LineCaptureTimeDomainSamples,
             LINE_CAPTURE_TIME_DOMAIN_SAMPLES,
             TimeDomainCaptureReq,
             DUMMY)
  

  

/*
   OamVectorRequest:
   octet 0: parameter ID (see G993.2)
   octet 1&2: MSB&LSB of startToneGroupIndex
   octet 3&4: MSB&LSB of stopToneGroupIndex
*/
typedef struct OamVectorRequest OamVectorRequest;
struct OamVectorRequest
{
  uint8 req[5];
};

/*
   OamVectorReply:
   octet 0: tone grouping factor G (SUPPORTEDCARRIERSus)
   octet 1: tone grouping factor G (SUPPORTEDCARRIERSds)
   octet 2: tone grouping factor G (MEDLEYSETus)
   octet 3: tone grouping factor G (MEDLEYSETds)
   octet 4-483: maximum 480 OAM octets
*/


typedef struct OamVectorReply OamVectorReply;
struct OamVectorReply
{
  uint8 rep[484];
};

#define LINE_GET_OAM_VECTOR (0x1a | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetOamVector, LINE_GET_OAM_VECTOR, OamVectorRequest,OamVectorReply)



#define LINE_GET_PTM_COUNTERS (0x1b | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPtmCounters, LINE_GET_PTM_COUNTERS, DUMMY,PtmCounters)


#ifdef NTR_PHASE_ALIGN
#define LINE_SET_NTR_PHASE_COMP (0x6D | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetNtrPhaseCompensation, LINE_SET_NTR_PHASE_COMP, NtrPhaseComp, DUMMY)


#define LINE_GET_NTR_PHASE_COMP (0x6E | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetNtrPhaseCompensation, LINE_GET_NTR_PHASE_COMP, DUMMY, NtrPhaseComp)
#endif
  
#ifdef ATU_C
typedef struct ReadEocRegister ReadEocRegister;
struct ReadEocRegister
{
  uint8 registerId;
};

#define LINE_READ_EOC_REGISTER (0x10 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineReadEocRegister, LINE_READ_EOC_REGISTER, ReadEocRegister, DUMMY)

#ifndef SUPPORT_HMI  /* Already defined in bcm_hmiLineMsg.h */
typedef struct EocReadResult EocReadResult;
struct EocReadResult
{
  uint8 eocReadStatus;
  uint8 length;
  uint8 eocData[32];
};
#endif

#define LINE_GET_EOC_READ_RESULT (0x11 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetEocReadResult, LINE_GET_EOC_READ_RESULT, DUMMY, EocReadResult)

typedef struct WriteEocRegister WriteEocRegister;
struct WriteEocRegister
{
  uint8 registerId;
  uint8 length;
  uint8 eocData[32];
};

#define LINE_WRITE_EOC_REGISTER (0x12 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineWriteEocRegister, LINE_WRITE_EOC_REGISTER, WriteEocRegister, DUMMY)

#ifndef SUPPORT_HMI  /* Already defined in bcm_hmiLineMsg.h */
typedef struct EocWriteResult EocWriteResult;
struct EocWriteResult
{
  uint8 eocWriteStatus;
  uint8 length;
};
#endif

#define LINE_GET_EOC_WRITE_RESULT (0x13 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetEocWriteResult, LINE_GET_EOC_WRITE_RESULT, DUMMY, EocWriteResult)

#endif

#define TRANSPARENTEOC_TYPE_CLEAREOC        0
#define TRANSPARENTEOC_TYPE_DATAGRAM        1
#define TRANSPARENTEOC_TYPE_NSF_NORMAL      2
#define TRANSPARENTEOC_TYPE_NSF_LOW_IKAN    3
#define TRANSPARENTEOC_ISLASTPART        0x80
typedef struct ClearEocMessage ClearEocMessage;
struct ClearEocMessage
{
  uint16 length;  // bit 15: NSF support, bit 14: Datagram support
  uint8  transparentEocType;
  uint8  partId;
  uint8  message[DATAGRAM_MAX_SEGMENT_GFAST];
};

#define LINE_SEND_CLEAREOC_MESSAGE (0x14 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSendClearEocMessage, LINE_SEND_CLEAREOC_MESSAGE, ClearEocMessage, DUMMY)

#define LINE_GET_CLEAREOC_MESSAGE (0x15 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetClearEocMessage, LINE_GET_CLEAREOC_MESSAGE, DUMMY, ClearEocMessage)


#define LINETEST_HIST_LEN 32
typedef struct LineTestHistogram LineTestHistogram;
struct LineTestHistogram
{ 
  uint32 cnt;
  uint32 refPwr;
  uint32 buckets[LINETEST_HIST_LEN];
};


#define LINE_GET_HISTOGRAM_MESSAGE (0x16 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetHistogramMessage,LINE_GET_HISTOGRAM_MESSAGE, DUMMY, LineTestHistogram)



#ifdef FRAMER_TEST

#define LINE_FRAMER_TEST (0x17 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineFramerTest, LINE_FRAMER_TEST, FramerConfig, DUMMY)

#endif /*FRAMER_TEST*/

#ifdef SUPPORT_HMI

#define LINE_GET_BANDPLAN_VDSL (0x18 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetBandPlanVDSL, LINE_GET_BANDPLAN_VDSL, DUMMY, BandPlan)

#else  /* Already defined in bcm_hmiLineMsg.h */

typedef struct BandPlan BandPlan;
struct BandPlan
{
  BandsDescriptor bandPlanDn;
  BandsDescriptor bandPlanUp;
};

typedef struct BandPlanInfo BandPlanInfo;
struct BandPlanInfo
{
  BandPlan bandplan;
  VdslToneGroup supportedSetDsBounds;
  VdslToneGroup supportedSetUsBounds;
  VdslToneGroup medleySetDsBounds;
  VdslToneGroup medleySetUsBounds;
};
#define LINE_GET_BANDPLAN_VDSL (0x18 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetBandPlanVDSL, LINE_GET_BANDPLAN_VDSL, DUMMY, BandPlanInfo)

#endif

#ifndef SUPPORT_HMI  /* Already defined in bcm_hmiLineMsg.h */
typedef struct MrefPsd MrefPsd;
struct MrefPsd
{
  DsMrefPsdDescriptor dsMrefPsdDescriptor; /* defined in PsdTypes.h */
  UsMrefPsdDescriptor usMrefPsdDescriptor;
};
#endif

#define LINE_GET_PSD_VDSL (0x1C | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPsdVDSL, LINE_GET_PSD_VDSL, DUMMY, MrefPsd)




typedef struct VDSLecho VDSLecho;
struct VDSLecho
{
 int16  numRxTones;
 fix8_8 totRxGain_dB;
 fix8_8 analogRxGain_dB;
 uint8  reserved[2];
 int16  partialVDSLechoMant[128];
 int8   partialVDSLechoExp[64];
};

#define LINE_GET_ECHO_COMPLEMENT (0x19 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetVDSLecho, LINE_GET_ECHO_COMPLEMENT, uint32, VDSLecho)

typedef struct LDPCBinfo LDPCBinfo;
struct LDPCBinfo
{
  uint8 passFail;
  uint8 lastTxState;
};




#define HMIOVERHEAD_MAX_BUFFER 256
typedef struct HmiOverheadMessage HmiOverheadMessage;
struct HmiOverheadMessage
{
  uint16 length;
  uint8 message[HMIOVERHEAD_MAX_BUFFER];
};


#define LINE_SEND_OVERHEAD_MESSAGE (0x38 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSendOverheadMessage, LINE_SEND_OVERHEAD_MESSAGE, HmiOverheadMessage, DUMMY)

#define LINE_GET_OVERHEAD_MESSAGE (0x39 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetOverheadMessage, LINE_GET_OVERHEAD_MESSAGE, uint8, HmiOverheadMessage)

typedef struct HmiOverheadStatus HmiOverheadStatus;
struct HmiOverheadStatus
{
  uint8 overheadStatus;
};

#define LINE_GET_OVERHEAD_STATUS (0x3a | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetOverheadStatus, LINE_GET_OVERHEAD_STATUS, DUMMY, HmiOverheadStatus)

#ifdef ATU_C

typedef struct LineLDResults LineLDResults;
struct LineLDResults
{
  uint32 attndr;
  uint16 latn;
  uint16 satn;
  int16  snrm;
  int16  actatpFe;
};

typedef struct LineLDResultsMsg LineLDResultsMsg;
struct LineLDResultsMsg
{
  LineLDResults ne;
  LineLDResults fe;
  LDPCBinfo ldPCBinfo[2]; /*NE/FE*/
};

#define LINE_GET_LD_RESULTS (0x3b | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetLDResults, LINE_GET_LD_RESULTS, DUMMY, LineLDResultsMsg)

typedef struct LineHlinLDResults LineHlinLDResults;
struct LineHlinLDResults
{
  uint16 hlinScale;
  Hlini  hlin[64];
};

typedef struct LineHlinLDRequest LineHlinLDRequest;
struct LineHlinLDRequest
{
  uint8 direction; /*0 = ne, 1 = fe*/
  uint8 partId; /*must be 0 for ne, 0-7 in case of fe*/
};

#define LINE_GET_HLIN_LD_RESULTS (0x3c | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetHlinLDResults, LINE_GET_HLIN_LD_RESULTS, LineHlinLDRequest, LineHlinLDResults)

typedef struct LineHlogLDResults LineHlogLDResults;
struct LineHlogLDResults
{
  uint16 hlogNe[64];
  uint16 hlogFe[128];
};

#define LINE_GET_HLOG_LD_RESULTS (0x3d | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetHlogLDResults, LINE_GET_HLOG_LD_RESULTS, uint8, LineHlogLDResults)

typedef struct LineQlnLDResults LineQlnLDResults;
struct LineQlnLDResults
{
  uint8 qlnNe[64];
  uint8 qlnFe[256];
};

#define LINE_GET_QLN_LD_RESULTS (0x3e | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetQlnLDResults, LINE_GET_QLN_LD_RESULTS, uint8, LineQlnLDResults)

typedef struct LineSnrLDResults LineSnrLDResults;
struct LineSnrLDResults
{
  uint8 snrNe[64];
  uint8 snrFe[256];
};

#define LINE_GET_SNR_LD_RESULTS (0x3f | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetSnrLDResults, LINE_GET_SNR_LD_RESULTS, uint8, LineSnrLDResults)

#define LINE_UPDATE_TESTPARAMS (0x40 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineUpdateTestParams, LINE_UPDATE_TESTPARAMS, uint8, uint8)

typedef struct HlogOperNe HlogOperNe;
struct HlogOperNe
{
  uint16 hlogNe[64];
};

#define LINE_GET_HLOG_OPER_NE (0x41 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetHlogOperNe, LINE_GET_HLOG_OPER_NE, DUMMY, HlogOperNe)

typedef struct QlnOperNe QlnOperNe;
struct QlnOperNe
{
  uint8 qlnNe[64];
};

#define LINE_GET_QLN_OPER_NE (0x42 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetQLNOperNe, LINE_GET_QLN_OPER_NE, DUMMY, QlnOperNe)


typedef struct LinearTssi LinearTssi;
struct LinearTssi
{
  uint16 usTssiLinear[64];        /*linear scale tssi table */
  uint16 dsTssiLinear[128];       /*linear scale tssi table */
};

#define LINE_GET_LINEAR_TSSI (0x44 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetLinearTssi, LINE_GET_LINEAR_TSSI, uint8, LinearTssi)

#endif /* ATU_C */

/* Error Types for ADSL/VDSL */
#define INTRO_ERROR_TYPE_CRC                                0 /*Currently only CRC is supported*/
#define INTRO_ERROR_TYPE_TX_CORRUPTION                      1
#define INTRO_ERROR_TYPE_RX_CORRUPTION                      2
#define INTRO_ERROR_TYPE_TX_SECOND_CORRUPTION               3
#define INTRO_ERROR_TYPE_TX_PERIOD_CORRUPTION               4
#define INTRO_ERROR_TYPE_RX_PERIOD_CORRUPTION               5
#define INTRO_ERROR_TYPE_RX_SECOND_CORRUPTION               6
#define INTRO_ERROR_TYPE_TX_CW_CORRUPTION                   7
#define INTRO_ERROR_TYPE_TX_LCD                             8
#define INTRO_ERROR_TYPE_RX_LCD                             9
#define INTRO_ERROR_TYPE_TX_DISABLE_BITSWAP                 10
#define INTRO_ERROR_TYPE_RX_DISABLE_BITSWAP                 11
#define INTRO_ERROR_TYPE_TX_DISABLE_SRA                     12
#define INTRO_ERROR_TYPE_RX_DISABLE_SRA                     13
#define INTRO_ERROR_TYPE_RX_ERASURE                         14
#define INTRO_ERROR_TYPE_MOVING_TX_NOISE                    15
#define INTRO_ERROR_TYPE_TX_PERIOD_CORRUPTION_NO_ERASURE    16
#define INTRO_ERROR_TYPE_PHYR_TX_INADVANCE                  17
#define INTRO_ERROR_TYPE_CHANGE_TX_GAIN                     18
#define INTRO_ERROR_TYPE_CHANGE_RX_GAIN                     19
#define INTRO_ERROR_TYPE_RX_DTU_BYTES                       24
#define INTRO_ERROR_TYPE_CHANGE_MARGIN                      27
#define INTRO_ERROR_TYPE_ROTATE_FEQ                         28
#ifdef ATU_R
#define INTRO_ERROR_TYPE_LPR                                29
#endif

/* Error Types for G.fast 
 * The periodic errors (GFAST_RX_PERIODIC & GFAST_TX_PERIODIC) are repeated every superframe.
 * To specify which symbols are impacted, use numberOfErrors and errorPeriodicity as follows:
 *    LSByte of errorPeriodicity: start symbol in logical frame indexing (i.e. 0 is the RMC symbol).
 *    numberOfErrors: the number of symbols impacted in a logical frame.
 *    MSByte of errorPeriodicity: bitmap of the impacted logical frames in a logical superframe (e.g. 0x3 means the first and second frame).
 * For example, this will corrupt every fourth symbol of the second frame in Rx:
 *    introduceErrors(fid, line=3, numberOfErrors=1, errorType=20, errorPeriodicity=(1<<9)|3,verbose=False, trigger=0)
 */
#define INTRO_ERROR_TYPE_GFAST_TX_CORRUPTION                INTRO_ERROR_TYPE_TX_CORRUPTION              /* = 1 - Corruption at toneEncoder input => digital corruption => margin unimpacted */
#define INTRO_ERROR_TYPE_GFAST_RX_CORRUPTION                INTRO_ERROR_TYPE_RX_CORRUPTION              /* = 2 - Corruption at toneDecoder input => analog  corruption => margin degraded   */
#define INTRO_ERROR_TYPE_GFAST_TX_PERIOD_CORRUPTION         INTRO_ERROR_TYPE_TX_PERIOD_CORRUPTION       /* = 4 - Corruption at toneEncoder input => digital corruption => margin unimpacted */
#define INTRO_ERROR_TYPE_GFAST_RX_PERIOD_CORRUPTION         INTRO_ERROR_TYPE_RX_PERIOD_CORRUPTION       /* = 5 - Corruption at toneDecoder input => analog  corruption => margin degraded   */
#define INTRO_ERROR_TYPE_GFAST_CHANGE_TX_GAIN               18  /* Not yet implemented */
#define INTRO_ERROR_TYPE_GFAST_CHANGE_RX_GAIN               19
#define INTRO_ERROR_TYPE_GFAST_RX_PERIODIC                  20                                          /* Corrupt dedicated symbols (frameIdx,symbIdx) of each superframe */
#define INTRO_ERROR_TYPE_GFAST_TX_PERIODIC                  21                                          /* Corrupt dedicated symbols (frameIdx,symbIdx) of each superframe */
#define INTRO_ERROR_TYPE_GFAST_RX_BURST                     22
#define INTRO_ERROR_TYPE_GFAST_TX_BURST                     23
#define INTRO_ERROR_TYPE_GFAST_DTU_BYTES                    24
#define INTRO_ERROR_TYPE_GFAST_TX_SYMB_BYTES                25
#define INTRO_ERROR_TYPE_GFAST_BREAK_RXTONE                 26
#define INTRO_ERROR_TYPE_GFAST_CHANGE_MARGIN                27
#define INTRO_ERROR_TYPE_GFAST_ROTATE_FEQ                   28
#ifdef ATU_R
#define INTRO_ERROR_TYPE_GFAST_LPR                          29
#endif
#define INTRO_ERROR_TYPE_GFAST_FORCE_SRA                    30
#define INTRO_ERROR_TYPE_GFAST_RMCR                         31

#define INTRO_ERROR_TYPE_GFAST_CHANGE_GAIN                  32


#define ENTER_GINP_TEST_MODE                 (0x107)
#define LEAVE_GINP_TEST_MODE                 (0x108)
#define INTRO_ERROR_ENTER_TPS_TEST_MODE      (0x109)
#define INTRO_ERROR_LEAVE_TPS_TEST_MODE      (0x10A)
#define INTRO_ERROR_MODIFY_CFG              (0x8000)

#define CORRUPTION_RANDOM                    0
#define CORRUPTION_ERASURE                   1
#define CORRUPTION_IDLE                      2

#define FORCE_SRA_OFF                           0
#define FORCE_SRA_ONCE                          1
#define FORCE_SRA_CONTINUOUSLY                  2

typedef struct IntroduceErrorsRequest IntroduceErrorsRequest;
struct IntroduceErrorsRequest
{
  uint16 errorType;
  int16  numberOfErrors;
  uint16 errorPeriodicity;      /* Reserved for customers */
  uint16 errorTrigger;          /* Reserved for customers */
  int16  targetMargin;          /* unit is (0.1dB) as itf field */
  int16  downShiftMargin;       /* unit is (0.1dB) as itf field */
  int16  upShiftMargin;         /* unit is (0.1dB) as itf field */
};

#ifndef SUPPORT_HMI  /* Already defined in bcm_hmiLineMsg.h */
#define LINE_INTRODUCE_ERRORS (0x45 | LINE_SERVICE_ID)
#endif
HPI_SERV_DEF(LineIntroduceErrors, LINE_INTRODUCE_ERRORS, IntroduceErrorsRequest, DUMMY)

#define LINE_GOTO_L3 (0x36 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGoToL3, LINE_GOTO_L3, DUMMY, DUMMY)

#define LINE_GOTO_L2 (0x46 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGoToL2, LINE_GOTO_L2, DUMMY, DUMMY)

#define LINE_GOTO_L0 (0x47 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGoToL0, LINE_GOTO_L0, DUMMY, DUMMY)
#ifndef SUPPORT_HMI  /* Already defined in bcm_hmiLineMsg.h */
#define LINE_L2_POWER_TRIM (0x48 | LINE_SERVICE_ID)
#endif
HPI_SERV_DEF(LineL2PowerTrim, LINE_L2_POWER_TRIM, int8, DUMMY)

#if defined (ATU_C) && defined(INM)
 #define LINE_GET_NE_INM_COUNTERS (0x49 | LINE_SERVICE_ID)
 HPI_SERV_DEF(LineGetNeInmCounters, LINE_GET_NE_INM_COUNTERS, DUMMY, InmMachinePersistentState)

 #define LINE_SET_NE_INM_PARAMETERS (0x4A | LINE_SERVICE_ID)
 HPI_SERV_DEF(LineSetNeInmParameters, LINE_SET_NE_INM_PARAMETERS, InmParameters, DUMMY)

 #if defined(INM) && defined(INM_TEST_MSG)
  #define LINE_TEST_NE_ISDD (0x4B | LINE_SERVICE_ID)
 HPI_SERV_DEF(LineTestNeISDD, LINE_TEST_NE_ISDD, IsddTestMsg, DUMMY)
 #endif
#endif

#ifdef HMI_SRA_SUPPORT
 #define LINE_TRIGGER_SRA (0x4B | LINE_SERVICE_ID)
 HPI_SERV_DEF(LineTriggerSRA, LINE_TRIGGER_SRA, TriggerSraMsg, DUMMY)
#endif

 
 #define LINE_SET_EXTRA_CONFIG (0x22 | LINE_SERVICE_ID)
 #define LINE_GET_EXTRA_CONFIG (0x23 | LINE_SERVICE_ID)
 HPI_SERV_DEF(LineSetExtraConfig, LINE_SET_EXTRA_CONFIG, ExtraConfig, DUMMY)
 HPI_SERV_DEF(LineGetExtraConfig, LINE_GET_EXTRA_CONFIG, DUMMY, ExtraConfig)

 
typedef struct BitswapEntry BitswapEntry;
struct BitswapEntry
{
  uint8 toneIndex;
  int8 deltaBi;
  int8 deltaGi;
};

#ifndef SUPPORT_HMI
#define MAX_SPFS_PER_LINE_TRIGGER_OLR 32 /* Should be 128, but limit to 32 to keep the HMI message within bounds */

#ifndef MATLAB_MEX
typedef struct OLRrequestHMI OLRrequestHMI;
struct OLRrequestHMI
{
  uint8 olrType; /*0=Bitswap, 1=SRA*/
  Bool requestPending; /*Reserved from the outside*/
  int16 sraDeltaL; /*SRA only. Accepted only if resulting L is valid.*/
  uint8 numSpfs;
  SubcarrierParameterField spf[MAX_SPFS_PER_LINE_TRIGGER_OLR];
};


 #define LINE_TRIGGER_OLR (0x49 | LINE_SERVICE_ID)
 HPI_SERV_DEF(LineTriggerOlr, LINE_TRIGGER_OLR, OLRrequestHMI, DUMMY)
#endif /* MATLAB_MEX */

#ifdef ADD_FEQ_OUTPUT_NOISE
 #define LINE_ADD_ARTIFICIAL_NOISE (0x4C | LINE_SERVICE_ID)
 HPI_SERV_DEF(LineAddArtificialNoise, LINE_ADD_ARTIFICIAL_NOISE, PsdDescriptorUp, DUMMY)
#endif



/*
   All commands related to the line test feature (0x20 to 0x24) ...
*/

#define LINE_START_LINE_TEST (0x20 | LINE_SERVICE_ID)
/* uint8  indicates the protocol set under test */
HPI_SERV_DEF(LineStartLineTest, LINE_START_LINE_TEST, uint8, DUMMY)

#define LINE_STOP_LINE_TEST  (0x21 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineStopLineTest, LINE_STOP_LINE_TEST, DUMMY, DUMMY)

typedef struct StartLoopCharReqMsg StartLoopCharReqMsg;
struct StartLoopCharReqMsg {
  uint8		logMeasPeriod;
  fix7_1 	agcSetting;
  UserDefinedTssiShapeDn userDefinedTssiShape;
  fix8_8	signalPsd;
  bool		tssiOptim;
  uint8 	forceAgc;
  uint8   echo512tones; /* only useful when ECHO_512_TONES is defined */
  int8 	    echoOffset;
  uint8  	carrierMask[CARRIER_MASK_LEN]; /* only for ADSL */
};

#define LINE_CHARACTERISE_LOOP (0x24 | LINE_SERVICE_ID)
/* uint8 indicates the number of symbol for the broadband measurement =
   1<<(9+X) */
HPI_SERV_DEF(StartLoopCharacterisation, LINE_CHARACTERISE_LOOP, StartLoopCharReqMsg, DUMMY)

typedef struct LoopCharacteristics LoopCharacteristics;
struct LoopCharacteristics
{
  uint32 testStatus; /* 0: test not yet started
                        1: test in progress
                        2: test completed
                     */
};
#endif  /*  */


#define LINE_GET_LOOP_CHARACTERISTIC (0x25 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetLoopCharacteristic, LINE_GET_LOOP_CHARACTERISTIC, DUMMY, LoopCharacteristics)



#define TESTSIGNAL_TYPE_REVERB           3
#define TESTSIGNAL_TYPE_MEDLEY           4
#define TESTSIGNAL_TYPE_QUIET            5
#define TESTSIGNAL_TYPE_VDSL_TRAINING    6
#define TESTSIGNAL_TYPE_VDSL_MEDLEY      7
#define TESTSIGNAL_TYPE_VDSL_QUIET       8
#define TESTSIGNAL_TYPE_VDSL_PERIODIC    9
#define TESTSIGNAL_TYPE_VDSL_XTC        10
#define TESTSIGNAL_TYPE_VDSL_VECTORING  11
#define TESTSIGNAL_TYPE_NOISEGEN_RFI    12
#define TESTSIGNAL_TYPE_GFAST_QUIET     13
#define TESTSIGNAL_TYPE_GFAST_PERIODIC  14
#define TESTSIGNAL_TYPE_GFAST_REVERB    15
#define TESTSIGNAL_TYPE_GFAST_MEDLEY    16




#define SIG_GEN_CONF_TSSI_OPTIM         0x1
#define SIG_GEN_CONF_FIX_CS_ANALOG_GAIN 0x2
#define SIG_GEN_CONF_SINGLE_MEASUREMENT 0x4
#define SIG_GEN_CONF_START_OFF          0x8
#define SIG_GEN_CONF_HISTOGRAM          0x10
#define SIG_GEN_CONF_GFAST_GAP          0x20
#define SIG_GEN_CONF_GFAST_ENABLE_XTC   0x40 
#define SIG_GEN_CONF_GFAST_ZERO_GAIN    0x80

#define FILT_CONF_BYPASS_DEC         0x1

#define FILT_CONF_CPE_TX_A           0x4

#ifdef NOISE_GENERATOR
/* 32bits structure defining duration and attenuation level of a segment
 * played during a noise scenario played by the noise generator */
typedef struct NoiseScenarioSegment NoiseScenarioSegment;
struct NoiseScenarioSegment
{
    fix8_8 dBLevel;
    uint16 duration;
};
#endif

#ifndef SUPPORT_HMI
#define LINE_START_SIGNAL_GEN_MEAS (0x26 | LINE_SERVICE_ID)
HPI_SERV_DEF(StartSignalGenAndMeas, LINE_START_SIGNAL_GEN_MEAS, SignalGeneration, DUMMY)

#define PsdMeasurementGrid (64)

typedef struct SignalMeasurement SignalMeasurement;
struct SignalMeasurement
{
  uint32 testStatus; /* 0: test not yet started
                        1: test in progress
                        2: test completed
                     */
  uint32  testDuration;  /* Duration in seconds of the peak
                            noise measurement (reported below) */
  /* all PSD values are expressed in dBm referred to 100Ohm line / Tonebin */
  fix8_8  peakPsd[PsdMeasurementGrid];   /*  max (x^2+y^2)/T, where T = peakMeasurementTime seconds */
  fix8_8  totalPsd[PsdMeasurementGrid];   /*  sum(x^2+y^2)/T,  where T = 1 sec.      */
  fix8_8  signalPsd[PsdMeasurementGrid];   /* (sum(x)^2 sum(y)^2)/T, where T = 1 sec */
  uint8   partId; /* introduced for VDSL -> measurement is transported in 26 chunks of 64 tones - acknowledgement of partId in request */
  uint8   toneSpacing; //0 = 4KHz, 1 = 8KHz, 2 = 51.75 KHz
  uint8   reserved[2];
};

typedef struct SignalMeasurementRequest SignalMeasurementRequest;
struct SignalMeasurementRequest
{
  uint8 partId; /* valid range = [0:25], total measurement vector is 26*64 long, and is retrieved in chunks of 64 tones */
  uint8 toneGroupingFactor; /* valid range = [0:4], to speed up retrieval process, only one tone in (1<<toneGroupingFactor) will be returned */
};


#define LINE_GET_SIGNAL_MEASUREMENT (0x27 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetSignalMeasurement, LINE_GET_SIGNAL_MEASUREMENT, SignalMeasurementRequest, SignalMeasurement)


#define LINE_START_MANUF_TEST  (0x28 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineStartManufTest, LINE_START_MANUF_TEST, ManufTestConfig, DUMMY)


#define LINE_GET_MANUF_TEST_RESULT   (0x29 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetManufTestRes, LINE_GET_MANUF_TEST_RESULT, DUMMY, ManufTestResult)

typedef struct AgcTest AgcTest;
struct AgcTest
{
  fix8_8 agc;
};

#define LINE_GET_AGC_TEST  (0x2A | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetAgcTest, LINE_GET_AGC_TEST, DUMMY, AgcTest)

#define LINE_GET_FE_PILOTS  (0x2B | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetFePilots, LINE_GET_FE_PILOTS, DUMMY, VdslMsgToneDescriptor)

typedef struct AccurateEcho AccurateEcho;
struct AccurateEcho
{
  int32 echoValues[64];
};

#define LINE_GET_ECHO  (0x2D | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetEcho, LINE_GET_ECHO, uint32, AccurateEcho)

typedef struct AccurateEchoVariance AccurateEchoVariance;
struct AccurateEchoVariance
{
  int16 varValues[128];
};

#define LINE_GET_ECHO_VARIANCE (0x2E | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetEchoVariance,LINE_GET_ECHO_VARIANCE, uint32, AccurateEchoVariance)

typedef struct SeltResult SeltResult;
struct SeltResult
{
  int32 seltValues[64];
};

#define LINE_GET_SELT_RESULT (0x30 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetSeltResult,LINE_GET_SELT_RESULT, uint32, SeltResult)

#define OAM_current15min    0
#define OAM_previous15min   1
#define OAM_current24hour   2
#define OAM_previous24hour  3

typedef struct PeriodIdentification PeriodIdentification;
struct PeriodIdentification
{
  uint32   periodId;
};

typedef struct PeriodCounters PeriodCounters;
struct PeriodCounters
{
  /* line level information */
  DerivedSecCounters  adslDerivCounts[2]; /* NE/FE */
  /* individual bearers */
  AdslAnomCounters    adslAnomCounts[2];   /* B0/B1 */
  AtmPerfCounters     atmPerfCounters[2];  /* B0/B1 */


  uint16   failedFullInitCount;      /* incremented each time an exception is
                                        reported in a non showtime state while in full init */
  uint8    fullInitCount;            /* Gfast [g997.2 ($7.7.13)/g9701 ($11.3.1.5)]: incremented each time FTU-O transitions from O-SILENT to O-INIT/HS state.
                                        xdsl [g997.1 ($7.2.1.3.1)]: incremented at each initialization attempt (successful and failed) */
  uint8    reInitCount;              /* incremented each time we start a new training
                                        within 20s after a showtime drop */
  uint16   fastInitCounter;          /* incremented each time training state is entered from showtime */
  uint16   failedFastInitCounter;    /* incremented each time an exception is
                                        reported in a non showtime state while in fast init */
  uint16   periodDuration;           /* indicates how long we have done
                                        monitoring in this period*/
  uint8    suspectFlagNE;            /* the suspect flag is activated either
                                       - the first time PM is read after a
                                         Santorini reset or line reset.
                                       - when the elapsed time between two
                                         reads is bigger than 1 hour.    */
  uint8    suspectFlagFE;            /* the suspect flag is activated either
                                       - the first time PM is read after a
                                         Santorini reset or line reset.
                                       - when the elapsed time between two
                                         reads is bigger than 1 hour.
                                       - the FE statistics are not reliable
                                         because of a IB were not reliable at
                                         some point in time during the last
                                         monitoring period.               */
  uint32   upTime;                   /* nr of seconds already in showtime */
  uint32   periodId;
  uint32   elapsedTimeIn15min;
  uint32   elapsedTimeIn24hour;
  GinpCounters ginpCounters[2];
  GfastRtxCounters gfastCounters[2];
};
#endif  /* SUPPORT_HMI */



#define LINE_GET_PERIOD_COUNTERS (0x31 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPeriodCounters,LINE_GET_PERIOD_COUNTERS, PeriodIdentification, HmiPeriodCounters)

typedef struct ThresholdsSet ThresholdsSet;
struct ThresholdsSet
{
  OAMthresholds   threshold15min[2]; /* NE-FE */
};

#define LINE_SET_ALARM_THRESHOLD (0x32 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetAlarmThreshold,LINE_SET_ALARM_THRESHOLD, ThresholdsSet, DUMMY)

#ifdef ATU_R

#define LINE_GENERATE_TX_ERROR_BURST (0x4A | LINE_DEBUG_ID)
HPI_SERV_DEF(LineGenerateTxErrorBurst, LINE_GENERATE_TX_ERROR_BURST, uint8, DUMMY)

typedef struct PerfDbgCount PerfDbgCount;
struct PerfDbgCount
{
  int32 accumulationTime;
  int32 toneDecoderErasures;
  int32 pilotErasures;
  uint32 pilotMse;
  uint32 showtimeScale;
  int32  freqOffset;
  int32  settlingTime;
  int16  feqI;
  int16  feqQ;
  int16  pilotIndex;
  fix8_8 std_pe;
  int8   owner;
  int8   reserved[3];
};

#define LINE_GET_DBG_COUNTERS (0x30 | LINE_DEBUG_ID)
HPI_SERV_DEF(LineGetPerfDbg, LINE_GET_DBG_COUNTERS,DUMMY,PerfDbgCount)

#endif /* ATU_R */

#ifdef ART_NOISE

typedef struct AppliedVirtualNoise AppliedVirtualNoise;
struct AppliedVirtualNoise
{
  uint8 noise[256];
};

#define LINE_GET_VIRT_NOISE (0x4D | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetVirtNoise, LINE_GET_VIRT_NOISE, uint8, AppliedVirtualNoise)

#endif /* ART_NOISE */


#define CPE_VENDOR_INFO_HS_AVAIL   1
#define CPE_VENDOR_INFO_ANSI_AVAIL 2
#define CPE_VENDOR_INFO_BCM_AVAIL  4
#define CPE_VENDOR_INFO_3RD_PARTY_AVAIL  8
#ifdef SUPPORT_HMI /* HsNegotiation.h */
typedef struct CPEParametersInfo CPEParametersInfo;
struct CPEParametersInfo
{
  uint32 minBitRate;
  uint32 maxBitRate;
  uint8  maxDelay;
  uint8  INPmin;
  uint8  msgMin;
  uint8  tpsTcType;
};
#endif
typedef struct CPEvendorInfo CPEvendorInfo;
struct CPEvendorInfo
{
  VendorId  hsVendorInfo;
  uint16    t1_413_vendorInfo;
  uint8     availInfo; /* bitmap indicating which info is available
                          bit [0] : hs available
                          bit [1] : t1_413_vendorInfo
                          bit [2] : bcm nsif version info
                          bit [3] : 3rd party nsif version info (unformatted)
                       */
  uint8     interopInfo;
  CPEParametersInfo cpeInfo[2]; /* US/DS */
  uint8     majVersion;
  uint8     minVersion;
  uint8     extraVersion;
  uint8     chipType;
};


#define LINE_GET_CPE_VENDOR_INFO (0x33 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetCPEvendorInfo, LINE_GET_CPE_VENDOR_INFO, DUMMY, CPEvendorInfo)



#define BGS_BUFF_LEN 368 /* multiple of 8 for gi retrieval */

typedef struct BitAllocationTable BitAllocationTable;
struct BitAllocationTable
{
  uint8  bitAllocTable[BGS_BUFF_LEN*2];
};

typedef struct SnrTable SnrTable;
struct SnrTable
{
  fix8_8 snr[BGS_BUFF_LEN];
};

typedef struct CarrierGainTable CarrierGainTable;
struct CarrierGainTable
{
  uint16 gi[BGS_BUFF_LEN];        /*linear scale gain table */
};


typedef struct BSGrequest BSGrequest;
struct BSGrequest
{
  uint8  downstream; /* for LineGetSnr message
                        . bit 7 is used to request an echo variance estimate (only available in LD_COMPLETE)
                        . bit 6 flags that the desired metric is margin iso SNR (only available in showtime)
                      */
  uint8  partId;     /* only used when startTone==0 && nTones==0 */
  uint16 startTone; 
  uint16 nTones;
  uint8  log2M;      /* log2M!=0 only applicable for LineGetBitAllocation message */
  uint8  includeRi;
  uint8  isDoi;
  uint8  isBackupRmc;
  uint8  reserved[4];
};

#define LINE_GET_BIT_ALLOCATION (0x2F | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetBitAllocation, LINE_GET_BIT_ALLOCATION, BSGrequest, BitAllocationTable)

#define LINE_GET_SNR            (0x37 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetSnr,           LINE_GET_SNR,            BSGrequest, SnrTable)

#define LINE_GET_CARRIER_GAINS (0x43 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetCarrierGains,  LINE_GET_CARRIER_GAINS,  BSGrequest, CarrierGainTable)

#define MAX_RMC_TONES_HMI 512
#define RMC_BUFF_LEN      240
typedef struct RmcBitAlloc
{
  uint32 nTonesRmc;
  uint8  bi[RMC_BUFF_LEN];
  uint16 rts[RMC_BUFF_LEN];
}  RmcBitAlloc;

#define LINE_GET_RMC_BITALLOC (0x74 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetRmcBitAlloc,  LINE_GET_RMC_BITALLOC,  BSGrequest, RmcBitAlloc)

typedef struct AfeDebugCommand AfeDebugCommand;
struct AfeDebugCommand
{
  uint8  writeIndicator;  /* 1 if doing a write, 0 for a read */
  uint16 address;         /* Opala Register */
  uint16 data;            /* value is don't care in case of a read */
};

typedef struct AfeDebugReply AfeDebugReply;
struct AfeDebugReply
{
  uint8  writeIndicator; /* Echo of counterpart in command */
  uint16 address;        /* Echo of counterpart in command */
  uint16 data;           /* value is don't care in case of a write */
};

#define LINE_EXECUTE_AFE_DEBUG_COMMAND (0x2C | LINE_DEBUG_ID)
HPI_SERV_DEF(LineExecAfeDebugCmd,LINE_EXECUTE_AFE_DEBUG_COMMAND,AfeDebugCommand,AfeDebugReply)

#ifdef ATU_R

  /*
    if freq jump, offset is in ppm, fix8_8
    if phase jump, offset is in samples, expressed in fix1_15
    
    freq jump will adapt LoopFilter freq register
    phase jump will adapt PhaseDetector input constellation point
  */

typedef struct ForcePllCmd ForcePllCmd;
struct ForcePllCmd
{
  uint16 isPhaseJump; 
  int16  offset;       
};


#define LINE_FORCE_PLL (0x2B | LINE_DEBUG_ID)
HPI_SERV_DEF(LineForcePll,LINE_FORCE_PLL,ForcePllCmd,DUMMY)

#else

#define LINE_SET_PTM_BONDING_CONFIG (0x4e | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetPtmBondingConfig, LINE_SET_PTM_BONDING_CONFIG, PtmBondingConfig, DUMMY)

#define LINE_GET_PTM_BONDING_CONFIG (0x4f | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPtmBondingConfig,LINE_GET_PTM_BONDING_CONFIG,DUMMY,PtmBondingConfig)

#define LINE_GET_PTM_BONDING_REGISTERS (0x50 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPtmBondingRegisters,LINE_GET_PTM_BONDING_REGISTERS,DUMMY,PtmBondingRegisters)

#endif

#ifndef SUPPORT_HMI  /* Already defined in bcm_hmiLineMsg.h */
typedef struct FeRawOamCounters FeRawOamCounters;
struct FeRawOamCounters
{
  uint16  es;
  uint16  ses;
  uint16  loss;
  uint16  fecs;
  uint16  uas;
  uint8   upTime; // upTime when we updated the local counters for the last
                  // time
  uint8   reserved;
};
#endif
  
#define LINE_GET_FE_RAW_OAM_COUNTERS (0x67 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetFeRawOamCounters,LINE_GET_FE_RAW_OAM_COUNTERS,DUMMY,FeRawOamCounters)



  
#if defined(VECTORING)
#ifdef ATU_C
typedef struct VectoringInitConfig VectoringInitConfig;
struct VectoringInitConfig
{
  uint8  log2M;        /* log2M contains
                          log2M[2:0]: minimal downsampling of error samples
                          during DS_INIT_2_S
                          log2M[7]: indicates the uses of fixed or floating point
                          during DS_INIT_2_S (0: fixed point (8bits) 1: floating point (6+6+4)) */
  uint8  direction; /* 0: US and DS
                       1: US only / 2: DS only */
  uint8  fdpsMode; /* 0: no FDPS
                      1: 8*64 (number of pilot sequences * pilot sequence length)
                      2: 5*128
                      3: 2*256 */
  uint8  reserved;
  uint8  initPilotSequence[PILOT_SEQUENCE_LEN_FDPS_HMI>>3];
  uint16 lineIdDs;
  uint16 lineIdUs;
  uint8  bMax;
  uint8  reservedB[3];
};

typedef struct VectoringInitConfigExt VectoringInitConfigExt;
struct VectoringInitConfigExt
{
  uint8  log2M;
  uint8  direction;
  uint8  fdpsMode; /* 0: no FDPS
                      1: 8*64 (number of pilot sequences * pilot sequence length)
                      2: 5*128
                      3: 2*256
                      4: 5*256
                      5: 5*512 */
  uint8  reserved;
  uint8  initPilotSequence[PILOT_SEQUENCE_LEN_FDPS_EXT>>3];
  uint16 lineIdDs;
  uint16 lineIdUs;
  uint8  bMax;
  uint8  reservedB[3];
};

typedef struct VectoringInitConfig35b VectoringInitConfig35b;
struct VectoringInitConfig35b
{
  uint8  log2M;
  uint8  direction;
  uint8  fdpsMode;              /* mode 0 to 5 */
  uint8  use35bLowAndHighBands; /* use a separate pilot sequence for low and high bands in 35b */
  uint8  initPilotSequence[PILOT_SEQUENCE_LEN_FDPS_35B>>3]; /* low band pilot sequence followed by high band */
  uint16 lineIdDs;
  uint16 lineIdUs;
  uint8  bMax;
  uint8  reservedB[3];
};

#define LINE_SET_PILOT_SEQUENCE    (0x51 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetPilotSequence,LINE_SET_PILOT_SEQUENCE,VectoringInitConfig35b,DUMMY)

typedef struct ChangePilotSeqMsg ChangePilotSeqMsg;
struct ChangePilotSeqMsg
{
  uint8 newPilotSequence[PILOT_SEQUENCE_LEN_FDPS_HMI>>3];
  uint8 direction; /* 0: US and DS
                      1: US only / 2: DS only */
  uint8 fdpsMode; /* 0: no FDPS
                     1: 8*64 (number of pilot sequences * pilot sequence length)
                     2: 5*128
                     3: 2*256 */
  uint8 noInterruptionGfast;
  uint8 reserved;
};

typedef struct ChangePilotSeqMsgExt ChangePilotSeqMsgExt;
struct ChangePilotSeqMsgExt
{
  uint8 newPilotSequence[PILOT_SEQUENCE_LEN_FDPS_EXT>>3];
  uint8 direction;
  uint8 fdpsMode; /* 0: no FDPS
                     1: 8*64 (number of pilot sequences * pilot sequence length)
                     2: 5*128
                     3: 2*256
                     4: 5*256
                     5: 5*512 */
  uint8 noInterruptionGfast;
  uint8 reserved;
};

typedef struct ChangePilotSeqMsg35b ChangePilotSeqMsg35b;
struct ChangePilotSeqMsg35b
{
  uint8 newPilotSequence[PILOT_SEQUENCE_LEN_FDPS_35B>>3];
  uint8 direction;
  uint8 fdpsMode;              /* mode 0 to 5 */
  uint8 noInterruptionGfast;
  uint8 use35bLowAndHighBands; /* use a separate pilot sequence for low and high bands in 35b */
};

#define LINE_CHANGE_PILOT_SEQUENCE (0x66 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineChangePilotSequence,LINE_CHANGE_PILOT_SEQUENCE,ChangePilotSeqMsg35b,DUMMY)
#endif /* ATU_C */

typedef struct VectoringCountersMsg VectoringCountersMsg;
struct VectoringCountersMsg
{
  EsCounters esCounters;
  uint16     isDsFeValid;
  uint8      suspectLostDsVf;                   /* G.fast only */
  uint8      suspectInconsistencyProbeSeqRx;    /* G.fast only */
  uint32     countDroppedDsEs;                  /* VDSL only */
  uint8      timedOutL3Requests;                /* G.fast only */
  uint8      reservedB1[3];
  uint32     reservedB2[2];
};
#define LINE_GET_VECTORING_COUNTERS (0x68|LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetVectoringCounters,LINE_GET_VECTORING_COUNTERS,DUMMY,VectoringCountersMsg)


#define LOW_DS_BACK_CHANNEL_CAP (1)
#define NO_DS_BACK_CHANNEL_IN_INIT (2)
#define CORRUPT_BACK_CHANNEL (3)
  
typedef struct VectoringErrorMsg VectoringErrorMsg;
struct VectoringErrorMsg{
  uint32 id;             /* Indicate the type of errors
                            0 = disable
                            LOW_DS_BACK_CHANNEL_CAP = indicate that the
                            request log2M is NOK.
                            NO_DS_BACK_CHANNEL_IN_INIT = do not send any DS
                            packet during init.
                            CORRUPT_BACK_CHANNEL = indicate that sync symbols
                            measurement is corrupted the fraction of corrupted
                            values is specified by the mask. */
  uint8  mask;
  uint8  reserved[3];
};

#define LINE_CONFIG_LEGACY_PILOT (0x69|LINE_SERVICE_ID)
HPI_SERV_DEF(LineConfigLegacyPilot,LINE_CONFIG_LEGACY_PILOT,LegacyPilotConfig,DUMMY)

#define LINE_SWITCH_STATE_SILENT (0x6A|LINE_SERVICE_ID)
HPI_SERV_DEF(LineSwitchStateSilent,LINE_SWITCH_STATE_SILENT,int32,DUMMY)
  
#define LINE_SET_VECTORING_ERROR (0x6B|LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetVectoringError,LINE_SET_VECTORING_ERROR,VectoringErrorMsg,DUMMY)

#define INFORM_VECT_CONV_COMPLETED (0x6C|LINE_SERVICE_ID)
HPI_SERV_DEF(InformVectConvergenceCompleted,INFORM_VECT_CONV_COMPLETED,uint8,DUMMY)

typedef struct StartVectorDump StartVectorDump;
struct StartVectorDump
{
  uint8        direction; /* 0: upstream 1: downstream                    */
  uint8        measurementId; /* measurement ID for G.fast (4LSB  will be set as 4 MSB
                                 of the lineId if measurementId!=0) */
  int8         lastReportedBand; /* Band from 0 to Nbands-1, -1: stop reporting   */
  uint8        log2M;
  uint8        log2q;    /* G.fast update period */
  uint8        vfType;   /* G.fast VF type:
                            0: DFT output (24 bits per tone)
                            1: Error Samples (24 bits per tone)
                            2: Error Samples (16 bits per tone for DS VF, converted to 24 bits per tone for VCE)
                            3: Error Samples (12 bits per tone for DS VF, converted to 24 bits per tone for VCE)
                            */
  uint16       lineIdDs; /* must be set to 0xFFFF on a G.vector line : only
                            used on legacy line to dump data symbol. */  
};




#ifdef ATU_C

typedef struct StartVectorDumpResponse StartVectorDumpResponse;
struct StartVectorDumpResponse
{
  uint16 log2M;
  uint8  q;	         /* updatePeriod */
  uint8  zLSB;	     /* shiftPeriod  */
  uint16 psIndex512; /* current DS pilot sequence index (512 cycle) */
  uint16 psIndex4N;  /* current DS pilot sequence index (4*N cycle) */
  uint8  bMax;
  uint8  esTypeGfast;
  uint8  zMSB;       /* shiftPeriod  */
  uint8  reserved;
};

#define LINE_START_VECTOR_DUMP     (0x52 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineStartVectorDump,LINE_START_VECTOR_DUMP,StartVectorDump,StartVectorDumpResponse)
#endif /* ATU_C */

typedef struct VectorMode VectorMode;
struct VectorMode
{
  uint8 direction;  /* 0 upstream 1: downstream */
  uint8 disableRxBitSwap;
  uint8 disableVN;
  uint8 reservedA;
};

typedef struct RuntimeCommand RuntimeCommand;
struct RuntimeCommand
{
  uint8 direction;  /* 0 upstream 1: downstream */
  uint8 commandId;
  uint8 reserved[2];
  uint8 data[100];
};

/* Runtime command ID */
#define RUNTIME_CMD_REENABLE_BITSWAP    0
#define RUNTIME_CMD_DISABLE_BITSWAP     1
#define RUNTIME_CMD_VN_MODE             2
#define RUNTIME_CMD_REENABLE_DSRA       3
#define RUNTIME_CMD_DISABLE_DSRA        4

#ifdef ATU_C
#define LINE_SET_VECTOR_MODE    (0x54 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetRuntimeCommand,LINE_SET_VECTOR_MODE,RuntimeCommand,DUMMY)
#endif

#ifdef ATU_C
#define LINE_RESET_PRECODER (0x55 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineResetPrecoder,LINE_RESET_PRECODER,DUMMY,DUMMY)

#define TONES_GAIN_SEGMENT (352)
typedef struct GainSegment GainSegment;
struct GainSegment
{
  int16 currentPsd;
  uint16 mapValXY;
  uint16 txGain[TONES_GAIN_SEGMENT];
};

typedef struct GainMsg GainMsg;
struct GainMsg
{
  uint16 startTone;
  uint8  log2M;
  uint8  includeRi;
};

#define LINE_GET_TX_GAINS    (0x56 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetTxGains,LINE_GET_TX_GAINS,GainMsg,GainSegment)

typedef struct VectorFlagMsg VectorFlagMsg;
struct VectorFlagMsg
{
  uint32 flag;          /* Definition of bits
                          flag[0]: avoid the modem to return to V_IDLE after
                          a successfull entry in V_INIT state
                          flag[1]: allows to transition from V_INIT_DS1/V_INIT to
                          V_TRANS
                          flag[2]: allows to transition from V_INIT_DS2 to V_TRANS
                          flag[3]: allows to transition from V_INIT_US1 to
                          V_TRANS
                          flag[4]: allows to transition from V_INIT_US2 to
                          V_TRANS
                          flag[5-6]:  reserved for future use
                          flag[7]: forbid the modem to start handshake after a
                          startLine or an autonomous restart. Modem will enter
                          V_WAIT_FOR_START
                          flag[8]: allows to transition from V_INIT_DS_2A to
                          V_INIT_DS_2 in a synchronous way.
                          flag[15]: indicates that the modem must start to
                          send QUIET signal till handshake is restarted
                       */
  uint8 reserved[2];
  int16 delay;         /* Delay before changing of phase
                          -1: Used forced value
                          0 : No delay
                          [1 to 1023]: delay in sync symbols */
  uint16 forcedValue;
  uint8 vceBit;        /* bit[0]: Set to 0 when called from G.vector VCE
                                  Set to 1 when called from G.fast   VCE
                       */
  uint8  reservedA;
  uint16 duration_cd_1_1; /* duration of O-P-CHANNEL-DISCOVERY-1-1 in SF */
  uint16 reservedB;
};
  
#define LINE_SET_VECTOR_COMPLETION_FLAG    (0x57 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetVectorCompletionFlag,LINE_SET_VECTOR_COMPLETION_FLAG,VectorFlagMsg,int16)

#define MAX_PRECODER_MSG_SIZE 224
#define MAX_LOG2_DOWNSAMPLING 4

typedef struct VecCoeff  VecCoeff;
struct VecCoeff
{
  int8   mantx;
  int8   manty;
  uint8  exp;
};

typedef struct PrecoderMsg PrecoderMsg;
struct PrecoderMsg
{
  uint8  log2M; /* Downsampling of coefficients by 2^M*/
  uint8  format; // compression format 0 means the coefficients are
                 // encapsulated in uint16:6,6,4. If format is 1, it means
                 // coeff is uint8[3] 8,8,4
  uint16 startTone; // index of the first coefficient (takes into account
                    // DS/US coeffs : first coef DS is at position 0
  uint16 nTones; // number of coefficients
  uint8  xtalkId; // index within the framer
  uint8  reserved;
  VecCoeff coef[MAX_PRECODER_MSG_SIZE];
};

  
// struct { exp, mantX, mantY }

#define LINE_SET_PRECODER    (0x58 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetPrecoder,LINE_SET_PRECODER,PrecoderMsg,DUMMY)

#ifdef GFAST_SUPPORT
#define PILOT_SEQUENCE_FAST_LEN_HMI (128)
typedef struct VectoringInitConfigGfast VectoringInitConfigGfast;
struct VectoringInitConfigGfast{
  uint16  lineIdDs;
  uint16  lineIdUs;
  /* Minimal VF samples configuration to be used during VF_DS_INIT_2_S
     If SOC capacity is too low, q is increased to meet the channel capacity
   */
  uint8   log2Fsub;     /* Frequency subsampling          */
  uint8   log2q;        /* Time frequency parameter (q,z), z is derived from psSeqLength/q */
  uint8   vfType;       /* G.fast VF type:
                           0: DFT output (24 bits per tone)
                           1: Error Samples (24 bits per tone)
                           2: Error Samples (16 bits per tone for DS VF)
                           3: Error Samples (12 bits per tone for DS VF) -> not supported
                           */
  uint8   psSeqLengthReporting;
  /* Pilot sequence setting */
  uint8   direction;    /* 0: US and DS
                           1: US only, 2: DS only */
  uint8   psSeqLength;  /* Pilot sequence length, fix in showtime */
  uint8   gfastEStype;  /* 0: VDSL-style ES
                           1: G.fast compliant ES
                           2: Both supported */
  uint8   reservedB;
  /* Pilot sequence is coded as 2 bits per elements (00: quiet, 01: +1, 10:
     -1, 11: forbidden. The 1st element is mapped on the 2 lsb's of the
     first octet, 2nd element is mapped on the 2 lsb's of the second octets,
     etc.
  */
  uint8   initPilotSequence[(PILOT_SEQUENCE_FAST_LEN_HMI*2)>>3];
};

typedef struct VectoringInitConfigGfastExt VectoringInitConfigGfastExt;
struct VectoringInitConfigGfastExt{
  uint16  lineIdDs;
  uint16  lineIdUs;
  /* Minimal VF samples configuration to be used during VF_DS_INIT_2_S
     If SOC capacity is too low, q is increased to meet the channel capacity
   */
  uint8   log2Fsub;     /* Frequency subsampling          */
  uint8   log2q;        /* Time frequency parameter (q,z), z is derived from psSeqLength/q */
  uint8   vfType;       /* G.fast VF type:
                           0: DFT output (24 bits per tone)
                           1: Error Samples (24 bits per tone)
                           2: Error Samples (16 bits per tone for DS VF)
                           3: Error Samples (12 bits per tone for DS VF) -> not supported
                           */
  uint8   psSeqLengthReporting;
  /* Pilot sequence setting */
  uint8   direction;    /* 0: US and DS
                           1: US only, 2: DS only */
  uint8   psSeqLength;  /* Pilot sequence length, fix in showtime */
  uint8   gfastEStype;  /* 0: VDSL-style ES
                           1: G.fast compliant ES
                           2: Both supported */
  uint8   reservedB;
  /* Pilot sequence is coded as 2 bits per elements (00: quiet, 01: +1, 10:
     -1, 11: forbidden. The 1st element is mapped on the 2 lsb's of the
     first octet, 2nd element is mapped on the 2 lsb's of the second octets,
     etc.
  */
  uint8   initPilotSequence[(PROBE_SEQUENCE_LEN*2)>>3];
  uint8   reservedC[8];
};

#define LINE_SET_VECTORING_CONFIG_FAST (0x71 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetVectoringConfigGfast,LINE_SET_VECTORING_CONFIG_FAST,VectoringInitConfigGfastExt,DUMMY)

typedef struct PrecoderMsgHdr PrecoderMsgHdr;
struct  PrecoderMsgHdr
{
  uint64  xtalkIds;
  int16   startTone;
  int16   nrOfTones;
  uint8   installNow; 
  uint8   isDs;
  uint8   nrOfDisturbers;
  uint8   reset; // 1 for reset, 2 for read
};



#define MAX_NO_GFAST_PRECODER_COEFS_ON_HMI ((HPI_MSG_SIZE-4*HPI_HEADER_LENGTH-2*8)/4) // = 512+256-4*6-2*8 = 728 -> 182 coefs

typedef struct PrecoderMsgGfast PrecoderMsgGfast;
struct PrecoderMsgGfast
{
  PrecoderMsgHdr header;
  uint8          coef[MAX_NO_GFAST_PRECODER_COEFS_ON_HMI*4];  
  
  // 28 bits packed into 4 octets coef[0]coef[1]coef[2]coef[3]
  // coef[0]&f                   -> exp
  // coef[0]>>4 + coef[1]<<4     -> manty
  // coef[2]    + (coef[3]&f)<<8 -> mantx

};

typedef struct PrecoderGfastCoefBuff PrecoderGfastCoefBuff;
struct PrecoderGfastCoefBuff
{
  uint8          coef[MAX_NO_GFAST_PRECODER_COEFS_ON_HMI*4];  
  // 28 bits packed into 4 octets coef[0]coef[1]coef[2]coef[3]
  // coef[0]&f                   -> exp
  // coef[0]>>4 + coef[1]<<4     -> manty
  // coef[2]    + (coef[3]&f)<<8 -> mantx
};



#define LINE_SET_PRECODER_GFAST      (0x6F | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetPrecoderGfast,LINE_SET_PRECODER_GFAST,PrecoderMsgGfast,DUMMY)
#define LINE_RESET_PRECODER_GFAST    (0x70 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineResetPrecoderGfast,LINE_RESET_PRECODER_GFAST,PrecoderMsgHdr,DUMMY)
#define LINE_GET_PRECODER_GFAST      (0x73 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPrecoderGfast,LINE_GET_PRECODER_GFAST,PrecoderMsgHdr,PrecoderGfastCoefBuff)

#define TIGAREQ_MSG_BIRI_SIZE 256
#define TIGAREQ_MSG_EXP_UNITY 12

typedef struct RxGainUpdateReqMsg{
  uint8   resetCache;
  uint8   interpolation;
  uint8   direction;
  uint8   isDOI;
  uint16  startTone;    /* Inclusive */
  uint16  endTone;      /* Inclusive */
  uint16  biri[TIGAREQ_MSG_BIRI_SIZE];
  uint16  maskedPilotTone;
  uint32  intBits[(TIGAREQ_MSG_BIRI_SIZE+1+31)/32];/* indicate the type of interpolation per edge
                                                      2 additional bits for the first and last edge
                                                      0: indicate 0-order interpolation.
                                                      1: indicate 1-order interpolation.
                                                      This is only for interpolation=4 and on YSTB
                                                   */
} RxGainUpdateReqMsg;

#define LINE_RXGAIN_UPDATEREQ_GFAST (0x72 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineRxGainUpdateRequest,LINE_RXGAIN_UPDATEREQ_GFAST,RxGainUpdateReqMsg,DUMMY)


// 48 breakpoints
// unit for PSD val is -1/256
// no more than 32 breakpoints should be defined for 'LineSetVectoringPsdGfast'
// LineGetVectoringPsdGfast can return up to 48 breakpoints
#define LINE_SET_VECTORING_PSD_GFAST (0x75 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetVectoringPsdGfast,LINE_SET_VECTORING_PSD_GFAST,DsMrefPsdDescriptor,DUMMY)

#define LINE_GET_VECTORING_PSD_GFAST (0x76 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetVectoringPsdGfast,LINE_GET_VECTORING_PSD_GFAST,DUMMY,DsMrefPsdDescriptor)

#define MAX_AFE_CAL_DESC_BP 32
typedef struct CalibrationDescriptor CalibrationDescriptor;
struct CalibrationDescriptor
{
  uint8         maxNbBP;
  uint8         n;
  BreakPoint    bp[MAX_AFE_CAL_DESC_BP];
};

typedef struct GetCalibrationRep GetCalibrationRep;
struct GetCalibrationRep{
  int32 refPsd;
  CalibrationDescriptor calDesc;
};

typedef struct GetCalibrationMsg GetCalibrationMsg;
struct GetCalibrationMsg
{
  uint8  downstreamDirection;
  uint8  reserved[3];
};

// command only valid in GFAST Training or Showtime
// Tone indices in units of 51.75KHz

#define LINE_GET_TIME_DOMAIN_FILTERING  (0x77 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetTimeDomainFiltering,LINE_GET_TIME_DOMAIN_FILTERING,GetCalibrationMsg,GetCalibrationRep)


typedef struct RxGainsUpdateStatusDescriptor{
  uint16  actualSfCountApply;
  uint16  actualSfCountRequest;
  uint8   failureCause;
  uint8   d_tigaResp;
  uint8   reserved[6];
} RxGainsUpdateStatusDescriptor;

typedef struct GetRxGainsUpdateStatusMsg{
  uint8   direction;
  uint8   reserved[3];
} GetRxGainsUpdateStatusMsg;

#define LINE_RXGAINUPDATE_STATUS_GFAST (0x78 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetRxGainsUpdateStatus,LINE_RXGAINUPDATE_STATUS_GFAST,GetRxGainsUpdateStatusMsg,RxGainsUpdateStatusDescriptor)

#define MAX_NR_OF_TONES_FAST_HMI (4096)
#if MAX_NR_OF_TONES_FAST_HMI<MAX_GFAST_TONES
# error "Not enough tones in G.fast on HMI"
#endif
  
typedef struct GfastToneMaskReqMsg{
  uint8   direction;
  uint8   resetCache;
  uint16  startTone;    /* Inclusive */
  uint16  endTone;      /* Inclusive */
  uint8   reservedA[6];
  uint8   toneMask[(MAX_NR_OF_TONES_FAST_HMI+7)/8]; /* 1 bit per tone */
} GfastToneMaskReqMsg;

#define LINE_SET_TONEMASK_GFAST (0x79 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetToneMask,LINE_SET_TONEMASK_GFAST,GfastToneMaskReqMsg,DUMMY)

#define LINE_SET_LOGICAL_FRAME_CONFIG (0x7A | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetLogicalFrameConfig, LINE_SET_LOGICAL_FRAME_CONFIG,LogicalFrameCfg,LogicalFrameCfg)

#if (CHIPSET >= YST_DEV)
#define LINE_CHANGE_MDS (0x7E | LINE_SERVICE_ID)
HPI_SERV_DEF(LineChangeMds, LINE_CHANGE_MDS, DtaRequest, DUMMY)

#define LINE_SET_DTA_CONFIG (0x7F | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetDtaConfig, LINE_SET_DTA_CONFIG, DtaConfig, DUMMY)

#define LINE_GET_DTA_CONFIG (0x80 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetDtaConfig, LINE_GET_DTA_CONFIG, DUMMY, DtaConfig)
#endif

#define LINE_SET_SELT_COMPATIBLE_MODE (0x81 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetSeltCompatibleMode, LINE_SET_SELT_COMPATIBLE_MODE, uint8, DUMMY)
#endif

typedef struct GetPrecoderMsg GetPrecoderMsg;
struct GetPrecoderMsg
{
  uint16 startTone;
  uint16 nTones;
  uint8  xtalkId;
  uint8  reserved[3];
};
typedef struct PrecoderCoefMsg PrecoderCoefMsg;
struct PrecoderCoefMsg
{
  VecCoeff coef[MAX_PRECODER_MSG_SIZE];
};

#define LINE_GET_PRECODER (0x5D | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPrecoder,LINE_GET_PRECODER,GetPrecoderMsg,PrecoderCoefMsg)

  
typedef struct PrecoderBandPlanMsg PrecoderBandPlanMsg;
struct PrecoderBandPlanMsg
{
  BandsDescriptor             mimoBandPlanTx;
  BandsDescriptor             mimoBandPlanRx;
  VdslVectoredBandsDescriptor vectBandsDs; /* downstream vectored bandplan */
  int16                       tone0Offset; /* Offset of the tone 0 in the SLVS */
  uint16                      xtcNrOfLowDsTones;
  uint16                      xtcNrOfLowUsTones;
  uint16                      xtcNrOfHighDsTones;
  uint16                      xtcNrOfHighUsTones;
};

#define LINE_GET_PRECODER_BANDPLAN (0x59 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPrecoderBandPlan,LINE_GET_PRECODER_BANDPLAN,DUMMY,PrecoderBandPlanMsg)

typedef struct PrecoderGainsMsg PrecoderGainsMsg;
struct PrecoderGainsMsg
{
  uint32 gainIn;
  uint32 gainOut;
};

#define LINE_GET_PRECODER_GAINS (0x5A | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetPrecoderGains,LINE_GET_PRECODER_GAINS,uint8,PrecoderGainsMsg)

typedef struct SetPrecoderGainsMsg SetPrecoderGainsMsg;
struct SetPrecoderGainsMsg
{
  uint8  direction;
  uint8  reserved[3];
  uint32 gainIn;
  uint32 gainOut;
};

#define LINE_SET_PRECODER_GAINS (0x53 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetPrecoderGains,LINE_SET_PRECODER_GAINS,SetPrecoderGainsMsg,DUMMY);
  
#define TONES_FEQ_SEGMENT_SHORT (64)
#define TONES_FEQ_SEGMENT       (128)
typedef struct FeqSegmentShort FeqSegmentShort;
struct FeqSegmentShort
{
  int32 refPsd;
  int16 mant[2*TONES_FEQ_SEGMENT_SHORT];
  int8  exp[TONES_FEQ_SEGMENT_SHORT];
};
typedef struct FeqSegment FeqSegment;
struct FeqSegment
{
  int32 refPsd; /* Reference received PSD for a (2^13,2^13) value at the FFT output, i.e for a FEQ value of 1 */
  int16 mant[2*TONES_FEQ_SEGMENT];
  int8  exp[TONES_FEQ_SEGMENT];
};

typedef struct FeqMsg FeqMsg;
struct FeqMsg
{
  uint16 startTone;
  uint8  log2M;
  uint8  minSnr;
};

#define LINE_GET_FEQ    (0x5B | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetFeq,LINE_GET_FEQ,FeqMsg,FeqSegment)

#define TONES_FEQ_OUTPUT_SEGMENT 96

typedef struct FeqOutputSegment FeqOutputSegment;
struct FeqOutputSegment
{
  uint8 testStatus; /* See SignalMeasurement */
  uint8 reserved;
  int16 feqOutput[2*TONES_FEQ_OUTPUT_SEGMENT];
};

typedef struct FeqOutputControl FeqOutputControl;
struct FeqOutputControl
{
  uint8 partId;
  uint8 getMaxPower;
};
  
#define LINE_GET_FEQ_OUTPUT    (0x5C | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetFeqOutput,LINE_GET_FEQ_OUTPUT,FeqOutputControl,FeqOutputSegment)

#endif /* ATU_C     */

#endif /* VECTORING */

#if defined(ATM_SAR_LINES_SUPPORTED)
#define IWF_FORWARD_MISMATCHED_PROTOCOL (1<<0)
#define IWF_NO_UPSTREAM_CONFIG          (1<<1)
#define IWF_NO_DOWNSTREAM_CONFIG        (1<<2)
#define IWF_DISABLE_CRC_CHECK           (1<<3)
#define IWF_NO_US_VLAN_TAG_INSERTION    (1<<4)
#define IWF_NO_DS_VLAN_TAG_INSERTION    (1<<5)
#define IWF_USE_IPV6_ETHERTYPE          (1<<6)

typedef struct LegacyAtmInterfaceConfig LegacyAtmInterfaceConfig;
struct LegacyAtmInterfaceConfig
{
  uint8  isEnabled;
  uint8  channelIndex;
  uint8  interworkingFunction;
  uint8  flags;

  uint32 atmHeader;
  uint32 atmHeaderMask;

  uint16 dsChannelTCI;
  uint16 dsChannelMask;
  uint16 usChannelTCI;

  uint8  macDA[6];
  uint8  macSA[6];

  uint16 sessionId;
  uint16 maxAAL5SDUSize;

  uint16 reserved;
};

typedef struct GetSetAtmInterfaceConfig GetSetAtmInterfaceConfig;
struct GetSetAtmInterfaceConfig
{
  uint8 channelIndex;
  uint8 flags;

  uint8  deprecated1;
  uint8  us_interworkingFunction;
  uint32 us_atmHeader;
  uint32 us_atmHeaderMask;
  uint16 us_channelTCI;
  uint8  us_macDA[6];
  uint8  us_macSA[6];
  uint16 us_sessionId;
  uint16 us_maxAAL5SDUSize;

  uint8  deprecated2;
  uint8  ds_interworkingFunction;
  uint16 ds_channelTCI;
  uint16 ds_channelMask;
  uint32 ds_atmHeader;
};

#define LEGACY_LINE_SET_ATM_INTERFACE (0x60 | LINE_SERVICE_ID)
HPI_SERV_DEF(LegacyLineSetAtmInterface,
             LEGACY_LINE_SET_ATM_INTERFACE,
             LegacyAtmInterfaceConfig,
             DUMMY)

#define LINE_GET_ATM_INTERFACE (0x61 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetAtmInterface,
             LINE_GET_ATM_INTERFACE,
             uint8,
             GetSetAtmInterfaceConfig)

#define IWF_NR_PROTOCOLS 4
#define IWF_PROTO_DETECT_0000 0
#define IWF_PROTO_DETECT_C021 1
#define IWF_PROTO_DETECT_FEFE 2
#define IWF_PROTO_DETECT_AAAA 3

typedef struct IwfCounters IwfCounters;
struct IwfCounters
{
  uint32 ds_valid_frames;
  uint32 ds_bad_frames;
  uint32 ds_TPID_mismatches;
  uint32 ds_VID_lookup_failures;
  uint32 ds_not_ipv4;
  uint32 ds_wrong_ethertype;
  uint32 ds_injected_atm_cells;
  uint32 ds_injected_aal5_frames;
  uint32 ds_cellOverflow;

  uint32 us_filter_matches;
  uint32 us_filter_mismatches;
  uint32 us_snooped_atm_cells;
  uint32 us_aal5_rx_frames;
  uint32 us_aal5_reassembly_timeout;
  uint32 us_protocol_mismatch;
  uint32 us_snooped_aal5_frames;
  uint32 us_aal5_size_exceeded;
  uint32 us_aal5_abort;
  uint16 prefix_detect[IWF_NR_PROTOCOLS];
  uint32 reserved[8];
};

#define LINE_GET_IWF_COUNTERS (0x62 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetIwfCounters, LINE_GET_IWF_COUNTERS, uint8, IwfCounters)

typedef struct LineIwfInfo LineIwfInfo;
struct LineIwfInfo
{
  uint8  macDA[6];      /* MAC DA (6B) */
  uint8  macSA[6];      /* MAC SA (6B) */
  uint16 usChannelTCI;
  uint16 reserved1;
};

#define LINE_SET_IWF_INFO (0x63 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetIwfInfo, LINE_SET_IWF_INFO, LineIwfInfo, DUMMY)

#define LINE_GET_IWF_INFO (0x64 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetIwfInfo,
             LINE_GET_IWF_INFO,
             DUMMY,
             LineIwfInfo)

#define IWF_SETMSG_NOT_IN_IWF_MODE              1 
#define IWF_SETMSG_TRAFFIC_CONFIG_NOT_AVAILABLE 2 
#define IWF_SETMSG_INVALID_IWFFUNC              3 
#define IWF_SETMSG_INVALID_CHANNEL_INDEX        4 
#define IWF_SETMSG_SAR_TX_API_ERROR             5 
#define IWF_SETMSG_SAR_RX_API_ERROR             6

#define NEW_LINE_SET_ATM_INTERFACE (0x65 | LINE_SERVICE_ID)
HPI_SERV_DEF(NewLineSetAtmInterface,
             NEW_LINE_SET_ATM_INTERFACE,
             GetSetAtmInterfaceConfig,
             DUMMY)

#endif /* ATM_SAR_LINES_SUPPORTED */
#ifdef NOISE_GENERATOR
#define LINE_SET_PSD_VECTOR (0x35 | LINE_SERVICE_ID)
typedef struct PsdVector PsdVector;
struct PsdVector
{
  fix8_8 psd[2048];
  uint32 psdId;
  uint8 muted;
  uint8 dummy[3];
};

HPI_SERV_DEF(LineSetPsdVector,
             LINE_SET_PSD_VECTOR,
             PsdVector,
             DUMMY)

#define LINE_SET_NOISE_MUTED  (0x51 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetNoiseMuted,
             LINE_SET_NOISE_MUTED,
             uint8, /* bitmap, bit 0 for muting psdId 0; 1 for psdId 1 */
             DUMMY)

typedef struct RfiConfig RfiConfig;
struct RfiConfig {
  uint16                rfiToneList[NOISE_GEN_MAX_RFI];
  fix8_8                rfiMagnitudes[NOISE_GEN_MAX_RFI];         /* FIXME: allow for modulation later, we can use tssiLinearNoisegen for the cooked data */
  Complex16             rfiBase[NOISE_GEN_MAX_RFI];               /* magnitude 1 vectors with phase of S0 waveform */
  Complex16             rfiInitialOffsets[NOISE_GEN_MAX_RFI];
  Complex16             rfiMainPhaseOffset[NOISE_GEN_MAX_RFI];
  int32                 rfiDrifts[NOISE_GEN_MAX_RFI*2];           /* WARNING: complex separated format */
  uint8                 nrOfRfiTones;
  uint8                 psdId;
  uint8                 reserved[2];
};
#define LINE_SET_NOISE_RFICONFIG  ( 0x52 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetRfiConfig,
             LINE_SET_NOISE_RFICONFIG,
             RfiConfig, 
             DUMMY)


#endif /* NOISE_GENERATOR */

#define LINE_SET_TX_CTRL  (0x34 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetTxCtrl,
             LINE_SET_TX_CTRL,
             uint8,
             DUMMY)

/*****************************************************************************/

#define LINE_SET_G9991_MONITOR   (0x5E | LINE_SERVICE_ID)

HPI_SERV_DEF(LineSetG9991Monitor, LINE_SET_G9991_MONITOR , G9991Monitor, DUMMY)

/*****************************************************************************/
#define LINE_GET_G9991_COUNTERS  (0x5F | LINE_SERVICE_ID)

HPI_SERV_DEF(LineGetG9991Counters, LINE_GET_G9991_COUNTERS , DUMMY , G9991Counters)

#ifdef SUPPORT_HMI
#define LINE_CHANGE_MDS (0x7E | LINE_SERVICE_ID)
HPI_SERV_DEF(LineChangeMds, LINE_CHANGE_MDS, DtaRequest, DUMMY)

#define LINE_SET_DTA_CONFIG (0x7F | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetDtaConfig, LINE_SET_DTA_CONFIG, DtaConfig, DUMMY)

#define LINE_GET_DTA_CONFIG (0x80 | LINE_SERVICE_ID)
HPI_SERV_DEF(LineGetDtaConfig, LINE_GET_DTA_CONFIG, DUMMY, DtaConfig)

#define LINE_SET_LOGICAL_FRAME_CONFIG (0x7A | LINE_SERVICE_ID)
HPI_SERV_DEF(LineSetLogicalFrameConfig, LINE_SET_LOGICAL_FRAME_CONFIG,LogicalFrameCfg,LogicalFrameCfg)
#endif


#endif /* LINEMESSAGEDEF_H */

