/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2013:proprietary:standard
 *
 *  This program is the proprietary software of Broadcom and/or its
 *  licensors, and may only be used, duplicated, modified or distributed pursuant
 *  to the terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied), right
 *  to use, or waiver of any kind with respect to the Software, and Broadcom
 *  expressly reserves all rights in and to the Software and all intellectual
 *  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1. This program, including its structure, sequence and organization,
 *     constitutes the valuable trade secrets of Broadcom, and you shall use
 *     all reasonable efforts to protect the confidentiality thereof, and to
 *     use this information only in connection with your use of Broadcom
 *     integrated circuit products.
 *
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *     PERFORMANCE OF THE SOFTWARE.
 *
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *     LIMITED REMEDY.
 * :>
 *
 * $Change: 116460 $
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include "i5api.h"
#include "ieee1905.h"
#include "ieee1905_glue.h"
#include "ieee1905_socket.h"
#include "ieee1905_datamodel_priv.h"
#include "ieee1905_trace.h"
#include "ieee1905_plc.h"
#include "ieee1905_wlcfg.h"
#include "ieee1905_flowmanager.h"
#include "ieee1905_json.h"
#include "i5ctl_wlcfg.h"
#include "ieee1905_security.h"
#include "ieee1905_udpsocket.h"
#include "ieee1905_message.h"
#include "i5ctl.h"
#include "ieee1905_wlmetric.h"
#include "ieee1905_cmsmdm.h"

#ifndef FD_COPY
#define FD_COPY(src,dest) memcpy((dest),(src),sizeof(dest))
#endif

#define I5_TRACE_MODULE i5TraceControl

#ifdef MULTIAP
static void i5ControlSendClientCapability(i5_socket_type *psock, t_I5_API_CLIENT_CAP_QUERY *pMsg,
  int cmd);
static void i5ControlSendHLData(i5_socket_type *psock, t_I5_API_HIGHER_LAYER_DATA *pMsg, int cmd);
static void i5ControlHLESendHLData(i5_socket_type *psock, void *data, int cmd);
static void i5ControlSendSteer(i5_socket_type *psock, t_I5_API_STEER *pMsg, int cmd);
static void i5ControlConfigListOpr(i5_socket_type *psock, t_I5_LIST_OPR *pMsg, int cmd);
static void i5ControlSteerConfigOpr(i5_socket_type *psock, t_I5_STEER_POLICY_CONFIG *pMsg, int cmd);
static void i5ControlSendClientAssocControl(i5_socket_type *psock, t_I5_API_ASSOC_CNTRL *pMsg,
  int cmd);
static void i5ControlSendFile(i5_socket_type *psock, t_I5_API_FILE *pMsg, int cmd);
static void i5ControlWPSOpr(i5_socket_type *psock, t_I5_API_WPS_CNTRL *pMsg, int cmd);
static void i5ControlSendApConfigRenew(i5_socket_type *psock, t_I5_API_RENEW_AP_CONFIG *pMsg,
  int cmd);
static void i5ControlSendBhSteer(i5_socket_type *psock, t_I5_API_BH_STEER *pMsg, int cmd);
static void i5ControlMetricConfigOpr(i5_socket_type *psock, t_I5_METRIC_RPT_POLICY_CONFIG *pMsg, int cmd);
static void i5ControlSendApMetricQuery(i5_socket_type *psock, t_I5_API_AP_METRIC_QUERY *pMsg,
  int cmd);
static void i5ControlSendAssociatedSTALinkMetricQuery(i5_socket_type *psock,
  t_I5_API_ASSOC_STA_LINK_METRIC_QUERY *pMsg, int cmd);
static void i5ControlSendUnAssociatedSTALinkMetricQuery(i5_socket_type *psock,
  t_I5_API_UNASSOC_STA_LINK_METRIC_QUERY *pMsg, int cmd);
static void i5ControlSendBeaconMetricQuery(i5_socket_type *psock, t_I5_API_BEACON_METRIC_QUERY *pMsg,
  int cmd);
static void i5ControlSendCAPI(i5_socket_type *psock, t_I5_API_FILE *pMsg, int cmd);
#if defined(MULTIAPR2)
static void i5ControlSendChannelScanRequest(i5_socket_type *psock, t_I5_API_CH_SCAN_REQ *pMsg);
static void i5ControlSendCACRequest(i5_socket_type *psock, t_I5_API_CAC_REQUEST *pMsg, int cmd);
static void i5ControlSendCACTermination(i5_socket_type *psock, t_I5_API_CAC_TERMINATION *pMsg, int cmd);
static void i5ControlAssocDisallowRequest(i5_socket_type *psock, t_I5_API_ASSOC_DISALLOW_REQ *pMsg);
#endif /* MULTIAPR2 */
#endif /* MULTIAP */

extern void i5VendorInformMessageSend(const unsigned char *dst_al_mac, const void *pmsg,
  const i5_message_types_t message_type, const void *reserved);

static void _i5ControlUnknownMessage(i5_socket_type *psock, t_I5_API_CMD_NAME cmd)
{
    i5apiSendMessage(psock->sd, cmd, 0, 0);
}

static void _i5ControlSetCmdRespSendMsgType(t_I5_CTL_CMDRESP_SEND_MSG_TYPE *response)
{
  memcpy(response->srcMacAddr, i5_config.i5_mac_address, MAC_ADDR_LEN);
  response->messageId = i5_config.last_message_identifier;
}

static void i5ControlDataModel(i5_socket_type *psock, int cmd)
{
  char *pMsgBuf;
  int   length;

  /* get size of DM and allocate memory */
  length  = i5DmCtlSizeGet();
  pMsgBuf = malloc(length);
  if ( pMsgBuf != NULL ) {
    /* send message back to requesting socket */
    i5DmLinkMetricsActivate();
    i5DmCtlRetrieve(pMsgBuf);
    i5apiSendMessage(psock->sd, cmd, pMsgBuf, length);
    free(pMsgBuf);
  }
}

static void i5ControlDataModelExtended(i5_socket_type *psock, int cmd)
{
  char *pMsgBuf = NULL;
  unsigned int length = 0;

  if ((pMsgBuf = i5DmGetExtendedDataModelBuffer(&length)) != NULL) {
    i5apiSendMessage(psock->sd, cmd, pMsgBuf, length);
    if (pMsgBuf) {
      free(pMsgBuf);
    }
  }
}

static void i5ControlAlMacAddress(i5_socket_type *psock, int cmd)
{
  /* get size of DM and allocate memory */
  char *pMsgBuf = malloc(MAC_ADDR_LEN);
  if ( pMsgBuf != NULL ) {
    /* send message back to requesting socket */
    i5DmCtlAlMacRetrieve(pMsgBuf);
    i5apiSendMessage(psock->sd, cmd, pMsgBuf, MAC_ADDR_LEN);
    free(pMsgBuf);
  }
}

#if defined(SUPPORT_HOMEPLUG)
static void i5ControlPlcHandler(t_I5_API_PLC_MSG *pMsg)
{
  if ( I5_API_PLC_UKE_START == pMsg->subcmd ) {
    i5PlcUKEStart();
  }
  else if ( I5_API_PLC_UKE_RANDOMIZE == pMsg->subcmd ) {
    i5PlcUKERandomize();
  }
  else {
    printf("Unknown PLC command %d\n", pMsg->subcmd);
  }
}
#endif

#ifndef MULTIAP
static void i5ControlJsonLegHandler(t_I5_API_JSON_LEG_MSG *pMsg)
{
  if (( I5_API_JSON_LEG_OFF == pMsg->subcmd ) || ( I5_API_JSON_LEG_ON == pMsg->subcmd )) {
    i5JsonConfigLegacyDisplay(I5_JSON_ALL_CLIENTS, pMsg->subcmd);
  }
  else {
    printf("Unknown JSON legacy command %d\n", pMsg->subcmd);
  }
}
#endif

#if defined(DMP_DEVICE2_IEEE1905BASELINE_1)
static void i5ControlFetchLinkMetrics (int sd, unsigned char *deviceId, unsigned char *remoteInterfaceId)
{
   const char blank[6] = {0};
   i5_dm_device_type* device = NULL;
   t_I5_API_CONFIG_GET_LINK_METRICS_REPLY rspData = {0};

   i5TraceInfo("Dev " I5_MAC_DELIM_FMT " RemIf " I5_MAC_DELIM_FMT " \n",
               I5_MAC_PRM(deviceId), I5_MAC_PRM(remoteInterfaceId) );

   if (memcmp(blank, deviceId, 6) == 0) {
      device = i5DmGetSelfDevice();
   }
   else {
      device = i5DmDeviceFind(deviceId);
   }
   if (device != NULL) {
      i5_dm_1905_neighbor_type *neighbor = i5Dm1905FindNeighborByRemoteInterface(device, remoteInterfaceId);
      if (NULL != neighbor) {
         rspData.linkAvailability = neighbor->availableThroughputCapacity;
         rspData.MacThroughputCapacity = neighbor->MacThroughputCapacity;
         rspData.packetErrors = 0;
         rspData.packetErrorsReceived = 0;
         rspData.transmittedPackets = 0;
         rspData.packetsReceived = 0;
         rspData.phyRate = 0;
         rspData.rcpi = 0;
      }
   }
   i5apiSendMessage(sd, I5_API_CMD_GET_CONFIG, &rspData, sizeof(rspData));
}
#endif

static void i5ControlLinkMetricIntervalHandler(t_I5_API_LINK_METRIC_INTERVAL *pMsg)
{
  i5DmSetLinkMetricInterval (pMsg->intervalMsec);
}

static void i5ControlSendBytesCommand (int length, t_I5_API_GOLDEN_NODE_SEND_MSG_BYTES *pmsg)
{
  unsigned int msgSize = length - sizeof (t_I5_API_MSG) - MAC_ADDR_LEN;
  i5MessageRawMessageSend(pmsg->macAddr, pmsg->message, msgSize);
}

