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

/******************************************************************************
 *   FileName        : bcm_hmiLineMsg.h
 *   Purpose         : Header for all line manager HMI message types
 *   Limitations     : None
 *   Creation Date   : 18-Sep-2001
 *   Current Release : $Name: $
 ******************************************************************************/
#ifndef BCM_HMI_LINE_MSG_H
# define BCM_HMI_LINE_MSG_H

#ifndef SUPPORT_HMI
# include "bcm_userdef.h"
#endif
# include "bcm_hmiCoreMsg.h"

/* {{{ Application ID */

# define LINE_MANAGER_ID(line) BCM_DEV_LINE_ID(line)

/* }}} */
/* {{{ Available services */

# define RESET_LINE                              0x101
# define SET_LINE_TRAFFIC_CONFIGURATION          0x102
# define GET_LINE_TRAFFIC_CONFIGURATION          0x103
# define SET_LINE_PHYSICAL_LAYER_CONFIGURATION   0x104
# define GET_LINE_PHYSICAL_LAYER_CONFIGURATION   0x105
# define SET_LINE_TEST_CONFIGURATION             0x106
# define GET_LINE_TEST_CONFIGURATION             0x107
# define START_LINE                              0x108
# define STOP_LINE                               0x109
# define GET_LINE_COUNTERS                       0x10A
# define GET_LINE_STATUS                         0x10B
# define GET_LINE_FEATURES                       0x10C
# define SET_LINE_PHY_CONFIG_VDSL                0x10D
# define GET_LINE_PHY_CONFIG_VDSL                0x10E
# define READ_EOC_REGISTER                       0x110
# define GET_EOC_READ_RESULT                     0x111
# define WRITE_EOC_REGISTER                      0x112
# define GET_EOC_WRITE_RESULT                    0x113
# define SEND_TRANSPARENT_EOC_MESSAGE            0x114
# define GET_TRANSPARENT_EOC_MESSAGE             0x115
# define GET_HISTOGRAM                           0x116 
# define GET_BAND_PLAN_VDSL                      0x118
# define GET_VDSL_ECHO                           0x119
# define GET_LINE_OAM_VECTOR                     0x11A
# define GET_LINE_PTM_COUNTERS                   0x11B
# define GET_LINE_MREF_PSD                       0x11C
# define SET_LINE_GFAST_CONFIG                   0x11E
# define GET_LINE_GFAST_CONFIG                   0x11F
# define ENTER_LINE_TEST_MODE                    0x120
# define EXIT_LINE_TEST_MODE                     0x121
# define SET_LINE_EXTRA_CONFIG                   0x122
# define GET_LINE_EXTRA_CONFIG                   0x123
# define START_LOOP_CHARACTERISATION             0x124
# define GET_LOOP_CHARACTERISATION               0x125
# define START_SIGNAL_GENERATION_AND_MEASUREMENT 0x126
# define GET_SIGNAL_MEASUREMENT                  0x127
# define START_MANUFACTURING_TEST                0x128
# define GET_MANUFACTURING_TEST_RESULT           0x129
# define GET_LINE_AGC_SETTING                    0x12A
# define GET_LINE_FE_PILOTS                      0x12B
# define GET_ECHO                                0x12D
# define GET_ECHO_VARIANCE                       0x12E
# define GET_LINE_BIT_ALLOCATION                 0x12F
# define GET_SELT                                0x130
# define GET_LINE_PERIOD_COUNTERS                0x131
# define SET_LINE_OAM_THRESHOLDS                 0x132
# define GET_LINE_FE_VENDOR_INFO                 0x133
# define SET_LINE_TX_CTRL                        0x134
# define LINE_GO_TO_L3                           0x136
# define GET_LINE_SNR                            0x137
# define SEND_OVERHEAD_MESSAGE                   0x138
# define GET_OVERHEAD_MESSAGE                    0x139
# define GET_OVERHEAD_STATUS                     0x13A
# define GET_LINE_LOOP_DIAG_RESULTS              0x13B
# define GET_LINE_LOOP_DIAG_HLIN                 0x13C
# define GET_LINE_LOOP_DIAG_HLOG                 0x13D
# define GET_LINE_LOOP_DIAG_QLN                  0x13E
# define GET_LINE_LOOP_DIAG_SNR                  0x13F
# define UPDATE_TEST_PARAMETERS                  0x140
# define GET_LINE_HLOG_NE                        0x141
# define GET_LINE_QLN_NE                         0x142
# define GET_LINE_CARRIER_GAIN                   0x143
# define GET_LINE_LINEAR_TSSI                    0x144
# define LINE_INTRODUCE_ERRORS                   0x145
# define LINE_GO_TO_L2                           0x146
# define LINE_GO_TO_L0                           0x147
# define LINE_L2_POWER_TRIM                      0x148
# define GET_LINE_NE_INM_COUNTERS                0x149
# define SET_LINE_NE_INM_CONFIG                  0x14A
# define SET_PTM_BONDING_CONFIG                  0x14E
# define GET_PTM_BONDING_CONFIG                  0x14F
# define GET_PTM_BONDING_REGISTERS               0x150
# define LINE_SET_PILOT_SEQUENCE                 0x151
# define LINE_START_VECTOR_DUMP                  0x152
# define LINE_SET_MIMO_GAINS                     0x153
# define LINE_RUNTIME_CMD                        0x154
#ifdef BCM_CPE
# define GET_VECTOR_ERRORSAMPLES                 0x155
#else
# define LINE_RESET_PRECODER                     0x155
#endif
# define LINE_GET_TX_GAINS                       0x156
# define LINE_SET_VECTOR_COMPLETION_FLAG         0x157
# define LINE_SET_PRECODER                       0x158
# define LINE_GET_MIMO_BANDPLAN                  0x159
# define LINE_GET_PRECODER_GAINS                 0x15A
# define LINE_GET_FEQ                            0x15B
# define LINE_GET_FEQ_OUTPUT                     0x15C
# define LINE_GET_PRECODER                       0x15D
# define SET_LINE_G9991_MONITOR                  0x15E
# define GET_LINE_G9991_COUNTERS                 0x15F
# define GET_LINE_ATM_INTERFACE                  0x161
# define GET_LINE_IWF_COUNTERS                   0x162
# define SET_LINE_IWF_INFO                       0x163
# define GET_LINE_IWF_INFO                       0x164
# define SET_LINE_ATM_INTERFACE                  0x165
# define LINE_CHANGE_PILOT_SEQUENCE              0x166
# define GET_LINE_FE_OAM_COUNTERS                0x167
# define LINE_GET_VECTORING_COUNTERS             0x168
# define LINE_SWITCH_STATE_SILENT               0x16A
# define LINE_INFORM_VECT_CONVERGENCE_COMPLETED  0x16C
# define SET_LINE_NTR_PHASE_COMP                 0x16D 
# define GET_LINE_NTR_PHASE_COMP                 0x16E 
# define GET_LINE_RMC_BIT_ALLOCATION             0x174
# define SET_LOGICAL_FRAME_CONFIG                0x17A
# define SET_LINE_GFAST_RFI_CONFIG               0x17B
# define GET_LINE_GFAST_RFI_CONFIG               0x17C
# define CHANGE_LINE_MDS                         0x17E
# define SET_LINE_DTA_CONFIG                     0x17F
# define GET_LINE_DTA_CONFIG                     0x180
# define SET_LINE_SELT_COMPATIBLE_MODE           0x181

# define AFE_DEBUG_COMMAND                       0x42C
/* }}} */

/* {{{ ResetLine service */
typedef struct ResetLineReq ResetLineReq;
struct ResetLineReq
{
  uint32 keepOam;    /* dont reset OAM state */
} BCM_PACKING_ATTRIBUTE ;

typedef ResetLineReq  ResetLineMsgReq;
extern MsgLayout      ResetLineLayout;
/* }}} */
/* {{{ Set/GetLineTrafficConfiguration services */

/* no specific constraint on min rate, kept for genericity */
# define BCM_RATE_MIN(rate) rate
/* ensure max bit rate is at least 64Kbps -- an HMI req */
# define BCM_RATE_MAX(rate) BCMAX(64, rate)

/* rate setting macro (mode is true for fixed rate) */
# define BCM_SET_RATE(bp, minRate, maxRate, mode, dir)                   \
  do {                                                                   \
    (bp)->config[dir].minBitRate = BCM_RATE_MIN(minRate);                \
    (bp)->config[dir].maxBitRate = BCM_RATE_MAX((mode)?(minRate):(maxRate)); \
  } while (0)

/* macro to set INP for VDSL provile 30a such that same noise duration
 * protection is provided as for other profiles (inp being expressed in
 * symbols) */
# define BCM_SET_INP_30A(inp) \
  ((inp) < 2 ? (inp) : ((inp) < 62 ? 2*((inp)-2)+2 : 126))

typedef struct BearerSpec BearerSpec;
struct BearerSpec
{
  uint32 minBitRate;     /* unit is Kbit/s */
  uint32 maxBitRate;     /* unit is Kbit/s */
  uint8  maxDelay;       /* max delay, unit is msec, 0 means FAST channel */
  uint8  INPmin;         /* minimum required impulse noise protection
                          * unit is 1/2 symbol, allowed values are
                          * 0;1;2;4;6;..;32 */
  uint8  INPmax;         /* PhyR control :
                          * maxmimum guaranteed impulse noise protection.
                          * Unit is 1/2 symbol, range is [0..255].
                          * This also defines the max jitter allowed */
  uint8  minRtxRate;     /* PhyR control :
                          * minimum guaranteed relative retransmission rate.
                          * Unit is relative to the minBitRate, in 1/256th
                          * range is [0..128] (0 to 50% guaranteed relative
                          * retransmission rate)
                          * Additionally, following constrain must be satisfied:
                          * (minBitRate*256)/(256-minRtxRate)) <= maxBitRate) */
  uint8  minRSoverhead;  /* PhyR control: minimum guaranteel R/N ratio.
                          * Unit is 1/256th, range is [0..64] (0 to 25% of
                          * minimum R/N ratio) */
  uint8  minDelay;       /* min delay, unit is msec */
  uint8  noINPcheck;     /* Corresponds to 997.1 !FORCEINP parameter: bit[1] = VDSL2, bit[0] = ADSL2
                          * Far-end:  erasure decoding is allowed. If claimed used, the INP value provided
                          *           by the Far-end is reported and there is no point in validating
                          *           the required INP against the value that the formula gives.
                          *           It also means that according to G992.3/5 amd 5, the "INP_no_erasure_not_required"
                          *           (sic!) bit in the TPS-TC NPar3 octet shall be set to this value,
                          * Near-End: erasure decoding gain will be booked in the rateSelect (if erasure
                          *           decoding is not disabled in lineTest) and the INP reported accounts for it
                          */
  uint8  ratio_BC;       /* percentage of excess bandwith */
  uint8  reinCfg;        /* bits 4:0 : INPmin for rein test in 1/2 symbols.
                          * bits 7:5 : rein frequency. unit is 20Hz */
  uint8  INPmin30a;      /* INPmin for VDSL2 profile 30a (8kHz symbol size).
                          * same unit and range as INPmin */
  uint8  INPminPhyR;     /* Allow to require different INPmin when PhyR is on
                          * same unit and range as INPmin */
  uint8  DVmax;          /* maximum delay variation allowable in a single SRA - VDSL2 only */
} BCM_PACKING_ATTRIBUTE ;

# define US 0                  /* UpStream link */
# define DS 1                  /* DownStream link */

# define BCM_DIR_NAME(dir) ((dir) == US ? "US" : "DS")

typedef struct BearerTrafficConfig BearerTrafficConfig;
struct BearerTrafficConfig
{
  BearerSpec config[2];         /* US/DS for Upstream/Downstream */
  uint8      txAtmFlags;        /* normally zero; bit mask controlling the
                                 * following:
                                 * - 1<<0 : disable ATM payload scrambling
                                 * - 0<<1 : reserved (set to 0)
                                 * - 0<<2 : reserved (set to 0)
                                 * - 1<<3 : disable HEC regeneration
                                 * - 0<<4 : reserved (set to 0)
                                 * - 0<<5 : reserved (set to 0)
                                 * - 0<<6 : reserved (set to 0)
                                 * - 1<<7 : enable minimum FIFO length */
  uint8      rxAtmFlags;        /* normally zero; bit mask controlling the following:
                                 * - 1<<0 : disable scrambler
                                 * - 1<<1 : output cells during pre-sync
                                 * - 1<<2 : no cell discard on bad hec
                                 * - 1<<3 : no idle cell discard
                                 * - 1<<4 : force delineation to sync
                                 * - 1<<5 : disable unassigned cell filtering */
  int8       alpha;             /* alpha incorrect cells trigger a SYNC to HUNT transition */
  int8       delta;             /* delta correct cells trigger a PRE-SYNC to SYNC transition */
  int32      minL2BitRate;      /* Negative values in the range [-1,-99] are interpreted as a
                                 * percentage of L0 rate expressed in Kbit/s  */
  uint16     L2LowRateSlots;    /* Number of time slots during with the
                                 * meanRate < L2min before switching to L2
                                 * 0 means autonomous L2 disabled */
  uint8      L2Packet;          /* filling threshold to be reached before
                                 * leaving L2 */
  uint8      isEnabled;         /* 1 => the bearer channel is enabled (in this
                                 * case, this bearer should have been
                                 * correctly configured during the global
                                 * config */
  uint8      trafficFlags;      /* bits[1:0]: Defines the FCS mode on the traffic interface
                                 *              0 => ignore FCS
                                 *              1 => check  FCS
                                 *              2 => append FCS
                                 * bit 2    : When set, the Tx buffer size will always allow for
				 *            the max packet size before raising xoff.
                                 * bit 5    : On BCM65450 only, if not set, bits [1:0] are ignored
				 *            and the FCS setting is retrieved from the booter.
                               */
  uint8       reserved[3];
} BCM_PACKING_ATTRIBUTE ;

# define BCM_TPS_TC_PTM_PRIORITY 1
# define BCM_TPS_TC_PTM          2
# define BCM_TPS_TC_ATM          4
# define BCM_TPS_TC_PTM_GFAST    16

# define BCM_TPS_TC_ALL (BCM_TPS_TC_PTM | BCM_TPS_TC_ATM)

# define BCM_TPS_TC_NAME(type) \
  ((type) == BCM_TPS_TC_PTM       ? "PTM" : \
  ((type) == BCM_TPS_TC_ATM       ? "ATM" : \
  ((type) == BCM_TPS_TC_PTM_GFAST ? "PTM GFAST" : \
  ((type) == BCM_TPS_TC_ALL       ? "ALL" : \
  ((type) == 0                    ? "0" : "Unknown")))))

# define B0 0                   /* bearer 0 */
# define B1 1                   /* bearer 1 */
# define BCM_BEARER_NAME(bearer) ((bearer) == B0 ? "B0" : "B1")

typedef struct LineTrafficConfig LineTrafficConfig;
struct LineTrafficConfig
{
  BearerTrafficConfig bearer[2];     /* B0/B1 bearer */
  uint8               tpsTcTypeADSL; /* bit 0: priority to PTM
                                      * bit 1: enable PTM
                                      * bit 2: enable ATM */
  uint8               tpsTcTypeVDSL; /* same for VDSL protocols */
  uint8               iwfFlags;      /* bit 0: enable multi-VC mode */
  uint8               reserved;
} BCM_PACKING_ATTRIBUTE ;

typedef LineTrafficConfig  SetLineTrafficConfigurationMsgReq;
extern  MsgLayout          SetLineTrafficConfigurationLayout;

typedef LineTrafficConfig  GetLineTrafficConfigurationMsgRep;
extern  MsgLayout          GetLineTrafficConfigurationLayout;

/* }}} */
/* {{{ SetLineExtraConfig service */

typedef struct SosParameters SosParameters;
struct SosParameters
{
  uint32 SOSminBearerRate;    /* minimum resulting rate (units of
                               * 1kbps), for a SOS request to be valid. */
  uint16 SOStime;             /* SOS monitoring time window in units of
                               * 1ms. Valid range is from 64 to 16320 ms. */
  uint16 SOScrc;              /* SOS CRC trigger condition: a SOS operation is
                               * started if the number of normalized CRC's
                               * excees SOScrc inside the SOS time window */
  uint8  SOSnTones;            /* SOS tone degraded condition: a SOS operation
                               * is triggered if the percentage of degraded
                               * tone inside the SOS time window exceeds
                               * SOSntones. Unit is in percent.*/
  uint8  SOSmax;              /* Max number of successful SOS procedures
                               * allowed in a 120 seconds interval before a
                               * full re-init is triggered.
                               * Zero is a special value indicating no limit on
                               * the maximum number of SOS re coveries
                               * within this time interval.
                               * Allowed range is [0, 15].*/
  uint8  reserved[2];
} BCM_PACKING_ATTRIBUTE;

typedef struct RocParameters RocParameters;
struct RocParameters
{
  uint16 marginOffsetRocDB;     /* Extra margin on ROC tones. Unit is 1/256 dB */
  uint8  INPmin;                /* 0-16 symbols in 1/2 symbol steps */
  uint8  reserved;
} BCM_PACKING_ATTRIBUTE;

typedef struct GinpParameters GinpParameters;
struct GinpParameters
{
  uint32  ETRmax;         /* Used to compute ETR = min(ETRu,ETRmax), needed to compute the LEFTR alarm. */
  uint32  ETRmin;         /* Abort the initialisation if this rate cannot be met. */
  uint32  NDRmax;         /* G.inp specific max net datarate (over-rules the traffic config value). */
  uint8   shineRatio;     /* Assumed fraction (0.001 to 0.255) of NDR necessary to correct shine.*/
  uint8   leftrThreshold; /* When EFTR < leftrThresold*NDR, leftr defect is raised. (Unit 0.01)
                           * range is 0 to 99. Special value 0 means threshold = ETR */
  uint8   maxDelay;       /* G.inp specific (over-rules the traffic config value) */
  uint8   minDelay;       /* G.inp specific (over-rules the traffic config value) */
  uint8   INPmin;         /* G.inp specific (over-rules the traffic config value) */
  uint8   reinCfg;        /* G.inp specific (over-rules the traffic config value)
                           * 5 LSB : inp min REIN (unit is 1/2 symbol)
                           * 3 MSB : rein freq (unit is 20Hz : allowed values are 5 or 6).
                           */
  uint8   minRSoverhead;  /* minimum Reff/N. Unit is 1/256th. */
  uint8   rtxMode;        /* The allowed value follow the G998.4 definition:
                           * FORBIDDEN   0  G.998.4 retransmission not allowed.
                           * PREFERRED   1  G.998.4 retransmission is preferred by the operator
                           *                (i.e. if G.998.4 RTX capability is
                           *                supported by both XTU's, the XTU's shall
                           *                select G.998.4 operation for this
                           *                direction)
                           * FORCED      2  Force the use of the G.998.4 retransmission
                           *                (i.e. if G.998.4 RTX capability in this
                           *                direction is not supported by both XTU's,
                           *                an initialization failure shall result).
                           * TESTMODE    3  Force the use of the G.998.4 retransmission in test mode
                           *                (i.e. if G.998.4 RTX capability is not
                           *                supported by both XTU's or not selected by
                           *                the XTU's, an initialization failure shall
                           *                result).
                           */
  uint8   reserved[4];
} BCM_PACKING_ATTRIBUTE;

#define BCM_GINP_FORBIDDEN   0
#define BCM_GINP_PREFERRED   1
#define BCM_GINP_FORCED      2
#define BCM_GINP_TESTMODE    3

typedef struct LineExtraConfig LineExtraConfig;
struct LineExtraConfig
{
  GinpParameters ginpParameters[2]; /* US/DS for Upstream/Downstream */
  SosParameters sosParameters[2]; /* US/DS for Upstream/Downstream */
  RocParameters rocParameters[2]; /* US/DS for Upstream/Downstream */
  uint8 enableDynamicDTG;    /* Set bit 0 to enable SRA US Dynamic D
                              * Set bit 1 to enable SRA DS Dynamic D
                              * Set bit 2 to enable SRA US Dynamic T & G
                              * Set bit 3 to enable SRA DS Dynamic T & G
                              */
  uint8 enableROC;           /* Bit 0 is for US direction, bit 1 for DS
                              * direction */
  uint8 reserved[2];
} BCM_PACKING_ATTRIBUTE;

typedef LineExtraConfig    SetLineExtraConfigMsgReq;
extern  MsgLayout          SetLineExtraConfigLayout;

typedef LineExtraConfig    GetLineExtraConfigMsgRep;
extern  MsgLayout          GetLineExtraConfigLayout;

#ifdef SUPPORT_HMI
#define GinpConfig  GinpParameters
#define ExtraConfig LineExtraConfig
#endif

/* }}} */
/* {{{ Set/GetLinePhysicalLayerConfiguration services */

# define BCM_NB_ADSL_TONE_DS        512
# define BCM_NB_ADSL_TONE_US         64
# ifdef BCM_35B_FW_SUPPORT
#  define BCM_NB_VDSL_TONE          8192
#  define BCM_NB_VDSL_TONE_DS       6983 /* BCM_VDSL2_SHAPE_D_5_PSD 35b */
#  define BCM_NB_VDSL_TONE_US       3942 /* BCM_VDSL2_SHAPE_C_276_b_PSD 35b */
#  define BCM_NB_VDSL_MAXTONES_BAND 5399 /* BCM_VDSL2_SHAPE_A_NUS0_35B_PSD 35b */
#  define BCM_NB_VDSL_ECHO_BAND        8
# else /* BCM_35B_FW_SUPPORT */
#  define BCM_NB_VDSL_TONE          4096
#  define BCM_NB_VDSL_TONE_DS       2898 /* BCM_VDSL2_SHAPE_D_1_PSD 17a */
#  define BCM_NB_VDSL_TONE_US       2494 /* BCM_VDSL2_SHAPE_B7_9_PSD 17a */
#  define BCM_NB_VDSL_MAXTONES_BAND 2082 /* BCM_VDSL2_SHAPE_A_NUS0_35B_PSD 30a */
#  define BCM_NB_VDSL_ECHO_BAND        4
# endif /* BCM_35B_FW_SUPPORT */
# define BCM_NB_GFAST_TONE_106      2048
# define BCM_NB_GFAST_TONE_212      4096
# define BCM_NB_GFAST_TONE_424      8192
# ifdef BCM_GFAST_424_SUPPORT
#  define BCM_NB_GFAST_TONE         BCM_NB_GFAST_TONE_424
# elif defined BCM_GFAST_212_SUPPORT
#  define BCM_NB_GFAST_TONE         BCM_NB_GFAST_TONE_212
# else
#  define BCM_NB_GFAST_TONE         BCM_NB_GFAST_TONE_106
# endif
# define BCM_MAX_NB_GFAST_TONE_106  2000
# define BCM_MAX_NB_GFAST_TONE_212  4000
# define BCM_MAX_NB_GFAST_TONE_424  8150
# define BCM_NB_OAM_TG              512  /* Applies to both GFAST and VDSL2 */
# define BCM_MAX_VDSL_TONE_30A     6957 /* 30MHz expressed as 4KHz tone */

# ifdef BCM_CPE
#  define BCM_NB_ADSL_TONE_NE BCM_NB_ADSL_TONE_DS
#  define BCM_NB_ADSL_TONE_FE BCM_NB_ADSL_TONE_US
# else
#  define BCM_NB_ADSL_TONE_NE BCM_NB_ADSL_TONE_US
#  define BCM_NB_ADSL_TONE_FE BCM_NB_ADSL_TONE_DS
# endif

# if defined(BCM_GFAST_SUPPORT)
#  define BCM_NB_TONE    BCMAX(BCM_NB_VDSL_TONE   , BCM_NB_GFAST_TONE)
#  define BCM_NB_TONE_DS BCMAX(BCM_NB_VDSL_TONE_DS, BCM_NB_GFAST_TONE)
#  define BCM_NB_TONE_US BCMAX(BCM_NB_VDSL_TONE_US, BCM_NB_GFAST_TONE)
# else
#  define BCM_NB_TONE    BCM_NB_VDSL_TONE
#  define BCM_NB_TONE_DS BCM_NB_VDSL_TONE_DS
#  define BCM_NB_TONE_US BCM_NB_VDSL_TONE_US
# endif

#define BCM_CUSTOM_PSD_TEMPLATE_DN_LEN 30
#define BCM_CUSTOM_PSD_TEMPLATE_UP_LEN 4
#define BCM_TSSI_BP_IDX(bp) (((bp)>>7)&0x1FF)
#define BCM_TSSI_BP_LOG(bp) ((bp)&0x7F)
#define BCM_TSSI_BP(idx, log) ((((idx)&0x1FF)<<7) | ((log)&0x7F))

typedef struct CustomPsdTemplateDn CustomPsdTemplateDn;
struct CustomPsdTemplateDn
{
  uint8  adsl2ProtocolType;     /* Bitmap: which protocol uses this shape
                                 * bit 0: reserved
                                 * bit 1: G.992.5 AnnexA/B/M
                                 * bit 2: G.992.3 AnnexA/B/J
                                 * bit 3: G.992.3 AnnexL */
  uint8  nrOfBreakPoints;        /* effective number of breakpoints */
  uint16 breakPoint[BCM_CUSTOM_PSD_TEMPLATE_DN_LEN]; /* 9 MSB is tone index [0;511]
                                                      * 7 LSB is log tssi in -.5dB,
                                                      * limited to [0, -62] dB */
} BCM_PACKING_ATTRIBUTE;

typedef struct CustomPsdTemplateUp CustomPsdTemplateUp;
struct CustomPsdTemplateUp
{
  uint8  enableCustomTssiShape;  /* enable = 2; disable = 0 */
  uint8  nrOfBreakPoints;        /* effective number of breakpoints */
  uint16 breakPoint[BCM_CUSTOM_PSD_TEMPLATE_UP_LEN]; /* 9 MSB is tone index [0;511]
                                                      * 7 LSB is log tssi in -.5dB,
                                                      * limited to [0, -62] dB */
} BCM_PACKING_ATTRIBUTE;

