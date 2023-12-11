/*
 * ECBD shared include file
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
 * All Rights Reserved.
 *
 * $Id: ecbd.h $
 */
#ifndef __ECBD_H__
#define __ECBD_H__

#define ECBD_DEBUG_ERROR	0x000001
#define ECBD_DEBUG_WARNING	0x000002
#define ECBD_DEBUG_INFO		0x000004
#define ECBD_DEBUG_DEBUG	0x000008
#define ECBD_DEBUG_PROBE	0x000010
#define ECBD_DEBUG_DUMP		0x001000

#define ECBD_OUTPUT_FILE_INTERVAL 500000
#define ECBD_OUTPUT_FILE_TIMEOUT 5000000

#define NVRAM_ECBD_ENABLE "ecbd_enable"
#define NVRAM_ECBD_MSGLEVEL "ecbd_msglevel"

#define BCM_EVENT_HEADER_LEN		(sizeof(bcm_event_t))
#define MAX_EVENT_BUFFER_LEN		1400
#define MAX_LINE_LEN			16
#define MAX_IOCTL_BUFLEN		2048
#define MAX_STEERING_GROUP_NUM		8

/* sync with wifi_hal */
#define HAL_RADIO_NUM_RADIOS		MAX_WLAN_ADAPTER
#define HAL_AP_NUM_APS_PER_RADIO	8
#define HAL_WIFI_TOTAL_NO_OF_APS	HAL_RADIO_NUM_RADIOS * HAL_AP_NUM_APS_PER_RADIO
#define HAL_AP_IDX_TO_HAL_RADIO(apIdx)  ((apIdx < (2 * HAL_AP_NUM_APS_PER_RADIO)) ? \
	(apIdx % 2) : (apIdx / HAL_AP_NUM_APS_PER_RADIO))
#define HAL_AP_IDX_TO_SSID_IDX(apIdx)  ((apIdx < (2 * HAL_AP_NUM_APS_PER_RADIO)) ? \
	(apIdx / 2) : (apIdx % HAL_AP_NUM_APS_PER_RADIO))
#define WL_DRIVER_TO_AP_IDX(idx, subidx) ((idx < 2) ? (idx + subidx * 2 + 1) : \
	((idx * HAL_AP_NUM_APS_PER_RADIO) + subidx + 1))

#define MAX_PROBEREQ_EVENT_INTERVAL	5
#define MACLIST_TIMEOUT			15

