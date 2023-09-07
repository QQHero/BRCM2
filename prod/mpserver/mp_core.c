/* @author: Tencent slimehsiao */

#include "mp_server.h"
#include "mp_core.h"
#include "mp_udpclient.h"
#include "mp_util.h"
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "TencentWiFi.h"

#include <iof_lib_drv.h>
#include <pthread.h>
#include <errno.h>
#include "TencentWiFi.h"



#define STARTGAME_MAX_BQW_ENABLED_STREAM_COUNT 1
#define COMMON_MAX_BQW_ENABLED_STREAM_COUNT 3
#define PROXY_LIST_SIZE 10

uint32_t magic_num = 2881128040; 
int BQW_enabled_stream_count = 0;         //BQW enabled(put in VI or VO) SP count
static int session_started_count = 0;  
static int gather_count = 0;     
char BQW_enabled_string[32] = "BQW_enabled:1";
char BQW_disabled_string[32] = "BQW_enabled:0";
char DECA_enabled_string[32] = "DECA_enabled:1";
char DECA_disabled_string[32] = "DECA_enabled:0";

int priority_used_5G[4] = {0};
int queue_priority_5G[4] = {4,5,6,7};  //6,7 for VO.  4,5 for VI
int priority_used_2G[4] = {0};
int queue_priority_2G[4] = {4,5,6,7};  //4,5 for VO.  6,7 for VI

//int deca_probe_enabled;   //deca probe is enabled
//int deca_probe_failed;    //deca probe ever failed for this session

struct proxy_ip_node
{
	char ip_addr[32];
	int  ref_count;
	int  stream_id;
	int  stream_priority;
	int  BQW_enabled;
};


struct proxy_ip_node proxy_list[PROXY_LIST_SIZE];


void send_session_info(session_node* cb_node);


void init_mp_core()
{
	//mp_set_wme_ac_ip("0.0.0.0", 0,  0, 0, 0, 0);  //for streamid 0, band0
	//mp_set_wme_ac_ip("0.0.0.0", 0,  0, 0, 0, 1);  //for streamid 0, band1	
	deca_init();

}

void init_proxy_list()
{
	for(int i = 0; i < PROXY_LIST_SIZE ; i++){
		strcpy(proxy_list[i].ip_addr, "");
		proxy_list[i].ref_count = -1;
		proxy_list[i].stream_id = -1;
		proxy_list[i].stream_priority = 0;
		proxy_list[i].BQW_enabled = -1;

	}

}

int get_proxy_ip_index(char* ip_addr)
{
	int i = 0;
	while (i < PROXY_LIST_SIZE){
		if(strcmp(proxy_list[i].ip_addr, ip_addr) == 0){
			return i;
		}
		i++;
	}
	//debug_print("This proxy ip is not existed!\n");
	return -1;	
}

int add_proxy_ip_node(char* ip_addr, int ref_count, int stream_id, int stream_priority, int BQW_enabled)
{	
	int i = 0;
	while (i < PROXY_LIST_SIZE){
		if(strcmp(proxy_list[i].ip_addr, "") == 0){
			strcpy(proxy_list[i].ip_addr, ip_addr);
			proxy_list[i].ref_count = ref_count;
			proxy_list[i].stream_id = stream_id;
			proxy_list[i].stream_priority = stream_priority;
			proxy_list[i].BQW_enabled = BQW_enabled;
			return i;
		}
		i++;
	}
	debug_print("proxy ip list is full!\n");
	return -1;	

}

void delete_proxy_ip_node(int index)
{
	strcpy(proxy_list[index].ip_addr, "");
	proxy_list[index].ref_count = -1;
	proxy_list[index].stream_id = -1;
	proxy_list[index].stream_priority = 0;
	proxy_list[index].BQW_enabled = -1;
}


void init_report_list()
{
	for(int i = 0; i < REPORT_LIST_SIZE ; i++){
		strcpy(report_list[i].ip_addr, "");
		report_list[i].ref_count = -1;
		report_list[i].sock_udp_fd = -1;
		memset(&report_list[i].addr, 0, sizeof(struct sockaddr_in));
	}

}

int get_report_ip_index(char* ip_addr)
{
	int i = 0;
	while (i < REPORT_LIST_SIZE){
		if(strcmp(report_list[i].ip_addr, ip_addr) == 0){
			return i;
		}
		i++;
	}
	//debug_print("This report ip is not existed!\n");
	return -1;	
}

int add_report_ip_node(char* ip_addr, int ref_count, int sock_udp_fd, struct sockaddr_in addr)
{	
	int i = 0;
	while (i < REPORT_LIST_SIZE){
		if(strcmp(report_list[i].ip_addr, "") == 0){
			strcpy(report_list[i].ip_addr, ip_addr);
			report_list[i].ref_count = ref_count;
			report_list[i].sock_udp_fd = sock_udp_fd;
			memcpy(&report_list[i].addr, &addr, sizeof(addr));
			return i;
		}
		i++;
	}
	debug_print("report ip list is full!\n");
	return -1;	

}

void delete_report_ip_node(int index)
{
	//debug_print("repot ip addr1:%s\n", report_list[index].ip_addr);
	strcpy(report_list[index].ip_addr, "");
	//debug_print("repot ip addr2:%s\n", report_list[index].ip_addr);
	report_list[index].ref_count = -1;
	report_list[index].sock_udp_fd = -1;
	memset(&report_list[index].addr, 0, sizeof(struct sockaddr_in));
}

int assign_stream_priority(int freq_band)
{	

	int i = 0;
	while (i < 4){
		if(freq_band == FREQUENCY_BAND_5GHZ){
			if(priority_used_5G[i] == 0){
			priority_used_5G[i] = 1;
			return queue_priority_5G[i];
			}
		}
		if(freq_band == FREQUENCY_BAND_2GHZ){
			if(priority_used_2G[i] == 0){
			priority_used_2G[i] = 1;
			return queue_priority_2G[i];
			}
		}
		i++;
	}
	debug_print("stream priority assignment is full!\n");
	return -1;	
}

int release_stream_priority(int priority, int freq_band)
{	 
	int i = 0;
	while (i < 4){
		if(freq_band == FREQUENCY_BAND_5GHZ){
			if(queue_priority_5G[i] == priority){
				priority_used_5G[i] = 0;
				//debug_print("release 5G stream priority:%d\n", priority);
				return 0;
			}
		}
		if(freq_band == FREQUENCY_BAND_2GHZ){
			if(queue_priority_2G[i] == priority){
				priority_used_2G[i] = 0;
				//debug_print("release 2G stream priority:%d\n", priority);
				return 0;
			}
		}
		i++;
	}

	debug_print("release unassigned stream prority:%d in freg_band:%d!\n",priority, freq_band);
	return -1;
}

