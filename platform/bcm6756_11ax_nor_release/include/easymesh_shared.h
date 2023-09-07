#ifndef __TD_EASYMESH_SHARED_H__
#define __TD_EASYMESH_SHARED_H__

#include "easymesh_nl_shared.h"

/*对外数据。不需要对外的数据不要放到该文件*/
/*该文件定义的数据均来自MultiAp协议，不要随意更改*/
#define TD_EM_AUTH_WPA2PSKMIXED  (TD_EM_AUTH_WPAPSK|TD_EM_AUTH_WPA2PSK)
#define TD_EM_AUTH_WPA2MIXED     (TD_EM_AUTH_WPA|TD_EM_AUTH_WPA2)
#define TD_EM_AUTH_WPA3_WPA2_PSK (TD_EM_AUTH_WPA3|TD_EM_AUTH_WPA2PSK)

typedef enum td_em_device_type /*add by qinke *//*one_device_info_tlv_t*/
{
    TD_EM_DEVICE_TYPE_2G_11B      = 0x100,
    TD_EM_DEVICE_TYPE_2G_11G      = 0x101,
    TD_EM_DEVICE_TYPE_5G_11A      = 0x102,
    TD_EM_DEVICE_TYPE_2G_11N      = 0x103,
    TD_EM_DEVICE_TYPE_5G_11N      = 0x104,
    TD_EM_DEVICE_TYPE_5G_11AC     = 0x105,
    TD_EM_DEVICE_TYPE_DEFAULT     = 0xffff
}td_em_device_type_e;

typedef enum td_em_workmode /*add by qinke *//*one_device_info_tlv_t*/
{
    TD_EM_BSS_WORKMODE_AP               = 0x0,
    TD_EM_BSS_WORKMODE_STATION          = 0x4,
    TD_EM_BSS_WORKMODE_DEFAULT          = 0xf
}td_em_workmode_e;
    
typedef enum td_assoc_control  /*add by liyahui *//*client_assoc_req_t*/
{
    TD_EM_ASSOC_BLOCK           = 0X00, // block
    TD_EM_ASSOC_UNBLOCK         = 0X01, // unblock
    TD_EM_ASSOC_ERROR_MAX
}td_assoc_control_e;

typedef enum td_em_bw /*add by qinke *//*one_device_info_tlv_t*/
{
    TD_EM_CHWIDTH20     = 0,
    TD_EM_CHWIDTH40     = 1,
    TD_EM_CHWIDTH80     = 2,
    TD_EM_CHWIDTH160    = 3,
    TD_EM_CHWIDTH80_80  = 4,
    TD_EM_CHWIDTH_ERROR = -1,
}td_em_bw_e;

typedef enum bh_steer_error_code  /*add by liyahui *//*backhaul_steering_response_t*/
{
    BH_STEER_SUCCESS                  = 0X00, // SUCCESS
    BH_STA_CANOT_OPERATE_CHNL         = 0X04, //Backhaul steering request rejected because the backhaul STA cannot operate on the channel specified
    BH_TARGET_BSS_SIGNAL_TOO_WEAK     = 0X05, //Backhaul steering request rejected because the target BSS signal is tooweak or not found.
    BH_AUTH_OR_ASSOC_FAIL             = 0X06, //Backhaul steering request authentication or association Rejected by the target BSS. 
    BH_ERROR_MAX
}bh_steer_error_code_e;

typedef enum backhaul_steer_result /*add by liyahui *//*backhaul_steering_response_t*/
{
    BACKHAUL_STEER_SUCCESS        = 0, //SUCCESS
    BACKHAUL_STEER_FAILED         = 1, //FAILED.
    BACKHAUL_STEER_MAX
}backhaul_steer_result_e;

typedef enum cac_complete_action /*add by liyahui *//*cac_request_radio_t*/
{
    CAC_ACTION_RAMAIN_ON_CHANNEL    = 0, //Remain on channel and continue to monitor for radar
    CAC_ACTION_RETURN_RADIO         = 1, //Return the radio that was performing the CAC toits most recent operational configuration.
    CAC_ACTION_MAX
}cac_complete_action_e;

