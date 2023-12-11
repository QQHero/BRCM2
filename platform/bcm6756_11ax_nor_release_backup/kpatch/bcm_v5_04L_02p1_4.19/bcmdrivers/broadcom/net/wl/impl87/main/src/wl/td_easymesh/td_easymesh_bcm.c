/*****************************************************************************
Copyright (C), 吉祥腾达，保留所有版权
File name ：td_easymesh_bcm.c
Description : easymesh
Author ：qinke@tenda.cn
Version ：v1.0
Date ：2020.5.10
*****************************************************************************/

#include <wlc.h>
#include <wlc_types.h>
#include <wlc_bsscfg.h>
#include <wlc_scb.h>
#include <wlc_wnm.h>
#include <wlc_iocv.h>
#include <wlc_lq.h>
#include <wlc_channel.h>
#include <wlc_macfltr.h>
#include <wlc_ht.h>
#include <wlc_vht.h>
#include <wlc_txbf.h>
#include <wlc_ampdu.h>
#include <wlc_rspec.h>
#include <wl_cfg80211.h>
#include <wl_linux.h>
#include <bcmwifi_channels.h>
#include <wlc_scan.h>
#include <wlc_scan_utils.h>
#include <td_easymesh.h>
#include "td_easymesh_dbg.h"

#include "easymesh_shared.h"
#include "td_easymesh_interface.h"
#include "td_easymesh_opclass.h"
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
#include "td_debug.h"
#endif

#ifndef WL_NUMCHANNELS
#define WL_NUMCHANNELS      64
#endif
#define TD_DOT11_BSSTYPE_ANY     2  /* d11 any BSS type */
#define TD_EM_CH_BW_MAX_LEN      6
#define TD_EM_MAX_SSID_LEN       32
#define TD_CHSCAN_RES_SCANTYPE 0x80
#define TD_MAPS_VAP_PER_RADIO    8
#define TD_ESP_BE                1
#define TD_DATA_FORMAT_SHIFT     3
#define TD_BA_WSIZE_SHIFT        5
#define TD_ESP_AMSDU_ENABLED    (1 << TD_DATA_FORMAT_SHIFT)
#define TD_ESP_AMPDU_ENABLED    (2 << TD_DATA_FORMAT_SHIFT)
#define TD_ESP_BA_WSIZE_NONE    (0 << TD_BA_WSIZE_SHIFT)
#define TD_ESP_BA_WSIZE_2       (1 << TD_BA_WSIZE_SHIFT)
#define TD_ESP_BA_WSIZE_4       (2 << TD_BA_WSIZE_SHIFT)
#define TD_ESP_BA_WSIZE_6       (3 << TD_BA_WSIZE_SHIFT)
#define TD_ESP_BA_WSIZE_8       (4 << TD_BA_WSIZE_SHIFT)
#define TD_ESP_BA_WSIZE_16      (5 << TD_BA_WSIZE_SHIFT)
#define TD_ESP_BA_WSIZE_32      (6 << TD_BA_WSIZE_SHIFT)
#define TD_ESP_BA_WSIZE_64      (7 << TD_BA_WSIZE_SHIFT)

#define TD_NSS_2                2
#define TD_NSS_3                3
#define TD_NSS_4                4
#define TD_NSS_8                8
/* HT RX Streams is represented by bits 5 and 4  */
#define TD_AP_HTCAP_RX_NSS_1        0x00000000  /* Bit 5 = 0 and bit 4 = 0 (00000000) */
#define TD_AP_HTCAP_RX_NSS_2        0x00000010  /* Bit 5 = 0 and bit 4 = 1 (00010000) */
#define TD_AP_HTCAP_RX_NSS_3        0x00000020  /* Bit 5 = 1 and bit 4 = 0 (00100000) */
#define TD_AP_HTCAP_RX_NSS_4        0x00000030  /* Bit 5 = 1 and bit 4 = 1 (00110000) */
/* HT TX Streams is represented by bits 7 and 6  */
#define TD_AP_HTCAP_TX_NSS_1        0x00000000  /* Bit 7 = 0 and bit 6 = 0 (00000000) */
#define TD_AP_HTCAP_TX_NSS_2        0x00000040  /* Bit 7 = 0 and bit 6 = 1 (01000000) */
#define TD_AP_HTCAP_TX_NSS_3        0x00000080  /* Bit 7 = 1 and bit 6 = 0 (10000000) */
#define TD_AP_HTCAP_TX_NSS_4        0x000000C0  /* Bit 7 = 1 and bit 6 = 1 (11000000) */
/* VHT RX Streams is represented by bits 4, 3 and 2 */
#define TD_AP_VHTCAP_RX_NSS_1       0x00000000  /* Bit 4 = 0, Bit 3 = 0, Bit 2 = 0 (00000000) */
#define TD_AP_VHTCAP_RX_NSS_2       0x00000004  /* Bit 4 = 0, Bit 3 = 0, Bit 2 = 1 (00000100) */
#define TD_AP_VHTCAP_RX_NSS_3       0x00000008  /* Bit 4 = 0, Bit 3 = 1, Bit 2 = 0 (00001000) */
#define TD_AP_VHTCAP_RX_NSS_4       0x0000000C  /* Bit 4 = 1, Bit 3 = 1, Bit 2 = 0 (00001100) */
#define TD_AP_VHTCAP_RX_NSS_8       0x0000001C  /* Bit 4 = 0, Bit 3 = 0, Bit 2 = 0 (00011100) */
/* VHT RX Streams is represented by bits 7, 6 and 5 */
#define TD_AP_VHTCAP_TX_NSS_1       0x00000000  /* Bit 7 = 0, Bit 6 = 0, Bit 5 = 0 (00000000) */
#define TD_AP_VHTCAP_TX_NSS_2       0x00000020  /* Bit 7 = 0, Bit 6 = 0, Bit 5 = 1 (00100000) */
#define TD_AP_VHTCAP_TX_NSS_3       0x00000040  /* Bit 7 = 0, Bit 6 = 1, Bit 5 = 0 (01000000) */
#define TD_AP_VHTCAP_TX_NSS_4       0x00000060  /* Bit 7 = 0, Bit 6 = 1, Bit 5 = 1 (01100000) */
#define TD_AP_VHTCAP_TX_NSS_8       0x000000E0  /* Bit 7 = 1, Bit 6 = 1, Bit 5 = 1 (11100000) */
/* HE RX Streams is represented by bits 4, 3 and 2 */
#define TD_AP_HECAP_RX_NSS_1        0x00000000  /* Bit 4 = 0, Bit 3 = 0, Bit 2 = 0 (00000000) */
#define TD_AP_HECAP_RX_NSS_2        0x00000004  /* Bit 4 = 0, Bit 3 = 0, Bit 2 = 1 (00000100) */
#define TD_AP_HECAP_RX_NSS_3        0x00000008  /* Bit 4 = 0, Bit 3 = 1, Bit 2 = 0 (00001000) */
#define TD_AP_HECAP_RX_NSS_4        0x0000000C  /* Bit 4 = 0, Bit 3 = 1, Bit 2 = 1 (00001100) */
#define TD_AP_HECAP_RX_NSS_8        0x0000001C  /* Bit 4 = 1, Bit 3 = 1, Bit 2 = 1 (00011100) */
/* HE RX Streams is represented by bits 7, 6 and 5 */
#define TD_AP_HECAP_TX_NSS_1        0x00000000  /* Bit 7 = 0, Bit 6 = 0, Bit 5 = 0 (00000000) */
#define TD_AP_HECAP_TX_NSS_2        0x00000020  /* Bit 7 = 0, Bit 6 = 0, Bit 5 = 1 (00100000) */
#define TD_AP_HECAP_TX_NSS_3        0x00000040  /* Bit 7 = 0, Bit 6 = 1, Bit 5 = 0 (01000000) */
#define TD_AP_HECAP_TX_NSS_4        0x00000060  /* Bit 7 = 0, Bit 6 = 1, Bit 5 = 1 (01100000) */
#define TD_AP_HECAP_TX_NSS_8        0x000000E0  /* Bit 7 = 1, Bit 6 = 1, Bit 5 = 1 (11100000) */

#define MAX_SCAN_STA_NUM                           64
#define SUP_ABILITY                                1
#define UNSUP_ABILITY                              0
#define TD_VHT_CAP_MCS_MAP_NSS_MAX                 8
#define TD_VHT_CAP_MCS_MAP_NONE                    3
#define TD_MCS_TABLE_SIZE                          33
#define TD_MCSSET_LEN                              16  /* 16-bits per 8-bit set to give 128-bits bitmap of MCS Index */
#define TD_VHT_CAP_MCS_MAP_NONE                    3
#define TD_VHT_CAP_MCS_MAP_S                       2 /* num bits for 1-stream */
#define TD_VHT_CAP_MCS_MAP_M                       0x3 /* mask for 1-stream */
#define TDNBBY                                     8   /* 8 bits per byte */
#define TD_VHT_CAP_INFO_SGI_80MHZ                  0x00000020
#define TD_VHT_CAP_INFO_SGI_160MHZ                 0x00000040
#define TD_VHT_CAP_INFO_SU_BEAMFMR                 0x00000800
#define TD_VHT_CAP_INFO_MU_BEAMFMR                 0x00080000
#define TD_TXBF_HE_SU_BFR_CAP                      0x04
#define TD_TXBF_HE_MU_BFR_CAP                      0x08

#define TD_CH_BW_10         "10"
#define TD_CH_BW_20         "20"
#define TD_CH_BW_40         "40"
#define TD_CH_BW_80         "80"
#define TD_CH_BW_160        "160"
#define TD_CH_BW_8080       "80+80"

#define RM_TIME                (100)
#define RM_REQUEST_MODE        (0)
#define RM_RANDOM_INTERVAL     (0)
#define RM_MAX_CHANNEL_NUM     (255)
#define RM_REPORT_DETATIL_LEN  (1)
#define BKT_MAX_BUF_128        (128)
#define BIT1                   (0x00000002)
#define BIT2                   (0x00000004)
#define TD_EM_KICK_STA_TIME    (100)
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
#define TD_EM_HB_TIMEOUT       (72)
extern void* g_em_bss[TD_EM_MAX_BAND];
typedef struct easymesh_timer {
    struct wl_timer *hb_timer;
    wl_info_t * wl;
}easymesh_timer_t;
static easymesh_timer_t g_hb_timer[TD_EM_MAX_BAND];
#endif

enum td_em_measurement_result {
    MEASUREMENT_UNKNOWN = 0,
    MEASUREMENT_PROCESSING = 1,
    MEASUREMENT_SUCCEED = 2,
    MEASUREMENT_INCAPABLE = 3,
    MEASUREMENT_REFUSED = 4,    
    MEASUREMENT_RECEIVED = 5,    
};

#define EM_CONTROLLER_BIT  0x1
#define EM_XMESH_BIT       0x2
#define EM_PRIO_ASSOC_BIT  0x4
#define EM_ETHX_BIT        0x8
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
#define EM_LINKUP          0x1

static void td_em_hbtimer_handler(void *arg);
static void td_em_init_hbtimer(em_osif osif);
static void td_em_refresh_hbtimer(wlc_info_t *wlc, struct wl_timer * t);
#endif

void em_drv_set_cfg(int cfg)
{
    g_em_mesh_cfg |= cfg;

    return;
}

void em_drv_clr_cfg(int cfg)
{
    g_em_mesh_cfg &= (~cfg);

    return;
}


int em_drv_get_cfg(void)
{
    return g_em_mesh_cfg;
}

#ifndef BIT
#define BIT(x) (1 << (x))
#endif
/* The following defines are copied from wlc_pub.h */
#define TD_MU_FEATURES_MUTX        (1 << 0)
#define TD_MU_FEATURES_MURX        (1 << 1)
#define td_isset(a, i) (((const uint8 *)a)[(i) / TDNBBY] & (1 << ((i) % TDNBBY)))

#define TD_GET_PPDU_TIME(rate) ((1500 * 8) / (rate * 50))
#define TD_VHT_MCS_MAP_GET_SS_IDX(nss) (((nss)-1) * TD_VHT_CAP_MCS_MAP_S)
#define TD_VHT_MCS_MAP_GET_MCS_PER_SS(nss, mcsMap) \
    (((mcsMap) >> TD_VHT_MCS_MAP_GET_SS_IDX(nss)) & TD_VHT_CAP_MCS_MAP_M)
#define TD_VHT_MCS_MAP_SET_MCS_PER_SS(nss, numMcs, mcsMap) \
    do { \
     (mcsMap) &= (~(TD_VHT_CAP_MCS_MAP_M << TD_VHT_MCS_MAP_GET_SS_IDX(nss))); \
     (mcsMap) |= (((numMcs) & TD_VHT_CAP_MCS_MAP_M) << TD_VHT_MCS_MAP_GET_SS_IDX(nss)); \
    } while (0)
#define TD_VHT_MCS_SS_SUPPORTED(nss, mcsMap) \
                 (TD_VHT_MCS_MAP_SET_MCS_PER_SS((nss), (mcsMap)) != TD_VHT_CAP_MCS_MAP_NONE)

typedef enum td_vht_cap_chan_width {
    TD_VHT_CAP_CHAN_WIDTH_SUPPORT_MANDATORY = 0x00,
    TD_VHT_CAP_CHAN_WIDTH_SUPPORT_160       = 0x04,
    TD_VHT_CAP_CHAN_WIDTH_SUPPORT_160_8080  = 0x08
} td_vht_cap_chan_width_t;
    
typedef struct ssid_type {
      unsigned char SSID_len;
      unsigned char SSID[TD_EM_MAX_SSID_LEN + 1];
    } td_em_ssid_type;

typedef struct chscan_result_nbr_item {
    unsigned char nbr_bssid[TD_EM_MACADDRLEN]; /* The BSSID indicated by the neighboring BSS */
    td_em_ssid_type nbr_ssid; /* SSID of the neighboring BSS */
    unsigned char nbr_rcpi; /* SignalStrength : An indicator of radio signal strength (RSSI) of
    * the Beacon or Probe Response frames of the neighboring BSS as received by the radio
    * measured in dBm. (RSSI is encoded per Table 9-154 of [[1]). Reserved: 221 - 255
    */
    unsigned char ch_bw_length; /* Length of Channel Bandwidth field */
    unsigned char ch_bw[TD_EM_CH_BW_MAX_LEN]; /* ChannelBandwidth : String indicating the maximum bandwidth
    * at which the neighbor BSS is operating, e.g., 20 or 40 or 80 or 80+80 or 160 MHz. */
    unsigned char chscan_result_nbr_flag;  /* Channel Scan  Result TLV Neighbor Flags
    * of type MAP_CHSCAN_RES_NBR_XXX_XXX */
    unsigned char channel_utilization; /* ChannelUtilization : If MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT
    * bit is set to 1, this field is present. Otherwise it is omitted. */
    unsigned short station_count; // StationCount : If MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT
} td_em_chscan_result_nbr_item;

typedef struct chscan_result_item {
    //unsigned char radio_mac[TD_EM_MACADDRLEN]; /* Radio mac address */
    //unsigned char opclass; /* Operating Class */
    unsigned char channel; /* Channel : The channel number
    *of the channel scanned by the radio given the operating class */
    unsigned char scan_status_code; /* Scan Status : A status code to indicate
    * whether a scan has been performed successfully and if not, the reason for failure
    * values of type MAP_CHSCAN_STATUS_XXX */
    //unsigned char timestamp_length; /* Length of Timestamp Octets */
    //unsigned char timestamp[IEEE1905_TS_MAX_LEN]; /* Timestamp Octets */
    unsigned char utilization; /* Utilization : The current channel utilization measured
    * by the radio on the scanned 20 MHz channel - as defined in section 9.4.2.28 of [1] */
    unsigned char noise; /* Noise : An indicator of the average radio noise plus interference power
    * measured on the 20MHz channel during a channel scan.
    * Encoding as defined as for ANPI in section 11.11.9.4 of [1] */
    unsigned short num_of_neighbors; /* NumberOfNeighbors :
    * The number of neighbor BSS discovered on this channel */

    td_em_chscan_result_nbr_item *neighbor_list; /* List of ieee1905_chscan_result_nbr_item type objects */

    unsigned int aggregate_scan_duration; /* AggregateScanDuration : Total time spent
    * performing the scan of this channel in milliseconds. */
    unsigned char chscan_result_flag;  /* Channel Scan Request TLV Flags
    * of type MAP_CHSCAN_RES_XXX */
} td_em_chscan_result_item;

typedef struct td_em_scanresult {
    unsigned int channel_num;
    unsigned char count;
} td_em_scanresult_t;

void td_em_get_update_channelscanresults(em_osif osif, td_em_scanresult_list_t * list);

unsigned char td_em_rssitorcpi(unsigned char rssi)
{
    //convert per 100 to per 220
    return (2 * ((rssi) + 110));  //RCPI = 2 *(P + 110)
}

