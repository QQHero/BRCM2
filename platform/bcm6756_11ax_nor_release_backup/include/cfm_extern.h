/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : cfm_extern.h
  版 本 号   : 初稿
  作    者   : l0018898
  生成日期   : 2015年10月28日
  最近修改   :
  功能描述   : 

  功能描述   : UGW系统支撑平台cfm模块对外（service/product）发布的唯一头文件

  修改历史   :
  1.日    期   : 2015年10月28日
    作    者   : l0018898
    修改内容   : 创建文件

******************************************************************************/
#ifndef CFM_EXTERN_H
#define CFM_EXTERN_H

#include "ugw_socket.h"

//参数输入、输出标准
#define IN 
#define OUT 

//定义mib中，参数和数值的最大长度
#define MAX_MIB_NAME_LENGTH         512
#define MAX_MIB_VALUE_LENGTH        2308
#define MAX_MIB_LINE_LENGTH         (MAX_MIB_NAME_LENGTH + MAX_MIB_VALUE_LENGTH + 2) // 2byte: '='和'\n'
#define NVRAM_DEFAULT_FLAG          "default_nvram"
#define FASTNAT_SWITCH_FILE         "/proc/net/fn_switch"

typedef enum MSG_CMD
{
    MSG_CMD_TIMER_REQ = 0,              //定时事件消息通知
    MSG_CMD_TIMER_RES,
    MSG_CMD_SET_REQ,                    //设置数值请求
    MSG_CMD_SET_RES,                    //设置数值回应
    MSG_CMD_GET_REQ,                    //获取参数请求
    MSG_CMD_GET_RES,                    //获取参数回应
    MSG_CMD_REG_REQ,                    //注册消息通知请求
    MSG_CMD_REG_RES,                    //注册消息通知回应
    MSG_CMD_POSTMSG_REQ,                //发送消息请求
    MSG_CMD_POSTMSG_RES,                //发送消息回应
    MSG_CMD_PUSHMSG_REQ,                //推进消息请求
    MSG_CMD_PUSHMSG_RES,                //推进消息回应
    MSG_CMD_COMMIT_REQ,                 //保存消息请求
    MSG_CMD_COMMIT_RES,                 //保存消息回应
    MSG_CMD_SHOW_REQ,                   //显示MIB请求
    MSG_CMD_SHOW_RES,                   //显示MIB内容应答
    MSG_CMD_RESTORE_REQ,                //恢复出厂MIB
    MSG_CMD_RESTORE_RES,                //恢复出厂MIB 回应
    MSG_CMD_UNSET_REQ,                  //保存消息回应(是否写FLASH)
    MSG_CMD_UNSET_RES,                  //清除设置数值请求
    MSG_CMD_COMMIT_NONE_RES,            //清除设置数值回应
    MSG_CMD_APMIB_SET_REQ,
    MSG_CMD_APMIB_SET_RES,
    MSG_CMD_APMIB_GET_REQ,
    MSG_CMD_APMIB_GET_RES,
    MSG_CMD_APMIB_SHOW_REQ,
    MSG_CMD_APMIB_SHOW_RES,
    MSG_CMD_APMIB_COMMIT_REQ,
    MSG_CMD_APMIB_COMMIT_RES,
    MSG_CMD_APMIB_DEFAULT_REQ,
    MSG_CMD_APMIB_DEFAULT_RES,
    MSG_CMD_COMMIT_SYNC_REQ,               //同步保存mib配置请求
    MSG_CMD_COMMIT_SYNC_RES,
    MSG_CMD_APMIB_UNSET_REQ,
    MSG_CMD_APMIB_UNSET_RES,
    MSG_CMD_APMIB_LOADFILE_REQ,         // apmib读文件批量更新请求
    MSG_CMD_APMIB_LOADFILE_RES,         // apmib读文件批量更新回应
}MSG_CMD_ENUM;

typedef enum CFMD_FLASH_WRITE_TYPE
{
    CFMD_FLASH_WRITE_ASYNC = 0,   //异步写flash
    CFMD_FLASH_WRITE_SYNC,        //同步写flash
    CFMD_FLASH_WRITE_BUTT
}CFMD_FLASH_WRITE_TYPE_ENUM;

typedef struct CFMD_MSG_INFO
{
    int  type;
    char name[MAX_MIB_NAME_LENGTH];
    char value[MAX_MIB_VALUE_LENGTH];          
}CFMD_MSG_INFO_STRU;

