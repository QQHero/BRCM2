/*****************************************************************************
 文件名    : kwb_ioctl.h
 命名风格  : 
 文件功能说明  ：定义ioctl参数，供wifibase驱动应用层同时共用，可以对外使用
 修改历史  :
*****************************************************************************/

#ifndef __KWB_IOCTL_H__
#define __KWB_IOCTL_H__

#define MAC_LENGTH         6
#define INPUT_MAC_LENGTH   17
#define STALIST_COUNT      64
#define PROBE_FRAME_SIZE  (512+24)

#define TRUE    1
#define FALSE   0

/* buf length */
#define W_BUF_LEN_8     8
#define W_BUF_LEN_12    12
#define W_BUF_LEN_16    16
#define W_BUF_LEN_18    18
#define W_BUF_LEN_32    32
#define W_BUF_LEN_48    48
#define W_BUF_LEN_64    64
#define W_BUF_LEN_128   128
#define W_BUF_LEN_256   256
#define W_BUF_LEN_512   512

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define IF_MAX          2
#define WIFI_BSS_MAX    8

#ifndef WIFIBASE_SSID_LEN
#define WIFIBASE_SSID_LEN 32        /* 二进制数据长度，不做编码转换 */
#endif

#ifndef WIFIBASE_SSID_UTF8_LEN
#define WIFIBASE_SSID_UTF8_LEN 48        /* UTF8 ssid长度 */
#endif


#ifndef WIFIBASE_SSID_ENCLEN
#define WIFIBASE_SSID_ENCLEN 8
#endif

#ifndef WIFIBASE_NETTPFE_LEN
#define WIFIBASE_NETTPFE_LEN 8
#endif

#ifndef WIFIBASE_NCTRLSB_LEN
#define WIFIBASE_NCTRLSB_LEN 8
#endif

#ifndef WIFIBASE_SECURITY_LEN
#define WIFIBASE_SECURITY_LEN 32
#endif

#ifndef WIFIBASE_SCANRES_CAPLEN
#define WIFIBASE_SCANRES_CAPLEN 8
#endif

#ifndef WIFIBASE_CHANLIST_SIZE
#define WIFIBASE_CHANLIST_SIZE 64
#endif


#define BAND_WIDTH_20         20         //20M频宽
#define BAND_WIDTH_40         40         //40M频宽
#define BAND_WIDTH_80         80         //80M频宽
#define BAND_WIDTH_160        160        //160M频宽

#define CH_MIN_2G_CHANNEL            1u     /* Min channel in 2G band */
#define CH_MAX_2G_CHANNEL            14u    /* Max channel in 2G band */
#define CH_MIN_5G_LOWCHANNEL         36u    /* Min channel in 5G band */
#define CH_MAX_5G_LOWCHANNEL         48u    /* Min channel in 5G band */
#define CH_MIN_5G_HIGCHANNEL         149u   /* Min channel in 5G band */
#define CH_MAX_5G_HIGCHANNEL         165u   /* Min channel in 5G band */

#ifndef WIFIBASE_COUNTRY_CODE_LEN 
#define WIFIBASE_COUNTRY_CODE_LEN 4
#endif

#ifndef WIFIBASE_2G_MAX_CHNNUM
#define WIFIBASE_2G_MAX_CHNNUM 14
#endif

#define WIFIBASE_IOCTL_MAXLEN       (128 * 1024)
#define WIFIBASE_IOCTL_MINLEN       (2 * 1024)
#define WIFIBASE_IOCTL_SMALL_LEN    (128)
#define MAX_AP_NUM                  (128 * 2)

#define DFS_CAC_CHECK_STATUS  1 //当前为CAC检测状态
#define DFS_NON_CAC_STATUS    0 //当前非CAC检测状态


typedef enum kwb_return_value_e {
    WIFIBASE_UNSUPPORT    = -4,
    WIFIBASE_OUTRANGE     = -3,
    WIFIBASE_NULL_POINTER = -2,
    WIFIBASE_ERROR        = -1,
    WIFIBASE_OK           =  0,
}kwb_return_value_t;

typedef enum kwb_wifi_bandtype{
    KWB_BAND_INVALID = 0,
    KWB_BAND_2G      = 2,
    KWB_BAND_5G      = 5,
    KWB_BAND_6G      = 6,
    KWB_BAND_ALL     = 13
}kwb_wifi_bandtype_e;

