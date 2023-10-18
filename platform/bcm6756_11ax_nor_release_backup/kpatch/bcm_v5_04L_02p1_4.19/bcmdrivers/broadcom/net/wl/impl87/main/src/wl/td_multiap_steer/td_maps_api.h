/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Sunjiajun
  生成日期   : 2019年8月
  最近修改   :

  功能描述   : 本模块提供给应用层的接口定义. 应用层(steerd)与本模块(td_multiap_steer)
            会包含此头文件. 原厂相关文件不要包含此头文件. 应用层, 本模块, 和原厂
            源文件 均需要包含的定义 应该放到 td_maps_public.h 中.

            修改本文件内定义的接口后, 应同时更改 MAPS_API_VERSION 版本号.
            以便及时检查出接口不一致造成的问题.
******************************************************************************/

#ifndef _TD_MULTIAP_STEER_API_H
#define _TD_MULTIAP_STEER_API_H
#include "td_maps_public.h"
#include <linux/netlink.h>

#ifdef __KERNEL__
#include <linux/kernel.h>

#else /* __KERNEL__ */
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>
/* for KM_NETLINK_WLAN */
#include "km_extern.h"

#endif /* __KERNEL__ */

#define MAPS_NETLINK KM_NETLINK_WLAN

#define RSSI_TOP 0
#define RSSI_BOTTOM (-100)

#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN 6
#endif

#ifndef MACSTR
#define MACSTR      "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef MAC2STR
#define MAC2STR(a)  (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif

#ifndef MAPS_SSID_LEN
#define MAPS_SSID_LEN 32
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef TBIT
#define TBIT(x) (1 << (x))
#endif

#define maps_test_bit(cap, bits) (0 != ((cap) & (bits)))
#define maps_set_bit(cap, bits) ((cap) |= (bits))
#define maps_reset_bit(cap, bits) ((cap) &= ~(bits))

/**
 * 用于检查应用层与驱动使用的API接口是否一致.
 * 当本文件内定义的接口发生变更时, 应更改 MAPS_API_VERSION 版本号.
*/
#define MAPS_API_VERSION "MAPS_API_V2"

/**
 * 由于目前使用u_int32_t类型存储需要拒绝的bss, 
 * 索引0不被使用, 因此总共最多支持(32-1)个bss(vap)
*/
#define MAPS_MAX_BSS_INDEX 31
/* 最多支持3张网卡 */
#define MAPS_MAX_RADIO_INDEX 3
/* 每张网卡最多支持8个vap */
#define MAPS_VAP_PER_RADIO 8
/* 最多处理32个Beacon Report */
#define MAPS_MAX_BEACON_REP 32

/**
 * 由于未赋值时 bss_index, radio_index 默认值均为0. 
 * 为避免混淆, 规定 bss_index, radio_index 有效索引值从1开始.
*/
/* 检查bss索引是否合法, 第0个元素不被使用 */
#define maps_valid_bss_index(index) ((index) > 0 && (index) <= MAPS_MAX_BSS_INDEX)
/* 检查radio索引是否合法, 第0个元素不被使用 */
#define maps_valid_radio_index(index) ((index) > 0 && (index) <= MAPS_MAX_RADIO_INDEX)

/**
 * 遍历所有BSS, 遍历索引区间[1, MAPS_MAX_BSS_INDEX]
 * 
 * pos: BSS相关结构体指针, 将指向当前遍历的BSS相关结构体
 * array: BSS相关结构体指针数组, 数组长度为(MAPS_MAX_BSS_INDEX + 1)
 * i: int类型, 存放索引值的临时变量
*/
#define maps_for_each_bss(pos, array, i) \
for ((i) = 1; (i) <= MAPS_MAX_BSS_INDEX && ((pos) = (array)[(i)], 1); (i)++)

/**
 * 遍历所有RADIO, 遍历索引区间[1, MAPS_MAX_RADIO_INDEX]
 * 
 * pos: RADIO相关结构体指针, 将指向当前遍历的RADIO相关结构体
 * array: RADIO相关结构体数组, 数组长度为(MAPS_MAX_RADIO_INDEX + 1)
 * i: int类型, 存放索引值的临时变量
*/
#define maps_for_each_radio(pos, array, i) \
for ((i) = 1; (i) <= MAPS_MAX_RADIO_INDEX && ((pos) = &(array)[(i)], 1); (i)++)

