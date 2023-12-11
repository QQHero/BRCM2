/*
 *  TR181 Data Model
 *
 *  Copyright 2022 Broadcom
 *
 *  This program is the proprietary software of Broadcom and/or
 *  its licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.  IF YOU HAVE NO
 *  AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 *  WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 *  THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1. This program, including its structure, sequence and organization,
 *  constitutes the valuable trade secrets of Broadcom, and you shall use
 *  all reasonable efforts to protect the confidentiality thereof, and to
 *  use this information only in connection with your use of Broadcom
 *  integrated circuit products.
 *
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *  REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 *  OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *  DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *  NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *  ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *  CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *  OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 *  BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 *  SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 *  IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *  IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 *  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 *  OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 *  NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *  <<Broadcom-WL-IPTag/Proprietary:>>
 *
 *  $Id: $
 */
#ifndef _WIFI_TR181_H_
#define _WIFI_TR181_H_

/*
*  Device.WiFi. TR181 Data Model 2.12
*     https://cwmp-data-models.broadband-forum.org/tr-181-2-12-0-cwmp-full.xml
*/

typedef bool		boolean;
typedef char *		hexBinary;
typedef char *		IPAddress;
typedef char *		list;					/* max 1024 */
typedef uint32		StatsCounter32;
typedef uint64		StatsCounter64;
typedef char *		string;
typedef unsigned int	unsignedInt;
typedef uint64		unsignedLong;

/*
*  String sizes. See https://cwmp-data-models.broadband-forum.org/tr-181-2-5-0.html#H.Data%20Types
*/
#define TR181_STR_64			(64)
#define TR181_STR_256			(256)
#define TR181_STR_ALIAS			TR181_STR_64
#define TR181_STR_IPADDR		(45)			/* v4 and v6 */
#define TR181_STR_IPPREFIX		(49)
#define TR181_STR_IP4ADDR		(15)			/* v4 only */
#define TR181_STR_IP4PREFIX		(19)			/* v4 only */
#define TR181_STR_MACADDR		(17)

/* Currently only the objects with RW paremters are constructed for SetParameterValues check */

typedef struct _Device_WiFi {
	unsignedInt	RadioNumberOfEntries;			/* RO */
	unsignedInt	SSIDNumberOfEntries;			/* RO */
	unsignedInt	AccessPointNumberOfEntries;		/* RO */
	unsignedInt	EndPointNumberOfEntries;		/* RO */
	StatsCounter32	ResetCounter;				/* RO, 2.12 */
	boolean		Reset;					/* RW, 2.12 */
}	Device_WiFi;

