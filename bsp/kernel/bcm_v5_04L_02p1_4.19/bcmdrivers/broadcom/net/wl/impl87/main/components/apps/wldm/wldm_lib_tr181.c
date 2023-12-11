/*
 * Broadband Forum TR181 Device Data Model Wireless Utility
 * https://cwmp-data-models.broadband-forum.org/tr-181-2-12-0-cwmp-full.xml
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

#include <inttypes.h>
#include <typedefs.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bcmnvram.h>
#include <bcmparams.h>
#include <shutils.h>
#include <wlutils.h>
#include <wlioctl.h>
#include "wldm_lib.h"
#include "wldm_lib_tr181.h"

#ifdef __CONFIG_DHDAP__
#include <dhdioctl.h>

extern int dhd_probe(char *name);
#endif /* __CONFIG_DHDAP__ */

#ifdef BCMDBG
#define ASSERT(statement)	if (!(statement)) \
	fprintf(stderr, "%s/%d: assert failed!", __FUNCTION__, __LINE__);
#else
#define ASSERT(statement)
#endif

#define BPRINTF(...)		do {  printf(__VA_ARGS__);  } while (0)

/* Some Data types for WiFi in TR181. For more details see TR106 or
*  "https://www.broadband-forum.org/cwmp/tr-181-2-5-0.html#H.Data Types"
*/
#define ATTR_TYPE_BOOL		0x01		/* true(1) or false(0) */
#define ATTR_TYPE_SI32		0x02		/* Signed Integer(32 bit) */
#define ATTR_TYPE_UI32		0x04		/* Unsigned Integer(32 bit) */
#define	ATTR_TYPE_UI64		0x08		/* Unsigned Integer(64 bit) */
#define ATTR_TYPE_MAC		0x10		/* Mac address string(17) */
#define ATTR_TYPE_IP		0x20		/* IPAddress string(45) or string(15) */
#define ATTR_TYPE_STR		0x40		/* String(default max is 16) */
#define ATTR_TYPE_PROP		0x80		/* Proprietary */
#define ATTR_TYPE_MASK		0xff

#define	ATTR_TYPE_INT		ATTR_TYPE_SI32	/* Int(32 bit) */
#define	ATTR_TYPE_UINT		ATTR_TYPE_UI32	/* UnsignedInt (32 bit) */
#define	ATTR_TYPE_SC32		ATTR_TYPE_UI32	/* StatsCounter32(32 bit) */
#define	ATTR_TYPE_SC64		ATTR_TYPE_UI64	/* StatsCounter64(64 bit) */
#define	ATTR_TYPE_ULNG		ATTR_TYPE_UI64	/* UnsignedLong(64 bit) */
#define ATTR_TYPE_HEX		ATTR_TYPE_STR
#define ATTR_TYPE_LIST		ATTR_TYPE_STR

/* Data Access(Readonly or Readwrite) */
#define ATTR_ACCS_RO		0x0000
#define ATTR_ACCS_RW		0x1000
#define ATTR_ACCS_MASK		0x1000

/* Implemtation specific attributes */
#define ATTR_ARRY		0x2000		/* Array: followed by placeholder node name "{i}".
						*  eg. Radio, SSID, AccessPoint, Profile, EndPoint, etc.
						*/

#define BUF_SZ			1024

/* Reference instance structure */
#define DM_DEPTH		16
typedef struct _ref_instance {
	int command;				/* API command */
	int fault_code;				/* TR069 falut code */
	char *pathname;				/* Input pathname */
	char *params;				/* Input parameter list */
	char *output_buf;			/* Output buffer from API */
	int output_buf_size;			/* Output buffer size from API */
	char *inpbuf;				/* Input from API */
	char input_buf[BUF_SZ];			/* Normalized input command */

	int target_index;		/* The list found corresponding to pathname */
	struct _dm_list *dm_trace[DM_DEPTH];
	char param_name[TR181_STR_256];		/* param name and value templates */
	char param_value[TR181_STR_64];

	/* Total instances currently from WiFi subsystem */
	int Radios, NeighborDiagResults, SSIDs, APs, EPs, AssociatedDevices, ACs, Profiles;

	/* Reference instance from pathname, instance(X or Y) >=1 */
	int Radio;				/* Radio.X. */
	int NeighborDiagResult;			/* NeighboringWiFiDiagnostic.Result.X. */
	int SSID;				/* SSID.X. */
	int AccessPoint;			/* AccessPoint.X. For EndPoint is 0 */
	union {
		int AssociatedDevice;		/* AccessPoint.X.AssociatedDevice.Y */
		int EndPoint;			/* EndPoint.X. */
	};
	int AC;					/* AccessPoint.X.AC.Y. or EndPoint.X.AC.Y. */
	int Profile;				/* EndPoint.X.Profile.Y */
}	ref_instance_t;

typedef struct _dm_list {
	char *name;				/* Name */
	unsigned int attribute;			/* Type, Access, etc */
	union {
		void *value;
		struct _dm_list *nextlist;	/* Next list, when Type is 0(node) */
		int (*wldm_func)(int cmd, int index, void *pValue, ...);		/* Function call for leaf */
	};
}	dm_list_t;

static int ref_instance_get(ref_instance_t *refinstance, char *name);

/*
*   The leaf parameter function shall handle
*   command(API command):
*      CMD_GET: RO/RW. Be aware of "hidden" like password.
*      CMD_SET: if RW
*/
static int
default_function(dm_list_t *dm_list, ref_instance_t *instance)
{
	if ((instance->command & (CMD_GET | CMD_LIST)) == CMD_LIST) {
		BPRINTF("%s\n", dm_list->name);
	}

	if (instance->command & CMD_GET) {
		if (instance->output_buf == NULL)
			BPRINTF("%s=NA\n", dm_list->name); /* get from driver */
	}

	if (instance->command & CMD_SET) {
		if (dm_list->attribute & ATTR_ACCS_RW)
			BPRINTF("set %s\n", dm_list->name);
		else
			BPRINTF("%s is readonly\n", dm_list->name);
	}
	return 0;
}