struct td_dot11k_beacon_measurement_report_info
{   unsigned char measure_token;
    unsigned char measure_mode;
    unsigned char measure_report_type;
    unsigned char op_class;
    unsigned char channel;
    unsigned int  measure_time_hi;
    unsigned int  measure_time_lo;    
    unsigned short measure_duration;
    unsigned char frame_info;
    unsigned char RCPI;
    unsigned char RSNI;
    unsigned char bssid[TD_EM_MACADDRLEN];    
    unsigned char antenna_id;
    unsigned int  parent_tsf;     
};

struct td_dot11k_beacon_measurement_report
{
    struct td_dot11k_beacon_measurement_report_info info;
    unsigned char subelements_len;    
    unsigned char subelements[MAX_BEACON_SUBLEMENT_LENG];
};


typedef struct td_dot11k_rrm_info {
    /* for issuing radio measurement request to and getting report from other STA*/
    unsigned char dialog_token;
    enum td_em_measurement_result measure_result;
    unsigned char beacon_report_num;
    struct td_dot11k_beacon_measurement_report beacon_report[MAX_BEACON_REPOR];
    unsigned char beacon_report_len[MAX_BEACON_REPOR];
    unsigned char beacon_measurement_token[MAX_BEACON_REPOR];
    unsigned char beacon_report_mode[MAX_BEACON_REPOR];
}td_dot11k_rrm_info_t;

bool vap_is_up(em_osif osif)
{
    if (!osif) {
        return 0;
    }

    if (osif->_psta) {
        return osif->enable;
    } else {
        return osif->up;
    }
}

static unsigned short wpa_swap_16(unsigned char *v)
{
    return (v[0] << 8| v[1]);
}

static unsigned int wpa_swap_32(unsigned char *v)
{
    return ((v[0] << 24) | (v[1] << 16) |(v[2] << 8) | v[3]);
}

/*****************************************************************************
 函 数 名  : td_rm_parse_beacon_report
 功能描述  : 解析Measurement Report中的beacon report
 输入参数  : dot11k_rrm_info_t *rrm_info
             unsigned char *pframe  
             int frame_len
 输出参数  : 无
 返 回 值  : 无
 
 修改历史      :
  1.日    期   : 2020年8月10日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_rm_parse_beacon_report(td_dot11k_rrm_info_t *rrm_info, unsigned char *pframe, int len, int frame_len)
{
    struct td_dot11k_beacon_measurement_report* beacon_rep = NULL;
    unsigned char element_len = pframe[len + 1];
    unsigned char subelement_len = 0;
    if (!rrm_info || !pframe) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    if(pframe[len + 3] == 0)  /*succeed*/
    {
        rrm_info->measure_result = MEASUREMENT_RECEIVED;
        if(element_len <= 3)
        {
            return len += 2 + element_len;
        }

        if(rrm_info->beacon_report_num < MAX_BEACON_REPOR)
        {
            beacon_rep = &rrm_info->beacon_report[rrm_info->beacon_report_num];
            memset(beacon_rep, 0x00, sizeof(struct td_dot11k_beacon_measurement_report));
            rrm_info->beacon_report_len[rrm_info->beacon_report_num] = element_len;
            rrm_info->beacon_measurement_token[rrm_info->beacon_report_num] = pframe[len+2];
            rrm_info->beacon_report_mode[rrm_info->beacon_report_num] = pframe[len+3];
            rrm_info->beacon_report_num++;
        }

        if(beacon_rep) {
            beacon_rep->info.measure_token = pframe[len + 2];
            beacon_rep->info.measure_mode = pframe[len + 3];
            beacon_rep->info.measure_report_type = pframe[len + 4];
            beacon_rep->info.op_class = pframe[len + 5];
            beacon_rep->info.channel = pframe[len + 6];
            beacon_rep->info.measure_time_lo = wpa_swap_32(&pframe[len + 7]);
            TD_EM_DBG_TRACE("Time-%x - %x - %x - %x \n", pframe[len + 7], pframe[len + 8],pframe[len + 9],pframe[len + 10]);
            beacon_rep->info.measure_time_hi = wpa_swap_32(&pframe[len + 11]);
            beacon_rep->info.measure_duration= wpa_swap_16(&pframe[len + 15]);
            TD_EM_DBG_TRACE("measure_duration-%x - %x \n", pframe[len + 15], pframe[len + 16]);
            beacon_rep->info.frame_info = pframe[len + 17];
            beacon_rep->info.RCPI = pframe[len + 18];
            beacon_rep->info.RSNI = pframe[len + 19];
            memcpy(beacon_rep->info.bssid, pframe + len + 20, TD_EM_MACADDRLEN);
            beacon_rep->info.antenna_id = pframe[len + 26];
            beacon_rep->info.parent_tsf = wpa_swap_32(&pframe[len + 27]);
            TD_EM_DBG_TRACE("parent_tsf-%x - %x - %x - %x \n", pframe[len + 27], pframe[len + 28],pframe[len + 29],pframe[len + 30]);
            TD_EM_DBG_TRACE("Receive beacon_report:SUCCESS, CH = %d, OP_CLASS = %d\n", beacon_rep->info.channel, beacon_rep->info.op_class);
            TD_EM_DBG_TRACE("Receive beacon_report:SUCCESS, RCPI = %d, RSNI = %d\n", beacon_rep->info.RCPI, beacon_rep->info.RSNI);
            TD_EM_DBG_TRACE("Receive beacon_report:SUCCESS, measure_time_lo = %08x \n", beacon_rep->info.measure_time_lo);
            TD_EM_DBG_TRACE("Receive beacon_report:SUCCESS, measure_time_hi = %08x \n", beacon_rep->info.measure_time_hi);
            TD_EM_DBG_TRACE("Receive beacon_report:SUCCESS, parent_tsf = %08x \n", beacon_rep->info.parent_tsf);
            TD_EM_DBG_TRACE("Receive beacon_report:SUCCESS, antenna_id = %d, measure_duration = %d\n", beacon_rep->info.antenna_id, beacon_rep->info.measure_duration);
        }

        /* parse subelements*/
        if(beacon_rep) {
            //偏移31,从802.11管理帧部分头部开始到第一个subelementid的长度为31字节
            len += 31;
            subelement_len = pframe[len + 1];
            //偏移2，subelement部分的长度需要包含subelement_id和length两个字段，共2字节
            beacon_rep->subelements_len = subelement_len + 2;
            //拷贝subelenment中的所有信息
            memcpy(beacon_rep->subelements, pframe + len, beacon_rep->subelements_len);
            //加上当前子元素的偏移量到总的偏移量,为下一次处理做准备
            len += beacon_rep->subelements_len;
        }
        return len;
    }
    else // MEASUREMENT_INCAPABLE or MEASUREMENT_REFUSED
    {   
        if (pframe[len + 3] & BIT1) {
            TD_EM_DBG_TRACE(" Receive beacon_report:MEASUREMENT_INCAPABLE\n");
            rrm_info->measure_result = MEASUREMENT_INCAPABLE;
        }

        if (pframe[len + 3] & BIT2) {
            TD_EM_DBG_TRACE(" Receive beacon_report:MEASUREMENT_REFUSED\n");
            rrm_info->measure_result = MEASUREMENT_REFUSED;
        }
        
        rrm_info->beacon_report_len[rrm_info->beacon_report_num] = element_len;
        rrm_info->beacon_measurement_token[rrm_info->beacon_report_num] = pframe[len+2];
        rrm_info->beacon_report_mode[rrm_info->beacon_report_num] = pframe[len+3];
        rrm_info->beacon_report_num = 1;
    }
    
    return len + element_len + 2;
}

/*****************************************************************************
 函 数 名  : td_parse_rm_report
 功能描述  : 解析Radio Measurement Report
 输入参数  : dot11k_rrm_info_t *rrm_info
             unsigned char *pframe  
             int frame_len
 输出参数  : 无
 返 回 值  : 无
 
 修改历史      :
  1.日    期   : 2020年8月10日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static void td_parse_rm_report(td_dot11k_rrm_info_t *rrm_info, unsigned char *pframe, int frame_len)
{
    int len;
    unsigned char element_id = 0;
    unsigned char element_len = 0;
    if (!rrm_info || !pframe) {
        TD_EM_DBG_PARAM_ERR("kmalloc failed!\n");
        return;
    }
    //len从3开始，是802.11 wireless management 中的catagery code/action/code/dialog token
    len = 3;
    while(len <= frame_len)
    {    
        //第4个字节
        element_id = pframe[len];
        //第5个字节
        element_len = pframe[len + 1];
        /*parsing every radio measurment report element*/
        if(DOT11_MNG_MEASURE_REPORT_ID == element_id) {//DOT11_MNG_MEASURE_REPORT_ID=39
            if(DOT11_MEASURE_TYPE_BEACON == pframe[len + 4]) {//DOT11_MEASURE_TYPE_BEACON==5,第8个字节
                len = td_rm_parse_beacon_report(rrm_info, pframe, len, frame_len);
                continue;
            }
        } 

        len += 2 + element_len;
    }

    return;
}

