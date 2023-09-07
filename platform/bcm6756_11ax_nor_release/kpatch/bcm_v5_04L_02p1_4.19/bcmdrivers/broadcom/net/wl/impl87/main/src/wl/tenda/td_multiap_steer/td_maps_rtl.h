/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Sunjiajun
  生成日期   : 2019年8月15日
  最近修改   :

  功能描述   : RTK数据结构访问及操作
            本文件内声明或定义的所有函数入参合法性由调用者保障. 
            对于较短小的函数, 为提高调用效率, 直接在本文件内以内联形式实现;
            对于较长或较复杂的函数, 实现在 td_maps_rtl.c, 本文件中仅提供其声明.
            本头文件仅被本模块(td_multiap_steer)包含, 应用层(steerd)及原厂相关
            源文件不应包含本头文件.
******************************************************************************/

#ifndef _TD_MULTIAP_STEER_RTL_H
#define _TD_MULTIAP_STEER_RTL_H
#include "td_maps_api.h"
#include "td_multiap_steer.h"

/* for rtl structs */
#include "8192cd_cfg.h"
#include "8192cd.h"
#include "8192cd_util.h"

#define RTK_RSSI_OFFSET 100
#define RTK_VAP_2G  2


typedef struct rtl_adapt_sta {
    /* nothing to add for RTL */
} adapt_sta_t;

/* ================== 以下函数相对短小, 为提高调用效率, 定义为内联 =================== */

static inline unsigned char* vendor_get_sta_mac(vendor_sta_t *vsta)
{
    return vsta->cmn_info.mac_addr;
}

/* 返回客户端rssi, 单位：dBm. 范围: -(RTK_RSSI_OFFSET - 1) ~ -1dBm. 失败时返回0 */
static inline signed char vendor_get_sta_rssi(vendor_sta_t *vsta)
{
    if (vsta->rssi >= RTK_RSSI_OFFSET) {
        /* 某些代码可能会认为RSSI 0dBm为非法值, 因此此处返回-1dBm */
        return -1;
    }

    if (!vsta->rssi) {
        /* invalid rssi */
        return 0;
    }

    return ((signed char)vsta->rssi) - RTK_RSSI_OFFSET;
}

/* vsta 是否支持BTM */
static inline bool vendor_get_sta_btm(vendor_sta_t *vsta)
{
#ifdef CONFIG_IEEE80211V
    return !!(vsta->bssTransSupport);
#elif defined(HAPD_11V)
    return !!(vsta->bssTransSupportHAPD11v);
#else
    return false;
#endif
}

/* vsta 是否支持RRM */
static inline bool vendor_get_sta_rrm(vendor_sta_t *vsta)
{
#ifdef DOT11K
    /* Beacon Active Measurement */
    return !!(vsta->rm.rm_cap[0] & 0x20);
#elif defined(HAPD_11K)
    return !!(vsta->rm_cap_HAPD11k[0] & 0x20);
#else
    return false;
#endif
}

static inline unsigned char vendor_get_channel(vendor_vap_t* vap)
{
    /*
     * rtk 1500(8197H+8832br)方案下 会定义 CONFIG_RTL8832BR 这个宏
     * 而且2.4G 和 5G 驱动的编译都有这个宏定义,所以用 CONFIG_RTL8832BR
     * 这个宏来区分 8197H+8832br  和其他 rtk的方案
    */
#ifdef CONFIG_RTL8832BR
    if(vap->vap_type != RTK_VAP_2G) {
        return 0;
    } else
#endif
    {
         if (unlikely(!vap->pmib)) {
            maps_err("NULL pointer");
            return 0;
        }
        return vap->pmib->dot11RFEntry.dot11channel;
    }
   
}

static inline unsigned char vendor_get_bss_index(vendor_vap_t* vap)
{
    return vap->bss_index;
}

