/*
 * Copyright (C) 2018, Tenda Corporation. All Rights Reserved.
 * desc: 本头文件是私有模块提供给内核的唯一头文件
 * date: 2018-1-15
 * author:kuangdaozhen
 */
#ifndef _KM_COMMON_H__
#define _KM_COMMON_H__

#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/timer.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/version.h>
#include <../net/bridge/br_private.h>
#include <../net/8021q/vlan.h>
#include <linux/netdevice.h>

#define CTFE_OK         0 /* Success */
#define CTFE_ERROR      -1   /* Error generic */
#define CTFE_BADARG     -2    /* Bad Argument */
#define CTFE_NOTFOUND   -30   /* Not Found */
#define CTF_FALSE       0
#define CTF_TRUE        1
#define WANID_SHIFT     18  /*wanid左移和右移的位数*/
#define CTF_L2TP_OVER_IPSEC      1 /* l2tp over ipsec加密类型标志 */

#define CTF_FLAGS_CACHED          (1 << 31)    /* Indicates cached connection */
#define CTF_FLAGS_EXCLUDED        (1 << 30)
#define CTF_FLAGS_REPLY_CACHED    (1 << 1)
#define CTF_FLAGS_ORG_CACHED      (1 << 0)
/*
--------------------------------------------------------------
|    0   |   1    |   2  | 3 | 4 |5 |     6     |     7     |
|CTF_NONE|CTF_DOWN|CTF_UP|CTF_RETAIN|CTF_PPTP_L2|CTF_PPPOES |
|        | CTF_DIR_MASK  |          |           |           |
-----------------------------------------------------------------------------------
|    8     |   9     |   10  |  11   |  12        |    13   |14     |  15         |
|CTF_PPPOE |CTF_TAG  |CTF_ORI|CTF_REP|CTF_TAG_DONE|CTF_DMZ_R|CTF_DMZ|CTF_ET_PACKET|
-----------------------------------------------------------------------------------
|    16       |   17           |   18           |19:25|   26                |
|CTF_WL_PACKET|CTF_FORWARD_POOL|CTF_PPTP_L2_XMIT|WANID|CTF_ACCELERATE_PACKET|
|    27              |   28                    |  29     |  30:32 |
|CTF_SLOW_XMIT_PACKET|FAST_L2_ACCELERATE_PACKET|CTF_IPSEC|NOT USED|
----------------------------------------------------------------
*/
#define CTF_NONE          0x0000
#define CTF_DOWN          0x0001
#define CTF_UP            0x0002
#define CTF_DIR_MASK      0x0003
#define CTF_RETAIN        0x001c      /*此字段作为保留字段，当前没有使用*/
#define CTF_PPTP_L2       0x0020
#define CTF_PPPOES        0x0040
#define CTF_PPPOE         0x0080
#define CTF_TAG           0x0100      /*此报文将从vlan接口发出，需要添加vid */
#define CTF_ORI           0x0200      /* 连接跟踪方向*/
#define CTF_REP           0x0400
#define CTF_TAG_DONE      0x0800      /*vid 已经打上，防止再次加vid */
#define CTF_DMZ_R         0x1000      /*在开启网桥netfilter时，标记在网桥中做了DNAT且找到路由的处理逻辑 */
#define CTF_DMZ           0x2000      /*作为多条NAT的前向标记 */

#define CTF_ET_PACKET     0x4000
#define CTF_WL_PACKET     0x8000
#define CTF_FORWARD_POOL  0x010000
#define CTF_PPTP_L2_XMIT  0x020000

#define CTF_PROCESSED     0x20000000  /* 标记此报文已经过fastnat处理 不需要重复处理 */

#define CTF_WAN           0x01fc0000  /*wanid使用从0x40000后的高7位来表示，总共127个wan，得到的wanid需要左移18位才能对应127个wanid*/
#define CTF_CT_DIR_BIT    0x09
#define CTF_ACCELERATE_PACKET 0x02000000 /* 标记该数据包是否是Fastnat 加速的数据包 */
#define CTF_SLOW_XMIT_PACKET 0x04000000 /* 判断Fastnat 发送该数据包时是否打开了slow xmit 慢加速开关 */
#define FAST_L2_ACCELERATE_PACKET 0x08000000 /* 标记该数据包是否是Fast_l2 加速的数据包 */
#define CTF_OR_FAST_L2_ACCELERATE_PACKET 0x0a000000 /* 标记该数据包是否是Fast_l2 或Fastnat 加速的数据包 */
#define CTF_IPSEC         0x10000000    /*标记 ipsec 加密 是否经过fastnat转发*/

#define CTF_SET_BIT(skb, x)  ((skb)->dir_ctf |= (x))
#define CTF_TEST_BIT(skb, x) ((skb)->dir_ctf & (x))
#define CTF_GET_DIR(skb)     ((skb)->dir_ctf & CTF_DIR_MASK)      //用来获取数据包上下行方向
#define CTF_GET_CT_DIR(skb)  ((skb)->dir_ctf >> CTF_CT_DIR_BIT & CTF_DIR_MASK)         //用来获取连接跟踪方向 0/1
#define CTF_GET_WANID(skb)   ((skb->dir_ctf & CTF_WAN) >> WANID_SHIFT)

/* PX_源dev_目的dev
  用来表征处理模式，add ppp头部还是del ppp头部*/

#define PX_STATIC_STATIC           0x1111

#define PX_STATIC_PPPOE            0x1100            /* add pppoe */
#define PX_STATIC_L2TP             0x1101            /* add l2tp */
#define PX_STATIC_PPTP             0x1102            /* add pptp */

#define PX_PPPOE_STATIC            0x0011            /* del pppoe */
#define PX_L2TP_STATIC             0x0111            /* del l2tp */
#define PX_PPTP_STATIC             0x0211            /* del pptp */

#define PX_PPPOE_PPPOE             0x0               /* change ssid */
#define PX_PPPOE_L2TP              0x01              /* del pppoe + add l2tp*/
#define PX_PPPOE_PPTP              0x02              /* del pppoe + add pptp */

#define PX_L2TP_PPPOE              0x0100            /* del l2tp + add pppoe */
#define PX_PPTP_PPPOE              0x0200            /* del pptp + add pppoe */

#define PPTP_RO_L2TP_FROM_FASTNAT_ACTION             1
#define PPTP_RO_L2TP_FROM_FASTNAT_OVER               -1

/*l2tp field*/
#define KM_L2TP_HDRFLAG_T       0x8000
#define KM_L2TP_HDRFLAG_L       0x4000
#define KM_L2TP_HDRFLAG_S       0x0800
#define KM_L2TP_HDRFLAG_O       0x0200
#define KM_L2TP_HDRFLAG_P       0x0100
#define KM_L2TP_HDR_VER_2       0x0002

/*
    20210118,wangjianqiang
    wan口的配置，使用dev->flags变量的最高位
*/
/*设置dev为wan口*/
#define KM_IFF_WAN 0x80000000
/* 最高位设置wan标记，后续7位设置wanid,后续删除wan标记只使用wanid */
#define KM_IFF_WAN_MASK 0xFF000000
#define KM_IFF_WAN_SHIFT 24 //得到wanid需要左移和右移的位数

