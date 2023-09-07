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
 * $Id: $
 */

#define WLDM_SERVER_IPADDR		INADDR_ANY
#define WLDM_SERVER_PORT		10181

/* TR069 fault codes */
#define TR069_INV_ARG			(9003) /* Invalid param names */
#define TR069_INV_PARAM_NAME		(9005) /* Invalid pathname/instance */
#define TR069_INV_PARAM_VALUE		(9007) /* Invalid param value(s) */

/* String sizes. See https://cwmp-data-models.broadband-forum.org/tr-181-2-5-0.html#H.Data%20Types
*/
#define TR181_STR_64			(64)
#define TR181_STR_ALIAS			TR181_STR_64
#define TR181_STR_256			(256)
#define TR181_STR_IPADDR		(45)	/* v4 and v6 */
#define TR181_STR_IPPREFIX		(49)
#define TR181_STR_IP4ADDR		(15)	/* v4 only */
#define TR181_STR_IP4PREFIX		(19)	/* v4 only */
#define TR181_STR_MACADDR		(17)

/*
*  Get the value(s) of the given pathname(cf. TR069 GetParameterValues).
*  Return TR069 fault code if invalid, or 0 if successful.
*
*  Example: wldm get Device.WiFi.Radio.1. Enable SupportedFrequencyBands OperatingStandards
*  The '\n' is used to delimit the pathname and parameters.
*     char inpbuf[] =
*        "Device.WiFi.Radio.1.\n"		//pathname
*        "Enable\n"				//param name
*        "SupportedFrequencyBands\n"
*        "OperatingStandards\n";
*
*     char outbuf[] will contain:
*        "Radio.1.\n"
*        "Enable=true\n"
*        "SupportedFrequencyBands=5GHz\n"
*        "OperatingStandards=ac\n";
*/
extern int wldm_get(char *inpbuf, char *outbuf, int outbuf_sz);

/*
*  List the parameter names of the given pathname(cf. TR069 GetParameterNames).
*  Return TR069 fault code if invalid, or 0 if successful.
*
*  Example: wldm list Device.WiFi.Radio.1. NextLevel=true
*  The '\n' is used to delimit the pathname and parameters.
*     char inpbuf[] =
*        "Device.WiFi.Radio.1.\n"		//param name
*        "NextLevel=true\n";
*
*     char outbuf[] will contain:
*        "Radio.1.\n"
*        "Enable\n"
*        "Status\n"
*	 ...;
*/
extern int wldm_list(char *inpbuf, char *outbuf, int outbuf_sz);

/*
*  Set the parameter names/values of the given pathname(cf. TR069 SetParameterValues).
*  Return TR069 fault code if invalid, or 0 if all validated and applied.
*
*  Example:
*     wldm set Device.WiFi.Radio.1. Enable=true SupportedFrequencyBands=5GHz OperatingStandards=ac
*  The '\n' is used to delimit the pathname and parameters.
*     char parambuf[] =
*        "Device.WiFi.Radio.1.\n"		//pathname name
*        "Enable=true\n"			//param name=value
*        "Alias=Broadcom 5GHz Wifi\n"		//Treat all chars between '=' and '\n' as value
*        "SupportedFrequencyBands=5GHz\n"
*        "OperatingStandards=ac\n";
*/
extern int wldm_set(char *inpbuf, char *outbuf, int outbuf_sz);
