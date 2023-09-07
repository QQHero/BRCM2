/*
 * WBD BRCM Vendor operations at Master
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
 * $Id: wbd_master_vndr_brcm.c 802267 2021-08-20 09:59:45Z $
 */

#include <typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wbd.h"
#include "wbd_shared.h"
#include "ieee1905_vendor.h"
#include "wbd_master_vndr_brcm.h"
#include "wbd_master_com_hdlr.h"
#include "ieee1905_tlv.h"
#include "wbd_tlv.h"

#define WBD_I5_BRCM_VNDR_OUI  "00:10:18"

#ifdef WBD_BRCM_VNDR_STUB

/* Callback function which gets called when a message with registered message_type is to be sent */
static void
stub_wbd_master_vndr_brcm_add_vendor_tlvs_cb(const i5_message_types_t message_type,
	const unsigned char *dst_al_mac, void *vndr_ctxt, const void *pmsg, const void *reserved)
{
	const unsigned char *vendor_tlv_data = (const unsigned char*)"BRCM";
	unsigned int vendor_tlv_data_len = 4;

	WBD_INFO("Add BRCM vendor TLV for message %d Dest "MACF". stroui[%s]\n",
		message_type, ETHERP_TO_MACF(dst_al_mac), WBD_I5_BRCM_VNDR_OUI);
	/* Insert vendor specific TLV with broadcom OUI in a message which has
	 * message type(message_type)
	 */
	i5VendorTlvInsert(pmsg, WBD_I5_BRCM_VNDR_OUI, vendor_tlv_data, vendor_tlv_data_len);
}

/* Callback function which gets called when a message with registered message_type is received */
static void
stub_wbd_master_vndr_brcm_rcv_vndr_tlvs_cb(const i5_message_types_t message_type,
	const unsigned char *src_al_mac, void *vndr_ctxt, const unsigned char *tlvs,
	const unsigned int len)
{
	/* Received vendor specific TLV with Broadcom OUI */
	WBD_INFO("Recieved BRCM vendor TLV for message %d From "MACF"\n",
		message_type, ETHERP_TO_MACF(src_al_mac));
	wbd_hexdump_ascii("BRCM VNDR:", tlvs, len);
}
#endif /* WBD_BRCM_VNDR_STUB */

/* Insert chan set TLV in Channel Selection Request message */
static void
wbd_master_insert_vndr_chan_set_info_to_msg(const void *pmsg)
{
	int ret = WBDE_OK;
	wbd_info_t *info = wbd_get_ginfo();
	wbd_cmd_vndr_set_chan_lst_t *item = NULL;
	dll_t *item_p;
	unsigned char *vendorSpec_msg;
	unsigned int vendorSpec_len;

	/* Travese maclist items */
	foreach_glist_item(item_p, info->wbd_master->vndr_set_chan_list) {

		item = (wbd_cmd_vndr_set_chan_lst_t*)item_p;

		vendorSpec_msg = (unsigned char *)wbd_malloc((sizeof(item->vndr_set_chan) +
			sizeof(i5_tlv_t)), &ret);
		WBD_ASSERT();

		wbd_tlv_encode_msg(&item->vndr_set_chan, sizeof(item->vndr_set_chan),
			WBD_TLV_CHAN_SET_TYPE, vendorSpec_msg,
			&vendorSpec_len);

		/* Insert vendor specific TLV with broadcom OUI in a Channel Selection Request */
		i5VendorTlvInsert(pmsg, WBD_I5_BRCM_VNDR_OUI, vendorSpec_msg, vendorSpec_len);
		free(vendorSpec_msg);
	}

end:
	/* Remove channel selection vendor information and init */
	wbd_ds_glist_cleanup(&info->wbd_master->vndr_set_chan_list);
	wbd_ds_glist_init(&info->wbd_master->vndr_set_chan_list);
}

/* Insert vendor policy config TLV in Multi AP Policy Config Request Message */
static void
wbd_master_insert_vndr_policy_config(const unsigned char *dst_al_mac, const void *pmsg)
{
	int ret = WBDE_OK;
	ieee1905_vendor_data msg_data;

	memset(&msg_data, 0, sizeof(msg_data));

	/* Allocate Dynamic mem for Vendor data from Lib */
	msg_data.vendorSpec_msg = (unsigned char*)wbd_malloc(IEEE1905_MAX_VNDR_DATA_BUF, &ret);
	WBD_ASSERT();

	eacopy(dst_al_mac, msg_data.neighbor_al_mac);

	wbd_master_get_vndr_tlv_metric_policy(&msg_data);

	/* Insert Vendor Specific TLV Data, in Message */
	if (msg_data.vendorSpec_len > 0) {
		i5VendorTlvInsert(pmsg, WBD_I5_BRCM_VNDR_OUI, msg_data.vendorSpec_msg,
			msg_data.vendorSpec_len);
	}

	/* Free Dynamic mem for Vendor data */
	if (msg_data.vendorSpec_msg) {
		free(msg_data.vendorSpec_msg);
	}

end:
	return;
}