typedef struct LinkConfig LinkConfig;
struct LinkConfig
{
  int16  targetMarginDB;        /* Target noise margin to be achived in showtime.
                                 * Unit is 0.1 dB, range is [0..31] dB. */
  int16  maxMarginDB;           /* Maximum noise margin. It defines the maximum
                                 * margin that must be achieved before attempting
                                 * to reduce the transmit power.  The transmit
                                 * power will not be reduced while the upstream
                                 * noise margin is lower than maxMarginDB.
                                 * Unit is 0.1 dB, range is [0..31] dB.
                                 * The special value "-1" can be used when no
                                 * Maximum Noise Margin limit is to be applied.*/
  int16  minMarginDB;           /* Minimum noise margin to be kept in
                                 * showtime. A retrain takes place if the
                                 * margin drops below the minimum for more
                                 * than 60s.
                                 * Unit is 0.1 dB, range is [0..31] dB. */
  int8   rateAdaptationFlags;   /* bit 0: enable rate adaptation through
                                 *        retrain when margin < minMargin
                                 * bit 1: enable SRA
                                 * bit 2: enable SOS (SRA must be enabled as well) */
  uint8  phyRdeltaMarginDB;     /* unit is (0.1 dB) on itf but internally 1/256 dB.
                                   if phyR is enabled, targetMargin is lowered by this amount */
  int16  downShiftNoiseMargin;  /* If the current noise margin is below the
                                 * downShiftNoiseMargin for more than the time
                                 * specified by minimumDownshiftRAInterval, the
                                 * ATU-x will trigger an SRA downshift.
                                 * Unit is 0.1 dB, range is [0..31] dB. */
  uint16 minimumDownshiftRAInterval; /* This parameter specifies the minimum time
                                      * interval during which the noise margin
                                      * should stay below the downShiftNoiseMargin
                                      * before the ATU-x triggers an SRA
                                      * downshift.
                                      * Unit is 1 second, range is [0..16383].*/
  int16  upShiftNoiseMargin;    /* If the current noise margin is above the
                                 * upShiftNoiseMargin for more than the time
                                 * specified by minimumUpshiftRAInterval, the
                                 * ATU-x will trigger an SRA upshift.
                                 * Unit is 0.1 dB, range is [0..31] dB. */
  uint16 minimumUpshiftRAInterval; /* This parameter specifies the minimum time
                                    * interval during which the noise margin
                                    * should stay above the upShiftNoiseMargin
                                    * before the ATU-x triggers an SRA upshift.
                                    * Unit is 1 second, range is [0..16383].*/
  int16  maxTxPower;            /* Maximum allowed transmit power.
                                 * Unit is 0.1 dBm, range is [-13, 20] dBm. */
} BCM_PACKING_ATTRIBUTE ;

#define PSD_NORMAL 0
#define PSD_PLUS   1
#define PSD_ANNEXM 1
#define PSD_REACH  2

#define PHY_STANDARD_DS_PSD_NORMAL (-400)
#define PHY_STANDARD_DS_PSD_PLUS   (-400)
#define PHY_STANDARD_DS_PSD_REACH  (-370)

#define PHY_STANDARD_US_PSD_NORMAL (-380)
#define PHY_STANDARD_US_PSD_ANNEXM (-404)
#define PHY_STANDARD_US_PSD_REACH  (-364)

#define PHY_STANDARD_US_PSD_V43    (-530)

#define PHY_US_PSD_NORMAL(phyConfig) (PHY_STANDARD_US_PSD_NORMAL + \
        (phyConfig).protocolSpecificLinkConfig[US].maxTxDeltaPsd[PSD_NORMAL])
#define PHY_US_PSD_ANNEXM(phyConfig) (PHY_STANDARD_US_PSD_ANNEXM + \
        (phyConfig).protocolSpecificLinkConfig[US].maxTxDeltaPsd[PSD_ANNEXM])
#define PHY_US_PSD_REACH(phyConfig)  (PHY_STANDARD_US_PSD_REACH + \
        (phyConfig).protocolSpecificLinkConfig[US].maxTxDeltaPsd[PSD_REACH])

#define PHY_DS_PSD_NORMAL(phyConfig) (PHY_STANDARD_DS_PSD_NORMAL + \
        (phyConfig).protocolSpecificLinkConfig[DS].maxTxDeltaPsd[PSD_NORMAL])
#define PHY_DS_PSD_PLUS(phyConfig)   (PHY_STANDARD_DS_PSD_PLUS + \
        (phyConfig).protocolSpecificLinkConfig[DS].maxTxDeltaPsd[PSD_PLUS])
#define PHY_DS_PSD_REACH(phyConfig)  (PHY_STANDARD_DS_PSD_REACH + \
        (phyConfig).protocolSpecificLinkConfig[DS].maxTxDeltaPsd[PSD_REACH])

typedef struct SpecificLinkConfig SpecificLinkConfig;
struct SpecificLinkConfig
{
  int16 maxTxDeltaPsd[3];       /* delta PSD w.r.t. standard, defining the
                                 * max PSD to apply: (unit is 0.1 dBm/Hz)
                                 * 0: normal (ADSL2)
                                 * 1: ADSL2PLUS (992.5)
                                 * 2: READSL (992.3 AnnexL) */
} BCM_PACKING_ATTRIBUTE;

#define PHY_OPTION_FAKE_DELT  (1U<<3)
#define PHY_OPTION_DM_ENABLED (1U<<4)

#define BCM_LOOPBACK_NONE               0
#define BCM_LOOPBACK_HYBRID             1
#define BCM_LOOPBACK_AFE                2
#define BCM_LOOPBACK_DSP                3
#define BCM_LOOPBACK_ATM                4
#define BCM_LOOPBACK_VDSL_HYBRID        6
#define BCM_LOOPBACK_VDSL_AFE           8
#define BCM_LOOPBACK_ANALOG_AFE         9
#define BCM_LOOPBACK_VDSL_ECHO          10
#define BCM_LOOPBACK_VDSL_ANALOG        11
#define BCM_LOOPBACK_GFAST_HYBRID       12
#define BCM_LOOPBACK_GFAST_DIGITAL      13
#define BCM_LOOPBACK_GFAST_ANALOG       14
#define BCM_LOOPBACK_GFAST_CALIB        15
#define BCM_LOOPBACK_VDSL_DIG_WITH_TX_POWER 16
#define BCM_LOOPBACK_GFAST_ECHO         17

const char* getLoopbackName(uint8 lpbk);
#define BCM_LOOPBACK_NAME getLoopbackName

#define BCM_MAX_L2_ATPRT 15

typedef struct PRMconfig PRMconfig;
struct PRMconfig
{
  uint8  prmMode;               /* 0: no PRM
                                 * 1: PRM @ 12kHz
                                 * 2: PRM @ 16kHz */
  uint8  reserved[3];
  uint32 prmPower;              /* 4000 limits the AGC to 12 dB */
} BCM_PACKING_ATTRIBUTE ;

#define BCM_VNS_DN_LEN 32
#define BCM_VNS_UP_LEN 16

typedef struct VirtNoiseShape VirtNoiseShape;
struct VirtNoiseShape
{
  uint16 toneIndex_US[BCM_VNS_UP_LEN];  /* Contains place for 16 tone indices
                                         * describing the virtual noise shape.
                                         * For k,l in [0,nrOfBreakPoints-1] and k<l,
                                         * toneIndex_US[k] shall be < toneIndex_US[l]
                                         * noiseLevel will be flat extended to the left
                                         * towards tone 0, and to the right
                                         * towards tone 4095.*/
  uint8  noiseLevel_US[BCM_VNS_UP_LEN]; /* Contains place for 16 PSD values of the
                                         * virtual noise.  It uses the same format
                                         * as G997.1, i.e. a value x represents a
                                         * PSD of -40-x/2 dBm/Hz). Valid range is
                                         * [0,200].  Special value 255 means no
                                         * virtual noise */
  uint16 toneIndex_DS[BCM_VNS_DN_LEN];  /* Same meaning and coding as for toneIndex_US */
  uint8  noiseLevel_DS[BCM_VNS_DN_LEN]; /* Same meaning and coding as for noiseLevel_US */
  uint8  nbOfBreakpoints_US;
  uint8  nbOfBreakpoints_DS;
  uint8  vnModeUS;                      /* Bit[0]   = Enable VN in ADSL
                                         * Bit[1]   = Enable VN in VDSL
                                         * Bit[3:2] = Referred VN: 00: TxRefVN
                                         *                       : 01: RxRefVN
                                         *                       : 10: combined RxRefVN and RxRefVNSF
                                         */
  uint8  vnModeDS;                      /* Bit[0]   = Enable VN in ADSL
                                         * Bit[1]   = Enable VN in VDSL
                                         * Bit[3:2] = Referred VN: 0x: TxRefVN
                                         *                       : 10: combined TxRefVN and TxRefVNSF
                                         */
} BCM_PACKING_ATTRIBUTE ;

#define BCM_NB_GHS_CS           18
#define BCM_GHS_CS_A43          0
#define BCM_GHS_CS_B43          1
#define BCM_GHS_CS_A43C         2
#define BCM_GHS_CS_V43          3
#define BCM_GHS_CS_F43          4
#define BCM_GHS_CS_F43C         5

#define BCM_NB_GHS_CS_TONE      3
#define BCM_GHS_IDX(cs,tone)    (BCM_NB_GHS_CS_TONE*(cs)+tone)
#define BCM_GHS_A43_IDX(i)      BCM_GHS_IDX(BCM_GHS_CS_A43 , i)
#define BCM_GHS_B43_IDX(i)      BCM_GHS_IDX(BCM_GHS_CS_B43 , i)
#define BCM_GHS_A43C_IDX(i)     BCM_GHS_IDX(BCM_GHS_CS_A43C, i)
#define BCM_GHS_V43_IDX(i)      BCM_GHS_IDX(BCM_GHS_CS_V43 , i)
#define BCM_GHS_F43_IDX(i)      BCM_GHS_IDX(BCM_GHS_CS_F43 , i)
#define BCM_GHS_F43C_IDX(i)     BCM_GHS_IDX(BCM_GHS_CS_F43C, i)

typedef struct ReinitConfig ReinitConfig;
struct ReinitConfig
{
  uint8 riPolicy;         /* not used in G.fast */
  uint8 riTimeThreshold;  /* retrain time in case of high BER event - range 5-31 sec*/
  uint8 losPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 1-20 */
  uint8 lomPersistency;   /* unit is 1s; if 0 then default will be chosen. range 2-20 */
  uint8 lorPersistency;   /* unit is 100 ms; if 0 then default will be chosen. range 1-20 */
  uint8 lowETRthreshold;  /* allowed time with ETR below ETRmin after FRA
                             range 1-30 sec, 0 disables the condition */
  uint8  reserved[2];
  uint32 lowANDEFTRthreshold;  /* unit is 1kbps */
} BCM_PACKING_ATTRIBUTE;

typedef struct PhysicalLayerConfig PhysicalLayerConfig;
struct PhysicalLayerConfig
{
  LinkConfig linkConfig[2];     /* US/DS for Upstream/Downstream */
  SpecificLinkConfig protocolSpecificLinkConfig[2]; /* US/DS for Upstream/Downstream */
  int16 maxRxPower;             /* Max receive power (ADSL2 only) */
  uint8 fixedPSD;               /* Set to 1 to forbid the modem to break the
                                 * ADSL initialization sequence and re-initialize at
                                 * a more optimal PSD level.
                                 * bit0: applies to all protocols
                                 * bit1: applies to ADSL1 protocols
                                 * bit2: applies to ADSL2 protocols
                                 * bit3: applies to PSD lowering in ADSL1
                                 * bit4: applies to PSD lowering in ADSL2
                                 */
  uint8 powerAdaptivity;        /* used when fixedPSD == 0
                                 * 0: rate Adaptivity (get Max rate)
                                 * 1: power adaptivity  (get Minimal power) */
  uint8 optionFlags;            /* Bit[0]: Set this flag to enable spectrum
                                 *         overlapping.
                                 * Bit[2]: NTR support - Annex A/B only
                                 *         0 - NTR support enabled
                                 *         1 - NTR support disabled
                                 * Bit[3]: Fake DELT support (retrain and do
                                 *         normal ini when DELT init failed)
                                 *         0 - disabled
                                 *         1 - enabled
                                 * Bit[4]: Diagnostic mode (DELT)
                                 *         0 - disabled
                                 *         1 - enabled
                                 * Bit[5]: Force use of low-pass Filter
                                 */
  uint8 loopbackMode;           /* Type of physical loopback used:
                                 *   0 means no loopback
                                 *   1 is hybrid loopback
                                 *   2 is external digital loopback
                                 *   3 is internal digital loopback
                                 *   4 is ATM loopback
                                 */
  uint8 carrierMaskUp[BCM_NB_ADSL_TONE_US/8]; /* Upstream carrier mask
                                 * Each bit corresponds to a tone, a 1 means
                                 * tone is present, a 0 means tone is masked.
                                 * carrierMaskUp[i] LSB is mapped on tone i*8+0,
                                 * carrierMaskUp[i] MSB is mapped on tone i*8+7
                                 */
  uint8 carrierMaskDn[BCM_NB_ADSL_TONE_DS/8]; /* Downstream carrier mask */
  uint8 actFailTimeout;         /* Timeout after which an activation failure
                                 * can be raised. Only affects the failures
                                 * 'NOT_FEASIBLE', 'COMM_PROBLEM'
                                 * and 'NO_PEER'.
                                 * Unit is 10s. (i.e. actFailTimeout=10 means
                                 * 100s timeout). Values below 30s are clamped
                                 * upward to 30s.
                                 */
  uint8 actFailThreshold[2];    /* Threshold on the activation failure count
                                 * that needs to be reached when the
                                 * activation timer expires to declare the
                                 * corresponding failure:
                                 * [0] -> threshold on 'NOT_FEASIBLE'
                                 * [1] -> threshold on 'COMM_PROBLEM'
                                 * Note that a threshold equal to 0 means that
                                 * no timer is used: a failure is raised
                                 * immediately.
                                 * If the failure is already present, the
                                 * threhold used is half the configured value,
                                 * such as to have an hysteresis on the
                                 * persistency check */
  uint8 reservedA;
  uint8 txFilterControl;        /* Additional Tx Filter setting:
                                 * bit 0: enable it for adsl2+
                                 * bit 1: enable it for adsl2
                                 * bit 2: enable it for adsl1 */
  uint8 psdOptimConfig;         /* Controls psd optimisation mode:
                                 * 0: no optimisation - fixed tssi shape
                                 * 1: psd optimisation: tssi for optimum psd mask filling
                                 * 2: psd shaping: shape tssi for optimum performance */
  uint8 automodePIalpha;        /* automoding alpha performance index */
  uint8 automodePIbeta;         /* automoding beta performance index */
  CustomPsdTemplateDn customPsdTemplateDn; /* custom ADSL2plus DS PSD template (together with maxDeltaPsd[PLUS]) */
  CustomPsdTemplateUp customPsdTemplateUp; /* custom ADSL2plus Annex M US PSD template (together with maxDeltaPsd[ANNEX_M]) */
  uint16 L0Time;                /* Minimum L0 time interval between L2 exit
                                 * and next L2 entry */
  uint16 L2Time;                /* Minimum L2 time interval between L2 entry
                                 * and first trim, and between trims */
  uint8 L2Atpr;                 /* Maximum Aggregate Transmit power reduction
                                 * per L2 trim. Range 0..L2Atpr */
  uint8 L2Atprt;                /* Maximum total Aggregate Transmit power
                                 * reduction in L2 mode. Range 0..15 */
  int8  hsTxPSD[BCM_NB_GHS_CS]; /* G.994.1 PSD level (in dBm/Hz):
                                 * - hsTxPsd[0, 1, 2]   : A43[40, 56, 64],
                                 * - hsTxPsd[3, 4, 5]   : B43[72, 88, 96],
                                 * - hsTxPsd[6, 7, 8]   : A43c[257, 293, 337],
                                 * - hsTxPsd[9, 10, 11] : V43[257, 383, 511],
                                 * - hsTxPsd[12, 13, 14]: F43[4368, 4440, 4488]
                                 * - hsTxPsd[15, 16, 17]: F43c[4368, 4440, reserved] */
  uint8 annexMsubModePsd;       /* Set annex M sub mode psd
                                 * 0: enables mask EU-56
                                 * 1-9: enables mask EU-(28+4*i)
                                 */
  uint8 perfWeightFactor[2];    /* VDSL2 only : weight factors use to optimize
                                 * the rate between US and DS. The
                                 * optimization criterium used is given by
                                 *   fact[1]*rateDS + fact[0]*rateUS
                                 * The allowed range is 0 - 255.
                                 * Special values:
                                 * {0,0} means perfWeightFactor = {maxBitRateDS, maxBitRateUS}
                                 * {x,0} means perfWeightFactor = {20,1}
                                 * {0,x} means perfWeightFactor = {1,20} */
  uint8 reservedB;
  uint8 ciPolicy[2];            /* Channel Initialization policies (US/DS) to
                                 * control the way the framing parameters have
                                 * to be derived. 4 LSB for ADSL CI policy, 4
                                 * MSB for VDSL CI policy, as follows:
                                 * 0: default behavior, same as before
                                 * 1: aims at maximizing (within some bounds)
                                 *    actual impulse noise protection.
                                 * 2: aims at maximizing the actual SNR margin
                                 *    (only standardized in ADSL2) */
  PRMconfig prmConfig;
  VirtNoiseShape virtNoiseShape; /* virtual noise as in G997.1 */
  TxFilterDef txFilter;          /* custom time domain Tx filter to be used on
                                  * that line. This is an opaque format, with a
                                  * set of pre-defined filter provided in the
                                  * PSD toolbox. This filter will be used
                                  * according to the current protocol, as
                                  * specified by the txFilterControl field
                                  * above. */
  int16 customTxFilterMaxTxPsd; /* maximum Tx PSD to be used in conjunction with
                                 * the configured custom Tx filter */
  ReinitConfig reinitConfig[2]; /* US/DS for Upstream/Downstream */
  uint8 monitoringTones;        /* If set, tones without capacity during
                                 * training will be sent in showtime to allow
                                 * the rate to increase when the noise is
                                 * removed (VDSL/RX only) */
  uint8 maxSRAtime;             /* time [s] before moving from LINE_SHOWTIME to
                                   LINE_SHOWTIME_WITH_TRAFFIC state. Disable = 0 */
  int8  vnSF_US;                /* US receiver-referred virtual noise scaling factor.
                                 * Valid range [-64.0, 63.5] dB, in steps of 0.5 dB */
  int8  vnSF_DS;                /* DS transmitter referred virtual noise scaling factor.
                                 * Valid range [-64.0, 63.5] dB, in steps of 0.5 dB */
  uint8 hsTxPSD_useVdslPsd;     /* only used on VDSL capable builds
                                   if set: use the DS TX PSD descriptor to 
                                   derive the PSD for each HS tone */
  uint8 reservedC[7];
} BCM_PACKING_ATTRIBUTE;

typedef PhysicalLayerConfig SetLinePhysicalLayerConfigurationMsgReq;
extern  MsgLayout           SetLinePhysicalLayerConfigurationLayout;

typedef PhysicalLayerConfig GetLinePhysicalLayerConfigurationMsgRep;
extern  MsgLayout           GetLinePhysicalLayerConfigurationLayout;

/* }}} */
/* {{{ Set/GetLinePhyConfigVDSL services */

#define BCM_4KHZ_TS  4.3125
#define BCM_8KHZ_TS  8.6250

typedef struct BreakPoint BreakPoint;
struct BreakPoint
{
  uint16 toneIndex;
  int16  psd;                  /* unit is (0.1 dB) */
} BCM_PACKING_ATTRIBUTE;


#define BCM_VDSL_NB_PSD_BP_DN 40
typedef struct PsdDescriptorDn PsdDescriptorDn;
struct PsdDescriptorDn
{
  uint8      nrOfBreakPoints;
  uint8      relax_OOB;        /* bit 0 - reserved
                                * bit 1 - if set, FW will apply extra shaping to improve performance 
                                * bit 2 - if set, extra shaping will be done in 17a mode to make sure 
                                *         that the PSD above 18.1 MHz is below -100dBm/Hz */
  uint16     dpbo_muf;         /* If not zero, provides the Maximum Usable Frequency from the DPBO 
                                * configuration. Only used in Long Reach VDSL mode to help in the merge 
                                * of the VDSL & READSL masks. */
  BreakPoint bp[BCM_VDSL_NB_PSD_BP_DN];
} BCM_PACKING_ATTRIBUTE;

#define BCM_VDSL_NB_PSD_BP_UP 20
typedef struct PsdDescriptorUp PsdDescriptorUp;
struct PsdDescriptorUp
{
  uint8      nrOfBreakPoints;
  uint8      annexUs0;          /* annexUs0 = bitmask
                                 * bits 0&1     : annex
                                 * bits 2 to 6  : subMaskInfo
                                 *
                                 * annex = 1 -> annex B
                                 * allowed values for submask info are:
                                 * 0 - NUS0
                                 * 1 - type A
                                 * 2 - type M
                                 * 3 - type B
                                 *
                                 * annex = 2 -> annex A
                                 * allowed values for submask info are:
                                 * 0  - NUS0
                                 * 1  - EU-32
                                 * 2  - EU-36
                                 * 3  - EU-40
                                 * 9  - EU-64
                                 * 10 - EU-128
                                 */
  int16      legacyUS0Psd;      /* maximum allowed PSD in US0, forcing a
                                 * ceiling on the configured mask. The mask
                                 * will only be sent without ceiling if the
                                 * CPE is known to be capable of handling a
                                 * high PSD without DS performance
                                 * degradation. Unit is 0.1 dB. */
  BreakPoint bp[BCM_VDSL_NB_PSD_BP_UP];
} BCM_PACKING_ATTRIBUTE ;

/* usefull macro to add a breakpoint to an existing PSD */
#define BCM_ADD_BREAKPOINT(list, idx, level) \
  do {(list)->bp[(list)->nrOfBreakPoints].toneIndex = (uint16) (idx);   \
      (list)->bp[(list)->nrOfBreakPoints].psd       = (int16) (level);  \
      (list)->nrOfBreakPoints++;                                        \
  } while (0)


#define VDSL_MSG_MAX_NO_OF_TONE_GROUPS_IN_BANDS_DESC 8
typedef struct VdslMsgToneGroupDescriptor VdslMsgToneGroupDescriptor;
struct VdslMsgToneGroupDescriptor
{
  uint8     noOfToneGroups;
  uint8     reserved;
  ToneGroup toneGroups[VDSL_MSG_MAX_NO_OF_TONE_GROUPS_IN_BANDS_DESC];
} BCM_PACKING_ATTRIBUTE ;

/* usefull macro to add a band to an existing bandplan */
#define BCM_ADD_BAND(bp, start, stop)                        \
  do {(bp)->toneGroups[(bp)->noOfToneGroups].startTone = (uint16) (start);\
      (bp)->toneGroups[(bp)->noOfToneGroups].endTone   = (uint16) (stop); \
      (bp)->noOfToneGroups++;                                             \
  } while (0)

/* same starting with freq in kHz */
#define BCM_ADD_BAND_FREQ(bp, startFreq, stopFreq, toneSpacing) \
 BCM_ADD_BAND(bp, ((startFreq)-1)/toneSpacing+1, (stopFreq)/toneSpacing)

/* usefull macros to add a rfi band/notch in kHz to an existing rfi description */
#define BCM_ADD_RFI_BAND(rfi, startFreq, stopFreq, toneSpacing) \
  do {(rfi)->toneGroups[(rfi)->nRfiBands].startTone = (uint16) ((startFreq)/toneSpacing);\
      (rfi)->toneGroups[(rfi)->nRfiBands++].endTone = (uint16) ((stopFreq)/toneSpacing+1); \
  } while (0)
#define BCM_ADD_RFI_GAP(rfi, startFreq, stopFreq, toneSpacing) \
  do {(rfi)->toneGroups[(rfi)->nRfiBands+(rfi)->nRfiGaps].startTone = (uint16) ((startFreq)/toneSpacing);\
      (rfi)->toneGroups[(rfi)->nRfiBands+(rfi)->nRfiGaps++].endTone = (uint16) ((stopFreq)/toneSpacing+1); \
  } while (0)

#define BCM_VDSL_NB_RFI_BANDS 16
#define BCM_VDSL_NB_RFI_GAPS 16
#define BCM_VDSL_NB_RFI_NOTCHES 24
typedef struct RfiDescriptor RfiDescriptor;
struct RfiDescriptor
{
  uint8  nRfiBands;             /* number of bands sent at -80 dBm/Hz, max = 16 */
  uint8  nRfiGaps;              /* number of bands sent with no power, max = 16 */
  ToneGroup toneGroups[BCM_VDSL_NB_RFI_NOTCHES];
} BCM_PACKING_ATTRIBUTE;


typedef struct PboSetting PboSetting;
struct PboSetting
{
  int16 a;          /* "a" parameter of value of UPBO descriptor.
                     * a/256 is the actual value and should be
                     * in [-40,-80.96] range. */
  int16 b;          /* "b" parameter of value of UPBO descriptor.
                     * b/256 is the actual value and should be
                     * in [-40.96,0] range. */
  uint16 kl0_ref;   /* kl0_ref for FEXT equalized UPBO unit = 0.1 dB.
                     * 997.1 specifies a valid range of 1.8 - 63.5 in steps of 0.1 dB
                     * 997.1 also specifies a special value of 0 (meaning no
                     * correction - just normal UPBO) */
} BCM_PACKING_ATTRIBUTE ;

typedef struct RefPsdDescriptor RefPsdDescriptor;
struct RefPsdDescriptor
{
  uint8      noOfToneGroups; /* number of US bands (not counting US0) for
                                which a PBO setting is defined.*/
  uint8      method;  /* This field controls the method used to apply UPBO
                       * (only meaningful if the noOfToneGroups value is
                       * larger than 0). The default upstream power backoff
                       * method applied follows G993.2 ammendment 1.
                       * bit 0: set upstream PBO according to DT's TS0364/96.
                       * bit 1: set upstream PBO according to the
                       *        FEXT-optimized UPBO method, G993.2, ammendment 2.
                       * bit 2: set upstream PBO according to the
                       *        FEXT-optimized UPBO method, without collaboration
                       *        from the CPE, allowing this method to be used with
                       *        a CPE that does not support ammendment 2. This
                       *        requires a retrain to install a new reference PSD
                       *        that yields the upstream PSD as desired for that
                       *        loop.
                       * If multiple bits are set, priority goes to the method
                       * controlled by bit 1, then bit 0, then bit 2. */
  PboSetting pbo[BCM_VDSL_NB_TONE_GROUPS];

} BCM_PACKING_ATTRIBUTE ;

/* usefull macro to add a pbo setting to an existing ref psd */
#define BCM_ADD_PBO_BAND(psd, valA, valB, valKl0)                   \
  do {(psd)->pbo[(psd)->noOfToneGroups].a       = (int16) (valA);   \
      (psd)->pbo[(psd)->noOfToneGroups].b       = (int16) (valB);   \
      (psd)->pbo[(psd)->noOfToneGroups].kl0_ref = (uint16) (valKl0);\
      (psd)->noOfToneGroups++;} while (0)

/* VDSL2 profiles (bitmaps, for configuration purpose) */
# define BCM_VDSL2_PROFILE_MASK_8A      (1U<<(BCM_SEL_VDSL2_PROFILE_8A -1))
# define BCM_VDSL2_PROFILE_MASK_8B      (1U<<(BCM_SEL_VDSL2_PROFILE_8B -1))
# define BCM_VDSL2_PROFILE_MASK_8C      (1U<<(BCM_SEL_VDSL2_PROFILE_8C -1))
# define BCM_VDSL2_PROFILE_MASK_8D      (1U<<(BCM_SEL_VDSL2_PROFILE_8D -1))
# define BCM_VDSL2_PROFILE_MASK_12A     (1U<<(BCM_SEL_VDSL2_PROFILE_12A-1))
# define BCM_VDSL2_PROFILE_MASK_12B     (1U<<(BCM_SEL_VDSL2_PROFILE_12B-1))
# define BCM_VDSL2_PROFILE_MASK_17A     (1U<<(BCM_SEL_VDSL2_PROFILE_17A-1))
# define BCM_VDSL2_PROFILE_MASK_30A     (1U<<(BCM_SEL_VDSL2_PROFILE_30A-1))
# define BCM_VDSL2_PROFILE_MASK_35B     (1U<<(BCM_SEL_VDSL2_PROFILE_35B-1))

