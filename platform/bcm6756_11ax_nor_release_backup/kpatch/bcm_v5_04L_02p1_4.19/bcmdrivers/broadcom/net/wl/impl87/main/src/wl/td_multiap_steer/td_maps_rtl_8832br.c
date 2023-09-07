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
#include "td_maps_rtl_8832br.h"



/*****************************************************************************
 函 数 名  : vendor_iterate_sta
 功能描述  : sta迭代,参考 rtw_debug_ap.c中的 proc_get_sta_info函数
 输入参数  : vendor_vap_t* vap        
             void(*cb)(vendor_sta_t*  
             vendor_vap_t*            
             unsigned long)           
             unsigned long data       
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2022年3月8日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
void vendor_iterate_sta(vendor_vap_t* vap, 
                        void(*cb)(vendor_sta_t*, vendor_vap_t*, unsigned long),
                        unsigned long data)
{
    struct sta_priv *pstapriv = &vap->stapriv;
    _list   *plist, *phead;
    struct sta_info *psta;
    struct sta_info *pfirsta;
    int i = 0;
    struct mlme_priv *pmlmepriv = &(vap->mlmepriv);
    struct wlan_network *cur_network = &(pmlmepriv->cur_network);
    struct mlme_ext_priv *pmlmeext = &(vap->mlmeextpriv);
    struct mlme_ext_info *pmlmeinfo = &(pmlmeext->mlmext_info);
    unsigned char self_mac[18]  = {0}, bmc_mac[18] = {0}, mac[18] = {0};

    if(vap->vap_type != RTK_VAP_5G) {
        return ;
    }

    if(rtw_is_adapter_up(vap)) {
        if(MLME_IS_STA(vap) && !(pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)) {
            pfirsta = rtw_get_stainfo(pstapriv, vap->phl_role->mac_addr);
        } else {
            pfirsta = rtw_get_stainfo(pstapriv, cur_network->network.MacAddress);
        }

        if(NULL == pfirsta) {
            maps_err("NULL pointer:pfirsta");
            return ;
        }

        if(pfirsta->phl_sta) {
            snprintf(self_mac, sizeof(self_mac), MAC_FMT, MAC_ARG(pfirsta->phl_sta->mac_addr));
        } else {
            maps_err("NULL pointer:pfirsta->phl_sta");
            return ;
        }
        snprintf(bmc_mac, sizeof(bmc_mac), "%s", "ff:ff:ff:ff:ff:ff");

        _rtw_spinlock_bh(&pstapriv->sta_hash_lock);
        for (i = 0; i < NUM_STA; i++) {
            phead = &(pstapriv->sta_hash[i]);
            plist = get_next(phead);
            while ((rtw_end_of_queue_search(phead, plist)) == _FALSE) {
                psta = LIST_CONTAINOR(plist, struct sta_info, hash_list);
                plist = get_next(plist);
                memset(mac, 0, sizeof(mac));
                if(psta->phl_sta) {
                    snprintf(mac, sizeof(mac), MAC_FMT, MAC_ARG(psta->phl_sta->mac_addr));
                    if(strncmp(mac, self_mac, sizeof(mac)) && strncmp(mac, bmc_mac, sizeof(mac))){
                        cb(psta, vap, data);
                    }
                }
            }
        }
        _rtw_spinunlock_bh(&pstapriv->sta_hash_lock);
    }else {
        maps_err("The interface is not running");
    }
    return ;
}


unsigned char vendor_get_channel_load(vendor_vap_t** vaps, int vap_num)
{
    struct dvobj_priv *dvobj = NULL;
    struct rtw_env_report *env_rpt = NULL;
    vendor_vap_t* vap = NULL;
    int i = 0;
    for (i = 0; i < vap_num; i++) {
        vap = vaps[i];
        /*
         * 5G频段的vap 才会在该函数中统计
         */
        if (vap &&  RTK_VAP_5G == vap->vap_type) {
            break;
        } else {
            vap = NULL;
        }
    }
    if(NULL == vap) {
        return 0;
    }
    dvobj = adapter_to_dvobj(vap);
    env_rpt = &(dvobj->env_rpt);
    return env_rpt->nhm_cca_ratio;
}



