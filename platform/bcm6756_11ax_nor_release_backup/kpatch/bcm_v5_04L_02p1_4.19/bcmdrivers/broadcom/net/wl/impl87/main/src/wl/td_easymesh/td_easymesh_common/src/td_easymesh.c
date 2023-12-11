/*****************************************************************************
Copyright (C), 吉祥腾达，保留所有版权
File name ：td_easymesh.c
Description : easymesh
Author ：qinke@tenda.cn
Version ：v1.0
Date ：2020.4.1
*****************************************************************************/

#include <net/sock.h>
#include <linux/uaccess.h>
#ifdef TD_EM_BROADCOM
#include <wlioctl.h>
#endif

#include "td_easymesh_nl.h"
#include "td_easymesh_dbg.h"
#include "td_easymesh.h"
#include "td_easymesh_interface.h"

#include "td_easymesh_opclass.h"

/*相关假数据内容宏定义*/
#define TEMPORARY_OP_CLASS            81
#define TEMPORARY_CHANNEL             149
#define TEMPORARY_RADIO_NUM           1
#define TEMPORARY_OP_CLASS_NUM        1
#define TEMPORARY_NUM                 1
#define TEMPORARY_PREFERENCE          0
#define TEMPORARY_CAC_STATUS_DATA     0
#define TEMPORARY_MIN_FREQ_SEPARATION 20
#define EM_MSG_MAX_BUFF_SIZE          2048
#define BTM_REPORT_LEN                24

#define NL_DATA_RSERVED               4
#define NETLINK_TYPE_LEN              4

int g_multiap_assoc_status = 0;
int g_em_mesh_cfg = 0;
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
void* g_em_bss[TD_EM_MAX_BAND] = {0};
#endif

/*lint  -e10*/
/*lint  -e161*/
/*lint  -e163*/
/*lint  -e101*/
/*lint  -e63*/
/*lint  -e26*/

static int em_copy_to_user(void *dst, void *src, int len);
static int em_copy_from_user(void *dst, void *src, int len);
typedef struct em_hostapd_msg
{
    int cmd;
    int reason;
    int state;
    unsigned char macaddr[TD_EM_MACADDRLEN];
}em_hostapd_msg_t;//lint !e129 !e19

/*****************************************************************************
 函 数 名  : td_em_sta_leave_event
 功能描述  : 客户端离线netlink通知
 输入参数  : em_osif osif                  
             char *bss_mac               
             char *sta_mac               
             netlink_rson_code_e reason  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_sta_leave_event(em_osif osif, char *bss_mac, char *sta_mac, netlink_rson_code_e reason)
{
    netlink_client_leave_t sta_leave_info;
    char dst_mac[TD_EM_MACADDRLEN * 4] = {0};

    if (!osif || !bss_mac || !sta_mac) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    memset(&sta_leave_info, 0, sizeof(netlink_client_leave_t));

    sta_leave_info.head.event = TD_EASYMESH_CLIENT_LEAVE;
    sta_leave_info.reason = reason;
    memcpy(sta_leave_info.mac.bss_mac, bss_mac, TD_EM_MACADDRLEN);
    memcpy(sta_leave_info.mac.sta_mac, sta_mac, TD_EM_MACADDRLEN);

    snprintf(dst_mac, sizeof(BROADCAST_MACADDR) , "%pM", sta_mac);
    if ( !strncmp(dst_mac, BROADCAST_MACADDR,  sizeof(BROADCAST_MACADDR) - 1) ) {
        TD_EM_DBG_PARAM_ERR("wifi is going down, send the broadcast disassoc!\n");
        return -1;
    } else if (td_em_get_sta_traffic(osif, &sta_leave_info.traffic, sta_mac) < 0) {
        return -1;
    }

    td_em_nl_send((char *)&sta_leave_info, sizeof(netlink_client_leave_t), TD_EM_NLUSER_MULTICAST_MESH);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_sta_connect_fail_event
 功能描述  : 客户端关联失败通知
 输入参数  : em_osif osif                    
             char *bss_mac                 
             char *sta_mac                 
             netlink_rson_code_e reason    
             netlink_status_code_e status  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
void td_em_sta_connect_fail_event(em_osif osif, char *bss_mac, char *sta_mac, netlink_rson_code_e reason, netlink_status_code_e status)
{
    netlink_client_connect_fail_t sta_info;

    if (!osif || !bss_mac || !sta_mac) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    memset(&sta_info, 0, sizeof(netlink_client_connect_fail_t));
    
    sta_info.head.event = TD_EASYMESH_CLIENT_CONNECT_FAIL;
    memcpy(sta_info.mac.bss_mac, bss_mac, TD_EM_MACADDRLEN);
    memcpy(sta_info.mac.sta_mac, sta_mac, TD_EM_MACADDRLEN);
    sta_info.status = status;
    sta_info.reason = reason;
    td_em_nl_send((char *)&sta_info, sizeof(netlink_client_connect_fail_t), TD_EM_NLUSER_MULTICAST_MESH);
    return;
}

/*****************************************************************************
 函 数 名  : td_em_client_event
 功能描述  : 客户端通知（上线、下线、关联失败）
 输入参数  : typeof(__func__) func         方便调试追踪
             typeof(__LINE__) line         方便调试追踪
             em_osif osif                    
             char *bss_mac                 
             char *sta_mac                 
             netlink_event_type_e event    
             netlink_rson_code_e reason    
             netlink_status_code_e status  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
void td_em_client_event(typeof(__FUNCTION__) func, typeof(__LINE__) line, void* osif, char *bss_mac, char *sta_mac, netlink_event_type_e event, netlink_rson_code_e reason, netlink_status_code_e status)
{

    if (!sta_mac || !func || !osif || !bss_mac) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    if (TD_EM_BSS_WORKMODE_STATION == td_em_get_interface_mode((em_osif)osif)) {
        TD_EM_DBG_MSG("current workmode is station\n");
        return;
    }

    switch (event) {
        case TD_EASYMESH_CLIENT_JOIN:
            TD_EM_DBG_MSG("CLIENT_JOIN func:%s(%d),bss_mac %pM, sta_mac%pM,status:%d, reason:%d\n", func, line, bss_mac ,sta_mac, status, reason);
        break;

        case TD_EASYMESH_CLIENT_LEAVE:
            if (td_em_sta_leave_event(osif, bss_mac, sta_mac, reason) >= 0) {
                TD_EM_DBG_MSG("CLIENT_LEAVE func:%s(%d),bss_mac %pM, sta_mac%pM,status:%d, reason:%d\n", func, line, bss_mac ,sta_mac, status, reason);
            }
        break;

        case TD_EASYMESH_CLIENT_CONNECT_FAIL:
            TD_EM_DBG_MSG("CLIENT_CONNECT_FAIL func:%s(%d),bss_mac %pM, sta_mac%pM,status:%d, reason:%d\n", func, line, bss_mac ,sta_mac, status, reason);
            td_em_sta_connect_fail_event(osif, bss_mac, sta_mac, reason, status);
        break;

        default:
            TD_EM_DBG_PARAM_ERR("default event\n");
        break;
    }

    return;
}

/*****************************************************************************
 函 数 名  : td_client_assoc_sta_control
 功能描述  : 关联sta控制请求
 输入参数  : em_osif osif          
             unsigned char *buf  
               
 输出参数  : 无
 返 回 值  : 0 成功，1 失败
 
 修改历史      :
  1.日    期   : 2020年5月22日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_client_assoc_sta_control(em_osif osif, unsigned char *buf)
{
    int ret = 0;
    if (!osif || !buf) 
    {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }
    TD_EM_DBG_TRACE("start!\n");
    ret = td_em_client_assoc_sta_control(osif, buf);
    TD_EM_DBG_TRACE("end!\n");

    return ret;
}

/*****************************************************************************
 函 数 名  : td_steer_btm_report
 功能描述  : 漫游切换报告
 输入参数  : unsigned char *buf  
 输出参数  : 无
 返 回 值  : 无
 
 修改历史      :
  1.日    期   : 2020年5月22日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static void td_steer_btm_report(em_osif osif, unsigned char *buf, unsigned char status)
{
    int offset = 0;
    client_steer_btm_report_t steer_btm_report;

    if (!osif || !buf)
    {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }
    
    steer_btm_report.type.event = TD_EASYMESH_STEER_BTM_REPORT;
    
    td_em_get_mac_addr(osif, steer_btm_report.radio_mac);
    memcpy(steer_btm_report.sta_mac, &buf[offset], TD_EM_MACADDRLEN);
    offset += TD_EM_MACADDRLEN;
    memcpy(steer_btm_report.target_bssid, &buf[offset], TD_EM_MACADDRLEN);
    steer_btm_report.btm_status = status;    
    
    td_em_nl_send((char *)&steer_btm_report, sizeof(client_steer_btm_report_t), TD_EM_NLUSER_EASYMESH);
    
    return;
}

/*****************************************************************************
 函 数 名  : maps_bss_tran_res_handler
 功能描述  : 处理sta的漫游状态
 输入参数  : vendor_sta_t *vsta    
             vendor_vap_t *vap     
             unsigned char status  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2021年7月8日
    作    者   : wangjianqiang
    修改内容   : 新生成函数

*****************************************************************************/
void td_em_sta_tran_res_handler(unsigned char *sta_mac, 
                                void *osif, 
                                unsigned char status, unsigned char *target_bssid)
{

    unsigned char tmpbuf[24] = {0};
    memcpy(tmpbuf, sta_mac, TD_EM_MACADDRLEN);
    memcpy(tmpbuf+TD_EM_MACADDRLEN, target_bssid, TD_EM_MACADDRLEN);
    
    td_steer_btm_report(osif, tmpbuf, status);
}
/*****************************************************************************
 函 数 名  : td_client_steer_request
 功能描述  : 漫游切换请求
 输入参数  : em_osif osif          
             unsigned char *buf  
             
 输出参数  : 无    
 返 回 值  : 0 成功，1 失败
 
 修改历史      :
  1.日    期   : 2020年5月22日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_client_steer_request(em_osif osif, unsigned char *buf)
{
    unsigned short disassoc_time = 0;
    unsigned short opp_window = 0;
    unsigned char request_mode = 0;
    client_steer_req_t client_steer;
    sta_info_report_t sta_info;
    unsigned char bss[TD_EM_MACADDRLEN] = {0};
    unsigned char sta[TD_EM_MACADDRLEN] = {0};    
    int isbtm_support = 0;
    int channel = 0;
    int op_class = 0;
    int ret = 0;
    int i = 0;
    
    if (!osif || !buf) 
    {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }

    memcpy(&client_steer, buf, sizeof(client_steer_req_t));
    request_mode = client_steer.req_mode;
    disassoc_time = client_steer.disassoc_time;
    opp_window = client_steer.opp_window;
    
    TD_EM_DBG_TRACE(" req_mode : %d\n", request_mode);
    TD_EM_DBG_TRACE(" opp_window : %d\n", opp_window);
    TD_EM_DBG_TRACE(" disassoc_time : %d\n", disassoc_time);
    TD_EM_DBG_TRACE(" sta_num : %d\n", client_steer.sta_num);

    if (0 == client_steer.sta_num && 1 == client_steer.target_bss_num)
    {
        //all association sta do btm request at the agent
    }
    else if (((1 <= client_steer.sta_num) || (1 <= client_steer.target_bss_num)) && (client_steer.sta_num == client_steer.target_bss_num))
    {
        /*do normal client steer*/
        for (i = 0; i < client_steer.sta_num; i++)
        {
            isbtm_support = td_em_is_support_btm(osif, client_steer.steer_req_sta[i].sta_mac);
            TD_EM_DBG_TRACE(" td_em_is_support_btm  %d \n", isbtm_support);
            if (isbtm_support)
            {
                TD_EM_DBG_TRACE(" STA MAC %02X-%02X-%02X-%02X-%02X-%02X \n",
                    client_steer.steer_req_sta[i].sta_mac[0], client_steer.steer_req_sta[i].sta_mac[1],
                    client_steer.steer_req_sta[i].sta_mac[2], client_steer.steer_req_sta[i].sta_mac[3],
                    client_steer.steer_req_sta[i].sta_mac[4], client_steer.steer_req_sta[i].sta_mac[5]);
                TD_EM_DBG_TRACE(" Target BSS MAC %02X-%02X-%02X-%02X-%02X-%02X \n",
                    client_steer.req_bss_info[i].target_bssid[0], client_steer.req_bss_info[i].target_bssid[1],
                    client_steer.req_bss_info[i].target_bssid[2], client_steer.req_bss_info[i].target_bssid[3],
                    client_steer.req_bss_info[i].target_bssid[4], client_steer.req_bss_info[i].target_bssid[5]);
                channel = client_steer.req_bss_info[i].channel;
                op_class = client_steer.req_bss_info[i].opclass;
                
                memcpy(bss, client_steer.req_bss_info[i].target_bssid, TD_EM_MACADDRLEN);
                memcpy(sta, client_steer.steer_req_sta[i].sta_mac, TD_EM_MACADDRLEN);
                ret = td_em_send_btm_req(osif, channel, op_class, bss, sta);
                TD_EM_DBG_TRACE(" td_em_send_btm_req ret %d \n", ret);
            }
            else
            {
                memset(&sta_info, 0, sizeof(sta_info_report_t));
                sta_info.sta_num = 1;
                sta_info.type.event = TD_EASYMESH_STEER_STA_BTM;
                
                memcpy(sta_info.sta_msg[i].sta_mac, client_steer.steer_req_sta[i].sta_mac, TD_EM_MACADDRLEN);

                td_em_nl_send((char *)&sta_info, sizeof(sta_info_report_t), TD_EM_NLUSER_EASYMESH);
                return 0;
            }
        }
        
    }
    else
    {
        TD_EM_DBG_DANGER(" can't do the client steer\n");
        return -1;
    }
     
     return ret;
}

