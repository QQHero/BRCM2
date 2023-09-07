#ifndef _TENCENTWIFI_H_
#define _TENCENTWIFI_H_
#include <stdint.h>
#include <netinet/in.h>
#include "cJSON.h"

struct ac_txq_params {
    //enum 80211_ac ac;
    int aifs;
    int cw_min;
    int cw_max;
    int txop;
};

struct time_stamp {
    uint32_t sec;
    uint32_t usec;
};

struct tencent_channel_info {
	uint8_t txop;     //change original idle_rate variable to txop
	int8_t noise_level;
	uint32_t brokensignal_count;
};
typedef struct tencent_channel_info ch_info;

struct bssap_info {
	uint8_t sta_count;
	int     overall_datarate;
	int     sta_status;   //only use in 6755 platform
	int     air_time;    //only use in 6755 platform
};
typedef struct bssap_info ap_info;

struct station_info {
	//int8_t  rssi;
	//uint32_t phy_rate;
	//uint32_t fb_phy_rate;  //fallback phy rate
	//uint8_t  antenna_count;
	//float    tx_data_rate;
	//float    tx_attempt_count;
	//uint32_t    tx_failure_count;
	//uint32_t    tx_retry_count;
	//uint32_t    tx_pkt_count;
	//uint32_t    rx_decrypt_failure_count;
	//char     chanspec[10];

	//uint8_t   channel_bw;
	//uint8_t   frequency_band;


	uint16_t   channel_num;
	uint32_t  ps_cnt;
	uint32_t  rts_tx_cnt;
	uint32_t  rts_failed_cnt;

};
typedef struct station_info sta_info;


struct delay_dist_info {

	uint32_t 	total_ac_pkts;
	uint32_t 	lost_ac_pkts; 
	uint32_t    avg_mac_delay;
	uint32_t    max_mac_delay;
	uint32_t    mac_delay_dist1_count;
	uint32_t    mac_delay_dist2_count;
	uint32_t    mac_delay_dist3_count;
	uint32_t    mac_delay_dist4_count;
	uint32_t    mac_delay_dist5_count;
	
};
typedef struct delay_dist_info delay_info;


#define NA_VALUE_INT 2244668800
#define NA_VALUE_FLOAT 2244668800.0

#define WB_RSPEC_MCS_MASK  0x0000000F

struct tencent_wifi_info {

	uint32_t 	rssi;
	uint32_t    phy_rate;
	uint32_t    sta_count;
	uint32_t    idle_rate;
	float       tx_attempt_avg;
	uint32_t   tx_fail_cnt;
	float       tx_data_rate;
	uint32_t    broken_signal_cnt;
	uint32_t    noise_level;
	uint32_t    channel_bw;     //	0-20MHz 1-40MHz 2-80MHz 3-160MHz
	uint32_t    freq_band;      //0-2.4G  1-5G
	uint32_t    overall_datarate;
	uint32_t    air_time;
	uint32_t    lost_ac_pkts;
	uint32_t    total_ac_pkts;
	uint32_t    avg_mac_delay;
	uint32_t    max_mac_delay;
	uint32_t    mac_delay_dist1_count;
	uint32_t    mac_delay_dist2_count;
	uint32_t    mac_delay_dist3_count;
	uint32_t    mac_delay_dist4_count;
	uint32_t    mac_delay_dist5_count;
	uint32_t    pktq_pkt_dropped;
	uint32_t    pktq_queue_length;	
	uint32_t    txop;
	float       nss;
	uint32_t    mcs;
	uint16_t   	channel_num;
	uint32_t  	ps_cnt;
	uint32_t  	rts_tx_cnt;
	uint32_t  	rts_failed_cnt;
	uint32_t    rx_ucast_pkts;
	uint32_t    rx_pkts_retried;
	int			rspec;
	int 		be_mcs;
};
typedef struct tencent_wifi_info wifi_info;



struct pktq_stats_info {

	uint32_t 	requested;
	uint32_t 	txfailed; 
	uint32_t    retried;
	uint32_t    utlsn;
	float       data_rate;
	uint32_t    phy_rate;
	uint8_t     bandwidth;
	uint8_t     air_use;
	float       nss_count;
	uint8_t     mcs_medium;

	
};
typedef struct pktq_stats_info pktq_info;

#define NA_VALUE_INT 2244668800
#define NA_VALUE_FLOAT 2244668800.0

struct aeb_enable_setting_info {

	char 	    mac_addr[6];
	uint32_t    idlerate_threshold1;
	uint32_t    idlerate_threshold2;
	int32_t     rssi_threshold1;
	int32_t     rssi_threshold2;
	uint32_t    pktqlen_threshold1;
	uint32_t    pktqlen_threshold2;
	uint32_t    timer_ms;
	uint8_t     freq_band;
};
typedef struct aeb_enable_setting_info aeb_setting;