#define ECBD_PRINT_ERROR(fmt, arg...) \
		do { if (ecbd_msglevel & ECBD_DEBUG_ERROR) \
			printf("ECBD >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); \
			fflush(stdout);} while (0)

#define ECBD_PRINT_WARNING(fmt, arg...) \
		do { if (ecbd_msglevel & ECBD_DEBUG_WARNING) \
			printf("ECBD >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); \
			fflush(stdout);} while (0)

#define ECBD_PRINT_INFO(fmt, arg...) \
		do { if (ecbd_msglevel & ECBD_DEBUG_INFO) \
			printf("ECBD >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); \
			fflush(stdout);} while (0)

#define ECBD_PRINT_DEBUG(fmt, arg...) \
		do { if (ecbd_msglevel & ECBD_DEBUG_DEBUG) \
			printf("ECBD >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); \
			fflush(stdout);} while (0)

#define ECBD_PRINT_PROBE(fmt, arg...) \
		do { if (ecbd_msglevel & ECBD_DEBUG_PROBE) \
			printf("ECBD >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); \
			fflush(stdout);} while (0)

#define ECBD_PRINT_DUMP(title, buf, len) \
		do { if (ecbd_msglevel & ECBD_DEBUG_DUMP) \
			ecbd_hexdump_ascii(title, buf, len); } while (0)

#define ECBD_SUCCESS	0
#define ECBD_FAIL -1

#define ECBD_VERSION 1
#define ECBD_DEFAULT_POLL_INTERVAL 1

#define	TIMEOUT_SIGNAL (SIGRTMIN)

#define MAC_TO_MACF(addr)	addr[0], \
							addr[1], \
							addr[2], \
							addr[3], \
							addr[4], \
							addr[5]

#define ECBD_MACF	"%02x:%02x:%02x:%02x:%02x:%02x"

#ifndef ETHER_ISBCAST
#define ETHER_ISBCAST(ea)	((((const uint8 *)(ea))[0] &          \
							((const uint8 *)(ea))[1] &            \
							((const uint8 *)(ea))[2] &            \
							((const uint8 *)(ea))[3] &            \
							((const uint8 *)(ea))[4] &            \
							((const uint8 *)(ea))[5]) == 0xff)
#endif /* ETHER_ISBCAST */

typedef struct {
	uint rssiProbeHWM;           /**< Probe response RSSI high water mark    */
	uint rssiProbeLWM;           /**< Probe response RSSI low water mark     */
	uint rssiAuthHWM;            /**< Auth response RSSI high water mark     */
	uint rssiAuthLWM;            /**< Auth response RSSI low water mark      */
	uint rssiInactXing;          /**< Inactive RSSI crossing threshold       */
	uint rssiHighXing;           /**< High RSSI crossing threshold           */
	uint rssiLowXing;            /**< Low RSSI crossing threshold            */
	uint authRejectReason;       /**< Inactive RSSI crossing threshold       */
} wifi_steering_clientConfig_t;

typedef enum sta_rssi_type {
	STA_RSSI_PROBE = 1,
	STA_RSSI_AUTH,
	STA_RSSI_INACTIVE,
	STA_RSSI_XING,
	STA_RSSI_MAX_TYPE
} sta_rssi_type_t;

/* use one sta table for different type lists:
 * ASSOC:     the STA is associated
 * ClientSet: the STA is configured by wifi_hal API ClientSet
 * MACLIST:   the STA is added to mac filter for probe/auth response control
 */

#define STA_TYPE_ASSOC      0x01
#define STA_TYPE_CLIENT_SET 0x02
#define STA_TYPE_MACLIST    0x04

#define ECBD_SENT_E_DISASSOC	(1 << 0)
#define ECBD_SENT_E_ASSOC	(1 << 1)

typedef struct ecbd_stalist {
	struct ether_addr addr;
	time_t assoc_time;		/* Assoc Timestamp */
	time_t disassoc_time;	/* Disassoc Timestamp */
	time_t active;			/* activity timestamp */
	time_t probe_time;		/* reduce event flooding */
	time_t maclist_time;	/* maclist filter timestamp */
	uint8  ifidx;			/* if index */
	uint8  bsscfgidx;		/* bsscfg index */
	char   ssid[MAX_SSID_LEN + 1];
	char   ifname[BCM_MSG_IFNAME_MAX];
	uint8  inactive_state;	/* based on traffic */
	uint8  band;
	int32  rssi;			/* per antenna rssi */
	int32  rssi_change;		/* crossing state */
	int32  rssi_change_auth;	/* crossing state */
	int32  rssi_change_assoc;	/* crossing state */
	uint32 phyrate;			/* unit Mbps */
	uint8  security;        /* open, WEP or WPA/WPA2 */
	uint8  type;            /* the mac is added by API (clientSet) or assoc */
	uint32 flags;			/* sta flags from wl */
	uint32 state;			/* Connected, disconnected etc */
	uint32 event_sent;		/* avoid duplicates */
	uint32 retrycount;
	uint32 maxrxrate;		/* Max rx Rate */
	uint32 maxtxrate;		/* Max tx Rate */
	uint32 rx_tot_pkts;
	uint32 tx_tot_pkts;
	wifi_steering_clientConfig_t *cli_cfg;
	dot11_rrm_cap_ie_t rm_cap;	/* RMCapabilities type = 0x46 (70) */
	int32  real_rssi;			/* the previous one "rssi" is used as snr */
	chanspec_t chanspec;
	uint16 aid;					/* association ID */
	struct ecbd_stalist *next;
} ecbd_stalist_t;

typedef enum sta_state {
	ACTIVE,
	INACTIVE,
	MAX_STATE
} sta_state_e;

/* support DPP (Device Provisioning Protocol) */

/* WiFi DPP Public Action Frame for DPP Auth protocol */
struct wifi_dpp_pub_act_frame {
	uint8   category;       /* PUB_AF_CATEGORY */
	uint8   action;         /* DPP_PUB_AF_ACTION */
	uint8   oui[3];         /* DPP_OUI */
	uint8   oui_type;       /* OUI type - DPP_VER */
	uint8   crypto_suite;   /* OUI subtype - DPP_TYPE_* */
	uint8   dpp_frame_type; /* nonzero, identifies req/rsp transaction */
	uint8   attributes[1];  /* A series of one or more DPP Attributes */
} __attribute__ ((packed));
typedef struct wifi_dpp_pub_act_frame wifi_dpp_pub_act_frame_t;

#define PUB_AF_CATEGORY         0x04
#define DPP_PUB_AF_ACTION       0x09
#define DPP_PUB_AF_ACTION_OUI_TYPE 0x1A
#define DPP_PUB_AF_FIXED_LEN    8

#ifndef WFA_OUI
#define WFA_OUI			"\x50\x6F\x9A"	/* WFA OUI */
#define WFA_OUI_LEN		3		/* WFA OUI length */
#endif

/* WiFi DPP Public Action Frame Type */
#define DPP_PAF_AUTH_REQ         0       /* Authentication Req */
#define DPP_PAF_AUTH_RSP         1       /* Authentication Rsp */
#define DPP_PAF_AUTH_CONF        2       /* Authentication Confirm */
#define DPP_PAF_RESERVED1        3       /* RESERVED */
#define DPP_PAF_RESERVED2        4       /* RESERVED */
#define DPP_PAF_PEER_DISC_REQ    5       /* Peer Discovery Request */
#define DPP_PAF_PEER_DISC_RSP    6       /* Peer Discovery Response */
#define DPP_PAF_PKEX_EX_REQ      7       /* PKEX Exchange Request */
#define DPP_PAF_PKEX_EX_RSP      8       /* PKEX Exchange Response */
#define DPP_PAF_PKEX_CM_REQ      9       /* PKEX Commit-Reveal Request */
#define DPP_PAF_PKEX_CM_RSP      10      /* PKEX Commit-Reveal Response */
#define DPP_PAF_TYPE_INVALID     255     /* Invalid type */

/* WiFi Generic Advertisement Service (GAS) Action Frame for DPP Configuration protocol */
struct wifi_dpp_gas_act_frame {
	uint8   category;        /* DPP_GAS_AF_CATEGORY */
	uint8   action;          /* DPP_GAS_AF_ACTION */
	uint8   token;           /* DPP_GAS token */
	uint8   asp_ie[3];       /* Advertisement Protocol element */
	uint8   asp_id[7];       /* Advertisement Protocol ID */
	uint16  req_len;         /* Query Request Length_* */
	uint8   gas_request[1];  /* Query Request Length */
} __attribute__ ((packed));
typedef struct wifi_dpp_gas_act_frame wifi_dpp_gas_act_frame_t;

/* GAS category is 0x04 also */
#define DPP_GAS_AF_ACTION		0x0A
#define DPP_GAS_AF_ACTION_FRAG	0x0C
#define DPP_GAS_AF_ASP_IE		"\x6C\x08\x00"	/* ID/LEN/INFO */
#define DPP_GAS_AF_ASP_ID		"\xDD\x05\x50\x6F\x9A\x1A\x01"	/* Type = 0x01, denoting the DPP Configuration protocol */

typedef enum wifi_hal_cb_dpp_subtype {
	WIFI_HAL_CB_DPP_AUTH_RESP = 1,
	WIFI_HAL_CB_DPP_CONFIG_REQ,
	WIFI_HAL_CB_ANQP_QAS_REQ
} wifi_hal_cb_dpp_subtype_t;

/* for ecbd call wifi_hal function wifi_receivedMgmtFrame */
typedef enum
{
	WIFI_MGMT_FRAME_TYPE_PROBE_REQ = 0,
	WIFI_MGMT_FRAME_TYPE_PROBE_RSP = 1,
	WIFI_MGMT_FRAME_TYPE_ASSOC_REQ = 2,
	WIFI_MGMT_FRAME_TYPE_ASSOC_RSP = 3,
	WIFI_MGMT_FRAME_TYPE_AUTH = 4,
	WIFI_MGMT_FRAME_TYPE_DEAUTH = 5,
	WIFI_MGMT_FRAME_TYPE_REASSOC_REQ = 6,
	WIFI_MGMT_FRAME_TYPE_REASSOC_RSP = 7,
	WIFI_MGMT_FRAME_TYPE_DISASSOC = 8,
	WIFI_MGMT_FRAME_TYPE_ACTION = 9,
} wifi_mgmtFrameType_t;

/* all socket */
enum {
	FD_TIMER = 0,
	FD_EAPD,
	FD_WIFI_HAL,
	FD_STA_CONN_CB,
	FD_ASSOC_DEV_CB,
	FD_AUTH_FAIL_CB,
	FD_MESH_CB,
	FD_RRM_BCNREP_CB,
	FD_BSSTRANS_CB,
	FD_DPP_CB,
	FD_CH_CHG_CB,
	ECBD_NUM_FD
};

/*
 * TODO: The below is duplicate from wifi_hal.h and wifi_hal_cb.h.
 * once wifi_hal.h and wifi_hal_cb.h files are included the below needs to be deleted.
 */
#define WIFI_HAL_TO_ECBD_MSG_UDP_PORT   51010
#define WIFI_HAL_CB_AUTH_FAIL_UDP_PORT  51012 /* change to use domain socket under RDKB_WLDM */
#define WIFI_HAL_CB_MESH_APCFG_UDP_PORT 51013 /* change to use domain socket under RDKB_WLDM */

/* domain path: all callback will use domain socket, same definition as wldm */
#define WIFI_HAL_CB_STA_CONN_DSOCKET    "/tmp/sta_connect_cb"
#define WIFI_HAL_CB_ASSOC_DEV_DSOCKET   "/tmp/assoc_dev_cb"
#define WIFI_HAL_CB_AUTH_FAIL_DSOCKET   "/tmp/auth_fail_cb"
#define WIFI_HAL_CB_MESH_STEER_DSOCKET  "/tmp/mesh_steer_cb"
#define WIFI_HAL_CB_RRM_BCNREP_DSOCKET  "/tmp/bcn_report_cb"
#define WIFI_HAL_CB_BSSTRANS_DSOCKET    "/tmp/bss_transit_cb"
/* support DPP (Device Provisioning Protocol) */
#define WIFI_HAL_CB_DPP_DSOCKET         "/tmp/dpp_cb"
#define WIFI_HAL_CB_CH_CHG_DSOCKET      "/tmp/ch_chg_cb"

#define MAX_MAC_ADDR_LEN	20
#define WIFI_HAL_EVT_VERSION	1

typedef enum wifi_hal_cb_type {
	WIFI_HAL_CB_STA_CONN = 1,
	WIFI_HAL_CB_ASSOC_DEV,
	WIFI_HAL_CB_AUTH_FAIL,
	WIFI_HAL_CB_MESH,
	WIFI_HAL_CB_RRM_BCNREP,
	WIFI_HAL_CB_BSSTRANS,
	WIFI_HAL_CB_DPP,
	WIFI_HAL_CB_CH_CHG,
	WIFI_HAL_CB_RADAR,
	WIFI_HAL_MAX_CB_TYPE
} wifi_hal_cb_type_t;

typedef enum wifi_hal_cb_btm_subtype {
	WIFI_HAL_CB_BSSTRANS_REQ = 1,
	WIFI_HAL_CB_BSSTRANS_QUERY,
	WIFI_HAL_CB_BSSTRANS_RESP
} wifi_hal_cb_btm_subtype_t;

/*
 * TODO: remove the below enum once wifi_hal.h is linked to ecbd
 */
typedef enum sta_conn_type {
	CONN_NEW = 1,
	CONN_RENEW = 2,
	CONN_RECONN_AFTER_INACTIVITY = 3,
	MAX_CONN_TYPE
} sta_conn_type_t;

#define MAX_CB_SUBSCRIBERS	8
typedef struct sta_conn_cb_subscribers {
	char sock_path[UNIX_PATH_MAX];
	struct sockaddr_un addr;
	socklen_t addr_len;
	int count;
	int FD[MAX_CB_SUBSCRIBERS];
} sta_conn_cb_subscribers_t;

typedef struct bcn_report_cb_subscribers {
	char sock_path[UNIX_PATH_MAX];
	struct sockaddr_un addr;
	socklen_t addr_len;
	int count;
	int FD[MAX_CB_SUBSCRIBERS];
} bcn_report_cb_subscribers_t;

typedef enum wifi_hal_cb_auth_fail_reason {
	WIFI_HAL_CB_AUTH_UNKNOWN = 0,
	WIFI_HAL_CB_AUTH_PASSWORD_FAILURE,
	WIFI_HAL_CB_AUTH_TIMEOUT,
	WIFI_HAL_CB_AUTH_MAX
} wifi_hal_cb_auth_failure_reason_t;

typedef struct wifi_hal_cb_evt {
	int version;
	int type;
	int reason;
	int apIndex;
	char mac[MAX_MAC_ADDR_LEN];
	unsigned char data[0];
} wifi_hal_cb_evt_t;

typedef enum wifi_hal_msg_type {
	WIFI_HAL_MSG_UNKNOWN = 0,
	WIFI_HAL_MSG_AP_CONFIG,
	WIFI_HAL_MSG_CLIENT_SET,
	WIFI_HAL_MSG_CLIENT_RM,
	WIFI_HAL_MSG_CLIENT_MEAS,
	WIFI_HAL_MSG_CLIENT_DISCONN,
	WIFI_HAL_MSG_MAX
} wifi_hal_msg_type_t;

/* for mesh steering event, copy from wifi_hal.h */
typedef unsigned char mac_address_t[6];

/* message from wifi_hal */
typedef struct wifi_hal_message {
	uint hal_msg_type;
	uint hal_msg_len;
	char data[0];
} wifi_hal_message_t;

/* wifi_hal ClientSet etc */
typedef struct wifi_steering_client {
	uint groupIndex;
	int apIndex;
	mac_address_t cli_mac;
	char data[0];
} wifi_steering_client_t;

/* from wifi_hal.h */
typedef struct wifi_steering_apConfig {
	int apIndex;
	uint utilCheckIntervalSec;   /**< Chan utilization check interval        */
	uint utilAvgCount;           /**< Number of samples to average           */
	uint inactCheckIntervalSec;  /**< Client inactive check internval        */
	uint inactCheckThresholdSec; /**< Client inactive threshold              */
} wifi_steering_apConfig_t;

/* wifi_hal setGroup */
typedef struct wifi_steering_setGroup {
	uint steeringgroupIndex;
	wifi_steering_apConfig_t cfg[];
} wifi_steering_setGroup_t;

/* group info */
typedef struct wifi_steering_apStats {
	uint avg_chan_util;
	uint chan_util_cnt;
	/* todo: others */
} wifi_steering_apStats_t;

typedef struct wifi_steering_group_info {
	wifi_steering_apConfig_t cfg[HAL_RADIO_NUM_RADIOS]; /* 0: 2G, 1: 5G */
	wifi_steering_apStats_t stats[HAL_RADIO_NUM_RADIOS];
} wifi_steering_group_info_t;

typedef struct wifi_steering_group {
	uint group_index;
	uint group_enable;
	wifi_steering_group_info_t *group_info;
} wifi_steering_group_t;

#ifndef ULLONG
#define ULLONG unsigned long long
#endif

#ifndef BOOL
#define BOOL  unsigned char
#endif

#ifndef CHAR
#define CHAR  char
#endif

#ifndef UCHAR
#define UCHAR unsigned char
#endif

#ifndef INT
#define INT   int
#endif

#ifndef UINT
#define UINT  unsigned int
#endif

typedef enum {
    WIFI_STEERING_EVENT_PROBE_REQ           = 1,    /**< Probe Request Event        */
    WIFI_STEERING_EVENT_CLIENT_CONNECT,             /**< Client Connect Event       */
    WIFI_STEERING_EVENT_CLIENT_DISCONNECT,          /**< Client Disconnect Event    */
    WIFI_STEERING_EVENT_CLIENT_ACTIVITY,            /**< Client Active Change Event */
    WIFI_STEERING_EVENT_CHAN_UTILIZATION,           /**< Channel Utilization Event  */
    WIFI_STEERING_EVENT_RSSI_XING,                  /**< Client RSSI Crossing Event */
    WIFI_STEERING_EVENT_RSSI,                       /**< Instant Measurement Event  */
    WIFI_STEERING_EVENT_AUTH_FAIL                   /**< Client Auth Failure Event  */
} wifi_steering_eventType_t;

typedef enum {
    WIFI_STEERING_RSSI_UNCHANGED            = 0,    /**< RSSI hasn't crossed        */
    WIFI_STEERING_RSSI_HIGHER,                      /**< RSSI went higher           */
    WIFI_STEERING_RSSI_LOWER                        /**< RSSI went lower            */
} wifi_steering_rssiChange_t;

typedef enum {
    DISCONNECT_SOURCE_UNKNOWN               = 0,    /**< Unknown source             */
    DISCONNECT_SOURCE_LOCAL,                        /**< Initiated locally          */
    DISCONNECT_SOURCE_REMOTE                        /**< Initiated remotely         */
} wifi_disconnectSource_t;

typedef enum {
    DISCONNECT_TYPE_UNKNOWN                 = 0,    /**< Unknown type               */
    DISCONNECT_TYPE_DISASSOC,                       /**< Disassociation             */
    DISCONNECT_TYPE_DEAUTH                          /**< Deauthentication           */
} wifi_disconnectType_t;

/**
 * @brief STA datarate information
 * These are STA capabilities values
 */
typedef struct {
    UINT                            maxChwidth;         /**< Max bandwidth supported                */
    UINT                            maxStreams;         /**< Max spatial streams supported          */
    UINT                            phyMode;            /**< PHY Mode supported                     */
    UINT                            maxMCS;             /**< Max MCS  supported                     */
    UINT                            maxTxpower;         /**< Max TX power supported                 */
    UINT                            isStaticSmps;       /**< Operating in Static SM Power Save Mode */
    UINT                            isMUMimoSupported;  /**< Supports MU-MIMO                       */
} wifi_steering_datarateInfo_t;

typedef struct {
    BOOL                            linkMeas;           /**< Supports link measurement      */
    BOOL                            neighRpt;           /**< Supports neighbor reports      */
    BOOL                            bcnRptPassive;      /**< Supports Passive 11k scans     */
    BOOL                            bcnRptActive;       /**< Supports Active 11k scans      */
    BOOL                            bcnRptTable;        /**< Supports beacon report table   */
    BOOL                            lciMeas;            /**< Supports LCI measurement       */
    BOOL                            ftmRangeRpt;        /**< Supports FTM Range report      */
} wifi_steering_rrmCaps_t;

typedef struct {
    mac_address_t                   client_mac;     /**< Client MAC Address         */
    UINT                            rssi;           /**< RSSI of probe frame        */
    BOOL                            broadcast;      /**< True if broadcast probe    */
    BOOL                            blocked;        /**< True if response blocked   */
} wifi_steering_evProbeReq_t;

typedef struct {
    mac_address_t                   client_mac;     /**< Client MAC Address         */
    UINT                            isBTMSupported; /**< Client supports BSS TM                 */
    UINT                            isRRMSupported; /**< Client supports RRM                    */
    BOOL                            bandCap2G;      /**< Client is 2.4GHz capable               */
    BOOL                            bandCap5G;      /**< Client is 5GHz capable                 */
    wifi_steering_datarateInfo_t    datarateInfo;   /**< Client supported datarate information  */
    wifi_steering_rrmCaps_t         rrmCaps;        /**< Client supported RRM capabilites       */
} wifi_steering_evConnect_t;

typedef struct {
    mac_address_t                   client_mac;     /**< Client MAC Address         */
    UINT                            reason;         /**< Reason code of disconnect  */
    wifi_disconnectSource_t         source;         /**< Source of disconnect       */
    wifi_disconnectType_t           type;           /**< Disconnect Type            */
} wifi_steering_evDisconnect_t;

typedef struct {
    mac_address_t                   client_mac;     /**< Client MAC Address         */
    BOOL                            active;         /**< True if client is active   */
} wifi_steering_evActivity_t;

typedef struct {
    UINT                            utilization;    /**< Channel Utilization 0-100  */
} wifi_steering_evChanUtil_t;

typedef struct {
    mac_address_t                   client_mac;     /**< Client MAC Address         */
    UINT                            rssi;           /**< Clients current RSSI       */
    wifi_steering_rssiChange_t      inactveXing;    /**< Inactive threshold Value   */
    wifi_steering_rssiChange_t      highXing;       /**< High threshold Value       */
    wifi_steering_rssiChange_t      lowXing;        /**< Low threshold value        */
} wifi_steering_evRssiXing_t;

typedef struct {
    mac_address_t                   client_mac;     /**< Client MAC Address         */
    UINT                            rssi;           /**< Clients current RSSI       */
} wifi_steering_evRssi_t;

typedef struct {
    mac_address_t                   client_mac;     /**< Client MAC Address         */
    UINT                            rssi;           /**< RSSI of auth frame         */
    UINT                            reason;         /**< Reject Reason              */
    BOOL                            bsBlocked;      /**< True if purposely blocked  */
    BOOL                            bsRejected;     /**< True if rejection sent     */
} wifi_steering_evAuthFail_t;

typedef struct {
	wifi_steering_eventType_t       type;           /**< Event TYpe                 */
	int                             apIndex;        /**< apIndex event is from      */
	ULLONG                          timestamp_ms;   /**< Optional: Event Timestamp  */
	union {
		wifi_steering_evProbeReq_t      probeReq;   /**< Probe Request Data         */
		wifi_steering_evConnect_t       connect;    /**< Client Connect Data        */
		wifi_steering_evDisconnect_t    disconnect; /**< Client Disconnect Data     */
		wifi_steering_evActivity_t      activity;   /**< Client Active Change Data  */
		wifi_steering_evChanUtil_t      chanUtil;   /**< Channel Utilization Data   */
		wifi_steering_evRssiXing_t      rssiXing;   /**< Client RSSI Crossing Data  */
		wifi_steering_evRssi_t          rssi;       /**< Client Measured RSSI Data  */
		wifi_steering_evAuthFail_t      authFail;   /**< Auth Failure Data          */
	} data;
} wifi_steering_event_t;

typedef struct ecbd_info {
	int version;
	int enable;			/* ECBD enabled or not */
	uint poll_interval; /* polling interval */
	uint ticks;			/* number of polling intervals */
	sta_conn_cb_subscribers_t sta_conn_cb_subscriber_fds;
	sta_conn_cb_subscribers_t assoc_dev_cb_subscriber_fds;
	sta_conn_cb_subscribers_t auth_fail_cb_subscriber_fds;
	sta_conn_cb_subscribers_t mesh_cb_subscriber_fds;
	bcn_report_cb_subscribers_t bcn_report_cb_subscriber_fds;
	sta_conn_cb_subscribers_t bsstrans_cb_subscriber_fds; /* re-use the data structure */
	sta_conn_cb_subscribers_t dpp_cb_subscriber_fds; /* re-use the data structure */
	sta_conn_cb_subscribers_t ch_chg_cb_subscriber_fds;
	/* TO DO */
} ecbd_info_t;

static int ecbd_apidx_to_ifname(int apIndex, char *ifname);
static wifi_steering_apConfig_t* ecbd_find_ap_cfg(uint8 ifidx, uint8 bsscfgidx, uint *g_idx);
static void ecbd_send_active_state_event(uint g_idx, int ap_idx,
	mac_address_t cli_mac, BOOL active);
static void ecbd_send_rssi_xing_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta,
	int rssi, int rssi_type);
static int ecbd_get_sta_rssi(char *ifname, struct ether_addr *addr, int *rssi);
static int ecbd_disconnect_sta(char *ifname, struct ether_addr *addr, int type, int reason);
static void ecbd_send_probereq_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta,
	int rssi, BOOL bcast);
static void ecbd_send_steering_conn_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta,
	int event_type, int reason, int source, int type);
static void ecbd_send_steering_authfail_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta,
	int rssi, int reason, BOOL rejected);
static int ecbd_get_noise(char *ifname, int *noise);
static void ecbd_send_rssi_measure_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta);
static int ecbd_enable_resp_filter(char *ifname);
static int ecbd_enable_macmode_allow(char *ifname);
static int ecbd_add_maclist(char *ifname, struct ether_addr *sta_addr);
static int ecbd_del_maclist(char *ifname, struct ether_addr *sta_addr);

#ifdef BUILD_RDKWIFI
#ifndef htod32
#define htod32(i)       (i)
#define htod16(i)       (i)
#define dtoh64(i)       (i)
#define dtoh32(i)       (i)
#define dtoh16(i)       (i)
#define htodchanspec(i) (i)-0x001000
#define dtohchanspec(i) (i)
#endif /* htod32 */
#endif /* BUILD_RDKWIFI */

#ifdef RDKB_RADIO_STATS_MEASURE /* definition changed, keep the old one as refernece */
/* some radio stats calculate for wifi_getRadioTrafficStats2 */

/* per radio based, need prefix wl0_/wl1_*/
#ifndef NVRAM_RADIO_STATS_MEAS_RATE /* may be defined in wldm_xxx.h */
#define NVRAM_RADIO_STATS_MEAS_RATE "radio_stats_measure_rate"
#define NVRAM_RADIO_STATS_MEAS_INTEVAL "radio_stats_measure_interval"
#define TMP_RADIO_STATS_FILE "radio_stats.txt"
#endif

#define DEFAULT_RADIO_STATS_MEAS_RATE 30 /* in sec */
#define DEFAULT_RADIO_STATS_MEAS_INTEVAL 1800 /* in sec, 30 min */

typedef struct ecbd_radio_stats {
	int radio_stats_measure_rate;
	int radio_stats_measure_interval;
	uint avg_count;
	/* match counter's name in wifi_hal */
	int radio_ChannelUtilization;
	int radio_ActivityFactor;
	int radio_RetransmissionMetirc;
	int radio_CarrierSenseThreshold_Exceeded;
	/* history counter */
	uint radio_tx;
	uint radio_retx;
} ecbd_radio_stats_t;

typedef struct ecbd_radio_stats_data {
	int act_factor;
	int obss;
} ecbd_radio_stats_data_t;
#endif /* RDKB_RADIO_STATS_MEASURE */

/* For ApAssociatedDevicesHighWatermarkThreshold */
/* should be lesser than or equal to MaxAssociatedDevices, default 50, if "0", no calculation */
/* For get/set ApAssociatedDevicesHighWatermarkThreshold */
#define NVRAM_ASSOC_DEV_HWM_TH "assoc_dev_hwm_th"

/* Number of times the current total number of associated device has reached the
   HighWatermarkThreshold value. This calculation can be based on the parameter
   AssociatedDeviceNumberOfEntries as well. Implementation specifics about this
   parameter are left to the product group and the device vendors. It can be updated
   whenever there is a new client association request to the access point. */
/* For wifi_getApAssociatedDevicesHighWatermarkThresholdReached */
#define NVRAM_ASSOC_DEV_HWM_TH_REACHED "assoc_dev_hwm_th_reached"

/* Maximum number of associated devices that have ever associated with the access point
   concurrently since the last reset of the device or WiFi module. */
/* For wifi_getApAssociatedDevicesHighWatermark */
#define NVRAM_ASSOC_DEV_HWM_MAX "assoc_dev_hwm_max"

/* Date and Time at which the maximum number of associated devices ever associated with the
   access point concurrenlty since the last reset of the device or WiFi module (or in short when
   was X_COMCAST-COM_AssociatedDevicesHighWatermark updated). This dateTime value is in UTC. */
/* For wifi_getApAssociatedDevicesHighWatermarkDate */
#define NVRAM_ASSOC_DEV_HWM_MAX_DATE "assoc_dev_hwm_max_date"

#define DEFAULT_ASSOC_DEV_HWM_TH 50

#endif /* __ECBD_H__ */
