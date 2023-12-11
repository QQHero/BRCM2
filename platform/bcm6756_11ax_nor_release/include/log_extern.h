/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : log_extern.h
  版 本 号   : 初稿
  作    者   : Hanzhenzhong
  生成日期   : 2015年12月3日
  最近修改   :
  功能描述   : 

  功能描述   : UGW系统支撑平台日志模块对外（service/product）发布的唯一头文
               件

  修改历史   :
  1.日    期   : 2015年12月3日
    作    者   : Hanzhenzhong
    修改内容   : 创建文件

******************************************************************************/
#ifndef LOG_EXTERN_H
#define LOG_EXTERN_H
#include "common_extern.h"

#define LOG_SOCKET_PATH         "/var/plog_socket"
#define LOG_FMT_STR_LEN         128
#define LOG_MAX_MSG_BUF_LEN     256
#define LOG_MAX_MODULE_NUM      100
#define LOG_DEBUG_DEFAULT_LINE  1024
#define LOG_DEBUG_FILE_PATH     "/var/"


typedef struct UGW_DEBUG_CB
{   
    char module_name[UGW_MODULE_NAME_LEN];
    unsigned char enable;
    unsigned char level;
    unsigned int  line_count;   /*用来记录日志行数防止上层进程退出行数统计失效，因此保留在共享内存里面*/
}UGW_DEBUG_CB_STRU;

typedef enum LOG_DEBUG_LEVEL
{
    UGW_TRACE,
    UGW_INFO,
    UGW_DBG, 
    UGW_ERROR,
    
    UGW_MAX
}LOG_DEBUG_LEVEL_ENUM;

typedef enum LOG_SYS_TYPE
{
    LOG_ALL,                   
    LOG_SYSTEM,                
    LOG_WEB_CLS_FILTER,      
    LOG_WAN1,              
    LOG_WAN2,                 
    LOG_WAN3,                
    LOG_WAN4,                
    LOG_DBG,                   
    LOG_LAN_DHCP,     
    LOG_WISP_DHCP,       
    LOG_CLEAR,                
    LOG_RELOAD,       
    LOG_ERROR,
    /* 操作日志 */
    LOG_LOGIN, //登录日志
    LOG_CFG_CHANGE, //配置变更
    LOG_SHUTDOWN, //开关机日志
    /* 运行日志 */
    LOG_PROCESS, //系统进程日志
    LOG_INTERFACE, //接口状态日志
    LOG_AP_ALERT, //AP告警日志
    LOG_SDWAN, //SDWAN运行日志
    LOG_ATTACK, //攻击日志
    LOG_FATAP, //胖AP运行日志
    LOG_MAX
}LOG_SYS_TYPE_ENUM;

typedef enum LOG_HANDLER_TYPE
{
    HANDLER_SYSTEM,                
    HANDLER_ADMIN,      
    HANDLER_GUEST
}LOG_HANDLER_TYPE_ENUM;

typedef enum LOG_SYS_BT_TYPE
{
    LOG_MESH_EMERGENCY,
    LOG_MESH_ALERT,
    LOG_MESH_CRITICAL,
    LOG_MESH_ERROR,
    LOG_MESH_WARNING,
    LOG_MESH_NOTICE,
    LOG_MESH_INFORMATION,
    LOG_MESH_DEBUG
}LOG_SYS_BT_TYPE_ENUM;

typedef struct LOG_SYS_MSG
{
    long msgtype;
    char msg[LOG_MAX_MSG_BUF_LEN];         
}LOG_SYS_MSG_STRU;

extern void log_debug_print(const char *func, int line, int level, char *module_name, char *fmt, ...);
int log_debug_linecnt(char *DebugFileName);

#define UGW_DEBUG_SHM_FILENAME "/bin/logdebug"
#define UGW_LOG_FIFO_FILE "/var/name_pipe"

#define UGW_DEBUG(level, module_name, format...) \
    log_debug_print(__func__, __LINE__, level, module_name,##format) 
#define UGW_FuncEntry(pModuleName) UGW_DEBUG(UGW_TRACE, pModuleName, "function entry!") 
#define UGW_FuncExit(pModuleName) UGW_DEBUG(UGW_TRACE, pModuleName, "function exit!")

void tdSyslog(int type, const char *format, ...);
void tdSyslog_mesh(int level, const char *format, ...);
#endif