typedef struct _Device_WiFi_Radio {
	boolean		Enable;					/* RW, 2.0 */
	string		Status;					/* RO, 2.0 */
			/* Up, Down, Unknown, Dormant, NotPresent, LowerLayerDown, Error(optional)
			*/
	string		Alias;					/* RW, 2.0 */
	string		Name;					/* RO, 2.0 */
	unsignedInt	LastChange;				/* RO, 2.0, in sec, canDeny */
	list		LowerLayers;				/* RW, 2.0, not used */
	boolean		Upstream;				/* RO, 2.0, deprecated */
	unsignedInt	MaxBitRate;				/* RO, 2.0, in Mbps */
	list		SupportedFrequencyBands;		/* RO, 2.0, 2.4GHz, 5GHz */
	string		OperatingFrequencyBand;			/* RW, 2.0 */
	list		SupportedStandards;			/* RO, 2.0, a b g n ac */
	list		OperatingStandards;			/* RW, 2.0, a b g n ac */
	list		PossibleChannels;			/* RO, 2.0 */
	list		ChannelsInUse;				/* RO, 2.0, canDeny */
	unsignedInt	Channel;				/* RW, 2.0 */
	boolean		AutoChannelSupported;			/* RO, 2.0 */
	boolean		AutoChannelEnable;			/* RW, 2.0 */
	unsignedInt	AutoChannelRefreshPeriod;		/* RW, 2.0, in sec */
	unsignedInt	ChannelLastChange;			/* RO, 2.12, in sec */
	string		ChannelLastSelectionReason;		/* RO, 2.12 */
			/* "Manual", "Auto_Startup", "Auto_User", "Auto_Refresh", "Auto_Dynamic",
			*  "Auto_DFS", "Unknown"
			*/
	unsignedInt	MaxSupportedSSIDs;			/* RO, 2.12, >= 1 */
	unsignedInt	MaxSupportedAssociations;		/* RO, 2.12, >= 1 */
	string		FirmwareVersion;			/* RO, 2.12, maxlen 64 */
	list		SupportedOperatingChannelBandwidths;	/* RO, 2.12 */
			/* "20MHz", "40MHz", "80MHz", "160MHz", "Auto" */
	string		OperatingChannelBandwidth;		/* RW, 2.0 */
	string		CurrentOperatingChannelBandwidth;	/* RO, 2.11 */
	string		ExtensionChannel;			/* RW, 2.0 */
			/* "AboveControlChannel", "BelowControlChannel", "Auto" */
	string		GuardInterval;				/* RW, 2.0 */
			/* "400nsec", "800nsec", "Auto" */
	int		MCS;					/* RW, 2.0, -1~15, 16~31 */
	list		TransmitPowerSupported;			/* RO, 2.0, -1~100 % */
	int		TransmitPower;				/* RW, 2.0, -1~100 % */
	boolean		IEEE80211hSupported;			/* RO, 2.0 */
	boolean		IEEE80211hEnabled;			/* RW, 2.0 */
	string		RegulatoryDomain;			/* RW, 2.0 */
	unsignedInt	RetryLimit;				/* RW, 2.8 */
	hexBinary	CCARequest;				/* RW, 2.8, len=11 */
	hexBinary	CCAReport;				/* RO, 2.8, len=12 */
	hexBinary	RPIHistogramRequest;			/* RW, 2.8, len=11 */
	hexBinary	RPIHistogramReport;			/* RO, 2.8, len=19 */
	unsignedInt	FragmentationThreshold;			/* RW, 2.8, in octets */
	unsignedInt	RTSThreshold;				/* RW, 2.8, in octets */
	unsignedInt	LongRetryLimit;				/* RW, 2.8 */
	unsignedInt	BeaconPeriod;				/* RW, 2.8, in msec */
	unsignedInt	DTIMPeriod;				/* RW, 2.8 */
	boolean		PacketAggregationEnable;		/* RW, 2.8 */
	string		PreambleType;				/* RW, 2.8 */
			/* "short", "long", "auto" */
	list		BasicDataTransmitRates;			/* RW, 2.8 */
	list		OperationalDataTransmitRates;		/* RW, 2.8 */
	list		SupportedDataTransmitRates;		/* RO, 2.8 */
}	Device_WiFi_Radio;

typedef struct {
	int	aci;		/**< Access catagory index */
	char	aifsn;		/**< Arbitration Inter-Frame Space Number */
	char	ecw_min;	/**< Lower bound Contention Window. */
	char	ecw_max;	/**< Upper bound Contention Window. */
	char	timer;		/**< */
} wldm_wifi_edca_t;

typedef struct _Device_WiFi_X_BROADCOM_COM_Radio {
	boolean		AxEnable;				/* RW, HE enable OR Disable flag */
	unsigned int	AxFeatures;				/* RW, Fetches OR Configures HE Features */
	unsigned int	AxBsscolor;				/* RW, Enables/disables HE BSS Coloring */
	unsigned int	AxMuType;				/* 11AX HE MU Type */
	boolean		STBCEnable;				/* RW, Enable/disable STBC */
	int		TxChainMask;				/* RW, Fetches OR Configures Tx Chain */
	int		RxChainMask;				/* RW, Fetches OR Configures Rx Chain */
	boolean		Greenfield11nEnable;			/* RW, Enable/disable 11n Green Field */
	boolean		CtsProtectionEnable;			/* RW, Enable/disable Cts Protection */
	int		ResetCount;				/* RW, Fetch Or Set Radio Reset Count */
	wldm_wifi_edca_t	AxMuEdca;			/* MUEDCA structure */
}	Device_WiFi_X_BROADCOM_COM_Radio;

typedef struct _Device_WiFi_X_RDK_Radio {
	boolean		amsdu;					/* amsdu enable disable */
	int		AutoChannelDwellTime;
	boolean		DfsSupport;
	boolean		DfsEnable;
}	Device_WiFi_X_RDK_Radio;

