/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Sunjiajun
  生成日期   : 2019年12月
  最近修改   :

  功能描述   : RTL数据结构访问及操作.
            本文件内定义的所有函数入参合法性由调用者保障. 
            对于较短小的函数, 为提高调用效率, 可在td_maps_rtl.h中以内联形式实现;
            在本文件中实现非static函数, 应该在td_maps_rtl.h中有对应的原型声明.
******************************************************************************/
#include "td_maps_rtl.h"

//开启HADP_11V之后不需要再从驱动发送11v的btm request报文
#ifdef HAPD_11V

#else
/* for BSS Transition */
#include "8192cd_11v.h"
#endif

/* for Beacon Request */
#include "8192cd_headers.h"

void vendor_iterate_sta(vendor_vap_t* vap, 
                        void(*cb)(vendor_sta_t*, vendor_vap_t*, unsigned long),
                        unsigned long data)
{
    struct list_head *plist, *phead;
    vendor_sta_t *vsta;
    vendor_vap_t* priv;
#ifdef SMP_SYNC
    unsigned long flags;
#endif

    phead = &vap->asoc_list;
    plist = phead;
    /* SMP_LOCK_ASOC_LIST宏中使用priv */
    priv = vap;

    /*
     * rtk 1500(8197H+8832br)方案下 会定义 CONFIG_RTL8832BR 这个宏
     * 而且2.4G 和 5G 驱动的编译都有这个宏定义,所以用 CONFIG_RTL8832BR
     * 这个宏来区分 8197H+8832br  和其他 rtk的方案
    */
#ifdef CONFIG_RTL8832BR
    if(vap->vap_type != RTK_VAP_2G) {
        return ;
    }
#endif
    if (RTL_HCI_PCIE == vap->hci_type) {
        SMP_LOCK_ASOC_LIST(flags);
    }

    while ((plist = asoc_list_get_next(vap, plist)) != phead) {
        vsta = list_entry(plist, vendor_sta_t, asoc_list);
        if (RTL_HCI_PCIE == vap->hci_type) {
            SMP_UNLOCK_ASOC_LIST(flags);
        }
        cb(vsta, vap, data);
        if (RTL_HCI_PCIE == vap->hci_type) {
            SMP_LOCK_ASOC_LIST(flags);
        }
    }

    if (RTL_HCI_PCIE == vap->hci_type) {
        SMP_UNLOCK_ASOC_LIST(flags);
    }

    return;
}


/* 获取某radio的信道占用率, vaps: radio下的所有vap, vap_num: radio下的vap数量 */
unsigned char vendor_get_channel_load(vendor_vap_t** vaps, int vap_num)
{
    int load = 0, i = 0, utilization;
    vendor_vap_t* vap = vaps[0];

    if (vap_num < 1 || !vap) {
        return 0;
    }
    /*
     * rtk 1500(8197H+8832br)方案下 会定义 CONFIG_RTL8832BR 这个宏
     * 而且2.4G 和 5G 驱动的编译都有这个宏定义,所以用 CONFIG_RTL8832BR
     * 这个宏来区分 8197H+8832br  和其他 rtk的方案
    */
#ifdef CONFIG_RTL8832BR
    utilization = -1;
    for(i = 0; i < vap_num; i++) {
        vap = vaps[i];
        if (!vap) {
            break;
        }
        /*
         * rtk 1500(8197H+8832br)方案下 vap 2.4G和5G 是两种不同的结构体
         * 这里 只有vap->vap_type 类型是 RTK_VAP_2G 也就是说 
         * vap =====  struct rtl8192cd_priv才处理
        */
        if(vap->vap_type != RTK_VAP_2G) {
            continue;
        }
        if( -1 == utilization) {
            utilization = vap->ext_stats.ch_utilization;
        }
        
        load += vap->ext_stats.tx_time;
    }
#else // 其他rtk(非8197H+8832br)方案 还是按照原来的方式处理
    utilization = vap->ext_stats.ch_utilization;
    for (i = 0; i < vap_num; i++) {
        vap = vaps[i];
        if (!vap) {
            break;
        }
        load += vap->ext_stats.tx_time;
    }
#endif
    /* convert to percentage, 0% ~ 100% */
    load = load / 10;
    load += utilization * 100 / 256;
    return (load > 100) ? 100 : load;
}

