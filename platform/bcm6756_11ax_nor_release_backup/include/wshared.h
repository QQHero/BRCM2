/*****************************************************************************
×÷ó?·??§: ìá1???é?2?òμ??μ??ó?úí·???t￡?í?ê±1??ú2?WShare ó? WServerê1ó?

*****************************************************************************/


#ifndef __AP_WSHARED_H__
#define __AP_WSHARED_H__
#include <sys/types.h>

#define GET_BUF_LEN              16

#define WLAN_SERVER_SOCKET_PATH     "/var/wlan_server_socket"
/*available channels, 2.4G:1-14,5G: 8 36,40,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165 179 181 188*/
#define MAX_CHANNELS_NUM            64

#define WIFISON_LAN "lan"
#define WIFISON_LAN_BR "br-"WIFISON_LAN
#define WIFISON_GUEST "guest"
#define WIFISON_GUEST_BR "br-"WIFISON_GUEST
#define WIFISON_ETH_NAME_PATH "/var/wifison_eth_name"

/* ?μ?×·???2é?ˉ?¨ò? */
#define FSS_FFT_BIN_MAX             56

typedef void td_wds_addif_brx_pfun (char *wds_buf);
extern td_wds_addif_brx_pfun *g_wds_addif;

typedef void td_em_loop_pfun (char *osif, int cfg);
extern td_em_loop_pfun *g_em_loop;

typedef enum sideband_flag
{
    SIDEBAND_NONE,      /*none*/
    SIDEBAND_UPPER,     /*lower*/
    SIDEBAND_LOWER      /*upper*/
}sideband_flag_e;

typedef struct flow_statistic{
     unsigned long rxpacks;
     unsigned long txpacks;
     unsigned long long txbytes;
     unsigned long long rxbytes;    
     unsigned long rxerror;
     unsigned long txerror;
}flow_statistic_t;

typedef struct channel_list
{
    int num;
    int channel[MAX_CHANNELS_NUM];
}channel_list_t;

/*begin 2018/05/11￡???????￡???è??éó?D?μàáD±íμ?è?2?*/
typedef struct channel_list_inparam
{
    int bw;             /*μ±?°′??í*/
    sideband_flag_e sb; /*μ±?°′??íμ?±?′?*/
    char country[3];    /*μ±?°1ú?ò??*/
}channel_list_inparam_t;
/*end 2018/05/11￡???????￡???è??éó?D?μàáD±íμ?è?2?*/

typedef struct station_info
{
    char mac[18];
    char link_time[16];     /*unit: second*/
    char rssi[8];           /*unit: dBm*/
    char noise[8];          /*unit: dBm*/
    char quality[8];        /*unit: percent(%)*/
    char link_rate[16];     /*unit: Mbps*/
    char tx_rate[16];       /*unit: Mbps*/
    char rx_rate[16];       /*unit: Mbps*/
    unsigned long long tx_bytes;  /*unit: bytes*/
    unsigned long long rx_bytes;  /*unit: bytes*/
    unsigned long tx_failures;
}station_info_t;

typedef struct chanscore_info
{
    unsigned int num;
    unsigned int chan[MAX_CHANNELS_NUM];
    unsigned int chanscore[MAX_CHANNELS_NUM];
    unsigned int chanlevel[MAX_CHANNELS_NUM];
}chanscore_info_t;

