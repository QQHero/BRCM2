/*
 * WBD Communication Related Definitions
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
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wbd_slave_com_hdlr.c 810808 2022-04-14 12:06:28Z $
 */

#include "wbd.h"
#include "wbd_shared.h"
#include "wbd_ds.h"
#include "wbd_com.h"
#include "wbd_sock_utility.h"
#include "wbd_json_utility.h"
#include "wbd_wl_utility.h"
#include "wbd_slave_control.h"
#include "wbd_slave_com_hdlr.h"
#include "wbd_blanket_utility.h"
#ifdef PLC_WBD
#include "wbd_plc_utility.h"
#endif /* PLC_WBD */

#include "ieee1905.h"
#include "ieee1905_tlv.h"
#include "wbd_tlv.h"
#include "wbd_slave_vndr_brcm.h"

#include <shutils.h>
#include <wlutils.h>
#include <wlif_utils.h>
#include <common_utils.h>
#ifdef BCM_APPEVENTD
#include "wbd_appeventd.h"
#endif /* BCM_APPEVENTD */
#include <wbd_rc_shared.h>
#include "blanket.h"
#include <linux/if_ether.h>

#ifndef BCM_EVENT_HEADER_LEN
#define BCM_EVENT_HEADER_LEN   (sizeof(bcm_event_t))
#endif

#define	WBD_TAF_STA_FMT				"toa-sta-%d"
#define	WBD_TAF_BSS_FMT				"toa-bss-%d"
#define WBD_TAF_DEF_FMT				"toa-defs"

/* Macros for ESP */
#define DATA_FORMAT_SHIFT	3
#define BA_WSIZE_SHIFT		5

#define ESP_BE			1
#define ESP_AMSDU_ENABLED	(1 << DATA_FORMAT_SHIFT)
#define ESP_AMPDU_ENABLED	(2 << DATA_FORMAT_SHIFT)
#define ESP_BA_WSIZE_NONE	(0 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_2		(1 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_4		(2 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_6		(3 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_8		(4 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_16		(5 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_32		(6 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_64		(7 << BA_WSIZE_SHIFT)

/* DFS CAC request status on radio after receiving cac msg from controller */
#define MAP_CAC_RQST_RCVD			0x01
#define MAP_CAC_RQST_RUNNING			0x02
#define MAP_CAC_RQST_COMPLETE			0x04

#define WBD_SWAP(a, b, T) do { T _t; _t = a; a = b; b = _t; } while (0)

#define WEAK_STRONG_STR(command) (((command) == WBD_STA_STATUS_WEAK) ? "WEAK" : "STRONG")

/* Information for slave's beacon request timer callback */
typedef struct wbd_bcn_req_arg {
	wbd_info_t *info;	/* wbd_info pointer */
	char ifname[IFNAMSIZ];	/* interface on which sta is associated */
	unsigned char neighbor_al_mac[IEEE1905_MAC_ADDR_LEN]; /* AL ID from where request came */
	blanket_bcnreq_t bcnreq_extn;	/* beacon request parameters */
	uint8 chan_count;	/* Count of channels on which beacon request to be sent */
	uint8 curr_chan_idx;	/* Index of current channel */
	uint8 *chan;		/* Array of channels */
	uint8 *opclass;         /* Array of op classes matching channels */
	uint8 subelement[WBD_MAX_BUF_64];	/* subelements array */
	uint8 subelem_len;	/* length of subelements */
} wbd_bcn_req_arg_t;

/* Information for Slave's action frame timer callback */
typedef struct wbd_actframe_arg {
	wbd_slave_item_t *slave;
	i5_dm_bss_type *i5_bss;	/* BSS where the STA is associated */
	int af_count; /* Number of actframes to send */
	struct ether_addr sta_mac; /* STA mac address */
} wbd_actframe_arg_t;

/* structure to hold the params to retry the STEER in case of STA not accepts */
typedef struct wbd_slave_steer_retry_arg {
	wbd_slave_item_t *slave;			/* Slave item */
	struct timeval assoc_time;			/* Last associated time */
	wbd_slave_steer_resp_cb_data_t *resp_cb_data;	/* Data to be sent to steering library */
} wbd_slave_steer_retry_arg_t;

/* structure to hold the params to send the unassociated STA link metrics */
typedef struct wbd_slave_map_monitor_arg {
	wbd_slave_item_t *slave;	/* Slave item */
	unsigned char neighbor_al_mac[IEEE1905_MAC_ADDR_LEN];	/* AL ID from where request
								 * came (Used only at agent)
								 */
	wbd_maclist_t *maclist;
	uint8 channel;		/* channel at which monitor request came */
	uint8 rclass;		/* Operating class for the channel specified */
} wbd_slave_map_monitor_arg_t;

/* structure to hold the params send the beacon report after time expires */
typedef struct wbd_slave_map_beacon_report_arg {
	wbd_info_t *info;		/* WBD Info */
	struct ether_addr sta_mac;	/* MAC address of STA */
} wbd_slave_map_beacon_report_arg_t;

typedef struct wbd_slave_valid_chan_info_list {
	uint8 opclass;
	uint8 channel;
	uint16 bitmap;
} wbd_slave_valid_chan_info_list_t;
#ifdef WLHOSTFBT
/* Check whether FBT enabling is possible or not. First it checks for psk2 and then wbd_fbt */
extern int wbd_is_fbt_possible(char *prefix);
#endif /* WLHOSTFBT */

/* ------------------------------------ Static Declarations ------------------------------------ */

/* Allocates memory for stamon maclist */
static bcm_stamon_maclist_t*
wbd_slave_alloc_stamon_maclist_struct(int ncount);
/* Creates maclist structure for input to stamon */
static int
wbd_slave_prepare_stamon_maclist(struct ether_addr *mac,
	bcm_stamon_maclist_t **stamonlist);
/* Remove a STA MAC from STAMON */
static int
wbd_slave_remove_sta_fm_stamon(wbd_slave_item_t *slave_item, struct ether_addr *mac,
	BCM_STAMON_STATUS *status);
/* Add MAC address and Priority to stamon maclist on index */
static int
wbd_slave_add_sta_to_stamon_maclist(bcm_stamon_maclist_t *list, struct ether_addr *mac,
	bcm_stamon_prio_t priority, chanspec_t chspec, int idx,
	bcm_offchan_sta_cbfn *cbfn, void *arg);
/* Parse and process the EVENT from EAPD */
static int
wbd_slave_process_event_msg(wbd_info_t* info, char* pkt, int len);
#ifdef PLC_WBD
/* Retrieve fresh stats from PLC for all the PLC associated nodes and update locally */
static int
wbd_slave_update_plc_assoclist(wbd_slave_item_t *slave);
#endif /* PLC_WBD */
/* Processes WEAK_CLIENT response */
static void
wbd_slave_process_weak_client_cmd_resp(unsigned char *neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len);
/* Callback fn for Slave to do rc restart gracefully */
static void
wbd_slave_rc_restart_timer_cb(bcm_usched_handle *hdl, void *arg);
/* Slave updates WEAK_CANCEL_BSD/STRONG_CANCEL_BSD Data */
static int
wbd_slave_process_weak_strong_cancel_bsd_data(i5_dm_bss_type *i5_bss,
	wbd_cmd_weak_cancel_bsd_t *cmdweakcancel, wbd_sta_status_t command);
/* Processes WEAK_CLIENT_BSD request */
static void
wbd_slave_process_weak_client_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Processes WEAK_CANCEL_BSD request */
static void
wbd_slave_process_weak_cancel_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Processes STRONG_CLIENT_BSD request */
static void
wbd_slave_process_strong_client_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata,
	void *arg);
/* Processes STRONG_CANCEL_BSD request */
static void
wbd_slave_process_strong_cancel_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata,
	void *arg);
/* Slave updates WEAK_CLIENT_BSD/STRONG_CLIENT_BSD Data */
static int
wbd_slave_process_weak_strong_client_bsd_data(i5_dm_bss_type *i5_bss,
	wbd_cmd_weak_client_bsd_t *cmdweakclient, wbd_sta_status_t command);
/* Processes WEAK_CLIENT_BSD/STRONG_CLIENT_BSD request */
static void
wbd_slave_process_weak_strong_client_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata,
	void *arg, wbd_sta_status_t command);
/* Processes WEAK_CANCEL_BSD/STRONG_CANCEL_BSD request */
static void
wbd_slave_process_weak_strong_cancel_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata,
	void *arg, wbd_sta_status_t command);
/* Processes STA_STATUS_BSD request */
static void
wbd_slave_process_sta_status_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Callback to send ACTION frame to STA */
static void
wbd_slave_actframe_timer_cb(bcm_usched_handle *hdl, void *arg);
/* Callback to send beacon request frame to weak STA */
static void
wbd_slave_bcn_req_timer_cb(bcm_usched_handle *hdl, void *arg);
/* Convert WBD error code to BSD error code */
static int
wbd_slave_wbd_to_bsd_error_code(int error_code);
/* Process BLOCK_CLIENT_BSD request */
static void
wbd_slave_process_blk_client_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* get taf's traffic configuration */
static int wbd_slave_get_taf_conf(wbd_slave_item_t* slave);
/* get taf's list configuration */
static int wbd_slave_get_taf_list(wbd_taf_list_t* list, const char* keyfmt);
/* get taf's def traffic configuration */
static int wbd_slave_get_taf_defprio(wbd_slave_item_t* slave);
/* Callback function indicating off channel STA addition to stamon driver */
static void wbd_slave_offchan_sta_cb(void *arg, struct ether_addr *ea);
/* Send beacon report */
static int
wbd_slave_store_beacon_report(char *ifname, uint8 *data, struct ether_addr *sta_mac);
/* trigger timer to send action frame to client */
static int wbd_slave_send_action_frame_to_sta(wbd_info_t* info, wbd_slave_item_t *slave,
	i5_dm_bss_type *i5_bss, struct ether_addr* mac_addr);
/* Process the Traffic Separation config policy. Return 1 If there is any error in the policy set
 * So that error message can be sent from ieee1905
 */
static int wbd_slave_check_for_ts_policy_errors(ieee1905_policy_config *policy);
/* -------------------------- Add New Functions above this -------------------------------- */

/* Get the SLAVELIST CLI command data */
static int
wbd_slave_process_slavelist_cli_cmd_data(wbd_info_t *info,
	wbd_cli_send_data_t *clidata, char **outdataptr);
/* Processes the SLAVELIST CLI command */
static void
wbd_slave_process_slavelist_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Get the INFO CLI command data */
static int
wbd_slave_process_info_cli_cmd_data(wbd_info_t *info,
	wbd_cli_send_data_t *clidata, char **outdataptr);
/* Processes the INFO CLI command */
static void
wbd_slave_process_info_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Get the CLIENTLIST CLI command data */
static int
wbd_slave_process_clientlist_cli_cmd_data(wbd_info_t *info,
	wbd_cli_send_data_t *clidata, char **outdataptr);
/* Processes the CLIENTLIST CLI command */
static void
wbd_slave_process_clientlist_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Processes the MONITORADD CLI command data */
static int
wbd_slave_process_monitoradd_cli_cmd_data(wbd_info_t *info, wbd_cli_send_data_t *clidata,
	char **outdataptr);
/* Processes the MONITORADD CLI command  */
static void
wbd_slave_process_monitoradd_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Processes the MONITORDEL CLI command data */
static int
wbd_slave_process_monitordel_cli_cmd_data(wbd_info_t *info,
	wbd_cli_send_data_t *clidata, char **outdataptr);
/* Processes the MONITORDEL CLI command */
static void
wbd_slave_process_monitordel_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Get the MONITORLIST CLI command data */
static int
wbd_slave_process_monitorlist_cli_cmd_data(wbd_info_t *info,
	wbd_cli_send_data_t *clidata, char **outdataptr);
/* Processes the MONITORLIST CLI command */
static void
wbd_slave_process_monitorlist_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg);
/* Callback for exception from communication handler for slave server */
static void
wbd_slave_com_server_exception(wbd_com_handle *hndl, void *arg, WBD_COM_STATUS status);
/* Callback for exception from communication handler for CLI */
static void
wbd_slave_com_cli_exception(wbd_com_handle *hndl, void *arg, WBD_COM_STATUS status);
/* Register all the commands for master server to communication handle */
static int
wbd_slave_register_server_command(wbd_info_t *info);
/* send operating channel report to master */
static void
wbd_slave_send_operating_chan_report(i5_dm_interface_type *sifr);
/* process vendor specific metric reporting policy */
static void wbd_slave_process_metric_reportig_policy_vndr_cmd(unsigned char *src_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len);
/* process vendor specific strong STA reporting policy */
static void wbd_slave_process_strong_sta_reportig_policy_vndr_cmd(unsigned char *src_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len);
/* Processes BSS capability query message */
static void wbd_slave_process_bss_capability_query_cmd(unsigned char *neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len);
/* Handle zero wait dfs message from controller */
static int wbd_slave_process_zwdfs_msg(unsigned char* neighbor_al_mac, unsigned char *tlv_data,
	unsigned short tlv_data_len);
/* Send 1905 Vendor Specific zero wait dfs message from Agent to Controller */
static int wbd_slave_send_vendor_msg_zwdfs(wbd_cmd_vndr_zwdfs_msg_t *msg);
/* Processes backhaul STA metric policy vendor message */
static void wbd_slave_process_backhaul_sta_metric_policy_cmd(wbd_info_t *info,
	unsigned char *neighbor_al_mac, unsigned char *tlv_data, unsigned short tlv_data_len);
/* Processes NVRAM set vendor message */
static void wbd_slave_process_vndr_nvram_set_cmd(wbd_info_t *info, unsigned char *neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len);
/* Process vendor specific chan set command */
static int wbd_slave_process_vndr_set_chan_cmd(unsigned char* neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len);
#if !defined(MULTIAPR2)
/* Check chan_info and update to master for the same */
static void wbd_slave_chk_and_send_chan_config_info(i5_dm_interface_type *pdmif,
	bool send_explictly);
#endif /* !MULTIAPR2 */
/* Use controller's chan info and local chanspecs list to prepare dfs_channel_forced
 * list and pass to firmware via "dfs_channel_forced" iovar
 */
static void wbd_slave_process_dfs_chan_info(unsigned char *src_al_mac, unsigned char *tlv_data,
	unsigned short tlv_data_len);
/* Create dfs_pref_chan_list */
static void
wbd_slave_prepare_dfs_list(wbd_dfs_chan_info_t *chan_info, i5_dm_interface_type *pdmif,
	wl_dfs_forced_t* dfs_frcd);
/* Calculate Chan Util Fm Chanim Stats, & Update Chan Util of this ChScan Result */
static void wbd_slave_calc_chscanresult_chanutil(i5_dm_interface_type *i5_ifr,
	chanspec_t stats_chanspec, uint8 *ccastats, int8 bgnoise);
/* Fetch chanim_stats all : Update Channel Utilization of all Results post Scan */
static int wbd_slave_update_chscanresult_chanutil(i5_dm_interface_type *i5_ifr);
/* Get channel utilization */
static int wbd_slave_get_chan_util(char *ifname, unsigned char *chan_util);
#if defined(MULTIAPR2)
/* Send Association status notification request to controller */
static void wbd_slave_send_association_status_notification(i5_dm_bss_type *bss);
/* send tunnel message to controller */
static void wbd_slave_send_tunneled_msg(ieee1905_tunnel_msg_t *tunnel_msg);
/* process cac start request message from controller and trigger DFS CAC if required */
static void wbd_slave_process_cac_request(wbd_info_t *info, ieee1905_cac_rqst_list_t *pmsg);
static int wbd_slave_create_cac_start_timer(wbd_info_t *info);
static void wbd_slave_start_cac_cb(bcm_usched_handle *hdl, void *arg);
/* Process cac terminate message from controller and stop DFS CAC if running */
static void wbd_slave_process_cac_terminate_rqst(wbd_info_t *info,
	ieee1905_cac_termination_list_t *pmsg);
/* Prepare CAC capability for each 5G capable radio */
static void wbd_slave_prepare_radio_cac_capability(i5_dm_interface_type *pdmif, uint8 **pmem,
	uint16 *payload_len);
/* Start On Boot local Channel Scan on Agent for all Radios */
static void wbd_slave_process_onboot_channel_scan(bcm_usched_handle *hdl, void *arg);
/* Get driver capability and check wheather radio is capable of BGDFS */
static int wbd_slave_get_driver_capability(char *ifname, wbd_ifr_item_t *ifr_vndr_info);
/* Prepare CAC capability for the given method */
static uint8 wbd_slave_update_cac_capability_for_method(i5_dm_interface_type *pdmif, uint8 **pbuf,
	uint32 is_edcrs_eu, uint8 method);
/* Prepare CAC capability for the given method and cac time */
static int wbd_slave_update_cac_capability_for_method_and_cac_time(i5_dm_interface_type *pdmif,
	uint8 *pbuf, uint16 *payload_len, uint32 is_edcrs_eu, bool weather, uint8 method);
/* Prepare valid opclass/channel list for the given radio */
static int wbd_slave_find_valid_oclass_chan_info_list(i5_dm_interface_type *pdmif,
	wbd_slave_valid_chan_info_list_t *chan_info);
/* Create active channels(done CAC) list */
static uint8 wbd_slave_prepare_active_list(i5_dm_interface_type *pdmif, uint8 *pbuf, uint16 *len,
	wbd_slave_valid_chan_info_list_t *valid_chan_info, int valid_chan_count);
/* Create radar detected chan list */
static int wbd_slave_prepare_radar_detected_chan_list(uint8 *pbuf, uint16 *len,
	wbd_slave_valid_chan_info_list_t *valid_chan_info, int valid_chan_count);
/* Create ongoing CAC chan list */
static int wbd_slave_prepare_ongoing_cac_chan_list(i5_dm_interface_type *pdmif, uint8 *pbuf);
/* check interface's radio capabiliy to find opclass is valid or not */
static bool wbd_slave_opclass_valid_for_interface(i5_dm_interface_type *pdmif, uint8 opclass);
/* check chan validity from radio capability of interface */
static bool wbd_slave_chan_valid_for_interface(i5_dm_interface_type *pdmif, uint8 opclass,
	uint8 chan);
/* Run the escan for radio */
static void wbd_slave_escan_run_per_radio(bcm_usched_handle *hdl, void *arg);
/* Create New Channel Scan Timer */
static int wbd_slave_add_channel_scan_timer(wbd_ifr_item_t *ifr_vndr_data);
/* Remove Old Channel Scan Timer & If asked, Cleanup the Timer Arg as well */
static void wbd_slave_remove_channel_scan_timer(wbd_ifr_item_t *ifr_vndr_data, bool cleanup_arg);
#endif /* MULTIAPR2 */
/* ------------------------------------ Static Declarations ------------------------------------ */

extern void wbd_exit_slave(wbd_info_t *info);

/* Udate the WBD band enumeration for slave */
int
wbd_slave_update_band_type(char* ifname, wbd_slave_item_t *slave)
{
	int ret = WBDE_OK, bridge_dgt;
	int band = WBD_BAND_LAN_INVALID;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(slave, WBDE_INV_ARG);

	/* Get Bridge from interface name */
	ret = wbd_get_ifname_bridge(ifname, &bridge_dgt);
	WBD_ASSERT();

	/* Get WBD Band Enumeration from ifname's Chanspec & Bridge Type */
	ret = wbd_identify_wbd_band_type(slave->wbd_ifr.chanspec, &band);
	WBD_ASSERT();

	/* Update the band */
	slave->band = band;

end:
	WBD_INFO("ifname[%s] Bridge[%d] Chanspec[0x%x] WBD_BAND[%d]\n",
		ifname, bridge_dgt, slave ? slave->wbd_ifr.chanspec : 0x00,
		slave ? slave->band : WBD_BAND_LAN_INVALID);
	WBD_EXIT();
	return ret;

}

int
wbd_slave_check_taf_enable(wbd_slave_item_t* slave)
{
	int taf_enable = 0;
	WBD_ENTER();

	taf_enable = blanket_get_config_val_int(slave->wbd_ifr.prefix, NVRAM_TAF_ENABLE, 0);
	taf_enable ? wbd_slave_get_taf_conf(slave): taf_enable;

	WBD_EXIT();
	return taf_enable;
}

static int
wbd_slave_get_taf_defprio(wbd_slave_item_t* slave)
{
	int ret = WBDE_OK;
	char keybuf[WBD_MAX_BUF_16];
	const char* keyfmt = WBD_TAF_DEF_FMT;
	char *val = NULL;
	WBD_ENTER();

	if (!slave->taf_info) {
		wbd_ds_slave_free_taf_list(slave->taf_info);
		goto end;
	}
	memset(keybuf, 0, sizeof(keybuf));
	snprintf(keybuf, sizeof(keybuf), keyfmt);

	val = blanket_nvram_safe_get(keybuf);
	if (!strcmp(val, "")) {
		WBD_DEBUG("No toa-def Nvram entries present \n");
		goto end;
	}

	slave->taf_info->pdef = (char*)wbd_malloc(WBD_MAX_BUF_128, &ret);
	WBD_ASSERT_ARG(slave->taf_info->pdef, WBDE_INV_ARG);

	memcpy(slave->taf_info->pdef, val, strlen(val)+1);
end:
	WBD_EXIT();
	return ret;
}

static int
wbd_slave_get_taf_list(wbd_taf_list_t* list, const char* keyfmt)
{
	int ret = WBDE_OK;
	char token[WBD_MAX_BUF_64];
	char keybuf[WBD_MAX_BUF_16];
	int i;
	char *val = NULL;
	char **ptr = NULL;
	int index = 0;
	WBD_ENTER();
#define WBD_MAX_STAPRIO		10 /* Hard Set at present */

	ptr = (char**)wbd_malloc((sizeof(char*) * WBD_MAX_STAPRIO), &ret);
	WBD_ASSERT();

	list->pStr = ptr;

	for (i = 0; i < WBD_MAX_STAPRIO; i++) {
		snprintf(keybuf, sizeof(keybuf), keyfmt, (i+1));
		val = blanket_nvram_safe_get(keybuf);
		if (!strlen(val)) {
			WBD_DEBUG("NO Entry for %s \n", keybuf);
			break;
		}
		memset(token, 0, sizeof(token));
		memcpy(token, val, strlen(val));
		list->pStr[index] = (char*)wbd_malloc(WBD_MAX_BUF_128, &ret);
		WBD_ASSERT();
		memcpy(list->pStr[index], token, sizeof(token));
		list->count++;
		index++;
	}

end:
	WBD_EXIT();
	return ret;
}

static int
wbd_slave_get_taf_conf(wbd_slave_item_t* slave)
{
	int ret = WBDE_OK;
	WBD_ENTER();

	slave->taf_info = (wbd_taf_params_t*)wbd_malloc
		(sizeof(wbd_taf_params_t), &ret);

	WBD_ASSERT_MSG("Slave["MACF"] Failed to allocate memory\n",
		ETHER_TO_MACF(slave->wbd_ifr.mac));

	ret =  wbd_slave_get_taf_list(&(slave->taf_info->sta_list), WBD_TAF_STA_FMT);
	if (ret != WBDE_OK) {
		WBD_DEBUG("NO TAF-STA specific NVRAMS \n");
		if (ret == WBDE_MALLOC_FL) {
			wbd_ds_slave_free_taf_list(slave->taf_info);
			slave->taf_info = NULL;
			goto end;
		}
	}

	ret =  wbd_slave_get_taf_list(&(slave->taf_info->bss_list), WBD_TAF_BSS_FMT);
	if (ret != WBDE_OK) {
		WBD_DEBUG("NO TAF-BSS specific NVRAMS \n");
		if (ret == WBDE_MALLOC_FL) {
			wbd_ds_slave_free_taf_list(slave->taf_info);
			slave->taf_info = NULL;
			goto end;
		}
	}

	ret =  wbd_slave_get_taf_defprio(slave);
end:
	WBD_EXIT();
	return ret;
}

#if !defined(MULTIAPR2)
/* Get interface specific chan_info for all available channels */
int
wbd_slave_get_chan_info(char* ifname, wbd_interface_chan_info_t* wbd_chan_info, int index_size)
{
	uint bitmap, channel;
	int ret = WBDE_OK, iter = 0;
	WBD_ENTER();

	WBD_ASSERT_ARG(wbd_chan_info, WBDE_INV_ARG);

	wbd_chan_info->count = 0;

	for (channel = 1; channel <= CHANNEL_5GH_LAST; channel++) {
		bitmap = 0x00;
		/* Get Interface specific chan_info.
		 * This function is used only in MAP Profile 1, so consider only 2G and 5G bands.
		 * 6G band is not supported in profile 1.
		 */
		ret = blanket_get_chan_info(ifname, channel,
			WL_CHANNEL_2G5G_BAND(channel), &bitmap);
		WBD_ASSERT();

		/* Extract timestamo information */
		bitmap &= WBD_CHAN_INFO_BITMAP_MASK;

		if (!(bitmap & WL_CHAN_VALID_HW)) {
			/* Invalid channel */
			continue;
		}

		if (!(bitmap & WL_CHAN_VALID_SW)) {
			/* Not supported in current locale */
			continue;
		}

		/* Prevent Buffer overflow */
		if (iter < index_size) {
			wbd_chan_info->chinfo[iter].channel = (uint8)channel;
			/* Exclude the Minute information from bitmap */
			wbd_chan_info->chinfo[iter].bitmap = bitmap;
			iter++;
		} else {
			break;
		}
	}

	wbd_chan_info->count = iter;
	/* Just to print the channel info */
	if (WBD_WL_DUMP_ENAB) {
		WBD_DEBUG("ifname[%s] index_size[%d] count[%d] and [channel, bitmap] [ ", ifname,
			index_size, wbd_chan_info->count);
		for (iter = 0; iter < wbd_chan_info->count; iter++) {
			WBD_DEBUG("[%d, 0x%x] ", wbd_chan_info->chinfo[iter].channel,
				wbd_chan_info->chinfo[iter].bitmap);
		}
		WBD_DEBUG(" ]\n");
	}

end:
	WBD_EXIT();
	return ret;
}
#endif /* !MULTIAPR2 */

/* Allocates memory for stamon maclist */
static bcm_stamon_maclist_t*
wbd_slave_alloc_stamon_maclist_struct(int ncount)
{
	int ret = WBDE_OK, buflen;
	bcm_stamon_maclist_t *tmp = NULL;
	WBD_ENTER();

	buflen = sizeof(bcm_stamon_maclist_t) + (sizeof(bcm_stamon_macinfo_t) * (ncount - 1));

	tmp = (bcm_stamon_maclist_t*)wbd_malloc(buflen, &ret);
	WBD_ASSERT();

	tmp->count = ncount;
end:
	WBD_EXIT();
	return tmp;
}

/* Delete a MAC from STAMON */
static int
wbd_slave_remove_sta_fm_stamon(wbd_slave_item_t *slave_item, struct ether_addr *mac,
	BCM_STAMON_STATUS *outstatus)
{
	int ret = WBDE_OK;
	BCM_STAMON_STATUS status = BCM_STAMONE_OK;
	bcm_stamon_maclist_t *tmp = NULL;
	WBD_ENTER();

	ret = wbd_slave_prepare_stamon_maclist(mac, &tmp);
	if (ret != WBDE_OK && !tmp) {
		WBD_WARNING("Band[%d] Slave["MACF"] MAC["MACF"]. Failed to prepare stamon maclist. "
			"Error : %s\n", slave_item->band, ETHER_TO_MACF(slave_item->wbd_ifr.mac),
			ETHERP_TO_MACF(mac), wbderrorstr(ret));
		goto end;
	}

	status = bcm_stamon_command(slave_item->stamon_hdl, BCM_STAMON_CMD_DEL, (void*)tmp, NULL);
	if (status != BCM_STAMONE_OK) {
		ret = WBDE_STAMON_ERROR;
		goto end;
	}

end:
	if (outstatus)
		*outstatus = status;

	if (tmp) {
		free(tmp);
	}
	WBD_EXIT();
	return ret;
}

/* Add MAC address and Priority to stamon maclist on index */
static int
wbd_slave_add_sta_to_stamon_maclist(bcm_stamon_maclist_t *list, struct ether_addr *mac,
	bcm_stamon_prio_t priority, chanspec_t chspec, int idx,
	bcm_offchan_sta_cbfn *cbfn, void *arg)
{
	WBD_ENTER();

	list->macinfo[idx].priority = priority;
	list->macinfo[idx].chspec = chspec;
	list->macinfo[idx].arg = arg;
	list->macinfo[idx].cbfn = cbfn;
	memcpy(&list->macinfo[idx].ea, mac, sizeof(list->macinfo[idx].ea));
	idx++;

	WBD_EXIT();
	return idx;
}

static void wbd_slave_offchan_sta_cb(void *arg, struct ether_addr *ea)
{
	/* Intentionally left blank for 1905, so that existing routine
	 * bcm_stamon_add_stas_to_driver in bcm stamon library
	 * able to add sta with chanspec different from agent's chanspec
	 * to sta monitor module with non zero off chan timer.
	 */
}

/* Retry the STEER */
static void
wbd_slave_steer_retry_timer_cb(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK;
	uint8 free_cb_data = 1;
	i5_dm_bss_type *i5_bss;
	wbd_slave_item_t *slave;
	wbd_assoc_sta_item_t* sta = NULL;
	i5_dm_clients_type* i5_assoc_sta = NULL;
	wbd_slave_steer_retry_arg_t* steer_retry_arg = NULL;
	wl_wlif_bss_trans_data_t *ioctl_data;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(arg, WBDE_INV_ARG);

	steer_retry_arg = (wbd_slave_steer_retry_arg_t*)arg;
	ioctl_data = (wl_wlif_bss_trans_data_t*)&steer_retry_arg->resp_cb_data->ioctl_data;
	slave = (wbd_slave_item_t*)steer_retry_arg->slave;

	/* Find I5 BSS based on MAC address */
	WBD_DS_GET_I5_SELF_BSS((unsigned char*)&slave->wbd_ifr.bssid, i5_bss, &ret);
	WBD_ASSERT_MSG("BSS["MACF"] STA["MACF"] %s\n",
		ETHER_TO_MACF(slave->wbd_ifr.bssid),
		ETHER_TO_MACF(ioctl_data->addr), wbderrorstr(ret));

	/* Retrive the sta from slave's assoclist */
	i5_assoc_sta = wbd_ds_find_sta_in_bss_assoclist(i5_bss, &ioctl_data->addr, &ret, &sta);
	if (!i5_assoc_sta) {
		WBD_INFO("Slave["MACF"] STA["MACF"] Failed to find "
			"STA in assoclist. Error : %s\n",
			ETHER_TO_MACF(slave->wbd_ifr.bssid),
			ETHER_TO_MACF(ioctl_data->addr), wbderrorstr(ret));
		goto end;
	}

	/* CSTYLED */
	if (timercmp(&i5_assoc_sta->assoc_tm, &steer_retry_arg->assoc_time, !=)) {
		WBD_INFO("Slave["MACF"] STA["MACF"] STAAssocTime["TIMEVALF"] "
			"TimerAssocTime["TIMEVALF"]. STA Assoc Time is not matching with the "
			"assoc time from timer created\n",
			ETHER_TO_MACF(slave->wbd_ifr.mac),
			ETHER_TO_MACF(ioctl_data->addr),
			TIMEVAL_TO_TIMEVALF(i5_assoc_sta->assoc_tm),
			TIMEVAL_TO_TIMEVALF(steer_retry_arg->assoc_time));
		goto end;
	}

	/* If the STA status is NORMAL or WEAK or STRONG, No need to retry STEER */
	if (sta->status == WBD_STA_STATUS_NORMAL || sta->status == WBD_STA_STATUS_WEAK ||
		sta->status == WBD_STA_STATUS_STRONG) {
		WBD_INFO("Slave["MACF"] STA["MACF"] Status[0x%x] is normal or weak or strong. "
			"So no need to send weak/strong client\n",
			ETHER_TO_MACF(slave->wbd_ifr.mac),
			ETHER_TO_MACF(ioctl_data->addr), sta->status);
		goto end;
	}

	/* For the 1st time directly issue the BSS transition request. Next time onwards,
	 * issue the WEAK_CLIENT so that let it find the TBSS
	 */
	if (sta->steer_fail_count == 1) {
		steer_retry_arg->resp_cb_data->flags |= WBD_STEER_RESP_CB_FLAGS_RETRY;

#ifdef BCM_APPEVENTD
		/* Send steer start event to appeventd. */
		{
			int rssi = WBD_RCPI_TO_RSSI(i5_assoc_sta->link_metric.rcpi);
			wbd_appeventd_steer_start(APP_E_WBD_SLAVE_STEER_START,
				slave->wbd_ifr.ifr.ifr_name,
				&ioctl_data->addr,
				&ioctl_data->bssid,
				rssi, 0);
		}
#endif /* BCM_APPEVENTD */

		ret = wl_wlif_do_bss_trans(slave->wlif_hdl, ioctl_data);
		WBD_INFO("Band[%d] In BSS "MACF" Steer STA "MACF" to BSSID "MACF" %ssent. "
			"ret=%d\n", slave->band, ETHER_TO_MACF(slave->wbd_ifr.bssid),
			ETHER_TO_MACF(ioctl_data->addr), ETHER_TO_MACF(ioctl_data->bssid),
			((ret != 0) ? "Not " : ""), ret);
		/* If the BTM request sent successfully, do not free the callback data because,
		 * it is required to process the response
		 */
		if (ret == 0) {
			free_cb_data = 0;
		}
		goto end;
	}

	/* Send WEAK_CLIENT command and Update STA Status = Weak */
	ret = wbd_slave_send_weak_strong_client_cmd(i5_bss, i5_assoc_sta, WBD_STA_STATUS_WEAK);

end:
	if (steer_retry_arg) {
		if (free_cb_data) {
			free(steer_retry_arg->resp_cb_data);
			steer_retry_arg->resp_cb_data = NULL;
		}
		free(steer_retry_arg);
		steer_retry_arg = NULL;
	}

	WBD_EXIT();
}

/* Calculate timeout to retry the STEER. The series of time will be ex: 5, 10, 20, 40 ... */
static uint32
wbd_slave_get_steer_retry_timeout(int tm_gap, wbd_assoc_sta_item_t *sta)
{
	/* First retry use initial timeout and store it in steer_retry_timeout variable */
	if (sta->steer_fail_count == 1) {
		sta->steer_retry_timeout = tm_gap;
	} else {
		/* Double of previous timeout */
		sta->steer_retry_timeout *= 2;
	}

	return sta->steer_retry_timeout;
}

/* Create the timer to retry the STEER again */
int
wbd_slave_create_steer_retry_timer(wbd_slave_item_t *slave, i5_dm_clients_type *sta,
	wbd_slave_steer_resp_cb_data_t *resp_cb_data)
{
	int ret = -1;
	wbd_slave_steer_retry_arg_t *param = NULL;
	wbd_assoc_sta_item_t *assoc_sta;
	wbd_info_t *info = slave->parent->parent;
	uint32 timeout = 0;
	WBD_ENTER();

	assoc_sta = (wbd_assoc_sta_item_t*)sta->vndr_data;
	if (assoc_sta == NULL) {
		WBD_WARNING("STA["MACDBG"] NULL Vendor Data\n", MAC2STRDBG(sta->mac));
		goto end;
	}

	WBD_INFO("Slave["MACF"] STA["MACDBG"] RetryCount[%d] TmGap[%d] SteerFailCount[%d] "
		"Create STEER Retry Timer\n",
		ETHER_TO_MACF(slave->wbd_ifr.mac), MAC2STRDBG(sta->mac),
		info->steer_retry_config.retry_count, info->steer_retry_config.tm_gap,
		assoc_sta->steer_fail_count);

	/* If -1(Infinite) Always create timer to retry */
	if (info->steer_retry_config.retry_count != -1) {
		/* check if the retry count is exceeded or not */
		if (assoc_sta->steer_fail_count > info->steer_retry_config.retry_count) {
			WBD_INFO("Slave["MACF"] STA["MACDBG"] STEER Retry count exceeded\n",
				ETHER_TO_MACF(slave->wbd_ifr.mac), MAC2STRDBG(sta->mac));
			goto end;
		}
	}

	param = (wbd_slave_steer_retry_arg_t*)wbd_malloc(sizeof(*param), &ret);
	WBD_ASSERT_MSG("Slave["MACF"] STA["MACDBG"] Failed to allocate STEER_RETRY param\n",
		ETHER_TO_MACF(slave->wbd_ifr.mac), MAC2STRDBG(sta->mac));

	param->slave = slave;
	memcpy(&param->assoc_time, &sta->assoc_tm, sizeof(param->assoc_time));
	param->resp_cb_data = resp_cb_data;

	/* Get timeout to retry the STEER */
	timeout = wbd_slave_get_steer_retry_timeout(info->steer_retry_config.tm_gap, assoc_sta);

	/* Create a timer */
	ret = wbd_add_timers(info->hdl, param, WBD_SEC_MICROSEC(timeout),
		wbd_slave_steer_retry_timer_cb, 0);
	WBD_ASSERT_MSG("Slave["MACF"] STA["MACDBG"] Interval[%d] Failed to create "
		"STEER Retry timer\n",
		ETHER_TO_MACF(slave->wbd_ifr.mac), MAC2STRDBG(sta->mac), timeout);
	ret = WBDE_OK;

end: /* Check Slave Pointer before using it below */

	WBD_EXIT();
	return ret;
}

/* Creates maclist structure for input to stamon */
static int
wbd_slave_prepare_stamon_maclist(struct ether_addr *mac,
	bcm_stamon_maclist_t **stamonlist)
{
	int ret = WBDE_OK;
	bcm_stamon_maclist_t *tmp = NULL;
	WBD_ENTER();

	tmp = wbd_slave_alloc_stamon_maclist_struct(1);
	if (!tmp) {
		ret = WBDE_MALLOC_FL;
		goto end;
	}

	tmp->macinfo[0].priority = BCM_STAMON_PRIO_MEDIUM;
	memcpy(&tmp->macinfo[0].ea, mac, sizeof(tmp->macinfo[0].ea));

	if (stamonlist) {
		*stamonlist = tmp;
	}

end:
	WBD_EXIT();
	return ret;
}

/* Get STA info from driver */
static int
wbd_slave_fill_sta_stats(char *ifname, struct ether_addr* addr, wbd_wl_sta_stats_t *wbd_sta)
{
	/* At present gets stats for Assoc sta only */
	int ret = WBDE_OK;
	sta_info_t sta_info;

	ret = blanket_get_rssi(ifname, addr, &wbd_sta->rssi);
	WBD_ASSERT();

	/* get other stats from STA_INFO iovar */
	ret = blanket_get_sta_info(ifname, addr, &sta_info);
	WBD_ASSERT();

	wbd_sta->tx_rate = (dtoh32(sta_info.tx_rate) / 1000);

end:
	return ret;
}

/* Callback to send ACTION frame to STA */
static void
wbd_slave_actframe_timer_cb(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK;
	wbd_actframe_arg_t* param = NULL;
	maclist_t *maclist = NULL;
	bool sta_found_in_assoclist = FALSE;
	uint count = 0;
	wbd_assoc_sta_item_t* assoc_sta = NULL;
	i5_dm_clients_type *i5_assoc_sta;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(arg, WBDE_INV_ARG);

	param = (wbd_actframe_arg_t*)arg;

	maclist = (maclist_t*)wbd_malloc(WLC_IOCTL_MAXLEN, &ret);
	WBD_ASSERT();

	ret = blanket_get_assoclist(param->slave->wbd_ifr.ifr.ifr_name, maclist, WLC_IOCTL_MAXLEN);

	/* check if mac in param still in mac list, else remove the timer and exit */
	for (count = 0; count < maclist->count; count++) {
		if (!memcmp(&(param->sta_mac), &(maclist->ea[count]), sizeof(param->sta_mac))) {
			sta_found_in_assoclist = TRUE;
			break;
		}
	}

	if (sta_found_in_assoclist) {
		ret = wbd_wl_actframe_to_sta(param->slave->wbd_ifr.ifr.ifr_name,
			&(param->sta_mac));
		param->af_count--;
	}
	if (!sta_found_in_assoclist || (param->af_count <= 0)) {
		/* stop the timer */
		wbd_remove_timers(param->slave->parent->parent->hdl,
			wbd_slave_actframe_timer_cb, param);
		WBD_DEBUG("Band[%d] Slave["MACF"] STA["MACF"] , deleting "
			"action frame timer\n", param->slave->band,
			ETHER_TO_MACF(param->slave->wbd_ifr.mac), ETHER_TO_MACF(param->sta_mac));

		i5_assoc_sta = wbd_ds_find_sta_in_bss_assoclist(param->i5_bss,
			&param->sta_mac, &ret, &assoc_sta);
		if (i5_assoc_sta) {
			assoc_sta->is_offchan_actframe_tm = 0;
		}
		free(param);
		param = NULL;
	}
end:
	if (maclist) {
		free(maclist);
		maclist = NULL;
	}
	WBD_EXIT();
}

/* Send error beacon report */
static void
wbd_slave_map_send_beacon_report_error(wbd_info_t *info, uint8 resp, unsigned char *al_mac,
	unsigned char *sta_mac)
{
	ieee1905_beacon_report report;

	memset(&report, 0, sizeof(report));
	memcpy(&report.neighbor_al_mac, al_mac, sizeof(report.neighbor_al_mac));
	memcpy(&report.sta_mac, sta_mac, sizeof(report.sta_mac));
	report.response = resp;
	ieee1905_send_beacon_report(&report);
	wbd_ds_remove_beacon_report(info, (struct ether_addr *)sta_mac);
}

/* Timer callback to Send beacon report */
static void
wbd_slave_map_send_beacon_report_cb(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK;
	wbd_slave_map_beacon_report_arg_t* param = NULL;
	wbd_beacon_reports_t *report;
	ieee1905_beacon_report map_report;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(arg, WBDE_INV_ARG);

	param = (wbd_slave_map_beacon_report_arg_t*)arg;
	WBD_INFO("STA["MACF"] Beacon report send\n", ETHER_TO_MACF(param->sta_mac));

	if ((report = wbd_ds_find_item_fm_beacon_reports(param->info, &param->sta_mac,
		&ret)) == NULL) {
		WBD_INFO("STA["MACF"] Beacon report not found\n", ETHER_TO_MACF(param->sta_mac));
		goto end;
	}

	if (report->report_element_count <= 0) {
		wbd_slave_map_send_beacon_report_error(param->info,
			IEEE1905_BEACON_REPORT_RESP_FLAG_NO_REPORT,
			(unsigned char*)&report->neighbor_al_mac,
			(unsigned char*)&report->sta_mac);
		goto end;
	}

	/* Report elements present so send it */
	memset(&map_report, 0, sizeof(map_report));
	memcpy(&map_report.neighbor_al_mac, &report->neighbor_al_mac,
		sizeof(map_report.neighbor_al_mac));
	memcpy(&map_report.sta_mac, &report->sta_mac, sizeof(map_report.sta_mac));
	map_report.response = IEEE1905_BEACON_REPORT_RESP_FLAG_SUCCESS;
	map_report.report_element_count = report->report_element_count;
	map_report.report_element_len = report->report_element_len;
	map_report.report_element = report->report_element;
	ieee1905_send_beacon_report(&map_report);
end:
	if (param) {
		free(param);
	}
	WBD_EXIT();
}

/* Callback to send beacon request frame to STA */
static void
wbd_slave_bcn_req_timer_cb(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK;
	wbd_bcn_req_arg_t *bcn_arg = NULL;
	int timeout = 0;
	bcnreq_t *bcnreq;
	int token = 0;
	blanket_bcnreq_t bcnreq_extn;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(arg, WBDE_INV_ARG);

	bcn_arg = (wbd_bcn_req_arg_t *)arg;

	bcnreq_extn = bcn_arg->bcnreq_extn;
	bcnreq = &bcnreq_extn.bcnreq;

	if (bcn_arg->curr_chan_idx < bcn_arg->chan_count) {
		bcnreq->channel = bcn_arg->chan[bcn_arg->curr_chan_idx];
		bcnreq_extn.opclass = bcn_arg->opclass[bcn_arg->curr_chan_idx];
		bcn_arg->curr_chan_idx++;
		WBD_DEBUG("Beacon request for channel %d curr_chan_idx[%d] opclass %d total[%d]\n",
			bcnreq->channel, bcn_arg->curr_chan_idx, bcnreq_extn.opclass,
			bcn_arg->chan_count);
	}

	ret = blanket_send_beacon_request(bcn_arg->ifname, &bcnreq_extn, bcn_arg->subelement,
		bcn_arg->subelem_len, &token);
	if (ret != WBDE_OK) {
		WBD_WARNING("Ifname[%s] STA["MACF"] Beacon request failed\n",
			bcn_arg->ifname, ETHER_TO_MACF(bcnreq->da));
		goto end;
	}

	WBD_INFO("Ifname[%s] STA["MACF"] Token[%d] channel[%d]\n", bcn_arg->ifname,
		ETHER_TO_MACF(bcnreq->da), token, bcnreq->channel);

	if (bcn_arg->curr_chan_idx >= bcn_arg->chan_count) {
		/* Don't schedule the timer since this is the last channel
		 * on which beacon request needs to be sent
		*/
		if (bcn_arg && bcn_arg->chan) {
			free(bcn_arg->chan);
		}

		if (bcn_arg && bcn_arg->opclass) {
			free(bcn_arg->opclass);
		}

		if (bcn_arg) {
			free(bcn_arg);
		}

		goto end;
	}

	/* There is no function available to update timer. remove_flag is set only
	 * after the completion of this callback. So, remove and add the timer with
	 * new timeout value, which will serve the purpose of update timer
	 */
	wbd_remove_timers(bcn_arg->info->hdl, wbd_slave_bcn_req_timer_cb, bcn_arg);

	timeout = WBD_MIN_BCN_REQ_DELAY + bcnreq->dur;

	ret = wbd_add_timers(bcn_arg->info->hdl, bcn_arg, WBD_MSEC_USEC(timeout),
		wbd_slave_bcn_req_timer_cb, 0);

end:
	if (ret != WBDE_OK) {
		if (bcn_arg && bcn_arg->chan) {
			free(bcn_arg->chan);
		}

		if (bcn_arg && bcn_arg->opclass) {
			free(bcn_arg->opclass);
		}

		if (bcn_arg) {
			free(bcn_arg);
		}
	}

	WBD_EXIT();
	return;
}

int
wbd_slave_is_backhaul_sta_associated(char *ifname, struct ether_addr *out_bssid)
{
	int err = WBDE_OK;
	sta_info_t sta_info;
	struct ether_addr cur_bssid;

	err = blanket_get_bssid(ifname, &cur_bssid);
	if (err == WBDE_OK) {
		memset(&sta_info, 0, sizeof(sta_info));
		err = blanket_get_sta_info(ifname, &cur_bssid, &sta_info);
	}

	/* The STA association is complete if authorized flag is set in sta_info */
	if (err == WBDE_OK && (sta_info.flags & WL_STA_AUTHO)) {
		WBD_INFO("Backhaul STA interface %s assoicated on ["MACF"]\n",
			ifname, ETHER_TO_MACF(cur_bssid));
		if (out_bssid) {
			memcpy(out_bssid, &cur_bssid, sizeof(cur_bssid));
		}
		return TRUE;
	}
	return FALSE;
}

/* Remove BSSID NVRAMs from all the backhaul STA interface except the ifname */
static void
wbd_slave_unset_other_bsta_interfaces_bssid(char *ifname)
{
	bool is_bssid_set = FALSE;
	int ret = WBDE_OK;
	i5_dm_device_type *i5_self_dev;
	i5_dm_interface_type *i5_iter_ifr;
	WBD_ENTER();

	WBD_SAFE_GET_I5_SELF_DEVICE(i5_self_dev, &ret);

	/* Go through each interface */
	foreach_i5glist_item(i5_iter_ifr, i5_dm_interface_type, i5_self_dev->interface_list) {

		/* Only for STA interface */
		if (!I5_IS_BSS_STA(i5_iter_ifr->mapFlags)) {
			continue;
		}

		if (strcmp(i5_iter_ifr->ifname, ifname) == 0) {
			continue;
		}

		blanket_nvram_prefix_unset(i5_iter_ifr->prefix, NVRAM_BSSID);
		is_bssid_set = TRUE;
		WBD_INFO("ifname %s prefix %s unset %s\n", i5_iter_ifr->ifname,
			i5_iter_ifr->prefix, NVRAM_BSSID);
	}

	/* Commit only once for all the NVRAM unset */
	if (is_bssid_set) {
		nvram_commit();
	}

end:
	WBD_EXIT();
}

void
wbd_slave_check_bh_join_timer_cb(bcm_usched_handle *hdl, void *arg)
{
	wbd_bh_steer_check_arg_t *param = NULL;
	wbd_best_bsta_ident_info_t *bsta_info;
	struct ether_addr cur_bssid;
	WBD_ENTER();

	/* Validate arg */
	if (!arg) {
		WBD_ERROR("Invalid argument\n");
		goto end;
	}

	param = (wbd_bh_steer_check_arg_t *)arg;
	if (!param->bh_steer_msg) {
		WBD_ERROR("Steering parameters NULL\n");
		free(param);
		goto end;
	}

	param->check_cnt--;

	if (wbd_slave_is_backhaul_sta_associated(param->ifname, &cur_bssid) &&
		(eacmp(&cur_bssid, &param->bh_steer_msg->trgt_bssid) == 0)) {
		/* If the neighbor device not found no point in sending response */
		if (!i5DmDeviceFind(param->bh_steer_msg->neighbor_al_mac)) {
			WBD_INFO("Device ["MACDBG"] which sent backhaul steering request not "
				"found. So, send the response after some time\n",
				MAC2STRDBG(param->bh_steer_msg->neighbor_al_mac));
			goto end;
		}
		param->bh_steer_msg->resp_status_code = 0; /* Success */
		/* Store the BSSID */
		wbd_slave_store_bssid_nvram(param->prefix, param->bh_steer_msg->trgt_bssid, 1);
		wbd_slave_unset_other_bsta_interfaces_bssid(param->ifname);
	} else if (param->check_cnt <= 0) {
		param->bh_steer_msg->resp_status_code = 1; /* Auth or Assoc failed */
	} else {
		goto end;
	}
	ieee1905_send_bh_steering_repsonse(param->bh_steer_msg);
	wbd_remove_timers(hdl, wbd_slave_check_bh_join_timer_cb, param);
	free(param->bh_steer_msg);
	free(param);

	bsta_info = &wbd_get_ginfo()->wbd_slave->best_bsta_info;
	memset(bsta_info->bh_steer_bssid, 0x00, sizeof(bsta_info->bh_steer_bssid));
	memset(bsta_info->bh_steer_sta, 0x00, sizeof(bsta_info->bh_steer_sta));
end:
	WBD_EXIT();
}

/* Process backhaul STA disassociation event */
static void
wbd_slave_process_bhsta_link_down_event(i5_dm_interface_type *i5_ifr)
{
	i5_dm_device_type *i5_device;
	i5_dm_interface_type *i5_upstream_ifr;

	ieee1905_bSTA_disassociated_from_backhaul_ap(i5_ifr->InterfaceId);

	/* If the upstream connection is via ethernet, then no need to
	 * enable roaming in STAs
	 */
	i5_device = i5DmFindController();
	if (i5_device && i5_device->psock) {
		i5_upstream_ifr = i5DmInterfaceFind(i5DmGetSelfDevice(),
			((i5_socket_type*)i5_device->psock)->u.sll.mac_address);
		if (i5_upstream_ifr && !i5DmIsInterfaceWireless(i5_upstream_ifr->MediaType)) {
			/* If upstream connection is via ethernet, then no need to start roam */
			WBD_INFO("Disassociated, but has ethernet connection\n");
			goto end;
		}
	}

	/* Unset the agent configured NVRAM. As this will reconfigure once the backhaul
	 * connection established
	 */
	blanket_nvram_prefix_set(NULL, NVRAM_MAP_AGENT_CONFIGURED, "0");

	wbd_ieee1905_set_bh_sta_params(IEEE1905_BH_STA_ROAM_ENAB_VAP_FOLLOW, NULL);

end:
	return;
}

/* Send unsolicited BTM report to controller */
static void
wbd_slave_send_unsolicited_btm_report(wbd_slave_item_t *slave, char *pkt, int len)
{
	int pdata_len, ret;
	dot11_bsstrans_resp_t *bsstrans_resp;
	dot11_neighbor_rep_ie_t *neighbor = NULL;
	bcm_event_t *dpkt;
	struct ether_addr *addr;
	struct ether_addr *sta_mac;
	ieee1905_btm_report btm_report;
	i5_dm_device_type *i5_controller;

	WBD_ASSERT_ARG(pkt, WBDE_INV_ARG);
	WBD_ASSERT_ARG(slave, WBDE_INV_ARG);

	i5_controller = i5DmFindController();
	if (!i5_controller) {
		WBD_INFO("Controller not found cannot send unsolicited BTM report\n");
		goto end;
	}

	memset(&btm_report, 0, sizeof(btm_report));

	pkt = pkt + IFNAMSIZ;
	pdata_len = len - IFNAMSIZ;

	if (pdata_len <= BCM_EVENT_HEADER_LEN) {
		WBD_WARNING("%s: BTM Response: data_len %d too small\n",
			slave->wbd_ifr.ifr.ifr_name, pdata_len);
		goto end;
	}

	dpkt = (bcm_event_t *)pkt;
	pkt += BCM_EVENT_HEADER_LEN; /* payload (bss response) */
	pdata_len -= BCM_EVENT_HEADER_LEN;
	sta_mac = (struct ether_addr *)(&(dpkt->event.addr));
	bsstrans_resp = (dot11_bsstrans_resp_t *)pkt;
	/* data will have the BSSID if the response is accept(i.e. 0) */
	addr = (struct ether_addr *)(bsstrans_resp->data);

	/* If the BSS transition response is 6, then BSSID field will
	 * be in neighbor report.
	 */
	if (bsstrans_resp->status == DOT11_BSSTRANS_RESP_STATUS_REJ_BSS_LIST_PROVIDED) {
		neighbor = (dot11_neighbor_rep_ie_t *)bsstrans_resp->data;
		addr = &neighbor->bssid;
	}

	WBD_INFO("%s: BTM Response: BSS["MACF"] STA["MACF"] token=%d status=%d ToBSS="MACF"\n",
		slave->wbd_ifr.ifr.ifr_name, ETHER_TO_MACF(slave->wbd_ifr.bssid),
		ETHERP_TO_MACF(sta_mac), bsstrans_resp->token, bsstrans_resp->status,
		ETHERP_TO_MACF(addr));

	btm_report.status = bsstrans_resp->status;
	/* If success then only BSSID field exists */
	if (btm_report.status == DOT11_BSSTRANS_RESP_STATUS_ACCEPT) {
		eacopy(addr, &btm_report.trgt_bssid);
	}

	eacopy(&i5_controller->DeviceId, &btm_report.neighbor_al_mac);
	eacopy(&slave->wbd_ifr.bssid, &btm_report.source_bssid);
	eacopy(sta_mac, &btm_report.sta_mac);
	btm_report.request_flags |= IEEE1905_STEER_FLAGS_MANDATE;
	ieee1905_send_btm_report(&btm_report);

end:
	return;
}

/* Process BSS Transition Response event */
static void
wbd_slave_process_bss_trans_response_event(wbd_info_t* info, char *ifname, char *pkt, int len)
{
	int ret = WBDE_OK;
	wbd_slave_item_t *slave = NULL;

	slave = wbd_ds_find_slave_ifname_in_blanket_slave(info->wbd_slave, ifname, &ret);
	if (slave) {
		/* Update the flag saying BTM report received. Unset it when we receive callback
		 * from steering library. In the end of this function if the flag is still set
		 * then it is unsolicited BTM report. Decide whether to send it to controller or not
		 */
		info->wbd_slave->flags |= WBD_BKT_SLV_FLAGS_BTM_REPORT_RECV;
		wl_wlif_process_bss_trans_resp_event(slave->wlif_hdl, pkt, len);
	} else {
		WBD_INFO("Ifname[%s] received BTM resp event but slave not found\n", ifname);
	}

	if (info->wbd_slave->flags & WBD_BKT_SLV_FLAGS_BTM_REPORT_RECV) {
		wbd_slave_send_unsolicited_btm_report(slave, pkt, len);
		info->wbd_slave->flags &= ~WBD_BKT_SLV_FLAGS_BTM_REPORT_RECV;
	}
}

#if defined(MULTIAPR2)
/* This callback function will get trigger every interval(Minute)
 *  to reset the count value of reporting rate.
 *  Agent should report unsuccessfull association event to controller.
 *  But Agent should not report more than limit/reporting rate within a minute.
 *  So maintaining counter and reset the counter every minute
 */
void
wbd_slave_reset_unsuccessful_assoc_count_timer_cb(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK;
	ieee1905_unsuccessful_assoc_config_t *param = NULL;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(arg, WBDE_INV_ARG);
	param = (ieee1905_unsuccessful_assoc_config_t *)arg;
	param->count = 0;
end:
	WBD_EXIT();
}

/* Get multiap R2 channel bandwidth string for the chanspec */
static char
wbd_slave_get_chscan_bw_string(chanspec_t chanspec, char *bw, unsigned char bw_sz)
{
	if (CHSPEC_IS20(chanspec)) {
		snprintf(bw, bw_sz, "%s", MAP_CH_BW_20);
		return strlen(MAP_CH_BW_20);
	} else if (CHSPEC_IS40(chanspec)) {
		snprintf(bw, bw_sz, "%s", MAP_CH_BW_40);
		return strlen(MAP_CH_BW_40);
	} else if (CHSPEC_IS80(chanspec)) {
		snprintf(bw, bw_sz, "%s", MAP_CH_BW_80);
		return strlen(MAP_CH_BW_80);
	} else if (CHSPEC_IS160(chanspec)) {
		snprintf(bw, bw_sz, "%s", MAP_CH_BW_160);
		return strlen(MAP_CH_BW_160);
	} else if (CHSPEC_IS8080(chanspec)) {
		snprintf(bw, bw_sz, "%s", MAP_CH_BW_8080);
		return strlen(MAP_CH_BW_8080);
	}

	return 0;
}

/* Agent Sends Channel Scan Report to Controller Per Radio */
static void
wbd_slave_send_chscan_report_per_radio(i5_dm_interface_type *evt_ifr,
	ieee1905_per_radio_opclass_list *ifr, unsigned char in_status_code)
{
	i5_dm_device_type *i5_device_cntrl = i5DmFindController();
	ieee1905_chscan_req_msg chscan_req;
	WBD_ENTER();

	/* Initialize Channel Scan Request list from Last Request Object */
	memset(&chscan_req, 0, sizeof(chscan_req));
	chscan_req.chscan_req_msg_flag |= MAP_CHSCAN_REQ_FRESH_SCAN;
	chscan_req.num_of_radios = 1;
	ieee1905_glist_init(&chscan_req.radio_list);
	ieee1905_glist_append(&chscan_req.radio_list, (dll_t*)(ifr));

	WBD_INFO("ifname %s : Sending Channel Scan Report Message to ["MACF"]\n",
		evt_ifr->ifname, ETHERP_TO_MACF(i5_device_cntrl->DeviceId));

	/* Send Channel Scan Report Message to a Multi AP Device */
	ieee1905_send_requested_stored_channel_scan(&chscan_req, in_status_code);

	WBD_EXIT();
}

#define WBD_PRINT_ESCAN_EVT_STATUS(status) \
	(((status) == WLC_E_STATUS_PARTIAL) ? "PARTIAL" : \
	(((status) == WLC_E_STATUS_SUCCESS) ? "SUCCESS" : "ABORTED"))

/* Parse and Process the ESCAN EVENT from EAPD */
static int
wbd_slave_process_event_escan(i5_dm_interface_type *evt_ifr, char *ifname, bcm_event_t *pvt_data)
{
	int ret = WBDE_OK;
	bool scantype_onboot = FALSE, scanstate_running = FALSE;
	uint32 escan_event_status;
	wl_escan_result_t *escan_data = NULL;
	i5_dm_device_type *selfdevice = i5DmGetSelfDevice();
	wbd_ifr_item_t *ifr_vndr_data = NULL;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(ifname, WBDE_INV_ARG);
	WBD_ASSERT_ARG(pvt_data, WBDE_INV_ARG);
	WBD_ASSERT_ARG(evt_ifr, WBDE_INV_ARG);
	WBD_ASSERT_ARG(evt_ifr->vndr_data, WBDE_INV_ARG);

	ifr_vndr_data = (wbd_ifr_item_t*)evt_ifr->vndr_data;
	escan_event_status = ntoh32(pvt_data->event.status);
	escan_data = (wl_escan_result_t*)(pvt_data + 1);

	/* Save Scan Type & State Indicators, as in SUCCESS & ABORT cases, they will change */
	/* Save Scan Type Indicator = OnBoot OR Requested */
	scantype_onboot = I5_IS_CHSCAN_ONBOOT(evt_ifr->flags) ? TRUE : FALSE;
	/* Save Scan State Indicator = Running OR Stopped */
	scanstate_running = I5_IS_CHSCAN_RUNNING(evt_ifr->flags) ? TRUE : FALSE;

	/* If Channel Scan is not initiated by Agent, Skip Processing */
	if (escan_data->sync_id != ifr_vndr_data->chscan_sync_id) {
		WBD_DEBUG("ON ESCAN %s: [%s] [%d]: Skip ESCAN_RESULT Event Processing. "
			"Sync ID Mismatch: Result_sync_id[%d] != Stored_sync_id[%d]\n",
			WBD_PRINT_ESCAN_EVT_STATUS(escan_event_status),
			ifname, escan_data->sync_id,
			escan_data->sync_id, ifr_vndr_data->chscan_sync_id);
		goto end;
	}
	/* If Channel Scan is Disabled on Agent, Skip Processing */
	if (!(wbd_get_ginfo()->wbd_slave->scantype_flags)) {
		WBD_INFO("ON ESCAN %s: [%s] [%d]: Skip ESCAN_RESULT Event Processing. "
			"Channel Scan Disabled.\n",
			WBD_PRINT_ESCAN_EVT_STATUS(escan_event_status),
			ifname, escan_data->sync_id);
		goto end;
	}

	/* Escan not finished */
	if (escan_event_status == WLC_E_STATUS_PARTIAL) {

		wl_bss_info_t *bi = &escan_data->bss_info[0];
		ieee1905_chscan_result_item *chscan_result;
		ieee1905_chscan_result_nbr_item *nbr_bss;
		char ssidbuf[SSID_FMT_BUF_LEN] = "";
		uint8 bi_opClass = 0;

		blanket_get_global_rclass(CH20MHZ_CHSPEC(bi->ctl_ch,
				CHSPEC_BAND(bi->chanspec)), &bi_opClass);

		WBD_DEBUG("ON ESCAN PARTIAL: [%s] [%d]: Result MAC["MACDBG"] BSSID["MACF"] "
			"RSSI[%d] ctrl_chan[%d] bi_opClass[%d]\n",
			ifname, escan_data->sync_id,
			MAC2STRDBG(evt_ifr->InterfaceId), ETHER_TO_MACF(bi->BSSID),
			bi->RSSI, bi->ctl_ch, bi_opClass);

		if (bi->RSSI == WLC_RSSI_INVALID) {
			WBD_DEBUG("\n\nInvalid RSSI\n");
			goto end;
		}

		/* Find the channel number in scan result of a device */
		chscan_result = i5DmFindChannelInScanResult(&selfdevice->stored_chscan_results,
			bi_opClass, bi->ctl_ch);

		if (chscan_result == NULL) {
			WBD_DEBUG("ON ESCAN PARTIAL: [%s] [%d]: IFR["MACDBG"] Chan[%d] not "
				"present\n", ifname, escan_data->sync_id,
				MAC2STRDBG(evt_ifr->InterfaceId), bi->ctl_ch);
			goto end;
		}

		/* Check if we've received info of same BSSID */
		nbr_bss = i5DmFindBSSIDInScanResult(chscan_result, (unsigned char*)&bi->BSSID);

		if (!nbr_bss) {
			/* New Neighbor BSS. Allocate memory and save it */
			nbr_bss = (ieee1905_chscan_result_nbr_item*)
				wbd_malloc(sizeof(*nbr_bss), &ret);
			WBD_ASSERT();

			ieee1905_glist_append(&chscan_result->neighbor_list, (dll_t*)nbr_bss);

			chscan_result->num_of_neighbors++;

			WBD_DEBUG("ON ESCAN PARTIAL: [%s] [%d]: New Neighbor BSS IFR["MACDBG"] "
				"BSSID["MACF"] Appended\n", ifname, escan_data->sync_id,
				MAC2STRDBG(evt_ifr->InterfaceId), ETHER_TO_MACF(bi->BSSID));
		}

		/* Update the Neighbor BSS fields */
		memcpy(nbr_bss->nbr_bssid, bi->BSSID.octet, sizeof(nbr_bss->nbr_bssid));
		if ((bi->SSID_len > 0) && (bi->SSID_len <= sizeof(nbr_bss->nbr_ssid.SSID))) {
			memcpy(nbr_bss->nbr_ssid.SSID, bi->SSID, bi->SSID_len);
			nbr_bss->nbr_ssid.SSID_len = bi->SSID_len;
			wbd_wl_format_ssid(ssidbuf, bi->SSID, bi->SSID_len);
		}
		nbr_bss->nbr_rcpi = (unsigned char)(WBD_RSSI_TO_RCPI(bi->RSSI));
		nbr_bss->ch_bw_length = wbd_slave_get_chscan_bw_string(bi->chanspec,
			(char*)nbr_bss->ch_bw, sizeof(nbr_bss->ch_bw));

		/* Fetch ChanUtil & StaCnt, if BSSLoad Element Present. Else omitted */
		if (dtoh32(bi->ie_length) && (blanket_get_qbss_load_element(bi,
			&nbr_bss->channel_utilization, &nbr_bss->station_count) == 0)) {
			nbr_bss->chscan_result_nbr_flag |= MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT;
		} else {
			nbr_bss->chscan_result_nbr_flag = 0;
			nbr_bss->channel_utilization = 0;
			nbr_bss->station_count = 0;
		}

		chscan_result->scan_status_code = MAP_CHSCAN_STATUS_SUCCESS;
		/* 1 : Scan was an Active scan */
		chscan_result->chscan_result_flag |= MAP_CHSCAN_RES_SCANTYPE;

		WBD_DEBUG("ON ESCAN PARTIAL: [%s] [%d]: NBR_BSSID["MACF"] NBR_SSID[%s] "
			"NBR_RCPI[%d (%d)] NBR_CHBW[%s] NBR_SCAN_STATUS[%d]\n",
			ifname, escan_data->sync_id,
			ETHERP_TO_MACF(nbr_bss->nbr_bssid), ssidbuf,
			nbr_bss->nbr_rcpi, WBD_RCPI_TO_RSSI(nbr_bss->nbr_rcpi), nbr_bss->ch_bw,
			chscan_result->scan_status_code);

	/* Escan finished : Success / Aborted */
	} else {

		WBD_INFO("ON ESCAN %s: [%s] [%d]: [%s] : Status[%d]\n",
			WBD_PRINT_ESCAN_EVT_STATUS(escan_event_status),
			ifname, escan_data->sync_id,
			(escan_event_status == WLC_E_STATUS_SUCCESS) ?
			"Escan Success!" : "Misc. Error/Abort",
			escan_event_status);

		/* If Channel Scan is Triggered by SmartMesh Controller */
		if (scanstate_running) {

			/* Unset Flag indicating Running a Requested Channel Scan on this radio */
			I5DmSetChScanRunning(evt_ifr, scantype_onboot ?
				I5_CHSCAN_ONBOOT : I5_CHSCAN_REQ_FRESH, I5_CHSCAN_STOP);

			/* Update Status Code = SUCCESS / ABORTED, of relavant Results
			 * in SelfDevice for a Radio
			 */
			i5DmUpdateChannelScanStatusPerRadio(selfdevice,
				&ifr_vndr_data->last_fresh_chscan_req, -1,
				(escan_event_status == WLC_E_STATUS_SUCCESS) ?
				MAP_CHSCAN_STATUS_SUCCESS : MAP_CHSCAN_STATUS_ABORTED);
		}

		/* Fetch chanim_stats all : Update Channel Util of all Results post Scan */
		wbd_slave_update_chscanresult_chanutil(evt_ifr);

		/* If Result is other than PARTIAL, and it was Requested (not OnBoot) Scan
		 * Send Channel Scan Results to Controller
		 */
		if (!scantype_onboot) {

			/* Agent Sends Channel Scan Report to Controller Per Radio */
			wbd_slave_send_chscan_report_per_radio(evt_ifr,
				&ifr_vndr_data->last_fresh_chscan_req,
				(escan_event_status == WLC_E_STATUS_SUCCESS) ?
				MAP_CHSCAN_STATUS_SUCCESS : MAP_CHSCAN_STATUS_ABORTED);
		}

		/* Reset Sync ID for this Interface, On EScan Success / Aborted */
		ifr_vndr_data->chscan_sync_id = 0;
	}
end:
	WBD_EXIT();
	return ret;
}

/* Extract primary VLAN ID from MAP ie */
static unsigned short
wbd_slave_get_primary_vlan_id_from_map_ie(uint8 *data, uint32 datalen)
{
	multiap_ie_t *map_ie = NULL;
	uint8 ie_type = MAP_IE_TYPE;

	if (wbd_get_ginfo()->map_profile < ieee1905_map_profile2) {
		return 0;
	}
	/* Find multiap IE with WFA_OUI */
	map_ie = (multiap_ie_t*)wbd_wl_find_ie(DOT11_MNG_VS_ID, data, datalen,
			WFA_OUI, &ie_type, 1);
	if (map_ie && map_ie->len > MIN_MAP_IE_LEN) {
		int tlv_len = 0;
		bcm_tlv_t *tlv = NULL;

		tlv_len = map_ie->len;
		for (tlv = (bcm_tlv_t *)map_ie->attr; tlv != NULL;
				tlv = (bcm_tlv_t *)bcm_next_tlv((bcm_tlv_t *)tlv,
					&tlv_len)) {

			if (tlv->id == MAP_8021Q_SETTINGS_SE_ID) {
				multiap_def_8021Q_settings_se_t *set =
					(multiap_def_8021Q_settings_se_t*)tlv;
				return dtoh16(set->prim_vlan_id);
			}
		}
	}
	return 0;
}
#endif /* MULTIAPR2 */

/* Parse and process the EVENT from EAPD */
static int
wbd_slave_process_event_msg(wbd_info_t* info, char* pkt, int len)
{
	int ret = WBDE_OK;
	bcm_event_t *pvt_data;
	uint32 datalen;
	struct ether_addr *sta_mac;
	char *ifname;
	struct ether_header *eth_hdr;
	uint8 *data;
	uint16 ether_type;
	uint32 evt_type, status = 0;
	i5_dm_interface_type *evt_ifr = NULL, *ifr;
	i5_dm_bss_type *evt_bss = NULL;
	i5_dm_device_type *selfdevice = i5DmGetSelfDevice();
	WBD_ENTER();

	ifname = (char *)pkt;
	eth_hdr = (struct ether_header *)(ifname + IFNAMSIZ);

	if ((ether_type = ntohs(eth_hdr->ether_type) != ETHER_TYPE_BRCM)) {
		WBD_WARNING("Ifname[%s] recved non BRCM ether type 0x%x\n", ifname, ether_type);
		ret = WBDE_EAPD_ERROR;
		goto end;
	}

	pvt_data = (bcm_event_t *)(ifname + IFNAMSIZ);
	evt_type = ntoh32(pvt_data->event.event_type);

	/* Special handling of event where we need the wlif_hdl from slave */
	if (evt_type == WLC_E_BSSTRANS_RESP) {
		WBD_INFO("ifname[%s] WLC_E_BSSTRANS_RESP\n", ifname);
		wbd_slave_process_bss_trans_response_event(info, ifname, pkt, len);

		goto end;
	}

	sta_mac = (struct ether_addr *)(&(pvt_data->event.addr));
	data = (uint8 *)(pvt_data + 1);
	datalen = ntoh32(pvt_data->event.datalen);

	foreach_i5glist_item(ifr, i5_dm_interface_type, selfdevice->interface_list) {
		if ((evt_bss = wbd_ds_get_i5_bss_match_ifname_in_ifr(ifr, ifname, &ret))) {
			evt_ifr = ifr;
			break;
		}
		if ((strcmp(ifname, ifr->ifname) == 0)) {
			evt_ifr = ifr;
			break;
		}
	}

	if (evt_ifr == NULL) {
		WBD_INFO("Event[0x%x] ifname[%s]. Ifr Not Present\n", evt_type, ifname);
		ret = WBDE_EAPD_ERROR;
		goto end;
	}

	switch (evt_type) {
		case WLC_E_LINK:
		{
			wbd_slave_item_t *slave = NULL, *iter_slave = NULL;
			dll_t *slave_item_p;

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);

			if (evt_bss) {
				WBD_INFO("Event[0x%x] ifname[%s] not a STA\n", evt_type, ifname);
				break;
			}

			/* We are not interested in link up event */
			if (ntoh16(pvt_data->event.flags) & WLC_EVENT_MSG_LINK) {
				WBD_DEBUG("Ifname[%s] Link UP Event for STA["MACF"]\n",
					ifname, ETHERP_TO_MACF(sta_mac));
				foreach_glist_item(slave_item_p, info->wbd_slave->br0_slave_list) {
					iter_slave = (wbd_slave_item_t*)slave_item_p;
					/* Check if the event is for disabled slave.
					 * if yes fill interface info
					 */
					if (strcmp(ifname, iter_slave->wbd_ifr.ifr.ifr_name) == 0) {
						slave = iter_slave;
						break;
					}
				}
				if (slave && !slave->wbd_ifr.enabled) {
					WBD_INFO("Ifname[%s] slave item ["MACF"]. "
						"fill interface\n", ifname,
						ETHERP_TO_MACF(sta_mac));
					wbd_slave_linkup_fill_interface_info(slave);
				}
				break;
			}

			WBD_INFO("Ifname[%s] Link DOWN Event for STA["MACF"]\n",
				ifname, ETHERP_TO_MACF(sta_mac));

			if (!I5_IS_BSS_STA(evt_ifr->mapFlags)) {
				WBD_INFO("Ifname[%s] Not a backhaul STA\n", ifname);
				break;
			}
			wbd_slave_process_bhsta_link_down_event(evt_ifr);
		}
		break;

		case WLC_E_ASSOC:
		case WLC_E_REASSOC:
		{
			unsigned short prim_vlan_id = 0;
			int is_8021q_present = 0;
			i5_dm_interface_type *i5_tmp_ifr;
			uint32 status, ies_len;
			uint8 *ies;

			status = ntoh32(pvt_data->event.status);

			WBD_INFO("Event[0x%x] ifname[%s] for STA["MACF"]\n",
				evt_type, ifname, ETHERP_TO_MACF(sta_mac));

			if (evt_bss) {
				WBD_INFO("Event[0x%x] ifname[%s] not a STA\n", evt_type, ifname);
				break;
			}

			if (status != WLC_E_STATUS_SUCCESS) {
				WBD_DEBUG("BHSTA ifname[%s] not associated status 0x%x\n",
					ifname, status);
				goto end;
			}

			if (!I5_IS_BSS_STA(evt_ifr->mapFlags)) {
				WBD_INFO("Ifname[%s] Not a backhaul STA\n", ifname);
				break;
			}

			ies = data + DOT11_ASSOC_RESP_FIXED_LEN;
			ies_len = datalen - DOT11_ASSOC_RESP_FIXED_LEN;
#if defined(MULTIAPR2)
			prim_vlan_id = wbd_slave_get_primary_vlan_id_from_map_ie(ies, ies_len);
			if (prim_vlan_id > 0) {
				is_8021q_present = 1;
			}
			WBD_INFO("ifname[%s] Primary VLAN ID[%d] is_8021q_present[%d]\n",
				ifname, prim_vlan_id, is_8021q_present);
#endif /* MULTIAPR2 */

			WBD_INFO("ifname[%s] associated, so disable roam on other interface\n",
				ifname);
			/* Disable roam on other STA interfaces */
			wbd_ieee1905_set_bh_sta_params(IEEE1905_BH_STA_ROAM_DISB_VAP_UP, evt_ifr);

			/* Inform MultiAP module to do renew */
			WBD_INFO("%s joined with vlan %d\n", ifname, prim_vlan_id);
			ieee1905_bSTA_associated_to_backhaul_ap(evt_ifr->InterfaceId, ifname,
				prim_vlan_id, is_8021q_present);

			i5_tmp_ifr = wbd_slave_choose_best_bh_sta();
			if (i5_tmp_ifr) {
				wbd_slave_disconnect_all_bstas(i5_tmp_ifr);
			}
		}
		break;

		/* handle split assoc requests for not allowing repeater to repeater connection */
		case WLC_E_PRE_ASSOC_IND:
		case WLC_E_PRE_REASSOC_IND:
		{
			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);
			ret = wbd_wl_send_assoc_decision(ifname, info->flags, data, len,
				sta_mac, evt_type);
			goto end;
		}
		break;

		case WLC_E_DEAUTH:	/* 5 */
		case WLC_E_DEAUTH_IND: /* 6 */
		case WLC_E_DISASSOC_IND: /* 12 */
		{
			uint32 reason;

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);
			reason = ntoh32(pvt_data->event.reason);
			if (!evt_bss) {
				ret = WBDE_EAPD_ERROR;
				goto end;
			}

			ret = blanket_sta_assoc_disassoc((struct ether_addr *)evt_bss->BSSID,
				sta_mac, 0, 0, 1, NULL, 0, (uint16)reason);
			if (ret != 0) {
				WBD_WARNING("Band[%d] BSS["MACF"] Ifname[%s] Deauth Event[0x%x] "
					"for STA["MACF"]. Failed to Add to MultiAP Error : %d\n",
					evt_ifr->band, ETHERP_TO_MACF(evt_bss->BSSID), ifname,
					evt_type, ETHERP_TO_MACF(sta_mac), ret);
			}
		}
		break;

		/* update sta info list */
		case WLC_E_ASSOC_REASSOC_IND_EXT:
		{
			multiap_ie_t *map_ie = NULL;
			wbd_assoc_sta_item_t *sta = NULL;
			i5_dm_clients_type *i5_assoc_sta = NULL;
#if defined(MULTIAPR2)
			ieee1905_tunnel_msg_t tunnel_msg;
#endif /* MULTIAPR2 */
			bool backhaul_sta = FALSE;
			uint8 ie_type = MAP_IE_TYPE;
			bool reassoc;
			struct dot11_management_header *hdr = (struct dot11_management_header*)data;
			uint8 *frame, *ies;
			uint32 frame_len, ies_len;
			uint32 reason;

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);
			reason = ntoh32(pvt_data->event.reason);
			if (!evt_bss) {
				ret = WBDE_EAPD_ERROR;
				goto end;
			}

			reassoc = ((ltoh16(hdr->fc) & FC_KIND_MASK) == FC_REASSOC_REQ);
			frame = data + sizeof(struct dot11_management_header);
			frame_len = datalen - sizeof(struct dot11_management_header);
			if (reassoc) {
				ies = frame + DOT11_REASSOC_REQ_FIXED_LEN;
				ies_len = frame_len - DOT11_REASSOC_REQ_FIXED_LEN;
			} else {
				ies = frame + DOT11_ASSOC_REQ_FIXED_LEN;
				ies_len = frame_len - DOT11_ASSOC_REQ_FIXED_LEN;
			}

			/* if it is Backhaul_sta, skip */
			map_ie = (multiap_ie_t*)wbd_wl_find_ie(DOT11_MNG_VS_ID, ies, ies_len,
				WFA_OUI, &ie_type, 1);

			if (map_ie && map_ie->len > MIN_MAP_IE_LEN) {
				int tlv_len = 0;
				bcm_tlv_t *tlv = NULL;

				tlv_len = map_ie->len;
				for (tlv = (bcm_tlv_t *)map_ie->attr; tlv != NULL;
					tlv = (bcm_tlv_t *)bcm_next_tlv
					((bcm_tlv_t *)tlv, &tlv_len)) {

					if (tlv->id  == MAP_EXT_ATTR) {
						multiap_ext_attr_t *attr = (multiap_ext_attr_t*)tlv;

						if (attr->attr_val & IEEE1905_BACKHAUL_STA) {
							backhaul_sta = TRUE;
							break;
						}
						 break; /* Only need MAP extension atrribute IE */
					}
				}
			}

			ret = blanket_sta_assoc_disassoc((struct ether_addr *)evt_bss->BSSID,
				sta_mac, 1, 0, 1, frame, frame_len, (uint16)reason);
			if (ret != 0) {
				WBD_WARNING("Band[%d] BSS["MACF"] Ifname[%s] Assoc Event[0x%x] "
					"for STA["MACF"]. Failed to Add to MultiAP Error : %d\n",
					evt_ifr->band, ETHERP_TO_MACF(evt_bss->BSSID), ifname,
					evt_type, ETHERP_TO_MACF(sta_mac), ret);
			}

			if (backhaul_sta) {
				i5_assoc_sta = wbd_ds_find_sta_in_bss_assoclist(evt_bss,
					sta_mac, &ret, &sta);
				if (!i5_assoc_sta) {
					WBD_ERROR("client info in MultiAp Database missing\n");
					goto end;
				}
				/* update backhaul contents */
				i5_assoc_sta->flags |= I5_CLIENT_FLAG_BSTA;
			}
			WBD_INFO("Band[%d] BSS["MACF"] Ifname[%s] %sAssoc Event[0x%x] for "
				"STA["MACF"]. DataLen[%d] FrameLen[%d] IELen[%d]\n",
				evt_ifr->band, ETHERP_TO_MACF(evt_bss->BSSID), ifname,
				reassoc ? "Re" : "", evt_type, ETHERP_TO_MACF(sta_mac),
				datalen, frame_len, ies_len);
#if defined(MULTIAPR2)
			/* send tunnel message to controller for (re)assoc event from
			 * sta
			 */
			memset(&tunnel_msg, 0, sizeof(tunnel_msg));
			WBD_INFO(" rcvd notification frame from sta["MACF"] of len [%d]\n",
				ETHERP_TO_MACF(sta_mac), datalen);

			memcpy(&tunnel_msg.source_mac, sta_mac, ETHER_ADDR_LEN);
			tunnel_msg.payload = frame;
			tunnel_msg.payload_len = frame_len;

			tunnel_msg.payload_type = (reassoc ?
				ieee1905_tunnel_msg_payload_re_assoc_rqst :
				ieee1905_tunnel_msg_payload_assoc_rqst);

			WBD_INFO(" send tunnel msg: payload type[%d] for sta["MACF"] "
				"payload len[%d] \n", tunnel_msg.payload_type,
				ETHERP_TO_MACF(sta_mac), frame_len);

			wbd_slave_send_tunneled_msg(&tunnel_msg);
#endif /* MULTIAPR2 */
		}
		break;

		case WLC_E_RADAR_DETECTED:
		{
			wl_event_radar_detect_data_t *radar_data = NULL;
			wbd_cmd_vndr_zwdfs_msg_t zwdfs_msg;
#if defined(MULTIAPR2)
			wbd_ifr_item_t *ifr_vndr_data = NULL;
#endif /* MULTIAPR2 */

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);
			radar_data = (wl_event_radar_detect_data_t*)data;
			if (!radar_data) {
				WBD_DEBUG(" Invalid event data passed at radar, exit\n");
				goto end;
			}

			evt_ifr->chanspec = radar_data->target_chanspec;

			WBD_INFO("Band[%d] Ifname[%s] RADAR DETECTED, current "
				"chanspec[0x%x], target chanspec[0x%x]\n", evt_ifr->band, ifname,
				radar_data->current_chanspec, radar_data->target_chanspec);

#if defined(MULTIAPR2)
			ifr_vndr_data = (wbd_ifr_item_t*)evt_ifr->vndr_data;
			if (!ifr_vndr_data) {
				WBD_ERROR(" Invalid ifr vendor data plz Debug, exiting ..\n");
				goto end;
			}
			if (ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_RUNNING) {
				ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_COMPLETE;
				ifr_vndr_data->cac_compl_status = MAP_CAC_STATUS_RADAR_DETECTED;
				wbd_slave_create_cac_start_timer(info);
			} else
#endif /* MULTIAPR2 */
			{
				/* prepare zwdfs msg, forward to 1905 to be passed to controller
				 * In R2, this is required only if the previous state was
				 * ISM. Radar detection in PRE-ISM CAC state will be reported via
				 * CAC completion TLV in channel preference report
				 */
				memset(&zwdfs_msg, 0, sizeof(zwdfs_msg));
				blanket_get_global_rclass(radar_data->target_chanspec,
						&(zwdfs_msg.opclass));
				zwdfs_msg.cntrl_chan =
					wf_chspec_ctlchan(radar_data->target_chanspec);
				zwdfs_msg.reason = WL_CHAN_REASON_CSA;
				memcpy(&zwdfs_msg.mac, evt_ifr->InterfaceId, ETHER_ADDR_LEN);
				wbd_slave_send_vendor_msg_zwdfs(&zwdfs_msg);
			}

			/* Send channel pref report with current radar channel as inoperable */
			ieee1905_send_chan_preference_report();
			wbd_slave_send_operating_chan_report(evt_ifr);
			ieee1905_notify_channel_change(evt_ifr);
			wbd_slave_add_ifr_nbr(evt_ifr, TRUE);
#if !defined(MULTIAPR2)
			/* check and send chan info to controller */
			wbd_slave_chk_and_send_chan_config_info(evt_ifr, FALSE);
#endif /* !MULTIAPR2 */
		}
		break;
		case WLC_E_RRM:
		{
			WBD_INFO("Band[%d] Ifname[%s] RRM EVENT[0x%x]\n",
				evt_ifr->band, ifname, evt_type);
			wbd_slave_store_beacon_report(ifname, data, sta_mac);
		}
		break;

		case WLC_E_AP_CHAN_CHANGE:
		{
			/* Firmware generates and share AP_CHAN_CHANGE event with reason based
			 * on following condition:
			 *
			 * 1: CSA to DFS Radar channel (ignored, handled in CAC_STATE_CHANGE event)
			 *
			 * 2: DFS_AP_MOVE start on DFS Radar channel
			 *
			 * 3: DFS_AP_MOVE radar or abort to indicate stop Scanning
			 *
			 *    Reasons from Firmware are:
			 *
			 *    a: REASON_CSA
			 *    b: REASON_DFS_AP_MOVE_START
			 *    c: REASON_DFS_AP_MOVE_RADAR_FOUND
			 *    d: REASON_DFS_AP_MOVE_ABORT
			 *    e: REASON_DFS_AP_MOVE_SUCCESS
			 *    f: REASON_DFS_AP_MOVE_STUNT
			 *
			 *    DFS_AP_MOVE_START reason: Master initiates dfs_ap_move across all
			 *    repeaters running in slave mode. Present impelentation only supports
			 *    dfs_ap_move initiation only through Master AP.
			 *
			 *    DFS_AP_MOVE_RADAR and ABORT reason: Both Master and Slave repeater
			 *    can generate this reason, on reception Master stops dfs_ap_move
			 *    activity at all other repeaters.
			 *
			 *    DFS_AP_MOVE_SUCCESS reason: To inform and update slave's new chanspec,
			 *    Each slave recieves this reason on completion and send message to
			 *    Master. On reception Master updates slave's chancpec in it's database.
			 *
			 */
			wl_event_change_chan_t *evt_data = NULL;
			wl_chan_change_reason_t reason;
			wbd_cmd_vndr_zwdfs_msg_t zwdfs_msg;
#if defined(MULTIAPR2)
			wbd_ifr_item_t * ifr_vndr_data = NULL;
#endif /* MULTIAPR2 */

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);

			evt_data = (wl_event_change_chan_t*)data;
			if ((evt_data->length != WL_CHAN_CHANGE_EVENT_LEN_VER_1) ||
				(evt_data->version != WL_CHAN_CHANGE_EVENT_VER_1)) {

				WBD_ERROR("Band[%d] INTF["MACF"] Ifname[%s] WLC_E_AP_CHAN_CHANGE"
				"event skipped. Length or version mismatch \n", evt_ifr->band,
				ETHERP_TO_MACF(evt_ifr->InterfaceId), ifname);

				break;
			}
			reason = evt_data->reason;

			if (reason == WL_CHAN_REASON_ANY) {
				wbd_slave_update_chanspec(evt_ifr, evt_data->target_chanspec);
				break;
			}

#if defined(MULTIAPR2)
			ifr_vndr_data = (wbd_ifr_item_t*)evt_ifr->vndr_data;
			if (!ifr_vndr_data) {
				WBD_ERROR(" Invalid ifr vendor data plz Debug, exiting ..\n");
				goto end;
			}
#endif /* MULTIAPR2 */
			/* ignore the event if slave has itself initiated in response to
			 * command from wbd master
			 */
			WBD_DEBUG("Current DFS event mask [0x%x] AP_CHAN_CHANGE reason [%d]\n",
				ifr_vndr_data->dfs_event_mask[0], reason);
			if (isset(ifr_vndr_data->dfs_event_mask, reason)) {
				clrbit(ifr_vndr_data->dfs_event_mask, reason);
#if defined(MULTIAPR2)
				if ((reason == WL_CHAN_REASON_DFS_AP_MOVE_ABORTED) &&
					(ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_RUNNING)) {
					ifr_vndr_data->cac_cur_state = 0;
					wbd_slave_create_cac_start_timer(info);
				}
#endif /* MULTIAPR2 */
				break;
			}

			/* for CSA, DFS_AP_MOVE_STOP update local slave's chanspec, for
			 * DFS_AP_MOVE_START first inform Master for the target chanspec and
			 * later update with current chanspec as dfs_ap_move operation can fail
			 */
			WBD_INFO("AP_CHAN_CHANGE event: Band[%d] INTF["MACF"] Ifname[%s],"
				" Firmware reason[%d] current chanspec[0x%x], target"
				" chanspec[0x%x]\n", evt_ifr->band,
				ETHERP_TO_MACF(evt_ifr->InterfaceId),
				ifname, reason, evt_ifr->chanspec, evt_data->target_chanspec);

			/* ignore DFS_AP_MOVE success call to controller */
			if ((reason == WL_CHAN_REASON_DFS_AP_MOVE_SUCCESS) ||
				(reason == WL_CHAN_REASON_DFS_AP_MOVE_STUNT_SUCCESS)) {
#if defined(MULTIAPR2)
				WBD_INFO("dfs_ap_move complete reason[%s] current cac state[%d]\n",
					(reason == WL_CHAN_REASON_DFS_AP_MOVE_SUCCESS) ?
					"SUCCESS" : "STUNT SUCCESS", ifr_vndr_data->cac_cur_state);

				if (ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_RUNNING) {
					ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_COMPLETE;
					ifr_vndr_data->cac_compl_status = MAP_CAC_STATUS_SUCCESS;
					wbd_slave_create_cac_start_timer(info);
				/* If CAC is running skip channel report, since it will be send
				 * after CAC finished for all interfaces
				 */
				} else
#endif /* MULTIAPR2 */
				{
					ieee1905_send_chan_preference_report();
				}
				if (reason == WL_CHAN_REASON_DFS_AP_MOVE_SUCCESS) {
					evt_ifr->chanspec = evt_data->target_chanspec;
					wbd_slave_send_operating_chan_report(evt_ifr);
					ieee1905_notify_channel_change(evt_ifr);
					wbd_slave_add_ifr_nbr(evt_ifr, TRUE);
				}
				break;
			}
			/* prepare zwdfs msg, forward to 1905 to be passed to controller */
			memset(&zwdfs_msg, 0, sizeof(zwdfs_msg));

			blanket_get_global_rclass(evt_data->target_chanspec, &(zwdfs_msg.opclass));
			zwdfs_msg.cntrl_chan = wf_chspec_ctlchan(evt_data->target_chanspec);
			zwdfs_msg.reason = evt_data->reason;
			memcpy(&zwdfs_msg.mac, evt_ifr->InterfaceId, ETHER_ADDR_LEN);

			if (reason == WL_CHAN_REASON_DFS_AP_MOVE_RADAR_FOUND) {
#if defined(MULTIAPR2)
				WBD_INFO("dfs_ap_move RADAR DETECTED current cac state[%d]\n",
					ifr_vndr_data->cac_cur_state);
				if (ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_RUNNING) {
					ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_COMPLETE;
					ifr_vndr_data->cac_compl_status =
						MAP_CAC_STATUS_RADAR_DETECTED;
					wbd_slave_create_cac_start_timer(info);
				} else
#endif /* MULTIAPR2 */
				{
					/* In R2, if RADAR is detected and CAC state is NOT RUNNING
					 * means, dfs_ap_move was not initiated by the controller.
					 * So send zwdfs vendor command to controller so that it can
					 * be propagated to other agents
					 */
					wbd_slave_send_vendor_msg_zwdfs(&zwdfs_msg);
				}
				/* If RADAR detected, don't wait the CAC to finish for other
				 * interfaces. Send channel preference report immediately for zwdfs
				 */
				evt_ifr->chanspec = evt_data->target_chanspec;
				ieee1905_send_chan_preference_report();
#if !defined(MULTIAPR2)
				wbd_slave_chk_and_send_chan_config_info(evt_ifr, FALSE);
#endif /* !MULTIAPR2 */
				wbd_slave_send_operating_chan_report(evt_ifr);
				ieee1905_notify_channel_change(evt_ifr);
				wbd_slave_add_ifr_nbr(evt_ifr, TRUE);
				break;
			}
#if defined(MULTIAPR2)
			/* Update CAC varible and states properly for dfs_ap_move initiated by
			 * some external applications, like wl.
			 */
			if (reason == WL_CHAN_REASON_DFS_AP_MOVE_START ||
				reason == WL_CHAN_REASON_DFS_AP_MOVE_STUNT) {
				info->wbd_slave->flags |= WBD_BKT_SLV_FLAGS_CAC_RUNNING;
				ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_RUNNING;
				ifr_vndr_data->old_chanspec = evt_ifr->chanspec;
				if (!BW_LE40(CHSPEC_BW(ifr_vndr_data->cac_rqst_chspec))) {
					ifr_vndr_data->cac_rqst_chspec &= ~WL_CHANSPEC_CTL_SB_MASK;
				}
				ifr_vndr_data->cac_rqst_method =
					MAP_CAC_METHOD_MIMO_DIMENSION_REDUCED_CAC;
				if (reason == WL_CHAN_REASON_DFS_AP_MOVE_START) {
					ifr_vndr_data->cac_rqst_chspec = evt_data->target_chanspec;
					ifr_vndr_data->cac_rqst_action =
						MAP_CAC_ACTION_STAY_ON_NEW_CHANNEL;
				} else {
					ifr_vndr_data->cac_rqst_action =
						MAP_CAC_ACTION_RETURN_TO_PREV_CHANNEL;
					blanket_get_global_rclass(ifr_vndr_data->cac_rqst_chspec,
						&(zwdfs_msg.opclass));
					zwdfs_msg.cntrl_chan =
						wf_chspec_ctlchan(ifr_vndr_data->cac_rqst_chspec);
				}
			}
			if (reason == WL_CHAN_REASON_DFS_AP_MOVE_ABORTED) {
				blanket_get_global_rclass(ifr_vndr_data->cac_rqst_chspec,
					&(zwdfs_msg.opclass));
				zwdfs_msg.cntrl_chan =
					wf_chspec_ctlchan(ifr_vndr_data->cac_rqst_chspec);
				ifr_vndr_data->cac_cur_state = 0;
			}
#endif /* MULTIAPR2 */
			/* Send zwdfs vendor command to controller for dfs_ap_move start, stunt,
			 * and abort if it was not initiated by the controller
			 */
			wbd_slave_send_vendor_msg_zwdfs(&zwdfs_msg);
		}
		break;

#if defined(MULTIAPR2)
		case WLC_E_MBO_CAPABILITY_STATUS:
		{
			wlc_mbo_bss_status_t *mbo_status = NULL;
			wbd_bss_item_t *wbd_bss = NULL;

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);
			mbo_status = (wlc_mbo_bss_status_t *)data;
			if (!mbo_status) {
				WBD_DEBUG(" Invalid event data passed for mbo_bss_status, exit\n");
				goto end;
			}

			if (!evt_bss) {
				   WBD_DEBUG("skip MBO Capability status event, bss not found \n");
				   goto end;
			}
			WBD_DEBUG("Band[%d] BSS["MACF"] Ifname[%s], mbo_bss_status "
				"assoc_allowance_status[%d], ap_attr[%d]\n", evt_ifr->band,
				ETHERP_TO_MACF(evt_bss->BSSID), ifname,
				mbo_status->assoc_allowance_status, mbo_status->ap_attr);

			evt_bss->assoc_allowance_status = !mbo_status->assoc_allowance_status;
			/* update bss's mbo params */
			wbd_bss = (wbd_bss_item_t*)evt_bss->vndr_data;
			wbd_bss->mbo_params.ap_attr = mbo_status->ap_attr;
			wbd_bss->mbo_params.mbo_enable = mbo_status->mbo_bss_enable;
			/* send association notification request message to controller */
			wbd_slave_send_association_status_notification(evt_bss);
		}
		break;

		case WLC_E_WNM_NOTIFICATION_REQ:
		case WLC_E_WNM_BSSTRANS_QUERY:
		case WLC_E_GAS_RQST_ANQP_QUERY:
		case WLC_E_BSSTRANS_QUERY:
		{
			ieee1905_tunnel_msg_t tunnel_msg;

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);

			memset(&tunnel_msg, 0, sizeof(tunnel_msg));
			WBD_INFO(" rcvd notification frame from sta["MACF"] of len [%d]\n",
				ETHERP_TO_MACF(sta_mac), datalen);

			memcpy(&tunnel_msg.source_mac, sta_mac, ETHER_ADDR_LEN);
			tunnel_msg.payload = data;
			tunnel_msg.payload_len = datalen;
			if (evt_type == WLC_E_WNM_NOTIFICATION_REQ) {
				tunnel_msg.payload_type = ieee1905_tunnel_msg_payload_wnm_rqst;
			} else if ((evt_type == WLC_E_WNM_BSSTRANS_QUERY) ||
					(evt_type == WLC_E_BSSTRANS_QUERY)) {
				tunnel_msg.payload_type = ieee1905_tunnel_msg_payload_btm_query;
			} else if (evt_type == WLC_E_GAS_RQST_ANQP_QUERY) {
				tunnel_msg.payload_type = ieee1905_tunnel_msg_payload_anqp_rqst;
			} else {
				WBD_INFO("unknown evt_type[%d], dont process, exit \n", evt_type);
				/* Unknown event, dont process further, exit */
				break;
			}
			wbd_slave_send_tunneled_msg(&tunnel_msg);

		}
		break;

		/* Following three events from driver are used for sending failed connection
		 * message. WLC_E_ASSOC_FAIL and WLC_E_REASSOC_FAIL sends status code and
		 * WLC_E_AUTH_FAIL sends reason code. To controller we should send status code
		 * if it is present or send reason code by setting status code as 0
		 */
		case WLC_E_ASSOC_FAIL:	     /* 188 Association fail event */
		case WLC_E_REASSOC_FAIL:     /* 189 Re-association Fail event */
			/* Sends status code */
			status = ntoh32(pvt_data->event.reason);
			/* Fall through */

		case WLC_E_AUTH_FAIL:	     /* 190 Authentication fail event */
		{
			/* Unsuccessful Association Policy TLV with the Report Unsuccessful
			 * Associations bit set to 1, and if the Multi-AP Agent has sent
			 * fewer than the maximum number of Failed Connection notifications
			 * in the preceding minute, and the Multi-AP Agent detects that Wi-Fi
			 * client has made a failed attempt to connect to any BSS operated by
			 * the Multi-AP Agent, the Multi-AP Agent shall send to the
			 * Multi-AP Controller a Failed Connection message including a STA
			 * MAC Address TLV identifying the client that has attempted to connect
			 * and a Status Code TLV with the Status Code set to a non-zero value that
			 * indicates the reason for association or authentication failure or a
			 * Status Code TLV with the Status Code set to zero and a Reason Code TLV
			 * with the Reason Code indicating the reason the STA was disassociated
			 * or deauthenticated.
			 */
			uint32 reason;
			ieee1905_unsuccessful_assoc_config_t *unsuccessful_assoc_config;

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);
			/* This event sends reason code */
			reason = ntoh32(pvt_data->event.reason);
			unsuccessful_assoc_config = &info->wbd_slave->unsuccessful_assoc_config;

			if (unsuccessful_assoc_config->report_flag &
				MAP_UNSUCCESSFUL_ASSOC_FLAG_REPORT) {

				if (unsuccessful_assoc_config->count <
					unsuccessful_assoc_config->max_reporting_rate) {
					ret = blanket_sta_assoc_failed_connection(sta_mac,
						(uint16)status, (uint16)reason);

					if (ret != 0) {
						WBD_WARNING("Band[%d] Ifname[%s] "
							"Unsuccessful Association Event[0x%x] "
							"for STA["MACF"]. Failed to Add to "
							"MultiAP Error : %d\n", evt_ifr->band,
							ifname, evt_type, ETHERP_TO_MACF(sta_mac),
							ret);
					} else {
						unsuccessful_assoc_config->count++;
					}
				} else {
					WBD_DEBUG("Not sending Failed connection message as number "
						"of failed connection message sent[%d] exceedes "
						"the Number of failed connection messages to be "
						"reported[%d]\n",
						unsuccessful_assoc_config->count,
						unsuccessful_assoc_config->max_reporting_rate);
				}
			}
		}
		break;

		/* To Send Channel Scan Results */
		case WLC_E_ESCAN_RESULT:
		{
			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);
			/* Parse and Process the ESCAN EVENT from EAPD */
			wbd_slave_process_event_escan(evt_ifr, ifname, pvt_data);
			break;
		}
#endif /* MULTIAPR2 */

		case WLC_E_CAC_STATE_CHANGE:
		{
			wlc_cac_event_t *cac_event = NULL;
			wl_dfs_status_all_t *all = NULL;
			wl_dfs_sub_status_t *sub0 = NULL;
			wbd_ifr_item_t *ifr_vndr_data = NULL;

			ifr_vndr_data = (wbd_ifr_item_t*)evt_ifr->vndr_data;

			WBD_INFO("Event[0x%x] ifname[%s]\n", evt_type, ifname);
			if (!ifr_vndr_data) {
				WBD_ERROR(" Invalid ifr vendor data plz Debug, exiting ..\n");
				goto end;
			}

			/* extract event information */
			cac_event = (wlc_cac_event_t *)data;
			all = (wl_dfs_status_all_t*)&(cac_event->scan_status);
			sub0 = &(all->dfs_sub_status[0]);

			/* as of now monitoring only for full time CAC, so sub0 is sufficient */
			if (sub0->state == WL_DFS_CACSTATE_PREISM_CAC) {
				wbd_cmd_vndr_zwdfs_msg_t zwdfs_msg;
				wbd_bss_item_t *wbd_bss = NULL;

				if (!evt_bss) {
					WBD_INFO("Received cac state change event on %s did not "
						"find corresponding bss so skip event processing\n",
						ifname);
					ret = WBDE_EAPD_ERROR;
					goto end;
				}

				wbd_bss = (wbd_bss_item_t*)evt_bss->vndr_data;
				WBD_INFO("FULL Time CAC started for "MACDBG"(%s) on chspec [0x%x]."
					" Current chanspec [0x%x] bss flags [0x%x]\n",
					MAC2STRDBG(evt_ifr->InterfaceId), ifname, sub0->chanspec,
					evt_ifr->chanspec, wbd_bss->flags);

				if (wbd_bss->flags & WBD_FLAGS_BSS_CAC_STARTED) {
					wbd_bss->flags &= ~WBD_FLAGS_BSS_CAC_STARTED;
					break;
				}
				if (evt_ifr->chanspec == sub0->chanspec) {
					break;
				}

#ifdef MULTIAPR2
				/* Update CAC varible and states properly for full time CAC
				 * initiated by some external applications, like wl.
				 */
				info->wbd_slave->flags |= WBD_BKT_SLV_FLAGS_CAC_RUNNING;
				ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_RUNNING;
				ifr_vndr_data->old_chanspec = evt_ifr->chanspec;
				ifr_vndr_data->cac_rqst_chspec = sub0->chanspec;
				if (!BW_LE40(CHSPEC_BW(ifr_vndr_data->cac_rqst_chspec))) {
					ifr_vndr_data->cac_rqst_chspec &= ~WL_CHANSPEC_CTL_SB_MASK;
				}
				ifr_vndr_data->cac_rqst_method = MAP_CAC_METHOD_CONTINOUS_CAC;
				ifr_vndr_data->cac_rqst_action = MAP_CAC_ACTION_STAY_ON_NEW_CHANNEL;
#endif /* MULTIAPR2 */
				/* Send zwdfs vendor command to controller for PRE-ISM CAC,
				 * since it was not initiated by the controller
				 */
				memset(&zwdfs_msg, 0, sizeof(zwdfs_msg));

				blanket_get_global_rclass(sub0->chanspec, &(zwdfs_msg.opclass));

				zwdfs_msg.cntrl_chan = wf_chspec_ctlchan(sub0->chanspec);
				/* FIXME: Assuming WL_CHAN_REASON_CSA_TO_DFS_CHAN_FOR_CAC_ONLY will
				 *        be used only by WBD
				 */
				zwdfs_msg.reason = WL_CHAN_REASON_CSA;
				memcpy(&zwdfs_msg.mac, evt_ifr->InterfaceId, ETHER_ADDR_LEN);

				wbd_slave_send_vendor_msg_zwdfs(&zwdfs_msg);
				break;
			}
			if (sub0->state == WL_DFS_CACSTATE_ISM && !cac_event->scan_core) {
				WBD_INFO("ISM started for "MACDBG" on chanspec [0x%x]. current "
					"chanspec [0x%x] cac state [%d] event mask [0x%x]\n",
					MAC2STRDBG(evt_ifr->InterfaceId), sub0->chanspec,
					evt_ifr->chanspec, ifr_vndr_data->cac_cur_state,
					ifr_vndr_data->dfs_event_mask[0]);
#ifdef MULTIAPR2
				/* CAC state over, and DFS state machine switched
				 * to ISM with success
				 */
				if (ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_RUNNING) {
					ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_COMPLETE;
					ifr_vndr_data->cac_compl_status = MAP_CAC_STATUS_SUCCESS;
					WBD_DEBUG("CAC running. NEW CAC state change is ISM for["
						MACF"]\n", ETHERP_TO_MACF(evt_ifr->InterfaceId));
					wbd_slave_create_cac_start_timer(info);
				} else
#endif /* MULTIAPR2 */
				{
					ieee1905_send_chan_preference_report();
				}
				if (evt_ifr->chanspec != sub0->chanspec) {
					evt_ifr->chanspec = sub0->chanspec;
					wbd_slave_send_operating_chan_report(evt_ifr);
					ieee1905_notify_channel_change(evt_ifr);
					wbd_slave_add_ifr_nbr(evt_ifr, TRUE);
				}
			}
			break;
		}

		default:
			WBD_INFO("Band[%d] Ifname[%s] Event[0x%x] UnKnown Event\n",
				evt_ifr->band, ifname, evt_type);
			break;
	}

end: /* Check Slave Pointer before using it below */

	/* good packet may be this is destined to us, if no error */
	WBD_EXIT();
	return ret;
}

/* Callback function called from scheduler library */
/* to Process event by accepting the connection and processing the client */
void
wbd_slave_process_event_fd_cb(bcm_usched_handle *handle, void *arg, bcm_usched_fds_entry_t *entry)
{
	int ret = WBDE_OK, rcv_ret;
	wbd_info_t *info = (wbd_info_t*)arg;
	char read_buf[WBD_BUFFSIZE_4K] = {0};
	WBD_ENTER();

	BCM_REFERENCE(ret);
	/* Get the data from client */
	rcv_ret = wbd_socket_recv_bindata(info->event_fd, read_buf, sizeof(read_buf));
	if ((rcv_ret <= 0)) {
		WBD_WARNING("Failed to recieve event. Error code : %d\n", rcv_ret);
		ret = WBDE_EAPD_ERROR;
		goto end;
	}
	/* Parse and process the EVENT from EAPD */
	ret = wbd_slave_process_event_msg(info, read_buf, rcv_ret);
end:
	WBD_EXIT();
}

/* Retrieve fresh stats from driver for the associated STAs and update locally.
 * If sta_mac address is provided get the stats only for that STA
 * Else if isweakstas is TRUE, get only for weak STAs else get for all the associated STAs
 */
int
wbd_slave_update_assoclist_fm_wl(i5_dm_bss_type *i5_bss, struct ether_addr *sta_mac,
	int isweakstas)
{
	int ret = WBDE_OK;
	wbd_assoc_sta_item_t* sta_item;
	wbd_assoc_sta_item_t outitem;
	i5_dm_clients_type *i5_sta;
	WBD_ENTER();

	/* Travese STA List associated with this BSS */
	foreach_i5glist_item(i5_sta, i5_dm_clients_type, i5_bss->client_list) {

		sta_item = (wbd_assoc_sta_item_t*)i5_sta->vndr_data;
		if (sta_item == NULL) {
			WBD_WARNING("STA["MACDBG"] NULL Vendor Data\n", MAC2STRDBG(i5_sta->mac));
			continue;
		}

		/* If STA mac address is provided, get the stats only for that STA.
		 * Else Check if the stats to be get only for WEAK STAs or for all the STAs
		 */
		if (sta_mac != NULL) {
			if (memcmp(sta_mac, i5_sta->mac, MAC_ADDR_LEN))
				continue;
		} else if ((isweakstas == TRUE) && (sta_item->status != WBD_STA_STATUS_WEAK) &&
			(sta_item->status != WBD_STA_STATUS_STRONG)) {
			continue;
		}

		/* Initialize local variables for this iteration */
		memset(&outitem, 0, sizeof(outitem));

		/* Get the STA info from WL driver for this STA item */
		if ((ret = wbd_slave_fill_sta_stats(i5_bss->ifname,
			(struct ether_addr*)&i5_sta->mac, &(sta_item->stats))) != WBDE_OK) {
			WBD_WARNING("BSS["MACDBG"] STA["MACDBG"] Wl fill stats failed. WL "
				"error : %s\n", MAC2STRDBG(i5_bss->BSSID),
				MAC2STRDBG(i5_sta->mac), wbderrorstr(ret));
			continue;
		}

		/* Now update the stats to be shared with Master */
		sta_item->stats.tx_rate = outitem.stats.tx_rate;
		sta_item->stats.rssi = outitem.stats.rssi;
	}

	WBD_EXIT();
	return ret;
}

#ifdef PLC_WBD
/* Retrieve fresh stats from PLC for all the PLC associated nodes and update locally */
static int
wbd_slave_update_plc_assoclist(wbd_slave_item_t *slave)
{
	wbd_plc_assoc_info_t *assoc_info = NULL;
	wbd_plc_sta_item_t *new_plc_sta;
	wbd_plc_info_t *plc_info;
	int ret = WBDE_OK;
	int i;

	WBD_ENTER();

	plc_info = &slave->parent->parent->plc_info;
	/* Do nothing if PLC is disabled */
	if (!plc_info->enabled) {
		ret = WBDE_PLC_DISABLED;
		goto end;
	}

	/* Get the assoclist */
	ret = wbd_plc_get_assoc_info(plc_info, &assoc_info);
	if (ret != WBDE_OK) {
		WBD_WARNING("Band[%d] Slave["MACF"]. Failed to get PLC assoc info : %s\n",
			slave->band, ETHER_TO_MACF(slave->wbd_ifr.mac), wbderrorstr(ret));
		goto end;
	}

	for (i = 0; i < assoc_info->count; i++) {
		WBD_INFO("Band[%d] Slave["MACF"] PLC rate to "MACF": tx %f rx %f\n",
			slave->band, ETHER_TO_MACF(slave->wbd_ifr.mac),
			ETHER_TO_MACF(assoc_info->node[i].mac),
			assoc_info->node[i].tx_rate,
			assoc_info->node[i].rx_rate);
	}

	/* Clear the list and add them */
	wbd_ds_slave_item_cleanup_plc_sta_list(slave);

	/* Add remote plc_sta to plc_sta_list */
	for (i = 0; i < assoc_info->count; i++) {
		/* Create a PLC STA item */
		new_plc_sta = (wbd_plc_sta_item_t*) wbd_malloc(sizeof(*new_plc_sta), &ret);
		WBD_ASSERT_MSG("Band[%d] Slave["MACF"] STA["MACF"]. %s\n", slave->band,
			ETHER_TO_MACF(slave->wbd_ifr.mac), ETHER_TO_MACF(new_plc_sta->mac),
			wbderrorstr(ret));

		/* Fill & Initialize PLC STA item data */
		eacopy(&assoc_info->node[i].mac, &new_plc_sta->mac);
		new_plc_sta->tx_rate = assoc_info->node[i].tx_rate;
		new_plc_sta->rx_rate = assoc_info->node[i].rx_rate;

		/* Add this new PLC STA item to plc_sta_list of this Slave */
		wbd_ds_glist_append(&slave->plc_sta_list, (dll_t *) new_plc_sta);
	}

end:
	if (assoc_info)
		free(assoc_info);

	WBD_EXIT();
	return ret;
}
#endif /* PLC_WBD */

/* Processes WEAK_CLIENT response */
static void
wbd_slave_process_weak_client_cmd_resp(unsigned char *neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len)
{
	int ret = WBDE_OK;
	wbd_cmd_weak_strong_client_resp_t cmdrsp;
	wbd_assoc_sta_item_t *sta = NULL;
	i5_dm_bss_type *i5_bss = NULL;
	WBD_ENTER();

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(tlv_data, "WEAK_STRONG_CLIENT_RESP");

	memset(&cmdrsp, 0, sizeof(cmdrsp));

	/* Decode Vendor Specific TLV for Message : weak/strong client response on receive */
	ret = wbd_tlv_decode_weak_strong_client_response((void *)&cmdrsp, tlv_data, tlv_data_len);
	if (ret != WBDE_OK) {
		WBD_WARNING("Failed to decode the weak/strong client response TLV\n");
		goto end;
	}

	WBD_INFO("Weak/Strong Client Vendor Specific Command Response : BSSID["MACF"] STA["MACF"] "
		"ErrorCode[%d] Dwell[%d]\n",
		ETHER_TO_MACF(cmdrsp.BSSID), ETHER_TO_MACF(cmdrsp.sta_mac),
		cmdrsp.error_code, cmdrsp.dwell_time);

	WBD_SAFE_GET_I5_SELF_BSS((unsigned char*)&cmdrsp.BSSID, i5_bss, &ret);

	/* Get STA pointer of Weak/Strong client */
	if (wbd_ds_find_sta_in_bss_assoclist(i5_bss, &cmdrsp.sta_mac, &ret, &sta) == NULL) {
		WBD_WARNING("Slave["MACDBG"] STA["MACF"]. %s\n",
			MAC2STRDBG(i5_bss->BSSID), ETHER_TO_MACF(cmdrsp.sta_mac),
			wbderrorstr(ret));
		goto end;
	}

	/* Save the response code and dwell time from the master on weak/strong client response */
	sta->error_code = wbd_wc_resp_reason_code_to_error(cmdrsp.error_code);
	sta->dwell_time = cmdrsp.dwell_time;

	/* If the STA is bouncing STA, update the dwell start time */
	if (sta->error_code == WBDE_DS_BOUNCING_STA) {
		sta->dwell_start = time(NULL);
		WBD_INFO("Slave["MACDBG"] STA["MACF"] error[%d] dwell[%lu] dwell_start[%lu]\n",
			MAC2STRDBG(i5_bss->BSSID), ETHER_TO_MACF(cmdrsp.sta_mac),
			sta->error_code, (unsigned long)sta->dwell_time,
			(unsigned long)sta->dwell_start);
	}

	if (sta->error_code == WBDE_IGNORE_STA) {
		/* If the error code is ignore update the status */
		sta->status = WBD_STA_STATUS_IGNORE;
		WBD_DEBUG("Slave["MACDBG"] Updated STA["MACF"] as Ignored\n",
			MAC2STRDBG(i5_bss->BSSID), ETHER_TO_MACF(cmdrsp.sta_mac));
	} else if (sta->error_code != WBDE_OK) {
		sta->status = WBD_STA_STATUS_NORMAL;
		WBD_DEBUG("Slave["MACDBG"] Updated STA["MACF"] as Normal Due to Error[%s] "
			"from Master\n", MAC2STRDBG(i5_bss->BSSID),
			ETHER_TO_MACF(cmdrsp.sta_mac), wbderrorstr(sta->error_code));
	}

end: /* Check Slave Pointer before using it below */

	WBD_EXIT();
}

/* Send WEAK_CLIENT/STRONG_CLIENT command and Update STA Status */
int
wbd_slave_send_weak_strong_client_cmd(i5_dm_bss_type *i5_bss, i5_dm_clients_type *i5_assoc_sta,
	wbd_sta_status_t command)
{
	int ret = WBDE_OK;
	i5_dm_device_type *controller_device = NULL;
	wbd_assoc_sta_item_t *assoc_sta;
	WBD_ENTER();

	assoc_sta = (wbd_assoc_sta_item_t*)i5_assoc_sta->vndr_data;
	WBD_SAFE_GET_I5_CTLR_DEVICE(controller_device, &ret);

	ret = ieee1905_send_assoc_sta_link_metric(controller_device->DeviceId, i5_assoc_sta->mac);

	WBD_CHECK_ERR_MSG(WBDE_COM_ERROR, "Slave["MACDBG"] STA["MACDBG"] Failed to send "
		"%s, Error : %s\n", MAC2STRDBG(i5_bss->BSSID),
		MAC2STRDBG(i5_assoc_sta->mac),
		(command == WBD_STA_STATUS_WEAK) ? "WEAK_CLIENT" : "STRONG_CLIENT",
		wbderrorstr(ret));

end:
	if (assoc_sta) {
		assoc_sta->error_code = WBDE_OK;

		/* If successfully sent WEAK_CLIENT/STRONG_CLIENT, Update STA's Status */
		if (ret == WBDE_OK) {
			if (command == WBD_STA_STATUS_WEAK) {
				assoc_sta->status = WBD_STA_STATUS_WEAK;
			} else if (command == WBD_STA_STATUS_STRONG) {
				assoc_sta->status = WBD_STA_STATUS_STRONG;
			}

#ifdef BCM_APPEVENTD
			/* Send weak client event to appeventd. */
			wbd_appeventd_weak_sta(
				(command == WBD_STA_STATUS_WEAK) ? APP_E_WBD_SLAVE_WEAK_CLIENT :
				APP_E_WBD_SLAVE_STRONG_CLIENT,
				i5_bss->ifname,	(struct ether_addr*)&i5_assoc_sta->mac,
				assoc_sta->stats.rssi, assoc_sta->stats.tx_failures,
				assoc_sta->stats.tx_rate);
#endif /* BCM_APPEVENTD */
		}
	} else {
		WBD_WARNING("STA["MACDBG"] NULL Vendor Data\n", MAC2STRDBG(i5_assoc_sta->mac));
	}

	WBD_EXIT();
	return ret;
}

/* Send WEAK_CANCEL command and Update STA Status = Normal */
int
wbd_slave_send_weak_strong_cancel_cmd(i5_dm_bss_type *i5_bss, i5_dm_clients_type *i5_assoc_sta,
	wbd_sta_status_t command)
{
	int ret = WBDE_OK;
	i5_dm_device_type *controller_device;
	wbd_assoc_sta_item_t *assoc_sta;
	WBD_ENTER();

	WBD_SAFE_GET_I5_CTLR_DEVICE(controller_device, &ret);

	ret = ieee1905_send_assoc_sta_link_metric(controller_device->DeviceId, i5_assoc_sta->mac);

	WBD_CHECK_ERR_MSG(WBDE_COM_ERROR, "Slave["MACDBG"] STA["MACDBG"] Failed to send "
		"%s, Error : %s\n", MAC2STRDBG(i5_bss->BSSID),
		MAC2STRDBG(i5_assoc_sta->mac),
		(command == WBD_STA_STATUS_WEAK) ? "WEAK_CANCEL" : "STRONG_CANCEL",
		wbderrorstr(ret));

end:
	/* If successfully sent WEAK_CANCEL/STRONG_CANCEL, Update STA's Status = Normal */
	if (ret == WBDE_OK) {
		assoc_sta = (wbd_assoc_sta_item_t*)i5_assoc_sta->vndr_data;
		if (assoc_sta == NULL) {
			WBD_WARNING("STA["MACDBG"] NULL Vendor Data\n",
				MAC2STRDBG(i5_assoc_sta->mac));
		} else {
			assoc_sta->status = WBD_STA_STATUS_NORMAL;

			/* Reset the Steer fail count also */
			assoc_sta->steer_fail_count = 0;
		}
	}

	WBD_EXIT();
	return ret;
}

/* Set mode of ACS deamon running on Repeater to Fixed Chanspec */
int
wbd_slave_set_acsd_mode_to_fixchspec(i5_dm_interface_type *pdmif)
{
	wbd_info_t *info = NULL;
	wbd_com_handle *com_hndl = NULL;
	int ret = WBDE_OK, mode;
	char tmpbuf[WBD_MAX_BUF_512];
	int sock_options = 0x0000;
	WBD_ENTER();

	info = wbd_get_ginfo();

	WBD_ASSERT_ARG(pdmif, WBDE_INV_ARG);

	/* Set ACS daemon's mode to Fixed Chanspec, only for Repeater not at RootAP */
	if (MAP_IS_CONTROLLER(info->map_mode)) {
		goto end;
	}

	mode = 5; /* ACS_MODE_FIXCHSPEC */

	snprintf(tmpbuf, sizeof(tmpbuf), "set&ifname=%s&param=mode&value=%d", pdmif->ifname, mode);

	sock_options =	WBD_COM_FLAG_CLIENT | WBD_COM_FLAG_BLOCKING_SOCK
			| WBD_COM_FLAG_NO_RESPONSE;

	com_hndl = wbd_com_init(info->hdl, INVALID_SOCKET, sock_options, NULL, NULL, info);

	if (!com_hndl) {
		WBD_ERROR("Band[%d] interface["MACF"] Failed to initialize the communication"
			"module for setting Fixed Chanspec[0x%x] Mode to ACSD\n", pdmif->band,
			ETHERP_TO_MACF(pdmif->InterfaceId), pdmif->chanspec);
		ret = WBDE_COM_ERROR;
		goto end;
	}

	/* Send the command to ACSD */
	ret = wbd_com_connect_and_send_cmd(com_hndl, ACSD_DEFAULT_CLI_PORT,
		WBD_LOOPBACK_IP, tmpbuf, NULL);
	WBD_CHECK_MSG("Band[%d] interface["MACF"] Failed to set Fixed Chanspec[0x%x] Mode to ACSD. "
		"Error : %s\n", pdmif->band, ETHERP_TO_MACF(pdmif->InterfaceId),
		pdmif->chanspec, wbderrorstr(ret));

end:
	if (com_hndl) {
		wbd_com_deinit(com_hndl);
	}

	WBD_EXIT();
	return ret;
}

int
wbd_slave_start_acsd_autochannel(i5_dm_interface_type *pdmif)
{
	wbd_info_t *info = NULL;
	wbd_com_handle *com_hndl = NULL;
	int ret = WBDE_OK;
	char tmpbuf[WBD_MAX_BUF_512];
	int sock_options = 0x0000;
	WBD_ENTER();

	info = wbd_get_ginfo();

	WBD_ASSERT_ARG(pdmif, WBDE_INV_ARG);

	/* Start acsd autochannel only from agent */
	if (MAP_IS_CONTROLLER(info->map_mode)) {
		goto end;
	}

	snprintf(tmpbuf, sizeof(tmpbuf), "autochannel&ifname=%s", pdmif->ifname);

	sock_options =	WBD_COM_FLAG_CLIENT | WBD_COM_FLAG_BLOCKING_SOCK |
		WBD_COM_FLAG_NO_RESPONSE;

	com_hndl = wbd_com_init(info->hdl, INVALID_SOCKET, sock_options, NULL, NULL, info);

	if (!com_hndl) {
		WBD_ERROR("Band[%d] interface["MACF"] Failed to initialize the communication"
			"module for starting ACSD autochannel\n", pdmif->band,
			ETHERP_TO_MACF(pdmif->InterfaceId));
		ret = WBDE_COM_ERROR;
		goto end;
	}

	/* Send the command to ACSD */
	ret = wbd_com_connect_and_send_cmd(com_hndl, ACSD_DEFAULT_CLI_PORT,
		WBD_LOOPBACK_IP, tmpbuf, NULL);
	WBD_CHECK_MSG("Band[%d] interface["MACF"] Failed to start ACSD autochannel "
		"Error : %s\n", pdmif->band, ETHERP_TO_MACF(pdmif->InterfaceId),
		wbderrorstr(ret));

end:
	if (com_hndl) {
		wbd_com_deinit(com_hndl);
	}

	WBD_EXIT();
	return ret;
}

/* To track cac is indeed being started from wbd setting cac started flag for each bss of ifr */
static void
wbd_slave_set_dfs_started_for_each_bss(i5_dm_interface_type *ifr)
{
	i5_dm_bss_type *bss = NULL;
	wbd_bss_item_t *wbd_bss = NULL;

	foreach_i5glist_item(bss, i5_dm_bss_type, ifr->bss_list) {
		wbd_bss = (wbd_bss_item_t*)bss->vndr_data;
		wbd_bss->flags |= WBD_FLAGS_BSS_CAC_STARTED;
	}
}

/* Set chanspec through ACSD deamon running on ACCESS POINT */
/* TODO : check for the new chanspec logic
 * Never Update Agent's Interface's Local Chanspec after this call.
 * That is done in AP_CHAN_CHANGE Event processing with Reason = WL_CHAN_REASON_ANY using API
 * wbd_slave_update_chanspec. This API also sends Operating Channel Report to Controller
 */
int
wbd_slave_set_chanspec_through_acsd(chanspec_t chspec,
	wl_chan_change_reason_t reason, i5_dm_interface_type *interface)
{
	int ret = WBDE_OK;
	char *nvval = NULL;
	char tmpbuf[WBD_MAX_BUF_512], strchspec[WBD_MAX_BUF_32];
	wbd_com_handle *com_hndl = NULL;
	wbd_info_t *info;
	int sock_options = 0x0000;
	uint16 channel;
	wbd_ifr_item_t *ifr_vndr_data = NULL;
	char prefix[IFNAMSIZ + 1];
	WBD_ENTER();

	info = wbd_get_ginfo();

	/* Fetch nvram param */
	snprintf(prefix, sizeof(prefix), "%s_", interface->wlParentName);
	nvval = blanket_nvram_prefix_safe_get(prefix, "ifname");

	ifr_vndr_data = (wbd_ifr_item_t*)interface->vndr_data;

	if (!ifr_vndr_data) {
		WBD_ERROR(" Invalid ifr vendor data plz Debug, exiting ..\n");
		goto end;
	}
	/* set dfs_event_mask, so that wbd_proces_event routine can ignore these events as it
	 * is generated by wbd itself. In case of CSA, non DFS channel won't generate an event.
	 * So check if the channel is DFS before setting the mask.
	 */
	if ((reason != WL_CHAN_REASON_CSA) &&
		reason != (WL_CHAN_REASON_CSA_TO_DFS_CHAN_FOR_CAC_ONLY)) {
		setbit(ifr_vndr_data->dfs_event_mask, reason);
	} else {
		FOREACH_20_SB(chspec, channel) {
			uint bitmap_sb = 0x00;
			blanket_get_chan_info(interface->ifname, channel,
				CHSPEC_BAND(chspec), &bitmap_sb);
			if ((bitmap_sb & WL_CHAN_RADAR) &&
				(bitmap_sb & WL_CHAN_PASSIVE) &&
				!(bitmap_sb & WL_CHAN_INACTIVE)) {
				/* From wl preism cac state change event gets generated for each
				 * bss. To track cac is indeed being started from wbd,
				 * setting cac started flag for each bss. While handling cac state
				 * change event we are checking this flag for bss.
				 */
				wbd_slave_set_dfs_started_for_each_bss(interface);
				break;
			}
		}
	}

	snprintf(tmpbuf, sizeof(tmpbuf), "set&ifname=%s&param=chanspec&value=%d&option=%d",
		nvval, chspec, (int)reason);

	sock_options =	WBD_COM_FLAG_CLIENT | WBD_COM_FLAG_BLOCKING_SOCK
			| WBD_COM_FLAG_NO_RESPONSE;

	com_hndl = wbd_com_init(info->hdl, INVALID_SOCKET, sock_options, NULL, NULL, info);

	if (!com_hndl) {
		WBD_ERROR("Interface ["MACF"] Failed to initialize the communication module "
			"for changing chanspec[0x%x] through ACSD\n",
			ETHERP_TO_MACF(interface->InterfaceId), chspec);
		ret = WBDE_COM_ERROR;
		goto end;
	}

	/* Send the command to ACSD */
	ret = wbd_com_connect_and_send_cmd(com_hndl, ACSD_DEFAULT_CLI_PORT,
		WBD_LOOPBACK_IP, tmpbuf, NULL);
	WBD_CHECK_MSG("Interface["MACF"] Failed to send change Chanspec[0x%x] to ACSD. Error: %s\n",
		ETHERP_TO_MACF(interface->InterfaceId), chspec, wbderrorstr(ret));

	/* For MultiAP certification, we use channels 6, 36 and 149. In dual band case, from
	 * sigma script we set the default channel to 6 and 36 using wlx_chanspec NVRAM. So,
	 * when device needs to work in 5GH(149) and after the channel selection required, if the
	 * device does rc restart to save some configuration it comes up in 5GL(36) channel.
	 * So, setting the wlx_chanspec NVRAM when setting the channel in case of MultiAP
	 * certification.
	 */
	if (WBD_IS_MAP_CERT(info->flags)) {
		if (wf_chspec_ntoa(chspec, strchspec)) {
			blanket_nvram_prefix_set(prefix, NVRAM_CHANSPEC, strchspec);
			WBD_INFO("set %s%s to %s\n", prefix, NVRAM_CHANSPEC, strchspec);
		} else {
			WBD_ERROR("ifname %s Failed to convert chspec 0x%x to string\n",
				nvval, chspec);
		}
	}

end:
	if (ret == WBDE_OK) {
		wbd_slave_wait_set_chanspec(interface, chspec);
	}
	if (com_hndl) {
		wbd_com_deinit(com_hndl);
	}

	WBD_EXIT();
	return ret;
}

#ifdef WLHOSTFBT
/* Get FBT_CONFIG_REQ Data from 1905 Device */
static int
wbd_slave_get_fbt_config_req_data(wbd_cmd_fbt_config_t *fbt_config_req)
{
	int ret = WBDE_OK;
	struct ether_addr bssid, br_mac;
	wbd_fbt_bss_entry_t new_fbt_bss;
	unsigned char al_mac[MAC_ADDR_LEN], map = 0;
	char wbd_ifnames[NVRAM_MAX_VALUE_LEN] = {0};
	char var_intf[IFNAMSIZ] = {0}, ifname[IFNAMSIZ] = {0}, *next_intf;
	unsigned short mdid = 0;
	i5_dm_device_type *controller_device = NULL;
	WBD_ENTER();

	/* Initialize fbt_config_req's Total item count in caller  */
	WBD_INFO("Get FBT Config Request Data\n");

	WBD_SAFE_GET_I5_CTLR_DEVICE(controller_device, &ret);

	/* Get DeviceID AL MAC Address */
	memcpy(al_mac, ieee1905_get_al_mac(), sizeof(al_mac));

	/* Get bridge address */
	wbd_get_bridge_mac(&br_mac);

	WBDSTRNCPY(wbd_ifnames, blanket_nvram_safe_get(WBD_NVRAM_IFNAMES),
		sizeof(wbd_ifnames));

	/* Traverse wbd_ifnames for each ifname */
	foreach(var_intf, wbd_ifnames, next_intf) {

		char prefix[IFNAMSIZ];

		/* Copy interface name temporarily */
		WBDSTRNCPY(ifname, var_intf, sizeof(ifname));

		/* Get prefix of the interface from Driver */
		blanket_get_interface_prefix(ifname, prefix, sizeof(prefix));

		/* Get the BSSID/MAC of the BSS */
		wbd_ether_atoe(blanket_nvram_prefix_safe_get(prefix, NVRAM_HWADDR), &bssid);

		/* Get IEEE1905_MAP_FLAG_XXX flag value for this BSS */
		map = wbd_get_map_flags(prefix);

		/* Loop for below activity, only for Fronthaul BSS */
		if (!I5_IS_BSS_FRONTHAUL(map)) {
			continue;
		}

		/* If FBT not possible on this BSS, go for next */
		if (!wbd_is_fbt_possible(prefix)) {
			continue;
		}

		/* If MDID = 0 Set from M2, No need to send FBT Request from Agent */
		mdid = (unsigned short)blanket_get_config_val_int(prefix, NVRAM_FBT_MDID, 0);
		if (I5_IS_SPLIT_VNDR_MSG(controller_device->flags) && !mdid) {
			continue;
		}

		WBD_INFO("Adding FBT Data to FBT_CONFIG_REQ : BSS[%s]\n", ifname);

		/* Fill FBT Data for this BSS & Save FBT info of this BSS */
		memset(&new_fbt_bss, 0, sizeof(new_fbt_bss));
		memset(&(new_fbt_bss.fbt_info), 0, sizeof(new_fbt_bss.fbt_info));

		/* Fill Blanket ID, AL_MAC for this BSS */
		new_fbt_bss.blanket_id = I5_IS_BSS_GUEST(map) ?
			WBD_BKT_ID_BR1 : WBD_BKT_ID_BR0;
		memcpy(new_fbt_bss.al_mac, al_mac, sizeof(new_fbt_bss.al_mac));

		/* Get/Generate R0KH_ID for this BSS */
		wbd_get_r0khid(prefix, new_fbt_bss.r0kh_id, sizeof(new_fbt_bss.r0kh_id), 1);
		new_fbt_bss.len_r0kh_id = strlen(new_fbt_bss.r0kh_id);

		/* Get/Generate R0KH_KEY for this BSS */
		wbd_get_r0khkey(prefix, new_fbt_bss.r0kh_key, sizeof(new_fbt_bss.r0kh_key), 1);
		new_fbt_bss.len_r0kh_key = strlen(new_fbt_bss.r0kh_key);

		/* Add mdid */
		new_fbt_bss.fbt_info.mdid = mdid;

		/* Prepare a new FBT BSS item for Slave's FBT Request List */
		memcpy(new_fbt_bss.bssid, (unsigned char*)&bssid,
			sizeof(new_fbt_bss.bssid));
		memcpy(new_fbt_bss.bss_br_mac, (unsigned char*)&br_mac,
			sizeof(new_fbt_bss.bss_br_mac));

		WBD_INFO("Adding new entry to FBT_CONFIG_REQ : "
			"Blanket[BR%d] Device["MACDBG"] "
			"BRDG["MACDBG"] BSS["MACDBG"] "
			"R0KH_ID[%s] LEN_R0KH_ID[%d] "
			"R0KH_Key[%s] LEN_R0KH_Key[%d] MDID[%d]\n",
			new_fbt_bss.blanket_id,
			MAC2STRDBG(new_fbt_bss.al_mac),
			MAC2STRDBG(new_fbt_bss.bss_br_mac),
			MAC2STRDBG(new_fbt_bss.bssid),
			new_fbt_bss.r0kh_id, new_fbt_bss.len_r0kh_id,
			new_fbt_bss.r0kh_key, new_fbt_bss.len_r0kh_key,
			new_fbt_bss.fbt_info.mdid);

		/* Add a FBT BSS item in a Slave's FBT Request List, Total item count++ */
		wbd_add_item_in_fbt_cmdlist(&new_fbt_bss, fbt_config_req, NULL, FALSE);
	}

end:
	WBD_EXIT();
	return ret;
}

/* Send 1905 Vendor Specific FBT_CONFIG_REQ command, from Agent to Controller */
int
wbd_slave_send_fbt_config_request_cmd()
{
	int ret = WBDE_OK;
	ieee1905_vendor_data vndr_msg_data;
	wbd_cmd_fbt_config_t *fbt_config_req; /* FBT Config Request Data */
	i5_dm_device_type *controller_device = NULL;
	wbd_info_t *info = wbd_get_ginfo();
	WBD_ENTER();

	memset(&vndr_msg_data, 0, sizeof(vndr_msg_data));

	WBD_SAFE_GET_I5_CTLR_DEVICE(controller_device, &ret);

	/* If the Controller is Non-BROADCOM, skip Sending FBT Request, Set map_agent_configured */
	if (!I5_IS_VENDOR_BROADCOM(controller_device->flags) &&
		(!(info->flags & WBD_INFO_FLAGS_RC_RESTART))) {
#ifdef BCM_APPEVENTD
		/* Raise and send MAP init end event to appeventd. */
		wbd_appeventd_map_init(APP_E_WBD_SLAVE_MAP_INIT_END,
			(struct ether_addr*)ieee1905_get_al_mac(), MAP_INIT_END,
			MAP_APPTYPE_SLAVE);
#endif /* BCM_APPEVENTD */
		blanket_nvram_prefix_set(NULL, NVRAM_MAP_AGENT_CONFIGURED, "1");
		WBD_INFO("Controller is NOT BRCM vendor, "
			"skip fbt config ! set map_agent_configured.");
		goto end;
	}

	/* Remove all Previous FBT Information data items on this Device */
	wbd_ds_glist_cleanup(&(info->wbd_fbt_config.entry_list));

	/* Initialize FBT Config Request Data, Utilize/Update Slave's FBT Config Object */
	fbt_config_req = &(info->wbd_fbt_config);
	memset(fbt_config_req, 0, sizeof(*fbt_config_req));
	wbd_ds_glist_init(&(fbt_config_req->entry_list));

	/* Check if FBT is possible on this 1905 Device or not */
	ret = wbd_ds_is_fbt_possible_on_agent();
	WBD_ASSERT_MSG("Device["MACDBG"]: %s\n",
		MAC2STRDBG(ieee1905_get_al_mac()), wbderrorstr(ret));

	/* Allocate Dynamic mem for Vendor data from App */
	vndr_msg_data.vendorSpec_msg =
		(unsigned char *)wbd_malloc(IEEE1905_MAX_VNDR_DATA_BUF, &ret);
	WBD_ASSERT();

	/* Fill Destination AL_MAC in Vendor data */
	memcpy(vndr_msg_data.neighbor_al_mac,
		controller_device->DeviceId, IEEE1905_MAC_ADDR_LEN);

	/* Get FBT Config Request Data from 1905 Device */
	wbd_slave_get_fbt_config_req_data(fbt_config_req);
	/* Dont send empty FBT config request */
	if (fbt_config_req->entry_list.count <= 0) {
		WBD_INFO("Empty FBT request Data, Dont send FBT config request\n");
		goto end;
	}

	/* Encode Vendor Specific TLV for Message : FBT_CONFIG_REQ to send */
	wbd_tlv_encode_fbt_config_request((void *)fbt_config_req,
		vndr_msg_data.vendorSpec_msg, &vndr_msg_data.vendorSpec_len);

	WBD_INFO("Send FBT_CONFIG_REQ from Device["MACDBG"] to Device["MACDBG"]\n",
		MAC2STRDBG(ieee1905_get_al_mac()), MAC2STRDBG(controller_device->DeviceId));

	/* Send Vendor Specific Message : FBT_CONFIG_REQ */
	wbd_slave_send_brcm_vndr_msg(&vndr_msg_data);

	WBD_CHECK_ERR_MSG(WBDE_DS_1905_ERR, "Failed to send "
		"FBT_CONFIG_REQ from Device["MACDBG"], Error : %s\n",
		MAC2STRDBG(ieee1905_get_al_mac()), wbderrorstr(ret));

	/* FBT_CONFIG_REQ is sent to the controller. So, update the flag accordingly */
	wbd_get_ginfo()->wbd_slave->flags |= WBD_BKT_SLV_FLAGS_FBT_REQ_SENT;
	wbd_get_ginfo()->wbd_slave->flags |= WBD_BKT_SLV_FLAGS_FBT_EXCHNG_TRIGRD;

	wbd_slave_create_fbt_resp_watchdog_timer();

end:
	if (vndr_msg_data.vendorSpec_msg) {
		free(vndr_msg_data.vendorSpec_msg);
	}
	WBD_EXIT();
	return ret;
}

/* FBT Config Response Vendor Watchdog timer Callback Function */
void
wbd_slave_fbt_resp_watchdog_timer_cb()
{
	wbd_get_ginfo()->flags &= ~WBD_INFO_FLAGS_WD_FBT_RESP_TM;

	if (atoi(blanket_nvram_prefix_safe_get(NULL, NVRAM_MAP_AGENT_CONFIGURED))) {
		WBD_INFO("FBT config transaction has been complete, "
			"no need to send config request\n");
		return;
	}
	wbd_slave_send_fbt_config_request_cmd();
}

/* Create FBT Config Response Vendor Message Watchdog Timer */
void
wbd_slave_create_fbt_resp_watchdog_timer()
{
	int ret = WBDE_OK;
	WBD_ENTER();

	/* If all_ap_configured timer already created, don't create again */
	if (wbd_get_ginfo()->flags & WBD_INFO_FLAGS_WD_FBT_RESP_TM) {
		WBD_INFO(" FBT config resp watchdog timer is already created\n");
		goto end;
	}

	/* Add the timer */
	WBD_INFO("create FBT config resp watchdog timer\n");
	ret = wbd_add_timers(wbd_get_ginfo()->hdl, wbd_get_ginfo(),
		WBD_SEC_MICROSEC(wbd_get_ginfo()->max.tm_wd_fbt_resp),
		wbd_slave_fbt_resp_watchdog_timer_cb, 0);

	/* If Add Timer Succeeded */
	if (ret == WBDE_OK) {
		/* Set all_ap_configured timer created flag, else error */
		wbd_get_ginfo()->flags |= WBD_INFO_FLAGS_WD_FBT_RESP_TM;
	} else {
		WBD_WARNING("Failed to create config FBT resp watchdog timer Interval[%d]\n",
			wbd_get_ginfo()->max.tm_wd_fbt_resp);
	}

end:
	WBD_EXIT();
	return;
}

/* Add r0kh and r1kh entry to hostapd at run time using hostapd_cli set command */
static void
wbd_slave_set_r0kh_r1kh_in_hapd(char* ifname, wbd_fbt_bss_entry_t *fbt_bss_info,
	wbd_fbt_bss_entry_t *fbt_data)
{
	char r0kh[WBD_MAX_BUF_256];
	char r1kh[WBD_MAX_BUF_256];
	char cmd[2 * WBD_MAX_BUF_256] = {0};
	char fbt_r0_hex_key[WBD_FBT_KH_KEY_LEN * 2];
	char fbt_r1_hex_key[WBD_FBT_KH_KEY_LEN * 2];
	char new_value[WBD_MAX_BUF_256] = {0};

	memset(r0kh, 0, sizeof(r0kh));
	memset(r1kh, 0, sizeof(r1kh));

	/* Prepeare r0kh for hostapd_cli set command */
	strcat(r0kh, wbd_ether_etoa(fbt_data->bssid, new_value));
	strcat(r0kh, " ");
	strcat(r0kh, fbt_data->r0kh_id);
	strcat(r0kh, " ");

	memset(fbt_r0_hex_key, 0, sizeof(fbt_r0_hex_key));
	bytes_to_hex((uchar *) fbt_data->r0kh_key, strlen(fbt_data->r0kh_key),
			(uchar *) fbt_r0_hex_key, sizeof(fbt_r0_hex_key));
	strcat(r0kh, fbt_r0_hex_key);

	/* Command for r0kh set */
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s set r0kh \"%s\"", ifname, r0kh);
	WBD_INFO("%s\n", cmd);
	system(cmd);

	/* Prepeare r1kh for hostapd_cli set command */
	strcat(r1kh, wbd_ether_etoa(fbt_data->bssid, new_value));
	strcat(r1kh, " ");
	strcat(r1kh, wbd_ether_etoa(fbt_data->bssid, new_value));
	strcat(r1kh, " ");

	memset(fbt_r1_hex_key, 0, sizeof(fbt_r1_hex_key));
	bytes_to_hex((uchar *) fbt_bss_info->r0kh_key, strlen(fbt_bss_info->r0kh_key),
		(uchar *) fbt_r1_hex_key, sizeof(fbt_r1_hex_key));
	strcat(r1kh, fbt_r1_hex_key);

	/* Command for r1kh set */
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s set r1kh \"%s\"", ifname, r1kh);
	WBD_INFO("%s\n", cmd);
	system(cmd);
}

/* Create the NVRAMs required for FBT for Neighbor APs */
uint32
wbd_slave_create_peer_fbt_nvrams(char* ifname, wbd_fbt_bss_entry_t *fbt_bss_info,
	wbd_cmd_fbt_config_t *fbt_config_resp, int *error)
{
	int ret = WBDE_OK;
	char prefix[IFNAMSIZ] = {0};
	dll_t *fbt_item_p = NULL;
	wbd_fbt_bss_entry_t *fbt_data = NULL;
	char new_value[WBD_MAX_BUF_256] = {0}, *nvval;
	char ap_name[WBD_MAX_BUF_256+2] = {0}, ap_name_base[WBD_MAX_BUF_256] = {0};
	char data[WBD_MAX_BUF_256] = {0};
	uint32 rc_flags = 0, cur_ap_rc_flags;
	char wlx_fbt_aps[NVRAM_MAX_VALUE_LEN] = {0}, old_wlx_fbt_aps[NVRAM_MAX_VALUE_LEN] = {0};
	i5_dm_bss_type *i5_peer_bss = NULL;
	ieee1905_ssid_type self_ssid;
	WBD_ENTER();

	memset(data, 0, sizeof(data));

	/* Validate arg */
	WBD_ASSERT_ARG(ifname, WBDE_INV_ARG);
	WBD_ASSERT_ARG(fbt_bss_info, WBDE_INV_ARG);
	WBD_ASSERT_ARG(fbt_config_resp, WBDE_INV_ARG);

	/* Get prefix of the interface from Driver */
	blanket_get_interface_prefix(ifname, prefix, sizeof(prefix));

	/* Fetch NVRAM FBT_APS for this BSS */
	WBDSTRNCPY(old_wlx_fbt_aps, blanket_nvram_prefix_safe_get(prefix, NVRAM_FBT_APS),
		sizeof(old_wlx_fbt_aps));
	wlx_fbt_aps[0] = '\0';

	/* Fetch SSID for this local BSS */
	nvval = blanket_nvram_prefix_safe_get(prefix, NVRAM_SSID);
	memcpy(self_ssid.SSID, nvval, sizeof(self_ssid.SSID));
	self_ssid.SSID_len = strlen(nvval);

	/* Travese FBT Config Response List items */
	foreach_glist_item(fbt_item_p, fbt_config_resp->entry_list) {

		fbt_data = (wbd_fbt_bss_entry_t *)fbt_item_p;
		cur_ap_rc_flags = 0;

		/* Loop for below activity, only for BSSID other than Self BSS's BSSID */
		if (memcmp(fbt_bss_info->bssid, fbt_data->bssid, MAC_ADDR_LEN) == 0) {
			continue;
		}

		/* Find bssid of FBT Config Response List item, in Topology */
		i5_peer_bss = wbd_ds_get_i5_bss_in_topology(fbt_data->bssid, &ret);
		if (ret != WBDE_OK) {
			WBD_DEBUG("Peer BSS["MACDBG"] not found. Skipping Peer NVRAM "
				"Save for this Peer BSS.\n", MAC2STRDBG(fbt_data->bssid));
			continue;
		}

		/* Loop for below activity, only for matching SSID */
		if (!(WBD_SSIDS_MATCH(self_ssid, i5_peer_bss->ssid))) {
			WBD_DEBUG("Peer BSS["MACDBG"] SSID not matching. Skipping Peer NVRAM "
				"Save for this Peer BSS.\n", MAC2STRDBG(fbt_data->bssid));
			continue;
		}

		/* AP_NAME_BASE = Prefix + R0KH_ID for this ITER_AP, and Generate AP_NAME */
		snprintf(ap_name_base, sizeof(ap_name_base), "%s%s", prefix, fbt_data->r0kh_id);
		memset(ap_name, 0, sizeof(ap_name));
		snprintf(ap_name, sizeof(ap_name), "%s_", ap_name_base);

		/* [1] Save NVRAM R0KH_ID for this BSS */
		cur_ap_rc_flags |= blanket_nvram_prefix_match_set(NULL,
			strcat_r(ap_name, NVRAM_FBT_R0KH_ID, data),
			fbt_data->r0kh_id, FALSE);

		/* [2] Save NVRAM R0KH_ID_LEN for this BSS */
		memset(new_value, 0, sizeof(new_value));
		snprintf(new_value, sizeof(new_value), "%d", fbt_data->len_r0kh_id);
		cur_ap_rc_flags |= blanket_nvram_prefix_match_set(NULL,
			strcat_r(ap_name, NVRAM_FBT_R0KH_ID_LEN, data),
			new_value, FALSE);

		/* [3] Save NVRAM R0KH_KEY for this BSS */
		cur_ap_rc_flags |= blanket_nvram_prefix_match_set(NULL,
			strcat_r(ap_name, NVRAM_FBT_R0KH_KEY, data),
			fbt_data->r0kh_key, FALSE);

		/* [4] Save NVRAM R1KH_ID for this BSS */
		memset(new_value, 0, sizeof(new_value));
		cur_ap_rc_flags |= blanket_nvram_prefix_match_set(NULL,
			strcat_r(ap_name, NVRAM_FBT_R1KH_ID, data),
			wbd_ether_etoa(fbt_data->bssid, new_value), FALSE);

		/* [5] Save NVRAM R1KH_KEY for this BSS */
		cur_ap_rc_flags |= blanket_nvram_prefix_match_set(NULL,
			strcat_r(ap_name, NVRAM_FBT_R1KH_KEY, data),
			fbt_bss_info->r0kh_key, FALSE);

		/* [6] Save NVRAM ADDR for this BSS */
		memset(new_value, 0, sizeof(new_value));
		cur_ap_rc_flags |= blanket_nvram_prefix_match_set(NULL,
			strcat_r(ap_name, NVRAM_FBT_ADDR, data),
			wbd_ether_etoa(fbt_data->bssid, new_value), FALSE);

		/* [7] Save NVRAM BR_ADDR for this BSS */
		memset(new_value, 0, sizeof(new_value));
		cur_ap_rc_flags |= blanket_nvram_prefix_match_set(NULL,
			strcat_r(ap_name, NVRAM_FBT_BR_ADDR, data),
			wbd_ether_etoa(fbt_data->bss_br_mac, new_value), FALSE);

		/* Add this ITER_AP's AP_NAME_BASE to FBT_APS NVRAM value */
		add_to_list(ap_name_base, wlx_fbt_aps, sizeof(wlx_fbt_aps));
		WBD_INFO("Adding %s in %sfbt_aps[%s]\n", ap_name_base, prefix, wlx_fbt_aps);

		/* If there is no changes in NVRAMs, we can safely think that the r0kh and r1kh
		 * entry has already loaded into the hostapd either at bootup or using
		 * previous hostapd_cli set command from WBD
		 */
		if (cur_ap_rc_flags){
			wbd_slave_set_r0kh_r1kh_in_hapd(ifname, fbt_bss_info, fbt_data);
		} else {
			WBD_INFO("%s: For peer with name[%s], No changes in peer FBT NVRAMs, "
				"No need to set it to hostapd CLI", ifname, ap_name_base);
		}
		rc_flags |= cur_ap_rc_flags;
	}

	/* [8] Save NVRAM FBT_APS for this BSS */
	if (wlx_fbt_aps[0] != '\0') {
		rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_APS,
			wlx_fbt_aps, BKT_CMP_LIST_WITH_CASE);
	}

	/* Remove all the non active FBT peer NVRAMs */
	rc_flags |= wbd_unset_nonactive_peer_fbt_nvrams(prefix, old_wlx_fbt_aps,
		wlx_fbt_aps);

	if (rc_flags) {
		rc_flags = WBD_FLG_NV_COMMIT;
	}

	WBD_INFO("%s in PEER_NVRAMs on getting new FBT_CONFIG_RESP "
		"data for BSS[%s]. Old[%s] New[%s]\n", (rc_flags) ? "Changes" : "NO Changes",
		ifname, old_wlx_fbt_aps, wlx_fbt_aps);

end:
	if (error) {
		*error = ret;
	}
	WBD_EXIT();
	return rc_flags;
}

/* Create the NVRAMs required for FBT for Self AP */
uint32
wbd_slave_create_self_fbt_nvrams(char* ifname, wbd_fbt_bss_entry_t *fbt_bss_info, int *error)
{
	int ret = WBDE_OK;
	char prefix[IFNAMSIZ] = {0};
	char r1kh_id[WBD_FBT_R0KH_ID_LEN] = {0};
	char new_value[WBD_MAX_BUF_256] = {0};
	uint32 rc_flags = 0;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(ifname, WBDE_INV_ARG);
	WBD_ASSERT_ARG(fbt_bss_info, WBDE_INV_ARG);

	/* Get prefix of the interface from Driver */
	blanket_get_interface_prefix(ifname, prefix, sizeof(prefix));

	/* [1] Save NVRAM WBD_FBT for this BSS. Add "psk2ft" in AKM */
	if (wbd_is_fbt_nvram_enabled(prefix) != WBD_FBT_DEF_FBT_ENABLED) {
		wbd_enable_fbt(prefix);
		rc_flags |= WBD_FLG_NV_COMMIT|WBD_FLG_RC_RESTART;
	}

	/* [2] Save NVRAM FBT_AP for this BSS */
	memset(new_value, 0, sizeof(new_value));
	snprintf(new_value, sizeof(new_value), "%d",
		WBD_FBT_DEF_AP);
	rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_AP,
		new_value, FALSE);

	/* [3] Save NVRAM R0KH_ID for this BSS */
	rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_R0KH_ID,
		fbt_bss_info->r0kh_id, FALSE);

	/* [4] Save NVRAM R0KH_KEY for this BSS */
	rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_R0KH_KEY,
		fbt_bss_info->r0kh_key, FALSE);

	/* [5] Get/Generate R1KH_ID for this BSS */
	wbd_get_r1khid(prefix, r1kh_id, sizeof(r1kh_id), 0);
	rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_R1KH_ID,
		r1kh_id, FALSE);

	/* [6] Save NVRAM FBT_MDID for this BSS */
	memset(new_value, 0, sizeof(new_value));
	snprintf(new_value, sizeof(new_value), "%d",
		fbt_bss_info->fbt_info.mdid);
	rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_MDID,
		new_value, FALSE);

	/* [7] Save NVRAM FBTOVERDS for this BSS */
	memset(new_value, 0, sizeof(new_value));
	snprintf(new_value, sizeof(new_value), "%d",
		(fbt_bss_info->fbt_info.ft_cap_policy & FBT_MDID_CAP_OVERDS));
	rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_OVERDS,
		new_value, FALSE);

	/* [8] Save NVRAM FBT_REASSOC_TIME for this BSS */
	memset(new_value, 0, sizeof(new_value));
	snprintf(new_value, sizeof(new_value), "%d",
		fbt_bss_info->fbt_info.tie_reassoc_interval);
	rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_REASSOC_TIME,
		new_value, FALSE);

end:
	if (rc_flags) {
		rc_flags = WBD_FLG_NV_COMMIT|WBD_FLG_RC_RESTART;
	}

	WBD_INFO("%s in SELF_NVRAMs on getting new FBT_CONFIG_RESP "
	"data for BSS[%s]\n", (rc_flags) ? "Need restart. Changes" : "NO Changes", ifname);

	if (error) {
		*error = ret;
	}
	WBD_EXIT();
	return rc_flags;
}

/* Process 1905 Vendor Specific FBT_CONFIG_RESP Message */
static int
wbd_slave_process_fbt_config_resp_cmd(unsigned char *neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len)
{
	int ret = WBDE_OK;
	wbd_cmd_fbt_config_t fbt_config_resp; /* FBT Config Response Data */
	dll_t *fbt_item_p = NULL;
	wbd_fbt_bss_entry_t *fbt_data = NULL;
	uint32 rc_flags = 0;
	struct ether_addr bssid;
	unsigned char map = 0;
	char wbd_ifnames[NVRAM_MAX_VALUE_LEN] = {0};
	char var_intf[IFNAMSIZ] = {0}, ifname[IFNAMSIZ] = {0}, *next_intf;
	wbd_info_t *info = wbd_get_ginfo();
	wbd_cmd_fbt_config_t *agnt_fbt_config = &(info->wbd_fbt_config);
	WBD_ENTER();

	/* If FBT_CONFIG_REQ is sent by this Slave, Reset bit */
	if (info->wbd_slave->flags & WBD_BKT_SLV_FLAGS_FBT_EXCHNG_TRIGRD) {
		WBD_INFO("FBT config response Triggered by a request sent by this Slave\n");
		wbd_get_ginfo()->wbd_slave->flags &= ~WBD_BKT_SLV_FLAGS_FBT_EXCHNG_TRIGRD;
	}

	/* If FBT_CONFIG_REQ is not sent to the controller, do not process the response */
	if (!(info->wbd_slave->flags & WBD_BKT_SLV_FLAGS_FBT_REQ_SENT)) {
		WBD_WARNING("Not Processing FBT config response as a request was never sent\n");
		WBD_EXIT();
		return ret;
	}

	/* Initialize FBT Config Response Data */
	memset(&fbt_config_resp, 0, sizeof(fbt_config_resp));
	wbd_ds_glist_init(&(fbt_config_resp.entry_list));

	/* Decode Vendor Specific TLV for Message : FBT_CONFIG_RESP on receive */
	ret = wbd_tlv_decode_fbt_config_response((void *)&fbt_config_resp, tlv_data, tlv_data_len);
	WBD_ASSERT_MSG("Failed to decode FBT Config Response From DEVICE["MACDBG"]\n",
		MAC2STRDBG(neighbor_al_mac));

	/* Travese FBT Config Response List items */
	foreach_glist_item(fbt_item_p, fbt_config_resp.entry_list) {

		fbt_data = (wbd_fbt_bss_entry_t *)fbt_item_p;

		WBD_INFO("From DEVICE["MACDBG"] BRDG["MACDBG"] BSS["MACDBG"] "
			"Received FBT_CONFIG_RESP : "
			"R0KH_ID[%s] LEN_R0KH_ID[%d] "
			"R0KH_Key[%s] LEN_R0KH_Key[%d] "
			"MDID[%d] FT_CAP[%d] FT_REASSOC[%d]\n", MAC2STRDBG(neighbor_al_mac),
			MAC2STRDBG(fbt_data->bss_br_mac), MAC2STRDBG(fbt_data->bssid),
			fbt_data->r0kh_id, fbt_data->len_r0kh_id,
			fbt_data->r0kh_key, fbt_data->len_r0kh_key,
			fbt_data->fbt_info.mdid,
			fbt_data->fbt_info.ft_cap_policy,
			fbt_data->fbt_info.tie_reassoc_interval);
	}

	WBDSTRNCPY(wbd_ifnames, blanket_nvram_safe_get(WBD_NVRAM_IFNAMES),
		sizeof(wbd_ifnames));

	/* Traverse wbd_ifnames for each ifname */
	foreach(var_intf, wbd_ifnames, next_intf) {

		char prefix[IFNAMSIZ];
		wbd_fbt_bss_entry_t *matching_fbt_resp_info = NULL, *matching_fbt_bss_info = NULL;

		/* Copy interface name temporarily */
		WBDSTRNCPY(ifname, var_intf, sizeof(ifname));

		/* Get prefix of the interface from Driver */
		blanket_get_interface_prefix(ifname, prefix, sizeof(prefix));

		/* Get the BSSID/MAC of the BSS */
		wbd_ether_atoe(blanket_nvram_prefix_safe_get(prefix, NVRAM_HWADDR), &bssid);

		/* Get IEEE1905_MAP_FLAG_XXX flag value for this BSS */
		map = wbd_get_map_flags(prefix);

		/* Loop for below activity, only for Fronthaul BSS */
		if (!I5_IS_BSS_FRONTHAUL(map)) {
			continue;
		}

		/* If FBT not possible on this BSS, go for next */
		if (!wbd_is_fbt_possible(prefix)) {
			continue;
		}

		/* Find matching fbt_info for this bssid, In FBT_CONFIG_RESP */
		matching_fbt_resp_info = wbd_find_fbt_bss_item_for_bssid((unsigned char*)&bssid,
			&fbt_config_resp);
		if (!matching_fbt_resp_info) {
			/* Disable FBT, if it is not already disabled */
			int fbt = blanket_get_config_val_int(prefix, WBD_NVRAM_FBT,
				WBD_FBT_NOT_DEFINED);
			if (fbt != WBD_FBT_DEF_FBT_DISABLED) {
				WBD_INFO("No FBT data for BSS["MACDBG"] prefix[%s]. Disable FBT\n",
					MAC2STRDBG((unsigned char*)&bssid), prefix);
				rc_flags |= wbd_dap_deinit_fbt_nvram_config(prefix);
			}
			continue;
		}

		/* Find matching fbt_info for this bssid, In Slave's FBT Data */
		matching_fbt_bss_info = wbd_find_fbt_bss_item_for_bssid((unsigned char*)&bssid,
			agnt_fbt_config);
		if (!matching_fbt_bss_info) {
			continue;
		}

		WBD_INFO("Updating FBT Data from FBT_CONFIG_RESP : BSS[%s]\n", ifname);

		/* Store matching fbt_info details to Vendor Specific Data */
		matching_fbt_bss_info->fbt_info.mdid =
			matching_fbt_resp_info->fbt_info.mdid;
		matching_fbt_bss_info->fbt_info.ft_cap_policy =
			matching_fbt_resp_info->fbt_info.ft_cap_policy;
		matching_fbt_bss_info->fbt_info.tie_reassoc_interval =
			matching_fbt_resp_info->fbt_info.tie_reassoc_interval;

		/* Create the NVRAMs required for FBT for Self AP */
		rc_flags |= wbd_slave_create_self_fbt_nvrams(ifname, matching_fbt_bss_info, &ret);

		/* Create the NVRAMs required for FBT for Neighbor APs */
		rc_flags |= wbd_slave_create_peer_fbt_nvrams(ifname, matching_fbt_bss_info,
			&fbt_config_resp, &ret);
	}

end:
	/* Remove all FBT Config Response data items */
	wbd_ds_glist_cleanup(&(fbt_config_resp.entry_list));

	/* One time rc_restart() for SELF_NVRAMs */
	if (rc_flags & WBD_FLG_RC_RESTART) {
		WBD_INFO("Creating rc restart timer\n");
		wbd_slave_create_rc_restart_timer(info);
		info->flags |= WBD_INFO_FLAGS_RC_RESTART;

	/* One time nvram_commit() for PEER_NVRAMs */
	} else if (rc_flags & WBD_FLG_NV_COMMIT) {
		nvram_commit();
	}

	/* Check if Restart is done if no more restart required.
	 * Whenever new device is connected with controller
	 * this is raise the event and set the NVRAM.
	 * So to avoid setting each time, check and if NVRAM is not set
	 * then only set the NVRAM and rasie the event.
	 */
	if (!(info->flags & WBD_INFO_FLAGS_RC_RESTART) &&
		(atoi(blanket_nvram_safe_get(NVRAM_MAP_AGENT_CONFIGURED)) == 0)) {
#ifdef BCM_APPEVENTD
		/* Raise and send MAP init end event to appeventd. */
		wbd_appeventd_map_init(APP_E_WBD_SLAVE_MAP_INIT_END,
			(struct ether_addr*)ieee1905_get_al_mac(), MAP_INIT_END,
			MAP_APPTYPE_SLAVE);
#endif /* BCM_APPEVENTD */
		blanket_nvram_prefix_set(NULL, NVRAM_MAP_AGENT_CONFIGURED, "1");
	}

	WBD_EXIT();
	return ret;
}

/* Create nvrams and enable fbt on this device */
int
wbd_slave_set_fbt_on_agent_using_m2(wbd_info_t *info)
{
	int ret = WBDE_OK;
	struct ether_addr bssid;
	unsigned char map = 0;
	char *wbd_ifnames = NULL;
	char var_intf[IFNAMSIZ] = {0}, *next_intf;
	unsigned short mdid = 0;
	char r0kh_id[WBD_FBT_R0KH_ID_LEN], r0kh_key[WBD_FBT_KH_KEY_LEN];
	char r1kh_id[WBD_FBT_R0KH_ID_LEN] = {0};
	char new_value[WBD_MAX_BUF_256] = {0};
	uint32 rc_flags = 0;

	WBD_ENTER();

	wbd_ifnames = blanket_nvram_safe_get(WBD_NVRAM_IFNAMES);
	if (wbd_ifnames == NULL) {
		WBD_INFO("wbd_ifnames is empty");
		return ret;
	}

	/* Traverse wbd_ifnames for each ifname */
	foreach(var_intf, wbd_ifnames, next_intf) {

		char prefix[IFNAMSIZ];

		/* Get prefix of the interface from Driver */
		blanket_get_interface_prefix(var_intf, prefix, sizeof(prefix));

		/* Get the BSSID/MAC of the BSS */
		wbd_ether_atoe(blanket_nvram_prefix_safe_get(prefix, NVRAM_HWADDR), &bssid);

		/* Get IEEE1905_MAP_FLAG_XXX flag value for this BSS */
		map = wbd_get_map_flags(prefix);

		/* Loop for below activity, only for Fronthaul BSS */
		if (!I5_IS_BSS_FRONTHAUL(map)) {
			continue;
		}

		/* If FBT not possible on this BSS, go for next */
		if (!wbd_is_fbt_possible(prefix)) {
			continue;
		}

		mdid = (unsigned short)blanket_get_config_val_int(prefix, NVRAM_FBT_MDID, 0);
		if (!mdid) {
			WBD_INFO("No FBT data for BSS["MACDBG"] prefix[%s]. Disable FBT\n",
				MAC2STRDBG((unsigned char*)&bssid), prefix);
			if (wbd_is_fbt_nvram_enabled(prefix) != WBD_FBT_DEF_FBT_DISABLED) {
				rc_flags |= wbd_dap_deinit_fbt_nvram_config(prefix);
			}
			continue;
		}

		/* Create Self_NVRAMs : the NVRAMs required for FBT for Self AP */
		/* [1] Save NVRAM WBD_FBT for this BSS. Add "psk2ft and/or saeft" in AKM */
		if (wbd_is_fbt_nvram_enabled(prefix) != WBD_FBT_DEF_FBT_ENABLED) {
			wbd_enable_fbt(prefix);
			rc_flags |= WBD_FLG_RC_RESTART;
		}

		/* [2] Save NVRAM FBT_AP for this BSS */
		memset(new_value, 0, sizeof(new_value));
		snprintf(new_value, sizeof(new_value), "%d", WBD_FBT_DEF_AP);
		rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_AP, new_value, FALSE);

		/* [3] Get/Generate R0KH_ID for this BSS */
		wbd_get_r0khid(prefix, r0kh_id, sizeof(r0kh_id), 0);
		rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_R0KH_ID,
			r0kh_id, FALSE);

		/* [4] Get/Generate R0KH_KEY for this BSS */
		wbd_get_r0khkey(prefix, r0kh_key, sizeof(r0kh_key), 0);
		rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_R0KH_KEY,
			r0kh_key, FALSE);

		/* [5] Get/Generate R1KH_ID for this BSS */
		wbd_get_r1khid(prefix, r1kh_id, sizeof(r1kh_id), 0);
		rc_flags |= blanket_nvram_prefix_match_set(prefix, NVRAM_FBT_R1KH_ID,
			r1kh_id, FALSE);

		WBD_INFO("Enabling FBT for BSS["MACF"] R0KH_ID[%s] R0KH_Key[%s] mdid[%d]\n",
			ETHER_TO_MACF(bssid), r0kh_id, r0kh_key, mdid);
	}

	/* One time rc_restart() for SELF_NVRAMs */
	if (rc_flags & WBD_FLG_RC_RESTART) {
		WBD_INFO("Creating rc restart timer\n");
		wbd_slave_create_rc_restart_timer(info);
		info->flags |= WBD_INFO_FLAGS_RC_RESTART;
	}

	/* Check if Restart is done if no more restart required.
	 * Whenever new device is connected with controller
	 * this is raise the event and set the NVRAM.
	 * So to avoid setting each time, check and if NVRAM is not set
	 * then only set the NVRAM and rasie the event.
	 */
	if (!(info->flags & WBD_INFO_FLAGS_RC_RESTART) &&
		(atoi(blanket_nvram_safe_get(NVRAM_MAP_AGENT_CONFIGURED)) == 0)) {
#ifdef BCM_APPEVENTD
		/* Raise and send MAP init end event to appeventd. */
		wbd_appeventd_map_init(APP_E_WBD_SLAVE_MAP_INIT_END,
			(struct ether_addr*)ieee1905_get_al_mac(), MAP_INIT_END,
			MAP_APPTYPE_SLAVE);
#endif /* BCM_APPEVENTD */
		blanket_nvram_prefix_set(NULL, NVRAM_MAP_AGENT_CONFIGURED, "1");
	}

	WBD_EXIT();
	return ret;
}
#endif /* WLHOSTFBT */

#if defined(MULTIAPR2)
/* Send Requested { STORED } Channel Scan Report to Controller */
static int
wbd_slave_process_requested_stored_channel_scan(unsigned char *src_al_mac,
	ieee1905_chscan_req_msg *chscan_req, unsigned char in_status_code)
{
	WBD_ENTER();

	WBD_INFO("Sending Requested { STORED } Channel Scan Report Message to ["MACF"]\n",
		ETHERP_TO_MACF(src_al_mac));

	/* Send Channel Scan Report Message to a Multi AP Device */
	if (chscan_req) {
		ieee1905_send_requested_stored_channel_scan(chscan_req, in_status_code);
	}

	WBD_EXIT();
	return WBDE_OK;
}
#define ERR_ESCAN_RUN_PER_RADIO() \
	do { \
		if (ifr->scan_type == I5_CHSCAN_REQ_FRESH) { \
			wbd_slave_send_chscan_report_per_radio(self_ifr, \
				&ifr_vndr_data->last_fresh_chscan_req, 0); \
		} \
		goto cleanup_timer_arg; \
	} while (0)

/* Run the escan for radio */
static void
wbd_slave_escan_run_per_radio(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK;
	i5_dm_device_type *selfdevice = i5DmGetSelfDevice();
	wbd_escan_per_radio_arg_t *chscan_ifr_arg = (wbd_escan_per_radio_arg_t *)arg;
	i5_dm_interface_type *self_ifr = NULL;
	wbd_ifr_item_t *ifr_vndr_data = NULL;
	ieee1905_per_radio_opclass_list *ifr = NULL;
	int params_size = 0;
	void *params = NULL;
	wl_escan_params_v2_t *params_v2 = NULL;
	wl_escan_params_v1_t *params_v1 = NULL;
	uint16 version = WL_SCAN_VERSION_T_VERSION;
	wl_scan_version_t ver;
	WBD_ENTER();

	/* Validate Timer Arg */
	WBD_ASSERT_ARG(chscan_ifr_arg, WBDE_INV_ARG);

	/* Unfold Timer Arg */
	self_ifr = i5DmFindInterfaceFromIfname(selfdevice, chscan_ifr_arg->ifname,
		strlen(chscan_ifr_arg->ifname));
	WBD_ASSERT_ARG(self_ifr, WBDE_INV_ARG);

	ifr_vndr_data = (wbd_ifr_item_t*)self_ifr->vndr_data;
	WBD_ASSERT_ARG(ifr_vndr_data, WBDE_INV_ARG);

	ifr = &chscan_ifr_arg->ifr;

	/* get the scan version to decide which scan_version to use */
	if (blanket_get_escan_ver(self_ifr->ifname, &ver) == WBDE_OK) {
		if (ver.scan_ver_major == WL_SCAN_VERSION_MAJOR_V2) {
			version = WL_SCAN_VERSION_MAJOR_V2;
		}
	} else {
		version = WL_SCAN_VERSION_T_VERSION;
	}

	if (version == WL_SCAN_VERSION_MAJOR_V2) {
		params_size += (WL_SCAN_PARAMS_FIXED_SIZE_V2 +
			OFFSETOF(wl_escan_params_v2_t, params)) +
			(WL_NUMCHANNELS * sizeof(uint16));
	} else {
		params_size += (WL_SCAN_PARAMS_FIXED_SIZE_V1 +
			OFFSETOF(wl_escan_params_v1_t, params)) +
			(WL_NUMCHANNELS * sizeof(uint16));
	}
	params_size += WL_SCAN_PARAMS_SSID_MAX * sizeof(wlc_ssid_t);

	params = malloc(params_size);
	if (params == NULL) {
		fprintf(stderr, "Error allocating %d bytes for scan params\n", params_size);
		ERR_ESCAN_RUN_PER_RADIO();
	}

	memset(params, 0, params_size);
	params_v1 = (wl_escan_params_v1_t*)params;
	if (version == WL_SCAN_VERSION_MAJOR_V2) {
		params_v2 = (wl_escan_params_v2_t*)params;
	}

	/* Update Channel Scan Request's Scan_Required Flag */
	i5DmUpdateChScanRequiredPerRadio(selfdevice, ifr);

	/* Save Current Scan Request as Previous for this Radio */
	i5DmCopyChScanRequestPerRadio(&ifr_vndr_data->last_fresh_chscan_req, ifr);

	/* Prepare scan params from Channel Scan Request */
	if ((version == WL_SCAN_VERSION_MAJOR_V2))
		ret = blanket_escan_prep_cs(self_ifr->ifname, ifr, &params_v2->params, &params_size,
			version);
	else
		ret = blanket_escan_prep_cs(self_ifr->ifname, ifr, &params_v1->params, &params_size,
			version);
	if (ret != WBDE_OK) {
		ERR_ESCAN_RUN_PER_RADIO();
	}

	/* Set Flag indicating Running a Requested Channel Scan on this radio */
	I5DmSetChScanRunning(self_ifr, ifr->scan_type, I5_CHSCAN_RUNNING);

	if (version == WL_SCAN_VERSION_MAJOR_V2) {
		params_v2->version = htod32(ESCAN_REQ_VERSION_V2);
		params_size += (uint)(uintptr)&((wl_escan_params_v2_t *)0)->params;
	} else  {
		params_v1->version = htod32(ESCAN_REQ_VERSION_V1);
		params_size += (uint)(uintptr)&((wl_escan_params_v1_t *)0)->params;
	}
		params_v1->action = htod16(WL_SCAN_ACTION_START);
#ifdef __linux__
		srand((unsigned)time(NULL));
		params_v1->sync_id = rand() & 0xffff;
#else
		params_v1->sync_id = 4321;
#endif /* __linux__ */
		/* Store Sync ID for this Interface, On EScan Start, To cmp in Escan Results */
		ifr_vndr_data->chscan_sync_id = params_v1->sync_id;
		params_v1->sync_id = htod16(params_v1->sync_id);

	/* Start the escan now for this radio : EScan Start = FALIED */
	if ((version == WL_SCAN_VERSION_MAJOR_V2))  {
		ret = blanket_escan_start(self_ifr->ifname, params_v2, params_size);
	} else {
		ret = blanket_escan_start(self_ifr->ifname, params_v1, params_size);
	}
	if (ret != WBDE_OK) {

		double laps_seconds;
		time_t now = time(NULL);
		bool retry_valid = FALSE;

		WBD_ERROR("Ifname[%s] sync_id[%d] %s Scan Start : Failed\n", self_ifr->ifname,
			ifr_vndr_data->chscan_sync_id, I5_PRINT_CHSCAN_TYPE(ifr->scan_type));

		/* Set Flag indicating Running a Requested Channel Scan on this radio */
		I5DmSetChScanRunning(self_ifr, ifr->scan_type, I5_CHSCAN_STOP);

		/* Reset Sync ID for this Interface, On EScan Start Failed */
		ifr_vndr_data->chscan_sync_id = 0;

		laps_seconds = difftime(now, ifr->ts_scan_recvd);

		retry_valid = (((ifr->scan_type == I5_CHSCAN_REQ_FRESH) &&
			((laps_seconds + DEF_MAP_CHSCAN_TM_RETRY_GRACE) <
			DEF_MAP_CHSCAN_TM_RESP)) ||
			((ifr->scan_type == I5_CHSCAN_ONBOOT) &&
			((chscan_ifr_arg->retry_count + 1) <
			DEF_MAP_CHSCAN_MAX_RETRY_ONBOOT)));

		/* Retry Escan on this Radio */
		if (retry_valid) {

			chscan_ifr_arg->retry_count++;

			wbd_slave_remove_channel_scan_timer(ifr_vndr_data, FALSE);

			ret = wbd_slave_add_channel_scan_timer(ifr_vndr_data);
			if (ret != WBDE_OK) {
				goto cleanup_timer_arg;
			}
			goto end;

		/* If not much time left for Retry Escan : Send the Results anyway */
		} else {

			if (ifr->scan_type == I5_CHSCAN_REQ_FRESH) {

				/* Agent Sends Channel Scan Report to Controller Per Radio */
				wbd_slave_send_chscan_report_per_radio(self_ifr,
					&ifr_vndr_data->last_fresh_chscan_req,
					MAP_CHSCAN_STATUS_RADIO_BUSY);
			}
			goto cleanup_timer_arg;
		}

	/* EScan Start = SUCCESS */
	} else {
		WBD_INFO("Ifname[%s] sync_id[%d] %s Scan Start : Success\n", self_ifr->ifname,
			ifr_vndr_data->chscan_sync_id, I5_PRINT_CHSCAN_TYPE(ifr->scan_type));

		/* (Re)Initialize relavant Channel Scan Result objects in Stored Results */
		i5DmChannelScanResultItemInitPerRadio(ifr, &selfdevice->stored_chscan_results);
	}

cleanup_timer_arg:
	/* Cleanup the Timer Arg */
	if (chscan_ifr_arg) {

		/* Erase Last Channel Scan Request data saved on this Radio */
		if (chscan_ifr_arg->ifr.num_of_opclass > 0) {
			i5DmGlistCleanup(&chscan_ifr_arg->ifr.opclass_list);
			chscan_ifr_arg->ifr.num_of_opclass = 0;
		}

		/* Cleanup Timer Arg iteself */
		free(chscan_ifr_arg);
		chscan_ifr_arg = NULL;
		ifr_vndr_data->chscan_ifr_arg = NULL;
	}

end:
	if (params) {
		free(params);
	}
	WBD_EXIT();
}

/* Check if { FRESH } Channel Scan Request is Too Soon */
static bool
wbd_slave_check_if_fresh_scan_too_soon(i5_dm_interface_type *self_ifr,
	ieee1905_per_radio_opclass_list *new_ifr)
{
	bool scan_too_soon = FALSE;
	unsigned int gap_bet_req = 0, min_scan_interval = 0;
	char prefix[IFNAMSIZ+2] = {0};
	wbd_ifr_item_t *ifr_vndr_data = NULL;
	WBD_ENTER();

	ifr_vndr_data = (wbd_ifr_item_t*)self_ifr->vndr_data;

	/* IF This is the First { FRESH } Channel Scan Request, Let it Scan */
	if (ifr_vndr_data->ts_last_fresh_chscan_recvd <= 0) {
		WBD_INFO("ifname %s First { FRESH } Channel Scan Req, Scan Not Too Soon.\n",
			self_ifr->ifname);
		scan_too_soon = FALSE;
		goto end;
	}

	/* Get Prefix for this Radio MAC */
	snprintf(prefix, sizeof(prefix), "%s_", self_ifr->wlParentName);

	/* Get NVRAM setting for Minimum Scan Interval */
	min_scan_interval = blanket_get_config_val_int(prefix,
		NVRAM_MAP_CHSCAN_MIN_SCAN_INT, DEF_MAP_CHSCAN_MIN_SCAN_INT);

	/* Find : Time Gap between recving Curr & Prev { FRESH } Scan Req fm Controller */
	gap_bet_req = difftime(new_ifr->ts_scan_recvd,
		ifr_vndr_data->ts_last_fresh_chscan_recvd);

	WBD_INFO("ifname %s Scan_Too_Soon[%s] ts_scan_recvd[%lu] "
		"ts_last_fresh_chscan_recvd[%lu] gap_bet_req[%u] min_scan_interval[%u]\n",
		self_ifr->ifname,
		(gap_bet_req < min_scan_interval) ? "YES" : "NO",
		(unsigned long)new_ifr->ts_scan_recvd,
		(unsigned long)ifr_vndr_data->ts_last_fresh_chscan_recvd,
		(unsigned int)gap_bet_req, (unsigned int)min_scan_interval);

	/* If GAP bet 2 { FRESH } Channel Scan Req is > Min Scan Interval, Let it Scan */
	if (gap_bet_req > min_scan_interval) {
		WBD_INFO("ifname %s gap_bet_req > min_scan_interval, Scan Not Too Soon.\n",
			self_ifr->ifname);
		scan_too_soon = FALSE;
		goto end;
	}

	WBD_ERROR("ifname %s Channel Scan Request too soon after last scan.\n", self_ifr->ifname);

	/* Save Current Scan Request as Previous for this Radio */
	i5DmCopyChScanRequestPerRadio(&ifr_vndr_data->last_fresh_chscan_req, new_ifr);

	/* Agent Sends Channel Scan Report to Controller Per Radio */
	wbd_slave_send_chscan_report_per_radio(self_ifr,
		&ifr_vndr_data->last_fresh_chscan_req,
		MAP_CHSCAN_STATUS_REQ_SOON);

	/* SCAN is Too Soon */
	scan_too_soon = TRUE;

end:
	/* In any case, Save Current { FRESH } Scan Request TS as Previous */
	ifr_vndr_data->ts_last_fresh_chscan_recvd = new_ifr->ts_scan_recvd;

	WBD_EXIT();
	return scan_too_soon;
}

/* Create New Channel Scan Timer */
static int
wbd_slave_add_channel_scan_timer(wbd_ifr_item_t *ifr_vndr_data)
{
	int ret = WBDE_OK, interval = 1;
	wbd_info_t *info = wbd_get_ginfo();
	WBD_ENTER();

	/* Validate Timer Arg */
	WBD_ASSERT_ARG(ifr_vndr_data, WBDE_INV_ARG);
	WBD_ASSERT_ARG(ifr_vndr_data->chscan_ifr_arg, WBDE_INV_ARG);

	interval = (ifr_vndr_data->chscan_ifr_arg->retry_count) ?
		MAP_CHSCAN_CALC_TMR_INTRVL((ifr_vndr_data->chscan_ifr_arg->retry_count)) :
		(ifr_vndr_data->chscan_ifr_arg->last_scan_aborted ?
		DEF_MAP_CHSCAN_TM_SCAN_GRACE : 1);

	WBD_INFO("ifname %s NEW Timer for { %s } Scan ADDED. "
		"Run EScan on Radio: Retry[%d] New_Interval[%d]\n",
		ifr_vndr_data->chscan_ifr_arg->ifname,
		I5_PRINT_CHSCAN_TYPE(ifr_vndr_data->chscan_ifr_arg->ifr.scan_type),
		ifr_vndr_data->chscan_ifr_arg->retry_count, interval);

	/* Create the New Channel Scan Timer */
	ret = wbd_add_timers(info->hdl, (void *)ifr_vndr_data->chscan_ifr_arg,
		WBD_SEC_MICROSEC(interval), wbd_slave_escan_run_per_radio, 0);
	if (ret != WBDE_OK) {
		WBD_WARNING("ifname %s Interval[%d] Failed to Add Timer to Run EScan "
			"on Radio. Error: %d\n",
			ifr_vndr_data->chscan_ifr_arg->ifname, interval, ret);
		goto end;
	}

end:
	WBD_EXIT();
	return ret;
}

/* Remove Old Channel Scan Timer & If asked, Cleanup the Timer Arg as well */
static void
wbd_slave_remove_channel_scan_timer(wbd_ifr_item_t *ifr_vndr_data, bool cleanup_arg)
{
	int ret = WBDE_OK;
	wbd_info_t *info = wbd_get_ginfo();
	WBD_ENTER();

	/* Validate Timer Arg */
	WBD_ASSERT_ARG(ifr_vndr_data, WBDE_INV_ARG);
	WBD_ASSERT_ARG(ifr_vndr_data->chscan_ifr_arg, WBDE_INV_ARG);

	WBD_INFO("ifname %s OLD Timer for { %s } Scan REMOVED.\n",
		ifr_vndr_data->chscan_ifr_arg->ifname,
		I5_PRINT_CHSCAN_TYPE(ifr_vndr_data->chscan_ifr_arg->ifr.scan_type));

	/* Remove the Old Channel Scan Timer */
	ret = wbd_remove_timers(info->hdl, wbd_slave_escan_run_per_radio,
		(void *)ifr_vndr_data->chscan_ifr_arg);
	if (ret != WBDE_OK) {
		WBD_WARNING("ifname %s Failed to Remove timer to Run EScan "
			"for this Radio. Error: %d\n",
			ifr_vndr_data->chscan_ifr_arg->ifname, ret);
		goto end;
	}

	/* If asked, Cleanup the Timer Arg as well */
	if (cleanup_arg) {

		/* Erase Last { FRESH } Channel Scan Request data saved on this Radio */
		if (ifr_vndr_data->chscan_ifr_arg->ifr.num_of_opclass > 0) {
			i5DmGlistCleanup(&ifr_vndr_data->chscan_ifr_arg->ifr.opclass_list);
			ifr_vndr_data->chscan_ifr_arg->ifr.num_of_opclass = 0;
		}

		/* Cleanup Timer Arg iteself */
		free(ifr_vndr_data->chscan_ifr_arg);
		ifr_vndr_data->chscan_ifr_arg = NULL;
	}

end:
	WBD_EXIT();
}

/* Create timer to Run ESCAN on this radio to perform { FRESH } or { ONBOOT } Channel Scan */
static int
wbd_slave_create_escan_run_per_radio_timer(i5_dm_interface_type *self_ifr,
	ieee1905_per_radio_opclass_list *new_ifr)
{
	int ret = WBDE_OK;
	bool last_scan_aborted = FALSE;
	wbd_ifr_item_t *ifr_vndr_data = NULL;
	i5_dm_device_type *self_device = i5DmGetSelfDevice();
	WBD_ENTER();

	ifr_vndr_data = (wbd_ifr_item_t*)self_ifr->vndr_data;
	WBD_ASSERT_ARG(ifr_vndr_data, WBDE_INV_ARG);

	/* If Channel Scan is already running, Abort the current scan */
	if (I5_IS_CHSCAN_RUNNING(self_ifr->flags)) {

		if (blanket_escan_abort(self_ifr->ifname) != 0) {
			WBD_ERROR("ifname %s Failed to Abort Scan\n", self_ifr->ifname);
		} else {
			WBD_ERROR("ifname %s Aborted the Channel Scan\n", self_ifr->ifname);
			last_scan_aborted = TRUE;
		}

		/* Update Status = ABORTED, of relavant Results in SelfDevice for Radio */
		i5DmUpdateChannelScanStatusPerRadio(self_device,
			&ifr_vndr_data->last_fresh_chscan_req, -1,
			MAP_CHSCAN_STATUS_ABORTED);
	}

	WBD_DEBUG("ifname %s Check for Any Previous Channel Scan Timer. Found[%s]\n",
		self_ifr->ifname, (ifr_vndr_data->chscan_ifr_arg) ? "YES" : "NO");

	/* If Channel Scan Timer is not Added Previously, Add New Timer */
	if (!ifr_vndr_data->chscan_ifr_arg) {
		goto create_escan_tmr;
	}

	/* For Previous { FRESH } Channel Scan Req, Send the Results */
	if (ifr_vndr_data->chscan_ifr_arg->ifr.scan_type == I5_CHSCAN_REQ_FRESH) {

		WBD_INFO("ifname %s Radio too busy to perform Channel Scan.\n",
			self_ifr->ifname);

		wbd_slave_send_chscan_report_per_radio(self_ifr,
			&ifr_vndr_data->chscan_ifr_arg->ifr,
			MAP_CHSCAN_STATUS_RADIO_BUSY);
	}

	/* Remove the OLD Timer, Cleanup Timer Arg */
	wbd_slave_remove_channel_scan_timer(ifr_vndr_data, TRUE);

create_escan_tmr:
	/* Check if Current { FRESH } Channel Scan Request Too Soon */
	if ((new_ifr->scan_type == I5_CHSCAN_REQ_FRESH) &&
		wbd_slave_check_if_fresh_scan_too_soon(self_ifr, new_ifr)) {

		/* Do not issue new Scan, Send with Error : RadioBusy */
		ret = WBDE_INV_ARG;
		goto end;
	}

	/* Create Timer Argument Structure */
	ifr_vndr_data->chscan_ifr_arg = (wbd_escan_per_radio_arg_t*)wbd_malloc(
		sizeof(*(ifr_vndr_data->chscan_ifr_arg)), &ret);
	WBD_ASSERT();

	/* Fill Timer Argument Structure */
	WBDSTRNCPY(ifr_vndr_data->chscan_ifr_arg->ifname, self_ifr->ifname,
		sizeof(ifr_vndr_data->chscan_ifr_arg->ifname));
	i5DmCopyChScanRequestPerRadio(&ifr_vndr_data->chscan_ifr_arg->ifr, new_ifr);
	ifr_vndr_data->chscan_ifr_arg->retry_count = 0;
	ifr_vndr_data->chscan_ifr_arg->last_scan_aborted = last_scan_aborted;

	/* Create New Channel Scan Timer */
	wbd_slave_add_channel_scan_timer(ifr_vndr_data);

end:
	WBD_EXIT();
	return ret;
}

/* Send Requested { FRESH } Channel Scan Report to Controller */
static int
wbd_slave_process_requested_fresh_channel_scan(ieee1905_chscan_req_msg *chscan_req,
	t_I5_CHSCAN_TYPE scan_type)
{
	int ret = WBDE_OK, radio_found = 0, i = 0;
	i5_dm_device_type *self_device = i5DmGetSelfDevice();
	i5_dm_interface_type *self_ifr = NULL;
	ieee1905_per_radio_opclass_list *dst_ifr = NULL, *ifr = NULL, *caps_ifr = NULL;
	ieee1905_chscan_req_msg caps_chscan_req;
	ieee1905_per_opclass_chan_list *opcls = NULL;
	i5_dm_rc_chan_map_type *rc_chan_map = NULL;

	WBD_ENTER();

	/* Validate arg */
	if (!chscan_req) {
		chscan_req = &caps_chscan_req;
	}

	/* Create Channel Scan Request Object in Self Device for OnBoot Scan */
	i5DmCreateChScanRequestFmCapabilities(&caps_chscan_req);

	/* Extract details for each radio */
	foreach_iglist_item(ifr, ieee1905_per_radio_opclass_list, chscan_req->radio_list) {

		radio_found = 0;
		rc_chan_map = NULL;

		/* Validate radio in Self Device */
		self_ifr = i5DmInterfaceFind(self_device, ifr->radio_mac);
		if (!self_ifr || !self_ifr->vndr_data ||
			!i5DmIsInterfaceWireless(self_ifr->MediaType)) {
			WBD_ERROR("IFR["MACF"] Not Found or Not Valid Wireless Interface.\n",
				ETHERP_TO_MACF(ifr->radio_mac));
			continue;
		}

		/* Validate details for each Radio : Number of Operating Classes */
		dst_ifr = ifr;
		if (ifr->num_of_opclass <= 0) {

			/* Extract details for each radio */
			foreach_iglist_item(caps_ifr, ieee1905_per_radio_opclass_list,
				caps_chscan_req.radio_list) {

				/* Skip the Results with Non-Matching Radios */
				if (memcmp(ifr->radio_mac, caps_ifr->radio_mac,
					sizeof(ifr->radio_mac)) == 0) {
					radio_found = 1;
					break;
				}
			}
			dst_ifr = (radio_found) ? caps_ifr : ifr;
		}

		/* Update Channel Scan Request's Type of Scan */
		dst_ifr->scan_type = scan_type;

		WBD_DEBUG("IFR[%s] num_opclass[%d]\n", self_ifr->wlParentName, ifr->num_of_opclass);

		/* Validate details for each Operating Class : Number of Channels */
		foreach_iglist_item(opcls, ieee1905_per_opclass_chan_list, dst_ifr->opclass_list) {

			if (!opcls->supported) {
				continue;
			}

			/* Extract Num of Chan and Chan List from rc_map for cur Operating Class */
			if (opcls->num_of_channels <= 0) {

				rc_chan_map = i5DmGetChannelListFmRCMap(opcls->opclass_val);
				if (!rc_chan_map) {
					WBD_ERROR("Invalid regclass: %d\n", opcls->opclass_val);
					continue;
				}
				memcpy(opcls->chan_list, &(rc_chan_map->channel[0]),
					rc_chan_map->count);
				for (i = 0; i < rc_chan_map->count; i++) {
					setbit(&opcls->supported_chan_list, i);
					setbit(&opcls->scan_required, i);
				}
				opcls->num_of_channels = rc_chan_map->count;
			}

			WBD_DEBUG("IFR[%s] opclass[%d] num_of_channels[%d] opclass_supported[%s]\n",
				self_ifr->wlParentName, opcls->opclass_val, opcls->num_of_channels,
				(opcls->supported) ? "Y" : "N");

			/* Extract details of each Channel */
			int idx_chan = 0;
			for (idx_chan = 0; idx_chan < opcls->num_of_channels; idx_chan++) {

				WBD_DEBUG("IFR[%s] opclass[%d] Tot_chans[%d] "
					"supported[%s] scan_req[%s] chan[%d]=[%d]\n",
					self_ifr->wlParentName, opcls->opclass_val,
					opcls->num_of_channels,
					isset(&opcls->supported_chan_list, idx_chan) ? "Y" : "N",
					isset(&opcls->scan_required, idx_chan) ? "Y" : "N",
					idx_chan, opcls->chan_list[idx_chan]);
			}

		}

		/* Run ESCAN for this radio to perform Requested {FRESH} Channel Scan */
		wbd_slave_create_escan_run_per_radio_timer(self_ifr, dst_ifr);
	}

	/* Free the memory allocated for Channel Scan Request Msg structure */
	i5DmChannelScanRequestInfoFree(&caps_chscan_req);

	WBD_EXIT();
	return ret;
}

/* Start Requested Channel Scan on Agent and Send Requested Channel Scan Report to Controller */
static int
wbd_slave_process_requested_channel_scan(unsigned char *src_al_mac,
	ieee1905_chscan_req_msg *chscan_req)
{
	int ret = WBDE_OK;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(src_al_mac, WBDE_INV_ARG);
	WBD_ASSERT_ARG(chscan_req, WBDE_INV_ARG);

	/* For Requested Channel Scan - Stored */
	if (!(chscan_req->chscan_req_msg_flag & MAP_CHSCAN_REQ_FRESH_SCAN)) {

		/* Send Requested { STORED } Channel Scan Report to Controller */
		wbd_slave_process_requested_stored_channel_scan(src_al_mac, chscan_req,
			MAP_CHSCAN_STATUS_SUCCESS);

	/* For Requested Channel Scan - Fresh */
	} else {

		/* Fresh Scan is Required in Request, But This Agent only Support On-Boot Scan */
		if (MAP_CHSCAN_ONLY_ONBOOT(wbd_get_ginfo()->wbd_slave->scantype_flags)) {

			/* Send Requested { STORED } Channel Scan Report to Controller */
			wbd_slave_process_requested_stored_channel_scan(src_al_mac, chscan_req,
				MAP_CHSCAN_STATUS_ONLY_ONBOOT);

		} else {

			/* Send Requested { FRESH } Channel Scan Report to Controller */
			wbd_slave_process_requested_fresh_channel_scan(chscan_req,
				I5_CHSCAN_REQ_FRESH);
		}
	}
end:
	WBD_EXIT();
	return ret;
}

/* Start On Boot local Channel Scan on Agent for all Radios */
static void
wbd_slave_process_onboot_channel_scan(bcm_usched_handle *hdl, void *arg)
{
	WBD_ENTER();

	/* Send Requested { FRESH } Channel Scan Report to Controller */
	wbd_slave_process_requested_fresh_channel_scan(NULL, I5_CHSCAN_ONBOOT);

	WBD_EXIT();
	return;
}

/* Create OnBoot Channel Scan timer to Perform ONBOOT Channel Scan on this Agent  */
int
wbd_slave_create_onboot_channel_scan_timer(wbd_info_t *info)
{
	int ret = WBDE_OK;
	i5_dm_device_type *self_device;
	wbd_device_item_t *device_vndr;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(info, WBDE_INV_ARG);
	WBD_SAFE_GET_I5_SELF_DEVICE(self_device, &ret);
	WBD_ASSERT_ARG(self_device->vndr_data, WBDE_INV_ARG);
	device_vndr = (wbd_device_item_t*)self_device->vndr_data;

	if (WBD_ONBOOT_NOT_FORCED_ON_CTRLAGENT(self_device->flags)) {
		WBD_INFO("Skip Performing Onboot Channel Scan for "
			"Controller Agent Device. ACSD Scan Stats are sufficient.\n");
		goto end;
	}
	if (!MAP_CHSCAN_DO_ONBOOT(info->wbd_slave->scantype_flags)) {
		WBD_INFO("OnBoot Channel Scan Disabled\n");
		goto end;
	}
	if (device_vndr->flags & WBD_DEV_FLAG_ONBOOT_CHSCAN_REQ_SENT)  {
		WBD_DEBUG("OnBoot Channel Scan already Initiated on Agent\n");
		goto end;
	}

	WBD_INFO("Slave Creating timer to Perform OnBoot Channel Scan, timeout [%d] \n",
		info->max.tm_do_onboot_scan);

	/* Add the timer */
	ret = wbd_add_timers(info->hdl, info,
		WBD_SEC_MICROSEC((info->max.tm_do_onboot_scan)),
		wbd_slave_process_onboot_channel_scan, 0);

	/* If Add Timer not Succeeded */
	if (ret != WBDE_OK) {
		if (ret == WBDE_USCHED_TIMER_EXIST) {
			WBD_INFO("OnBoot Channel Scan Request timer already exist\n");
		} else {
			WBD_WARNING("Timeout[%d] Slave Failed to create OnBoot Channel Scan "
				"timer Error: %d\n", (info->max.tm_do_onboot_scan), ret);
		}
	} else {
		/* Set Onboot Channel Scan Request Send Flag */
		device_vndr->flags |= WBD_DEV_FLAG_ONBOOT_CHSCAN_REQ_SENT;
	}

end:
	WBD_EXIT();
	return ret;
}
#endif /* MULTIAPR2 */

/* Callback fn to process M2 is received for all Wireless Interfaces */
static void
wbd_slave_process_all_ap_configured(bcm_usched_handle *hdl, void *arg)
{
	wbd_info_t *info = (wbd_info_t *)arg;
	int ret = WBDE_OK;
	WBD_ENTER();

	WBD_ASSERT_ARG(info, WBDE_INV_ARG);

	/* Add static neighbor list entries from topology */
	wbd_slave_add_nbr_from_topology();

#if !defined(MULTIAPR2)
	WBD_DEBUG("send chan info to controller\n");
	/* send interface specific chan info to controller in vendor message */
	wbd_slave_send_chan_info();
#endif /* !MULTIAPR2 */

	WBD_DEBUG("configure ACSD fixchanspec mode on agent \n");
	/* configure acsd to opearate in Fix chanspec mode on all agents,
	 * objective is to prevent acsd on repeaters not select any channel
	 * on it's own. In case MultiAP daemon wishes to change the channel
	 * on agent, it issues wbd_slave_set_chanspec_through_acsd on agent
	 */
	wbd_slave_set_acsd_mode_fix_chanspec();
	/* Reset all_ap_configured timer created flag */
	info->flags &= ~WBD_INFO_FLAGS_ALL_AP_CFGRED_TM;

#if defined(MULTIAPR2)
	/* Create OnBoot Channel Scan timer to Perform ONBOOT Channel Scan on this Agent  */
	wbd_slave_create_onboot_channel_scan_timer(info);
#endif /* MULTIAPR2 */

end:
	WBD_EXIT();
}

/* Create timer for Callback fn to process M2 is received for all Wireless Interfaces */
int
wbd_slave_create_all_ap_configured_timer()
{
	int ret = WBDE_OK;
	WBD_ENTER();

	/* If all_ap_configured timer already created, don't create again */
	if (wbd_get_ginfo()->flags & WBD_INFO_FLAGS_ALL_AP_CFGRED_TM) {
		WBD_INFO("all_ap_configured timer already created\n");
		goto end;
	}

	/* Add the timer */
	ret = wbd_add_timers(wbd_get_ginfo()->hdl, wbd_get_ginfo(),
		WBD_SEC_MICROSEC(wbd_get_ginfo()->max.tm_process_ap_configure),
		wbd_slave_process_all_ap_configured, 0);

	/* If Add Timer Succeeded */
	if (ret == WBDE_OK) {
		/* Set all_ap_configured timer created flag, else error */
		wbd_get_ginfo()->flags |= WBD_INFO_FLAGS_ALL_AP_CFGRED_TM;
	} else {
		WBD_WARNING("Failed to create all_ap_configured timer for Interval[%d]\n",
			wbd_get_ginfo()->max.tm_process_ap_configure);
	}

end:
	WBD_EXIT();
	return ret;
}

/* Process 1905 Vendor Specific Messages at WBD Application Layer */
int
wbd_slave_process_vendor_specific_msg(wbd_info_t *info, ieee1905_vendor_data *msg_data)
{
	int ret = WBDE_OK;
	unsigned char *tlv_data = NULL;
	unsigned short tlv_data_len, tlv_hdr_len;
	unsigned int pos;
	i5_dm_device_type *controller_device = NULL;
	WBD_ENTER();

	WBD_SAFE_GET_I5_CTLR_DEVICE(controller_device, &ret);

	/* Store TLV Hdr len */
	tlv_hdr_len = sizeof(i5_tlv_t);

	/* Initialize data pointers and counters */
	for (pos = 0, tlv_data_len = 0;

		/* Loop till we reach end of vendor data */
		(int)(msg_data->vendorSpec_len - pos) > 0;

		/* Increament the pointer with current TLV Header + TLV data */
		pos += tlv_hdr_len + tlv_data_len) {

		/* For TLV, Initialize data pointers and counters */
		tlv_data = &msg_data->vendorSpec_msg[pos];

		/* Pointer is at the next TLV */
		i5_tlv_t *ptlv = (i5_tlv_t *)tlv_data;

		/* Get next TLV's data length (Hdr bytes skipping done in fn wbd_tlv_decode_xxx) */
		tlv_data_len = ntohs(ptlv->length);

		WBD_DEBUG("vendorSpec_len[%d] tlv_hdr_len[%d] tlv_data_len[%d] pos[%d] type[%d]\n",
			msg_data->vendorSpec_len, tlv_hdr_len, tlv_data_len, pos, ptlv->type);

		/* Any vendor specific tlv which does not require the controller almac check
		 * needs to be checked here.
		 */
		if ((ptlv->type != WBD_TLV_BSS_CAPABILITY_QUERY_TYPE) &&
			(eacmp(controller_device->DeviceId, msg_data->neighbor_al_mac) != 0)) {
			WBD_INFO("Received vendor specific tlv %d from device["MACDBG"] which is "
				"different device than the current controller["MACDBG"]\n",
				ptlv->type, MAC2STRDBG(msg_data->neighbor_al_mac),
				MAC2STRDBG(controller_device->DeviceId));
			return ret;
		}

		switch (ptlv->type) {

			case WBD_TLV_FBT_CONFIG_RESP_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(WBD_TLV_FBT_CONFIG_RESP_TYPE),
					msg_data->vendorSpec_len);
#ifdef WLHOSTFBT
				/* Process 1905 Vendor Specific FBT_CONFIG_RESP Message */
				wbd_slave_process_fbt_config_resp_cmd(
					msg_data->neighbor_al_mac, tlv_data, tlv_data_len);
#endif /* WLHOSTFBT */
				break;

			case WBD_TLV_VNDR_METRIC_POLICY_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(WBD_TLV_VNDR_METRIC_POLICY_TYPE),
					msg_data->vendorSpec_len);
				/* Process 1905 Vendor Specific Metric Reporting Policy Message */
				wbd_slave_process_metric_reportig_policy_vndr_cmd(
					msg_data->neighbor_al_mac, tlv_data, tlv_data_len);
				break;

			case WBD_TLV_WEAK_CLIENT_RESP_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(ptlv->type),
					msg_data->vendorSpec_len);
				/* Process 1905 Vendor Specific weak client response Message */
				wbd_slave_process_weak_client_cmd_resp(
					msg_data->neighbor_al_mac, tlv_data, tlv_data_len);
				break;

			case WBD_TLV_BSS_CAPABILITY_QUERY_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(ptlv->type),
					msg_data->vendorSpec_len);
				/* Process 1905 Vendor Specific BSS Capability Query Message */
				wbd_slave_process_bss_capability_query_cmd(
					msg_data->neighbor_al_mac, tlv_data, tlv_data_len);
				break;

			case WBD_TLV_REMOVE_CLIENT_REQ_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(WBD_TLV_REMOVE_CLIENT_REQ_TYPE),
					msg_data->vendorSpec_len);
				break;

			case WBD_TLV_STEER_REQUEST_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(WBD_TLV_STEER_REQUEST_TYPE),
					msg_data->vendorSpec_len);
				wbd_tlv_decode_vendor_steer_request(NULL,
					msg_data->vendorSpec_msg, msg_data->vendorSpec_len);
				break;

			case WBD_TLV_ZWDFS_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(ptlv->type),
					msg_data->vendorSpec_len);
				/* Process 1905 Vendor Specific ZWDFS messgae */
				wbd_slave_process_zwdfs_msg(msg_data->neighbor_al_mac, tlv_data,
					tlv_data_len);
				break;

			case WBD_TLV_BH_STA_METRIC_POLICY_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(ptlv->type),
					msg_data->vendorSpec_len);
				/* Process Vendor Specific backhaul STA mertric policy Message */
				wbd_slave_process_backhaul_sta_metric_policy_cmd(info,
					msg_data->neighbor_al_mac, tlv_data, tlv_data_len);
				break;

			case WBD_TLV_NVRAM_SET_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(ptlv->type),
					msg_data->vendorSpec_len);
				/* Process Vendor Specific NVRAM Set Message */
				wbd_slave_process_vndr_nvram_set_cmd(info,
					msg_data->neighbor_al_mac, tlv_data, tlv_data_len);
				break;
			case WBD_TLV_CHAN_SET_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(ptlv->type),
					msg_data->vendorSpec_len);
				/* Process 1905 Vendor Specific set channel */
				wbd_slave_process_vndr_set_chan_cmd(msg_data->neighbor_al_mac,
					tlv_data, tlv_data_len);
				break;

			case WBD_TLV_DFS_CHAN_INFO_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(ptlv->type),
					msg_data->vendorSpec_len);

				/* Process 1905 Vendor Specific chan info and chanspec list */
				wbd_slave_process_dfs_chan_info(msg_data->neighbor_al_mac,
					tlv_data, tlv_data_len);
				break;

			case WBD_TLV_VNDR_STRONG_STA_POLICY_TYPE:
				WBD_INFO("Processing %s vendorSpec_len[%d]\n",
					wbd_tlv_get_type_str(WBD_TLV_VNDR_STRONG_STA_POLICY_TYPE),
					msg_data->vendorSpec_len);
				/* Process 1905 Vendor Specific Strong STA Reporting
				 * Policy Message
				 */
				wbd_slave_process_strong_sta_reportig_policy_vndr_cmd(
					msg_data->neighbor_al_mac, tlv_data, tlv_data_len);
				break;

			default:
				WBD_WARNING("Vendor TLV[%s] processing not Supported by Slave.\n",
					wbd_tlv_get_type_str(ptlv->type));
				break;
		}

	}

	WBD_DEBUG("vendorSpec_len[%d] tlv_hdr_len[%d] tlv_data_len[%d] pos[%d]\n",
		msg_data->vendorSpec_len, tlv_hdr_len, tlv_data_len, pos);

end:
	WBD_EXIT();
	return ret;
}

/* Callback fn for Slave to do rc restart gracefully */
static void
wbd_slave_rc_restart_timer_cb(bcm_usched_handle *hdl, void *arg)
{
	wbd_info_t *info = (wbd_info_t *)arg;
	int ret = WBDE_OK;
	WBD_ENTER();

	WBD_ASSERT_ARG(info, WBDE_INV_ARG);
	/* nvram commit, rc restart  */
	if (info->flags & WBD_INFO_FLAGS_RC_RESTART) {
		wbd_do_rc_restart_reboot(WBD_FLG_RC_RESTART | WBD_FLG_NV_COMMIT);
	}
end:
	WBD_EXIT();
}

/* Create timer for Slave to do rc restart gracefully */
int
wbd_slave_create_rc_restart_timer(wbd_info_t *info)
{
	int ret = WBDE_OK;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(info, WBDE_INV_ARG);

	/* If rc restart timer already created, don't create again */
	if (info->flags & WBD_INFO_FLAGS_IS_RC_RESTART_TM) {
		WBD_INFO("Restart timer already created\n");
		/* Postpone the timer by removing the existing timer and creating it again */
		wbd_remove_timers(info->hdl, wbd_slave_rc_restart_timer_cb, info);
		info->flags &= ~WBD_INFO_FLAGS_IS_RC_RESTART_TM;
	}

	/* Add the timer */
	ret = wbd_add_timers(info->hdl, info,
		WBD_SEC_MICROSEC(info->max.tm_rc_restart),
		wbd_slave_rc_restart_timer_cb, 0);

	/* If Add Timer Succeeded */
	if (ret == WBDE_OK) {
		/* Set rc restart timer created flag, else error */
		info->flags |= WBD_INFO_FLAGS_IS_RC_RESTART_TM;
	} else {
		WBD_WARNING("Failed to create rc restart timer for Interval[%d]\n",
			info->max.tm_rc_restart);
	}

end:
	WBD_EXIT();
	return ret;
}

/* Processes WEAK_CLIENT_BSD request */
static void
wbd_slave_process_weak_client_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	return wbd_slave_process_weak_strong_client_bsd_cmd(hndl, childfd, cmddata, arg,
		WBD_STA_STATUS_WEAK);
}

/* Processes WEAK_CANCEL_BSD request */
static void
wbd_slave_process_weak_cancel_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	return wbd_slave_process_weak_strong_cancel_bsd_cmd(hndl, childfd, cmddata, arg,
		WBD_STA_STATUS_WEAK);
}

/* Processes STRONG_CLIENT_BSD request */
static void
wbd_slave_process_strong_client_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata,
	void *arg)
{
	return wbd_slave_process_weak_strong_client_bsd_cmd(hndl, childfd, cmddata, arg,
		WBD_STA_STATUS_STRONG);
}

/* Processes STRONG_CANCEL_BSD request */
static void
wbd_slave_process_strong_cancel_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata,
	void *arg)
{
	return wbd_slave_process_weak_strong_cancel_bsd_cmd(hndl, childfd, cmddata, arg,
		WBD_STA_STATUS_STRONG);
}

/* Processes all WEAK_CLIENT/STRONG_CLIENT data from BSD or from CLI */
static int
wbd_slave_process_weak_strong_client_data(i5_dm_bss_type *i5_bss, struct ether_addr *sta_mac,
	wbd_cmd_weak_client_bsd_t *cmdweakclient, wbd_sta_status_t command)
{
	int ret = WBDE_OK;
	i5_dm_clients_type *i5_assoc_sta;
	wbd_assoc_sta_item_t *assoc_sta;
	WBD_ENTER();

	/* Get Assoc item pointer */
	i5_assoc_sta = wbd_ds_find_sta_in_bss_assoclist(i5_bss, sta_mac, &ret, &assoc_sta);
	/* In a rare case scenario, we saw the station is in driver assoclist but not in
	 * WBD list. BSD has some timer which queries driver for mismatch in its assoclist
	 * and driver. So, BSD has the correct information. So, if WBD gets weak/strong client from
	 * BSD and if that STA is not in WBD assoclist, we can add it if it is in driver
	 */
	if (ret == WBDE_DS_UN_ASCSTA) {
		WBD_INFO("%s: Received %s STA notification for "MACF" which is not in assoclist "
			"of BSS "MACDBG". Refreshing assoclist\n",
			i5_bss->ifname, WEAK_STRONG_STR(command), ETHERP_TO_MACF(sta_mac),
			MAC2STRDBG(i5_bss->BSSID));
		blanket_check_and_add_sta_from_assoclist(i5_bss->ifname,
			(struct ether_addr*)i5_bss->BSSID, sta_mac);
	}
	WBD_ASSERT_MSG("Slave["MACDBG"] STA["MACF"]. %s\n", MAC2STRDBG(i5_bss->BSSID),
			ETHERP_TO_MACF(sta_mac), wbderrorstr(ret));

	/* If its in ignore list */
	if (assoc_sta->status == WBD_STA_STATUS_IGNORE) {
		WBD_WARNING("Slave["MACDBG"] STA["MACF"] In Ignore list\n",
			MAC2STRDBG(i5_bss->BSSID), ETHERP_TO_MACF(sta_mac));
		ret = WBDE_IGNORE_STA;
		goto end;
	}
	if (I5_IS_BSS_STA(i5_assoc_sta->flags)) {
		WBD_WARNING("Slave["MACDBG"] STA["MACF"] is backhaul sta, skip\n",
			MAC2STRDBG(i5_bss->BSSID), ETHERP_TO_MACF(sta_mac));
		ret = WBDE_IGNORE_STA;
		goto end;
	}

	WBD_INFO("Found %s STA "MACF" in BSS "MACDBG"\n",
		WEAK_STRONG_STR(command), ETHERP_TO_MACF(sta_mac), MAC2STRDBG(i5_bss->BSSID));

	if (cmdweakclient) {
		/* Update the stats locally from the stats we got it from the BSD */
		assoc_sta->stats.active = time(NULL);
		assoc_sta->stats.idle_rate = cmdweakclient->datarate;
		assoc_sta->stats.rssi = cmdweakclient->rssi;
		assoc_sta->stats.tx_tot_failures = cmdweakclient->tx_tot_failures;
		assoc_sta->stats.tx_failures = cmdweakclient->tx_failures;
		assoc_sta->stats.tx_rate = cmdweakclient->tx_rate;
		assoc_sta->stats.rx_tot_bytes = cmdweakclient->rx_tot_bytes;
		assoc_sta->stats.rx_tot_pkts = cmdweakclient->rx_tot_pkts;
		assoc_sta->band = cmdweakclient->band;
	}

	/* Send WEAK_CLIENT/STRONG_CLIENT command and Update STA Status = Weak/Strong */
	ret = wbd_slave_send_weak_strong_client_cmd(i5_bss, i5_assoc_sta, command);

end: /* Check Slave Pointer before using it below */

	WBD_EXIT();
	return ret;
}

/* Slave updates WEAK_CLIENT_BSD/STRONG_CLIENT_BSD Data */
static int
wbd_slave_process_weak_strong_client_bsd_data(i5_dm_bss_type *i5_bss,
	wbd_cmd_weak_client_bsd_t *cmdweakclient, wbd_sta_status_t command)
{
	int ret = WBDE_OK;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(i5_bss, WBDE_INV_ARG);

	/* Process WEAK_CLIENT/STRONG_CLIENT data and send it to master */
	ret = wbd_slave_process_weak_strong_client_data(i5_bss, &cmdweakclient->mac, cmdweakclient,
		command);

end: /* Check Slave Pointer before using it below */

	WBD_EXIT();
	return ret;
}

/* Processes WEAK_CLIENT_BSD/STRONG_CLIENT request */
static void
wbd_slave_process_weak_strong_client_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata,
	void *arg, wbd_sta_status_t command)
{
	int ret = WBDE_OK;
	wbd_cmd_weak_client_bsd_t *cmdweakclient = (wbd_cmd_weak_client_bsd_t*)cmddata;
	i5_dm_bss_type *i5_bss;
	i5_dm_interface_type *i5_ifr;
	char response[WBD_MAX_BUF_256], cmd[WBD_MAX_BUF_32];
	WBD_ENTER();

	if (command == WBD_STA_STATUS_WEAK) {
		WBDSTRNCPY(cmd, "WEAK_CLIENT_BSD", sizeof(cmd));
	} else if (command == WBD_STA_STATUS_STRONG) {
		WBDSTRNCPY(cmd, "STRONG_CLIENT_BSD", sizeof(cmd));
	}

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(cmdweakclient, cmd);

	/* Choose BSS based on mac */
	WBD_DS_GET_I5_SELF_BSS((unsigned char*)&cmdweakclient->cmdparam.dst_mac, i5_bss, &ret);
	WBD_ASSERT_MSG("Slave["MACF"] %s\n",
		ETHER_TO_MACF(cmdweakclient->cmdparam.dst_mac), wbderrorstr(ret));

	/* If the interface is not configured, dont accept weak/strong client */
	i5_ifr = (i5_dm_interface_type*)WBD_I5LL_PARENT(i5_bss);
	if (!i5_ifr->isConfigured) {
		ret = WBDE_AGENT_NT_JOINED;
		WBD_WARNING("Band[%d] %s from BSD["MACF"] to ["MACF"]. Error : %s",
			cmdweakclient->cmdparam.band, cmd,
			ETHER_TO_MACF(cmdweakclient->cmdparam.src_mac),
			ETHER_TO_MACF(cmdweakclient->cmdparam.dst_mac), wbderrorstr(ret));
		goto end;
	}

	/* process the WEAK_CLIENT_BSD data */
	ret = wbd_slave_process_weak_strong_client_bsd_data(i5_bss, cmdweakclient, command);
	WBD_ASSERT();

end: /* Check Slave Pointer before using it below */

	if (ret == WBDE_DS_UNKWN_SLV) {
		WBD_WARNING("Band[%d] %s from BSD["MACF"] to ["MACF"]. Error : %s",
			cmdweakclient->cmdparam.band, cmd,
			ETHER_TO_MACF(cmdweakclient->cmdparam.src_mac),
			ETHER_TO_MACF(cmdweakclient->cmdparam.dst_mac), wbderrorstr(ret));
	}

	/* Creates the WEAK_CLIENT_BSD/STRONG_CLIENT_BSD response to send back */
	if (cmdweakclient) {
		snprintf(response, sizeof(response), "resp&mac="MACF"&errorcode=%d",
			ETHER_TO_MACF(cmdweakclient->mac), wbd_slave_wbd_to_bsd_error_code(ret));
		if (wbd_com_send_cmd(hndl, childfd, response, NULL) != WBDE_OK) {
			WBD_WARNING("Band[%d] Slave["MACF"] %s : %s\n",
				cmdweakclient->cmdparam.band,
				ETHER_TO_MACF(cmdweakclient->cmdparam.dst_mac), cmd,
				wbderrorstr(WBDE_SEND_RESP_FL));
		}

		free(cmdweakclient);
	}

	WBD_EXIT();
}

/* Slave updates WEAK_CANCEL_BSD/STRONG_CANCEL_BSD Data */
static int
wbd_slave_process_weak_strong_cancel_bsd_data(i5_dm_bss_type *i5_bss,
	wbd_cmd_weak_cancel_bsd_t *cmdweakcancel, wbd_sta_status_t command)
{
	int ret = WBDE_OK;
	i5_dm_clients_type *i5_assoc_sta;
	wbd_assoc_sta_item_t *assoc_sta;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(i5_bss, WBDE_INV_ARG);

	/* Get Assoc item pointer */
	i5_assoc_sta = wbd_ds_find_sta_in_bss_assoclist(i5_bss, &cmdweakcancel->mac, &ret,
		&assoc_sta);
	WBD_ASSERT_MSG("Slave["MACDBG"] STA["MACF"]. %s\n", MAC2STRDBG(i5_bss->BSSID),
			ETHER_TO_MACF(cmdweakcancel->mac), wbderrorstr(ret));

	/* Update the stats locally from the stats we got it from the BSD */
	assoc_sta->stats.active = time(NULL);
	assoc_sta->stats.idle_rate = cmdweakcancel->datarate;
	assoc_sta->stats.rssi = cmdweakcancel->rssi;
	assoc_sta->stats.tx_tot_failures = cmdweakcancel->tx_tot_failures;
	assoc_sta->stats.tx_failures = cmdweakcancel->tx_failures;
	assoc_sta->stats.tx_rate = cmdweakcancel->tx_rate;
	assoc_sta->stats.rx_tot_bytes = cmdweakcancel->rx_tot_bytes;
	assoc_sta->stats.rx_tot_pkts = cmdweakcancel->rx_tot_pkts;

	/* Check if its already weak/strong client */
	if (((command == WBD_STA_STATUS_WEAK) && (assoc_sta->status == WBD_STA_STATUS_WEAK ||
		assoc_sta->status == WBD_STA_STATUS_WEAK_STEERING)) ||
		((command == WBD_STA_STATUS_STRONG) &&
		(assoc_sta->status == WBD_STA_STATUS_STRONG ||
		assoc_sta->status == WBD_STA_STATUS_STRONG_STEERING))) {
		WBD_INFO("Band[%d] in BSS "MACDBG" %s STA "MACF" became Normal status=%d\n",
			cmdweakcancel->cmdparam.band, MAC2STRDBG(i5_bss->BSSID),
			WEAK_STRONG_STR(command), ETHER_TO_MACF(cmdweakcancel->mac),
			assoc_sta->status);
		/* Send WEAK_CANCEL/STRONG_CANCEL command and Update STA Status = Normal */
		ret = wbd_slave_send_weak_strong_cancel_cmd(i5_bss, i5_assoc_sta, command);
	}

end: /* Check Slave Pointer before using it below */

	WBD_EXIT();
	return ret;
}

/* Processes WEAK_CANCEL_BSD/STRONG_CANCEL_BSD request */
static void
wbd_slave_process_weak_strong_cancel_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata,
	void *arg, wbd_sta_status_t command)
{
	int ret = WBDE_OK;
	wbd_cmd_weak_cancel_bsd_t *cmdweakcancel = (wbd_cmd_weak_cancel_bsd_t*)cmddata;
	i5_dm_bss_type *i5_bss;
	char response[WBD_MAX_BUF_256], cmd[WBD_MAX_BUF_32];
	WBD_ENTER();

	if (command == WBD_STA_STATUS_WEAK) {
		WBDSTRNCPY(cmd, "WEAK_CANCEL_BSD", sizeof(cmd));
	} else if (command == WBD_STA_STATUS_STRONG) {
		WBDSTRNCPY(cmd, "STRONG_CANCEL_BSD", sizeof(cmd));
	}

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(cmdweakcancel, cmd);

	/* Choose BSS based on mac */
	WBD_DS_GET_I5_SELF_BSS((unsigned char*)&cmdweakcancel->cmdparam.dst_mac, i5_bss, &ret);
	WBD_ASSERT_MSG("Slave["MACF"] %s\n",
		ETHER_TO_MACF(cmdweakcancel->cmdparam.dst_mac), wbderrorstr(ret));

	/* process the WEAK_CANCEL_BSD data */
	ret = wbd_slave_process_weak_strong_cancel_bsd_data(i5_bss, cmdweakcancel, command);
	WBD_ASSERT();

end: /* Check Slave Pointer before using it below */

	if (ret == WBDE_DS_UNKWN_SLV) {
		WBD_WARNING("Band[%d] %s from BSD["MACF"] to ["MACF"]. Error : %s",
			cmdweakcancel->cmdparam.band,
			cmd, ETHER_TO_MACF(cmdweakcancel->cmdparam.src_mac),
			ETHER_TO_MACF(cmdweakcancel->cmdparam.dst_mac), wbderrorstr(ret));
	}

	/* Creates the WEAK_CANCEL_BSD response to send back */
	if (cmdweakcancel) {
		snprintf(response, sizeof(response), "resp&mac="MACF"&errorcode=%d",
			ETHER_TO_MACF(cmdweakcancel->mac), wbd_slave_wbd_to_bsd_error_code(ret));
		if (wbd_com_send_cmd(hndl, childfd, response, NULL) != WBDE_OK) {
			WBD_WARNING("Band[%d] Slave["MACF"] %s : %s\n",
				cmdweakcancel->cmdparam.band,
				ETHER_TO_MACF(cmdweakcancel->cmdparam.dst_mac), cmd,
				wbderrorstr(WBDE_SEND_RESP_FL));
		}

		free(cmdweakcancel);
	}

	WBD_EXIT();
}

/* Processes STA_STATUS_BSD request */
static void
wbd_slave_process_sta_status_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	int ret = WBDE_OK;
	wbd_cmd_sta_status_bsd_t *cmdstastatus = (wbd_cmd_sta_status_bsd_t*)cmddata;
	i5_dm_bss_type *i5_bss;
	wbd_assoc_sta_item_t *assoc_sta;
	char response[WBD_MAX_BUF_256];
	uint32 dwell_time = 0;
	WBD_ENTER();

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(cmdstastatus, "STA_STATUS_BSD");

	/* Choose BSS based on mac */
	WBD_DS_GET_I5_SELF_BSS((unsigned char*)&cmdstastatus->cmdparam.dst_mac, i5_bss, &ret);
	WBD_ASSERT_MSG("Slave["MACF"] %s\n",
		ETHER_TO_MACF(cmdstastatus->cmdparam.dst_mac), wbderrorstr(ret));

	/* Get Assoc item pointer */
	wbd_ds_find_sta_in_bss_assoclist(i5_bss, &cmdstastatus->sta_mac, &ret,
		&assoc_sta);
	WBD_ASSERT_MSG("Slave["MACDBG"] STA["MACF"]. %s\n", MAC2STRDBG(i5_bss->BSSID),
			ETHER_TO_MACF(cmdstastatus->sta_mac), wbderrorstr(ret));

	ret = assoc_sta->error_code;
	dwell_time = assoc_sta->dwell_time;

end: /* Check Slave Pointer before using it below */

	if (ret == WBDE_DS_UNKWN_SLV) {
		WBD_WARNING("Band[%d] STA_STATUS_BSD from BSD["MACF"] to ["MACF"]. Error : %s",
			cmdstastatus->cmdparam.band, ETHER_TO_MACF(cmdstastatus->cmdparam.src_mac),
			ETHER_TO_MACF(cmdstastatus->cmdparam.dst_mac), wbderrorstr(ret));
	}
	/* Creates the STA_STATUS_BSD response to send back */
	if (cmdstastatus) {
		snprintf(response, sizeof(response), "resp&mac="MACF"&errorcode=%d&dwell=%lu",
			ETHER_TO_MACF(cmdstastatus->sta_mac), wbd_slave_wbd_to_bsd_error_code(ret),
			(unsigned long)dwell_time);
		if (wbd_com_send_cmd(hndl, childfd, response, NULL) != WBDE_OK) {
			WBD_WARNING("Band[%d] Slave["MACF"] STA_STATUS_BSD : %s\n",
				cmdstastatus->cmdparam.band,
				ETHER_TO_MACF(cmdstastatus->cmdparam.dst_mac),
				wbderrorstr(WBDE_SEND_RESP_FL));
		}

		free(cmdstastatus);
	}

	WBD_EXIT();
}

/* Convert WBD error code to BSD error code */
static int
wbd_slave_wbd_to_bsd_error_code(int error_code)
{
	int ret_code = 0;
	WBD_ENTER();

	switch (error_code) {
		case WBDE_OK:
			ret_code = WBDE_BSD_OK;
			break;

		case WBDE_IGNORE_STA:
			ret_code = WBDE_BSD_IGNORE_STA;
			break;

		case WBDE_NO_SLAVE_TO_STEER:
			ret_code = WBDE_BSD_NO_SLAVE_TO_STEER;
			break;

		case WBDE_DS_BOUNCING_STA:
			ret_code = WBDE_BSD_DS_BOUNCING_STA;
			break;

		default:
			ret_code = WBDE_BSD_FAIL;
			break;
	}

	WBD_EXIT();
	return ret_code;
}

/* -------------------------- Add New Functions above this -------------------------------- */

/* Get the SLAVELIST CLI command data */
static int
wbd_slave_process_slavelist_cli_cmd_data(wbd_info_t *info, wbd_cli_send_data_t *clidata,
	char **outdataptr)
{
	int ret = WBDE_OK, band_check = 0, outlen = WBD_MAX_BUF_8192, len = 0, count = 0;
	char *outdata = NULL;
	i5_dm_device_type *device;
	i5_dm_interface_type *ifr;
	i5_dm_bss_type *bss;

	outdata = (char*)wbd_malloc(outlen, &ret);
	WBD_ASSERT();

	/* Validate fn args */
	WBD_ASSERT_ARG(info, WBDE_INV_ARG);
	WBD_ASSERT_MSGDATA(clidata, "SLAVELIST_CLI");

	/* Check if Band is requested */
	if (WBD_BAND_VALID((clidata->band))) {
		band_check = 1;
	}

	WBD_SAFE_GET_I5_SELF_DEVICE(device, &ret);
	ret = wbd_snprintf_i5_device(device, CLI_CMD_I5_DEV_FMT, &outdata, &outlen, &len,
		WBD_MAX_BUF_8192);
	foreach_i5glist_item(ifr, i5_dm_interface_type, device->interface_list) {
		/* Skip non-wireless or interfaces with zero bss count */
		if (!i5DmIsInterfaceWireless(ifr->MediaType) || !ifr->BSSNumberOfEntries) {
			continue;
		}
		/* Band specific validation */
		if (band_check && clidata->band == ieee1905_get_band_from_channel(
				ifr->opClass, CHSPEC_CHANNEL(ifr->chanspec))) {
			continue;
		}
		foreach_i5glist_item(bss, i5_dm_bss_type, ifr->bss_list) {
			ret = wbd_snprintf_i5_bss(bss, CLI_CMD_I5_BSS_FMT, &outdata, &outlen,
				&len, WBD_MAX_BUF_8192, FALSE, FALSE);
			count++;
		}
	}

	if (!count) {
		snprintf(outdata + strlen(outdata), outlen - strlen(outdata),
			"No entry found \n");
	}

end: /* Check Slave Pointer before using it below */

	if (ret != WBDE_OK) {
		snprintf(outdata + strlen(outdata), outlen - strlen(outdata),
			"%s\n", wbderrorstr(ret));
	}
	if (outdataptr) {
		*outdataptr = outdata;
	}
	return ret;
}

/* Processes the SLAVELIST CLI command */
static void
wbd_slave_process_slavelist_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	int ret = WBDE_OK;
	wbd_cli_send_data_t *clidata = (wbd_cli_send_data_t*)cmddata;
	wbd_info_t *info = (wbd_info_t*)arg;
	char *outdata = NULL;

	BCM_REFERENCE(ret);
	ret = wbd_slave_process_slavelist_cli_cmd_data(info, clidata, &outdata);

	if (outdata) {
		if (wbd_com_send_cmd(hndl, childfd, outdata, NULL) != WBDE_OK) {
			WBD_WARNING("SLAVELIST CLI : %s\n", wbderrorstr(WBDE_SEND_RESP_FL));
		}
		free(outdata);
	}

	if (clidata)
		free(clidata);

	return;
}

/* Get the INFO CLI command data */
static int
wbd_slave_process_info_cli_cmd_data(wbd_info_t *info, wbd_cli_send_data_t *clidata,
	char **outdataptr)
{
	int ret = WBDE_OK, outlen = WBD_MAX_BUF_8192, len = 0;
	char *outdata = NULL;
	int count = 0, band_check = 0;
	i5_dm_device_type *device;
	i5_dm_interface_type *ifr;
	i5_dm_bss_type *bss;

	outdata = (char*)wbd_malloc(outlen, &ret);
	WBD_ASSERT();

	/* Validate fn arg */
	WBD_ASSERT_ARG(info, WBDE_INV_ARG);
	WBD_ASSERT_MSGDATA(clidata, "INFO_CLI");

	/* Check if Band is requested */
	if (WBD_BAND_VALID((clidata->band))) {
		band_check = 1;
	}

	WBD_SAFE_GET_I5_CTLR_DEVICE(device, &ret);
	ret = wbd_snprintf_i5_device(device, CLI_CMD_I5_DEV_FMT, &outdata, &outlen, &len,
		WBD_MAX_BUF_8192);

	WBD_SAFE_GET_I5_SELF_DEVICE(device, &ret);
	ret = wbd_snprintf_i5_device(device, CLI_CMD_I5_DEV_FMT, &outdata, &outlen, &len,
		WBD_MAX_BUF_8192);
	foreach_i5glist_item(ifr, i5_dm_interface_type, device->interface_list) {
		/* Skip non-wireless or interfaces with zero bss count */
		if (!i5DmIsInterfaceWireless(ifr->MediaType) || !ifr->BSSNumberOfEntries) {
			continue;
		}
		/* Band specific validation */
		if (band_check && clidata->band == ieee1905_get_band_from_channel(
				ifr->opClass, CHSPEC_CHANNEL(ifr->chanspec))) {
			continue;
		}
		foreach_i5glist_item(bss, i5_dm_bss_type, ifr->bss_list) {
			ret = wbd_snprintf_i5_bss(bss, CLI_CMD_I5_BSS_FMT, &outdata, &outlen,
				&len, WBD_MAX_BUF_8192, TRUE, TRUE);
			count++;
		}
	}

	if (!count) {
		snprintf(outdata + strlen(outdata), outlen - strlen(outdata),
			"No entry found \n");
	}

end: /* Check Slave Pointer before using it below */

	if (ret != WBDE_OK) {
		snprintf(outdata + strlen(outdata), outlen - strlen(outdata),
			"%s\n", wbderrorstr(ret));
	}
	if (outdataptr) {
		*outdataptr = outdata;
	}
	return ret;
}

/* Processes the INFO CLI command */
static void
wbd_slave_process_info_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	int ret = WBDE_OK;
	wbd_cli_send_data_t *clidata = (wbd_cli_send_data_t*)cmddata;
	wbd_info_t *info = (wbd_info_t*)arg;
	char *outdata = NULL;

	BCM_REFERENCE(ret);
	ret = wbd_slave_process_info_cli_cmd_data(info, clidata, &outdata);

	if (outdata) {
		if (wbd_com_send_cmd(hndl, childfd, outdata, NULL) != WBDE_OK) {
			WBD_WARNING("INFO CLI : %s\n", wbderrorstr(WBDE_SEND_RESP_FL));
		}
		free(outdata);
	}

	if (clidata)
		free(clidata);

	return;
}

/* Get the CLIENTLIST CLI command data */
static int
wbd_slave_process_clientlist_cli_cmd_data(wbd_info_t *info,
	wbd_cli_send_data_t *clidata, char **outdataptr)
{
	int ret = WBDE_OK, len = 0, outlen = WBD_MAX_BUF_8192, count = 0, band_check = 0;
	char *outdata = NULL;
	i5_dm_device_type *device;
	i5_dm_interface_type *ifr;
	i5_dm_bss_type *bss;

	/* Validate fn args */
	WBD_ASSERT_ARG(info, WBDE_INV_ARG);
	WBD_ASSERT_MSGDATA(clidata, "CLIENTLIST_CLI");

	outdata = (char*)wbd_malloc(outlen, &ret);
	WBD_ASSERT();

	/* Check if Band is requested */
	if (WBD_BAND_VALID((clidata->band))) {
		band_check = 1;
	}

	WBD_SAFE_GET_I5_SELF_DEVICE(device, &ret);
	ret = wbd_snprintf_i5_device(device, CLI_CMD_I5_DEV_FMT, &outdata, &outlen, &len,
		WBD_MAX_BUF_8192);
	foreach_i5glist_item(ifr, i5_dm_interface_type, device->interface_list) {
		/* Skip non-wireless or interfaces with zero bss count */
		if (!i5DmIsInterfaceWireless(ifr->MediaType) || !ifr->BSSNumberOfEntries) {
			continue;
		}
		/* Band specific validation */
		if (band_check && clidata->band == ieee1905_get_band_from_channel(
				ifr->opClass, CHSPEC_CHANNEL(ifr->chanspec))) {
			continue;
		}
		foreach_i5glist_item(bss, i5_dm_bss_type, ifr->bss_list) {
			ret = wbd_snprintf_i5_clients(bss, &outdata, &outlen, &len,
				WBD_MAX_BUF_8192, TRUE);
			count++;
		}
	}

	if (!count) {
		snprintf(outdata + strlen(outdata), outlen - strlen(outdata),
			"No entry found \n");
	}

end: /* Check Slave Pointer before using it below */

	if (ret != WBDE_OK) {
		snprintf(outdata + strlen(outdata), outlen - strlen(outdata),
			"%s\n", wbderrorstr(ret));
	}
	if (outdataptr) {
		*outdataptr = outdata;
	}
	return ret;
}

/* Processes the CLIENTLIST CLI command */
static void
wbd_slave_process_clientlist_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	int ret = WBDE_OK;
	wbd_cli_send_data_t *clidata = (wbd_cli_send_data_t*)cmddata;
	wbd_info_t *info = (wbd_info_t*)arg;
	char *outdata = NULL;

	BCM_REFERENCE(ret);
	ret = wbd_slave_process_clientlist_cli_cmd_data(info, clidata, &outdata);

	if (outdata) {
		if (wbd_com_send_cmd(hndl, childfd, outdata, NULL) != WBDE_OK) {
			WBD_WARNING("CLIENTLIST CLI : %s\n", wbderrorstr(WBDE_SEND_RESP_FL));
		}
		free(outdata);
	}

	if (clidata)
		free(clidata);

	return;
}

/* Processes the MONITORADD CLI command data */
static int
wbd_slave_process_monitoradd_cli_cmd_data(wbd_info_t *info, wbd_cli_send_data_t *clidata,
	char **outdataptr)
{
	int ret = WBDE_OK;
	BCM_STAMON_STATUS status;
	bcm_stamon_maclist_t *tmp = NULL;
	struct ether_addr mac;
	char *outdata = NULL;
	int outlen = WBD_MAX_BUF_256;
	wbd_slave_item_t *slave = NULL;

	outdata = (char*)wbd_malloc(outlen, &ret);
	WBD_ASSERT();

	/* Validate arg */
	WBD_ASSERT_ARG(info, WBDE_INV_ARG);

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(clidata, "MONITORADD_CLI");

	/* Choose Slave Info based on mac */
	WBD_GET_VALID_MAC(clidata->bssid, &mac, "MONITORADD_CLI", WBDE_INV_BSSID);
	WBD_SAFE_GET_SLAVE_ITEM(info, (&mac), WBD_CMP_MAC, slave, (&ret));

	/* Get Validated Non-NULL MAC */
	WBD_GET_VALID_MAC(clidata->mac, &mac, "MONITORADD_CLI", WBDE_INV_MAC);

	/* Add the MAC to stamon list */
	ret = wbd_slave_prepare_stamon_maclist(&mac, &tmp);
	if (ret != WBDE_OK && !tmp) {
		goto end;
	}

	/* Issue stamon add command */
	status = bcm_stamon_command(slave->stamon_hdl, BCM_STAMON_CMD_ADD,
		(void*)tmp, NULL);
	if (status != BCM_STAMONE_OK) {
		WBD_WARNING("Band[%d] Slave["MACF"] Failed to add MAC["MACF"]. Stamon error : %s\n",
			slave->band, ETHER_TO_MACF(slave->wbd_ifr.mac),
			ETHER_TO_MACF(mac), bcm_stamon_strerror(status));
		ret = WBDE_STAMON_ERROR;
		goto end;
	}

end: /* Check Slave Pointer before using it below */

	if (ret == WBDE_OK) {
		snprintf(outdata, outlen, "Successfully Added MAC : %s\n", clidata->mac);
	} else {
		snprintf(outdata, outlen, "Failed to Add. Error : %s\n", wbderrorstr(ret));
	}
	if (outdataptr) {
		*outdataptr = outdata;
	}
	if (tmp) {
		free(tmp);
	}

	return ret;
}

/* Processes the MONITORADD CLI command  */
static void
wbd_slave_process_monitoradd_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	int ret = WBDE_OK;
	wbd_cli_send_data_t *clidata = (wbd_cli_send_data_t*)cmddata;
	wbd_info_t *info = (wbd_info_t*)arg;
	char *outdata = NULL;

	BCM_REFERENCE(ret);
	ret = wbd_slave_process_monitoradd_cli_cmd_data(info, clidata, &outdata);

	if (outdata) {
		if (wbd_com_send_cmd(hndl, childfd, outdata, NULL) != WBDE_OK) {
			WBD_WARNING("MONITORADD CLI : %s\n", wbderrorstr(WBDE_SEND_RESP_FL));
		}
		free(outdata);
	}

	if (clidata)
		free(clidata);

	return;
}

/* Processes the MONITORDEL CLI command data */
static int
wbd_slave_process_monitordel_cli_cmd_data(wbd_info_t *info, wbd_cli_send_data_t *clidata,
	char **outdataptr)
{
	int ret = WBDE_OK;
	BCM_STAMON_STATUS status;
	struct ether_addr mac;
	char *outdata = NULL;
	int outlen = WBD_MAX_BUF_256;
	wbd_slave_item_t *slave = NULL;

	outdata = (char*)wbd_malloc(outlen, &ret);
	WBD_ASSERT();

	/* Validate arg */
	WBD_ASSERT_ARG(info, WBDE_INV_ARG);

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(clidata, "MONITORDEL_CLI");

	/* Choose Slave Info based on mac */
	WBD_GET_VALID_MAC(clidata->bssid, &mac, "MONITORDEL_CLI", WBDE_INV_BSSID);
	WBD_SAFE_GET_SLAVE_ITEM(info, (&mac), WBD_CMP_MAC, slave, (&ret));

	if (strlen(clidata->mac) > 0) {

		/* Get Validated Non-NULL MAC */
		WBD_GET_VALID_MAC(clidata->mac, &mac, "MONITORDEL_CLI", WBDE_INV_MAC);

		ret = wbd_slave_remove_sta_fm_stamon(slave, &mac, &status);
		goto end;
	}

	/* If the MAC field is empty delete all */
	status = bcm_stamon_command(slave->stamon_hdl, BCM_STAMON_CMD_DEL, NULL, NULL);
	if (status != BCM_STAMONE_OK) {
		WBD_WARNING("Band[%d] Slave["MACF"] Failed to delete STA. Stamon error : %s\n",
			slave->band, ETHER_TO_MACF(slave->wbd_ifr.mac),
			bcm_stamon_strerror(status));
		ret = WBDE_STAMON_ERROR;
		goto end;
	}

end: /* Check Slave Pointer before using it below */

	if (ret == WBDE_OK) {
		if (strlen(clidata->mac) <= 0) {
			snprintf(outdata, outlen, "Successfully Deleted All STAs\n");
		} else {
			snprintf(outdata, outlen, "Successfully Deleted MAC : %s\n", clidata->mac);
		}
	} else {
		snprintf(outdata, outlen, "Failed to Delete. Error : %s. Stamon Error : %s\n",
			wbderrorstr(ret), bcm_stamon_strerror(status));
	}
	if (outdataptr) {
		*outdataptr = outdata;
	}

	return ret;
}

/* Processes the MONITORDEL CLI command  */
static void
wbd_slave_process_monitordel_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	int ret = WBDE_OK;
	wbd_cli_send_data_t *clidata = (wbd_cli_send_data_t*)cmddata;
	wbd_info_t *info = (wbd_info_t*)arg;
	char *outdata = NULL;

	BCM_REFERENCE(ret);
	ret = wbd_slave_process_monitordel_cli_cmd_data(info, clidata, &outdata);

	if (outdata) {
		if (wbd_com_send_cmd(hndl, childfd, outdata, NULL) != WBDE_OK) {
			WBD_WARNING("MONITORDEL CLI : %s\n", wbderrorstr(WBDE_SEND_RESP_FL));
		}
		free(outdata);
	}

	if (clidata)
		free(clidata);

	return;
}

/* Get the MONITORLIST CLI command data */
static int
wbd_slave_process_monitorlist_cli_cmd_data(wbd_info_t *info, wbd_cli_send_data_t *clidata,
	char **outdataptr)
{
	int ret = WBDE_OK, index = 0;
	BCM_STAMON_STATUS status;
	struct ether_addr mac;
	bcm_stamon_maclist_t *tmp = NULL;
	bcm_stamon_list_info_t *list = NULL;
	char *outdata = NULL;
	int outlen = WBD_MAX_BUF_2048, totlen = 0, i;
	char tmpline[WBD_MAX_BUF_512];
	wbd_slave_item_t *slave = NULL;

	outdata = (char*)wbd_malloc(outlen, &ret);
	WBD_ASSERT();

	/* Validate arg */
	WBD_ASSERT_ARG(info, WBDE_INV_ARG);

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(clidata, "MONITORLIST_CLI");

	/* Choose Slave Info based on mac */
	WBD_GET_VALID_MAC(clidata->bssid, &mac, "MONITORLIST_CLI", WBDE_INV_BSSID);
	WBD_SAFE_GET_SLAVE_ITEM(info, &mac, WBD_CMP_MAC, slave, (&ret));

	if (strlen(clidata->mac) > 0) {

		/* Get Validated Non-NULL MAC */
		WBD_GET_VALID_MAC(clidata->mac, &mac, "MONITORLIST_CLI", WBDE_INV_MAC);

		ret = wbd_slave_prepare_stamon_maclist(&mac, &tmp);
		WBD_ASSERT();

		status = bcm_stamon_command(slave->stamon_hdl, BCM_STAMON_CMD_GET, (void*)tmp,
			(void**)&list);
	} else {
		/* If the MAC field is empty get all */
		status = bcm_stamon_command(slave->stamon_hdl, BCM_STAMON_CMD_GET, NULL,
			(void**)&list);
	}

	if (status != BCM_STAMONE_OK && !list) {
		snprintf(outdata, outlen, "Stamon error : %s\n",
			bcm_stamon_strerror(status));
		ret = WBDE_STAMON_ERROR;
		goto end;
	}

	snprintf(outdata + strlen(outdata), outlen - strlen(outdata),
		"Total no of Monitored STA's %d\n", list->list_info_len);

	totlen = strlen(outdata);

	for (i = 0; i < list->list_info_len; i++) {
		snprintf(tmpline, sizeof(tmpline), "\tSTA[%d] : "MACF"   RSSI  %d   Status  %d\n",
			++index, ETHER_TO_MACF(list->info[i].ea),
			list->info[i].rssi, list->info[i].status);

		wbd_strcat_with_realloc_buffer(&outdata, &outlen, &totlen,
			WBD_MAX_BUF_2048, tmpline);
	}

end: /* Check Slave Pointer before using it below */

	if (ret != WBDE_OK) {
		snprintf(outdata + strlen(outdata), outlen - strlen(outdata),
			"%s\n", wbderrorstr(ret));
	}
	if (outdataptr) {
		*outdataptr = outdata;
	}

	if (tmp)
		free(tmp);
	if (list)
		free(list);

	return ret;
}

/* Processes the MONITORLIST CLI command */
static void
wbd_slave_process_monitorlist_cli_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	int ret = WBDE_OK;
	wbd_cli_send_data_t *clidata = (wbd_cli_send_data_t*)cmddata;
	wbd_info_t *info = (wbd_info_t*)arg;
	char *outdata = NULL;

	BCM_REFERENCE(ret);
	ret = wbd_slave_process_monitorlist_cli_cmd_data(info, clidata, &outdata);

	if (outdata) {
		if (wbd_com_send_cmd(hndl, childfd, outdata, NULL) != WBDE_OK) {
			WBD_WARNING("MONITORLIST CLI : %s\n", wbderrorstr(WBDE_SEND_RESP_FL));
		}
		free(outdata);
	}

	if (clidata)
		free(clidata);

	return;
}

/* Processes BLOCK_CLIENT_BSD request */
static void
wbd_slave_process_blk_client_bsd_cmd(wbd_com_handle *hndl, int childfd, void *cmddata, void *arg)
{
	int ret = WBDE_OK;
	wbd_cmd_block_client_t *cmdblkclient = (wbd_cmd_block_client_t*)cmddata;
	wbd_info_t *info = (wbd_info_t*)arg;
	i5_dm_bss_type *i5_bss;
	char response[WBD_MAX_BUF_256] = {0};

	WBD_ENTER();

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(cmdblkclient, "BLOCK_CLIENT_BSD");

	/* Choose BSS based on mac */
	WBD_DS_GET_I5_SELF_BSS((unsigned char*)&cmdblkclient->cmdparam.dst_mac, i5_bss, &ret);
	WBD_ASSERT_MSG("Slave["MACF"] %s\n",
		ETHER_TO_MACF(cmdblkclient->cmdparam.dst_mac), wbderrorstr(ret));

	/* Dont send block client if the timeout is not provided */
	if (info->max.tm_blk_sta <= 0) {
		goto end;
	}

	wbd_slave_send_assoc_control(info->max.tm_blk_sta, i5_bss->BSSID,
		(unsigned char*)&cmdblkclient->tbss_mac, &cmdblkclient->mac);

end:

	 /* Creates the BLOCK_CLIENT_BSD response to send back */
	 if (cmdblkclient) {
		snprintf(response, sizeof(response), "resp&mac="MACF"&errorcode=%d",
			ETHER_TO_MACF(cmdblkclient->cmdparam.src_mac),
			wbd_slave_wbd_to_bsd_error_code(ret));
		if (wbd_com_send_cmd(hndl, childfd, response, NULL) != WBDE_OK) {
			WBD_WARNING("Band[%d] Slave["MACF"] BLOCK_CLIENT_BSD : %s\n",
				cmdblkclient->cmdparam.band,
				ETHER_TO_MACF(cmdblkclient->cmdparam.dst_mac),
				wbderrorstr(WBDE_SEND_RESP_FL));
		}

		free(cmdblkclient);
	}

	WBD_EXIT();
}

/* Timer callback to read MAP's monitored STAs */
static void
wbd_slave_map_monitored_rssi_read_cb(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK;
	uint i, k;
	BCM_STAMON_STATUS status;
	bcm_stamon_list_info_t *list = NULL;
	wbd_slave_item_t *slave = NULL;
	wbd_slave_map_monitor_arg_t* monitor_arg = NULL;
	ieee1905_unassoc_sta_link_metric metric;
	ieee1905_unassoc_sta_link_metric_list *staInfo;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(arg, WBDE_INV_ARG);

	monitor_arg = (wbd_slave_map_monitor_arg_t*)arg;
	slave = monitor_arg->slave;
	memset(&metric, 0, sizeof(metric));
	wbd_ds_glist_init((wbd_glist_t*)&metric.sta_list);

	/* get the list from stamon */
	status = bcm_stamon_command(slave->stamon_hdl, BCM_STAMON_CMD_GET, NULL, (void**)&list);
	if (status != BCM_STAMONE_OK && !list) {
		WBD_WARNING("Band[%d] Slave["MACF"] Failed to Get stamon stats. Stamon error : %s",
			slave->band, ETHER_TO_MACF(slave->wbd_ifr.mac),
			bcm_stamon_strerror(status));
		ret = WBDE_STAMON_ERROR;
		goto end;
	}

	/* Now parse through the list and create list to send it */
	for (i = 0; i < list->list_info_len; i++) {
		/* Check if is there any error for this STA */
		if (list->info[i].status != BCM_STAMONE_OK)
			continue;

		/* Find the MAC address */
		for (k = 0; k < monitor_arg->maclist->count; k++) {
			if (eacmp(&monitor_arg->maclist->mac[k], &list->info[i].ea)) {
				continue;
			}
			WBD_INFO("Found STAMON STA["MACF"] rssi %d\n",
				ETHER_TO_MACF(list->info[i].ea), list->info[i].rssi);
			/* Create the STA entry */
			staInfo =
				(ieee1905_unassoc_sta_link_metric_list*)wbd_malloc(sizeof(*staInfo),
				&ret);
			WBD_ASSERT();
			clock_gettime(CLOCK_REALTIME, &staInfo->queried);
			memcpy(staInfo->mac, &list->info[i].ea, ETHER_ADDR_LEN);
			/* TODO:add proper chanspec either from firmware or from local bcm stamon
			 * list for this sta
			 */
			staInfo->channel = monitor_arg->channel;
			staInfo->rcpi = WBD_RSSI_TO_RCPI(list->info[i].rssi);
			wbd_ds_glist_append((wbd_glist_t*)&metric.sta_list, (dll_t*)staInfo);
		}
	}
	if (list)
		free(list);
	/* Delete all STAs from monitor list */
	for (k = 0; k < monitor_arg->maclist->count; k++) {
		wbd_slave_remove_sta_fm_stamon(slave,
			(struct ether_addr*)&monitor_arg->maclist->mac[k], &status);
	}
	/* Send the list to IEEE1905 module */
	memcpy(&metric.neighbor_al_mac, monitor_arg->neighbor_al_mac,
		sizeof(metric.neighbor_al_mac));
	metric.opClass = monitor_arg->rclass;
	ieee1905_send_unassoc_sta_link_metric(&metric);
end:
	wbd_ds_glist_cleanup((wbd_glist_t*)&metric.sta_list);

	if (monitor_arg) {
		if (monitor_arg->maclist) {
			free(monitor_arg->maclist);
			monitor_arg->maclist = NULL;
		}
		free(monitor_arg);
	}

	WBD_EXIT();
}

/* Calculate Chan Util Fm Chanim Stats, & Update Chan Util of this ChScan Result */
static void
wbd_slave_calc_chscanresult_chanutil(i5_dm_interface_type *i5_ifr,
	chanspec_t stats_chanspec, uint8 *ccastats, int8 bgnoise)
{
	int ret = WBDE_OK;
	ieee1905_chscan_result_item *chscan_result = NULL;
	unsigned char stats_channel = CHSPEC_CHANNEL(stats_chanspec);
	uint8 stats_opclass = 0;
	uint32 agent_flags = wbd_get_ginfo()->wbd_slave->flags;
	i5_dm_device_type *self_device = i5DmGetSelfDevice();
	WBD_ENTER();

	/* Validate fn args */
	WBD_ASSERT_ARG(i5_ifr, WBDE_INV_ARG);
	WBD_ASSERT_ARG(ccastats, WBDE_INV_ARG);

	/* Find Stats Channel Number in Stored Scan Results of a Device */
	blanket_get_global_rclass(stats_chanspec, &stats_opclass);
	chscan_result = i5DmFindChannelInScanResult(
		&self_device->stored_chscan_results, stats_opclass, stats_channel);
	if (chscan_result == NULL) {
		WBD_DEBUG("ifname[%s] Opclass[%d] Channel[%d] not present in Stored "
			"ChannelScan Results\n", i5_ifr->ifname, stats_opclass, stats_channel);
		goto end;
	}

	/* Calculate Chan Util Fm Chanim Stats, and Update Chan Util of this ChScan Result */
	chscan_result->noise = (unsigned char)(WBD_RSSI_TO_RCPI(bgnoise));

	/* Get In-channel utilization */
	if (stats_channel == wf_chspec_ctlchan(i5_ifr->chanspec)) {
		wbd_slave_get_chan_util(i5_ifr->ifname, &chscan_result->utilization);

	/* Get Off-channel utilization */
	} else {
		chscan_result->utilization = WBD_IS_BUSY_100_TXOP(agent_flags) ?
			blanket_calc_chanutil_offchan(i5_ifr->ifname, ccastats) :
			blanket_calc_chanutil(i5_ifr->ifname,
				WBD_IS_TX_IN_CHAN_UTIL(agent_flags), ccastats);
	}
	WBD_DEBUG("Ifname[%s] Opclass[%d] Channel[%d] Chan_Util[%d] Noise[%d/%d]\n",
		i5_ifr->ifname, stats_opclass, stats_channel, chscan_result->utilization,
		chscan_result->noise, bgnoise);
end:
	WBD_EXIT();
}

/* Fetch chanim_stats all : Update Channel Utilization of all Results post Scan */
static int
wbd_slave_update_chscanresult_chanutil(i5_dm_interface_type *i5_ifr)
{
	int ret = WBDE_OK, buflen = WLC_IOCTL_MAXLEN, iter = 0;
	char *data_buf = NULL;
	wl_chanim_stats_t *chanim_stats = NULL;
	WBD_ENTER();

	/* Validate fn args */
	WBD_ASSERT_ARG(i5_ifr, WBDE_INV_ARG);

	data_buf = (char*)wbd_malloc(buflen, &ret);
	WBD_ASSERT_MSG("Ifname[%s] Failed to malloc for chanim_stats\n", i5_ifr->ifname);

	ret = blanket_get_chanim_stats(i5_ifr->ifname, WL_CHANIM_COUNT_ALL, data_buf, buflen);
	WBD_ASSERT_MSG("Ifname[%s] Failed to get chanim_stats. Err[%d]\n", i5_ifr->ifname, ret);

	chanim_stats = (wl_chanim_stats_t*)data_buf;
	if (chanim_stats->count <= 0) {
		WBD_WARNING("Ifname[%s] No chanim stats\n", i5_ifr->ifname);
		goto end;
	}

	if (chanim_stats->version == WL_CHANIM_STATS_V2) {
		chanim_stats_v2_t *stats;
		stats = (chanim_stats_v2_t *)chanim_stats->stats;

		for (iter = 0; iter < chanim_stats->count; iter++, stats++) {
			/* Calc Chan Util Fm Stats, and Update Chan Util of this ChScan Result */
			wbd_slave_calc_chscanresult_chanutil(i5_ifr, stats->chanspec,
				stats->ccastats, stats->bgnoise);
		}
	} else if (chanim_stats->version <= WL_CHANIM_STATS_VERSION) {
		chanim_stats_t *stats;
		uint8 *nstats;
		uint elmt_size = 0;

		if (chanim_stats->version == WL_CHANIM_STATS_VERSION_3) {
			elmt_size = sizeof(chanim_stats_v3_t);
		} else if (chanim_stats->version == WL_CHANIM_STATS_VERSION_4) {
			elmt_size = sizeof(chanim_stats_t);
		} else {
			WBD_ERROR("Unsupported version : %d\n", chanim_stats->version);
			goto end;
		}

		nstats = (uint8 *)chanim_stats->stats;
		stats = chanim_stats->stats;

		for (iter = 0; iter < chanim_stats->count; iter++) {
			/* Calc Chan Util Fm Stats, and Update Chan Util of this ChScan Result */
			wbd_slave_calc_chscanresult_chanutil(i5_ifr, stats->chanspec,
				stats->ccastats, stats->bgnoise);
			/* move to the next element in the list */
			nstats += elmt_size;
			stats = (chanim_stats_t *)nstats;
		}
	}
end:
	if (data_buf) {
		free(data_buf);
	}
	WBD_EXIT();
	return ret;
}

/* Get channel utilization */
static int
wbd_slave_get_chan_util(char *ifname, unsigned char *chan_util)
{
	int ret = WBDE_OK, buflen = WBD_MAX_BUF_512;
	char *data_buf = NULL;
	wl_chanim_stats_t *list;
	WBD_ENTER();

	/* Validate fn args */
	WBD_ASSERT_ARG(ifname, WBDE_INV_ARG);
	WBD_ASSERT_ARG(chan_util, WBDE_INV_ARG);

	data_buf = (char*)wbd_malloc(buflen, &ret);
	WBD_ASSERT_MSG("Ifname[%s] Failed to allocate memory for chanim_stats\n", ifname);

	ret = blanket_get_chanim_stats(ifname, WL_CHANIM_COUNT_ONE, data_buf, buflen);
	WBD_ASSERT_MSG("Ifname[%s] Failed to get the chanim_stats. Err[%d]\n", ifname, ret);

	list = (wl_chanim_stats_t*)data_buf;
	if (list->count <= 0) {
		WBD_WARNING("Ifname[%s] No chanim stats\n", ifname);
		goto end;
	}
	*chan_util = blanket_calc_chanutil(ifname,
		WBD_IS_TX_IN_CHAN_UTIL(wbd_get_ginfo()->wbd_slave->flags),
		list->stats[0].ccastats);
end:
	if (data_buf) {
		free(data_buf);
	}
	WBD_EXIT();
	return ret;

}

/* Get Interface metrics */
int
wbd_slave_process_get_interface_metric_cb(wbd_info_t *info, char *ifname,
	unsigned char *ifr_mac, ieee1905_interface_metric *metric)
{
	int ret = WBDE_OK;
	WBD_ENTER();

	ret = wbd_blanket_get_interface_metric(ifname, metric);

	WBD_EXIT();
	return ret;
}

/* Get AP metrics */
int
wbd_slave_process_get_ap_metric_cb(wbd_info_t *info, char *ifname,
	unsigned char *bssid, ieee1905_ap_metric *metric)
{
	int ret = WBDE_OK;
	int amsdu = 0, ampdu = 0, ampdu_ba_wsize = 0;
	int rate = 0, ppdu_time = 0;
	unsigned char chan_util;
#if defined(MULTIAPR2)
	blanket_counters_t counters;
#endif /* MULTIAPR2 */
	WBD_ENTER();

	ret = blanket_is_amsdu_enabled(ifname, &amsdu);
	ret = blanket_is_ampdu_enabled(ifname, &ampdu);
	ret = blanket_get_ampdu_ba_wsize(ifname, &ampdu_ba_wsize);
	ret = blanket_get_rate(ifname, &rate);
	ret = wbd_slave_get_chan_util(ifname, &chan_util);

	WBD_INFO("amsdu = %d, ampdu = %d, ampdu_ba_wsize =%d, rate = %d\n",
		amsdu, ampdu, ampdu_ba_wsize, rate);
	memset(metric, 0, sizeof(*metric));

	metric->include_bit_esp = IEEE1905_INCL_BIT_ESP_BE;

	/* access category */
	metric->esp_ac_be[0] |= ESP_BE;

	/* data format */
	if (ampdu) {
		metric->esp_ac_be[0] |= ESP_AMPDU_ENABLED;
	}

	if (amsdu) {
		metric->esp_ac_be[0] |= ESP_AMSDU_ENABLED;
	}

	/* BA window size */
	if (ampdu_ba_wsize == 64) {
		metric->esp_ac_be[0] |= ESP_BA_WSIZE_64;
	} else if (ampdu_ba_wsize >= 32) {
		metric->esp_ac_be[0] |= ESP_BA_WSIZE_32;
	} else if (ampdu_ba_wsize >= 16) {
		metric->esp_ac_be[0] |= ESP_BA_WSIZE_16;
	} else if (ampdu_ba_wsize >= 8) {
		metric->esp_ac_be[0] |= ESP_BA_WSIZE_8;
	} else if (ampdu_ba_wsize >= 6) {
		metric->esp_ac_be[0] |= ESP_BA_WSIZE_6;
	} else if (ampdu_ba_wsize >= 4) {
		metric->esp_ac_be[0] |= ESP_BA_WSIZE_4;
	} else if (ampdu_ba_wsize >= 2) {
		metric->esp_ac_be[0] |= ESP_BA_WSIZE_2;
	} else {
		metric->esp_ac_be[0] |= ESP_BA_WSIZE_NONE;
	}

	/* Estimated Air time fraction
	 * 255 representing 100%
	 */
	metric->esp_ac_be[1] = 255 - chan_util;

	/* Data PPDU Duration Target
	 * Duration to transmit 1 packet in
	 * units of 50 microseconds
	 */
	if (rate) {
		ppdu_time =  (1500 * 8) / (rate * 50);
	}

	if (ppdu_time) {
		metric->esp_ac_be[2] = (unsigned char)ppdu_time;
	} else {
		/* Avoid sending out 0 as ppdu_time */
		metric->esp_ac_be[2] = 1;
	}

#if defined(MULTIAPR2)
	memset(&counters, 0, sizeof(counters));
	blanket_get_counters(ifname, &counters);
	metric->unicastBytesSent = (uint64)counters.txbyte;
	metric->unicastBytesReceived = (uint64)counters.rxbyte;
	metric->multicastBytesSent = (uint64)counters.txmcastframe;
	metric->multicastBytesReceived = (uint64)counters.rxmcastframe;
	metric->broadcastBytesSent = (uint64)counters.txbcastframe;
	metric->broadcastBytesReceived = (uint64)counters.rxbcastframe;

	WBD_INFO("Ifname[%s] ESP IE = 0x%x %x %x txbyte[%llu] rxbyte[%llu] txmcastframe[%llu] "
		"rxmcastframe[%llu] txbcastframe[%llu] rxbcastframe[%llu]\n",
		ifname, metric->esp_ac_be[0], metric->esp_ac_be[1], metric->esp_ac_be[2],
		metric->unicastBytesSent, metric->unicastBytesReceived, metric->multicastBytesSent,
		metric->multicastBytesReceived, metric->broadcastBytesSent,
		metric->broadcastBytesReceived);
#endif /* MULTIAPR2 */
	WBD_EXIT();
	return ret;
}

/* Add STAs to sta monitor to measure the RSSI */
int
wbd_slave_process_get_unassoc_sta_metric_cb(wbd_info_t *info,
	ieee1905_unassoc_sta_link_metric_query *query)
{
	int ret = WBDE_OK, i, band;
	BCM_STAMON_STATUS status;
	uint bw = 0, channel;
	chanspec_t chspec;
	bcm_stamon_maclist_t *stamonlist = NULL;
	wbd_slave_item_t *slave = NULL;
	wbd_slave_map_monitor_arg_t *param = NULL;
	unassoc_query_per_chan_rqst *prqst = NULL;
	wbd_maclist_t *maclist = NULL;
	int maclist_size = 0;
	uint8 nextidx = 0;
	i5_dm_device_type *self_dev;
	i5_dm_interface_type *i5_ifr;
	i5_dm_bss_type *i5_bss, *i5_assoc_bss;
	i5_dm_clients_type *i5_assoc_sta;
	WBD_ENTER();

	WBD_ASSERT_ARG(query, WBDE_INV_ARG);
	if (query->chCount <= 0) {
		WBD_WARNING("No Mac list \n");
		goto end;
	}

	WBD_SAFE_GET_I5_SELF_DEVICE(self_dev, &ret);

	prqst = query->data;
	for (i = 0; i < query->chCount; i++) {
		/* get channel */
		struct ether_addr *mac_addr = NULL;
		struct ether_addr *maclist_ea = NULL;
		uint8 n_sta = 0;
		uint8 count = 0;
		bcm_offchan_sta_cbfn *cbfn = NULL;
		channel = prqst[i].chan;

		/* Prepare the chanspec */
		blanket_get_bw_from_rc(query->opClass, &bw);
		band = blanket_opclass_to_band(query->opClass);

		chspec = wf_channel2chspec(channel, bw, band);
		if (!wf_chspec_valid(chspec)) {
			/* traverse next element */
			WBD_INFO("Invalid chanspec. RClass[%d] bw[0x%x] chspec[0x%x]\n",
				query->opClass, bw, chspec);
			continue;
		}

		n_sta = prqst[i].n_sta;
		if (!n_sta) {
			/* traverse next element */
			continue;
		}
		stamonlist = wbd_slave_alloc_stamon_maclist_struct(n_sta);
		if (!stamonlist) {
			ret = WBDE_MALLOC_FL;
			WBD_WARNING("Error: %s\n", wbderrorstr(ret));
			goto end;
		}
		maclist_size = OFFSETOF(wbd_maclist_t, mac) + (n_sta * sizeof(struct ether_addr));
		maclist = (wbd_maclist_t*)wbd_malloc(maclist_size, &ret);
		WBD_ASSERT();

		mac_addr = (struct ether_addr*)prqst[i].mac_list;
		maclist_ea = (struct ether_addr*)&(maclist->mac);

		/* start for each element */
		nextidx = 0;
		for (count = 0; count < n_sta; count++) {
			wbd_assoc_sta_item_t* assoc_sta = NULL;
			i5_dm_device_type *i5_assoc_dev = NULL;
			i5_dm_interface_type *i5_assoc_ifr = NULL;
			ieee1905_ssid_type *ssid = NULL;
			int wbd_band;

			/* Find the STA in the topology */
			i5_assoc_sta = wbd_ds_find_sta_in_topology((unsigned char*)&mac_addr[count],
				&ret);
			if (i5_assoc_sta) {
				assoc_sta = (wbd_assoc_sta_item_t*)i5_assoc_sta->vndr_data;
				i5_assoc_bss = (i5_dm_bss_type*)WBD_I5LL_PARENT(i5_assoc_sta);
				i5_assoc_ifr = (i5_dm_interface_type*)WBD_I5LL_PARENT(i5_assoc_bss);
				i5_assoc_dev = (i5_dm_device_type*)WBD_I5LL_PARENT(i5_assoc_ifr);
				ssid = &i5_assoc_bss->ssid;
			} else {
				WBD_INFO("STA["MACF"] not found in topology\n",
					ETHER_TO_MACF(mac_addr[count]));
			}

			/* Get the band, then get the BSS in the local device based of map flags.
			 * From the that BSSID, get the slave item
			 */
			if (i5_assoc_sta && I5_CLIENT_IS_BSTA(i5_assoc_sta)) {
				/* If the STA is backhaul STA, dont monitor if the STA MAC address
				 * is in self device
				 */
				if (wbd_ds_get_self_i5_interface((unsigned char*)&mac_addr[count],
					&ret)) {
					WBD_INFO("STA["MACDBG"] is backhaul STA and its from "
						"same device\n", MAC2STRDBG(i5_assoc_sta->mac));
					continue;
				}
			}

			/* Find the BSS based on MAP flags and Band. So that the backhaul STA
			 * will be monitored in backhaul BSS and Fronthaul STA will be monitored
			 * in fronthaul BSS. By defualt MAP Flags is fronthaul. So if it is
			 * not able to find the backhaul BSS, it will still use the fronthaul BSS
			 * with matching band
			 */
			wbd_band = ieee1905_get_band_from_channel(query->opClass, channel);
			WBD_DS_FIND_I5_BSS_IN_DEVICE_FOR_BAND_AND_SSID(self_dev, wbd_band, i5_bss,
				ssid, &ret);
			if ((ret != WBDE_OK) || !i5_bss) {
				WBD_INFO("Device["MACDBG"] Band[%d] : %s\n",
					MAC2STRDBG(self_dev->DeviceId), wbd_band, wbderrorstr(ret));
				continue;
			}
			WBD_DEBUG("Device["MACDBG"] Band[%d] BSS["MACDBG"]\n",
				MAC2STRDBG(self_dev->DeviceId), wbd_band,
				MAC2STRDBG(i5_bss->BSSID));
			i5_ifr = (i5_dm_interface_type*)WBD_I5LL_PARENT(i5_bss);
			if (chspec != i5_ifr->chanspec) {
				if (self_dev->BasicCaps &
					IEEE1905_AP_CAPS_FLAGS_UNASSOC_RPT_NON_CH) {
					cbfn = wbd_slave_offchan_sta_cb;
				} else {
					WBD_DEBUG("Device["MACDBG"] offchannel sta monitor is "
						"not enabled\n", MAC2STRDBG(self_dev->DeviceId));
					continue;
				}
			}

			slave = wbd_ds_find_slave_addr_in_blanket_slave(info->wbd_slave,
				(struct ether_addr*)i5_bss->BSSID, WBD_CMP_BSSID, &ret);
			if (!slave) {
				/* look for another element in query */
				WBD_INFO("Device["MACDBG"] Band[%d] BSSID["MACDBG"] : %s\n",
					MAC2STRDBG(self_dev->DeviceId), wbd_band,
					MAC2STRDBG(i5_bss->BSSID), wbderrorstr(ret));
				continue;
			}

			/* if passed STA mac address belongs to assoclist in slave,
			 * skip the operation and send action frame to this sta. Let
			 * other agent listen to the response from sta and measure
			 * the rssi
			 */
			if (i5_assoc_sta && (i5_assoc_dev == i5DmGetSelfDevice())) {
				if (assoc_sta == NULL) {
					WBD_WARNING("STA["MACDBG"] NULL Vendor Data\n",
						MAC2STRDBG(i5_assoc_sta->mac));
					continue;
				}

				/* Create a timer to send ACTION frame to the sta */
				if (assoc_sta->is_offchan_actframe_tm) {
					continue;
				}
				ret = wbd_slave_send_action_frame_to_sta(info, slave,
					(i5_dm_bss_type*)WBD_I5LL_PARENT(i5_assoc_sta),
					(struct ether_addr*)&i5_assoc_sta->mac);
				if (ret == WBDE_OK) {
					assoc_sta->is_offchan_actframe_tm = 1;
				}
			} else {
				nextidx = wbd_slave_add_sta_to_stamon_maclist(stamonlist,
					(struct ether_addr*)&mac_addr[count],
					BCM_STAMON_PRIO_MEDIUM, chspec,	nextidx,
					cbfn, (void*)slave);
				memcpy(&maclist_ea[count], &mac_addr[count],
					sizeof(struct ether_addr));

				maclist->count++;
			}
		}
		/* Add the list to the stamon module */
		if (nextidx > 0) {
			stamonlist->count = nextidx;
			status = bcm_stamon_command(slave->stamon_hdl, BCM_STAMON_CMD_ADD,
				(void*)stamonlist, NULL);
			if (status != BCM_STAMONE_OK) {
				WBD_WARNING("Band[%d] Slave["MACF"] Failed to add %d MAC to stamon "
					"Stamon error : %s\n", slave->band,
					ETHER_TO_MACF(slave->wbd_ifr.mac), stamonlist->count,
					bcm_stamon_strerror(status));
				ret = WBDE_STAMON_ERROR;
				goto end;
			}
			/* Create timer for getting the measured RSSI */
			param = (wbd_slave_map_monitor_arg_t*)wbd_malloc(sizeof(*param), &ret);
			WBD_ASSERT_MSG("Band[%d] Slave["MACF"] Monitor param malloc failed\n",
				slave->band, ETHER_TO_MACF(slave->wbd_ifr.mac));

			param->slave = slave;
			memcpy(param->neighbor_al_mac, query->neighbor_al_mac,
				sizeof(param->neighbor_al_mac));
			param->maclist = maclist;
			param->channel = channel;
			param->rclass = query->opClass;
			ret = wbd_add_timers(info->hdl, param,
				WBD_SEC_MICROSEC(info->max.tm_map_monitor_read),
				wbd_slave_map_monitored_rssi_read_cb, 0);
		} else {
			/* no sta added to stamon list, release memory if allocated */
			if (maclist) {
				free(maclist);
				maclist = NULL;
			}
		}
		if (stamonlist) {
			free(stamonlist);
			stamonlist = NULL;
		}
	} /* iterate next element list */

	return WBDE_OK;
end:
	if (maclist) {
		free(maclist);
		maclist = NULL;
	}
	if (stamonlist) {
		free(stamonlist);
		stamonlist = NULL;
	}
	WBD_EXIT();
	return ret;
}

/* trigger timer to send action frame to client */
static int
wbd_slave_send_action_frame_to_sta(wbd_info_t* info, wbd_slave_item_t *slave,
	i5_dm_bss_type *i5_bss, struct ether_addr* mac_addr)
{
	int ret = WBDE_OK;
	wbd_actframe_arg_t* actframe_arg = NULL;

	if (!info || !slave) {
		ret = WBDE_INV_ARG;
		return ret;
	}
	if (!mac_addr) {
		ret = WBDE_NULL_MAC;
		return ret;
	}
	actframe_arg = (wbd_actframe_arg_t*)wbd_malloc(sizeof(*actframe_arg), &ret);
	WBD_ASSERT();

	actframe_arg->slave = slave;
	actframe_arg->i5_bss = i5_bss;
	memcpy(&(actframe_arg->sta_mac), mac_addr, sizeof(actframe_arg->sta_mac));

	if (actframe_arg && info->max.offchan_af_interval > 0) {
		actframe_arg->af_count = info->max.offchan_af_period /
			info->max.offchan_af_interval;
		ret = wbd_add_timers(info->hdl, actframe_arg,
			WBD_MSEC_USEC(info->max.offchan_af_interval),
			wbd_slave_actframe_timer_cb, 1);

	}
end:
	return ret;
}

/* Send beacon metrics request to a STA */
int wbd_slave_process_beacon_metrics_query_cb(wbd_info_t *info, char *ifname, unsigned char *bssid,
	ieee1905_beacon_request *query)
{
	int ret = WBDE_OK;
	bcnreq_t *bcnreq;
	uint8 len = 0, idx, k = 0;
	uint8 subelement[WBD_MAX_BUF_256];
	int token = 0;
	wbd_beacon_reports_t *report = NULL;
	wbd_slave_map_beacon_report_arg_t *param = NULL;
	int timeout = 0;
	blanket_bcnreq_t bcnreq_extn;
	WBD_ENTER();

	memset(&bcnreq_extn, 0, sizeof(bcnreq_extn));
	bcnreq = &bcnreq_extn.bcnreq;
	/* Initialize Beacon Request Frame data */
	bcnreq->bcn_mode = DOT11_RMREQ_BCN_ACTIVE;
	bcnreq->dur = 10;
	bcnreq->channel = query->channel;
	eacopy(query->sta_mac, &bcnreq->da);
	bcnreq->random_int = 0x0000;
	if (query->ssid.SSID_len > 0) {
		memcpy(bcnreq->ssid.SSID, query->ssid.SSID, query->ssid.SSID_len);
		bcnreq->ssid.SSID_len = query->ssid.SSID_len;
	}
	bcnreq->reps = 0x0000;
	eacopy(bssid, &bcnreq_extn.src_bssid);
	eacopy(&query->bssid, &bcnreq_extn.target_bssid);
	bcnreq_extn.opclass = query->opclass;

	/* Prepare subelement TLVs */
	/* Include reporting detail */
	subelement[len] = 2;
	subelement[len + 1] = 1;
	subelement[len + TLV_HDR_LEN] = query->reporting_detail;
	len += 1 + TLV_HDR_LEN;

	/* Include AP Channel Report */
	if (query->ap_chan_report) {
		for (idx = 0; idx < query->ap_chan_report_count; idx++) {
			subelement[len] = 51;
			subelement[len + 1] = query->ap_chan_report[k]; k++;
			memcpy(&subelement[len + TLV_HDR_LEN], &query->ap_chan_report[k],
				subelement[len + 1]);
			k += subelement[len + 1];
			len += subelement[len + 1] + TLV_HDR_LEN;
		}
	}

	/* Include element IDs */
	if (query->element_list) {
		subelement[len] = 10;
		subelement[len + 1] = query->element_ids_count;
		memcpy(&subelement[len + TLV_HDR_LEN], query->element_list, subelement[len + 1]);
		len += subelement[len + 1] + TLV_HDR_LEN;
	}

	ret = blanket_send_beacon_request(ifname, &bcnreq_extn, subelement, len, &token);
	if (ret != WBDE_OK) {
		WBD_WARNING("Ifname[%s] STA["MACDBG"] Beacon request failed\n",
			ifname, MAC2STRDBG(query->sta_mac));
		goto end;
	}
	WBD_INFO("Ifname[%s] STA["MACDBG"] \n", ifname, MAC2STRDBG(query->sta_mac));
	report = wbd_ds_add_item_to_beacon_reports(info,
		(struct ether_addr*)&query->neighbor_al_mac, 0,
		(struct ether_addr*)&query->sta_mac);
	if (!report) {
		WBD_WARNING("Ifname[%s] STA["MACDBG"] Beacon request add failed\n",
			ifname, MAC2STRDBG(query->sta_mac));
		goto end;
	}

	/* Create timer for checking if the STA sent beacon report or not */
	param = (wbd_slave_map_beacon_report_arg_t*)wbd_malloc(sizeof(*param), &ret);
	WBD_ASSERT_MSG("Ifname[%s] STA["MACDBG"] Beacon report param malloc failed\n",
		ifname, MAC2STRDBG(query->sta_mac));

	param->info = info;
	memcpy(&param->sta_mac, &query->sta_mac, sizeof(param->sta_mac));

	/* Timeout for beacon report sending will be once we exhaust sending all
	 * beacon requests + beacon request duration of each request +
	 * beacon response wait timeout
	*/
	if (query->ap_chan_report_count) {
		timeout = (query->ap_chan_report_count - 1) *
			(WBD_MIN_BCN_REQ_DELAY + bcnreq->dur) +
			info->max.tm_map_send_bcn_report;
	} else {
		timeout = info->max.tm_map_send_bcn_report;
	}

	ret = wbd_add_timers(info->hdl, param, WBD_MSEC_USEC(timeout),
		wbd_slave_map_send_beacon_report_cb, 0);
	if (ret != WBDE_OK) {
		WBD_ERROR("Ifname[%s] STA["MACDBG"] Token[%d] Interval[%d] Failed to create "
			"beacon report timer\n", ifname, MAC2STRDBG(query->sta_mac), token,
			info->max.tm_map_send_bcn_report);
		goto end;
	}
end:
	if (ret != WBDE_OK) {
		if (param) {
			free(param);
		}
		wbd_ds_remove_beacon_report(info, (struct ether_addr*)&query->sta_mac);
		wbd_slave_map_send_beacon_report_error(info,
			IEEE1905_BEACON_REPORT_RESP_FLAG_UNSPECIFIED,
			query->neighbor_al_mac, query->sta_mac);
	}
	WBD_EXIT();
	return ret;
}

/* Send beacon metrics request for each channel to a STA */
int wbd_slave_process_per_chan_beacon_metrics_query_cb(wbd_info_t *info, char *ifname,
	unsigned char *bssid, ieee1905_beacon_request *query)
{
	int ret = WBDE_OK;
	bcnreq_t *bcnreq;
	uint8 len = 0, idx, k = 0;
	uint8 subelement[WBD_MAX_BUF_64];
	uint8 chan[WBD_MAX_BUF_64];
	uint8 opclass[WBD_MAX_BUF_64];
	int idx2;
	uint8 chan_count = 0, tmp_chan_count = 0;
	wbd_beacon_reports_t *report = NULL;
	wbd_slave_map_beacon_report_arg_t *param = NULL;
	wbd_bcn_req_arg_t *bcn_arg = NULL;
	time_t now = 0;
	int timeout = 0, diff = 0;
	blanket_bcnreq_t bcnreq_extn;

	WBD_ENTER();

	/* Check whether we already have reports recieved within WBD_MIN_BCN_METRIC_QUERY_DELAY */
	report = wbd_ds_find_item_fm_beacon_reports(info,
		(struct ether_addr*)&(query->sta_mac), &ret);

	if (report) {
		now = time(NULL);
		diff = now - report->timestamp;
		if (diff > info->max.tm_per_chan_bcn_req) {
			/* Reports are stale free the earlier reports */
			WBD_INFO("Reports are old send beacon request to sta["MACDBG"]"
				"now[%lu]  timestamp[%lu] diff[%d]\n",
				MAC2STRDBG(query->sta_mac), now, report->timestamp, diff);

			wbd_ds_remove_beacon_report(info, (struct ether_addr*)&(query->sta_mac));
		} else {
			/* Already present reports can be sent */
			WBD_INFO("Reports are available for sta["MACDBG"]"
				"now[%lu]  timestamp[%lu] diff[%d]\n",
				MAC2STRDBG(query->sta_mac), now, report->timestamp, diff);

			goto sendreport;
		}
	} else {
		WBD_INFO("No Previous Reports are available for sta["MACDBG"] \n",
			MAC2STRDBG(query->sta_mac));
		ret = WBDE_OK;
	}

	memset(&bcnreq_extn, 0, sizeof(bcnreq_extn));
	bcnreq = &bcnreq_extn.bcnreq;
	/* Initialize Beacon Request Frame data */
	bcnreq->bcn_mode = DOT11_RMREQ_BCN_ACTIVE;
	bcnreq->dur = 10; /* Duration of the measurement in terms of TU's */
	bcnreq->channel = query->channel;
	eacopy(query->sta_mac, &bcnreq->da);
	bcnreq->random_int = 0x0000;
	if (query->ssid.SSID_len > 0) {
		memcpy(bcnreq->ssid.SSID, query->ssid.SSID, query->ssid.SSID_len);
		bcnreq->ssid.SSID_len = query->ssid.SSID_len;
	}
	bcnreq->reps = 0x0000;
	eacopy(bssid, &bcnreq_extn.src_bssid);
	eacopy(&query->bssid, &bcnreq_extn.target_bssid);
	bcnreq_extn.opclass = query->opclass;

	/* Prepare subelement TLVs */
	/* Include reporting detail */
	subelement[len] = 2;
	subelement[len + 1] = 1;
	subelement[len + TLV_HDR_LEN] = query->reporting_detail;
	len += 1 + TLV_HDR_LEN;

	/* Include element IDs */
	if (query->element_list) {
		subelement[len] = 10;
		subelement[len + 1] = query->element_ids_count;
		memcpy(&subelement[len + TLV_HDR_LEN], query->element_list, subelement[len + 1]);
		len += subelement[len + 1] + TLV_HDR_LEN;
	}

	bcn_arg = (wbd_bcn_req_arg_t *) wbd_malloc(sizeof(*bcn_arg), &ret);
	WBD_ASSERT_MSG("Ifname[%s] STA["MACDBG"] Beacon report bcn_arg malloc failed\n",
		ifname, MAC2STRDBG(query->sta_mac));

	memset(bcn_arg, 0, sizeof(*bcn_arg));

	bcn_arg->info = info;
	strncpy(bcn_arg->ifname, ifname, IFNAMSIZ-1);
	eacopy(query->neighbor_al_mac, bcn_arg->neighbor_al_mac);
	bcn_arg->bcnreq_extn = bcnreq_extn;
	memcpy(bcn_arg->subelement, subelement, len);
	bcn_arg->subelem_len = len;

	/* Include AP Channel Report */
	if (query->ap_chan_report) {
		for (idx = 0; idx < query->ap_chan_report_count; idx++) {
			/* Here first byte of ap_chan_report is total length of channels
			 * corresponding to single rclass and second byte is rclass.
			*/
			tmp_chan_count = query->ap_chan_report[k] - 1;
			for (idx2 = 0; idx2 < tmp_chan_count; idx2++) {
				opclass[chan_count + idx2] = query->ap_chan_report[k + 1];
			}
			k += 2;
			memcpy(&chan[chan_count], &query->ap_chan_report[k], tmp_chan_count);
			k += tmp_chan_count;
			chan_count += tmp_chan_count;
		}

		if (chan_count) {
			bcn_arg->chan = (uint8 *) wbd_malloc(chan_count, &ret);
			WBD_ASSERT_MSG("Ifname[%s] STA["MACDBG"] Beacon report malloc failed\n",
				ifname, MAC2STRDBG(query->sta_mac));

			memset(bcn_arg->chan, 0, chan_count);
			memcpy(bcn_arg->chan, chan, chan_count);
			bcn_arg->opclass = (uint8 *) wbd_malloc(chan_count, &ret);
			WBD_ASSERT_MSG("Ifname[%s] STA["MACDBG"] Beacon report malloc failed\n",
				ifname, MAC2STRDBG(query->sta_mac));
			memset(bcn_arg->opclass, 0, chan_count);
			memcpy(bcn_arg->opclass, opclass, chan_count);
			bcn_arg->chan_count = chan_count;
		} else {
			bcn_arg->chan = NULL;
			bcn_arg->chan_count = 0;
		}
	}

	ret = wbd_add_timers(info->hdl, bcn_arg, 0,
			wbd_slave_bcn_req_timer_cb, 0);

	if (ret != WBDE_OK) {
		WBD_ERROR("Ifname[%s] STA["MACDBG"] Failed to create timer\n",
			ifname, MAC2STRDBG(query->sta_mac));
	}

	report = wbd_ds_add_item_to_beacon_reports(bcn_arg->info,
		(struct ether_addr*)&query->neighbor_al_mac, 0,
		(struct ether_addr*)&query->sta_mac);
	if (!report) {
		WBD_WARNING("Ifname[%s] STA["MACDBG"] Beacon request add failed\n",
			ifname, MAC2STRDBG(query->sta_mac));
		goto end;
	}

	/* Timeout for beacon report sending will be once we exhaust sending all
	 * beacon requests + beacon request duration of each request +
	 * beacon response wait timeout
	*/
	if (chan_count) {
		timeout = (chan_count - 1) * (WBD_MIN_BCN_REQ_DELAY + bcnreq->dur) +
			info->max.tm_map_send_bcn_report;
	} else {
		timeout = info->max.tm_map_send_bcn_report;
	}

sendreport:
	/* Create timer for checking if the STA sent beacon report or not */
	param = (wbd_slave_map_beacon_report_arg_t*)wbd_malloc(sizeof(*param), &ret);
	WBD_ASSERT_MSG("Ifname[%s] STA["MACDBG"] Beacon report param malloc failed\n",
		ifname, MAC2STRDBG(query->sta_mac));

	param->info = info;
	memcpy(&param->sta_mac, &query->sta_mac, sizeof(param->sta_mac));

	ret = wbd_add_timers(info->hdl, param,
		WBD_MSEC_USEC(timeout),
		wbd_slave_map_send_beacon_report_cb, 0);
	if (ret != WBDE_OK) {
		WBD_WARNING("Ifname[%s] STA["MACDBG"] Interval[%d] Failed to create "
			"beacon report timer\n", ifname, MAC2STRDBG(query->sta_mac),
			timeout);
		goto end;
	}

end:
	if (ret != WBDE_OK) {
		if (param) {
			free(param);
		}
		wbd_remove_timers(info->hdl, wbd_slave_bcn_req_timer_cb, bcn_arg);
		wbd_ds_remove_beacon_report(info, (struct ether_addr*)query->sta_mac);
		wbd_slave_map_send_beacon_report_error(info,
			IEEE1905_BEACON_REPORT_RESP_FLAG_UNSPECIFIED, query->neighbor_al_mac,
			query->sta_mac);

		if (bcn_arg && bcn_arg->chan) {
			free(bcn_arg->chan);
		}

		if (bcn_arg && bcn_arg->opclass) {
			free(bcn_arg->opclass);
		}

		if (bcn_arg) {
			free(bcn_arg);
		}
	}

	WBD_EXIT();
	return ret;
}

/* Store beacon report */
static int
wbd_slave_store_beacon_report(char *ifname, uint8 *data, struct ether_addr *sta_mac)
{
	int ret = WBDE_OK;
	wl_rrm_event_t *event;
	dot11_rm_ie_t *ie;
	wbd_beacon_reports_t *report;
	uint8 *tmpbuf = NULL;
	uint16 len = 0;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(ifname, WBDE_INV_ARG);
	WBD_ASSERT_ARG(data, WBDE_INV_ARG);

	/* get subtype of the event */
	event = (wl_rrm_event_t *)data;
	WBD_INFO("Ifname[%s] version:0x%02x len:0x%02x cat:0x%02x subevent:0x%02x\n",
		ifname, event->version, event->len, event->cat, event->subevent);

	/* copy event data based on subtype */
	if (event->subevent != DOT11_MEASURE_TYPE_BEACON) {
		WBD_INFO("Ifname[%s] Unhandled event subtype: 0x%2X\n", ifname, event->subevent);
		goto end;
	}

	ie = (dot11_rm_ie_t *)(event->payload);
	dot11_rmrep_bcn_t *rmrep_bcn;
	rmrep_bcn = (dot11_rmrep_bcn_t *)&ie[1];
	WBD_INFO("Ifname[%s] Token[%d] BEACON EVENT, regclass: %d, channel: %d, "
		"rcpi: %d, bssid["MACF"]\n", ifname, ie->token, rmrep_bcn->reg,
		rmrep_bcn->channel, rmrep_bcn->rcpi, ETHER_TO_MACF(rmrep_bcn->bssid));
	/* Traverse through each beaocn report */
	if ((report = wbd_ds_find_item_fm_beacon_reports(wbd_get_ginfo(),
		sta_mac, &ret)) == NULL) {
		WBD_INFO("Token[%d] For STA["MACF"] not found\n",
			ie->token, ETHERP_TO_MACF(sta_mac));
		goto end;
	}

	/* minimum length field is 3 which inclued toke, mode and type */
	if (ie->len <= 3) {
		goto end;
	}

	/* Total length will be ie len plus 2(for element ID and length field) */
	len = report->report_element_len + ie->len + 2;
	/* total len should be less than ethernet frame len minus 11(which is minimum length of the
	 * beacon metrics response TLV
	 */
	if (len > ETH_FRAME_LEN - 11) {
		WBD_INFO("Token[%d] For STA["MACF"] Total length of the beacon report[%d] "
			"exceeding[%d]\n",
			ie->token, ETHERP_TO_MACF(sta_mac), len, ETH_FRAME_LEN);
		goto end;
	}

	tmpbuf = (uint8*)wbd_malloc(len, &ret);
	WBD_ASSERT_MSG("Ifname[%s] STA["MACF"] Beacon report element malloc failed\n",
		ifname, ETHERP_TO_MACF(sta_mac));
	if (report->report_element) {
		memcpy(tmpbuf, report->report_element, report->report_element_len);
		free(report->report_element);
		report->report_element = NULL;
	}
	memcpy(tmpbuf + report->report_element_len, ie, ie->len + 2);
	report->report_element = tmpbuf;
	report->report_element_count++;
	report->report_element_len = len;
	report->timestamp = time(NULL);
	WBD_INFO("Ifname[%s] timestamp[%lu] IE Len[%d] Total Len[%d] Count[%d]\n",
		ifname, report->timestamp, ie->len,
		report->report_element_len, report->report_element_count);
end:
	WBD_EXIT();
	return ret;
}

/* Process channel utilization threshold check */
static void
wbd_slave_chan_util_check_timer_cb(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK;
	unsigned char chan_util;
	unsigned char *mac = (unsigned char*)arg;
	i5_dm_device_type *i5_device_cntrl;
	i5_dm_interface_type *i5_ifr = NULL;
	wbd_ifr_item_t *ifr_vndr_data;
	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(arg, WBDE_INV_ARG);

	/* Find Interface in Self Device, matching to Radio MAC */
	if ((i5_ifr = wbd_ds_get_self_i5_interface(mac, &ret)) == NULL) {
		WBD_INFO("For["MACDBG"]. %s\n", MAC2STRDBG(mac), wbderrorstr(ret));
		goto remove_timer;
	}
	ifr_vndr_data = (wbd_ifr_item_t*)i5_ifr->vndr_data;
	/* if there is no vendor data or chan util thld is 0, remove the timer */
	if (!ifr_vndr_data || ifr_vndr_data->chan_util_thld == 0) {
		goto remove_timer;
	}

	/* Get channel utilization */
	wbd_slave_get_chan_util(i5_ifr->ifname, &chan_util);
	WBD_DEBUG("Ifname[%s] chan_util[%d] thld[%d] reported[%d]\n", i5_ifr->ifname,
		chan_util, ifr_vndr_data->chan_util_thld, ifr_vndr_data->chan_util_reported);

	/* Send AP metrics if the channel utilization is crossed threshold and we haven't reported
	 * when it crossed previously or channel utilization is less than threshold and we haven't
	 * reported previously when it came down.
	 */
	if (((chan_util > ifr_vndr_data->chan_util_thld) &&
		(ifr_vndr_data->chan_util_reported < ifr_vndr_data->chan_util_thld)) ||
		((chan_util < ifr_vndr_data->chan_util_thld) &&
		(ifr_vndr_data->chan_util_reported > ifr_vndr_data->chan_util_thld))) {
		/* Send AP Metrics */
		i5_device_cntrl = i5DmFindController();
		if (i5_device_cntrl) {
			ieee1905_send_ap_metrics_response(i5_device_cntrl->DeviceId, mac);
			ifr_vndr_data->chan_util_reported = chan_util;
			WBD_DEBUG("Ifname[%s] chan_util[%d] thld[%d] reported[%d] Sent AP Metrics "
				"Response.\n", i5_ifr->ifname, chan_util,
				ifr_vndr_data->chan_util_thld, ifr_vndr_data->chan_util_reported);
		}
	}

	goto end;

remove_timer:
	wbd_remove_timers(hdl, wbd_slave_chan_util_check_timer_cb, arg);
	free(arg);

end:
	WBD_EXIT();
	return;
}

/* process channel utilization policy */
static void
wbd_slave_process_chan_util_policy(i5_dm_interface_type *i5_ifr, ieee1905_ifr_metricrpt *metricrpt)
{
	int ret = WBDE_OK;
	wbd_ifr_item_t *ifr_vndr_data;
	unsigned char *mac;

	WBD_ASSERT_ARG(i5_ifr->vndr_data, WBDE_INV_ARG);
	ifr_vndr_data = (wbd_ifr_item_t*)i5_ifr->vndr_data;

	WBD_DEBUG("IFR["MACDBG"] ap_mtrc_chan_util[%d] ifr_vndr_data->chan_util_thld[%d]\n",
		MAC2STRDBG(i5_ifr->InterfaceId), metricrpt->ap_mtrc_chan_util,
		ifr_vndr_data->chan_util_thld);
	/* Check for AP Metrics Channel Utilization Reporting Threshold. AP Metrics
	 * Channel Utilization Reporting Threshold is greate than 0, send ap metrics
	 * report when it crosses the threshold
	 */
	if (metricrpt->ap_mtrc_chan_util > 0) {
		/* If it is already there, no need to process */
		if (ifr_vndr_data->chan_util_thld > 0) {
			goto end;
		}

		ifr_vndr_data->chan_util_thld = metricrpt->ap_mtrc_chan_util;
		ifr_vndr_data->chan_util_reported = 0;

		mac = (unsigned char*)wbd_malloc(MAC_ADDR_LEN, &ret);
		WBD_ASSERT_MSG("IFR["MACDBG"] Failed to alloc chan_util_check arg\n",
			MAC2STRDBG(i5_ifr->InterfaceId));

		memcpy(mac, i5_ifr->InterfaceId, MAC_ADDR_LEN);
		/* Create a timer to send REMOVE_CLIENT_REQ cmd to selected BSS */
		ret = wbd_add_timers(wbd_get_ginfo()->hdl, (void*)mac,
			WBD_SEC_MICROSEC(wbd_get_ginfo()->max.tm_wd_weakclient),
			wbd_slave_chan_util_check_timer_cb, 1);
		if (ret != WBDE_OK) {
			WBD_WARNING("Interval[%d] Failed to create chan_util_check timer\n",
				wbd_get_ginfo()->max.tm_wd_weakclient);
			free(mac);
		}

		/* 1st time process immediately */
		wbd_slave_chan_util_check_timer_cb(wbd_get_ginfo()->hdl, (void*)mac);
	} else {
		/* make threshold to 0. When the timer gets called, it will remove the timer */
		ifr_vndr_data->chan_util_thld = 0;
	}

end:
	return;
}

/* Recieved Multi-AP Metric report policy configuration */
static void
wbd_slave_metric_report_policy_rcvd(wbd_info_t *info, ieee1905_metricrpt_config *metric_policy,
	wbd_cmd_vndr_metric_policy_config_t *vndr_policy)
{
	int ret = WBDE_OK, t_rssi = 0;
	uint32 rc_restart = 0;
	char new_val[WBD_MAX_BUF_128], prefix[IFNAMSIZ+2] = {0};
	dll_t *item_p;
	ieee1905_ifr_metricrpt *metricrpt = NULL;
	i5_dm_interface_type *i5_ifr = NULL;
	wbd_metric_policy_ifr_entry_t *vndr_pol_data = NULL;
	wbd_vndr_metric_rpt_policy_t def_vndr_pol, *ptr_vndr_pol = NULL;
	WBD_ENTER();

	def_vndr_pol.t_idle_rate = WBD_STA_METRICS_REPORTING_IDLE_RATE_THLD;
	def_vndr_pol.t_tx_rate = WBD_STA_METRICS_REPORTING_TX_RATE_THLD;
	def_vndr_pol.t_tx_failures = WBD_STA_METRICS_REPORTING_TX_FAIL_THLD;
	def_vndr_pol.flags = WBD_WEAK_STA_POLICY_FLAG_RSSI;

	/* Travese Metric Policy Config List items */
	foreach_glist_item(item_p, metric_policy->ifr_list) {

		metricrpt = (ieee1905_ifr_metricrpt*)item_p;
		vndr_pol_data = NULL;

		WBD_INFO("Metric Report Policy For["MACDBG"]\n", MAC2STRDBG(metricrpt->mac));

		/* Find Interface in Self Device, matching to Radio MAC, of this Metric Pol Item */
		if ((i5_ifr = wbd_ds_get_self_i5_interface(metricrpt->mac, &ret)) == NULL) {
			WBD_INFO("Metric Report Policy For["MACDBG"]. %s\n",
				MAC2STRDBG(metricrpt->mac), wbderrorstr(ret));
			continue;
		}

		wbd_slave_process_chan_util_policy(i5_ifr, metricrpt);

		/* Convert RCPI to RSSI */
		t_rssi = WBD_RCPI_TO_RSSI(metricrpt->sta_mtrc_rssi_thld);

		/* Find matching Vendor Metric Policy for this Radio MAC */
		if (vndr_policy) {
			vndr_pol_data = wbd_find_vndr_metric_policy_for_radio(metricrpt->mac,
				vndr_policy);
		}

		WBD_INFO("Find Vendor Metric Report Policy For Radio["MACDBG"] : %s. %s\n",
			MAC2STRDBG(metricrpt->mac), vndr_pol_data ? "Success" : "Failure",
			vndr_pol_data ? "" : "Using Default Vendor Policy.");

		/* Get Vendor Metric Policy pointer */
		ptr_vndr_pol = vndr_pol_data ? (&(vndr_pol_data->vndr_policy)) : (&def_vndr_pol);

		/* Get Prefix for this Radio MAC */
		snprintf(prefix, sizeof(prefix), "%s_", i5_ifr->wlParentName);

		/* Prepare New Value of "wbd_weak_sta_cfg" from Policy Received from Controller */
		snprintf(new_val, sizeof(new_val), "%d %d %d %d %d 0x%x",
			ptr_vndr_pol->t_idle_rate, t_rssi, metricrpt->sta_mtrc_rssi_hyst,
			ptr_vndr_pol->t_tx_rate, ptr_vndr_pol->t_tx_failures, ptr_vndr_pol->flags);

		/* Match NVRAM and NEW value, and if mismatch, Set new value in NVRAM */
		rc_restart |= blanket_nvram_prefix_match_set(prefix, WBD_NVRAM_WEAK_STA_CFG,
			new_val, FALSE);

		WBD_INFO("MAC["MACDBG"] IDLE_RATE[%d] RCPI[%d (%d)] Hysterisis[%d] "
			"PHY_RATE[%d] TX_FAILURES[%d] FLAGS[0x%X] "
			"NVRAM[%s%s=%s]\n", MAC2STRDBG(metricrpt->mac), ptr_vndr_pol->t_idle_rate,
			metricrpt->sta_mtrc_rssi_thld, t_rssi, metricrpt->sta_mtrc_rssi_hyst,
			ptr_vndr_pol->t_tx_rate, ptr_vndr_pol->t_tx_failures,
			ptr_vndr_pol->flags, prefix, WBD_NVRAM_WEAK_STA_CFG, new_val);
	}

	WBD_INFO("Weak STA Config %s Changed.%s\n", rc_restart ? "" : "NOT",
		rc_restart ? "RC Restart...!!" : "");

	/* If required, Execute nvram commit/rc restart/reboot commands */
	if (rc_restart) {
		WBD_INFO("Creating rc restart timer\n");
		wbd_slave_create_rc_restart_timer(info);
		info->flags |= WBD_INFO_FLAGS_RC_RESTART;
	}

	WBD_EXIT();
}

/* Recieved Multi-AP Strong Sta report policy configuration */
static void
wbd_slave_strong_sta_report_policy_rcvd(wbd_info_t *info,
	ieee1905_metricrpt_config *metric_policy,
	wbd_cmd_vndr_strong_sta_policy_config_t *vndr_strong_sta_policy)
{
	int ret = WBDE_OK;
	uint32 rc_restart = 0;
	char new_val[WBD_MAX_BUF_128], prefix[IFNAMSIZ+2] = {0};
	dll_t *item_p;
	ieee1905_ifr_metricrpt *metricrpt = NULL;
	i5_dm_interface_type *i5_ifr = NULL;
	wbd_strong_sta_policy_ifr_entry_t *vndr_pol_data = NULL;
	wbd_vndr_strong_sta_rpt_policy_t def_vndr_pol, *ptr_vndr_pol = NULL;
	WBD_ENTER();

	def_vndr_pol.t_idle_rate = WBD_STRONG_STA_METRICS_REPORTING_IDLE_RATE_THLD;
	def_vndr_pol.t_rssi = WBD_STRONG_STA_METRICS_REPORTING_RSSI_THLD;
	def_vndr_pol.t_hysterisis = WBD_STRONG_STA_METRICS_REPORTING_RSSI_HYSTERISIS_MARGIN;
	def_vndr_pol.flags = WBD_STRONG_STA_POLICY_FLAG_RSSI;

	/* Travese Metric Policy Config List items */
	foreach_glist_item(item_p, metric_policy->ifr_list) {

		metricrpt = (ieee1905_ifr_metricrpt*)item_p;
		vndr_pol_data = NULL;

		WBD_INFO("Strong Sta Report Policy For["MACDBG"]\n", MAC2STRDBG(metricrpt->mac));

		/* Find Interface in Self Device, matching to Radio MAC, of this Metric Pol Item */
		if ((i5_ifr = wbd_ds_get_self_i5_interface(metricrpt->mac, &ret)) == NULL) {
			WBD_INFO("Strong Sta Report Policy For["MACDBG"]. %s\n",
				MAC2STRDBG(metricrpt->mac), wbderrorstr(ret));
			continue;
		}

		if (vndr_strong_sta_policy) {
			/* Find matching Vendor Metric Policy for this Radio MAC */
			vndr_pol_data = wbd_find_vndr_strong_sta_policy_for_radio(metricrpt->mac,
				vndr_strong_sta_policy);
		}

		WBD_INFO("Find Vendor Strong Sta Report Policy For Radio["MACDBG"] : %s. %s\n",
			MAC2STRDBG(metricrpt->mac), vndr_pol_data ? "Success" : "Failure",
			vndr_pol_data ? "" : "Using Default Vendor Policy.");

		/* Get Vendor Strong Sta Policy pointer */
		ptr_vndr_pol = vndr_pol_data ? (&(vndr_pol_data->vndr_policy)) : (&def_vndr_pol);

		/* Get Prefix for this Radio MAC */
		snprintf(prefix, sizeof(prefix), "%s_", i5_ifr->wlParentName);

		/* Prepare New Value of "wbd_strong_sta_cfg" from Policy Received from Controller */
		snprintf(new_val, sizeof(new_val), "%d %d %d 0x%x",
			ptr_vndr_pol->t_idle_rate, ptr_vndr_pol->t_rssi,
			ptr_vndr_pol->t_hysterisis, ptr_vndr_pol->flags);

		/* Match NVRAM and NEW value, and if mismatch, Set new value in NVRAM */
		rc_restart |= blanket_nvram_prefix_match_set(prefix, WBD_NVRAM_STRONG_STA_CFG,
			new_val, FALSE);

		WBD_INFO("MAC["MACDBG"] Idle_rate [%d] RSSI[%d] Hysterisis[%d] "
			"flags[0x%x] NVRAM[%s%s=%s]\n", MAC2STRDBG(metricrpt->mac),
			ptr_vndr_pol->t_idle_rate, ptr_vndr_pol->t_rssi,
			ptr_vndr_pol->t_hysterisis, ptr_vndr_pol->flags,
			prefix, WBD_NVRAM_STRONG_STA_CFG, new_val);
	}

	WBD_INFO("Strong STA Config %s Changed.%s\n", rc_restart ? "" : "NOT",
		rc_restart ? "RC Restart...!!" : "");

	/* If required, Execute nvram commit/rc restart/reboot commands */
	if (rc_restart) {
		WBD_INFO("Creating rc restart timer\n");
		wbd_slave_create_rc_restart_timer(info);
		info->flags |= WBD_INFO_FLAGS_RC_RESTART;
	}

	WBD_EXIT();
}

/* process vendor specific metric reporting policy */
static void
wbd_slave_process_metric_reportig_policy_vndr_cmd(unsigned char *src_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len)
{
	int ret = WBDE_OK;
	unsigned int bytes_read = 0;
	wbd_info_t *info = wbd_get_ginfo();
	wbd_cmd_vndr_metric_policy_config_t *vndr_policy = NULL;

	if (info->wbd_slave->vndr_policy == NULL) {
		info->wbd_slave->vndr_policy = (wbd_cmd_vndr_metric_policy_config_t*)
			wbd_malloc(sizeof(wbd_cmd_vndr_metric_policy_config_t), &ret);
		WBD_ASSERT();
		wbd_ds_glist_init(&(info->wbd_slave->vndr_policy->entry_list));
	}

	vndr_policy = info->wbd_slave->vndr_policy;

	if (vndr_policy->num_entries > 0) {
		/* Remove all VNDR_METRIC_POLICY data items */
		wbd_ds_glist_cleanup(&(vndr_policy->entry_list));
	}
	wbd_ds_glist_init(&(vndr_policy->entry_list));

	/* Decode Vendor Specific TLV : Metric Policy Vendor Data on receive */
	ret = wbd_tlv_decode_vndr_metric_policy((void *)vndr_policy,
		tlv_data, tlv_data_len, &bytes_read);
	WBD_ASSERT_MSG("Failed to decode Metrics Policy\n");

end:
	return;
}

/* process vendor specific strong STA reporting policy */
static void
wbd_slave_process_strong_sta_reportig_policy_vndr_cmd(unsigned char *src_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len)
{
	int ret = WBDE_OK;
	unsigned int bytes_read = 0;
	wbd_info_t *info = wbd_get_ginfo();
	wbd_cmd_vndr_strong_sta_policy_config_t *vndr_strong_sta_policy = NULL;

	if (info->wbd_slave->vndr_strong_sta_policy == NULL) {
		info->wbd_slave->vndr_strong_sta_policy = (wbd_cmd_vndr_strong_sta_policy_config_t*)
			wbd_malloc(sizeof(wbd_cmd_vndr_metric_policy_config_t), &ret);
		WBD_ASSERT();
		wbd_ds_glist_init(&(info->wbd_slave->vndr_strong_sta_policy->entry_list));
	}

	vndr_strong_sta_policy = info->wbd_slave->vndr_strong_sta_policy;

	if (vndr_strong_sta_policy->num_entries > 0) {
		/* Remove all VNDR_STRONG_STA_POLICY data items */
		wbd_ds_glist_cleanup(&(vndr_strong_sta_policy->entry_list));
	}
	wbd_ds_glist_init(&(vndr_strong_sta_policy->entry_list));

	/* Decode Vendor Specific TLV : Strong Sta Policy Vendor Data on receive */
	ret = wbd_tlv_decode_vndr_strong_sta_policy((void *)vndr_strong_sta_policy,
		tlv_data, tlv_data_len, &bytes_read);
	WBD_ASSERT_MSG("Failed to decode Strong Sta Policy\n");

end:
	return;
}

/* recieved Multi-AP Policy Configuration */
void
wbd_slave_policy_configuration_cb(wbd_info_t *info, ieee1905_policy_config *policy,
	unsigned short rcvd_policies, ieee1905_vendor_data *in_vndr_tlv)
{
	int ret = WBDE_OK;
	wbd_cmd_vndr_metric_policy_config_t *vndr_policy; /* Metric Policy Vendor Data */
	wbd_cmd_vndr_strong_sta_policy_config_t *vndr_strong_sta_policy;	/* Strong Sta
										 * Vendor Data
										 */
	bool rc_restart = FALSE;
	WBD_ENTER();

	/* If metric report policy configuration recieved */
	if (rcvd_policies & MAP_POLICY_TYPE_FLAG_METRIC_REPORT) {

		vndr_policy = info->wbd_slave->vndr_policy;
		vndr_strong_sta_policy = info->wbd_slave->vndr_strong_sta_policy;

		/* Process Recieved Multi-AP Metric Report Policy Configuration */
		wbd_slave_metric_report_policy_rcvd(info, &policy->metricrpt_config, vndr_policy);

		if (vndr_policy) {
			/* If VNDR_METRIC_POLICY List is filled up */
			if (vndr_policy->num_entries > 0) {

				/* Remove all VNDR_METRIC_POLICY data items */
				wbd_ds_glist_cleanup(&(vndr_policy->entry_list));
			}
			free(vndr_policy);
			info->wbd_slave->vndr_policy = NULL;
		}

		/* Process Recieved Multi-AP Strong Sta Report Policy Configuration */
		wbd_slave_strong_sta_report_policy_rcvd(info, &policy->metricrpt_config,
			vndr_strong_sta_policy);

		if (vndr_strong_sta_policy) {
			/* If VNDR_STRONG_STA_POLICY List is filled up */
			if (vndr_strong_sta_policy->num_entries > 0) {

				/* Remove all VNDR_STRONG_STA_POLICY data items */
				wbd_ds_glist_cleanup(&(vndr_strong_sta_policy->entry_list));
			}
			free(vndr_strong_sta_policy);
			info->wbd_slave->vndr_strong_sta_policy = NULL;
		}

	}

#if defined(MULTIAPR2)
	/* If traffic separation policy configuration recieved */
	if (rcvd_policies &
		(MAP_POLICY_TYPE_FLAG_TS_8021QSET | MAP_POLICY_TYPE_FLAG_TS_POLICY)) {
		if (wbd_slave_process_traffic_separation_policy(policy, rcvd_policies) == 0) {
			rc_restart = TRUE;
		}
	}
	/* If Unsuccessful Association report policy configuration recieved */
	if (rcvd_policies & MAP_POLICY_TYPE_FLAG_UNSUCCESSFUL_ASSOCIATION) {
		memcpy(&(info->wbd_slave->unsuccessful_assoc_config),
			&(policy->unsuccessful_assoc_config),
			sizeof(info->wbd_slave->unsuccessful_assoc_config));

		if (info->wbd_slave->unsuccessful_assoc_config.report_flag &
				MAP_UNSUCCESSFUL_ASSOC_FLAG_REPORT) {
			ret = wbd_add_timers(info->hdl,
				&info->wbd_slave->unsuccessful_assoc_config,
				WBD_SEC_MICROSEC(IEEE1905_RESET_UNSUCCESSFUL_ASSOC_COUNT_TIEMOUT),
				wbd_slave_reset_unsuccessful_assoc_count_timer_cb, 1);
			if (ret != WBDE_OK) {
				WBD_WARNING("Failed to create reset unsuccessful association "
					"count timer for Interval[%d]\n",
					IEEE1905_RESET_UNSUCCESSFUL_ASSOC_COUNT_TIEMOUT);
			}
		} else {
			wbd_remove_timers(info->hdl,
				wbd_slave_reset_unsuccessful_assoc_count_timer_cb,
				&info->wbd_slave->unsuccessful_assoc_config);
		}
	}
	/* If Backhaul BSS policy configuration received */
	if (rcvd_policies &
		(MAP_POLICY_TYPE_FLAG_BACKHAUL_BSS)) {
		if (wbd_slave_process_backhaulbss_config_policy(policy) == 0) {
			rc_restart = TRUE;
		}
	}
#endif /* MULTIAPR2 */

	if (rc_restart) {
		WBD_INFO("Creating rc restart timer\n");
		info->flags |= WBD_INFO_FLAGS_RC_RESTART;
		wbd_slave_create_rc_restart_timer(info);
	}
	WBD_EXIT();
}

/* Callback for exception from communication handler for slave server */
static void
wbd_slave_com_server_exception(wbd_com_handle *hndl, void *arg, WBD_COM_STATUS status)
{
	WBD_ERROR("Exception from slave server\n");
}

/* Callback for exception from communication handler for CLI */
static void
wbd_slave_com_cli_exception(wbd_com_handle *hndl, void *arg, WBD_COM_STATUS status)
{
	WBD_ERROR("Exception from CLI server\n");
}

/* Callback for exception from communication handler for EVENTD */
/* static void
wbd_slave_com_eventd_exception(wbd_com_handle *hndl, void *arg, WBD_COM_STATUS status)
{
	WBD_ERROR("Exception from EVENTD server\n");
}
*/

/* Register all the commands for master server to communication handle */
static int
wbd_slave_register_server_command(wbd_info_t *info)
{
	/* Commands from BSD */
	wbd_com_register_cmd(info->com_serv_hdl, WBD_CMD_WEAK_CLIENT_BSD_REQ,
		wbd_slave_process_weak_client_bsd_cmd, info,
		wbd_json_parse_weak_strong_client_bsd_cmd);

	wbd_com_register_cmd(info->com_serv_hdl, WBD_CMD_WEAK_CANCEL_BSD_REQ,
		wbd_slave_process_weak_cancel_bsd_cmd, info,
		wbd_json_parse_weak_strong_cancel_bsd_cmd);

	wbd_com_register_cmd(info->com_serv_hdl, WBD_CMD_STA_STATUS_BSD_REQ,
		wbd_slave_process_sta_status_bsd_cmd, info, wbd_json_parse_sta_status_bsd_cmd);

	wbd_com_register_cmd(info->com_serv_hdl, WBD_CMD_BLOCK_CLIENT_BSD_REQ,
		wbd_slave_process_blk_client_bsd_cmd, info, wbd_json_parse_blk_client_bsd_cmd);

	wbd_com_register_cmd(info->com_serv_hdl, WBD_CMD_STRONG_CLIENT_BSD_REQ,
		wbd_slave_process_strong_client_bsd_cmd, info,
		wbd_json_parse_weak_strong_client_bsd_cmd);

	wbd_com_register_cmd(info->com_serv_hdl, WBD_CMD_STRONG_CANCEL_BSD_REQ,
		wbd_slave_process_strong_cancel_bsd_cmd, info,
		wbd_json_parse_weak_strong_cancel_bsd_cmd);

	/* Now register CLI commands */
	wbd_com_register_cmd(info->com_cli_hdl, WBD_CMD_CLI_VERSION,
		wbd_process_version_cli_cmd, info, wbd_json_parse_cli_cmd);

	wbd_com_register_cmd(info->com_cli_hdl, WBD_CMD_CLI_SLAVELIST,
		wbd_slave_process_slavelist_cli_cmd, info, wbd_json_parse_cli_cmd);

	wbd_com_register_cmd(info->com_cli_hdl, WBD_CMD_CLI_INFO,
		wbd_slave_process_info_cli_cmd, info, wbd_json_parse_cli_cmd);

	wbd_com_register_cmd(info->com_cli_hdl, WBD_CMD_CLI_CLIENTLIST,
		wbd_slave_process_clientlist_cli_cmd, info, wbd_json_parse_cli_cmd);

	wbd_com_register_cmd(info->com_cli_hdl, WBD_CMD_CLI_MONITORLIST,
		wbd_slave_process_monitorlist_cli_cmd, info, wbd_json_parse_cli_cmd);

	wbd_com_register_cmd(info->com_cli_hdl, WBD_CMD_CLI_MONITORADD,
		wbd_slave_process_monitoradd_cli_cmd, info, wbd_json_parse_cli_cmd);

	wbd_com_register_cmd(info->com_cli_hdl, WBD_CMD_CLI_MONITORDEL,
		wbd_slave_process_monitordel_cli_cmd, info, wbd_json_parse_cli_cmd);

	wbd_com_register_cmd(info->com_cli_hdl, WBD_CMD_CLI_MSGLEVEL,
		wbd_process_set_msglevel_cli_cmd, info, wbd_json_parse_cli_cmd);

	/* Now register EVENTD commands */

	return WBDE_OK;
}

/* Initialize the communication module for slave */
int
wbd_init_slave_com_handle(wbd_info_t *info)
{
	/* Initialize communication module for server FD */
	info->com_serv_hdl = wbd_com_init(info->hdl, info->server_fd, 0x0000,
		wbd_json_parse_cmd_name, wbd_slave_com_server_exception, info);
	if (!info->com_serv_hdl) {
		WBD_ERROR("Failed to initialize the communication module for slave server\n");
		return WBDE_COM_ERROR;
	}

	/* Initialize communication module for CLI */
	info->com_cli_hdl = wbd_com_init(info->hdl, info->cli_server_fd, 0x0000,
		wbd_json_parse_cli_cmd_name, wbd_slave_com_cli_exception, info);
	if (!info->com_cli_hdl) {
		WBD_ERROR("Failed to initialize the communication module for CLI server\n");
		return WBDE_COM_ERROR;
	}

	/* Initialize communication module for EVENTFD */
	/* info->com_eventd_hdl = wbd_com_init(info->hdl, info->event_fd, WBD_COM_SERV_TYPE_BIN,
		wbd_json_parse_cmd_name, wbd_slave_com_eventd_exception, info);
	if (!info->com_cli_hdl) {
		WBD_ERROR("Failed to initialize the communication module for EVENTD server\n");
		return WBDE_COM_ERROR;
	}
	*/

	wbd_slave_register_server_command(info);

	return WBDE_OK;
}

/* Utility to loop in all bss of self device
 * and add rrm static neighbor list
*/
static void
wbd_slave_add_nbr(i5_dm_bss_type *nbss, chanspec_t nchanspec,
	uint8 nrclass, i5_dm_device_type *sdev, bool noselfbss)
{
	i5_dm_interface_type *sifr = NULL;
	i5_dm_bss_type *sbss = NULL;
	blanket_nbr_info_t bkt_nbr;

	WBD_INFO("Adding Neighbor["MACDBG"] chanspec = %x \n",
			MAC2STRDBG(nbss->BSSID), nchanspec);

	memset(&bkt_nbr, 0, sizeof(bkt_nbr));

	eacopy((struct ether_addr *)(nbss->BSSID), &(bkt_nbr.bssid));

	bkt_nbr.channel = wf_chspec_ctlchan(nchanspec);
	memcpy(&(bkt_nbr.ssid.SSID), &(nbss->ssid.SSID), nbss->ssid.SSID_len);
	bkt_nbr.ssid.SSID_len = strlen((const char*)bkt_nbr.ssid.SSID);
	bkt_nbr.chanspec = nchanspec;
	bkt_nbr.reg = nrclass;

	foreach_i5glist_item(sifr, i5_dm_interface_type, sdev->interface_list) {

		/* skip if this is not wireless interface */
		if (!i5DmIsInterfaceWireless(sifr->MediaType) ||
			!sifr->BSSNumberOfEntries) {
			continue;
		}

		foreach_i5glist_item(sbss, i5_dm_bss_type, sifr->bss_list) {
			/* Add Bss in neighbor list only if ssid is same */
			if (!WBD_SSIDS_MATCH(sbss->ssid, nbss->ssid)) {
				continue;
			}

			/* skip for own bss */
			if (noselfbss && (memcmp(sbss->BSSID, nbss->BSSID, MAC_ADDR_LEN) == 0)) {
				continue;
			}

			blanket_get_rclass(sbss->ifname, nchanspec, &(bkt_nbr.reg));
			blanket_add_nbr(sbss->ifname, &bkt_nbr);
		}
	}
}

/* Add AP's from whole blanket in static neighbor list */
void
wbd_slave_add_nbr_from_topology()
{
	int ret = WBDE_OK;
	i5_dm_network_topology_type *topology;
	i5_dm_device_type *dev = NULL, *sdev = NULL;
	i5_dm_interface_type *ifr = NULL;
	i5_dm_bss_type *bss = NULL;
	WBD_ENTER();

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);

	/* Get Topology of Agent from 1905 lib */
	topology = (i5_dm_network_topology_type *)ieee1905_get_datamodel();

	/* Loop for all the Devices in Agent Topology */
	foreach_i5glist_item(dev, i5_dm_device_type, topology->device_list) {

		/* skip for self device */
		if (memcmp(sdev->DeviceId, dev->DeviceId, MAC_ADDR_LEN) == 0) {
			continue;
		}

		foreach_i5glist_item(ifr, i5_dm_interface_type, dev->interface_list) {
			if (!i5DmIsInterfaceWireless(ifr->MediaType) ||
				!ifr->BSSNumberOfEntries) {
				continue;
			}

			/* Loop for all the BSSs in Interface */
			foreach_i5glist_item(bss, i5_dm_bss_type, ifr->bss_list) {

				/* add neighbor in self device bss */
				wbd_slave_add_nbr(bss, ifr->chanspec, ifr->opClass, sdev, FALSE);
			}
		}
	}

end:
	wbd_slave_add_nbr_from_self_dev(sdev);
	WBD_EXIT();
}

/* Add self device AP's in static neighbor list */
void
wbd_slave_add_nbr_from_self_dev(i5_dm_device_type *sdev)
{
	i5_dm_interface_type *ifr = NULL;
	i5_dm_bss_type *bss = NULL;
	WBD_ENTER();

	/* Loop through own device */
	foreach_i5glist_item(ifr, i5_dm_interface_type, sdev->interface_list) {
		if (!i5DmIsInterfaceWireless(ifr->MediaType) ||
			!ifr->BSSNumberOfEntries) {
			continue;
		}

		if (CHSPEC_IS6G(ifr->chanspec)) {
			return;
		}

		/* Loop for all the BSSs in Interface */
		foreach_i5glist_item(bss, i5_dm_bss_type, ifr->bss_list) {

			/* add neighbor in self device bss */
			wbd_slave_add_nbr(bss, ifr->chanspec, ifr->opClass, sdev, TRUE);
		}
	}

	WBD_EXIT();
}

/* Add new AP in static neighbor list */
void
wbd_slave_add_bss_nbr(i5_dm_bss_type *bss)
{
	int ret = WBDE_OK;
	i5_dm_device_type *dev = NULL, *sdev = NULL;
	i5_dm_interface_type *ifr = NULL;
	WBD_ENTER();

	/* Get self device */
	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);

	/* get interface of bss */
	ifr = (i5_dm_interface_type *)WBD_I5LL_PARENT(bss);

	/* get device of bss */
	dev = (i5_dm_device_type *)WBD_I5LL_PARENT(ifr);

	/* skip for self device */
	if (memcmp(sdev->DeviceId, dev->DeviceId, MAC_ADDR_LEN) == 0) {
		goto end;
	}

	/* add neighbor in self device bss */
	wbd_slave_add_nbr(bss, ifr->chanspec, ifr->opClass, sdev, FALSE);

end:
	WBD_EXIT();
}

/* Delete an AP from static neighbor list */
void
wbd_slave_del_bss_nbr(i5_dm_bss_type *bss)
{
	int ret = WBDE_OK;
	i5_dm_device_type *dev = NULL, *sdev = NULL;
	i5_dm_interface_type *ifr = NULL, *sifr = NULL;
	i5_dm_bss_type *sbss = NULL;
	WBD_ENTER();

	/* Get self device */
	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);

	/* get interface of bss */
	ifr = (i5_dm_interface_type *)WBD_I5LL_PARENT(bss);

	/* get device of bss */
	dev = (i5_dm_device_type *)WBD_I5LL_PARENT(ifr);

	/* skip for self device */
	if (memcmp(sdev->DeviceId, dev->DeviceId, MAC_ADDR_LEN) == 0) {
		goto end;
	}

	WBD_INFO("Deleting Neighbor["MACDBG"] \n", MAC2STRDBG(bss->BSSID));

	foreach_i5glist_item(sifr, i5_dm_interface_type, sdev->interface_list) {
		foreach_i5glist_item(sbss, i5_dm_bss_type, sifr->bss_list) {
			blanket_del_nbr(sbss->ifname, (struct ether_addr *)(bss->BSSID));
		}
	}

end:
	WBD_EXIT();
}

/* Add neighbor in static neiohgbor list entry with updated channel */
void
wbd_slave_add_ifr_nbr(i5_dm_interface_type *i5_ifr, bool noselfbss)
{
	int ret = WBDE_OK;
	i5_dm_device_type *sdev = NULL;
	i5_dm_bss_type *bss = NULL;
	WBD_ENTER();

	if (noselfbss && CHSPEC_IS6G(i5_ifr->chanspec)) {
		return;
	}

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);

	/* Loop for all the BSSs in Interface */
	foreach_i5glist_item(bss, i5_dm_bss_type, i5_ifr->bss_list) {

		/* add neighbor in self device bss */
		wbd_slave_add_nbr(bss, i5_ifr->chanspec, i5_ifr->opClass, sdev, noselfbss);
	}
end:
	WBD_EXIT();
}

void
wbd_slave_send_opchannel_reports(void)
{
	int ret = WBDE_OK;
	i5_dm_device_type *sdev = NULL;
	i5_dm_interface_type *sifr = NULL;

	WBD_ENTER();

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);
	foreach_i5glist_item(sifr, i5_dm_interface_type, sdev->interface_list) {
		chanspec_t chanspec = 0;
		/* Send OpChannel Report, only for Requested & Valid Wi-Fi Interfaces */
		if ((!i5DmIsInterfaceWireless(sifr->MediaType)) ||
			!(sifr->chan_sel_resp_code > I5_CHAN_SEL_RESP_CODE_NONE)) {
			continue;
		}
		/* Get chanspec from firmware */
		blanket_get_chanspec(sifr->ifname, &chanspec);
		if (chanspec != sifr->chanspec) {
			sifr->chanspec = chanspec;
			ieee1905_notify_channel_change(sifr);
		}
		blanket_get_global_rclass(sifr->chanspec, &(sifr->opClass));
		wbd_slave_send_operating_chan_report(sifr);
		sifr->chan_sel_resp_code = I5_CHAN_SEL_RESP_CODE_NONE;
		wbd_slave_add_ifr_nbr(sifr, TRUE);
	}
end:
	WBD_EXIT();
}

static void
wbd_slave_send_operating_chan_report(i5_dm_interface_type *sifr)
{
	ieee1905_operating_chan_report chan_rpt;
	uint8 opclass;
	uint8 channel;
	int tx_pwr;
	int ret = WBDE_OK;
	int i, j, idx = 0;
	uint8 rc_first, rc_last;
	i5_dm_rc_chan_map_type *g_rc_chan_map = NULL;
	uint32 g_rc_chan_map_count = 0;
	uint cur_bw = 0;

	WBD_ENTER();

	memset(&chan_rpt, 0, sizeof(chan_rpt));
	chan_rpt.list =	(operating_rpt_opclass_chan_list *)wbd_malloc(
			sizeof(operating_rpt_opclass_chan_list) * I5_MAX_INTF_RCS, &ret);
	WBD_ASSERT();

	memcpy(&chan_rpt.radio_mac, &sifr->InterfaceId, sizeof(chan_rpt.radio_mac));
	blanket_get_txpwr_target_max(sifr->ifname, &tx_pwr);
	chan_rpt.tx_pwr = (uint8)tx_pwr;

	blanket_get_global_rclass(sifr->chanspec, &opclass);
	channel = wf_chspec_ctlchan(sifr->chanspec);
	blanket_get_bw_from_rc(opclass, &cur_bw);
	if ((opclass < REGCLASS_24G_FIRST) || (opclass > REGCLASS_6G_LAST)) {
		WBD_ERROR("radio ["MACF"] has invalid chanspec [0x%x] opclass [%d] channel [%d]\n",
			ETHERP_TO_MACF(sifr->InterfaceId), sifr->chanspec, opclass, channel);
		goto end;
	}

	WBD_INFO("Radio ["MACF"] chanspec [0x%x] opclass [%d] channel [%d] tx_pwr [%d]\n",
		ETHERP_TO_MACF(sifr->InterfaceId), sifr->chanspec, opclass, channel, tx_pwr);

	/* Identify the operating class range based on current opearaing class.
	 * ie, ignore out of band operating classes
	 */
	if (opclass <= REGCLASS_24G_LAST) {
		rc_first = REGCLASS_24G_FIRST;
		rc_last = REGCLASS_24G_LAST;
	} else if ((opclass >= REGCLASS_6G_FIRST)) {
		rc_first = REGCLASS_6G_FIRST;
		rc_last = REGCLASS_6G_LAST;
	} else {
		rc_first = REGCLASS_5G_FIRST;
		rc_last = REGCLASS_5G_LAST;
	}

	g_rc_chan_map = i5DmGetRCChannelMap(&g_rc_chan_map_count);
	for (i = 0; i < g_rc_chan_map_count; i++) {
		uint bw = 0;

		if (g_rc_chan_map[i].regclass < rc_first) {
			continue;
		}
		if (g_rc_chan_map[i].regclass > rc_last) {
			/* All the opclasses in current band are over. exit */
			break;
		}
		/* Skip the higher bw operating classes */
		blanket_get_bw_from_rc(g_rc_chan_map[i].regclass, &bw);
		if (bw > cur_bw) {
			continue;
		}

		/* Iterate through the channel list for a matching channel */
		for (j = 0; (j < g_rc_chan_map[i].count) && (idx < I5_MAX_INTF_RCS); j++) {
			int min = 0, max = 0;

			if (g_rc_chan_map[i].regclass <= REGCLASS_5G_40MHZ_LAST) {
				/* Table E-4 for this opclass has control channel. So the
				 * channel has to match exactly
				 */
				if (channel == g_rc_chan_map[i].channel[j]) {
					/* Add this channel and continue with next opclass */
					chan_rpt.list[idx].op_class = g_rc_chan_map[i].regclass;
					chan_rpt.list[idx].chan = channel;
					idx++;
					WBD_DEBUG("%d: Supported opclass [%d] control channel "
						"[%d]\n", idx, g_rc_chan_map[i].regclass, channel);
					break;
				}
				continue;
			}

			/* Handle center channel cases. 6G 20MHz also will be handled here.
			 * It is ok becasue in 20MHz center and control channel are same
			 */
			switch (bw) {
				case WL_CHANSPEC_BW_20:
					min = channel;
					max = channel;
					break;
				case WL_CHANSPEC_BW_40:
					min = channel - 2;
					max = channel + 2;
					break;
				case WL_CHANSPEC_BW_80:
					min = channel - 6;
					max = channel + 6;
					break;
				case WL_CHANSPEC_BW_160:
					min = channel - 14;
					max = channel + 14;
					break;

			}
			/* For any control channel, the center channel will be in the range of
			 * min - max. So if a center channel is found within this range add it to
			 * the list of supported operating classes
			 */
			if ((g_rc_chan_map[i].channel[j] <= max) &&
				(g_rc_chan_map[i].channel[j] >= min)) {
				chan_rpt.list[idx].op_class = g_rc_chan_map[i].regclass;
				chan_rpt.list[idx].chan = g_rc_chan_map[i].channel[j];
				idx++;
				WBD_DEBUG("%d: Supported opclass [%d] center channel [%d]\n", idx,
					g_rc_chan_map[i].regclass, g_rc_chan_map[i].channel[j]);
				break;
			}
		}
	}
	chan_rpt.n_op_class = idx;
	/* Send operating chan report to 1905 to Controller */
	ieee1905_send_operating_chan_report(&chan_rpt);
end:
	if (chan_rpt.list) {
		free(chan_rpt.list);
	}
	WBD_EXIT();
}

/* set acsd mode to fix chanspec on interfaces of repeaters */
int
wbd_slave_set_acsd_mode_fix_chanspec(void)
{
	i5_dm_device_type *sdev = NULL;
	i5_dm_interface_type *pdmif = NULL;
	wbd_info_t *info = NULL;
	int ret = WBDE_OK;

	WBD_ENTER();

	info = wbd_get_ginfo();

	/* set acs fix chanspec mode for:
	 * - agent not runing on controller
	 * - MultiAP toplogy operating in single channel mode
	 */
	if (!MAP_IS_AGENT(info->map_mode) || MAP_IS_CONTROLLER(info->map_mode) ||
		(WBD_MCHAN_ENAB(info->flags))) {
		WBD_DEBUG(" acsd fix chanspec mode not required, exit \n");
		goto end;
	}

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);

	foreach_i5glist_item(pdmif, i5_dm_interface_type, sdev->interface_list) {
		if (!i5DmIsInterfaceWireless(pdmif->MediaType)) {
			continue;
		}
		if (pdmif->ChanPrefs.rc_count == 0) {
			WBD_INFO("Controller has no preference for ["MACF"]. So don't set acsd"
				" to fix chanspec mode\n", ETHERP_TO_MACF(pdmif->InterfaceId));
			continue;

		}
		/* For Single-channel, Repeaters need to be in Fixed Chanspec mode
		 * For Multi-channel, Repeaters can run ACSD to pick best channel
		 * Exception: even in single channel mode, dedicated backhaul can
		 * operate in Multi-channel if it is not explicitly disabled setting
		 * nvram.
		 */
		if (!WBD_IS_DEDICATED_BACKHAUL(pdmif->mapFlags) ||
			WBD_IS_SCHAN_BH_ENABLED(info->flags)) {

			WBD_DEBUG("set acs mode fix chanspec for radio:["MACF"] \n",
				ETHERP_TO_MACF(pdmif->InterfaceId));

			/* Set mode of ACS running on Repeater to Fixed Chanspec */
			wbd_slave_set_acsd_mode_to_fixchspec(pdmif);
		}
	}

end:
	WBD_EXIT();
	return ret;
}

#if !defined(MULTIAPR2)
int
wbd_slave_send_chan_info(void)
{
	int ret = WBDE_OK;
	i5_dm_device_type *sdev = NULL;
	i5_dm_interface_type *pdmif = NULL;
	i5_dm_bss_type *bss = NULL;
	wbd_ifr_item_t *ifr_vndr_info = NULL;
	wbd_info_t *info = NULL;
	unsigned int max_index = 0;
	bool fronthaul_bss_configured = FALSE;

	WBD_ENTER();

	info = wbd_get_ginfo();

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);

	/* send chan info for 5g interface, configured with atleast 1 bss as fronthaul BSS */
	foreach_i5glist_item(pdmif, i5_dm_interface_type, sdev->interface_list) {
		WBD_DEBUG("interface["MACF"] band[%d] mapflags[%x] \n",
			ETHERP_TO_MACF(pdmif->InterfaceId), pdmif->band, pdmif->mapFlags);

		ifr_vndr_info = (wbd_ifr_item_t*)pdmif->vndr_data;
		if (!ifr_vndr_info) {
			WBD_ERROR(" No valid vendor info for this interface, exit\n");
			continue;
		}

		foreach_i5glist_item(bss, i5_dm_bss_type, pdmif->bss_list) {
			if (I5_IS_BSS_FRONTHAUL(bss->mapFlags)) {
				WBD_DEBUG(" prepare chan info for bss["MACF"] with mapflags[%x] \n",
					ETHERP_TO_MACF(&bss->BSSID), bss->mapFlags);

				fronthaul_bss_configured = TRUE;
				break;
			}
		}

		if (!fronthaul_bss_configured || !WBD_BAND_TYPE_LAN_5G(pdmif->band) ||
			WBD_MCHAN_ENAB(info->flags)) {

			/* free vndr_info->chan info memory if exist */
			if (ifr_vndr_info->chan_info) {
				free(ifr_vndr_info->chan_info);
				ifr_vndr_info->chan_info = NULL;
			}
			continue;
		}

		/* prepare chan info for 5G interface configured with Fronthaul BSS */
		max_index = (sizeof(ifr_vndr_info->chan_info->chinfo))/
			(sizeof(ifr_vndr_info->chan_info->chinfo[1]));

		WBD_INFO("alloc buffer for max_index[%d] chan info size[%zu] block size[%zu]\n",
			max_index, sizeof(ifr_vndr_info->chan_info->chinfo),
			sizeof(ifr_vndr_info->chan_info->chinfo[1]));

		wbd_slave_get_chan_info(pdmif->ifname, ifr_vndr_info->chan_info, max_index);

		wbd_slave_chk_and_send_chan_config_info(pdmif, TRUE);
	}
end:
	WBD_EXIT();
	return ret;
}
#endif /* !MULTIAPR2 */

#if defined(MULTIAPR2)
static void
wbd_slave_send_association_status_notification(i5_dm_bss_type *bss)
{
	ieee1905_association_status_notification assoc_notif;
	int ret = WBDE_OK;

	WBD_ENTER();

	memset(&assoc_notif, 0, sizeof(assoc_notif));
	WBD_ASSERT_ARG(bss, WBDE_INV_ARG);

	memset(&assoc_notif, 0, sizeof(assoc_notif));
	assoc_notif.list = (association_status_notification_bss *)wbd_malloc(
			sizeof(association_status_notification_bss), &ret);
	WBD_ASSERT();

	memcpy(&assoc_notif.list->bssid, &bss->BSSID, sizeof(assoc_notif.list->bssid));
	/* current support is for 1 bssid */
	assoc_notif.count = 1;
	assoc_notif.list->assoc_allowance_status = bss->assoc_allowance_status;
	/* Send 1905 association status notification to Controller */
	ieee1905_send_association_status_notification(&assoc_notif);
end:
	if (assoc_notif.list) {
		free(assoc_notif.list);
	}
	WBD_EXIT();
}
#endif /* MULTIAPR2 */

void
wbd_slave_update_chanspec(i5_dm_interface_type *sifr, chanspec_t chanspec)
{
	int tx_pwr;
	WBD_ENTER();

	if (!WBD_I5LL_NEXT(&sifr->bss_list) || !sifr->isConfigured) {
		goto end;
	}

	blanket_get_txpwr_target_max(sifr->ifname, &tx_pwr);
	if ((chanspec == sifr->chanspec) &&
		(!sifr->TxPowerLimit || sifr->TxPowerLimit == tx_pwr)) {
		WBD_INFO("[%s] Chanspec[0x%x] / TxPwr[%d] not changed. "
			"Skip updating radio chanspec to Controller.\n",
			sifr->ifname, chanspec, tx_pwr);
		goto end;
	}
	WBD_INFO("[%s] Chanspec / TxPwr changed. "
		"Chanspec: new[0x%x] old[0x%x] TxPwr: new[%d] old[%d]\n",
		sifr->ifname, chanspec, sifr->chanspec, tx_pwr, sifr->TxPowerLimit);
#if !defined(MULTIAPR2)
	/* check if agent needs to inform controller about
	 * any recent change in chan info if any.
	 * if yes.. prepare vendor message with new chan info
	 * and list of chanspec and send to controller.
	 */
	/* only fronthaul interface chan info is required */
	if (WBD_BAND_TYPE_LAN_5G(sifr->band)) {
		i5_dm_bss_type *bss = NULL;
		foreach_i5glist_item(bss, i5_dm_bss_type, sifr->bss_list) {
			if (I5_IS_BSS_FRONTHAUL(bss->mapFlags)) {
				WBD_DEBUG("check existing chan info for bss["MACF"]"
					"with mapflags[%x], with new chan info"
					"if different than existing\n",
					ETHERP_TO_MACF(&bss->BSSID), bss->mapFlags);
				wbd_slave_chk_and_send_chan_config_info(sifr, TRUE);
				break;
			}
		}
	}
#endif /* !MULTIAPR2 */
	if (sifr->chanspec != chanspec) {
		if (WBD_BAND_TYPE_LAN_5G(sifr->band)) {
			uint8 channel;

			FOREACH_20_SB(chanspec, channel) {
				uint bitmap_sb = 0x00;

				blanket_get_chan_info(sifr->ifname, channel,
					CHSPEC_BAND(chanspec), &bitmap_sb);
				if ((bitmap_sb & WL_CHAN_RADAR) &&
					(bitmap_sb & WL_CHAN_PASSIVE) &&
					!(bitmap_sb & WL_CHAN_INACTIVE)) {
					WBD_INFO("CSA to Radar channel. Ignore, since it is "
						"handled in CAC_STATE_CHANGE event\n");
					goto end;
				}
			}
		}
		sifr->chanspec = chanspec;
		ieee1905_notify_channel_change(sifr);
		wbd_slave_add_ifr_nbr(sifr, TRUE);

	}
	sifr->TxPowerLimit = (unsigned char)tx_pwr;

	/* If Interface is bSTA and its roaming, do not send OpChannel Report */
	if (I5_IS_BSS_STA(sifr->mapFlags) && (sifr->roam_enabled == TRUE)) {
		WBD_INFO("[%s] bSTA["MACF"] is roaming to Chanspec[0x%x]. "
			"Skip updating radio chanspec to Controller.\n",
			sifr->ifname, ETHERP_TO_MACF(sifr->InterfaceId), chanspec);
		goto end;
	}

	wbd_slave_send_operating_chan_report(sifr);
end:
	WBD_EXIT();
}

/* Send BSS capability report message */
static void
wbd_slave_send_bss_capability_report_cmd(unsigned char *neighbor_al_mac, unsigned char *radio_mac)
{
	int ret = WBDE_OK;
	i5_dm_interface_type *i5_ifr = NULL;
	ieee1905_vendor_data vndr_msg_data;
	WBD_ENTER();

	/* Fill vndr_msg_data struct object to send Vendor Message */
	memset(&vndr_msg_data, 0x00, sizeof(vndr_msg_data));
	memcpy(vndr_msg_data.neighbor_al_mac, neighbor_al_mac, IEEE1905_MAC_ADDR_LEN);

	WBD_SAFE_GET_I5_SELF_IFR(radio_mac, i5_ifr, &ret);

	/* Allocate Dynamic mem for Vendor data from App */
	vndr_msg_data.vendorSpec_msg =
		(unsigned char *)wbd_malloc(IEEE1905_MAX_VNDR_DATA_BUF, &ret);
	WBD_ASSERT();

	/* Update the BSS capability */
	wbd_slave_update_bss_capability(i5_ifr);

	WBD_INFO("Send BSS capability report from Device["MACDBG"] to Device["MACDBG"]\n",
		MAC2STRDBG(ieee1905_get_al_mac()), MAC2STRDBG(vndr_msg_data.neighbor_al_mac));

	/* Encode Vendor Specific TLV for Message : BSS capability report to send */
	ret = wbd_tlv_encode_bss_capability_report((void *)i5_ifr,
		vndr_msg_data.vendorSpec_msg, &vndr_msg_data.vendorSpec_len);
	WBD_ASSERT_MSG("Failed to encode BSS capability Report which needs to be sent "
		"from Device["MACDBG"] to Device["MACDBG"]\n",
		MAC2STRDBG(ieee1905_get_al_mac()), MAC2STRDBG(vndr_msg_data.neighbor_al_mac));

	/* Send Vendor Specific Message : BSS capability report */
	ret = wbd_slave_send_brcm_vndr_msg(&vndr_msg_data);

	WBD_CHECK_ERR_MSG(WBDE_DS_1905_ERR, "Failed to send "
		"BSS capability Report from Device["MACDBG"] to Device["MACDBG"], Error : %s\n",
		MAC2STRDBG(ieee1905_get_al_mac()), MAC2STRDBG(vndr_msg_data.neighbor_al_mac),
		wbderrorstr(ret));

end:

	if (vndr_msg_data.vendorSpec_msg) {
		free(vndr_msg_data.vendorSpec_msg);
	}
	WBD_EXIT();
}

/* Processes BSS capability query message */
static void
wbd_slave_process_bss_capability_query_cmd(unsigned char *neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len)
{
	int ret = WBDE_OK;
	unsigned char radio_mac[ETHER_ADDR_LEN];
	WBD_ENTER();

	memset(radio_mac, 0, sizeof(radio_mac));

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(tlv_data, "BSS Capability Query");

	/* Decode Vendor Specific TLV for Message : BSS capability query on receive */
	ret = wbd_tlv_decode_bss_capability_query((void *)&radio_mac, tlv_data, tlv_data_len);
	if (ret != WBDE_OK) {
		WBD_WARNING("Failed to decode the BSS capability query TLV\n");
		goto end;
	}

	WBD_INFO("BSS capability query Vendor Specific Command : Radio MAC["MACDBG"]\n",
		MAC2STRDBG(radio_mac));

	/* Send BSS capability report */
	wbd_slave_send_bss_capability_report_cmd(neighbor_al_mac, radio_mac);

end:
	WBD_EXIT();
}

/* start/stop DFS_AP_MOVE on reception of zwdfs msg from controller via
 * acsd. skip this process for interface operating in apsta mode
 */
static int
wbd_slave_process_zwdfs_msg(unsigned char* neighbor_al_mac, unsigned char *tlv_data,
	unsigned short tlv_data_len)
{
	int ret = WBDE_OK;
	i5_dm_device_type *self_device = NULL;
	wbd_cmd_vndr_zwdfs_msg_t zwdfs_msg;
	wl_chan_change_reason_t reason;
	i5_dm_interface_type *pdmif = NULL;
	chanspec_t chanspec;
	uint bw = 0;
	wbd_info_t *info = wbd_get_ginfo();

	if (!tlv_data || !tlv_data_len || !neighbor_al_mac || !info)
	{
		WBD_ERROR("Invalid argument passed, exit \n");
		return WBDE_INV_ARG;
	}

	memset(&zwdfs_msg, 0, sizeof(zwdfs_msg));

	WBD_SAFE_GET_I5_SELF_DEVICE(self_device, &ret);

	wbd_tlv_decode_zwdfs_msg((void*)&zwdfs_msg, tlv_data, tlv_data_len);

	WBD_INFO("Decode zwdfs message: reason[%d], cntrl_chan[%d] opclass[%d] "
		"from device:["MACF"] to interface:["MACF"]\n",
		zwdfs_msg.reason, zwdfs_msg.cntrl_chan, zwdfs_msg.opclass,
		ETHERP_TO_MACF(neighbor_al_mac), ETHER_TO_MACF(zwdfs_msg.mac));

	pdmif = wbd_ds_get_i5_interface((uchar*)self_device->DeviceId,
			(uchar*)&zwdfs_msg.mac, &ret);
	WBD_ASSERT();

	reason = zwdfs_msg.reason;

	if (I5_IS_BSS_STA(pdmif->mapFlags)) {
		WBD_DEBUG("Interface ["MACF"] is STA, no channel switch in sta mode\n",
			ETHER_TO_MACF(zwdfs_msg.mac));
		/* Firmware is synchronizing channel switch across APSTA
		 * repeater and Root AP
		 */
		goto end;
	}

	blanket_get_bw_from_rc(zwdfs_msg.opclass, &bw);

	chanspec = wf_channel2chspec(zwdfs_msg.cntrl_chan, bw,
			blanket_opclass_to_band(zwdfs_msg.opclass));
	/* Update the chanspec through ACSD */
	wbd_slave_set_chanspec_through_acsd(chanspec, reason, pdmif);

end:
	(void)(pdmif);
	return ret;
}

/* Send 1905 Vendor Specific AP_CHAN_CHANGE from Agent to Controller */
static int
wbd_slave_send_vendor_msg_zwdfs(wbd_cmd_vndr_zwdfs_msg_t *msg)
{
	int ret = WBDE_OK;
	ieee1905_vendor_data vndr_msg_data;
	i5_dm_device_type *controller_device = NULL;

	WBD_ENTER();

	memset(&vndr_msg_data, 0, sizeof(vndr_msg_data));

	WBD_ASSERT_ARG(msg, WBDE_INV_ARG);
	/* Allocate Dynamic mem for Vendor data from App */
	vndr_msg_data.vendorSpec_msg =
		(unsigned char *)wbd_malloc((sizeof(*msg) + sizeof(i5_tlv_t)), &ret);
	WBD_ASSERT();

	WBD_SAFE_GET_I5_CTLR_DEVICE(controller_device, &ret);

	/* Fill Destination AL_MAC in Vendor data */
	memcpy(vndr_msg_data.neighbor_al_mac,
		controller_device->DeviceId, IEEE1905_MAC_ADDR_LEN);

	WBD_INFO("Send vendor msg ZWDFS with reason[%d] from Device["MACDBG"] to"
		" Device["MACDBG"] for  src interface["MACF"] cntrl_chan[%d],"
		" opclass[%d]\n",
		msg->reason, MAC2STRDBG(ieee1905_get_al_mac()),
		MAC2STRDBG(controller_device->DeviceId), ETHERP_TO_MACF(&msg->mac),
		msg->cntrl_chan, msg->opclass);

	wbd_tlv_encode_zwdfs_msg((void*)msg, vndr_msg_data.vendorSpec_msg,
		&vndr_msg_data.vendorSpec_len);

	WBD_DEBUG("send zwdfs message vendor data len[%d]\n", vndr_msg_data.vendorSpec_len);

	/* Send Vendor Specific Message */
	wbd_slave_send_brcm_vndr_msg(&vndr_msg_data);

	WBD_CHECK_ERR_MSG(WBDE_DS_1905_ERR, "Failed to send "
		"AP_CHAN_CHANGE msg from Device["MACDBG"], Error : %s\n",
		MAC2STRDBG(ieee1905_get_al_mac()), wbderrorstr(ret));

end:
	if (vndr_msg_data.vendorSpec_msg) {
		free(vndr_msg_data.vendorSpec_msg);
	}
	WBD_EXIT();
	return ret;
}

static int
wbd_slave_process_vndr_set_chan_cmd(unsigned char* neighbor_al_mac, unsigned char *tlv_data,
	unsigned short tlv_data_len)
{
	int ret = WBDE_OK;
	i5_dm_interface_type *pdmif = NULL, *i5_upstream_ifr;
	i5_dm_device_type *controller_device;
	wbd_cmd_vndr_set_chan_t *msg;
	chanspec_t chanspec;
	uint bw = 0;
	wbd_info_t *info = wbd_get_ginfo();
	i5_dm_bss_type *pbss;

	if (!tlv_data || !tlv_data_len || !neighbor_al_mac || !info)
	{
		WBD_ERROR("Invalid argument passed, exit \n");
		ret = WBDE_INV_ARG;
		goto end;
	}

	WBD_SAFE_GET_I5_CTLR_DEVICE(controller_device, &ret);

	tlv_data += sizeof(i5_tlv_t);
	msg = (wbd_cmd_vndr_set_chan_t *)tlv_data;
	WBD_INFO("Decode message: cntrl_chan[%d] opclass[%d] "
		"from device ["MACF"] to interface:["MACF"] \n",
		msg->cntrl_chan, msg->rclass,
		ETHERP_TO_MACF(neighbor_al_mac), ETHERP_TO_MACF(&msg->mac));

	WBD_SAFE_GET_I5_SELF_IFR((uchar*)&msg->mac, pdmif, &ret);
	pdmif->chan_sel_resp_code = I5_CHAN_SEL_RESP_CODE_ACCEPT;
	pbss = (i5_dm_bss_type *)pdmif->bss_list.ll.next;
	if (!pbss) {
		WBD_INFO("No bss on interface ["MACF"]. Skip setting channel\n",
			ETHERP_TO_MACF(pdmif->InterfaceId));
		goto end;
	}

	/* If upstream connection is via Wi-Fi through this Interface :
	 * No need to Change Chanspec, Send Channel Selection Response with code = DECLINE_3
	 * Don't Send Operating Channel Report
	 */
	i5_upstream_ifr = i5DmInterfaceFind(i5DmGetSelfDevice(),
		((i5_socket_type*)controller_device->psock)->u.sll.mac_address);
	if (i5_upstream_ifr && (eacmp(i5_upstream_ifr->InterfaceId, pdmif->InterfaceId) == 0)) {
		/* No need to set same chanspec again */
		WBD_INFO("No need to set channel on Active Wi-Fi Backhaul Interface["MACF"]. "
			"Skip setting channel\n", ETHERP_TO_MACF(pdmif->InterfaceId));
		pdmif->chan_sel_resp_code = I5_CHAN_SEL_RESP_CODE_DECLINE_3;
		goto end;
	}
	blanket_get_bw_from_rc(msg->rclass, &bw);

	chanspec = wf_channel2chspec(msg->cntrl_chan, bw, blanket_opclass_to_band(msg->rclass));

	WBD_INFO("IFR["MACF"] Local Chanspec[0x%X] CSA Chanspec[0x%X]\n",
			ETHERP_TO_MACF(pdmif->InterfaceId), pdmif->chanspec, chanspec);

	/* If upstream connection is Other than Wi-Fi, and Chanspec is same :
	 * No need to Change Chanspec, Send Channel Selection Response with code = ACCEPT
	 * Send Operating Channel Report
	 */
	if (pdmif->chanspec == chanspec) {
		/* No need to set same chanspec again */
		WBD_INFO("Don't set same chanspec[0x%X] again on interface["MACF"]. "
			"Skip setting channel\n", chanspec, ETHERP_TO_MACF(pdmif->InterfaceId));
		goto end;
	}

	WBD_INFO(" passed chanspec to acsd [%x] \n", chanspec);
	/* Update the chanspec through ACSD */
	wbd_slave_set_chanspec_through_acsd(chanspec, WL_CHAN_REASON_CSA, pdmif);
	/* set chanspec to pdmif */
	pdmif->chanspec = chanspec;
end:
	(void)(pdmif);
	return ret;
}

/* Processes backhaul STA metric policy vendor message */
static void
wbd_slave_process_backhaul_sta_metric_policy_cmd(wbd_info_t *info, unsigned char *neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len)
{
	int ret = WBDE_OK;
	uint32 rc_restart = 0;
	wbd_weak_sta_policy_t metric_policy;
	char new_val[WBD_MAX_BUF_128];
	WBD_ENTER();

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(tlv_data, "Backhaul STA Metric Policy");

	memset(&metric_policy, 0x00, sizeof(metric_policy));

	/* Decode Vendor Specific TLV for Message : Backhaul STA Vendor Metric Reporting Policy
	 * Config on receive
	 */
	ret = wbd_tlv_decode_backhaul_sta_metric_report_policy((void*)&metric_policy, tlv_data,
		tlv_data_len);
	WBD_ASSERT_MSG("Failed to decode the Backhaul STA Metric Policy TLV From "
		"Device["MACDBG"]\n", MAC2STRDBG(neighbor_al_mac));

	/* Prepare New Value of "wbd_weak_sta_cfg_bh" from Policy Received from Controller */
	snprintf(new_val, sizeof(new_val), "%d %d %d %d %d 0x%04x",
		metric_policy.t_idle_rate, metric_policy.t_rssi, metric_policy.t_hysterisis,
		metric_policy.t_tx_rate, metric_policy.t_tx_failures, metric_policy.flags);

	/* Match NVRAM and NEW value, and if mismatch, Set new value in NVRAM */
	rc_restart |= blanket_nvram_prefix_match_set(NULL, WBD_NVRAM_WEAK_STA_CFG_BH,
		new_val, FALSE);

	WBD_INFO("Backhaul STA Metric Policy from device["MACDBG"] t_rssi[%d] t_hysterisis[%d] "
		"t_idle_rate[%d] t_tx_rate[%d] t_tx_failures[%d] flags[0x%x] NVRAM[%s=%s]\n",
		MAC2STRDBG(neighbor_al_mac), metric_policy.t_rssi, metric_policy.t_hysterisis,
		metric_policy.t_idle_rate, metric_policy.t_tx_rate, metric_policy.t_tx_failures,
		metric_policy.flags, WBD_NVRAM_WEAK_STA_CFG_BH, new_val);

	WBD_INFO("Backhaul Weak STA Config %s Changed.%s\n", rc_restart ? "" : "NOT",
		rc_restart ? "RC Restart...!!" : "");

	/* If required, Execute nvram commit/rc restart/reboot commands */
	if (rc_restart) {
		WBD_INFO("Creating rc restart timer\n");
		wbd_slave_create_rc_restart_timer(info);
		info->flags |= WBD_INFO_FLAGS_RC_RESTART;
	}
end:
	WBD_EXIT();
}

/* Get prefix for radio MAC and SSID. The ifname can be primary or virtual. If the primary prefix
 * of WBD ifname matches with the prefix of radio MAC, then return the prefix of that WBD ifname
 */
static int
wbd_slave_get_prefix_from_radio_mac_and_ssid(unsigned char *mac, ieee1905_ssid_type *ssid,
	char *prefix, int prefix_len)
{
	int ret = -1;
	i5_dm_interface_type *i5_ifr;
	char name[NVRAM_MAX_VALUE_LEN], *wbd_ifnames, *next = NULL;
	char ifr_prefix[NVRAM_MAX_PARAM_LEN+2];
	WBD_ENTER();

	i5_ifr = i5DmInterfaceFind(i5DmGetSelfDevice(), mac);
	if (i5_ifr == NULL) {
		WBD_INFO("IFR["MACDBG"] Not Found\n", MAC2STRDBG(mac));
		goto end;
	}

	snprintf(ifr_prefix, sizeof(ifr_prefix), "%s_", i5_ifr->wlParentName);

	/* Read BSS info names */
	wbd_ifnames = blanket_nvram_safe_get(WBD_NVRAM_IFNAMES);
	if (strlen(wbd_ifnames) <= 0) {
		WBD_WARNING("NVRAM[%s] Not set. Cannot read WBD ifnames\n", WBD_NVRAM_IFNAMES);
		goto end;
	}

	/* For each WBD ifnames */
	foreach(name, wbd_ifnames, next) {

		char wbd_prefix[NVRAM_MAX_PARAM_LEN+2], radio_prefix[NVRAM_MAX_PARAM_LEN+2], *nvval;

		/* Get prefix of the interface from Driver */
		blanket_get_interface_prefix(name, wbd_prefix, sizeof(wbd_prefix));
		/* Get the SSID and compare. This is to use the same SSID for a radio */
		nvval = blanket_nvram_prefix_safe_get(wbd_prefix, NVRAM_SSID);
		if (strlen(nvval) != ssid->SSID_len ||
			memcmp(nvval, ssid->SSID, ssid->SSID_len)) {
			continue;
		}

		/* If the prefix of interface and primary prefix of WBD ifname matches then
		 * return that prefix.
		 */
		blanket_get_radio_prefix(name, radio_prefix, sizeof(radio_prefix));
		if (strcmp(ifr_prefix, radio_prefix) == 0) {
			snprintf(prefix, prefix_len, "%s", wbd_prefix);
			ret = WBDE_OK;
			goto end;
		}
	}

end:
	WBD_EXIT();
	return ret;
}

/* Processes NVRAM set vendor message */
static void
wbd_slave_process_vndr_nvram_set_cmd(wbd_info_t *info, unsigned char *neighbor_al_mac,
	unsigned char *tlv_data, unsigned short tlv_data_len)
{
	int ret = WBDE_OK, idx, k;
	uint32 rc_restart = 0;
	wbd_cmd_vndr_nvram_set_t cmd;
	i5_dm_interface_type *i5_ifr;
	i5_dm_bss_type *i5_bss;
	char prefix[NVRAM_MAX_PARAM_LEN + 2] = {0};
	WBD_ENTER();

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(tlv_data, "NVRAM Set Vendor Message");

	memset(&cmd, 0x00, sizeof(cmd));

	/* Decode Vendor Specific TLV for Message : NVRAM set on receive */
	ret = wbd_tlv_decode_nvram_set((void*)&cmd, tlv_data,
		tlv_data_len);
	WBD_ASSERT_MSG("Failed to decode the NVRAM Set TLV From Device["MACDBG"]\n",
		MAC2STRDBG(neighbor_al_mac));

	/* Set all the common NVRAMs (Without Prefix) */
	for (idx = 0; idx < cmd.n_common_nvrams; idx++) {
		/* Match NVRAM and NEW value, and if mismatch, Set new value in NVRAM */
		if (strlen(cmd.common_nvrams[idx].name) > 0 &&
			strlen(cmd.common_nvrams[idx].value) > 0) {
			rc_restart |= blanket_nvram_prefix_match_set(NULL,
				cmd.common_nvrams[idx].name,
				cmd.common_nvrams[idx].value, FALSE);
			WBD_DEBUG("NVRAM Set from device["MACDBG"] NVRAM[%s=%s] rc_restart=%d\n",
				MAC2STRDBG(neighbor_al_mac), cmd.common_nvrams[idx].name,
				cmd.common_nvrams[idx].value, rc_restart);
		}
	}

	/* Set all the Radio NVRAMs (With Primary Prefix) */
	for (idx = 0; idx < cmd.n_radios; idx++) {
		i5_ifr = wbd_ds_get_self_i5_interface(cmd.radio_nvrams[idx].mac, &ret);
		if (i5_ifr == NULL) {
			WBD_INFO("For["MACDBG"]. %s\n", MAC2STRDBG(cmd.radio_nvrams[idx].mac),
				wbderrorstr(ret));
			continue;
		}

		/* Prefix for this Radio */
		snprintf(prefix, sizeof(prefix), "%s_", i5_ifr->wlParentName);

		for (k = 0; k < cmd.radio_nvrams[idx].n_nvrams; k++) {
			/* Match NVRAM and NEW value, and if mismatch, Set new value in NVRAM */
			if (strlen(cmd.radio_nvrams[idx].nvrams[k].name) > 0 &&
				strlen(cmd.radio_nvrams[idx].nvrams[k].value) > 0) {
				rc_restart |= blanket_nvram_prefix_match_set(prefix,
					cmd.radio_nvrams[idx].nvrams[k].name,
					cmd.radio_nvrams[idx].nvrams[k].value, FALSE);
				WBD_DEBUG("Radio NVRAM Set from device["MACDBG"] NVRAM[%s%s=%s] "
					"rc_restart=%d\n",
					MAC2STRDBG(neighbor_al_mac), prefix,
					cmd.radio_nvrams[idx].nvrams[k].name,
					cmd.radio_nvrams[idx].nvrams[k].value, rc_restart);
			}
		}
	}

	/* Set all the BSS NVRAMs (With MBSS Prefix) */
	for (idx = 0; idx < cmd.n_bsss; idx++) {
		i5_bss = wbd_ds_get_self_i5_bss(cmd.bss_nvrams[idx].mac, &ret);
		if (i5_bss == NULL) {
			WBD_INFO("For["MACDBG"]. %s\n", MAC2STRDBG(cmd.bss_nvrams[idx].mac),
				wbderrorstr(ret));
			continue;
		}

		/* Get Prefix for this BSS */
		blanket_get_interface_prefix(i5_bss->ifname, prefix, sizeof(prefix));

		for (k = 0; k < cmd.bss_nvrams[idx].n_nvrams; k++) {
			/* Match NVRAM and NEW value, and if mismatch, Set new value in NVRAM */
			if (strlen(cmd.bss_nvrams[idx].nvrams[k].name) > 0 &&
				strlen(cmd.bss_nvrams[idx].nvrams[k].value) > 0) {
				rc_restart |= blanket_nvram_prefix_match_set(prefix,
					cmd.bss_nvrams[idx].nvrams[k].name,
					cmd.bss_nvrams[idx].nvrams[k].value, FALSE);
				WBD_DEBUG("BSS NVRAM Set from device["MACDBG"] NVRAM[%s%s=%s] "
					"rc_restart=%d\n",
					MAC2STRDBG(neighbor_al_mac), prefix,
					cmd.bss_nvrams[idx].nvrams[k].name,
					cmd.bss_nvrams[idx].nvrams[k].value, rc_restart);
			}
		}
	}

	/* Set all the SSID specific NVRAMs */
	for (idx = 0; idx < cmd.n_ssid; idx++) {

		/* Get the prefix for which the NVRAMs to be set */
		if (wbd_slave_get_prefix_from_radio_mac_and_ssid(cmd.mac,
			&cmd.ssid_nvrams[idx].ssid, prefix, sizeof(prefix)) != WBDE_OK) {
			WBD_INFO("For["MACDBG"]. SSID %s prefix not found\n",
				MAC2STRDBG(cmd.mac), cmd.ssid_nvrams[idx].ssid.SSID);
			continue;
		}

		for (k = 0; k < cmd.ssid_nvrams[idx].n_nvrams; k++) {
			/* Match NVRAM and NEW value, and if mismatch, Set new value in NVRAM */
			if (strlen(cmd.ssid_nvrams[idx].nvrams[k].name) > 0 &&
				strlen(cmd.ssid_nvrams[idx].nvrams[k].value) > 0) {
				rc_restart |= blanket_nvram_prefix_match_set(prefix,
					cmd.ssid_nvrams[idx].nvrams[k].name,
					cmd.ssid_nvrams[idx].nvrams[k].value, FALSE);
				WBD_DEBUG("SSID NVRAM Set from device["MACDBG"] SSID[%s] "
					"RADIO["MACDBG"] NVRAM[%s%s=%s] rc_restart=%d\n",
					MAC2STRDBG(neighbor_al_mac), cmd.ssid_nvrams[idx].ssid.SSID,
					MAC2STRDBG(cmd.mac), prefix,
					cmd.ssid_nvrams[idx].nvrams[k].name,
					cmd.ssid_nvrams[idx].nvrams[k].value, rc_restart);
			}
		}
	}

	WBD_INFO("NVRAM Set %s Changed.%s\n", rc_restart ? "" : "NOT",
		rc_restart ? "RC Restart...!!" : "");

	/* If required, Execute nvram commit/rc restart/reboot commands */
	if (rc_restart) {
		WBD_INFO("Creating rc restart timer\n");
		wbd_slave_create_rc_restart_timer(info);
		info->flags |= WBD_INFO_FLAGS_RC_RESTART;
	}
end:
	wbd_free_nvram_sets(&cmd);
	WBD_EXIT();
}

#if defined(MULTIAPR2)
/* Processes Channel Scan Request by Agent */
void
wbd_slave_process_channel_scan_req_cb(unsigned char *src_al_mac,
	ieee1905_chscan_req_msg *chscan_req)
{
	int ret = WBDE_OK, idx_chan = 0;
	ieee1905_per_radio_opclass_list *ifr = NULL;
	i5_dm_device_type *self_device = NULL;
	i5_dm_interface_type *i5_ifr = NULL;
	WBD_ENTER();

	/* Validate Message data */
	WBD_ASSERT_MSGDATA(chscan_req, "Channel Scan Request");

	WBD_SAFE_GET_I5_SELF_DEVICE(self_device, &ret);

	/* Extract Channel Scan Request TLV Flags */
	WBD_DEBUG("Flags = [0x%X] ", chscan_req->chscan_req_msg_flag);

	/* Extract Number of radios : upon which channel scans are requested */
	WBD_DEBUG("Num of Radios = [%d] \n", chscan_req->num_of_radios);

	/* Extract details for each radio */
	foreach_iglist_item(ifr, ieee1905_per_radio_opclass_list, chscan_req->radio_list) {

		ieee1905_per_opclass_chan_list *opcls = NULL;

		/* Find IFR in Self Device, Find Vendor IFR Data */
		i5_ifr = i5DmInterfaceFind(self_device, ifr->radio_mac);
		if (i5_ifr == NULL) {
			WBD_WARNING("IFR["MACF"] Not Found\n", ETHERP_TO_MACF(ifr->radio_mac));
			continue;
		}

		/* Extract Radio Unique Identifier of a radio of the Multi-AP Agent */
		WBD_DEBUG("IFR["MACF"] ", ETHERP_TO_MACF(ifr->radio_mac));

		/* Extract Number of Operating Classes */
		WBD_DEBUG("Num of OpClass = [%d] \n", ifr->num_of_opclass);

		/* Extract details for each Operating Class */
		foreach_iglist_item(opcls, ieee1905_per_opclass_chan_list, ifr->opclass_list) {

			/* Extract Operating Class Value */
			WBD_DEBUG("OpClass = [%d] ", opcls->opclass_val);

			/* Extract Number of Channels specified in the Channel List */
			WBD_DEBUG("Num of Channels = [%d] \n", opcls->num_of_channels);

			/* Extract Channel List */
			for (idx_chan = 0; idx_chan < opcls->num_of_channels; idx_chan++) {
				WBD_DEBUG("Channel = [%d] \n", opcls->chan_list[idx_chan]);
				WBD_DEBUG("Channel Supported = [%d] \n",
					opcls->supported_chan_list[idx_chan]);
			}
		}
	}

	/* Start Requested Channel Scan on Agent, requested by Controller */
	wbd_slave_process_requested_channel_scan(src_al_mac, chscan_req);

end:
	WBD_EXIT();
}
#endif /* MULTIAPR2 */

/* Store the BSSID in the NVRAM . If the force is set, then store it directly. Else store only
 * if the NVRAM is not defined
 */
void
wbd_slave_store_bssid_nvram(char *prefix, unsigned char *bssid, int force)
{
	char *nvval;
	char str_mac[WBD_STR_MAC_LEN + 1] = {0};

	wbd_ether_etoa(bssid, str_mac);

	/* force is 1, set directly */
	if (force) {
		blanket_nvram_prefix_set(prefix, NVRAM_BSSID, str_mac);
		nvram_commit();
	} else {
		/* Set only if the NVRAM is not set or has default mac addr */
		nvval = blanket_nvram_prefix_safe_get(prefix, NVRAM_BSSID);
		if (!nvval || (strlen(nvval) < WBD_STR_MAC_LEN) ||
			!strcmp(nvval, WBD_DEF_BHSTA_BSSID)) {
			blanket_nvram_prefix_set(prefix, NVRAM_BSSID, str_mac);
			nvram_commit();
		}
	}
}

/* Use controller's chan info and local chanspecs list to prepare dfs_channel_forced
 * list and pass to firmware via "dfs_channel_forced" iovar
 */
static void
wbd_slave_process_dfs_chan_info(unsigned char *src_al_mac, unsigned char *tlv_data,
	unsigned short tlv_data_len)
{
	i5_dm_interface_type *pdmif = NULL;
	wbd_cmd_vndr_controller_dfs_chan_info_t chan_info_msg;
	wl_dfs_forced_t *dfs_frcd = NULL;
	int ret = WBDE_OK;
	uint32 ioctl_size = 0;

	WBD_ENTER();
	memset(&chan_info_msg, 0, sizeof(chan_info_msg));

	ret = wbd_tlv_decode_dfs_chan_info((void*)&chan_info_msg, tlv_data, tlv_data_len);
	WBD_ASSERT();
	if (chan_info_msg.chan_info == NULL) {
		WBD_INFO("No DFS channels for mac["MACF"] band[%d]",
			ETHERP_TO_MACF(&chan_info_msg.mac), chan_info_msg.band);
		goto end;
	}

	WBD_DEBUG("dfs chan msg : mac["MACF"] band[%d] chan_info count[%d]\n",
		ETHERP_TO_MACF(&chan_info_msg.mac), chan_info_msg.band,
		chan_info_msg.chan_info->count);

	WBD_SAFE_GET_I5_SELF_IFR((uchar*)&chan_info_msg.mac, pdmif, &ret);
	/* prepare the dfs list */
	dfs_frcd = (wl_dfs_forced_t*)wbd_malloc(WBD_MAX_BUF_512, &ret);

	/* Presume: chan info from master is different than agent's curret chan info
	 * and proceed to prepare dfs list with provided chan info
	 */
	wbd_slave_prepare_dfs_list(chan_info_msg.chan_info, pdmif, dfs_frcd);

	ioctl_size = WL_DFS_FORCED_PARAMS_FIXED_SIZE +
		(dfs_frcd->chspec_list.num * sizeof(chanspec_t));

	dfs_frcd->version = DFS_PREFCHANLIST_VER;
	/* set dfs force list */
	blanket_set_dfs_forced_chspec(pdmif->ifname, dfs_frcd, ioctl_size);
end:
	if (dfs_frcd) {
		free(dfs_frcd);
	}
	if (chan_info_msg.chan_info) {
		free(chan_info_msg.chan_info);
	}
	WBD_EXIT();
}

#if !defined(MULTIAPR2)
/* Get list of channels  in 20 Mhz BW by issuing chan info iovar
 * and compare with slave's local data base if present. Inform
 * controller via vendor message with TLV: CHAN_INFO and chan info
 * payload.
 */
static void
wbd_slave_chk_and_send_chan_config_info(i5_dm_interface_type *intf, bool send_explictly)
{
	ieee1905_vendor_data vndr_msg;
	i5_dm_device_type *controller_device = NULL;
	wbd_ifr_item_t *ifr_vndr_info = NULL;
	wbd_interface_chan_info_t *wl_chan_info = NULL;
	wbd_interface_chan_info_t *chan_info = NULL;
	wbd_cmd_vndr_intf_chan_info_t *chan_info_msg = NULL;
	wbd_info_t *info = NULL;
	char *ifname = NULL;
	int len = WBD_MAX_BUF_512;
	unsigned int max_index = 0;
	int ret = WBDE_OK;

	WBD_ENTER();

	/* Validate arg */
	WBD_ASSERT_ARG(intf, WBDE_INV_ARG);
	memset(&vndr_msg, 0, sizeof(vndr_msg));

	info = wbd_get_ginfo();

	ifr_vndr_info = (wbd_ifr_item_t *)intf->vndr_data;

	/* perform chan_info for 5G BAND ONLY and single chan case */
	if (!WBD_BAND_TYPE_LAN_5G(intf->band) ||
		(WBD_MCHAN_ENAB(info->flags))) {
		goto end;
	}

	WBD_SAFE_GET_I5_CTLR_DEVICE(controller_device, &ret);

	if (send_explictly) {
		/* explicit request at init time to let controller
		 * know of Fronthaul 5g interface chan info.
		 */
		goto xmit_chan_info;
	}
	/* WBD logic got to know of channel change or radar event,
	 * get chan info from firmware. Compare with existing chan info
	 * and send new chan info if required.
	 */
	ifname = intf->ifname;
	wl_chan_info = (wbd_interface_chan_info_t*)wbd_malloc(len, &ret);
	WBD_ASSERT();

	max_index = (sizeof(ifr_vndr_info->chan_info->chinfo))/
		(sizeof(ifr_vndr_info->chan_info->chinfo[1]));

	ret = wbd_slave_get_chan_info(ifname, wl_chan_info, max_index);

	WBD_INFO("Band[%d] Slave["MACF"] cur chinfo count[%d] prv chinfo count [%d] compare\n",
		intf->band, ETHERP_TO_MACF(intf->InterfaceId), wl_chan_info->count,
		ifr_vndr_info->chan_info->count);

	if (!memcmp((uchar*)wl_chan_info, (uchar*)ifr_vndr_info->chan_info,
		wl_chan_info->count * (sizeof(wbd_chan_info_t)))) {
		/* no need to send chan info, as no change in chan info
		 * information
		 */
		WBD_DEBUG("NO change in chan info, no need to send chan info\n");
		goto end;
	}

xmit_chan_info:
	chan_info = send_explictly ? ifr_vndr_info->chan_info : wl_chan_info;
	/* chan info is different, prepare vendor message to send chan info to
	 * controller
	 */
	vndr_msg.vendorSpec_msg =
		(unsigned char *)wbd_malloc(IEEE1905_MAX_VNDR_DATA_BUF, &ret);
	WBD_ASSERT();

	/* Fill Destination AL_MAC in Vendor data */
	memcpy(vndr_msg.neighbor_al_mac,
		controller_device->DeviceId, IEEE1905_MAC_ADDR_LEN);

	WBD_DEBUG("new chan info, update controller for chan info and chanspec list \n");

	chan_info_msg = (wbd_cmd_vndr_intf_chan_info_t *)wbd_malloc(
		sizeof(wbd_cmd_vndr_intf_chan_info_t), &ret);

	WBD_ASSERT();

	memcpy(&chan_info_msg->mac, &intf->InterfaceId, ETHER_ADDR_LEN);
	chan_info_msg->band = intf->band;
	chan_info_msg->chan_info = chan_info;
	WBD_DEBUG("send msg to MAC["MACF"] band[%d]\n",	ETHERP_TO_MACF(&chan_info_msg->mac),
		chan_info_msg->band);

	wbd_tlv_encode_chan_info((void*)chan_info_msg, vndr_msg.vendorSpec_msg,
		&vndr_msg.vendorSpec_len);

	WBD_DEBUG("CHAN_INFO vendor msg len[%d] MACF["MACF"] band[%d]\n",
		vndr_msg.vendorSpec_len, ETHERP_TO_MACF(&chan_info_msg->mac),
		chan_info_msg->band);

	/* Send Vendor Specific Message */
	wbd_slave_send_brcm_vndr_msg(&vndr_msg);
end:
	if (send_explictly) {
		if (wl_chan_info) {
			free(wl_chan_info);
		}
	} else {
		/* remove earlier interface chan_info and store latest one */
		free(ifr_vndr_info->chan_info);
		ifr_vndr_info->chan_info = wl_chan_info;
	}
	if (vndr_msg.vendorSpec_msg) {
		free(vndr_msg.vendorSpec_msg);
	}
	if (chan_info_msg) {
		free(chan_info_msg);
	}
	WBD_EXIT();
}
#endif /* !MULTIAPR2 */

/* Create dfs_pref_chan_list */
static void
wbd_slave_prepare_dfs_list(wbd_dfs_chan_info_t *chan_info, i5_dm_interface_type *pdmif,
	wl_dfs_forced_t* dfs_frcd)
{
	wl_uint32_list_t *chanspec_list = NULL;
	int i = 0, k = 0;
	uint8 sub_channel;
	chanspec_t chspec;
	bool use_channel;
	uint16 buflen = 0;
	int ret = WBDE_OK;

	WBD_ENTER();

	dfs_frcd->chspec_list.num = 0;

	/* get list of chanspec, free after use */
	buflen = (MAXCHANNEL + 1) * sizeof(uint32);

	chanspec_list =  (wl_uint32_list_t *)wbd_malloc(buflen, &ret);
	WBD_ASSERT_MSG(" Failed to allocate memory for chanspecs\n");

	blanket_get_chanspecs_list(pdmif->ifname, chanspec_list, buflen);

	WBD_INFO("Prepare dfs chan list for interface ["MACF"]. Current chanspec[0x%x] "
		"available chanspecs [%d] channels in dfs chan info [%d]\n",
		ETHERP_TO_MACF(pdmif->InterfaceId), pdmif->chanspec, chanspec_list->count,
		chan_info->count);

	/* given up to 80/160 MHZ chanspecs; checks if it is a DFS channel but pre-cleared to use */
	for (i = (chanspec_list->count -1); i >= 0; i--) {
		/* Reverese traversal */
		chspec = (unsigned short)chanspec_list->element[i];
		use_channel = TRUE;

		FOREACH_20_SB(chspec, sub_channel) {
			/* skip chspec if sub_channel does not belong to
			 * common dfs chan info received from controller
			 */
			if (!wbd_check_chan_good_candidate(sub_channel, chan_info)) {
				WBD_DEBUG("sub_channel[%d] not good. ignore for dfs list\n",
					sub_channel);
				use_channel = FALSE;
				break;
			}
		}
		WBD_DEBUG("%d: %s chanspec[0x%x]\n", i, use_channel ? "USE" : "SKIP", chspec);
		/* when all subchannels are in good condition this chanspec is a good candidate */
		if (use_channel) {
			dfs_frcd->chspec_list.list[k++] = chspec;
			dfs_frcd->chspec_list.num++;
			WBD_DEBUG("[%d]: chanspec[0x%x]\n", k, chspec);
		}
	}

	WBD_DEBUG("dfs chan list entries[%d] \n", dfs_frcd->chspec_list.num);
end:
	if (chanspec_list) {
		free(chanspec_list);
	}
	WBD_EXIT();
}

/* Waiting for maximum delay in acsd to set chanspec,
 * before slave sending operating channel report.
 */
void
wbd_slave_wait_set_chanspec(i5_dm_interface_type *interface, chanspec_t chspec)
{
	int delay = 0;
	chanspec_t chanspec;

	while (delay <= WBD_MAX_ACSD_SET_CHSPEC_DELAY) {
		chanspec = 0;
		blanket_get_chanspec(interface->ifname, &chanspec);
		if (chanspec != chspec) {
			usleep(WBD_GET_CHSPEC_GAP * 1000);
			delay += WBD_GET_CHSPEC_GAP;
			continue;
		}
		break;
	}
}

/* Remove and deauth sta entry from assoclist */
void
wbd_slave_remove_and_deauth_sta(char *ifname, unsigned char *parent_bssid,
	unsigned char *sta_mac)
{
	int ret = WBDE_OK;

	/* Remove a STA item from a Slave's Assoc STA List */
	ret = blanket_sta_assoc_disassoc((struct ether_addr *)parent_bssid,
		(struct ether_addr *)sta_mac, 0, 0, 1, NULL, 0, 0);
	if (ret == WBDE_OK) {
		/* Update Driver to remove stale client entry,
		 *  at present reason is fixed i.e.
		 *  DOT11_RC_STALE_DETECTION, it can be generalized
		 *  if required.
		 */
		ret = blanket_deauth_sta(ifname, (struct ether_addr *)sta_mac,
			DOT11_RC_STALE_DETECTION);
	} else {
		WBD_WARNING("BSS["MACDBG"] STA["MACDBG"]. "
			"Failed to remove from assoclist. Error : %s\n",
			MAC2STRDBG(parent_bssid), MAC2STRDBG(sta_mac), wbderrorstr(ret));
	}
}

#if defined(MULTIAPR2)
/* send tunnel message to controller */
static void
wbd_slave_send_tunneled_msg(ieee1905_tunnel_msg_t *tunnel_msg)
{
	WBD_ENTER();

	WBD_INFO("tunnel msg for sta["MACF"] with tunnel msg type[%d] \n",
		ETHERP_TO_MACF(tunnel_msg->source_mac), tunnel_msg->payload_type);

	ieee1905_send_tunneled_msg(tunnel_msg);
	WBD_EXIT();
}

/* Remove traffic separation policy NVRAMs */
static int
wbd_slave_remove_traffic_separation_policy()
{
	int ret = -1, tmpret;
	char *nvval, *next = NULL;
	char strvlan[WBD_MAX_BUF_16], nvname[NVRAM_MAX_PARAM_LEN];
	char var_intf[IFNAMSIZ] = {0};
	char prefix[IFNAMSIZ], strmap[WBD_MAX_BUF_16];
	unsigned char map_flags = 0;
	WBD_ENTER();

	nvval = blanket_nvram_safe_get(NVRAM_MAP_TS_VLANS);
	WBD_INFO("[%s]=[%s]\n", NVRAM_MAP_TS_VLANS, nvval);

	foreach(strvlan, nvval, next) {
		snprintf(nvname, sizeof(nvname), "map_ts_%s_ssid", strvlan);
		if (strlen(blanket_nvram_safe_get(nvname)) > 0) {
			blanket_nvram_prefix_unset(NULL, nvname);
			WBD_INFO("Unset NVRAM %s\n", nvname);
			ret = 0;
		}
	}

	if (strlen(nvval) > 0) {
		blanket_nvram_prefix_unset(NULL, NVRAM_MAP_TS_VLANS);
		WBD_INFO("Unset NVRAM %s\n", NVRAM_MAP_TS_VLANS);
		ret = 0;
	}

	if (strlen(blanket_nvram_safe_get(NVRAM_MAP_TS_PRIM_VLAN_ID)) > 0) {
		blanket_nvram_prefix_unset(NULL, NVRAM_MAP_TS_PRIM_VLAN_ID);
		WBD_INFO("Unset NVRAM %s\n", NVRAM_MAP_TS_PRIM_VLAN_ID);
		ret = 0;
	}

	if (strlen(blanket_nvram_safe_get(NVRAM_MAP_TS_DEF_PCP)) > 0) {
		blanket_nvram_prefix_unset(NULL, NVRAM_MAP_TS_DEF_PCP);
		WBD_INFO("Unset NVRAM %s\n", NVRAM_MAP_TS_DEF_PCP);
		ret = 0;
	}

	/* Unset guest bit from nvram wlx.y_map */
	nvval = blanket_nvram_safe_get(WBD_NVRAM_IFNAMES);

	/* Traverse wbd_ifnames for each ifname */
	foreach(var_intf, nvval, next) {
		blanket_get_interface_prefix(var_intf, prefix, sizeof(prefix));

		map_flags = wbd_get_map_flags(prefix);
		if (I5_IS_BSS_GUEST(map_flags)) {
			int i;

			for (i = 1; i < WLIFU_MAX_NO_BRIDGE; i++) {
				snprintf(nvname, sizeof(nvname), "lan%d_ifnames", i);
				if (find_in_list(blanket_nvram_safe_get(nvname), var_intf)) {
					break;
				}
			}
			if (i >= WLIFU_MAX_NO_BRIDGE) {
				WBD_WARNING("Guest interface %s is not on any secondary bridges\n",
					var_intf);
				continue;
			}
			tmpret = wbd_slave_move_ifname_to_list(var_intf, nvname,
				NVRAM_LAN_IFNAMES);
			if (tmpret == 0) {
				WBD_INFO("Moved %s to Primary\n", var_intf);
				ret = 0;
			}

			map_flags &= ~IEEE1905_MAP_FLAG_GUEST;
			snprintf(strmap, sizeof(strmap), "%d", map_flags);
			blanket_nvram_prefix_set(prefix, NVRAM_MAP, strmap);
			WBD_INFO("Set %s%s to 0x%02x\n", prefix, NVRAM_MAP, map_flags);
			ret = 0;
		}
	}
	return ret;
}

/* Move the SSID's ifname to lanX_ifnames */
static int
wbd_slave_ts_move_ssid_to_secondary(unsigned char *ssid, unsigned int bridge_idx)
{
	int ret = -1, tmpret;
	char *nvval;
	char var_intf[IFNAMSIZ] = {0}, *next_intf;
	char prefix[IFNAMSIZ], strmap[WBD_MAX_BUF_16];
	unsigned char map_flags = 0;
	char src_lanX_ifnames[NVRAM_MAX_PARAM_LEN];
	char dst_lanY_ifnames[NVRAM_MAX_PARAM_LEN];

	snprintf(dst_lanY_ifnames, sizeof(dst_lanY_ifnames), "lan%d_ifnames", bridge_idx);

	nvval = blanket_nvram_safe_get(WBD_NVRAM_IFNAMES);

	/* Traverse wbd_ifnames for each ifname */
	foreach(var_intf, nvval, next_intf) {
		int i;
		blanket_get_interface_prefix(var_intf, prefix, sizeof(prefix));

		if (strcmp(blanket_nvram_prefix_safe_get(prefix, NVRAM_SSID), (char*)ssid) != 0) {
			continue;
		}

		for (i = 0; i < WLIFU_MAX_NO_BRIDGE; ++i) {
			if (i == 0) {
				snprintf(src_lanX_ifnames, sizeof(src_lanX_ifnames),
					"lan_ifnames");
			} else {
				snprintf(src_lanX_ifnames, sizeof(src_lanX_ifnames),
					"lan%d_ifnames", i);
			}
			if (find_in_list(nvram_safe_get(src_lanX_ifnames), var_intf)) {
				break;
			}
		}
		if (i >= WLIFU_MAX_NO_BRIDGE) {
			WBD_ERROR("Interface %s present in wbd_ifnames but "
				"not found in any of lanX_ifnames\n", var_intf);
			continue;
		}

		if (i == bridge_idx) {
			WBD_INFO("Interface %s is already in %s. Nothing to do\n",
				var_intf, dst_lanY_ifnames);
			continue;
		}
		/* SSID is matching with the ifname. So move it to secondary */
		WBD_INFO("Moving SSID %s ifname %s from %s to %s\n", ssid, var_intf,
			src_lanX_ifnames, dst_lanY_ifnames);
		tmpret = wbd_slave_move_ifname_to_list(var_intf,
			src_lanX_ifnames, dst_lanY_ifnames);
		if (tmpret == 0) {
			WBD_INFO("Moved %s from br%d to br%d\n", var_intf, i, bridge_idx);
			ret = 0;
		}

		map_flags = wbd_get_map_flags(prefix);
		if (!I5_IS_BSS_GUEST(map_flags)) {
			map_flags |= IEEE1905_MAP_FLAG_GUEST;
			snprintf(strmap, sizeof(strmap), "%d", map_flags);
			blanket_nvram_prefix_set(prefix, NVRAM_MAP, strmap);
			WBD_INFO("Set %s_%s to 0x%02x\n", prefix, NVRAM_MAP, map_flags);
			ret = 0;
		}
	}

	return ret;
}

/* Update SSID for the VLANs return 0 if there is any change in the settings */
static int
wbd_slave_process_traffic_separation_ssids_policy(ieee1905_ts_policy_t *ts_policy, char *strvlan,
	short int primary_vlan_id, unsigned int bridge_idx)
{
	int ret = -1;
	dll_t *item_p;
	ieee1905_ssid_list_type *ssid_list;
	char *nvval;
	char nvval_ssid_list[NVRAM_MAX_VALUE_LEN], nvname[NVRAM_MAX_PARAM_LEN];
	char strssid[IEEE1905_MAX_SSID_LEN * 4 + 1];	/* Maximum size = 32 spaces each escaped
							 * using 4 bytes
							 */

	/* Create NVRAM name in the format map_ts_vlanid_ssid */
	snprintf(nvname, sizeof(nvname), "map_ts_%s_ssid", strvlan);
	nvval = blanket_nvram_safe_get(nvname);
	memset(nvval_ssid_list, 0, sizeof(nvval_ssid_list));
	WBDSTRNCPY(nvval_ssid_list, nvval, sizeof(nvval_ssid_list));
	WBD_INFO("[%s]=[%s]\n", nvname, nvval_ssid_list);

	/* Travese Traffic Separation SSID List items */
	foreach_glist_item(item_p, ts_policy->ssid_list) {

		ssid_list = (ieee1905_ssid_list_type*)item_p;

		/* Copy SSID and escape spaces as SSID is put into space separated nvram list */
		memcpy(strssid, ssid_list->ssid.SSID, ssid_list->ssid.SSID_len);
		strssid[ssid_list->ssid.SSID_len] = 0;
		wbd_escape_space(strssid, sizeof(strssid));

		/* Check if the SSID is present in the list */
		if (!find_in_list(nvval_ssid_list, strssid)) {
			if (add_to_list(strssid, nvval_ssid_list, sizeof(nvval_ssid_list)) == 0) {
				nvram_set(nvname, nvval_ssid_list);
				WBD_INFO("New [%s]=[%s]\n", nvname, nvval_ssid_list);
				ret = 0;
			}
		}
		/* Now move the ifname to secondary bridge if the SSID belongs to non
			* primary VLAN
			*/
		if (atoi(strvlan) != primary_vlan_id) {
			ret = wbd_slave_ts_move_ssid_to_secondary(
				ssid_list->ssid.SSID, bridge_idx);
		}
	}

	return ret;
}

/* Process the traffic separation policy. Return 0 If there is any change in the policy So that
 * the caller can restart
 */
int
wbd_slave_process_traffic_separation_policy(ieee1905_policy_config *policy,
	unsigned char rcvd_policies)
{
	int ret = -1, vlan_id;
	dll_t *item_p;
	ieee1905_ts_policy_t *ts_policy;
	char *nvval, *next, *ssid_nvval, *next_ssid;
	char strvlan[WBD_MAX_BUF_16];
	char nvval_ts_list[NVRAM_MAX_VALUE_LEN] = {0}, nvval_ssid_list[NVRAM_MAX_VALUE_LEN] = {0};
	char itr_vlan_id[NVRAM_MAX_VALUE_LEN], itr_ssid[NVRAM_MAX_VALUE_LEN];
	char nvname[NVRAM_MAX_VALUE_LEN * 2];
	unsigned int bridge_idx = 1; /* 0 is primary bridge */
	WBD_ENTER();

	/* If there is no SSID to VLANID mapping, then remove the traffic separation policy */
	if (policy->ts_policy_list.count == 0) {
		if (wbd_slave_remove_traffic_separation_policy() == 0) {
			ret = 0;
		}
		goto end;
	}
	if (wbd_slave_check_for_ts_policy_errors(policy) == 1) {
		WBD_DEBUG("Profile 2 Error Message sent from ieee1905 \n");
		goto end;
	}

	nvval = blanket_nvram_safe_get(NVRAM_MAP_TS_VLANS);
	memset(nvval_ts_list, 0, sizeof(nvval_ts_list));
	WBDSTRNCPY(nvval_ts_list, nvval, sizeof(nvval_ts_list));
	WBD_INFO("[%s]=[%s]\n", NVRAM_MAP_TS_VLANS, nvval_ts_list);

	/* Loop through all the VLAN IDs stored in NVRAM to identify the VLAN ID's which are
	 * already present locally but not in the request
	 */
	foreach(itr_vlan_id, nvval_ts_list, next) {

		vlan_id = atoi(itr_vlan_id);
		if ((ts_policy = i5DmTSPolicyFindVlanId(&policy->ts_policy_list, vlan_id))) {
			/* VLAN ID present, now check all the SSID's present for that VLAN ID
			 * or not
			 */
			snprintf(nvname, sizeof(nvname), "map_ts_%s_ssid", itr_vlan_id);
			ssid_nvval = blanket_nvram_safe_get(nvname);
			memset(nvval_ssid_list, 0, sizeof(nvval_ssid_list));
			WBDSTRNCPY(nvval_ssid_list, ssid_nvval, sizeof(nvval_ssid_list));

			foreach(itr_ssid, ssid_nvval, next_ssid) {

				char strssid[IEEE1905_MAX_SSID_LEN * 4 + 1];

				/* Copy SSID and unescape spaces */
				WBDSTRNCPY(strssid, itr_ssid, sizeof(strssid));
				wbd_unescape_space(strssid);

				if (i5DmTSPolicyFindSSIDinSSIDList(ts_policy,
					(unsigned char*)strssid)) {
					continue;
				}
				/* SSID not found in request. Remove it */
				remove_from_list(itr_ssid, nvval_ssid_list,
					sizeof(nvval_ssid_list));
				nvram_set(nvname, nvval_ssid_list);
				WBD_INFO("After removing SSID[%s]. New [%s]=[%s]\n",
					itr_ssid, nvname, nvval_ssid_list);
				ret = 0;
			}
			continue;
		}

		/* Need to remove this vlan ID and SSID's assigned to it */
		remove_from_list(itr_vlan_id, nvval_ts_list, sizeof(nvval_ts_list));
		nvram_set(NVRAM_MAP_TS_VLANS, nvval_ts_list);
		WBD_INFO("After removing VLAN ID[%s]. New [%s]=[%s]\n",
			itr_vlan_id, NVRAM_MAP_TS_VLANS, nvval_ts_list);
		ret = 0;

		/* remove SSID */
		snprintf(nvname, sizeof(nvname), "map_ts_%s_ssid", itr_vlan_id);
		if (strlen(blanket_nvram_safe_get(nvname)) > 0) {
			blanket_nvram_prefix_unset(NULL, nvname);
			WBD_INFO("Unset NVRAM %s\n", nvname);
		}
	}

	/* Travese Traffic Separation Policy Config List items */
	foreach_glist_item(item_p, policy->ts_policy_list) {

		ts_policy = (ieee1905_ts_policy_t*)item_p;

		/* Check if the VLAN ID is present in the list */
		snprintf(strvlan, sizeof(strvlan), "%d", ts_policy->vlan_id);
		if (!find_in_list(nvval_ts_list, strvlan)) {
			if (add_to_list(strvlan, nvval_ts_list, sizeof(nvval_ts_list)) == 0) {
				nvram_set(NVRAM_MAP_TS_VLANS, nvval_ts_list);
				WBD_INFO("New [%s]=[%s]\n", NVRAM_MAP_TS_VLANS, nvval_ts_list);
				ret = 0;
			}
		}

		if (wbd_slave_process_traffic_separation_ssids_policy(ts_policy, strvlan,
			policy->prim_vlan_id, bridge_idx) == 0) {
			ret = 0;
		}
		if (ts_policy->vlan_id != policy->prim_vlan_id) {
			bridge_idx++;
		}
	}

	/* Now apply 802.1Q Default settings */
	if (rcvd_policies & MAP_POLICY_TYPE_FLAG_TS_8021QSET) {
		snprintf(strvlan, sizeof(strvlan), "%d", policy->prim_vlan_id);
		if (blanket_nvram_prefix_match_set(NULL, NVRAM_MAP_TS_PRIM_VLAN_ID, strvlan,
			FALSE)) {
			WBD_INFO("New [%s]=[%s]\n", NVRAM_MAP_TS_PRIM_VLAN_ID, strvlan);
			ret = 0;
		}
		snprintf(strvlan, sizeof(strvlan), "%d", policy->default_pcp);
		if (blanket_nvram_prefix_match_set(NULL, NVRAM_MAP_TS_DEF_PCP, strvlan, FALSE)) {
			WBD_INFO("New [%s]=[%s]\n", NVRAM_MAP_TS_DEF_PCP, strvlan);
			ret = 0;
		}
	}

end:
	WBD_EXIT();
	return ret;
}

/* Process tunnel CAC message from controller */
void wbd_slave_process_cac_msg(wbd_info_t *info, uint8 *al_mac, void *msg, uint32 msg_type)
{
	WBD_ENTER();

	switch (msg_type) {
		case MAP_CAC_RQST:
		{
			wbd_slave_process_cac_request(info, (ieee1905_cac_rqst_list_t*)msg);
		}
		break;

		case MAP_CAC_TERMINATE:
		{
			wbd_slave_process_cac_terminate_rqst(info,
				(ieee1905_cac_termination_list_t*)msg);
		}
		break;

		default:
		{
			WBD_ERROR("Invalid CAC msg[%d] recvd, exit \n", msg_type);
		}
		break;
	}
	WBD_EXIT();
}

static void
wbd_slave_start_cac_cb(bcm_usched_handle *hdl, void *arg)
{
	i5_dm_device_type *sdev = NULL;
	i5_dm_interface_type *pdmif = NULL;
	wbd_ifr_item_t *ifr_vndr_data;
	int ret = 0;
	wl_chan_change_reason_t reason = 0;
	wbd_info_t *info = (wbd_info_t *)arg;
	bool cac_initiated = 0;

	WBD_ENTER();

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);
	foreach_i5glist_item(pdmif, i5_dm_interface_type, sdev->interface_list) {
		if (!pdmif->vndr_data) {
			WBD_ERROR(" No vendor data exist for interface ["MACF"]\n",
				ETHERP_TO_MACF(pdmif->InterfaceId));
			continue;
		}
		ifr_vndr_data = (wbd_ifr_item_t*)pdmif->vndr_data;

		if (ifr_vndr_data->cac_cur_state != MAP_CAC_RQST_RCVD) {
			continue;
		}

		if (ifr_vndr_data->cac_rqst_method == MAP_CAC_METHOD_MIMO_DIMENSION_REDUCED_CAC) {
			reason = WL_CHAN_REASON_DFS_AP_MOVE_START;
		} else if (ifr_vndr_data->cac_rqst_method == MAP_CAC_METHOD_CONTINOUS_CAC) {
			if (ifr_vndr_data->cac_rqst_action == MAP_CAC_ACTION_STAY_ON_NEW_CHANNEL) {
				reason = WL_CHAN_REASON_CSA;
			} else {
				reason = WL_CHAN_REASON_CSA_TO_DFS_CHAN_FOR_CAC_ONLY;
			}
		}

		/* Backup current chanspec, required for returning to previous channel */
		ifr_vndr_data->old_chanspec = pdmif->chanspec;

		WBD_DEBUG("Before initiate CAC request for ["MACF"] current cac state[%d]\n",
			ETHERP_TO_MACF(pdmif->InterfaceId), ifr_vndr_data->cac_cur_state);

		/* Update the chanspec through ACSD */
		wbd_slave_set_chanspec_through_acsd(ifr_vndr_data->cac_rqst_chspec, reason, pdmif);

		if (ifr_vndr_data->cac_rqst_action == MAP_CAC_ACTION_RETURN_TO_PREV_CHANNEL) {
			if (ifr_vndr_data->cac_rqst_method ==
				MAP_CAC_METHOD_MIMO_DIMENSION_REDUCED_CAC) {
				/* issue stunt sub command after dfs_ap_move start */
				WBD_INFO("issue stunt command to DFS_AP_MOVE iovar \n");
				wbd_slave_set_chanspec_through_acsd(ifr_vndr_data->cac_rqst_chspec,
					WL_CHAN_REASON_DFS_AP_MOVE_STUNT, pdmif);
			}
		}

		WBD_INFO("Initiated CAC for ["MACF"] for chanspec[%x] with method[%d] reason [%d] "
			"expected action after cac[%d] \n", ETHERP_TO_MACF(pdmif->InterfaceId),
			ifr_vndr_data->cac_rqst_chspec, ifr_vndr_data->cac_rqst_method, reason,
			ifr_vndr_data->cac_rqst_action);

		ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_RUNNING;
		cac_initiated = 1;
	}
end:
	if (!cac_initiated) {
		WBD_INFO("No CACs to initiate. Mark CAC start timer stoped \n");
		info->wbd_slave->flags &= ~WBD_BKT_SLV_FLAGS_CAC_RUNNING;

		/* Send channel preference report, if there is at least one
		 * completed CAC and no new CACs initiated
		 */
		foreach_i5glist_item(pdmif, i5_dm_interface_type, sdev->interface_list) {
			if (!pdmif->vndr_data) {
				WBD_ERROR(" No vendor data exist for interface ["MACF"]\n",
					ETHERP_TO_MACF(pdmif->InterfaceId));
				continue;
			}
			ifr_vndr_data = (wbd_ifr_item_t*)pdmif->vndr_data;

			if (ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_COMPLETE) {
				ieee1905_send_chan_preference_report();
				break;
			}
		}
	}
	WBD_EXIT();
}

static int
wbd_slave_create_cac_start_timer(wbd_info_t *info)
{
	int ret = WBDE_OK;
	i5_dm_device_type *sdev = NULL;
	i5_dm_interface_type *pdmif = NULL;
	wbd_ifr_item_t *ifr_vndr_data;
	WBD_ENTER();

	WBD_ASSERT_ARG(info, WBDE_INV_ARG);

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);

	foreach_i5glist_item(pdmif, i5_dm_interface_type, sdev->interface_list) {
		if (!pdmif->vndr_data) {
			WBD_ERROR(" No vendor data exist for interface ["MACF"]\n",
				ETHERP_TO_MACF(pdmif->InterfaceId));
			continue;
		}
		ifr_vndr_data = (wbd_ifr_item_t*)pdmif->vndr_data;

		if (ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_RUNNING) {
			WBD_INFO("CAC running on if %s. No need to start CAC timer\n",
				pdmif->ifname);
			goto end;
		}
	}

	WBD_INFO("Creating timer for starting CAC\n");
	ret = wbd_add_timers(info->hdl, info, 0, wbd_slave_start_cac_cb, 0);

	if (ret == WBDE_OK) {
		info->wbd_slave->flags |= WBD_BKT_SLV_FLAGS_CAC_RUNNING;
	} else {
		WBD_WARNING("Failed to create cac start timer\n");
	}

end:
	WBD_EXIT();
	return ret;
}

/* Process CAC Request message from controller, initiate CAC on radio if required. */
static void
wbd_slave_process_cac_request(wbd_info_t *info, ieee1905_cac_rqst_list_t *pmsg)
{
	i5_dm_interface_type *pdmif = NULL;
	wbd_ifr_item_t *ifr_vndr_data;
	ieee1905_radio_cac_rqst_t *cac_rqst = NULL;
	int ret = 0;
	uint16 chanspec = 0;
	uint8 cac_method = 0;
	uint8 cac_action = 0;
	wl_chan_change_reason_t reason = 0;
	uint8 i = 0;
	bool start_cac = 0;
	bool cac_method_valid;
	bool cac_action_valid;

	WBD_ENTER();
	if (!pmsg) {
		WBD_ERROR("Invalid CAC request arguments, exit \n");
		return;
	}

	WBD_DEBUG("CAC start request for n_radios[%d] \n", pmsg->count);

	cac_rqst = pmsg->params;

	for (i = 0; i < pmsg->count; i++) {
		uint8 channel;
		uint bitmap = 0;

		pdmif = wbd_ds_get_self_i5_interface(cac_rqst[i].mac, &ret);
		if (!pdmif || !pdmif->vndr_data) {
			WBD_ERROR(" No valid interface exist with ["MACF"] or No vendor data\n",
				ETHERP_TO_MACF(cac_rqst[i].mac));
			continue;
		}
		ifr_vndr_data = (wbd_ifr_item_t*)pdmif->vndr_data;

		WBD_INFO("CAC request for ["MACF"] opclass[%d] chan[%d] flags[%x] \n",
			ETHERP_TO_MACF(cac_rqst[i].mac), cac_rqst[i].opclass,
			cac_rqst[i].chan, cac_rqst[i].flags);

		chanspec = blanket_prepare_chanspec(cac_rqst[i].chan, cac_rqst[i].opclass, 0, 0);

		/* Get cac method and check if it is enabled and supported */
		cac_method = GET_MAP_CAC_METHOD(cac_rqst[i].flags);
		cac_method_valid =
			MAP_IS_CAC_METHOD_ENABLED(ifr_vndr_data->cac_method_flag, cac_method) &&
			((cac_method == MAP_CAC_METHOD_MIMO_DIMENSION_REDUCED_CAC) ||
			(cac_method == MAP_CAC_METHOD_CONTINOUS_CAC));

		/* Get cac completion action and check if it is valid */
		cac_action = GET_MAP_CAC_COMPLETION_ACTION(cac_rqst[i].flags);
		cac_action_valid =
			(cac_action == MAP_CAC_ACTION_STAY_ON_NEW_CHANNEL) ||
			(cac_action == MAP_CAC_ACTION_RETURN_TO_PREV_CHANNEL);

		/* Check if all the fields in the requst for the current radio is valid */
		if (!wf_chspec_valid(chanspec) || !cac_method_valid || !cac_action_valid) {
			WBD_ERROR("Invalid CAC request parameter(s) for Interface ["MACF"]. "
				"%svalid chanspec[0x%x] (opclass[%d] channel[%d], "
				"%svalid cac method (supported flag [0x%x] request [%d]), "
				"%svalid cac action [%d]\n", ETHERP_TO_MACF(cac_rqst[i].mac),
				wf_chspec_valid(chanspec) ? "" : "in", chanspec,
				cac_rqst[i].opclass, cac_rqst[i].chan,
				cac_method_valid ? "" : "in", ifr_vndr_data->cac_method_flag,
				cac_method, cac_action_valid ? "" : "in", cac_action);

			if (ifr_vndr_data->cac_cur_state == 0) {
				ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_COMPLETE;
				ifr_vndr_data->cac_compl_status =
					MAP_CAC_STATUS_RQST_NOT_SUPPORTED;
			}
			continue;
		}

		/* abort any ongoing cac, if any cac is running while taking new request */
		if (ifr_vndr_data->cac_cur_state) {
			chanspec_t sb_masked_cac_chspec;

			/* side bands are not relevent for center channels in Table E-4 */
			if (cac_rqst[i].opclass <= REGCLASS_5G_40MHZ_LAST) {
				sb_masked_cac_chspec = ifr_vndr_data->cac_rqst_chspec;
			} else {
				sb_masked_cac_chspec =
					blanket_mask_chanspec_sb(ifr_vndr_data->cac_rqst_chspec);
			}

			if ((sb_masked_cac_chspec == chanspec) &&
				(ifr_vndr_data->cac_rqst_method == cac_method) &&
				(ifr_vndr_data->cac_rqst_action == cac_action)) {
				WBD_INFO("cac is already added with same opclass[%d] chan[%d]\n",
					cac_rqst[i].chan, cac_rqst[i].opclass);
				continue;
			} else if (ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_RUNNING) {
				WBD_INFO("abort Running CAC method [%d]\n",
					ifr_vndr_data->cac_rqst_method);
				if (ifr_vndr_data->cac_rqst_method ==
					MAP_CAC_METHOD_MIMO_DIMENSION_REDUCED_CAC) {
					wbd_slave_set_chanspec_through_acsd(
						ifr_vndr_data->cac_rqst_chspec,
						WL_CHAN_REASON_DFS_AP_MOVE_ABORTED, pdmif);
				} else {
					wbd_slave_set_chanspec_through_acsd(
						ifr_vndr_data->old_chanspec,
						WL_CHAN_REASON_CSA, pdmif);
				}
			}
		}

		FOREACH_20_SB(chanspec, channel) {
			uint bitmap_sb = 0x00;
			ret = blanket_get_chan_info(pdmif->ifname, channel,
				CHSPEC_BAND(chanspec), &bitmap_sb);
			if (ret < 0) {
				WBD_ERROR("chan_info failed for chan %d\n", channel);
				break;
			}
			if (!(bitmap_sb & WL_CHAN_VALID_HW) ||
					!(bitmap_sb & WL_CHAN_VALID_SW)) {
				ret = -1;
				break;
			}
			bitmap |= bitmap_sb;
		}
		if ((ret < 0) || !(bitmap & WL_CHAN_RADAR)) {
			WBD_ERROR("Invalid or non-DFS chanspec [0x%x] ret [%d] bitmap [0x%x]\n",
				chanspec, ret, bitmap);
			if (ifr_vndr_data->cac_cur_state == 0) {
				ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_COMPLETE;
				ifr_vndr_data->cac_compl_status = MAP_CAC_STATUS_RQST_NONCONFORM;
			}
			continue;
		}

		if (chanspec == pdmif->chanspec) {
			if (bitmap & WL_CHAN_PASSIVE) {
				ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_RUNNING;
				info->wbd_slave->flags |= WBD_BKT_SLV_FLAGS_CAC_RUNNING;
			} else {
				ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_COMPLETE;
				ifr_vndr_data->cac_compl_status = MAP_CAC_STATUS_SUCCESS;
			}
			ifr_vndr_data->cac_rqst_chspec = chanspec;
			WBD_INFO("CAC request received on current chanspec [0x%x]. "
				"cac state [%d] bitmap [0x%d]\n", chanspec,
				ifr_vndr_data->cac_cur_state, bitmap);
			continue;
		}

		if (bitmap & WL_CHAN_INACTIVE) {
			WBD_INFO("Chanspec [0x%x] is already radar deteced. bitmap [0x%x]\n",
				chanspec, bitmap);
			ifr_vndr_data->cac_rqst_chspec = chanspec;
			ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_COMPLETE;
			ifr_vndr_data->cac_compl_status = MAP_CAC_STATUS_RADAR_DETECTED;
			continue;
		}

		/* XXX: zdfs from one DFS channel to another maynot work on all platforms.
		 * But agent should try it, if the controller asks
		 */

		WBD_INFO("Received CAC for ["MACF"] for chanspec[%x] with method[%d] reason [%d] "
			"cac completion action [%d]\n", ETHERP_TO_MACF(pdmif->InterfaceId),
			chanspec, cac_method, reason, cac_action);

		/* start fresh, reset ABORT status if set with terminate CAC */
		ifr_vndr_data->cac_cur_state = MAP_CAC_RQST_RCVD;
		ifr_vndr_data->cac_rqst_chspec = chanspec;
		ifr_vndr_data->cac_rqst_method = cac_method;
		ifr_vndr_data->cac_rqst_action = cac_action;
		start_cac = 1;
	}

	if (start_cac) {
		wbd_slave_create_cac_start_timer(info);
	}
	if (!(info->wbd_slave->flags & WBD_BKT_SLV_FLAGS_CAC_RUNNING)) {
		ieee1905_send_chan_preference_report();
	}
	WBD_EXIT();
}

/* Process CAC terminate message from controller to stop ongoing CAC on 5G radio */
static void
wbd_slave_process_cac_terminate_rqst(wbd_info_t *info, ieee1905_cac_termination_list_t *pmsg)
{
	i5_dm_interface_type *pdmif = NULL;
	ieee1905_radio_cac_params_t *cac_term = NULL;
	wbd_ifr_item_t *ifr_vndr_data = NULL;
	int ret = 0;
	uint16 chanspec = 0;
	uint8 i = 0;
	bool cac_running = 0;

	WBD_ENTER();

	if (!pmsg) {
		WBD_ERROR("Invalid CAC terminate arguments, exit \n");
		goto end;
	}

	WBD_INFO("CAC terminate for n_radio[%d] \n", pmsg->count);

	cac_term = pmsg->params;

	for (i = 0; i < pmsg->count; i++) {
		chanspec_t sb_masked_cac_chspec;

		pdmif = wbd_ds_get_self_i5_interface(cac_term[i].mac, &ret);
		if (!pdmif || !pdmif->vndr_data) {
			WBD_ERROR(" No valid interface exist with ["MACF"] or No vendor data\n",
				ETHERP_TO_MACF(cac_term[i].mac));
			continue;
		}
		ifr_vndr_data = (wbd_ifr_item_t*)pdmif->vndr_data;

		WBD_INFO("CAC terminate rqst for: ["MACF"] ifname[%s] opclass[%d] chan[%d]. "
			"Current CAC details chanspec [0x%x] method [%d] action [%d] state [%d] "
			"completion status [%d]\n", ETHERP_TO_MACF(pdmif->InterfaceId),
			pdmif->ifname, cac_term[i].opclass, cac_term[i].chan,
			ifr_vndr_data->cac_rqst_chspec, ifr_vndr_data->cac_rqst_method,
			ifr_vndr_data->cac_rqst_action, ifr_vndr_data->cac_cur_state,
			ifr_vndr_data->cac_compl_status);

		if (ifr_vndr_data->cac_cur_state == 0) {
			WBD_WARNING("Ignoring CAC termination request for ["MACF"] since "
				"no CAC running\n", ETHERP_TO_MACF(pdmif->InterfaceId));
			continue;
		}

		chanspec = blanket_prepare_chanspec(cac_term[i].chan, cac_term[i].opclass, 0, 0);

		/* side bands are not relevent for center channels in Table E-4 */
		if (cac_term[i].opclass <= REGCLASS_5G_40MHZ_LAST) {
			sb_masked_cac_chspec = ifr_vndr_data->cac_rqst_chspec;
		} else {
			sb_masked_cac_chspec =
				blanket_mask_chanspec_sb(ifr_vndr_data->cac_rqst_chspec);
		}

		if (sb_masked_cac_chspec != chanspec) {
			WBD_WARNING("Ignoring CAC termination request for ["MACF"]. CAC "
				"requested on chanspec [0x%x]. termination requested on "
				"chanspec [0x%x] (opclass [%d] channel [%d])\n",
				ETHERP_TO_MACF(pdmif->InterfaceId), sb_masked_cac_chspec,
				chanspec, cac_term[i].opclass, cac_term[i].chan);
			continue;
		}

		if (ifr_vndr_data->cac_cur_state == MAP_CAC_RQST_RUNNING) {
			if (ifr_vndr_data->cac_rqst_method == MAP_CAC_METHOD_CONTINOUS_CAC) {
				wbd_slave_set_chanspec_through_acsd(ifr_vndr_data->old_chanspec,
					WL_CHAN_REASON_CSA, pdmif);
			} else {
				wbd_slave_set_chanspec_through_acsd(ifr_vndr_data->cac_rqst_chspec,
					WL_CHAN_REASON_DFS_AP_MOVE_ABORTED, pdmif);
			}
			cac_running = 1;
		}
		ifr_vndr_data->cac_cur_state = 0;
	}
end:
	if (cac_running) {
		wbd_slave_create_cac_start_timer(info);
	}
	WBD_EXIT();
}

/* Prepare CAC capability for Multi AP agent with all cac capable radios */
void
wbd_slave_prepare_cac_capability(uint8 **pbuf, uint16 *payload_len)
{
	i5_dm_device_type *self_dev = NULL;
	i5_dm_interface_type *pdmif = NULL;
	ieee1905_cac_capabilities_t *cac_cap = NULL;
	uint8 *ptr = NULL;
	uint8 cac_capable_radio = 0, get_country_code = 1;
	int ret = WBDE_OK;

	WBD_ENTER();

	if (!pbuf || !*pbuf) {
		WBD_ERROR("CAC capbility buffer NULL\n");
		return;
	}

	cac_cap = (ieee1905_cac_capabilities_t*)*pbuf;
	ptr = *pbuf;

	WBD_SAFE_GET_I5_SELF_DEVICE(self_dev, &ret);

	/* traverse each interface in self device and add cac capability for valid
	 * interface i.e. 5G and cac capable
	 */
	foreach_i5glist_item(pdmif, i5_dm_interface_type, self_dev->interface_list) {
		uint16 filled_len = 0;
		if (!i5DmIsInterfaceWireless(pdmif->MediaType)) {
			continue;
		}

		/* Read country code from the first radio only */
		if (get_country_code) {
			wl_country_t country;

			memset(&country, 0, sizeof(country));
			blanket_get_country(pdmif->ifname, &country);
			memcpy(cac_cap->country, country.ccode, sizeof(cac_cap->country));
			ptr += OFFSETOF(ieee1905_cac_capabilities_t, ifr_info);
			WBD_INFO("ifname [%s] CAC Capability Country: [%c%c]\n",
				pdmif->ifname, cac_cap->country[0], cac_cap->country[1]);
			get_country_code = 0;
		}

		if (!WBD_BAND_TYPE_LAN_5G(pdmif->band)) {
			continue;
		}

		wbd_slave_prepare_radio_cac_capability(pdmif, &ptr, &filled_len);
		if (filled_len) {
			cac_capable_radio++;
			ptr += filled_len;
		}

		WBD_DEBUG("Done CAC capability for ["MACF"] with payload[%d] \n",
			ETHERP_TO_MACF(pdmif->InterfaceId), filled_len);
	}
	cac_cap->n_radio = cac_capable_radio;
	*payload_len = ptr - *pbuf;
	WBD_DEBUG("Total CAC capable radio [%d] payload [%d] \n", cac_capable_radio,
		*payload_len);
end:
	WBD_EXIT();
}

/* As of now getting driver capability just to check whether
 * driver supports 3+1 BGDFS feature or not. Hence not storing
 * capability for future use.
 */
static int
wbd_slave_get_driver_capability(char *ifname, wbd_ifr_item_t *ifr_vndr_info)
{
	int ret = WBDE_OK;
#ifndef MULTIAP_PLUGFEST
	char driver_capability[WBD_MAX_BUF_8192];
	char *str = NULL;
	char *token = " ";

	WBD_ENTER();

	WBD_ASSERT_ARG(ifname, WBDE_INV_ARG);
	WBD_ASSERT_ARG(ifr_vndr_info, WBDE_INV_ARG);
	memset(driver_capability, 0, sizeof(driver_capability));

	ret = blanket_get_driver_capability(ifname, driver_capability, sizeof(driver_capability));
	WBD_ASSERT();

	str = strtok(driver_capability, token);
	/* since this interface is 5G, Continous CAC is by default enabled */
	MAP_ENABLE_CAC_METHOD(ifr_vndr_info->cac_method_flag, MAP_CAC_METHOD_CONTINOUS_CAC);

	while (str != NULL) {
		if (!strncmp(str, "bgdfs", strlen("bgdfs"))) {
			wlc_rev_info_t rev = {0};
			blanket_get_revinfo((char *)ifname, &rev);

			if (rev.corerev < 128) {
				WBD_INFO("%s: explicitly disabling bgdfs on corerev < 128\n", ifname);
				break;
			}
			MAP_ENABLE_CAC_METHOD(ifr_vndr_info->cac_method_flag,
				MAP_CAC_METHOD_MIMO_DIMENSION_REDUCED_CAC);
			break;
		}

		str = strtok(NULL, token);
	}
	WBD_INFO("ifname[%s] support cac method [%x] \n", ifname, ifr_vndr_info->cac_method_flag);
end:
	WBD_EXIT();
#else
	MAP_ENABLE_CAC_METHOD(ifr_vndr_info->cac_method_flag, MAP_CAC_METHOD_CONTINOUS_CAC);
	WBD_INFO("ifname[%s] support cac method [%x] \n", ifname, ifr_vndr_info->cac_method_flag);
#endif /* MULTIAP_PLUGFEST */
	return ret;
}

static void
wbd_slave_prepare_radio_cac_capability(i5_dm_interface_type *pdmif, uint8 **pmem,
	uint16 *payload_len)
{
	ieee1905_ifr_cac_info_t *radio_info = (ieee1905_ifr_cac_info_t*)*pmem;
	wbd_ifr_item_t *ifr_vndr_data;
	uint8 *pbuf = NULL;
	uint32 is_edcrs_eu = 0;

	WBD_ENTER();

	if (!pdmif) {
		WBD_ERROR(" No valid interface exist for this event, skip \n");
		goto end;
	}

	ifr_vndr_data = (wbd_ifr_item_t*)pdmif->vndr_data;
	if (!ifr_vndr_data) {
		WBD_ERROR("[%s]: Invalid ifr vendor data plz Debug, exiting ..\n",
			pdmif->ifname);
		goto end;
	}

	pbuf = *pmem;
	pbuf += OFFSETOF(ieee1905_ifr_cac_info_t, mode_info);

	/* CAC capability TLV information per radio:
	 * ------------------------------------------------------------------------------------
	 * | MAC(6 Byte) | Number of types of CAC (1 bytes) | variable info * number of types |
	 * -----------------------------------------------------------------------------------
	 *
	 * Number of types of CAC - combinations of CAC(3+1/Full Time CAC)
	 *			and number of seconds required to complete CAC
	 *
	 * variable info for CAC Radio:
	 *	CAC mode support - 1 Byte (3+1 BGDFS/Full time CAC)
	 *	Number of seconds required to complete CAC - 3 Bytes
	 *	Number of classes supported for Given mode - 1 Byte
	 *	variable :
	 *	---------------------------------------------------------
	 *	|operating class | number of channels | list of channels |
	 *	---------------------------------------------------------
	 */
	memcpy(&radio_info->mac, (uchar*)pdmif->InterfaceId, ETHER_ADDR_LEN);

	wbd_slave_get_driver_capability(pdmif->ifname, ifr_vndr_data);
	/* Check if EDCRS, only in 5GH because all the weather channels are in 5GH */
	if (pdmif->band & BAND_5GH) {
		blanket_get_is_edcrs_eu(pdmif->ifname, &is_edcrs_eu);
	}

	WBD_DEBUG("[%s]: band [%d] CAC cap: [0x%x] edcrs [%d]\n", pdmif->ifname,
		pdmif->band, ifr_vndr_data->cac_method_flag, is_edcrs_eu);
	if (MAP_IS_CAC_METHOD_ENABLED(ifr_vndr_data->cac_method_flag,
		MAP_CAC_METHOD_CONTINOUS_WITH_DEDICATED_RADIO)) {
		radio_info->n_cac_types += wbd_slave_update_cac_capability_for_method(pdmif,
			&pbuf, is_edcrs_eu, MAP_CAC_METHOD_CONTINOUS_WITH_DEDICATED_RADIO);
	}
	if (MAP_IS_CAC_METHOD_ENABLED(ifr_vndr_data->cac_method_flag,
		MAP_CAC_METHOD_CONTINOUS_CAC)) {
		radio_info->n_cac_types += wbd_slave_update_cac_capability_for_method(pdmif,
			&pbuf, is_edcrs_eu, MAP_CAC_METHOD_CONTINOUS_CAC);
	}
	if (MAP_IS_CAC_METHOD_ENABLED(ifr_vndr_data->cac_method_flag,
		MAP_CAC_METHOD_MIMO_DIMENSION_REDUCED_CAC)) {
		radio_info->n_cac_types += wbd_slave_update_cac_capability_for_method(pdmif,
			&pbuf, is_edcrs_eu, MAP_CAC_METHOD_MIMO_DIMENSION_REDUCED_CAC);
	}
	if (MAP_IS_CAC_METHOD_ENABLED(ifr_vndr_data->cac_method_flag,
			MAP_CAC_METHOD_TIME_SLICED_CAC)) {
		radio_info->n_cac_types += wbd_slave_update_cac_capability_for_method(pdmif,
			&pbuf, is_edcrs_eu, MAP_CAC_METHOD_TIME_SLICED_CAC);
	}
	WBD_INFO("[%s]: band [%d] Number of CAC types added to capability: %d\n",
		pdmif->ifname, pdmif->band, radio_info->n_cac_types);
	if (radio_info->n_cac_types) {
		*payload_len += pbuf - *pmem;
	}

end:
	WBD_DEBUG("radio["MACF"] CAC INFO payload [%d] pbuf[%p] start[%p] \n",
		ETHERP_TO_MACF(pdmif->InterfaceId), *payload_len, pbuf, *pmem);

	WBD_EXIT();
}

/* Prepare CAC capability for the given method */
static uint8
wbd_slave_update_cac_capability_for_method(i5_dm_interface_type *pdmif,
	uint8 **pbuf, uint32 is_edcrs_eu, uint8 method)
{
	uint16 len = 0;
	uint8 n_cac_types = 0;

	wbd_slave_update_cac_capability_for_method_and_cac_time(
		pdmif, *pbuf, &len, is_edcrs_eu, 0, method);
	if (len) {
		n_cac_types++;
		*pbuf += len;
	}
	if (is_edcrs_eu) {
		wbd_slave_update_cac_capability_for_method_and_cac_time(
			pdmif, *pbuf, &len, is_edcrs_eu, 1, method);
		if (len) {
			n_cac_types++;
			*pbuf += len;
		}
	}

	return n_cac_types;
}

/* List of radar channels for cac time of 60 seconds for edcrs compliant countries */
static int
wbd_slave_update_cac_capability_for_method_and_cac_time(i5_dm_interface_type *pdmif,
	uint8 *pbuf, uint16 *payload_len, uint32 is_edcrs_eu, bool weather, uint8 method)
{
	int ret = WBDE_OK;
	uint8 *pstart = NULL;
	ieee1905_cac_mode_info_t *mode_info = NULL;
	ieee1905_radio_caps_type *RadioCaps;
	uint8 rc_first, rc_last;
	uint16 bytes_to_rd;
	uint16 bytes_rd = 0;
	uint8 rclass_cnt;
	int rcaps_rc_count;
	uint8 *ptr = NULL;
	uint8 n_cac_support_opclass = 0;
	uint32 cac_time;
	const i5_dm_rc_chan_map_type *rc_chan_map;
	unsigned int reg_class_count = 0;

	WBD_ENTER();

	rc_chan_map = i5DmGetRCChannelMap(&reg_class_count);

	pstart = pbuf;

	mode_info = (ieee1905_cac_mode_info_t*)pbuf;
	/* fill mode, seconds, total opclass support for interface */
	mode_info->mode = method;

	/* number of seconds required to complete CAC : 3 bytes */
	/* TODO: add new IOVAR to get CAC time */
	if (is_edcrs_eu && weather) {
		cac_time = 600;
	} else {
		cac_time = 60;
	}
	memcpy((uint8*)mode_info->seconds, (uint8*)&(cac_time), 3);
	WBD_SWAP(mode_info->seconds[0], mode_info->seconds[2], uint8);
	pbuf += OFFSETOF(ieee1905_cac_mode_info_t, list);

	RadioCaps = &pdmif->ApCaps.RadioCaps;
	pdmif->band = ieee1905_get_band_from_radiocaps(RadioCaps);
	if (pdmif->band == BAND_5GL) {
		rc_first = REGCLASS_5GL_FIRST;
		/* 5GL opclasses are not continuous. 115 - 120 and 128 - 130.
		 * Handle 128 - 130 as special case
		 */
		rc_last = REGCLASS_5GL_40MHZ_LAST;
	} else if (pdmif->band == BAND_5GH) {
		rc_first = REGCLASS_5GH_FIRST;
		rc_last = REGCLASS_5GH_LAST;
	} else if (pdmif->band == (BAND_5GH | BAND_5GL)) {
		rc_first = REGCLASS_5G_FIRST;
		rc_last = REGCLASS_5G_LAST;
	} else {
		WBD_ERROR("Invalid band : %d\n", pdmif->band);
		goto end;
	}
	WBD_DEBUG("[%s]: Updating CAC capabilities TLV for band [%d] edcrs_eu [%d] weather [%d] "
		"method [%d]\n", pdmif->ifname, pdmif->band, is_edcrs_eu, weather, method);

	rcaps_rc_count = RadioCaps->List[0];
	ptr = &RadioCaps->List[1];
	bytes_to_rd = RadioCaps->Len;

	/* Go thorugh the complete list of regulatory classes and channels */
	for (rclass_cnt = 0; rclass_cnt < reg_class_count; rclass_cnt++) {
		uint8 chan_bitmap[(MAXCHANNEL + 7) / NBBY] = {0};
		uint8 chan_cnt;
		uint8 i;
		uint8 chan = 0;
		ieee1905_rc_chan_list_t *list = (ieee1905_rc_chan_list_t*)pbuf;

		/* Skip the out of band regulatory classes */
		if (rc_chan_map[rclass_cnt].regclass < rc_first ||
			rc_chan_map[rclass_cnt].regclass > rc_last) {

			if (pdmif->band == BAND_5GL &&
				rc_chan_map[rclass_cnt].regclass > REGCLASS_5G_40MHZ_LAST &&
				rc_chan_map[rclass_cnt].regclass <= REGCLASS_5G_LAST) {
				/* Don't skip it */
			} else {
				continue;
			}
		}

		/* exclude list of channels present in list from radio caps,
		 * these channels are being marked as non operable
		 */
		for (i = 0; (i < rcaps_rc_count) && (bytes_rd < bytes_to_rd); i++) {
			radio_cap_sub_info_t *radio_sub_info = (radio_cap_sub_info_t *)ptr;

			/* Mark the channel for skipping, if it is set invalid in radio caps */
			if (rc_chan_map[rclass_cnt].regclass == radio_sub_info->regclass) {
				for (chan_cnt = 0; chan_cnt < radio_sub_info->n_chan; chan_cnt++) {
					setbit(chan_bitmap, radio_sub_info->list_chan[chan_cnt]);
				}
				break;
			}
			/* read next oplcass from radio capability and update bytes_rd value */
			ptr += sizeof(radio_cap_sub_info_t) + radio_sub_info->n_chan;
			bytes_rd += sizeof(radio_cap_sub_info_t) + radio_sub_info->n_chan;
		}

		for (chan_cnt = 0; chan_cnt < rc_chan_map[rclass_cnt].count; chan_cnt++) {
			uint bitmap = 0;
			chanspec_t chanspec;
			uint8 channel;

			chan = rc_chan_map[rclass_cnt].channel[chan_cnt];
			if (isset(chan_bitmap, chan)) {
				WBD_DEBUG("skipping rclass [%d] channel [%d] since not supported "
					"in Radio Caps\n", rc_chan_map[rclass_cnt].regclass, chan);
				continue;
			}
			if (rc_chan_map[rclass_cnt].regclass > REGCLASS_5G_40MHZ_LAST &&
				(((pdmif->band == BAND_5GL) && (chan >= 100)) ||
				((pdmif->band == BAND_5GH) && (chan < 100)))) {
				WBD_DEBUG("skipping rclass [%d] channel [%d] since out of band\n",
					rc_chan_map[rclass_cnt].regclass, chan);
				continue;
			}
			chanspec = blanket_prepare_chanspec(chan,
				rc_chan_map[rclass_cnt].regclass, 0, 0);

			FOREACH_20_SB(chanspec, channel) {
				uint bitmap_sb = 0x00;
				ret = blanket_get_chan_info(pdmif->ifname, channel,
					CHSPEC_BAND(chanspec), &bitmap_sb);
				if (ret < 0) {
					WBD_ERROR("chan_info failed for chan %d\n", chan);
					break;
				}
				if (!(bitmap_sb & WL_CHAN_VALID_HW) ||
					!(bitmap_sb & WL_CHAN_VALID_SW)) {
					ret = -1;
					break;
				}
				bitmap |= bitmap_sb;
			}

			if (ret < 0 || !(bitmap & WL_CHAN_RADAR)) {
				WBD_DEBUG("skipping rclass [%d] channel [%d] bitmap: [0x%x] since "
					"it is not radar channel\n",
					rc_chan_map[rclass_cnt].regclass, chan, bitmap);
				continue;
			}

			if (is_edcrs_eu) {
				if (!weather && (bitmap & WL_CHAN_RADAR_EU_WEATHER)) {
					WBD_DEBUG("skipping rclass [%d] channel [%d] bitmap [0x%x] "
						"since it is weather channel\n",
						rc_chan_map[rclass_cnt].regclass, chan, bitmap);
					continue;
				}
				if (weather && !(bitmap & WL_CHAN_RADAR_EU_WEATHER)) {
					WBD_DEBUG("skipping rclass [%d] channel [%d] bitmap [0x%x] "
						"since it is NOT weather channel\n",
						rc_chan_map[rclass_cnt].regclass, chan, bitmap);
					continue;
				}
			}
			/* add this channel to list */
			list->chan[list->n_chan++] = chan;
			WBD_DEBUG("Added rclass [%d] channel [%d] to CAC capability TLV\n",
				rc_chan_map[rclass_cnt].regclass, chan);
		}
		if (list && list->n_chan) {
			/* update tmp buffer with chan and rclass */
			list->opclass = rc_chan_map[rclass_cnt].regclass;
			/* update total payload_len to be send to caller along with filled buffer */
			pbuf += sizeof(ieee1905_rc_chan_list_t) + list->n_chan;
			n_cac_support_opclass++;
		}
	}
	if (n_cac_support_opclass) {
		*payload_len = pbuf - pstart;
		mode_info->n_opclass = n_cac_support_opclass;
	}
	WBD_INFO("[%s]: Updated CAC capabilities TLV for band [%d] edcrs_eu [%d] weather [%d] "
		"method [%d] cac time [%d] with n_opclass [%d] and length [%d]\n",
		pdmif->ifname, pdmif->band, is_edcrs_eu, weather, method, cac_time,
		mode_info->n_opclass, *payload_len);

end:
	WBD_EXIT();
	return ret;
}

/* Prepare CAC completion TLV payload for all Radio received CAC request from controller */
void
wbd_slave_prepare_cac_complete_tlv_payload(uint8 **pbuf, uint32 *payload_len)
{
	i5_dm_device_type *sdev = NULL;
	i5_dm_interface_type *sifr = NULL;
	wbd_ifr_item_t *ifr_vndr_data;
	int ret = WBDE_OK;
	int len = 0;

	WBD_ENTER();

	WBD_ASSERT_ARG(*pbuf, WBDE_INV_ARG);

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);
	foreach_i5glist_item(sifr, i5_dm_interface_type, sdev->interface_list) {
		ieee1905_radio_cac_completion_t *cac_compl =
			(ieee1905_radio_cac_completion_t *)(*pbuf + 1 + len);

		ifr_vndr_data = (wbd_ifr_item_t*)sifr->vndr_data;
		if (!ifr_vndr_data) {
			WBD_ERROR(" Invalid ifr vendor data plz Debug, exiting ..\n");
			continue;
		}

		if (ifr_vndr_data->cac_cur_state != MAP_CAC_RQST_COMPLETE) {
			continue;
		}
		WBD_INFO("fill CAC complete details for ["MACF"] chanspec[0x%x]\n",
			ETHERP_TO_MACF(sifr->InterfaceId), ifr_vndr_data->cac_rqst_chspec);

		ifr_vndr_data->cac_cur_state = 0;

		(**pbuf)++; /* Number of radios */
		len += sizeof(ieee1905_radio_cac_completion_t);
		/* assuming all radios completed CAC */
		memcpy(&cac_compl->mac, sifr->InterfaceId, ETHER_ADDR_LEN);

		blanket_get_global_rclass(ifr_vndr_data->cac_rqst_chspec, &cac_compl->opclass);
		if (cac_compl->opclass <= REGCLASS_5G_40MHZ_LAST) {
			cac_compl->chan = wf_chspec_ctlchan(ifr_vndr_data->cac_rqst_chspec);
		} else {
			cac_compl->chan = ifr_vndr_data->cac_rqst_chspec & WL_CHANSPEC_CHAN_MASK;
		}
		cac_compl->status = ifr_vndr_data->cac_compl_status;

		if (ifr_vndr_data->cac_compl_status == MAP_CAC_STATUS_RADAR_DETECTED) {
			uint16 channel;
			int i = 0;
			uint band = CHSPEC_BAND(ifr_vndr_data->cac_rqst_chspec);

			FOREACH_20_SB(ifr_vndr_data->cac_rqst_chspec, channel) {
				uint bitmap_sb = 0x00;

				blanket_get_chan_info(sifr->ifname, channel, band, &bitmap_sb);
				if (!(bitmap_sb & WL_CHAN_RADAR)) {
					continue;
				}
				blanket_get_global_rclass(CH20MHZ_CHSPEC(channel, band),
					&cac_compl->list[i].op_class);
				cac_compl->list[i].chan = channel;
				i++;
			}
			cac_compl->n_opclass_chan = i;
			len += sizeof(opclass_chan_list) * i;
		}
	}
	if (len) {
		*payload_len = len + 1; /* 1 byte for count */
	}
	WBD_INFO("CAC completion TLV: length [%d]\n", *payload_len);
end:
	WBD_EXIT();
}

/* Prepare CAC status payload for all Radio received CAC request from
 * controller.
 */
void
wbd_slave_prepare_cac_status_payload(uint8 **pbuf, uint16 *payload_len)
{
	int ret = WBDE_OK;
	i5_dm_device_type *sdev = NULL;
	i5_dm_interface_type *sifr = NULL;
	uint8 *ptr = NULL;
	uint8 *radar_pbuf = NULL;
	uint16 radar_filled_len = 1; /* one for the count */
	int valid_chan_count;
	uint8 *active_chan_count;
	uint8 *radar_chan_count;
	uint8 *n_count;

	WBD_ENTER();

	active_chan_count = *pbuf;
	ptr = (*pbuf) + 1;

	WBD_SAFE_GET_I5_SELF_DEVICE(sdev, &ret);

	radar_pbuf = (uint8 *)wbd_malloc(WBD_MAX_BUF_1024, &ret);
	WBD_ASSERT_ARG(radar_pbuf, WBDE_INV_ARG);

	radar_chan_count = radar_pbuf;

	/* prepare available channels in agent */
	foreach_i5glist_item(sifr, i5_dm_interface_type, sdev->interface_list) {
		wbd_slave_valid_chan_info_list_t valid_chan_info[MAXCHANNEL];
		uint16 filled_len = 0;
		uint16 active_filled_len = 0;

		if (!i5DmIsInterfaceWireless(sifr->MediaType)) {
			continue;
		}
		valid_chan_count = wbd_slave_find_valid_oclass_chan_info_list(sifr,
			valid_chan_info);

		*active_chan_count += wbd_slave_prepare_active_list(sifr, ptr,
			&active_filled_len, valid_chan_info, valid_chan_count);
		ptr += active_filled_len;

		if (!WBD_BAND_TYPE_LAN_5G(sifr->band)) {
			continue;
		}
		*radar_chan_count += wbd_slave_prepare_radar_detected_chan_list(
			(radar_pbuf + radar_filled_len), &filled_len,
			valid_chan_info, valid_chan_count);
		radar_filled_len += filled_len;
	}

	WBD_INFO("Number of Active channels [%d] Number of radar channels [%d]\n",
		*active_chan_count, *radar_chan_count);

	if (*radar_chan_count) {
		memcpy(ptr, radar_pbuf, radar_filled_len);
	}
	ptr += radar_filled_len;

	/* prepare ongoing CAC channels in agent */
	n_count = ptr++;
	foreach_i5glist_item(sifr, i5_dm_interface_type, sdev->interface_list) {
		if (!i5DmIsInterfaceWireless(sifr->MediaType) ||
			!WBD_BAND_TYPE_LAN_5G(sifr->band)) {
			continue;
		}
		/* prepare list of available channels for all interfaces in agent */
		if (wbd_slave_prepare_ongoing_cac_chan_list(sifr, ptr) == WBDE_OK) {
			ptr += sizeof(ieee1905_ongoing_cac_opclass_chan_info_t);
			(*n_count)++;
		}
	}

	*payload_len = ptr - *pbuf;
end:
	if (radar_pbuf) {
		free(radar_pbuf);
	}
	WBD_INFO("CAC status: payload_len[%d] \n", *payload_len);
	WBD_EXIT();
}

/* Prepare valid opclass/channel list for the given radio */
static int
wbd_slave_find_valid_oclass_chan_info_list(i5_dm_interface_type *pdmif,
	wbd_slave_valid_chan_info_list_t *chan_info)
{
	int i, j, index = 0;
	i5_dm_rc_chan_map_type *g_rc_chan_map = NULL;
	uint32 g_rc_chan_map_count = 0;
	uint channel;
	chanspec_t chanspec;
	int ret = WBDE_OK;

	WBD_ENTER();

	g_rc_chan_map = i5DmGetRCChannelMap(&g_rc_chan_map_count);
	for (i = 0; i < g_rc_chan_map_count; i++) {
		if (!wbd_slave_opclass_valid_for_interface(pdmif, g_rc_chan_map[i].regclass)) {
			continue;
		}

		for (j = 0; j < g_rc_chan_map[i].count; j++) {

			if (!wbd_slave_chan_valid_for_interface(pdmif, g_rc_chan_map[i].regclass,
				g_rc_chan_map[i].channel[j])) {
				continue;
			}

			chanspec = blanket_prepare_chanspec(g_rc_chan_map[i].channel[j],
				g_rc_chan_map[i].regclass, 0, 0);

			chan_info[index].bitmap = WL_CHAN_INACTIVE;
			FOREACH_20_SB(chanspec, channel) {
				uint bitmap_sb = 0x00;
				ret = blanket_get_chan_info(pdmif->ifname, channel,
					CHSPEC_BAND(chanspec), &bitmap_sb);
				if (ret < 0) {
					WBD_ERROR("chan_info failed for chan %d\n", channel);
					break;
				}
				if (!(bitmap_sb & WL_CHAN_VALID_HW) ||
					!(bitmap_sb & WL_CHAN_VALID_SW)) {
					ret = -1;
					break;
				}
				/* Mark a channel as out of service only if all the sub-bands are
				 * radar detected. CSP:CS00012224611 */
				if ((chan_info[index].bitmap & WL_CHAN_INACTIVE) &&
					!(bitmap_sb & WL_CHAN_INACTIVE)) {
					chan_info[index].bitmap &= (~WL_CHAN_INACTIVE);
				}
				chan_info[index].bitmap |= (bitmap_sb & (~WL_CHAN_INACTIVE));
			}
			if (ret < 0) {
				continue;
			}
			chan_info[index].opclass = g_rc_chan_map[i].regclass;
			chan_info[index].channel = g_rc_chan_map[i].channel[j];
			WBD_DEBUG("[%d]: opclass[%d] chan[%d] bitmap [0x%x] is valid for [%s]\n",
				index, chan_info[index].opclass, chan_info[index].channel,
				chan_info[index].bitmap, pdmif->ifname);
			index++;

		}
	}
	return index;
}

/* Create active channels(done CAC) list */
static uint8
wbd_slave_prepare_active_list(i5_dm_interface_type *pdmif, uint8 *pbuf, uint16 *len,
	wbd_slave_valid_chan_info_list_t *valid_chan_info, int valid_chan_count)
{
	ieee1905_opclass_chan_info_t *chan_info = NULL;
	int i = 0;
	uint8 index = 0;

	WBD_ENTER();

	chan_info = (ieee1905_opclass_chan_info_t*)pbuf;

	for (i = 0; i < valid_chan_count; i++) {
		if (!(valid_chan_info[i].bitmap & WL_CHAN_RADAR)) {
			chan_info[index].opclass = valid_chan_info[i].opclass;
			chan_info[index].chan = valid_chan_info[i].channel;
			index++;
		}
	}

	for (i = 0; i < valid_chan_count; i++) {
		if ((valid_chan_info[i].bitmap & WL_CHAN_RADAR) &&
			(!(valid_chan_info[i].bitmap & WL_CHAN_INACTIVE)) &&
			(!(valid_chan_info[i].bitmap & WL_CHAN_PASSIVE))) {

			chan_info[index].opclass = valid_chan_info[i].opclass;
			chan_info[index].chan = valid_chan_info[i].channel;
			chan_info[index].duration = htons(0); //TODO: get active time
			index++;
		}
	}

	*len = (index * sizeof(ieee1905_opclass_chan_info_t));

	WBD_DEBUG("CAC status available chan list : n_count[%d] len[%d] \n", index, *len);

	WBD_EXIT();
	return index;
}

/* Create radar detected chan list */
static int
wbd_slave_prepare_radar_detected_chan_list(uint8 *pbuf, uint16 *len,
	wbd_slave_valid_chan_info_list_t *valid_chan_info, int valid_chan_count)
{
	ieee1905_opclass_chan_info_t *chan_info = NULL;
	int i = 0;
	uint8 index = 0;

	WBD_ENTER();

	chan_info = (ieee1905_opclass_chan_info_t*)pbuf;

	for (i = 0; i < valid_chan_count; i++) {
		if ((valid_chan_info[i].bitmap & WL_CHAN_RADAR) &&
			(valid_chan_info[i].bitmap & WL_CHAN_INACTIVE)) {
			chan_info[index].opclass = valid_chan_info[i].opclass;
			chan_info[index].chan = valid_chan_info[i].channel;
			chan_info[index].duration =
				htons(((valid_chan_info[i].bitmap >> 24) & 0xff) * 60);
			WBD_DEBUG("[%d]: opclass [%d] channel [%d] duration  [%d] seconds\n",
				index, chan_info[index].opclass, chan_info[index].chan,
				chan_info[index].duration);
			index++;
		}
	}
	*len = (index * sizeof(ieee1905_opclass_chan_info_t));

	WBD_DEBUG("CAC status available chan list : n_count[%d] len[%d] \n", index, *len);

	WBD_EXIT();
	return index;
}

/* Create ongoing CAC chan list */
static int
wbd_slave_prepare_ongoing_cac_chan_list(i5_dm_interface_type *pdmif, uint8 *pbuf)
{
	ieee1905_ongoing_cac_opclass_chan_info_t *chan_info = NULL;
	wl_dfs_status_all_t *dfs_status_all = NULL;
	uint8 sub_channel;
	uint32 cac_time = 60;
	uint32 is_edcrs_eu = 0;
	wl_dfs_sub_status_t *sub = NULL;
	int count;
	int ret = WBDE_OK;

	WBD_ENTER();
	dfs_status_all = (wl_dfs_status_all_t *)wbd_malloc(WBD_MAX_BUF_128, &ret);
	WBD_ASSERT_ARG(dfs_status_all, WBDE_INV_ARG);
	blanket_get_dfs_status_all(pdmif->ifname, dfs_status_all, WBD_MAX_BUF_128);

	for (count = 0; count < dfs_status_all->num_sub_status; ++count) {
		sub = &dfs_status_all->dfs_sub_status[count];

		if (sub->state == WL_DFS_CACSTATE_PREISM_CAC) {
			break;
		}
	}
	if (!sub || (sub->state != WL_DFS_CACSTATE_PREISM_CAC)) {
		WBD_DEBUG("No CAC ongoing on interface [%s]\n", pdmif->ifname);
		ret = -1;
		goto end;
	}

	blanket_get_is_edcrs_eu(pdmif->ifname, &is_edcrs_eu);
	if (is_edcrs_eu) {
		FOREACH_20_SB(sub->chanspec, sub_channel) {
			uint bitmap_sb = 0x00;
			blanket_get_chan_info(pdmif->ifname, sub_channel,
				CHSPEC_BAND(sub->chanspec), &bitmap_sb);
			if (bitmap_sb & WL_CHAN_RADAR_EU_WEATHER) {
				cac_time = 600;
				break;
			}
		}
	}

	chan_info = (ieee1905_ongoing_cac_opclass_chan_info_t *)pbuf;
	blanket_get_global_rclass(sub->chanspec, &chan_info->opclass);
	if (chan_info->opclass > REGCLASS_5G_40MHZ_LAST) {
		chan_info->chan = CHSPEC_CHANNEL(sub->chanspec);
	} else {
		chan_info->chan = wf_chspec_ctlchan(sub->chanspec);
	}

	cac_time -= (sub->duration / 1000);
	memcpy(chan_info->duration, (uint8*)&(cac_time), 3);
	WBD_SWAP(chan_info->duration[0], chan_info->duration[2], uint8);

	WBD_INFO("Ongoing CAC on opclass [%d] channel [%d] cac_time remaining [%d] seconds\n",
		chan_info->opclass, chan_info->chan, cac_time);
end:
	if (dfs_status_all) {
		free(dfs_status_all);
	}
	WBD_EXIT();
	return ret;
}

/* check opclass validity from radio capability of interface */
static bool
wbd_slave_opclass_valid_for_interface(i5_dm_interface_type *pdmif, uint8 opclass)
{

	ieee1905_radio_caps_type *RadioCaps = &pdmif->ApCaps.RadioCaps;
	radio_cap_sub_info_t *radio_sub_info = NULL;
	int rcaps_rc_count = RadioCaps->List ? RadioCaps->List[0] : 0;
	uint8 *ptr = RadioCaps->List;
	uint16 bytes_to_rd;
	uint16 bytes_rd = 0;
	uint16 i = 0;

	WBD_ENTER();

	WBD_DEBUG("total valid opclass[%d] for interface["MACF"] \n", *ptr,
		ETHERP_TO_MACF(pdmif->InterfaceId));

	bytes_to_rd = RadioCaps->Len;
	ptr++; /* skip number of opclass info */

	for (i = 0; (i < rcaps_rc_count) && (bytes_rd < bytes_to_rd); i++) {
		radio_sub_info = (radio_cap_sub_info_t *)ptr;
		if (radio_sub_info->regclass == opclass) {
			return TRUE;
		}
		/* read next oplcass from radio capability and update bytes_rd value */
		ptr += sizeof(radio_cap_sub_info_t) + radio_sub_info->n_chan;
		bytes_rd += sizeof(radio_cap_sub_info_t) + radio_sub_info->n_chan;
	}
	return FALSE;
}

/* check chan validity from radio capability of interface */
static bool
wbd_slave_chan_valid_for_interface(i5_dm_interface_type *pdmif, uint8 opclass, uint8 chan)
{
	ieee1905_radio_caps_type *RadioCaps = &pdmif->ApCaps.RadioCaps;
	radio_cap_sub_info_t *radio_sub_info = NULL;
	int rcaps_rc_count = RadioCaps->List ? RadioCaps->List[0] : 0;
	uint16 bytes_to_rd;
	uint16 bytes_rd = 0;
	uint16 i = 0;
	uint8 chan_cnt = 0;
	uint8 *ptr = RadioCaps->List;

	WBD_ENTER();

	WBD_DEBUG("total valid opclass[%d] for interface["MACF"] \n", *ptr,
		ETHERP_TO_MACF(pdmif->InterfaceId));

	bytes_to_rd = RadioCaps->Len;
	ptr++; /* skip number of opclass info */

	for (i = 0; (i < rcaps_rc_count) && (bytes_rd < bytes_to_rd); i++) {
		radio_sub_info = (radio_cap_sub_info_t *)ptr;
		if (radio_sub_info->regclass != opclass) {
			/* read next oplcass from radio capability and update bytes_rd value */
			ptr += sizeof(radio_cap_sub_info_t) + radio_sub_info->n_chan;
			bytes_rd += sizeof(radio_cap_sub_info_t) + radio_sub_info->n_chan;
			continue;
		}

		for (chan_cnt = 0; chan_cnt < radio_sub_info->n_chan; chan_cnt++) {
			if (radio_sub_info->list_chan[chan_cnt] == chan) {
				/* radio caps contains non operable chan in operable
				 * opclass.
				 */
				return FALSE;
			}
		}
		/* found valid opclas and chan not present in list, means valid chan */
		return TRUE;
	}
	return FALSE;
}

/* Process the Backhaul BSS config policy. Return 0 If there is any change in the policy So that
 * the caller can restart
 */
int
wbd_slave_process_backhaulbss_config_policy(ieee1905_policy_config *policy)
{
	int ret = -1, bssret = -1;
	dll_t *item_p;
	ieee1905_bh_bss_list *temp_bhbss = NULL;
	char strmap[WBD_MAX_BUF_16];
	i5_dm_device_type *device;
	i5_dm_bss_type *bss;
	WBD_ENTER();

	if (!policy) {
		goto end;
	}

	if ((device = i5DmGetSelfDevice()) == NULL) {
		ret = WBDE_DS_UN_DEV;
		goto end;
	}

	/* Travese Backhaul BSS Configuration Policy List items to set nvram for map */
	foreach_glist_item(item_p, policy->no_bh_bss_list) {
		unsigned char update_mapflag = 0;
		char prefix[IFNAMSIZ];

		/* Get Backhaul BSS and BSTA map flag from IEEE1905 */
		temp_bhbss = (ieee1905_bh_bss_list*)item_p;

		if (temp_bhbss->bsta_flag & I5_TLV_BH_BSS_CONFIG_P1_BSTA_DISALLOWED) {
			update_mapflag |= IEEE1905_MAP_FLAG_PROF1_DISALLOWED;
		}
		if (temp_bhbss->bsta_flag & I5_TLV_BH_BSS_CONFIG_P2_BSTA_DISALLOWED) {
			update_mapflag |= IEEE1905_MAP_FLAG_PROF2_DISALLOWED;
		}

		/* Get BSS in the device using BSSID for checking mapFlags */
		bss = wbd_ds_get_i5_bss_in_device(device, temp_bhbss->bssid, &bssret);

		if (bssret != WBDE_OK) {
			continue;
		}

		if ((bss->mapFlags & update_mapflag) == update_mapflag) {
			continue;
		}

		WBD_DEBUG("BSSID:["MACF"] mapFlags [0x%x] ifname[%s] "
			"update_mapflag [0x%x]\n", MAC2STRDBG(bss->BSSID), bss->mapFlags,
			bss->ifname, update_mapflag);

		/* Get prefix of the interface from Driver */
		blanket_get_interface_prefix(bss->ifname, prefix, sizeof(prefix));

		/* mapFlag in BSS is different from update_mapflag;
		* set nvram map for this bss and do rc restart
		*/
		update_mapflag |= bss->mapFlags;
		snprintf(strmap, sizeof(strmap), "%d", update_mapflag);

		blanket_nvram_prefix_set(prefix, NVRAM_MAP, strmap);

		WBD_INFO("Set %s_%s to 0x%02x\n", prefix, NVRAM_MAP,
			update_mapflag);

		ret = 0;
	}
end:
	WBD_EXIT();
	return ret;
}

/* Process the Traffic Separation config policy. Return 1 If there is any error in the policy set
 * So that error message can be sent from ieee1905
 */
int
wbd_slave_check_for_ts_policy_errors(ieee1905_policy_config *policy)
{
	int ret = -1;
	ieee1905_err_code_t *err_resp = NULL;
	i5_dm_device_type *device;
	i5_dm_interface_type *pdmif;
	char ifname[256], *wbd_ifnames, *next_intf = NULL;
	WBD_ENTER();

	if (!policy) {
		goto end;
	}

	if ((device = i5DmGetSelfDevice()) == NULL) {
		ret = WBDE_DS_UN_DEV;
		goto end;
	}

	/* Clean up before filling the latest data. */
	i5DmGlistCleanup(&policy->no_err_code_list);
	ieee1905_glist_init(&policy->no_err_code_list);

	/* if no primary VLAN ID received;
	 * set Profile 2 Error Message
	 */
	if (policy->prim_vlan_id == 0) {

		err_resp = i5DmAddErrorCodeToList(policy, map_p2_err_pcp_or_vlanid_not_provided,
				NULL);
		if (err_resp == NULL) {
			WBD_WARNING("Failed to add Error Code [0x%x] to list\n",
				map_p2_err_pcp_or_vlanid_not_provided);
			goto end;
		}
	}

	/* if VLAN ID received is greater than MAXTotalVIDs available;
	 * set Profile 2 Error Message
	 */
	if (policy->ts_policy_list.count > device->p2ApCap.max_vids) {

		err_resp = i5DmAddErrorCodeToList(policy,
				map_p2_err_unique_vlanid_max_supported, NULL);
		if (err_resp == NULL) {
			WBD_WARNING("Failed to add Error Code [0x%x] to list\n",
				map_p2_err_unique_vlanid_max_supported);
			goto end;
		}
	}

	/* Traverse wbd_ifnames for each ifname to check TS policy */
	wbd_ifnames = blanket_nvram_safe_get(WBD_NVRAM_IFNAMES);
	if (strlen(wbd_ifnames) <= 0) {
		WBD_INFO("NVRAM[%s] Not set. So, error code for traffic separation policy "
			"is not set\n", WBD_NVRAM_IFNAMES);
		goto end;
	}
	foreach(ifname, wbd_ifnames, next_intf) {

		char prefix[IFNAMSIZ], radio_prefix[IFNAMSIZ];
		struct ether_addr bssid = {0x00}, radio_mac = {0x00};
		unsigned char map = 0;

		/* Get primary prefix */
		blanket_get_radio_prefix(ifname, radio_prefix, sizeof(radio_prefix));

		/* Get the interface MAC address from primary prefix */
		wbd_ether_atoe(blanket_nvram_prefix_safe_get(radio_prefix, NVRAM_HWADDR),
			&radio_mac);

		/* Get the interface from self device */
		pdmif = wbd_ds_get_i5_ifr_in_device(device, (unsigned char *)&radio_mac, NULL);

		/* If interface info is NULL continue to get next BSS */
		if (pdmif == NULL) {
			WBD_ERROR("Interface [" MACF "] does not exists. ifname[%s] "
				"primary prefix [%s]\n", ETHER_TO_MACF(radio_mac), ifname,
				radio_prefix);
			continue;
		}

		/* If TS interface policy set with mix of FH with Prof1 BH Support
		 * and mix of BH with Prof1 support and Prof2 support; continue
		 */
		if (I5_IS_TS_MIX_FH_P1BH_SUPPORTED(pdmif->flags) &&
			I5_IS_TS_MIX_P1BH_P2BH_SUPPORTED(pdmif->flags)) {
			continue;
		}

		/* Get prefix of the interface from Driver */
		blanket_get_interface_prefix(ifname, prefix, sizeof(prefix));

		/* Get IEEE1905_MAP_FLAG_XXX flag value for this BSS */
		map = wbd_get_map_flags(prefix);

		/* Traffic Separation on combined fronthaul and Profile-1 backhaul unsupported */
		if (!I5_IS_TS_MIX_FH_P1BH_SUPPORTED(pdmif->flags) &&
			I5_IS_BSS_FRONTHAUL(map) && I5_IS_BSS_BACKHAUL(map) &&
			!I5_IS_BSS_PROF1_DISALLOWED(map)) {

			/* Get the BSSID/MAC of the BSS */
			wbd_ether_atoe(blanket_nvram_prefix_safe_get(prefix, NVRAM_HWADDR),
				&bssid);

			err_resp = i5DmAddErrorCodeToList(policy,
				map_p2_err_ts_fh_prof1_not_supported,
				(unsigned char *)&bssid);
			if (err_resp == NULL) {
				WBD_WARNING("Failed to add Error Code [0x%x] to list\n",
					map_p2_err_ts_fh_prof1_not_supported);
				goto end;
			}
		}

		/* Traffic Separation on combined Profile-1 backhaul and Profile-2
		 * backhaul unsupported
		 */
		if (!I5_IS_TS_MIX_P1BH_P2BH_SUPPORTED(pdmif->flags) &&
			I5_IS_BSS_BACKHAUL(map) && !I5_IS_BSS_PROF1_DISALLOWED(map) &&
			!I5_IS_BSS_PROF2_DISALLOWED(map)) {

			/* Get the BSSID/MAC of the BSS */
			wbd_ether_atoe(blanket_nvram_prefix_safe_get(prefix, NVRAM_HWADDR),
				&bssid);

			err_resp = i5DmAddErrorCodeToList(policy,
				map_p2_err_ts_prof1_prof2_not_supported,
				(unsigned char *)&bssid);
			if (err_resp == NULL) {
				WBD_WARNING("Failed to add Error Code [0x%x] to list\n",
					map_p2_err_ts_prof1_prof2_not_supported);
				goto end;
			}
		}
	}
end:
	if (policy->no_err_code_list.count > 0) {
		ret = 1;
	}
	WBD_EXIT();
	return ret;
}
#endif /* MULTIAPR2 */
