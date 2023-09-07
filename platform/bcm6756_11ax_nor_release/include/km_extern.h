/***********************************************************
    Copyright (C), 1998-2017, Tenda Tech. Co., Ltd.
    FileName: km_extern.h
    Description: 内核组向产品组提供的公共接口头文件
    Version : 1.0
    Date: 2017.11.9
    Function List:
    km_sys_proc_set
    History:
    <author>   <time>     <version >   <desc>
    hjl        2017-11-9   1.0        new
************************************************************/
#ifndef KM_EXTERN_H
#define KM_EXTERN_H

#include "common_extern.h"
#include "common_cjson.h"

/*---------------System--Proc--Configuration--Interface---------------*/
//提供给产品的proc参数的宏
#define PROC_MAX_CONN  "max_conn"

/*****************************************************************************
 函 数 名  : km_sys_proc_set
 功能描述  : 设置单个proc参数
 输入参数  : ProcName: proc参数对应的宏
                               ProcValue: proc参数对应的值
 输出参数  : 无
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_sys_proc_set( const char* ProcName, const char* ProcValue );







/*---------------Ip--Group--Rule--Interface---------------*/

/*****************************************************************************
 函 数 名  : km_ip_group_add_rule
 功能描述  : 添加ip组规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月20日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/

/*
config_obj数据实例（ip组添加、修改）

{
    "IpGroup":[
        {
            "IpGroupId": 1,
            "IpSegment":[
                {
                    "StartIp":"192.168.0.100",
                    "EndIp":"192.168.0.200"
                },
                {
                    "StartIp":"192.168.1.100",
                    "EndIp":"192.168.3.200"
                }
            ]
        },
        {
            "IpGroupId": 2,
            "IpSegment":[
                {
                    "StartIp":"192.168.4.100",
                    "EndIp":"192.168.4.200"
                },
                {
                    "StartIp":"192.168.5.100",
                    "EndIp":"192.168.8.200"
                }
            ]
        }
    ]
}

*/
UGW_RETURN_CODE_ENUM  km_ip_group_add_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_ip_group_update_rule
 功能描述  : 更新ip组规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月20日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM  km_ip_group_update_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_ip_group_del_rule
 功能描述  : 删除ip组规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月20日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/
/*
config_obj数据实例（ip组添加、修改）

{
    "IpGroupId":[
            1，
            2
    ]
}

*/
UGW_RETURN_CODE_ENUM  km_ip_group_del_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_ip_group_clear_rule
 功能描述  : 清空已下发的ip组规则
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月20日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/

UGW_RETURN_CODE_ENUM km_ip_group_clear_rule();






/*---------------Mac--Group--Rule--Interface---------------*/

/*****************************************************************************
 函 数 名  : km_mac_group_add_rule
 功能描述  : 添加mac组规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月15日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
/*
config_obj数据实例（mac组添加、修改）

{
"MacInfoArray":[
        {
            "MacGroupId":1,
            "Mac":["6c:4b:90:3e:ab:f2","6c:4b:90:3e:ab:bb"]
        },
        {
            "MacGroupId":2,
            "Mac":["6c:4b:90:3e:ab:f4","6c:4b:90:3e:ab:bd"]
        },
        {
            "MacGroupId":3,
            "Mac":["6c:4b:90:3e:ab:f6","6c:4b:90:3e:ab:be"]
        }
    ]
}

*/
UGW_RETURN_CODE_ENUM  km_mac_group_add_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_mac_group_update_rule
 功能描述  : 更新mac组规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月15日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM  km_mac_group_update_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_mac_group_del_rule
 功能描述  : 删除mac组规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月15日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
/*

config_obj数据实例（mac组删除）

{
    "MacGroupId":[5,3,2,1,4]
}

*/

UGW_RETURN_CODE_ENUM  km_mac_group_del_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_mac_group_clear_rule
 功能描述  : 清空已下发的mac组规则
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月15日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_mac_group_clear_rule();









/*---------------Url--Filter--Rule--Interface---------------*/