/*****************************************************************************
 函 数 名  : td_em_send_btm_req
 功能描述  : 判断STA是否支持BTM
 输入参数  : em_osif osif          
             int channel
             int opclass
             unsigned char *target_bss
             unsigned char *sta  
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年8月12日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_send_btm_req(em_osif osif, int channel, int opclass, unsigned char *target_bss,unsigned char *sta)
{
    char *ioctl_buf = NULL;
    wl_af_params_t *af_params = NULL;
    wl_action_frame_t *action_frame = NULL;
    dot11_bsstrans_req_t *transreq = NULL;
    dot11_neighbor_rep_ie_t *nbr_ie = NULL;
    dot11_wide_bw_chan_ie_t *wbc_ie = NULL;
    dot11_ngbr_bsstrans_pref_se_t *nbr_pref = NULL;
    wlc_info_t *wlc = NULL;
    char *pcur = NULL;
    int ioctl_len = 0;
    static int bss_token = 0;

    if (!osif || !target_bss || !sta) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("wlc is null ptr!\n");
        return -1;
    }
    
    ioctl_buf = (char *)MALLOC(wlc->osh, WLC_IOCTL_MAXLEN);
    if(!ioctl_buf) {
        TD_EM_DBG_PARAM_ERR("malloc fail");
        return -1;
    }
    
    TD_EM_DBG_TRACE("td_em_send_btm_req channel %d \n", channel);
    TD_EM_DBG_TRACE("td_em_send_btm_req op_class %d \n", opclass);  

    //封装Action code: BSS Transition Management Request帧数据结构
    memset(ioctl_buf, 0, WLC_IOCTL_MAXLEN);
    strncpy(ioctl_buf, "actframe", WLC_IOCTL_MAXLEN);
    ioctl_len = strlen(ioctl_buf) + 1;
    pcur = ioctl_buf + ioctl_len;

    af_params = (wl_af_params_t *)pcur;
    af_params->channel = 0;
    af_params->dwell_time = -1;
    action_frame = &af_params->action_frame;
   
    memcpy(&action_frame->da, sta, ETHER_ADDR_LEN);
    action_frame->packetId = (uint32)(uintptr)action_frame;

    //计算整个帧的长度
    action_frame->len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
        DOT11_NGBR_BSSTRANS_PREF_SE_IE_LEN +
        TLV_HDR_LEN + DOT11_BSSTRANS_REQ_LEN +
        TLV_HDR_LEN + DOT11_WIDE_BW_IE_LEN;

    ioctl_len += action_frame->len;

    transreq = (dot11_bsstrans_req_t *)&action_frame->data[0];
    transreq->category = DOT11_ACTION_CAT_WNM;
    transreq->action = DOT11_WNM_ACTION_BSSTRANS_REQ;

    if (++bss_token == 0) {
        bss_token = 1;
    }

    transreq->token = bss_token;
    transreq->reqmode = DOT11_BSSTRANS_REQMODE_PREF_LIST_INCL 
                        | DOT11_BSSTRANS_REQMODE_ABRIDGED
                        | DOT11_BSSTRANS_REQMODE_DISASSOC_IMMINENT;
    transreq->disassoc_tmr = 0x0000;
    transreq->validity_intrvl = 0xFF;

    //封装Neighbor Report数据字段
    nbr_ie = (dot11_neighbor_rep_ie_t *)&transreq->data[0];
    nbr_ie->id = DOT11_MNG_NEIGHBOR_REP_ID;
    nbr_ie->len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
        DOT11_NGBR_BSSTRANS_PREF_SE_IE_LEN +
        DOT11_WIDE_BW_IE_LEN + TLV_HDR_LEN;
    
    memcpy(&nbr_ie->bssid, target_bss, ETHER_ADDR_LEN);
    nbr_ie->bssid_info = 0x00000000;
    nbr_ie->reg = opclass;
    nbr_ie->channel = channel;
    nbr_ie->phytype = 0;

    wbc_ie = (dot11_wide_bw_chan_ie_t *)&nbr_ie->data[0];
    wbc_ie->id = DOT11_NGBR_WIDE_BW_CHAN_SE_ID;
    wbc_ie->len = DOT11_WIDE_BW_IE_LEN;
    wbc_ie->channel_width = 0;
    wbc_ie->center_frequency_segment_0 = channel;
    wbc_ie->center_frequency_segment_1 = 0;

    pcur = (char *)wbc_ie + TLV_HDR_LEN + DOT11_WIDE_BW_IE_LEN;
    nbr_pref = (dot11_ngbr_bsstrans_pref_se_t *)pcur;
    nbr_pref->sub_id = DOT11_NGBR_BSSTRANS_PREF_SE_ID;
    nbr_pref->len = DOT11_NGBR_BSSTRANS_PREF_SE_LEN;
    nbr_pref->preference = DOT11_NGBR_BSSTRANS_PREF_SE_HIGHEST;
    ioctl_len += DOT11_NGBR_BSSTRANS_PREF_SE_IE_LEN;

    //通过Ioctl发送BTM Req请求Action 帧
    if (BCME_OK != wlc_ioctl(wlc, WLC_SET_VAR, (void *)ioctl_buf, WL_WIFI_AF_PARAMS_SIZE, osif->wlcif)) {
        TD_EM_DBG_PARAM_ERR("actframe fail");
        MFREE(wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);
        return -1;
    }

    MFREE(wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_is_support_btm
 功能描述  : 判断STA是否支持BTM
 输入参数  : em_osif osif          
             unsigned char *mac  
 输出参数  : 无
 返 回 值  : True 支持，FALSE不支持失 
 
 修改历史      :
  1.日    期   : 2020年8月12日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
bool td_em_is_support_btm(em_osif osif, unsigned char *sta_mac)
{
    struct ether_addr addr;
    struct scb *scb = NULL;
    uint32 cap = 0;
    
    if (!osif || !sta_mac) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return FALSE;
    }

    memcpy(&addr, sta_mac, sizeof(addr));
    scb = wlc_scbfind(osif->wlc, osif, &addr);
    
    if (scb) {
        cap = wlc_wnm_get_scbcap(osif->wlc, scb);
    }
    
    return cap? TRUE: FALSE;
}

/*****************************************************************************
 函 数 名  : td_em_client_assoc_sta_control
 功能描述  : 处理关联STA切换请求
 输入参数  : em_osif osif          
             unsigned char *buf  
 输出参数  : 无
 返 回 值  : 0成功，-1失败 
 
 修改历史      :
  1.日    期   : 2020年5月22日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_client_assoc_sta_control(em_osif osif, unsigned char *buf)
{
    client_assoc_req_t client_assoc_sta;
    struct ether_addr addr;
    int i = 0;
    int ret = 0;
   
    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    memset(&client_assoc_sta, 0, sizeof(client_assoc_req_t));
    memcpy(&client_assoc_sta, buf, sizeof(client_assoc_req_t));
    TD_EM_DBG_TRACE("assoc_control : %d\n", client_assoc_sta.assoc_control);
    TD_EM_DBG_TRACE("sta_num : %d\n", client_assoc_sta.sta_num);
    
    //第二阶段只做协议交互，上层没有做具体逻辑处理?，无线底层暂只做简单处理。
    for (i = 0; i < client_assoc_sta.sta_num && TD_EM_ASSOC_BLOCK == client_assoc_sta.assoc_control; i++)
    {
        TD_EM_DBG_TRACE("STA MAC %02X-%02X-%02X-%02X-%02X-%02X \n",
                client_assoc_sta.sta_info[i].sta_mac[0], client_assoc_sta.sta_info[i].sta_mac[1],
                client_assoc_sta.sta_info[i].sta_mac[2], client_assoc_sta.sta_info[i].sta_mac[3],
                client_assoc_sta.sta_info[i].sta_mac[4], client_assoc_sta.sta_info[i].sta_mac[5]);
        memcpy(&addr, client_assoc_sta.sta_info[i].sta_mac, TD_EM_MACADDRLEN);

        ret = wlc_ioctl(osif->wlc, WLC_SCB_DEAUTHENTICATE, &addr, TD_EM_MACADDRLEN, osif->wlcif);
        if (BCME_OK != ret) {
            TD_EM_DBG_TRACE("wlc_ioctl ret %d \n", ret);
            return -1;
        }
    }
    
    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_priv_by_mac
 功能描述  : 通过MAC地址找到对应的priv
 输入参数  : unsigned char *mac  
 输出参数  : 无
 返 回 值  : 对应的bsscfg
 
 修改历史      :
  1.日    期   : 2020年5月20日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
wlc_bsscfg_t *td_em_get_priv_by_mac(wlc_info_t *wlc, unsigned char *mac)
{
    struct ether_addr addr;
    wlc_bsscfg_t *bsscfg = NULL;
    
    if (!wlc || !mac) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return NULL;
    }

    memcpy(&addr, mac, sizeof(addr));
    bsscfg = wlc_bsscfg_find_by_hwaddr(wlc, &addr);
    
    return bsscfg;
}

/*****************************************************************************
 函 数 名  : td_em_get_bh_steer_results
 功能描述  : 获取回传链路优化状态
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 0成功，-1失败 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_bh_steer_results(em_osif osif, backhaul_steering_response_t *bh_steer_rsp)
{
    if (!osif || !bh_steer_rsp) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
   
    osif->multiap_bssSteer_scan = FALSE;
    osif->multiap_bssSteer_channel = 0;
    
    bh_steer_rsp->type.event = TD_EASYMESH_BACKHAUL_STEER_RESPONSE;
    memcpy(bh_steer_rsp->sta_mac, &(osif->multiap_sta_mac), TD_EM_MACADDRLEN);
    memcpy(bh_steer_rsp->bssid, &(osif->BSSID), TD_EM_MACADDRLEN);
    memcpy(bh_steer_rsp->target_bssid, &(osif->multiap_target_bssid), TD_EM_MACADDRLEN);
    bh_steer_rsp->result_code = g_td_em_fake_data.bh_steer_result_code;//暂时写假数据， 并且该数值可通过串口配置
    bh_steer_rsp->error_code = g_td_em_fake_data.bh_steer_error_code;//暂时写假数据， 并且该数值可通过串口配置
    TD_EM_DBG_TRACE("result_code %d!\n", bh_steer_rsp->result_code);
    TD_EM_DBG_TRACE("Respone: backhaul_sta_mac  %02X%02X%02X%02X%02X%02X \n",
        bh_steer_rsp->sta_mac[0], bh_steer_rsp->sta_mac[1],bh_steer_rsp->sta_mac[2],bh_steer_rsp->sta_mac[3],bh_steer_rsp->sta_mac[4],bh_steer_rsp->sta_mac[5]);
    TD_EM_DBG_TRACE("Respone: bssid  %02X%02X%02X%02X%02X%02X \n", 
        bh_steer_rsp->bssid[0], bh_steer_rsp->bssid[1],bh_steer_rsp->bssid[2],bh_steer_rsp->bssid[3],bh_steer_rsp->bssid[4],bh_steer_rsp->bssid[5]);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_do_backhaul_Steer
 功能描述  : 执行回传链路优化相关动作
 输入参数  : em_osif osif          
             unsigned char *buf     
 输出参数  : 无
 返 回 值  :0成功，-1失败 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_do_backhaul_steer(em_osif osif, unsigned char *buf)
{
    unsigned char op_class, channel;
    backhaul_steering_request_t *bh_steer = NULL;

    bh_steer = (backhaul_steering_request_t *)buf;
    
    if (!osif || !bh_steer) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
  
    TD_EM_DBG_TRACE(" Request: op class %d \n", bh_steer->op_class);
    TD_EM_DBG_TRACE(" Request: channel %d \n", bh_steer->channel);
    TD_EM_DBG_TRACE(" Request: backhaul_sta_mac  %02X%02X%02X%02X%02X%02X \n", bh_steer->backhaul_sta_mac[0], bh_steer->backhaul_sta_mac[1],bh_steer->backhaul_sta_mac[2],bh_steer->backhaul_sta_mac[3],bh_steer->backhaul_sta_mac[4],bh_steer->backhaul_sta_mac[5]);
    TD_EM_DBG_TRACE(" Request: target_bssid  %02X%02X%02X%02X%02X%02X \n", bh_steer->target_bssid[0], bh_steer->target_bssid[1],bh_steer->target_bssid[2],bh_steer->target_bssid[3],bh_steer->target_bssid[4],bh_steer->target_bssid[5]);

    op_class = bh_steer->op_class;
    channel = bh_steer->channel;
    
    osif->multiap_bssSteer_scan = TRUE;
    osif->multiap_bssSteer_channel = channel;
    //save the target bssid
    memcpy(&(osif->multiap_target_bssid), bh_steer->target_bssid , TD_EM_MACADDRLEN);
    memcpy(&(osif->multiap_sta_mac), bh_steer->backhaul_sta_mac, TD_EM_MACADDRLEN);
    return 0;    
}

/*****************************************************************************
 函 数 名  : td_em_set_steer_policy
 功能描述  : 配置漫游策略
 输入参数  : em_osif osif          
             unsigned char *buf  
 输出参数  : 无
 返 回 值  : 0成功，-1失败 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_set_steer_policy(em_osif osif, unsigned char *buf)
{
    int i = 0;
    steering_policy_t *steering_policy = NULL;
    wlc_bsscfg_t *target_bsscfg = NULL;

    if (!osif|| !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    
    steering_policy = (steering_policy_t *)buf;
    
    TD_EM_DBG_TRACE(" radio num : %d\n",steering_policy->radio_nr);
    for (i = 0; i < steering_policy->radio_nr; i++)
    {   
        TD_EM_DBG_TRACE(" find MAC %02X-%02X-%02X-%02X-%02X-%02X \n",
            steering_policy->radio_steer_policy_data[i].radio_mac[0], steering_policy->radio_steer_policy_data[i].radio_mac[1],
            steering_policy->radio_steer_policy_data[i].radio_mac[2], steering_policy->radio_steer_policy_data[i].radio_mac[3],
            steering_policy->radio_steer_policy_data[i].radio_mac[4], steering_policy->radio_steer_policy_data[i].radio_mac[5]);
            target_bsscfg = td_em_get_priv_by_mac(osif->wlc, steering_policy->radio_steer_policy_data[i].radio_mac);
        if (target_bsscfg) {
            TD_EM_DBG_TRACE("[%d]OK-MAC  %pM \n", i+1, &target_bsscfg->BSSID);
            target_bsscfg->multiap_steering_policy = steering_policy->radio_steer_policy_data[i].policy;
            target_bsscfg->multiap_cu_threshold = steering_policy->radio_steer_policy_data[i].cu_threshold;
            target_bsscfg->multiap_rcpi_threshold = steering_policy->radio_steer_policy_data[i].rcpi_threshold;
            
            TD_EM_DBG_TRACE("[%d]:steering_policy %d \n", i+1,target_bsscfg->multiap_steering_policy);
            TD_EM_DBG_TRACE("[%d]:cu_threshold %d \n", i+1,target_bsscfg->multiap_cu_threshold);
            TD_EM_DBG_TRACE("[%d]:rcpi_threshold %d \n", i+1,target_bsscfg->multiap_rcpi_threshold);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_opclass_by_chnl
 功能描述  : 通过channel获取对应的操作类
 输入参数  : em_osif osif   
             int channel  
 输出参数  : 无
 返 回 值  : -1失败，非-1为对应的操作类
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
#if 0
int td_em_get_opclass_by_chnl(em_osif osif, int channel)
{

    return 0;
}
#endif
/*****************************************************************************
 函 数 名  : td_em_get_max_power
 功能描述  : 获取当前web最大功率
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_max_power(em_osif osif)
{
    int pwr;

    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    wlc_iovar_getint(osif->wlc, "em_maxpower", &pwr);

    return pwr;
}

/*****************************************************************************
 函 数 名  : td_em_get_mac_addr
 功能描述  : 获取当前vap的mac地址
 输入参数  : em_osif osif               
             unsigned char* mac_addr  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_mac_addr(em_osif osif, unsigned char* mac_addr)
{
    if (!osif || !mac_addr) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }
    
    memcpy(mac_addr, &(osif->cur_etheraddr), sizeof(osif->cur_etheraddr));

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_channel_width
 功能描述  : 获取频宽
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
td_em_bw_e td_em_get_channel_width(em_osif osif)
{
    uint16 bw_flag = 0;
    td_em_bw_e bw_mhz = 0;

    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }

    bw_flag = CHSPEC_BW(osif->wlc->default_bss->chanspec);

    if (bw_flag == WL_CHANSPEC_BW_160) {
        bw_mhz = TD_EM_CHWIDTH160;
    } else if (bw_flag == WL_CHANSPEC_BW_80) {
        bw_mhz = TD_EM_CHWIDTH80;
    } else if (bw_flag == WL_CHANSPEC_BW_40) {
        bw_mhz = TD_EM_CHWIDTH40;
    } else if (bw_flag == WL_CHANSPEC_BW_20) {
        bw_mhz = TD_EM_CHWIDTH20;
    } else if (bw_flag == WL_CHANSPEC_BW_8080) {
        bw_mhz = TD_EM_CHWIDTH80_80;
    } else {
        bw_mhz = TD_EM_CHWIDTH_ERROR;
    }
    
    return bw_mhz;
}

/*****************************************************************************
 函 数 名  : td_em_get_country_map
 功能描述  : 获取国家码
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
tm_em_country_map_e td_em_get_country_map(em_osif osif)
{
    wlc_info_t  *wlc = NULL;
    
    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }

    wlc = osif->wlc;

    if (wlc_us_code(wlc)) {
        return TD_EM_FCC;
    } else if (wlc_eu_code(wlc)) {
        return TD_EM_ETSI;
    } else if (wlc_japan(wlc)) {
        return TD_EM_MKK;
    } else {
        return TD_EM_GLOBAl;
    }
}

/*****************************************************************************
 函 数 名  : td_em_get_max_min_channel
 功能描述  : 获取当前最大，最小的信道编号
 输入参数  : em_osif osif           
             unsigned int *maxch  
             unsigned int *minch  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_max_min_channel(em_osif osif, unsigned int *maxch, unsigned int *minch)
{
    wl_channels_in_country_t *cic = NULL;
    const char *country = NULL;
    char buf[WLC_IOCTL_SMLEN] = {0};
    int ret = 0, len = 0, k = 0;

    if (!osif || !maxch || !minch)
    {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }

    cic = (wl_channels_in_country_t *)buf;
    cic->buflen = WLC_IOCTL_SMLEN;
    cic->count = 0;

    country = wlc_channel_country_abbrev(osif->wlc->cmi);
    len = strlen(country);
    if ((len > 3) || (len < 2)) {
        return -1;
    }

    strncpy(cic->country_abbrev, country, sizeof(cic->country_abbrev));
    
    cic->band = osif->wlc->band->bandtype;
    TD_EM_DBG_TRACE(" band %d - \n", cic->band);
    
    ret = wlc_get_channels_in_country(osif->wlc, cic);
    TD_EM_DBG_TRACE(" wlc_get_channels_in_country -ret %d - \n", ret);

    for (k = 0, *maxch = *minch = cic->channel[k]; k < cic->count; k++) {
        if (cic->channel[k] > *maxch) {
            *maxch = cic->channel[k];
        }
        if (cic->channel[k] < *minch) {
            *minch = cic->channel[k];
        }
    }

    TD_EM_DBG_TRACE("chan range %d - %d \n", *maxch , *minch);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_sta_traffic
 功能描述  : 获取客户端流量信息
 输入参数  : em_osif osif                                        
             td_em_associated_sta_traffic_tlv_t *traffic_info  
             u1Byte *sta_mac                                   
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_sta_traffic(em_osif osif, td_em_associated_sta_traffic_tlv_t *traffic_info, unsigned char *sta_mac)
{
    struct scb * scb= NULL;
    struct ether_addr mac = {0};
    wlc_scb_stats_t *scb_stats = NULL;

    if (!osif || !sta_mac) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return false;
    }

    memcpy(mac.octet, sta_mac, TD_EM_MACADDRLEN);
    scb = wlc_scbfind(osif->wlc, osif, (const struct ether_addr *)&mac);
    if (!scb) {
        TD_EM_DBG_PARAM_ERR("sta is unassoc!\n");
        return -1;
    }
    
    scb_stats = (wlc_scb_stats_t *)&scb->scb_stats;
    
    traffic_info->rx_bytes = scb_stats->rx_ucast_bytes + scb_stats->rx_mcast_bytes;
    traffic_info->rx_fail = scb_stats->rx_decrypt_failures;
    traffic_info->rx_packet =scb_stats->rx_mcast_pkts + scb_stats->rx_ucast_pkts; 
    traffic_info->tx_bytes = scb_stats->tx_mcast_bytes + scb_stats->tx_ucast_bytes;
    traffic_info->tx_fail = scb_stats->tx_failures;
    traffic_info->tx_packet = scb_stats->tx_pkts_total;
    traffic_info->retranscount = scb_stats->tx_pkts_retried;
    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_sta_isassoc
 功能描述  : 通过mac地址获取客户端连接状态
 输入参数  : em_osif osif              
             unsigned char *sta_mac  
 输出参数  : 无
 返 回 值  : 0:未关联 1:已关联
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/

bool td_em_sta_isassoc(em_osif osif, unsigned char *sta_mac)
{
    struct ether_addr mac = {0};

    if (!osif || !sta_mac) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return false;
    }

    memcpy(mac.octet, sta_mac, TD_EM_MACADDRLEN);
    return !!wlc_scbfind(osif->wlc, osif, (const struct ether_addr *)&mac);//将指针转为bool类型
}

/*****************************************************************************
 函 数 名  : td_em_get_sta_link_time
 功能描述  : 遍历sta回调函数
 输入参数  : em_osif sta

 输出参数  : void arg
 返 回 值  : 写入的字节数，方便做偏移
 
 修改历史      :
  1.日    期   : 2020年6月1日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_sta_link_time(void *sta, void *arg)
{
    struct scb *psta = (struct scb *)sta;
    association_client_sta_info_t *sta_info = (association_client_sta_info_t *)arg;

    if (!sta || !arg) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }

    sta_info->link_time = (unsigned short)(psta->bsscfg->wlc->pub->now - psta->assoctime);
    memcpy(sta_info->sta_macaddr, psta->ea.octet, sizeof(sta_info->sta_macaddr));
    return sizeof(association_client_sta_info_t);

}

/*****************************************************************************
 函 数 名  : td_em_foreach_sta_list
 功能描述  : 遍历sta列表
 输入参数  : em_osif osif                               
           int  sta_num 上层要获取的sta数量 (该值大于0且小于关联的sta的数目,否则拷贝所有sta信息)
           unsigned char * data (需要写入的参数)
           int (*cb)(em_osif osif, void *sta, void *arg) (回调函数)(返回值为写入的字节数小于0则错误,
           如果返回值不正确会导致地址偏移错误。sta为客户端信息。arg写入的数据)
           ps:data和cb指针可以传空
 输出参数  : data 数据 
 返 回 值 : sta个数
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_iterate_sta_list(em_osif osif, int sta_num, unsigned char * data, int (*cb)(void *sta, void *arg))
{
    struct scb_iter scbiter = {0};
    struct scb *scb = NULL;
    int offset = 0, num = 0;
    if (!osif) {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }

    FOREACH_BSS_SCB(osif->wlc->scbstate, &scbiter, osif, scb) {
        if ((sta_num > 0) && (num > sta_num)) {
            break;
        }
        if (SCB_ASSOCIATED(scb)) {
            if ((cb && data) && ((offset = (*cb)((void *)scb, (void *)data)) >= 0)) {
                data += offset;
            } else if (offset < 0) {
                break;
            } else {
            }
            num++;
            offset = 0;
        }
    }

    return num;
}

/*****************************************************************************
 函 数 名  : td_em_check_drv_state
 功能描述  : 检测当前状态
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
bool td_em_check_drv_state(em_osif osif)
{
    return false;
}

/*****************************************************************************
 函 数 名  : td_em_get_sta_num
 功能描述  : 获取客户端数量
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_sta_num(em_osif osif)
{
    return td_em_iterate_sta_list(osif, 0, NULL, NULL);
}

/*****************************************************************************
 函 数 名  : td_em_get_interface_mode
 功能描述  : 获取接口工作模式
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
td_em_workmode_e td_em_get_interface_mode(em_osif osif)
{
   if (!osif) {
       TD_EM_DBG_PARAM_ERR(" null pointer!\n");
       return -1;
   }
   if (BSSCFG_AP(osif)) {
        return TD_EM_BSS_WORKMODE_AP;
   } else if (BSSCFG_STA(osif)) {
        return TD_EM_BSS_WORKMODE_STATION;
   } else {
        return TD_EM_BSS_WORKMODE_DEFAULT;
   }
}

/*****************************************************************************
 函 数 名  : td_em_get_centerfreq
 功能描述  : 获取中心频点(由于协议里面没有规定2.4g中心频点计算方法,
           所以2.4g中心频点默认为0)
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : unsigned
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
unsigned int td_em_get_centerfreq(em_osif osif)
{
    chanspec_t  chanspec;

    if (!osif) {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }

    
    if (osif->associated) {
        chanspec = osif->current_bss->chanspec;
    } else {
        chanspec = osif->wlc->default_bss->chanspec;
    }

    if ((chanspec & WL_CHANSPEC_BAND_MASK) == WL_CHANSPEC_BAND_2G) {
        return 0;
    } else {
        return chanspec & INVCHANSPEC;
    }
}

/*****************************************************************************
 函 数 名  : td_em_get_media_type
 功能描述  : 获取芯片工作模式(由于multi ap中该字段不是位操作,
             所以根据协议版本由高至低获取)
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : unsigned
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
td_em_device_type_e td_em_get_media_type(em_osif osif)
{
    int band, vhtmode, nmode, mode_reqd, gmode;
    wlc_info_t *wlc = NULL;

    if (!osif) {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }

    wlc = osif->wlc;
    if (osif->associated) {
        band = (int32)(osif->current_bss->chanspec & WL_CHANSPEC_BAND_MASK);
    } else {
        band = (int32)(wlc->default_bss->chanspec & WL_CHANSPEC_BAND_MASK);
    }

    wlc_iovar_getint(wlc, "nmode", &nmode);
    wlc_iovar_getint(wlc, "vhtmode", &vhtmode);
    wlc_iovar_getint(wlc, "mode_reqd", &mode_reqd);
    wlc_iovar_getint(wlc, "mode_reqd", &gmode);

    if (vhtmode && (WL_CHANSPEC_BAND_5G == band)) {
        return TD_EM_DEVICE_TYPE_5G_11AC;
    } else if (nmode && (WL_CHANSPEC_BAND_5G == band)) {
        return TD_EM_DEVICE_TYPE_5G_11N;
    } else if (nmode && (WL_CHANSPEC_BAND_2G == band)) {
        return TD_EM_DEVICE_TYPE_2G_11N;
    } else if ((!nmode && !vhtmode && !mode_reqd) && (WL_CHANSPEC_BAND_5G == band)) {
        return TD_EM_DEVICE_TYPE_5G_11A;
    } else if ((!nmode && !vhtmode && !mode_reqd && gmode)  && (WL_CHANSPEC_BAND_2G == band)) {
        return TD_EM_DEVICE_TYPE_2G_11G;
    } else if ((!nmode && !vhtmode && !mode_reqd && !gmode) && (WL_CHANSPEC_BAND_2G == band)) {
        return TD_EM_DEVICE_TYPE_2G_11B;
    } else {
        return TD_EM_DEVICE_TYPE_DEFAULT;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_ssid
 功能描述  : 获取SSID
 输入参数  : em_osif osif

 输出参数  : unsigned char* ssid
 返 回 值  : ssid len
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_ssid(em_osif osif, unsigned char* ssid)
{
    if (!osif || !ssid) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }
    memcpy(ssid, osif->SSID , osif->SSID_len);
    return osif->SSID_len;
}

/*****************************************************************************
 函 数 名  : td_em_get_auth_type
 功能描述  : 获取认证类型(BCM方案在上层实现)
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
td_em_authtype_e td_em_get_auth_type(em_osif osif)
{
    return 0;
}


/*****************************************************************************
 函 数 名  : td_em_get_encryption_type
 功能描述  : 获取加密类型(BCM方案在上层实现)
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
td_em_encryption_type_e td_em_get_encryption_type(em_osif osif)
{
    return 0;
}


/*****************************************************************************
 函 数 名  : td_em_get_netkey
 功能描述  : 获取当前秘钥(BCM方案在上层实现)
 输入参数  : em_osif osif  

 输出参数  : char *key   
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月20日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
void td_em_get_netkey(em_osif osif, unsigned char *key)
{
    return;
}

int td_em_get_sae_cap(em_osif osif)
{
#if defined(WL_SAE)
    return 1;
#else 
    return 0;
#endif
}

/*****************************************************************************
 函 数 名  : td_em_get_bss_type
 功能描述  : 获取当前接口角色（backhual bss 、fronthual bss
                 ..）
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
td_em_bss_type_e td_em_get_bss_type(em_osif osif)
{
    td_em_bss_type_e type = TD_EM_BSS_NONE;
    uint8 map_attr = 0;

    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }

    map_attr = osif->map_attr;
    
    if (map_attr & MAP_EXT_ATTR_FRNTHAUL_BSS) {
        type = TD_EM_FRONTHAUL_BSS;
    } else if (map_attr & MAP_EXT_ATTR_BACKHAUL_BSS) {
        type = TD_EM_BACKHAUL_BSS;
    } else if (map_attr & MAP_EXT_ATTR_BACKHAUL_STA) {
        type = TD_EM_BACKHAUL_STA;
    }else if (map_attr & MAP_EXT_ATTR_TEAR_DOWN){
        type = TD_EM_TEAR_DOWN;
    } else {
    }

    return type;
}

/*****************************************************************************
 函 数 名  : td_em_get_apcapability
 功能描述      : 查询ap具备哪些能力
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月19日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_apcapability(em_osif osif, unsigned char *send_buf)
{
    unsigned int         len = 0;

    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    memset(send_buf, 0, 1);
    //support unassociated STA Link Metric on channel BSS operating on
    send_buf[0] |= BIT(7);

    //support unassociated STA Link Metric report on channel BSS not operating on
    send_buf[0] |= BIT(6);

    //support agent-initated RSSI based steering
    //send_buf[0] |= BIT(5);

    len += 1;
    TD_EM_DBG_MSG(" send_buf[0] = %02x\n",send_buf[0]);

    return len;

}

/*****************************************************************************
 函 数 名  : td_em_get_apcapability_2
 功能描述      : 协议v2对ap具备能力的补充
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月19日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_apcapability_2(em_osif osif, unsigned char *send_buf)
{
    unsigned int        len = 0;

    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    send_buf[0]  = 0;
    len +=1;
    TD_EM_DBG_MSG(" send_buf[0] = %02x\n",send_buf[0]);

    return len;

}

/*****************************************************************************
 函 数 名  : td_wl_wlif_get_max_nss
 功能描述      : 获取当前设备能支持的最大nss
 输入参数      : wlc_bss_info_t *bi
 输出参数      : nss :txrx天线数量
 返 回 值  : 其他：txrx天线数量
 日    期    : 2020年7月31日
 作    者    : 尹家政
*****************************************************************************/
int td_wl_wlif_get_max_nss(wlc_bss_info_t *bi)
{
    int i = 0, mcs_idx = 0;
    int mcs = 0, isht = 0;
    int nss = 0;

    if (bi->flags2 & WLC_BSS2_VHT) {
        uint mcs_cap = 0;

        for (i = 1; i <= TD_VHT_CAP_MCS_MAP_NSS_MAX; i++) {
            mcs_cap = TD_VHT_MCS_MAP_GET_MCS_PER_SS(i,
                    bi->vht_txmcsmap);
            if (mcs_cap != TD_VHT_CAP_MCS_MAP_NONE) {
                nss++; /* Calculate the number of streams */
            }
        }

        if (nss) {
            return nss;
        }
    }

    /* For 802.11n networks, use MCS table */
    for (mcs_idx = 0; mcs_idx < (TD_MCSSET_LEN * 8); mcs_idx++) {
        if (td_isset(bi->rateset.mcs, mcs_idx) && mcs_idx < TD_MCS_TABLE_SIZE) {
            mcs = mcs_idx;
            isht = 1;
        }
    }

    if (isht) {
        int nss = 0;

        if (mcs > 32) {
            printf("MCS is Out of range \n");
        } else if (mcs == 32) {
            nss = 1;
        } else {
            nss = 1 + (mcs / 8);
        }

        return nss;
    }

    return nss;
}

