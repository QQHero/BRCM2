/******************************************************************************
          版权所有 (C), 2015-2020, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Liuke
  生成日期   : 2020年6月
  最近修改   :

  功能描述   : BCM数据结构访问及操作.
            本文件内定义的所有函数入参合法性由调用者保障. 
            对于较短小的函数, 为提高调用效率, 可在td_maps_bcm.h中以内联形式实现;
            在本文件中实现非static函数, 应该在td_maps_bcm.h中有对应的原型声明.
******************************************************************************/
#include <siwifi_defs.h>
          
#include "td_maps_sf.h"

#define MAX_NSS_NUM             2
#define MAX_HT_MCS_NUM          8
#define MAX_VHT_MCS_NUM         10
#define BW_20MHZ                0
#define BW_40MHZ                1
#define BW_80MHZ                2
#define BW_160MHZ               3

extern const uint8_t legrates_lut[];
static const uint32_t legrates_tbl[] = {10, 20, 55, 110,
    60, 90, 120, 180, 240, 360, 480, 540};
static const uint32_t vht_rate_tbl[4][10] = {
    {6500, 13000, 19500, 26000, 39000, 52000, 58500, 65000, 78000, 86500},
    {13500, 27000, 40500, 54000, 81000, 108000, 121500, 135000, 162000, 180000},
    {29300, 58500, 87800, 117000, 175500, 234000, 263300, 292500, 351000, 390000},
    {58500, 117000, 175500, 234000, 351000, 468000, 526500, 585000, 702000, 780000},
};          
          
#define SF_MAX_BUF_128     (128)
#define SF_RM_TIME         (100)
#define SF_STA_LOAD_FACTOR (125)
#define SF_BYTE_LEN        (8)
#define SF_KBPS_UNIT       (1000)
#define SF_LOAD_SAMPLE_CNT 5         
          
void vendor_iterate_sta(vendor_vap_t* bsscfg, 
                        void(*cb)(vendor_sta_t*, vendor_vap_t*, unsigned long),
                        unsigned long data)
{
    vendor_sta_t *sta = NULL;

    if (!bsscfg) {
        return;
    }

    spin_lock_bh(&bsscfg->siwifi_hw->cb_lock);
    list_for_each_entry(sta, &bsscfg->ap.sta_list, list) {
      if (sta && sta->associated) {
          cb(sta, bsscfg, data);
      }
    }
    spin_unlock_bh(&bsscfg->siwifi_hw->cb_lock);
}

extern int siwifi_freq_to_idx(struct siwifi_hw *siwifi_hw, int freq);
/* 获取某radio的信道占用率, vaps: radio下的所有vap, vap_num: radio下的vap数量 */
unsigned char vendor_get_channel_load(vendor_vap_t** bsscfgs, int vap_num)
{
    int i;
    vendor_vap_t *bsscfg = NULL;
    uint16_t utilization = 0, survey_idx = 0;
  
    if (vap_num < 1 || !bsscfgs) {
        return 0;
    }

    for (i = 0; i < vap_num; i++) {
        bsscfg = bsscfgs[i];
        if (bsscfg) {
            break;
        }
    }

    if (bsscfg->ap_settings && bsscfg->ap_settings->chandef.chan 
        && bsscfg->siwifi_hw && bsscfg->siwifi_hw->survey) {
        survey_idx = siwifi_freq_to_idx(bsscfg->siwifi_hw, bsscfg->ap_settings->chandef.chan->center_freq);
        if (bsscfg->siwifi_hw->survey[survey_idx].chan_time_ms == 0) {
            utilization = 0;
        } else {
            utilization = (bsscfg->siwifi_hw->survey[survey_idx].chan_time_busy_ms * 1000) / bsscfg->siwifi_hw->survey[survey_idx].chan_time_ms;
        }
        
        return (utilization / 10);
    }
    return 0;
}

/* 客户端支持几条流 */
unsigned char vendor_get_sta_nss(vendor_sta_t *scb)
{
    return scb->stats.data_rx_nss + 1;
}


/* 对sta发送BSS Transition Request, bss: 目标BSS, channel: bss的工作信道 */
int vendor_bss_trans(vendor_vap_t *bsscfg,
                    unsigned char *sta, 
                    unsigned char *bss, 
                    unsigned char channel,
                    unsigned char op_class,
                    unsigned char phy_type)
{
    int ret = 0;
    return ret;
}

static unsigned int sf_sqca_mhz_to_ieee(unsigned int freq)
{
    if (freq < 2484) {
        return (freq - 2407) / 5;
    } else if (freq == 2484) {
        return 14;
    } else if (freq < 5180) {
        goto unknow_channel;
    } else if (freq <= 5825) {
        return (freq - 5000) / 5;
    }

unknow_channel:
    maps_err("unknow channel %d", freq);
    return 0;
}

unsigned char vendor_get_channel(vendor_vap_t* bsscfg)
{
    if (bsscfg->ap_settings == NULL) {
        maps_err("NULL pointer");
        return 0;
    }

    return sf_sqca_mhz_to_ieee(bsscfg->ap_settings->chandef.chan->center_freq);
}

static u_int8_t sf_average_load(u_int8_t *array, size_t len)
{
    unsigned int i = 0;
    unsigned int load_cnt = 0;
    int load_sum = 0;

    if (!array) {
        maps_err("NULL pointer");
        return 0;
    }
    
    for (i = 0; i < len; i++) {
        if (!array[i]) {
            continue;
        }

        load_cnt++;
        load_sum += array[i];
    }

    if (!load_cnt) {
        return 0;
    }

    return load_sum / load_cnt;
}