typedef struct _Device_WiFi_SSID {
	boolean		Enable;					/* RW, 2.0 */
	string		Status;					/* RO, 2.0 */
			/* Up, Down, Unknown, Dormant, NotPresent, LowerLayerDown, Error(optional)
			*/
	string		Alias;					/* RW, 2.0 */
	string		Name;					/* RO, 2.0 */
	unsignedInt	LastChange;				/* RO, 2.0, in sec, canDeny */
	list		LowerLayers;				/* RW, 2.0 */
	char		BSSID[6];				/* RO, 2.0 */
	char		MACAddress[6];				/* RO, 2.0 */
	string		SSID;					/* RW, 2.0 */
	boolean		Upstream;				/* RO, 2.0 */
}	Device_WiFi_SSID;

typedef struct _Device_WiFi_AccessPoint {
	boolean		Enable;					/* RW, 2.0 */
	string		Status;					/* RO, 2.0 */
			/* Up, Down, Unknown, Dormant, NotPresent, LowerLayerDown, Error(optional)
			*/
	string		Alias;					/* RW, 2.0 */
	string		SSIDReference;				/* RW, 2.0, max len 256 */
	boolean		SSIDAdvertisementEnabled;		/* RW, 2.0 */
	unsignedInt	RetryLimit;				/* RW, 2.0 */
	boolean		WMMCapability;				/* RO, 2.0 */
	boolean		UAPSDCapability;			/* RO, 2.0 */
	boolean		WMMEnable;				/* RW, 2.0 */
	boolean		UAPSDEnable;				/* RW, 2.0 */
	unsignedInt	AssociatedDeviceNumberOfEntries;	/* RO, 2.0 */
	unsignedInt	MaxAssociatedDevices;			/* RW, 2.4 */
	boolean		IsolationEnable;			/* RW, 2.4 */
	boolean		MACAddressControlEnabled;		/* RW, 2.9 */
	list		AllowedMACAddress;			/* RW, 2.9 */
	unsignedInt	MaxAllowedAssociations;			/* RW, 2.12 */
}	Device_WiFi_AccessPoint;

typedef struct _Device_WiFi_X_RDK_AccessPoint {
	unsignedInt	MACAddressControMode;
			/* RW   0 : Disable MAC address matching ,
			*	1 : Deny association to stations on the MAC list
			*	2 : Allow association to stations on the MAC list
			*/
	string		MACAddresslist;
}	Device_WiFi_X_RDK_AccessPoint;

typedef struct _Device_WiFi_X_BROADCOM_COM_AccessPoint {
	unsigned char	RMCapabilities[DOT11_RRM_CAP_LEN];	/* 11K RM Capabilties */
}	Device_WiFi_X_BROADCOM_COM_AccessPoint;

typedef struct _Device_WiFi_AccessPoint_Security {
	list		ModesSupported;				/* RO, 2.0 */
			/* "None", "WEP-64", "WEP-128", "WPA-Personal", "WPA2-Personal",
			*  "WPA-WPA2-Personal", "WPA-Enterprise", "WPA2-Enterprise",
			*  "WPA-WPA2-Enterprise"
			*/
	string		ModeEnabled;				/* RW, 2.0 */
	hexBinary	WEPKey;					/* RW, 2.0, len 5 or 13 */
	hexBinary	PreSharedKey;				/* RW, 2.0, maxlen 32 */
	string		KeyPassphrase;				/* RW, 2.0, len 8~63 */
	unsignedInt	RekeyingInterval;			/* RW, 2.0, in sec */
	IPAddress	RadiusServerIPAddr;			/* RW, 2.0 */
	IPAddress	SecondaryRadiusServerIPAddr;		/* RW, 2.5 */
	unsignedInt	RadiusServerPort;			/* RW, 2.0 */
	unsignedInt	SecondaryRadiusServerPort;		/* RW, 2.5 */
	string		RadiusSecret;				/* RW, 2.0, hidden */
	string		SecondaryRadiusSecret;			/* RW, 2.5, hidden */
	string		MFPConfig;				/* RW, 2.11 */
			/* "Disabled", "Optional", "Required" */
	boolean		Reset;					/* RW, 2.4 */
}	Device_WiFi_AccessPoint_Security;