typedef enum kwb_nettype {
    WIFI_NETTYPE_11B  = 0x01,
    WIFI_NETTYPE_11G  = 0x02,
    WIFI_NETTYPE_11A  = 0x04,
    WIFI_NETTYPE_11N  = 0x08,
    WIFI_NETTYPE_11AC = 0x10,
    WIFI_NETTYPE_11AX = 0x20
} kwb_nettype_e;

typedef enum kwb_channel {
    CHANNEL_NUM_AUTO   = 0,
    CHANNEL_NUM_1      = 1,
    CHANNEL_NUM_2      = 2,
    CHANNEL_NUM_3      = 3,
    CHANNEL_NUM_4      = 4,
    CHANNEL_NUM_5      = 5,
    CHANNEL_NUM_6      = 6,
    CHANNEL_NUM_7      = 7,
    CHANNEL_NUM_8      = 8,
    CHANNEL_NUM_9      = 9,
    CHANNEL_NUM_10     = 10,
    CHANNEL_NUM_11     = 11,
    CHANNEL_NUM_12     = 12,
    CHANNEL_NUM_13     = 13,
    CHANNEL_NUM_36     = 36,
    CHANNEL_NUM_40     = 40,
    CHANNEL_NUM_42     = 42,
    CHANNEL_NUM_44     = 44,
    CHANNEL_NUM_48     = 48,
    CHANNEL_NUM_52     = 52,
    CHANNEL_NUM_56     = 56,
    CHANNEL_NUM_60     = 60,
    CHANNEL_NUM_64     = 64,
    CHANNEL_NUM_149    = 149,
    CHANNEL_NUM_153    = 153,
    CHANNEL_NUM_155    = 155,
    CHANNEL_NUM_157    = 157,
    CHANNEL_NUM_161    = 161,
    CHANNEL_NUM_165    = 165
} kwb_channel_e;

typedef enum kwb_bandwidth {
    KWB_BWIDTHINVALID = 0,
    KWB_BWIDTH5 = 5,
    KWB_BWIDTH10 = 10,
    KWB_BWIDTH20 = 20,
    KWB_BWIDTH30 = 30,
    KWB_BWIDTH40 = 40,
    KWB_BWIDTH80 = 80,
    KWB_BWIDTH160 = 160,
    KWB_BWIDTH80_80 = 8080,
} kwb_bandwidth_e;

typedef enum kwb_wpa_type
{
    WPA_WPA,
    WPA_WPA2,
    WPA_WPA2_MIXED
} kwb_wpa_type_e;

typedef enum kwb_security_type
{
    SECURITY_NONE,
    SECURITY_WEP,
    SECURITY_WPAPSK,
    SECURITY_WPA,
    SECURITY_8021X
} kwb_security_type_e;

typedef enum kwb_wep_type
{
    WEP_OPEN,
    WEP_SHARED,
    WEP_FLAG = -1  /* 标识WEP加密 */
} kwb_wep_type_e;

typedef enum kwb_wepkey_format
{
    WEP_ASCII,
    WEP_HEX
} kwb_wepkey_format_e;

typedef enum kwb_wpapsk_type
{
    PSK_DISABLED,
    PSK_WPAPSK,
    PSK_WPA2PSK,
    PSK_WPAPSK_WPA2PSK_MIXED,
    PSK_WPA3SAE,
    PSK_WPA3SAE_WPA2PSK_MIXED,
    PSK_WPAOWE
} kwb_wpapsk_type_e;

typedef enum kwb_wpa_crypto
{
    CRYPTO_AES,
    CRYPTO_TKIP,
    CRYPTO_AES_MIXED
} kwb_wpa_crypto_e;
    
typedef enum kwb_scan_6g_type
{
    SCAN_6G_RNR_CHANNELS        = 1,
    SCAN_6G_PSC_CHANNELS        = 2,
    SCAN_6G_ALL_CHANNELS        = 4
} kwb_scan_6g_type_e;


typedef struct security_wep
{
    kwb_wep_type_e type;
    int keyselect;
    kwb_wepkey_format_e key_format[4];
    char key[4][26 + 1];
} security_wep_t;

typedef struct security_wpapsk
{
    kwb_wpapsk_type_e type;
    kwb_wpa_crypto_e crypto;
    char key[64 + 1];
    int rekey_time;
} security_wpapsk_t;

