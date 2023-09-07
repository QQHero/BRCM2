/*
**  Copyright (C), 1998-2014, Tenda Tech. Co., Ltd.
**
**  Project:	Cloud manager Api v1.0
**  File:    	
**  Author: 	lixiaolong
**  Date:    	05/14/2015
**
**  Purpose:
**    		.
**
**  History: 
**  <author>   <time>          <version >   <desc>
*	$Id: 
*/

#ifndef __UC_M_WIFI_TYPES_H__
#define __UC_M_WIFI_TYPES_H__
#include <stdint.h>

#define MAX_SSID_LENGTH				64
#define MAX_WIFI_PASSWD_LENGTH 		64 
#define MAX_WIFI_SEC_LENGTH 		64 
#define MAX_WIFI_BAND_LENGTH 		64 
#define MAX_WIFI_MODE_LENGTH 		64 
#define MAX_WIFI_PREFIX_LENGTH 		64
#define  BUFF_LEN   16

#define MAX_SEC_OPTION 		5
#define MAX_CHANNEL_2_OPTION 		25
#define MAX_CHANNEL_5_OPTION 		25
#define MAX_CHANNEL_6_OPTION 		64
#define MAX_BAND_OPTION 		5
#define MAX_MODE_OPTION 		5
#define MAX_INT_OPTION		10

#define MAX_WPS				10
#define MAX_SN_LENGTH				64
#define MAX_NAME_LENGTH				64
#define MAX_PIN_LENGTH				64
#define TERMINAL_NUM		150

//common mask set and has
#define SET_WIFI_X(wifi, x)	\
	((wifi)->mask |= (1 << x))
#define HAS_WIFI_X(wifi, x)	\
	(((wifi)->mask & (1 << x)) == (1 << x))
/*
** wifi struct
*/
typedef enum {
	WIFI_2G = 0,
	WIFI_5G,
	WIFI_6G,
	WIFI_MAX,
}wifi_type_t;

enum {
	WIFI_BASIC_DETAIL_SEC = 0,
	WIFI_BASIC_DETAIL_ENABLE,
	WIFI_BASIC_DETAIL_PASSWD,
	WIFI_BASIC_DETAIL_SSID_HIDE,
	WIFI_BASIC_DETAIL_CHANNEL,
	WIFI_BASIC_DETAIL_BANDWIDTH,
	WIFI_BASIC_DETAIL_MODE,
	WIFI_BASIC_DETAIL_SSID_PREFIX,
	WIFI_BASIC_DETAIL_CUR_CHANNEL,
};

#define SET_WIFI_BASIC_2G(wifi_basic) \
	SET_WIFI_X(wifi_basic, WIFI_2G)
#define SET_WIFI_BASIC_5G(wifi_basic) \
	SET_WIFI_X(wifi_basic, WIFI_5G)
#define SET_WIFI_BASIC_6G(wifi_basic) \
	SET_WIFI_X(wifi_basic, WIFI_6G)
#define HAS_WIFI_BASIC_2G(wifi_basic) \
	HAS_WIFI_X(wifi_basic, WIFI_2G)
#define HAS_WIFI_BASIC_5G(wifi_basic)  \
	HAS_WIFI_X(wifi_basic, WIFI_5G)
#define HAS_WIFI_BASIC_6G(wifi_basic) \
	HAS_WIFI_X(wifi_basic, WIFI_6G)

#define SET_WIFI_BASIC_DETAIL_SEC(wifi_basic_detail) \
	SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_SEC)
#define SET_WIFI_BASIC_DETAIL_PASSWD(wifi_basic_detail) \
	SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_PASSWD)
#define SET_WIFI_BASIC_DETAIL_SSID_HIDE(wifi_basic_detail) \
	SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_SSID_HIDE)	
#define SET_WIFI_BASIC_DETAIL_ENABLE(wifi_basic_detail) \
	SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_ENABLE)	
#define SET_WIFI_BASIC_DETAIL_CHANNEL(wifi_basic_detail) \
		SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_CHANNEL) 
#define SET_WIFI_BASIC_DETAIL_BANDWIDTH(wifi_basic_detail) \
		SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_BANDWIDTH) 
#define SET_WIFI_BASIC_DETAIL_MODE(wifi_basic_detail) \
		SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_MODE) 
#define SET_WIFI_BASIC_DETAIL_SSID_PREFIX(wifi_basic_detail) \
		SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_SSID_PREFIX) 