/*****************************************************************************
 函 数 名  : td_steer_policy
 功能描述  : 漫游策略配置
 输入参数  : em_osif osif          
             unsigned char *buf  
                  
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_steer_policy(em_osif osif, unsigned char *buf)
{
    int ret = 0;
    if (!osif || !buf) 
    {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }
    TD_EM_DBG_TRACE(" start!\n");
    ret = td_em_set_steer_policy(osif, buf);
    TD_EM_DBG_TRACE(" end!\n");
    
    return ret;
}

/*****************************************************************************
 函 数 名  : td_update_backhaul_steer_results
 功能描述  : 回传链路优化状态通知
 输入参数  : em_osif osif  
 输出参数  : 无
 返 回 值  : 无
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_backhaul_steering_response(em_osif osif, void *buf)
{
    backhaul_steering_response_t bh_steer_rsp;
    int ret = 0;
    //send the backhaul results
    if (!osif) 
    {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }
    ret = td_em_get_bh_steer_results(osif, &bh_steer_rsp);

    if (-1 == ret)
    {
        TD_EM_DBG_DANGER("td_em_get_bh_steer_results failed!\n");
        return -1;
    }
    TD_EM_DBG_TRACE(" result_code : %d\n", bh_steer_rsp.result_code);
    td_em_nl_send((char *)&bh_steer_rsp, sizeof(backhaul_steering_response_t), TD_EM_NLUSER_EASYMESH);
    return 0;
}


/*****************************************************************************
 函 数 名  : td_backhaul_steering_request
 功能描述      : 处理回传链路请求
 输入参数      : em_osif osif          
             unsigned char *buf  
                  
 输出参数  : int *output_len
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_backhaul_steering_request(em_osif osif, unsigned char *buf, int *output_len)
{
    int ret = 0;
    if (!osif || !output_len || !buf) 
    {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }
    /* 进行回传链路优化 */
    ret = td_em_do_backhaul_steer(osif, buf);
    *output_len = 0;

    return ret;
}

/*****************************************************************************
 函 数 名  : td_em_roam_notify
 功能描述  : 回传链路切换bss成功后通知到上层
 输入参数  : u8 *curbssid
 输出参数  : 无

 修改历史      :
  1.日    期   : 2022年6月22日
    作    者   : shiguikang
    修改内容   : 新生成函数
*****************************************************************************/
void td_em_roam_notify(u8 *curbssid)
{
    backhaul_steering_response_t backhaul_steering_response_event;

    memset(&backhaul_steering_response_event, 0, sizeof(backhaul_steering_response_event));
    backhaul_steering_response_event.type.event = TD_EASYMESH_BACKHAUL_STEER_RESPONSE;
    memcpy(backhaul_steering_response_event.bssid, curbssid, TD_EM_MACADDRLEN);
    backhaul_steering_response_event.result_code = BACKHAUL_STEER_SUCCESS;
    td_em_nl_send((char *)&backhaul_steering_response_event, sizeof(backhaul_steering_response_event), TD_EM_NLUSER_EASYMESH);
    return;
}

/*****************************************************************************
 函 数 名  : td_cac_termination
 功能描述  : 获取cac termination相关信息
 输入参数  : em_osif osif                          
             cac_termination_t *cac_termination  
             int sizeofbuf                     
 输出参数  : int *output_len
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_cac_termination(em_osif osif, cac_termination_t *cac_termination, int sizeofbuf, int *output_len)
{
    int i = 0;
    int size = 0;
    
    if (!osif || !output_len || !cac_termination) 
    {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }
    
    size = sizeof(cac_termination) + TEMPORARY_RADIO_NUM * sizeof(cac_termination_radio_t);
    if (sizeofbuf < size)
    {
        TD_EM_DBG_DANGER("sizeofbuf = %d, size = %d;Not enough space!\n",sizeofbuf, size);
        return -1;
    }
    
    /*第一阶段暂时用不到，填充假数据*/
    cac_termination->radio_nr = TEMPORARY_RADIO_NUM;

    for(i = 0; i < cac_termination->radio_nr; i++)
    {
        td_em_get_mac_addr(osif, cac_termination->radios[i].radio_unique_identifier);
        cac_termination->radios[i].channel = TEMPORARY_CHANNEL; //暂时填充假数据,保证协议正常交互
        cac_termination->radios[i].op_class = TEMPORARY_OP_CLASS; //暂时填充假数据,保证协议正常交互
    }

    *output_len = size;

    return 0;
}