/*****************************************************************************
 函 数 名  : td_em_get_htcapability
 功能描述      : 查询ap是否具备高吞吐能力
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月19日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_htcapability(em_osif osif, unsigned char *send_buf)
{
    unsigned int   flags           = 0;
    unsigned int   mimo_mode       = 0;
    unsigned int   len             = 0;
    easymesh_ht_ability_tlv_t *val = (easymesh_ht_ability_tlv_t*)send_buf;
    wlc_info_t   *wlc              = NULL;
    len = sizeof(easymesh_ht_ability_tlv_t);

    if (!osif || !val) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if(!(osif->current_bss->flags & WLC_BSS_HT)){
        val->capability_flag = UNSUP_ABILITY;
        len = 1;  //capability flag占一个字节
        return len;
    }

    val->capability_flag = SUP_ABILITY;
    mimo_mode = td_wl_wlif_get_max_nss(osif->current_bss);

    //spatial stream support
    switch (mimo_mode) {
        case TD_NSS_2:
            val->data |= TD_AP_HTCAP_RX_NSS_2;
            val->data |= TD_AP_HTCAP_TX_NSS_2;
        break;

        case TD_NSS_3:
            val->data |= TD_AP_HTCAP_RX_NSS_3;
            val->data |= TD_AP_HTCAP_TX_NSS_3;
        break;

        case TD_NSS_4:
            val->data |= TD_AP_HTCAP_RX_NSS_4;
            val->data |= TD_AP_HTCAP_TX_NSS_4;
        break;

        default:
        break;
    }

    flags = wlc_ht_get_cap(wlc->hti);
    //HT support for 40MHz
    if (flags & HT_CAP_40MHZ) {
        val->data |= BIT(1);
    }
    //Short GI Support for 40MHz
    if (flags & HT_CAP_SHORT_GI_40) {
        val->data |= BIT(2);
    }
    //Short GI Support for 20MHz
    if (flags & HT_CAP_SHORT_GI_20) {
        val->data |= BIT(3);
    }

    memcpy(val->bssid, osif->cur_etheraddr.octet, TD_EM_MACADDRLEN);

    TD_EM_DBG_MSG(" val->data = %02x\n",val->data);
    return len;

}

/*****************************************************************************
 函 数 名  : td_em_get_vhtapCapability
 功能描述      : 查询ap是否具备超高吞吐能力
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月19日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_vhtapCapability(em_osif osif, unsigned char *send_buf)
{
    unsigned int   flags            = 0;
    unsigned int   mimo_mode        = 0;
    unsigned int   len              = 0;
    easymesh_vht_ability_tlv_t *val = (easymesh_vht_ability_tlv_t*)send_buf;
    wlc_info_t    *wlc              = NULL;;

    if (!osif || !val) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if(!(osif->current_bss->flags2 & WLC_BSS2_VHT)){
        val->capability_flag = UNSUP_ABILITY;
        len = 1;            //capability flag占一个字节
        return len;
    }

    val->capability_flag = SUP_ABILITY;

    mimo_mode = td_wl_wlif_get_max_nss(osif->current_bss);

    //spatial stream support
    switch (mimo_mode) {
        case TD_NSS_2:
            val->spatial_streams |= TD_AP_VHTCAP_RX_NSS_2;
            val->spatial_streams |= TD_AP_VHTCAP_TX_NSS_2;
        break;

        case TD_NSS_3:
            val->spatial_streams |= TD_AP_VHTCAP_RX_NSS_3;
            val->spatial_streams |= TD_AP_VHTCAP_TX_NSS_3;
        break;

        case TD_NSS_4:
            val->spatial_streams |= TD_AP_VHTCAP_RX_NSS_4;
            val->spatial_streams |= TD_AP_VHTCAP_TX_NSS_4;
        break;

        case TD_NSS_8:
            val->spatial_streams |= TD_AP_VHTCAP_RX_NSS_8;
            val->spatial_streams |= TD_AP_VHTCAP_TX_NSS_8;
        break;

        default:
        break;
    }

    flags = wlc_vht_get_cap_info(wlc->vhti);
    //Short GI Support for 80MHz
    if (flags & TD_VHT_CAP_INFO_SGI_80MHZ) {
        val->spatial_streams |= BIT(1);
    }
    //Short GI Support for 160MHz and 80+80 MHz
    if ((flags & TD_VHT_CAP_INFO_SGI_160MHZ)) {
        val->spatial_streams |= BIT(0);
    }
    //VHT Support for 160 MHz
    if (flags & TD_VHT_CAP_CHAN_WIDTH_SUPPORT_160) {
        val->vht_capable |= BIT(6);
    }
    //VHT Support for 80+80 MHz
    if (flags & TD_VHT_CAP_CHAN_WIDTH_SUPPORT_160_8080) {
        val->vht_capable |= BIT(7);
    }

    //SU Beamformer Capable
    if (flags & TD_VHT_CAP_INFO_SU_BEAMFMR) {
        val->vht_capable |= BIT(5);
    }
    //MU Beamformer Capable
    if (flags & TD_VHT_CAP_INFO_MU_BEAMFMR) {
        val->vht_capable |= BIT(4);
    }

    memcpy(val->bssid, osif->cur_etheraddr.octet, TD_EM_MACADDRLEN);

    //vht tx MCS
    val->tx_mcs = osif->current_bss->vht_txmcsmap;

    //vht rx MCS
    val->rx_mcs = osif->current_bss->vht_rxmcsmap;

    len = sizeof(easymesh_vht_ability_tlv_t);

    TD_EM_DBG_MSG(" val->vht_capable = %02x\n",val->vht_capable);
    TD_EM_DBG_MSG(" val->spatial_streams = %02x\n",val->spatial_streams);
    return len;

}

/*****************************************************************************
 函 数 名  : td_em_get_heapcapability
 功能描述      : 查询ap是否具备极高吞吐能力
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月19日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_heapcapability(em_osif osif, unsigned char *send_buf)
{
    int            bfr_cap         = 0;
    unsigned int   len             = 0;
    unsigned int   mimo_mode       = 0;
    unsigned char *mcs_len         = send_buf;
    wlc_info_t    *wlc             = NULL;

    easymesh_he_ability_tlv_t* val = NULL;

    if (!osif || !mcs_len) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    
    val = (easymesh_he_ability_tlv_t*)kmalloc(sizeof(easymesh_he_ability_tlv_t),GFP_ATOMIC);
    if (NULL == val) {
        TD_EM_DBG_PARAM_ERR("val kmalloc error!\n");
        return -1;
    }

    memset(val, 0, sizeof(easymesh_he_ability_tlv_t));
    if(!(osif->current_bss->flags3 & WLC_BSS3_HE)){
        val->capability_flag = UNSUP_ABILITY;
        len = 1;
        memcpy(mcs_len, val, sizeof(easymesh_he_ability_tlv_t));
        kfree(val);
        return len;
    }

    val->capability_flag  = SUP_ABILITY;
    mimo_mode = td_wl_wlif_get_max_nss(osif->current_bss);

    //spatial stream support
    switch (mimo_mode) {
        case TD_NSS_2:
            val->spatial_streams |= TD_AP_HECAP_RX_NSS_2;
            val->spatial_streams |= TD_AP_HECAP_TX_NSS_2;
        break;

        case TD_NSS_3:
            val->spatial_streams |= TD_AP_HECAP_RX_NSS_3;
            val->spatial_streams |= TD_AP_HECAP_TX_NSS_3;
        break;

        case TD_NSS_4:
            val->spatial_streams |= TD_AP_HECAP_RX_NSS_4;
            val->spatial_streams |= TD_AP_HECAP_TX_NSS_4;
        break;

        case TD_NSS_8:
            val->spatial_streams |= TD_AP_HECAP_RX_NSS_8;
            val->spatial_streams |= TD_AP_HECAP_TX_NSS_8;
        break;

        default:
        break;
    }

    //HE Support for 160 MHz
    if ((osif->current_bss->he_sup_bw160_tx_mcs != 0xffff) || (osif->current_bss->he_sup_bw160_rx_mcs != 0xffff)) {
        val->spatial_streams |= BIT(0);
    }
    //HE Support for 80+80 MHz
    if ((osif->current_bss->he_sup_bw80p80_tx_mcs != 0xffff) || (osif->current_bss->he_sup_bw80p80_rx_mcs != 0xffff)) {
        val->spatial_streams |= BIT(1);
    }

    bfr_cap = td_get_wlc_txbf(wlc->txbf);
    //SU beanformer capable
    if (bfr_cap & TD_TXBF_HE_SU_BFR_CAP) {
        val->he_capable |= BIT(7);
    }
    //MU beamformer capable
    if (bfr_cap & TD_TXBF_HE_MU_BFR_CAP) {
        val->he_capable |= BIT(6);
    }
    //UL MU-MIMO capable
    if (wlc->pub->mu_features & TD_MU_FEATURES_MURX) {
        val->he_capable |= BIT(5);
    }
    //UL MU-MIMO OFDMA capable
    if (wlc->pub->mu_features & TD_MU_FEATURES_MURX) {
        val->he_capable |= BIT(4);
    }
    //DL MU-MIMO OFDMA capable
    if (wlc->pub->mu_features & TD_MU_FEATURES_MUTX) {
        val->he_capable |= BIT(3);
    }
    //UL OFDMA capable
    val->he_capable |= BIT(2);
    //DL OFDMA capable
    val->he_capable |= BIT(1);

    memcpy(val->bssid, osif->cur_etheraddr.octet, TD_EM_MACADDRLEN);

    send_buf[0] = 0;
    mcs_len++;

    /* Add mcs map of 80MHZ */
    *((unsigned short *)mcs_len) = osif->current_bss->he_sup_bw80_tx_mcs;
    mcs_len += 2;
    *((unsigned short *)mcs_len) = osif->current_bss->he_sup_bw80_rx_mcs;
    mcs_len += 2;

    /* Add mcs map of 160 MHz */
    if ((osif->current_bss->he_sup_bw160_tx_mcs != 0xffff) || (osif->current_bss->he_sup_bw160_rx_mcs != 0xffff)) {
        *((unsigned short *)mcs_len) = osif->current_bss->he_sup_bw160_tx_mcs;
        mcs_len += 2;
        *((unsigned short *)mcs_len) = osif->current_bss->he_sup_bw160_rx_mcs;
        mcs_len += 2;
    }

    /* Add mcs map of 80p80 MHz */
    if ((osif->current_bss->he_sup_bw80p80_tx_mcs != 0xffff) || (osif->current_bss->he_sup_bw80p80_rx_mcs != 0xffff)) {
        *((unsigned short *)mcs_len) = osif->current_bss->he_sup_bw80p80_tx_mcs;
        mcs_len += 2;
        *((unsigned short *)mcs_len) = osif->current_bss->he_sup_bw80p80_rx_mcs;
        mcs_len += 2;
    }

    send_buf[0] = mcs_len - send_buf - 1;
    memcpy(mcs_len, val, sizeof(easymesh_he_ability_tlv_t));

    len = sizeof(easymesh_he_ability_tlv_t) + send_buf[0] + 1;  //k占一个字节+k字节内容+结构体内容

    TD_EM_DBG_MSG(" len = %d\n",len);
    TD_EM_DBG_MSG(" val->spatial_streams = %02x\n",val->spatial_streams);
    TD_EM_DBG_MSG(" val->he_capable = %02x\n",val->he_capable);

    kfree(val);
    return len;

}


