#ifndef _IOF_LIB_DRV_H_
#define _IOF_LIB_DRV_H_

#define __must_check __attribute__((warn_unused_result))
#if defined(CONFIG_MVL_PHY)
#define PHY_NUM_MAX    10
#else
#define PHY_NUM_MAX    6
#endif
#define BUF32_LEN 32

/* 设置path长度最大为1024，原因是当使用snprintf函数来拼接路径时，如果有多个%s，会假定前一个%s的长度为256，之前值为100，在开启 -Wall 选项的时候会告警 */
#define PATH_LEN_MAX 1024

#define VLAN_MODE_BRIDGE    1
#define VLAN_MODE_NAT       0
#define VLAN_LAN_VID        1
#define VLAN_WAN_VID        2
#define DEFAULT_MODE        1    //标识符，VLAN设置为Default模式
#define CUSTOM_MODE         2    //标识符，VLAN设置为Custom模式

typedef enum {
    LED_SYS = 0x00,
    LED_WPS = 0x01,
    LED_24G = 0x02,
    LED_5G  = 0x03,
    LED_USB = 0x04,
    LED_LAN = 0x05,
    LED_WAN = 0x06,
    LED_RED = 0x07,
    LED_BLUE = 0x08,
    LED_GREEN = 0x09,
    LED_END = 0x1f,

    BTN_RST = 0x20,
    BTN_WPS = 0x21,
    BTN_WIFI = 0x22,
    BTN_END = 0x3f
} gpio_id_t;
#define GPIO_LED_ID_MASK    0x3FF
#define GPIO_ALL_ID_MASK    0x7000003FFULL

typedef enum {
    LED_ON = 1,
    LED_OFF = 2,
    LED_BLINK = 3,
    LED_NORMAL = 4,
    LED_FORCE_ON = 16,
    LED_FORCE_OFF = 17
} led_pattern_t;
#define LED_PATTERN_MASK 0x3001E

typedef enum {
    ETH_NM_INVALID = -1,
    LAN_1 = 0, /* lan1的桥名字 */
    LAN_2 = 1,
    LAN_3 = 2,
    LAN_4 = 3,
    LAN_MAX,

    L_LAN_1 = 4, /* lan1虚拟设备名字 */
    L_LAN_2 = 5,
    L_LAN_3 = 6,
    L_LAN_4 = 7,

    P_LAN_1 = 8, /* lan1实体设备名字 */
    P_LAN_2 = 9,
    P_LAN_3 = 10,
    P_LAN_4 = 11,

    WAN_1 = 12, /* wan1设备名字 */
    WAN_2 = 13,
    WAN_3 = 14,
    WAN_4 = 15,
    WAN_MAX,

    P_WAN_1 = 16, /* wan1设备名字 */
    P_WAN_2 = 17,
    P_WAN_3 = 18,
    P_WAN_4 = 19,
    P_WAN_MAX,

    WLAN_24G_1 = 20, /* 无线网络设备2.4G名字 */
    WLAN_24G_2 = 21,
    WLAN_24G_3 = 22,
    WLAN_24G_4 = 23,

    WLAN_5G_1 = 24, /* 无线网络设备5G名字 */
    WLAN_5G_2 = 25,
    WLAN_5G_3 = 26,
    WLAN_5G_4 = 27,

    WLAN_6G_1 = 28, /* 无线网络设备6G名字 */
    WLAN_6G_2 = 29,
    WLAN_6G_3 = 30,
    WLAN_6G_4 = 31,

    PORT_CPU1,
    PORT_CPU2,
    PORTCPU_MAX,
    IPTV_LAN_1,    //THE port for iptv lan side
    IPTV_LAN_2,
    IPTV_LAN_3,
    IPTV_LAN_4,
    IPTV_WAN_1,     //The port for iptv wan side
    IPTV_MAX
} eth_idx_t;

typedef enum {
    IOF_ROUTE_MODE = 1,
    IOF_WISP_MODE = 2,
    IOF_APCLIENT_MODE = 3,
    IOF_AP_MODE = 4
} net_mode_t;

struct ifnm_tab {
    eth_idx_t key;
    char val[BUF32_LEN];
    char new_name[BUF32_LEN];
};

struct port_status {
    unsigned int link;
    unsigned int speed;
    unsigned int duplex;
    unsigned int nway;
};

struct mac_port {
    unsigned int opmode;
    unsigned char mac[6];
    unsigned int port_id;
};

typedef enum {
    _LINK_UP = 1,
    _LINK_DOWN = 2
} port_link_t;

typedef enum {
    _10M = 1,
    _100M = 2,
    _1000M = 3,
    _2500M = 4
} port_speed_t;

typedef enum {
    _HALF_DUPLEX = 1,
    _FULL_DUPLEX = 2
} port_duplex_t;

