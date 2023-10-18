/*
<:copyright-BRCM:2021:proprietary:standard

   Copyright (c) 2021 Broadcom 
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
/****************************************************************************
 *
 * DslCommonDef.h
 *
 *
 * Description:
 *	This file contains common definitions used in PHY and DSL driver
 *
 *                Copyright (c) 2004 Broadcom Corporation                     
 *
 *      This material is the confidential trade secret and proprietary        
 *      information of Broadcom Corporation. It may not be reproduced,        
 *      used, sold or transferred to any third party without the prior        
 *      written consent of Broadcom Corporation. All rights reserved.         
 *                                                                            
 *****************************************************************************/

#ifndef	__DslCommonDef_H
#define	__DslCommonDef_H

/* Fire status bitmap */
#define kFireDsEnabled                                  0x1
#define kFireUsEnabled                                  0x2
#define kGinpDsEnabled                                  0x4
#define kGinpUsEnabled                                  0x8
#define kArqDs                                          (kFireDsEnabled|kGinpDsEnabled)
#define kArqUs                                          (kFireUsEnabled|kGinpUsEnabled)

/* Bonding statuses*/
#define kDslBondingATM                              0x4
#define kDslBondingPTM                              0x1

/*  line-drop reason code */
#define kRetrainReasonLosDetector                   0
#define kRetrainReasonRdiDetector                   1
#define kRetrainReasonNegativeMargin                2
#define kRetrainReasonTooManyUsFEC                  3
#define kRetrainReasonCReverb1Misdetection          4
#define kRetrainReasonTeqDsp                        5
#define kRetrainReasonAnsiTonePowerChange           6
#define kRetrainReasonIfftSizeChange                7
#define kRetrainReasonRackChange                    8
#define kRetrainReasonVendorIdSync                  9
#define kRetrainReasonTargetMarginSync             10
#define kRetrainReasonToneOrderingException        11
#define kRetrainReasonCommandHandler               12
#define kRetrainReasonDslStartPhysicalLayerCmd     13
#define kRetrainReasonUnknown                      14
#define kRetrainReasonTrainingFailure              15   /* Renamed from kRetrainReasonG992Failure, if we cannot classify the failure in any other code then we use this code, can be used for HS/ADSL/VDSL training Failures*/
#define kRetrainReasonSes                          16
#define kRetrainReasonCoMinMargin                  17
#define kRetrainReasonANFPMaskSelect               18
#define kRetrainReasonDisableAnnexL                19

/* New added codes can be used for VDSL/ADSL/HS */
#define kRetrainReasonConfigError                   20      /* This can represent general config error, LINE_FORCED_RTX_US, LINE_FORCED_RTX_DS or any other configuration mode */
#define kRetrainReasonTimeout                       21      /* All timeout errors */
#define kRetrainReasonNoCommonOPMode                22      /* Could be used during HS if no common operational mode found */
#define kRetrainReasonNoATUC                        23      /* No ATU-C detection during HS */
#define kRetrainReasonNoCommonTPSTC                 24      /* VDSL mode only If we do not find the common TPSTC mode. */
#define kRetrainReasonShowtimeFailure               25      /* Showtime failure if we cannot classify showtime retrain reason into any defined ones */

/* New reason code for Gfast */
#define kRetrainReasonLowETR                        26


/* dslModulationType */
#define	kNoCommonModulation	0x00000000
#define	kG994p1				0x00000020				/* G.994.1 or G.hs */
#define	kT1p413				0x00000040				/* T1.413 handshaking */
#define	kG992p1AnnexA		0x00000001				/* G.992.1 or G.dmt Annex A */
#define	kG992p1AnnexB		0x00000002				/* G.992.1 or G.dmt Annex B */
#define	kG992p1AnnexC		0x00000004				/* G.992.1 or G.dmt Annex C */
#define	kG992p2AnnexAB		0x00000008				/* G.992.2 or G.lite Annex A/B */
#define	kG992p2AnnexC		0x00000010				/* G.992.2 or G.lite Annex C */
#define	kG992p3AnnexA		0x00000100				/* G.992.3 or G.DMTbis Annex A */
#define	kG992p3AnnexB		0x00000200				/* G.992.3 or G.DMTbis Annex A */
#define	kG992p1AnnexI		0x00000400				/* G.992.1 Annex I */
#define kG992p3AnnexJ       0x00000800
#define kG992p5AnnexA       0x00010000              /* G.992.5 Annex A */
#define kG992p5AnnexB       0x00020000              /* G.992.5 Annex B */
#define kG992p5AnnexI       0x00040000              /* G.992.5 Annex I */
#define kG992p3AnnexM       0x00080000              /* G.992.3 Annex M */
#define kG992p5AnnexM       0x01000000              /* G.992.5 Annex M */
#define kG992p5AnnexJ       0x00100000              /* G.992.5 Annex J */
#define kG993p2AnnexA       0x02000000              /* G.993.2 Annex A */
#define kGfastAnnexA        0x04000000              /* G.fast Annex A */
#define kMgfastAnnexA       0x08000000              /* G.9711 Annex A */

#define	kG992p3AllModulations	(kG992p3AnnexA | kG992p3AnnexB | kG992p3AnnexJ | kG992p3AnnexM)
#define	kG992p5AllModulations	(kG992p5AnnexA | kG992p5AnnexB | kG992p5AnnexI | kG992p5AnnexJ | kG992p5AnnexM)

/* demodCapabilities bitmap */
#define	kEchoCancellorEnabled					0x00000001
#define	kSoftwareTimeErrorDetectionEnabled		0x00000002
#define	kSoftwareTimeTrackingEnabled			0x00000004
#define kDslTrellisEnabled			            0x00000008
#define	kHardwareTimeTrackingEnabled			0x00000010
#define kHardwareAGCEnabled						0x00000020
#define kDigitalEchoCancellorEnabled			0x00000040
#define kReedSolomonCodingEnabled				0x00000080
#define kAnalogEchoCancellorEnabled				0x00000100
#define	kT1p413Issue1SingleByteSymMode			0x00000200
#define	kDslAturXmtPowerCutbackEnabled			0x00000400
#ifdef G992_ANNEXC_LONG_REACH
#define kDslAnnexCPilot48                       0x00000800
#define kDslAnnexCReverb33_63                   0x00001000
#else
#define	kDslDisableTxFilter7ForSpecialCNXT		0x00000800	/* Disable tx fitler Id 4 for CNXT coVendorFirmwareID == 0x39 and coVendorSpecificInfo == 0x10 */
#define kDslEnableHighPrecisionTxBitswap		0x00001000
#endif
#ifdef G992_ANNEXC
#define kDslCentilliumCRCWorkAroundEnabled		0x00002000
#else
#define kDslEnableRoundUpDSLoopAttn		        0x00002000
#endif
#define	kDslBitSwapEnabled						      0x00004000
#define	kDslADILowRateOptionFixDisabled			0x00008000
#define	kDslAnymediaGSPNCrcFixEnabled			  0x00010000
#define	kDslMultiModesPreferT1p413				  0x00020000
#define	kDslT1p413UseRAck1Only					    0x00040000
#define	kDslUE9000ADI918FECFixEnabled			  0x00080000
#define	kDslG994AnnexAMultimodeEnabled			0x00100000
#define	kDslATUCXmtPowerMinimizeEnabled			0x00200000
#define	kDropOnDataErrorsDisabled			      0x00400000
#define	kDslSRAEnabled						          0x00800000

#define	kDslT1p413HigherToneLevelNeeded			0x01000000
#define	kDslT1p413SubsampleAlignmentEnabled	0x02000000
#define	kDslT1p413DisableUpstream2xIfftMode 0x04000000
/* reuse kSoftwareTimeTrackingEnabled driver bit to enable/disale multi pilot feature */
#define	kSinglePilotTrackingEnabled			0x00000004

