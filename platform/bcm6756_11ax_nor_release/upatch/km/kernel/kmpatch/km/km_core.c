/**
*date:2017-1-12
*desc:
    1、该文件主要是用来定义全局变量、指针函数；
    2、该文件不允许有任何功能实现的代码添加进来，如果需要实现具体的功能
        需要添加对应的模块，任何将对应指针函数在这里定义；
author: kuangdaozhen
**/
#include <linux/cpu.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/hash.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/notifier.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <linux/rtnetlink.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <net/dst.h>
#include <net/pkt_sched.h>
#include <net/checksum.h>
#include <net/xfrm.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/netpoll.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <net/wext.h>
#include <net/iw_handler.h>
#include <asm/current.h>
#include <linux/audit.h>
#include <linux/dmaengine.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/if_arp.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/in.h>
#include <linux/jhash.h>
#include <linux/random.h>
#include <net/km_common.h>

#ifdef CONFIG_SA_FASTPATH
ctf_t *kcih = NULL;
EXPORT_SYMBOL(kcih);
#endif


#ifdef CONFIG_TENDA_PRIVATE_KM

#ifdef CONFIG_BEHAVIOR_MANAGER
/* 行为管理总开关,为适配FC移至km_core.c */
int g_bm_enable = 0;
EXPORT_SYMBOL_GPL(g_bm_enable);
#endif

/* 是否需要fastpath && 是否需要数据包走协议栈fastpath才能学习到完整转发信息 */
int (*km_fastpath_need_slow_learning)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_fastpath_need_slow_learning);

void (*km_dhcp_options_add_wireless_client)(const unsigned char *mac, const char *ssid) = NULL;
EXPORT_SYMBOL(km_dhcp_options_add_wireless_client);
int (*km_dhcp_options_get_client_accesspoint)(const unsigned char *mac, void *client_info) = NULL;
EXPORT_SYMBOL(km_dhcp_options_get_client_accesspoint);

#ifdef CONFIG_KM_NF_CONNTRACK
int (*km_bm_l2_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_bm_l2_hook);
int ( *km_l2_nf_conntrack_driver_rx_hook)(struct sk_buff *skb, struct net_device *dev) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_l2_nf_conntrack_driver_rx_hook);
void (*km_l2_nf_conntrack_driver_tx_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_l2_nf_conntrack_driver_tx_hook);
void (*km_l2_nf_conntrack_init_hook)(struct nf_conn *ct) = NULL;
EXPORT_SYMBOL(km_l2_nf_conntrack_init_hook);
#ifdef CONFIG_KM_NF_CONNTRACK_WIRELESS_MESH_INFO
int (*km_nf_conntrack_get_mesh_info)(struct sk_buff *skb, unsigned int *id,
                                                int *previous_ifindex, int *current_ifindex,
                                                unsigned int *current_speed, unsigned char mesh_flag) = NULL;
EXPORT_SYMBOL(km_nf_conntrack_get_mesh_info);
void (*km_nf_conntrack_set_mesh_info)(struct sk_buff *skb, const int previous_ifindex,
                                                const int current_ifindex, unsigned char mesh_flag) = NULL;
EXPORT_SYMBOL(km_nf_conntrack_set_mesh_info);
#endif

#endif
#ifdef CONFIG_MESH_MULTIFREQ_LOADBALANCE
#ifdef CONFIG_TD_MESH_V3
int (*km_meshv3_multifreq_lb_hook)(struct sk_buff *skb, struct net_device *dev, u8 *dmac, int fc_open) = NULL;
EXPORT_SYMBOL(km_meshv3_multifreq_lb_hook);
int (*km_meshv3_multifreq_lb_check_is_5g_and_wire_interface)(struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_meshv3_multifreq_lb_check_is_5g_and_wire_interface);
#else
int (*km_mesh_multifreq_lb_hook)(struct sk_buff *skb, struct net_device *dev, u8 *dmac) = NULL;
EXPORT_SYMBOL(km_mesh_multifreq_lb_hook);
#endif
#ifdef CONFIG_TD_MESH_V3
int (*km_mesh_multifreq_lb_rx_detect_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_mesh_multifreq_lb_rx_detect_hook);
#endif
void (*km_multifreq_lb_cache_free)(void *multifreq_lb_ext) = NULL;
EXPORT_SYMBOL(km_multifreq_lb_cache_free);
#endif

void(*km_wireless_client_online)(const unsigned char *mac, int is_5g,
                                struct net_device *dev, const char *ssid, const int ssid_len) = NULL;
EXPORT_SYMBOL(km_wireless_client_online);
void(*km_wireless_client_offline)(const unsigned char *mac, struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_wireless_client_offline);

void (*km_wan_traffic_statistic)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_wan_traffic_statistic);
void (*km_wan_set_wanid)(struct sk_buff *skb, struct net_device *dev) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_wan_set_wanid);
void (*km_nf_conntrack_init)(struct nf_conn *ct) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_nf_conntrack_init);

void (*km_nf_conntrack_destroy)(struct nf_conn *ct) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_nf_conntrack_destroy);

//兼容以前微企规定vlan号大于等于2，小于等于10为wan口的规则
int km_wan_vlanid_start = 2;
EXPORT_SYMBOL(km_wan_vlanid_start);
int km_wan_vlanid_end = 10;
EXPORT_SYMBOL(km_wan_vlanid_end);

//限制二层fdb转发表的数量，如果不限制模拟发送大量合法arp请求时会导致内存耗尽
int km_public_bridge_fdb_max_entry_limit = 1000;
int km_public_bridge_fdb_entry_statistic = 0;
EXPORT_SYMBOL(km_public_bridge_fdb_max_entry_limit);
EXPORT_SYMBOL(km_public_bridge_fdb_entry_statistic);
#ifdef CONFIG_BRIDGE_MAC_FILTER
int (*km_l2_bm_mac_filter_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_l2_bm_mac_filter_hook);
#endif
#ifdef CONFIG_BRIDGE_INHIBIT
int (*km_bridge_inhibit_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_bridge_inhibit_hook);
#endif
#endif