typedef struct security_wpa
{
    kwb_wpa_type_e type;
    kwb_wpa_crypto_e crypto;
    char radius_ip[W_BUF_LEN_16];
    int radius_port;
    char radius_key[W_BUF_LEN_64 + 1];
    int rekey_time;
} security_wpa_t;

typedef struct security_8021x
{
    char radius_ip[W_BUF_LEN_16];
    int radius_port;
    char radius_key[W_BUF_LEN_64 + 1];
} security_8021x_t;

typedef union kwb_security
{
    security_wep_t wep;
    security_wpapsk_t wpapsk;
    security_wpa_t wpa;
    security_8021x_t d8021x;
} kwb_security_u;

typedef struct kwb_advanced_scan_arg_s {
    /* d11 scan type. 0: active; 1: passive */
     unsigned char scan_type;
    /* duration before switch to the next channel. unit: ms. */
    unsigned short interval;
    /* if passive_scan is zero, probe request will be sent during scanning */
    unsigned char passive_scan;
    /* count of channel_list, zero means scan all available channels */
    unsigned int channel_count;
    /* channels to scan */
    unsigned char channel_list[WIFIBASE_CHANLIST_SIZE];
    /* ssid to be added to probe request if it's not empty */
    char ssid[WIFIBASE_SSID_UTF8_LEN + 1];
    unsigned int def_6g_scan_type;
    int wait_time;
    unsigned long long filter;
    unsigned char nprobes;
} kwb_advanced_scan_arg_t;

typedef struct kwb_acs_scan_arg_s {
    /* count of channel_list, zero means scan all available channels */
    unsigned int channel_count;
    /* channels to scan */
    unsigned char channel_list[WIFIBASE_CHANLIST_SIZE];

    int wait_time;
} kwb_acs_scan_arg_t;

typedef struct kwb_channel_list_s {
    int num;
    int channel[WIFIBASE_CHANLIST_SIZE];
} kwb_channel_list_t;

typedef enum kwb_sideband_flag {
    KWB_SIDEBAND_DEFAULT,   /*default*/
    KWB_SIDEBAND_NONE,      /*none*/
    KWB_SIDEBAND_UPPER,     /*upper*/
    KWB_SIDEBAND_LOWER      /*lower*/
} kwb_sideband_flag_e;

typedef struct kwb_chan_list_arg_s {
    kwb_bandwidth_e bw;             /*当前带宽*/
    kwb_sideband_flag_e sb; /*当前带宽的边带*/
    char country_code[WIFIBASE_COUNTRY_CODE_LEN];    /*当前国家码*/
} kwb_chan_list_arg_t;

typedef struct kwb_channel_switch_arg_s {
    /* Which channel do you want to switch */
    unsigned char chanel;
    /* csa notify count */
    unsigned int notify_count;
    /* Which bandwidth do you want to switch */
    kwb_bandwidth_e bw;
} kwb_channel_switch_arg_t;

typedef struct spectrum_info_s
{
   int channel;
   int bss_num;
   int noisefloor;
   int chanload;
} kwb_spectrum_info_t;

typedef struct kwb_ap_info_s {
    char ssid[WIFIBASE_SSID_LEN + 1];        /*驱动二进制格式，不做编码转换*/
    char ssid_utf8[WIFIBASE_SSID_LEN + 1];   /*编码转换后的长度，GB2312每个汉字需要2个字节，转换成UTF8需要(32/2)*3=48字节*/
    char ssid_encode[WIFIBASE_SSID_ENCLEN];    /*utf-8,gb2312*/
    unsigned char mac[MAC_LENGTH];
    unsigned char nettype;
    kwb_channel_e channel;
    kwb_bandwidth_e bandwidth;     /*20,40,80,auto*/
    kwb_sideband_flag_e nctrlsb;          /*none,lower,upper*/
    kwb_security_u security;
    short signal;
    unsigned short capability;       /*AD-hoc,AP*/
    unsigned int extflag;        /* 扫描结果标记 */
} kwb_ap_info_t;

typedef struct kwb_ap_list_s {
    unsigned int count;
    unsigned int chan_num;
    kwb_ap_info_t ap[MAX_AP_NUM];
    kwb_spectrum_info_t chan_interfer[WIFIBASE_CHANLIST_SIZE];
} kwb_ap_list_t;