int check_app_stream_limit(char* app_id)
{
	if(strcmp(app_id, "startgame") == 0){
		return STARTGAME_MAX_BQW_ENABLED_STREAM_COUNT;
	}
	else
		return COMMON_MAX_BQW_ENABLED_STREAM_COUNT;
}

five_tuples_t * mp_get_tuples(char* session_id)
{
	session_node *node = get_session_node(session_id);

	if(node != NULL) {
		return node->tuples;
	} else {
		debug_print("can't find session\n");
		return NULL;
	}	
}

int mp_set_tuples(char* session_id, five_tuples_t *tuples)
{
	int ret;
	session_node *node = get_session_node(session_id);
	five_tuples_t temp_tuples[MAX_TUPLES];

	del_ac_queue_by_ip(node->freg_band,1,node->sta_ip_addr);


	if(node != NULL) {
		memcpy(node->tuples,tuples,sizeof(five_tuples_t)*MAX_TUPLES);
		memcpy(temp_tuples,tuples,sizeof(five_tuples_t)*MAX_TUPLES);
		for(int i=0;i<MAX_TUPLES;i++) {
			if(strcmp(temp_tuples[i].dst_addr,node->sta_ip_addr)==0) {
				//memcpy(&tuple_item,&tuples[i],sizeof(five_tuples_t));
				ret = add_ac_queue_tuple(node->freg_band,temp_tuples[i],node->stream_priority);
				if(ret==-1) {
					debug_print("mp_set_tuple: add_acqueue failed\n");
				}
			}
		}		
		return 1;
	} else {
		debug_print("can't find session\n");
		return 0;
	}



}

int mp_start_session_without_report(char* session_id, char* sta_ip_addr, char* app_id, int timer_ms, uint32_t version_num)
{
	if(get_session_node(session_id) != NULL) {
		debug_print("session already existed\n");
		return 1;
	}

	session_node* new_node = create_session_node(session_id, sta_ip_addr, "0.0.0.0", 0, "0.0.0.0", 0, app_id, timer_ms, version_num);
	if(new_node == NULL){
		debug_print("cannot create new session\n");
		return -1;
	}

	

	debug_print("ip_to_mac_address\n");

	if(ip_to_mac_address(new_node->sta_ip_addr, new_node->sta_mac_addr) == -1){
		debug_print("ip to mac address error\n");
		free(new_node);
		return -1;
	}
	else
		debug_print("mac addr:%s\n",new_node->sta_mac_addr);

	new_node->report = 0;
	new_node->stream_priority = 6;
	new_node->session_config.RS_enabled = 1;

	if(get_sta_freq_band(new_node) == -1){
		debug_print("get sta freq_band failed\n");
		free(new_node);
		return -1;
	}
	else{
		debug_print("sta freg_band: %d", new_node->freg_band);
	}	

	del_ac_queue_by_ip(new_node->freg_band,1,sta_ip_addr);

	debug_print("insert_session_node\n");
	if(insert_session_node(new_node) == -1)
	{
		debug_print("insert session node failed\n");
		free(new_node);
		return 1;
	}

	debug_print("session_status\n");
	new_node->session_status = SESSION_STATUS_START;

	session_started_count += 1;

	return 0;
}