#define SET_WIFI_BASIC_DETAIL_CUR_CHANNEL(wifi_basic_detail) \
			SET_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_CUR_CHANNEL)

	
#define HAS_WIFI_BASIC_DETAIL_SEC(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_SEC)
#define HAS_WIFI_BASIC_DETAIL_PASSWD(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_PASSWD)
#define HAS_WIFI_BASIC_DETAIL_SSID_HIDE(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_SSID_HIDE)
#define HAS_WIFI_BASIC_DETAIL_ENABLE(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_ENABLE)
#define HAS_WIFI_BASIC_DETAIL_CHANNEL(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_CHANNEL)
#define HAS_WIFI_BASIC_DETAIL_BANDWIDTH(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_BANDWIDTH)
#define HAS_WIFI_BASIC_DETAIL_MODE(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_MODE)
#define HAS_WIFI_BASIC_DETAIL_SSID_PREFIX(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_SSID_PREFIX)
#define HAS_WIFI_BASIC_DETAIL_CUR_CHANNEL(wifi_basic_detail) \
	HAS_WIFI_X(wifi_basic_detail, WIFI_BASIC_DETAIL_CUR_CHANNEL)

typedef struct wifi_detail_s {
	int 	mask;
	wifi_type_t 	type;
	int enable;
	char	ssid[MAX_SSID_LENGTH];
	char	passwd[MAX_WIFI_PASSWD_LENGTH];
	int		ssid_hide;
	char sec[MAX_WIFI_SEC_LENGTH];
	char ssid_prefix[MAX_WIFI_PREFIX_LENGTH];
	int channel;
	char bandwidth[MAX_WIFI_BAND_LENGTH];
	char mode[MAX_WIFI_MODE_LENGTH];
	int cur_channel;
} wifi_detail_t;

//wifi basic info
typedef struct channel_5_s{
	uint32_t 	 n_channel_choice_5_option;
	int		channel_5[MAX_CHANNEL_5_OPTION];
}channel_5_t;

typedef struct channel_6_s{
	uint32_t 	 n_channel_choice_6_option;
	int		channel_6[MAX_CHANNEL_6_OPTION];
}channel_6_t;

enum {
	WIFI_ENABLE = 0,
	WIFI_DOUBLE_BAND,
	WIFI_THREE_BAND,
};

#define SET_WIFI_ENABLE(wifi_basic) \
	SET_WIFI_X(wifi_basic, WIFI_ENABLE)
#define SET_WIFI_DOUBLE_BAND(wifi_basic) \
	SET_WIFI_X(wifi_basic, WIFI_DOUBLE_BAND)
#define SET_WIFI_THREE_BAND(wifi_basic) \
	SET_WIFI_X(wifi_basic, WIFI_THREE_BAND)
#define HAS_WIFI_ENABLE(wifi_basic) \
	HAS_WIFI_X(wifi_basic, WIFI_ENABLE)
#define HAS_WIFI_DOUBLE_BAND(wifi_basic)  \
	HAS_WIFI_X(wifi_basic, WIFI_DOUBLE_BAND)
#define HAS_WIFI_THREE_BAND(wifi_basic)  \
	HAS_WIFI_X(wifi_basic, WIFI_THREE_BAND)

typedef struct wifi_basic_s {
	int 			mask;
	int 			n_wifi_detail;
	wifi_detail_t	wifi_detail[WIFI_MAX];
	uint32_t 	 n_sec_option;
	char		 sec_option[MAX_SEC_OPTION][MAX_WIFI_SEC_LENGTH];
	uint32_t 	 n_channel_choice_2;
	int		 channel_choice_2[MAX_CHANNEL_2_OPTION];
	uint32_t 	 n_channel_choice_5_band;
	channel_5_t		 channel_choice_5[MAX_BAND_OPTION];
	uint32_t 	 n_bandwidth_option_2;
	char		 bandwidth_option_2[MAX_BAND_OPTION][MAX_WIFI_BAND_LENGTH];
	uint32_t 	 n_bandwidth_option_5;
	char		 bandwidth_option_5[MAX_BAND_OPTION][MAX_WIFI_BAND_LENGTH];
	uint32_t 	 n_mode_option_2;
	char		 mode_option_2[MAX_MODE_OPTION][MAX_WIFI_MODE_LENGTH];
	uint32_t 	 n_mode_option_5;
	char		 mode_option_5[MAX_MODE_OPTION][MAX_WIFI_MODE_LENGTH];
	uint32_t 	 n_channel_choice_6;
	channel_6_t		 channel_choice_6[MAX_BAND_OPTION];
	uint32_t 	 n_bandwidth_option_6;
	char		 bandwidth_option_6[MAX_BAND_OPTION][MAX_WIFI_BAND_LENGTH];
	uint32_t 	 n_mode_option_6;
	char		 mode_option_6[MAX_MODE_OPTION][MAX_WIFI_MODE_LENGTH];
	int double_band_blend;
	int three_band_blend;
	uint32_t 	 n_sec_6_option;
	char		 sec_6_option[MAX_SEC_OPTION][MAX_WIFI_SEC_LENGTH];
} wifi_basic_t;


