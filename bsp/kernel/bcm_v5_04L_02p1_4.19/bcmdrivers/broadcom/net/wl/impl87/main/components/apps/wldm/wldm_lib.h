/*
 * Broadcom Wifi Data Model Library
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
#ifndef _WLDM_LIB_H_
#define _WLDM_LIB_H_

#include "802.11.h"
#include "wifi_tr181.h"

#define WLDM_ACTION_WAIT_TIME_MS	0		/* ms */
#define WLDM_AUTO_APPLY_TIME_MS		0		/* ms */

#define NVRAM_NAME_SIZE			128
#define BUF_SIZE			256
#define STRING_LENGTH_128		128
#define STRING_LENGTH_64                64
#define STRING_LENGTH_32                32
#define STRING_LENGTH_16                16
#define STRING_LENGTH_8                 8

#define WLDM_DEBUG_ERROR		0x000001
#define WLDM_DEBUG_WARNING		0x000002
#define WLDM_DEBUG_INFO			0x000004
#define WLDM_DEBUG_DEBUG		0x000008

extern unsigned long wldm_msglevel;
extern int wldm_set_wldm_msglevel(unsigned long msglevel);
extern void wldm_log(const char *fmt, ...);

#define NVRAM_WLDM_MSGLEVEL		"wldm_msglevel"
#define WIFI_ERR(fmt, arg...) \
do { \
	if (wldm_msglevel & WLDM_DEBUG_ERROR) { \
		wldm_log("WLDM-ERR >> "fmt, ##arg); \
	} \
}	while (0)

#define WIFI_WARNING(fmt, arg...) \
do { \
	if (wldm_msglevel & WLDM_DEBUG_WARNING) { \
		wldm_log("WLDM >> "fmt, ##arg); \
	} \
}	while (0)

#define WIFI_INFO(fmt, arg...) \
do { \
	if (wldm_msglevel & WLDM_DEBUG_INFO) { \
		wldm_log("WLDM >> "fmt, ##arg); \
	} \
}	while (0)

#define WIFI_DBG(fmt, arg...) \
do { \
	if (wldm_msglevel & WLDM_DEBUG_DEBUG) { \
		wldm_log("WLDM-DBG >> "fmt, ##arg); \
	} \
}	while (0)

#define PRINT_BUF(buf, bufsz, format, ...) \
do { \
	int size; \
	if (buf && ((bufsz) > 1)) { \
		size = snprintf(buf, (bufsz) - 1, format, __VA_ARGS__); \
		buf[size] = '\0'; \
		bufsz -= size; \
	} else \
		printf(format, __VA_ARGS__); \
}	while (0)

#define IGNORE_CMD_WARNING(cmd, ignored) \
do { \
	if ((cmd) & (ignored)) \
		WIFI_DBG("%s: ignore cmd %x!\n", __FUNCTION__, (cmd) & (ignored)); \
}	while (0)

#define NVRAM_SET(nvram_name, buf) \
do { \
	if (wlcsm_nvram_set(nvram_name, buf) != 0) { \
		WIFI_DBG("%s, wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, \
			nvram_name, buf); \
		return -1; \
	} \
}	while (0)

#define NVRAM_STRING_SET(ifname, nv_name, string_value) \
do { \
	char nvram_name[NVRAM_NAME_SIZE]; \
	if (ifname) \
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", ifname, nv_name); \
	else \
		snprintf(nvram_name, sizeof(nvram_name), "%s", nv_name); \
	if (wlcsm_nvram_set(nvram_name, string_value) != 0) { \
		WIFI_DBG("%s, wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, \
			nvram_name, string_value); \
		return -1; \
	} \
} while (0)

#define NVRAM_INT_SET(ifname, nv_name, int_value) \
do { \
	char nvram_name[NVRAM_NAME_SIZE]; \
	char buf[BUF_SIZE]; \
	if (ifname) \
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", ifname, nv_name); \
	else \
		snprintf(nvram_name, sizeof(nvram_name), "%s", nv_name); \
	snprintf(buf, sizeof(buf), "%d", int_value); \
	if (wlcsm_nvram_set(nvram_name, buf) != 0) { \
		WIFI_DBG("%s, wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, \
			nvram_name, buf); \
		return -1; \
	} \
} while (0)

#define NVRAM_BOOL_SET(ifname, nv_name, bool_value) \
do { \
	char nvram_name[NVRAM_NAME_SIZE]; \
	char buf[BUF_SIZE]; \
	if (ifname) \
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", ifname, nv_name); \
	else \
		snprintf(nvram_name, sizeof(nvram_name), "%s", nv_name); \
	snprintf(buf, sizeof(buf), "%d", bool_value ? 1 : 0); \
	if (wlcsm_nvram_set(nvram_name, buf) != 0) { \
		WIFI_DBG("%s, wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, \
			nvram_name, buf); \
		return -1; \
	} \
} while (0)

/* len is buffer length before, becomes string length after */
#define NVRAM_STRING_GET(ifname, nv_name, buf, plen) \
do { \
	char nvram_name[NVRAM_NAME_SIZE]; \
	char *nvram_value; \
	if (*plen >= 1 ) { \
		if (ifname) \
			snprintf(nvram_name, sizeof(nvram_name), "%s_%s", ifname, nv_name); \
		else \
			snprintf(nvram_name, sizeof(nvram_name), "%s", nv_name); \
		nvram_value = nvram_safe_get(nvram_name); \
		*plen = MIN(*plen - 1, strlen(nvram_value)); \
		strncpy(buf, nvram_value, *plen); \
		*(char *)(buf + *plen) = 0; \
	} \
} while (0)

#define NVRAM_INT_GET(ifname, nv_name, int_value) \
do { \
	char nvram_name[NVRAM_NAME_SIZE]; \
	char *nvram_value; \
	if (ifname) \
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", ifname, nv_name); \
	else \
		snprintf(nvram_name, sizeof(nvram_name), "%s", nv_name); \
	nvram_value = nvram_safe_get(nvram_name); \
	*(int *)int_value = atoi(nvram_value); \
} while (0)

#define NVRAM_BOOL_GET(ifname, nv_name, bool_value) \
do { \
	char nvram_name[NVRAM_NAME_SIZE]; \
	char *nvram_value; \
	if (ifname) \
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", ifname, nv_name); \
	else \
		snprintf(nvram_name, sizeof(nvram_name), "%s", nv_name); \
	nvram_value = nvram_safe_get(nvram_name); \
	*(boolean *)bool_value = atoi(nvram_value); \
} while (0)

#define IOVAR_INT_SET(osifname, iovar, int_value) \
do { \
	if (wl_iovar_setint(osifname, iovar, int_value) < 0) { \
		WIFI_DBG("%s: wl_iovar_setint %s=%d failed!\n", __FUNCTION__, iovar, int_value); \
		return -1; \
	} \
} while (0)

#define IOVAR_INT_GET(osifname, iovar, int_value) \
do { \
	if (wl_iovar_getint(osifname, iovar, int_value) < 0) { \
		WIFI_DBG("%s: wl_iovar_getint %s failed!\n", __FUNCTION__, iovar); \
		return -1; \
	} \
} while (0)

#define ARRAY_SIZE(arry)             (sizeof(arry)/sizeof(arry[0]))

/* For feat_bca_wlan, WL_MAX_NUM_RADIOS is 3, WL_MAX_NUM_MBSSID is 8 */
#ifdef WLAN_BCG_CM_LINUX
#ifndef MAX_WLAN_ADAPTER
#define MAX_WLAN_ADAPTER		WL_MAX_NUM_RADIOS
#endif /* MAX_WLAN_ADAPTER */
#define WL_MAX_NUM_SSID			WL_MAX_NUM_MBSSID
#endif /* WLAN_BCG_CM_LINUX */

/*
*  Below objects are created to assist set/apply functions.
*  For each RW parameter of an object, the associated MASK is to mark whether the parameter is
*  set previously or not.
*  If yes, and validation is correct, set the apply_map; otherwise set the reject_map.
*/

/* Device_WiFi has only one RW parameter. */

/* Radio object */
typedef struct _Radio_Object {
	unsigned int			apply_map;		/* bit map to be applied */
	unsigned int			reject_map;		/* bit map to indicate failure */
	Device_WiFi_Radio		Radio;
}	Radio_Object;

#define Radio_Enable_MASK					(1 << 0)
#define Radio_Alias_MASK					(1 << 1)
#define Radio_LowerLayers_MASK					(1 << 2)
#define Radio_OperatingFrequencyBand_MASK			(1 << 3)
#define Radio_OperatingStandards_MASK				(1 << 4)
#define Radio_Channel_MASK					(1 << 5)
#define Radio_AutoChannelEnable_MASK				(1 << 6)
#define Radio_AutoChannelRefreshPeriod_MASK			(1 << 7)
#define Radio_OperatingChannelBandwidth_MASK			(1 << 8)
#define Radio_ExtensionChannel_MASK				(1 << 9)
#define Radio_GuardInterval_MASK				(1 << 10)
#define Radio_MCS_MASK						(1 << 11)
#define Radio_TransmitPower_MASK				(1 << 12)
#define Radio_IEEE80211hEnabled_MASK				(1 << 13)
#define Radio_RegulatoryDomain_MASK				(1 << 14)
#define Radio_RetryLimit_MASK					(1 << 15)
#define Radio_CCARequest_MASK					(1 << 16)
#define Radio_RPIHistogramRequest_MASK				(1 << 17)
#define Radio_FragmentationThreshold_MASK			(1 << 18)
#define Radio_RTSThreshold_MASK					(1 << 19)
#define Radio_LongRetryLimit_MASK				(1 << 20)
#define Radio_BeaconPeriod_MASK					(1 << 21)
#define Radio_DTIMPeriod_MASK					(1 << 22)
#define Radio_PacketAggregationEnable_MASK			(1 << 23)
#define Radio_PreambleType_MASK					(1 << 24)
#define Radio_BasicDataTransmitRates_MASK			(1 << 25)
#define Radio_OperationalDataTransmitRates_MASK			(1 << 26)
#define Radio_OBJ_MASKS						((1 << 27) - 1)

typedef struct _X_BROADCOM_COM_Radio_Object {
	unsigned int				apply_map;	/* bit map to be applied */
	unsigned int				reject_map;	/* bit map to indicate failure */
	Device_WiFi_X_BROADCOM_COM_Radio	X_BROADCOM_COM_Radio;
}	X_BROADCOM_COM_Radio_Object;
#define X_BROADCOM_COM_Radio_AxEnable_MASK			(1 << 0)
#define X_BROADCOM_COM_Radio_AxFeatures_MASK			(1 << 1)
#define X_BROADCOM_COM_Radio_AxBsscolor_MASK			(1 << 2)
#define X_BROADCOM_COM_Radio_AxMuType_MASK			(1 << 3)
#define X_BROADCOM_COM_Radio_STBCEnable_MASK			(1 << 4)
#define X_BROADCOM_COM_Radio_TxChainMask_MASK			(1 << 5)
#define X_BROADCOM_COM_Radio_RxChainMask_MASK			(1 << 6)
#define X_BROADCOM_COM_Radio_Greenfield11nEnable_MASK		(1 << 7)
#define X_BROADCOM_COM_Radio_CtsProtectionEnable_MASK		(1 << 8)
#define X_BROADCOM_COM_Radio_ResetCount_MASK			(1 << 9)
#define X_BROADCOM_COM_Radio_AxMuEdca_MASK			(1 << 10)
#define X_BROADCOM_COM_Radio_OBJ_MASKS				((1 << 11) - 1)

typedef struct _X_RDK_Radio_Object {
	unsigned int				apply_map;      /* bit map to be applied */
	unsigned int				reject_map;     /* bit map to indicate failure */
	Device_WiFi_X_RDK_Radio			X_RDK_Radio;
}	X_RDK_Radio_Object;
#define X_RDK_Radio_AmsduEnable_MASK			(1<<0)
#define X_RDK_Radio_AutoChannelDwellTime_MASK		(1<<1)
#define X_RDK_Radio_DfsEnable_MASK			(1<<2)
#define X_RDK_Radio_OBJ_MASKS				((1<<3) - 1)

/* SSID object */
typedef struct _SSID_Object {
	unsigned int			apply_map;		/* bit map to be applied */
	unsigned int			reject_map;		/* bit map to indicate failure */
	Device_WiFi_SSID		Ssid;
}	SSID_Object;

#define SSID_Enable_MASK					(1 << 0)
#define SSID_Alias_MASK						(1 << 1)
#define SSID_LowerLayers_MASK					(1 << 2)
#define SSID_SSID_MASK						(1 << 3)
#define SSID_OBJ_MASKS						((1 << 4) - 1)

/* SSID Supported Rates Bitmap Control features object */
typedef struct _X_LGI_Rates_Bitmap_Control_Object {
	unsigned int apply_map;
	unsigned int reject_map;
	Device_WiFi_AccessPoint_X_LGI_WiFiSupportedRates	Bitmap;
}	X_LGI_Rates_Bitmap_Control_Object;
#define X_LGI_RATE_CONTROL_Enable_MASK				(1 << 0)
#define X_LGI_RATE_CONTROL_BasicRate_MASK			(1 << 1)
#define X_LGI_RATE_CONTROL_SupportRate_MASK			(1 << 2)
#define X_LGI_RATE_CONTROL_OBJ_MASKS				((1 << 3) - 1)

/* Access Point object */
typedef struct _AccessPoint_Object {
	unsigned int			apply_map;		/* bit map to be applied */
	unsigned int			reject_map;		/* bit map to indicate failure */
	Device_WiFi_AccessPoint		Ap;
}	AccessPoint_Object;

#define AccessPoint_Enable_MASK					(1 << 0)
#define AccessPoint_Alias_MASK					(1 << 1)
#define AccessPoint_SSIDReference_MASK				(1 << 2)
#define AccessPoint_SSIDAdvertisementEnabled_MASK		(1 << 3)
#define AccessPoint_RetryLimit_MASK				(1 << 4)
#define AccessPoint_WMMEnable_MASK				(1 << 5)
#define AccessPoint_UAPSDEnable_MASK				(1 << 6)
#define AccessPoint_MaxAssociatedDevices_MASK			(1 << 7)
#define AccessPoint_IsolationEnable_MASK			(1 << 8)
#define AccessPoint_MACAddressControlEnabled_MASK		(1 << 9)
#define AccessPoint_AllowedMACAddress_MASK			(1 << 10)
#define AccessPoint_MaxAllowedAssociations_MASK			(1 << 11)
#define AccessPoint_OBJ_MASKS					((1 << 12) - 1)

typedef struct _X_RDK_AccessPoint_Object {
	unsigned int			apply_map;              /* bit map to be applied */
	unsigned int			reject_map;             /* bit map to indicate failure */
	Device_WiFi_X_RDK_AccessPoint	Ap;
}	X_RDK_AccessPoint_Object;

#define X_RDK_AccessPoint_MACAddressControMode_MASK		(1 << 0)
#define X_RDK_AccessPoint_MACAddresslist_MASK			(1 << 1)
#define X_RDK_AccessPoint_OBJ_MASKS				((1 << 2) - 1)

typedef struct _X_BROADCOM_COM_AccessPoint_Object {
	unsigned int			apply_map;              /* bit map to be applied */
	unsigned int			reject_map;             /* bit map to indicate failure */
	Device_WiFi_X_BROADCOM_COM_AccessPoint	Ap;
}	X_BROADCOM_COM_AccessPoint_Object;

#define X_BROADCOM_COM_AccessPoint_RMCapabilities_MASK		(1 << 0)
#define X_BROADCOM_COM_AccessPoint_OBJ_MASKS			((1 << 1) - 1)

/* Access Point Security object */
typedef struct _AccessPoint_Security_Object {
	unsigned int			apply_map;		/* bit map to be applied */
	unsigned int			reject_map;		/* bit map to indicate failure */
	Device_WiFi_AccessPoint_Security	Security;
}	AccessPoint_Security_Object;

#define AccessPoint_Security_ModeEnabled_MASK			(1 << 0)
#define AccessPoint_Security_WEPKey_MASK			(1 << 1)
#define AccessPoint_Security_PreSharedKey_MASK			(1 << 2)
#define AccessPoint_Security_KeyPassphrase_MASK			(1 << 3)
#define AccessPoint_Security_RekeyingInterval_MASK		(1 << 4)
#define AccessPoint_Security_RadiusServerIPAddr_MASK		(1 << 5)
#define AccessPoint_Security_SecondaryRadiusServerIPAddr_MASK	(1 << 6)
#define AccessPoint_Security_RadiusServerPort_MASK		(1 << 7)
#define AccessPoint_Security_SecondaryRadiusServerPort_MASK	(1 << 8)
#define AccessPoint_Security_RadiusSecret_MASK			(1 << 9)
#define AccessPoint_Security_SecondaryRadiusSecret_MASK		(1 << 10)
#define AccessPoint_Security_MFPConfig_MASK			(1 << 11)
#define AccessPoint_Security_Reset_MASK				(1 << 12)
#define AccessPoint_Security_OBJ_MASKS				((1 << 13) - 1)

/* Access Point Security X_COMCAST_COM_RadiusSettings object */
typedef struct _AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object {
	Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings	RadiusSettings;
}	AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object;

/* Access Point Security object */
typedef struct _X_RDK_AccessPoint_Security_Object {
	unsigned int				apply_map;      /* bit map to be applied */
	unsigned int				reject_map;     /* bit map to indicate failure */
	Device_WiFi_AccessPoint_X_RDK_Security	Security;
}	X_RDK_AccessPoint_Security_Object;

#define X_RDK_AccessPoint_Security_BasicAuthmode_MASK		(1 << 0)
#define X_RDK_AccessPoint_Security_Encryption_MASK		(1 << 1)
#define X_RDK_AccessPoint_Security_AuthMode_MASK		(1 << 2)
#define X_RDK_AccessPoint_Security_RadiusReAuthInterval_MASK	(1 << 3)
#define X_RDK_AccessPoint_Security_RadiusOperatorName_MASK	(1 << 4)
#define X_RDK_AccessPoint_Security_RadiusLocationData_MASK	(1 << 5)
#define X_RDK_AccessPoint_Security_RadiusGreylist_MASK		(1 << 6)
#define X_RDK_AccessPoint_Security_RadiusDASPort_MASK		(1 << 7)
#define X_RDK_AccessPoint_Security_RadiusDASClientIPAddr_MASK	(1 << 8)
#define X_RDK_AccessPoint_Security_RadiusDASSecret_MASK		(1 << 9)
#define X_RDK_AccessPoint_Security_WPAPairwiseRetries_MASK	(1 << 10)
#define X_RDK_AccessPoint_Security_WPAPMKLifetime_MASK		(1 << 11)
#define X_RDK_AccessPoint_Security_WPA3TransitionDisable_MASK	(1 << 12)
#define X_RDK_AccessPoint_Security_OBJ_MASKS			((1 << 13) - 1)

/* Access Point WPS object */
typedef struct _AccessPoint_WPS_Object {
	unsigned int			apply_map;		/* bit map to be applied */
	unsigned int			reject_map;		/* bit map to indicate failure */
	Device_WiFi_AccessPoint_WPS	Wps;
}	AccessPoint_WPS_Object;

#define AccessPoint_WPS_Enable_MASK				(1 << 0)
#define AccessPoint_WPS_ConfigMethodsEnabled_MASK		(1 << 1)
#define AccessPoint_WPS_PIN_MASK				(1 << 2)
#define AccessPoint_WPS_OBJ_MASKS				((1 << 3) - 1)

/* Access Point AC Object */
typedef struct _AccessPoint_AC_Object {
	unsigned int			apply_map;		/* bit map to be applied */
	unsigned int			reject_map;		/* bit map to indicate failure */
	Device_WiFi_AccessPoint_AC	Ac;
}	AccessPoint_AC_Object;

#define AccessPoint_AC_Alias_MASK				(1 << 0)
#define AccessPoint_AC_AIFSN_MASK				(1 << 1)
#define AccessPoint_AC_ECWMin_MASK				(1 << 2)
#define AccessPoint_AC_ECWMax_MASK				(1 << 3)
#define AccessPoint_AC_TxOpMax_MASK				(1 << 4)
#define AccessPoint_AC_AckPolicy_MASK				(1 << 5)
#define AccessPoint_AC_OutQLenHistogramIntervals_MASK		(1 << 6)
#define AccessPoint_AC_OutQLenHistogramSampleInterval_MASK	(1 << 7)
#define AccessPoint_AC_OBJ_MASKS				((1 << 8) - 1)

/* Access Point Accounting object */
typedef struct _AccessPoint_Accounting_Object {
	unsigned int				apply_map;	/* bit map to be applied */
	unsigned int				reject_map;	/* bit map to indicate failure */
	Device_WiFi_AccessPoint_Accounting	Accounting;
}	AccessPoint_Accounting_Object;

#define AccessPoint_Accounting_Enable_MASK			(1 << 0)
#define AccessPoint_Accounting_ServerIPAddr_MASK		(1 << 1)
#define AccessPoint_Accounting_SecondaryServerIPAddr_MASK	(1 << 2)
#define AccessPoint_Accounting_ServerPort_MASK			(1 << 3)
#define AccessPoint_Accounting_SecondaryServerPort_MASK		(1 << 4)
#define AccessPoint_Accounting_Secret_MASK			(1 << 5)
#define AccessPoint_Accounting_SecondarySecret_MASK		(1 << 6)
#define AccessPoint_Accounting_InterimInterval_MASK		(1 << 7)
#define AccessPoint_Accounting_OBJ_MASKS			((1 << 8) - 1)

bool hapd_disabled();
#define HAPD_DISABLED() hapd_disabled()

/* Init to configure number of radios to manage */
extern int wldm_init(int radios);

/* Deinit to cleanup */
extern int wldm_deinit(void);

/* Return the number of radios */
extern int wldm_get_radios(void);
extern int wldm_get_max_aps(void);

/* Return the nvram interface name of the given apIndex(ssidIndex) */
extern char *wldm_get_nvifname(int apIndex);

/* Return the os interface name of the given apIndex(ssidIndex) */
extern char *wldm_get_osifname(int apIndex);

/* Return the nvram interface name of the given radioIndex */
extern char *wldm_get_radio_nvifname(int radioIndex);

/* Return the os interface name of the given radioIndex */
extern char *wldm_get_radio_osifname(int radioIndex);

/* Return the radio index of the given apIndex(ssidIndex) */
extern int wldm_get_radioIndex(int apIndex);

/* Return the bss index of the given apIndex(ssidIndex) */
extern int wldm_get_bssidx(int apIndex);

/* Return the ap index of the given os interface name */
extern int wldm_get_apindex(char *osifname);

/* Apply all changes previously cached for this radio. Objects will be freed automatically. */
extern int wldm_apply(int radioIndex, int acts_to_defer);

/* Apply all changes previously cached. Objects will be freed after applying automatically. */
extern int wldm_apply_all(void);

/* Free the allocated objects w/o applying. */
extern int wldm_free_all(void);

/* Object release function for all wldm_get_XxxObject() */
extern void wldm_rel_Object(void *pObj, bool start_auto_apply_timer);

/* Stop/start wireless security related daemons(nas/wps or hostapd, and bsd) */
extern int wldm_stop_wsec_daemons(int radioIndex);
extern int wldm_start_wsec_daemons(int radioIndex);
extern int wldm_restart_wsec_daemons(int radioIndex);

/* Radio object, index starts from 0. */
extern int wldm_free_RadioObject(int radioIndex);
extern Radio_Object *wldm_get_RadioObject(int radioIndex, int checkMask);
extern X_BROADCOM_COM_Radio_Object *wldm_get_X_BROADCOM_COM_RadioObject(int radioindex, int checkMask);
extern X_RDK_Radio_Object *wldm_get_X_RDK_RadioObject(int radioIndex, int checkMask);
extern int wldm_apply_RadioObject(int radioIndex);

/* SSID object: index starts from 0. */
extern int wldm_free_SSIDObject(int ssidIndex);
extern SSID_Object *wldm_get_SSIDObject(int ssidIndex, int checkMask);
extern int wldm_apply_SSIDObject(int ssidIndex);

/* X_LGI_Rates_Bitmap_Control_Object */
extern int wldm_free_X_LGI_RatesControlObject(int ssidIndex);
extern X_LGI_Rates_Bitmap_Control_Object *wldm_get_X_LGI_RatesControlObject(int ssidIndex, int checkMask);
extern int wldm_apply_X_LGI_RatesControlObject(int ssidIndex);

/* AccessPoint object: index starts from 0. */
extern int wldm_free_AccessPointObject(int apIndex);
extern AccessPoint_Object *wldm_get_AccessPointObject(int apIndex, int checkMask);
extern X_RDK_AccessPoint_Object *wldm_get_X_RDK_AccessPointObject(int apIndex, int checkMask);
extern X_BROADCOM_COM_AccessPoint_Object *wldm_get_X_BROADCOM_COM_AccessPointObject(int apIndex, int checkMask);
extern int wldm_apply_AccessPointObject(int apIndex);

/* AccessPoint_Security object: index starts from 0. */
extern int wldm_free_AccessPointSecurityObject(int apIndex);
extern AccessPoint_Security_Object *wldm_get_AccessPointSecurityObject(int apIndex, int checkMask);
extern X_RDK_AccessPoint_Security_Object *wldm_get_X_RDK_AccessPointSecurityObject(int apIndex, int checkMask);
extern int wldm_apply_AccessPointSecurityObject(int apIndex);

/* AccessPoint_Security_X_COMCAST_COM_RadiusSettings object: index starts from 0. */
extern int wldm_free_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(int apIndex);
extern AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object *wldm_get_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(int apIndex);
extern int wldm_apply_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(int apIndex);

/* AccessPoint_WPS object: index starts from 0. */
extern int wldm_free_AccessPointWPSObject(int apIndex);
extern AccessPoint_WPS_Object *wldm_get_AccessPointWPSObject(int apIndex, int checkMask);
extern int wldm_apply_AccessPointWPSObject(int apIndex);

/* AccessPoint_AC object: index starts from 0. */
extern int wldm_free_AccessPointACObject(int apIndex, int acIndex);
extern AccessPoint_AC_Object *wldm_get_AccessPointACObject(int apIndex, int acIndex, int checkMask);
extern int wldm_apply_AccessPointACObject(int apIndex, int acIndex);

/* AccessPoint_Accounting object: index starts from 0. */
extern int wldm_free_AccessPointAccountingObject(int apIndex);
extern AccessPoint_Accounting_Object *wldm_get_AccessPointAccountingObject(int apIndex, int checkMask);
extern int wldm_apply_AccessPointAccountingObject(int apIndex);

/* WL Data Model Parameter APIs */
#include "wldm_lib_wifi.h"

#endif /* _WLDM_LIB_H_ */
