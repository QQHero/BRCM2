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
#include "td_maps_bcm.h"

#include <wlc_iocv.h>
#include <wlc_lq.h>
#define BCM_MAX_BUF_128     (128)
#define BCM_RM_TIME         (100)
#define BCM_STA_LOAD_FACTOR (125)
#define BCM_BYTE_LEN        (8)
#define BCM_KBPS_UNIT       (1000)

void vendor_iterate_sta(vendor_vap_t* bsscfg, 
                        void(*cb)(vendor_sta_t*, vendor_vap_t*, unsigned long),
                        unsigned long data)
{
    struct scb_iter scbiter;
    struct scb *scb;

    if (NULL == bsscfg->wlc->scbstate) {
        return;
    }

    FOREACH_BSS_SCB(bsscfg->wlc->scbstate, &scbiter, bsscfg, scb){
        if (SCB_ASSOCIATED(scb)) {
            cb(scb, bsscfg, data);
        }
    }
    
    return;
}

/* 获取某radio的信道占用率, vaps: radio下的所有vap, vap_num: radio下的vap数量 */
unsigned char vendor_get_channel_load(vendor_vap_t** bsscfgs, int vap_num)
{
    vendor_vap_t* bsscfg = NULL;
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
    if(bsscfg){
        chan_stat = wlc_lq_chanspec_to_chanim_stats(bsscfg->wlc->chanim_info,
                bsscfg->current_bss->chanspec);

        if (chan_stat) {
            free = chan_stat->chan_idle;
        }
    }

    return (100 - free);
}

/* 客户端支持几条流 */
unsigned char vendor_get_sta_nss(vendor_sta_t *scb)
{
   return wlc_ratespec_nss(scb->scb_stats.tx_rate);    
}

/* 将驱动收到的Beacon Report 转为 maps_msg_beacon_rep_t 格式 */
void vendor_fill_beacon_rep(vendor_sta_t * scb, 
                            vendor_vap_t *vap, 
                            maps_msg_beacon_rep_t* msg)
{
    int i = 0;
    maps_beacon_rep_t *rep;
    
    if (!scb || !vap || !msg) {
        maps_err("NULL pointer");
        return;
    }

    for (i = 0; i < scb->beacon_rep.beacon_report_num && i < MAPS_MAX_BEACON_REP; i++) {
        rep = &msg->reports[i];
        rep->op_class = scb->beacon_rep.beacon_report[i].op_class;
        rep->channel = scb->beacon_rep.beacon_report[i].channel;
        rep->rcpi = (int8_t)scb->beacon_rep.beacon_report[i].RCPI;
        rep->rsni = (int8_t)scb->beacon_rep.beacon_report[i].RSNI;
        memcpy(rep->bssid, &scb->beacon_rep.beacon_report[i].bssid, MAC_ADDR_LEN);
        maps_dbg("[Beacon] BSS[%pM]-RCPI:%d Chanl:%d.", rep->bssid, scb->beacon_rep.beacon_report[i].RCPI, rep->channel);
    }

    msg->count = scb->beacon_rep.beacon_report_num;
    
    return;
}

static int bcm_wlc_ioval_set(vendor_vap_t *bsscfg, char *ioname, char *buf, int len)
{
    char *ioctl_buf = NULL;
    int ioctl_len = 0;

    if (!bsscfg || !ioname || !buf) {
        maps_err("null pointer!");
        return -1;
    }

    if (strlen(ioname) + len > WLC_IOCTL_MAXLEN) {
        maps_err("overflow");
        return -1;
    }

    ioctl_buf = (char *)MALLOC(bsscfg->wlc->osh, WLC_IOCTL_MAXLEN);
    if(!ioctl_buf){
        maps_err("malloc fail");
        return -1;
    }

    memset(ioctl_buf, 0, WLC_IOCTL_MAXLEN);
    strncpy(ioctl_buf, ioname, strlen(ioname));
    ioctl_len = strlen(ioname) + 1;

    memcpy(ioctl_buf + ioctl_len, buf, len);
    if (wlc_ioctl(bsscfg->wlc, WLC_SET_VAR, (void *)ioctl_buf,
        WLC_IOCTL_MAXLEN, bsscfg->wlcif)) {
        maps_err("wl%d ioval %s fail", bsscfg->wlc->pub->unit, ioname);
        MFREE(bsscfg->wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);
        return -1;
    }

    MFREE(bsscfg->wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);
    return 0;
}