#ifdef CONFIG_SA_FASTPATH_L2
void (*km_fast_l2_nf_conntrack_ext_alloc)(struct nf_conn *ct) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_fast_l2_nf_conntrack_ext_alloc);

void (*km_fast_l2_nf_conntrack_ext_free)(struct nf_conn *ct) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_fast_l2_nf_conntrack_ext_free);

int (*km_fast_l2_dev_hard_start_xmit)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_fast_l2_dev_hard_start_xmit);

int (*km_fast_l2_cache_vlan_ethhdr)(struct neighbour *neigh, struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_fast_l2_cache_vlan_ethhdr);

int (*km_fast_l2_br_handle_frame)(struct sk_buff *skb, struct net_bridge_port *p) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_fast_l2_br_handle_frame);

void (*km_fast_l2_ext_delete_by_dev)(struct net_device *dev) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_fast_l2_ext_delete_by_dev);
#endif


#ifdef CONFIG_KM_SDW_FAST
int (*km_sdw_fast_set_flow_cpu)(struct sk_buff *skb, int *cpu) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_sdw_fast_set_flow_cpu);

void (*km_sdw_fast_create_tunc_hook)(char *mac, uint32_t tunl_ip,
        uint16_t tunl_port, int enable, char *alg, char *key) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_sdw_fast_create_tunc_hook);

void (*km_sdw_fast_destroy_tunc_hook)(void) = NULL;
EXPORT_SYMBOL(km_sdw_fast_destroy_tunc_hook);

void (*km_sdw_fast_update_tunc_hook)(struct sk_buff *skb, char *mac, char *localmac, u_int32_t local_ip,
                                    u_int16_t local_port, struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_sdw_fast_update_tunc_hook);

void (*km_sdw_fast_update_wanipc_hook)(u_int32_t local_ip, struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_sdw_fast_update_wanipc_hook);

int (*km_sdw_fast_forward)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_sdw_fast_forward);

int (*km_sdw_fast_create_client_conn_hook)(struct sk_buff *skb, char *mac,
                                uint32_t local_ip) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_sdw_fast_create_client_conn_hook);

void (*km_sdw_fast_free_ext_hook)(struct km_ct_ext *ct_ext, struct nf_conn *ct) = NULL;
EXPORT_SYMBOL(km_sdw_fast_free_ext_hook);

#endif


#ifdef CONFIG_INTERFACE_ISOLATE
int (*km_interface_isolate)(struct sk_buff *skb,struct net_device*dev) = NULL;
EXPORT_SYMBOL(km_interface_isolate);
int (*km_ssid_isolate)(struct sk_buff *skb, struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_ssid_isolate);
#endif
#ifdef CONFIG_NETWORK_ISOLATE
int (*km_network_isolate)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_network_isolate);
#endif

#ifdef CONFIG_LOAD_BALANCE
void (*km_load_balance_ct_detach)(struct nf_conn *ct) __rcu __read_mostly = NULL;
EXPORT_SYMBOL_GPL(km_load_balance_ct_detach);
#endif

#ifdef CONFIG_FLOW_IDENTIFY
void (*km_flow_identify)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_flow_identify);

void* (*km_flow_identify_alloc_user_flow_stat_hook)(void **user_flow_stat) = NULL;
EXPORT_SYMBOL(km_flow_identify_alloc_user_flow_stat_hook);
void (*km_flow_identify_free_user_flow_stat_hook)(void *user_flow_stat) = NULL;
EXPORT_SYMBOL(km_flow_identify_free_user_flow_stat_hook);

u_int32_t (*km_flow_identify_get_flow_prio_hook)(void *flow_ide_ext) = NULL;
EXPORT_SYMBOL(km_flow_identify_get_flow_prio_hook);

void (*km_flow_identify_alloc_flow_ide_ext_hook)(void *ct_ext) = NULL;
EXPORT_SYMBOL(km_flow_identify_alloc_flow_ide_ext_hook);
void (*km_flow_identify_free_flow_ide_ext_hook)(void *ct_ext) = NULL;
EXPORT_SYMBOL(km_flow_identify_free_flow_ide_ext_hook);

int32_t (*km_flow_identify_show_flow_rate_hook)(struct seq_file *s, void *v) = NULL;
EXPORT_SYMBOL(km_flow_identify_show_flow_rate_hook);
int32_t (*km_flow_identify_show_flow_bytes_hook)(struct seq_file *s, void *v) = NULL;
EXPORT_SYMBOL(km_flow_identify_show_flow_bytes_hook);

void (*km_flow_identify_clear_user_flow_stat_hook)(void *user_flow_stat) = NULL;
EXPORT_SYMBOL(km_flow_identify_clear_user_flow_stat_hook);
#endif

#ifdef CONFIG_KM_AUDIT
/* 上网审计模块需要的函数指针，将上网审计模块的proc注册到online_ip中，通过online_ip来进行输出 */
int32_t (*km_audit_show_user_info_hook)(struct seq_file *s, void *v) = NULL;
EXPORT_SYMBOL(km_audit_show_user_info_hook);
int32_t (*km_audit_clear_audit_info)(void *online_ip) = NULL;
EXPORT_SYMBOL(km_audit_clear_audit_info);
#endif

#ifdef CONFIG_NOS_CONTROL_V2
int (*km_nos_enqueue_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_nos_enqueue_hook);
void (*km_nos_track_user_free)(void *user) = NULL;
EXPORT_SYMBOL(km_nos_track_user_free);

void (*km_nos_track_flow_free)(void *track) = NULL;
EXPORT_SYMBOL(km_nos_track_flow_free);

int (*km_nos_skb_is_limited)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_nos_skb_is_limited);
#endif /* CONFIG_NOS_CONTROL_V2 */

