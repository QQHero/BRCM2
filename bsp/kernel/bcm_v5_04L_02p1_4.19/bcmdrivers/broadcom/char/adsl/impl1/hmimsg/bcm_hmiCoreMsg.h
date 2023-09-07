/******************************************************************************
 * $COPYRIGHT$
 ******************************************************************************
 *   FileName        : bcm_hmiCoreMsg.h 
 *   Purpose         : Header for all core manager HMI message types
 *   Limitations     : None
 *   Creation Date   : 17-Sep-2001
 *   Current Release : $Name: $  
 ******************************************************************************/

#ifndef BCM_HMI_CORE_MSG_H
# define BCM_HMI_CORE_MSG_H

#ifdef SUPPORT_HMI
# include "bcm_layout.h"

# define BCM_DEV_NB_LINES 1   /* defined BCM_GFAST_ON_65423 for CLI build to fake support for G.fast 212 with 1 line per dev. */
# define BCM_NB_ECHANNELS 2
# define BCM_NB_ES_ADDR   12  /* CLI code defined BCM_NB_ES_ADDR == BCM65200_NB_CORES(12) */

#else
# include "bcm_userdef.h"
# include "bcm_layout.h"
# include "bcm_types.h"
#endif

/* {{{ Application ID */

# define CORE_MANAGER_ID                0x100

/* }}} */
/* {{{ Available services */

# define SET_DEVICE_CONFIGURATION                       0x01
# define GET_DEVICE_CONFIGURATION                       0x02
# define GET_LINE_INTERRUPT_STATUS                      0x03
# define CHANGE_LINE_INTERRUPT_MASK                     0x04
# define RESET_DEVICE                                   0x05     
# define DEVICE_MINOR_EXCEPTION                         0x06
# define GET_FIRMWARE_VERSION_INFO                      0x07
# define WRITE_GPIO                                     0x08
# define READ_GPIO                                      0x09
# define SET_OAM_PERIOD_ELAPSED_TIME                    0x0C
# define SET_OEM_ID                                     0x0D
# define SET_PTP_TIME                                   0x0E
# define SET_PORT_ID                                    0x0F
# define DEVICE_VECT_WAIT_SYNC                          0x10
# define DEVICE_VECT_GET_SYNC                           0x11
# define DEVICE_VECT_SET_OFFSET                         0x12
# define DEVICE_VECT_TOGGLE_GPIO                        0x13
# define SET_INTER_DEVICE_GMII_CONFIG                   0x14
# define SET_VCE_MAC_ADDRESS                            0x15
# define GET_VCE_MAC_ADDRESS                            0x15 /* same but used at CPE */
# define PERFORM_I2C_ACTION                             0x18
# define GET_NTR_STATUS                                 0x1A /* CPE only */
# define ALIGN_TDD_TO_PPS                               0x1A /* CO  only */
# define SET_MDIO_CONTROL                               0x1B
# define WRITE_MDIO_REGISTER                            0x1C
# define READ_MDIO_REGISTER                             0x1D
# define SET_RPF_CONTROL                                0x1E
# define SANT_XTCRING_DO_ACTION                         0x1F
# define SANT_XTCRING_GET_RESULTS                       0x20
# define SANT_XTC_INIT_65400                            0x21
# define SET_GLOBAL_DTA_CONFIG                          0x23
# define GET_GLOBAL_DTA_CONFIG                          0x24
# define CONTROL_TRAFFIC_REPORT                         0x25
# define SET_EXTRA_LD_CONFIG                            0x26
# define SET_LED_PATTERN                                0x27
# define SET_TX_FILTER                                  0x28
# define SET_AFE_CONNECT                                0x29
# define SET_COMMON_BANDPLANS                           0x2B
# define SET_GPIO_LED_PATTERN                           0x2C
# define SET_IWF_CONFIG                                 0x2E
# define GET_IWF_CONFIG                                 0x2F
# define DEVICE_CTRL_SYNC_PULSE                         0x30
# define DEVICE_GET_SYNC_MEAS                           0x31
# define DEVICE_CTRL_SYNC_CORR                          0x32
# define SANT_XTC_INIT                                  0x33
# define SANT_GET_CPU_MON_INFO                          0x34
# define REPORT_PTM_SYNC_ON_GPIO                        0x37
# define DPLL_INBAND_REALIGN                            0x38
# define CONFIGURE_WAIT_BOND_TX_TRAFFIC_AFTER_SHOWTIME  0x39
# define SET_VCE_VERSION                                0x3A
# define GET_VECTORING_STATES                           0x3B
# define SET_RUNTIME_DISABLE_BITSWAP                    0x3C
# define SET_DPLL_CONFIG                                0x3D
# define GET_DPLL_STATUS                                0x3E
# define ALIGN_HB_TO_VP                                 0x3F
# define SERDES_EYE_SCAN                                0x40
# define SERDES_EYE_SCAN_RESULT                         0x41
# define SERDES_GET_EYE_SCAN_DATA                       0x42
# define SERDES_EYE_DENSITY                             0x43
# define SERDES_EYE_DENSITY_RESULT                      0x44
# define SERDES_GET_EYE_DENSITY_DATA                    0x45
# define SERDES_GET_LANE_STATE_EXTENDED                 0x46
# define SERDES_GET_LANE_STATES                         0x47
# define SERDES_START_PRBS                              0x48
# define SERDES_GET_PRBS_RESULT                         0x49
# define SERDES_READ                                    0x4a
# define SERDES_WRITE                                   0x4b
# define SERDES_UPDATE                                  0x4c
# define XTC_FRAMER_COUNTS                              0x4d
# define SERDES_LOOPBACK                                0x4e
# define VP_ALIGN_INFO                                  0x4F
# define CREATE_EXTRA_BONDING_GROUP                     0x50
# define SET_XTC_FRAMER_ENABLE                          0x51
# define SERDES_RESET                                   0x52
# define SERDES_INVERT_POLARITY                         0x53
# define SERDES_SET_LANE                                0x54
# define SERDES_DISABLE_PRBS                            0x55
# define GET_XTC_FRAMER_ENABLE                          0x56
# define SET_XTC_FRAMER_STATUS                          0x57
# define GET_XTC_FRAMER_STATUS                          0x58
# define INSTALL_CPE_ES_RELAY                           0x5D
# define GET_SF_COUNT                                   0x5D
# define SERDES_DUMP_8051                               0x62
# define SERDES_READ_PMD_REGS                           0x63
# define SERDES_DUMP_EVENT                              0x64 
# define SANT_GET_TDD_INFO                              0x66
# define SANT_GET_TIME_REF_INFO                         0x67  
# define GET_DLV_SYNCRHO_STATUS                         0x69  
# define REQUEST_TIMEREF_MOVE                           0x6A
# define INSTALL_TRAFFIC_RELAY                          0x6B
# define GET_AVS_MONITORING_DATA                        0x6C
# define XTC_MONITOR_ROUNDTRIP                          0x6D
# define SANT_PS_SYNCHRO                                0x6E
# define DPLL_I2C_CMD                                   0x6F
# define BOND_GET_EXTRA_GROUPS                          0x70
# define RESET_AFE_TD_IFCE                              0x71

# define SERDES_GET_CORE_STATE                          0x77
# define RPF_SWITCH_BACK_TO_XTC                         0x7C

# define FASTBACK_SET_CO_INFO                           0x7D
# define FASTBACK_SET_CPE_INFO                          0x7E

# define FORCE_FRAMER_BLOCK                             0x7F
# define SERDES_SETTXFIR                                0x80
# define SERDES_STARTBERSCAN                            0x81
# define SERDES_GETBERSCAN                              0x82


/* }}} */

/* {{{ Set- and GetDeviceConfiguration */

# ifdef BCM_G9991_ONE_SID_PER_LINE
#  define BCM_SID_2_ADDR(sid)  ((((sid) & 3) << 8) + ((sid) >> 2))
# else
#  define BCM_SID_2_ADDR(sid)  (sid)
# endif
# ifdef BCM_G9991_ONE_SID_PER_LINE
#  define BCM_ADDR_2_SID(addr)  ((((addr) & 0xFF) << 2) + (((addr) >> 8) & 3))
# else
#  define BCM_ADDR_2_SID(addr)  (addr)
# endif

# define BCM_NWS_PORT_DISABLED   (0xff)
# define BCM_NWS_PORT_OLD_FORMAT (0x80) 

typedef struct NetworkIOPort NetworkIOPort;
struct NetworkIOPort {
  uint16 channel;       /* only used for g999.1 links. 
                         * bits 7..0 specify the SID9..SID2 bits of the TCI field (lineId for DSL PHY).
                         * bits 9..8 specify the SID1..SID0 bits of the TCI field (respectively BC and PR bits for DSL PHY).
                         * could be ever used to specify vlan in case of std. eth. */
  uint8  mac;           /* 0..15 : MAC 
                         * 16 EXTPIF 
                         * 17 XTC 
                         * 0xFF disabled (BCM_NWS_PORT_DISABLED) */
  uint8  flags;         /* bit 0 : set to 1 to indicate payload needs FCS addition
                         * bit 1 : set to 1 to indicate this flow must match on DMAC 
                         * bit 7 : set to 1 to indicate old format where channel holds both SID and MAC, for backward compatibility */

} BCM_PACKING_ATTRIBUTE ;

typedef struct LineConfig LineConfig;
struct LineConfig
{
  NetworkIOPort nwsPort[2]; /* nwsPort[0] defines the network port used for the normal priority traffic on line n
                             * nwsPort[1] defines the network port used for the high priority traffic on line n. */
} BCM_PACKING_ATTRIBUTE ;

static inline Bool isNetworkPortDisabled(NetworkIOPort* port)
{
  if (port->flags & BCM_NWS_PORT_OLD_FORMAT)
    return port->channel == 0xFFFF;
  else 
    return port->mac == BCM_NWS_PORT_DISABLED;
}
  
NetworkIOPort utopiaAddr2NetworkPort(uint16 devId, int16 utopiaAddr, uint8 framingType);
  
/* For interop reasons, the countryCode, providerCode and vendorInfo are no
 * longer available on the HMI -- they're frozen */
# define BCM_COUNTRY_CODE 181
# define BCM_PROVIDER_CODE "BDCM"
# define BCM_VENDOR_INFO 19796

typedef struct BreakPoint256 BreakPoint256;
struct BreakPoint256
{
  uint16 toneIndex;
  int16  value;               /* unit is 1/256 dB */
} BCM_PACKING_ATTRIBUTE ;

#define BCM_NB_TX_GAIN_DESC_BP 6
typedef struct TxGainDescriptor TxGainDescriptor;
struct TxGainDescriptor
{
  uint8         toneSpacing; /* 0 -> 4KHz, 1 -> 52KHz */
  uint8         n;
  BreakPoint256 bp[BCM_NB_TX_GAIN_DESC_BP];
}  BCM_PACKING_ATTRIBUTE ;

#define BCM_NB_RX_GAIN_DESC_BP 12
typedef struct RxGainDescriptor RxGainDescriptor;
struct RxGainDescriptor
{
  uint8         toneSpacing; /* 0 -> 4KHz, 1 -> 52KHz */
  uint8         n;
  BreakPoint256 bp[BCM_NB_RX_GAIN_DESC_BP];
}  BCM_PACKING_ATTRIBUTE ;

typedef struct LineDriverPowerModes LineDriverPowerModes; 
struct LineDriverPowerModes 
{
  uint8   idle;                 /* LD bias when line is stopped/admin down */ 
  uint8   listen;               /* LD bias in handshake listening state (ie
                                 * until first C-TONES are sent) */
  uint8   activate;             /* LD bias in handshake once TX is active until training */
  uint8   maxTone256;           /* LD bias from first training state onwards if
                                 * max used ds tone in [0,255]  */
  uint8   maxTone512;           /* LD bias from first training state onwards if
                                 * max used ds tone in [256,511]  */
  uint8   maxTone1024;          /* LD bias from first training state onwards if
                                 * max used ds tone in [512,1023]  */
  uint8   maxTone2048;          /* LD bias from first training state onwards if
                                 * max used ds tone in [1024,2047] */
  uint8   maxTone4096;          /* LD bias from first training state onwards if
                                 * max used ds tone in [2048,4095] */
  uint8   maxTone8192;          /* LD bias from first training state onwards if
                                 * max used ds tone in [4096,8191] */
  uint8   gfastTx50;            /* gfast 50MHz  mode TX */
  uint8   gfastRx50;            /* gfast 50MHz  mode RX */
  uint8   gfastTx100;           /* gfast 100MHz mode TX */
  uint8   gfastRx100;           /* gfast 100MHz mode RX */
  uint8   gfastTx200;           /* gfast 200MHz mode TX */
  uint8   gfastRx200;           /* gfast 200MHz mode RX */
  uint8   testVdsl;             /* LD bias when in line test state for VDSL */
  uint8   testAdsl;             /* LD bias when in line test state for ADSL */
  uint8   reducedPower;         /* LD bias when in line is in reduced power */
  uint8   reserved[2];          /* Keeps structure multiple of 32 bits */
} BCM_PACKING_ATTRIBUTE ;

typedef struct AfeConfig AfeConfig;
struct AfeConfig 
{
  uint32 dataLinesMap[2];       /* bit map identifying the lines whose
                                 * TxAltCtrl signal carries serial data
                                 * configuration for 6502 LD */
  uint32 clockLinesMap[2];      /* bit map identifying the lines whose
                                 * TxAltCtrl signal carries serial clock
                                 * for the configuration f 6502 LD */
  uint16 data;                  /* configuration data to be sent to 6502 LD */
  uint16 ldSupply;              /* LD supply voltage (unit is 10mV) */
} BCM_PACKING_ATTRIBUTE ;

#define MASK_CFG_VECTORING_VDSL_SUPPORT  (1U<<0)
#define MASK_CFG_VECTORING_FAST_SUPPORT  (1U<<1)
#define MASK_CFG_VECTORING_FASTBACK_CPE_SUPPORT (1U<<2)
#define MASK_CFG_EVCE_VDSL_SUPPORT       (1U<<5)
#define MASK_CFG_EVCE_FAST_SUPPORT       (1U<<6)

#define GFAST_TDD_PARAMS_MDS 28
#define GFAST_TDD_PARAMS_MF  36
#define GFAST_TDD_PARAMS_MSF  8
#define GFAST_TDD_PARAMS_CE  10
#define GFAST_TDD_PARAMS_DTA  0

/* xtcSerdesFlags values */
#define BCM_NO_BACKPRESSURE_VP                 (1<<0)
#define BCM_SWAP_VP_EMBEDDED_PRIORITIES        (1<<1)
#define BCM_ADD_FCS_ON_ES                      (1<<2)
#define BCM_ADD_FCS_ON_MGMT                    (1<<3)
#define BCM_VECTORING_VP_RELAY                 (1<<4)

#define BCM_ES_ROUTING_MODE_ALL_INBAND       (0x0<<5)
#define BCM_ES_ROUTING_MODE_US_OUTBAND       (0x1<<5)
#define BCM_ES_ROUTING_MODE_INJECT_VP        (0x2<<5)
#define BCM_ES_ROUTING_MODE_INTERNAL         (0x3<<5)
#define BCM_ES_ROUTING_MODE_MASK             (0x3<<5)
#define BCM_ADD_FEC_ON_10GKR                 (0x1<<7)

typedef struct DeviceConfig DeviceConfig;
struct DeviceConfig
{
  LineConfig lineConfig[BCM_DEV_NB_LINES];
  uint8  trafficPHYcontrol;        /* bits[1:0]: define how the UL2/PL2 is enabled:
                                                   0 -> Utopia is enabled as soon device is configured
                                                   1 -> Tx utopia is flushed continuously.
                                                   2 -> Utopia is enabled only when the line is in showtime
                                    * bits[2]  : traffic goes via GMII
                                    * bits[3]  : set to enable 3symbol threshold for ppf.
                                                   >= bcm65100 only.
				    * bits[4]  : set to enable special Xoff notification packet for
				    *              Gfast line in DOI, to allow fast return to full NOI.
				    * bits[6]  : set to enable support of jumbo frames in non-bonded Gfast lines.
                                    */            
             
  uint8  ptmMode;                  /* bit 0: set to 1 to place dummy PTM byte on 
                                    *        position 2, instead of 5
                                    * bit 1: set to 1 indicate use PTMv5 in DS
                                    * bit 2: set to 1 indicate PTM CRC16 is
                                             completely handled by the network processor
                                    * bit 3: set to 1 indicate the PTM CRC16 need
                                             to be verified by the DSP. Note this bit is
                                             not used when bit2 is set to 1.
                                    * bit 4: PTM CRC is not stripped in US
                                    * bit 5: set to 1 for 6 header/48 data byte PTM over 
                                             utopia format:
                                             H0 PAD PAD PAD PAD PAD P0 P1 P2 ... P48
                                    * bit 6: set to 1 to indicate that ATM cells are received in a 53 byte format:
                                             H0 H1 H2 H3 HEC P0 P1 P2 ... P48
                                             When not set ATM cells are received in a 54 byte format:
                                             H0 H1 H2 H3 HEC PAD P0 P1 ... P48
                                   *  bit 7: used in conjunction with bit 6 equal to 0:
                                             set to 0 to indicate the position of the padding byte is at byte 6
                                             set to 1 to indicate the position of the padding byte is at byte 54
                                 */