typedef enum {
    _EN_AUNG = 1,  /* 使能自协商 */
    _DIS_AUNG = 2 /* 去使能自协商 */
} port_nw_t;

struct igmp_conf {
    unsigned int cpu_port;               /* CPU口端口号*/
    unsigned int iptv_port;              /* iptv端口号(0 - 4) */
    unsigned int wan_port;               /* WAN口端口号(0 - 4) */
    unsigned int *iptp_custom_vids;      /* 存放要配置vlan id */
    unsigned int iptv_custom_main_vid;   /* 要配置vlan id个数 */
    unsigned int iptv_custom_vids_num;   /* IPTV主vlan id */
};

struct port_mib_count {
    unsigned int rx_bytes_lo;            /*low 32 bits 接收字节数*/
    unsigned int rx_bytes_hi;            /*high 32 bits 接收字节数*/
    unsigned int tx_bytes_lo;            /*low 32 bits 发送字节数*/
    unsigned int tx_bytes_hi;            /*high 32 bits 发送字节数*/
    unsigned int unicast_rx_num;            /*单播包接收个数*/
    unsigned int unicast_tx_num;            /*单播包发送个数*/
    unsigned int multicast_rx_num;            /*组播包接收个数*/
    unsigned int multicast_tx_num;            /*组播包发送个数*/
    unsigned int broadcast_rx_num;            /*广播包发送个数*/
    unsigned int broadcast_tx_num;            /*广播包发送个数*/
};

struct fdb_mac {
    unsigned int port_id;             /*节点mac地址对应物理端口号*/
    unsigned char mac[6];               /*mac 地址内容*/
    struct fdb_mac *next;               /*下一个mac地址入口*/
};

struct statis_info {
    unsigned long long txpackets;
    unsigned long long txbytes;
    unsigned long long txerror;
    unsigned long long txdropd;
    unsigned long long rxpackets;
    unsigned long long rxbytes;
    unsigned long long rxerror;
    unsigned long long rxdropd;
};

/*
    设置输出型gpio(LED)状态
    id: 输入参数，枚举gpio_id_t类型值，其他值为非法
    mode ：输入参数，枚举led_pattern_t类型值，其他值为非法
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_set(gpio_id_t  id, unsigned int mode);
/*
    获取输入型gpio(BTN)状态
    id: 输入参数，枚举gpio_id_t类型值，其他值为非法
    mode ：输出参数，调用者确保不传空指针，0：按键按下；1：按键弹起
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_get(gpio_id_t id, unsigned int *mode);

/*
    更改gpio(led)频率
    id:输入参数，枚举gpio_id_t类型值，其他值为非法
    freq:输入参数，调用确保该值大于0
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_set_freq(gpio_id_t id, unsigned int freq);

/*
    更改gpio(led)start:一般在修改led灯频率时调用
    id:输入参数，枚举gpio_id_t类型值，其他值为非法
    start:输入参数，0:关闭，1：开启。
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_set_start(gpio_id_t id, unsigned int start);

/*
    获取端口状态
    port_id：输入参数，有效值范围(0 - 4)
    status: 输入参数, 要设置的端口模式
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_port_status_set(unsigned int port_id,
    struct port_status *status);
/*
    获取端口状态
    port_id：输入参数，有效值范围(0 - 4)
    status: 输入参数, 要设置的端口模式
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_port_status_get(unsigned int port_id,
    struct port_status *status);

/*
    获取port端口统计计数
    port_id:输入参数，
    输出参数:count, 返回端口接收和发送字节数
    返回值: 非0:函数执行失败， 0:函数执行成功
*/
int __must_check iof_port_mib_count_get(unsigned int port_id,
    struct port_mib_count *count);

/*
    端口镜像设置
    enable:端口镜像使能控制，为1开启，其他值不使能
    mirror_port:输入参数，镜像端口号
    mirrored_ports:输入参数，被镜像端口掩码
    返回值：非0执行失败，0函数执行成功
*/
int __must_check iof_port_mirror_set(unsigned int enable, unsigned int mirror_port,
    unsigned int mirrored_ports);
    
/*
    硬件转发设置
    enable:硬件使能控制，为1开启，其他值不使能
    返回值：无
*/    
int __must_check iof_set_hw_switching(unsigned int enable);

/*
    switch端口定义为wan口设置
    enable:使能控制，为1定义为wan口
    ifname：接口名称ethx
    返回值：无
*/ 
int __must_check iof_set_switch_port_wan(char *ifname, unsigned int enable);

/*
    端口重新自协商
    port_id:输入参数，端口号
    返回值：非0执行失败，0函数执行成功
*/
int __must_check iof_port_restart_auto_nego(unsigned int port_id);