# define BCM_VDSL2_PROFILE_MASK_8                          \
  (BCM_VDSL2_PROFILE_MASK_8A | BCM_VDSL2_PROFILE_MASK_8B | \
   BCM_VDSL2_PROFILE_MASK_8C | BCM_VDSL2_PROFILE_MASK_8D)

# define BCM_VDSL2_PROFILE_MASK_12 \
  (BCM_VDSL2_PROFILE_MASK_12A | BCM_VDSL2_PROFILE_MASK_12B)

# ifdef BCM_35B_FW_SUPPORT
#  define BCM_VDSL2_PROFILE_MASK_DEFAULT                     \
  (BCM_VDSL2_PROFILE_MASK_8   | BCM_VDSL2_PROFILE_MASK_12  | \
   BCM_VDSL2_PROFILE_MASK_17A | BCM_VDSL2_PROFILE_MASK_30A | \
   BCM_VDSL2_PROFILE_MASK_35B)
# else
#  define BCM_VDSL2_PROFILE_MASK_DEFAULT                     \
  (BCM_VDSL2_PROFILE_MASK_8   | BCM_VDSL2_PROFILE_MASK_12  | \
   BCM_VDSL2_PROFILE_MASK_17A | BCM_VDSL2_PROFILE_MASK_30A)
# endif

/* Special control value for vectoring */
# define BCM_VECT_ENABLE_FIXED_BANDPLAN                 0x0
# define BCM_VECT_ENABLE_AND_OVERWRITE_FIXED_BANDPLAN   0x1
# define BCM_VECT_DISABLE_FIXED_BANDPLAN                0x2
# define BCM_VECT_ENABLE_G_HS_G993_2_Y                  0x4
# define BCM_VECT_ENABLE_G_HS_G993_5                    0x8
# define BCM_VECT_ENABLE_FDPS                           0x10

typedef struct PhysicalLayerConfigVDSL PhysicalLayerConfigVDSL;
struct PhysicalLayerConfigVDSL
{
  PsdDescriptorDn    limitingMaskDn;
  PsdDescriptorUp    limitingMaskUp;
  BandPlanDescriptor bandPlanDn;
  BandPlanDescriptor bandPlanUp;
  RfiDescriptor      rfiNotches;
  RefPsdDescriptor   pboPsdUp;
  uint16             enableProfileBitmap;   /* VDSL2 profiles bitmap
                                             * see macro BCM_VDSL2_PROFILE_MASK_xx
                                             * a bit being set means that the protocol will be negociated */
  uint8              lrMode;                /* bitmap. 0=off, non-zero=on bit0 = short allowed, bit1 = medium allowed, bit2 = long allowed -> advised value = 7   */
  uint8              powerCutBackOffset;    /* imposes DS power constraint on
                                               short loop.
                                               The range is [0,32].
                                               Advised value is 12,
                                               corresponding to a
                                               upper maximum of 12dB DS power cutback.*/
  int16              forceElectricalLength; /* if >=0, enforce US PBO
                                             * corresponding to a line whose
                                             * attenuation at 1MHz equals this
                                             * number (unit is .1 dB)
                                             * if <0, means no elen forced
                                             * -1: final elen =  max(kl0_CO,kl0_CPE)
                                             * -2: final elen =  min(kl0_CO,kl0_CPE)
                                             * -3: final elen =  kl0_CO
                                             * -4: final elen =  kl0_CPE */
  int16              maxTxPowerDn;          /* Maximum allowed downstream transmit power.
                                             * Valid range is [-25.6,25.6].
                                             * Unit is 0.1 dBm. */
  int16              maxRxPowerUp;          /* Maximum allowed upstream received power.
                                             * Valid range is [-25.6,25.6].
                                             * Unit is 0.1 dBm. */
  uint16             maxUsedTone;           /* When a value different from zero
                                             * is spedified, this parameter
                                             * limits the maximum frequency that
                                             * may be used for both US and DS */
  uint8              vectorFlag;            /* Flag to control vectoring
                                               bit[1]: disable the fix
                                               bandplan. Also disable all
                                               vectoring mode */
  uint8              aeleMode;              /* aeleMode for Amd7 kl0 estimation ([0,3]) */
  uint16             dleThreshold;          /* Threshold in DMT symbols to
                                               declare a DLE event */
  uint8              upboElmt;              /* upboElmt for Amd7 kl0 estimation ([10]) */
  int8               rxThreshDn;            /* rxThreshDs for Amd7 kl0 estimation ([-64,-30]) */
  int8               rxThreshUp;            /* rxThreshUs for Amd7 kl0 estimation ([-128,-15]) */
  int8               dsTargetMarginOffset;    /* 0.1 dB unit. Valid range is [-80:1:79]
                                                 A negative/positive value is used to lower/increase
                                                 the DS targetMargin by the offset */
  uint16             dsTargetMarginSplitTone; /* real tone index (max 8191) */
  uint8              dsTargetMarginBitmap;    /* bit[0]: only apply the margin offset in 35b
                                                 bit[1]: apply the margin offset on low tones (< splitTone) */
  uint8              distortionMinThresholdSNRminusMTPR; /* frontend distortion measurement: minimum required relative distortion free SNR threshold to stop a line */
  uint8              distortionMaxThresholdMTPR;         /* frontend distortion measurement: maximum allowed absolute distortion SNR threshold to stop a line */
  uint8              reservedB[7];
} BCM_PACKING_ATTRIBUTE;

typedef PhysicalLayerConfigVDSL SetLinePhyConfigVDSLMsgReq;
extern  MsgLayout               SetLinePhyConfigVDSLLayout;

typedef PhysicalLayerConfigVDSL GetLinePhyConfigVDSLMsgRep;
extern  MsgLayout               GetLinePhyConfigVDSLLayout;

/* }}} */
/* {{{ Set/GetLinePhyConfigGFAST services */

# define BCM_52KHZ_TS 51.750
# define BCM_GFAST_DEFAULT_BP_START 43
# ifdef BCM_GFAST_212_SUPPORT
#  define BCM_GFAST_DEFAULT_BP_END   4042
# else
#  define BCM_GFAST_DEFAULT_BP_END   2042
# endif

# define BCM_GFAST_NB_PSD_BP 32
typedef struct PsdDescriptorGfast PsdDescriptorGfast;
struct PsdDescriptorGfast
{
  uint8      nrOfBreakPoints;
  uint8      reserved[3];
  BreakPoint bp[BCM_GFAST_NB_PSD_BP];
} BCM_PACKING_ATTRIBUTE;

typedef struct UsPboDescriptorGfast UsPboDescriptorGfast;
struct UsPboDescriptorGfast
{
  uint8      noOfToneGroups; /* 0, no UPBO, 1 active */
  uint8      method; /* reserved */
  PboSetting pbo[1];
} BCM_PACKING_ATTRIBUTE ;

/* GFAST profiles (bitmaps, for configuration purpose) */
# define BCM_GFAST_PROFILE_MASK_106a    (1U<<(BCM_SEL_GFAST_PROFILE_106a -1))
# define BCM_GFAST_PROFILE_MASK_212a    (1U<<(BCM_SEL_GFAST_PROFILE_212a -1))
# define BCM_GFAST_PROFILE_MASK_106b    (1U<<(BCM_SEL_GFAST_PROFILE_106b -1))
# define BCM_GFAST_PROFILE_MASK_106c    (1U<<(BCM_SEL_GFAST_PROFILE_106c -1))
# define BCM_GFAST_PROFILE_MASK_212c    (1U<<(BCM_SEL_GFAST_PROFILE_212c -1))
# define BCM_GFAST_PROFILE_MASK_DEFAULT \
  (BCM_GFAST_PROFILE_MASK_106a |        \
   BCM_GFAST_PROFILE_MASK_106b |        \
   BCM_GFAST_PROFILE_MASK_106c |        \
   BCM_GFAST_PROFILE_MASK_212a |        \
   BCM_GFAST_PROFILE_MASK_212c )

# define BCM_MGFAST_PROFILE_MASK_P424a    (1U<<(BCM_SEL_MGFAST_PROFILE_P424a -1))
# define BCM_MGFAST_PROFILE_MASK_P424d    (1U<<(BCM_SEL_MGFAST_PROFILE_P424d -1))
# define BCM_MGFAST_PROFILE_MASK_Q424a    (1U<<(BCM_SEL_MGFAST_PROFILE_Q424a -1))
# define BCM_MGFAST_PROFILE_MASK_Q424d    (1U<<(BCM_SEL_MGFAST_PROFILE_Q424d -1))
# define BCM_MGFAST_PROFILE_MASK_DEFAULT \
  (BCM_MGFAST_PROFILE_MASK_P424a |       \
   BCM_MGFAST_PROFILE_MASK_P424d |       \
   BCM_MGFAST_PROFILE_MASK_Q424a |       \
   BCM_MGFAST_PROFILE_MASK_Q424d )

# define BCM_GFAST_PROFILE_ANNEX_X_FORBIDDEN   0
# define BCM_GFAST_PROFILE_ANNEX_X_ALLOWED     1
# define BCM_GFAST_PROFILE_ANNEX_X_FORCED      2

# define BCM_GFAST_PROFILE_ANNEX_D_FORBIDDEN   0
# define BCM_GFAST_PROFILE_ANNEX_D_ALLOWED     1
# define BCM_GFAST_PROFILE_ANNEX_D_FORCED      2

typedef struct PhysicalLayerConfigGfast PhysicalLayerConfigGfast;
struct PhysicalLayerConfigGfast
{
  PsdDescriptorGfast limitingMaskDn;
  PsdDescriptorGfast limitingMaskUp;
  ToneGroup          bandPlanDn;
  ToneGroup          bandPlanUp;
  UsPboDescriptorGfast pboPsdUp;
  uint8              enableProfileBitmap;   /* see BCM_GFAST_PROFILE_MASK_xxx */
  uint8              enableProfileBitmapMgfast;
  int16              forceElectricalLength; /* if >=0, enforce US PBO
                                             * corresponding to a line whose
                                             * attenuation at 1MHz equals this
                                             * number (unit is .1 dB)
                                             * if <0, means no elen forced
                                             * -1: final elen =  max(kl0_CO,kl0_CPE)
                                             * -2: final elen =  min(kl0_CO,kl0_CPE)
                                             * -3: final elen =  kl0_CO
                                             * -4: final elen =  kl0_CPE */
  uint8              retrainAllowed;        /* allows a retrain in O-PRM/R-PRM for PSD optimization 
                                               0: No retrain allowed for PSD optimization
                                               1: Retrain allowed after O-PRM/R-PRM for PSD optimization
                                               2: Force retrain after O-PRM/R-PRM (can be used for debugging)
                                               3: Retrain allowed in O-PRM/R-PRM for PSD optimization, but only if CPE supports extended timeouts
                                            */
  uint8              fdxFallBackMgfast;     /* = 1 -> TDD fallback from FDX enabled (based on loop conditions), 2 -> TDD fallback forced (FDXZ->TDDZ or FDXC->TDD) */
  int16              maxRxPowerUp;          /* Maximum allowed upstream received power.
                                             * Valid range is [-25.6,25.6].
                                             * Unit is 0.1 dBm. */
  uint16             maxUsedTone;           /* When a value different from zero
                                             * is specified, this parameter
                                             * limits the maximum frequency that
                                             * may be used for both US and DS */

  int16             initialTgPrime;         /* initial value of Tg1' that the CPE should install, -1 to let the FW decide */
  int8              Sds;                    /* nr of active frames in the DS TDD slot [2..M_DS], -1 means fill all available space */
  int8              Sus;                    /* nr of active frames in the US TDD slot [2..M_US], -1 means fill all available space */
  uint8             soc_redundancy;         /* DS soc repetion rate in ChannelDiscovery 0 = minimal/1 = max (m_ds/4) */
  uint8             reservedB[3];
  LinkConfig        linkConfig[2];          /* US/DS */
  int8              reservedC[4];
  uint8             rmcrLorTrigger[2];      /* Unit is 50 ms, 0 means RMC-R is disabled */
  ReinitConfig      reinitConfig[2];        /* US/DS */
} BCM_PACKING_ATTRIBUTE;


typedef struct FraConfig FraConfig;
struct FraConfig
{
  uint8  FRA_TIME;      /* monitoring time  range 1 - Msf */
  uint8  FRA_NTONES;    /* % degraded tones range 1-100   */
  uint16 FRA_RTX_UC;    /* rtx_uc anomalies range 1-1023  */
  uint8  FRA_VENDISC;   /* enables vendor disc method     */
  uint8  reserved;
}BCM_PACKING_ATTRIBUTE;


typedef struct RmcConfig RmcConfig;
struct RmcConfig
{
  int16  targetMarginRmc; /* range is [0..310 ], unit is 0.1 dB */
  int16  minMarginRmc;    /* used as trigger for RPA. range is [0..310 ], unit is 0.1 dB */
  uint8  maxBi;           /* max the bit loading on RMC tone. range is [2..6] bits */
  uint8  minStartToneRmc; /* encoded as physical tone index divided by 8, range is [0..128] */
} BCM_PACKING_ATTRIBUTE;

typedef struct TrafficConfigGfast TrafficConfigGfast;
struct TrafficConfigGfast
{
  uint32  NDRmax;         /* Max Net Data Rate. Unit is kb/s */
  uint32  ETRmin;         /* Used to abort init if that rate cannot be achieved */
  uint32  GDRmax;         /* Gamma Data Rate. Used to define the valid T_BUDGET range */
  uint32  GDRmin;         /* Gamma Data Rate. Used to define the valid T_BUDGET range */
  uint16  maxDelay;       /* unit is us - range 0-16000 (16ms) */
  uint16  INPmin;         /* unit is 1/2 symbol - range 0-1040 by step of 2 */
  uint8   INPmin_rein;    /* unit is 1/2 symbol - range 0-126 by step of 2 */
  uint8   shineRatio;     /* Assumed fraction of NDR necessary to correct shine - range 0-100(in 0.001 units) */
  uint8   iatRein;        /* 0-3 for [100, 120, 300, 360] Hz */
  uint8   minRSoverhead;  /* minimum R/N. Unit is 1/256th - range 0-64 by step of 8 */
  uint8   rtxTestMode;    /* accelerated MTBE testing */
  uint8   tpsTcTestMode;  /* disables dummy DTU generation */
  uint8   enableIdleSymbol;  /* enable idle symbol generation (need to be enabled to be std compliant) */
  uint8   disableBackpressure; /* bit 0 decouples US backpressure request from US retransmission handling
				* bit 1 disables US flow control bit generation in DS RMC message. */
  uint8   reserved[4];
} BCM_PACKING_ATTRIBUTE;


typedef struct GfastConfig GfastConfig;
struct GfastConfig
{
  PhysicalLayerConfigGfast phyConfig;
  TrafficConfigGfast       trafficConfig[2];
  RmcConfig                rmcConfig[2];
  FraConfig                fraConfig[2];
}BCM_PACKING_ATTRIBUTE;

typedef GfastConfig SetLineGfastConfigMsgReq;
extern  MsgLayout   SetLineGfastConfigLayout;

typedef GfastConfig GetLineGfastConfigMsgRep;
extern  MsgLayout   GetLineGfastConfigLayout;

/* }}} */
/* {{{ Set/GetLineGfastRFIconfig services */

#define BCM_GFAST_NB_RFI_BANDS 32
typedef struct RfiBandsGfast RfiBandsGfast;
struct RfiBandsGfast
{
  uint8  nRfiBands;             /* number of bands sent at -80 dBm/Hz, max = 16 */
  uint8  reserved;
  ToneGroup toneGroups[BCM_GFAST_NB_RFI_BANDS];
} BCM_PACKING_ATTRIBUTE;

#define BCM_GFAST_NB_IAR_BANDS 15
#define MAX_NB_RFI_PLUS_IAR_BANDS_GFAST (BCM_GFAST_NB_RFI_BANDS+BCM_GFAST_NB_IAR_BANDS)
typedef struct RfiAndIarBandsGfast RfiAndIarBandsGfast;
struct RfiAndIarBandsGfast
{
  uint8  nrOfRfiBands;
  uint8  nrOfRfiGaps;
  ToneGroup toneGroups[MAX_NB_RFI_PLUS_IAR_BANDS_GFAST];
} BCM_PACKING_ATTRIBUTE;

#define BCM_GFAST_NB_CAR_MASK 32
typedef struct CarMaskGfast CarMaskGfast;
struct CarMaskGfast
{
  uint8  reserved;
  uint8  nrOfGaps;
  ToneGroup toneGroups[BCM_GFAST_NB_CAR_MASK];
} BCM_PACKING_ATTRIBUTE;

#define BCM_NB_CAR_MASK_RMC 2
typedef struct CarMaskRmc CarMaskRmc;
struct CarMaskRmc
{
  uint8  reserved;
  uint8  nrOfGaps;
  ToneGroup toneGroups[BCM_NB_CAR_MASK_RMC];
} BCM_PACKING_ATTRIBUTE;

typedef struct GfastRFIconfig GfastRFIconfig;
struct GfastRFIconfig
{
  RfiBandsGfast      rfiNotches;
  uint16             iarBands;              /* 15 bits bitmask following coding as in G998.4 table 11.70.8 */
  CarMaskGfast       carMaskDn;
  CarMaskGfast       carMaskUp;
  CarMaskRmc         carMaskRmcDn;
  CarMaskRmc         carMaskRmcUp;
}BCM_PACKING_ATTRIBUTE;

typedef GfastRFIconfig SetLineGfastRFIconfigMsgReq;
extern  MsgLayout   SetLineGfastRFIconfigLayout;

typedef GfastRFIconfig GetLineGfastRFIconfigMsgRep;
extern  MsgLayout   GetLineGfastRFIconfigLayout;

/* }}} */
/* {{{ Set/GetLineTestConfiguration services */

#define LINE_MODE_MASK_992_1_A    (1U<< LINE_SEL_PROT_992_1_A)
#define LINE_MODE_MASK_992_1_B    (1U<< LINE_SEL_PROT_992_1_B)
#define LINE_MODE_MASK_992_1_C    (1U<< LINE_SEL_PROT_992_1_C)
#define LINE_MODE_MASK_992_2_A    (1U<< LINE_SEL_PROT_992_2_A)
#define LINE_MODE_MASK_992_2_C    (1U<< LINE_SEL_PROT_992_2_C)
#define LINE_MODE_MASK_992_1_H    (1U<< LINE_SEL_PROT_992_1_H)
#define LINE_MODE_MASK_992_1_I    (1U<< LINE_SEL_PROT_992_1_I)
#define LINE_MODE_MASK_993_2      (1U<< LINE_SEL_PROT_993_2)
#define LINE_MODE_MASK_993_2_VECT (1U<< LINE_SEL_PROT_993_2_VECT)
#define LINE_MODE_MASK_993_5      (1U<< LINE_SEL_PROT_993_5)
#define LINE_MODE_MASK_9701       (1U<< LINE_SEL_PROT_9701)
#define LINE_MODE_MASK_9711       (1U<< LINE_SEL_PROT_9711)
#define LINE_MODE_MASK_992_3_A    (1U<< LINE_SEL_PROT_992_3_A)
#define LINE_MODE_MASK_992_3_B    (1U<< LINE_SEL_PROT_992_3_B)
#define LINE_MODE_MASK_992_3_I    (1U<< LINE_SEL_PROT_992_3_I)
#define LINE_MODE_MASK_992_3_M    (1U<< LINE_SEL_PROT_992_3_M)
#define LINE_MODE_MASK_992_3_J    (1U<< LINE_SEL_PROT_992_3_J)
#define LINE_MODE_MASK_992_4_I    (1U<< LINE_SEL_PROT_992_4_I)
#define LINE_MODE_MASK_992_3_L1   (1U<< LINE_SEL_PROT_992_3_L1)
#define LINE_MODE_MASK_992_3_L2   (1U<< LINE_SEL_PROT_992_3_L2)
#define LINE_MODE_MASK_992_5_A    (1U<< LINE_SEL_PROT_992_5_A)
#define LINE_MODE_MASK_992_5_B    (1U<< LINE_SEL_PROT_992_5_B)
#define LINE_MODE_MASK_992_5_I    (1U<< LINE_SEL_PROT_992_5_I)
#define LINE_MODE_MASK_992_5_M    (1U<< LINE_SEL_PROT_992_5_M)
#define LINE_MODE_MASK_992_5_J    (1U<< LINE_SEL_PROT_992_5_J)
#define LINE_MODE_MASK_SADSL      (1U<< LINE_SEL_PROT_SADSL)
#define LINE_MODE_MASK_ANSI       (1U<< LINE_SEL_PROT_ANSI)
#define LINE_MODE_MASK_ETSI       (1U<< LINE_SEL_PROT_ETSI)

/* some usefull aggregates */
#define LINE_MODE_MASK_ADSL1  (LINE_MODE_MASK_992_1_A | \
                               LINE_MODE_MASK_992_1_B | \
                               LINE_MODE_MASK_992_1_C | \
                               LINE_MODE_MASK_992_2_A | \
                               LINE_MODE_MASK_992_2_C | \
                               LINE_MODE_MASK_992_1_H | \
                               LINE_MODE_MASK_992_1_I | \
                               LINE_MODE_MASK_ANSI    | \
                               LINE_MODE_MASK_ETSI)
#define LINE_MODE_MASK_ADSL2P (LINE_MODE_MASK_992_5_A  | \
                               LINE_MODE_MASK_992_5_B  | \
                               LINE_MODE_MASK_992_5_I  | \
                               LINE_MODE_MASK_992_5_M  | \
                               LINE_MODE_MASK_992_5_J)
#define LINE_MODE_MASK_ADSL2  (LINE_MODE_MASK_992_3_A  | \
                               LINE_MODE_MASK_992_3_B  | \
                               LINE_MODE_MASK_992_3_I  | \
                               LINE_MODE_MASK_992_3_M  | \
                               LINE_MODE_MASK_992_3_J  | \
                               LINE_MODE_MASK_992_4_I  | \
                               LINE_MODE_MASK_992_3_L1 | \
                               LINE_MODE_MASK_992_3_L2 | \
                               LINE_MODE_MASK_ADSL2P)

#define LINE_MODE_MASK_ADSL   (LINE_MODE_MASK_ADSL1 | \
                               LINE_MODE_MASK_ADSL2)

#define LINE_MODE_MASK_VDSL2  (LINE_MODE_MASK_993_2      | \
                               LINE_MODE_MASK_993_2_VECT | \
                               LINE_MODE_MASK_993_5)

#define LINE_MODE_MASK_GFAST  (LINE_MODE_MASK_9701 | \
                               LINE_MODE_MASK_9711)

#define BCM_PTM_PREEMPT_BIT(dir, bearer) (1U<<((dir)*2))<<(bearer)

