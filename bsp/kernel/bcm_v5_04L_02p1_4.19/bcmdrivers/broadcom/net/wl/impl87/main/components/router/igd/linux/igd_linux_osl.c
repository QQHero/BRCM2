/*
 * Broadcom UPnP module linux OS dependent implementation
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
 * $Id: igd_linux_osl.c 692019 2017-03-24 19:24:28Z $
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>

#include <upnp.h>

#define _PATH_PROCNET_DEV           "/proc/net/dev"

#include <InternetGatewayDevice.h>
#include <igd_mainloop.h>

#define __KERNEL__
#include <asm/types.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include <shutils.h>

typedef struct _if_stats {
	unsigned long rx_packets;	/* total packets received       */
	unsigned long tx_packets;	/* total packets transmitted    */
	unsigned long rx_bytes;		/* total bytes received         */
	unsigned long tx_bytes;		/* total bytes transmitted      */
	unsigned long rx_errors;	/* bad packets received         */
	unsigned long tx_errors;	/* packet transmit problems     */
	unsigned long rx_dropped;	/* no space in linux buffers    */
	unsigned long tx_dropped;	/* no space available in linux  */
	unsigned long rx_multicast;	/* multicast packets received   */
	unsigned long rx_compressed;
	unsigned long tx_compressed;
	unsigned long collisions;

	/* detailed rx_errors: */
	unsigned long rx_length_errors;
	unsigned long rx_over_errors;	/* receiver ring buff overflow  */
	unsigned long rx_crc_errors;	/* recved pkt with crc error    */
	unsigned long rx_frame_errors;	/* recv'd frame alignment error */
	unsigned long rx_fifo_errors;	/* recv'r fifo overrun          */
	unsigned long rx_missed_errors;	/* receiver missed packet       */

	/* detailed tx_errors */
	unsigned long tx_aborted_errors;
	unsigned long tx_carrier_errors;
	unsigned long tx_fifo_errors;
	unsigned long tx_heartbeat_errors;
	unsigned long tx_window_errors;

} if_stats_t;
#undef __KERNEL__

/*
 * The functions below are required by the
 * upnp device, for example, InternetGatewayDevice.
 */
static char *
get_name(char *name, char *p)
{
	while (isspace(*p))
		p++;

	while (*p) {
		/* Eat white space */
		if (isspace(*p))
			break;

		/* could be an alias */
		if (*p == ':') {
			char *dot = p, *dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;

			/* it wasn't, backup */
			if (*p != ':') {
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;

			p++;
			break;
		}

		*name++ = *p++;
	}

	*name++ = '\0';
	return p;
}

static int
procnetdev_version(char *buf)
{
	if (strstr(buf, "compressed"))
		return 3;

	if (strstr(buf, "bytes"))
		return 2;

	return 1;
}

static int
get_dev_fields(char *bp, int versioninfo, if_stats_t *pstats)
{
	switch (versioninfo) {
	case 3:
		sscanf(bp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
			&pstats->rx_bytes,
			&pstats->rx_packets,
			&pstats->rx_errors,
			&pstats->rx_dropped,
			&pstats->rx_fifo_errors,
			&pstats->rx_frame_errors,
			&pstats->rx_compressed,
			&pstats->rx_multicast,
			&pstats->tx_bytes,
			&pstats->tx_packets,
			&pstats->tx_errors,
			&pstats->tx_dropped,
			&pstats->tx_fifo_errors,
			&pstats->collisions,
			&pstats->tx_carrier_errors,
			&pstats->tx_compressed);
		break;

	case 2:
		sscanf(bp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
			&pstats->rx_bytes,
			&pstats->rx_packets,
			&pstats->rx_errors,
			&pstats->rx_dropped,
			&pstats->rx_fifo_errors,
			&pstats->rx_frame_errors,
			&pstats->tx_bytes,
			&pstats->tx_packets,
			&pstats->tx_errors,
			&pstats->tx_dropped,
			&pstats->tx_fifo_errors,
			&pstats->collisions,
			&pstats->tx_carrier_errors);

		pstats->rx_multicast = 0;
		break;

	case 1:
		sscanf(bp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
			&pstats->rx_packets,
			&pstats->rx_errors,
			&pstats->rx_dropped,
			&pstats->rx_fifo_errors,
			&pstats->rx_frame_errors,
			&pstats->tx_packets,
			&pstats->tx_errors,
			&pstats->tx_dropped,
			&pstats->tx_fifo_errors,
			&pstats->collisions,
			&pstats->tx_carrier_errors);

		pstats->rx_bytes = 0;
		pstats->tx_bytes = 0;
		pstats->rx_multicast = 0;
		break;
	}

	return 0;
}

int
igd_osl_wan_ifstats(IGD_CTRL *igd_ctrl, igd_stats_t *pstats)
{
	if_stats_t stats;

	FILE *fh, *vfh;
	char buf[512], vlanx_path[64], vlanx_devname[IFNAMSIZ];
	int err;
	int procnetdev_vsn;  /* version information */
	char *stats_ifname = igd_ctrl->wan_ifname;

	if (strncmp(stats_ifname, "vlan", 4) == 0) {
		sprintf(vlanx_path, "/proc/net/vlan/%s", stats_ifname);

		vfh = fopen(vlanx_path, "r");
		if (!vfh) {
			fprintf(stderr, "Warning: cannot open %s (%s). Limited output.\n",
				vlanx_path, strerror(errno));
			return 0;
		}

		while (fgets(buf, sizeof(buf), vfh)) {
			char *s;
			char name[50];

			s = get_name(name, buf);
			if (strcmp(name, "Device") == 0) {
				sscanf(s, "%s",	vlanx_devname);
				stats_ifname = vlanx_devname;
				break;
			}
		}
		if (ferror(vfh)) {
			perror(vlanx_path);
			fclose(vfh);
			return -1;
		}
		fclose(vfh);
	}

	/*
	 * PR83501, delay getting statistics to make sure
	 * Microsoft WLK IGDBytesSent test packets
	 * have been forwarded to WAN side.
	 */
	usleep(20000);

	memset(pstats, 0, sizeof(*pstats));
	memset(&stats, 0, sizeof(stats));

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		fprintf(stderr, "Warning: cannot open %s (%s). Limited output.\n",
			_PATH_PROCNET_DEV, strerror(errno));
		return 0;
	}

	fgets(buf, sizeof(buf), fh);	/* eat line */
	fgets(buf, sizeof(buf), fh);

	procnetdev_vsn = procnetdev_version(buf);

	err = 0;
	while (fgets(buf, sizeof(buf), fh)) {
		char *s;
		char name[50];

		s = get_name(name, buf);
		if (strcmp(name, stats_ifname) == 0) {
			get_dev_fields(s, procnetdev_vsn, &stats);

			pstats->rx_packets = stats.rx_packets;
			pstats->tx_packets = stats.tx_packets;
			pstats->rx_bytes = stats.rx_bytes;
			pstats->tx_bytes = stats.tx_bytes;
			break;
		}
	}
	if (ferror(fh)) {
		perror(_PATH_PROCNET_DEV);
		err = -1;
	}
	fclose(fh);

	return err;
}

