/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Sunjiajun
  生成日期   : 2019年12月
  最近修改   :

  功能描述   : QCA数据结构访问及操作.
            本文件内定义的所有函数入参合法性由调用者保障. 
            对于较短小的函数, 为提高调用效率, 可在td_maps_qca.h中以内联形式实现;
            在本文件中实现非static函数, 应该在td_maps_qca.h中有对应的原型声明.
******************************************************************************/
#include "td_maps_qca.h"
#include "ieee80211_api.h"
#include "ieee80211_rrm.h"
#include "ieee80211_wnm_proto.h"

#include <linux/kernel.h>
#include <linux/spinlock.h>

#define QCA_STA_LOAD_FACTOR 125

typedef struct qca_ch_load {
    u_int32_t clr_cnt; /* restore ieee80211_chan_stats::chan_clr_cnt */
    u_int32_t cycle_cnt; /* restore ieee80211_chan_stats::cycle_cnt */
    u_int8_t ch_load; /* channel utilization as a percentage, 0% ~ 100% */
} qca_ch_load_t;

typedef struct qca_sta_iter {
    vendor_vap_t* vap;
    void (*cb)(vendor_sta_t*, vendor_vap_t*, unsigned long);
    unsigned long data;
} qca_sta_iter_t;


static qca_ch_load_t s_ch_loads[MAPS_MAX_RADIO_INDEX + 1];

static void qca_sta_iter_cb(void *arg, vendor_sta_t *vsta)
{
    qca_sta_iter_t *piter = (qca_sta_iter_t*)arg;

    if (unlikely(!piter->cb)) {
        maps_err("NULL pointer");
        return;
    }

    if (!vendor_is_sta_neighbor(vsta)) {
        piter->cb(vsta, piter->vap, piter->data);
    }
    return;
}

void vendor_iterate_sta(vendor_vap_t* vap, 
                        void(*cb)(vendor_sta_t*, vendor_vap_t*, unsigned long),
                        unsigned long data)
{
    qca_sta_iter_t iter = {vap, cb, data};
    wlan_iterate_station_list(vap, qca_sta_iter_cb, &iter);
}

/* 对sta发送BSS Transition Request, bss: 目标BSS, channel: bss的工作信道 */
int vendor_bss_trans(vendor_vap_t *vap,
                    unsigned char *sta, 
                    unsigned char *bss, 
                    unsigned char channel,
                    unsigned char op_class,
                    unsigned char phy_type)
{
    static unsigned char s_token = 1;
    struct ieee80211_bstm_reqinfo_target req = {
        .dialogtoken = s_token++,
        .num_candidates = 1,
        .disassoc = 0,
        .disassoc_timer = 0
    };
    struct ieee80211_bstm_candidate *cand = &req.candidates[0];

    memcpy(cand->bssid, bss, MAC_ADDR_LEN);
    cand->channel_number = channel;
    cand->preference = 0xff;
    cand->op_class = op_class;
    cand->phy_type = phy_type;

    return wlan_send_bstmreq_target(vap, sta, &req);
}

/* 对sta发送Beacon Request, channel: 请求扫描的信道 */
int vendor_beacon_req(vendor_vap_t *vap, 
                    unsigned char *sta,
                    char *ssid, 
                    unsigned char channel, 
                    unsigned char op_class)
{
    struct ieee80211_rrm_beaconreq_info_s req = {
        .regclass = op_class,
        .channum = channel,
        .duration = 50 , /* Scan 50ms */
        .random_ivl = 0,
        .reqmode = 0, 
        .reqtype = 5, /* Beacon Request */
        .mode = 1, /* Active scan */
        .req_ssid = IEEE80211_BCNREQUEST_VALIDSSID_REQUESTED,
        .rep_cond = 0, /* Report to be issued after each measurement */
        .rep_thresh = 0, 
        .rep_detail = 1, /* All fixed length fields and any requested elements in the Request information element if present */
        .req_ie = 0,
        .lastind = 0,
        .num_chanrep = 0,
        .ssidlen = strlen(ssid)
    };

    memset(&req.bssid, 0xff, MAC_ADDR_LEN); /* broadcast */
    snprintf(req.ssid, sizeof(req.ssid), "%s", ssid);
    return wlan_send_beacon_measreq(vap, sta, &req);
}