typedef enum cac_method /*add by liyahui *//*cac_request_radio_t*/
{
    CAC_METHOD_CONTINUOUS_CAC               = 0, //Continuous CAC
    CAC_METHOD_CONTINUOUS_DEDICATE_RADIO    = 1, //Continuous with dedicated radio
    CAC_METHOD_MIMO_DIMENSION_REDUCE        = 2, //MIMO dimension reduced
    CAC_METHOD_TIME_SLICED_CAC              = 3, //Time sliced CAC
    CAC_METHOD__MAX
}cac_method_e;

typedef enum cac_complete_status /*add by liyahui *//*cac_completion_report_radio_t*/
{
    CAC_COMPLETE_SUCCESS                    = 0, //0x00: Successful
    CAC_COMPLETE_RADAR_DETECT               = 1, //0x01: Radar detected
    CAC_COMPLETE_NOT_SUPPORT                = 2, //0x02: CAC not supported as requested (capability mismatch)
    CAC_COMPLETE_NONCONFORMANT_REGULATION   = 3, //0x03: Request was considered to be nonconformant to regulations in the country in which the MultiAP Agent is operating
    CAC_COMPLETE_OTHER_ERROR                = 4, //0x04: Other error
    CAC_COMPLETE__MAX
}cac_complete_status_e;
    
typedef enum chnl_select_rep_code /*add by liyahui *//*channel_select_response_t*/
{
    CHNL_SELECT_ACCEPT                      = 0,
    CHNL_SELECT_DECLINE_PREFER_CHANGE       = 1, //0x01: Decline because request violates current preferences which have changed since last reported
    CHNL_SELECT_DECLINE_REPORT_PREFERENCE   = 2, //0x02: Decline because request violates most recently reported
    CHNL_SELECT_DECLINE_CURRENT_BACKHAUL    = 3, //0x03: Decline because request would prevent operation of a currently operating backhaul link (where backhaul STA and BSS share a radio)
    CHNL_SELECT_MAX
}chnl_select_rep_code_e;

typedef enum chnl_prefer_reason /*add by liyahui *//*channel_prefer_t*/
{
    CHNL_PREFER_UNSPECIFIED                  = 0,
    CHNL_PREFER_INTERFER_LOCAL_ENVIROMENT    = 1,  //Proximate non-802.11 interferer in local environment
    CHNL_PREFER_INTRA_NET_OBSS_MANAGEMENT    = 2,  //Intra-network 802.11 OBSS interference management
    CHNL_PREFER_EXTERNAL_NET_OBSS_MANAGEMENT = 3,  //External network 802.11 OBSS interference management
    CHNL_PREFER_REDUCED_COVERAGE             = 4,  //Reduced coverage
    CHNL_PREFER_REDUCED_THROUGHPUT           = 5,  //Reduced throughput
    CHNL_PREFER_IN_DEVICE_INTERFER_WITHIN_AP = 6,  //In-device Interferer within AP
    CHNL_PREFER_OPERATE_DISALLOWED           = 7,  //Operation disallowed due to radar detection on a DFS channel
    CHNL_PREFER_USE_SHARE_RADIO              = 8,  //Operation would prevent backhaul operation using shared radio (can only be specified by the Multi-AP Agent)
    CHNL_PREFER_CAC_RUN_AND_VALID            = 9,  //Immediate operation possible on a DFS channel – CAC has been run and is still valid and channel has been cleared for use (can only be specified by the Multi-AP Agent)
    CHNL_PREFER_DFS_STATS_UNKNOW             = 10, //DFS channel state unknown (CAC has not run or its validity period has expired) (can only be specified by the Multi-AP Agent)
    CHNL_PREFER_DFS_CLEAR_INDICATION         = 11, //Controller DFS Channel Clear Indication (Can only be specified by the Multi-AP Controller)
    CHNL_PREFER_MAX
} chnl_prefer_reason_e;

typedef enum td_em_control_state /*add by qinke *//*one_device_info_tlv_t*/
{
    TD_EM_CTROL_DISCONNECT  = 0,
    TD_EM_CTROL_CONNECT     = 1,
} td_em_control_state_e;

typedef struct assoc_sta_info
{
    unsigned char sta_num;
    unsigned char sta_mac[TD_EM_MACADDRLEN];
    unsigned char associated_sta_bssid[TD_EM_MACADDRLEN];
}assoc_sta_info_t;

typedef struct association_client_sta_info/*add by qinke *//*association_client_tlv*/
{
    unsigned short link_time;
    unsigned char sta_macaddr[TD_EM_MACADDRLEN];
}association_client_sta_info_t;