typedef struct kwb_bss_status_s {
    unsigned char bss_idx;
    unsigned char bss_en;
} kwb_bss_status_t;

typedef struct kwb_sta_info_s {
    unsigned char mac[MAC_LENGTH];
    int rssi;
    unsigned int txrate;
    unsigned int rxrate;
    int link_time;
    int noise;
    int link_rate;
    unsigned long long tx_bytes;
    unsigned long long rx_bytes;
}kwb_sta_info_t;

typedef struct td_sta_info {
    unsigned char ether_addr_octet[MAC_LENGTH];
    char ifname[IFNAMSIZ];
    int rssi;
    int sup11k;
    int sup11v;
    unsigned long last_time; /* 记录STA最近一次连接/断开的时间 */
} td_sta_info_t;

typedef struct kwb_chan_info_s {
    unsigned int channel;
    unsigned int chan_busy;
    kwb_bandwidth_e bandwidth;
    kwb_sideband_flag_e side_flag;       /*none,lower,upper*/
    char country_code[WIFIBASE_COUNTRY_CODE_LEN]; /*当前国家码*/
} kwb_chan_info_t;

typedef struct kwb_dfs_status_s{
    unsigned int state;           /*dfs cac 检测状态 */
    unsigned int duration;        /*cac静默时间 */
} kwb_dfs_status_t;

typedef enum kwb_cmd {
    /** sta_info , range:       0 - 999   **/
    WIFIBASE_CMD_STA_INFO_BEGIN     = 0,
    WIFIBASE_CMD_GET_STA_INFO       = 1 + WIFIBASE_CMD_STA_INFO_BEGIN,
    WIFIBASE_CMD_GET_AP_TX,
    WIFIBASE_CMD_SET_STA_MONITOR,
    WIFIBASE_CMD_GET_STA_MONITOR,
    WIFIBASE_CMD_BEACON_REQ,
    WIFIBASE_CMD_BSS_TRANS_REQ,

    /** chan_info ,range:    1000 - 1999  **/
    WIFIBASE_CMD_CHAN_INFO_BEGIN    = 1000,
    WIFIBASE_CMD_GET_CHAN_INFO      = 1 + WIFIBASE_CMD_CHAN_INFO_BEGIN,

    WIFIBASE_CMD_GET_OSIF,
    WIFIBASE_CMD_CHANNEL_SWITCH,
    WIFIBASE_CMD_SCAN,
    WIFIBASE_CMD_GET_SCANRESULTS,
    WIFIBASE_CMD_ADVANCE_SCAN_OUT,
    WIFIBASE_CMD_SET_CHANNEL,
    WIFIBASE_CMD_GET_CHANNEL_SPECTRUM,
    WIFIBASE_CMD_GET_CHANNEL_LIST,
    WIFIBASE_CMD_GET_TXRX_ERR,
    WIFIBASE_CMD_GET_BW_CAP,
    WIFIBASE_CMD_GET_WLAN_INFO,
    WIFIBASE_CMD_GET_STA_NUM,

    WIFIBASE_CMD_GET_OS_IFNAME,         /* mibif转osif */
    WIFIBASE_CMD_GET_MIB_IFNAME,        /* osif转mibif */
    WIFIBASE_CMD_GET_DFS_RADAR,         /*获取雷达信号信息*/
    WIFIBASE_CMD_GET_SCANSTATE,
    WIFIBASE_CMD_GET_BANDTYPE,
    WIFIBASE_CMD_GET_CAC_STATUS,
    WIFIBASE_CMD_GET_ASSOCLIST,
    WIFIBASE_CMD_SET_DFS_FORCE_SWITCH_CHANBW,

    /* bss get/set */
    WIFIBASE_CMD_BSS_BEGIN          = 1200,
    WIFIBASE_CMD_GET_SSID           = 1 + WIFIBASE_CMD_BSS_BEGIN,
    WIFIBASE_CMD_GET_STA_MAXNUM,
    WIFIBASE_CMD_KICK_STA,
    WIFIBASE_CMD_GET_AP_CAP,
    WIFIBASE_CMD_CHECK_STA_ONLINE,
    WIFIBASE_CMD_GET_BSSID,
    WIFIBASE_CMD_GET_TD_STA_INFO,
    WIFIBASE_CMD_GET_RADIO_STATIS,
    WIFIBASE_CMD_GET_IF_STATIS,
    WIFIBASE_CMD_GET_BSS_STATUS,
    WIFIBASE_CMD_GET_BSSMODE,           /*represent bss current workmode*/
    WIFIBASE_CMD_GET_BSS_TYPE,           /* 获取接口类型 */
    WIFIBASE_CMD_SET_BSS_HIDE,
    WIFIBASE_CMD_SET_BSS_ACL,
    /* easymesh cmds */
    WIFIBASE_CMD_GET_EASYMESH       = 2000,

    /* steer cmds */
    WIFIBASE_CMD_STEER_MAPS         = 2050,     /* steer maps ioctl */
    WIFIBASE_CMD_SET_BSSLOAD        = 2051,     /* set fake bssload */
    WIFIBASE_CMD_ADD_RRM_NBR        = 2052,     /* add nbr info */
    WIFIBASE_CMD_DEL_RRM_NBR        = 2053,     /* del nbr info */
#ifdef CONFIG_TENDA_GAME_SPEEDUP
    WIFIBASE_CMD_WME_SET_AC         = 2054,
    WIFIBASE_CMD_WME_GET_AC         = 2055,
#endif
    WIFIBASE_CMD_FIX_RATE           = 2060,
    WIFIBASE_CMD_GET_STA_CHANIM_INFO    = 2061,
    WIFIBASE_CMD_SET_SPECIAL_STA    = 2062,
    WIFIBASE_CMD_GET_SPECIAL_STA    = 2063,
    WIFIBASE_CMD_SET_RTSTHRESH      = 2064,
    WIFIBASE_CMD_SET_FEATURE_INFO   = 2065,
    WIFIBASE_CMD_GET_AP_CHANIM_INFO    = 2066,
    WIFIBASE_CMD_GET_DLYSTATS       = 2067,
    /* ADPTVTY_CONTROL cmd */
    WIFIBASE_CMD_SET_ADAPTIVITY     = 3000,
} kwb_cmd_e;