/* test mode related demodCapabilities, for internal use only */
#define	kDslTestDemodCapMask					0xF8000000
#define	kDslSendReverbModeEnabled				0x10000000
#define	kDslSendMedleyModeEnabled				0x20000000
#define	kDslAutoRetrainDisabled					0x40000000
#ifdef ADSL_PLL_BUG_WORK_AROUND
#define kDslPllWorkaroundEnabled                0x80000000
#else
#define kDslEnableUpboTxCompOffset		        0x80000000  /* Use this bit to disable UPBO offset applied to pass CPE 158 UPBO certification */
#endif  //ADSL_PLL_BUG_WORK_AROUND
#define kDslAfeLoopbackModeEnabled              0x08000000

/* demodCapabilities bitmap2 */

#ifdef G992_ANNEXC
/* only in Annex C */
#define kDslAnnexCProfile1	    			    0x00000001
#define kDslAnnexCProfile2	    			    0x00000002
#define kDslAnnexCProfile3	    			    0x00000004
#define kDslAnnexCProfile4	    			    0x00000008
#define kDslAnnexCProfile5	    			    0x00000010
#define kDslAnnexCProfile6	    			    0x00000020
#define kDslAnnexCPilot64			   	        0x00000040
#define kDslAnnexCPilot48                       0x00000080
#define kDslAnnexCPilot32			   	        0x00000100
#define kDslAnnexCPilot16			   	        0x00000200
#define kDslAnnexCA48B48			   		    0x00000400
#define kDslAnnexCA24B24			    	    0x00000800
#define kDslAnnexCReverb33_63                   0x00001000
#define kDslAnnexCCReverb6_31	  		        0x00002000

#define kDslAnnexIShapedSSVI                    0x00004000
#define kDslAnnexIFlatSSVI                      0x00008000

#define kDslAnnexIPilot64			   	        0x00010000
#define kDslAnnexIA48B48			   		    0x00020000
#define kDslAnnexIPilot128			   	        0x00040000
#define kDslAnnexIPilot96			   	        0x00080000
#endif

/* Only in Annex A */
/* Bits 0 to 8 : Annex M submask control */
/* bit 9 : enable custom mode            */
#define kDslAnnexMEU32                          (1<<0)
#define kDslAnnexMEU36                          (1<<1)
#define kDslAnnexMEU40                          (1<<2)
#define kDslAnnexMEU44                          (1<<3)
#define kDslAnnexMEU48                          (1<<4)
#define kDslAnnexMEU52                          (1<<5)
#define kDslAnnexMEU56                          (1<<6)
#define kDslAnnexMEU60                          (1<<7)
#define kDslAnnexMEU64                          (1<<8)
#define kDslAnnexMcustomModeShift               9
#define kDslAnnexMcustomMode                    (1<<kDslAnnexMcustomModeShift)
/* Bits 10 to 15 : When kDslRetrainOnSesEnabled is enabled, this is the amount
 * of time (in units of five seconds) with continuous SES before the modem
 * retrains.
 */
#define kDslSesRetrainThresholdMask				0x0000FC00
#define kDslSesRetrainThresholdShift			10
#define kDslDisableL2                           0x00010000
#define kDigEcShowtimeUpdateDisabled            0x00020000
#define kDigEcShowtimeFastUpdateDisabled        0x00040000
#define kDslRetrainOnSesEnabled                 0x00080000
#define kDsl24kbyteInterleavingEnabled          0x00100000
#define kDslRetrainOnDslamMinMargin             0x00200000
#define kDslFireDsSupported                     0x00400000
#define kDslFireUsSupported                     0x00800000
#define kDslDisableNitro                        0x01000000
#define kDslReinCounterMeasureControl           0x02000000
#define kDslPhyRDelayRxQSupported               0x04000000
#define kDslPhyRNoDelayRxQSupported             0x08000000
#define kDslForceFastBS						    0x80000000
#define kDslDemod2Reserved						0x70000000

/* Features bitmap */
#define	kG992p2RACK1   						    0x00000001
#define	kG992p2RACK2							0x00000002
#define	kG992p2DBM								0x00000004
#define	kG992p2FastRetrain						0x00000008
#define	kG992p2RS16								0x00000010
#define	kG992p2ClearEOCOAM						0x00000020
#define	kG992NTREnabled							0x00000040
#define	kG992p2EraseAllStoredProfiles			0x00000080
#define kG992p2FeaturesNPar2Mask                0x0000003B
#define kG992p2FeaturesNPar2Shift                        0

#define kG992p1RACK1                            0x00000100
#define kG992p1RACK2                            0x00000200
#define kG992p1STM                              0x00000800
#define kG992p1ATM                              0x00001000
#define	kG992p1ClearEOCOAM						0x00002000
#define kG992p1FeaturesNPar2Mask                0x00003B00
#define kG992p1FeaturesNPar2Shift                        8
#define kG992p1DualLatencyUpstream				0x00004000
#define kG992p1DualLatencyDownstream			0x00008000
#define kG992p1HigherBitRates					0x40000000

#if defined(G992P1_ANNEX_I)
#define kG992p1HigherBitRates1over3				0x80000000
#define kG992p1AnnexIShapedSSVI                 0x00000001
#define kG992p1AnnexIFlatSSVI                   0x00000002
#define kG992p1AnnexIPilotFlag			   		0x00000008
#define kG992p1AnnexIPilot64			   		0x00000001
#define kG992p1AnnexIPilot128			   		0x00000004
#define kG992p1AnnexIPilot96			   		0x00000008
#define kG992p1AnnexIPilotA48B48                0x00000010
#endif

#define kG992p1AnnexBRACK1                      0x00010000
#define kG992p1AnnexBRACK2                      0x00020000
#define kG992p1AnnexBUpstreamTones1to32			0x00040000
#define kG992p1AnnexBSTM                        0x00080000
#define kG992p1AnnexBATM                        0x00100000
#define	kG992p1AnnexBClearEOCOAM				0x00200000
#define kG992p1AnnexBFeaturesNPar2Mask          0x003F0000
#define kG992p1AnnexBFeaturesNPar2Shift                 16

#ifdef ANNEX_C   /* apps needing this should define ANNEX_C, or reconsider if they need ANNEX_C at all */
#define kG992p1AnnexCRACK1                      0x01000000
#define kG992p1AnnexCRACK2                      0x02000000
#define kG992p1AnnexCDBM						0x04000000
#define kG992p1AnnexCSTM                        0x08000000
#define kG992p1AnnexCATM                        0x10000000
#define	kG992p1AnnexCClearEOCOAM				0x20000000
#define kG992p1AnnexCFeaturesNPar2Mask          0x3F000000
#define kG992p1AnnexCFeaturesNPar2Shift                 24
#endif

#define kG992p1HigherBitRates1over3				0x80000000

/* auxFeatures bitmap */
#define	kG994p1PreferToExchangeCaps				0x00000001
#define	kG994p1PreferToDecideMode				0x00000002
#define	kG994p1PreferToMPMode				    0x00000004
#define	kAfePwmSyncClockShift					3
#define	kAfePwmSyncClockMask					(0xF << kAfePwmSyncClockShift)
#define	AfePwmSyncClockEnabled(val)				(((val) & kAfePwmSyncClockMask) != 0)
#define	AfePwmGetSyncClockFreq(val)				((((val) & kAfePwmSyncClockMask) >> kAfePwmSyncClockShift) - 1)
#define	AfePwmSetSyncClockFreq(val,freq)		((val) |= ((((freq)+1) << kAfePwmSyncClockShift) & kAfePwmSyncClockMask))
#define kDslAtmBondingSupported                 0x00000080
#define kDslPtmBondingSupported                 0x00000100
#define kDslT1413DisableCACT1                   0x00000200  /* Disable C-ACT1 C-ACT2 ping pong for Telefonica */
#define kDslAnnexJhandshakeB43J43Toggle         0x00000400

#define kDslG994p1DisableA43C                   0x00000800  /* Disable A43C set for Cincinati Bell */
#define kDslG992FTFeatureBit                    0x00001000  /* Enable FT specific feature for HBI DSLAMS */
#define kDslGspn13BitLimitFixDisable            0x00002000  /* disable fix (sending blank vendorId) that achieves bimax of 15 in US */

