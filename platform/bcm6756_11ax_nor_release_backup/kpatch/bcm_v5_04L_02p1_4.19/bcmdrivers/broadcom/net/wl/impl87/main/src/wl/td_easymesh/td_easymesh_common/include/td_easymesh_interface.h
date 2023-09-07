#ifndef __TD_EASYMESH_INTERFACE_H__
#define __TD_EASYMESH_INTERFACE_H__

#ifdef TD_EM_BROADCOM
#include <wlc_types.h>
typedef wlc_bsscfg_t* em_osif;
#endif

#ifdef CONFIG_RTL8832BR
#ifdef TD_EASYMESH_8832BR_WIFI6
#include <drv_types.h>
typedef struct _ADAPTER* em_osif;
typedef struct sta_info em_ossta;
#else
#include <8192cd.h>
typedef struct rtl8192cd_priv* em_osif;
typedef struct stat_info em_ossta;
#endif
#endif


typedef enum tm_em_country_map
{
    TD_EM_GLOBAl = 0,
    TD_EM_FCC    = 1,
    TD_EM_ETSI   = 2,
    TD_EM_MKK    = 3,
}tm_em_country_map_e;

typedef struct td_em_scanresult_list {
    unsigned char count;
    char *channel_info;
} td_em_scanresult_list_t;

extern bool vap_is_up(em_osif osif);

extern int td_em_get_sta_link_time(void *sta, void *arg);

extern bool td_em_check_drv_state(em_osif osif);

extern int td_em_get_sta_num(em_osif osif);

extern td_em_device_type_e td_em_get_media_type(em_osif osif);

extern unsigned int td_em_get_centerfreq(em_osif osif);

extern td_em_workmode_e td_em_get_interface_mode(em_osif osif);

extern int td_em_get_sta_traffic(em_osif osif, td_em_associated_sta_traffic_tlv_t *sta_info, unsigned char *sta_mac);

//extern bool td_em_sta_isassoc(em_osif osif, unsigned char *sta_mac);

extern tm_em_country_map_e td_em_get_country_map(em_osif osif);

extern int td_em_get_max_min_channel(em_osif osif, unsigned int *maxch, unsigned int *minch);

extern td_em_bw_e td_em_get_channel_width(em_osif osif);

extern int td_em_get_max_power(em_osif osif);

extern int td_em_get_mac_addr(em_osif osif, unsigned char* mac_addr);

extern int td_em_get_ssid(em_osif osif, unsigned char* ssid);

extern td_em_authtype_e td_em_get_auth_type(em_osif osif);

extern td_em_encryption_type_e td_em_get_encryption_type(em_osif osif);

extern td_em_bss_type_e td_em_get_bss_type(em_osif osif);

extern void td_em_get_netkey(em_osif osif, unsigned char *key);

//extern int td_em_get_opclass_by_chnl(em_osif osif, int channel);

extern int td_em_set_steer_policy(em_osif osif, unsigned char *buf);

extern int td_em_do_backhaul_steer(em_osif osif, unsigned char *tmpbuf);

extern int td_em_get_bh_steer_results(em_osif osif, backhaul_steering_response_t *bh_steer_rsp);

extern int td_em_client_steer(em_osif osif, unsigned char *buf);

extern int td_em_client_assoc_sta_control(em_osif osif, unsigned char *buf);

extern int td_em_get_apcapability(em_osif osif, unsigned char *result_buf);

extern int td_em_get_apcapability_2(em_osif osif, unsigned char *result_buf);

extern int td_em_get_htcapability(em_osif osif, unsigned char *result_buf);

extern int td_em_get_vhtapCapability(em_osif osif, unsigned char *result_buf);

extern int td_em_get_heapcapability(em_osif osif, unsigned char *result_buf);

extern int td_em_backhaul_sta_radio_capabilities(em_osif osif, unsigned char *result_buf);

extern int td_em_get_apmetric(em_osif osif, unsigned char *result_buf);

extern int td_em_get_assocstatrafficstats(em_osif osif, unsigned char *result_buf);

extern int td_em_get_assocstalinkmetric(em_osif osif, unsigned char *result_buf, int sizeofbuf);

extern int td_em_get_extendedapmetric(em_osif osif, unsigned char *result_buf);

extern int td_em_get_radiometric(em_osif osif, unsigned char *result_buf);

extern int td_easymesh_get_associated_sta_extended_link_metrics(em_osif osif, unsigned char *send_buf);

extern int td_em_do_channelscan(em_osif osif);

extern void td_em_ap_save_channelscanresults(em_osif osif, unsigned char* result_buf, td_em_scanresult_list_t * list);

extern void td_em_get_channelscanresults_tlv_lenth(em_osif osif, int *len, td_em_scanresult_list_t * list);

extern void td_em_priv_malloc_free(em_osif osif, td_em_scanresult_list_t * list);

extern bool td_em_is_support_btm(em_osif osif, unsigned char *mac);

extern int td_em_send_btm_req(em_osif osif, int channel, int opclass, unsigned char *target_bss,unsigned char *sta);

extern int td_easymesh_beacon_metrics_rsp_notifiy(void *osif, unsigned char *result_buf, int *sizeofbuf);

extern int td_easymesh_1905_txlink_metric(em_osif osif, unsigned char *result_buf, int *sizeofbuf);

extern int td_easymesh_1905_rxlink_metric(em_osif osif, unsigned char *result_buf, int *sizeofbuf);

extern int td_do_beacon_request(em_osif osif, unsigned char *send_buf);

extern int td_em_iterate_sta_list(em_osif osif, int sta_num, unsigned char * data, int (*cb)(void *sta, void *arg));

extern int td_em_get_sae_cap(em_osif osif);

extern int td_em_eth_connect_driver(em_osif osif, unsigned char *send_buf);

extern int td_em_get_eth_connect_status(em_osif osif, unsigned char *send_buf, int *sizeofbuf);

extern int td_em_priority_assoc_flag_drv(em_osif osif, unsigned char *send_buf);

extern int td_em_set_role_drv(em_osif osif, unsigned char *buf);

extern int td_em_set_xmesh_enable_drv(em_osif osif, unsigned char *buf);

extern int td_em_get_ext_assoc_status(em_osif osif);

#ifdef CONFIG_TENDA_XMESH_HB_ENABLE
extern int  td_em_set_xmesh_hb_drv(em_osif osif, unsigned char *buf);

extern int  td_em_xmesh_set_bss_drv(em_osif osif, unsigned char *buf);

extern void td_em_deinit_hbtimer(void);
#endif

#endif
