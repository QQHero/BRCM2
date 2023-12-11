/*
 * Broadcom IGD module, InternetGatewayDevice_table.c
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
 * $Id: InternetGatewayDevice_table.c 241182 2011-02-17 21:50:03Z $
 */
#include <upnp.h>
#include <InternetGatewayDevice.h>

/* << TABLE BEGIN */
/*
 * WARNNING: DON'T MODIFY THE FOLLOWING TABLES
 * AND DON'T REMOVE TAG :
 *          "<< TABLE BEGIN"
 *          ">> TABLE END"
 */

extern UPNP_ACTION action_x_layer3forwarding[];
extern UPNP_ACTION action_x_wancommoninterfaceconfig[];
extern UPNP_ACTION action_x_wanipconnection[];

extern UPNP_STATE_VAR statevar_x_layer3forwarding[];
extern UPNP_STATE_VAR statevar_x_wancommoninterfaceconfig[];
extern UPNP_STATE_VAR statevar_x_wanipconnection[];

static UPNP_SERVICE InternetGatewayDevice_service [] =
{
	{"/control?Layer3Forwarding",		"/event?Layer3Forwarding",		"urn:schemas-upnp-org:service:Layer3Forwarding",	"L3Forwarding1",	action_x_layer3forwarding,		statevar_x_layer3forwarding},
	{"/control?WANCommonInterfaceConfig",	"/event?WANCommonInterfaceConfig",	"urn:schemas-upnp-org:service:WANCommonInterfaceConfig","WANCommonIFC1",	action_x_wancommoninterfaceconfig,	statevar_x_wancommoninterfaceconfig},
	{"/control?WANIPConnection",		"/event?WANIPConnection",		"urn:schemas-upnp-org:service:WANIPConnection",		"WANIPConn1",		action_x_wanipconnection,		statevar_x_wanipconnection},
	{0,					0,					0,							0,			0,					0}
};

static UPNP_ADVERTISE InternetGatewayDevice_advertise [] =
{
	{"urn:schemas-upnp-org:device:InternetGatewayDevice",		"eb9ab5b2-981c-4401-a20e-b7bcde359dbb", ADVERTISE_ROOTDEVICE},
	{"urn:schemas-upnp-org:service:Layer3Forwarding",		"eb9ab5b2-981c-4401-a20e-b7bcde359dbb",	ADVERTISE_SERVICE},
	{"urn:schemas-upnp-org:device:WANDevice",			"e1f05c9d-3034-4e4c-af82-17cdfbdcc077", ADVERTISE_DEVICE},
	{"urn:schemas-upnp-org:service:WANCommonInterfaceConfig",	"e1f05c9d-3034-4e4c-af82-17cdfbdcc077",	ADVERTISE_SERVICE},
	{"urn:schemas-upnp-org:device:WANConnectionDevice",		"1995cf2d-d4b1-4fdb-bf84-8e59d2066198", ADVERTISE_DEVICE},
	{"urn:schemas-upnp-org:service:WANIPConnection",		"1995cf2d-d4b1-4fdb-bf84-8e59d2066198",	ADVERTISE_SERVICE},
	{0,															""}
};

extern char xml_InternetGatewayDevice[];
extern char xml_x_layer3forwarding[];
extern char xml_x_wancommoninterfaceconfig[];
extern char xml_x_wanipconnection[];

/* FIXME, Should have an icon file to send out.
 * Plan to do with flex/yacc
 */
static UPNP_DESCRIPTION InternetGatewayDevice_description [] =
{
	{"/InternetGatewayDevice.xml",          "text/xml",     0,      xml_InternetGatewayDevice},
	{"/x_layer3forwarding.xml",             "text/xml",     0,      xml_x_layer3forwarding},
	{"/x_wancommoninterfaceconfig.xml",     "text/xml",     0,      xml_x_wancommoninterfaceconfig},
	{"/x_wanipconnection.xml",              "text/xml",     0,      xml_x_wanipconnection},
	{0,                                     0,              0,      0}
};

UPNP_DEVICE InternetGatewayDevice =
{
	"InternetGatewayDevice.xml",
	InternetGatewayDevice_service,
	InternetGatewayDevice_advertise,
	InternetGatewayDevice_description,
	InternetGatewayDevice_open,
	InternetGatewayDevice_close,
	InternetGatewayDevice_timeout,
	InternetGatewayDevice_notify
};
/* >> TABLE END */