typedef struct kwb_adaptivity_info {
    char adaptvty_type[W_BUF_LEN_8];
    char enable;
    int edcca_threshold;
}kwb_adaptivity_info_t;

typedef struct kwb_info_s {
    char mib_name[IFNAMSIZ];
    long data_ptr;
    unsigned int data_len;
} kwb_base_info_t;

typedef struct kwb_em_info {
    unsigned int  mode;
    int           input_len;
    int           output_len;
    char         *data;
}kwb_em_info_t;


/* 柔性数组，用于多实例数据带回数据长度 */
typedef struct kwb_data {
    int len;
    char data[0];
}kwb_data_t;

typedef struct pack_info {
    unsigned int  unit;   /* device instance number */
    int  rxerr;
    int  txerr;
} pack_info_t;

typedef struct bw_cap {
    unsigned int cap_bwidth20;
    unsigned int cap_bwidth40;
    unsigned int cap_bwidth80;
    unsigned int cap_bwidth160;
    unsigned int cap_bwidth8080;
} bw_cap_t;

typedef struct kwb_ap_cap_s {
    int en_dot11k;
    int en_dot11v;
} kwb_ap_cap_t;

typedef struct kwb_dfs_radar {
    unsigned int channel;    //雷达信号所在信道
    unsigned int radar_type;   //雷达信号类型
    unsigned int nop_time;    //非占用时间
    unsigned char dfs_chan_flag;
} kwb_dfs_radar_t;

typedef struct sta_monitor_msg {
    int rssi;
    unsigned char ether_addr_octet[MAC_LENGTH];
} sta_monitor_msg_t;

typedef struct beacon_req_params {
    unsigned int channel;
    unsigned char sta_mac[MAC_LENGTH];
} beacon_req_params_t;

typedef struct bss_trans_req_params {
    /* 要迁移的客户端mac */
    unsigned char sta_mac[MAC_LENGTH];
    /* 要迁移到的目标bssid */
    unsigned char bssid[MAC_LENGTH];
    /* 要迁移到的目标信道 */
    unsigned char channel;
} bss_trans_req_params_t;

typedef struct kwb_radio_info_s{
    unsigned long long txframe;
    unsigned long long txbyte;
    unsigned long long txerror;
    unsigned long long txdropped;
    unsigned long long rxframe;
    unsigned long long rxbyte;
    unsigned long long rxerror;
    unsigned long long rxdropped;
}kwb_radio_info_t;

