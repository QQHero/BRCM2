/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Sunjiajun
  生成日期   : 2019年8月15日
  最近修改   : 2020.6.10 增加BCM方案

  功能描述   : 与应用层(steerd)配合使用. 实现客户端的认证拒绝, 状态监控, 事件的通知
            以及相关报文收发.

  其   它   : 返回值为int的函数, 未特殊说明下, 0为正常, 非0为失败.
            本文件内允许持有和传递vendor_sta_t, vendor_vap_t, adapt_sta_t类型的指针; 
            但禁止直接访问其内部结构. 访问其内部结构的代码应该放到方案相关的文件中.
******************************************************************************/
#include <net/sock.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/jhash.h>
#include <linux/jiffies.h>
#include <linux/netdevice.h>
#include <linux/random.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <net/wifi_common.h>
#include "td_maps_sf.h"
#include "td_multiap_steer.h"
#include "td_maps_api.h"


typedef enum delay_reject_type {
    DELAY_REJECT_PROBE = 0,
    DELAY_REJECT_AUTH,
} delay_reject_type_e;


/* 记录客户端信息的最大数量 */
#define MAPS_MAX_NODES              256
/* hash桶数量 */
#define MAPS_HASH_SIZE              128
#ifndef BROADCOM
/* 定时器触发周期, 单位: 秒 */
#define MAPS_TIMER_INTERVAL         2
#endif
/* 客户端RSSI变化超过`MAPS_RSSI_REP_DIFF`后会报告给应用层 */
#define MAPS_RSSI_REP_DIFF          3
/* 通报客户端状态的最大时间间隔, 单位:MAPS_TIMER_INTERVAL秒 */
#define MAPS_STA_CYCLE              5
/* 通报信道占用率的最大时间间隔, 单位:MAPS_TIMER_INTERVAL秒 */
#ifdef SIFLOWER
#define MAPS_RADIO_CYCLE            4
#else
#define MAPS_RADIO_CYCLE            8
#endif   
/* 最大IOCTL消息长度 */
#define MAPS_MAX_MSG_SIZE           512
/* 确保其他CPU对共享数据访问结束的等待时间, 单位 ms */
#define MAPS_CLEAN_DELAY            20
/* 一个既不太大也不太小(既不会触发Upgrade Steering 
    也不会触发 Downgrade Steering 或 AP Steering)的值.
    当无法获取到RSSI时, 使用此值代替, 避免误触发 */
#define MAPS_RSSI_DUMMY             -62

#define maps_timeout(start_time, seconds) \
time_after(jiffies, (start_time) + (seconds) * (HZ))

typedef maps_msg_conf_t maps_config_t;

typedef struct maps_sta {
    struct hlist_node node;
    unsigned char mac[MAC_ADDR_LEN];
    /* 客户端支持的频段 */
    maps_band_e bands;
    /* 是否阻止客户端连接 */
    maps_block_mode_e mode;
    /**
     * 应该拒绝此客户端认证的bss, 按位取bss索引值. 如0x3, 
     * 表示索引为1和2的bss应该拒绝认证. 因此,
     * bss最大索引值不能超过(sizeof(unsigned int) * 8 - 1)
    */
    unsigned int block_bss;
    /* 客户端当前连接的BSS索引 */
    unsigned char assoc_bss;
    /* 连续拒绝此客户端认证的次数 */
    unsigned char reject_cnt;
    /* 客户端在各频段上收到包的时间戳 */
    unsigned long band_time[MAPS_BAND_NUM];
    /* 上次拒绝此客户端认证的时间戳 */
    unsigned long reject_time;
    /* 客户端上次更新的时间戳 */
    unsigned long update_time;

    /* add begin by dengxingde, 2020-04-20, 原因: 用户2G延时接入 */
    /* 客户端创建的时间戳 */
    unsigned long create_time;

    bool hold_delay;
    /* add end by dengxingde, 2020-04-20 */

    /* 客户端是否支持11n */
    bool ht;
    /* 客户端是否支持11ac */
    bool vht;
    /* 客户端是否支持11ax */
    bool he;
    /* 客户端是否已关联 */
    bool associated;
    /* 客户端连续一段时间内流量低于阈值视为idle, 否则active */
    bool active;
    /* 客户端上次active状态的时间戳 */
    unsigned long active_time;
    /* 存储多个信道占用率采样和, 用于取平均值 */
    unsigned int load_sum;
    /* 客户端流量造成的信道占用 */
    unsigned char load;
    /* 客户端上次广播的rssi */
    signed char last_rssi;
    /* 提供给适配层使用, 定义在 td_maps_<rtl|qca>.h */
    adapt_sta_t asta;
    /* 用于标记是否上报应用层 */ 
    bool is_report;
} maps_sta_t;

typedef struct maps_radio {
    maps_band_mode_e mode;
    /* 此工作的频段 */
    maps_band_e work_band;
    /* MAPS_BAND_BLOCK_RSSI模式下, 当RSSI在[reject_rssi_lower, reject_rssi_upper]区间时, 拒绝认证. */
    int8_t reject_rssi_upper;
    int8_t reject_rssi_lower;
    /* 当客户端RSSI不在[monitor_rssi_low, monitor_rssi_high]区间时, 通知应用层.*/
    int8_t monitor_rssi_high;
    int8_t monitor_rssi_low;
    /* 此radio的所有vap */
    vendor_vap_t* vaps[MAPS_VAP_PER_RADIO];
    /* 存储多个信道占用率采样和, 用于取平均值 */
    unsigned int load_sum;
    /* 客户端信道负载, 取值0~100 */
    unsigned char load;
    unsigned char channel;
    unsigned int sta_num;
    /* 用于标记是否上报应用层 */ 
    bool is_report;
   // unsigned char op_class;     /* operation class */
} maps_radio_t;

typedef struct maps_bss {
    vendor_vap_t *vap;
    u_int32_t ess_radios;
} maps_bss_t;

#ifndef SIFLOWER
/* SIFLOWER 方案2.4G和5G对同一份驱动加载两份，加载的空间相互独立，导致2.4G与5G重复创建
这些全局变量且不能相互访问，该方案下将这些变量定义在km_core.c中，通过extern方式调用，
实现2.4G与5G共用这些变量。*/
static struct sock *td_maps_nl_sock = NULL;    
static maps_radio_t td_maps_radios[MAPS_MAX_RADIO_INDEX + 1];  
static maps_sta_t td_maps_sta_list[MAPS_MAX_NODES];  
static maps_config_t td_maps_config = {.enable = false}; 
int td_maps_dbg = MAPS_LOG_ERROR;   
static struct hlist_head td_maps_sta_h_heads[MAPS_HASH_SIZE]; 
/* hash种子 */
static u_int32_t td_maps_hash_rnd;  
static maps_bss_t td_maps_mng_bss[MAPS_MAX_BSS_INDEX + 1];   
#endif


void maps_init_variable(void) {
    
    if (NULL == td_maps_radios) {
        td_maps_radios = (void *)kmalloc(sizeof(maps_radio_t) * (MAPS_MAX_RADIO_INDEX + 1), GFP_ATOMIC);
        if (NULL == td_maps_radios) {
            maps_err("td_maps_radios malloc failed!");
        }
    }
    
    if (NULL == td_maps_sta_list) {
        td_maps_sta_list = (void *)kmalloc(sizeof(maps_sta_t) * (MAPS_MAX_NODES), GFP_ATOMIC);
        if (NULL == td_maps_sta_list) {
            maps_err("td_maps_sta_list malloc failed!");
        }
    }
    
    if (NULL == td_maps_config) {
        td_maps_config = (void *)kmalloc(sizeof(maps_config_t), GFP_ATOMIC);
        if (NULL == td_maps_config) {
            maps_err("td_maps_config malloc failed!");
        }
    }

    ((maps_config_t *)td_maps_config)->enable = false;
    
    if (NULL == td_maps_mng_bss) {
        td_maps_mng_bss = (void *)kmalloc(sizeof(maps_bss_t)  * (MAPS_MAX_BSS_INDEX + 1), GFP_ATOMIC);
        if (NULL == td_maps_mng_bss) {
            maps_err("td_maps_mng_bss malloc failed!");
        }
    }
    
    td_maps_dbg = MAPS_LOG_ERROR;
}

static struct sk_buff *maps_nl_put(const void *msg, size_t msg_size, u_int8_t event)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
    maps_msg_head_t *head;

    if (NULL == td_maps_nl_sock) {
        maps_err("sock is NULL!");
        return NULL;
    }

    nl_skb = nlmsg_new(msg_size + sizeof(maps_msg_head_t), GFP_ATOMIC);
    if (NULL == nl_skb) {
        maps_err("create skb fail!");
        return NULL;
    }

    nlh = nlmsg_put(nl_skb, 0, 0, MAPS_NETLINK, 
                    msg_size + sizeof(maps_msg_head_t), 0);
    if (NULL == nlh) {
        maps_err("nlmsg_put fail!");
        nlmsg_free(nl_skb);
        return NULL;
    }

    head = NLMSG_DATA(nlh);
    head->type = event;
    memcpy(head->magic, MAPS_MSG_MAGIC, MAPS_MSG_MAGIC_LEN);
    if (msg && msg_size) {
        memcpy(head->msg, msg, msg_size);
    }

    return nl_skb;
}