/*
 *WIFI GUEST INFO
 */
enum {
	WIFI_GUEST_DETAIL_SEC = 0,
	WIFI_GUEST_DETAIL_PWD
};

enum{
	WIFI_GUEST_TIMEOUT = WIFI_MAX,
	WIFI_GUEST_RATE,
	WIFI_GUEST_RATE_UPLIMIT,
};

#define SET_WIFI_GUEST_2G(guest_info) \
	SET_WIFI_X(guest_info, WIFI_2G)
#define SET_WIFI_GUEST_5G(guest_info) \
	SET_WIFI_X(guest_info, WIFI_5G)
#define SET_WIFI_GUEST_6G(guest_info) \
	SET_WIFI_X(guest_info, WIFI_6G)
#define HAS_WIFI_GUEST_2G(guest_info) \
	HAS_WIFI_X(guest_info, WIFI_2G)
#define HAS_WIFI_GUEST_5G(guest_info)  \
	HAS_WIFI_X(guest_info, WIFI_5G)
#define HAS_WIFI_GUEST_6G(guest_info)  \
	HAS_WIFI_X(guest_info, WIFI_6G)

#define HAS_WIFI_GUEST_TIMEOUT(guest_info) \
	HAS_WIFI_X(guest_info, WIFI_GUEST_TIMEOUT)

#define HAS_WIFI_GUEST_RATE(guest_info) \
        HAS_WIFI_X(guest_info, WIFI_GUEST_RATE)
		
#define HAS_WIFI_GUEST_RATE_UPLIMIT(guest_info) \
		HAS_WIFI_X(guest_info, WIFI_GUEST_RATE_UPLIMIT)

#define SET_WIFI_GUEST_TIMEOUT(guest_info) \
	SET_WIFI_X(guest_info, WIFI_GUEST_TIMEOUT)

#define SET_WIFI_GUEST_RATE(guest_info) \
    	SET_WIFI_X(guest_info, WIFI_GUEST_RATE)
	
#define SET_WIFI_GUEST_RATE_UPLIMIT(guest_info) \
		SET_WIFI_X(guest_info, WIFI_GUEST_RATE_UPLIMIT)


#define SET_WIFI_GUEST_DETAIL_SEC(guest_detail) \
	SET_WIFI_X(guest_detail, WIFI_GUEST_DETAIL_SEC)
#define SET_WIFI_GUEST_DETAIL_PWD(guest_detail) \
	SET_WIFI_X(guest_detail, WIFI_GUEST_DETAIL_PWD)
#define HAS_WIFI_GUEST_DETAIL_SEC(guest_detail) \
	HAS_WIFI_X(guest_detail, WIFI_GUEST_DETAIL_SEC)
#define HAS_WIFI_GUEST_DETAIL_PWD(guest_detail) \
	HAS_WIFI_X(guest_detail, WIFI_GUEST_DETAIL_PWD)

typedef struct guest_detail_s {
	int 	mask;
	wifi_type_t type;
	int 	guest_enable;
	char	guest_ssid[MAX_SSID_LENGTH];	
	char	guest_passwd[MAX_WIFI_PASSWD_LENGTH];
	char 	sec[MAX_WIFI_SEC_LENGTH];
}guest_detail_t;


typedef struct guest_info_s {
	int    	mask;
	guest_detail_t guest_detail[WIFI_MAX];
	uint32_t timeout;
	uint32_t rate;
	uint32_t rate_uplimit;
	uint32_t n_timeout_option;
	int32_t	 timeout_option[MAX_INT_OPTION]; 
	uint32_t n_rate_option;
	int32_t  rate_option[MAX_INT_OPTION];
} guest_info_t;

/*
 *WIFI CHANNEL
 */
 enum {
	WIFI_CHANNEL_5G = 0,
};

