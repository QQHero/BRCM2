/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*******************************************************************************
 *              Copyright 2000,2001,2002,2003 Element 14 Inc                   *
 *                                                                             *
 * This material is the confidential trade secret and proprietary information  *
 * of Element 14 Inc.  It may not be reproduced, used, sold or transferred to  *
 * any third party  without the prior written  consent of Element 14 Inc. All  *
 * rights reserved.                                                            *
 *                                                                             *
 ******************************************************************************/
/* 
   $Id: CoreMessageDef.h,v 1.393 2015/01/27 09:16:25 kdmaeyer Exp $
*/

#ifndef COREMESSAGEDEF_H
#define COREMESSAGEDEF_H

#ifndef ATU_C
#define ATU_C
#endif
#ifndef VDSL
#define VDSL
#endif

#define NUMBER_OF_CORES         (1)
#define MAX_PTM_PRIORITY_NUMBER (2)
#define LINES_PER_DEV           (1)

#ifndef BCM_HMI_CORE_MSG_H
#include "bcm_hmiCoreMsg.h"
#endif

#define CORE_BASE_APP_ID 0x100
#define VERSION_STRING_LENGTH 64 /* bytes */

/***********************************************************************/
/***********    Santorini Configuration Services    ********************/
/***********************************************************************/

#define CORE_INVALID_POWER_SETTING          1
#define CORE_NO_BEARER_ENABLED              2
#define CORE_DUAL_LATENCY_NOT_SUPPORTED     3
#define CORE_UTOPIA_ADDRESS_OUT_OF_RANGE    4
#define CORE_UTOPIA_ADDRESS_USED_TWICE      5
#define CORE_UTOPIA_INBAND_ADDRESS_USED     6
#define CORE_INVALID_LDSUPPLY_SETTING       7
#define CORE_INVALID_SEGMENT_SETTING        8
#define CORE_INVALID_GFAST_TDD_SETTING      9

#define CORE_NO_DEFAULT_SEGMENT_SETTING     10
#define CORE_INTERFACE_NOT_SUPPORTED        11
#define CORE_INVALID_ERR_SAMPLES_PHY_ADDR   12
#define CORE_INVALID_VECTORING_SETTING      13
#define CORE_INVALID_VF_MODE                14

#define UTOPIA_ADDR_DISABLE (-1)
#define UTOPIA_ADDR_RGMII_0 (-3) // EXT_PIF_0
#define UTOPIA_ADDR_RGMII_1 (-4) // EXT_PIF_1


/* SantoriniSetConfig must be sent only once before any line configuration can be done 
   multiples config of some of the fields possible if some bit is set */


#define validTxPowerCalibrationAndHwMaxOutputPowerAdsl(txGain,hwMaxOutputPower) ((hwMaxOutputPower-txGain) <= (DAC_VRMS_ADSL_DB(afeGetChipVersion(),afeGetChipRevision()) + 10*256))
#define validTxPowerCalibrationAndHwMaxOutputPowerVdsl(txGain,hwMaxOutputPower) ((hwMaxOutputPower-txGain) <= (DAC_VRMS_VDSL_DB + 10*256))

#define ATM_53_BYTE_MODE        0x40
#define ASSYMETRIC_ATM_LAYOUT   0x02
#define ATM_PADD_AT_END         0x80


#define SPREAD_US_TRAFFIC           (1<<0) /* b0 in pl2Flags */
#define ATM_ETHERNET_IWF            (1<<2) /* b2 in pl2Flags */
#define PTM_NO_RXERR                (1<<4) /* b4 in pl2Flags */
#define ATM_CORRECT_SHORT_AAL5      (1<<5) /* b5 in pl2Flags */
#define G9991_DUMMY_EOP_SHOWT_ENTRY (1<<7) /* b7 in pl2Flags */

#define CORECONFIG_FLAGS_SPREAD_GFAST_TX_TASKS  (1<<0)
#define CORECONFIG_FLAGS_SNOOP_ATM_F4F5         (1<<5)
#define CORECONFIG_FLAGS_ALSO_PAD_BOE           (1<<6)
#define CORECONFIG_FLAGS_DISABLE_PORT_CHECK     (1<<7)



// flags used in enableUtopiaControl
#define PBUF_FLUSH_WHEN_IDLE             (1<<0)
#define PBUF_ENABLE_ONLY_DURING_SHOWTIME (1<<1)
#define TRAFFIC_VIA_MII                  (1<<2)
#define PPF_3SYMBOL_THRESHOLD            (1<<3)
#define PBUF_INBAND_DO_NOTIFCATION       (1<<4)
#define USE_MAXIMUM_DS_BUFFER            (1<<5)
#define ENABLE_GFAST_JUMBO_FRAME_SUPPORT (1<<6)






#define ES_ROUTING_MODE_MASK             (0x3<<5)
#define ES_ROUTING_MODE_ALL_INBAND       (0x0<<5)
#define ES_ROUTING_MODE_US_OUTBAND       (0x1<<5)

// flags applicable to xtcSerdesFlags 
#define BACKPRESSURE_ON_10GKR              (1<<0)
#define SWAP_10GKR_EMBEDDED_PRIORITIES     (1<<1)
#define ADD_FCS_ON_ES                      (1<<2)
#define ADD_FCS_ON_MGMT                    (1<<3)
#define VECTORING_10GKR_RELAY              (1<<4)
#define ES_ROUTING_MODE_INJECT_10Gkr     (0x2<<5)
#define ES_ROUTING_MODE_TR               (0x3<<5)
#if defined(GFAST_SUPPORT)
#define ES_ROUTING_MODE_LAST  ES_ROUTING_MODE_TR
#else
#define ES_ROUTING_MODE_LAST  ES_ROUTING_MODE_INJECT_10Gkr
#endif
#define ADD_FEC_ON_10GKR                   (1<<7)

#define ES_GROOMING                        (1<<8)



#define ES_GET_ROUTING_MODE(CONFIG_FIELD) (CONFIG_FIELD & ES_ROUTING_MODE_MASK) 