  uint8  flags;                    /* bits 0-4: reserved, set to 0 
                                    * bit 5: filter F4/F5 ATM cells
                                    * bit 6: enable US G999.1 fragments padding in boe mode
                                    * bit 7: reserved */
  uint8  reservedA;
  uint8  pl2Flags;                 /* bit 2: enable ATM to Ethernet interworking function
                                    * bit 4: disables Rx Err reporting on BCM65xxx */
  uint8  globalDeviceId ;          /* identification of the device on the
                                    * board. Independant from deviceId */
  uint16 extraXoffOverhang;        /* Number of bytes that are prequeued in the NPU and cannot be stopped by
				    * G999.1 Xoff or stdandard Ethernet pause frame. This will make the DSP send 
                                    * Xoff sooner and give NPU more time to really stop transmitting */
  uint16 rxSegLen;                 /* Size of the segments used on the G999.1 link in US direction. */
  int8  deviceId ;                 /* identification of the device on the bonding ring. Independent from globalDeviceId 
                                    * Set to -1 to keep booter settings */
  uint8  supportVectoring;         /*
                                     - bit[0] indicates support of G.993.5
                                       vectoring.
                                     - bit[1] inficates support of vectoring
                                       on G.9701
                                   */
  uint16 txGainRef;                /* Absolute Tx gain between AFE chip output
                                    * and the line. This is the 0 dB reference of
                                    * txGainDescriptor. Unit is 1/256 dB */
  TxGainDescriptor txGainDescriptor; /* Fine tuning of the transmit gain
                                      * through a frequency-dependant relative
                                      * gain. Unit is 1/256 dB relative to
                                      * txGainRef. Range is [-50;0] dB */
  int16  rxAfeGains[2];            /* Mean Rx gain between the line and
                                    * the AFE device input in the US0 AnnexA
                                    * band and US0 AnnexB band. Unit is 1/256 dB. */
  int16  rxGainRef;                /* Absolute Rx gain between the line and
                                    * the AFE chip input. This is the 0 dB
                                    * reference of the rxGainDescriptor (1/256 dB) */
  int16  reservedC;
  RxGainDescriptor rxGainDescriptor; /* Fine tuning of the receive gain
                                      * through a frequency-dependant relative
                                      * gain. Unit is 1/256 dB relative to
                                      * rxGainRef. Range is [-50;0] dB */
  int16  agcGains[7];              /* agcGains[i], i = 0..6, is used to indicate the real gain of the
                                    * AGC for a nominal value of (6 * i ? 12) dB with respect to the
                                    * AGC setting of 0 dB (agcGains[2] is always equal to 0).
                                    * Note: These values depend on the receiver front-end
                                    * designs. Unit is 1/256 dB. */
  LineDriverPowerModes lineDriverPowerModes;
  AfeConfig afeConfig;             /* Specific configuration for BCM6502 line
                                    * driver. Leave all fields to zero if this
                                    * line driver is not used. */
  uint8  txPowerOptimizationIlv;   /* Allow power boost when INP = 2 DMT
                                    * Unit is 1/256 dB.  Range is [0:1[ dB */
  uint8  afeTxCtrlFlags;           /* Controls the way the TX_CTRL and TX_ALT_CTRL AFE chips output
				    * pins are managed:
                                    * bit3:0 : reserved
                                    * bit4-5: pairing mode for dual Line Driver:
                                    *       0x0 => afe line i and i+1 are paired
                                    *       0x1 => afe line i and i+2 are paired
                                    *       0x2 => afe line i and i+4 are paired
                                    */
  int16  rxCrestFactor;            /* Crest factor of the input signal. Unit
                                    * is 1/256. 0 means default settings */
  int16  hwMaxOutputPowerGfast;    /* HW limitation on the max transmit power
                                    * in GFAST mode; unit is 1/256 dBm */
  int16  hwMaxOutputPowerVdsl;     /* HW limitation on the max transmit power
                                    * in VDSL mode; unit is 1/256 dBm */
  int16  hwMaxOutputPowerAdsl;     /* HW limitation on the max transmit power
                                    * in ADSL mode; unit is 1/256 dBm */
  uint16 trafficGroupingMask;      /* It only applies to the BCM654XX device when a 10G G999.1 traffic interface is used. 
				    * This bit mask defines how the G999.1 streams are allocated to the internal links 
				    * in the device. The streams whose SIDs (given by the utopiaAddr field) have bit(s) 
				    * matching the trafficGroupingMask are extracted.
				    * -	If cascading of the 10G link toward another device is enabled, the mask must be 
				    *   set such that is extracts all the streams belonging to the device and none of the 
				    *   streams belonging to a subtended device.
				    * - If cascading is not enabled, the mask must be set such that it extracts all the
				    *   streams belonging to the first half of the line and note of the streams belonging
				    * to the second half of the lines.
				    */
  uint8  afeMode;                  /* BCM65400 :
				    * bit0: set to 1 to enable the AFE interface towards a BCM652XX to manage lines
				    *       in Gfast/VDSL overlay mode (VDSL lines being serviced from the BCM652XX device).
				    * BCM65450 :
				    * bit0: set to 1 to 2x oversampling in G.fast 212 rx
				    * bit1: set to 1 to 4x oversampling in G.fast 212 tx
				    */
  uint8  liftCapLD;                /* Optimize receive path noise for 6502 LD */
  int16  powerThreshold;           /* Tx power value below which the line
                                    * driver will operate in a reduced power
                                    * mode. Unit 1/256dBm */
  uint8  reservedE[3];
  uint8  ISDNhardware;             /* Set to one to identify an ISDN hardware.
                                      As a consequence, annex J or M should use
                                      US0 annex B calibration values instead of
                                      the annex A calibration values. */
  uint8  reservedF;
  uint8  ldType;                   /* Set specific Line Driver type:
                                    * bit 0: BCM65040 
                                    * bits 4-7: BCM65800 crest reduction control
                                    */
  uint8  integrityCheck;           /* Firmware integrity check:
                                    * bit 0: enable auto correction
                                    * bit 1: enable auto reboot
                                    * bit 3: display information in the firmware string in case of parity fault
                                    * bit 4: force information display
                                     */
  uint8  initSignalCrestControl[2];/* byte 0: Annex A
                                      byte 1: Annex B/M/J */
  uint8  overwriteConfig;          /* Setting this bit allows to overwrite the
                                    * device configuration (debug purpose only) */
  uint8  ESmaskingMode;            /* Controls LOSS, ES and SES during UAS:
                                    * 0: all UAS counted as LOSS, ES and SES
                                    * 1: LOSS, ES, SES inhibited during UAS */
  uint8  OAMstatisticsCountMode;   /* modify the behavior of ES, SES and UAS
                                    * counters when not in Running Activation
                                    * nor Running Showtime:
                                    * bit1:0 0: incremented each seccond
                                    *        1: incremented when no LPR failure
                                    *        2: remains frozen 
                                    * bit2:  1: increment counters in LPM (RPF)
                                    * bit3:  1: include a subset of the PTM
                                    *           inside the LinePeriodCounters 
                                    * bit4: disable persistent LOS failure monitoring
                                    *       When this bit is set, showtime
                                    *       will not be left even after 5-6
                                    *       seconds of continuous LOS defect
                                    *
                                    * bit 5: Align lastRetrainCause field on
                                    *        G997.1 Initialization success/failure
                                    *        cause coding
                                    * bit 6: controls minEFTR format as per g998.4/corr4
                                    * bit 7: controls systematic LOS failure report
                                    *        at startup/goOutShowtimePM (report if 0).
                                    */
  uint8  enableHPIreplyInterrupt;  /* every HMI reply will generate an int
                                    * bit 0: HPI reply
                                    * bit 1: in-band HMI reply */
  uint8  autoMsgConfig;            /* Autonomous message configuration:
                                    * 0: send ATM in-band autonomous messages
                                    * 1: send IP in-band autonomous messages
                                    * 2: Don't send autonomous messages */
  uint16 autoMsgDestPort;          /* Autonomous message dest. UDP port */
  uint32 autoMsgDestIPaddr;        /* Autonomous message dest. UDP IP address */
  int32  controlUtopiaAddr;        /* BCM65450: reserved
                                     * others: extra  in-band control:
                                     * -1 disable in-band control,
                                     * -2 keep booter config */
  uint8  repetitionRate;           /* autonomous repetition rate (seconds) */
  uint8  autoMsgATMheader[5];      /* ATM header for autonomous messages */
  uint8  reservedG[2];
  uint32 autonomousRef;            /* device reference identification to be
                                    * used in autonomous message header */
  uint8  ntrGpioEnable;            /* Only at CPE */
  uint8  ntrGpioAddress;           /* Only at CPE */
  uint8  ppsGpioEnable;            /* Only at CPE */
  uint8  ppsGpioAddress;           /* Only at CPE */
  uint8  ntrGpioExternalControl;   /* Only at CPE */
  uint8  reservedH[11];
#ifdef BCM_VECTORING_LEGACY
  uint8  legacyKey[2];
#else /* BCM_VECTORING_LEGACY */
  uint8  reservedK[2];
#endif /* BCM_VECTORING_LEGACY */
  int16  hwMaxOutputPowerHighBW;   /* HW limitation on the max transmit power
                                    * in high BW VDSL mode. Unit is 1/256 dBm */
  uint8  reInitTimeOut;            /* Timeout for CPE detection in [s] */
  uint8  rxSegLenMin;              /* minimum g999.1 US fragment length.
                                    * 0 -> no penultimate mode.
                                    * > 0 -> indidcates minimum fragment length
                                    */
  uint8  gfastTddParamsMds;
  uint8  gfastTddParamsMf;
  uint8  gfastTddParamsMsf;
  uint8  gfastCyclicExtensionM;     /* Lcp = gfast_cyclic_extension_m*N/64. Only supported value 10 */
  uint16 xtcSerdesFlags;                           /* bit 0 : set to 1 in order to disable 10GKr XOFF/XON flow control.
                                                    * bit 1 : set to 1 in order to set ES on prio0(high) and MGMT on prio1(low)
                                                    * bit 2 : set to 1 in to enable FCS generation on ES   relay from NPU to 10Gkr
                                                    * bit 3 : set to 1 in to enable FCS generation on MGMT relay from NPU to 10Gkr
                                                    * bit 4 : set to indicate the DSP should relay ES and MGMT packets between NPU and 10GKR.
                                                    * Bit 5..6 : error sample routing mode.
                                                    *            For VDSL only product :
                                                    *             0 : indicates all error samples are send inband with traffic.
                                                    *             1 : indicates the US ES should be sent on separate channel, indicated
                                                    *                 by errorSamplesAddrOOB. DS ES are still inband. This mode is usefull
                                                    *                 for external bonding.
                                                    *             2 : set to indicate US and DS ES are directly injected into the 10Gkr xtc
                                                    *                 serdes embedded channel.
                                                    *                 NOTE : this mode is only available on 652xx.
                                                    *             3 : set to indicate US and DS ES are directly send to a relay/vce core via the tokenring buffer.
                                                    *                 NOTE : this mode is only available for gfast chipset
                                                    *            For GFAST product :
                                                    *             0 : not supported.
                                                    *             1 : All ES are sent on separate channel, indicated by errorSamplesAddrOOB[X]
                                                    *                    In case of 4 line DSP, errorSamplesAddrOOB[>=4] will be ignored.
                                                    *                    In case of 6 line DSP, errorSamplesAddrOOB[>=6] will be ignored.
                                                    *             2,3 : same as VDSL
                                                    *
                                                    *  more info in section 12, global configuration.
                                                    */

  NetworkIOPort errorSamplesAddrOOB[BCM_NB_ES_ADDR]; /* BCM6522x chipset (VDSL)   : nws port for out of band error samples per core
                                                      * BCM6524x chipset (G.fast) : nws port for out of band error samples per line.
                                                      * BCM654xx : errorSamplesAddrOOB[0] is used for all lines ([1..11] are unused). 
                                                      */

  NetworkIOPort xtcSerdesRelayAddr[BCM_NB_ECHANNELS];/* nws port for the relay of MGMT and ES on xtc serdes link */
  uint16 gfastVFsamplesVlan;           /* Vlan tag for gfast VF samples. No Vlan tag if set to 0. */
  uint8  progGainXtcTx;                /* Programmable shift for Tx VDSL XTC gains 
                                          bit[3:0] =  right shift on tx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on tx gainOut. Valid values 0:10.
                                       */
  uint8  progGainXtcRx;                /* Programmable shift for Rx VDSL XTC gains
                                          bit[3:0] =  right shift on rx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on rx gainOut. Valid values 0:10.
                                       */
  uint8  progGainXtcTxGfast;           /* Programmable shift for Gfast Tx XTC gains
                                          bit[3:0] =  right shift on tx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on tx gainOut. Valid values 0:10.
                                       */
  uint8  progGainXtcRxGfast;           /* Programmable shift for Gfast Rx XTC gains
                                          bit[3:0] =  right shift on rx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on rx gainOut. Valid values 0:10.
                                       */
  uint8  enableDtaFraming;             /* if set, the DTA compatible framing (RMC position early in the TDD frame) is enabled.
                                        * Only supported on BCM65400 with supportVectoring = 0 */
  uint8  us10GshaperCfg;               /* Only applicable when the 10G G999.1 traffic interface is enabled. 
					  Configures the 10G US shaper : the US traffic is limited to a rate equals to 
					  N/M * 10Gb/s, with:
					  bits [7:4] N
					  bits [3:0] M
					  Note that value 0 is interpreted as 16.
					  Default value us10GshaperCfg = 0x0 sets N=16/M=16 and disables the US 10G shaper.
					  Configuring us10GshaperCfg = 0x12 limits the US rate on the inteface to 5Gb/s.
				       */
  uint8   coax;                       /* medium is coax iso twisted pair */
  uint8   reservedJ;                  /* reserved */
  uint8   lrGfast;                    /* if set, the long reach Gfast timing is selected to accomodate longer loop lengths */
  uint8	  reservedL[5];               /* reserved for future usage */

} BCM_PACKING_ATTRIBUTE ;

typedef DeviceConfig    SetDeviceConfigurationMsgReq;
extern  MsgLayout       SetDeviceConfigurationLayout;


typedef DeviceConfig    GetDeviceConfigurationMsgRep;
extern  MsgLayout       GetDeviceConfigurationLayout;


/* }}} */
/* {{{ GetLineInterruptStatus */

# define LINE_INT_IDLE_NOT_CONFIGURED_STATE    (1U<<0)
# define LINE_INT_SRA_NE_RATE_CHANGED          (1U<<1)
# define LINE_INT_SOS_NE_RATE_CHANGED          (1U<<2)
# define LINE_INT_INIT_FAIL                    (1U<<3)
# define LINE_INT_RE_INIT                      (1U<<4)
# define LINE_INT_VECT_STATE_CHANGE            (1U<<5)
# define LINE_INT_NE_OAM_15MIN_THRESHOLD       (1U<<6)
# define LINE_INT_TRANSPARENT_EOC_TX_FAILED    (1U<<7)
# define LINE_INT_FE_OAM_15MIN_THRESHOLD       (1U<<9)
# define LINE_INT_PREV_15MIN_RETROFIT          (1U<<10)
# define LINE_INT_NO_REPLY_TO_VECT_EOC         (1U<<11)
# define LINE_INT_LINE_STATE_CHANGE            (1U<<12)
# define LINE_INT_RPA_NE                       (1U<<13)
# define LINE_INT_RPA_FE                       (1U<<14)
# define LINE_INT_SOS_FE_RATE_CHANGED          (1U<<15)
# define LINE_INT_FAILURE_STATE_CHANGE         (1U<<16)
# define LINE_INT_EOC_READ_REGISTER_READY      (1U<<17)
# define LINE_INT_OVERHEAD_TRANSACTION_DONE    (1U<<18)
# define LINE_INT_TRANSPARENT_EOC_RECEIVED     (1U<<19)
# define LINE_INT_CPE_DYING_GASP               (1U<<20)
# define LINE_INT_AUTO_RESET                   (1U<<21)
# define LINE_INT_TRANSPARENT_EOC_TRANSMITTED  (1U<<22)
# define LINE_INT_BITSWAP_NE                   (1U<<23)
# define LINE_INT_BITSWAP_FE                   (1U<<24)
# define LINE_INT_15MIN_PERIOD                 (1U<<25)
# define LINE_INT_24HOUR_PERIOD                (1U<<26)
# define LINE_INT_BONDING_RATE_CHANGED         (1U<<27)
# define LINE_INT_TIGA                         (1U<<28)
# define LINE_INT_SRA_FE_RATE_CHANGED          (1U<<29)
# define LINE_INT_ATUC_NOT_VDSL                (1U<<30) /* only at CPE */
# define LINE_INT_NO_UPBO_SUPPORT              (1U<<30) /* only at CO */
# define LINE_INT_BONDING_INCONSISTENT_CONFIG  (1U<<31)

# define LINE_INT_SRA_RATE_CHANGED             (LINE_INT_SRA_NE_RATE_CHANGED | LINE_INT_SRA_FE_RATE_CHANGED)
# define LINE_INT_SOS_RATE_CHANGED             (LINE_INT_SOS_NE_RATE_CHANGED | LINE_INT_SOS_FE_RATE_CHANGED)

/* some usefull aggregates */
# if defined(BCM_USE_INIT_THRESHOLDS)
#  define LINE_INT_15MIN_THRESHOLD  (LINE_INT_INIT_FAIL | \
                                     LINE_INT_RE_INIT   | \
                                     LINE_INT_NE_OAM_15MIN_THRESHOLD | \
                                     LINE_INT_FE_OAM_15MIN_THRESHOLD)