int mp_start_session(char* session_id, char* sta_ip_addr, char* proxy_ip_addr, int proxy_port, char* report_ip_addr, int report_port, char* app_id, int timer_ms, uint32_t version_num)
{
	char message[UDP_BUFSIZE];
	int mem_index;
	memset(message, 0, UDP_BUFSIZE);

	if (session_id==NULL || sta_ip_addr ==NULL || report_ip_addr == NULL || app_id == NULL) {
		debug_print("NULL Input String\n");
		return -1;
	}

	if(get_session_node(session_id) != NULL) {
		debug_print("session already existed\n");
		return 1;
	}

	session_node* new_node = create_session_node(session_id, sta_ip_addr, proxy_ip_addr, proxy_port, report_ip_addr, report_port, app_id,timer_ms, version_num);
	if(new_node == NULL){
		debug_print("cannot create new session\n");
		return -1;
	}
	   
	//only create udp socket fd if proxy node not existed
	int report_ip_index;
	report_ip_index = get_report_ip_index(new_node->report_ip_addr);
	if(report_ip_index == -1){
		int fd = udp_sock_create(new_node->report_ip_addr, new_node->report_port, &new_node->addr);
		if(fd == -1){
			debug_print("udp_sock_create failed\n");
			free(new_node);
			return -1;
		}
		else{
			new_node->sock_udp_fd = fd; 
			new_node->report_ip_index = add_report_ip_node(new_node->report_ip_addr,1,fd, new_node->addr);
			new_node->report = 1;
			//debug_print("udp socket created: proxy ip-%s, port-%d, index-%d",new_node->proxy_ip_addr, new_node->report_port, new_node->report_ip_index );
		}
	}
	else{
		new_node->report_ip_index = report_ip_index;
		report_list[report_ip_index].ref_count += 1;
		new_node->sock_udp_fd = report_list[report_ip_index].sock_udp_fd;
		memcpy(&new_node->addr, &report_list[report_ip_index].addr, sizeof(struct sockaddr_in));
		debug_print("udp socket copied: proxy ip-%s, port-%d, index-%d",new_node->proxy_ip_addr, new_node->report_port, report_ip_index);

	}
	debug_print("ip addr to parse:%s\n",new_node->sta_ip_addr);
	
	if(ip_to_mac_address(new_node->sta_ip_addr, new_node->sta_mac_addr) == -1){
		debug_print("ip to mac address error\n");
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_session_error_report(message, new_node->session_id, SESSION_ERROR_IPTOMACADDR_FAILED);	
		if(udp_send(new_node->sock_udp_fd,&new_node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",new_node->session_id);
		}	
		close(new_node->sock_udp_fd);
		free(new_node);
		return -1;
	}
	else
		debug_print("mac addr:%s\n",new_node->sta_mac_addr);

/*
	if(start_timer(new_node->session_id, &new_node->timer_id, new_node->timer_ms, timer_callback) == -1){
		debug_print("start_timer failed\n");
		memset(message, 0, UDP_BUFSIZE);
		mem_index = compose_error_report(message, new_node->session_id, SESSION_ERROR_TIMER_START_FAILED);	
		if(udp_send(new_node->sock_udp_fd,&new_node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",new_node->session_id);
		}	
		close(new_node->sock_udp_fd);
		free(new_node);
		return -1;
	}
*/

	if(get_sta_freq_band(new_node) == -1){
		debug_print("get sta freq_band failed\n");
		free(new_node);
		return -1;
	}
	else{
		debug_print("sta freg_band: %d", new_node->freg_band);
	}

	if(insert_session_node(new_node) == -1)
	{
		debug_print("insert session node failed\n");
		free(new_node);
		return 1;
	}

	new_node->session_status = SESSION_STATUS_START;
	memset(message, 0, UDP_BUFSIZE);
	mem_index = compose_session_start_report(message, new_node->session_id);	
	if(udp_send(new_node->sock_udp_fd,&new_node->addr, message, mem_index) == -1){
		debug_print("session id:%s udp send error\n",new_node->session_id);
	}


    //enable AEB event report - 10000ms
	
	unsigned char mac[6];
	sscanf(new_node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);


	aeb_setting* setting_info;
	setting_info = (aeb_setting*) malloc(sizeof(aeb_setting));
	memset(setting_info, 0, sizeof(aeb_setting));

	memcpy(setting_info->mac_addr, mac, 6);
	setting_info->freq_band = new_node->freg_band;
	setting_info->timer_ms = 10;
	setting_info->idlerate_threshold1 = 80;
	setting_info->idlerate_threshold2 = 70;
	setting_info->rssi_threshold1 = -55;
	setting_info->rssi_threshold2 = -65;
	setting_info->pktqlen_threshold1 = 40;
	setting_info->pktqlen_threshold2 = 20;

	// Event report
	//enable_aeb_event_report(setting_info);

	free(setting_info);
	
	session_started_count += 1;

	return 0;
}

int mp_stop_session(char* session_id, int stop_reason)
{
	
	session_node* stop_node = get_session_node(session_id);
	char message[UDP_BUFSIZE];
	int mem_index;
	int result = 0;

	if(stop_node!= NULL){
	

        /*
		if(stop_timer(stop_node->timer_id) == -1){
			debug_print("stop_timer failed\n");
			memset(message, 0, UDP_BUFSIZE);
			mem_index = compose_error_report(message, stop_node->session_id, SESSION_ERROR_TIMER_STOP_FAILED);	
			if(udp_send(stop_node->sock_udp_fd,&stop_node->addr, message, mem_index) == -1){
				debug_print("session id:%s udp send error\n",stop_node->session_id);
			}	
			result = -1;
		}
		*/
	
		/* report session stop info to proxy*/
		stop_node->session_status = SESSION_STATUS_STOP;
		memset(message, 0, UDP_BUFSIZE);
		unsigned char ifname_macaddr[6]   = {0};
    	char wanip[MAX_ADDR_SIZE];
		char wanmac[MAX_ADDR_SIZE];

		memset(wanip, 0, MAX_ADDR_SIZE);
		if (0 == ifaddrs_get_ifip(get_eth_name(WAN_1), wanip))
		{
			//printf("WAN PORT IP:%s\n", wanip);


		}
		memset(wanmac, 0, MAX_ADDR_SIZE);
		if (0 == ifaddrs_get_if_netmac(get_eth_name(WAN_1), ifname_macaddr))
		{
			sprintf(wanmac, "%02x:%02x:%02x:%02x:%02x:%02x",
					ifname_macaddr[0], ifname_macaddr[1], ifname_macaddr[2],
					ifname_macaddr[3], ifname_macaddr[4], ifname_macaddr[5]);
			//printf("WAN MAC:%s\n", wanmac);

		}

		int mem_index = compose_session_stop_report(message, stop_node->session_id, stop_reason, stop_node->session_config.BQW_enabled, stop_node->session_config.DECA_enabled,stop_node->mp_version_num, stop_node->app_id, wanmac, wanip);
		if(udp_send(stop_node->sock_udp_fd,&stop_node->addr, message, mem_index) == -1){
			debug_print("session id:%s udp send error\n",stop_node->session_id);
		}

		if(mp_reset_config(session_id) == -1){
			debug_print("mp_reset_config failed\n");
			memset(message, 0, UDP_BUFSIZE);
			mem_index = compose_session_error_report(message, stop_node->session_id, SESSION_ERROR_RESET_AC_QUEUE_CONFIG_FAILED);	
			if(udp_send(stop_node->sock_udp_fd,&stop_node->addr, message, mem_index) == -1){
				debug_print("session id:%s udp send error\n",stop_node->session_id);
			}	
			result = -1;
		}
		//debug_print("mp_reset_config succeeded!\n");

		//check whether report ip is not used and close the socket fd
		report_list[stop_node->report_ip_index].ref_count -= 1;
		if(report_list[stop_node->report_ip_index].ref_count == 0){
			close(report_list[stop_node->report_ip_index].sock_udp_fd);
			delete_report_ip_node(stop_node->report_ip_index);
			//debug_print("delete_report_ip_node success, index:%d\n",stop_node->report_ip_index);
		}

		if(stop_node->session_config.DECA_enabled == 1 ){
 			deca_stop_probe(stop_node->freg_band);
		}

		if (stop_node->session_config.RS_enabled == 1) {
			set_fixrate_AC_off(stop_node->stream_priority,stop_node->freg_band);
		}


		/* already report all UDP data to proxy, can delete session node (including socket fd) now*/
		if(delete_session_node(session_id) == -1){
			debug_print("delete_session_node failed\n");
			result = -1;
		}

		
	}
	else{
		debug_print("mp_stop_session: session_node id: %s not found error\n", session_id);
		result = -1;
	}

	debug_print("session_started_count:%d, session_cnt:%d\n", session_started_count, get_session_count() );

	if(session_started_count > 10 && get_session_count() == 0){
		debug_print("exit program\n");
		exit(0);
	}

	return result;
}


int add_BQW_disable_proxy_node(char* proxy_ip_addr, char* result, char* result_pre)
{
	int proxy_index;
	proxy_index = add_proxy_ip_node(proxy_ip_addr, 1, -1, -1, 0);
	if(proxy_index < 0){
		debug_print("add_proxy_ip_node failed\n");
		return -1;
	}
	else{
		if(hal_deca_supported()){
			sprintf(result, "%s;%s", result_pre, BQW_disabled_string);
		}
		else{
			strcpy(result, BQW_disabled_string);
		}
		return proxy_index;
	}

}

int add_BQW_enable_proxy_node(char* proxy_ip_addr, int stream_id, int stream_prority, char* result, char* result_pre)
{
    int proxy_index;
	proxy_index = add_proxy_ip_node(proxy_ip_addr, 1, stream_id, stream_prority, 1);
	if(proxy_index < 0){
		debug_print("add_proxy_ip_node failed\n");
		return -1;
	}
	else{
		if(hal_deca_supported()){
			sprintf(result, "%s;%s", result_pre, BQW_enabled_string);
		}
		else{
			strcpy(result, BQW_enabled_string);
		}
		return proxy_index;
	}

}


int mp_apply_config(char* session_id, struct apconfig config, char* result)
{	

    if(result == NULL){
		debug_print("mp_apply_config result pointer not allocated!\n");
		return -1;
	}
	else{

		session_node* cf_node = get_session_node(session_id);

		//Test RS Algo
		//cf_node->session_config.RS_enabled = 1;


		char result_pre[32];
		//printf("[DECA] 1st point, config:%d\n", config.DECA_enabled);

		//TODO: need to check appid, only config DECA to 1 if it is startgame

		if(hal_deca_supported()){
			if(config.DECA_enabled == 1 && get_session_count() == 1){
				strcpy(result_pre, DECA_enabled_string);
				cf_node->session_config.DECA_enabled = 1;
				//deca_start_probe(cf_node->freg_band);
				//cf_node->deca_probe_enabled = 1;				
			}		
			else{
				strcpy(result_pre, DECA_disabled_string);
				cf_node->session_config.DECA_enabled = 0;
			}
		}
		else{
			cf_node->session_config.DECA_enabled = 0;			
		}

        //appid not startgame, could have more than one stream in the same AC queue
		if (cf_node != NULL && check_app_stream_limit(cf_node->app_id) > 1) {
				
			int stream_priority;
			int stream_id;
			int proxy_ip_index;

			proxy_ip_index = get_proxy_ip_index(cf_node->proxy_ip_addr);
			//debug_print("mp_apply_config: proxy_ip_index: %d\n",proxy_ip_index);

		
			if(proxy_ip_index >= 0){
				proxy_list[proxy_ip_index].ref_count += 1;
				
				if(proxy_list[proxy_ip_index].BQW_enabled == 1){
					cf_node->session_config.BQW_enabled = 1;
					cf_node->proxy_ip_index = proxy_ip_index;
					cf_node->stream_id = proxy_list[proxy_ip_index].stream_id;
					cf_node->stream_priority = proxy_list[proxy_ip_index].stream_priority;
					if(hal_deca_supported()){
						sprintf(result, "%s;%s", result_pre, BQW_enabled_string);
					}
					else{
						strcpy(result, BQW_enabled_string);
					}
					
				}
				if(proxy_list[proxy_ip_index].BQW_enabled == 0){
					cf_node->session_config.BQW_enabled = 0;
					cf_node->proxy_ip_index = proxy_ip_index;
					if(hal_deca_supported()){
					sprintf(result, "%s;%s", result_pre, BQW_disabled_string);
					}
					else{
						strcpy(result, BQW_disabled_string);
					}
				}

				return 0;
			}
			//proxy ip not exists
			else{

				if(config.BQW_enabled == 1){

					if(BQW_enabled_stream_count < check_app_stream_limit(cf_node->app_id)){

						stream_id = assign_stream_id();

						if(stream_id == -1){
							debug_print("stream_id assign failed\n");
							send_session_error_report(cf_node, SESSION_ERROR_STREAM_ID_ASSIGN_FAILED);
							
							cf_node->session_config.BQW_enabled = 0;
							cf_node->stream_priority = 0;
						
							int index = add_BQW_disable_proxy_node(cf_node->proxy_ip_addr,result, result_pre);
							if(index == -1){
								debug_print("add_proxy_ip_node failed\n");
								return -1;
							}
							else{
								cf_node->proxy_ip_index = index;
								return 0;
							}

						}
						else{
							cf_node->stream_id = stream_id;
						}

						stream_priority = assign_stream_priority(cf_node->freg_band);

						if(stream_priority == -1){
							//release stream id
							release_stream_id(cf_node->stream_id);

							send_session_error_report(cf_node, SESSION_ERROR_STREAM_PRIORITY_ASSIGN_FAILED);

							cf_node->session_config.BQW_enabled = 0;
							cf_node->stream_priority = 0;

							int index = add_BQW_disable_proxy_node(cf_node->proxy_ip_addr,result, result_pre);
							if(index == -1){
								debug_print("add_proxy_ip_node failed\n");
								return -1;
							}
							else{
								cf_node->proxy_ip_index = index;
								return 0;
							}			
						}
						else{
							cf_node->stream_priority = stream_priority;
						}
							
						//int ac_result = mp_set_wme_ac_ip(cf_node->proxy_ip_addr, 24,  cf_node->proxy_port, cf_node->stream_priority, cf_node->stream_id, cf_node->freg_band);
						
						five_tuples_t tuple;
						strcpy(tuple.src_addr,cf_node->proxy_ip_addr);
						strcpy(tuple.dst_addr,cf_node->sta_ip_addr);
						sprintf(tuple.src_port,"%d",cf_node->proxy_port);
						
						sprintf(tuple.dst_port,"0");
						sprintf(tuple.protocol,"tcp");
						
						//add_ac_queue_tuple(int wlan_interface,five_tuples_t tuple,int priority)
						int ac_result = add_ac_queue_tuple(cf_node->freg_band,tuple,cf_node->stream_priority); 
						usleep(200000); //200ms
			
						if(ac_result != 0){

							debug_print("mp_apply_config: set_wme_ac_ip failed\n");

							//release stream id
							release_stream_id(cf_node->stream_id);
							//release stream priority
							release_stream_priority(cf_node->stream_priority, cf_node->freg_band);

							send_session_error_report(cf_node, SESSION_ERROR_APPLY_AC_QUEUE_CONFIG_FAILED);

							cf_node->session_config.BQW_enabled = 0;
							cf_node->stream_priority = 0;

							int index = add_BQW_disable_proxy_node(cf_node->proxy_ip_addr,result, result_pre);
							if(index == -1){
								debug_print("add_proxy_ip_node failed\n");
								return -1;
							}
							else{
								cf_node->proxy_ip_index = index;
								return 0;	
							}

						}
						//after all, BQW enabled succesfully

						cf_node->session_config.BQW_enabled = 1;
						BQW_enabled_stream_count += 1;
						debug_print("BQW enabled stream count:%d\n", BQW_enabled_stream_count);

			
						int index = add_BQW_enable_proxy_node(cf_node->proxy_ip_addr,cf_node->stream_id, cf_node->stream_priority, result, result_pre);
						if(index < 0){
							debug_print("add_proxy_ip_node failed\n");
							return -1;
						}
						else{
							cf_node->proxy_ip_index = index;
							return 0;
						}
					}
					else{
						debug_print("BQW_enabled_stream count exceeds maximum value, BQW disable\n");

						send_session_error_report(cf_node,SESSION_ERROR_BQW_ENABLE_STREAM_EXCEED_MAX);

						cf_node->session_config.BQW_enabled = 0;
						cf_node->stream_priority = 0;

						int index = add_BQW_disable_proxy_node(cf_node->proxy_ip_addr,result, result_pre);
						if(index == -1){
							debug_print("add_proxy_ip_node failed\n");
							return -1;
						}
						else{
							cf_node->proxy_ip_index = index;
							return 0;
						}
					}
				}
				else if(config.BQW_enabled == 0){
					
					cf_node->session_config.BQW_enabled = 0;
					cf_node->stream_priority = 0;

					int index = add_BQW_disable_proxy_node(cf_node->proxy_ip_addr,result, result_pre);
					if(index == -1){
						debug_print("add_proxy_ip_node failed\n");
						return -1;
					}
					else{
						cf_node->proxy_ip_index = index;
						return 0;
					}
				}
			}
		}
		// START case
		else if(cf_node != NULL && check_app_stream_limit(cf_node->app_id) == 1){

			if(config.BQW_enabled == 1 && BQW_enabled_stream_count < 1){
				cf_node->stream_id = 0;
				cf_node->stream_priority = 4;  //fix to VI
				//int ac_result = mp_set_wme_ac_ip(cf_node->proxy_ip_addr, 24,  cf_node->proxy_port, cf_node->stream_priority, cf_node->stream_id, cf_node->freg_band);
				five_tuples_t tuple;
				strcpy(tuple.src_addr,cf_node->proxy_ip_addr);
				//inet_ntop(AF_INET, (void*)&(cf_node->addr), tuple.dst_addr, INET_ADDRSTRLEN);
				strcpy(tuple.dst_addr,cf_node->sta_ip_addr);
				sprintf(tuple.src_port,"%d",cf_node->proxy_port);
				sprintf(tuple.dst_port,"0");
				sprintf(tuple.protocol,"tcp");
						
				int ac_result = add_ac_queue_tuple(cf_node->freg_band,tuple,cf_node->stream_priority); 
				usleep(200000); //200ms
			
				if(ac_result != 0){

					debug_print("mp_apply_config: set_wme_ac_ip failed\n");
					send_session_error_report(cf_node, SESSION_ERROR_APPLY_AC_QUEUE_CONFIG_FAILED);
					cf_node->session_config.BQW_enabled = 0;
					cf_node->stream_id = -1;
					cf_node->stream_priority = 0;
					if(hal_deca_supported()){
						sprintf(result, "%s;%s", result_pre, BQW_disabled_string);
					}
					else{
						strcpy(result, BQW_disabled_string);
					}
					return 0;	
				}
			
				//after all, BQW enabled succesfully

				cf_node->session_config.BQW_enabled = 1;
				BQW_enabled_stream_count += 1;
				debug_print("BQW enabled stream count:%d\n", BQW_enabled_stream_count);
				if(hal_deca_supported()){
					sprintf(result, "%s;%s", result_pre, BQW_enabled_string);
				}
				else{
					strcpy(result, BQW_enabled_string);
				}
				return 0;
			}
			else{
				cf_node->session_config.BQW_enabled = 0;
				cf_node->stream_priority = 0;
				if(hal_deca_supported()){
					sprintf(result, "%s;%s", result_pre, BQW_disabled_string);
				}
				else{
					strcpy(result, BQW_disabled_string);
				}
				return 0;
			}
		}

		else{
			debug_print("mp_apply_config: session_node id: %s not found error\n", session_id);
			return -1;
		}	
	}
}

int mp_reset_config(char* session_id)
{
	session_node* rs_node = get_session_node(session_id);
	//appid not startgame, could have more than one stream in the same AC queue
	if (rs_node != NULL && check_app_stream_limit(rs_node->app_id) > 1) {
		debug_print("mp_reset_config, proxy ip index:%d, BQW enabled:%d\n", rs_node->proxy_ip_index, rs_node->session_config.BQW_enabled);
		// only handle the case with proxy ip already added to the list, other case is fatal error (not handling)
		if(rs_node->proxy_ip_index >= 0){

			//int end_result = 0;
		
			proxy_list[rs_node->proxy_ip_index].ref_count -= 1;

			//remove element from the list and set wifibase api if BQW enabled
			if(proxy_list[rs_node->proxy_ip_index].ref_count == 0){

				if(proxy_list[rs_node->proxy_ip_index].BQW_enabled == 1){

					int ac_result;

					//debug_print("mp_reset_config ref_count=0, stream id:%d, stream priority:%d\n",rs_node->stream_id,rs_node->stream_priority);
					//ac_result = mp_set_wme_ac_ip("0.0.0.0", 0,  0, 0, rs_node->stream_id, rs_node->freg_band);
					ac_result = del_ac_queue_by_ip(rs_node->freg_band,1,rs_node->sta_ip_addr);

					usleep(200000); //200ms
					if(ac_result == 0){
						release_stream_priority(rs_node->stream_priority,rs_node->freg_band);
						release_stream_id(rs_node->stream_id);
						BQW_enabled_stream_count -= 1;
						debug_print("BQW enabled stream count:%d\n", BQW_enabled_stream_count);
					}
					else{
						debug_print("mp_reset_config: set_wme_ac_ip failed\n");
						send_session_error_report(rs_node, SESSION_ERROR_RESET_AC_QUEUE_CONFIG_FAILED);
					}

				}

				//set this proxy list element to initial value
				delete_proxy_ip_node(rs_node->proxy_ip_index);

			}
			return 0;
		}
		else{
			debug_print("mp_reset_config: proxy_ip_index is illegal\n");
			return -1;
		}
	}
	// START case
	else if(rs_node != NULL && check_app_stream_limit(rs_node->app_id) == 1){
		if(rs_node->session_config.BQW_enabled == 1){

			int ac_result;

			//debug_print("mp_reset_config ref_count=0, stream id:%d, stream priority:%d\n",rs_node->stream_id,rs_node->stream_priority);
			//ac_result = mp_set_wme_ac_ip("0.0.0.0", 0,  0, 0, rs_node->stream_id, rs_node->freg_band);
			ac_result = del_ac_queue_by_ip(rs_node->freg_band,1,rs_node->sta_ip_addr);

			usleep(200000); //200ms
			if(ac_result == 0){
	
				BQW_enabled_stream_count -= 1;
				debug_print("BQW enabled stream count:%d\n", BQW_enabled_stream_count);
			}
			else{
				debug_print("mp_reset_config: set_wme_ac_ip failed\n");
				send_session_error_report(rs_node, SESSION_ERROR_RESET_AC_QUEUE_CONFIG_FAILED);
			}
			return 0;

		}
		else{
			return 0;
		}

	}

	else {
		debug_print("mp_reset_config: session_node id: %s not found error\n", session_id);
		return -1;
	}		
}

int mp_report_heartbeat(char* session_id)
{
    session_node* hb_node = get_session_node(session_id);
	if(hb_node!= NULL){
		hb_node->life_time = MAX_SESSION_LIFE_TIME;
		return 0;
	}
	else{
		debug_print("mp_report_heartbeat: session_node id: %s not found error\n", session_id);
		return -1;
	}	
}

int mp_report_timestamp(char* session_id, uint32_t time_sec, uint32_t time_usec)
{
    session_node* ts_node = get_session_node(session_id);
	if(ts_node!= NULL){
		//hb_node->life_time = MAX_SESSION_LIFE_TIME;
		struct time_stamp sent_time, current_time, diff_time;
		sent_time.sec = time_sec;
		sent_time.usec = time_usec;

		ts_node->received_udp_echo = 1;  //ever received udp echo frame in this session

		get_current_timestamp(&current_time);
	
		timestamp_diff(&current_time, &sent_time, &diff_time);
	
		int diff_time_ms = timestamp_in_msecs(&diff_time);


		if(diff_time_ms > ts_node->last_rtt_ms)
			ts_node->last_rtt_ms = diff_time_ms;
		return 0;
	}
	else{
		debug_print("mp_report_timestamp: session_node id: %s not found error\n", session_id);
		return -1;
	}	
}

int check_udp_fd(int fd)
{
	int i = 0;
	while (i < REPORT_LIST_SIZE){
		if(report_list[i].sock_udp_fd == fd){
			return 1;
		}
		i++;
	}
	//debug_print("This fd is not existed!\n");
	return 0;	
}


void mp_session_unit_test(void)
{



}

void gather_ap_info() 
{
	/*
	gather_count += 1;
	if(gather_count < 5)
		return;
	else
		gather_count = 0;
	*/
	//get each session node in list and send the info to server
	session_node* node; 
	session_node* next_node;
	node = get_first_session_node();
	

	//TODO: set lifttime for report
	while(node!=NULL) {
		next_node = node->next;
		if (node->report ==1 ) {
			send_session_info(node);
		}		
		node = next_node;
	}	
	
	clear_hal_info();	
}

void send_session_info(session_node* cb_node) 
{
    
	char message[UDP_BUFSIZE];
	int mem_index;
	int mcs;
	int rspec;

	if(cb_node!= NULL){

		int remain_time = cb_node->life_time - cb_node->timer_ms;
		if(remain_time < 0){
			debug_print("session id:%s life time expired\n",cb_node->session_id);
			mp_stop_session(cb_node->session_id, SESSION_STOPREASON_TIMER_EXPIRED);
			return;
		}
		cb_node->life_time = remain_time;

		wifi_info* new_wifi_info = get_wifi_info(cb_node);

		if(new_wifi_info == NULL){
			return;
		}

		if (cb_node->session_config.RS_enabled == 1) {			
			//printf("Get MCS:%d\n", new_wifi_info->be_mcs); //for Test

			if ((new_wifi_info->be_mcs > 0) && (new_wifi_info->be_mcs < 20)) {  //for abnormal overflow case
				mcs = new_wifi_info->be_mcs;
				if (mcs > 2) {
					mcs -= 2;
				} else {
					mcs = 1;
				}
				rspec = (new_wifi_info->rspec & ~WB_RSPEC_MCS_MASK) | mcs;
				set_fixrate_AC(cb_node->stream_priority,mcs,rspec,cb_node->freg_band);				
			}
		}

		if(cb_node->session_config.DECA_enabled == 1 && cb_node->deca_probe_failed < 2){

			//For debuging
			//printf("Tx Attempt:%f\n", new_wifi_info->tx_attempt_avg);
			//printf("RSSI:%d\n", new_wifi_info->rssi);

			if (cb_node->deca_probe_enabled) {
            	//printf("200ms loop for DECA checking, rts status:%d\n", get_rts_status(cb_node->freg_band));
				//printf("rts_TX_CNT:%d\n", new_wifi_info->rts_tx_cnt);
				//printf("rts_FAILED_CNT:%d\n", new_wifi_info->rts_failed_cnt);
				if (deca_probe_failed_check(new_wifi_info) == 1) {
					cb_node->deca_probe_failed += 1;
				} else {
					cb_node->deca_probe_failed = 0;
				}
			}
			

			if(cb_node->deca_probe_failed >= 2){
				deca_stop_probe(cb_node->freg_band);
				cb_node->deca_probe_enabled = 0;
			}
			else{
				cb_node->deca_probe_enabled = deca_policy_refresh(new_wifi_info, cb_node->deca_probe_enabled);
				if(cb_node->deca_probe_enabled){
					deca_start_probe(cb_node->freg_band);
				}
				else{
					deca_stop_probe(cb_node->freg_band);
					cb_node->deca_probe_failed = 0;
				}
			}
		}

		cb_node->sample_number += 1;
		if(cb_node->sample_number == 2){
			cb_node->session_status = SESSION_STATUS_ONGOING;	
			debug_print("change to session status ongoing\n");
		}

		if(cb_node->session_status == SESSION_STATUS_ONGOING){

	
			memset(message, 0, UDP_BUFSIZE);
			//add magic number integer in the head
			int magic_num_net = htonl(magic_num);
			memcpy(message, &magic_num_net, 4);
			mem_index += 4;
			mem_index = compose_tlv_string(REPORT_TAG_SESSION_ID,MAX_SESSION_ID_LENGTH,cb_node->session_id,message,mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_CONFIG,cb_node->session_config.BQW_enabled,message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_RSSI,new_wifi_info->rssi,message,mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_PHY_RATE,new_wifi_info->phy_rate,message, mem_index);
			//mem_index = compose_tlv_lint(REPORT_TAG_SESSION_FB_PHY_RATE,new_stainfo->fb_phy_rate,message, mem_index);
			//mem_index = compose_tlv_byte(REPORT_TAG_SESSION_ANTENNA_COUNT,new_stainfo->antenna_count,message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_STA_COUNT,new_wifi_info->sta_count,message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_IDLE_RATE,new_wifi_info->idle_rate,message, mem_index);
			mem_index = compose_tlv_float(REPORT_TAG_SESSION_TX_ATTEMPT_AVERAGE,new_wifi_info->tx_attempt_avg,message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_TX_FAIL_COUNT,new_wifi_info->tx_fail_cnt,message, mem_index);
			//mem_index = compose_tlv_lint(REPORT_TAG_SESSION_RX_DECRYPT_FAIL_COUNT,new_stainfo->rx_decrypt_failure_count,message, mem_index);
			mem_index = compose_tlv_float(REPORT_TAG_SESSION_TX_DATA_RATE,new_wifi_info->tx_data_rate,message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_BROKENSIGNAL_COUNT,new_wifi_info->broken_signal_cnt,message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_NOISE_LEVEL,new_wifi_info->noise_level,message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_SESSION_STATUS,cb_node->session_status,message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_CHANNEL_NUM, new_wifi_info->channel_num, message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_CHANNEL_BW, new_wifi_info->channel_bw, message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_FREQUENCY_BAND, cb_node->freg_band, message, mem_index);
			if(cb_node->last_rtt_ms == 0 && cb_node->received_udp_echo == 1){
				cb_node->last_rtt_ms = 201;      //201 means udp echo frame may be lost, or the echo frame delays over 200ms
			}
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_AP_PROXY_RTT,cb_node->last_rtt_ms,message, mem_index);
			cb_node->last_rtt_ms = 0;
		    //get current timestamp
			struct time_stamp current_time;
			get_current_timestamp(&current_time);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_AP_TIMESTAMP_SEC,current_time.sec,message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_AP_TIMESTAMP_USEC,current_time.usec,message, mem_index);

			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_TX_OVERALL_DATARATE,new_wifi_info->overall_datarate,message, mem_index);

			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_AIRTIME, new_wifi_info->air_time , message, mem_index);

			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_LOST_AC_PKTS, new_wifi_info->lost_ac_pkts , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_TOTAL_AC_PKTS, new_wifi_info->total_ac_pkts , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_AVG_MAC_DELAY, new_wifi_info->avg_mac_delay , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_MAX_MAC_DELAY , new_wifi_info->max_mac_delay , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_MAX_DELAY_DIST1_COUNT , new_wifi_info->mac_delay_dist1_count , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_MAX_DELAY_DIST2_COUNT , new_wifi_info->mac_delay_dist2_count , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_MAX_DELAY_DIST3_COUNT , new_wifi_info->mac_delay_dist3_count , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_MAX_DELAY_DIST4_COUNT , new_wifi_info->mac_delay_dist4_count , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_PKTQ_PKT_DROPPED , new_wifi_info->pktq_pkt_dropped , message, mem_index);
			mem_index = compose_tlv_lint(REPORT_TAG_SESSION_PKTQ_QUEUE_LENGTH , new_wifi_info->pktq_queue_length , message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_SESSION_COUNT , get_session_count() , message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_TXOP , new_wifi_info->txop , message, mem_index);
			mem_index = compose_tlv_float(REPORT_TAG_SESSION_NSS,new_wifi_info->nss,message, mem_index);
			mem_index = compose_tlv_byte(REPORT_TAG_SESSION_MCS , new_wifi_info->mcs , message, mem_index);
			if(get_hal_version() == HAL_VERSION_UGW6){
				mem_index = compose_tlv_lint(REPORT_TAG_SESSION_PSCNT , new_wifi_info->ps_cnt , message, mem_index);
				mem_index = compose_tlv_byte(REPORT_TAG_SESSION_DECA_CONFIG ,cb_node->session_config.DECA_enabled , message, mem_index);
				mem_index = compose_tlv_byte(REPORT_TAG_SESSION_DECA_PROBE_ENABLED ,cb_node->deca_probe_enabled , message, mem_index);
				mem_index = compose_tlv_byte(REPORT_TAG_SESSION_DECA_PROBE_FAILED , cb_node->deca_probe_failed , message, mem_index);
				mem_index = compose_tlv_lint(REPORT_TAG_SESSION_RX_UCAST, new_wifi_info->rx_ucast_pkts, message, mem_index);
				if (cb_node->deca_probe_enabled) {
					mem_index = compose_tlv_lint(REPORT_TAG_SESSION_RTS_TX_CNT,new_wifi_info->rts_tx_cnt, message, mem_index);
					mem_index = compose_tlv_lint(REPORT_TAG_SESSION_RTS_FAILED_CNT,new_wifi_info->rts_failed_cnt, message, mem_index);
				}
			}
			//mem_index = compose_tlv_lint(REPORT_TAG_SESSION_RX_UCAST, new_wifi_info->rx_ucast_pkts, message, mem_index);

			if(udp_send(cb_node->sock_udp_fd,&cb_node->addr, message, mem_index) == -1){
				debug_print("session id:%s udp send error\n",cb_node->session_id);
			}	

		}

		free(new_wifi_info);	
	
	}
	else{
		debug_print("timer_callback: session_node not found error\n");
	}

}



