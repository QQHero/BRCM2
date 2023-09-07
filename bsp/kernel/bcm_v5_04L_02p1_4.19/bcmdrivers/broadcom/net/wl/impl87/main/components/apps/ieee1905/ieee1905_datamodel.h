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

#ifndef _IEEE1905_DATAMODEL_H_
#define _IEEE1905_DATAMODEL_H_

/*
 * IEEE1905 Data-Model
 */

#include "ieee1905_defines.h"
#include "ieee1905_linkedlist.h"
#include <bcmwifi_channels.h>
#include <bcmutils.h>

#define I5_DM_VERSION 0

typedef int (*i5MacAddressDeliveryFunc)(unsigned char const * macAddressList, unsigned char numMacs);

/* Define a Generic List */
typedef struct ieee1905_glist {
	uint count;			/* Count of list of objects */
	dll_t head;			/* Head Node of list of objects */
} ieee1905_glist_t;

/* Traverse each item of a Generic List */
#define foreach_iglist_item(item, type, list) \
		for ((item) = ((type*)dll_head_p(&((list).head))); \
			! dll_end(&((list).head), (dll_t*)(item)); \
			(item) = ((type*)dll_next_p((dll_t*)(item))))

/* Traverse each item of a Generic List, Check for additional condition */
#define foreach_iglist_item_ex(item, type, list, condition) \
		for ((item) = ((type*)dll_head_p(&((list).head))); \
			((!dll_end(&((list).head), (dll_t*)item))&& ((condition))); \
			(item) = ((type*)dll_next_p((dll_t*)(item))))

/* Traverse each item of a Generic List, with keep track of next node */
#define foreach_safe_iglist_item(item, type, list, next) \
		for ((item) = ((type*)dll_head_p(&((list).head))); \
			!dll_end(&((list).head), (dll_t*)(item)); \
			(item) = (next))

/* Initialize generic list */
static inline void ieee1905_glist_init(ieee1905_glist_t *list)
{
	list->count = 0;
	dll_init(&(list->head));
}

/* Append a node to generic list */
static inline void ieee1905_glist_append(ieee1905_glist_t *list, dll_t *new_obj)
{
	dll_append((dll_t *)&(list->head), new_obj);
	++(list->count);
}

/* Delete a node from generic list */
static inline void ieee1905_glist_delete(ieee1905_glist_t *list, dll_t *obj)
{
	dll_delete(obj);
	--(list->count);
}

/* Delete all the node from generic list */
void i5DmGlistCleanup(ieee1905_glist_t *list);

#ifdef MULTIAP

/* Bit flags used in i5_dm_clients_type structure */
#define I5_CLIENT_FLAG_QUERY_STATE    0x01  /* 0 means pending, 1 means done */
#define I5_CLIENT_FLAG_XXX            0x02  /* This is not used now. we can use it later */
#define I5_CLIENT_FLAG_BSTA           0x04  /* This is same as IEEE1905_MAP_FLAG_STA */

#define I5_CLIENT_IS_BSTA(client)    ((client)->flags & I5_CLIENT_FLAG_BSTA)

/* Bit flags used in i5_dm_bss_type structure */
#define I5_BSS_FLAG_QUERY_STATE     0x01  /* 0 means pending, 1 means done */
#define I5_BSS_FLAG_EBTABLE_RULE    0x02  /* EBTABLE rule created for VLANs */

/* Default TX_RATE for ethernet agents, is currently 1 Gbps(1000 Mbps) */
#define I5_DM_DEFAULT_ETH_TX_RATE	1000

/* Type of Channel Scan */
typedef enum t_i5_chascan_type
{
  I5_CHSCAN_ONBOOT	= 0,
  I5_CHSCAN_REQ_STORED,
  I5_CHSCAN_REQ_FRESH,
} t_I5_CHSCAN_TYPE;

/* Print Channel Scan Type String */
#define I5_PRINT_CHSCAN_TYPE(scan_type)	(((scan_type) == I5_CHSCAN_ONBOOT) ? "ONBOOT" : \
  (((scan_type) == I5_CHSCAN_REQ_STORED) ? "STORED" : "FRESH"))

/* Status of Channel Scan */
typedef enum t_i5_chascan_status
{
  I5_CHSCAN_STOP	= 0,
  I5_CHSCAN_RUNNING,
} t_I5_CHSCAN_STATUS;

/* Bit flags for status book-keeping used in i5_dm_interface_type structure */
#define I5_FLAG_IFR_M1_SENT			0x1
#define I5_FLAG_IFR_M2_RECEIVED			0x2
#define I5_FLAG_IFR_CHSCAN_RUNNING		0x4 /* Already Running a Requested
                                                   * Channel Scan on this radio
                                                   */
#define I5_FLAG_IFR_TS_MIX_FH_P1BH_SUPPORTED	0x8 /* Traffic Separation on combined fronthaul
                                                   * and Profile-1 backhaul support
                                                   */
#define I5_FLAG_IFR_TS_MIX_P1BH_P2BH_SUPPORTED  0x10 /* Traffic Separation on combined
                                                     * Profile-1 backhaul and Profile-2
                                                     * backhaul support
                                                     */
#define I5_FLAG_IFR_CHSCAN_ONBOOT		0x20 /* Already Running an  Onboot
                                                   * Channel Scan on this radio
                                                   */
#define I5_FLAG_IFR_INIT_STORED_SCAN_RES  0x40  /* Already initialized store scan result */
#define I5_FLAG_IFR_CAC_PENDING           0x80  /* CAC is not completed */