#if defined(CONFIG_NOS_CONTROL_V2)
#ifdef CONFIG_KM_NF_CONNTRACK
void (*km_l2_nf_conntrack_init_nos_hook)(void *l2_nfct, void *ct_ext) = NULL;
EXPORT_SYMBOL(km_l2_nf_conntrack_init_nos_hook);
void (*km_l2_nf_conntrack_destroy_nos_hook)(void *l2_nfct, void *ct_ext) = NULL;
EXPORT_SYMBOL(km_l2_nf_conntrack_destroy_nos_hook);
#endif
#endif /* defined(CONFIG_NOS_CONTROL_V2) */

#ifdef CONFIG_KM_NF_CONNTRACK
void (*km_l2_nf_conntrack_init_ct_ext_hook)(struct sk_buff *skb, void *l2_nfct) = NULL;
EXPORT_SYMBOL(km_l2_nf_conntrack_init_ct_ext_hook);
void (*km_l2_nf_conntrack_destroy_ct_ext_hook)(void *l2_nfct) = NULL;
EXPORT_SYMBOL(km_l2_nf_conntrack_destroy_ct_ext_hook);

void (*km_online_ip_create_by_l2_nf_conn_hook)(struct sk_buff* skb, void *nfct) = NULL;
EXPORT_SYMBOL(km_online_ip_create_by_l2_nf_conn_hook);
void (*km_online_ip_delete_by_l2_nf_conn_hook)(void *nfct) = NULL;
EXPORT_SYMBOL(km_online_ip_delete_by_l2_nf_conn_hook);

void (*km_l2_nf_conntrack_put)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_l2_nf_conntrack_put);
#endif

#ifdef CONFIG_DDOS_CONTROL

u8 (*km_ddos_process_attack_fence)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_ddos_process_attack_fence);

u8 (*km_ddos_process_attack_fence_conntrack)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_ddos_process_attack_fence_conntrack);

#endif

#ifdef CONFIG_ARP_FENCE_CONTROL
u8 (*km_arp_process_broadcast_packets)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_arp_process_broadcast_packets);
u8 (*km_arp_fence_broadcast_packets)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_arp_fence_broadcast_packets);
#endif
#ifdef CONFIG_BEHAVIOR_MANAGER
int (*km_bm_connect_limit_action)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_bm_connect_limit_action);

void (*km_bm_get_tcp_mss)(struct sk_buff *skb, enum tcp_conntrack state) = NULL;
EXPORT_SYMBOL(km_bm_get_tcp_mss);
#endif

#ifdef CONFIG_DNS_REDIRECT_CONTROL
int (*km_dns_redirect_action)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_dns_redirect_action);
#endif

#ifdef CONFIG_MULTI_BROAD_FILTER_CONTROL
int (*km_multi_broad_filter_process_packets)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_multi_broad_filter_process_packets);
#endif

#ifdef CONFIG_OS_IDENTIFY
void (*km_os_identify)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_os_identify);
#endif

#ifdef CONFIG_OS_IDENTIFY_GET_SYS_TYPE
int (*km_os_identify_get_system_type)(u8 *ethaddr) = NULL;
EXPORT_SYMBOL(km_os_identify_get_system_type);
int (*km_os_identify_get_system_type_from_node)(u8 *ethaddr) = NULL;
EXPORT_SYMBOL(km_os_identify_get_system_type_from_node);
#endif

#ifdef CONFIG_AP_DATA_TUNNEL
//参数is_wireless 1表示是无线，0表示有线
int (*km_ap_data_tunnel)(struct sk_buff *skb, struct net_device *dev, int is_wireless) = NULL;
EXPORT_SYMBOL(km_ap_data_tunnel);
#endif

#ifdef CONFIG_TUNNEL_FORWARD
//参数is_wireless 1表示是无线，0表示有线
int (*km_tf_do_fastpath)(struct sk_buff *skb, struct net_device *from_dev, int is_wireless) = NULL;
EXPORT_SYMBOL(km_tf_do_fastpath);
#endif

#ifdef CONFIG_AUTO_DISCOVER
void (*km_autodiscover_upload_online)(struct hlist_head *head,
    struct net_bridge_fdb_entry *fdb, struct net_bridge_port *source,
    const unsigned char *addr) = NULL;
EXPORT_SYMBOL(km_autodiscover_upload_online);

void (*km_autodiscover_upload_offline)(struct net_bridge_fdb_entry *f) = NULL;
EXPORT_SYMBOL(km_autodiscover_upload_offline);

int (*km_autodiscover_netlink_send_msg)(void *message, int msize) = NULL;
EXPORT_SYMBOL(km_autodiscover_netlink_send_msg);

void (*km_autodiscover_online_ip_notify)(u_int8_t *macaddr, u_int32_t ipaddr, u_int8_t *dev_name) = NULL;
EXPORT_SYMBOL(km_autodiscover_online_ip_notify);
#endif

#ifdef CONFIG_DHCP_OPTIONS
int (*km_dhcp_options_handle)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_dhcp_options_handle);
#endif

#ifdef CONFIG_TENDA_PRIVATE_KM

int (*km_hook_br_handle_frame_finish_hook)(struct sk_buff *skb, struct net_bridge_fdb_entry *dst) = NULL;
EXPORT_SYMBOL(km_hook_br_handle_frame_finish_hook);

int (*km_hook_br_handle_frame_finish_2_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_hook_br_handle_frame_finish_2_hook);

int (*km_hook_br_fdb_update_online_ip_hook)(struct sk_buff *skb, struct net_bridge_fdb_entry *dst) = NULL;
EXPORT_SYMBOL(km_hook_br_fdb_update_online_ip_hook);

void (*km_hook_fdb_delete_hook)(struct net_bridge *br, struct net_bridge_fdb_entry *f) = NULL;
EXPORT_SYMBOL(km_hook_fdb_delete_hook);

