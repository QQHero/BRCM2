/*
 * Linux-specific portion of SSD (SSID Steering Daemon)
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
 * $Id: ssd.c $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <ethernet.h>
#include <wlutils.h>
#include <bcmnvram.h>
#include <ctype.h>
#include <wlif_utils.h>
#include <bcmparams.h>
#include <shutils.h>
#include <ssd.h>
#include <security_ipc.h>
#ifndef MAX_RADIO_NUM
#define MAX_RADIO_NUM 4 /* refer to rc/rc.c */
#endif

extern char * strcasestr(const char * s1, const char * s2);

#ifdef SSD_DEBUG
int ssd_msglevel = SSD_DEBUG_ERROR | SSD_DEBUG_WARNING | SSD_DEBUG_INFO;
#define SSD_DEBUG_DUMP
#else
int ssd_msglevel = SSD_DEBUG_ERROR | SSD_DEBUG_WARNING;
#endif

/* some global variables */
ssd_maclist_t *ssd_maclist_hdr = NULL; /* associated sta maclist to be handled */

static bool ssd_running = FALSE;

/* for ssd_cli to daemon */
static int cli_listenfd = -1, cli_sendfd = -1;

static void
ssd_term_hdlr(int sig)
{
	ssd_running = FALSE;
	return;
}

#ifdef SSD_DEBUG_DUMP
static void ssd_hexdump_ascii(const char *title, const unsigned char *buf,
        unsigned int len)
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
#endif /* SSD_DEBUG_DUMP */

/* add addr to assoc maclist */
static void ssd_add_assoc_maclist(char *ssid, bcm_event_t *dpkt)
{
	ssd_maclist_t *ptr;
	char buf[32];
	struct ether_addr *sta_addr;

	if (!dpkt) {
		SSD_PRINT_ERROR("Exiting: empty event packet\n");
		return;
	}

	sta_addr = &(dpkt->event.addr);

	/* adding to maclist */
	ptr = ssd_maclist_hdr;

	while (ptr) {
		if ((eacmp(&(ptr->addr), sta_addr) == 0) &&
			(ptr->ifidx == dpkt->event.ifidx) &&
			(ptr->bsscfgidx == dpkt->event.bsscfgidx)) {
			ptr->timestamp = time(NULL);
			SSD_PRINT_INFO("update ssid %s, MAC=%s\n",
				ssid, ether_etoa(sta_addr->octet, buf));
			return;
		}
		ptr = ptr->next;
	}

	/* add new sta to maclist */
	ptr = malloc(sizeof(ssd_maclist_t));
	if (!ptr) {
		SSD_PRINT_ERROR("Exiting malloc failure\n");
		return;
	}
	memset(ptr, 0, sizeof(ssd_maclist_t));
	memcpy(&ptr->addr, sta_addr, sizeof(struct ether_addr));
	ptr->next = ssd_maclist_hdr;
	ssd_maclist_hdr = ptr;

	ptr->timestamp = time(NULL);
	strncpy(ptr->ssid, ssid, sizeof(ptr->ssid));
	ptr->ssid[sizeof(ptr->ssid)-1] = '\0';
	ptr->ifidx = dpkt->event.ifidx;
	ptr->bsscfgidx = dpkt->event.bsscfgidx;
	ptr->security = 0;

	SSD_PRINT_INFO("add ssid %s, MAC=%s\n", ssid, ether_etoa(sta_addr->octet, buf));

	return;
}

