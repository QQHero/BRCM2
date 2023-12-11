/* @author: Tencent torowen */
#ifndef _MPAEB_H_
#define _MPAEB_H_

#include <linux/if_ether.h>
#include <linux/types.h>

#include <ethernet.h>
#include <wlioctl.h>


/* get_ifname_unit() index is << 4 */
#define EAPD_WKSP_PORT_INDEX_SHIFT	4
#define EAPD_WKSP_SPORT_OFFSET		(1 << 5)
#define EAPD_WKSP_MPORT_OFFSET		(1 << 6)
#define EAPD_WKSP_VX_PORT_OFFSET	(1 << 7)

#define EAPD_WKSP_MEVENT_UDP_PORT	44000
#define EAPD_WKSP_MEVENT_UDP_RPORT 	EAPD_WKSP_MEVENT_UDP_PORT
#define EAPD_WKSP_MEVENT_UDP_SPORT 	EAPD_WKSP_MEVENT_UDP_PORT + EAPD_WKSP_SPORT_OFFSET

#define BCM_EVENT_HEADER_LEN	(sizeof(bcm_event_t))
#define MAX_EVENT_BUFFER_LEN	1400
#define MAX_LINE_LEN			16

#define TENCENT_EVENT_CNT       10
#define MAX_TENCENT_SCB         16

struct tencent_sta_event
{
	struct          ether_addr ea;
	uint8           need_event; //bit map[bit2 utils, bit1 rssi, bit0 idlerate]
	uint8           idlerate_level;
	uint8           rssi_level;
	uint8           utils_level;
	uint8           idlerate_percent;
	int8            rssi;
	uint32          queue_utils;
};
struct  tencent_event_info
{
	struct          ether_addr ea;
	uint16					pad;
	uint8           idlerate_percent[TENCENT_EVENT_CNT];
	int8            rssi[TENCENT_EVENT_CNT];
	uint32          queue_utils[TENCENT_EVENT_CNT];
};
struct  tencent_all_event
{
	uint32          sta_num;
	uint32          datalen;
	struct          tencent_event_info tencent_event_info[0];
};

int enable_aeb_event_report(char*, uint8_t);
int process_monitor_info(tencent_sta_t*);



#endif