/*****************************************************************************
 函 数 名  : km_url_filter_add_global_url_cfg
 功能描述  : 解析全局url配置文件并将全局url规则下发到内核
 输入参数  : 全局url配置文件路径和文件名
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_add_global_url_cfg ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_url_filter_add_define_url_cfg
 功能描述  : 添加url  自定义配置
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_add_define_url_cfg ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_url_filter_update_define_url_cfg
 功能描述  : 更新url  自定义配置
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_update_define_url_cfg ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_url_filter_del_define_url_cfg
 功能描述  : 删除url  自定义配置
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_del_define_url_cfg ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_url_filter_add_rule
 功能描述  : 添加url  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_add_rule( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_url_filter_update_rule
 功能描述  : 更新url  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM  km_url_filter_update_rule( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_url_filter_del_rule
 功能描述  : 删除url  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM  km_url_filter_del_rule( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_url_filter_clear_global_url_cfg
 功能描述  : 清空已下发的全局url  配置规则
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_clear_global_url_cfg();

/*****************************************************************************
 函 数 名  : km_url_filter_clear_define_url_cfg
 功能描述  : 清空已下发的自定义url  配置规则
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_clear_define_url_cfg();

/*****************************************************************************
 函 数 名  : km_url_filter_clear_rule
 功能描述  : 清空已下发的url  过滤规则
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_clear_rule();

/*****************************************************************************
 函 数 名  : km_url_filter_enable
 功能描述  : url  过滤模块开关(开启)
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_enable();

/*****************************************************************************
 函 数 名  : km_url_filter_disable
 功能描述  : url  过滤模块开关(关闭)
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月9日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_disable();
/*****************************************************************************
 Prototype    : km_url_filter_set_lan_ip
 Description  : 设置lan口ip地址
 Input        : config_obj
 Output       : None
 Return Value : 失败 UGW_ERR 成功 UGW_OK

  History        :
  1.Date         : 2018/10/18
    Author       : tangyuansheng
    Modification : Created function

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_set_lan_ip( cJSON * config_obj );

/*****************************************************************************
 Prototype    : km_url_filter_update_lan_ip
 Description  : 更新lan口ip地址
 Input        : config_obj
 Output       : None
 Return Value : 失败 UGW_ERR 成功 UGW_OK

  History        :
  1.Date         : 2018/10/18
    Author       : tangyuansheng
    Modification : Created function

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_url_filter_update_lan_ip( cJSON * config_obj );

/*与应用层同步自定义与全局网址组id的使用，避免更新网址库大小对业务影响*/
#define URL_FILTER_MAX_GLOBAL_COUNT     159          /*可使用1~159*/
#define URL_FILTER_MAX_DEFINE_COUNT     30           /*可使用1~30*/
#define URL_GROUP_ID_MAX                (URL_FILTER_MAX_GLOBAL_COUNT + URL_FILTER_MAX_DEFINE_COUNT)










/*----------------Mac-Filter--Rule--Interface---------------*/

/*****************************************************************************
 函 数 名  : km_mac_filter_add_rule
 功能描述  : 添加mac  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
/*
    config_obj数据实例（mac过滤添加、删除、修改）

    {
            "Mac":[
                "b0:83:fe:b5:9b:6f",
                "00:e0:4c:6c:9b:58"
            ]
     }
 */
UGW_RETURN_CODE_ENUM km_mac_filter_add_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_mac_filter_update_rule
 功能描述  : 更新mac  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_mac_filter_update_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_mac_filter_del_rule
 功能描述  : 删除mac  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_mac_filter_del_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_mac_filter_clear_rule
 功能描述  : 清空已下发的mac  过滤规则
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_mac_filter_clear_rule ();

/*****************************************************************************
 函 数 名  : km_mac_filter_set_mode
 功能描述  : 是否允许未启用规则和列表外的主机访问互联网
 输入参数  : config_obj         0：不允许；1：允许
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_mac_filter_set_mode ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_mac_filter_set_type
 功能描述  : 是黑名单还是白名单
 输入参数  : config_obj         0：黑名单；1：白名单
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_mac_filter_set_type ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_mac_filter_enable
 功能描述  : mac  过滤模块开关(开启)
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_mac_filter_enable ();

/*****************************************************************************
 函 数 名  : km_mac_filter_disable
 功能描述  : mac  过滤模块开关(关闭)
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_mac_filter_disable ();









/*----------------App-Filter--Rule--Interface---------------*/

/*****************************************************************************
 函 数 名  : km_app_filter_add_rule
 功能描述  : 添加app  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_app_filter_add_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_app_filter_update_rule
 功能描述  : 更新app  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_app_filter_update_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_app_filter_del_rule
 功能描述  : 删除app  过滤规则
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_app_filter_del_rule ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_app_filter_clear_rule
 功能描述  : 清空已下发的app  过滤规则
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_app_filter_clear_rule ();

/*****************************************************************************
 函 数 名  : km_app_filter_enable
 功能描述  : app  过滤模块开关(开启)
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_app_filter_enable ();

/*****************************************************************************
 函 数 名  : km_app_filter_disable
 功能描述  : app  过滤模块开关(关闭)
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月13日
    作    者   : hjl
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_app_filter_disable ();









/*----------------fastpath-switch--Interface---------------*/