/*****************************************************************************
 函 数 名  : td_cac_request
 功能描述  : 获取cac request相关信息
 输入参数  : em_osif osif                  
             cac_request_t *cac_request  
             int sizeofbuf             
 输出参数  : int *output_len
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_cac_request(em_osif osif, cac_request_t *cac_request, int sizeofbuf,int *output_len)
{
    int i = 0;
    int size = 0;
    
    if (!osif || !output_len || !cac_request) 
    {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }

    size = sizeof(cac_request_t) + TEMPORARY_RADIO_NUM*sizeof(cac_request_radio_t);
    if (sizeofbuf < size)
    {
        TD_EM_DBG_DANGER(" Not enough space!\n");
        return -1;
    }
    
    /*第一阶段暂时用不到，填充假数据*/
    cac_request->radio_nr = TEMPORARY_RADIO_NUM;

    for(i = 0; i < cac_request->radio_nr; i++)
    {
        td_em_get_mac_addr(osif, cac_request->radios[i].radio_unique_identifier);
        cac_request->radios[i].op_class = TEMPORARY_OP_CLASS; //暂时填充假数据,保证协议正常交互
        cac_request->radios[i].channel = TEMPORARY_CHANNEL; //暂时填充假数据,保证协议正常交互
        cac_request->radios[i].cac_method = CAC_METHOD_CONTINUOUS_CAC;
        cac_request->radios[i].cac_completion_action = CAC_ACTION_RAMAIN_ON_CHANNEL;
    }

    *output_len = size;

    return 0;
}

/*****************************************************************************
 函 数 名  : td_operating_channel_report
 功能描述  :     获取操作信息信息
 输入参数  :     em_osif osif                                     
             operating_channel_report_t *operating_channel  
             int sizeofbuf                                
 输出参数  : int *output_len
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容     : 新生成函数

*****************************************************************************/
static int td_operating_channel_report(em_osif osif, operating_channel_report_t *operating_channel, int sizeofbuf, int *output_len)
{
    int i = 0;
    int size = 0;
    
    if (!osif || !output_len || !operating_channel) 
    {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }
    
    size = sizeof(operating_channel_report_t) + TEMPORARY_OP_CLASS_NUM * sizeof(operating_ch_report_op_class_t);
    if (sizeofbuf < size)
    {
        TD_EM_DBG_DANGER(" Not enough space!\n");
        return -1;
    }
    TD_EM_DBG_TRACE(" size : %d\n", size);
    
    td_em_get_mac_addr(osif, operating_channel->addr);
    operating_channel->cur_tx_pwr = td_em_get_max_power(osif); //当前传输功率
    operating_channel->cur_op_class_nr = TEMPORARY_OP_CLASS_NUM; //临时假数据

    for (i = 0; i < operating_channel->cur_op_class_nr; i++)
    {
        operating_channel->operating_channels[i].op_class = TEMPORARY_OP_CLASS;   //暂时填充假数据,保证协议正常交互
        operating_channel->operating_channels[i].cur_channel = TEMPORARY_CHANNEL; //暂时填充假数据,保证协议正常交互
    }

    *output_len = size;

    return 0;
}

/*****************************************************************************
 函 数 名  : td_channel_selection_response
 功能描述  : 信道选择请求回应信息获取
 输入参数  : em_osif osif                                          
             channel_select_response_t *channel_select_response  
             int sizeofbuf                                     
 输出参数  : int *output_len
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_channel_selection_response(em_osif osif, channel_select_response_t *channel_select_response, int sizeofbuf,int *output_len)
{
    int size = 0;
    
    if (!osif || !output_len || !channel_select_response) 
    {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }

    size = sizeof(channel_select_response_t);
    TD_EM_DBG_TRACE(" size : %d\n", size);
    
    if (sizeofbuf < size)
    {
        TD_EM_DBG_DANGER(" Not enough space!\n");
        return -1;
    }
    
    td_em_get_mac_addr(osif, channel_select_response->addr);
    channel_select_response->response_code = g_td_em_fake_data.chnl_select_rsp_code;//第一阶段由于上层暂时没有决策逻辑，暂时写假数据,并且该数值可通过串口配置
    TD_EM_DBG_TRACE(" response_code : %d\n", channel_select_response->response_code);

    *output_len = size;

    return 0;
}

/*****************************************************************************
 函 数 名  : td_transmit_power_limit
 功能描述  : 获取功率限制信息
 输入参数  : em_osif osif                                    
             transmit_power_limit_t *transmit_power_limit  
             int sizeofbuf                               
 输出参数  : int *output_len
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_transmit_power_limit(em_osif osif, transmit_power_limit_t *transmit_power_limit, int sizeofbuf,int *output_len)
{
    int size = 0;
    
    if (!osif || !output_len || !transmit_power_limit) 
    {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }

    size = sizeof(transmit_power_limit_t);

    if (sizeofbuf < size)
    {
        TD_EM_DBG_DANGER(" Not enough space!\n");
        return -1;
    }
    
    /*获取mac地址*/
    td_em_get_mac_addr(osif, transmit_power_limit->addr);
    /*获取最大功率*/
    transmit_power_limit->transmit_power_limit_eirp = td_em_get_max_power(osif);
    *output_len = size;

    return 0;
}

/*****************************************************************************
 函 数 名  : td_cac_status_report
 功能描述  : 获取cac status 想选信息
 输入参数  : em_osif osif          
             unsigned char *buf  
             int sizeofbuf    
 输出参数  :  int *output_len
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_cac_status_report(em_osif osif, unsigned char *buf, int sizeofbuf, int *output_len)
{
    cac_status_report_t cac_status;
    int i = 0;
    int ret = 0;
    unsigned char offset = 0;
   
    if (!osif || !output_len || !buf) 
    {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }
    
    /*第一阶段暂时用不到，暂时预留接口, 填写假数据*/
    memset(&cac_status, 0, sizeof(cac_completion_report_t));
    cac_status.available_channel_nr = TEMPORARY_NUM;
    cac_status.available_channels = (cac_complete_available_chnl_t *)kmalloc(cac_status.available_channel_nr * sizeof(cac_complete_available_chnl_t), GFP_ATOMIC);
    if (NULL == cac_status.available_channels)
    {
        TD_EM_DBG_DANGER("kmalloc failed!\n");
        return -1;
    }
    
    /* 获取可利用信道相关信息 */
    for (i = 0; i < cac_status.available_channel_nr; i++)
    {
        cac_status.available_channels[i].ac_op_class = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
        cac_status.available_channels[i].ac_channel  = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
        cac_status.available_channels[i].identify_time = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
    }
    
    /* 获取可利用信道相关信息 */
    cac_status.nonoccup_pair_nr = 1;
    cac_status.nonoccup_pairs = (cac_non_occupancy_classchnl_pairs_t *)kmalloc(cac_status.nonoccup_pair_nr * sizeof(cac_non_occupancy_classchnl_pairs_t), GFP_ATOMIC);
    if (NULL == cac_status.nonoccup_pairs)
    {
        TD_EM_DBG_DANGER(" kmalloc failed!\n");
        ret = -1;
        goto error1;
    }
    
    for (i = 0; i < cac_status.nonoccup_pair_nr; i++)
    {
        cac_status.nonoccup_pairs[i].nonoccup_op_class = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
        cac_status.nonoccup_pairs[i].nonoccup_channel  = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
        cac_status.nonoccup_pairs[i].nonoccup_remaining_time = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
    }

    cac_status.active_pair_nr = 1;
    cac_status.active_pairs = (cac_active_class_chnl_pairs_t *)kmalloc(cac_status.active_pair_nr * sizeof(cac_active_class_chnl_pairs_t), GFP_ATOMIC);
    if (NULL == cac_status.active_pairs)
    {
        TD_EM_DBG_DANGER(" kmalloc failed!\n");
        ret = -1;
        goto error2;
    }

    for (i = 0; i < cac_status.active_pair_nr; i++)
    {
        cac_status.active_pairs[i].active_op_class = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
        cac_status.active_pairs[i].active_channel  = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
        cac_status.active_pairs[i].active_remaining_time = TEMPORARY_CAC_STATUS_DATA;//暂时写假数据
    }

    buf[offset++] = cac_status.available_channel_nr;
    for (i = 0; i < cac_status.available_channel_nr; i++)
    {
        buf[offset++] = cac_status.available_channels[i].ac_op_class;
        buf[offset++] = cac_status.available_channels[i].ac_channel;
        memcpy(&buf[offset], &(cac_status.available_channels[i].identify_time), sizeof(cac_status.available_channels[i].identify_time));
        offset += sizeof(cac_status.available_channels[i].identify_time);

        if (sizeofbuf < offset)
        {
            TD_EM_DBG_DANGER(" Not enough space!\n");
            ret = -1;
            goto error3;
        }
    }

    buf[offset++] = cac_status.nonoccup_pair_nr;
    for (i = 0; i < cac_status.nonoccup_pair_nr; i++)
    {
        buf[offset++] = cac_status.nonoccup_pairs[i].nonoccup_op_class;
        buf[offset++] = cac_status.nonoccup_pairs[i].nonoccup_channel;
        memcpy(&buf[offset], &(cac_status.nonoccup_pairs[i].nonoccup_remaining_time), sizeof(cac_status.nonoccup_pairs[i].nonoccup_remaining_time));
        offset += sizeof(cac_status.nonoccup_pairs[i].nonoccup_remaining_time);

        if (sizeofbuf < offset)
        {
            TD_EM_DBG_DANGER(" Not enough space!\n");
            ret = -1;
            goto error3;
        }
    }  

    buf[offset++] = cac_status.active_pair_nr;
    for (i = 0; i < cac_status.active_pair_nr; i++)
    {
        buf[offset++] = cac_status.active_pairs[i].active_op_class;
        buf[offset++] = cac_status.active_pairs[i].active_channel;
        memcpy(&buf[offset], &(cac_status.active_pairs[i].active_remaining_time), sizeof(cac_status.active_pairs[i].active_remaining_time));
        offset += sizeof(cac_status.active_pairs[i].active_remaining_time);

        if (sizeofbuf < offset)
        {
            TD_EM_DBG_DANGER(" Not enough space!\n");
            ret = -1;
            goto error3;
        }
    }

    *output_len = offset;
    