/**
 * 单播发送netlink消息
 * 
 * 参数:
 * msg --- 发送的消息内容
 * msg_size --- 消息长度(字节)
 * pid --- 接收者pid
 * event --- 事件, 取值maps_ev_e
 * 
 * 返回值:
 * 成功0; 失败非0值
*/
static int maps_nl_unicast(const void *msg, size_t size,
                            u_int32_t pid, u_int8_t event)
{
    struct sk_buff *nl_skb = maps_nl_put(msg, size, event);

    if (!nl_skb) {
        return -1;
    }

    return netlink_unicast(td_maps_nl_sock, nl_skb, pid, MSG_DONTWAIT);
}

/**
 * 广播发送netlink消息
 * 
 * 参数:
 * msg --- 发送的消息内容
 * msg_size --- 消息长度(字节)
 * pid --- 接收者pid
 * event --- 事件, 取值maps_ev_e
 * 
 * 返回值:
 * 成功0; 失败非0值
*/
static int maps_nl_broadcast(const void *msg, size_t size, u_int8_t event)
{
    struct sk_buff *nl_skb = maps_nl_put(msg, size, event);

    if (!nl_skb) {
        return -1;
    }

    return netlink_broadcast(td_maps_nl_sock, nl_skb, 
            0, MAPS_NETLINK_GROUP, GFP_ATOMIC);
}


/**
 * 通知客户端状态变化和更新
 * 
 * 参数:
 * event --- 事件类型, 包括:MAPS_EV_STA_JOIN, MAPS_EV_STA_LEAVE, MAPS_EV_STA_BAND, MAPS_EV_STA_STATUS,
*/
static void maps_report_sta(maps_sta_t *sta, 
                            vendor_sta_t *vsta, 
                            unsigned char bss_index, 
                            unsigned char sta_num, 
                            maps_ev_e event)
{
    maps_msg_sta_t status;

    memset(&status, 0, sizeof(status));
    memcpy(status.mac, sta->mac, MAC_ADDR_LEN);
    status.bss_index = bss_index;
    status.sta_num = sta_num;
    status.bands = sta->bands;
    status.load = sta->load;
    status.active = sta->active;
    status.caps = 0;
    if (sta->ht) {
        status.caps |= MAPS_CAP_HT;
    }

    if (sta->vht) {
        status.caps |= MAPS_CAP_VHT;
    }
    
    if (sta->he) {
        status.caps |= MAPS_CAP_HE;
    }

    if (vsta) {
        status.rssi = vendor_get_sta_rssi(vsta);
        if (!status.rssi) {
            maps_info("invalid rssi of "MACSTR, MAC2STR(sta->mac));
            status.rssi = MAPS_RSSI_DUMMY;
        }

        status.nss = vendor_get_sta_nss(vsta);
        if (vendor_get_sta_rrm(vsta)) {
            status.caps |= MAPS_CAP_RRM;
        }
        if (vendor_get_sta_btm(vsta)) {
            status.caps |= MAPS_CAP_BTM;
        }
    }

    maps_dump("sta " MACSTR ", bss: %d, bands: %x, rssi:%ddBm, nss:%d"
            ", load:%d%%, active:%d, cap: %x, sta_num: %d, event: %d", 
            MAC2STR(status.mac), status.bss_index, status.bands, status.rssi, status.nss,
            status.load, status.active, status.caps, status.sta_num, event);

    maps_nl_broadcast(&status, sizeof(status), event);
}

/**
 * 通知客户端认证被拒绝, 对应于事件MAPS_EV_REJECT
 * 
 * 参数:
 * mac --- 客户端mac
 * bss_index --- 客户端认证的bss索引
 * count --- 拒绝次数
*/
static void maps_report_reject(unsigned char *mac, unsigned char bss_index, 
                                unsigned char count, signed char rssi)
{
    maps_msg_reject_t reject;

    memcpy(reject.mac, mac, MAC_ADDR_LEN);
    reject.bss_index = bss_index;
    reject.count = count;
    reject.rssi = rssi;

    maps_info("reject "MACSTR" %d times, BSS%d, RSSI %d",
                MAC2STR(mac), count, bss_index, rssi);
    maps_nl_broadcast(&reject, sizeof(reject), MAPS_EV_REJECT);
}

/**
 * 通知radio状态变化和更新
 * 
 * 参数:
 * event --- 时间类型, 包括: MAPS_EV_RADIO_STATUS
*/
static void maps_report_radio(unsigned char radio_index, 
                            maps_radio_t *radio,
                            maps_ev_e event)
{
    maps_msg_radio_t msg;

    msg.channel = radio->channel;
    msg.load = radio->load;
    msg.work_band = radio->work_band;
    msg.sta_num = radio->sta_num;
    msg.radio_index = radio_index;
    maps_dump("radio %d, channel: %d, load: %d%%, event: %d", 
            radio_index, radio->channel, radio->load, event);

    maps_nl_broadcast(&msg, sizeof(msg), event);
}

/**
 * 通知应用层VAP的状态或配置可能发生了变化
*/
static void maps_report_vap_update(vendor_vap_t *vap)
{
    maps_msg_bss_update_t msg;

    msg.bss_index = vendor_get_bss_index(vap);
    vendor_get_bssid(vap, msg.bssid);

    maps_nl_broadcast(&msg, sizeof(msg), MAPS_EV_VAP_UPDATE);
}

static inline struct hlist_head* maps_hash_head(unsigned char *mac)
{
    unsigned int hash = jhash_2words(*(u_int32_t*)(mac + 2), 
                                *(u_int32_t*)mac, td_maps_hash_rnd);
    return &td_maps_sta_h_heads[hash % MAPS_HASH_SIZE];
}

/**
 * 查找指定mac地址的客户端
*/
static maps_sta_t* maps_find_sta(unsigned char *mac)
{
    maps_sta_t *tpos;
    struct hlist_node *n;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
    struct hlist_node *pos;
#endif
    struct hlist_head *head = maps_hash_head(mac);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
    hlist_for_each_entry_safe(tpos, n, head, node)
#else
    hlist_for_each_entry_safe(tpos, pos, n, head, node)
#endif
    {
        if (0 == memcmp(tpos->mac, mac, MAC_ADDR_LEN)) {
            return tpos;
        }
    }

    return NULL;
}

/**
 * 移除客户端
*/
static void maps_remove_sta(maps_sta_t *sta)
{
    hlist_del_init(&sta->node);
    return;
}

/**
 * 添加客户端
*/
static maps_sta_t* maps_add_sta(unsigned char *mac)
{
    maps_sta_t *tpos;
    maps_sta_t *reuse_sta = NULL;
    struct hlist_head *head;
    int i;

    if (NULL == td_maps_sta_list) {
        return NULL;
    }
    /* 查找可以回收使用的maps_sta_node_t结构体 */
    for (i = 0; i < MAPS_MAX_NODES; i++) {
        tpos = ((maps_sta_t *)(td_maps_sta_list) + i);

        if (hlist_unhashed(&tpos->node)) {
            /* 此节点未被使用 */
            reuse_sta = tpos;
            break;
        }

        if (MAPS_BLOCK_DEFAULT == tpos->mode &&  !tpos->associated) {
            /* 回收最旧的节点 */
            if (!reuse_sta ||
                time_before(tpos->update_time, reuse_sta->update_time)) {
                reuse_sta = tpos;
            }
        }
    }

    if (!reuse_sta) {
        maps_err("STA list is full!");
        return NULL;
    }

    if (!hlist_unhashed(&reuse_sta->node)) {
        maps_dbg(MACSTR " is reused", MAC2STR(reuse_sta->mac));
        maps_remove_sta(reuse_sta);
    }

    memset(reuse_sta, 0 , sizeof(maps_sta_t));
    memcpy(reuse_sta->mac, mac, MAC_ADDR_LEN);
    reuse_sta->update_time = jiffies;
    /* add begin by dengxingde, 2020-04-20, 原因: 2G延时接入 */
    reuse_sta->create_time = jiffies;
    reuse_sta->hold_delay = 1;
    /* add end by dengxingde, 2020-04-20 */
    head = maps_hash_head(mac);
    hlist_add_head(&reuse_sta->node, head);

    return reuse_sta;
}

