/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Sunjiajun
  生成日期   : 2019年12月
  最近修改   :

  功能描述   : QCA数据结构访问及操作.
            本文件内声明或定义的所有函数入参合法性由调用者保障. 
            对于较短小的函数, 为提高调用效率, 直接在本文件内以内联形式实现;
            对于较长或较复杂的函数, 实现在 td_maps_qca.c, 本文件中仅提供其声明.
            本头文件仅被本模块(td_multiap_steer)包含, 应用层(steerd)及原厂相关
            源文件不应包含本头文件.
******************************************************************************/

#ifndef _TD_MULTIAP_STEER_QCA_H
#define _TD_MULTIAP_STEER_QCA_H
#include "td_maps_api.h"
#include "td_multiap_steer.h"

#include "ieee80211_var.h"
#include "ieee80211_node.h"

#include <linux/kernel.h>


#define QCA_RSSI_OFFSET 95
#define QCA_RATE_SAMPLE_CNT 5
#define QCA_LOAD_SAMPLE_CNT 5
#define QCA_BTM_CAP_BIT 0x00080000

typedef struct qca_adapt_sta {
    u_int64_t rx_bytes;
    u_int64_t tx_bytes;
    u_int8_t load; /* air time occupied by this station as a percentage */
    u_int32_t throughput; /* throughput in byte/s */
    u_int32_t tx_rate_samples[QCA_RATE_SAMPLE_CNT];
    u_int32_t rx_rate_samples[QCA_RATE_SAMPLE_CNT];
    unsigned int rate_sample_idx;
    u_int8_t load_samples[QCA_LOAD_SAMPLE_CNT];
    unsigned int load_sample_idx;
    unsigned long update_time;
} adapt_sta_t;

/* ================== 以下函数相对短小, 为提高调用效率, 定义为内联 =================== */

static inline unsigned char* vendor_get_sta_mac(vendor_sta_t *vsta)
{
    return vsta->ni_macaddr;
}

/* 返回: 客户端rssi, 单位: dBm. 范围: -(QCA_RSSI_OFFSET - 1) ~ -1dBm, 失败时返回0 */
static inline signed char vendor_get_sta_rssi(vendor_sta_t *vsta)
{
    if (vsta->ni_rssi >= QCA_RSSI_OFFSET) {
        /* 某些代码可能会认为RSSI 0dBm为非法值, 因此此处返回-1dBm */
        return -1;
    }

    if (!vsta->ni_rssi) {
        /* invalid rssi */
        return 0;
    }

    return ((signed char)vsta->ni_rssi) - QCA_RSSI_OFFSET;
}

/* 是否支持BTM */
static inline bool vendor_get_sta_btm(vendor_sta_t *vsta)
{
    /* BSS Transition Actived in little endian */

    unsigned int cap = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)

    cap = !!(vsta->ext_caps.ni_ext_capabilities & QCA_BTM_CAP_BIT);
#else

    cap = !!(vsta->ni_ext_capabilities & QCA_BTM_CAP_BIT);
#endif

    return cap;
}

/* vsta 是否支持RRM */
static inline bool vendor_get_sta_rrm(vendor_sta_t *vsta)
{
    return !!(vsta->ni_flags & IEEE80211_NODE_RRM);
}

static inline unsigned char vendor_get_channel(vendor_vap_t* vap)
{
    if (unlikely(!vap->iv_bsschan)) {
        return 0;
    }
    return vap->iv_bsschan->ic_ieee;
}

static inline unsigned char vendor_get_bss_index(vendor_vap_t* vap)
{
    return vap->td_maps_bss_index;
}

static inline void vendor_set_bss_index(vendor_vap_t* vap, unsigned char index)
{
    vap->td_maps_bss_index = index;
}

static inline unsigned char vendor_get_radio_index(vendor_vap_t* vap)
{
    return vap->td_maps_radio_index;
}

static inline void vendor_set_radio_index(vendor_vap_t* vap, unsigned char index)
{
    vap->td_maps_radio_index = index;
}

/* 是否为邻居节点 */
static inline bool vendor_is_sta_neighbor(vendor_sta_t *vsta)
{
#ifdef TD_AI_MESH_ENABLE
    return vsta->is_ai_mesh;
#else
    return false;
#endif
}

static inline unsigned char vendor_get_sta_num(vendor_vap_t *vap)
{
    return vap->iv_sta_assoc;
}

static inline bool vendor_sta_associated(vendor_sta_t *vsta)
{
    return (vsta->ni_associd) && !(vsta->ni_flags & IEEE80211_NODE_LEAVE_ONGOING);
}

static inline void vendor_get_bssid(vendor_vap_t *vap, u_int8_t *mac)
{
    memcpy(mac, vap->iv_myaddr, MAC_ADDR_LEN);
}

/* 客户端是否支持11n */
static inline bool vendor_get_sta_ht(vendor_sta_t *vsta)
{
    return !!(vsta->ni_flags & IEEE80211_NODE_HT);
}

/* 客户端是否支持11ac */
static inline bool vendor_get_sta_vht(vendor_sta_t *vsta)
{
    return !!(vsta->ni_flags & IEEE80211_NODE_VHT);
}

/* 客户端是否支持11ax */
static inline bool vendor_get_sta_he(vendor_sta_t *scb)
{
    /*Waiting for development*/
    return false;
}

/* 客户端支持几条流 */
static inline unsigned char vendor_get_sta_nss(vendor_sta_t *vsta)
{
    return vsta->ni_rxstreams;
}


/* ======== 以下函数相对较复杂, 不定义为内联, 实现位于td_maps_qca.c, 下面提供其声明 ======== */

void vendor_iterate_sta(vendor_vap_t* vap, 
                        void(*cb)(vendor_sta_t*, vendor_vap_t*, unsigned long),
                        unsigned long data);

int vendor_bss_trans(vendor_vap_t *vap,
                    unsigned char *sta, 
                    unsigned char *bss, 
                    unsigned char channel,
                    unsigned char op_class,
                    unsigned char phy_type);

int vendor_beacon_req(vendor_vap_t *vap, 
                    unsigned char *sta,
                    char *ssid, 
                    unsigned char channel, 
                    unsigned char op_class);

void vendor_fill_beacon_rep(vendor_sta_t *vsta, 
                            vendor_vap_t *vap, 
                            maps_msg_beacon_rep_t* msg);

unsigned char vendor_get_channel_load(vendor_vap_t** vaps, int vap_num);

unsigned char vendor_get_sta_load(vendor_sta_t *vsta, 
                                adapt_sta_t *asta);

unsigned int vendor_get_sta_flow(vendor_sta_t *vsta,
                                adapt_sta_t *asta);

void vendor_clean(void);


#endif /* _TD_MULTIAP_STEER_QCA_H */
