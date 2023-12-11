/***********************************************************
    Copyright (C), 1998-2022, Tenda Tech. Co., Ltd.
    FileName: xmesh_extern.h
    Description: 平台向产品组提供的公共接口头文件
    Version : 1.0
    Date: 2022.4.8
    <author>   <time>     <version >   <desc>
    wzz        2022-4-8   1.0        new
************************************************************/
#ifndef XMESH_EXTERN_H
#define XMESH_EXTERN_H



#define XMESH_SOCKET_PATH               "/var/xmesh_socket"

enum XMESH_SOCKET_MSG_TYPE
{
    XMESH_SOCKET_MSG_DEVICE_OPERATION = 1,  // 设备管理数据下发
                                            // 格式：type = 1，一个字节
    XMESH_SOCKET_MSG_CHANGE_DBG_LEVEL,      // 修改日志等级
                                            // 格式：type = 2，dbg_level = n，共两个字节
    XMESH_SOCKET_MSG_CONFIG_RENEW,          // 配置同步
                                            // 格式：type = 3，config_type = n，共两个字节
    XMESH_SOCKET_MSG_GET_TOPOLOGY,          // 获取拓扑
                                            // 格式：type = 4，一个字节
    XMESH_SOCKET_MSG_CONFIG_SET,            // 设备配置修改，如设备名称修改。
                                            // 格式：type = 5, mib_type = n，共两个字节
    XMESH_SOCKET_MSG_GET_RSSI_AND_LINK_TYPE,// 获取本地节点的rssi和link_type,供led灯闪烁使用   
                                            // 格式：type = 6, mib_type = n，共两个字节
    XMESH_SOCKET_MSG_ADD_NEW_AGENT,         // 触发扫描和添加新节点

    XMESH_SOCKET_MSG_GET_AGENT_WPS,         // 获取子节点wps状态
    XMESH_SOCKET_MSG_GET_AGENT_UPGRADE_INFO,// 获取子节点在线升级信息
    XMESH_SOCKET_MSG_GET_AGENT_SCHED_REBOOT_INFO,// 获取子节点自动维护信息（强制升级）

    XMESH_SOCKET_MSG_GET_SN_AND_MAC,

    XMESH_SOCKET_MSG_UPDATE_NODE_INFO,      //触发节点更新本地信息并发一次topo discover

    XMESH_SOCKET_MSG_GET_LOCAL_NODE_RSSI_AND_LINK_TYPE, //获取本地节点当前组网连接类型与rssi

    XMESH_SOCKET_MSG_CLIENT_OFF_LINE,

    XMESH_SOCKET_MSG_SEND_NODE_ACTION_TO_CONTROLLER, //向主节点发送节点动作（如：子节点移除）

    XMESH_SOCKET_MSG_OPEN_AGENT_TELNET,// 开启子节点telnet

    XMESH_SOCKET_MSG_CBRR,         //角色切换请求17

    XMESH_SOCKET_MSG_GET_LOCAL_NODE_FATHER_5G_BBSS_MAC,         //获取当前节点父节点5G BBSS MAC地址
    XMESH_SOCKET_MSG_CLIENT_ON_LINE,
};

// 用以区分是谁在通过unix socket向easymesh发送消息
//
enum MESH_PROCESS_MESSAGE_ID
{
    MESH_NETCTRL_MESSAGE_ID = 1,
    MESH_LLDP_MESSAGE_ID,
    MESH_STEER_MESSAGE_ID,
};

// 配置类型TLV
//
#define XMESH_CONFIG_TYPE_NONE                    0x0000
#define XMESH_CONFIG_TYPE_24G                     0x8000
#define XMESH_CONFIG_TYPE_5G                      0x4000
#define XMESH_CONFIG_TYPE_MAC_FILTER              0x2000
#define XMESH_CONFIG_TYPE_ALL_AGENT_LED           0x0001
#define XMESH_CONFIG_TYPE_AGENT_NAME              0x0002
#define XMESH_CONFIG_TYPE_REBOOT_TIMER            0x0004
#define XMESH_CONFIG_TYPE_CLIENT_NAME             0x0008
#define XMESH_CONFIG_TYPE_QOS_RULE                0x0010
#define XMESH_CONFIG_TYPE_TIME_ZONE               0x0020
#define XMESH_CONFIG_TYPE_PARENT_CONTROL          0X0040
#define XMESH_CONFIG_TYPE_AGENT_DMZ               0X0080
#define XMESH_CONFIG_TYPE_BAND_STEER              0x0100
#define XMESH_CONFIG_TYPE_SMART_ASSISTANT         0x0200
#define XMESH_CONFIG_TYPE_WPS_SET                 0x0400
#define XMESH_CONFIG_TYPE_ONE_AGENT_LED           0x0800 //只针对单一agent设置
#define XMESH_CONFIG_TYPE_UCLOUD_INFO             0X1000
#define XMESH_CONFIG_TYPE_ALL                     0xFFFF