#define ES_ROUTING_MODE_RAW(CONFIG_FIELD,MODE) (ES_GET_ROUTING_MODE(CONFIG_FIELD) == MODE)

#ifdef ATU_C
// we will always output to TR, so all selection code should select that.
#define ES_ROUTING_MODE(CONFIG_FIELD,MODE) (ES_ROUTING_MODE_RAW(ES_ROUTING_MODE_TR,MODE))
#define NUM_OOB_ADDRESSES (1)
#else
#define NUM_OOB_ADDRESSES (0)
#endif

#define ES_ROUTING_MODE_VDSL ES_ROUTING_MODE

#if 1
#define SantoriniConfig DeviceConfig
#else
typedef struct SantoriniConfig SantoriniConfig;
struct SantoriniConfig
{
  LineConfig           lineConfig[LINES_PER_DEV];  /* line configuration for NUMBER_OF_CORES cores */

  uint8                enableUtopiaControl;        /* bits[1:0]: define how the UL2/PL2 is enabled:
                                                                 0 -> Utopia is enabled as soon device is configured
                                                                 1 -> Tx utopia is flushed continuously.
                                                                 2 -> Utopia is enabled only when the line is in showtime
                                                      bits[2]  : traffic goes via GMII
                                                      bits[3]  : set to enable 3symbol threshold for ppf.
                                                                  >= bcm65100 only.
                                                      bits[4]  : set to enable XOFF notification packet in D0 mode.
                                                      bits[5]  : set to use maximum available DS buffer before XOFF. 
                                                      bits[6]  : set to enable support of jumbo frame in Gfast.

                                                   */
  uint8                globalTpsFlags;             /* bit 0: 1 means dummy PTM byte is on position
                                                             2, otherwise it is on position 5
                                                      bit 1: set to 1 indicate asymmetric ATM layout mode
                                                             if this bit is not set,
                                                                bits [6,7] define the ATM layout for US and DS.
                                                             when this bit is set,
                                                                bits [6,7] define the ATM layout for DS only.
                                                                US layout will be defined as [!valueofbit6,7]
                                                      bit 2: set to 1 indicate PTM CRC16 is
                                                             completely handled by the network processor
                                                      bit 3: set to 1 indicate the PTM CRC16 need
                                                             to be verified by the DSP. Note this bit is
                                                             not used when bit2 is set to 1.
                                                      bit 4: set to 1 indicate the PTM CRC16 need
                                                             to be verified by the NPU. Note this bit is
                                                             not used when bit2 is set to 1.
                                                      bit 5: set to 1 for 6 header/48 data byte PTM over
                                                             utopia format:
                                                             H0 PAD PAD PAD PAD PAD P0 P1 P2 ... P48
                                                      bit 6: set to 1 to indicate that ATM cells are received in a 53 byte format:
                                                             H0 H1 H2 H3 HEC P0 P1 P2 ... P48
                                                             When not set ATM cells are received in a 54 byte format:
                                                             H0 H1 H2 H3 HEC PAD P0 P1 ... P48
                                                      bit 7: used in conjunction with bit 6 equal to 0:
                                                             set to 0 to indicate the position of the padding byte is at byte 6
                                                             set to 1 to indicate the position of the padding byte is at byte 54
                                                   */

  uint8                flagz;                      /*  CALLED flagz iso flags fro easy search of usage through code
                                                       bit7 disable sanity check on interface port configuration
                                                       bit6 indicates that us g999.1 fragments should also be padded in boe mode.
                                                       bit5 indiicates that ATM F4/F5 cells should be snooped in US
                                                       bit0 disable tx clock delay on RGMII bonding interfaces. only on bcm651xx
                                                   */
  uint8                reservedA;
  uint8                pl2Flags;                   /* set bit 0 to enable spreading of US traffic
                                                    * set bit 1 to enable odd parity check on PL2 bus
                                                    * set bit 2 to enable ATM to Ethernet interworking
                                                    * set bit 3 to enable fused ATM/PL2 descriptors in DS
                                                    * set bit 4 to prevent the RX_ERR signal to be generated in US. only applies to bcm651xx
                                                    * set bit 5 to enable aal5 short cell correction in US
                                                    * set bit 6 to pull traffic in case of bonding backpressure.
                                                    * set bit 7 to send 1 dummy packet with g999.1 eop set each time when a line enters showtime*/

  uint8                globalDevId;                /* indicate a global device id number. */
  uint16               extraXoffOverhang;          /* number of bytes that are prequeued in the NPU and cannot be stopped by g999.1 xoff or std.eth pause  */
  uint16               rxSegLen;                   /* max fragment size for US g999.1 fragments */
  int8                 deviceId;                   /* identification of the device on the board. Valid range is 0 to 7.
                                                       Set to -1 to keep booter settings */
                                                   /* TODO : -1 handling */
  uint8                supportVectoring;           /* On TACANA platform:
                                                        Bit 0    : 0:No vectoring support/1: vectoring support
                                                      On KOS platform:
                                                        Bit 0    : 0:No VDSL  vectoring support/1: VDSL  vectoring support
                                                        Bit 1    : 0:No GFAST vectoring support/1: GFAST vectoring support
                                                        Bit 6    : 1 / use internal VCE (BLV up to 16L/4dev)
                                                        Bit 7    : 1 / use internal VCE for 4 line CLV
                                                   */
  fix8_8               txGainRef;                  /* transmit gain for the 0 dB reference of the txGainDescriptor */
  TxGainDescriptor     txGainDescriptor;
  fix8_8               rxAfeGains[4];              /* mean gain in US0(AnnexA)|US0(AnnexB)|US1(998) */
  RxGainDescriptor     rxGainDescriptor;           /* 8 breakpoints containing slope of RX filters */
  fix8_8               agcGains[7];
  LineDriverPowerModes lineDriverPowerModes;
  AfeConfig            afeConfig;
  ufix0_8              txPowerOptimizationIlv;     /* range [0:255], ie [0:1[ dB */
  uint8                afeTxCtrlFlags;             /* bit 0: user control for txCtrl
                                                    * bit 1: user control for txAltCtrl
                                                    * bit 2: enable new lineDriverPowerModes (reduced power and ADSL line test)
                                                    * bit 3: for programmable peak detect
                                                    * bit 4: 0 for [i, i+1], 1 for [i, i+2] pairing
                                                    * bit 5: set to 1 for FW6430-5555 workaround
                                                    */
  int16                rxCrestFactor;

