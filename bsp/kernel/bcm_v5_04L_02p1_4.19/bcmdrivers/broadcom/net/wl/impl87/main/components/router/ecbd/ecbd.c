/*
 * Linux-specific portion of ECBD (Event Callback Daemon)
 * (OS dependent file)
 *
 * Copyright 2020 Broadcom
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
 * $Id: ecbd.c $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <net/if.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <linux/un.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <ethernet.h>
#include <wlutils.h>
#include <bcmnvram.h>
#include <ctype.h>
#include <wlif_utils.h>
#include <bcmparams.h>
#include <shutils.h>
#include <security_ipc.h>
#include <sys/timerfd.h>
#include <802.11ax.h>
#include <wlcsm_lib_api.h>
#include <bcmendian.h>
#ifdef RDKB_RADIO_STATS_MEASURE /* definition changed, keep the old one as refernece */
#include "wlioctl_utils.h"
#ifdef RDKB_WLDM
#include "wldm_lib.h"
#endif
#endif /* RDKB_RADIO_STATS_MEASURE */
#include <ecbd.h>

/* from <d11.h> */
#ifndef PHY_TYPE_G
#define	PHY_TYPE_G		2
#define	PHY_TYPE_N		4	/**< N-Phy value */
#define	PHY_TYPE_AC		11	/**< AC-Phy value */
#define	PHY_TYPE_AX		13	/**< AX-Phy value */
#define	PHY_TYPE_NULL		0xf	/**< Invalid Phy value */
#endif /* PHY_TYPE_G */
/* Sync with wlc_types.h */
extern int bcm_ether_atoe(const char *p, struct ether_addr *ea);

#ifndef DOT11_RC_DISASSOC_BTM
/* TODO #include proper d11 header file */
/* 12 is unused by STA but could be used by AP/GO */
#define DOT11_RC_DISASSOC_BTM       12  /* Disassociated due to BSS Transition Magmt */
#endif /* DOT11_RC_DISASSOC_BTM */

#ifdef RDKB_WLDM
static int ecbd_get_BSSTransitionActivation(char *ifname, BOOL *activatep);
#else
extern int wlcsm_mngr_wifi_getBSSTransitionActivation(unsigned int ap_idx, BOOL *activatep);
#endif

/* as in wifi_hal.c */
#define WLAN_CM_RDK_RRM_LOG_FILE_NVRAM "rrm_rdk_log_file"
#define WLAN_CM_RDK_RRM_LOG_FILE "/tmp/rdk_log_rrm_11k.txt"
#define RDKB_RRM_STRING_LENGTH 64

/* some global variables */
#define STA_INFO_UPDATE_RATE_TIMER 10
#define CMDBUF_LEN 256
static ecbd_stalist_t *ecbd_stalist_hdr = NULL;
static ecbd_info_t *ecbd_info = NULL;
static int ecbd_msglevel = ECBD_DEBUG_ERROR;

static int watchdog_call = 0;
static void ecbd_update_rates(void);

static wifi_steering_group_t steer_groups[MAX_STEERING_GROUP_NUM];

#ifndef RDKB_WLDM
#if defined(BUILD_NO_CMS)
/* copy from wlevt.c, use this flag to define ltc pthread (to Comcast/Plume WM callback) */
#include <pthread.h>
static pthread_t ltcWlevtThreadId = (pthread_t)NULL;
static void ltc_socket_create(void);
#endif /* BUILD_NO_CMS */
#endif /* #ifndef RDKB_WLDM */

void ecbd_sent_msg_to_ltc(bcm_event_t *dpkt, WL_STATION_LIST_ENTRY *p_wlsta);
void ecbd_wd_sent_msg_to_ltc(ecbd_stalist_t *ecbd_sta, int event_type);
int ecbd_get_wl_stainfo(WL_STATION_LIST_ENTRY *ptr_sta, ecbd_stalist_t *sta_info);
static void ecbd_update_assoc_dev_hwm_cnt(char *nvifname, int assoc_cnt, int reset);

/* for regular repeated log */
#define WLAN_APPS_LOG_ECBD_REGULAR_TIMER 600
/* for the 1st time after associated */
#define WLAN_APPS_LOG_ECBD_INITIAL_TIMER 15
#define WLAN_APPS_LOG_ECBD_BS_DATA_FILE "/tmp/ecbd_sta_bs_data.txt"

typedef enum sta_log_type {
	STA_LOG_REGULAR,
	STA_LOG_INITIAL,
	MAX_LOG_TYPE
} sta_log_type_e;

#ifndef WLC_MAXBSSCFG
#define WLC_MAXBSSCFG	8
#endif

#ifndef WL_STA_VER_V7
#define WL_STA_VER_V7	7
#endif

#ifndef WL_STA_AID
#define WL_STA_AID(a)		((a) &~ 0xc000)
#endif

char *log_nvram_buf = NULL;
char log_buf[256] = {0};

static struct pollfd fds[ECBD_NUM_FD];
static nfds_t nfds = ECBD_NUM_FD;

#ifdef RDKB_RADIO_STATS_MEASURE
static ecbd_radio_stats_t ecbd_radio_stats_array[HAL_RADIO_NUM_RADIOS];
#endif /* RDKB_RADIO_STATS_MEASURE */

static int
ecbd_watchdog_tmr_delete(void)
{
	struct sigaction  act;
	struct itimerspec tmr;
	int ret = 0;

	/* Ignore the timeout signals. */
	sigemptyset(&act.sa_mask);
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	if (sigaction(TIMEOUT_SIGNAL, &act, NULL))
		if (!ret) {
			ECBD_PRINT_ERROR("sigaction TIMEOUT failed: %s", strerror(errno));
			ret = errno;
		}

	/* Disarm any current timeouts. */
	tmr.it_value.tv_sec = 0;
	tmr.it_value.tv_nsec = 0L;
	tmr.it_interval.tv_sec = 0;
	tmr.it_interval.tv_nsec = 0;
	if (timerfd_settime(fds[FD_TIMER].fd, TFD_TIMER_ABSTIME, &tmr, NULL))
		if (!ret) {
			ECBD_PRINT_ERROR("timerfd_settime failed: %s", strerror(errno));
			ret = errno;
		}

	return ret;
}

static int
ecbd_start_watchdog_tmr(int timerfd, unsigned int interval, long nanosec)
{
	struct itimerspec timerValue;

	bzero(&timerValue, sizeof(timerValue));
	timerValue.it_value.tv_sec = interval;
	timerValue.it_value.tv_nsec = nanosec;
	timerValue.it_interval.tv_sec = interval;
	timerValue.it_interval.tv_nsec = nanosec;

	/* start timer */
	if (timerfd_settime(timerfd, 0, &timerValue, NULL) < 0) {
		printf("could not start timer\n");
		return ECBD_FAIL;
	}

	return ECBD_SUCCESS;
}
static void
ecbd_hexdump_ascii(const char *title, const unsigned char *buf, unsigned int len)
{
	int i, llen;
	const unsigned char *pos = buf;

	printf("%s - (data len=%lu):\n", title, (unsigned long) len);
	while (len) {
		llen = len > MAX_LINE_LEN ? MAX_LINE_LEN : len;
		printf("    ");
		for (i = 0; i < llen; i++)
			printf(" %02x", pos[i]);
		for (i = llen; i < MAX_LINE_LEN; i++)
			printf("   ");
		printf("   ");
		for (i = 0; i < llen; i++) {
			if (isprint(pos[i]))
				printf("%c", pos[i]);
			else
				printf("*");
		}
		for (i = llen; i < MAX_LINE_LEN; i++)
			printf(" ");
		printf("\n");
		pos += llen;
		len -= llen;
	}
}

static void
ecbd_update_rates(void)
{
	/* Debug only, show all STA in list */
	char ioctl_buf[MAX_IOCTL_BUFLEN];
	ecbd_stalist_t *ptr;
	char buf[32], *param;
	sta_info_t *sta_info;
	int buflen, rx_rate, tx_rate;
	int ret = 0;

	ECBD_PRINT_DEBUG("\n === Update Rates === \n");

	ptr = ecbd_stalist_hdr;
	while (ptr) {
		if (!(ptr->type & STA_TYPE_ASSOC) || (ptr->state == INACTIVE)) {
			ptr = ptr->next;
			continue;
		}

		ECBD_PRINT_DEBUG("MAC=%s on ssid %s idx=%d bssidx=%d\n",
			ether_etoa(ptr->addr.octet, buf), ptr->ssid, ptr->ifidx, ptr->bsscfgidx);
		strcpy(ioctl_buf, "sta_info");
		buflen = strlen(ioctl_buf) + 1;
		param = (char *)(ioctl_buf + buflen);
		memcpy(param, &ptr->addr, ETHER_ADDR_LEN);

		ret = wl_ioctl(ptr->ifname, WLC_GET_VAR, ioctl_buf, sizeof(ioctl_buf));

		if (ret < 0) {
			ECBD_PRINT_ERROR("Err: intf:%s STA:"MACF" sta_info\n",
				ptr->ifname, ETHER_TO_MACF(ptr->addr));
		} else {
			sta_info = (sta_info_t *)ioctl_buf;
			rx_rate = sta_info->rx_rate;
			tx_rate = sta_info->tx_rate;
			if (rx_rate > ptr->maxrxrate) ptr->maxrxrate = rx_rate;
			if (tx_rate > ptr->maxtxrate) ptr->maxtxrate = tx_rate;

			/* update rx_tot_pkts/tx_tot_pkts */
			if ((ptr->tx_tot_pkts != sta_info->tx_tot_pkts) ||
				(ptr->rx_tot_pkts != sta_info->rx_tot_pkts))
				ptr->active = time(NULL);

			ptr->rx_tot_pkts = sta_info->rx_tot_pkts;
			ptr->tx_tot_pkts = sta_info->tx_tot_pkts;

			ptr->aid = sta_info->aid;
			if (sta_info->ver >= WL_STA_VER_V7) {
				sta_info_v7_t *sta_v7 = (sta_info_v7_t *)ioctl_buf;
				ptr->chanspec = sta_v7->chanspec;
			} else {
				ECBD_PRINT_INFO("sta_info->ver=%d\n", sta_info->ver);
			}
		}

		ptr = ptr->next;
	}
}

static void
ecbd_get_pktq_stats_cmd(char *ifname, char *sta_mac, char *cmdBuf)
{
	int band;
	memset(cmdBuf, 0, CMDBUF_LEN);

	if (wl_ioctl(ifname, WLC_GET_BAND, &band, sizeof(band)) < 0)
		ECBD_PRINT_ERROR("%s WLC_GET_BAND failed!!\n", ifname);
	if (band == 1) {
		sprintf(cmdBuf, "wl -i %s pktq_stats a:%s", ifname, sta_mac);
	} else {
		sprintf(cmdBuf, "wl -i %s pktq_stats n:%s", ifname, sta_mac);
	}
}