int compose_session_start_report(char* message, char* session_id){

	int mem_index = 0;
	//add magic number integer in the head
   	uint32_t magic_num_net = htonl(magic_num);
    memcpy(message, &magic_num_net, 4);
	mem_index += 4;
	mem_index = compose_tlv_string(REPORT_TAG_SESSION_ID,MAX_CHAR_SIZE,session_id,message,mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_SESSION_STATUS,SESSION_STATUS_START,message, mem_index);
	return mem_index;
	
}

int decompose_magic_num(char* message, uint32_t *magic_number) {
	if (message==NULL) {
		debug_print("message is null\n");
		return -1;
	}
	int mem_index = 0;
	uint32_t magic_num_net;
	memcpy(&magic_num_net,message, 4);
	//debug_print("origin magic_num:%x\n",magic_num_net);
	
	*magic_number = ntohl(magic_num_net);
	int debug_magic = *magic_number;
	//debug_print("magic number:%d\n",debug_magic);
	mem_index += 4;
	return mem_index;
}


int compose_session_stop_report(char* message, char* session_id, int stop_reason, int bqw_config, int deca_config, uint32_t mp_version_num, char* app_id, char* ap_mac_addr, char* ap_ip_addr){

	int mem_index = 0;
	//add magic number integer in the head
   	uint32_t magic_num_net = htonl(magic_num);
    memcpy(message, &magic_num_net, 4);
	mem_index += 4;
	mem_index = compose_tlv_string(REPORT_TAG_SESSION_ID, MAX_CHAR_SIZE, session_id, message, mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_CONFIG, bqw_config, message, mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_DECA_CONFIG, deca_config, message, mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_SESSION_STATUS,SESSION_STATUS_STOP,message, mem_index);
	mem_index = compose_tlv_lint(REPORT_TAG_SESSION_VERSION_NUM,mp_version_num,message, mem_index);
	mem_index =	compose_tlv_string(REPORT_TAG_SESSION_APP_ID,MAX_APP_ID_LENGTH,app_id,message,mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_STOP_REASON,stop_reason,message, mem_index);
	
	if(ap_mac_addr != NULL){
		mem_index = compose_tlv_string(REPORT_TAG_SESSION_AP_MAC_ADDR, MAX_ADDR_SIZE, ap_mac_addr, message, mem_index);
	}
	if(ap_ip_addr != NULL){
		mem_index = compose_tlv_string(REPORT_TAG_SESSION_AP_IP_ADDR, MAX_ADDR_SIZE, ap_ip_addr, message, mem_index);
	}
	return mem_index;
	
}