#define KM_WAN_IFF_SET_WANID(dev, wanid) ((dev)->flags |= (((wanid) << KM_IFF_WAN_SHIFT) & KM_IFF_WAN_MASK))

#define KM_WAN_IFF_GET_WANID(dev) ((((dev)->flags) & (~KM_IFF_WAN)) >> KM_IFF_WAN_SHIFT)
#define KM_WAN_IFF_RESET_WANID(dev) ((dev)->flags &= (~(KM_IFF_WAN_MASK)))
#define KM_WAN_IFF_TEST_WANID(dev)((((dev)->flags & (~KM_IFF_WAN)) & KM_IFF_WAN_MASK) ? 1 : 0)

/* 兼容处理，暂时保留，后续不再使用wan标记，直接使用wanid */
#define KM_WAN_IFF_SET_BIT(dev) ((dev)->flags |= (KM_IFF_WAN))
/*清除dev的wan口表示*/
#define KM_WAN_IFF_RESET_BIT(dev) ((dev)->flags &= (~(KM_IFF_WAN)))
/*判断dev是否被设置为wan口*/
#define KM_WAN_IFF_TEST_BIT(dev) ((dev)->flags & KM_IFF_WAN)
/*
20200711,wangjianqiang
end
*/
#define INCREASE_RX_PPTP_OR_L2TP_FROM_FASTNAT(dev, storage)  \
{                                                         \
   struct net_device *net_dev = (struct net_device*)dev;    \
   struct rtnl_link_stats64 *stats = (struct rtnl_link_stats64 *)storage; \
   if(net_dev->rx_flow_flag == PPTP_RO_L2TP_FROM_FASTNAT_ACTION){        \
        stats->rx_bytes += net_dev->stats.rx_bytes;                       \
        stats->rx_packets += net_dev->stats.rx_packets;                   \
    }                                                                     \
}

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif

#if defined(__GNUC__)
    #define BWL_PRE_PACKED_STRUCT
    #define BWL_POST_PACKED_STRUCT __attribute__((packed))
#else
    #define BWL_PRE_PACKED_STRUCT
    #define BWL_POST_PACKED_STRUCT
#endif

BWL_PRE_PACKED_STRUCT struct ctf_ether_header {
    u_int8_t    ether_dhost[ETHER_ADDR_LEN];
    u_int8_t    ether_shost[ETHER_ADDR_LEN];
    u_int16_t   ether_type;
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct ctf_ether_addr {
    u_int8_t octet[ETHER_ADDR_LEN];
} BWL_POST_PACKED_STRUCT;

#undef    BWL_PRE_PACKED_STRUCT
#undef    BWL_POST_PACKED_STRUCT

/*提供给外部函数、hook点调用的代码定位宏*/
#define KM_LOCATE_TENDA(x) (x)

#define CTF_ACTION_TAG          (1 << 0)
#define CTF_ACTION_UNTAG        (1 << 1)
#define CTF_ACTION_SNAT         (1 << 2)
#define CTF_ACTION_DNAT         (1 << 3)
#define CTF_ACTION_PPPOE_ADD    (1 << 4)
#define CTF_ACTION_PPPOE_DEL    (1 << 5)
#define CTF_ACTION_PPPOE        (1 << 6)
#define CTF_ACTION_PPTP_ADD     (1 << 7)
#define CTF_ACTION_PPTP_DEL     (1 << 8)
#define CTF_ACTION_L2TP_ADD     (1 << 9)
#define CTF_ACTION_L2TP_DEL     (1 << 10)
#define CTF_ACTION_CONN_CLOSE   (1 << 11)
#define OUT_ACTION              (CTF_ACTION_PPPOE_ADD | CTF_ACTION_PPTP_ADD | CTF_ACTION_L2TP_ADD)
#define IN_ACTION               (CTF_ACTION_PPPOE_DEL | CTF_ACTION_PPTP_DEL | CTF_ACTION_L2TP_DEL)

#define CTF_ENAB(ci)        ((ci) != NULL && (ci)->_ctf)
#ifdef CONFIG_SA_FASTPATH_IPV6
#define CTF_ENAB_V6(ci)        ((ci) != NULL && (ci)->_ctf_v6)
#endif
#define CTF_NOT_NULL(ci)        ((ci) != NULL)
#define ctf_enable()    (CTF_ENAB(kcih) ? CTFE_OK : CTFE_ERROR)
#if defined (CTFPOOL) || defined (CTFMAP)
#define ctf_forward(osh, p, d)    (CTF_ENAB(kcih) ? (kcih)->forward(kcih, osh, p, d) : CTFE_ERROR)
#else
#define ctf_forward(p, d)    (CTF_ENAB(kcih) ? (kcih)->forward(kcih, p, d) : CTFE_ERROR)
#define ctf_forward_eth_protocol(p, d) (CTF_ENAB(kcih) ? (kcih)->forward_eth_protocol(kcih, p, d) : CTFE_ERROR)
#endif

#define ctf_ipc_add(skb, hooknum, ct, ctinfo, manip) do {    \
    (CTF_ENAB(kcih) ? (kcih)->ipc_add(kcih, skb, hooknum, ct, ctinfo, manip) : CTFE_ERROR);    \
} while (0)
#define ctf_ipc_delete(ct)    \
    (CTF_NOT_NULL(kcih) ? (kcih)->ipc_delete(kcih, ct) : CTFE_OK)
#define ctf_ipc_count_get(i) \
    (CTF_ENAB(kcih) ? (kcih)->ipc_count_get(kcih, i) : CTFE_OK)
#define ctf_ipc_lkup_rcu(sip, dip, p, sp, dp, check)    \
    (CTF_ENAB(kcih) ? (kcih)->ipc_lkup_rcu(kcih, sip, dip, p, sp, dp, check) : NULL)
#define ctf_ipc_lkup(sip, dip, p, sp, dp, check)    \
    (CTF_ENAB(kcih) ? (kcih)->ipc_lkup(kcih, sip, dip, p, sp, dp, check) : NULL)
#ifdef CONFIG_SA_FASTPATH_IPV6
#define ctf_ipc_v6_add(skb) do {    \
    if (CTF_ENAB_V6(kcih)) {        \
        (kcih)->ipc_v6_add(kcih, skb); \
    } } while (0)