typedef struct association_client_tlv
{
    unsigned int sta_num;
    unsigned char bss_macaddr[TD_EM_MACADDRLEN];
    association_client_sta_info_t info[];
}association_client_tlv_t;

typedef struct ap_operational_bss_tlv
{
    unsigned char bss_macaddr[TD_EM_MACADDRLEN];
    unsigned char bss_ssid[TD_EM_WLAN_SSID_MAXLEN + 1];
    int ssid_len;
}ap_operational_bss_tlv_t;

typedef struct td_em_map_unoperable_ch/*add by qinke */
{
    unsigned char opclass;
    unsigned char txpow;
    unsigned char num_unopch;
    unsigned char  band_width;
    unsigned char unopch[TD_EM_MAX_CHANNELS_PER_OPERATING_CLASS];
    unsigned char num_opch;
    unsigned char opch[TD_EM_MAX_CHANNELS_PER_OPERATING_CLASS];
}td_em_map_unoperable_ch_t;

typedef enum td_em_authtype/*add by qinke *//*autoconfig_m2_tlv_t*/
{
    TD_EM_AUTH_OPEN           = 0x1,
    TD_EM_AUTH_WPAPSK         = 0x2,
    TD_EM_AUTH_SHARED         = 0x4,
    TD_EM_AUTH_WPA            = 0x8,
    TD_EM_AUTH_WPA2           = 0x10,
    TD_EM_AUTH_WPA2PSK        = 0x20,
    TD_EM_AUTH_WPA3           = 0x40
}td_em_authtype_e;
#define TD_EM_AUTH_WPA2PSKMIXED  (TD_EM_AUTH_WPAPSK|TD_EM_AUTH_WPA2PSK)
#define TD_EM_AUTH_WPA2MIXED     (TD_EM_AUTH_WPA|TD_EM_AUTH_WPA2)
#define TD_EM_AUTH_WPA3_WPA2_PSK (TD_EM_AUTH_WPA3|TD_EM_AUTH_WPA2PSK)

typedef enum td_em_encryption_type/*add by qinke *//*autoconfig_m2_tlv_t*//*根据协议定义*/
{
    TD_EM_ENCRYPT_NONE        = 0x0001,
    TD_EM_ENCRYPT_WEP         = 0x0002,
    TD_EM_ENCRYPT_TKIP        = 0x0004,
    TD_EM_ENCRYPT_AES         = 0x0008,
    TD_EM_ENCRYPT_TKIPAES     = 0x000c
}td_em_encryption_type_e;

typedef enum td_em_connect_type
{
    TD_EM_CONNECT_TYPE_ESS        = 1,
    TD_EM_CONNECT_TYPE_IBSS       = 2
}td_em_connect_type_e;

typedef enum td_em_bss_type/*add by qinke *//*autoconfig_m2_tlv_t*//*根据协议定义 1-3为保留字段*/
{
    TD_EM_BSS_NONE                = 0,
    TD_EM_TEAR_DOWN               = 4,
    TD_EM_FRONTHAUL_BSS           = 5,
    TD_EM_BACKHAUL_BSS            = 6,
    TD_EM_BACKHAUL_STA            = 7,
}td_em_bss_type_e;

typedef struct easymesh_autoconfig
{
    char device_name      [TD_EM_BUFF_LEN_32 + 1];
    char manufacturer_name[TD_EM_BUFF_LEN_64 + 1];
    char model_name       [TD_EM_BUFF_LEN_64 + 1];
    char model_number     [TD_EM_BUFF_LEN_64 + 1];
    char serial_number    [TD_EM_BUFF_LEN_64 + 1]; 
    unsigned char uuid    [TD_EM_BUFF_LEN_16]; 
    unsigned short auth_types;
    unsigned short encryption_types;
    unsigned short config_methods;
    unsigned char connection_types;
    unsigned short wps_assoc_stat;
    unsigned short wps_config_error;
    unsigned short device_category_id;
    unsigned short device_sub_category_id;
    unsigned int device_oui;
    unsigned short device_pw_id;
    unsigned int os_version;
    unsigned char config_state;
    unsigned char config_err;
}easymesh_autoconfig_t;

typedef struct autoconfig_m1_tlv
{
    int type;
}autoconfig_m1_tlv_t;

typedef struct one_device_info/*add by qinke */
{
    td_em_device_type_e media_type;
    td_em_workmode_e role;
    unsigned char channel_Band;
    unsigned char center_freq1;
    unsigned char center_freq2;
}one_device_info_tlv_t;

