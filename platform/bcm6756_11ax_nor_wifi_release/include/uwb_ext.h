/*****************************************************************************
 文件名    : uwb_ext.h
 命名风格  : 
 文件功能说明  ：wifibase的应用接口的定义，可以包含该头文件，只供应用层使用
 修改历史  :
*****************************************************************************/
#ifndef __UWB_EXT_H__
#define __UWB_EXT_H__

#include "kwb_ioctl.h"

#define UWB_MAX_CHANNELS_NUM            32
#define UWB_SCAN_IGNORE_DATA_FILE         "/etc/acbs_config/%s_full_scan_ignore.txt"
#define UWB_SCAN_DATA_FILE                 "/etc/acbs_config/%s_fullscan.txt"
#define UWB_ACBS_DATA_FILE                 "/etc/acbs_config/acbs.txt"

/* 与驱动对齐，总长度不大于 255 -1(frage_type) - 6(指定目的mac才回该ie)
   = 248，但最大长度还取决于无线驱动中buffer长度, 以及管理帧长度的限制,
   实际最大长度可能小于255,建议写入长度不要超过 64 字节 */
#define UWB_PRIVATE_IE_MAX_IE_SIZE 248

/* private ie模块阀值结构体 */
typedef struct uwb_private_ie_threshold {
    unsigned int maxbeacontxentry;
    unsigned char maxbeacontxconcurrent;
    unsigned int maxproberxentry;
    unsigned char maxproberespentry;
} uwb_private_ie_threshold_t;

/* private ie模块端口创建规则结构体 */
typedef struct uwb_private_ie_port_rule {
    /* 端口, 建议上层一个功能使用一个port，取值[0,128) */
    unsigned char port_num;
    /* 打上该标记的功能port，ie数据完全由用户控制，可以与其他厂商互通 */
    unsigned char raw_ie;
    /* 校验幻数, 避免不同产品功能用到相同的 port 造成冲突,为兼容现有驱动模块，配置了raw_ie也需要设置该值，取值[0,256) */
    unsigned char magic;
    /* 端口并行发送数据包的无线接口 */
    char ifnames[W_BUF_LEN_32];
    /* 创建时由应用层指定,用于指定iedata的前id_len个字节为ID，为兼容现有驱动模块，配置了raw_ie也需要设置该值，取值(0,256) */
    unsigned char id_len;
} uwb_private_ie_port_rule_t;

/* private ie模块厂商信息规则结构体 */
typedef struct uwb_private_ie_oui_rule {
    unsigned char band;
    unsigned char oui[3];
} uwb_private_ie_oui_rule_t;

/* wifi频段枚举类型 */
typedef enum uwb_band_type_enum {
    UWB_2G_BAND,
    UWB_5GL_BAND,
    UWB_5GH_BAND,
    UWB_6G_BAND,
} uwb_band_type_e;

/* private ie模块端口发包规则结构体 */
typedef struct uwb_private_ie_packet_tx_rule {
    unsigned char port_num;
    /* 指定的mac才回应该ie信息，可选 */
    unsigned char mac[MAC_LENGTH];
    /* 指定的帧类型才回该ie信息 */
    unsigned char frame_type;
    unsigned char iedata_len;
    char iedata[UWB_PRIVATE_IE_MAX_IE_SIZE];
} uwb_private_ie_packet_tx_rule_t;

/* 帧类型枚举结构 */
typedef enum uwb_frame_type {
    UWB_FRAME_TYPE_PROBE_REQ,
    UWB_FRAME_TYPE_BEACON,
    UWB_FRAME_TYPE_PROBE_RES,
    UWB_FRAME_TYPE_ASSOC_REQ,
    UWB_FRAME_TYPE_ASSOC_RES,
    UWB_FRAME_TYPE_AUTH,
    UWB_FRAME_TYPE_UNKNOW,
} uwb_frame_type_e;

typedef enum kwb_ssid_encode {
    UWB_SSID_ENCODE_UTF8   = 0,
    UWB_SSID_ENCODE_GB2312 = 1
} kwb_ssid_encode_e;

typedef struct chanscores_info
{
    unsigned char num;
    unsigned char chan[UWB_MAX_CHANNELS_NUM];
    unsigned char chanlevel[UWB_MAX_CHANNELS_NUM];
    unsigned int chanscore[UWB_MAX_CHANNELS_NUM];
}chanscores_info_t;

