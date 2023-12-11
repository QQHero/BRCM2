/******************************************************************************
  文 件 名   : km_lib.h
  版 本 号   : 初稿
  作    者   : lp
  生成日期   : 2021年2月4日
  最近修改   :
  功能描述   : 新增的内核组向平台提供的公共接口头文件
******************************************************************************/
#ifndef _KM_LIB_H_
#define _KM_LIB_H_

#include <linux/skbuff.h>

/* netlink部分声明开始 */
#define KM_NETLINK_MSG_TYPE_BASE 0x1000    //内核的netlink消息类型的起始值
#define BSP_NETLINK_MSG_TYPE_BASE 0x2000    //bsp cbb的netlink消息类型的起始值
#define WIFI_NETLINK_MSG_TYPE_BASE 0x3000   //wifi的netlink消息类型的起始值

/*
    通用函数返回值定义
*/
typedef enum {
    eRET_MIN = 0,
    eRET_SUCCESS = eRET_MIN,
    eRET_FAILURE,
    eRET_INVALID_ARG,
    eRET_INVALID_STATE,
    eRET_NO_RESOURCE,
    eRET_ALREADY_EXIST,
    eRET_NOT_EXIST,
    eRET_TIMEOUT,
    eRET_MAX,
/* To use same size of unsigned int */
eRET_PADDING = ((unsigned int)~0),
} e_ret;

/* 内核使用netlink的模块枚举类型定义区域，只允许在枚举类型的后面添加新注册的
   模块，不允许给模块的枚举类型赋值（KM_NETLINK_MSG_TYPE_BASE初值除外） */
typedef enum {
    KM_NETLINK_AUTO_DISCOVER = KM_NETLINK_MSG_TYPE_BASE,
    KM_NETLINK_PORTAL_AUTH,
    KM_NETLINK_MAC_FILTER,
    KM_NETLINK_ONLINE_IP,
    KM_NETLINK_LOAD_BALANCE,
    KM_NETLINK_DNS_REDIRECT,
    KM_NETLINK_WATCHDOG,
    KM_NETLINK_MULTI_BROAD_FILTER,
    KM_NETLINK_WAN_STAT,
    KM_NETLINK_INTERFACE_ISOLATE,
    KM_NETLINK_DHCP_OPTIONS,
    KM_NETLINK_OS_IDENTIFY,
    KM_NETLINK_PPPOE_AUTH,
    KM_NETLINK_SUPER_USER,
    KM_NETLINK_MULTIFREQ_LB,
    KM_NETLINK_HTTP_REDIRECT,
    KM_NETLINK_LINK_LOOP,
    KM_NETLINK_WIFIEVENTS,
    KM_NETLINK_KMEVENTS,
    KM_NETLINK_SPEED_TEST,          //0X1012 netlink type is  KM_NETLINK_START + 18 = 118
    KM_NETLINK_NET_TOPO,
    KM_NETLINK_USER1,
    KM_NETLINK_USER2,
    KM_NETLINK_TUNNEL_FORWARD,
    KM_NETLINK_FLOW_IDENTIFY,
    KM_NETLINK_MSG_TYPE_MAX,
} KM_NETLINK_MSG_TYPE;

/* BSP组使用netlink的模块枚举类型定义区域，只允许在枚举类型的后面添加新注册的
   模块，不允许给模块的枚举类型赋值（BSP_NETLINK_MSG_TYPE_BASE初值除外） */
typedef enum {
    BSP_NETLINK_PHYCHECK     = BSP_NETLINK_MSG_TYPE_BASE,
    BSP_NETLINK_MSG_TYPE_MAX,
} BSP_NETLINK_MSG_TYPE;

/* WIFI组使用netlink的模块枚举类型定义区域，只允许在枚举类型的后面添加新注册的
   模块，不允许给模块的枚举类型赋值（WIFI_NETLINK_MSG_TYPE_BASE初值除外） */
typedef enum {
    WIFI_NETLINK_WIFI_BASE     = WIFI_NETLINK_MSG_TYPE_BASE,
    WIFI_NETLINK_PRIVATE_IE,
    WIFI_NETLINK_MSG_TYPE_MAX,
} WIFI_NETLINK_MSG_TYPE;

/* 平台消息头相关数据结构及宏定义 */
#define PLATFORM_MSG_MAGIC "platform_msg_magic"
#define PLATFORM_MSG_MAGIC_LEN 18
struct platform_msg_head {
    char magic[PLATFORM_MSG_MAGIC_LEN];
    char type[0];
    /* C99标准使用的可变长度数组后面不能有数据，即type[0]后不能有数据，目前
       暂先屏蔽该pclint error */
    unsigned char data[0];//lint !e157
};