/*
    传入参数: mac_addr 需要克隆的本机mac地址
    返回值：非0执行失败，0函数执行成功
*/
int __must_check iof_mac_clone_set(unsigned char *mac_addr);

int __must_check iof_vlan_init(void);
int __must_check iof_vlan_passthrough_init(void);

/*
    通过mac地址查找数据包来自哪一个物理端口
    mac_addr: 输入参数，需要查找的mac地址
    port_id:输入参数，返回mac地址所在的物理端口
    返回值务必检查返回值，非0失败，0执行成功
*/
int __must_check iof_fdb_port_by_mac(unsigned char *mac_addr,
    unsigned int *port_id);

/*
    上层必须检查返回值，如果不为空用完后
    必须遍历链表释放掉所有节点
*/
struct fdb_mac *iof_fdb_wired_mac_table(void);
/*
    设置VLAN
    vid：输入参数，vlan id号
    mpbmp：输入参数，vid包含的端口，使用bit map形式
    ut_pbmp：输入参数，vid包含的不带tag的端口，使用bit map形式
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_vlan_set(unsigned int vid, unsigned int mpbmp,
    unsigned int ut_pbmp);
/*
    设置端口默认VLAN
    port_id：输入参数，端口号，0 - 4
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_pvid_set(unsigned int port_id, unsigned int pvid);
/*
    设置所有端口默认VLAN
    pvid：输入参数，调用者保证不传空指针，按端口的顺序依次写默认vlan，
    如“1,1,1,1,2”
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_pvid_all_set(char *pvid);
/*
    配置igmp，用在iptv上
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_api_init();
/*
    初始化调用接口，
    返回值澹:务必检查返回值，非0，执行失败，0:执行成功
*/
int __must_check iof_igmp_config(struct igmp_conf *conf);

int __must_check iof_mac_addr_set(unsigned char *mac_addr);
int __must_check iof_mac_addr_get(unsigned char *mac_addr);

char *iof_eth_name_get(eth_idx_t if_id);
int iof_set_eth_name(eth_idx_t if_id, char *new_name);
int iof_get_port_by_eth_name(char *eth_name);
int iof_get_eth_name_by_port(int port, char *eth_name, int size);

/*
return 0 failed
return 1 success
*/
int iof_get_statis_info_by_eth_name(char *eth_name, struct statis_info *info);

/*
    通过mac地址查询对应dev_name
    输入mac:地址
               dev_len:输出参数存储空间大小
    输出:devname
*/
int iof_fdb_find_dev_by_mac(unsigned char *mac,  char *devname, unsigned int dev_len);

/*
    按键绑定信号
    id:输入参数，枚举gpio_id_t类型值，其他值为非法
    signal:输入参数，为绑定信号, 信号不为0
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_set_signal(gpio_id_t id, unsigned int signal);

/*
    按键绑定进程pid
    id:输入参数，枚举gpio_id_t（按键）类型值，其他值为非法
    pid:输入参数，为进程号, 进程号不为0.
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_set_pid(gpio_id_t id, unsigned int pid);

/*
    设置led共享shared_start
    shared_start:输入参数，0:关闭，1：开启。
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_set_led_shared_start(unsigned int shared_start);

/*
    设置led共享频率shared_freq
    shared_freq:输入参数，调用确保该值大于0
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_set_led_shared_freq(unsigned int shared_freq);

/*
    设置led是否加入共享定时器中
    id:输入参数，枚举gpio_id_t类型值，其他值为非法
    shared_timer:输入参数，0:清除，1：加入。
    返回值：务必检查返回值，非0：函数执行失败，0：函数执行成功
*/
int __must_check iof_gpio_set_led_index_shared_timer(gpio_id_t id, unsigned int shared_timer);

/*
    判断是否是复位按键信号
    sig_val：收到的信号参数值
    1：是复位按键信号，0：非复位按键信号
*/
static inline int iof_gpio_reset_signal(int sig_val)
{
    return !!(sig_val & 0x1);
}

/*
    判断是否是WPS按键信号
    sig_val：收到的信号参数值
    1：是WPS按键信号，0：非WPS按键信号
*/
static inline int iof_gpio_wps_signal(int sig_val)
{
    return !!(sig_val & 0x100);
}

/*
    判断是否是WIFI按键信号
    sig_val：收到的信号参数值
    1：是WIFI按键信号，0：非WIFI按键信号
*/
static inline int iof_gpio_wifi_signal(int sig_val)
{
    return !!(sig_val & 0x10000);
}

int __must_check iof_port_id_get_from_signal(int sig_val,
    unsigned int *port_id);
int __must_check iof_link_get_from_signal(int sig_val, unsigned int *link);

/*
    设置网络工作模式：支持路由模式，WIPS模式，AP模式
*/
int __must_check iof_network_mode_set(net_mode_t net_mode);

