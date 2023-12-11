#include <signal.h>
#include "TencentWiFi.h"
#include "../mp_util.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include "../site_typedefs.h"
#include <wlioctl.h>
#include <kwb_ioctl.h>

#define BUFSIZE 256
#define DATARATEMBIT (1000 * 8 / 1024 / 1024)

int bcm_get_chanim_stats(void *wl, char *buf);
int bcm_get_sta_info(void *wl, char *ea, char *buf);
int get_rssi(void *wl, char *mac_addr);

void wl_get_pktq_stats2(session_node* node, char* output_buf);

char if_name_2G[] = "wlan0.2";
char if_name_5G[] = "wlan1.2";

float avg_tx_num = 0;
float high_num = 0;


uint32_t mp_version_num = 675602003;

void init_hal()
{

	
	struct ifreq ifr;

	strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	bcm_dlystats_clear((void*)&ifr);

	strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	bcm_dlystats_clear((void*)&ifr);

	turn_off_rts(FREQUENCY_BAND_2GHZ);
	turn_off_rts(FREQUENCY_BAND_5GHZ);

	uwb_reset_wmm_qos_info("wlan0.2");
	uwb_reset_wmm_qos_info("wlan1.2");


	//turn_on_rts();

	//TODO: excucute all the 3 commands once to clear the previous data


}

//call every time we finish the 200ms wifi info gathering
void clear_hal_info()
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	bcm_dlystats_clear((void*)&ifr);

	strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	bcm_dlystats_clear((void*)&ifr);

}

int get_hal_version()
{
	return HAL_VERSION_UGW6;

}

static int exec_prog(char **argv)
{
    pid_t   my_pid;
    int     status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;

    if (0 == (my_pid = fork())) {
            if (-1 == execve(argv[0], (char **)argv , NULL)) {
                    perror("child process execve failed [%m]");
                    return -1;
            }
    }

#ifdef WAIT_FOR_COMPLETION
    timeout = 1000;

    while (0 == waitpid(my_pid , &status , WNOHANG)) {
            if ( --timeout < 0 ) {
                    perror("timeout");
                    return -1;
            }
            sleep(1);
    }

    printf("%s WEXITSTATUS %d WIFEXITED %d [status %d]\n",
            argv[0], WEXITSTATUS(status), WIFEXITED(status), status);

    if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
            perror("%s failed, halt system");
            return -1;
    }

#endif
    return 0;
}


int add_ac_queue_tuple(int wlan_interface,five_tuples_t tuple,int priority)
{   
	wb_wmm_info_t msg;
	unsigned char *ifname = NULL;
	struct in_addr srcip;
	struct in_addr dstip;
	int ret;

	if(wlan_interface == FREQUENCY_BAND_5GHZ)
		ifname = "wlan1.2";
	if(wlan_interface == FREQUENCY_BAND_2GHZ)
		ifname = "wlan0.2";

	inet_pton(AF_INET,tuple.src_addr,(void*)&srcip);
	inet_pton(AF_INET,tuple.dst_addr,(void*)&dstip);
	//msg.srcip = ntohl(srcip.s_addr);
	msg.stream.ip_info.srcip = ntohl(srcip.s_addr);
	msg.stream.ip_info.srcport = atoi(tuple.src_port);
	msg.stream.ip_info.dstip = ntohl(dstip.s_addr);
	msg.stream.ip_info.dstport = atoi(tuple.dst_port);

	if(strcmp(tuple.protocol,"tcp")==0) {
		msg.stream.ip_info.protocol = 6;
	} else if (strcmp(tuple.protocol,"udp")==0) {
		msg.stream.ip_info.protocol = 17;
	}
	

	msg.priority = (char)priority;
	msg.stream_id = 1; //ip_info

    ret = uwb_add_wmm_qos_info(ifname, &msg);
    if (ret) {
		debug_print("ifname:%s,src ip:%s,src port:%s,dst ip:%s,dst port:%s,protocol:%s\n",ifname,tuple.src_addr,tuple.src_port,tuple.dst_addr,tuple.dst_port,tuple.protocol);
        printf("%s[%d] uwb_set_wme_ac_ip error. ret = %d\n", __func__, __LINE__, ret);
        perror("Error:");
    }

	return ret;

}

int del_ac_queue_by_ip(int wlan_interface,int src_dst_ip,char* ip_address)
{
	wb_wmm_info_t msg;
	int ret;
	unsigned char *ifname = NULL;
	struct in_addr ipaddr;
	inet_pton(AF_INET,ip_address,(void*)&ipaddr);

	if(wlan_interface == FREQUENCY_BAND_5GHZ)
		ifname = "wlan1.2";
	if(wlan_interface == FREQUENCY_BAND_2GHZ)
		ifname = "wlan0.2";

	msg.stream.ip_info.dstip = ntohl(ipaddr.s_addr);
	msg.stream_id = 1; //ip_info

    ret = uwb_del_wmm_qos_info(ifname, &msg);
    if (ret) {
		debug_print("ifname:%s,ip:%s\n",ifname,ip_address);
        printf("%s[%d] uwb_del_wme_ac_ip error. ret = %d\n", __func__, __LINE__, ret);
        perror("Error:");
    }	
}

int ap_set_wme_ac_ip(char* client_ip, char* server_ip,int netlen,int srcport,int priority,int stream_id, int freq_band)
{

}