error3:
    if(cac_status.active_pairs)
    {
        kfree(cac_status.active_pairs);
        cac_status.active_pairs = NULL;
    }
error2:       
    if(cac_status.nonoccup_pairs)
    {
        kfree(cac_status.nonoccup_pairs);
        cac_status.nonoccup_pairs = NULL;
    }

error1:
    if(cac_status.available_channels)
    {
        kfree(cac_status.available_channels);
        cac_status.available_channels = NULL;
    }

    return ret;
}

/*****************************************************************************
 函 数 名  : td_cac_completion_report
 功能描述  : 获取CAC_COMPLETION TLV相关信息
 输入参数  : em_osif osif          
             unsigned char *buf  
             int sizeofbuf     
 输出参数  : int *output_len
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_cac_completion_report(em_osif osif, unsigned char *buf, int sizeofbuf,int *output_len)
{
    cac_completion_report_t cac_complet;
    int i = 0;
    int j = 0;
    int ret = 0;
    unsigned char offset = 0;
   
    if (!osif || !output_len || !buf) 
    {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }
    
    /*第一阶段暂时用不到，暂时预留接口, 填写假数据*/
    memset(&cac_complet, 0, sizeof(cac_completion_report_t));
    cac_complet.radio_nr = TEMPORARY_RADIO_NUM;
    cac_complet.radios = (cac_completion_report_radio_t *)kmalloc(cac_complet.radio_nr * sizeof(cac_completion_report_radio_t), GFP_ATOMIC);
    if (NULL == cac_complet.radios)
    {
        TD_EM_DBG_DANGER(" kmalloc failed!\n");
        return -1;
    }
    
    for (i = 0; i < cac_complet.radio_nr; i++)
    {
        td_em_get_mac_addr(osif, cac_complet.radios[i].radio_unique_identifier);
        cac_complet.radios[i].op_class = TEMPORARY_OP_CLASS;//暂时写假数据
        cac_complet.radios[i].channel  = TEMPORARY_CHANNEL;//暂时写假数据
        cac_complet.radios[i].status   = CAC_COMPLETE_SUCCESS;//暂时写假数据
        cac_complet.radios[i].pairs_nr = TEMPORARY_RADIO_NUM;//暂时写假数据
        cac_complet.radios[i].pairs    = (cac_completion_report_class_channel_pairs_t *)kmalloc(cac_complet.radios[i].pairs_nr * sizeof(cac_completion_report_class_channel_pairs_t), GFP_ATOMIC);
        if (NULL == cac_complet.radios[i].pairs)
        {
            TD_EM_DBG_DANGER(" kmalloc failed!\n");
            ret = -1;
            goto error;

        }

        for(j = 0; j < cac_complet.radios[i].pairs_nr; j++)
        {
            cac_complet.radios[i].pairs[j].pairs_op_class = TEMPORARY_OP_CLASS;//暂时写假数据
            cac_complet.radios[i].pairs[j].pairs_channel  = TEMPORARY_CHANNEL;//暂时写假数据
        }
    }

    buf[offset++] = cac_complet.radio_nr;
    for (i = 0; i < cac_complet.radio_nr; i++)
    {
        memcpy(&buf[offset], cac_complet.radios[i].radio_unique_identifier, TD_EM_MACADDRLEN);
        offset += TD_EM_MACADDRLEN;
        buf[offset++] = cac_complet.radios[i].op_class;
        buf[offset++] = cac_complet.radios[i].channel;
        buf[offset++] = cac_complet.radios[i].status ;
        buf[offset++] = cac_complet.radios[i].pairs_nr;

        for(j = 0; j < cac_complet.radios[i].pairs_nr; j++)
        {
            buf[offset++] = cac_complet.radios[i].pairs[j].pairs_op_class;
            buf[offset++] = cac_complet.radios[i].pairs[j].pairs_channel;
        }

        if (sizeofbuf < offset)
        {
            TD_EM_DBG_DANGER(" Not enough space!\n");
            ret = -1;
            goto error1;
        }
    }
    
    *output_len = offset;
error1:
    for (i = 0; i < cac_complet.radio_nr; i++) 
    {
        if (cac_complet.radios[i].pairs_nr)
        {
            kfree(cac_complet.radios[i].pairs);
            cac_complet.radios[i].pairs = NULL;
        }
    }
    
error:
    if (cac_complet.radios) 
    {
        kfree(cac_complet.radios);
        cac_complet.radios = NULL;
    }

    return ret;
}

/*****************************************************************************
 函 数 名  : td_radio_operate_restrict
 功能描述  : 获取radio_operate_restrict信息
 输入参数  : em_osif osif                                            
            radio_operation_restriction_t *radio_operat_restrict  
            int sizeofbuf                                      
 输出参数  :  int *output_len 
            radio_operation_restriction_t *radio_operat_restrict
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_radio_operate_restrict(em_osif osif, radio_operation_restriction_t *radio_operat_restrict, int sizeofbuf,int *output_len)
{
    ap_radio_basic_cap_tlv_t ap_radio_basic_cap;
    int op_class_cout = 0;
    int unop_class_cout = 0;
    int size = 0;
    int i, j, k;

    if (!osif || !output_len || !radio_operat_restrict) 
    {
        TD_EM_DBG_PARAM_ERR(" null pointer!\n");
        return -1;
    }
    
    td_em_get_mac_addr(osif, radio_operat_restrict->addr);
    
    op_class_cout = td_em_get_opclass(osif, (unsigned char *)ap_radio_basic_cap.operating_class, td_em_get_max_power(osif));
    if (0 > op_class_cout)
    {
       op_class_cout = 0;
    }

    /*统计不可操作信道数量*/
    for (i = 0; i < op_class_cout && 0 != ap_radio_basic_cap.operating_class[i].num_unopch; i++)
    {
        unop_class_cout++;
    }

    /* 计算空间大小 */
    size = (sizeof(radio_operation_restriction_t) + unop_class_cout*(sizeof(radio_operation_restriction_op_class_t)));
    if (sizeofbuf < size)
    {
        TD_EM_DBG_PARAM_ERR(" Not enough space!\n");
        return -1;
    }
    
    radio_operat_restrict->op_class_num = unop_class_cout;
    if (0 != radio_operat_restrict->op_class_num)
    {
        for (i = 0, j = 0; i < op_class_cout && 0 != ap_radio_basic_cap.operating_class[i].num_unopch; i++)
        {
            /*获取操作类中，不可操作的信道信息,并传给上层应用*/
            radio_operat_restrict->radio_operate_restrict_op_class[j].opclass = ap_radio_basic_cap.operating_class[i].opclass;
            radio_operat_restrict->radio_operate_restrict_op_class[j].channel_num = ap_radio_basic_cap.operating_class[i].num_unopch;
            TD_EM_DBG_TRACE(" channel_num %d  \n", radio_operat_restrict->radio_operate_restrict_op_class[j].channel_num);

            for (k = 0; k < radio_operat_restrict->radio_operate_restrict_op_class[j].channel_num; k++)
            {
                radio_operat_restrict->radio_operate_restrict_op_class[j].restriction_channel[k].channel = ap_radio_basic_cap.operating_class[i].unopch[k];
                radio_operat_restrict->radio_operate_restrict_op_class[j].restriction_channel[k].min_freq_separation = TEMPORARY_MIN_FREQ_SEPARATION;//暂时先写20dbm
                TD_EM_DBG_TRACE(" channel [%d] \n", radio_operat_restrict->radio_operate_restrict_op_class[j].restriction_channel[k].channel);
            }
            j++;
        }
    }
    
    *output_len = size;

    return 0;
}
/*****************************************************************************
 函 数 名  : td_channel_preference_report
 功能描述  : 获取channel preference 相关信息
 输入参数  :    em_osif osif                        
            int sizeofbuf                   
 输出参数  :    channel_prefer_t *channel_prefer
            int *output_len 
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_channel_preference_report(em_osif osif, channel_prefer_t *channel_prefer, int sizeofbuf,int *output_len)
{
    ap_radio_basic_cap_tlv_t ap_radio_basic_cap;
    int op_class_total_cout = 0;
    int size = 0;
    int i;
    
    if (!osif || !output_len || !channel_prefer) 
    {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }

    td_em_get_mac_addr(osif, channel_prefer->addr);

    /*获取当前radio的所有操作类*/
    op_class_total_cout = td_em_get_opclass(osif, (unsigned char *)ap_radio_basic_cap.operating_class, td_em_get_max_power(osif));
    if (0 > op_class_total_cout)
    {
        op_class_total_cout = 0;
    }
    TD_EM_DBG_TRACE(" op_class_total_cout %d  \n", op_class_total_cout);

    if (sizeofbuf < size)
    {
        TD_EM_DBG_DANGER("Not enough space!\n");
        return -1;
    }
    
    channel_prefer->op_class_count = op_class_total_cout;
    
    for (i = 0; i < op_class_total_cout; i++)
    {
        channel_prefer->op_class_t[i].op_class = ap_radio_basic_cap.operating_class[i].opclass;
        TD_EM_DBG_TRACE("[%d] op_class %d  \n", i+1, channel_prefer->op_class_t[i].op_class);
        channel_prefer->op_class_t[i].channel_num = ap_radio_basic_cap.operating_class[i].num_opch;
        TD_EM_DBG_TRACE("[%d] channel_num %d  \n", i+1, channel_prefer->op_class_t[i].channel_num);
        memcpy(channel_prefer->op_class_t[i].channel, ap_radio_basic_cap.operating_class[i].opch, channel_prefer->op_class_t[i].channel_num);
        channel_prefer->op_class_t[i].preference = TEMPORARY_PREFERENCE; //暂时写假数据
        TD_EM_DBG_TRACE("[%d] reason_code %d  \n", i+1, channel_prefer->op_class_t[i].op_class);
        channel_prefer->op_class_t[i].reason_code = g_td_em_fake_data.channel_prefer_rson_code;//暂时写假数据,并且该数值可通过串口配置
        channel_prefer->op_class_t[i].bandwidth =  ap_radio_basic_cap.operating_class[i].band_width;
        TD_EM_DBG_TRACE("[%d] reason_code %d  \n", i+1, channel_prefer->op_class_t[i].reason_code);
    }
    
    /* 计算buf大小 */
    size = sizeof(channel_prefer_t) + channel_prefer->op_class_count * sizeof(channel_preference_op_class_t);
    TD_EM_DBG_TRACE(" size %d  \n", size);
    *output_len = size;
    
    return 0;
}