typedef enum easymesh_op
{
    EM_OP_NONE                         = 0,

    EM_ASSOC_CLIENT_INFO               = 1,
    EM_DEVICE_INFO                     = 2,
    EM_AP_OPERATE_BSS                  = 3,
    EM_AP_BASIC_CAP                    = 4,
    EM_AP_RADIO_ID                     = 5,
    EM_AUTO_CONFIG_M1                  = 6,
    EM_AUTO_CONFIG_M2                  = 7,
    EM_AUTO_CONFIG_M1_FD               = 8,
    EM_AUTO_CONFIG_M2_FD               = 9,
    EM_SAE_CAP                         = 10,
    EM_XMESH_SET_BSS                   = 12,
    EM_CHANNEL_PREFER                  = 30,
    EM_RADIO_OPERATE_RESTRICT          = 31,
    EM_CAC_COMPLETION_REPORT           = 32,
    EM_CAC_STATUS_REPORT               = 33,
    EM_TRANSMIT_POWER_LIMIT            = 34,
    EM_CHANNEL_SELECT_RESPONSE         = 35,
    EM_OPERATE_CHANNEL_REPORT          = 36,
    EM_CAC_REQUEST                     = 37,
    EM_CAC_TERMINATION                 = 38,
    EM_BACKHAUL_STEER_REQ              = 39,
    EM_BACKHAUL_STEER_RSP              = 40,
    EM_STEER_POLICY                    = 41,
    EM_METRIC_REPORT_POLICY            = 42,
    EM_DEFAULT_802_1Q_SETING           = 43,
    EM_CHANNEL_SCAN_REPORT_POLICY      = 44,
    EM_UNSUCCESSFUL_ASSOCIAT_POLICY    = 45,
    EM_BACKHAUL_BSS_CONFIGURATION      = 46,
    EM_CLIENT_STEER_REQUEST            = 47,
    EM_CLIENT_STEER_BTM_REQUEST        = 48,
    EM_CLIENT_ASSOCIAT_CONTROL_REQ     = 49,
    EM_TUNNELED_MESSAGE                = 50,
    EM_ASSOCIATION_STATUS_NOTIFY       = 51,
    EM_AP_CAPABILITY                   = 61,
    EM_HT_AP_CAPABILITY                = 62,
    EM_VHT_AP_CAPABILITY               = 63,
    EM_HE_AP_CAPABILITY                = 64,
    EM_CHSCAN_CAPABILITY               = 65,
    EM_AP_CAPABILITY_2                 = 66,
    EM_BACKHAUL_CAPABILITY             = 67,
    EM_AP_METRIC_QUERY                 = 68,
    EM_CAC_CAPABILITY                  = 69,
    EM_AP_METRICS                      = 70,
    EM_ASSOC_STA_TRAFFIC_STAT          = 71,
    EM_ASSOC_STA_LINK_METRICS          = 72,
    EM_AP_EXTENDED_METRICS             = 73,
    EM_RADIO_METRICS                   = 74,
    EM_ASSOC_STA_EXTEND_LINK_METRICS   = 75,
    EM_DO_CHANNEL_SCAN                 = 76,
    EM_1905_TXLINK_METRIC              = 77,
    EM_1905_RXLINK_METRIC              = 78,
    EM_BEACON_METRICS_QUERY            = 79,
    EM_ETHX_CONNECT                    = 80,
    EM_GET_ETH_STATUS                  = 81,
    EM_ASSOC_PRIO_FLAG                 = 82,
    EM_ROLE                            = 83,
    EM_XMESH_ENABLE                    = 84,
    EM_XMESH_HEART_BEAT                = 85,
    EM_OP_MAX
}easymesh_op_e;

/*ap scan results*/
typedef struct ap_info
{
    char ssid[48+1];        /*12???D??×?·?ò?é?￡?gb2312μ???×a??3éutf-8êy?Yò?3?*/
    char ssid_encode[8];    /*utf-8,gb2312*/
    char mac[18];
    char nettype[12];       /*b,g,bg,bgn,bgn+ac,bgn+ac+ax;a,an,an+ac,an+ac+ax*/
    char channel[8];
    char bandwidth[8];     /*20,40,80,auto*/
    char nctrlsb[8];        /*none,lower,upper*/
    char signal[8];
    char security[32];      /*none,wep,wpa&wpa2/aes*/
    char capability[8];    /*AD-hoc,AP*/
}ap_info_t;

typedef enum extend_link{
    EXTEND_LINK_INVALID,    /* invalid state */
    EXTEND_LINK_DOWN,       /* interface down */
    EXTEND_LINK_IDLE,       /* interface up, before link */
    EXTEND_LINK_SCANNING,   /* scanning uper-AP */
    EXTEND_LINK_CONNECTED,  /* connected success */
#ifdef CONFIG_TENDA_REPEATER_STATUS
    EXTEND_LINK_WRONG_KEY,  /* wrong key*/
#endif
    EXTEND_LINK_ERROR       /* connect error */
}extend_link_e;

typedef enum wifi_wps_status
{
    WPS_STATUS_INIT,
    WPS_STATUS_STOP,
    WPS_STATUS_START,
    WPS_STATUS_PBC_OVERLAP,
    WPS_STATUS_ERROR,
    WPS_STATUS_SUCCESS,
    /* WPS-PBC session is going on */
    WPS_STATUS_PBC_PROGRESSING,
    WPS_STATUS_PBC_TIMEOUT,
    /* NO WPS session recorded */
    WPS_STATUS_NONE,
    WPS_STATUS_MAX
}wifi_wps_status_e;