typedef struct kwb_if_info_s{
    unsigned long long txframe;
    unsigned long long txbyte;
    unsigned long long txerror;
    unsigned long long radio_txdropped;
    unsigned long long rxframe;
    unsigned long long rxbyte;
    unsigned long long rxerror;
    unsigned long long radio_rxdropped;
}kwb_if_info_t;

typedef struct kwb_nbr_info_s {
    /*os_ifname*/
    char os_ifname[IFNAMSIZ];
    /* bssid */
    unsigned char bssid[MAC_LENGTH];
    /* 信道 */
    unsigned char channel;
    /*频段*/
    unsigned char band;
    /* op class */
    unsigned char op_class;
    unsigned int  ssid_len;
    unsigned char ssid[WIFIBASE_SSID_LEN + 1];
    unsigned int  chanspec;
} kwb_nbr_info_t;

typedef struct kwb_dfs_forced_params_s
{
    unsigned char channel;
    kwb_bandwidth_e bandwidth;
} kwb_dfs_forced_params_t;

#ifndef WLAN_INFO
#define WLAN_INFO
typedef struct kwb_wlan_info_s
{
    unsigned char mac[MAC_LENGTH];
    int txrate;
    int rxrate;
    int rssi;
    int channel;
    int chan_busy;
}kwb_wlan_info_t;
#endif

#ifdef CONFIG_TENDA_GAME_SPEEDUP

#define IP_ID 1
#define AC_QUE_LEN 4

typedef enum wb_identify{
    DEFAULT_IDENT  = 0,
    IP_IDENT       = 1,
}wb_identify_e;

typedef enum wb_action{
    DEL_INFO        = 0,
    ADD_INFO        = 1,
    RESET_INFO        = 2,
}wb_action_e;

typedef struct wb_ip_stream_info_s
{
    unsigned short  srcport;       //portnum
    unsigned int    srcip;
    unsigned short  dstport;       //portnum
    unsigned int    dstip;
    unsigned int    protocol;       //协议
}wb_ip_stream_info_t;

typedef struct wb_wmm_qos_info_s
{
    unsigned char   priority;   //0~7  802.1d
    unsigned int    stream_id;     //哪种标志
    unsigned int    action;
    union {
        wb_ip_stream_info_t ip_info;
    } stream;
}wb_wmm_info_t;

/* WL_RSPEC defines for rate information */
#define WB_RSPEC_RATE_MASK		0x000000FF      /* rate or HT MCS value */
#define WB_RSPEC_HE_MCS_MASK		0x0000000F      /* HE MCS value */
#define WB_RSPEC_HE_NSS_MASK		0x000000F0      /* HE Nss value */
#define WB_RSPEC_HE_NSS_SHIFT		4               /* HE Nss value shift */
#define WB_RSPEC_VHT_MCS_MASK		0x0000000F      /* VHT MCS value */
#define WB_RSPEC_VHT_NSS_MASK		0x000000F0      /* VHT Nss value */
#define WB_RSPEC_VHT_NSS_SHIFT		4               /* VHT Nss value shift */
#define WB_RSPEC_TXEXP_MASK		0x00000300
#define WB_RSPEC_TXEXP_SHIFT		8
#define WB_RSPEC_BW_MASK		0x00070000      /* bandwidth mask */
#define WB_RSPEC_BW_SHIFT		16              /* bandwidth shift */
#define WB_RSPEC_STBC			0x00100000      /* STBC encoding, Nsts = 2 x Nss */
#define WB_RSPEC_TXBF			0x00200000      /* bit indicates TXBF mode */
#define WB_RSPEC_LDPC			0x00400000      /* bit indicates adv coding in use */
#define WB_RSPEC_SGI			0x00800000      /* Short GI mode */
#define WB_RSPEC_ENCODING_MASK		0x03000000      /* Encoding of Rate/MCS field */
#define WB_RSPEC_OVERRIDE_RATE		0x40000000      /* bit indicate to override mcs only */
#define WB_RSPEC_OVERRIDE_MODE		0x80000000      /* bit indicates override rate & mode */