typedef struct autoconfig_m2_tlv/*add by qinke */
{
    td_em_connect_type_e connect_type;
    unsigned char association_state;
    td_em_authtype_e auth_type;
    td_em_encryption_type_e encr_type;
    td_em_bss_type_e bss_type;
    unsigned char ssid[TD_EM_WLAN_SSID_MAXLEN + 1];
    unsigned char key[TD_EM_MAX_KEYLEN + 1];
    unsigned char backhaul_sta_assoc_profile1;
    unsigned char backhaul_sta_assoc_profile2;
}autoconfig_m2_tlv_t;

typedef struct ap_radio_basic_cap_tlv
{
    unsigned char radio_unique_identifier[TD_EM_MACADDRLEN];
    int operating_class_count;
    td_em_map_unoperable_ch_t operating_class[TD_EM_MAX_CHANNELS_PER_OPERATING_CLASS];
}ap_radio_basic_cap_tlv_t;

typedef struct radio_identifier_tlv/*add by qinke */
{
    unsigned char radio_unique_identifier[TD_EM_MACADDRLEN];
}radio_identifier_tlv_t;

/*************add by liyahui for Channel Preference TLVs***************/
typedef struct channel_preference_op_class 
{
    unsigned char  op_class; 
    unsigned char  channel_num;//Number of channels specified in the Channel List
    unsigned char  bandwidth;
    unsigned char  channel[MAX_CHANNE_NUM];//Channel list (Each octet describes a single channel number)
    unsigned char  preference;
    chnl_prefer_reason_e  reason_code;
}channel_preference_op_class_t;

typedef struct channel_prefer
{
    unsigned char addr[TD_EM_MACADDRLEN];
    unsigned char op_class_count;
    channel_preference_op_class_t op_class_t[];
}channel_prefer_t;
/*************add by liyahui for Channel Preference TLVs***************/

/*************add by liyahui for Radio Operation Restriction TLVs***************/
typedef struct radio_operation_restriction_channel
{
    unsigned char   channel;
    unsigned char   min_freq_separation;
}radio_operation_restriction_channel_t;

typedef struct radio_operation_restriction_op_class
{
    unsigned char     opclass; 
    unsigned char     channel_num;
    radio_operation_restriction_channel_t restriction_channel[MAX_CHANNE_NUM];
}radio_operation_restriction_op_class_t;

typedef struct radio_operation_restriction
{
    unsigned char addr[TD_EM_MACADDRLEN];
    unsigned char op_class_num;
    radio_operation_restriction_op_class_t radio_operate_restrict_op_class[]; 
}radio_operation_restriction_t;
/*************add by liyahui for Radio Operation Restriction TLVs***************/

/*************add by liyahui for CAC Completion Report TLVs***************/
typedef struct cac_completion_report_class_channel_pairs
{
    unsigned char pairs_op_class;
    unsigned char pairs_channel;
}cac_completion_report_class_channel_pairs_t;

typedef struct cac_completion_report_radio
{
    unsigned char   radio_unique_identifier[TD_EM_MACADDRLEN];
    unsigned char   op_class;
    unsigned char   channel;
    cac_complete_status_e   status;
    unsigned char   pairs_nr;
    cac_completion_report_class_channel_pairs_t *pairs;
}cac_completion_report_radio_t;

typedef struct cac_completion_report
{
    unsigned char   radio_nr;
    cac_completion_report_radio_t *radios;
}cac_completion_report_t;
/*************add by liyahui for CAC Completion Report TLVs***************/

/*************add by liyahui for CAC Status Report TLVs***************/
typedef struct cac_active_class_chnl_pairs
{
    unsigned char   active_op_class;
    unsigned char   active_channel;
    unsigned short int  active_remaining_time;  //Seconds
}cac_active_class_chnl_pairs_t;

typedef struct cac_non_occupancy_classchnl_pairs
{
    unsigned char   nonoccup_op_class;
    unsigned char   nonoccup_channel;
    unsigned short int  nonoccup_remaining_time;  //Seconds

}cac_non_occupancy_classchnl_pairs_t;

typedef struct cac_complete_available_chnl
{
    unsigned char   ac_op_class;
    unsigned char   ac_channel;
    unsigned short int  identify_time;  //minute //Set to zero for non-DFS channels.
}cac_complete_available_chnl_t;