static void
ecbd_update_sta_band_info(ecbd_stalist_t *sta)
{
	int band;
	ecbd_stalist_t *ptr;

	/* update band cap info for this sta */
	if (wl_ioctl(sta->ifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
		ECBD_PRINT_ERROR("Err: intf:%s band\n", sta->ifname);
		return;
	}

	ptr = ecbd_stalist_hdr;
	if (ptr == NULL) {
		ECBD_PRINT_DEBUG("ecbd_mac_list is empty\n");
		return;
	}
	do {
		/* compare mac only, the sta may be on different interface */
		if (eacmp(&(ptr->addr), &(sta->addr)) == 0) {
			if (band == WLC_BAND_5G)
				ptr->band |= WLC_BAND_5G;
			else if (band == WLC_BAND_2G)
				ptr->band |= WLC_BAND_2G;

			ECBD_PRINT_INFO("STA "MACF" ifidx=%d bssidx=%d band=%d\n",
				ETHER_TO_MACF(ptr->addr), ptr->ifidx, ptr->bsscfgidx, ptr->band);
		}
		ptr = ptr->next;
	} while (ptr != NULL);
}

static void
ecbd_show_stalist(void)
{
	/* Debug only, show all STA in list */
	ecbd_stalist_t *ptr;
	char buf[32];
	int total = 0;

	if (!(ecbd_msglevel & ECBD_DEBUG_INFO))
		return;
	ECBD_PRINT_INFO("\n === Show all STA info === \n");

	ptr = ecbd_stalist_hdr;
	while (ptr) {
		total ++;
		ECBD_PRINT_INFO("MAC=%s on ssid %s idx=%d bssidx=%d assoc_time=%lu disassoc_time=%lu state=%d "
			"type=0x%x RMcap=0x%02x%02x%02x%02x%02x\n",
			ether_etoa(ptr->addr.octet, buf), ptr->ssid, ptr->ifidx, ptr->bsscfgidx,
			ptr->assoc_time, ptr->disassoc_time, ptr->state, ptr->type,
			ptr->rm_cap.cap[0], ptr->rm_cap.cap[1], ptr->rm_cap.cap[2], ptr->rm_cap.cap[3], ptr->rm_cap.cap[4]);
		ptr = ptr->next;
	}

	ECBD_PRINT_INFO("Total STA count: %d\n", total);
}

static ecbd_stalist_t *
ecbd_retrieve_sta_info(struct ether_addr *sta_addr, uint8 ifidx, uint8 bsscfgidx)
{
	ecbd_stalist_t *ptr;

	ptr = ecbd_stalist_hdr;
	if (ptr == NULL) {
		ECBD_PRINT_DEBUG("ecbd_mac_list is empty\n");
		goto exit;
	}
	do {
		ECBD_PRINT_DEBUG("Search: STA "MACF" ifidx=%d bssidx=%d\n",
			ETHER_TO_MACF(ptr->addr), ptr->ifidx, ptr->bsscfgidx);

		if ((eacmp(&(ptr->addr), sta_addr) == 0) &&
			(ptr->ifidx == ifidx) && (ptr->bsscfgidx == bsscfgidx)) {
			break;
		}
		ptr = ptr->next;
	} while (ptr != NULL);

exit:
	return ptr;
}

/* Extract one tlv from data */
/* Example for RMCapabilities: see ecbd_update_sta_RMcap below
 */
static int
ecbd_extract_from_tlv(uint8 *dp, uint8 max_dlen, uint8 in_type, uint8 in_maxlen, uint8 *out_valp, uint8 *out_len)
{
	uint8 atype;
	uint8 alen;
	uint8 *tlvp;  /* tlv pointer */

	tlvp = dp;
	while (tlvp != NULL) {
		atype = *tlvp++;
		alen = *tlvp++;
		if (atype == in_type) {
			ECBD_PRINT_INFO("%s Found type=0x%x len=%d in_maxlen=%d \n", __FUNCTION__, atype, alen, in_maxlen);
			if (in_maxlen >= alen) {
				memcpy(out_valp, tlvp, alen);
				*out_len = alen;
				return ECBD_SUCCESS;
			}
			else {
				ECBD_PRINT_ERROR("%s Len error type=0x%x len=%d in_maxlen=%d \n",
					__FUNCTION__, atype, alen, in_maxlen);
				return ECBD_FAIL;
			}
		}
		else {
			/* to next tlv */
			tlvp = tlvp + alen;
			if ((tlvp - dp) >= max_dlen) {
				ECBD_PRINT_ERROR("%s Not Found type=0x%x  \n", __FUNCTION__, atype);
				return ECBD_FAIL;
			}
		}
	}
	return ECBD_FAIL;
}

static int
ecbd_update_sta_RMcap(ecbd_stalist_t *sta_info, bcm_event_t *evtp)
{
	dot11_rrm_cap_ie_t rm_cap;
	uint8 rm_caplen;
	uint8 *dp;
	int ret;

	dp = (uint8 *)(&(evtp->event)) + sizeof(wl_event_msg_t);
	ret = ecbd_extract_from_tlv(dp, ntohl(evtp->event.datalen), DOT11_MNG_RRM_CAP_ID, DOT11_RRM_CAP_LEN,
		(uint8 *)(&rm_cap), &rm_caplen);
	if (ret == ECBD_SUCCESS) {
		memcpy((uint8 *)(&sta_info->rm_cap), (uint8 *)(&rm_cap), rm_caplen);
		ECBD_PRINT_INFO("%s Populated rm_cap = %02x %02x %02x %02x %02x \n", __FUNCTION__, sta_info->rm_cap.cap[0],
			sta_info->rm_cap.cap[1], sta_info->rm_cap.cap[2], sta_info->rm_cap.cap[3], sta_info->rm_cap.cap[4]);
	}
	return (ret);
}

/* add addr to stalist */
static ecbd_stalist_t *
ecbd_add_stalist(char *ssid, struct ether_addr *sta_addr, uint8 ifidx, uint8 bsscfgidx, char *ifname, uint8 type)
{
	ecbd_stalist_t *ptr;
	char buf[32] = {0}, cmdBuf[CMDBUF_LEN] = {0};
	FILE *fp;

	ECBD_PRINT_DEBUG("\n === Add STA %s === \n", ssid);

	/* adding to stalist */
	ptr = ecbd_stalist_hdr;

	while (ptr) {
		if ((eacmp(&(ptr->addr), sta_addr) == 0) &&
			(ptr->ifidx == ifidx) &&
			(ptr->bsscfgidx == bsscfgidx)) {
			ECBD_PRINT_INFO("update ssid %s, MAC=%s\n",
				ssid, ether_etoa(sta_addr->octet, buf));
			break;
		}
		ptr = ptr->next;
	}

	if (!ptr) {
		/* add new sta to stalist */
		ptr = malloc(sizeof(ecbd_stalist_t));
		if (!ptr) {
			ECBD_PRINT_ERROR("Exiting malloc failure\n");
			return NULL;
		}
		/* for new item */
		memset(ptr, 0, sizeof(ecbd_stalist_t));
		memcpy(&ptr->addr, sta_addr, sizeof(struct ether_addr));
		ptr->next = ecbd_stalist_hdr;
		ecbd_stalist_hdr = ptr;

		ptr->ifidx = ifidx;
		ptr->bsscfgidx = bsscfgidx;
		/* ptr->security = 0; */
	}

	/* update info for both old and new sta */
	if (ssid) {
		strncpy(ptr->ssid, ssid, sizeof(ptr->ssid));
		ptr->ssid[sizeof(ptr->ssid)-1] = '\0';
	}

	if (ifname) {
		strncpy(ptr->ifname, ifname, sizeof(ptr->ifname));
		ptr->ifname[sizeof(ptr->ifname)-1] = '\0';
	}

	if (type & STA_TYPE_ASSOC) {
		ptr->assoc_time = time(NULL);
		ptr->disassoc_time = 0;
		ptr->state = ACTIVE;
		ptr->event_sent = 0;
	} else if (type & STA_TYPE_MACLIST) {
		ptr->maclist_time = time(NULL); /* for maclist timeout */
	}

	ptr->type |= type;
	/* TODO: other info */

	ECBD_PRINT_INFO("add ssid %s, MAC=%s, type=0x%x\n", ssid, ether_etoa(sta_addr->octet, buf), ptr->type);

	if (ptr->type & STA_TYPE_ASSOC) {
		ecbd_get_pktq_stats_cmd(ptr->ifname, ether_etoa(sta_addr->octet, buf), cmdBuf);
		if ((fp = popen(cmdBuf, "r")) == NULL) {
			ECBD_PRINT_ERROR("file cannot be read %s failed!!\n", cmdBuf);
		}
		else {
			pclose(fp);
		}
	}

	ecbd_show_stalist();

	return ptr;
}

/* remove addr from stalist */
static void
ecbd_del_stalist(struct ether_addr *sta_addr, uint8 ifidx, uint8 bsscfgidx, uint8 type)
{
	ecbd_stalist_t *ptr, *prev;
	char buf[32];
	int found = 0;

	ptr = ecbd_stalist_hdr;

	if (ptr == NULL) {
		ECBD_PRINT_DEBUG("ecbd_mac_list is empty\n");
		return;
	}

	if ((eacmp(&(ptr->addr), sta_addr) == 0) &&
		(ptr->ifidx == ifidx) && (ptr->bsscfgidx == bsscfgidx)) {
		/* this is the first one */
		ptr->type &= ~type;
		if (!ptr->type)
			ecbd_stalist_hdr = ptr->next;
		found = 1;
	}
	else {
		prev = ptr;
		ptr = ptr->next;

		while (ptr) {
			if ((eacmp(&(ptr->addr), sta_addr) == 0) &&
				(ptr->ifidx == ifidx) && (ptr->bsscfgidx == bsscfgidx)) {
				ptr->type &= ~type;
				if (!ptr->type)
					prev->next = ptr->next;
				found = 1;
				break;
			}
			prev = ptr;
			ptr = ptr->next;
		}
	}

	if (ptr && found) {
		if (!ptr->type) {
			ECBD_PRINT_INFO("Free ssid %s, MAC=%s, assoc_time=%lu type=0x%x(%x)\n",
				ptr->ssid, ether_etoa(ptr->addr.octet, buf), ptr->assoc_time, ptr->type, type);
			free(ptr);
			ptr = NULL;
		}
	}
	else {
		ECBD_PRINT_INFO("STA %s not found\n", ether_etoa(sta_addr->octet, buf));
	}

	ecbd_show_stalist();

	return;
}

static int
ecbd_notify_subscriber(void *evt, uint32 len, int sock_fd)
{
	int flags = MSG_NOSIGNAL;

	if (send(sock_fd, evt, len, flags) == -1) {
		perror("send failed: ");
		return -errno;
	}
	return ECBD_SUCCESS;
}

static void
ecbd_close_cb_dsockets(void)
{
	int i = 0;
	wifi_hal_cb_type_t cb_type;

	for (cb_type = 0; cb_type < WIFI_HAL_MAX_CB_TYPE; cb_type++) {
		switch (cb_type) {
			case WIFI_HAL_CB_STA_CONN:
				for (i = 0; i < ecbd_info->sta_conn_cb_subscriber_fds.count; i++) {
					if (ecbd_info->sta_conn_cb_subscriber_fds.FD[i] >= 0) {
						close(ecbd_info->sta_conn_cb_subscriber_fds.FD[i]);
						ecbd_info->sta_conn_cb_subscriber_fds.FD[i] = -1;
					}
				}
				unlink(ecbd_info->sta_conn_cb_subscriber_fds.sock_path);
				break;

			case WIFI_HAL_CB_ASSOC_DEV:
				for (i = 0; i < ecbd_info->assoc_dev_cb_subscriber_fds.count; i++) {
					if (ecbd_info->assoc_dev_cb_subscriber_fds.FD[i] >= 0) {
						close(ecbd_info->assoc_dev_cb_subscriber_fds.FD[i]);
						ecbd_info->assoc_dev_cb_subscriber_fds.FD[i] = -1;
					}
				}
				unlink(ecbd_info->assoc_dev_cb_subscriber_fds.sock_path);
				break;

			case WIFI_HAL_CB_AUTH_FAIL:
				for (i = 0; i < ecbd_info->auth_fail_cb_subscriber_fds.count; i++) {
					if (ecbd_info->auth_fail_cb_subscriber_fds.FD[i] >= 0) {
						close(ecbd_info->auth_fail_cb_subscriber_fds.FD[i]);
						ecbd_info->auth_fail_cb_subscriber_fds.FD[i] = -1;
					}
				}
				unlink(ecbd_info->auth_fail_cb_subscriber_fds.sock_path);
				break;

			case WIFI_HAL_CB_MESH:
				for (i = 0; i < ecbd_info->mesh_cb_subscriber_fds.count; i++) {
					if (ecbd_info->mesh_cb_subscriber_fds.FD[i] >= 0) {
						close(ecbd_info->mesh_cb_subscriber_fds.FD[i]);
						ecbd_info->mesh_cb_subscriber_fds.FD[i] = -1;
					}
				}
				unlink(ecbd_info->mesh_cb_subscriber_fds.sock_path);
				break;

			case WIFI_HAL_CB_RRM_BCNREP:
				for (i = 0; i < ecbd_info->bcn_report_cb_subscriber_fds.count; i++) {
					if (ecbd_info->bcn_report_cb_subscriber_fds.FD[i] >= 0) {
						close(ecbd_info->bcn_report_cb_subscriber_fds.FD[i]);
						ecbd_info->bcn_report_cb_subscriber_fds.FD[i] = -1;
					}
				}
				unlink(ecbd_info->bcn_report_cb_subscriber_fds.sock_path);
				break;

			case WIFI_HAL_CB_BSSTRANS:
				for (i = 0; i < ecbd_info->bsstrans_cb_subscriber_fds.count; i++) {
					if (ecbd_info->bsstrans_cb_subscriber_fds.FD[i] >= 0) {
						close(ecbd_info->bsstrans_cb_subscriber_fds.FD[i]);
						ecbd_info->bsstrans_cb_subscriber_fds.FD[i] = -1;
					}
				}
				unlink(ecbd_info->bsstrans_cb_subscriber_fds.sock_path);
				break;

			case WIFI_HAL_CB_DPP:
				for (i = 0; i < ecbd_info->dpp_cb_subscriber_fds.count; i++) {
					if (ecbd_info->dpp_cb_subscriber_fds.FD[i] >= 0) {
						close(ecbd_info->dpp_cb_subscriber_fds.FD[i]);
						ecbd_info->dpp_cb_subscriber_fds.FD[i] = -1;
					}
				}
				unlink(ecbd_info->dpp_cb_subscriber_fds.sock_path);
				break;

			case WIFI_HAL_CB_CH_CHG:
				for (i = 0; i < ecbd_info->ch_chg_cb_subscriber_fds.count; i++) {
					if (ecbd_info->ch_chg_cb_subscriber_fds.FD[i] >= 0) {
						close(ecbd_info->ch_chg_cb_subscriber_fds.FD[i]);
						ecbd_info->ch_chg_cb_subscriber_fds.FD[i] = -1;
					}
				}
				unlink(ecbd_info->ch_chg_cb_subscriber_fds.sock_path);
				break;

			case WIFI_HAL_MAX_CB_TYPE:
			default:		/* Should be removed once all callbacks are added */
				break;
		}
	}
}

static int
ecbd_notify_wifi_hal_subscribers(void *evt, uint32 len, wifi_hal_cb_type_t cb_type)
{
	int ret = ECBD_SUCCESS;
	int i;

	if ((evt == NULL) || (len == 0))
		return ECBD_FAIL;

	switch (cb_type) {
		case WIFI_HAL_CB_STA_CONN:
			for (i = 0; i < ecbd_info->sta_conn_cb_subscriber_fds.count; i++) {
				if (ecbd_info->sta_conn_cb_subscriber_fds.FD[i] >= 0) {
					ECBD_PRINT_ERROR("notifying STA_CONN subscriber[%d] with FD - %d \n",
						i, ecbd_info->sta_conn_cb_subscriber_fds.FD[i]);
					ret = ecbd_notify_subscriber(evt, len,
						ecbd_info->sta_conn_cb_subscriber_fds.FD[i]);
					if (ret != ECBD_SUCCESS) {
						if (ret == -EPIPE) {
							perror("STA_CONN: Peer socket got closed:");
							close(ecbd_info->sta_conn_cb_subscriber_fds.FD[i]);
							ecbd_info->sta_conn_cb_subscriber_fds.FD[i] = -1;
						}
					}
				}
			}
		break;

		case WIFI_HAL_CB_ASSOC_DEV:
			for (i = 0; i < ecbd_info->assoc_dev_cb_subscriber_fds.count; i++) {
				if (ecbd_info->assoc_dev_cb_subscriber_fds.FD[i] >= 0) {
					ECBD_PRINT_ERROR("notifying ASSOC_DEV subscriber[%d] with FD - %d \n",
						i, ecbd_info->assoc_dev_cb_subscriber_fds.FD[i]);
					ret = ecbd_notify_subscriber(evt, len,
						ecbd_info->assoc_dev_cb_subscriber_fds.FD[i]);
					if (ret != ECBD_SUCCESS) {
						if (ret == -EPIPE) {
							perror("ASSOC_DEV: Peer socket got closed:");
							close(ecbd_info->assoc_dev_cb_subscriber_fds.FD[i]);
							ecbd_info->assoc_dev_cb_subscriber_fds.FD[i] = -1;
						}
					}
				}
			}
		break;

		case WIFI_HAL_CB_AUTH_FAIL:
			for (i = 0; i < ecbd_info->auth_fail_cb_subscriber_fds.count; i++) {
				if (ecbd_info->auth_fail_cb_subscriber_fds.FD[i] >= 0) {
					ECBD_PRINT_ERROR("notifying AUTH_FAIL subscriber[%d] with FD - %d \n",
						i, ecbd_info->auth_fail_cb_subscriber_fds.FD[i]);
					ret = ecbd_notify_subscriber(evt, len,
						ecbd_info->auth_fail_cb_subscriber_fds.FD[i]);
					if (ret != ECBD_SUCCESS) {
						if (ret == -EPIPE) {
							perror("AUTH_FAIL: Peer socket got closed:");
							close(ecbd_info->auth_fail_cb_subscriber_fds.FD[i]);
							ecbd_info->auth_fail_cb_subscriber_fds.FD[i] = -1;
						}
					}
				}
			}
		break;

		case WIFI_HAL_CB_MESH:
			for (i = 0; i < ecbd_info->mesh_cb_subscriber_fds.count; i++) {
				if (ecbd_info->mesh_cb_subscriber_fds.FD[i] >= 0) {
					ECBD_PRINT_ERROR("notifying MESH subscriber[%d] with FD - %d \n",
						i, ecbd_info->mesh_cb_subscriber_fds.FD[i]);
					ret = ecbd_notify_subscriber(evt, len,
						ecbd_info->mesh_cb_subscriber_fds.FD[i]);
					if (ret != ECBD_SUCCESS) {
						if (ret == -EPIPE) {
							perror("MESH: Peer socket got closed:");
							close(ecbd_info->mesh_cb_subscriber_fds.FD[i]);
							ecbd_info->mesh_cb_subscriber_fds.FD[i] = -1;
						}
					}
				}
			}
		break;

		case WIFI_HAL_CB_RRM_BCNREP:
			for (i = 0; i < ecbd_info->bcn_report_cb_subscriber_fds.count; i++) {
				if (ecbd_info->bcn_report_cb_subscriber_fds.FD[i] >= 0) {
					ECBD_PRINT_ERROR("notifying RRM_BCNREP subscriber[%d] with FD - %d \n",
						i, ecbd_info->bcn_report_cb_subscriber_fds.FD[i]);
					ECBD_PRINT_DUMP("ecbd_notify_wifi_hal_subscribers-WIFI_HAL_CB_RRM_BCNREP", evt, 128);
					ret = ecbd_notify_subscriber(evt, len,
						ecbd_info->bcn_report_cb_subscriber_fds.FD[i]);
					if (ret != ECBD_SUCCESS) {
						if (ret == -EPIPE) {
							perror("RRM_BCNREP: Peer socket got closed:");
							close(ecbd_info->bcn_report_cb_subscriber_fds.FD[i]);
							ecbd_info->bcn_report_cb_subscriber_fds.FD[i] = -1;
						}
					}
				}
			}
			ECBD_PRINT_DEBUG("%s Done ecbd_notify_wifi_hal_subscribers WIFI_HAL_CB_RRM_BCNREP \n", __FUNCTION__);
		break;

		case WIFI_HAL_CB_BSSTRANS:
			for (i = 0; i < ecbd_info->bsstrans_cb_subscriber_fds.count; i++) {
				if (ecbd_info->bsstrans_cb_subscriber_fds.FD[i] >= 0) {
					ECBD_PRINT_INFO("notifying BSSTRANS subscriber[%d] with FD - %d len=%d\n",
						i, ecbd_info->bsstrans_cb_subscriber_fds.FD[i], len);
					ECBD_PRINT_DUMP("ecbd_notify_wifi_hal_subscribers-BSSTRANS", evt, len);
					ret = ecbd_notify_subscriber(evt, len,
						ecbd_info->bsstrans_cb_subscriber_fds.FD[i]);
					if (ret != ECBD_SUCCESS) {
						if (ret == -EPIPE) {
							perror("BSSTRANS: Peer socket got closed:");
							close(ecbd_info->bsstrans_cb_subscriber_fds.FD[i]);
							ecbd_info->bsstrans_cb_subscriber_fds.FD[i] = -1;
						}
					}
					else {
						ECBD_PRINT_INFO("notifying BSSTRANS subscriber[%d] with FD - %d len=%d SUCC\n",
							i, ecbd_info->bsstrans_cb_subscriber_fds.FD[i], len);
					}
				}
			}
		break;

		case WIFI_HAL_CB_DPP:
			for (i = 0; i < ecbd_info->dpp_cb_subscriber_fds.count; i++) {
				if (ecbd_info->dpp_cb_subscriber_fds.FD[i] >= 0) {
					ECBD_PRINT_INFO("notifying DPP subscriber[%d] with FD - %d len=%d\n",
						i, ecbd_info->dpp_cb_subscriber_fds.FD[i], len);
					ECBD_PRINT_DUMP("ecbd_notify_wifi_hal_subscribers-DPP", evt, len);
					ret = ecbd_notify_subscriber(evt, len,
						ecbd_info->dpp_cb_subscriber_fds.FD[i]);
					if (ret != ECBD_SUCCESS) {
						if (ret == -EPIPE) {
							perror("DPP: Peer socket got closed:");
							close(ecbd_info->dpp_cb_subscriber_fds.FD[i]);
							ecbd_info->dpp_cb_subscriber_fds.FD[i] = -1;
						}
					}
					else {
						ECBD_PRINT_INFO("notifying DPP subscriber[%d] with FD - %d len=%d SUCC\n",
							i, ecbd_info->dpp_cb_subscriber_fds.FD[i], len);
					}
				}
			}
		break;

		case WIFI_HAL_CB_CH_CHG:
			for (i = 0; i < ecbd_info->ch_chg_cb_subscriber_fds.count; i++) {
				if (ecbd_info->ch_chg_cb_subscriber_fds.FD[i] >= 0) {
					ECBD_PRINT_INFO("notifying CH_CHG subscriber[%d] with FD - %d len=%d\n",
						i, ecbd_info->ch_chg_cb_subscriber_fds.FD[i], len);
					ECBD_PRINT_DUMP("ecbd_notify_wifi_hal_subscribers-CH_CHG", evt, len);
					ret = ecbd_notify_subscriber(evt, len,
						ecbd_info->ch_chg_cb_subscriber_fds.FD[i]);
					if (ret != ECBD_SUCCESS) {
						if (ret == -EPIPE) {
							perror("CH_CHG: Peer socket got closed:");
							close(ecbd_info->ch_chg_cb_subscriber_fds.FD[i]);
							ecbd_info->ch_chg_cb_subscriber_fds.FD[i] = -1;
						}
					}
					else {
						ECBD_PRINT_INFO("notifying CH_CHG subscriber[%d] with FD - %d len=%d SUCC\n",
							i, ecbd_info->ch_chg_cb_subscriber_fds.FD[i], len);
					}
				}
			}
		break;

		default:	/* Should be removed once all callbacks are added */
		break;
	}
	return ret;
}

int
ecbd_notify_wifi_hal(void *evt, uint32 len, uint16 port)
{
	struct sockaddr_in sockaddr;
	int sentBytes = 0;
	int ecbd_sock = -1;

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sockaddr.sin_port = htons(port);

	if ((ecbd_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		ECBD_PRINT_ERROR("%s@%d Unable to create socket", __FUNCTION__, __LINE__);
		return ECBD_FAIL;
	}

	if (ecbd_sock >= 0) {
		sentBytes = sendto(ecbd_sock, evt, len, 0,
				(struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in));

		if (sentBytes != len) {
			ECBD_PRINT_ERROR("UDP send failed; sendingBytes[%d], sentBytes[%d]\n",
				len, sentBytes);
		}
	}

	if (ecbd_sock >= 0) {
		close(ecbd_sock);
	}

	return ECBD_SUCCESS;
}

static int
ecbd_get_apIndex(char ifname[])
{
	int unit = 0, subunit = 0;

	if (get_ifname_unit(ifname, &unit, &subunit)) {
		ECBD_PRINT_ERROR("%s@%d: Fail to get ifname unit %s\n",  __FUNCTION__, __LINE__, ifname);
		return 0;
	}

	if (unit >= 2)
		return (unit * HAL_AP_NUM_APS_PER_RADIO) + (subunit > 0 ? subunit : 0);

	if (subunit > 0)
		return unit + subunit*2;
	else
		return unit;
}

static int
ecbd_log_rrm_event(char *logStr)
{
	char *str = NULL;
	FILE *rrm_log_fd = NULL;
	char date_str[RDKB_RRM_STRING_LENGTH] = {0};
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	if ((str = wlcsm_nvram_get(WLAN_CM_RDK_RRM_LOG_FILE_NVRAM))) {
		rrm_log_fd = fopen(str, "a");
	}
	else {
		rrm_log_fd = fopen(WLAN_CM_RDK_RRM_LOG_FILE, "a");
	}

	if (rrm_log_fd != NULL) {
		snprintf(date_str, RDKB_RRM_STRING_LENGTH, "%d-%d-%d %d:%d:%d\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		fprintf(rrm_log_fd, "%9lu %s %s \n", (unsigned long)(time(NULL)), date_str, logStr);
		fclose(rrm_log_fd);
	}
	return 0;
}

static int
ecbd_get_wsec(char *ifname, uint32 *wsec)
{
	int err = 0;
	int32 val;

	err = wl_iovar_getint(ifname, "wsec", &val);
	if (!err) {
		*wsec = val;
		ECBD_PRINT_INFO("%s: get wsec=%d for %s\n",  __FUNCTION__, val, ifname);
	} else {
		ECBD_PRINT_ERROR("%s: fail to get wsec for %s\n",  __FUNCTION__, ifname);
	}

	return err;
}

static int
ecbd_get_wpa_auth(char *ifname, uint32 *wpa_auth)
{
	int err = 0;
	int32 val;

	err = wl_iovar_getint(ifname, "wpa_auth", &val);
	if (!err) {
		*wpa_auth = val;
		ECBD_PRINT_INFO("%s: get wpa_auth=%d for %s\n",  __FUNCTION__, val, ifname);
	} else {
		ECBD_PRINT_ERROR("%s: fail to get wpa_auth for %s\n",  __FUNCTION__, ifname);
	}

	return err;
}

static int
ecbd_is_open_security(char *ifname)
{
	int ret = 0; /* default: not open mode */
	uint32 wsec = 0;
	uint32 wpa_auth = 0;

	if (ecbd_get_wsec(ifname, &wsec)) {
		return ret;
	}

	if (ecbd_get_wpa_auth(ifname, &wpa_auth)) {
		return ret;
	}

	if ((wsec == 0) || (wpa_auth == 0))
		ret = 1;

	ECBD_PRINT_INFO("%s: security open=%d for %s\n",  __FUNCTION__, ret, ifname);
	return ret;
}

#ifdef RDKB_WLDM
static int
ecbd_get_BSSTransitionActivation(char *ifname, BOOL *activatep)
{
	int val;

	if (wl_iovar_getint(ifname, "wnm", &val) < 0) {
		ECBD_PRINT_ERROR("%s fail to getBSSTransitionActivation on %s\n",
			__FUNCTION__, ifname);
		return -1;
	}

	*activatep = (val & WL_WNM_BSSTRANS) ? 1 : 0;
	return 0;
}
#endif /* RDKB_WLDM */

static void
ecbd_handle_probe_event(int bytes, bcm_event_t *dpkt)
{
	uchar *ptr = (uchar *)dpkt, *pbody;
	char buf[32], ssid[MAX_SSID_LEN + 1], ap_ifname[BCM_MSG_IFNAME_MAX + 1], nvram_name[64];
	uint8 ssid_len;
	BOOL bcast = FALSE, ssid_bcast = FALSE;
	uint32 group_idx;
	int ap_idx, rssi, data_len, noise = 0, sub_idx, val, found = 0;
	ecbd_stalist_t *sta_info = NULL;
	wlc_ssid_t ap_ssid;
	wl_event_rx_frame_data_t *rxframe_data;
	ecbd_stalist_t sta_tmp; /* for sta not in associated list */
	struct dot11_management_header *hdr;
	struct ether_addr *da;

	/* Currently wifi driver only send the event via primary interface */
	/* check all virtual interfaces per ssid to determine subindex */

	data_len = IFNAMSIZ + BCM_EVENT_HEADER_LEN + sizeof(wl_event_rx_frame_data_t);

	/* hard code &hdr->da to check broadcast */
	if (bytes > data_len) {
		hdr = (struct dot11_management_header *)
			(ptr + BCM_EVENT_HEADER_LEN + sizeof(wl_event_rx_frame_data_t));
		da = &(hdr->da);
		ECBD_PRINT_PROBE("da mac="MACF" for ProbeReq\n", ETHER_TO_MACF(*da));
		if (ETHER_ISBCAST(da))
			bcast = TRUE;
	}

	/* read STA's ssid and check bcast */
	ssid[0] = 0;  /* read STA's SSID from frame */
	ssid_len = 0;
	if (bytes > (data_len + sizeof(struct dot11_management_header))) {
		pbody = (uchar*)hdr + sizeof(struct dot11_management_header);
		if (*pbody == 0) {
			/* SSID IE */
			ssid_len = *(pbody + 1);
			if (ssid_len == 0)
				bcast = TRUE;
			else if (ssid_len < sizeof(ssid)) {
				strncpy(ssid, (char*)(pbody + 2), ssid_len);
				ssid[ssid_len] = '\0';
				ECBD_PRINT_PROBE("WLC_E_PROBREQ_MSG_RX: STA ssid=%s\n", ssid);
				if (strncmp(ssid, "Broadcast", sizeof("Broadcast") - 1) == 0) {
					bcast = TRUE;
					ssid_bcast = TRUE;
				}
			}
		}
	}

	/* Check all interfaces on the band */
	for (sub_idx = 0; sub_idx < WLC_MAXBSSCFG; sub_idx++) {

		if (found)
			break; /* do once if a particular ap found */

		if (sub_idx == 0) {
			snprintf(ap_ifname, sizeof(ap_ifname), "%s", dpkt->event.ifname);
			/* in case driver send the event via a real vap in the future */
			if (strchr(ap_ifname, '.'))
				found = 1; /* do once */
		}
		else {
			snprintf(ap_ifname, sizeof(ap_ifname), "wl%d.%d", dpkt->event.ifidx, sub_idx);
		}
		ECBD_PRINT_DEBUG("WLC_E_PROBREQ_MSG_RX: ap_ifname=%s bcast=%d STA ssid=%s len=%d (%d)\n",
			ap_ifname, bcast, ssid, ssid_len, found);

		snprintf(nvram_name, sizeof(nvram_name), "%s_bss_enabled", ap_ifname);
		if (!nvram_match(nvram_name, "1")) {
			ECBD_PRINT_DEBUG("bss_enabled not set for %s\n", nvram_name);
			continue;
		}

		/* skip if the interface is not up */
		if (wl_ioctl(ap_ifname, WLC_GET_UP, &val, sizeof(val)) < 0 || !val)
			continue;

		ap_ssid.SSID[0] = '\0';
		ap_ssid.SSID_len = 0;

		if (wl_ioctl(ap_ifname, WLC_GET_SSID, &ap_ssid, sizeof(ap_ssid)) < 0) {
			ECBD_PRINT_ERROR("Fail to get ssid for %s\n", ap_ifname);
			continue; /* try next ap */
		}
		ap_ssid.SSID[ap_ssid.SSID_len] = '\0';

		ECBD_PRINT_DEBUG("ssid=%s len=%d on %s (subidx=%d) sta=%s bcast=%d\n",
			ap_ssid.SSID, ap_ssid.SSID_len, ap_ifname, sub_idx, ssid, bcast);

		/* consider the ap is not configured under this condition */
		if (ap_ssid.SSID_len == 0)
			continue;

		/* only check the matched ap for specific STA SSID */
		if (ssid_len && !ssid_bcast) {
			if (strcmp(ssid, (char *)(ap_ssid.SSID)) != 0)
				continue;

			found = 1;
			ECBD_PRINT_PROBE("Found ssid=%s on %s (subidx=%d)\n",
				ssid, ap_ifname, sub_idx);
		}

		/* convert to actual bsscfgidx */
		dpkt->event.bsscfgidx = sub_idx;
		/* re-calculate ap_idx according to actual ifname */
		ap_idx = ecbd_get_apIndex(ap_ifname);

		/* avoid flooding */
		if (ecbd_find_ap_cfg(dpkt->event.ifidx, dpkt->event.bsscfgidx, &group_idx) == NULL) {
			ECBD_PRINT_DEBUG("WLC_E_PROBREQ_MSG_RX: ap_idx=%d is not defined "
				"in a mesh group\n", ap_idx);
			continue;
		}

		if ((sta_info = ecbd_retrieve_sta_info(&(dpkt->event.addr),
			dpkt->event.ifidx, dpkt->event.bsscfgidx)) == NULL) {
			sta_info = &sta_tmp;

			memset(sta_info, 0, sizeof(ecbd_stalist_t));
			memcpy(&sta_info->addr, &(dpkt->event.addr), sizeof(struct ether_addr));
			memcpy(sta_info->ifname, ap_ifname, sizeof(sta_info->ifname) - 1);
			sta_info->ifname[sizeof(sta_info->ifname) - 1] = '\0';
			sta_info->ifidx = dpkt->event.ifidx;
			sta_info->bsscfgidx = dpkt->event.bsscfgidx;
		}

		if (bytes >= data_len) {
			rxframe_data = (wl_event_rx_frame_data_t *)(ptr + BCM_EVENT_HEADER_LEN);
			rssi = htonl(rxframe_data->rssi);

			if (ecbd_get_noise(sta_info->ifname, &noise) == ECBD_FAIL) {
				ECBD_PRINT_ERROR("Err: intf:%s noise\n", sta_info->ifname);
				noise = -90; /* use a default value */
			}

			/* convert to snr */
			// rssi = rssi - noise;
			sta_info->rssi = (rssi > noise) ? (rssi - noise) : 0;
			ECBD_PRINT_PROBE("WLC_E_PROBREQ_MSG_RX: rssi=%d (%d) noise=%d snr=%d\n",
				rxframe_data->rssi, rssi, noise, sta_info->rssi);
		}

		/* only send steering event for STA which configured by ClientSet */
		if ((sta_info->type & STA_TYPE_CLIENT_SET) && sta_info->cli_cfg) {
			int32 rssi_change = WIFI_STEERING_RSSI_UNCHANGED;
			time_t now = time(NULL);
			int send_event = 1; /* flag to send steering probe request event to wifi_hal */

			if (ssid_len != 0) {
				if (strcmp(ssid, (char *)(ap_ssid.SSID)) == 0) {
					/* per Comcast/Plume, directed probe is not broadcast */
					bcast = FALSE;
				}
				else if (strcmp(ssid, "Broadcast"))
					send_event = 0;
			}

			/* send probereq event */
			ECBD_PRINT_DEBUG("WLC_E_PROBREQ_MSG_RX: ptr=%p rxframe_data=%p "
				"bytes=%d %d %d %d "
				"group_idx=%d apidx=%d bcast=%d type=0x%x send_event=%d\n",
				ptr, rxframe_data, bytes, IFNAMSIZ, BCM_EVENT_HEADER_LEN,
				sizeof(wl_event_rx_frame_data_t), group_idx, ap_idx, bcast,
				sta_info->type, send_event);

			ECBD_PRINT_DUMP("REVD Probe_RX:", ptr, bytes);

			/* use timestampe to reduce probe request event flooding for same STA */
			/* Plume requests to send all probe request events to wifi_hal */
			/* if ((now - sta_info->probe_time) > MAX_PROBEREQ_EVENT_INTERVAL) { */
			if (send_event) {
				ecbd_send_probereq_event(group_idx, ap_idx, sta_info,
					sta_info->rssi, bcast);

				/* update band cap info for the sta which is 1) configured; 2) not broadcast Probe */
				if ((sta_info->band & WLC_BAND_ALL) != WLC_BAND_ALL) {
					ecbd_update_sta_band_info(sta_info);
				}
			}
			sta_info->probe_time = now;

			/* use status change to reduce probe request xing event flooding for same STA */
			if (sta_info->cli_cfg->rssiProbeLWM || sta_info->cli_cfg->rssiProbeHWM) {
				/* if both WM are 0, skip the comparison */
				if ((uint)sta_info->rssi > sta_info->cli_cfg->rssiProbeHWM)
					rssi_change = WIFI_STEERING_RSSI_HIGHER;
				else if ((uint)sta_info->rssi < sta_info->cli_cfg->rssiProbeLWM)
					rssi_change = WIFI_STEERING_RSSI_LOWER;

				if (sta_info->rssi_change != rssi_change) {
					sta_info->rssi_change = rssi_change;
					ecbd_send_rssi_xing_event(group_idx, ap_idx, sta_info,
						sta_info->rssi, STA_RSSI_PROBE);
				}
			}

			/* probe response control */
			if (rssi_change != WIFI_STEERING_RSSI_UNCHANGED) {
				/* add to deny list */
				if (ecbd_add_maclist(sta_info->ifname, &sta_info->addr) == ECBD_SUCCESS) {
					snprintf(log_buf, sizeof(log_buf), "Add MAC=%s to maclist SNR=%d "
						"(noise=%d) pHWM=%d pLWM=%d when probe request on %s\n",
						ether_etoa(dpkt->event.addr.octet, buf), (uint)sta_info->rssi, noise,
						sta_info->cli_cfg->rssiProbeHWM, sta_info->cli_cfg->rssiProbeLWM,
						ap_ifname);
					WLAN_APPS_LOGS("ECBD", log_buf);
				}
			}
			else {
				/* del from deny list */
				if (ecbd_del_maclist(sta_info->ifname, &sta_info->addr) == ECBD_SUCCESS) {
					snprintf(log_buf, sizeof(log_buf), "Remove MAC=%s from maclist SNR=%d "
						"(noise=%d) pHWM=%d pLWM=%d when probe request on %s\n",
						ether_etoa(dpkt->event.addr.octet, buf), (uint)sta_info->rssi, noise,
						sta_info->cli_cfg->rssiProbeHWM, sta_info->cli_cfg->rssiProbeLWM,
						ap_ifname);
					WLAN_APPS_LOGS("ECBD", log_buf);
				}
			}
		}
	} /* sub_idx */
}

static void
ecbd_process_eapd_evt(int sock_fd)
{
	int bytes;
	uchar buf_ptr[MAX_EVENT_BUFFER_LEN], *ptr = buf_ptr;
	bcm_event_t *dpkt;
	char buf[32];
	char ssid[MAX_SSID_LEN+1];
	uint8 ssid_len;
	uint32 event_id = 0, event_sent = 0;
	uint32 reason;
	uint32 group_idx;
	int ap_idx, rssi;
	ecbd_stalist_t *sta_info = NULL;
	wl_event_rx_frame_data_t *rxframe_data;
	int data_len, unit = 0, subunit = 0;

	if ((bytes = recv(sock_fd, ptr, MAX_EVENT_BUFFER_LEN, 0)) > IFNAMSIZ) {

		ptr = ptr + IFNAMSIZ;
		dpkt = (bcm_event_t *)ptr;

		/* ECBD_PRINT_DUMP("REVD:", ptr, bytes); */
		event_id = ntohl(dpkt->event.event_type);
		reason = ntohl(dpkt->event.reason);
		dpkt->event.ifname[BCM_MSG_IFNAME_MAX - 1] = '\0';
		ECBD_PRINT_DEBUG("Received event %d, reason %d,MAC=%s on interface %s\n",
			event_id, reason, ether_etoa(dpkt->event.addr.octet, buf), dpkt->event.ifname);

		/* unify index */
		if (get_ifname_unit(dpkt->event.ifname, &unit, &subunit)) {
			ECBD_PRINT_ERROR("Fail to get ifname index %s\n", dpkt->event.ifname);
			return;
		}

		dpkt->event.ifidx = unit;
		dpkt->event.bsscfgidx = (subunit > 0) ? subunit : 0;

		ap_idx = ecbd_get_apIndex(dpkt->event.ifname);

		if ((event_id != WLC_E_PROBREQ_MSG) && (event_id != WLC_E_PROBREQ_MSG_RX)) {
			ECBD_PRINT_INFO("event_id=%d reason=%d MAC=%s intf=%s ifidx=%d bssidx=%d apidx=%d\n",
				event_id, reason, ether_etoa(dpkt->event.addr.octet, buf), dpkt->event.ifname,
				dpkt->event.ifidx, dpkt->event.bsscfgidx, ap_idx);
		}

		switch (event_id) {
			case WLC_E_AUTH_IND:
				ECBD_PRINT_INFO("Event %d (WLC_E_AUTH_IND): MAC=%s ifidx=%d, bssidx=%d, bytes=%d\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf),
					dpkt->event.ifidx, dpkt->event.bsscfgidx, bytes);
				break;

			case WLC_E_AUTH_REQ: /* new event to check auth watermark */
				ECBD_PRINT_INFO("Event %d (WLC_E_AUTH_REQ): MAC=%s ifidx=%d, bssidx=%d, bytes=%d\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf),
					dpkt->event.ifidx, dpkt->event.bsscfgidx, bytes);
				ECBD_PRINT_DUMP("REVD Auth:", ptr, bytes);

				snprintf(log_buf, sizeof(log_buf), "Event %d (WLC_E_AUTH_REQ): MAC=%s Reason=%d on %s\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf), reason, dpkt->event.ifname);
				WLAN_APPS_LOGS("ECBD", log_buf);

				/* send auth rssi xing event if necessary */
				if (ecbd_find_ap_cfg(dpkt->event.ifidx, dpkt->event.bsscfgidx, &group_idx) != NULL) {
					sta_info = ecbd_retrieve_sta_info(&(dpkt->event.addr), dpkt->event.ifidx,
						dpkt->event.bsscfgidx);
					if (sta_info && (sta_info->type & (STA_TYPE_ASSOC | STA_TYPE_CLIENT_SET))) {

						if (sta_info->cli_cfg != NULL) {
							BOOL be_rejected;
							/* default: in range */
							int32 rssi_change_auth = WIFI_STEERING_RSSI_UNCHANGED;

							/* use rssi from the event data payload */
							data_len = IFNAMSIZ + BCM_EVENT_HEADER_LEN + sizeof(wl_event_rx_frame_data_t);

							ECBD_PRINT_INFO("Old snr=%d for MAC=%s bytes=%d data_len=%d\n",
								sta_info->rssi, ether_etoa(dpkt->event.addr.octet, buf), bytes, data_len);

							if (bytes >= data_len) {
								int noise = 0;
								rxframe_data = (wl_event_rx_frame_data_t *)(ptr + BCM_EVENT_HEADER_LEN);
								rssi = htonl(rxframe_data->rssi);

								if (ecbd_get_noise(dpkt->event.ifname, &noise) == ECBD_FAIL) {
									ECBD_PRINT_ERROR("Err: intf:%s noise\n", dpkt->event.ifname);
									noise = -90; /* use a default value */
								}

								/* convert to snr */
								sta_info->rssi = (rssi > noise)?(rssi - noise):0;
								ECBD_PRINT_INFO("WLC_E_AUTH_REQ: rssi=%d (%d) noise=%d snr=%d\n",
									rxframe_data->rssi, rssi, noise, sta_info->rssi);
							}

							/* check the water mark first */
							ECBD_PRINT_INFO("Check RSSI WM for MAC=%s when WLC_E_AUTH_REQ: "
								"snr=%d aHWM=%d aLWM=%d\n",
								ether_etoa(dpkt->event.addr.octet, buf), (uint)sta_info->rssi,
								sta_info->cli_cfg->rssiAuthHWM, sta_info->cli_cfg->rssiAuthLWM);

							/* use status change to reduce auth request crossing
							 * event flooding for same STA
							 * if sta_info->rssi == 0 (not updated yet), allow it in
							 */
							if (sta_info->rssi &&
								(sta_info->cli_cfg->rssiAuthLWM || sta_info->cli_cfg->rssiAuthHWM)) {
								/* if both WM are 0, skip the comparison (same as in range) */
								if ((uint)sta_info->rssi > sta_info->cli_cfg->rssiAuthHWM)
									rssi_change_auth = WIFI_STEERING_RSSI_HIGHER;
								else if ((uint)sta_info->rssi < sta_info->cli_cfg->rssiAuthLWM)
									rssi_change_auth = WIFI_STEERING_RSSI_LOWER;

								if (sta_info->rssi_change_auth != rssi_change_auth) {
									sta_info->rssi_change_auth = rssi_change_auth;
									ecbd_send_rssi_xing_event(group_idx, ap_idx, sta_info,
										sta_info->rssi, STA_RSSI_AUTH);
								}
							}

							/* auth response control */
							if (rssi_change_auth != WIFI_STEERING_RSSI_UNCHANGED) {
								/* not in watermark range, add to deny list */
								if (ecbd_add_maclist(sta_info->ifname, &sta_info->addr) == ECBD_SUCCESS) {
									snprintf(log_buf, sizeof(log_buf), "Add MAC=%s to maclist SNR=%d "
										"(aHWM=%d aLWM=%d) when Auth on %s\n",
										ether_etoa(dpkt->event.addr.octet, buf), (uint)sta_info->rssi,
										sta_info->cli_cfg->rssiAuthHWM, sta_info->cli_cfg->rssiAuthLWM,
										sta_info->ifname);
									WLAN_APPS_LOGS("ECBD", log_buf);
								}

								if (sta_info->cli_cfg->authRejectReason == 0) {
									be_rejected = FALSE;
									ECBD_PRINT_INFO("Auth silently ignore for MAC=%s "
										"when WLC_E_AUTH_REQ\n",
										ether_etoa(dpkt->event.addr.octet, buf));
								} else {
									be_rejected = TRUE;
									/* send deauth with the reason */
									ECBD_PRINT_INFO("Deauth reason %d for MAC=%s "
										"when WLC_E_AUTH_REQ\n",
										sta_info->cli_cfg->authRejectReason,
										ether_etoa(dpkt->event.addr.octet, buf));
									ecbd_disconnect_sta(dpkt->event.ifname,
										&(sta_info->addr), DISCONNECT_TYPE_DEAUTH,
										sta_info->cli_cfg->authRejectReason);

								}

								ecbd_send_steering_authfail_event(group_idx, ap_idx, sta_info,
									sta_info->rssi, sta_info->cli_cfg->authRejectReason,
									be_rejected);
							}
							else {
								/* remove from deny list if RSSI is in watermark range */
								ECBD_PRINT_INFO("Remove MAC=%s from blacklist (SNR in range) "
									"when WLC_E_AUTH_REQ\n",
									ether_etoa(dpkt->event.addr.octet, buf));
								if (ecbd_del_maclist(sta_info->ifname, &sta_info->addr) == ECBD_SUCCESS) {
									snprintf(log_buf, sizeof(log_buf), "Remove MAC=%s from maclist SNR=%d "
										"(aHWM=%d aLWM=%d) when Auth on %s\n",
										ether_etoa(dpkt->event.addr.octet, buf), (uint)sta_info->rssi,
										sta_info->cli_cfg->rssiAuthHWM, sta_info->cli_cfg->rssiAuthLWM,
										sta_info->ifname);
									WLAN_APPS_LOGS("ECBD", log_buf);
								}
							}
						} /* sta_info->cli_cfg != NULL */
					} /* sta_info */
				} /* ap_cfg */

				break;

			case WLC_E_ASSOC_IND: /* 8 */
				/* fall through */
			case WLC_E_REASSOC_IND: /* 10 */
				ECBD_PRINT_INFO("Event %d (Re)Assoc: MAC=%s ifidx=%d, bssidx=%d, bytes=%d\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf),
					dpkt->event.ifidx, dpkt->event.bsscfgidx, bytes);
				ECBD_PRINT_DUMP("REVD Assoc:", ptr, bytes);

				snprintf(log_buf, sizeof(log_buf), "MAC=%s %s on %s\n",
					ether_etoa(dpkt->event.addr.octet, buf),
					(event_id == WLC_E_ASSOC_IND) ? "Associated":"Re-Associated", dpkt->event.ifname);
				WLAN_APPS_LOGS("ECBD", log_buf);

				ptr += BCM_EVENT_HEADER_LEN;
				ssid_len = *(ptr+1);
				if ((*ptr == 0) && (ssid_len >= 0) && (ssid_len < (MAX_SSID_LEN+1)))
				{
					wifi_hal_cb_evt_t cb_evt;
					/* ecbd_stalist_t *sta_info = NULL; */

					memset(&cb_evt, 0, sizeof(wifi_hal_cb_evt_t));
					cb_evt.version = WIFI_HAL_EVT_VERSION;
					cb_evt.apIndex = ecbd_get_apIndex(dpkt->event.ifname);
					cb_evt.type = WIFI_HAL_CB_STA_CONN;
					strncpy(cb_evt.mac, ether_etoa(dpkt->event.addr.octet, buf), MAX_MAC_ADDR_LEN - 1);

					/* SSID IE */
					if (ssid_len)
						strncpy(ssid, (char*)(ptr+2), sizeof(ssid) - 1);
					ssid[sizeof(ssid) - 1] = '\0';
					ECBD_PRINT_INFO("event=%s,ssid=%s,if=%s,idx=%d,bidx=%d,STA=%s\n",
						(event_id == WLC_E_ASSOC_IND) ? "ASSOC_IND" : "REASSOC_IND",
						ssid, dpkt->event.ifname,
						dpkt->event.ifidx, dpkt->event.bsscfgidx,
						ether_etoa(dpkt->event.addr.octet, buf));

					event_sent = 0;
					sta_info = ecbd_retrieve_sta_info(&(dpkt->event.addr), dpkt->event.ifidx,
						dpkt->event.bsscfgidx);
					if (sta_info) {
						event_sent = sta_info->event_sent;
						/* the sta may be a pre-configured client */
						ECBD_PRINT_INFO("Existing STA: MAC=%s assoc_time=%lu disassoc_time=%lu "
							"state=%d type=0x%x\n",
							ether_etoa(sta_info->addr.octet, buf), sta_info->assoc_time,
							sta_info->disassoc_time, sta_info->state, sta_info->type);

						if (event_id == WLC_E_ASSOC_IND) {
							if ((sta_info->disassoc_time > 0) && (sta_info->state == INACTIVE)) {
								sta_info->disassoc_time = 0;
								cb_evt.reason = CONN_RECONN_AFTER_INACTIVITY;
							}
							else {
								cb_evt.reason = CONN_NEW;
							}
						} else if (event_id == WLC_E_REASSOC_IND) {
							cb_evt.reason = CONN_RENEW;
						}
					} else {
						cb_evt.reason = CONN_NEW;
					}

					/* always call ecbd_add_stalist to update info (type, assoc_time etc.) */
					sta_info = ecbd_add_stalist(ssid, &(dpkt->event.addr), dpkt->event.ifidx,
						dpkt->event.bsscfgidx, dpkt->event.ifname, STA_TYPE_ASSOC);
					if (sta_info == NULL) {
						ECBD_PRINT_ERROR("Fail to get the sta MAC=%s after adding it to list\n",
							ether_etoa(dpkt->event.addr.octet, buf));
						break;
					}
					else {
						if (ecbd_update_sta_RMcap(sta_info, dpkt) == ECBD_SUCCESS) {
							ECBD_PRINT_INFO("%s Populated rm_cap = %02x %02x %02x %02x %02x \n",
								__FUNCTION__, sta_info->rm_cap.cap[0],
								sta_info->rm_cap.cap[1], sta_info->rm_cap.cap[2],
								sta_info->rm_cap.cap[3], sta_info->rm_cap.cap[4]);
						}
					}
					ECBD_PRINT_INFO("STA associated: MAC=%s assoc_time=%lu disassoc_time=%lu "
						"state=%d type=0x%x interface=%s\n",
						ether_etoa(sta_info->addr.octet, buf), sta_info->assoc_time,
						sta_info->disassoc_time,
						sta_info->state, sta_info->type, sta_info->ifname);

					/* send steering connection event */
					if (ecbd_find_ap_cfg(dpkt->event.ifidx, dpkt->event.bsscfgidx, &group_idx) == NULL) {
						ECBD_PRINT_WARNING("WLC_E_(RE)ASSOC_IND: "
							"apconfig not found, use group_idx 0\n");
						group_idx = 0; /* default */
					}

					ecbd_notify_wifi_hal_subscribers(&cb_evt, sizeof(wifi_hal_cb_evt_t),
						WIFI_HAL_CB_STA_CONN);

					/* for security mode, wait for event WLC_E_AUTHORIZED */
					if (ecbd_is_open_security(dpkt->event.ifname)) {
						WL_STATION_LIST_ENTRY wl_sta, *p_wlsta = NULL;

						ECBD_PRINT_INFO("Send cb event for %s, event_sent=%d\n",
							ether_etoa(sta_info->addr.octet, buf), event_sent);

						/* assoc event trigger the high watermark update */
						ecbd_update_assoc_dev_hwm_cnt(dpkt->event.ifname, -1, 0);

						if (event_sent & ECBD_SENT_E_ASSOC) {
							/* may be sent by watchdog */
							ECBD_PRINT_DEBUG("connect event already sent to steering callback\n");
						}
						else {
							ecbd_send_steering_conn_event(group_idx, ap_idx, sta_info,
								WIFI_STEERING_EVENT_CLIENT_CONNECT, 0, 0, 0);
							sta_info->event_sent |= ECBD_SENT_E_ASSOC;

							/* write to log */
							snprintf(log_buf, sizeof(log_buf),
								"Send connect event MAC=%s on %s apIdx %d (Open Mode)\n",
								ether_etoa(sta_info->addr.octet, buf), dpkt->event.ifname, ap_idx);
							WLAN_APPS_LOGS("ECBD", log_buf);

							if (ecbd_get_wl_stainfo(&wl_sta, sta_info) == ECBD_SUCCESS)
								p_wlsta = &wl_sta;

							ECBD_PRINT_INFO("MAC=%s Open Security on ifname=%s (p_wlsta=%p)\n",
								ether_etoa(dpkt->event.addr.octet, buf),
								dpkt->event.ifname, p_wlsta);

							ecbd_sent_msg_to_ltc(dpkt, p_wlsta);
						}
					}
				}
				break;

			case WLC_E_AUTHORIZED: /* 136 */
				ECBD_PRINT_INFO("Event %d AUTHORIZED: MAC=%s ifidx=%d, bssidx=%d, bytes=%d\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf),
					dpkt->event.ifidx, dpkt->event.bsscfgidx, bytes);

				sta_info = ecbd_retrieve_sta_info(&(dpkt->event.addr), dpkt->event.ifidx, dpkt->event.bsscfgidx);
				if (sta_info && (sta_info->type & STA_TYPE_ASSOC)) {
					WL_STATION_LIST_ENTRY wl_sta, *p_wlsta = NULL;

					/* autho event trigger the high watermark update */
					ecbd_update_assoc_dev_hwm_cnt(dpkt->event.ifname, -1, 0);

					if (sta_info->event_sent & ECBD_SENT_E_ASSOC) {
						/* may be sent by watchdog */
						ECBD_PRINT_DEBUG("connect event already sent to steering callback\n");
						break;
					}

					if (ecbd_get_wl_stainfo(&wl_sta, sta_info) == ECBD_SUCCESS) {
						wl_sta.authenticationState = wl_sta.authorized = 1;
						p_wlsta = &wl_sta;
					}

					ECBD_PRINT_INFO("MAC=%s authorized: assoc_time=%lu now=%lu state=%d type=0x%x interface=%s\n",
						ether_etoa(sta_info->addr.octet, buf), sta_info->assoc_time, time(NULL),
						sta_info->state, sta_info->type, sta_info->ifname);

					ecbd_sent_msg_to_ltc(dpkt, p_wlsta);

					if (ecbd_find_ap_cfg(dpkt->event.ifidx, dpkt->event.bsscfgidx, &group_idx) == NULL) {
						ECBD_PRINT_WARNING("WLC_E_AUTHORIZED: apconfig not found, use group_idx 0\n");
						group_idx = 0; /* default */
					}

					ECBD_PRINT_INFO("WLC_E_AUTHORIZED: group_idx=%d ap_idx=%d\n",
						group_idx, ap_idx);
					ecbd_send_steering_conn_event(group_idx, ap_idx, sta_info,
						WIFI_STEERING_EVENT_CLIENT_CONNECT, 0, 0, 0);
					sta_info->event_sent |= ECBD_SENT_E_ASSOC;

					/* write to log */
					snprintf(log_buf, sizeof(log_buf),
						"Send connect event MAC=%s on %s apIdx %d (AUTHORIZED)\n",
						ether_etoa(sta_info->addr.octet, buf), dpkt->event.ifname, ap_idx);
					WLAN_APPS_LOGS("ECBD", log_buf);
					/* TODO send auth_cb "succ" */
				}
				else {
					ECBD_PRINT_ERROR("MAC=%s fail when authorized: %s\n",
						ether_etoa(dpkt->event.addr.octet, buf), sta_info?"not assoc":"not found");
				}
				break;

			case WLC_E_ASSOC_FAIL:
				/* fall through */
			case WLC_E_REASSOC_FAIL:
				{
					wifi_hal_cb_evt_t cb_evt;

					ECBD_PRINT_INFO("Event %d ASSOC_FAIL MAC=%s ifidx=%d bssidx=%d reason=%d\n",
						event_id, ether_etoa(dpkt->event.addr.octet, buf),
						dpkt->event.ifidx, dpkt->event.bsscfgidx, reason);

					/* DOT11_SC_ASSOC_BUSY_FAIL (17):
						Association denied because AP is
						unable to handle additional
						associated stations
					*/

					if (reason != DOT11_SC_ASSOC_BUSY_FAIL) {
						ECBD_PRINT_WARNING("Reason %d is not DOT11_SC_ASSOC_BUSY_FAIL\n", reason);
						break;
					}

					memset(&cb_evt, 0, sizeof(wifi_hal_cb_evt_t));

					cb_evt.version = WIFI_HAL_EVT_VERSION;
					cb_evt.apIndex = ecbd_get_apIndex(dpkt->event.ifname);
					cb_evt.type = WIFI_HAL_CB_AUTH_FAIL;
					cb_evt.reason = reason;
					strncpy(cb_evt.mac, ether_etoa(dpkt->event.addr.octet, buf), MAX_MAC_ADDR_LEN - 1);

					/* max associated client count reached rejection */
					ECBD_PRINT_INFO("Notify maxassoc reached for STA %s on %s reason=%d\n",
						ether_etoa(dpkt->event.addr.octet, buf), dpkt->event.ifname, cb_evt.reason);

					ecbd_notify_wifi_hal_subscribers(&cb_evt, sizeof(wifi_hal_cb_evt_t),
						WIFI_HAL_CB_AUTH_FAIL);

				}
				break;

			case WLC_E_DEAUTH:
			case WLC_E_DEAUTH_IND:
				{
					/* ecbd_stalist_t *sta_info = NULL; */
					wifi_hal_cb_evt_t cb_evt;
					wl_event_data_rssi_t *rssi = NULL;

					ECBD_PRINT_INFO("Event %d (WLC_E_DEAUTHx): MAC=%s ifidx=%d, bssidx=%d\n",
						event_id, ether_etoa(dpkt->event.addr.octet, buf),
						dpkt->event.ifidx, dpkt->event.bsscfgidx);

					snprintf(log_buf, sizeof(log_buf), "Event %d (WLC_E_DEAUTH): MAC=%s Reason=%d on %s\n",
						event_id, ether_etoa(dpkt->event.addr.octet, buf), reason, dpkt->event.ifname);
					WLAN_APPS_LOGS("ECBD", log_buf);

					/* print rssi of kicked out client */
					if (DOT11_RC_INACTIVITY == reason && dpkt->event.datalen >= sizeof(wl_event_data_rssi_t)) {
						ptr += BCM_EVENT_HEADER_LEN;
						rssi = (wl_event_data_rssi_t *)ptr;
						snprintf(log_buf, sizeof(log_buf), "Kicked out inactive STA, MAC=%s rssi=%d\n", ether_etoa(dpkt->event.addr.octet, buf), rssi->rssi);
						WLAN_APPS_LOGS("ECBD", log_buf);
					}

					if ((sta_info = ecbd_retrieve_sta_info(&(dpkt->event.addr), dpkt->event.ifidx,
						dpkt->event.bsscfgidx)) != NULL) {
						if ((sta_info->assoc_time > 0) && (sta_info->state == ACTIVE)) {
							sta_info->disassoc_time = time(NULL);
							sta_info->assoc_time = 0;
							sta_info->state = INACTIVE;
						}
					}
					ecbd_show_stalist();

					memset(&cb_evt, 0, sizeof(wifi_hal_cb_evt_t));

					cb_evt.version = WIFI_HAL_EVT_VERSION;
					cb_evt.type = WIFI_HAL_CB_AUTH_FAIL;
					cb_evt.apIndex = ecbd_get_apIndex(dpkt->event.ifname);

					/* change to use reason from driver directly */
					cb_evt.reason = reason;

					/* STA deauth reason mapping */
					switch (reason) {
						case DOT11_RC_TIMEOUT:
							ECBD_PRINT_INFO("Auth timeout\n");
							break;
						case DOT11_RC_DISASSOC_BTM:
							/* skip FBT related deauth */
							ECBD_PRINT_INFO("Event skipped\n");
							return;
						case DOT11_RC_INACTIVITY:
							ECBD_PRINT_INFO("STA inactive\n");
							break;
						default:
							ECBD_PRINT_INFO("Event with other reason %d\n", reason);
							break;
					}

					strncpy(cb_evt.mac, ether_etoa(dpkt->event.addr.octet, buf), MAX_MAC_ADDR_LEN - 1);
#ifndef RDKB_WLDM
					ecbd_notify_wifi_hal(&cb_evt, sizeof(wifi_hal_cb_evt_t),
						WIFI_HAL_CB_AUTH_FAIL_UDP_PORT);
#else
					ecbd_notify_wifi_hal_subscribers(&cb_evt, sizeof(wifi_hal_cb_evt_t),
						WIFI_HAL_CB_AUTH_FAIL);
#endif

					/* also send to steering event to mesh callback */
					if (sta_info) {
						/* send steering disconnection event */
						if (ecbd_find_ap_cfg(dpkt->event.ifidx, dpkt->event.bsscfgidx,
							&group_idx) == NULL) {
							ECBD_PRINT_DEBUG("WLC_E_(RE)ASSOC_IND: "
								"apconfig not found, use group_idx 0\n");
							group_idx = 0; /* default */
						}
						ecbd_send_steering_conn_event(group_idx, ap_idx, sta_info,
							WIFI_STEERING_EVENT_CLIENT_DISCONNECT, reason,
							DISCONNECT_SOURCE_LOCAL, DISCONNECT_TYPE_DEAUTH);

						/* send disassoc event to ltc (some driver doesn't send WLC_E_DISASSOC_IND) */
						if (!(sta_info->event_sent & ECBD_SENT_E_DISASSOC)) {
							ECBD_PRINT_INFO("send WLC_E_DISASSOC_IND when DEAUTH\n");
							ecbd_sent_msg_to_ltc(dpkt, NULL);
							sta_info->event_sent |= ECBD_SENT_E_DISASSOC;
						}
					}
				}
				break;

			case WLC_E_DISASSOC:
			case WLC_E_DISASSOC_IND:
				ECBD_PRINT_INFO("Event %d %s (WLC_E_DISASSOC_IND): MAC=%s ifidx=%d, bssidx=%d\n",
					event_id, (event_id == WLC_E_DEAUTH_IND) ? "DEAUTH" : "DISASSOC",
					ether_etoa(dpkt->event.addr.octet, buf),
					dpkt->event.ifidx, dpkt->event.bsscfgidx);

				snprintf(log_buf, sizeof(log_buf), "Event %d (WLC_E_DISASSOC_IND): MAC=%s Reason=%d on %s\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf), reason, dpkt->event.ifname);
				WLAN_APPS_LOGS("ECBD", log_buf);

				{
					/* ecbd_stalist_t *sta_info = NULL; */

					sta_info = ecbd_retrieve_sta_info(&(dpkt->event.addr), dpkt->event.ifidx,
						dpkt->event.bsscfgidx);
					if (sta_info) {
						if ((sta_info->type & STA_TYPE_ASSOC) && (sta_info->assoc_time > 0) &&
							(sta_info->state == ACTIVE)) {
							sta_info->disassoc_time = time(NULL);
							sta_info->assoc_time = 0;
							sta_info->state = INACTIVE;
						}

						/* use a new flag to avoid duplicated event */
						if (!(sta_info->event_sent & ECBD_SENT_E_DISASSOC)) {
							/* send steering disconnection event */
							if (ecbd_find_ap_cfg(dpkt->event.ifidx, dpkt->event.bsscfgidx,
								&group_idx) == NULL) {
								ECBD_PRINT_DEBUG("WLC_E_(RE)ASSOC_IND: apconfig not found, "
									"use group_idx 0\n");
								group_idx = 0; /* default */
							}
							ecbd_send_steering_conn_event(group_idx, ap_idx, sta_info,
								WIFI_STEERING_EVENT_CLIENT_DISCONNECT, reason,
								DISCONNECT_SOURCE_REMOTE, DISCONNECT_TYPE_DISASSOC);

							ecbd_sent_msg_to_ltc(dpkt, NULL);
							sta_info->event_sent |= ECBD_SENT_E_DISASSOC;
						} else {
							ECBD_PRINT_DEBUG("Disconnect event has been sent when WLC_E_DISASSOC_IND\n");
						}
					} else {
						ECBD_PRINT_ERROR("Event %s (WLC_E_DISASSOC_IND): "
							"MAC=%s sta_info is not found\n",
							(event_id == WLC_E_DEAUTH_IND) ? "DEAUTH" : "DISASSOC",
							ether_etoa(dpkt->event.addr.octet, buf));
					}

					ecbd_show_stalist();
				}
				break;
			case WLC_E_PROBREQ_MSG:
				ECBD_PRINT_PROBE("Event %d (WLC_E_PROBREQ_MSG): MAC=%s ifidx=%d, bssidx=%d\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf),
					dpkt->event.ifidx, dpkt->event.bsscfgidx);
				/* TODO */
				break;
			case WLC_E_PROBREQ_MSG_RX:
				ECBD_PRINT_PROBE("Event %d (WLC_E_PROBREQ_MSG_RX) MAC=%s ifidx=%d, bssidx=%d (%s)\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf),
					dpkt->event.ifidx, dpkt->event.bsscfgidx, dpkt->event.ifname);

				ecbd_handle_probe_event(bytes, dpkt);
				break;
			case WLC_E_PRUNE:
				/* fall through */
			case WLC_E_MIC_ERROR:
				{
					wifi_hal_cb_evt_t cb_evt;

					ECBD_PRINT_WARNING("Event %d (%s): MAC=%s ifidx=%d, bssidx=%d\n",
						event_id, (event_id == WLC_E_PRUNE)?"WLC_E_PRUNE":"WLC_E_MIC_ERROR",
						ether_etoa(dpkt->event.addr.octet, buf),
						dpkt->event.ifidx, dpkt->event.bsscfgidx);

					if (event_id == WLC_E_PRUNE) {
						char *akms, nv_name[64];

						if (reason != WLC_E_PRUNE_ENCR_MISMATCH) {
							ECBD_PRINT_WARNING("Reason %d is not WLC_E_PRUNE_ENCR_MISMATCH\n", reason);
							break;
						}

						snprintf(nv_name, sizeof(nv_name), "%s_akm", dpkt->event.ifname);
						akms = nvram_safe_get(nv_name);
						ECBD_PRINT_INFO("nvram %s is <%s>\n", nv_name, akms);

						if (!strstr(akms, "sae")) {
							if ((sta_info = ecbd_retrieve_sta_info(&(dpkt->event.addr), dpkt->event.ifidx,
								dpkt->event.bsscfgidx)) == NULL) {
								ECBD_PRINT_INFO("STA %s is not in list\n", ether_etoa(dpkt->event.addr.octet, buf));
								break;
							}

							if (!(sta_info->type & STA_TYPE_ASSOC)) {
								ECBD_PRINT_INFO("STA %s is not in assoc state\n", ether_etoa(dpkt->event.addr.octet, buf));
								break;
							}
						}
					}

					memset(&cb_evt, 0, sizeof(wifi_hal_cb_evt_t));

					cb_evt.version = WIFI_HAL_EVT_VERSION;
					cb_evt.apIndex = ecbd_get_apIndex(dpkt->event.ifname);
					cb_evt.type = WIFI_HAL_CB_AUTH_FAIL;
					cb_evt.reason = DOT11_RC_AUTH_INVAL; /* 2 for wrong password */

					/* wrong passord */
					ECBD_PRINT_INFO("Notify wrong password for STA %s on %s reason=%d\n",
						ether_etoa(dpkt->event.addr.octet, buf), dpkt->event.ifname, cb_evt.reason);
					strncpy(cb_evt.mac, ether_etoa(dpkt->event.addr.octet, buf), MAX_MAC_ADDR_LEN - 1);

#ifndef RDKB_WLDM
					ecbd_notify_wifi_hal(&cb_evt, sizeof(wifi_hal_cb_evt_t),
						WIFI_HAL_CB_AUTH_FAIL_UDP_PORT);
#else
					ecbd_notify_wifi_hal_subscribers(&cb_evt, sizeof(wifi_hal_cb_evt_t),
						WIFI_HAL_CB_AUTH_FAIL);
#endif

				}
				break;

			case WLC_E_ACTION_FRAME:
				/* DPP is kind of action frame */
				{
					int found = 0;
					int type = 0;
					char *af_ptr;
					int af_len;
					wifi_dpp_pub_act_frame_t *pub_ptr;
					wifi_dpp_gas_act_frame_t *gas_ptr;

					ECBD_PRINT_DUMP("REVD Action:", ptr, bytes);

					/* BCM_EVENT_HEADER_LEN is 72 bytes */
					if (bytes <= BCM_EVENT_HEADER_LEN) {
						ECBD_PRINT_ERROR("REVD Action: length %d too short", bytes);
						break;
					}
					af_ptr = (char *)(ptr + BCM_EVENT_HEADER_LEN);
					af_len = bytes - (IFNAMSIZ + BCM_EVENT_HEADER_LEN);

					ECBD_PRINT_INFO("ACTION_FRAME: ptr=%p (%p) bytes=%d (%d) pub_len=%d gas_len=%d\n",
						ptr, af_ptr, bytes, af_len, sizeof(wifi_dpp_pub_act_frame_t), sizeof(wifi_dpp_gas_act_frame_t));

					pub_ptr = (wifi_dpp_pub_act_frame_t *)af_ptr;

					if (pub_ptr->category == PUB_AF_CATEGORY) {

						/* DPP auth/config share the same category "04" */
						ECBD_PRINT_INFO("ACTION_FRAME: MAC=%s ifidx=%d bssidx=%d cat=%d action=%d oui_type=0x%x dpp_frame_type=%d\n",
							ether_etoa(dpkt->event.addr.octet, buf),
							dpkt->event.ifidx, dpkt->event.bsscfgidx,
							pub_ptr->category, pub_ptr->action, pub_ptr->oui_type, pub_ptr->dpp_frame_type);

						if ((pub_ptr->action == DPP_PUB_AF_ACTION) &&
							(memcmp(pub_ptr->oui, WFA_OUI, WFA_OUI_LEN) == 0) &&
							(pub_ptr->oui_type == DPP_PUB_AF_ACTION_OUI_TYPE)) {
							/* currecntly only care about AUTH response */
							if (pub_ptr->dpp_frame_type == DPP_PAF_AUTH_RSP) {
								found = 1;
								type = WIFI_HAL_CB_DPP_AUTH_RESP;
							}
						} else {
							gas_ptr = (wifi_dpp_gas_act_frame_t *)af_ptr;
							if (gas_ptr->action == DPP_GAS_AF_ACTION ||
								gas_ptr->action == DPP_GAS_AF_ACTION_FRAG) {
								if ((memcmp(gas_ptr->asp_ie, DPP_GAS_AF_ASP_IE, 3) == 0) &&
									(memcmp(gas_ptr->asp_id, DPP_GAS_AF_ASP_ID, 7) == 0)) {
									found = 1;
									type = WIFI_HAL_CB_DPP_CONFIG_REQ;
								} else {
									char *ptr = (char *)&(gas_ptr->asp_ie[0]);
									ECBD_PRINT_INFO("Parse ANQP: af_ptr=%p ptr=%p ptr[0]=0x%x ptr[3]=0x%x\n",
										af_ptr, ptr, ptr[0], ptr[3]);
									/* sample format: 04 0a 02 6c 02 00 00 06 00 00 01 02 00 01 01 */
									if ((ptr[0] == DOT11_MNG_ADVERTISEMENT_ID) &&
										(ptr[3] == ADVP_ANQP_PROTOCOL_ID)) {
										found = 1;
										type = WIFI_HAL_CB_ANQP_QAS_REQ;
									}
								}
							}
						}

						/* followings for register based callback */
						if (found) {
							wifi_hal_cb_evt_t *vptr;
							int vlen = 0;

							/* to wifi_hal event length */
							vlen = sizeof(wifi_hal_cb_evt_t) + af_len;

							vptr = (wifi_hal_cb_evt_t *) malloc(vlen);
							if (vptr == NULL) {
								ECBD_PRINT_ERROR("Err: alloc buff %d for WLC_E_ACTION_FRAME DPP\n", vlen);
								break;
							}

							memset(vptr, 0, vlen);

							vptr->version = WIFI_HAL_EVT_VERSION;
							vptr->type = WIFI_MGMT_FRAME_TYPE_ACTION;

							/* use reason as subtype */
							vptr->reason = type;

							vptr->apIndex = ap_idx;
							strncpy(vptr->mac, ether_etoa(dpkt->event.addr.octet, buf), MAX_MAC_ADDR_LEN - 1);

							/* point to payload (action frame) */
							memcpy(vptr->data, af_ptr, af_len);

							ECBD_PRINT_INFO("Notify Event %d (DPP) to wifi_hal: MAC=%s af_len=%d vlen=%d (type=%d)\n",
								event_id, ether_etoa(dpkt->event.addr.octet, buf),
								af_len, vlen, type);

							ecbd_notify_wifi_hal_subscribers(vptr, vlen, WIFI_HAL_CB_DPP);

							if (vptr) {
								free(vptr);
								vptr = NULL;
							}

							break;
						}
					}
				}
				/* fall through for other Action Frame */
			case WLC_E_RRM:
				{
					wifi_hal_cb_evt_t *cb_evtp;
					int cb_len;
					dot11_rm_ie_t *ie;
					wl_rrm_event_t *rrmevtp;
					unsigned char *cbdp;
					char logStr[128];
					rrmevtp = (wl_rrm_event_t *)((unsigned char *)dpkt + sizeof(bcm_event_t));
					ie = (dot11_rm_ie_t *)(rrmevtp->payload);

					/* ECBD_PRINT_DUMP("ecbd_process_eapd_evt-WLC_E_RRM-dpkt", (unsigned char *)dpkt, 128); */

					if (rrmevtp->cat  == DOT11_RM_ACTION_RM_REP) {
						if (rrmevtp->subevent == DOT11_MEASURE_TYPE_BEACON) {
							cb_len = sizeof(wifi_hal_cb_evt_t) + rrmevtp->len;
							cb_evtp = (wifi_hal_cb_evt_t *) malloc(cb_len);
							memset(cb_evtp, 0, cb_len);

							ECBD_PRINT_WARNING("Event %d (WLC_E_RRM): MAC=%s ifidx=%d, "
								"bssidx=%d datalen=%d \n",
								event_id, ether_etoa(dpkt->event.addr.octet, buf),
								dpkt->event.ifidx, dpkt->event.bsscfgidx, dpkt->event.datalen);
							cb_evtp->version = WIFI_HAL_EVT_VERSION;
							cb_evtp->apIndex = dpkt->event.ifidx;
							cb_evtp->type = WIFI_HAL_CB_RRM_BCNREP;
							strncpy(cb_evtp->mac, ether_etoa(dpkt->event.addr.octet, buf),
								MAX_MAC_ADDR_LEN - 1);
							cbdp = (unsigned char *)cb_evtp->data;
							memcpy(cbdp, (unsigned char *)(rrmevtp->payload),  rrmevtp->len);
							ECBD_PRINT_INFO("ecbd_process_eapd_evt cb apIndex=%d "
								"type=0x%x cb_len=%d\n",
								cb_evtp->apIndex, cb_evtp->type, cb_len);

							memset(logStr, 0, 128);
							snprintf(logStr, 128,  "<802.11K Rx> Radio:%d Beacon Report "
								"from MAC=%s Token=0x%x \n",
								cb_evtp->apIndex, buf, ie->token);
							ecbd_log_rrm_event(logStr);

							ecbd_notify_wifi_hal_subscribers(cb_evtp, cb_len,
								WIFI_HAL_CB_RRM_BCNREP);
							if (cb_evtp) {
								free(cb_evtp);
								cb_evtp = NULL;
							}
						} /* DOT11_MEASURE_TYPE_BEACON */
					} /* DOT11_RM_ACTION_RM_REP */
					else if (rrmevtp->cat == DOT11_RM_ACTION_NR_REQ) {
						dot11_rm_action_t *actp = (dot11_rm_action_t *)(rrmevtp->payload);
						memset(logStr, 0, 128);
						snprintf(logStr, 128,  "<802.11K Rx> Radio:%d Neighbor Request "
							"from MAC=%s Token=0x%x\n",
							dpkt->event.ifidx, ether_etoa(dpkt->event.addr.octet, buf), actp->token);
						ecbd_log_rrm_event(logStr);
					}
					else if (rrmevtp->cat == DOT11_RM_ACTION_NR_REP) {
						dot11_rm_action_t *actp = (dot11_rm_action_t *)(rrmevtp->payload);
						memset(logStr, 0, 128);
						snprintf(logStr, 128,  "<802.11K Tx> Radio:%d Neighbor Report "
							"to MAC=%s Token=0x%x\n",
							dpkt->event.ifidx, ether_etoa(dpkt->event.addr.octet, buf), actp->token);
						ecbd_log_rrm_event(logStr);
					}
				}
				break;

			case WLC_E_BSSTRANS_RESP:
			case WLC_E_BSSTRANS_REQ:
			case WLC_E_BSSTRANS_QUERY:
				{
					BOOL activate;

					ECBD_PRINT_INFO("Event %d (WLC_E_BSSTRANSx): MAC=%s ifidx=%d, bssidx=%d, bytes=%d ptr=%p\n",
						event_id, ether_etoa(dpkt->event.addr.octet, buf),
						dpkt->event.ifidx, dpkt->event.bsscfgidx, bytes, ptr);

					/* ignore all BTM events if BTM is not activated on the radio */
#ifdef RDKB_WLDM
					if (ecbd_get_BSSTransitionActivation(dpkt->event.ifname, &activate) == 0)
#else
					if (wlcsm_mngr_wifi_getBSSTransitionActivation(ap_idx, &activate) == 0)
#endif
					{
						if (activate == 0) {
							ECBD_PRINT_INFO("%s, BTM is not activated for apidx=%d\n", __FUNCTION__, ap_idx);
							break;
						}
					}
					else {
						ECBD_PRINT_ERROR("%s, fail to get BTM activation for apidx=%d\n", __FUNCTION__, ap_idx);
						break;
					}

					if (bytes <= (IFNAMSIZ + BCM_EVENT_HEADER_LEN) || bytes > MAX_EVENT_BUFFER_LEN) {
						ECBD_PRINT_ERROR("length of WLC_E_BSSTRANSx %d is not correct\n", bytes);
						break;
					}

					sta_info = ecbd_retrieve_sta_info(&(dpkt->event.addr), dpkt->event.ifidx, dpkt->event.bsscfgidx);

					/* Only invoke the BTM callback for configured STA */
					if (sta_info && (sta_info->type & STA_TYPE_CLIENT_SET) && sta_info->cli_cfg) {
						wifi_hal_cb_evt_t *vptr;
						int plen = 0, vlen = 0;

						ECBD_PRINT_DUMP("REVD WLC_E_BSSTRANS:", ptr, bytes);

						/* payload length */
						plen = bytes - (IFNAMSIZ + BCM_EVENT_HEADER_LEN);

						/* to wifi_hal event length */
						vlen = sizeof(wifi_hal_cb_evt_t) + plen;

						vptr = (wifi_hal_cb_evt_t *) malloc(vlen);
						if (vptr == NULL) {
							ECBD_PRINT_ERROR("Err: alloc buff %d for WLC_E_BSSTRANSx\n", vlen);
							break;
						}

						memset(vptr, 0, vlen);

						vptr->version = WIFI_HAL_EVT_VERSION;
						vptr->type = WIFI_HAL_CB_BSSTRANS;

						/* use reason as subtype */
						if (event_id == WLC_E_BSSTRANS_REQ)
							vptr->reason = WIFI_HAL_CB_BSSTRANS_REQ;
						else if (event_id == WLC_E_BSSTRANS_QUERY)
							vptr->reason = WIFI_HAL_CB_BSSTRANS_QUERY;
						else
							vptr->reason = WIFI_HAL_CB_BSSTRANS_RESP;

						vptr->apIndex = ap_idx;
						strncpy(vptr->mac, ether_etoa(dpkt->event.addr.octet, buf), MAX_MAC_ADDR_LEN - 1);

						/* point to payload */
						ptr += BCM_EVENT_HEADER_LEN;
						memcpy(vptr->data, ptr, plen);

						ECBD_PRINT_INFO("Notify Event %d (WLC_E_BSSTRANSx) to wifi_hal: MAC=%s bytes=%d plen=%d vlen=%d (%d %d) ptr=%p\n",
							event_id, ether_etoa(dpkt->event.addr.octet, buf),
							bytes, plen, vlen, sizeof(mac_address_t), sizeof(wifi_hal_cb_evt_t), ptr);

						ecbd_notify_wifi_hal_subscribers(vptr, vlen, WIFI_HAL_CB_BSSTRANS);

						if (vptr) {
							free(vptr);
							vptr = NULL;
						}
					}

				}
				break;

			case WLC_E_AP_CHAN_CHANGE:
				{
					wifi_hal_cb_evt_t *cb_evtp;
					int cb_len;
					wl_event_change_chan_t *ch_chg_evtp;
					unsigned char *cbdp;

					ECBD_PRINT_INFO("Event %d (WLC_E_AP_CHAN_CHANGE): \n", event_id);

					ptr += BCM_EVENT_HEADER_LEN;
					ch_chg_evtp = (wl_event_change_chan_t *)ptr;
					if ((ch_chg_evtp->length != WL_CHAN_CHANGE_EVENT_LEN_VER_1) ||
						(ch_chg_evtp->version != WL_CHAN_CHANGE_EVENT_VER_1)) {
						ECBD_PRINT_ERROR("WLC_E_AP_CHAN_CHANGE event skipped "
							"Length=%d or versioni%d mismatch \n",
							ch_chg_evtp->length, ch_chg_evtp->version);
					} else {
						ECBD_PRINT_INFO("reason=%d target chanspec=0x%x\n",
							ch_chg_evtp->reason, ch_chg_evtp->target_chanspec);
						cb_len = sizeof(wifi_hal_cb_evt_t) + ch_chg_evtp->length;
						cb_evtp = (wifi_hal_cb_evt_t *) malloc(cb_len);
						memset(cb_evtp, 0, cb_len);

						ECBD_PRINT_INFO("Event %d (WLC_E_AP_CHAN_CHANGE): "
							"MAC=%s ifidx=%d, bssidx=%d datalen=%d \n",
							event_id, ether_etoa(dpkt->event.addr.octet, buf),
							dpkt->event.ifidx, dpkt->event.bsscfgidx,
							dpkt->event.datalen);
						cb_evtp->version = WIFI_HAL_EVT_VERSION;
						cb_evtp->apIndex = dpkt->event.ifidx;
						cb_evtp->type = WIFI_HAL_CB_CH_CHG;
						strncpy(cb_evtp->mac, ether_etoa(dpkt->event.addr.octet, buf),
							MAX_MAC_ADDR_LEN - 1);
						cbdp = (unsigned char *)cb_evtp->data;
						memcpy(cbdp, (unsigned char *)(ch_chg_evtp),
							ch_chg_evtp->length);
						ECBD_PRINT_INFO("ecbd_process_eapd_evt cb apIndex=%d "
							"type=0x%x cb_len=%d\n",
							cb_evtp->apIndex, cb_evtp->type, cb_len);

						ecbd_notify_wifi_hal_subscribers(cb_evtp, cb_len,
							WIFI_HAL_CB_CH_CHG);
						if (cb_evtp) {
							free(cb_evtp);
							cb_evtp = NULL;
						}
					}
				}
				break;

			case WLC_E_RADAR_DETECTED:
				{
					wifi_hal_cb_evt_t *cb_evtp;
					int cb_len;
					wl_event_radar_detect_data_t *radar_evtp;
					unsigned char *cbdp;

					ECBD_PRINT_INFO("Event %d (WLC_E_RADAR_DETECTED): \n", event_id);

					ptr += BCM_EVENT_HEADER_LEN;
					radar_evtp = (wl_event_radar_detect_data_t *)ptr;
					ECBD_PRINT_INFO("current chspec=0x%x target chspec=0x%x\n",
						radar_evtp->current_chanspec,
						radar_evtp->target_chanspec);
					cb_len = sizeof(wifi_hal_cb_evt_t) + sizeof(wl_event_radar_detect_data_t);
					cb_evtp = (wifi_hal_cb_evt_t *) malloc(cb_len);
					memset(cb_evtp, 0, cb_len);

					ECBD_PRINT_INFO("Event %d (WLC_E_RADAR_DETECTED): "
						"MAC=%s ifidx=%d, bssidx=%d datalen=%d \n",
						event_id, ether_etoa(dpkt->event.addr.octet, buf),
						dpkt->event.ifidx, dpkt->event.bsscfgidx,
						dpkt->event.datalen);
					cb_evtp->version = WIFI_HAL_EVT_VERSION;
					cb_evtp->apIndex = dpkt->event.ifidx;
					cb_evtp->type = WIFI_HAL_CB_RADAR;
					strncpy(cb_evtp->mac, ether_etoa(dpkt->event.addr.octet, buf),
						MAX_MAC_ADDR_LEN - 1);
					cbdp = (unsigned char *)cb_evtp->data;
					memcpy(cbdp, (unsigned char *)(radar_evtp),
						sizeof(wl_event_radar_detect_data_t));
					ECBD_PRINT_INFO("ecbd_process_eapd_evt cb apIndex=%d "
						"type=0x%x cb_len=%d\n",
						cb_evtp->apIndex, cb_evtp->type, cb_len);

					ecbd_notify_wifi_hal_subscribers(cb_evtp, cb_len,
							WIFI_HAL_CB_CH_CHG);
					if (cb_evtp) {
						free(cb_evtp);
						cb_evtp = NULL;
					}
				}
				break;

			default:
				ECBD_PRINT_WARNING("Event %d (unknown): MAC=%s ifidx=%d, bssidx=%d\n",
					event_id, ether_etoa(dpkt->event.addr.octet, buf),
					dpkt->event.ifidx, dpkt->event.bsscfgidx);
				/* TODO */
				break;
		}
	}
}

static void
ecbd_process_wifi_hal_msg(int sock_fd)
{
	int bytes;
	unsigned char buf_ptr[MAX_EVENT_BUFFER_LEN] = {0}, *ptr = buf_ptr;
	wifi_hal_message_t *dpkt;
	uint hal_msg_type = 0;
	uint hal_msg_len = 0;
	wifi_steering_setGroup_t *setgroup;
	uint g_idx;
	wifi_steering_apConfig_t *apcfg;
	wifi_steering_group_t *group_ptr;
	wifi_steering_client_t *cli_ptr;
	ecbd_stalist_t *sta_info = NULL;
	struct ether_addr local_ea;
	uint8 ifidx, bsscfgidx;
	char ifname[BCM_MSG_IFNAME_MAX];
	int rssi;
	uint type, reason, *iptr;

	if ((bytes = recv(sock_fd, ptr, MAX_EVENT_BUFFER_LEN, 0)) > IFNAMSIZ) {

		dpkt = (wifi_hal_message_t *)ptr;

		ECBD_PRINT_DUMP("REVD:", ptr, bytes);
		hal_msg_type = dpkt->hal_msg_type;
		hal_msg_len = dpkt->hal_msg_len;

		ECBD_PRINT_DEBUG("Received wifi_hal msg type=%d, len=%d\n",
			hal_msg_type, hal_msg_len);

		switch (hal_msg_type) {
			case WIFI_HAL_MSG_AP_CONFIG:
				ECBD_PRINT_INFO("HAL msg %d (WIFI_HAL_MSG_AP_CONFIG), wifi_steering_apConfig_t=%d\n",
					hal_msg_type, sizeof(wifi_steering_apConfig_t));

				ptr = (unsigned char *)(dpkt->data);
				setgroup = (wifi_steering_setGroup_t *)ptr;
				g_idx = setgroup->steeringgroupIndex;
				ECBD_PRINT_INFO("groupIndex=%d, dpkt=%p ptr=%p\n",
					g_idx, dpkt, ptr);

				if (g_idx < 0 || g_idx >= MAX_STEERING_GROUP_NUM) {
					ECBD_PRINT_ERROR("group index %d out of range\n", g_idx);
					return;
				}

				group_ptr = &steer_groups[g_idx];

				if (hal_msg_len < sizeof(wifi_steering_apConfig_t)) {
					/* remove/disable this group */
					ECBD_PRINT_INFO("Disable groupIndex=%d\n", g_idx);
					group_ptr->group_enable = 0;
					if (group_ptr->group_info)
						free(group_ptr->group_info);
				}
				else {
					int i, len_left = hal_msg_len - sizeof(g_idx);

					/* config this group */
					ECBD_PRINT_INFO("Config groupIndex=%d len_left=%d\n", g_idx, len_left);
					if (!group_ptr->group_info) {
						group_ptr->group_info = malloc(sizeof(wifi_steering_group_info_t));
						if (!group_ptr->group_info) {
							ECBD_PRINT_ERROR("Exiting malloc failure for group %d\n", g_idx);
							return;
						}
					}

					group_ptr->group_index = g_idx;
					group_ptr->group_enable = 1;

					for (i = 0; i < HAL_RADIO_NUM_RADIOS; i++) {
						/* clean cfg in case partial config */
						memset(&(group_ptr->group_info->cfg[i]), 0, sizeof(wifi_steering_apConfig_t));

						if (len_left < sizeof(wifi_steering_apConfig_t)) {
							ECBD_PRINT_INFO("Index=%d len_left=%d exit\n", i, len_left);
							break;
						}

						apcfg = &(setgroup->cfg[i]);
						memcpy(&(group_ptr->group_info->cfg[i]), apcfg, sizeof(wifi_steering_apConfig_t));
						ECBD_PRINT_INFO("AP CFG: apidx=%d, %d, %d, %d, %d\n",
							apcfg->apIndex, apcfg->utilCheckIntervalSec,
							apcfg->utilAvgCount, apcfg->inactCheckIntervalSec,
							apcfg->inactCheckThresholdSec);
						/* clean stats */
						memset(&(group_ptr->group_info->stats[i]), 0, sizeof(wifi_steering_apStats_t));

						/* use probe/authresp_mac_filter + allow maclist to control response */
						if (ecbd_apidx_to_ifname(apcfg->apIndex, ifname) == ECBD_SUCCESS) {
							ecbd_enable_resp_filter(ifname);
							ecbd_enable_macmode_allow(ifname);
						}

						/* add log when cloud to config AP group */
						snprintf(log_buf, sizeof(log_buf), "Config ApGroup %d CFG: apidx=%d, %d, %d, %d, %d\n",
							g_idx, apcfg->apIndex,
							apcfg->utilCheckIntervalSec, apcfg->utilAvgCount,
							apcfg->inactCheckIntervalSec, apcfg->inactCheckThresholdSec);
						WLAN_APPS_LOGS("ECBD", log_buf);

						len_left -= sizeof(wifi_steering_apConfig_t);
					}
				}
				break;

			case WIFI_HAL_MSG_CLIENT_SET:
				cli_ptr = (wifi_steering_client_t *)dpkt->data;
				ECBD_PRINT_INFO("WIFI_HAL_MSG_CLIENT_SET: group_idx=%d ap_idx=%d cli_mac="ECBD_MACF"\n",
					cli_ptr->groupIndex, cli_ptr->apIndex, MAC_TO_MACF(cli_ptr->cli_mac));

				/* try to find the sta from the list */
				ifidx = HAL_AP_IDX_TO_HAL_RADIO(cli_ptr->apIndex);
				bsscfgidx = HAL_AP_IDX_TO_SSID_IDX(cli_ptr->apIndex);
				memcpy(local_ea.octet, cli_ptr->cli_mac, sizeof(mac_address_t));

				ECBD_PRINT_INFO("ifidx=%d bsscfgidx=%d, ea-mac="MACF"\n",
					ifidx, bsscfgidx, ETHER_TO_MACF(local_ea));

				if (ecbd_apidx_to_ifname(cli_ptr->apIndex, ifname) != ECBD_SUCCESS) {
					ECBD_PRINT_ERROR("Fail to get ifname for apidx %d\n", cli_ptr->apIndex);
					ifname[0] = 0;
				}
				sta_info = ecbd_add_stalist(NULL, &local_ea, ifidx, bsscfgidx, ifname, STA_TYPE_CLIENT_SET);
				if (sta_info != NULL) {
					/* allocate config buf for this client */
					if (sta_info->cli_cfg == NULL) {
						sta_info->cli_cfg = malloc(sizeof(wifi_steering_clientConfig_t));
						if (sta_info->cli_cfg == NULL) {
							ECBD_PRINT_ERROR("Fail to allocate config memory for Client "MACF"\n",
								ETHER_TO_MACF(local_ea));
							break;
						}
					}
					memcpy(sta_info->cli_cfg, cli_ptr->data, sizeof(wifi_steering_clientConfig_t));
					/* add to deny list when STA config */
					// ecbd_add_maclist(ifname, &local_ea);
					ECBD_PRINT_INFO("Config for "MACF": rssiProbeHWM=%d, rssiProbeLWM=%d\n",
						ETHER_TO_MACF(local_ea), sta_info->cli_cfg->rssiProbeHWM,
							sta_info->cli_cfg->rssiProbeLWM);

					/* add log when cloud to config a Client */
					snprintf(log_buf, sizeof(log_buf), "Config client ifname=%s, "MACF" pHWM=%d, pLWM=%d, "
						"aHWM=%d, aLWM=%d, iXing=%d, hXing=%d, lXing=%d Reason=%d\n",
						ifname, ETHER_TO_MACF(local_ea),
						sta_info->cli_cfg->rssiProbeHWM, sta_info->cli_cfg->rssiProbeLWM,
						sta_info->cli_cfg->rssiAuthHWM, sta_info->cli_cfg->rssiAuthLWM,
						sta_info->cli_cfg->rssiInactXing, sta_info->cli_cfg->rssiHighXing,
						sta_info->cli_cfg->rssiLowXing, sta_info->cli_cfg->authRejectReason);
					WLAN_APPS_LOGS("ECBD", log_buf);

				}
				break;

			case WIFI_HAL_MSG_CLIENT_RM:
				cli_ptr = (wifi_steering_client_t *)dpkt->data;
				ECBD_PRINT_INFO("group_idx=%d ap_idx=%d, Client-MAC="ECBD_MACF"\n",
					cli_ptr->groupIndex, cli_ptr->apIndex, MAC_TO_MACF(cli_ptr->cli_mac));

				/* try to find the sta from the list */
				ifidx = HAL_AP_IDX_TO_HAL_RADIO(cli_ptr->apIndex);
				bsscfgidx = HAL_AP_IDX_TO_SSID_IDX(cli_ptr->apIndex);
				memcpy(local_ea.octet, cli_ptr->cli_mac, sizeof(mac_address_t));

				sta_info = ecbd_retrieve_sta_info(&local_ea, ifidx, bsscfgidx);
				if (sta_info == NULL) {
					ECBD_PRINT_INFO("Client "MACF" is not found, apidx=%d ifidx=%d bssidx=%d\n",
						ETHER_TO_MACF(local_ea), cli_ptr->apIndex, ifidx, bsscfgidx);
					break;
				}

				/* free config buf for this client */
				if (sta_info->cli_cfg != NULL) {
					free(sta_info->cli_cfg);
					sta_info->cli_cfg = NULL;
					sta_info->type &= ~STA_TYPE_CLIENT_SET;
					ECBD_PRINT_INFO("Clean config for "MACF"\n", ETHER_TO_MACF(local_ea));
				}

				if (ecbd_apidx_to_ifname(cli_ptr->apIndex, ifname) != ECBD_SUCCESS) {
					ECBD_PRINT_ERROR("Fail to get ifname for apidx %d\n", cli_ptr->apIndex);
					ifname[0] = 0;
				}
				/* remove from the deny list */
				ecbd_del_maclist(ifname, &local_ea);
				break;

			case WIFI_HAL_MSG_CLIENT_MEAS:
				cli_ptr = (wifi_steering_client_t *)dpkt->data;
				ECBD_PRINT_INFO("Measure: group_idx=%d ap_idx=%d, Client-MAC="ECBD_MACF"\n",
					cli_ptr->groupIndex, cli_ptr->apIndex, MAC_TO_MACF(cli_ptr->cli_mac));

				if (ecbd_apidx_to_ifname(cli_ptr->apIndex, ifname) == ECBD_SUCCESS) {
					ECBD_PRINT_INFO("group_idx=%d ap_idx=%d, Client-MAC="ECBD_MACF"\n",
						cli_ptr->groupIndex, cli_ptr->apIndex, MAC_TO_MACF(cli_ptr->cli_mac));

					/* try to find the sta from the list */
					ifidx = HAL_AP_IDX_TO_HAL_RADIO(cli_ptr->apIndex);
					bsscfgidx = HAL_AP_IDX_TO_SSID_IDX(cli_ptr->apIndex);
					memcpy(local_ea.octet, cli_ptr->cli_mac, sizeof(mac_address_t));

					sta_info = ecbd_retrieve_sta_info(&local_ea, ifidx, bsscfgidx);
					if (sta_info == NULL) {
						ECBD_PRINT_INFO("Client "MACF" is not found, apidx=%d ifidx=%d bssidx=%d\n",
							ETHER_TO_MACF(local_ea), cli_ptr->apIndex, ifidx, bsscfgidx);
						break;
					}

					if (ecbd_get_sta_rssi(ifname, &local_ea, &rssi) == ECBD_SUCCESS) {
						ECBD_PRINT_INFO("Measure "MACF" snr=%d\n", ETHER_TO_MACF(local_ea), rssi);
						sta_info->rssi = rssi;
						/* send RSSI event */
						ecbd_send_rssi_measure_event(cli_ptr->groupIndex, cli_ptr->apIndex, sta_info);
					}
					else {
						ECBD_PRINT_ERROR("Fail to measure "MACF" rssi\n", ETHER_TO_MACF(local_ea));
					}
				}
				else {
					ECBD_PRINT_ERROR("Err: wrong ifname to measure Client-MAC="ECBD_MACF"\n",
						MAC_TO_MACF(cli_ptr->cli_mac));
				}
				break;

			case WIFI_HAL_MSG_CLIENT_DISCONN:
				/* disconnect this client */
				cli_ptr = (wifi_steering_client_t *)dpkt->data;
				ECBD_PRINT_INFO("disconnect: group_idx=%d ap_idx=%d, Client-MAC="ECBD_MACF"\n",
					cli_ptr->groupIndex, cli_ptr->apIndex, MAC_TO_MACF(cli_ptr->cli_mac));

				if (ecbd_apidx_to_ifname(cli_ptr->apIndex, ifname) == ECBD_SUCCESS) {
					ECBD_PRINT_INFO("group_idx=%d ap_idx=%d, Client-MAC="ECBD_MACF"\n",
						cli_ptr->groupIndex, cli_ptr->apIndex, MAC_TO_MACF(cli_ptr->cli_mac));

					/* try to find the sta from the list */
					ifidx = HAL_AP_IDX_TO_HAL_RADIO(cli_ptr->apIndex);
					bsscfgidx = HAL_AP_IDX_TO_SSID_IDX(cli_ptr->apIndex);
					memcpy(local_ea.octet, cli_ptr->cli_mac, sizeof(mac_address_t));

					sta_info = ecbd_retrieve_sta_info(&local_ea, ifidx, bsscfgidx);
					if (sta_info == NULL) {
						ECBD_PRINT_INFO("Client "MACF" is not found, apidx=%d ifidx=%d bssidx=%d\n",
							ETHER_TO_MACF(local_ea), cli_ptr->apIndex, ifidx, bsscfgidx);
						break;
					}
					iptr = (uint*)cli_ptr->data;
					type = *iptr;
					reason = *(iptr+1);
					ECBD_PRINT_INFO("disconnect: type=%d reason=%d (%p, %p)\n", type, reason, iptr, iptr+1);
					ecbd_disconnect_sta(ifname, &(sta_info->addr), type, reason);
				}
				else {
					ECBD_PRINT_ERROR("Err: wrong ifname to disconnect Client-MAC="ECBD_MACF"\n",
						MAC_TO_MACF(cli_ptr->cli_mac));
				}

				break;

			default:
				ECBD_PRINT_WARNING("WiFi HAL message %d (unknown)\n", hal_msg_type);
				break;
		}
	}
}

static void
sigterm_handler(int signum, siginfo_t *info, void *ptr)
{
	int i = 1;

	ecbd_close_cb_dsockets();
	ecbd_watchdog_tmr_delete();

	for (i = 1; i < nfds; i++) {
		if (fds[i].fd >= 0) {
			close(fds[i].fd);
			fds[i].fd = -1;
		}
	}

	free(ecbd_info);
	ecbd_info = NULL;
	_Exit(0);
}

static void
daemon_sigterm_install(int *term_fd)
{
	struct sigaction act;

	sigemptyset(&act.sa_mask);
	act.sa_sigaction = sigterm_handler;
	act.sa_flags = SA_SIGINFO | SA_RESETHAND;
	if (sigaction(SIGTERM, &act, NULL) < 0) {
		ECBD_PRINT_ERROR("sigaction TERM failed: %s", strerror(errno));
		_Exit(EXIT_FAILURE);
	}
	*term_fd = 0;
}

static int
ecbd_create_dserver_socket(char *sock_path)
{
	int socket_fd = -1;
	struct sockaddr_un address;
	socklen_t address_length = sizeof(struct sockaddr_un);

	if (sock_path == NULL) {
		ECBD_PRINT_ERROR("%s: Invalid sock_path NULL\n", __FUNCTION__);
		return ECBD_FAIL;
	}

	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		ECBD_PRINT_ERROR("socket() failed for %s due to %s\n",
			sock_path, strerror(errno));
		return ECBD_FAIL;
	}

	unlink(sock_path);

	/* start with a clean address structure */
	memset(&address, 0, sizeof(struct sockaddr_un));
	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, UNIX_PATH_MAX, sock_path);

	if (bind(socket_fd, (struct sockaddr *) &address,
			sizeof(struct sockaddr_un)) != 0)
	{
		ECBD_PRINT_ERROR("bind() failed for %s due to %s\n",
			sock_path, strerror(errno));
		goto exit;
	}

	if (listen(socket_fd, MAX_CB_SUBSCRIBERS) != 0)
	{
		ECBD_PRINT_ERROR("listen() failed for %s due to %s\n",
			sock_path, strerror(errno));
		goto exit;
	}

	if (strcmp(sock_path, WIFI_HAL_CB_STA_CONN_DSOCKET) == 0) {
		strncpy(ecbd_info->sta_conn_cb_subscriber_fds.sock_path, sock_path, UNIX_PATH_MAX - 1);
		memcpy(&ecbd_info->sta_conn_cb_subscriber_fds.addr, &address, sizeof(struct sockaddr_un));
		memcpy(&ecbd_info->sta_conn_cb_subscriber_fds.addr_len, &address_length, sizeof(socklen_t));
	}
	else if (strcmp(sock_path, WIFI_HAL_CB_ASSOC_DEV_DSOCKET) == 0) {
		strncpy(ecbd_info->assoc_dev_cb_subscriber_fds.sock_path, sock_path, UNIX_PATH_MAX - 1);
		memcpy(&ecbd_info->assoc_dev_cb_subscriber_fds.addr, &address, sizeof(struct sockaddr_un));
		memcpy(&ecbd_info->assoc_dev_cb_subscriber_fds.addr_len, &address_length, sizeof(socklen_t));
	}
	else if (strcmp(sock_path, WIFI_HAL_CB_AUTH_FAIL_DSOCKET) == 0) {
		strncpy(ecbd_info->auth_fail_cb_subscriber_fds.sock_path, sock_path, UNIX_PATH_MAX - 1);
		memcpy(&ecbd_info->auth_fail_cb_subscriber_fds.addr, &address, sizeof(struct sockaddr_un));
		memcpy(&ecbd_info->auth_fail_cb_subscriber_fds.addr_len, &address_length, sizeof(socklen_t));
	}
	else if (strcmp(sock_path, WIFI_HAL_CB_MESH_STEER_DSOCKET) == 0) {
		strncpy(ecbd_info->mesh_cb_subscriber_fds.sock_path, sock_path, UNIX_PATH_MAX - 1);
		memcpy(&ecbd_info->mesh_cb_subscriber_fds.addr, &address, sizeof(struct sockaddr_un));
		memcpy(&ecbd_info->mesh_cb_subscriber_fds.addr_len, &address_length, sizeof(socklen_t));
	}
	else if (strcmp(sock_path, WIFI_HAL_CB_RRM_BCNREP_DSOCKET) == 0) {
		strncpy(ecbd_info->bcn_report_cb_subscriber_fds.sock_path, sock_path, UNIX_PATH_MAX - 1);
		memcpy(&ecbd_info->bcn_report_cb_subscriber_fds.addr, &address, sizeof(struct sockaddr_un));
		memcpy(&ecbd_info->bcn_report_cb_subscriber_fds.addr_len, &address_length, sizeof(socklen_t));
	}
	else if (strcmp(sock_path, WIFI_HAL_CB_BSSTRANS_DSOCKET) == 0) {
		strncpy(ecbd_info->bsstrans_cb_subscriber_fds.sock_path, sock_path, UNIX_PATH_MAX - 1);
		memcpy(&ecbd_info->bsstrans_cb_subscriber_fds.addr, &address, sizeof(struct sockaddr_un));
		memcpy(&ecbd_info->bsstrans_cb_subscriber_fds.addr_len, &address_length, sizeof(socklen_t));
	}
	else if (strcmp(sock_path, WIFI_HAL_CB_DPP_DSOCKET) == 0) {
		strncpy(ecbd_info->dpp_cb_subscriber_fds.sock_path, sock_path, UNIX_PATH_MAX - 1);
		memcpy(&ecbd_info->dpp_cb_subscriber_fds.addr, &address, sizeof(struct sockaddr_un));
		memcpy(&ecbd_info->dpp_cb_subscriber_fds.addr_len, &address_length, sizeof(socklen_t));
	}
	else if (strcmp(sock_path, WIFI_HAL_CB_CH_CHG_DSOCKET) == 0) {
		strncpy(ecbd_info->ch_chg_cb_subscriber_fds.sock_path, sock_path, UNIX_PATH_MAX - 1);
		memcpy(&ecbd_info->ch_chg_cb_subscriber_fds.addr, &address, sizeof(struct sockaddr_un));
		memcpy(&ecbd_info->ch_chg_cb_subscriber_fds.addr_len, &address_length, sizeof(socklen_t));
	}

	return socket_fd;
exit:
	if (socket_fd >= 0)
		close(socket_fd);
	unlink(sock_path);
	return ECBD_FAIL;
}

static int
ecbd_create_udp_socket(unsigned int port)
{
	int reuse = 1;
	int fd = -1;
	struct sockaddr_in sockaddr;

	/* open loopback socket to communicate with EAPD */
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sockaddr.sin_port = htons(port);

	if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		ECBD_PRINT_ERROR("Unable to create loopback socket\n");
		return ECBD_FAIL;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
		ECBD_PRINT_ERROR("Unable to setsockopt to loopback socket %d.\n", fd);
		goto exit;
	}

	if (bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		ECBD_PRINT_ERROR("Unable to bind to loopback socket %d\n", fd);
		goto exit;
	}

	ECBD_PRINT_INFO("opened loopback socket %d\n", fd);
	return fd;

exit:
	close(fd);
	return ECBD_FAIL;
}

static int
ecbd_apidx_to_ifname(int apIndex, char *ifname)
{
	int idx, subidx;

	if ((apIndex >= HAL_WIFI_TOTAL_NO_OF_APS) || (apIndex < 0)) {
		ECBD_PRINT_ERROR("Err: wrong apindex %d\n", apIndex);
		return ECBD_FAIL;
	}

	idx = HAL_AP_IDX_TO_HAL_RADIO(apIndex);
	subidx = HAL_AP_IDX_TO_SSID_IDX(apIndex);

	if (subidx == 0) {
		snprintf(ifname, BCM_MSG_IFNAME_MAX, "wl%d", idx);
	} else {
		snprintf(ifname, BCM_MSG_IFNAME_MAX, "wl%d.%d", idx, subidx);
	}

	ECBD_PRINT_INFO("%s: apIndex=%d idx=%d subidx=%d ifname=%s\n",
		__FUNCTION__, apIndex, idx, subidx, ifname);

	return ECBD_SUCCESS;
}

static int
#ifdef RDKB_RADIO_STATS_MEASURE
ecbd_get_chanim_stats_bw_util(char *ifname, int *bw_util, ecbd_radio_stats_data_t *pdata)
#else
ecbd_get_chanim_stats_bw_util(char *ifname, int *bw_util)
#endif /* RDKB_RADIO_STATS_MEASURE */
{
	char ioctl_buf[MAX_IOCTL_BUFLEN];
	int buflen = sizeof(ioctl_buf);
	int txop;
	wl_chanim_stats_t param;
	wl_chanim_stats_t *list;
	char *ptr;
	int tlen;
	int ret = -1;
	chanim_stats_t *stats;

	/* Get WL_CHANIM_COUNT_ALL ioctl values. */
	list = (wl_chanim_stats_t *) ioctl_buf;

	memset(&param, 0, sizeof(param));
	param.buflen = buflen;
	param.count = 1;
	param.version = WL_CHANIM_STATS_VERSION;

	memset(ioctl_buf, 0, sizeof(ioctl_buf));
	strcpy(ioctl_buf, "chanim_stats");
	tlen = strlen(ioctl_buf) + 1;
	ptr = (char *)(ioctl_buf + tlen);
	memcpy(ptr, &param, sizeof(wl_chanim_stats_t));

	ret = wl_ioctl(ifname, WLC_GET_VAR, ioctl_buf, sizeof(ioctl_buf));

	if (ret < 0) {
		ECBD_PRINT_ERROR("Err: get chanim_stats for intf %s \n", ifname);
		return ECBD_FAIL;
	}

	if (list->version < WL_CHANIM_STATS_VERSION_3 ||
		list->version > WL_CHANIM_STATS_VERSION) {
		ECBD_PRINT_ERROR("Err: chanim_stats version %d doesn't match, "
			"this progrem supports version [%d..%d]\n",
			list->version, WL_CHANIM_STATS_VERSION_3, WL_CHANIM_STATS_VERSION);
		return ECBD_FAIL;
	}

	stats = list->stats;

	if (stats == NULL) {
		ECBD_PRINT_ERROR("Err: stats for chanim_stats is NULL\n");
		return ECBD_FAIL;
	}

	txop = stats->ccastats[CCASTATS_TXOP];
	txop = (txop > 100) ? 100 : txop;
	*bw_util = 100 - txop;

#ifdef RDKB_RADIO_STATS_MEASURE
	if (pdata) {
		pdata->act_factor = stats->ccastats[CCASTATS_TXDUR] +
			stats->ccastats[CCASTATS_INBSS];
		pdata->obss = stats->ccastats[CCASTATS_OBSS];
	}
#endif /* RDKB_RADIO_STATS_MEASURE */

	return ECBD_SUCCESS;
}

static void
ecbd_update_group_chan_util(int tick, wifi_steering_group_t *g_ptr)
{
	wifi_steering_group_info_t *g_info;
	wifi_steering_apConfig_t *ap_cfg;
	wifi_steering_apStats_t *ap_stats;
	char ifname[BCM_MSG_IFNAME_MAX];
	int i, len;
	uint bw_util;
	char buf[MAX_EVENT_BUFFER_LEN], *bufptr = buf;
	wifi_steering_event_t *steering_evt;

	if (g_ptr == NULL) {
		ECBD_PRINT_ERROR("Err: g_ptr NULL pointer\n");
		return;
	}

	g_info = g_ptr->group_info;
	if (g_info == NULL) {
		ECBD_PRINT_ERROR("Err: g_info NULL pointer\n");
		return;
	}

	for (i = 0; i < HAL_RADIO_NUM_RADIOS; i++) {
		ap_cfg = &g_info->cfg[i];
		ap_stats = &g_info->stats[i];

		if ((ap_cfg->utilCheckIntervalSec != 0) && ((tick % ap_cfg->utilCheckIntervalSec) == 0)) {
			if (ecbd_apidx_to_ifname(ap_cfg->apIndex, ifname) == ECBD_SUCCESS) {
				/* read bw_util for ifname */
				ECBD_PRINT_INFO("update channel utilization for %s\n", ifname);
#ifdef RDKB_RADIO_STATS_MEASURE
				if (ecbd_get_chanim_stats_bw_util(ifname, (int *)(&bw_util), NULL) == ECBD_SUCCESS) {
#else
				if (ecbd_get_chanim_stats_bw_util(ifname, (int *)(&bw_util)) == ECBD_SUCCESS) {
#endif /* RDKB_RADIO_STATS_MEASURE */
					ECBD_PRINT_INFO("Got bw_util %d for %s, current avg=%d\n",
						bw_util, ifname, ap_stats->avg_chan_util);
					if (ap_cfg->utilAvgCount <= 1)
						ap_stats->avg_chan_util = bw_util;
					else {
						ap_stats->avg_chan_util = (ap_stats->avg_chan_util +
							(ap_cfg->utilAvgCount - 1) * bw_util) / ap_cfg->utilAvgCount;
					}
					ECBD_PRINT_INFO("after calculate avg_chan_util=%d on cnt %d\n",
						ap_stats->avg_chan_util, ap_cfg->utilAvgCount);

					ap_stats->chan_util_cnt++;
					if (ap_stats->chan_util_cnt >= ap_cfg->utilAvgCount) {
						ap_stats->chan_util_cnt = 0;
						/* send event to wifi_hal */
						*(uint*)bufptr = g_ptr->group_index;
						steering_evt = (wifi_steering_event_t *)(bufptr + sizeof(uint));
						steering_evt->type = WIFI_STEERING_EVENT_CHAN_UTILIZATION;
						steering_evt->apIndex = ap_cfg->apIndex;
						steering_evt->timestamp_ms = time(NULL);
						steering_evt->data.chanUtil.utilization = ap_stats->avg_chan_util;
						len = sizeof(uint) + sizeof(wifi_steering_event_t);

#ifndef RDKB_WLDM
						ecbd_notify_wifi_hal(bufptr, len, WIFI_HAL_CB_MESH_APCFG_UDP_PORT);
#else
						ecbd_notify_wifi_hal_subscribers(bufptr, len, WIFI_HAL_CB_MESH);
#endif
						ECBD_PRINT_DEBUG("Send type %d (%p, %p) len=%d, ch_util (%d) event "
							"to wifi hal at %llu (ullong=%d)\n",
							steering_evt->type, bufptr, steering_evt, len,
							ap_stats->avg_chan_util, steering_evt->timestamp_ms, sizeof(ULLONG));
					}
				}
			}
		}
	}
}

/* return apConfig pointer as well as group index */
static wifi_steering_apConfig_t*
ecbd_find_ap_cfg(uint8 ifidx, uint8 bsscfgidx, uint *g_idx)
{
	int i, j;
	wifi_steering_group_t *g_ptr;
	wifi_steering_group_info_t *g_info;
	wifi_steering_apConfig_t *ap_cfg;

	for (i = 0; i < MAX_STEERING_GROUP_NUM; i++)
	{
		g_ptr = &steer_groups[i];
		if (!g_ptr->group_enable || !g_ptr->group_info) {
			ECBD_PRINT_DEBUG("group %i not ready\n", i);
			continue;
		}

		g_info = g_ptr->group_info;
		for (j = 0; j < HAL_RADIO_NUM_RADIOS; j++) {
			ap_cfg = &g_info->cfg[j];

			ECBD_PRINT_DEBUG("ecbd_find_ap_cfg: %d, %d, %d\n",
				ap_cfg->apIndex, ifidx, bsscfgidx);
			if ((ifidx == HAL_AP_IDX_TO_HAL_RADIO(ap_cfg->apIndex)) &&
				(bsscfgidx == HAL_AP_IDX_TO_SSID_IDX(ap_cfg->apIndex))) {
				*g_idx = i;
				ECBD_PRINT_DEBUG("ecbd_find_ap_cfg: found group index %d, apcfg index %d\n",
					i, j);
				return ap_cfg;
			}
		}
	}
	return NULL;
}

static BOOL
ecbd_sta_in_blocked(ecbd_stalist_t *sta)
{
	int macmode = 0;
	char maclist_buf[MAX_IOCTL_BUFLEN];
	struct maclist *maclist = (struct maclist *) maclist_buf;
	int cnt;

	if (sta == NULL) {
		ECBD_PRINT_WARNING("sta null pointer\n");
		return FALSE;
	}

	ECBD_PRINT_DEBUG("check sta "MACF" on interface %s\n",
		ETHER_TO_MACF(sta->addr), sta->ifname);

	if (wl_ioctl(sta->ifname, WLC_GET_MACMODE,
		&macmode, sizeof(macmode)) < 0) {
		ECBD_PRINT_ERROR("fail to get macmode\n");
		return FALSE;
	}

	if (macmode != WLC_MACMODE_DENY) {
		ECBD_PRINT_DEBUG("macmode %d is not in deny mode\n", macmode);
		return FALSE;
	}

	if (wl_ioctl(sta->ifname, WLC_GET_MACLIST, (void *)maclist,
		sizeof(maclist_buf)) < 0) {
		ECBD_PRINT_ERROR("fail to get maclist\n");
		return FALSE;
	}

	for (cnt = 0; cnt < maclist->count; cnt++) {
		ECBD_PRINT_INFO("cnt[%d] mac:"MACF"\n", cnt,
			ETHER_TO_MACF(maclist->ea[cnt]));
		if (eacmp(&(maclist->ea[cnt]), &(sta->addr)) == 0) {
			ECBD_PRINT_DEBUG("found mac "MACF" in deny list\n", ETHER_TO_MACF(sta->addr));
			return TRUE;
		}
	}
	return FALSE;
}

/* use sta_info to get data for connect event,
   expect the content of ptr_conn has been initialized to 0. */
static int
ecbd_get_sta_conn_data(ecbd_stalist_t *sta, wifi_steering_evConnect_t *ptr_conn)
{
	char ioctl_buf[MAX_IOCTL_BUFLEN];
	char buf[32], *param;
	sta_info_t *sta_info;
	int buflen;
	int ret = ECBD_FAIL;
	int i, j, max = 0;
	uint max_mcs = 0, max_nss = 1;

	if (sta == NULL || ptr_conn == NULL) {
		ECBD_PRINT_ERROR("%s: null pointer\n", __FUNCTION__);
		return ret;
	}

	ECBD_PRINT_INFO("%s: MAC=%s on ifname %s\n", __FUNCTION__,
		ether_etoa(sta->addr.octet, buf), sta->ifname);

	strcpy(ioctl_buf, "sta_info");
	buflen = strlen(ioctl_buf) + 1;
	param = (char *)(ioctl_buf + buflen);
	memcpy(param, &sta->addr, ETHER_ADDR_LEN);

	if ((ret = wl_ioctl(sta->ifname, WLC_GET_VAR, ioctl_buf, sizeof(ioctl_buf))) == 0) {
		char *phy_str = "unknown";

		sta_info = (sta_info_t *)ioctl_buf;
		sta_info->ver = dtoh16(sta_info->ver);
		sta_info->flags = dtoh32(sta_info->flags);
		sta->flags = sta_info->flags;

		ECBD_PRINT_INFO("%s: version=%d flags=0x%x ht_cap=0x%x\n",
			__FUNCTION__, sta_info->ver, sta_info->flags, sta_info->ht_capabilities);

		/* phyMode per client */
		if (sta->flags & WL_STA_HE_CAP) {
			ptr_conn->datarateInfo.phyMode = PHY_TYPE_AX; /* 13 */
			phy_str = "ax";
		} else if (sta->flags & WL_STA_VHT_CAP) {
			ptr_conn->datarateInfo.phyMode = PHY_TYPE_AC; /* 11 */
			phy_str = "ac";
		} else if (sta->flags & WL_STA_N_CAP) {
			ptr_conn->datarateInfo.phyMode = PHY_TYPE_N; /* 4 */
			phy_str = "n";
		} else {
			ptr_conn->datarateInfo.phyMode = PHY_TYPE_G; /* 2 */
			phy_str = "g";
		}
		ECBD_PRINT_INFO("ifname=%s, Client-MAC="MACF" flags=0x%x phyMode=%d (%s)\n",
			sta->ifname, ETHER_TO_MACF(sta->addr), sta->flags,
			ptr_conn->datarateInfo.phyMode, phy_str);

		/* maxChwidth */
		switch (sta_info->link_bw) {
			case 1:
				ptr_conn->datarateInfo.maxChwidth = 20;
				break;
			case 2:
				ptr_conn->datarateInfo.maxChwidth = 40;
				break;
			case 3:
				ptr_conn->datarateInfo.maxChwidth = 80;
				break;
			case 4:
				ptr_conn->datarateInfo.maxChwidth = 160;
				break;
			default:
				ECBD_PRINT_ERROR("%s: wrong link_bw=%d\n", __FUNCTION__, sta_info->link_bw);
				break;
		}
		ECBD_PRINT_INFO("%s: maxChwidth=%d\n", __FUNCTION__, ptr_conn->datarateInfo.maxChwidth);

		/* mcs, nss, mu-mimo etc. */
		if (sta_info->flags & WL_STA_HE_CAP) {
			/* for 802.11ax */
			uint16 *rs_he_mcs = NULL;
			int tmp_nss = 1;
			wl_rateset_args_u_t *rateset_adv = NULL;

			if (sta_info->ver >= WL_STA_VER_7) {
				/* sta_info already uses v7 format */
				rateset_adv = (wl_rateset_args_u_t *)&sta_info->rateset_adv;
			}
			else {
				ECBD_PRINT_ERROR("%s: unsupported sta_info version %d for 11ax HE\n", __FUNCTION__, sta_info->ver);
				return ret;
			}

			if (rateset_adv == NULL) {
				ECBD_PRINT_ERROR("%s: rateset_adv null for 11ax HE\n", __FUNCTION__);
				return ret;
			}

			rs_he_mcs = rateset_adv->rsv2.he_mcs;
			if (rs_he_mcs != NULL && rs_he_mcs[0] != 0xffff) {
				/* parse rs_he_mcs, refer to wl_print_hemcsnss() */
				static const char zero[sizeof(uint16) * WL_HE_CAP_MCS_MAP_NSS_MAX] = { 0 };
				uint16 he_txmcsmap;
				uint tx_mcs;

				if (!memcmp(rs_he_mcs, zero, sizeof(uint16) * WL_HE_CAP_MCS_MAP_NSS_MAX)) {
					ECBD_PRINT_ERROR("%s: rs_he_mcs all zero\n", __FUNCTION__);
					return ret;
				}

				/* for bw 80/160/80p80 */
				for (i = 0; i < 3; i++) {
					he_txmcsmap = dtoh16(rs_he_mcs[i * 2]);

					ECBD_PRINT_INFO("%s: HE he_txmcsmap=0x%x i=%d\n", __FUNCTION__,
						he_txmcsmap, i);

					for (j = 1; j <= HE_CAP_MCS_MAP_NSS_MAX; j++) {
						tx_mcs = HE_CAP_MAX_MCS_NSS_GET_MCS(j, he_txmcsmap);

						ECBD_PRINT_INFO("%s: HE tx_mcs=0x%x j=%d\n", __FUNCTION__,
							tx_mcs, j);

						if (tx_mcs != HE_CAP_MAX_MCS_NONE) {
							max = (tx_mcs == HE_CAP_MAX_MCS_0_11 ? 11 :
								(tx_mcs == HE_CAP_MAX_MCS_0_9 ? 9 :
								(tx_mcs == HE_CAP_MAX_MCS_0_7 ? 7 : 0)));
							tmp_nss = j;
						}
					}
				}
			}
			max_mcs = max;
			max_nss = max?tmp_nss:1;

			/* isMUMimoSupported */
			if (sta_info->he_flags & (WL_STA_HE_SU_MU_BEAMFORMEE | WL_STA_HE_MU_BEAMFORMER)) {
				ptr_conn->datarateInfo.isMUMimoSupported = 1;
			}

			ECBD_PRINT_INFO("%s: HE he_flags=0x%x isMUMimoSupported=%d\n", __FUNCTION__,
				sta_info->he_flags, ptr_conn->datarateInfo.isMUMimoSupported);
		}
		else if (sta_info->flags & WL_STA_VHT_CAP) {
			/* VHT: use (uint16 *)sta_info->rateset_adv.vht_mcs */
			uint16 *mcsset = (uint16 *)sta_info->rateset_adv.vht_mcs;

			for (i = 0; i < VHT_CAP_MCS_MAP_NSS_MAX; i++) {
				ECBD_PRINT_INFO("%s: VHT mcsset=0x%x i=%d\n", __FUNCTION__,
					mcsset[i], i);

				if (mcsset[i]) {
					if (i == 0)
						ECBD_PRINT_INFO("VHT SET : ");
					else
						ECBD_PRINT_INFO("        : ");

					for (j = 0; j <= 9; j++) {
						if (isbitset(mcsset[i], j)) {
							if (ecbd_msglevel & ECBD_DEBUG_INFO)
								printf("%dx%d ", j, i + 1);
							max = j;
						}
					}
					if (ecbd_msglevel & ECBD_DEBUG_INFO)
						printf("\n");
				} else {
					break;
				}
			}
			max_mcs = max;
			max_nss = max?i:1;

			/* isMUMimoSupported */
			if (sta_info->vht_flags & (WL_STA_MU_BEAMFORMER | WL_STA_MU_BEAMFORMEE)) {
				ptr_conn->datarateInfo.isMUMimoSupported = 1;
			}

			ECBD_PRINT_INFO("%s: VHT vht_flags=0x%x isMUMimoSupported=%d\n", __FUNCTION__,
				sta_info->vht_flags, ptr_conn->datarateInfo.isMUMimoSupported);

		}
		else if (sta_info->ht_capabilities) {
			/* HT: use (char *)sta_info->rateset_adv.mcs */
			char *mcsset = (char *)sta_info->rateset_adv.mcs;

			for (i = 0; i < MCSSET_LEN; i++) {
				ECBD_PRINT_INFO("%s: HT mcsset=%x i=%d (%d)\n", __FUNCTION__,
					mcsset[i], i, MCSSET_LEN);
			}

			ECBD_PRINT_INFO("MCS SET : [ ");
			for (i = 0; i < (MCSSET_LEN * 8); i++) {
				if (isset(mcsset, i)) {
					if (ecbd_msglevel & ECBD_DEBUG_INFO)
						printf("%d ", i);
					max = i;
				}
			}
			ECBD_PRINT_INFO("]\n");

			max_mcs = max;

			if (max >= 0 && max <= 7)
				max_nss = 1;
			else if (max >= 8 && max <= 15)
				max_nss = 2;
			else if (max >= 16 && max <= 23)
				max_nss = 3;
			else if (max >= 24 && max <= 31)
				max_nss = 4;
			else {
				max_nss = 1;
				ECBD_PRINT_ERROR("%s: mcs index %d too big\n", __FUNCTION__, max);
			}
		} else {
			/* legacy */
			max_mcs = 0;
			max_nss = 1;
			ECBD_PRINT_INFO("%s: Legacy mode: mcs=%d nss=%d\n", __FUNCTION__,
				max_mcs, max_nss);
		}

		ptr_conn->datarateInfo.maxStreams = max_nss;
		ptr_conn->datarateInfo.maxMCS = max_mcs;

		ECBD_PRINT_INFO("%s: Got mcs=%d nss=%d\n", __FUNCTION__, max_mcs, max_nss);

		/* BTM */
		if (sta_info->wnm_cap & WL_WNM_BSSTRANS)
			ptr_conn->isBTMSupported = 1;

		ECBD_PRINT_INFO("%s: sta_info->wnm_cap=0x%x isBTMSupported=%d\n",
			__FUNCTION__, sta_info->wnm_cap, ptr_conn->isBTMSupported);

		/* RMCAP */
		if (sta->rm_cap.cap[0] || sta->rm_cap.cap[1] ||
			sta->rm_cap.cap[2] || sta->rm_cap.cap[3] ||
			sta->rm_cap.cap[4]) {
			ptr_conn->isRRMSupported = 1;
		}

		ECBD_PRINT_INFO("%s: isRRMSupported=%d\n", __FUNCTION__, ptr_conn->isRRMSupported);

		if (isset(sta->rm_cap.cap, DOT11_RRM_CAP_LINK)) {
			ptr_conn->rrmCaps.linkMeas = 1;
		}

		if (isset(sta->rm_cap.cap, DOT11_RRM_CAP_NEIGHBOR_REPORT)) {
			ptr_conn->rrmCaps.neighRpt = 1;
		}

		if (isset(sta->rm_cap.cap, DOT11_RRM_CAP_BCN_PASSIVE)) {
			ptr_conn->rrmCaps.bcnRptPassive = 1;
		}

		if (isset(sta->rm_cap.cap, DOT11_RRM_CAP_BCN_ACTIVE)) {
			ptr_conn->rrmCaps.bcnRptActive = 1;
		}

		if (isset(sta->rm_cap.cap, DOT11_RRM_CAP_BCN_TABLE)) {
			ptr_conn->rrmCaps.bcnRptTable = 1;
		}

		if (isset(sta->rm_cap.cap, DOT11_RRM_CAP_LCIM)) {
			ptr_conn->rrmCaps.lciMeas = 1;
		}

		if (isset(sta->rm_cap.cap, DOT11_RRM_CAP_FTM_RANGE)) {
			ptr_conn->rrmCaps.ftmRangeRpt = 1;
		}

		ECBD_PRINT_INFO("%s rm_cap = %02x %02x %02x %02x %02x \n", __FUNCTION__,
			sta->rm_cap.cap[0], sta->rm_cap.cap[1],
			sta->rm_cap.cap[2], sta->rm_cap.cap[3],
			sta->rm_cap.cap[4]);

		ECBD_PRINT_INFO("rrmCaps: lm=%d nr=%d bp=%d ba=%d bt=%d lci=%d ftm=%d\n",
			ptr_conn->rrmCaps.linkMeas,
			ptr_conn->rrmCaps.neighRpt,
			ptr_conn->rrmCaps.bcnRptPassive,
			ptr_conn->rrmCaps.bcnRptActive,
			ptr_conn->rrmCaps.bcnRptTable,
			ptr_conn->rrmCaps.lciMeas,
			ptr_conn->rrmCaps.ftmRangeRpt);

		/* add bandCap2G & bandCap5G */
		if (sta->band & WLC_BAND_2G)
			ptr_conn->bandCap2G = 1;

		if (sta->band & WLC_BAND_5G)
			ptr_conn->bandCap5G = 1;

		ECBD_PRINT_INFO("band=%d bandCap2G=%d bandCap5G=%d\n",
			sta->band, ptr_conn->bandCap2G, ptr_conn->bandCap5G);

		ret = ECBD_SUCCESS;
	}

	return ret;
}

/* for steering connect/disconnect
 * event_type:
 *   WIFI_STEERING_EVENT_CLIENT_CONNECT
 *   WIFI_STEERING_EVENT_CLIENT_DISCONNECT
 * other parameters are for disconnect event
 */
static void
ecbd_send_steering_conn_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta, int event_type, int reason, int source, int type)
{
	int len;
	char buf[MAX_EVENT_BUFFER_LEN], *bufptr = buf;
	wifi_steering_event_t *steering_evt;

	ECBD_PRINT_INFO("send_steering_connect_event: g_idx=%d ap_idx=%d, Client-MAC="MACF" event_type=%d (%s)\n",
		g_idx, ap_idx, ETHER_TO_MACF(sta->addr), event_type,
		(event_type == WIFI_STEERING_EVENT_CLIENT_CONNECT)?"connect":"disconnect");

	*(uint*)bufptr = g_idx;
	steering_evt = (wifi_steering_event_t *)(bufptr + sizeof(uint));
	steering_evt->type = event_type;
	steering_evt->apIndex = ap_idx;
	steering_evt->timestamp_ms = time(NULL);

	/* parameter */
	if (event_type == WIFI_STEERING_EVENT_CLIENT_CONNECT) {
		memset(&(steering_evt->data.connect), 0, sizeof(wifi_steering_evConnect_t));
		memcpy(steering_evt->data.connect.client_mac, sta->addr.octet, sizeof(mac_address_t));

		/* update band cap info for this sta again when connect */
		ecbd_update_sta_band_info(sta);

		if (ecbd_get_sta_conn_data(sta, &(steering_evt->data.connect)) != ECBD_SUCCESS) {
			ECBD_PRINT_ERROR("%s: fail to get sta data_info\n", __FUNCTION__);
			steering_evt->data.connect.datarateInfo.phyMode = PHY_TYPE_NULL; /* 0xf */
		}
	}
	else if (event_type == WIFI_STEERING_EVENT_CLIENT_DISCONNECT) {
		memcpy(steering_evt->data.disconnect.client_mac, sta->addr.octet, sizeof(mac_address_t));
		steering_evt->data.disconnect.reason = reason;
		steering_evt->data.disconnect.source = source;
		steering_evt->data.disconnect.type = type;
		ECBD_PRINT_INFO("disconnect_event: reason=%d source=%d type=%d\n", reason, source, type);
	}
	else {
		ECBD_PRINT_ERROR("wrong connect/disconnect event type!\n");
		return;
	}

	len = sizeof(uint) + sizeof(wifi_steering_event_t);
#ifndef RDKB_WLDM
	ecbd_notify_wifi_hal(bufptr, len, WIFI_HAL_CB_MESH_APCFG_UDP_PORT);
#else
	ecbd_notify_wifi_hal_subscribers(bufptr, len, WIFI_HAL_CB_MESH);
#endif
}

static void
ecbd_send_probereq_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta, int rssi, BOOL bcast)
{
	int len;
	char buf[MAX_EVENT_BUFFER_LEN], *bufptr = buf;
	wifi_steering_event_t *steering_evt;

	ECBD_PRINT_DEBUG("send_probereq_event: g_idx=%d ap_idx=%d, Client-MAC="MACF"\n",
		g_idx, ap_idx, ETHER_TO_MACF(sta->addr));

	*(uint*)bufptr = g_idx;
	steering_evt = (wifi_steering_event_t *)(bufptr + sizeof(uint));
	steering_evt->type = WIFI_STEERING_EVENT_PROBE_REQ;
	steering_evt->apIndex = ap_idx;
	steering_evt->timestamp_ms = time(NULL);

	/* parameter */
	memcpy(steering_evt->data.probeReq.client_mac, sta->addr.octet, sizeof(mac_address_t));
	steering_evt->data.probeReq.rssi = rssi;
	steering_evt->data.probeReq.broadcast = bcast;
	steering_evt->data.probeReq.blocked	= ecbd_sta_in_blocked(sta);

	len = sizeof(uint) + sizeof(wifi_steering_event_t);
#ifndef RDKB_WLDM
	ecbd_notify_wifi_hal(bufptr, len, WIFI_HAL_CB_MESH_APCFG_UDP_PORT);
#else
	ecbd_notify_wifi_hal_subscribers(bufptr, len, WIFI_HAL_CB_MESH);
#endif

}

static void
ecbd_send_active_state_event(uint g_idx, int ap_idx, mac_address_t cli_mac, BOOL active)
{
	int len;
	char buf[MAX_EVENT_BUFFER_LEN], *bufptr = buf;
	wifi_steering_event_t *steering_evt;

	ECBD_PRINT_INFO("send_active_state_event: g_idx=%d ap_idx=%d, Client-MAC="ECBD_MACF"\n",
		g_idx, ap_idx, MAC_TO_MACF(cli_mac));

	*(uint*)bufptr = g_idx;
	steering_evt = (wifi_steering_event_t *)(bufptr + sizeof(uint));
	steering_evt->type = WIFI_STEERING_EVENT_CLIENT_ACTIVITY;
	steering_evt->apIndex = ap_idx;
	steering_evt->timestamp_ms = time(NULL);

	/* parameter */
	memcpy(steering_evt->data.activity.client_mac, cli_mac, sizeof(mac_address_t));
	steering_evt->data.activity.active = active;

	len = sizeof(uint) + sizeof(wifi_steering_event_t);
#ifndef RDKB_WLDM
	ecbd_notify_wifi_hal(bufptr, len, WIFI_HAL_CB_MESH_APCFG_UDP_PORT);
#else
	ecbd_notify_wifi_hal_subscribers(bufptr, len, WIFI_HAL_CB_MESH);
#endif
}

static void
ecbd_send_rssi_measure_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta)
{
	char buf[MAX_EVENT_BUFFER_LEN], *bufptr = buf;
	wifi_steering_event_t *steering_evt;
	int len;

	steering_evt = (wifi_steering_event_t *)(bufptr + sizeof(uint));
	*(uint*)bufptr = g_idx;
	steering_evt->apIndex = ap_idx;
	steering_evt->timestamp_ms = time(NULL);

	/* parameter */
	steering_evt->type = WIFI_STEERING_EVENT_RSSI;
	memcpy(steering_evt->data.rssi.client_mac, sta->addr.octet, sizeof(mac_address_t));
	steering_evt->data.rssi.rssi = sta->rssi;

	len = sizeof(uint) + sizeof(wifi_steering_event_t);
#ifndef RDKB_WLDM
	ecbd_notify_wifi_hal(bufptr, len, WIFI_HAL_CB_MESH_APCFG_UDP_PORT);
#else
	ecbd_notify_wifi_hal_subscribers(bufptr, len, WIFI_HAL_CB_MESH);
#endif
}