#endif
#define ctf_dev_delete_ipc(dev)  (CTF_NOT_NULL(kcih) ? (kcih)->ipc_dev_delete(kcih, dev) : CTFE_OK)
#define ctf_session_delete_ipc(session)  do {if (CTF_NOT_NULL(kcih)) (kcih)->ipc_session_delete(kcih, session);} while (0)
#define ctf_add_vlan_tag(skb, ct) do {if (CTF_ENAB(kcih)) ((kcih)->add_vlan_tag(kcih, skb, ct));} while (0)
//for bridge
#define ctf_brc_notify(group, type, data) (CTF_ENAB(kcih) ? (kcih)->brc_notify(kcih, group, type, data): CTFE_ERROR)
#define ctf_brc_lkup(e)        (CTF_ENAB(kcih) ? (kcih)->brc_lkup(kcih, e) : NULL)
#define ctf_brc_lkup_brid(e, id)    (CTF_ENAB(kcih) ? (kcih)->brc_lkup_brid(kcih, e, id) : NULL)
#define ctf_brc_portal_enable(enable) do {if (CTF_ENAB(kcih)) (kcih)->portal_en = (enable);} while (0)
#define ctf_brc_portal_change(e, state) do {if (CTF_ENAB(kcih)) (kcih)->brc_portal_change(kcih, e, state);} while (0)
#ifdef PKTC
#define ctf_pktc_enable(d, e, b)    (CTF_NOT_NULL(kcih) ? (kcih)->enable(kcih, d, e, b) : CTFE_OK)
#define ctf_pktc_dev_register(d, b)    (CTF_NOT_NULL(kcih) ? (kcih)->dev_register(kcih, d, b) : CTFE_OK)
#define ctf_pktc_dev_unregister(d)    do {if (CTF_NOT_NULL(kcih)) (kcih)->dev_unregister(ci, d);} while (0)
#endif
#define ctf_register_hook(reg)   (CTF_NOT_NULL(kcih) ? (kcih)->register_hook(kcih, reg) : CTFE_ERROR)
#define ctf_unregister_hook(reg)   (CTF_NOT_NULL(kcih) ? (kcih)->unregister_hook(kcih, reg) : CTFE_ERROR)

#define ctf_register_proc_filesystem(reg, len, parent_name) \
    (CTF_NOT_NULL(kcih) ? (kcih)->register_proc_filesystem(kcih, reg, len, parent_name) : CTFE_OK)
#define ctf_unregister_proc_filesystem(reg, len, parent_name, parent_dir) \
    (CTF_NOT_NULL(kcih) ? (kcih)->unregister_proc_filesystem(kcih, reg, len, parent_name, parent_dir) : CTFE_OK)
#define ctf_nf_conntrack_ext_alloc(ct) (CTF_NOT_NULL(kcih) ? (kcih)->ext_alloc(ct) : CTFE_ERROR)
#define ctf_nf_conntrack_ext_free(ct) (CTF_NOT_NULL(kcih) ? (kcih)->ext_free(ct) : CTFE_ERROR)
#ifdef CONFIG_SA_FASTPATH_NAT_PPTP
#define ctf_pptp_compress_packet(payload) (CTF_ENAB(kcih) ? (kcih)->pptp_compress_packet(payload) : CTF_FALSE)
#define ctf_pptp_control_packet(payload) (CTF_ENAB(kcih) ? (kcih)->pptp_control_packet(payload) : CTF_FALSE)
#endif

/* wifievent sta status message */
#ifdef CONFIG_TENDA_PRIVATE_KM
typedef enum sta_status_msg_type {
    KM_WIRELESS_STA_NONE                = 0,
    KM_WIRELESS_STA_OFFLIN              = 1,
    KM_WIRELESS_STA_AUTH                = 2,
    KM_WIRELESS_STA_DEAUTH              = 3,
    KM_WIRELESS_STA_ASSOC_REQ           = 4,
    KM_WIRELESS_STA_REASSOC_REQ         = 5,
    KM_WIRELESS_STA_DISASSOC            = 6,
} sta_status_msg_type_t;
#endif

enum {
    CTF_BRIDGE_ROUTING,
    CTF_IP_PRE_ROUTING,
    CTF_IP_POST_ROUTING,
    CTF_DEV_XMIT_ROUTING,
    CTF_MAX_HOOKS,
};

enum {
    CTFPROTO_UNSPEC =  0,
    CTFPROTO_IPV4   =  2,
    CTFPROTO_IPV6   = 10,
    CTFPROTO_NUMPROTO,
};

enum ctf_hook_priorities {
    CTF_HOOK_PRI_FIRST = INT_MIN,
    CTF_HOOK_PRI_BRIDGE_ROUTING = 0,
    CTF_HOOK_PRI_IP_PRE_ROUTING = 0,
    CTF_HOOK_PRI_IP_POST_ROUTING = 0,
    CTF_HOOK_PRI_DEV_XMIT_ROUTING = 0,
    CTF_HOOK_PRI_LAST = INT_MAX,
};
#define CTF_DEFAULT 255 //赋初值使用
#define CTF_DROP 0
#define CTF_ACCEPT 1
#define CTF_STOLEN 2    //协议栈接管该数据包，fastnat不需要进行处理
#define CTF_MAX_VERDICT CTF_STOLEN
#define CTF_VERDICT_MASK 0x00000003

typedef u_int32_t ctf_hookfn(u_int32_t hooknum, u_int8_t pf, struct sk_buff *skb);
struct ctf_hook_ops {
    struct list_head list;
    ctf_hookfn *hook;
    struct module *owner;
    u_int8_t pf;   //对应不同的协议族，如PF_INET
    u_int32_t hooknum;
    int32_t priority;
    int8_t *name;
};

struct ctf_proc_filesystem_entry {
    char *name;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,90)
    void *proc_read;    //简单的proc文件系统操作需要赋值，seq文件操作不需要赋值
    void *proc_write;   //简单的proc文件系统操作需要赋值，seq文件操作不需要赋值
#endif
    umode_t mode;
    struct file_operations *file_ops; //如果需要支持seq文件操作需要给该参数赋值，简单的proc操作不需要
};

/*
 * define for the qos to process the skb because of ctfpool and ctfmap
 */
#if defined (CTFPOOL) || defined (CTFMAP)
#define CTF_SKB_FREE(skb)     kfree(skb)
#define CTF_SKB_XMIT(skb)    dev_queue_xmit(skb)
#else
#define CTF_SKB_FREE(skb)    kfree_skb(skb)
#define CTF_SKB_XMIT(skb)    dev_queue_xmit(skb)
#endif

typedef struct ctf_fn  ctf_t;
typedef struct ctf_ipc    ctf_ipc_t;
#ifdef CONFIG_SA_FASTPATH_IPV6
typedef struct ctf_ipc_v6   ctf_ipc_v6_t;
#endif
typedef struct ctf_conn_tuple    ctf_conn_tuple_t;
typedef struct ctf_conn_tuple_v6    ctf_conn_tuple_v6_t;

typedef struct ctf_brc ctf_brc_t;
#ifdef PKTC
typedef struct ctf_brc_hot ctf_brc_hot_t;
#endif

typedef void (*ctf_detach_cb_t)(ctf_t *ci, void *arg);
typedef ctf_t * (*ctf_attach_t)(void *osh, u_int8_t *name, u_int32_t *msg_level,
                                ctf_detach_cb_t cb, void *arg);
#if defined (CTFPOOL) || defined (CTFMAP)
typedef int32_t (*ctf_forward_t)(ctf_t *ci, void *osh, void *p, void *rxifp);
#else
typedef int32_t (*ctf_forward_t)(ctf_t *ci, void *p, void *rxifp);
#endif

typedef void (*ctf_ipc_add_t)(ctf_t *ci, struct sk_buff *skb, u_int32_t hooknum,
                      struct nf_conn *ct, enum ip_conntrack_info ctinfo,
                    struct nf_conntrack_tuple *manip);
