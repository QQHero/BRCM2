/*
 * WBD daemon (Linux)
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
 * $Id: wbd_slave.c 809077 2022-03-07 11:43:08Z $
 */

#include <getopt.h>

#include "wbd.h"
#include "wbd_shared.h"
#include "wbd_ds.h"
#include "wbd_com.h"
#include "wbd_json_utility.h"
#include "wbd_sock_utility.h"
#include "wbd_wl_utility.h"
#include "wbd_blanket_utility.h"
#include "wbd_plc_utility.h"
#include "wbd_slave_control.h"
#include "wbd_slave_com_hdlr.h"
#include "blanket.h"
#include <common_utils.h>
#ifdef BCM_APPEVENTD
#include "wbd_appeventd.h"
#include "appeventd_wbd.h"
#endif /* BCM_APPEVENTD */
#include "wbd_slave_vndr_brcm.h"

wbd_info_t *g_wbdinfo = NULL;
unsigned int wbd_msglevel = WBD_DEBUG_DEFAULT;
char g_wbd_process_name[BKT_MAX_PROCESS_NAME];

/* Read "wbd_ifnames" NVRAM and get actual ifnames */
extern int wbd_read_actual_ifnames(char *wbd_ifnames1, int len1, bool create);
/* Find First DWDS Primary Interface, with mode = STA */
extern int wbd_find_dwds_sta_primif(char *ifname, int len, char *ifname1, int len1);

/* Get Slave module info */
wbd_info_t *
wbd_get_ginfo()
{
	return (g_wbdinfo);
}

/* Gets Slave NVRAM settings */
static int
wbd_slave_retrieve_nvram_config(wbd_slave_item_t *slave)
{
	char *str, *prefix;
	int ret = WBDE_OK, num = 0;
	wbd_wc_thld_t threshold_cfg;
	WBD_ENTER();

	prefix = slave->wbd_ifr.prefix;
	slave->wc_info.wc_algo = blanket_get_config_val_int(prefix, WBD_NVRAM_WC_ALGO, 0);
	if (slave->wc_info.wc_algo >= wbd_get_max_wc_algo())
		slave->wc_info.wc_algo = 0;

	/* Read threshold index and prepare threshold config from predefined entries */
	slave->wc_info.wc_thld = blanket_get_config_val_int(prefix, WBD_NVRAM_WC_THLD_IDX, 0);
	if (slave->wc_info.wc_thld >= wbd_get_max_wc_thld())
		slave->wc_info.wc_thld = 0;
	memcpy(&slave->wc_info.wc_thld_cfg, wbd_get_wc_thld(slave),
		sizeof(slave->wc_info.wc_thld_cfg));

	/* read threshold from NVRAM */
	str = blanket_nvram_prefix_safe_get(prefix, WBD_NVRAM_WC_THLD);
	if (str) {
		num = sscanf(str, "%d %f %x",
			&threshold_cfg.t_rssi,
			&threshold_cfg.tx_rate,
			&threshold_cfg.flags);
		if (num == 3) {
			memcpy(&slave->wc_info.wc_thld_cfg, &threshold_cfg,
				sizeof(slave->wc_info.wc_thld_cfg));
		} else {
			WBD_WARNING("%s : %s%s = %s\n", wbderrorstr(WBDE_INV_NVVAL),
				prefix, WBD_NVRAM_WC_THLD, str);
		}
		blanket_log_default_nvram("%s=%d %f %x\n", WBD_NVRAM_WC_THLD, threshold_cfg.t_rssi,
			threshold_cfg.tx_rate, threshold_cfg.flags);
	}

	if (blanket_get_config_val_int(NULL, NVRAM_MAP_STA_WNM_CHECK, WBD_DEF_STA_WNM_CHECK)) {
		slave->parent->parent->flags |= WBD_INFO_FLAGS_STA_WNM_CHECK;
	}

	if (blanket_get_config_val_int(NULL, NVRAM_MAP_PER_CHAN_BCN_REQ,
		WBD_DEF_PER_CHAN_BCN_REQ)) {
		slave->parent->parent->flags |= WBD_INFO_FLAGS_PER_CHAN_BCN_REQ;
	}

	/* Read weak sta policy and steer flags nvram */
	str = blanket_nvram_prefix_safe_get(slave->wbd_ifr.primary_prefix, WBD_NVRAM_WEAK_STA_CFG);
	if (str[0] != '\0') {
		num = sscanf(str, "%d %d %d %d %d %x", &slave->weak_sta_policy.t_idle_rate,
			&slave->weak_sta_policy.t_rssi, &slave->weak_sta_policy.t_hysterisis,
			&slave->weak_sta_policy.t_tx_rate, &slave->weak_sta_policy.t_tx_failures,
			&slave->weak_sta_policy.flags);
	}
	if (num != 6) {
		/* In case of failure set default value */
		slave->weak_sta_policy.t_idle_rate = WBD_STA_METRICS_REPORTING_IDLE_RATE_THLD;
		slave->weak_sta_policy.t_rssi = WBD_STA_METRICS_REPORTING_RSSI_THLD;
		slave->weak_sta_policy.t_hysterisis =
			WBD_STA_METRICS_REPORTING_RSSI_HYSTERISIS_MARGIN;
		slave->weak_sta_policy.t_tx_rate = WBD_STA_METRICS_REPORTING_TX_RATE_THLD;
		slave->weak_sta_policy.t_tx_failures = WBD_STA_METRICS_REPORTING_TX_FAIL_THLD;
		slave->weak_sta_policy.flags = WBD_WEAK_STA_POLICY_FLAG_RSSI;
	}
	blanket_log_default_nvram("%s=%d %d %d %d %d %x\n", WBD_NVRAM_WEAK_STA_CFG,
		slave->weak_sta_policy.t_idle_rate, slave->weak_sta_policy.t_rssi,
		slave->weak_sta_policy.t_hysterisis, slave->weak_sta_policy.t_tx_rate,
		slave->weak_sta_policy.t_tx_failures, slave->weak_sta_policy.flags);

	slave->steer_flags = blanket_get_config_val_int(NULL,
		WLIFU_NVRAM_STEER_FLAGS, WLIFU_DEF_STEER_FLAGS);

	WBD_DEBUG("ifname[%s] prefix[%s] weak_sta_algo[%d] weak_sta_threshold[%d] %s rssi[%d] "
		"tx_rate[%f] flags[0x%X] slave_flags[0x%X] BSSID_Info[0x%x] steer_flags[0x%x]\n",
		slave->wbd_ifr.ifr.ifr_name, prefix, slave->wc_info.wc_algo,
		slave->wc_info.wc_thld, WBD_NVRAM_WC_THLD, slave->wc_info.wc_thld_cfg.t_rssi,
		slave->wc_info.wc_thld_cfg.tx_rate, slave->wc_info.wc_thld_cfg.flags,
		slave->flags, slave->wbd_ifr.bssid_info, slave->steer_flags);

	WBD_EXIT();
	return ret;
}

/* Based on info mode & uplink connectivity, Slave decides type common to all Slaves on a device */
static int
wbd_slave_identify_slave_type(wbd_info_t *info, wbd_slave_type_t *out_slave_type)
{
	int ret = WBDE_OK;
	char name[IFNAMSIZ] = {0}, name1[IFNAMSIZ] = {0};
	WBD_ENTER();

	/* Get the slave type */
	/* TODO : We just added only for ETHERNET, DWDS and PLC slaves. Needs to be
	 * done for MOCA interfaces
	 */

	/* Check if WBD Application Mode is Master */
	if (MAP_IS_CONTROLLER(info->map_mode)) {

		/* Slave Type is Master (PMSlave - Slave Running in Master) */
		*out_slave_type = WBD_SLAVE_TYPE_MASTER;
		goto end;
	}

	/* Find First DWDS Primary Interface, with mode = STA */
	ret = wbd_find_dwds_sta_primif(name, sizeof(name), name1, sizeof(name1));
	if (ret == WBDE_OK) {

		/* Slave Type is DWDS */
		*out_slave_type = WBD_SLAVE_TYPE_DWDS;
		goto end;
	}

#ifdef PLC_WBD
	/* Check if PLC is enabled */
	if (wbd_plc_is_enabled(&info->plc_info)) {

		/* Slave Type is PLC */
		*out_slave_type = WBD_SLAVE_TYPE_PLC;
		goto end;
	}
#endif /* PLC_WBD */

	/* Slave Type is Ethernet, By Default */
	*out_slave_type = WBD_SLAVE_TYPE_ETHERNET;

end:
	WBD_INFO("Slave Type[%d]\n", *out_slave_type);
	WBD_EXIT();
	return ret;
}

