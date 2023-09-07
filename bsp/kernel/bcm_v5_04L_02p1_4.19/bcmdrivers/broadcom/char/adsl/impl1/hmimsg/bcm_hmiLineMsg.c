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

#ifdef SUPPORT_HMI
#include "bcm_BasicTypes.h"
#include "bcm_hmiLineMsg.h"
#else
#include "bcm_userdef.h"
#include "bcm_hmiLineMsg.h"
#include "bcm_hmiMsgDef.h"
#endif

/* {{{ layout variable of principal types */

typeLayout ResetLineMsgReqLayout[] = {
  {  1, uint32Layout},
  {  0, NULL}
};

typeLayout BearerSpecLayout[] = {
  {  2, uint32Layout},
  { 12, uint8Layout},
  {  0, NULL}
};
typeLayout BearerTrafficConfigLayout[] = {
  {  2, BearerSpecLayout},
  {  4, uint8Layout},           /* txAtmFlags */
  {  1, uint32Layout},          /* minL2BitRate */
  {  1, uint16Layout},          /* L2lowRateSlots */
  {  6, uint8Layout},
  {  0, NULL}
};
typeLayout LineTrafficConfigLayout[] = {
  {  2, BearerTrafficConfigLayout},
  {  4, uint8Layout},           /* tpsTcTypeADSL */
  {  0, NULL}
};
typeLayout LinkConfigLayout[] = {
  {  3, uint16Layout},
  {  2, uint8Layout},
  {  5, uint16Layout},
  {  0, NULL}
};
typeLayout ReinitConfigLayout[] = {
  {  8, uint8Layout},
  {  1, uint32Layout},
  {  0, NULL}
};

#ifndef SUPPORT_HMI
extern typeLayout TxFilterDefLayout[];
typeLayout PhysicalLayerConfigLayout[] = {
  {  2,                                LinkConfigLayout},
  {  2*3,                              uint16Layout}, /* SpecificLinkConfig */
  {  1,                                uint16Layout}, /* maxRxPower */
  {  4,                                uint8Layout},
  {  BCM_NB_ADSL_TONE_US/8,            uint8Layout}, /* carrierMaskUp */
  {  BCM_NB_ADSL_TONE_DS/8,            uint8Layout}, /* carrierMaskDn */
  {  8,                                uint8Layout},
  {  2,                                uint8Layout}, /* customPsdTemplatePlusDn */
  {  BCM_CUSTOM_PSD_TEMPLATE_DN_LEN,   uint16Layout},
  {  2,                                uint8Layout}, /* customPsdTemplateAnnexMup */
  {  BCM_CUSTOM_PSD_TEMPLATE_UP_LEN,   uint16Layout},
  {  2,                                uint16Layout}, /* L0time */
  {  BCM_NB_GHS_CS+5,                  uint8Layout},  /* L2Atpr*/
  {  3,                                uint8Layout},  /* reservedB */
  {  4,                                uint8Layout},  /* prmConfig */
  {  1,                                uint32Layout},
  {  BCM_VNS_UP_LEN,                   uint16Layout}, /* virtNoiseShape */
  {  BCM_VNS_UP_LEN,                   uint8Layout},
  {  BCM_VNS_DN_LEN,                   uint16Layout},
  {  BCM_VNS_DN_LEN,                   uint8Layout},
  {  4,                                uint8Layout},
  {  1,                                TxFilterDefLayout}, /* txFilter */
  {  1,                                uint16Layout}, /* customTxFilterMaxTxPsd */
  {  2,                                ReinitConfigLayout},
  { 12,                                uint8Layout}, /* monitoringTones */
  {  0,                                NULL}
};
typeLayout PhysicalLayerConfigVDSLLayout[] = {
  {  2,                         uint8Layout},  /* limitingMaskDn */
  {  1,                         uint16Layout}, /* dpbo_muf */
  {  2*BCM_VDSL_NB_PSD_BP_DN,   uint16Layout},
  {  2,                         uint8Layout}, /* limitingMaskUp */
  {  1,                         uint16Layout},
  {  2*BCM_VDSL_NB_PSD_BP_UP,   uint16Layout},
  {  2,                         uint8Layout}, /* bandPlanDn */
  {  2*BCM_VDSL_NB_TONE_GROUPS, uint16Layout},
  {  2,                         uint8Layout}, /* bandPlanUp */
  {  2*BCM_VDSL_NB_TONE_GROUPS, uint16Layout},
  {  2,                         uint8Layout}, /* rfiNotches */
  {  2*BCM_VDSL_NB_RFI_NOTCHES, uint16Layout},
  {  2,                         uint8Layout}, /* pboPsdUp */
  {  3*BCM_VDSL_NB_TONE_GROUPS, uint16Layout},
  {  1,                         uint16Layout}, /* enableProfileBitmap */
  {  2,                         uint8Layout},  /* reservedA */
  {  4,                         uint16Layout}, /* forceElectricalLength */
  {  2,                         uint8Layout},  /* vectorFlag */
  {  1,                         uint16Layout}, /* dleThreshold */
  {  4,                         uint8Layout},  /* upboElmt */
  {  1,                         uint16Layout}, /* dsTargetMarginSplitTone */
  {  10,                        uint8Layout},  /* dsTargetMarginBitmap */
  {  0,                         NULL}
};
#endif	/* SUPPORT_HMI */

typeLayout PsdDescriptorGfastLayout[] = {
  {  4,                         uint8Layout},
  {  2*BCM_GFAST_NB_PSD_BP,     uint16Layout},
  {  0,                         NULL}
};
typeLayout TrafficConfigGfastLayout[] = {
  {   4,   uint32Layout},
  {   2,   uint16Layout},
  {  12,   uint8Layout},
  {   0,   NULL}
};
typeLayout RmcConfigLayout[] = {
  {   2,   uint16Layout},
  {   2,   uint8Layout},
  {   0,   NULL}
};
typeLayout FraConfigLayout[] = {
  {   2,   uint8Layout},
  {   1,   uint16Layout},
  {   2,   uint8Layout},
  {   0,   NULL}
};

typeLayout GfastConfigLayout[] = {
  {   2, PsdDescriptorGfastLayout},
  {   4, uint16Layout},         /* bandPlanDn/Up */
  {   2, uint8Layout},          /* pboPsdUp */
  {   3, uint16Layout},         /* pbo */
  {   2, uint8Layout},          /* enableProfileBitmap */
  {   1, uint16Layout},         /* forceElectricalLength */
  {   2, uint8Layout},          /* retrainAllowed */
  {   3, uint16Layout},         /* maxRxPowerUp */
  {   6, uint8Layout},          /* Sds */
  {   2, LinkConfigLayout},
  {   6, uint8Layout},          /* reservedB[4] + rmcrLorTrigger[2] */
  {   2, ReinitConfigLayout},
  {   2, TrafficConfigGfastLayout},
  {   2, RmcConfigLayout},
  {   2, FraConfigLayout},
  {   0, NULL}
};
typeLayout CarMaskGfastLayout[] = {
  {  2,                       uint8Layout},
  {  2*BCM_GFAST_NB_CAR_MASK, uint16Layout},
  {  0,                       NULL}
};
typeLayout CarMaskRmcLayout[] = {
  {  2,                       uint8Layout},
  {  2*BCM_NB_CAR_MASK_RMC, uint16Layout},
  {  0,                       NULL}
};
typeLayout GfastRFIconfigLayout[] = {
  {  2,                          uint8Layout},
  {  2*BCM_GFAST_NB_RFI_BANDS+1, uint16Layout},
  {  2,                          CarMaskGfastLayout},
  {  2,                          CarMaskRmcLayout},
  {  0,                          NULL}
};

typeLayout DtaRequestLayout[] =
{
  {4, uint8Layout},
  {0,NULL}
};

typeLayout DtaConfigLayout[] =
{
  {6, uint8Layout},
  {2, uint16Layout},
  {14, uint8Layout},
  {5, uint32Layout},
  {16, uint8Layout},
  {0,NULL}
};

typeLayout LineIntroduceErrorsMsgReqLayout[] =
{
 {   7,   uint16Layout},
 {   0,   NULL}
};

typeLayout SetLineLogicalFrameConfigMsgReqLayout[] = {
  { 6, uint8Layout},
  { 1, uint16Layout},
  { 4, uint8Layout},
  { 0, NULL},
};

#ifndef SUPPORT_HMI
typeLayout TestConfigMessageLayout[] = {
  {   2,   uint32Layout},       /* disableProtocolBitmap */
  {  14,   uint8Layout},        /* disableCoding */
  {   3,   uint16Layout},       /* disableLargeD */
  {  38,   uint8Layout},        /* L2testConfig */
  {   1,   uint16Layout},       /* vectorBlackList */
  {  12,   uint8Layout},
  {   0,   NULL}
};
typeLayout GinpParametersLayout[] = {
  {  3,   uint32Layout},
  { 12,   uint8Layout},
  {  0,   NULL}
};
typeLayout SosParametersLayout[] = {
  {  1,   uint32Layout},
  {  2,   uint16Layout},
  {  4,   uint8Layout},
  {  0,   NULL}
};
typeLayout RocParametersLayout[] = {
  {  1,   uint16Layout},
  {  2,   uint8Layout},
  {  0,   NULL}
};
typeLayout LineExtraConfigLayout[] = {
  {  2,   GinpParametersLayout},
  {  2,   SosParametersLayout},
  {  2,   RocParametersLayout},
  {  4,   uint8Layout},
  {  0,   NULL}
};
  