int compose_session_error_report(char* message, char* session_id, uint8_t error_code)
{

	int mem_index = 0;
	//add magic number integer in the head
   	uint32_t magic_num_net = htonl(magic_num);
    memcpy(message, &magic_num_net, 4);
	mem_index += 4;
	mem_index = compose_tlv_string(REPORT_TAG_SESSION_ID,MAX_CHAR_SIZE,session_id,message,mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_SESSION_STATUS,SESSION_STATUS_ERROR,message, mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_ERROR_CODE,error_code,message, mem_index);
	return mem_index;
	
}

int compose_session_error_detailed_report(char* message, char* session_id, uint8_t error_code, char* detailed_info)
{

	int mem_index = 0;
	//add magic number integer in the head
   	uint32_t magic_num_net = htonl(magic_num);
    memcpy(message, &magic_num_net, 4);
	mem_index += 4;
	mem_index = compose_tlv_string(REPORT_TAG_SESSION_ID,MAX_CHAR_SIZE,session_id,message,mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_SESSION_STATUS,SESSION_STATUS_ERROR,message, mem_index);
	mem_index = compose_tlv_byte(REPORT_TAG_SESSION_ERROR_CODE,error_code,message, mem_index);
	mem_index = compose_tlv_string(REPORT_TAG_SESSION_ERROR_DETAILED_INFO,MAX_INFO_SIZE,detailed_info,message,mem_index);
	return mem_index;
	
}