typedef struct cac_status_report
{
    unsigned char   available_channel_nr;
    cac_complete_available_chnl_t* available_channels;
    unsigned char   nonoccup_pair_nr;
    cac_non_occupancy_classchnl_pairs_t* nonoccup_pairs;
    unsigned char   active_pair_nr;
    cac_active_class_chnl_pairs_t* active_pairs;
}cac_status_report_t;
/*************add by liyahui for CAC Status Report TLVs***************/

/*************add by liyahui for Transmit Power Limit TLV**************/
typedef struct transmit_power_limit
{
    unsigned char addr[TD_EM_MACADDRLEN];
    unsigned char transmit_power_limit_eirp;
}transmit_power_limit_t;
/*************add by liyahui for Transmit Power Limit TLV***************/

/*************add by liyahui for Channel Selection Response TLV*************/
typedef struct channel_select_response{
    unsigned char addr[TD_EM_MACADDRLEN];
    chnl_select_rep_code_e response_code;
}channel_select_response_t;
/*************add by liyahui for Channel Selection Response TLV*************/

/*************add by liyahui for Operating Channel Report TLVs**************/
typedef struct operating_ch_report_op_class
{
    unsigned char    op_class;
    unsigned char    cur_channel; //Current operating channel number in the Operating Class
}operating_ch_report_op_class_t;

typedef struct operating_channel_report
{
    unsigned char    addr[TD_EM_MACADDRLEN];//Radio Unique identifier.
    unsigned char    cur_op_class_nr;//Number of current operating classes
    unsigned char    cur_tx_pwr;//Current Transmit Power EIRP
    operating_ch_report_op_class_t operating_channels[];
}operating_channel_report_t;
/*************add by liyahui for Operating Channel Report TLVs**********/

/*************add by liyahui for CAC Request TLVs*********************/
typedef struct cac_request_radio
{
    unsigned char   radio_unique_identifier[TD_EM_MACADDRLEN];
    unsigned char   op_class;
    unsigned char   channel;
    cac_method_e   cac_method;
    cac_complete_action_e   cac_completion_action;
}cac_request_radio_t;

typedef struct cac_request
{
    unsigned char      radio_nr;
    cac_request_radio_t radios[];
}cac_request_t;
/*************add by liyahui for CAC Request TLVs*********************/

/*************add by liyahui for CAC Termination TLVs*****************/
typedef struct cac_termination_radio{
    unsigned char   radio_unique_identifier[TD_EM_MACADDRLEN];
    unsigned char   op_class;
    unsigned char   channel;
}cac_termination_radio_t;

typedef struct cac_termination{
    unsigned char           radio_nr;
    cac_termination_radio_t radios[];
}cac_termination_t;
/*************add by liyahui for CAC Termination TLVs*****************/

/*************add by liyahui for Backhaul Steering Request TLVs**********/
typedef struct backhaul_steering_request{
    unsigned char      backhaul_sta_mac[TD_EM_MACADDRLEN];    // The MAC address of the associated backhaul STA operated by the MultiAP Agent
    unsigned char      target_bssid[TD_EM_MACADDRLEN];  // The BSSID of the target BSS.
    unsigned char      op_class; //Operating class  
    unsigned char      channel;//Channel number on which Beacon frames are being transmitted by the target BSS.直接显示信道，不是那种多个
}backhaul_steering_request_t;
/*************add by liyahui for Backhaul Steering Request TLVs************/

/*************add by liyahui for Backhaul Steering Rsponse TLVs***********/
typedef struct backhaul_steering_response
{
    netlink_event_head_e type;
    unsigned char        sta_mac[TD_EM_MACADDRLEN];    // The MAC address of the associated backhaul STA operated by the MultiAP Agent
    unsigned char        bssid[TD_EM_MACADDRLEN];  // The BSSID of the target BSS.
    unsigned char        target_bssid[TD_EM_MACADDRLEN];
    backhaul_steer_result_e   result_code; 
    bh_steer_error_code_e     error_code;
}backhaul_steering_response_t;
/*************add by liyahui for Backhaul Steering Rsponse TLVs*********/

/*************add by liyahui for Steering Policy TLVs***************/
typedef struct radio_steering_config{
    unsigned char    radio_mac[TD_EM_MACADDRLEN];
    unsigned char    policy;//Steering Policy
    unsigned char    cu_threshold;//Channel Utilization Threshold
    unsigned char    rcpi_threshold;//RCPI Steering Threshold
}radio_steering_config_t;