# else
#  define LINE_INT_15MIN_THRESHOLD  (LINE_INT_NE_OAM_15MIN_THRESHOLD | \
                                     LINE_INT_FE_OAM_15MIN_THRESHOLD)
# endif

typedef struct LineInterrupt LineInterrupt;
struct LineInterrupt
{
  uint64 interruptSummary;      /* one bit per line bit 0 refers to line 0 */
  uint32 interruptStatus[BCM_DEV_NB_LINES]; /* Status Register */
  uint32 interruptMask[BCM_DEV_NB_LINES];   /* Mask Register */
} BCM_PACKING_ATTRIBUTE ;

typedef LineInterrupt GetLineInterruptStatusMsgRep;
extern  MsgLayout     GetLineInterruptStatusLayout;

/* }}} */
/* {{{ ChangeLineInterruptMask */

typedef struct SantChangeLineIMR SantChangeLineIMR;
struct SantChangeLineIMR
{
  /* Bit array specifying for which interrupts the newMask must be taken into
   * account */
  uint32 concernedInterrupts[BCM_DEV_NB_LINES];
  /* Bit array giving the new mask for the concernedInterrupts. */
  uint32 newMask[BCM_DEV_NB_LINES];
} BCM_PACKING_ATTRIBUTE ;

typedef SantChangeLineIMR   ChangeLineInterruptMaskMsgReq;
extern  MsgLayout           ChangeLineInterruptMaskLayout;

/* }}} */
/* {{{ ResetDevice */

extern MsgLayout  ResetDeviceLayout;

/* }}} */
/* {{{ DeviceMinorException */

extern MsgLayout  DeviceMinorExceptionLayout;

/* }}} */
/* {{{ GetFirmwareVersionInfo */

/* afe chip ids returned by FW */
# define BCM65800A0 0x27
# define BCM65800   0x28
# define BCM65829   0x29
# define BCM65840   0x2A
# define BCM658XX   0x2B
# define BCM65400   0x40
# define BCM65400B0 (BCM65400+8)
# define BCM65400B1 (BCM65400+16)
# define BCM65450   0x45
# define BCM65900   0xBA
# define BCM65920   0xBB
# define BCM65930   0xBC
# define BCM65940   0xBD
# define BCM65944   0xBE
# define BCM65941   0xBF
# define BCM659FF   0xC1        /* not read yet */
# define BCM65900C0 (BCM65900+16)
# define BCM65936   0xF7
# define BCM65937   0xF8
# define BCM6593X   0xFE        /* OTP not read yet */
# define BCM659XX   BCM6593X    /* last known 65900 serie */

typedef struct FirmwareVersionInfo FirmwareVersionInfo;
struct FirmwareVersionInfo
{
  uint32 hwChipId;
  uint16 fwMajorVersion;
  uint8  fwMinorVersion;
  uint8  fwFixVersion;
  uint16 hmiMajorVersion;
  uint16 hmiMinorVersion;
  uint8  info[64];
  uint8  nrOfLinesSupportedInFirmwareVersion;
  uint8  afeChipId;
  uint8  ldTypeMap;
  uint8  reservedA;
  uint8  buildType; 
  uint8  reservedB[3];
} BCM_PACKING_ATTRIBUTE ;

typedef FirmwareVersionInfo  GetFirmwareVersionInfoMsgRep;
extern  MsgLayout            GetFirmwareVersionInfoLayout;

typedef struct FirmwareVersionInfoCPE FirmwareVersionInfoCPE;
struct FirmwareVersionInfoCPE
{
  uint32 hwChipId;
  uint16 fwMajorVersion;
  uint8  fwMinorVersion;
  uint8  fwFixVersion;
  uint16 hmiMajorVersion;
  uint16 hmiMinorVersion;
  uint8  info[64];
  uint8  nrOfLinesSupportedInFirmwareVersion;
  uint8  afeChipId;
  uint8  cpldVersion;
  uint8  vcxoVersion; // 0x1d = Si514, 0x1c = Si570, 0x6a = SiTime3521
  uint8  buildType; 
  uint8  reservedB[3];
} BCM_PACKING_ATTRIBUTE ;

uint32 getAfeChipNb(uint8 afeChipId);

/* }}} */
/* {{{ WriteGPIO */

typedef struct GPoutData GPoutData;
struct GPoutData {
  uint32 newState;                 /* 32-bits bitmap */
  uint32 dontMask;                 /* ~mask to apply to map */
} BCM_PACKING_ATTRIBUTE ;

typedef GPoutData WriteGPIOMsgReq;
extern  MsgLayout WriteGPIOLayout;

/* }}} */
/* {{{ ReadGPIO */

typedef struct GPinData GPinData;
struct GPinData {
  uint32 state;                    /* 32-bits bitmap */
} BCM_PACKING_ATTRIBUTE ;

typedef GPinData  ReadGPIOMsgRep;
extern  MsgLayout ReadGPIOLayout;

/* }}} */
/* {{{ SetOAMperiodElapsedTime */

typedef struct OAMperiodElapsedTime OAMperiodElapsedTime;
struct OAMperiodElapsedTime
{
  uint32 elapsedTime;
  uint32 numberOfSecIn15min;
  uint16 numberOf15minIn24hour;
  uint16 periodInterruptOffset;
} BCM_PACKING_ATTRIBUTE ;

typedef OAMperiodElapsedTime SetOAMperiodElapsedTimeMsgReq;
extern  MsgLayout            SetOAMperiodElapsedTimeLayout;

/* }}} */
/* {{{ SetOEMid */

typedef struct SetOEMidReq SetOEMidReq;
struct SetOEMidReq
{
  uint8 len;                          /* Length of the OEM data provided. Must be between 56 and 120 */
  uint8 data[120];                    /* Defines the data returned upon receiving the EOC request for inventory or 
				       * auxiliary inventory data. The first first 56 octets defines the inventory
				       * response, a programmable nr of octets (max 64) defines the Auxiliary 
				       * Inventory response.
				       */
} BCM_PACKING_ATTRIBUTE ;

typedef SetOEMidReq SetOEMidMsgReq;
extern  MsgLayout SetOEMidLayout;

/* }}} */
/* {{{ SetPtpTime */

typedef struct PtpTime PtpTime;
struct PtpTime {
  uint8 secondsSinceEpoch[6];       /* Octet 0 is MSB. Specifying time=0 allows retrieving the current time 
				     * without setting a new time. */
  uint8 nanoSecondsSinceSecond[4]; 
} BCM_PACKING_ATTRIBUTE ;

typedef PtpTime   SetPtpTimeMsgReq;
typedef PtpTime   SetPtpTimeMsgRep;
extern  MsgLayout SetPtpTimeLayout;

/* }}} */
/* {{{ SetPortId  */

typedef struct SetPortIdReq SetPortIdReq;
struct SetPortIdReq
{
  uint16 data[36];           /* Defines the mapping between the internal device line numbering and a global line number
			      * that shall be communicated to the CPE as part of the EOC Inventory data: if the characters
			      * '%d' are present in the provided inventory data string (see SetOEMid message above),
			      * those characters are replaced by the global line number of the corresponding line.
			      */
} BCM_PACKING_ATTRIBUTE ;

typedef SetPortIdReq SetPortIdMsgReq;
extern  MsgLayout SetPortIdLayout;

/* }}} */
/* {{{ DeviceVectWaitSync */

typedef struct GpioStruct GpioStruct;
struct GpioStruct
{
  uint8 port;  /* 0=>A, 1=>B, 2=>C, 3=>D */ 
  uint8 pin;   /* 0->7                   */ 
} BCM_PACKING_ATTRIBUTE;

typedef GpioStruct   DeviceVectWaitSyncMsgReq;
extern  MsgLayout    DeviceVectWaitSyncLayout;

/* }}} */
/* {{{ DeviceVectGetSync */

typedef struct SyncTime SyncTime;
struct SyncTime {
  uint64 timeSinceBoot;
  int32  reserved[2];
} BCM_PACKING_ATTRIBUTE;

typedef SyncTime  DeviceVectGetSyncMsgRep;
extern  MsgLayout DeviceVectGetSyncLayout;

/* }}} */
/* {{{ DeviceVectSetOffset */

typedef struct SyncOffset SyncOffset;
struct SyncOffset {
  int32 offset;
} BCM_PACKING_ATTRIBUTE;

typedef SyncOffset DeviceVectSetOffsetMsgReq;
extern  MsgLayout  DeviceVectSetOffsetLayout;

/* }}} */
/* {{{ DeviceVectToggleGpio */

typedef struct VectToggleInput VectToggleInput;
struct VectToggleInput {
  uint8 config;
  GpioStruct gpioStruct;
} BCM_PACKING_ATTRIBUTE;

typedef struct VectToggleOutput VectToggleOutput;
struct VectToggleOutput {
  uint64 timeSinceBoot;
  int32  offsetSyncSymbol;
  uint32 reserved;
} BCM_PACKING_ATTRIBUTE;

typedef VectToggleInput  DeviceVectToggleGpioMsgReq;
typedef VectToggleOutput DeviceVectToggleGpioMsgRep;
extern  MsgLayout        DeviceVectToggleGpioLayout;

/* }}} */
/* {{{ SetDeviceCtrlSyncPulse */

#define BCM_SYNC_PULSE_WAIT       0
#define BCM_SYNC_PULSE_ALIGN     32
#define BCM_SYNC_PULSE_FINALIZE  64
#define BCM_SYNC_PULSE_GENERATE 128

typedef struct SyncPulse SyncPulse;
struct SyncPulse {
  uint8 mode;                   /* see BCM_SYNC_PULSE_xxx */
} BCM_PACKING_ATTRIBUTE;

typedef SyncPulse SetDeviceCtrlSyncPulseMsgReq;
extern  MsgLayout SetDeviceCtrlSyncPulseLayout;

/* }}} */
/* {{{ ConfigWaitBondTxTrafficAfterShowtime */

typedef struct WaitTime WaitTime ;
struct WaitTime {
  uint8 time;
} BCM_PACKING_ATTRIBUTE;

typedef WaitTime   ConfigWaitBondTxTrafficAfterShowtimeMsgReq;
extern  MsgLayout  ConfigWaitBondTxTrafficAfterShowtimeLayout;

/* }}} */
/* {{{ GetDeviceCtrlSyncMeas */

typedef struct SyncMeas SyncMeas;
struct SyncMeas {
  uint8  valid;
  uint8  reservedA[3];
  uint32 offsetSyncSymbol;
  uint64 offset;
  uint32 delay;
  uint8  reservedB[4];
} BCM_PACKING_ATTRIBUTE;

typedef SyncMeas  GetDeviceCtrlSyncMeasMsgRep;
extern  MsgLayout GetDeviceCtrlSyncMeasLayout;

/* }}} */
/* {{{ SetDeviceCtrlSyncCorr */

typedef struct SyncCorr SyncCorr;
struct SyncCorr {
  int32  offsetMSB;
  uint32 offset;
  uint32 addOffset;
  uint32 offset24hour;
} BCM_PACKING_ATTRIBUTE;

typedef SyncCorr SetDeviceCtrlSyncCorrMsgReq;
extern  MsgLayout SetDeviceCtrlSyncCorrLayout;

/* older message for 65100 */
typedef struct SyncCorr65100 SyncCorr65100;
struct SyncCorr65100 {
  uint32 delay;
  uint32 offset;
  uint32 addOffset;
} BCM_PACKING_ATTRIBUTE;

typedef SyncCorr65100 SetBcm65100CtrlSyncCorrMsgReq;
extern  MsgLayout SetBcm65100CtrlSyncCorrLayout;

/* }}} */
/* {{{ RequestTimeRefMove */

typedef struct TimeRefMove TimeRefMove;
struct TimeRefMove
{
  int psnSeqToMove;  /* psnSeqToMove means  move timeRef into the past by complete psnSeq */
  int hbPeriodMove;  /* hbPeriodMove means  move timeRef in the past by  HeartBeat period (250usec)*/
}BCM_PACKING_ATTRIBUTE;

typedef TimeRefMove RequestTimeRefMoveMsgReq;
extern  MsgLayout RequestTimeRefMoveLayout;
  

/* }}} */
/* {{{ GetTimeRefInfo */
typedef struct TimeRefInfo TimeRefInfo;
struct TimeRefInfo
{
  uint32 elapsedTimeSinceTimeRef;    /* unit is seconds  */
  uint32 debugData; 
  uint32 elapsedTddFramesSinceLastPsn0; /* number of TDD frames between PSN0
                                           TDD frame and the globalRef */
  uint32 offset24hour; 
} BCM_PACKING_ATTRIBUTE;

typedef TimeRefInfo GetTimeRefInfoMsgRep;
extern  MsgLayout GetTimeRefInfoLayout;

/* }}} */
/* {{{ GetTddInfo */
typedef struct TddInfo TddInfo;
struct TddInfo
{
  uint8 Msf;
  uint8 Mf;
  uint8 Mds;
  uint8 RMCdsIdx;   /* RMCds index in the TDDphy */
  uint8 dsPipeline; /* number of symbols buffered between VP and AFE in DS */
  uint8 RMCusIdx;   /* RMCus index in the TDDphy */
  uint8 usPipeline; /* number of symbols buffered between AFE and VP in US */
  uint8 dsToUsDeadTime; /* expressed in frames turnaround between ds and us */
  uint8 usToDsDeadTime; /* expressed in frames turnaround between us and ds */
  uint8 reserved[1];
  uint16 splitTone;  /* first US tone transfered on the XTC link */
  uint32 muspPeriod; /* expressed in GfastSymbols */
  uint32 uspPeriod;  /* expressed in GfastSymbols */
} BCM_PACKING_ATTRIBUTE;

typedef TddInfo GetTddInfoMsgRep;
extern  MsgLayout GetTddInfoLayout;
  
/* }}} */
/* {{{ GetDlvSynchroStatus */

#define DLV_SYNC_unknown        0
#define DLV_SYNC_remoteReady    1
#define DLV_SYNC_ongoing        2
#define DLV_SYNC_aligned        3

typedef struct DlvSynchroStatus DlvSynchroStatus;
struct DlvSynchroStatus
{
  int currentState;
  int subStateInformation;  /* only required when debugging */
};

typedef DlvSynchroStatus GetDlvSynchroStatusMsgRep;
extern MsgLayout        GetDlvSynchroStatusLayout;

/* }}} */
/* {{{ StartXtcInit */
/* BCM65200 XTC modes */
#define XTC_MODE_CLV                0
#define XTC_MODE_BLV                1
#define XTC_MODE_SLV                2
#define XTC_MODE_BCM65300_SLV       3
#define XTC_MODE_DLV                4
#define XTC_MODE_RBLV               5
#define MAX_10G_LINKS_PER_DEVICE_65200 (4)
#define MAX_10G_LINKS_PER_DEVICE_65400 (6)
#define MAX_10G_LINKS_PER_DEVICE       (20) /* maximum # xtc links in use per device */

#define XTC_EXTRA_CFG_SWAP_ECHAN_BIT 0

/* only first 4 bytes are used for BCM65300 */
typedef struct XtcInit XtcInit;
struct XtcInit
{
  uint8 xtcMode;         /* BCM65200: 0x0: CLV, 0x1: BLV, 0X2: SLV, 0x3: BCM65300 compatible SLV, 0x4: DLV (dual line card) */
                         /*           0x80: replace mode in G.Fast
                          *           0x40: replace mode in VDSL2 */
  uint8 reservedA;
  uint16 xtcVpRoundtrip; /* Nominal VP roundtrip delay at 4.4MHz on BCM65300
                            Nominal VP roundtrip delay at 35MHz on BCM65200  */
  int32  xtcVpSyncDelay; /* nominal delay between VPsync event and afeGroup0
                            correction in samples at 560MHz */
  uint8 numDeadPackets;  /* number of deadPackets:
                             SLV mode 2: means number of packets during DS/US swap
                             BCM65300 SLV mode 3: not relevant
                          */
  uint8 masterDevice;    /* set to one to indicate this is the master device of
                            the board. The other device will have to align their
                            timing on this device */
  uint8 devicesPerBoard; /* used in case of BLV in order to compute the amount
                            of vectored lines */
  uint8 framerLinkMuxing;/* 0x0: no multiplexing
                            0x1: BCM65300 compatible 35b mode with bandplan split
                            0x2: lines are split over 2 links - 0x42: the second link is merged into another DSP device
                            0x3: link merge mode: the third link is merged into the second link
                         */
  
  /* vectoring formating */
  uint8 bitsPerCompressedTone; /* between 13 to 18 */
  uint8 exponentSharing; /* indicate which kind of exponent sharing should be used */
  uint8 coefficientFormat; /* XTC_COEFF_18BIT 0=18bits or XTC_COEFF_20BIT 1=20bits only CLV/BLV*/
  