#define I5_IS_M1_SENT(flags)			((flags) & I5_FLAG_IFR_M1_SENT)
#define I5_IS_M2_RECEIVED(flags)		((flags) & I5_FLAG_IFR_M2_RECEIVED)
#define I5_IS_CHSCAN_RUNNING(flags)		((flags) & I5_FLAG_IFR_CHSCAN_RUNNING)
#define I5_IS_TS_MIX_FH_P1BH_SUPPORTED(flags)	((flags) & I5_FLAG_IFR_TS_MIX_FH_P1BH_SUPPORTED)
#define I5_IS_TS_MIX_P1BH_P2BH_SUPPORTED(flags)	((flags) & I5_FLAG_IFR_TS_MIX_P1BH_P2BH_SUPPORTED)
#define I5_IS_CHSCAN_ONBOOT(flags)		((flags) & I5_FLAG_IFR_CHSCAN_ONBOOT)
#define I5_IS_INIT_STORED_SCAN_RES(flags)       ((flags) & I5_FLAG_IFR_INIT_STORED_SCAN_RES)
#define I5_IS_IFR_CAC_PENDING(flags)            ((flags) & I5_FLAG_IFR_CAC_PENDING)

/* SSID Type */
typedef struct {
  unsigned char	SSID_len;
  unsigned char	SSID[IEEE1905_MAX_SSID_LEN + 1];
} ieee1905_ssid_type;

#define	I5_FLAG_BSS_AP_METRICS_RCVD	0x1	/* Flag to track ap metrics received for bss */
#define I5_IS_BSS_AP_METRICS_RCVD(f)	((f) & I5_FLAG_BSS_AP_METRICS_RCVD)
typedef struct {
  unsigned char include_bit_esp; /* Include bit for the Estimated Service Parameters Information.
                                 * Its of type IEEE1905_INCL_BIT_ESP_XX
                                 */
  unsigned char esp_ac_be[IEEE1905_ESP_LEN]; /* ESP Information field for AC=BE */
  unsigned char esp_ac_bk[IEEE1905_ESP_LEN]; /* ESP Information field for AC=BK */
  unsigned char esp_ac_vo[IEEE1905_ESP_LEN]; /* ESP Information field for AC=VO */
  unsigned char esp_ac_vi[IEEE1905_ESP_LEN]; /* ESP Information field for AC=VI */
  unsigned char flags; /* flags of type I5_FLAG_BSS_AP_METRICS_XX */
#if defined(MULTIAPR2)
  /* AP Extended Metrics */
  uint64 unicastBytesSent;        /* Total unicast bytes transmitted */
  uint64 unicastBytesReceived;    /* Total unicast bytes received */
  uint64 multicastBytesSent;      /* Total multicast bytes transmitted */
  uint64 multicastBytesReceived;  /* Total multicast bytes received */
  uint64 broadcastBytesSent;      /* Total broadcast bytes transmitted */
  uint64 broadcastBytesReceived;  /* Total broadcast bytes received */
#endif /* MULTIAPR2 */
  time_t ts_recvd; /* AP metrics Resp Received timestamp, used by Controller */
} ieee1905_ap_metric;

/* VHT Capabilities */
typedef struct {
  unsigned short  TxMCSMap;	/* VHT Tx MCS map */
  unsigned short  RxMCSMap;	/* VHT Rx MCS map */
  unsigned char  CapsEx;	/* VHT extended capabilities */
  unsigned char  Caps;		/* VHT capabilities */
  unsigned char  Valid;		/* Wheter VHT caps are valid or not. */
} ieee1905_vht_caps_type;

/* Radio Capabilities */
typedef struct {
  unsigned char maxBSSSupported;  /* Maximum number of BSSs supported by this radio */
  unsigned char Len;	/* length of List */
  unsigned char Valid;	/* Whether radio caps are valid or not */
  unsigned char *List;	/* List of opclass */
  unsigned short ListSize;  /* Memory allocated to the List */
} ieee1905_radio_caps_type;

/* HE Capabilities */
typedef struct {
  unsigned short TxBW80MCSMap;		/* HE 80Mhz Tx MCS map */
  unsigned short RxBW80MCSMap;		/* HE 80Mhz Tx MCS map */
  unsigned short TxBW160MCSMap;		/* HE 160Mhz Tx MCS map */
  unsigned short RxBW160MCSMap;		/* HE 160Mhz Tx MCS map */
  unsigned short TxBW80p80MCSMap;	/* HE 80p80Mhz Tx MCS map */
  unsigned short RxBW80p80MCSMap;	/* HE 80p80Mhz Tx MCS map */
  unsigned char  CapsEx;		/* HE extended capabilities */
  unsigned char  Caps;			/* HE capabilities */
  unsigned char Valid;			/* Whether HE caps are valid or not */
} ieee1905_he_caps_type;

/* Channel Scan Capabilities Flags */
#define MAP_CHSCAN_CAP_ONBOOT_ONLY	0x80  /* "On boot only" flag, bit 7 Indicates whether
	* the specified radio is capable only of "On boot" scans, or can perform scans upon request
	* 0 : Agent can perform Requested scans
	* 1 : Agent can only perform scan on boot
	*/
/* Bit Mask to get bits 5 to 6 for Scan Impact from Channel Scan Capabilities Flags */
#define MAP_CHSCAN_CAP_SCAN_IMPACT_MASK	0x9F  /* Mask bits other than 5 & 6 to get Scan Impact */
/* MACRO to get Scan Impact from Channel Scan Capabilities Flags */
#define I5_CHSCAN_CAP_GET_SCAN_IMPACT_TYPE(x)   ((x) & MAP_CHSCAN_CAP_SCAN_IMPACT_MASK)

/* Shift Left to set Scan Impact in Channel Scan Capabilities Flags */
#define MAP_CHSCAN_CAP_SCAN_IMPACT_SHIFT	5

/* Scan Impact types defined by specification :
 * Guidance information on the expected impact on any Fronthaul or Backhaul operations
 * on this radio of using this radio to perform a channel scan */
#define MAP_CHSCAN_CAP_SCAN_IMPACT_NONE		0x00 /* 0x00 No impact
	* (independent radio not used for Fronthaul or backhaul is available for scanning) */
