/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : common_check.h
  版 本 号   : 初稿
  作    者   : zzh
  生成日期   : 2015年11月25日
  最近修改   :
  功能描述   : 

  功能描述   : 配置以及逻辑检测库函数接口定义头文件

  修改历史   :
  1.日    期   : 2015年11月25日
    作    者   : zzh
    修改内容   : 创建文件

******************************************************************************/
#ifndef COMMON_CHECK_H
#define COMMON_CHECK_H
#include "common_extern.h"

/*****************************************************************************
 函 数 名  : check_ip_is_same_net
 功能描述  : 检测两个ip是否处于同一网段
 输入参数  : char *ip1    
             char *mask1  
             char *ip2    
             char *mask2  
 输出参数  : 无
 返 回 值  : 1同一网段，0不同网段
 
 修改历史      :
  1.日    期   : 2015年11月26日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int check_ip_is_same_net(char *ip1,char *mask1,char *ip2,char *mask2);
/*******************************************************
Function: check_ip_is_addrpool
Author: Liushenghui
Description: 检测ip地址是否处于地址池内
Input:
    pStartIp - start ip.
    pEndIp - end ip.
    pIp - ip for check.
Output:
Return: 1 if ip is between two ip, 0 if not.
Others:
********************************************************/
int check_ip_is_addrpool(const char * pStartIp, const char * pEndIp,
                const char * pIp);

#endif

