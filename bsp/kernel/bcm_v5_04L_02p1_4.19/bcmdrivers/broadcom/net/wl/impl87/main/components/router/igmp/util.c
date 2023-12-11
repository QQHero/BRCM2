/*
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
 * $Id: util.c 710512 2017-07-13 03:54:07Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#ifdef LINUX_INET
#include <linux/in.h>
#endif
#include <linux/mroute.h>

#include "util.h"

#include "igmp.h"

int             log_level;
extern char     upstream_interface[10][IFNAMSIZ+1];

void
wait_for_interfaces()
{
	int             fd = 0;
	struct ifreq    iface;
	int             i = 0;
	int             try = 0;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return;
	for (i = 0; i < 10; i++) {
		if (strlen(upstream_interface[i]) > 1) {
			strncpy(iface.ifr_name, upstream_interface[i], IFNAMSIZ);
			upstream_interface[i][IFNAMSIZ] = '\0';
			IGMP_DBG("Wait for upstream interface %s to go up \n", iface.ifr_name);
			while ((ioctl(fd, SIOCGIFADDR, &iface) < 0) && (try < 6)) {
					while (sleep(1) > 0);
					try++;
			} // sleep for 6 sec max.
		}
	}
	IGMP_DBG("upstream interface up after try=%d\n", try);
	close(fd);
}

int
upstream_interface_lookup(char *s)
{
	int             i;

	for (i = 0; i < 10; i++)
		if (strcmp(s, upstream_interface[i]) == 0)
			return 1;

	return 0;
}

/*
 * Set/reset the IP_MULTICAST_LOOP. Set/reset is specified by "flag".
 */
void
k_set_loop(socket, flag)
int             socket;
int             flag;
{
	u_char          loop;

	loop = flag;
	if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_LOOP,
				   (char *) &loop, sizeof(loop)) < 0)
		printf("setsockopt IP_MULTICAST_LOOP %u\n", loop);
}

/*
 * Set the IP_MULTICAST_IF option on local interface ifa.
 */
void
k_set_if(socket, ifa)
int             socket;
u_long          ifa;
{
	struct in_addr  adr;

	adr.s_addr = ifa;
	if (setsockopt(socket, IPPROTO_IP, IP_MULTICAST_IF,
				   (char *) &adr, sizeof(adr)) < 0)
		printf("ERROR in setsockopt IP_MULTICAST_IF \n");
}

/*
 * void debug(int level)
 *
 * Write to stdout
 */
void
debug(int level, const char *fmt, ...)
{
	va_list         args;

	if (level < log_level)
		return;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

/*
 * u_short in_cksum(u_short *addr, int len)
 *
 * Compute the inet checksum
 */
unsigned short
in_cksum(unsigned short *addr, int len)
{
	int             nleft = len;
	int             sum = 0;
	unsigned short *w = addr;
	unsigned short  answer = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	if (nleft == 1) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	answer = ~sum;
	return (answer);
}

/*
 * interface_list_t* get_interface_list(short af, short flags, short unflags)
 *
 * Get the list of interfaces with an address from family af, and whose flags
 * match 'flags' and don't match 'unflags'.
 */
interface_list_t *
get_interface_list(short af, short flags, short unflags)
{
	interface_list_t *ifp,
					 *ifprev,
					 *list;
	struct sockaddr *psa;
	struct ifreq    ifr;
	int             sockfd;
	int             ifindex, err;
	char proc_net_dev[] = "/proc/net/dev";
	FILE *fp;
	char buf[256], *c, *name;

	if (!(fp = fopen(proc_net_dev, "r")))
		return NULL;

	/* eat first two lines */
	if (!fgets(buf, sizeof(buf), fp) ||
		!fgets(buf, sizeof(buf), fp)) {
		fclose(fp);
		return NULL;
	}

	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		fclose(fp);
		return NULL;
	}

	list = ifp = ifprev = NULL;

	while (fgets(buf, sizeof(buf), fp)) {
		c = buf;
		while (isspace(*c))
			c++;
		if (!(name = strsep(&c, ":")))
			continue;

		strncpy(ifr.ifr_name, name, IFNAMSIZ-1);
		ifr.ifr_name[IFNAMSIZ-1] = 0;

		if (ioctl(sockfd, SIOCGIFINDEX, &ifr))
			continue;
		ifindex = ifr.ifr_ifindex;

		/*
		 * skip the one user did not enable from webUI
		 */
		if (upstream_interface_lookup(name) == 0 && strncmp(name, "br", 2) != 0)
			// if ( upstream_interface_lookup(p) == 0)
			continue;

		err = ioctl(sockfd, SIOCGIFADDR, (void *) &ifr);
		if (err < 0)
			continue;
		psa = &ifr.ifr_ifru.ifru_addr;
		// eddie
		ifp = (interface_list_t *) malloc(sizeof(*ifp));
		if (ifp) {
			ifp->ifl_ifindex = ifindex;
			IGMP_DBG("ifp->ifl_ifindex=%d name=%s\n",
				ifp->ifl_ifindex, ifr.ifr_name);
			strncpy(ifp->ifl_name, ifr.ifr_name, IFNAMSIZ);
			memcpy(&ifp->ifl_addr, psa, sizeof(*psa));
			ifp->ifl_next = NULL;
			if (list == NULL)
				list = ifp;
			if (ifprev != NULL)
				ifprev->ifl_next = ifp;
			ifprev = ifp;
		}
	}

	fclose(fp);
	close(sockfd);
	return list;
}

/*
 * void free_interface_list(interface_list_t *ifl)
 *
 * Free a list of interfaces
 */
void
free_interface_list(interface_list_t * ifl)
{
	interface_list_t *ifp = ifl;

	while (ifp) {
		ifl = ifp;
		ifp = ifp->ifl_next;
		free(ifl);
	}
}

/*
 * short get_interface_flags(char *ifname)
 *
 * Get the value of the flags for a certain interface
 */
short
get_interface_flags(char *ifname)
{
	struct ifreq    ifr;
	int             sockfd,
					err;

	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
		return -1;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	err = ioctl(sockfd, SIOCGIFFLAGS, (void *) &ifr);
	close(sockfd);
	if (err == -1)
		return -1;
	return ifr.ifr_flags;
}

/*
 * short set_interface_flags(char *ifname, short flags)
 *
 * Set the value of the flags for a certain interface
 */
short
set_interface_flags(char *ifname, short flags)
{
	struct ifreq    ifr;
	int             sockfd,
					err;

	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
		return -1;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_flags = flags;
	err = ioctl(sockfd, SIOCSIFFLAGS, (void *) &ifr);
	close(sockfd);
	if (err == -1)
		return -1;
	return 0;
}

/*
 * short get_interface_flags(char *ifname)
 *
 * Get the value of the flags for a certain interface
 */
int
get_interface_mtu(char *ifname)
{
	struct ifreq    ifr;
	int             sockfd,
					err;

	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd <= 0)
		return -1;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	err = ioctl(sockfd, SIOCGIFMTU, (void *) &ifr);
	close(sockfd);
	if (err == -1)
		return -1;
	return ifr.ifr_mtu;
}

/*
 * int mrouter_onoff(int sockfd, int onoff)
 *
 * Tell the kernel if a multicast router is on or off
 */
int
mrouter_onoff(int sockfd, int onoff)
{
	int             err,
					cmd,
					i;

	cmd = (onoff) ? MRT_INIT : MRT_DONE;
	i = 1;
	err = setsockopt(sockfd, IPPROTO_IP, cmd, (void *) &i, sizeof(i));
	return err;
}
/* FILE-CSTYLED */