int (*km_hook_fdb_create_hook)(struct hlist_head *head,
    struct net_bridge_fdb_entry *fdb, struct net_bridge_port *source,
    const unsigned char *addr) = NULL;
EXPORT_SYMBOL(km_hook_fdb_create_hook);

int (*km_hook_br_fdb_update_hook)(struct hlist_head *head,
    struct net_bridge_fdb_entry *fdb, struct net_bridge_port *source,
    const unsigned char *addr) = NULL;
EXPORT_SYMBOL(km_hook_br_fdb_update_hook);

void (*km_hook___copy_skb_header_hook)(struct sk_buff *new, const struct sk_buff *old) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook___copy_skb_header_hook);

void (*km_hook_vlan_do_receive_hook)(struct sk_buff *skb,  struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_hook_vlan_do_receive_hook);

int (*km_hook_netif_receive_skb_hook)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_netif_receive_skb_hook);
int (*km_hook___netif_receive_skb_hook)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook___netif_receive_skb_hook);


int (*km_hook___netif_receive_skb_core_hook)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook___netif_receive_skb_core_hook);

int (*km_hook_ip_rcv_hook)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_ip_rcv_hook);

int (*km_hook_dev_xmit_prerouting_hook)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_dev_xmit_prerouting_hook);

int (*km_hook_nf_conntrack_create_prerouting_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_hook_nf_conntrack_create_prerouting_hook);

int (*km_hook_nf_conntrack_create_postrouting_hook)(struct sk_buff *skb, struct nf_conn *ct) = NULL;
EXPORT_SYMBOL(km_hook_nf_conntrack_create_postrouting_hook);

int (*km_hook_driver_wireless_tx_packet_hook)(struct sk_buff *skb, struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_hook_driver_wireless_tx_packet_hook);

int (*km_hook_driver_wireless_mesh_tx_hook)(struct sk_buff *skb, struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_hook_driver_wireless_mesh_tx_hook);

int (*km_hook_driver_wireless_rx_packet_hook)(struct sk_buff *skb, struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_hook_driver_wireless_rx_packet_hook);


void (*km_hook_ip_rcv_finish_hook)(struct sk_buff *skb, struct net_device *dev) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_ip_rcv_finish_hook);

void (*km_hook_ip6_rcv_finish_hook)(struct sk_buff *skb, struct net_device *dev) = NULL;
EXPORT_SYMBOL(km_hook_ip6_rcv_finish_hook);

void (*km_hook_set_ct_hook)(struct sk_buff* skb, struct nf_conn* ct) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_set_ct_hook);
void* (*km_hook_get_ct_hook)(struct sk_buff* skb) = NULL;
EXPORT_SYMBOL(km_hook_get_ct_hook);
void (*km_hook_put_ct_hook)(struct sk_buff* skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_put_ct_hook);

int (*km_hook_driver_tx_packet_hook)(struct sk_buff *skb, struct net_device *dev) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_driver_tx_packet_hook);


int (*km_hook_driver_rx_packet_hook)(struct sk_buff *skb, struct net_device *dev) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_driver_rx_packet_hook);

int (*km_hook_br_handle_frame_hook)(struct sk_buff *skb) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_br_handle_frame_hook);

int (*km_hook_netif_receive_skb_internal_hook)(struct sk_buff *skb, void **rflowp, int *cpu) __rcu __read_mostly = NULL;
EXPORT_SYMBOL(km_hook_netif_receive_skb_internal_hook);

u32 km_tty_driver_close = 0;
EXPORT_SYMBOL(km_tty_driver_close);

u32 km_upgrade_ignore_sig = 0;
EXPORT_SYMBOL(km_upgrade_ignore_sig);

#ifdef CONFIG_KM_KWDOG
int (*km_kwdog_feed_dog)(char *buffer) = NULL;
EXPORT_SYMBOL(km_kwdog_feed_dog);
#endif

#ifdef CONFIG_NETWORK_TOPOLOGY
void (*km_net_topo_hook)(struct sk_buff *skb);
EXPORT_SYMBOL(km_net_topo_hook);

void (*km_net_topo_vlan_hook)(struct sk_buff *skb);
EXPORT_SYMBOL(km_net_topo_vlan_hook);
#endif

#ifdef CONFIG_L2_WIRELESS_REDIRECT_AUTH
int (*km_l2_auth_wireless_tx_packet_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_l2_auth_wireless_tx_packet_hook);
int (*km_l2_auth_wireless_rx_packet_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_l2_auth_wireless_rx_packet_hook);
#endif
#ifdef CONFIG_L2_WIRE_REDIRECT_AUTH
int (*km_l2_auth_tx_packet_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_l2_auth_tx_packet_hook);
int (*km_l2_auth_rx_packet_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_l2_auth_rx_packet_hook);
#endif


#ifdef CONFIG_TRACE_SKB_RUNNING_TIME
struct km_tracepoint g_km_tracepoint_array[KM_TRACEPOINT_MAX_NUM] = {0};     //TracePoint的数据结构
u32 g_km_trace_average_num = 10000;                 // 目标区间累加多少个数据包求平均,默认10000
u32 g_km_trace_count = 0;                           // 统计两点之间数据包的累加次数
u64 g_km_trace_result = 0;                          // 保存count次累加后，数据包经过目标区间的平均时间
u64 g_km_trace_sum = 0;                             // 保存目标区间累加数据包的累加时间值
u32 g_km_trace_start = 0;                           // 目标区间开始点
u32 g_km_trace_end = 0;                             // 目标区间结束点
u32 g_km_tracepoint_num = 0;                        //统计当前TracePoint数目的全局变量
u32 g_km_trace_enable = 0;                          //km_trace模块总开关，默认关闭
EXPORT_SYMBOL(g_km_trace_average_num);
EXPORT_SYMBOL(g_km_trace_count);
EXPORT_SYMBOL(g_km_trace_result);
EXPORT_SYMBOL(g_km_trace_sum);
EXPORT_SYMBOL(g_km_trace_start);
EXPORT_SYMBOL(g_km_trace_end);
EXPORT_SYMBOL(g_km_tracepoint_num);
EXPORT_SYMBOL(g_km_trace_enable);
EXPORT_SYMBOL(g_km_tracepoint_array);
#endif

