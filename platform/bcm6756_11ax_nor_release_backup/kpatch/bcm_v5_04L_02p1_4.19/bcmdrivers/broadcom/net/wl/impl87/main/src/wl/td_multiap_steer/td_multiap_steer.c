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

#if defined(CONFIG_RTL8192CD)
/*
*rtl8832br改动较大td_maps_rtl.h中代码重用率不高这里暂时拆分成两个文件
*/
#ifdef TD_8832BR_WIFI6
#include "td_maps_rtl_8832br.h"
#else
#include "td_maps_rtl.h"
#endif
#elif defined(VENDOR_QCA)
#include "td_maps_qca.h"
#elif defined(BROADCOM)
#include "td_maps_bcm.h"
#elif defined(SIFLOWER)
#include "td_maps_sf.h"
#else
#error unsupported driver!
#endif

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
#define MAPS_RADIO_CYCLE            8
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
#ifdef BROADCOM
    unsigned char op_class;     /* operation class */
#endif
} maps_radio_t;

typedef struct maps_bss {
    vendor_vap_t *vap;
    u_int32_t ess_radios;
} maps_bss_t;


/*
* rtl8197h_8832br_8367rb_11ax方案使用的3.6.0版本的原厂sdk 2.4G和5G驱动分离,rtk_wifi6.ko
* rtl8192cd.ko 导致本文件需要在两个驱动中都编译，这样带来了 td_maps_nl_sock 这个sock会被
* 重复创建。这里采用在5G驱动下创建 td_maps_nl_sock 并EXPORT_SYMBOL_GPL 导出符号
* 2.4G驱动下直接引用这个这sock
*
*  特别说明 rtk 1500(8197H+8832br)方案下 td_multiap_steer.c 和 tc_maps_rtl.c 会在
*  2.4G驱动的下编译(编译进8192cd.ko模块中)
*  td_multiap_steer.c 和 tc_maps_rtl_8832br.c 会在5G驱动下编译(编译进 rtk_wifi6.ko模块中)
*  这样就需要用 TD_8832BR_WIFI6 来区分当前的编译是2.4G还是5G
*  用  CONFIG_RTL8832BR 宏来区分是 rtk 1500(8197H+8832br) 方案还是其他
*/

/*
 * rtk 1500(8197H+8832br)方案下 会定义 CONFIG_RTL8832BR 这个宏
 * 而且2.4G 和 5G 驱动的编译都有这个宏定义,所以用 CONFIG_RTL8832BR
 * 这个宏来区分 8197H+8832br  和其他 rtk的方案
*/
#ifdef CONFIG_RTL8832BR

#ifdef TD_8832BR_WIFI6
/*
 *rtk 1500(8197H+8832br)方案下(5G驱动部分) 先定义如下变量，然后 EXPORT_SYMBOL_GPL 
 *导出这些变量
*/
struct sock *td_maps_nl_sock = NULL;
maps_config_t td_maps_config = {.enable = false};
maps_sta_t td_maps_sta_list[MAPS_MAX_NODES];
struct hlist_head td_maps_sta_h_heads[MAPS_HASH_SIZE];
int td_maps_dbg = MAPS_LOG_ERROR;
maps_radio_t td_maps_radios[MAPS_MAX_RADIO_INDEX + 1];
maps_bss_t td_maps_mng_bss[MAPS_MAX_BSS_INDEX + 1];
u_int32_t td_maps_hash_rnd;
EXPORT_SYMBOL_GPL(td_maps_nl_sock);
EXPORT_SYMBOL_GPL(td_maps_config);
EXPORT_SYMBOL_GPL(td_maps_sta_list);
EXPORT_SYMBOL_GPL(td_maps_sta_h_heads);
EXPORT_SYMBOL_GPL(td_maps_dbg);
EXPORT_SYMBOL_GPL(td_maps_radios);
EXPORT_SYMBOL_GPL(td_maps_mng_bss);
EXPORT_SYMBOL_GPL(td_maps_hash_rnd);
#else
/*
 *rtk 1500(8197H+8832br)方案下(2.4G驱动部分) 直接引用这些变量 
*/
extern struct sock *td_maps_nl_sock;
extern maps_config_t td_maps_config ;
extern maps_sta_t td_maps_sta_list[MAPS_MAX_NODES];
extern struct hlist_head td_maps_sta_h_heads[MAPS_HASH_SIZE];
extern int td_maps_dbg;
extern maps_radio_t td_maps_radios[MAPS_MAX_RADIO_INDEX + 1];
extern maps_bss_t td_maps_mng_bss[MAPS_MAX_BSS_INDEX + 1];
extern u_int32_t td_maps_hash_rnd;

#endif //TD_8832BR_WIFI6

