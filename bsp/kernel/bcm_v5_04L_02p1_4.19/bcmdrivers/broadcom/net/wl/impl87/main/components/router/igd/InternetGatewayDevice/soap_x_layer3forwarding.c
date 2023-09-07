/*
 * Broadcom IGD module, soap_x_layer3forwarding.c
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
 * $Id: soap_x_layer3forwarding.c 738255 2017-12-27 22:36:47Z $
 */
#include <upnp.h>
#include <InternetGatewayDevice.h>

/*
 * WARNNING: PLEASE IMPLEMENT YOUR CODES AFTER
 *          "<< USER CODE START >>"
 * AND DON'T REMOVE TAG :
 *          "<< AUTO GENERATED FUNCTION: "
 *          ">> AUTO GENERATED FUNCTION"
 *          "<< USER CODE START >>"
 */

/* << AUTO GENERATED FUNCTION: statevar_DefaultConnectionService() */
static int
statevar_DefaultConnectionService
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE * service,
	UPNP_TLV * tlv
)
{
	HINT(STR::tlv)

	/* << USER CODE START >> */
	UPNP_SERVICE *wan_service = 0;
	UPNP_ADVERTISE *wan_advertise = 0;
	char *buf = upnp_buffer(context);

	wan_service = upnp_get_service_by_control_url(context, "/control?WANIPConnection");
	if (!wan_service)
		return SOAP_ACTION_FAILED;

	wan_advertise = upnp_get_advertise_by_name(context, wan_service->name);
	if (!wan_advertise)
		return SOAP_ACTION_FAILED;

	/* Construct out string */
	sprintf(buf, "uuid:%s:WANConnectionDevice:1,urn:upnp-org:serviceId:%s",
		wan_advertise->uuid,
		wan_service->service_id);

	/* Update tlv */
	upnp_tlv_set(tlv, (uintptr_t)buf);
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_SetDefaultConnectionService() */
static int
action_SetDefaultConnectionService
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR:UPNP_TLV *in_NewDefaultConnectionService = UPNP_IN_TLV("NewDefaultConnectionService"); )

	/* << USER CODE START >> */
	/* modify default connection service */
	return OK;
}
/* >> AUTO GENERATED FUNCTION */

/* << AUTO GENERATED FUNCTION: action_GetDefaultConnectionService() */
static int
action_GetDefaultConnectionService
(
	UPNP_CONTEXT * context,
	UPNP_SERVICE *service,
	IN_ARGUMENT *in_argument,
	OUT_ARGUMENT *out_argument
)
{
	HINT( STR::UPNP_TLV *out_NewDefaultConnectionService = UPNP_OUT_TLV("NewDefaultConnectionService"); )

	/* << USER CODE START >> */
	UPNP_TLV *out_NewDefaultConnectionService = UPNP_OUT_TLV("NewDefaultConnectionService");

	return statevar_DefaultConnectionService(context, service, out_NewDefaultConnectionService);
}
/* >> AUTO GENERATED FUNCTION */

/* << TABLE BEGIN */
/*
 * WARNNING: DON'T MODIFY THE FOLLOWING TABLES
 * AND DON'T REMOVE TAG :
 *          "<< TABLE BEGIN"
 *          ">> TABLE END"
 */

#define	STATEVAR_DEFAULTCONNECTIONSERVICE   0

/* State Variable Table */
UPNP_STATE_VAR statevar_x_layer3forwarding[] =
{
	{0, "DefaultConnectionService", UPNP_TYPE_STR,  &statevar_DefaultConnectionService, 1},
	{0, 0,                          0,              0,                                  0}
};

/* Action Table */
static ACTION_ARGUMENT arg_in_SetDefaultConnectionService [] =
{
	{"NewDefaultConnectionService", UPNP_TYPE_STR,  STATEVAR_DEFAULTCONNECTIONSERVICE}
};

static ACTION_ARGUMENT arg_out_GetDefaultConnectionService [] =
{
	{"NewDefaultConnectionService", UPNP_TYPE_STR,  STATEVAR_DEFAULTCONNECTIONSERVICE}
};

UPNP_ACTION action_x_layer3forwarding[] =
{
	{"GetDefaultConnectionService", 0,  0,                                  1,  arg_out_GetDefaultConnectionService,    &action_GetDefaultConnectionService},
	{"SetDefaultConnectionService", 1,  arg_in_SetDefaultConnectionService, 0,  0,                                      &action_SetDefaultConnectionService},
	{0,                             0,  0,                                  0,  0,                                      0}
};
/* >> TABLE END */