/*
dual-band(双频合一)radio->mode为MAPS_BAND_BLOCK_RSSI
时各频段pre-association阶段rssi阈值如下(双频5G处理逻辑存在差异):

一、dual-band(双频合一)时5G只考虑pre_down情况：
1、开启pre_down
reject_rssi_lower : -100    reject_rssi_upper: -69

2.4G(tri-band/dual-band)，2.4G只考虑pre_up情况:
1、开启pre_down/不开启pre_down；开启pre_up
reject_rssi_lower : -55    reject_rssi_upper: 0

*/
static bool _should_block(
                        maps_band_prefer_e prefer,
                        maps_sta_t *sta,
                        signed char rssi,
                        unsigned char bss_index,
                        maps_radio_t *radio)
{
    if (MAPS_BLOCK_DISBALE == sta->mode) {
        return false;

    } else if (MAPS_BLOCK_BSS == sta->mode) {

        if (maps_test_bit(sta->block_bss, 1 << bss_index)) {
            return true;

        } else {
            return false;
        }

    } else { /* MAPS_BLOCK_DEFAULT */

        if (MAPS_BAND_ACCEPT == radio->mode) {
            return false;

        } else if (MAPS_BAND_BLOCK == radio->mode) {
            return true;

        } else { /* MAPS_BAND_BLOCK_RSSI */
            /**1、双频合一5G与2.4G处理逻辑；**/
            if (rssi > radio->reject_rssi_lower &&
                rssi < radio->reject_rssi_upper)
            {
                return true;
            
            } else {
                return false;
            }
        }

    }

    return false;
}

static bool maps_has_available_essid(unsigned char bss_index,unsigned char radio_index)
{
    u_int32_t ess_radios;
    maps_radio_t *radio;
    maps_bss_t *bss;
    int i;

    if (NULL == td_maps_mng_bss) {
        return false;
    }

    if (NULL == td_maps_radios) {
        return false;
    }

    bss = ((maps_bss_t *)td_maps_mng_bss + bss_index);
    ess_radios = bss->ess_radios;

    /* MAPS_BLOCK_DEFAULT */
    maps_for_each_radio_multiap(radio, (maps_radio_t *)td_maps_radios, i) {
        if (i == radio_index) {
            /* 本频段 */
            continue;
        }

        /* 其他radio上有相同的essid存在 */
        if ((ess_radios & (1 << i))) {
            return true;
        }
    }
    return false;
}

static bool maps_2G_should_delay_reject(maps_sta_t *sta,
                                    signed char rssi,
                                    unsigned char bss_index,
                                    unsigned char radio_index,
                                    int reject_type)
{
    maps_radio_t *radio = NULL;

    if (!maps_valid_bss_index(bss_index) ||
        !maps_valid_radio_index(radio_index)) {
        return false;
    }

    if (NULL == td_maps_radios) {
        return false;
    }
    if (NULL == td_maps_config) {
        return false;
    }
    radio = ((maps_radio_t *)td_maps_radios + radio_index);
    if (!radio) {
       return false;
    }

    if (!IS_2G(radio->work_band)) {
        return false;
    }

    /* sta no need hold */
    if (!sta->hold_delay) {
        return false;
    }

    /* 用户关联数量没有达到阀值*/
    if (radio->sta_num < ((maps_config_t *)td_maps_config)->delay_reject_stanum) {
        return false;
    }

    if (!maps_has_available_essid(bss_index, radio_index)) {
        return false;
    }

    if ((DELAY_REJECT_PROBE == reject_type) && ((maps_config_t *)td_maps_config)->enable_delay_reject_probe) {
        return true;
    } else if ((DELAY_REJECT_AUTH == reject_type) && ((maps_config_t *)td_maps_config)->enable_delay_reject_auth) {
        return true;
    }

    return false;
}

/**
 * 是否应该拒绝客户端连接?
*/
static bool maps_should_block(maps_sta_t *sta,
                            signed char rssi,
                            unsigned char bss_index,
                            unsigned char radio_index)
{
    maps_radio_t *radio;
    maps_radio_t *radio_tmp;
    maps_bss_t *bss;
    u_int32_t ess_radios;
    int i;
    maps_band_prefer_e prefer = MAPS_PREFER_NONE;

    if (!maps_valid_bss_index(bss_index) ||
        !maps_valid_radio_index(radio_index)) {
        return false;
    }

    if (NULL == td_maps_radios) {
        return false;
    }
    
    if (NULL == td_maps_mng_bss) {
        return false;
    }
    
    radio = ((maps_radio_t *)td_maps_radios + radio_index);
    if (!radio) {
       return false;
    }

    bss = ((maps_bss_t *)td_maps_mng_bss + bss_index);
    ess_radios = bss->ess_radios;
    /* MAPS_BLOCK_DEFAULT */
    if (!_should_block(prefer, sta, rssi, bss_index, radio))
    {
        return false;
    }

    if (MAPS_BLOCK_BSS == sta->mode) {
        /* post-association steering, 由steerd确保存在可连接的BSS, 跳过下面的安全检查 */
        return true;
    }
    maps_for_each_radio_multiap(radio_tmp, (maps_radio_t *)td_maps_radios, i) {
        if (i == radio_index) {
            /* 本频段 */
            continue;
        }

        if (!(ess_radios & (1 << i))) {
            /* 此BSS所属的ESS没有用到索引为i的radio */
            continue;
        }

        if (sta->bands & radio_tmp->work_band) {
            if (!_should_block(prefer, sta, rssi, bss_index, radio_tmp))
            {
                /* pre-association steering, 必须保证其它频段可以连接才能拒绝连接 */
                return true;
            }
        }
    }
    maps_dump("sta %pM should block RADIO%d, but no other radio is available"
                ", accept it", sta->mac, radio_index);
    return false;
}

/**
 * Probe Request报文处理
 * 
 * 参数: 
 * mac --- 客户端mac地址
 * rssi --- 客户端信号强度, 范围: -100dBm~0dBm
 * vap --- 收到的vap
 * 
 * 返回值:
 * 0 应回复Probe Response, 1 不应回复Probe Response
*/
int maps_probe_req_handler(unsigned char *mac, 
                        signed char rssi, 
                        vendor_vap_t *vap, 
                        bool ht,
                        bool vht,
                        bool he,
                        unsigned char channel)
{
    /* TODO 需要考虑如果此处丢弃Probe Request, 是否会影响组网 */
    maps_sta_t *sta;
    maps_band_shift_e band;
    unsigned char bss_index;
    unsigned char radio_index;
    unsigned char sta_num;

    if (!mac || !rssi || !vap) {
        maps_err("invalid argument:%s%s%s", mac ? "" : " STA mac", rssi ? "" : " STA rssi", vap ? "" : " vap");
        return 0;
    }

    if (NULL == td_maps_config) {
        return 0;
    }
    if (!((maps_config_t *)td_maps_config)->enable) {
        return 0;
    }

    spin_lock_bh(&s_steer_lock);
    bss_index = vendor_get_bss_index(vap);
    radio_index = vendor_get_radio_index(vap);
    band = maps_channel_to_band(channel);

    if (!maps_valid_band(band)) {
        maps_err("unsupported channel %d.", channel);
        spin_unlock_bh(&s_steer_lock);
        return 0;
    }

    maps_dump("receive probe from " MACSTR " on channel %d(band %d)"
            ", rssi %d", MAC2STR(mac), channel, band, rssi);

    sta = maps_find_sta(mac);
    if (NULL == sta) {
        sta = maps_add_sta(mac);
        if (NULL == sta) {
            maps_dbg("add sta to hash set failed");
            spin_unlock_bh(&s_steer_lock);
            return 0;
        }
    }

    /* 更新客户端信息 */
    if (ht) {
        sta->ht = true;
    }

    if (vht) {
        sta->vht = true;
    }
     
    if (he)  {
        sta->he = true;
    }
    
    sta->update_time = jiffies;
    sta->band_time[band] = jiffies;

    if (!maps_test_bit(sta->bands, 1 << band)) {
        maps_set_bit(sta->bands, 1 << band);

        maps_dbg(MACSTR " support channel %d(band %d)",
                MAC2STR(mac), channel, band);
        if (sta->associated) {
            sta_num = vendor_get_sta_num(vap);
            maps_report_sta(sta, NULL, bss_index, sta_num, MAPS_EV_STA_BAND);
        }
    } else {
    }

    /* 是否回复Probe Response */
    if (((maps_config_t *)td_maps_config)->enable_reject_probe && 
        maps_should_block(sta, rssi, bss_index, radio_index)) {

        maps_dbg("bss%u drop probe of "MACSTR, bss_index, MAC2STR(mac));
        spin_unlock_bh(&s_steer_lock);
        return 1;
    }

    if (maps_2G_should_delay_reject(sta, rssi, bss_index, radio_index, DELAY_REJECT_PROBE)) {
        maps_dbg("bss%u radio%d delay reject probe of "MACSTR, bss_index, radio_index, MAC2STR(mac));
        spin_unlock_bh(&s_steer_lock);
        return 1;
    }

    spin_unlock_bh(&s_steer_lock);
    return 0;
}