/* 将驱动收到的Beacon Report 转为 maps_msg_beacon_rep_t 格式 */
void vendor_fill_beacon_rep(vendor_sta_t *vsta, 
                            vendor_vap_t *vap, 
                            maps_msg_beacon_rep_t* msg)
{
    maps_beacon_rep_t *rep;
    struct ieee80211_beacon_report *report;
    struct ieee80211_beacon_entry *bentry;
    struct ieee80211_beacon_report_table *btable = vap->rrm->beacon_table;
    int i = 0;

    memcpy(msg->sta, vsta->ni_macaddr, MAC_ADDR_LEN);
    msg->bss_index = vendor_get_bss_index(vap);

    spin_lock(&btable->lock);
    TAILQ_FOREACH(bentry, &(btable->entry), blist) {
        if (i >= MAPS_MAX_BEACON_REP) {
            break;
        }

        report = &bentry->report;
        rep = &msg->reports[i];
        rep->op_class = report->reg_class;
        rep->channel = report->ch_num;
        rep->rcpi = (int8_t)report->rcpi;
        rep->rsni = (int8_t)report->rsni;
        memcpy(rep->bssid, report->bssid, MAC_ADDR_LEN);
        i++;
    }
    spin_unlock(&btable->lock);
    msg->count = i;
    return;
}

/* 获取某radio的信道占用率, vaps: radio下的所有vap, vap_num: radio下的vap数量 */
unsigned char vendor_get_channel_load(vendor_vap_t** vaps, int vap_num)
{
    u_int8_t old_load;
    u_int32_t old_clr_cnt, old_cycle_cnt, tmp;
    struct ieee80211_chan_stats chan_stats;
    struct ieee80211com *ic;
    qca_ch_load_t *load;
    vendor_vap_t *vap = vaps[0];
    u_int8_t radio_index = vendor_get_radio_index(vap);

    if (!maps_valid_radio_index(radio_index)) {
        maps_err("invalid radio%d", radio_index);
        return 0;
    }

    load = &s_ch_loads[radio_index];
    old_load = load->ch_load;
    old_clr_cnt = load->clr_cnt;
    old_cycle_cnt = load->cycle_cnt;

    ic = vap->iv_ic;
    if (unlikely(!ic) || unlikely(!ic->ic_get_cur_chan_stats)) {
        maps_err("NULL pointer");
        return 0;
    }

    ic->ic_get_cur_chan_stats(ic, &chan_stats);
    maps_dump("RADIO%d, clr_cnt: %u, cycle_cnt: %u, old_clr_cnt: %u, "
            "old_cycle_cnt: %u, old_load: %u", radio_index, 
            chan_stats.chan_clr_cnt, chan_stats.cycle_cnt, 
            old_clr_cnt, old_cycle_cnt, old_load);

    load->clr_cnt = chan_stats.chan_clr_cnt;
    load->cycle_cnt = chan_stats.cycle_cnt;

    if (!old_clr_cnt && !old_cycle_cnt) {
        /* no record, can't calculate the channel utilization */
        return old_load;
    }

    if (chan_stats.chan_clr_cnt < old_clr_cnt || 
        chan_stats.cycle_cnt < old_cycle_cnt) {
        /* counters overflowed */
        return old_load;
    }

    /* to 100% */
    tmp = (chan_stats.cycle_cnt - old_cycle_cnt) / 100;
    tmp = tmp < 1 ? 1: tmp;
    load->ch_load = (chan_stats.chan_clr_cnt - old_clr_cnt) / tmp;
    load->ch_load = load->ch_load > 100 ? 100: load->ch_load;

    return load->ch_load;
}

static u_int32_t qca_average_rate(u_int32_t *array, size_t len)
{
    unsigned int i = 0;
    unsigned int rate_cnt = 0;
    u_int32_t rate_sum = 0;

    for (; i < len; i++) {
        if (!array[i]) {
            continue;
        }

        rate_cnt++;
        rate_sum += array[i];
    }

    if (!rate_cnt) {
        return 0;
    }

    return rate_sum / rate_cnt;
}

