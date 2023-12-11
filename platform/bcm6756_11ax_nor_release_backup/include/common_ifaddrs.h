/*     $Id: ifaddrs.h,v 1.1.1.1 2003/01/16 15:41:11 root Exp $    */
/*    from USAGI: ifaddrs.h,v 1.1 2001/01/26 07:11:48 yoshfuji Exp    */

/*
 * Copyright (c) 1995, 1999
 *    Berkeley Software Design, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef COMMON_IFADDRS_H
#define COMMON_IFADDRS_H
#include "common_extern.h"

#ifndef invalidstr
#define invalidstr(s) (s == 0 || s[0] == 0)
#endif

/*****************************************************************************
 函 数 名  : ifaddrs_get_interface_addr
 功能描述  : 通过接口名获取对应的IP地址
 输入参数  : char *IfName  
 输出参数  : 无
 返 回 值  : 成功返回对应的网络字节序的IP地址，0失败
 
 修改历史      :
  1.日    期   : 2015年12月1日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
unsigned long ifaddrs_get_interface_addr(char *IfName);

/*****************************************************************************
 函 数 名  : ifaddrs_ethaddr_aton
 功能描述  : 把以太网地址字符串格式转化为二进制网络字节序格式
 输入参数  : const char *mac_str  string in xx:xx:xx:xx:xx:xx notation   
 输出参数  : unsigned char *macaddr  binary data
 返 回 值  : TRUE if conversion was successful and FALSE otherwise
 
 修改历史      :
  1.日    期   : 2016年1月8日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ifaddrs_ethaddr_aton(const char *mac_str, unsigned char *macaddr);

/*****************************************************************************
 函 数 名  : ifaddrs_ethaddr_ntoa
 功能描述  : 把以太网地址二进制格式转化成字符串格式
 输入参数  : const unsigned char *macaddr  binary data
 输出参数  : char *mac_str string in xx:xx:xx:xx:xx:xx notation
 返 回 值  : mac_str
 
 修改历史      :
  1.日    期   : 2016年1月8日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
char* ifaddrs_ethaddr_ntoa(const unsigned char *macaddr, char *mac_str);

/*
    Function:ifaddrs_numbers_and_dot_ip_valid
    Description:
        判断点分十进制ip有效性
    Input:
        ip地址
    Output:
        none
    Return:
        0:ip地址无效，1，ip地址有效
    Author:kdz
    Date:2014.8.20
*/
int ifaddrs_numbers_and_dot_ip_valid(const char *ip);

/*****************************************************************************
 函 数 名  : ifaddrs_get_ifip
 功能描述  : 获取接口ip地址
 输入参数  : const char *ifname 接口名  
 输出参数  : char *if_addr 接口ip
 返 回 值  : -1失败，0成功
 
 修改历史      :
  1.日    期   : 2015年11月25日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ifaddrs_get_ifip(const char *ifname, char *if_addr);

/*****************************************************************************
 函 数 名  : ifaddrs_get_if_netmask
 功能描述  : 获取接口子网掩码
 输入参数  : const char *ifname  接口名
 输出参数  : char *if_net  掩码
 返 回 值  : -1失败，0成功
 
 修改历史      :
  1.日    期   : 2015年11月25日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ifaddrs_get_if_netmask(const char *ifname, char *if_net);

/*****************************************************************************
 函 数 名  : ifaddrs_get_if_netmac
 功能描述  : 通过接口名获取mac地址
 输入参数  : char *ifname  接口名
 输出参数  : unsigned char *if_mac  对应的接口mac地址
 返 回 值  : -1失败，0成功
 
 修改历史      :
  1.日    期   : 2015年11月25日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ifaddrs_get_if_netmac(char *ifname, unsigned char *if_mac);

/*****************************************************************************
 函 数 名  : ifaddrs_ipmask_to_numeric
 功能描述  : 把子网掩码转化为数字表示
 输入参数  : const char *mask  
 输出参数  : 无
 返 回 值  : 对应子网掩码的后缀值
 
 修改历史      :
  1.日    期   : 2015年11月25日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ifaddrs_ipmask_to_numeric(const char *mask);

/*****************************************************************************
 函 数 名  : ifaddrs_get_network_addr_seg
 功能描述  : 获取ip地址的地址段 (例如:192.168.10.1-->结果:192.168.10.0)
 输入参数  : const char *ip     
             const char *mask   
 输出参数  : char *networkaddr 网段地址
 返 回 值  :  -1失败，0成功
 
 修改历史      :
  1.日    期   : 2015年11月25日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ifaddrs_get_network_addr_seg(const char *ip,const char *mask,char *networkaddr);

/*****************************************************************************
 函 数 名  : ifaddrs_mac_increase
 功能描述  : 对应位的mac地址加1 //例如:00:11:22:33:44:55 
 输入参数  : unsigned char *mac  
             int c   表示mac地址对应的位    
             当c = 5时,执行该函数后mac地址变为00:11:22:33:44:56       
 输出参数  : 无
 返 回 值  : -1失败，0成功
 
 修改历史      :
  1.日    期   : 2015年11月25日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ifaddrs_mac_increase(unsigned char *mac, int c);

/******************************************************************************
Function:     ifaddrs_ip_increase
Description:  网段或ip增加1
Input:
inIpAddr 第一个ip地址
ipNetMask  对应的子网掩码
outIpAddr 输出ip地址
mode   mode=0,网段加1；mode=1ip加1；mode=2，ip和网段都加1

Output:
   无
Return:
非0 不需要修改
0  修改成功
Others:
*******************************************************************************/
int ifaddrs_ip_increase(char *inIpAddr, char *ipNetMask, char *outIpAddr, int mode);

/*****************************************************************************
 函 数 名  : ifaddrs_get_subnet
 功能描述  : 通过ip和子网掩码获取子网网段 
             例如(192.168.10.1,255.255.255.0-->192.168.10.0/24)
 输入参数  : const char *ip    
             const char *mask  
 输出参数  : char *net  
 返 回 值  : 0成功，-1失败
 
 修改历史      :
  1.日    期   : 2015年11月26日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ifaddrs_get_subnet(const char *ip, const char *mask, char *net);


/*****************************************************************************
 函 数 名  : ifaddrs_get_interface_mtu
 功能描述  : 获取对应接口的mtu值
 输入参数  : const char *ifname       
             unsigned int *mtu_value  
 输出参数  : 无
 返 回 值  : 失败:UGW_ERR
             成功:UGW_OK
 
 修改历史      :
  1.日    期   : 2015年12月24日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM ifaddrs_get_interface_mtu(const char *ifname, unsigned int *mtu_value);

/*****************************************************************************
 函 数 名  : ifaddrs_get_interface_downup
 功能描述  : 获取bss down up状态信息
 输入参数  : mib_name：无线接口
 输出参数  : 
 返 回 值  : Up: UGW_OK  down: UGW_ERR
 修改历史      :
  1.日    期   : 2021年6月28日
    作    者   : huangyongjie
    修改内容   : 新生成函数
*****************************************************************************/
int ifaddrs_get_interface_downup(const char *osifname);

#endif

