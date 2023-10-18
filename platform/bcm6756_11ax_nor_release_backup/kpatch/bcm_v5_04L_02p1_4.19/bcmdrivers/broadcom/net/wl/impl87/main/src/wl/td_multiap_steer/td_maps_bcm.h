/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Liuke
  生成日期   : 2020年6月1日
  最近修改   :

  功能描述   : BCM数据结构访问及操作
            本文件内声明或定义的所有函数入参合法性由调用者保障. 
            对于较短小的函数, 为提高调用效率, 直接在本文件内以内联形式实现;
            对于较长或较复杂的函数, 实现在 td_maps_rtl.c, 本文件中仅提供其声明.
            本头文件仅被本模块(td_multiap_steer)包含, 应用层(steerd)及原厂相关
            源文件不应包含本头文件.
******************************************************************************/

#ifndef _TD_MULTIAP_STEER_RTL_H
#define _TD_MULTIAP_STEER_RTL_H

#include <wlc.h>
#include <wlc_types.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>
#include <wlc_wnm.h>




#include "td_maps_api.h"
#include "td_multiap_steer.h"

#define BCM_RSSI_OFFSET 95
#define BCM_RATE_SAMPLE_CNT 5
#define BCM_LOAD_SAMPLE_CNT 5

typedef struct bcm_adapt_sta {
    u_int64_t rx_bytes;
    u_int64_t tx_bytes;
    u_int8_t load; /* air time occupied by this station as a percentage */
    u_int32_t throughput; /* throughput in byte/s */
    u_int32_t tx_rate_samples[BCM_RATE_SAMPLE_CNT];
    u_int32_t rx_rate_samples[BCM_RATE_SAMPLE_CNT];
    unsigned int rate_sample_idx;
    u_int8_t load_samples[BCM_LOAD_SAMPLE_CNT];
    unsigned int load_sample_idx;
    unsigned long update_time;
} adapt_sta_t;


/* ================== 以下函数相对短小, 为提高调用效率, 定义为内联 =================== */

/* 返回客户端rssi, 单位：dBm. 范围: -100 ~ -1dBm. 失败时返回0 */
static inline signed char vendor_get_sta_rssi(vendor_sta_t *scb)
{
    return scb->rssi;
}

/* vsta 是否支持BTM */
static inline bool vendor_get_sta_btm(vendor_sta_t *scb)
{
    uint32 cap = 0;

    cap = wlc_wnm_get_scbcap(SCB_WLC(scb), scb);
   
    return cap? TRUE: FALSE;
}

/* vsta 是否支持RRM */
static inline bool vendor_get_sta_rrm(vendor_sta_t *scb)
{

    return SCB_RRM(scb);
}

static inline unsigned char* vendor_get_sta_mac(vendor_sta_t *scb)
{
    
    return (unsigned char*)&scb->ea.octet;
}

static inline unsigned char vendor_get_bss_index(vendor_vap_t* bsscfg)
{

    return bsscfg->td_maps_bss_index;
}

static inline void vendor_set_bss_index(vendor_vap_t* bsscfg, unsigned char index)
{
    bsscfg->td_maps_bss_index = index;
}

static inline unsigned char vendor_get_radio_index(vendor_vap_t* bsscfg)
{
    return bsscfg->td_maps_radio_index;
}

static inline void vendor_set_radio_index(vendor_vap_t* bsscfg, unsigned char index)
{
    bsscfg->td_maps_radio_index = index;
}

/* 是否为邻居节点 */
static inline bool vendor_is_sta_neighbor(vendor_sta_t *scb)
{
#if defined(AI_MESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
    return scb->is_ai_mesh;
#endif
    return false;
}

static inline unsigned char vendor_get_sta_num(vendor_vap_t *bsscfg)
{
    struct scb_iter scbiter;
    struct scb *scb;
    unsigned char num = 0;

    FOREACH_BSS_SCB(bsscfg->wlc->scbstate, &scbiter, bsscfg, scb) {
        if(SCB_ASSOCIATED(scb)){
            num++;
        }
    }

    return num;
}

static inline bool vendor_sta_associated(vendor_sta_t *scb)
{
    
    return (SCB_WLC(scb)->pub->now - scb->used < SCB_SHORT_TIMEOUT);
}

static inline void vendor_get_bssid(vendor_vap_t *bsscfg, u_int8_t *mac)
{

    if ((BSSCFG_STA(bsscfg) && !bsscfg->associated)) {
        memcpy(mac, "/x00/x00/x00/x00/x00/x00", ETHER_ADDR_LEN);
    }else{
        memcpy(mac, bsscfg->BSSID.octet, ETHER_ADDR_LEN);
    }
    
    return ;
}

/* 客户端是否支持11n */
static inline bool vendor_get_sta_ht(vendor_sta_t *scb)
{
    
    return SCB_HT_CAP(scb);
}

/* 客户端是否支持11ac */
static inline bool vendor_get_sta_vht(vendor_sta_t *scb)
{

    return SCB_VHT_CAP(scb);
}

static inline bool vendor_get_sta_he(vendor_sta_t *scb)
{
    return SCB_HE_CAP(scb);
}

static inline void vendor_clean(void)
{
    /* nothing to do for BCM */
}

/* ======== 以下函数相对较复杂, 不定义为内联, 实现位于td_maps_rtl.c, 下面提供其声明 ======== */

void vendor_iterate_sta(vendor_vap_t* bsscfg, 
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

unsigned char vendor_get_sta_load(vendor_sta_t *vsta, adapt_sta_t *asta);

unsigned int vendor_get_sta_flow(vendor_sta_t *scb, adapt_sta_t *asta);

unsigned char vendor_get_sta_nss(vendor_sta_t *scb);

unsigned char vendor_get_channel(vendor_vap_t* bsscfg);

unsigned char vendor_get_op_class(vendor_vap_t *bsscfg);

#ifdef TD_STEER_SUPPORT_6G
unsigned int vendor_get_frequency(vendor_vap_t* bsscfg);
#endif

#endif