static void
ecbd_send_rssi_xing_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta, int rssi, int rssi_type)
{
	wifi_steering_clientConfig_t *cli_cfg;
	uint high = 0, low = 0;
	int len;
	char buf[MAX_EVENT_BUFFER_LEN], *bufptr = buf;
	wifi_steering_event_t *steering_evt;
	int rssi_xing = 0;

	if (sta == NULL) {
		ECBD_PRINT_ERROR("Err: sta is NULL\n");
		return;
	}

	if ((cli_cfg = sta->cli_cfg) == NULL) {
		ECBD_PRINT_ERROR("Err: client is not configured for STA:"MACF"\n",
			ETHER_TO_MACF(sta->addr));
		return;
	}

	switch (rssi_type) {
		case STA_RSSI_PROBE:
			high = cli_cfg->rssiProbeHWM;
			low = cli_cfg->rssiProbeLWM;
			break;
		case STA_RSSI_AUTH:
			high = cli_cfg->rssiAuthHWM;
			low = cli_cfg->rssiAuthLWM;
			break;
		case STA_RSSI_INACTIVE:
			break;
		case STA_RSSI_XING:
			high = cli_cfg->rssiHighXing;
			low = cli_cfg->rssiLowXing;
			break;

		default:
			ECBD_PRINT_ERROR("Err: wrong rssi type %d\n", rssi_type);
			return;
	}

	ECBD_PRINT_INFO("Check RSSI for "MACF" rssi=%d high=%d low=%d type=%d\n",
		ETHER_TO_MACF(sta->addr), rssi, high, low, rssi_type);

	steering_evt = (wifi_steering_event_t *)(bufptr + sizeof(uint));

	/* compare rssi and send event */
	if (rssi_type == STA_RSSI_INACTIVE) {
		rssi_xing = 1;
		steering_evt->data.rssiXing.inactveXing = WIFI_STEERING_RSSI_HIGHER;
		steering_evt->data.rssiXing.highXing = WIFI_STEERING_RSSI_UNCHANGED;
		steering_evt->data.rssiXing.lowXing = WIFI_STEERING_RSSI_UNCHANGED;
	}
	else {
		steering_evt->data.rssiXing.inactveXing = WIFI_STEERING_RSSI_UNCHANGED;
		if (rssi > high) {
			steering_evt->data.rssiXing.highXing = WIFI_STEERING_RSSI_HIGHER;
			rssi_xing = 1;
		}
		else
			steering_evt->data.rssiXing.highXing = WIFI_STEERING_RSSI_UNCHANGED;

		if (rssi < low) {
			steering_evt->data.rssiXing.lowXing = WIFI_STEERING_RSSI_LOWER;
			rssi_xing = 1;
		}
		else
			steering_evt->data.rssiXing.lowXing = WIFI_STEERING_RSSI_UNCHANGED;
	}

	ECBD_PRINT_DEBUG("send_rssi_xing_event: g_idx=%d ap_idx=%d, Client-MAC="MACF" rssi_xing=%d\n",
		g_idx, ap_idx, ETHER_TO_MACF(sta->addr), rssi_xing);

	if (rssi_xing) {
		*(uint*)bufptr = g_idx;

		steering_evt->apIndex = ap_idx;
		steering_evt->timestamp_ms = time(NULL);

		/* parameter */
		memcpy(steering_evt->data.rssiXing.client_mac, sta->addr.octet, sizeof(mac_address_t));

		steering_evt->type = WIFI_STEERING_EVENT_RSSI_XING;
		steering_evt->data.rssiXing.rssi = rssi;

		len = sizeof(uint) + sizeof(wifi_steering_event_t);
#ifndef RDKB_WLDM
		ecbd_notify_wifi_hal(bufptr, len, WIFI_HAL_CB_MESH_APCFG_UDP_PORT);
#else
		ecbd_notify_wifi_hal_subscribers(bufptr, len, WIFI_HAL_CB_MESH);
#endif
	}
}

