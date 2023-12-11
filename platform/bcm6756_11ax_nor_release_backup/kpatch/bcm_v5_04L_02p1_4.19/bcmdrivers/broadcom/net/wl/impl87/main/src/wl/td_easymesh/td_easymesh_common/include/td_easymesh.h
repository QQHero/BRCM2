#ifndef	_TD_EASYMESH_H_
#define _TD_EASYMESH_H_
/* 只给无线驱动用 */

#include "easymesh_shared.h" 
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
#include "td_easymesh_interface.h"
#endif

#ifndef _TD_EASYMESH_C_
#define EXTERN  extern
#else
#define EXTERN
#endif

#define STA_ASSOC_ALLOW      (1)
#define STA_ASSOC_UNALLOW    (0) 
#define AGENT_ETH_UP         (1)
#define EM_CONTROLLER_BIT  0x1
#define EM_XMESH_BIT       0x2
#define EM_PRIO_ASSOC_BIT  0x4
#define EM_ETHX_BIT        0x8
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
#define TD_EM_MAX_BAND     3
#endif
#define BROADCAST_MACADDR "ff:ff:ff:ff:ff:ff"

typedef enum easymesh_op
{
    EM_OP_NONE                         = 0,

    EM_ASSOC_CLIENT_INFO               = 1,
    EM_DEVICE_INFO                     = 2,
    EM_AP_OPERATE_BSS                  = 3,
    EM_AP_BASIC_CAP                    = 4,
    EM_AP_RADIO_ID                     = 5,
    EM_AUTO_CONFIG_M1                  = 6,
    EM_AUTO_CONFIG_M2                  = 7,
    EM_HOSTAPD_NOTIFY                  = 11,
    EM_SAE_CAP                         = 10,
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
    EM_XMESH_SET_BSS                   = 12,
#endif
    EM_CHANNEL_PREFER                  = 30,
    EM_RADIO_OPERATE_RESTRICT          = 31,
    EM_CAC_COMPLETION_REPORT           = 32,
    EM_CAC_STATUS_REPORT               = 33,
    EM_TRANSMIT_POWER_LIMIT            = 34,
    EM_CHANNEL_SELECT_RESPONSE         = 35,
    EM_OPERATE_CHANNEL_REPORT          = 36,
    EM_CAC_REQUEST                     = 37,
    EM_CAC_TERMINATION                 = 38,
    EM_BACKHAUL_STEER_REQ              = 39,
    EM_BACKHAUL_STEER_RSP              = 40,
    EM_STEER_POLICY                    = 41,
    EM_METRIC_REPORT_POLICY            = 42,
    EM_DEFAULT_802_1Q_SETING           = 43,
    EM_CHANNEL_SCAN_REPORT_POLICY      = 44,
    EM_UNSUCCESSFUL_ASSOCIAT_POLICY    = 45,
    EM_BACKHAUL_BSS_CONFIGURATION      = 46,
    EM_CLIENT_STEER_REQUEST            = 47,
    EM_CLIENT_STEER_BTM_REQUEST        = 48,
    EM_CLIENT_ASSOCIAT_CONTROL_REQ     = 49,
    EM_TUNNELED_MESSAGE                = 50,
    EM_ASSOCIATION_STATUS_NOTIFY       = 51,
    EM_AP_CAPABILITY                   = 61,
    EM_HT_AP_CAPABILITY                = 62,
    EM_VHT_AP_CAPABILITY               = 63,
    EM_HE_AP_CAPABILITY                = 64,
    EM_CHSCAN_CAPABILITY               = 65,
    EM_AP_CAPABILITY_2                 = 66,
    EM_BACKHAUL_CAPABILITY             = 67,
    EM_AP_METRIC_QUERY                 = 68,
    EM_CAC_CAPABILITY                  = 69,
    EM_AP_METRICS                      = 70,
    EM_ASSOC_STA_TRAFFIC_STAT          = 71,
    EM_ASSOC_STA_LINK_METRICS          = 72,
    EM_AP_EXTENDED_METRICS             = 73,
    EM_RADIO_METRICS                   = 74,
    EM_ASSOC_STA_EXTEND_LINK_METRICS   = 75,
    EM_DO_CHANNEL_SCAN                 = 76,
    EM_1905_TXLINK_METRIC              = 77,
    EM_1905_RXLINK_METRIC              = 78,
    EM_BEACON_METRICS_QUERY            = 79,
    EM_ETHX_CONNECT                    = 80,
    EM_GET_ETH_STATUS                  = 81,
    EM_ASSOC_PRIO_FLAG                 = 82,
    EM_ROLE                            = 83,
    EM_XMESH_ENABLE                    = 84,
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
    EM_XMESH_HEART_BEAT                = 85,
#endif
    EM_OP_MAX
}easymesh_op_e;


#ifndef TD_EM_BROADCOM
typedef struct easymesh_ioctl_frame
{
    easymesh_op_e type;
    int input_len;
    int output_len;
    unsigned char *data;
}em_io_frame_t;
#endif

extern int g_multiap_assoc_status;
extern int g_em_mesh_cfg;

EXTERN int td_easymesh_ioctl(void* osif, char *arg, int len);
EXTERN void td_em_client_event(typeof(__FUNCTION__) func, typeof(__LINE__) line, void* osif, char *bss_mac, char *sta_mac, netlink_event_type_e event, netlink_rson_code_e reason, netlink_status_code_e status);
EXTERN void td_em_clientcapability_notify(unsigned char *mac, unsigned char *bssid, unsigned int framelength, unsigned char *framedata);
EXTERN void td_easymesh_beacon_metrics_rsp_notifiy_tlv(void *osif);
EXTERN void td_em_wds_addif_br_notify(char *ifname);
EXTERN void td_em_extend_down_notify(unsigned int cfg);
EXTERN void td_em_roam_notify(u8 *curbssid);
EXTERN void td_em_ap_updatechannelscanresults(void *osif);
EXTERN int td_em_is_extend_intf_assoc(void *osif);
void td_em_sta_tran_res_handler(unsigned char *sta_mac, 
                                void *osif, 
                                unsigned char status, unsigned char *target_bss);
#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
EXTERN int td_em_set_xmesh_hb(em_osif osif, unsigned char *buf);
#endif

EXTERN void td_easymesh_exit(void);
EXTERN int td_easymesh_init(void);

#endif