/* Update the non MAP BSS count on the vendor data section of IEEE1905's interface structure */
static void
wbd_slave_increment_nonmap_bss_count(char *ifname)
{
	int ret = WBDE_OK;
	unsigned char mapFlags;
	char prefix[IFNAMSIZ], primary_prefix[IFNAMSIZ] = {0};
	struct ether_addr radio_mac;
	i5_dm_interface_type *i5_ifr;
	WBD_ENTER();

	/* Get Prefix from OS specific interface name */
	blanket_get_interface_prefix(ifname, prefix, sizeof(prefix));
	mapFlags = wbd_get_map_flags(prefix);
	/* Do not count STA interface as BSS */
	if (I5_IS_BSS_STA(mapFlags) ||
		(strcmp(blanket_nvram_prefix_safe_get(prefix, "mode"), "sta") == 0)) {
		WBD_DEBUG("ifname[%s] prefix[%s] mapFlags[0x%x] is STA\n",
		ifname, prefix, mapFlags);
		goto end;
	}

	/* get the primary prefix info */
	ret = blanket_get_radio_prefix(ifname, primary_prefix, sizeof(primary_prefix));
	WBD_ASSERT();

	/* Skip primary interface with Disabled BSS. The primary ifname with BSS disabled will be
	 * present in lan_ifnames
	 */
	if ((strcmp(prefix, primary_prefix) == 0) &&
		(blanket_get_config_val_int(prefix, NVRAM_BSS_ENABLED, 0) == 0)) {
		WBD_DEBUG("ifname[%s] prefix[%s] primary_prefix[%s]. Skip Primary interface "
			"with BSS disbaled\n", ifname, prefix, primary_prefix);
		goto end;
	}

	/* Get the primary radio interface MAC address */
	blanket_get_radio_mac(primary_prefix, &radio_mac);

	i5_ifr = i5DmInterfaceFind(i5DmGetSelfDevice(), (unsigned char*)&radio_mac);
	if (i5_ifr == NULL) {
		WBD_INFO("IFR["MACF"] Not Found\n", ETHER_TO_MACF(radio_mac));
		goto end;
	}

	WBD_ASSERT_ARG(i5_ifr->vndr_data, WBDE_INV_ARG);

	((wbd_ifr_item_t*)i5_ifr->vndr_data)->non_map_bss_count++;
	WBD_INFO("ifname[%s] PrimaryPrifix[%s] RadioMAC["MACF"] non_map_bss_count[%d]\n",
		ifname, primary_prefix, ETHER_TO_MACF(radio_mac),
		((wbd_ifr_item_t*)i5_ifr->vndr_data)->non_map_bss_count);

end:
	WBD_EXIT();
}

#define BKT_SCAN_HOME_TIME_LT128 60 /* ms */

/* Fm driver, Fill Slave's Interface info to local ds */
static int
wbd_slave_fill_interface_info(wbd_info_t* info)
{
	int ret = WBDE_OK, in_band = WBD_BAND_LAN_2G;
	wbd_slave_item_t *slave = NULL;
	char wbd_ifnames[NVRAM_MAX_VALUE_LEN] = {0};
	char *ifnames_list = NULL, *non_map_ifnames = NULL;
	char name[IFNAMSIZ] = {0}, var_intf[IFNAMSIZ] = {0}, *next_intf;
	wbd_slave_type_t slave_type;
	char *appname = "wbd";
	wlc_rev_info_t rev = {0};

	WBD_ENTER();

	/* Based on info mode & uplink connectivity, Slave decides type common to all Slaves */
	wbd_slave_identify_slave_type(info, &slave_type);

	/* Read "wbd_ifnames" NVRAM, get actual ifnames */
	wbd_read_actual_ifnames(wbd_ifnames, sizeof(wbd_ifnames), TRUE);

	/* read the non MAP ifnames to be retained as it is. This has the list of ifnames to be
	 * managed outside of smartmesh
	 */
	non_map_ifnames = blanket_nvram_safe_get(NVRAM_NON_MAP_IFNAMES);

	if ((ifnames_list = ieee1905_get_all_lanifnames_list()) == NULL) {
		WBD_ERROR("lanX_ifnames are empty\n");
		goto end;
	}
	WBD_INFO("Actual wbd_ifnames[%s] lan_ifnames[%s] List if ifnames managed outside of "
		"SmartMesh[%s]\n", wbd_ifnames, ifnames_list, non_map_ifnames);

	/* Traverse ifnames list */
	foreach(var_intf, ifnames_list, next_intf) {

		/* Copy interface name temporarily */
		WBDSTRNCPY(name, var_intf, sizeof(name) - 1);

		/* Check if valid Wireless Interface and its Primary Radio is enabled or not */
		if (!blanket_is_interface_enabled(name, TRUE, &ret)) {
			continue; /* Skip non-Wireless & disabled Wireless Interface */
		}

		/* Do not consider this BSS if this ifname is present in the non map BSS
		 * ifname list. Means, do not add it to nonmap BSS(which we should delete) and
		 * do not create slave item for it
		 */
		if (find_in_list(non_map_ifnames, name)) {
			WBD_INFO("ifname %s is in the list of ifnames managed outside of "
				"SmartMesh [%s]. Do not process it.\n",
				name, non_map_ifnames);
			continue;
		}

		/* Set scan_home_time for all radio */
		if (!blanket_is_interface_virtual(name)) {

			/* For 4365 with default scan_home_time 45ms beacon loss
			 * are observed during scan.
			 * For that validated corerev for each interface and
			 * set increased default scan_home_time as 60ms.
			 */
			blanket_get_revinfo(name, &rev);
			if (rev.corerev < 128) {
				blanket_set_scan_home_time(name, BKT_SCAN_HOME_TIME_LT128);
			}
		}

		/* Add only the BSS which is in WBD ifnames(MAP BSS). For non MAP BSS just update
		 * the count
		 */
		if (!find_in_list(wbd_ifnames, name)) {
			wbd_slave_increment_nonmap_bss_count(name);
			continue;
		}

		/* Allocate & Initialize Slave Info struct for this Band, add to br0_slave_list */
		ret = wbd_ds_create_slave_for_band(info->wbd_slave, in_band, &slave);
		if (ret != WBDE_OK) {
			continue; /* Skip Slave creation for Invalid Interface */
		}

		/* Copy interface name to WBD Interface Info */
		WBDSTRNCPY(slave->wbd_ifr.ifr.ifr_name, name, IFNAMSIZ - 1);

		/* Initialize wlif module handle. */
		slave->wlif_hdl = wl_wlif_init(info->hdl, slave->wbd_ifr.ifr.ifr_name,
			NULL, NULL, appname);
		/* Assign common Slave Type and Uplink MAC to all Slaves on this device */
		slave->slave_type = slave_type;
	}

end:
	WBD_EXIT();

	/*
	 * This function can return WBDE_OK always. Even if wbd_wl_fill_interface_info() fails
	 * here, wbd_init_slave_item_timer_cb() function takes care of filling it at a later point
	 */
	return WBDE_OK;
}

/* This is to fill the slave item for Profile 1/2 disallowed case BHSTA not associated */
void
wbd_slave_linkup_fill_interface_info(wbd_slave_item_t *slave)
{
	int ret = WBDE_OK;
	unsigned char map;
	char *name;
	i5_dm_device_type *self_device;

	WBD_ENTER();

	if (!slave) {
		WBD_WARNING("Invalid slave\n");
		goto end;
	}

	name = slave->wbd_ifr.ifr.ifr_name;
	if (!slave->wbd_ifr.enabled &&
		(ret = wbd_wl_fill_interface_info(&slave->wbd_ifr)) != WBDE_OK) {
		WBD_DEBUG("Slave not enabled and fill interface failed for interface [%s]. \n",
			name);
		goto end;
	}

	/* Gets NVRAM settings for this Slave */
	wbd_slave_retrieve_nvram_config(slave);

	/* Init sta traffic info for slave, if present */
	if (!wbd_slave_check_taf_enable(slave)) {
		WBD_DEBUG("NVRAM taf_enable is not enable \n");
	}
	/* Initialize stamon module for Slave's interface */
	ret = wbd_ds_slave_stamon_init(slave);
	WBD_ASSERT();

	/* Get MAP */
	map = wbd_get_map_flags(slave->wbd_ifr.prefix);
	WBD_INFO("map: %d prefix: [%s]\n", map, slave->wbd_ifr.prefix);

	/* Add this BSS to the multiAP library */
	ieee1905_add_bss((unsigned char *)&slave->wbd_ifr.radio_mac,
		(unsigned char *)&slave->wbd_ifr.bssid,
		slave->wbd_ifr.blanket_ssid.SSID, slave->wbd_ifr.blanket_ssid.SSID_len,
		slave->wbd_ifr.chanspec, name, map);

	/* On ethernet agents the autoconfig resp msg gets received before link up event.
	 * Which causes the backhaul bss to remain with incorrect macmode settings.
	 * Hence on receiving linkup event for backhaul bss, if controller device is found than
	 * unblocking the backhaul bss.
	 */
	WBD_SAFE_GET_I5_SELF_DEVICE(self_device, &ret);
	if (!I5_IS_CTRLAGENT(self_device->flags) && I5_IS_BSS_BACKHAUL(map) &&
		i5DmFindController()) {
		wbd_slave_block_unblock_backhaul_sta_assoc(1);
	}

	/* Add all the associated clients */
	blanket_add_all_associated_stas(name, &slave->wbd_ifr.bssid);

	/* Identify and Update the band in */
	ret = wbd_slave_update_band_type(name, slave);

	WBD_CHECK_MSG("Band[%d] ifname[%s] Slave["MACF"]. Faield to get chan info. %s\n",
		slave->band, name, ETHER_TO_MACF(slave->wbd_ifr.mac), wbderrorstr(ret));

#ifdef PLC_WBD
	if (slave_type == WBD_SLAVE_TYPE_PLC) {
		wbd_plc_get_local_plc_mac(&slave->parent->parent->plc_info,
			&slave->wbd_ifr.plc_mac);
	}
#endif /* PLC_WBD */

	WBD_INFO("Interface Info read success : ifname[%s] Slave["MACF"] BSSID["MACF"]\n",
		name, ETHER_TO_MACF(slave->wbd_ifr.mac), ETHER_TO_MACF(slave->wbd_ifr.bssid));
end:
	WBD_EXIT();
}

