#ifndef _TD_IE_MANAGE_H_
#define _TD_IE_MANAGE_H_


#include "td_private_ie.h"
#include "td_vendor_operations.h"

#include <linux/kernel.h>
//#include <wlc_types.h>

#define DEFAULT_TD_IE_LIMIT 256
#define TD_MAX_KERNEL_IE 64

typedef void (*te_ie_callback_t)(unsigned char *data, unsigned char len, 
            ven_vap_t *vap, const unsigned char *sa, td_pe_frame_type_e frame_type);

/* 
+------------------+---------------+-------------+-------------+------+
| ElementID(1byte) | length(1byte) | OUI(3bytes) | port(1byte) | data |
+------------------+---------------+-------------+-------------+------+
*/
typedef struct td_element {
    unsigned char element_id;
    unsigned char len;
    unsigned char oui[TD_OUI_LEN];
    unsigned char port;
    unsigned char data[0];
} __packed td_element_t;

void td_recv_ie(td_element_t *ele, 
                ven_vap_t *vap, 
                wlc_info_t *wlc,
                const unsigned char *sa, 
                td_pe_frame_type_e frame_type,
                int rssi);

static inline bool is_td_ie(td_element_t *ele)
{

    return (ele->len > TD_OUI_LEN && 0 == 
            memcmp(ele->oui, TD_PRIVATE_OUI, TD_OUI_LEN));
}

size_t td_add_ie(unsigned char *buff, size_t size, 
                ven_vap_t *vap, td_pe_frame_type_e frame_type);


static inline struct net_device* ven_vap_to_dev(ven_vap_t *vap)
{
    if (unlikely(!vap)) {
        return NULL;
    }
    return vap->wlcif->wlif->dev;
}

static inline ven_vap_t* ven_dev_to_vap(struct net_device *dev)
{
    if (unlikely(!dev)) {
        return NULL;
    }

    return wlc_bsscfg_find_by_wlcif(WL_INFO_GET(dev)->wlc, WL_DEV_IF(dev)->wlcif);
}

#ifdef CONFIG_PRIVATE_IE_ENCRYPT
int wl_schedule_task(wl_info_t *wl, void (*fn)(struct wl_task *), void *context);

static void __td_update_beacon(wl_task_t *task)
{
    wlc_info_t *wlc = (wlc_info_t *)task->context;

    WL_LOCK(wlc->wl);

    wlc_update_beacon(wlc);
    wlc_update_probe_resp(wlc, true);

    WL_UNLOCK(wlc->wl);
    kfree(task);
}
#endif

static inline void td_update_beacon(ven_vap_t *vap)
{
    /* nothing to do for RTK and BCM*/
#ifdef VENDOR_QCA
    IEEE80211_VAP_APPIE_UPDATE_ENABLE(vap);

    return ;
#endif
#ifdef CONFIG_PRIVATE_IE_ENCRYPT
    if (!vap) {
        return;
    }

    wl_schedule_task(vap->wlc->wl, __td_update_beacon, vap->wlc);
#endif
    return ;
}

int td_register_ie(ven_vap_t *vap, unsigned char port, 
                    unsigned char frame_types, unsigned char* data, 
                    unsigned char len, te_ie_callback_t rx_callback);

int td_unregister_ie(ven_vap_t *vap, unsigned char port);

int td_update_ie(ven_vap_t *vap, unsigned char port, 
                unsigned char* data, unsigned char len);

int td_clean_vap_ie(ven_vap_t *vap);

static inline int td_register_ie_dev(struct net_device *dev, unsigned char port, 
                            unsigned char frame_types, unsigned char* data, 
                            unsigned char len, te_ie_callback_t rx_callback)
{
    ven_vap_t *vap = ven_dev_to_vap(dev);
    if (!vap) {
        return -EPERM;
    }
    return td_register_ie(vap, port, frame_types, data, len, rx_callback);
}

static inline int td_unregister_ie_dev(struct net_device *dev, unsigned char port)
{
    ven_vap_t *vap = ven_dev_to_vap(dev);
    if (!vap) {
        return -EPERM;
    }
    return td_unregister_ie(vap, port);
}

static inline int td_update_ie_dev(struct net_device *dev, unsigned char port, 
                                    unsigned char* data, unsigned char len)
{
    ven_vap_t *vap = ven_dev_to_vap(dev);
    if (!vap) {
        return -EPERM;
    }
    return td_update_ie(vap, port, data, len);
}

void td_ie_init(wlc_info_t *wlc);
void td_ie_exit(void);

#endif /* _TD_IE_MANAGE_H_ */