int compose_tlv_string(uint8_t tag, uint8_t length, char* value, char* message, int mem_index)
{

	memcpy(message + mem_index, &tag, 1);
	mem_index += 1;
	memcpy(message + mem_index, &length, 1);
	mem_index += 1;
	memcpy(message + mem_index, value, length);
	mem_index += length;

	return mem_index;
	
}


int compose_tlv_sint(uint8_t tag, int16_t value, char* message, int mem_index)
{

	memcpy(message + mem_index, &tag, 1);
	mem_index += 1;
	uint8_t length = 2;
	memcpy(message + mem_index, &length, 1);
	mem_index += 1;
	uint16_t value_ns = htons(value);
	memcpy(message + mem_index, &value_ns, length);
	mem_index += length;

	return mem_index;
}

int compose_tlv_lint(uint8_t tag, uint32_t value, char* message, int mem_index)
{
	if(value == NA_VALUE_INT){
		return mem_index;
	}
	memcpy(message + mem_index, &tag, 1);
	mem_index += 1;
	uint8_t length = 4;
	memcpy(message + mem_index, &length, 1);
	mem_index += 1;
	uint32_t value_ns = htonl(value);
	memcpy(message + mem_index, &value_ns, length);
	mem_index += length;

	return mem_index;
}


int compose_tlv_float(uint8_t tag, float value, char* message, int mem_index)
{
	if(value == NA_VALUE_FLOAT){
		return mem_index;
	}
	memcpy(message + mem_index, &tag, 1);
	mem_index += 1;
	uint8_t length = 4;
	memcpy(message + mem_index, &length, 1);
	mem_index += 1;
	char *byte_ptr = &value;	
	char byte3 = (char)*byte_ptr;
	char byte2 = (char)*(byte_ptr+1);
	char byte1 = (char)*(byte_ptr+2);
	char byte0 = (char)*(byte_ptr+3);
	memcpy(message + mem_index, &byte0, 1);
	mem_index += 1;
	memcpy(message + mem_index, &byte1, 1);
	mem_index += 1;
	memcpy(message + mem_index, &byte2, 1);
	mem_index += 1;	
	memcpy(message + mem_index, &byte3, 1);
	mem_index += 1;

	return mem_index;
}