static int bcm_btm_actframe(vendor_vap_t *bsscfg,
                    unsigned char *sta, 
                    unsigned char *bss, 
                    unsigned char channel,
                    unsigned char op_class,
                    unsigned char phy_type,
                    unsigned char bss_token)
{
    wl_af_params_t *af_params;
    wl_action_frame_t *action_frame;
    dot11_bsstrans_req_t *transreq;
    dot11_neighbor_rep_ie_t *nbr_ie;
    dot11_wide_bw_chan_ie_t *wbc_ie;
    dot11_ngbr_bsstrans_pref_se_t *nbr_pref;
    char *ioctl_buf = NULL;
    char *pcur = NULL;
    int ioctl_len = 0;

    ioctl_buf = (char *)MALLOC(bsscfg->wlc->osh, WLC_IOCTL_MAXLEN);
    if(!ioctl_buf){
        maps_err("malloc fail");
        return -1;
    }

    maps_info("wl%d send BTM request for %pM to bss %pM.", bsscfg->wlc->pub->unit, sta, bss);

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
   
    action_frame->len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
        TLV_HDR_LEN + DOT11_BSSTRANS_REQ_LEN +
        TLV_HDR_LEN + DOT11_NGBR_BSSTRANS_PREF_SE_LEN +
        TLV_HDR_LEN + DOT11_WIDE_BW_IE_LEN;

    ioctl_len += action_frame->len;

    transreq = (dot11_bsstrans_req_t *)&action_frame->data[0];
    transreq->category = DOT11_ACTION_CAT_WNM;
    transreq->action = DOT11_WNM_ACTION_BSSTRANS_REQ;

    transreq->token = bss_token;
    transreq->reqmode = DOT11_BSSTRANS_REQMODE_PREF_LIST_INCL 
                        | DOT11_BSSTRANS_REQMODE_ABRIDGED;
    transreq->disassoc_tmr = 0x0000;
    transreq->validity_intrvl = 100;

    nbr_ie = (dot11_neighbor_rep_ie_t *)&transreq->data[0];
    nbr_ie->id = DOT11_MNG_NEIGHBOR_REP_ID;
    nbr_ie->len = DOT11_NEIGHBOR_REP_IE_FIXED_LEN +
        DOT11_NGBR_BSSTRANS_PREF_SE_IE_LEN +
        DOT11_WIDE_BW_IE_LEN + TLV_HDR_LEN;
    
    memcpy(&nbr_ie->bssid, bss, ETHER_ADDR_LEN);
    nbr_ie->bssid_info = 0x00000000;
    nbr_ie->reg = op_class;
    nbr_ie->channel = channel;
    nbr_ie->phytype = phy_type;

    nbr_pref = (dot11_ngbr_bsstrans_pref_se_t *)&nbr_ie->data[0];
    nbr_pref->sub_id = DOT11_NGBR_BSSTRANS_PREF_SE_ID;
    nbr_pref->len = DOT11_NGBR_BSSTRANS_PREF_SE_LEN;
    nbr_pref->preference = DOT11_NGBR_BSSTRANS_PREF_SE_HIGHEST;

    pcur = (char *)nbr_pref + DOT11_NGBR_BSSTRANS_PREF_SE_IE_LEN;
    wbc_ie = (dot11_wide_bw_chan_ie_t *)pcur;
    wbc_ie->id = DOT11_NGBR_WIDE_BW_CHAN_SE_ID;
    wbc_ie->len = DOT11_WIDE_BW_IE_LEN;
    wbc_ie->channel_width = 0;
    wbc_ie->center_frequency_segment_0 = channel;
    wbc_ie->center_frequency_segment_1 = 0;

    if (wlc_ioctl(bsscfg->wlc, WLC_SET_VAR, (void *)ioctl_buf, 
        WL_WIFI_AF_PARAMS_SIZE, bsscfg->wlcif)) {
        
        maps_err("actframe fail");
        MFREE(bsscfg->wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);
        return -1;
    }

    MFREE(bsscfg->wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);

    return 0;
}