/**
 * Auth报文处理
 * 
 * 参数: 
 * mac --- 客户端mac地址
 * rssi --- 客户端信号强度, 范围: -100dBm~0dBm
 * vap --- 客户端认证的vap
 * 
 * 返回值:
 * 0 应接受认证, 1 不应接受认证
*/
int maps_auth_handler(unsigned char *mac, signed char rssi, vendor_vap_t *vap)
{
    maps_sta_t *sta;
    maps_band_shift_e band;
    unsigned char channel;
    unsigned char bss_index;
    unsigned char radio_index;
    int reject = 0;

    if (!mac || !rssi || !vap) {
        maps_err("invalid argument");
        return 0;
    }

    if (NULL == td_maps_config) {
        return 0;
    }
    
    if (!((maps_config_t *)td_maps_config)->enable) {
        return 0;
    }

    spin_lock_bh(&s_steer_lock);
    bss_index = vendor_get_bss_index(vap);
    channel = vendor_get_channel(vap);
    radio_index = vendor_get_radio_index(vap);
    band = maps_channel_to_band(channel);
    if (!maps_valid_band(band)) {
        maps_err("unsupported channel %d.", channel);
        spin_unlock_bh(&s_steer_lock);
        return 0;
    }

    maps_info(MACSTR " try to auth on bss%u, radio%u, channel:%u, rssi:%d", 
            MAC2STR(mac), bss_index, radio_index, channel, rssi);

    sta = maps_find_sta(mac);
    if (NULL == sta) {
        sta = maps_add_sta(mac);
        if (NULL == sta) {
            maps_err("add sta to hash set failed");
            spin_unlock_bh(&s_steer_lock);
            return 0;
        }
    }

    /* 更新客户端信息 */
    sta->update_time = jiffies;
    sta->band_time[band] = jiffies;
    maps_set_bit(sta->bands, 1 << band);
    
    /* 是否拒绝认证 */
    if (((maps_config_t *)td_maps_config)->enable_reject_auth && 
        maps_should_block(sta, rssi, bss_index, radio_index)) {
        reject = 1;
    }

    if (maps_2G_should_delay_reject(sta, rssi, bss_index, radio_index, DELAY_REJECT_AUTH)) {
        maps_info("bss%u radio%d delay reject auth of "MACSTR, bss_index, radio_index, MAC2STR(mac));
        reject = 1;
    }

    if (reject) {
        if (sta->reject_cnt >= ((maps_config_t *)td_maps_config)->reject_max_cnt) {
            sta->reject_cnt = 0;
            sta->reject_time = 0;
            maps_info("reject "MACSTR" too many times, let it go", MAC2STR(mac));
            spin_unlock_bh(&s_steer_lock);
            return 0;

        } else {
            sta->reject_cnt++;
            sta->reject_time = jiffies;
            maps_report_reject(mac, bss_index, sta->reject_cnt, rssi);
            spin_unlock_bh(&s_steer_lock);
            return 1;
        }
    }

    spin_unlock_bh(&s_steer_lock);
    return 0;
}

/**
 * 用于客户端关联时或应用层初始化时将客户端信息加入到hash表中
*/
static void maps_sta_join(vendor_sta_t *vsta,
                        vendor_vap_t *vap,
                        unsigned char sta_num)
{
    maps_sta_t *sta;
    unsigned char *mac;
    unsigned char bss_index;
    maps_band_shift_e band;
    unsigned char channel;

    if (vendor_is_sta_neighbor(vsta)) {
        return;
    }

    bss_index = vendor_get_bss_index(vap);
    if (!maps_valid_bss_index(bss_index)) {
        /* bss_index为0表示此vap未开启steering. */
        return;
    }

    channel = vendor_get_channel(vap);
    band = maps_channel_to_band(channel);

    if (!maps_valid_band(band)) {
        maps_err("unsupported channel %d.", channel);
        return;
    }

    /* 前面认证时更新了时间戳, sta短期内不太可能被回收. 因此下面不再加锁保护 */
    mac = vendor_get_sta_mac(vsta);
    sta = maps_find_sta(mac);
    if (NULL == sta) {
        sta = maps_add_sta(mac);
        if (NULL == sta) {
            maps_err("add " MACSTR " to hash set failed", MAC2STR(mac));
            return;
        }
    }

    if (vendor_get_sta_ht(vsta)) {
        sta->ht = true;
    }

    if (vendor_get_sta_vht(vsta)) {
        sta->vht = true;
    }

    if (vendor_get_sta_he(vsta)) {
        sta->he = true;
    }
    
    sta->update_time = jiffies;
    sta->band_time[band] = jiffies;
    maps_set_bit(sta->bands, 1 << band);

    maps_info(MACSTR" associate to bss%u, old bss%u", 
            MAC2STR(mac), bss_index, sta->assoc_bss);
    sta->assoc_bss = bss_index;
    sta->associated = true;
    maps_report_sta(sta, vsta, bss_index, sta_num, MAPS_EV_STA_JOIN);

    return;
}

/**
 * 客户端关联成功时的处理
 * 
 * 参数: 
 * vap --- 客户端连接的vap
 * vsta --- 连接的客户端
*/
void maps_assoc_handler(vendor_sta_t *vsta, vendor_vap_t *vap)
{
    unsigned char sta_num;

    if (!vsta || !vap) {
        maps_err("invalid argument");
        return;
    }

    if (NULL == td_maps_config) {
        return;
    }
    
    if (!((maps_config_t *)td_maps_config)->enable) {
        return;
    }

    spin_lock_bh(&s_steer_lock);
    sta_num = vendor_get_sta_num(vap);
    sta_num++;

    maps_sta_join(vsta, vap, sta_num);

    spin_unlock_bh(&s_steer_lock);
    return;
}

/**
 * 客户端认证解除的处理
 * 
 * 参数: 
 * vap --- 客户端解除认证的vap
 * vsta --- 解除认证的客户端
*/
void maps_disassoc_handler(vendor_sta_t *vsta, vendor_vap_t *vap)
{
    maps_sta_t *sta;
    unsigned char *mac;
    unsigned char bss_index;
    maps_band_shift_e band;
    unsigned char channel;
    unsigned char sta_num;

    if (!vsta || !vap) {
        maps_err("invalid argument");
        return;
    }

    if (NULL == td_maps_config) {
        return;
    }
    
    if (!((maps_config_t *)td_maps_config)->enable) {
        maps_err(" steerd disable");
        return;
    }

    spin_lock_bh(&s_steer_lock);
    if (vendor_is_sta_neighbor(vsta)) {
        spin_unlock_bh(&s_steer_lock);
        return;
    }

    bss_index = vendor_get_bss_index(vap);
    if (!maps_valid_bss_index(bss_index)) {
        /* bss_index为0表示此vap未开启steering. */
        spin_unlock_bh(&s_steer_lock);
        return;
    }

    channel = vendor_get_channel(vap);
    band = maps_channel_to_band(channel);

    if (!maps_valid_band(band)) {
        maps_err("unsupported channel %d.", channel);
        spin_unlock_bh(&s_steer_lock);
        return;
    }

    mac = vendor_get_sta_mac(vsta);
    /* sta已关联, 不会被回收. 下面使用sta是安全的, 不再加锁保护 */
    sta = maps_find_sta(mac);
    if (NULL == sta) {
        maps_err("can't find sta "MACSTR, MAC2STR(mac));
        spin_unlock_bh(&s_steer_lock);
        return;
    }

    sta->update_time = jiffies;
    sta->band_time[band] = jiffies;
    maps_set_bit(sta->bands, 1 << band);

    sta_num = vendor_get_sta_num(vap);
    if (0 == sta_num) {
        maps_err("no sta associated BSS%d", bss_index);
    } else {
        sta_num--;
    }
    
    maps_info(MACSTR" disassociate bss%u", MAC2STR(mac), bss_index);
    maps_report_sta(sta, NULL, bss_index, sta_num, MAPS_EV_STA_LEAVE);

    /**
     * 手机从某一频段漫游到另一频段时, 可能会先关联目标频段, 
     * 再断开之前的频段. 因此此函数调用时, 手机可能实际上还连着.
    */
    if (bss_index == sta->assoc_bss) {
        sta->associated = false;
    } else {
        maps_dbg(MACSTR" disconnect bss%u, but it associated to bss%u", 
                MAC2STR(mac), bss_index, sta->assoc_bss);
    }

    spin_unlock_bh(&s_steer_lock);
    return;
}

/**
 * 收到vsta 回复的BSS Transition Response
*/
void maps_bss_tran_res_handler(vendor_sta_t *vsta, 
                                vendor_vap_t *vap, 
                                unsigned char status)
{
    unsigned char *mac;
    unsigned char bss_index;
    maps_msg_bss_trans_rsp_t msg;

    if (!vsta || !vap) {
        maps_err("invalid arguments");
        return;
    }

    if (NULL == td_maps_config) {
        return;
    }  
    
    if (!((maps_config_t *)td_maps_config)->enable) {
        maps_err(" steerd disable");
        return;
    }

    spin_lock_bh(&s_steer_lock);
    bss_index = vendor_get_bss_index(vap);
    if (!maps_valid_bss_index(bss_index)) {
        spin_unlock_bh(&s_steer_lock);
        return;
    }

    mac = vendor_get_sta_mac(vsta);
    msg.bss_index = bss_index;
    msg.status = status;
    memcpy(msg.mac, mac, MAC_ADDR_LEN);
    maps_nl_broadcast(&msg, sizeof(msg), MAPS_EV_BSS_TRANS);

    spin_unlock_bh(&s_steer_lock);
    return;
}

