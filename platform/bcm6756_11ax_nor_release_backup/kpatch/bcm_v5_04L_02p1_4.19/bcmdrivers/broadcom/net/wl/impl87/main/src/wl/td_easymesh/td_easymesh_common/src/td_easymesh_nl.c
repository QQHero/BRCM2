/*****************************************************************************
Copyright (C), 吉祥腾达，保留所有版权
File name ：td_easymesh_nl_sock.c
Description : easymesh
Author ：qinke@tenda.cn
Version ：v1.0
Date ：2020.4.1
*****************************************************************************/

#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include "td_easymesh_dbg.h"
#include "td_easymesh_nl.h"

#define MAX_PAYLOAD 4096
#define TD_EM_NETLINK 48

typedef struct td_em_nl_pidmsg
{
    netlink_user_type_e type;
    unsigned int pid;
    int reqlen;
    const char *req;
    int rsplen;
    const char *rsp;
}td_em_nl_pidmsg_t;

static td_em_nl_pidmsg_t g_td_em_nl_pidmsg [] = {{TD_EM_NLUSER_EASYMESH, 0, 0, "td_set_easymesh_pid", 0, "td set easymesh netlink pid success"},
                                                 {TD_EM_NLUSER_WSERVER, 0, 0, "td_em_set_wserver_pid", 0, "td_em_set_wserver netlink pid success"},
                                                 {TD_EM_NLUSER_XMESH, 0, 0, "td_em_set_xmesh_pid", 0, "td_em_set_xmesh_pid success"} 
                                                };

typedef struct td_em_nl_ctx{
    struct sock *td_easymesh_nl_sock;
    spinlock_t td_em_nl_send_lock;
    spinlock_t td_em_nl_recv_lock;
    int npid_msg;
    td_em_nl_pidmsg_t *pid_msg;
}td_easymesh_netklink_ctx_t;

static td_easymesh_netklink_ctx_t td_easymesh_netklink;

static td_easymesh_netklink_ctx_t *td_em_handle(void)
{
    return (td_easymesh_netklink_ctx_t *)&td_easymesh_netklink;
}