#define kDslMonitorToneFeatureDisable           0x00004000
#define kDslG992p5MinimizeDSDelay               0x00008000 /* If enabled, will reduce the achieved delay in DS to provide more room for SRA */
#define kDslG992p5MonitorNOMPSDDs               0x00010000 /* If enabled, CPE will monitor MAXNOPSD to reduce gi. Not recommended for TR-100 type of tests */
#define kDslGinpDsSupported                     0x00020000
#define kDslGinpUsSupported                     0x00040000
#define kDslEnableATTCompatibility              0x00080000 /* If enabled, ADSL modem will behave similar to 2Wire's 2701 in a set of situations for AT&T deployment */
														   /* 1) Prefer G.DMT HS over T1.413 HS */
                                                           /* 2) Prefer ADSL1 over ADSL2 AnnexL in Shortloops in an AT&T profile that has Annex L and ADSL1 enabled and ADSL2+/ADSL2 disabled */
#define kDslEnableENRMeasureForAdsl2            0x00100000 /* If enabled, allow ENR measurement during R-Lineprobe (Adsl2) */
#define kDslG992BTFeatureBit                    0x00200000 /* If enabled, CPE will monitor REIN condition and retrain to adjust AGC/Pilot to pass REIN test */
#define kDslEnableSpecialCO4AdslFilt            0x00400000 /* If enabled, CPE will use a Tx filter against IKNS DSLAMs that works better for CO4  */
                                                           /* It would have been better to use vendor specific info to distinguish, but IKNS does not send any*/
#define kDslEnableCiPolicyTwo                   0x00800000
#define kDslPTMPreemptionDisable	              0x01000000 /* If set, the modem will not declare support of PTM Preemption in HS for Adsl2/2+ or in Training for Vdsl2 */
#define kDslG992p5MinDsPCB                      0x02000000 /* If enabled, will reduce the PCB in DS to provide more noise immunity */
#define kDslDisableRNC                          0x04000000 /* Disable RNC (for RNC-capable builds) */
#define	kDslSOSEnabled                          0x08000000
#define	kDslROCEnabled                          0x10000000
#define kDslPhyRAllowNoMinRSoverhead            0x20000000
#define	kDslPhyEnableShortCLR                   0x40000000
#define	kDslPhyEnable6328ForGVT                 0x80000000

/* phyExtraCfg[0] bitmap */
/* Remove unused driver bits */
/* #define kPhyCfg1EnableImpFiltInArqMode         0x00000001   obsolete */
/* #define kPhyCfg1EnableImpFiltInFecMode         0x00000002   obsolete */
#define kPhyCfg1EnableBelgacomInterop          0x00000004
#define kPhyCfg1ExternalBondingDiscovery       0x00000008
#define kPhyCfg1ExternalBondingSlave           0x00000010
#define	kPhyCfg1EnableMoniteringToneForL2mode  0x00000020
#define kPhyCfg1EnableTelecomPolandInterop     0x00000040
#define kPhyCfg1DisableTrainingImpGating       0x00000080   /* disables training impulse gating when set */
#define kPhyCfg1DisableMeqInRein               0x00000100   /* disables margin equalization under high REIN event */
#define kPhyCfg1EnableAutonomousINP            0x00000200   /* allow cpe to boost the current INP setting */
#define kPhyCfg1EnableNegMgnAdjustment         0x00000400   /* allow ADSL negative margin adjustment option for very low noise loops */
#define	kPhyCfg1DisableEnhancedL2Bit           0x00000800	  /* if enabled, limit the total L2 power to maxL2PCB set by DSLAM at L2 request */
#define kPhyCfg1IncludeAdsl1InBondingCLR       0x00001000   /* if enabled, include ADSL1 in Bonding CLR */
#define kPhyCfg1IncludeCXSYOAMInterop          0x00002000   /* If enabled, will enable HDLC control bit work around against CXSY */
#define kPhyCfg1EnableDSFFT512Interop          0x00004000   /* If enabled, force downstream FFT size of 512 for G.DMT and ADSL2 */
#define kPhyCfg1EnableHlogInterpolation        0x00008000   /* If enabled, interpolate HAM band notching when SNR is below 6dB */
#define kPhyCfg1DisableRxDelayManagement       0x00010000   /* disables rx delay management when set */
#define	kPhyCfg1V43PSDlevel1                   0x00020000   /* 01 with 10dB down */
#define	kPhyCfg1V43PSDlevel2                   0x00040000   /* 10 with 15dB down */
#define	kPhyCfg1V43PSDlevel3                   0x00060000   /* 11 with 20dB down, also used as mask for bit 17 and 18 */
#define	kPhyCfg1ReduceCToneDetectionDelay      0x00080000   /* Reduce C-Tone detection delay after xmt R-Tone-Req for Anymedia DSLAM */
#define	kPhyCfg1G992DTFeatureBit               0x00100000   /* Enable DT Lab ADSL Annex B HIGH Noise feature */
#define	kPhyCfg1G992DisableCNXTINPmin2         0x00200000   /* Disable the feature to set INPmin to 2 from 0 for maxD>7 of CNXT DSLAMs */
#define kPhyCfg1DisableHlogMasking             0x00400000	/* Disable the feature to limit Hlog to -96.3dB */
#define kPhyCfg1DetectLUA2P72HBI               0x00800000   /* restore proper detection of CNXT A2P72_HBI instead of H563ADEF */


#define kPhyCfg1LosDropTimingBit0              0x01000000
#define kPhyCfg1LosDropTimingBit1              0x02000000
#define kPhyCfg1LosDropTimingBit2              0x04000000
#define kPhyCfg1LosDropTimingBit3              0x08000000
#define kPhyCfg1LosDropTimingBit4              0x10000000
#define kPhyCfg1LosDropTimingBit5              0x20000000

#define kPhyCfg1LosDropTimingMask              (kPhyCfg1LosDropTimingBit0|kPhyCfg1LosDropTimingBit1|kPhyCfg1LosDropTimingBit2|kPhyCfg1LosDropTimingBit3|kPhyCfg1LosDropTimingBit4|kPhyCfg1LosDropTimingBit5)
#define kPhyCfg1EnableATTNDRframingAllTargets  0x40000000   /* Enable framing inclusive attainable data rate computation for all modes */
#define kPhyCfg1AlternateTogglingStartPhase    0x80000000

/* phyExtraCfg[1] bitmap */
#define kPhyCfg2AltBondingRSForHighNoise       0x00000001
#define kPhyCfg2DetectTrainingOnXtalk          0x00000002
#define kPhyCfg2DisablePtmNonBondingConnection 0x00000004
#define kPhyCfg2EnableLabTestModeInBonding     0x00000008
#define kPhyCfg2TxPafEnabled                   0x00000010
#define kPhyCfg2SendWIREtoGspnG992p3           0x00000020
#define kPhyCfg2EnbleReferenceNoiseCancelling  0x00000040  /* Enable Reference Noise cancelling for BCM63138/148 */
#define kPhyCfg2EnbleSTGRTSSIhandling          0x00000080  /* Enable TSSI handling for STGR_LIM_A2P_48_HB firmware TAO 9.14.2*/
#define kPhyCfg2EnableGfastFdxMode             0x00000100  /* Enable gfast FDX mode */
#define kPhyCfg2EnableGfastVdslMMode	         0x00000200  /* Enable gfast vdsl multimode enable */
#define kPhyCfg2EnableGfastVdslMMTimeOut0      0x00000400  /* gfast vdsl multimode timeout 0*/
#define kPhyCfg2EnableGfastVdslMMTimeOut1      0x00000800  /* gfast vdsl multimode timeout 1*/
#define kPhyCfg2DisableGfastTIGA               0x00001000
#define kPhyCfg2DisableGfastA43ToneSet         0x00002000
#define kPhyCfg2DisableGfastB43ToneSet         0x00004000
#define kPhyCfg2DisableGfastSRA                0x00008000
#define kPhyCfg2DisableAELEM                   0x00010000  /* Disable VDSL alternative electrical length estimation */
#define kPhyCfg2ForceSRAtoUseTargetMargin      0x00020000  /* Use the existing target margin for SRA reconfiguration (not dnshiftMargin+1dB and upshiftMargin-1 dB */
#define kPhyCfg2DisableGfastRPA                0x00040000  /* Disable GFAST RPA */
#define kPhyCfg2DisableGfastFRA                0x00080000  /* Disable GFAST FRA */
#define kPhyCfg2DisableGfastIdleSymbols        0x00100000
#define kPhyCfg2DisableRefErrSample			       0x00200000  /* To Enable reference error sample generation  */
#define kPhyCfg2EnableGfastV43ToneSet          0x00400000  /* enable V43 tone set for GFAST */
#define kPhyCfg2TTNETFeatureBit                0x00800000  /* Enable TTNET VDSL US rate feature */
#define kPhyCfg2EnableFastPLLFilter            0x01000000  /* Enable wider PLL filter with settling time of 35 TDD frame */
#define kPhyCfg2DisablePostOPVecFeqUpdate      0x02000000  /* Disable post O-P-Vector feq update  */
#define kPhyCfg2EnableVdslLCHPF                0x04000000  /* Enable LC HPF in 63268 AFE for VDSL not used on 42 branch*/
#define kPhyCfg2EnableRSCoderZeroDelay         0x08000000  /* Enable RS encoder in DS for D = 1 to match old behavior */
#define kPhyCfg2EnablePhyMaxAttnDr			   0x10000000  /* To Enable Phy Max Attn DR calculation based on Training Max Attn DR  */
#define kPhyCfg2IncreaseG994StartDelay         0x20000000  /* Increase initial G.994.1 quiet period before RTones from 100ms to 1s */
#define kPhyCfg2DisableGfastFullDTUsra         0x40000000  /* Disable full DTU changes in SRA/OLR */
#define kPhyCfg2PreferBondingOverPhyR          0x80000000  /* on 63138, advertise PhyR only in non-Bonding context */