/* 对sta发送BSS Transition Request, bss: 目标BSS, channel: bss的工作信道 */
int vendor_bss_trans(vendor_vap_t *bsscfg,
                    unsigned char *sta, 
                    unsigned char *bss, 
                    unsigned char channel,
                    unsigned char op_class,
                    unsigned char phy_type)
{
    nbr_rpt_elem_t btq_nbr_elem;        // 邻居表
    wl_bsstrans_req_t bsstrans_req;     // BTM请求
    static int bss_token = 0;
    int ret = -1;

    memset(&btq_nbr_elem, 0, sizeof(btq_nbr_elem));
    btq_nbr_elem.version = WL_RRM_NBR_RPT_VER;
    memcpy(&btq_nbr_elem.bssid, bss, ETHER_ADDR_LEN);
    /* bssid_info直接设置DOT11_NGBR_BI_HT|DOT11_NGBR_BI_VHT. ToDo：网络模式变化适配 */
    btq_nbr_elem.bssid_info = 0x180F;
    btq_nbr_elem.channel = channel;

#if BRCM_CHIP == 6756 || BRCM_CHIP == 63178
    btq_nbr_elem.chanspec = CH20MHZ_CHSPEC(channel, WL_CHANNEL_2G5G_BAND(channel));
#else
    btq_nbr_elem.chanspec = CH20MHZ_CHSPEC(channel);
#endif
    btq_nbr_elem.reg = op_class;
    btq_nbr_elem.phytype = 9;  // vht
    btq_nbr_elem.bss_trans_preference = 0xFF;

    if (0 == ++bss_token) {
        bss_token = 1;
    }

    /* 更新邻居表 */
    if (memcmp(&bsscfg->BSSID, bss, ETHER_ADDR_LEN)){
        /* clean neighbors first*/
        bcm_wlc_ioval_set(bsscfg, "wnm_btq_nbr_del", "\x00\x00\x00\x00\x00\x00", ETHER_ADDR_LEN);
        bcm_wlc_ioval_set(bsscfg, "wnm_btq_nbr_add", (char *)&btq_nbr_elem, 
            sizeof(btq_nbr_elem));
    }

    maps_info("wl%d update neighbor %pM \n", bsscfg->wlc->pub->unit, bss);

    /* 发送BTM请求 */
    memset(&bsstrans_req, 0, sizeof(bsstrans_req));
    bsstrans_req.reqmode = DOT11_BSSTRANS_REQMODE_PREF_LIST_INCL 
                        | DOT11_BSSTRANS_REQMODE_ABRIDGED;
    bsstrans_req.tbtt = 0;
    memcpy(&bsstrans_req.sta_mac, sta, ETHER_ADDR_LEN);
    bsstrans_req.retry_delay = 0;
    bsstrans_req.reason = 0;
    bsstrans_req.token = bss_token;

    /* 如果BTM失败，尝试传统actframe */
    if (0 == bcm_wlc_ioval_set(bsscfg, "wnm_bsstrans_req", (char *)&bsstrans_req, 
                                sizeof(bsstrans_req))) {
        ret = 0;
    } else if (0 == bcm_btm_actframe(bsscfg, sta, bss, channel, op_class, 
                                phy_type, bss_token)) {
        ret = 0;
    } else {
    }

    /* clean neighbors at last */
    if (-1 == bcm_wlc_ioval_set(bsscfg, "wnm_btq_nbr_del", "\x00\x00\x00\x00\x00\x00",
                                 ETHER_ADDR_LEN)) {
        ret = -1;
    }

    if (-1 == ret) {
        maps_err("wl%d bss trans %pM fail", bsscfg->wlc->pub->unit, sta);
    }
    
    return ret;
}

