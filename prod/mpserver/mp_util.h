/* @author: Tencent slimehsiao */
#ifndef _MPUTILS_H_
#define _MPUTILS_H_


#include<netinet/in.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include "TencentWiFi.h"


#define MAX_CHAR_SIZE 64
#define MAX_ADDR_SIZE 32
#define MAX_INFO_SIZE 128
//#define MAX_APP_ID_LENGTH 16

struct session_linked_list
{
	char session_id[MAX_CHAR_SIZE];            //master key
	char sta_ip_addr[MAX_CHAR_SIZE];
	char sta_mac_addr[MAX_CHAR_SIZE];
	char proxy_ip_addr[MAX_CHAR_SIZE];          //ip of game server
	int proxy_port;                             //port of game server
	char report_ip_addr[MAX_CHAR_SIZE];          //ip of report server
	int report_port;                             //port of report server
	char app_id[MAX_APP_ID_LENGTH];
	int timer_ms;
	timer_t timer_id;	
	int sock_udp_fd;
	struct sockaddr_in addr;
    struct session_linked_list *next;
	uint64_t tx_total_bytes;
	uint32_t tx_total_pkts;
	uint32_t tx_failure_pkts;
	uint32_t tx_retry_pkts;
	uint32_t rx_total_pkts;
	uint32_t rx_decrypt_fail_pkts;
	struct apconfig session_config;
	uint32_t mp_version_num;
	uint8_t session_status;
	uint32_t life_time;
	uint8_t	sample_number;
	int 	freg_band;
	int 	stream_id;   							// between 0-7
	int     stream_priority;
	int     proxy_ip_index;  
	int     report_ip_index; 
	int     last_rtt_ms;       
	int     received_udp_echo;  
	int     last_rssi;   
	int 	deca_probe_enabled;   //deca probe is enabled
	int 	deca_probe_failed;    //deca probe ever failed for this session
	int 	report;               //0:don't report 1:report
	five_tuples_t tuples[MAX_TUPLES];              
};
typedef struct session_linked_list session_node;


//session_node* create_session_node(char* session_id, char* sta_ip_addr, char* proxy_ip_addr, int proxy_port, char* app_id, int timer_ms, uint32_t version_num);
session_node* create_session_node(char* session_id, char* sta_ip_addr, char* proxy_ip_addr, int proxy_port, char* report_ip_addr, int report_port, char* app_id, int timer_ms, uint32_t version_num);
int insert_session_node(session_node* new_node);
int delete_session_node(char* session_id);
session_node* get_session_node(char* session_id);
void print_all_sessions();
session_node*  get_first_session_node();
const char* get_localtime_str();
int start_timer(char* session_id, timer_t* timerid, int interval_ms, void(*handle_function)(int signo, siginfo_t *si, void *uc));
int stop_timer(timer_t timerid);
uint64_t string_to_uint64(const char *nptr, char **endptr, int base);
uintmax_t string_to_umax(const char *nptr, char **endptr, int base);
int assign_stream_id();
int release_stream_id(int id);
int is_stream_id_avaliable();

uint64_t timestamp_in_usecs(struct time_stamp *time);
double timestamp_in_secs(struct time_stamp *time);
double timestamp_in_msecs(struct time_stamp *time);
int timestamp_compare(struct time_stamp *time1, struct time_stamp *time2);
int timestamp_diff(struct time_stamp *time1, struct time_stamp *time2, struct time_stamp *diff);
int get_current_timestamp(struct time_stamp *time1);

void string_lower_to_upper(char string[]);

int get_session_count();





#define debug_print(f_, ...) printf("[%s]", get_localtime_str()), printf((f_), ##__VA_ARGS__)

#endif