/* ap scan results */
typedef struct uwb_ap_info
{
    char ssid[W_BUF_LEN_48 + 1];        /* 12个中文字符以上，gb2312导致转换成utf-8数据溢出 */
    char ssid_encode[W_BUF_LEN_8];      /* utf-8,gb2312 */
    char mac[W_BUF_LEN_18];
    char nettype[W_BUF_LEN_12];         /* b,g,bg,bgn,bgn+ac,bgn+ac+ax;a,an,an+ac,an+ac+ax */
    char channel[W_BUF_LEN_8];
    char bandwidth[W_BUF_LEN_8];        /* 20,40,80,auto */
    char nctrlsb[W_BUF_LEN_8];          /* none,lower,upper */
    char signal[W_BUF_LEN_8];
    char security[W_BUF_LEN_32];        /* none,wep,wpa&wpa2/aes */
    char capability[W_BUF_LEN_8];       /* AD-hoc,AP */
} uwb_ap_info_t;

/*****************************************************************************
 函 数 名  : uwb_get_chan_info
 功能描述  : 通过IOCTL向驱动发送一条消息
 输入参数  : ifname：获取chan_info的接口

 输出参数  : chan_info
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2020年9月16日
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_chan_info(const char *mib_name, kwb_chan_info_t *chan_info);

/*****************************************************************************
 函 数 名  : uwb_get_cac_status
 功能描述  : 通过IOCTL向驱动发送一条消息,获取当前dfs检测状态
 输入参数  : mib_name：获取chan_info的接口

 输出参数  : chan_info
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2022年5月17日
    作    者   : yangnana
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_cac_status(const char *mib_name, kwb_dfs_status_t *cac_status_info);

/*****************************************************************************
 函 数 名  : uwb_get_sta_info
 功能描述  : 通过IOCTL向驱动发送一条消息
 输入参数  : ifname：获取sta_info的接口

 输出参数  : sta_info
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2020年12月14日
    作    者   : bcz
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_sta_info(const char *mib_name, unsigned char * mac, kwb_sta_info_t *sta_info, int num, int *out_num);

/*****************************************************************************
 函 数 名  : uwb_get_td_sta_info
 功能描述      : 获取某个STA的信息，加上td_前缀，与SDK自带的sta_info区分开
 输入参数      : mib_name : 接口名

 输出参数      : td_sta_info : STA信息，包括rssi、sup11k、sup11v、last_time等

 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月22日
    作    者   : pangjialian
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_td_sta_info(const char *mib_name, td_sta_info_t *sta_info);

/*****************************************************************************
 函 数 名  : uwb_scan_out
 功能描述  : :应用层 扫描
 输入参数  : ifname:接口
           cfg :扫描参数
 输出参数  : 
 返 回 值  : 失败 -1；成功：0
 修改历史      :
  1.日    期   : 2020年9月17日
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_scan_out(const char *mib_name, kwb_advanced_scan_arg_t *scfg, kwb_ap_info_t *aplist, int num, int *out_num);

/*****************************************************************************
 函 数 名  : uwb_scan
 功能描述      : AP扫描
 输入参数      : mib_name : 接口mib名
             scfg : 扫描参数，包括扫描类型、信道扫描之间的时间间隔、
                    扫描信道数、信道列表等

 输出参数      : 无
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年02月22日
    作    者   : pangjialian
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_scan(const char *mib_name, kwb_advanced_scan_arg_t *scfg);
/*****************************************************************************
 函 数 名  : uwb_get_scanresults
 功能描述      : 获取AP扫描结果
 输入参数      : mib_name : 接口mib名
             max_num : AP数量最大值
 输出参数      : aplist : AP列表，存放所有AP的信息
             act_num : 实际扫描得到的AP数量
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年02月22日
    作    者   : pangjialian
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_scanresults(const char *mib_name, kwb_ap_list_t *aplist);
/*****************************************************************************
 函 数 名  : uwb_get_channel_spectrum
 功能描述  : :应用层 信道干扰查询
 输入参数  : ifname:接口
             cfg ：参数
 输出参数  : 
 返 回 值  : 失败 -1；成功：0
 修改历史      :
  1.日    期   : 2020年9月17日
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_channel_spectrum(const char *mib_name, kwb_acs_scan_arg_t *cfg, kwb_spectrum_info_t *info, int num, int *out_num);

/*****************************************************************************
 函 数 名  : uwb_channel_switch
 功能描述  : :csa通知
 输入参数  : ifname:接口
              cfg ：csa :参数
 输出参数  : 
 返 回 值  : 失败 -1；成功：0
 修改历史      :
  1.日    期   : 2020年9月17日
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_channel_switch(const char *mib_name, kwb_channel_switch_arg_t *cfg);

/*****************************************************************************
 函 数 名  : uwb_get_channel_list
 功能描述  : :获取信道列表
 输入参数  : mib_name :接口mib名
           arg : 获取信道列表的参数。为空默认获取所有的列表
           list:信道列表
 输出参数  : 
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2020年9月17日
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_channel_list(const char *mib_name, kwb_chan_list_arg_t *arg, kwb_channel_list_t *list);

/*****************************************************************************
 函 数 名  : uwb_easymesh_get_info
 功能描述  : :获取easymesh信息
 输入参数  : mib_name :接口mib名
           opmode : easymesh进程需要拿什么信息
           data:用于存放上报的信息
           data_len:data存放的长度
 输出参数  : 
 返 回 值  : 小于0:失败  大于等于0 成功：返回值为数据有效长度
 修改历史      :
  1.日    期   : 2021年1月27日
    作    者   : yinjiazheng
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_easymesh_get_info(const char *mib_name, int opmode, void *data, int data_len);

/*****************************************************************************
 函 数 名  : uwb_get_txrx_err
 功能描述  : :获取信道列表
 输入参数  : mib_name :接口mib名

 输出参数  : info :无线txrx 错包数
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年02月03日
    作    者   : haungyongjie
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_txrx_err(const char *mib_name, pack_info_t *info);

/*****************************************************************************
 函 数 名  : uwb_get_bw_cap
 功能描述  : :获取各频宽能力
 输入参数  : mib_name :接口mib名

 输出参数  : info :bw_cap_t
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年02月05日
    作    者   : haungyongjie
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_bw_cap(const char *mib_name, bw_cap_t *bw_cap);

/*****************************************************************************
 函 数 名  : uwb_get_scanstate
 功能描述  : :获取扫描状态
 输入参数  : mib_name :接口mib名

 输出参数  : info :state
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年11月18日
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_scanstate(const char *mib_name, int *state);

/*****************************************************************************
 函 数 名  : uwb_get_ssid
 功能描述  : 获取SSID
 输入参数  : mib_name
            arg 用户参数
            
 输出参数  : ssid
 返 回 值  : 成功 WIFIBASE_OK 小于0
 修改历史      :
  1.日    期   : 2021年2月21日
    作    者   : liuke
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_ssid(const char *mib_name, void *data, int data_en);

/*****************************************************************************
 函 数 名  : uwb_get_sta_maxnum
 功能描述  : 获取允许的STA最大数量
 输入参数  : mib_name         
 输出参数  : 无
 返 回 值  : 允许的STA最大数量, 0表示出错
 修改历史      :
  1.日    期   : 2021年2月22日
    作    者   : liuke
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_sta_maxnum(const char *mib_name);

/*****************************************************************************
 函 数 名  : uwb_get_mib_ifname
 功能描述  : 根据逻辑接口获取系统接口名称
 输入参数  : os_name        
 输出参数  : mibif
 返 回 值  : 成功 WIFIBASE_OK 小于0
 修改历史      :
  1.日    期   : 2021年2月22日
    作    者   : liuke
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_mib_ifname(const char *os_name, void *data, int data_en);

/*****************************************************************************
 函 数 名  : uwb_get_os_ifname
 功能描述  : 根据逻辑接口获取系统接口名称
 输入参数  : mib_name        
 输出参数  : osif
 返 回 值  : 成功 WIFIBASE_OK 小于0
 修改历史      :
  1.日    期   : 2021年2月21日
    作    者   : liuke
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_os_ifname(const char *mib_name, void *data, int data_en);

/*****************************************************************************
 函 数 名  : uwb_steer_maps
 功能描述  : :获取信道列表
 输入参数  : mib_name :接口mib名

 输出参数  : ioctl数据
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年02月07日
    作    者   : liuke
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_steer_maps(const char *mib_name, void *data, unsigned int data_en);

/*****************************************************************************
 函 数 名  : uwb_kick_sta
 功能描述  : : 踢掉客户端
 输入参数  :   mib_name :接口mib名
            mac
            mac_len
 输出参数  : 无
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月03日
    作    者   : liyahui1
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_kick_sta(const char *mib_name, void *mac, unsigned int mac_len);

/*****************************************************************************
 函 数 名  : uwb_get_ap_cap
 功能描述  : 获取ap能力信息
 输入参数  : mib_name：获取ap_cap的接口

 输出参数  : ap_cap
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2021年3月04日
    作    者   : liyahui
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_ap_cap(const char *mib_name, kwb_ap_cap_t *ap_cap);

/*****************************************************************************
 函 数 名  : uwb_set_bssload
 功能描述  : :设置beacon中的bssload，即信道利用率，用于提升BTM切换成功率
 输入参数  : mib_name :接口mib名
            load: 预设值
 输出参数  : 无
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月6日
    作    者   : liuke
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_set_bssload(const char *mib_name, int load);

/*****************************************************************************
 函 数 名  : uwb_check_sta_online
 功能描述  : : 判断STA是否在线
 输入参数  :   mib_name :接口mib名
            mac
            mac_len
 输出参数  : 无
 返 回 值  : 在线:1, 不在线: 0
 修改历史      :
  1.日    期   : 2021年03月08日
    作    者   : liuke
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_check_sta_online(const char *mib_name, void *mac, unsigned int mac_len);

/*****************************************************************************
 函 数 名  : uwb_get_dfs_radar_status
 功能描述  : 获取信道中是否存在雷达信号
 输入参数  : const char *mib_name         
             kwb_dfs_radar_t *radar_info  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2021年3月4日
    作    者   : ynn
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_get_dfs_radar_status(const char *mib_name, kwb_dfs_radar_t *radar_info);

/*****************************************************************************
 函 数 名  : uwb_set_sta_monitor
 功能描述      : 使能监听，并且添加某个STA到监听列表
 输入参数      : mib_name : 接口名
             sta_msg : STA参数信息，包括MAC、RSSI等
 输出参数      : 无

 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月08日
    作    者   : pangjialian
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_set_sta_monitor(const char *mib_name, sta_monitor_msg_t *sta_msg);

/*****************************************************************************
 函 数 名  : uwb_get_sta_monitor
 功能描述      : 获取某个STA的信息，包括RSSI等
 输入参数      : mib_name : 接口名

 输出参数      : sta_msg : STA参数信息，包括MAC、RSSI等

 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月08日
    作    者   : pangjialian
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_sta_monitor(const char *mib_name, sta_monitor_msg_t *sta_msg);

/*****************************************************************************
 函 数 名  : uwb_beacon_req
 功能描述      : 对sta发送Beacon Request, 让sta知道ap的信息
 输入参数      : mib_name : 接口名
             br_params: Beacon Request输入参数

 输出参数      : 无

 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月15日
    作    者   : pangjialian
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_beacon_req(const char *mib_name, beacon_req_params_t *br_params);

/*****************************************************************************
 函 数 名  : uwb_bss_trans_req
 功能描述      : 对sta发送BSS Transition Request
 输入参数      : mib_name : 接口名
             btr_params: BSS Transition Request输入参数

 输出参数      : 无

 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月15日
    作    者   : pangjialian
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_bss_trans_req(const char *mib_name, bss_trans_req_params_t *btr_params);

/*****************************************************************************
 函 数 名  : uwb_get_bssid
 功能描述  : : 获取指定接口的bssid
 输入参数  :   mib_name :接口mib名
            mac
            mac_len
 输出参数  : 无
 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月10日
    作    者   : liyahui
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_bssid(const char *mib_name, void *mac, unsigned int mac_len);

/*****************************************************************************
 函 数 名  : uwb_get_radio_statis
 功能描述      : 获取radio级别的信息
 输入参数      : mib_name : 接口名

 输出参数      : radioinfo : 参数信息

 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月08日
    作    者   : shijianhong
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_radio_statis(const char *mib_name, kwb_radio_info_t *radioinfo);

/*****************************************************************************
 函 数 名  : uwb_get_if_statis
 功能描述      : 获取vap即 无线接口 的信息
 输入参数      : mib_name : 接口名

 输出参数      : radioinfo : 参数信息

 返 回 值  : 失败 小于0；成功：0
 修改历史      :
  1.日    期   : 2021年03月13日
    作    者   : shijianhong
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_if_statis(const char *mib_name, kwb_if_info_t *if_info);

/*****************************************************************************
 函 数 名  : uwb_add_rrm_neighbor
 功能描述  : 设置无线接口的neighber信息
 输入参数  : mib_name：无线接口
           nbr_info：邻居信息
 输出参数  : 
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2021年4月14日
    作    者   : liyahui
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_add_rrm_neighbor(const char *mib_name, kwb_nbr_info_t *nbr_info);

/*****************************************************************************
 函 数 名  : uwb_del_rrm_neighbor
 功能描述  : 删除无线接口的neighber信息
 输入参数  : mib_name：无线接口
           mac
 输出参数  : 
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2021年4月14日
    作    者   : liyahui
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_del_rrm_neighbor(const char *mib_name, void *mac, unsigned int mac_len);

/*****************************************************************************
 函 数 名  : uwb_get_chanscore
 功能描述  : 获取信道评分
 输入参数  : mib_name：无线接口
           chanscores：信道评分信息指针
           return_now: 立即返回标识
 输出参数  : 
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2021年5月14日
    作    者   : huangyongjie
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_chanscore(const char *mib_name, char *csa_en, int return_now, chanscores_info_t *chanscores);

/*****************************************************************************
 函 数 名  : uwb_get_chanscore
 功能描述  : 获取信道评分
 输入参数  : mib_name：无线接口
           chanscores：信道评分信息指针
 输出参数  : 
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2021年5月14日
    作    者   : huangyongjie
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_bss_status(const char *mib_name, kwb_bss_status_t *bss_status);

/*****************************************************************************
 函 数 名  : uwb_get_band_stainfo
 功能描述  : 获取radio的所有sta信息
 输入参数  : mib_name：无线接口
 输出参数  : sta_info, out_num
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2021年6月19日
    作    者   : huangyongjie
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_band_stainfo(const char *mib_name, kwb_sta_info_t *sta_info, int get_sta_num, int *out_num);

/*****************************************************************************
 函 数 名  : uwb_private_ie_enable
 功能描述  : 使能private ie模块
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败:WIFIBASE_ERROR
 
 修改历史      :
  1.日    期   : 2021年6月17日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_private_ie_enable();

/*****************************************************************************
 函 数 名  : uwb_private_ie_disable
 功能描述  : 关闭private ie模块
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败:WIFIBASE_ERROR
 
 修改历史      :
  1.日    期   : 2021年6月17日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_private_ie_disable();

/*****************************************************************************
 函 数 名  : uwb_private_ie_get_threshold
 功能描述  : private ie模块阀值获取接口
 输入参数  : uwb_private_ie_threshold_t *threshold  
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败:WIFIBASE_NULL_POINTER
 
 修改历史      :
  1.日    期   : 2021年6月17日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_private_ie_get_threshold(uwb_private_ie_threshold_t *threshold);

/*****************************************************************************
 函 数 名  : uwb_private_ie_create_port
 功能描述  : 创建一个端口，建议：一个功能使用一个端口
 输入参数  : uwb_private_ie_port_rule_t *port_rule  
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败:WIFIBASE_NULL_POINTER WIFIBASE_ERROR
 
 修改历史      :
  1.日    期   : 2021年6月19日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_private_ie_create_port(uwb_private_ie_port_rule_t *port_rule);

/*****************************************************************************
 函 数 名  : uwb_private_ie_destory_port
 功能描述  : 销毁一个端口
 输入参数  : uwb_private_ie_port_rule_t *port_rule  
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败:WIFIBASE_NULL_POINTER WIFIBASE_ERROR
 
 修改历史      :
  1.日    期   : 2021年6月19日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_private_ie_destory_port(uwb_private_ie_port_rule_t *port_rule);

/*****************************************************************************
 函 数 名  : uwb_private_ie_port_start_packet_tx
 功能描述  : 启用port_num功能端口，在frame_type类型的数据包，携带iedata字段
 输入参数  : uwb_private_ie_packet_tx_rule_t *tx_rule  
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败:WIFIBASE_NULL_POINTER WIFIBASE_ERROR
 
 修改历史      :
  1.日    期   : 2021年6月19日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_private_ie_port_start_packet_tx(uwb_private_ie_packet_tx_rule_t *tx_rule);

/*****************************************************************************
 函 数 名  : uwb_private_ie_add_oui_rule
 功能描述  : 下发用户指定的oui信息，并指定处理该oui的band信息
 输入参数  : uwb_private_ie_oui_rule *oui_rule  
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败:WIFIBASE_NULL_POINTER WIFIBASE_ERROR
 
 修改历史      :
  1.日    期   : 2021年7月14日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_private_ie_add_oui_rule(uwb_private_ie_oui_rule_t *oui_rule);

/*****************************************************************************
 函 数 名  : uwb_private_ie_del_oui_rule
 功能描述  : 删除用户指定的oui信息
 输入参数  : uwb_private_ie_oui_rule *oui_rule  
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败:WIFIBASE_NULL_POINTER WIFIBASE_ERROR
 
 修改历史      :
  1.日    期   : 2021年7月14日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_private_ie_del_oui_rule(uwb_private_ie_oui_rule_t *oui_rule);

/*****************************************************************************
 函 数 名  : uwb_get_bssmode
 功能描述  : 获取指定无线接口的工作模式(ap/sta)
 输入参数  : mib_name : 接口名
    status : 状态值
 输出参数  : status：AP代表ap模式，STA代表sta模式
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 
 修改历史      :
  1.日    期   : 2021年12月27日
    作    者   : wanggang
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_get_bssmode(const char *mib_name, char *status);

/*****************************************************************************
 函 数 名  : uwb_get_bandtype
 功能描述  : 通过IOCTL向驱动发送一条消息
 输入参数  : mib_name：获取band type的接口

 输出参数  : band type
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2022年2月28日
    作    者   : hyj
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_bandtype(const char *mib_name, char *band);

/*****************************************************************************
 函 数 名  : uwb_set_adaptivity
 功能描述  : 开启或关闭各认证方式的自适应功能
 输入参数  : const char *mib_name：接口名
            kwb_adaptivity_info_t *input_info：认证类型，使能开关，阈值

 输出参数  :
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2022年8月31日
    作    者   : liuqi
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_set_adaptivity(const char *mib_name, kwb_adaptivity_info_t *input_info);

/*****************************************************************************
 函 数 名  : uwb_dfs_force_switch_chanbw
 功能描述  : 配置AP检测到dfs雷达信号时强制切换到设定的信道频宽
 输入参数  : mib_name：接口对应的公共的wlan名
    cfg：需要设置的信道频宽
 输出参数  : 
 返 回 值  : 成功 WIFIBASE_OK；失败 WIFIBASE_ERROR
 修改历史      :
  1.日    期   : 2022年08月13日
    作    者   : liuwei
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_dfs_force_switch_chanbw(const char *mib_name, kwb_dfs_forced_params_t *cfg);

/*****************************************************************************
 函 数 名: uwb_get_bss_type
 功能描述  : 获取bss接口属性(nomal bss/fronthual bss/backhaul bss/backhaul sta)
 输入参数  : mib_ifname ：接口名
 输出参数  : bss_type : bss接口属性(nomal bss/fronthual bss/backhaul bss/backhaul sta)
 返 回 值: 成功 : WIFIBASE_OK 失败 : 小于0
 修改历史      :
  1.日    期   : 2022年09月21日
    作    者   : hzx
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_get_bss_type(const char *mib_ifname, char *bss_type);

#ifdef CONFIG_TENDA_GAME_SPEEDUP
/*****************************************************************************
 函 数 名  : uwb_add_wmm_qos_info
 功能描述  : 指定无线接口，通过IOCTL向驱动发送一条消息，添加无线qos信息
 输入参数  : mib_name : 接口名
    msg : 状态值
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_add_wmm_qos_info(const char *mib_name, wb_wmm_info_t *msg);

/*****************************************************************************
 函 数 名  : uwb_del_wmm_qos_info
 功能描述  : 指定无线接口，通过IOCTL向驱动发送一条消息，删除无线qos信息
 输入参数  : mib_name : 接口名
            msg:状态值
            msg->stream_id:流id(如ip流 为1)
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER

 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_del_wmm_qos_info(const char *mib_name, wb_wmm_info_t *msg);

/*****************************************************************************
 函 数 名  : uwb_reset_wmm_qos_info
 功能描述  : 指定无线接口，通过IOCTL向驱动发送一条消息，重置无线qos信息
 输入参数  : mib_name : 接口名
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_reset_wmm_qos_info(const char *mib_name);

/*****************************************************************************
 函 数 名  : uwb_get_wmm_qos_info
 功能描述  : 指定无线接口，通过IOCTL向驱动发送一条消息，获取无线qos信息
 输入参数  : mib_name : 接口名
 输出参数  : msg->priority: ac队列优先级
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_get_wmm_qos_info(const char *mib_name, wb_wmm_info_t * msg);

/*****************************************************************************
 函 数 名  : uwb_fix_rate
 功能描述  :    指定无线接口,设置固定的协商速率
 输入参数  :    mib_name : 接口名
            info : 特征信息，驱动根据此信息设置速率
            msg : 固定速率
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_fix_rate(const char *mib_name, int *msg,wb_feature_info_t *info);

/*****************************************************************************
 函 数 名  : uwb_get_sta_chanim_stats
 功能描述  :    指定无线接口,获取当前的sta相关的无线参数信息
 输入参数  :    mib_name : 接口名
            msg->mac ：客户端mac
            msg->stream_priority : 数据流在那个ac队列优先级中
 输出参数  :    msg : sta相关的无线参数信息
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_get_sta_chanim_stats(const char *mib_name, wb_sta_chanim_info_t *msg);

/*****************************************************************************
 函 数 名  : uwb_get_ap_chanim_stats
 功能描述  :    指定无线接口,获取当前ap口相关的无线参数信息
 输入参数  :    mib_name : 接口名

 输出参数  :   msg : AP口相关的无线参数信息
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_get_ap_chanim_stats(const char *mib_name, wb_ap_chanim_info_t *msg);

/*****************************************************************************
 函 数 名  : uwb_set_sta_params_info_of_flowcontrol_level
 功能描述  :    指定无线接口,设置 aeb算法中，触发sta上报流控等级的参数阈值
 输入参数  :    mib_name : 接口名
            msg : 各个参数阈值
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_set_sta_params_info_of_flowcontrol_level(const char *mib_name, wb_special_sta_t *msg);

/*****************************************************************************
 函 数 名  : uwb_get_sta_params_info_of_flowcontrol_level
 功能描述  :    指定无线接口,获取 aeb算法中，触发sta上报流控等级的参数阈值
 输入参数  :    mib_name : 接口名
 输出参数  :    msg : 各个参数阈值
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_get_sta_params_info_of_flowcontrol_level(const char *mib_name, wb_special_sta_t *msg);

/*****************************************************************************
 函 数 名  : uwb_set_sta_report_threshold
 功能描述  :    指定无线接口,设置rts阈值
 输入参数  :    mib_name : 接口名
            thresh : 阈值
 输出参数  : 无
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_set_rtsthresh(const char *mib_name, int thresh);

/*****************************************************************************
 函 数 名  : uwb_get_sta_dlystats
 功能描述  :    指定无线接口,获取sta在mac层中的数据延时信息
 输入参数  :    mib_name : 接口名
            msg->mac : 客户端mac
            
 输出参数  : msg ：所有的延时信息
 返 回 值  : 成功:WIFIBASE_OK 失败: WIFIBASE_ERROR/WIFIBASE_NULL_POINTER
 修改历史      :
  1.日    期   : 2022年11月3日
    作    者   : wanghan
    修改内容   : 新生成函数

*****************************************************************************/
int uwb_get_sta_dlystats(const char *mib_name, wb_dlystats_info_t *msg);


#endif


/*****************************************************************************
 函 数 名: uwb_set_bss_hide
 功能描述  : 设置bss接口隐藏
 输入参数  : mib_ifname ：接口名
           value : bss接口隐藏mib值
 输出参数  :
 返 回 值: 成功 : WIFIBASE_OK 失败 : 小于0
 修改历史      :
  1.日    期   : 2022年09月22日
    作    者   : liuwei
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_set_bss_hide(const char *mib_ifname, int bss_hide);

/*****************************************************************************
 函 数 名: uwb_set_bss_acl
 功能描述  : 设置bss接口mac过滤模式
 输入参数  : mib_ifname ：接口名
           value : bss接口mac过滤模式mib值
 输出参数  :
 返 回 值: 成功 : WIFIBASE_OK 失败 : 小于0
 修改历史      :
  1.日    期   : 2022年09月22日
    作    者   : liuwei
    修改内容   : 新生成函数
*****************************************************************************/
int uwb_set_bss_acl(const char *mib_ifname, int bss_macmode);

#endif