typedef struct TestConfigMessage TestConfigMessage;
struct TestConfigMessage
{
  uint32 disableProtocolBitmap; /* Bitmap to select allowed modes (see macros
                                 * LINE_MODE_MASK_XXX here above */
  uint32 reservedA;
  uint8  disableCoding;         /* bit0   : if set, disable trellis coding
                                 * bit1   : disable Reed-Solomon coding
                                 * bit4   : disable ADSLx rx erasure decoding
                                 * bit5   : disable VDSL2 rx erasure decoding
                                 * bit6   : disable VDSL2 rx erasure if more than US0 is used.
                                 * bit7   : disable RS in VDSL fast mode */
  uint8  restartMode;           /* The 4 LSB controls the autonomous line
                                 * state transitions:
                                 * 0 - Default behavior.
                                 * 1 - When the Running Showtime state is
                                 *     left due to the presence of ten
                                 *     consecutive SES, the modem goes to
                                 *     the Idle Configured state instead of
                                 *     going autonomously to the Running
                                 *     Activation state.
                                 * 2 - Do not leave Running Showtime state
                                 *     in case of ten consecutive SES. When
                                 *     this option is active, the only way
                                 *     to leave the Running Showtime state
                                 *     is to send the StopLine message.
                                 * 3 - Stop after performing G.handshake.
                                 * 4 - Stay in Reverb : the modem will stop
                                 *     during the initialization and stay in C-REVERB1
                                 * 5 - Stay in Medley : the modem will stop
                                 *     during the initialization and stay in C-MEDLEY
                                 * Bit 4 disables the activation duty cycle
                                 * that normally takes place during activation
                                 * to save power during CPE activation tone detection.
                                 */
  uint8  disableShalfOption;    /* bit0: disable S=0.5 for ADSL1 modes
                                 * bit1: ignore GSPN CPE reporting of S=0.5
                                 *       support in R-MSG1
                                 * bit2: allow the use of S=1/2 although N<128 */
  uint8  disableExtendedIlv;    /* Bit map with the following meaning:
                                 * bit0: Disable the support of the optional
                                 *       ADSL2+ 24kB interleaver
                                 * bit1: Disable the support of the proprietary
                                 *       ADSL2+ extended US interleaver.
                                 */
  uint8  disableBitSwap;        /* bit0: disable US bitswaps
                                 * bit1: disable DS btiswaps
                                 * bit2: disable extended bitswaps message
                                 *       (use ADSL1 style bitswap messages).
                                 * bit3: force continuous US bitswap.
                                 * bit4: disable VDSL2 US SRA
                                 * bit5: disable VDSL2 DS SRA
                                 * bit7: disable ADSL channel re-acquisition
                                 */
  int8   forceAgc;              /* Force AGC settings in VDSL.  Unit is .5dB
                                 * A value of 0 let the fw select best value. */
  uint8  disableBitRateLimitation; /* VDSL2: Disable rate limitation */
  uint8  disableUS1bitConst;    /* ADSL2: Disable 1 bit constellation in US */
  uint8  disableWindowing;      /* Disable the transmit windowing */
  uint8  disableFarEndErrorCheck; /* Disable exit from showtime because of FE
                                   * errors
                                   * - bit0: ADSL1
                                   * - bit1: ADSL2(+)
                                   * - bit2: VDSL2
								   * - bit3: Gfast
                                   * - bit4: force short OAM timeout
                                   * Disable FE UAS declaration in absence of FE
                                   * counter response
                                   * - bit5: ADSL
                                   * - bit6: VDSL
                                   */
  uint8  reservedB;
  uint8  disableUpdateTestParameters; /* Disable automatic test params update
                                 * (must be done manually) */
  uint8  msgMin[2];             /* Set the MSG min value, unit is kbs
                                 * US/DS for Upstream/Downstream */
  uint16 disableLargeD;         /* bitmap disabling optional high D values */
  uint16 maxL2BitRate[2];       /* Only applied when overruling L2 min/max.
                                 * Index 0 is for B0 */
  uint8  L2L3TestConfig;        /* bit0: disable autonomous L2 entry/exit
                                 * bit1: disable power trim
                                 * bit2: overrule L2 min/max rate calcul
                                 * bit3: enable acceptance of L3 power down requests.
                                 * bit4: disable L2 rate > 4 Mbps against BRCM CPE */
  uint8  L2MinAtpr;             /* Minimum Power Reduction at L2 entry. Valid
                                 * range: [0..15]. Will be clamped at
                                 * L2Atpr.*/
  uint8  forceSegmentation;     /* Force ovh messages with size > 10 to be
                                 * segmented (both request/response) */
  uint8  relaxInpMinCheck;      /* Always accept INPminDS=3 even if INPmin>3 */
  uint8  SRAvalidationControl;  /* bit 0: limit SRA such as to respect the 15 to 20ms PER limit
                                 * bit 1: relax SRA INPmin constraint by 20%
                                 * bit 2: relax SRA maxDelay constraint by 20%
                                 * bit 4-7: expected SNR improvement (in dB)
                                   to allocate US reserve memory
                                 */
  uint8  disableVdslUsPbo;      /* US power back off & spectrum optimisation control:
                                 * bit 0: disable USPBO
                                 * bit 1: disable DS spectrum optimisation
                                 * bit 2: disable US spectrum optimisation
                                 * bit 3: force US0+DS1 only
                                 * bit 4: ignore DS PSD ceiling
                                 * bit 5: don't use 17.6 MHz sampling mode for 8MHz profiles
                                 */
  uint8  dynamicInterleaver[2]; /* VDSL2 only : foces a given interleaver memory
                                 * allocation for US (resp DS) direction:
                                 * 0: dynamic interleaver memory allocation (ie, -ve logic),
                                 *    using set formula irrespective of whether
                                 *   dynamicInterleaver in the other direction is non-zero.
                                 * x in[1..255]: forces to allocate x/256 of the
                                 *               interleaver memory for US (resp DS)*/
  uint8  noWaitAllBondingLines; /* bit0: if set this allows the bonding group to be
                                 *       active as soon as at least one line is in
                                 *       showtime. (non-standard behavior)
                                 * bit1: if set to 1, the CO will continue the handshake procedure
                                 *       regardless of whether the previous bonding handshake transactions
                                 *       had succeeded or not.
                                 * bit 2: if set to 1, disables the autonomous
                                          minDelay/maxRate tuning in internal bonding.
                                 * bit 3: if set to 1, this requests that the FE transmitter
                                          disables its Tx delay equalization feature
                                         (either Tx delay management or Tx TS tweaking).
                                 * bit 4: if set to 1, bonded 17a is preferred over single line 35b against BCM63138
                                 */
  uint8 protocolFallbackControl;/* bit0: disable protocol fallback mechanism
                                 * bit1: enable fallback from G.vector to lower prio protocols like VDSL2 and ADSL2
                                 * bit2: increase ANSI activation time-out
                                 *       to 40 seconds (from 15 seconds).
                                 * bit6: do not allow to by-pass R_ACT_REQ detection during G.hs startup
                                 * bit7: enable fallback from G.fast to lower prio protocols like G.vector/VDSL2 and ADSL2 */
  uint8 validRateExtension;     /* The valid DS rate interval used to validate
                                 * the R-PARAMS message is extended by this
                                 * amount. Allowed range is [0..255] kb/s. */
  uint8 disablePTMpreemption;   /* when set to 0, PTM preemption will be enabled
                                 * and negotiated with the remote end
                                 * bit 0: US/B0
                                 * bit 1: US/B1
                                 * bit 2: DS/BO
                                 * bit 3: DS/B1
                                 * only applies if corresponding bearer enabled. */
  uint8 phyRcontrol[2];         /* index 0 is for US
                                 * bit 0: disable PhyR support in this direction
                                 * bit 1: reserved
                                 * bit 2: enables automoding between PhyR
                                 *        and interleaving
                                 * bit 3: force the use of the delay queue
                                 * bit 4: disable DS RTX to IFEC fallback
                                 */
  uint8 SRAmodeControl;         /* control Upstream SRA mode
                                 * bit 0: upshift shoot for targetMargin iso upShiftNoiseMargin-1
                                 * bit 1: downshift shoot for targetMargin iso downShiftNoiseMargin+1
                                 */
  uint8 profileAutoModing;      /* enable VDSL2 profile automoding
                                 * bit 0: switch to 8b profile if only 8MHz usable BW
                                 *        and configured maxTxPower > 14.5dBm
                                 * bit 1: switch to 12a profile if only 12MHz usable BW
                                 *        and no US0 CPE support in 17a profile
                                 * bit 2: always start in 17a, and only retrain if
                                 *        30a would show better capacity
                                 * bit 3: allows to reinitialize to disable US0
                                 *        when this improves performances
                                 */
  uint8 profileAutoModingExt;   /* VDSL2 profile automoding extension
                                 * bit 0: if set together with bit0 of
                                 *        profileAutoModing, switch to 8b profile is
                                 *        done with preference for DS performance,
                                 *        potentially loosing US2
                                 */
  uint8 AdslVdslAutomodingCfg;  /* = maxTone/16.
                                   If both CO and CPE are not using carriers above (AdslVdslAutomodingCfg*16)
                                   then ADSL will be selected */
  uint8 VdslGfastAutomodingCfg;  /* = maxTone/16 (50KHz tonespacing)
                                   If both CO and CPE are not using carriers above (VdslGfastAutomodingCfg*16)
                                   then VDSL will be selected */
  uint8 VdslGfastAutomodingKl0;  /* kl0 above which the line will retrain in VDSL mode *(in 0.5 dB units). 0 means feature not active */
  uint8 forceLRVDSL;            /* 1=LR short loop,2=LR long loop */
  uint8 disableGfastMonTones;   /* gfast only - monitor tones will be sliced to nearest 4-QAM constellation point iso PRN determined one */
  uint8 disableBandplanReductionGfast; /* GFAST only. No change of bandplan in PRM exchange, all tones are monitoring tones */
  uint8 allowV43;               /* 1 -> allow G.HS session using the V43 toneset */
  uint8 syncSymbolChecking;     /* Bit 1 = 1 means enable LOF defect detection */
  uint8 internalNoiseConfig;    /* set 1: internal noise disabled */
  uint8 clampGi;                /* if set, the maxGi used in US is limited to 1.5dB iso
                                   2.5 dB. */
  int8  kl0Delta;               /* allows to test the UPBO PSD policing functionality:
                                 * kl0Delta will be added to O-UPDATE final kl0 without
                                 * changing internal kl0 state. Unit is 1 dB
                                 * => this will trigger the LineInterrupt warning, flagging
                                 *    a CPE that's not compliant to UPBO requirements */
  uint8 disableGi;              /*
                                  bit[0] is set to 1 to force all Rx gi's to 0dB
                                  bit[4] is set to 1 to force the same analog Tx PGA between G.993.5 CD and TR if 17a profile is selected
                                  bit[5] is set to 1 to force the same analog Tx PGA between G.993.5 CD and TR if 35b profile is selected
                                  bit[7] is set to 1 to force uses of frequency domain handshake
                                */
  uint8 disableLargeDus;        /* If set to one, it disables the support for
                                 * the optional ADSL2 US D values larger than 8 */
# ifndef BCM_CPE
  uint8 forceStrictDelayInpCheck; /* If set to one, forces strict check on
                                   * maxDelay and DS SRA to */
  uint8 hsDetectionThreshold;     /* Linear value.
                                   * Default level 0 corresponds to value 16 corresponding to about 20kft.
                                   * Each factor of 2 increase (6dB) corresponds roughly to 1km reduction in detection reach */
# else
  int8 snrAveraging;
  uint8 disableAnnex;           /* bitmap
                                 * Bit 0 = 1 -> no Annex A masks supported
                                 * Bit 1 = 1 -> no Annex B masks supported
                                 * Bit 2 = 1 -> no Annex C masks supported */
# endif
  uint8  disablePhaseLockNtr;   /* if set, the BCM proprietary high accuracy
                                 * NTR transport is disabled. */
  uint8  attNdrMethod;          /* Attainable net data rate method.
                                    Valid values:
                                    - 0: Basic (legacy) method
                                    - 1: Improved method, uses INP_min and ATTNDR_max_delay_octet as configured
                                    - 2: Improved method, uses INP_min = 0 and ATTNDR_max_delay_octet as configured */
  uint8  attNdrMDOsplit;        /* Percentage of the total itlv memory that is dedicated to the DS improved attndr method.
                                    Valid values:
                                    - [5%-95%] in steps of 1%
                                    - 0%       : only valid for fast mode in the DS
                                    - 100%     : only valid for fast mode in the US */
  uint8  ignoreINPminGdmt;       /* If set, INPmin constraint is ignored in ADSL1 */
  uint8  numSymbolsPtmHunt;      /* if non-zero, this will determine the number of consecutive corrupted symbols that will triggeri transition to PTM HUNT */
  uint8  ginpFlags;              /* Control some G.inp behavior:
                                  * - bit 0: forces TS tweaking to be active  (default active only in bonding with minDelay>0)
                                  * - bit 1: forces TS tweaking to be disabled
                                  * - bit 2: disable PTM out-of-sync transition in case of uncorrectable long impulse leading to Ginp out-of-sync state
                                  */
  uint16 vectoringBlackList;    /* disable G.vector against this BRCM CPE version and earlier
                                 * Format is (majorVersion<<8 | minorVersion<<3 | fixVersion) */
  uint8  reservedC[10];
  uint8  disableGfastOlr;
  uint8  gfastHsCtrl;           /* set to 1 to enable operation against BCM63138 FW prior to 42i */
} BCM_PACKING_ATTRIBUTE ;

typedef TestConfigMessage SetLineTestConfigurationMsgReq;
extern  MsgLayout         SetLineTestConfigurationLayout;

typedef TestConfigMessage GetLineTestConfigurationMsgRep;
extern  MsgLayout         GetLineTestConfigurationLayout;

# ifdef BCM_CPE
#  define BCM_TESTCFG_SNR_AVERAGING(pCfg) ((pCfg)->snrAveraging)
#  define BCM_TESTCFG_DISABLE_ANNEX(pCfg) ((pCfg)->disableAnnex)
# else
/* for mix co/cpe platforms, use CO structure for CPE fields */
#  define BCM_TESTCFG_SNR_AVERAGING(pCfg) ((pCfg)->forceStrictDelayInpCheck)
#  define BCM_TESTCFG_DISABLE_ANNEX(pCfg) ((pCfg)->hsDetectionThreshold)
# endif

/* }}} */
/* {{{ StartLine service */

extern MsgLayout    StartLineLayout;

/* }}} */
/* {{{ StopLine service */

extern MsgLayout    StopLineLayout;

/* }}} */
/* {{{ GetLineCounters service */

typedef struct GfastCounters GfastCounters;
struct GfastCounters
{
  uint32  LANDEFTRS;   /* count of seconds with landeftr defect active
                        * (Low all-NOI data error-free throughput rate) */
  uint32  ANDEFTRmin;  /* Lowest ANDEFTR observed in the current interval */
  uint32  ANDEFTRsum;  /* ANDEFTRacc/2^16 where ANDEFTRacc = sum(ANDEFR*1sec) */
  uint32  ANDEFTRmax;  /* Highest ANDEFTR observed in the current interval */
  uint32  ANDEFTRDS;   /* count of seconds in with ANDEFTR is defined */
} BCM_PACKING_ATTRIBUTE;

typedef struct GinpCounters GinpCounters;
struct GinpCounters
{
  uint32   LEFTRS;      /* Low Error-Free Troughtput Rate Second */
  uint32   minEFTR;     /* Lowest EFTR observed in the current interval */
  uint32   maxEFTR;     /* Highest EFTR observed in the current interval */
  uint32   errFreeBits; /* #bits belonging to correct DTU's leaving the Rx PMS-TC * 2^(-16) */
} BCM_PACKING_ATTRIBUTE;

typedef struct DerivedSecCounters DerivedSecCounters;
struct DerivedSecCounters
{
  uint32   FECS;   /* FEC second */
  uint32   LOSS;   /* LOS second */
  uint32   ES;     /* errored second: CRC-8 error or LOS or SEF (or LPR) */
  uint32   SES;    /* severely errored second : idem as above but CRC-8 > 18 */
  uint32   UAS;    /* unavailable second : more then 10 consecutive SES */
  uint32   AS;     /* available second = elapsed time - UAS_L */
  uint32   LOFS;   /* LOF second */
  uint32   LPRS;   /* LOL second in NE instance, LPR second in FE instance */
  uint32   LORS;   /* LOR seconds - G.fast only */
} BCM_PACKING_ATTRIBUTE ;

typedef struct AdslAnomNeFeCounters AdslAnomNeFeCounters;
struct AdslAnomNeFeCounters
{
  uint32   CV;                     /* CRC-8 count */
  uint32   FEC;                    /* FEC corrections */
  uint32   uncorrectableCodeword;  /* codewords that could not be corrected */
  uint32   rtxCW;                  /* number of codewords retransmitted */
  uint32   rtxCorrectedCW;         /* number of CW corrected through retransmission */
  uint32   rtxUncorrectedCW;       /* number of CW left uncorrected after retransmission */
} BCM_PACKING_ATTRIBUTE ;

typedef struct AdslAnomCounters AdslAnomCounters;
struct AdslAnomCounters
{
  AdslAnomNeFeCounters ne;         /* Near-End anomaly counters */
  AdslAnomNeFeCounters fe;         /* Far-End anomaly counters */
} BCM_PACKING_ATTRIBUTE ;

# define BCM_IDLE_CELLS(count) \
  ((count).totalCells-(count).totalUserCells-(count).hecErrors-(count).overflowCells)

typedef struct AtmPerfNeFeCounters AtmPerfNeFeCounters;
struct AtmPerfNeFeCounters
{
  uint32   totalCells;
  uint32   totalUserCells;
  uint32   hecErrors;
  uint32   overflowCells;
  uint32   idleCellBitErrors;
} BCM_PACKING_ATTRIBUTE ;

typedef struct AtmPerfCounters AtmPerfCounters;
struct AtmPerfCounters
{
  AtmPerfNeFeCounters ne;
  AtmPerfNeFeCounters fe;
} BCM_PACKING_ATTRIBUTE ;

typedef struct PtmPrioPerfCounters PtmPrioPerfCounters;
struct PtmPrioPerfCounters
{
  uint32 ptm_rx_packet_count;      /* number of rx eth packet received */
  uint32 ptm_tx_packet_count;      /* number of tx eth packet sent */
  uint32 TC_CV;                    /* 64/65b code violation */
  uint32 ptm_rx_utopia_overflow;   /* number of rx eth packet that could not
                                    * be passed to upper layer becasue of
                                    * utopia overflow */
  uint32 ptm_rx_crc_error_count;   /* number of rx eth packet with ptm crc */
} BCM_PACKING_ATTRIBUTE ;

typedef struct PtmPerfCounters PtmPerfCounters;
struct PtmPerfCounters
{
  PtmPrioPerfCounters p0;
  PtmPrioPerfCounters p1;
} BCM_PACKING_ATTRIBUTE ;

/* When line is in PTM mode, peformance counters are organized according to
 * PtmPerfCounters, otherwise as AtmPerfCounters */
typedef union HmiPerfCounters HmiPerfCounters;
union HmiPerfCounters
{
  AtmPerfCounters atm;
  PtmPerfCounters ptm;
} ;


# define NE 0
# define FE 1
# define BCM_SIDE_NAME(side) ((side) == NE ? "NE" : "FE")

typedef struct LineCounters LineCounters;
struct LineCounters
{
  DerivedSecCounters derivedCounts[2];     /* index 0 is for NE */
  AdslAnomCounters   adslAnomalyCounts[2]; /* index 0 is for B0 */
  HmiPerfCounters    perfCounts[2];        /* index 0 is for B0 */
  uint16 failedFullInitCount;              /* incremented each time an initialization
                                            * failure is reported in current period while in Full init */
  uint8  fullInitCount;                    /* incremented at each full init attempt */
  uint8  reInitCount;                      /* incremented each time we start a new training
                                            * within 20s after a showtime drop */
  uint16 fastInitCounter;                  /* incremented at each fast init attempt */
  uint16 failedFastInitCounter;            /* incremented each time an initialization
                                            * failure is reported in current period while in Fast init */
  uint16 elapsedTimeSinceLastRead;         /* number of seconds since last read action */
  uint8  suspectFlagNE;                    /* the suspect flag is activated either
                                            * - the first time PM is read after a
                                            *   Santorini reset or line reset.
                                            * - when the elapsed time between two
                                            *   reads is bigger than 1 hour. */
  uint8  suspectFlagFE;                    /* the suspect flag is activated either
                                            * - the first time PM is read after a
                                            *   Santorini reset or line reset.
                                            * - when the elapsed time between two
                                            *   reads is bigger than 1 hour.
                                            * - the FE statistics are not reliable
                                            *   because of a IB were not reliable at
                                            *   some point in time during the last
                                            *   monitoring period.
                                            */
  uint32 upTime;                           /* nr of seconds already in showtime */
  uint32 reserved[3];
  GinpCounters  ginpCounters[2];           /* index 0 is for NE */
  GfastCounters gfastCounters[2];          /* index 0 is for NE */
} BCM_PACKING_ATTRIBUTE ;

typedef LineCounters   GetLineCountersMsgRep;
extern  MsgLayout      GetLineCountersLayout;

/* }}} */
/* {{{ GetLineStatus service */

/* macro to compute actual SEQp (only one lp in ADSL2) */
# define BCM_ACTUAL_SEQ(s, d, b)                                 \
  (BCM_IS_SEL_PROT_VDSL2(s) ? ((s).linkStatus[d].bearerLatencyInfo[b].U * \
                               (s).linkStatus[d].bearerLatencyInfo[b].G): \
   (BCM_IS_SEL_PROT_ADSL2(s) ? (s).linkStatus[d].MSGc + 6 : 68))

/* macro to compute actual Overhead Data Rate (in kbps) */
# define BCM_ACTUAL_OR(s, d, b) \
  (4*(s).linkStatus[d].bearerLatencyInfo[b].M*(s).linkStatus[d].bearerLatencyInfo[b].L* \
   (BCM_IS_SEL_PROT_VDSL2(s) ? (s).linkStatus[d].bearerLatencyInfo[b].G : 1) / \
   ((s).linkStatus[d].bearerLatencyInfo[b].N * (s).linkStatus[d].bearerLatencyInfo[b].T))

/* macro to compute actual Overhead Message Rate */
# define BCM_ACTUAL_MSG(s, d, b) \
  (BCM_ACTUAL_OR(s, d, b) * (BCM_ACTUAL_SEQ(s, d, b) - 6) / BCM_ACTUAL_SEQ(s, d, b))

typedef struct BearerLatencyInfo BearerLatencyInfo;
struct BearerLatencyInfo
{
  /* misc */
  uint8  phyLatencyIndex;       /* indicate on which physicalLatency path this
                                 * bearer is being mapped */
  uint8  INPreport;             /* method used to compute the ActINP: 0 = according to INP_no_erasure formula
                                                                      1 = estimated by the xTU receiver */
  uint16 reserved;

  /* bearer rate reporting */
  uint32 actualBitRate;         /* Net Data Rate = current bit rate */
  uint32 etru;                  /* Unlimited Expected Throughput Rate
                                 * ETRu = actualBitRate - rtxOH */

  /* main framing parameters */
  uint32 L;                     /* Number of bits per symbol */
  uint16 D;                     /* interleaving depth: =1 for fast path */
  uint16 B;                     /* Nominal number of bearer octets per MUX data
                                 * frame.  Index 0 is for Bearer B0 */
  uint8  N;                     /* RS codeword size */
  uint8  R;                     /* RS codeword overhead bytes */
  uint8  I;                     /* VDSL only : Interleaver block length */
  uint8  M;                     /* VDSL : Number of MUX data frames per FEC data
                                 * frame (codeword) 
                                 * Gfast : number of symbols per TDD frame in this direction. */
  uint8  T;                     /* Number of MUX data frames per sync octet */
  uint8  G;                     /* Number of overhead octets in superframe */
  uint16 U;                     /* Number of overhead subframes in superframe */

  /* RTX specific framing settings */
  uint8  Q;                     /* G.Inp / G.fast : Number of RS CW per DTU */
  uint8  V;                     /* G.Inp: Number of padding bytes per DTU */
                                /* In VDSL2 interleaving: Number of overhead frames in superframe */
  uint8  Nret;                  /* Maximum number of retransmission
                                 * Nret = 0 means ARQ disabled on this bearer
                                 * rtxRxQueue = Nret * rtxTxQueue */
  uint8  ginpFraming;           /* 3 LSB's: 0 if G.inp is not active, 1 or 2 (framingType)
                                              if active (W = (ginpFraming&0x3)-1)
                                 * 5 MSB's: lb value (lb)
                                 */
  uint8  rtxTxQueue;            /* length of the retransmission queue in Tx
                                 * direction (e.g. in US it is the CPE queue length)
                                 */
  uint8  phyRrrcBits;           /* number of bits allocated to phyR
                                 * retransmission requests. These are used to request
                                 * retransmission in the reverse direction */
  uint16 rtxRxQueue;            /* length of the retransmission queue in Rx direction */
  uint8  reservedB[2];
  /* bearer properties */
  uint16 INP;                   /* actual INP guaranteed on this latency
                                 * path (1/2 symbol) */
  uint8  tpsTcOptions;          /* tpstc options resulting from negotiation.
                                 * bit 0: PTM preemption enabled
                                 * bit 7: nitro enabled (not reported yet)
                                 */
  uint8  INPrein;               /* actual INP guaranteed on this lateency path
                                 * against rein noise. */
  uint8  delay;                 /* actual delay incurred on this latency path (ms) */
  uint8  codingType;            /* codingType used for this latency
                                 * 0: No coding
                                 * 1: Trellis coding
                                 * 2: Reed-Solomon coding
                                 * 3: Concatenated (trellis and RS) coding
                                 */

  /* capabilities on this bearer */
  uint32 maxMemory;             /* Maximum available nominal memory for the current lp */
  uint32 Lmax;                  /* Maximum possible L (SRA upshift) */
  uint32 Lmin;                  /* Minimum possible L (SRA downshift) */

  /* Gfast specific */
  uint32 gdr;                   /* Gamma data rate (accounts for Tbudget and actMmax) */
  uint32 Ldr;                   /* Number of bearer bits per symbol during RMC symbol */
  uint8  dtaMmax;               /* Max Mus/Mds configured for DTA */
  uint8  actMmax;               /* Max Mus/Mds allowed on this line, accounting for dtaETRMin
                                 * in the reversed direction. */
  uint8  reservedD[2];
  uint32 idr;                   /* Instantaneous data rate (= NDR computed for the current M) */
} BCM_PACKING_ATTRIBUTE ;

#ifndef SUPPORT_HMI
# define LP0 0
# define LP1 1
#endif

# define LINE_STATUS_FAILURE_UAS               (1U<< 1)
# define LINE_STATUS_FAILURE_NO_SHOWTIME       (1U<< 2)
# define LINE_STATUS_FAILURE_ESE               (1U<< 3) /* SDK only, unused by FW */
# define LINE_STATUS_FAILURE_LOR               (1U<< 4)
# define LINE_STATUS_FAILURE_LCD               (1U<< 6)
# define LINE_STATUS_FAILURE_LOS               (1U<<12)
# define LINE_STATUS_FAILURE_LOF               (1U<<13)
# define LINE_STATUS_FAILURE_NCD               (1U<<14)
# define LINE_STATUS_FAILURE_DGL               (1U<<16)
# define LINE_STATUS_FAILURE_OHP               (1U<<17)
# define LINE_STATUS_FAILURE_LPR               (1U<<18)
# define LINE_STATUS_FAILURE_PERSISTENT_LOS    (1U<<19)
# define LINE_STATUS_FAILURE_XTALK_INIT        (1U<<20)
# define LINE_STATUS_FAILURE_RTX_INIT          (1U<<21)
# define LINE_STATUS_FAILURE_PROFILE_ERROR     (1U<<23) /* SDK only, unused by FW */
# define LINE_STATUS_FAILURE_CONFIG_ERROR      (1U<<24)
# define LINE_STATUS_FAILURE_NOT_FEASIBLE      (1U<<25)
# define LINE_STATUS_FAILURE_COMM_PROBLEM      (1U<<26)
# define LINE_STATUS_FAILURE_NO_PEER_DETECTED  (1U<<27)
# define LINE_STATUS_FAILURE_LOM               (1U<<28)
# define LINE_STATUS_FAILURE_L3_CPE            (1U<<29)
# define LINE_STATUS_FAILURE_DLE               (1U<<31)

typedef struct LinkStatus LinkStatus;
struct LinkStatus
{
  int16  snrMargin;
  int16  outputPower;
  int16  refPsd;                /* Adsl2 only: refPsd = nominal PSD - power cutback */
  uint16 electricalLength;      /* kl0: attenuation at 1 MHz */
  uint32 actualLineBitRate;     /* ATM net data rate of all bearers plus all
                                 * overhead information */
  uint32 attainableBitRate;     /* maximum bit rate on the line */
  uint32 attEtr;                /* Attainable ETR corresponding to the attainable NDR reported above */
  uint32 eftr;                  /* Error-Free Throughput Rate (G998.4 only) */
  uint32 aggAchNDR;             /* Aggregate Achievable NDR (G998.4 only):
                                 * maximum aggregate rate achievable by the
                                 * transceiver */
  uint8  attNDRmethod;          /* Attainable NDR computation method */
  uint8  attNDRactDelay;        /* Actual delay used in the computation of ATTNDR */
  uint8  attNDRactInp;          /* Actual INP used in the computation of ATTNDR */
  uint8  attNDRactInpRein;      /* Actual INP against REIN noise used in the
                                 * computation of ATTNDR */
  uint8  MSGc;                  /* nb octets in message-based portion of the
                                 * overhead framing structure (ADSL2 only) */
  uint8  MSGlp;                 /* ID of latency path carrying the
                                 * message-based overhead */
  uint8  snrMode;               /* actual SNR mode in use:
                                 * 1: external noise only
                                 * 2: TxRefVN /
                                 * 3: RxRefVN */
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
  int16  snrMarginROC;
  uint16 rateLossROC;           /* Rate loss due to ROC */
  uint32 attainableGDR;         /* Gamma Data Rate */
  uint32 andEftr;               /* All-NOI Data Error Free Throughput Rate */
  uint16 autoSensingMetric;     /* metric used in GFAST->VDSL automoding, 0xffff if NA */
  int16  todNs;                 /* for US = TimeSync timestaps delta T4-T3, DS = T2-T1. Only valid for GFAST, and only if timingStatus = 0  */
  uint8  reserved1[4];
  uint8  supportedMinMds;       /* US: communicated FTUR_DTA_min_Mds value. Reports zero if annexD is not enabled. DS: min Mds supported by the CO */
  uint8  ttr;                   /* NOI duration */
  uint8  ta;                    /* NOI-DOI gap */
  uint8  tBudget;               /* max allowed transmission time */
  BearerLatencyInfo bearerLatencyInfo[2]; /* Index 0 is for B0  */
} BCM_PACKING_ATTRIBUTE ;

