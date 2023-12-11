#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
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

#include "TencentWiFi.h"

#include <linux/netlink.h>
#include "holymsg.h"

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


//Tenda Event defines
#define MAX_PAYLOAD 1024 // maximum payload size
#define NETLINK_GAME_SPEED_EVENTS 100 //自定义的协议
#define KM_NETLINK_WIFIEVENTS 0x1011 
#define EVENT_AUDIT_TYPE "platform_msg_magicwifievents.flow_control_grade.from_kernel"
#define WLC_E_SPECIAL_STA_EVENT     207 /* AEB algorithm special sta(game acceleration) message reporting */
#define WLC_E_SPECIAL_STA_ALL_EVENT 208
#define TENCENT_EVENT_CNT       10
#define MAX_TENCENT_SCB         16

static void handle_audit_handler(const char *data, unsigned len);

struct tencent_sta_event
{
	//struct          ether_addr ea;
	unsigned char           ea[6];
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
	//struct          ether_addr ea;
	unsigned char           ea[6];
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


int init_event_socket()
{
    struct sockaddr_nl src_addr, dest_addr;
    int sock_fd, retval;

    // Create a socket
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GAME_SPEED_EVENTS);
    if(sock_fd == -1){
        printf("error getting socket: %s", strerror(errno));
        return -1;
    }
    // To prepare binding
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); //A：设置源端端口号
    src_addr.nl_groups = 0xffffffff;
    //Bind
    retval = bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
    if(retval < 0){
        printf("bind failed: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

	printf("[AEB]Init event socket\n");

	return sock_fd;
}

void mevent_event_handler(int sock_fd)
{
	char sbuff[2048];
	char data[2048];
	struct nlmsghdr *nlh = NULL;
	int iret;
	int len;
	char *databuf = NULL;

	iret = read(sock_fd,sbuff,sizeof(sbuff));

	if (iret < 0){
		printf("[AEB]READ EVENT ERROR\n");
		return;
	} 

	nlh = (struct nlmsghdr *)sbuff;

	if(KM_NETLINK_WIFIEVENTS == nlh->nlmsg_type) {
		memcpy(data, NLMSG_DATA(nlh), nlh->nlmsg_len);
		len = strlen(data) + 1;
		//printf("Received message: %s %d %02x\n",(char *) NLMSG_DATA(nlh),nlh->nlmsg_len,nlh->nlmsg_type);
		if(!strncmp(data,EVENT_AUDIT_TYPE,strlen(data))) {
			databuf = data + len;
			//printf("Received data message: %s %d\n",data,len);
			len = nlh->nlmsg_len - len;
			handle_audit_handler(databuf,len);
		}
	}	

}

/*
void mevent_event_handler(int sock_fd)
{
    int state;
    int len;	
    struct nlmsghdr *nlh = NULL; //Netlink数据包头
    struct iovec iov;
    struct msghdr msg;
    char *data = NULL;
    char *databuf = NULL;
    int state_smg = 0;		
	struct sockaddr_nl dest_addr;

    len = sizeof(EVENT_AUDIT_TYPE);

	printf("mevent_event_handler!\n");
    
    data = (char *)malloc(NLMSG_SPACE(MAX_PAYLOAD));

    // To orepare create mssage
    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	
    if(!nlh){
        printf("malloc nlmsghdr error!\n");
        close(sock_fd);
        //return -1;
	}
	

    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid(); //C：设置源端口
    nlh->nlmsg_flags = 0;

    iov.iov_base = (void *)nlh;
    iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);

    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;
    //Create mssage
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;

	state = recvmsg(sock_fd, &msg, 0);
	if(state<0)
	{
		printf("state<1");
	}
	if(KM_NETLINK_WIFIEVENTS == nlh->nlmsg_type) {
		memcpy(data, NLMSG_DATA(nlh), nlh->nlmsg_len);
		len = strlen(data) + 1;
		//printf("Received message: %s %d %02x\n",(char *) NLMSG_DATA(nlh),nlh->nlmsg_len,nlh->nlmsg_type);
		if(!strncmp(data,EVENT_AUDIT_TYPE,strlen(data))) {
			databuf = data + len;
			//printf("Received data message: %s %d\n",data,len);
			len = nlh->nlmsg_len - len;
			handle_audit_handler(databuf,len);
		}
	}
	free(nlh);
	free(data);
}
*/

static void handle_audit_handler(const char *data, unsigned len)
{
	cJSON *status;
	cJSON *databuf;
    cJSON *datalen;
    cJSON *ifname;
    cJSON *buf;
    cJSON *subitem;
    char *name;
	struct tencent_sta_event sta;
	struct tencent_all_event all;
	struct tencent_event_info *info;
    info = all.tencent_event_info;
    int i,j;

    cJSON *root = NULL;
    char *msg = NULL;
    
    if (!data) {
        printf("invalid param\n");
        return ;
    }

    root = cJSON_Parse(data);
    if (!root) {
        printf("%s %d cjson obj is NULL\n");
        return ;
    }

    ifname = cJSON_GetObjectItem(root,"ifname");
    if (!ifname) {
        printf("ifname from %s failed\n", data);
        return ;
    }
    name = ifname->valuestring;
    status = cJSON_GetObjectItem(root,"status");
    if (!status) {
        printf("status from %s failed\n", data);
        return ;
    }

    datalen= cJSON_GetObjectItem(root,"data_len");
    if (!datalen) {
        printf("datalen from %s failed\n", data);
        return ;
    }
    
    //printf("data_len %d\n",datalen->valueint);


    msg = cJSON_Print(root);
    if (NULL == msg) {
        cJSON_Delete(root);
        return ;
    }
    
   // printf("you receive:%s\n", msg);
        if (status->valueint == WLC_E_SPECIAL_STA_EVENT) {
                databuf= cJSON_GetObjectItem(root,"mac");
                if (!databuf) {
                    printf("databuf mac from %s failed\n", data);
                    return ;
                }
                sscanf(databuf->valuestring, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &sta.ea[0], &sta.ea[1], &sta.ea[2], &sta.ea[3], &sta.ea[4], &sta.ea[5]);

                databuf= cJSON_GetObjectItem(root,"rssi");
                if (!databuf) {
                    printf("databuf rssi from %s failed\n", data);
                    return ;
                }
                sta.rssi = databuf->valueint;
                sta.rssi > 0 ? sta.rssi - 255 : sta.rssi;

                databuf= cJSON_GetObjectItem(root,"rssi_level");
                if (!databuf) {
                    printf("databuf rssi_level from %s failed\n", data);
                    return ;
                }
                sta.rssi_level = databuf->valueint;

                databuf= cJSON_GetObjectItem(root,"need_event");
                if (!databuf) {
                    printf("databuf need_event from %s failed\n", data);
                    return ;
                }
                sta.need_event = databuf->valueint;

                databuf= cJSON_GetObjectItem(root,"idlerate_level");
                if (!databuf) {
                    printf("databuf idlerate_level from %s failed\n", data);
                    return ;
                }
                sta.idlerate_level = databuf->valueint;

                databuf= cJSON_GetObjectItem(root,"utils_level");
                if (!databuf) {
                    printf("databuf utils_level from %s failed\n", data);
                    return ;
                }
                sta.utils_level = databuf->valueint;

                databuf= cJSON_GetObjectItem(root,"idlerate_percent");
                if (!databuf) {
                    printf("databuf idlerate_percent from %s failed\n", data);
                    return ;
                }
                sta.idlerate_percent = databuf->valueint;

                databuf= cJSON_GetObjectItem(root,"queue_utils");
                if (!databuf) {
                    printf("databuf queue_utils from %s failed\n", data);
                    return ;
                }
                sta.queue_utils = databuf->valueint;

            printf("[%s AEB]sta_event: idlerate_percent=%u, rssi=%d, queue_utils=%u, idlerate_level=%u, rssi_level=%u,utils_level=%u\n", name, sta.idlerate_percent, sta.rssi, sta.queue_utils,sta.idlerate_level, sta.rssi_level, sta.utils_level);
        }

        if (status->valueint == WLC_E_SPECIAL_STA_ALL_EVENT) {
            all.datalen = datalen->valueint;
            // reach MAX event cnt(10) event process
            databuf= cJSON_GetObjectItem(root,"sta_num");
            if (!databuf) {
                printf("databuf sta_num from %s failed\n", data);
                return ;
            }
            all.sta_num = databuf->valueint;

            databuf= cJSON_GetObjectItem(root,"Sendinfo");
            if (!databuf) {
                printf("databuf Sendinfo from %s failed\n", data);
                return ;
            }
            databuf = databuf->child;
            
            for(i = 0; i < all.sta_num; i++) {
                buf = cJSON_GetObjectItem(databuf,"mac");
                if (!buf) {
                    printf("buf mac from %s failed\n", data);
                    return ;
                }
                sscanf(buf->valuestring, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &info->ea[0], &info->ea[1], &info->ea[2], &info->ea[3], &info->ea[4], &info->ea[5]);

                buf = cJSON_GetObjectItem(databuf,"rssi");
                for(j = 0; j < cJSON_GetArraySize(buf); j++)
                {
                    subitem = cJSON_GetArrayItem(buf, j);
                    info[i].rssi[j] = subitem->valueint;
                    info[i].rssi[j] > 0 ? info[i].rssi[j] - 255 : info[i].rssi[j];
                }

                buf = cJSON_GetObjectItem(databuf,"idlerate_percent");
                for(j = 0; j < cJSON_GetArraySize(buf); j++)
                {
                    subitem = cJSON_GetArrayItem(buf, j);
                    info[i].idlerate_percent[j] = subitem->valueint;
                }

                buf = cJSON_GetObjectItem(databuf,"queue_utils");
                for(j = 0; j < cJSON_GetArraySize(buf); j++)
                {
                    subitem = cJSON_GetArrayItem(buf, j);
                    info[i].queue_utils[j] = subitem->valueint;
                }
                databuf = databuf->next;
            }
            
            printf("%s %d [AEB]idlerate0:%u, idlerate9:%u, rssi0:%d, rssi9:%d, utils0:%u, utils9:%u\n", name, all.sta_num, info[0].idlerate_percent[0], info[0].idlerate_percent[9], info[0].rssi[0],info[0].rssi[9],info[0].queue_utils[0], info[0].queue_utils[9]);
        }
        

    return ;
}


/*
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
		
		if (type == WLC_E_SPECIAL_STA_EVENT) {
			//sta over thershold event process
			sta = (struct tencent_sta_event *)databuf;
			printf("[AEB]sta_event: idlerate_percent=%u, rssi=%d, queue_utils=%u, idlerate_level=%u, rssi_level=%u,utils_level=%u\n", sta->idlerate_percent, sta->rssi, sta->queue_utils,sta->idlerate_level, sta->rssi_level, sta->utils_level);
			
		}
		
		if (type == WLC_E_SPECIAL_STA_ALL_EVENT) {
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
*/

int enable_aeb_event_report(aeb_setting* setting_info)
{
	special_sta_t* info;
	info = (special_sta_t*)malloc(sizeof(special_sta_t));
	memset(info, 0, sizeof(special_sta_t));


	printf("[AEB]enable_aeb_event_report\n");

	memcpy(&info->ea, setting_info->mac_addr, ETHER_ADDR_LEN);
	info->action = 1;
	info->idlerate_thresh[0] = setting_info->idlerate_threshold1;
	info->idlerate_thresh[1] = setting_info->idlerate_threshold2;
	info->rssi_thresh[0] = setting_info->rssi_threshold1;
	info->rssi_thresh[1] = setting_info->rssi_threshold2;
	info->utils_thresh[0] = setting_info->pktqlen_threshold1;
	info->utils_thresh[1] = setting_info->pktqlen_threshold2;
	info->timer_windows = setting_info->timer_ms;

    struct ifreq ifr;
	int err;

    if(setting_info->freq_band == FREQUENCY_BAND_2GHZ){
        strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
    }
      if(setting_info->freq_band == FREQUENCY_BAND_5GHZ){
        strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
    }

	if ((err = wlu_iovar_set((void*)&ifr,"special_sta" , info, sizeof(special_sta_t)) < 0)) {
		printf("[AEB]set failed\n");
	}

	printf("[AEB]set ok\n");
		
	return err;

}