  uint8 noFullEnable;
  /* 65300 compatibility settings */
  uint32 BCM65300SlvDsSetting;
  uint32 BCM65300SlvUsSetting;
  int16  psnOffset;         /* apply a systematic PSN offset w.r.t. the one
                             * retrieved from the VP (expressed in DMT symbols) */
  uint8  noBpClamping;      /* to avoid upstream bandplan clamping in TCSLV on
                               BCM65200A0/B0 */
  uint8  coefReplication; /* coef interpolation mode:
                             bit0 : Gfast replication mode : 0->interpolate,1->duplicate
                             bit1 : VDSL replication mode : 0->interpolate,1->duplicate
                          */
  uint32 slvNumberPackets;         /* configure a fixed number of TC-SLV or Tidal SLV packets
                                      - 0 will use the default SW setting */
  uint8 modifyDefault10GKRmapping; /* 0 == default mapping */
  uint8 mapping10GKR[MAX_10G_LINKS_PER_DEVICE_65200]; /*internal to external mapping*/
  uint8 tcslv35bMode;
  uint16 psnIncPeriod; /* Gfast framing : indicates periodicity at which
                        * SCNT increases - expressed in TDD frames */
  uint32 extraConfigFlags;  /* bit[0]: indicate if the echan is on internal XTC link 1 instead of 0 */
  uint8 disableVAatStart;
  uint8 variousFlag;                  /* bit[1]: use Xtc gains below iso deviceConfig Xtc gains */
  uint8 progGainXtcTx;                /* Programmable shift for Tx VDSL XTC gains
                                          bit[3:0] =  right shift on tx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on tx gainOut. Valid values 0:10.
                                       */
  uint8 progGainXtcRx;                /* Programmable shift for Rx VDSL XTC gains
                                          bit[3:0] =  right shift on rx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on rx gainOut. Valid values 0:10.
                                       */
  uint8 progGainXtcTxGfast;           /* Programmable shift for Gfast Tx XTC gains
                                          bit[3:0] =  right shift on tx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on tx gainOut. Valid values 0:10.
                                       */
  uint8 progGainXtcRxGfast;           /* Programmable shift for Gfast Rx XTC gains
                                          bit[3:0] =  right shift on rx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on rx gainOut. Valid values 0:10.
                                       */
  uint8 reservedE[38];
} BCM_PACKING_ATTRIBUTE;

typedef XtcInit     StartXtcInitMsgReq;
extern  MsgLayout   StartXtcInitLayout;

typedef struct XtcInit654xxMsg XtcInit654xxMsg;
struct XtcInit654xxMsg{
  uint8  vectorMode;   /* CLV=0, BLV=1, SLV=2
                          upper bits definition :
                          0x80 Gfast replace mode
                          0x40 VDSL replace mode
                        */
  uint8  linkMux;  /* indicates how 10GKR are being used  0:SLV and 2-chip BLV;  1:3-chip BLV ; 
                      2 SLV with VDSL tone split over low and high link */
  uint8  devicesPerBoard;
  uint8  coefReplication; /* coef interpolation mode:
                             bit0 : Gfast replication mode : 0->interpolate,1->replicate
                             bit1 : VDSL replication mode : 0->interpolate,1->replicate
                             Configuration 0x1 (Gfast replicate/VDSL interpolate)  is not valid
                          */
  uint8  mapping10GKR[MAX_10G_LINKS_PER_DEVICE];  /* mapping10GKR[i] = j indicates external 10GKR
                                                           index j should map on internal 10GKR i  */
  uint8  reservedB;
  uint8  coefficientFormat;
  
  /* all subsequent fields are reserved in case of CLV/BLV mode */
  int32    xtcVpRountrip; /* C35 */
  int32    xtcVpSyncDelayC560;
  int32    hybridGroupingIs16;
  int32    hybridVdslActiveFrames;
  int32    hybridDeadFrames;
  int32    hybridTidalSwapPointDelay; /* unit C35 */
  uint16   psnIncPeriod;
  uint8    disableVAatStart;
  uint8    vceDevIdx;    /* VCE device idx as defined in the topology of the HMI. Only used if nrOfBoards > 1*/
  uint8    nrOfBoards;   /* Nr of boards. Set to 0 or 1 for BLV, Set to 2 or 3 for DLV */
  uint8    logReuseFactorGfast; /* Log2 of Reuse Factor (RF) of G.fast if value=0, used default
                                    RF=2 for 1,2,3 devs vectoring group
                                    RF=4 for 4,6   devs vectoring group
                                */
  uint8    variousFlag;        /* bit[1]: use Xtc gains below iso deviceConfig Xtc gains */
  /* G.fast and G.vector reuse factor or not necessarily the same. 
   * Setting the value to zero equals log2M=2 (necessary for backwards compatibility, other values are real log2M values. */
  uint8    logReuseFactorVdsl; 
  uint8    nrOfLinesInVce; /* Only used with embedded G.fast VCE */
  uint8    inverseXonPolarity; /* If set to 1, will invert the polarity of the Xon bit in echan (only available on B1) */
  uint8    dsToUsDeadTime;  /* value to use for the amount of frames between DS and US. If the configured value is smalled than optimal value, the optimal value will be selected. */
  uint8    slvGainShift;/* Extra gain on input of error coming from VP, total gain 2^(-5+slvGainShift)
                            bit[0:3]= gain when line is in VDSL
                            bit[4:7]= gain when line is in G.fast
                           Only availble on BCM654xxB0 and higher
                        */
  uint8    vectLineId[16];  /* Only used with embedded G.fast VCE */
  uint8    progGainXtcTx;              /* Programmable shift for Tx VDSL XTC gains
                                          bit[3:0] =  right shift on tx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on tx gainOut. Valid values 0:10.
                                       */
  uint8    progGainXtcRx;              /* Programmable shift for Rx VDSL XTC gains
                                          bit[3:0] =  right shift on rx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on rx gainOut. Valid values 0:10.
                                       */
  uint8    progGainXtcTxGfast;         /* Programmable shift for Gfast Tx XTC gains
                                          bit[3:0] =  right shift on tx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on tx gainOut. Valid values 0:10.
                                       */
  uint8    progGainXtcRxGfast;         /* Programmable shift for Gfast Rx XTC gains
                                          bit[3:0] =  right shift on rx gainIn.  Valid values 0:5.
                                          bit[7:4] =  left shift  on rx gainOut. Valid values 0:10.
                                       */
  uint8    reservedD[20];
} BCM_PACKING_ATTRIBUTE;

typedef XtcInit654xxMsg StartXtcInit65400MsgReq;
extern  MsgLayout       StartXtcInit65400Layout;

/* }}} */

typedef struct GetSFCountRep GetSFCountRep;
struct GetSFCountRep{
  uint16 SFcount;
} BCM_PACKING_ATTRIBUTE;
typedef GetSFCountRep GetSFCountMsgRep;
extern  MsgLayout GetSFCountLayout;

/* {{{ StartXtcRing */

#define XTCRING_ACTION_CONFIG_HW 0
#define XTCRING_ACTION_START     1
#define XTCRING_ACTION_STOP      2
#define XTCRING_ACTION_FLAGS_DISABLE_CLOCK_TERM (1<<0)
#define XTCRING_ACTION_FLAGS_DISABLE_DATA_TERM  (1<<1)
#define XTCRING_ACTION_FLAGS_DISABLE_TOP_LANES  (1<<2)

typedef struct XtcRingAction XtcRingAction;
struct XtcRingAction
{
  uint8  action;
  uint8  flags;
  uint16 p_delay;
  uint8  amplitude;
  uint8  reserved;
} BCM_PACKING_ATTRIBUTE;

typedef XtcRingAction StartXtcRingMsgReq;
extern  MsgLayout     StartXtcRingLayout;

/* }}} */
/* {{{ GetXtcRingResult */

#define XTC_IN_NR_PDLL         (16)
typedef struct XtcRingResult XtcRingResult;
struct XtcRingResult {
  uint16 p_delay;
  uint16 qdll_delay;
  uint16 cdll_delay;
  uint16 rdll_delay;
  uint16 pdll_delay[ XTC_IN_NR_PDLL ];
  uint32 dll_lock_status;
  uint8  ref_error;
  uint8  afe_clk_min;
  uint8  afe_clk_max;
  uint8  rx_termination;
  uint16 prev_crc_cnt_0_3;
  uint16 prev_crc_cnt_4_7;
  uint16 prev_crc_cnt_8_B;
  uint16 prev_crc_cnt_C_F;
  uint16 crc_cnt_0_3;
  uint16 crc_cnt_4_7;
  uint16 crc_cnt_8_B;
  uint16 crc_cnt_C_F;
  uint16 mux_config;
  uint16 tracking_count;
} BCM_PACKING_ATTRIBUTE;

typedef XtcRingResult GetXtcRingResultMsgRep;
extern  MsgLayout     GetXtcRingResultLayout;

/* }}} */
/* {{{ PerformI2C */

typedef struct I2CRequest I2CRequest;
struct I2CRequest {
  uint8 deviceId;
  uint8 address;
  uint8 numberBytes;
  uint8 isWriteAccess;  
  uint8 reserved[4];
  uint64 data;
} BCM_PACKING_ATTRIBUTE;

typedef struct I2CReply I2CReply;
struct I2CReply {
  uint64 data;
} BCM_PACKING_ATTRIBUTE;
typedef I2CRequest PerformI2CMsgReq;
typedef I2CReply   PerformI2CMsgRep;
extern  MsgLayout  PerformI2CLayout;

/* }}} */
/* {{{ DpllInbandRealign */
extern  MsgLayout    DpllInbandRealignLayout;

/* }}} */
/* {{{ SetDpllConfig */

/* Specific BCM65200 messages for vectoring */
#define DPLL_CONTROL_METHOD_DAC     0
#define DPLL_CONTROL_METHOD_SI514   1
#define DPLL_CONTROL_METHOD_SI570   2
#define DPLL_CONTROL_METHOD_TILMK61 3
#define DPLL_CONTROL_METHOD_SIT3521 4

/* may be overwritten to select another DCXO control method */
#ifndef DPLL_CONTROL_METHOD_I2C
# define DPLL_CONTROL_METHOD_I2C DPLL_CONTROL_METHOD_SI514 
#endif

#define DPLL_64KHZ_CLOCK 0
#define DPLL_128KHZ_CLOCK 1
#define DPLL_IN_BAND_CLOCK 2

typedef struct DpllConfig DpllConfig;
struct DpllConfig{
  uint8 activateSynchro; /* 1=activate
                            if DCXO is used
                              0=reset to value read at boot.
                              2=reset to value set at the last synchro with VP
                                if DCXO is used.
                            if VCXO is used
                              0,2=reset to mid-range DAC value.
                            3=open the DPLL loop
                         */
  uint8 controlMethod; /* 0 = DAC/VCXO, 
                          1 = I2C, Silabs Si514 DCXO, 
                          2 = I2C, Silabs Si570 DCXO,
                          3 = I2C, TI LMK61 DCXO
                       */
  uint8 i2cPhyAddr;    /* 0=default */
  uint8 clockType;     /* 0=64kHz, 1=128kHz, 2=in-band */
  uint8 inBandClockConfig; /* Only on BCM654xx platform
                              bit[0]: Indicate master or slave
                              bit[4:7]: Indicate the 10 G-KR to lock to.
                           */
  uint8 reserved[11];
} BCM_PACKING_ATTRIBUTE;
typedef DpllConfig SetDpllConfigMsgReq;
extern  MsgLayout    SetDpllConfigLayout;

/* }}} */
/* {{{ GetDpllStatus */

typedef struct DpllStatusReq DpllStatusReq;
struct DpllStatusReq{
  uint16 measurementTime64Period; /* duration of the dpll status monitoring
                                     expressed in 64kHz periods */
} BCM_PACKING_ATTRIBUTE;

#define MAX_DPLL_VALUES 100
typedef struct CoarseFineTupple CoarseFineTupple;
struct CoarseFineTupple{
  int16 fineRef;
  int16 coarseRef;
} BCM_PACKING_ATTRIBUTE;

typedef struct DpllStatusRep DpllStatusRep;
struct DpllStatusRep{
  CoarseFineTupple max64Ref;
  CoarseFineTupple maxHBref;
  uint16           outOfDelayLineCounter;
  CoarseFineTupple firstValues[MAX_DPLL_VALUES];
  uint16           refEdgeSeen;
} BCM_PACKING_ATTRIBUTE;

typedef DpllStatusReq GetDpllStatusMsgReq;
typedef DpllStatusRep GetDpllStatusMsgRep;
extern  MsgLayout     GetDpllStatusLayout;

/* }}} */
/* {{{ VpAlignInfo */


typedef struct XtcFrameMonRes XtcFrameMonRes;
struct XtcFrameMonRes
{
  int32 ds0Link0TxTime;  /* C140 */
  int32 ds0Link0RxTime;  /* C140 */
  int32 ds0Link1RxTime;
  int32 ds0Link2RxTime;
  int32 mUspDetection; /* C140 */
  int32 refPoint;      /* C140 (64kHz edge+xtcVpSyncDelay) */
  int32 reservedA; /* reserved */
  int32 measuredRttSw; /* C140 */
  int32 reservedB[1];
  int32 ds0CorrToVpClk; /* C140 BCM65450 only - indicate the distance between the 
			   arrival of the DS tone 0 correction and VP ref clock rising edge */ 
  uint64 mUspValues; /* mUsp counter values */
  uint32 timePrecision; /* note on BCM65450 this indicates the jitter seen on rx packets */
  uint32 reservedC[5];
} BCM_PACKING_ATTRIBUTE;

typedef struct VpAlignInfoReq VpAlignInfoReq;
struct VpAlignInfoReq{
  uint8 noPsnCheck;
  uint8 reserved[3];
} BCM_PACKING_ATTRIBUTE;

typedef struct VpAlignInfoRep VpAlignInfoRep;
struct VpAlignInfoRep{
  uint64 elapsedTimeSinceRefPointC70;                 /* elapsed time at 70.656Msamples w.r.t. reference time of this device */
  int32 vpAlignHbOffsetC70;                          /* misalignment at 70.656Msamples between expected and actual heart beat:
                                                         0 in case of perfect alignment */
  int32 symbolsBetweenPsn0AndRefPoint;               /* number of DMT symbols between device reference time and first PSN0 symbol */
  int32 symbolsBetweenObservedAndConfiguredPsn0;     /* number of DMT symbols between last observed PSN0 symbol index
                                                        and the expected PSN0 symbol index: should be 0
                                                        special values:
                                                        0x8000: no PSN change detected
                                                        0x8001: PSN change detected but position not accurately identified
                                                         */
  uint32 reserved;
  XtcFrameMonRes frameMonRes;
} BCM_PACKING_ATTRIBUTE;

typedef VpAlignInfoReq VpAlignInfoMsgReq;
typedef VpAlignInfoRep VpAlignInfoMsgRep;
extern  MsgLayout     VpAlignInfoLayout;

/* }}} */
/* {{{ AlignHBtoVP */

typedef struct AlignHBtoVPReq AlignHBtoVPReq;
struct AlignHBtoVPReq{
   int32 noFullEnable;
} BCM_PACKING_ATTRIBUTE;

typedef AlignHBtoVPReq  AlignHBtoVPMsgReq;
extern  MsgLayout       AlignHBtoVPLayout;

/* }}} */
/* {{{ PsSynchro */

typedef struct PsSynchro PsSynchro;
struct PsSynchro {
  uint16 pilotSequenceLengthDS; /* DS pilot sequence length (4*N) */
  uint16 currentPSN_512;        /* VP current PSN value (512 cycle) */
  uint16 currentPSN_4N;         /* VP current PSN value (4*N cycle) */
  uint16 reserved;
} BCM_PACKING_ATTRIBUTE;

typedef PsSynchro PilotSequenceSynchroMsgReq;
extern  MsgLayout PilotSequenceSynchroLayout;

/* }}} */
/* {{{ SetInterDeviceGmiiConfig */

# define BCM_MAX_GMII_DEV 8

typedef struct InterDeviceGmiiConfig InterDeviceGmiiConfig;
struct InterDeviceGmiiConfig {
  uint8 link[BCM_MAX_GMII_DEV]; /* BCM65200:
                                 *   2 bits per core, 12 cores per device (MAX_DEVICES = 3) as follows:
                                 *    link[0] bits 1..0 : link for destination core 0 on next device
                                 *    link[0] bits 3..2 : link for destination core 1 on next device
                                 *    link[0] bits 5..4 : link for destination core 2 on next device
                                 *    link[0] bits 7..6 : link for destination core 3 on next device
                                 *    link[1] bits 1..0 : link for destination core 4 on next device
                                 *    link[1] bits 3..2 : link for destination core 5 on next device
                                 *    link[1] bits 5..4 : link for destination core 6 on next device
                                 *    link[1] bits 7..6 : link for destination core 7 on next device
                                 *    link[2] bits 1..0 : link for destination core 8 on next device
                                 *    link[2] bits 3..2 : link for destination core 9 on next device
                                 *    link[2] bits 5..4 : link for destination core 10 on next device
                                 *    link[2] bits 7..6 : link for destination core 11 on next device
                                 *    link[3] bits 1..0 : link for destination core 0 on previous device
                                 *    link[3] bits 3..2 : link for destination core 1 on previous device
                                 *    link[3] bits 5..4 : link for destination core 2 on previous device
                                 *    link[3] bits 7..6 : link for destination core 3 on previous device
                                 *    link[4] bits 1..0 : link for destination core 4 on previous device
                                 *    link[4] bits 3..2 : link for destination core 5 on previous device
                                 *    link[4] bits 5..4 : link for destination core 6 on previous device
                                 *    link[4] bits 7..6 : link for destination core 7 on previous device
                                 *    link[5] bits 1..0 : link for destination core 8 on previous device
                                 *    link[5] bits 3..2 : link for destination core 9 on previous device
                                 *    link[5] bits 5..4 : link for destination core 10 on previous device
                                 *    link[5] bits 7..6 : link for destination core 11 on previous device
                                 *   The link parameter defines how the traffic towards
                                 *   the concerned core & device (=link index) must be routed:
                                 *      link = 0 : use GMII bus #0
                                 *      link = 1 : use GMII bus #1
                                 *      link = 3 : destination cannot be reached.
                                 * BCM65400Bx (not supported on A0):
                                 *   link[0] bits 3..0: number of devices in bonding ring
                                 *   link[0] bit 7: link connected to 'next' device (next device = thisDevice->deviceId + 1)
                                 *   link[n] one entry for each device in the ring in 'next' order (starting with 'next' device) 
                                 *           bits 4..0 : 1 bit per core on target device.
                                 *                       Set to 1 in order to pick the long route towards the target core.
                                 *                       Should be set to 0 normally. Can be helpfull in some load balancing schemes. 
                                 */
} BCM_PACKING_ATTRIBUTE;