typedef struct _Device_WiFi_AccessPoint_X_RDK_Security {
	string		BasicAuthentication;			/* RW, RDKB specific */
	string		Encryption;				/* RW, RDKB specific */
	unsignedInt	AuthMode;				/* RW, RDKB specific */
	unsignedInt	RadiusReAuthInterval;			/* RW, RDKB specific */
	string		RadiusOperatorName;			/* RW, RDKB specific */
	string		RadiusLocationData;			/* RW, RDKB specific */
	boolean		RadiusGreylistEnable;			/* RW, RDKB specific */
	unsignedInt	RadiusDASPort;				/* RW, RDKB specific */
	IPAddress	RadiusDASClientIPAddr;			/* RW, RDKB specific */
	string		RadiusDASSecret;			/* RW, RDKB specific */
	unsignedInt	WPAPairwiseRetries;			/* RW, RDKB specific */
	unsignedInt	WPAPMKLifetime;				/* RW, RDKB specific */
	list		EncryptionModesSupported;		/* RO, RDKB specific */
	boolean		WPA3TransitionDisable;			/* RW, RDKB specific */
}	Device_WiFi_AccessPoint_X_RDK_Security;

typedef struct Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings {
	unsignedInt	RadiusServerRetries;			/* RW */
	unsignedInt	RadiusServerRequestTimeout;		/* RW */
	unsignedInt	PMKLifetime;				/* RW */
	boolean		PMKCaching;				/* RW */
	unsignedInt	PMKCacheInterval;			/* RW */
	unsignedInt	MaxAuthenticationAttempts;		/* RW */
	unsignedInt	BlacklistTableTimeout;			/* RW */
	unsignedInt	IdentityRequestRetryInterval;		/* RW */
	unsignedInt	QuietPeriodAfterFailedAuthentication;	/* RW */
}	Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings;

typedef struct _Device_WiFi_AccessPoint_WPS {
	boolean		Enable;					/* RW, 2.0 */
	list		ConfigMethodsSupported;			/* RO, 2.0 */
			/* "USBFlashDrive", "Ethernet", "Label", "Display", "ExternalNFCToken",
			*  "IntegratedNFCToken", "NFCInterface", "PushButton", "PIN",
			*  "PhysicalPushButton", "PhysicalDisplay", "VirtualPushButton",
			*  "VirtualDisplay"
			*/
	list		ConfigMethodsEnabled;			/* RW, 2.0 */
	string		Status;					/* RO, 2.0 */
			/* "Disabled", "Error", "Unconfigured", "Configured", "SetupLocked" */
	string		Version;				/* RO, 2.11 */
	string		PIN;					/* RW, 2.11 */
			/* "\d{4}|\d{8}", hidden */
}	Device_WiFi_AccessPoint_WPS;

typedef struct _Device_WiFi_AccessPoint_AssociatedDevice {
	char		MACAddress[6];				/* RO, 2.0 */
	string		OperatingStandard;			/* RO, 2.0, a b g n ac */
	boolean		AuthenticationState;			/* RO, 2.0 */
	unsignedInt	LastDataDownlinkRate;			/* RO, 2.0, >=1000 kbps */
	unsignedInt	LastDataUplinkRate;			/* RO, 2.0, >=1000 kbps */
	unsignedInt	AssociationTime;			/* RO, 2.12, Seconds in UTC */
	int		SignalStrength;				/* RO, 2.0, -200~0 dBm */
	int		Noise;					/* RO, 2.12, -200~0 dBm */
	unsignedInt	Retransmissions;			/* RO, 2.0, 0~100, per 100 pkts */
	boolean		Active;					/* RO, 2.0 */
}	Device_WiFi_AccessPoint_AssociatedDevice;

typedef struct _Device_WiFi_AccessPoint_AssociatedDevice_Stats {
	StatsCounter64	BytesSent;				/* RO, 2.8 */
	StatsCounter64	BytesReceived;				/* RO, 2.8 */
	StatsCounter64	PacketsSent;				/* RO, 2.8 */
	StatsCounter64	PacketsReceived;			/* RO, 2.8 */
	StatsCounter32	ErrorsSent;				/* RO, 2.8 */
	StatsCounter32	RetransCount;				/* RO, 2.8 */
	StatsCounter32	FailedRetransCount;			/* RO, 2.8 */
	StatsCounter32	RetryCount;				/* RO, 2.8 */
	StatsCounter32	MultipleRetryCount;			/* RO, 2.8 */
}	Device_WiFi_AccessPoint_AssociatedDevice_Stats;