static u_int8_t qca_average_load(u_int8_t *array, size_t len)
{
    unsigned int i = 0;
    unsigned int load_cnt = 0;
    int load_sum = 0;

    for (; i < len; i++) {
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

static void qca_get_smooth_rate(vendor_sta_t *vsta, adapt_sta_t *asta,
                                u_int32_t *p_rx_rate, u_int32_t *p_tx_rate)
{
    u_int32_t cur_rx_rate, cur_tx_rate;
    struct ieee80211com *ic = vsta->ni_ic;

    if (unlikely(!ic) || unlikely(!ic->ic_node_getrate)) {
        maps_err("NULL pointer");
        *p_rx_rate = 0;
        *p_tx_rate = 0;
        return;
    }

    cur_rx_rate = ic->ic_node_getrate(vsta, IEEE80211_RATE_RX); /* in kbps */
    cur_tx_rate = ic->ic_node_getrate(vsta, IEEE80211_RATE_TX); /* in kbps */

    if (!cur_rx_rate || !cur_tx_rate) {
        maps_info("invalid rate of "MACSTR", rx_rate: %u, tx_rate: %u", 
                MAC2STR(vsta->ni_macaddr), cur_rx_rate, cur_tx_rate);
    } else {
        unsigned int idx = asta->rate_sample_idx++ % QCA_RATE_SAMPLE_CNT;
        asta->tx_rate_samples[idx] = cur_tx_rate;
        asta->rx_rate_samples[idx] = cur_rx_rate;
    }

    *p_tx_rate = qca_average_rate(asta->tx_rate_samples, 
                                ARRAY_SIZE(asta->tx_rate_samples));
    *p_rx_rate = qca_average_rate(asta->rx_rate_samples, 
                                ARRAY_SIZE(asta->rx_rate_samples));
    return;
}

static void qca_get_sta_statistics(vendor_sta_t *vsta, 
                                    adapt_sta_t *asta)
{
    u_int32_t rx_rate, tx_rate, deltaj, load, tmp;
    u_int64_t old_rx_bytes = asta->rx_bytes;
    u_int64_t old_tx_bytes = asta->tx_bytes;

    if (time_before(jiffies, asta->update_time + HZ)) {
        /* updated just now, don't calculate again */
        return;
    }

    asta->rx_bytes = vsta->ni_stats.ns_rx_bytes;
    asta->tx_bytes = vsta->ni_stats.ns_tx_bytes;
    deltaj = jiffies - asta->update_time;
    deltaj = (0 == deltaj) ? 1 : deltaj;

    if (!asta->update_time){
        /* no record, can't calculate */
        asta->update_time = jiffies;
        return;
    }
    asta->update_time = jiffies;

    asta->throughput = asta->rx_bytes + asta->tx_bytes - 
                        old_rx_bytes - old_tx_bytes;
    asta->throughput = asta->throughput / (deltaj / HZ);

    /* convert throughput to air time as a percentage */
    load = 0;
    qca_get_smooth_rate(vsta, asta, &rx_rate, &tx_rate);
    if (rx_rate) {
        tmp = asta->rx_bytes - old_rx_bytes; /* u_int64_t to u_int32_t */
        load += tmp / (1000 / 100) / deltaj * HZ * 8 / rx_rate;
    }
    if (tx_rate) {
        tmp = asta->tx_bytes - old_tx_bytes; /* u_int64_t to u_int32_t */
        load += tmp / (1000 / 100) / deltaj * HZ * 8 / tx_rate;
    }

    load = load * QCA_STA_LOAD_FACTOR / 100;
    asta->load_samples[asta->load_sample_idx++ % QCA_LOAD_SAMPLE_CNT] = load;
    asta->load = qca_average_load(asta->load_samples, ARRAY_SIZE(asta->load_samples));
    if (asta->load > 100) { /* 100% at most */
        asta->load = 100;
    }

    maps_dump("STA "MACSTR", deltaj: %u, HZ: %u, old_rx_bytes: %llu,"
            " old_tx_bytes: %llu, rx_bytes: %llu, tx_bytes: %llu, rx_rate:"
            " %u, tx_rate: %u, throughput: %u, load: %u, average load: %u", 
            MAC2STR(vsta->ni_macaddr), deltaj, HZ, old_rx_bytes,
            old_tx_bytes, asta->rx_bytes, asta->tx_bytes, rx_rate,
            tx_rate, asta->throughput, load, asta->load);

    return;
}

/* 获取客户端占用的空口时间, 范围: 0%~100% */
unsigned char vendor_get_sta_load(vendor_sta_t *vsta, 
                                adapt_sta_t *asta)
{
    qca_get_sta_statistics(vsta, asta);
    return asta->load;
}

/* 获取客户端的流量, 单位: Byte/S */
unsigned int vendor_get_sta_flow(vendor_sta_t *vsta,
                                adapt_sta_t *asta)
{
    qca_get_sta_statistics(vsta, asta);
    return asta->throughput;
}

void vendor_clean(void)
{
    memset(s_ch_loads, 0, sizeof(s_ch_loads));
    return ;
}