/*****************************************************************************
 函 数 名  : td_em_backhaul_sta_radio_capabilities
 功能描述      : 无线sta radio能力查询
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月19日
 作    者    : 尹家政
*****************************************************************************/
int td_em_backhaul_sta_radio_capabilities(em_osif osif, unsigned char *send_buf)
{
    unsigned int   len             = 0;
    easymesh_bhstaradio_capability_tlv_t *val = (easymesh_bhstaradio_capability_tlv_t*)send_buf;
    //wlc_info_t   *wlc              = cfg->wlc;

    if (!osif || !val) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (!osif->_map) {
        TD_EM_DBG_PARAM_ERR("not found vxd!\n");
        return -1;
    }
    memcpy(val->radio_bssid, osif->current_bss->BSSID.octet, TD_EM_MACADDRLEN); //谁是backhual

    if (osif->map_attr & 0x80) {
        TD_EM_DBG_MSG(" osif->SSID= %s \n",osif->SSID);
        val->addr_include |= BIT(7);
        memcpy(val->bhsta_radio_addr, osif->cur_etheraddr.octet, TD_EM_MACADDRLEN);
    }

    len = sizeof(easymesh_bhstaradio_capability_tlv_t);
    TD_EM_DBG_MSG(" send_buf[0] = %02x\n",send_buf[0]);
    return len;

}


/*****************************************************************************
 函 数 名  : td_em_vendor_get_channel_load
 功能描述      : 获取某radio的信道占用率, vaps: radio下的所有vap
 输入参数      : wlc_bsscfg_t** bsscfgs
             int vap_num radio下的vap数量
 输出参数      : 100 - free
 返 回 值  : 信道利用率
 日    期    : 2020年8月15日
 作    者    : 尹家政
*****************************************************************************/
unsigned char td_em_vendor_get_channel_load(wlc_bsscfg_t **bsscfgs, int vap_num)
{
    wlc_bsscfg_t* bsscfg      = NULL;
    chanim_stats_t *chan_stat = NULL;
    int free = 100, i;

    if (vap_num < 1 || !bsscfgs) {
        return 0;
    }

    for (i = 0; i < vap_num; i++) {
        bsscfg = bsscfgs[i];
        if (bsscfg) {
            break;
        }
    }

    /* all vap has the same channel load, just get one */
    if (bsscfg) {
        chan_stat = wlc_lq_chanspec_to_chanim_stats(bsscfg->wlc->chanim_info,
                bsscfg->current_bss->chanspec);

        if (chan_stat) {
            free = chan_stat->chan_idle;
        }
    }

    return (100 - free);
}

/*****************************************************************************
 函 数 名  : td_em_get_apmetric
 功能描述      : ap测量
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月20日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_apmetric(em_osif osif, unsigned char *send_buf)
{
    unsigned int  len              = 0;
    unsigned char esp_ie           = 0;
    int   ampdu_ba_wsize           = 0;
    ratespec_t rspec               = {0};
    int rate = 0, ppdu_time        = 0;
    easymesh_ap_metrics_tlv_t *val = (easymesh_ap_metrics_tlv_t*)send_buf;
    wlc_info_t   *wlc              = NULL;

    if (!osif || !val) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    ampdu_ba_wsize = wlc_ampdu_tx_get_ba_tx_wsize(wlc->ampdu_tx);

    //The BSSID of the BSS
    memcpy(val->radio_bssid, osif->current_bss->BSSID.octet, TD_EM_MACADDRLEN);
    TD_EM_DBG_MSG(" %02X:%02X:%02X:%02X:%02X:%02X\n", val->radio_bssid[0], val->radio_bssid[1], val->radio_bssid[2],
                                                      val->radio_bssid[3], val->radio_bssid[4], val->radio_bssid[5]);

    // The Channel utilization
    val->channel_utilization = td_em_vendor_get_channel_load(&osif, TD_MAPS_VAP_PER_RADIO);

    //The Number of STAs current associated with this BSS
    val->assoc_sta_num = td_em_get_sta_num(osif);

    memset(&val->service_parameters[0], 0, 12);  //12表示AC=BE，AC=BK，AC=VO，AC=VI 每个3字节
    //Estimated Service Parameter Information Field
    esp_ie |= BIT(7);

    val->bit_service_parameters = esp_ie;

    /* access category */
    val->service_parameters[0] |= TD_ESP_BE;

    /* data format */
    if (wlc->pub->_amsdu_tx) {
        val->service_parameters[0] |= TD_ESP_AMPDU_ENABLED;
    }

    if (wlc->pub->_ampdu_tx) {
        val->service_parameters[0] |= TD_ESP_AMSDU_ENABLED;
    }

    /* BA window size */
    if (64 == ampdu_ba_wsize) {
        val->service_parameters[0] |= TD_ESP_BA_WSIZE_64;
    } else if (ampdu_ba_wsize >= 32) {
        val->service_parameters[0] |= TD_ESP_BA_WSIZE_32;
    } else if (ampdu_ba_wsize >= 16) {
        val->service_parameters[0] |= TD_ESP_BA_WSIZE_16;
    } else if (ampdu_ba_wsize >= 8) {
        val->service_parameters[0] |= TD_ESP_BA_WSIZE_8;
    } else if (ampdu_ba_wsize >= 6) {
        val->service_parameters[0] |= TD_ESP_BA_WSIZE_6;
    } else if (ampdu_ba_wsize >= 4) {
        val->service_parameters[0] |= TD_ESP_BA_WSIZE_4;
    } else if (ampdu_ba_wsize >= 2) {
        val->service_parameters[0] |= TD_ESP_BA_WSIZE_2;
    } else {
        val->service_parameters[0] |= TD_ESP_BA_WSIZE_NONE;
    }

    /* Estimated Air time fraction
     * 255 representing 100%
     */
    val->service_parameters[1] = 255 - val->channel_utilization;

    /* Data PPDU Duration Target
     * Duration to transmit 1 packet in
     * units of 50 microseconds
     */
    rspec = wlc_get_rspec_history(osif);
    rate = RSPEC2KBPS(rspec)/500;
    if (rate) {
        ppdu_time = TD_GET_PPDU_TIME(rate);    //bcm驱动内部换算方法，暂无文档参考
    }

    if (ppdu_time) {
        val->service_parameters[2] = (unsigned char)ppdu_time;
    } else {
        /* Avoid sending out 0 as ppdu_time */
        val->service_parameters[2] = 1;
    }

    len = sizeof(easymesh_ap_metrics_tlv_t);
    return len;

}

/*****************************************************************************
 函 数 名  : td_easymesh_get_scb_info
 功能描述      : get radio下scb信息
 输入参数      : em_osif osif
             struct ether_addr *client_mac
 输出参数      : scb
 返 回 值  : 
 日    期    : 2020年8月15日
 作    者    : 尹家政
*****************************************************************************/
scb_t *td_easymesh_get_scb_info(em_osif osif,struct ether_addr *client_mac)
{
    wlc_info_t   *wlc              = NULL;
    enum wlc_bandunit bandunit;
    
    if (!osif || !client_mac) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return NULL;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return NULL;
    }

    if (osif->up) {
        bandunit = CHSPEC_BANDUNIT(osif->current_bss->chanspec);
    } else {
        bandunit = CHSPEC_BANDUNIT(wlc->home_chanspec);
    }
    
    return wlc_scbfindband(wlc, osif, (struct ether_addr *)client_mac, bandunit);

}

/*****************************************************************************
 函 数 名  : td_em_get_assocstatrafficstats
 功能描述      : 关联sta的流量统计
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月20日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_assocstatrafficstats(em_osif osif, unsigned char *send_buf)
{
    int           len              = 0;
    unsigned char sta_mac[TD_EM_MACADDRLEN] = {0};
    wlc_info_t   *wlc              = NULL;
    struct scb   *scb              = NULL;
    struct scb   *bcmc_scb         = NULL;

    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    bcmc_scb = WLC_BCMCSCB_GET(wlc, osif);

    memcpy(sta_mac, send_buf, TD_EM_MACADDRLEN);
    TD_EM_DBG_MSG(" %02X:%02X:%02X:%02X:%02X:%02X\n", sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);

    scb = td_easymesh_get_scb_info(osif, (struct ether_addr *)sta_mac);

    if (scb) {
        easymesh_associated_sta_traffic_stat_tlv_t *val = (easymesh_associated_sta_traffic_stat_tlv_t*)send_buf;

        memcpy(val->assic_staaddr, sta_mac, TD_EM_MACADDRLEN);

        val->bytesSent = scb->scb_stats.tx_ucast_bytes + (bcmc_scb ? bcmc_scb->scb_stats.tx_ucast_bytes : 0); //tx bytes

        val->bytesReceived = scb->scb_stats.rx_ucast_bytes + scb->scb_stats.rx_mcast_bytes; //rx bytes

        val->packetsSent = scb->scb_stats.tx_pkts + (bcmc_scb ? bcmc_scb->scb_stats.tx_pkts : 0); //tx pkts

        val->packetsReceived = scb->scb_stats.rx_ucast_pkts + scb->scb_stats.rx_mcast_pkts; //rx pkts

        val->txPacketsErrors = scb->scb_stats.tx_failures; //tx fail

        val->rxPacketsErrors = scb->scb_stats.rx_decrypt_failures;     //rx fail

        val->retransmissionCount = scb->scb_stats.tx_pkts_retried;     //retransmission

        len = sizeof(easymesh_associated_sta_traffic_stat_tlv_t); //For TLV and length

    } else {
        memset(send_buf, 0, sizeof(easymesh_associated_sta_traffic_stat_tlv_t));
        len = 0;
    }

    return len;

}
/*****************************************************************************
 函 数 名  : td_em_get_assocstalinkmetric
 功能描述      : 关联sta的链路测量
 输入参数      : em_osif osif
             unsigned char *send_buf
             int sizeofbuf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月20日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_assocstalinkmetric(em_osif osif, unsigned char *send_buf, int sizeofbuf)
{
    int data_len = 0;
    unsigned char sta_mac[TD_EM_MACADDRLEN] = {0};
    easymesh_associated_sta_link_metrics_tlv_t *val = NULL;
    wlc_info_t   *wlc              = NULL;
    struct scb   *scb              = NULL;

    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    memcpy(sta_mac, send_buf, TD_EM_MACADDRLEN);
    TD_EM_DBG_MSG(" %02X:%02X:%02X:%02X:%02X:%02X\n", sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);

    data_len = sizeof(easymesh_associated_sta_link_metrics_tlv_t) + (1*sizeof(easymesh_numbssrepfor_sta_t));
    if (sizeofbuf < data_len) {
        TD_EM_DBG_PARAM_ERR("sizeof_buf < val!\n");
        return -1;
    }

    val = (easymesh_associated_sta_link_metrics_tlv_t *) kmalloc(data_len, GFP_ATOMIC);
    if (NULL == val) {
        TD_EM_DBG_PARAM_ERR("val kmalloc failed!\n");
        return -1;
    }
    scb = td_easymesh_get_scb_info(osif, (struct ether_addr *)sta_mac);

    if (scb) {
        int i = 0;
        memcpy(val->assic_staaddr, sta_mac, TD_EM_MACADDRLEN);
        val->bss_num = 1; //暂时只考虑单频组网。

        for (i = 0; i < 1; i++) {
            memcpy(val->bss_assoc_info[i].staassic_bssaddr, osif->current_bss->BSSID.octet, TD_EM_MACADDRLEN);
            val->bss_assoc_info[i].time_delta = 0;
            val->bss_assoc_info[i].tx_rate = (RSPEC2KBPS(scb->scb_stats.tx_rate)/1000);  
            val->bss_assoc_info[i].rx_rate = (RSPEC2KBPS(scb->scb_stats.rx_rate)/1000);
            val->bss_assoc_info[i].rcpi    = td_em_rssitorcpi(scb->rssi); //rssi converted to rcpi
        }

        memcpy(send_buf, val, data_len);

    } else {
        TD_EM_DBG_MSG(" %02X:%02X:%02X:%02X:%02X:%02X\n",
            sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);
        memset(send_buf, 0, data_len);
        data_len = 0;
    }
    kfree(val);
    return data_len;

}

/*****************************************************************************
 函 数 名  : td_em_get_extendedapmetric
 功能描述      : ap测量扩展
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月20日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_extendedapmetric(em_osif osif, unsigned char *send_buf)
{
    int data_len = 0;
    easymesh_ap_extended_metrics_tlv_t *val = (easymesh_ap_extended_metrics_tlv_t*)send_buf;
    wlc_info_t   *wlc              = NULL;
    data_len = sizeof(easymesh_ap_extended_metrics_tlv_t);

    if (!osif || !val) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    memcpy(val->bssid, osif->current_bss->BSSID.octet, TD_EM_MACADDRLEN);

    val->unicastBytes_tx   = dtoh32(wlc->pub->_cnt->txbyte);         //UnicastBytesSent
    val->broadcastBytes_tx = dtoh32(wlc->pub->_cnt->txbcast);        //BroadcastBytesSent
    val->multicastBytes_tx = dtoh32(wlc->pub->_cnt->txmulti);        //MulticastBytesSent

    val->unicastBytes_rx   = dtoh32(wlc->pub->_cnt->rxbyte);         //UnicastBytesReceived
    val->broadcastBytes_rx = dtoh32(wlc->pub->_cnt->rxbcast);        //BroadcastBytesReceived
    val->multicastBytes_rx = dtoh32(wlc->pub->_cnt->rxmulti);        //MulticastBytesReceived

    return data_len;

}

/*****************************************************************************
 函 数 名  : td_em_get_radiometric
 功能描述      : radio测量
 输入参数      : em_osif osif
             unsigned char *send_buf
 输出参数      : len :send_buf填充的长度
 返 回 值  : -1：osif || send_buf 为空
              其他：send_buf填充的长度
 日    期    : 2020年5月20日
 作    者    : 尹家政
*****************************************************************************/
int td_em_get_radiometric(em_osif osif, unsigned char *send_buf)
{
    int data_len = 0, iob_len = 512;
    wlc_info_t   *wlc                 = NULL;
    easymesh_radio_metrics_tlv_t *val = (easymesh_radio_metrics_tlv_t*)send_buf;
    wl_chanim_stats_t *iob = (wl_chanim_stats_t *)kmalloc(iob_len, GFP_ATOMIC);

    if (!iob) {
        TD_EM_DBG_PARAM_ERR("malloc iob error!\n");
        return -1;
    }
    if (!osif || !val) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        kfree(iob);
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        kfree(iob);
        return -1;
    }

    data_len = sizeof(easymesh_radio_metrics_tlv_t);

    memcpy(val->bssid, osif->cur_etheraddr.octet, TD_EM_MACADDRLEN);

    if (!td_em_chanim_get_stats_cb(wlc->chanim_info, iob, &iob_len, 0x01)) {
        val->noise        = (unsigned char)(td_em_rssitorcpi(iob->stats[0].bgnoise));
        val->receiveOther = (unsigned char)((iob->stats[0].ccastats[2] * 255) / 100);
        val->receiveSelf  = (unsigned char)((iob->stats[0].ccastats[1] * 255) / 100);
        val->transmit     = (unsigned char)((iob->stats[0].ccastats[0] * 255) / 100);
    }
    kfree(iob);
    return data_len;

}