static int td_em_ap_radio_basic_cap_tlv(em_osif osif, ap_radio_basic_cap_tlv_t *ap_radio_basic_cap, int *output_len)
{

    if (!osif || !output_len || !ap_radio_basic_cap) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    td_em_get_mac_addr(osif, ap_radio_basic_cap->radio_unique_identifier);
    ap_radio_basic_cap->operating_class_count = td_em_get_opclass(osif, 
    (unsigned char *)ap_radio_basic_cap->operating_class, td_em_get_max_power(osif));
    
    if (ap_radio_basic_cap->operating_class_count < 0) {
        TD_EM_DBG_RET_ERR("get operating class error!\n");
        return -1;
    }

    *output_len = sizeof(ap_radio_basic_cap_tlv_t);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_radio_identifier_tlv
 功能描述  : 获取td_em_radio_identifier_tlv信息
 输入参数  : em_osif osif                        

 输出参数  : int *output_len
             radio_identifier_tlv_t *radio_id
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_radio_identifier_tlv(em_osif osif, radio_identifier_tlv_t *radio_id, int *output_len)
{
    if (!osif || !output_len || !radio_id) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    td_em_get_mac_addr(osif, radio_id->radio_unique_identifier);
    *output_len = sizeof(radio_identifier_tlv_t);
    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_ap_operational_bss_tlv
 功能描述  : 获取ap_operational_bss_tlv信息
 输入参数  : em_osif osif                                    

 输出参数  : ap_operational_bss_tlv_t *ap
             int *output_len
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_ap_operational_bss_tlv(em_osif osif, ap_operational_bss_tlv_t *ap_operational_bss, int *output_len)
{
    if (!osif || !output_len || !ap_operational_bss) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    td_em_get_mac_addr(osif, ap_operational_bss->bss_macaddr);
    ap_operational_bss->ssid_len = td_em_get_ssid(osif, ap_operational_bss->bss_ssid);
    *output_len = sizeof(ap_operational_bss_tlv_t);
    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_association_client_tlv
 功能描述  : 获取association_client信息
 输入参数  : em_osif osif                                   
             int sizeofbuf 输出数据buf大小                 

 输出参数  : association_client_tlv_t *assoc_client_info  
             int *output_len                              
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_association_client_tlv(em_osif osif, association_client_tlv_t *assoc_client_info, int sizeofbuf, int *output_len)
{
    int len = 0;
    if (!osif || !output_len || !assoc_client_info) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    assoc_client_info->sta_num = td_em_get_sta_num(osif);

    len = sizeof(association_client_tlv_t) + sizeof(association_client_sta_info_t) * assoc_client_info->sta_num;
    if (sizeofbuf < len) {
        TD_EM_DBG_PARAM_ERR("Space exceeded!\n");
        return -1;
    }

    td_em_get_mac_addr(osif, assoc_client_info->bss_macaddr);

    if (td_em_check_drv_state(osif)) {
        TD_EM_DBG_RET_ERR("check drv state error!\n");
        return -1;
    }

    td_em_iterate_sta_list(osif, 0, (unsigned char *)assoc_client_info->info, td_em_get_sta_link_time);

    *output_len = len;

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_one_device_info_tlv
 功能描述  : 获取one_device信息
 输入参数  : em_osif osif                          

 输出参数  : one_device_info_tlv_t *device_info  
             int *output_len                     
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_one_device_info_tlv(em_osif osif, one_device_info_tlv_t *device_info, int *output_len)
{
    if (!osif || !output_len || !device_info) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    device_info->media_type = td_em_get_media_type(osif);

    device_info->channel_Band = td_em_get_channel_width(osif);

    device_info->center_freq1 = td_em_get_centerfreq(osif);

    device_info->role = td_em_get_interface_mode(osif);
  
    *output_len = (int)sizeof(one_device_info_tlv_t);
    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_autoconfig_m2_tlv
 功能描述  : 获取autoconfig m2信息
 输入参数  : em_osif osif                    

 输出参数  : autoconfig_m2_tlv_t *m2_info  
             int *output_len               
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_autoconfig_m2_tlv(em_osif osif, autoconfig_m2_tlv_t *m2_info, int *output_len)
{
    if (!osif || !output_len || !m2_info) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    m2_info->association_state = 1;//auto config M2报文是关联后才触发，所以关联状态写1(已关联)
    m2_info->auth_type = td_em_get_auth_type(osif);
    m2_info->encr_type = td_em_get_encryption_type(osif);
    td_em_get_ssid(osif, m2_info->ssid);
    m2_info->bss_type = td_em_get_bss_type(osif);
    if (TD_EM_BSS_WORKMODE_AP == td_em_get_interface_mode(osif)) { //暂时参考瑞昱，写死
        m2_info->connect_type = TD_EM_CONNECT_TYPE_ESS;
    } else {
        m2_info->connect_type = TD_EM_CONNECT_TYPE_IBSS;
    }

    td_em_get_netkey(osif, m2_info->key);

    *output_len = sizeof(autoconfig_m2_tlv_t);

    return 0;
}

static int td_em_get_apcapability_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_apcapability(osif, result_buf);

    return 0;
}

static int td_em_get_apcapability_2_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_apcapability_2(osif, result_buf);

    return 0;
}

static int td_em_get_htcapability_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_htcapability(osif, result_buf);

    return 0;
}

static int td_em_get_vhtapCapability_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_vhtapCapability(osif, result_buf);

    return 0;
}

static int td_em_get_heapcapability_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_heapcapability(osif, result_buf);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_channelscan_capabilities_tlv
 功能描述      : 查询信道扫描能力
 输入参数      : em_osif osif
             unsigned char *result_buf
             int *output_len
 输出参数      : 0 : result_buf获取成功
 返 回 值  : -1：osif 为空|| result_buf 为空 || 需要填入的数据长度大于sizeofbuf
           其他：result_buf填充的长度
 日    期    : 2020年5月19日
 作    者    : 尹家政
*****************************************************************************/
static int td_em_get_channelscan_capabilities_tlv(em_osif osif, unsigned char *result_buf, int sizeofbuf, int *output_len)
{
    easymesh_chscan_ability_tlv_t *val ;
    int i = 0;
    ap_radio_basic_cap_tlv_t *op_class = NULL;
    
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }
    
    op_class = (ap_radio_basic_cap_tlv_t *)kmalloc(sizeof(ap_radio_basic_cap_tlv_t), GFP_ATOMIC);
    if (NULL == op_class) {
        TD_EM_DBG_PARAM_ERR("op_class kmalloc error!\n");
        return -1;
    }

    memset(op_class, 0, sizeof(ap_radio_basic_cap_tlv_t));
    td_em_get_mac_addr(osif, op_class->radio_unique_identifier);

    op_class->operating_class_count = td_em_get_opclass(osif, (unsigned char *)op_class->operating_class, td_em_get_max_power(osif));

    val = (easymesh_chscan_ability_tlv_t *)kmalloc((sizeof(easymesh_chscan_ability_tlv_t) + sizeof(easymesh_opclass_info_t)*op_class->operating_class_count),GFP_ATOMIC);

    if (NULL == val) {
        TD_EM_DBG_PARAM_ERR("val kmalloc error!\n");
        kfree(op_class);
        return -1;
    }

    if (sizeofbuf < (sizeof(easymesh_chscan_ability_tlv_t) + sizeof(easymesh_opclass_info_t)*op_class->operating_class_count)) {
        TD_EM_DBG_PARAM_ERR("sizeofbuf < val !\n");
        kfree(val);
        kfree(op_class);
        return -1;
    }
    memset(val, 0, (sizeof(easymesh_chscan_ability_tlv_t) + sizeof(easymesh_opclass_info_t)*op_class->operating_class_count));

    val->opclass_num = op_class->operating_class_count;
    memcpy(val->radio_bssid, op_class->radio_unique_identifier, TD_EM_MACADDRLEN);

    for (i = 0; i < val->opclass_num; i++) {
        val->opclass_info[i].op_class = op_class->operating_class[i].opclass;
        val->opclass_info[i].channel_num = op_class->operating_class[i].num_opch;
        memcpy(val->opclass_info[i].channel_list, op_class->operating_class[i].opch, sizeof(op_class->operating_class[i].opch));
    }

    memcpy(result_buf, val, (sizeof(easymesh_chscan_ability_tlv_t)+sizeof(easymesh_opclass_info_t)*op_class->operating_class_count));

    *output_len = (sizeof(easymesh_chscan_ability_tlv_t)+sizeof(easymesh_opclass_info_t)*op_class->operating_class_count);

    kfree(val);
    kfree(op_class);
    TD_EM_DBG_MSG("result_buf[0] = %02x", result_buf[0]);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_clientcapability_notify
 功能描述      : 上报客户端能力
 输入参数      : unsigned char *mac, unsigned char *bssid, 
             unsigned char framelength, unsigned char *framedata
 输出参数      :
 返 回 值  : 
 日    期    : 2020年5月19日
 作    者    : 尹家政
*****************************************************************************/
void td_em_clientcapability_notify(unsigned char *mac, unsigned char *bssid, unsigned int framelength, unsigned char *framedata)
{
    unsigned int date_len = sizeof(easymesh_clientcapability_tlv_t) + framelength;
    easymesh_clientcapability_tlv_t *clientcapability = (easymesh_clientcapability_tlv_t*)kmalloc(date_len,GFP_ATOMIC);

    if (NULL == clientcapability) {
        TD_EM_DBG_PARAM_ERR("clientcapability kmalloc error!\n");
        return;
    }

    memset(clientcapability, 0, date_len);

    clientcapability->head.event = TD_EASYMESH_CLIENT_CONNECT_NOTIFY;
    memcpy(clientcapability->macaddr, mac, TD_EM_MACADDRLEN);
    memcpy(clientcapability->bssid, bssid, TD_EM_MACADDRLEN);
    clientcapability->framelength = framelength;
    memcpy(clientcapability->framebody, framedata, clientcapability->framelength);

    td_em_nl_send((unsigned char*)clientcapability, date_len, TD_EM_NLUSER_MULTICAST_MESH);
    kfree(clientcapability);
    return;
}

/*****************************************************************************
 函 数 名  : td_em_wds_addif_br_notify
 功能描述      : 创建wds接口时通知netctl
 输入参数      : char *ifname
 输出参数      :
 返 回 值  : 
 日    期    : 2020年8月24日
 作    者    : 洪桂阳
*****************************************************************************/
void td_em_wds_addif_br_notify(char *ifname)
{
    int data_len = 0;
    char *msg = NULL;
    wds_create_notify_t *wds_notify = NULL;
    char *value = NULL;

    if (!ifname) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    wds_notify = (wds_create_notify_t *)kmalloc(sizeof(wds_create_notify_t), GFP_ATOMIC);
    if (!wds_notify) {
        TD_EM_DBG_PARAM_ERR("malloc wds_notify error!\n");
        return;
    }

    data_len = sizeof(wds_create_notify_t) + sizeof(netlink_event_head_e) + NL_DATA_RSERVED;  //4表示保留位
    msg = (char *)kmalloc(data_len, GFP_ATOMIC);
    
    if (!msg) {
        TD_EM_DBG_PARAM_ERR("malloc msg error!\n");
        kfree(wds_notify);
        return;
    }
    memset(msg, 0, data_len);
    value = msg;
    *value = TD_EASYMESH_WDS_ADDIF_BR;
    value = value + NETLINK_TYPE_LEN + NL_DATA_RSERVED;  //4个字节的type，4个字节的保留位

    memset(wds_notify, 0, sizeof(wds_create_notify_t));
    wds_notify->type.event = TD_EASYMESH_WDS_ADDIF_BR;
    memcpy(wds_notify->ifname, ifname, sizeof(wds_notify->ifname));
    TD_EM_DBG_TRACE("netlink: wds ifname(%s)  data_len= %d\n", wds_notify->ifname, data_len);
    memcpy(value, (char *)wds_notify, sizeof(wds_create_notify_t));

    td_em_nl_send(msg, data_len, TD_EM_NLUSER_WSERVER);
    kfree(msg);
    kfree(wds_notify);
    return;
}


/*****************************************************************************
 函 数 名  : td_em_extend_down_notify
 功能描述      : 
 输入参数      : char *ifname
 输出参数      :
 返 回 值  : 
 日    期    : 2020年8月24日
 作    者    : 洪桂阳
*****************************************************************************/
void td_em_extend_down_notify(unsigned int cfg)
{
    int data_len = 0;
    char *msg = NULL;
    char *value = NULL;

    data_len = sizeof(unsigned int) + sizeof(netlink_event_head_e) + NL_DATA_RSERVED; 
    msg = (char *)kmalloc(data_len, GFP_ATOMIC);
    
    if (!msg) {
        TD_EM_DBG_PARAM_ERR("malloc msg error!\n");
        return;
    }

    memset(msg, 0, data_len);
    value = msg;
    *value = TD_EASYMESH_EXTEND_DOWN;
    value = value + NETLINK_TYPE_LEN + NL_DATA_RSERVED;  
    *value = cfg;

    td_em_nl_send(msg, data_len, TD_EM_NLUSER_WSERVER);
    kfree(msg);

    return;
}

static int td_em_set_assoc_prio_flag(em_osif osif, unsigned char *buf)
{
    int ret = 0;

    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    ret = td_em_priority_assoc_flag_drv(osif, buf);

    return ret;
}

static int td_em_set_role(em_osif osif, unsigned char *buf)
{
    int ret = 0;

    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    ret = td_em_set_role_drv(osif, buf);

    return ret;
}

static int td_em_set_xmesh_enable(em_osif osif, unsigned char *buf)
{
    int ret = 0;

    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    ret = td_em_set_xmesh_enable_drv(osif, buf);

    return ret;
}

#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
int td_em_set_xmesh_hb(em_osif osif, unsigned char *buf)
{
    int ret = -1;

    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return ret;
    }

    ret = td_em_set_xmesh_hb_drv((em_osif)osif, buf);

    return ret;
}