/*****************************************************************************
 函 数 名  : cfms_encode_msg
 功能描述  : 组装ie信元格式数据
 输入参数  : UGW_IN CFMD_MSG_INFO_STRU *SendMsg  
 输出参数  : UGW_OUT UGW_MSG_STRU **RespMsg
 返 回 值  : 成功: UGW_OK
             失败: UGW_ERR
 
 修改历史      :
  1.日    期   : 2016年4月1日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM cfms_encode_msg(UGW_OUT UGW_MSG_STRU **RespMsg, UGW_IN CFMD_MSG_INFO_STRU *SendMsg);

/*****************************************************************************
 函 数 名  : cfms_decode_msg
 功能描述  : 解析cfms ie信元格式数据
 输入参数  : UGW_IN UGW_MSG_STRU *RcvMsg          
 输出参数  : UGW_OUT CFMD_MSG_INFO_STRU *CfmdMsg
 返 回 值  : 成功: UGW_OK
             失败: UGW_ERR
 
 修改历史      :
  1.日    期   : 2016年4月1日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM cfms_decode_msg(UGW_IN UGW_MSG_STRU *RcvMsg, UGW_OUT CFMD_MSG_INFO_STRU *CfmdMsg);

/*****************************************************************************
 函 数 名  : cfms_proc_send_msg
 功能描述  : cfms发送确认数据包
 输入参数  : int sock           
             UGW_MSG_STRU *Msg  
 输出参数  : 无
 返 回 值  : 成功: UGW_OK
             失败: UGW_ERR
 
 修改历史      :
  1.日    期   : 2016年4月1日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM cfms_proc_send_msg(int sock, UGW_MSG_STRU *Msg);

/*************************************************
Function:     GetValue
Description:  从mib库中，获取数值
Input:
  name, 输入需要获取字符串的名称

Output:
  value, 提供返回字符串的存放地址

Return:
1： 成功
0： 失败 
Others:
*************************************************/
int GetValue(IN const char *name, OUT char *value);

/*************************************************
Function:     SetValue
Description:  设置数值到mib库中
Input:
  name, 输入需要保存的字符串名称
  value, 输入需要的数值

Output:
  无

Return:
1： 成功
0： 失败 
Others:
*************************************************/
int SetValue(IN const char *name, IN char *value);


/*************************************************
Function:     UnSetValue
Description:  删除mib中的某个数值
Input:
  name, 输入需要删除的字符串名称

Output:
  无

Return:
1： 成功
0： 失败 
Others:
*************************************************/
int UnSetValue(IN char *name);



/*************************************************
Function:     ShowValue
Description:  打印所有的mib中数值到指定文件
Input:
  file, 数值输出到指定的文件名称

Output:
  无

Return:
1： 成功
0： 失败 
Others:
*************************************************/
int ShowValue(char *file);

/*************************************************
Function:     RestoreValue
Description:  清除所有当前相关参数，恢复到出厂默认设置
Input:
 无
Output:
 无 

Return:
1： 成功
0： 失败 
Others:
*************************************************/
int RestoreValue();


/*************************************************
Function:     CommitCfm
Description:  通知CFM保存参数到Flash
Input:
 无 
Output:
 无 

Return:
1： 成功
0： 失败 
Others:
*************************************************/
int CommitCfm();

/*****************************************************************************
 函 数 名  : CommitSyncCfm
 功能描述  : 新增同步写flash的函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 
             1： 成功
             0： 失败 
 
 修改历史      :
  1.日    期   : 2017年7月31日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int CommitSyncCfm();


/*************************************************
Function:     PostMsg
Description:  发送消息，给指定任务
Input:
  AppName, 消息名称
  MsgType, 消息类型
Output:
 无 

Return:
1： 成功
0： 失败 
Others:
  AppName为"*"，则系统通知所有的注册该消息类型的任务。
*************************************************/
int PostMsg(IN char  *AppName,  IN char *MsgType);

/*************************************************
Function:     RegMsgHandle
Description:  注册一个函数，用于接收由CFM模块发送到相关任务消息
Input:
  AppName, 消息名称
  AppId,    应用id
  handle,  收到消息，处理回调函数
Output:
 无 

Return:
1： 成功
0： 失败 
Others:
*************************************************/
typedef int (MSGHANDLE)(char *AppName,char *MsgType);
int RegMsgHandle(IN char *AppName, IN char *MsgType, IN MSGHANDLE *handle);

/*****************************************************************************
 函 数 名  : RestoreNvram
 功能描述  : 恢复nvram默认配置
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2016年8月30日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
void RestoreNvram();

/*****************************************************************************
 函 数 名  : UploadValue
 功能描述  : 导入配置文件到mib
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2016年8月30日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int UploadValue();

int ApmibShow(void);
int ApmibCommitCfm(void);
int ApmibGetValue(IN const char *name, OUT char *value);
int ApmibSetValue(IN const char *name, IN char *value);
int ApmibUnSetValue(IN const char *name);
int ApmibDefault(void);
int ApmibLoadFile(IN const char *file);
/*****************************************************************************
 函 数 名  : cfms_set_default_cfg
 功能描述  : 用于保存恢复出厂设置需要同步到mib值中的数据
 输入参数  : char *key    
             char *value  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2017年9月5日
    作    者   : xujun
    修改内容   : 新生成函数

*****************************************************************************/
int cfms_set_default_cfg(char *key, char *value);
#endif