static void
ecbd_send_steering_authfail_event(uint g_idx, int ap_idx, ecbd_stalist_t *sta, int rssi, int reason, BOOL rejected)
{
	int len;
	char buf[MAX_EVENT_BUFFER_LEN], *bufptr = buf;
	wifi_steering_event_t *steering_evt;
	BOOL blocked = ecbd_sta_in_blocked(sta);

	ECBD_PRINT_INFO("send_steering_authfail_event: g_idx=%d ap_idx=%d "
		"Client-MAC="MACF" rssi=%d reason=%d blocked=%d rejected=%d\n",
		g_idx, ap_idx, ETHER_TO_MACF(sta->addr), rssi, reason, blocked, rejected);

	*(uint*)bufptr = g_idx;
	steering_evt = (wifi_steering_event_t *)(bufptr + sizeof(uint));
	steering_evt->type = WIFI_STEERING_EVENT_AUTH_FAIL;
	steering_evt->apIndex = ap_idx;
	steering_evt->timestamp_ms = time(NULL);

	/* parameter */
	memcpy(steering_evt->data.authFail.client_mac, sta->addr.octet, sizeof(mac_address_t));
	steering_evt->data.authFail.rssi = rssi;
	steering_evt->data.authFail.reason = reason;
	steering_evt->data.authFail.bsBlocked = blocked;
	steering_evt->data.authFail.bsRejected = rejected;

	len = sizeof(uint) + sizeof(wifi_steering_event_t);
#ifndef RDKB_WLDM
	ecbd_notify_wifi_hal(bufptr, len, WIFI_HAL_CB_MESH_APCFG_UDP_PORT);
#else
	ecbd_notify_wifi_hal_subscribers(bufptr, len, WIFI_HAL_CB_MESH);
#endif
}

