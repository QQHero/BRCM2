#ifndef __TD_WL_CORE_SYMB_H__
#define __TD_WL_CORE_SYMB_H__


#ifdef __cplusplus
extern "C" {
#endif

#if (!defined(DONGLEBUILD)) && defined(CONFIG_UGW_BCM_DONGLE)
#define TDCORE_FUNC_ASSIGN(func) (tdcore_##func = func)
#define TDCORE_FUNC_ASSIGN_TMP(func) (tdcore_##func = func##_tmp)
#define TDCORE_FUNC_UNASSIGN(func) (tdcore_##func = NULL)
#define TDCORE_FUNC(func) tdcore_##func
#define TDCORE_FUNC_IS_NULL(func) !tdcore_##func
#define TDCORE_FUNC_IF(func) if(!TDCORE_FUNC_IS_NULL(func))
#define TDCORE_STRUCT(var) tdcore_##var

#else
#define TDCORE_FUNC_ASSIGN(func)
#define TDCORE_FUNC_UNASSIGN(func)
#define TDCORE_FUNC(func) func
#define TDCORE_FUNC_IS_NULL(func) 0
#define TDCORE_FUNC_ASSIGN_TMP(func)
#define TDCORE_FUNC_IF(func)
#define TDCORE_STRUCT(var) &var
#endif


#ifndef DONGLEBUILD


extern void (*tdcore_maps_disassoc_handler)(void *vsta, void *vap);
extern void (*tdcore_maps_assoc_handler)(void *vsta, void *vap);
extern int  (*tdcore_maps_auth_handler)(unsigned char *mac, signed char rssi, void *vap);
extern int  (*tdcore_maps_probe_req_handler)(unsigned char *mac, signed char rssi, void *vap,
                             unsigned char ht, unsigned char vht, unsigned char he, unsigned char channel);
extern void (*tdcore_maps_bss_tran_res_handler)(void *vsta, void *vap, unsigned char status);
extern void (*tdcore_maps_beacon_rep_handler)(void *vsta, void *vap);
extern void (*tdcore_maps_vap_available_handler)(void *vap);
extern void (*tdcore_maps_vap_unavailable_handler)(void *vap);
extern int  (*tdcore_maps_ioctl_handler)(void *data, unsigned int len, void* vap);
extern int  (*tdcore_maps_set_dbg)(void *msg, unsigned int msg_len);
extern void (*tdcore_maps_timer_handler)(void *wlc);
extern int td_maps_dbg;


extern unsigned short (*tdcore_wf_chspec_primary20_chspec)(unsigned short chspec);
extern unsigned int (*tdcore_wlc_ratespec_nss)(unsigned int rspec);
extern unsigned char (*tdcore_wlc_get_regclass)(void *wlc_cmi, unsigned short chanspec);




extern int (*tdcore_kwb_nl_multicast_send)(char *data, int data_len, unsigned int group,
            unsigned int tdmid);
extern void (*tdcore_bcm_wifi_event_handler)(void *event, void **data_ptr, unsigned int event_type, unsigned int sub_type, unsigned int datalen);


extern void *tdcore_g_em_timer;
extern int (*tdcore_easymesh_ioctl)(void *osif, char *arg, int len);
extern void (*tdcore_td_em_sta_tran_res_handler)(unsigned char *sta_mac, void *osif, unsigned char status, unsigned char *target_bssid);
extern int (*tdcore_td_em_chanim_get_stats_cb)(void *c_info, void* iob, int *len, int cnt);
extern void (*tdcore_td_em_ap_updatechannelscanresults)(void *osif);
extern void (*tdcore_td_em_wds_addif_br_notify)(char *ifname, void *osif);
extern void (*tdcore_td_em_client_event)(void* osif, char *bss_mac, char *sta_mac, int event, int reason, int status);
extern void (*tdcore_td_em_clientcapability_notify)(unsigned char *mac, unsigned char *bssid, unsigned int framelength, unsigned char *framedata, void *osif);
extern void (*tdcore_td_easymesh_beacon_metrics_rsp_notifiy_tlv)(void *sta_if, void *osif);
extern int (*tdcore_wlc_us_code)(void *wlc);
extern int (*tdcore_wlc_eu_code)(void *wlc);
extern int (*tdcore_wlc_get_channels_in_country)(void *wlc, void *arg);
extern int (*tdcore_td_get_wlc_txbf)(void *txbf);
extern void (*tdcore_wlc_scb_iterinit)(void *scbstate, void *scbiter, void *bsscfg);
extern void* (*tdcore_wlc_bsscfg_find_by_hwaddr)(void *wlc, void *hwaddr);
extern int (*tdcore_wlc_channel_get_cur_rclass)(void *wlc);
extern unsigned char (*tdcore_wlc_ampdu_tx_get_ba_tx_wsize)(void *ampdu_tx);
extern void* (*tdcore_wlc_lq_chanspec_to_chanim_stats)(void *c_info, int chanspec_t);
extern int (*tdcore_wlc_ioctl)(void *wlc, int cmd, void *arg, int len, void *wlcif);
extern unsigned int (*tdcore_wlc_vht_get_cap_info)(void *vhti);
extern int (*tdcore_wlc_iovar_getint)(void *wlc, const char *name, int *arg);
extern unsigned int (*tdcore_wlc_wnm_get_scbcap)(void *wlc, void *scb);
extern unsigned short (*tdcore_wlc_ht_get_cap)(void *pub);
extern unsigned int (*tdcore_wlc_get_rspec_history)(void *cfg);
extern const char* (*tdcore_wlc_channel_country_abbrev)(void *wlc_cm);
extern int (*tdcore_wlc_japan)(void *wlc);
extern void* (*tdcore_wlc_scbfindband)(void *wlc, void *bsscfg, void *ea, int bandunit);
extern void* (*tdcore_wlc_scbfind)(void *wlc, void *bsscfg, void *ea);
extern unsigned int (*tdcore_wf_rspec_to_rate)(unsigned int rspec);
extern int (*tdcore_wlc_custom_scan)(void *wlc, char *arg, int arg_len, unsigned short chanspec_start, int macreq, void *cfg);
extern int (*tdcore_bcmwifi_rclass_get_opclass)(unsigned char type, unsigned short chanspec, void *rclass);
extern unsigned short (*tdcore_phy_utils_get_chanspec)(void *pi);
extern unsigned char (*tdcore_wf_chspec_primary20_chan)(unsigned short);
extern void (*tdcore_wlc_bss_mac_event)(void* wlc, void *bsscfg, unsigned int msg, void *addr, unsigned int result, unsigned int status, unsigned int auth_type, void *data, int datalen);
extern void* (*tdcore_wlc_scb_iternext)(void *scbstate, void *scbiter);
extern void* (*tdcore_wl_init_timer)(void *wl, void (*fn)(void *arg), void *arg, const char *tname);
extern void (*tdcore_wl_add_timer)(void  *wl, void *t, unsigned int ms, int periodic);
extern int (*tdcore_wl_del_timer)(void *wl, void *t);
extern int g_em_mesh_cfg;
extern void (*tdcore_td_em_extend_down_notify_linux)(unsigned int cfg);
extern int (*tdcore_td_em_is_extend_intf_assoc)(void *osif);


extern int (*tdcore_bcm_wl_ioctl)(char *name, int cmd, void *buf, int len);
extern void (*tdcore_td_skb_debug)(void *p, char is_tx, const char *func, int line);
extern unsigned int *tdcore_g_td_debug;
extern unsigned int *tdcore_g_td_easymesh_dbg;
extern void *tdcore_g_td_em_fake_data;
extern void (*tdcore_td_wldebug_ioctl)(char *buffer, unsigned int plen);


extern int (*tdcore_bcm_iovar_getbuf)(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, int buflen);
extern int (*tdcore_td_private_ie_cmd_ioctl)(void *ubuf, unsigned int size);
extern int (*tdcore_td_private_ie_port_operation)(void *ubuf, unsigned int size, void *kbuf, unsigned int buf_len);
extern void (*tdcore_td_recv_ie)(void *ele, void *vap, void *wlc, const unsigned char *sa, int frame_type, int rssi);
typedef unsigned int (*wlc_iem_calc_xx_fn_t)(void *ctx, void *data);
typedef int (*wlc_iem_xx_fn_t)(void *ctx, void *data);
extern int (*tdcore_wlc_iem_vs_add_parse_fn_mft)(void *iem, unsigned short fstbmp,
    unsigned short id, wlc_iem_xx_fn_t parse_fn, void *ctx);
extern int (*tdcore_wlc_iem_vs_add_build_fn_mft)(void *iem, unsigned short fstbmp,
    unsigned short prio,
    wlc_iem_calc_xx_fn_t calc_fn, wlc_iem_xx_fn_t build_fn, void *ctx);
extern void (*tdcore_td_ie_init)(void *wlc);

extern void* (*tdcore_wlc_bsscfg_find_by_wlcif)(void *wlc, void *wlcif);
extern void (*tdcore_wlc_update_beacon)(void *wlc);
extern void (*tdcore_wlc_update_probe_resp)(void *wlc, unsigned int  suspend);
typedef void (*td_schedule_task_fn_t)(void * task);
extern int (*tdcore_wl_schedule_task)(void *wl, td_schedule_task_fn_t fn, void *context);
extern void* (*tdcore_find_bsscfg_dev_by_ifname)(char *name);

#else

extern int bcm_iovar_getbuf(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, int buflen);
extern int td_private_ie_cmd_ioctl(void *ubuf, unsigned int size);
extern int td_private_ie_port_operation(void *ubuf, unsigned int size, void *kbuf, unsigned int buf_len);
extern void* find_bsscfg_dev_by_ifname(char * name);


#ifdef TD_EASYMESH_SUPPORT
extern int easymesh_ioctl(void *osif, char *arg, int len);
extern void td_em_sta_tran_res_handler(unsigned char *sta_mac, void *osif, unsigned char status, unsigned char *target_bssid);
extern void td_em_ap_updatechannelscanresults(void *osif);
extern void td_em_wds_addif_br_notify(char *ifname, void *osif);
extern void td_em_client_event(void* osif, char *bss_mac, char *sta_mac, int event, int reason, int status);
extern void td_em_clientcapability_notify(unsigned char *mac, unsigned char *bssid, unsigned int framelength, unsigned char *framedata, void *osif);
extern void td_easymesh_beacon_metrics_rsp_notifiy_tlv(void *sta_if, void *osif);
extern int g_em_mesh_cfg;
extern void td_em_extend_down_notify_linux(unsigned int cfg, void *osif);
extern int td_em_is_extend_intf_assoc(void *osif);

#endif
extern void td_wldebug_ioctl(char *buffer, unsigned int plen);
extern int maps_set_dbg(void *msg, unsigned int msg_len);
#endif /*DONGLE_BUILD*/

#ifdef __cplusplus
}
#endif
#endif /*__TD_WL_CORE_SYMB_H__*/