static inline void vendor_set_bss_index(vendor_vap_t* vap, unsigned char index)
{
    vap->bss_index = index;
}

static inline unsigned char vendor_get_radio_index(vendor_vap_t* vap)
{
    return vap->radio_index;
}

static inline void vendor_set_radio_index(vendor_vap_t* vap, unsigned char index)
{
    vap->radio_index = index;
}

/* 获取客户端占用的空口时间, 范围: 0%~100% */
static inline unsigned char vendor_get_sta_load(vendor_sta_t *vsta, 
                                                adapt_sta_t *unused)
{
    /* total_tx_time, total_rx_time: 1秒内 TX RX共用时间的毫秒数 */
    int load = (vsta->total_tx_time + vsta->total_rx_time) / 10;
    return (load > 100) ? 100 : load;
}

/* 获取客户端的流量, 单位: Byte/S */
static inline unsigned int vendor_get_sta_flow(vendor_sta_t *vsta, 
                                                adapt_sta_t *unused)
{
    return  (vsta->tx_bytes - vsta->prev_tx_byte) + 
            (vsta->rx_bytes - vsta->prev_rx_byte);
}

/* 是否为邻居节点 */
static inline bool vendor_is_sta_neighbor(vendor_sta_t *vsta)
{
#ifdef CONFIG_RTK_MESH
    return vsta->mesh_neighbor_TBL.State != 0;
#else
    return false;
#endif
}

static inline unsigned char vendor_get_sta_num(vendor_vap_t *vap)
{
    /*
     * rtk 1500(8197H+8832br)方案下 会定义 CONFIG_RTL8832BR 这个宏
     * 而且2.4G 和 5G 驱动的编译都有这个宏定义,所以用 CONFIG_RTL8832BR
     * 这个宏来区分 8197H+8832br  和其他 rtk的方案
    */
#ifdef CONFIG_RTL8832BR
    if(vap->vap_type != RTK_VAP_2G) {
        return 0;
    } else
#endif
    {
        return (unsigned char)vap->assoc_num;
    }
    
    
}

static inline bool vendor_sta_associated(vendor_sta_t *vsta)
{
    /* vsta->expire_to == 0 时老化. 将阈值提高以提前处理老化. */
    return vsta->expire_to > 10;
}

static inline void vendor_get_bssid(vendor_vap_t *vap, u_int8_t *mac)
{
    if (unlikely(!vap->dev) || unlikely(!vap->dev->dev_addr)) {
        maps_err("NULL pointer");
        return;
    }

    memcpy(mac, vap->dev->dev_addr, MAC_ADDR_LEN);
}

/* 客户端是否支持11n */
static inline bool vendor_get_sta_ht(vendor_sta_t *vsta)
{
    return vsta->ht_cap_len > 0;
}

/* 客户端是否支持11ac */
static inline bool vendor_get_sta_vht(vendor_sta_t *vsta)
{
#ifdef RTK_AC_SUPPORT
    return vsta->vht_cap_len > 0;
#else
    return false;
#endif
}

/* 客户端是否支持11ax */
static inline bool vendor_get_sta_he(vendor_sta_t *scb)
{
    /*Waiting for development*/
    return false;
}

static inline void vendor_clean(void)
{
    /* nothing to do for RTL */
}

/* ======== 以下函数相对较复杂, 不定义为内联, 实现位于td_maps_rtl.c, 下面提供其声明 ======== */

unsigned char vendor_get_sta_nss(vendor_sta_t *vsta);

void vendor_iterate_sta(vendor_vap_t* vap, 
                        void(*cb)(vendor_sta_t*, vendor_vap_t*, unsigned long),
                        unsigned long data);

unsigned char vendor_get_channel_load(vendor_vap_t** vaps, int vap_num);

void vendor_fill_beacon_rep(vendor_sta_t *vsta, 
                            vendor_vap_t *vap, 
                            maps_msg_beacon_rep_t* msg);

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

#endif