/* per Comcast and Plume, all rssi here should be converted to snr */
static int
ecbd_get_noise(char *ifname, int *noise)
{
	char ioctl_buf[MAX_IOCTL_BUFLEN];
	wl_chanim_stats_t *list;
	chanim_stats_t *stats;
	wl_chanim_stats_t param;
	char *ptr;
	int ret, tlen;

	list = (wl_chanim_stats_t *) ioctl_buf;

	memset(&param, 0, sizeof(param));
	param.buflen = WLC_IOCTL_MAXLEN;
	param.count = 1;
	param.version = WL_CHANIM_STATS_VERSION;

	memset(ioctl_buf, 0, sizeof(ioctl_buf));
	strcpy(ioctl_buf, "chanim_stats");
	tlen = strlen(ioctl_buf) + 1;
	ptr = (char *)(ioctl_buf + tlen);
	memcpy(ptr, &param, sizeof(wl_chanim_stats_t));

	ret = wl_ioctl(ifname, WLC_GET_VAR, ioctl_buf, sizeof(ioctl_buf));

	if (ret < 0) {
		ECBD_PRINT_ERROR("Err: intf:%s chanim_stats for noise\n", ifname);
		return ECBD_FAIL;
	}

	stats = list->stats;
	*noise = stats->bgnoise;

	return ECBD_SUCCESS;
}