int compose_tlv_byte(uint8_t tag, uint32_t value1, char* message, int mem_index)
{
    if(value1 == NA_VALUE_INT){
		return mem_index;
	}

	uint8_t value = value1;
	memcpy(message + mem_index, &tag, 1);
	mem_index += 1;
	uint8_t length = 1;
	memcpy(message + mem_index, &length, 1);
	mem_index += 1;
	memcpy(message + mem_index, &value, length);
	mem_index += length;

	return mem_index;
}

void send_session_error_report(session_node* err_node, uint8_t error_code)
{
	char message[UDP_BUFSIZE];
	int mem_index;
	memset(message, 0, UDP_BUFSIZE);
	mem_index = compose_session_error_report(message, err_node->session_id, error_code);	
	if(udp_send(err_node->sock_udp_fd,&err_node->addr, message, mem_index) == -1){
		debug_print("session id:%s udp send error\n",err_node->session_id);
	}	
}

#define UDP_BUF_LEN 2000

void print_buf_hex(int length,char *buf) {

	for (int i=0;i<length;i++) {
		debug_print("byte%d[%x] ",i,buf[i]);
	}

}

void handle_udp_echo(int index) 
{
	if (report_list[index].sock_udp_fd <= 0) {
		debug_print("Invalid sock_fd.\n");
		return;
	}
	char buf[UDP_BUF_LEN];  
	socklen_t len;
	int count;
	struct sockaddr_in report_addr;  //report_ip_addr
	memset(buf, 0, UDP_BUF_LEN);
	len = sizeof(report_addr);
	//debug_print("READY to RECV UDP PACKET\n");
	count = recvfrom(report_list[index].sock_udp_fd, buf, UDP_BUF_LEN, 0, (struct sockaddr*)&report_addr, &len);  //recvfrom is blocking function
	if(count == -1) {
		debug_print("recevied error");
		return;
	}

	//print_buf_hex(count,buf); //print buffer

	int mem_index = 0;
	uint32_t magic_number;
	mem_index = decompose_magic_num(buf,&magic_number);
	if (magic_number!=magic_num) {
		debug_print("magic_num error\n");
		return;
	}
	uint8_t tag;
	uint8_t length;
	char session_id[MAX_CHAR_SIZE];
	uint32_t timestamp_sec=0;
	uint32_t timestamp_usec=0;
	memcpy(&tag,buf + mem_index, 1);
	mem_index += 1;
	//debug_print("tag 1:%d\n",tag);
	if (tag!=REPORT_TAG_SESSION_ID) {
		debug_print("tag is not session id:%d\n",tag);
		return;
	} else {
		memcpy(&length, buf + mem_index, 1);
		mem_index += 1;
		memcpy(session_id ,buf + mem_index, length);
		session_id[length] = '\0';
		mem_index += length;
	}
	memcpy(&tag,buf + mem_index, 1);
	mem_index += 1;
    //debug_print("tag 2:%d\n",tag);
	if (tag!=REPORT_TAG_SESSION_AP_TIMESTAMP_SEC) {
		debug_print("tag is not REPORT_TAG_SESSION_AP_TIMESTAMP_SEC:%d\n",tag);
		return;
	} 

	memcpy(&length, buf + mem_index, 1);
	mem_index += 1;
	if (length!=4) {
		debug_print("REPORT_TAG_SESSION_AP_TIMESTAMP_SEC length error\n");
		return;
	} else {
		memcpy(&timestamp_sec, buf + mem_index, length);
		timestamp_sec = ntohl(timestamp_sec);
		mem_index += length;
	}

	memcpy(&tag,buf + mem_index, 1);
	mem_index += 1;

	if (tag!=REPORT_TAG_SESSION_AP_TIMESTAMP_USEC) {
		debug_print("tag is not REPORT_TAG_SESSION_AP_TIMESTAMP_USEC:%d\n",tag);
		return;
	} 

	memcpy(&length, buf + mem_index, 1);
	mem_index += 1;
	if (length!=4) {
		debug_print("REPORT_TAG_SESSION_AP_TIMESTAMP_USEC length error\n");
		return;
	} else {
		memcpy(&timestamp_usec, buf + mem_index, length);
		timestamp_usec = ntohl(timestamp_usec);
		mem_index += length;
	}
	
	mp_report_timestamp(session_id,timestamp_sec,timestamp_usec);

	//debug_print("handle UDP finished\n");
	
}