unsigned int
igd_osl_wan_max_bitrates(char *wan_devname, unsigned long *rx, unsigned long *tx)
{
	struct ethtool_cmd ecmd;
	struct ifreq ifr;
	int fd, err;
	long speed = 0;

	/* This would have problem of pppoe */

	/* Setup our control structures. */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, wan_devname);

	/* Open control socket. */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd >= 0) {
		ecmd.cmd = ETHTOOL_GSET;
		ifr.ifr_data = (caddr_t)&ecmd;

		err = ioctl(fd, SIOCETHTOOL, &ifr);
		if (err >= 0) {

			unsigned int mask = ecmd.supported;

			/* dump_ecmd(&ecmd); */
			if ((mask & (ADVERTISED_1000baseT_Half|ADVERTISED_1000baseT_Full))) {
				speed = (1000 * 1000000);
			}
			else if ((mask & (ADVERTISED_100baseT_Half|ADVERTISED_100baseT_Full))) {
				speed = (100 * 1000000);
			}
			else if ((mask & (ADVERTISED_10baseT_Half|ADVERTISED_10baseT_Full))) {
				speed = (10 * 1000000);
			}
			else {
				speed = 0;
			}
		}
		else {
			/* Cannot read, assume 100M */
			speed = 100 * 1000000;
		}

		/* close the control socket */
		close(fd);
	}
	else {
		/* Assume 100M */
		speed = 100 * 1000000;
	}

	*rx = *tx = speed;
	return TRUE;
}

int
igd_osl_wan_ip(char *wan_ifname, struct in_addr *inaddr)
{
	inaddr->s_addr = 0;

	if (upnp_osl_ifaddr(wan_ifname, inaddr) == 0)
		return 1;

	return 0;
}

int
igd_osl_wan_isup(char *wan_ifname)
{
	struct in_addr inaddr = {0};

	if (upnp_osl_ifaddr(wan_ifname, &inaddr) == 0) {
		/* Check ip address */
		if (inaddr.s_addr != 0)
			return 1;
	}

	return 0;
}

void
igd_osl_nat_config(char *wan_ifname, IGD_PORTMAP *map)
{
	void upnpnat(int argc, char **argv);

	char cmd[256];
	char *argv[32] = {0};
	char *name, *p, *next = "";
	int i;

	sprintf(cmd, "igdnat -i %s -eport %d -iport %d -en %d",
		wan_ifname,
		map->external_port,
		map->internal_port,
		map->enable);

	if (strlen(map->remote_host)) {
		strcat(cmd, " -remote ");
		strcat(cmd, map->remote_host);
	}

	if (strlen(map->protocol)) {
		strcat(cmd, " -proto ");
		strcat(cmd, map->protocol);
	}

	if (strlen(map->internal_client)) {
		strcat(cmd, " -client ");
		strcat(cmd, map->internal_client);
	}

	/* Seperate into argv[] */
	for (i = 0, name = cmd, p = name;
		name && name[0];
		name = next, p = 0, i++) {
		/* Get next token */
		strtok_r(p, " ", &next);
		argv[i] = name;
	}

	/* Run the upnpnat command */
	_eval(argv, ">/dev/console", 0, NULL);

	return;
}