static void i5ControlSendCommand(i5_socket_type *psock, t_I5_API_SEND_MESSAGE *pMsg, int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  switch (pMsg->msgInfo.messageId) {
    /* Type = 0 */
    case i5MessageTopologyDiscoveryValue:
      {
        i5_socket_type *bridgeSocket = i5SocketFindDevSocketByType(i5_socket_type_bridge_ll);
        if (NULL != bridgeSocket) {
          i5MessageTopologyDiscoverySend(bridgeSocket);
          _i5ControlSetCmdRespSendMsgType(&response);
        }
        break;
      }
    /* Type = 1 */
    case i5MessageTopologyNotificationValue:
      i5MessageTopologyNotificationSend(NULL, NULL, 0);
      _i5ControlSetCmdRespSendMsgType(&response);
      break;
    /* Type = 2 */
    case i5MessageTopologyQueryValue:
      {
        i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
        if (pDevice && pDevice->psock) {
          i5MessageRawTopologyQuerySend (pDevice->psock, pMsg->msgInfo.macAddr, 0 /* without retries */, i5MessageTopologyQueryValue);
          _i5ControlSetCmdRespSendMsgType(&response);
        } else {
          i5TraceInfo("i5MessageTopologyQueryValue : Neighbor AL MAC Address "
            I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
        }
        break;
      }
    case i5MessageTopologyResponseValue:
      i5TraceInfo("Unhandled i5MessageTopologyResponseValue\n");
      break;
    /* Type = 4 */
    case i5MessageVendorSpecificValue:
      /* I can do this */
      /* Hard code something something */
      i5TraceError("i5MessageVendorSpecificValue : TBD - spec has multiple results\n");
      break;
    /* Type = 5 */
    case i5MessageLinkMetricQueryValue:
      {
        unsigned char al_id[6];
	i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);

	if (pDevice && pDevice->psock) {
	  if (pMsg->data_len == 0) {
	    i5TraceInfo("Link Metric query: All neighbour\n");
	    i5MessageLinkMetricQuerySend(pDevice->psock, pMsg->msgInfo.macAddr, 0, NULL);
	  } else if (i5String2MacAddr((char *)pMsg->data, al_id)) {
	    i5TraceInfo("Link Metric query: Specific neighbour\n");
	    i5MessageLinkMetricQuerySend(pDevice->psock, pMsg->msgInfo.macAddr, 0x01, al_id);
	  } else {
	    printf("MAC Address must be of form xx:xx:xx:xx:xx:xx\n");
	    return;
	  }
          _i5ControlSetCmdRespSendMsgType(&response);
        }
        break;
      }
    case i5MessageLinkMetricResponseValue:
      i5TraceInfo("Unhandled i5MessageLinkMetricResponseValue\n");
      break;
    case i5MessageApAutoconfigurationSearchValue:
      i5TraceInfo("Unhandled i5MessageApAutoconfigurationSearchValue\n");
      break;
    case i5MessageApAutoconfigurationResponseValue:
      i5TraceInfo("Unhandled i5MessageApAutoconfigurationResponseValue\n");
      break;
    case i5MessageApAutoconfigurationWscValue:
      i5TraceInfo("Unhandled i5MessageApAutoconfigurationWscValue\n");
      break;
    case i5MessageApAutoconfigurationRenewValue:
      i5TraceInfo("Unhandled i5MessageApAutoconfigurationRenewValue\n");
      break;
    case i5MessagePushButtonEventNotificationValue:
      i5TraceInfo("Unhandled i5MessagePushButtonEventNotificationValue\n");
      break;
    case i5MessagePushButtonJoinNotificationValue:
      i5TraceInfo("Unhandled i5MessagePushButtonJoinNotificationValue\n");
      break;
#ifdef MULTIAP
    /* Type = 32769 */
    case i5MessageAPCapabilityQueryValue:
      {
        i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
        if (pDevice && pDevice->psock) {
          i5MessageAPCapabilityQuerySend(pDevice->psock, pMsg->msgInfo.macAddr);
          _i5ControlSetCmdRespSendMsgType(&response);
        } else {
          i5TraceInfo("i5MessageAPCapabilityQueryValue : Neighbor AL MAC Address "
            I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
        }
        break;
      }
    case i5MessageMultiAPPolicyConfigRequestValue:
      {
        i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
        if (pDevice && pDevice->psock) {
          i5MessageMultiAPPolicyConfigRequestSend(pDevice);
          _i5ControlSetCmdRespSendMsgType(&response);
        } else {
          i5TraceInfo("i5MessageMultiAPPolicyConfigRequestValue : Neighbor AL MAC Address "
            I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
        }
        break;
      }
    case i5MessageChannelPreferenceQueryValue:
      {
        i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
        if (pDevice && pDevice->psock) {
          i5MessageChannelPreferenceQuerySend(pDevice->psock, pMsg->msgInfo.macAddr);
          _i5ControlSetCmdRespSendMsgType(&response);
        } else {
          i5TraceInfo("i5MessageChannelPreferenceQueryValue : Neighbor AL MAC Address "
            I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
        }
        break;
      }
    case i5MessageChannelPreferenceReportValue:
      {
        i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
        if (pDevice && pDevice->psock) {
          i5MessageChannelPreferenceReportSendUnsolicited(pDevice->psock, pMsg->msgInfo.macAddr,
            pMsg->data, pMsg->data_len);
          _i5ControlSetCmdRespSendMsgType(&response);
        } else {
          i5TraceInfo("i5MessageChannelPreferenceReportValue: Neighbor AL MAC Address "
            I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
        }
        break;
      }
    case i5MessageChannelSelectionRequestValue:
      {
        i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
        if (pDevice && pDevice->psock) {
          i5MessageChannelSelectionRequestSend(pDevice->psock, pMsg->msgInfo.macAddr,
            pMsg->data, pMsg->data_len);
          _i5ControlSetCmdRespSendMsgType(&response);
        } else {
          i5TraceInfo("i5MessageChannelSelectionRequestValue : Neighbor AL MAC Address "
            I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
        }
        break;
      }
    case i5MessageCombinedInfrastructureMetricsValue:
      {
        i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
        if (pDevice && pDevice->psock) {
          i5MessageCombinedInfrastructureMetricsSend(pDevice->psock, pMsg->msgInfo.macAddr);
          _i5ControlSetCmdRespSendMsgType(&response);
        } else {
          i5TraceInfo("i5MessageCombinedInfrastructureMetricsValue : Neighbor AL MAC Address "
            I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
        }
        break;
      }
    /* Type = 32807 */
    case i5MessageBackhaulSTACapabilityQueryValue:
      {
        i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
        if (pDevice && pDevice->psock) {
          i5MessageBackhaulSTACapabilityQuerySend(pDevice->psock, pMsg->msgInfo.macAddr);
          _i5ControlSetCmdRespSendMsgType(&response);
        } else {
          i5TraceInfo("i5MessageBackhaulSTACapabilityQueryValue : Neighbor AL MAC Address "
            I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
        }
        break;
      }
#endif /* MULTIAP */
    default:
      i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
      break;
  }

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

#if defined(WIRELESS)
static void i5ControlSetWiFiPassSsid ( t_I5_API_PASSWORD_SSID_MSG* msg )
{
  char cmdStr[256] = "";
  int commitFlag = 0;

  i5Trace("ssid:(%s) password:(%s)\n", msg->ssid, msg->password);

  if (msg->ssid[0] != '\0') {
    if (strpbrk((char *)msg->ssid, "?\"$[\\]+") == NULL) {
      snprintf(cmdStr, sizeof(cmdStr), "nvram set wl0_ssid=\"%s\"",msg->ssid);
      i5Trace("%s",cmdStr);
      system(cmdStr);
      commitFlag = 1;
    }
  }
  if (msg->password[0] != '\0') {
    if (strpbrk((char *)msg->password, "?\"$[\\]+") == NULL) {
      snprintf(cmdStr, sizeof(cmdStr), "nvram set wl0_wpa_psk=\"%s\"",msg->password);
      i5Trace("%s",cmdStr);
      system(cmdStr);
      commitFlag = 1;
    }
  }

#if defined(SUPPORT_IEEE1905_GOLDENNODE) && defined(SUPPORT_IEEE1905_REGISTRAR)
  snprintf(cmdStr, sizeof(cmdStr), "nvram set wl0_akm=\"psk2\"");
  i5Trace("%s",cmdStr);
  system(cmdStr);

  snprintf(cmdStr, sizeof(cmdStr), "nvram set wl0_crypto=\"aes\"");
  i5Trace("%s",cmdStr);
  system(cmdStr);

  i5Trace("nvram commit");
#endif

  if (commitFlag) {
    i5Trace("nvram commit");
    system("nvram commit");
  }
}
#endif

#if defined(SUPPORT_HOMEPLUG)
static void i5ControlSetPlcPass (i5_socket_type *psock, t_I5_API_PASSWORD_SSID_MSG* msg, int cmd)
{
  char replyData = i5PlcSetPasswordNmk(msg->password);

  i5Trace("\n");
  if (0 != replyData) {
    i5TraceError("Internal error while setting NMK\n");
  }
}
#endif

static void i5ControlSetConfigHandler(void *pMsg, int length)
{
  t_I5_API_CONFIG_BASE *pCfg = (t_I5_API_CONFIG_BASE *)pMsg;
  i5TraceInfo("Subcommand %d\n", pCfg->subcmd);
  switch ( pCfg->subcmd ) {
    case I5_API_CONFIG_BASE:
    {
      i5_dm_device_type *pdevice = i5DmGetSelfDevice();
      unsigned int isRegistrar = I5_IS_REGISTRAR(i5_config.flags);

      if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_CONFIG_BASE)) ) {
        printf("Invalid length for I5_API_CMD_SET_CONFIG\n");
        break;
      }

      i5Trace("I5_API_CMD_SET_CONFIG - BASE: %d %s %d - bandEn %d, %d\n",
              pCfg->isEnabled,
              pCfg->deviceFriendlyName,
              pCfg->isRegistrar,
              pCfg->apFreqBand24En,
              pCfg->apFreqBand5En);

      i5_config.running = pCfg->isEnabled;
      if ( i5_config.running && pdevice )
      {
 #if defined(WIRELESS)
        int wlConfChange = 0;

        if ( isRegistrar != pCfg->isRegistrar )
        {
          /* change to enrollee - clear configure setting */
          if (0 == pCfg->isRegistrar)
          {
            i5_dm_interface_type *pinterface = pdevice->interface_list.ll.next;
            while ( pinterface )
            {
              if ( i5DmIsInterfaceWireless(pinterface->MediaType) )
              {
                pinterface->isConfigured = 0;
              }
              pinterface = pinterface->ll.next;
            }
          }
          wlConfChange = 1;
        }
        else if ( (i5_config.freqBandEnable[i5MessageFreqBand_802_11_2_4Ghz] != pCfg->apFreqBand24En) ||
                  (i5_config.freqBandEnable[i5MessageFreqBand_802_11_5Ghz] != pCfg->apFreqBand5En) )
        {
          wlConfChange = 1;
        }

        if (pCfg->isRegistrar) {
          i5_config.flags |= I5_CONFIG_FLAG_REGISTRAR;
        } else {
          i5_config.flags &= ~(I5_CONFIG_FLAG_REGISTRAR);
        }
        i5_config.freqBandEnable[i5MessageFreqBand_802_11_2_4Ghz] = pCfg->apFreqBand24En;
        i5_config.freqBandEnable[i5MessageFreqBand_802_11_5Ghz] = pCfg->apFreqBand5En;

        if ( wlConfChange )
        {
#ifdef MULTIAP
          if (i5_config.ptmrApSearch == NULL ) {
            i5WlCfgMultiApControllerSearch(NULL);
          }
#endif /* MULTIAP */
        }
 #endif
        /* do not assume that setup->deviceFriendlyName is NULL terminated */
        strncpy(i5_config.friendlyName, pCfg->deviceFriendlyName, I5_DEVICE_FRIENDLY_NAME_LEN-1);
        i5_config.friendlyName[I5_DEVICE_FRIENDLY_NAME_LEN-1] = '\0';
        strncpy(pdevice->friendlyName, pCfg->deviceFriendlyName, I5_DEVICE_FRIENDLY_NAME_LEN-1);
        pdevice->friendlyName[I5_DEVICE_FRIENDLY_NAME_LEN-1] = '\0';
      }
      break;
    }
 #if defined(DMP_DEVICE2_IEEE1905BASELINE_1)
    case I5_API_CONFIG_SET_NETWORK_TOPOLOGY:
    {
      t_I5_API_CONFIG_SET_NETWORK_TOPOLOGY *pCfg = (t_I5_API_CONFIG_SET_NETWORK_TOPOLOGY *)pMsg;
      if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_CONFIG_SET_NETWORK_TOPOLOGY)) ) {
        printf("Invalid length for I5_API_CMD_SET_CONFIG\n");
        break;
      }

      i5Trace("I5_API_CMD_SET_CONFIG - NETWORK TOPOLOGY: %d\n", pCfg->isEnabled);
      if (i5_config.networkTopEnabled != pCfg->isEnabled) {
        i5_config.networkTopEnabled = pCfg->isEnabled;
        i5CmsMdmProcessNetworkTopologyConfigChange();
      }
      break;
    }
 #endif

    default:
      break;
  }
}

static void i5ControlGetConfigHandler(void *pMsg, i5_socket_type *psock, int length )
{
  t_I5_API_CONFIG_BASE *pCfg = (t_I5_API_CONFIG_BASE *)pMsg;
  i5TraceInfo("Subcommand %d\n", pCfg->subcmd);
  switch ( pCfg->subcmd ) {
    case I5_API_CONFIG_BASE:
    {
      t_I5_API_CONFIG_BASE rspData = { 0 };

      rspData.isEnabled = i5_config.running;
      if (I5_IS_REGISTRAR(i5_config.flags)) {
        rspData.isRegistrar = 1;
      }
      /* friendlyName is NULL terminated */
      strncpy(rspData.deviceFriendlyName, i5_config.friendlyName, I5_DEVICE_FRIENDLY_NAME_LEN);
      rspData.apFreqBand24En = i5_config.freqBandEnable[i5MessageFreqBand_802_11_2_4Ghz];
      rspData.apFreqBand5En = i5_config.freqBandEnable[i5MessageFreqBand_802_11_5Ghz];

      i5Trace("I5_API_CMD_SET_CONFIG - BASE: %d %s %d - bandEn %d, %d\n",
              rspData.isEnabled,
              rspData.deviceFriendlyName,
              rspData.isRegistrar,
              rspData.apFreqBand24En,
              rspData.apFreqBand5En);

      i5apiSendMessage(psock->sd, I5_API_CMD_GET_CONFIG, &rspData, sizeof(rspData));
      break;
    }
#if defined(DMP_DEVICE2_IEEE1905BASELINE_1)
    case I5_API_CONFIG_GET_LINK_METRICS:
    {
      t_I5_API_CONFIG_GET_LINK_METRICS* msg = pMsg;
      i5ControlFetchLinkMetrics(psock->sd, msg->ieee1905Id, msg->remoteInterfaceId);
      break;
    }
#endif

    default:
      i5TraceError("Unhandled Get command - %d\n", pCfg->subcmd);
      _i5ControlUnknownMessage(psock, I5_API_CMD_GET_CONFIG);
      break;
  }
}

/* return value contains the number of bytes read or -1 for error
 * free the buffer after reading the data
 */
static int i5ControlSocketOnReceive(i5_socket_type *psock, unsigned char **out_data, unsigned int *out_length)
{
  int sockfd = psock->sd;
  unsigned int length, data_length, ret_recv = 0;
  unsigned char header_buf[sizeof(t_I5_API_MSG)] = {0};
  unsigned char *buf = NULL;

  i5TraceInfo("\n");

  if (!out_data || !out_length) {
      i5TraceError("Invalid Arguments\n");
      return -1;
  }

  /* First Pass : Read socket just till Header, to Get the Total Read Buffer size from Header */
  ret_recv = i5SocketRecvData(psock, header_buf, sizeof(t_I5_API_MSG));

  /* Check socket read bytes are valid or not */
  if (ret_recv <= 0) {

    i5TraceError("sockfd[%d]. Read Failed Socket: type %d, Process %p\n",
      sockfd, psock->type, psock->process);
    return -1;
  }

  /* Get the Total Read Buffer size from Header Buffer, skipping first 4 bytes */
  if (ret_recv >= sizeof(t_I5_API_MSG)) {

    memcpy(&length, header_buf + sizeof(t_I5_API_CMD_NAME), sizeof(length));
    i5Trace("Total size : %u Total Read : %u\n", length, ret_recv);

  } else {

    i5TraceError("sockfd[%d]. Doesn't contain any size. Socket: type %d, Process %p\n",
      sockfd, psock->type, psock->process);
    return -1;
  }

  /* Alllocate Read Buffer of size "length", Note: length includes header size(8) as well */
  buf = (unsigned char *)malloc( sizeof(unsigned char) * ( length ) );
  if (!buf) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  /* Copy Header 8 bytes to Read Buffer */
  memcpy(buf, header_buf, sizeof(t_I5_API_MSG));

  /* If there is no additional data to be read apart from Header, Exit */
  if (length == sizeof(t_I5_API_MSG)) {
    i5TraceError("sockfd[%d]. Read Buffer length is 0. indicates there is no data."
      " Socket: type %d, Process %p\n", sockfd, psock->type, psock->process);
    goto end;
  }

  /* Second Pass : Read socket for Actual data, skipping Header 8 bytes, which we already read */
  data_length = length - sizeof(t_I5_API_MSG);
  ret_recv = i5SocketRecvData(psock, buf + sizeof(t_I5_API_MSG), data_length);
  i5Trace("Total size : %u Total Read : %u\n", data_length, ret_recv);

  /* Check socket read bytes are valid or not */
  if (ret_recv <= 0) {

    if (buf != NULL) {
      free(buf);
      buf = NULL;
    }

    i5TraceError("sockfd[%d]. Read Failed. Socket: type %d, Process %p\n",
      sockfd, psock->type, psock->process);
    return -1;
  }

end:
  /* Return Filled Data Buffer & Total Read Bytes */
  *out_data = buf;
  *out_length = length;

  return 0;
}

static void i5ControlSocketReceive(i5_socket_type *psock)
{
  int ret_recv = 0;
  unsigned char *buf = NULL;
  unsigned int length = 0;

  i5TraceInfo("\n");

  /* Read socket data, and Allocate Read Buffer. Free Read Buffer after processing data */
  ret_recv = i5ControlSocketOnReceive(psock, &buf, &length);

  /* Check return value of socket read fn is valid or not */
  if (ret_recv < 0) {
	/* socket has been closed */
	i5SocketClose(psock);
  }
  else {
    t_I5_API_MSG *pMsg;

    pMsg = (t_I5_API_MSG *)&buf[0];
    i5TraceInfo("Command %d \n", pMsg->cmd);
    switch ( pMsg->cmd ) {
      case I5_API_CMD_RETRIEVE_DM:
        if ( length < sizeof(t_I5_API_MSG)) {
          printf("Invalid length for I5_API_CMD_RETRIEVE_DM\n");
          _i5ControlUnknownMessage(psock, pMsg->cmd);
        }
        else {
          i5ControlDataModel(psock, pMsg->cmd);
        }
        break;

      case I5_API_CMD_TRACE:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_TRACE_MSG))) {
          printf("Invalid length for I5_API_CMD_TRACE\n");
        }
        else {
          t_I5_API_TRACE_MSG* traceCmd = (t_I5_API_TRACE_MSG *)(pMsg + 1);
          if ( ((traceCmd->module_id >= 0) && (traceCmd->module_id < i5TraceLast)) ||
                (traceCmd->module_id == 255) || (traceCmd->module_id == i5TracePacket) ) {
            i5TraceSet(traceCmd->module_id, traceCmd->depth, traceCmd->ifindex, traceCmd->interfaceMac);
          }
          else {
            printf("Invalid moduled_id (%d) for I5_API_CMD_TRACE\n", traceCmd->module_id);
          }
        }
        break;

      case I5_API_CMD_TRACE_TIME:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(int)) ) {
          printf("Invalid length for I5_API_CMD_TRACE_TIME\n");
        }
        else {
          i5TraceTimestampSet(*(int *)(pMsg + 1));
        }
        break;