//int mp_set_wme_ac_ip(char *server_ip,int netlen,int srcport,int priority,int stream_id, int freq_band)
//{

	/*
     kwb_wme_info_t msg;
     //unsigned char *ifname = "wlan1";
	 unsigned char *ifname = NULL;
     struct in_addr srcip;
     unsigned int netmask;
     int ret,i;

	 if(freq_band == FREQUENCY_BAND_5GHZ)
	 	ifname = "wlan1";
	 if(freq_band == FREQUENCY_BAND_2GHZ)
	 	ifname = "wlan0";

    inet_pton(AF_INET,server_ip,(void*)&srcip);
    
     //netmask
     if (netlen < 0 || netlen > 32) {
         printf("error: %s[%d]: netmask len rang 1~32 cur value %d!\n"
             , __func__, __LINE__
             , netlen);
         return WIFIBASE_OUTRANGE;
     }
     if (netlen > 0) {
         netmask = 0x80000000;
     } else {
         netmask = 0;
     }
     for (i = 0; i < netlen-1; i++) {
         netmask = netmask >>1;
         netmask |= 0x80000000;
     }
     //srcport
     if (srcport < 0 || srcport > 0xffff) {
         printf("ERROR: %s[%d]: srcport rang 0~65535 cur value %d!\n"
             , __func__, __LINE__
             , srcport);
         return WIFIBASE_OUTRANGE;
     }
     //priority
     if (priority < 0 || priority > 7) {
         printf("ERROR: %s[%d]: priority rang 0~7cur value %d!\n"
             , __func__, __LINE__
             , priority);
         return WIFIBASE_OUTRANGE;
     }
     
     memset(&msg, 0, sizeof(kwb_wme_info_t));
     msg.srcip = ntohl(srcip.s_addr);
     msg.netmask = netmask;
     msg.srcport = srcport;
     msg.priority = priority;
     msg.itemid = stream_id;
    
     printf("%s[%d]: set ip 0x%08x:%u netmask 0x%08x pri %u [item:%u] [ifname:%s]\n"
         , __func__, __LINE__
         , msg.srcip
         , msg.srcport
         , msg.netmask
         , msg.priority
         , msg.itemid
		 , ifname
		 );
     ret = uwb_set_wme_ac_ip(ifname, &msg);
     if (ret) {
         printf("%s[%d] uwb_set_wme_ac_ip error. ret = %d\n", __func__, __LINE__, ret);
         perror("Error:");
     }
    
     return ret;
	 */
//}


int set_wmm_traffic(int wlan_interface,char *server_ip_address,int net_mask_length,int port,int ac_queue_tos )
{
    char net_mask_string[80];
    sprintf(net_mask_string,"%d",net_mask_length);

    char port_string[80];
    sprintf(port_string,"%d",port);

    char ac_queue_tos_string[80];
    sprintf(ac_queue_tos_string,"%d",ac_queue_tos);

    char* args[] = {"/bin/wifibase","-s","set_wme_ac_ip","wlan1",server_ip_address,net_mask_string,port_string,ac_queue_tos_string, NULL};
    exec_prog((char **)args);
}

int clear_wmm_traffic()  //TODO: the tos queue id should be maintain
{   
    int ac_queue_tos = 7; 
    char ac_queue_tos_string[80];
    sprintf(ac_queue_tos_string,"%d",ac_queue_tos);

    char* args[] = {"/bin/wifibase","-s","set_wme_ac_ip","wlan1","0.0.0.0","0","0",ac_queue_tos_string, NULL};
    exec_prog((char **)args);
}

int set_wmm_parameter(char* ac_queue_name,char* parameter_name,char* parameter_number) 
{   
    char* args[] = {"/bin/wl","-i","wl1.2","wme_ac","ap",ac_queue_name,parameter_name,parameter_number,NULL};
    exec_prog((char **)args);
}

int disable_wlan0() 
{   
    char* args[] = {"/bin/wl","-i","wl0.2","down",NULL};
    exec_prog((char **)args);
}


int ip_to_mac_address(char* ip_addr, char* mac_address) 
{
    char cmd[BUFSIZE];	 
    char buf[BUFSIZE];
    FILE *fp;
	int found = 0;
	char dest[BUFSIZE];
    memset(dest, 0, BUFSIZE);
    int num = 0;
	char *tempstr = NULL;

	sprintf(cmd, "arp %s",ip_addr); 

    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return -1;
    }

	    
    if(fgets(buf, BUFSIZE, fp) != NULL) {
		tempstr = strstr(buf, "at");
		printf("tempstr-%s\n",tempstr);
		num =strstr(tempstr,"[") - tempstr ;
		printf("tempstr num-%d\n",num);
		//strncpy(dest,tempstr+3,num-3);
		strncpy(dest,tempstr+3,num-4);
		strcpy(mac_address, dest);
		pclose(fp);
		return 0;
    }

    if(pclose(fp))  {
        printf("Command not found or exited with error status\n");
        return -1;
    }
	printf("Mac addr not found\n");
    return -1;
}