/* Read blanket slave NVRAMs */
static void
wbd_read_blanket_slave_nvrams(wbd_blanket_slave_t *bkt_slave)
{
	uint32 flags = 0;
	WBD_ENTER();

	/* Get NVRAM : AP auto configuration search threshold */
	bkt_slave->n_ap_auto_config_search_thld = (uint8)blanket_get_config_val_uint(NULL,
		WBD_NVRAM_AP_CONFIG_SEARCH_THLD, WBD_DEF_AP_CONFIG_SEARCH_THLD);

	/* Get NVRAM : Channel Scan Capabilities Flags : OnBoot / Requested / Both */
	bkt_slave->scantype_flags = (uint8)blanket_get_config_val_uint(NULL,
		NVRAM_MAP_CHSCAN_SCANTYPE_FLAGS, DEF_MAP_CHSCAN_SCANTYPE_FLAGS);

	/* Get NVRAM : Slave flags */
	flags = blanket_get_config_val_uint(NULL, WBD_NVRAM_SLAVE_FLAGS, WBD_NVRAM_SLAVE_FLAGS_DEF);
	if (flags & WBD_NVRAM_SLAVE_FLAGS_KEEP_FH_BSS_UP) {
		bkt_slave->flags |= WBD_BKT_SLV_FLAGS_KEEP_FH_BSS_UP;
	}

	/* Get NVRAM : TXduration enabled or not to get Channel Utilization */
	if (blanket_get_config_val_int(NULL, WBD_NVRAM_USE_TX_IN_CHAN_UTIL,
		WBD_DEF_TX_IN_CHAN_UTIL)) {
		bkt_slave->flags |= WBD_BKT_SLV_FLAGS_TX_IN_CHAN_UTIL;
	} else {
		bkt_slave->flags &= ~WBD_BKT_SLV_FLAGS_TX_IN_CHAN_UTIL;
	}

	/* Get NVRAM : Busy = 100 - TxOp or not to get Channel Utilization */
	if (blanket_get_config_val_int(NULL, WBD_NVRAM_BUSY_100_TXOP,
		WBD_DEF_BUSY_100_TXOP)) {
		bkt_slave->flags |= WBD_BKT_SLV_FLAGS_BUSY_100_TXOP;
	} else {
		bkt_slave->flags &= ~WBD_BKT_SLV_FLAGS_BUSY_100_TXOP;
	}

	/* Get NVRAM : Agent should Exclude Pref = 0 Chanspecs  */
	if (blanket_get_config_val_int(NULL, NVRAM_MAP_AGNT_EXCLD_PREF0_CH,
		DEF_MAP_AGNT_EXCLD_PREF0_CH)) {
		bkt_slave->flags |= WBD_BKT_SLV_FLAGS_EXCL_PREF0_CH;
	} else {
		bkt_slave->flags &= ~WBD_BKT_SLV_FLAGS_EXCL_PREF0_CH;
	}

	WBD_INFO("AP Auto Config Search Treshold[%d] scantype_flags[%d] Slave Flags[0x%x]\n",
		bkt_slave->n_ap_auto_config_search_thld, bkt_slave->scantype_flags,
		bkt_slave->flags);

	WBD_EXIT();
}

/* Initialize the slave module */
static int
wbd_init_slave(wbd_info_t *info)
{
	int ret = WBDE_OK;
	wbd_slave_item_t *slave = NULL;
	dll_t *slave_item_p;
	WBD_ENTER();

#ifdef PLC_WBD
	/* Init PLC interface */
	wbd_plc_init(&info->plc_info);
#endif /* PLC_WBD */

	/* Read backhual NVRAM config */
	wbd_retrieve_backhaul_nvram_config(&info->wbd_slave->metric_policy_bh);
	blanket_nvram_prefix_set(NULL, NVRAM_MAP_AGENT_CONFIGURED, "0");

	/* Read blanket slave NVRAM config */
	wbd_read_blanket_slave_nvrams(info->wbd_slave);

	/* Fm driver, Fill Slave's Interface info to local ds */
	ret = wbd_slave_fill_interface_info(info);
	WBD_ASSERT_MSG("Failed to get interface info... Aborting...\n");

	/* Try to open the server FD */
	info->server_fd = wbd_try_open_server_fd(EAPD_WKSP_WBD_TCP_SLAVE_PORT, &ret);
	WBD_ASSERT_MSG("Failed to create server socket\n");

	/* Try to open the server FD for CLI */
	info->cli_server_fd = wbd_try_open_server_fd(EAPD_WKSP_WBD_TCP_SLAVECLI_PORT, &ret);
	WBD_ASSERT_MSG("Failed to create CLI socket\n");

	/* Try to open the Event FD and add it to scheduler */
	info->event_fd = wbd_try_open_event_fd(EAPD_WKSP_WBD_UDP_SPORT, &ret);
	WBD_ASSERT_MSG("Failed to create event socket for all the events from driver\n");
	ret = wbd_add_fd_to_schedule(info->hdl, info->event_fd,
		info, wbd_slave_process_event_fd_cb);
	WBD_ASSERT();

	/* Initialize the communication module for slave */
	ret = wbd_init_slave_com_handle(info);
	WBD_ASSERT();

	/* Traverse br0 Slave Item List for this Blanket Slave for each band */
	foreach_glist_item(slave_item_p, info->wbd_slave->br0_slave_list) {
		/* Choose Slave Info based on band */
		slave = (wbd_slave_item_t*)slave_item_p;
		wbd_slave_linkup_fill_interface_info(slave);
	}

end:
	WBD_EXIT();
	return ret;
}

/* Callback from IEEE 1905 module to get interface info */
static int
wbd_slave_get_interface_info_cb(char *ifname, ieee1905_ifr_info *info)
{
	int ret = WBDE_OK;
	char prefix[IFNAMSIZ];
	wl_bss_info_t *bss_info;
	chanspec_t chanspec;
	WBD_ENTER();

	memset(&info->ap_caps, 0, sizeof(info->ap_caps));

	/* Get Prefix from OS specific interface name */
	blanket_get_interface_prefix(ifname, prefix, sizeof(prefix));
	info->mapFlags = wbd_get_map_flags(prefix);

	/* Check interface (for valid BRCM wl interface) */
	(void)blanket_probe(ifname);

	blanket_get_chanspec(ifname, &chanspec);
	info->chanspec = (unsigned short)chanspec;

	/* If its STA interface get the BSSID to fill in the media specific info */
	if (I5_IS_BSS_STA(info->mapFlags)) {
		blanket_get_bssid(ifname, (struct ether_addr*)info->bssid);
	}

	/* Get BSS Info */
	ret = blanket_get_bss_info(ifname, &bss_info);
	WBD_ASSERT();

	/* Get HT capabilities */
	(void)blanket_get_ht_cap(ifname, bss_info, &info->ap_caps.HTCaps);

	/* Get VHT capabilities */
	(void)blanket_get_vht_cap(ifname, bss_info, &info->ap_caps.VHTCaps);

	/* Get HE capabilities */
	(void)blanket_get_he_cap(ifname, bss_info, &info->ap_caps.HECaps);

	/* Fill radio capability list */
	(void)blanket_get_radio_cap(ifname, &info->ap_caps.RadioCaps);

#if defined(MULTIAPR2)
	/* Fill Channel Scan capability list */
	(void)blanket_get_channel_scan_cap(ifname, &info->ap_caps.ChScanCaps);
#endif /* MULTIAPR2 */

end:
	WBD_EXIT();
	return ret;
}

/* Exit the slave module */
void
wbd_exit_slave(wbd_info_t *info)
{
	int ret = WBDE_OK;
	WBD_ENTER();

	BCM_REFERENCE(ret);

	/* Set flag to mark it as application is closing. This has to be set before calling
	 * ieee1905_deinit. Because we block backhual and fronthaul BSS if the controller device
	 * is removed based on this flag. And device removal happens from ieee1905_deinit.
	 */
	if (info) {
		info->flags |= WBD_INFO_FLAGS_CLOSING_APP;
	}

	ieee1905_deinit();

	/* Set agent configured NVRAM to 0 */
	blanket_nvram_prefix_set(NULL, NVRAM_MAP_AGENT_CONFIGURED, "0");

	WBD_ASSERT_ARG(info, WBDE_INV_ARG);

	wbd_ds_blanket_slave_cleanup(info);
	wbd_com_deinit(info->com_cli_hdl);
	wbd_com_deinit(info->com_serv_hdl);
	wbd_info_cleanup(info);
	g_wbdinfo = NULL;
	ieee1905_free_lanifnames_list();

end:
	WBD_EXIT();
}

/* Signal handler to display tty output. */
void
wbd_slave_tty_hdlr(int sig)
{
	BCM_REFERENCE(sig);

	wbd_tty_hdlr(WBD_SLAVE_FILE_TTY);
}

/* Signal handler */
void
wbd_signal_hdlr(int sig)
{
	WBD_ERROR("Signal : %d WBD Slave going to shutdown\n", sig);
	wbd_exit_slave(g_wbdinfo);
	exit(0);
}

/* Callback from IEEE 1905 module when the agent gets STEER request */
static void
wbd_ieee1905_steer_request_cb(ieee1905_steer_req *steer_req)
{
	wbd_slave_process_map_steer_request_cb(g_wbdinfo, steer_req);
}

/* Callback from IEEE 1905 module when the agent gets block/unblock STA request */
static void
wbd_ieee1905_block_unblock_sta_request_cb(ieee1905_block_unblock_sta *block_unblock_sta)
{
	wbd_slave_process_map_block_unblock_sta_request_cb(g_wbdinfo, block_unblock_sta);
}