/*******************
*  Radio data model.
*******************/
dm_list_t Device_Wifi_Radio_Stats_[] = {	/* Device.WiFi.Radio.{i}.Stats. */
   /* Parameters */
   {	"BytesSent",			(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"BytesReceived",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"PacketsSent",			(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"PacketsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"ErrorsSent",			(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"ErrorsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"DiscardPacketsSent",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"DiscardPacketsReceived",	(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"PLCPErrorCount",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"FCSErrorCount",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"InvalidMACCount",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"PacketsOtherReceived",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"Noise",			(ATTR_ACCS_RO | ATTR_TYPE_INT),		{NULL}	},
   {	"TotalChannelChangeCount",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"ManualChannelChangeCount",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"AutoStartupChannelChangeCount",(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"AutoUserChannelChangeCount",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"AutoRefreshChannelChangeCount",(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"AutoDynamicChannelChangeCount",(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"AutoDFSChannelChangeCount",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_Radio_[] = {		/* Device.WiFi.Radio.{i}. */
   /* Parameters */
   {	"Enable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{wldm_Radio_Enable}	},
   {	"Status",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	},
	/* Up, Down, Unknown, Dormant, NotPresent, LowerLayerDown, Error(optional) */
   {	"Alias",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{wldm_Radio_Alias}	}, /* string(64) */
   {	"Name",				(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	},
   {	"LastChange",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* In sec */
   {	"LowerLayers",			(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	}, /* List, not used */
   {	"MaxBitRate",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* Mbps */
   {	"SupportedFrequencyBands",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /* List: 2.4GHz 5GHz */
   {	"OperatingFrequencyBand",	(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* 2.4GHz 5GHz */
   {	"SupportedStandards",		(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /* List: a b g n ac */
   {	"OperatingStandards",		(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	}, /* 2.4GHz 5GHz */

   {	"PossibleChannels",		(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /* List: 1-255 */
   {	"ChannelsInUse",		(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /* List of busy channels? */
   {	"Channel",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* Get/Set the current channel */
   {	"AutoChannelSupported",		(ATTR_ACCS_RO | ATTR_TYPE_BOOL),	{NULL}	}, /* True or False */
   {	"AutoChannelEnable",		(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* Set True to scan */
   {	"AutoChannelRefreshPeriod",	(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0: boot time. sec */
   {	"ChannelLastChange",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"ChannelLastSelectionReason",	(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* 2.12 */
	/* Manual, Auto_Startup, Auto_User, Auto_Refresh, Auto_Dynamic, Auto_DFS, Unknown */

   {	"MaxSupportedSSIDs",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"MaxSupportedAssociations",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
   {	"FirmwareVersion",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* 2.12 */

   {	"SupportedOperatingChannelBandwidths", (ATTR_ACCS_RO | ATTR_TYPE_STR),	{NULL}	}, /* 2.12 */
	/* 20MHz, 40MHz, 80MHz, 160MHz, Auto */
   {	"OperatingChannelBandwidth",	(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	},
   {	"CurrentOperatingChannelBandwidth", (ATTR_ACCS_RO | ATTR_TYPE_STR),	{NULL}	}, /* 2.11 */
   {	"ExtensionChannel",		(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	},
	/* AboveControlChannel, BelowControlChannel, Auto */

   {	"GuardInterval",		(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* 400nsec, 800nsec, Auto */
   {	"MCS",				(ATTR_ACCS_RW | ATTR_TYPE_INT),		{NULL}	}, /* -1(auto)-15, 16-31, Auto */
   {	"TransmitPowerSupported",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /* -1(auto). eg: 0,25,50,75,100 */
   {	"TransmitPower",		(ATTR_ACCS_RW | ATTR_TYPE_INT),		{NULL}	}, /* -1(auto) - 100 */
   {	"IEEE80211hSupported",		(ATTR_ACCS_RO | ATTR_TYPE_BOOL),	{NULL}	}, /* True or False */
   {	"IEEE80211hEnabled",		(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* True or False */
   {	"RegulatoryDomain",		(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* 3 chars */
   {	"RetryLimit",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0 - 7 */
   {	"CCARequest",			(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{NULL}	}, /* HexBinary of length 11 */
   {	"CCAReport",			(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{NULL}	}, /* HexBinary of length 12 */
   {	"RPIHistogramRequest",		(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{NULL}	}, /* HexBinary of length 11 */
   {	"RPIHistogramReport",		(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{NULL}	}, /* HexBinary of length 19 */
   {	"FragmentationThreshold",	(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{wldm_Radio_FragmentationThreshold}	}, /* In octets */
   {	"RTSThreshold",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{wldm_Radio_RTSThreshold}	}, /* In octets */
   {	"LongRetryLimit",		(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* */
   {	"BeaconPeriod",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* In msec */
   {	"DTIMPeriod",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* In beacons */
   {	"PacketAggregationEnable",	(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* Only to 802.11n */
   {	"PreambleType",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* short, long, auto */
   {	"BasicDataTransmitRates",	(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	}, /* List in Mbps */
   {	"OperationalDataTransmitRates",	(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	}, /* List in Mbps for unicast */
   {	"SupportedDataTransmitRates",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	},
	/* List in Mbps for unicast, permit a station to connect */

   /* Intermediate nodes */
   {	"Stats.",			(ATTR_ACCS_RO),				{Device_Wifi_Radio_Stats_}	}, /* Device.WiFi.Radio.X.Stats. */
   {	NULL,				0,					{NULL}	}
};

/*********************************************************
*  SSIDNeighboringWiFiDiagnostic(Scan results) data model.
*********************************************************/
dm_list_t Device_Wifi_NeighboringWiFiDiagnostic_Result_[] = {
   /* Parameters */
   {	"Radio",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* Device.WiFi.Radio.X. */
   {	"SSID",				(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	},
   {	"BSSID",			(ATTR_ACCS_RO | ATTR_TYPE_MAC),		{NULL}	},
   {	"Mode",				(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* AdHoc, Infrastructure */
   {	"Channel",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 1 ~ 255 */
   {	"SignalStrength",		(ATTR_ACCS_RO | ATTR_TYPE_INT),		{NULL}	}, /* -200 ~ 0 dBm */
   {	"SecurityModeEnabled",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	},
	/* None, WEP, WPA, WPA2, WPA-WPA2, WPA-Enterprise, WPA2-Enterprise, WPA-WPA2-Enterprise */
   {	"EncryptionMode",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* TKIP, AES */
   {	"OperatingFrequencyBand",	(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* 2.4GHz, 5GHz */
   {	"SupportedStandards",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* a, b, g, n, ac */
   {	"OperatingStandards",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* a, b, g, n, ac */
   {	"OperatingChannelBandwidth",	(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* 20MHz, 40MHz, 80MHz, 160MHz, Auto */
   {	"BeaconPeriod",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* ms */
   {	"Noise",			(ATTR_ACCS_RO | ATTR_TYPE_INT),		{NULL}	}, /* -200 ~ 0 dBm */
   {	"BasicDataTransferRates",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /* list of rates */
   {	"SupportedDataTransferRates",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /* list of rates */
   {	"DTIMPeriod",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* ms */
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_NeighboringWiFiDiagnostic_[] = {
   /* Parameters */
   {	"DiagnosticsState",		(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	},
	/* None, Requested, Canceled, Complete, Error */
   {	"ResultNumberOfEntries",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"Result.",			(ATTR_ACCS_RO | ATTR_ARRY),		{Device_Wifi_NeighboringWiFiDiagnostic_Result_}	},
   {	NULL,				0,					{NULL}	}
};

/************************
*  SSID(MBSS) data model.
************************/
dm_list_t Device_WiFi_SSID_Stats_[] = {	/* Device.WiFi.Radio.{i}.Stats. */
   /* Parameters */
   {	"BytesSent",			(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"BytesReceived",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"PacketsSent",			(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"PacketsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"ErrorsSent",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"RetransCount",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"FailedRetransCount",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"RetryCount",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"MultipleRetryCount",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"ACKFailureCount",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"AggregatedPacketCount",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"ErrorsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"UnicastPacketsSent",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"UnicastPacketsReceived",	(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"DiscardPacketsSent",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"DiscardPacketsReceived",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"MulticastPacketsSent",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"MulticastPacketsReceived",	(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"BroadcastPacketsSent",		(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"BroadcastPacketsReceived",	(ATTR_ACCS_RO | ATTR_TYPE_ULNG),	{NULL}	},
   {	"UnknownProtoPacketsReceived",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_SSID_[] = {
   /* Parameters */
   {	"Enable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{wldm_SSID_Enable}	}, /* Device.WiFi.Radio.X. */
   {	"Status",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	},
	/* Up, Down, Unknown, Dormant, NotPresent, LowerLayerDown, Error(optional) */
   {	"Alias",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* string(64) */
   {	"Name",				(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* max 64 */
   {	"LastChange",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* In sec */
   {	"LowerLayers",			(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	}, /* List. Eg: Device.WiFi.Radio.X. */
   {	"BSSID",			(ATTR_ACCS_RO | ATTR_TYPE_MAC),		{NULL}	}, /* BSSID */
   {	"MACAddress",			(ATTR_ACCS_RO | ATTR_TYPE_MAC),		{NULL}	}, /* MAC address of this interface */
   {	"SSID",				(ATTR_ACCS_RW | ATTR_TYPE_STR),		{wldm_SSID_SSID}	},
   {	"Upstream",			(ATTR_ACCS_RO | ATTR_TYPE_BOOL),	{NULL}	}, /* 2.12 */

   /* Intermediate nodes */
   {	"Stats.",			(ATTR_ACCS_RO),				{Device_WiFi_SSID_Stats_}	}, /* Device.WiFi.Radio.X.Stats. */
   {	NULL,				0,					{NULL}	}
};

/*******************************************
*  X_COMCAST_COM_RadiusSettings. data model.
********************************************/
static dm_list_t Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings_[] = {	/* Device.WiFi.AccessPoint.{i}.Security.X_COMCAST_COM_RadiusSettings */
   /* Parameters */
   {	"RadiusServerRetries",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"RadiusServerRequestTimeout",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"PMKLifetime",				(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"PMKCaching",				(ATTR_ACCS_RO | ATTR_TYPE_BOOL),	{NULL}	},
   {	"PMKCacheInterval",			(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"MaxAuthenticationAttempts",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"BlacklistTableTimeout",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"IdentityRequestRetryInterval",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
   {	"QuietPeriodAfterFailedAuthentication",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	},
};

/*************************
*  AccessPoint data model.
*************************/
dm_list_t Device_WiFi_AccessPoint_Security_[] = {	/* Device.WiFi.AccessPoint.{i}.Security. */
   /* Parameters */
   {	"ModesSupported",			(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	},
	/* None, WEP-64, WEP-128, WPA-Personal, WPA2-Personal, WPA-WPA2-Personal, WPA-Enterprise, WPA2-Enterprise, WPA-WPA2-Enterprise */
   {	"ModeEnabled",				(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	},
   {	"WEPKey",				(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{NULL}	}, /* HexBinary of length 5 or 13 */
   {	"PreSharedKey",				(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{NULL}	}, /* HexBinary of max length 32 */
   {	"KeyPassphrase",			(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{wldm_AccessPoint_Security_KeyPassphrase}	}, /* HexBinary of length 8 ~ 63 */
   {	"RekeyingInterval",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* default 3600 sec */
   {	"RadiusServerIPAddr",			(ATTR_ACCS_RW | ATTR_TYPE_IP),		{wldm_AccessPoint_Security_RadiusServerIPAddr}	}, /* IP Address */
   {	"SecondaryRadiusServerIPAddr",		(ATTR_ACCS_RW | ATTR_TYPE_IP),		{wldm_AccessPoint_Security_SecondaryRadiusServerIPAddr}	}, /* IP Address */
   {	"RadiusServerPort",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{wldm_AccessPoint_Security_RadiusServerPort}	}, /* default 1812 sec */
   {	"SecondaryRadiusServerPort",		(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{wldm_AccessPoint_Security_SecondaryRadiusServerPort}	}, /* default 1812 sec */
   {	"RadiusSecret",				(ATTR_ACCS_RW | ATTR_TYPE_STR),		{wldm_AccessPoint_Security_RadiusSecret}	}, /* Hidden */
   {	"SecondaryRadiusSecret",		(ATTR_ACCS_RW | ATTR_TYPE_STR),		{wldm_AccessPoint_Security_SecondaryRadiusSecret}	}, /* Hidden */
   {	"MFPConfig",				(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* Disabled, Optional, Required */
   {	"Reset",				(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* true or false */

   /* Intermediate nodes */
   {	"X_COMCAST_COM_RadiusSettings.",	(ATTR_ACCS_RO),				{Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings_}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_AccessPoint_WPS_[] = {	/* Device.WiFi.AccessPoint.{i}.WPS. */
   /* Parameters */
   {	"Enable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* true or false */
   {	"ConfigMethodsSupported",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	},
	/* USBFlashDrive, Ethernet, Label, Display, ExternalNFCToken, IntegratedNFCToken, NFCInterface,
	*  PushButton, PIN, PhysicalPushButton, PhysicalDisplay, VirtualPushButton, VirtualDisplay
	*/
   {	"ConfigMethodsEnabled",		(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	},
   {	"Status",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* 2.11 */
	/* Disabled, Error, Unconfigured, Configured, SetupLocked */
   {	"Version",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* 2.11 */
   {	"PIN",				(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* 2.11 */
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_AccessPoint_AssociatedDevice_Stats_[] = {	/* Device.WiFi.AccessPoint.{i}.AssociatedDevice.Y.Stats. */
   /* Parameters */
   {	"BytesSent",			(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"BytesReceived",		(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"PacketsSent",			(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"PacketsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"ErrorsSent",			(ATTR_ACCS_RO | ATTR_TYPE_UI32),	{NULL}	},
   {	"RetransCount",			(ATTR_ACCS_RO | ATTR_TYPE_UI32),	{NULL}	},
   {	"FailedRetransCount",		(ATTR_ACCS_RO | ATTR_TYPE_UI32),	{NULL}	},
   {	"RetryCount",			(ATTR_ACCS_RO | ATTR_TYPE_UI32),	{NULL}	},
   {	"MultipleRetryCount",		(ATTR_ACCS_RO | ATTR_TYPE_UI32),	{NULL}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_AccessPoint_AssociatedDevice_[] = {
   /* Parameters */
   {	"MACAddress",			(ATTR_ACCS_RO | ATTR_TYPE_MAC),		{NULL}	},
   {	"OperatingStandards",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* a, b, g, n, ac */
   {	"AuthenticationState",		(ATTR_ACCS_RO | ATTR_TYPE_BOOL),	{NULL}	},
   {	"LastDataDownlinkRate",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* kbps */
   {	"LastDataUplinkRate",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* kbps */
   {	"AssociationTime",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 2.12 */
	/* dateTime, Date and time in UTC when the device was associated */
   {	"SignalStrength",		(ATTR_ACCS_RO | ATTR_TYPE_INT),		{NULL}	}, /* -200 ~ 0 in dBm */
   {	"Noise",			(ATTR_ACCS_RO | ATTR_TYPE_INT),		{NULL}	}, /* 2.12 */
   {	"Retransmissions",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* In packets */
   {	"Active",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	},

   /* Intermediate nodes */
   {	"Stats.",			(ATTR_ACCS_RO),				{Device_WiFi_AccessPoint_AssociatedDevice_Stats_}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_AccessPoint_AC_Stats_[] = {	/* Device.WiFi.AccessPoint.{i}.AC.Y.Stats. */
   /* Parameters */
   {	"BytesSent",			(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"BytesReceived",		(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"PacketsSent",			(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"PacketsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"ErrorsSent",			(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"ErrorsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"DiscardPacketsSent",		(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"DiscardPacketsReceived",	(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"RetransCount",			(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"OutQLenHistogram",		(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_AccessPoint_AC_[] = {	/* Device.WiFi.AccessPoint.{i}.AC.Y.Stats. */
   /* Parameters */
   {	"AccessCategory",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* BE, BK, VI, VO */
   {	"Alias",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* string(64) */
   {	"AIFSN",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 2 - 15 */
   {	"ECWMin",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0 - 15 in usec */
   {	"ECWMax",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0 - 15 in usec */
   {	"TxOpMax",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0 - 255 in 32usec unit */
   {	"AckPolicy",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	},
   {	"OutQLenHistogramIntervals",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /*  */
   {	"OutQLenHistogramSampleInterval", (ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /*  */

   /* Intermediate nodes */
   {	"Stats.",			(ATTR_ACCS_RO),				{Device_WiFi_AccessPoint_AC_Stats_}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_AccessPoint_Accounting_[] = {	/* Device.WiFi.AccessPoint.{i}.Accounting. */
   /* Parameters */
   {	"Enable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* BE, BK, VI, VO */
   {	"ServerIPAddr",			(ATTR_ACCS_RW | ATTR_TYPE_IP),		{NULL}	},
   {	"SecondaryServerIPAddr",	(ATTR_ACCS_RW | ATTR_TYPE_IP),		{NULL}	},
   {	"ServerPort",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* default 1813 */
   {	"SecondaryServerPort",		(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* default 1813 */
   {	"Secret",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* Hidden */
   {	"SecondarySecret",		(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* Hidden */
   {	"InterimInterval",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* default 0 in sec */
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_AccessPoint_[] = {	/* Device.WiFi.AccessPoint.{i}. */
   /* Parameters */
   {	"Enable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* true or false */
   {	"Status",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	},
	/* Disabled, Enabled, Error_Misconfigured, Error(optional) */
   {	"Alias",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* */
   {	"SSIDReference",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	},
	/* max 256,  Device.WiFi.SSID.{i}. */
   {	"SSIDAdvertisementEnabled",	(ATTR_ACCS_RO | ATTR_TYPE_BOOL),	{wldm_AccessPoint_SSIDAdvertisementEnabled}	}, /* */
   {	"WMMCapability",		(ATTR_ACCS_RO | ATTR_TYPE_BOOL),	{NULL}	}, /* */
   {	"UAPSDCapability",		(ATTR_ACCS_RO | ATTR_TYPE_BOOL),	{NULL}	}, /* */
   {	"WMMEnable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* */
   {	"UAPSDEnable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* */
   {	"AssociatedDeviceNumberOfEntries", (ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* */
   {	"MaxAssociatedDevices",		(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0: no limit */
   {	"IsolationEnable",		(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{wldm_AccessPoint_IsolationEnable}	}, /* */
   {	"MACAddressControlEnabled",	(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* White list */
   {	"AllowedMACAddress",		(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	}, /* 2.9 */
   {	"MaxAllowedAssociations",	(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	}, /* 2.12 */

   /* Intermediate nodes */
   {	"Security.",			(ATTR_ACCS_RO),				{Device_WiFi_AccessPoint_Security_}	},
   {	"WPS.",				(ATTR_ACCS_RO),				{Device_WiFi_AccessPoint_WPS_}	},
   {	"AssociatedDevice.",		(ATTR_ACCS_RO),				{Device_WiFi_AccessPoint_AssociatedDevice_}	},
   {	"AC.",				(ATTR_ACCS_RO | ATTR_ARRY),		{Device_WiFi_AccessPoint_AC_}	},
   {	"Accounting.",			(ATTR_ACCS_RO),				{Device_WiFi_AccessPoint_Accounting_}	},
   {	NULL,				0,					{NULL}	}
};

/***************************
*  EndPoint(STA) data model.
***************************/
dm_list_t Device_WiFi_EndPoint_Stats_[] = {	/* Device.WiFi.EndPoint.{i}.Stats. */
   /* Parameters */
   {	"LastDataDownlinkRate",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* kbps */
   {	"LastDataUplinkRate",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* kbps */
   {	"SignalStrength",		(ATTR_ACCS_RO | ATTR_TYPE_INT),		{NULL}	}, /* -200 ~ 0 in dBm */
   {	"Retransmissions",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* 0 ~ 100 in packets */
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_EndPoint_Security_[] = {	/* Device.WiFi.EndPoint.{i}.Security. */
   /* Parameters */
   {	"ModesSupported",		(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	},
	/* None, WEP-64, WEP-128, WPA-Personal, WPA2-Personal, WPA-WPA2-Personal, WPA-Enterprise, WPA2-Enterprise, WPA-WPA2-Enterprise */
   {	NULL,				0,			{NULL}	}
};

dm_list_t Device_WiFi_EndPoint_Profile_Security_[] = {		/* Device.WiFi.EndPoint.{i}.Profile.Y.Security. */
   /* Parameters */
   {	"ModeEnabled",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* One of ModesSupported */
   {	"WEPKey",			(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{NULL}	}, /* HexBinary of length 5 or 13 */
   {	"PreSharedKey",			(ATTR_ACCS_RW | ATTR_TYPE_HEX),		{NULL}	}, /* HexBinary of max length 32 */
   {	"KeyPassphrase",		(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* Of length 8 ~ 63 */
   {	"MFPConfig",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* Disabled, Optional, Required */
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_EndPoint_Profile_[] = {	/* Device.WiFi.EndPoint.{i}.Profile.{i}. */
   /* Parameters */
   {	"Enable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* true or false */
   {	"Status",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* Disabled, Active, Available, Error(optional) */
   {	"Alias",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* */
   {	"SSID",				(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* max 256,  Device.WiFi.SSID.X. */
   {	"Location",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* max 256,  Device.WiFi.SSID.X. */
   {	"Priority",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0: highest, 0 ~ 255 */

   /* Intermediate nodes */
   {	"Security.",			(ATTR_ACCS_RW),				{Device_WiFi_EndPoint_Profile_Security_}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_EndPoint_WPS_[] = {	/* Device.WiFi.EndPoint.{i}.WPS. */
   /* Parameters */
   {	"Enable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* true(default) or false */
   {	"ConfigMethodsSupported",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	},
	/* USBFlashDrive, Ethernet, Label, Display, ExternalNFCToken, IntegratedNFCToken, NFCInterface,
	*  PushButton, PIN, PhysicalPushButton, PhysicalDisplay, VirtualPushButton, VirtualDisplay
	*/
   {	"ConfigMethodsEnabled",		(ATTR_ACCS_RW | ATTR_TYPE_LIST),	{NULL}	},
   {	"Status",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* Disabled, Error, Unconfigured, Configured */
   {	"Version",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* eg. "2.0" */
   {	"PIN",				(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* Hidden. Note spec is unsignedInt */
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_EndPoint_AC_Stats_[] = {	/* Device.WiFi.EndPoint.{i}.AC.{i}.Stats. */
   /* Parameters */
   {	"BytesSent",			(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"BytesReceived",		(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"PacketsSent",			(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"PacketsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_SC64),	{NULL}	},
   {	"ErrorsSent",			(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"ErrorsReceived",		(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"DiscardPacketsSent",		(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"DiscardPacketsReceived",	(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	},
   {	"RetransCount",			(ATTR_ACCS_RO | ATTR_TYPE_SC32),	{NULL}	}, /* In packets */
   {	"OutQLenHistogram",		(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_EndPoint_AC_[] = {	/* Device.WiFi.EndPoint.{i}.AC.{i}. */
   /* Parameters */
   {	"AccessCategory",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* BE, BK, VI, VO */
   {	"Alias",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* string(64) */
   {	"AIFSN",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 2 - 15 */
   {	"ECWMin",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0 - 15 in usec */
   {	"ECWMax",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0 - 15 in usec */
   {	"TxOpMax",			(ATTR_ACCS_RW | ATTR_TYPE_UINT),	{NULL}	}, /* 0 - 255, in 32usec unit */
   {	"AckPolicy",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* true: Ack */
   {	"OutQLenHistogramIntervals",	(ATTR_ACCS_RO | ATTR_TYPE_LIST),	{NULL}	}, /*  */
   {	"OutQLenHistogramSampleInterval", (ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /*  */

   /* Intermediate nodes */
   {	"Stats.",			(ATTR_ACCS_RO),				{Device_WiFi_EndPoint_AC_Stats_}	},
   {	NULL,				0,					{NULL}	}
};

dm_list_t Device_WiFi_EndPoint_[] = {
   /* Parameters */
   {	"Enable",			(ATTR_ACCS_RW | ATTR_TYPE_BOOL),	{NULL}	}, /* true or false */
   {	"Status",			(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* Disabled, Enabled, Error_Misconfigured, Error(optional) */
   {	"Alias",			(ATTR_ACCS_RW | ATTR_TYPE_STR),		{NULL}	}, /* string(64) */
   {	"ProfileReference",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* max 256,  Device.WiFi.EndPoint.Profile.X. */
   {	"SSIDReference",		(ATTR_ACCS_RO | ATTR_TYPE_STR),		{NULL}	}, /* max 256,  Device.WiFi.SSID.X. */
   {	"ProfileNumberOfEntries",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{NULL}	}, /* max 256,  Device.WiFi.EndPoint.Profile.X. */

   /* Intermediate nodes */
   {	"Stats.",			(ATTR_ACCS_RO),				{Device_WiFi_EndPoint_Stats_}	},
   {	"Security.",			(ATTR_ACCS_RO),				{Device_WiFi_EndPoint_Security_}	},
   {	"Profile.",			(ATTR_ACCS_RW | ATTR_ARRY),		{Device_WiFi_EndPoint_Profile_}	},
   {	"WPS.",				(ATTR_ACCS_RO),				{Device_WiFi_EndPoint_WPS_}	},
   {	"AC.",				(ATTR_ACCS_RO | ATTR_ARRY),		{Device_WiFi_EndPoint_AC_}	},
   {	NULL,				0,					{NULL}	}
};

/**************************
*  Device.WiFi. data model.
**************************/
dm_list_t Device_WiFi_[] = {			/* Device.WiFi. */
   /* Parameters */
   {	"RadioNumberOfEntries",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{wldm_RadioNumberOfEntries}	},
   {	"SSIDNumberOfEntries",		(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{wldm_SSIDNumberOfEntries}	},
   {	"AccessPointNumberOfEntries",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{wldm_AccessPointNumberOfEntries}	},
   {	"EndPointNumberOfEntries",	(ATTR_ACCS_RO | ATTR_TYPE_UINT),	{wldm_EndPointNumberOfEntries}	},

   /* Intermediate nodes */
   {	"Radio.",			(ATTR_ACCS_RO | ATTR_ARRY),		{Device_WiFi_Radio_}			},
   {	"NeighboringWiFiDiagnostic.",	(ATTR_ACCS_RO),				{Device_WiFi_NeighboringWiFiDiagnostic_}	},
   {	"SSID.",			(ATTR_ACCS_RW | ATTR_ARRY),		{Device_WiFi_SSID_}			},
   {	"AccessPoint.",			(ATTR_ACCS_RW | ATTR_ARRY),		{Device_WiFi_AccessPoint_}		},
   {	"EndPoint.",			(ATTR_ACCS_RW | ATTR_ARRY),		{Device_WiFi_EndPoint_}			},

   /* Proprietary */
   {	NULL,				0,					{NULL}	}
};

/* TR181 Device.WiFi. Root pathname */
#define STR_DEVICE_WIFI			"Device.WiFi."
#define STR_DEVICE_WIFI_BRIEF		".."

dm_list_t Tr181_WirelessDM[] = {	/* Device.WiFi. */
   {	STR_DEVICE_WIFI,		(ATTR_ACCS_RO),				{Device_WiFi_}	},
   {	STR_DEVICE_WIFI_BRIEF,		(ATTR_ACCS_RO),				{Device_WiFi_}	},
   {	NULL,				0,					{NULL}		}
};

/*******************
*  Helper functions.
*******************/
static int
find_dm_item(char *pathname, dm_list_t *dm_list)
{
	int i = 0, len, type;

	while (dm_list[i].name != NULL) {
		len = strlen(dm_list[i].name);
		type = dm_list[i].attribute & ATTR_TYPE_MASK;
		if (type && strcmp(pathname, dm_list[i].name) == 0) {
			/* Leaf found */
			return i;
		} else if (type == 0 && strncmp(pathname, dm_list[i].name, len) == 0) {
			/* Intermediate node found */
			return i;
		}
		i++;
	}
	return -1;
}

/*
*  Return >= 1 for specified valid instance, -1 for all, and 0 when invalid
*/
static int
ref_instance_set(ref_instance_t *refinstance, char *name, unsigned instance)
{
	/* No need to set Device.WiFi. */
	if (strcmp(name, STR_DEVICE_WIFI) == 0 || strcmp(name, STR_DEVICE_WIFI_BRIEF) == 0)
		return 1;

	/* Currently Radio, SSID, NeighboringWiFiDiagnostic.Result, AccessPoint, AC,
	*  EndPoint and Profile
	*/
	if (strcmp(name, "Radio.") == 0)
		refinstance->Radio = instance;
	else if (strcmp(name, "SSID.") == 0)
		refinstance->SSID = instance;
	else if (strcmp(name, "AccessPoint.") == 0)
		refinstance->AccessPoint = instance;
	else if (strcmp(name, "AC.") == 0)
		refinstance->AC = instance;
	else if (strcmp(name, "Result.") == 0)
		refinstance->NeighborDiagResult = instance;
	else if (strcmp(name, "AssociatedDevice.") == 0)
		refinstance->AssociatedDevice = instance;
	else if (strcmp(name, "EndPoint.") == 0)
		refinstance->EndPoint = instance;
	else if (strcmp(name, "Profile.") == 0)
		refinstance->Profile = instance;
	else {
		//WIFI_DBG("Instance %d specified for unknown %s!\n", instance, name);
		return 0;
	}
	return instance;
}

static int
ref_instance_get(ref_instance_t *refinstance, char *name)
{
	int instance = -1; /* ALL */

	/* Currently Radio, SSID, NeighboringWiFiDiagnostic.Result, AccessPoint, AC,
	*  EndPoint and Profile
	*/
	if (strcmp(name, STR_DEVICE_WIFI) == 0 || strcmp(name, STR_DEVICE_WIFI_BRIEF) == 0)
		instance = 1;
	else if (strcmp(name, "Radio.") == 0)
		instance = refinstance->Radio;
	else if (strcmp(name, "SSID.") == 0)
		instance = refinstance->SSID;
	else if (strcmp(name, "AccessPoint.") == 0)
		instance = refinstance->AccessPoint;
	else if (strcmp(name, "AC.") == 0)
		instance = refinstance->AC;
	else if (strcmp(name, "Result.") == 0)
		instance = refinstance->NeighborDiagResult;
	else if (strcmp(name, "AssociatedDevice.") == 0)
		instance = refinstance->AssociatedDevice;
	else if (strcmp(name, "EndPoint.") == 0)
		instance = refinstance->EndPoint;
	else if (strcmp(name, "Profile.") == 0)
		instance = refinstance->Profile;
	else {
		if (refinstance->Radio > 0)
			return refinstance->Radio;
		if (refinstance->SSID > 0)
			return refinstance->SSID;

		/* Array under AccessPoint.{i}., like AC.{i}. and AssociatedDevice,{i}. */
		if (refinstance->AssociatedDevice > 0)
			return refinstance->AssociatedDevice;
		if (refinstance->AC > 0)
			return refinstance->AC;
		if (refinstance->AccessPoint > 0)
			return refinstance->AccessPoint;

		WIFI_DBG("get instance for unknown %s!\n", name);
	}
	return instance;
}

static int
get_total_instances(ref_instance_t *refinstance, char *name)
{
	int total = 0;

	/* WiFi, Radio, SSID, NeighboringWiFiDiagnostic.Result, AccessPoint,
	*  AC, EndPoint and Profile
	*/
	if (strcmp(name, STR_DEVICE_WIFI) == 0 || strcmp(name, STR_DEVICE_WIFI_BRIEF) == 0)
		total = 1;
	else if (strcmp(name, "Radio.") == 0)
		total = refinstance->Radios;
	else if (strcmp(name, "SSID.") == 0)
		total = refinstance->SSIDs;
	else if (strcmp(name, "AccessPoint.") == 0)
		total = refinstance->APs;
	else if (strcmp(name, "AC.") == 0)
		total = refinstance->ACs;
	else if (strcmp(name, "Result.") == 0)
		total = refinstance->NeighborDiagResults;
	else if (strcmp(name, "AssociatedDevice.") == 0)
		total = refinstance->AssociatedDevices;
	else if (strcmp(name, "EndPoint.") == 0)
		total = refinstance->EPs;
	else if (strcmp(name, "Profile.") == 0)
		total = refinstance->Profiles;
	else
		WIFI_DBG("get total instance for unknown %s!\n", name);
	return total;
}

/*
*  Convert space separated string to '\n' separated output.
*  All chars between '=' and '\n' or EOL are considered value and copied as is.
*/
int
normalize_string(char *input, char *output, int outbufsz)
{
	int equal = 0;

	/* Skip beginning space or '\n' */
	while (*input && outbufsz) {
		if (*input == '=') {
			equal++;
		}
		if (equal == 0) {
			if (*input == ' ' || *input == '\n') {
				*output++ = '\n';
				while (*input == ' ' || *input == '\n')
					input++;
			}
			*output++ = *input;
		} else {
			/* Copy until '\n' or EOL */
			if (*input == '\n' || *input == '\0')
				equal--;
			*output++ = *input;
		}
		input++;
		outbufsz--;
	}
	if (outbufsz == 0) {
		WIFI_DBG("Buffer too short!\n");
		return -1;
	}
	*output = '\0';
	return 0;
}

static dm_list_t *
wldm_pathname_find(char *pathname, dm_list_t *dm_list, ref_instance_t *refinstance)
{
	int i, instance = 0;
	char *next_pathname;

	if (pathname == NULL || dm_list == NULL || refinstance == NULL)
		return NULL;

	i = find_dm_item(pathname, dm_list);
	if (i < 0) {
		WIFI_DBG("%s not found!\n", pathname);
		return NULL;
	}

	refinstance->dm_trace[++refinstance->target_index] = &dm_list[i];
	next_pathname = pathname + strlen(dm_list[i].name);
	if (dm_list[i].attribute & ATTR_ARRY) {
		/* Check if instance number is specified */
		if (*next_pathname == '\0') {
			/* Instance is not specified */
			instance = -1;	/* All */
		} else if (sscanf(next_pathname, "%d.", &instance) != 1) {
			WIFI_DBG("Expect instance number after %s!\n", dm_list[i].name);
		} else if (instance < 1) {
			/* Instance starts from 1 */
			WIFI_DBG("Invalid instance %d!\n", instance);
			return NULL;
		}
		if (ref_instance_set(refinstance, dm_list[i].name, instance) == 0) {
			return NULL;
		}
		if (instance > 0) {
			if ((next_pathname = strchr(next_pathname, '.')) == NULL) {
				WIFI_DBG("Expecting . after %d!\n", instance);
				return NULL;
			}
			next_pathname += 1; /* Skip "." */
		}
	}
	if (*next_pathname == '\0') {
		/* End of the pathname */
		return &dm_list[i];
	}

	if ((dm_list[i].attribute & ATTR_TYPE_MASK) == 0) {
		/* A node, which does not have TR181 defined data type */
		/* Continue to look up the next list */
		return wldm_pathname_find(next_pathname, dm_list[i].nextlist, refinstance);
	}

	return NULL;
}

static int
wkdm_instance_validate(dm_list_t *dm_list, ref_instance_t *refinstance)
{
	int len;
	unsigned int value;

	len = sizeof(value);
	if (strstr(refinstance->pathname, "Radio.")) {
		if (refinstance->Radio == 0) {
			/* No instance number is specified */
			return 0;
		}
		/* Check the radio instance */
		if (wldm_RadioNumberOfEntries(CMD_GET, 0, &value, &len, NULL, NULL) < 0) {
			WIFI_DBG("Failed to get RadioNumberOfEntries!\n");
			return -1;
		}
		refinstance->Radios = value;
		if (refinstance->Radio > (int)value) {
			WIFI_DBG("Radio.%d is out of range [1..%d]!\n",
				refinstance->Radio, refinstance->Radios);
			return -1;
		}
	} else if (strstr(refinstance->pathname, "SSID.")) {
		if (refinstance->SSID == 0) {
			/* No instance number is specified */
			return 0;
		}
		/* Check the SSID instance */
		if (wldm_SSIDNumberOfEntries(CMD_GET, 0, &value, &len, NULL, NULL) < 0) {
			WIFI_DBG("Failed to get SSIDNumberOfEntries!\n");
			return -1;
		}
		refinstance->SSIDs = value;
		if (((refinstance->command & (CMD_ADD | CMD_DEL)) == 0) &&
		    (refinstance->SSID > (int)value)) {
			WIFI_DBG("SSID.%d is out of range [1..%d]!\n",
				refinstance->SSID, value);
			return -1;
		}
	} else if (strstr(refinstance->pathname, "AccessPoint.")) {
		if (refinstance->AccessPoint == 0) {
			/* No instance number is specified */
			return 0;
		}
		/* Check the AccessPoint instance */
		if (wldm_AccessPointNumberOfEntries(CMD_GET, 0, &value, &len, NULL, NULL) < 0) {
			WIFI_DBG("Failed to get wldm_AccessPointNumberOfEntries!\n");
			return -1;
		}
		refinstance->APs = value;
		if (((refinstance->command & (CMD_ADD | CMD_DEL)) == 0) &&
		    (refinstance->AccessPoint > (int)value)) {
			WIFI_DBG("AccessPoint.%d is out of range [1..%d]!\n",
				refinstance->AccessPoint, value);
			return -1;
		}
		if (strstr(refinstance->pathname, "AssociatedDevice.")) {
			refinstance->AssociatedDevices = 2;
		} else if (strstr(refinstance->pathname, "AC.")) {
			if (refinstance->AC > 4)
				return -2;
		}
	} else if (strstr(refinstance->pathname, "EndPoint.")) {
		/* Check the AccessPoint instance */
		refinstance->EPs = 2;
	}
	return 0;
}

/*
*  Verify the Access and values of the parameters after the pathname.
*  Return: > 0 bytes consumed, < 0 if error.
*/
static int
wkdm_param_parse(dm_list_t *dm_list, ref_instance_t *refinstance, char *params)
{
	int len = 0, maxsz, tomatch;
	dm_list_t *target = refinstance->dm_trace[refinstance->target_index];
	char *param = params, *delimiter, *expectstr;
	char *name = &refinstance->param_name[0], *value = &refinstance->param_value[0];

	if (target->attribute & ATTR_TYPE_MASK) {
		if (refinstance->command == CMD_GET && param && *param != '\0' &&
		    (target->attribute & ATTR_TYPE_MASK) != ATTR_TYPE_PROP) {
			/* Unexpected parameter */
			name[0] = value[0] = '\0';
			WIFI_DBG("Unexpect parameter after %s!\n", refinstance->pathname);
			return -1;
		}

		/* Expect value */
		tomatch = '\n';
		expectstr = value;
		maxsz = sizeof(refinstance->param_value);
		strcpy(name, target->name);
	} else {
		if (refinstance->command & (CMD_GET | CMD_LIST)) {
			/* Get, expect name in param */
			tomatch = '\n';
		} else {
			/* Expect name=value in param */
			tomatch = '=';
		}
		expectstr = name;
		maxsz = sizeof(refinstance->param_name);
	}
	delimiter = strchr(param, tomatch);
	len = (delimiter == NULL) ? strlen(param) : (delimiter - param);
	if (delimiter == NULL && (tomatch == '=')) {
		refinstance->fault_code = TR069_INV_ARG;
		WIFI_DBG("Expect name=value after %s!\n", refinstance->pathname);
		return -1;
	}

	if ((len == 0 && (refinstance->command & CMD_SET)) || len >= maxsz) {
		refinstance->fault_code = TR069_INV_ARG;
		WIFI_DBG("Invalid Parameter %s length %d!\n",
			(expectstr == name) ? "name" : "value", len);
		return -1;
	}
	strncpy(expectstr, param, len);
	expectstr[len] = '\0';

	if (tomatch == '\n')
		return len + 1;

	if ((target->attribute & ATTR_TYPE_MASK) && refinstance->command == CMD_ADD) {
		refinstance->fault_code = TR069_INV_ARG;
		WIFI_DBG("Parameter %s cannot be added!\n", dm_list->name);
		return -2;
	}

	/* Expect value in param */
	param = delimiter + 1;
	delimiter = strchr(param, '\n');
	len = (delimiter == NULL) ? strlen(param) : (delimiter - param);
	if (len == 0 || len >= sizeof(refinstance->param_value)) {
		refinstance->fault_code = TR069_INV_ARG;
		WIFI_DBG("Invalid Parameter value length %d!\n", len);
		return -3;
	}
	strncpy(value, param, len);
	value[len] = '\0';
	return (param - params) + len + 1;
}

static int
get_param_value(dm_list_t *dm_list, ref_instance_t *refinstance, char *buf, int *plen)
{
	if (dm_list->attribute & ATTR_TYPE_BOOL) {
		*((int *)buf) = (strcmp(refinstance->param_value, "true") == 0 ||
			strcmp(refinstance->param_value, "1") == 0) ? 1 : 0;
		*plen = sizeof(int);
	} else if (dm_list->attribute & ATTR_TYPE_SI32) {
		sscanf(refinstance->param_value, "%d", (int *)buf);
		*plen = sizeof(int);
	} else if (dm_list->attribute & ATTR_TYPE_UI32) {
		sscanf(refinstance->param_value, "%u", (unsigned int *)buf);
		*plen = sizeof(unsigned int);
	} else if (dm_list->attribute & ATTR_TYPE_UI64) {
		sscanf(refinstance->param_value, "%" PRIu64, (uint64_t *)buf);
		*plen = sizeof(uint64_t);
	} else if (dm_list->attribute & ATTR_TYPE_STR) {
		int len = strlen(refinstance->param_value);

		if (len > *plen - 1) {
			*plen = 0;
			return -1;
		}
		strcpy(buf, refinstance->param_value);
		*plen = len;
	} else {
		WIFI_DBG("%s: unknown data type %d ###\n", __FUNCTION__, dm_list->attribute);
		return -1;
	}
	return 0;
}

static int
wldm_do_func(dm_list_t *dm_list, ref_instance_t *refinstance, int index,
	char *param_name, char *param_value)
{
	int ret, len;
	char buf[1024];

	if ((refinstance->command & (CMD_ADD | CMD_DEL)) ||
	    ((refinstance->command & CMD_SET) && (dm_list->attribute & ATTR_ACCS_MASK) ==
	    ATTR_ACCS_RO)) {
		/* Add/del is not allowed for a leaf; RO param cannot be set */
		WIFI_DBG("For %s(%s), command is not allowed!\n", dm_list->name,
			(dm_list->attribute & ATTR_ACCS_MASK) ? "RW" : "RO");
		return -1;
	}
	if (dm_list->wldm_func) {
		if (refinstance->command & CMD_SET) {
			/* Convert param_value into buf for CMD_SET */
			len = sizeof(buf);
			if (get_param_value(dm_list, refinstance, buf, &len) < 0)
				return -1;
		} else {
			len = sizeof(buf);
		}
		ret = dm_list->wldm_func(refinstance->command, index, buf, &len, NULL, NULL);
	} else {
		ret = default_function(dm_list, refinstance);
	}
	return ret;
}

/*
*  Walk through the list and call the functions. If nextlevel is not 0, walk the next list too.
*  If nextlevel > 0, walk only the number of next level it specifies. If < 0, no limit.
*/
static int
wldm_do_cmd_walk(dm_list_t *dm_list, ref_instance_t *refinstance, int nextlevel,
	int index, char *param_name, char *param_value)
{
	int ret = 0, total_count = 0, match_one = (param_name) ? 1 : 0;
	dm_list_t *list = dm_list;

	while (list->name) {
		if (match_one && strncmp(param_name, list->name, strlen(list->name)) != 0) {
			list++;
			continue;
		}

		if (list->attribute & ATTR_TYPE_MASK) {
			/* Call the function */
			ret = wldm_do_func(list, refinstance, index, param_name, param_value);
			if (ret >= 0)
				total_count++;
		} else if (nextlevel) {
			BPRINTF("\n%s:\n", list->name);
			total_count += wldm_do_cmd_walk(list->nextlist, refinstance,
				(nextlevel > 0) ? nextlevel - 1 : nextlevel, index, NULL, NULL);
		}
		if (match_one)
			break;
		list++;
	}
	if (match_one == 0)
		BPRINTF("\n");
	return total_count;
}

static void
wldm_print_header(dm_list_t *dm_list, ref_instance_t *refinstance, int instance)
{
	int i, j, is_node = ((dm_list->attribute & ATTR_TYPE_MASK) == 0);
	dm_list_t *target;

	/* Print the output header */
	j = (is_node) ? refinstance->target_index : refinstance->target_index - 1;
	for (i = 0; i <= j; i++) {
		target = refinstance->dm_trace[i];
		if (target->attribute & ATTR_ARRY)
			BPRINTF("%s%d.", target->name, instance);
		else
			BPRINTF("%s", target->name);
	}
	if (is_node)
		BPRINTF("\n");
}

#if !defined(WLDM_AUTO_APPLY_TIME_MS) || (WLDM_AUTO_APPLY_TIME_MS <= 0)
/* Do apply if auto apply timer is not enabled. */
static int
wldm_do_apply(dm_list_t *dm_list, ref_instance_t *refinstance)
{
	int ret = wldm_apply_all();

	return ret;
}
#endif /* WLDM_AUTO_APPLY_TIME_MS */

static int
wldm_do_cmd(dm_list_t *dm_list, ref_instance_t *refinstance)
{
	int t, ret, instance, total_instances, total_count = 0;
	char *params = refinstance->params;
	dm_list_t *list;

	/* Do list command */
	if (refinstance->command == CMD_LIST) {
		int nextlevel = 0;

		/* Print all the parameter names */
		if (params && strstr(params, "NextLevel=true"))
			nextlevel = -1; /* Walk through all next levels */

		BPRINTF("%s:\n", dm_list->name);
		total_count = wldm_do_cmd_walk(dm_list->nextlist, refinstance,
			nextlevel, 0, NULL, NULL);
		return total_count;
	}

	if (refinstance->command & CMD_GET)
		refinstance->command |= CMD_LIST; /* CMD_GET | CMD_LIST means to print */

	instance = ref_instance_get(refinstance, dm_list->name);
	if (dm_list->attribute & ATTR_TYPE_MASK) {
		/* It is a leaf, call the api function directly */
		if (refinstance->command & CMD_SET)
			ret = wkdm_param_parse(dm_list, refinstance, params);
		if (refinstance->command & CMD_GET) {
			if (*params != '\0') {
				WIFI_DBG("Excessive parameters:\n%s\n", params);
				return -1;
			}
			wldm_print_header(dm_list, refinstance, instance);
		}
		ret = wldm_do_func(dm_list, refinstance, instance - 1,
			dm_list->name, refinstance->param_value);
		return ret;
	}

	if (instance < 0) {
		/* The instance is not specified, walk through all instances */
		t = 1;
		total_instances = get_total_instances(refinstance, dm_list->name);
	} else {
		t = total_instances = instance;
	}
	list = dm_list->nextlist;
	for (; t <= total_instances; t++) {
		/* Set the instance for interation */
		ref_instance_set(refinstance, dm_list->name, t);

		/* Print the output header */
		if (refinstance->command & CMD_GET)
			wldm_print_header(dm_list, refinstance, t);

		/* The pathname denotes an Intermediate node(partial parameter name) */
		params = refinstance->params;
		if (params == NULL || *params == '\0') {
			/* No params are specified. Go through all nodes of that instance */
			total_count += wldm_do_cmd_walk(list, refinstance, 0, t - 1, NULL, NULL);
			continue;
		}

		while (*params) {
			/* Some parameters specified after the pathname */
			ret = wkdm_param_parse(dm_list, refinstance, params);
			if (ret < 0)
				return -1;
			params += ret;
			ret = wldm_do_cmd_walk(list, refinstance, 0, t - 1,
				refinstance->param_name, refinstance->param_value);
			if (ret < 0)
				break;
			total_count += ret;
		}
		BPRINTF("\n");
	}

	//WIFI_DBG("total_count=%d\n", total_count);
	return total_count;
}

static int
wldm_cmd(int command, char *inpbuf, char *outbuf, int outbuf_sz)
{
	ref_instance_t *refinstance;
	dm_list_t *found;
	int len, ret = -1;
	char *pathname;

	refinstance = malloc(sizeof(*refinstance));
	if (refinstance == NULL) {
		WIFI_DBG("Out of memory!\n");
		return ret;
	}
	memset(refinstance, 0, sizeof(*refinstance));

	refinstance->command = command;
	refinstance->output_buf = outbuf;
	refinstance->output_buf_size = outbuf_sz;
	refinstance->inpbuf = inpbuf;
	refinstance->target_index = -1;

	len = strlen(inpbuf);
	if (len >= sizeof(refinstance->input_buf)) {
		WIFI_DBG("Pathname length %d too long!\n", len);
		goto exit;
	}
	len = strcspn(inpbuf, " =\n");
	if (len == 0 || len >=  sizeof(refinstance->input_buf)) {
		WIFI_DBG("Unexpected Pathname length %d!\n", len);
		goto exit;
	}

	pathname = refinstance->pathname = &refinstance->input_buf[0];
	strncpy(pathname, inpbuf, len);
	refinstance->pathname[len] = '\0';
	refinstance->params = refinstance->pathname + len + 1; /* Skip '\0' */
	strcpy(refinstance->params, refinstance->inpbuf + len + 1);

	found = wldm_pathname_find(pathname, Tr181_WirelessDM, refinstance);
	if (found == NULL) {
		refinstance->fault_code = TR069_INV_PARAM_NAME;
		WIFI_DBG("Pathname %s not found!\n", pathname);
		goto exit;
	}

	if (wkdm_instance_validate(found, refinstance) < 0) {
		refinstance->fault_code = TR069_INV_PARAM_NAME;
		WIFI_DBG("Instance validation failed!\n");
		goto exit;
	}

	ret = wldm_do_cmd(found, refinstance);
	if (ret < 0)
		goto exit;

#if !defined(WLDM_AUTO_APPLY_TIME_MS) || (WLDM_AUTO_APPLY_TIME_MS <= 0)
	/* Do apply if auto apply timer is not enabled. */
	if (command & CMD_SET)
		ret = wldm_do_apply(found, refinstance);
#endif /* WLDM_AUTO_APPLY_TIME_MS */

exit:
	if (ret < 0)
		ret = refinstance->fault_code;
	else
		ret = 0;
	free(refinstance);
	return ret;
}

/********************
*  WL Data Model API:
********************/

/*
*  Add a new instance of the given pathname.
*  Return < 0 if invalid pathname, or number of items completed.
*
*  Example: wldm add Device.WiFi.AccessPoint.2. SSIDReference=Device.WiFi.SSID.3.
*  The '\n' is used to delimit the pathname and parameters.
*     char inpbuf[] =
*        "Device.WiFi.AccessPoint.2.\n"		//pathname
*        "SSIDReference=Device.WiFi.SSID.3.\n"	//param name
*/
int
wldm_add(char *inpbuf, char *outbuf, int outbuf_sz)
{
	if (inpbuf == NULL)
		return -1;

	return wldm_cmd(CMD_ADD, inpbuf, outbuf, outbuf_sz);
}

/*
*  Del the existing instance by pathname.
*  Return < 0 if invalid pathname.
*
*  Example: wldm del Device.WiFi.AccessPoint.2.
*  The '\n' is used to delimit the pathname and parameters.
*     char inpbuf[] =
*        "Device.WiFi.AccessPoint.2.\n"		//pathname
*/
int
wldm_del(char *inpbuf, char *outbuf, int outbuf_sz)
{
	if (inpbuf == NULL)
		return -1;

	return wldm_cmd(CMD_DEL, inpbuf, outbuf, outbuf_sz);
}

/*
*  Get the value(s) of the given pathname(cf. TR069 GetParameterValues).
*  Return TR069 fault code if invalid, or 0 if successful.
*
*  Example: wldm get Device.WiFi.Radio.1. Enable SupportedFrequencyBands OperatingStandards
*  The '\n' is used to delimit the pathname and parameters.
*     char inpbuf[] =
*        "Device.WiFi.Radio.1.\n"		//pathname
*        "Enable\n"				//param name
*        "SupportedFrequencyBands\n"
*        "OperatingStandards\n";
*
*     char outbuf[] will contain:
*        "Radio.1.\n"
*        "Enable=true\n"
*        "SupportedFrequencyBands=5GHz\n"
*        "OperatingStandards=ac\n";
*/
int
wldm_get(char *inpbuf, char *outbuf, int outbuf_sz)
{
	if (inpbuf == NULL)
		return -1;

	return wldm_cmd(CMD_GET, inpbuf, outbuf, outbuf_sz);
}

/*
*  List the parameter names of the given pathname(cf. TR069 GetParameterNames).
*  Return TR069 fault code if invalid, or 0 if successful.
*
*  Example: wldm list Device.WiFi.Radio.1. NextLevel=true
*  The '\n' is used to delimit the pathname and parameters.
*     char inpbuf[] =
*        "Device.WiFi.Radio.1.\n"		//param name
*        "NextLevel=true\n";
*
*     char outbuf[] will contain:
*        "Radio.1.\n"
*        "Enable\n"
*        "Status\n"
*	 ...;
*/
int
wldm_list(char *inpbuf, char *outbuf, int outbuf_sz)
{
	if (inpbuf == NULL)
		return -1;

	return wldm_cmd(CMD_LIST, inpbuf, outbuf, outbuf_sz);
}

/*
*  Set the parameter names/values of the given pathname(cf. TR069 SetParameterValues).
*  Return TR069 fault code if invalid, or 0 if all validated and applied.
*
*  Example:
*     wldm set Device.WiFi.Radio.1. Enable=true SupportedFrequencyBands=5GHz OperatingStandards=ac
*  The '\n' is used to delimit the pathname and parameters.
*     char parambuf[] =
*        "Device.WiFi.Radio.1.\n"		//pathname name
*        "Enable=true\n"			//param name=value
*        "Alias=Broadcom 5GHz Wifi\n"		//Treat all chars between '=' and '\n' as value.
*        "OperatingFrequencyBands=5GHz\n"
*        "OperatingStandards=ac\n";
*/
int
wldm_set(char *inpbuf, char *outbuf, int outbuf_sz)
{
	if (inpbuf == NULL)
		return -1;

	return wldm_cmd(CMD_SET, inpbuf, outbuf, outbuf_sz);
}

/* END */