/*****************************************************************************
 函 数 名  : td_easymesh_get_associated_sta_extended_link_metrics
 功能描述      : sta链路测量扩展
 输入参数      : em_osif osif
             easymesh_assocsta_extended_link_metricstlv_t *val
 输出参数      : 数据是否填充成功
 返 回 值  : -1：osif || send_buf 为空
              其他：数据未填成功
 日    期    : 2020年5月22日
 作    者    : 尹家政
*****************************************************************************/
int td_easymesh_get_associated_sta_extended_link_metrics(em_osif osif, unsigned char *send_buf)
{
    unsigned char  sta_mac[TD_EM_MACADDRLEN];
    easymesh_assocsta_extended_link_metricstlv_t *val = (easymesh_assocsta_extended_link_metricstlv_t *) send_buf;
    wlc_info_t   *wlc              = NULL;
    struct scb   *scb              = NULL;

    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    memcpy(sta_mac, &send_buf[0], TD_EM_MACADDRLEN);
    TD_EM_DBG_MSG(" %02X:%02X:%02X:%02X:%02X:%02X\n", sta_mac[0], sta_mac[1], sta_mac[2], sta_mac[3], sta_mac[4], sta_mac[5]);
    scb = td_easymesh_get_scb_info(osif, (struct ether_addr *)sta_mac);

    if (scb) {
        memcpy(val->sta_addr, sta_mac, TD_EM_MACADDRLEN);
        memcpy(val->sta_bss_info[0].bssid, osif->current_bss->BSSID.octet, TD_EM_MACADDRLEN);

        val->bss_num = 1;  //暂时只考虑单频，后续多频组网还需完善。
        val->sta_bss_info[0].lastdatadownlinkrate        = RSPEC2KBPS(scb->scb_stats.tx_rate); //kbps
        val->sta_bss_info[0].lastdatauplinkrate          = RSPEC2KBPS(scb->scb_stats.rx_rate); //kbps
        val->sta_bss_info[0].utilizationreceive          = (unsigned int)((wlc->pub->now - scb->assoctime) * 1000);
        val->sta_bss_info[0].utilizationtransmit         = (unsigned int)((wlc->pub->now - scb->assoctime) * 1000);
        TD_EM_DBG_MSG("lastdatadownlinkrate= %d\n",val->sta_bss_info[0].lastdatadownlinkrate);
        TD_EM_DBG_MSG("lastdatauplinkrate= %d\n",val->sta_bss_info[0].lastdatauplinkrate);
        TD_EM_DBG_MSG("utilizationreceive= %d\n",val->sta_bss_info[0].utilizationreceive);
        TD_EM_DBG_MSG("utilizationtransmit= %d\n",val->sta_bss_info[0].utilizationtransmit);

    } else {

        return 1; //For TLV and length
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_check_scan_state
 功能描述  : 检查扫描状态
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年6月1日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static bool td_em_check_scan_state(em_osif osif)
{
    wlc_info_t   *wlc              = NULL;
    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (SCAN_IN_PROGRESS(wlc->scan)) {
        return false;
    }

    return true;
}

/*****************************************************************************
 函 数 名  : td_em_channel_scan_trigger_ss
 功能描述      : 执行信道扫描
 输入参数      : struct rtl8192cd_priv *priv
             
 输出参数      : 
 返 回 值  : 
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
static int td_em_channel_scan_trigger_ss(em_osif osif)
{
    wlc_info_t   *wlc              = NULL;
    int params_size = WL_SCAN_PARAMS_FIXED_SIZE + WL_NUMCHANNELS * sizeof(uint16);
    wl_scan_params_t *params;
    struct ether_addr ether_bcast = {{255, 255, 255, 255, 255, 255}};
    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    params_size += WL_SCAN_PARAMS_SSID_MAX * sizeof(wlc_ssid_t);
    params = (wl_scan_params_t*)kmalloc(params_size,GFP_ATOMIC);
    if (NULL == params) {
        TD_EM_DBG_PARAM_ERR("Error allocating %d bytes for scan params\n", params_size);
        return -1;
    }

    memset(params, 0, params_size);

    //scan 参数设置
    memcpy(&params->bssid, &ether_bcast, TD_EM_MACADDRLEN);
    params->bss_type      = TD_DOT11_BSSTYPE_ANY;
    params->scan_type     = 0;
    params->nprobes       = -1;
    params->active_time   = -1;
    params->passive_time  = -1;
    params->home_time     = -1;
    params->channel_num   = 0;  //0表示自动信道

    if (td_em_check_scan_state(osif) == false) {
        kfree(params);
        return -1;
    }

    wlc_custom_scan(wlc, (char*)params, params_size, 0, WLC_ACTION_SCAN, osif);
    kfree(params);
    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_do_channelscan
 功能描述      : 请求信道扫描
 输入参数      : em_osif osif
 输出参数      : 数据是否填充成功
 返 回 值  : -1：osif || send_buf 为空
              其他：请求成功
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
int td_em_do_channelscan(em_osif osif)
{
    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (td_em_channel_scan_trigger_ss(osif) != 0) {
        return 1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_malloc_channel_list
 功能描述      : 给s_scan_results_list分配空间
 输入参数      : td_em_scanresult_t *array  //统计channel num 数组
             unsigned char count
 输出参数      :
 返 回 值  :
 日    期    : 2020年8月25日
 作    者    : 尹家政
*****************************************************************************/
int td_em_malloc_channel_list(td_em_scanresult_t *array, unsigned char count, td_em_scanresult_list_t * list)
{
    int i = 0;
    td_em_chscan_result_item *channel;

    if (!list) {
        TD_EM_DBG_PARAM_ERR("null ptr\n");
        return -1;
    }

    list->count = count;
    if (!list->count) {
        TD_EM_DBG_PARAM_ERR("channel list->count = 0\n");
        return -1;
    }
    list->channel_info = (char *) kmalloc(sizeof(td_em_chscan_result_item)*list->count, GFP_ATOMIC);
    if (NULL == list->channel_info) {
        TD_EM_DBG_PARAM_ERR("channel kmalloc failed!\n");
        return -1;
    }
    channel = (td_em_chscan_result_item *)list->channel_info;
    memset(channel, 0, sizeof(td_em_chscan_result_item)*list->count);

    for (i = 0; i < list->count; i++) {
        channel->channel = array[i].channel_num;
        if (!array[i].count) {
            TD_EM_DBG_PARAM_ERR("array[i].count = 0\n");
            kfree(list->channel_info);
            return -1;
        }
        channel->neighbor_list = (td_em_chscan_result_nbr_item*) kmalloc(sizeof(td_em_chscan_result_nbr_item)*array[i].count, GFP_ATOMIC);
        if (NULL == channel->neighbor_list) {
            TD_EM_DBG_PARAM_ERR("channel->neighbor_list kmalloc failed!\n");
            kfree(list->channel_info);
            return -1;
        }
        memset(channel->neighbor_list, 0, sizeof(td_em_chscan_result_nbr_item)*array[i].count);
        channel ++;
    }
    return 0;
}
/*****************************************************************************
 函 数 名  : td_em_get_channelscanresults_tlv_lenth
 功能描述      : 计算结构体长度
 输入参数      : em_osif *osif
             int *len
 输出参数      :
 返 回 值  :
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
void td_em_get_channelscanresults_tlv_lenth(em_osif osif, int *len, td_em_scanresult_list_t * list)
{
    wlc_info_t   *wlc              = NULL;
    td_em_scanresult_t channel_array[32] = {0};
    unsigned char i = 0, j = 0, ch_num = 0;
    int ctl_ch = -1;

    if (!osif || !len) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return ;
    }

    for (i = 0; i < wlc->scan_results->count && i < MAX_SCAN_STA_NUM; i++) {
        wlc_bss_info_t *bssinfo = wlc->scan_results->ptrs[i];
        ctl_ch = wf_chspec_ctlchan(bssinfo->chanspec);
        for (j = 0; j < 32; j++) {
            if (channel_array[j].channel_num == ctl_ch) {
                channel_array[j].count++;
                break;
            } else if (0 == channel_array[j].channel_num) {
                channel_array[j].channel_num = ctl_ch;
                channel_array[j].count++;
                ch_num++;
                break;
            }
        }
    }
    *len += (sizeof(easymesh_channel_scan_result_radio_t) + (sizeof(easymesh_channel_scan_result_channel_t) * ch_num));

    *len += (sizeof(easymesh_channel_scan_neighbor_t) * i);
    TD_EM_DBG_MSG("len= %d\n",*len);

    td_em_malloc_channel_list(channel_array, ch_num, list);
    td_em_get_update_channelscanresults(osif, list);
    //td_em_printk(len);
    return ;

}

/*****************************************************************************
 函 数 名  : td_em_get_chscan_bw_string
 功能描述      : 获取带宽
 输入参数      : chanspec_t chanspec
             char *bw
             unsigned char bw_sz
 输出参数      :
 返 回 值  :
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
char td_em_get_chscan_bw_string(chanspec_t chanspec, char *bw, unsigned char bw_sz)
{
    if (CHSPEC_IS20(chanspec)) {
        snprintf(bw, bw_sz, "%s", TD_CH_BW_20);
        return TD_20M;
    } else if (CHSPEC_IS40(chanspec)) {
        snprintf(bw, bw_sz, "%s", TD_CH_BW_40);
        return TD_40M;
    } else if (CHSPEC_IS80(chanspec)) {
        snprintf(bw, bw_sz, "%s", TD_CH_BW_80);
        return TD_80M;
    } else if (CHSPEC_IS160(chanspec)) {
        snprintf(bw, bw_sz, "%s", TD_CH_BW_160);
        return TD_160M;
    } else if (CHSPEC_IS8080(chanspec)) {
        snprintf(bw, bw_sz, "%s", TD_CH_BW_8080);
        return TD_80M_80M;
    } else {

    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_update_channelscanresults
 功能描述      : 保存信道扫描数据到全局数组
 输入参数      : em_osif *osif

 输出参数      :
 返 回 值  :
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
void td_em_get_update_channelscanresults(em_osif osif, td_em_scanresult_list_t * list)
{
    int ctl_ch = -1;
    int i = 0, k = 0;
    wlc_info_t   *wlc              = NULL;
    td_em_chscan_result_item *chscan_result = NULL;
    td_em_chscan_result_nbr_item *nbr_bss   = NULL;
    //char ssidbuf[SSID_FMT_BUF_LEN] = "";
    wlc_bss_info_t *bi = wlc->scan_results->ptrs[0];

    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    wlc = osif->wlc;
    if (!list || !wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return ;
    }

    for (k = 0; k < wlc->scan_results->count && k < MAX_SCAN_STA_NUM; k++) {
        bi = wlc->scan_results->ptrs[k];
        ctl_ch = wf_chspec_ctlchan(bi->chanspec);
        chscan_result = (td_em_chscan_result_item *)list->channel_info;

        for (i = 0; i < list->count; i++) {
            //chscan_result = (td_em_chscan_result_item *)list->channel_info + (sizeof(td_em_chscan_result_item)*i);
            if (!chscan_result) {
                break;
            }

            if (ctl_ch != chscan_result->channel) {
                chscan_result ++;
                continue;
            }

            nbr_bss = chscan_result->neighbor_list + chscan_result->num_of_neighbors;
            if (!nbr_bss) {
                break;
            }

            chscan_result->num_of_neighbors++;
            memcpy(nbr_bss->nbr_bssid, bi->BSSID.octet, sizeof(nbr_bss->nbr_bssid));
            if ( bi->SSID_len > sizeof(nbr_bss->nbr_ssid.SSID)) {
                bi->SSID_len = sizeof(nbr_bss->nbr_ssid.SSID);
                //wbd_wl_format_ssid(ssidbuf, bi->SSID, bi->SSID_len);
            }
            memcpy(nbr_bss->nbr_ssid.SSID, bi->SSID, bi->SSID_len);
            nbr_bss->nbr_ssid.SSID_len = bi->SSID_len;

            nbr_bss->nbr_rcpi = (unsigned char)(td_em_rssitorcpi(bi->RSSI));
            nbr_bss->ch_bw_length = td_em_get_chscan_bw_string(bi->chanspec,
            (char*)nbr_bss->ch_bw, sizeof(nbr_bss->ch_bw));
                /* Update the Neighbor BSS fields */
            
            /* Fetch ChanUtil & StaCnt, if BSSLoad Element Present. Else omitted 
            if (dtoh32(bi->ie_length) && (0 == blanket_get_qbss_load_element(bi,
                &nbr_bss->channel_utilization, &nbr_bss->station_count))) {
                nbr_bss->chscan_result_nbr_flag |= MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT;
            } else {
                nbr_bss->chscan_result_nbr_flag = 0;
                nbr_bss->channel_utilization = 0;
                nbr_bss->station_count = 0;
            }*/

            chscan_result->scan_status_code = 0;  //scan successful
            /* 1 : Scan was an Active scan */
            chscan_result->chscan_result_flag |= TD_CHSCAN_RES_SCANTYPE;
            chscan_result->noise = (unsigned char)bi->phy_noise;
            chscan_result->utilization = td_em_vendor_get_channel_load(&osif, TD_MAPS_VAP_PER_RADIO);
            chscan_result ++;
            break;
        }
    }
    return ;
}

