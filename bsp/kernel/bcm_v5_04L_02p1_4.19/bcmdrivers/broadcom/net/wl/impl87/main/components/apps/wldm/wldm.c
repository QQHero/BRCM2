/*
 * Broadband Forum TR181 Device Data Model Wireless Utility
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>		/* usleep */
#include "wldm_lib_tr181.h"

#define WAIT_TIME_MS		1500
extern int wldm_init(int radios);

static void
usage(char *appname)
{
	printf("%s get|set|add|del <PATHNAME> [value]|"
		"[param1=value1 param2=value2 ...]\n", appname);
	printf("Get examples:\n");
	printf("\t%s get Device.WiFi.Radio.1.\n", appname);
	printf("\t%s get Device.WiFi.Radio.1.Enable\n", appname);
	printf("\t%s get Device.WiFi.Radio.1. "
		"Enable SupportedFrequencyBands OperatingStandards\n", appname);
	printf("\t%s get Device.WiFi.SSID.\n", appname);
	printf("Set examples:\n");
	printf("\t%s set Device.WiFi.Radio.1.Enable true\n", appname);
	printf("\t%s set Device.WiFi.Radio.1. Enable=true Channel=1\n", appname);
	printf("Add/Del Entry examples:\n");
	printf("\t%s add Device.WiFi.SSID.3. "
		"LowerLayers=Device.WiFi.Radio.1. SSID=\"New SSID\"\n", appname);
	printf("\t%s add Device.WiFi.AccessPoint.3. SSIDReference=Device.WiFi.SSID.3.\n", appname);
	printf("\t%s del Device.WiFi.AccessPoint.3.\n", appname);
	printf("\t%s del Device.WiFi.SSID.3.\n", appname);
}

static char input_buf[2048];

static int
convert_args(int argc, char **argv, char *outbuf, int outbuf_sz)
{
	int len, buf_size = outbuf_sz;

	/* Convert to '\n' separated string */
	while ((argc > 0) && (buf_size > 0)) {
		len = strlen(*argv);
		if (len + 2 > buf_size) /* 2 for '\n' and '\0' */
			return -1;
		len = snprintf(outbuf, buf_size, "%s\n", *argv++);
		outbuf += len;
		buf_size -= len;
		argc--;
	}
	return (outbuf_sz - buf_size);
}

int
main(int argc, char *argv[])
{
	int ret = 0;
	char *command;

	if (argc < 3) {
		usage(argv[0]);
		exit(1);
	}

	if (convert_args(argc - 2, &argv[2], input_buf, sizeof(input_buf)) < 0) {
		fprintf(stderr, "%s: failed to convert arguments!", argv[0]);
		exit(1);
	}

	wldm_init(-1);
	command = argv[1];
	if (!strcmp(command, "get")) {
		/* TR069 GetParameterValues */
		ret = wldm_get(input_buf, NULL, 0);
	} else if (!strcmp(command, "set")) {
		/* TR069 SetParameterValues */
		ret = wldm_set(input_buf, NULL, 0);
		if (ret == 0)
			usleep(WAIT_TIME_MS * 1000);
	} else if (!strcmp(command, "list")) {
		/* TR069 GetParameterNames */
		ret = wldm_list(input_buf, NULL, 0);
	} else if (!strcmp(command, "add")) {
		fprintf(stderr, "%s not supported yet!\n", command);
	} else if (!strcmp(command, "del")) {
		fprintf(stderr, "%s not supported yet!\n", command);
	} else {
		fprintf(stderr, "%s: %s not supported!", argv[0], command);
		usage(argv[0]);
		exit(1);
	}
	return ret;
}

/* END */