typeLayout DerivedSecCountersLayout[] = {
  {  9,   uint32Layout},
  {  0,   NULL}
};
typeLayout AdslAnomCountersLayout[] = {
  { 12,   uint32Layout},
  {  0,   NULL}
};
typeLayout HmiPerfCountersLayout[] = {
  { 10,   uint32Layout},
  {  0,   NULL}
};
typeLayout GinpCountersLayout[] = {
  {  4,   uint32Layout},
  {  0,   NULL}
};
typeLayout GfastCountersLayout[] = {
  {  5,   uint32Layout},
  {  0,   NULL}
};

typeLayout LineCountersLayout[] = {
  {  2,   DerivedSecCountersLayout},
  {  2,   AdslAnomCountersLayout},
  {  2,   HmiPerfCountersLayout},
  {  1,   uint16Layout},        /* failedFullInitCount */
  {  2,   uint8Layout},
  {  3,   uint16Layout},        /* fastInitCounter */
  {  2,   uint8Layout},
  {  4,   uint32Layout},        /* upTime + reserved */
  {  2,   GinpCountersLayout},
  {  2,   GfastCountersLayout},
  {  0,   NULL}
};

typeLayout GetLineStatusMsgReqLayout[] = {
  {  1,   uint32Layout},
  {  0,   NULL}
};
typeLayout BearerLatencyInfoLayout[] = {
  {  2,   uint8Layout},         /* phyLatencyIndex... INPreport */
  {  1,   uint16Layout},        /* reserved */
  {  3,   uint32Layout},        /* actualBitRate ... L */
  {  2,   uint16Layout},        /* D ... B */
  {  6,   uint8Layout},         /* N ... G */
  {  1,   uint16Layout},        /* U */
  {  6,   uint8Layout},         /* Q ... phyRrrcBits */
  {  1,   uint16Layout},        /* rtxRxQueue */
  {  2,   uint8Layout},         /* reservedB */
  {  1,   uint16Layout},        /* INP */
  {  4,   uint8Layout},         /* tpsTcOptions ... codingType */
  {  5,   uint32Layout},        /* maxMemory ... Ldr */
  {  4,   uint8Layout},         /* dtaMax ... reservedD */
  {  1,   uint32Layout},        /* idr */
  {  0,   NULL}
};
typeLayout LinkStatusLayout[] = {
  {  4,   uint16Layout},        /* snrMargin... electricalLength */
  {  5,   uint32Layout},        /* actualLineBitRate ... aggAchNDR */
  { 10,   uint8Layout},         /* attNDRmethod ... riPolicy */
  {  1,   uint16Layout},        /* loopDelay */
  {  2,   uint32Layout},        /* currentFailures ... changedFailures */
  {  2,   uint16Layout},        /* snrMarginROC ... rateLossROC */
  {  2,   uint32Layout},        /* attainableGDR ... andEftr */
  {  2,   uint16Layout},        /* autoSensingMetric ... todNs */
  {  8,   uint8Layout},         /* reserved1 ... tBudget */
  {  2,   BearerLatencyInfoLayout},
  {  0,   NULL}
};
typeLayout LineStatusLayout[] = {
  {   8,   uint8Layout},        /* state ... farendDetected */
  {   1,   uint16Layout},       /* actualElectricalLength */
  {  14,   uint8Layout},        /* tpsTcType ... reserved */
  {  50,   uint16Layout},       /* perBandStatus */
  {  11,   uint32Layout},       /* lastRetrainInfo */
  {  40,   uint8Layout},
  {   4,   uint8Layout},
  {   4,   uint8Layout},        /* gfastProfile ... reservedB */
  {   2,   LinkStatusLayout},
  {   0,   NULL}
};

typeLayout LineFeaturesLayout[] = {
  { 3*2+4*4, uint32Layout},
  { 0,       NULL}
};
    