/* WB_RSPEC_ENCODING field defs */
#define WB_RSPEC_ENCODE_RATE	0x00000000      /* Legacy rate is stored in RSPEC_RATE_MASK */
#define WB_RSPEC_ENCODE_HT	0x01000000      /* HT MCS is stored in RSPEC_RATE_MASK */
#define WB_RSPEC_ENCODE_VHT	0x02000000      /* VHT MCS and Nss is stored in RSPEC_RATE_MASK */
#define WB_RSPEC_ENCODE_HE	0x03000000      /* HE MCS and Nss is stored in RSPEC_RATE_MASK */

/* WB_RSPEC_BW field defs */
#define WB_RSPEC_BW_UNSPECIFIED 0
#define WB_RSPEC_BW_20MHZ       0x00010000
#define WB_RSPEC_BW_40MHZ       0x00020000
#define WB_RSPEC_BW_80MHZ       0x00030000
#define WB_RSPEC_BW_160MHZ      0x00040000

#define WB_RSPEC_HE_GI_MASK	0x00000C00	/* HE GI indices */
#define WB_RSPEC_HE_GI_SHIFT	10
#define WB_HE_GI_TO_RSPEC(gi)	(((gi) << WB_RSPEC_HE_GI_SHIFT) & WB_RSPEC_HE_GI_MASK)
#define CEIL(x, y)		(((x) + ((y) - 1)) / (y))

enum wb_rate_set{
    WIFIBASE_CMD_AUTO_RATE = 0,
    WIFIBASE_CMD_FIX_LEGACY = 1,
    WIFIBASE_CMD_FIX_HT = 2,
    WIFIBASE_CMD_FIX_VHT = 3,
    WIFIBASE_CMD_FIX_HE = 4,
};

typedef struct wb_feature_info{
    unsigned int speed_traffic; //ac队列通道（0-7）
}wb_feature_info_t;

typedef struct wb_fixrate_info_s
{
    unsigned int mode_set ;
    unsigned int rate ;
    unsigned int mcs ;
    unsigned int nss ;
    unsigned int tx_exp ;
    unsigned int bw ;
    unsigned int stbc ;
    unsigned int ldpc ;
    unsigned int sgi ;
}wb_fixrate_info_t;

typedef struct wb_dlystats_info_s
{
    unsigned char   mac[MAC_LENGTH];//客户端地址
    unsigned int    total_ac_pkts[AC_QUE_LEN];
    unsigned int    lost_ac_pkts[AC_QUE_LEN]; 
    unsigned int    avg_mac_delay[AC_QUE_LEN];
    unsigned int    max_mac_delay[AC_QUE_LEN];
    unsigned int    mac_delay_dist1_count[AC_QUE_LEN];
    unsigned int    mac_delay_dist2_count[AC_QUE_LEN];
    unsigned int    mac_delay_dist3_count[AC_QUE_LEN];
    unsigned int    mac_delay_dist4_count[AC_QUE_LEN];
    unsigned int    mac_delay_dist5_count[AC_QUE_LEN];
}wb_dlystats_info_t;


typedef struct wb_sta_chanim_info_s
{
    unsigned char   mac[MAC_LENGTH];//客户端地址
    unsigned char   air_use;
    unsigned int    mcs;
    unsigned int    stream_priority;   //0-7 bk - vo
    unsigned int    rssi; //无线信号强度
    unsigned int    ps_cnt;
    unsigned int    rts_tx_cnt;
    unsigned int    rts_failed_cnt;
    unsigned int    requested;
    unsigned int    txfailed;
    unsigned int    retried;
    unsigned int    utlsn;
    unsigned int    data_rate;
    unsigned int    phy_rate;
    unsigned int    nss;
}wb_sta_chanim_info_t;

typedef struct wb_ap_chanim_info_s
{
    unsigned char   txop;
    unsigned int    noise_level;
    unsigned int    brokensignal_count;
    unsigned int    sta_count;
    unsigned int    overall_datarate;
    unsigned int    sta_status;
    unsigned int    bandwidth;
    unsigned int    channel;
    unsigned int    chan_busy;
    unsigned int    idle_rate;
}wb_ap_chanim_info_t;


typedef struct wb_special_sta_e {
    unsigned char           ea[6];
    unsigned char           action; //add 1 delete 2
    unsigned char           idlerate_thresh[2];
    signed char             rssi_thresh[2];
    unsigned int            utils_thresh[2];
    unsigned char           timer_windows;
}wb_special_sta_t;

#endif

#endif