static uint32_t calculate_leg_rate(uint8_t legrate)
{
    int rate_idx;
    uint32_t rate_kbps = 0;
    if (legrate > 15) {
        return rate_kbps;
    }
    rate_idx = legrates_lut[legrate];
    if (rate_idx < 0 || rate_idx > 11) {
        return rate_kbps;
    }
    return legrates_tbl[rate_idx];
}


static uint32_t calculate_ht_vht_rate(uint8_t bw, uint8_t mcs, uint8_t shortgi, uint8_t nss)
{
    uint32_t rate_kbps = 0;
    if (bw > BW_80MHZ || mcs >= MAX_VHT_MCS_NUM) {
        return rate_kbps;
    }
    rate_kbps = vht_rate_tbl[bw][mcs] * (nss + 1);
    if (shortgi) {
        rate_kbps = (rate_kbps / 9) * 10;
    }
    return (rate_kbps + 49) / 100;
}

static uint32_t vendor_fill_rx_data(struct siwifi_sta *sta)
{
    uint8_t nss = 0, mcs = 0, sgi = 0, bw = 0;
    uint32_t rx_rate = 0; 

    if (sta->stats.format_mod == FORMATMOD_NON_HT || sta->stats.format_mod == FORMATMOD_NON_HT_DUP_OFDM) {
        rx_rate = calculate_leg_rate(sta->stats.leg_rate & 0xF);
    } else {
        sgi = sta->stats.short_gi;
        mcs = sta->stats.data_rx_mcs;
        nss = sta->stats.data_rx_nss + 1;
        switch (sta->stats.data_rx_bw)
        {
            case 0:
                bw = BW_20MHZ;
                break;
            case 1:
                bw = BW_40MHZ;
                break;
            case 2:
                bw = BW_80MHZ;
                break;
            default:
                return 0;
        }
        rx_rate = calculate_ht_vht_rate(bw, mcs, sgi, nss);
    }
    return rx_rate;
}

static void sf_get_sta_statistics(vendor_sta_t *vsta, adapt_sta_t *asta)
{
    u_int32_t rx_rate = 0; 
    u_int32_t tx_rate = 0;
    u_int32_t load = 0;
    u_int32_t deltaj = 0;
    u_int32_t tmp = 0;
    u_int64_t old_rx_bytes = 0;
    u_int64_t old_tx_bytes = 0;

    if (!vsta || ! asta) {
        maps_err("NULL pointer");
        return;
    }
    if (time_before(jiffies, asta->update_time + HZ)) {
        /* updated just now, don't calculate again */
        return;
    }
    
    old_rx_bytes = asta->rx_bytes;
    old_tx_bytes = asta->tx_bytes;
    asta->rx_bytes = vsta->stats.rx_bytes;
    asta->tx_bytes = vsta->stats.tx_bytes;
    deltaj = jiffies - asta->update_time;
    deltaj = (0 == deltaj) ? 1 : deltaj;

    if (!asta->update_time) {
        /* no record, can't calculate */
        asta->update_time = jiffies;
        return;
    }
    asta->update_time = jiffies;

    asta->throughput = asta->rx_bytes + asta->tx_bytes - old_rx_bytes - old_tx_bytes;
    asta->throughput = asta->throughput / (deltaj / HZ);
    load = 0;
    
    tx_rate = vsta->stats.last_tx_rate;
    rx_rate = vendor_fill_rx_data(vsta);
    if (rx_rate) {
        tmp = asta->rx_bytes - old_rx_bytes; /* u_int64_t to u_int32_t */ /* 空口占用率=流量/时间/速率 */
        load += tmp / (SF_KBPS_UNIT / 100) / deltaj * HZ * SF_BYTE_LEN / rx_rate; /* 100表示百分数 */
    }
    if (tx_rate) {
        tmp = asta->tx_bytes - old_tx_bytes; /* u_int64_t to u_int32_t */
        load += tmp / (SF_KBPS_UNIT / 100) / deltaj * HZ * SF_BYTE_LEN / tx_rate; /* 100表示百分数 */
    }

    load = load * SF_STA_LOAD_FACTOR / 100; /* 100% */
    asta->load_samples[asta->load_sample_idx++ % SF_LOAD_SAMPLE_CNT] = load;
    asta->load = sf_average_load(asta->load_samples, ARRAY_SIZE(asta->load_samples));
    if (asta->load > 100) { 
        asta->load = 100;
    }

  return;
}

/* 获取客户端占用的空口时间, 范围: 0%~100% */
unsigned char vendor_get_sta_load(vendor_sta_t *scb, 
                                adapt_sta_t *asta)
{
    sf_get_sta_statistics(scb, asta);
    return asta->load;
}

/* 获取客户端的流量, 单位: Byte/S */
unsigned int vendor_get_sta_flow(vendor_sta_t *scb,
                              adapt_sta_t *asta)
{
    sf_get_sta_statistics(scb, asta);
    return asta->throughput;
}

/* unsigned char vendor_get_op_class(vendor_vap_t *vap)
{
    return wlc_get_regclass(vap->wlc->cmi, 
        wf_chspec_ctlchspec(vap->current_bss->chanspec));
}*/

void vendor_fill_beacon_rep(vendor_sta_t *vsta, 
                          vendor_vap_t *vap, 
                          maps_msg_beacon_rep_t* msg)
{
    return;
}

int vendor_beacon_req(vendor_vap_t *vap, 
                    unsigned char *sta,
                    char *ssid, 
                    unsigned char channel, 
                    unsigned char op_class)
{
    return false;
}