/* 客户端支持几条流 */
unsigned char vendor_get_sta_nss(vendor_sta_t *vsta)
{
    u_int8_t *support_mcs;/* 客户端流数 */

    if (vsta->ht_cap_len < sizeof(struct ht_cap_elmt)) {
        return 1;
    }

    support_mcs = vsta->ht_cap_buf.support_mcs;

    if (support_mcs[3]) {
        return 4;
    } else if (support_mcs[2]) {
        return 3;
    } else if (support_mcs[1]) {
        return 2;
    } else {
        return 1;
    }
}

/* 将驱动收到的Beacon Report 转为 maps_msg_beacon_rep_t 格式 */
void vendor_fill_beacon_rep(vendor_sta_t *vsta, 
                            vendor_vap_t *vap, 
                            maps_msg_beacon_rep_t* msg)
{
    int i;
    maps_beacon_rep_t *rep;
    struct dot11k_beacon_measurement_report_info *info;

    for (i = 0; i < vsta->rm.beacon_report_num && i < MAPS_MAX_BEACON_REP; i++) {
        rep = &msg->reports[i];
        info = &vsta->rm.beacon_report[i].info;
        rep->op_class = info->op_class;
        rep->channel = info->channel;
        rep->rcpi = (int8_t)info->RCPI;
        rep->rsni = (int8_t)info->RSNI;
        memcpy(rep->bssid, info->bssid, MAC_ADDR_LEN);
    }

    msg->count = vsta->rm.beacon_report_num;
    return;
}

/* 对sta发送BSS Transition Request, bss: 目标BSS, channel: bss的工作信道 */
int vendor_bss_trans(vendor_vap_t *vap,
                    unsigned char *sta, 
                    unsigned char *bss, 
                    unsigned char channel,
                    unsigned char op_class,
                    unsigned char phy_type)
{
//开启HADP_11V之后不需要再从驱动发送11v的btm request报文
#ifdef HAPD_11V
    return 0;
#else
    /**
     * 需要清空rm_neighbor_bitmask, 并将bssTransPara.FromUser设为1, 
     * 否则驱动会从rm_neighbor_report[]中读取参数, 而不是bssTransPara.
    */
    memset(vap->rm_neighbor_bitmask, 0, sizeof(vap->rm_neighbor_bitmask));
    vap->bssTransPara.FromUser = 1;

    memcpy(vap->bssTransPara.bssid_mac, bss, MAC_ADDR_LEN);
    vap->bssTransPara.channel = channel;
    return issue_BSS_Trans_Req(vap, sta, 0, NULL, 0);
#endif
}

/* 对sta发送Beacon Request, channel: 请求扫描的信道 */
int vendor_beacon_req(vendor_vap_t *vap, 
                    unsigned char *sta,
                    char *ssid, 
                    unsigned char channel, 
                    unsigned char op_class)
{
//开启HADP_11K之后不需要再从驱动发送11k的Beacon Request报文
#ifdef HAPD_11K
    return 0;
#else
    struct dot11k_beacon_measurement_req req;

    memset(&req, 0, sizeof(req));
    snprintf(req.ssid, sizeof(req.ssid), "%s", ssid);
    req.channel = channel;
    /* 平均每个信道等待100ms */
    req.measure_duration = 100;
    req.mode = BEACON_MODE_ACTIVE;
    /* 只需要rcpi即可, 不需要返回IE */
    req.report_detail = 0;
    req.op_class = op_class;

    return rm_beacon_measurement_request(vap, sta, &req);
#endif
}