int td_em_xmesh_set_bss(em_osif osif, unsigned char *buf)
{
    int ret = -1;

    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return ret;
    }

    ret = td_em_xmesh_set_bss_drv(osif, buf);

    return ret;
}
#endif

static int td_em_backhaul_sta_radio_capabilities_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_backhaul_sta_radio_capabilities(osif, result_buf);

    return 0;
}

static int td_em_ap_metric_query_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{

    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    td_em_get_mac_addr(osif, result_buf);

    *output_len = TD_EM_MACADDRLEN;

    TD_EM_DBG_MSG("result_buf[0] = %02x:%02x:%02x:%02x:%02x:%02x \n ", result_buf[0],result_buf[1],result_buf[2],
                                                                            result_buf[3],result_buf[4],result_buf[5]);
    return 0;
}

static int td_em_get_apmetric_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_apmetric(osif, result_buf);

    return 0;
}

static int td_em_get_assocstatrafficstats_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_assocstatrafficstats(osif, result_buf);

    return 0;
}

static int td_em_get_assocstalinkmetric_tlv(em_osif osif, unsigned char *result_buf, int sizeofbuf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_assocstalinkmetric(osif, result_buf, sizeofbuf);

    return 0;
}

static int td_em_get_extendedapmetric_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_extendedapmetric(osif, result_buf);

    return 0;
}

static int td_em_get_radiometric_tlv(em_osif osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_get_radiometric(osif, result_buf);

    return 0;
}