typedef enum wifi_wps_error_indication
{
    WPS_EI_NO_ERROR,
    WPS_EI_SECURITY_TKIP_ONLY_PROHIBITED,
    WPS_EI_SECURITY_WEP_PROHIBITED,
    WPS_EI_AUTH_FAILURE,
    WPS_EI_UNKNOWN,
    WPS_EI_MAX
}wifi_wps_ei_e;

/* ath_ioctlà??1μ?×ó?üá? */
typedef enum {
    TD_IOCTL_TDMA = 1,
} wifi_td_ath_ioctl_e;

#define SSID_LEN 32
#define PSK_HEX 64
#define PSK_ASCII 63
#define SERC_MODE_LEN 20
typedef struct wpa_network
{
    char ssid[SSID_LEN+1];
    char psk[PSK_HEX+1];
    char key_mgmt[SERC_MODE_LEN+1];
} wpa_network_t;

typedef struct wps_stats
{
    enum wifi_wps_status status;
    enum wifi_wps_error_indication ei;
    /* provide wireless information after wps-pair */
    wpa_network_t network;
}wps_stats_t;

typedef struct interface_stats
{
    char state;         //0-disable 1-enable
}interface_stats_t;


typedef struct chan_info //dynamic channel infomation
{
    char channel[8];
    char bandwidth[8];       /*20,40,80*/    
    char side_flag[8];       /*none,lower,upper*/ 
    int qbss_load;
}chan_info_t;

typedef struct spectrum_info
{
   int channel;
   int bss_num;
   int noisefloor;
   int chanload;
}spectrum_info_t;

/* 频谱分析用户数据结构体 */
typedef struct td_fss_usr_data_s {
        u_int16_t   channel;
        int8_t    cur_bin_pwr[FSS_FFT_BIN_MAX]; 
        int8_t    max_bin_pwr[FSS_FFT_BIN_MAX]; 
        int8_t    agv_bin_pwr[FSS_FFT_BIN_MAX]; 
} __attribute__((packed)) td_fss_usr_data_t;

#define TD_ATH_IOCTL_EXTEND             (SIOCDEVPRIVATE + 9)

typedef struct {
    u_int32_t cmd;               /* common ioctl definition */
    void *buf;                  /* pointer to user buffer */
    u_int32_t len;               /* length of user buffer */
} td_ex_ioctl_t;

typedef struct tdma_sta_level_s {
    u_int8_t active_level;
    u_int8_t active_status;
    u_int8_t sta_mac[6];
} tdma_sta_level_t;

typedef struct tdma_cfg {
    unsigned int hrtime;
    unsigned int update_time;
    int enable;
} tdma_cfg_t;

#define CUSTOM_MAX_IE_SIZE 128
#define CUSTOM_MAX_IE_NUM 128
/* OUI in hex */
#define CUSTOM_IE_OUI "c85359"

typedef struct td_custom_ie_head
{
    /* type(the first byte) can be used to distinguish different messages */
    u_int8_t        type;
    u_int8_t        nonce;
    u_int8_t        mac[6];
    u_int8_t        reserved[4];
} __attribute__((__packed__)) td_custom_ie_head_t;

typedef struct td_custom_ie_info
{
    td_custom_ie_head_t* head;
    char length;
} td_custom_ie_info_t;

typedef struct td_custom_ie_report
{
    td_custom_ie_info_t ies[CUSTOM_MAX_IE_NUM];
    char ie_num;
    /* truncated==1 means that some ie cache in driver is not copyed for small buffer */
    char truncated;
} td_custom_ie_report_t;

typedef enum wifison_damon
{
    WIFISON_HYD_LAN,    /* default hyd damon, listen on 127.0.0.1:7777 */
    WIFISON_HYD_GUEST,  /* hyd damon for wifison original guest network, listen on 127.0.0.1:7778 */
    WIFISON_LBD         /* reserved for lbd which listen on 127.0.0.1:7787 */
} wifison_damon_e;

typedef enum adv_scan_type
{
    TD_SCAN_ALL_ENTRY                   = 0,
    TD_SCAN_MESH_NODE                   = 1,
    TD_SCAN_MAX
}adv_scan_results_type_e;