/* phyExtraCfg[2] bitmap */
#define kPhyCfg3CapVdslTxPsd                   0x00000001  /* For !USO_only modes (for Kaon Media) to reduce echo on 138BJ CPE on 42 branch only */
#define kPhyCfg3EnableVdslLRmodeByDefault      0x00000002  /* Enabling vdsl lr mode from the start of training itself */
#define kPhyCfg3G994ImprovedRcvForHighNoise    0x00000004  /* Enable G994.1 Improved Rcv for High Noise */
#define kPhyCfg3ForceStrictFRATime             0x00000008  /* Force Strict FRA time > INPMin */
#define kPhyCfg3MinimizeGfastVDSLtoggle        0x00000010  /* Prevent AFE toggle switch from VDSL to G.Fast mode */
#define kPhyCfg3ForceF43ToneSetOnly            0x00000020  /* Force F43 toneset only for testing */
#define kPhyCfg3EnableF43cToneSet              0x00000040  /* If enabled, transmit F43c instead of F43 */
#define kPhyCfg3Reserved                       0x00008000  /* Reserved */

/* SubChannel Info bitMap for G992p1 */
#define kSubChannelASODownstream                0x00000001
#define kSubChannelAS1Downstream                0x00000002
#define kSubChannelAS2Downstream                0x00000004
#define kSubChannelAS3Downstream                0x00000008
#define kSubChannelLSODownstream                0x00000010
#define kSubChannelLS1Downstream                0x00000020
#define kSubChannelLS2Downstream                0x00000040
#define kSubChannelLS0Upstream                  0x00000080
#define kSubChannelLS1Upstream                  0x00000100
#define kSubChannelLS2Upstream                  0x00000200
#define kSubChannelInfoOctet1Mask               0x0000001F
#define kSubChannelInfoOctet2Mask               0x000003E0
#define kSubChannelInfoOctet1Shift              		 0
#define kSubChannelInfoOctet2Shift              		 5

/* VDSL and Gfast profiles */
#define		PROFILE8a 		0x1
#define		PROFILE8b 		0x2
#define		PROFILE8c 		0x4
#define		PROFILE8d 		0x8
#define		PROFILE12a 		0x10
#define		PROFILE12b 		0x20
#define		PROFILE17a 		0x40
#define		PROFILE30a 		0x80
#define		PROFILE30e 		0x100
#define		PROFILE70a 		0x200
#define   PROFILEVDSLLR     0x400
#define   PROFILEGFAST106A  0x1000  /* (GFAST_PROFILE_106A <<12) */
#define   PROFILEGFAST212A  0x2000  /* (GFAST_PROFILE_212A <<12) */
#define   PROFILEGFAST106B  0x4000  /* (GFAST_PROFILE_106B <<12) */
#define   PROFILEGFAST106C  0x8000  /* (GFAST_PROFILE_106C <<12) */
#define   PROFILEGFAST212C 0x10000  /* (GFAST_PROFILE_212C <<12) */
#define   PROFILEMGFASTP424A  0x20000
#define   PROFILEMGFASTP424D  0x40000
#define   PROFILEMGFASTQ424C  0x80000
#define   PROFILEMGFASTQ424D  0x100000

/* Long Reach VDSL2 mode defines */
#define     VDSL2LR_OFF    0
#define     VDSL2LR_SHORT  1
#define     VDSL2LR_MEDIUM 2
#define     VDSL2LR_LONG   3

/* Long Reach VDSL2 US0 types */
#define     VDSL2LR_ANNEX_A     1
#define     VDSL2LR_ANNEX_M     2
#define     VDSL2LR_ANNEX_B     4

/*cfgFlags bit defines*/
#define         CfgFlagsLdMode                          0x00000001
#define         CfgFlagsFextEqualized                   0x00000002
#define         CfgFlagsRawEthernetDS                   0x00000004
#define         CfgFlagsNoPtmCrcCalc                    0x00000008
#define         CfgFlagsNoG994AVdslToggle               0x00000010
#define         CfgFlagsAlignAfterPeriodics             0x00000020
#define         CfgFlagsVdslNoPtmSupport	              0x00000040
#define         CfgFlagsVdslNoAtmSupport	              0x00000080
#define         CfgFlagsVdslIfxPeriodic	                0x00000100 /* Enable Ifx Periodic start shift offset */
#define         CfgFlagsDynamicDFeatureDisable          0x00000200
#define         CfgFlagsDynamicFFeatureDisable          0x00000400
#define         CfgFlagsSOSFeatureDisable               0x00000800
#define         CfgFlagsVdslIknsHighPwrProfile8Disable  0x00001000
#define         CfgFlagsVdslIknsUs0FullPsdEnable        0x00002000
#define         CfgFlagsDisableVectoring                0x00004000
#define         CfgFlagsEnableATTNDRframingConstrains   0x00008000
#define         CfgFlagsDisableV43                      0x00010000
#define         CfgFlagsExtraPowerCutBack               0x00020000
#define         CfgFlagsVdslLineProbeEnable             0x00040000
#define         CfgFlagsEnableIkanosCO4Interop          0x00080000
#define         CfgFlagsUseCiPolicy2AsDefaultInVDSL2    0x00100000   /* use ciPolicy=2 as default in VDSL2 */
#define         CfgFlagsEnableErrorSamplePacketsCounter 0x00200000
#define         CfgFlagsEnableG993p2AnnexY              0x00400000
#define         CfgFlagsDynamicV43handling              0x00800000
#define         CfgFlagsAttnDrAmd1Enabled               0x01000000
#define         CfgFlagsEnableFDPS_US                   0x02000000
#define         CfgFlagsReserved0                       0x04000000
#define         CfgFlagsEnableSingleLine8KToneMode      0x08000000
#define         CfgFlagsReserved1                       0x10000000


/* Board AFE ID bitmap definitions */

