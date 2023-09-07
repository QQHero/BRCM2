/*
 * Broadcom IGD main loop
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
 * $Id: igd_mainloop.c 551827 2015-04-24 08:59:54Z $
 */
#include <upnp.h>
#include <InternetGatewayDevice.h>
#include <igd_mainloop.h>

int igd_flag;

/* Dispatch UPnP incoming messages. */
int
igd_mainloop_dispatch(UPNP_CONTEXT *context)
{
	struct timeval tv = {1, 0};    /* timed out every second */
	int n;
	int max_fd;
	fd_set  fds;

	FD_ZERO(&fds);

	/* Set select sockets */
	max_fd = upnp_fdset(context, &fds);

	/* select sockets */
	n = select(max_fd+1, &fds, (fd_set *)NULL, (fd_set *)NULL, &tv);
	if (n > 0) {
		upnp_dispatch(context, &fds);
	}

	upnp_timeout(context);

	return 0;
}

/*
 * Initialize igd
 */
static UPNP_CONTEXT *
igd_mainloop_init(int igd_port, int igd_adv_time, IGD_NET *netlist)
{
	int i;
	char **lan_ifnamelist;
	UPNP_CONTEXT *context;
	char random_buf[UPNP_URL_UUID_LEN + 1];
	char *randomstring = NULL;

	if (!upnp_get_url_randomstring(random_buf)) {
		randomstring = random_buf;
	}
	/* initialize config to default values */
	 if (igd_port <= 0 || igd_port > 65535)
		igd_port = 1980;

	if (igd_adv_time <= 0) {
		/* ssdp alive notify every 10 minutes */
		igd_adv_time = 600;
	}

	/* Get net count */
	for (i = 0; netlist[i].lan_ifname; i++)
		;

	lan_ifnamelist = (char **)calloc(i + 1, sizeof(char *));
	if (lan_ifnamelist == NULL)
		return NULL;

	/* Setup lan ifname list */
	for (i = 0; netlist[i].lan_ifname; i++)
		lan_ifnamelist[i] = netlist[i].lan_ifname;

	InternetGatewayDevice.private = (void *)netlist;

	/* Init upnp context */
	context = upnp_init(igd_port, igd_adv_time, lan_ifnamelist,
		&InternetGatewayDevice, randomstring);

	/* Clean up */
	free(lan_ifnamelist);
	return context;
}

/*
 * UPnP portable main loop.
 * It initializes the UPnP protocol and event variables.
 * And loop handler the UPnP incoming requests.
 */
int
igd_mainloop(int igd_port, int igd_adv_time, IGD_NET *netlist)
{
	UPNP_CONTEXT *context;

	/* initialize upnp */
	igd_flag = 0;

	/* init context */
	context = igd_mainloop_init(igd_port, igd_adv_time, netlist);
	if (context == NULL)
		return IGD_FLAG_SHUTDOWN;

	/* main loop */
	while (1) {
		switch (igd_flag) {
		case IGD_FLAG_SHUTDOWN:
			upnp_deinit(context);

			printf("IGD shutdown!\n");
			return IGD_FLAG_SHUTDOWN;

		case IGD_FLAG_RESTART:
			upnp_deinit(context);

			printf("IGD restart!\n");
			return IGD_FLAG_RESTART;

		case 0:
		default:
			igd_mainloop_dispatch(context);
			break;
		}
	}

	return igd_flag;
}

void
igd_stop_handler()
{
	igd_flag = IGD_FLAG_SHUTDOWN;
}

void
igd_restart_handler()
{
	igd_flag = IGD_FLAG_RESTART;
}