typedef int (*ctf_ipc_delete_t)(ctf_t *ci, struct nf_conn *ct);

typedef int32_t (*ctf_ipc_count_get_t)(ctf_t *ci);
typedef ctf_ipc_t * (*ctf_ipc_lkup_rcu_t)(ctf_t *ci, u_int32_t sip, u_int32_t dip, u_int8_t proto,
                                    u_int16_t sp, u_int16_t dp, bool brc_update_check);
typedef ctf_ipc_t * (*ctf_ipc_lkup_t)(ctf_t *ci, u_int32_t sip, u_int32_t dip, u_int8_t proto,
                                    u_int16_t sp, u_int16_t dp, bool brc_update_check);
typedef int32_t (*ctf_ipc_dev_delete_t)(ctf_t *ci, void *dev);

typedef void (*ctf_ipc_delete_session_t)(ctf_t *ci, void *session);
#ifdef CONFIG_SA_FASTPATH_IPV6
typedef void (*ctf_ipc_v6_add_t)(ctf_t *ci, struct sk_buff *skb);
#endif
typedef void (*ctf_add_vlan_tag_t)(ctf_t *ci, struct sk_buff *skb, struct nf_conn *ct);

/* for the bridge*/
typedef void (*ctf_brc_notify_t)(ctf_t *ci, int group, int type, const void *data);

typedef ctf_brc_t * (*ctf_brc_lkup_t)(ctf_t *ci, const u_int8_t *da);
typedef ctf_brc_t * (*ctf_brc_lkup_rcu_t)(ctf_t *ci, const u_int8_t *da);
typedef ctf_brc_t * (*ctf_brc_lkup_brid_t)(ctf_t *ci, const u_int8_t *da, u_int16_t id);
typedef ctf_brc_t * (*ctf_brc_lkup_brid_rcu_t)(ctf_t *ci, const u_int8_t *da, u_int16_t id);
typedef int32_t (*ctf_brc_portal_change_t)(ctf_t *ci, u_int8_t *da, u_int8_t state);

#ifdef PKTC
typedef int32_t (*ctf_enable_t)(ctf_t *ci, void *dev, bool enable, ctf_brc_hot_t **brc_hot);
typedef int32_t (*ctf_dev_register_t)(ctf_t *ci, void *dev, bool br);
typedef void (*ctf_dev_unregister_t)(ctf_t *ci, void *dev);
struct ctf_brc_hot {
    struct ctf_ether_addr    ea;    /* Dest address */
    ctf_brc_t        *brcp;    /* BRC entry corresp to dest mac */
};
#endif
typedef int32_t (*ctf_register_hook_t)(ctf_t *ci, struct ctf_hook_ops *reg);
typedef int32_t (*ctf_unregister_hook_t)(ctf_t *ci, struct ctf_hook_ops *reg);

typedef struct proc_dir_entry *(*ctf_register_proc_filesystem_t)(ctf_t *ci,
    struct ctf_proc_filesystem_entry *reg, int len, char *parent_name);
typedef void (*ctf_unregister_proc_filesystem_t)(ctf_t *ci,
    struct ctf_proc_filesystem_entry *reg, int len, char *parent_name, struct proc_dir_entry *parent_dir);

typedef int32_t (*ctf_nf_conntrack_ext_alloc_t)(struct nf_conn *ct);
typedef int32_t (*ctf_nf_conntrack_ext_free_t)(struct nf_conn *ct);

#ifdef CONFIG_SA_FASTPATH_NAT_PPTP
typedef int (*ctf_pptp_compress_packet_t)(u8 *payload);
typedef int (*ctf_pptp_control_packet_t)(u8 *payload);
#endif

struct ctf_fn {
#if defined (CTFPOOL) || defined (CTFMAP)
    void *et_osh; /*add for ctfpool,ctfmap in driver */
    void *wl_osh;
#endif
    u_int8_t portal_en;
    ctf_forward_t forward;
    ctf_forward_t forward_eth_protocol; //驱动调用eth_type_trans已经对数据包data进行了偏移，快转时需要还原
    ctf_ipc_add_t ipc_add;
    ctf_ipc_delete_t ipc_delete;
    ctf_ipc_count_get_t ipc_count_get;
    ctf_ipc_lkup_rcu_t ipc_lkup_rcu;
    ctf_ipc_lkup_t ipc_lkup;
    ctf_ipc_dev_delete_t ipc_dev_delete; /* pppoe 断开重连*/
    ctf_ipc_delete_session_t ipc_session_delete;
#ifdef CONFIG_SA_FASTPATH_IPV6
    ctf_ipc_v6_add_t ipc_v6_add;
#endif
    ctf_add_vlan_tag_t add_vlan_tag;
    ctf_brc_notify_t brc_notify;      /* for the bridge*/
    ctf_brc_lkup_t brc_lkup;
    ctf_brc_lkup_rcu_t brc_lkup_rcu;
    ctf_brc_lkup_brid_t brc_lkup_brid;
    ctf_brc_lkup_brid_rcu_t brc_lkup_brid_rcu;
    ctf_brc_portal_change_t brc_portal_change; /*for the bridge portal*/
#ifdef PKTC
    ctf_enable_t enable;
    ctf_dev_register_t dev_register;
    ctf_dev_unregister_t dev_unregister;
#endif
    u_int8_t _ctf;
#ifdef CONFIG_SA_FASTPATH_IPV6
    u_int8_t _ctf_v6;
#endif
    ctf_register_hook_t register_hook;
    ctf_unregister_hook_t unregister_hook;
    ctf_register_proc_filesystem_t register_proc_filesystem;
    ctf_unregister_proc_filesystem_t unregister_proc_filesystem;
    ctf_nf_conntrack_ext_alloc_t ext_alloc;
    ctf_nf_conntrack_ext_free_t ext_free;
#ifdef CONFIG_SA_FASTPATH_NAT_PPTP
    ctf_pptp_compress_packet_t pptp_compress_packet;
    ctf_pptp_control_packet_t pptp_control_packet;
#endif
};

struct ctf_brc {
    struct hlist_node    hlist;  /* brc entry Pointer  */
    struct rcu_head      rcu;    /* brc rcu Pointer  */
    void *txif;        /* Interface connected to host */
    void *txvifp;        /* vlan Interface connected to host*/
    u_int32_t live;        /* Counter used to expire the entry */
    u_int16_t vid;        /* VLAN id to use on txif */
    u_int8_t action;        /* Tag or untag the frames */
    u_int8_t state;
    u_int16_t br_id;   /*对ap使用,保证在任何情况下路由器不会在不同桥内转发*/
    struct ctf_ether_addr dhost;        /* MAC addr of host */
    unsigned long last_jiffies;        /*record the update time*/
    atomic_t use;                      /*ref cnt*/
};

struct ctf_conn_tuple {
    u_int32_t sip, dip;
    u_int16_t sp, dp;
    u_int8_t proto;
};

struct ctf_conn_tuple_v6 {
    struct in6_addr sip, dip;
    u_int16_t         sp, dp;
    u_int8_t         proto;
};