typedef struct steering_policy
{
    unsigned char            radio_nr;//Number of radios for which control policy is being indicated
    radio_steering_config_t  radio_steer_policy_data[MAX_RADIOS_NUM];
}steering_policy_t;
/*************add by liyahui for Steering Policy TLVs****************/

/*************add by liyahui for Client Steer Request TLVs************/
typedef struct client_req_bss_info
{
    unsigned char   target_bssid[TD_EM_MACADDRLEN];//Target BSSID
    unsigned char   opclass;//Target BSS Operating Class  
    unsigned char   channel;//Target BSS Channel Number for channel on which the Target BSS is transmitting Beacon frames.
}client_req_bss_info_t;

typedef struct client_req_steer_sta
{
    unsigned char    sta_mac[TD_EM_MACADDRLEN];
}client_req_steer_sta_t;

typedef struct client_steer_req
{
    unsigned char     radio_mac[TD_EM_MACADDRLEN];
    unsigned char     req_mode;
    unsigned short    opp_window;
    unsigned short    disassoc_time;
    unsigned char     sta_num;
    client_req_steer_sta_t  steer_req_sta[MAX_STA_NUM];
    unsigned char     target_bss_num;
    client_req_bss_info_t   req_bss_info[MAX_TARGET_BSS_NUM];
}client_steer_req_t;
/*************add by liyahui for Client Steer Request TLVs***************/

/*************add by liyahui for Client Steer BTM Report TLVs*************/
typedef struct client_steer_btm_report
{
    netlink_event_head_e type;
    unsigned char     radio_mac[TD_EM_MACADDRLEN];
    unsigned char     sta_mac[TD_EM_MACADDRLEN];
    unsigned char     btm_status;
    unsigned char     target_bssid[TD_EM_MACADDRLEN];
}client_steer_btm_report_t;
/*************add by liyahui for Client Steer BTM Report TLVs***************/

/*************add by liyahui for NOT SUPPROT BTM STA  Report ***********/
typedef struct sta_mac_info
{
    unsigned char    sta_mac[TD_EM_MACADDRLEN];
}sta_mac_info_t;

typedef struct sta_info_report
{
    netlink_event_head_e type;
    unsigned char    sta_num;
    sta_mac_info_t   sta_msg[MAX_STA_NUM];
}sta_info_report_t;
/*************add by liyahui for NOT SUPPROT BTM STA  Report *************/

/*************add by liyahui for Client Association Control Request TLVs************/
typedef struct client_assoc_req
{
    unsigned char    bssid[TD_EM_MACADDRLEN];
    td_assoc_control_e  assoc_control;
    unsigned short   validity_period;
    unsigned char    sta_num;
    sta_mac_info_t   sta_info[MAX_STA_NUM];
}client_assoc_req_t;
/*************add by liyahui for Client Association Control Request TLVs************/

/************* easymesh_ht_ability begin ***********/
typedef struct easymesh_ht_ability{
    unsigned char capability_flag;
    unsigned char bssid[TD_EM_MACADDRLEN];
    unsigned char data;
} easymesh_ht_ability_tlv_t;

typedef struct easymesh_vht_ability{
    unsigned char    capability_flag;
    unsigned char    bssid[TD_EM_MACADDRLEN];
    unsigned char    spatial_streams;
    unsigned char    vht_capable;
    unsigned short   tx_mcs;
    unsigned short   rx_mcs;
} easymesh_vht_ability_tlv_t;

typedef struct easymesh_he_ability{
    unsigned char    capability_flag;
    unsigned char    bssid[TD_EM_MACADDRLEN];
    unsigned char    spatial_streams;
    unsigned char    he_capable;
} easymesh_he_ability_tlv_t;
/********   chscan_ability begin  *********/
typedef struct easymesh_opclass_info{
    unsigned char    op_class;
    unsigned char    channel_num;
    unsigned char    channel_list[TD_EM_MAX_CHANNELS_PER_OPERATING_CLASS];
} easymesh_opclass_info_t;

typedef struct easymesh_chscan_ability{
    unsigned char               radio_bssid[TD_EM_MACADDRLEN];
    unsigned char               opclass_num;
    easymesh_opclass_info_t     opclass_info[];
} easymesh_chscan_ability_tlv_t;

