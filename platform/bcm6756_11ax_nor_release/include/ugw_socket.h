/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : ugw_socket.h
  版 本 号   : 初稿
  作    者   : zzh
  生成日期   : 2015年12月14日
  最近修改   :
  功能描述   : 

  功能描述   : ugw平台通信机制公共头文件定义

  修改历史   :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 创建文件

******************************************************************************/
#ifndef UGW_SOCKET_H
#define UGW_SOCKET_H
#include <sys/un.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "common_extern.h"

#define MAX_EPOLL_EVENTS            10
#define MAX_EPOLL_LISTEN_NUM        32

#define TIMER_SOCKET_PATH           "/var/tm_socket"
#define CFMD_SOCKET_PATH            "/var/cfm_socket"
#define NETCTRL_SOCKET_PATH         "/var/nt_socket"
#define MULTIWAN_SOCKET_PATH        "/var/mw_socket"
#define PMONITOR_SOCKET_PATH        "/var/pm_socket"
#define PLOG_SOCKET_PATH            "/var/plog_socket"
#define NET_CK_SOCKET_PATH          "/var/network_ck_socket"

#define UGW_MSG_HEADER  \
        int Length;     /*消息体的长度*/ 

#define UGW_MSG_HEADER_LEN              sizeof(UGW_MSG_STRU)

typedef int (*ugw_socket_msg_handler)(int sock);

typedef struct UGW_MSG
{
    UGW_MSG_HEADER
    char Body[0];
}UGW_MSG_STRU;

typedef struct UGW_SOCKET_FD
{
    int sock;
    ugw_socket_msg_handler fn;        /*套接字收包处理函数*/     
}UGW_SOCKET_FD_STRU;

/*****************************************************************************
 函 数 名  : ugw_proc_msg_register
 功能描述  : 用来注册套接字消息处理函数
 输入参数  : UGW_SOCKET_FD_STRU *pFdSocket        
             int ProcessId                            
             ugw_socket_msg_handler pMsgHandle  回调函数
 输出参数  : 无
 返 回 值  : 0成功,1失败
 
 修改历史      :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM ugw_proc_msg_register(UGW_SOCKET_FD_STRU *pFdSocket, 
                                           const char *path, 
                                           ugw_socket_msg_handler pMsgHandle);

/*****************************************************************************
 函 数 名  : ugw_proc_msg_unregister
 功能描述  : 注销消息处理函数
 输入参数  : UGW_SOCKET_FD_STRU *pFdSocket  
             int ProcessId                        
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_VOID ugw_proc_msg_unregister(UGW_SOCKET_FD_STRU *pFdSocket, const char *path);

/*****************************************************************************
 函 数 名  : ugw_proc_send_msg
 功能描述  : 消息发送函数定义
 输入参数  : UGW_MSG_STRU *pMsgValue  
 输出参数  : 无
 返 回 值  : 失败:-1
             成功:实际发送的字节数
 
 修改历史      :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ugw_proc_send_msg(UGW_MSG_STRU *pMsgValue, char *path);

/*****************************************************************************
 函 数 名  : ugw_proc_recv_msg
 功能描述  : 接收套接字数据接口函数
 输入参数  : int sock    
             UGW_MSG_STRU *pMsgValue  
 输出参数  : 无
 返 回 值  : 失败:-1
             成功:实际接收的字节数
 
 修改历史      :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ugw_proc_recv_msg(int sock, UGW_MSG_STRU **pMsgValue);

/*****************************************************************************
 函 数 名  : ugw_connect_server
 功能描述  : 客户端用来连接服务器
 输入参数  : int ProcessId  
 输出参数  : 无
 返 回 值  : 失败-1，成功返回套接字
 
 修改历史      :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ugw_connect_server(const char *path);

/*****************************************************************************
 函 数 名  : ugw_socket_shut_down
 功能描述  : 关闭套接字描述符
 输入参数  : int fd  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_VOID ugw_socket_shut_down(int fd);

/*****************************************************************************
 函 数 名  : ugw_set_socket_timeout
 功能描述  : 设置套接字发送和接收超时时间
 输入参数  : int sock             
             struct timeval * tv  
 输出参数  : 无
 返 回 值  : 0成功，1失败
 
 修改历史      :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM ugw_set_socket_timeout(int sock, struct timeval * tv);

/*****************************************************************************
 函 数 名  : ugw_accept_client
 功能描述  : 监听新的客户端连接
 输入参数  : int server_fd                 
             struct sockaddr_un *out_addr  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2015年12月14日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int ugw_accept_client(int server_fd, struct sockaddr_un *out_addr);

/*****************************************************************************
 函 数 名  : ugw_socket_run_epoll
 功能描述  : 套接字监听客户端和收发数据流
 输入参数  : UGW_SOCKET_FD_STRU *fd_server  
             int efd                
             struct epoll_event *events
             int i
 输出参数  : 无
 返 回 值  : 无
 
 修改历史      :
  1.日    期   : 2017年3月20日
    作    者   : luolang
    修改内容   : 修改函数

*****************************************************************************/
UGW_VOID ugw_socket_run_epoll(UGW_SOCKET_FD_STRU *fd_server, 
                        int efd, struct epoll_event *events, int i);

/*****************************************************************************
 函 数 名  : ugw_register_poll
 功能描述  : 调用epoll_ctl添加文件描述符
 输入参数  : int efd  int fd   
 输出参数  : 无
 返 回 值  : 0表示注册成功，-1表示失败
 修改历史      :
  1.日    期   : 2017年5月9日
    作    者   : luomj
    修改内容   : 新生成函数

*****************************************************************************/
int ugw_register_poll(int efd,int fd);

/*****************************************************************************
 函 数 名  : ugw_fetch_epoll_events
 功能描述  : 调用epoll_wait监听fd
 输入参数  : UGW_SOCKET_FD_STRU *fd_server  int efd  int timeout
 输出参数  : 无
 返 回 值  : 无
 修改历史      :
  1.日    期   : 2017年5月9日
    作    者   : luomj
    修改内容   : 新生成函数

*****************************************************************************/

UGW_VOID ugw_fetch_epoll_events(UGW_SOCKET_FD_STRU *fd_server,int efd,int timeout);

#endif