#define XMESH_CONFIG_TYPE_AGENT_SET (XMESH_CONFIG_TYPE_ALL_AGENT_LED | XMESH_CONFIG_TYPE_AGENT_NAME | XMESH_CONFIG_TYPE_REBOOT_TIMER \
                                     | XMESH_CONFIG_TYPE_CLIENT_NAME | XMESH_CONFIG_TYPE_QOS_RULE | XMESH_CONFIG_TYPE_TIME_ZONE \
                                     | XMESH_CONFIG_TYPE_PARENT_CONTROL | XMESH_CONFIG_TYPE_AGENT_DMZ | XMESH_CONFIG_TYPE_BAND_STEER \
                                     | XMESH_CONFIG_TYPE_SMART_ASSISTANT | XMESH_CONFIG_TYPE_WPS_SET | XMESH_CONFIG_TYPE_ONE_AGENT_LED \
                                     | XMESH_CONFIG_TYPE_UCLOUD_INFO)

/* 扩展配置类型tlv */
enum XMESH_EXT_CONF_TYPE {
    CONF_TYPE_EX_NONE              = 0x00000000,
    CONF_TYPE_EX_TYPE_6G           = 0x00000001, //使用新增宏来针对6G配置
    CONF_TYPE_EX_WIFI_SWITCH       = 0x00000002, //wifi开关同步需求
    CONF_TYPE_EX_WIFI_TIMER        = 0x00000004, //wifi定时同步需求
    CONF_TYPE_EX_TOPO_INFO         = 0x00000008, //组网信息同步需求
    CONF_TYPE_EX_GENERIC           = 0x00000010, //一般配置请求，不区分band的部分通用配置写到这里面
    CONF_TYPE_EX_ALL               = 0xFFFFFFFF
};

/* 实现两个通用的宏来对扩展配置进行get和set */
#define XMESH_CONFIG_TYPE_GET(type, sub_type) ((type) & (sub_type))
#define XMESH_CONFIG_TYPE_SET(type, sub_type) ((type) |= (sub_type))

#define XMESH_CONFIG_TYPE_SET_24G(type)           (XMESH_CONFIG_TYPE_SET(type, XMESH_CONFIG_TYPE_24G))
#define XMESH_CONFIG_TYPE_SET_5G(type)            (XMESH_CONFIG_TYPE_SET(type ,XMESH_CONFIG_TYPE_5G))
#define XMESH_CONFIG_TYPE_SET_MAC_FILTER(type)    (XMESH_CONFIG_TYPE_SET(type ,XMESH_CONFIG_TYPE_MAC_FILTER))
#define XMESH_CONFIG_TYPE_GET_6G(type)            (XMESH_CONFIG_TYPE_GET(type, CONF_TYPE_EX_TYPE_6G))
#define XMESH_CONFIG_TYPE_GET_24G(type)           (XMESH_CONFIG_TYPE_GET(type ,XMESH_CONFIG_TYPE_24G))
#define XMESH_CONFIG_TYPE_GET_5G(type)            (XMESH_CONFIG_TYPE_GET(type ,XMESH_CONFIG_TYPE_5G))
#define XMESH_CONFIG_TYPE_GET_MAC_FILTER(type)    (XMESH_CONFIG_TYPE_GET(type ,XMESH_CONFIG_TYPE_MAC_FILTER))
#define XMESH_CONFIG_TYPE_GET_AGENT_CONFIG(type)  (XMESH_CONFIG_TYPE_GET(type ,XMESH_CONFIG_TYPE_AGENT_SET))



#define XMESH_PROCESS_NAME           "xmesh"

#define XMESH_FILE_PROD_INFO         "/tmp/product_info"
#define XMESH_PROD_FW_VER            "fw_ver"
#define XMESH_PROD_HW_VER            "hw_ver"
#define XMESH_PROD_SVN_VER           "svn_ver"

#define XMESH_RECV_BUF_SIZE          65535

#endif