typedef InterDeviceGmiiConfig SetInterDeviceGmiiConfigMsgReq;
extern MsgLayout SetInterDeviceGmiiConfigLayout;

/* predefined configurations */
#ifdef BCM_GMII_CONFIG
# define BCM65100_GMII_CONFIG_2(devId)      0
# define BCM65100_GMII_CONFIG_3(devId)      (1+((devId)%3))
# define BCM65100_GMII_CONFIG_4(devId)      (4+((devId)%4)) 
# define BCM65200_GMII_CONFIG_2(devId)      (8+((devId)%2))
# define BCM65200_GMII_CONFIG_3(devId)      (8+(((devId)%3)%2)) /* BCM65200_GMII_CONFIG_2(0) good for dev 0 & 2 */
# define BCM65200_GMII_CONFIG_2_1LNK(devId) 10
# define BCM65400_GMII_CONFIG_2(devId)      11
# define BCM65400_GMII_CONFIG_3(devId)      12
# define BCM65400_GMII_CONFIG_4(devId)      13

extern const SetInterDeviceGmiiConfigMsgReq defaultGmiiConfig[];
extern const int nbOfGmiiConfig;
int getGmiiConfigIndex(uint16 devId, uint16 nbOfDevices);

/* define some macro for standard configurations (2/3/4 devices inter-connected) */
# define BCM_GMII_CONFIG_2(devId) getGmiiConfigIndex(devId, 2)
# define BCM_GMII_CONFIG_3(devId) getGmiiConfigIndex(devId, 3)
# define BCM_GMII_CONFIG_4(devId) getGmiiConfigIndex(devId, 4)
#endif

/* }}} */
/* {{{ Set- and GetVceMacAddress */

#define DS_ES_ADDR_TYPE 0
#define US_ES_ADDR_TYPE 1
#define ES_SRC_ADDR_TYPE 2
#define PS_ADDR_TYPE 3
#define DS_VF_ADDR_TYPE 128
#define US_VF_ADDR_TYPE 129
#define VF_SRC_ADDR_TYPE 130
#define DRA_ADDR_TYPE    131

typedef struct VceMacAddress VceMacAddress;
struct VceMacAddress {
  uint8 mac[6];
  uint8 mode;
  uint8 addressType;    /* 0: DS Error samples VCE mac-address */
                        /* 1: US Error samples VCE mac-address */
                        /* 2: Source mac-address */
                        /* 3: US Pilot samples VCE mac-address */
                        /* 128: DS G.fast VF samples VCE mac-address */
                        /* 129: US G.fast VF samples VCE mac-address */
                        /* 130: G.fast VF samples source mac-address */
                        /* 131: DRA mac-address */
  uint16 vlan;
} BCM_PACKING_ATTRIBUTE ;

typedef VceMacAddress SetVceMacAddressMsgReq;
extern  MsgLayout     SetVceMacAddressLayout;

typedef VceMacAddress GetVceMacAddressMsgRep;
extern MsgLayout      GetVceMacAddressLayout;

/* }}} */
/* {{{ GetNtrStatus */

#define NTR_STATUS_ERROR_LEN 256

typedef struct NtrStatus NtrStatus;
struct NtrStatus
{
  int8   lineId;                /* line controlling the Ntr Xtal. lineId =  -1 is no control */
  int8   ntrStatus;             /* -1: free running, 
                                 *  0: acquiring lock,
                                 *  1: pre lock (jitter below TH),
                                 *  2: confirmed lock */
  int8   ntrMethod;             /* 0: Standard ADSL/VDSL,
                                   1: PhaseLocked */
  int8   maxError;              /* max error after lock has been declared */
  int8   minError;              /* min error after lock has been declared */
  int8   errorFormat;           /* shift of error sample unit wrt 35MHz 
                                 * (ie 2.2MHz unit will have shift value 
                                 * +4 = -log2(2.2/35)) */
  int8   ppsStatus;             /* -1: pps free running, 
                                 *  2: pps is locked, 
                                 *  5: pps in holdover */
  uint8  todCounter;            /*  0: default, 
                                 * >0: counts the nr of ToD messages from DSLAM
                                 *     after ppsStatus is valid (clamped to 255) */
  uint32 stdDev;                /* sigma of error over 10 second period */
  uint32 errorSampleId;         /* timestamp of the first error sample */
  uint32 lockCount;             /* nr of ticks in full lock */
  uint32 prelockCount;          /* number of seconds of continuous good lock
                                 * before declaring full lock */ 
  int16  offsetPpsNtr;          /* offset of NTR/PPS edge at the DSLAM side (in ticks @ 141MHz) */ 
  uint8  reserved[2];
  int32  phyOffsetFe;           /* offset of NE NTR clock wrt NE PHY sampling clock (in ppm*256) */
  /* Error sample buffer. Most recent sample (with errorSampleId) is in location
   * NTR_STATUS_ERROR_LEN-1 */
  int8   phaseErrorHistory[NTR_STATUS_ERROR_LEN]; 
}  BCM_PACKING_ATTRIBUTE ;

typedef NtrStatus GetNtrStatusMsgRep;
extern  MsgLayout GetNtrStatusLayout;

/* }}} */
/* {{{ AlignTddToPpsLayout */

typedef uint16    AlignTddToPpsMsgReq;
typedef int32     AlignTddToPpsMsgRep;
extern  MsgLayout AlignTddToPpsLayout;

const char* getTddLockStateName(int32 state);

/* }}} */
/* {{{ SetMDIOcontrol */

typedef struct SetMDIOcontrolReq SetMDIOcontrolReq;
struct SetMDIOcontrolReq
{
  uint8 enableMDIOpreamble;                
  uint8 MDCclockRate;           /* MDIO clock freq = 110/(MDCclockRate+1) MHz */
} BCM_PACKING_ATTRIBUTE ;

typedef SetMDIOcontrolReq SetMDIOcontrolMsgReq;
extern  MsgLayout         SetMDIOcontrolLayout;

/* }}} */
/* {{{ WriteMDIOregister */

typedef struct WriteMDIOregisterReq WriteMDIOregisterReq;
struct WriteMDIOregisterReq
{
  uint8  phyAddress;                
  uint8  regAddress;
  uint16 value;
} BCM_PACKING_ATTRIBUTE ;

typedef WriteMDIOregisterReq WriteMDIOregisterMsgReq;
extern MsgLayout             WriteMDIOregisterLayout;

/* }}} */
/* {{{ ReadMDIOregister */

typedef struct ReadMDIOregisterReq ReadMDIOregisterReq;
struct ReadMDIOregisterReq
{
  uint8  phyAddress;                
  uint8  regAddress;
} BCM_PACKING_ATTRIBUTE ;

typedef ReadMDIOregisterReq ReadMDIOregisterMsgReq;
typedef uint16              ReadMDIOregisterMsgRep;
extern MsgLayout            ReadMDIOregisterLayout;

/* }}} */
/* {{{ Set- and GetGlobalDtaConfig */

#define DEFAULT_DTA_CFG_HS_MDS         GFAST_TDD_PARAMS_MDS
#define DEFAULT_DTA_CFG_DTA_MDS        GFAST_TDD_PARAMS_MDS
#define DEFAULT_DTA_CFG_DTA_SMAX       8
#define DEFAULT_DTA_CFG_DTA_COUNTDOWN  8
#define DEFAULT_DTA_CFG_DRA_ENABLED    1
#define DEFAULT_DTA_CFG_NOI_CONTROL_ENABLED    1
#define DEFAULT_DTA_CFG_DTA_MIN_TIME   4

typedef struct GlobalDtaConfig GlobalDtaConfig;
struct GlobalDtaConfig
{
  uint8  hsMds;          /* handshake Mds value: number of downstream symbol positions in a TDD frame requested by the DRA when a line is training or when conflicting traffic requirements are observed */
  uint8  dtaMds;         /* preferred Mds value (DTA_Mds) : number of downstream symbol positions in a TDD frame requested by the DRA when no user traffic constraint applies */
  uint8  dtaSmax;        /* maximum step size for DTA changes (DTA_SMax) is the maximum change in Mds requested by the DRA for a single DTA update */
  uint8  dtaCountDown;   /* TDD countdown value for DTA. Range [4 15]. */
  uint8  draEnabled;     /* enable autonomous DRA based on internal traffic monitoring */ 
  uint8  noiControlEnabled;  /* bit 0 enables autonomous DS TTR change based on internal traffic monitoring
                              * bit 1 also enables US TTR change */
  uint16 dtaMinTime;     /* minimum time (in 0.01s unit) between two successive DTA */
} BCM_PACKING_ATTRIBUTE ;

typedef GlobalDtaConfig    SetGlobalDtaConfigMsgReq;
extern  MsgLayout          SetGlobalDtaConfigLayout;

typedef GlobalDtaConfig    GetGlobalDtaConfigMsgRep;
extern  MsgLayout          GetGlobalDtaConfigLayout;


/* }}} */

/* {{{ ControlTrafficReport service */
typedef struct TrafficControl TrafficControl;
struct TrafficControl {
  int32 enable;
} BCM_PACKING_ATTRIBUTE;

typedef TrafficControl     ControlTrafficReportMsgReq;
extern  MsgLayout          ControlTrafficReportLayout;
/* }}} */


/* {{{ SetExtraLdConfig */

/* All fields have same definition as in DeviceConfig */
typedef struct LineDriverDependentConfig LineDriverDependentConfig;
struct LineDriverDependentConfig
{ 
  TxGainDescriptor txGainDescriptor;
  uint16 txGainRef;
  int16  powerThreshold;
  LineDriverPowerModes lineDriverPowerModes;
  uint8  afeTxCtrlFlags;
  uint8  ldType;
  uint8  reservedA[2];
  int16  hwMaxOutputPowerVdsl;
  int16  hwMaxOutputPowerAdsl;
  uint8  reservedB[12];
} BCM_PACKING_ATTRIBUTE ;

typedef LineDriverDependentConfig SetExtraLdConfigMsgReq;
extern  MsgLayout                 SetExtraLdConfigLayout;

/* }}} */
/* {{{ SetLedPattern */

typedef struct LedPattern LedPattern;
struct LedPattern
{
  uint8   idlePattern[8];
  uint8   activatePattern[8];
  uint8   trainingPattern[8];
  uint8   showtimePattern[8];
  uint32  elapsedTime;
  uint8   enable;
  uint8   reserved[3];
} BCM_PACKING_ATTRIBUTE ;

typedef LedPattern SetLedPatternMsgReq;
extern  MsgLayout         SetLedPatternLayout;
/* }}} */
/* {{{ SetTxFilter */

# define BCM_TX_FILTER_LEN 49

typedef struct TxFilterDef TxFilterDef;
struct TxFilterDef 
{
  uint8 filterId;
  uint8 filterDescription[BCM_TX_FILTER_LEN];
} BCM_PACKING_ATTRIBUTE ;


typedef TxFilterDef SetTxFilterMsgReq;
#ifndef BCM_SWIG
extern MsgLayout SetTxFilterLayout;
#endif

/* }}} */
/* {{{ SetAfeConnection */

/* {{{ static BCM65100 configurations */

# define BCM65100_AFE_CONFIG_16(devId)         &bcm65100afeCfg[0]
# define BCM65100_AFE_CONFIG_16_IPDV9_1(devId) &bcm65100afeCfg[1]
# define BCM65100_AFE_CONFIG_16_OPT2(devId)    &bcm65100afeCfg[2]
# define BCM65100_AFE_CONFIG_965800(devId)     &bcm65100afeCfg[3]
# define BCM65100_AFE_CONFIG_8(devId)          &bcm65100afeCfg[4]
# define BCM65100_AFE_CONFIG_8_IPDV9_1(devId)  &bcm65100afeCfg[5]
# define BCM65100_AFE_CONFIG_8_OPT2(devId)     &bcm65100afeCfg[6]
# define BCM65100_AFE_CONFIG_8_965800(devId)   &bcm65100afeCfg[7]
# define BCM65100_AFE_CONFIG_24(devId)         &bcm65100afeCfg[(8+(devId)%2)]
# define BCM65100_AFE_CONFIG_24_3B(devId)      &bcm65100afeCfg[10]
# define BCM65100_AFE_CONFIG_32(devId)         &bcm65100afeCfg[11]
# define BCM65100_AFE_CONFIG_48(devId)         &bcm65100afeCfg[12+(devId)%2]

/* }}} */
/* {{{ static BCM65200 configurations */

/* VDSL configs with LD pairing line n and n+2 (=> BCM_AFE_TX_CTRL_FLAGS=16) */
# define BCM65200_AFE_CONFIG_36(devId)          &bcm65200afeCfg[0]
# define BCM65200_AFE_CONFIG_32(devId)          &bcm65200afeCfg[1]
# define BCM65200_AFE_CONFIG_24(devId)          &bcm65200afeCfg[2+(devId)%2]
# define BCM65200_AFE_CONFIG_24_6B(devId)       &bcm65200afeCfg[4]

/* VDSL configs with LD pairing line n and n+1 (=> BCM_AFE_TX_CTRL_FLAGS=0) */
# define BCM65200_AFE_CONFIG_STRAIGHT_LD_36(devId) &bcm65200afeCfg[5]
/* BCM65200_AFE_CONFIG_STRAIGHT_LD_72(0) is the same as BCM65200_AFE_CONFIG_STRAIGHT_LD_36 */
# define BCM65200_AFE_CONFIG_STRAIGHT_LD_72(devId) &bcm65200afeCfg[5+(devId)%2]
# define BCM65200_AFE_CONFIG_STRAIGHT_LD_32(devId) &bcm65200afeCfg[7]
# define BCM65200_AFE_CONFIG_STRAIGHT_LD_24(devId) &bcm65200afeCfg[8+(devId)%2]

/* GFAST config with BCM65940 (2 lines per AFE) */
# define BCM65200_AFE_CONFIG_6(devId)           &bcm65200afeCfg[10]
/* GFAST config for just 1 line (use AFE line 8) */
# define BCM65200_AFE_CONFIG_1(devId)           &bcm65200afeCfg[11]
/* GFAST config with BCM65246 */
# define BCM65200_AFE_CONFIG_6_4(devId)         &bcm65200afeCfg[12+(devId)%2]
/* GFAST config with BCM65244 -- only 4 DSP lines */
# define BCM65200_AFE_CONFIG_4(devId)           &bcm65200afeCfg[14]
/* GFAST config for 16L BLV: first 2 dev use BCM65200_AFE_CONFIG_6_4, last one
   uses BCM65200_AFE_CONFIG_4 */
# define BCM65200_AFE_CONFIG_16(devId)          &bcm65200afeCfg[12+(devId)%3]
/* special config for 652xxC0 bad substrate connections */
# define BCM65200_AFE_CONFIG_36_C0_PATCH(devId) &bcm65200afeCfg[15]
/* BCM965200_FT (IPDv10_FT) special config for VDSL6B FW */ 
# define BCM65200_AFE_CONFIG_IPDV10FT_6B(devId) &bcm65200afeCfg[16]
/* GFAST config for 8L BLV on DPF platform */
# define BCM65200_AFE_CONFIG_DPF_8(devId)       &bcm65200afeCfg[18+(devId)%2]
/* GFAST config for BCM965200_FT (IPDv10_FT) for 4 L */
# define BCM65200_AFE_CONFIG_IPDV10FT_4(devId)  &bcm65200afeCfg[18]
/* GFAST config for 4L BLV with BCM65940 (2 lines per AFE) */
# define BCM65200_AFE_CONFIG_4_2(devId)         &bcm65200afeCfg[20]
/* VDSL6B config for 4L on DPF2 platform */
# define BCM65200_AFE_CONFIG_DPF2_6B(devId)     &bcm65200afeCfg[21]
/* VDSL6B config for 12L CPE with optimized hw */
# define BCM65200_AFE_CONFIG_12(devId)          &bcm65200afeCfg[22]
/* VDSL6B config for 12L CPE with optimized hw and only 8L */
# define BCM65200_AFE_CONFIG_8(devId)           &bcm65200afeCfg[23]
/* VDSL6B config for 24L with BCM65936 as on BCM965200_65936 */
# define BCM65200_AFE_CONFIG_24_6B_65936(devId) &bcm65200afeCfg[24]
# define BCM65200_AFE_CONFIG_48_6B_65936(devId) &bcm65200afeCfg[25+(devId)%2]
/* VDSL6B config for 16L with BCM65936 as on BCM965200_65936 */
# define BCM65200_AFE_CONFIG_16_6B_65936(devId) &bcm65200afeCfg[27]
/* VDSL5B config for 18L with BCM65936 as on BCM965200_65936 */
# define BCM65200_AFE_CONFIG_36_5B_65936(devId) &bcm65200afeCfg[28]

/* }}} */
/* {{{ static BCM65400 configurations */

/* GFAST config for 8 lines */
# define BCM65400_AFE_CONFIG_8(devId)          &bcm65400afeCfg[0]

/* AFE config is mandatory on BCM654xx -> enforce it */
# if defined BCM65400_SUPPORT && !defined BCM65400_AFE_CONFIG
#  define BCM65400_AFE_CONFIG BCM65400_AFE_CONFIG_8
# endif