/*****************************************************************************
 函 数 名  : km_fastpath_sa_enable
 功能描述  :
             1、启用NAT、桥加速功能；
             2、启用NAT加速，wan口static、dhcp、pppoe接入都会进行加速；
             3、启用桥加速，wlan2lan加功能，如果桥支持PKTC加速也会同时启用；
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月21日
    作    者   : kuangdaozhen
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_fastpath_sa_enable();

/*****************************************************************************
 函 数 名  : km_fastpath_sa_disable
 功能描述  : 关闭所有软加速功能
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月21日
    作    者   : kuangdaozhen
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_fastpath_sa_disable();

/*****************************************************************************
 函 数 名  : km_fastpath_sa_slow_xmit_enable
 功能描述  : 开启fastnat慢速转发功能，进行流量整形
 输入参数  : 无
 输出参数  : 无
 返 回 值  : UGW_RETURN_CODE_ENUM

 修改历史      :
  1.日    期   : 2018年11月4日
    作    者   : 黄嘉乐
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_fastpath_sa_slow_xmit_enable();

/*****************************************************************************
 函 数 名  : km_fastpath_sa_slow_xmit_disable
 功能描述  : 关闭fastnat慢速转发功能，进行流量整形
 输入参数  : 无
 输出参数  : 无
 返 回 值  : UGW_RETURN_CODE_ENUM

 修改历史      :
  1.日    期   : 2018年11月4日
    作    者   : 黄嘉乐
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_fastpath_sa_slow_xmit_disable();

/*****************************************************************************
 函 数 名  : km_fastpath_sa_config
 功能描述  :
             1、开启或者关闭桥加速功能；
             2、开启或者关闭桥PKTC聚合功能；
             3、开启或者关闭pptp加速功能；
             4、开启或者关闭l2tp加速功能；
             5、开启或者关闭pppoe server加速功能；
             6、每个选项不是必须的，json格式中没有该选项就保持默认
 输入参数  : json数据格式
 {
     "Bridge":{
         "Enable":1,
         "PktcEnable":0
     } ,
     "VPN":{
         "PptpEnable":0,
         "L2tpEnable":1
     },
     "PppoeServerEnable":0
 }
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月21日
    作    者   : kuangdaozhen
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_fastpath_sa_config(cJSON *config_obj);

/*****************************************************************************
 函 数 名  : km_fastpath_hw_config
 功能描述  : 关闭硬加速软加速接口
 输入参数  : json数据格式
{
    "Reset":0,                  // for RTK, A、B类地址不支持加速
    "Enable":true,              // for RTK, 开关加速
    "WirelessEnable":true,      // for RTK, 控制wlan到wan加速开关
    "Type":[                    // for BCM, case代表服务开启状态
        {"case":1, "value":1},  // 如果需要关闭IPV6加速，可以添加
        {"case":2, "value":1},  // {"case":15, "value":1}字段即可
    ]
}
## 序号依次排列 ##
    KM_DEFAULT_STATUS  = 0,      //默认状态
    KM_LAN_ADDRESS_ILLEGAL,      //某些RTK产品硬件加速不支持AB类地址
    KM_TRAFFIC_STATISTICS,       //产品需要打开页面显示流量
    KM_HW_SWITCH_STATUS,         //某些产品hw开关默认关闭
    KM_WAN_CONNECT_PPPOE_DOUBLE, //wan口pppoe+dhcp/static双接入
    KM_WAN_CONNECT_PPTP_DOUBLE,  //wan口pptp+dhcp/static双接入
    KM_WAN_CONNECT_L2TP_DOUBLE,  //wan口l2tp+dhcp/static双接入
    KM_WAN_CONNECT_DOUBLE,       //wan口双接入统称
    KM_PPTP_CLIENT_ENABLE,       //开启pptp客户端
    KM_PPTP_SERVICE_ENABLE,      //开启pptp服务器
    KM_L2TP_CLIENT_ENABLE,       //开启l2tp客户端
    KM_L2TP_SERVICE_ENABLE,      //开启l2tp服务器
    KM_QOS_ENABLE,               //开启qos
    KM_NAT66_ENABLE,             //开启NAT66
    KM_BRIDGE_SYSMODE,           //系统为桥模式
    KM_IPV6_ENABLE,              //ipv6开启，提供isp使用
    KM_IPV4_ENABLE               //ipv4开启，提供isp使用

 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2017年11月21日
    作    者   : kuangdaozhen
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_fastpath_hw_config(cJSON * config_obj);


/*----------------Load_balance--Interface---------------*/

