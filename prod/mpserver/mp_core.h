/* @author: Tencent slimehsiao */
#ifndef _MPCORE_H_
#define _MPCORE_H_

#define REPORT_LIST_SIZE 10

struct report_ip_node
{
	char ip_addr[32];
	int  ref_count;
	int  sock_udp_fd;
	struct sockaddr_in addr; 
};



struct report_ip_node report_list[REPORT_LIST_SIZE];

struct apconfig;
typedef struct session_linked_list session_node;
typedef struct five_tuples five_tuples_t;

void init_mp_core();
int mp_start_session(char* session_id, char* sta_ip_addr, char* proxy_ip_addr, int proxy_port, char* report_ip_addr, int report_port, char* app_id, int timer_ms, uint32_t version_num);
void mp_session_unit_test(void);
//int mp_apply_config(char* session_id, struct apconfig config);
int mp_apply_config(char* session_id, struct apconfig config, char* result);
int mp_stop_session(char* session_id, int reason);
int mp_report_heartbeat(char* session_id);
int mp_report_timestamp(char* session_id, uint32_t time_ms, uint32_t time_us);
int check_udp_fd(int fd);
//void timer_callback(int signo, siginfo_t *si, void *uc);
void gather_ap_info();
int assign_stream_priority(int freq_band);
int release_stream_priority(int priority, int freq_band);
void init_proxy_list();
void init_report_list();



int compose_tlv_string(uint8_t tag, uint8_t length, char* value, char* message, int mem_index);
int compose_tlv_sint(uint8_t tag, int16_t value, char* message, int mem_index);
int compose_tlv_lint(uint8_t tag, uint32_t value, char* message, int mem_index);
int compose_tlv_byte(uint8_t tag, uint32_t value, char* message, int mem_index);
int compose_tlv_float(uint8_t tag, float value, char* message, int mem_index);
int compose_session_stop_report(char* message, char* session_id, int stop_reason, int bqw_config, int deca_config, uint32_t mp_version_num, char* app_id, char* ap_mac_addr, char* ap_ip_addr);
int compose_session_error_report(char* message, char* session_id, uint8_t error_code);
int compose_session_error_detailed_report(char* message, char* session_id, uint8_t error_code, char* detailed_info);
int compose_session_start_report(char* message, char* session_id);
void send_session_error_report(session_node* err_node, uint8_t error_code);

int decompose_magic_num(char* message, uint32_t *magic_number);

void handle_udp_echo(int index);

int mp_start_session_without_report(char* session_id, char* sta_ip_addr, char* app_id, int timer_ms, uint32_t version_num);
int mp_set_tuples(char* session_id, five_tuples_t* tuples);
five_tuples_t * mp_get_tuples(char* session_id);





#endif