  fix8_8               hwMaxOutputPowerGfast;
  fix8_8               hwMaxOutputPowerVdsl;
  fix8_8               hwMaxOutputPowerAdsl;

  uint16               trafficGroupingMask;      /* 65400:10G only: identifies the bits in the utopiaAddr
                                                  *    which extract a unique traffic group number.
                                                  * no cascading : the first half of the lines (utiopiaAddr[0..N/2[)
                                                       must be in a group other than the second half 
                                                       (utopiaAddr[N/2..N[) of the lines. 
                                                  * cascading : each device has its own traffic group.
                                                       The trafficGroupingMask must be set to the same
                                                       value across the devices, and is used to decide
                                                       if traffic belongs to the current or a subtended
                                                       device */

  uint8                afeVrefMode;                /* Only defined on BCM654XX :
                                                    * bit0: set to 1 to enable the AFE interface towards a BCM652XX to 
                                                    *       manage lines in Gfast/VDSL overlay mode (VDSL lines being 
                                                    *       serviced from the BCM652XX device).
                                                    * bit7-1: used on special AFE validation test build.
                                                    */
  uint8                noInternalNoise;            /* avoid using internal/external noise differentiation */
  int16                powerThreshold;		           /* Txpower value below which the line driver will operate
									                                              in a reduced power mode */
  uint8                shortLoopDeployment;        /* if set, the detection threshold for VDSL signals will be
                                                      increased, as to be more robust wrt Xtalk, at the expense
                                                      of reducing the reach in no-noise cases
                                                    */
  uint8                fireDemoLineNumber;         /* line number on which the demo key can be used to enable PhyR */
  uint8                noHwMaxOutputPowerCheck;
  uint8                ISDNhardware;
  uint8                enablesDivOcheck;
  uint8                ldType;                     /* bit 0-3: 1: BCM_65040
                                                               2: ISL 1561
                                                               3: THS6225
                                                      bit[4:7] for crest reduction on BCM65800 (enabled if !=0) */
#define PARITY_FORCE_DISPLAY 16     // force display information about parity information
#define PARITY_DISPLAY        8     // display information about parity information
#define PARITY_AUTO_REBOOT    2     // reboot in case parity error cannot be corrected
#define PARITY_AUTO_RECOVERY  1     // automatically recover parity errors when detected
  uint8                programCheckOption;        /* bit 0 -> enable autocorrection,
                                                     bit 1 -> enable reboot
                                                     bit 2 -> reserved avoid problems with earlier version
                                                     bit 3 -> display information in the FW strings
                                                    */
  uint8                initSignalCrestControl[2];  /* high crest control value:
                                                    * 0 means use build default
                                                    * 1-255 actual crest control
                                                    * [0] is for DS starting at tone 32 (annexA)
                                                    * [1] is for DS starting at tone 64 (annexB/M/J)
                                                    */
  uint8                overwriteCoreConfig;        /* If set to 1 it`s possible to change the Core Config
                                                    * This is intentionally not documented in the HMI spec */

  uint8                ESmaskingMode;              /* !!! only bit[0] is usable because
                                                     - this field is combined with the OAMstatisticsCountMode
                                                       through a shift by 2, oring
                                                     - bit 1 of the or-ed field is used for LPR state
                                                   */

  uint8                OAMstatisticsCountMode;     /* bits [1..0]: indicate how to increment the ES,SES,UAS when not in showtime.
                                                      bit 2      : set to 1 changes the meaning of the fullInitCount to re-init count in HMI 10.1
                                                                   compatibility mode. Bit 2 should not be used with HMI 10.2
                                                      bit 3      : 1: include a subset of the PTM inside the LinePeriodCounters
                                                      bit 4      : indicates that Persistent LOS should not be used
                                                      bit 5      : lastRetrainCause 4 LSB are masked when the exception is triggered from showtime
                                                      bit 6      : controls minEFTR format as per g998.4/corr4
                                                      bit 7      : controls LOS failure setting at startup/goOutShowtimePM. If 0, LOS failure set.
                                                   */
  uint8                enableHPIreplyInterrupt;    /* indicate whether an interrupt should be generated when an HPI reply is posted */
  uint8                autoMsgConfig;              /* 0=AAL5 autoMsgs / 1=IP autoMsgs / 2=no autoMsgs*/
  uint16               autoMsgDestPort;            /* LE byte order*/
  uint32               autoMsgDestIPaddr;          /* LE byte order*/

  int32                controlUtopiaAddr;          /* utopia port used for in-band control
                                                        -1 disable in-band control,
                                                        -2 keep the current port used for in-band download
                                                   */
  uint8                repetitionRate;

  uint8                autoMsgATMheader[5];
  uint8                reservedD[2];
  uint32               autonomousRef;
#ifdef ATU_R
  uint8                ntrGpio_enable;
  uint8                ntrGpio_address; //0-7 -> GPIOA[0-7],8-15 GPIOB[0-7] etc
  uint8                ppsGpio_enable;
  uint8                ppsGpio_address;
  uint8                ntrGpio_externalControl;     // GPIOC[0] (output) -> indicate DSP ready to handle NTR, GPIOC[1] (input) -> DSP allowed to use ntr DCXO 
  uint8                reservedCpe[11];
#else
  uint8                phyRkey[16];
#endif
  uint16               exclusivityFeature;         /* Reserved to activate ALU exclusivity features */
  fix8_8               hwMaxOutputPowerHighBW;
  uint8                reInitTimeOut;              /* Timeout for CPE detection in [s] */
  uint8                rxSegLenMin;                /* minimum g999.1 US fragment length.
                                                      0 -> no penultimate mode.
                                                      <> 0 -> indidcates minimum fragment length
                                                    */
  uint8                gfast_tdd_params_Mds;       /* US/DS split, valid range [2,32] */
  uint8                gfast_tdd_params_Mf;        /* currently hardwired to 36       */
  uint8                gfast_tdd_params_Msf;       /* currently hardwired to 8        */
  uint8                gfast_cyclic_extension_m;   /* Lcp = gfast_cyclic_extension_m*N/64. currently hardwired to 10 */

