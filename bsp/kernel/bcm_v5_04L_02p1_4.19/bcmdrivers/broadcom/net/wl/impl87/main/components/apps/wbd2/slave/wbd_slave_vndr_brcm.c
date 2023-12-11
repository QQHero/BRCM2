/*
 * WBD BRCM Vendor operations at Slave
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
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wbd_slave_vndr_brcm.c 802267 2021-08-20 09:59:45Z $
 */

#include <typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wbd.h"
#include "wbd_shared.h"
#include "ieee1905_vendor.h"
#include "wbd_slave_vndr_brcm.h"
#include "wbd_slave_com_hdlr.h"

#define WBD_I5_BRCM_VNDR_OUI  "00:10:18"

#ifdef WBD_BRCM_VNDR_STUB

/* Callback function which gets called when a message with registered message_type is to be sent */
static void
stub_wbd_slave_vndr_brcm_add_vendor_tlvs_cb(const i5_message_types_t message_type,
	const unsigned char *dst_al_mac, void *vndr_ctxt, const void *pmsg, const void *reserved)
{
	const unsigned char *vendor_tlv_data = (const unsigned char*)"BRCM";
	unsigned int vendor_tlv_data_len = 4;

	WBD_INFO("Add BRCM vendor TLV for message %d Dest "MACF". stroui[%s]\n",
		message_type, ETHERP_TO_MACF(dst_al_mac), WBD_I5_BRCM_VNDR_OUI);
	/* Insert vendor specific TLV with broadcom OUI */
	i5VendorTlvInsert(pmsg, WBD_I5_BRCM_VNDR_OUI, vendor_tlv_data, vendor_tlv_data_len);

	/* This is one example for sending a standalone vendor specific message with broadcom OUI */
	i5VendorMessageSend(dst_al_mac, 0, WBD_I5_BRCM_VNDR_OUI, vendor_tlv_data,
		vendor_tlv_data_len);
}
#endif /* WBD_BRCM_VNDR_STUB */

/* Callback function which gets called when a message with registered message_type is received */
static void
wbd_slave_vndr_brcm_rcv_vndr_tlvs_cb(const i5_message_types_t message_type,
	const unsigned char *src_al_mac, void *vndr_ctxt, const unsigned char *tlvs,
	const unsigned int len)
{
	ieee1905_vendor_data vndr_msg_data;

	memset(&vndr_msg_data, 0, sizeof(vndr_msg_data));

	/* Fill Destination AL_MAC in Vendor data */
	eacopy(src_al_mac, vndr_msg_data.neighbor_al_mac);
	vndr_msg_data.vendorSpec_msg = (unsigned char*)tlvs;
	vndr_msg_data.vendorSpec_len = (unsigned int)len;

	/* Received vendor specific TLV with Broadcom OUI */
	WBD_DEBUG("Recieved BRCM vendor TLVs in message 0x%04x From "MACF"\n",
		message_type, ETHERP_TO_MACF(src_al_mac));
	wbd_slave_process_vendor_specific_msg(wbd_get_ginfo(), &vndr_msg_data);
}

/* Send broadcom vendor specific message */
int
wbd_slave_send_brcm_vndr_msg(ieee1905_vendor_data *vndr_msg_data)
{
	return i5VendorMessageSend(vndr_msg_data->neighbor_al_mac, 0, WBD_I5_BRCM_VNDR_OUI,
		vndr_msg_data->vendorSpec_msg, vndr_msg_data->vendorSpec_len);
}

void
wbd_slave_vndr_brcm_register()
{
#ifdef WBD_BRCM_VNDR_STUB
	/* Register a callback to add vendor specific TLV in Topology Discovery Message */
	i5VendorTLVSendCbsRegister(i5MessageTopologyDiscoveryValue,
		stub_wbd_slave_vndr_brcm_add_vendor_tlvs_cb, NULL);
#endif /* WBD_BRCM_VNDR_STUB */

	/* Register a callback to receive vendor specific TLV which has Broadcom OUI from
	 * vendor specific Message
	 */
	i5VendorTLVReceiveCbsRegister(i5MessageVendorSpecificValue,
		I5_VNDR_RCV_FLAGS_CALL_CB_PRE_PROCESS, WBD_I5_BRCM_VNDR_OUI,
		wbd_slave_vndr_brcm_rcv_vndr_tlvs_cb, NULL);

	/* Register a callback to receive vendor specific TLV which has Broadcom OUI from
	 * Channel Selection Request Message
	 */
	i5VendorTLVReceiveCbsRegister(i5MessageChannelSelectionRequestValue,
		I5_VNDR_RCV_FLAGS_CALL_CB_PRE_PROCESS, WBD_I5_BRCM_VNDR_OUI,
		wbd_slave_vndr_brcm_rcv_vndr_tlvs_cb, NULL);

	/* Register a callback to receive vendor specific TLV which has Broadcom OUI from
	 * Multi AP Policy Config Request Message
	 */
	i5VendorTLVReceiveCbsRegister(i5MessageMultiAPPolicyConfigRequestValue,
		I5_VNDR_RCV_FLAGS_CALL_CB_PRE_PROCESS, WBD_I5_BRCM_VNDR_OUI,
		wbd_slave_vndr_brcm_rcv_vndr_tlvs_cb, NULL);

	/* Register a callback to receive vendor specific TLV which has Broadcom OUI from
	 * Client Steering Request Message
	 */
	i5VendorTLVReceiveCbsRegister(i5MessageClientSteeringRequestValue,
		I5_VNDR_RCV_FLAGS_CALL_CB_PRE_PROCESS, WBD_I5_BRCM_VNDR_OUI,
		wbd_slave_vndr_brcm_rcv_vndr_tlvs_cb, NULL);
}