/* Callback from IEEE 1905 module to prepare channel preference report */
static void
wbd_ieee1905_prepare_channel_pref_cb(i5_dm_interface_type *i5_intf,
	ieee1905_chan_pref_rc_map_array *cp)
{
	wbd_slave_prepare_local_channel_preference(i5_intf, cp);
}

/* Agent Update Non-operable Channels of an interface */
static void
wbd_ieee1905_nonoperable_channel_update(unsigned char *al_mac, unsigned char *interface_mac,
	ieee1905_chan_pref_rc_map_array *cp)
{
	i5_dm_device_type *i5_self_device = i5DmGetSelfDevice();
	i5_dm_interface_type *i5_ifr = NULL;
	wbd_ifr_item_t *ifr_vndr_data = NULL;

	/* Check if, Map Agent need to Report Preference = 0 as Non-operable to ACSD ? */
	if (!WBD_IS_EXCL_PREF0_CH((g_wbdinfo->wbd_slave->flags))) {
		return; /* If not, skip */
	}
	if ((i5_ifr = i5DmInterfaceFind(i5_self_device, interface_mac)) == NULL) {
		WBD_WARNING("IFR["MACDBG"] not found\n", MAC2STRDBG(interface_mac));
		return;
	}
	ifr_vndr_data = (wbd_ifr_item_t*)i5_ifr->vndr_data;
	if (!ifr_vndr_data) {
		WBD_WARNING("IFR["MACDBG"] Vendor Data not found\n", MAC2STRDBG(interface_mac));
		return;
	}
	WBD_INFO("IFR["MACDBG"] Updating DYNAMIC Non-operable Channels of an interface.\n",
		MAC2STRDBG(i5_ifr->InterfaceId));

	/* Update Non-operable Chanspec List of an interface */
	blanket_update_dynamic_nonoperable_chanspec_list(i5_ifr->band,
		cp, &ifr_vndr_data->pref0_exclude_chlist);

	/* IF Non-operable Chanspec List count >= 0 , then Send */
	if (ifr_vndr_data->pref0_exclude_chlist.count >= 0) {

		/* Send Consolidated Exclude list to ACSD (empty list to reset) */
		wbd_slave_process_nonoperable_channel_update(i5_ifr);
	}
}

/* Callback from IEEE 1905 module when the agent gets channel Selection request */
static void
wbd_ieee1905_recv_chan_selection_request_cb(unsigned char *al_mac, unsigned char *interface_mac,
	ieee1905_chan_pref_rc_map_array *cp, unsigned char rclass_local_count,
	ieee1905_chan_pref_rc_map *local_chan_pref)
{
	WBD_INFO("IFR["MACDBG"] Received Channel Selection Request from Controller.\n",
		MAC2STRDBG(interface_mac));

	wbd_slave_process_chan_selection_request_cb(g_wbdinfo, al_mac, interface_mac, cp,
		rclass_local_count, local_chan_pref);

	wbd_ieee1905_nonoperable_channel_update(al_mac, interface_mac, cp);
}
/* Callback from IEEE 1905 module when the agent gets channel Selection request */
static void
wbd_ieee1905_send_opchannel_report_cb(void)
{
	wbd_slave_send_opchannel_reports();
}

/* Callback from IEEE 1905 module when the agent gets Tx power limit */
static void
wbd_ieee1905_set_tx_power_limit_cb(char *ifname, unsigned char tx_power_limit)
{
	wbd_slave_process_set_tx_power_limit_cb(g_wbdinfo, ifname, tx_power_limit);
}

/* Callback from IEEE 1905 module to get the backhual link metric */
static int
wbd_ieee1905_get_backhaul_link_metric_cb(char *ifname, unsigned char *interface_mac,
	ieee1905_backhaul_link_metric *metric)
{
	return (wbd_slave_process_get_backhaul_link_metric_cb(g_wbdinfo, ifname,
		interface_mac, metric));
}

/* Callback from IEEE 1905 module to get the interface metrics */
static int
wbd_ieee1905_get_interface_metric_cb(char *ifname, unsigned char *ifr_mac,
	ieee1905_interface_metric *metric)
{
	WBD_DEBUG("Received callback: Ifname[%s] Interface["MACDBG"] Interface Metrics\n",
		ifname, MAC2STRDBG(ifr_mac));
	return (wbd_slave_process_get_interface_metric_cb(g_wbdinfo, ifname, ifr_mac, metric));
}

/* Callback from IEEE 1905 module to get the AP metrics */
static int
wbd_ieee1905_get_ap_metric_cb(char *ifname, unsigned char *bssid, ieee1905_ap_metric *metric)
{
	WBD_DEBUG("Received callback: Ifname[%s] BSS["MACDBG"] AP Metrics\n",
		ifname, MAC2STRDBG(bssid));
	return (wbd_slave_process_get_ap_metric_cb(g_wbdinfo, ifname, bssid, metric));
}

/* Callback from IEEE 1905 module to get the associated STA link metrics and STA traffic stats */
static int
wbd_ieee1905_get_assoc_sta_metric_cb(char *ifname, unsigned char *bssid, unsigned char *sta_mac,
	ieee1905_sta_link_metric *metric, ieee1905_sta_traffic_stats *traffic_stats,
	ieee1905_vendor_data *out_vndr_tlv)
{
	WBD_DEBUG("Received callback: Ifname[%s] BSS["MACDBG"] STA["MACDBG"] Assoc STA Metrics\n",
		ifname, MAC2STRDBG(bssid), MAC2STRDBG(sta_mac));
	return (wbd_slave_process_get_assoc_sta_metric(ifname, bssid, sta_mac,
		metric, traffic_stats, out_vndr_tlv));
}

/* Callback from IEEE 1905 module to get the un associated STA link metrics */
static int
wbd_ieee1905_get_unassoc_sta_metric_cb(ieee1905_unassoc_sta_link_metric_query *query)
{
	return (wbd_slave_process_get_unassoc_sta_metric_cb(g_wbdinfo, query));
}

/* Callback from IEEE 1905 module to init the device */
static void
wbd_ieee1905_device_init_cb(i5_dm_device_type *i5_device)
{
	i5_dm_device_type *self_device = i5DmGetSelfDevice();
	WBD_ENTER();

	/* Do below, Only for Self Device */
	if (eacmp(self_device->DeviceId, i5_device->DeviceId) != 0) {
		return;
	}

	WBD_DEBUG("Received callback: Device["MACDBG"] got added\n",
		MAC2STRDBG(i5_device->DeviceId));

	wbd_ds_device_init(i5_device);

	WBD_EXIT();
}

/* Callback from IEEE 1905 module to deinit the device */
static void
wbd_ieee1905_device_deinit_cb(i5_dm_device_type *i5_device)
{
	i5_dm_device_type *self_device = i5DmGetSelfDevice();
	WBD_ENTER();

	WBD_DEBUG("Received callback: Device["MACDBG"] %s got removed\n",
		MAC2STRDBG(i5_device->DeviceId),
		I5_IS_MULTIAP_CONTROLLER(i5_device->flags) ? "Controller" : "Agent");

	/* If the device being removed is controller and the self device is not the agent
	 * running on the controller and not upstream AP and the device is getting removed when the
	 * application is not closing, then only disable the backhaul BSS
	 */
	if (!WBD_CLOSING_APP(g_wbdinfo->flags) && !I5_IS_CTRLAGENT(self_device->flags) &&
		!WBD_UAP_ENAB(g_wbdinfo->flags) && (i5_device == i5DmFindController())) {
		WBD_ERROR("Controller device "MACDBG" removed. Blocking backhaul BSS\n",
			MAC2STRDBG(i5_device->DeviceId));
		wbd_slave_block_unblock_backhaul_sta_assoc(0);
		/* Bring down all the fronthaul BSS if it is up */
		wbd_blanket_util_enable_disable_all_fronthaul_bss(0);
	}

	WBD_EXIT();
}

/* Callback from IEEE 1905 module to init the interface on device */
static void
wbd_ieee1905_interface_init_cb(i5_dm_interface_type *i5_ifr)
{
	i5_dm_device_type *i5_device;

	if (!i5_ifr) {
		return;
	}

	/* If not self device, dont init */
	i5_device = (i5_dm_device_type*)WBD_I5LL_PARENT(i5_ifr);
	if (!i5DmDeviceIsSelf(i5_device->DeviceId)) {
		return;
	}

	wbd_ds_interface_init(i5_ifr);
}

/* Callback from IEEE 1905 module to deinit the interface on device */
static void
wbd_ieee1905_interface_deinit_cb(i5_dm_interface_type *i5_ifr)
{
	wbd_ds_interface_deinit(i5_ifr);
}

/* Callback from IEEE 1905 module to init the BSS on interface */
static void
wbd_ieee1905_bss_init_cb(i5_dm_bss_type *pbss)
{
	wbd_ds_bss_init(pbss);

	/* Add static neighbor list entry of this bss */
	wbd_slave_add_bss_nbr(pbss);
}

/* Callback from IEEE 1905 module to deinit the BSS on interface */
static void
wbd_ieee1905_bss_deinit_cb(i5_dm_bss_type *pbss)
{
	wbd_ds_bss_deinit(pbss);

	/* Delete static neighbor list entry */
	wbd_slave_del_bss_nbr(pbss);
}

/* Callback from IEEE 1905 module to Create the BSS on interface */
static int
wbd_ieee1905_create_bss_on_ifr_cb(char *ifname,
	ieee1905_glist_t *bssinfo_list, ieee1905_policy_config *policy, unsigned char policy_type)
{
	return (wbd_slave_create_bss_on_ifr(g_wbdinfo, ifname, bssinfo_list, policy, policy_type));
}