  uint16                xtcSerdesFlags;            /* bit 0 : set to 1 in order to disable 10GKr XOFF/XON flow control.
                                                              If set to 0, DSP will indicate pause on echan.
                                                      bit 1 : set to 1 in order to set ES on prio0(high) and MGMT on prio1(low)
                                                      bit 2 : set to 1 in to enable FCS generation on ES   relay from NPU to 10Gkr
                                                      bit 3 : set to 1 in to enable FCS generation on MGMT relay from NPU to 10Gkr
                                                      bit 4 : set to indicate the DSP should relay ES and MGMT packets between NPU and 10GKR.
                                                      Bit 5..6 : error sample routing mode.
                                                                 For VDSL only product :
                                                                   0 : indicates all error samples are send inband with traffic.
                                                                   1 : indicates the US ES should be sent on separate channel, indicated
                                                                       by errorSamplesAddrOOB. DS ES are still inband. This mode is usefull
                                                                       for external bonding.
                                                                   2 : set to indicate US and DS ES are directly injected into the 10Gkr xtc
                                                                       serdes embedded channel.
                                                                       NOTE : this mode is only available on 652xx.
                                                                   3 : set to indicate US and DS ES are directly send to a relay/vce core via the tokenring buffer.
                                                                       NOTE : this mode is only available for gfast chipset
                                                                 For GFAST product :
                                                                   0 : not supported.
                                                                   1 : All ES are sent on separate channel, indicated by errorSamplesAddrOOB[X]
                                                                       In case of 4 line DSP, errorSamplesAddrOOB[>=4] will be ignored.
                                                                       In case of 6 line DSP, errorSamplesAddrOOB[>=6] will be ignored.
                                                                 2,3 : same as VDSL
                                                      Bit 8 : for GFAST product only. set to enable ES aggregation / grooming.
                                                      more info in section 12, global configuration.
                                                   */

  uint16               errorSamplesAddrOOB[12];    /* VDSL platform : phy addresses for out of band error samples per core
                                                      GFAST platform : phy addresses for out of band error samples per line.
                                                      in case of s3mii :
                                                            bits 0..7 indicate which s3mii link to use. 
                                                                      value is incremented by 1 per core.
                                                      for g999.1 link types
                                                            bits 0..7 indicate which sid to use. 
                                                            bits 8..9 indicate priority and bearer bit setting
                                                            bcm65200 :
                                                            bit  14..15 indicate which link to use. 
                                                            other :
                                                            bit  15 indicate which link to use. 
                                                      for YST : errorSamplesAddrOOB[1..11] is unused. errorSamplesAddrOOB[0] for all lines.
                                                   */

  uint16               xtcSerdesRelayAddr[2];      /* phy addresses for the relay of MGMT and ES on xtc serdes link
                                                      format is identical to errorSamplesAddrOOB
                                                    */

  uint16               gfastVecFbVlan;             /* VLAN for VF packets in g.fast */
  uint8                progGainXtcTx;              /* Programmable shift for Tx VDSL XTC gains
                                                      bit[3:0] =  right shift on tx gainIn.  Valid values 0:5.
                                                      bit[7:4] =  left shift  on tx gainOut. Valid values 0:10.
                                                   */
  uint8                progGainXtcRx;              /* Programmable shift for Rx VDSL XTC gains
                                                      bit[3:0] =  right shift on rx gainIn.  Valid values 0:5.
                                                      bit[7:4] =  left shift  on rx gainOut. Valid values 0:10.
                                                   */
  uint8                progGainXtcTxGfast;         /* Programmable shift for Tx Gfast XTC gains
                                                      bit[3:0] =  right shift on tx gainIn.  Valid values 0:5.
                                                      bit[7:4] =  left shift  on tx gainOut. Valid values 0:10.
                                                   */
  uint8                progGainXtcRxGfast;         /* Programmable shift for Rx Gfast XTC gains
                                                      bit[3:0] =  right shift on rx gainIn.  Valid values 0:5.
                                                      bit[7:4] =  left shift  on rx gainOut. Valid values 0:10.
                                                   */
  uint8                dtaCompatibleFraming;      /* if set, the DTA compatible framing (RMC positions 2/2) is enabled. Only supported on YST */
  uint8                us10GshaperCfg;            /* Only applicable when the 10G G999.1 traffic interface is enabled. 
                                                     Configures the 10G US shaper : the US traffic is limited to a rate equals to 
                                                     N/M * 10Gb/s, with:
                                                     bits [7:4] N
                                                     bits [3:0] M
                                                     Note that value 0 is interpreted as 16.
                                                     Default value us10GshaperCfg = 0x0 sets N=16/M=16 and disables the US 10G shaper.
                                                     Configuring us10GshaperCfg = 0x12 limits the US rate on the inteface to 5Gb/s.
                                                  */
  uint8                coax;                      /* deployment over COAX - note that all power config & reporting assumes 75Ohm ref impedance */
  uint8                overwriteStdProfilePower;  /* if bit[0] is set, the standard profile power limit for all profiles up to 17a is overwritten with 20.5dBm */
                                                  /* if bit[1] is set, the standard 35b profile power limit is overwritten with 20.5dBm */
  uint8                lrGfast;                   /* if set, the long reach Gfast timing is selected (TG2 > 11.2us), to accomodate longer loop lengths */
  uint8                reservedC[5];              /* reserved for future usage */
};
#endif