//导出符号声明区域
extern e_ret km_lib_netlink_unicast_send_message(void *data, int data_len, pid_t pid, KM_NETLINK_MSG_TYPE msg_type);
extern e_ret km_lib_netlink_broadcast_send_message(void *data, int data_len,
    KM_NETLINK_MSG_TYPE msg_type);
extern void km_lib_netlink_register_rcv_ops(KM_NETLINK_MSG_TYPE msg_type,
    e_ret (*rcv_msg)(struct sk_buff *skb));
extern void km_lib_netlink_unregister_rcv_ops(KM_NETLINK_MSG_TYPE msg_type);
/* netlink部分声明结束 */

/* cJSON部分声明开始 */
/* cJSON Types: */
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6
#define cJSON_String2 7
#define cJSON_Binary 8
#define cJSON_IsReference 256
#define MAX_JSON_LENGTH     4096*5       //最大cJSON对象转换字符串长度

/* The cJSON structure: */
typedef struct cJSON {
    struct cJSON *next,*prev;   /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct cJSON *child;        /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

    int type;                   /* The type of the item, as above. */

    char *valuestring;          /* The item's string, if type==cJSON_String */
    int valueint;               /* The item's number, if type==cJSON_Number; The item's binary data size, if type==cJSON_Binary */

    char *string;               /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} cJSON;

typedef struct cJSON_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJSON_Hooks;

/* Supply malloc, realloc and free functions to cJSON */
extern void cJSON_InitHooks(cJSON_Hooks* hooks);

/* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
extern cJSON *cJSON_Parse(const char *value);
/* Render a cJSON entity to text for transfer/storage. Free the char* when finished. */
extern char  *cJSON_Print(cJSON *item);
/* Render a cJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern char  *cJSON_PrintUnformatted(cJSON *item);
/* Delete a cJSON entity and all subentities. */
extern void   cJSON_Delete(cJSON *c);

/* Returns the number of items in an array (or object). */
extern int    cJSON_GetArraySize(cJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern cJSON *cJSON_GetArrayItem(cJSON *array,int item);
/* Get item "string" from object. Case insensitive. */
extern cJSON *cJSON_GetObjectItem(cJSON *object,const char *string);

/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
extern const char *cJSON_GetErrorPtr(void);

/* These calls create a cJSON item of the appropriate type. */
extern cJSON *cJSON_CreateNull(void);
extern cJSON *cJSON_CreateTrue(void);
extern cJSON *cJSON_CreateFalse(void);
extern cJSON *cJSON_CreateBool(int b);
extern cJSON *cJSON_CreateNumber(int num);
extern cJSON *cJSON_CreateString(const char *string);
extern cJSON *cJSON_CreateArray(void);
extern cJSON *cJSON_CreateObject(void);
extern cJSON *cJSON_CreateBinary(const char *binary, size_t size);

/* These utilities create an Array of count items. */
extern cJSON *cJSON_CreateIntArray(int *numbers,int count);
extern cJSON *cJSON_CreateFloatArray(float *numbers,int count);
extern cJSON *cJSON_CreateDoubleArray(double *numbers,int count);
extern cJSON *cJSON_CreateStringArray(const char **strings,int count);

/* Append item to the specified array/object. */
extern void cJSON_AddItemToArray(cJSON *array, cJSON *item);
extern void cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
extern void cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
extern cJSON *cJSON_DetachItemFromArray(cJSON *array,int which);
extern void   cJSON_DeleteItemFromArray(cJSON *array,int which);
extern cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string);
extern void   cJSON_DeleteItemFromObject(cJSON *object,const char *string);

/* Update array items. */
extern void cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem);
extern void cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);

/* Duplicate a cJSON item */
extern cJSON *cJSON_Duplicate(cJSON *item,int recurse);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */

/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
extern cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated);

/* Macros for creating things quickly. */
#define cJSON_AddNullToObject(object,name)      cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object,name)      cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object,name)     cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object,name,b)    cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object,name,n)  cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object,name,s)  cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
#define cJSON_AddString2ToObject(object,name,s) cJSON_AddItemToObject(object, name, cJSON_CreateString2(s))
#define cJSON_AddBinaryToObject(object,name,binary,size) cJSON_AddItemToObject(object, name, cJSON_CreateBinary(binary, size))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define cJSON_SetIntValue(object,val)           ((object)?(object)->valueint=(object)->valuedouble=(val):(val))
/* cJSON部分声明结束 */

#endif