struct session_linked_list;
typedef struct session_linked_list session_node;

typedef struct five_tuples 
{
	char src_addr[32];
	char dst_addr[32];
	char src_port[16];
	char dst_port[16];
	char protocol[8]; 
}five_tuples_t;

#define MAX_TUPLES 8

#define MAX_STRING_LENGTH 256
#define MAX_WLAN_CMDS 6
#define MAX_SESSION_ID_LENGTH 64
#define MAX_APP_ID_LENGTH 16
#define MAX_CHANSPEC_LENGTH 10

#define MAX_SESSION_LIFE_TIME 30000   //30 seconds

#define MAX_CONFIGS 1

#define UDP_BUFSIZE 512


//session_status
#define SESSION_STATUS_START 1
#define SESSION_STATUS_ONGOING 2
#define SESSION_STATUS_STOP 3
#define SESSION_STATUS_ERROR 4

//session stop reason
#define SESSION_STOPREASON_NORMAL 1
#define SESSION_STOPREASON_TIMER_EXPIRED 2
#define SESSION_STOPREASON_DEASSOC 3
#define SESSION_STOPREASON_SWITCH_BAND 4
#define SESSION_STOPREASON_DEASSOC_HIGH_RSSI 5
#define SESSION_STOPREASON_MULTIPLE_STREAM_IN_SP 6

//session error code
#define SESSION_NO_ERROR 0
#define SESSION_ERROR_ABNORMAL_CHANNEL_DATA 1
#define SESSION_ERROR_ABNORMAL_BSS_DATA 2
#define SESSION_ERROR_ABNORMAL_STA_DATA 3
#define SESSION_ERROR_APPLY_AC_QUEUE_CONFIG_FAILED 4
#define SESSION_ERROR_RESET_AC_QUEUE_CONFIG_FAILED 5
#define SESSION_ERROR_AC_QUEUE_FLOW_ABNORMAL 6
#define SESSION_ERROR_TIMER_START_FAILED 7
#define SESSION_ERROR_TIMER_STOP_FAILED 8
#define SESSION_ERROR_IPTOMACADDR_FAILED 9
#define SESSION_ERROR_STREAM_ID_ASSIGN_FAILED 10
#define SESSION_ERROR_STREAM_PRIORITY_ASSIGN_FAILED 11
#define SESSION_ERROR_BQW_ENABLE_STREAM_EXCEED_MAX 12
#define SESSION_ERROR_ABNORMAL_MACDELAY_DATA 13
#define SESSION_ERROR_ABNORMAL_PKTQSTATS_DATA 14
#define SESSION_ERROR_ABNORMAL_STREAM_PRIORITY 15
#define SESSION_ERROR_UNKNOWN 16