/* 对sta发送Beacon Request, channel: 请求扫描的信道 */
int vendor_beacon_req(vendor_vap_t *vap, 
                    unsigned char *sta,
                    char *ssid, 
                    unsigned char channel, 
                    unsigned char op_class)
{
    wl_action_frame_t * action_frame;
    wl_af_params_t * af_params;
    char *ioctl_buf = NULL;
    uint16 af_len;
    dot11_rmreq_t *rmreq_ptr;
    dot11_rmreq_bcn_t *rmreq_bcn_ptr;
    static int static_tocken = 0;
    int ssid_len = 0;
    char *pcur = NULL;
    uint ioctl_len = 0;

    ioctl_buf = (char *)MALLOC(vap->wlc->osh, WLC_IOCTL_MAXLEN);;
    if(!ioctl_buf){
        maps_err("malloc fail");
        return -1;
    }
    
    maps_info("wl%d send Beacon request for %pM to ssid %s.", vap->wlc->pub->unit, sta, ssid);

    memset(ioctl_buf, 0, WLC_IOCTL_MAXLEN);
    strncpy(ioctl_buf, "actframe", WLC_IOCTL_MAXLEN);
    ioctl_len = strlen(ioctl_buf) + 1;
    pcur = ioctl_buf + ioctl_len;

    /* Update Action Frame's token */
    static_tocken = (static_tocken > BCM_MAX_BUF_128) ? 0 : static_tocken;

    /* Update Action Frame's parameters */
    af_params = (wl_af_params_t *)pcur;
    af_params->channel = 0;
    af_params->dwell_time = 0;
    action_frame = &af_params->action_frame;
    action_frame->packetId = (uint32)(uintptr)action_frame;

    /* Update STA MAC */
    memcpy(&action_frame->da, sta, sizeof(action_frame->da));

    /* Update Source BSSID */
    memcpy(&af_params->BSSID, &(vap->BSSID), sizeof(af_params->BSSID));

    /* Update Action Frame's Length */
    af_len = DOT11_RMREQ_LEN + DOT11_RMREQ_BCN_LEN;
    action_frame->len = af_len;

    /* Update Action Frame's Basic data */
    rmreq_ptr = (dot11_rmreq_t *)&action_frame->data[0];
    rmreq_ptr->category = DOT11_ACTION_CAT_RRM;
    rmreq_ptr->action = DOT11_RM_ACTION_RM_REQ;
    rmreq_ptr->token = ++static_tocken;
    maps_info("Action Frame Token[%d] For STA[%pM]\n", static_tocken, sta);
    rmreq_ptr->reps = 0;
    
    /* Update Action Frame's data specific to Beacon Request Frame Type */
    rmreq_bcn_ptr = (dot11_rmreq_bcn_t *)&rmreq_ptr->data[0];
    rmreq_bcn_ptr->id = DOT11_MNG_MEASURE_REQUEST_ID;

    /* Update Beacon Request Element's Length */
    rmreq_bcn_ptr->len = DOT11_RMREQ_BCN_LEN - TLV_HDR_LEN;
    rmreq_bcn_ptr->token = ++static_tocken;

    maps_info("Beacon Frame Token[%d] For STA[%pM]\n", static_tocken, sta);
    rmreq_bcn_ptr->mode = 0;
    rmreq_bcn_ptr->type = DOT11_MEASURE_TYPE_BEACON;
    rmreq_bcn_ptr->reg = op_class;
    rmreq_bcn_ptr->channel = channel;
    rmreq_bcn_ptr->interval = 0x0000;
    rmreq_bcn_ptr->duration = BCM_RM_TIME; //时间单位:ms
    rmreq_bcn_ptr->bcn_mode = DOT11_RMREQ_BCN_ACTIVE;

    /* Update Target BSSID */
    //steerd的beacon测量请求不需要制定bssid,填充通配符即可
    memset(&rmreq_bcn_ptr->bssid.octet, 0xFF, ETHER_ADDR_LEN);
    
    ssid_len = strlen(ssid);
    maps_info("- %s -- SSID len %d \n", __FUNCTION__, strlen(ssid));
    /* Append SSID optional subelement data */
    if (ssid_len) {
        /* Append SSID TLV to the frame */
        action_frame->data[af_len] = 0;
        action_frame->data[af_len + 1] = ssid_len;
        memcpy(&action_frame->data[af_len + TLV_HDR_LEN], ssid, ssid_len);
        /* Update Action Frame's Length - AGAIN */
        af_len += ssid_len + TLV_HDR_LEN;
        action_frame->len = af_len;
        /* Update Beacon Request Element's Length - AGAIN */
        rmreq_bcn_ptr->len += ssid_len + TLV_HDR_LEN;
        
        maps_info("- %s -- SSID %s \n", __FUNCTION__, ssid);
    }

    if (wlc_ioctl(vap->wlc, WLC_SET_VAR, (void *)ioctl_buf, WL_WIFI_AF_PARAMS_SIZE, vap->wlcif)) {
        maps_err("actframe fail");
        MFREE(vap->wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);
        return -1;
    }

    MFREE(vap->wlc->osh, ioctl_buf, WLC_IOCTL_MAXLEN);

    return 0;
}