/*****************************************************************************
 函 数 名  : td_em_ap_save_channelscanresults
 功能描述      : 保存数据到buf
 输入参数      : em_osif osif
             unsigned char* result_buf
 输出参数      :
 返 回 值  :
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
void td_em_ap_save_channelscanresults(em_osif osif, unsigned char* result_buf, td_em_scanresult_list_t * list)
{
    unsigned char i, j;
    easymesh_channel_scan_result_radio_t   *val  = NULL;
    easymesh_channel_scan_result_channel_t *val1 = NULL;
    easymesh_channel_scan_neighbor_t       *val2 = NULL;
    wlc_info_t   *wlc                            = NULL;

    td_em_chscan_result_item *chscan_result = NULL;
    td_em_chscan_result_nbr_item *nbr_bss   = NULL;

    if (!list || !osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return ;
    }

    val = (easymesh_channel_scan_result_radio_t *)result_buf;
    val->msg_flag   = TD_EASYMESH_CHANNEL_SCAN_RESULT;

    memcpy(val->priv_bssid, osif->cur_etheraddr.octet, TD_EM_MACADDRLEN);
    TD_EM_DBG_MSG(" %02X:%02X:%02X:%02X:%02X:%02X\n", osif->cur_etheraddr.octet[0], osif->cur_etheraddr.octet[1], osif->cur_etheraddr.octet[2],
                                                      osif->cur_etheraddr.octet[3], osif->cur_etheraddr.octet[4], osif->cur_etheraddr.octet[5]);

    val->channel_nr   = list->count;

    result_buf += sizeof(easymesh_channel_scan_result_radio_t);

    for (i = 0; i < list->count; i++) {
        chscan_result = (td_em_chscan_result_item *)list->channel_info + i;
        if (!chscan_result) {
            break;
        }
        val1 = (easymesh_channel_scan_result_channel_t *)result_buf;
        val1->channel       = chscan_result->channel; //channel
        val1->scan_status   = chscan_result->scan_status_code; //scan_status
        memset(val1->timestamp, 0, 10); //timestamp
        TD_EM_DBG_MSG("val1->channel=  %d\n",val1->channel);

        val1->channel_utilization   = chscan_result->utilization; //channel_utilization
        val1->noise                 = chscan_result->noise; //noise
        val1->neighbor_nr           = chscan_result->num_of_neighbors; //neighbor_nr
        val1->scan_timer            = 0; //_wlc_get_next_chan_scan_time(wlc->scan);

        result_buf += sizeof(easymesh_channel_scan_result_channel_t);

        for (j = 0; j < chscan_result->num_of_neighbors; j++) {

            val2 = (easymesh_channel_scan_neighbor_t *)result_buf;
            nbr_bss = chscan_result->neighbor_list + j;
            if (!nbr_bss) {
                break;
            }
            memcpy(val2->bssid, nbr_bss->nbr_bssid, TD_EM_MACADDRLEN); //neighbor_nr
            if (nbr_bss->nbr_ssid.SSID_len > TD_EM_WLAN_SSID_MAXLEN) {
                nbr_bss->nbr_ssid.SSID_len = TD_EM_WLAN_SSID_MAXLEN;
            }
            val2->ssid_length = nbr_bss->nbr_ssid.SSID_len;//ssid_length
            memcpy(val2->ssid, nbr_bss->nbr_ssid.SSID, nbr_bss->nbr_ssid.SSID_len); //ssid
            TD_EM_DBG_MSG("SSID_len=  %d\n",nbr_bss->nbr_ssid.SSID_len);
            TD_EM_DBG_MSG("nbr_ssid.SSID=  %s\n",nbr_bss->nbr_ssid.SSID);
            TD_EM_DBG_MSG("val2->ssid=  %s\n",val2->ssid);

            val2->bss_load            = nbr_bss->chscan_result_nbr_flag;
            val2->signal_strength     = nbr_bss->nbr_rcpi; //signal_strength
            val2->channel_band_width  = nbr_bss->ch_bw_length; //channel_band_width
            val2->channel_utilization = nbr_bss->channel_utilization; //channel_utilization
            val2->station_count       = nbr_bss->station_count; //station_count

            result_buf += sizeof(easymesh_channel_scan_neighbor_t);
        }
    }

    return ;

}

/*****************************************************************************
 函 数 名  : td_em_priv_malloc_free
 功能描述      : 释放申请的空间
 输入参数      : em_osif osif
 输出参数      : 
 返 回 值  :
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
void td_em_priv_malloc_free(em_osif osif, td_em_scanresult_list_t * list)
{
    td_em_chscan_result_item *chscan_result = NULL;
    td_em_chscan_result_nbr_item *nbr_bss   = NULL;
    unsigned char i;

    if (!list) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    //free memory
    if (list->channel_info) {
        for (i = 0; i < list->count; i++) {
            chscan_result = (td_em_chscan_result_item *)list->channel_info + i;
            nbr_bss = chscan_result->neighbor_list;
            if (nbr_bss) {
                kfree(nbr_bss);
            }
        }
        kfree(list->channel_info);
    }
    return ;
}

/*****************************************************************************
 函 数 名  : td_easymesh_beacon_metrics_rsp_notifiy
 功能描述      : beacon测量信息上报
 输入参数      : void *sta_if
             unsigned char *result_buf
             int *sizeofbuf
 输出参数      : 数据是否填充成功
 返 回 值  : -1：osif || send_buf 为空
              0：数据填成功
 日    期    : 2020年8月10日
 作    者    : liyahui
*****************************************************************************/
int td_easymesh_beacon_metrics_rsp_notifiy(void *sta_if, unsigned char *result_buf, int *sizeofbuf)
{
    int date_len = 0;
    int beacon_len = 0;
    unsigned char *buf = NULL;
    unsigned char i = 0;
    em_beacon_metrics_rsp_tlv_t *val = NULL;
    unsigned char sta_mac[TD_EM_MACADDRLEN] = {0};
    char *recev_buf = (char *)sta_if;
    td_dot11k_rrm_info_t *rrm_info = NULL;

    if (!recev_buf || !result_buf || !sizeofbuf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    
    rrm_info = (td_dot11k_rrm_info_t *)kmalloc(sizeof(td_dot11k_rrm_info_t), GFP_ATOMIC);
    if (!rrm_info) {
        TD_EM_DBG_PARAM_ERR("kmalloc  failed!\n");
        return -1;
    }
    //帧的总长度保存在帧的sta_if的第一个子节和第二个字节
    beacon_len = (recev_buf[0] << 8) + recev_buf[1]; 
    
    //偏移掉前两个字节，前两个字节保存的是长度
    memcpy(sta_mac, recev_buf + 2, TD_EM_MACADDRLEN);
    //偏移掉长度字段和sta_mac地址字段
    buf = recev_buf + 2 + TD_EM_MACADDRLEN;
    TD_EM_DBG_TRACE(" STA MAC -%pM \n", sta_mac);
    TD_EM_DBG_TRACE(" Body len -%d \n", beacon_len);
    memset(rrm_info, 0, sizeof(td_dot11k_rrm_info_t));

    td_parse_rm_report(rrm_info, buf, beacon_len);

    date_len = sizeof(em_beacon_metrics_rsp_tlv_t)+sizeof(em_beacon_element_info_t)*rrm_info->beacon_report_num;

    if (date_len > *sizeofbuf) {
        TD_EM_DBG_PARAM_ERR(" date_len > netlink_bufsize  drop\n");
        kfree(rrm_info);
        return -1;
    }
    
    val = (em_beacon_metrics_rsp_tlv_t *) result_buf;
    val->info_type = TD_EASYMESH_BEACON_METRIC_RSP;
    val->beacon_rep_num = rrm_info->beacon_report_num;
    val->beacon_ie  = DOT11_MNG_MEASURE_REPORT_ID;
    val->element_id = DOT11_MEASURE_TYPE_BEACON;

    memcpy(val->sta_addr, sta_mac, TD_EM_MACADDRLEN);

    for(i = 0; i <= val->beacon_rep_num; i++) {

        val->beacon_emt[i].measure_result = rrm_info->measure_result;
        if(MEASUREMENT_INCAPABLE == rrm_info->measure_result || MEASUREMENT_REFUSED == rrm_info->measure_result) {
            break;
        }
        val->beacon_emt[i].beacon_report_info.measure_token = rrm_info->beacon_report[i].info.measure_token;
        val->beacon_emt[i].beacon_report_info.measure_mode= rrm_info->beacon_report[i].info.measure_mode;
        val->beacon_emt[i].beacon_report_info.measure_report_type = rrm_info->beacon_report[i].info.measure_report_type;
        val->beacon_emt[i].beacon_report_info.op_class = rrm_info->beacon_report[i].info.op_class;
        val->beacon_emt[i].beacon_report_info.channel  = rrm_info->beacon_report[i].info.channel;
        val->beacon_emt[i].beacon_report_info.measure_time_hi  = (rrm_info->beacon_report[i].info.measure_time_hi);
        val->beacon_emt[i].beacon_report_info.measure_time_lo  = (rrm_info->beacon_report[i].info.measure_time_lo);
        val->beacon_emt[i].beacon_report_info.measure_duration = (rrm_info->beacon_report[i].info.measure_duration);
        val->beacon_emt[i].beacon_report_info.frame_info = rrm_info->beacon_report[i].info.frame_info;
        val->beacon_emt[i].beacon_report_info.RCPI = rrm_info->beacon_report[i].info.RCPI;
        val->beacon_emt[i].beacon_report_info.RSNI = rrm_info->beacon_report[i].info.RSNI;
        memcpy(val->beacon_emt[i].beacon_report_info.bssid, rrm_info->beacon_report[i].info.bssid, TD_EM_MACADDRLEN);
        val->beacon_emt[i].beacon_report_info.antenna_id = rrm_info->beacon_report[i].info.antenna_id;
        val->beacon_emt[i].beacon_report_info.parent_tsf = (rrm_info->beacon_report[i].info.parent_tsf);
        memcpy(val->beacon_emt[i].subelements, rrm_info->beacon_report[i].subelements, rrm_info->beacon_report[i].subelements_len);//?
        val->beacon_emt[i].subelements_len = rrm_info->beacon_report[i].subelements_len;
    }

    *sizeofbuf = date_len;
    kfree(rrm_info);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_easymesh_1905_txlink_metric
 功能描述      : 联合链路tx测量
 输入参数      : em_osif osif
             unsigned char *result_buf
             int *sizeofbuf
 输出参数      : 数据是否填充成功
 返 回 值  : -1：osif || send_buf 为空
              0：表示数据获取成功
              其他：数据未填成功
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
int td_easymesh_1905_txlink_metric(em_osif osif, unsigned char *result_buf, int *sizeofbuf)
{
    int buflen = 512;
    easymesh_1905_txlink_metric_t *     val;
    unsigned char sta_mac[TD_EM_MACADDRLEN] = {0};
    wlc_info_t   *wlc              = NULL;
    struct scb   *scb              = NULL;
    struct scb   *bcmc_scb         = NULL;
    unsigned short link_available  = 0;
    wl_chanim_stats_t *iob = (wl_chanim_stats_t*) result_buf;

    if (!osif || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    bcmc_scb = WLC_BCMCSCB_GET(wlc, osif);
    memcpy(&sta_mac[0], &iob[0], TD_EM_MACADDRLEN);
    TD_EM_DBG_TRACE("sta_mac  %02X:%02X:%02X:%02X:%02X:%02X \n", sta_mac[0], sta_mac[1],
                                                   sta_mac[2], sta_mac[3],
                                                   sta_mac[4], sta_mac[5]);

    if (0 != td_em_chanim_get_stats_cb(wlc->chanim_info, iob, &buflen, 0x1)) {
        TD_EM_DBG_PARAM_ERR("get stats error!\n");
        return -1;
    }

    link_available = iob->stats[0].ccastats[1] + iob->stats[0].ccastats[6] +
        iob->stats[0].ccastats[7] + iob->stats[0].ccastats[8];

    memset(result_buf,0 ,512);
    val = (easymesh_1905_txlink_metric_t *) result_buf;
    scb = td_easymesh_get_scb_info(osif, (struct ether_addr *)sta_mac);

    if (scb) {
        val->linkavailability = (link_available < 100) ? link_available : 100;  //0 ~ 100
        val->packeterrors     = scb->scb_stats.tx_failures;
        val->tx_packet        = scb->scb_stats.tx_pkts + (bcmc_scb ? bcmc_scb->scb_stats.tx_pkts : 0);
        val->tx_avarage       = (RSPEC2KBPS(scb->scb_stats.tx_rate)/1000);  //macThroughputCapacity最大吞吐量   Mbps
        val->phy_rate         = 0;
        TD_EM_DBG_MSG("tx_avarage= %d\n",val->tx_avarage);
    } else {
        *sizeofbuf = 0;
        TD_EM_DBG_PARAM_ERR("null pstat!\n");
        return 1;
    }

    *sizeofbuf = sizeof(easymesh_1905_txlink_metric_t);
    return 0;

}

/*****************************************************************************
 函 数 名  : td_easymesh_1905_rxlink_metric
 功能描述      : 联合链路rx测量
 输入参数      : em_osif osif
             unsigned char *result_buf
             int *sizeofbuf
 输出参数      : 数据是否填充成功
 返 回 值  : -1：osif || send_buf 为空
              0：表示数据获取成功
              其他：数据未填成功
 日    期    : 2020年5月29日
 作    者    : 尹家政
*****************************************************************************/
int td_easymesh_1905_rxlink_metric(em_osif osif, unsigned char *result_buf, int *sizeofbuf)
{
    unsigned char                       sta_mac[TD_EM_MACADDRLEN];
    easymesh_1905_rxlink_metric_t *     val;
    wlc_info_t   *wlc              = NULL;
    struct scb   *scb              = NULL;

    if (!osif || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    val = (easymesh_1905_rxlink_metric_t *) result_buf;

    memcpy(&sta_mac[0], val, TD_EM_MACADDRLEN);
    scb = td_easymesh_get_scb_info(osif, (struct ether_addr *)sta_mac);

    if (scb) {
        val->packeterrors     = 0;
        val->rx_packet        = scb->scb_stats.rx_ucast_pkts + scb->scb_stats.rx_mcast_pkts;
        val->rssi             = (-(scb->rssi));
    } else {
        *sizeofbuf = 0;
        TD_EM_DBG_PARAM_ERR("null pstat!\n");
        return 1;
    }

    *sizeofbuf = sizeof(easymesh_1905_rxlink_metric_t);
    return 0;
}

#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
/******************************************************************
Function:     td_em_get_ext_assoc_status
Description : Checking the Connection Status
Input       : em_osif osif
Output      : No
Return      : 0：Cannot connect
              1：Normal connection
*******************************************************************/
int td_em_get_ext_assoc_status(em_osif osif)
{
    int i;
    wlc_bsscfg_t *bsscfg = NULL;
    wlc_info_t   *wlc = NULL;

    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return STA_ASSOC_UNALLOW;
    }

    wlc = osif->wlc;

    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return STA_ASSOC_UNALLOW;
    }

    /* find sta inf and find assoc status */
    FOREACH_UP_AP(wlc, i, bsscfg) {
        if (((bsscfg->map_attr & MAP_EXT_ATTR_BACKHAUL_BSS) || (g_em_mesh_cfg & EM_XMESH_BIT)) 
            && (TD_EM_CTROL_CONNECT == bsscfg->td_ethx_connect)) {
            TD_EM_DBG_TRACE("STA_ASSOC_ALLOW\n");
            return STA_ASSOC_ALLOW;
        }
    }

    TD_EM_DBG_TRACE("STA_ASSOC_UNALLOW\n");
    return STA_ASSOC_UNALLOW;
}

/******************************************************************
Function:     td_em_bsta_kick_sta
Description : Kick out the parent node
Input       : void *sta
              void *arg
Output      : No
Return      : 0successful, -1failure
*******************************************************************/
int td_em_bsta_kick_sta(void *sta, void *arg)
{
    wlc_info_t *wlc = NULL;
    association_client_sta_info_t linktime;

    if (!sta || !arg) {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }

    wlc_bsscfg_t *bsscfg = (wlc_bsscfg_t *)arg;
    memset(&linktime, 0, sizeof(linktime));
    wlc = bsscfg->wlc;

    if (td_em_get_sta_link_time(sta, &linktime) <= 0) {
        return 0;
    }

    /*To prevent taking IP too slowly and triggering the kick node by mistake, 
    time interval judgment is needed to determine the behavior*/
    if ((linktime.link_time >= TD_EM_KICK_STA_TIME) || TD_IS_NULL_MAC(bsscfg->BSSID.octet)) {
        TD_EM_DBG_DANGER("is sta linktime %d, sta mac :%pM\n", linktime.link_time, linktime.sta_macaddr);
        wlc_ioctl(wlc, WLC_SCB_DEAUTHENTICATE, linktime.sta_macaddr, TD_EM_MACADDRLEN, bsscfg->wlcif);
    }

    return 0;
}

/******************************************************************
Function:     td_em_set_bbss_status
Description : Configuring interface td_ethx_connect;
              Reply to probe request and  respond to association, 
              Use it to prevent loops
Input       : wlc_info_t *wlc
              td_em_control_state_e status
Output      : No
*******************************************************************/
void td_em_set_bbss_status(wlc_info_t *wlc, td_em_control_state_e status)
{
    em_osif osif_iter = NULL;
    int i = 0;

    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    /*Send a link_up message to the Wserver, To enable the interface */
    FOREACH_AP(wlc, i, osif_iter) {
        if ((osif_iter->map_attr & MAP_EXT_ATTR_BACKHAUL_BSS) || (g_em_mesh_cfg & EM_XMESH_BIT)) {
            if (osif_iter->td_ethx_connect != status) {
                TD_EM_DBG_DANGER("state_change to  %d -> %d!\n", osif_iter->td_ethx_connect, status);
                osif_iter->td_ethx_connect = status;
                if (TD_EM_CTROL_CONNECT == status) {
                    if (g_em_mesh_cfg & EM_XMESH_BIT) {
                        /*link up=0x1*/
                        TD_EM_DBG_MSG("em_xmesh_bit \n");
                        td_em_extend_down_notify(EM_XMESH_BIT|EM_LINKUP);
                    } else {
                        TD_EM_DBG_MSG("easymesh bit\n");
                        td_em_extend_down_notify(EM_LINKUP);
                    }
                }
            }
            break;
        }
    }

    return;
}

/******************************************************************
Function:     td_em_action_bsta_status
Description : In the BSTA role, the parameter is 0;
              To prevent the loop action
Input       : wlc_info_t *wlc
              td_em_control_state_e status
Output      : No
*******************************************************************/
void td_em_action_bsta_status(wlc_info_t *wlc, td_em_control_state_e status)
{
    int i = 0;
    em_osif osif_iter = NULL;

    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    /* The parent node needs to be kicked off;
     Send a link_down message to the Wserver, To close the interface */
    FOREACH_STA(wlc, i, osif_iter) {
        if ((osif_iter->map_attr & MAP_EXT_ATTR_BACKHAUL_STA) 
            || (g_em_mesh_cfg & EM_XMESH_BIT)) {
            if (TD_EM_CTROL_DISCONNECT == status) {
                td_em_iterate_sta_list(osif_iter, 0, (void *)osif_iter, td_em_bsta_kick_sta);
                TD_EM_DBG_DANGER("TD_EM_CTROL_DISCONNECT \n");
                if (g_em_mesh_cfg & EM_XMESH_BIT) {
                    TD_EM_DBG_MSG("em_xmesh_bit \n");
                    /* link down=0x2 */
                    td_em_extend_down_notify(EM_XMESH_BIT);
                } else {
                    TD_EM_DBG_MSG("easymesh bit \n");
                    td_em_extend_down_notify(0);
                }
            } else {
            }
            break;
        }
    }

    return;
}

int td_em_set_xmesh_hb_drv(em_osif osif, unsigned char *send_buf)
{
    wlc_info_t *wlc = NULL;
    td_em_control_state_e status ;

    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    /* Heartbeat packets and timers can change the interface status */
    status = *(td_em_control_state_e *)send_buf;
    wlc = osif->wlc;

    TD_EM_DBG_HBINFO("[wl%d.%d] : in status = %d.\n", 
        wlc->pub->unit, WLC_BSSCFG_IDX(osif), status);

    td_em_set_bbss_status(wlc, status);
    td_em_action_bsta_status(wlc, status);

    /*This prevents the upper-layer from 
        sending heartbeat packets without delivering the role configuration */
    if (g_hb_timer[wlc->pub->unit].hb_timer) {
        /*receive heartbeat packet then refresh the timer*/
        td_em_refresh_hbtimer(wlc, g_hb_timer[wlc->pub->unit].hb_timer);
        TD_EM_DBG_HBINFO("[wl%d.%d] : td_em refresh hbtimer.\n", 
            wlc->pub->unit, WLC_BSSCFG_IDX(osif));
    } else {
        TD_EM_DBG_HBINFO("[wl%d.%d] : td_em init hb timer.\n", 
            wlc->pub->unit, WLC_BSSCFG_IDX(osif));
        td_em_init_hbtimer(osif);
    }

    return 0;
}

int td_em_xmesh_set_bss_drv(em_osif osif, unsigned char *buf)
{
    int idx = 0;

    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1 ;
    }

    idx = *buf;
    if (idx < 0 || idx >= TD_EM_MAX_BAND) {
        TD_EM_DBG_PARAM_ERR("Input parameter load!\n");
        return -1;
    }

    /*For the interface delivered from the upper layer,
    record the WLC address and use it in the timer*/
    g_em_bss[idx] = (void *)osif->wlc;

    return 0;
}

static void td_em_hbtimer_handler(void *arg)
{
    /* The default parameter is 0 and a heartbeat 
    packet is required to update the parameter */
    td_em_control_state_e send_buf = TD_EM_CTROL_DISCONNECT;
    wlc_info_t *wlc = NULL;

    wlc = (wlc_info_t *)arg;
    TD_EM_DBG_HBINFO("[wl%d.%d] : td_em hbtimer handler.\n", wlc->pub->unit, WLC_BSSCFG_IDX(wlc->primary_bsscfg));
    td_em_set_xmesh_hb_drv(wlc->primary_bsscfg, (unsigned char *)&send_buf);

    return;
}

static void td_em_init_hbtimer(em_osif osif)
{
    wlc_info_t *wlc = NULL;
    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return ;
    }

    wlc = osif->wlc;
    if (g_hb_timer[wlc->pub->unit].hb_timer) {
        TD_EM_DBG_RET_ERR("[wl%d.%d] : The timer is suspended and does not need to be initialized again!\n", 
            wlc->pub->unit, WLC_BSSCFG_IDX(osif));
        return;
    }

    g_hb_timer[wlc->pub->unit].hb_timer = wl_init_timer(osif->wlc->wl, td_em_hbtimer_handler, osif->wlc, "hbtimer");
    g_hb_timer[wlc->pub->unit].wl = osif->wlc->wl;
    if (!g_hb_timer[wlc->pub->unit].hb_timer || !g_hb_timer[wlc->pub->unit].wl) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return ;
    }
    wl_add_timer(osif->wlc->wl, g_hb_timer[wlc->pub->unit].hb_timer, TD_EM_HB_TIMEOUT * 1000, FALSE);

    TD_EM_DBG_HBINFO("[wl%d.%d] : td_em init hb timer finished.\n", wlc->pub->unit, WLC_BSSCFG_IDX(osif));
    return;
}