/*
 *其他方案 2.4G vap和5G vap结构体一样的情况下直接沿用原来的逻辑
*/
#else 
static struct sock *td_maps_nl_sock = NULL;
static maps_config_t td_maps_config = {.enable = false};
static maps_sta_t td_maps_sta_list[MAPS_MAX_NODES];
static struct hlist_head td_maps_sta_h_heads[MAPS_HASH_SIZE];
int td_maps_dbg = MAPS_LOG_ERROR;
static maps_radio_t td_maps_radios[MAPS_MAX_RADIO_INDEX + 1];
static maps_bss_t td_maps_mng_bss[MAPS_MAX_BSS_INDEX + 1];
/* hash种子 */
static u_int32_t td_maps_hash_rnd;
#endif //CONFIG_RTL8832BR

#ifndef BROADCOM
static struct timer_list s_timer;
#endif
/* 用于处理客户端查找, 添加和老化时的并行问题 */
static DEFINE_SPINLOCK(s_steer_lock);



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

    /* 查找可以回收使用的maps_sta_node_t结构体 */
    for (i = 0; i < MAPS_MAX_NODES; i++) {
        tpos = &td_maps_sta_list[i];

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

#ifdef TD_STEER_SUPPORT_6G
static char *prefer_band_to_str(maps_band_prefer_e prefer)
{
    switch (prefer) {
        case MAPS_PREFER_2G:
            return "2.4G";
        case MAPS_PREFER_5G:
            return "5G";
        case MAPS_PREFER_6G:
            return "6G";
        case MAPS_PREFER_DUAL_BAND:
            return "dual-band";
        default:
            return "NONE";
    }

    return "NONE";
}

/**三频合一时，且当前radio为5G频段时，意味6G不符合要求，
不需要考虑大于hrssi_5时会触发pre_assoc high rssi_event，则：
1、5G的reject_rssi_upper不为0时，使用reject_rssi_upper值判断是否符合连接5G的rssi标准；
2、5G的reject_rssi_upper为0时，使用MAPS_RSSI_DUMMY值判断是否符合连接5G的rssi标准。
**/
static void _should_block_process_5G(maps_radio_t *radio,
                            maps_band_prefer_e *prefer,
                            signed char rssi,
                            bool *flag)
{
    /*首次进入_should_block函数，为5G第三种情况（3、同时开启pre_down与pre_up）处理逻辑*/
    if (MAPS_PREFER_NONE == *prefer && RSSI_BOTTOM != radio->reject_rssi_lower && 
        RSSI_TOP != radio->reject_rssi_upper) {
        if (rssi < radio->reject_rssi_lower &&
            rssi > radio->reject_rssi_upper) 
        {
            *flag = false;
        } else {
            *flag = true;
        }
    }
    /*非首次进入_should_block函数，prefer为5G，rssi确保在5G合适范围内*/
    if (MAPS_PREFER_5G == *prefer || MAPS_PREFER_DUAL_BAND == *prefer) {
        if (RSSI_TOP != radio->reject_rssi_upper) {
            if (rssi > radio->reject_rssi_upper) {
                *flag = false;
            } else {
                *flag = true;
            }
        }

        if (RSSI_TOP == radio->reject_rssi_upper) {
            if (rssi > MAPS_RSSI_DUMMY) {
                *flag = false;
            } else {
                *flag = true;
            }
        }
    }

    return;
}

/*
###此处的 tri-band(三频合一)，5G有以下情况：
1、开启pre_down；不开启pre_up(同dual-band)
reject_rssi_lower : -100    reject_rssi_upper: -69
  1)、reject_rssi_lower < rssi < reject_rssi_upper, do pre_down

2、不开启pre_down；开启pre_up
reject_rssi_lower : -45    reject_rssi_upper: 0
  1)、reject_rssi_lower < rssi < reject_rssi_upper， do pre_up

3、同时开启pre_down与pre_up(lower与upper的值大小与逻辑相反)
reject_rssi_lower : -45    reject_rssi_upper: -69
  1) 、rssi > reject_rssi_lower, do pre_up
  2)、rssi < reject_rssi_upper, do pre_down
4、其它的合理情况(不触发pre_rssi_event)不会进入到此函数再来处理，在(_should_block)中已经处理完成

2.4与6G：
1、只有2.4G与6G各自radio的lower < rssi < upper，即2.4满足pre_up阈值，6G满足pre_down阈值
的情况，才会进入此函数，其他情况已经在(_should_block)中已经处理完成

bandty             2.4G   5G(dual-band/tri-band)  5G(tri-band)   6G
reject_rssi_lower  -55           -100                -45/-45    -100
reject_rssi_upper   0            -69                 -69/0      -55
*/
static void maps_parse_preassoc_rssi_event_5G(maps_radio_t *radio, bool *rssi_flag, signed char rssi)
{
    /*将5G的rssi_flag为true代表pre_up，为false代表pre_down*/
    if (RSSI_BOTTOM == radio->reject_rssi_lower && *rssi_flag) {
        /*5G 情况1，此处rssi_flag一定为true，此时将rssi_flag置为false作为pre_down的flag*/
        *rssi_flag = false;
    } else if (RSSI_TOP != radio->reject_rssi_upper && RSSI_BOTTOM != radio->reject_rssi_lower) {
        if (rssi < radio->reject_rssi_upper) {
            /*5G 情况3，开启pre_down且rssi满足down的阈值*/
            *rssi_flag = false;
        } else if (rssi > radio->reject_rssi_lower) {
            /*5G 情况3，开启pre_up且rssi满足up的阈值*/
            *rssi_flag = true;
        } else {}
    } else if (RSSI_TOP == radio->reject_rssi_upper && *rssi_flag) {
        /*5G 情况2，开启pre_up ,此处rssi_flag一定为true*/
        *rssi_flag = true;
    } else {}

    maps_dbg("rssi = (%d)dbm, lower = (%d)dbm, upper = (%d)dbm, rssi_flag = %s\n", 
        rssi, radio->reject_rssi_lower, radio->reject_rssi_upper, *rssi_flag ? "true" : "false");

    return;
}

/*
    priority of pre-association upgrade-steering strategy:
*1）、当前sta属于5G频段，优先连接6G频段
*2）、当前sta属于2.4G频段，优先连接6G频段，次之连接5G频段迁移

    priority of pre-association downgrade-steering strategy:
*1）、当前sta属于6G频段，优先连接5G频段，次之连接2.4G频段
*2）、当前sta属于5G频段，优先连接2.4G频段
*/
static void maps_preassoc_rssi_event_select_band(maps_radio_t *radio,
                                            maps_sta_t *sta,
                                            maps_band_prefer_e *prefer,
                                            signed char rssi)
{
    /*通过首次进入_should_block函数处理后，执行到此处可得知2.4G与6G的rssi_flag一定为true，5G情况较复杂*/
    bool rssi_flag = (rssi > radio->reject_rssi_lower) && (rssi < radio->reject_rssi_upper);

    if (CAP_5G(radio->work_band)) {
        maps_parse_preassoc_rssi_event_5G(radio, &rssi_flag, rssi);
    }

    if (rssi_flag) {
        /*Pre-association upgrade-steering*/
        if (IS_2G(radio->work_band)) {
            if (CAP_6G(sta->bands)) {
                *prefer = MAPS_PREFER_6G;
            } else if (CAP_5G(sta->bands)) {
                *prefer = MAPS_PREFER_5G;
            } else {}
            goto end;
        } else if (CAP_5G(radio->work_band)) {
            if (CAP_6G(sta->bands)) {
                *prefer = MAPS_PREFER_6G;
                goto end;
            }
        } else {}
    } 

    /*post-association downgrade-steering*/
    if (CAP_5G(radio->work_band) && !rssi_flag) {
        if (CAP_2G(sta->bands)) {
            *prefer = MAPS_PREFER_2G;
            goto end;
        }
    } else if (IS_6G(radio->work_band) && rssi_flag) {
        if (CAP_5G(sta->bands)) {
            *prefer = MAPS_PREFER_5G;
        } else if (CAP_2G(sta->bands)) {
            *prefer = MAPS_PREFER_2G;
        } else {}
        goto end;
    }

end:
    return;
}

/*
    priority of pre-association offload-steering strategy:
*1）、当前sta属于6G频段，优先连接5G频段，次之再连接2.4G
*2）、当前sta属于5G_H频段，优先连接6G频段，再使用双频旧规格(5L/2.4G随机选择一个连接)
*3）、当前sta属于5G_L频段，优先连接6G频段，再使用双频旧规格(5H/2.4G随机选择一个连接)
*3）、当前sta属于2.4G频段，优先连接6G频段，次之连接5G频段
*/
static void maps_preassoc_offload_select_band(maps_radio_t *radio, 
                                        maps_sta_t *sta, 
                                        maps_band_prefer_e *prefer)
{
    bool load_base = (MAPS_BAND_BLOCK == radio->mode);
    if (!load_base) {
        return;
    }
        /*offload-steering*/
    if (IS_2G(radio->work_band)) {
        if (CAP_6G(sta->bands)) {
            *prefer = MAPS_PREFER_6G;
        } else if (CAP_5G(sta->bands)) {
            *prefer = MAPS_PREFER_5G;
        } else {}
        goto end;
    } else if (CAP_5G(radio->work_band)) {
        /*当前radio在5G，则sta支持5G, 默认也支持2.4G*/
        *prefer = MAPS_PREFER_DUAL_BAND;
        if (CAP_6G(sta->bands)) {
            *prefer = MAPS_PREFER_6G;
        }
        goto end;
    } else if (IS_6G(radio->work_band)) {
        if (CAP_5G(sta->bands)) {
            *prefer = MAPS_PREFER_5G;
        } else if (CAP_2G(sta->bands)) {
            *prefer = MAPS_PREFER_2G;
        } else {}
        goto end;
    } else {
        maps_err("sta "MACSTR" associated with an illegal bandtype", MAC2STR(sta->mac));
    }

end:
    return;
}
#endif

/*
dual-band(双频合一)/tri-band(三频合一)时radio->mode为MAPS_BAND_BLOCK_RSSI
时各频段pre-association阶段rssi阈值如下(双频和三频合一的5G处理逻辑存在差异):

一、dual-band(双频合一)时5G只考虑pre_down情况：
1、开启pre_down
reject_rssi_lower : -100    reject_rssi_upper: -69

二、tri-band(三频合一)时5G只考虑以下情况：
1、开启pre_down；不开启pre_up(同dual-band的5G)
reject_rssi_lower : -100    reject_rssi_upper: -69
2、不开启pre_down；开启pre_up
reject_rssi_lower : -45    reject_rssi_upper: -0
3、同时开启pre_down与pre_up(lower与upper的值大小与逻辑相反)
reject_rssi_lower : -45    reject_rssi_upper: -69

2.4G(tri-band/dual-band)，2.4G只考虑pre_up情况:
1、开启pre_down/不开启pre_down；开启pre_up
reject_rssi_lower : -55    reject_rssi_upper: 0

6G(tri-band)只考虑pre_down情况:
1、开启pre_down；开启/不开启pre_up
reject_rssi_lower : -100    reject_rssi_upper: -55

bandtype           2.4G   5G(dual-band/tri-band)  5G(tri-band)   6G
reject_rssi_lower  -55           -100                -45/-45    -100
reject_rssi_upper   0            -69                 -69/0      -55
*/
static bool _should_block(
                        maps_band_prefer_e prefer,
                        maps_sta_t *sta,
                        signed char rssi,
                        unsigned char bss_index,
                        maps_radio_t *radio)
{
#ifdef TD_STEER_SUPPORT_6G
    bool flag = false;
#endif

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
#ifdef TD_STEER_SUPPORT_6G
            if (td_maps_config.mode && CAP_5G(radio->work_band)) {
                _should_block_process_5G(radio, &prefer, rssi, &flag);
                return flag;
            }
#endif
            /**1、双频合一5G与2.4G处理逻辑；
              2、三频合一时6G与2.4G以及5G第1、2种情况(1、仅开启pre_down；2、仅开启pre_up)处理逻辑**/
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
    int i;

    ess_radios = td_maps_mng_bss[bss_index].ess_radios;
    /* MAPS_BLOCK_DEFAULT */
    maps_for_each_radio(radio, td_maps_radios, i) {
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

    radio = &td_maps_radios[radio_index];

    if (!IS_2G(radio->work_band)) {
        return false;
    }

    /* sta no need hold */
    if (!sta->hold_delay) {
        return false;
    }

    /* 用户关联数量没有达到阀值*/
    if (radio->sta_num < td_maps_config.delay_reject_stanum) {
        return false;
    }

    if (!maps_has_available_essid(bss_index, radio_index)) {
        return false;
    }

    if ((DELAY_REJECT_PROBE == reject_type) && td_maps_config.enable_delay_reject_probe) {
        return true;
    } else if ((DELAY_REJECT_AUTH == reject_type) && td_maps_config.enable_delay_reject_auth) {
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
    u_int32_t ess_radios;
    int i;
    maps_band_prefer_e prefer = MAPS_PREFER_NONE;

    if (!maps_valid_bss_index(bss_index) ||
        !maps_valid_radio_index(radio_index)) {
        return false;
    }

    radio = &td_maps_radios[radio_index];
    ess_radios = td_maps_mng_bss[bss_index].ess_radios;
    /* MAPS_BLOCK_DEFAULT */
    if (!_should_block(prefer, sta, rssi, bss_index, radio))
    {
        return false;
    }

    if (MAPS_BLOCK_BSS == sta->mode) {
        /* post-association steering, 由steerd确保存在可连接的BSS, 跳过下面的安全检查 */
        return true;
    }
#ifdef TD_STEER_SUPPORT_6G
    if (td_maps_config.mode) {
        maps_preassoc_rssi_event_select_band(radio, sta, &prefer, rssi);
        maps_preassoc_offload_select_band(radio, sta, &prefer);

        /*没找到更好的band*/
        if (MAPS_PREFER_NONE == prefer) {
            goto exit;
        }
        maps_info("sta %pM connecting with band:0x%2x now, but it prefer band is %s", 
            sta->mac, radio->work_band, prefer_band_to_str(prefer));
    }
retry:
#endif
    maps_for_each_radio(radio_tmp, td_maps_radios, i) {
        if (i == radio_index) {
            /* 本频段 */
            continue;
        }

        if (!(ess_radios & (1 << i))) {
            /* 此BSS所属的ESS没有用到索引为i的radio */
            continue;
        }

        if (sta->bands & radio_tmp->work_band) {
#ifdef TD_STEER_SUPPORT_6G
            if (td_maps_config.mode) {
                if ((MAPS_PREFER_2G == prefer && !IS_2G(radio_tmp->work_band)) ||
                    (MAPS_PREFER_5G == prefer && !CAP_5G(radio_tmp->work_band)) ||
                    (MAPS_PREFER_6G == prefer && !IS_6G(radio_tmp->work_band)) ||
                    (MAPS_PREFER_DUAL_BAND == prefer && IS_6G(radio_tmp->work_band))) {
                        continue;
                }
            }
#endif
            if (!_should_block(prefer, sta, rssi, bss_index, radio_tmp))
            {
                /* pre-association steering, 必须保证其它频段可以连接才能拒绝连接 */
                return true;
            }
        }
    }
#ifdef TD_STEER_SUPPORT_6G
    if (td_maps_config.mode) {
        /*pre_association steering strategy , refer to function(maps_preassoc_rssi_event_select_band 
            and maps_preassoc_offload_select_band)*/
        if (MAPS_PREFER_6G == prefer && CAP_5G(sta->bands)) {
            prefer = MAPS_PREFER_5G;
            goto retry;
        } else if (MAPS_PREFER_5G == prefer && CAP_2G(sta->bands)) {
            prefer = MAPS_PREFER_2G;
            goto retry;
        } else {}
    }
exit:
#endif
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
#ifdef TD_STEER_SUPPORT_6G
    unsigned int freq = 0;
#endif

    if (!mac || !rssi || !vap) {
        maps_err("invalid argument:%s%s%s", mac ? "" : " STA mac", rssi ? "" : " STA rssi", vap ? "" : " vap");
        return 0;
    }

    if (!td_maps_config.enable) {
        return 0;
    }

    spin_lock_bh(&s_steer_lock);
    bss_index = vendor_get_bss_index(vap);
    radio_index = vendor_get_radio_index(vap);
#ifdef TD_STEER_SUPPORT_6G
    freq = vendor_get_frequency(vap);
    band = maps_frequency_to_band(freq);
#else
    band = maps_channel_to_band(channel);
#endif
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
    if (td_maps_config.enable_reject_probe && 
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
#ifdef TD_STEER_SUPPORT_6G
    unsigned int freq = 0;
#endif
    int reject = 0;

    if (!mac || !rssi || !vap) {
        maps_err("invalid argument");
        return 0;
    }

    if (!td_maps_config.enable) {
        return 0;
    }

    spin_lock_bh(&s_steer_lock);
    bss_index = vendor_get_bss_index(vap);
    channel = vendor_get_channel(vap);
    radio_index = vendor_get_radio_index(vap);
#ifdef TD_STEER_SUPPORT_6G
    freq = vendor_get_frequency(vap);
    band = maps_frequency_to_band(freq);
#else
    band = maps_channel_to_band(channel);
#endif
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
    if (td_maps_config.enable_reject_auth && 
        maps_should_block(sta, rssi, bss_index, radio_index)) {
        reject = 1;
    }

    if (maps_2G_should_delay_reject(sta, rssi, bss_index, radio_index, DELAY_REJECT_AUTH)) {
        maps_info("bss%u radio%d delay reject auth of "MACSTR, bss_index, radio_index, MAC2STR(mac));
        reject = 1;
    }

    if (reject) {
        if (sta->reject_cnt >= td_maps_config.reject_max_cnt) {
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
#ifdef TD_STEER_SUPPORT_6G
    unsigned int freq = 0;
#endif

    if (vendor_is_sta_neighbor(vsta)) {
        return;
    }

    bss_index = vendor_get_bss_index(vap);
    if (!maps_valid_bss_index(bss_index)) {
        /* bss_index为0表示此vap未开启steering. */
        return;
    }

    channel = vendor_get_channel(vap);
#ifdef TD_STEER_SUPPORT_6G
    freq = vendor_get_frequency(vap);
    band = maps_frequency_to_band(freq);
#else
    band = maps_channel_to_band(channel);
#endif
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

    if (!td_maps_config.enable) {
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
#ifdef TD_STEER_SUPPORT_6G
    unsigned int freq = 0;
#endif

    if (!vsta || !vap) {
        maps_err("invalid argument");
        return;
    }

    if (!td_maps_config.enable) {
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
#ifdef TD_STEER_SUPPORT_6G
    freq = vendor_get_frequency(vap);
    band = maps_frequency_to_band(freq);
#else
    band = maps_channel_to_band(channel);
#endif
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

    if (!td_maps_config.enable) {
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

    if (!td_maps_config.enable) {
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

    if (!vap) {
        maps_err("invalid arguments");
        return;
    }

    if (!td_maps_config.enable) {
        return;
    }

    spin_lock_bh(&s_steer_lock);
    maps_report_vap_update(vap);

    /* 将其从s_managed_bss中移除 */
    for (i = 1; i <= MAPS_MAX_BSS_INDEX; i++) {
        if (vap == td_maps_mng_bss[i].vap) {
            td_maps_mng_bss[i].vap = NULL;
            maps_dbg("unmanage BSS%d", i);
        }
    }

    /* 将其从s_radios中移除 */
    maps_for_each_radio(radio, td_maps_radios, i) {
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
    unsigned char bss_index;
    unsigned char radio_index;
    int i;

    if (!vap) {
        maps_err("invalid arguments");
        return;
    }

    if (!td_maps_config.enable) {
        return;
    }

    spin_lock_bh(&s_steer_lock);
    maps_report_vap_update(vap);

    bss_index = vendor_get_bss_index(vap);
    maps_dbg("BSS%u is up", bss_index);

    if (maps_valid_bss_index(bss_index) && !td_maps_mng_bss[bss_index].vap) {
        td_maps_mng_bss[bss_index].vap = vap;
        maps_dbg("manage BSS%u", bss_index);
    }

    radio_index = vendor_get_radio_index(vap);
    if (maps_valid_radio_index(radio_index)) {
        radio = &td_maps_radios[radio_index];

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

    if (vendor_sta_associated(vsta)) {
        sta_num = vendor_get_sta_num(vap);
        maps_sta_join(vsta, vap, sta_num);
    }

    return;
}

/**
 * 为vap设置其索引, 并设置相关配置
*/
static int maps_set_bss_conf(void *msg, size_t msg_len, vendor_vap_t *vap)
{
    maps_msg_bss_conf_t *bssc;
    unsigned char index;
    int i;

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
            if (vap == td_maps_mng_bss[i].vap) {
                td_maps_mng_bss[i].vap = NULL;
                maps_dbg("remove bss%d", i);
                return 0;
            }
        }

        maps_err("the bss is not managed");
        return -1;
    } else {
        if (NULL != td_maps_mng_bss[index].vap) {
            maps_err("bss %u is already managed", index);
            return -1;
        }

        td_maps_mng_bss[index].vap = vap;
        td_maps_mng_bss[index].ess_radios = bssc->ess_radios;
        maps_dbg("add bss%u", index);
        vendor_iterate_sta(vap, _join_sta, 0);
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

    conf = (maps_config_t *)msg;

    memcpy(&td_maps_config, conf, sizeof(maps_config_t));
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

    radio = &td_maps_radios[radio_index];
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

    if (msg_len < sizeof(maps_msg_radio_add_t)) {
        maps_err("wrong msg size %d", msg_len);
        return -EINVAL;
    }

    radio_add = (maps_msg_radio_add_t *)msg;
    index = radio_add->index;
    if (!maps_valid_radio_index(index)) {
        return -EINVAL;
    }

    radio = &td_maps_radios[index];
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

#ifdef BROADCOM
    maps_radio_t *radio;
    int i;
    
    maps_for_each_radio(radio, td_maps_radios, i){
        if (channel == radio->channel){
            return radio->op_class;
        }
    }

    maps_err("channel %d operation class not found", channel);
    return 0;
#else
    /* TODO 添加US, EU, JP的Regulatory class */
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
#endif
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

#ifdef BROADCOM
    phy_type = 0;
#else
    /* TODO 2.4G 不一定是HT(7), 5G也不一定是VHT(9) */
    phy_type = (btm_req->channel <= 14 ? 7 : 9);

#endif

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
    td_maps_config.enable = false;
    maps_dbg("flushing status...");
    /**
     * 将s_config.enable设为false后, 后续的中断或定时器会直接return, 
     * 不会造成并行问题. 对于正在运行的中断或定时器, 等待一段时间, 
     * 确保对下面变量的访问已经结束.
     * 
     * 由于IOCTL外面可能会被驱动加锁, 不能睡眠, 使用delay代替
    */
    mdelay(MAPS_CLEAN_DELAY);

    memset(&td_maps_config, 0, sizeof(td_maps_config));
    memset(&td_maps_mng_bss, 0, sizeof(td_maps_mng_bss));
    memset(&td_maps_radios, 0, sizeof(td_maps_radios));
    memset(&td_maps_sta_h_heads, 0, sizeof(td_maps_sta_h_heads));
    memset(&td_maps_sta_list, 0, sizeof(td_maps_sta_list));
    vendor_clean();

    return 0;
}

static int maps_dump_radios(void)
{
    maps_radio_t *radio;
    int i;

    maps_for_each_radio(radio, td_maps_radios, i) {
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
        if (td_maps_mng_bss[i].vap) {
            printk("vap[%d] ess_radios=0x%x\n",i, td_maps_mng_bss[i].ess_radios);
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
    printk("\n");
    printk("[configurations]\n");
    printk("enable=%d\n", td_maps_config.enable);
    printk("mode=%d\n", td_maps_config.mode);
    printk("enable_reject_probe=%d\n", td_maps_config.enable_reject_probe);
    printk("enable_reject_auth=%d\n", td_maps_config.enable_reject_auth);
    printk("probe_age=%d\n", td_maps_config.probe_age);
    printk("reject_age=%d\n", td_maps_config.reject_age);
    printk("reject_max_cnt=%d\n", td_maps_config.reject_max_cnt);
    printk("active_slot=%d\n", td_maps_config.active_slot);
    printk("active_flow=%d\n", td_maps_config.active_flow);
    printk("enable_delay_reject_probe=%d\n", td_maps_config.enable_delay_reject_probe);
    printk("enable_delay_reject_auth=%d\n", td_maps_config.enable_delay_reject_auth);
    printk("delay_reject_age=%ds\n", td_maps_config.delay_reject_age);
    printk("delay_reject_stanum=%d\n", td_maps_config.delay_reject_stanum);

    return 0;
}

static void maps_probe_age(maps_sta_t *tpos)
{
    int j;

    if (tpos->associated || !td_maps_config.probe_age) {
        return;
    }

    for (j = 0; j < MAPS_BAND_NUM; j++) {
        if (maps_timeout(tpos->band_time[j], td_maps_config.probe_age)) {
            if (maps_test_bit(tpos->bands, 1 << j)) {

                maps_reset_bit(tpos->bands, 1 << j);
                tpos->hold_delay = 0;
                maps_dbg("band %d record of "MACSTR" is aged",
                         j, MAC2STR(tpos->mac));
            }
        }

        /* add begin by dengxingde, 2020-04-20, 原因: 2G延时接入，老化 */
        if (maps_timeout(tpos->create_time, td_maps_config.delay_reject_age)) {
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
    if (tpos->reject_cnt && 
        maps_timeout(tpos->reject_time, td_maps_config.reject_age)) {

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
static void maps_sta_monitor(vendor_sta_t *vsta, 
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
    radio = &td_maps_radios[radio_index];

    /* 更新客户端空口占用率 */
    sta->load_sum += vendor_get_sta_load(vsta, &sta->asta);
    if (0 == (times % MAPS_STA_CYCLE)) {
        sta->load = (unsigned char)(sta->load_sum / MAPS_STA_CYCLE);
        sta->load_sum = 0;
        if (sta->load > 100) {
            sta->load = 100;
        }
    }

    /* 更新客户端active状态 */
    old_active = sta->active;
    flow = vendor_get_sta_flow(vsta, &sta->asta);
    if (flow > td_maps_config.active_flow) {

        sta->active_time = jiffies;
        if (!sta->active) {
            maps_dbg(MACSTR " is active(flow:%uB/s)", MAC2STR(sta->mac), flow);
            sta->active = true;
        }

    } else if (sta->active && 
        maps_timeout(sta->active_time, td_maps_config.active_slot)) {
        
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
    if (cur_rssi > radio->monitor_rssi_high ||
        cur_rssi < radio->monitor_rssi_low ||
        abs(sta->last_rssi - cur_rssi) > MAPS_RSSI_REP_DIFF ||
        old_active != sta->active ||
        0 == times % MAPS_STA_CYCLE) {

        sta->last_rssi = cur_rssi;
        maps_report_sta(sta, vsta, bss_index, sta_num, MAPS_EV_STA_STATUS);
    }

    return;
}

/**
 * radio状态监控
 * 
 * 参数: 
 * times --- 定时器触发的次数
*/
#ifdef BROADCOM
static void maps_radio_monitor(unsigned long times, wlc_info_t *wlc)
#else
static void maps_radio_monitor(unsigned long times)
#endif
{
    maps_radio_t *radio;
    vendor_vap_t *vap;
    unsigned char sta_num, sta_num_old;
    unsigned int load;
#ifdef TD_STEER_SUPPORT_6G
    unsigned int freq = 0;
#endif
    unsigned char old_channel;
    maps_band_shift_e band_shift;
    int i, j;

    maps_for_each_radio(radio, td_maps_radios, i) {
        if (!radio->vaps[0] 
#ifdef BROADCOM
        || radio->vaps[0]->wlc != wlc
#endif
        ) {
            continue;
        }

        /* 更新信道占用率 */
        load = vendor_get_channel_load(radio->vaps, MAPS_VAP_PER_RADIO);
        radio->load_sum += load;
        if (0 == (times % MAPS_RADIO_CYCLE)) {
            radio->load = (unsigned char)(radio->load_sum / MAPS_RADIO_CYCLE);
            radio->load_sum = 0;
            if (radio->load > 100) {
                radio->load = 100;
            }
        }

        /* 更新信道 */
        old_channel = radio->channel;
        radio->channel = vendor_get_channel(radio->vaps[0]);
        /* 获取到的信道是0   则 将old_channel      赋值给 radio->channel*/
        if(radio->channel == 0) {
            radio->channel = old_channel;
        }
#ifdef TD_STEER_SUPPORT_6G
        freq = vendor_get_frequency(radio->vaps[0]);
        band_shift = maps_frequency_to_band(freq);
#else
        band_shift = maps_channel_to_band(radio->channel);
#endif
        if (!maps_valid_band(band_shift)) {
            maps_err("unsupported channel %d.", radio->channel);
            continue;
        }
        radio->work_band = TBIT(band_shift);

#ifdef BROADCOM
        /* 更新operation class */
        radio->op_class = vendor_get_op_class(radio->vaps[0]);
#endif
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
        if (old_channel != radio->channel || 
            sta_num_old != radio->sta_num ||
            0 == (times % MAPS_RADIO_CYCLE)) {
            
            maps_report_radio(i, radio, MAPS_EV_RADIO_STATUS);
        }

    }

    return;
}

#ifdef BROADCOM
void maps_timer_handler(wlc_info_t *wlc)
{
    int i;
    vendor_vap_t *vap;
    /* 用于记录各频段定时器触发次数 */
    static unsigned long s_times[MAPS_MAX_RADIO_INDEX] = {0};

    if (!td_maps_config.enable) {
        return;
    }
    spin_lock_bh(&s_steer_lock);
    maps_sta_age_handler();

    /* 有效索引从1开始 */
    for (i = 1; i <= MAPS_MAX_BSS_INDEX; i++) {
        vap = td_maps_mng_bss[i].vap;
        if (NULL == vap || vap->wlc != wlc) {
            continue;
        }

        vendor_iterate_sta(vap, maps_sta_monitor, s_times[wlc->pub->unit]);
    }

    maps_radio_monitor(s_times[wlc->pub->unit], wlc);
    spin_unlock_bh(&s_steer_lock);
    s_times[wlc->pub->unit]++;

    return;

}
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
static void maps_timer_handler(unsigned long data)
#else
static void maps_timer_handler(struct timer_list *t)
#endif
{
    int i;
    vendor_vap_t *vap;

    /* 用于记录定时器触发次数 */
    static unsigned long s_times = 0;
    if (!td_maps_config.enable) {
        mod_timer(&s_timer, jiffies + MAPS_TIMER_INTERVAL * HZ);
        return;
    }
    spin_lock_bh(&s_steer_lock);
    maps_sta_age_handler();

        /* 有效索引从1开始 */
        for (i = 1; i <= MAPS_MAX_BSS_INDEX; i++) {
            vap = td_maps_mng_bss[i].vap;
            if (NULL == vap) {
                continue;
            }

            vendor_iterate_sta(vap, maps_sta_monitor, s_times);
        }

    maps_radio_monitor(s_times);
    spin_unlock_bh(&s_steer_lock);
    s_times++;
    mod_timer(&s_timer, jiffies + MAPS_TIMER_INTERVAL * HZ);

    return;
}
#endif

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

    /* BCM no need copy here */
#ifdef BROADCOM
    head = (maps_msg_head_t *)data;
#else
    if (copy_from_user(buff, data, len)) {
        maps_err("copy from user failed!");
        return -EFAULT;
    }
    head = (maps_msg_head_t *)buff;
#endif

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

    if (0 == ret) {
#ifdef BROADCOM
        /* BCM no need copy here */
#else
        if (copy_to_user(data, buff, len)) {
            maps_err("copy to user failed!");
            ret = -EFAULT;
        }
#endif
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

    /*
     * rtk 1500(8197H+8832br)方案下 会定义 CONFIG_RTL8832BR 这个宏
     * 而且2.4G 和 5G 驱动的编译都有这个宏定义,所以用 CONFIG_RTL8832BR
     * 这个宏来区分 8197H+8832br  和其他 rtk的方案
    */
#ifdef CONFIG_RTL8832BR
    if(NULL != td_maps_nl_sock) {
        return ;
    }
#endif

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
#ifndef BROADCOM
    struct timer_list *timer;
#endif
    /* 准备hash种子 */


    /*
     * rtk 1500(8197H+8832br)方案下 会定义 CONFIG_RTL8832BR 这个宏
     * 而且2.4G 和 5G 驱动的编译都有这个宏定义,所以用 CONFIG_RTL8832BR
     * 这个宏来区分 8197H+8832br  和其他 rtk的方案
    */
#ifdef CONFIG_RTL8832BR
#ifdef TD_8832BR_WIFI6
    get_random_bytes(&td_maps_hash_rnd, sizeof(td_maps_hash_rnd));
#endif
#else //非8197H+8832br 方案按照原来的处理
    get_random_bytes(&td_maps_hash_rnd, sizeof(td_maps_hash_rnd));
#endif

    maps_init_nl();

#ifndef BROADCOM
    timer = &s_timer;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
    init_timer(timer);
    timer->function = maps_timer_handler;
#else
    timer_setup(timer, maps_timer_handler, 0);
#endif
    timer->expires = jiffies + MAPS_TIMER_INTERVAL * HZ;
    add_timer(timer);
#endif
    maps_dbg("td_multiap_steer init finished.");

    return;
}

void maps_exit(void)
{
    td_maps_config.enable = false;
#ifndef BROADCOM
    if (timer_pending(&s_timer)) {
        del_timer(&s_timer);
    }
#endif
    maps_dbg("td_multiap_steer exit.");
    mdelay(MAPS_CLEAN_DELAY);

    if (td_maps_nl_sock) {
        netlink_kernel_release(td_maps_nl_sock);
        td_maps_nl_sock = NULL;
    }

    return;
}