/* }}} */
/* {{{ static BCM65450 configurations */

/* GFAST config for 8 lines */
# define BCM65450_AFE_CONFIG_16(devId)         &bcm65450afeCfg[0] /* 16L G.fast */
# define BCM65450_AFE_CONFIG_8(devId)          &bcm65450afeCfg[1] /*  8L G.fast */
# define BCM65450_AFE_CONFIG_36(devId)         &bcm65450afeCfg[2] /* 36L VDSL */

/* }}} */

# ifndef BCM_AFE_CONNECT_SIZE
#  define BCM_AFE_CONNECT_SIZE 99
# endif
# define BCM_AFE_CONNECT_PROFILE_OFFSET 72

struct SetAfeConnection
{
  uint8 data[BCM_AFE_CONNECT_SIZE];
} BCM_PACKING_ATTRIBUTE ;

#ifndef BCM_SWIG
typedef struct SetAfeConnection SetAfeConnectionMsgReq;
extern MsgLayout SetAfeConnectionLayout;
extern const SetAfeConnectionMsgReq bcm65100afeCfg[];
extern const unsigned int nbOfBcm65100afeCfg;
extern const SetAfeConnectionMsgReq bcm65200afeCfg[];
extern const unsigned int nbOfBcm65200afeCfg;
extern const SetAfeConnectionMsgReq bcm65400afeCfg[];
extern const unsigned int nbOfBcm65400afeCfg;
extern const SetAfeConnectionMsgReq bcm65450afeCfg[];
extern const unsigned int nbOfBcm65450afeCfg;
#endif

const SetAfeConnectionMsgReq* getAfeConnectMsg(int devId, int coreId);

# ifndef BCM_AFE_CONFIG
#  define BCM_AFE_CONFIG getAfeConnectMsg
# endif

#ifndef SUPPORT_HMI
/* }}} */
/* {{{ SetCommonBandPlans */

typedef struct CommonBandPlans CommonBandPlans;
struct CommonBandPlans
{
  uint8 checkFlag;
  uint8 groupMethod; /* Create bandplan with a constant multiple of tones per
                        band
                        0: 16 tones
                        1: 32 tones
                     */
  uint16 splitToneDs;
  BandPlanDescriptor bandPlanDn;
  BandPlanDescriptor bandPlanUp;
  BandPlanDescriptor bandPlanDnFast;
  BandPlanDescriptor bandPlanUpFast;
  uint16 splitToneUs;
} BCM_PACKING_ATTRIBUTE;

typedef CommonBandPlans SetCommonBandPlansMsgReq;
extern  MsgLayout       SetCommonBandPlansLayout;

/* }}} */
/* {{{ SetVceVersion */

typedef struct VceVersion VceVersion;
struct VceVersion{
  /* Contains 10 bytes as defined in O-SIGNATURE */
  uint8 countryCode[2];
  uint8 vendorId[4];
  uint8 versionNumber[4];
} BCM_PACKING_ATTRIBUTE;

typedef VceVersion  SetVceVersionMsgReq;
extern  MsgLayout   SetVceVersionLayout;

/* }}} */
/* {{{ GetVectoringStates */

typedef struct VectoringStatesReq VectoringStatesReq;
struct VectoringStatesReq{
  uint8 source;      /* 0 ignored the toggle bit - Always return the next value
                        1-2 valid sources */
  uint8 reservedA;
  uint8 toggleBit;   /* If the value is identical to the value sent in the previous request with the same source,
                        the reply of the previous request at the same source is resent.
                        If the value differs, the current values are sent in the reply. */

  uint8 reservedB;  
} BCM_PACKING_ATTRIBUTE;

#define INT_VECT_STATE_MASK              0x01
#define INT_VECT_RX_GAIN_DS_MASK         0x02
#define INT_VECT_NON_ZERO_TIGARESP_MASK  0x04
#define INT_VECT_RX_GAIN_US_MASK         0x08
#define INT_VECT_RESET_EVENT_MASK        0x10
#define INT_VECT_RX_GAIN_DS_FORCED_MASK  0x20
#define INT_VECT_OLR_MASK (INT_VECT_RX_GAIN_DS_MASK|INT_VECT_NON_ZERO_TIGARESP_MASK|INT_VECT_RX_GAIN_US_MASK|INT_VECT_RX_GAIN_DS_FORCED_MASK)
#define INT_VECT_DS_PROBESEQ_UPDATE_MASK 0x40
#define INT_VECT_US_PROBESEQ_UPDATE_MASK 0x80
#define INT_VECT_PROBESEQ_UPDATE_MASK (INT_VECT_DS_PROBESEQ_UPDATE_MASK|INT_VECT_US_PROBESEQ_UPDATE_MASK)

#define INT_SLM_LINE_CHANGED_MASK 0x80
#define INT_SLM_LINE_MASK         0x7f
typedef struct VectoringStates VectoringStates;
struct VectoringStates{
  uint8 toggleBitValue;
  uint8 slmLine;     /* b[7]=1 if the device has toggled to SLM
                        b[6:0] indicates the line in SLM */
  uint8 reserved[2];
  uint8 vecState[BCM_DEV_NB_LINES];
  uint8 vecLineStateChanges[BCM_DEV_NB_LINES];
} BCM_PACKING_ATTRIBUTE;

typedef VectoringStatesReq GetVectoringStatesMsgReq;
typedef VectoringStates GetVectoringStatesMsgRep;
extern  MsgLayout       GetVectoringStatesLayout;
#endif  /* SUPPORT_HMI */

/* }}} */
/* {{{ SetRuntimeDisableBitswap */

typedef struct RuntimeDisableBitswap RuntimeDisableBitswap;
struct RuntimeDisableBitswap{
    uint64 lineIds;
    uint8  direction;
    uint8  commandId;
    uint8  reserved[6];
} BCM_PACKING_ATTRIBUTE;

typedef RuntimeDisableBitswap SetRuntimeDisableBitswapMsgReq;
extern  MsgLayout             SetRuntimeDisableBitswapLayout;

/* }}} */
/* {{{ SerdesConfigEyeScan */

typedef struct SerdesAddressLane SerdesAddressLane;
struct SerdesAddressLane
{
    int32 phyAddress;   /* see SERDES_xxx_PHY_ADDR */
    int32 lane;         /* 0-3 for bonding, XGXS or Merlin, 0-2 for XTC*/ 
} BCM_PACKING_ATTRIBUTE;

/* For eye scan only, the msb of the phyAddress relaxes the lane state check (allows eye measurement on PRBS signal) */
typedef SerdesAddressLane  SerdesConfigEyeScanMsgReq;
extern  MsgLayout          SerdesConfigEyeScanLayout;

/* }}} */
/* {{{ SerdesGetEyeScanResult */

typedef struct SerdesEyeScanRep SerdesEyeScanRep;
struct SerdesEyeScanRep
{
    int32 femtosecsPerDivision;
    int16 voltages[64];             
    int16 measurementOngoing;       /* 1 while measuring, 0 when results are available */
    int16 reserved;
} BCM_PACKING_ATTRIBUTE;
typedef SerdesEyeScanRep   SerdesGetEyeScanResultMsgRep;
extern MsgLayout           SerdesGetEyeScanResultLayout;

/* }}} */
/* {{{ SerdesGetEyeScanData */

typedef struct SerdesEyeScanDataRep SerdesEyeScanDataRep;
struct SerdesEyeScanDataRep
{
    int8 data[488]; 
}BCM_PACKING_ATTRIBUTE;
typedef uint8                SerdesGetEyeScanDataMsgReq;
typedef SerdesEyeScanDataRep SerdesGetEyeScanDataMsgRep;
extern  MsgLayout            SerdesGetEyeScanDataLayout;

/* }}} */
/* {{{ SerdesConfigEyeDensity */

typedef SerdesAddressLane  SerdesConfigEyeDensityMsgReq;
extern  MsgLayout          SerdesConfigEyeDensityLayout;

/* }}} */
/* {{{ SerdesConfigGetEyeDensityData */

typedef SerdesEyeScanRep   SerdesGetEyeDensityResultMsgRep;
extern  MsgLayout          SerdesGetEyeDensityResultLayout;

/* }}} */
/* {{{ SerdesGetEyeDensityData */

typedef uint8                SerdesGetEyeDensityDataMsgReq;
typedef SerdesEyeScanDataRep SerdesGetEyeDensityDataMsgRep;
extern  MsgLayout            SerdesGetEyeDensityDataLayout;

/* }}} */
/* {{{ SerdesGetLaneStateExtended */

typedef struct SerdesLaneStateExt  SerdesLaneStateExt;
struct SerdesLaneStateExt {
  uint16 ucv_config;            /* uC lane configuration */
  int16 rx_ppm;                 /* Frequency offset of local reference clock with respect to RX data in ppm */
  int16 p1_lvl;                 /* Vertical threshold voltage of p1 slicer (mV) */
  int16 m1_lvl;                 /* Vertical threshold voltage of m1 slicer (mV) */
  uint16 link_time;             /* Link time in milliseconds */
  /** OSR Mode */
  uint8 tx;                     /* TX OSR Mode */
  uint8 rx;                     /* RX OSR Mode */
  uint8 tx_rx;                  /* OSR Mode for TX and RX (used when both TX and RX should have same OSR Mode) */
  uint8 sig_det;                /* Signal Detect */
  uint8 rx_lock;                /* PMD RX Lock */
  int8 clk90;                   /* Delay of zero crossing slicer, m1, wrt to data in PI codes */
  int8 clkp1;                   /* Delay of diagnostic/lms slicer, p1, wrt to data in PI codes */
  int8 pf_main;                 /* Peaking Filter Main Settings */
  int8 pf_hiz;                  /* Peaking Filter Hiz mode enable */
  int8 pf_bst;                  /* Peaking Filter DC gain adjustment for CTLE */
  int8 pf2_ctrl;                /* Low Frequency Peaking filter control */
  int8 vga;                     /* Variable Gain Amplifier settings */
  int8 dc_offset;               /* DC offset DAC control value */
  int8 p1_lvl_ctrl;             /* P1 eyediag status */
  int8 dfe1;                    /* DFE tap 1 value */
  int8 dfe2;                    /* DFE tap 2 value */
  int8 dfe3;                    /* DFE tap 3 value */
  int8 dfe4;                    /* DFE tap 4 value */
  int8 dfe5;                    /* DFE tap 5 value */
  int8 dfe6;                    /* DFE tap 6 value */
  int8 dfe1_dcd;                /* DFE tap 1 Duty Cycle Distortion */
  int8 dfe2_dcd;                /* DFE tap 2 Duty Cycle Distortion */
  int8 pe;                      /* Slicer calibration control codes (p1 even) */
  int8 ze;                      /* Slicer calibration control codes (data even) */
  int8 me;                      /* Slicer calibration control codes (m1 even) */
  int8 po;                      /* Slicer calibration control codes (p1 odd) */
  int8 zo;                      /* Slicer calibration control codes (data odd) */
  int8 mo;                      /* Slicer calibration control codes (m1 odd) */
  int16 tx_ppm;                 /* Frequency offset of local reference clock with respect to TX data in ppm */
  int8 txfir_pre;               /* TX equalization FIR pre tap weight */
  int8 txfir_main;              /* TX equalization FIR main tap weight */
  int8 txfir_post1;             /* TX equalization FIR post1 tap weight */
  int8 txfir_post2;             /* TX equalization FIR post2 tap weight */
  int8 txfir_post3;             /* TX equalization FIR post3 tap weight */
  uint8 reservedA;
  uint16 heye_left;             /* Horizontal left eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 heye_right;            /* Horizontal right eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 veye_upper;            /* Vertical upper eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 veye_lower;            /* Vertical lower eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint8 br_pd_en;               /* Baud Rate Phase Detector enable */
  uint8 reset_state;            /* lane_reset_state **/
  uint8 stop_state;             /* uC stopped state **/
  uint8 reservedB;
} BCM_PACKING_ATTRIBUTE; 

typedef SerdesAddressLane  SerdesGetLaneStateExtendedMsgReq;
typedef SerdesLaneStateExt SerdesGetLaneStateExtendedMsgRep;
extern  MsgLayout          SerdesGetLaneStateExtendedLayout;

typedef struct SerdesLaneStateMerlin16Ext  SerdesLaneStateMerlin16Ext;
struct SerdesLaneStateMerlin16Ext {
  uint16 ucv_config;  /** uC lane configuration */
  uint8 ucv_status;  /** uC lane status */
  uint8 reservedA;
  int16 rx_ppm;  /** Frequency offset of local reference clock with respect to RX data in ppm */
  int16 p1_lvl;  /** Vertical threshold voltage of p1 slicer (mV) */
  int16 m1_lvl;  /** Vertical threshold voltage of m1 slicer (mV) [Used to read out 'channel loss hint' in PAM4 mode] */
  uint16 link_time;  /** Link time in milliseconds */
  uint8 tx;                     /* TX OSR Mode */
  uint8 rx;                     /* RX OSR Mode */
  uint8 tx_rx;                  /* OSR Mode for TX and RX (used when both TX and RX should have same OSR Mode) */
  uint8 sig_det;  /** Signal Detect */
  uint8 sig_det_chg;  /** Signal Detect Change */
  uint8 rx_lock;  /** PMD RX Lock */
  uint8 rx_lock_chg;  /** PMD RX Lock Change */
  int8 clk90;  /** Delay of zero crossing slicer, m1, wrt to data in PI codes */
  int8 clkp1;  /** Delay of diagnostic/lms slicer, p1, wrt to data in PI codes */
  int8 pf_main;  /** Peaking Filter Main Settings */
  int8 pf_hiz;  /** Peaking Filter Hiz mode enable */
  int8 pf2_ctrl;  /** Low Frequency Peaking filter control */
  int8 vga;  /** Variable Gain Amplifier settings */
  int8 dc_offset;  /** DC offset DAC control value */
  int8 p1_lvl_ctrl;  /** P1 eyediag status */
  int8 dfe1;  /** DFE tap 1 value */
  int8 dfe2;  /** DFE tap 2 value */
  int8 dfe3;  /** DFE tap 3 value */
  int8 dfe4;  /** DFE tap 4 value */
  int8 dfe5;  /** DFE tap 5 value */
  int8 dfe6;  /** DFE tap 6 value */
  int8 dfe1_dcd;  /** DFE tap 1 Duty Cycle Distortion */
  int8 dfe2_dcd;  /** DFE tap 2 Duty Cycle Distortion */
  int8 pe;  /** Slicer calibration control codes (p1 even) */
  int8 ze;  /** Slicer calibration control codes (data even) */
  int8 me;  /** Slicer calibration control codes (m1 even) */
  int8 po;  /** Slicer calibration control codes (p1 odd) */
  int8 zo;  /** Slicer calibration control codes (data odd) */
  int8 mo;  /** Slicer calibration control codes (m1 odd) */
  uint8 reservedB;
  int16 tx_ppm;  /** Frequency offset of local reference clock with respect to TX data in ppm */
  int8 txfir_pre;  /** TX equalization FIR pre tap weight */
  int8 txfir_main;  /** TX equalization FIR main tap weight */
  int8 txfir_post1;  /** TX equalization FIR post1 tap weight */
  int8 txfir_post2;  /** TX equalization FIR post2 tap weight */
  int8 txfir_post3;  /** TX equalization FIR post3 tap weight */
  int8 txfir_rpara;  /** TX equalization FIR rpara tap weight */
  uint16 heye_left;  /** Horizontal left eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 heye_right;  /** Horizontal right eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 veye_upper;  /** Vertical upper eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 veye_lower;  /** Vertical lower eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint8 br_pd_en;  /** Baud Rate Phase Detector enable */
  uint8 reset_state;  /** lane_reset_state **/
  uint8 tx_reset_state;  /** lane_tx_reset_state **/
  uint8 stop_state;  /** uC stopped state **/
  uint8 soc_pos;  /** Sigdet offset correction - positive **/
  uint8 soc_neg;  /** Sigdet offset correction - negative **/
} BCM_PACKING_ATTRIBUTE ;

typedef SerdesAddressLane  SerdesGetLaneStateMerlin16ExtendedMsgReq;
typedef SerdesLaneStateMerlin16Ext SerdesGetLaneStateMerlin16ExtendedMsgRep;
extern  MsgLayout          SerdesGetLaneStateMerlin16ExtendedLayout;