/**
 * 收到了 vsta 回复的Beacon Report
*/
void maps_beacon_rep_handler(vendor_sta_t *vsta, vendor_vap_t *vap)
{
    unsigned char *mac;
    maps_msg_beacon_rep_t msg;
    unsigned char bss_index;

    
    if (!vsta || !vap) {
        maps_err("invalid arguments");
        return;
    }

    if (NULL == td_maps_config) {
        return;
    }
    
    if (!((maps_config_t *)td_maps_config)->enable) {
        maps_err(" steerd disable");
        return;
    }

    spin_lock_bh(&s_steer_lock);
    bss_index = vendor_get_bss_index(vap);
    if (!maps_valid_bss_index(bss_index)) {
        spin_unlock_bh(&s_steer_lock);
        return;
    }

    memset(&msg, 0, sizeof(msg));
    mac = vendor_get_sta_mac(vsta);
    memcpy(msg.sta, mac, MAC_ADDR_LEN);
    msg.bss_index = bss_index;
    vendor_fill_beacon_rep(vsta, vap, &msg);

    maps_dbg("receive beacon report from " MACSTR ", count %u", 
                MAC2STR(msg.sta), msg.count);
    maps_nl_broadcast(&msg, sizeof(msg), MAPS_EV_BEACON_REP);

    spin_unlock_bh(&s_steer_lock);
    return;
}

/**
 * vap 不可用
*/
void maps_vap_unavailable_handler(vendor_vap_t *vap)
{
    int i, j;
    maps_radio_t *radio;
    maps_bss_t *bss;

    if (!vap) {
        maps_err("invalid arguments");
        return;
    }

    if (!td_maps_config || !td_maps_mng_bss || !td_maps_radios) {
        return;
    }
    
    if (!((maps_config_t *)td_maps_config)->enable) {
        return;
    }
    
    spin_lock_bh(&s_steer_lock);
    maps_report_vap_update(vap);

    /* 将其从s_managed_bss中移除 */
    for (i = 1; i <= MAPS_MAX_BSS_INDEX; i++) {
        bss = ((maps_bss_t *)td_maps_mng_bss + i);
        if (vap == bss->vap) {
            bss->vap = NULL;
            maps_dbg("unmanage BSS%d", i);
        }
    }

    /* 将其从s_radios中移除 */
    maps_for_each_radio_multiap(radio, (maps_radio_t *)(td_maps_radios), i) {
        for (j = 0; j < MAPS_VAP_PER_RADIO; j++) {
            if (vap == radio->vaps[j]) {
                radio->vaps[j] = NULL;
                maps_dbg("remove vap from RADIO%d", i);
            }
        }
    }

    maps_dbg("vap is down");

    spin_unlock_bh(&s_steer_lock);
    return;
}

/**
 * vap 就绪
*/
void maps_vap_available_handler(vendor_vap_t *vap)
{
    maps_radio_t *radio;
    maps_bss_t *bss;
    unsigned char bss_index;
    unsigned char radio_index;
    int i;

    if (!vap) {
        maps_err("invalid arguments");
        return;
    }

    if (!td_maps_config || !td_maps_mng_bss || !td_maps_radios) {
       return;
    }

    if (!((maps_config_t *)td_maps_config)->enable) {
        maps_err(" steerd disable");
        return;
    }
    
    spin_lock_bh(&s_steer_lock);
    maps_report_vap_update(vap);

    bss_index = vendor_get_bss_index(vap);
    maps_dbg("BSS%u is up", bss_index);

    bss = ((maps_bss_t *)td_maps_mng_bss + bss_index);
    if (maps_valid_bss_index(bss_index) && !bss->vap) {
        bss->vap = vap;
        maps_dbg("manage BSS%u", bss_index);
    }

    radio_index = vendor_get_radio_index(vap);
    if (maps_valid_radio_index(radio_index)) {
        radio = ((maps_radio_t *)td_maps_radios + radio_index);
        
        for (i = 0; i < MAPS_VAP_PER_RADIO; i++) {
            if (radio->vaps[i] == vap) {
                spin_unlock_bh(&s_steer_lock);
                return;
            }
        }

        for (i = 0; i < MAPS_VAP_PER_RADIO; i++) {
            if (NULL == radio->vaps[i]) {
                radio->vaps[i] = vap;
                maps_dbg("add vap to RADIO%d", radio_index);
                spin_unlock_bh(&s_steer_lock);
                return;
            }
        }

        maps_err("no enough space, support %d vap per radio", MAPS_VAP_PER_RADIO);
    }

    spin_unlock_bh(&s_steer_lock);
    return;
}

/**
 * 与应用层的 IOCTL 接口测试
*/
static int maps_ioctl_pong(void *msg, size_t msg_len)
{
    if (msg_len < MAPS_PONG_LEN) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    memcpy(msg, MAPS_PONG, MAPS_PONG_LEN);
    return 0;
}