typedef struct ctf_nat {
    u_int16_t port;
    u_int32_t ip;
} ctf_nat_t;

struct ctf_ipc {
    struct hlist_node        hlist;  /* ipc entry Pointer  */
    struct rcu_head          rcu;    /* ipc rcu Pointer  */
    ctf_brc_t *brcp;             /*Pointer to a brc, we want to update txif*/
    unsigned long last_jiffies;  /*record updte time*/
    void *txif;        /* Target interface to send */
    u_int32_t action;        /* NAT and/or VLAN actions */
    struct nf_conn *nf_conntrack;
    struct ctf_ether_addr dhost;        /* Destination MAC address */
    struct ctf_ether_addr shost;        /* Source MAC address */
    ctf_conn_tuple_t tuple;    /* Tuple to uniquely id the flow */
    u_int8_t dir;
    struct ctf_nat nat;        /* Manip data for SNAT, DNAT */
#if defined(CONFIG_SA_FASTPATH_NAT_PPTP) || defined(CONFIG_SA_FASTPATH_NAT_L2TP)
    void *pppox_opt;
#endif
    void *ppp_ifp;    /* PPP interface handle */
    u_int16_t pppoe_sid[0];    /* PPPOE session to use, sid[0]表示单侧pppoe下的ssid;ssid[1]表示双侧pppoe时的接收报文ssid */
};

struct ctf_ipc_v6 {
    struct hlist_node        hlist;  /* ipc entry Pointer  */
    struct rcu_head          rcu;    /* ipc rcu Pointer  */
    ctf_brc_t *brcp;             /*Pointer to a brc, we want to update txif*/
    unsigned long last_jiffies;  /*record updte time*/
    void *txif;        /* Target interface to send */
    u_int32_t action;        /* NAT and/or VLAN actions */
    struct nf_conn *nf_conntrack;
    struct ctf_ether_addr dhost;        /* Destination MAC address */
    struct ctf_ether_addr shost;        /* Source MAC address */
    ctf_conn_tuple_v6_t tuple_v6;    /* Tuple to uniquely id the flow */
    u_int8_t dir;
    void *ppp_ifp;    /* PPP interface handle */
    u_int16_t pppoe_sid[0];    /* PPPOE session to use, sid[0]表示单侧pppoe下的ssid??                                      ssid[1]表示双侧pppoe时的接收报文ssid */
};

struct ctf_ppp_sk {
    struct pppox_sock *po;            /*pointer to pppoe socket*/
    u_int8_t pppox_protocol;    /*0:pppoe/1:l2tp/ 2:pptp*/
    struct ctf_ether_addr dhost;            /*Remote MAC addr of host the pppox socket is bound to*/
};

typedef struct ctf_ppp {
    struct ctf_ppp_sk psk;
    u_int16_t pppox_id;    /*PPTP peer call id if wan type is pptp, PPPOE session ID if wan type is PPPOE*/
} ctf_ppp_t;

extern void ip_send_check(struct iphdr *ip);
extern void ppp_rxstats_upd(void *pppif, struct sk_buff *skb);
extern void ppp_txstats_upd(void *pppif, struct sk_buff *skb);

extern ctf_t *kcih;        /* 定义在内核dev.c */
#ifdef PKTC
/* compare two ethernet addresses - assumes the pointers can be referenced as shorts */
#define ctf_eacmp(a, b)    ((((const u_int16_t *)(a))[0] ^ ((const u_int16_t *)(b))[0]) | \
                     (((const u_int16_t *)(a))[1] ^ ((const u_int16_t *)(b))[1]) | \
                     (((const u_int16_t *)(a))[2] ^ ((const u_int16_t *)(b))[2]))

#define    ctf_ether_cmp(a, b)    ctf_eacmp(a, b)


/* Hot bridge cache lkup */
#define MAXBRCHOT    256
#define MAXBRCHOTIF    4
#define CTF_BRC_HOT_HASH(da)     ((((u_int8_t *)da)[4] ^ ((u_int8_t *)da)[5]) & (MAXBRCHOT - 1))
#define CTF_HOTBRC_CMP(hbrc, da, rxifp) \
({ \
    ctf_brc_hot_t *bh = (hbrc) + CTF_BRC_HOT_HASH(da); \
    ((ctf_eacmp((bh)->ea.octet, (da)) == 0) && (bh->brcp->txif != (rxifp))); \
})

/*
桥转是否发开启PKTC功能：
1、在有线lan到有线lan转发时关闭PKTC功能，如果不关闭PKTC性能只有几M；
2、在无线lan到有线lan转发时开启PKTC功能，提升wlan2lan转发性能；
3、判断是否转发时识别目的转发接口是否带VLAN，如果带VLAN就不通过PKTC转发
*/
#define CTF_HOTBRC_IS_FORWARD(hbrc, da, rxifp) ({\
    ctf_brc_hot_t *bh = (hbrc) + CTF_BRC_HOT_HASH(da); \
    (!bh->brcp->vid);    \
})

/* Header prep for packets matching hot bridge cache entry */
#define CTF_HOTBRC_L2HDR_PREP(osh, hbrc, prio, data, p) \
do { \
    u_int8_t *l2h; \
    ctf_brc_hot_t *bh = (hbrc) + CTF_BRC_HOT_HASH(data); \
    ASSERT(*(u_int16_t *)((data) + VLAN_TPID_OFFSET) == HTON16(ETHER_TYPE_8021Q)); \
    if (bh->brcp->action & CTF_ACTION_UNTAG) { \
        /* Remove vlan header */ \
        l2h = PKTPULL((osh), (p), VLAN_TAG_LEN); \
        ctf_ether_rcopy(l2h - VLAN_TAG_LEN + ETHER_ADDR_LEN, \
                    l2h + ETHER_ADDR_LEN); \
        ctf_ether_rcopy(l2h - VLAN_TAG_LEN, l2h); \
    } else { \
        /* Update vlan header */ \
        l2h = (data); \
        *(u_int16_t *)(l2h + VLAN_TCI_OFFSET) = \
                    HTON16((prio) << VLAN_PRI_SHIFT | bh->brcp->vid); \
    } \
} while (0)
#endif

#ifdef CONFIG_SA_FASTPATH_NAT_L2TP
struct ctf_pppol2tp_inet {
    __u32 saddr;         /* src IP address of tunnel */
    __u32 daddr;         /* src IP address of tunnel */
    __be16 sport;        /* src port                 */
    __be16 dport;        /* dst port                 */
    __u8  tos;           /* IP tos                   */
    __u8 ttl;
};

struct l2tp_opt {
    struct ctf_pppol2tp_inet    inet;
    struct l2tp_session *session;
};
#endif

#ifdef CONFIG_TRACE_SKB_RUNNING_TIME
#define KM_TRACEPOINT_MAX_NUM   32
#define FUNCTION_NAME_LEN       64

struct km_tracepoint {
    char        func_name[FUNCTION_NAME_LEN];       //TracePoint所在的函数名称
    u16         line_number;                        //TracePoint所在的行号
    u8          is_init;                            //是否初始化
};
#endif

enum {
    KM_INVALID_ETHER_PORT = 0xE0,
    KM_WRIELESS_PORT = 0xFF,
};