typedef struct easymesh_bhstaradio_capability{
    unsigned char    radio_bssid[TD_EM_MACADDRLEN];
    unsigned char    addr_include;
    unsigned char    bhsta_radio_addr[TD_EM_MACADDRLEN];
} easymesh_bhstaradio_capability_tlv_t;

typedef struct easymesh_ap_metrics{
    unsigned char    radio_bssid[TD_EM_MACADDRLEN];
    unsigned char    channel_utilization;
    unsigned short   assoc_sta_num;
    unsigned char    bit_service_parameters;
    unsigned char    service_parameters[12];
} easymesh_ap_metrics_tlv_t;

typedef struct easymesh_associated_sta_traffic_stat{
    unsigned char    assic_staaddr[TD_EM_MACADDRLEN];
    unsigned int     bytesSent;
    unsigned int     bytesReceived;
    unsigned int     packetsSent;
    unsigned int     packetsReceived;
    unsigned int     txPacketsErrors;
    unsigned int     rxPacketsErrors;
    unsigned int     retransmissionCount;
} easymesh_associated_sta_traffic_stat_tlv_t;
/************associated_sta_link_metrics_tlv begin***********/
typedef struct easymesh_numbssrepfor_sta{
    unsigned char           staassic_bssaddr[TD_EM_MACADDRLEN];
    unsigned int            time_delta;
    unsigned int            rx_rate;
    unsigned int            tx_rate;
    unsigned char           rcpi;
} easymesh_numbssrepfor_sta_t;

typedef struct easymesh_associated_sta_link_metrics{
    unsigned char                   assic_staaddr[TD_EM_MACADDRLEN];
    unsigned char                   bss_num;
    easymesh_numbssrepfor_sta_t     bss_assoc_info[];
} easymesh_associated_sta_link_metrics_tlv_t;
/************associated_sta_link_metrics_tlv end ***********/

/************** ap_extended_metrics_tlv begin *************/
typedef struct easymesh_ap_extended_metrics{
    unsigned char                   bssid[TD_EM_MACADDRLEN];
    unsigned long                   unicastBytes_tx;
    unsigned long                   unicastBytes_rx;
    unsigned long                   multicastBytes_tx;
    unsigned long                   multicastBytes_rx;
    unsigned long                   broadcastBytes_tx;
    unsigned long                   broadcastBytes_rx;
}easymesh_ap_extended_metrics_tlv_t;
/***************  ap_extended_metrics_tlv end *************/

/***************   radio_metrics_tlv begin   *************/
typedef struct easymesh_radio_metrics{
    unsigned char          bssid[TD_EM_MACADDRLEN];
    unsigned char          noise;
    unsigned char          transmit;
    unsigned char          receiveSelf;
    unsigned char          receiveOther;
} easymesh_radio_metrics_tlv_t;
/***************    radio_metrics_tlv end    *************/
/****** easymesh_assocsta_extended_link_metrics_tlv begin *****/
typedef struct easymesh_numof_bssid_for_agent{
    unsigned char         bssid[TD_EM_MACADDRLEN];
    unsigned int          lastdatadownlinkrate;
    unsigned int          lastdatauplinkrate;
    unsigned int          utilizationreceive;
    unsigned int          utilizationtransmit;
} easymesh_numof_bssid_for_agent_t;

typedef struct easymesh_assocsta_extended_link_metrics{
    unsigned char                     sta_addr[TD_EM_MACADDRLEN];
    unsigned char                     bss_num;
    easymesh_numof_bssid_for_agent_t  sta_bss_info[];
} easymesh_assocsta_extended_link_metricstlv_t;
/****** easymesh_assocsta_extended_link_metrics_tlv end *****/
/************* clannel_scan_result_tlv begin ***********/
typedef enum channel_band_width {
    ERROR                  = 0,
    TD_20M                 = 1,
    TD_40M                 = 2,
    TD_80M                 = 3,
    TD_160M                = 4,
    TD_80M_80M             = 5,
}channel_band_width_t;
typedef struct easymesh_channel_scan_neighbor {
    unsigned char       bssid[TD_EM_MACADDRLEN];
    unsigned char       ssid_length;
    unsigned char       signal_strength;
    unsigned char       bss_load;
    unsigned char       channel_band_width; //20 or 40 or 80 or 80+80 or 160 MHz - 1 2 3 4 5  (0 -> invalid)
    unsigned char       channel_utilization;
    unsigned short      station_count;
    char                ssid[TD_EM_WLAN_SSID_MAXLEN];
}easymesh_channel_scan_neighbor_t;