#if defined(WIRELESS)
      case I5_API_CMD_WLCFG:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_WLCFG_MSG))) {
          printf("Invalid length for I5_API_CMD_WLCFG\n");
          _i5ControlUnknownMessage(psock, pMsg->cmd);
        }
        else {
          i5ctlWlcfgHandler(psock, (t_I5_API_WLCFG_MSG *)(pMsg + 1));
        }
        break;
#endif

#if defined(SUPPORT_HOMEPLUG)
      case I5_API_CMD_PLC:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_PLC_MSG))) {
          printf("Invalid length for I5_API_CMD_PLC\n");
        }
        else {
          i5ControlPlcHandler((t_I5_API_PLC_MSG *)(pMsg + 1));
        }
        break;
#endif
#if defined(SUPPORT_IEEE1905_FM)
      case I5_API_CMD_FLOWSHOW:
         i5FlowManagerShow();
         break;
#endif /* defined(SUPPORT_IEEE1905_FM) */

      case I5_API_CMD_LINKUPDATE:
        i5MessageSendLinkQueries();
        break;

#ifndef MULTIAP
      case I5_API_CMD_JSON_LEG:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_JSON_LEG_MSG))) {
          printf("Invalid length for I5_API_CMD_JSON_LEG\n");
        }
        else {
          i5ControlJsonLegHandler((t_I5_API_JSON_LEG_MSG *)(pMsg + 1));
        }
        break;
#endif
      case I5_API_CMD_PUSH_BUTTON:
        i5SecurityProcessLocalPushButtonEvent();
#if defined(IEEE1905_KERNEL_MODULE_PB_SUPPORT)
        i5UdpSocketTriggerWirelessPushButtonEvent(1);
#endif
        break;

      case I5_API_CMD_SHOW_AL_MAC:
        i5ControlAlMacAddress(psock, pMsg->cmd);
        break;

      case I5_API_CMD_SEND_MESSAGE:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_SEND_MESSAGE)) ) {
          printf("Invalid length for I5_API_CMD_SEND_MESSAGE\n");
          _i5ControlUnknownMessage(psock, pMsg->cmd);
        }
        else {
          i5ControlSendCommand(psock, (t_I5_API_SEND_MESSAGE *)(pMsg + 1), pMsg->cmd);
        }
        break;

      case I5_API_CMD_SET_LQ_INTERVAL:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_LINK_METRIC_INTERVAL)) ) {
          printf("Invalid length for I5_API_CMD_SET_LQ_INTERVAL\n");
        }
        else {
          i5ControlLinkMetricIntervalHandler((t_I5_API_LINK_METRIC_INTERVAL *)(pMsg + 1));
        }
        break;

      case I5_API_CMD_SEND_BYTES:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_GOLDEN_NODE_SEND_MSG_BYTES)) ) {
          printf("Invalid length for I5_API_CMD_SEND_BYTES\n");
        }
        else {
          i5ControlSendBytesCommand (length, (t_I5_API_GOLDEN_NODE_SEND_MSG_BYTES*) (pMsg + 1) );
        }
        break;

#if defined(WIRELESS)
      case I5_API_CMD_SET_WIFI_PASS_SSID:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_PASSWORD_SSID_MSG)) ) {
          printf("Invalid length for I5_API_CMD_SET_WIFI_PASS_SSID\n");
        }
        else {
          i5ControlSetWiFiPassSsid ( (t_I5_API_PASSWORD_SSID_MSG*) (pMsg + 1));
        }
        break;
      case I5_API_CMD_SET_WIFI_OVERRIDE_BW:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_OVERRIDE_BW_MSG)) ) {
          printf("Invalid length for I5_API_CMD_SET_WIFI_OVERRIDE_BW\n");
        }
        else {
          t_I5_API_OVERRIDE_BW_MSG *overrideMsg = (t_I5_API_OVERRIDE_BW_MSG*) (pMsg + 1);
          i5WlLinkMetricsOverrideBandwidth(overrideMsg->availBwMbps, overrideMsg->macThroughBwMbps, overrideMsg->overrideCount);
        }
        break;
#endif
#if defined(SUPPORT_HOMEPLUG)
      case I5_API_CMD_SET_PLC_PASS_NMK:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_PASSWORD_SSID_MSG)) ) {
          printf("Invalid length for I5_API_CMD_SET_PLC_PASS_NMK\n");
        }
        else {
          i5ControlSetPlcPass (psock, (t_I5_API_PASSWORD_SSID_MSG*) (pMsg + 1), pMsg->cmd);
        }
        break;
      case I5_API_CMD_SET_PLC_OVERRIDE_BW:
        if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_OVERRIDE_BW_MSG)) ) {
          printf("Invalid length for I5_API_CMD_SET_PLC_OVERRIDE_BW\n");
        }
        else {
          t_I5_API_OVERRIDE_BW_MSG *overrideMsg =  (t_I5_API_OVERRIDE_BW_MSG*) (pMsg + 1);
          i5PlcLinkMetricsOverrideBandwidth(overrideMsg->availBwMbps, overrideMsg->macThroughBwMbps, overrideMsg->overrideCount);
        }
        break;
#endif
#if defined(SUPPORT_MOCA)
      case I5_API_CMD_SET_MOCA_PASS:
        /* There's no password in MoCA */
        break;
#endif
      case I5_API_CMD_SHOW_MSGS:
        i5MessageDumpMessages();
        break;
      case I5_API_CMD_SHOW_SOCKETS:
        i5SocketDumpSockets();
        break;
      case I5_API_CMD_STOP:
        i5_config.running = 0;
#if defined(USEBCMUSCHED)
        bcm_usched_stop(i5_config.usched_hdl);