#define MAP_CHSCAN_CAP_SCAN_IMPACT_REDUCED	0x01 /* 0x01 Reduced number of spatial streams */
#define MAP_CHSCAN_CAP_SCAN_IMPACT_TMSLICE	0x02 /* 0x02 Time slicing impairment */
#define MAP_CHSCAN_CAP_SCAN_IMPACT_UNAVAIL	0x03 /* 0x03 Radio unavailable for >= 2 seconds */

/* Channel Scan Capabilities */
typedef struct {
  unsigned char chscan_cap_flag;  /* Channel Scan Capabilities Flags
	* of type MAP_CHSCAN_CAP_XXX_XXX */
  unsigned int	min_scan_interval; /* Minimum Scan Interval :
	* The minimum interval in seconds between the start of
	* two consecutive channel scans on this radio
	*/
  unsigned char Len;	/* length of List */
  unsigned char Valid;  /* Whether Channel Scan caps are valid or not */
  unsigned char *List;	/* List of opclass */
  unsigned short ListSize;  /* Memory allocated to the List */
} ieee1905_channel_scan_caps_type;

#if defined(MULTIAPR2)
/* Operating channel and its list channels for a given CAC type */
typedef struct {
  uint8 opclass;			/* operating class */
  uint8 n_chan;				/* Number of channels */
  uint8 chan[16];			/* List of channel for the opclass */
} i5_dm_op_class_list_t;

/* Details of a CAC type */
typedef struct {
  uint8 method;				/* CAC method */
  uint32 time;				/* CAC duration in seconds */
  uint8 n_op_class;			/* Number of opclasses for the method and duration */
  i5_dm_op_class_list_t opclasses[8];	/* Structure array for opclass list */
} i5_dm_cac_types_t;

/* Per interface CAC capability strucutre (Controller only) */
typedef struct {
  uint8 method_flag;		/* Flags of type WBD_CAC_XXX */
  uint8 n_types;		/* Number of CACs supported. */
  i5_dm_cac_types_t types[8];	/* Structure Array for cac types */
} i5_dm_cac_capabilities_t;
#endif /* MULTIAPR2 */

/* AP capabilities */
typedef struct {
  unsigned char HTCaps;		/* HT caps. */
  ieee1905_vht_caps_type VHTCaps;	/* VHT caps */
  ieee1905_radio_caps_type RadioCaps;	/* Radio caps */
#if defined(MULTIAPR2)
  ieee1905_channel_scan_caps_type ChScanCaps;	/* Channel Scan caps */
  i5_dm_cac_capabilities_t *cac_cap; /* CAC capability strucutre (Controller only) */
#endif /* MULTIAPR2 */
  ieee1905_he_caps_type HECaps;		/* HE caps */
} ieee1905_ap_caps_type;

typedef struct {
  struct timespec queried;  /* Time at which the link metric calculated */
  unsigned int delta; /* The time delta in ms between queried and report was sent */
  unsigned int downlink_rate; /* Estimated MAC Data Rate in downlink (in Mb/s) */
  unsigned int uplink_rate; /* Estimated MAC Data Rate in upnlink (in Mb/s) */
#if defined(MULTIAPR2)
  unsigned int last_data_downlink_rate; /* STA last Data Rate in downlink (in Mb/s) */
  unsigned int last_data_uplink_rate; /* STA last Data Rate in upnlink (in Mb/s) */
  unsigned int utilization_recv; /* STA Utilization received */
  unsigned int utilization_tx; /* STA Utilization transmit */
#endif /* MULTIAPR2 */
  unsigned char rcpi; /* Measured uplink RSSI for STA in RCPI */
} ieee1905_sta_link_metric;

typedef struct {
  uint64 bytes_sent;  /* number of bytes sent to the associated STA */
  uint64 bytes_recv;  /* number of bytes received from the associated STA */
  unsigned int packets_sent;  /* number of packets successfully sent to the associated STA */
  unsigned int packets_recv;  /* number of packets received from the associated STA */
  unsigned int tx_packet_err;  /* number of packets which could not be transmitted */
  unsigned int rx_packet_err;  /* number of packets which were received in error form */
  unsigned int retransmission_count;  /* number of packets sent with the retry flag set */
} ieee1905_sta_traffic_stats;

typedef struct {
  unsigned char regclass;
  unsigned char count;
  unsigned char channel[IEEE1905_MAX_RCCHANNELS];
  unsigned char pref;
  unsigned char reason;
} ieee1905_chan_pref_rc_map;

typedef struct {
  uint8 rc_count;
  ieee1905_chan_pref_rc_map *rc_map;
} ieee1905_chan_pref_rc_map_array;

/* Interfcae Metric */
typedef struct {
  unsigned char chan_util;  /* Channel Utilization */
#if defined(MULTIAPR2)
  unsigned char noise;  /* An indicator of the average radio noise plus interference power
                         * measured for the primary operating channel
                         */
  unsigned char transmit; /* The percentage of time (linearly scaled with 255 representing 100%)
                           * the radio has spent on individually or group addressed transmissions
                           * by the AP
                           */
  unsigned char receive_self; /* The percentage of time (linearly scaled with 255 representing 100%)
                               * the radio has spent on receiving individually or group addressed
                               * transmissions from any STA associated with any BSS operating on
                               * this radio
                               */
  unsigned char receive_other;  /* The percentage of time (linearly scaled with 255 representing
                                 * 100%) the radio has spent on receiving valid IEEE 802.11 PPDUs
                                 * that are not associated with any BSS operating on this radio
                                 */
#endif /* MULTIAPR2 */
} ieee1905_interface_metric;

