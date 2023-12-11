/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : monitor_extern.h
  版 本 号   : 初稿
  作    者   : zzh
  生成日期   : 2015年12月4日
  最近修改   :
  功能描述   : 

  功能描述   : monitor进程对外接口函数声明以及消息定义

  修改历史   :
  1.日    期   : 2015年12月4日
    作    者   : zzh
    修改内容   : 创建文件

******************************************************************************/

#ifndef MONITOR_EXTERN_H
#define MONITOR_EXTERN_H

#include <unistd.h>
#include "common_extern.h"
#include "ugw_socket.h"

#define MONITOR_CMDLINE_LEN 256
#define MONITOR_PROGRAM_NAME_LEN 64
#define MONITOR_PATH_LENGTH 64

typedef enum MONITOR_HANDLE_MSG_TYPE
{
    MONITOR_HANDLE_TIMER = 0,
    MONITOR_HANDLE_NETWORK_SUCCESS = 1,
    MONITOR_HANDLE_PROGRAM_START,
    MONITOR_HANDLE_PROGRAM_STOP,
    MONITOR_HANDLE_PROGRAM_RESTART,
    MONITOR_HANDLE_PROGRAM_SYSTEM,    //执行类似system功能
    MONITOR_GET_PROGRAM_PID_BY_NAME,
    MONITOR_SHOW_PROGRAM_INFO,
    MONITOR_CHECK_PROGRAM_INFO,
    MONITOR_HANDLE_SUCCESS,   //11
    MONITOR_HANDLE_FAILURE,   //12
    MONITOR_HANDLE_RELEASE_RELEASETIONSHIP,
    MONITOR_HANDLE_BUTT
}MONITOR_HANDLE_MSG_TYPE_ENUM;

typedef struct MONITOR_MSG_INFO
{
    int type;           //消息类型
    char cmdline[MONITOR_CMDLINE_LEN];
    char program_name[MONITOR_PROGRAM_NAME_LEN];
    char path[MONITOR_PATH_LENGTH];
    pid_t program_pid;
    int proctype;       //进程类型
    int pid_exist;      //保留字段，目前作为进程查询的返回值，0不存在，1存在      
}MONITOR_MSG_INFO_STRU;

/*****************************************************************************
 函 数 名  : monitor_decode_msg
 功能描述  : ie格式数据解码函数
 输入参数  : UGW_IN UGW_MSG_STRU *RcvMsg                
 输出参数  : UGW_OUT MONITOR_MSG_INFO_STRU *MonitorMsg  
 返 回 值  : 成功: UGW_OK
             失败: UGW_ERR
 
 修改历史      :
  1.日    期   : 2016年3月31日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM monitor_decode_msg(UGW_IN UGW_MSG_STRU *RcvMsg, UGW_OUT MONITOR_MSG_INFO_STRU *MonitorMsg);

/*****************************************************************************
 函 数 名  : monitor_encode_msg
 功能描述  : monitor进程组装ie格式数据函数
 输入参数  : UGW_IN MONITOR_MSG_INFO_STRU *SendMsg  
 输出参数  : UGW_OUT UGW_MSG_STRU **RespMsg   
 返 回 值  : 成功: UGW_OK
             失败: UGW_ERR
             
 修改历史      :
  1.日    期   : 2016年3月31日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM monitor_encode_msg(UGW_OUT UGW_MSG_STRU **RespMsg, UGW_IN MONITOR_MSG_INFO_STRU *SendMsg);

/*****************************************************************************
 函 数 名  : monitor_proc_send_msg
 功能描述  : 向指定的套接字发送数据
 输入参数  : int sock           
             UGW_MSG_STRU *Msg  
 输出参数  : 无
 返 回 值  : 成功: UGW_OK
             失败: UGW_ERR
 
 修改历史      :
  1.日    期   : 2016年3月31日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM monitor_proc_send_msg(int sock, UGW_MSG_STRU *Msg);

//启动cmdline指定的进程，并且一旦进程挂掉monitor会自动重启该进程
int monitor_mstart(char *cmdline);

//启动cmdline指定的进程，启动的进程挂掉不会重新启动
int monitor_start(char *cmdline);
//重新启动进程，如果进程存在就kill掉对应的进程，如果
//进程不存在就直接启动该进程
int monitor_restart(char *cmdline);
//停止进程
int monitor_stop(char *program_name);
//调用该接口相当于调用了system函数
int monitor_system(char *cmdline);
//通过进程名获取对应进程的pid
int monitor_program_get_pid(char *program_name);
//打印所有监控进程信息
int monitor_program_show();
/*网络初始化ok拉起平台基本模块常驻进程*/
int monitor_system_network_ok();

/*查询某一进程是否存在，0不存在，1存在*/
int monitor_check_exist(const char *program_name);

//启动cmdline指定的进程，并且一旦进程挂掉就会通知拉起该进程的模块，让其重新启动该进程
//例如multiwan进程启动wan接入进程
int monitor_start_extern(char *cmdline);

int monitor_release_relationship(const char *program_name);
#endif