/*****************************************************************************
 函 数 名  : vendor_get_sta_nss
 功能描述  : 客户端支持几条流
 输入参数  : vendor_sta_t *vsta  
 输出参数  : 无
 返 回 值  : unsigned
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
unsigned char vendor_get_sta_nss(vendor_sta_t *vsta)
{
    unsigned char rx = 0, tx = 0;
    tx = rtw_get_sta_tx_nss(vsta->padapter, vsta);
    rx = rtw_get_sta_rx_nss(vsta->padapter, vsta);
    return (tx > rx) ? tx : rx;
}


/*****************************************************************************
 函 数 名  : vendor_fill_beacon_rep
 功能描述  : 将驱动收到的Beacon Report 转为 maps_msg_beacon_rep_t
                 格式
 输入参数  : vendor_sta_t *vsta          
             vendor_vap_t *vap           
             maps_msg_beacon_rep_t* msg  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
void vendor_fill_beacon_rep(vendor_sta_t *vsta, 
                            vendor_vap_t *vap, 
                            maps_msg_beacon_rep_t* msg)
{
    int i;
    maps_beacon_rep_t *rep;
    struct dot11k_beacon_measurement_report_info *info;

    for (i = 0; i < vsta->rm_beacon_rpt_num && i < MAPS_MAX_BEACON_REP; i++) {
        rep = &msg->reports[i];
        info = &vsta->rm_beacon_rpt_list[i].info;
        rep->op_class = info->op_class;
        rep->channel = info->channel;
        rep->rcpi = (int8_t)info->RCPI;
        rep->rsni = (int8_t)info->RSNI;
        memcpy(rep->bssid, info->bssid, MAC_ADDR_LEN);
    }
    msg->count = vsta->rm_beacon_rpt_num;
    return;
}




/*****************************************************************************
 函 数 名  : vendor_bss_trans
 功能描述  : 对sta发送BSS Transition Request, bss: 目标BSS, channel:
                 bss的工作信道
 输入参数  : vendor_vap_t *vap       
             unsigned char *sta      
             unsigned char *bss      
             unsigned char channel   
             unsigned char op_class  
             unsigned char phy_type  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
int vendor_bss_trans(vendor_vap_t *vap,
                    unsigned char *sta, 
                    unsigned char *bss, 
                    unsigned char channel,
                    unsigned char op_class,
                    unsigned char phy_type)
{
    return 0;
}


/*****************************************************************************
 函 数 名  : vendor_beacon_req
 功能描述  : 对sta发送Beacon Request, channel: 请求扫描的信道
 输入参数  : vendor_vap_t *vap       
             unsigned char *sta      
             char *ssid              
             unsigned char channel   
             unsigned char op_class  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
int vendor_beacon_req(vendor_vap_t *vap, 
                    unsigned char *sta,
                    char *ssid, 
                    unsigned char channel, 
                    unsigned char op_class)
{
    return 0;
}



/*****************************************************************************
 函 数 名  : vendor_get_sta_load
 功能描述  : 获取客户端占用的空口时间, 范围: 0%~100%
 输入参数  : vendor_sta_t *vsta   
             adapt_sta_t *unused  
 输出参数  : 无
 返 回 值  : unsigned
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
unsigned char vendor_get_sta_load(vendor_sta_t *vsta, 
                                                adapt_sta_t *unused)
{
    u32 curr_tx_mbytes_1s = 0, curr_rx_mbytes_1s = 0;
    int rx_rate = 0, tx_rate = 0;
    u16 current_rate;
    unsigned char tmp_rate[20] = {0};
    /*
    * tx_tp_kbits 转为 tx_tp_mbits 直接除1024 这里直接 进行移位操作
    * 可以避免 Unknown symbol __udivdi3 错误  原因如下：
    * 32位系统中的除法操作（如：a/b），当a为64位变量时不能直接使用除法
    * 符号‘/’，否则就会出现上述错误。如果必须做64位除法应该使用函数do_div(a,b)
    */
    curr_tx_mbytes_1s = (u32)((u64)vsta->sta_stats.tx_tp_kbits >> 10);
    curr_rx_mbytes_1s = (u32)((u64)vsta->sta_stats.rx_tp_kbits >> 10);
    current_rate = vsta->cur_tx_data_rate;
    get_current_rate(vsta, current_rate, &tx_rate, tmp_rate);

    current_rate = vsta->cur_rx_data_rate;
    get_current_rate(vsta, current_rate, &rx_rate, tmp_rate);

     return (u8)((curr_tx_mbytes_1s/tx_rate + 
        curr_rx_mbytes_1s/rx_rate) / 2);
}