/* Transmitter and reciever link metric */
typedef struct {
  struct timespec queried; /* Last queried time */
  unsigned int txPacketErrors; /* Estimated number of lost packets during the measurement period */
  unsigned int transmittedPackets; /* Estimated number of packets transmitted */
  unsigned short macThroughPutCapacity; /* The maximum MAC throughput in Mbps */
  unsigned short linkAvailability; /* average percentage of time that the link is available for
                                    * data transmission
                                    */
  unsigned short phyRate; /* PHY rate in Mbps */
  unsigned int receivedPackets; /* Number of packets received at the interface */
  unsigned int rxPacketErrors; /* Estimated number of lost packets during the measurement period */
  signed char rssi; /* estimated RSSI in dB */
  unsigned int prevRxBytes;
  unsigned int latestRxBytes;
} ieee1905_backhaul_link_metric;

typedef struct {
  unsigned int txPacketErrors; /* Estimated number of lost packets during the measurement period */
  unsigned int transmittedPackets; /* Estimated number of packets transmitted */
  unsigned int receivedPackets; /* Number of packets received at the interface */
  unsigned int rxPacketErrors; /* Estimated number of lost packets during the measurement period */
} ieee1905_old_backhaul_link_counter;

/* Structure to represent Client object in a BSS Object */
typedef struct {
  i5_ll_listitem  ll;
  unsigned char   mac[MAC_ADDR_LEN]; /* Unique STA MAC address of Client */
  struct timeval  assoc_tm; /* timestamp when Client got associated to BSS,
                                   * to find time elapsed since the Client got associated
                                   */
  unsigned char *assoc_frame;
  unsigned short assoc_frame_len;
  ieee1905_sta_traffic_stats traffic_stats; /* Associated STA Traffic Stats */
  ieee1905_sta_traffic_stats old_traffic_stats; /* Old Associated STA Traffic Stats */
  ieee1905_sta_link_metric link_metric; /* Associated STA Link Metrics */

  void *vndr_data;    /* vendor specific data pointer which can be filled by the user */
  unsigned char flags;  /* Flags of type I5_CLIENT_FLAG_XXX :
                                    * Flags of Client object which saves its type (bSTA) if it is Backhaul STA
                                    */
} i5_dm_clients_type;

typedef struct {
  uint8 regclass;
  uint8 count;
  uint8 channel[IEEE1905_80211_CHAN_PER_REGCLASS];
} i5_dm_rc_chan_map_type;

#define IEEE1905_MAX_CH_IN_OPCLASS	60 /* Max Possible Chspec in an Operating Class */
#define MAP_MAX_NONOP_CHSPEC		256 /* Max Possible Non-Operable Chanspecs in a Radio */

/* Information of each Chanspec in Chanspec List */
typedef struct i5_chspec_item {
	dll_t node; /* self referencial (next,prev) pointers of type dll_t */
	chanspec_t chspec; /* Chanspec */
} i5_chspec_item_t;

typedef struct {
  dll_t node;			/* self referencial (next,prev) pointers of type dll_t */

  unsigned char supported;  /* Is the Operating Class Supported for Scan */

  unsigned char opclass_val; /* Operating Class :
	* contains an enumerated value from Table E-4 in Annex E of [1], specifying the
	* global operating class in which the subsequent Channel List is valid. */

  unsigned char num_of_channels;  /* Number of Operating Classes :
	* for which channel scans are being requested on this radio.
	* If the Perform Fresh Scan bit is set to 0, this field shall be set to zero
	* and the following fields shall be omitted
	*/

  unsigned char supported_chan_list[(IEEE1905_MAX_CH_IN_OPCLASS + 7)/8]; /* Each Channel in Channel List Supported or not */

  unsigned char scan_required[(IEEE1905_MAX_CH_IN_OPCLASS + 7)/8]; /* Each Channel in Channel List Scan Req or not */

  unsigned char chan_list[IEEE1905_MAX_CH_IN_OPCLASS]; /* Channel List : Contains a variable number of octets.
	* Each octet describes a single channel number in the Operating Class on which the Agent is requested to perform a scan.
	* An empty Channel List field (k=0) indicates that the Agent is requested to scan on all channels in the Operating Class.
	*/
} ieee1905_per_opclass_chan_list;

typedef struct {
  dll_t node;			/* self referencial (next,prev) pointers of type dll_t */

  unsigned char supported;  /* Is the Radio Supported for Scan */

  unsigned char radio_mac[MAC_ADDR_LEN]; /* Radio mac address */

  unsigned char num_of_opclass;  /* Number of Operating Classes :
	* for which channel scans are being requested on this radio.
	* If the Perform Fresh Scan bit is set to 0, this field shall be set to zero
	* and the following fields shall be omitted
	*/

  ieee1905_glist_t opclass_list;  /* List of ieee1905_per_opclass_chan_list type objects */

  time_t ts_scan_recvd; /* Timestamp when Channel Scan Request Received */

  t_I5_CHSCAN_TYPE scan_type; /* Type of Scan */

} ieee1905_per_radio_opclass_list;

/* Channel Scan Request TLV Flags */
#define MAP_CHSCAN_REQ_FRESH_SCAN 0x80  /* Perform Fresh Scan :
	* Indicator to identify whether a fresh scan is being requested, or whether
	* stored results from previous (including on-boot) scan are requested
	* 0 : Return stored results of last successful scan
	* 1 : Perform a fresh scan and return results
	*/
/* Channel Scan Request TLV Configuration */
typedef struct {
  unsigned char chscan_req_msg_flag;  /* Channel Scan Request TLV Flags
	* of type MAP_CHSCAN_REQ_XXX_XXX */

  unsigned char num_of_radios;  /* Number of radios :
	* upon which channel scans are requested */

  ieee1905_glist_t radio_list;  /* List of ieee1905_per_radio_opclass_list type objects */

} ieee1905_chscan_req_msg;

/* Channel Scan Result TLV Neighbor Flags */
#define MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT 0x80 /* BSS Load Element Present
	* Set to one if the neighboring BSS's Beacons/Probe Responses include a BSS Load Element
	* as defined in section 9.4.2.28 of [1]. Set to 0 otherwise.
	* 0 : field not present
	* 1 : field present
	*/