#define AFE_CHIP_SHIFT				28
#define AFE_CHIP_MASK				(0xF << AFE_CHIP_SHIFT)
#define AFE_CHIP_INT				1
#define AFE_CHIP_6505				2
#define AFE_CHIP_6306				3
#define AFE_CHIP_CH0        4
#define AFE_CHIP_CH1        5
#define AFE_CHIP_GFAST      6     /* for G.fast only CH0: CH0 Tx + CH0 Rx + CH2 Rx */
#define AFE_CHIP_GFAST0     6     /* same as AFE_CHIP_GFAST but clearly indicate CH0 */
#define AFE_CHIP_GFCH0      7     /* for G.fast/VDSL combo CH0*/
#define AFE_CHIP_GFAST1     8     /* for G.fast only CH1 */
#define AFE_CHIP_GFCH1      9     /* for G.fast/VDSL combo CH1 */
#define AFE_CHIP_MAX				AFE_CHIP_GFCH1
#define AFE_CHIP_INT_BITMAP			(AFE_CHIP_INT << AFE_CHIP_SHIFT)
#define AFE_CHIP_6505_BITMAP		(AFE_CHIP_6505 << AFE_CHIP_SHIFT)
#define AFE_CHIP_6306_BITMAP		(AFE_CHIP_6306 << AFE_CHIP_SHIFT)
#define AFE_CHIP_CH0_BITMAP		  (AFE_CHIP_CH0 << AFE_CHIP_SHIFT)
#define AFE_CHIP_CH1_BITMAP		  (AFE_CHIP_CH1 << AFE_CHIP_SHIFT)
#define AFE_CHIP_RNC_BITMAP		  (AFE_CHIP_RNC << AFE_CHIP_SHIFT)
#define AFE_CHIP_GFAST_BITMAP		(AFE_CHIP_GFAST << AFE_CHIP_SHIFT)
#define AFE_CHIP_GFAST0_BITMAP	(AFE_CHIP_GFAST0 << AFE_CHIP_SHIFT)
#define AFE_CHIP_GFAST1_BITMAP	(AFE_CHIP_GFAST1 << AFE_CHIP_SHIFT)
#define AFE_CHIP_GFCH0_BITMAP		(AFE_CHIP_GFCH0 << AFE_CHIP_SHIFT)
#define AFE_CHIP_GFCH1_BITMAP		(AFE_CHIP_GFCH1 << AFE_CHIP_SHIFT)

#define AFE_CHIP_REV_SHIFT			25
#define AFE_CHIP_REV_MASK			(0x7 << AFE_CHIP_REV_SHIFT)

#define AFE_LD_SHIFT				21
#define AFE_LD_MASK					(0xF << AFE_LD_SHIFT)
#define AFE_LD_ISIL1556				    1
#define AFE_LD_6301					      2
#define AFE_LD_6302					      3
#define AFE_LD_6303					      4
#define AFE_LD_MicroSemiLE87281		4  /* reuse when AFE_CHIP_ID = 6 or 7 in gfast mode */
#define AFE_LD_6304               5
#define AFE_LD_6305               6

#define AFE_LD_MAX					AFE_LD_6305
#define AFE_LD_ISIL1556_BITMAP		(AFE_LD_ISIL1556 << AFE_LD_SHIFT)
#define AFE_LD_MicroSemiLE87281_BITMAP		(AFE_LD_MicroSemiLE87281 << AFE_LD_SHIFT)
#define AFE_LD_6301_BITMAP			(AFE_LD_6301 << AFE_LD_SHIFT)
#define AFE_LD_6302_BITMAP			(AFE_LD_6302 << AFE_LD_SHIFT)
#define AFE_LD_6303_BITMAP			(AFE_LD_6303 << AFE_LD_SHIFT)
#define AFE_LD_6304_BITMAP			(AFE_LD_6304 << AFE_LD_SHIFT)
#define AFE_LD_6305_BITMAP			(AFE_LD_6305 << AFE_LD_SHIFT)

#define AFE_LD_REV_SHIFT			18
#define AFE_LD_REV_MASK				(0x7 << AFE_LD_REV_SHIFT)
#define AFE_LD_REV_6303_VR5P3	1
#define AFE_LD_REV_6303_VR5P3_BITMAP (AFE_LD_REV_6303_VR5P3 << AFE_LD_REV_SHIFT)

#define AFE_FE_ANNEX_SHIFT			15
#define AFE_FE_ANNEX_MASK			(0x7 << AFE_FE_ANNEX_SHIFT)
#define AFE_FE_ANNEXX				        0
#define AFE_FE_ANNEXA				        1
#define AFE_FE_ANNEXB				        2
#define AFE_FE_ANNEXJ               3
#define AFE_FE_ANNEXBJ              4
#define AFE_FE_ANNEXM               5
#define AFE_FE_ANNEXC               6
#define AFE_FE_ANNEXA_BITMAP		(AFE_FE_ANNEXA << AFE_FE_ANNEX_SHIFT)
#define AFE_FE_ANNEXB_BITMAP		(AFE_FE_ANNEXB << AFE_FE_ANNEX_SHIFT)
#define AFE_FE_ANNEXJ_BITMAP        (AFE_FE_ANNEXJ << AFE_FE_ANNEX_SHIFT)
#define AFE_FE_ANNEXBJ_BITMAP       (AFE_FE_ANNEXBJ << AFE_FE_ANNEX_SHIFT)
#define AFE_FE_ANNEXM_BITMAP        (AFE_FE_ANNEXM << AFE_FE_ANNEX_SHIFT)
#define AFE_FE_ANNEXX_BITMAP        (AFE_FE_ANNEXX << AFE_FE_ANNEX_SHIFT)
#define AFE_FE_ANNEXC_BITMAP        (AFE_FE_ANNEXC << AFE_FE_ANNEX_SHIFT)

#define AFE_FE_AVMODE_SHIFT			13
#define AFE_FE_AVMODE_MASK			(0x3 << AFE_FE_AVMODE_SHIFT)
#define AFE_FE_AVMODE_COMBO			0
#define AFE_FE_AVMODE_ADSL			1
#define AFE_FE_AVMODE_VDSL			2
#define AFE_FE_AVMODE_COMBO_BITMAP	(AFE_FE_AVMODE_COMBO << AFE_FE_AVMODE_SHIFT)
#define AFE_FE_AVMODE_ADSL_BITMAP	(AFE_FE_AVMODE_ADSL << AFE_FE_AVMODE_SHIFT)
#define AFE_FE_AVMODE_VDSL_BITMAP	(AFE_FE_AVMODE_VDSL << AFE_FE_AVMODE_SHIFT)

#define AFE_FE_REV_SHIFT			  8
#define AFE_FE_REV_MASK				  (0x1F << AFE_FE_REV_SHIFT)

#define AFE_COAX_SHIFT		    	7
#define AFE_COAX_MASK				    (0x1 << AFE_COAX_SHIFT)

#define AFE_FE_RESERVE_SHIFT		0
#define AFE_FE_RESERVE_MASK			(0x7F << AFE_FE_RESERVE_SHIFT)
#define AFE_FE_RESERVE_RNC      0x40
#define AFE_FE_RESERVE_LD15V    0x20

/* obsolete VDSL only */
#define AFE_FE_REV_ISIL_MODE_MASK	0x40
#define AFE_FE_REV_ISIL_MODE_VDSL	0
#define AFE_FE_REV_ISIL_MODE_AVDSL	0x40

/* VDSL only */
#define AFE_FE_REV_ISIL_REV1		1
#define AFE_FE_REV_ISIL_REV_12_21   2   /* Use for A.12.21 which uses the 2/4 RXHPF design */

#define AFE_FE_REV_ISIL_REV1_BITMAP	(AFE_FE_REV_ISIL_REV1 << AFE_FE_REV_SHIFT)

/* combo */
#define AFE_FE_REV_6302_REV1		1
#define AFE_FE_REV_6302_REV_7_12	  1
#define AFE_FE_REV_6302_REV_7_2_21  2       // Bonding board 6306 path with additional rx LPF