typedef struct easymesh_channel_scan_result_channel {
    unsigned char       channel;
    unsigned char       scan_status;
    char                timestamp[10];
    unsigned char       channel_utilization;
    unsigned char       noise;
    unsigned short      neighbor_nr;
    unsigned char       scan_timer;
}easymesh_channel_scan_result_channel_t;

typedef struct easymesh_channel_scan_result_radio {
    unsigned int  msg_flag;
    unsigned char priv_bssid[TD_EM_MACADDRLEN];
    unsigned char channel_nr;
}easymesh_channel_scan_result_radio_t;
/************* clannel_scan_result_tlv end ***********/
/************* beacon_metrics_rsp_tlv begin ***********/
enum easymesh_measure_result{
    TD_MEASUREMENT_UNKNOWN    = 0,
    TD_MEASUREMENT_PROCESSING = 1,
    TD_MEASUREMENT_SUCCEED    = 2,
    TD_MEASUREMENT_INCAPABLE  = 3,
    TD_MEASUREMENT_REFUSED    = 4,
    TD_MEASUREMENT_RECEIVED   = 5,
};

typedef struct easymesh_dot11k_beacon_measurement_report_info{
    unsigned char measure_token;
    unsigned char measure_mode;
    unsigned char measure_report_type;
    unsigned char op_class;
    unsigned char channel;
    unsigned int  measure_time_hi;
    unsigned int  measure_time_lo;    
    unsigned short measure_duration;
    unsigned char frame_info;
    unsigned char RCPI;
    unsigned char RSNI;
    unsigned char bssid[TD_EM_MACADDRLEN];
    unsigned char antenna_id;
    unsigned int  parent_tsf;
}easymesh_11k_beacon_measurement_repinfo_t;

typedef struct easymesh_beacon_element_info{
    unsigned char                     measure_result;
    easymesh_11k_beacon_measurement_repinfo_t    beacon_report_info;
    unsigned char subelements_len;    
    unsigned char subelements[MAX_BEACON_SUBLEMENT_LENG];
} em_beacon_element_info_t;

typedef struct easymesh_beacon_metrics_rsp{
    unsigned int                      info_type;
    unsigned char                     sta_addr[TD_EM_MACADDRLEN];
    unsigned char                     beacon_ie;
    unsigned char                     element_id;
    unsigned char                     beacon_rep_num;    //该TLV中包含的测量报告元素的数量
    em_beacon_element_info_t          beacon_emt[];
} em_beacon_metrics_rsp_tlv_t;
/************* beacon_metrics_rsp_tlv end ***********/
/***** 1905.1 transmitter link metric TLV begin *****/
typedef struct easymesh_1905_txlink_metric{
    unsigned int                     packeterrors;
    unsigned int                     tx_packet;
    unsigned short                   tx_avarage;  //macThroughputCapacity最大吞吐量
    unsigned short                   linkavailability;
    unsigned short                   phy_rate;
} easymesh_1905_txlink_metric_t;

typedef struct easymesh_1905_rxlink_metric{
    unsigned int                     packeterrors;
    unsigned int                     rx_packet;
    unsigned char                    rssi;
} easymesh_1905_rxlink_metric_t;

/****** 1905.1 transmitter link metric TLV end ******/

/***************   Beacon Metrics Query_tlv begin   *************/
typedef struct ap_channel_report
{
    unsigned char len;
    unsigned char op_class;
    unsigned char channel[AP_CHANNELS_NUM];
}ap_channel_report_t;

typedef struct easymesh_beacon_metrics_query{
    unsigned char          sta_mac[TD_EM_MACADDRLEN];
    unsigned char          op_class;
    unsigned char          channel_num;
    unsigned char          bssid[TD_EM_MACADDRLEN];
    unsigned char          report_detail;
    unsigned char          ssid_len;
    unsigned char          ssid[SSID_MAXLEN + 1];
    unsigned char          request_ie_len;
    unsigned char          request_ie[MAX_REQ_IE_LEN];
    unsigned char          ap_chnl_num;
    ap_channel_report_t ap_chnl_report[MAX_AP_CHNEL_REPORT];
} easymesh_beacon_metrics_query_tlv_t;
/***************    Beacon Metrics Query_tlv end    *************/

#endif