/* this one return real rssi */
static int
_ecbd_get_sta_rssi(char *ifname, struct ether_addr *addr, int *rssi)
{
	int err;
	scb_val_t scb_val;

	if (!rssi) {
		ECBD_PRINT_ERROR("Err: use NULL pointer to get rssi\n");
		return ECBD_FAIL;
	}

	memcpy(&scb_val.ea, addr, ETHER_ADDR_LEN);
	err = wl_ioctl(ifname, WLC_GET_RSSI, &scb_val, sizeof(scb_val));
	if (err) {
		ECBD_PRINT_ERROR("Err: intf:%s STA:"MACF" rssi\n",
			ifname, ETHER_TO_MACF(*addr));
		return ECBD_FAIL;
	}

	*rssi = scb_val.val; /* convert to snr */
	ECBD_PRINT_INFO("Got: rssi=%d for STA "MACF" onintf:%s\n", *rssi, ETHER_TO_MACF(*addr), ifname);

	return ECBD_SUCCESS;
}

/* the rssi means snr here */
static int
ecbd_get_sta_rssi(char *ifname, struct ether_addr *addr, int *rssi)
{
	int tmp_rssi = 0, noise = 0;

	if (_ecbd_get_sta_rssi(ifname, addr, &tmp_rssi) != ECBD_SUCCESS)
		return ECBD_FAIL;

	if (ecbd_get_noise(ifname, &noise) == ECBD_FAIL) {
		ECBD_PRINT_ERROR("Err: intf:%s noise\n", ifname);
		return ECBD_FAIL;
	}

	*rssi = (tmp_rssi > noise)?(tmp_rssi - noise) : 0; /* convert to snr */
	ECBD_PRINT_INFO("Got: rssi=%d noise=%d snr=%d for STA "MACF" onintf:%s\n",
		tmp_rssi, noise, *rssi, ETHER_TO_MACF(*addr), ifname);

	return ECBD_SUCCESS;
}

/* check inactive and RSSI, if hit threshold, send event to wifi_hal */
static void
ecbd_check_sta_status(int tick)
{
	uint g_idx;
	wifi_steering_apConfig_t *ap_cfg = NULL;
	ecbd_stalist_t *ptr;
	char buf[32];
	time_t now;
	wifi_steering_clientConfig_t *cli_cfg;

	ECBD_PRINT_DEBUG("\n === check inactive and RSSI === \n");

	ptr = ecbd_stalist_hdr;
	while (ptr) {
		if (!(ptr->type & STA_TYPE_ASSOC) || (ptr->state == INACTIVE)) {
			ptr = ptr->next;
			continue;
		}

		ECBD_PRINT_DEBUG("MAC=%s on ssid %s idx=%d bssidx=%d\n",
			ether_etoa(ptr->addr.octet, buf), ptr->ssid, ptr->ifidx, ptr->bsscfgidx);

		/* inactive */
		ap_cfg = ecbd_find_ap_cfg(ptr->ifidx, ptr->bsscfgidx, &g_idx);
		if (ap_cfg == NULL) {
			ECBD_PRINT_DEBUG("AP Config for ifidx=%d bsscfgidx=%d is not found\n",
				ptr->ifidx, ptr->bsscfgidx);
		}
		else {
			if (ap_cfg->inactCheckIntervalSec && ((tick % ap_cfg->inactCheckIntervalSec) == 0)) {
				now = time(NULL);
				if ((now - ptr->active) > ap_cfg->inactCheckThresholdSec) {
					/* inactive */
					if (ptr->inactive_state == 0) {
						/* send event from active to inactive */
						ECBD_PRINT_INFO("MAC=%s from active to inactive\n",
							ether_etoa(ptr->addr.octet, buf));
						ecbd_send_active_state_event(g_idx, ap_cfg->apIndex, ptr->addr.octet, FALSE);
						ptr->inactive_state = 1;
					}

					/* check rssi for inactive STA */
					if ((cli_cfg = ptr->cli_cfg) == NULL) {
						ECBD_PRINT_INFO("Client Config for %s is not defined\n",
							ether_etoa(ptr->addr.octet, buf));
					} else {
						if (ptr->rssi > cli_cfg->rssiInactXing) {
							ECBD_PRINT_INFO("rssiInactXing mark for %s: %d, %d\n",
								ether_etoa(ptr->addr.octet, buf), ptr->rssi,
								cli_cfg->rssiInactXing);

							ecbd_send_rssi_xing_event(g_idx, ap_cfg->apIndex, ptr, ptr->rssi,
								STA_RSSI_INACTIVE);
						}
					}

				} else {
					/* active */
					if (ptr->inactive_state) {
						/* send event from inactive to active */
						ECBD_PRINT_INFO("MAC=%s from inactive to active\n",
							ether_etoa(ptr->addr.octet, buf));
						ecbd_send_active_state_event(g_idx, ap_cfg->apIndex, ptr->addr.octet, TRUE);
						ptr->inactive_state = 0;
					}
				}
			}

			/* check RSSI xing for associated STA */
			if ((cli_cfg = ptr->cli_cfg) != NULL) {
				int rssi;
				int32 rssi_change_assoc = WIFI_STEERING_RSSI_UNCHANGED;

				if (ecbd_get_sta_rssi(ptr->ifname, &(ptr->addr), &rssi) == ECBD_SUCCESS) {
					ptr->rssi = rssi;
					ECBD_PRINT_DEBUG("Update snr %d for MAC=%s\n",
						rssi, ether_etoa(ptr->addr.octet, buf));
				}
				else {
					ECBD_PRINT_ERROR("Fail to read rssi for MAC=%s when update\n",
						ether_etoa(ptr->addr.octet, buf));
				}

				ECBD_PRINT_DEBUG("Check RSSI WM for associated MAC=%s snr=%d hXing=%d, lXing=%d\n",
					ether_etoa(ptr->addr.octet, buf), ptr->rssi,
					cli_cfg->rssiHighXing, cli_cfg->rssiLowXing);

				if (cli_cfg->rssiHighXing || cli_cfg->rssiLowXing) {
					/* use status change to reduce crossing event flooding for same STA */
					if (ptr->rssi > cli_cfg->rssiHighXing)
						rssi_change_assoc = WIFI_STEERING_RSSI_HIGHER;
					else if (ptr->rssi < cli_cfg->rssiLowXing)
						rssi_change_assoc = WIFI_STEERING_RSSI_LOWER;

					if (ptr->rssi_change_assoc != rssi_change_assoc) {
						ptr->rssi_change_assoc = rssi_change_assoc;
						ecbd_send_rssi_xing_event(g_idx, ap_cfg->apIndex, ptr, ptr->rssi, STA_RSSI_XING);
					}
				}
			}
		}

		ptr = ptr->next;
	}
}

static int
ecbd_disconnect_sta(char *ifname, struct ether_addr *addr, int type, int reason)
{
	scb_val_t scb_val;
	int ret = 0;

	ECBD_PRINT_INFO("Disconnect STA "MACF" on intf:%s type=%d, reason=%d\n",
		ETHER_TO_MACF(*addr), ifname, type, reason);

	if (type == DISCONNECT_TYPE_DISASSOC) {
		ECBD_PRINT_INFO("Disassoc on intf:%s STA:"MACF"\n",
			ifname, ETHER_TO_MACF(*addr));
		ret = wl_ioctl(ifname, WLC_DISASSOC, NULL, 0);
	} else if (type == DISCONNECT_TYPE_DEAUTH) {

		scb_val.val = reason;
		memcpy(&scb_val.ea, addr, ETHER_ADDR_LEN);
		ret = wl_ioctl(ifname, WLC_SCB_DEAUTHENTICATE_FOR_REASON,
			&scb_val, sizeof(scb_val));

		if (ret < 0) {
			ECBD_PRINT_ERROR("Fail to deauth STA:"MACF" on intf:%s \n",
				ETHER_TO_MACF(*addr), ifname);
		} else {
			ECBD_PRINT_INFO("Succ to deauth STA:"MACF" on intf:%s \n",
				ETHER_TO_MACF(*addr), ifname);
		}
	} else {
		ECBD_PRINT_INFO("Disconnect STA "MACF" on intf:%s unknown type=%d\n",
			ETHER_TO_MACF(*addr), ifname, type);
	}
	return ret;
}

/* for wifi_app log */
/* output cli "bs_data" to a tmp file then parse */
/* sample output
wl -i wl1 bs_data > /tmp/ecbd_sta_bs_data.tmp
wl -i wl1.1 bs_data >> /tmp/ecbd_sta_bs_data.tmp
wl: wl driver adapter not found

cat /tmp/ecbd_sta_bs_data.tmp
  Station Address   PHY Mbps  Data Mbps    Air Use   Data Use    Retries
E4:D5:3D:95:85:98        0.0        0.0       0.0%       0.0%       0.0%
F0:99:BF:76:FF:54        0.0        0.0       0.0%       0.0%       0.0%
*/

/*
	log_type:
	0 (STA_LOG_REGULAR): all STAs by regular interval (repeated log)
	1 (STA_LOG_INITIAL): 1st time log after associated
*/

static void
ecbd_wifi_app_log_sta(int log_type)
{
	ecbd_stalist_t *ptr;
	int rssi;
	char buf_chanspec[20];
	char cmd[128];
	int i, j;
	int need_log = 0;
	int count = 0;
	int title = 0;
	FILE *fp;
	char mac_str[20];
	char line[128];
	float data_rate = 0;
	float phy_rate = 0;
	float use = 0, air = 0, rtr = 0;

	ECBD_PRINT_INFO("%s: log_type=%d\n",  __FUNCTION__, log_type);

	/* preview */
	ptr = ecbd_stalist_hdr;
	while (ptr) {
		if (!(ptr->type & STA_TYPE_ASSOC) || (ptr->state == INACTIVE)) {
			ptr = ptr->next;
			continue;
		}

		/* real_rssi == 0 means the STA is just associated, not log created */
		if (ptr->real_rssi == 0) {
			need_log = 1;
			break;
		}
		count++;
		ptr = ptr->next;
	}

	if ((log_type == STA_LOG_REGULAR) && count) {
		need_log = 1;
	}

	if (!need_log) {
		ECBD_PRINT_INFO("%s: no log necessary\n",  __FUNCTION__);
		return;
	}

	/* output bs_data for all intrfaces to tmp file */
	unlink(WLAN_APPS_LOG_ECBD_BS_DATA_FILE);
	for (i = 0; i < HAL_RADIO_NUM_RADIOS; i++) {
		for (j = 0; j < WLC_MAXBSSCFG; j++) {
			if (j == 0) {
				/* output chanim_stats to logfile directly at the same time */
				/* title first */
				snprintf(log_buf, sizeof(log_buf), "chanim_stats on wl%d", i);
				WLAN_APPS_LOGS("ECBD", log_buf);

				/* output chanim_stats to logfile (physical interface only) */
				snprintf(cmd, sizeof(cmd), "wl -i wl%d chanim_stats >> %s", i, log_nvram_buf?log_nvram_buf:WLAN_APPS_LOG_FILE);
				system(cmd);

				/* for bs_data (phyrate etc) */
				snprintf(cmd, sizeof(cmd), "wl -i wl%d bs_data >> %s", i, WLAN_APPS_LOG_ECBD_BS_DATA_FILE);
			}
			else
				snprintf(cmd, sizeof(cmd), "wl -i wl%d.%d bs_data >> %s", i, j, WLAN_APPS_LOG_ECBD_BS_DATA_FILE);
			system(cmd);
			ECBD_PRINT_INFO("%s: cmd=<%s>\n",  __FUNCTION__, cmd);
		}
	}

	if ((fp = fopen(WLAN_APPS_LOG_ECBD_BS_DATA_FILE, "r")) == NULL) {
		ECBD_PRINT_ERROR("Err: Open sta bs_data file %s\n", WLAN_APPS_LOG_ECBD_BS_DATA_FILE);
		return;
	}

	ptr = ecbd_stalist_hdr;

	while (ptr) {
		if (!(ptr->type & STA_TYPE_ASSOC) || (ptr->state == INACTIVE)) {
			ptr = ptr->next;
			continue;
		}

		/* real_rssi == 0 means the STA is just associated, not log created */
		if ((log_type == STA_LOG_INITIAL) && ptr->real_rssi) {
			ptr = ptr->next;
			continue;
		}

		/* get phyrate etc, use bs_data file */
		fseek(fp, 0, SEEK_SET);
		ether_etoa(ptr->addr.octet, mac_str);
		while (fgets(line, sizeof(line), fp)) {
			ECBD_PRINT_INFO("line=<%s>, MAC=%s\n", line, mac_str);
			if (strstr(line, mac_str)) {
				sscanf(line, "%s %f %f %f%% %f%% %f%%\n",
					mac_str, &phy_rate, &data_rate, &air, &use, &rtr);
				ECBD_PRINT_INFO("Get phyrate etc. %s %10.1f %10.1f %9.1f%% %9.1f%% %9.1f%%\n",
					mac_str, phy_rate, data_rate, air, use, rtr);
				ptr->phyrate = (int)phy_rate;
				break;
			}
		}

		/* get rssi */
		if (_ecbd_get_sta_rssi(ptr->ifname, &(ptr->addr), &rssi) == ECBD_SUCCESS) {
			ptr->real_rssi = rssi;
		}

		/* channel buf */
		wf_chspec_ntoa(ptr->chanspec, buf_chanspec);

		/* write log to file */
		if (!title) {
			snprintf(log_buf, sizeof(log_buf), "<STA MAC> <Phyrate> <RSSI> <Channel> <Aid> <Tx bytes> <Rx bytes> <Datarate> <Air Use> <Data Use> <Retries>");
			WLAN_APPS_LOGS("ECBD", log_buf);
			title = 1;
		}

		snprintf(log_buf, sizeof(log_buf), "%s %10.1f %d %s %d %d %d %10.1f %9.1f%% %9.1f%% %9.1f%%",
			mac_str, phy_rate, ptr->real_rssi, buf_chanspec, WL_STA_AID(ptr->aid),
			ptr->tx_tot_pkts, ptr->rx_tot_pkts, data_rate, air, use, rtr);
		WLAN_APPS_LOGS("ECBD", log_buf);

		ptr = ptr->next;
	}
	if (fp)
		fclose(fp);

	ECBD_PRINT_INFO("%s: end\n",  __FUNCTION__);
}

