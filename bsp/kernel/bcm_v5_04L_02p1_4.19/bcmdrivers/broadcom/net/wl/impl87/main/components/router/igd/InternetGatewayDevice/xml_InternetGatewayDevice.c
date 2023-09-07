/*
 * Broadcom IGD module, xml_InternetGatewayDevice.c
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
 * $Id: xml_InternetGatewayDevice.c 551827 2015-04-24 08:59:54Z $
 */

char xml_InternetGatewayDevice[] =
	"<?xml version=\"1.0\"?>\r\n"
	"<root "
	"xmlns=\"urn:schemas-upnp-org:device-1-0\" "
	"xmlns:pnpx=\"http://schemas.microsoft.com/windows/pnpx/2005/11\" "
	"xmlns:df=\"http://schemas.microsoft.com/windows/2008/09/devicefoundation\">\r\n"
	"\t<specVersion>\r\n"
	"\t\t<major>1</major>\r\n"
	"\t\t<minor>0</minor>\r\n"
	"\t</specVersion>\r\n"
	"\t<device>\r\n"
	"\t\t<pnpx:X_hardwareId>BCM947XX_VEN_0033&amp;DEV_0008&amp;REV_01</pnpx:X_hardwareId>\r\n"
	"\t\t<pnpx:X_compatibleId>urn:schemas-upnp-org:device:InternetGatewayDevice:1</pnpx:X_compatibleId>\r\n"
	"\t\t<pnpx:X_deviceCategory>NetworkInfrastructure.Router</pnpx:X_deviceCategory>\r\n"
	"\t\t<df:X_deviceCategory>Network.Router.Wireless</df:X_deviceCategory>\r\n"
	"\t\t<deviceType>urn:schemas-upnp-org:device:InternetGatewayDevice:1</deviceType>\r\n"
	"\t\t<friendlyName>router</friendlyName>\r\n"
	"\t\t<manufacturer>Broadcom</manufacturer>\r\n"
	"\t\t<manufacturerURL>http://www.broadcom.com</manufacturerURL>\r\n"
	"\t\t<modelDescription>Gateway</modelDescription>\r\n"
	"\t\t<modelName>router</modelName>\r\n"
	"\t\t<modelNumber>947xx</modelNumber>\r\n"
	"\t\t<serialNumber>0000001</serialNumber>\r\n"
	"\t\t<modelURL>http://www.broadcom.com</modelURL>\r\n"
	"\t\t<UDN>uuid:eb9ab5b2-981c-4401-a20e-b7bcde359dbb</UDN>\r\n"
	"\t\t<serviceList>\r\n"
	"\t\t\t<service>\r\n"
	"\t\t\t\t<serviceType>urn:schemas-upnp-org:service:Layer3Forwarding:1</serviceType>\r\n"
	"\t\t\t\t<serviceId>urn:upnp-org:serviceId:L3Forwarding1</serviceId>\r\n"
	"\t\t\t\t<SCPDURL>x_layer3forwarding.xml</SCPDURL>\r\n"
	"\t\t\t\t<controlURL>control?Layer3Forwarding</controlURL>\r\n"
	"\t\t\t\t<eventSubURL>event?Layer3Forwarding</eventSubURL>\r\n"
	"\t\t\t</service>\r\n"
	"\t\t</serviceList>\r\n"
	"\t\t<deviceList>\r\n"
	"\t\t\t<device>\r\n"
	"\t\t\t\t<deviceType>urn:schemas-upnp-org:device:WANDevice:1</deviceType>\r\n"
	"\t\t\t\t<friendlyName>WANDevice</friendlyName>\r\n"
	"\t\t\t\t<manufacturer>Broadcom</manufacturer>\r\n"
	"\t\t\t\t<manufacturerURL>http://www.broadcom.com</manufacturerURL>\r\n"
	"\t\t\t\t<modelDescription>Gateway</modelDescription>\r\n"
	"\t\t\t\t<modelName>router</modelName>\r\n"
	"\t\t\t\t<modelURL>http://www.broadcom.com</modelURL>\r\n"
	"\t\t\t\t<UDN>uuid:e1f05c9d-3034-4e4c-af82-17cdfbdcc077</UDN>\r\n"
	"\t\t\t\t<serviceList>\r\n"
	"\t\t\t\t\t<service>\r\n"
	"\t\t\t\t\t\t<serviceType>urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1</serviceType>\r\n"
	"\t\t\t\t\t\t<serviceId>urn:upnp-org:serviceId:WANCommonIFC1</serviceId>\r\n"
	"\t\t\t\t\t\t<SCPDURL>x_wancommoninterfaceconfig.xml</SCPDURL>\r\n"
	"\t\t\t\t\t\t<controlURL>control?WANCommonInterfaceConfig</controlURL>\r\n"
	"\t\t\t\t\t\t<eventSubURL>event?WANCommonInterfaceConfig</eventSubURL>\r\n"
	"\t\t\t\t\t</service>\r\n"
	"\t\t\t\t</serviceList>\r\n"
	"\t\t\t\t<deviceList>\r\n"
	"\t\t\t\t\t<device>\r\n"
	"\t\t\t\t\t\t<deviceType>urn:schemas-upnp-org:device:WANConnectionDevice:1</deviceType>\r\n"
	"\t\t\t\t\t\t<friendlyName>WAN Connection Device</friendlyName>\r\n"
	"\t\t\t\t\t\t<manufacturer>Broadcom</manufacturer>\r\n"
	"\t\t\t\t\t\t<manufacturerURL>http://www.broadcom.com</manufacturerURL>\r\n"
	"\t\t\t\t\t\t<modelDescription>Gateway</modelDescription>\r\n"
	"\t\t\t\t\t\t<modelName>router</modelName>\r\n"
	"\t\t\t\t\t\t<modelURL>http://www.broadcom.com</modelURL>\r\n"
	"\t\t\t\t\t\t<UDN>uuid:1995cf2d-d4b1-4fdb-bf84-8e59d2066198</UDN>\r\n"
	"\t\t\t\t\t\t<serviceList>\r\n"
	"\t\t\t\t\t\t\t<service>\r\n"
	"\t\t\t\t\t\t\t\t<serviceType>urn:schemas-upnp-org:service:WANIPConnection:1</serviceType>\r\n"
	"\t\t\t\t\t\t\t\t<serviceId>urn:upnp-org:serviceId:WANIPConn1</serviceId>\r\n"
	"\t\t\t\t\t\t\t\t<SCPDURL>x_wanipconnection.xml</SCPDURL>\r\n"
	"\t\t\t\t\t\t\t\t<controlURL>control?WANIPConnection</controlURL>\r\n"
	"\t\t\t\t\t\t\t\t<eventSubURL>event?WANIPConnection</eventSubURL>\r\n"
	"\t\t\t\t\t\t\t</service>\r\n"
	"\t\t\t\t\t\t</serviceList>\r\n"
	"\t\t\t\t\t</device>\r\n"
	"\t\t\t\t</deviceList>\r\n"
	"\t\t\t</device>\r\n"
	"\t\t</deviceList>\r\n"
	"\t\t<presentationURL>http://255.255.255.255</presentationURL>\r\n"
	"\t</device>\r\n"
	"</root>\r\n"
	"\r\n";