/*****************************************************************************
 函 数 名  : km_load_balance_add_wan_element
 功能描述  : 添加wan口信息
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月8日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/

/*
config_obj数据实例（wan口信息添加）
{
    "WanElement":[
        {
            "WanId":1,
            "BandWidth":{
                Up:100
                Down:100
            },
            "IfName":"vlan2",
            "InternetStatus":2
        },
        {
            "WanId":2,
            "BandWidth":{
                Up:100
                Down:100
            },
            "IfName":"vlan3",
            "InternetStatus":1
        }
    ]
}

*/
UGW_RETURN_CODE_ENUM km_load_balance_add_wan_element ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_load_balance_update_wan_element
 功能描述  : 更新wan口信息
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月8日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/

/*
config_obj数据实例（wan口信息更新）
{
    "WanElement":[
        {
            "WanId":1,
            "BandWidth":{
                Up:100
                Down:100
            },
            "IfName":"vlan2",
            "InternetStatus":2
        },
        {
            "WanId":2,
            "BandWidth":{
                Up:100
                Down:100
            }
        }
    ]
}

*/

UGW_RETURN_CODE_ENUM km_load_balance_update_wan_element ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_load_balance_del_wan_element
 功能描述  : 删除wan口信息
 输入参数  : config_obj
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月8日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/
/*
config_obj数据实例（wan口信息删除）
{
    "WanId":[
        1,
        2
    ]
}

*/
UGW_RETURN_CODE_ENUM km_load_balance_del_wan_element ( cJSON * config_obj );

/*****************************************************************************
 函 数 名  : km_load_balance_set_mode
 功能描述  : 设置wan口负载均衡的模式
 输入参数  : config_obj         0：关闭负载均衡；1：开启负载均衡；
                                2: 开启基于应用的负载均衡
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月8日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/
/*
config_obj数据实例
{
    "Mode":2
}

*/
UGW_RETURN_CODE_ENUM km_load_balance_set_mode ( cJSON * config_obj );