int split(char dst[][BUFSIZE], char* str, const char* spl)
{

    int n = 0;
    char *result = NULL;
	char a;
	int i=1;
	a = str[0];
	while (a!=NULL) {
		a = str[i];
		++i;
	}
    result = strtok(str, spl);
    while( result != NULL )
    {
        strcpy(dst[n++], result);
        result = strtok(NULL, spl);
    }
    return n;
}

ch_info* get_channel_info(session_node* node)
{
    //ioctl test
	struct ifreq ifr;
	if(node->freg_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(node->freg_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}

	unsigned char mac[6];
	sscanf(node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

	char ioctl_buf[WLC_IOCTL_MAXLEN];
	ch_info* temp_info = bcm_get_chanim_stats((void*)&ifr, ioctl_buf);


	return temp_info;
}

delay_info* get_macdelay_info(session_node* node)
{

	struct ifreq ifr;
	if(node->freg_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(node->freg_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}

	unsigned char mac[6];
	sscanf(node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);


	char ioctl_buf[WLC_IOCTL_MAXLEN];


	int ac_index;
	if(node->stream_priority == 0)   //BE
		ac_index = 0;  //BE
	else if(node->stream_priority == 4 || node->stream_priority == 5)   //VI
		ac_index = 2;  //VI
	else if(node->stream_priority == 6 || node->stream_priority == 7)   //VO
		ac_index = 3;  //VO
	else{
		//stream priority value is abnormal	
		char message[UDP_BUFSIZE];
		int mem_index;
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_report(message,node->session_id, SESSION_ERROR_ABNORMAL_STREAM_PRIORITY);
		if(udp_send(node->sock_udp_fd,&node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",node->session_id);
		}
		return NULL;
	}

	delay_info* new_delayinfo = bcm_get_dlystats((void*)&ifr, mac, ioctl_buf, ac_index);

	//bcm_dlystats_clear((void*)&ifr);

	return new_delayinfo;
}

void wl_get_pktq_stats(session_node* node, char* output_buf)
{

	char cmd[BUFSIZE];
	char buf[BUFSIZE];
	char dest[BUFSIZE];
	FILE *fp;
	
	memset(dest, 0, BUFSIZE);
	int num = 0;
	char *tempstr = NULL;	

	if(node->freg_band == FREQUENCY_BAND_2GHZ)
		sprintf(cmd, "wl -i wl0.2 pktq_stats A:%s",node->sta_mac_addr);  

	if(node->freg_band == FREQUENCY_BAND_5GHZ)
		sprintf(cmd, "wl -i wl1.2 pktq_stats A:%s",node->sta_mac_addr);  

	
	printf("cmd: %s\n",cmd);


	if ((fp = popen(cmd, "r")) == NULL) {
			printf("Error opening pipe!\n");
			return -1;
	}
	if(fgets(buf, BUFSIZE, fp) == NULL){
		printf("No return for pktq_stats!\n");
		pclose(fp);
			return -1;
	}



	int line_index = 0;
	
	while (fgets(buf, BUFSIZE, fp) != NULL) {


		printf("wl_output: %s\n",buf);
		
		char dst[20][BUFSIZE];
		char* spl = ":";
    	int cnt = split(dst, buf, spl);
		
		if(line_index > 0 && line_index < 9){	
			char dst2[20][BUFSIZE];
			char* spl = ",";
			int cnt2 = split(dst2, dst[1], spl);
			int len = strlen(dst2[0]);   //0 is the element to store the Tx requested 
			int i;
			char temp[20];
			strcpy(temp, dst2[0]);
			for(i =0; i<len; i++){
				if(temp[i] != 0x20)
				break;
			}
			strcat(output_buf,",");
			strcat(output_buf,temp+i-1);					
			
		}
		line_index += 1;
	}

	pclose(fp);
	
	//printf("output_buf: %s\n",output_buf);


}


void wl_get_pktq_stats2(session_node* node, char* output_buf)
{

	char cmd[BUFSIZE];
	char buf[BUFSIZE];
	char dest[BUFSIZE];
	FILE *fp;
	
	memset(dest, 0, BUFSIZE);
	int num = 0;
	char *tempstr = NULL;	

	if(node->freg_band == FREQUENCY_BAND_2GHZ)
		sprintf(cmd, "wl -i wl0.2 pktq_stats N:%s",node->sta_mac_addr);  

	if(node->freg_band == FREQUENCY_BAND_5GHZ)
		sprintf(cmd, "wl -i wl1.2 pktq_stats N:%s",node->sta_mac_addr);  

	
	printf("cmd: %s\n",cmd);


	if ((fp = popen(cmd, "r")) == NULL) {
			printf("Error opening pipe!\n");
			return -1;
	}
	if(fgets(buf, BUFSIZE, fp) == NULL){
		printf("No return for pktq_stats!\n");
		pclose(fp);
			return -1;
	}



	int line_index = 0;
	
	while (fgets(buf, BUFSIZE, fp) != NULL) {


		printf("wl_output: %s\n",buf);
		
		char dst[20][BUFSIZE];
		char* spl = ":";
    	int cnt = split(dst, buf, spl);
		
		if(line_index > 0 && line_index < 9){	
			char dst2[20][BUFSIZE];
			char* spl = ",";
			int cnt2 = split(dst2, dst[1], spl);
			int len = strlen(dst2[0]);   //0 is the element to store the Tx requested 
			int i;
			char temp[20];
			strcpy(temp, dst2[0]);
			for(i =0; i<len; i++){
				if(temp[i] != 0x20)
				break;
			}
			strcat(output_buf,",");
			strcat(output_buf,temp+i-1);					
			
		}
		line_index += 1;
	}

	pclose(fp);
	
	//printf("output_buf: %s\n",output_buf);


}

int get_connection_rssi(session_node* node)
{
	struct ifreq ifr;
	if(node->freg_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(node->freg_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}

	unsigned char mac[6];
	sscanf(node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

	int rssi = bcm_get_rssi((void*)&ifr, mac);

	if(rssi == RSSI_ERROR_VALUE){
		if(node->freg_band == FREQUENCY_BAND_2GHZ){
			strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
		}
		if(node->freg_band == FREQUENCY_BAND_5GHZ){
			strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
		}

		rssi = bcm_get_rssi((void*)&ifr, mac);
		if(rssi == RSSI_ERROR_VALUE){
			return RSSI_CHECK_DEASSOCIATE;
		}
		else{
			return RSSI_CHECK_BAND_SWITCH;
		}
	}

	return rssi;
}


sta_info* get_station_info(session_node* node)
{
	struct ifreq ifr;
	if(node->freg_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(node->freg_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}

	unsigned char mac[6];
	sscanf(node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);


	char ioctl_buf[WLC_IOCTL_MAXLEN];

	sta_info* info = bcm_get_sta_info((void*)&ifr, mac, ioctl_buf);
	return info;

}


pktq_info* get_pktq_info(session_node* node)
{


	struct ifreq ifr;
	if(node->freg_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(node->freg_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}

	unsigned char mac[6];
	sscanf(node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);


	char ioctl_buf[WLC_IOCTL_MAXLEN];

	int ac_index;
	if(node->stream_priority == 0)   //BE
		ac_index = 0;  //BE
	else if(node->stream_priority == 4 || node->stream_priority == 5)   //VI
		ac_index = 2;  //VI
	else if(node->stream_priority == 6 || node->stream_priority == 7)   //VO
		ac_index = 3;  //VO
	else{
		//stream priority value is abnormal	
		char message[UDP_BUFSIZE];
		int mem_index;
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_report(message,node->session_id, SESSION_ERROR_ABNORMAL_STREAM_PRIORITY);
		if(udp_send(node->sock_udp_fd,&node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",node->session_id);
		}
		return NULL;
	}
    


	pktq_info* info = bcm_get_pktq_stats((void*)&ifr,  mac, ioctl_buf, ac_index, node->stream_priority);

    

    //abnormal scenario, ioctl return requested = 0, we want to get the wl return to compare what is wrong
	
	if(info->requested < 1){
		char debug_string[256];
		sprintf(debug_string,"[pktq_stats wl]SP:%d,AC:%d,WL-",node->stream_priority, ac_index);
		char *temp_string;
		char output_buf[128];
		memset(output_buf,0, 128);
		//wl_get_pktq_stats2(node,output_buf);	
		//memset(output_buf,0, 128);
		wl_get_pktq_stats(node,output_buf);		
		strcat(debug_string, output_buf);
		//debug_print("ioctl debug pktq_stats:%s\n",debug_string);


		char message[UDP_BUFSIZE];
		int mem_index;
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_detailed_report(message,node->session_id, SESSION_ERROR_UNKNOWN, debug_string);
		if(udp_send(node->sock_udp_fd,&node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",node->session_id);
		}

		return NULL;

	}
	

	return info;

}



ap_info* get_bssap_info(session_node* node)
{

	struct ifreq ifr;
	if(node->freg_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(node->freg_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}

	char ioctl_buf[WLC_IOCTL_MAXLEN];
	ap_info* temp_info = bcm_get_bs_data((void*)&ifr, ioctl_buf);

	return temp_info;

}


wifi_info* get_wifi_info(session_node* cb_node)
{
    char message[UDP_BUFSIZE];
    int mem_index;
    int ret;
    static int i;
    unsigned char *ifname = NULL;
    wb_sta_chanim_info_t chanim_stats;
    wb_ap_chanim_info_t params;
    wb_dlystats_info_t dlystats_info;
    kwb_chan_info_t chan_info = {0};
    int ac_index;

    if(cb_node->stream_priority == 0)   //BE
        ac_index = 0;  //BE
    else if(cb_node->stream_priority == 4 || cb_node->stream_priority == 5)   //VI
        ac_index = 2;  //VI
    else if(cb_node->stream_priority == 6 || cb_node->stream_priority == 7)   //VO
        ac_index = 3;  //VO
    else{
        //stream priority value is abnormal 
        char message[UDP_BUFSIZE];
        int mem_index;
        memset(message, 0, UDP_BUFSIZE);
        mem_index = compose_session_error_report(message,cb_node->session_id, SESSION_ERROR_ABNORMAL_STREAM_PRIORITY);
        if(udp_send(cb_node->sock_udp_fd,&cb_node->addr, message, mem_index) == -1){
            debug_print("session id:%s udp send error\n",cb_node->session_id);
        }
        return NULL;
    }

    memset(&chanim_stats, 0, sizeof(wb_sta_chanim_info_t));
    memset(&params, 0, sizeof(wb_ap_chanim_info_t));
    memset(&dlystats_info, 0, sizeof(wb_dlystats_info_t));
    
    if(cb_node->freg_band == FREQUENCY_BAND_5GHZ)
        ifname = "wlan1.2";
    if(cb_node->freg_band == FREQUENCY_BAND_2GHZ)
        ifname = "wlan0.2";

    sscanf(cb_node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &chanim_stats.mac[0], &chanim_stats.mac[1], &chanim_stats.mac[2], &chanim_stats.mac[3], &chanim_stats.mac[4], &chanim_stats.mac[5]);
    sscanf(cb_node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dlystats_info.mac[0], &dlystats_info.mac[1], &dlystats_info.mac[2], &dlystats_info.mac[3], &dlystats_info.mac[4], &dlystats_info.mac[5]);
    chanim_stats.stream_priority = cb_node->stream_priority;

    ret = uwb_get_sta_chanim_stats(ifname, &chanim_stats);
    if (ret) {
       printf("%s[%d] uwb_get_sta_chanim_info error. ret = %d\n", __func__, __LINE__, ret);
       perror("Error:");
       return NULL;
    }

    ret = uwb_get_ap_chanim_stats(ifname, &params);
    if (ret) {
       printf("%s[%d] case_get_ap_chanim_info error. ret = %d\n", __func__, __LINE__, ret);
       perror("Error:");
       return NULL;
    }

    ret = uwb_get_sta_dlystats(ifname, &dlystats_info);
    if (ret) {
       printf("%s[%d] uwb_get_dlystats error. ret = %d\n", __func__, __LINE__, ret);
       perror("Error:");
       return NULL;
    }

    ret = uwb_get_chan_info(ifname, &chan_info);
    if (ret < 0) {
        printf("uwb_get_chan_info is failed,maybe the user input is error,ifname = %s\n", ifname);
        return NULL;
    }
    
	int rssi = chanim_stats.rssi;
	if(rssi == RSSI_CHECK_BAND_SWITCH){   
		debug_print("session id:%s STA already swtich band, stop session\n",cb_node->session_id);
		mp_stop_session(cb_node->session_id, SESSION_STOPREASON_SWITCH_BAND);
		return NULL;
	}
	else if(rssi == RSSI_CHECK_DEASSOCIATE){
		debug_print("session id:%s STA deassociated, stop session\n",cb_node->session_id);
		if(cb_node->session_status == SESSION_STATUS_ONGOING && cb_node->last_rssi > -70){
			mp_stop_session(cb_node->session_id, SESSION_STOPREASON_DEASSOC_HIGH_RSSI);
		}
		else{
			mp_stop_session(cb_node->session_id, SESSION_STOPREASON_DEASSOC);
		}
		return NULL;
	}
	else{
		cb_node->last_rssi = rssi;
	}

	int idle_rate_total = params.txop + chanim_stats.air_use;
	if(idle_rate_total > 100)
		idle_rate_total = 100;

	float tx_attempt_count = 0;
	if((chanim_stats.retried > 0) && (chanim_stats.requested > 0)){
		tx_attempt_count = (float) chanim_stats.retried / (float) chanim_stats.requested;
		tx_attempt_count += 1;
	}
	else{
		tx_attempt_count = 1;
	}

    if(i == 300) {
        i = 0;
        printf("%s %d avg_tx_num=%f %f %02x\n",__func__, __LINE__, avg_tx_num, high_num, chanim_stats.mac[5]);
        printf("%s %d avg_tx_num=%f %f %02x\n",__func__, __LINE__, avg_tx_num, high_num, chanim_stats.mac[5]);
    }
    if(tx_attempt_count < 2) {
        avg_tx_num += tx_attempt_count;
    } else {
        high_num++;
    }
    i++;
    
	wifi_info* temp_info;
    temp_info = (wifi_info *) malloc(sizeof(wifi_info));
	memset(temp_info, 0, sizeof(wifi_info));

	temp_info->rssi = chanim_stats.rssi;
	temp_info->phy_rate = chanim_stats.phy_rate;
	temp_info->sta_count = params.sta_count;
	temp_info->idle_rate = idle_rate_total;
	temp_info->txop = params.txop;
	temp_info->tx_attempt_avg = tx_attempt_count;
	temp_info->tx_fail_cnt = chanim_stats.txfailed;
	temp_info->tx_data_rate = chanim_stats.data_rate;
	temp_info->broken_signal_cnt = params.brokensignal_count;
	temp_info->noise_level = params.noise_level;
	temp_info->channel_bw = chan_info.bandwidth;     
	temp_info->freq_band = cb_node->freg_band;      //0-2.4G  1-5G
	temp_info->overall_datarate = params.overall_datarate;
	temp_info->air_time = chanim_stats.air_use;
	temp_info->lost_ac_pkts = dlystats_info.lost_ac_pkts[ac_index];
	temp_info->total_ac_pkts = dlystats_info.total_ac_pkts[ac_index];
	temp_info->avg_mac_delay = dlystats_info.avg_mac_delay[ac_index];
	temp_info->max_mac_delay = dlystats_info.max_mac_delay[ac_index];
	temp_info->mac_delay_dist1_count = dlystats_info.mac_delay_dist1_count[ac_index];
	temp_info->mac_delay_dist2_count = dlystats_info.mac_delay_dist2_count[ac_index];
	temp_info->mac_delay_dist3_count = dlystats_info.mac_delay_dist3_count[ac_index];
	temp_info->mac_delay_dist4_count = dlystats_info.mac_delay_dist4_count[ac_index];
	temp_info->mac_delay_dist5_count = dlystats_info.mac_delay_dist5_count[ac_index];
	temp_info->pktq_pkt_dropped = chanim_stats.txfailed;
	temp_info->pktq_queue_length = chanim_stats.utlsn;	
	temp_info->nss = chanim_stats.nss;
	temp_info->mcs = chanim_stats.mcs;
	temp_info->rts_tx_cnt = chanim_stats.rts_tx_cnt;
	temp_info->rts_failed_cnt = chanim_stats.rts_failed_cnt;
	temp_info->ps_cnt = chanim_stats.ps_cnt;
	temp_info->channel_num = chan_info.channel;
	temp_info->rspec = chanim_stats.rspec;
	temp_info->be_mcs = chanim_stats.be_mcs;

	//TODO RX UCAST rx_ucast_pkts
	temp_info->rx_ucast_pkts = chanim_stats.rx_ucast_pkts;
	temp_info->rx_pkts_retried = chanim_stats.rx_pkts_retried;

	return temp_info;
}




wifi_info* get_wifi_info_bcm(session_node* cb_node)
{

	char message[UDP_BUFSIZE];
	int mem_index;

	int rssi = get_connection_rssi(cb_node);
	if(rssi == RSSI_CHECK_BAND_SWITCH){   
		debug_print("session id:%s STA already swtich band, stop session\n",cb_node->session_id);
		mp_stop_session(cb_node->session_id, SESSION_STOPREASON_SWITCH_BAND);
		return NULL;
	}
	else if(rssi == RSSI_CHECK_DEASSOCIATE){
		debug_print("session id:%s STA deassociated, stop session\n",cb_node->session_id);
		if(cb_node->session_status == SESSION_STATUS_ONGOING && cb_node->last_rssi > -70){
			mp_stop_session(cb_node->session_id, SESSION_STOPREASON_DEASSOC_HIGH_RSSI);
		}
		else{
			mp_stop_session(cb_node->session_id, SESSION_STOPREASON_DEASSOC);
		}
		return NULL;
	}
	else{
		cb_node->last_rssi = rssi;
	}



	delay_info* new_delayinfo = get_macdelay_info(cb_node);
	if(new_delayinfo == NULL){
		debug_print("get delay info failed\n");
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_report(message,cb_node->session_id, SESSION_ERROR_ABNORMAL_MACDELAY_DATA);
		if(udp_send(cb_node->sock_udp_fd,&cb_node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",cb_node->session_id);
		}
		return NULL;
	}	
	else{
		/*
		debug_print("[ioctl]lost:%d\n", new_delayinfo->lost_ac_pkts);
		debug_print("[ioctl]total:%d\n", new_delayinfo->total_ac_pkts);
		debug_print("[ioctl]avg delay:%d\n", new_delayinfo->avg_mac_delay);
		debug_print("[ioctl]max delay:%d\n", new_delayinfo->max_mac_delay);
		debug_print("[ioctl]delay dist 1 count:%d\n", new_delayinfo->mac_delay_dist1_count);
		debug_print("[ioctl]delay dist 2 count:%d\n", new_delayinfo->mac_delay_dist2_count);
		debug_print("[ioctl]delay dist 3 count:%d\n", new_delayinfo->mac_delay_dist3_count);
		debug_print("[ioctl]delay dist 4 count:%d\n", new_delayinfo->mac_delay_dist4_count);
		debug_print("[ioctl]delay dist 5 count:%d\n", new_delayinfo->mac_delay_dist5_count);
		*/
			
		
	}

	pktq_info* new_pktqinfo = get_pktq_info(cb_node);
	if(new_pktqinfo == NULL){
		debug_print("get pktq info failed\n");
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_report(message,cb_node->session_id, SESSION_ERROR_ABNORMAL_PKTQSTATS_DATA);
		if(udp_send(cb_node->sock_udp_fd,&cb_node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",cb_node->session_id);
		}
		free(new_delayinfo);
		return NULL;
	}	
	else{
		/*
		debug_print("[ioctl]requested:%d\n", new_pktqinfo->requested);
		debug_print("[ioctl]txfailed:%d\n", new_pktqinfo->txfailed);
		debug_print("[ioctl]retried:%d\n", new_pktqinfo->retried);
		debug_print("[ioctl]txfailed:%d\n", new_pktqinfo->utlsn);
		debug_print("[ioctl]datarate:%f\n", new_pktqinfo->data_rate);
		debug_print("[ioctl]phyrate:%d\n", new_pktqinfo->phy_rate);
		debug_print("[ioctl]bandwidth:%d\n", new_pktqinfo->bandwidth);
		
		debug_print("[ioctl]airuse:%d\n", new_pktqinfo->air_use);  
		
		debug_print("[ioctl]nsscount:%f\n", new_pktqinfo->nss_count);  
		debug_print("[ioctl]mcsmedium:%d\n", new_pktqinfo->mcs_medium);  
		
		*/
	}

	ch_info* new_chinfo = get_channel_info(cb_node);
	if(new_chinfo == NULL){
		debug_print("get channel info failed\n");
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_report(message,cb_node->session_id, SESSION_ERROR_ABNORMAL_CHANNEL_DATA);
		if(udp_send(cb_node->sock_udp_fd,&cb_node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",cb_node->session_id);
		}
		free(new_delayinfo);
		free(new_pktqinfo);
		return NULL;
	}	
	else{
		
		//debug_print("txop:%d\n",new_chinfo->txop);
		//debug_print("broken count:%d\n",new_chinfo->brokensignal_count);
		//debug_print("noise level:%d\n",new_chinfo->noise_level);
		
		
	}


	ap_info* new_apinfo = get_bssap_info(cb_node);
	if(new_apinfo == NULL){
		debug_print("get bssap info failed\n");
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_report(message,cb_node->session_id, SESSION_ERROR_ABNORMAL_BSS_DATA);
		if(udp_send(cb_node->sock_udp_fd,&cb_node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",cb_node->session_id);
		}
		free(new_delayinfo);
		free(new_pktqinfo);
		free(new_chinfo);
		return NULL;
	}
	else{
		
		//debug_print("sta count:%d\n",new_apinfo->sta_count);
		//debug_print("overall datarate:%d\n",new_apinfo->overall_datarate);
		


	}


	sta_info* new_stainfo = get_station_info(cb_node);
	if(new_stainfo == NULL){
		debug_print("get sta info failed\n");
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_report(message,cb_node->session_id, SESSION_ERROR_ABNORMAL_STA_DATA);
		if(udp_send(cb_node->sock_udp_fd,&cb_node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",cb_node->session_id);
		}
		free(new_delayinfo);
		free(new_pktqinfo);
		free(new_chinfo);
		free(new_apinfo);
		return NULL;
	}
	else{
		
		//debug_print("[ap_core]channel num:%d\n",new_stainfo->channel_num);
		//debug_print("[ap_core]ps_cnt:%d\n",new_stainfo->ps_cnt);
		//debug_print("[ap_core]rts_tx_cnt:%d\n",new_stainfo->rts_tx_cnt);
		//debug_print("[ap_core]rts_failed_cnt:%d\n",new_stainfo->rts_failed_cnt);
		//debug_print("overall datarate:%d\n",new_apinfo->overall_datarate);
	
	}

	int idle_rate_total = new_chinfo->txop + new_pktqinfo->air_use;
	if(idle_rate_total > 100)
		idle_rate_total = 100;

	float tx_attempt_count;
	if((new_pktqinfo->retried > 0) && (new_pktqinfo->requested > 0)){
		tx_attempt_count = (float) new_pktqinfo->retried / (float) new_pktqinfo->requested;
		tx_attempt_count += 1;
	}
	else{
		tx_attempt_count = 1;
	}


	wifi_info* temp_info;
    temp_info = (wifi_info *) malloc(sizeof(wifi_info));
	memset(temp_info, 0, sizeof(wifi_info));

	temp_info->rssi = rssi;
	temp_info->phy_rate = new_pktqinfo->phy_rate;
	temp_info->sta_count = new_apinfo->sta_count;
	temp_info->idle_rate = idle_rate_total;
	temp_info->txop = new_chinfo->txop;
	temp_info->tx_attempt_avg = tx_attempt_count;
	temp_info->tx_fail_cnt = new_pktqinfo->txfailed;
	temp_info->tx_data_rate = new_pktqinfo->data_rate;
	temp_info->broken_signal_cnt = new_chinfo->brokensignal_count;
	temp_info->noise_level = new_chinfo->noise_level;
	temp_info->channel_bw = new_pktqinfo->bandwidth;     
	temp_info->freq_band = cb_node->freg_band;      //0-2.4G  1-5G
	temp_info->overall_datarate = new_apinfo->overall_datarate;
	temp_info->air_time = new_pktqinfo->air_use;
	temp_info->lost_ac_pkts = new_delayinfo->lost_ac_pkts;
	temp_info->total_ac_pkts = new_delayinfo->total_ac_pkts;
	temp_info->avg_mac_delay = new_delayinfo->avg_mac_delay;
	temp_info->max_mac_delay = new_delayinfo->max_mac_delay;
	temp_info->mac_delay_dist1_count = new_delayinfo->mac_delay_dist1_count;
	temp_info->mac_delay_dist2_count = new_delayinfo->mac_delay_dist2_count;
	temp_info->mac_delay_dist3_count = new_delayinfo->mac_delay_dist3_count;
	temp_info->mac_delay_dist4_count = new_delayinfo->mac_delay_dist4_count;
	temp_info->mac_delay_dist5_count = new_delayinfo->mac_delay_dist5_count;
	temp_info->pktq_pkt_dropped = new_pktqinfo->txfailed;
	temp_info->pktq_queue_length = new_pktqinfo->utlsn;	
	temp_info->nss = new_pktqinfo->nss_count;
	temp_info->mcs = new_pktqinfo->mcs_medium;
	temp_info->rts_tx_cnt = new_stainfo->rts_tx_cnt;
	temp_info->rts_failed_cnt = new_stainfo->rts_failed_cnt;
	temp_info->ps_cnt = new_stainfo->ps_cnt;
	temp_info->channel_num = new_stainfo->channel_num;


	free(new_delayinfo);
	free(new_pktqinfo);
	free(new_chinfo);
	free(new_apinfo);
	free(new_stainfo);
	return temp_info;



}



int get_sta_freq_band(session_node* node)
{
	char cmd[BUFSIZE];	  
	sprintf(cmd, "wl -i wl1.2 sta_info %s",node->sta_mac_addr);

	char buf[BUFSIZE];
	FILE *fp;
	char dest[BUFSIZE];
	memset(dest, 0, BUFSIZE);
	int num = 0;
	char *tempstr = NULL;

	if ((fp = popen(cmd, "r")) == NULL) {
		printf("Error opening pipe!\n");
		return -1;
	}

	if(fgets(buf, BUFSIZE, fp) == NULL){
		printf("No return for sta_info!\n");
		node->freg_band = FREQUENCY_BAND_2GHZ;
	}
	else{
		node->freg_band = FREQUENCY_BAND_5GHZ;
	}
	pclose(fp);
	return 0;
	
}

int turn_off_rts(int freq_band)
{

	char *cmd = NULL;
	int err;

	FILE *fp;

	//printf("[DECA]turn off rts\n");

	struct ifreq ifr;

	if(freq_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(freq_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}


	if(err = wlu_iovar_setint((void*)&ifr, "ampdu_rts", 0) < 0){
		printf("[DECA] turn off %s RTS failed, reason:%d\n", ifr.ifr_name, err);
		return err;
	}	

	if(err = wlu_iovar_setint((void*)&ifr, "rtsthresh", 8000) < 0){
		printf("[DECA] set RTS thresh failed, reason:%d\n", err);
	}

	return err;	

	/*

	cmd = "wl -i wl1.2 ampdu_rts 0";	  
	if ((fp = popen(cmd, "r")) == NULL) {
			printf("Error opening pipe!\n");
			return -1;
	}

	cmd = "wl -i wl1.2 rtsthresh 8000";	  
	if ((fp = popen(cmd, "r")) == NULL) {
			printf("Error opening pipe!\n");
			return -1;
	}

	return 0;	

	*/

}

int turn_on_rts(int freq_band)
{

	char *cmd = NULL;
	int err;

	FILE *fp;

	//printf("[DECA]turn on rts\n");


	struct ifreq ifr;

	
	if(freq_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(freq_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}


	if(err = wlu_iovar_setint((void*)&ifr, "ampdu_rts", 1) < 0){
		printf("[DECA] turn on RTS failed, reason:%d\n", err);
		return err;
	}	

	if(err = wlu_iovar_setint((void*)&ifr, "rtsthresh", 2347) < 0){
		printf("[DECA] set RTS thresh failed, reason:%d\n", err);
	}

	return err;	
	



	/*

	cmd = "wl -i wl1.2 ampdu_rts 1";	  
	if ((fp = popen(cmd, "r")) == NULL) {
			printf("Error opening pipe!\n");
			return -1;
	}

	cmd = "wl -i wl1.2 rtsthresh 2347";	  
	if ((fp = popen(cmd, "r")) == NULL) {
			printf("Error opening pipe!\n");
			return -1;
	}

	*/

	//return 0;	
}

int get_rts_status(int freq_band)
{	
	int rts_status;
	int err;
	struct ifreq ifr;

	if(freq_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifr.ifr_name, "wl0.2", IFNAMSIZ);
	}
	if(freq_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifr.ifr_name, "wl1.2", IFNAMSIZ);
	}

	if(err = wlu_iovar_getint((void*)&ifr, "ampdu_rts", &rts_status) < 0){
		printf("[DECA] get RTS status failed, reason:%d\n", err);
		return -1;
	}

	printf("[DECA] RTS status check:%d\n", rts_status);

    return rts_status;

}


int set_fixrate_AC(int ac_number, int mcs, int rspec,int freq_band)
{
	int ret;
	unsigned char ifname[BUFSIZE];
	wb_feature_info_t info;
	memset(&info, 0, sizeof(wb_feature_info_t));


	if(freq_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifname, "wlan0.2", IFNAMSIZ);
	}
	if(freq_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifname, "wlan1.2", IFNAMSIZ);
	}

	info.speed_traffic = ac_number;
    rspec = (rspec & ~WB_RSPEC_MCS_MASK) | mcs;
    ret = uwb_fix_rate(ifname, &rspec, &info);

    if (ret) {
        printf("%s[%d] uwb_fix_rate error. ret = %d\n", __func__, __LINE__, ret);
        perror("Error:");
    } 

	return ret;

}


int set_fixrate_AC_off(int ac_number,int freq_band) 
{
	int ret;
	int rspec;
	unsigned char ifname[BUFSIZE];
	wb_feature_info_t info;
	memset(&info, 0, sizeof(wb_feature_info_t));


	if(freq_band == FREQUENCY_BAND_2GHZ){
		strncpy(ifname, "wlan0.2", IFNAMSIZ);
	}
	if(freq_band == FREQUENCY_BAND_5GHZ){
		strncpy(ifname, "wlan1.2", IFNAMSIZ);
	}

	info.speed_traffic = ac_number;
    rspec = 0;
    ret = uwb_fix_rate(ifname, &rspec, &info);

    if (ret) {
        printf("%s[%d] uwb_fix_rate error. ret = %d\n", __func__, __LINE__, ret);
        perror("Error:");
    } 

	return ret;	
}