#define maps_for_each_radio_multiap(pos, array, i) \
for ((i) = 1; (i) <= MAPS_MAX_RADIO_INDEX && ((pos) = ((array) + (i))); (i)++)

typedef enum maps_band_shift {
    MAPS_BAND_UNKNOWN = 0,
    MAPS_2G_SHIFT,
    MAPS_5L_SHIFT,
    MAPS_5H_SHIFT,
    MAPS_6G_SHIFT,
    MAPS_BAND_NUM
} maps_band_shift_e;

#define maps_valid_band(band) ((band) > MAPS_BAND_UNKNOWN && (band) < MAPS_BAND_NUM)

typedef enum maps_band_prefer {
    MAPS_PREFER_NONE,
    MAPS_PREFER_2G,
    MAPS_PREFER_5G,
    MAPS_PREFER_6G,
    MAPS_PREFER_DUAL_BAND,/*双频旧规格，随机选择5G与2.4G*/
} maps_band_prefer_e;

typedef enum maps_band {
    MAPS_IG = 1 << MAPS_BAND_UNKNOWN,/*illegal band*/
    MAPS_2G = 1 << MAPS_2G_SHIFT,
    MAPS_5L = 1 << MAPS_5L_SHIFT,
    MAPS_5H = 1 << MAPS_5H_SHIFT,
    MAPS_6G = 1 << MAPS_6G_SHIFT,
} maps_band_e;

typedef enum maps_cap {
    MAPS_CAP_HT  = 1<<0,
    MAPS_CAP_VHT = 1<<1,
    MAPS_CAP_RRM = 1<<2,
    MAPS_CAP_BTM = 1<<3,
    MAPS_CAP_HE  = 1<<4,
} maps_cap_e;

#define MAPS_5G_ALL     MAPS_5L | MAPS_5H

#define IS_2G(bandtype)         (MAPS_2G == bandtype ? true : false)
#define IS_5L(bandtype)         (MAPS_5L == bandtype ? true : false)
#define IS_5H(bandtype)         (MAPS_5H == bandtype ? true : false)
#define IS_6G(bandtype)         (MAPS_6G == bandtype ? true : false)
/*bandtype capabilities*/
#define CAP_2G(bandtype)         (MAPS_2G & bandtype ? true : false)
#define CAP_5G(bandtype)         ((MAPS_5G_ALL) & bandtype ? true : false)
#define CAP_6G(bandtype)         (MAPS_6G & bandtype ? true : false)

typedef enum maps_band_mode {
    /* 不拒绝认证 */
    MAPS_BAND_ACCEPT,
    /* 拒绝认证, 用于pre-association offload steering. */
    MAPS_BAND_BLOCK,
    /* 当rssi在阈值范围内时拒绝认证, 用于pre-association upgrade/downgrade steering. */
    MAPS_BAND_BLOCK_RSSI
} maps_band_mode_e;

typedef enum maps_block_mode {
    /* 按照maps_band_mode_e判断是否拒绝客户端连接.  用于pre-association steering. */
    MAPS_BLOCK_DEFAULT,
    /* 拒绝客户端连接特定的BSS, 用于post-association steering. */
    MAPS_BLOCK_BSS,
    /* 不拒绝客户端连接, 用于保护名单 */
    MAPS_BLOCK_DISBALE,
} maps_block_mode_e;

#ifdef TD_STEER_SUPPORT_6G
static inline maps_band_shift_e maps_frequency_to_band(unsigned int freq)
{
    /**
     * 频段与信道频率范围映射表, 格式:
     * {频段flag, 信道频率上限, 信道频率下限}
    */
    unsigned int freq_map2band[][3] = {
        {MAPS_2G_SHIFT, 2412, 2484},//1-14
        {MAPS_5L_SHIFT, 5180, 5320},//36-64
        {MAPS_5H_SHIFT, 5500, 5885},//100-177
        {MAPS_6G_SHIFT, 5935, 7115},//1-233
    };
    unsigned int *map_item = NULL;
    int i;

    for (i = 0; i < ARRAY_SIZE(freq_map2band); i++) {
        map_item = freq_map2band[i];
        if (freq >= map_item[1] && freq <= map_item[2]) {
            return map_item[0];
        }
    }

    return MAPS_BAND_UNKNOWN;
}
#else
static inline maps_band_shift_e maps_channel_to_band(unsigned char channel)
{
    /**
     * 频段与信道映射表, 格式:
     * {频段flag, 频段下限信道, 频段上限信道}
    */
    unsigned char band_map[][3] = {
        {MAPS_2G_SHIFT, 1, 14},
        {MAPS_5L_SHIFT, 36, 64},
        {MAPS_5H_SHIFT, 100, 196}
    };
    unsigned char *map_item = NULL;
    int i;
    
    for (i = 0; i < ARRAY_SIZE(band_map); i++) {
        map_item = band_map[i];
        if (channel >= map_item[1] && channel <= map_item[2]) {
            return map_item[0];
        }
    }

    return MAPS_BAND_UNKNOWN;
}
#endif