/* copied from td_scan.h: td_scan_config */
typedef struct advanced_scan_arg {
    /* d11 scan type. 0: active; 1: passive */
    unsigned char scan_type;
    /* duration before switch to the next channel. unit: ms. */
    unsigned char interval;
    /* if passive_scan is zero, probe request will be sent during scanning */
    unsigned char passive_scan;
    /* count of channel_list, zero means scan all available channels */
    unsigned int channel_count;
    /* channels to scan */
    unsigned char channel_list[64];
    /* ssid to be added to probe request if it's not empty */
    char ssid[SSID_LEN + 1];
} advanced_scan_arg_t;

typedef struct advanced_scanres_arg
{
    adv_scan_results_type_e type;
}advanced_scanres_arg_t;

typedef struct advanced_scan_out_arg
{
    advanced_scan_arg_t scan_cfg;
    advanced_scanres_arg_t result_cfg;
}advanced_scan_out_arg_t;

typedef struct wf_country {
    char country_abbrev[4];    /**< nul-terminated country code used in
                         * the Country IE
                         */
    int rev;                /**< revision specifier for ccode
                         * on set, -1 indicates unspecified.
                         * on get, rev >= 0
                         */
    char ccode[4];        /**< nul-terminated built-in country code.
                         * variable length, but fixed size in
                         * struct allows simple allocation for
                         * expected country strings <= 3 chars.
                         */
} wf_country_t;
/*
function:       tpi_wifi_get_osifname
description:    get os ifname of mib ifname
input:      mibif
output:     osif
return:         ok:osif, false: NULL
*/
char* tpi_wifi_get_osifname(char *mibif, char *osif);

/*
function:       tpi_wifi_get_easymesh_bsss
description:    Gets the mesh role properties of the input band
input:          mibif
output:         bsss
return:         ok:bsss, false: NULL
*/
char *tpi_wifi_get_easymesh_bsss(char *mibif, char *bsss);

/*
function:       tpi_wifi_get_osband
description:    Gets the real interface name of the current schema
input:          mibif
output:         osband
return:         ok:osband, false: NULL
*/
char *tpi_wifi_get_osband(char *mibif, char *osband);

/*
function:       tpi_wifi_get_wifison_ifname
description:    get os ifname of mib ifname in wifison workmode
                wlan0.0~wlan0.5 for fonthual, wlan0.7 for backhaul ap,
                wlan0.x for backhaul sta, wlan0.6 reserved for wifison native guest.
                If wlan0.6 is enabled, two bridges will be used for traffic separation.
                The os ifname of wlan0.6/wlan0.7/wlan0.x is unknown until wifison is configured.
input:      mibif
output:     osif
return:         ok:osif, false: NULL
*/
char* tpi_wifi_get_wifison_ifname(char *mibif, char *osif);

/*
function:       tpi_wifi_get_mibifname_by_osname
description:    get mibifname by the os_ifname
for example in bcm:     controller: osif:wl1    mibif:wlan1.0 agent: osif:wl1.8    mibif:wlan1.0

input:      mibif
output:     osif
return:         ok:0, false:-1
*/
int tpi_wifi_get_mibifname_by_osname(char *osif, int is_rfif, char *mibif);

/*
function:       tpi_wifi_exec_hyd_cmd
description:    execute a command of hyd(or lbd), and get the output string
input:          cmd: command to execute, damon: where to send the command
output:         output: command output string
return:         ok:0, false: -1
*/
int tpi_wifi_exec_hyd_cmd(const char* cmd, char* output, int output_size, wifison_damon_e damon);


/*
function:       tpi_wifi_get_wps_status
description:    get wps status of `osifname`
input:      osifname
output:     status
return:         success:0, failed:-1
*/
int tpi_wifi_get_wps_status(char *osifname, wps_stats_t* status);

int tpi_ifconfig_dev_down_up(char *osifname, int need_up);

/*
function:       tpi_wifi_check_downup
description:    get interface downup status
input:      osifname
output:     
return:         up:1, down:0
*/
int tpi_wifi_check_downup(char *osifname);

/*
function:       tpi_wifi_get_mibif_band
description:    get band of mib ifname
input:      mibif
output:     
return:         5G:5, 2.4G:2
*/
int tpi_wifi_get_mibif_band(char *mibif);

/*
function:       tpi_wifi_scan
description:    ap scan once
input:      osif: scan interface
output:     
return:         ok:0, failed:-1
*/
int tpi_wifi_scan(char *osif);