/* Callback function which gets called when a message with registered message_type is to be sent */
static void
wbd_master_vndr_brcm_add_vendor_tlvs_cb(const i5_message_types_t message_type,
	const unsigned char *dst_al_mac, void *vndr_ctxt, const void *pmsg, const void *reserved)
{
	WBD_DEBUG("Add BRCM vendor TLV in 0x%04x message while sending to "MACF" with oui[%s]\n",
		message_type, ETHERP_TO_MACF(dst_al_mac), WBD_I5_BRCM_VNDR_OUI);

	switch (message_type) {

		case i5MessageChannelSelectionRequestValue:
			wbd_master_insert_vndr_chan_set_info_to_msg(pmsg);
			break;

		case i5MessageMultiAPPolicyConfigRequestValue:
			wbd_master_insert_vndr_policy_config(dst_al_mac, pmsg);
			break;

		default:
			WBD_WARNING("Not registered message type[0x%04x] for adding vendor "
				"specific TLV.\n", message_type);
			break;
	}
}

/* Callback function which gets called when a message with registered message_type is received */
static void
wbd_master_vndr_brcm_rcv_vndr_tlvs_cb(const i5_message_types_t message_type,
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
	wbd_master_process_vendor_specific_msg(&vndr_msg_data);
}

/* Send broadcom vendor specific message */
int
wbd_master_send_brcm_vndr_msg(ieee1905_vendor_data *vndr_msg_data)
{
	return i5VendorMessageSend(vndr_msg_data->neighbor_al_mac, 0, WBD_I5_BRCM_VNDR_OUI,
		vndr_msg_data->vendorSpec_msg, vndr_msg_data->vendorSpec_len);
}

/* To register send/receive additional broadcom vendor specific TLVs in any controller messages */
void
wbd_master_vndr_brcm_register()
{
#ifdef WBD_BRCM_VNDR_STUB

	/* Register a callback to add vendor specific TLV in Topology Discovery Message */
	i5VendorTLVSendCbsRegister(i5MessageTopologyDiscoveryValue,
		stub_wbd_master_vndr_brcm_add_vendor_tlvs_cb, NULL);

	/* Register a callback to receive vendor specific TLV which has Broadcom OUI from
	 * Topology Discovery Message. Call the callback before processing any of the standard
	 * TLVs
	 */
	i5VendorTLVReceiveCbsRegister(i5MessageTopologyDiscoveryValue,
		I5_VNDR_RCV_FLAGS_CALL_CB_PRE_PROCESS, WBD_I5_BRCM_VNDR_OUI,
		stub_wbd_master_vndr_brcm_rcv_vndr_tlvs_cb, NULL);
#endif /* WBD_BRCM_VNDR_STUB */

	/* Register a callback to add vendor specific TLV in Channel Selection Request Message */
	i5VendorTLVSendCbsRegister(i5MessageChannelSelectionRequestValue,
		wbd_master_vndr_brcm_add_vendor_tlvs_cb, NULL);

	/* Register a callback to add vendor specific TLV in Multi AP Policy Config Request
	 * Message
	 */
	i5VendorTLVSendCbsRegister(i5MessageMultiAPPolicyConfigRequestValue,
		wbd_master_vndr_brcm_add_vendor_tlvs_cb, NULL);

	/* Register a callback to receive vendor specific TLV which has Broadcom OUI from
	 * vendor specific Message
	 */
	i5VendorTLVReceiveCbsRegister(i5MessageVendorSpecificValue,
		I5_VNDR_RCV_FLAGS_CALL_CB_PRE_PROCESS, WBD_I5_BRCM_VNDR_OUI,
		wbd_master_vndr_brcm_rcv_vndr_tlvs_cb, NULL);

	/* Register a callback to receive vendor specific TLV which has Broadcom OUI from
	 * Associated STA Link Metrics Response Message
	 */
	i5VendorTLVReceiveCbsRegister(i5MessageAssociatedSTALinkMetricsResponseValue,
		I5_VNDR_RCV_FLAGS_CALL_CB_PRE_PROCESS, WBD_I5_BRCM_VNDR_OUI,
		wbd_master_vndr_brcm_rcv_vndr_tlvs_cb, NULL);

	/* Register a callback to receive vendor specific TLV which has Broadcom OUI from
	 * AP Metrics Response Message
	 */
	i5VendorTLVReceiveCbsRegister(i5MessageAPMetricsResponseValue,
		I5_VNDR_RCV_FLAGS_CALL_CB_PRE_PROCESS, WBD_I5_BRCM_VNDR_OUI,
		wbd_master_vndr_brcm_rcv_vndr_tlvs_cb, NULL);
}