#define AAL5_AUTOMSG 0
#define IP_AUTOMSG   1
#define NO_AUTOMSG   2

typedef struct SoftwareVersion SoftwareVersion;
struct SoftwareVersion {
  uint32 hwChipId;
  uint16 fwMajorVersion;
  uint8  fwMinorVersion;
  uint8  fwFixVersion;
  uint16 hmiMajorVersion;
  uint16 hmiMinorVersion;
  uint8  versionString[VERSION_STRING_LENGTH];
  uint8  nrOfLinesSupportedInFirmwareVersion;
  uint8  afeChipId; /* 0x05 for BCM6505
                       0x06 for BCM6506
                       0x26 for BCM6526
                       0x21 for BCM6421 */                     
  uint8  ldTypeMap; /* ldType map for lines on this core */
  uint8  vcxoVersion;           /* ATU_R only */
  uint8  buildType; /* report SW_TYPE so SDK can determine capabilities */
  uint8  reserved[3];
};

typedef struct SantReadSmemReq SantReadSmemReq;
struct SantReadSmemReq
{
  uint32 start;
  uint32 length;
};

/*Shortcut: declared like this to make the matlab smemextract test script easy.
 *The size of SantReadSmemReply packets depend on the length specified in the
 *request*/
typedef struct SantReadSmemReply SantReadSmemReply;
struct SantReadSmemReply
{
  uint8 smem[256];
};

#define SANT_SET_CONFIG 0x0001
HPI_SERV_DEF(SantoriniSetConfig, SANT_SET_CONFIG, SantoriniConfig, DUMMY)

#define SANT_GET_CONFIG 0x0002
HPI_SERV_DEF(SantoriniGetConfig, SANT_GET_CONFIG, DUMMY, SantoriniConfig)

#define SANT_GET_LINE_ISR 0x0003
HPI_SERV_DEF(SantoriniGetLineISR, SANT_GET_LINE_ISR, DUMMY, LineInterrupt)

#define SANT_CHANGE_LINE_IMR 0x0004
HPI_SERV_DEF(SantoriniChangeLineIMR, SANT_CHANGE_LINE_IMR, SantChangeLineIMR, DUMMY)

#define SANT_RESET_DEVICE 0x0005     
HPI_SERV_DEF(SantoriniDeviceReset, SANT_RESET_DEVICE, DUMMY, DUMMY)

#define SANT_DEV_MIN_EXCEPT 0x0006
HPI_SERV_DEF(SantDevMinExcept, SANT_DEV_MIN_EXCEPT, DUMMY, DUMMY)

#define SANT_GET_SOFTWARE_VERSION 0x0007
HPI_SERV_DEF(SantGetSoftwareVersion, SANT_GET_SOFTWARE_VERSION, DUMMY, SoftwareVersion)

#define SANT_WRITE_GP_OUT 0x0008
HPI_SERV_DEF(SantoriniWriteGPout, SANT_WRITE_GP_OUT, GPoutData, DUMMY)

#define SANT_READ_GP_IN 0x0009
HPI_SERV_DEF(SantoriniReadGPin, SANT_READ_GP_IN, DUMMY, uint32)

#define SANT_READ_SMEM 0x000A
HPI_SERV_DEF(SantReadSmem, SANT_READ_SMEM, SantReadSmemReq, SantReadSmemReply)
  
typedef struct SetOAMperiodElapsedTime SetOAMperiodElapsedTime;
struct SetOAMperiodElapsedTime
{
  uint32 elapsedTime;
  uint32 numberOfSecIn15min;
  uint16 numberOf15minIn24hour;
  uint16 periodInterruptOffset;
};

#define SANT_SET_OAM_PERIOD_ELAPSED_TIME 0x000C
HPI_SERV_DEF(SetOAMperiodElapsedTime, SANT_SET_OAM_PERIOD_ELAPSED_TIME, SetOAMperiodElapsedTime, DUMMY)


typedef struct OEMid OEMid;
struct OEMid {
  uint8 len; /* should be in [56,120] */
  uint8 OEM_data[120]; /* first 56 octets for Inventory, programmable nr of octets (max 64) for Auxiliary Inventory */ 
};

#define SANT_SET_OEM_ID 0x000D

HPI_SERV_DEF(SetOEMid, SANT_SET_OEM_ID, OEMid, DUMMY) 

#define SANT_SET_NTP_TIME 0x000E
HPI_SERV_DEF(SetPtpTime, SANT_SET_NTP_TIME, PtpTime,PtpTime) 

  /* command can be used for writing or reading the absolute time
     if setPtpTime (sec,nsec) = (0,0), no new date will be set
     in all cases, the modem will return the actual ptp time in the 'updatedPtpTime'
     The time is kept up to date by the modem using the 8KHz ticks on the NTR pin */
void setPtpTime(OUT PtpTime *updatedPtpTime, IN PtpTime *setPtpTime);


typedef struct DspToBoardId DspToBoardId;
struct DspToBoardId {
  uint16 dspIdToBoardId[36];
};

#define SANT_MAP_DSP_TO_BOARD_ID 0x000F
HPI_SERV_DEF(SetMapDsptoBoardId,SANT_MAP_DSP_TO_BOARD_ID, DspToBoardId,DUMMY) 


#define SANT_SET_LED_PATTERN 0x0027
HPI_SERV_DEF(SantSetLedPattern, SANT_SET_LED_PATTERN, LedPattern, DUMMY);


typedef struct SetGpioLedPattern SetGpioLedPattern;
struct SetGpioLedPattern
{
  uint8 gpioIdx; /* can go from 0 to 15 */
  uint8 isActive; /* 1 indicate the gpio should be set in output mode */
  uint16 pattern; /* each bit is the value of the gpo during 1 period. After 8
                    period, the pattern will repeat itself */
};

#define SANT_SET_GPIO_LED_PATTERN 0x002C
HPI_SERV_DEF(SantSetGpioLedPattern, SANT_SET_GPIO_LED_PATTERN, SetGpioLedPattern, DUMMY);