/*
function:       tpi_wifi_get_scanresults
description:    get apscan results
input:      osif: scan interface        
            num: ap max number
output:     aplist: ap list buffer
return:         ap real number
*/
int tpi_wifi_get_scanresults(char *osif, ap_info_t *aplist, int num);


/*
function:       tpi_wifi_scan_results
description:    do scan && get results
input:      osif: scan interface        
            num: ap max number
            count: get fail count, 1s/pre
output:     aplist: ap list buffer
return:         ap real number
*/
int tpi_wifi_scan_results(char *osif, ap_info_t *aplist, int num,unsigned char count);

/*
function:       tpi_wifi_get_clientlist
description:    get bss station list
input:      osif: bss interface     
            num: station max number
output:     aplist: station list buffer
return:         station real number
*/
int tpi_wifi_get_clientlist(char *osif, station_info_t *stalist, int num);

/*
function:       tpi_wifi_get_channel_list
description:    get available channels of specific interface,not include 0
input:      osif: radio interface
            bw: current bandwidth
            sb: sideband flag, none/upper/lower
output:     channels: channel list
return:         -1:error, 0:ok
*/
int tpi_wifi_get_channel_list(char *osif, int bw, sideband_flag_e sb, channel_list_t *channels);

/*
function:       tpi_wifi_get_channel_list_by_country
description:    ?ù?Y1ú?òoí′??í·μ???éó?D?μàáD±í
input:      osif: ?÷?ó?ú??
            inparam.bw: μ±?°′??í
            inparam.sb: 40M/80Mμ?±?′?none/upper/lower
            inparam.country: 1ú?ò??
output:     channels: ?éó?channel áD±í
return:         -1:error, 0:ok
*/
int tpi_wifi_get_channel_list_by_country(char *osif, channel_list_inparam_t inparam, channel_list_t *channels);

/*
function:       tpi_wifi_get_flow_statistic 
description:    get flow statistic of interface
input:      osif: wifi interface            
output:     fstat: flow statistic data
return:         -1:failed, 0:ok
*/
int tpi_wifi_get_flow_statistic(char *osif, flow_statistic_t *fstat);


/*
function:       tpi_wifi_get_extend_state
description:    get ap-extend link status
input:      osif: ap-extend interface       
output:
return:         ap-extend link status
*/
extend_link_e tpi_wifi_get_extend_state(char *osif);

/*
function:       tpi_wifi_get_dfs_chaninfo
description:    get DFS chan select info
input:      osif: wifi interface          
output:     chan_info_t: channel bw chan_offset;
return:         -1:failed, 0:ok
*/
int tpi_wifi_get_dfs_chaninfo(char *osif, chan_info_t *cinfo);

/*
function:       tpi_wifi_get_extend_linktime
description:    get linktime of extend interface
input:      osif: extend interface          
output:     linktime: extend linktime;
return:         -1:failed, 0:ok
*/
int tpi_wifi_get_extend_linktime(char *osif, unsigned long *linktime);
/*
function:       tpi_wifi_get_extend_wdslinktime
description:    get linktime of extend interface
input:      osif: extend interface          
output:     linktime: extend linktime;
return:         -1:failed, 0:ok
*/
int tpi_wifi_get_extend_wdslinktime(char *osif, unsigned long *linktime, char *mac);
/*
function:       tpi_wifi_get_extend_wdslinkstatus
description:    get linktime of extend interface
input:      osif: extend interface          
output:     linktime: extend linktime;
return:         -1:failed, 0:ok
*/
extend_link_e tpi_wifi_get_extend_wdslinkstate(char *osif, char *mac);

/*
function:       tpi_wifi_get_chanscores
description:    get channel scores 
input:      osif: wifi interface            
output:     chanscores: channel scores;
return:         -1:failed, 0:ok
*/
int tpi_wifi_get_chanscores(char *osif, chanscore_info_t *chanscores);

/*
function:        tpi_wifi_get_chaninfo 
description:    get current channel status of interface
input:            osif: wifi interface            
output:          cinfo: current channel, bandwidth and sideband
return:          -1:failed, 0:ok
*/
int tpi_wifi_get_chaninfo(char *osif, chan_info_t *cinfo);

/*
function:        tpi_wifi_is_scanning 
description:    get current ap scan status
input:            osif: wifi interface            
output:          
return:          0: scan finished or not start, 1: scanning now
*/
int tpi_wifi_is_scanning(char *osif);