struct km_public_skb_km_cb {
    struct net_device *original_dev;
    void *ct;
    void *online_ip;
    void *l2_nfct;
    uint8_t l2_nfct_dir:1;
    uint8_t sdw_flag;           //sdw数据包标识
    uint8_t ether_src_port;     //数据包的以太网来源端口
    uint8_t multifreq_lb_flag;  //多频汇聚数据包标识
};

#define KM_PUBLIC_GET_SKB_KM_CB(skb) ((struct km_public_skb_km_cb *)&(skb->km_cb[0]))

extern int (*km_hook_fdb_create_hook)(struct hlist_head *head,
    struct net_bridge_fdb_entry *fdb, struct net_bridge_port *source,
    const unsigned char *addr);
extern int (*km_hook_netif_receive_skb_hook)(struct sk_buff *skb);
extern int (*km_hook___netif_receive_skb_core_hook)(struct sk_buff *skb);
extern int (*km_hook___netif_receive_skb_hook)(struct sk_buff *skb);
extern int (*km_hook_ip_rcv_hook)(struct sk_buff *skb);
extern int (*km_hook_nf_conntrack_create_prerouting_hook)(struct sk_buff *skb) ;
extern int (*km_hook_nf_conntrack_create_postrouting_hook)(struct sk_buff *skb, struct nf_conn *ct);

extern int km_driver_wireless_mesh_tx_hook(struct sk_buff *skb, struct net_device *dev);
extern int km_driver_rx_packet_hook(struct sk_buff *skb, struct net_device *dev, uint8_t port);
extern int km_driver_tx_packet_hook(struct sk_buff *skb, struct net_device *dev);

extern void km_set_ct_hook(struct sk_buff *skb);
extern void *km_public_get_ct(struct sk_buff *skb);
extern void *km_get_ct_hook(struct sk_buff *skb);
extern void km_ppp_send_frame_hook(struct sk_buff *skb);
extern void km_xfrm_output_hook(struct sk_buff *skb);

extern void km_ip_rcv_finish_hook(struct sk_buff *skb, struct net_device *dev);
extern void km_ip6_rcv_finish_hook(struct sk_buff *skb, struct net_device *dev);
extern int km_driver_wireless_tx_packet_hook(struct sk_buff *skb, struct net_device *dev);
extern void km_nf_conntrack_init_hook(struct nf_conn *ct);
extern void km_nf_conntrack_destroy_hook(struct nf_conn *ct);
extern int km_nos_rate_handle(struct sk_buff *skb);
extern int km_dev_xmit_prerouting_hook(struct sk_buff *skb);
extern void  km_vlan_do_receive_hook(struct sk_buff *skb,  struct net_device *dev);
extern void km___copy_skb_header_hook(struct sk_buff *new, const struct sk_buff *old);
extern void km_kfree_skb_hook(struct sk_buff *skb);
extern void km_br_fdb_update_hook(struct hlist_head *head,
    struct net_bridge_fdb_entry *fdb, struct net_bridge_port *source,
    const unsigned char *addr);
extern void km_fdb_delete_hook(struct net_bridge *br, struct net_bridge_fdb_entry *f);
extern int km_driver_wireless_rx_packet_hook(struct sk_buff *skb, struct net_device *dev);
extern int (*km_hook_br_handle_frame_hook)(struct sk_buff *skb);
extern void(*km_hook_br_forward_finish_hook)(struct sk_buff *skb);
extern int (*km_hook_netif_receive_skb_internal_hook)(struct sk_buff *skb, void **rflowp, int *cpu);

extern void km__alloc_skb_hook(struct sk_buff *skb);
extern u32 km_tty_driver_close;
extern int (*km_autodiscover_netlink_send_msg)(void *message, int msize);
extern void (*km_autodiscover_upload_online)(struct hlist_head *head, struct net_bridge_fdb_entry *fdb,
    struct net_bridge_port *source, const unsigned char *addr);
extern void (*km_autodiscover_upload_offline)(struct net_bridge_fdb_entry *f);
extern void (*km_autodiscover_online_ip_notify)(u_int8_t *macaddr, u_int32_t ipaddr, u_int8_t *dev_name);
extern int (*km_bm_connect_limit_action)(struct sk_buff *skb);
extern int (*km_fastpath_need_slow_learning)(struct sk_buff *skb);
extern int km_consider_fastpath_learning(struct sk_buff *skb);
extern struct net_device *km_public_get_original_dev(struct sk_buff *skb);
#ifdef CONFIG_AP_DATA_TUNNEL
extern int (*km_ap_data_tunnel)(struct sk_buff *skb, struct net_device *dev, int is_wireless);
#endif

#ifdef CONFIG_TUNNEL_FORWARD
extern int (*km_tf_do_fastpath)(struct sk_buff *skb, struct net_device *from_dev, int is_wireless);
#endif

#ifdef CONFIG_OS_IDENTIFY_GET_SYS_TYPE
extern int (*km_os_identify_get_system_type)(u8 *ethaddr);
extern int (*km_os_identify_get_system_type_from_node)(u8 *ethaddr);
#endif

extern void* km_public_get_online_ip(struct sk_buff *skb);
extern void* km_public_set_online_ip(struct sk_buff *skb, void * online_ip);

extern void(*km_wireless_client_online)(const unsigned char *mac, int is_5g,
                                struct net_device *dev, const char *ssid, const int ssid_len);
extern void(*km_wireless_client_offline)(const unsigned char *mac, struct net_device *dev);

extern void km_unregister_netdevice_hook(struct net_device *dev);

#ifdef CONFIG_SA_FASTPATH_L2
extern int (*km_fast_l2_br_handle_frame)(struct sk_buff *skb, struct net_bridge_port *p);
extern int (*km_fast_l2_dev_hard_start_xmit)(struct sk_buff *skb);
extern int (*km_fast_l2_cache_vlan_ethhdr)(struct neighbour *neigh, struct sk_buff *skb);
extern void (*km_fast_l2_ext_delete_by_dev)(struct net_device *dev);
#endif

#ifdef CONFIG_KM_SDW_FAST
extern void (*km_sdw_fast_create_tunc_hook)(char *mac, uint32_t tunl_ip,
        uint16_t tunl_port, int enable, char *alg, char *key);
extern void (*km_sdw_fast_destroy_tunc_hook)(void);

extern void (*km_sdw_fast_update_tunc_hook)(struct sk_buff *skb, char *mac, char *localmac, u_int32_t local_ip, 
                                    u_int16_t local_port, struct net_device *dev);

extern int (*km_sdw_fast_create_client_conn_hook)(struct sk_buff *skb, char *mac, uint32_t local_ip);
extern void (*km_sdw_fast_update_wanipc_hook)(u_int32_t local_ip, struct net_device *dev);

#endif