#ifdef CONFIG_EVENTS_CENTER
/* 客户端更新rssi事件 */
void (*km_eventscenter_wifiev_sta_update_rssi_handler)(u_int8_t *ether_addr, int rssi) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_sta_update_rssi_handler);

/* Bss接收到对应客户端的BTM迁移回应状态事件 */
void (*km_eventscenter_wifiev_bss_trans_res_handler)(u_int8_t *ether_addr, u_int8_t status) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_bss_trans_res_handler);

/* 客户端beacon包回应事件 */
void (*km_eventscenter_wifiev_beacon_rep_handler)(u_int8_t *ether_addr, void *data) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_beacon_rep_handler);

/* band设置信道事件 */
void (*km_eventscenter_wifiev_set_channel_handler)(int band, unsigned int channel) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_set_channel_handler);

/* 设置band的rssi事件 */
void (*km_eventscenter_wifiev_set_ssid_handler)(int band, char *ifname, unsigned char *ssid,
    unsigned int ssid_len) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_set_ssid_handler);

/* 客户端状态事件 */
void (*km_eventscenter_wifiev_sta_status_handler)(u_int8_t *ether_addr, char *dev_name, int message) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_sta_status_handler);

/* 客户端上线事件 */
void (*km_eventscenter_wifiev_sta_join_handler)(u_int8_t *ether_addr, char *dev_name, int rssi,
    unsigned band, unsigned sup11k, unsigned sup11v) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_sta_join_handler);

/* 客户端下线事件 */
void (*km_eventscenter_wifiev_sta_leave_handler)(u_int8_t *ether_addr) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_sta_leave_handler);

/* 客户端密码错误事件 */
void (*km_eventscenter_wifiev_sta_passerr_handler)(u_int8_t *ether_addr) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_sta_passerr_handler);

/* 黑名单拒绝连接事件 */
void (*km_eventscenter_wifiev_sta_blackrej_handler)(u_int8_t *ether_addr) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_sta_blackrej_handler);

/* 因客户端的数量超过了当前ap设定支持的最大客户端接入数量而接入失败时的通知事件 */
void (*km_eventscenter_wifiev_sta_maxcntrej_handler)(u_int8_t *ether_addrt) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_sta_maxcntrej_handler);

/* 检测dfs信道是否有雷达信号 */
void (*km_eventscenter_wifiev_dfs_chanspes_detected_handler)(u_int16_t cur_chan,
    u_int16_t target_chan, u_int8_t radar_type) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_dfs_chanspes_detected_handler);

/* 检测无线接口状态是否有变化(down/up) */
void (*km_eventscenter_wifiev_intf_status_changed_handler)(int band, bool state, u_int8_t *ether_addr) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_intf_status_changed_handler);

/* 游戏加速流量控制等级上报 */
void (*km_eventscenter_wifiev_game_speed_flowcontrol_handler)(char* ifname, int status, void *data, u_int8_t data_len) = NULL;
EXPORT_SYMBOL(km_eventscenter_wifiev_game_speed_flowcontrol_handler);


/* 客户端通过fdb上线的事件 */
void (*km_eventscenter_kmev_upload_online_handler)(struct hlist_head *head,
    struct net_bridge_fdb_entry *fdb, struct net_bridge_port *source, const unsigned char *addr) = NULL;
EXPORT_SYMBOL(km_eventscenter_kmev_upload_online_handler);

/* 客户端通过fdb下线的事件 */
void (*km_eventscenter_kmev_upload_offline_handler)(struct net_bridge_fdb_entry *f) = NULL;
EXPORT_SYMBOL(km_eventscenter_kmev_upload_offline_handler);

/* 客户端更新ip地址的事件 */
void (*km_eventscenter_kmev_online_ip_info_handler)(u_int8_t *macaddr, u_int32_t ipaddr, u_int8_t *dev_name) = NULL;
EXPORT_SYMBOL(km_eventscenter_kmev_online_ip_info_handler);

/* 收到igmp报文上报源ip的事件 */
void (*km_eventscenter_kmev_igmp_ip_handler)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(km_eventscenter_kmev_igmp_ip_handler);

#endif

#ifdef CONFIG_KM_AUDIT
/* 上网审计eventscenter上报接口，用于给用户层上报上网审计信息 */
void (*km_eventscenter_kmev_audit_handler)(void *obj) = NULL;
EXPORT_SYMBOL(km_eventscenter_kmev_audit_handler);
#endif