typedef enum {US0=0, US1, US2, US3, US4, DS1, DS2, DS3, DS4} BandIndexes;
typedef struct PerBandStatus PerBandStatus;
struct PerBandStatus {
  int16 LATN[10];               /* US0/US1/US2/US3/US4/DS1/DS2/DS3/DS4/reserved
                                   - in case of ADSL, only US0 and DS1 is used */
  int16 SATN[10];               /* US0/US1/US2/US3/US4/DS1/DS2/DS3/DS4/reserved
                                   - in case of ADSL, only US0 and DS1 is used*/
  int16 SNRM[10];               /* US0/US1/US2/US3/US4/DS1/DS2/DS3/DS4/reserved
                                   - in case of ADSL, only US0 and DS1 is used */
  int16 TXPWR[10];              /* US0/US1/US2/US3/US4/DS1/DS2/DS3/DS4/reserved
                                   - in case of ADSL, only US0 and DS1 is used */
  int16 ELE[10];                /* US0/US1/US2/US3/US4/DS1/DS2/DS3/DS4/reserved
                                   - not relevant in case of ADSL */
} BCM_PACKING_ATTRIBUTE ;


typedef struct FailureLog FailureLog;
struct FailureLog
{
  uint32 failures[2]; /* same bitmap as currentFailure NE/FE */
} BCM_PACKING_ATTRIBUTE ;


typedef struct DefectLog DefectLog;
struct DefectLog
{
  uint32 defect[2]; /* exact bitmap need to be defined */
} BCM_PACKING_ATTRIBUTE ;

# define EVENT_TIME_LOG_LENGTH 40

typedef struct LastRetrainInfo LastRetrainInfo;
struct LastRetrainInfo
{
  DefectLog firstNeDefect; /* During Showtime,
                           * firstNeDefect is the log of
                           * {currentDefect[upTime][NE+FE], upTime} when
                           * ((currentDefect[upTime-1][NE]&DEFECTS_LEADING_TO_SES) == 0)
                           *    && ((currentDefect[upTime][NE]&DEFECTS_LEADING_TO_SES) != 0)
                           */
  DefectLog firstFeDefect; /* During Showtime,
                           * firstFeDefect is the log of
                           * {currentDefect[upTime][NE+FE], upTime} when
                           * ((currentDefect[upTime-1][FE]&DEFECTS_LEADING_TO_SES) == 0)
                           *    && ((currentDefect[upTime][FE]&DEFECTS_LEADING_TO_SES) != 0)
                           */
  FailureLog firstNeFailure; /* During Showtime,
                           * firstNeFailure is the log of
                           * {currentFailure[upTime][NE+FE], upTime} when
                           * ((currentFailure[upTime-1][NE]&FAILURES_LEADING_TO_SES) == 0)
                           *    && ((currentFailure[upTime][NE]&FAILURES_LEADING_TO_SES) != 0)
                           */
  FailureLog firstFeFailure; /* During Showtime,
                           * firstFeFailure is the log of
                           * {currentFailure[upTime][NE+FE], upTime} when
                           * ((currentFailure[upTime-1][FE]&FAILURES_LEADING_TO_SES) == 0)
                           *    && ((currentFailure[upTime][FE]&FAILURES_LEADING_TO_SES) != 0)
                           */

  FailureLog ShowtimeExitFailureLog; /* a log of currentFailure[upTime][NE+FE] ,
                                      * upTime when the modem quit Showtime
                                      */
  uint32  uptime;
  uint8   eventTimeLog[EVENT_TIME_LOG_LENGTH]; /* log showtime events
                             *  0 = Record time of the firstNeDefect above
                             *  1 = Record time of the firstFeDefect above
                             *  2 = Record time of the firstNeFailure above
                             *  3 = Record time of the firstFeFailure above
                             *  4 = Successful counter mgmt read response received
                             *  5 = Successful test parameter read response received
                             *  6 = Successful overhead message received (request or response)
                             *  7 = Successful DS bitswap applied
                             *  8 = Rejected DS bitswap
                             *  9 = Successful US bitswap applied
                             * 10 = Failed US bitswap transaction
                             * 11 = Successful L0 ->L2 transition
                             * 12 = Successful L2 power trim applied
                             * 13 = Successful L2 ->L0 transition
                             * 14 = US margin observed below minMargin
                             * 15 = DS margin observed below minMargin
                             * 16 = Channel re-acquisition performed
                             * 17 = Successful DS SRA applied
                             * 18 = Rejected DS SRA
                             * 19 = Successful US SRA applied
                             * 20 = Failed US SRA transaction
                             * 21 = INM response received
                             * 22 = Vector response
                             * 23 = Successful DS SOS/FRA applied
                             * 24 = Rejected DS SOS/FRA
                             * 25 = Successful US SOS/FRA applied
                             * 26 = Failed US SOS/FRA transaction
                             * 27 = Tx turned off by a DLE
                             * 28 = Tx turned on by a DLE
                             * 29 = Successful DS TIGA applied
                             * 30 = Successful DS RPA applied
                             * 31 = Rejected DS RPA
                             * 32 = Successful US RPA applied
                             * 33 = Failed US RPA
                             * 34 = TX sync symbol turned off by a DLE
                             * 35 = TX sync symbol turned on by a DLE
                             */

  uint8   lastTransmittedState; /* ADSL: - Bits [5:0] last successful transmitted state
                                 *         (from 1 for G.hs to 32 to Showtime).
                                 *       - Bit[7] retrain controller state: set if the current
                                 *         initialization follows a probing initialization and
                                 *         is intended for PSD and/or power optimization.
                                 * VDSL:   Last successfully transmitted/received SOC message ID.
                                 */
  uint8   lastRetrainCause;     /* bits[3:0] give the cause of the last init */
                                /* failure, according to G.997.1.
                                 * Possible values are:
                                 * 1 - wrong profile (Configuration Error)
                                 * 2 - Configuration Not Feasible
                                 *     (requested bit rate cannot be achieved)
                                 * 3 - Communication Problem (CRC or time-out
                                 *     error)
                                 * 4 - no ATU-R detected
                                 * 5 - training started on crosstalk
                                 * 6 - forced retransmission error
                                 * 7 - Gfast leaving showtime because of L3 
                                 * 8 - retrain because persistent LOR in Gfast
                                 * bits[7:4] give extra information
                                 * - in case of a Communication Problem:
                                 *   0 - timeout during initialisation
                                 *   1 - CRC error during initialisation message
                                 *   2 - Unavailable Time decleared during
                                 *       showtime at NE
                                 *   3 - Dynamic Rate Adaptation triggered
                                 *       retrain at NE
                                 *   4 - Unavailable Time decleared during
                                 *       showtime at FE
                                 *   5 - Dynamic Rate Adaptation triggered
                                 *       retrain at FE
                                 *   6 - Far-End line counters unavailable (ADSL2)
                                 *   7 - Near-end persistent LCD or NCD
                                 *   8 - Far-end persistent LCD or NCD
                                 *   9 - No common operation mode found (e.g. protocol,
                                 *       VDSL profile, ...)
                                 *   A - No common TPS-TC mode found (ATM/PTM mismatch)
                                 *   B - Forced Retransmission Failure in US (G.inp
                                 *       is mandated, but not supported by the remote).
                                 *   C - Forced Retransmission Failure in DS (G.inp
                                 *       is mandated, but not supported by the remote).
                                 *   D - Not allowed vector CPE type connected
                                 * - in case of a Configuration Not Feasible:
                                 *   0 - Upstream Configuration Not Feasible
                                 *   1 - Downstream Configuration Not Feasible
                                 *   2 - G.inp upstream maxDelay < roundtrip
                                 *   3 - G.inp upstream maxDelay < roundtrip
                                 * - in case of a Forced RTX Error:
                                 *   0 - RTX not supported in US
                                 *   1 - RTX not supported in DS
                                 * Special values:
                                 * 0xFE : retrain because of an internal exception or VCE-triggered line reset
                                 * 0xFF : retrain attempted to work around an interop problem
                                 */

  uint8   lastParamValidationFailure; /* this field contains the reason why
                                         the framing parameters from
                                         the CPE have been refused. */
  uint8   reserved;
} BCM_PACKING_ATTRIBUTE ;

# define LINE_TRAFFIC_ATM        BCM_TPS_TC_ATM
# define LINE_TRAFFIC_PTM        (BCM_TPS_TC_PTM | BCM_TPS_TC_PTM_GFAST)
# define LINE_TRAFFIC_INACTIVE   1

# define LINE_STATUS_IDL_NCON    0
# define LINE_STATUS_IDL_CONF    1
# define LINE_STATUS_RUN_ACTI    2
# define LINE_STATUS_RUN_INIT    3
# define LINE_STATUS_RUN_SHOW    4
# define LINE_STATUS_TST_MODE    5
# define LINE_STATUS_RUN_LD_INIT 6
# define LINE_STATUS_IDL_LD_DONE 7
# define LINE_STATUS_RUN_SHOW_L2 8

const char* getLineStatusName(uint8 status);
# define BCM_LINE_STATE_NAME getLineStatusName

# define BCM_LINE_INIT(state) \
  ((state) == LINE_STATUS_RUN_ACTI || (state) == LINE_STATUS_RUN_INIT)
# define BCM_LINE_SHOWTIME(state) \
  ((state) == LINE_STATUS_RUN_SHOW || (state) == LINE_STATUS_RUN_SHOW_L2)
# define BCM_LINE_RUNNING(state) \
   (BCM_LINE_INIT(state) || BCM_LINE_SHOWTIME(state))
# define BCM_LINE_LOW_POWER(status) \
  ((status).state == LINE_STATUS_IDL_NCON && (status).subState == 255)

# define LINE_SEL_PROT_NONE      -1
# define LINE_SEL_PROT_992_1_A    0
# define LINE_SEL_PROT_992_1_B    1
# define LINE_SEL_PROT_992_1_C    2
# define LINE_SEL_PROT_992_2_A    3
# define LINE_SEL_PROT_992_2_C    4
# define LINE_SEL_PROT_992_1_H    5
# define LINE_SEL_PROT_992_1_I    6
# define LINE_SEL_PROT_993_2      7
# define LINE_SEL_PROT_993_2_VECT 8
# define LINE_SEL_PROT_993_5      9
# define LINE_SEL_PROT_9701      11
# define LINE_SEL_PROT_9711      12
# define LINE_SEL_PROT_992_3_A   16
# define LINE_SEL_PROT_992_3_B   17
# define LINE_SEL_PROT_992_3_I   18
# define LINE_SEL_PROT_992_3_M   19
# define LINE_SEL_PROT_992_3_J   20
# define LINE_SEL_PROT_992_4_I   21
# define LINE_SEL_PROT_992_3_L1  22
# define LINE_SEL_PROT_992_3_L2  23
# define LINE_SEL_PROT_992_5_A   24
# define LINE_SEL_PROT_992_5_B   25
# define LINE_SEL_PROT_992_5_I   26
# define LINE_SEL_PROT_992_5_M   27
# define LINE_SEL_PROT_992_5_J   28
# define LINE_SEL_PROT_SADSL     29
# define LINE_SEL_PROT_ANSI      30
# define LINE_SEL_PROT_ETSI      31

# define BCM_SEL_PROTOCOL_NAME getProtocolName
const char* getProtocolName(int8 protocol);