/* Callback from IEEE 1905 module to get interface info */
static int
wbd_ieee1905_get_interface_info_cb(char *ifname, ieee1905_ifr_info *info)
{
	return (wbd_slave_get_interface_info_cb(ifname, info));
}

/* Callback from IEEE 1905 module when the agent gets backhaul STEER request */
static void
wbd_ieee1905_bh_steer_request_cb(char *ifname, ieee1905_backhaul_steer_msg *bh_steer_req)
{
	WBD_DEBUG("Received callback: Backhaul STA["MACDBG"] is moving to BSSID["MACDBG"]\n",
		MAC2STRDBG(bh_steer_req->bh_sta_mac), MAC2STRDBG(bh_steer_req->trgt_bssid));
	wbd_slave_process_bh_steer_request_cb(g_wbdinfo->hdl, bh_steer_req);
}

/* Callback from IEEE 1905 module when the agent gets beacon metrics query */
static int
wbd_ieee1905_beacon_metrics_query_cb(char *ifname, unsigned char *bssid,
	ieee1905_beacon_request *query)
{
	WBD_DEBUG("Received callback: Ifname[%s] BSS["MACDBG"] STA["MACDBG"]Beacon metrics query\n",
		ifname, MAC2STRDBG(bssid), MAC2STRDBG(query->sta_mac));
	if (WBD_PER_CHAN_BCN_REQ(g_wbdinfo->flags))
		return wbd_slave_process_per_chan_beacon_metrics_query_cb(g_wbdinfo, ifname,
			bssid, query);
	else
		return wbd_slave_process_beacon_metrics_query_cb(g_wbdinfo, ifname, bssid, query);
}

/* Callback from IEEE 1905 module when the agent gets Multi-AP Policy configuration */
static void
wbd_ieee1905_policy_configuration_cb(ieee1905_policy_config *policy, unsigned short rcvd_policies,
	ieee1905_vendor_data *in_vndr_tlv)
{
	WBD_DEBUG("Received callback: Multi-AP Policy Configuration\n");
	return (wbd_slave_policy_configuration_cb(g_wbdinfo, policy, rcvd_policies, in_vndr_tlv));
}

/* Callback from IEEE 1905 module when the STA got added in the data model */
static void
wbd_ieee1905_sta_added_cb(i5_dm_clients_type *i5_assoc_sta)
{
	i5_dm_bss_type *i5_bss;
	i5_dm_interface_type *i5_ifr;
	i5_dm_device_type *i5_device;

	i5_bss = (i5_dm_bss_type*)WBD_I5LL_PARENT(i5_assoc_sta);
	WBD_INFO("%sSTA "MACDBG" associated on BSS "MACDBG"\n",
		I5_CLIENT_IS_BSTA(i5_assoc_sta)? "Backhaul ": "", MAC2STRDBG(i5_assoc_sta->mac),
		MAC2STRDBG(i5_bss->BSSID));

	wbd_ds_sta_init(g_wbdinfo, i5_assoc_sta);

	/* If the BSS is backhaul BSS, then its a backhaul STA. Create the bakchual STA weak
	 * client watch dog timer if not exists
	 */
	if (I5_CLIENT_IS_BSTA(i5_assoc_sta)) {
		i5_ifr = (i5_dm_interface_type*)WBD_I5LL_PARENT(i5_bss);
		i5_device = (i5_dm_device_type*)WBD_I5LL_PARENT(i5_ifr);
		if (i5_device == i5DmGetSelfDevice()) {
			wbd_slave_create_backhaul_sta_weak_client_watchdog_timer(g_wbdinfo);
		}
	}
}

/* Callback from IEEE 1905 module when the STA got removed from the data model */
static void
wbd_ieee1905_sta_removed_cb(i5_dm_clients_type *i5_assoc_sta)
{
	WBD_DEBUG("Received callback: STA "MACDBG" got Removed from BSS "MACDBG"\n",
		MAC2STRDBG(i5_assoc_sta->mac),
		MAC2STRDBG(((i5_dm_bss_type*)WBD_I5LL_PARENT(i5_assoc_sta))->BSSID));
	wbd_ds_remove_beacon_report(g_wbdinfo, (struct ether_addr*)&(i5_assoc_sta->mac));
	return (wbd_ds_sta_deinit(i5_assoc_sta));
}

/* Callback from IEEE 1905 module to inform AP auto configuration */
static void
wbd_ieee1905_ap_configured_cb(unsigned char *al_mac, unsigned char *radio_mac,
	int if_band)
{
	WBD_DEBUG("Received callback: AP autoconfigured on Device ["MACDBG"] Radio["MACDBG"]"
		"in band [%d]\n", MAC2STRDBG(al_mac), MAC2STRDBG(radio_mac), if_band);
	/* Check if M2 is received for all Wireless Interfaces */
	if (i5DmIsAllInterfacesConfigured()) {

		WBD_INFO("Received callback: M2 is received for all Wireless Interfaces, "
			"Create all_ap_configured timer\n");

		if ((wbd_ds_is_fbt_possible_on_agent() == WBDE_DS_FBT_NT_POS) &&
			(!(g_wbdinfo->flags & WBD_INFO_FLAGS_RC_RESTART))) {
#ifdef BCM_APPEVENTD
			/* Raise and send MAP init end event to appeventd. */
			wbd_appeventd_map_init(APP_E_WBD_SLAVE_MAP_INIT_END,
				(struct ether_addr*)ieee1905_get_al_mac(), MAP_INIT_END,
				MAP_APPTYPE_SLAVE);
#endif /* BCM_APPEVENTD */
			blanket_nvram_prefix_set(NULL, NVRAM_MAP_AGENT_CONFIGURED, "1");
		}

#ifdef WLHOSTFBT

		if (!(g_wbdinfo->wbd_slave->flags & WBD_BKT_SLV_FLAGS_SEND_FBT_REQ)) {
			/* If FBT generate local is set on all interfaces and
			 * Controller is sending FBT configurations in M2,
			 * and Security is not WPA3, then
			 * Agent can skip sending FBT Request, and set
			 * FBT based on FBT configurations sent in M2.
			 * If Controller want to Disable FBT on Agents, it will send MDID = 0.
			*/
			WBD_INFO("MDID in M2 is received for all the interfaces. "
				"now enable FBT on the device from M2 FBT data\n");
			wbd_slave_set_fbt_on_agent_using_m2(g_wbdinfo);
		} else {
			/* Send 1905 Vendor Specific FBT_CONFIG_REQ command,
			 * from Agent to Controller.
			*/
			WBD_INFO("M2 is received for all the interfaces. "
				"now Send FBT Config Request\n");
			wbd_slave_send_fbt_config_request_cmd();
		}
#endif /* WLHOSTFBT */

		wbd_slave_create_all_ap_configured_timer();
	}
}

/* Callback from IEEE 1905 module to indicate channel change of interface on device */
static void
wbd_ieee1905_interface_chan_change_cb(i5_dm_interface_type *i5_ifr)
{
	i5_dm_device_type *i5_device;

	if (!i5_ifr) {
		WBD_ERROR("ifr NULL \n");
		return;
	}

	/* If self device, dont add neighbors */
	i5_device = (i5_dm_device_type*)WBD_I5LL_PARENT(i5_ifr);
	if (i5DmDeviceIsSelf(i5_device->DeviceId)) {
		return;
	}

	/* Add static neighbor entry of this interface bss's with updated channel */
	wbd_slave_add_ifr_nbr(i5_ifr, FALSE);
}

/* Timer Callback to Start M1
 * if all the BSS are up, start M1. Otherwise restart the timer again
 */
static void
wbd_slave_start_m1_timer_cb(bcm_usched_handle *hdl, void *arg)
{
	int ret = WBDE_OK, send_m1 = 1;
	wbd_info_t *info = (wbd_info_t *)arg;
	wbd_slave_item_t *slave = NULL;
	dll_t *slave_item_p;
	WBD_ENTER();

	if (!info) {
		WBD_WARNING("Invalid slave\n");
		goto end;
	}

	/* Traverse br0 Slave Item List for this Blanket Slave for each band */
	foreach_glist_item(slave_item_p, info->wbd_slave->br0_slave_list) {
		/* Choose Slave Info based on band */
		slave = (wbd_slave_item_t*)slave_item_p;
		if (slave->wbd_ifr.enabled) {
			continue;
		}

		wbd_slave_linkup_fill_interface_info(slave);
		if (!slave->wbd_ifr.enabled) {
			send_m1 = 0;
			break;
		}
	}

	if (send_m1) {
		WBD_INFO("All Slave's enabled start sending M1 immediatedly \n");
		ieee1905_start_m1();
	} else {
		wbd_remove_timers(hdl, wbd_slave_start_m1_timer_cb, info);
		WBD_DEBUG("M1 start timer expired,but BSS ["MACF"] not up. restarting the timer \n",
			ETHER_TO_MACF(slave->wbd_ifr.mac));
		ret = wbd_add_timers(hdl, info,
			WBD_MSEC_USEC(WBD_SLEEP_START_M1_FAIL),
			wbd_slave_start_m1_timer_cb, 0);
		if (ret != WBDE_OK) {
			WBD_WARNING("Interval[%d] Failed to slave update timer\n",
				WBD_SLEEP_START_M1_FAIL);
		}
	}

end:
	WBD_EXIT();
}

/* Callback from IEEE 1905 module to inform 1905 AP-Autoconfiguration Response message from
 * controller and check slave enabled to start m1 for unconfigured radio
 */