/*
 *Set mac address on wan interface
 *INPUT
 * - wanid    : eg(WAN_1,WAN_2,WAN_3,WAN_4)
 * - macaddr: the mac address need set on wan interface
 * - mode:    depend on workmode set rx rule
 *Return
 * - 0:success
 * - >0:failure
*/
int iof_ifconfig_wan_up(eth_idx_t wanid, char *macaddr, net_mode_t mode);

int iof_vlan_del(unsigned int vid);

struct iof_portdesc{
    int phyid;
    char name[8];
};

/*
 *iptv init ,call this function when enable iptv
 *INPUT
 * - vlanid     : the iptv's vlan id
 * - wanid      : eg(P_WAN_1,P_WAN_2,P_WAN_3,P_WAN_4)
 * - iof_eth_idx2phyid : the func used for convert logical id to phyid or port's name
 *RETURN
 * 0 : success
 * others : failure
*/
int iof_iptv_init(int vlanid
                    ,eth_idx_t wanid
                    ,int untag
                    ,int (*iof_eth_idx2phyid)(eth_idx_t,struct iof_portdesc *));

/*
 *iptv exit ,call this function when disable iptv
 *INPUT
 * - vlanid     : the iptv's vlan id
 * - wanid      : eg(P_WAN_1,P_WAN_2,P_WAN_3,P_WAN_4)
 * - iof_eth_idx2phyid : the func used for convert logical id to phyid or port's name
 *RETURN
 * 0 : success
 * others : failure
*/
int iof_iptv_exit(int vlanid
                    ,eth_idx_t wanid
                    ,int (*iof_eth_idx2phyid)(eth_idx_t,struct iof_portdesc *));

/*
 *creat a iptv vlan,and set a lan port as iptv port
 *INPUT
 * - vlanid : iptv vlan
 * - wanid  : wan port's logical id(eg :P_WAN_1)
 * - iptvid : iptv port's logical id (eg: P_LAN_1)
 * - mode   :
 * - iof_eth_idx2phyid : the function use to map port's logical id to physical id
 *RETURN
 * 0 : SUCCESS
 * others : FAILURE
*/
int iof_iptv_setport(int vlanid
                    ,eth_idx_t wanid
                    ,eth_idx_t iptvid
                    ,net_mode_t mode
                    ,int (*iof_eth_idx2phyid)(eth_idx_t,struct iof_portdesc *));



/*
 *remove a iptv port from iptv  vlan,and rset the iptv port as normal lan port
 *INPUT
 * - vlanid : iptv vlan
 * - wanid  : wan port's logical id(eg :P_WAN_1)
 * - iptvid : iptv port's logical id (eg: P_LAN_1)
 * - mode   :
 * - iof_eth_idx2phyid : the function use to map port's logical id to physical id
 *RETURN
 * 0 : SUCCESS
 * others : FAILURE
*/
int iof_iptv_removeport(int vlanid
                    ,eth_idx_t wanid
                    ,eth_idx_t iptvid
                    ,net_mode_t mode
                    ,int (*iof_eth_idx2phyid)(eth_idx_t,struct iof_portdesc *));
/*
 *add a wifi port to iptv  vlan,some times mybe need add one or more wifi port to iptv vlan
 *INPUT
 * - vlanid     : iptv vlan
 * - portname   : wifi port's name(eg :wl0 wlan0 wl1 wlan1)
 * - mode       :
 *RETURN
 * 0 : SUCCESS
 * others : FAILURE
*/
int iof_iptv_addwifi(int vlanid
                    ,char *portname
                    ,net_mode_t mode);

/*
 *remove a wifi port from iptv vlan;Reset the wifi port to lan vlan
 *INPUT
 * - vlanid     : iptv vlan
 * - portname   : wifi port's name(eg :wl0 wlan0 wl1 wlan1)
 * - mode       :
 *RETURN
 * 0 : SUCCESS
 * others : FAILURE
*/
int iof_iptv_removewifi(int vlanid
                    ,char *portname
                    ,net_mode_t mode);


/*
 *this may call affter iof_ifconfig_wan_up be called
 *INPUT
 * - vlanid : iptv vlan
 * - wanid  : wan port's logical id(eg :P_WAN_1)
 * - iptvid : iptv port's logical id (eg: P_LAN_1)
 * - mode   : VLAN mode(Default or Custom)
 * - iof_eth_idx2phyid : the function use to map port's logical id to physical id
 *RETURN
 * 0 : SUCCESS
 * others : FAILURE
*/
int iof_iptv_addrule_end(int vlanid
                    ,eth_idx_t wanid
                    ,eth_idx_t iptvid
                    ,net_mode_t mode
                    ,int (*iof_eth_idx2phyid)(eth_idx_t,struct iof_portdesc *));

#endif

