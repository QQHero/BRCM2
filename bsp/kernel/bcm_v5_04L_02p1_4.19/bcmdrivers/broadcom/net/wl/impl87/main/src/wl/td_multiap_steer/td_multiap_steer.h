/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Sunjiajun
  生成日期   : 2019年12月
  最近修改   :

  功能描述   : 本文件存放 本模块实现的需要原厂驱动调用的钩子函数, 以及 本模块通用的定义. 
            本模块(td_multiap_steer)和原厂相关源文件会包含此头文件. 
******************************************************************************/
#ifndef _TD_MULTIAP_STEER_H_
#define _TD_MULTIAP_STEER_H_

#include <linux/kernel.h>
#include "td_maps_public.h"

#if defined(CONFIG_RTL8192CD)

#ifndef TD_8832BR_WIFI6
typedef struct rtl8192cd_priv vendor_vap_t;
typedef struct stat_info vendor_sta_t;
#else
typedef struct _ADAPTER vendor_vap_t;
typedef struct sta_info vendor_sta_t;

#endif

#elif defined(SIFLOWER) 
typedef struct siwifi_sta vendor_sta_t;
typedef struct siwifi_vif vendor_vap_t;


#elif defined(VENDOR_QCA)
typedef struct ieee80211_node vendor_sta_t;
typedef struct ieee80211vap vendor_vap_t;

#elif defined(BROADCOM)
#define TD_MAX_BEACON_REPORT  (32)

typedef struct td_11k_beacon_measurement_rep_info
{
    unsigned char op_class;
    unsigned char channel;
    unsigned char RCPI;
    unsigned char RSNI;
    unsigned char bssid[ETHER_ADDR_LEN];
}td_11k_beacon_measurement_rep_info_t;

typedef struct td_11k_rrm_info {
    unsigned char beacon_report_num;
    td_11k_beacon_measurement_rep_info_t beacon_report[TD_MAX_BEACON_REPORT];
}td_11k_rrm_info_t;

typedef struct scb vendor_sta_t;
typedef wlc_bsscfg_t vendor_vap_t;

#else
#error unsupported driver!
#endif

/* defined in td_multiap_steer.c */
extern int td_maps_dbg;


#define maps_info(fmt, args...) \
do {\
    if (unlikely(td_maps_dbg >= MAPS_LOG_INFO)) {\
        printk("td_multiap_steer: INFO %s(%d), "fmt"\n", __func__, __LINE__, ##args);\
    }\
} while (0)

#define maps_dbg(fmt, args...) \
do {\
    if (unlikely(td_maps_dbg >= MAPS_LOG_DEBUG)) {\
        printk("td_multiap_steer: DBG %s(%d), "fmt"\n", __func__, __LINE__, ##args);\
    }\
} while (0)

#define maps_dump(fmt, args...) \
do {\
    if (unlikely(td_maps_dbg >= MAPS_LOG_DUMP)) {\
        printk("td_multiap_steer: DUMP %s(%d), "fmt"\n", __func__, __LINE__, ##args);\
    }\
} while (0)

#define maps_dump_hex(addr, len) \
do {    \
    if (unlikely(td_maps_dbg >= MAPS_LOG_DUMP)) {     \
        int i;  \
        if (addr!=NULL && len>0) {    \
            printk("\n%s(%d): addr=%p, len=%d\n", __func__, __LINE__, (void *)addr, len); \
            for (i = 0; i < len; i++) { \
                printk("%02X%c", *((unsigned char *)addr + i), i==len-1?'\n':   \
                    (i+1)%16==0?'\n':' ');  \
            }   \
        }   \
    }   \
}while(0)

#define maps_err(fmt, args...) \
printk("td_multiap_steer: ERROR %s(%d), "fmt"\n", __func__, __LINE__, ##args)

#ifdef BROADCOM
void maps_timer_handler(wlc_info_t *wlc);
#endif
int maps_ioctl_handler(void *data, size_t len, vendor_vap_t* vap);
void maps_init(void);
void maps_exit(void);

#ifndef CONFIG_RTL8192CD
/* 为避免头文件交叉依赖, RTL中下面声明放到8192cd_util.h */
void maps_disassoc_handler(vendor_sta_t *vsta, vendor_vap_t *vap);
void maps_assoc_handler(vendor_sta_t *vsta, vendor_vap_t *vap);
#endif

/**
 * 认证处理函数
 * 返回值: 0 接受认证; 1 拒绝认证
*/
int maps_auth_handler(unsigned char *mac, signed char rssi, vendor_vap_t *vap);

/**
 * probe request处理函数
 * 返回值: 0 回复probe response; 1 不回复probe response
*/
int maps_probe_req_handler(unsigned char *mac, signed char rssi, vendor_vap_t *vap, 
                            bool ht, bool vht, bool he, unsigned char channel);

/* 用于通知应用层收到BSS transition response */
void maps_bss_tran_res_handler(vendor_sta_t *vsta, vendor_vap_t *vap, unsigned char status);

/* 用于通知应用层收到Beacon Report */
void maps_beacon_rep_handler(vendor_sta_t *vsta, vendor_vap_t *vap);

/* 接口 down/up 或 create/destroy 事件 */
void maps_vap_available_handler(vendor_vap_t *vap);
void maps_vap_unavailable_handler(vendor_vap_t *vap);
int maps_set_dbg(void *msg, size_t msg_len);
#endif