#ifdef CONFIG_SKIP_FASTPATH
int (*km_skip_fastpath_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL_GPL(km_skip_fastpath_hook);
#endif

/* 1、将自添加的skb_buff成员变量放在此处init，而不放在内核模块如kmbase初始
   化，主要是当模块还没能加载时，数据包已经通过此处并上协议栈，当相关模
   块加载完成后，由于未初始化的成员变量初始值为随机，会造成相关模块如l2-
   _nfconntrack对变量的误判而引起一系列死机，该情况特别在多核并且系统启
   动时收到分片数据报文时容易出现，无线驱动收包hook也应作相同处理
   2、将km_cb等放在此处init，并且不单独设置成函数，主要是因为：在smartbit
   或ixia打流测极限性能时，数据包alloc需要及时，多一个函数出入栈导致少
   转发接近1-2w个包(64Bit) */
void __always_inline km_skb_member_variables_init(struct sk_buff *skb, uint8_t port)
{
    struct km_public_skb_km_cb *km_cb = KM_PUBLIC_GET_SKB_KM_CB(skb);
    km_cb->ether_src_port = port;
    km_cb->original_dev = NULL;
    km_cb->online_ip = NULL;
    km_cb->ct = NULL;
#ifdef CONFIG_KM_NF_CONNTRACK
    km_cb->l2_nfct = NULL;
    km_cb->l2_nfct_dir = ~0;
#endif
#ifdef CONFIG_KM_SDW_FAST
    km_cb->sdw_flag = 0;
#endif
#if defined(CONFIG_MESH_MULTIFREQ_LOADBALANCE) && defined(CONFIG_TD_MESH_V3)
    km_cb->multifreq_lb_flag = 0;
#endif
    skb->dir_ctf = CTF_NONE;
    skb->skb_tx_iif = 0;
#ifdef CONFIG_SA_FASTPATH
    skb->ctf_source_dev = NULL;
#endif
#ifdef CONFIG_TRACE_SKB_RUNNING_TIME
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
    skb->tracepoint_time_init.tv64 = 0;
#else
    skb->tracepoint_time_init = 0;
#endif
#endif
}

int inline km_driver_rx_packet_hook(struct sk_buff *skb, struct net_device *dev, uint8_t port)
{
    km_skb_member_variables_init(skb, port);
    if (likely(km_hook_driver_rx_packet_hook)) {
        if (NF_DROP == km_hook_driver_rx_packet_hook(skb, dev)) {
            return NF_DROP;
        }
    }

    return NF_ACCEPT;
}
EXPORT_SYMBOL(km_driver_rx_packet_hook);

//返回值为1就DROP该数据包，0表示就ACCEPT该数据包
int inline km_driver_tx_packet_hook(struct sk_buff *skb, struct net_device *dev)
{
    if (likely(km_hook_driver_tx_packet_hook)) {
        return km_hook_driver_tx_packet_hook(skb, dev);
    }
    return 0;
}
EXPORT_SYMBOL(km_driver_tx_packet_hook);

void inline km_ip_rcv_finish_hook(struct sk_buff *skb, struct net_device *dev)
{
    if (likely(km_hook_ip_rcv_finish_hook)) {
        km_hook_ip_rcv_finish_hook(skb, dev);
    }
    return ;
}
EXPORT_SYMBOL(km_ip_rcv_finish_hook);

void km_ip6_rcv_finish_hook(struct sk_buff *skb, struct net_device *dev)
{
    if (km_hook_ip6_rcv_finish_hook) {
        km_hook_ip6_rcv_finish_hook(skb, dev);
    }
#ifdef CONFIG_SA_FASTPATH_IPV6
#ifdef CONFIG_TD_IPV6_V2
    ctf_ipc_v6_add(skb, NF_INET_PRE_ROUTING, NULL);
#else
    ctf_ipc_v6_add(skb);
#endif
#endif
    return ;
}
EXPORT_SYMBOL(km_ip6_rcv_finish_hook);

int inline km_driver_wireless_rx_packet_hook(struct sk_buff *skb, struct net_device *dev)
{
    km_skb_member_variables_init(skb, KM_WRIELESS_PORT);
    if (km_hook_driver_wireless_rx_packet_hook) {
         if (km_hook_driver_wireless_rx_packet_hook(skb, dev)) {
            kfree_skb(skb);
            return 1;
         }
    }

    return 0;
}
EXPORT_SYMBOL(km_driver_wireless_rx_packet_hook);

int inline km_driver_wireless_tx_packet_hook(struct sk_buff *skb, struct net_device *dev)
{
    if (km_hook_driver_wireless_tx_packet_hook) {
        if (km_hook_driver_wireless_tx_packet_hook(skb, dev)) {
            kfree_skb(skb);
            return 1;
        }
    }
    return 0;
}
EXPORT_SYMBOL(km_driver_wireless_tx_packet_hook);

int km_driver_wireless_mesh_tx_hook(struct sk_buff *skb, struct net_device *dev)
{

    if (km_hook_driver_wireless_mesh_tx_hook) {
        km_hook_driver_wireless_mesh_tx_hook(skb, dev);
    }

    return 0;
}
EXPORT_SYMBOL(km_driver_wireless_mesh_tx_hook);


void km_nf_conntrack_init_hook(struct nf_conn *ct)
{
#ifdef CONFIG_SA_FASTPATH
    ct->ctf_ext = NULL;
    ctf_nf_conntrack_ext_alloc(ct);
#endif

#ifdef CONFIG_SA_FASTPATH_L2
    ct->fast_l2_ext = NULL;
    if (km_fast_l2_nf_conntrack_ext_alloc) {
        km_fast_l2_nf_conntrack_ext_alloc(ct);
    }
#endif

    //km_ct_ext必须初始化为NULL,否则等系统启动之后在加载kmbase.ko模块会出现死机
#ifdef CONFIG_TENDA_PRIVATE_KM
    ct->km_ct_ext = NULL;
#endif

    if (km_nf_conntrack_init) {
        km_nf_conntrack_init(ct);
    }
#ifdef CONFIG_KM_NF_CONNTRACK
    /*必须放在km_nf_conntrack_init后面*/
    if (km_l2_nf_conntrack_init_hook) {
        km_l2_nf_conntrack_init_hook(ct);
    }
#endif
    return ;
}
EXPORT_SYMBOL(km_nf_conntrack_init_hook);

void km_nf_conntrack_destroy_hook(struct nf_conn *ct)
{
#ifdef CONFIG_SA_FASTPATH_NAT
    ctf_ipc_delete(ct);
    ctf_nf_conntrack_ext_free(ct);
#endif

#ifdef CONFIG_SA_FASTPATH_L2
    if (km_fast_l2_nf_conntrack_ext_free) {
        km_fast_l2_nf_conntrack_ext_free(ct);
    }
#endif

    if (km_nf_conntrack_destroy) {
        km_nf_conntrack_destroy(ct);
    }

}
EXPORT_SYMBOL(km_nf_conntrack_destroy_hook);

int km_dev_xmit_prerouting_hook(struct sk_buff *skb)
{
    if (likely(km_hook_dev_xmit_prerouting_hook)) {
        return km_hook_dev_xmit_prerouting_hook(skb);
    }
    return NF_ACCEPT;
}
EXPORT_SYMBOL(km_dev_xmit_prerouting_hook);

void  km_vlan_do_receive_hook(struct sk_buff *skb,  struct net_device *dev)
{
    if (km_hook_vlan_do_receive_hook){
        km_hook_vlan_do_receive_hook(skb, dev);
    }
}
EXPORT_SYMBOL(km_vlan_do_receive_hook);

void __always_inline km___copy_skb_header_hook(struct sk_buff *new, const struct sk_buff *old)
{
    if (likely(km_hook___copy_skb_header_hook)) {
        km_hook___copy_skb_header_hook(new, old);
    } else {
        /*处理kmbase模块还未加载的情况*/
        km__alloc_skb_hook(new);
    }
}
EXPORT_SYMBOL(km___copy_skb_header_hook);

void km_br_fdb_update_hook(struct hlist_head *head,
    struct net_bridge_fdb_entry *fdb, struct net_bridge_port *source,
    const unsigned char *addr)
{
    if (km_hook_br_fdb_update_hook) {
        km_hook_br_fdb_update_hook(head, fdb, source, addr);
    }
}
EXPORT_SYMBOL(km_br_fdb_update_hook);

void km_fdb_delete_hook(struct net_bridge *br, struct net_bridge_fdb_entry *f)
{
    if (km_hook_fdb_delete_hook){
        km_hook_fdb_delete_hook(br, f);
    }
}
EXPORT_SYMBOL(km_fdb_delete_hook);

void __always_inline km__alloc_skb_hook(struct sk_buff *skb)
{
    km_skb_member_variables_init(skb, KM_INVALID_ETHER_PORT);
    return ;
}
EXPORT_SYMBOL(km__alloc_skb_hook);

void __always_inline km_kfree_skb_hook(struct sk_buff *skb)
{
#ifdef CONFIG_KM_NF_CONNTRACK
    if (likely(km_l2_nf_conntrack_put)) {
        km_l2_nf_conntrack_put(skb);
    }
#endif

    if (likely(km_hook_put_ct_hook)) {
        km_hook_put_ct_hook(skb);
    }

    return ;
}
EXPORT_SYMBOL(km_kfree_skb_hook);

void km_unregister_netdevice_hook(struct net_device *dev)
{
#ifdef CONFIG_SA_FASTPATH_L2
    /* 注销网络设备时，需通知到内核fast_l2模块，将ct对应的fast_l2_ext进行删
       除，避免设备down掉fast_l2因dev为空导致死机问题 */
    if (km_fast_l2_ext_delete_by_dev) {
        km_fast_l2_ext_delete_by_dev(dev);
    }
#endif
    return ;
}
EXPORT_SYMBOL(km_unregister_netdevice_hook);

void inline km_set_ct_hook(struct sk_buff *skb)
{
    struct nf_conn* ct = NULL;

    ct = (struct nf_conn*)km_get_nf_ct(skb);
    if (!ct || unlikely(nf_ct_is_dying(ct))) {
        return ;
    }

    if (likely(km_hook_set_ct_hook)) {
        km_hook_set_ct_hook(skb, ct);
    }

    return ;
}

EXPORT_SYMBOL(km_set_ct_hook);

void* km_get_ct_hook(struct sk_buff *skb)
{
    if (km_hook_get_ct_hook) {
        return km_hook_get_ct_hook(skb);
    }

    return NULL;
}
EXPORT_SYMBOL(km_get_ct_hook);

void km_ppp_send_frame_hook(struct sk_buff *skb)
{
    if (CTF_TEST_BIT(skb, CTF_UP)) {
        if (NULL == km_get_ct_hook(skb)) {
            km_set_ct_hook(skb);
        }
    }

    return ;
}
EXPORT_SYMBOL(km_ppp_send_frame_hook);

void km_xfrm_output_hook(struct sk_buff *skb)
{
    if (CTF_TEST_BIT(skb, CTF_UP)) {
        if (NULL == km_get_ct_hook(skb)) {
            km_set_ct_hook(skb);
        }
    }

    return ;
}
EXPORT_SYMBOL(km_xfrm_output_hook);

void km_sdw_send_handler_hook(struct sk_buff *skb)
{
    if (CTF_TEST_BIT(skb, CTF_UP)) {
        if (NULL == km_get_ct_hook(skb)) {
            km_set_ct_hook(skb);
        }
    }

    return ;
}
EXPORT_SYMBOL(km_sdw_send_handler_hook);

/*****************************************************************************
 函 数 名  : km_conduct_xmit_skb_gso_feature
 功能描述  : Gso特性校验，如果发送设备不支持Gso数据包转发，则在此处将数据包
             分段
 输入参数  : skb
 输出参数  : 无
 返 回 值  :

 修改历史      :
  1.日    期   : 2020年12月12日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
#define km_conduct_xmit_skb_gso_feature(skb) do {\
    if (netif_needs_gso(skb, skb->dev->features)) {\
        struct sk_buff *segs = NULL;\
        segs = skb_gso_segment(skb, skb->dev->features);\
        if (IS_ERR_OR_NULL(segs)) {\
            consume_skb(skb); \
            rcu_read_unlock_bh();\
            return CTFE_ERROR;\
        }\
        consume_skb(skb);\
        skb = segs;\
    } else {\
        if (skb_needs_linearize(skb, skb->dev->features) && __skb_linearize(skb)) {\
            kfree_skb(skb);\
            rcu_read_unlock_bh();\
            return CTFE_ERROR;\
        }\
    }\
} while (0)

/*****************************************************************************
 函 数 名  : km_ndo_start_xmit_skb
 功能描述  : 数据包直接发送宏函数，支持分段数据包发送
 输入参数  : skb
             dev
             txq
 输出参数  : 无
 返 回 值  :

 修改历史      :
  1.日    期   : 2020年12月12日
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
#define km_ndo_start_xmit_skb(skb, dev, txq) do {  \
    do {\
        struct sk_buff *next = skb->next;\
        int rc = NETDEV_TX_OK;           \
        skb->next = NULL;\
        rc = dev->netdev_ops->ndo_start_xmit(skb, dev);  \
        if (unlikely(!dev_xmit_complete(rc))) {          \
            kfree_skb(skb);                         \
        }                                                \
        txq->trans_start = jiffies;\
        skb = next;\
    } while (skb);\
} while (0)

/*****************************************************************************
 函 数 名  : km_fast_xmit_skb
 功能描述  : 内核模块数据包快速发送统一接口
 输入参数  : struct sk_buff *skb
 输出参数  : 无
 返 回 值  : CTFE_OK:成功 CTFE_ERROR:失败

 修改历史      :
  1.日    期   : 2020年8月12日 星期三
    作    者   : tys
    修改内容   : 新生成函数

*****************************************************************************/
int km_fast_xmit_skb(struct sk_buff *skb)
{
    struct netdev_queue *txq = NULL;
    struct net_device *dev = skb->dev;
    struct Qdisc *q = NULL;
    int cpu = -1;
    int rc = CTFE_OK;

    if (CTF_TEST_BIT(skb, CTF_OR_FAST_L2_ACCELERATE_PACKET)) {
        km_dev_queue_xmit_hook(skb);
        rcu_read_lock_bh();
        if (!(dev->flags & IFF_UP)) {
            rc = CTFE_ERROR;
            goto rc_handle;
        }

        /* 设备驱动会通过skb->queue_mapping选择发包环形队列，故在此处设置 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,2,0)
        txq = netdev_core_pick_tx(dev, skb, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
        txq = netdev_pick_tx(dev, skb, NULL);
#elif LINUX_VERSION_CODE >=  KERNEL_VERSION(3,10,90)
        txq = netdev_pick_tx(dev, skb);
#else
        txq = dev_pick_tx(dev, skb);
#endif
        /* 添加流量整形动作，主要是解决qca_v502方案W15E Wan to wlan跑流波形掉坑
           问题，能够解决该问题的主要原因是:Wlan接口由于队列满等情况部分处理不过
           来的数据包会在__dev_xmit_skb函数入队重发，避免了丢包 */
        if (CTF_TEST_BIT(skb, CTF_SLOW_XMIT_PACKET)) {
            q = rcu_dereference_bh(txq->qdisc);
            if (q->enqueue) {
                __dev_xmit_skb(skb, q, dev, txq);
                goto rc_handle;
            }
        }

        cpu = smp_processor_id();
        /* 避免deadlock */
        if (unlikely(txq->xmit_lock_owner == cpu)) {
            rc = CTFE_ERROR;
            goto rc_handle;
        }
        /* Gso特性校验 */
        km_conduct_xmit_skb_gso_feature(skb);
        /* 调用ndo_start_xmit对应的驱动发包函数时，对队列进行上锁，避免多核
           时同一quene id的数据包同时调用驱动发包，导致quene id对应的驱动发
           包环形队列内存被踩情况发生 */
        HARD_TX_LOCK(dev, txq, cpu);
        /* 数据包发送 */
        km_ndo_start_xmit_skb(skb, dev, txq);
        HARD_TX_UNLOCK(dev, txq);
    } else {
        dev_queue_xmit(skb);
        /* 在else中并没有上锁，使用解锁操作会出现踩内存，出现的一种打印信息
        为在原子操作中被调度 */
        goto rc_handle_no_unlock;
    }

rc_handle:
    rcu_read_unlock_bh();
rc_handle_no_unlock:
    if (CTFE_ERROR == rc) {
        kfree_skb(skb);
    }

    /*
     * 该函数返回值仅有 CTFE_ERROR 和 CTFE_OK 两种，
     * 调用协议栈发包时不关注数据包是否发包成功或者失败，
     * 返回 CTFE_ERROR 情况调用者也不必关注，经过此的数据
     * 包都将被free，后续不能引用。
     */

    return rc;
}
EXPORT_SYMBOL(km_fast_xmit_skb);

int km_consider_fastpath_learning(struct sk_buff *skb)
{
    if (!km_fastpath_need_slow_learning || !km_fastpath_need_slow_learning(skb)) {
        return 0;
    }

    /* 在fastpath不支持限速时，限速情况不用走协议栈学习。 */
#if defined(CONFIG_NOS_CONTROL_V2) && defined(CONFIG_SKIP_FASTPATH)
    if (km_nos_skb_is_limited && km_nos_skb_is_limited(skb)) {
        return 0;
    }
#endif

    return 1;
}
EXPORT_SYMBOL(km_consider_fastpath_learning);

/*****************************************************************************
 函 数 名  : km_upgrading_signal_hook
 功能描述  : 捕获内核发送的信号，在升级时忽略信号;

       在上层tenda_upgrade进程写flash前会设置km_upgrade_ignore_sig
       用来规避在升级写flash过程中，init进程收到内核发送的异常信号（SIGBUS）
       然后异常退出重启系统，导致升级失败的问题;

       升级过程中重启的问题，目前仅在矽昌方案中有出现，其他方案若是出现
       同样问题可以将这个hook点挂到内核 kernel/signal.c force_sig_info 函数
       中忽略掉内核上报的信号。

 输入参数  : signo
           struct task_struct *p
 输出参数  : 无
 返 回 值  :

 修改历史      :
  1.日    期   : 2022年08月25日
    作    者   : jpj
    修改内容   : 新生成函数

*****************************************************************************/
int km_upgrading_signal_hook(int signo, struct task_struct *p)
{
    /* 忽略所有信号 */
    if (km_upgrade_ignore_sig == 1) {
        WARN_ON(p->pid == 1);
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(km_upgrading_signal_hook);

#endif