/* remove addr from maclist */
static void ssd_del_assoc_maclist(struct ether_addr *sta_addr, uint8 ifidx, uint8 bsscfgidx)
{
	ssd_maclist_t *ptr, *prev;
	char buf[32];
	int found = 0;

	ptr = ssd_maclist_hdr;

	if (ptr == NULL) {
		SSD_PRINT_INFO("ssd_mac_list is empty\n");
		return;
	}

	prev = NULL;
	while (ptr) {
		if (!ptr->softblocked && (eacmp(&(ptr->addr), sta_addr) == 0) &&
			(ptr->ifidx == ifidx) && (ptr->bsscfgidx == bsscfgidx)) {
			if (prev)
				prev->next = ptr->next;
			else
				ssd_maclist_hdr = ptr->next;
			found = 1;
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}

	if (ptr && found) {
		SSD_PRINT_INFO("Free ssid %s, MAC=%s, timestamp=%lu\n",
			ptr->ssid, ether_etoa(ptr->addr.octet, buf), ptr->timestamp);
		free(ptr);
	}

	/* Debug only, show list after removal */
	ptr = ssd_maclist_hdr;
	while (ptr) {
		SSD_PRINT_INFO("ssid %s, MAC=%s, timestamp=%lu\n",
			ptr->ssid, ether_etoa(ptr->addr.octet, buf), ptr->timestamp);
		ptr = ptr->next;
	}

	return;
}

static int ssd_ssid_type(char *ssid, char* ifname)
{
	int ret;
	char nvram_name[100], prefix[IFNAMSIZ];
	char *nvram_str;

	if (osifname_to_nvifname(ifname, prefix, IFNAMSIZ) < 0) {
		SSD_PRINT_WARNING("fail to convert ifname %s to nv_ifname\n", ifname);
		return SSD_TYPE_DISABLE;
	}

	SSD_PRINT_INFO("os_ifname=%s; nv_ifname=%s\n", ifname, prefix);

	/* compare ssid first */
	if (ssid) {
		sprintf(nvram_name, "%s_ssid", prefix);
		nvram_str = nvram_get(nvram_name);
		if ((nvram_str == NULL) || strcmp(nvram_str, ssid)) {
			SSD_PRINT_INFO("ssid (%s) not match (%s) on nvram (%s)\n",
				ssid, nvram_str, nvram_name);
			return SSD_TYPE_DISABLE;
		}
	}

	sprintf(nvram_name, "%s_%s", prefix, NVRAM_SSD_SSID_TYPE);
	nvram_str = nvram_get(nvram_name);

	SSD_PRINT_INFO("nvram=%s, value=%s\n", nvram_name, nvram_str);

	if (nvram_str == NULL) {
		ret = SSD_TYPE_DISABLE;
	}
	else {
		ret = atoi(nvram_str);
		if ((ret < SSD_TYPE_DISABLE) || (ret > SSD_TYPE_PUBLIC))
			ret = SSD_TYPE_DISABLE;
	}

	return ret;
}

static char *nvram_get_with_prefix(char *prefix, char *basename)
{
	char nvram_name[100];
	sprintf(nvram_name, "%s_%s", prefix, basename);
	return nvram_get(nvram_name);
}

static void ssd_update_deny_list(char *prefix, ssd_maclist_t *ptr)
{
	char *nvram_str;
	char buf[32], maclist_str[ETHER_ADDR_STR_LEN * 64];
	char mbuf[WLC_IOCTL_SMLEN] __attribute__ ((aligned(4)));
	struct maclist *maclist_ptr;
	struct ether_addr *ea;
	char var[32], *next;
	char os_ifname[IFNAMSIZ];
	int change_macmode = 0, val;

	/* xx_bss_enabled=1 */
	nvram_str = nvram_get_with_prefix(prefix, "bss_enabled");
	if ((nvram_str == NULL) || (atoi(nvram_str) != 1)) {
		return;
	}
	SSD_PRINT_INFO("%s_%s=%s\n", prefix, "bss_enabled", nvram_str);

	/* if xx_mode == ap */
	nvram_str = nvram_get_with_prefix(prefix, "mode");
	if ((nvram_str == NULL) || strcmp(nvram_str, "ap")) {
		return;
	}
	SSD_PRINT_INFO("%s_%s=%s\n", prefix, "mode", nvram_str);

	/* if xx_ssd_type == SSD_TYPE_PUBLIC */
	nvram_str = nvram_get_with_prefix(prefix, "ssd_type");
	if ((nvram_str == NULL) || (atoi(nvram_str) != SSD_TYPE_PUBLIC)) {
		return;
	}
	SSD_PRINT_INFO("%s_%s=%s\n", prefix, "ssd_type", nvram_str);

	/* if xx_macmode == deny or disabled */
	nvram_str = nvram_get_with_prefix(prefix, "macmode");
	if ((nvram_str == NULL) || (strcmp(nvram_str, "allow") == 0)) {
		return;
	}

	if (strcmp(nvram_str, "disabled") == 0)
		change_macmode = 1; /* force to use "deny" when original setting is "disabled" */
	SSD_PRINT_INFO("%s_%s=%s\n", prefix, "macmode", nvram_str);

	/* add the mac to xx_maclist */
	nvram_str = nvram_get_with_prefix(prefix, "maclist");
	ether_etoa(ptr->addr.octet, buf);

	SSD_PRINT_INFO("maclists=%s, new mac=%s\n", nvram_str, buf);

	if ((nvram_str == NULL) || strcasestr(nvram_str, buf) == NULL)
	{
		/* not exist, append the new mac */
		SSD_PRINT_INFO("change %s's deny maclist: append new MAC=%s\n", prefix, buf);

		/* update nvram */
		if (nvram_str == NULL)
			sprintf(maclist_str, "%s", buf);
		else {
			if ((strlen(nvram_str) + strlen(buf)) > sizeof(maclist_str)) {
				SSD_PRINT_WARNING("maclist length exceeds limit\n");
				return;
			}
			sprintf(maclist_str, "%s %s", nvram_str, buf);
		}

		sprintf(buf, "%s_maclist", prefix);
		nvram_set(buf, maclist_str);

		if (change_macmode) {
			sprintf(buf, "%s_macmode", prefix);
			nvram_set(buf, "deny");
		}

		/* nvram_commit */
		nvram_commit();

		/* setup a flag for 2 purposes:
		   1. avoid to apply the action multiple times
		   2. keep the record in list for log (ssd_cli -l)
		*/
		ptr->softblocked = 1;

		/* Set the real-time MAC list via ioctl */
		maclist_ptr = (struct maclist *)mbuf;
		maclist_ptr->count = 0;

		ea = maclist_ptr->ea;
		foreach(var, maclist_str, next) {
			if (((char *)((&ea[1])->octet)) > ((char *)(&mbuf[sizeof(mbuf)])))
				break;
			if (ether_atoe(var, ea->octet)) {
				maclist_ptr->count++;
				ea++;
			}
		}
		SSD_PRINT_INFO("maclist_ptr->count=%d\n", maclist_ptr->count);

		/* ioctl to inform driver (need os_name) */
		if (nvifname_to_osifname(prefix, os_ifname, sizeof(os_ifname)) < 0) {
			SSD_PRINT_WARNING("fail to convert to os_ifname for %s!\n", prefix);
			return;
		}

		if (wl_ioctl(os_ifname, WLC_SET_MACLIST, mbuf, sizeof(mbuf))) {
			SSD_PRINT_WARNING("wl_ioctl WLC_SET_MACLIST error!\n");
		}

		if (change_macmode) {
			val = WLC_MACMODE_DENY;
			if (wl_ioctl(os_ifname, WLC_SET_MACMODE, &val, sizeof(val))) {
				SSD_PRINT_WARNING("wl_ioctl WLC_SET_MACMODE error!\n");
			}
		}
	}
	else {
		/* consider previously-added maclist as softblocked also */
		ptr->softblocked = 1;
		SSD_PRINT_INFO("MAC %s is already in %s's deny maclist\n", buf, prefix);
	}
}

static void ssd_process_assoc_maclist(ssd_maclist_t *ptr)
{
	int i, j;
	char nv_name[IFNAMSIZ], ifname[IFNAMSIZ+6];
	char buf[32];

	SSD_PRINT_INFO("ssid %s, MAC=%s, timestamp=%lu\n",
		ptr->ssid, ether_etoa(ptr->addr.octet, buf), ptr->timestamp);

	/* for loop for all interfaces */
	for (i = 1; i <= DEV_NUMIFS; i++) {
#if defined(BCA_CPEROUTER)
		sprintf(ifname, "wl%d", i-1);
#else
		sprintf(ifname, "eth%d", i);
#endif

		if (!wl_probe(ifname)) {
			if (osifname_to_nvifname(ifname, nv_name, sizeof(nv_name)) < 0) {
				SSD_PRINT_WARNING("fail to convert ifname %s to nv_name\n", ifname);
				return;
			}

			SSD_PRINT_INFO("convert ifname %s to nv_name %s\n", ifname, nv_name);

			/* handle this primary interface */
			if (strlen(nv_name) == 3) {
				SSD_PRINT_INFO("handle the primary interface %s\n", nv_name);
				ssd_update_deny_list(nv_name, ptr);
			}

			/* for loop for all virtual BSS interface */
			for (j = 1; j < WL_MAXBSSCFG; j++) {
				sprintf(ifname, "%s.%d", nv_name, j);
				ssd_update_deny_list(ifname, ptr);
			}
		}
	}
}

static void ssd_check_assoc_maclist(void)
{
	ssd_maclist_t *ptr;
	time_t now = time(NULL);
	char buf[32];

	ptr = ssd_maclist_hdr;

	if (ptr) {
		SSD_PRINT_INFO("current timestamp=%lu\n", now);
	}

	while (ptr) {
		SSD_PRINT_INFO("ssid %s, MAC=%s, timestamp=%lu\n",
			ptr->ssid, ether_etoa(ptr->addr.octet, buf), ptr->timestamp);
		if ((now - ptr->timestamp > INTERVAL_ASSOC_CONFIRM) &&
			!ptr->softblocked) {
			ssd_process_assoc_maclist(ptr);
			ssd_del_assoc_maclist(&(ptr->addr), ptr->ifidx, ptr->bsscfgidx);
			/* handle one item once */
			break;
		}
		ptr = ptr->next;
	}

	return;
}

static void ssd_event_handler(int sock)
{
	int bytes;
	bcm_event_t *dpkt;
	char buf[32];
	uchar buf_ptr[MAX_EVENT_BUFFER_LEN], *ptr = buf_ptr;
	char ssid[MAX_SSID_LEN+1];
	uint8 ssid_len;
	int ssid_type;
	uint32 event_id;
	struct timeval tv = {1, 0};    /* timed out every second */
	fd_set fdset;
	int status, fdmax;

	FD_ZERO(&fdset);
	fdmax = -1;

	if (sock >= 0) {
		FD_SET(sock, &fdset);
		if (sock > fdmax)
			fdmax = sock;
	}
	else {
		SSD_PRINT_ERROR("Err: wrong socket\n");
		return;
	}

	/* support ssd_cli */
	if (cli_listenfd != SSD_DEFAULT_FD) {
		FD_SET(cli_listenfd, &fdset);
		if (cli_listenfd > fdmax) {
			fdmax = cli_listenfd;
		}
	}

	status = select(fdmax+1, &fdset, NULL, NULL, &tv);
	if ((status > 0) && FD_ISSET(sock, &fdset)) {
		if ((bytes = recv(sock, ptr, MAX_EVENT_BUFFER_LEN, 0)) > IFNAMSIZ) {

			ptr = ptr + IFNAMSIZ;
			dpkt = (bcm_event_t *)ptr;

#ifdef SSD_DEBUG_DUMP
			ssd_hexdump_ascii("REVD:", ptr, bytes);
#endif

			event_id = ntohl(dpkt->event.event_type);
			SSD_PRINT_INFO("Received event %d, MAC=%s\n",
				event_id, ether_etoa(dpkt->event.addr.octet, buf));

			if ((event_id == WLC_E_ASSOC_IND) || (event_id == WLC_E_REASSOC_IND)) {
				ptr += BCM_EVENT_HEADER_LEN;
				ssid_len = *(ptr+1);
				if ((*ptr == 0) && (ssid_len > 0) && (ssid_len < (MAX_SSID_LEN+1)))
				{
					/* SSID IE */
					strncpy(ssid, (char*)(ptr+2), ssid_len);
					ssid[ssid_len] = '\0';
					SSD_PRINT_INFO("event=%d,ssid=%s,if=%s,idx=%d,bidx=%d\n",
						event_id, ssid, dpkt->event.ifname,
						dpkt->event.ifidx, dpkt->event.bsscfgidx);

					ssid_type = ssd_ssid_type(ssid, dpkt->event.ifname);
					if (ssid_type == SSD_TYPE_PRIVATE) {
						SSD_PRINT_INFO("Add MAC=%s for private %s\n",
							ether_etoa(dpkt->event.addr.octet, buf),
							dpkt->event.ifname);
						ssd_add_assoc_maclist(ssid, dpkt);
					}
				}
			}
			else if (DISCONNECT_EVENT(event_id)) {
				/* remove the STA from the ssd_maclist */
				ssid_type = ssd_ssid_type(NULL, dpkt->event.ifname);
				if (ssid_type == SSD_TYPE_PRIVATE) {
					SSD_PRINT_INFO("Event %d: del MAC=%s ifidx=%d, bssidx=%d\n",
						event_id, ether_etoa(dpkt->event.addr.octet, buf),
						dpkt->event.ifidx, dpkt->event.bsscfgidx);
					ssd_del_assoc_maclist(&(dpkt->event.addr),
						dpkt->event.ifidx, dpkt->event.bsscfgidx);
				}
			}
		}
	}

	/* check maclist */
	if (ssd_maclist_hdr)
		ssd_check_assoc_maclist();

	/* Process CLI commands */
	if (cli_listenfd != SSD_DEFAULT_FD && FD_ISSET(cli_listenfd, &fdset)) {
		ssd_daemon_proc_cli_req();
	}

	return;
}

static void
ssd_main_loop(int sock)
{
	/* establish a handler to handle SIGTERM. */
	signal(SIGTERM, ssd_term_hdlr);
	ssd_running = TRUE;
	while (ssd_running) {
		ssd_event_handler(sock);
	}
}

static int ssd_eapd_socket_init(void)
{
	int reuse = 1;
	struct sockaddr_in sockaddr;
	int ssd_socket = -1;

	/* open loopback socket to communicate with EAPD */
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sockaddr.sin_port = htons(EAPD_WKSP_SSD_UDP_SPORT);

	if ((ssd_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		SSD_PRINT_ERROR("Unable to create loopback socket\n");
		return -1;
	}

	if (setsockopt(ssd_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
		SSD_PRINT_ERROR("Unable to setsockopt to loopback socket %d.\n", ssd_socket);
		goto exit1;
	}

	if (bind(ssd_socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		SSD_PRINT_ERROR("Unable to bind to loopback socket %d\n", ssd_socket);
		goto exit1;
	}

	SSD_PRINT_INFO("opened loopback socket %d\n", ssd_socket);
	return ssd_socket;

	/* error handling */
exit1:
	close(ssd_socket);
	return -1;
}

static void ssd_usage(void)
{
	printf("\nssd nvram:\n");
	printf(" ssd_enable=0|1 <0: disable; 1:enable>\n");
	printf(" wlx[.y]_ssd_type=0|1|2 <0: Disabled; 1: Private; 2: Public>\n");
	printf(" ssd_msglevel=1|2|4 <bit0: Err; bit1:Warning; bit2:Info>\n");

	printf("\nssd_cli command line options:\n");
	printf(" -d read nvram to set msglevel\n");
	printf(" -c clear softblock\n");
	printf(" -l list softblock\n");
	printf(" -h\n");
	printf(" -H this help usage\n");
	printf("\n");
}

int ssd_clear_softblock(void)
{
	int i, j;
	char ifname[IFNAMSIZ], prefix[16], nvname[32], *nvram_str = NULL, cmd[64];
	ssd_maclist_t *ptr, *next;

	/* loop for all wifi interfaces */
	for (i = 1; i <= MAX_RADIO_NUM; i++) {
		/* read ifname from nvram wlx_ifname */
		snprintf(nvname, sizeof(nvname), "wl%d_ifname", i - 1);
		nvram_str = nvram_get(nvname);
		if (nvram_str != NULL) {
			strncpy(ifname, nvram_str, sizeof(ifname) - 1);
			SSD_PRINT_INFO("os_ifname %s for %s\n", ifname, nvname);
		} else {
			SSD_PRINT_INFO("nvram %s not defined\n", nvname);
			continue;
		}

		/* make sure ifname exsits and is wl interface */
		if (!wl_probe(ifname)) {
			for (j = 0; j < WL_MAXBSSCFG; j++) {
				if (j == 0)
					snprintf(prefix, sizeof(prefix), "wl%d", i - 1);
				else
					snprintf(prefix, sizeof(prefix), "wl%d.%d", i - 1, j);

				/* only handle interface with ssd_type = SSD_TYPE_PUBLIC (2) */
				nvram_str = nvram_get_with_prefix(prefix, "ssd_type");
				if ((nvram_str == NULL) || (atoi(nvram_str) != SSD_TYPE_PUBLIC)) {
					continue;
				}

				snprintf(cmd, sizeof(cmd), "wl -i %s mac none", j ? prefix : ifname);
				system(cmd);
				SSD_PRINT_INFO("cmd to clean maclist <%s>\n", cmd);

				snprintf(nvname, sizeof(nvname), "%s_maclist", prefix);
				nvram_set(nvname, "");

			} /* bssidx */
		}
	} /* ifidx */

	/* clean all internal database */
	ptr = ssd_maclist_hdr;
	while (ptr) {
		next = ptr->next;
		free(ptr);
		ptr = next;
	}
	ssd_maclist_hdr = NULL;

	return SSD_OK;
}

/* output format: <date_str> <ifidx> <bssidx> <sta mac_str> */
int ssd_list_softblock(void)
{
	int i, j, ret = SSD_OK;
	char ifname[IFNAMSIZ], nvname[32], *nvram_str = NULL;
	char cmd[128], buf[32], date_str[128] = {0};
	FILE *out;
	ssd_maclist_t *ptr;
	struct tm tm;

	if (!ssd_maclist_hdr) {
		SSD_PRINT_INFO("no softblock record\n");
		/* create an empty file */
		snprintf(cmd, sizeof(cmd), "touch %s", SSD_SOFTBLOCK_LIST_FILE);
		system(cmd);
		SSD_PRINT_INFO("cmd to create empty softblock list <%s>\n", cmd);
		return SSD_OK;
	}

	if ((out = fopen(SSD_SOFTBLOCK_LIST_FILE_TMP, "w")) == NULL) {
		SSD_PRINT_ERROR("Fail to open log file\n");
		return SSD_FAIL;
	}

	/* loop for all wifi interfaces */
	for (i = 1; i <= MAX_RADIO_NUM; i++) {
		/* read ifname from nvram wlx_ifname */
		snprintf(nvname, sizeof(nvname), "wl%d_ifname", i - 1);
		nvram_str = nvram_get(nvname);
		if (nvram_str != NULL) {
			strncpy(ifname, nvram_str, sizeof(ifname) - 1);
			SSD_PRINT_INFO("os_ifname %s for %s\n", ifname, nvname);
		} else {
			SSD_PRINT_INFO("nvram %s not defined\n", nvname);
			continue;
		}

		/* make sure ifname exsits and is wl interface */
		if (!wl_probe(ifname)) {
			for (j = 0; j < WL_MAXBSSCFG; j++) {
				if (j == 0)
					snprintf(nvname, sizeof(nvname), "wl%d_ssd_type", i - 1);
				else
					snprintf(nvname, sizeof(nvname), "wl%d.%d_ssd_type", i - 1, j);

				/* only handle interface with ssd_type = SSD_TYPE_PUBLIC (2) */
				nvram_str = nvram_get(nvname);
				if ((nvram_str == NULL) || (atoi(nvram_str) != SSD_TYPE_PUBLIC)) {
					continue;
				}

				/* output to tmp file */
				ptr = ssd_maclist_hdr;

				while (ptr) {
					SSD_PRINT_INFO("ssid %s, MAC=%s, timestamp=%lu, softblocked=%d\n",
						ptr->ssid, ether_etoa(ptr->addr.octet, buf), ptr->timestamp, ptr->softblocked);

					if (ptr->softblocked) {
						tm = *localtime(&ptr->timestamp);
						snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d@%02d:%02d:%02d",
							tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
						fprintf(out, "%s %d %d %s\n", date_str,
							i - 1, j, ether_etoa(ptr->addr.octet, buf));
					}
					ptr = ptr->next;
				}
			} /* bssidx */
		}
	} /* ifidx */

	fclose(out);
	if (rename(SSD_SOFTBLOCK_LIST_FILE_TMP, SSD_SOFTBLOCK_LIST_FILE) != 0) {
		SSD_PRINT_ERROR("Err for log data");
		unlink(SSD_SOFTBLOCK_LIST_FILE_TMP);
		ret = SSD_FAIL;
	}

	return ret;
}

/* "ssd_cli -d" to read nvram and set msglevel */
static int ssd_set_msglevel_hdlr(void)
{
	int ret = SSD_OK;
	char *val = NULL;

	if ((val = nvram_get(NVRAM_SSD_DEBUG_LEVEL)) == NULL) {
		SSD_PRINT_ERROR("nvram %s not configured\n", NVRAM_SSD_DEBUG_LEVEL);
		ret = SSD_FAIL;
	} else {
		ssd_msglevel = strtoul(val, NULL, 0);
	}
	return ret;
}

/* "ssd_cli -c" for wifi_clearSoftBlockBlacklist */
static int ssd_clear_softblock_hdlr(void)
{
	int ret;

	SSD_PRINT_INFO("Enter\n");
	ret = ssd_clear_softblock();
	SSD_PRINT_INFO("Exit\n");
	return ret;
}

/* "ssd_cli -l" for wifi_getSoftBlockBlacklistEntries */
static int ssd_list_softblock_hdlr(void)
{
	int ret;

	SSD_PRINT_INFO("Enter\n");
	ret = ssd_list_softblock();
	SSD_PRINT_INFO("Exit\n");
	return ret;
}

/* use socket to deliver cli message to daemon */

/* common function to close the socket */
void
ssd_close_socket(int *sockfd)
{
	if (*sockfd < 0) {
		return;
	}
	close(*sockfd);
	*sockfd = SSD_DEFAULT_FD;
}

/* daemon side
	1. socket init
	2. listen socket
	3. process cli message
*/
static int
ssd_proc_clicmd(uint8 cmd_id)
{
	int ret;

	switch (cmd_id) {
		case SSD_CMD_SET_MSGLEVEL:
			ret = ssd_set_msglevel_hdlr();
			break;
		case SSD_CMD_SOFTBLOCK_CLEAR:
			ret = ssd_clear_softblock_hdlr();
			break;
		case SSD_CMD_SOFTBLOCK_LIST:
			ret = ssd_list_softblock_hdlr();
			break;
		default :
			SSD_PRINT_ERROR("Invalid  SSD command ID %d\n", cmd_id);
			ret = SSD_FAIL;
			break;
	}
	return ret;
}

/*
 * Receives and processes the commands from client
 *   Wait for connection from client
 *   Process the command
 *   close connection with client
 */
static int
ssd_daemon_proc_cli_req(void)
{
	int ret = SSD_OK;
	int fd = -1;
	uint8 cmd_id;
	struct sockaddr_in cliaddr;
	socklen_t len; /* need initialize here to avoid EINVAL */

	len = sizeof(cliaddr);
	fd = accept(cli_listenfd, (struct sockaddr *)&cliaddr, &len);
	if (fd < 0) {
		if (errno == EINTR)
			return 0;
		else {
			SSD_PRINT_ERROR("accept failed: errno: %d - %s\n", errno, strerror(errno));
			return -1;
		}
	}
	/* get command from client */
	if (read(fd, &(cmd_id), sizeof(cmd_id)) < 0) {
		SSD_PRINT_ERROR("Failed reading message from client: %s\n", strerror(errno));
		ret = SSD_FAIL;
		goto done;
	}

	SSD_PRINT_INFO("daemon receive cmd %d via cli_listenfd %d\n",
		 cmd_id, cli_listenfd);

	/* Process the cli request */
	ret = ssd_proc_clicmd(cmd_id);

	/* send back result */
	SSD_PRINT_INFO("daemon send ret %d to cli via socket %d\n", ret, fd);
	if (write(fd, &ret, sizeof(ret)) < 0) {
		SSD_PRINT_ERROR("Failed to send ret to cli: %s\n", strerror(errno));
		ret = SSD_FAIL;
	}

done:
	ssd_close_socket(&fd);
	return ret;
}

/* Open a TCP socket for getting requests from client */
int
ssd_daemon_cli_socket_init(void)
{
	int sockfd = SSD_DEFAULT_FD, optval = 1;
	struct sockaddr_in sockaddr;
	int cli_port = EAPD_WKSP_SSD_CLI_PORT;

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sockaddr.sin_port = htons(cli_port);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		SSD_PRINT_ERROR("portno[%d]. socket error is : %s\n", cli_port, strerror(errno));
		goto error;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		SSD_PRINT_ERROR("sockfd[%d] portno[%d]. setsockopt error is : %s\n",
				sockfd, cli_port, strerror(errno));
		goto error;
	}

	if (bind(sockfd, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
		SSD_PRINT_ERROR("sockfd[%d] portno[%d]. bind error is : %s\n", sockfd, cli_port,
			strerror(errno));
		goto error;
	}

	if (listen(sockfd, 10) < 0) {
		SSD_PRINT_ERROR("sockfd[%d] portno[%d]. listen error is : %s\n", sockfd, cli_port,
			strerror(errno));
		goto error;
	}
	return sockfd;

error:
	ssd_close_socket(&sockfd);
	return SSD_DEFAULT_FD;
}

/* cli side:
 * connect_to_server() - Establish a TCP connection to the SSD server command port.
 * On success, the context socket is updated and SSD_OK is returned.
 */

/*
 * set up and send a command to the server, read and process the response.
 */
static int
ssd_cli_send_msg(uint8 cmd_id)
{
	int ret = SSD_OK;

	if (cli_sendfd < 0) {
		SSD_PRINT_ERROR("cli_sendfd not ready\n");
		return SSD_FAIL;
	}

	/* Send it */
	if (write(cli_sendfd, &cmd_id,  sizeof(cmd_id)) < 0) {
		SSD_PRINT_ERROR("Failed to send command to server: %s\n", strerror(errno));
		return SSD_FAIL;
	}

	return ret;
}

static int
ssd_connect_to_server(void)
{
	int sock = SSD_DEFAULT_FD;
	struct sockaddr_in sockaddr;
	int cli_port = EAPD_WKSP_SSD_CLI_PORT;

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = inet_addr(SSD_DEFAULT_SERVER_HOST);
	sockaddr.sin_port = htons(cli_port);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		SSD_PRINT_ERROR("portno[%d]. socket error is : %s\n", cli_port, strerror(errno));
		goto error;
	}
	if (connect(sock, (const struct sockaddr *)&sockaddr,	sizeof(sockaddr)) < 0) {
		SSD_PRINT_ERROR("Could not connect to %s port %d.\n",
			SSD_DEFAULT_SERVER_HOST, cli_port);
		goto error;
	}

	cli_sendfd = sock;
	return SSD_OK;
error:
	ssd_close_socket(&sock);
	return SSD_DEFAULT_FD;
}

/* Open a TCP socket as client for sending requests to server */
int
ssd_cli_msg_to_daemon(uint8 cmd_id)
{
	int ret = SSD_OK;

	ret = ssd_connect_to_server();
	if (ret == SSD_OK) {
		ret = ssd_cli_send_msg(cmd_id);
	}
	return ret;
}

int
main(int argc, char **argv)
{
	char filename[128], cmd[256];
	int c, sock, ret, bytes;

	/* support some cli */
	if (argc > 1) {
		while ((c = getopt(argc, argv, "dclhH")) != -1) {
			switch (c) {
				case 'd':
					ssd_cli_msg_to_daemon(SSD_CMD_SET_MSGLEVEL);
					break;
				case 'c':
					ssd_cli_msg_to_daemon(SSD_CMD_SOFTBLOCK_CLEAR);
					break;
				case 'l':
					snprintf(filename, sizeof(filename), "%s",
						SSD_SOFTBLOCK_LIST_FILE);
					unlink(filename);
					ssd_cli_msg_to_daemon(SSD_CMD_SOFTBLOCK_LIST);
					break;
				case 'h':
				case 'H':
					ssd_usage();
					return SSD_OK;
				default:
					printf("%s invalid option\n", argv[0]);
					return SSD_FAIL;
			}

			/* read reply from daemon */
			bytes = read(cli_sendfd, &(ret), sizeof(ret));
			if (bytes <= 0) {
				printf("SSD_CLI: Fail to read response from daemon via %d\n", cli_sendfd);
				ret = SSD_FAIL;
			}

			if (ret != SSD_OK) {
				printf("SSD_CLI: not successful\n");
			}

			if (c == 'l') {
				if (ret == SSD_OK) {
					snprintf(cmd, sizeof(cmd), "cat %s", filename);
					system(cmd);
				} else {
					printf("SSD_CLI: softblock list is not ready\n");
				}
			}

			shutdown(cli_sendfd, SHUT_WR);
			ssd_close_socket(&(cli_sendfd));
			return ret;
		}
	}

	/* UDP socket to eapd init */
	if ((sock = ssd_eapd_socket_init()) < 0) {
		SSD_PRINT_ERROR("Err: fail to init socket\n");
		return sock;
	}

	/* init socket in daemon to listen message from cli "ssd_cli" */
	if ((cli_listenfd = ssd_daemon_cli_socket_init()) < 0) {
		SSD_PRINT_ERROR("Err: fail to init cli socket\n");
		return SSD_FAIL;
	}

	/* receive wl event from ssd-eap via UDP */
	ssd_main_loop(sock);

	close(sock);
	ssd_close_socket(&cli_listenfd);

	return 0;
}