#define SANT_SET_TXFILTER 0x0028
HPI_SERV_DEF(SantSetTxFilter, SANT_SET_TXFILTER, TxFilterDef, DUMMY);


# define AFE_CONNECT_SIZE 72
# define AFE_REMAINING    27
# define AFE_RX_PIN_MAPPING (0)
# define AFE_TX_PIN_MAPPING (0)


typedef struct AfeConnection AfeConnection;
struct AfeConnection 
{
  uint8  data[AFE_CONNECT_SIZE+AFE_RX_PIN_MAPPING+AFE_TX_PIN_MAPPING+AFE_REMAINING]; 
  /* On Toliman, only the first 10 bytes are in use.
     - First 8 bytes define  AfeHWProfile (see afe-toliman/afeDriver.h)
     - last two bytes define the offset between the FW line number and the HW
       line number (modulo 8) respectively for core 0 and 1.
   * On Mikeno, this maps to 1 byte per line */
};
#define SANT_SET_AFE_CONNECT 0x0029
HPI_SERV_DEF(Set_afe_connect, SANT_SET_AFE_CONNECT, AfeConnection, DUMMY);



#define SANT_GET_EXPECTED_LINE_NUMBER 0x002A
HPI_SERV_DEF(GetExpectedLineNumber, SANT_GET_EXPECTED_LINE_NUMBER, DUMMY, uint32);


#define MAX_NUM_IM_DEVICES (8)

// each device will have one of following configured values
#define VIA_LINK0 (0)
#define VIA_LINK1 (1)
#define BAD_LINK  (2) // results in unaccepted configuration
#define UNREACHABLE (3)

// uint64 parameter field has following format :
// bits 0..1 : link_cfg for deviceId 0
// bits 2..3 : link_cfg for deviceId 1
// ...
// bits 62..63 : link_cfg for deviceId 32
#define SANT_SET_INTER_DEVICE_TR_CONFIG 0x0014
HPI_SERV_DEF(SantSetInterDeviceTRConfig, SANT_SET_INTER_DEVICE_TR_CONFIG, uint64, DUMMY)


typedef struct LineDriverDependentConfigExt LineDriverDependentConfigExt;
struct LineDriverDependentConfigExt
{
  TxGainDescriptor     txGainDescriptor;
  fix8_8               txGainRef;       /* tranmist gain for the 0 dB reference of the txGainDescriptor */
  int16                powerThreshold;  /* Txpower value below which the line driver will operate in a reduced power mode */
  LineDriverPowerModes lineDriverPowerModes;
  uint8                afeTxCtrlFlags;  /* bit 0: user control for txCtrl
                                         * bit 1: user control for txAltCtrl
                                         * bit 2: enable new lineDriverPowerModes
                                         *       (reduced power and ADSL line test)
                                         * bit 3: for programmable peak detect
                                         * bit 4: 0 for [i, i+1], 1 for [i, i+2] pairing
                                         */
  uint8                ldType;          /* bit 0: BCM_65040 */ 
                                        /* Still to convert: hwaSupportCommon.c and hwaSupportSwFFT.c*/
  uint8                reservedA[2];
  fix8_8               hwMaxOutputPowerVdsl;
  fix8_8               hwMaxOutputPowerAdsl;
  uint8                reservedB[12];    /* Reserved */
};


#define SANT_SET_EXTRA_LD_CONFIG 0x0026
HPI_SERV_DEF(SantSetExtraLdConfig, SANT_SET_EXTRA_LD_CONFIG, LineDriverDependentConfigExt, DUMMY)


#define SCHED_STAT_HIST_LENGTH  6
#define SCHED_STAT_LINE_NUMBER  3


typedef struct SchedStatReport SchedStatReport;
struct SchedStatReport
{
  uint16           totalSpentTime; /* accumulation of all the time spent in
                                       this task  use this to findout number
                                       of cycles spent = (totalSpentTime*16)<<
                                       cycleShifting */
  uint16           totalicacheMiss;
  uint16           maxSpentTime;    /* maximum amount of time spent in 1 run
                                       of the task (unit is 16 cycles) */
  int16            minDistToDeadLine; /* minimum distance to th deadline can
                                         be negative (2.2MHz ticks)  */
};


typedef struct SchedulingStatEntry SchedulingStatEntry;
struct SchedulingStatEntry
{
  uint32            totalTime; /* amount of time elapsed while this entry was
                                  monitored [ticks at 2.2MHz */
  SchedStatReport   perLineStats[(SCHED_STAT_LINE_NUMBER+1)*2]; /* last one represents the ones
                                                                  which are not in the first lines.
                                                                  note even are Tx and odd are Rx*/  
};


typedef struct SchedMonConfig SchedMonConfig;
struct SchedMonConfig
{  
  uint8                cycleShifting;
  uint8                selectInstrCount;  
  uint16               schedPeriodAt16kHz; // init once but can be modified 
                                            // from interface (ticks at
                                            // 2.2. >>7 gives ticks at 16kHz
};

typedef struct SchedulingMonitoring SchedulingMonitoring;
struct SchedulingMonitoring
{
  uint64               lastSamplingTime; /* we assume sampling freq is high enough to keep
                                            time on 32 bits */
  SchedulingStatEntry  statEntries[SCHED_STAT_HIST_LENGTH];
  uint32               perLineSymbol[SCHED_STAT_LINE_NUMBER]; /* 3MSByte = symbolCount, LSbyte = phase */
  int16                minDistToDeadLine;  // monitored since reset
  uint16               maxSpentTime;       // monitored since reset

  SchedMonConfig       config;
  uint8                nextEntry;
  uint8                reserved[3];  
};



/* define two message to interact with the scheduling monitoring -- this also
   reset the minDistanceTodeadline and the maxSpentTime */