unsigned char vendor_get_channel(vendor_vap_t* bsscfg)
{
    return wf_chspec_ctlchan(bsscfg->wlc->chanspec);
}

static u_int8_t bcm_average_load(u_int8_t *array, size_t len)
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

static void bcm_get_sta_statistics(vendor_sta_t *vsta, adapt_sta_t *asta)
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
    asta->rx_bytes = vsta->scb_stats.rx_ucast_bytes + vsta->scb_stats.rx_mcast_bytes;
    asta->tx_bytes = vsta->scb_stats.tx_ucast_bytes + vsta->scb_stats.tx_mcast_bytes;
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
    
    tx_rate = vsta->scb_stats.tx_rate;
    rx_rate = vsta->scb_stats.rx_rate;
    if (rx_rate) {
        tmp = asta->rx_bytes - old_rx_bytes; /* u_int64_t to u_int32_t */ /* 空口占用率=流量/时间/速率 */
        load += tmp / (BCM_KBPS_UNIT / 100) / deltaj * HZ * BCM_BYTE_LEN / rx_rate; /* 100表示百分数 */
    }
    if (tx_rate) {
        tmp = asta->tx_bytes - old_tx_bytes; /* u_int64_t to u_int32_t */
        load += tmp / (BCM_KBPS_UNIT / 100) / deltaj * HZ * BCM_BYTE_LEN / tx_rate; /* 100表示百分数 */
    }

    load = load * BCM_STA_LOAD_FACTOR / 100; /* 100% */
    asta->load_samples[asta->load_sample_idx++ % BCM_LOAD_SAMPLE_CNT] = load;
    asta->load = bcm_average_load(asta->load_samples, ARRAY_SIZE(asta->load_samples));
    if (asta->load > 100) { /* 100% at most */
        asta->load = 100;
    }

    return;
}

/* 获取客户端占用的空口时间, 范围: 0%~100% */
unsigned char vendor_get_sta_load(vendor_sta_t *scb, 
                                adapt_sta_t *asta)
{
    bcm_get_sta_statistics(scb, asta);
    return asta->load;
}

/* 获取客户端的流量, 单位: Byte/S */
unsigned int vendor_get_sta_flow(vendor_sta_t *scb,
                                adapt_sta_t *asta)
{
    bcm_get_sta_statistics(scb, asta);
    return asta->throughput;
}

unsigned char vendor_get_op_class(vendor_vap_t *vap)
{
    return wlc_get_regclass(vap->wlc->cmi, 
        wf_chspec_ctlchspec(vap->current_bss->chanspec));
}

#ifdef TD_STEER_SUPPORT_6G
unsigned int vendor_get_frequency(vendor_vap_t* bsscfg)
{
    return wf_chanspec2mhz(bsscfg->wlc->chanspec);
}
#endif