typedef struct SerdesLaneStateFalcon16Ext  SerdesLaneStateFalcon16Ext;
struct SerdesLaneStateFalcon16Ext {
  uint16 ucv_config;  /** uC lane configuration */
  uint8 ucv_status;  /** uC lane status */
  uint8 reservedA;
  int16 rx_ppm;  /** Frequency offset of local reference clock with respect to RX data in ppm */
  int16 p1_lvl;  /** Vertical threshold voltage of p1 slicer (mV) */
  int16 m1_lvl;  /** Vertical threshold voltage of m1 slicer (mV) [Used to read out 'channel loss hint' in PAM4 mode] */
  uint16 link_time;  /** Link time in milliseconds */
  uint8 tx;          /* TX OSR Mode */
  uint8 rx;          /* RX OSR Mode */
  uint8 tx_rx;       /* OSR Mode for TX and RX (used when both TX and RX should have same OSR Mode) */
  uint8 sig_det;  /** Signal Detect */
  uint8 sig_det_chg;  /** Signal Detect Change */
  uint8 rx_lock;  /** PMD RX Lock */
  uint8 rx_lock_chg;  /** PMD RX Lock Change */
  int8 clk90;  /** Delay of zero crossing slicer, m1, wrt to data in PI codes */
  int8 clkp1;  /** Delay of diagnostic/lms slicer, p1, wrt to data in PI codes */
  int8 pf_main;  /** Peaking Filter Main Settings */
  int8 pf_hiz;  /** Peaking Filter Hiz mode enable */
  int8 pf2_ctrl;  /** Low Frequency Peaking filter control */
  int8 vga;  /** Variable Gain Amplifier settings */
  int8 dc_offset;  /** DC offset DAC control value */
  int8 p1_lvl_ctrl;  /** P1 eyediag status */
  int8 dfe1;  /** DFE tap 1 value */
  int8 dfe2;  /** DFE tap 2 value */
  int8 dfe3;  /** DFE tap 3 value */
  int8 dfe4;  /** DFE tap 4 value */
  int8 dfe5;  /** DFE tap 5 value */
  int8 dfe6;  /** DFE tap 6 value */
  int8 dfe1_dcd;  /** DFE tap 1 Duty Cycle Distortion */
  int8 dfe2_dcd;  /** DFE tap 2 Duty Cycle Distortion */
  uint8 reservedB;
  int16 tx_ppm;  /** Frequency offset of local reference clock with respect to TX data in ppm */
  int8 txfir_pre;  /** TX equalization FIR pre tap weight */
  int8 txfir_main;  /** TX equalization FIR main tap weight */
  int8 txfir_post1;  /** TX equalization FIR post1 tap weight */
  int8 txfir_post2;  /** TX equalization FIR post2 tap weight */
  int8 txfir_post3;  /** TX equalization FIR post3 tap weight */
  int8 txfir_rpara;  /** TX equalization FIR rpara tap weight */
  uint16 heye_left;  /** Horizontal left eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 heye_right;  /** Horizontal right eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 veye_upper;  /** Vertical upper eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint16 veye_lower;  /** Vertical lower eye margin @ 1e-5 as seen by internal diagnostic slicer in mUI and mV */
  uint8 br_pd_en;  /** Baud Rate Phase Detector enable */
  uint8 reset_state;  /** lane_reset_state **/
  uint8 tx_reset_state;  /** lane_tx_reset_state **/
  uint8 stop_state;  /** uC stopped state **/
  uint8 soc_pos;  /** Sigdet offset correction - positive **/
  uint8 soc_neg;  /** Sigdet offset correction - negative **/
} BCM_PACKING_ATTRIBUTE ;

typedef SerdesAddressLane  SerdesGetLaneStateFalcon16ExtendedMsgReq;
typedef SerdesLaneStateMerlin16Ext SerdesGetLaneStateFalcon16ExtendedMsgRep;
extern  MsgLayout          SerdesGetLaneStateFalcon16ExtendedLayout;


/* }}} */
/* {{{ SerdesGetLaneStates */

/* BCM65200 PHY addresses */
#define SERDES_BONDING_PHY_ADDR          1
#define SERDES_XTC_PHY_ADDR              2

/* BCM65400 PHY addresses */
#define SERDES_XGXS_A0_PHY_ADDR   1
#define SERDES_MERLIN_B_PHY_ADDR  2
#define SERDES_XGXS_A1_PHY_ADDR   3
#define SERDES_MERLIN_C_PHY_ADDR  4

/* BCM65450 PHY addresses */
#define SERDES_FALCON_0_PHY_ADDR  1
#define SERDES_FALCON_1_PHY_ADDR  2
#define SERDES_FALCON_2_PHY_ADDR  3
#define SERDES_MERLIN_PHY_ADDR    4
//const char* getSerdesPhyName(ChipFamily chipFmly, uint16 addr);

#define NR_OF_XTC_COUNTER_LINKS          2

/* Serdes lane state values */
#define SERDES_STATE_NO_SIGNAL_DETECT    0
#define SERDES_STATE_SIGNAL_DETECT       1
#define SERDES_STATE_RX_PMD_LOCK         3
#define SERDES_STATE_LINK_UP             7
#define SERDES_STATE_NOT_AVAILABLE      16
#define SERDES_STATE_POWERED_DOWN       32
#define SERDES_STATE_IN_RESET           64
const char* getSerdesStateName(uint16 state);

/* Serdes lane protocol config */
#define SERDES_KR            3
#define SERDES_XFI           4
#define SERDES_SFI           5
#define SERDES_SFI_LONG      6

/* Serdes lane speed values */
#define SERDES_SPEED_100M         0
#define SERDES_SPEED_1G           1
#define SERDES_SPEED_2G5          2
#define SERDES_SPEED_10G          0x00
#define SERDES_SPEED_20G          0x08
#define SERDES_SPEED_25G          0x18
#define SERDES_SPEED_10G_KR       (SERDES_SPEED_10G | SERDES_KR)
#define SERDES_SPEED_10G_XFI      (SERDES_SPEED_10G | SERDES_XFI)
#define SERDES_SPEED_10G_SFI      (SERDES_SPEED_10G | SERDES_SFI)
#define SERDES_SPEED_10G_SFI_LONG (SERDES_SPEED_10G | SERDES_SFI_LONG)
#define SERDES_SPEED_20G_KR       (SERDES_SPEED_20G | SERDES_KR)      
#define SERDES_SPEED_20G_XFI      (SERDES_SPEED_20G | SERDES_XFI)     
#define SERDES_SPEED_20G_SFI      (SERDES_SPEED_20G | SERDES_SFI)     
#define SERDES_SPEED_20G_SFI_LONG (SERDES_SPEED_20G | SERDES_SFI_LONG)
#define SERDES_SPEED_25G_KR       (SERDES_SPEED_25G | SERDES_KR)      
#define SERDES_SPEED_25G_XFI      (SERDES_SPEED_25G | SERDES_XFI)     
#define SERDES_SPEED_25G_SFI      (SERDES_SPEED_25G | SERDES_SFI)     
#define SERDES_SPEED_25G_SFI_LONG (SERDES_SPEED_25G | SERDES_SFI_LONG)
const char* getSerdesSpeedName(uint16 speed);

#define BCM_NB_SERDES_LANE               4
typedef struct SerdesLaneStates SerdesLaneStates;
struct SerdesLaneStates {
  int32 pllLock;
  int32 laneStateArray[BCM_NB_SERDES_LANE]; /* Lane states: see above */
  int32 laneSpeedArray[BCM_NB_SERDES_LANE]; /* Lane speeds: see above */
} BCM_PACKING_ATTRIBUTE;

typedef int32              SerdesGetLaneStatesMsgReq;
typedef SerdesLaneStates   SerdesGetLaneStatesMsgRep;
extern  MsgLayout          SerdesGetLaneStatesLayout;

/* }}} */
/* {{{ SerdesStartPrbs */

typedef struct SerdesPrbsConfig SerdesPrbsConfig;
struct SerdesPrbsConfig {
  int32 phyAddressGen;          /* Generator phy address*/ 
  int32 phyAddressCheck;        /* Checker phy address */
  int32 laneGen;                /* Generator lane*/
  int32 laneCheck;              /* Checker lane */
  int32 polyMode;               /* Determines the polynomial used when generating the prbs pattern the higher the number the more complicated the
                                 * polynomial (higher order) which implies longer runs of zeroes/ones */
  int32 enableLoopback;         /* 1 for internal loopback */
  int32 duration;
} BCM_PACKING_ATTRIBUTE;

typedef SerdesPrbsConfig        SerdesStartPrbsMsgReq;
typedef uint32                  SerdesStartPrbsMsgRep;
extern  MsgLayout               SerdesStartPrbsLayout;

/* }}} */
/* {{{ SerdesGetPrbsResult */

typedef struct SerdesPrbsResult SerdesPrbsResult;
struct SerdesPrbsResult {
  int32 lock;                   /* 0 if not locked, errorCount will be meaningless in this case */
  int32 errorCount;             /* errorCount */
  uint32 timestamp;             /* timestamp */
} BCM_PACKING_ATTRIBUTE;

typedef SerdesAddressLane       SerdesGetPrbsResultMsgReq;
typedef SerdesPrbsResult        SerdesGetPrbsResultMsgRep;
extern  MsgLayout               SerdesGetPrbsResultLayout;

/* }}} */
/* {{{ SerdesRead */

struct SerdesReadReq {
    int32 phyAddress;
    int32 lane;
    int32 aerBits; 
    int32 nonIEEEReg; /* 0: IEEE;  1: NON_IEEE */
    int32 address;
} BCM_PACKING_ATTRIBUTE;
typedef struct SerdesReadReq SerdesReadMsgReq;
typedef uint16               SerdesReadMsgRep;
extern  MsgLayout            SerdesReadLayout;

/* }}} */
/* {{{ SerdesWrite */

struct SerdesWriteReq {
    int32 phyAddress;
    int32 lane;
    int32 aerBits; 
    int32 nonIEEEReg; /* 0: IEEE;  1: NON_IEEE */
    int32 address;
    int32 value;
} BCM_PACKING_ATTRIBUTE;
typedef struct SerdesWriteReq SerdesWriteMsgReq;
extern  MsgLayout             SerdesWriteLayout;

/* }}} */
/* {{{ SerdesUpdate */

struct SerdesUpdateReq {
    int32 phyAddress;
    int32 lane;
    int32 aerBits; 
    int32 nonIEEEReg; /* 0: IEEE;  1: NON_IEEE */
    int32 address;
    int32 value;
    int32 mask;
} BCM_PACKING_ATTRIBUTE;
typedef struct SerdesUpdateReq SerdesUpdateMsgReq;
extern  MsgLayout              SerdesUpdateLayout;

/* }}} */
/* {{{ SerdesLoopback */

# define SERDES_LOOPBACK_PCS        0
# define SERDES_LOOPBACK_PMD        1
# define SERDES_LOOPBACK_REMOTE_PCS 2
# define SERDES_LOOPBACK_REMOTE_PMD 3

typedef struct SerdesLoopbackReq SerdesLoopbackReq;
struct SerdesLoopbackReq {
    int32 phyAddress;
    int32 lane;
    int32 loopbackMode;  /* Loopback modes: 
                          * 0: PCS level internal loopback
                          * 1: PMD level internal loopback
                          * 2: PMD level external loopback
                          * adding bit 0x80 disables CL72 in loopback if enabled
                          */
    int32 disableLoopback;
} BCM_PACKING_ATTRIBUTE;
typedef struct SerdesLoopbackReq SerdesLoopbackMsgReq;
extern  MsgLayout                SerdesLoopbackLayout;

/* }}} */
/* {{{ SerdesInvertPolarity */

typedef struct SerdesInvertPolarityReq SerdesInvertPolarityReq;
struct SerdesInvertPolarityReq {
    int32 phyAddress;
    int32 lane;
    int32 rxTxBoth;      /* Loopback modes: 
                          * 0: Rx
                          * 1: Tx
                          * 2: Both
                          */
} BCM_PACKING_ATTRIBUTE;
typedef struct SerdesInvertPolarityReq SerdesInvertPolarityMsgReq;
extern  MsgLayout                      SerdesInvertPolarityLayout;

/* }}} */
/* {{{ SerdesResetReq */

typedef SerdesAddressLane  SerdesResetMsgReq;
extern  MsgLayout             SerdesResetLayout;

/* }}} */
/* {{{ SerdesSetLane */

typedef struct SerdesSetLaneReq SerdesSetLaneReq;
struct SerdesSetLaneReq
{
  int32 phyAddress;
  int32 lane; 
  int32 enable;       /* 0: Disable lane
                       * 1: Enable lane
                       * 2: PowerDown lane
                       * 3: PowerUp lane
                       */
} BCM_PACKING_ATTRIBUTE;

typedef SerdesSetLaneReq      SerdesSetLaneMsgReq;
extern  MsgLayout             SerdesSetLaneLayout;

/* }}} */
/* {{{ SerdesDisablePrbs */

typedef struct SerdesDisablePrbsReq SerdesDisablePrbsReq;
struct SerdesDisablePrbsReq
{
  int32 phyAddress;
  int32 lane; 
  int32 disableBitmap;/* b0: disable generator
                         b1: disable checker */
} BCM_PACKING_ATTRIBUTE;

typedef SerdesDisablePrbsReq  SerdesDisablePrbsMsgReq;
extern  MsgLayout             SerdesDisablePrbsLayout;

/* }}} */
/* {{{ SerdesDump8051 */

typedef struct Serdes8051RegDump Serdes8051RegDump;
struct Serdes8051RegDump {
  int32      length;
  uint8      data[256];
};
typedef SerdesAddressLane     SerdesDump8051MsgReq; 
typedef Serdes8051RegDump     SerdesDump8051MsgRep; 
extern  MsgLayout             SerdesDump8051Layout;

/* }}} */
/* {{{ SerdesReadPmdRegisters */

typedef struct SerdesDumpPmd SerdesDumpPmd;
struct SerdesDumpPmd
{
  int32 phyAddress;
  int32 lane;
  int32 baseAddress;
  int32 numRegisters;
};

typedef struct SerdesDumpPmdData SerdesDumpPmdData;
struct SerdesDumpPmdData
{
  uint16 data[256];
};

typedef SerdesDumpPmdData     SerdesReadPmdRegistersMsgRep;
typedef SerdesDumpPmd         SerdesReadPmdRegistersMsgReq;
extern  MsgLayout             SerdesReadPmdRegistersLayout;

/* }}} */
/* {{{ SerdesDumpEventList */

typedef struct SerdesDumpEvents SerdesDumpEvents;
struct SerdesDumpEvents 
{
  uint8 data[744];
};

typedef int32                 SerdesDumpEventListMsgReq;
typedef SerdesDumpEvents      SerdesDumpEventListMsgRep;
extern  MsgLayout             SerdesDumpEventListLayout;

/* }}} */
/* {{{ SerdesGetCoreState */

struct SerdesCoreState {
  uint8  core_reset;            /* Core DP Reset State */
  uint8  pll_pwrdn;             /* PLL Powerdown enable */
  uint8  uc_active;             /* Micro active enable */
  uint8  reservedA;
  uint16 comclk_mhz;            /* Comclk Frequency in Mhz */
  uint16 ucode_version;         /* uCode Major Version number */
  uint8  ucode_minor_version;   /* uCode Minor Version number */
  uint8  afe_hardware_version;  /* AFE Hardware version */
  uint8  temp_idx;              /* uC Die Temperature Index */
  uint8  reservedB;
  uint16 avg_tmon;              /* Average Die Temperature (13-bit format) */
  uint8  rescal;                /* Analog Resistor Calibration value */
  uint8  reservedC;
  uint16 vco_rate_mhz;          /* VCO Rate in MHz */
  uint8  analog_vco_range;      /* Analog VCO Range */
  uint8  pll_div;               /* PLL Divider value */
  uint8  pll_lock;              /* PLL Lock */
  uint8  reservedD;
  int16  die_temp;              /* Live die temperature in Celsius */
  uint8  core_status;
  uint8  reservedE;
} BCM_PACKING_ATTRIBUTE ;

typedef uint8                   SerdesGetCoreStateMsgReq;
typedef struct SerdesCoreState  SerdesGetCoreStateMsgRep;
extern  MsgLayout               SerdesGetCoreStateLayout;

struct SerdesCoreStateMerlin16 {
  uint8  core_reset;  /** Core DP Reset State */
  uint8  pll_pwrdn;  /**  PLL Powerdown enable */
  uint8  uc_active;  /** Micro active enable */
  uint8  reservedA;
  uint16 comclk_mhz;  /** Comclk Frequency in Mhz */
  uint16 ucode_version;  /** uCode Major Version number */
  uint8  ucode_minor_version;  /** uCode Minor Version number */
  uint8  reservedB[3];
  uint32 api_version;  /** API Version number */
  uint8  afe_hardware_version;  /** AFE Hardware version */
  uint8  temp_idx;  /** uC Die Temperature Index */
  int16  avg_tmon;  /** Average Die Temperature (13-bit format) */
  uint8  rescal;  /** Analog Resistor Calibration value */
  uint8 reservedC;
  uint16 vco_rate_mhz;  /** VCO Rate in MHz */
  uint8  analog_vco_range;  /**  Analog VCO Range */
  uint8  reserdedD;
  uint32 pll_div;  /** PLL Divider value.  (Same encoding as enum #merlin16_shasta_pll_div_enum.) */
  uint8  pll_lock;  /** PLL Lock */
  uint8  pll_lock_chg;  /** PLL Lock Change */
  int16 die_temp;  /** Live die temperature in Celsius */
  uint8 core_status;  /** Core Status Variable */
} BCM_PACKING_ATTRIBUTE ;

typedef uint8                           SerdesGetCoreStateMerlin16MsgReq;
typedef struct SerdesCoreStateMerlin16  SerdesGetCoreStateMerlin16MsgRep;
extern  MsgLayout                       SerdesGetCoreStateMerlin16Layout;