#define AFE_FE_REV_6302_REV_7_2_1	  3
#define AFE_FE_REV_6302_REV_7_2		  4
#define AFE_FE_REV_6302_REV_7_2_UR2	5
#define AFE_FE_REV_6303_REV_12_3_42	5       // A.12.3.40 except using 6kV transformer
#define AFE_FE_REV_6303_REV_12_3_82	5       // A.12.3.80 except using 6kV transformer
#define AFE_FE_REV_6302_REV_7_2_2	  6
#define AFE_FE_REV_6302_REV_7_2_30	7
#define AFE_FE_REV_ISIL_6302_REV_12_40	8   // 6302 AND 12V driver for ADSL through 30a HW rev A.12.40
#define AFE_FE_REV_6303_REV_12_3_62	8       // A.12.3.60 except using 6kV transformer
#define AFE_FE_REV_6303_REV_12_3_72	8       // A.12.3.70 except using 6kV transformer
#define AFE_FE_REV_6303_REV_12_3_30	9
#define AFE_FE_REV_6303_REV_12_3_20	1
#define AFE_FE_REV_6303_REV_12_3_40	1       // AFE for ch0/ch1 with 30a support
#define AFE_FE_REV_6303_REV_12_3_60	1       // AFE for ch0/ch1 with 30a support, work with 63158
#define AFE_FE_REV_6303_REV_12_3_50	2       // AFE for ch0/ch1 w/o 30a support
#define AFE_FE_REV_6303_REV_12_3_70	3       // AFE for ch0/ch1 w 35b support
#define AFE_FE_REV_6303_REV_12_3_80	3       // AFE for ch0/ch1 w 35b support
#define AFE_FE_REV_6303_REV_12_3_35	3       // 63168 + 6303 17a only AFE
#define AFE_FE_REV_6303_REV_12_3_75	4       // AFE for ch0/ch1 w/o 35b support
#define AFE_FE_REV_6303_REV_12_3_85	4       // AFE for ch0/ch1 w/o 35b support
#define AFE_FE_REV_6303_MicroSemi_REV_12_50  1   // 63138 AFE for g.fast:
                                                 //   with AFE_CHIP_GFAST: ch0 = X.12.50
                                                 //   with AFE_CHIP_GFCH0: ch0 = X.12.50 and X.12.3.40 switchable
#define AFE_FE_REV_6303_MicroSemi_REV_12_51  2   // 63138 AFE for g.fast:
                                                 //   with AFE_CHIP_GFAST: ch0 = X.12.51
                                                 //   with AFE_CHIP_GFCH0: ch0 = X.12.51 and X.12.3.40 switchable
#define AFE_FE_REV_6304_REV_12_4_40	1       // AFE for ch0/ch1 with 6304.  Ch1 supports A/V17/V35; Ch0 supports GF
#define AFE_FE_REV_6304_REV_12_4_45	2       // AFE for ch0 that supports A/V/G

#define AFE_FE_REV_6304_REV_12_4_60	1       // AFE for ch0/ch1 with 6304.  Ch1 supports A/V17/V35; Ch0 supports GF; work with 63158
#define AFE_FE_REV_6304_REV_12_4_60_1	1       // AFE for ch0/ch1 with 6304.  GF_106; work with 63158
#define AFE_FE_REV_6305_REV_12_5_60_1	1       // AFE for ch0/ch1 with 6305.  GF_106; work with 63158
#define AFE_FE_REV_6305_REV_12_5_60_2	2       // AFE for ch0/ch1 with 6305.  GF_212; work with 63158
#define AFE_FE_REV_6304_REV_12_4_60_2	3       // AFE for ch0/ch1 with 6304.  GF_106; work with 63158
#define AFE_FE_REV_6304_REV_12_4_80	  4       // AFE for ch0/ch1 with 6304.  GF_106; work with 63146
#define AFE_FE_REV_6305_REV_12_5_80	  4       // AFE for ch0/ch1 with 6305.  GF_212; work with 63146
//#define AFE_FE_REV_6303_146__REV_12_3_60	3   // AFE for ch0 with 30e support, work with 63146
//#define AFE_FE_REV_6303_146__REV_12_3_85	4   // AFE for ch1 with 17a support, work with 63146

/*
BCM963146REF:  AFE_ID = 0x70808300 (CH0 AFE A/V path with BCM6303 line driver/6B support, AFE A.12.3.80)
               AFE_ID = 0x90808400 (CH1 AFE A/V path with BCM6303 line driver/5B support, AFE A.12.3.85)

               AFE_ID = 0x70C00300 (CH0 AFE V/G_212 path with BCM6305 line driver, AFE X.12.5.80)
               AFE_ID = 0x70A08400 (CH0 AFE A/V/G path with BCM6304 line driver, AFE A.12.4.80)

       146REF1D: gfast 212 plus a/v bonding to 17a
                 X.12.5.80 / A.12.3.80
                 A.12.3.85

       146REF2D: gfast 106 plus a/v bonding to 17a
                 A.12.4.80
                 A.12.3.85
*/


/* RNC configurations */
#define AFE_FE_REV_RNC_REV_10_1 1
#define AFE_FE_REV_RNC_REV_10_2 2
#define AFE_FE_REV_RNC_REV_10_3 3
#define AFE_FE_REV_RNC_REV_10_4 4


#define AFE_FE_REV_6302_REV1_BITMAP		(AFE_FE_REV_6302_REV1 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6302_REV_7_12_BITMAP	(AFE_FE_REV_6302_REV_7_12 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6302_REV_7_2_21_BITMAP	(AFE_FE_REV_6302_REV_7_2_21 << AFE_FE_REV_SHIFT)

#define AFE_FE_REV_6302_REV_7_2_1_BITMAP	(AFE_FE_REV_6302_REV_7_2_1 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6302_REV_7_2_BITMAP		(AFE_FE_REV_6302_REV_7_2 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6302_REV_7_2_UR2_BITMAP	(AFE_FE_REV_6302_REV_7_2_UR2 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6302_REV_7_2_2_BITMAP	(AFE_FE_REV_6302_REV_7_2_2 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6302_REV_7_2_30_BITMAP	(AFE_FE_REV_6302_REV_7_2_30 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_ISIL_6302_REV_12_40_BITMAP	(AFE_FE_REV_ISIL_6302_REV_12_40 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_30_BITMAP	(AFE_FE_REV_6303_REV_12_3_30 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_35_BITMAP	(AFE_FE_REV_6303_REV_12_3_35 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_20_BITMAP	(AFE_FE_REV_6303_REV_12_3_20 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_40_BITMAP	(AFE_FE_REV_6303_REV_12_3_40 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_42_BITMAP	(AFE_FE_REV_6303_REV_12_3_42 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_50_BITMAP	(AFE_FE_REV_6303_REV_12_3_50 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_70_BITMAP	(AFE_FE_REV_6303_REV_12_3_70 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_72_BITMAP	(AFE_FE_REV_6303_REV_12_3_72 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_75_BITMAP	(AFE_FE_REV_6303_REV_12_3_75 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_80_BITMAP	(AFE_FE_REV_6303_REV_12_3_80 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_82_BITMAP	(AFE_FE_REV_6303_REV_12_3_82 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_85_BITMAP	(AFE_FE_REV_6303_REV_12_3_85 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_MicroSemi_REV_12_50_BITMAP	(AFE_FE_REV_6303_MicroSemi_REV_12_50 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_MicroSemi_REV_12_51_BITMAP	(AFE_FE_REV_6303_MicroSemi_REV_12_51 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6304_REV_12_4_40_BITMAP	(AFE_FE_REV_6304_REV_12_4_40 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6304_REV_12_4_45_BITMAP	(AFE_FE_REV_6304_REV_12_4_45 << AFE_FE_REV_SHIFT)

#define AFE_FE_REV_6303_REV_12_3_60_BITMAP	  (AFE_FE_REV_6303_REV_12_3_60 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6303_REV_12_3_62_BITMAP	  (AFE_FE_REV_6303_REV_12_3_62 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6304_REV_12_4_60_BITMAP	  (AFE_FE_REV_6304_REV_12_4_60 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6304_REV_12_4_60_1_BITMAP	(AFE_FE_REV_6304_REV_12_4_60_1 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6304_REV_12_4_60_2_BITMAP	(AFE_FE_REV_6304_REV_12_4_60_2 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6304_REV_12_4_80_BITMAP	  (AFE_FE_REV_6304_REV_12_4_80 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6305_REV_12_5_60_1_BITMAP	(AFE_FE_REV_6305_REV_12_5_60_1 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6305_REV_12_5_60_2_BITMAP	(AFE_FE_REV_6305_REV_12_5_60_2 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6305_REV_12_5_80_BITMAP	  (AFE_FE_REV_6305_REV_12_5_80 << AFE_FE_REV_SHIFT)

/* ADSL only */