#endif /* USEBCMUSCHED */
        break;
      case I5_API_CMD_SET_CONFIG:
          i5TraceInfo("SET CONFIG\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_CONFIG_SUBCMD)) ) {
            printf("Invalid length for I5_API_CMD_SET_CONFIG\n");
          }
          else {
            i5ControlSetConfigHandler((pMsg + 1), length);
          }
          break;
      case I5_API_CMD_GET_CONFIG:
          i5TraceInfo("GET CONFIG\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_CONFIG_SUBCMD)) ) {
            printf("Invalid length for I5_API_CMD_GET_CONFIG\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlGetConfigHandler((pMsg + 1), psock, length);
          }
          break;
#ifdef MULTIAP
      case I5_API_CMD_CLIENT_CAP:
          i5TraceInfo("Client Capability Query\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_CLIENT_CAP_QUERY)) ) {
            printf("Invalid length for I5_API_CMD_CLIENT_CAP\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendClientCapability(psock, (t_I5_API_CLIENT_CAP_QUERY *)(pMsg + 1), pMsg->cmd);
          }
          break;

      case I5_API_CMD_SEND_HL_DATA:
          i5TraceInfo("Higher Layer Data Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_HIGHER_LAYER_DATA)) ) {
            printf("Invalid length for I5_API_CMD_SEND_HL_DATA\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendHLData(psock, (t_I5_API_HIGHER_LAYER_DATA *)(pMsg + 1), pMsg->cmd);
          }
          break;

      case I5_API_CMD_SEND_STEER:
          i5TraceInfo("Client Steering Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_STEER)) ) {
            printf("Invalid length for t_I5_API_STEER\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendSteer(psock, (t_I5_API_STEER *)(pMsg + 1), pMsg->cmd);
          }
          break;

      case I5_API_CMD_LIST_OPERATION:
          i5TraceInfo("List opearion Message \n");
          if (length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_LIST_OPR))) {
            printf("Invalid length for t_I5_LIST_OPR\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
	    i5ControlConfigListOpr(psock, (t_I5_LIST_OPR *)(pMsg + 1), pMsg->cmd);
          }
	  break;

      case I5_API_CMD_STEER_CONFIG:
          i5TraceInfo("Steer config opearion Message \n");
          if (length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_STEER_POLICY_CONFIG))) {
            printf("Invalid length for t_I5_STEER_POLICY_CONFIG\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
	    i5ControlSteerConfigOpr(psock, (t_I5_STEER_POLICY_CONFIG *)(pMsg + 1), pMsg->cmd);
          }
          break;

      case I5_API_CMD_ASSOC_CNTRL:
          i5TraceInfo("Client Association Control Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_ASSOC_CNTRL)) ) {
            printf("Invalid length for t_I5_API_ASSOC_CNTRL\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendClientAssocControl(psock, (t_I5_API_ASSOC_CNTRL *)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_SEND_FILE:
          i5TraceInfo("Send File Data\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_FILE)) ) {
            printf("Invalid length for I5_API_CMD_SEND_FILE\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendFile(psock, (t_I5_API_FILE *)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_WPS:
          i5TraceInfo("WPS Control Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_WPS_CNTRL)) ) {
            printf("Invalid length for t_I5_API_WPS_CNTRL\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlWPSOpr(psock, (t_I5_API_WPS_CNTRL *)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_RENEW_CONFIG:
          i5TraceInfo("Send AP configuration renew Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_RENEW_AP_CONFIG)) ) {
            printf("Invalid length for t_I5_API_RENEW_AP_CONFIG\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendApConfigRenew(psock, (t_I5_API_RENEW_AP_CONFIG *)(pMsg + 1), pMsg->cmd);
          }
	  break;
      case I5_API_CMD_SEND_BH_STEER:
          i5TraceInfo("Backhaul Steering Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_BH_STEER)) ) {
            printf("Invalid length for t_I5_API_BH_STEER\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendBhSteer(psock, (t_I5_API_BH_STEER *)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_METRIC_CONFIG:
          i5TraceInfo("Metric Config Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_METRIC_RPT_POLICY_CONFIG)) ) {
            printf("Invalid length for t_I5_METRIC_RPT_POLICY_CONFIG\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlMetricConfigOpr(psock, (t_I5_METRIC_RPT_POLICY_CONFIG *)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_SEND_AP_METRIC_QUERY:
          i5TraceInfo("Send AP Metric Query Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_AP_METRIC_QUERY)) ) {
            printf("Invalid length for t_I5_API_AP_METRIC_QUERY\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendApMetricQuery(psock, (t_I5_API_AP_METRIC_QUERY *)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_SEND_ASSOC_STA_LINK_METRIC_QUERY:
          i5TraceInfo("Send Associated STA Link Metrics Query Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_ASSOC_STA_LINK_METRIC_QUERY)) ) {
            printf("Invalid length for t_I5_API_ASSOC_STA_LINK_METRIC_QUERY\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendAssociatedSTALinkMetricQuery(psock,
              (t_I5_API_ASSOC_STA_LINK_METRIC_QUERY *)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_SEND_UNASSOC_STA_LINK_METRIC_QUERY:
          i5TraceInfo("Send UnAssociated STA Link Metrics Query Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_UNASSOC_STA_LINK_METRIC_QUERY)) ) {
            printf("Invalid length for t_I5_API_UNASSOC_STA_LINK_METRIC_QUERY\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendUnAssociatedSTALinkMetricQuery(psock,
              (t_I5_API_UNASSOC_STA_LINK_METRIC_QUERY *)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_SEND_BEACON_METRIC_QUERY:
          i5TraceInfo("Send Beacon Metric Query Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_BEACON_METRIC_QUERY)) ) {
            printf("Invalid length for I5_API_CMD_SEND_BEACON_METRIC_QUERY\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendBeaconMetricQuery(psock,
              (t_I5_API_BEACON_METRIC_QUERY *)(pMsg + 1), pMsg->cmd);
          }
          break;

      case I5_API_CMD_HLE_SEND_HL_DATA:
          i5TraceInfo("HLE wants to send Higher Layer Data over 1905 Layer\n");
          if ( length < (sizeof(i5_higher_layer_data_send_t) - sizeof(unsigned char *)) ) {
            printf("Invalid length for I5_API_CMD_HLE_SEND_HL_DATA\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlHLESendHLData(psock, (void *)(pMsg), pMsg->cmd);
          }
          break;

      case I5_API_CMD_RETRIEVE_DM_EXT:
        if ( length < sizeof(t_I5_API_MSG)) {
          printf("Invalid length for I5_API_CMD_RETRIEVE_DM_EXT\n");
          _i5ControlUnknownMessage(psock, pMsg->cmd);
        }
        else {
          i5ControlDataModelExtended(psock, pMsg->cmd);
        }
        break;

      case I5_API_CMD_SEND_CAPI:
          i5TraceInfo("Send CAPI Command\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_FILE)) ) {
            printf("Invalid length for I5_API_CMD_SEND_CAPI\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendCAPI(psock, (t_I5_API_FILE *)(pMsg + 1), pMsg->cmd);
          }
          break;

#if defined(MULTIAPR2)
      case I5_API_CMD_SEND_CHSCAN:
          i5TraceInfo("Channel Scan Request\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_CH_SCAN_REQ)) ) {
            printf("Invalid length for I5_API_CMD_SEND_CHSCAN\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendChannelScanRequest(psock, (t_I5_API_CH_SCAN_REQ *)(pMsg + 1));
          }
        break;

      case I5_API_CMD_SEND_CAC_REQUEST:
          i5TraceInfo("Send CAC request Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_CAC_REQUEST)) ) {
            i5TraceDirPrint("Invalid length for t_I5_API_CAC_REQUEST\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
	   i5ControlSendCACRequest(psock, (t_I5_API_CAC_REQUEST *)(pMsg + 1), pMsg->cmd);
          }
          break;

      case I5_API_CMD_SEND_CAC_TERMINATION:
          i5TraceInfo("Send CAC terminate Message\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_CAC_TERMINATION)) ) {
            i5TraceDirPrint("Invalid length for t_I5_API_CAC_TERMINATION\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlSendCACTermination(psock,(t_I5_API_CAC_TERMINATION*)(pMsg + 1), pMsg->cmd);
          }
          break;
      case I5_API_CMD_ASSOC_DISALLOW:
          i5TraceInfo("Assoc Disallow Request\n");
          if ( length < (sizeof(t_I5_API_MSG) + sizeof(t_I5_API_ASSOC_DISALLOW_REQ)) ) {
            printf("Invalid length for I5_API_CMD_ASSOC_DISALLOW\n");
            _i5ControlUnknownMessage(psock, pMsg->cmd);
          }
          else {
            i5ControlAssocDisallowRequest(psock, (t_I5_API_ASSOC_DISALLOW_REQ *)(pMsg + 1));
          }
        break;
#endif /* MULTIAPR2 */
#endif /* MULTIAP */
      default:
        printf("Unknown command received %d\n", pMsg->cmd);
        _i5ControlUnknownMessage(psock, pMsg->cmd);
        break;
    }
  }

  if (buf) {
    free(buf);
  }
  return;
}

#ifdef MULTIAP
static int i5ControlListAddItem(ieee1905_glist_t *list, unsigned char *mac)
{
  ieee1905_sta_list *sta = (ieee1905_sta_list*)malloc(sizeof(*sta));
  if (!sta) {
    i5TraceDirPrint("malloc error\n");
    return 0;
  }

  memcpy(sta->mac, mac, MAC_ADDR_LEN);
  ieee1905_glist_append(list, (dll_t*)sta);

  return 1;
}

static int i5ControlListDeleteItem(ieee1905_glist_t *list, unsigned char *mac)
{
  dll_t *item_p, *next_p;
  ieee1905_sta_list *item;
  int ret = 0;
  char str[I5_MAC_STR_BUF_LEN] = {0}, *invalid_mac = "00:00:00:00:00:00";

  (void)i5MacAddr2String(mac, str);

  if (!strcmp(str, invalid_mac)) {
     /* Delete whole list */
     i5DmGlistCleanup(list);
     ret = 1;
  } else {
    /* Travese List */
    for (item_p = dll_head_p(&list->head);
      !dll_end(&list->head, item_p);
      item_p = next_p) {

      item = (ieee1905_sta_list *)item_p;

      next_p = dll_next_p(item_p);
      if (!memcmp(item->mac, mac, MAC_ADDR_LEN)) {
        /* Remove item itself from list */
        ieee1905_glist_delete(list, item_p);
        free(item_p);
        ret = 1;
        break;
      }
    }
  }

  return ret;
}

static void i5ControlListShow(ieee1905_glist_t *list, char *response, unsigned int len)
{
  dll_t *item_p, *next_p;
  ieee1905_sta_list *item;
  int count = 0;

  if (!response || !len) {
    return;
  }

  count = snprintf(response, len, "%s%d%s", "List has ", list->count,  " items: \n");

  for (item_p = dll_head_p(&list->head);
    !dll_end(&list->head, item_p);
    item_p = next_p) {
    item = (ieee1905_sta_list *)item_p;
    next_p = dll_next_p(item_p);
    if (len > count) {
      len = len - count;
      count += snprintf(response + count, len, ""I5_MAC_DELIM_FMT"\n", I5_MAC_PRM(item->mac));
    }
  }
}

static void i5ControlConfigListOpr(i5_socket_type *psock, t_I5_LIST_OPR *pMsg, int cmd)
{
  ieee1905_glist_t *list = &i5_config.policyConfig.no_steer_sta_list;
  unsigned int max_length = 1024;
  char *response = (char *)malloc(max_length);

  if (!response) {
    i5TraceDirPrint("malloc error\n");
    return;
  }
  memset(response, 0, max_length);

  if (pMsg->list_id < 0 || pMsg->list_id > 1) {
    goto end;
  }

  if (pMsg->operation < 0 || pMsg->operation > 2) {
    goto end;
  }

  if (pMsg->list_id == 1) {
    list = &i5_config.policyConfig.no_btm_steer_sta_list;
  }

  if(pMsg->operation == 0) {
    if (i5ControlListDeleteItem(list, pMsg->MAC)) {
      snprintf(response, max_length, "%s", "Item deleted successfully\n");
    }
  } else if (pMsg->operation == 1) {
    if (i5ControlListAddItem(list, pMsg->MAC)) {
      snprintf(response, max_length, "%s", "Item added successfully\n");
    }
  } else if(pMsg->operation == 2) {
    i5ControlListShow(list, response, max_length);
  }

end:
  i5apiSendMessage(psock->sd, cmd, response, max_length);
}

static void i5ControlWPSOpr(i5_socket_type *psock, t_I5_API_WPS_CNTRL *pMsg, int cmd)
{
  i5WlCfgHandleWPSPBC(pMsg->rfband, pMsg->mode);
  i5apiSendMessage(psock->sd, cmd, NULL, 0);
}

static void i5ControlSteerConfigOpr(i5_socket_type *psock, t_I5_STEER_POLICY_CONFIG *pMsg, int cmd)
{

  ieee1905_glist_t *list = &i5_config.policyConfig.steercfg_bss_list;
  unsigned int opr = pMsg->operation;
  unsigned int max_length = 1024;
  char *response = (char *)malloc(max_length);

  if (!response) {
    i5TraceDirPrint("malloc error\n");
    return;
  }
  memset(response, 0, max_length);

  if (opr < 0 || opr > 2) {
    goto end;
  }

  if(opr == 0) {
    if (i5ControlListDeleteItem(list, pMsg->MAC)) {
      snprintf(response, max_length, "%s", "Item deleted successfully\n");
    }
  } else if (opr == 1) {
    ieee1905_bss_steer_config *bss = (ieee1905_bss_steer_config*)malloc(sizeof(ieee1905_bss_steer_config));
    if (!bss) {
      i5TraceDirPrint("malloc error\n");
      goto end;
    }

    memcpy(bss->mac, pMsg->MAC, MAC_ADDR_LEN);
    bss->policy = pMsg->steer_policy;
    bss->bssload_thld = pMsg->bss_load_thld;
    bss->rssi_thld = pMsg->rssi_thld;
    ieee1905_glist_append(list, (dll_t*)bss);
    snprintf(response, max_length, "%s", "Item added successfully\n");
  } else if(opr == 2) {
    i5ControlListShow(list, response, max_length);
  }

end:
  i5apiSendMessage(psock->sd, cmd, response, max_length);
}
#endif	/* MULTIAP */

static void i5ControlSocketAccept(i5_socket_type *psock)
{
  struct sockaddr_un clientAddr;
  unsigned int sockAddrSize;
  int sd;
  int flags;

  i5TraceInfo("\n");
  sockAddrSize = sizeof(clientAddr);
  if ((sd = accept(psock->sd, (struct sockaddr *)&clientAddr, &sockAddrSize)) < 0) {
    fprintf(stderr, "%s: accept connection failed. errno=%d\n", __func__, errno);
    return;
  }

  flags = fcntl(sd, F_GETFL, 0);
  if(flags < 0) {
    fprintf(stderr, "%s: cannot retrieve socket flags. errno=%d\n", __func__, errno);
  }
  if ( fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0 ) {
    fprintf(stderr, "%s: cannot set socket to non-blocking. errno=%d\n", __func__, errno);
  }

  if ( NULL == i5SocketNew(sd, i5_socket_type_stream, i5ControlSocketReceive) ) {
    close(sd);
  }
  return;
}

i5_socket_type *i5ControlSocketCreate(int supServiceFlag)
{
  i5_socket_type *psock;
  int             sd;
  int             flags;
  int             optval = 1;
  socklen_t       optlen = sizeof(optval);
  unsigned short port = I5_GLUE_CONTROL_SOCK_PORT;

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket() error, %s\n", strerror(errno));
    return NULL;
  }

  psock = i5SocketNew(sd, i5_socket_type_stream, i5ControlSocketAccept);
  if ( NULL == psock ) {
    printf("i5SocketNew failed\n");
    close(sd);
    return NULL;
  }

  if (I5_IS_MULTIAP_CONTROLLER(supServiceFlag)) {
    port = I5_GLUE_CONTROL_SOCK_CONTROLLER_PORT;
  }

  /* Allow reusing the socket immediately when application is restarted */
  if (setsockopt(psock->sd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen)) {
    printf("setsockopt error %s\n", strerror(errno));
  }

  if (fcntl(psock->sd, F_SETFD, FD_CLOEXEC) == -1) {
    printf("fcntl error %s\n", strerror(errno));
  }

  psock->u.sinl.sa.sin_family      = AF_INET;
  psock->u.sinl.sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  psock->u.sinl.sa.sin_port        = htons(port);
  if((bind(sd, (struct sockaddr *)&psock->u.sinl.sa, sizeof(struct sockaddr_in))) == -1) {
    printf("bind() to port %d error, %s\n", port, strerror(errno));
    i5SocketClose(psock);
    return NULL;
  }

  if ((listen(sd, I5_MESSAGE_BACKLOG)) == -1) {
    printf("listen() to port %d error, %s", port, strerror(errno));
    i5SocketClose(psock);
    return NULL;
  }

  flags = fcntl(sd, F_GETFL, 0);
  if(flags < 0) {
    printf("cannot retrieve socket flags. error=%s", strerror(errno));
  }
  if ( fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0 ) {
    printf("cannot set socket to non-blocking. error=%s", strerror(errno));
  }

  return(psock);
}

#ifdef MULTIAP
static void i5ControlSendClientCapability(i5_socket_type *psock, t_I5_API_CLIENT_CAP_QUERY *pMsg,
  int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  if (pMsg->msgInfo.messageId != i5MessageClientCapabilityQueryValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {
      i5MessageClientCapabilityQuerySend(pDevice->psock, pMsg->msgInfo.macAddr, pMsg->clientMAC,
        pMsg->BSSID);
      _i5ControlSetCmdRespSendMsgType(&response);
    } else {
        i5TraceInfo("i5MessageClientCapabilityQueryValue : Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
      }
  }

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

static void i5ControlSendHLData(i5_socket_type *psock, t_I5_API_HIGHER_LAYER_DATA *pMsg, int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  if (pMsg->msgInfo.messageId != i5MessageHigherLayerDataValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (i5DmDeviceIsSelf(pMsg->msgInfo.macAddr)) {
      i5TraceInfo("i5MessageHigherLayerDataValue : Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Is Self Device\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
      goto end;
    }
    if (pDevice && pDevice->psock) {
      i5MessageHigherLayerDataMessageSend(pDevice->psock, pMsg->msgInfo.macAddr, pMsg->protocol,
        pMsg->data, pMsg->data_len);
      _i5ControlSetCmdRespSendMsgType(&response);
    } else {
        i5TraceInfo("i5MessageHigherLayerDataValue : Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
      }
  }

end:
  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

static void i5ControlHLESendHLData(i5_socket_type *psock, void *data, int cmd)
{
  i5_higher_layer_data_send_t *pMsg = (i5_higher_layer_data_send_t *) data;
  unsigned char *data_buf = (unsigned char *) data;

  i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->dst_al_mac);

  if (i5DmDeviceIsSelf(pMsg->dst_al_mac)) {

    i5TraceInfo("i5MessageHigherLayerDataValue : Destination AL MAC Address "
      I5_MAC_DELIM_FMT " Is Self Device\n", I5_MAC_PRM(pMsg->dst_al_mac));
    return;
  }
  if (pDevice && pDevice->psock) {

    unsigned int payload_offset = sizeof(pMsg->tag) + sizeof(pMsg->length) +
      sizeof(pMsg->dst_al_mac) + sizeof(pMsg->protocol);
    unsigned int payload_length = (pMsg->length - payload_offset);

    i5MessageHigherLayerDataMessageSend(pDevice->psock, pMsg->dst_al_mac, pMsg->protocol,
      &data_buf[payload_offset], payload_length);

  } else {

    i5TraceInfo("i5MessageHigherLayerDataValue : Neighbor AL MAC Address "
      I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->dst_al_mac));
  }
}

static int i5ControlPrepareSteerReq(t_I5_API_STEER *pMsg, ieee1905_steer_req *steer_req)
{
  ieee1905_sta_list *sta_info;
  ieee1905_bss_list *bss_info;

  memcpy(&steer_req->source_bssid, pMsg->bssid, sizeof(steer_req->source_bssid));

  /* Initialize sta and bss list */
  ieee1905_glist_init(&steer_req->sta_list);
  ieee1905_glist_init(&steer_req->bss_list);

  if (pMsg->request_mode) {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_MANDATE;
  } else {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_OPPORTUNITY;
  }

  if (pMsg->disassoc_imminent)
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_DISASSOC_IMNT;

  if (pMsg->abridged)
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_BTM_ABRIDGED;

  steer_req->opportunity_window = pMsg->opportunity_window;
  steer_req->dissassociation_timer = pMsg->disassoc_timer;

  if (!i5DmIsMacNull(pMsg->sta_mac)) {
    sta_info = (ieee1905_sta_list*)malloc(sizeof(*sta_info));
    if (!sta_info) {
      i5TraceDirPrint("malloc error\n");
      return -1;
    }
    memcpy(sta_info->mac, pMsg->sta_mac, MAC_ADDR_LEN);
    ieee1905_glist_append(&steer_req->sta_list, (dll_t*)sta_info);
  }

  /* Only for steer Mandate */
  if (IEEE1905_IS_STEER_MANDATE(steer_req->request_flags) && !i5DmIsMacNull(pMsg->trgt_bssid)) {
    bss_info = (ieee1905_bss_list*)malloc(sizeof(*bss_info));
    if (!bss_info) {
      i5TraceDirPrint("malloc error\n");
      return -1;
    }
    memcpy(bss_info->bssid, pMsg->trgt_bssid, MAC_ADDR_LEN);
    bss_info->target_op_class = pMsg->operating_class;
    bss_info->target_channel = pMsg->channel;
    ieee1905_glist_append(&steer_req->bss_list, (dll_t*)bss_info);
  }

  return 0;
}

static void i5ControlSendSteer(i5_socket_type *psock, t_I5_API_STEER *pMsg,
  int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};
  ieee1905_steer_req steer_req;

  memset(&steer_req, 0, sizeof(steer_req));

  if (pMsg->msgInfo.messageId != i5MessageClientSteeringRequestValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {
      if (i5ControlPrepareSteerReq(pMsg, &steer_req) != 0) {
        i5DmSteerRequestInfoFree(&steer_req);
        goto end;
      }
      i5MessageClientSteeringRequestSend(pDevice->psock, pMsg->msgInfo.macAddr, &steer_req, NULL);
      _i5ControlSetCmdRespSendMsgType(&response);
      i5DmSteerRequestInfoFree(&steer_req);
    } else {
        i5TraceInfo("i5MessageClientSteeringRequestValue : Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
      }
  }

end:
  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

static int i5ControlPrepareBlockUnblock(t_I5_API_ASSOC_CNTRL *pMsg,
  ieee1905_block_unblock_sta *block_unblock_sta)
{
  ieee1905_sta_list *sta_info;

  /* Prepare the block/unblock STA structure to send client association control message */
  memcpy(block_unblock_sta->source_bssid, pMsg->bssid, sizeof(block_unblock_sta->source_bssid));
  block_unblock_sta->unblock = pMsg->assocCntrl;
  block_unblock_sta->time_period = pMsg->validity;
  ieee1905_glist_init(&block_unblock_sta->sta_list);
  sta_info = (ieee1905_sta_list*)malloc(sizeof(*sta_info));
  if (!sta_info) {
    i5TraceDirPrint("malloc error\n");
    goto end;
  }

  memcpy(sta_info->mac, pMsg->sta_mac, sizeof(sta_info->mac));
  ieee1905_glist_append(&block_unblock_sta->sta_list, (dll_t*)sta_info);
  return 0;

end:
  return -1;
}

static void i5ControlSendClientAssocControl(i5_socket_type *psock, t_I5_API_ASSOC_CNTRL *pMsg,
  int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};
  ieee1905_block_unblock_sta block_unblock_sta;

  memset(&block_unblock_sta, 0, sizeof(block_unblock_sta));

  if (pMsg->msgInfo.messageId != i5MessageClientAssociationControlRequestValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {
      if (i5ControlPrepareBlockUnblock(pMsg, &block_unblock_sta) != 0) {
        i5DmBlockUnblockInfoFree(&block_unblock_sta);
        goto end;
      }
      i5MessageClientAssociationControlRequestSend(pDevice->psock, pMsg->msgInfo.macAddr,
        &block_unblock_sta);
      _i5ControlSetCmdRespSendMsgType(&response);
      i5DmBlockUnblockInfoFree(&block_unblock_sta);
    } else {
        i5TraceInfo("i5MessageClientAssociationControlRequestValue : Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
    }
  }

end:
  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

#ifdef MULTIAP
static void i5ControlSendBhSteer(i5_socket_type *psock, t_I5_API_BH_STEER *pMsg, int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};
  ieee1905_backhaul_steer_msg bh_steer_req;

  memset(&bh_steer_req, 0, sizeof(bh_steer_req));

  if (pMsg->msgInfo.messageId != i5MessageBackhaulSteeringRequestValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {
      memcpy(&bh_steer_req.bh_sta_mac, pMsg->bh_sta_mac, sizeof(bh_steer_req.bh_sta_mac));
      memcpy(&bh_steer_req.trgt_bssid, pMsg->trgt_bssid, sizeof(bh_steer_req.trgt_bssid));
      bh_steer_req.opclass = pMsg->opclass;
      bh_steer_req.channel = pMsg->channel;

      i5MessageBackhaulSteeringRequestSend(pDevice->psock, pMsg->msgInfo.macAddr, &bh_steer_req);
      _i5ControlSetCmdRespSendMsgType(&response);
    } else {
        i5TraceInfo("i5MessageBackhaulSteeringRequestValue: Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
      }
  }
  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

static void i5ControlMetricConfigOpr(i5_socket_type *psock, t_I5_METRIC_RPT_POLICY_CONFIG *pMsg, int cmd)
{

  ieee1905_metricrpt_config *config = &i5_config.policyConfig.metricrpt_config;
  ieee1905_glist_t *list = &config->ifr_list;
  unsigned int opr = pMsg->operation;
  unsigned int max_length = 1024;
  char *response = (char *)malloc(max_length);

  if (!response) {
    i5TraceDirPrint("malloc error\n");
    return;
  }
  memset(response, 0, max_length);

  if (opr < 0 || opr > 2) {
    goto end;
  }

  if(opr == 0) {
    config->ap_rpt_intvl = 0;
    if (i5ControlListDeleteItem(list, pMsg->MAC)) {
      snprintf(response, max_length, "%s", "Item deleted successfully\n");
    }
  } else if (opr == 1) {
    ieee1905_ifr_metricrpt *rpt = (ieee1905_ifr_metricrpt*)malloc(sizeof(*rpt));
    if (!rpt) {
      i5TraceDirPrint("malloc error\n");
      goto end;
    }
    memset(rpt, 0, sizeof(*rpt));
    config->ap_rpt_intvl = pMsg->ap_rpt_intvl;
    memcpy(rpt->mac, pMsg->MAC, MAC_ADDR_LEN);
    rpt->sta_mtrc_rssi_thld = pMsg->sta_mtrc_rssi_thld;
    rpt->sta_mtrc_rssi_hyst = pMsg->sta_mtrc_rssi_hyst;
    rpt->ap_mtrc_chan_util = pMsg->ap_mtrc_chan_util;
    rpt->sta_mtrc_policy_flag = pMsg->sta_mtrc_policy_flag;
    ieee1905_glist_append(list, (dll_t*)rpt);
    snprintf(response, max_length, "%s", "Item added successfully\n");
  } else if(opr == 2) {
    i5ControlListShow(list, response, max_length);
  }

end:
  i5apiSendMessage(psock->sd, cmd, response, max_length);
}
#endif /* MULTIAP */

static void i5ControlProcessFile(unsigned char *data, unsigned int len)
{
  char *pch;
  int index = 0;
  ieee1905_client_bssinfo_type *clientbss = NULL;

  i5DmGlistCleanup(&i5_config.client_bssinfo_list);
  ieee1905_glist_init(&i5_config.client_bssinfo_list);

  pch = strtok((char*)data, " \r\n");
  while (pch) {
    printf("index %d pch %s\n", index, pch);
    if (index == 0) {
      clientbss = (ieee1905_client_bssinfo_type *)malloc(sizeof(*clientbss));
      if (!clientbss) {
        i5TraceDirPrint("Malloc Failed\n");
        goto end;
      }
      memset(clientbss, 0, sizeof(*clientbss));
    }

    if (index == 0) {
      /* AL ID */
      if (NULL == i5String2MacAddr(pch, clientbss->ALID)) {
        i5TraceError("MAC Address must be of form xx:xx:xx:xx:xx:xx: %s\n", pch);
        goto end;
      }
    } else if (index == 1) {
      /* Op class */
      if (strncmp(pch, "8x", 2) == 0) {
        clientbss->band_flag = BAND_2G;
      } else if (strncmp(pch, "11x", 3) == 0) {
        clientbss->band_flag = BAND_5GL;
      } else if (strncmp(pch, "12x", 3) == 0) {
        clientbss->band_flag = BAND_5GH;
      } else {
        i5TraceError("Invalid op class : %s (valid are 8x, 11x and 12x)\n", pch);
        goto end;
      }
    } else if (index == 2) {
      /* SSID */
      clientbss->ssid.SSID_len = (unsigned char)strlen(pch);
      if (clientbss->ssid.SSID_len > IEEE1905_MAX_SSID_LEN) {
        i5TraceError("SSID is too long. len: %d max: %d SSID: %s\n",
          clientbss->ssid.SSID_len, IEEE1905_MAX_SSID_LEN, pch);
        goto end;
      }
      I5STRNCPY((char *)clientbss->ssid.SSID, pch, clientbss->ssid.SSID_len + 1);
    } else if (index == 3) {
      /* Auth */
      clientbss->AuthType = (unsigned short)strtoul(pch, NULL, 16);
    } else if (index == 4) {
      /* Encr */
      clientbss->EncryptType = (unsigned short)strtoul(pch, NULL, 16);
    } else if (index == 5) {
      /* Password */
      clientbss->NetworkKey.key_len = strlen(pch);
      if (clientbss->NetworkKey.key_len > IEEE1905_MAX_KEY_LEN) {
        i5TraceError("key is too long. len: %d max: %d key: %s\n",
          clientbss->NetworkKey.key_len, IEEE1905_MAX_KEY_LEN, pch);
        goto end;
      }
      I5STRNCPY((char *)clientbss->NetworkKey.key, pch, clientbss->NetworkKey.key_len + 1);
    } else if (index == 6) {
      /* Backhaul */
      if (strtoul(pch, NULL, 10)) {
	      clientbss->map_flag |= IEEE1905_MAP_FLAG_BACKHAUL;
      }
    } else if (index == 7) {
      /* Fronthaul */
      if (strtoul(pch, NULL, 10)) {
	      clientbss->map_flag |= IEEE1905_MAP_FLAG_FRONTHAUL;
      }
    }

    index++;
    if (index == 8) {
      i5TraceDirPrint("ALID["I5_MAC_DELIM_FMT"] band[0x%x] ssid_len[%d] ssid[%s] auth[0x%x] "
        "encr[0x%x] pwd_len[%d] Password[%s] map_flag[0x%x]\n",
        I5_MAC_PRM(clientbss->ALID), clientbss->band_flag, clientbss->ssid.SSID_len,
        clientbss->ssid.SSID, clientbss->AuthType, clientbss->EncryptType,
        clientbss->NetworkKey.key_len, clientbss->NetworkKey.key, clientbss->map_flag);
      ieee1905_glist_append(&i5_config.client_bssinfo_list, (dll_t*)clientbss);
      index = 0;
    }
    pch = strtok(NULL, " \r\n");
  }

  return;
end:
  if (clientbss) {
    free(clientbss);
  }
}

static void i5ControlProcessR2File(unsigned char *data, unsigned int len)
{
  char *pch;
  int index = 0;
  ieee1905_client_bssinfo_type *clientbss = NULL;

  i5DmGlistCleanup(&i5_config.client_bssinfo_list);
  ieee1905_glist_init(&i5_config.client_bssinfo_list);

  pch = strtok((char*)data, " \n");
  while (pch) {
    printf("index %d pch %s\n", index, pch);
    if (index == 0) {
      clientbss = (ieee1905_client_bssinfo_type *)malloc(sizeof(*clientbss));
      if (!clientbss) {
        i5TraceDirPrint("Malloc Failed\n");
        goto end;
      }
      memset(clientbss, 0, sizeof(*clientbss));
    }

    if (index == 0) {
      /* AL ID */
      if (NULL == i5String2MacAddr(pch, clientbss->ALID)) {
        i5TraceError("MAC Address must be of form xx:xx:xx:xx:xx:xx: %s\n", pch);
        goto end;
      }
    } else if (index == 1) {
      /* Op class */
      if (strncmp(pch, "8x", 2) == 0) {
        clientbss->band_flag = BAND_2G;
      } else if (strncmp(pch, "11x", 3) == 0) {
        clientbss->band_flag = BAND_5GL;
      } else if (strncmp(pch, "12x", 3) == 0) {
        clientbss->band_flag = BAND_5GH;
      } else {
        i5TraceError("Invalid op class : %s (valid are 8x, 11x and 12x)\n", pch);
        goto end;
      }
    } else if (index == 2) {
      /* SSID */
      clientbss->ssid.SSID_len = (unsigned char)strlen(pch);
      if (clientbss->ssid.SSID_len > IEEE1905_MAX_SSID_LEN) {
        i5TraceError("SSID is too long. len: %d max: %d SSID: %s\n",
          clientbss->ssid.SSID_len, IEEE1905_MAX_SSID_LEN, pch);
        goto end;
      }
      I5STRNCPY((char *)clientbss->ssid.SSID, pch, clientbss->ssid.SSID_len + 1);
    } else if (index == 3) {
      /* Auth */
      clientbss->AuthType = (unsigned short)strtoul(pch, NULL, 16);
    } else if (index == 4) {
      /* Encr */
      clientbss->EncryptType = (unsigned short)strtoul(pch, NULL, 16);
    } else if (index == 5) {
      /* Password */
      clientbss->NetworkKey.key_len = strlen(pch);
      if (clientbss->NetworkKey.key_len > IEEE1905_MAX_KEY_LEN) {
        i5TraceError("key is too long. len: %d max: %d key: %s\n",
          clientbss->NetworkKey.key_len, IEEE1905_MAX_KEY_LEN, pch);
        goto end;
      }
      I5STRNCPY((char *)clientbss->NetworkKey.key, pch, clientbss->NetworkKey.key_len + 1);
    } else if (index == 6) {
      /* Backhaul */
      if (strtoul(pch, NULL, 10)) {
	      clientbss->map_flag |= IEEE1905_MAP_FLAG_BACKHAUL;
      }
    } else if (index == 7) {
      /* Fronthaul */
      if (strtoul(pch, NULL, 10)) {
	      clientbss->map_flag |= IEEE1905_MAP_FLAG_FRONTHAUL;
      }
    } else if (index == 8) {
      if (strtoul(pch, NULL, 10)) {
	      clientbss->map_flag |= IEEE1905_MAP_FLAG_PROF1_DISALLOWED;
      }
    } else if (index == 9) {
      if (strtoul(pch, NULL, 10)) {
	      clientbss->map_flag |= IEEE1905_MAP_FLAG_PROF2_DISALLOWED;
      }
    }

    index++;
    if ((strcmp(pch, "</END>") == 0) && (index >= 8)) {
      i5TraceDirPrint("ALID["I5_MAC_DELIM_FMT"] band[0x%x] ssid_len[%d] ssid[%s] auth[0x%x] "
        "encr[0x%x] pwd_len[%d] Password[%s] map_flag[0x%x]\n",
        I5_MAC_PRM(clientbss->ALID), clientbss->band_flag, clientbss->ssid.SSID_len,
        clientbss->ssid.SSID, clientbss->AuthType, clientbss->EncryptType,
        clientbss->NetworkKey.key_len, clientbss->NetworkKey.key, clientbss->map_flag);
      ieee1905_glist_append(&i5_config.client_bssinfo_list, (dll_t*)clientbss);
      index = 0;
    }
    pch = strtok(NULL, " \n");
  }

  return;
end:
  if (clientbss) {
    free(clientbss);
  }
}

static void i5ControlSendFile(i5_socket_type *psock, t_I5_API_FILE *pMsg, int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  i5ControlProcessFile(pMsg->data, pMsg->data_len);

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

static void i5ControlSendApConfigRenew(i5_socket_type *psock, t_I5_API_RENEW_AP_CONFIG *pMsg,
  int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  if (pMsg->msgInfo.messageId != i5MessageApAutoconfigurationRenewValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
    goto end;
  }

  if (pMsg->role != 0) {
    i5TraceInfo("Only Registrar is allowed to send renew\n");
    goto end;
  }

  i5MessageApAutoconfigurationRenewSend(NULL, pMsg->rfband);
  _i5ControlSetCmdRespSendMsgType(&response);

end:
  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

static void i5ControlSendApMetricQuery(i5_socket_type *psock, t_I5_API_AP_METRIC_QUERY *pMsg,
  int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  if (pMsg->msgInfo.messageId != i5MessageAPMetricsQueryValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {
      i5MessageAPMetricsQuerySend(pDevice->psock, pMsg->msgInfo.macAddr, pMsg->bssids,
        pMsg->bssCount);
      _i5ControlSetCmdRespSendMsgType(&response);
    } else {
      i5TraceInfo("i5MessageAPMetricsQueryValue : Neighbor AL MAC Address "
        I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
    }
  }

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response));
}

static void i5ControlSendAssociatedSTALinkMetricQuery(i5_socket_type *psock,
  t_I5_API_ASSOC_STA_LINK_METRIC_QUERY *pMsg, int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  if (pMsg->msgInfo.messageId != i5MessageAssociatedSTALinkMetricsQueryValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {
      i5MessageAssociatedSTALinkMetricsQuerySend(pDevice->psock, pMsg->msgInfo.macAddr,
        pMsg->clientMAC);
      _i5ControlSetCmdRespSendMsgType(&response);
    } else {
        i5TraceInfo("i5MessageAssociatedSTALinkMetricsQueryValue : Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
    }
  }

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

static void i5ControlSendUnAssociatedSTALinkMetricQuery(i5_socket_type *psock,
	t_I5_API_UNASSOC_STA_LINK_METRIC_QUERY *pMsg, int cmd)
{
 t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};
 ieee1905_unassoc_sta_link_metric_query query;
 unassoc_query_per_chan_rqst *per_chan_rqst = NULL;
 uint8 i;

 memset(&query, 0, sizeof(query));
 if (pMsg->msgInfo.messageId != i5MessageUnAssociatedSTALinkMetricsQueryValue) {
   i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
 } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {

    i5TraceInfo("Unassoc link metric query\n");
    /* Populate information from CLI to the query structure, current supported
     * CLI command format is: AL_MAC : OPCLASS : CHANNEL : CLIENT MAC
     */
    query.opClass = pMsg->opclass;
    /* Currently only one channel per request supported at controller */
    query.chCount = 1;

    i5TraceInfo("oplass=%d,chCount=%d\n",query.opClass, query.chCount);
    if (!query.chCount) {
      i5TraceDirPrint("No list provided\n");
      goto end;
    }
    query.data = (unassoc_query_per_chan_rqst*)malloc(sizeof(unassoc_query_per_chan_rqst) * query.chCount);
    if (!query.data) {
      i5TraceDirPrint("Memory malloc failed \n");
      goto end;
    }
    per_chan_rqst = query.data;
      /* process data in order:
       * ----------------------------------------------------------------------
       * chan(1 byte)| number_of_sta(1 byte) | list of Mac address (variable) |
       * ----------------------------------------------------------------------
       *
       */
    for(i = 0; i < query.chCount; i++) {
      per_chan_rqst[i].chan = pMsg->channel;
      per_chan_rqst[i].n_sta = 1; /* currently 1 sta for 1 channel request support at controller */
      i5TraceInfo("chan=%d,n_sta=%d\n", per_chan_rqst[i].chan, per_chan_rqst[i].n_sta);
      if (per_chan_rqst[i].n_sta) {
        per_chan_rqst[i].mac_list = (unsigned char *)malloc(per_chan_rqst[i].n_sta * MAC_ADDR_LEN);
	if (!per_chan_rqst[i].mac_list) {
	  i5TraceDirPrint("Memory malloc failed \n");
	  goto end;
	}
	memcpy(per_chan_rqst[i].mac_list, &(pMsg->clientMAC), (per_chan_rqst[i].n_sta * MAC_ADDR_LEN));
	i5TraceInfo("copied MAC =" I5_MAC_DELIM_FMT"\n", I5_MAC_PRM(per_chan_rqst[i].mac_list));
      }
    }

      i5MessageUnAssociatedSTALinkMetricsQuerySend(pDevice->psock, pMsg->msgInfo.macAddr, &query);
      _i5ControlSetCmdRespSendMsgType(&response);
    } else {
        i5TraceInfo("i5MessageUnAssociatedSTALinkMetricsQueryValue : Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
    }
 }

 i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
end:
  /* free allocated data */
  per_chan_rqst = query.data;
  for(i = 0; i < query.chCount; i++) {
    if (per_chan_rqst[i].mac_list) {
      free(per_chan_rqst[i].mac_list);
    }
  }
  if (query.data) {
    free(query.data);
  }
}

static int i5ControlPrepareBeaconMetricQuery(t_I5_API_BEACON_METRIC_QUERY *pMsg,
  ieee1905_beacon_request *query)
{
  unsigned char *buf = NULL;
  unsigned char idx = 0;
  int count, ap_chan_report_len = 0, tmp_len, ret = 0, buf_len = 0;

  memcpy(query->sta_mac, pMsg->sta_mac, 6);
  query->opclass = pMsg->opclass;
  query->channel = pMsg->channel;
  memcpy(query->bssid, pMsg->bssid, 6);
  if (pMsg->subelements_len <= 0) {
    goto end;
  }

  /* Convert sub elements to binary */
  buf = (unsigned char*)malloc(pMsg->subelements_len/2);
  if (buf == NULL) {
    i5TraceDirPrint("Failed to allocate memory for subelements\n");
    ret = -1;
    goto end;
  }

  for (count = 0; count < (pMsg->subelements_len / 2); count++) {
    unsigned int tmp;
    sscanf((char *)(pMsg->subelements + 2 * count), "%02x", &tmp);
    buf[count] =  (unsigned char)tmp;
  }
  buf_len = pMsg->subelements_len / 2;

  /* First octet is reporting detail */
  query->reporting_detail = buf[idx]; idx++;
  /* Read SSID Length and SSID */
  query->ssid.SSID_len = buf[idx]; idx++;
  if (query->ssid.SSID_len > 0) {
    memcpy(query->ssid.SSID, &buf[idx], query->ssid.SSID_len); idx += query->ssid.SSID_len;
  }

  if (buf_len <= idx) {
    i5TraceInfo("No sub elements\n");
    goto end;
  }

  /* Read AP channel report length */
  query->ap_chan_report_count = buf[idx]; idx++;
  if (query->ap_chan_report_count > 0) {
    for (count = 0; count < query->ap_chan_report_count; count++) {
      tmp_len = buf[idx]; idx++;
      ap_chan_report_len += tmp_len + 1;
      idx += tmp_len;
    }
    idx -= ap_chan_report_len;
    query->ap_chan_report_len = ap_chan_report_len;
    query->ap_chan_report = (unsigned char*)malloc(ap_chan_report_len);
    if (query->ap_chan_report == NULL) {
      i5TraceDirPrint("Failed to allocate memory for ap_chan_report\n");
      ret = -1;
      goto end;
    }
    memcpy(query->ap_chan_report, &buf[idx], ap_chan_report_len);
    idx += ap_chan_report_len;
  }

  if (buf_len <= idx) {
    i5TraceInfo("No element IDs\n");
    goto end;
  }

  /* read element IDs count and element IDs */
  query->element_ids_count = buf[idx]; idx++;
  if (query->element_ids_count > 0) {
    query->element_list = (unsigned char*)malloc(query->element_ids_count);
    if (query->element_list == NULL) {
      i5TraceDirPrint("Failed to allocate memory for element_list\n");
      ret = -1;
      goto end;
    }
    memcpy(query->element_list, &buf[idx], query->element_ids_count);
  }

end:
  if (buf) {
    free(buf);
  }
  return ret;
}

static void i5ControlSendBeaconMetricQuery(i5_socket_type *psock, t_I5_API_BEACON_METRIC_QUERY *pMsg,
  int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  if (pMsg->msgInfo.messageId != i5MessageBeaconMetricsQueryValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    ieee1905_beacon_request query;

    if (pDevice && pDevice->psock) {
      memset(&query, 0, sizeof(query));
      if (i5ControlPrepareBeaconMetricQuery(pMsg, &query) == 0) {
        i5MessageBeaconMetricsQuerySend(pDevice->psock, pMsg->msgInfo.macAddr, &query);
        _i5ControlSetCmdRespSendMsgType(&response);
      }
      if (query.ap_chan_report) {
        free(query.ap_chan_report);
      }
      if (query.element_list) {
        free(query.element_list);
      }
    } else {
      i5TraceInfo("i5MessageAPMetricsQueryValue : Neighbor AL MAC Address "
        I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
    }
  }

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response));
}

#define CAPI_TAG_DEV_SET_CONFIG "dev_set_config"
#define CAPI_TAG_DEV_SEND_1905  "dev_send_1905"
#define CAPI_TAG_DESTALID       "DestALid"
#define CAPI_TAG_MESSAGE_TYPE   "MessageTypeValue"
#define CAPI_TAG_TLV_TYPE       "tlv_type"
#define CAPI_TAG_TLV_LENGTH     "tlv_length"
#define CAPI_TAG_TLV_VALUE      "tlv_value"

/* Remove all the brackets, spaces and Hex prefix(0x) from the buffer */
static void i5ControlCAPIRemoveBracketsSpaceHex(char *data, char *outdata, int *out_len)
{
  int i = 0, j = 0, len = strlen(data);

  for (i = 0; i < len; i++) {
    if (data[i] == '}' || data[i] == '{' || data[i] == ' ' || data[i] == ':') {
      continue;
    } else if (data[i] == 'x' && data[i-1] == '0') {
      /* If hex character, remove the previous 0 */
      j--;
      continue;
    }
    outdata[j++] = data[i];
  }

  if (out_len) {
    *out_len = j;
  }
}

#define I5_CAPI_CMP strcasecmp

/* dev_set_config CAPI command handling. This data will have traffic separation policy in TLV
 * format and BSS info
 */
static int i5CtlDevSetConfig(char *in_data, unsigned int data_len)
{
  char *data = NULL, *token = NULL, *bss_data = NULL;
  unsigned char tlvtype = 0;
  unsigned short len = 0;
  int count, ret = 0, is_ts_policy = 0, bss_info_idx = 1, is_policy_freed = 0;

  data = (char*)malloc(sizeof(char) * (data_len + 1));
  if (data == NULL) {
    i5TraceDirPrint("Failed to allocate data\n");
    ret = -1;
    goto end;
  }
  memset(data, 0, (data_len + 1));
  memcpy(data, in_data, data_len);

  /* Allocate memory to hold each BSS data. So that we can parse everything together in the end */
  bss_data = (char*)malloc(sizeof(char) * (data_len + 100));
  if (bss_data == NULL) {
    i5TraceDirPrint("Failed to allocate bss_data\n");
    ret = -1;
    goto end;
  }
  memset(bss_data, 0, (data_len + 100));

  token = strtok(data, ",");

  while (token != NULL) {
    char bss_name[100];

    i5TraceDirPrint("token[%s]\n", token);
    /* Each BSS name will be like bss_info1, bss_info2..., bss_infon */
    snprintf(bss_name, sizeof(bss_name), "bss_info%d", bss_info_idx);

    if (strstr(token, bss_name) != 0) {
      bss_info_idx++;
      token = strtok(NULL, ",");
      if (token) {
        /* Add the bss to buffer with new line character */
        snprintf(bss_data+strlen(bss_data), (data_len+1), "%s </END>\n", token);
        i5TraceDirPrint("After %s : bss_data[%s]\n", bss_name, bss_data);
      }
    } else if (strstr(token, CAPI_TAG_TLV_TYPE) != 0) {
      token = strtok(NULL, ",");
      if (token) {
        tlvtype = (unsigned char)strtoul(token, NULL, 0);
        i5TraceDirPrint("TLV Type %s %d\n", token, tlvtype);
      }
    } else if (strstr(token, CAPI_TAG_TLV_LENGTH) != 0) {
      token = strtok(NULL, ",");
      if (token) {
        len = (unsigned short)strtoul(token, NULL, 0);
        i5TraceDirPrint("TLV Length %s %d\n", token, len);
      }
    } else if (strstr(token, CAPI_TAG_TLV_VALUE) != 0) {
      token = strtok(NULL, ",");
      if (token) {
        char outdata[I5_PACKET_BUF_LEN] = {0};
        int outdata_len;

        i5TraceDirPrint("TLV Value %s\n", token);
        i5ControlCAPIRemoveBracketsSpaceHex(token, outdata, &outdata_len);
        i5TraceDirPrint("TLV Value With Removed Brackets[Len:%d] %s\n", outdata_len/2, outdata);

        if (tlvtype == 0xb5 || tlvtype == 0xb6) {
          i5TraceDirPrint("Traffic Separation Policy TLV present\n");
          is_ts_policy = 1;
        }

        /* If the TLV is basic 802.1q settings */
        if (tlvtype == 0xb5) {
          unsigned int vlan_id, idx = 0, def_pcp;

          /* VLAN ID */
          sscanf((char*)(outdata + idx), "%04x", &vlan_id);
          idx += 4;
          /* Default PCP */
          sscanf((char*)(outdata + idx), "%02x", &def_pcp);
          idx += 2;
          i5TraceDirPrint("VLANID[%d] DefaultPCP[%d]\n", vlan_id, def_pcp);
          i5_config.policyConfig.prim_vlan_id = (unsigned short)vlan_id;
          i5_config.policyConfig.default_pcp = (unsigned char)def_pcp;
          i5_config.flags |= I5_CONFIG_FLAG_TS_ACTIVE;
        } else if (tlvtype == 0xb6) { /* Traffic separation policy TLV */
          unsigned int ssid_cnt, ssid_len, vlan_id, idx = 0;
          ieee1905_ssid_type ssid;

          /* First byte is number of SSIDs */
          sscanf((char*)(outdata + idx), "%02x", &ssid_cnt);
          idx += 2;
          i5TraceDirPrint("SSID count[%d]\n", ssid_cnt);

          /* For each SSID in the TLV */
          for (count = 0; count < ssid_cnt; count++) {
            ieee1905_ts_policy_t *ts_policy;

            memset(&ssid, 0, sizeof(ssid));

            /* frst byte is SSID len */
            sscanf((char*)(outdata + idx), "%02x", &ssid_len);
            idx += 2;
            ssid.SSID_len = (unsigned char)ssid_len;
            /* next is SSID which is in character. so copy as it is */
            memcpy(ssid.SSID, (outdata + idx), ssid_len);
            idx += ssid_len;
            /* Next is VLAN ID */
            sscanf((char*)(outdata + idx), "%04x", &vlan_id);
            idx += 4;
            i5TraceDirPrint("SSID[%s] SSID Len[%d] VLANID[%d]\n", ssid.SSID, ssid.SSID_len, vlan_id);

            if (!is_policy_freed) {
              /* Clean up before filling the latest data. */
              i5DmTSPolicyCleanup(&i5_config.policyConfig.ts_policy_list);
              ieee1905_glist_init(&i5_config.policyConfig.ts_policy_list);
              is_policy_freed = 1;
            }

            ts_policy = i5DmAddVLANIDToList(&i5_config.policyConfig.ts_policy_list, vlan_id);
            if (ts_policy == NULL) {
              ret = -1;
              goto end;
            }
            i5DmAddSSIDToList(&i5_config.policyConfig.ts_policy_list, &ts_policy->ssid_list, &ssid);
          }
        }
      }
    }
    token = strtok(NULL, ",");
  }

  /* Process the BSS info data */
  i5ControlProcessR2File((unsigned char*)bss_data, data_len);

end:
  if (is_ts_policy) {
    i5GluePrintTSPolicy();
  }

  if (data) {
    free(data);
  }

  if (bss_data) {
    free(bss_data);
  }

  /* If the extraction failed and traffic separation policy is initialized, then free it */
  if ((ret == -1) && (is_policy_freed)) {
    i5DmTSPolicyCleanup(&i5_config.policyConfig.ts_policy_list);
    ieee1905_glist_init(&i5_config.policyConfig.ts_policy_list);
  }

  return ret;
}

/* Create the message and TLVs based on the dev_send_1905 CAPI command */
static int i5ControlParseAndSendCAPICmd(char *in_data, unsigned int data_len)
{
  char *data = NULL, *token = NULL;
  i5_message_type *pmsg = NULL;
  unsigned char destALID[MAC_ADDR_LEN];
  i5_dm_device_type *pDevice;
  unsigned char *pmem = NULL;
  unsigned char tlvtype;
  unsigned short len, msgtype = 0;
  i5_tlv_t *ptlv = NULL;
  int count, ret = -1, send_to_all = 0, is_ts_policy = 0;

  data = (char*)malloc(sizeof(char) * (data_len + 1));
  memset(data, 0, (data_len + 1));
  memcpy(data, in_data, data_len);
  i5TraceDirPrint("Len[%d] data %s\n", data_len, data);

  token = strtok(data, ",");

  if (I5_CAPI_CMP(token, CAPI_TAG_DEV_SET_CONFIG) == 0) {
    i5CtlDevSetConfig(in_data, data_len);
    i5TraceDirPrint("Dev Set Config command\n");
    goto end;
  }

  if (I5_CAPI_CMP(token, CAPI_TAG_DEV_SEND_1905) != 0) {
    i5TraceDirPrint("Invalid Command %s\n", token);
    goto end;
  }

  while (token != NULL) {
    if (I5_CAPI_CMP(token, CAPI_TAG_DESTALID) == 0) {
      token = strtok(NULL, ",");
      if (token) {
        if (pmsg) {
          i5TraceDirPrint("pmsg is already present. Free it\n");
          i5MessageFree(pmsg);
        }
        i5TraceDirPrint("Destination AL ID %s\n", token);
        if (i5String2MacAddr(token, destALID) == NULL) {
          i5TraceDirPrint("Failed to convert Destination AL ID\n");
          goto end;
        }

        pDevice = i5DmDeviceFind(&destALID[0]);
        if (pDevice && pDevice->psock) {
          pmsg = i5MessageCreate(pDevice->psock, &destALID[0], I5_PROTO);
          if (pmsg == NULL) {
            i5TraceDirPrint("Failed to create pmsg for Neighbor["I5_MAC_DELIM_FMT"] not found\n",
              I5_MAC_PRM(destALID));
            goto end;
          }
        } else {
          send_to_all = 1;
          pmsg = i5MessageCreate(NULL, &destALID[0], I5_PROTO);
          if (!pDevice) {
            i5TraceDirPrint("Neighbor["I5_MAC_DELIM_FMT"] not found\n", I5_MAC_PRM(destALID));
          } else {
            i5TraceDirPrint("Neighbor["I5_MAC_DELIM_FMT"] devices socket still not exists\n",
              I5_MAC_PRM(destALID));
          }
        }
      }
    } else if (I5_CAPI_CMP(token, CAPI_TAG_MESSAGE_TYPE) == 0) {
      if (!pmsg) {
        i5TraceDirPrint("Message Not Created\n");
        goto end;
      }
      token = strtok(NULL, ",");
      if (token) {
        msgtype = strtoul(token, NULL, 0);
        i5TraceDirPrint("Message Type %s %d\n", token, msgtype);
        i5_config.last_message_identifier++;
        i5PacketHeaderInit(pmsg->ppkt, msgtype, i5_config.last_message_identifier);
      }
    } else if (strstr(token, CAPI_TAG_TLV_TYPE) != 0) {
      if (!pmsg) {
        i5TraceDirPrint("Message Not Created\n");
        goto end;
      }
      token = strtok(NULL, ",");
      if (token) {
        tlvtype = (unsigned char)strtoul(token, NULL, 0);
        i5TraceDirPrint("TLV Type %s %d\n", token, tlvtype);
        if (pmem) {
          i5TraceDirPrint("TLV is already present. Free it and create new\n");
          free(pmem);
          pmem = NULL;
        }
        if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
          i5TraceDirPrint("malloc error for TLV %d\n", tlvtype);
          goto end;
        }
        ptlv = (i5_tlv_t *)pmem;
        ptlv->type = tlvtype;
      }
    } else if (strstr(token, CAPI_TAG_TLV_LENGTH) != 0) {
      if (!pmsg) {
        i5TraceDirPrint("Message Not Created\n");
        goto end;
      }
      token = strtok(NULL, ",");
      if (token) {
        len = (unsigned short)strtoul(token, NULL, 0);
        i5TraceDirPrint("TLV Length %s %d\n", token, len);
        if (pmem == NULL || ptlv == NULL) {
          i5TraceDirPrint("Failed to parse TLV\n");
          goto end;
        }
        ptlv->length = htons(len);
      }
    } else if (strstr(token, CAPI_TAG_TLV_VALUE) != 0) {
      if (!pmsg) {
        i5TraceDirPrint("Message Not Created\n");
        goto end;
      }
      token = strtok(NULL, ",");
      if (token) {
        char outdata[I5_PACKET_BUF_LEN] = {0};
        int outdata_len, pmem_idx = sizeof(i5_tlv_t);

        if (pmem == NULL) {
          i5TraceDirPrint("Failed to parse TLV\n");
          goto end;
        }
        i5TraceDirPrint("TLV Value %s\n", token);
        i5ControlCAPIRemoveBracketsSpaceHex(token, outdata, &outdata_len);
        i5TraceDirPrint("TLV Value With Removed Brackets[Len:%d] %s\n", outdata_len/2, outdata);

        if (ptlv->type == 0xb5 || ptlv->type == 0xb6) {
          i5TraceDirPrint("Traffic Separation Policy TLV present\n");
          is_ts_policy = 1;
        }

        /* if the TLV is traffic separation policy, then the SSID will be plain text */
        if (ptlv->type == 0xb6) {
          unsigned int ssid_cnt, ssid_len, vlan_id, idx = 0;

          /* First byte is number of SSIDs */
          sscanf((char*)(outdata + idx), "%02x", &ssid_cnt);
          idx += 2;
          pmem[pmem_idx++] = (unsigned char)ssid_cnt;
          i5TraceDirPrint("SSID count[%d]\n", ssid_cnt);

          /* This is a plugfest requirement. First UCC will configure the traffic separation policy
           * using WSC messages. Then for Agent1, UCC will send policy config with 0 SSID so that
           * the Agent1 removes traffic separation. As the controller has all the policy config
           * we should disable it here. or else it will send it in WSC message to agent1 again
           */
          if (ssid_cnt == 0) {
            i5_config.flags &= ~I5_CONFIG_FLAG_TS_ACTIVE;
          }

          for (count = 0; count < ssid_cnt; count++) {
            /* frst byte is SSID len */
            sscanf((char*)(outdata + idx), "%02x", &ssid_len);
            idx += 2;
            pmem[pmem_idx++] = (unsigned char)ssid_len;
            /* next is SSID which is in character. so copy as it is */
            memcpy(&pmem[pmem_idx], (outdata + idx), ssid_len);
            idx += ssid_len;
            pmem_idx += ssid_len;
            /* Next is VLAN ID */
            sscanf((char*)(outdata + idx), "%04x", &vlan_id);
            idx += 4;
            *((unsigned short *)&pmem[pmem_idx]) = htons(vlan_id);
            pmem_idx += 2;
            i5TraceDirPrint("SSID Len[%d] VLANID[%d]\n", ssid_len, vlan_id);
          }
        } else {
          for (count = 0; count < outdata_len/2; count++) {
            unsigned int tmp;
            sscanf((char *)(outdata + 2 * count), "%02x", &tmp);
            pmem[pmem_idx++] = (unsigned char)tmp;
          }
        }
        i5TraceDirPrint("After Insert TLV Len[%d]\n", pmem_idx);
        i5MessageInsertTlv(pmsg, pmem, pmem_idx);
        free(pmem);
        pmem = NULL;
      }
    }

    token = strtok(NULL, ",");
  }

  if (pmsg) {
    i5TlvEndOfMessageTypeInsert(pmsg);
    if (send_to_all) {
      i5_socket_type *pif;

      pif = (i5_socket_type *)i5_config.i5_socket_list.ll.next;
      while (pif != NULL) {
        if (pif->type == i5_socket_type_ll) {
          i5TraceDirPrint("For Neighbor["I5_MAC_DELIM_FMT"] Sending on Ifname[%s]\n",
            I5_MAC_PRM(destALID), pif->u.sll.ifname);
          pmsg->psock = pif;
          i5VendorInformMessageSend(&destALID[0], pmsg, msgtype, NULL);
          i5MessageSend(pmsg, 0);
        }
        pif = pif->ll.next;
      }
    } else {
      i5VendorInformMessageSend(&destALID[0], pmsg, msgtype, NULL);
      i5MessageSend(pmsg, 0);
    }

    if (i5_config.flags & I5_CONFIG_FLAG_TS_ACTIVE) {
      i5TraceDirPrint("TS policy active.[%d]\n", is_ts_policy);
    }
    i5GluePrintTSPolicy();

    ret = 0;
  }

end:
  if (pmem) {
    free(pmem);
  }
  if (pmsg) {
    i5MessageFree(pmsg);
  }
  if (data) {
    free(data);
  }

  return ret;
}

static void i5ControlSendCAPI(i5_socket_type *psock, t_I5_API_FILE *pMsg, int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};

  if (i5ControlParseAndSendCAPICmd((char*)pMsg->data, pMsg->data_len) == 0) {
    _i5ControlSetCmdRespSendMsgType(&response);
  }

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
}

#if defined(MULTIAPR2)
/* Send channel scan request to neighbor. Create the request based on the channel scan capability
 * received from the nieghbor
 */
static void i5ControlSendChannelScanRequest(i5_socket_type *psock, t_I5_API_CH_SCAN_REQ *pMsg)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};
  i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
  ieee1905_chscan_req_msg chscan_req;
  ieee1905_per_radio_opclass_list *radio_info = NULL;
  ieee1905_per_opclass_chan_list *opclass_info_1 = NULL;
  int num_of_radios = 0;
  i5_dm_interface_type *ifr = NULL;

  memset(&chscan_req, 0, sizeof(chscan_req));

  /* Initialize radio list */
  ieee1905_glist_init(&chscan_req.radio_list);

  if (pDevice == NULL) {
    i5TraceError("Neighbor device["I5_MAC_DELIM_FMT"] not found\n", I5_MAC_PRM(pDevice->DeviceId));
    return;
  }

  /* Insert Channel Scan Request TLV Flags */
  if (pMsg->type) {
    chscan_req.chscan_req_msg_flag |= MAP_CHSCAN_REQ_FRESH_SCAN;
  }

  /* Insert details for each radio */
  foreach_i5glist_item(ifr, i5_dm_interface_type, pDevice->interface_list) {
    /* Loop for, only for wireless interfaces with valid chanspec & BSSes */
    if (!i5DmIsInterfaceWireless(ifr->MediaType)) {
      i5TraceInfo("Not a wireless interface "I5_MAC_DELIM_FMT"\n",
        I5_MAC_PRM(ifr->InterfaceId));
      continue;
    }

    if (!ifr->ApCaps.ChScanCaps.List || ifr->ApCaps.ChScanCaps.Valid == 0) {
      i5TraceDirPrint("Channel Scan Capability Not Present for the Radio "I5_MAC_DELIM_FMT"\n",
        I5_MAC_PRM(ifr->InterfaceId));
        continue;
    }

    radio_info = (ieee1905_per_radio_opclass_list*)malloc(sizeof(*radio_info));
    memset(radio_info, 0, sizeof(*radio_info));
    ieee1905_glist_init(&radio_info->opclass_list);
    memcpy(radio_info->radio_mac, ifr->InterfaceId, MAC_ADDR_LEN);

    if (pMsg->type) {
      int idx = 0, rcount = 0, idx_rclass, chcount;

      rcount = radio_info->num_of_opclass = ifr->ApCaps.ChScanCaps.List[idx++];
      for (idx_rclass = 0; idx_rclass < rcount; idx_rclass++) {
        opclass_info_1 = (ieee1905_per_opclass_chan_list *)malloc(sizeof(*opclass_info_1));
        memset(opclass_info_1, 0, sizeof(*opclass_info_1));

        opclass_info_1->opclass_val = ifr->ApCaps.ChScanCaps.List[idx++];
        /* Send 0 as number of channels so that the agent  sends channel scan report for all the
         * channels it supports
         */
        opclass_info_1->num_of_channels = 0;
        chcount = (unsigned char)ifr->ApCaps.ChScanCaps.List[idx++];
        idx += chcount;

        ieee1905_glist_append(&radio_info->opclass_list, (dll_t*)opclass_info_1);
        i5TraceDirPrint("IFR["I5_MAC_DELIM_FMT"] Rclass[%d] Channel Count[%d]\n",
          I5_MAC_PRM(ifr->InterfaceId), opclass_info_1->opclass_val,
          opclass_info_1->num_of_channels);
      }
    } else {
      radio_info->num_of_opclass = 0;
    }
    i5TraceDirPrint("opclass count[%d]\n", radio_info->num_of_opclass);
    ieee1905_glist_append(&chscan_req.radio_list, (dll_t*)radio_info);
    num_of_radios++;
  }

  chscan_req.num_of_radios = num_of_radios;

  if (pDevice && pDevice->psock) {
    i5MessageChannelScanRequestSend(pDevice, &chscan_req);
    _i5ControlSetCmdRespSendMsgType(&response);
  } else {
    i5TraceInfo("i5ControlSendChannelScanRequest : Neighbor AL MAC Address "
      I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
  }

  i5apiSendMessage(psock->sd, 44, &response, sizeof(response) );

  i5DmChannelScanRequestInfoFree(&chscan_req);
}

/* Send CAC request message to agent from controller */
static void i5ControlSendCACRequest(i5_socket_type *psock, t_I5_API_CAC_REQUEST *pMsg, int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};
  ieee1905_cac_rqst_list_t *cac_rqst = NULL;
  ieee1905_radio_cac_rqst_t *radio_params = NULL;

  if (pMsg->msgInfo.messageId != i5MessageCACRequestValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {

      i5TraceInfo("CAC request, for n_radio[%d] \n", pMsg->count);

      cac_rqst = (ieee1905_cac_rqst_list_t*)malloc(pMsg->count *
		    (sizeof(ieee1905_radio_cac_rqst_t)));

      if (!cac_rqst) {
        i5TraceDirPrint("Memory malloc failed \n");
        goto end;
      }

      cac_rqst->count = pMsg->count;
      if (!cac_rqst->count) {
        i5TraceDirPrint("No CAC info for any radio, exit \n");
        goto end;
      }

      i5TraceInfo("n_radio[%d]\n", cac_rqst->count);

      radio_params = &cac_rqst->params[0];

      memcpy(radio_params, (uchar*)pMsg->params,
	(cac_rqst->count * sizeof(ieee1905_radio_cac_rqst_t)));
      prhex("radio params for CAC request : \n", (uchar*)radio_params,
	(cac_rqst->count * sizeof(ieee1905_radio_cac_rqst_t)));

      i5MessageCACRequestSend(pDevice, cac_rqst);
      _i5ControlSetCmdRespSendMsgType(&response);
    } else {
        i5TraceInfo("i5MessageCACRequestValue : Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
    }
  }

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response) );
end:
  if (cac_rqst) {
    free(cac_rqst);
  }
}

/* Send CAC termination message to agent from controller */
static void i5ControlSendCACTermination(i5_socket_type *psock, t_I5_API_CAC_TERMINATION *pMsg, int cmd)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};
  ieee1905_cac_termination_list_t *cac_termination = NULL;
  ieee1905_radio_cac_params_t *radio_params = NULL;

  if (pMsg->msgInfo.messageId != i5MessageCACTerminationValue) {
    i5TraceInfo("Unhandled Message Type %d\n", pMsg->msgInfo.messageId);
  } else {
    i5_dm_device_type *pDevice = i5DmDeviceFind(pMsg->msgInfo.macAddr);
    if (pDevice && pDevice->psock) {
      i5TraceInfo("CAC Terminate \n");

      cac_termination = (ieee1905_cac_termination_list_t*)malloc(pMsg->count *
	    (sizeof(ieee1905_radio_cac_params_t)));

      if (!cac_termination) {
        i5TraceDirPrint("Memory malloc failed \n");
        goto end;
      }

      cac_termination->count = pMsg->count;
      if (!cac_termination->count) {
        i5TraceDirPrint("No CAC info for any radio, exit \n");
        goto end;
      }

      i5TraceInfo("n_radio[%d]\n", cac_termination->count);

      radio_params = &cac_termination->params[0];

      memcpy(radio_params, (uchar*)pMsg->params,
	(cac_termination->count * sizeof(ieee1905_radio_cac_rqst_t)));
      prhex("radio params for CAC termination: \n", (uchar*)radio_params,
	(cac_termination->count * sizeof(ieee1905_radio_cac_rqst_t)));

      i5MessageCACTerminationSend(pDevice, cac_termination);
     _i5ControlSetCmdRespSendMsgType(&response);
    } else {
        i5TraceInfo("i5MessageCACTerminateValue: Neighbor AL MAC Address "
          I5_MAC_DELIM_FMT " Not Found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
    }
  }

  i5apiSendMessage(psock->sd, cmd, &response, sizeof(response));

end:
  if (cac_termination) {
    free(cac_termination);
  }
}

/* Set or unset Association Disallowed Attribute in Beacon */
static void i5ControlAssocDisallowRequest(i5_socket_type *psock, t_I5_API_ASSOC_DISALLOW_REQ *pMsg)
{
  t_I5_CTL_CMDRESP_SEND_MSG_TYPE response = {};
  i5_dm_device_type *pDevice;
  i5_dm_bss_type *pbss;

  pDevice = i5DmGetSelfDevice();

  pbss = i5DmFindBSSFromDevice(pDevice, pMsg->msgInfo.macAddr);
  if (pbss == NULL) {
    i5TraceDirPrint("BSSID["I5_MAC_DELIM_FMT"] Not found\n", I5_MAC_PRM(pMsg->msgInfo.macAddr));
    return;
  }

  if (i5_config.cbs.mbo_assoc_disallowed) {
    i5TraceInfo("ifname[%s] reason[%d]\n", pbss->ifname, pMsg->reason);
    i5_config.cbs.mbo_assoc_disallowed(pbss->ifname, (unsigned char)pMsg->reason);
  }

  i5apiSendMessage(psock->sd, 0, &response, sizeof(response));
}
#endif /* MULTIAPR2 */
#endif /* MULTIAP */