struct SerdesCoreStateFalcon16 {
  uint8  core_reset;  /** Core DP Reset State */
  uint8  pll_pwrdn;  /**  PLL Powerdown enable */
  uint8  uc_active;  /** Micro active enable */
  uint8  reservedA;
  uint16 comclk_mhz;  /** Comclk Frequency in Mhz */
  uint16 ucode_version;  /** uCode Major Version number */
  uint8  ucode_minor_version;  /** uCode Minor Version number */
  uint8  reservedB[3];
  uint32 api_version;  /** API Version number */
  uint8  afe_hardware_version;  /** AFE Hardware version */
  uint8  temp_idx;  /** uC Die Temperature Index */
  int16  avg_tmon;  /** Average Die Temperature (13-bit format) */
  uint8  rescal;  /** Analog Resistor Calibration value */
  uint8 reservedC;
  uint16 vco_rate_mhz;  /** VCO Rate in MHz */
  uint8  analog_vco_range;  /**  Analog VCO Range */
  uint8  reserdedD;
  uint32 pll_div;  /** PLL Divider value.  (Same encoding as enum #falcon16_shasta_pll_div_enum.) */
  uint8  pll_lock;  /** PLL Lock */
  uint8  pll_lock_chg;  /** PLL Lock Change */
  int16 die_temp;  /** Live die temperature in Celsius */
  uint8 core_status;  /** Core Status Variable */
  uint8 refclk_doubler;  /** Refclk doubler enable */
  int16 pll_comp_thresh;  /** PLL comparator threshold */
} BCM_PACKING_ATTRIBUTE ;

typedef uint8                           SerdesGetCoreStateFalcon16MsgReq;
typedef struct SerdesCoreStateFalcon16  SerdesGetCoreStateFalcon16MsgRep;
extern  MsgLayout                       SerdesGetCoreStateFalcon16Layout;


/* }}} */
/* {{{ SerdesSetTxFir */
struct SerdesTxFir {
  int phyAddress; 
  uint8 pre;
  uint8 main;
  uint8 post1;
  uint8 post2; 
  uint8 post3;
} BCM_PACKING_ATTRIBUTE ;

typedef struct SerdesTxFir   SerdesSetTxFirMsgReq;
extern  MsgLayout            SerdesSetTxFirLayout;

/* }}} */
/* {{{ SerdesStartBERScan */
struct SerdesSetBERConfig
{
  int32  phyAddress;
  int32  lane;
  uint32 scanMode;
  uint32 timerControl;
  uint32 maxErrorControl;
  uint32 offsetStart;
} BCM_PACKING_ATTRIBUTE ;

typedef struct SerdesSetBERConfig   SerdesStartBERScanMsgReq;
typedef uint32                      SerdesStartBERScanMsgRep;
extern  MsgLayout                   SerdesStartBERScanLayout;
/* }}} */
/* {{{ SerdesGetBERScan */
struct SerdesBERData
{
    int32               status;
    uint32                 cnt;
    uint32            errs[64];
    uint32            time[64];
} BCM_PACKING_ATTRIBUTE ;

typedef SerdesAddressLane     SerdesGetBERScanMsgReq;
typedef struct SerdesBERData  SerdesGetBERScanMsgRep;
extern  MsgLayout             SerdesGetBERScanLayout;
/* }}} */

/* {{{ GetXtcFramerCounters */

typedef struct XtcFramerCounters XtcFramerCounters; 
struct XtcFramerCounters {
    uint32 frameCountRx;    
    uint32 frameErrors;
    uint32 frameCountTx;
    uint32 ethCountRx;
    uint32 ethFcsErrorRx;
    uint32 ethCountTx;
} BCM_PACKING_ATTRIBUTE;

typedef struct RelayOverflowCounters RelayOverflowCounters;
struct RelayOverflowCounters {
    uint32 overflowRx;
    uint32 overflowTx;
}BCM_PACKING_ATTRIBUTE;

typedef struct XtcFrameTiming XtcFrameTiming;
struct XtcFrameTiming
{
  uint32 lastRoundTrip; /*  expressed in nsecs, 0xFFFFFFFF means not valid */
}BCM_PACKING_ATTRIBUTE;


/*although there are potentially more xtc links (e.g. 4 links on 65200C0), the message
  only allows to retrieve the counters for 2 links at a time.
*/
typedef struct LaneSelection LaneSelection;
struct LaneSelection {
  uint8 laneSelect;
} BCM_PACKING_ATTRIBUTE;
struct GetXtcFramerCountersRep {
    XtcFramerCounters counters[NR_OF_XTC_COUNTER_LINKS];
    RelayOverflowCounters overflowCounters[2]; /* 0 for ES, 1 for management */
    uint32 afeTime;
    XtcFrameTiming xtcFrameTiming[NR_OF_XTC_COUNTER_LINKS];  
} BCM_PACKING_ATTRIBUTE;
typedef struct LaneSelection           GetXtcFramerCountersMsgReq;
typedef struct GetXtcFramerCountersRep GetXtcFramerCountersMsgRep;
extern  MsgLayout                      GetXtcFramerCountersLayout;

/* }}} */
/* {{{ SetGpioLedPattern */

typedef struct SetGpioLedPatternReq SetGpioLedPatternReq;
struct SetGpioLedPatternReq
{
  uint8 gpioIdx;                /*  0-7  GPIOA 0-7
                                 *  8-15 GPIOB 0-7
                                 * 16-23 GPIOC 0-7
                                 * 24-31 GPIOD 0-7 */
  uint8 enable;                 /* enable the GPIO in output mode */
  uint16 pattern;                /* each bit is the value of the GPIO during a
                                   125ms period. After 16 periods, the pattern
                                   will repeat itself */
} BCM_PACKING_ATTRIBUTE;

typedef SetGpioLedPatternReq SetGpioLedPatternMsgReq;
extern  MsgLayout SetGpioLedPatternLayout;

/* }}} */
/* {{{ Set and Get IwfConfig */

typedef struct IwfConfig IwfConfig;
struct IwfConfig {
  uint16 rawEthertype;
  uint16 maxAAL5SDUSize;
  uint16 expectedTPID;
  uint16 reserved1;
  uint16 reserved2;
  uint16 reserved3;
  uint32 reserved4;
} BCM_PACKING_ATTRIBUTE;

typedef IwfConfig SetIwfConfigMsgReq;
extern  MsgLayout SetIwfConfigLayout;

typedef IwfConfig GetIwfConfigMsgRep;
extern  MsgLayout GetIwfConfigLayout;

/* }}} */
/* {{{ ReportPtmSyncOnGpio */

typedef struct ReportPtmSyncOnGpioReq ReportPtmSyncOnGpioReq;
struct ReportPtmSyncOnGpioReq {
  uint32 enable;                /* set to 1 to report PTM sync on GPIOA[lineID] */
} BCM_PACKING_ATTRIBUTE ;

typedef ReportPtmSyncOnGpioReq ReportPtmSyncOnGpioMsgReq;
extern  MsgLayout ReportPtmSyncOnGpioLayout;

/* }}} */
/* {{{ SetXtcFramer */

typedef struct SetXtcFramerMsg SetXtcFramerMsg;
struct SetXtcFramerMsg {
  uint8 enable;   /* 0: disable frame transmission */
                  /* 1: enable frame transmission
                   * 2: enable framer-core itfs */
  uint8 disableVA; /*disable/enable Vectoring Active*/
  uint8 forceVA; /*bypass state and force the Vectoring Active being set.*/
  uint8 reserved;
} BCM_PACKING_ATTRIBUTE;

typedef SetXtcFramerMsg SetXtcFramerMsgReq;
extern MsgLayout SetXtcFramerLayout;

typedef SetXtcFramerMsg GetXtcFramerMsgRep;
extern MsgLayout GetXtcFramerLayout;
/* }}} */
/* {{{ ForceFramerBlock */

typedef struct ForceFramerBlockMsg ForceFramerBlockMsg;
struct ForceFramerBlockMsg {
  uint8 lane;
  uint8 enable;
} BCM_PACKING_ATTRIBUTE;

typedef ForceFramerBlockMsg ForceFramerBlockMsgReq;
extern MsgLayout ForceFramerBlockLayout;
/* }}} */
/* {{{ XtcMonitorRoundtrip */

typedef struct XtcMonitorRoundtripRequest  XtcMonitorRoundtripRequest;
struct XtcMonitorRoundtripRequest
{
  uint32 monitoringDuration; /* in usec - maximum duration is 200 msecs */
} BCM_PACKING_ATTRIBUTE;

typedef struct XtcMonitorRoundtripRes  XtcMonitorRoundtripRes;
struct XtcMonitorRoundtripRes
{
  uint32 minRoundtrip;  /* in nsec */
  uint32 maxRoundtrip;  /* in nsec */
  uint32 avgRoundtrip;  /* in nsec */
  uint32 numMeas; /* number of RTT measurement seen */
} BCM_PACKING_ATTRIBUTE;


typedef struct XtcMonitorRoundtripReply  XtcMonitorRoundtripReply;
struct XtcMonitorRoundtripReply
{
  XtcMonitorRoundtripRes res[16]; /* one entry per external xtc link */
} BCM_PACKING_ATTRIBUTE;

typedef XtcMonitorRoundtripRequest XtcMonitorRoundtripMsgReq;
typedef XtcMonitorRoundtripReply XtcMonitorRoundtripMsgRep;
extern MsgLayout XtcMonitorRoundtripLayout;

/* }}} */
/* {{{ GetXtcFramerStatus */

typedef struct XtcFramerStatus XtcFramerStatus;
struct XtcFramerStatus {
  uint8 aliveLinkStatus[2];
  uint8 aliveChanged[2];
  uint8 vaLinkStatus[2];
  uint8 vaChanged[2];
} BCM_PACKING_ATTRIBUTE;
typedef XtcFramerStatus GetXtcFramerStatusMsgRep;
extern MsgLayout GetXtcFramerStatusLayout;

/* }}} */
/* {{{ CreateExtraBondingGroup */

typedef uint8    CreateExtraBondingGroupMsgReq;
extern MsgLayout CreateExtraBondingGroupLayout;

/* }}} */
/* {{{ BondGetExtraGroups */
typedef uint64               BondGetExtraGroupsMsgRep;
extern  MsgLayout            BondGetExtraGroupsLayout;

/* }}} */
/* {{{ AvsMonitorData  */
typedef struct AvsMonitorData AvsMonitorData;
struct AvsMonitorData {
  int32 temperature; /* Temp in 1/256C */
  int32 voltage1V0;   
  int32 voltage1V8;
  int32 voltage3V3;
} BCM_PACKING_ATTRIBUTE;

typedef AvsMonitorData GetAvsMonitoringDataMsgRep; 
extern MsgLayout GetAvsMonitoringDataLayout; 
/* }}} */
/* {{{ BackgroundLogEntry */

typedef struct BackgroundLogEntry BackgroundLogEntry;
struct BackgroundLogEntry
{
  uint32 reservedA;
  uint16 reservedB;
  uint16 reservedC;
};

/* }}} */
/* {{{ BackgroupLogger */

#define BACKGROUND_LOGGER_ENTRY_NUMBER 20
typedef struct BackgroundLogger BackgroundLogger;
struct BackgroundLogger
{
  BackgroundLogEntry entries[BACKGROUND_LOGGER_ENTRY_NUMBER];
};

/* }}} */
/* {{{ GetCpuMonitoring */

typedef struct CpuMonitor CpuMonitor;
struct CpuMonitor
{
  uint16 reservedA;
  uint16 reservedB;
  uint16 reservedC;
  uint16 reservedD;
  uint16 reservedE;
  uint16 reservedF;
};

#define MAX_CPU_MONITOR 5
typedef struct CpuLogger CpuLogger;
struct CpuLogger
{
  CpuMonitor monitor[MAX_CPU_MONITOR];
};

typedef struct CpuMonitoringRep CpuMonitoringRep;
struct CpuMonitoringRep {
  uint8 reservedA;
  uint8 reservedB;
  CpuLogger reservedC;
  uint16 reservedD;
  uint16 reservedE;
  uint16 reservedF;
  BackgroundLogger reservedG;
} BCM_PACKING_ATTRIBUTE;

typedef int32               GetCpuMonitoringMsgReq;
typedef CpuMonitoringRep    GetCpuMonitoringMsgRep;
extern  MsgLayout           GetCpuMonitoringLayout;

typedef struct DpllI2cCmdReq DpllI2cCmdMsgReq;
struct DpllI2cCmdReq{
  uint8 readNotWrite; /* set to 1 to read a value */
  uint8 phyAdd;       /* phy address */
  uint8 firstReg;     /* First register (1 byte per register) to read or first register to write */
  uint8 nbReg;        /* If read:
                           - Nb of register to read  (one by one).
                           - Nb of register to write (in one command by increasing register index)
                         Maximum value = 8
                      */
  uint32 reserved;
  uint8  bytes[8];
} BCM_PACKING_ATTRIBUTE;

typedef struct DpllI2cCmdRep DpllI2cCmdMsgRep;
struct DpllI2cCmdRep{
  uint8 bytes[8];
} BCM_PACKING_ATTRIBUTE;

extern MsgLayout          DpllI2cCmdLayout;


/* }}} */
/* {{{ ResetAfeTdIfce */

extern MsgLayout  ResetAfeTdIfceLayout;

/* }}} */
/* {{{ InstallTrafficRelay */
# ifdef BCM_TRAFFIC_RELAY
#  ifdef BCM_GFAST_SUPPORT
#   ifdef BCM65200_SUPPORT
#    define BCM_MAX_NB_RELAY_LINES_PER_CORE 3
#    define BCM_MAX_NB_HMI_RELAY 1
#    define BCM_MAX_NB_ES_RELAY 6
#   endif
#   ifdef BCM65400_SUPPORT
#    define BCM_MAX_NB_RELAY_LINES_PER_CORE 1
#    define BCM_MAX_NB_HMI_RELAY 1
#    define BCM_MAX_NB_ES_RELAY 0
#   endif
#  else
#    define BCM_MAX_NB_RELAY_LINES_PER_CORE 4
#    define BCM_MAX_NB_HMI_RELAY 0
#    define BCM_MAX_NB_ES_RELAY 0
#  endif
#  define BCM_MAX_NB_RELAY_SIDS BCM_MAX_NB_RELAY_LINES_PER_CORE+BCM_MAX_NB_HMI_RELAY+BCM_MAX_NB_ES_RELAY
typedef struct InstallTrafficRelayMsg InstallTrafficRelayMsg;
struct InstallTrafficRelayMsg{
  uint8  dspLink;
  uint8  npuLink;
  uint8  remoteDspMac[6];
  uint16 sidMap[BCM_MAX_NB_RELAY_SIDS];
} BCM_PACKING_ATTRIBUTE;

typedef struct InstallTrafficRelayMsg InstallTrafficRelayMsgReq;
extern MsgLayout InstallTrafficRelayLayout;
# endif

typedef uint8 InstallCpeEsRelayMsgReq;
extern MsgLayout InstallCpeEsRelayLayout;

/* }}} */
/* {{{ SetRpfControl */

typedef struct RpfControl RpfControl;
struct RpfControl
{
  uint32 maskSetBit;    /* setting a bit to one will set the corresponding bit
                         * to 1 in the RpfPowerMask */
  uint32 maskResetBit;  /* setting a bit to one will set the corresponding bit
                         * to 0 in the RpfPowerMask */
  uint32 pinStatus;     /* only in replies */
  uint32 reserved[4];
} BCM_PACKING_ATTRIBUTE;

typedef RpfControl SetRpfControlMsgReq;
typedef RpfControl SetRpfControlMsgRep;
extern  MsgLayout SetRpfControlLayout;

#ifndef SUPPORT_HMI
static inline uint32 setRpfLineMask(uint16 lineId, FirmwareVersionInfo *fwVersion)
{
  uint16 shift =
    ((BCM_CHIP_IS_654xx(fwVersion->hwChipId) && fwVersion->buildType == BCM_BUILD_TYPE_GFAST212) ||
     (BCM_CHIP_IS_6545x(fwVersion->hwChipId) && fwVersion->buildType == BCM_BUILD_TYPE_GFAST424)) ? 1 : 0;
  
  return 1U << (BCM_DEV_LINE_ID(lineId) << shift);
}
static inline uint32 setRpfSlmMask(FirmwareVersionInfo *fwVersion)
{
  uint16 slmIdx = BCM_CHIP_IS_654xx(fwVersion->hwChipId)? 8 : 16;
  return 1U << slmIdx;               
}
#endif
/* }}} */
/* {{{ RpfSwitchBackToXtc */

extern MsgLayout RpfSwitchBackToXtcLayout;

/* }}} */
/* {{{ FastbackSetCo/CpeInfo */

typedef struct FastbackCoInfo FastbackCoInfo;
struct FastbackCoInfo {
  uint32 availInfo; // set to 1 if information is avaialble
  int32  coPregen;
  int32  coLateRx;
  int32  numTonesPart1; // number of tones prior to the bandplan split
  uint32 reserved[6];
};


typedef struct FastbackCpeInfo FastbackCpeInfo;
struct FastbackCpeInfo {
  uint32 availInfo; // set to 1 if information is avaialble
  int32  cpeDelayInfo;  // C141
  int32  cpeTxToRxDelay; // C210
  uint32 reserved[6];
};

/* msg that can be sent to the Fastback CPE */
typedef FastbackCoInfo FastbackSetCoInfoMsgReq;
typedef FastbackCpeInfo FastbackSetCoInfoMsgRep;
extern  MsgLayout FastbackSetCoInfoLayout;

/* msg that can be sent to the Fastback CPE */
typedef FastbackCpeInfo FastbackSetCpeInfoMsgReq;
typedef FastbackCoInfo FastbackSetCpeInfoMsgRep;
extern  MsgLayout FastbackSetCpeInfoLayout;

/* }}} */

/* {{{ Prototypes */

int coreVerifyXtcRingResult(XtcRingResult *result, uint8 *crc_diff);
int coreVerifySeveralXtcRingResult(XtcRingResult *newResult,XtcRingResult *oldResult,uint8 *crc_diff);

Bool serdesPhyCheck(uint16 devId, uint32 phyAddress);

/* }}} */

#endif
