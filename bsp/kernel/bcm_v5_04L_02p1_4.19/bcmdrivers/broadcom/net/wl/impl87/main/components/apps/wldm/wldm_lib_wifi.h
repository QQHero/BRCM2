/*
 * Broadcom Wifi Data Model Library.
 *
 * Copyright 2022 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifndef __WLDM_LIB_WIFI_H__
#define __WLDM_LIB_WIFI_H__

#include "bcmutils.h"

/* TR069 supported commands */
#define CMD_LIST		(1 << 0)
#define CMD_GET			(1 << 1)
#define CMD_SET			(1 << 2)
#define CMD_ADD			(1 << 3)
#define CMD_DEL			(1 << 4)

/* Auxiliary commands */
#define CMD_GET_NVRAM		(1 << 16)
#define CMD_SET_NVRAM		(1 << 17)
#define CMD_SET_IOCTL		(1 << 18)

#define WIFI_SUPPORTEDSTANDARDS_NONAX_2G	"b,g,n"
#define WIFI_SUPPORTEDSTANDARDS_AX_2G		"g,n,ax"
#define WIFI_SUPPORTEDSTANDARDS_NONAX_5G	"a,n,ac"
#define WIFI_SUPPORTEDSTANDARDS_AX_5G		"a,n,ac,ax"
#define WIFI_SUPPORTEDSTANDARDS_NONAX_6G	""
#define WIFI_SUPPORTEDSTANDARDS_AX_6G		"ax"
#define MAX_STDSTR_LEN				32

/* ACSD commands */
#define ACSD			"acsd2"
#define ACS_CLI			"acs_cli2"

/* opmode_cap_t from wlc_ap.c
* That is: OMC_HE > OMC_VHT > OMC_HT > OMC_ERP > OMC_NONE.
*/
typedef enum _opmode_cap_t {

	OMC_NONE = 0,		/**< no requirements for STA to associate to BSS */

	OMC_ERP = 1,		/**< STA must advertise ERP (11g) capabilities
				 * to be allowed to associate to 2G band BSS.
				 */
	OMC_HT = 2,		/**< STA must advertise HT (11n) capabilities to
				 * be allowed to associate to the BSS.
				 */
	OMC_VHT = 3,		/**< Devices must advertise VHT (11ac) capabilities
				 * to be allowed to associate to the BSS.
				 */
	OMC_HE = 4,		/**< Devices must advertise HE (11ax) capabilities
				 * to be allowed to associate to the BSS.
				 */
	OMC_MAX
} opmode_cap_t;

/* wlc_rate.h */
#define WLC_RATE_FLAG                   0x80    /**< basic rate flag */
#define RATE_MASK                       0x7f    /* Rate value mask w/o basic rate flag */
#define RATE_MASK_FULL                  0xff    /* Rate value mask with basic rate flag */
#define WLC_STD_MAX_VHT_MCS             9	/**< 11ac std VHT MCS 0-9 */
#define MAX_HT_RATES                    8       /* max no. of ht rates supported (0-7) */
#define MAX_VHT_RATES                   12      /* max no. of vht rates supported (0-9 and prop 10-11) */
#define MAX_HE_MCS			11	/* max mcs index of HE rates supported (mcs 0-11) */
#define WLC_MAXMCS			32	/**< max valid mcs index */

/* wlc_pub.h */
#define WL_HE_FEATURES_5G		0x0001
#define WL_HE_FEATURES_2G		0x0002
#define WL_HE_FEATURES_DLOMU            0x0004	/**< HE DL-OFDMA MU */
#define WL_HE_FEATURES_ULOMU            0x0008
#define WL_HE_FEATURES_DLMMU            0x0010	/**< HE DL-MIMO MU */
#define WL_HE_FEATURES_ULMMU            0x0020
#define WL_HE_FEATURES_ULMU_STA         0x0040	/**< HE STA UL-MU */
#define WL_HE_FEATURES_ERSU_RX          0x0080
#define WL_HE_FEATURES_ERSU_TX          0x0100
#define HE_FEATURES_DEFAULT             ((WL_HE_FEATURES_ULMMU - 1))

/* Following struct is from main/src/wl/exe/wluc_he.c */
typedef struct {
	uint16 id;
	uint16 len;
	uint32 val;
} he_xtlv_v32;

typedef enum _he_mu_type_t {
	/* Downlink MU types */
	HE_MU_DL_NONE  = 0,			/* HE SU DL */
	HE_MU_DL_OFDMA,				/* HE DL OFDMA is enabled/disabled */
	HE_MU_DL_HEMUMIMO,			/* HE DL MU-MIMO is enabled/disabled */
	HE_MU_DL_OFDMA_HEMUMIMO,		/* HE DL OFDMA and MU-MIMO both enabled/disabled */
	/* Uplink MU types */
	HE_MU_UL_NONE,				/* HE SU UL */
	HE_MU_UL_OFDMA,				/* HE UL OFDMA enabled/disabled */
} he_mu_type_t;

typedef int32 he_features_t;
typedef uint32 vht_mu_features_t;

typedef enum _link_direction_t {
	HE_MU_DOWNLINK,
	HE_MU_UPLINK,
} link_direction_t;

#define MAX_IOCTL_BUFLEN			2048