#define AFE_FE_REV_6302_REV_5_2_1	1
#define AFE_FE_REV_6302_REV_5_2_2	2
#define AFE_FE_REV_6302_REV_5_2_3	3
#define AFE_FE_REV_6301_REV_5_1_1	1
#define AFE_FE_REV_6301_REV_5_1_2	2
#define AFE_FE_REV_6301_REV_5_1_3	3
#define AFE_FE_REV_6301_REV_5_1_4	4

#define AFE_FE_REV_6302_REV_5_2_1_BITMAP	(AFE_FE_REV_6302_REV_5_2_1 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6302_REV_5_2_2_BITMAP	(AFE_FE_REV_6302_REV_5_2_2 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6302_REV_5_2_3_BITMAP	(AFE_FE_REV_6302_REV_5_2_3 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6301_REV_5_1_1_BITMAP	(AFE_FE_REV_6301_REV_5_1_1 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6301_REV_5_1_2_BITMAP	(AFE_FE_REV_6301_REV_5_1_2 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6301_REV_5_1_3_BITMAP	(AFE_FE_REV_6301_REV_5_1_3 << AFE_FE_REV_SHIFT)
#define AFE_FE_REV_6301_REV_5_1_4_BITMAP	(AFE_FE_REV_6301_REV_5_1_4 << AFE_FE_REV_SHIFT)

/* derived AFE definitions */

#if 0
#define AFE_FE_FULL_MASK			(AFE_LD_MASK | AFE_FE_ANNEX_MASK | AFE_FE_AVMODE_MASK | AFE_FE_REV_MASK)
#define AFE_FE_FULL_CHIP_MASK			(AFE_CHIP_MASK | AFE_LD_MASK | AFE_FE_ANNEX_MASK | AFE_FE_AVMODE_MASK | AFE_FE_REV_MASK)

#define AFE_FE_COMB_A_7_12			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_12)
#define AFE_FE_COMB_A_7_2_21		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_21)

#define AFE_FE_COMB_M_7_2_1			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_1)
#define AFE_FE_COMB_B_7_2			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXB_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2)
#define AFE_FE_COMB_B_7_2_UR2		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXB_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_UR2)
#define AFE_FE_COMB_J_7_2_1			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_1)
#define AFE_FE_COMB_J_7_2_2			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_2)
#define AFE_FE_COMB_BJ_7_2_2		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_2)
#define AFE_FE_COMB_BJ_7_2_1		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_1)


#define AFE_FE_ADSL_A_5_1_1			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_1)
#define AFE_FE_ADSL_A_5_2_1			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6302_REV_5_2_1)
#define AFE_FE_ADSL_A_5_2_2			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6302_REV_5_2_2)
#define AFE_FE_ADSL_M_5_1_1			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_1)
#define AFE_FE_ADSL_M_5_1_2			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_2)
#define AFE_FE_ADSL_B_5_1_1			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXB_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_1)
#define AFE_FE_ADSL_BJ_5_1_1		(AFE_LD_6301_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_1)

#else

#define AFE_FE_FULL_MASK			(AFE_LD_MASK | AFE_FE_ANNEX_MASK | AFE_FE_AVMODE_MASK | AFE_FE_REV_MASK)
#define AFE_FE_FULL_CHIP_MASK			(AFE_CHIP_MASK | AFE_LD_MASK | AFE_FE_ANNEX_MASK | AFE_FE_AVMODE_MASK | AFE_FE_REV_MASK)
#define AFE_FE_FULL_CHIP_NOANNEX_MASK			(AFE_CHIP_MASK | AFE_LD_MASK | AFE_FE_AVMODE_MASK | AFE_FE_REV_MASK)

#define AFE_FE_COMB_A_7_12			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_12_BITMAP)
#define AFE_FE_COMB_A_7_2_21		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_21_BITMAP)
#define AFE_FE_COMB_A_7_2_30		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_30_BITMAP)
#define AFE_FE_COMB_M_7_2_30		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_30_BITMAP)
#define AFE_FE_COMB_BJ_7_2_30		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_30_BITMAP)
#define AFE_FE_COMB_B_7_2_30		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXB_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_30_BITMAP)
#define AFE_FE_COMB_J_7_2_30		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_30_BITMAP)
#define AFE_FE_COMB_BJ_12_3_40_0  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_40_BITMAP)
#define AFE_FE_COMB_BJ_12_3_40_1	(AFE_LD_6303_BITMAP | AFE_CHIP_CH1_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_40_BITMAP)

#define AFE_FE_COMB_A_12_40		    (AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_ISIL_6302_REV_12_40_BITMAP)
#define AFE_FE_COMB_A_12_3_30		  (AFE_LD_6303_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_30_BITMAP)
#define AFE_FE_COMB_A_12_3_35		  (AFE_LD_6303_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_35_BITMAP)
//#define AFE_FE_COMB_A_12_3_20		(AFE_LD_6303_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_20_BITMAP)
#define AFE_FE_COMB_A_12_3_20		  (AFE_CHIP_INT_BITMAP | AFE_LD_6303_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_20_BITMAP)
#define AFE_FE_COMB_BJ_12_3_20		(AFE_LD_6303_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_20_BITMAP)
#define AFE_FE_COMB_BJ_12_3_30		(AFE_LD_6303_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_30_BITMAP)
#define AFE_FE_COMB_M_12_3_20		  (AFE_LD_6303_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_20_BITMAP)
#define AFE_FE_COMB_M_12_3_30		  (AFE_LD_6303_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_30_BITMAP)
#define AFE_FE_COMB_A_12_3_40_0		(AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_40_BITMAP)
#define AFE_FE_COMB_A_12_3_40_1		(AFE_LD_6303_BITMAP | AFE_CHIP_CH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_40_BITMAP)
#define AFE_FE_COMB_A_12_3_42_0		(AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_42_BITMAP)
#define AFE_FE_COMB_A_12_3_42_1		(AFE_LD_6303_BITMAP | AFE_CHIP_CH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_42_BITMAP)
#define AFE_FE_COMB_A_12_3VR5P3_40_0		(AFE_LD_REV_6303_VR5P3_BITMAP | AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_40_BITMAP)
#define AFE_FE_COMB_A_12_3VR5P3_40_1		(AFE_LD_REV_6303_VR5P3_BITMAP | AFE_LD_6303_BITMAP | AFE_CHIP_CH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_40_BITMAP)
#define AFE_FE_COMB_A_12_3_50		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_50_BITMAP)
#define AFE_FE_COMB_A_12_3_50_1		(AFE_LD_6303_BITMAP | AFE_CHIP_CH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_50_BITMAP)
#define AFE_FE_COMB_A_12_3_70		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_70_BITMAP)
#define AFE_FE_COMB_A_12_3_72		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_72_BITMAP)
#define AFE_FE_COMB_A_12_3_75		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_75_BITMAP)
#define AFE_FE_COMB_A_12_3_80		  (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_80_BITMAP)
#define AFE_FE_COMB_A_12_3_82		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_82_BITMAP)
#define AFE_FE_COMB_A_12_3_85		  (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_85_BITMAP)
#define AFE_FE_COMB_ALL_12_3_85		  (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH1_BITMAP |                        AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_85_BITMAP)
#define AFE_FE_COMB_M_12_3_70		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_70_BITMAP)
#define AFE_FE_COMB_M_12_3_75   	  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_75_BITMAP)
#define AFE_FE_COMB_M_12_3_80		  (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_80_BITMAP)
#define AFE_FE_COMB_M_12_3_82		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_82_BITMAP)
#define AFE_FE_COMB_M_12_3_85		  (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_85_BITMAP)
#define AFE_FE_COMB_BJ_12_3_70		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_70_BITMAP)
#define AFE_FE_COMB_BJ_12_3_75		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_75_BITMAP)
#define AFE_FE_COMB_BJ_12_3_80		  (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_80_BITMAP)
#define AFE_FE_COMB_BJ_12_3_85		  (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_85_BITMAP)
#define AFE_FE_COMB_X_12_50		    (AFE_LD_6303_BITMAP | AFE_CHIP_GFAST_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_MicroSemi_REV_12_50_BITMAP)
#define AFE_FE_COMB_A_12_50		    (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_MicroSemi_REV_12_50_BITMAP)
#define AFE_FE_COMB_BJ_12_50		  (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_MicroSemi_REV_12_50_BITMAP)
#define AFE_FE_COMB_X_12_51		    (AFE_LD_6303_BITMAP | AFE_CHIP_GFAST_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_MicroSemi_REV_12_51_BITMAP)
#define AFE_FE_COMB_A_12_51		      (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_MicroSemi_REV_12_51_BITMAP)
#define AFE_FE_COMB_BJ_12_51		    (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_MicroSemi_REV_12_51_BITMAP)
#define AFE_FE_COMB_BJ_12_3_50		  (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_50_BITMAP)
#define AFE_FE_COMB_BJ_12_3_50_1    (AFE_LD_6303_BITMAP | AFE_CHIP_CH1_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_50_BITMAP)
#define AFE_FE_COMB_A_12_3_60_0     (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_60_BITMAP)
#define AFE_FE_COMB_A_12_3_60_1     (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_60_BITMAP)
#define AFE_FE_COMB_A_12_3_62_0     (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_62_BITMAP)
#define AFE_FE_COMB_A_12_3_62_1     (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_62_BITMAP)
#define AFE_FE_COMB_M_12_3_60_0     (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_60_BITMAP)
#define AFE_FE_COMB_M_12_3_60_1     (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_60_BITMAP)
#define AFE_FE_COMB_BJ_12_3_60_0     (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_60_BITMAP)
#define AFE_FE_COMB_BJ_12_3_60_1     (AFE_LD_6303_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_60_BITMAP)
//#define AFE_FE_COMB_M_12_4_60_0     (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)
//#define AFE_FE_COMB_M_12_4_60_1     (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)