#ifdef CONFIG_FLOW_IDENTIFY
extern void* (*km_flow_identify_alloc_user_flow_stat_hook)(void **user_flow_stat);
extern void (*km_flow_identify_free_user_flow_stat_hook)(void *user_flow_stat);
extern u_int32_t (*km_flow_identify_get_flow_prio_hook)(void *flow_ide_ext);
extern int32_t (*km_flow_identify_show_flow_rate_hook)(struct seq_file *s, void *v);
extern int32_t (*km_flow_identify_show_flow_bytes_hook)(struct seq_file *s, void *v);
extern void (*km_flow_identify_clear_user_flow_stat_hook)(void *user_flow_stat);
#endif

#ifdef CONFIG_NOS_CONTROL_V2
extern int (*km_nos_skb_is_limited)(struct sk_buff *skb);
#endif

#ifdef CONFIG_KM_NF_CONNTRACK
extern void (*km_l2_nf_conntrack_driver_tx_hook)(struct sk_buff *skb);
extern int (*km_l2_nf_conntrack_driver_rx_hook)(struct sk_buff *skb, struct net_device *dev);
extern void (*km_l2_nf_conntrack_init_hook)(struct nf_conn *ct);
#ifdef CONFIG_KM_NF_CONNTRACK_WIRELESS_MESH_INFO
extern int (*km_nf_conntrack_get_mesh_info)(struct sk_buff *skb, unsigned int *id, 
                                                int *previous_ifindex, int *current_ifindex, 
                                                unsigned int *current_speed, unsigned char mesh_flag);
extern void (*km_nf_conntrack_set_mesh_info)(struct sk_buff *skb, const int previous_ifindex, 
                                                const int current_ifindex, unsigned char mesh_flag);
#endif   
#endif

#ifdef CONFIG_MESH_MULTIFREQ_LOADBALANCE
#ifdef CONFIG_TD_MESH_V3
extern int (*km_meshv3_multifreq_lb_hook)(struct sk_buff *skb, struct net_device *dev, u8 *dmac, int fc_open);
extern int (*km_meshv3_multifreq_lb_check_is_5g_and_wire_interface)(struct net_device *dev);
#else
extern int (*km_mesh_multifreq_lb_hook)(struct sk_buff *skb, struct net_device *dev, u8 *dmac);
#endif
extern void (*km_multifreq_lb_cache_free)(void *multifreq_lb_ext);
#ifdef CONFIG_TD_MESH_V3
extern int (*km_mesh_multifreq_lb_rx_detect_hook)(struct sk_buff *skb);
#endif
#endif

extern void (*km_dhcp_options_add_wireless_client)(const unsigned char *mac, const char *ssid);
extern int (*km_dhcp_options_get_client_accesspoint)(const unsigned char *mac, void *client_info);
extern int (*km_hook_br_handle_frame_finish_hook)(struct sk_buff *skb, struct net_bridge_fdb_entry *dst);
extern int (*km_hook_br_handle_frame_finish_2_hook)(struct sk_buff *skb);

#ifdef CONFIG_L2_WIRELESS_REDIRECT_AUTH
extern int (*km_l2_auth_wireless_tx_packet_hook)(struct sk_buff *skb);
extern int (*km_l2_auth_wireless_rx_packet_hook)(struct sk_buff *skb);
#endif
#ifdef CONFIG_L2_WIRE_REDIRECT_AUTH
extern int (*km_l2_auth_tx_packet_hook)(struct sk_buff *skb);
extern int (*km_l2_auth_rx_packet_hook)(struct sk_buff *skb);
#endif

#ifdef CONFIG_EVENTS_CENTER
/* 客户端更新rssi事件 */
extern void (*km_eventscenter_wifiev_sta_update_rssi_handler)(u_int8_t *ether_addr, int rssi);

/* Bss接收到对应客户端的BTM迁移回应状态事件 */
extern void (*km_eventscenter_wifiev_bss_trans_res_handler)(u_int8_t *ether_addr, u_int8_t status);

/* 客户端beacon包回应事件 */
extern void (*km_eventscenter_wifiev_beacon_rep_handler)(u_int8_t *ether_addr, void *data);

/* band设置信道事件 */
extern void (*km_eventscenter_wifiev_set_channel_handler)(int band, unsigned int channel);

/* 设置band的rssi事件 */
extern void (*km_eventscenter_wifiev_set_ssid_handler)(int band, char *ifname, unsigned char *ssid, 
    unsigned int ssid_len);

/* 客户端状态事件 */
extern void (*km_eventscenter_wifiev_sta_status_handler)(u_int8_t *ether_addr, char *dev_name, int message);

/* 客户端上线事件 */
extern void (*km_eventscenter_wifiev_sta_join_handler)(u_int8_t *ether_addr, char *dev_name, int rssi, 
    unsigned band, unsigned sup11k, unsigned sup11v);

/* 客户端下线事件 */
extern void (*km_eventscenter_wifiev_sta_leave_handler)(u_int8_t *ether_addr);

/* 客户端密码错误事件 */
extern void (*km_eventscenter_wifiev_sta_passerr_handler)(u_int8_t *ether_addr);

/* 黑名单拒绝连接事件 */
extern void (*km_eventscenter_wifiev_sta_blackrej_handler)(u_int8_t *ether_addr);

/* 因客户端的数量超过了当前ap设定支持的最大客户端接入数量而接入失败时的通知事件 */
extern void (*km_eventscenter_wifiev_sta_maxcntrej_handler)(u_int8_t *ether_addrt);

/* 检测dfs信道是否有雷达信号 */
extern void (*km_eventscenter_wifiev_dfs_chanspes_detected_handler)(u_int16_t cur_chan, u_int16_t target_chan, 
    u_int8_t radar_type);

/* 检测无线接口状态是否有变化(down/up) */
extern void (*km_eventscenter_wifiev_intf_status_changed_handler)(int band, bool state, u_int8_t *ether_addr);

/* 游戏加速流量控制等级上报 */
extern void (*km_eventscenter_wifiev_game_speed_flowcontrol_handler)(char* ifname, int status, void *data, u_int8_t data_len);

/* 客户端通过fdb上线的事件 */
extern void (*km_eventscenter_kmev_upload_online_handler)(struct hlist_head *head, 
    struct net_bridge_fdb_entry *fdb, struct net_bridge_port *source, const unsigned char *addr);

/* 客户端通过fdb下线的事件 */
extern void (*km_eventscenter_kmev_upload_offline_handler)(struct net_bridge_fdb_entry *f);

/* 客户端更新ip地址的事件 */
extern void (*km_eventscenter_kmev_online_ip_info_handler)(u_int8_t *macaddr, u_int32_t ipaddr, u_int8_t *dev_name);

/* 收到igmp报文上报源ip的事件 */
extern void (*km_eventscenter_kmev_igmp_ip_handler)(struct sk_buff *skb);

#endif

#ifdef CONFIG_SKIP_FASTPATH
extern int (*km_skip_fastpath_hook)(struct sk_buff *skb);
#endif

extern u32 km_upgrade_ignore_sig;
extern int km_upgrading_signal_hook(int signo, struct task_struct *p);

/* slow xmit import symbol */
extern int __dev_xmit_skb(struct sk_buff *skb, struct Qdisc *q, 
    struct net_device *dev, struct netdev_queue *txq);

//NFCT_PTRMASK表示的是_nfct除去最低三位剩余的位数
static inline struct nf_conn *km_get_nf_ct(struct sk_buff *skb)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 10, 17)
    return (struct nf_conn *)(skb->nfct);