/* String constants indicating the channel bandwidth field */
#define MAP_CH_BW_10		"10"
#define MAP_CH_BW_20		"20"
#define MAP_CH_BW_40		"40"
#define MAP_CH_BW_80		"80"
#define MAP_CH_BW_160		"160"
#define MAP_CH_BW_8080		"80+80"

/* Maximum Length of String constants indicating the channel bandwidth field */
#define IEEE1905_CH_BW_MAX_LEN	6

/* Channel Scan Result TLV Neighbor Configuration */
typedef struct {
  dll_t node;		/* Self refrential pointer of type dll_t*/

  unsigned char nbr_bssid[MAC_ADDR_LEN]; /* The BSSID indicated by the neighboring BSS */
  ieee1905_ssid_type nbr_ssid; /* SSID of the neighboring BSS */
  unsigned char nbr_rcpi; /* SignalStrength : An indicator of radio signal strength (RSSI) of
	* the Beacon or Probe Response frames of the neighboring BSS as received by the radio
	* measured in dBm. (RSSI is encoded per Table 9-154 of [[1]). Reserved: 221 - 255
	*/
  unsigned char ch_bw_length; /* Length of Channel Bandwidth field */
  unsigned char ch_bw[IEEE1905_CH_BW_MAX_LEN]; /* ChannelBandwidth : String indicating the maximum bandwidth
	* at which the neighbor BSS is operating, e.g., 20 or 40 or 80 or 80+80 or 160 MHz. */
  unsigned char chscan_result_nbr_flag;  /* Channel Scan  Result TLV Neighbor Flags
	* of type MAP_CHSCAN_RES_NBR_XXX_XXX */
  unsigned char channel_utilization; /* ChannelUtilization : If MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT
    * bit is set to 1, this field is present. Otherwise it is omitted. */
  unsigned short station_count; /* StationCount : If MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT
    * bit is set to 1, this field is present. Otherwise it is omitted.
    * The number of associated stations reported by the neighboring BSS per the BSS Load
    * element if present n Beacon or Probe Response frames as defined in section 9.4.2.28 of [1]. */
} ieee1905_chscan_result_nbr_item;

#define MAX_CHSCAN_RESULT_NBR_SIZE(emt_nbr) (sizeof((emt_nbr).nbr_bssid) + \
  sizeof((emt_nbr).nbr_ssid.SSID_len) + sizeof((emt_nbr).nbr_ssid.SSID) + \
  sizeof((emt_nbr).nbr_rcpi) + sizeof((emt_nbr).ch_bw_length) + \
  sizeof((emt_nbr).ch_bw) + sizeof((emt_nbr).chscan_result_nbr_flag) + \
  sizeof((emt_nbr).channel_utilization) + sizeof((emt_nbr).station_count))

/* Channel Scan Result TLV Flags */
#define MAP_CHSCAN_RES_SCANTYPE 0x80 /* Scan Type :
	* Indicates whether the scan was performed passively or with Active probing
	* 0 : Scan was Passive scan
	* 1 : Scan was an Active scan
	*/

/* Maximum Length of String indicating Timestamp field */
#define IEEE1905_TS_MAX_LEN	30

/* Scan Status Code Values */
#define MAP_CHSCAN_STATUS_SUCCESS	0x00 /* Scan Successful */
#define MAP_CHSCAN_STATUS_UNSUPPORTED	0x01 /* Scan not supported on this operating class/channel on this radio */
#define MAP_CHSCAN_STATUS_REQ_SOON	0x02 /* Request too soon after last scan */
#define MAP_CHSCAN_STATUS_RADIO_BUSY	0x03 /* Radio too busy to perform scan */
#define MAP_CHSCAN_STATUS_UNCOMPLETE	0x04 /* Scan not completed */
#define MAP_CHSCAN_STATUS_ABORTED	0x05 /* Scan aborted */
#define MAP_CHSCAN_STATUS_ONLY_ONBOOT	0x06 /* Fresh scan not supported. Radio only supports on boot scans */
#define MAP_CHSCAN_STATUS_RESERVED	0x07 /* Reserved : 0x07 - 0xFF */

/* Print Channel Scan Status Codes */
#define MAP_PRINT_CHSCAN_STATUS(scan_status) \
  (((scan_status) == MAP_CHSCAN_STATUS_SUCCESS) ? "SUCCESS" : \
  (((scan_status) == MAP_CHSCAN_STATUS_UNSUPPORTED) ? "UNSUPPORTED" : \
  (((scan_status) == MAP_CHSCAN_STATUS_REQ_SOON) ? "TOO_SOON" : \
  (((scan_status) == MAP_CHSCAN_STATUS_RADIO_BUSY) ? "RADIO_BUSY" : \
  (((scan_status) == MAP_CHSCAN_STATUS_UNCOMPLETE) ? "UNCOMPLETE" : \
  (((scan_status) == MAP_CHSCAN_STATUS_ABORTED) ? "ABORTED" : \
  (((scan_status) == MAP_CHSCAN_STATUS_ONLY_ONBOOT) ? "ONLY_ONBOOT" : "RESERVED")))))))

/* Iterference of other BSS in this 20MHz Channel, Derived from all Channel Scan Results */
typedef struct ieee1905_chan_pry_info {
	uint8 channel; /* 20 MHz channel in Channel Scan Result */
	uint8 n_ctrl; /* # of BSS' using this as their ctl channel */
	uint8 n_ext20; /* # of 40/80/160 MHZBSS' using this as their ext20 channel */
	uint8 n_ext40; /* # of 80/160 MHZ BSS' using this as one of their ext40 channels */
	uint8 n_ext80; /* # of 160MHZ BSS' using this as one of their ext80 channels */
} ieee1905_chan_pry_info_t;

typedef struct ieee1905_chan_pry {
	uint8 control;
	uint8 ext20;
	uint8 ext40[2];
	uint8 ext80[4];
} ieee1905_chan_pry_t;