#define BW_20MHZ				(WL_RSPEC_BW_20MHZ >> WL_RSPEC_BW_SHIFT)
#define BW_40MHZ				(WL_RSPEC_BW_40MHZ >> WL_RSPEC_BW_SHIFT)
#define BW_80MHZ				(WL_RSPEC_BW_80MHZ >> WL_RSPEC_BW_SHIFT)
#define BW_160MHZ				(WL_RSPEC_BW_160MHZ >> WL_RSPEC_BW_SHIFT)

#define WL_STA_ANT_MAX				4       /* max possible rx antennas */

typedef struct wldm_enum_to_str_map {
	int		enum_val;
	const char	*str_val;
} wldm_enum_to_str_map_t;

/* Following HE_CC defines are derived from wlc_he,c */
#define HE_CC_AP_DETECT_ENAB            (1 << 0)
#define HE_CC_AP_REPORT_HANDLER_ENAB    (1 << 1)
#define HE_CC_AUTO_ENAB                 (1 << 2)
#define HE_CC_EVENT_ENAB                (1 << 3)
#define HE_CC_STA_DETECT_ENAB           (1 << 4)

/* match wifi_twt_agreement_type_t in hal */
typedef enum {
	wldm_twt_agreement_type_individual,	/**< Set an individual TWT session. */
	wldm_twt_agreement_type_broadcast,	/**< Set a Broadcast TWT session. */
} wldm_twt_agreement_type_t;

/* match wifi_twt_operation_t in hal */
typedef struct {
	boolean	implicit;		/**< True is implicit, or false to be explicit*/
	boolean	announced;		/**< True is announced, or false to be unannounced */
	boolean	trigger_enabled;	/**< Enable the TWT trigger */
	unsigned int flowID;		/**< Agreement identifier */
} wldm_twt_operation_t;

/* match wifi_twt_individual_params_t in hal */
typedef struct {
	unsigned int	wakeTime_uSec;		/**< Wake time of TWT session in microseconds */
	unsigned int	wakeInterval_uSec;	/**< TWT wake interval in microseconds*/
	unsigned int	minWakeDuration_uSec;	/**< Minimum TWT wake interval in microseconds*/
	unsigned int	channel;		/**< Channel of the TWT session*/
} wldm_twt_individual_params_t;

/* match wifi_twt_broadcast_params_t in hal */
typedef struct {
	unsigned int	target_beacon_uSec;	/**< next xmit time of a Beacon in microseconds */
	unsigned int	listen_interval_uSec;	/**< Interval between subsequent beacons carrying TWT information in microseconds*/
} wldm_twt_broadcast_params_t;

/* match wifi_twt_params_t in hal */
typedef struct {
	wldm_twt_agreement_type_t	agreement;	/**< Agreement of the TWT session i.e. Individual or broadcast  */
	wldm_twt_operation_t		operation;	/**< Set the operation of the TWT session */
	union {
		wldm_twt_individual_params_t	individual; /**< Set configuration for Individual TWT session */
		wldm_twt_broadcast_params_t	broadcast;  /**< Set configuration for Broadcast TWT session */
	} params;
	boolean sessionPaused;	/**< TRUE if the session is in pause, but it hasn't been teardown  */
} wldm_twt_params_t;

/* match wifi_twt_dev_info_t in hal */
#define WLDM_MAX_NUM_TWT_SESSION 50	/* MAX_NUM_TWT_SESSION in hal */
typedef struct {
	unsigned int		numTwtSession;		/**< Number of TWT session for that device */
	wldm_twt_params_t	twtParams[WLDM_MAX_NUM_TWT_SESSION];	/**< List of TWT session that device has joined */
} wldm_twt_dev_info_t;

/*
*  The wldm functions arguments.
*  cmd: the command to do, please refer to CPE Methods in TR-069.
*     list: GetParameterNames
*     get: GetParameterValues
*     set: SetParameterValues
*     add: AddObject
*     del: DeleteObject
*     SetParameterAttributes and GetParameterAttributes are currently not supported.
*  index: the array index, which is TR181 index {i} - 1.
*  *pvalue: input value for set, and output value for get command of the TR181 defined data type.
*  *plen: the size of the content in *pvalue. It is input for set, and output for get command.
*  *pbuf: the human readable text output buffer.
*  *pbufsz: the pbuf size available(input), and pbuf size remained(output).
*/

/* Device.WiFi. */
extern int wldm_RadioNumberOfEntries(int cmd, int index,
	unsignedInt *pvalue, int *plen,	char *pbuf, int *pbufsz);