# define BCM_IS_SEL_PROT_ADSL2_MAIN(lineStatus) \
  ((lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_A || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_B || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_M || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_J)
# define BCM_IS_SEL_PROT_ADSL2_PLUS(lineStatus) \
  ((lineStatus).selectedProtocol == LINE_SEL_PROT_992_5_A || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_5_B || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_5_M || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_5_J)
# define BCM_IS_SEL_PROT_ADSL2_REACH(lineStatus) \
  ((lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_L1 || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_L2)
# define BCM_IS_SEL_PROT_VDSL2(lineStatus) \
  ((lineStatus).selectedProtocol == LINE_SEL_PROT_993_2 || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_993_2_VECT || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_993_5)
# define BCM_IS_SEL_PROT_GFAST(lineStatus) \
  ((lineStatus).selectedProtocol == LINE_SEL_PROT_9701 || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_9711)
# define BCM_IS_SEL_PROT_MGFAST(lineStatus) \
  ((lineStatus).selectedProtocol == LINE_SEL_PROT_9711)


# define BCM_IS_SEL_PROT_ADSL2(lineStatus) \
   (BCM_IS_SEL_PROT_ADSL2_MAIN(lineStatus) || \
    BCM_IS_SEL_PROT_ADSL2_PLUS(lineStatus) || \
    BCM_IS_SEL_PROT_ADSL2_REACH(lineStatus))

# define BCM_IS_SEL_PROT_XDSL2(lineStatus) \
   (BCM_IS_SEL_PROT_ADSL2(lineStatus) || \
    BCM_IS_SEL_PROT_VDSL2(lineStatus))

# define BCM_IS_SEL_PROT_DOUBLE_DS BCM_IS_SEL_PROT_ADSL2_PLUS

# define BCM_IS_SEL_PROT_DOUBLE_US(lineStatus) \
  ((lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_M || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_J || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_5_M || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_5_J || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_SADSL   )

# define BCM_IS_SEL_PROT_ANNEX_B(lineStatus) \
  ((lineStatus).selectedProtocol == LINE_SEL_PROT_ETSI    || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_1_B || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_B || \
   (lineStatus).selectedProtocol == LINE_SEL_PROT_992_5_B)

# define BCM_LAST_CARRIER_US(lineStatus)                           \
  (BCM_IS_SEL_PROT_VDSL2(lineStatus)                               \
   || BCM_IS_SEL_PROT_GFAST(lineStatus)) ? BCM_NB_OAM_TG :         \
   (BCM_IS_SEL_PROT_DOUBLE_US(lineStatus) ||                       \
    BCM_IS_SEL_PROT_ANNEX_B(lineStatus)) ? BCM_NB_ADSL_TONE_US :   \
   ((lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_L1 ? 24 : \
   ((lineStatus).selectedProtocol == LINE_SEL_PROT_992_3_L2 ? 16 : \
    BCM_NB_ADSL_TONE_US/2)))

# define BCM_LAST_CARRIER_DS(lineStatus)                           \
  (BCM_IS_SEL_PROT_VDSL2(lineStatus)                               \
   || BCM_IS_SEL_PROT_GFAST(lineStatus) ? BCM_NB_OAM_TG :          \
   (BCM_IS_SEL_PROT_DOUBLE_DS(lineStatus) ?                        \
    BCM_NB_ADSL_TONE_DS : BCM_NB_ADSL_TONE_DS/2))

# define BCM_SEL_VDSL2_PROFILE_8A      1
# define BCM_SEL_VDSL2_PROFILE_8B      2
# define BCM_SEL_VDSL2_PROFILE_8C      3
# define BCM_SEL_VDSL2_PROFILE_8D      4
# define BCM_SEL_VDSL2_PROFILE_12A     5
# define BCM_SEL_VDSL2_PROFILE_12B     6
# define BCM_SEL_VDSL2_PROFILE_17A     7
# define BCM_SEL_VDSL2_PROFILE_30A     8
# define BCM_SEL_VDSL2_PROFILE_35B     9

/* Convert profile ID as returned by GetLineStatus into readable format */
const char* getVdslProfileName(uint8 profile);

# ifndef BCM_SEL_VDSL2_PROFILE
#  define BCM_SEL_VDSL2_PROFILE getVdslProfileName
# endif

# define BCM_SEL_GFAST_PROFILE_106a      1
# define BCM_SEL_GFAST_PROFILE_212a      2
# define BCM_SEL_GFAST_PROFILE_106b      3
# define BCM_SEL_GFAST_PROFILE_106c      4
# define BCM_SEL_GFAST_PROFILE_212c      5

# define BCM_SEL_MGFAST_PROFILE_P424a    1
# define BCM_SEL_MGFAST_PROFILE_P424d    2
# define BCM_SEL_MGFAST_PROFILE_Q424a    3
# define BCM_SEL_MGFAST_PROFILE_Q424d    4

# define BCM_SEL_MGFAST_PDX_FRAMING_MODE_FDXC    0
# define BCM_SEL_MGFAST_PDX_FRAMING_MODE_TDD     1
# define BCM_SEL_MGFAST_PDX_FRAMING_MODE_FDXZ    2
# define BCM_SEL_MGFAST_PDX_FRAMING_MODE_TDDZ    3

/* Convert profile ID as returned by GetLineStatus into readable format */
const char* getGfastProfileName(uint8 profile);
const char* getMGfastProfileName(uint8 profile);
# ifndef BCM_SEL_GFAST_PROFILE
#  define BCM_SEL_GFAST_PROFILE getGfastProfileName
# endif
# ifndef BCM_SEL_MGFAST_PROFILE
#  define BCM_SEL_MGFAST_PROFILE getMGfastProfileName
# endif

const char* getLongReachActMode(uint8 lrActMode);
# ifndef BCM_LR_MODE_EXPAND
#  define BCM_LR_MODE_EXPAND getLongReachActMode
# endif

/* NE (RX) Link is US at CO, but DS at CPE */
# define BCM_DIR(lnk, pDev) \
  ((lnk) == (BCM_DEV_IS_RUNNING_CPE_FW(pDev)? FE : NE)? US : DS)
# define BCM_LINK(dir, pDev) \
  ((dir) == (BCM_DEV_IS_RUNNING_CPE_FW(pDev)? DS : US)? NE : FE)

/* get dir name from link */
# define BCM_LINK_NAME(lnk, pDev) BCM_DIR_NAME(BCM_DIR(lnk, pDev))

typedef enum {
  LDPM_IDLE = 0,
  LDPM_LISTEN,
  LDPM_ACTIVATE,                 
  LDPM_MAX_TONE_256,
  LDPM_MAX_TONE_512,
  LDPM_MAX_TONE_1024,
  LDPM_MAX_TONE_2048,
  LDPM_MAX_TONE_4096,
  LDPM_MAX_TONE_8192,
  LDPM_GFAST_TX_50,
  LDPM_GFAST_RX_50,
  LDPM_GFAST_TX_100,
  LDPM_GFAST_RX_100,
  LDPM_GFAST_TX_200,
  LDPM_GFAST_RX_200,
  LDPM_TEST_VDSL,
  LDPM_TEST_ADSL,
  LDPM_REDUCED_POWER,
  LDPM_FORCED_MODE
} LineDriverPowerState;
const char* getLineDriverPowerStateName(uint8 state);

typedef struct LineStatus LineStatus;
struct LineStatus
{
  uint8   state;                /* Actual state of a line */
  uint8   subState;             /* set to one when in showtime and traffic is enabled */
  uint8   vectoringState;       /* Actual vectoring state of the line */
  int8    selectedProtocol;     /* Actual selected protocol */
  uint8   vdslProfile;          /* selected VDSL2 profile (see BCM_SEL_VDSL2_PROFILE_xx) */
  uint8   loopbackMode;         /* Actual loopback mode */
  uint8   us0Mask;              /* Actual US0 PSD mask used. The coding used
                                 * is the same as for the field in the
                                 * annexUs0 field (in PsdDescriptorUp). */
  uint8   farendDetected;       /* A handshake message has been received from the farend */
  int16   actualElectricalLength; /* Final kl0 applied for upbo */
  uint8   tpsTcType[2];         /* TPS-TC type.  Index 0 is for bearer B0.
                                 * Possible values:
                                 * 1 - Inactive bearer
                                 * 2 - PTM (VDSL2 only)
                                 * 4 - ATM */
  int8    timingStatus;         /* NTR/ToD link status
                                 *  -2: no signal on 8KHz NTR pin of DSP
                                 *  -1: default = no info
                                 *   0: amd7 freq and time sync reached
                                 * 1-4: progress as in table 11-34.4/G993.2
                                 * All other values are reserved */
  uint8   lineDriverPowerState; /* see LineDriverPowerState enum type */
  uint8   lr_actMode;           /* 0=no LR, 1=LR short, 2=LR medium, 3=LR long  */
  uint8   annexXenabled;        /* DTA annex X negotiated */
  uint8   iDtaEnabled;          /* iDTA enabled */
  uint8   annexDenabled;        /* DTA annex D negotiated */
  uint8   cDtaEnabled;          /* cDTA enabled (CDTA_PROC_ACT) */
  uint8   qlnLbFailure;         /* VDSL self-diagnostic failure at the start of training phase. */
  uint8   afeSnr;               /* G.vector self-diagnostic during O-P-VECTOR1: measured SNR
				 * (with the noise measured without DS signal present) [dB/4] */
  uint8   afeMtpr;              /* G.vector self-diagnostic during O-P-VECTOR1: measured MTPR [dB/4] */
  uint8   actPdxFramingMode;    /* 997.3 MGFAST framing mode (0=FDXC, 1=TDD, 2=FDXZ, 3=TDDZ) */ 
  uint8   reserved[1];
  PerBandStatus perBandStatus;
  LastRetrainInfo lastRetrainInfo;
  uint8   gfastProfile;         /* report BCM_SEL_GFAST_PROFILE_xx-1 */
  uint8   hsMus;                /* handshake Mus */
  uint8   hsMds;                /* handshake Mds */
  uint8   reservedB[1];

  LinkStatus linkStatus[2];     /* Index 0 is for RX (=NE) path -- US at CO, DS at CPE */
} BCM_PACKING_ATTRIBUTE ;

typedef struct GetLineStatusReq GetLineStatusReq;
struct GetLineStatusReq
{
  uint32 statusControl;         /* bit 0: report local NE G.INP test parameters
                                 * bit 1: report local FE G.INP test parameters
                                 * bit 2: report US ATTNDR including all traffic constraints (INP, delay)
                                 * bit 3: postprocess DS ATTNDR including all traffic constraints
                                 * bit 4: debug feature reporting the last DRA traffic monitoring results
                                 *        in eftr (observed traffic) and andEftr (observed capacity)
                                 */

} BCM_PACKING_ATTRIBUTE ;

typedef GetLineStatusReq    GetLineStatusMsgReq;
typedef LineStatus          GetLineStatusMsgRep;
extern  MsgLayout           GetLineStatusLayout;

/* }}} */
/* {{{ GetLineFeatures service */

# define BCM_FEAT_OPT_ADSL1_NTR              (1U<<11)
# define BCM_FEAT_OPT_ADSL1_DUAL_LAT_US      (1U<<12)
# define BCM_FEAT_OPT_ADSL1_DUAL_LAT_DS      (1U<<13)
# define BCM_FEAT_OPT_ADSL1_SMALL_S          (1U<<14)
# define BCM_FEAT_OPT_ADSL1_OVERLAP_FREQ     (1U<<16)
# define BCM_FEAT_OPT_ADSL1_TRELLIS          (1U<<17)
# define BCM_FEAT_OPT_ADSL1_NITRO            (1U<<18)

# define BCM_FEAT_OPT_XDSL2_NTR              (1U<<16)
# define BCM_FEAT_OPT_XDSL2_DUAL_LAT_US      (1U<<17)
# define BCM_FEAT_OPT_XDSL2_DUAL_LAT_DS      (1U<<18)
# define BCM_FEAT_OPT_XDSL2_TRELLIS          (1U<<19)
# define BCM_FEAT_OPT_XDSL2_DELT             (1U<<20)
# define BCM_FEAT_OPT_XDSL2_NITRO            (1U<<21)
# define BCM_FEAT_OPT_XDSL2_PTM_ADSL         (1U<<22)
# define BCM_FEAT_OPT_XDSL2_PTM_PREEMPT_US   (1U<<23)
# define BCM_FEAT_OPT_XDSL2_PTM_PREEMPT_DS   (1U<<24)
# define BCM_FEAT_OPT_XDSL2_VIRTUAL_NOISE    (1U<<25)
# define BCM_FEAT_OPT_XDSL2_INM_INPEQ_FORMAT (1U<<27)

# define BCM_FEAT_OPT_VDSL2_US0_EU32         (1U<<8)
# define BCM_FEAT_OPT_VDSL2_US0_EU36         (1U<<9)
# define BCM_FEAT_OPT_VDSL2_US0_EU40         (1U<<10)
# define BCM_FEAT_OPT_VDSL2_US0_EU44         (1U<<11)
# define BCM_FEAT_OPT_VDSL2_US0_EU48         (1U<<12)
# define BCM_FEAT_OPT_VDSL2_US0_EU52         (1U<<13)
# define BCM_FEAT_OPT_VDSL2_US0_EU56         (1U<<14)
# define BCM_FEAT_OPT_VDSL2_US0_EU60         (1U<<15)
# define BCM_FEAT_OPT_VDSL2_US0_EU64         (1U<<16)
# define BCM_FEAT_OPT_VDSL2_US0_IN_12B       (1U<<17)
# define BCM_FEAT_OPT_VDSL2_US0_IN_17A       (1U<<18)
# define BCM_FEAT_OPT_VDSL2_FEXT_UPBO        (1U<<22)
# define BCM_FEAT_OPT_VDSL2_FDPS             (1U<<26)
# define BCM_FEAT_OPT_VDSL2_35B              (1U<<27)
# define BCM_FEAT_OPT_VDSL2_LRVDSL           (1U<<29)
# define BCM_FEAT_OPT_VDSL2_SHORT_EOCSEG     (1U<<30)

# define BCM_FEAT_OPT_GFAST_106a             BCM_GFAST_PROFILE_MASK_106a
# define BCM_FEAT_OPT_GFAST_212a             BCM_GFAST_PROFILE_MASK_212a
# define BCM_FEAT_OPT_GFAST_106b             BCM_GFAST_PROFILE_MASK_106b
# define BCM_FEAT_OPT_GFAST_106c             BCM_GFAST_PROFILE_MASK_106c
# define BCM_FEAT_OPT_GFAST_212c             BCM_GFAST_PROFILE_MASK_212c
# define BCM_FEAT_OPT_MGFAST_P424a           (1U<<8)       
# define BCM_FEAT_OPT_MGFAST_P424d           (1U<<9)
# define BCM_FEAT_OPT_MGFAST_Q424a           (1U<<10)
# define BCM_FEAT_OPT_MGFAST_Q424d           (1U<<11)
# define BCM_FEAT_OPT_GFAST_LR               (1U<<16)
# define BCM_FEAT_OPT_GFAST_ANNEX_D          (1U<<17)
# define BCM_FEAT_OPT_GFAST_EXTENDEDPSLEN    (1U<<18)
# define BCM_FEAT_OPT_GFAST_VAR_TIMEOUT      (1U<<19)
# define BCM_FEAT_OPT_GFAST_CD_1_1_LEN       (0xFU<<20) /* 20 .. 23 */
# define BCM_FEAT_OPT_GFAST_MNDSNOI          (3U<<24)   /* 24 .. 25 */
# define BCM_FEAT_OPT_GFAST_DATAGRAM_EOC     (1U<<26)
# define BCM_FEAT_OPT_GFAST_RPFIB            (1U<<27)
# define BCM_FEAT_OPT_GFAST_INM              (1U<<28)
# define BCM_FEAT_OPT_GFAST_ANNEX_X          (1U<<29)
# define BCM_FEAT_OPT_GFAST_RMCR             (1U<<30)
# define BCM_FEAT_OPT_GFAST_ANDEFTR          (1U<<31)

typedef struct ProtocolType ProtocolType;
struct ProtocolType {
  uint32 adsl1;
  uint32 xdsl2;
  uint32 vdsl2;
  uint32 gfast;
} BCM_PACKING_ATTRIBUTE ;

typedef struct StandardsBitMap StandardsBitMap;
struct StandardsBitMap
{
  uint32 bitMap;                /* Near-End supported standards */
  uint32 reserved;
} BCM_PACKING_ATTRIBUTE ;

typedef struct LineFeatures LineFeatures;
struct LineFeatures {
  StandardsBitMap supportedStandards[2]; /* index 0 is for NE */
  StandardsBitMap enabledStandards;      /* standards enabled as per test
                                          * config */
  ProtocolType    supportedOptions[2];   /* index 0 is for NE */
  ProtocolType    enabledOptions[2];     /* options enabled as per test config
                                          * US/DS for Upstream/Downstream */
} BCM_PACKING_ATTRIBUTE ;

typedef LineFeatures GetLineFeaturesMsgRep;
extern  MsgLayout    GetLineFeaturesLayout;

/* }}} */
/* {{{ ReadEocRegister service */

typedef struct EocRegisterId EocRegisterId;
struct EocRegisterId
{
  uint8 registerId;
} BCM_PACKING_ATTRIBUTE ;

typedef EocRegisterId  ReadEocRegisterMsgReq;
extern  MsgLayout      ReadEocRegisterLayout;

/* }}} */
/* {{{ GetEocReadResult service */

typedef struct EocReadResult EocReadResult;
struct EocReadResult
{
  uint8 eocReadStatus;
  uint8 length;
  uint8 eocData[32];
} BCM_PACKING_ATTRIBUTE ;

typedef EocReadResult  GetEocReadResultMsgRep;
extern  MsgLayout      GetEocReadResultLayout;

/* }}} */
/* {{{ WriteEocRegister service */

typedef struct EocRegister EocRegister;
struct EocRegister
{
  uint8 registerId;
  uint8 length;
  uint8 eocData[32];
} BCM_PACKING_ATTRIBUTE ;

typedef EocRegister  WriteEocRegisterMsgReq;
extern  MsgLayout    WriteEocRegisterLayout;

/* }}} */
/* {{{ GetEocWriteResult service */

typedef struct EocWriteResult EocWriteResult;
struct EocWriteResult
{
  uint8 eocWriteStatus;
  uint8 length;
} BCM_PACKING_ATTRIBUTE ;

typedef EocWriteResult GetEocWriteResultMsgRep;
extern  MsgLayout      GetEocWriteResultLayout;

/* }}} */
/* {{{ Send- and GetTransparentEocMessage services */

#define BCM_TRANSPARENT_EOC_MAX_SEGMENT_LENGTH (700)

typedef enum {BCM_TRANSPARENT_EOC_CLEAR_EOC = 0,
              BCM_TRANSPARENT_EOC_DATAGRAM_EOC,
              BCM_TRANSPARENT_EOC_NSF_NORMPRIO_EOC,
              BCM_TRANSPARENT_EOC_NSF_LOWPRIO_EOC,
              /* fast version of datagram not requiring EOC transmitted CB to proceed */
              BCM_TRANSPARENT_EOC_FAST_DATAGRAM_EOC 
} TransparentEocMsgType;

typedef struct TransparentEocMessage TransparentEocMessage;
struct TransparentEocMessage
{
  uint16 length;   /* segment length */
  uint8  eocType;  /* see TransparentEocMsgType enum type */
  uint8  partId;   /* MSB set = last part */
  uint8  message[BCM_TRANSPARENT_EOC_MAX_SEGMENT_LENGTH];
} BCM_PACKING_ATTRIBUTE ;

typedef TransparentEocMessage  SendTransparentEocMessageMsgReq;
extern  MsgLayout              SendTransparentEocMessageLayout;

typedef TransparentEocMessage  GetTransparentEocMessageMsgRep;
extern  MsgLayout              GetTransparentEocMessageLayout;

/* }}} */
/* {{{ GetBandPlan service */

# define BCM_IS_US0_ACTIVE(bp) \
  ((bp).bandPlanUp.toneGroups[0].startTone < 128)

# define BCM_FIRST_ACTIVE_BAND(bp, dir)                      \
  (dir == US?                                                \
   ((bp).bandPlanUp.toneGroups[0].startTone <  128? 0 :      \
    (bp).bandPlanUp.toneGroups[0].startTone < 1205? 1 : 2) : \
   ((bp).bandPlanDn.toneGroups[0].startTone <  859? 1 :      \
    (bp).bandPlanDn.toneGroups[0].startTone < 2338? 2 : 3))

# define BCM_GROUP_FACTOR(tg) \
  ((tg).endTone <= (BCM_NB_OAM_TG<<0) ? 0 :     \
   (tg).endTone <= (BCM_NB_OAM_TG<<1) ? 1 :     \
   (tg).endTone <= (BCM_NB_OAM_TG<<2) ? 2 :     \
   (tg).endTone <= (BCM_NB_OAM_TG<<3) ? 3 :     \
   4)

typedef struct BandPlan BandPlan;
struct BandPlan {
  BandPlanDescriptor bandPlanDn;
  BandPlanDescriptor bandPlanUp;
  ToneGroup supportedSetBoundsDn; /* start and stop tones of the DS supported
                                   * carrier set (see def in G992.3 clause 3.56)*/
  ToneGroup supportedSetBoundsUp; /* start and stop tones of the US supported
                                   * carrier set (see def in G992.3 clause 3.56)*/
  ToneGroup medleySetBoundsDn;    /* start and stop tones of the DS medley
                                   * set (see def in G992.3 clause 3.36) */
  ToneGroup medleySetBoundsUp;    /* start and stop tones of the US medley
                                   * set (see def in G992.3 clause 3.36) */
} BCM_PACKING_ATTRIBUTE ;

typedef BandPlan GetBandPlanMsgRep;
extern  MsgLayout    GetBandPlanLayout;

uint16 getBandPlanEndTone(BandPlan *bp, uint32 direction);

/* }}} */
/* {{{ EnterLineTestMode service */

typedef struct TestModeSelection TestModeSelection;
struct TestModeSelection
{
  uint8 filterMode;             /* indicates type of high-pass filter for ADSL
                                 * signals (0: Annex-A, 1: Annex-B */
} BCM_PACKING_ATTRIBUTE ;

typedef TestModeSelection  EnterLineTestModeMsgReq;
extern  MsgLayout          EnterLineTestModeLayout;

/* }}} */
/* {{{ ExitLineTestMode service */

extern MsgLayout    ExitLineTestModeLayout;

/* }}} */
/* {{{ StartLoopCharacterisation service */

typedef struct LoopCharacterisation LoopCharacterisation;
struct LoopCharacterisation
{
  uint8 measurementPeriod;      /* effective period = 10*2^measurementPeriod */
  int8  agcSetting;             /* If the forcedAgc field is set to one, this
                                 * field defines the agc gain to be used for the
                                 * measurement. Unit is 0.5 dB. Allowed range
                                 * is [-24 to +60] dB */
  CustomPsdTemplateDn customPsdTemplateDn;
  int16 signalPsd;              /* nominal psd to be used during singal
                                 * generation unit is 1/256 */
  uint8  psdOptim;              /* if set to one, use deciveConfig to limit the
                                 * total output power and consequenctly use the
                                 * customPsdTemplateDn and signalPsd as a mask
                                 * indication. */
  uint8 forceAgc;               /* Set on one to force a fixed AGC setting
                                 * during the measurement (to be used in
                                 * conjunction with the agcSetting field above
                                 */
  uint8 adsl2Plus;              /* perform loop characterization over ADSL2plus
                                   band (2.2MHz) */
  int8  offset;                 /* adjust echo phase alignment */
  uint8 carrierMask[64];        /* Carrier mask */
} BCM_PACKING_ATTRIBUTE ;

typedef LoopCharacterisation StartLoopCharacterisationMsgReq;
extern  MsgLayout            StartLoopCharacterisationLayout;

/* }}} */
/* {{{ GetLoopCharacterisation service */

typedef struct LoopCharacteristics LoopCharacteristics;
struct LoopCharacteristics
{
  uint32 testStatus;            /* 0: test not yet started
                                 * 1: test in progress
                                 * 2: test completed */
} BCM_PACKING_ATTRIBUTE ;

typedef LoopCharacteristics GetLoopCharacteristicsMsgRep;
extern  MsgLayout           GetLoopCharacteristicsLayout;

/* }}} */
/* {{{ StartSignalGenerationAndMeasurement service */

# define TESTSIGNAL_TYPE_REVERB           3
# define TESTSIGNAL_TYPE_MEDLEY           4
# define TESTSIGNAL_TYPE_QUIET            5
# define TESTSIGNAL_TYPE_TRAINING_VDSL    6
# define TESTSIGNAL_TYPE_MEDLEY_VDSL      7
# define TESTSIGNAL_TYPE_QUIET_VDSL       8
# define TESTSIGNAL_TYPE_PERIODIC_VDSL    9
# define TESTSIGNAL_TYPE_XTC_VDSL        10
# define TESTSIGNAL_TYPE_QUIET_GFAST     13
# define TESTSIGNAL_TYPE_PERIODIC_GFAST  14
# define TESTSIGNAL_TYPE_REVERB_GFAST    15

typedef struct SignalGeneration SignalGeneration;
struct SignalGeneration
{
  uint8    signalType;          /* see TESTSIGNAL_TYPE_... */
  int8     crestControl;        /* control crest factor of signal 3&6 */
  int16    signalPsdAdsl;
  uint32   dutyCycleON;
  uint32   dutyCycleOFF;
  uint8    carrierMaskAdsl[BCM_NB_ADSL_TONE_DS/8];
  uint8    filterConfig;        /* bit0: bypass decimator
                                 * bit1: enable extra Tx LPF
                                 * bit2: bypass 4-tap interpolator, even in 17A mode
                                 */

  uint8    measurementLength;   /* signal measurement over a length equal to
                                 * 2^measurementlength */
  uint8    sigGenConfig;        /* bit0: do tssi optimization;
                                   bit1: fix CS and analog gain;
                                   bit2: only do 1 signal measurement
                                   bit6: enable XTC in GFast
                                   bit7: force Gfast gainIn/gainOut to 0 */
  uint8    txFilterControl;     /* use Tx Filter setting */
  TxFilterDef txFilter;
  CustomPsdTemplateDn customTssiShapeAdsl; /* custom TSSI shape for ADSL signals */
  uint8    is30a;               /* use 30MHz bandwidth */
  int8     agcSetting;          /* If the forcedAgc field is set to one, this
                                 * field defines the agc gain to be used for the
                                 * measurement. Unit is 0.5 dB. Allowed range
                                 * is [-24 to +60] dB */
  uint8    reservedA[2];
  BandPlanDescriptor bandPlanDn; /* band plan DS for VDSL signals */
  BandPlanDescriptor bandPlanUp; /* band plan US for VDSL signals */
  PsdDescriptorDn    limitingMaskDn;
  RfiDescriptor      rfiNotches;
  int16   maxTxPowerVdsl;       /* max Tx Power for VDSL signals.
                                 * Unit is 1/256 dB */
  uint8 forceAgc;               /* Set on one to force a fixed AGC setting
                                 * during the measurement (to be used in
                                 * conjunction with the agcSetting field above
                                 */
  uint8 loopbackMode;
  uint8 powerMode;              /* If the MSB is set to one, the line driver
                                 * bias settings normally used in line test
                                 * are overwritten by the value configured in
                                 * the 4 LSB's. In addition, if bit 5 is set
                                 * to one, the peakdetect pin is forced to low
                                 * (when BCM65040 LD is used). */
  uint8 avgFactor;              /* The ADSL REVERB and VDSL PERIODIC signal
                                 * generation supports a new mode where
                                 * different signals are generated and
                                 * measured. The avgFactor controls the
                                 * averaging:
                                 * 0:    no averaging / a single REVERB signal is
                                 *       generated.
                                 * 1-16: averaging by 2^(-avgFactor)
                                 */
  uint16 socWord;               /* Two octets modulating the XTC_VDSL signal */
  uint8  reservedB[2];
} BCM_PACKING_ATTRIBUTE ;

typedef SignalGeneration  StartSignalGenerationAndMeasurementMsgReq;
extern  MsgLayout         StartSignalGenerationAndMeasurementLayout;

/* }}} */
/* {{{ GetSignalMeasurement service */

# define BCM_TONE2HZ_PSD(x) (((x)-9305)/256.0)

typedef struct SignalRequest SignalRequest;
struct SignalRequest
{
  uint8 partId;                 /* part of the measurement vector to retrieve,
                                 * in chunks of 64 tones */
  uint8 toneGroupingFactor;     /* Speed up factor: only retrieves tones
                                 * multiple of (1<<toneGroupingFactor) */
} BCM_PACKING_ATTRIBUTE ;

# define BCM_PSD_LEN 64

typedef struct SignalMeasurement SignalMeasurement;
struct SignalMeasurement
{
  uint32 testStatus;            /* 0: test not yet started
                                 * 1: test in progress
                                 * 2: test completed */
  uint32 testDuration;          /* Duration in seconds of the peak
                                 * noise measurement (reported below) */
  /* all PSD values are expressed in dBm referred to 100Ohm line / Tonebin */
  int16  peakPsd[BCM_PSD_LEN];  /*  max (x^2+y^2)/T,
                                 *  where T = peakMeasurementTime seconds */
  int16  totalPsd[BCM_PSD_LEN]; /*  sum(x^2+y^2)/T,  where T = 1 sec.      */
  int16  signalPsd[BCM_PSD_LEN];/* (sum(x)^2 sum(y)^2)/T, where T = 1 sec */
  uint8  partId;               /*  acknowledgement of partId in request */
  uint8  toneSpacing;          /* tonespacing used in measurement. 0 = 4KHz, 1 = 8KHz, 2 = 51.75 KHz */
  uint8  reserved[2];
} BCM_PACKING_ATTRIBUTE ;

typedef SignalRequest     GetSignalMeasurementMsgReq;
typedef SignalMeasurement GetSignalMeasurementMsgRep;
extern  MsgLayout         GetSignalMeasurementLayout;

/* }}} */
/* {{{ StartManufacturingTest service */

typedef struct ManufTestConfig ManufTestConfig;
struct ManufTestConfig
{
  uint8  loopbackMode;          /* see physical configuration */
  int8   crestControl;          /* control the crest factor of the signal */
                                /* used for the high crest THD test */
  uint16 duration;              /* in second */
  uint8  loopbackToneStart;
  uint8  loopbackToneStop;
  uint8  skipAFEtest;
  uint8  bandwidthMode; /* type of signal to be used for the loopback test part of the
                         * manufacturing test:
                         * BWmode=0: send signal over the ADSLband.
                         * BWmode=1: send signal over the ADSL2+ band.
                         * BWmode=2: send VDSL 8b signal
                         * BWmode=3: send flat VDSL 17a signal
                         * BWmode=4: send combined VDSL17a+8b signal with PSD shaping
                         */
} BCM_PACKING_ATTRIBUTE ;

typedef ManufTestConfig StartManufacturingTestMsgReq;
extern  MsgLayout       StartManufacturingTestLayout;

/* }}} */
/* {{{ GetManufacturingTestResult service */

# define BCM_MANUF_BANDS 4
typedef struct ManufacturingTestResult ManufacturingTestResult;
struct ManufacturingTestResult
{
  uint32 testStatus;            /* 0: test not yet started
                                 * 1: test in progress
                                 * 2: test completed */
  uint32 loopbackRate;
  uint32 loopbackBits;
  uint32 loopbackErrors;
  int32  THRLmean;
  int32  THRLdip;
  int32  THDandNoise[BCM_MANUF_BANDS];
                                /* THD and Noise measrured in US band.
                                 * Index 0 is for VDSL US0 band or ADSL mode
                                 * Index 1 is for VDSL US1 band
                                 * Index 2 is for VDSL US2 band
                                 * Index 3 is for VDSL US3 band
                                 * Unit is 0.1 dB */
  int32  THDdeltaSignal[BCM_MANUF_BANDS]; /* Difference between Total PSD and
                                           * signal PSD. Unit is 0.1 dB.
                                          */
  int32  THDdeltaHighCrest[BCM_MANUF_BANDS];
                                /* THD increase due to the high crest signal */
                                /* used. Expressed as a delta in 0.1 dB unit */
                                /* with respect to the THD result provided */
                                /* in the same test. */
  int32  agc;
  uint16 gfastAfeCalibrationResult[2];
  int16 gfastAfeCalibrationMetric; /* offset in % (fix8_8, 256 = 1%) */
  uint16 reserved;
} BCM_PACKING_ATTRIBUTE ;

typedef ManufacturingTestResult GetManufacturingTestResultMsgRep;
extern  MsgLayout               GetManufacturingTestResultLayout;

/* }}} */
/* {{{ GetLineAGCsetting service */

typedef struct AGCsetting AGCsetting;
struct AGCsetting
{
  int16 agc;                    /* Receiver gain used during last test */
} BCM_PACKING_ATTRIBUTE ;

typedef AGCsetting GetLineAGCsettingMsgRep;
extern  MsgLayout  GetLineAGCsettingLayout;

/* }}} */
/* {{{ LineGetFePilots service */

#define VDSL_MSG_MAX_NO_OF_TONES_IN_TONE_DESC        16

typedef struct ToneDescriptor ToneDescriptor;
struct ToneDescriptor
{
  uint8  noOfTones;
  uint8  reserved;
  uint16 tones[VDSL_MSG_MAX_NO_OF_TONES_IN_TONE_DESC];
};

typedef ToneDescriptor GetLineFePilotsMsgRep;
extern  MsgLayout  GetLineFePilotsLayout;

/* }}} */
/* {{{ GetEcho service */

# define BCM_ECHO_PART_SIZE 64

typedef struct LongPartId LongPartId;
struct LongPartId
{
  uint32 partId;
} BCM_PACKING_ATTRIBUTE ;

typedef struct AccurateEcho AccurateEcho;
struct AccurateEcho
{
  int32 partialEchoResponse[BCM_ECHO_PART_SIZE];
} BCM_PACKING_ATTRIBUTE ;

typedef LongPartId   GetEchoMsgReq;
typedef AccurateEcho GetEchoMsgRep;
extern  MsgLayout    GetEchoLayout;

/* }}} */
/* {{{ GetSelt service */

typedef struct SeltResult SeltResult;
struct SeltResult
{
  int32 seltValue[64];
} BCM_PACKING_ATTRIBUTE;

typedef LongPartId GetSeltMsgReq;
typedef SeltResult GetSeltMsgRep;
extern  MsgLayout  GetSeltLayout;

/* }}} */
/* {{{ GetEchoVariance service */

# define BCM_ECHOVAR_PART_SIZE 128

typedef struct AccurateEchoVariance AccurateEchoVariance;
struct AccurateEchoVariance
{
  int16 varValues[BCM_ECHOVAR_PART_SIZE];
} BCM_PACKING_ATTRIBUTE ;

typedef LongPartId           GetEchoVarianceMsgReq;
typedef AccurateEchoVariance GetEchoVarianceMsgRep;
extern  MsgLayout            GetEchoVarianceLayout;

/* }}} */
/* {{{ GetLineBitAllocation service */

# define BCM_GET_BI_REP_SIZE 736
# define BCM_NB_BIT_ALLOC_DS_PART (((BCM_NB_TONE_DS-1)/BCM_GET_BI_REP_SIZE)+1)
# define BCM_NB_BIT_ALLOC_US_PART (((BCM_NB_TONE_US-1)/BCM_GET_BI_REP_SIZE)+1)

typedef struct LineBitAllocAndSnrAndGiReq LineBitAllocAndSnrAndGiReq;
struct LineBitAllocAndSnrAndGiReq
{
  uint8 isDownstream;           /* 0 for US, 1 for DS */
  uint8 partId;                 /* bits [6:0]: specifies the toneBlock.
                                 *      Data for tones toneBlock*480 to
                                 *      toneBlock*480+479 will be retrieved.
                                 * The partId field is only used if both startTone
                                 * nTones fields below are zero.
                                 */
  uint16 startTone;             /* first tone to include in the toneBlock for which
                                 * bits, snr or gi is retrieved. */
  uint16 nTones;                /* number of tones to include in the toneBlock */
  uint8  log2M;                 /* log2M!=0 only applicable for LineGetBitAllocation message */
  uint8  includeRi;
  uint8  reserved[6];

} BCM_PACKING_ATTRIBUTE ;

typedef struct GetLineBitAllocationRep GetLineBitAllocationRep;
struct GetLineBitAllocationRep
{
  uint8  bitAllocTable[BCM_GET_BI_REP_SIZE];
} BCM_PACKING_ATTRIBUTE ;

typedef GetLineBitAllocationRep GetLineBitAllocationMsgRep;
typedef LineBitAllocAndSnrAndGiReq GetLineBitAllocationMsgReq;
extern  MsgLayout GetLineBitAllocationLayout;

# define BCM_MAX_RMC_TONES           512
# define BCM_NB_RMC_BIT_ALLOC_PART   3
# define BCM_RMC_BIT_ALLOC_PART_SIZE 240

typedef struct GetLineRmcBitAllocationRep GetLineRmcBitAllocationRep;
struct GetLineRmcBitAllocationRep
{
  uint32 nTonesRmc;
  uint8  bi[BCM_RMC_BIT_ALLOC_PART_SIZE];
  uint16 rts[BCM_RMC_BIT_ALLOC_PART_SIZE];
} BCM_PACKING_ATTRIBUTE ;

typedef GetLineRmcBitAllocationRep GetLineRmcBitAllocationMsgRep;
typedef LineBitAllocAndSnrAndGiReq GetLineRmcBitAllocationMsgReq;
extern  MsgLayout GetLineRmcBitAllocationLayout;

/* }}} */
/* {{{ GetLineSnr service */

# define BCM_GET_SNR_REP_SIZE 368
# define BCM_NB_SNR_US_PART (((BCM_NB_TONE_US-1)/BCM_GET_SNR_REP_SIZE)+1)

/* request part uses LineBitAllocAndSnrAndGiReq, with each toneBlock having 368 */
/* tones (and only US is supported) */
typedef struct LineSnr LineSnr;
struct LineSnr
{
  uint16 snr[BCM_GET_SNR_REP_SIZE];
} BCM_PACKING_ATTRIBUTE ;

typedef LineBitAllocAndSnrAndGiReq GetLineSnrMsgReq;
typedef LineSnr                    GetLineSnrMsgRep;
extern  MsgLayout                  GetLineSnrLayout;

/* }}} */
/* {{{ GetVDSLEcho service */

typedef struct GetVDSLechoRep GetVDSLechoRep;
struct GetVDSLechoRep
{
  uint16 numRxTones;
  int16 totalRxGain;            /* 1/256 dB unit */
  int16 analogRxGain;           /* 1/256 dB unit */
  uint8 reserved[2];
  int16 partialVDSLechoMant[2*BCM_ECHO_PART_SIZE];
  int8  partialVDSLechoExp[BCM_ECHO_PART_SIZE];
} BCM_PACKING_ATTRIBUTE ;

typedef GetVDSLechoRep GetVDSLechoMsgRep;
typedef LongPartId GetVDSLechoMsgReq;
extern  MsgLayout  GetVDSLechoLayout;

/* }}} */
/* {{{ GetLinePeriodCounters service */

# define PERIOD_CURRENT_15_MIN    0
# define PERIOD_PREVIOUS_15_MIN   1
/* following period counters indexes are not accepted by HMI, they are
 * extensions processed by the CounterMgr_getCounters function or the 'api
 * getCounters' CLI command. */
# define PERIOD_CURRENT_24_HOURS  2
# define PERIOD_PREVIOUS_24_HOURS 3
# define PERIOD_CURRENT_RUNNING   4
# define PERIOD_SINCE_RESET       5
# define PERIOD_SINCE_LAST_READ   6

# define BCM_COUNTER_PERIOD_NAME(period) \
  ((period) == PERIOD_CURRENT_15_MIN  ?   "Current  15 min" : \
   (period) == PERIOD_PREVIOUS_15_MIN ?   "Previous 15 min" : \
   (period) == PERIOD_CURRENT_24_HOURS ?  "Current  24 hrs" : \
   (period) == PERIOD_PREVIOUS_24_HOURS ? "Previous 24 hrs" : \
   (period) == PERIOD_CURRENT_RUNNING ?   "Running" : \
   (period) == PERIOD_SINCE_RESET ?       "Since Reset" : \
   (period) == PERIOD_SINCE_LAST_READ ?   "Since Last Read" : \
   "unknown")

typedef struct PeriodIdentification PeriodIdentification;
struct PeriodIdentification
{
  uint32   periodIdentification;   /* Flag indicating which OAM report period
                                    * 0: current  15 min
                                    * 1: previous 15 min
                                    */
} BCM_PACKING_ATTRIBUTE ;

# define BCM_SUSPECT_BECAUSE_RESET           1
# define BCM_SUSPECT_BECAUSE_TOO_LONG_PERIOD 2
# define BCM_SUSPECT_BECAUSE_FE_NOT_RELIABLE 4 /* only for far end */
# define BCM_SUSPECT_BECAUSE_WRONG_RETROFIT  8 /* only for near end */

typedef struct HmiPeriodCounters HmiPeriodCounters;
struct HmiPeriodCounters
{
  DerivedSecCounters  derivedCounts[2];     /* index 0 is for NE */
  AdslAnomCounters    adslAnomalyCounts[2]; /* index 0 is for B0 */
  HmiPerfCounters     perfCounts[2];        /* index 0 is for B0 */

  uint16 failedFullInitCount;      /* incremented each time an initialization
                                    * failure is reported in current period */
  uint8  fullInitCount;            /* incremented each time we enter showtime */
  uint8  reInitCount;              /* incremented each time we start a new training
                                    * within 20s after a showtime drop */
  uint16 fastInitCounter;
  uint16 failedFastInitCounter;
  uint16 periodDuration;           /* indicate the real duration of the OAM period
                                      in seconds. */
  uint8  suspectFlagNE;            /* the suspect flag is activated either
                                    * - the first time PM is read after a
                                    *   DSP reset or line reset.
                                    * - when the elapsed time between two
                                    *   reads is bigger than 1 hour.
                                    * - when retrofit fails */
  uint8  suspectFlagFE;            /* the suspect flag is activated either
                                    * - the first time PM is read after a
                                    *   Santorini reset or line reset.
                                    * - when the elapsed time between two
                                    *   reads is bigger than 1 hour.
                                    * - the FE statistics are not reliable
                                    *   because of a IB were not reliable at
                                    *   some point in time during the last
                                    *   monitoring period.
                                    */
  uint32 upTime;                   /* nr of seconds already in showtime */
  uint32 periodIdentification;
  uint32 elapsedTimeIn15min;
  uint32 elapsedTimeIn24hour;
  GinpCounters ginpCounters[2];      /* index 0 is for NE */
  GfastCounters gfastCounters[2];    /* index 0 is for NE */
} BCM_PACKING_ATTRIBUTE ;

typedef PeriodIdentification GetLinePeriodCountersMsgReq;
typedef HmiPeriodCounters    GetLinePeriodCountersMsgRep;
extern  MsgLayout            GetLinePeriodCountersLayout;

/* }}} */
/* {{{ SetLineOAMthresholds service */

typedef struct OAMthresholds OAMthresholds;
struct OAMthresholds
{
  uint32  LOSSthreshold;
  uint32  ESthreshold;
  uint32  SESthreshold;
  uint32  UASthreshold;
  uint32  LOFSthreshold;
  uint32  LPRSthreshold;        /* LOLS thresh NE, LPRS thresh FE */
  uint32  LEFTRSthreshold;
  uint32  LORSthreshold;
} BCM_PACKING_ATTRIBUTE ;

typedef struct NewThresholds NewThresholds;
struct NewThresholds
{
  OAMthresholds   threshold15min[2]; /* NE-FE */
} BCM_PACKING_ATTRIBUTE ;

typedef NewThresholds SetLineOAMthresholdsMsgReq;
extern  MsgLayout     SetLineOAMthresholdsLayout;

/* }}} */
/* {{{ GetLineFEvendorInfo service */

/* vendor info is MSByte first as per standard */
# define BCM_GET_VENDOR_INFO(info) \
  (((info) [0]<<8) + (info)[1])

/* country code: if first byte != 255, ignore second byte */
# define BCM_GET_COUNTRY_CODE(code) \
  ((code[0]) == 0xFF ? (code)[1] : (code)[0])

typedef struct VendorId VendorId;
struct VendorId
{
  uint8  countryCode[2];
  uint8  providerCode[4];
  uint8  vendorInfo[2];
} BCM_PACKING_ATTRIBUTE ;

typedef struct FEparamsInfo FEparamsInfo;
struct FEparamsInfo
{
  uint32 minBitRate;
  uint32 maxBitRate;
  uint8  maxDelay;
  uint8  INPmin;
  uint8  msgMin;
  uint8  reserved;
} BCM_PACKING_ATTRIBUTE ;

typedef struct BcmCpeVersion BcmCpeVersion;
struct BcmCpeVersion
{
  uint8 majVersion;
  uint8 minVersion;
  uint8 extraVersion;
  uint8 chipType;
} BCM_PACKING_ATTRIBUTE ;

# define BCM_VENDOR_INFO_HS_AVAIL   1
# define BCM_VENDOR_INFO_ANSI_AVAIL 2
# define BCM_VENDOR_INFO_BCM_AVAIL  4

# define BCM_VENDOR_INFO_INTEROP_GFAST_STAGE_1 0x01

typedef struct FEvendorInfo FEvendorInfo;
struct FEvendorInfo
{
  VendorId      hsVendorId;
  uint16        t1_413_vendorInfo;
  uint8         availInfo;      /* bitmap indicating which info is available
                                 * bit [0] : hs available
                                 * bit [1] : eocVendorId avail
                                 * bit [2] : BCM CPE info avail
                                 */
  uint8         interopInfo;
  FEparamsInfo  params[2];       /* US/DS for Upstream/Downstream */
  BcmCpeVersion cpeVer;
} BCM_PACKING_ATTRIBUTE ;

typedef FEvendorInfo  GetLineFEvendorInfoMsgRep;
extern  MsgLayout     GetLineFEvendorInfoLayout;

/* }}} */
/* {{{ Send/GetOverheadMessage service */

# define BCM_OVH_BUF_SIZE 256
typedef struct OverheadMessage OverheadMessage;
struct OverheadMessage
{
  int16  length;
  uint8  message[BCM_OVH_BUF_SIZE];
} BCM_PACKING_ATTRIBUTE ;

typedef OverheadMessage SendOverheadMessageMsgReq;
extern  MsgLayout       SendOverheadMessageLayout;

typedef struct OneBytePartId OneBytePartId;
struct OneBytePartId
{
  uint8 partId;                 /* 0-3 */
} BCM_PACKING_ATTRIBUTE ;

typedef OneBytePartId   GetOverheadMessageMsgReq;
typedef OverheadMessage GetOverheadMessageMsgRep;
extern  MsgLayout       GetOverheadMessageLayout;

/* }}} */
/* {{{ GetOverheadStatus service */

/* GetOverheadStatusMsgRep is a one byte reply giving the status as follows:
 * 0: reply available
 * 1: still busy */
typedef uint8     GetOverheadStatusMsgRep;
extern  MsgLayout GetOverheadStatusLayout;

/* }}} */
/* {{{ GetLineLoopDiagResults service */

typedef struct LoopDiagResults LoopDiagResults;
struct LoopDiagResults {
  uint32 attndr;      /* Attainable bit rate. Unit is 1 bit per second (bps).*/
  uint16 LATN;        /* Loop attenuation. Range is [0..1023], unit is 0.1 dB.
                       * loopAttenuation = 1023 is a special value indicating that
                       * the loop attenuation is outside the represented range.*/
  uint16 SATN;        /* Signal attenuation. Range is [0..1023], unit is 0.1 dB.
                       * signalAttenuation = 1023 is a special value indicating
                       * that the loop attenuation is outside the represented
                       * range.*/
  int16  SNRM;        /* unit is 0.1 dB. snrMargin = 512 is a special value
                       * indicating that the signal-to-noise ratio margin is
                       * outside the represented range.*/
  int16  actatpFe;    /* The far-end actual aggregate transmit power as determined
                       * by the receive PMD function. Range is [-511..512], unit is
                       * 0.1 dBm. actualTxPowerFE = 512 is a special value
                       * indicating that the actual aggregate transmit power is
                       * outside the represented range.*/
} BCM_PACKING_ATTRIBUTE;

typedef struct LoopDiagPCBInfo LoopDiagPCBInfo;
struct LoopDiagPCBInfo {
  uint8 passFail;     /* Cause of the last failed full initialization. Coding is
                       * the same as the lastRetrainCause field in the
                       * lastRetrainInfo structure.*/
  uint8 lastTxState;  /* Last transmitted state reach during the last failed
                       * full initialization. Coding is the same as the
                       * lastTransmittedState field in the lastRetrainInfo
                       * structure.*/
} BCM_PACKING_ATTRIBUTE;

typedef struct LineLoopDiagResults LineLoopDiagResults;
struct LineLoopDiagResults
{
  LoopDiagResults ne;
  LoopDiagResults fe;
  LoopDiagPCBInfo pcbNe;
  LoopDiagPCBInfo pcbFe;
} BCM_PACKING_ATTRIBUTE ;

typedef LineLoopDiagResults GetLineLoopDiagResultsMsgRep;
extern  MsgLayout GetLineLoopDiagResultsLayout;

/* }}} */
/* {{{ GetLineLoopDiagHlin service */

typedef struct LineHlinReq LineHlinReq;
struct LineHlinReq
{
  uint8 direction;              /* 0 = NE, 1 = FE */
  uint8 partId;                 /* 0 for NE, 0-7 in case of FE */
} BCM_PACKING_ATTRIBUTE ;

typedef struct LineHlin LineHlin;
struct LineHlin
{
  uint16       hlinScale;
  ComplexInt16 hlin[64];
} BCM_PACKING_ATTRIBUTE ;

typedef LineHlinReq GetLineLoopDiagHlinMsgReq;
typedef LineHlin    GetLineLoopDiagHlinMsgRep;
extern  MsgLayout   GetLineLoopDiagHlinLayout;

/* }}} */
/* {{{ GetLineLoopDiagHlog service */

typedef uint16 LineHlogNE[64];
typedef uint16 LineHlogFEpart[128];

typedef struct LineHlog LineHlog;
struct LineHlog
{
  LineHlogNE     hlogNE;        /* Near end Hlog */
  LineHlogFEpart hlogFE;        /* Far end Hlog (1/4 part) */
} BCM_PACKING_ATTRIBUTE ;

typedef OneBytePartId  GetLineLoopDiagHlogMsgReq;
typedef LineHlog       GetLineLoopDiagHlogMsgRep;
extern  MsgLayout      GetLineLoopDiagHlogLayout;

/* }}} */
/* {{{ GetLineLoopDiagQln service */

typedef uint8 LineQlnNE[64];
typedef uint8 LineQlnFEpart[256];

typedef struct LineQln LineQln;
struct LineQln
{
  LineQlnNE     qlnNE;          /* Near end QLN */
  LineQlnFEpart qlnFE;          /* Far end QLN (1/2 part) */
} BCM_PACKING_ATTRIBUTE ;

typedef OneBytePartId  GetLineLoopDiagQlnMsgReq;
typedef LineQln        GetLineLoopDiagQlnMsgRep;
extern  MsgLayout      GetLineLoopDiagQlnLayout;

/* }}} */
/* {{{ GetLineLoopDiagSnr service */

typedef uint8 LineSnrNE[64];
typedef uint8 LineSnrFEpart[256];

typedef struct LoopDiagSnr LoopDiagSnr;
struct LoopDiagSnr
{
  LineSnrNE     snrNE;          /* Near end SNR */
  LineSnrFEpart snrFE;          /* Far end SNR (1/2 part) */
} BCM_PACKING_ATTRIBUTE ;

typedef OneBytePartId  GetLineLoopDiagSnrMsgReq;
typedef LoopDiagSnr    GetLineLoopDiagSnrMsgRep;
extern  MsgLayout      GetLineLoopDiagSnrLayout;

/* }}} */
/* {{{ UpdateTestParameters service */

# define BCM_TEST_PARAM_SHOW_STATUS   0
# define BCM_TEST_PARAM_INVALIDATE    1
# define BCM_TEST_PARAM_INVALIDATE_NE 2
# define BCM_TEST_PARAM_INVALIDATE_FE 4

typedef struct UpdateTestParametersReq UpdateTestParametersReq;
struct UpdateTestParametersReq
{
  uint8 command;                /* 0: read status of test parameters
                                 * 1: invalidate test parameters */
} BCM_PACKING_ATTRIBUTE ;

typedef struct UpdateTestParametersRep UpdateTestParametersRep;
struct UpdateTestParametersRep
{
  uint8 status;                 /* 0: test parameters are present
                                 * 1: test parameters are invalidated */
} BCM_PACKING_ATTRIBUTE ;

typedef UpdateTestParametersReq UpdateTestParametersMsgReq;
typedef UpdateTestParametersRep UpdateTestParametersMsgRep;
extern  MsgLayout               UpdateTestParametersLayout;

/* }}} */
/* {{{ GetLineHlogNE service */

typedef struct GetLineHlogNEMsgRep GetLineHlogNEMsgRep;
struct GetLineHlogNEMsgRep
{
  LineHlogNE hlogNE;
} BCM_PACKING_ATTRIBUTE ;

extern  MsgLayout      GetLineHlogNELayout;

/* }}} */
/* {{{ GetLineQlnNE service */

typedef struct GetLineQlnNEMsgRep GetLineQlnNEMsgRep;
struct GetLineQlnNEMsgRep
{
  LineQlnNE qlnNE;
} BCM_PACKING_ATTRIBUTE ;

extern  MsgLayout      GetLineQlnNELayout;

/* }}} */
/* {{{ GetLineCarrierGains service */

# define BCM_GET_GI_REP_SIZE 368
# define BCM_NB_CARRIER_GAINS_DS_PART (((BCM_NB_TONE_DS-1)/BCM_GET_GI_REP_SIZE)+1)
# define BCM_NB_CARRIER_GAINS_US_PART (((BCM_NB_TONE_US-1)/BCM_GET_GI_REP_SIZE)+1)

typedef struct LineCarrierGains LineCarrierGains;
struct LineCarrierGains
{
  uint16     gi[BCM_GET_GI_REP_SIZE]; /* Unit is 1/512 dB */
} BCM_PACKING_ATTRIBUTE ;


typedef LineBitAllocAndSnrAndGiReq GetLineCarrierGainsMsgReq;
typedef LineCarrierGains           GetLineCarrierGainsMsgRep;
extern  MsgLayout                  GetLineCarrierGainsLayout;

/* }}} */
/* {{{ GetLineLinearTssi service */

# define BCM_TSSI_US_SIZE 64
# define BCM_TSSI_DS_SIZE 128
# define BCM_GET_TSSI_REP_SIZE (BCM_TSSI_US_SIZE+BCM_TSSI_DS_SIZE)

typedef uint16 LineTssiUS[BCM_TSSI_US_SIZE];  /* unit is 1/32768 */
typedef uint16 LineTssiDSpart[BCM_TSSI_DS_SIZE];

typedef struct LineLinearTssi LineLinearTssi;
struct LineLinearTssi
{
  LineTssiUS     tssiUS;            /* US linear TSSI values */
  LineTssiDSpart tssiDS;            /* DS linear TSSI values (1/4 part) */
} BCM_PACKING_ATTRIBUTE ;

typedef OneBytePartId  GetLineLinearTssiMsgReq;
typedef LineLinearTssi GetLineLinearTssiMsgRep;
extern  MsgLayout      GetLineLinearTssiLayout;

/* }}} */
/* {{{ LineIntroduceErrors service */

#define BCM_ERROR_TYPE_CRC                 0
#define BCM_ERROR_TYPE_RTX_TEST_ON        0x107
#define BCM_ERROR_TYPE_RTX_TEST_OFF       0x108
#define BCM_ERROR_TYPE_TPS_TEST_ON        0x109
#define BCM_ERROR_TYPE_TPS_TEST_OFF       0x10A
#define BCM_ERROR_TYPE_MODIFY_CFG        0x8000

typedef struct LineErrors LineErrors;
struct LineErrors
{
  uint16  errorType;
  int16   numberOfErrors;
  uint16  errorPeriodicity;
  uint16  errorTrigger;
  int16   targetMargin;
  int16   downShiftMargin;
  int16   upShiftMargin;
} BCM_PACKING_ATTRIBUTE ;

typedef LineErrors LineIntroduceErrorsMsgReq;
extern  MsgLayout  LineIntroduceErrorsLayout;

/* }}} */
/* {{{ LineGoToL3 service */

extern MsgLayout    LineGoToL3Layout;

/* }}} */
/* {{{ LineGoToL2 service */

extern MsgLayout    LineGoToL2Layout;

/* }}} */
/* {{{ LineGoToL0 service */

extern MsgLayout    LineGoToL0Layout;

/* }}} */
/* {{{ LineL2PowerTrim service */

typedef struct PowerTrim PowerTrim;
struct PowerTrim
{
  int8 deltaPsd;
} BCM_PACKING_ATTRIBUTE ;

typedef PowerTrim LineL2PowerTrimMsgReq;
extern  MsgLayout LineL2PowerTrimLayout;

/* }}} */
/* {{{ GetLineOamVector - used to retrieve VDSL2 DELT information */

/* VDSL2 showtime only Hlog-US, Qln-US
   VDSL2 DELT-end: all

   byte 0   : parameter ID :
   Hlog-NE = 0x01 (2 reply byte per toneGroup)
   Hlin-NE = 0x02 (6 reply byte per toneGroup)
   Qln-NE  = 0x03 (1 reply byte per toneGroup)
   Snr-NE  = 0x04 (1 reply byte per toneGroup)
   Hlog-FE = 0x81 (2 reply byte per toneGroup)
   Hlin-FE = 0x82 (6 reply byte per toneGroup)
   Qln-FE  = 0x83 (1 reply byte per toneGroup)
   Snr-FE  = 0x84 (1 reply byte per toneGroup)

   byte 1..2: msb and lsb of startToneGroupIndex
   byte 3..4: msb and lsb of stopToneGroupIndex
 */

# define BCM_GET_OAM_HLOG      0x01
# define BCM_GET_OAM_HLIN      0x02
# define BCM_GET_OAM_QLN       0x03
# define BCM_GET_OAM_SNR       0x04
# define BCM_GET_OAM_PARAM_ID(id, dir) (((dir)<<7) | (id&0xF))

# define BCM_GET_OAM_WORD_SIZE(id) \
  ( (id&0xF) == BCM_GET_OAM_HLOG ? 2 : \
    (id&0xF) == BCM_GET_OAM_HLIN ? 6 : 1)

# define BCM_GET_OAM_REP_SIZE 484

typedef struct GetLineOamVectorReq GetLineOamVectorReq;
struct GetLineOamVectorReq
{
  uint8 data[5];
} BCM_PACKING_ATTRIBUTE;

/* OamVectorReply:
   byte 0: QLNGus, tone grouping factor of the SUPPORTEDCARRIERSus,
                   applicable to Qln-US,Hlog-US and Hlin-US
   byte 1: QLNGds, tone grouping factor of the SUPPORTEDCARRIERSds,
                   applicable to Qln-DS,Hlog-DS and Hlin-DS
   byte 2: SNRGus, tone grouping factor of the MEDLEYSETus, applicable to Snr-US
   byte 3: SNRGds, tone grouping factor of the MEDLEYSETds, applicable to Snr-DS
   byte 4-483: maximum 480 OAM bytes
*/
typedef struct GetLineOamVectorRep GetLineOamVectorRep;
struct GetLineOamVectorRep
{
  uint8 data[BCM_GET_OAM_REP_SIZE];
} BCM_PACKING_ATTRIBUTE;

typedef GetLineOamVectorReq GetLineOamVectorMsgReq;
typedef GetLineOamVectorRep GetLineOamVectorMsgRep;
extern  MsgLayout        GetLineOamVectorLayout;

/* }}} */
/* {{{ GetLinePtmCounters */

typedef struct PtmChannelCounters PtmChannelCounters;
struct PtmChannelCounters
{
  uint32 TC_CV;                  /* 64/65b code violation */
  uint32 ptm_rx_utopia_overflow; /* number of rx eth packet that could not be
                                    passed to upper layer becasue of utopia overflow */
  uint32 ptm_rx_crc_error_count; /* number of rx eth packet with ptm crc */
  uint32 ptm_rx_packet_count; /* number of rx eth packet received */
  uint32 ptm_tx_packet_count; /* number of tx eth packet */
  uint32 ptm_tx_buffer_flushes; /* number of tx buffer flush - usually because
                                   msg too big has been received */
} BCM_PACKING_ATTRIBUTE ;


typedef struct PtmCounters PtmCounters;
struct PtmCounters
{
  PtmChannelCounters  ptmCounters[2][2]; /* 2 bearers, 2 priorities */
} BCM_PACKING_ATTRIBUTE ;

typedef PtmCounters GetLinePtmCountersMsgRep;
extern  MsgLayout   GetLinePtmCountersLayout;

/* }}} */
/* {{{ SetLineG9991Monitor */

typedef struct G9991Monitor G9991Monitor;
struct G9991Monitor
{
  uint8   monitor;     /* id of the monitor block used to monitor this line. Possible value 0 or 1 */
  uint8   reservedA;
  uint16  reservedB;

} BCM_PACKING_ATTRIBUTE;

typedef G9991Monitor  SetLineG9991MonitorMsgReq;
extern  MsgLayout     SetLineG9991MonitorLayout;

/* }}} */
/* {{{ GetLineG9991Counters */

typedef struct G9991Counters G9991Counters;
struct G9991Counters
{
  /* BLOCK */
  uint8  monitoringId;           /* id of this line's monitor */
  uint8  reservedA;
  uint16 reservedB;
  /* MAC counters */
  uint32 rxFragments;            /* fragments received on MAC block */
  uint32 rxFCSError;             /* fragments received on MAC block containing FCS errors */
  uint32 rxBadAlignment;         /* fragments with bad allignment on MAC block*/
  uint32 rxShortFragments;       /* fragments with illegal length on MAC block*/
  uint32 rxLongFragments;        /* fragments with illegal length on MAC block*/
  uint32 rxOverflow;             /* number of bytes which overflowed in rx MAC block */

  uint32 txFragments;            /* fragments transmitted from MAC block */
  uint32 txErrors;               /* tx fragments discarded in MAC block */
  uint32 generatedFCframes;      /* flow control frames transmitted from MAC block */

  /* counters from celliser */
  uint32 rxFragmentsSidValid;    /* fragments received with valid sid matching the monitored line. */
  uint32 rxFragmentsSidInvalid;  /* number of unrecognized fragments */

  /* counters from netb */
  uint32 rxFramesB0P0;           /* number of frames received on B0 low/normal priority of monitored line. */
  uint32 rxFramesB0P1;           /* number of frames received on B0 high priority of monitored line. */
  uint32 rxFramesB1;             /* number of frames received on B1  of monitored line*/
} BCM_PACKING_ATTRIBUTE;

typedef G9991Counters GetLineG9991CountersMsgRep;
extern  MsgLayout     GetLineG9991CountersLayout;

/* }}} */
/* {{{ GetLineMrefPsd */

#ifndef __MREFPSD_
#define __MREFPSD_

# define BCM_MAX_DS_MREF_PSD_BP 48
# define BCM_MAX_US_MREF_PSD_BP 48

typedef struct DsMrefPsdDescriptor DsMrefPsdDescriptor;
struct DsMrefPsdDescriptor {
  uint8      reserved;
  uint8      n;
  BreakPoint bp[BCM_MAX_DS_MREF_PSD_BP];
} BCM_PACKING_ATTRIBUTE ;

typedef struct UsMrefPsdDescriptor UsMrefPsdDescriptor;
struct UsMrefPsdDescriptor {
  uint8      reserved;
  uint8      n;
  BreakPoint bp[BCM_MAX_US_MREF_PSD_BP];
} BCM_PACKING_ATTRIBUTE ;

typedef struct MrefPsd MrefPsd;
struct MrefPsd {
  DsMrefPsdDescriptor dsDescriptor;
  UsMrefPsdDescriptor usDescriptor;
} BCM_PACKING_ATTRIBUTE ;

#endif  /* __MREFPSD_ */

typedef MrefPsd   GetLineMrefPsdMsgRep;
extern  MsgLayout GetLineMrefPsdLayout;

/* }}} */
/* {{{ Set/GetLineNtrPhaseComp */

typedef struct NtrPhaseComp NtrPhaseComp;
struct NtrPhaseComp
{
  int16 vdsl;                   /* phase correction on the PPS signal,
                                 * expressed in 7.08ns periods */
  uint8 reserved[18];
} BCM_PACKING_ATTRIBUTE ;

typedef NtrPhaseComp SetLineNtrPhaseCompMsgReq;
extern  MsgLayout    SetLineNtrPhaseCompLayout;

typedef NtrPhaseComp GetLineNtrPhaseCompMsgRep;
extern  MsgLayout    GetLineNtrPhaseCompLayout;

/* }}} */
/* {{{ SetLineNeInmConfig */

typedef struct InmParameters InmParameters;
struct InmParameters {
  uint8  equivalentInpMode;
  uint8  clusterContinuationParameter;
  uint16 interArrivalTimeOffset;
  uint8  interArrivalTimeStep;
  int8   isddSensitivityDB;
  uint8  equivalentInpFormat;
  uint8  bridging;
  uint8  inpeqScalingFactor;
  uint8  iatScalingFactor;
  uint8  protocol;
  uint8  reserved;
} BCM_PACKING_ATTRIBUTE ;

typedef InmParameters  SetLineNeInmConfigMsgReq;
extern  MsgLayout      SetLineNeInmConfigLayout;

/* }}} */
/* {{{ GetLineNeInmCounters */

# define BCM_INM_INPEQ_LEN 18
# define BCM_INM_IAT_LEN 8

typedef struct InmCounters InmCounters;
struct InmCounters {
  uint32 equivalentInpHistogram[BCM_INM_INPEQ_LEN];
  uint32 interArrivalHistogram[BCM_INM_IAT_LEN];
  uint32 totalSymbols;
  uint32 totalBlankLogicalFrames;
} BCM_PACKING_ATTRIBUTE ;

typedef struct InmReplyData InmReplyData;
struct InmReplyData {
  InmCounters   inmCounters;
  InmParameters inmParameters;
} BCM_PACKING_ATTRIBUTE ;

typedef InmReplyData   GetLineNeInmCountersMsgRep;
extern  MsgLayout      GetLineNeInmCountersLayout;

/* }}} */
/* {{{ SetLineLogicalFrameConfig service */
#ifndef __LOGICALFRAMECFG_
#define __LOGICALFRAMECFG_

typedef struct LogicalFrameCfg LogicalFrameCfg;
struct LogicalFrameCfg
{
  uint8   TTR;
  uint8   TA;
  uint8   TBUDGET;
  uint8   IDF;
  uint8   TIQ;
  uint8   direction;
  uint16  sfCountApply;
  int8    applyNow;
  uint8   reserved[3];
} BCM_PACKING_ATTRIBUTE ;

#endif  /* __LOGICALFRAMECFG_ */

typedef LogicalFrameCfg  SetLineLogicalFrameConfigMsgReq;
typedef LogicalFrameCfg  SetLineLogicalFrameConfigMsgRep;
extern  MsgLayout        SetLineLogicalFrameConfigLayout;

/* }}} */
/* {{{ SetLineSeltCompatibleMode service */

typedef struct seltCompatibleMode seltCompatibleMode;
struct seltCompatibleMode
{
  uint8   seltCompMode;
} BCM_PACKING_ATTRIBUTE ;

typedef seltCompatibleMode SetLineSeltCompatibleModeMsgReq;
extern  MsgLayout          SetLineSeltCompatibleModeLayout;

/* }}} */
/* {{{ Set/GetPtmBondingConfig */

# define BCM_NB_GHS_TRANSACTIONS 8
# define BCM_DISCOVERY_REG_LEN   6

typedef struct PtmBondingConfig PtmBondingConfig;
struct PtmBondingConfig {
  uint8 enable;          /* set to 1 to enable G.Hs PTM bonding codepoint */
  uint8 nbTransactions;  /* number of CLR/CL transactions to run */
  uint8 transactions[BCM_NB_GHS_TRANSACTIONS];
                         /* list of up to 8 transactions that define how
                            the modem should behave on each subsequent
                            CLR/CL transaction.
                            It defines how the CL should be constructed
                            (bits 0:3)
                            - b0: SPar(2) PME Aggregation Discovery bit
                            - b1: NPar(3) Clear if same bit
                            - b2: Npar(3) remote_discovery_register source:
                                  0 -> copy from CPE value received in last CLR
                                  1 -> copy from SetHsBondingConfiguration message
                            - b3: Spar(2) PME Aggregation bit */
  uint8  discoveryReg[BCM_DISCOVERY_REG_LEN];
                          /* register value to use in NPar3 if needed */
} BCM_PACKING_ATTRIBUTE ;

typedef PtmBondingConfig SetPtmBondingConfigMsgReq;
typedef PtmBondingConfig GetPtmBondingConfigMsgRep;
extern  MsgLayout        SetPtmBondingConfigLayout;
extern  MsgLayout        GetPtmBondingConfigLayout;

/* }}} */
/* {{{ GetPtmBondingRegisters */

typedef struct PtmBondingRegisters PtmBondingRegisters;
struct PtmBondingRegisters {
  uint8  bondingSupport;  /**
                           * bit 0: bonding supported by CPE
                           * bit 1: discoveryRegr was present in last CLR
                           * bit 2: aggregationReg was present in last CLR
                           * bit 3:5: same but for first CLR received in list.
                           * bit 7: reported registers valid - bit is reset when new PTM bonding config is sent
                           *        or is 0 if the transaction was not completed
                           */
  uint8  prevDiscoveryReg[BCM_DISCOVERY_REG_LEN];
  uint8  newDiscoveryReg [BCM_DISCOVERY_REG_LEN];
  uint8  pmeId;           /* Bit7 is set if CPE side has sent its PEM id
                           * according to G998.2 amd2.
                           * In that case, bit 4-0 corresponds to the CPE global line ID
                           */
  uint8  reserved[2];
  uint32 prevAggregationReg;
  uint32 newAggregationReg;
} BCM_PACKING_ATTRIBUTE ;

typedef PtmBondingRegisters GetPtmBondingRegistersMsgRep;
extern  MsgLayout GetPtmBondingRegistersLayout;

/* }}} */
/* {{{ Set/GetAtmInterface */

# define IWF_NB_CHANNELS         8

# define IWF_DROP_ALL_TRAFFIC    0
# define IWF_BRIDGED_LLC_NOFCS   1
# define IWF_BRIDGED_LLC_FCS     2
# define IWF_BRIDGED_VCMUX_NOFCS 3
# define IWF_BRIDGED_VCMUX_FCS   4
# define IWF_ROUTED_LLC          5
# define IWF_ROUTED_VCMUX        6
# define IWF_RAW_AAL5            7 /* upstream only */
# define IWF_RAW_ATM             8 /* upstream only */
# define IWF_PPPOA_SESSION_LLC   9
# define IWF_PPPOA_SESSION_VCMUX 10
# define IWF_DS_INJECTION        11 /* downstream only */
# define IWF_DISABLED            12
# define IWF_AAL5_SDU_FORWARD    13

#define IWF_NAME(iwf) \
  ((iwf) == IWF_DROP_ALL_TRAFFIC       ? "Drop"                 : \
   (iwf) == IWF_BRIDGED_LLC_NOFCS      ? "Bridged LLC NoFCS"    : \
   (iwf) == IWF_BRIDGED_LLC_FCS        ? "Bridged LLC FCS"      : \
   (iwf) == IWF_BRIDGED_VCMUX_NOFCS    ? "Bridged VCmux NoFCS"  : \
   (iwf) == IWF_BRIDGED_VCMUX_FCS      ? "Bridged VCmux FCS"    : \
   (iwf) == IWF_ROUTED_LLC             ? "Routed LLC"           : \
   (iwf) == IWF_ROUTED_VCMUX           ? "Routed VCmux"         : \
   (iwf) == IWF_RAW_AAL5               ? "Raw AAL5"             : \
   (iwf) == IWF_RAW_ATM                ? "Raw ATM"              : \
   (iwf) == IWF_PPPOA_SESSION_LLC      ? "PPPoA Session LLC"    : \
   (iwf) == IWF_PPPOA_SESSION_VCMUX    ? "PPPoA Session VCmux"  : \
   (iwf) == IWF_DS_INJECTION           ? "DS Injection"         : \
   (iwf) == IWF_DISABLED               ? "Disabled"             : \
   "Unknown")

# define IWFLAGS_FWD_MISMATCHED_PROTOCOL  (1U<<0)
# define IWFLAGS_NO_US_CONFIG             (1U<<1)
# define IWFLAGS_NO_DS_CONFIG             (1U<<2)
# define IWFLAGS_NO_CRC_CHECK             (1U<<3)
# define IWFLAGS_NO_US_VLAN_TAG_INSERTION (1U<<4)
# define IWFLAGS_NO_DS_VLAN_TAG_INSERTION (1U<<5)

typedef struct AtmInterfaceConfig AtmInterfaceConfig;
struct AtmInterfaceConfig
{
  uint8 channelIndex;           /* channel idx in case of multi-PVC support */
  uint8 flags;                  /* see IWFLAGS_ definitions above */

  uint8  reservedA;
  uint8  us_interworkingFunction; /* see IWF_... definitions above */
  uint32 us_atmHeader;
  uint32 us_atmHeaderMask;
  uint16 us_channelTCI;
  uint8  us_macDA[6];
  uint8  us_macSA[6];
  uint16 us_sessionId;
  uint16 us_maxAAL5SDUSize;

  uint8  reservedB;
  uint8  ds_interworkingFunction; /* see IWF_... definitions above */
  uint16 ds_channelTCI;
  uint16 ds_channelMask;
  uint32 ds_atmHeader;
} BCM_PACKING_ATTRIBUTE ;

typedef struct ChannelId ChannelId;
struct ChannelId
{
  uint8 channelIndex;
} BCM_PACKING_ATTRIBUTE;

typedef AtmInterfaceConfig    SetLineAtmInterfaceMsgReq;
extern  MsgLayout             SetLineAtmInterfaceLayout;

typedef ChannelId             GetLineAtmInterfaceMsgReq;
typedef AtmInterfaceConfig    GetLineAtmInterfaceMsgRep;
extern  MsgLayout             GetLineAtmInterfaceLayout;

/* }}} */
/* {{{ GetLineIwfCounters */

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
} BCM_PACKING_ATTRIBUTE;

typedef ChannelId   GetLineIwfCountersMsgReq;
typedef IwfCounters GetLineIwfCountersMsgRep;
extern  MsgLayout   GetLineIwfCountersLayout;

/* }}} */
/* {{{ Set/GetIwfInfo */

typedef struct LineIwfInfo LineIwfInfo;
struct LineIwfInfo
{
  uint8  macDA[6];      /* MAC DA (6B) */
  uint8  macSA[6];      /* MAC SA (6B) */
  uint16 usChannelTCI;
  uint16 reserved1;
} BCM_PACKING_ATTRIBUTE;

typedef LineIwfInfo SetLineIwfInfoMsgReq;
extern  MsgLayout   SetLineIwfInfoLayout;

typedef LineIwfInfo GetLineIwfInfoMsgRep;
extern  MsgLayout   GetLineIwfInfoLayout;

/* }}} */
/* {{{ SetLineTxCtrl service */

/* A single byte data controls both TxCtrl and TxAltCtrl signals as follows:
 * bit0: TxCtrl    driven high/low
 * bit1: TxAltCtrl driven high/low
 * bit2: TxCtrl    set in high-Z
 * bit3: TxAltCtrl set in high-Z
 */
typedef uint8     SetLineTxCtrlMsgReq;
extern  MsgLayout SetLineTxCtrlLayout;

/* }}} */
/* {{{ LineSetPilotSequence */

#define NR_OF_PILOT_SEQUENCE_ELEMENTS (512)
#if defined(BCM_FDPS_LENGTH_EXTENSION) || defined(BCM_GFAST_SUPPORT) /* increased size required for Gfast probe sequence extension */
# ifdef BCM_35B_SUPPORT
#define NR_OF_PILOT_SEQUENCE_ELEMENTS_FDPS (5120)
# else
#define NR_OF_PILOT_SEQUENCE_ELEMENTS_FDPS (2560)
# endif
#else
#define NR_OF_PILOT_SEQUENCE_ELEMENTS_FDPS (640)
#endif

#define MINIMUM_DOWNSAMPLING_MASK (7)
#define ES_FORMAT_BIT_POS (7)
typedef struct PilotSequence PilotSequence;
struct PilotSequence
{
  uint8 log2M;   /* log2M contains
                    log2M[2:0]: minimal downsampling of error samples during DS_INIT_2_S
                    log2M[7]: indicates the uses of fixed or floating point during DS_INIT_2_S
                    (0: fixed point (8bits) 1: floating point (6+6+4)) */
  uint8 direction;
  uint8 fdpsMode;
  uint8 reserved;
  uint8 bitsPattern[NR_OF_PILOT_SEQUENCE_ELEMENTS_FDPS/8];
  uint16 lineIdDs;
  uint16 lineIdUs;
  uint8 bMax;
  uint8 reservedB[3];
} BCM_PACKING_ATTRIBUTE;

typedef PilotSequence LineSetPilotSequenceMsgReq;
extern  MsgLayout     LineSetPilotSequenceLayout;

/* }}} */
/* {{{ LineChangePilotSequence */

typedef struct NewPilotSequence NewPilotSequence;
struct NewPilotSequence
{
  uint8 newPilotSequence[NR_OF_PILOT_SEQUENCE_ELEMENTS_FDPS/8];
  uint8 direction;
  uint8 fdpsMode;
  uint8 reserved[2];
};
typedef NewPilotSequence LineChangePilotSequenceMsgReq;
extern MsgLayout         LineChangePilotSequenceLayout;

/* }}} */
/* {{{ LineGetVectoringCounters */

typedef struct VectoringCountersMsg VectoringCountersMsg;
struct VectoringCountersMsg {
  uint32 esDsCounter;
  uint32 esUsCounter;
  uint32 esDsFeCounter;
  uint16 isDsFeValid;
  uint16 reservedA;
  uint32 countDroppedDsEs; /* VDSL only. Count the number of ES dropped
                              because of invalid length */
  uint8  timedOutL3Requests; /* G.fast only */
  uint8  reservedB1[3];
  uint32 reservedB2[2];
} BCM_PACKING_ATTRIBUTE;

typedef VectoringCountersMsg LineGetVectoringCountersMsgRep;
extern MsgLayout             LineGetVectoringCountersLayout;

/* }}} */
/* {{{ LineStartVectorDump */

typedef struct StartVectorDump StartVectorDump;
struct StartVectorDump
{
  uint8 direction;
  uint8 measurementId; /* measurement ID for G.fast (4LSB  will be set as 4 MSB
                          of the lineId if measurementId!=0) */
  int8  lastReportedBand; /* Band from 0 to Nbands-1, -1: stop reporting   */
  uint8 log2M;
  uint8 log2q;            /* G.fast update period */
  uint8 vfType;           /* G.fast VF type - 0: DFT output
                                              1: Error samples */
  uint16 lineId;          /*  The second uint16 field defines the lineId that will be included in the
                              message containing the probe samples. When the message is used to
                              dump error samples on normal vectoring lines, this field,
                              must be set to 0xFFFF (in that case the lineId is configured
                              in the setPilotSequence message). */
  uint8 psLenReporting;
  uint8 flagsA;
  uint8 reserved[6];
} BCM_PACKING_ATTRIBUTE;

typedef struct VectorDumpReply VectorDumpReply;
struct VectorDumpReply
{
  uint16 expectedLogGroup;
  uint8  q;                 /* G.fast updatePeriod */
  uint8  z;                 /* G.fast shiftPeriod */
  uint16 psIndex512;        /* current DS pilot sequence index (512 cycle) */
  uint16 psIndex4N;         /* current DS pilot sequence index (4*N cycle) */
  uint8  bMax;
  uint8  esTypeGfast;       /* Type of format for clipped ES in G.fast mode
                                0: Vdsl format
                                1: G.fast format
                             */
  uint8  reserved[2];
} BCM_PACKING_ATTRIBUTE;

typedef StartVectorDump LineStartVectorDumpMsgReq;
typedef VectorDumpReply LineStartVectorDumpMsgRep;
extern  MsgLayout       LineStartVectorDumpLayout;

/* }}} */
/* {{{ LineGetTxGains */

#define NR_OF_TXGAIN_ELEMENTS (352)

typedef struct TwoByteStartTone TwoByteStartTone;
struct TwoByteStartTone
{
  uint16 startTone;
  uint8 log2M;
  uint8 includeRi;
} BCM_PACKING_ATTRIBUTE ;

typedef struct TxGains TxGains;
struct TxGains
{
  int16  currentPsd;
  uint16 mapValXY;
  uint16 txGain[NR_OF_TXGAIN_ELEMENTS];
} BCM_PACKING_ATTRIBUTE;

typedef TwoByteStartTone LineGetTxGainsMsgReq;
typedef TxGains          LineGetTxGainsMsgRep;
extern  MsgLayout        LineGetTxGainsLayout;

/* }}} */
/* {{{ LineSetPrecoder */

#define MAX_PRECODER_MSG_SIZE 336
#define MAX_LOG2_DOWNSAMPLING 4
typedef struct PrecoderMsg PrecoderMsg;
struct PrecoderMsg
{
  uint8  log2M; /* Downsampling of coefficients by 2^M*/
  uint8  format; /* coefficient format
                    0: 2 bytes per coefficient (6/6/4)
                       exp    = coef[0] bit[3:0]
                       mantIm = coef[0] bit[9:4]
                       mantRe = coef[0] bit[15:10]
                    1: 3 bytes per coefficient (8/8/4)
                       mantRe = coef[0] bit[7:0]
                       mantIm = coef[0] bit[15:8]
                       exp    = coef[1] bit[3:0] */
  uint16 startTone;
  uint16 nTones;
  uint8  xtalkId;
  uint8  reservedA;
  uint16 coef[MAX_PRECODER_MSG_SIZE];
} BCM_PACKING_ATTRIBUTE;
typedef PrecoderMsg      LineSetPrecoderMsgReq;
extern  MsgLayout        LineSetPrecoderLayout;

/* }}} */
/* {{{ LineGetPrecoder */

typedef struct LineGetPrecoderReq LineGetPrecoderReq;
struct LineGetPrecoderReq
{
  uint16 startTone;
  uint16 nTones;
  uint8  xtalkId;
  uint8  reservedA[3];
} BCM_PACKING_ATTRIBUTE;

typedef LineGetPrecoderReq LineGetPrecoderMsgReq;

typedef struct LineGetPrecoderRep LineGetPrecoderRep;
struct LineGetPrecoderRep
{
  uint16 coef[MAX_PRECODER_MSG_SIZE];
} BCM_PACKING_ATTRIBUTE;

typedef LineGetPrecoderRep LineGetPrecoderMsgRep;
extern  MsgLayout       LineGetPrecoderLayout;

/* }}} */
/* {{{ LineRuntimeCommand */

#define RUNTIME_CMD_DATA_LEN (100)
#define RUNTIME_CMD_UNDISABLE_BITSWAP   0
#define RUNTIME_CMD_DISABLE_BITSWAP     1
#define RUNTIME_CMD_VN_MODE             2
#define RUNTIME_CMD_MAX_COMMAND         RUNTIME_CMD_VN_MODE

typedef struct RuntimeCommand RuntimeCommand;
struct RuntimeCommand
{
  uint8 direction;
  uint8 commandId;
  uint8 reserved[2];
  uint8 data[RUNTIME_CMD_DATA_LEN];
} BCM_PACKING_ATTRIBUTE;

typedef RuntimeCommand   LineRuntimeCommandMsgReq;
extern  MsgLayout        LineRuntimeCommandLayout;

/* }}} */
/* {{{ LineGetMimoBandPlan */

typedef struct MimoBandPlan MimoBandPlan;
struct MimoBandPlan {
  BandPlanDescriptor         bandPlanDn;
  BandPlanDescriptor         bandPlanUp;
  VdslMsgToneGroupDescriptor vectBandDs; /*Ds vectored bandplan*/
  uint16                     tone0Offset;
  uint16                     xtcNrOfLowDsTones;
  uint16                     xtcNrOfLowUsTones;
  uint16                     xtcNrOfHighDsTones;
  uint16                     xtcNrOfHighUsTones;
} BCM_PACKING_ATTRIBUTE;

typedef MimoBandPlan LineGetMimoBandPlanMsgRep;
extern  MsgLayout    LineGetMimoBandPlanLayout;

/* }}} */
/* {{{ LineSetVectorCompletionFlag */

/*Completion flag defines*/
#define VECTORING_IDLE_POS              (0)
#define VECTORING_CONT_OPVECTOR_1_0_POS (1)
#define VECTORING_CONT_OPVECTOR_2_1_POS (2)
#define VECTORING_CONT_RPVECTOR_POS     (3)
#define VECTORING_CONT_RPVECTOR2_POS    (4)
#define VECTORING_INIT_FAILURE_POS      (5)
#define VECTORING_WAIT_POS              (7)
#ifdef BCM_VECTORING_LEGACY
#define VECTORING_WAIT_LEG_POS          (11)
#endif /* BCM_VECTORING_LEGACY */
/*Completion flagB defines*/
#define VECTORING_CONT_OPVECTOR_1_1_POS (8)
#define VECTORING_CONT_OPVECTOR_2_0_POS (9)
#define VECTORING_CONT_SWITCH_GFAST_POS (13)
#define VECTORING_CONT_DEACTIVATING_POS (14)
#define VECTORING_STOP_SIGNAL_TX        (15)
/*Completion flagC defines*/
#ifdef BCM_VECTORING_LR
#define VECTORING_LR_WAIT_POS               (16)
#define VECTORING_LR_CONT_OPVECTOR_0_POS    (17)
#define VECTORING_LR_CONT_OPVECTOR_1_POS    (18)
#define VECTORING_LR_CONT_OPVECTOR_1_LR_POS (19)
#define VECTORING_LR_CONT_PILOT_POS         (20)
#define VECTORING_CONT_OPVECTOR_1_1_AFE_CHANGE_POS (21)
#endif
/* Completion flag defines for G.fast (vceBit=1 in this case)*/
#define VECTORING_FAST_WAIT_POS         (15)

typedef struct VectorCompletion VectorCompletion;
struct VectorCompletion
{
  uint8 flag;                 /* 0-3 */
  uint8 flagB;
  uint8 flagC;
  uint8 reservedFlag;
  uint8 reserved[2];
  int16 delay;
  uint16 forcedDelay;
#ifdef BCM_VECTORING_FAST_SUPPORT
  uint8 vceBit;
  uint8 reservedA;
  uint16 duration_cd_1_1;
  uint16 reservedB;
#endif
} BCM_PACKING_ATTRIBUTE ;

typedef struct VCReply VCReply;
struct VCReply
{
  uint16 realDelay;
} BCM_PACKING_ATTRIBUTE ;

typedef VectorCompletion          LineSetVectorCompletionFlagMsgReq;
typedef VCReply                   LineSetVectorCompletionFlagMsgRep;
extern  MsgLayout                 LineSetVectorCompletionFlagLayout;


/* }}} */
/* {{{ GetPrecoderGains*/

typedef struct OneByteMimoType OneByteMimoType;
struct OneByteMimoType
{
  uint8 mimoType;                 /* 0-3 */
} BCM_PACKING_ATTRIBUTE ;

typedef struct PrecoderGains PrecoderGains;
struct PrecoderGains
{
  uint32 gainIn;
  uint32 gainOut;
} BCM_PACKING_ATTRIBUTE;

typedef OneByteMimoType GetPrecoderGainsMsgReq;
typedef PrecoderGains   GetPrecoderGainsMsgRep;
extern  MsgLayout       GetPrecoderGainsLayout;

/* }}} */
/* {{{ LineResetPrecoder service */

extern MsgLayout    LineResetPrecoderLayout;

/* }}} */
/* {{{ LineGetFeq */

typedef struct FeqMsg FeqMsg;
struct FeqMsg
{
  uint16 startTone;
  uint8  log2M;
  uint8  minSnrDB; /* Forced the reported FEQ to (mantRe=0,mantIm=0,exp=0x7F) for tone with SNR below that value.
                      This is done only in showtime.
                      0 is a special value to disable all filtering */
} BCM_PACKING_ATTRIBUTE;

#define TONES_FEQ_SEGMENT (128)
typedef struct FeqSegment FeqSegment;
struct FeqSegment
{
  int32 refPsd;
  int16 mant[2*TONES_FEQ_SEGMENT];
  int8  exp[TONES_FEQ_SEGMENT];
} BCM_PACKING_ATTRIBUTE;

typedef FeqMsg        LineGetFeqMsgReq;
typedef FeqSegment    LineGetFeqMsgRep;
extern  MsgLayout     LineGetFeqLayout;

/* }}} */
/* {{{ AfeDebugCommand */

# define AFE_ADDRESS_START 0
# ifdef BCM_VDSL_ON_6545X
#  define AFE_ADDRESS_STOP 0x7ff
# else
#  define AFE_ADDRESS_STOP 0x5ff
# endif

int afeAddressExist(uint16 address, int chip);
int afeAddressMask(uint16 address, int chip);

typedef struct AfeDebugCommandMsg AfeDebugCommandMsg;
struct AfeDebugCommandMsg
{
  uint8 writeIndicator;         /* 1 if doing a write, 0 for a read */
  uint8 reserved;
  uint16 address;               /* AFE Register address */
  uint16 data;                  /* req value is don't care in case of a read
                                 * rep value is don't care in case of a write */
} BCM_PACKING_ATTRIBUTE;

typedef AfeDebugCommandMsg AfeDebugCommandMsgReq;
typedef AfeDebugCommandMsg AfeDebugCommandMsgRep;
extern  MsgLayout AfeDebugCommandLayout;

/* }}} */
/* {{{ LineSetMimoGains */

typedef struct SetPrecoderGainsMsg SetPrecoderGainsMsg;
struct SetPrecoderGainsMsg
{
  uint8  direction;
  uint8  reserved[3];
  uint32 gainIn;
  uint32 gainOut;
} BCM_PACKING_ATTRIBUTE;

typedef SetPrecoderGainsMsg LineSetMimoGainsMsgReq;
extern  MsgLayout           LineSetMimoGainsLayout;

/* }}} */
/* {{{ LineGetFeqOutput */

typedef struct FeqOutputSegmentMsg FeqOutputSegmentMsg;
struct FeqOutputSegmentMsg
{
  uint8 partId;
  uint8 getMaxPower;
} BCM_PACKING_ATTRIBUTE;

#define TONES_FEQ_OUTPUT_SEGMENT 96

typedef struct FeqOutputSegment FeqOutputSegment;
struct FeqOutputSegment
{
  uint8 testStatus; /* See SignalMeasurement */
  uint8 reserved;
  int16 feqOutput[2*TONES_FEQ_OUTPUT_SEGMENT];
} BCM_PACKING_ATTRIBUTE;

typedef FeqOutputSegmentMsg LineGetFeqOutputMsgReq;
typedef FeqOutputSegment LineGetFeqOutputMsgRep;
extern  MsgLayout        LineGetFeqOutputLayout;

/* }}} */
/* {{{ LineSwitchStateSilent */

typedef struct SwitchStateSilent SwitchStateSilent;
struct SwitchStateSilent
{
  uint32 state;
} BCM_PACKING_ATTRIBUTE;

typedef SwitchStateSilent LineSwitchStateSilentMsgReq;
extern  MsgLayout         LineSwitchStateSilentLayout;

/* }}} */
/* {{{ LineInformVectConvCompl */

typedef struct InformVectConvergenceCompleted InformVectConvergenceCompleted;
struct InformVectConvergenceCompleted{
  uint8 reserved;
} BCM_PACKING_ATTRIBUTE;
typedef InformVectConvergenceCompleted LineInformVectConvComplMsgReq;
extern MsgLayout LineInformVectConvComplLayout;

/* }}} */
/* {{{ GetLineFeOamCounters */

typedef struct FeRawOamCounters FeRawOamCounters;
struct FeRawOamCounters
{
  uint16  ES;
  uint16  SES;
  uint16  LOSS;
  uint16  FECS;
  uint16  UAS;
  uint8   upTime;               /* upTime when local counters were last updated */
  uint8   reserved;
} BCM_PACKING_ATTRIBUTE ;

typedef FeRawOamCounters GetLineFeOamCountersMsgRep;
extern  MsgLayout        GetLineFeOamCountersLayout;

/* }}} */
/* {{{ LineChangeMds */

typedef struct DtaRequest DtaRequest;
struct DtaRequest {
  uint8   Mds;
  uint8   initialCountDown; /* Initial TDD countdown value. Allowed range [6 15], recommendended value 15. */
  uint8   reserved[2];
} BCM_PACKING_ATTRIBUTE ;

typedef DtaRequest       ChangeLineMdsMsgReq;
extern  MsgLayout        ChangeLineMdsLayout;

/* }}} */
/* {{{ SetLineDtaConfig */

typedef struct DtaConfig DtaConfig;
struct DtaConfig {
  /* The first 8 parameters below apply only in case of iDTA (annexX), and are not applicable to cDTA (annex D).
   * In case of cDTA, the values from the GlobalDtaConfig (see bcm_hmiCoreMsg.h) apply.
   */
  uint8  hsMds;          /* handshake Mds value : number of downstream symbol positions in a TDD frame 
                          * requested by the DRA when a line is training or when conflicting traffic 
                          * requirements are observed */
  uint8  dtaMds;         /* preferred Mds value (DTA_Mds) provides the number of downstream symbol positions 
                          * in a TDD frame requested by the DRA in case of conflicting US/DS BW usage. */
  uint8  dtaSmax;        /* maximum step size for DTA changes (DTA_SMax) is the maximum change in Mds 
                          * requested by the DRA for a single DTA update */
  uint8  dtaCountDown;   /* TDD countdown value for DTA. Range [6 15].
                          * It should be larger than maxDelay for maximum robustness */
  uint8  draEnabled;     /* enable autonomous DTA based on internal traffic monitoring */
  uint8  noiControlEnabled;  /* bit 0 enables autonomous DS TTR change based on internal traffic monitoring
                              * bit 1 also enables US TTR change.
			      * bit 2 enables a autonomous return to full frame in DS (over-ruling the externally
			      *       configured TTR value) upon generation of Xoff on the G999.1 interface. */
  uint16 dtaMinTime;     /* minimum time (in 0.01 second unit) between two successive DTA. Minimum value is 3 (30ms) */
  uint16 updatePeriod;   /* The traffic is monitored over updatePeriod TDD frames before evaluating the DTA trigger.
			  * Valid range is [ceil(dtaMinTime*40/3), 2^16-1]. */
  uint8  draTestMode;    /* if set the line requests random Mds changes within its acceptable Mds range. */
  uint8  reservedB;
  /* The parameters below apply for both iDTA and cDTA */
  uint8  dtaMinMds;
  uint8  dtaMaxMds;
  uint8  rateThreshold;  /* DTA is triggered when the DPR is larger than rateThreshold * NDR is either direction.
                            Expressed in 1/256th. Recommended value is 243 (95%) */
  uint8  annexXmode;     /* Exclusively used during G.hs. Has no effect if changed in showtime.
                            iDTA : 0->AnnexX_FORBIDDEN / 1->AnnexX_ALLOWED / 2->AnnexX_FORCED */
  uint8  iDtaAllowed;    /* Exclusively used during Gfast training. Has no effect if changed in showtime. */
  uint8  annexDmode;     /* cDTA : 0->AnnexD_FORBIDDEN / 1->AnnexD_ALLOWED / 2->AnnexD_FORCED */
  uint8  cDtaAllowed;    /* Exclusively used during Gfast training. Has no effect if changed in showtime. */
  uint8  reservedA[5];
  uint32 dtaETRmin[2];   /* Used to bound Mds for US/DS directions */
  uint32 dtaNDRmax[2];   /* Used to bound Mds for US/DS directions */
  uint32 dtaCounter;     /* Counter of applied DTA changes */
  uint8  lastMds[8];     /* Reports the last 8 transitions */
  uint8  reservedC[8];
} BCM_PACKING_ATTRIBUTE ;

typedef DtaConfig        SetLineDtaConfigMsgReq;
extern  MsgLayout        SetLineDtaConfigLayout;

/* }}} */
/* {{{ GetLineDtaConfig */

typedef DtaConfig        GetLineDtaConfigMsgRep;
extern  MsgLayout        GetLineDtaConfigLayout;

/* }}} */
#endif