/*========================= 通讯消息头部定义 ======================== */
#define MAPS_MSG_MAGIC_LEN 7
#define MAPS_MSG_MAGIC "steering"

typedef struct maps_msg_head {
    u_int8_t magic[MAPS_MSG_MAGIC_LEN];
    u_int8_t type;
    u_int8_t msg[0];
} maps_msg_head_t;

/*=============== netlink 通讯定义, 用于驱动向应用层通知 ==============*/
#define MAPS_NETLINK_GROUP 1

/* NETLINK events */
typedef enum maps_ev {
    /* 应用层发送, 检验netlink是否畅通 */
    MAPS_EV_PING,
    /* 驱动回复, 存放当前API接口的版本号(即 td_maps_api.h 的校验和) */
    MAPS_EV_PONG,
    /* 客户端关联 */
    MAPS_EV_STA_JOIN,
    /* 客户端解除关联 */
    MAPS_EV_STA_LEAVE,
    /* 客户端支持某个频段 */
    MAPS_EV_STA_BAND,
    /* 客户端状态(rssi, active, load)更新 */
    MAPS_EV_STA_STATUS,
    /* 收到Beacon Report */
    MAPS_EV_BEACON_REP,
    /* 收到BSS Transmition Response */
    MAPS_EV_BSS_TRANS,
    /* 拒绝客户端连接 */
    MAPS_EV_REJECT,
    /* radio状态(如: 信道, 信道占用率)更新 */
    MAPS_EV_RADIO_STATUS,
    /* vap的配置可能被更改 */
    MAPS_EV_VAP_UPDATE,
} maps_ev_e;

typedef struct maps_msg_sta {
    u_int8_t mac[MAC_ADDR_LEN];
    /* 客户端连接的BSS */
    u_int8_t bss_index;
    /* 客户端支持的频段 */
    maps_band_e bands;
    /* 客户端支持的协议 */
    maps_cap_e caps;
    /* 客户端信号强度, -100dBm~0dBm */
    int8_t rssi;
    /* 客户端流数量 */
    u_int8_t nss;
    /* 客户端占用的空口时间, 0%~100% */
    u_int8_t load;
    /* 索引为 bss_index 的BSS上所连接的客户端数量 */
    u_int8_t sta_num;
    /* 此客户端当前是否存在网络活动 */
    bool active;
} maps_msg_sta_t;

typedef struct maps_msg_radio {
    u_int8_t radio_index;
    u_int8_t channel;
    u_int8_t load;
    u_int8_t sta_num;
    maps_band_e work_band;
} maps_msg_radio_t;

typedef struct maps_msg_reject {
    u_int8_t mac[MAC_ADDR_LEN];
    u_int8_t bss_index;
    u_int8_t count;
    int8_t rssi;
} maps_msg_reject_t;

typedef struct maps_msg_bss_trans_rsp {
    /* 客户端mac地址 */
    u_int8_t mac[MAC_ADDR_LEN];
    u_int8_t bss_index;
    /* 报文返回状态, 0为接受 */
    u_int8_t status;
} maps_msg_bss_trans_rsp_t;

typedef struct maps_beacon_rep {
    u_int8_t op_class;
    u_int8_t channel;
    int8_t rcpi;
    int8_t rsni;
    u_int8_t bssid[MAC_ADDR_LEN];
} maps_beacon_rep_t;

typedef struct maps_msg_beacon_rep {
    u_int8_t sta[MAC_ADDR_LEN];
    u_int8_t bss_index;
    u_int8_t count;
    maps_beacon_rep_t reports[MAPS_MAX_BEACON_REP];
} maps_msg_beacon_rep_t;

typedef struct maps_msg_bss_update {
    u_int8_t bss_index;
    u_int8_t bssid[MAC_ADDR_LEN];
} maps_msg_bss_update_t;

