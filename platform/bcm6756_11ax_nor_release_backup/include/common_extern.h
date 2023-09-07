/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : common_extern.h
  版 本 号   : 初稿
  作    者   : l0018898
  生成日期   : 2015年10月28日
  最近修改   :
  功能描述   : 

  功能描述   : UGW系统支撑平台公共模块对外（service/product）发布的唯一头文件

  修改历史   :
  1.日    期   : 2015年10月28日
    作    者   : l0018898
    修改内容   : 创建文件

******************************************************************************/
#ifndef COMMON_EXTERN_H
#define COMMON_EXTERN_H

#define UGW_VOID                        void
#define UGW_NULL_PTR                    0
#define UGW_NULL_DWORD                  0xffffffff
#define UGW_NULL_WORD                   0xffff
#define UGW_NULL_BYTE                   0xff
#define UGW_NULL_LONG                   0xffffffff
#define UGW_TRUE                        1
#define UGW_FALSE                       0
#define ALL_BITS_ULL                    0xffffffffULL
#define ALL_BITS_UL                     0xffffUL
#define RED                             "\033[0;32;31m"
#define GREEN                           "\033[0;32;32m"
#define LIGHT_GREEN                     "\033[1;32m"
#define NONE                            "\033[m"

#define UGW_INTERFACE_LEN               16
#define UGW_IPADDR_STR_LEN              16
#define UGW_IPV6ADDR_STR_LEN            40
#define UGW_MAX_MAC_FILTER_NUM          256
#define UGW_MAC_BIN_LEN                 6
#define UGW_MAC_STR_LEN                 18
#define UGW_IF_NAME_LEN                 32
#define UGW_URL_LEN                     128
#define UGW_TMP_STR_LEN                 256
#define UGW_MAX_MSG_BUF_LEN             256 
#define UGW_IP_RANGE_STR_LEN            64
#define UGW_DNS_USER_NAME_LEN           32
#define UGW_DNS_USER_PWD_LEN            32
#define UGW_DOMAIN_NAME_LEN             64
#define UGW_PROCESS_NAME_LEN            32
#define UGW_FILE_NAME_LEN               64
#define UGW_BOOL_STR_LEN                2
#define UGW_SOCKET_PATH_LEN             64
#define UGW_MODULE_NAME_LEN             32
#define UGW_DATE_STR_LEN                8
#define UGW_TIME_STR_LEN                32
#define UGW_FILE_LINE_LEN               1024

#define UGW_MAX_WAN_NUM                 4
#define MAX_WIFI_CLIENT_NUM             128
#define SSID_LENGTH_MAX                 32

#define RT_TABLE_NM_STATIC_RT               "staticroute"
#define RT_TABLE_NM_ARPGW                   "arpgw"

#define UGW_ARRAY_SIZE(x)               (sizeof(x) / sizeof((x)[0]))

#define UGW_BIT(n) (1 << (n))
#define UGW_SET_BIT(a, offset) ((a) |= 1 << (offset))
#define UGW_CLEAR_BIT(a, offset) ((a) &= ~(1 << (offset)))
#define UGW_TEST_BIT(a, offset) ((a) & (1 << (offset)))

#define UGW_MAX(x, y)       ({ typeof(x) _x = (x); typeof(y) _y = (y); _x > _y ? _x : _y; })
#define UGW_MIN(x, y)       ({ typeof(x) _x = (x); typeof(y) _y = (y); _x < _y ? _x : _y; })

#define UGW_OFFSETOF(type, member)          ((int)(long *)&(((type *)0)->member))
#define UGW_MEMBER_SIZE(type, member)       (sizeof(((type *)0)->member))
#define UGW_CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - UGW_OFFSETOF(type, member)))

#define UGW_COMMON_POINT_VALID(p)       (UGW_NULL_PTR != (p))

#define UGW_IN    
#define UGW_OUT

#define TPI_BUFLEN_2        2         /* buffer length 2   */
#define TPI_BUFLEN_4        4         /* buffer length 4   */
#define TPI_BUFLEN_8        8         /* buffer length 8   */
#define TPI_BUFLEN_16       16        /* buffer length 16  */
#define TPI_BUFLEN_32       32        /* buffer length 32  */
#define TPI_BUFLEN_64       64        /* buffer length 64  */
#define TPI_BUFLEN_128      128       /* buffer length 128 */
#define TPI_BUFLEN_256      256       /* buffer length 256 */
#define TPI_BUFLEN_512      512       /* buffer length 512 */
#define TPI_BUFLEN_1024     1024      /* buffer length 1024*/

#define TPI_DEBUG(fmt, args...)         do{        /* show debug*/    \
    printf("Debug->%s: %s(%d)--"fmt, __FILE__, __func__, __LINE__, ## args);    \
}while(0)

#define TPI_ERROR(fmt, args...)         do{        /* show error*/    \
    printf("Error->%s: %s(%d)--"fmt, __FILE__, __func__, __LINE__, ## args);    \
}while(0)

/*基本数据类型*/
typedef unsigned long long  u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char       u8;

/* 接口函数返回值类型 */
typedef enum tpi_ret{
    TPI_RET_OK = 0,                        /* 成功 */
    TPI_RET_APP_RUNNING = 1,            /* 模块正在运行 */
    TPI_RET_APP_DEAD = 2,                /* 模块已经退出 */
    TPI_RET_NULL_POINTER = 1001,        /* 空指针错误 */
    TPI_RET_INVALID_PARAM = 1002,        /* 非法参数 */
    TPI_RET_ERROR = 0xff                /* 失败 */
}TPI_RET;

typedef enum UGW_RETURN_CODE
{
    UGW_OK,
    UGW_ERR,
    UGW_BUTT           
}UGW_RETURN_CODE_ENUM;

typedef enum UGW_BOOL_TYPE
{
    UGW_BOOL_FALSE = 0,
    UGW_BOOL_TRUE,
    UGW_BOOL_BUTT
}UGW_BOOL_TYPE_ENUM;

#define UGW_MODULE_DBG            "dbg"
#define UGW_MODULE_MONITOR        "monitor"
#define UGW_MODULE_TIMER          "timer"
#define UGW_MODULE_CFMS           "cfms"
#define UGW_MODULE_LOG            "logserver"
#define UGW_MODULE_NETCTRL        "netctrl"
#define UGW_MODULE_MULTIWAN       "multiWAN"
#define UGW_MODULE_NETCHECK       "network_check"
#define UGW_MODULE_CFMC           "cfmc"
#define UGW_MODULE_DHCPC          "dhcpc"
#define UGW_MODULE_HTTPD          "httpd"
#define UGW_MODULE_APMNG_SVR      "apmng_svr"
#define UGW_MODULE_ARP_RECEIVE    "arp_receive"
#define UGW_MODULE_ATE            "ate"
#define UGW_MODULE_AUTO_DISCOVER  "auto_discover"
#define UGW_MODULE_PORTAL         "portal"
#define UGW_MODULE_AUTHSERVER     "authserver"
#define UGW_MODULE_URLRECORD      "url_record"
#define UGW_MODULE_CLOUDMANAGE    "cloud_manage"
#define UGW_MODULE_UCLOUD_CTL     "ucloud_ctl"
#define UGW_MODULE_COMMON_SO      "common_so"
#define UGW_MODULE_PROD_COMMON_SO "prod_common_so"
#define UGW_MODULE_AP_COMMON_SO   "ap_common_so"
#define UGW_MODULE_WSERVER        "wserver"

#endif