/*
function:        tpi_wifi_get_noise 
description:    get current noise dBm
input:            osif: wifi interface            
output:          noise: noise value
return:          -1:fail, 0:ok
*/
int tpi_wifi_get_noise(char *osif, int *noise);
int tpi_wifi_get_spectrum(char *osif,struct spectrum_info *sinfo,int num,int count);

/*
function:        tpi_set_tdma_sta_level 
description:     set sta level
input:           ifname: radio interface  note:[wifi1]
                 sta_level:sta set param
output:          NULL
return:          -1:fail, 0:ok
*/
void tpi_set_tdma_sta_level(char *ifname, tdma_sta_level_t sta_level);

void tpi_set_tdma_cfg(char *ifname, tdma_cfg_t *cfg);

/*
function:        tpi_wifi_get_fss_result 
description:     get current fss result
input:           osif: radio interface  note:[QCA:athx RTK:wlanx]
                 sinfo:fss info
                 num:channel num of setting
output:          sinfo: fss info
return:          -1:fail, 0:ok
*/
void tpi_wifi_set_fss_param(char *ifname, int fss_channel, int fss_en);
int tpi_wifi_get_fss_result(char *osif, td_fss_usr_data_t *sinfo, int num);
    
/*
function:        tpi_wifi_get_custom_ie
description:     get the received custom InfornationElements
input:           buff_len: size of the buffer
output:          buff: ie buffer
return:          pointer of ie report, NULL if failed
*/
td_custom_ie_report_t* tpi_wifi_get_custom_ie(char *buff, int buff_len);

/*
function:           tpi_wifi_set_custom_ie 
description:        add or update a custom ie in iee80211 frame
input:              osif: wifi interface
                    mac: mac address of myself
                    type: 0~0xfe, type defined by product
                    msg: messages to spread
                    msg_len: length of msg
output:             
return:             -1:fail, 0:ok
*/
int tpi_wifi_set_custom_ie(const char* osif, const char* mac, char type, const char* msg, int msg_len);

/*
function:          tpi_wifi_del_custom_ie 
description:       delete a custom ie in iee80211 frame
input:             osif: wifi interface
                   type: type defined by product, 0xff for all
output:            
return:            -1:fail, 0:ok
*/
int tpi_wifi_del_custom_ie(const char* osif, char type);

/*
function:          tpi_wifi_is_apmode 
description:      is bss apmode 
input:             osif: wifi interface
output:            
return:            1:apmode , 0:not apmode,like sta mode
*/
int tpi_wifi_is_apmode(char *osif);

/*
function:          tpi_wifi_iwconfig_is_assoced 
description:      is bss access point assoced 
input:             osif: wifi interface
output:            
return:            1:assoc , 0:not assoc
*/
int tpi_wifi_iwconfig_is_assoced(char *osif);

/*
function:         tpi_wifi_advanced_scan 
description:      scan with some options
input:            osif: wifi interface
                  arg : scan options
output:            
return:           0: success; others: failed
*/
int tpi_wifi_advanced_scan(char *osif, advanced_scan_arg_t *arg);

/*
function:       tpi_easymesh_get_info
description:    get easymesh info of osif
input:      osifname 
            opmode
            data:user data
            len:user input data length
output:     data
return:     output data len
*/
int tpi_easymesh_get_info(const char *osif, easymesh_op_e opmode, void *data, int len);

void netlink_wds_msg_process(char *ifname);

int tpi_is_extend_intf(char *osif);

int tpi_wifi_isp_event_ioctl(int type, void *data, int data_len, char *ifname);

int tpi_wifi_advance_scan_out(char *osif, ap_info_t *aplist, int num, unsigned char count,  advanced_scan_out_arg_t *arg);

/*
function:        tpi_wifi_get_extend_rssi 
description:     get current extend_rssi dBm
input:           osif: wifi interface
output:          rssi: rssi value
return:          -1:fail, 0:ok
*/
int tpi_wifi_get_extend_rssi(char *osif, int *rssi);

/*
function   :    tpi_get_dual_apclient_ifname
description:    get apclient ifname
input      :    ifname
output:     
return     :    ok:0, false: -1
*/
int tpi_get_dual_apclient_ifname(char *ifname, int *length);

#ifdef CONFIG_TENDA_TEMP_SOLUTION
int tpi_wifi_get_acs_enable(char *osifname);
int tpi_wifi_get_interface_status(char *ifname, interface_stats_t *status);
#endif

#endif