/*================== ioctl 通讯定义, 用于应用层向驱动下发配置 ===============*/
#define MAPS_PONG  "pong"
#define MAPS_PONG_LEN  4

typedef enum maps_cmd {
    /* 检验ioctl是否正常 */
    MAPS_CMD_PING,
    /* 设置bss索引 */
    MAPS_CMD_SET_BSS,
    /* 设置sta配置 */
    MAPS_CMD_SET_STA,
    /* 设置配置 */
    MAPS_CMD_SET_CONFIG,
    /* 为某个radio添加bss */
    MAPS_CMD_ADD_RADIO,
    /* 设置radio配置 */
    MAPS_CMD_SET_RADIO,
    /* 打印配置信息 */
    MAPS_CMD_DUMP_CONFIG,
    /* 打印各radio的状态及配置 */
    MAPS_CMD_DUMP_RADIO, 
    /* 打印各客户端的状态信息 */
    MAPS_CMD_DUMP_STA,
    /* 清除所有状态 */
    MAPS_CMD_CLEAN,
    /* 设置debug开关 */
    MAPS_CMD_DBG,
    /* 发送BSS Transition Request */
    MAPS_CMD_BSS_TRANS,
    /* 发送Beacon Request */
    MAPS_CMD_BEACON_REQ,
} maps_cmd_e;

typedef struct maps_msg_bss_conf {
    /* index取值范围: 0~MAPS_MAX_BSS_INDEX. 若index值为0, 此bss上连接的客户端不会被迁移 */
    u_int8_t index;
    /* 此BSS所在的ESS使用了哪些radio, 以radio索引值按位设置 */
    u_int32_t ess_radios;
} maps_msg_bss_conf_t;

/* 用于下发各频段配置 */
typedef struct maps_msg_radio_conf {
    u_int8_t index;
    maps_band_mode_e mode;
    /* MAPS_BAND_BLOCK_RSSI模式下, 当RSSI在[reject_rssi_lower, reject_rssi_upper]区间时, 拒绝认证. */
    int8_t reject_rssi_upper;
    int8_t reject_rssi_lower;
    /* 当客户端RSSI不在[monitor_rssi_low, monitor_rssi_high]区间时, 通知应用层.*/
    int8_t monitor_rssi_high;
    int8_t monitor_rssi_low;
} maps_msg_radio_conf_t;

/* 用于设定物理网卡索引值 */
typedef struct maps_msg_radio_add {
    u_int8_t index;
} maps_msg_radio_add_t;

typedef struct maps_msg_conf {
    /* 总开关 */
    bool enable;
    /* 是否允许拒绝回复probe response */
    bool enable_reject_probe;
    /* 是否允许拒绝认证 */
    bool enable_reject_auth;
    /* probe request(客户端支持频段)信息的老化时间 */
    u_int32_t probe_age;
    /* reject_age秒内连续拒绝客户端reject_max_cnt次后放行 */
    u_int8_t reject_age;
    u_int8_t reject_max_cnt;
    /* 客户端连续sta_active_interval秒内流量低于sta_active_flow(byte/s)则认为是idle状态 */
    u_int8_t active_slot;
    u_int32_t active_flow;

    bool enable_delay_reject_probe;
    bool enable_delay_reject_auth;
    u_int32_t delay_reject_age;     //s
    u_int8_t  delay_reject_stanum;
    /*steerd mode, 0 : dual_band; 1 : tri_band*/
    bool mode;
} maps_msg_conf_t;

typedef struct maps_msg_sta_conf {
    u_int8_t mac[MAC_ADDR_LEN];
    maps_block_mode_e mode;
    /* 应该拒绝此客户端认证的bss, 按位取bss索引值. */
    u_int32_t block_bss;
} maps_msg_sta_conf_t;

typedef struct maps_msg_bss_trans_req {
    /* 要迁移的客户端mac */
    u_int8_t sta[MAC_ADDR_LEN];
    /* 要迁移到的目标bssid */
    u_int8_t bss[MAC_ADDR_LEN];
    /* 要迁移到的目标信道 */
    unsigned char channel;
} maps_msg_bss_trans_req_t;

typedef struct maps_msg_beacon_req {
    char ssid[MAPS_SSID_LEN + 1];
    u_int8_t sta[MAC_ADDR_LEN];
    u_int8_t channel;
} maps_msg_beacon_req_t;


#endif /* _TD_MULTIAP_STEER_API_H */