typedef struct _Device_WiFi_AccessPoint_AC {
	string		AccessCategory;				/* RO, 2.8 */
			/* "BE", "BK", "VI", "VO" */
	string		Alias;					/* RW, 2.8 */
	unsignedInt	AIFSN;					/* RW, 2.8, 2~15 */
	unsignedInt	ECWMin;					/* RW, 2.8, 0~15 */
	unsignedInt	ECWMax;					/* RW, 2.8, 0~15 */
	unsignedInt	TxOpMax;				/* RW, 2.8, 0~255, in 32us */
	boolean		AckPolicy;				/* RW, 2.8 */
	list		OutQLenHistogramIntervals;		/* RW, 2.8 */
	unsignedInt	OutQLenHistogramSampleInterval;		/* RW, 2.8, in sec */
}	Device_WiFi_AccessPoint_AC;

typedef struct _Device_WiFi_AccessPoint_X_LGI_WiFiSupportedRates
{
	boolean		Enable;
	string		BasicRatesBitMap;
	string		SupportedRatesBitMap;
}	Device_WiFi_AccessPoint_X_LGI_WiFiSupportedRates;

typedef struct _Device_WiFi_SSID_Stats
{
	unsignedLong	BytesSent;
	unsignedLong	BytesReceived;
	unsignedLong	PacketsSent;
	unsignedLong	PacketsReceived;
	unsignedLong	RetransCount;
	unsignedLong	FailedRetransCount;
	unsignedLong	RetryCount;
	unsignedLong	MultipleRetryCount;
	unsignedLong	ACKFailureCount;
	unsignedLong	AggregatedPacketCount;
	unsignedLong	ErrorsSent;
	unsignedLong	ErrorsReceived;
	unsignedLong	UnicastPacketsSent;
	unsignedLong	UnicastPacketsReceived;
	unsignedLong	DiscardPacketsSent;
	unsignedLong	DiscardPacketsReceived;
	unsignedLong	MulticastPacketsSent;
	unsignedLong	MulticastPacketsReceived;
	unsignedLong	BroadcastPacketsSent;
	unsignedLong	BroadcastPacketsReceived;
	unsignedLong	UnknownProtoPacketsReceived;
}	Device_WiFi_SSID_Stats;

typedef struct _Device_WiFi_Radio_TrafficStats2
{
	unsigned long	radio_BytesSent;
	unsigned long	radio_BytesReceived;
	unsigned long	radio_PacketsSent;
	unsigned long	radio_PacketsReceived;
	unsigned long	radio_ErrorsSent;
	unsigned long	radio_ErrorsReceived;
	unsigned long	radio_DiscardPacketsSent;
	unsigned long	radio_DiscardPacketsReceived;
	unsigned long	radio_PLCPErrorCount;
	unsigned long	radio_FCSErrorCount;
	unsigned long	radio_InvalidMACCount;
	unsigned long	radio_PacketsOtherReceived;
	int		radio_NoiseFloor;
	unsigned long	radio_ChannelUtilization;
	int		radio_ActivityFactor;
	int		radio_CarrierSenseThreshold_Exceeded;
	int		radio_RetransmissionMetirc;
	int		radio_MaximumNoiseFloorOnChannel;
	int		radio_MinimumNoiseFloorOnChannel;
	int		radio_MedianNoiseFloorOnChannel;
	unsigned long	radio_StatisticsStartTime;
} Device_WiFi_Radio_TrafficStats2;

typedef struct _Device_WiFi_AccessPoint_Accounting {
	boolean		Enable;					/* RW, 2.5 */
	IPAddress	ServerIPAddr;				/* RW, 2.5 */
	IPAddress	SecondaryServerIPAddr;			/* RW, 2.5 */
	unsignedInt	ServerPort;				/* RW, 2.5 */
	unsignedInt	SecondaryServerPort;			/* RW, 2.5 */
	string		Secret;					/* RW, 2.5, hidden */
	string		SecondarySecret;			/* RW, 2.5, hidden */
	unsignedInt	InterimInterval;			/* RW, 2.5 in sec */
}	Device_WiFi_AccessPoint_Accounting;

#endif /* _WIFI_TR181_H_ */