/*****************************************************************************
 函 数 名  : vendor_get_sta_flow
 功能描述  : 获取客户端的流量, 单位: Byte/S
 输入参数  : vendor_sta_t *vsta   
             adapt_sta_t *unused  
 输出参数  : 无
 返 回 值  : unsigned
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
unsigned int vendor_get_sta_flow(vendor_sta_t *vsta, adapt_sta_t *unused)
{
    u64 curr_tx_bytes_1s = 0, curr_rx_bytes_1s = 0;
    /*
     * kbits/s 转换为 Byte/s 应该是先乘以1024(左移10为)转为 bits/s 
     * 然后除 8 (右移3位) 同样为了避免  Unknown symbol __udivdi3 错误
     * 这里不用除法改用 移位操作
    */
    curr_tx_bytes_1s = ((u64)vsta->sta_stats.tx_tp_kbits << (10 - 3));
    curr_rx_bytes_1s = ((u64)vsta->sta_stats.rx_tp_kbits << (10 - 3));
    return (u32)(curr_tx_bytes_1s + curr_rx_bytes_1s);
}

/*****************************************************************************
 函 数 名  : vendor_get_bssid
 功能描述  : 获取ap的bssid (mac地址)
 输入参数  : vendor_vap_t *vap  
             u_int8_t *mac      
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
void vendor_get_bssid(vendor_vap_t *vap, u_int8_t *mac)
{
    struct sta_info *pfirsta;
    struct sta_priv *pstapriv = &vap->stapriv;
    struct mlme_priv *pmlmepriv = &(vap->mlmepriv);
    struct wlan_network *cur_network = &(pmlmepriv->cur_network);
    struct mlme_ext_priv *pmlmeext = &(vap->mlmeextpriv);
    struct mlme_ext_info *pmlmeinfo = &(pmlmeext->mlmext_info);

    if(rtw_is_adapter_up(vap)) {
        if(MLME_IS_STA(vap) && !(pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS)) {
            pfirsta = rtw_get_stainfo(pstapriv, vap->phl_role->mac_addr);
        } else {
            pfirsta = rtw_get_stainfo(pstapriv, cur_network->network.MacAddress);
        }

        if(NULL == pfirsta) {
            maps_err("NULL pointer:pfirsta");
            return ;
        }
        if(pfirsta->phl_sta) {
            memcpy(mac, pfirsta->phl_sta->mac_addr, MAC_ADDR_LEN);
        } else {
            maps_err("NULL pointer:pfirsta->phl_sta");
            return ;
        }
    } else {
        maps_err("The interface is not running");
        return ;
    }
    return ;
}

/*****************************************************************************
 函 数 名  : vendor_get_sta_rssi
 功能描述  : 返回客户端rssi, 单位：dBm. 范围: -(RTK_RSSI_OFFSET
                 - 1) ~ -1dBm. 失败时返回0
 输入参数  : vendor_sta_t *vsta  
 输出参数  : 无
 返 回 值  : static
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
signed char vendor_get_sta_rssi(vendor_sta_t *vsta)
{
    signed char rssi = 0;
    rssi = rtw_phl_get_sta_rssi(vsta->phl_sta);
    if (rssi >= RTK_RSSI_OFFSET) {
        /* 某些代码可能会认为RSSI 0dBm为非法值, 因此此处返回-1dBm */
        return -1;
    }

    if (!rssi) {
        /* invalid rssi */
        return 0;
    }

    return (rssi) - RTK_RSSI_OFFSET;
}


/*****************************************************************************
 函 数 名  : vendor_get_sta_btm
 功能描述  : vsta 是否支持BTM
 输入参数  : vendor_sta_t *vsta  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2022年3月9日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
bool vendor_get_sta_btm(vendor_sta_t *vsta)
{
#ifdef CONFIG_IEEE80211V
    return  rtw_wnm_get_ext_cap_btm(vsta->ext_capab_ie_data);
#else
    return false;
#endif
}