/* Channel Scan Result TLV Configuration */
typedef struct {
  dll_t node;		/* Self refrential pointer of type dll_t*/

  /* TLV Related Fields */
  unsigned char radio_mac[MAC_ADDR_LEN]; /* Radio mac address */
  unsigned char opclass; /* Operating Class */
  unsigned char channel; /* Channel : The channel number
	* of the channel scanned by the radio given the operating class */
  unsigned char scan_status_code; /* Scan Status : A status code to indicate
	* whether a scan has been performed successfully and if not, the reason for failure
	* values of type MAP_CHSCAN_STATUS_XXX */
  unsigned char timestamp_length; /* Length of Timestamp Octets */
  unsigned char timestamp[IEEE1905_TS_MAX_LEN]; /* Timestamp Octets */
  unsigned char utilization; /* Utilization : The current channel utilization measured
	* by the radio on the scanned 20 MHz channel - as defined in section 9.4.2.28 of [1] */
  unsigned char noise; /* Noise : An indicator of the average radio noise plus interference power
	* measured on the 20MHz channel during a channel scan.
	* Encoding as defined as for ANPI in section 11.11.9.4 of [1] */
  unsigned short num_of_neighbors; /* NumberOfNeighbors :
	* The number of neighbor BSS discovered on this channel */

  ieee1905_glist_t neighbor_list; /* List of ieee1905_chscan_result_nbr_item type objects */

  unsigned int aggregate_scan_duration; /* AggregateScanDuration : Total time spent
	* performing the scan of this channel in milliseconds. */
  unsigned char chscan_result_flag;  /* Channel Scan Request TLV Flags
	* of type MAP_CHSCAN_RES_XXX */

  time_t ts_scan_start; /* Timestamp when {FRESH} Channel Scan started */
  time_t ts_scan_end; /* Timestamp when {FRESH} Channel Scan ended */

  /* Result Interpretation Fields used by Controller */
  char ifname[I5_MAX_IFNAME]; /* Interface name wlX.Y of this Radio */
  chanspec_t chanspec_20; /* 20 MHz Chaspec of Scan Channel */
  ieee1905_chan_pry_info_t pry_info; /* Info of Iterference of other BSS in this 20MHz Channel */

} ieee1905_chscan_result_item;

/* Channel Scan Report Message Configuration */
typedef struct {

  unsigned char num_of_results; /* Number of Channel Scan Result items */
  ieee1905_glist_t chscan_result_list; /* List of ieee1905_chscan_result_item type objects */

} ieee1905_chscan_report_msg;

/* Structure to represent BSS object in an Interface Object */
typedef struct {
  i5_ll_listitem      ll;
  char ifname[I5_MAX_IFNAME]; /* OS specific BSS name */
  unsigned char mapFlags; /* Of Type IEEE1905_MAP_FLAG_XXX */
  unsigned char flags;    /* Flags of type I5_BSS_FLAG_XXX */
  unsigned char       BSSID[MAC_ADDR_LEN]; /* Unique BSSID of BSS object */
  ieee1905_ssid_type  ssid; /* SSID of BSS object */
  unsigned short      ClientsNumberOfEntries; /* Number of Client objects associated with this BSS */
  i5_dm_clients_type  client_list; /* List of Client objects associated to this BSS */
  ieee1905_ap_metric  APMetric; /* Ap Metric */

  uint8	assoc_allowance_status;	/* whether BSS is capable of accepting assoc req or not */
  uint32 avg_tx_rate;		/* Avg tx rate based on the mcs map, nss and bw */
  void *vndr_data;    /* vendor specific data pointer which can be filled by the user */
} i5_dm_bss_type;

/* Profile-2(R2) AP Capability */
typedef struct {
  unsigned short max_sp_rules;            /* Max no. of service prioritization Rules */
  unsigned char byte_cntr_unit; /* Byte Counter Units. The units used for byte counters when the
                                 * Agent reports traffic statistics
                                 */
  unsigned char max_vids;       /* Max Total Number of unique VIDs the Multi-AP Agent supports */
} i5_dm_p2_ap_capability_type;

typedef struct ieee1905_opclass_chan_info {
  uint8 opclass;
  uint8 chan;
  uint16 duration; /* duration after CAC(in minutes) or after detecting radar(in seconds) */
} ieee1905_opclass_chan_info_t;

typedef struct ieee1905_ongoing_cac_opclass_chan_info {
  uint8 opclass;
  uint8 chan;
  uint8 duration[3]; /* duration in seconds remaining to complete CAC */
} ieee1905_ongoing_cac_opclass_chan_info_t;

typedef struct ieee1905_opclass_chan_info_list {
  uint8 n_count;	/* number of chan, agent indicates as available channels */
  ieee1905_opclass_chan_info_t info[0];
} ieee1905_opclass_chan_info_list_t;

typedef struct ieee1905_ongoing_cac_info {
  uint8 n_count; /* number of class/chan pairs that have an active CAC omgoing */
  ieee1905_ongoing_cac_opclass_chan_info_t info[0];
} ieee1905_ongoing_cac_list_t;

typedef struct ieee1905_cac_status {
  ieee1905_opclass_chan_info_list_t active_chan_info[1]; /* list of available chan in agent */
  ieee1905_opclass_chan_info_list_t radar_info[1]; /* list of radar detected chan in agent */
  ieee1905_ongoing_cac_list_t ongoing_cac_info[1]; /* list of ongoing cac chan in agent */
} ieee1905_cac_status_t;
#endif /* MULTIAP */