typeLayout EocRegisterIdLayout[] = {
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout EocReadResultLayout[] =
{
  { 34, uint8Layout},
  { 0 , NULL}
};

typeLayout EocRegisterLayout[] =
{
  { 34, uint8Layout},
  { 0,  NULL}
};

typeLayout EocWriteResultLayout[] =
{
  { 2, uint8Layout},
  { 0, NULL}
};

typeLayout TransparentEocMessageLayout[] =
{
  { 1,                                        uint16Layout},
  { 2+BCM_TRANSPARENT_EOC_MAX_SEGMENT_LENGTH, uint8Layout},
  { 0,                                        NULL}
};


typeLayout GetHistogramMsgRepLayout [] =
{
  { LINETEST_HIST_LEN+2,           uint32Layout},
  { 0,                             NULL}
};

typeLayout GetBandPlanMsgRepLayout [] =
{
  { 2,                             uint8Layout},
  { BCM_VDSL_NB_TONE_GROUPS*2,     uint16Layout},
  { 2,                             uint8Layout},
  { BCM_VDSL_NB_TONE_GROUPS*2+4*2, uint16Layout},
  { 0,                             NULL}
};

typeLayout TestModeSelectionLayout[] =
{
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout LoopCharacteristicsLayout[] =
{
  { 1, uint32Layout},
  { 0, NULL}
};

typeLayout StartLoopCharacterisationMsgReqLayout[] =
{
  { 2+2,                              uint8Layout},
  { BCM_CUSTOM_PSD_TEMPLATE_DN_LEN,   uint16Layout}, /* tssiBreakPoint */
  { 1,                                uint16Layout},
  { 4+64,                             uint8Layout},
  { 0,                                NULL}
};

typeLayout SignalGenerationLayout[] = {
  {  2,                                            uint8Layout},
  {  1,                                            uint16Layout}, /* signalPsd */
  {  2,                                            uint32Layout},
  { BCM_NB_ADSL_TONE_DS/8+5+BCM_TX_FILTER_LEN+2,   uint8Layout},
  { BCM_CUSTOM_PSD_TEMPLATE_DN_LEN,                uint16Layout}, /* tssiBreakPoint */
  {  6,                                            uint8Layout},
  { BCM_VDSL_NB_TONE_GROUPS*2,                     uint16Layout},
  {  2,                                            uint8Layout},
  { BCM_VDSL_NB_TONE_GROUPS*2,                     uint16Layout},
  {  4,                                            uint8Layout},
  { BCM_VDSL_NB_PSD_BP_DN*2,                       uint16Layout},
  {  2,                                            uint8Layout},
  { 2*BCM_VDSL_NB_RFI_NOTCHES+1,                   uint16Layout},
  {  4,                                            uint8Layout},
  {  1,                                            uint16Layout},
  {  2,                                            uint8Layout},
  {  0,                                            NULL}
};

typeLayout GetSignalMeasurementMsgReqLayout[] =
{
  { 2,   uint8Layout},
  { 0,   NULL}
};

typeLayout SignalMeasurementLayout[] =
{
  { 2,             uint32Layout},
  { 3*BCM_PSD_LEN, uint16Layout},
  { 4,             uint8Layout},
  { 0,             NULL}
};

typeLayout ManufTestConfigLayout[] =
{
  { 2, uint8Layout},
  { 1, uint16Layout},
  { 4, uint8Layout},
  { 0, NULL}
};

typeLayout ManufacturingTestResultLayout[] =
{
  { 3*BCM_MANUF_BANDS+7, uint32Layout},
  { 4,                   uint16Layout},
  { 0,                   NULL}
};

typeLayout AGCsettingLayout[] =
{
  { 1, uint16Layout},
  { 0, NULL}
};

typeLayout ToneDescriptorLayout[] =
{
  { 2, uint8Layout},
  { 16, uint16Layout},
  { 0, NULL}
};

typeLayout LongPartIdLayout[] =
{
  { 1, uint32Layout},
  { 0, NULL}
};

typeLayout AccurateEchoLayout[] =
{
  { BCM_ECHO_PART_SIZE, uint32Layout},
  { 0,                  NULL}
};

typeLayout GetVDSLechoMsgRepLayout[] =
{
  { 3,                    uint16Layout},
  { 2,                    uint8Layout},
  { 2*BCM_ECHO_PART_SIZE, uint16Layout},
  { BCM_ECHO_PART_SIZE,   uint8Layout},
  { 0,                    NULL}
};

typeLayout SeltResultLayout[] =
{
  { 64, uint32Layout},
  { 0,  NULL}
};


typeLayout AccurateEchoVarianceLayout[] =
{
  { BCM_ECHOVAR_PART_SIZE, uint16Layout},
  { 0,                     NULL}
};

typeLayout LineBitAllocAndSnrAndGiReqLayout[] =
{
  { 2,  uint8Layout},
  { 2,  uint16Layout},
  { 8,  uint8Layout},
  { 0,  NULL}
};

typeLayout GetLineBitAllocationRepLayout[] =
{
  { BCM_GET_BI_REP_SIZE, uint8Layout},
  { 0,                   NULL}
};

typeLayout GetLineRmcBitAllocationRepLayout[] =
{
  { 1                          , uint32Layout},
  { BCM_RMC_BIT_ALLOC_PART_SIZE, uint8Layout},
  { BCM_RMC_BIT_ALLOC_PART_SIZE, uint16Layout},
  { 0,                           NULL}
};

typeLayout GetLineSnrMsgRepLayout[] =
{
  { BCM_GET_SNR_REP_SIZE, uint16Layout},
  { 0,                    NULL}
};

typeLayout PeriodIdentificationLayout[] =
{
  { 1, uint32Layout},
  { 0, NULL}
};

typeLayout HmiPeriodCountersLayout[] = {
  {  2,   DerivedSecCountersLayout},
  {  2,   AdslAnomCountersLayout},
  {  2,   HmiPerfCountersLayout},
  {  1,   uint16Layout}, /* failedFullInitCount */
  {  2,   uint8Layout}, /* fullInitCount + reInitCount */
  {  3,   uint16Layout}, /* fastInitCounter ... periodDuration */
  {  2,   uint8Layout}, /* suspectFlagXX */
  {  4,   uint32Layout}, /* upTime ... elapsedTimeIn24hour */
  {  2,   GinpCountersLayout},
  {  2,   GfastCountersLayout},
  {  0,   NULL}
};

typeLayout NewThresholdsLayout[] =
{
  { 2*8, uint32Layout},
  { 0,   NULL}
};

typeLayout FEvendorInfoLayout[] =
{
  { 8,  uint8Layout},
  { 1,  uint16Layout},
  { 2,  uint8Layout},
  { 2,  uint32Layout},
  { 4,  uint8Layout},
  { 2,  uint32Layout},
  { 8,  uint8Layout},
  { 0,  NULL}
};

typeLayout OverheadMessageLayout[] =
{
  { 1,                uint16Layout},
  { BCM_OVH_BUF_SIZE, uint8Layout},
  { 0,                NULL}
};

typeLayout OneBytePartIdLayout[] =
{
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout GetLineLoopDiagResultsMsgRepLayout[] = 
{
 {   1,   uint32Layout}, /* attndr */
 {   4,   uint16Layout},
 {   1,   uint32Layout}, /* attndr */
 {   4,   uint16Layout},
 {   4,   uint8Layout},
 {   0,   NULL}
};

typeLayout GetLineLoopDiagHlinMsgReqLayout[] =
{
  { 2, uint8Layout},
  { 0, NULL}
};

typeLayout LineHlinLayout[] =
{
  { 1+64*2, uint16Layout},
  { 0,      NULL}
};

typeLayout LineHlogLayout[] =
{
  { 64+128, uint16Layout},
  { 0,      NULL}
};

typeLayout LineQlnLayout[] =
{
  { 64+256, uint8Layout},
  { 0,      NULL}
};

typeLayout LoopDiagSnrLayout[] =
{
  { 64+256, uint8Layout},
  { 0,      NULL}
};

typeLayout UpdateTestParametersMsgReqLayout[] =
{
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout UpdateTestParametersMsgRepLayout[] =
{
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout LineHlogNELayout[] =
{
  { 64, uint16Layout},
  { 0,  NULL}
};

typeLayout LineQlnNELayout[] =
{
  { 64, uint8Layout},
  { 0,  NULL}
};

typeLayout GetLineCarrierGainsMsgReqLayout[] =
{
  { 2,  uint8Layout},
  { 2,  uint16Layout},
  { 8,  uint8Layout},
  { 0,  NULL}
};

typeLayout LineCarrierGainsLayout[] =
{
  { BCM_GET_GI_REP_SIZE, uint16Layout},
  { 0,                   NULL}
};

typeLayout LineLinearTssiLayout[] =
{
  { BCM_TSSI_US_SIZE+BCM_TSSI_DS_SIZE, uint16Layout},
  { 0,                                 NULL}
};

typeLayout LineL2PowerTrimMsgReqLayout[] =
{
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout GetLineOamVectorMsgReqLayout[] = 
{
  { 5, uint8Layout},
  { 0, NULL}
};

typeLayout GetLinePtmCountersMsgRepLayout[] = 
{
  { 24, uint32Layout},
  { 0,  NULL}
};

typeLayout GetLineG9991CountersMsgRepLayout[] = 
{
  { 2,  uint8Layout},
  { 1,  uint16Layout},
  { 14, uint32Layout},
  { 0,  NULL}
};

typeLayout SetLineG9991MonitorMsgReqLayout[] = 
{
  { 2, uint8Layout},
  { 1, uint16Layout},
  { 0, NULL}
};

typeLayout GetLineOamVectorMsgRepLayout[] = {
  { BCM_GET_OAM_REP_SIZE, uint8Layout},
  { 0,                    NULL}
};

typeLayout GetLineMrefPsdMsgRepLayout[] = {
  { 2,                        uint8Layout},
  { 2*BCM_MAX_DS_MREF_PSD_BP, uint16Layout},
  { 2,                        uint8Layout},
  { 2*BCM_MAX_US_MREF_PSD_BP, uint16Layout},
  { 0,                        NULL}
};

typeLayout NtrPhaseCompLayout[] = 
{
  {  1, uint16Layout},
  { 18, uint8Layout},
  {  0, NULL}
};

typeLayout GetLineNeInmCountersMsgRepLayout[] = {
  { BCM_INM_INPEQ_LEN+BCM_INM_IAT_LEN+2, uint32Layout},
  { 2,                                   uint8Layout},
  { 1,                                   uint16Layout},
  { 8,                                   uint8Layout},
  { 0,                                   NULL},
};

typeLayout SetLineNeInmConfigMsgReqLayout[] = {
  { 2, uint8Layout},
  { 1, uint16Layout},
  { 8, uint8Layout},
  { 0, NULL},
};

typeLayout SetLineLogicalFrameConfigMsgReqLayout[] = {
  { 6, uint8Layout},
  { 1, uint16Layout},
  { 4, uint8Layout},
  { 0, NULL},
};

typeLayout SetLineSeltCompatibleModeMsgReqLayout[] =
{
  { 1, uint8Layout},
  { 0, NULL}
};

#ifndef BCM_CPE
typeLayout SetPtmBondingConfigMsgReqLayout[] = {
  { BCM_NB_GHS_TRANSACTIONS+BCM_DISCOVERY_REG_LEN+2, uint8Layout},
  { 0,                                               NULL},
};

typeLayout GetPtmBondingConfigMsgRepLayout[] = {
  { BCM_NB_GHS_TRANSACTIONS+BCM_DISCOVERY_REG_LEN+2, uint8Layout},
  { 0,  NULL},
};

typeLayout GetPtmBondingRegistersMsgRepLayout[] = {
  { 2*BCM_DISCOVERY_REG_LEN+4, uint8Layout},
  { 2,  uint32Layout},
  { 0,  NULL},
};
#endif

typeLayout ChannelIdLayout[] = {
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout AtmInterfaceConfigLayout[] = {
  { 4,  uint8Layout},
  { 2,  uint32Layout},
  { 1,  uint16Layout},
  { 12, uint8Layout},
  { 2,  uint16Layout},
  { 2,  uint8Layout},
  { 2,  uint16Layout},
  { 1,  uint32Layout},
  { 0,  NULL},
};

#ifdef BCM_CPE
typeLayout GetVectorErrorSamplesMsgReqLayout[] =
{
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout GetVectorErrorSamplesMsgRepLayout[] =
{
  { 3,                            uint16Layout},
  { 1,                            uint8Layout},
  { BCM_NR_BYTES_IN_VECT_SEGMENT, uint8Layout},
  { 0,                            NULL}
};
#endif

typeLayout GetPrecoderGainsMsgReqLayout[] = {
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout GetPrecoderGainsMsgRepLayout[] = {
  { 2, uint32Layout},
  { 0, NULL}
};

typeLayout LineGetFeqMsgReqLayout[] = {
  { 1, uint16Layout},
  { 2, uint8Layout},
  { 0, NULL}
};

typeLayout LineGetFeqMsgRepLayout[] = {
  {1,                   uint32Layout},
  {2*TONES_FEQ_SEGMENT, uint16Layout},
  {TONES_FEQ_SEGMENT,   uint8Layout},
  {0,                   NULL}
};

typeLayout LineSwitchStateSilentMsgReqLayout[] = {
  {1, uint32Layout},
  {0, NULL}
};

typeLayout LineInformVectConvComplMsgReqLayout [] ={
  {1, uint8Layout},
  {0, NULL}    
};

typeLayout GetLineIwfCountersMsgReqLayout[] = {
  { 1, uint8Layout},
  { 0, NULL}
};

typeLayout GetLineIwfCountersMsgRepLayout[] = {
  { 18,               uint32Layout},
  { IWF_NR_PROTOCOLS, uint16Layout},
  { 8,                uint32Layout},
  { 0,                NULL}
};

typeLayout LineIwfInfoLayout[] = {
  { 12, uint8Layout},
  { 2,  uint16Layout},
  { 0, NULL}
};

typeLayout LineSetPilotSequenceMsgReqLayout[] = {
  { 4+(NR_OF_PILOT_SEQUENCE_ELEMENTS_FDPS/8), uint8Layout},
  { 2, uint16Layout},
  { 4, uint8Layout},
  { 0, NULL}
};

typeLayout LineChangePilotSequenceMsgReqLayout[] = {
  { (NR_OF_PILOT_SEQUENCE_ELEMENTS_FDPS/8), uint8Layout},
  { 4, uint8Layout},
  { 0, NULL}
};  

typeLayout LineGetVectoringCountersMsgRepLayout[] = {
  { 3, uint32Layout},
  { 2, uint16Layout},
  { 1, uint32Layout},
  { 4, uint8Layout},
  { 2, uint32Layout},
  { 0, NULL}
};

typeLayout LineStartVectorDumpMsgReqLayout[] = {
  { 2, uint8Layout},
  { 1, uint8Layout}, 
  { 3, uint8Layout},
  { 1, uint16Layout},
  { 8, uint8Layout},
  { 0, NULL}
};

typeLayout LineStartVectorDumpMsgRepLayout[] = {
  { 1, uint16Layout},
  { 2, uint8Layout},
  { 2, uint16Layout},
  { 4, uint8Layout},
  { 0, NULL}
};

typeLayout LineGetTxGainsMsgReqLayout[] = {
  { 1, uint16Layout},
  { 2, uint8Layout},
  { 0, NULL}
};

typeLayout LineGetTxGainsMsgRepLayout[] = {
  { 1, uint16Layout},
  { 1, uint16Layout},
  { NR_OF_TXGAIN_ELEMENTS, uint16Layout},
  { 0, NULL}
};

typeLayout LineSetPrecoderMsgReqLayout[] = {
  { 2, uint8Layout},
  { 1, uint16Layout},
  { 1, uint16Layout},
  { 2, uint8Layout},
  { MAX_PRECODER_MSG_SIZE, uint16Layout},
  { 0, NULL}
};

typeLayout LineGetPrecoderMsgReqLayout[] = {
  { 2, uint16Layout},
  { 4, uint8Layout},
  { 0, NULL}
};

typeLayout LineGetPrecoderMsgRepLayout[] = {
  { MAX_PRECODER_MSG_SIZE, uint16Layout},
  { 0, NULL}
};

typeLayout LineRuntimeCommandMsgReqLayout[] = {
  { 4+RUNTIME_CMD_DATA_LEN, uint8Layout},
  { 0, NULL}
};

typeLayout LineGetMimoBandPlanMsgRepLayout [] =
{
  { 2,                                                  uint8Layout},
  { BCM_VDSL_NB_TONE_GROUPS*2,                          uint16Layout},
  { 2,                                                  uint8Layout},
  { BCM_VDSL_NB_TONE_GROUPS*2,                          uint16Layout},
  { 2,                                                  uint8Layout},
  { VDSL_MSG_MAX_NO_OF_TONE_GROUPS_IN_BANDS_DESC*2,     uint16Layout},
  { 5,                                                  uint16Layout},
  { 0,                                                  NULL}
};

typeLayout LineSetVectorCompletionFlagMsgReqLayout[] = {
  { 4, uint8Layout},
  { 2, uint8Layout},
  { 1, uint16Layout},
  { 1, uint16Layout},
#ifdef BCM_VECTORING_FAST_SUPPORT
  { 2, uint8Layout},
  { 2, uint16Layout},
#endif
  { 0, NULL}
};

typeLayout LineSetVectorCompletionFlagMsgRepLayout[] = {
  { 1, uint16Layout},
  { 0, NULL}
};

typeLayout AfeDebugCommandMsgReqLayout[] =
{
  { 2, uint8Layout},
  { 2, uint16Layout},
  { 0, NULL}
};

typeLayout AfeDebugCommandMsgRepLayout[] =
{
  { 2, uint8Layout},
  { 2, uint16Layout},
  { 0, NULL}
};

typeLayout LineSetMimoGainsMsgReqLayout[] =
{
  {4, uint8Layout},
  {2, uint32Layout},
  {0,NULL}
};

typeLayout LineGetFeqOutputMsgReqLayout[] =
{
  {2,uint8Layout},
  {0,NULL} 
};

typeLayout LineGetFeqOutputMsgRepLayout[] =
{
  {2,uint8Layout},
  {2*TONES_FEQ_OUTPUT_SEGMENT,uint16Layout},
  {0,NULL}
};

typeLayout FeRawOamCountersLayout[] =
{
  {5, uint16Layout},
  {2, uint8Layout},
  {0,NULL}
};

typeLayout DtaRequestLayout[] =
{
  {4, uint8Layout},
  {0,NULL}
};

typeLayout DtaConfigLayout[] =
{
  {6, uint8Layout},
  {2, uint16Layout},
  {14, uint8Layout},
  {5, uint32Layout},
  {16, uint8Layout},
  {0,NULL}
};

/* }}} */
/* {{{ message layout using the macros in hmiMsgDef.h */

HMI_MSG_REQ_LAYOUT(ResetLine, RESET_LINE, ResetLineMsgReq)

HMI_MSG_REQ_LAYOUT(SetLineTrafficConfiguration,
                   SET_LINE_TRAFFIC_CONFIGURATION,
                   LineTrafficConfig)

HMI_MSG_REP_LAYOUT(GetLineTrafficConfiguration,
                   GET_LINE_TRAFFIC_CONFIGURATION,
                   LineTrafficConfig)

HMI_MSG_REQ_LAYOUT(SetLinePhysicalLayerConfiguration,
                   SET_LINE_PHYSICAL_LAYER_CONFIGURATION,
                   PhysicalLayerConfig)

HMI_MSG_REP_LAYOUT(GetLinePhysicalLayerConfiguration,
                   GET_LINE_PHYSICAL_LAYER_CONFIGURATION,
                   PhysicalLayerConfig)

HMI_MSG_REQ_LAYOUT(SetLinePhyConfigVDSL, SET_LINE_PHY_CONFIG_VDSL,
                   PhysicalLayerConfigVDSL)

HMI_MSG_REP_LAYOUT(GetLinePhyConfigVDSL, GET_LINE_PHY_CONFIG_VDSL,
                   PhysicalLayerConfigVDSL)

HMI_MSG_REQ_LAYOUT(SetLineGfastConfig, SET_LINE_GFAST_CONFIG,
                   GfastConfig)

HMI_MSG_REP_LAYOUT(GetLineGfastConfig, GET_LINE_GFAST_CONFIG,
                   GfastConfig)

HMI_MSG_REQ_LAYOUT(SetLineGfastRFIconfig, SET_LINE_GFAST_RFI_CONFIG,
                   GfastRFIconfig)

HMI_MSG_REP_LAYOUT(GetLineGfastRFIconfig, GET_LINE_GFAST_RFI_CONFIG,
                   GfastRFIconfig)

HMI_MSG_REQ_LAYOUT(SetLineTestConfiguration,
                   SET_LINE_TEST_CONFIGURATION,
                   TestConfigMessage)

HMI_MSG_REP_LAYOUT(GetLineTestConfiguration,
                   GET_LINE_TEST_CONFIGURATION,
                   TestConfigMessage)

HMI_MSG_REQ_LAYOUT(SetLineExtraConfig,
                   SET_LINE_EXTRA_CONFIG,
                   LineExtraConfig)

HMI_MSG_REP_LAYOUT(GetLineExtraConfig,
                   GET_LINE_EXTRA_CONFIG,
                   LineExtraConfig)

HMI_MSG_NPL_LAYOUT(StartLine, START_LINE)

HMI_MSG_NPL_LAYOUT(StopLine, STOP_LINE)

HMI_MSG_REP_LAYOUT(GetLineCounters, GET_LINE_COUNTERS,
                   LineCounters)

HMI_MSG_ALL_LAYOUT(GetLineStatus, GET_LINE_STATUS,
                   GetLineStatusMsgReq, LineStatus)

HMI_MSG_REP_LAYOUT(GetLineFeatures, GET_LINE_FEATURES,
                   LineFeatures)

HMI_MSG_REQ_LAYOUT(ReadEocRegister, READ_EOC_REGISTER,
                   EocRegisterId)

HMI_MSG_REP_LAYOUT(GetEocReadResult, GET_EOC_READ_RESULT,
                   EocReadResult)

HMI_MSG_REQ_LAYOUT(WriteEocRegister, WRITE_EOC_REGISTER,
                   EocRegister)

HMI_MSG_REP_LAYOUT(GetEocWriteResult, GET_EOC_WRITE_RESULT,
                   EocWriteResult)

HMI_MSG_REQ_LAYOUT(SendTransparentEocMessage, SEND_TRANSPARENT_EOC_MESSAGE, TransparentEocMessage)

HMI_MSG_REP_LAYOUT(GetTransparentEocMessage, GET_TRANSPARENT_EOC_MESSAGE, TransparentEocMessage)

HMI_MSG_REP_LAYOUT(GetHistogram, GET_HISTOGRAM, GetHistogramMsgRep)

HMI_MSG_REP_LAYOUT(GetBandPlan, GET_BAND_PLAN_VDSL, GetBandPlanMsgRep)

HMI_MSG_REQ_LAYOUT(EnterLineTestMode, ENTER_LINE_TEST_MODE, TestModeSelection)

HMI_MSG_NPL_LAYOUT(ExitLineTestMode, EXIT_LINE_TEST_MODE)

HMI_MSG_REQ_LAYOUT(StartLoopCharacterisation, START_LOOP_CHARACTERISATION,
                   StartLoopCharacterisationMsgReq)

HMI_MSG_REP_LAYOUT(GetLoopCharacteristics, GET_LOOP_CHARACTERISATION ,
                   LoopCharacteristics)

HMI_MSG_REQ_LAYOUT(StartSignalGenerationAndMeasurement,
                   START_SIGNAL_GENERATION_AND_MEASUREMENT,
                   SignalGeneration)

HMI_MSG_ALL_LAYOUT(GetSignalMeasurement, GET_SIGNAL_MEASUREMENT,
                   GetSignalMeasurementMsgReq, SignalMeasurement)

HMI_MSG_REQ_LAYOUT(StartManufacturingTest, START_MANUFACTURING_TEST,
                   ManufTestConfig)

HMI_MSG_REP_LAYOUT(GetManufacturingTestResult, GET_MANUFACTURING_TEST_RESULT,
                   ManufacturingTestResult)

HMI_MSG_REP_LAYOUT(GetLineAGCsetting, GET_LINE_AGC_SETTING,
                   AGCsetting)

HMI_MSG_REP_LAYOUT(GetLineFePilots, GET_LINE_FE_PILOTS,
                   ToneDescriptor)

HMI_MSG_ALL_LAYOUT(GetEcho, GET_ECHO,
                   LongPartId, AccurateEcho)

HMI_MSG_ALL_LAYOUT(GetVDSLecho, GET_VDSL_ECHO,
                   LongPartId, GetVDSLechoMsgRep)

HMI_MSG_ALL_LAYOUT(GetSelt, GET_SELT,
                   LongPartId, SeltResult)

HMI_MSG_ALL_LAYOUT(GetEchoVariance, GET_ECHO_VARIANCE,
                   LongPartId, AccurateEchoVariance)

HMI_MSG_ALL_LAYOUT(GetLineBitAllocation, GET_LINE_BIT_ALLOCATION,
                   LineBitAllocAndSnrAndGiReq , GetLineBitAllocationRep)

HMI_MSG_ALL_LAYOUT(GetLineRmcBitAllocation, GET_LINE_RMC_BIT_ALLOCATION,
                   LineBitAllocAndSnrAndGiReq, GetLineRmcBitAllocationRep)

HMI_MSG_ALL_LAYOUT(GetLineSnr, GET_LINE_SNR,
                   LineBitAllocAndSnrAndGiReq , GetLineSnrMsgRep)

HMI_MSG_ALL_LAYOUT(GetLinePeriodCounters, GET_LINE_PERIOD_COUNTERS,
                   PeriodIdentification, HmiPeriodCounters)

HMI_MSG_REQ_LAYOUT(SetLineOAMthresholds, SET_LINE_OAM_THRESHOLDS,
                   NewThresholds)

HMI_MSG_REP_LAYOUT(GetLineFEvendorInfo, GET_LINE_FE_VENDOR_INFO, FEvendorInfo)

HMI_MSG_REQ_LAYOUT(SendOverheadMessage, SEND_OVERHEAD_MESSAGE, OverheadMessage)

HMI_MSG_ALL_LAYOUT(GetOverheadMessage, GET_OVERHEAD_MESSAGE,
                   OneBytePartId, OverheadMessage)

HMI_MSG_REP_LAYOUT(GetOverheadStatus, GET_OVERHEAD_STATUS, uint8)

HMI_MSG_REP_LAYOUT(GetLineLoopDiagResults, GET_LINE_LOOP_DIAG_RESULTS,
                   GetLineLoopDiagResultsMsgRep)

HMI_MSG_ALL_LAYOUT(GetLineLoopDiagHlin, GET_LINE_LOOP_DIAG_HLIN,
                   GetLineLoopDiagHlinMsgReq, LineHlin)

HMI_MSG_ALL_LAYOUT(GetLineLoopDiagHlog, GET_LINE_LOOP_DIAG_HLOG,
                   OneBytePartId, LineHlog)

HMI_MSG_ALL_LAYOUT(GetLineLoopDiagQln, GET_LINE_LOOP_DIAG_QLN,
                   OneBytePartId, LineQln)

HMI_MSG_ALL_LAYOUT(GetLineLoopDiagSnr, GET_LINE_LOOP_DIAG_SNR,
                   OneBytePartId, LoopDiagSnr)

HMI_MSG_ALL_LAYOUT(UpdateTestParameters, UPDATE_TEST_PARAMETERS,
                   UpdateTestParametersMsgReq, UpdateTestParametersMsgRep)

HMI_MSG_REP_LAYOUT(GetLineHlogNE, GET_LINE_HLOG_NE, LineHlogNE)

HMI_MSG_REP_LAYOUT(GetLineQlnNE, GET_LINE_QLN_NE, LineQlnNE)

HMI_MSG_ALL_LAYOUT(GetLineCarrierGains, GET_LINE_CARRIER_GAIN,
                   GetLineCarrierGainsMsgReq, LineCarrierGains)

HMI_MSG_ALL_LAYOUT(GetLineLinearTssi, GET_LINE_LINEAR_TSSI,
                   OneBytePartId, LineLinearTssi)

HMI_MSG_REQ_LAYOUT(LineIntroduceErrors, LINE_INTRODUCE_ERRORS,
                   LineIntroduceErrorsMsgReq)

HMI_MSG_NPL_LAYOUT(LineGoToL3, LINE_GO_TO_L3)

HMI_MSG_NPL_LAYOUT(LineGoToL2, LINE_GO_TO_L2)

HMI_MSG_NPL_LAYOUT(LineGoToL0, LINE_GO_TO_L0)

HMI_MSG_REQ_LAYOUT(LineL2PowerTrim, LINE_L2_POWER_TRIM, LineL2PowerTrimMsgReq)

HMI_MSG_ALL_LAYOUT(GetLineOamVector, GET_LINE_OAM_VECTOR,
                   GetLineOamVectorMsgReq, GetLineOamVectorMsgRep)

HMI_MSG_REP_LAYOUT(GetLinePtmCounters, GET_LINE_PTM_COUNTERS,
                   GetLinePtmCountersMsgRep)

HMI_MSG_REP_LAYOUT(GetLineMrefPsd, GET_LINE_MREF_PSD, GetLineMrefPsdMsgRep)

HMI_MSG_REQ_LAYOUT(SetLineNtrPhaseComp, SET_LINE_NTR_PHASE_COMP, NtrPhaseComp)

HMI_MSG_REP_LAYOUT(GetLineNtrPhaseComp, GET_LINE_NTR_PHASE_COMP, NtrPhaseComp)

HMI_MSG_REP_LAYOUT(GetLineG9991Counters, GET_LINE_G9991_COUNTERS, GetLineG9991CountersMsgRep)

HMI_MSG_REQ_LAYOUT(SetLineG9991Monitor, SET_LINE_G9991_MONITOR, SetLineG9991MonitorMsgReq)

HMI_MSG_REP_LAYOUT(GetLineNeInmCounters, GET_LINE_NE_INM_COUNTERS, GetLineNeInmCountersMsgRep)

HMI_MSG_REQ_LAYOUT(SetLineNeInmConfig, SET_LINE_NE_INM_CONFIG, SetLineNeInmConfigMsgReq)

HMI_MSG_ALL_LAYOUT(SetLineLogicalFrameConfig, SET_LOGICAL_FRAME_CONFIG, SetLineLogicalFrameConfigMsgReq, SetLineLogicalFrameConfigMsgReq)

HMI_MSG_REQ_LAYOUT(SetLineSeltCompatibleMode, SET_LINE_SELT_COMPATIBLE_MODE, SetLineSeltCompatibleModeMsgReq)

#ifndef BCM_CPE
HMI_MSG_REQ_LAYOUT(SetPtmBondingConfig, SET_PTM_BONDING_CONFIG,
		   SetPtmBondingConfigMsgReq)

HMI_MSG_REP_LAYOUT(GetPtmBondingConfig, GET_PTM_BONDING_CONFIG,
		   GetPtmBondingConfigMsgRep)

HMI_MSG_REP_LAYOUT(GetPtmBondingRegisters, GET_PTM_BONDING_REGISTERS,
		   GetPtmBondingRegistersMsgRep)
#endif

HMI_MSG_REQ_LAYOUT(SetLineAtmInterface, SET_LINE_ATM_INTERFACE,
		   AtmInterfaceConfig)

HMI_MSG_ALL_LAYOUT(GetLineAtmInterface, GET_LINE_ATM_INTERFACE,
		   ChannelId, AtmInterfaceConfig)

HMI_MSG_ALL_LAYOUT(GetLineIwfCounters, GET_LINE_IWF_COUNTERS,
		   ChannelId, GetLineIwfCountersMsgRep)

HMI_MSG_REQ_LAYOUT(SetLineIwfInfo, SET_LINE_IWF_INFO,
		   LineIwfInfo)

HMI_MSG_REP_LAYOUT(GetLineIwfInfo, GET_LINE_IWF_INFO,
		   LineIwfInfo)

HMI_MSG_ALL_LAYOUT(AfeDebugCommand, AFE_DEBUG_COMMAND,
                   AfeDebugCommandMsgReq, AfeDebugCommandMsgRep)

HMI_MSG_REQ_LAYOUT(SetLineTxCtrl, SET_LINE_TX_CTRL, uint8)

HMI_MSG_REP_LAYOUT(GetLineFeOamCounters, GET_LINE_FE_OAM_COUNTERS,
                   FeRawOamCounters)

#ifdef BCM_CPE
HMI_MSG_ALL_LAYOUT(GetVectorErrorSamples, GET_VECTOR_ERRORSAMPLES,
                   GetVectorErrorSamplesMsgReq, GetVectorErrorSamplesMsgRep)
#else
HMI_MSG_REQ_LAYOUT(LineSetPilotSequence, LINE_SET_PILOT_SEQUENCE,
		   LineSetPilotSequenceMsgReq)

HMI_MSG_REQ_LAYOUT(LineChangePilotSequence, LINE_CHANGE_PILOT_SEQUENCE,
                   LineChangePilotSequenceMsgReq)
  
HMI_MSG_ALL_LAYOUT(LineStartVectorDump, LINE_START_VECTOR_DUMP,
		   LineStartVectorDumpMsgReq, LineStartVectorDumpMsgRep)

HMI_MSG_ALL_LAYOUT(LineGetTxGains, LINE_GET_TX_GAINS,
		   LineGetTxGainsMsgReq, LineGetTxGainsMsgRep)

HMI_MSG_REQ_LAYOUT(LineSetPrecoder, LINE_SET_PRECODER,
		   LineSetPrecoderMsgReq)

HMI_MSG_ALL_LAYOUT(LineGetPrecoder, LINE_GET_PRECODER,
		   LineGetPrecoderMsgReq,LineGetPrecoderMsgRep)

HMI_MSG_REQ_LAYOUT(LineRuntimeCommand, LINE_RUNTIME_CMD,
		   LineRuntimeCommandMsgReq)

HMI_MSG_REP_LAYOUT(LineGetMimoBandPlan, LINE_GET_MIMO_BANDPLAN,
		   LineGetMimoBandPlanMsgRep)

HMI_MSG_ALL_LAYOUT(GetPrecoderGains, LINE_GET_PRECODER_GAINS,
                   GetPrecoderGainsMsgReq, GetPrecoderGainsMsgRep)

HMI_MSG_ALL_LAYOUT(LineGetFeq, LINE_GET_FEQ,
                   LineGetFeqMsgReq, LineGetFeqMsgRep)

HMI_MSG_ALL_LAYOUT(LineSetVectorCompletionFlag, LINE_SET_VECTOR_COMPLETION_FLAG,
		   LineSetVectorCompletionFlagMsgReq, LineSetVectorCompletionFlagMsgRep)

HMI_MSG_NPL_LAYOUT(LineResetPrecoder, LINE_RESET_PRECODER)

HMI_MSG_REQ_LAYOUT(LineSetMimoGains, LINE_SET_MIMO_GAINS, LineSetMimoGainsMsgReq)

HMI_MSG_ALL_LAYOUT(LineGetFeqOutput, LINE_GET_FEQ_OUTPUT,
                   LineGetFeqOutputMsgReq, LineGetFeqOutputMsgRep)

HMI_MSG_REQ_LAYOUT(LineSwitchStateSilent, LINE_SWITCH_STATE_SILENT, LineSwitchStateSilentMsgReq)

HMI_MSG_REQ_LAYOUT(LineInformVectConvCompl, LINE_INFORM_VECT_CONVERGENCE_COMPLETED, LineInformVectConvComplMsgReq)
#endif

HMI_MSG_REP_LAYOUT(LineGetVectoringCounters, LINE_GET_VECTORING_COUNTERS,LineGetVectoringCountersMsgRep)
  
HMI_MSG_REQ_LAYOUT(ChangeLineMds, CHANGE_LINE_MDS,
		   DtaRequest)
HMI_MSG_REQ_LAYOUT(SetLineDtaConfig, SET_LINE_DTA_CONFIG,
		   DtaConfig)
HMI_MSG_REP_LAYOUT(GetLineDtaConfig, GET_LINE_DTA_CONFIG,
		   DtaConfig)

/* }}} */
/* {{{ afeAddressExist: tells which afe registers exists */

int afeAddressExist(uint16 address, int chip)
{
  if (IN_RANGE(chip,BCM65800A0,BCM658XX)) 
    return (address < 0x6 || 
            IN_RANGE(address,   0x8,  0x1B) || 
            IN_RANGE(address,  0x20,  0x27) || 
            IN_RANGE(address, 0x100, 0x111) || 
            IN_RANGE(address, 0x118, 0x119) || 
            IN_RANGE(address, 0x120, 0x121) || 
            IN_RANGE(address, 0x180, 0x204) ||
            IN_RANGE(address, 0x220, 0x222) ||
            IN_RANGE(address, 0x240, 0x321) ||
            IN_RANGE(address, 0x324, 0x32A) ||
            IN_RANGE(address, 0x400, 0x4ff));
  else if (chip == BCM65936) 
    return (address < 0x7
            || IN_RANGE(address, 0x008, 0x01A)  
            || address == 0x01C                
            || IN_RANGE(address, 0x01F, 0x02E)  
            || IN_RANGE(address, 0x030, 0x036)  
            || IN_RANGE(address, 0x100, 0x102)  
            || IN_RANGE(address, 0x104, 0x111)  
            || IN_RANGE(address, 0x118, 0x119)  
            || IN_RANGE(address, 0x120, 0x123)  
            || IN_RANGE(address, 0x180, 0x205) 
            || IN_RANGE(address, 0x220, 0x235) 
            || IN_RANGE(address, 0x240, 0x321) 
            || IN_RANGE(address, 0x325, 0x32B) 
            || IN_RANGE(address, 0x330, 0x331) 
            || IN_RANGE(address, 0x400, 0x5ff)
#ifdef BCM_VDSL_ON_6545X
            /* extension for internal BCM65450 AFEi registers */
            || IN_RANGE(address, 0x600, 0x607)
            || IN_RANGE(address, 0x610, 0x613)
            || IN_RANGE(address, 0x700, 0x705)
            || IN_RANGE(address, 0x710, 0x718)
#endif
            );
  else if (IN_RANGE(chip,BCM65900,BCM659XX)) 
    return (address < 0x7 || 
            IN_RANGE(address,   0x8,  0x1A) || 
            (chip<BCM65900C0? IN_RANGE(address, 0x1C, 0x2E) : IN_RANGE(address, 0x1B, 0x34)) || 
            IN_RANGE(address, 0x100, 0x102) || 
            IN_RANGE(address, 0x104, 0x111) || 
            IN_RANGE(address, 0x118, 0x119) || 
            IN_RANGE(address, 0x120, 0x123) || 
            IN_RANGE(address, 0x180, 0x204) ||
            IN_RANGE(address, 0x220, 0x235) ||
            IN_RANGE(address, 0x240, 0x321) ||
            IN_RANGE(address, 0x325, chip < BCM65900C0? 0x32A:0x32C) ||
            IN_RANGE(address, 0x400, 0x4ff));
  else if (chip == BCM65400B0) 
    return (address < 3 || address == 4 || address == 8 ||
            IN_RANGE(address, 0xA, 0xD) ||
            IN_RANGE(address, 0x10, 0x1F) ||
            IN_RANGE(address, 0x2D, 0x2E) ||
            IN_RANGE(address, 0x30, 0x34) ||
            IN_RANGE(address, 0x100, 0x102) || 
            IN_RANGE(address, 0x104, 0x111) || 
            IN_RANGE(address, 0x118, 0x119) || 
            IN_RANGE(address, 0x120, 0x124) || 
            IN_RANGE(address, 0x180, 0x204) ||
            IN_RANGE(address, 0x208, 0x21D) ||
            IN_RANGE(address, 0x220, 0x22D) || address == 0x235 ||
            IN_RANGE(address, 0x240, 0x321) ||
            IN_RANGE(address, 0x325, 0x32C) ||
            IN_RANGE(address, 0x400, 0x4ff));
  else if (chip == BCM65450) 
    return (address < 7 || address == 8 ||
            IN_RANGE(address, 0xA, 0x15) ||
            IN_RANGE(address, 0x18, 0x1A) ||
            IN_RANGE(address, 0x1D, 0x25) ||
            IN_RANGE(address, 0x2A, 0x30) ||
            IN_RANGE(address, 0x32, 0x34) ||
            IN_RANGE(address, 0x40, 0x52) ||
            IN_RANGE(address, 0x100, 0x10F) || 
            IN_RANGE(address, 0x118, 0x11A) || 
            IN_RANGE(address, 0x120, 0x127) || 
            IN_RANGE(address, 0x180, 0x23A) ||
            IN_RANGE(address, 0x240, 0x321) ||
            IN_RANGE(address, 0x325, 0x330) ||
            IN_RANGE(address, 0x400, 0x43f));
  return 0;
}

/* }}} */
/* {{{ afeAddressMask: tells which bits are r/w */

int afeAddressMask(uint16 address, int chip)
{
  int mask = 0xFFFF;

  if (!afeAddressExist(address, chip))
    return 0;
  
  /* {{{ BCM65800 */
  if (IN_RANGE(chip,BCM65800A0,BCM658XX)) {
    switch(address) {
    case 0x001: mask = 0xFFF7; break;
    case 0x002: 
    case 0x003: mask = 0x0000; break;
    case 0x008: mask = 0x17F0; break;
    case 0x00A: 
    case 0x00B: mask = 0x1FFF; break;
    case 0x00D: mask = 0x0000; break;
    case 0x00E: mask = 0x003F; break;
    case 0x00F: 
    case 0x014: 
    case 0x025: 
    case 0x026: 
    case 0x027: mask = 0x0000; break;
    case 0x100: mask = 0x3FFF; break;
    case 0x101: mask = 0x0000; break;
    case 0x102: mask = 0x000F; break;
    case 0x103: mask = 0x07FF; break;
    case 0x104: 
    case 0x105: 
    case 0x106: 
    case 0x107: 
    case 0x108: 
    case 0x109: 
    case 0x10A: 
    case 0x10B: 
    case 0x10C: 
    case 0x10D: mask = 0x0FFF; break;
    case 0x10F: mask = 0x00FF; break;
    case 0x110: mask = 0x7FFF; break;
    case 0x111: mask = 0x00FF; break;
    case 0x118: mask = 0x01F0; break;
    case 0x119: mask = 0x01FF; break;
    case 0x200: mask = 0xE07F; break;
    case 0x201: mask = 0x0000; break;
    case 0x202: mask = 0x07FF; break;
    case 0x203: mask = 0x1FFF; break;
    case 0x204: mask = 0x0000; break;
    case 0x300: mask = 0x003F; break;
    case 0x302: 
    case 0x303: 
    case 0x304: 
    case 0x305: 
    case 0x306: 
    case 0x307: 
    case 0x308: 
    case 0x309: 
    case 0x30A: 
    case 0x30B: 
    case 0x30C: 
    case 0x30D: 
    case 0x30E: 
    case 0x30F: 
    case 0x310: 
    case 0x311: 
    case 0x312: 
    case 0x313: 
    case 0x314: 
    case 0x315: 
    case 0x316: 
    case 0x317: 
    case 0x318: 
    case 0x319: 
    case 0x31A: 
    case 0x31B: 
    case 0x31C: 
    case 0x31D: 
    case 0x31E: 
    case 0x31F: 
    case 0x320: 
    case 0x321: 
    case 0x324: mask = 0x0FFF; break;
    case 0x325: mask = 0x000F; break;
    case 0x326: mask = 0x0FFF; break;
    case 0x329: 
    case 0x32A: mask = 0x3FFF; break;
    default:    mask = 0xFFFF; break;
    }
  }
  /* }}} */
  /* {{{ BCM65900 */
  else if (IN_RANGE(chip,BCM65900,BCM659XX)) {
    switch(address) {
    case 0x000: mask = 0x7FFF; break;
    case 0x001: mask = 0xFFF7; break;
    case 0x002: 
    case 0x003: mask = 0x0000; break;
    case 0x004: mask = 0x3FFF; break;
    case 0x005: mask = 0x1FFF; break;
    case 0x006: mask = 0x0007; break;
    case 0x008: mask = 0x7FF0; break;
    case 0x009: mask = 0x07FF; break;
    case 0x00A: mask = 0x3FFF; break;
    case 0x00B: mask = 0x1FFF; break;
    case 0x00D: mask = 0x0000; break;
    case 0x00E: mask = 0x07FF; break;
    case 0x00F: 
    case 0x017: 
    case 0x025: 
    case 0x026: 
    case 0x027: 
    case 0x028: 
    case 0x02C: mask = 0x0000; break;
    case 0x02E: mask = 0x001F; break;
    case 0x030: mask = 0x000F; break;
    case 0x031: mask = 0x7FFF; break;
    case 0x032: mask = 0x3FFF; break;
    case 0x033: mask = 0x00FF; break;
    case 0x100: mask = 0x7FFF; break;
    case 0x101: mask = 0x0000; break;
    case 0x102: mask = 0x000F; break;
    case 0x104: 
    case 0x105: 
    case 0x106: 
    case 0x107: 
    case 0x108: 
    case 0x109: 
    case 0x10A: 
    case 0x10B: 
    case 0x10C: 
    case 0x10D: mask = 0x0FFF; break;
    case 0x10F: mask = 0x00FF; break;
    case 0x110: mask = 0x7FFF; break;
    case 0x111: mask = 0x00FF; break;
    case 0x118: mask = 0x01F0; break;
    case 0x119: mask = 0x7FFF; break;
    case 0x200: mask = 0xE7FF; break;
    case 0x201: mask = 0x0000; break;
    case 0x202: mask = 0x003F; break;
    case 0x203: mask = 0x7FFF; break;
    case 0x204: mask = 0x0000; break;
    case 0x300: mask = 0x007F; break;
    case 0x302: 
    case 0x303: 
    case 0x304: 
    case 0x305: 
    case 0x306: 
    case 0x307: 
    case 0x308: 
    case 0x309: 
    case 0x30A: 
    case 0x30B: 
    case 0x30C: 
    case 0x30D: 
    case 0x30E: 
    case 0x30F: 
    case 0x310: 
    case 0x311: 
    case 0x312: 
    case 0x313: 
    case 0x314: 
    case 0x315: 
    case 0x316: 
    case 0x317: 
    case 0x318: 
    case 0x319: 
    case 0x31A: 
    case 0x31B: 
    case 0x31C: 
    case 0x31D: 
    case 0x31E: 
    case 0x31F: 
    case 0x320: 
    case 0x321: 
    case 0x324: mask = 0x1FFF; break;
    case 0x325: mask = 0x000F; break;
    case 0x326: mask = 0x1FFF; break;
    case 0x329: 
    case 0x32A: mask = 0x3FFF; break;
    default:    mask = 0xFFFF; break;
    }
  }
  /* }}} */
  /* {{{ BCM65400 */
  else if (chip == BCM65400B0) {
    switch(address) {
    case 0x001: mask = 0xFFF7; break;
    case 0x002: mask = 0x0000; break;
    case 0x004: mask = 0x3FFF; break;
    case 0x008: mask = 0x7FF0; break;
    case 0x00A: mask = 0x3FFF; break;
    case 0x00B: mask = 0x1FFF; break;
    case 0x00D: mask = 0x0000; break;
    case 0x02E: mask = 0x001F; break;
    case 0x030: mask = 0x001F; break;
    case 0x032: mask = 0x3FFF; break;
    case 0x034: mask = 0x0000; break;
    case 0x100: mask = 0x7FFF; break;
    case 0x101: mask = 0x0000; break;
    case 0x102: mask = 0x000F; break;
    case 0x104: 
    case 0x105: 
    case 0x106: 
    case 0x107: 
    case 0x108: 
    case 0x109: 
    case 0x10A: 
    case 0x10B: 
    case 0x10C: 
    case 0x10D: mask = 0x0FFF; break;
    case 0x10F: mask = 0x00FF; break;
    case 0x110: mask = 0x7FFF; break;
    case 0x111: mask = 0x00FF; break;
    case 0x118: mask = 0x03F0; break;
    case 0x119: mask = 0x7FFF; break;
    case 0x124: mask = 0x0000; break;
    case 0x200: mask = 0xEFFF; break;
    case 0x201: mask = 0x0000; break;
    case 0x202: mask = 0x003F; break;
    case 0x203: mask = 0x7FFF; break;
    case 0x204: mask = 0x0000; break;
    case 0x20f: 
    case 0x21a: mask = 0x0001; break;
    case 0x210: 
    case 0x21b: mask = 0x0000; break;
    case 0x22d: mask = 0x0000; break;
    case 0x300: mask = 0x007F; break;
    case 0x302: 
    case 0x303: 
    case 0x304: 
    case 0x305: 
    case 0x306: 
    case 0x307: 
    case 0x308: 
    case 0x309: 
    case 0x30A: 
    case 0x30B: 
    case 0x30C: 
    case 0x30D: 
    case 0x30E: 
    case 0x30F: 
    case 0x310: 
    case 0x311: 
    case 0x312: 
    case 0x313: 
    case 0x314: 
    case 0x315: 
    case 0x316: 
    case 0x317: 
    case 0x318: 
    case 0x319: 
    case 0x31A: 
    case 0x31B: 
    case 0x31C: 
    case 0x31D: 
    case 0x31E: 
    case 0x31F: 
    case 0x320: 
    case 0x321: 
    case 0x325: mask = 0x000F; break;
    case 0x326: mask = 0x1FFF; break;
    case 0x329: 
    case 0x32A: mask = 0x3FFF; break;
    case 0x32B:
    case 0x32C: mask = 0x0000; break;
    default:    mask = 0xFFFF; break;
    }
  }
  /* }}} */
  /* {{{ BCM65450 */
  else if (chip == BCM65450) {
    switch(address) {
    case 0x001: mask = 0x0077; break;
    case 0x002: 
    case 0x003: mask = 0x0000; break;
    case 0x004: mask = 0x00F7; break;
    case 0x005: 
    case 0x006: mask = 0x1FFF; break;
    case 0x008: mask = 0xCFF0; break;
    case 0x009: mask = 0x07FF; break;
    case 0x00A: mask = 0x3FFF; break;
    case 0x00B: mask = 0x1FFF; break;
    case 0x00D: mask = 0x0000; break;
    case 0x00E: mask = 0x01FF; break;
    case 0x00F: mask = 0x0000; break;
    case 0x010: 
    case 0x011: 
    case 0x012: 
    case 0x013: 
    case 0x014: 
    case 0x015: mask = 0x1FFF; break;
    case 0x02C: 
    case 0x02F: mask = 0x0007; break;
    case 0x030: mask = 0x001F; break;
    case 0x032: mask = 0x3FFF; break;
    case 0x034: mask = 0x00FF; break;
    case 0x043: mask = 0x7FFF; break;
    case 0x049: mask = 0x01FF; break;
    case 0x04A: mask = 0x3FFF; break;
    case 0x04E: mask = 0x03FF; break;
    case 0x052: mask = 0x00FF; break;
    case 0x101: mask = 0x0000; break;
    case 0x102: mask = 0x000F; break;
    case 0x103: mask = 0x0007; break;
    case 0x104: 
    case 0x105: 
    case 0x106: 
    case 0x107: 
    case 0x108: 
    case 0x109: 
    case 0x10A: 
    case 0x10B: 
    case 0x10C: 
    case 0x10D: mask = 0x0FFF; break;
    case 0x10F: mask = 0x00FF; break;
    case 0x118: mask = 0x03F0; break;
    case 0x119: mask = 0x7FFF; break;
    case 0x11A: mask = 0x1FFF; break;
    case 0x127: mask = 0x0000; break;
    case 0x201: mask = 0x0000; break;
    case 0x202: mask = 0x0FFF; break;
    case 0x203: mask = 0x7FFF; break;
    case 0x204: mask = 0x0000; break;
    case 0x205: mask = 0x00FF; break;
    case 0x206: mask = 0x1FFF; break;
    case 0x207: mask = 0x00FF; break;
    case 0x208: 
    case 0x210: 
    case 0x218: 
    case 0x220: mask = 0xFFF7; break;
    case 0x20D: 
    case 0x215: 
    case 0x21D: 
    case 0x225: mask = 0x0000; break;
    case 0x23A: mask = 0x0000; break;
    case 0x300: mask = 0x007F; break;
    case 0x302: 
    case 0x303: 
    case 0x304: 
    case 0x305: 
    case 0x306: 
    case 0x307: 
    case 0x308: 
    case 0x309: 
    case 0x30A: 
    case 0x30B: 
    case 0x30C: 
    case 0x30D: 
    case 0x30E: 
    case 0x30F: 
    case 0x310: 
    case 0x311: 
    case 0x312: 
    case 0x313: 
    case 0x314: 
    case 0x315: 
    case 0x316: 
    case 0x317: 
    case 0x318: 
    case 0x319: 
    case 0x31A: 
    case 0x31B: 
    case 0x31C: 
    case 0x31D: 
    case 0x31E: 
    case 0x31F: 
    case 0x320: 
    case 0x321: 
    case 0x325: mask = 0x000F; break;
    case 0x326: mask = 0x1FFF; break;
    case 0x329: 
    case 0x32A: mask = 0x3FFF; break;
    case 0x32B:
    case 0x32C: mask = 0x0000; break;
    case 0x32D: 
    case 0x32E: mask = 0x3FFF; break;
    case 0x32F: 
    case 0x330: mask = 0x0000; break;
    default:    mask = 0xFFFF; break;
    }
  }
  /* }}} */
  return mask;
}

/* }}} */
/* {{{ getBandPlanEndTone */

uint16 getBandPlanEndTone(BandPlan *bp, uint32 direction) 
{
  uint16 endTone;
  
  if (direction == US) {
    if (bp->medleySetBoundsUp.endTone > 0)
      endTone  = bp->medleySetBoundsUp.endTone;
    else
      endTone  = bp->supportedSetBoundsUp.endTone;
  }
  else {
    if (bp->medleySetBoundsDn.endTone > 0)
      endTone  = bp->medleySetBoundsDn.endTone;
    else
      endTone  = bp->supportedSetBoundsDn.endTone;
  }
  return BCMIN(endTone, BCM_NB_TONE);
}
/* }}} */
/* {{{ static hash tables */

static const HashTable loopbackNames[] = {
  {BCM_LOOPBACK_NONE,                    "Disabled"               },
  {BCM_LOOPBACK_HYBRID,                  "Hybrid"                 },
  {BCM_LOOPBACK_AFE,                     "Digital_AFE_Chip"       },
  {BCM_LOOPBACK_ANALOG_AFE,              "Analog_AFE_Chip"        },
  {BCM_LOOPBACK_DSP,                     "Digital_DSP_Chip"       },
  {BCM_LOOPBACK_ATM,                     "Digital_ATM"            },
  {BCM_LOOPBACK_VDSL_ECHO,               "Echo_VDSL"              },
  {BCM_LOOPBACK_VDSL_HYBRID,             "Hybrid_VDSL"            },
  {BCM_LOOPBACK_VDSL_AFE,                "Digital_AFE_VDSL"       },
  {BCM_LOOPBACK_VDSL_ANALOG,             "Analog_AFE_VDSL"        },
  {BCM_LOOPBACK_GFAST_HYBRID,            "Hybrid_Gfast"           },
  {BCM_LOOPBACK_GFAST_DIGITAL,           "Digital_AFE_Gfast"      },
  {BCM_LOOPBACK_GFAST_ANALOG,            "Analog_AFE_Gfast"       },
  {BCM_LOOPBACK_GFAST_CALIB,             "Analog_AFE_Gfast_Calib" },
  {BCM_LOOPBACK_VDSL_DIG_WITH_TX_POWER,  "Digital_AFE_VDSL_Power" },
  {BCM_LOOPBACK_GFAST_ECHO,              "Echo_Gfast"             }};
const char* getLoopbackName(uint8 lpbk)
{
  return BCM_GET_NAME(loopbackNames, lpbk);
}

static const HashTable lineStatusNames[] = {
  {LINE_STATUS_IDL_NCON,     "Idle"                          },
  {LINE_STATUS_IDL_CONF,     "Configured"                    },
  {LINE_STATUS_RUN_ACTI,     "Running_Activation"            },
  {LINE_STATUS_RUN_INIT,     "Running_Initialisation"        },
  {LINE_STATUS_RUN_SHOW,     "Running_Showtime_L0"           },
  {LINE_STATUS_TST_MODE,     "Test_Mode"                     },
  {LINE_STATUS_RUN_LD_INIT,  "Running_Loop_Diagnostics_Init" },
  {LINE_STATUS_IDL_LD_DONE,  "Idle_Loop_Diagnostics_Done"    },
  {LINE_STATUS_RUN_SHOW_L2,  "Running_Showtime_L2"           }};
const char* getLineStatusName(uint8 status)
{
  return BCM_GET_NAME(lineStatusNames, status);
}

static const HashTable vdslProfileNames[] = {
  {0,                              "none"     },
  {BCM_SEL_VDSL2_PROFILE_8A,       "PROF_8A"  },
  {BCM_SEL_VDSL2_PROFILE_8B,       "PROF_8B"  },
  {BCM_SEL_VDSL2_PROFILE_8C,       "PROF_8C"  },
  {BCM_SEL_VDSL2_PROFILE_8D,       "PROF_8D"  },
  {BCM_SEL_VDSL2_PROFILE_12A,      "PROF_12A" },
  {BCM_SEL_VDSL2_PROFILE_12B,      "PROF_12B" },
  {BCM_SEL_VDSL2_PROFILE_17A,      "PROF_17A" },
  {BCM_SEL_VDSL2_PROFILE_30A,      "PROF_30A" },
  {BCM_SEL_VDSL2_PROFILE_35B,      "PROF_35B" }};
const char* getVdslProfileName(uint8 profile)
{
  return BCM_GET_NAME(vdslProfileNames, profile);
}

static const HashTable gfastProfileNames[] = {
  {0,                              "none"  },
  {BCM_SEL_GFAST_PROFILE_106a,     "106a"  },
  {BCM_SEL_GFAST_PROFILE_106b,     "106b"  },
  {BCM_SEL_GFAST_PROFILE_212a,     "212a"  },
  {BCM_SEL_GFAST_PROFILE_106c,     "106c"  },
  {BCM_SEL_GFAST_PROFILE_212c,     "212c"  },
  {BCM_SEL_GFAST_PROFILE_424 ,     "424 "  } };
const char* getGfastProfileName(uint8 profile)
{
  return BCM_GET_NAME(gfastProfileNames, profile);
}

static const HashTable longReachActModeNames[] = {
  {0,                              "LR not active"             },
  {1,                              "LR operation short loop"   },
  {2,                              "LR operation medium loop"  },
  {3,                              "LR operation long loop"    },
  {4,                              "LR operation GFAST"        },
};
const char* getLongReachActMode(uint8 lrActMode)
{
  return BCM_GET_NAME(longReachActModeNames, lrActMode);
}


static const HashTable xdslProtocolsNames[] = {
  {LINE_SEL_PROT_NONE,        "NONE"},
  {LINE_SEL_PROT_992_1_A,     "G.992.1_Annex_A"},
  {LINE_SEL_PROT_992_1_B,     "G.992.1_Annex_B"},
  {LINE_SEL_PROT_992_1_C,     "G.992.1_Annex_C"},
  {LINE_SEL_PROT_992_2_A,     "G.992.2_Annex_A"},
  {LINE_SEL_PROT_992_2_C,     "G.992.2_Annex_C"},
  {LINE_SEL_PROT_992_1_H,     "G.992.1_Annex_H"},
  {LINE_SEL_PROT_992_1_I,     "G.992.1_Annex_I"},
  {LINE_SEL_PROT_993_2,       "G.993.2 (VDSL2)"},
  {LINE_SEL_PROT_993_2_VECT,  "G.993.2 (Vector Friendly)"},
  {LINE_SEL_PROT_993_5,       "G.993.5"},
  {LINE_SEL_PROT_9701,        "G.9701"},
  {LINE_SEL_PROT_992_3_A,     "G.992.3_Annex_A"},
  {LINE_SEL_PROT_992_3_B,     "G.992.3_Annex_B"},
  {LINE_SEL_PROT_992_3_I,     "G.992.3_Annex_I"},
  {LINE_SEL_PROT_992_3_M,     "G.992.3_Annex_M"},
  {LINE_SEL_PROT_992_3_J,     "G.992.3_Annex_J"},
  {LINE_SEL_PROT_992_4_I,     "G.992.4_Annex_I"},
  {LINE_SEL_PROT_992_3_L1,    "G.992.3_Annex_L1"},
  {LINE_SEL_PROT_992_3_L2,    "G.992.3_Annex_L2"},
  {LINE_SEL_PROT_992_5_A,     "G.992.5_Annex_A"},
  {LINE_SEL_PROT_992_5_B,     "G.992.5_Annex_B"},
  {LINE_SEL_PROT_992_5_I,     "G.992.5_Annex_I"},
  {LINE_SEL_PROT_992_5_M,     "G.992.5_Annex_M"},
  {LINE_SEL_PROT_992_5_J,     "G.992.5_Annex_J"},
  {LINE_SEL_PROT_SADSL,       "SADSL"},
  {LINE_SEL_PROT_ANSI,        "ANSI"},
  {LINE_SEL_PROT_ETSI,        "ETSI"}};
const char* getProtocolName(int8 protocol)
{
  return BCM_GET_NAME(xdslProtocolsNames, protocol);
}

static const HashTable lineDriverPowerStateNames[] = {
  {LDPM_IDLE,                 "idle"},
  {LDPM_LISTEN,               "listen"},
  {LDPM_ACTIVATE,             "activate"},
  {LDPM_MAX_TONE_256,         "maxTone256"},
  {LDPM_MAX_TONE_512,         "maxTone512"},
  {LDPM_MAX_TONE_1024,        "maxTone1024"},
  {LDPM_MAX_TONE_2048,        "maxTone2048"},
  {LDPM_MAX_TONE_4096,        "maxTone4096"},
  {LDPM_MAX_TONE_8192,        "maxTone8192"},
  {LDPM_GFAST_TX_50,          "gfastTx50"},
  {LDPM_GFAST_RX_50,          "gfastRx50"},
  {LDPM_GFAST_TX_100,         "gfastTx100"},
  {LDPM_GFAST_RX_100,         "gfastRx100"},
  {LDPM_GFAST_TX_200,         "gfastTx200"},
  {LDPM_GFAST_RX_200,         "gfastRx200"},
  {LDPM_TEST_VDSL,            "testVdsl"},
  {LDPM_TEST_ADSL,            "testAdsl"},
  {LDPM_REDUCED_POWER,        "reducedPower"},
  {LDPM_FORCED_MODE,          "forcedTestMode"}};
const char* getLineDriverPowerStateName(uint8 state)
{
  return BCM_GET_NAME(lineDriverPowerStateNames, state);
}

/* }}} */
#endif	/* SUPPORT_HMI */

#ifdef DATA_PARSING
#include "bcm_hmiCoreMsg.c"
#undef BCM_LOG_VIEW
#include "bcm_hmiBondingMsg.c"
#undef BCM_LOG_VIEW
#include "bcm_hmiDebugMsg.c"
#undef BCM_LOG_VIEW
#include "bcm_hmiBootMsg.c"
#undef BCM_LOG_VIEW
#include "bcm_stpApi.c"
#undef BCM_LOG_VIEW
#include "bcm_stpVec.c"
#endif