#define SANT_SET_SCHEDULING_CONF  0x0016
HPI_SERV_DEF(SantSetSchedulingConf, SANT_SET_SCHEDULING_CONF, SchedMonConfig, DUMMY)

#define SANT_GET_SCHEDULING_PROF  0x0017
HPI_SERV_DEF(SantGetSchedulingProf, SANT_GET_SCHEDULING_PROF, DUMMY, SchedulingMonitoring)

#ifdef LOG_BACKGROUND_INFORMATION

#define SANT_GET_CPU_MON_INFO 0x0034
HPI_SERV_DEF(SantGetCpuMonInfo, SANT_GET_CPU_MON_INFO, int32, CpuMonitoringInfo)

#endif  
/* for CPE access the i2c interface -- write read command */
typedef struct I2C_request I2C_request;
struct I2C_request
{
  uint8 deviceId;
  uint8 address;
  uint8 numberBytes;
  uint8 isWriteAccess;  
  uint64 data;
};

#define ACCESS_I2C  0x0018
HPI_SERV_DEF(AccessI2C, ACCESS_I2C, I2C_request, uint64)

typedef struct VCXO_request VCXO_request;
struct VCXO_request
{
  uint8  setNtr;
  uint8  reserved[3];
  uint64 vcxo_reg_val; 
};

#define Si57X_SETFREQ  0x0019
HPI_SERV_DEF(Si57x_setFreq, Si57X_SETFREQ, VCXO_request, DUMMY)


/* CO only */
typedef struct NtrTodDelta NtrTodDelta;
struct NtrTodDelta
{ 
  uint32 sampleId;
  int16  nSamples;
  int16 todDeltaHistory[128];
};

#define SANT_GET_NTR_TOD_DELTA  0x001a
HPI_SERV_DEF(SantGetNtrTodStatus, SANT_GET_NTR_TOD_DELTA,uint16,NtrTodDelta)
  /* argument is monitoring freq (in 1/100 of Hz), with MSB indicator of ToD freq:
     0 -> ToD input freq = 8KHz
     1 -> ToD input freq = 2048KHz */

#define SANT_ALIGN_TDD_TO_PPS  0x001a
HPI_SERV_DEF(SantAlignTddToPps, SANT_ALIGN_TDD_TO_PPS,uint16,int32)
  //input = uint16 -> not used for the time being (can be used as identifier for DCXO)
  //output = locking status 
  // -1 -> invalig signal on PPS pin (too far from 4/3KHz)
  // 0 -> init state
  // 1 -> signal validation state
  // 2 -> acquire lock
  // 3 -> locked & tracking


#define XTC_INIT_FIRST_PASS (1<<0) /* bit 0 = initial XTC init done (no common bandplan set, XTC core not enabled) */
#define XTC_INIT_FINAL_PASS (1<<1) /* bit 1 = final XTC init done (after common bandplan was set) */
#define XTC_INIT_IN_SLM     (1<<2) /* bit 2 = device entered SLM, so allow another XTC init  */

#define BCM_SYNC_PULSE_WAIT       0
#define BCM_SYNC_PULSE_FINALIZE  64
#define BCM_SYNC_PULSE_GENERATE 128
#define BCM_SYNC_LEARN     32


typedef struct ControlSyncPulseMsg ControlSyncPulseMsg;
struct ControlSyncPulseMsg 
{
  uint8 mode;  /* mode =   0: wait for pulse (action to slave)
                  mode =  64: final step of init. Set the afeSyncStart and
                  reset internal counters.
                  mode = 128: generate the pulse (action to master) */
};

#define SANT_CONTROL_SYNC_PULSE 0x0030
HPI_SERV_DEF( SantControlSyncPulse, SANT_CONTROL_SYNC_PULSE, ControlSyncPulseMsg, DUMMY);

typedef struct GetSyncMeasMsg GetSyncMeasMsg;
struct GetSyncMeasMsg 
{
  uint8  valid;  /* Indicate if the data are valid */
  uint8  reserved[3];
  uint32 offsetSyncSymbol; /* Offset already stored in the device (only for
                              validation in SLV) */
  uint64 offset; /* Offsetbetween detection (slave) or generation (master) of sync pulse and
                    the afeSyncStart

                    unit = C2.208, in case of KOS this is a multiple of
                    complete symbols
                 */
  uint32 delay;  /* Delay between master and slave afe sync in 1/256 of
                    c70. Only valid from slave
                    0 in case of KOS indicate the delta between afeSync
                    (master and slave)
                 */
  uint8  reservedB[4];
};

#define SANT_GET_SYNC_MEAS 0x0031
HPI_SERV_DEF( SantGetSyncMeas, SANT_GET_SYNC_MEAS,DUMMY,GetSyncMeasMsg);

typedef struct SetSyncCorrMsg SetSyncCorrMsg;
struct SetSyncCorrMsg
{
  int32 offsetMsb;  /* 32 upper bits of the offset */
  uint32 offset;  /* number of 4kHz DMT symbol we want to move the reference
                   * time. Number is expressed on 64 bits anbd upper bits are
                   * located in offsetMsb */
  int32 addOffset; /* Additional to indicate the position in the pilot
                       sequence and VDSL superframe of the modified
                       afeSyncStart. Only valid if delay==0 and core=0
                   */
  uint32 boundary24hToRefTime; /* new value for elapsedTimeIn24h at
                                * refTime. Expressed in msec - value 0 means
                                * ignore If 0 was required, then use 24hour
                                * value
                                */
};

#define SANT_SET_SYNC_CORR 0x0032
HPI_SERV_DEF(SantSetSyncCorr, SANT_SET_SYNC_CORR, SetSyncCorrMsg, DUMMY);  

typedef struct PilotSequenceSynchroRequest PilotSequenceSynchroRequest;
struct PilotSequenceSynchroRequest
{
  uint16 pilotSequenceLengthDS; /* pilot sequence length (4*N) */
  uint16 currentPSN_512;        /* VP current PSN value (512 cycle) */
  uint16 currentPSN_4N;         /* VP current PSN value (4*N cycle) */
  uint16 reserved;
};

