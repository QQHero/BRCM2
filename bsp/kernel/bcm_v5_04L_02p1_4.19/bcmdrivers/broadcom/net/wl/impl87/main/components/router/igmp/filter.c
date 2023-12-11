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
 * $Id: filter.c 710512 2017-07-13 03:54:07Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bcmnvram.h>
#include <netconf.h>
#include "igmp.h"

/* FILE-CSTYLED */

void
mcast_filter_init(netconf_filter_t * filter, uint8 * ifname,
				  struct in_addr mgrp)
{
	memset(filter, 0, sizeof(netconf_filter_t));
	//filter->match.state = NETCONF_NEW;
	strncpy(filter->match.in.name, (char *)ifname, IFNAMSIZ);
	filter->match.src.netmask.s_addr = htonl(INADDR_ANY);
	filter->match.src.ipaddr.s_addr = htonl(INADDR_ANY);
	filter->match.dst.netmask.s_addr = htonl(INADDR_BROADCAST);
	filter->match.dst.ipaddr.s_addr = mgrp.s_addr;
	filter->target = NETCONF_ACCEPT;
	filter->dir = NETCONF_FORWARD;

	return;
}

void
mcast_filter_add(uint8 * ifname, struct in_addr mgrp_addr)
{
	netconf_filter_t filter;
	int             ret;

	IGMP_DBG("Adding filter on %s for multicast group %s\n",
			 ifname, inet_ntoa(mgrp_addr));

	mcast_filter_init(&filter, ifname, mgrp_addr);

	ret = netconf_add_filter(&filter);
	if (ret) {
		IGMP_DBG("Netconf couldn't add filter %d\n", ret);
		return;
	}

	return;
}

void
mcast_filter_del(uint8 * ifname, struct in_addr mgrp_addr)
{
	netconf_filter_t filter;
	int             ret;

	IGMP_DBG("Deleting filter on %s for multicast group %s\n",
			 ifname, inet_ntoa(mgrp_addr));

	mcast_filter_init(&filter, ifname, mgrp_addr);

	ret = netconf_del_filter(&filter);
	if (ret) {
		IGMP_DBG("Netconf couldn't delete filter %d\n", ret);
		return;
	}

	return;
}
