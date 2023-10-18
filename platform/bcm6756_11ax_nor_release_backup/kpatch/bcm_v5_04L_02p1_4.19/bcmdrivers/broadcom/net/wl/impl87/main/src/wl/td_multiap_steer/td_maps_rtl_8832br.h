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
#include "drv_types.h"
#define RTK_RSSI_OFFSET 100

#define RTK_VAP_5G  5


typedef struct rtl_adapt_sta {
    /* nothing to add for RTL */
} adapt_sta_t;

/* ================== 以下函数相对短小, 为提高调用效率, 定义为内联 =================== */

static inline unsigned char* vendor_get_sta_mac(vendor_sta_t *vsta)
{
    return vsta->phl_sta->mac_addr;
}

/* 返回客户端rssi, 单位：dBm. 范围: -(RTK_RSSI_OFFSET - 1) ~ -1dBm. 失败时返回0 */
signed char vendor_get_sta_rssi(vendor_sta_t *vsta);

/* vsta 是否支持BTM */
bool vendor_get_sta_btm(vendor_sta_t *vsta);

/* vsta 是否支持RRM */
static inline bool vendor_get_sta_rrm(vendor_sta_t *vsta)
{
    /* Beacon Active Measurement */
#ifdef CONFIG_RTW_80211K
    return !!(vsta->rm_en_cap[0]);
#else
    return false;
#endif
}

static inline unsigned char vendor_get_channel(vendor_vap_t* vap)
{
    if(vap->vap_type != RTK_VAP_5G) {
        return 0;
    } else {
        return vap->mlmeextpriv.cur_channel;
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
unsigned char vendor_get_sta_load(vendor_sta_t *vsta, adapt_sta_t *unused);

/* 获取客户端的流量, 单位: Byte/S */
unsigned int vendor_get_sta_flow(vendor_sta_t *vsta, adapt_sta_t *unused);


/* 是否为邻居节点 */
static inline bool vendor_is_sta_neighbor(vendor_sta_t *vsta)
{
    /*暂时没有实现*/
    return false;
}

static inline unsigned char vendor_get_sta_num(vendor_vap_t *vap)
{
    //vap 是 RTK_VAP_5G 类型才能获取sta num
    if(vap->vap_type != RTK_VAP_5G) {
        return 0;
    } else {
        //exclude AP itself for ap mode
        return vap->stapriv.asoc_sta_count <= 1 ? 0 : (unsigned char)(vap->stapriv.asoc_sta_count -1);
    }
}

static inline bool vendor_sta_associated(vendor_sta_t *vsta)
{
    /* vsta->expire_to == 0 时老化. 将阈值提高以提前处理老化. */
    return vsta->expire_to > 10;
}

void vendor_get_bssid(vendor_vap_t *vap, u_int8_t *mac);


/* 客户端是否支持11n */
static inline bool vendor_get_sta_ht(vendor_sta_t *vsta)
{
    return vsta->htpriv.ht_option;
}

/* 客户端是否支持11ac */
static inline bool vendor_get_sta_vht(vendor_sta_t *vsta)
{
    return vsta->vhtpriv.vht_option;
}

/* 客户端是否支持11ax */
static inline bool vendor_get_sta_he(vendor_sta_t *vsta)
{
    return vsta->hepriv.he_option;
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