static int td_easymesh_get_associated_sta_extended_link_metrics_tlv(em_osif osif, unsigned char *result_buf, int sizeofbuf, int *output_len)
{
    int date_len = 0;

    date_len = sizeof(easymesh_assocsta_extended_link_metricstlv_t) + sizeof(easymesh_numof_bssid_for_agent_t);//暂时只考虑单频，后续多频组网还需完善。

    if (date_len > sizeofbuf) {
        TD_EM_DBG_PARAM_ERR("date_len > sizeofbuf!\n");
        return -1;
    }

    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (!td_easymesh_get_associated_sta_extended_link_metrics(osif, result_buf)) {
        *output_len = date_len;
    } else {
        memset(result_buf, 0, date_len);
        *output_len = 0;
    }

    return 0;
}

static int td_em_do_channelscan_tlv(void *osif, unsigned char *result_buf, int *output_len)
{
    if (!osif || !output_len || !result_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    *output_len = td_em_do_channelscan(osif);

    return 0;
}

void td_em_ap_updatechannelscanresults(void *osif)
{
    unsigned int date_len = 0;
    unsigned char *send_buf = NULL;
    td_em_scanresult_list_t s_scan_results_list = {0};

    if (!osif ) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    td_em_get_channelscanresults_tlv_lenth(osif, &date_len, (td_em_scanresult_list_t *)&s_scan_results_list);

    send_buf = (unsigned char *)kmalloc(date_len * sizeof(unsigned char), GFP_ATOMIC);

    if (NULL == send_buf) {
        TD_EM_DBG_PARAM_ERR("send_buf kmalloc error!\n");
        return;
    }

    memset(send_buf, 0, date_len);

    td_em_ap_save_channelscanresults(osif, send_buf, (td_em_scanresult_list_t *)&s_scan_results_list);

    td_em_nl_send(send_buf, date_len, TD_EM_NLUSER_EASYMESH);

    td_em_priv_malloc_free(osif, (td_em_scanresult_list_t *)&s_scan_results_list);

    kfree(send_buf);

    return;
}


void td_easymesh_beacon_metrics_rsp_notifiy_tlv(void *sta_if)
{
    int date_len = EM_MSG_MAX_BUFF_SIZE;
    unsigned char *send_buf = NULL;
    if (!sta_if) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return;
    }

    send_buf = (unsigned char *)kmalloc(EM_MSG_MAX_BUFF_SIZE, GFP_ATOMIC);//超过栈空间小，使用malloc分配空间
    if (!send_buf) {
        TD_EM_DBG_PARAM_ERR("malloc error!\n");
        return;
    }
    memset(send_buf, 0, EM_MSG_MAX_BUFF_SIZE);

    if (!td_easymesh_beacon_metrics_rsp_notifiy(sta_if, send_buf, &date_len)) {
        td_em_nl_send(send_buf, date_len, TD_EM_NLUSER_EASYMESH);
    }
/*
    for (i= 0; i < date_len; i++) {
        printk("%02x",send_buf[i]);  //后续测试需要，暂时保留
    }
    printk("\n");
*/
    kfree(send_buf);
    return ;
}