/* Channel Selection Response Code Constants */
typedef enum t_i5_chan_sel_resp_code
{
  I5_CHAN_SEL_RESP_CODE_NONE = -1,/* NONE */
  I5_CHAN_SEL_RESP_CODE_ACCEPT = 0, /* 0x00: Accept */
  I5_CHAN_SEL_RESP_CODE_DECLINE_1,  /* 0x01: Decline because request
			* violates current preferences which have changed since last reported
			*/
  I5_CHAN_SEL_RESP_CODE_DECLINE_2,  /* 0x02: Decline because request
			* violates most recently reported preferences
			*/
  I5_CHAN_SEL_RESP_CODE_DECLINE_3,  /* 0x03: Decline because request
			* would prevent operation of a currently operating backhaul link
			* (where backhaul STA and BSS share a radio)
			*/
  I5_CHAN_SEL_RESP_CODE_RESERVED    /* 0x04 - 0xFF: Reserved */
} t_I5_CHAN_SEL_RESP_CODE;

/* Structure to represent Interface object in a Device Object */
typedef struct {
  i5_ll_listitem ll;
  unsigned char  state;
  unsigned char  InterfaceId[MAC_ADDR_LEN]; /* Unique MAC address of Interface object */
  char  ifname[I5_MAX_IFNAME]; /* OS specific interface name */
  char  prefix[I5_MAX_IFNAME];  /* wlX_ or wlX.y_ Prefix of OS specific interface name */
  unsigned int   Status; /* Current state of interface (UP/DWON) */
  unsigned int   SecurityStatus;
  unsigned short MediaType; /* Interface type (Eth/ Wi-Fi 11ac/ Wi-Fi 11n24) :
                                 * 0xffff indicates a Generic Phy Device
                                 */
  unsigned char  netTechOui[I5_PHY_INTERFACE_NETTECHOUI_SIZE];
  unsigned char  netTechVariant;
  unsigned char  netTechName[I5_PHY_INTERFACE_NETTECHNAME_SIZE];
  unsigned char  url[I5_PHY_INTERFACE_URL_MAX_SIZE];
  unsigned char  MediaSpecificInfo[I5_MEDIA_SPECIFIC_INFO_MAX_SIZE];
  unsigned int   MediaSpecificInfoSize;
  i5MacAddressDeliveryFunc i5MacAddrDeliver;
#ifdef MULTIAP
  unsigned int   BSSNumberOfEntries; /* Number of BSS objects in this Interface */
  chanspec_t chanspec;
  unsigned char opClass;
  unsigned char band; /* Of type BAND_XXX. For dual band it will have both 5GL and 5GH */
#endif /* MULTIAP */
  union {
    struct {
      char          wlParentName[I5_MAX_IFNAME];
      unsigned char isRenewPending;
      unsigned char confStatus;
      unsigned char credChanged;
      unsigned char isConfigured; /* enrollee only */
    };
  };
#ifdef MULTIAP
  i5_dm_bss_type           bss_list; /* List of BSS's operating in this interface */
  ieee1905_chan_pref_rc_map_array ChanPrefs;
  unsigned char	      TxPowerLimit;
  t_I5_CHAN_SEL_RESP_CODE chan_sel_resp_code; /* Channel Selection Response Code */
  ieee1905_ap_caps_type     ApCaps;   /* AP capabilities. */
  ieee1905_interface_metric ifrMetric;  /* Interface Metric */
  unsigned char bssid[MAC_ADDR_LEN];  /* BSSID in case of IEEE1905_MAP_FLAG_STA */
  unsigned char mapFlags; /* Of Type IEEE1905_MAP_FLAG_XXX */
  unsigned char flags;	/* Of Type I5_FLAG_IFR_XXX */
  void *ptmrGetBSSID;  /* Timer for getting the bSTA's BSSID */
  unsigned char roam_enabled;	/* Set this to indicate roam is enabled for the interface(STA) */
#endif /* MULTIAP */
  void *vndr_data;    /* vendor specific data pointer which can be filled by the user */
} i5_dm_interface_type;

/* Structure to represent Legacy (Non-1905) Neighbor Device of Current Device Object */
typedef struct {
  i5_ll_listitem ll;
  unsigned char  state;
  unsigned char  LocalInterfaceId[MAC_ADDR_LEN];  /* MAC address of Local Device's Interface (LcIf)
                                     * on which the Neighbor was seen
                                     */
  unsigned char  NeighborInterfaceId[MAC_ADDR_LEN]; /* Neighbor Device’s Interface from which
                                     * this Neighbor was seen (NbIF) to Local Device
                                     */
} i5_dm_legacy_neighbor_type;

/* Structure to represent 1905 Neighbor Device of Current Device Object */
typedef struct {
  i5_ll_listitem   ll;
  unsigned char    state;
  unsigned char    LocalInterfaceId[MAC_ADDR_LEN]; /* MAC address of Local Device's Interface (LcIf)
                                     * on which the Neighbor was seen
                                     */
  unsigned char    Ieee1905Id[MAC_ADDR_LEN]; /* Neighbor Device's 1905 AL MAC address (NbAL) */
  unsigned char    NeighborInterfaceId[MAC_ADDR_LEN]; /* Neighbor Device’s Interface from which
                                     * this Neighbor was seen (NbIF) to Local Device
                                     */
  unsigned char    IntermediateLegacyBridge; /* Bridge on which Neighbor is seen */
  unsigned short   MacThroughputCapacity;             /* in Mbit/s */
  unsigned short   availableThroughputCapacity;       /* in Mbit/s */
#ifdef MULTIAP
  ieee1905_backhaul_link_metric metric; /* Backhaul Link Metrics for neighbor */
  ieee1905_old_backhaul_link_counter old_metric; /* Old backhaul metric counters */
#endif /* MULTIAP */
  unsigned int     prevRxBytes;
  unsigned int     latestRxBytes;
  unsigned char    ignoreLinkMetricsCountdown;
  char             localIfname[I5_MAX_IFNAME];
  unsigned int     localIfindex;
  void            *bridgeDiscoveryTimer;

  void *vndr_data;    /* vendor specific data pointer which can be filled by the user */
} i5_dm_1905_neighbor_type;