static int
ecbd_sta_in_assoclist(ecbd_stalist_t *sta_ptr)
{
	struct maclist *assoclist;
	int rc, cnt, ret = ECBD_FAIL;

	if (sta_ptr == NULL) {
		ECBD_PRINT_ERROR("%s@%d sta_ptr is NULL", __FUNCTION__, __LINE__);
		return ret;
	}

	assoclist = malloc(WLC_IOCTL_MEDLEN);
	if (!assoclist) {
		ECBD_PRINT_ERROR("%s: malloc failed, %s\n", __FUNCTION__, strerror(errno));
		return ret;
	}

	assoclist->count = htod32((WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN);
	rc = wl_ioctl(sta_ptr->ifname, WLC_GET_ASSOCLIST, assoclist, WLC_IOCTL_MEDLEN);
	if (rc < 0) {
		ECBD_PRINT_ERROR("%s: fail to get assoclist on %s, error: %s\n",
			__FUNCTION__, sta_ptr->ifname, strerror(errno));
		free(assoclist);
		return ret;
	}

	ECBD_PRINT_INFO("%s: assoclist count = %d on %s, check "MACF"\n",
		__FUNCTION__, assoclist->count, sta_ptr->ifname, ETHER_TO_MACF(sta_ptr->addr));

	for (cnt = 0; cnt < assoclist->count; cnt++) {
		ECBD_PRINT_INFO("cnt[%d] mac:"MACF"\n", cnt, ETHER_TO_MACF(assoclist->ea[cnt]));
		if (eacmp(&(assoclist->ea[cnt]), &(sta_ptr->addr)) == 0) {
			ECBD_PRINT_INFO("mac:"MACF" in assoclist\n", ETHER_TO_MACF(sta_ptr->addr));
			ret = ECBD_SUCCESS;
			break;
		}
	}
	free(assoclist);

	return ret;
}

/* For ApAssociatedDevicesHighWatermarkThreshold */
static int ecbd_get_assoc_dev_count(char *osifname)
{
	struct maclist *assoclist;
	char *ioctl_buf;
	int count = 0;

	assoclist = malloc(WLC_IOCTL_MEDLEN);
	if (!assoclist) {
		ECBD_PRINT_ERROR("malloc failed, %s\n", strerror(errno));
		return -1;
	}

	if (ecbd_is_open_security(osifname)) {
		/* open mode read assoclist */
		assoclist->count = htod32((WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN);
		if (wl_ioctl(osifname, WLC_GET_ASSOCLIST, assoclist, WLC_IOCTL_MEDLEN) < 0) {
			ECBD_PRINT_ERROR("Fail to get assoclist, %s\n", strerror(errno));
			free(assoclist);
			return -1;
		}
	}
	else {
		/* Security mode read autho_sta_list */
		ioctl_buf = (char *)assoclist;
		strncpy(ioctl_buf, "autho_sta_list", WLC_IOCTL_MEDLEN - 1);
		ioctl_buf[WLC_IOCTL_MEDLEN - 1] = '\0';
		if (wl_ioctl(osifname, WLC_GET_VAR, ioctl_buf, WLC_IOCTL_MEDLEN) < 0) {
			ECBD_PRINT_ERROR("Fail to get autho_sta_list, %s\n", strerror(errno));
			free(assoclist);
			return -1;
		}
	}

	count = assoclist->count;
	free(assoclist);

	ECBD_PRINT_INFO("assoclist count=%d on %s\n", count, osifname);

	return count;
}

static int
ecbd_get_max_assoc(char *nvifname, int *max_assoc)
{
	char nvram_name[64], ifname[BCM_MSG_IFNAME_MAX], *s;
	int value;

	if (nvifname == NULL || max_assoc == NULL)
		return -1;

	snprintf(nvram_name, sizeof(nvram_name), "%s_bss_maxassoc", nvifname);

	value = atoi(nvram_safe_get(nvram_name));
	if (value <= 0) {
		snprintf(ifname, sizeof(ifname), "%s", nvifname);
		if ((s = strchr(ifname, '.')) != NULL)
			*s = '\0'; /* change to radio interface name */

		snprintf(nvram_name, sizeof(nvram_name), "%s_cfg_maxassoc", ifname);
		value = atoi(nvram_safe_get(nvram_name));
	}
	ECBD_PRINT_INFO("Read maxassoc %d from %s\n", value, nvram_name);
	*max_assoc = value;
	return 0;
}

static int
ecbd_get_assoc_dev_hwm_th(char *nvifname)
{
	char nvram_name[64], *nvram_value;
	int max_assoc = 0, value_th;

	if (nvifname == NULL)
		return 0; /* no calculate */

	if (ecbd_get_max_assoc(nvifname, &max_assoc) != 0) {
		/* display error only, continue as value = 0 */
		ECBD_PRINT_ERROR("Fail to get maxassoc from %s\n", nvifname);
	}

	snprintf(nvram_name, sizeof(nvram_name), "%s_%s",
		nvifname, NVRAM_ASSOC_DEV_HWM_TH);

	if ((nvram_value = nvram_get(nvram_name)) == NULL) {
		/* default */
		value_th = max_assoc ? max_assoc : DEFAULT_ASSOC_DEV_HWM_TH;
	}
	else {
		/* config "0" means no Watermark calculation algorithm */
		value_th = atoi(nvram_value);

		/* adjust based on max_assoc */
		if (value_th > max_assoc)
			value_th = max_assoc;
	}

	ECBD_PRINT_INFO("max_assoc=%d value_th=%d for %s\n",
		max_assoc, value_th, nvifname);

	return value_th;
}

/* update counter per BSS
	NVRAM_ASSOC_DEV_HWM_TH_REACHED
	NVRAM_ASSOC_DEV_HWM_MAX
	NVRAM_ASSOC_DEV_HWM_MAX_DATE
	assoc_cnt=-1: read assoclist again
	reset=1: clear the old value (do once when init)
 */
static void ecbd_update_assoc_dev_hwm_cnt(char *nvifname, int assoc_cnt, int reset)
{
	char osifname[BCM_MSG_IFNAME_MAX], nvram_name[64], *nvram_value, new_value[64];
	int assoc_dev_hwm_th, assoc_dev_hwm_max, reached_cnt, write_nvram;
	time_t timestamp;

	if (!nvifname) {
		ECBD_PRINT_ERROR("interfacce name is null\n");
		return;
	}

	if (nvifname_to_osifname(nvifname, osifname,
		sizeof(osifname)) != 0) {
		ECBD_PRINT_ERROR("Fail to convert %s to osifname\n", nvifname);
		return;
	}

	if ((assoc_dev_hwm_th = ecbd_get_assoc_dev_hwm_th(nvifname)) == 0) {
		/* threshold is 0, do nothing except reset */
		if (reset) {
			snprintf(nvram_name, sizeof(nvram_name), "%s_%s",
				nvifname, NVRAM_ASSOC_DEV_HWM_TH_REACHED);
			if (nvram_get(nvram_name) != NULL) {
				nvram_set(nvram_name, "0");
			}
			snprintf(nvram_name, sizeof(nvram_name), "%s_%s",
				nvifname, NVRAM_ASSOC_DEV_HWM_MAX);
			if (nvram_get(nvram_name) != NULL) {
				nvram_set(nvram_name, "0");
			}
			snprintf(nvram_name, sizeof(nvram_name), "%s_%s",
				nvifname, NVRAM_ASSOC_DEV_HWM_MAX_DATE);
			if (nvram_get(nvram_name) != NULL) {
				nvram_set(nvram_name, "0");
			}
		}
		ECBD_PRINT_DEBUG("Not calculate asso_dev watermark on %s\n", nvifname);
		return;
	}

	if (assoc_cnt < 0) {
		/* read assoclist count */
		if ((assoc_cnt = ecbd_get_assoc_dev_count(osifname)) < 0) {
			ECBD_PRINT_ERROR("Fail to get assoc count on %s\n", osifname);
			return;
		}
	}

	/* 1) NVRAM_ASSOC_DEV_HWM_TH_REACHED */
	snprintf(nvram_name, sizeof(nvram_name), "%s_%s",
		nvifname, NVRAM_ASSOC_DEV_HWM_TH_REACHED);
	nvram_value = nvram_get(nvram_name);

	/* debug only */
	ECBD_PRINT_INFO("nvif=%s osif=%s assoc_cnt=%d nvram_name=%s nvram_value=%s\n",
		 nvifname, osifname, assoc_cnt, nvram_name, nvram_value ? nvram_value : "null");

	write_nvram = 0;
	reached_cnt = 0;
	if (nvram_value) {
		if (reset) {
			write_nvram = 1; /* reset when old nvram exists */
		} else {
			/* read initial value from nvram */
			reached_cnt = atoi(nvram_value);
		}
	}

	if (assoc_cnt >= assoc_dev_hwm_th) {
		reached_cnt ++;
		write_nvram = 1;
	}

	if (write_nvram) {
		snprintf(new_value, sizeof(new_value), "%d", reached_cnt);
		nvram_set(nvram_name, new_value);
		ECBD_PRINT_INFO("Write %d to %s\n", reached_cnt, nvram_name);
	}

	/* 2) NVRAM_ASSOC_DEV_HWM_MAX */
	snprintf(nvram_name, sizeof(nvram_name), "%s_%s",
		nvifname, NVRAM_ASSOC_DEV_HWM_MAX);
	nvram_value = nvram_get(nvram_name);

	write_nvram = 0;
	assoc_dev_hwm_max = 0;
	if (nvram_value) {
		if (reset) {
			write_nvram = 1; /* reset when old nvram exists */
			timestamp = 0;
		} else {
			/* read initial value from nvram */
			assoc_dev_hwm_max = atoi(nvram_value);
		}
	}

	if (assoc_cnt && (assoc_cnt >= assoc_dev_hwm_max)) {
		assoc_dev_hwm_max = assoc_cnt;
		write_nvram = 1;
		timestamp = time(NULL);
	}

	/* debug only */
	ECBD_PRINT_INFO("assoc_cnt=%d assoc_dev_hwm_max=%d nvram_name=%s nvram_value=%s\n",
		 assoc_cnt, assoc_dev_hwm_max, nvram_name, nvram_value ? nvram_value : "null");

	if (write_nvram) {
		snprintf(new_value, sizeof(new_value), "%d", assoc_dev_hwm_max);
		nvram_set(nvram_name, new_value);
		ECBD_PRINT_INFO("Write %d to %s\n", assoc_dev_hwm_max, nvram_name);

		/* 3) NVRAM_ASSOC_DEV_HWM_MAX_DATE */
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s",
			nvifname, NVRAM_ASSOC_DEV_HWM_MAX_DATE);
		snprintf(new_value, sizeof(new_value), "%lu", timestamp);
		nvram_set(nvram_name, new_value);
		ECBD_PRINT_INFO("Write %s to %s\n", new_value, nvram_name);
	}
}

/* in case disassoc/deauth event missing */
#define ECBD_CHECK_ASSOC_INTERVAL 5
#define ECBD_CHECK_ASSOC_CONN_INTV 60
#define ECBD_CHECK_ASSOC_BOOT_TIME 300
static int reset_assoc_dev_hwm = 1;
static void
ecbd_update_stalist_by_assoclist(int tick)
{
	ecbd_stalist_t *ptr = ecbd_stalist_hdr, *sta_info;
	uint32 group_idx;
	int apindex, i, j, cnt, send_event, val = 0;
	char buf[32] = {0}, nvram_name[64], *ioctl_buf;
	char nvifname[BCM_MSG_IFNAME_MAX], osifname[BCM_MSG_IFNAME_MAX];
	struct maclist *assoclist;

	/* for disconnect event missing */
	while (ptr != NULL) {
		if ((ptr->type & STA_TYPE_ASSOC) && (ptr->disassoc_time == 0) &&
			(ecbd_sta_in_assoclist(ptr) != ECBD_SUCCESS)) {

			ECBD_PRINT_INFO("MAC=%s no longer in assoclist on %s\n",
				ether_etoa(ptr->addr.octet, buf), ptr->ifname);

			/* add to log */
			snprintf(log_buf, sizeof(log_buf), "MAC=%s on %s out of sync with wl driver\n",
				ether_etoa(ptr->addr.octet, buf), ptr->ifname);
			WLAN_APPS_LOGS("ECBD", log_buf);

			ptr->assoc_time = 0;
			ptr->disassoc_time = time(NULL);
			ptr->state = INACTIVE;

			/* send mesh disconnect event */
			if (ecbd_find_ap_cfg(ptr->ifidx, ptr->bsscfgidx, &group_idx) == NULL) {
				ECBD_PRINT_WARNING("%s: apconfig not found\n", __FUNCTION__);
				group_idx = 0; /* default */
			}

			apindex = WL_DRIVER_TO_AP_IDX(ptr->ifidx, ptr->bsscfgidx) - 1; /* hal index starting from 0 */
			/* use reason DOT11_RC_AUTH_INVAL (2): Previous authentication no longer valid */
			ecbd_send_steering_conn_event(group_idx, apindex, ptr,
				WIFI_STEERING_EVENT_CLIENT_DISCONNECT, DOT11_RC_AUTH_INVAL,
				DISCONNECT_SOURCE_LOCAL, DISCONNECT_TYPE_DEAUTH);
		}
		ptr = ptr->next;
	}

	/* in case assoc/reassoc event is not sent to callback */
	/* send "connect" event to cb 5 times in the first 5 min after reboot */
	if ((tick > ECBD_CHECK_ASSOC_BOOT_TIME) ||
		(tick % ECBD_CHECK_ASSOC_CONN_INTV) != 0) {
		return;
	}

	assoclist = malloc(WLC_IOCTL_MEDLEN);
	if (!assoclist) {
		ECBD_PRINT_ERROR("malloc failed, %s\n", strerror(errno));
		return;
	}

	for (i = 0; i < HAL_RADIO_NUM_RADIOS; i++) {
		for (j = 0; j < WLC_MAXBSSCFG; j++) {
			if (j == 0)
				snprintf(nvifname, sizeof(nvifname), "wl%d", i);
			else
				snprintf(nvifname, sizeof(nvifname), "wl%d.%d", i, j);

			snprintf(nvram_name, sizeof(nvram_name), "%s_bss_enabled", nvifname);
			if (!nvram_match(nvram_name, "1")) {
				ECBD_PRINT_DEBUG("bss_enabled not set for %s\n", nvifname);
				continue;
			}

			if (nvifname_to_osifname(nvifname, osifname,
				sizeof(osifname)) != 0) {
				ECBD_PRINT_DEBUG("Fail to convert %s to osifname\n", nvifname);
				continue;
			}

			/* if interface UP */
			val = 0;
			if (wl_ioctl(osifname, WLC_GET_UP, &val, sizeof(val)) < 0 || !val) {
				ECBD_PRINT_DEBUG("%s not UP\n", osifname);
				continue;
			}

			memset(assoclist, 0, WLC_IOCTL_MEDLEN);

			if (ecbd_is_open_security(osifname)) {
				/* open mode read assoclist */
				assoclist->count = htod32((WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN);
				if (wl_ioctl(osifname, WLC_GET_ASSOCLIST, assoclist, WLC_IOCTL_MEDLEN) < 0) {
					ECBD_PRINT_ERROR("Fail to get assoclist, %s\n", strerror(errno));
					free(assoclist);
					return;
				}
			}
			else {
				/* Security mode read autho_sta_list */
				ioctl_buf = (char *)assoclist;
				strcpy(ioctl_buf, "autho_sta_list");
				if (wl_ioctl(osifname, WLC_GET_VAR, ioctl_buf, WLC_IOCTL_MEDLEN) < 0) {
					ECBD_PRINT_ERROR("Fail to get autho_sta_list, %s\n", strerror(errno));
					free(assoclist);
					return;
				}
			}

			ECBD_PRINT_INFO("assoclist count=%d on %s\n", assoclist->count, osifname);
			if (reset_assoc_dev_hwm) {
				ecbd_update_assoc_dev_hwm_cnt(nvifname, assoclist->count, 1);

				/* write to log */
				snprintf(log_buf, sizeof(log_buf), "Reset assoc_dev high watermark on %s at %d\n",
					nvifname, tick);
				WLAN_APPS_LOGS("ECBD", log_buf);
			}

			for (cnt = 0; cnt < assoclist->count; cnt++) {
				ECBD_PRINT_INFO("cnt[%d] mac:"MACF"\n", cnt, ETHER_TO_MACF(assoclist->ea[cnt]));
				send_event = 1; /* send 5 times unconditionally */
				sta_info = ecbd_retrieve_sta_info(&(assoclist->ea[cnt]), i, j);
				if (sta_info) {
					/* the sta may be a pre-configured client */
					ECBD_PRINT_INFO("Existing STA: MAC=%s assoc_time=%lu disassoc_time=%lu "
						"state=%d type=0x%x\n",
						ether_etoa(sta_info->addr.octet, buf), sta_info->assoc_time,
						sta_info->disassoc_time, sta_info->state, sta_info->type);
				}

				if (send_event) {
					/* always call ecbd_add_stalist to update info (type, assoc_time etc.) */
					sta_info = ecbd_add_stalist(NULL, &(assoclist->ea[cnt]), i,
						j, osifname, STA_TYPE_ASSOC);
					if (sta_info == NULL) {
						ECBD_PRINT_ERROR("Fail to add the sta MAC=%s to list\n",
							ether_etoa(assoclist->ea[cnt].octet, buf));
						continue;
					}

					/* send the connect event to mesh callback */
					if (ecbd_find_ap_cfg(i, j, &group_idx) == NULL) {
						ECBD_PRINT_WARNING("apconfig not found\n");
						group_idx = 0; /* default */
					}

					apindex = WL_DRIVER_TO_AP_IDX(i, j) - 1; /* hal index starting from 0 */
					ecbd_send_steering_conn_event(group_idx, apindex, sta_info,
						WIFI_STEERING_EVENT_CLIENT_CONNECT, 0, 0, 0);

					/* send to another callback ASSOC_DEV (newAp) */
					ecbd_wd_sent_msg_to_ltc(sta_info, WLC_E_ASSOC_IND);

					sta_info->event_sent |= ECBD_SENT_E_ASSOC;
					ECBD_PRINT_INFO("Watchdog send connect event MAC=%s "
						"on %s apIndex %d tick %d\n",
						ether_etoa(sta_info->addr.octet, buf), osifname, apindex, tick);

					/* write to log */
					snprintf(log_buf, sizeof(log_buf), "watchdog send connect event MAC=%s "
						"on %s apIdx %d tick %d\n",
						ether_etoa(sta_info->addr.octet, buf), osifname, apindex, tick);
					WLAN_APPS_LOGS("ECBD", log_buf);

				}
			}
		} /* bssindex j */
	} /* radioindex i */

	if (reset_assoc_dev_hwm)
		reset_assoc_dev_hwm = 0; /* do once */

	free(assoclist);
}

#ifdef RDKB_RADIO_STATS_MEASURE
static void
ecbd_update_radio_measure_param(int index, int reset)
{
	ecbd_radio_stats_t *stats_ptr;
	char nv_name[64], *str = NULL;

	stats_ptr = &ecbd_radio_stats_array[index];

	if (reset) {
		memset(stats_ptr, 0, sizeof(ecbd_radio_stats_t));
	}

	snprintf(nv_name, sizeof(nv_name), "wl%d_%s", index, NVRAM_RADIO_STATS_MEAS_RATE);
	if ((str = wlcsm_nvram_get(nv_name))) {
		stats_ptr->radio_stats_measure_rate = atoi(str);
	}
	else {
		stats_ptr->radio_stats_measure_rate = DEFAULT_RADIO_STATS_MEAS_RATE;
	}

	snprintf(nv_name, sizeof(nv_name), "wl%d_%s", index, NVRAM_RADIO_STATS_MEAS_INTEVAL);
	if ((str = wlcsm_nvram_get(nv_name))) {
		stats_ptr->radio_stats_measure_interval = atoi(str);
	}
	else {
		stats_ptr->radio_stats_measure_interval = DEFAULT_RADIO_STATS_MEAS_INTEVAL;
	}

	if (stats_ptr->radio_stats_measure_rate) {
		stats_ptr->avg_count = stats_ptr->radio_stats_measure_interval /
			stats_ptr->radio_stats_measure_rate;
	}
	else {
		stats_ptr->avg_count = DEFAULT_RADIO_STATS_MEAS_INTEVAL /
			DEFAULT_RADIO_STATS_MEAS_RATE;
	}

	ECBD_PRINT_INFO("radio %d measure_rate=%d measure_interval=%d avg_count=%d\n",
		index, stats_ptr->radio_stats_measure_rate,
		stats_ptr->radio_stats_measure_interval,
		stats_ptr->avg_count);
}

static void
ecbd_radio_stats_init(void)
{
	int i;

	for (i = 0; i < HAL_RADIO_NUM_RADIOS; i++) {
		ecbd_update_radio_measure_param(i, 1);
	}
}

static int
ecbd_calculate_average(int prev, int latest, int count)
{
	if (count <= 1) {
		return latest;
	}
	else {
		return (prev * (count - 1) + latest) / count;
	}
}

static int
ecbd_get_radio_transmit(char *ifname, uint *trans, uint *retrans)
{
	char cntbuf[MAX_IOCTL_BUFLEN];
	wl_cnt_wlc_t *wlc_cnt;

	if (wl_iovar_get(ifname, "counters", cntbuf, MAX_IOCTL_BUFLEN) < 0) {
		ECBD_PRINT_ERROR("%s: wl_iovar_get() counters failed on %s\n", __FUNCTION__, ifname);
		return -1;
	}

	if (wl_cntbuf_to_xtlv_format(NULL, cntbuf, MAX_IOCTL_BUFLEN, 0) != 0) {
		ECBD_PRINT_ERROR("%s: counter xtlv format failed!\n", __FUNCTION__);
		return -1;
	}

	if (!(wlc_cnt = (wl_cnt_wlc_t *)GET_WLCCNT_FROM_CNTBUF(cntbuf))) {
		ECBD_PRINT_ERROR("%s: counter information extraction failed!\n", __FUNCTION__);
		return -1;
	}

	*trans = wlc_cnt->txframe;
	*retrans = wlc_cnt->txretrans;

	return ECBD_SUCCESS;
}

static int
ecbd_radio_stats_calculate(int tick)
{
	int i, ret, val, ch_util, precent_retx;
	ecbd_radio_stats_data_t stats_data;
	uint tx, retx, delta_tx, delta_retx;
	ecbd_radio_stats_t *stats_ptr;
	char ifname[BCM_MSG_IFNAME_MAX], filename[128];
	FILE *fp;

	for (i = 0; i < HAL_RADIO_NUM_RADIOS; i++) {
		stats_ptr = &ecbd_radio_stats_array[i];
		snprintf(ifname, sizeof(ifname), "wl%d", i);

		val = 0;
		ret = wl_ioctl(ifname, WLC_GET_UP, &val, sizeof(val));
		if (ret < 0 || !val) {
			ECBD_PRINT_ERROR("Err: ifname[%s] is not up, val is [%d]\n",
				ifname, val);
			continue;
		}

		/* collect data and calculate per sample rate */
		if (stats_ptr->radio_stats_measure_rate &&
			(tick % stats_ptr->radio_stats_measure_rate) == 0) {

			if (ecbd_get_chanim_stats_bw_util(ifname, &ch_util, &stats_data) == ECBD_SUCCESS) {

				ECBD_PRINT_INFO("radio %d ch_util=%d runtime=%d actf=%d runtime=%d "
					"cst_exc=%d runtime=%d\n",
					i, stats_ptr->radio_ChannelUtilization, ch_util,
					stats_ptr->radio_ActivityFactor, stats_data.act_factor,
					stats_ptr->radio_CarrierSenseThreshold_Exceeded, stats_data.obss);

				stats_ptr->radio_ChannelUtilization =
					ecbd_calculate_average(stats_ptr->radio_ChannelUtilization,
					ch_util, stats_ptr->avg_count);

				stats_ptr->radio_ActivityFactor =
					ecbd_calculate_average(stats_ptr->radio_ActivityFactor,
					stats_data.act_factor, stats_ptr->avg_count);

				stats_ptr->radio_CarrierSenseThreshold_Exceeded =
					ecbd_calculate_average(stats_ptr->radio_CarrierSenseThreshold_Exceeded,
					stats_data.obss, stats_ptr->avg_count);

				ECBD_PRINT_INFO("radio %d After average ch_util=%d actf=%d cst_exc=%d\n",
					i, stats_ptr->radio_ChannelUtilization,
					stats_ptr->radio_ActivityFactor,
					stats_ptr->radio_CarrierSenseThreshold_Exceeded);
			}

			/* Percentage of packets that had to be re-transmitted */
			if (ecbd_get_radio_transmit(ifname, &tx, &retx) == ECBD_SUCCESS) {

				ECBD_PRINT_INFO("radio %d radio_tx=%d runtime=%d radio_retx=%d runtime=%d\n",
					i, stats_ptr->radio_tx, tx,
					stats_ptr->radio_retx, retx);

				delta_tx = (tx > stats_ptr->radio_tx) ? (tx - stats_ptr->radio_tx) : 0;
				delta_retx = (retx > stats_ptr->radio_retx) ? (retx - stats_ptr->radio_retx) : 0;

				stats_ptr->radio_tx = tx;
				stats_ptr->radio_retx = retx;

				if (delta_tx == 0) {
					precent_retx = delta_retx ? 100 : 0;
				} else {
					precent_retx = (delta_retx * 100) / delta_tx;
					precent_retx = (precent_retx > 100) ? 100 : precent_retx;
				}

				ECBD_PRINT_INFO("radio %d radio_RetransmissionMetirc=%d precent_retx=%d\n",
					i, stats_ptr->radio_RetransmissionMetirc, precent_retx);

				stats_ptr->radio_RetransmissionMetirc =
					ecbd_calculate_average(stats_ptr->radio_RetransmissionMetirc,
					precent_retx, stats_ptr->avg_count);
			}
		}

		if (stats_ptr->radio_stats_measure_interval &&
			(tick % stats_ptr->radio_stats_measure_interval) == 0) {
			/* output the result to tmp file (wifi_hal will pick up) */
			snprintf(filename, sizeof(filename), "/tmp/wl%d_%s", i, TMP_RADIO_STATS_FILE);
			fp = fopen(filename, "w");
			if (fp) {
				fprintf(fp, "%d %d %d %d\n",
					stats_ptr->radio_ChannelUtilization,
					stats_ptr->radio_ActivityFactor,
					stats_ptr->radio_RetransmissionMetirc,
					stats_ptr->radio_CarrierSenseThreshold_Exceeded);
				fclose(fp);
				ECBD_PRINT_INFO("write %d %d %d %d to file %s\n",
					stats_ptr->radio_ChannelUtilization,
					stats_ptr->radio_ActivityFactor,
					stats_ptr->radio_RetransmissionMetirc,
					stats_ptr->radio_CarrierSenseThreshold_Exceeded,
					filename);
			}
			else {
				ECBD_PRINT_ERROR("%s: fail to open file %s\n", __FUNCTION__, filename);
				snprintf(log_buf, sizeof(log_buf), "%s: fail to open file %s\n",
					 __FUNCTION__, filename);
				WLAN_APPS_LOGS("ECBD", log_buf);
			}

			/* update rate/inteval from nvram */
			ecbd_update_radio_measure_param(i, 0);
		}
	}
	return ECBD_SUCCESS;
}
#endif /* RDKB_RADIO_STATS_MEASURE */

#define INACTIVITY_TIMEOUT	60
/* TODO: Retrieve this value through ioctl for each interface and store it in the ecbd_info */
static void
ecbd_watchdog(ecbd_info_t *info)
{
	ecbd_stalist_t *ptr = ecbd_stalist_hdr, *next;
	char buf[32] = {0};
	int i;
	wifi_steering_group_t *g_ptr;
	time_t present_time = time(NULL);

	while (ptr != NULL) {
		next = ptr->next;
		if ((ptr->type & STA_TYPE_ASSOC) && (ptr->state == INACTIVE) && (ptr->disassoc_time > 0)) {
			if ((present_time - ptr->disassoc_time)	> INACTIVITY_TIMEOUT) {
				ECBD_PRINT_INFO("Deleting MAC=%s on ssid %s idx=%d bssidx=%d "
					"assoc_time=%lu disassoc_time=%lu\n",
					ether_etoa(ptr->addr.octet, buf), ptr->ssid, ptr->ifidx,
					ptr->bsscfgidx, ptr->assoc_time, ptr->disassoc_time);
				ecbd_del_stalist(&ptr->addr, ptr->ifidx, ptr->bsscfgidx, STA_TYPE_ASSOC);
				present_time = time(NULL);
			}
		}

		ptr = next;
	}

	/* TO DO: sta info update */
	/* ECBD_PRINT_ERROR("(%d) Version: %d\n", info->ticks, info->version); */
	++watchdog_call;
	if ((watchdog_call % STA_INFO_UPDATE_RATE_TIMER) == 0) {
		ecbd_update_rates();
		/* watchdog_call = 0; */
	}

	if ((watchdog_call % WLAN_APPS_LOG_ECBD_REGULAR_TIMER) == 0) {
		ecbd_wifi_app_log_sta(STA_LOG_REGULAR);
	}

	if ((watchdog_call % WLAN_APPS_LOG_ECBD_INITIAL_TIMER) == 0) {
		ecbd_wifi_app_log_sta(STA_LOG_INITIAL);
	}

	/* all groups */
	for (i = 0; i < MAX_STEERING_GROUP_NUM; i++)
	{
		g_ptr = &steer_groups[i];
		if (!g_ptr->group_enable || !g_ptr->group_info) {
			ECBD_PRINT_DEBUG("group %i not ready\n", i);
			continue;
		}

		ecbd_update_group_chan_util(watchdog_call, g_ptr);
	}

	if ((watchdog_call % ECBD_CHECK_ASSOC_INTERVAL) == 0) {
		ecbd_update_stalist_by_assoclist(watchdog_call);
	}

	/* inactive and rssi */
	ecbd_check_sta_status(watchdog_call);

#ifdef RDKB_RADIO_STATS_MEASURE
	ecbd_radio_stats_calculate(watchdog_call);
#endif /* RDKB_RADIO_STATS_MEASURE */
}

static int
ecbd_accept_sta_conn_cb_connection(int sock_fd)
{
	int connection_fd = -1;
	int iter = 0;

	connection_fd = accept(sock_fd,
			(struct sockaddr *)&ecbd_info->sta_conn_cb_subscriber_fds.addr,
			&ecbd_info->sta_conn_cb_subscriber_fds.addr_len);

	if (connection_fd < 0) {
		perror("Invalid connection fd");
		return ECBD_FAIL;
	}

	for (iter = 0; (iter < ecbd_info->sta_conn_cb_subscriber_fds.count) && (iter < MAX_CB_SUBSCRIBERS); iter++) {
		if (ecbd_info->sta_conn_cb_subscriber_fds.FD[iter] == -1) {
			ecbd_info->sta_conn_cb_subscriber_fds.FD[iter] = connection_fd;
			break;
		}
	}

	if (iter == MAX_CB_SUBSCRIBERS) {
		ECBD_PRINT_ERROR("MAX Subscribers reached");
		close(connection_fd);
	}
	else if (iter == ecbd_info->sta_conn_cb_subscriber_fds.count) {
		ecbd_info->sta_conn_cb_subscriber_fds.FD[iter] = connection_fd;
		ecbd_info->sta_conn_cb_subscriber_fds.count++;
	}
	ECBD_PRINT_INFO("STA_CONN Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->sta_conn_cb_subscriber_fds.count);

	snprintf(log_buf, sizeof(log_buf),
		"STA_CONN Callback Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->sta_conn_cb_subscriber_fds.count);
	WLAN_APPS_LOGS("ECBD", log_buf);

	return ECBD_SUCCESS;
}

static int
ecbd_accept_assoc_dev_cb_connection(int sock_fd)
{
	int connection_fd = -1;
	int iter = 0;

	connection_fd = accept(sock_fd,
			(struct sockaddr *)&ecbd_info->assoc_dev_cb_subscriber_fds.addr,
			&ecbd_info->assoc_dev_cb_subscriber_fds.addr_len);

	if (connection_fd < 0) {
		perror("Invalid connection fd");
		return ECBD_FAIL;
	}

	for (iter = 0; (iter < ecbd_info->assoc_dev_cb_subscriber_fds.count) && (iter < MAX_CB_SUBSCRIBERS); iter++) {
		if (ecbd_info->assoc_dev_cb_subscriber_fds.FD[iter] == -1) {
			ecbd_info->assoc_dev_cb_subscriber_fds.FD[iter] = connection_fd;
			break;
		}
	}

	if (iter == MAX_CB_SUBSCRIBERS) {
		ECBD_PRINT_ERROR("MAX Subscribers reached");
		close(connection_fd);
	}
	else if (iter == ecbd_info->assoc_dev_cb_subscriber_fds.count) {
		ecbd_info->assoc_dev_cb_subscriber_fds.FD[iter] = connection_fd;
		ecbd_info->assoc_dev_cb_subscriber_fds.count++;
	}
	ECBD_PRINT_INFO("ASSOC_DEV Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->assoc_dev_cb_subscriber_fds.count);

	snprintf(log_buf, sizeof(log_buf),
		"ASSOC_DEV Callback Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->assoc_dev_cb_subscriber_fds.count);
	WLAN_APPS_LOGS("ECBD", log_buf);

	return ECBD_SUCCESS;
}

static int
ecbd_accept_auth_fail_cb_connection(int sock_fd)
{
	int connection_fd = -1;
	int iter = 0;

	connection_fd = accept(sock_fd,
			(struct sockaddr *)&ecbd_info->auth_fail_cb_subscriber_fds.addr,
			&ecbd_info->auth_fail_cb_subscriber_fds.addr_len);

	if (connection_fd < 0) {
		perror("Invalid connection fd");
		return ECBD_FAIL;
	}

	for (iter = 0; (iter < ecbd_info->auth_fail_cb_subscriber_fds.count) && (iter < MAX_CB_SUBSCRIBERS); iter++) {
		if (ecbd_info->auth_fail_cb_subscriber_fds.FD[iter] == -1) {
			ecbd_info->auth_fail_cb_subscriber_fds.FD[iter] = connection_fd;
			break;
		}
	}

	if (iter == MAX_CB_SUBSCRIBERS) {
		ECBD_PRINT_ERROR("MAX Subscribers reached");
		close(connection_fd);
	}
	else if (iter == ecbd_info->auth_fail_cb_subscriber_fds.count) {
		ecbd_info->auth_fail_cb_subscriber_fds.FD[iter] = connection_fd;
		ecbd_info->auth_fail_cb_subscriber_fds.count++;
	}
	ECBD_PRINT_INFO("AUTH_FAIL Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->auth_fail_cb_subscriber_fds.count);

	snprintf(log_buf, sizeof(log_buf),
		"AUTH_FAIL Callback Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->auth_fail_cb_subscriber_fds.count);
	WLAN_APPS_LOGS("ECBD", log_buf);

	return ECBD_SUCCESS;
}

static int
ecbd_accept_mesh_cb_connection(int sock_fd)
{
	int connection_fd = -1;
	int iter = 0;

	connection_fd = accept(sock_fd,
			(struct sockaddr *)&ecbd_info->mesh_cb_subscriber_fds.addr,
			&ecbd_info->mesh_cb_subscriber_fds.addr_len);

	if (connection_fd < 0) {
		perror("Invalid connection fd");
		return ECBD_FAIL;
	}

	for (iter = 0; (iter < ecbd_info->mesh_cb_subscriber_fds.count) && (iter < MAX_CB_SUBSCRIBERS); iter++) {
		if (ecbd_info->mesh_cb_subscriber_fds.FD[iter] == -1) {
			ecbd_info->mesh_cb_subscriber_fds.FD[iter] = connection_fd;
			break;
		}
	}

	if (iter == MAX_CB_SUBSCRIBERS) {
		ECBD_PRINT_ERROR("MAX Subscribers reached");
		close(connection_fd);
	}
	else if (iter == ecbd_info->mesh_cb_subscriber_fds.count) {
		ecbd_info->mesh_cb_subscriber_fds.FD[iter] = connection_fd;
		ecbd_info->mesh_cb_subscriber_fds.count++;
	}
	ECBD_PRINT_INFO("MESH Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->mesh_cb_subscriber_fds.count);

	snprintf(log_buf, sizeof(log_buf),
		"MESH Callback Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->mesh_cb_subscriber_fds.count);
	WLAN_APPS_LOGS("ECBD", log_buf);

	return ECBD_SUCCESS;
}

static int
ecbd_process_bcn_report_evt(int sock_fd)
{
	int connection_fd = -1;
	int iter = 0;

	connection_fd = accept(sock_fd,
			(struct sockaddr *)&ecbd_info->bcn_report_cb_subscriber_fds.addr,
			&ecbd_info->bcn_report_cb_subscriber_fds.addr_len);

	if (connection_fd < 0) {
		perror("Invalid connection fd");
		return ECBD_FAIL;
	}

	for (iter = 0; (iter < ecbd_info->bcn_report_cb_subscriber_fds.count) && (iter < MAX_CB_SUBSCRIBERS); iter++) {
		if (ecbd_info->bcn_report_cb_subscriber_fds.FD[iter] == -1) {
			ecbd_info->bcn_report_cb_subscriber_fds.FD[iter] = connection_fd;
			break;
		}
	}

	if (iter == MAX_CB_SUBSCRIBERS) {
		ECBD_PRINT_ERROR("MAX Subscribers reached");
		close(connection_fd);
	}
	else if (iter == ecbd_info->bcn_report_cb_subscriber_fds.count) {
		ecbd_info->bcn_report_cb_subscriber_fds.FD[iter] = connection_fd;
		ecbd_info->bcn_report_cb_subscriber_fds.count++;
	}
	ECBD_PRINT_INFO("RRM_BCNREP Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->bcn_report_cb_subscriber_fds.count);

	snprintf(log_buf, sizeof(log_buf),
		"RRM_BCNREP Callback Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->bcn_report_cb_subscriber_fds.count);
	WLAN_APPS_LOGS("ECBD", log_buf);

	return ECBD_SUCCESS;
}

static int
ecbd_accept_bsstrans_cb_connection(int sock_fd)
{
	int connection_fd = -1;
	int iter = 0;

	connection_fd = accept(sock_fd,
			(struct sockaddr *)&ecbd_info->bsstrans_cb_subscriber_fds.addr,
			&ecbd_info->bsstrans_cb_subscriber_fds.addr_len);

	if (connection_fd < 0) {
		perror("Invalid connection fd");
		return ECBD_FAIL;
	}

	for (iter = 0; (iter < ecbd_info->bsstrans_cb_subscriber_fds.count) && (iter < MAX_CB_SUBSCRIBERS); iter++) {
		if (ecbd_info->bsstrans_cb_subscriber_fds.FD[iter] == -1) {
			ecbd_info->bsstrans_cb_subscriber_fds.FD[iter] = connection_fd;
			break;
		}
	}

	if (iter == MAX_CB_SUBSCRIBERS) {
		ECBD_PRINT_ERROR("MAX Subscribers reached");
		close(connection_fd);
	}
	else if (iter == ecbd_info->bsstrans_cb_subscriber_fds.count) {
		ecbd_info->bsstrans_cb_subscriber_fds.FD[iter] = connection_fd;
		ecbd_info->bsstrans_cb_subscriber_fds.count++;
	}
	ECBD_PRINT_INFO("BSSTRANS Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->bsstrans_cb_subscriber_fds.count);

	snprintf(log_buf, sizeof(log_buf),
		"BSSTRANS Callback Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->bsstrans_cb_subscriber_fds.count);
	WLAN_APPS_LOGS("ECBD", log_buf);

	return ECBD_SUCCESS;
}

static int
ecbd_accept_dpp_cb_connection(int sock_fd)
{
	int connection_fd = -1;
	int iter = 0;

	connection_fd = accept(sock_fd,
		(struct sockaddr *)&ecbd_info->dpp_cb_subscriber_fds.addr,
		&ecbd_info->dpp_cb_subscriber_fds.addr_len);

	if (connection_fd < 0) {
		perror("Invalid connection fd");
		return ECBD_FAIL;
	}

	for (iter = 0; (iter < ecbd_info->dpp_cb_subscriber_fds.count) && (iter < MAX_CB_SUBSCRIBERS); iter++) {
		if (ecbd_info->dpp_cb_subscriber_fds.FD[iter] == -1) {
			ecbd_info->dpp_cb_subscriber_fds.FD[iter] = connection_fd;
			break;
		}
	}

	if (iter == MAX_CB_SUBSCRIBERS) {
		ECBD_PRINT_ERROR("MAX Subscribers reached");
		close(connection_fd);
	}
	else if (iter == ecbd_info->dpp_cb_subscriber_fds.count) {
		ecbd_info->dpp_cb_subscriber_fds.FD[iter] = connection_fd;
		ecbd_info->dpp_cb_subscriber_fds.count++;
	}
	ECBD_PRINT_INFO("DPP Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->dpp_cb_subscriber_fds.count);

	snprintf(log_buf, sizeof(log_buf),
		"DPP Callback Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->dpp_cb_subscriber_fds.count);
	WLAN_APPS_LOGS("ECBD", log_buf);

	return ECBD_SUCCESS;
}

static int
ecbd_accept_ch_chg_cb_connection(int sock_fd)
{
	int connection_fd = -1;
	int iter = 0;

	connection_fd = accept(sock_fd,
		(struct sockaddr *)&ecbd_info->ch_chg_cb_subscriber_fds.addr,
		&ecbd_info->ch_chg_cb_subscriber_fds.addr_len);

	if (connection_fd < 0) {
		perror("Invalid connection fd");
		return ECBD_FAIL;
	}

	for (iter = 0; (iter < ecbd_info->ch_chg_cb_subscriber_fds.count) && (iter < MAX_CB_SUBSCRIBERS); iter++) {
		if (ecbd_info->ch_chg_cb_subscriber_fds.FD[iter] == -1) {
			ecbd_info->ch_chg_cb_subscriber_fds.FD[iter] = connection_fd;
			break;
		}
	}

	if (iter == MAX_CB_SUBSCRIBERS) {
		ECBD_PRINT_ERROR("MAX Subscribers reached");
		close(connection_fd);
	}
	else if (iter == ecbd_info->ch_chg_cb_subscriber_fds.count) {
		ecbd_info->ch_chg_cb_subscriber_fds.FD[iter] = connection_fd;
		ecbd_info->ch_chg_cb_subscriber_fds.count++;
	}
	ECBD_PRINT_INFO("CH_CHG Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->ch_chg_cb_subscriber_fds.count);

	snprintf(log_buf, sizeof(log_buf),
		"CH_CHG Callback Accept FD - %d FDs_count - %d\n",
		connection_fd, ecbd_info->ch_chg_cb_subscriber_fds.count);
	WLAN_APPS_LOGS("ECBD", log_buf);

	return ECBD_SUCCESS;
}

static int
ecbd_main_process(ecbd_info_t *info)
{
	int term_fd = -1, sock = -1, timer_fd = -1, sta_conn_fd = -1, bcn_report_fd = -1,
		bsstrans_fd = -1, dpp_fd = -1, ch_chg_fd = -1;
	int assoc_dev_fd = -1, auth_fail_fd = -1, mesh_fd = -1;
	char *val;
	int i = 0, ret = 0;

	/* Register and Initalize TERM signal */
	daemon_sigterm_install(&term_fd);

	/* Set up all file descriptors to listen to */
	/* UDP socket to eapd event */
	if ((sock = ecbd_create_udp_socket(EAPD_WKSP_ECBD_UDP_SPORT)) < 0) {
		ECBD_PRINT_ERROR("Err: fail to init socket for EAPD\n");
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_EAPD].fd = sock;
	fds[FD_EAPD].events = POLLIN;

	/* Set up a socket to listen to message from wifi_hal */
	if ((sock = ecbd_create_udp_socket(WIFI_HAL_TO_ECBD_MSG_UDP_PORT)) < 0) {
		ECBD_PRINT_ERROR("Err: fail to init socket for WiFi HAL\n");
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_WIFI_HAL].fd = sock;
	fds[FD_WIFI_HAL].events = POLLIN;

	timer_fd = timerfd_create(CLOCK_REALTIME, 0);
	if (timer_fd < 0) {
		ECBD_PRINT_ERROR("failed to create timer fd\n");
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_TIMER].fd = timer_fd;
	fds[FD_TIMER].events = POLLIN;

	if (ecbd_start_watchdog_tmr(timer_fd, info->poll_interval, 0) < 0) {
		ret = ECBD_FAIL;
		goto exit;
	}

	if ((sta_conn_fd = ecbd_create_dserver_socket(WIFI_HAL_CB_STA_CONN_DSOCKET)) < 0) {
		ECBD_PRINT_ERROR("failed to create dserver fd for %s\n",
			WIFI_HAL_CB_STA_CONN_DSOCKET);
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_STA_CONN_CB].fd = sta_conn_fd;
	fds[FD_STA_CONN_CB].events = POLLIN;

	if ((assoc_dev_fd = ecbd_create_dserver_socket(WIFI_HAL_CB_ASSOC_DEV_DSOCKET)) < 0) {
		ECBD_PRINT_ERROR("failed to create dserver fd for %s\n",
			WIFI_HAL_CB_ASSOC_DEV_DSOCKET);
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_ASSOC_DEV_CB].fd = assoc_dev_fd;
	fds[FD_ASSOC_DEV_CB].events = POLLIN;

	if ((auth_fail_fd = ecbd_create_dserver_socket(WIFI_HAL_CB_AUTH_FAIL_DSOCKET)) < 0) {
		ECBD_PRINT_ERROR("failed to create dserver fd for %s\n",
			WIFI_HAL_CB_AUTH_FAIL_DSOCKET);
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_AUTH_FAIL_CB].fd = auth_fail_fd;
	fds[FD_AUTH_FAIL_CB].events = POLLIN;

	if ((mesh_fd = ecbd_create_dserver_socket(WIFI_HAL_CB_MESH_STEER_DSOCKET)) < 0) {
		ECBD_PRINT_ERROR("failed to create dserver fd for %s\n",
			WIFI_HAL_CB_MESH_STEER_DSOCKET);
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_MESH_CB].fd = mesh_fd;
	fds[FD_MESH_CB].events = POLLIN;

	if ((bcn_report_fd = ecbd_create_dserver_socket(WIFI_HAL_CB_RRM_BCNREP_DSOCKET)) < 0) {
		ECBD_PRINT_ERROR("failed to create dserver fd for %s\n",
			WIFI_HAL_CB_RRM_BCNREP_DSOCKET);
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_RRM_BCNREP_CB].fd = bcn_report_fd;
	fds[FD_RRM_BCNREP_CB].events = POLLIN;

	/* use domain socket to support 802.11V callback */
	if ((bsstrans_fd = ecbd_create_dserver_socket(WIFI_HAL_CB_BSSTRANS_DSOCKET)) < 0) {
		ECBD_PRINT_ERROR("failed to create dserver fd for %s\n",
			WIFI_HAL_CB_BSSTRANS_DSOCKET);
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_BSSTRANS_CB].fd = bsstrans_fd;
	fds[FD_BSSTRANS_CB].events = POLLIN;

	log_nvram_buf = nvram_get(WLAN_APPS_LOG_FILE_NVRAM);

	/* use domain socket to support DPP callback */
	if ((dpp_fd = ecbd_create_dserver_socket(WIFI_HAL_CB_DPP_DSOCKET)) < 0) {
		ECBD_PRINT_ERROR("failed to create dserver fd for %s\n",
			WIFI_HAL_CB_DPP_DSOCKET);
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_DPP_CB].fd = dpp_fd;
	fds[FD_DPP_CB].events = POLLIN;

	if ((ch_chg_fd = ecbd_create_dserver_socket(WIFI_HAL_CB_CH_CHG_DSOCKET)) < 0) {
		ECBD_PRINT_ERROR("failed to create dserver fd for %s\n",
			WIFI_HAL_CB_CH_CHG_DSOCKET);
		ret = ECBD_FAIL;
		goto exit;
	}

	fds[FD_CH_CHG_CB].fd = ch_chg_fd;
	fds[FD_CH_CHG_CB].events = POLLIN;

	while (1) {

		int ret = poll(fds, nfds, -1);
		if (ret == -1) {
			if (errno == EINTR)
				continue;

			ECBD_PRINT_ERROR("poll failed - %s", strerror(errno));
			break;
		}

		/* Process watchdog timer */
		if (fds[FD_TIMER].revents & POLLIN) {
			ecbd_watchdog(info); /* update sta info and handle non-eapd event */

			val = nvram_safe_get(NVRAM_ECBD_MSGLEVEL);
			if (strcmp(val, ""))
				ecbd_msglevel = strtoul(val, NULL, 0);

			if (ecbd_start_watchdog_tmr(timer_fd, info->poll_interval, 0) < 0) {
				ret = ECBD_FAIL;
				break;
			}
		}

		/* Process EAPD events */
		if (fds[FD_EAPD].revents & POLLIN) {
			ecbd_process_eapd_evt(fds[FD_EAPD].fd);
		}

		/* Process WiFi HAL message */
		if (fds[FD_WIFI_HAL].revents & POLLIN) {
			ecbd_process_wifi_hal_msg(fds[FD_WIFI_HAL].fd);
		}

		if (fds[FD_STA_CONN_CB].revents & POLLIN) {
			ecbd_accept_sta_conn_cb_connection(fds[FD_STA_CONN_CB].fd);
		}
		if (fds[FD_ASSOC_DEV_CB].revents & POLLIN) {
			ecbd_accept_assoc_dev_cb_connection(fds[FD_ASSOC_DEV_CB].fd);
		}
		if (fds[FD_AUTH_FAIL_CB].revents & POLLIN) {
			ecbd_accept_auth_fail_cb_connection(fds[FD_AUTH_FAIL_CB].fd);
		}
		if (fds[FD_MESH_CB].revents & POLLIN) {
			ecbd_accept_mesh_cb_connection(fds[FD_MESH_CB].fd);
		}
		if (fds[FD_RRM_BCNREP_CB].revents & POLLIN) {
			ecbd_process_bcn_report_evt(fds[FD_RRM_BCNREP_CB].fd);
		}
		if (fds[FD_BSSTRANS_CB].revents & POLLIN) {
			ecbd_accept_bsstrans_cb_connection(fds[FD_BSSTRANS_CB].fd);
		}
		if (fds[FD_DPP_CB].revents & POLLIN) {
			ecbd_accept_dpp_cb_connection(fds[FD_DPP_CB].fd);
		}
		if (fds[FD_CH_CHG_CB].revents & POLLIN) {
			ecbd_accept_ch_chg_cb_connection(fds[FD_CH_CHG_CB].fd);
		}
	}

exit:
	ecbd_close_cb_dsockets();
	if (fds[FD_TIMER].fd > 0) {
		ecbd_watchdog_tmr_delete();
		fds[FD_TIMER].fd = 0;
	}

	for (i = 1; i < nfds; i++) {
		if (fds[i].fd >= 0) {
			close(fds[i].fd);
			fds[i].fd = -1;
		}
	}

	return ret;
} /* End of ecbd_main_process */

static int
ecbd_info_configuration(ecbd_info_t *info)
{
	char *val;

	info->version = ECBD_VERSION;
	info->poll_interval = ECBD_DEFAULT_POLL_INTERVAL;

	val = nvram_safe_get(NVRAM_ECBD_ENABLE);
	if (strcmp(val, ""))
		info->enable = (uint8)strtol(val, NULL, 0);

	/* if not enable return */
	if (!ecbd_info->enable) {
		ECBD_PRINT_WARNING("ecbd is not enabled\n");
		return ECBD_FAIL;
	}
	val = nvram_safe_get(NVRAM_ECBD_MSGLEVEL);
	if (strcmp(val, ""))
		ecbd_msglevel = strtoul(val, NULL, 0);

	ECBD_PRINT_INFO("info->enable=%d, ecbd_msglevel=%d\n", info->enable, ecbd_msglevel);

	/* TODO for other config */

	return ECBD_SUCCESS;
}

int
main(int argc, char **argv)
{
	int ret = ECBD_SUCCESS;

	ecbd_info = (ecbd_info_t *)malloc(sizeof(ecbd_info_t));
	if (ecbd_info == NULL) {
		ECBD_PRINT_ERROR("malloc fails\n");
		return ECBD_FAIL;
	}
	memset(ecbd_info, 0, sizeof(ecbd_info_t));

	/* Configure ecbd info */
	if (ecbd_info_configuration(ecbd_info) != ECBD_SUCCESS) {
		ECBD_PRINT_ERROR("Err: fail to init ecbd\n");
		ret = ECBD_FAIL;
		goto exit;
	}

#ifndef RDKB_WLDM
#if defined(BUILD_NO_CMS)
	if (pthread_create(&ltcWlevtThreadId, NULL, (void *)&ltc_socket_create, NULL)) {
		printf("%s:%d create ltcWlevtThread thread fail.\n", __FUNCTION__, __LINE__);
		pthread_cancel(ltcWlevtThreadId);
		exit(errno);
	}
#endif
#endif /* #ifndef RDKB_WLDM */

#ifdef RDKB_RADIO_STATS_MEASURE
	ecbd_radio_stats_init();
#endif /* RDKB_RADIO_STATS_MEASURE */

	ecbd_main_process(ecbd_info);
exit:
	if (ecbd_info)
		free(ecbd_info);

	sleep(1);
	_Exit(ret);
}

/* add probe/auth response control
 * use following ioctl and allow maclist
 * probresp_mac_filter (IOV_PROB_RESP_MAC_FILTER)
 * authresp_mac_filter (IOV_AUTHRESP_MACFLTR)
 */

static int
ecbd_enable_resp_filter(char *ifname)
{
	int ret = ECBD_SUCCESS;

	if (wl_iovar_setint(ifname, "probresp_mac_filter", 1) < 0) {
		ECBD_PRINT_ERROR("Err: set probresp_mac_filter for %s\n", ifname);
		ret = ECBD_FAIL;
	}

	if (wl_iovar_setint(ifname, "authresp_mac_filter", 1) < 0) {
		ECBD_PRINT_ERROR("Err: set authresp_mac_filter for %s\n", ifname);
		ret = ECBD_FAIL;
	}

	return ret;
}

static int
ecbd_enable_macmode_allow(char *ifname)
{
	int val = WLC_MACMODE_DENY;
	/* int val = WLC_MACMODE_DISABLED;  Phase-1 */

	if (wl_ioctl(ifname, WLC_SET_MACMODE, &val, sizeof(val)) < 0) {
		ECBD_PRINT_ERROR("Err: ifnams[%s] set macmode\n", ifname);
		return ECBD_FAIL;
	}

	return ECBD_SUCCESS;
}

static int
ecbd_add_maclist(char *ifname, struct ether_addr *sta_addr)
{
	char tmp_maclist_buf[MAX_IOCTL_BUFLEN];
	struct maclist *tmp_maclist = (struct maclist *)tmp_maclist_buf;
	int i;

	memset(tmp_maclist_buf, 0, MAX_IOCTL_BUFLEN);
	if (wl_ioctl(ifname, WLC_GET_MACLIST, (void *)tmp_maclist_buf, sizeof(tmp_maclist_buf)) < 0) {
		ECBD_PRINT_ERROR("Err: get %s maclist fails\n", ifname);
		return ECBD_FAIL;
	}

	for (i = 0; i < tmp_maclist->count; i++) {
		if (eacmp(sta_addr, &(tmp_maclist->ea[i])) == 0) {
			/* found (already in maclist), do nothing */
			ECBD_PRINT_DEBUG("Succ: sta "MACF" already in maclist on interface %s\n",
				ETHER_TO_MACF(*sta_addr), ifname);
			return ECBD_FAIL;
		}
	}

	memcpy(&(tmp_maclist->ea[tmp_maclist->count]), sta_addr, ETHER_ADDR_LEN);
	tmp_maclist->count ++;

	if (wl_ioctl(ifname, WLC_SET_MACLIST, tmp_maclist, sizeof(tmp_maclist_buf)) < 0) {
		ECBD_PRINT_ERROR("Err: set %s maclist fails (count=%d)\n", ifname, tmp_maclist->count);
		return ECBD_FAIL;
	}

	ECBD_PRINT_DEBUG("Succ: add sta "MACF" to maclist on interface %s\n", ETHER_TO_MACF(*sta_addr), ifname);
	return ECBD_SUCCESS;
}

static int
ecbd_del_maclist(char *ifname, struct ether_addr *sta_addr)
{
	char old_maclist_buf[MAX_IOCTL_BUFLEN];
	char new_maclist_buf[MAX_IOCTL_BUFLEN];
	struct maclist *old_maclist, *new_maclist;
	int i, found = 0;
	struct ether_addr *ea;

	old_maclist = (struct maclist *)old_maclist_buf;
	new_maclist = (struct maclist *)new_maclist_buf;

	memset(old_maclist_buf, 0, MAX_IOCTL_BUFLEN);
	memset(new_maclist_buf, 0, MAX_IOCTL_BUFLEN);

	if (wl_ioctl(ifname, WLC_GET_MACLIST, (void *)old_maclist_buf, sizeof(old_maclist_buf)) < 0) {
		ECBD_PRINT_ERROR("Err: get %s maclist fails\n", ifname);
		return ECBD_FAIL;
	}

	ea = &(new_maclist->ea[0]);
	for (i = 0; i < old_maclist->count; i++) {
		if (eacmp(sta_addr, &(old_maclist->ea[i])) != 0) {
			memcpy(ea, &(old_maclist->ea[i]), sizeof(struct ether_addr));
			new_maclist->count++;
			ea++;
		}
		else {
			found = 1; /* the STA in old maclist */
		}
	}

	if (found == 0) {
		/* sta not in old maclist, do nothing */
		ECBD_PRINT_DEBUG("sta "MACF" is not in maclist on interface %s\n",
			ETHER_TO_MACF(*sta_addr), ifname);
		return ECBD_FAIL;
	}

	if (wl_ioctl(ifname, WLC_SET_MACLIST, new_maclist, sizeof(new_maclist_buf)) < 0) {
		ECBD_PRINT_ERROR("Err: set %s maclist fails (count=%d)\n", ifname, new_maclist->count);
		return ECBD_FAIL;
	}

	ECBD_PRINT_DEBUG("Succ: del sta "MACF" from maclist on interface %s\n", ETHER_TO_MACF(*sta_addr), ifname);
	return ECBD_SUCCESS;
}

#ifndef RDKB_WLDM
#if defined(BUILD_NO_CMS)
/* move pthread (to Comcast/Plume WM connect/disconnect callback) from wlevt.c to here */
pthread_mutex_t g_WLEVT_MUTEX = PTHREAD_MUTEX_INITIALIZER;

#define MAX_CONNECTED_CLIENTS 5
const char notifySocketPath[] = "/tmp/notify-socket";
static int clientSockets[MAX_CONNECTED_CLIENTS] = {-1, -1, -1, -1, -1};
static int master_socket;

void get_wlevt_lock(void)
{
	pthread_mutex_lock(&g_WLEVT_MUTEX);
}

void release_wlevt_lock(void)
{
	pthread_mutex_unlock(&g_WLEVT_MUTEX);
}

static int ltc_socket_deinit(void)
{
	int sd, i;

	if (master_socket >= 0) {
		printf("%s:%d deinitialized \n", __FUNCTION__, __LINE__);
		close(master_socket);
		master_socket = -1;
	}

	get_wlevt_lock();
	for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
		sd = clientSockets[i];
		if (sd >= 0) {
			close(sd);
			clientSockets[i] = 0;
		}
	}
	release_wlevt_lock();

	return 0;
}

static void ltc_socket_create(void)
{
	int addrlen, new_socket, i, sd;
	int max_sd;
	struct sockaddr_un address;

	fd_set readfds;
	/* Create a master domain socket here where instances of liblattice_Wifi socket client
	 * will connect to
	 */
	if ((master_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		printf("%s:%d domain socket failure %d\n", __FUNCTION__, __LINE__, errno);
		return;
	}

	unlink(notifySocketPath);
	/* type of socket created -> UNIX domain socket */
	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, notifySocketPath, sizeof(address.sun_path)-1);

	/* bind the socket */
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
		printf("%s:%d bind err %d\n", __FUNCTION__, __LINE__, errno);
		close(master_socket);
		return;
	}

	/* try to specify maximum MAX_CONNECTED_CLIENTS pending connections for the master socket */
	if (listen(master_socket, MAX_CONNECTED_CLIENTS) < 0) {
		printf("%s:%d socket listen failure %d\n", __FUNCTION__, __LINE__, errno);
		close(master_socket);
		return;
	}

	/* accept the incoming connection */
	addrlen = sizeof(address);
	printf("%s:%d Waiting for connections ...\n", __FUNCTION__, __LINE__);

	while (1) {
		/* clear the socket set */
		FD_ZERO(&readfds);

		/* add master socket to set */
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		get_wlevt_lock();

		/* add child sockets to set */
		for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
			/* socket descriptor */
			sd = clientSockets[i];

			/* if valid socket descriptor then add to read list */
			if (sd >= 0)
				FD_SET(sd, &readfds);

			/* highest file descriptor number, need it for the select function */
			if (sd > max_sd)
				max_sd = sd;
		}
		release_wlevt_lock();

		/* Here any new client connection , it will get set,
		 * say callback registered from CCspwifiagent or plume agent
		 */
		if (FD_ISSET(master_socket, &readfds)) {
			if ((new_socket = accept(master_socket,
				(struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
				printf("%s:%d Queue accept failure %d\n", __FUNCTION__, __LINE__, errno);
				close(master_socket);
				close(new_socket);
				return;
			}

			get_wlevt_lock();
			/* add new socket to array of sockets -> subscriber list */
			for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
				/* if position is empty */
				if (clientSockets[i] == -1) {
					clientSockets[i] = new_socket;
					break;
				}
			}
			release_wlevt_lock();
		}
	}
	ltc_socket_deinit();
	pthread_exit(NULL);
}
#endif /* BUILD_NO_CMS */
#endif /* #ifndef RDKB_WLDM */

/* use the function for both w/ or w/o RDKB_WLDM */
static int ltc_socket_sendup(WL_STA_EVENT_DETAIL *p_sta, WL_STATION_LIST_ENTRY *p_wlsta)
{
#ifndef RDKB_WLDM
	int i;
#endif
	char buf[MAX_IOCTL_BUFLEN], *ptr = buf;
	int len;

	len = sizeof(WL_STA_EVENT_DETAIL);
	memcpy(ptr, (char *)p_sta, len);

	if (p_wlsta) {
		ptr += len;
		memcpy(ptr, (char *)p_wlsta, sizeof(WL_STATION_LIST_ENTRY));
		len += sizeof(WL_STATION_LIST_ENTRY);
		ECBD_PRINT_INFO("append wl_sta info:  rssi=%d snr=%d, len=%d (%d, %d)\n",
			p_wlsta->signalStrength, p_wlsta->snr, len, sizeof(WL_STA_EVENT_DETAIL), sizeof(WL_STATION_LIST_ENTRY));
	} else {
		ECBD_PRINT_INFO("no wl_sta info:  len=%d\n", len);
	}

#ifndef RDKB_WLDM
	get_wlevt_lock();
	for (i = 0; i < MAX_CONNECTED_CLIENTS; i++) {
		int sd = clientSockets[i];
		/* send the message */
		if (sd != -1) {
			/* if (send(sd, (char *)p_sta, sizeof(WL_STA_EVENT_DETAIL), 0) == -1) { */
			if (send(sd, (char *)buf, len, 0) == -1) {
				/* Close the socket and mark as available in list for reuse */
				printf("%s:%d  %d\n", __FUNCTION__, __LINE__, errno);
				close(sd);
				clientSockets[i] = -1;
			}
			else {
				ECBD_PRINT_INFO("%s:%d i=%d sd=%d len=%d send OK\n", __FUNCTION__, __LINE__, i, sd, len);
			}
		}
	}
	release_wlevt_lock();
#else
	ecbd_notify_wifi_hal_subscribers(buf, len, WIFI_HAL_CB_ASSOC_DEV);
#endif
	return 0;
}

/* void ecbd_sent_msg_to_ltc(bcm_event_t *dpkt) */
void ecbd_sent_msg_to_ltc(bcm_event_t *dpkt, WL_STATION_LIST_ENTRY *p_wlsta)
{
	unsigned int idx = 0, sub_idx = 0;
	WL_STA_EVENT_DETAIL sta_detail;
	char buf[32];

	sta_detail.event_type = ntohl(dpkt->event.event_type);

	/* map WLC_E_AUTHORIZED to WLC_E_ASSOC_IND as connect event */
	if (sta_detail.event_type == WLC_E_AUTHORIZED)
		sta_detail.event_type = WLC_E_ASSOC_IND;

	/* map DEAUTH to WLC_E_DISASSOC_IND */
	if (sta_detail.event_type == WLC_E_DEAUTH || sta_detail.event_type == WLC_E_DEAUTH_IND ||
		sta_detail.event_type == WLC_E_DISASSOC)
		sta_detail.event_type = WLC_E_DISASSOC_IND;

	memcpy(&(sta_detail.mac), (char *)(&(dpkt->event.addr)), 6);
	sscanf(dpkt->event.ifname, "wl%u.%u", &idx, &sub_idx);
	sta_detail.radio_idx = idx;
	sta_detail.sub_idx = sub_idx;

	ECBD_PRINT_INFO("%s:%d ifname=%s, idx=%d (%d) sub_idx=%d (%d)\n", __FUNCTION__, __LINE__,
		dpkt->event.ifname, idx, dpkt->event.ifidx,  sub_idx, dpkt->event.bsscfgidx);

	ECBD_PRINT_INFO("%s:%d event_type=%d (%d) STA ADDR=%s\n",
		__FUNCTION__, __LINE__, ntohl(dpkt->event.event_type),
		sta_detail.event_type, ether_etoa(dpkt->event.addr.octet, buf));

	ltc_socket_sendup(&sta_detail, p_wlsta);
}

/* watchdog to send event to ASSOC_DEV (newAp) callback in case assoc event missing */
void ecbd_wd_sent_msg_to_ltc(ecbd_stalist_t *ecbd_sta, int event_type)
{
	WL_STATION_LIST_ENTRY wl_sta, *p_wlsta = NULL;
	WL_STA_EVENT_DETAIL sta_detail;
	char buf[32];

	sta_detail.event_type = event_type;

	/* map WLC_E_AUTHORIZED to WLC_E_ASSOC_IND as connect event */
	if (sta_detail.event_type == WLC_E_AUTHORIZED)
		sta_detail.event_type = WLC_E_ASSOC_IND;

	/* map DEAUTH to WLC_E_DISASSOC_IND */
	if (sta_detail.event_type == WLC_E_DEAUTH || sta_detail.event_type == WLC_E_DEAUTH_IND ||
		sta_detail.event_type == WLC_E_DISASSOC)
		sta_detail.event_type = WLC_E_DISASSOC_IND;

	memcpy(&(sta_detail.mac), (char *)(&(ecbd_sta->addr)), sizeof(struct ether_addr));
	sta_detail.radio_idx = ecbd_sta->ifidx;
	sta_detail.sub_idx = ecbd_sta->bsscfgidx;

	if (ecbd_get_wl_stainfo(&wl_sta, ecbd_sta) == ECBD_SUCCESS)
		p_wlsta = &wl_sta;

	ECBD_PRINT_INFO("ifname=%s idx=%d subidx=%d event=%d STA=%s\n",
		ecbd_sta->ifname, sta_detail.radio_idx, sta_detail.sub_idx,
		sta_detail.event_type, ether_etoa(sta_detail.mac, buf));

	ltc_socket_sendup(&sta_detail, p_wlsta);
}

int
ecbd_get_wl_stainfo(WL_STATION_LIST_ENTRY *ptr_sta, ecbd_stalist_t *sta_info)
{
	struct maclist *assoclist;
	int rc, cnt;
	int rssi, noise;
	char ioctl_buf[MAX_IOCTL_BUFLEN];
	int buflen;
	char *param;
	sta_info_t *sta_info_io;

	if (sta_info == NULL) {
		ECBD_PRINT_ERROR("%s@%d sta_info is NULL", __FUNCTION__, __LINE__);
		return ECBD_FAIL;
	}

	memset(ptr_sta, 0, sizeof(WL_STATION_LIST_ENTRY));

	/* assoclist "WLC_GET_ASSOCLIST" for active & associated */
	assoclist = malloc(WLC_IOCTL_MEDLEN);
	if (!assoclist) {
		ECBD_PRINT_ERROR("%s: malloc failed, %s\n", __FUNCTION__, strerror(errno));
		return ECBD_FAIL;
	}

	assoclist->count = htod32((WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN);
	rc = wl_ioctl(sta_info->ifname, WLC_GET_ASSOCLIST, assoclist, WLC_IOCTL_MEDLEN);
	if (rc < 0) {
		ECBD_PRINT_ERROR("%s: fail to get assoclist, %s\n", __FUNCTION__, strerror(errno));
		free(assoclist);
		return ECBD_FAIL;
	}

	ECBD_PRINT_INFO("%s: assoclist count = %d, check "MACF"\n",
		__FUNCTION__, assoclist->count, ETHER_TO_MACF(sta_info->addr));

	for (cnt = 0; cnt < assoclist->count; cnt++) {
		ECBD_PRINT_INFO("cnt[%d] mac:"MACF"\n", cnt, ETHER_TO_MACF(assoclist->ea[cnt]));
		if (eacmp(&(assoclist->ea[cnt]), &(sta_info->addr)) == 0) {
			ptr_sta->active = ptr_sta->associated = 1;
			ECBD_PRINT_INFO("mac:"MACF" in assoclist\n", ETHER_TO_MACF(sta_info->addr));
			break;
		}
	}
	free(assoclist);

	/* autho_sta_list for authenticationState & authorized */
	strcpy(ioctl_buf, "autho_sta_list");
	rc = wl_ioctl(sta_info->ifname, WLC_GET_VAR, ioctl_buf, sizeof(ioctl_buf));

	if (rc < 0) {
		ECBD_PRINT_ERROR("%s: fail to get autho_sta_list, %s\n", __FUNCTION__, strerror(errno));
		return ECBD_FAIL;
	}

	assoclist = (struct maclist *)ioctl_buf;

	ECBD_PRINT_INFO("%s: autho_sta_list count = %d, check "MACF"\n",
		__FUNCTION__, assoclist->count, ETHER_TO_MACF(sta_info->addr));

	for (cnt = 0; cnt < assoclist->count; cnt++) {
		ECBD_PRINT_INFO("cnt[%d] mac:"MACF"\n", cnt, ETHER_TO_MACF(assoclist->ea[cnt]));
		if (eacmp(&(assoclist->ea[cnt]), &(sta_info->addr)) == 0) {
			ptr_sta->authenticationState = ptr_sta->authorized = 1;
			ECBD_PRINT_INFO("mac:"MACF" in autho_sta_list\n", ETHER_TO_MACF(sta_info->addr));
			break;
		}
	}

	/* sta_info for */
	strcpy(ioctl_buf, "sta_info");
	buflen = strlen(ioctl_buf) + 1;
	param = (char *)(ioctl_buf + buflen);
	memcpy(param, &sta_info->addr, ETHER_ADDR_LEN);

	rc = wl_ioctl(sta_info->ifname, WLC_GET_VAR, ioctl_buf, sizeof(ioctl_buf));

	if (rc < 0) {
		ECBD_PRINT_ERROR("Err: intf:%s STA:"MACF" sta_info\n",
			sta_info->ifname, ETHER_TO_MACF(sta_info->addr));
	} else {
		sta_info_io = (sta_info_t *)ioctl_buf;
		ptr_sta->lastDataDownlinkRate = sta_info_io->rx_rate;
		ptr_sta->lastDataUplinkRate = sta_info_io->tx_rate;
		ptr_sta->retransmissions = sta_info_io->tx_pkts_retries;
		ECBD_PRINT_INFO("ecbd_get_wl_stainfo: lastDataDownlinkRate=%d "
			"lastDataUplinkRate=%d retransmissions=%d\n",
			ptr_sta->lastDataDownlinkRate,
			ptr_sta->lastDataUplinkRate,
			ptr_sta->retransmissions);

		if (sta_info_io->flags & WL_STA_VHT_CAP) {
			strcpy(ptr_sta->operStandard, "ac");
		}
		else if (sta_info_io->flags & WL_STA_N_CAP) {
			strcpy(ptr_sta->operStandard, "n");
		}
		else {
			strcpy(ptr_sta->operStandard, "g");
		}

		switch (sta_info_io->link_bw) {
			case 1:
				ptr_sta->operBandwidth = 20;
				break;
			case 2:
				ptr_sta->operBandwidth = 40;
				break;
			case 3:
				ptr_sta->operBandwidth = 80;
				break;
			case 4:
				ptr_sta->operBandwidth = 160;
				break;
			default:
				break;
		}
	}

	/* rssi for signalStrength */
	if (_ecbd_get_sta_rssi(sta_info->ifname, &(sta_info->addr), &rssi) == ECBD_SUCCESS) {
		ptr_sta->signalStrength = rssi;
		ECBD_PRINT_INFO("Get rssi %d for MAC="MACF"\n", rssi, ETHER_TO_MACF(sta_info->addr));

		/* rssi - noise for snr */
		if (ecbd_get_noise(sta_info->ifname, &noise) == ECBD_FAIL) {
			ECBD_PRINT_ERROR("Err: intf:%s noise\n", sta_info->ifname);
			noise = -90; /* use a default value */
		}

		ptr_sta->snr = rssi - noise;

		ECBD_PRINT_INFO("ecbd_get_wl_stainfo: rssi=%d noise=%d snr=%d active=%d\n",
			rssi, noise, ptr_sta->snr, ptr_sta->active);
	}
	else {
		ECBD_PRINT_ERROR("Fail to read rssi for MAC="MACF"\n", ETHER_TO_MACF(sta_info->addr));
	}

	return ECBD_SUCCESS;
}
