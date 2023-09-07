#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/types.h>
#include <linux/filter.h>

#include <ethernet.h>
#include <bcmevent.h>
#include <bcmutils.h>

#include "Tencentwifi.h"

void mevent_event_handler(int sock)
{
	int bytes;
	int datalen;
	bcm_event_t *dpkt;
	uchar buf_ptr[MAX_EVENT_BUFFER_LEN], *ptr = buf_ptr;
	uint32 type;
	uint32 status;
	char *databuf;
	struct tencent_sta_event *sta;
	struct tencent_all_event *all;
	struct tencent_event_info *info;
	
	char eabuf[ETHER_ADDR_STR_LEN];
				
	if ((bytes = recv(sock, ptr, MAX_EVENT_BUFFER_LEN, 0)) > IFNAMSIZ) {
				
		ptr = ptr + IFNAMSIZ;
		dpkt = (bcm_event_t *)ptr;
			
			
		type = ntohl(dpkt->event.event_type);
		//datalen = ntohl(dpkt->event.datalen);
		status = ntohl(dpkt->event.status);
		
		databuf = (char *)(&dpkt->event + 1);
		
		if (type == WLC_E_TENCENT_STA_EVENT) {
			//sta over thershold event process
			sta = (struct tencent_sta_event *)databuf;
			printf("[AEB]sta_event: idlerate_percent=%u, rssi=%d, queue_utils=%u, idlerate_level=%u, rssi_level=%u,utils_level=%u\n", sta->idlerate_percent, sta->rssi, sta->queue_utils,sta->idlerate_level, sta->rssi_level, sta->utils_level);
			
		}
		
		if (type == WLC_E_TENCENT_ALL_EVENT) {
			// reach MAX event cnt(10) event process
			all = (struct tencent_all_event *)databuf;
			printf("[AEB]all_event: sta_num=%d,datalen=%d\n",all->sta_num, all->datalen );

	        for(int i=0; i<all->sta_num; i++) {
			info = &all->tencent_event_info[i];
			printf("[AEB]idlerate0:%u, idlerate9:%u, rssi0:%d, rssi9:%d, utils0:%u, utils9:%u\n", info->idlerate_percent[0], info->idlerate_percent[9], info->rssi[0],info->rssi[9],info->queue_utils[0], info->queue_utils[9]);
		    }
			
		}
	}
}

int enable_aeb_event_report(char *ea, uint8 timer_window)
{
	tencent_sta_t* info;
	info = (tencent_sta_t*)malloc(sizeof(tencent_sta_t));
	memset(info, 0, sizeof(tencent_sta_t));


	printf("[AEB]enable_aeb_event_report\n");

	memcpy(&info->ea, ea, ETHER_ADDR_LEN);
	info->action = 1;
	info->idlerate_thresh[0] = 50;
	info->idlerate_thresh[1] = 80;
	info->rssi_thresh[0] = -55;
	info->rssi_thresh[1] = -70;
	info->utils_thresh[0] = 30;
	info->utils_thresh[1] = 80;
	info->timer_windows = timer_window;
	process_monitor_info(info);

}