#else
    return (struct nf_conn *)(skb->_nfct & NFCT_PTRMASK);
#endif
}

static inline bool km_skb_ct_is_untracked(struct sk_buff *skb)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
    return (skb->_nfct & NFCT_INFOMASK) == IP_CT_UNTRACKED;
#else
    return nf_ct_is_untracked((struct nf_conn *)skb->nfct);
#endif
}

static inline enum ip_conntrack_info km_get_nf_ctinfo(struct sk_buff *skb)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 10, 17)
    return skb->nfctinfo;
#else
    return skb->_nfct & NFCT_INFOMASK;
#endif 
}

#if LINUX_VERSION_CODE <  KERNEL_VERSION(3,10,90)
extern struct netdev_queue *dev_pick_tx(struct net_device *dev, 
    struct sk_buff *skb);
#endif

#define km_br_handle_frame_finish_hook(skb, dst)  do { \
    int km_ret = NF_ACCEPT; \
    if (km_hook_br_handle_frame_finish_hook) { \
        km_ret = km_hook_br_handle_frame_finish_hook(skb, dst); \
        if (NF_DROP == km_ret) { \
            goto drop;\
        } \
    } \
} while (0)

#define km_br_handle_frame_finish_2_hook(skb)  do { \
    int km_ret = NF_ACCEPT; \
    if (km_hook_br_handle_frame_finish_2_hook) { \
        km_ret = km_hook_br_handle_frame_finish_2_hook(skb); \
        if (NF_DROP == km_ret) {\
            goto pass;\
        } \
    } \
} while (0)

extern int (*km_hook_br_fdb_update_online_ip_hook)(struct sk_buff *skb, struct net_bridge_fdb_entry *dst);
#define km_br_fdb_update_online_ip_hook(skb, dst)  do { \
    if (km_hook_br_fdb_update_online_ip_hook) { \
        km_hook_br_fdb_update_online_ip_hook(skb, dst); \
    } \
} while (0)


#define km_nf_conntrack_create_postrouting_hook(skb, ct) do {   \
    if (km_hook_nf_conntrack_create_postrouting_hook) { \
        if (NF_DROP == km_hook_nf_conntrack_create_postrouting_hook(skb, ct)){ \
            ret = NF_DROP;  \
        }\
    }\
} while (0)

#define km_nf_conntrack_create_prerouting_hook(skb) do {    \
    int km_ret = 0; \
    if (km_hook_nf_conntrack_create_prerouting_hook){\
        km_ret = km_hook_nf_conntrack_create_prerouting_hook(skb);\
        if (NF_DROP == km_ret) { \
            return ERR_PTR(-EACCES); \
        }\
    }\
 } while (0)

#define km_dev_queue_xmit_hook(skb) do {     \
    int ret = NF_ACCEPT;                     \
    skb_reset_mac_header(skb);               \
    ret = km_dev_xmit_prerouting_hook(skb);  \
    if (NF_DROP == ret) {                    \
        kfree_skb(skb);                      \
        return NET_XMIT_SUCCESS;             \
    } else if (NF_STOLEN == ret) {           \
        return NET_XMIT_SUCCESS;             \
    }                                        \
} while (0)

#define km_mesh_vif_tx_xmit_hook(skb, dev, dmac) do {     \
    int ret = -EPERM;                               \
    if (km_mesh_multifreq_lb_hook) {                \
        ret = km_mesh_multifreq_lb_hook(skb, dev, dmac);  \
        if(NETDEV_TX_OK == ret) {                   \
            rcu_read_unlock();                      \
            return NETDEV_TX_OK;                    \
        } else if (-EPERM != ret) {                 \
             goto __DROP;                           \
        }                                           \
    }                                               \
} while (0)

#define km_mesh_rx_data_xmit_hook(skb, dev, dmac) do {    \
    int ret = -EPERM;                               \
    if (km_mesh_multifreq_lb_hook) {                \
        ret = km_mesh_multifreq_lb_hook(skb, dev, dmac);  \
        if (NETDEV_TX_OK == ret) {                  \
            return;                                 \
        } else if (-EPERM != ret) {                 \
            kfree_skb(skb);                         \
            return;                                 \
        }                                           \
    }                                               \
} while (0)

#define km_ip_rcv_hook(skb) do {    \
    if (likely(km_hook_ip_rcv_hook)) {\
        if (unlikely(NET_RX_DROP == km_hook_ip_rcv_hook(skb))) {    \
             kfree_skb(skb);    \
             return NET_RX_DROP;    \
        }   \
    } \
} while (0)

#define km_netif_receive_skb_hook(skb) do {\
    if (likely(km_hook_netif_receive_skb_hook)) {\
        if (unlikely(NET_RX_DROP == km_hook_netif_receive_skb_hook(skb))) {    \
            kfree_skb(skb);    \
            return NET_RX_DROP;\
        }    \
    }\
} while (0)

#define km___netif_receive_skb_core_hook(skb) do {\
    if (likely(km_hook___netif_receive_skb_core_hook)) {\
        if (NF_ACCEPT == km_hook___netif_receive_skb_core_hook(skb)) { \
            return NET_RX_SUCCESS;\
        } \
    }\
} while (0)

#define km___netif_receive_skb_hook(skb) do {\
    if (likely(km_hook___netif_receive_skb_hook)){\
        if (unlikely(NET_RX_DROP == km_hook___netif_receive_skb_hook(skb))) {    \
            kfree_skb(skb);    \
            return NET_RX_DROP;\
        }    \
     }\
} while (0)

//返回值为NULL表示不创建fdb转发条目
#define km_fdb_create_hook(head, fdb, source, addr) do {   \
    int km_ret = 0; \
    fdb->online_ip_ext = NULL;  \
    if (km_hook_fdb_create_hook) {  \
        km_ret = km_hook_fdb_create_hook(head, fdb, source, addr); \
        if (NF_DROP == km_ret) {\
            kmem_cache_free(br_fdb_cache, fdb);\
            fdb = NULL;\
            return NULL;\
        }\
    }   \
} while (0)

#define km_br_handle_frame_hook(skb) do {   \
    int km_ret = 0; \
    if (likely(km_hook_br_handle_frame_hook)) { \
        km_ret = km_hook_br_handle_frame_hook(skb); \
        if (unlikely(NF_DROP == km_ret)) {\
            kfree_skb(skb); \
            return RX_HANDLER_CONSUMED;\
        }\
    }   \
} while (0)
#define km_n_tty_write_hook(buf, nr) do {  \
    if (km_tty_driver_close) { \
        return nr;\
    }    \
} while (0)

#define km_netif_receive_skb_internal_hook(skb, rflow, cpu) do { \
    int km_ret = 0; \
    if (likely(km_hook_netif_receive_skb_internal_hook)) { \
        km_ret = km_hook_netif_receive_skb_internal_hook(skb, (void **)rflow, cpu); \
        if (NF_ACCEPT == km_ret) { \
            rcu_read_unlock(); \
            return NET_RX_SUCCESS; \
        } \
    } \
} while (0)

#endif