static int
wbd_ieee1905_ap_auto_config_resp_cb(i5_dm_device_type *i5_device)
{
	int ret = WBDE_OK;
	wbd_blanket_slave_t *wbd_slave;
	i5_dm_device_type *self_device = i5DmGetSelfDevice();
	i5_dm_interface_type *i5_ifr;
	struct ether_addr bssid;
	char prefix[IFNAMSIZ];
	i5_socket_type *psock = NULL;
	WBD_ENTER();

	/* Enable all the backhaul BSS as controller is detected */
	WBD_DEBUG("Received callback: 1905 AP-Autoconfiguration Response from Device["MACDBG"]\n",
		MAC2STRDBG(i5_device->DeviceId));
	WBD_ASSERT_ARG(g_wbdinfo->wbd_slave, WBDE_INV_ARG);

	wbd_slave = (wbd_blanket_slave_t*)g_wbdinfo->wbd_slave;

	/* If the device being added is controller and the self device is not the agent
	 * running on the controller, then only enable the backhaul BSS
	 */
	if (!I5_IS_CTRLAGENT(self_device->flags) && I5_IS_MULTIAP_CONTROLLER(i5_device->flags)) {
		WBD_INFO("Controller device "MACDBG" detected. Unblocking backhaul BSS\n",
			MAC2STRDBG(i5_device->DeviceId));
			wbd_slave_block_unblock_backhaul_sta_assoc(1);
			/* Bring up all the fronthaul BSS if it is down */
			wbd_blanket_util_enable_disable_all_fronthaul_bss(1);
	}
	wbd_slave->n_ap_auto_config_search = 0;

	if (i5_device->psock) {
		psock = (i5_socket_type*)i5_device->psock;
		WBD_INFO("Controller is reachable through ifname[%s] and MAC["MACF"]\n",
			psock->u.sll.ifname, ETHERP_TO_MACF(psock->u.sll.mac_address));
	}

	/* Go thorugh each interface to find a STA interface to store the BSSID */
	foreach_i5glist_item(i5_ifr, i5_dm_interface_type, self_device->interface_list) {
		if (!i5DmIsInterfaceWireless(i5_ifr->MediaType) ||
			!I5_IS_BSS_STA(i5_ifr->mapFlags)) {
			continue;
		}

		/* If controller is not reachable in this MAC, then do not store the BSSID */
		if (psock && (eacmp(psock->u.sll.mac_address, i5_ifr->InterfaceId))) {
			continue;
		}

		memset(&bssid, 0, sizeof(bssid));
		if (blanket_get_bssid(i5_ifr->ifname, &bssid) == WBDE_OK) {
			/* If the BSSID is not NULL store it in NVRAM */
			if (!(ETHER_ISNULLADDR(&bssid))) {
				/* Get Prefix from OS specific interface name */
				blanket_get_interface_prefix(i5_ifr->ifname, prefix,
					sizeof(prefix));
				wbd_slave_store_bssid_nvram(prefix, (unsigned char*)&bssid, 0);
				break;
			}
		}
	}

	wbd_slave_start_m1_timer_cb(g_wbdinfo->hdl, g_wbdinfo);
end:
	WBD_EXIT();

	return WBDE_OK;
}

#if defined(MULTIAPR2)
/* Callback from IEEE 1905 module when the agents want to process cac msg */
static void
wbd_ieee1905_process_cac_msg_cb(uint8 *al_mac, void *msg, uint32 msg_type)
{
	WBD_DEBUG("Received ieee1905 callback: Process cac message\n");
	return wbd_slave_process_cac_msg(g_wbdinfo, al_mac, msg, msg_type);
}
#endif /* MULTIAPR2 */

/* Callback from IEEE 1905 module to inform 1905 AP-Autoconfiguration search message sent */
static void
wbd_ieee1905_ap_auto_config_search_sent()
{
	int ret = WBDE_OK;
	wbd_blanket_slave_t *wbd_slave;
	i5_dm_device_type *self_device = i5DmGetSelfDevice();
	WBD_ENTER();

	/* Enable all the backhaul BSS as controller is detected */
	WBD_DEBUG("Received callback: 1905 AP-Autoconfiguration Search Sent\n");
	WBD_ASSERT_ARG(g_wbdinfo->wbd_slave, WBDE_INV_ARG);

	wbd_slave = (wbd_blanket_slave_t*)g_wbdinfo->wbd_slave;

	wbd_slave->n_ap_auto_config_search++;

	/* If the count of AP auto configuration search exceeds max limit block the backhaul BSS */
	if (!I5_IS_CTRLAGENT(self_device->flags) &&
		(wbd_slave->n_ap_auto_config_search > wbd_slave->n_ap_auto_config_search_thld)) {
		WBD_ERROR("AP-Autoconfiguration search count %d exceeds threshold of %d. Assuming "
			"controller not reachable and blocking backhaul BSS\n",
			wbd_slave->n_ap_auto_config_search,
			wbd_slave->n_ap_auto_config_search_thld);
		wbd_slave_block_unblock_backhaul_sta_assoc(0);
		/* Bring down all the fronthaul BSS if it is up */
		wbd_blanket_util_enable_disable_all_fronthaul_bss(0);
	}

end:
	WBD_EXIT();
}

/* Callback from IEEE 1905 module to remove and deauth STA from assoclist */
static void
wbd_ieee1905_remove_and_deauth_sta_cb(char *ifname, unsigned char *parent_bssid,
	unsigned char *sta_mac)
{
	wbd_slave_remove_and_deauth_sta(ifname, parent_bssid, sta_mac);
}

#if defined(MULTIAPR2)
/* Callback from IEEE 1905 module to Process Channel Scan Request by Agent */
static void
wbd_ieee1905_channel_scan_req_cb(unsigned char *src_al_mac,
	ieee1905_chscan_req_msg *chscan_req)
{
	WBD_DEBUG("Received callback: Process Channel Scan Request by Agent\n");

	return wbd_slave_process_channel_scan_req_cb(src_al_mac, chscan_req);
}

/* Callback from IEEE 1905 module to prepare cac complete payload */
static void
wbd_ieee1905_prepare_cac_complete_cb(uint8 **pbuf, uint32 *payload_len)
{
	wbd_slave_prepare_cac_complete_tlv_payload(pbuf, payload_len);
}

/* Callback from IEEE 1905 module to prepare cac complete payload */
static void
wbd_ieee1905_prepare_cac_status(uint8 **pbuf, uint16 *payload_len)
{
	wbd_slave_prepare_cac_status_payload(pbuf, payload_len);
}

/* Callback from IEEE 1905 module to prepare cac capability for cac capable radios */
static void
wbd_ieee1905_prepare_cac_capability_cb(uint8 **pbuf, uint16 *payload_len)
{
	wbd_slave_prepare_cac_capability(pbuf, payload_len);
}

/* Callback from IEEE 1905 module to get backhaul STA profile */
static unsigned char
wbd_ieee1905_get_bh_sta_profile_cb(char *ifname)
{
	WBD_INFO("Get profile for ifname %s\n", ifname);

	return wbd_blanket_get_bh_sta_profile(ifname);
}

/* Callback from IEEE 1905 module to set or unset association disallowed attribute in beacon */
static unsigned char
wbd_ieee1905_mbo_assoc_disallowed_cb(char *ifname, unsigned char reason)
{
	WBD_INFO("Set or Unset association disallowed attribute in beacon. Ifname[%s] reason[%d]\n",
		ifname, reason);

	return wbd_blanket_mbo_assoc_disallowed(ifname, reason);
}

/* Callback from IEEE 1905 module to set DFS channel clear, as indicated by the controller */
static void
wbd_ieee1905_set_dfs_chan_clear_cb(char *ifname, ieee1905_chan_pref_rc_map *chan_pref)
{
	wbd_blanket_set_dfs_chan_clear(ifname, chan_pref);
}

/* Callback from IEEE 1905 module to get the primary VLAN ID for the backhaul STA interface */
static unsigned short
wbd_ieee1905_get_primary_vlan_id(char *ifname)
{
	unsigned short vlan_id = 0;

	WBD_INFO("Get primary VLAN ID for ifname %s\n", ifname);

	blanket_get_primary_vlan_id(ifname, &vlan_id);

	return vlan_id;
}
#endif /* MULTIAPR2 */

/* Inform IEEE1905 about all the associated backhaul STAs */
static void
wbd_slave_notify_connected_bstas_to_iee1905(i5_dm_device_type *i5_self_dev)
{
	i5_dm_interface_type *i5_iter_ifr = NULL;
	unsigned short vlan_id = 0;

	/* Go through each interface */
	foreach_i5glist_item(i5_iter_ifr, i5_dm_interface_type, i5_self_dev->interface_list) {

		/* Only for STA interface */
		if (!I5_IS_BSS_STA(i5_iter_ifr->mapFlags)) {
			continue;
		}

		/* If the BSSID is NULL for a interface, then it is not connected to
		 * upstrem AP
		 */
		if (ETHER_ISNULLADDR(i5_iter_ifr->bssid)) {
			WBD_INFO("ifname[%s] backhaul STA is not associated\n",
				i5_iter_ifr->ifname);
			continue;
		}

#ifdef MULTIAPR2
		/* Get VLAN ID for the IEEE1905 to create VLAN interface */
		blanket_get_primary_vlan_id(i5_iter_ifr->ifname, &vlan_id);
#endif /* MULTIAPR2 */

		WBD_INFO("ifname[%s] is associated and VLAN ID[%d]\n", i5_iter_ifr->ifname,
			vlan_id);
		ieee1905_bSTA_associated_to_backhaul_ap(i5_iter_ifr->InterfaceId,
			i5_iter_ifr->ifname, vlan_id, vlan_id ? 1 : 0);
	}

	return;
}