/**
 * 设置调试级别
*/
int maps_set_dbg(void *msg, size_t msg_len)
{
    if (msg_len < sizeof(int)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    td_maps_dbg = *(int *)msg;
    maps_dbg("set dbg to %d", td_maps_dbg);

    return 0;
}

static void _join_sta(vendor_sta_t *vsta, 
                    vendor_vap_t *vap, 
                    unsigned long data)
{
    unsigned char sta_num;

    spin_lock_bh(&s_steer_lock);
    if (vendor_sta_associated(vsta)) {
        sta_num = vendor_get_sta_num(vap);
        maps_sta_join(vsta, vap, sta_num);
    }
    spin_unlock_bh(&s_steer_lock);

    return;
}

/**
 * 为vap设置其索引, 并设置相关配置
*/
static int maps_set_bss_conf(void *msg, size_t msg_len, vendor_vap_t *vap)
{
    maps_msg_bss_conf_t *bssc;
    maps_bss_t *bss;
    unsigned char index;
    int i;

    if (NULL == td_maps_mng_bss) {
        return -1;
    }
    
    if (msg_len < sizeof(maps_msg_bss_conf_t)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    bssc = (maps_msg_bss_conf_t *)msg;
    index = bssc->index;

    if (index < 0 || index > MAPS_MAX_BSS_INDEX) {
        maps_err("invalid index %d", index);
        return -EINVAL;
    }

    vendor_set_bss_index(vap, index);
    
    if (0 == index) {
        /* index 值为0的bss禁止steering, 清除此VAP的引用 */
        for (i = 1; i <= MAPS_MAX_BSS_INDEX; i++) {
            bss = ((maps_bss_t *)td_maps_mng_bss + i);
            if (vap == bss->vap) {
                bss->vap = NULL;
                maps_dbg("remove bss%d", i);
                return 0;
            }
        }

        maps_err("the bss is not managed");
        return -1;
    } else {
        bss = ((maps_bss_t *)td_maps_mng_bss + index);
        if (NULL != bss ->vap) {
            maps_err("bss %u is already managed", index);
            return -1;
        }

        bss ->vap = vap;
        bss ->ess_radios = bssc->ess_radios;
        maps_dbg("add bss%u", index);
        spin_unlock_bh(&s_steer_lock);
        vendor_iterate_sta(vap, _join_sta, 0);
        spin_lock_bh(&s_steer_lock);
        return 0;
    }

}

/**
 * 设置配置参数
*/
static int maps_set_config(void *msg, size_t msg_len)
{
    maps_config_t *conf;

    if (msg_len < sizeof(maps_config_t)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    if (NULL == td_maps_config) {
        return -1;
    }
    
    conf = (maps_config_t *)msg;

    memcpy(td_maps_config, conf, sizeof(maps_config_t));
    maps_dbg("update configs, enable=%d", conf->enable);

    return 0;
}

/**
 * 设置radio参数
*/
static int maps_set_radio_conf(void *msg, size_t msg_len)
{
    maps_msg_radio_conf_t *radio_conf;
    maps_radio_t *radio;
    unsigned char radio_index;

    if (NULL == td_maps_radios) {
        return -EINVAL;
    }
    
    if (msg_len < sizeof(maps_msg_radio_conf_t)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    radio_conf = (maps_msg_radio_conf_t *)msg;
    radio_index = radio_conf->index;
    if (!maps_valid_radio_index(radio_index)) {
        maps_err("invalid radio index %u", radio_index);
        return -EINVAL;
    }

    radio = ((maps_radio_t *)td_maps_radios + radio_index);
    radio->reject_rssi_upper = radio_conf->reject_rssi_upper;
    radio->reject_rssi_lower = radio_conf->reject_rssi_lower;
    radio->monitor_rssi_high = radio_conf->monitor_rssi_high;
    radio->monitor_rssi_low = radio_conf->monitor_rssi_low;
    radio->mode = radio_conf->mode;

    maps_dbg("update config for radio %d", radio_index);

    return 0;
}

/**
 * 设置客户端配置
*/
static int maps_set_sta_conf(void *msg, size_t msg_len)
{
    maps_msg_sta_conf_t *sconf;
    maps_sta_t *sta;

    if (msg_len < sizeof(maps_msg_sta_conf_t)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    sconf = (maps_msg_sta_conf_t *)msg;
    
    sta = maps_find_sta(sconf->mac);
    if (NULL == sta) {
        sta = maps_add_sta(sconf->mac);
        if (NULL == sta) {
            maps_err("sta list is full!");
            return -1;
        }
        maps_dbg("add STA " MACSTR, MAC2STR(sconf->mac));
    }
    sta->block_bss = sconf->block_bss;
    sta->mode = sconf->mode;

    maps_dbg("update config of "MACSTR", mode:%d, block_bss:%d", 
            MAC2STR(sconf->mac), sconf->mode, sconf->block_bss);

    return 0;
}

/**
 * 为radio添加vap
*/
static int maps_add_radio_vap(void *msg, size_t msg_len, vendor_vap_t *vap)
{
    maps_msg_radio_add_t *radio_add;
    maps_radio_t *radio;
    int index, i;

    if (NULL == td_maps_radios) {
        return -EINVAL;
    }
    
    if (msg_len < sizeof(maps_msg_radio_add_t)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    radio_add = (maps_msg_radio_add_t *)msg;
    index = radio_add->index;
    if (!maps_valid_radio_index(index)) {
        return -EINVAL;
    }

    radio = ((maps_radio_t *)td_maps_radios + index);
    vendor_set_radio_index(vap, index);
    maps_dbg("add vap to radio %d", index);
    
    for (i = 0; i < MAPS_VAP_PER_RADIO; i++) {
        if (NULL == radio->vaps[i]) {
            radio->vaps[i] = vap;
            return 0;
        }
    }

    maps_err("radio list is full");
    return -1;
}

static unsigned int maps_get_op_class(unsigned char channel)
{
    // TODO 添加US, EU, JP的Regulatory class /
    if (channel <= 14) {
        return 81;
    } else if (channel >= 36 && channel <= 48) {
        return 115;
    } else if (channel >= 52 && channel <= 64)  {
        return 118;
    } else if (channel >= 100 && channel <= 140) {
        return 121;
    } else if (channel >= 149 && channel <= 169) {
        return 169;
    } else {
        maps_err("unsupported channel %u", channel);
        return 0;
    }
}

/**
 * 发送BSS Transition Request
*/
static int maps_bss_trans_req(void *msg, size_t msg_len, vendor_vap_t *vap)
{
    unsigned char op_class;
    unsigned char phy_type;
    maps_msg_bss_trans_req_t *btm_req;

    if (msg_len < sizeof(maps_msg_bss_trans_req_t)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    btm_req = (maps_msg_bss_trans_req_t *)msg;
    op_class = maps_get_op_class(btm_req->channel);

    /* TODO 2.4G 不一定是HT(7), 5G也不一定是VHT(9) */
    phy_type = (btm_req->channel <= 14 ? 7 : 9);


    maps_dbg("trans " MACSTR " to " MACSTR
            ", channel %u, op_class %u, phy_type %u", 
            MAC2STR(btm_req->sta), MAC2STR(btm_req->bss), 
            btm_req->channel, op_class, phy_type);

    return vendor_bss_trans(vap, btm_req->sta, btm_req->bss, 
                            btm_req->channel, op_class, phy_type);
}


/**
 * 发送Beacon Request
*/
static int maps_beacon_req(void *msg, size_t msg_len, vendor_vap_t *vap)
{
    maps_msg_beacon_req_t *req;
    unsigned char op_class;
    unsigned char channel;

    if (msg_len < sizeof(maps_msg_beacon_req_t)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    req = (maps_msg_beacon_req_t *)msg;
    channel = req->channel;
    op_class = maps_get_op_class(channel);

    maps_dbg("send beacon request to " MACSTR 
            " channel: %u, ssid: '%s', op_class: %u", 
            MAC2STR(req->sta), req->channel, req->ssid, op_class);

    return vendor_beacon_req(vap, req->sta, req->ssid, channel, op_class);
}

/**
 * 清除所有状态
*/
static int maps_clean(void)
{
    if (!td_maps_config || !td_maps_mng_bss || !td_maps_radios) {
        return -1;
    }
    
    ((maps_config_t *)td_maps_config)->enable = false;
    
    maps_dbg("flushing status...");
    /**
     * 将s_config.enable设为false后, 后续的中断或定时器会直接return, 
     * 不会造成并行问题. 对于正在运行的中断或定时器, 等待一段时间, 
     * 确保对下面变量的访问已经结束.
     * 
     * 由于IOCTL外面可能会被驱动加锁, 不能睡眠, 使用delay代替
    */
    mdelay(MAPS_CLEAN_DELAY);

    memset(td_maps_config, 0, sizeof(td_maps_config));
    memset(td_maps_mng_bss, 0, sizeof(maps_bss_t)  * (MAPS_MAX_BSS_INDEX + 1));
    memset(td_maps_radios, 0, sizeof(maps_radio_t) * (MAPS_MAX_RADIO_INDEX + 1));
    memset(&td_maps_sta_h_heads, 0, sizeof(td_maps_sta_h_heads));
    memset(td_maps_sta_list, 0, sizeof(maps_sta_t) * (MAPS_MAX_NODES));
    vendor_clean();

    return 0;
}

static int maps_dump_radios(void)
{
    maps_radio_t *radio;
    maps_bss_t *bss;
    int i;

    if (!td_maps_radios || !td_maps_mng_bss) {
        return -1;
    }
    
    maps_for_each_radio_multiap(radio, (maps_radio_t *)td_maps_radios, i) {
        if (!radio->vaps[0]) {
            continue;
        }

        printk("\n");
        printk("[radio%d]\n", i);
        printk("channel=%d\n", radio->channel);
        printk("load=%d%%\n", radio->load);
        printk("load_sum=%d\n", radio->load_sum);
        printk("mode=%d\n", radio->mode);
        printk("work_band=0x%x\n", radio->work_band);
        printk("reject_rssi_upper=%d\n", radio->reject_rssi_upper);
        printk("reject_rssi_lower=%d\n", radio->reject_rssi_lower);
        printk("monitor_rssi_high=%d\n", radio->monitor_rssi_high);
        printk("monitor_rssi_low=%d\n", radio->monitor_rssi_low);
    }

    printk("\n");
    for (i = 1; i <= MAPS_MAX_BSS_INDEX; i++) {
        bss = ((maps_bss_t *)td_maps_mng_bss + i);
        if (bss->vap) {
            printk("vap[%d] ess_radios=0x%x\n",i, bss->ess_radios);
        }
    }

    return 0;
}

static int maps_dump_stas(void)
{
    maps_sta_t *tpos;
    struct hlist_node *n;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
    struct hlist_node *pos;
#endif
    struct hlist_head *head;
    int i, j, count;

    count = 0;
    printk("jiffies=%lu\n", jiffies);
    for (i = 0; i < MAPS_HASH_SIZE; i++) {
        head = &td_maps_sta_h_heads[i];
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
        hlist_for_each_entry_safe(tpos, n, head, node)
#else
        hlist_for_each_entry_safe(tpos, pos, n, head, node)
#endif
        {
            count++;
            printk("\n");
            printk("["MACSTR"]\n", MAC2STR(tpos->mac));
            printk("bands=0x%x\n", tpos->bands);
            printk("mode=0x%x\n", tpos->mode);
            printk("block_bss=0x%x\n", tpos->block_bss);
            printk("assoc_bss=0x%x\n", tpos->assoc_bss);
            printk("reject_cnt=%u\n", tpos->reject_cnt);
            printk("reject_time=%lu\n", tpos->reject_time);
            printk("update_time=%lu\n", tpos->update_time);
            printk("associated=%d\n", tpos->associated);
            printk("active=%d\n", tpos->active);
            printk("active_time=%lu\n", tpos->active_time);
            printk("load_sum=%u\n", tpos->load_sum);
            printk("load=%u\n", tpos->load);
            printk("last_rssi=%d\n", tpos->last_rssi);
            printk("ht=%d\n", tpos->ht);
            printk("vht=%d\n", tpos->vht);
            printk("hold_delay=%d (%lds)\n", tpos->hold_delay, (jiffies - tpos->create_time)/HZ);
            printk("band_time=");
            for (j = 1; j < MAPS_BAND_NUM; j ++) {
                printk("%lu ", tpos->band_time[j]);
            }
            printk("\n");
        }
    }
    printk("\n%d records\n", count);

    return 0;
}

static int maps_dump_config(void)
{
    if (NULL == td_maps_config) {
        printk("\ntd_maps_config is NULL \n");
    }
    printk("\n");
    printk("[configurations]\n");
    printk("enable=%d\n", ((maps_config_t *)td_maps_config)->enable);
    printk("mode=%d\n", ((maps_config_t *)td_maps_config)->mode);
    printk("enable_reject_probe=%d\n", ((maps_config_t *)td_maps_config)->enable_reject_probe);
    printk("enable_reject_auth=%d\n", ((maps_config_t *)td_maps_config)->enable_reject_auth);
    printk("probe_age=%d\n", ((maps_config_t *)td_maps_config)->probe_age);
    printk("reject_age=%d\n", ((maps_config_t *)td_maps_config)->reject_age);
    printk("reject_max_cnt=%d\n", ((maps_config_t *)td_maps_config)->reject_max_cnt);
    printk("active_slot=%d\n", ((maps_config_t *)td_maps_config)->active_slot);
    printk("active_flow=%d\n", ((maps_config_t *)td_maps_config)->active_flow);
    printk("enable_delay_reject_probe=%d\n", ((maps_config_t *)td_maps_config)->enable_delay_reject_probe);
    printk("enable_delay_reject_auth=%d\n", ((maps_config_t *)td_maps_config)->enable_delay_reject_auth);
    printk("delay_reject_age=%ds\n", ((maps_config_t *)td_maps_config)->delay_reject_age);
    printk("delay_reject_stanum=%d\n", ((maps_config_t *)td_maps_config)->delay_reject_stanum);

    return 0;
}

static void maps_probe_age(maps_sta_t *tpos)
{
    int j;

    if (NULL == td_maps_config) {
        return;
    }
    
    if (tpos->associated || !((maps_config_t *)td_maps_config)->probe_age) {
        return;
    }

    for (j = 0; j < MAPS_BAND_NUM; j++) {
        if (maps_timeout(tpos->band_time[j], ((maps_config_t *)td_maps_config)->probe_age)) {
            if (maps_test_bit(tpos->bands, 1 << j)) {

                maps_reset_bit(tpos->bands, 1 << j);
                tpos->hold_delay = 0;
                maps_dbg("band %d record of "MACSTR" is aged",
                         j, MAC2STR(tpos->mac));
            }
        }

        /* add begin by dengxingde, 2020-04-20, 原因: 2G延时接入，老化 */
        if (maps_timeout(tpos->create_time, ((maps_config_t *)td_maps_config)->delay_reject_age)) {
            if (maps_test_bit(tpos->bands, 1 << j)) {
                tpos->hold_delay = 0;
                maps_dbg("band %d reset hold_delay of "MACSTR"",
                         j, MAC2STR(tpos->mac));

            }
        }
        /* add end by dengxingde, 2020-04-20 */
    }

    return;
}

static void maps_reject_age(maps_sta_t *tpos)
{
    if (NULL == td_maps_config) {
        return;
    }
    
    if (tpos->reject_cnt && 
        maps_timeout(tpos->reject_time, ((maps_config_t *)td_maps_config)->reject_age)) {

        tpos->reject_cnt = 0;
        tpos->reject_time = 0;
        maps_dbg("reject record of "MACSTR" is aged", MAC2STR(tpos->mac));
    }

    return;
}

/**
 * 客户端老化
*/
static void maps_sta_age_handler(void)
{
    maps_sta_t *tpos;
    struct hlist_node *n;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
    struct hlist_node *pos;
#endif
    struct hlist_head *head;
    int i;

    for (i = 0; i < MAPS_HASH_SIZE; i++) {
        head = &td_maps_sta_h_heads[i];
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
        hlist_for_each_entry_safe(tpos, n, head, node)
#else
        hlist_for_each_entry_safe(tpos, pos, n, head, node)
#endif
        {
            maps_reject_age(tpos);

            /* 已关联客户端不老化支持频段信息, 且不能被释放 */
            if (tpos->associated) {
                continue;    
            }

            maps_probe_age(tpos);
            if (0 == tpos->bands &&
                0 == tpos->reject_cnt &&
                MAPS_BLOCK_DEFAULT == tpos->mode) {

                /* 此节点没有记录任何信息, 可以被回收 */
                maps_dbg("node for "MACSTR" is recycled", MAC2STR(tpos->mac));
                maps_remove_sta(tpos);
            }
        }
    }

    return;
}

/**
 * 客户端状态监控
 * 
 * 参数: 
 * times --- 定时器触发的次数
*/
static void _maps_sta_monitor(vendor_sta_t *vsta, 
                            vendor_vap_t *vap, 
                            unsigned long times)
{
    maps_sta_t *sta;
    unsigned char *mac;
    signed char cur_rssi;
    unsigned char radio_index;
    unsigned int flow;
    bool old_active;
    maps_radio_t *radio;
    int bss_index;
    unsigned char sta_num;
    bool associated;

    if (!td_maps_config || !td_maps_radios) {
        return;
    }
    
    if (vendor_is_sta_neighbor(vsta)) {
        return;
    }

    bss_index = vendor_get_bss_index(vap);
    if (!maps_valid_bss_index(bss_index)) {
        return;
    }

    associated = vendor_sta_associated(vsta);

    mac = vendor_get_sta_mac(vsta);
    sta = maps_find_sta(mac);
    if (!sta) {
        if (!associated) {
            return;
        } else {

            maps_err("can't find node of " MACSTR, MAC2STR(mac));
            sta = maps_add_sta(mac);
            if (!sta) {
                maps_err("can't add node "MACSTR, MAC2STR(mac));
                return;
            }
        }
    }

    sta_num = vendor_get_sta_num(vap);

    if (associated && !sta->associated) {

        sta->associated = associated;
        maps_info(MACSTR" associated, but no notifiaction", MAC2STR(mac));
        maps_sta_join(vsta, vap, sta_num);

    } else if (!associated && sta->associated) {

        maps_info(MACSTR" left, but no notifiaction", MAC2STR(mac));
        maps_report_sta(sta, NULL, bss_index, sta_num, MAPS_EV_STA_LEAVE);
        sta->associated = associated;
    }

    if (!associated) {
        return;
    }

    radio_index = vendor_get_radio_index(vap);
    if (!maps_valid_radio_index(radio_index)) {
        maps_err("invalid radio_index %d of sta " MACSTR,
                 radio_index, MAC2STR(mac));
        return;
    }

    radio = ((maps_radio_t *)td_maps_radios + radio_index);

    /* 更新客户端空口占用率 */
    sta->load_sum += vendor_get_sta_load(vsta, &sta->asta);
    if (0 == (times % MAPS_STA_CYCLE) && !sta->is_report) {
        sta->load = (unsigned char)(sta->load_sum / MAPS_STA_CYCLE);
        sta->load_sum = 0;
        if (sta->load > 100) {
            sta->load = 100;
        }
    }

    /* 更新客户端active状态 */
    old_active = sta->active;
    flow = vendor_get_sta_flow(vsta, &sta->asta);
    if (flow > ((maps_config_t *)td_maps_config)->active_flow) {

        sta->active_time = jiffies;
        if (!sta->active) {
            maps_dbg(MACSTR " is active(flow:%uB/s)", MAC2STR(sta->mac), flow);
            sta->active = true;
        }

    } else if (sta->active && 
        maps_timeout(sta->active_time, ((maps_config_t *)td_maps_config)->active_slot)) {
        
        maps_dbg(MACSTR " is idle (flow: %uB/s)", MAC2STR(sta->mac), flow);
        sta->active = false;
    }

    /* 更新rssi */
    cur_rssi = vendor_get_sta_rssi(vsta);
    if (!cur_rssi) {
        maps_info("invalid rssi of "MACSTR, MAC2STR(sta->mac));
        return;
    }

    /* 检查是否需要通报客户端状态 */
    if ((cur_rssi > radio->monitor_rssi_high ||
        cur_rssi < radio->monitor_rssi_low ||
        abs(sta->last_rssi - cur_rssi) > MAPS_RSSI_REP_DIFF ||
        old_active != sta->active ||
        0 == times % MAPS_STA_CYCLE) && 
        !sta->is_report ) {
        sta->is_report = true;
        sta->last_rssi = cur_rssi;
        maps_report_sta(sta, vsta, bss_index, sta_num, MAPS_EV_STA_STATUS);
    } else {
        sta->is_report = false;
    }

    return;
}

static void maps_sta_monitor(vendor_sta_t *vsta, 
                            vendor_vap_t *vap, 
                            unsigned long times)
{
    spin_lock_bh(&s_steer_lock);

    _maps_sta_monitor(vsta, vap, times);

    spin_unlock_bh(&s_steer_lock);
}

/**
 * radio状态监控
 * 
 * 参数: 
 * times --- 定时器触发的次数
*/

static void maps_radio_monitor(unsigned long times)
{
    maps_radio_t *radio;
    vendor_vap_t *vap;
    unsigned char sta_num, sta_num_old;
    unsigned int load;  
    unsigned char old_channel;
    maps_band_shift_e band_shift;
    int i, j;

    if (NULL == td_maps_radios) {
        return;
    }
    
    maps_for_each_radio_multiap(radio, (maps_radio_t *)(td_maps_radios), i) {
        if (!radio->vaps[0]) {
            continue;
        }

        /* 更新信道占用率 */
        load = vendor_get_channel_load(radio->vaps, MAPS_VAP_PER_RADIO);
        radio->load_sum += load;
        if (0 == (times % MAPS_RADIO_CYCLE) && !radio->is_report) {
            radio->load = (unsigned char)(radio->load_sum / (MAPS_RADIO_CYCLE * 2));
            radio->load_sum = 0;
            if (radio->load > 100) {
                radio->load = 100; 
            }
        } else if ((times < MAPS_RADIO_CYCLE) && !radio->is_report) {
            radio->load = load;
        }

        /* 更新信道 */
        old_channel = radio->channel;
        radio->channel = vendor_get_channel(radio->vaps[0]);
        /* 获取到的信道是0   则 将old_channel      赋值给 radio->channel*/
        if(radio->channel == 0) {
            radio->channel = old_channel;
        }
        band_shift = maps_channel_to_band(radio->channel);

        if (!maps_valid_band(band_shift)) {
            maps_err("unsupported channel %d.", radio->channel);
            continue;
        }
        radio->work_band = TBIT(band_shift);

        /* 更新客户端数 */
        sta_num = 0;
        sta_num_old = radio->sta_num;
        for (j = 0; j < MAPS_VAP_PER_RADIO; j++) {
            vap = radio->vaps[j];
            if (!vap) {
                continue;
            }
            sta_num += vendor_get_sta_num(vap);
        }
        radio->sta_num = sta_num;

        /* 需要时通报状态给应用层 */
        if ((old_channel != radio->channel || 
            sta_num_old != radio->sta_num ||
            0 == (times % MAPS_RADIO_CYCLE)) && !radio->is_report) {
            radio->is_report = true;
            maps_report_radio(i, radio, MAPS_EV_RADIO_STATUS);
        } else {
            radio->is_report = false;
        }

    }

    return;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
static void maps_timer_handler(unsigned long data)
#else
static void maps_timer_handler(struct timer_list *t)
#endif
{
    int i;
    vendor_vap_t *vap;
    maps_bss_t *bss;

    /* 用于记录定时器触发次数 */
    static unsigned long s_times = 1;

    if (NULL == td_maps_config || NULL == td_maps_mng_bss) {
        return;
    }
    
    if (!((maps_config_t *)td_maps_config)->enable) {
        mod_timer(&td_maps_timer, jiffies + MAPS_TIMER_INTERVAL * HZ);
        return;
    }
    spin_lock_bh(&s_steer_lock);
    maps_sta_age_handler();

    /* 有效索引从1开始 */
    for (i = 1; i <= MAPS_MAX_BSS_INDEX; i++) {
        bss = ((maps_bss_t *)td_maps_mng_bss + i);
        vap = bss->vap;
        if (NULL == vap) {
            continue;
        }

        spin_unlock_bh(&s_steer_lock);
        vendor_iterate_sta(vap, maps_sta_monitor, s_times);
        spin_lock_bh(&s_steer_lock);
    }

    maps_radio_monitor(s_times);
    spin_unlock_bh(&s_steer_lock);
    s_times++;
    mod_timer(&td_maps_timer, jiffies + MAPS_TIMER_INTERVAL * HZ);

    return;
}

/**
 * data: 用户上下文指针
 * len: `data`长度
 * vap: IOCTL使用的interface对应的vap
*/
int maps_ioctl_handler(void *data, size_t len, vendor_vap_t *vap)
{
    maps_msg_head_t *head;
    int ret;
    int cmd;
    int msg_len;
    void *msg;
    char buff[MAPS_MAX_MSG_SIZE] = {0};

    if (!data || !vap) {
        maps_err("invalid arguments");
        return -EINVAL;
    }
    if (len < sizeof(maps_msg_head_t) || 
        len > sizeof(buff)) {
        maps_err("wrong size %u, should be [%u, %u]", len, 
                sizeof(maps_msg_head_t), sizeof(buff));
        return -EINVAL;
    }

    head = (maps_msg_head_t *)data;
    if (0 != memcmp(head->magic, MAPS_MSG_MAGIC, MAPS_MSG_MAGIC_LEN)) {
        maps_err("wrong magic!");
        return -EINVAL;
    }

    ret = 0;
    cmd = head->type;
    msg = head->msg;
    msg_len = len - sizeof(maps_msg_head_t);

    spin_lock_bh(&s_steer_lock);
    switch (cmd) {
        case MAPS_CMD_PING:
            ret = maps_ioctl_pong(msg, msg_len);
            break;
        case MAPS_CMD_SET_BSS:
            ret = maps_set_bss_conf(msg, msg_len, vap);
            break;
        case MAPS_CMD_SET_CONFIG:
            ret = maps_set_config(msg, msg_len);
            break;
        case MAPS_CMD_SET_RADIO:
            ret = maps_set_radio_conf(msg, msg_len);
            break;
        case MAPS_CMD_ADD_RADIO:
            ret = maps_add_radio_vap(msg, msg_len, vap);
            break;
        case MAPS_CMD_SET_STA:
            ret = maps_set_sta_conf(msg, msg_len);
            break;
        case MAPS_CMD_DUMP_CONFIG:
            ret = maps_dump_config();
            break;
        case MAPS_CMD_DUMP_RADIO:
            ret = maps_dump_radios();
            break;
        case MAPS_CMD_DUMP_STA:
            ret = maps_dump_stas();
            break;
        case MAPS_CMD_CLEAN:
            ret = maps_clean();
            break;
        case MAPS_CMD_DBG:
            ret = maps_set_dbg(msg, msg_len);
            break;
        case MAPS_CMD_BSS_TRANS:
            ret = maps_bss_trans_req(msg, msg_len, vap);
            break;
        case MAPS_CMD_BEACON_REQ:
            ret = maps_beacon_req(msg, msg_len, vap);
            break;
        default:
            maps_err("invalid command %d", head->type);
            ret = -EOPNOTSUPP;
            break;
    }

    spin_unlock_bh(&s_steer_lock);
    return ret;
}

static void maps_nl_input(struct sk_buff *skb)
{
    maps_msg_head_t *head;
    struct nlmsghdr *nlh = nlmsg_hdr(skb);
    int msg_len;

    msg_len = nlh->nlmsg_len - NLMSG_HDRLEN - sizeof(maps_msg_head_t);
    if (msg_len < 0) {
        maps_err("wrong msg size %d", nlh->nlmsg_len);
        return;
    }

    head = (maps_msg_head_t *)NLMSG_DATA(nlh);
    if (0 != memcmp(head->magic, MAPS_MSG_MAGIC, MAPS_MSG_MAGIC_LEN)) {
        maps_err("wrong magic!");
        return;
    }

    switch (head->type) {
        case MAPS_EV_PING:
            maps_nl_unicast(MAPS_API_VERSION, sizeof(MAPS_API_VERSION),
                            nlh->nlmsg_pid, MAPS_EV_PONG);
            break;
        default:
            maps_err("unsupported event %d", head->type);
            break;
    }

    return;
}

/**
 * netlink初始化
*/
static void maps_init_nl(void)
{

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)

    struct netlink_kernel_cfg cfg = {
        .groups = MAPS_NETLINK_GROUP,
        .input  = maps_nl_input,
    };

    if (NULL != td_maps_nl_sock) {
        return;
    }

    td_maps_nl_sock = netlink_kernel_create(&init_net, MAPS_NETLINK, &cfg);
#else
    
    td_maps_nl_sock = netlink_kernel_create(&init_net, MAPS_NETLINK, MAPS_NETLINK_GROUP, maps_nl_input, NULL, THIS_MODULE);
#endif

    if (NULL == td_maps_nl_sock) {
        maps_err("create netlink sock fail!");
        return;
    } 
    
    return;
}

void maps_init(void)
{
    
	struct timer_list *timer;
    /* 准备hash种子 */
    get_random_bytes(&td_maps_hash_rnd, sizeof(td_maps_hash_rnd));
    
    maps_init_nl();
    maps_init_variable();
    timer = &td_maps_timer;
    if (!timer_pending(timer)) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
        init_timer(timer);
        timer->function = maps_timer_handler;
#else
        timer_setup(timer, maps_timer_handler, 0);
#endif
        timer->expires = jiffies + MAPS_TIMER_INTERVAL * HZ;
        add_timer(timer);
    }

    maps_dbg("td_multiap_steer init finished.");

    return;
}

void maps_exit(void)
{
    if (NULL == td_maps_config) {
        return;
    }
    
    ((maps_config_t *)td_maps_config)->enable = false;
    
    maps_dbg("td_multiap_steer exit.");
    mdelay(MAPS_CLEAN_DELAY);

    del_timer(&td_maps_timer);

    if (td_maps_nl_sock) {
        netlink_kernel_release(td_maps_nl_sock);
        td_maps_nl_sock = NULL;
    }
    
    if (td_maps_radios) {
        kfree(td_maps_radios);
        td_maps_radios = NULL;
    }
    
    if (td_maps_sta_list) {
        kfree(td_maps_sta_list);
        td_maps_sta_list = NULL;
    }
    
    if (td_maps_config) {
        kfree(td_maps_config);
        td_maps_config = NULL;
    }
    
    if (td_maps_mng_bss) {
        kfree(td_maps_mng_bss);
        td_maps_mng_bss = NULL;
    }

    return;
}