void td_em_deinit_hbtimer(void)
{
    int idx = 0;
    wlc_info_t *wlc = NULL;

    for (idx = 0; idx < TD_EM_MAX_BAND ; idx++) {
        if (g_em_bss[idx]) {
            wlc = (wlc_info_t *)g_em_bss[idx];
        } else {
            continue;
        }

        if (g_hb_timer[idx].hb_timer) {
            wl_free_timer(wlc->wl, g_hb_timer[idx].hb_timer);
            g_hb_timer[idx].wl = NULL;
            g_hb_timer[idx].hb_timer = NULL;
            g_em_bss[idx] = NULL;
        }
    }

    TD_EM_DBG_HBINFO("td_em deinit hb timer finished.\n");
    return;
}

static void td_em_refresh_hbtimer(wlc_info_t *wlc, struct wl_timer * t)
{
    wl_del_timer(wlc->wl, t);
    wl_add_timer(wlc->wl, t, TD_EM_HB_TIMEOUT * 1000, FALSE);

    return;
}

#else
int td_em_get_ext_assoc_status(em_osif osif)
{
    int i;
    wlc_bsscfg_t *bsscfg = NULL;
    int is_allow = STA_ASSOC_UNALLOW;
    int is_eth_connect = 0;
    bool sta_inf_find = false;
    wlc_info_t   *wlc = NULL;

    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return STA_ASSOC_UNALLOW;
    }

    wlc = osif->wlc;

    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return STA_ASSOC_UNALLOW;
    }

    /* find sta inf and find assoc status */
    FOREACH_STA(wlc, i, bsscfg) {
         if (bsscfg->associated) {
            g_multiap_assoc_status |= (wlc->pub->unit+1); /* unit statrtidx=0 */
            is_allow = STA_ASSOC_ALLOW;
        } else {
            g_multiap_assoc_status &= (~(wlc->pub->unit+1));
            is_allow = STA_ASSOC_UNALLOW;
        }
        if (bsscfg->td_ethx_connect) {
            is_eth_connect = AGENT_ETH_UP;
        }
        sta_inf_find = true;
    }

    if (0 == g_multiap_assoc_status && sta_inf_find) {
        is_allow = STA_ASSOC_UNALLOW;
    } else {
        is_allow = STA_ASSOC_ALLOW;
    }

    if (is_eth_connect) {
        is_allow = STA_ASSOC_ALLOW; 
    }

    return is_allow;
}

#endif

/*****************************************************************************
 函 数 名  : td_em_eth_connect_driver
 功能描述  : 获取有线连接状态
 输入参数  : em_osif osif               
             unsigned char *send_buf  
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年8月10日
    作    者   : hongguiyang
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_eth_connect_driver(em_osif osif, unsigned char *send_buf)
{
    static int lst_eth_status = 0;

    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (osif->_ap) {
        TD_EM_DBG_PARAM_ERR("not support ap mode!\n");
        return -1;
    }    

    osif->td_ethx_connect = 0;

    /* 1.上层是通过心跳包检测是否连接
           2.心跳包通道可能来自无线或者有线
           3.因此需要判断如果没有无线关联，则来自有线
           4.如果有线无线同时存在，则可以认为是来自无线。
           5.该方案只要是解决只有有线连接的情况
       */  
    if (!osif->associated && (*send_buf != 0)) {
        osif->td_ethx_connect = *send_buf;
    }

    if ((0 == osif->td_ethx_connect) 
         && (!osif->associated) 
         && (lst_eth_status != osif->td_ethx_connect)) {
        if (g_em_mesh_cfg & EM_XMESH_BIT) {
            td_em_extend_down_notify(EM_XMESH_BIT);
        } else {
            td_em_extend_down_notify(0);     
        }
    }

    lst_eth_status = osif->td_ethx_connect;

    TD_EM_DBG_TRACE("td_ethx_connect(%d) lst_eth_status(%d) indx=%d\n", osif->td_ethx_connect, lst_eth_status, osif->wlcif->index);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_priority_assoc_flag_drv
 功能描述  : 获取有线连接状态
 输入参数  : em_osif osif               
             unsigned char *send_buf  
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年8月10日
    作    者   : hongguiyang
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_priority_assoc_flag_drv(em_osif osif, unsigned char *send_buf)
{
    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (*send_buf) {
        em_drv_set_cfg(EM_PRIO_ASSOC_BIT);   
    } else {
        em_drv_clr_cfg(EM_PRIO_ASSOC_BIT);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_set_role_drv
 功能描述  :   Record the mesh role of the interface
 输入参数  : em_osif osif               
             unsigned char *send_buf  
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年8月10日
    作    者   : hongguiyang
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_set_role_drv(em_osif osif, unsigned char *send_buf)
{
    wlc_info_t *wlc = NULL;
    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    wlc = osif->wlc;
    /*Only child nodes need to start the timer; 
    If the active node is configured, delete the timer*/
    if (*send_buf) {
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
        if (g_hb_timer[wlc->pub->unit].hb_timer) {
            wl_free_timer(wlc->wl, g_hb_timer[wlc->pub->unit].hb_timer);
            g_hb_timer[wlc->pub->unit].wl = NULL;
            g_hb_timer[wlc->pub->unit].hb_timer = NULL;
        }
#endif
        em_drv_set_cfg(EM_CONTROLLER_BIT);
    } else {
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
        TD_EM_DBG_HBINFO("[wl%d.%d] : td_em set role of drv(%d).\n", 
            wlc->pub->unit, WLC_BSSCFG_IDX(osif), *send_buf);
        td_em_init_hbtimer(osif);
#endif
        em_drv_clr_cfg(EM_CONTROLLER_BIT);
    }

    printk("%s, g_em_mesh_cfg=%x\n", __func__,g_em_mesh_cfg);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_set_xmesh_enable_drv
 功能描述  : 获取有线连接状态
 输入参数  : em_osif osif               
             unsigned char *send_buf  
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年8月10日
    作    者   : hongguiyang
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_set_xmesh_enable_drv(em_osif osif, unsigned char *send_buf)
{
    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (*send_buf) {
        em_drv_set_cfg(EM_XMESH_BIT);   
    } else {
        em_drv_clr_cfg(EM_XMESH_BIT);
    }

    if (!osif->associated) {
        td_em_extend_down_notify(EM_XMESH_BIT);
    }

    printk("%s, g_em_mesh_cfg=%x\n", __func__,g_em_mesh_cfg);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_eth_connect_status
 功能描述  : 通告wserver有线状态
 输入参数  : em_osif osif               
             unsigned char *send_buf  
             sizeofbuf
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年8月10日
    作    者   : hongguiyang
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_eth_connect_status(em_osif osif, unsigned char *send_buf, int *sizeofbuf)
{
    int *tmp = NULL;

    if (!osif || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (osif->_ap) {
        TD_EM_DBG_PARAM_ERR("not support ap mode!\n");
        return -1;
    }    
    
    tmp = (int *)send_buf;

    *tmp = osif->td_ethx_connect;
    *sizeofbuf = sizeof(int);

    TD_EM_DBG_TRACE("td_ethx_connect(%d)\n", osif->td_ethx_connect);

    return 0;
}
/*****************************************************************************
 函 数 名  : td_em_get_cur_opclass
 功能描述  : 获取当前信道和频宽所属的opclass的值
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : static
 
 修改历史      :
  1.日    期   : 2021年7月23日
    作    者   : 王建强
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_get_cur_opclass(em_osif osif)
{
    bcmwifi_rclass_opclass_t rclass = 0;
    wlc_info_t *wlc = NULL;
    bcmwifi_rclass_type_t rc_type;

    wlc = osif->wlc;
    if (!wlc) {
        return -1;
    }

    rc_type = wlc_channel_get_cur_rclass(wlc);
    bcmwifi_rclass_get_opclass(rc_type, wlc->default_bss->chanspec, &rclass);
    return rclass;
}
/*****************************************************************************
 函 数 名  : td_do_beacon_request
 功能描述  : 执行beacon request请求
 输入参数  : em_osif osif               
             unsigned char *send_buf  
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年8月10日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
int td_do_beacon_request(em_osif osif, unsigned char *send_buf)
{
    easymesh_beacon_metrics_query_tlv_t *val = (easymesh_beacon_metrics_query_tlv_t*)send_buf;
    wl_action_frame_t * action_frame;
    wl_af_params_t * af_params;
    dot11_rmreq_t *rmreq_ptr;
    dot11_rmreq_bcn_t *rmreq_bcn_ptr;
    static int static_tocken = 0;
    char *pcur = NULL;
    char *ioctl_buf = NULL;
    wlc_info_t *wlc = NULL;
    uint ioctl_len = 0;
    uint16 af_len;
    int chnl = 0;

    if (!osif || !val) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    
    wlc = osif->wlc;
    if (!wlc) {
        TD_EM_DBG_PARAM_ERR("wlc is null ptr!\n");
        return -1;
    }

    ioctl_buf = (char *)MALLOC(wlc->osh, WLC_IOCTL_MAXLEN);
    if (!ioctl_buf) {
        TD_EM_DBG_PARAM_ERR("malloc fail");
        return -1;
    }

    memset(ioctl_buf, 0, WLC_IOCTL_MAXLEN);
    strncpy(ioctl_buf, "actframe", WLC_IOCTL_MAXLEN);
    ioctl_len = strlen(ioctl_buf) + 1;
    pcur = ioctl_buf + ioctl_len;

    chnl = wf_chspec_ctlchan(WLC_BAND_PI_RADIO_CHANSPEC);
    
    TD_EM_DBG_TRACE("[LYH] - %s -- chnl %d \n", __FUNCTION__, chnl);
    /* Fill Action Frame's token */
    static_tocken = (static_tocken > BKT_MAX_BUF_128) ? 0 : static_tocken;

    /* Fill Action Frame's parameters */
    af_params = (wl_af_params_t *)pcur;
    af_params->channel = 0;
    af_params->dwell_time = 0;
    action_frame = &af_params->action_frame;
    action_frame->packetId = (uint32)(uintptr)action_frame;

    /*Fill STA MAC */
    memcpy(&action_frame->da, val->sta_mac, sizeof(action_frame->da));
    /*Fill Source BSSID */
    memcpy(&af_params->BSSID, &(osif->BSSID), sizeof(af_params->BSSID));

    /*Fill Action Frame's Length */
    af_len = DOT11_RMREQ_LEN + DOT11_RMREQ_BCN_LEN;
    action_frame->len = af_len;

    /*Fill Action Frame's Basic data */
    rmreq_ptr = (dot11_rmreq_t *)&action_frame->data[0];
    rmreq_ptr->category = DOT11_ACTION_CAT_RRM;
    rmreq_ptr->action = DOT11_RM_ACTION_RM_REQ;
    rmreq_ptr->token = ++static_tocken;
    TD_EM_DBG_TRACE("Action Frame Token[%d] For STA[%pM]\n", static_tocken, val->sta_mac);
    rmreq_ptr->reps = 0; //重复次数，0为不重复

    /*Fill Action Frame's data specific to Beacon Request Frame Type */
    rmreq_bcn_ptr = (dot11_rmreq_bcn_t *)&rmreq_ptr->data[0];
    rmreq_bcn_ptr->id = DOT11_MNG_MEASURE_REQUEST_ID;

    /*Fill Beacon Request Element's Length */
    rmreq_bcn_ptr->len = DOT11_RMREQ_BCN_LEN - TLV_HDR_LEN;
    rmreq_bcn_ptr->token = ++static_tocken;
    rmreq_bcn_ptr->mode = RM_REQUEST_MODE;
    rmreq_bcn_ptr->type = DOT11_MEASURE_TYPE_BEACON;
    rmreq_bcn_ptr->reg = td_em_get_cur_opclass(osif);
    rmreq_bcn_ptr->channel = chnl;
    rmreq_bcn_ptr->interval = RM_RANDOM_INTERVAL;
    rmreq_bcn_ptr->duration = RM_TIME;
    rmreq_bcn_ptr->bcn_mode = DOT11_RMREQ_BCN_ACTIVE;

    /*Fill Target BSSID */
    memset(&rmreq_bcn_ptr->bssid.octet, 0xFF, ETHER_ADDR_LEN);
    TD_EM_DBG_TRACE(" BSSID  %pM \n", &osif->BSSID);
    TD_EM_DBG_TRACE(" SSID len %d \n", val->ssid_len);
    /* Append SSID optional subelement data */
    if (val->ssid_len) {
        /* Append SSID TLV to the frame */
        action_frame->data[af_len] = DOT11_MNG_SSID_ID;
        action_frame->data[af_len + 1] = val->ssid_len;
        memcpy(&action_frame->data[af_len + TLV_HDR_LEN], val->ssid, val->ssid_len);
        /* Update Action Frame's Length - AGAIN */
        af_len += val->ssid_len + TLV_HDR_LEN;
        action_frame->len = af_len;
        /* Update Beacon Request Element's Length - AGAIN */
        rmreq_bcn_ptr->len += val->ssid_len + TLV_HDR_LEN;
        TD_EM_DBG_TRACE("SSID %s \n", val->ssid);
    }
    //通过Ioctl发送Beacon Req请求Action帧
    if (BCME_OK != wlc_ioctl(wlc, WLC_SET_VAR, (void *)ioctl_buf, WL_WIFI_AF_PARAMS_SIZE, osif->wlcif)) {
     TD_EM_DBG_PARAM_ERR("send actframe fail");
     MFREE(wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);
     return -1;
    }

    MFREE(wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);
    return 0;
}