/* Handle the connected and disconnected backhaul STAs at boot up time. This is required bcoz
 * there will not be any events coming from the firmware which are already associated before WBD
 * comes up
 */
static void
wbd_slave_handle_bstas_on_boot()
{
	int ret = WBDE_OK, is_any_bsta_associated = 0;
	i5_dm_device_type *i5_self_dev = NULL;
	WBD_ENTER();

	WBD_SAFE_GET_I5_SELF_DEVICE(i5_self_dev, &ret);

	/* Update the MAP flags in interface. This is required for all the calls below to check
	 * whether the interface is backhaul STA ot not
	 */
	wbd_slave_update_map_flags_of_all_ifr(i5_self_dev);

	/* Update the BSSID's of all the BSTAs. Required to know whether the BSTA is
	 * connected to not
	 */
	is_any_bsta_associated = wbd_blanket_util_update_bssids_of_all_bstas(i5_self_dev);

	/* Inform IEEE1905 about all the associated backhaul STAs */
	wbd_slave_notify_connected_bstas_to_iee1905(i5_self_dev);

	/* Set keep_ap_up to 0 for all the BSTA interfaces initially. Setting it to 1 happens
	 * later if needed
	 */
	wbd_blanket_util_set_bsta_keep_ap_up(i5_self_dev, 0);

	/* Bring up all the virtual interfaces of a BSTA which is not connected if any one of the
	 * BSTA is connected. This is required bcoz the WBD can read all the BSS details if it
	 * is up.
	 */
	if (is_any_bsta_associated) {
		wbd_blanket_util_vap_up_on_disconnected_bstas(i5_self_dev);
	}

end:
	WBD_EXIT();
}

/* Initialize ieee 1905 module */
static void
wbd_slave_register_ieee1905_callbacks(ieee1905_call_bks_t *cbs)
{
	cbs->steer_req = wbd_ieee1905_steer_request_cb;
	cbs->block_unblock_sta_req = wbd_ieee1905_block_unblock_sta_request_cb;
	cbs->prepare_channel_pref = wbd_ieee1905_prepare_channel_pref_cb;
	cbs->recv_chan_selection_req = wbd_ieee1905_recv_chan_selection_request_cb;
	cbs->send_opchannel_rpt = wbd_ieee1905_send_opchannel_report_cb;
	cbs->set_tx_power_limit = wbd_ieee1905_set_tx_power_limit_cb;
	cbs->backhaul_link_metric = wbd_ieee1905_get_backhaul_link_metric_cb;
	cbs->interface_metric = wbd_ieee1905_get_interface_metric_cb;
	cbs->ap_metric = wbd_ieee1905_get_ap_metric_cb;
	cbs->assoc_sta_metric = wbd_ieee1905_get_assoc_sta_metric_cb;
	cbs->unassoc_sta_metric = wbd_ieee1905_get_unassoc_sta_metric_cb;
	cbs->device_init = wbd_ieee1905_device_init_cb;
	cbs->device_deinit = wbd_ieee1905_device_deinit_cb;
	cbs->interface_init = wbd_ieee1905_interface_init_cb;
	cbs->interface_deinit = wbd_ieee1905_interface_deinit_cb;
	cbs->bss_init = wbd_ieee1905_bss_init_cb;
	cbs->bss_deinit = wbd_ieee1905_bss_deinit_cb;
	cbs->create_bss_on_ifr = wbd_ieee1905_create_bss_on_ifr_cb;
	cbs->get_interface_info = wbd_ieee1905_get_interface_info_cb;
	cbs->backhaul_steer_req = wbd_ieee1905_bh_steer_request_cb;
	cbs->beacon_metrics_query = wbd_ieee1905_beacon_metrics_query_cb;
	cbs->configure_policy = wbd_ieee1905_policy_configuration_cb;
	cbs->client_init = wbd_ieee1905_sta_added_cb;
	cbs->client_deinit = wbd_ieee1905_sta_removed_cb;
	cbs->ap_configured = wbd_ieee1905_ap_configured_cb;
	cbs->interface_chan_change = wbd_ieee1905_interface_chan_change_cb;
	cbs->ap_auto_config_resp = wbd_ieee1905_ap_auto_config_resp_cb;
	cbs->ap_auto_config_search_sent = wbd_ieee1905_ap_auto_config_search_sent;
	cbs->set_bh_sta_params = wbd_ieee1905_set_bh_sta_params;
	cbs->remove_and_deauth_sta_entry = wbd_ieee1905_remove_and_deauth_sta_cb;
#if defined(MULTIAPR2)
	cbs->channel_scan_req = wbd_ieee1905_channel_scan_req_cb;
	cbs->process_cac_msg = wbd_ieee1905_process_cac_msg_cb;
	cbs->prepare_cac_complete = wbd_ieee1905_prepare_cac_complete_cb;
	cbs->prepare_cac_capabilities = wbd_ieee1905_prepare_cac_capability_cb;
	cbs->get_bh_sta_profile = wbd_ieee1905_get_bh_sta_profile_cb;
	cbs->mbo_assoc_disallowed = wbd_ieee1905_mbo_assoc_disallowed_cb;
	cbs->prepare_cac_status = wbd_ieee1905_prepare_cac_status;
	cbs->set_dfs_chan_clear = wbd_ieee1905_set_dfs_chan_clear_cb;
	cbs->get_primary_vlan_id = wbd_ieee1905_get_primary_vlan_id;
#endif /* MULTIAPR2 */
}

/* Initialize the blanket module */
void
wbd_slave_init_blanket_module()
{
	blanket_module_info_t bkt_info;

	/* Initialize blanket module */
	memset(&bkt_info, 0, sizeof(bkt_info));
	bkt_info.msglevel = blanket_get_config_val_uint(NULL, NVRAM_BKT_MSGLEVEL,
		BKT_DEBUG_DEFAULT);
	blanket_module_init(&bkt_info);
}

#ifdef MULTIAPR2
/* Cretae the SSIDs for a VLANID and create traffic separation policy from NVRAM */
static int wbd_load_ts_vlanid(ieee1905_config *config, unsigned short vlanid)
{
	char nvname[128], name[256], *nvval, *next = NULL;
	ieee1905_ts_policy_t *ts_policy = NULL;
	ieee1905_ssid_type ssid;
	WBD_ENTER();

	WBD_INFO("Load VLAN config for VLAN ID %d\n", vlanid);

	/* Add VLAN ID to policy list. this return the pointer to the policy if added
	 * successfully or if the VLAN ID is already present in the list
	 */
	ts_policy = i5DmAddVLANIDToList(&config->ts_policy_list, vlanid);
	if (ts_policy == NULL) {
		WBD_WARNING("Failed to add VLAN ID[%d] to list\n", vlanid);
		goto end;
	}

	/* Get SSIDs for the corresponding VLAN ID and create list for SSIDs */
	snprintf(nvname, sizeof(nvname), "map_ts_%d_ssid", vlanid);
	nvval = blanket_nvram_safe_get(nvname);
	if (strlen(nvval) <= 0) {
		WBD_WARNING("NVRAM[%s] Not Defined. SSID list not present for VLAN ID[%d]\n",
			nvname, vlanid);
		goto end;
	}

	/* For each SSID in the NVRAM list */
	foreach(name, nvval, next) {

		/* Un-escape spaces in SSID */
		wbd_unescape_space(name);

		memset(&ssid, 0, sizeof(ssid));
		ssid.SSID_len = (unsigned char)strlen(name);
		WBDSTRNCPY((char*)ssid.SSID, name, sizeof(ssid.SSID));

		/* Add SSID to the VLAN policy list */
		WBD_INFO("VLAN ID[%d] SSID[%s] Len[%d]. Add SSID to list\n\n", vlanid, ssid.SSID,
			ssid.SSID_len);
		if (i5DmAddSSIDToList(&config->ts_policy_list, &ts_policy->ssid_list,
			&ssid) == NULL) {
			WBD_WARNING("For VLAN ID[%d] Failed to add SSID[%s] of len[%d] to list\n",
				vlanid, ssid.SSID, ssid.SSID_len);
			goto end;
		}
	}

	WBD_DEBUG("Load VLAN config for VLAN ID %d is successfull\n", vlanid);
	WBD_EXIT();
	return 0;

end:
	if (ts_policy) {
		i5DmGlistCleanup(&ts_policy->ssid_list);
		ieee1905_glist_delete(&config->ts_policy_list, (dll_t*)ts_policy);
		free(ts_policy);
	}
	WBD_EXIT();

	return -1;
}

/* Fill the VLAN policy structure for MultiAP R2 */
static int
wbd_slave_fill_vlan_policy(ieee1905_config *config, uint16 sec_vlan_id)
{
	int ret = -1;
	unsigned short vlan_id;
	char name[256], *nvval, *next = NULL;
	WBD_ENTER();

	/* Get VLAN ID list from NVRAM */
	nvval = blanket_nvram_safe_get(NVRAM_MAP_TS_VLANS);
	if (strlen(nvval) <= 0) {
		WBD_INFO("NVRAM[%s] Not set. So, traffic separation policy is not present\n",
			NVRAM_MAP_TS_VLANS);
		goto end;
	}

	/* For each VLAN ID in the NVRAM list */
	foreach(name, nvval, next) {
		vlan_id = (unsigned short)strtoul(name, NULL, 0);
		if (wbd_load_ts_vlanid(config, vlan_id) == -1) {
			goto end;
		}
	}

	ret = WBDE_OK;
	config->flags |= I5_INIT_FLAG_TS_ACTIVE;

end:
	WBD_EXIT();
	return ret;
}