extern int wldm_SSIDNumberOfEntries(int cmd, int index,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPointNumberOfEntries(int cmd, int index,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_EndPointNumberOfEntries(int cmd, int index,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.Radio.{i}. */
extern int wldm_Radio_Enable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_Alias(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_FragmentationThreshold(int cmd, int radioIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_RTSThreshold(int cmd, int radioIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_AutoChannelEnable(int cmd, int radioIndex,
        boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_AutoChannelSupported(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_ExtensionChannel(int cmd, int radioIndex,
        string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_OperatingChannelBandwidth(int cmd, int radioIndex,
        string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_Channel(int cmd, int radioIndex,
	unsignedInt *pvalue, int *plen, int bw, int extChan, char *pbuf, int *pbufsz);
extern int wldm_Radio_GuardInterval(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_BasicDataTransmitRates(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_SupportedDataTransmitRates(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_OperationalDataTransmitRates(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_MaxBitRate(int cmd, int radioIndex, int *pvalue, int *plen,
	char *pbuf, int *pbufsz);

extern int wldm_Radio_ChannelsInUse(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_RegulatoryDomain(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_OperatingFrequencyBand(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
/* Returns number of valid channels or -1 if IOCTL failure or *plen < possible channel string len */
extern int wldm_Radio_PossibleChannels(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_Status(int cmd, int radioIndex, boolean *pvalue,
	int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_Cts_Protection_Enable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_Carrier_Sense_Threshold(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsze);
extern int wldm_Radio_TxChainMask(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_RxChainMask(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_BeaconPeriod(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_TransmitPowerSupported(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_DTIMPeriod(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_AMSDUEnable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_ObssCoexistenceEnable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_IEEE80211hEnabled(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_AutoChannelRefreshPeriod(int cmd, int radioIndex,
	unsigned int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_AutoChannelDwellTime(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_DfsSupport(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_DfsEnable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int  wldm_Radio_MCS(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_TransmitPower(int cmd, int radioIndex,
        int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_TrafficStats2(int cmd, int radioIndex,
	Device_WiFi_Radio_TrafficStats2 *radio_Traffic_Stats, int *plen, char *pbuf, int *pbufsz);

/* Proprietary Radio APIs */
extern int wldm_xbrcm_Radio_CtsProtectionEnable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_xbrcm_Radio_STBCEnable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_xbrcm_Radio_TxChainMask(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_xbrcm_Radio_RxChainMask(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_xbrcm_Radio_Greenfield11nSupported(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_xbrcm_Radio_Greenfield11nEnable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_xbrcm_Radio_ResetCount(int cmd, int radioIndex,
        int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_xbrcm_acs(int cmd, int radioIndex,
        void *pvalue, int *plen, char *pbuf, int *pbufsz);

/* Proprietary Rate Control APIs */
extern int wldm_RatesBitmapControl_Enable(int cmd,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_RatesBitmapControl_BasicRate(int cmd, int ssidIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_RatesBitmapControl_SupportedRate(int cmd, int ssidIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.SSID.{i}. */
extern int wldm_SSID_Enable(int cmd, int ssidIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_SSID_SSID(int cmd, int ssidIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_SSID_MACAddress(int cmd, int ssidIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_SSID_Status(int cmd, int ssidIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int  wldm_SSID_TrafficStats(int cmd, int ssidIndex,
	Device_WiFi_SSID_Stats  *SSID_Traffic_Stats, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.AccessPoint.{i}. */
extern int wldm_AccessPoint(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_SSIDAdvertisementEnabled(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_IsolationEnable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *printbuf, int *pbufsz);
extern int wldm_AccessPoint_MaxAssociatedDevices(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Status(int cmd, int apIndex, char *pvalue,
	int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_AssociatedDeviceNumber(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Enable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_RetryLimit(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_UAPSDCapability(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_UAPSDEnable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_WMMCapability(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_WMMEnable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_AclDeviceNumber(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_MACAddressControMode(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_AclDevices(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_AclDevice(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_kickAssociatedDevice(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
/*
extern int wldm_AccessPoint_AssociatedDevice(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
*/
extern int wldm_AccessPoint_DelAclDevices(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.AccessPoint.{i}.Security. */
extern int wldm_AccessPoint_Security_KeyPassphrase(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Basic_Authenticationmode(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_Modessupported(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_ModeEnabled(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Wpa_Encryptionmode(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_PreSharedKey(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_AuthMode(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_MFPConfig(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusServerIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusServerPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusSecret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_SecondaryRadiusServerIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_SecondaryRadiusServerPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_SecondaryRadiusSecret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusReAuthInterval(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusOperatorName(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusLocationData(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusGreylist(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusDASPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusDASClientIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_RadiusDASSecret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_WPAPairwiseRetries(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_EncryptionModesSupported(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_WPAPMKLifetime(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Security_WPA3TransitionDisable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.AccessPoint.{i}.Security.X_COMCAST_COM_RadiusSettings. */
extern int wldm_AccessPoint_Security_X_COMCAST_COM_RadiusSettings(int cmd, int apIndex,
	void *pvalue, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.AccessPoint.{i}.WPS. */
extern int wldm_AccessPoint_WPS_ConfigMethodsSupported(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_WPS_ConfigMethodsEnabled(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_WPS_PIN(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_WPS_Enable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_WPS_ConfigurationState(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.AccessPoint.{i}.Accounting. */
extern int wldm_AccessPoint_Accounting_Enable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Accounting_ServerIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Accounting_SecondaryServerIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Accounting_ServerPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Accounting_SecondaryServerPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Accounting_Secret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Accounting_SecondarySecret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_AccessPoint_Accounting_InterimInterval(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.AccessPoint.{i}.AssociatedDevice. */
extern int wldm_AccessPoint_Device_SignalStrength(int cmd, int apIndex,
	int *pvalue, int *plen, char *MAC, char *pbuf, int *pbufsz);

/* Proprietary: AX features */
/* HE */
extern int wldm_AXenable(int cmd, unsigned int radioIndex,
	boolean *result, int *plen, char *pbuf, int *pbufsz);

extern int wldm_AXfeatures(int cmd, unsigned int radioIndex,
	unsigned int *result, int *plen, char *pbuf, int *pbufsz);

extern int wldm_AXbssColor(int cmd, unsigned int radioIndex,
	unsigned int *color, int *plen, char *pbuf, int *pbufsz);

extern int wldm_AXavailableBssColors(int cmd, unsigned int radioIndex,
        unsigned char *color_list, int *plen, char *pbuf, int *pbufsz);

extern int wldm_xbrcm_Radio_AXmuType(int cmd, int radioIndex,
	he_mu_type_t *mutype, uint *plen, char *pbuf, int *pbufsz);

extern int wldm_xbrcm_Radio_AXmuEdca(int cmd, int radioIndex,
	wldm_wifi_edca_t *wldm_edca, uint *plen, char *pbuf, int *pbufsz);

extern int wldm_11ax_twt(int cmd, unsigned int radioIndex, void *pvalue, int *plen,
	char *pbuf, int *pbufsz);

extern int wldm_Radio_OperatingStandards(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz);
extern int wldm_Radio_SupportedStandards(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz);

/* Device.WiFi.AccessPoint.{i}.AssociatedDevice */
typedef enum wldm_diagnostic_result {
	DIAG_RESULT_TR181 = 0,
	DIAG_RESULT_1,
	DIAG_RESULT_2,
	DIAG_RESULT_3,
} wldm_diagnostic_result_t;

#define ASSOC_DEV_INFO	\
	unsigned char				cli_MACAddress[ETHER_ADDR_LEN]; \
	bool					cli_AuthenticationState; \
	unsigned int				cli_LastDataDownlinkRate; \
	unsigned int				cli_LastDataUplinkRate; \
	int					cli_SignalStrength; \
	unsigned int				cli_Retransmissions; \
	bool					cli_Active; \
	char					cli_OperatingStandard[STRING_LENGTH_64]; \
	char					cli_OperatingChannelBandwidth[STRING_LENGTH_64]; \
	int					cli_SNR; \
	unsigned long				cli_BytesSent; \
	unsigned long				cli_BytesReceived; \
	int					cli_RSSI; \
	unsigned long				cli_DataFramesSentAck; \
	unsigned long				cli_DataFramesSentNoAck; \

#define ASSOC_DEV2_INFO	\
	ASSOC_DEV_INFO; \
	unsigned long long			cli_Associations;

typedef struct _wldm_wifi_associated_dev1 {
	ASSOC_DEV_INFO;
} wldm_wifi_associated_dev1_t;

typedef struct _wldm_wifi_associated_dev2 {
	ASSOC_DEV2_INFO;
} wldm_wifi_associated_dev2_t;

typedef struct _wldm_wifi_associated_dev3 {
	ASSOC_DEV2_INFO;
	unsigned long				cli_PacketsSent;
	unsigned long				cli_PacketsReceived;
	unsigned long				cli_ErrorsSent;
	unsigned long				cli_RetransCount;
	unsigned long				cli_FailedRetransCount;
	unsigned long				cli_RetryCount;
	/* XXX TBD - check correct flag and update when ready
	 * wldm_dl_mu_stats_t			cli_DownlinkMuStats;
	 * wldm_ul_mu_stats_t			cli_UplinkMuStats;
	 */
	wldm_twt_dev_info_t			cli_TwtParams;
} wldm_wifi_associated_dev3_t;

extern int wldm_AccessPoint_AssociatedDevice(int cmd, int apIndex, wldm_diagnostic_result_t type,
	void *associated_dev_array, unsigned int *plen, char *pbuf, int *pbufsz);

typedef struct _wldm_wifi_rssi_snapshot {
	int8					rssi[WL_STA_ANT_MAX];
} wldm_wifi_rssi_snapshot_t;

typedef struct _wldm_wifi_associated_dev_stats {
	StatsCounter64				cli_rx_bytes;
	StatsCounter64				cli_tx_bytes;
	StatsCounter64				cli_rx_frames;
	StatsCounter64				cli_tx_frames;
	StatsCounter64				cli_rx_retries;
	StatsCounter64				cli_tx_retries;
	StatsCounter64				cli_rx_errors;
	StatsCounter64				cli_tx_errors;
	StatsCounter32				cli_rx_rate;
	StatsCounter32				cli_tx_rate;
	wldm_wifi_rssi_snapshot_t		cli_rssi_bcn;
} wldm_wifi_associated_dev_stats_t;

typedef struct _wldm_wifi_associatedDevRateInfoStats {
	unsigned char				nss;
	unsigned char				mcs;
	unsigned short int			bw;
	StatsCounter64				flags;
	StatsCounter64				msdus;
	StatsCounter64				mpdus;
	unsigned char				rx_rssi_combined;
} wldm_wifi_associatedDevRateInfoStats_t;

extern int wldm_AccessPoint_AssocDevice(int cmd, int apIndex,
        char *pvalue, int *plen, char *pbuf, int *pbufsz);

/* Radio upTime */
#define CMD_UPTIME_GET  0
#define CMD_UPTIME_INIT 1
extern int wl_UpTime(int radioIndex, int cmd, unsigned int *upTime);
extern int wldm_Radio_LastChange(int cmd, unsigned int radioIndex,
	unsigned int *upSecs, int *plen, char *pbuf, int *pbufsz);

/* for callback algorithm */
typedef enum wldm_callback_id {
	FD_STA_CONN = 0,
	FD_ASSOC_DEV, /* For WM wifi_newApAssociatedDevice_callback */
	FD_AUTH_FAIL,
	FD_MESH,
	FD_RRM_BCNREP,
	FD_BSSTRANS,
	FD_DPP,
	FD_CH_CHG,
	NUM_FD
} wldm_callback_id_t;

typedef enum wldm_cb_action {
	WLDM_CB_UNREGISTER = 0,
	WLDM_CB_REGISTER
} wldm_cb_action_t;

/* domain path: all callback will use domain socket */
#define WIFI_HAL_CB_STA_CONN_DSOCKET      "/tmp/sta_connect_cb"
#define WIFI_HAL_CB_ASSOC_DEV_DSOCKET     "/tmp/assoc_dev_cb"

#define WIFI_HAL_CB_AUTH_FAIL_DSOCKET     "/tmp/auth_fail_cb"
#define WIFI_HAL_CB_MESH_STEER_DSOCKET    "/tmp/mesh_steer_cb"

#define WIFI_HAL_CB_RRM_BCNREP_DSOCKET    "/tmp/bcn_report_cb"
#define WIFI_HAL_CB_BSSTRANS_DSOCKET      "/tmp/bss_transit_cb"
#define WIFI_HAL_CB_DPP_DSOCKET           "/tmp/dpp_cb"
#define WIFI_HAL_CB_CH_CHG_DSOCKET        "/tmp/ch_chg_cb"

typedef struct wldm_callback_thread {
	int thread_initialized;
	int count;
	pthread_t cbThreadId;
} wldm_callback_thread_t;

extern int wldm_callback(wldm_cb_action_t action, int cb_id,
	int (* cb_handler)(int), wldm_callback_thread_t *);

/* proprietary support for 11v BTM (BSS Transition Management) */
typedef enum wldm_btm_action {
	WLDM_BTM_SEND_REQUEST = 0,
	WLDM_BTM_GET_ACTIVATION,
	WLDM_BTM_SET_ACTIVATION,
	WLDM_BTM_GET_CLIENT_CAP,
	WLDM_BTM_GET_IMPLEMENTED
} wldm_btm_action_t;

extern int wldm_11v_btm(wldm_btm_action_t action, unsigned int apIndex, unsigned char peer_mac[6],
	bool *pvalue, char *in_request, unsigned int option_len, char *ptr_option);

extern int wl_sendActionFrame(int apIndex, unsigned char peer[6], unsigned char bssid[6],
	uint frequency, char *frame, uint frame_len);

/* WFA WPS */
typedef enum wldm_wfa_wps_cmd {
	WFA_WPS_ACTIVATE_PUSH_BUTTON = 0,
	WFA_WPS_SET_CLIENT_PIN,
	WFA_WPS_CANCEL,
	WFA_WPS_GET_STATUS
} wldm_wfa_wps_cmd_t;

typedef struct wldm_wfa_wps_param {
	int apIndex;
	wldm_wfa_wps_cmd_t cmd;
	union {
		string pin;
		char status[32]; /* Success | Failed | In_progress */
	} param;
} wldm_wfa_wps_param_t;
int wldm_wfa_wps(wldm_wfa_wps_param_t *param);

/* HSPOT 11u Interworking */
extern int wldm_11u_iw(int cmd, int index,
        void *pvalue, uint *plen, char *pvar, int *pvarsz);
extern void wldm_hspot_restart_if_needed();
/* HSPOT */
extern int wldm_hspot(int cmd, int index,
        void *pvalue, uint *plen, char *pvar, int *pvarsz);

/* BSD steering policy default value */
#define BSD_DEFAULT_BW_UTIL_2G                  0
#define BSD_DEFAULT_BW_UTIL_5G                  80
#define BSD_DEFAULT_SAMPLE_PERIOD               5
#define BSD_DEFAULT_CONSECUTIVE_SAMPLE_COUNT    3
#define BSD_DEFAULT_RSSI_THRESHOLD              0
#define BSD_DEFAULT_PHYRATE_THRESHOLD           0
#define BSD_DEFAULT_EXTENSION_FLAG_2G           0x12
#define BSD_DEFAULT_EXTENSION_FLAG_5G           0x0

typedef struct bsd_steering_policy {
	int bwUtil;
	int samplePeriod;
	int consecutiveSampleCount;
	int rssiThreshold;
	int phyRateThreshold;
	int extFlag;
} bsd_steering_policy_t;

typedef enum wldm_xbrcm_bsd_param_id {
	WLDM_BSD_STEER_CAP = 0,
	WLDM_BSD_STEER_ENABLE,
	WLDM_BSD_STEER_APGROUP,
	WLDM_BSD_STEER_BANDUTIL,
	WLDM_BSD_STEER_RSSI,
	WLDM_BSD_STEER_PHYRATE
} wldm_xbrcm_bsd_param_id_t;

#define SSD_SOFTBLOCK_LIST_FILE     "/tmp/ssd_softblock_list.log"

extern int wldm_xbrcm_bsd(int cmd, int radioIndex, wldm_xbrcm_bsd_param_id_t param_id,
	void *param, int *paramlen);
extern int wldm_xbrcm_ssd(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);
extern int wldm_xbrcm_phy(int cmd, int radioIndex, void *pvalue, uint *plen, char *pvar,
	int *pvarsz);
extern int wldm_xbrcm_ap(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);

typedef struct _wldm_channel_stats {
	int			ch_number;
	boolean			ch_in_pool;
	int			ch_noise;
	boolean			ch_radar_noise;
	int			ch_max_80211_rssi;
	int			ch_non_80211_noise;
	int			ch_utilization;
	unsigned long long	ch_utilization_total;
	unsigned long long	ch_utilization_busy;
	unsigned long long	ch_utilization_busy_tx;
	unsigned long long	ch_utilization_busy_rx;
	unsigned long long	ch_utilization_busy_self;
	unsigned long long	ch_utilization_busy_ext;
} wldm_channel_stats_t;

typedef struct _wldm_channel_stats2 {
	unsigned int		ch_Frequency;
	int			ch_NoiseFloor;
	int			ch_Non80211Noise;
	int			ch_Max80211Rssi;
	unsigned int		ch_ObssUtil;
	unsigned int		ch_SelfBssUtil;
} wldm_channel_stats2_t;

extern int wldm_xbrcm_lq(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);
extern int wldm_xbrcm_sta(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);
extern int wldm_xbrcm_counter(int cmd, int index, void *pvalue, uint *plen, char *pvar,
	int *pvarsz);
typedef struct _wldm_neighbor_ap2 {
	char					ap_SSID[STRING_LENGTH_64];
	char					ap_BSSID[STRING_LENGTH_64];
	char					ap_Mode[STRING_LENGTH_64];
	uint					ap_Channel;
	int					ap_SignalStrength;
	char					ap_SecurityModeEnabled[STRING_LENGTH_64];
	char					ap_EncryptionMode[STRING_LENGTH_64];
	char					ap_OperatingFrequencyBand[STRING_LENGTH_16];
	char					ap_SupportedStandards[STRING_LENGTH_64];
	char					ap_OperatingStandards[STRING_LENGTH_16];
	char					ap_OperatingChannelBandwidth[STRING_LENGTH_16];
	uint					ap_BeaconPeriod;
	int					ap_Noise;
	char					ap_BasicDataTransferRates[BUF_SIZE];
	char					ap_SupportedDataTransferRates[BUF_SIZE];
	uint					ap_DTIMPeriod;
	uint					ap_ChannelUtilization;
} wldm_neighbor_ap2_t;

/* Wifi Scan Modes */
typedef enum {
	WLDM_RADIO_SCAN_MODE_NONE = 0,
	WLDM_RADIO_SCAN_MODE_FULL,
	WLDM_RADIO_SCAN_MODE_ONCHAN,
	WLDM_RADIO_SCAN_MODE_OFFCHAN,
	WLDM_RADIO_SCAN_MODE_SURVEY,
	WLDM_RADIO_SCAN_MODE_DCS
} wldm_neighborScanMode_t;

typedef struct wifi_scan_params {
	wldm_neighborScanMode_t			scan_mode;
	int					dwell_time;
	uint					num_channels;
	uint					*chan_list;
} wldm_scan_params_t;
extern int wldm_xbrcm_scan(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);

typedef enum wldm_xbrcm_factory_reset_cmd {
	WLDM_NVRAM_FACTORY_RESTORE = 0,
	WLDM_NVRAM_FACTORY_RESET_RADIO,
	WLDM_NVRAM_FACTORY_RESET_AP,
	WLDM_NVRAM_FACTORY_RESET_APSEC
} wldm_xbrcm_factory_reset_cmd_t;

#define NVRAM_FACTORY_DEFAULT_RADIO	"/usr/local/etc/wlan/nvram_default_radio.txt"
#define NVRAM_FACTORY_DEFAULT_AP	"/usr/local/etc/wlan/nvram_default_ap.txt"
#define NVRAM_FACTORY_DEFAULT_TMP	"/tmp/nvram_default_tmp.txt"

extern int wldm_xbrcm_factory_reset(wldm_xbrcm_factory_reset_cmd_t cmd_id, int index,
	int commit, int restart);

extern int wldm_xbrcm_11ac(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);
extern int wldm_11h_dfs(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);

/* 11K */
#define WLDM_RRM_CAP_NEIGHBOR_REPORT_ENABLE     (1 << DOT11_RRM_CAP_NEIGHBOR_REPORT)
#define RRM_CAP_NEIGHBOR_REPORT_SET(activate, rrmcap) \
	(activate) ? (rrmcap |= WLDM_RRM_CAP_NEIGHBOR_REPORT_ENABLE) : \
		(rrmcap &= ~(WLDM_RRM_CAP_NEIGHBOR_REPORT_ENABLE))
#define RRM_CAP_NEIGHBOR_REPORT_GET(rrmcap) \
	(rrmcap & (WLDM_RRM_CAP_NEIGHBOR_REPORT_ENABLE)) ? 1 : 0

/* Length till bssid (without optional elements) in wifi_BeaconRequest_t */
#define RM_BCN_REQ_MANDATORY_ELEM_LEN	13

#define RMCAPF				"%02x %02x %02x %02x %02x"
#define RMCAP_TO_RMCAPF(rmcap)		rmcap[4], rmcap[3], rmcap[2], rmcap[1], rmcap[0]

typedef struct {
	unsigned char rm_actionId;		/* for a DOT11_RM_ACTION_xxxx */
	unsigned char rm_ieType;		/* for a DOT11_MEASURE_TYPE_xxx */
	void *in_request;
	int in_reqLen;
	unsigned short numRepetitions;
} wl_af_rrm_req_info_t;

typedef enum wldm_rrm_cmd {
	WLDM_RRM_SEND_REQUEST,
	WLDM_RRM_GET_CLIENT_RRM_CAP,
	WLDM_RRM_SET_NEIGHBOR_REPORTS
} wldm_rrm_cmd_t;

extern int wldm_xbrcm_AccessPoint_RMCapabilities(int cmd, int apIndex, unsigned char *pvalue,
	int *plen, char *pbuf, int *pbufsz);
extern int wldm_11k_rrm_cmd(wldm_rrm_cmd_t action, int apIndex, unsigned char *peer_mac,
	unsigned char *pvalue, int *plen, unsigned char *outBuf, int *outlen);

#ifdef RDKB_RADIO_STATS_MEASURE /* definition changed, keep the old one as refernece */
/* per radio based, need prefix wl0_/wl1_ */
#define NVRAM_RADIO_STATS_MEAS_RATE "radio_stats_measure_rate"
#define NVRAM_RADIO_STATS_MEAS_INTEVAL "radio_stats_measure_interval"
#define TMP_RADIO_STATS_FILE "radio_stats.txt"
#endif /* RDKB_RADIO_STATS_MEASURE */

/* proprietary support for 11r (Fast BSS Transition, short for Fast Transition or FT) */
typedef enum wldm_ft_action {
	WLDM_FT_GET_ACTIVATED = 0,
	WLDM_FT_GET_ACTIVATED_NVRAM,
	WLDM_FT_SET_ACTIVATED,
	WLDM_FT_SET_ACTIVATED_NVRAM,
	WLDM_FT_GET_OverDSACTIVATED,
	WLDM_FT_SET_OverDSACTIVATED,
	WLDM_FT_GET_MobilityDomainID,
	WLDM_FT_SET_MobilityDomainID,
	WLDM_FT_GET_R0KeyHolderID,
	WLDM_FT_SET_R0KeyHolderID,
	WLDM_FT_GET_R1KeyHolderID,
	WLDM_FT_SET_R1KeyHolderID
} wldm_ft_action_t;

static inline boolean wldm_ap_enabled(int apIndex) {
	boolean enable = FALSE;
	int plen;
	if (wldm_AccessPoint_Enable(CMD_GET_NVRAM, apIndex,
		(boolean *)&enable, &plen, NULL, NULL) < 0 || enable == FALSE) {
		return 0;
	}
	return enable;
}
#define WLDM_AP_DISABLED(apIndex) (!wldm_ap_enabled(apIndex))
#define WLDM_AP_ENABLED(apIndex) (wldm_ap_enabled(apIndex))

#define FT_LEN_MDID 3
#define FT_LEN_KHID 48

extern int wldm_11r_ft(wldm_ft_action_t action, int apIndex, void *pvalue, int *plen, char *pbuf);

typedef struct _wldm_channel {
	uint8 control;
	uint8 ext20;
	uint8 ext40[2];
	uint8 ext80[4];
} wldm_channel_t;

typedef enum _wldm_channelState {
	WLDM_CHAN_STATE_AVAILABLE = 1,
	WLDM_CHAN_STATE_DFS_NOP_FINISHED,
	WLDM_CHAN_STATE_DFS_NOP_START,
	WLDM_CHAN_STATE_DFS_CAC_START,
	WLDM_CHAN_STATE_DFS_CAC_COMPLETED
} wldm_channelState_t;

typedef struct _wldm_channelMap_t {
	uint ch_number;
	wldm_channelState_t ch_state;
} wldm_channelMap_t;

extern int wldm_xplume_opensync(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);

/* uint32 list - as in wl_uint32_list_t wlioctl.h*/
typedef struct _wl_uint32_list {
        /** in - # of elements, out - # of entries */
        uint32 count;
        /** variable length uint32 list */
        uint32 element[1];
} wldm_uint32_list_t;

/* to match wifi_freq_bands_t in wifi_hal.h v3.0 */
typedef enum {
	WLDM_FREQUENCY_2_4_BAND	=	0x1,
	WLDM_FREQUENCY_5_BAND	=	0x2,
	WLDM_FREQUENCY_5L_BAND	=	0x4,
	WLDM_FREQUENCY_5H_BAND	=	0x8,
	WLDM_FREQUENCY_6_BAND	=	0x10,
	WLDM_FREQUENCY_60_BAND	=	0x20
} wldm_freq_bands_t;

typedef struct _wldm_atm_staperc_t {
	char macstr[ETHER_ADDR_STR_LEN];	/* MAC addr */
	uint8 perc;				/* SCB ATM percentage */
} wldm_atm_staperc_t;

extern int wldm_xbrcm_atm(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);

#define WLDM_MAX_SECONDARY_CHANNELS	7
/* wldm_xbrcm_radio_param_t tries to mirror wifi_radio_operationParam_t in wifi_hal_radio.h
 * with exception of similar elements may have different data types. eg. band
 * since wifi_hal_xx.h are maintained by third parties, there is no guarrentee of one to one
 * match of each elements between the two structs.
 * wldm is responsible for set/get data from wifi driver, while wifi_hal is responsible for
 * packing and converting data to types defined by wifi_hal_xx.h
 */
typedef struct _wldm_xbrcm_radio_param {
	boolean enable;				/* The radio enable. */
	char band[STRING_LENGTH_32];		/* The radio frequency band */
	boolean AutoChannelEnable;		/* AutoChannelEnable flag */
	uint channel;				/* The radio primary channel. */
	char bandwidth[STRING_LENGTH_32];	/* The channel bandwidth */
	uint32 numSecondaryChannels;		/* The number of secondary channels in the list */
	uint32 channelSecondary[WLDM_MAX_SECONDARY_CHANNELS];	/* secondary radio channel list. */
	char variant[STRING_LENGTH_32];		/* The radio operating mode */
	uint32 csa_beacon_count;		/* Specifies how long CSA need to be announced. */
	char countryCode[STRING_LENGTH_32];	/* The country code. */
	boolean DCSEnabled;			/* Set DCSEnabled to TRUE to enable DCS. */
	uint32 dtimPeriod;			/* The DTIM period. */
	uint32 beaconInterval;			/* The beacon interval. */
	uint32 operatingClass;			/* The Operating class. */
	char basicDataTransmitRates[STRING_LENGTH_32];	/* The basic data transmit rates in Mbps. */
	uint32 operationalDataTransmitRates;	/* The operational data transmit rates in Mbps. */
	uint32 fragmentationThreshold;		/* The fragmentation threshold in bytes. */
	uint32 guardInterval;			/* The guard interval. */
	uint32 transmitPower;			/* Transmit power in percentage, eg "75", "100". */
	uint32 rtsThreshold;			/* Packet threshold in bytes for RTS/CTS backoff */
} wldm_xbrcm_radio_param_t;

extern int wldm_xbrcm_radio(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz);

/* TWT related definitions */
#ifdef WL_TWT_LIST_VER
#define WLDM_TWT_CMD_ENAB		WL_TWT_CMD_ENAB
#define WLDM_TWT_CMD_SETUP		WL_TWT_CMD_SETUP
#define WLDM_TWT_CMD_TEARDOWN		WL_TWT_CMD_TEARDOWN
#define WLDM_TWT_CMD_INFO		WL_TWT_CMD_INFO
#define WLDM_TWT_CMD_LIST		WL_TWT_CMD_LIST

#if (WL_TWT_LIST_VER >= 2)
#define wldm_twt_list_t			wl_twt_list_v2_t
#define wldm_twt_sdesc_t		wl_twt_sdesc_v2_t
#else
#define wldm_twt_list_t			wl_twt_list_t
#define wldm_twt_sdesc_t		wl_twt_sdesc_t
#endif /* WL_TWT_LIST_VER >= 2 */
#define wldm_twt_teardown_t		wl_twt_teardown_t

/* match with wlc_twt.c WLC_xxx */
#define WLDM_SCHEDID_TSF_SHIFT		12
#define WLDM_TSFL_SCHEDID_MASK_INV	((1 << WLDM_SCHEDID_TSF_SHIFT) - 1)
#define WLDM_TSFL_SCHEDID_MASK		(~WLDM_TSFL_SCHEDID_MASK_INV)
#define WLDM_TSFL_TO_SCHEDID(tsfl)	(uint16)((tsfl) >> WLDM_SCHEDID_TSF_SHIFT)
#define WLDM_SCHEDID_TO_TSFL(schedid)	((schedid) << WLDM_SCHEDID_TSF_SHIFT)
/* Wake duration is in multiple of 4msec schedid, but there is a minimum of 8msec. */
#define WLDM_TWT_MIN_WAKE_DURATION	WLDM_SCHEDID_TO_TSFL(2)
#define WLDM_TWT_WAKE_64k_USEC		(256 << 8) /* limit of wake_duration for WFA R1 */

typedef struct wldm_twt_list_all {
	int			devcnt;
	wldm_twt_list_t		tlist[0];
} wldm_twt_list_all_t;

typedef struct wldm_twt_sess_info {
	struct ether_addr	peer;	/* associated mac */
	unsigned int		twtId;	/* id as in twt list */
	unsigned int		agtype;	/* individual or broadcast */
	unsigned int		sessId;	/* id for hal api twt teardown */
} wldm_twt_sess_info_t;

/* for broadcast twt setup and teardown */
typedef struct wldm_twt_setup_info {
	wldm_twt_params_t	tparams;
	boolean			create;
	int			sessId;
} wldm_twt_setup_info_t;

typedef struct wldm_twt_cmd_info {
	unsigned int	twtCmd;	/* see wlioctl for TWT top level command IDs */
	unsigned char	twtCmdInfo[512];
} wldm_twt_cmd_info_t;
#endif /* WL_TWT_LIST_VER */

#define WLDM_MAX_CH_LIST_LEN		128
typedef struct _wldm_channel_weights_s {
	uint16 count;
	uint8 ch_list[WLDM_MAX_CH_LIST_LEN];
	uint16 wt_list[WLDM_MAX_CH_LIST_LEN];
} wldm_channel_weights_t;

#define DEFAULT_ASSOC_DEV_HWM_TH 50
extern int wldm_xbrcm_assoc_dev_hwm(int cmd, int index, void *pvalue, uint *plen, char *pvar,
	int *pvarsz);

typedef enum _wldm_dfs_chan_status_t {
	WLDM_DFS_CAC_PERIOD = 0,
	WLDM_DFS_CLEARED,
	WLDM_DFS_NOT_CLEARED
} wldm_dfs_chan_status_t;

typedef struct _wldm_dfs_status_t {
        unsigned long channel;
        wldm_dfs_chan_status_t status;
} wldm_dfs_status_t;

#endif /* __WLDM_LIB_WIFI_H__ */