UGW_RETURN_CODE_ENUM km_qos_set_global ( cJSON * config_obj) ;
UGW_RETURN_CODE_ENUM km_qos_enable() ;
UGW_RETURN_CODE_ENUM km_qos_disable();
UGW_RETURN_CODE_ENUM km_qos_set_valid_wan(cJSON * config_obj) ;
UGW_RETURN_CODE_ENUM km_qos_set_group_rule(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_qos_set_single_mode(cJSON * config_obj) ;
UGW_RETURN_CODE_ENUM km_qos_add_single_rule(cJSON * config_obj) ;
UGW_RETURN_CODE_ENUM km_qos_upd_single_rule(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_qos_del_single_rule(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_qos_flush_single_rule();
UGW_RETURN_CODE_ENUM km_qos_set_priority(cJSON * config_obj);

UGW_RETURN_CODE_ENUM km_ddos_ip_set_lan_info( cJSON * config_obj ) ;
UGW_RETURN_CODE_ENUM km_ddos_ip_enable(void) ;
UGW_RETURN_CODE_ENUM km_ddos_ip_disable(void) ;

UGW_RETURN_CODE_ENUM km_ddos_ip_set_ip_option(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_ddos_ip_get_ip_option_attack_info(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_ddos_ip_clear_ip_option_attack_info(void) ;

UGW_RETURN_CODE_ENUM km_ddos_ip_set_ddos(cJSON * ddos_obj);
UGW_RETURN_CODE_ENUM km_ddos_ip_get_ddos_attack_info(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_ddos_ip_clear_ddos_attack_info(void);

UGW_RETURN_CODE_ENUM km_ddos_ip_get_invalid_pkt_info(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_ddos_ip_clear_invalid_pkt_info(void);

UGW_RETURN_CODE_ENUM km_arp_fence_set_lan_info( cJSON * config_obj );
UGW_RETURN_CODE_ENUM km_arp_fence_enable(void) ;
UGW_RETURN_CODE_ENUM km_arp_fence_disable(void) ;
UGW_RETURN_CODE_ENUM km_arp_fence_clear(void) ;
UGW_RETURN_CODE_ENUM km_arp_fence_get_info(cJSON * attack_info_obj);
UGW_RETURN_CODE_ENUM km_arp_fence_set_broadcast_info(cJSON * config_obj);

UGW_RETURN_CODE_ENUM km_get_online_ip(cJSON *online_ip_obj);
UGW_RETURN_CODE_ENUM km_online_ip_set_count(cJSON *config_obj);


/*
date:2018-2-5
author:kuangdaozhen
desc:
    km_wan_del_interface_info和km_wan_add_interface_info主要用来配置无线wan(wisp模式)和
    和有线wan口相关信息
    cjson格式如下
    {
        Wireless:["ath8","wifi0"],
        Wire:["vlan2","vlan3"],
    }
*/
UGW_RETURN_CODE_ENUM km_wan_del_interface_info(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_wan_add_interface_info(cJSON * config_obj);

/*****************************************************************************
 函 数 名  : km_wan_statistic_get_wan_flow
 功能描述  : 上报wan口流量信息
 输入参数  : none
 输出参数  : none
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK

 修改历史      :
  1.日    期   : 2018年8月10日
    作    者   : luxibao
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM km_wan_statistic_get_wan_flow ( cJSON * wan_flow_obj );
/*
{
"WanFlow":[
    {
        "WanId":1,
        "WanRate":
        {
            "UpRate":100,
            "DownRate":200
        },
        "WanByte":
        {
            "UpByte":3000,
            "DownByte":4000
        }
    },
    {
        "WanId":2,
        "WanRate":
        {
            "UpRate":300,
            "DownRate":400
        },
        "WanByte":
        {
            "UpByte":5000,
            "DownByte":6000
        }
    },
    {
        "WanId":3,
        "WanRate":
        {
            "UpRate":0,
            "DownRate":0
        },
        "WanByte":
        {
            "UpByte":0,
            "DownByte":0
        }
    },
    {
        "WanId":4,
        "WanRate":
        {
            "UpRate":0,
            "DownRate":0
        },
        "WanByte":
        {
            "UpByte":0,
            "DownByte":0
        }
    }
  ]
}

*/

/* 上报wan口各种协议的链接数量信息 */
UGW_RETURN_CODE_ENUM km_wan_statistic_get_wan_connect_count ( cJSON * wan_connect_count_obj );
/*
{
"WanConnectStatistic":[
    {
        "WanId":1,
        "WanConnectCount":
        {
            "TcpCount":100,
            "UdpCount":200
        }
    },
    {
        "WanId":2,
        "WanConnectCount":
        {
            "TcpCount":300,
            "UdpCount":400
        }
    }
  ]
}
*/

/*
date:2018-9-3
author:huangjiale
desc:
    提供给产品及km接口层内部使用的用户层netlink通信接口
*/
//提供出去的用户ID

enum
{
    KM_NETLINK_PHYCHECK = 79,    /*提供给bsp phy检查的netlink号*/
    KM_NETLINK_WLAN = 89,        /*提供给无线组的netlink号*/
    KM_NETLINK_AUTODISCOVER = 95,/*提供给应用层autodiscover进程的netlink号*/
};

typedef enum SOCKET_RECIVE_RET
{
    RECV_RET_CONTINUE = 0,        /* 继续接受数据 */
    RECV_RET_STOP = 1,            /* 关闭套接字 */
    RECV_RET_BUTT
}SOCKET_RECIVE_RET_ENUM;

//提供出去的函数模型
typedef SOCKET_RECIVE_RET_ENUM (*netlink_callback)(void *);

//提供出去的接口
int km_netlink_user_socket_init(int msg_type);
int km_netlink_user_socket_bind_init(int msg_type);
UGW_VOID km_netlink_user_socket_deinit(int sock);
UGW_RETURN_CODE_ENUM km_netlink_user_send_request(int sock, int msg_type,
    void *buffer, int buff_size);
UGW_RETURN_CODE_ENUM km_netlink_user_rcvmsg(int sock, void **buffer, int buff_size);
UGW_RETURN_CODE_ENUM km_netlink_user_get_info_from_kernel(int msg_type,
    u16 request_type, void **info, int info_size);
UGW_RETURN_CODE_ENUM km_netlink_user_send_message(int msg_type,
    u16 request_type, void *message, int message_size);
UGW_RETURN_CODE_ENUM km_netlink_user_loop_rcvmsg(int msg_type,
    int buff_size, netlink_callback deal);


/*
date:2018-9-10
author:gaobaoliang
desc:
    提供给产品使用的portal模块接口
*/
UGW_RETURN_CODE_ENUM  km_portal_add_white_list( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_portal_del_white_list( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_portal_clear_white_list( cJSON * config_obj);
UGW_RETURN_CODE_ENUM  km_portal_add_auth_user( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_portal_update_auth_user( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_portal_del_auth_user( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_portal_clear_auth_user();
UGW_RETURN_CODE_ENUM  km_portal_add_interface_auth_info (cJSON* config_obj );
UGW_RETURN_CODE_ENUM  km_portal_update_interface_auth_info(cJSON* config_obj );
UGW_RETURN_CODE_ENUM  km_portal_del_interface_auth_info(cJSON* config_obj );
UGW_RETURN_CODE_ENUM  km_portal_clear_interface_auth_info();
UGW_RETURN_CODE_ENUM  km_portal_set_expire_time( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_portal_set_aging_time( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_portal_enable();
UGW_RETURN_CODE_ENUM  km_portal_disable();
int  km_portal_user_rcv_msg_init();
UGW_RETURN_CODE_ENUM  km_portal_user_rcv_msg(int socket, cJSON *user_obj);

/*
date:2018-11-6
author:gaobaoliang
desc:
    提供给产品使用的ssid portal 接口
*/

UGW_RETURN_CODE_ENUM  km_dhcp_options_updata_client_info(cJSON* config_obj);
UGW_RETURN_CODE_ENUM  km_portal_accesspoint_auth_disable();
UGW_RETURN_CODE_ENUM  km_portal_accesspoint_auth_enable();
/*
date:2018-9-30
author:yangnana
desc:
    提供给产品使用的dns重定向接口
*/

UGW_RETURN_CODE_ENUM  km_dns_redirect_add_rule ( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_dns_redirect_del_rule ( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_dns_redirect_update_rule ( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_dns_redirect_clear_rule ();
UGW_RETURN_CODE_ENUM  km_dns_redirect_enable();
UGW_RETURN_CODE_ENUM  km_dns_redirect_disable ();

/*
date:2018-10-25
author:wangjianqiang
desc:
    提供给产品的interface_isolate接口
*/
UGW_RETURN_CODE_ENUM km_interface_isolate_add_rule(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_interface_isolate_del_rule(cJSON* config_obj);
UGW_RETURN_CODE_ENUM km_interface_isolate_clear_rule(void);
UGW_RETURN_CODE_ENUM km_interface_isolate_enable(void);
UGW_RETURN_CODE_ENUM km_interface_isolate_disable(void);
UGW_RETURN_CODE_ENUM km_ssid_isolate_enable(void);
UGW_RETURN_CODE_ENUM km_ssid_isolate_disable(void);
UGW_RETURN_CODE_ENUM km_ssid_isolate_add_rule(cJSON* config_obj);
UGW_RETURN_CODE_ENUM km_ssid_isolate_del_rule(cJSON* config_obj);
UGW_RETURN_CODE_ENUM km_ssid_isolate_clear_rule();

/*
date:2018-10-19
author:yuxiao
desc:
    提供给产品的dhcp_options接口
*/
UGW_RETURN_CODE_ENUM km_dhcpopt_enable();
UGW_RETURN_CODE_ENUM km_dhcpopt_disable();
UGW_RETURN_CODE_ENUM km_dhcpopt_set_guest_network_ifname(cJSON *config_obj);
int km_dhcpopt_rcv_msg_init();
UGW_RETURN_CODE_ENUM km_dhcpopt_rcv_msg(int sock, cJSON *user_obj);
UGW_VOID km_dhcpopt_socket_release(int sock);

/*
date:2018-11-11
author:yangnana
desc:
    提供给产品使用的广播组播报文过滤接口
*/

UGW_RETURN_CODE_ENUM  km_multi_broad_filter_add_rule ( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_multi_broad_filter_del_rule ( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_multi_broad_filter_set_mode ( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_multi_broad_filter_clear_rule ();
UGW_RETURN_CODE_ENUM  km_multi_broad_filter_enable();
UGW_RETURN_CODE_ENUM  km_multi_broad_filter_disable ();
/*
date:2018-11-5
author:yuxiao
desc:
    提供给产品的client_os_identify接口
*/
UGW_RETURN_CODE_ENUM km_os_identify_enable();
UGW_RETURN_CODE_ENUM km_os_identify_disable();
int km_os_identify_rcv_msg_init();
UGW_RETURN_CODE_ENUM km_os_identify_rcv_msg(int sock, cJSON *user_obj);
UGW_VOID km_os_identify_socket_release(int sock);


/*
date:2018-11-24
author:yangnana
desc:
    提供给产品使用的喂狗接口
*/
UGW_RETURN_CODE_ENUM km_watchdog_add_rule(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_watchdog_enable();
UGW_RETURN_CODE_ENUM km_watchdog_disable();
UGW_RETURN_CODE_ENUM km_watchdog_feed_dog();

/*
date:2018-12-17
author:gaobaoliang
desc:
用于二层行为管理，提供应用层下发规则的接口
*/

UGW_RETURN_CODE_ENUM km_bm_add_local_white_list (cJSON* config_obj);
UGW_RETURN_CODE_ENUM km_bm_clear_local_white_list (cJSON* config_obj);
UGW_RETURN_CODE_ENUM km_bm_set_redirect_info (cJSON* config_obj);

/*
date:2019-1-17
author:yangnana
desc:
提供给应用层串口打印控制的借口
*/

UGW_RETURN_CODE_ENUM km_console_control(cJSON * config_obj);

/* 应用层操作串口动作与内核保持一致，由内核统一提供 */
#define KM_CONSOLE_DISABLE_ALL 0
#define KM_CONSOLE_ENABLE_ALL 1
#define KM_CONSOLE_ENABLE_KERNEL 2
#define KM_CONSOLE_DISABLE_KERNEL 3
#define KM_CONSOLE_ENABLE_TTY 4
#define KM_CONSOLE_DISABLE_TTY 5

UGW_RETURN_CODE_ENUM km_fastpath_hw_flow_flush(void);
UGW_RETURN_CODE_ENUM km_nf_conntrack_flush(void);
UGW_RETURN_CODE_ENUM km_l2_nf_conntrack_disable(void);
UGW_RETURN_CODE_ENUM km_l2_nf_conntrack_enable(void);
/*
date:2020-6-15
author:chengcongming
desc:
用于PPPOE认证使用，提供给应用下发相关的规则信息
*/
UGW_RETURN_CODE_ENUM  km_pppoe_auth_add_redirect_auth_info(cJSON *config_obj);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_del_white_list(cJSON *config_obj);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_clear_ip_white_list(void);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_clear_mac_white_list(void);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_add_white_list(cJSON *config_obj);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_clear(void);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_delete(cJSON *config_obj);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_update(cJSON *config_obj);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_add(cJSON *config_obj);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_disable(void);
UGW_RETURN_CODE_ENUM  km_pppoe_auth_enable(void);

/*
date:2020-05-08
author:chengcongming
desc:
提供给应用层使用super_user模块的接口
*/
UGW_RETURN_CODE_ENUM  km_super_user_disable(void);
UGW_RETURN_CODE_ENUM  km_super_user_enable(void);
UGW_RETURN_CODE_ENUM  km_super_user_clear_rule(void);
UGW_RETURN_CODE_ENUM  km_super_user_del_rule(cJSON *config_obj);
UGW_RETURN_CODE_ENUM  km_super_user_add_rule ( cJSON *config_obj);

/*
date:2020-09-09
author:pangxinyou
desc:
提供给应用层使用bridge_inhibit模块的接口
*/
UGW_RETURN_CODE_ENUM  km_bridge_inhibit_enable(void);
UGW_RETURN_CODE_ENUM  km_bridge_inhibit_disable(void);

/*
date:2020-10-22
author:yx
desc:
提供给应用层使用多频汇聚模块的接口
*/
UGW_RETURN_CODE_ENUM  km_multifreq_lb_enable(void);
UGW_RETURN_CODE_ENUM  km_multifreq_lb_disable(void);
UGW_RETURN_CODE_ENUM  km_multifreq_lb_set_threshold(cJSON *config_obj);
UGW_RETURN_CODE_ENUM  km_multifreq_lb_set_weight(cJSON *config_obj);

/* 提供给应用层使用Http重定向模块的接口 */
UGW_RETURN_CODE_ENUM  km_http_redirect_disable(void);
UGW_RETURN_CODE_ENUM  km_http_redirect_enable (void);
UGW_RETURN_CODE_ENUM  km_http_redirect_clear_rule (void);
UGW_RETURN_CODE_ENUM  km_http_redirect_update_rule( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_http_redirect_del_rule ( cJSON * config_obj );
UGW_RETURN_CODE_ENUM  km_http_redirect_add_rule ( cJSON * config_obj );

/*提供给应用层使用link loop时的接口*/
UGW_RETURN_CODE_ENUM km_link_loop_add_vlan_rule(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_link_loop_update_vlan_rule(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_link_loop_del_vlan_rule(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_link_loop_clear_vlan_rule(void);
UGW_RETURN_CODE_ENUM km_link_loop_update_config(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_link_loop_add_global_config(cJSON * config_obj);
UGW_RETURN_CODE_ENUM km_link_loop_enable(void);
UGW_RETURN_CODE_ENUM km_link_loop_disable(void);

UGW_RETURN_CODE_ENUM km_net_topo_get_devs_info(cJSON *root);
UGW_RETURN_CODE_ENUM km_net_topo_get_pass_route_info(cJSON *root);
UGW_RETURN_CODE_ENUM km_net_topo_set_valid_interface(cJSON *root);
UGW_RETURN_CODE_ENUM km_net_topo_set_server_info(cJSON *root);
UGW_RETURN_CODE_ENUM km_net_topo_get_server_in_which_port(cJSON *root);

/*
date:2021-09-15
author:lzb
desc:
提供给应用层添加和删除集中转发隧道的接口
*/
UGW_RETURN_CODE_ENUM km_tf_add_tunnel(cJSON *config_obj);
UGW_RETURN_CODE_ENUM km_tf_del_tunnel(cJSON *config_obj);

/*
date:2022-03-26
author:jpj
desc:
提供给应用层添加流识别模块规则的接口
*/
UGW_RETURN_CODE_ENUM km_flow_identify_add_rules(cJSON *config_obj);


/*
date:2022-09-06
author:ynn
desc:
提供给应用层添加设备测速规则的接口
*/
/*****************************************************************************
 功能描述  : 配置设备间进行发送配置
 输入参数  : config_obj
    {
        dstip:<ipaddr> -string      测速的目的ip
        dstport:<portnum> - int     测速报文的目的端口
        srcnic:<nicname> - string   测速报文的源网口名
        time_sec:<seconds> - int    测速时间，-1 表示永久  >0 : 测速时长秒
    }
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK
*******************************************************************************/
UGW_RETURN_CODE_ENUM km_spdtst_config_tx(cJSON *config_obj);

/*****************************************************************************
 功能描述  : 配置设备间进行接收配置
 输入参数  : config_obj
    {
        dstport:<portnum> - int     测速报文的目的端口
        time_sec:<seconds> - int    测速时间，-1 表示永久  >0 : 测速时长秒
    }
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK
*******************************************************************************/
UGW_RETURN_CODE_ENUM km_spdtst_config_rx(cJSON *config_obj);

/*****************************************************************************
 功能描述  : 配置设备间控制配置接口
 输入参数  : config_obj
    {
        cmd:<cmdtype> - int     
            SPDTST_CMD_STARTTX - 启动测速发送
            SPDTST_CMD_STARTRX - 启动测速接收
            SPDTST_CMD_STOPTST - 停止测速发送或接收
    }
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK
*******************************************************************************/
UGW_RETURN_CODE_ENUM km_spdtst_control(cJSON *config_obj);

/*****************************************************************************
 功能描述  : 获取设备间测速的测速结果
 输入参数  : msg_len - 消息缓冲区msg的长度
 输出参数  : msg - 消息缓冲区，目前接口只需要传入一个32bit变量的首地址
    
 返 回 值  : 失败 UGW_ERR 成功 UGW_OK
*******************************************************************************/
UGW_RETURN_CODE_ENUM km_spdtst_get_speed(void *msg, u32 msg_len);

/*****************************************************************************
 功能描述  : 获取设备间测速模块的当前配置以及测速状态
 输入参数  : 无
 输出参数  : respons - 以json对象返回测速配置和测速状态，上层调用需要释放该json对象
 {
    status:<statcode>
    config:<string>
    dstport:<portnum>
    time_sec:<seconds>
 }

 返 回 值  : 失败 UGW_ERR 成功 UGW_OK
*******************************************************************************/
UGW_RETURN_CODE_ENUM km_spdtst_get_status(cJSON **respons);

#endif
