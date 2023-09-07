/*
 * Linux-specific portion of ECBD (Event Callback Daemon)
 * (OS dependent file)
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
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wifi_hal_cb.h $
 */
#ifndef __WIFI_HAL_CB_H__
#define __WIFI_HAL_CB_H__

typedef struct {
	int thread_initialized;
	int count;
	wifi_apAssociatedDevice_callback associate_cb;
	wifi_newApAssociatedDevice_callback assoc_dev_cb;
	wifi_apAuthEvent_callback auth_cb;
	wifi_steering_eventCB_t mesh_cb;
#if WIFI_HAL_VERSION_GE_2_12 || defined(WIFI_HAL_VERSION_3)
	wifi_RMBeaconReport_callback beaconReport_cb;
#endif
	wifi_BTMQueryRequest_callback btm_query_req_cb;
	wifi_BTMResponse_callback btm_resp_cb;
#if WIFI_HAL_VERSION_GE_2_16 || defined(WIFI_HAL_VERSION_3)
#if WIFI_HAL_VERSION_GE_2_19 || defined(WIFI_HAL_VERSION_3)
	wifi_receivedMgmtFrame_callback mgmt_frame_cb;
#else
	wifi_dppAuthResponse_callback_t dpp_authresp_cb;
	wifi_dppConfigRequest_callback_t dpp_configreq_cb;
#endif /* WIFI_HAL_MINOR_VERSION >= 19 */
#endif
#if WIFI_HAL_VERSION_GE_2_18 || defined(WIFI_HAL_VERSION_3)
	wifi_chan_eventCB_t chan_change_cb;
#endif /* WIFI_HAL_VERSION_GE_2_18 */
} wifi_callback_fnc_t;

#define MAX_EVENT_BUFFER_LEN	1024

#define WIFI_HAL_TO_ECBD_MSG_UDP_PORT		51010
// #define WIFI_HAL_CB_STA_CONN_UDP_PORT		51011		/* Socket port for WiFI_HAL and ECBD communication */
#define WIFI_HAL_CB_AUTH_FAIL_UDP_PORT		51012		/* Socket port for WiFI_HAL and ECBD communication */
#define WIFI_HAL_CB_MESH_APCFG_UDP_PORT		51013
// #define WIFI_HAL_CB_RRM_BCNREP_UDP_PORT		51014		/* Socket port for rrm msgs between WiFI_HAL and ECBD */

#define WIFI_HAL_CB_STA_CONN_DSOCKET    "/tmp/sta_connect_cb"
#define WIFI_HAL_CB_RRM_BCNREP_DSOCKET  "/tmp/bcn_report_cb"
#define WIFI_HAL_CB_BSSTRANS_DSOCKET	"/tmp/bss_transit_cb"
#define WIFI_HAL_CB_DPP_DSOCKET			"/tmp/dpp_cb"
#define WIFI_HAL_CB_CH_CHG_DSOCKET	"/tmp/ch_chg_cb"

#define MAX_MAC_ADDR_LEN	20

#define WIFI_HAL_EVT_VERSION    1

/* type inside event data, sync with ecbd */
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
	WIFI_HAL_CB_BSSTRANS_REQ=1,
	WIFI_HAL_CB_BSSTRANS_QUERY,
	WIFI_HAL_CB_BSSTRANS_RESP,
} wifi_hal_cb_btm_subtype_t;

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
	WIFI_HAL_MSG_RRM_BCNREP,
	WIFI_HAL_MSG_MAX
} wifi_hal_msg_type_t;

/* wifi_hal message */
typedef struct wifi_hal_message {
	uint hal_msg_type;
	uint hal_msg_len;
	char data[0];
} wifi_hal_message_t;

/* wifi_hal setGroup */
typedef struct wifi_steering_setGroup {
	uint steeringgroupIndex;
	wifi_steering_apConfig_t cfg[];
} wifi_steering_setGroup_t;

/* wifi_hal ClientSet etc */
typedef struct wifi_steering_client {
	uint groupIndex;
	int apIndex;
	mac_address_t cli_mac;
	char data[0];
} wifi_steering_client_t;

static int wifi_hal_notify_ecbd(void *hal_msg, uint len, unsigned short port);

/* support DPP (Device Provisioning Protocol), copy from ecbd.h */
#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

/* WiFi DPP Public Action Frame for DPP Auth protocol */
struct wifi_dpp_pub_act_frame {
	uint8   category;       /* DPP_PUB_AF_CATEGORY */
	uint8   action;         /* DPP_PUB_AF_ACTION */
	uint8   oui[3];         /* DPP_OUI */
	uint8   oui_type;       /* OUI type - DPP_VER */
	uint8   crypto_suite;   /* OUI subtype - DPP_TYPE_* */
	uint8   dpp_frame_type; /* nonzero, identifies req/rsp transaction */
	uint8   attributes[1];  /* A series of one or more DPP Attributes*/
} __attribute__ ((packed));
typedef struct wifi_dpp_pub_act_frame wifi_dpp_pub_act_frame_t;

#define DPP_PUB_AF_CATEGORY     0x04
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
	uint8   gas_request[1];  /* Query Request Length*/
} __attribute__ ((packed));
typedef struct wifi_dpp_gas_act_frame wifi_dpp_gas_act_frame_t;

/* GAS category is 0x04 also */
#define DPP_GAS_AF_ACTION	0x0A
#define DPP_GAS_AF_ACTION_FRAG	0x0C
#define DPP_GAS_AF_ASP_IE	"\x6C\x08\x00"	/* ID/LEN/INFO */
#define DPP_GAS_AF_ASP_ID	"\xDD\x05\x50\x6F\x9A\x1A\x01"	/* Type = 0x01, denoting the DPP Configuration protocol */

typedef enum wifi_hal_cb_dpp_subtype {
	WIFI_HAL_CB_DPP_AUTH_RESP=1,
	WIFI_HAL_CB_DPP_CONFIG_REQ,
	WIFI_HAL_CB_ANQP_QAS_REQ
} wifi_hal_cb_dpp_subtype_t;

/* flag if callback take over the control (to hspotap.c) */
#define NVNM_HS2_ANQP_HAL	"hs2_anqp_hal"

/* copy/use wl event type directly */
#ifndef WLC_E_ASSOC_IND
#define WLC_E_ASSOC_IND		8	/* 802.11 ASSOC indication */
#define WLC_E_REASSOC_IND	10	/* 802.11 REASSOC indication */
#define WLC_E_DISASSOC_IND	12	/* 802.11 DISASSOC indication */
#endif

#ifndef WLC_E_RADAR_DETECTED
#define WLC_E_RADAR_DETECTED	160     /* Radar Detected event */
#endif
#ifndef WLC_E_AP_CHAN_CHANGE
#define WLC_E_AP_CHAN_CHANGE	170     /* AP channe change event propagate to use */
#endif

#endif /* __WIFI_HAL_CB_H__ */