#else /* MULTIAPR2 */

/* Check if the guest network is enabled or not */
static int
wbd_is_guest_enabled()
{
	char *lan1_ifnames, *wbd_ifnames, var_intf[IFNAMSIZ] = {0}, *next_intf;

	lan1_ifnames = blanket_nvram_safe_get(NVRAM_LAN1_IFNAMES);
	wbd_ifnames = blanket_nvram_safe_get(WBD_NVRAM_IFNAMES);

	/* Traverse wbd_ifnames for each ifname */
	foreach(var_intf, wbd_ifnames, next_intf) {

		/* If the interface name is present in lan1_ifnames, then the guest network
		 * is present
		 */
		if (find_in_list(lan1_ifnames, var_intf)) {
			return 1;
		}
	}

	return 0;
}

/* Fill the VLAN policy structure for MultiAP R1 */
static int
wbd_slave_fill_vlan_policy(ieee1905_config *config, uint16 sec_vlan_id)
{
	int ret = -1;
	unsigned short vlan_id;
	char *lan1_ifnames, *wbd_ifnames, *next_intf, *nvval;
	char var_intf[IFNAMSIZ] = {0}, prefix[IFNAMSIZ];
	ieee1905_ts_policy_t *ts_policy = NULL;
	ieee1905_ssid_type ssid;
	WBD_ENTER();

	if (!wbd_is_guest_enabled()) {
		WBD_INFO("Guest is not enabled\n");
		goto end;
	}

	lan1_ifnames = blanket_nvram_safe_get(NVRAM_LAN1_IFNAMES);
	wbd_ifnames = blanket_nvram_safe_get(WBD_NVRAM_IFNAMES);

	/* Traverse wbd_ifnames for each ifname */
	foreach(var_intf, wbd_ifnames, next_intf) {

		/* Get Prefix from OS specific interface name */
		blanket_get_interface_prefix(var_intf, prefix, sizeof(prefix));

		/* If the interface name is present in lan1_ifnames, then the guest network
		 * is present. For guest, use secondary VLAN ID else use primary VLAN ID
		 */
		if (find_in_list(lan1_ifnames, var_intf)) {
			vlan_id = sec_vlan_id;
		} else {
			vlan_id = config->prim_vlan_id;
		}

		/* Add VLAN ID to policy list. this return the pointer to the policy if added
		 * successfully or if the VLAN ID is already present in the list
		 */
		WBD_INFO("VLAN ID[%d] for ifname[%s] prefix[%s]\n", vlan_id, var_intf, prefix);
		ts_policy = i5DmAddVLANIDToList(&config->ts_policy_list, vlan_id);
		if (ts_policy == NULL) {
			WBD_WARNING("For prefix[%s] Failed to add VLAN ID[%d] to list\n",
				prefix, vlan_id);
			goto end;
		}

		memset(&ssid, 0, sizeof(ssid));
		nvval = blanket_nvram_prefix_safe_get(prefix, NVRAM_SSID);
		memcpy(ssid.SSID, nvval, sizeof(ssid.SSID));
		ssid.SSID_len = strlen(nvval);

		/* Add SSID to the VLAN policy list */
		WBD_INFO("VLAN ID[%d] for prefix[%s] ssid[%s] len[%d]\n", vlan_id, prefix,
			ssid.SSID, ssid.SSID_len);
		if (i5DmAddSSIDToList(&config->ts_policy_list, &ts_policy->ssid_list,
			&ssid) == NULL) {
			WBD_WARNING("For VLAN ID[%d] prefix[%s] Failed to add SSID[%s] of len[%d] "
				"to list\n", vlan_id, prefix, ssid.SSID, ssid.SSID_len);
			goto end;
		}
	}
	ret = WBDE_OK;
	config->flags |= I5_INIT_FLAG_TS_ACTIVE;

end:
	WBD_EXIT();
	return ret;
}
#endif /* MULTIAPR2 */

/* Read the VLAN(Traffic separation) configuration */
static void
wbd_slave_read_vlan_config(ieee1905_config *config)
{
	int ret = -1;
	uint16 sec_vlan_id;
	WBD_ENTER();

	memset(config, 0, sizeof(*config));

	ieee1905_glist_init(&config->ts_policy_list);

	wbd_get_basic_common_vlan_config(config, &sec_vlan_id);

	if (config->flags & I5_INIT_FLAG_TS_SUPPORTED) {
		ret = wbd_slave_fill_vlan_policy(config, sec_vlan_id);
	}

	if (ret != WBDE_OK) {
		i5DmTSPolicyCleanup(&config->ts_policy_list);
		WBD_INFO("Failed to load traffic separation policy either due to guest is not "
			"enabled or failed to read the data\n");
	}

	WBD_EXIT();
}

/* If vendors want to send/receive additional vendor specific TLVs in any agent messages, they can
 * register their functions here
 */
void
wbd_slave_register_vendors()
{
	wbd_slave_vndr_brcm_register();
}

/* main entry point */
int
main(int argc, char *argv[])
{
	int ret = WBDE_OK;
	int map_mode, no_bh_at_bootup;
	ieee1905_call_bks_t cbs;
	i5_dm_interface_type *i5_ifr;
	ieee1905_config config;
	char fname[WBD_MAX_BUF_256];

	/* Set Process Name */
	WBDSTRNCPY(g_wbd_process_name, "wbd_slave", sizeof(g_wbd_process_name));

	/* Copy and set file name */
	blanket_log_get_default_nvram_filename(fname, sizeof(fname));

	/* Filename exists remove to avoid duplication */
	if (remove(fname) != 0) {
		WBD_WARNING("Unable to delete %s: %s...\n", fname, strerror(errno));
	}

	map_mode = blanket_get_config_val_int(NULL, WBD_NVRAM_MULTIAP_MODE, MAP_MODE_FLAG_DISABLED);
	/* If agent is not supported exit the slave */
	if (!MAP_IS_AGENT(map_mode)) {
		WBD_WARNING("Multi-AP mode (%d) not configured as Agent...\n", map_mode);
		goto end;
	}

	memset(&cbs, 0, sizeof(cbs));

	/* Parse common cli arguments */
	wbd_parse_cli_args(argc, argv);

	WBD_INFO("WBD MAIN START...\n");

	/* Provide necessary info to debug_monitor for service restart */
	dm_register_app_restart_info(getpid(), argc, argv, NULL);

	wbd_slave_init_blanket_module();

	/* Allocate & Initialize the info structure */
	g_wbdinfo = wbd_info_init(&ret, NULL, FALSE);
	WBD_ASSERT();

	/* Allocate & Initialize Blanket Slave structure object */
	ret = wbd_ds_blanket_slave_init(g_wbdinfo);
	WBD_ASSERT();

	wbd_slave_register_ieee1905_callbacks(&cbs);

	/* Read the VLAN(Traffic separation configuration */
	wbd_slave_read_vlan_config(&config);

	ret = ieee1905_module_init(g_wbdinfo, MAP_MODE_FLAG_AGENT, 0, &cbs, &config);
	WBD_ASSERT();
	/* If the guest is enabled, clean the traffic separation policy which is loaded to
	 * local variable
	 */
	if (config.flags & I5_INIT_FLAG_TS_ACTIVE) {
		i5DmTSPolicyCleanup(&config.ts_policy_list);
	}

	/* Handle the connected and disconnected backhaul STAs at boot up time. This is required
	 * bcoz there will not be any events coming from the firmware for the BSTAs which are
	 * already associated before WBD comes up
	 */
	wbd_slave_handle_bstas_on_boot();

	/* Initialize Slave */
	ret = wbd_init_slave(g_wbdinfo);
	WBD_ASSERT();

	/* WBD & 1905 are initialized properly. Now enable signal handlers */
	signal(SIGTERM, wbd_signal_hdlr);
	signal(SIGINT, wbd_signal_hdlr);
	signal(SIGPWR, wbd_slave_tty_hdlr);

	/* If multiple backhaul STA's connected, then choose the best one and disconnect all the
	 * othe BSTAs. This is to avoid STP switching between the BSTAs
	 */
	i5_ifr = wbd_slave_choose_best_bh_sta();
	if (i5_ifr) {
		wbd_slave_disconnect_all_bstas(i5_ifr);
	}

	wbd_slave_register_vendors();

	blanket_start_multiap_messaging();

	/* Enable RSSI based backhaul STA roam and set roam trigger.
	 * Value 2 is WLC_ROAM_STATE_LOW_RSSI
	 */
	wbd_slave_set_bh_sta_roam_state(2);

	no_bh_at_bootup = blanket_get_config_val_int(NULL, NVRAM_MAP_BLOCK_BH_AT_BOOT_UP,
		WBD_DEF_BLOCK_BH_AT_BOOTUP);
	if (no_bh_at_bootup && !MAP_IS_CONTROLLER(map_mode)) {
		WBD_INFO("Blocking Backhaul bss and disabling fronthaul bss on bootup."
			" They will be restored when controller is detected\n");
		/* Block backhaul sta association to backhaul BSS */
		wbd_slave_block_unblock_backhaul_sta_assoc(0);
		/* Bring down all the fronthaul BSS if it is up */
		wbd_blanket_util_enable_disable_all_fronthaul_bss(0);
	}

	/* Main loop which keeps on checking for the timers and fd's */
	wbd_run(g_wbdinfo->hdl);

end:
	/* Exit Slave */
	WBD_INFO("Exiting the Slave\n");
	wbd_exit_slave(g_wbdinfo);
	WBD_INFO("WiFi Blanket Daemon End...\n");

	return ret;
}