/*****************************************************************************
 函 数 名  : td_em_nl_exit
 功能描述  : td easymesh 模块netlink退出函数
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
void td_em_nl_exit(void)
{
    td_easymesh_netklink_ctx_t *nl = NULL;
    nl = td_em_handle();

    if (nl) {
        if (nl->td_easymesh_nl_sock) {
            netlink_kernel_release(nl->td_easymesh_nl_sock);
        }
        memset(nl, 0, sizeof(td_easymesh_netklink_ctx_t));
    }

    return;
}

/*****************************************************************************
 函 数 名  : td_em_nl_skbput
 功能描述  : 创建skb，并将netlink数据写入skb
 输入参数  : char *data    
             int data_len  
             int pid
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static struct sk_buff * td_em_nl_skbput(char *data, int data_len, int pid)
{
    struct sk_buff * skb = NULL;
    char *fn;
    int err = 0;
    struct nlmsghdr *nlh = NULL;

    if (!data) {
        err = -ENOBUFS;
        fn  = "null ptr";
        goto msg_fail;
    }
    skb = alloc_skb(NLMSG_SPACE(data_len), GFP_ATOMIC);
    if (!skb) {
        err = -ENOBUFS;
        fn  = "alloc_skb";
        goto msg_fail;
    }

    nlh = nlmsg_put(skb, 0, 0, 0, data_len, 0);

    if (!nlh) {
        err = -ENOBUFS;
        fn  = "nlmsg_put";
        goto msg_fail;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
    NETLINK_CB(skb).portid = 0; 
#else
    NETLINK_CB(skb).pid = 0; 
#endif
    NETLINK_CB(skb).dst_group = 0; 

    memcpy(NLMSG_DATA(nlh), data, data_len);
    goto success;

msg_fail:
    TD_EM_DBG_RET_ERR("drop netlink msg: pid=%d msglen=%d %s: err=%d\n", pid, data_len, fn, err);
    if (skb) {
        kfree_skb(skb);
        skb = NULL;
    }

success:
    return skb;
}

/*****************************************************************************
 函 数 名  : td_em_nl_unicast_send
 功能描述  : td easymesh 模块netlink单播发送数据函数
 输入参数  : char *data    
             int data_len  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static void td_em_nl_unicast_send(char *data, int data_len, int pid)
{
    td_easymesh_netklink_ctx_t *nl = td_em_handle();
    struct sk_buff * skb;
    char *fn = NULL;
    int err = 0;

    if (!nl || data_len > MAX_PAYLOAD || !data || !pid) {
        TD_EM_DBG_PARAM_ERR("input param error data len = %d, nl->td_easymesh_user_pid = %d \n", 
        data_len, pid);
        return;
    }

    TD_EM_SPIN_LOCK(&nl->td_em_nl_send_lock);

    skb = td_em_nl_skbput(data, data_len, pid);
    if (!skb) {
        err = -ENOBUFS;
        fn  = "alloc_skb";
        goto msg_fail;
    }

    err = netlink_unicast(nl->td_easymesh_nl_sock, skb, pid, MSG_DONTWAIT);//该函数会对skb进行释放。
    if (err < 0) {
        fn = "nlmsg_unicast";
        goto msg_fail; 
    }
    TD_EM_DBG_MSG("success len = %d, pid = %d\n", data_len, pid);
    TD_EM_SPIN_UNLOCK(&nl->td_em_nl_send_lock);
    return;

msg_fail:
    TD_EM_DBG_RET_ERR("drop netlink msg: pid=%d msglen=%d %s: err=%d\n", pid, data_len, fn, err);
    TD_EM_SPIN_UNLOCK(&nl->td_em_nl_send_lock);
    return;
}

/*****************************************************************************
 函 数 名  : td_em_nl_group_mask
 功能描述  : 组播掩码 0是单播
 输入参数  : int group 
 输出参数  : 无
 返 回 值  : 掩码
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static inline int td_em_nl_group_mask(int group)
{
    return group ? 1 << (group - 1) : 0;
}

/*****************************************************************************
 函 数 名  : td_em_nl_broadcast_send
 功能描述  : td_em_nl_broadcast_send netlink多播发送数据函数
 输入参数  : char *data    
             int data_len  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static void td_em_nl_multicast_send(char *data, int data_len)
{
    td_easymesh_netklink_ctx_t *nl = NULL;
    struct sk_buff * skb;
    char *fn = NULL;
    int err = 0;

    nl = td_em_handle();
    if (!nl || data_len > MAX_PAYLOAD || !data) {
        TD_EM_DBG_PARAM_ERR("input param error:nl = %p, data_len = %d, data = %p\n",nl, data_len ,data);
        return;
    }

    TD_EM_SPIN_LOCK(&nl->td_em_nl_send_lock);

    skb = td_em_nl_skbput(data, data_len, TD_EM_NLUSER_MULTICAST_MESH);
    if (!skb) {
        err = -ENOBUFS;
        fn  = "alloc_skb";
        goto msg_fail;
    }
    
    //该函数会对skb进行释放, easymehs+xmesh多播组编号为1 标准内核接口
    err = nlmsg_multicast(nl->td_easymesh_nl_sock, skb, 0, td_em_nl_group_mask(1), GFP_ATOMIC); //lint !e119

    if (err < 0) {
        fn = "nlmsg_broadcast";
        goto msg_fail; 
    }
    TD_EM_DBG_MSG("success len = %d\n", data_len);
    TD_EM_SPIN_UNLOCK(&nl->td_em_nl_send_lock);//lint !e10 !e24 !e2
    return;//lint !e26

msg_fail:
    TD_EM_DBG_RET_ERR("drop netlink msg: msglen=%d %s: err=%d\n", data_len, fn, err);
    TD_EM_SPIN_UNLOCK(&nl->td_em_nl_send_lock);//lint !e10 !e24 !e10 !e2
    return;//lint !e26
}

/*****************************************************************************
 函 数 名  : td_em_nl_find_typeidx
 功能描述  : 找到type的下标
 输入参数  : void *data    

 输出参数  : 
 返 回 值  : 下标
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_nl_find_typeidx(void *data)
{
    td_easymesh_netklink_ctx_t *nl = NULL;
    netlink_user_type_e type = -1;
    int i = 0;
    nl = td_em_handle();

    if (!nl || !nl->pid_msg || !nl->npid_msg || !data) {
        TD_EM_DBG_PARAM_ERR("null ptr\n");
        return -1;
    }

    for (i = 0; i < nl->npid_msg; i++) {
        if (!memcmp(data, nl->pid_msg[i].req, nl->pid_msg[i].reqlen)) {
            type = i;
            TD_EM_DBG_MSG("pid_msg req=%s, type=%d\n", nl->pid_msg[i].req, type);
            break;
        }
    }
    
    return type;
}

/*****************************************************************************
 函 数 名  : td_em_nl_find_pid
 功能描述  : 找到该进程的pid
 输入参数  : type

 输出参数  : 
 返 回 值  : pid
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_nl_find_pid(netlink_user_type_e type)
{
    unsigned int pid = 0, i;
    td_easymesh_netklink_ctx_t *nl = NULL;
    nl = td_em_handle();
    
    if (!nl || !nl->pid_msg || !nl->npid_msg) {
        TD_EM_DBG_PARAM_ERR("null ptr\n");
        return -1;
    }

    for (i = 0; i < nl->npid_msg; i++) {
        if (nl->pid_msg[i].type == type) {
            pid = nl->pid_msg[i].pid;
            break;
        }
    }

    return pid;
}

/*****************************************************************************
 函 数 名  : td_em_nl_send
 功能描述  : 提供给外部调用的发送函数。外部可以根据type选择是单播还是组播
 输入参数  : type 进程标号，组

 输出参数  : 
 返 回 值  : pid
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
void td_em_nl_send(char *data, int data_len, netlink_user_type_e type)
{
    td_easymesh_netklink_ctx_t *nl = NULL;
    unsigned int pid = 0;

    nl = td_em_handle();
    if (!data || !nl) {
        TD_EM_DBG_PARAM_ERR("null ptr\n");
        return;
    }

    pid = td_em_nl_find_pid(type);

    if (pid > 0) {
        td_em_nl_unicast_send(data, data_len, pid);
    } else if(TD_EM_NLUSER_MULTICAST_MESH == type) {
        td_em_nl_multicast_send(data, data_len);
    } else {
        TD_EM_DBG_PARAM_ERR("nofound pid Maybe this progress is not initialized\n");
    }

    return;
}

/*****************************************************************************
 函 数 名  : td_em_nl_rcv
 功能描述  : td easymesh 模块netlink接收上层数据函数
 输入参数  : struct sk_buff *skb  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static void td_em_nl_rcv(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    td_easymesh_netklink_ctx_t *nl = NULL;
    int type_idx = 0;

    nl = td_em_handle();
    if (!skb || !nl) {
        TD_EM_DBG_PARAM_ERR("skb is NULL\n");
        return;
    }

    nlh = (struct nlmsghdr *)skb->data;

    if (!nlh) {
        TD_EM_DBG_PARAM_ERR("nlh is NULL\n");
        return;
    }

    type_idx = td_em_nl_find_typeidx(NLMSG_DATA(nlh));
    if (type_idx < 0) {
        TD_EM_DBG_PARAM_ERR("no found type\n");
        return;
    }

    TD_EM_SPIN_LOCK(&nl->td_em_nl_recv_lock);

    if (nlh->nlmsg_pid) {
        nl->pid_msg[type_idx].pid = nlh->nlmsg_pid;
        TD_EM_DBG_MSG("%s pid = %d\n", nl->pid_msg[type_idx].req, nl->pid_msg[type_idx].pid);
        td_em_nl_unicast_send((char *)(nl->pid_msg[type_idx].rsp), nl->pid_msg[type_idx].rsplen, nl->pid_msg[type_idx].pid);
    } else {
        TD_EM_DBG_PARAM_ERR("no fount pid\n");
    }

    TD_EM_SPIN_UNLOCK(&nl->td_em_nl_recv_lock);//lint !e24 !e10 !e2

    return;//lint !e26
}

/*****************************************************************************
 函 数 名  : td_em_nl_init_pidmsg
 功能描述  : 初始化pid结构体
 输入参数  : td_easymesh_netklink_ctx_t *nl 
 输出参数  : 
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int td_em_nl_init_pidmsg(td_easymesh_netklink_ctx_t *nl)
{
    int i;

    if (!nl) {
        TD_EM_DBG_PARAM_ERR("skb is NULL\n");
        return -1;
    }

    nl->npid_msg = sizeof(g_td_em_nl_pidmsg)/sizeof(g_td_em_nl_pidmsg[0]);
    nl->pid_msg = (td_em_nl_pidmsg_t *)g_td_em_nl_pidmsg;
    for (i = 0; i < nl->npid_msg; i++) {
        nl->pid_msg[i].reqlen = strlen(nl->pid_msg[i].req) + 1;
        nl->pid_msg[i].rsplen = strlen(nl->pid_msg[i].rsp) + 1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_nl_init
 功能描述  : td easymesh 模块netlink初始化函数
 输入参数  : void  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_nl_init(void)
{
    td_easymesh_netklink_ctx_t *nl = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
    struct netlink_kernel_cfg cfg;
#endif

    nl = td_em_handle();
    if (!nl || nl->td_easymesh_nl_sock) {
        TD_EM_DBG_PARAM_ERR("td_easymesh_nl_sock is set");
        return -1;
    }

    memset(nl, 0, sizeof(td_easymesh_netklink_ctx_t));

    if (td_em_nl_init_pidmsg(nl) < 0) {
        TD_EM_DBG_PARAM_ERR("init pid msg error");
        return -1;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
    memset(&cfg, 0, sizeof(cfg));

    cfg.input = td_em_nl_rcv,

    nl->td_easymesh_nl_sock = netlink_kernel_create(&init_net, TD_EM_NETLINK, &cfg);
#else
    nl->td_easymesh_nl_sock = netlink_kernel_create(&init_net, TD_EM_NETLINK, 0, td_em_nl_rcv, NULL, THIS_MODULE);
#endif

    if (!nl->td_easymesh_nl_sock) {
        TD_EM_DBG_PARAM_ERR("Cannot create netlink socket");
        return -ENOMEM;
    }

    spin_lock_init(&nl->td_em_nl_send_lock);
    spin_lock_init(&nl->td_em_nl_recv_lock);
    return 0;
}