//proxy report TLV tag number
#define REPORT_TAG_SESSION_ID 1
#define REPORT_TAG_SESSION_CONFIG 2
#define REPORT_TAG_SESSION_RSSI 3
#define REPORT_TAG_SESSION_PHY_RATE 4
#define REPORT_TAG_SESSION_FB_PHY_RATE 5
#define REPORT_TAG_SESSION_ANTENNA_COUNT 6
#define REPORT_TAG_SESSION_STA_COUNT 7
#define REPORT_TAG_SESSION_IDLE_RATE 8
#define REPORT_TAG_SESSION_TX_ATTEMPT_AVERAGE 9
#define REPORT_TAG_SESSION_TX_FAIL_COUNT 10
#define REPORT_TAG_SESSION_RX_DECRYPT_FAIL_COUNT 11
#define REPORT_TAG_SESSION_TX_DATA_RATE 12
#define REPORT_TAG_SESSION_BROKENSIGNAL_COUNT 13
#define REPORT_TAG_SESSION_NOISE_LEVEL 14
#define REPORT_TAG_SESSION_SESSION_STATUS 15
#define REPORT_TAG_SESSION_VERSION_NUM 16
#define REPORT_TAG_SESSION_APP_ID 17
#define REPORT_TAG_SESSION_ERROR_CODE 18
#define REPORT_TAG_SESSION_STOP_REASON 19
#define REPORT_TAG_SESSION_CHANNEL_NUM 20
#define REPORT_TAG_SESSION_CHANNEL_BW 21
#define REPORT_TAG_SESSION_FREQUENCY_BAND 22
#define REPORT_TAG_SESSION_AP_MAC_ADDR 23
#define REPORT_TAG_SESSION_AP_IP_ADDR 24
#define REPORT_TAG_SESSION_AP_PROXY_RTT 25
#define REPORT_TAG_SESSION_AP_TIMESTAMP_SEC 26
#define REPORT_TAG_SESSION_AP_TIMESTAMP_USEC 27
#define REPORT_TAG_SESSION_TX_OVERALL_DATARATE 28
#define REPORT_TAG_SESSION_AIRTIME 29
#define REPORT_TAG_SESSION_LOST_AC_PKTS 30
#define REPORT_TAG_SESSION_TOTAL_AC_PKTS 31
#define REPORT_TAG_SESSION_AVG_MAC_DELAY 32
#define REPORT_TAG_SESSION_MAX_MAC_DELAY 33
#define REPORT_TAG_SESSION_MAX_DELAY_DIST1_COUNT 34
#define REPORT_TAG_SESSION_MAX_DELAY_DIST2_COUNT 35
#define REPORT_TAG_SESSION_MAX_DELAY_DIST3_COUNT 36
#define REPORT_TAG_SESSION_MAX_DELAY_DIST4_COUNT 37
#define REPORT_TAG_SESSION_MAX_DELAY_DIST5_COUNT 38
#define REPORT_TAG_SESSION_PKTQ_PKT_DROPPED 39
#define REPORT_TAG_SESSION_PKTQ_QUEUE_LENGTH 40
#define REPORT_TAG_SESSION_ERROR_DETAILED_INFO 41
#define REPORT_TAG_SESSION_SESSION_COUNT 42
#define REPORT_TAG_SESSION_TXOP 43
#define REPORT_TAG_SESSION_NSS 44
#define REPORT_TAG_SESSION_MCS 45
#define REPORT_TAG_SESSION_PSCNT 46
#define REPORT_TAG_SESSION_DECA_CONFIG 47
#define REPORT_TAG_SESSION_DECA_PROBE_ENABLED 48
#define REPORT_TAG_SESSION_DECA_PROBE_FAILED 49
#define REPORT_TAG_SESSION_RX_UCAST 50
#define REPORT_TAG_SESSION_RTS_TX_CNT 52
#define REPORT_TAG_SESSION_RTS_FAILED_CNT 53








#define FREQUENCY_BAND_2GHZ 0
#define FREQUENCY_BAND_5GHZ 1



#define CHANNEL_BW_20MHZ 0
#define CHANNEL_BW_40MHZ 1
#define CHANNEL_BW_80MHZ 2
#define CHANNEL_BW_160MHZ 3




#define RSSI_ERROR_VALUE -255
#define RSSI_CHECK_BAND_SWITCH -256
#define RSSI_CHECK_DEASSOCIATE -257



#define HAL_VERSION_AX3 	0
#define HAL_VERSION_AX12PRO 1
#define HAL_VERSION_UGW6 	2
 




struct JSON_CMD {
    char cmd_string[MAX_STRING_LENGTH];
    int (*handle_function)(int,cJSON *);
};

struct apconfig {
    int BQW_enabled;
	int DECA_enabled;
	int RS_enabled;
};

struct session_linked_list;
typedef struct session_linked_list session_node;

int add_ac_queue_tuple(int wlan_interface,five_tuples_t tuple,int priority);
int del_ac_queue_by_ip(int wlan_interface,int src_dst_ip,char* ip_address);
int ap_set_wme_ac_ip(char *client_ip, char *server_ip,int netlen,int srcport,int priority,int stream_id, int freq_band);

int set_wmm_traffic(int wlan_interface,char *server_ip_address,int net_mask_length,int port,int ac_queue_tos );
int clear_wmm_traffic();
int set_ampdu_length(int length);
int set_wmm_parameter(char* ac_queue_name,char* parameter_name,char* parameter_number) ;
int ip_to_mac_address(char* ip_addr, char* mac_address) ;
wifi_info* get_wifi_info(session_node* );
ch_info* get_channel_info();
ap_info* get_bssap_info();
sta_info* get_station_info(session_node* );
delay_info* get_macdelay_info(session_node*);
pktq_info* get_pktq_info(session_node* );
int get_connection_rssi(session_node* );
int get_pktq_stats(int ac_queue_id);
int disable_wlan0();
int get_sta_freq_band(session_node* node);

int turn_off_rts(int);
int turn_on_rts(int);
void init_hal();
void clear_hal_info();
int get_rts_status(int);


//aeb, platform dependent
int enable_aeb_event_report(aeb_setting*);


//deca, platform independent
int deca_policy_refresh(wifi_info*, int);
int deca_start_probe(int);
int deca_stop_probe(int);
void deca_init();
int deca_probe_failed_check(wifi_info*);
int hal_deca_supported();


int get_hal_version();

int set_fixrate_AC(int ac_number, int mcs, int rspec,int freq_band);
int set_fixrate_AC_off(int ac_number,int freq_band);



#endif


