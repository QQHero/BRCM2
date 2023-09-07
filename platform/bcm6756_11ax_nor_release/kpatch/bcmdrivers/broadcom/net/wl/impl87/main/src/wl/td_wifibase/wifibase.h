#ifndef __WIFIBASE_H__
#define __WIFIBASE_H__

#include <linux/netdevice.h>
#include "kwb_common.h"
#include "kwb_ioctl.h"
#include <ethernet.h>

/*lint  -e18*/
int kwb_bcm_kernel_init(wb_fn_ctx_t *ctx);

#ifdef CONFIG_WB_EASYMESH
typedef struct easymesh_driver_hook {
    int (*td_easymesh_ioctl_hook)(void *osif, char *arg, int len);
    void (*td_em_client_event_hook)(typeof(__FUNCTION__) func, typeof(__LINE__) line, void* osif, char *bss_mac, char *sta_mac, netlink_event_type_e event, netlink_rson_code_e reason, netlink_status_code_e status);
    void (*td_em_clientcapability_notify_hook)(unsigned char *mac, unsigned char *bssid, unsigned int framelength, unsigned char *framedata);
    void (*td_easymesh_beacon_metrics_rsp_notifiy_tlv_hook)(void *osif);
    void (*td_em_wds_addif_br_notify_hook)(char *ifname);
    void (*td_em_extend_down_notify_hook)(unsigned int cfg);
    void (*td_em_ap_updatechannelscanresults_hook)(void *osif);
}easymesh_driver_hook_t;
extern struct easymesh_driver_hook g_easymesh_driver_hook;
extern struct td_em_fake_data g_td_em_fake_data;
extern int g_em_mesh_cfg;
extern unsigned int g_td_debug;
extern struct ether_addr g_print_mac;
#endif
/*lint  +e18*/
#endif