/* Structure to represent Bridging Tuples in a Device Object */
typedef struct {
  i5_ll_listitem ll;
  unsigned char  state;
  char           ifname[I5_MAX_IFNAME];
  unsigned int   forwardingInterfaceListNumEntries; /* Number of MAC address
                                 * of forwarding interfaces in current Device
                                 */
  unsigned char  ForwardingInterfaceList[FWD_IF_LIST_LEN]; /* List of MAC address
                                 * of forwarding interfaces in current Device
                                 */
} i5_dm_bridging_tuple_info_type;

/* Structure to hold the device info retrieved from M1 msg */
#define WSC_BUF64_BYTE		64
#define WSC_BUF32_BYTE		32
typedef struct {
  char device_name[WSC_BUF32_BYTE + 1];
  char manufacturer[WSC_BUF64_BYTE + 1];
  char model_name[WSC_BUF32_BYTE + 1];
  char model_num[WSC_BUF32_BYTE + 1];
  char serial_num[WSC_BUF32_BYTE + 1];
  bool is_present;				/* flag to track whether device info is present or not */
} i5_dm_wsc_device_info_t;

/* Structure to represent Device objects in a Network Topology */
typedef struct i5_dm_device_type_t_{
  i5_ll_listitem ll;
  unsigned char  state;
  unsigned char  queryState;
  unsigned char  validated;
  unsigned char  numTopQueryFailures;
  unsigned char  nodeVersion;
  void          *nodeVersionTimer;
  unsigned int   hasChanged;
  void          *watchdogTimer;
  unsigned int   Version;
  unsigned char  DeviceId[MAC_ADDR_LEN]; /* Unique IEEE 1905 (AL) MAC address of 1905 Device */
  unsigned int   InterfaceNumberOfEntries; /* Number of Interface objects in this Device */
  unsigned int   LegacyNeighborNumberOfEntries; /* Number of Legacy (Non-1905) Devices connected */
  unsigned int   Ieee1905NeighborNumberOfEntries; /* Number of 1905 Device objects connected */
  unsigned int   BridgingTuplesNumberOfEntries; /* Number of Bridging Tuples of this Device */
  void          *psock;  /* Store the psock information on which the packet is recieved for this */
  unsigned int   flags;	/* Flags of type I5_CONFIG_FLAG_XXX :
                                 * Flags of Device object which saves its type (Controller /Registrar /CtrlAgent /Agent),
                                 * type of logical link (DWDS/Eth) between current & Local Device,
                                 */
#ifdef MULTIAP
  void          *steerOpportunityTimer; /* Timer to be created after sending steering
                                           * opportunity request for a device
                                           */
  unsigned char BasicCaps;	/* AP Basic caps. of Type IEEE1905_AP_CAPS_FLAGS_XXXX */
  unsigned short macThroughPutCapacity; /* MAC Throughput Capacity from this to the parent device */
  unsigned char profile;		/* MultiAP Profile */
  unsigned char bh_bss_band; /* Of type BAND_XXX. For dual band it will have both 2G and 5G */
  unsigned char bsta_band; /* Of type BAND_XXX. For dual band it will have both 2G and 5G */
  unsigned char bands;	   /* Of type BAND_XXX. Supported bands by the device */
#if defined(MULTIAPR2)
  unsigned char byte_cntr_unit; /* Byte Counter Units to be used while sending the stats to this
                                 * device. This value is updated while sending the AP capability
                                 * TLV to this device
                                 */
  i5_dm_p2_ap_capability_type p2ApCap;	/* Profile-2 AP AP Capability */
  ieee1905_chscan_report_msg stored_chscan_results; /* Channel Scan Reports of Self Device to send
                                                     * Requested Scan - Stored
                                                     */
  ieee1905_opclass_chan_info_list_t *active_info; /* list of active channels (controller only) */
  ieee1905_opclass_chan_info_list_t *inactive_info; /* list of inactive(radar detected) channels
						       *(controller only)
						       */
  uint8 *cac_cap_info_buf; /* CAC capability info buffer pointer */
  uint16 cac_cap_info_len; /* CAC capability info buffer length */
#endif /* MULTIAPR2 */
#endif /* MULTIAP */
  char           friendlyName[I5_DEVICE_FRIENDLY_NAME_LEN];

  void *vndr_data;    /* vendor specific data pointer which can be filled by the user */

  i5_dm_interface_type           interface_list; /* List of Interface objects in this Device */
  i5_dm_legacy_neighbor_type     legacy_list; /* List of Legacy(Non-1905) Devices connected */
  i5_dm_1905_neighbor_type       neighbor1905_list; /* List of 1905 Device objects connected */
  i5_dm_bridging_tuple_info_type bridging_tuple_list; /* List of Bridging Tuples of this Device */
  struct i5_dm_device_type_t_ *parentDevice;  /* Pointer to the parent device */
  time_t active_time;
  time_t added_time;  /* Time at which device has added */
  i5_dm_wsc_device_info_t dev_info; /* Device info retrieved from wsc m1 msg */
} i5_dm_device_type;

typedef struct {
  void            *dmLinkMetricTimer;
  char            dmLinkMetricIntervalValid;
  unsigned int    dmLinkMetricIntervalMsec;
  void            *dmLinkMetricActivatedTimer;
} i5_dm_link_metric_autoquery_type;

/* Structure to represent SmartMesh Data Model of current Network Topology */
typedef struct {
  i5_dm_device_type device_list; /* List of Device objects in a Network Topology */
  i5_dm_device_type *selfDevice; /* Pointer to the self device for easy access */
  i5_dm_device_type *currentControllerDevice; /* Pointer to the current controller device */
  unsigned int   DevicesNumberOfEntries; /* Number of Device objects in a Network Topology */
  unsigned char  updateStpNeeded;
  i5_dm_link_metric_autoquery_type linkMetricAuto;
  void          *pLinkMetricTimer;
  void          *pPerAPLinkMetricTimer;
} i5_dm_network_topology_type;

#endif