/* Already defined in bcm_hmiCoreMsg.h */
// #define SANT_PS_SYNCHRO 0x006E
HPI_SERV_DEF(SantPilotSequenceSynchro, SANT_PS_SYNCHRO, PilotSequenceSynchroRequest, DUMMY)

#ifdef GFAST_SUPPORT

typedef struct TimeRefMove TimeRefMove;
struct TimeRefMove
{
  int psnSeqToMove;  // psnSeqToMove means  move timeRef into the past by x complete psnSeq
  int hbPeriodMove;  // hbPeriodMove means  move timeRef in the past by y HeartBeat period (250usec)
};

#define SANT_TIMERREF_MOVE 0x006A
HPI_SERV_DEF(SantTimeRefMove, SANT_TIMERREF_MOVE, TimeRefMove, DUMMY);  



typedef struct TimeRefInfo TimeRefInfo;
struct TimeRefInfo
{
  uint32 elapsedTimeSinceTimeRef; /* unit is seconds  */
  uint32 reserved; /* remainder of the elapsed time in C2.208 */
  uint32 elapsedTddFramesSinceLastPsn0; /* number of TDD frames between PSN0
                                           TDD frame and the globalRef */
  uint32 boundary24hToRefTime;  //elasped time in msec at refTime since
                                //previous 24hour boundary. note 0 msec is
                                //replaced by 24h value
};

#define SANT_GET_TIME_REF_INFO 0x0067  
HPI_SERV_DEF(SantGetTimeRefInfo, SANT_GET_TIME_REF_INFO, DUMMY, TimeRefInfo);


typedef struct TddInfo TddInfo;
struct TddInfo
{
  uint8 Msf;
  uint8 Mf;
  uint8 Mds;
  uint8 RMCdsIdx;
  uint8 dsPipeline; // number of symbols buffered between VP and AFE in DS
  uint8 RMCusIdx;
  uint8 usPipeline; // number of symbols buffered between AFE and VP in US
  uint8 dsToUsDeadTime; // expressed in frames turnaround between ds and us
  uint8 usToDsDeadTime; // expressed in frames turnaround between us and ds
  uint8 reserved[1];
  uint16 splitTone;  // first US tone transfered on the XTC link
  uint32 muspPeriod; // expressed in GfastSymbols
  uint32 uspPeriod;  // expressed in GfastSymbols
};

#define SANT_GET_TDD_INFO 0x66
HPI_SERV_DEF(SantGetTddInfo, SANT_GET_TDD_INFO, DUMMY, TddInfo);

typedef struct PrecoderBypassCmd PrecoderBypassCmd;
struct PrecoderBypassCmd
{
  uint64 lineIds;
  uint8  bypassOn;
  uint8  direction; // 0x01 -> ds, 0x02 -> us, 0x03 -> both
  int16  offset; // undocumented, used for debug
  uint8  reserved[4];
};

#define SANT_PRECODER_BYPASS_CMD 0x0068
HPI_SERV_DEF(SantPrecoderBypassCmd, SANT_PRECODER_BYPASS_CMD, PrecoderBypassCmd, DUMMY)


#endif



#define SANT_REPORT_PTMSYNC_ON_GPIO 0x0037
 /* 1 means report PtmSync state on gpio (gpio=1 means out of sync,gpio=0
    means PTM in sync), 0 means do not report */
HPI_SERV_DEF(SantReportPtmSyncOnGpio, SANT_REPORT_PTMSYNC_ON_GPIO, uint32, DUMMY);


/***********************************************************************/
/***********    Santorini Boot Services             ********************/
/***********************************************************************/

typedef struct SantGoOp SantGoOp;
struct SantGoOp
{
  uint32  imageType;
  uint32  codeSize;
  int32   controlUtopiaAddr;
};

#define SANT_GO_OPERATIONAL 0x202
HPI_SERV_DEF(SantGoOp, SANT_GO_OPERATIONAL, SantGoOp, int32)

typedef struct GenBootReq GenBootReq;
struct GenBootReq
{
  uint16 interval;
};

typedef struct BootReq BootReq;
struct BootReq
{
  uint32 imageType;
  uint32 hwChipId;
  uint16 dlProtocolMajorVersion;
  uint16 dlProtocolMinorVersion;
  uint16 bootProtocolMajorVersion;
  uint16 bootProtocolMinorVersion;
};

#define SANT_GEN_BOOT_REQ 0x200
HPI_SERV_DEF(GenBootReq, SANT_GEN_BOOT_REQ, GenBootReq, BootReq)


typedef struct BootPacketHeader BootPacketHeader;
struct BootPacketHeader
{
  uint32 imageType;
  uint32 offset;
  uint32 packetSize;
  /*uint8 imageBinary[packetSize]*/
};


#define SANT_BOOT_PACKET 0x201
HPI_SERV_DEF(BootPacket, SANT_BOOT_PACKET, BootPacketHeader, DUMMY)

typedef struct BootReadSmemReq BootReadSmemReq;
struct BootReadSmemReq
{
  uint32 start;
  uint32 length;
};

/*Shortcut: declared like this to make the matlab smemextract test script easy.
 *The size of BootReadSmemReply packets depend on the length specified in the
 *request*/
typedef struct BootReadSmemReply BootReadSmemReply;
struct BootReadSmemReply
{
  uint8 smem[256];
};

#define SANT_BOOT_READ_SMEM 0x205
HPI_SERV_DEF(BootReadSmem, SANT_BOOT_READ_SMEM, BootReadSmemReq, BootReadSmemReply)

#define CORE_SET_DTA_CONFIG (0x23)
HPI_SERV_DEF(CoreSetDtaConfig, CORE_SET_DTA_CONFIG, GlobalDtaConfig, DUMMY)

#define CORE_GET_DTA_CONFIG (0x24)
HPI_SERV_DEF(CoreGetDtaConfig, CORE_GET_DTA_CONFIG, DUMMY, GlobalDtaConfig)

#endif
