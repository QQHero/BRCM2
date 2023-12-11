/**
*date:2022-04-26
*desc:
    1、该文件主要是用来定义wifi相关全局变量、指针函数；
    2、该文件不允许有任何功能实现的代码添加进来，如果需要实现具体的功能
        需要添加对应的模块，任何将对应指针函数在这里定义；
**/
#include <net/sock.h>
#include <net/wifi_common.h>


#ifdef CONFIG_TENDA_PRIVATE_WLAN
void *td_maps_config = NULL;
EXPORT_SYMBOL(td_maps_config);
struct sock *td_maps_nl_sock = NULL;
EXPORT_SYMBOL(td_maps_nl_sock);
void *td_maps_radios = NULL;
EXPORT_SYMBOL(td_maps_radios);
void *td_maps_sta_list = NULL;
EXPORT_SYMBOL(td_maps_sta_list);
int td_maps_dbg = 0;
EXPORT_SYMBOL(td_maps_dbg);
struct hlist_head td_maps_sta_h_heads[MAPS_HASH_SIZE];
EXPORT_SYMBOL(td_maps_sta_h_heads);
u_int32_t td_maps_hash_rnd;
EXPORT_SYMBOL(td_maps_hash_rnd);
void *td_maps_mng_bss = NULL;
EXPORT_SYMBOL(td_maps_mng_bss);
void (*td_wifi_led_counts_hook)(void *dev, int flag) = NULL;
EXPORT_SYMBOL(td_wifi_led_counts_hook);
struct timer_list td_maps_timer;
EXPORT_SYMBOL(td_maps_timer);
DEFINE_SPINLOCK(s_steer_lock);
EXPORT_SYMBOL(s_steer_lock);
#endif