#define SET_WIFI_CHANNEL_5G(wifi_channel) \
	SET_WIFI_X(wifi_channel, WIFI_CHANNEL_5G)
#define HAS_WIFI_CHANNEL_5G(wifi_channel) \
	HAS_WIFI_X(wifi_channel, WIFI_CHANNEL_5G)
	
typedef struct wifi_channel_info_s {
	int mask;
	int 	chan_2g_sta;
	int 	chan_5g_sta;
} wifi_channel_info_t;

/*
 * WIFI POWER
 */
  enum {
	WIFI_POWER_5G = 0,
};

#define SET_WIFI_POWER_5G(wifi_power) \
	SET_WIFI_X(wifi_power, WIFI_POWER_5G)
#define HAS_WIFI_POWER_5G(wifi_power) \
	HAS_WIFI_X(wifi_power, WIFI_POWER_5G)	
	
typedef struct wifi_power_s {
	int 	mask;
	int		wifi_2g_power;
	int		wifi_5g_power;
} wifi_power_t;

//wps cfg
enum{
	WIFI_WPS_STATUS = 0,
};

#define SET_WIFI_WPS_STATUS(guest_info) \
	SET_WIFI_X(guest_info, WIFI_WPS_STATUS)
#define HAS_WIFI_WPS_STATUS(guest_info)  \
	HAS_WIFI_X(guest_info, WIFI_WPS_STATUS)

typedef struct node_wps_info_s{
	int mask;
	int enable;
	char sn[MAX_SN_LENGTH];
	char pin[MAX_PIN_LENGTH];
	char name[MAX_NAME_LENGTH];
	int status;
}node_wps_info_t;

typedef struct wps_cfg_s{
	int n_wps;
	node_wps_info_t wps[MAX_WPS];
}wps_cfg_t;


/*
*WIFI COMMON ACK
*/
enum {
	_WIFI_BASIC_ = 0,
	_WIFI_GUEST_,
	_WIFI_CHANNEL_,
	_WIFI_POWER_,
	_WIFI_WPS_,
};

#define SET_ACK_WIFI_BASIC(ack) \
	SET_WIFI_X(ack, _WIFI_BASIC_)
#define SET_ACK_WIFI_GUEST(ack) \
	SET_WIFI_X(ack, _WIFI_GUEST_)
#define SET_ACK_WIFI_CHANNEL(ack) \
	SET_WIFI_X(ack, _WIFI_CHANNEL_)
#define SET_ACK_WIFI_POWER(ack) \
	SET_WIFI_X(ack, _WIFI_POWER_)
#define SET_ACK_WIFI_WPS(ack) \
		SET_WIFI_X(ack, _WIFI_WPS_)
#define HAS_ACK_WIFI_BASIC(ack) \
	HAS_WIFI_X(ack, _WIFI_BASIC_)
#define HAS_ACK_WIFI_GUEST(ack) \
	HAS_WIFI_X(ack, _WIFI_GUEST_)
#define HAS_ACK_WIFI_CHANNEL(ack) \
	HAS_WIFI_X(ack, _WIFI_CHANNEL_)
#define HAS_ACK_WIFI_POWER(ack) \
	HAS_WIFI_X(ack, _WIFI_POWER_)
#define HAS_ACK_WIFI_WPS(ack) \
	HAS_WIFI_X(ack, _WIFI_WPS_)
	
typedef struct wifi_common_ack_s {
	int mask;
	int err_code;		//cmd execute error, defines by register process
	wifi_basic_t	basic;
	guest_info_t	guest;
	wifi_channel_info_t channel;
	wifi_power_t	power;
	wps_cfg_t		wps;
}wifi_common_ack_t;

typedef struct wifi_alexa_guest_s {
	int enable;
	char guest_ssid[64];
	char ssid[64];
}wifi_alexa_guest_t;

typedef struct terminal_detail_s {
	int net_access_value;
	char net_access_id[64];
	char device_name[64];
	char host_name[64];
	char mac_address[64];
}terminal_detail_t;


typedef struct wifi_alexa_terminal_list_s {
	int n_terminal;
	terminal_detail_t terminal[TERMINAL_NUM];
}wifi_alexa_terminal_list_t;


struct wifi_alexa_terminal_net_s {
	int enable;
	int duration;
	char net_access_id[64];
};


typedef struct wifi_alexa_comman_switch_s {
	int enable;
}wifi_alexa_comman_switch_t;

#endif