static int td_easymesh_1905_txlink_metric_tlv(em_osif osif, unsigned char *send_buf, int *output_len)
{
    if (!osif || !output_len || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (td_easymesh_1905_txlink_metric(osif, send_buf, output_len)) {
        return -1;
    }

    return 0;
}

static int td_easymesh_1905_rxlink_metric_tlv(em_osif osif, unsigned char *send_buf, int *output_len)
{
    if (!osif || !output_len || !send_buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (td_easymesh_1905_rxlink_metric(osif, send_buf, output_len)) {
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_beacon_metric_query
 功能描述  : beacon request 请求
 输入参数  : em_osif osif          
             unsigned char *buf  
 输出参数  : 无
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2020年5月29日
    作    者   : liyahui
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_beacon_metric_query(em_osif osif, unsigned char *buf)
{
    int ret = 0;

    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    ret = td_do_beacon_request(osif, buf);

    return ret;
}

static int td_em_eth_connect_notify(em_osif osif, unsigned char *buf)
{
    int ret = 0;
    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    ret = td_em_eth_connect_driver(osif, buf);

    return ret;
}


static int td_em_get_eth_status(em_osif osif, unsigned char *buf, int *output_len)
{
    int ret = 0;

    if (!osif || !buf || !output_len) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    ret = td_em_get_eth_connect_status(osif, buf , output_len);

    return ret;
}


static int td_em_sae_cap(em_osif osif, char *buf, int *output_len)
{
    if (!osif || !buf || !output_len) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (td_em_get_sae_cap(osif)) {
        *buf = 1;//1代表支持
    } else {
        *buf = 0;//0代表不支持
    }

    *output_len = 1;//输出长度,不论是否支持都应该有输出长度

    return 0;
}

int td_em_is_extend_intf_assoc(void *osif)
{
    if (!osif) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    return td_em_get_ext_assoc_status((em_osif)osif);
}

static int td_em_hostapd_notify(em_osif osif, char *buf)
{
    em_hostapd_msg_t *msg = (em_hostapd_msg_t *)buf;
    char bssmac[TD_EM_MACADDRLEN] = {0};

    if (!osif || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr!\n");
        return -1;
    }

    if (!memcmp(msg->macaddr, bssmac, TD_EM_MACADDRLEN)) {
        return 0;
    }

    td_em_get_mac_addr(osif, bssmac);

    td_em_client_event(__FUNCTION__,  __LINE__, osif, bssmac, msg->macaddr, msg->cmd, msg->reason, msg->state);

    return 0;
}

int msgtype_is_need_up_check(easymesh_op_e type)
{
    switch (type) {
        case EM_SAE_CAP:
        case EM_ETHX_CONNECT:
        case EM_XMESH_ENABLE:
            return 0;
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
        /* To deliver a child node role, down up does not need to be checked by the interface,
        So does the new heartbeat mechanism */
        case EM_XMESH_HEART_BEAT:
            return 0;
        case EM_ROLE:
            return 0;
#endif
        default:
            break;
    }    

    return 1;
}

/*****************************************************************************
 函 数 名  : td_easymesh_domsg
 功能描述  : easymesh ioctl消息分发
 输入参数  : em_osif osif          
             easymesh_op_e type  
             char *buf           
             int sizeofbuf       
             int *output_len     
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_easymesh_domsg(em_osif osif, easymesh_op_e type, char *buf, int sizeofbuf, int *output_len)
{
    int need = 0;

    if (!osif || !output_len || !buf) {
        TD_EM_DBG_PARAM_ERR("null ptr osif = 0x%p, output_len = %d, buf = 0x%p\n", osif, *output_len, buf);
        return -1;
    }

    need = msgtype_is_need_up_check(type); 
    if (need && !vap_is_up(osif)) {
        TD_EM_DBG_PARAM_ERR("vap is down\n");
        return -1;
    }
    
    switch (type) {
        case EM_ASSOC_CLIENT_INFO:
            return td_em_association_client_tlv(osif, (association_client_tlv_t *)buf, sizeofbuf, output_len);

        case EM_DEVICE_INFO:
            return td_em_one_device_info_tlv(osif, (one_device_info_tlv_t *)buf, output_len);

        case EM_AP_OPERATE_BSS:
            return td_em_ap_operational_bss_tlv(osif, (ap_operational_bss_tlv_t *)buf, output_len);

        case EM_AP_BASIC_CAP:
            return td_em_ap_radio_basic_cap_tlv(osif, (ap_radio_basic_cap_tlv_t *)buf, output_len);

        case EM_AP_RADIO_ID:
            return td_em_radio_identifier_tlv(osif, (radio_identifier_tlv_t *)buf, output_len);
        
        case EM_AUTO_CONFIG_M2:
            return td_em_autoconfig_m2_tlv(osif, (autoconfig_m2_tlv_t *)buf, output_len);

        case EM_SAE_CAP:
            return td_em_sae_cap(osif, buf, output_len);

        case EM_HOSTAPD_NOTIFY:
            return td_em_hostapd_notify(osif, buf);

        case EM_CHANNEL_PREFER:
            return td_channel_preference_report(osif, (channel_prefer_t *)buf, sizeofbuf,output_len);

        case EM_RADIO_OPERATE_RESTRICT:
            return td_radio_operate_restrict(osif, (radio_operation_restriction_t *)buf, sizeofbuf, output_len);

        case EM_CAC_COMPLETION_REPORT:
            return td_cac_completion_report(osif, buf, sizeofbuf, output_len);
              
        case EM_CAC_STATUS_REPORT:
            return td_cac_status_report(osif, buf, sizeofbuf, output_len);
          
        case EM_TRANSMIT_POWER_LIMIT:
            return td_transmit_power_limit(osif, (transmit_power_limit_t *)buf, sizeofbuf, output_len);

        case EM_CHANNEL_SELECT_RESPONSE:
            return td_channel_selection_response(osif, (channel_select_response_t *)buf, sizeofbuf, output_len);

        case EM_OPERATE_CHANNEL_REPORT:
            return td_operating_channel_report(osif, (operating_channel_report_t *)buf, sizeofbuf,output_len);
         
        case EM_CAC_REQUEST:
            return td_cac_request(osif, (cac_request_t *)buf, sizeofbuf,output_len);
         
        case EM_CAC_TERMINATION:
            return td_cac_termination(osif, (cac_termination_t *)buf, sizeofbuf,output_len);

        case EM_BACKHAUL_STEER_REQ:
            return td_backhaul_steering_request(osif, buf, output_len);

        case EM_BACKHAUL_STEER_RSP:
            return td_backhaul_steering_response(osif, NULL);

        case EM_STEER_POLICY:
            return td_steer_policy(osif, buf);

        case EM_CLIENT_STEER_REQUEST:
            return td_client_steer_request(osif, buf);

        case EM_CLIENT_ASSOCIAT_CONTROL_REQ:
            return td_client_assoc_sta_control(osif, buf);

        case EM_AP_CAPABILITY:
            return td_em_get_apcapability_tlv(osif, (unsigned char*)buf, output_len);

        case EM_AP_CAPABILITY_2:
            return td_em_get_apcapability_2_tlv(osif, (unsigned char*)buf, output_len);

        case EM_HT_AP_CAPABILITY:
            return td_em_get_htcapability_tlv(osif, (unsigned char*)buf, output_len);

        case EM_VHT_AP_CAPABILITY:
            return td_em_get_vhtapCapability_tlv(osif, (unsigned char*)buf, output_len);

        case EM_HE_AP_CAPABILITY:
            return td_em_get_heapcapability_tlv(osif, (unsigned char*)buf, output_len);

        case EM_CHSCAN_CAPABILITY:
            return td_em_get_channelscan_capabilities_tlv(osif, (unsigned char*)buf, sizeofbuf, output_len);

        case EM_BACKHAUL_CAPABILITY:
            return td_em_backhaul_sta_radio_capabilities_tlv(osif, (unsigned char*)buf, output_len);

        case EM_AP_METRIC_QUERY:
            return td_em_ap_metric_query_tlv(osif, (unsigned char*)buf, output_len);

        case EM_AP_METRICS:
            return td_em_get_apmetric_tlv(osif, (unsigned char*)buf, output_len);

        case EM_ASSOC_STA_TRAFFIC_STAT:
            return td_em_get_assocstatrafficstats_tlv(osif, (unsigned char*)buf, output_len);

        case EM_ASSOC_STA_LINK_METRICS:
            return td_em_get_assocstalinkmetric_tlv(osif, (unsigned char*)buf, sizeofbuf, output_len);

        case EM_AP_EXTENDED_METRICS:
            return td_em_get_extendedapmetric_tlv(osif, (unsigned char*)buf, output_len);

        case EM_RADIO_METRICS:
            return td_em_get_radiometric_tlv(osif, (unsigned char*)buf, output_len);
            
        case EM_ASSOC_STA_EXTEND_LINK_METRICS:
            return td_easymesh_get_associated_sta_extended_link_metrics_tlv(osif, (unsigned char*)buf, sizeofbuf, output_len);
            
        case EM_DO_CHANNEL_SCAN:
            return td_em_do_channelscan_tlv(osif, (unsigned char*)buf, output_len);
            
        case EM_1905_TXLINK_METRIC:
            return td_easymesh_1905_txlink_metric_tlv(osif, (unsigned char*)buf, output_len);
            
        case EM_1905_RXLINK_METRIC:
            return td_easymesh_1905_rxlink_metric_tlv(osif, (unsigned char*)buf, output_len);

        case EM_BEACON_METRICS_QUERY:
            return td_em_beacon_metric_query(osif, (unsigned char*)buf);

        case EM_ETHX_CONNECT:
            return td_em_eth_connect_notify(osif, (unsigned char*)buf);

        case EM_GET_ETH_STATUS:
            return td_em_get_eth_status(osif, (unsigned char*)buf, output_len);

        case EM_ASSOC_PRIO_FLAG:
            return td_em_set_assoc_prio_flag(osif, (unsigned char*)buf);

        case EM_ROLE:
            return td_em_set_role(osif, (unsigned char*)buf);

        case EM_XMESH_ENABLE:
            return td_em_set_xmesh_enable(osif, (unsigned char*)buf);

#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
        case EM_XMESH_HEART_BEAT:
            return td_em_set_xmesh_hb((void*)osif, (unsigned char*)buf);

        case EM_XMESH_SET_BSS:
            return td_em_xmesh_set_bss((void*)osif, (unsigned char*)buf);
#endif

        default:
            TD_EM_DBG_PARAM_ERR("default type = %d\n", type);
            return -1;
        break;
    }

    return 0;
}


/*****************************************************************************
 函 数 名  : td_easymesh_ioctl
 功能描述  : easymesh ioctl
 输入参数  : em_osif osif          
           char *arg
           int len
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年7月21日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_easymesh_ioctl(void *osif, char *arg, int len)
{
    em_io_frame_t io_frame = {0};
    int output_len = 0, ret = -1;
    char *tmpbuf = NULL;
    em_osif vap = (em_osif)osif;

    if (!osif || !arg) {
        TD_EM_DBG_PARAM_ERR("null ptr\n");
        goto out;
    }

    tmpbuf = (char *)kmalloc(EM_MSG_MAX_BUFF_SIZE, GFP_ATOMIC);
    if (!tmpbuf) {
        TD_EM_DBG_PARAM_ERR("malloc tmpbuf error\n");
        goto out;
    }
    memset(tmpbuf, 0, EM_MSG_MAX_BUFF_SIZE);

    if ((len != sizeof(io_frame)) || em_copy_from_user((void *)&io_frame, (void *)arg, sizeof(io_frame))) {  //接收应用层传下来的数据,1.用户数据地址 2.输入数据长度 3.帧类型
        TD_EM_DBG_PARAM_ERR("td copy from user error\n");
        goto out;
    }

    if (!io_frame.data || io_frame.input_len > EM_MSG_MAX_BUFF_SIZE) { 
        TD_EM_DBG_PARAM_ERR("Invalid ioctl data \n");
        goto out;
    }

    if (copy_from_user((void *)tmpbuf, (void *)io_frame.data, io_frame.input_len)) { //将用户输入的数据拷贝至内核空间(最大允许2048)
        TD_EM_DBG_PARAM_ERR("td copy from user error\n");
        goto out;
    }

    if (td_easymesh_domsg(vap, io_frame.type, tmpbuf, EM_MSG_MAX_BUFF_SIZE, &output_len)) {//进入td easymesh模块进行数据处理
        TD_EM_DBG_PARAM_ERR("td td easymesh ioctl fail\n");
        goto out;
    }
    
    if (output_len > 0 && output_len <= EM_MSG_MAX_BUFF_SIZE) { //有数据输出,则拷贝数据给用户
        io_frame.output_len = output_len;
        if (em_copy_to_user((void *)io_frame.data, (void *)tmpbuf, io_frame.output_len)) {
            TD_EM_DBG_PARAM_ERR("td copy to user error\n");
            goto out;

        }
        if (em_copy_to_user((void *)arg, (void *)&io_frame, sizeof(io_frame))) {
            TD_EM_DBG_PARAM_ERR("td io_frame copy to user error\n");
            goto out;
            
        }
    }
    ret = 0;
out:
    if (tmpbuf) {
        kfree(tmpbuf);
    }

    return ret;
}

/*****************************************************************************
 函 数 名  : td_em_copy_to_user
 功能描述  : 适配多种方案的拷贝动作
 输入参数  : void *dst 
           void *src, 
           int len  
 输出参数  : 无
 返 回 值  : 还未拷贝的字节数
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int em_copy_from_user(void *dst, void *src, int len)
{
    if (!dst || !src) {
        TD_EM_DBG_PARAM_ERR("null ptr\n");
        return -1;
    }
    
    if(access_ok(VERIFY_READ, src, len)) {
        return copy_to_user(dst, src, len);
    } else {
        memcpy(dst, src, len);
    }
    
    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_copy_to_user
 功能描述  : 适配多种方案的拷贝动作
 输入参数  : void *dst 
           void *src, 
           int len  
 输出参数  : 无
 返 回 值  : 还未拷贝的字节数
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int em_copy_to_user(void *dst, void *src, int len)
{
    if (!dst || !src) {
        TD_EM_DBG_PARAM_ERR("null ptr\n");
        return -1;
    }

    if(access_ok(VERIFY_WRITE, dst, len)) {
        return copy_to_user(dst, src, len);
    } else {
        memcpy(dst, src, len);
    }
    
    return 0;
}

/*****************************************************************************
 函 数 名  : td_easymesh_init
 功能描述  : 初始化easymesh模块
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_easymesh_init(void)
{
    int ret;

    td_em_dbg_init();
    ret = td_em_nl_init();

    return ret;
}

/*****************************************************************************
 函 数 名  : td_easymesh_exit
 功能描述  : 退出easymesh模块
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
void td_easymesh_exit(void)
{
    td_em_nl_exit();
    
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
    td_em_deinit_hbtimer();
#endif

    return;
}
/*lint  +e10*/
/*lint  +e161*/
/*lint  +e163*/
/*lint  +e101*/
/*lint  +e63*/
/*lint  +e26*/