#define AFE_FE_COMB_X_12_4_40		    (AFE_LD_6304_BITMAP | AFE_CHIP_GFAST_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_40_BITMAP)
#define AFE_FE_COMB_X_12_4_40_GF    (AFE_LD_6304_BITMAP | AFE_CHIP_GFAST_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_40_BITMAP)
#define AFE_FE_COMB_X_12_4_40_V	    (AFE_LD_6304_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_40_BITMAP)
#define AFE_FE_COMB_A_12_4_40		    (AFE_LD_6304_BITMAP | AFE_CHIP_CH1_BITMAP   | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_40_BITMAP)
#define AFE_FE_COMB_A_12_4_45		    (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_45_BITMAP)
#define AFE_FE_COMB_A_12_4_45_GF	  (AFE_LD_6304_BITMAP | AFE_CHIP_GFAST_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_45_BITMAP)
#define AFE_FE_COMB_A_12_4_45_V		  (AFE_LD_6304_BITMAP | AFE_CHIP_CH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_45_BITMAP)
#define AFE_FE_COMB_X_12_4_60_0     (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)
#define AFE_FE_COMB_X_12_4_60_1     (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)
#define AFE_FE_COMB_ALL_12_4_60_0     (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)
#define AFE_FE_COMB_ALL_12_4_60_1     (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)
#define AFE_FE_COMB_A_12_4_60_0	    (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)
#define AFE_FE_COMB_A_12_4_60_1     (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)
#define AFE_FE_COMB_A_12_4_80	      (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_80_BITMAP)
#define AFE_FE_COMB_ALL_12_4_80	    (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_80_BITMAP)
#define AFE_FE_COMB_A_12_4_60_1_0   (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_1_BITMAP)
#define AFE_FE_COMB_A_12_4_60_1_1   (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_1_BITMAP)
#define AFE_FE_COMB_A_12_4_60_2_0   (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_2_BITMAP)
#define AFE_FE_COMB_A_12_4_60_2_1   (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_2_BITMAP)
#define AFE_FE_COMB_M_12_4_60_0	    (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)
#define AFE_FE_COMB_M_12_4_60_1     (AFE_LD_6304_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6304_REV_12_4_60_BITMAP)

#define AFE_FE_COMB_X_12_5_60_1_0   (AFE_LD_6305_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6305_REV_12_5_60_1_BITMAP)
#define AFE_FE_COMB_X_12_5_60_1_1   (AFE_LD_6305_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6305_REV_12_5_60_1_BITMAP)
#define AFE_FE_COMB_X_12_5_60_2_0   (AFE_LD_6305_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6305_REV_12_5_60_2_BITMAP)
#define AFE_FE_COMB_X_12_5_60_2_1   (AFE_LD_6305_BITMAP | AFE_CHIP_GFCH1_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6305_REV_12_5_60_2_BITMAP)
#define AFE_FE_COMB_X_12_5_80       (AFE_LD_6305_BITMAP | AFE_CHIP_GFCH0_BITMAP | AFE_FE_ANNEXX_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6305_REV_12_5_80_BITMAP)


// 12.3.40 AnnexM
#define AFE_FE_COMB_M_12_3_40_0		(AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP |  AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP  | AFE_FE_REV_6303_REV_12_3_40_BITMAP)
#define AFE_FE_COMB_M_12_3_40_1		(AFE_LD_6303_BITMAP | AFE_CHIP_CH1_BITMAP |  AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP  | AFE_FE_REV_6303_REV_12_3_40_BITMAP)
// 12.3.50 AnnexM
#define AFE_FE_COMB_M_12_3_50	    (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP |  AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_50_BITMAP)

// 12.3.40 AnnexC
#define AFE_FE_COMB_C_12_3_40_0     (AFE_LD_6303_BITMAP | AFE_CHIP_CH0_BITMAP |  AFE_FE_ANNEXC_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_40_BITMAP)
#define AFE_FE_COMB_C_12_3_40_1     (AFE_LD_6303_BITMAP | AFE_CHIP_CH1_BITMAP |  AFE_FE_ANNEXC_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6303_REV_12_3_40_BITMAP)


#define AFE_FE_COMB_M_7_2_1			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_1_BITMAP)
#define AFE_FE_COMB_B_7_2			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXB_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_BITMAP)
#define AFE_FE_COMB_B_7_2_UR2		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXB_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_UR2_BITMAP)
#define AFE_FE_COMB_J_7_2_1			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_1_BITMAP)
#define AFE_FE_COMB_J_7_2_2			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_2_BITMAP)
#define AFE_FE_COMB_BJ_7_2_2		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_2_BITMAP)
#define AFE_FE_COMB_BJ_7_2_1		(AFE_LD_6302_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_COMBO_BITMAP | AFE_FE_REV_6302_REV_7_2_1_BITMAP)


#define AFE_FE_ADSL_A_5_1_1			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_1_BITMAP)
#define AFE_FE_ADSL_A_5_1_2			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_2_BITMAP)
#define AFE_FE_ADSL_M_5_1_2			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_2_BITMAP)
#define AFE_FE_ADSL_A_5_1_3			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_3_BITMAP)
#define AFE_FE_ADSL_A_5_1_4			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_4_BITMAP)
#define AFE_FE_ADSL_A_5_2_1			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6302_REV_5_2_1_BITMAP)
#define AFE_FE_ADSL_A_5_2_2			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6302_REV_5_2_2_BITMAP)
#define AFE_FE_ADSL_A_5_2_3			(AFE_LD_6302_BITMAP | AFE_FE_ANNEXA_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6302_REV_5_2_3_BITMAP)
#define AFE_FE_ADSL_M_5_1_1			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXM_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_1_BITMAP)
#define AFE_FE_ADSL_B_5_1_1			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXB_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_1_BITMAP)
#define AFE_FE_ADSL_BJ_5_1_1		(AFE_LD_6301_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_1_BITMAP)
#define AFE_FE_ADSL_B_5_1_2			(AFE_LD_6301_BITMAP | AFE_FE_ANNEXB_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_2_BITMAP)
#define AFE_FE_ADSL_BJ_5_1_2		(AFE_LD_6301_BITMAP | AFE_FE_ANNEXBJ_BITMAP | AFE_FE_AVMODE_ADSL_BITMAP | AFE_FE_REV_6301_REV_5_1_2_BITMAP)

#endif

/* reserved */
#define AFE_RESERVED_SHIFT			0
#define AFE_RESERVED_MASK			(0xFF << AFE_RESERVED_SHIFT)

#endif	/* __DslCommonDef_H */
