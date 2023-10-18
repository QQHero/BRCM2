#ifndef _TD_PRIVATE_IE_H_
#define _TD_PRIVATE_IE_H_

#include <linux/types.h>
#include <linux/netdevice.h>

/* 0x80~0xff are used by userspace */
#define PIE_USER_PORT_START 0x80
#define TD_PRIVATE_OUI "\xc8\x53\x5a"
#define TD_IE_VENDOR 221
#define TD_OUI_LEN 3

#define TD_PRIVATE_IE_KEY "THEMESISPASSED"
/* 帧类型 */
typedef enum td_pe_frame_type {
    TD_PE_FRAME_TYPE_PROBE_REQ,
    TD_PE_FRAME_TYPE_BEACON,
    TD_PE_FRAME_TYPE_PROBE_RES,
    TD_PE_FRAME_TYPE_ASSOC_REQ,
    TD_PE_FRAME_TYPE_ASSOC_RES,
    TD_PE_FRAME_TYPE_AUTH,
    TD_PE_FRAME_TYPE_UNKNOW,
} td_pe_frame_type_e;

/* private ie模块厂商信息规则结构体 */
typedef struct td_ie_oui_node {
    unsigned char band;
    unsigned char oui[3];
    struct list_head list_node;
} td_ie_oui_node_t;

#ifdef BCM
/* following symbols are defined in td_ie_manage.c for QCA、BCM, in 8192cd_sme.c for RTK */
extern int (*td_add_elements_cb)(unsigned char *buff, size_t size, struct net_device *dev, int frame_type);
#else
extern int (*td_add_elements_cb)(unsigned char *buff, size_t size, struct net_device *dev);
#endif

#ifdef BCM //begain changed by bcz 20210515
extern void (*td_save_element_cb)(unsigned char *content, unsigned char len, int rssi, 
    const unsigned char *sa, int frame_type, int band);
#else
extern void (*td_save_element_cb)(unsigned char *content, unsigned char len);
#endif //end changed by bcz 20210515


#ifdef BCM
extern unsigned int (*td_calc_element_cb)(struct net_device *dev);
#endif

void td_update_dev_beacon(struct net_device *dev);

#ifdef BCM
static inline int pie_add_elements(unsigned char *buff, size_t size, 
                                    struct net_device *dev, int frame_type)
{
    if (!td_add_elements_cb) {
        return 0;
    }
    return td_add_elements_cb(buff, size, dev, frame_type);
}
#else
static inline int pie_add_elements(unsigned char *buff, size_t size, 
                                    struct net_device *dev)
{
    if (!td_add_elements_cb) {
        return 0;
    }
    return td_add_elements_cb(buff, size, dev);
}
#endif

#ifdef BCM //begain changed by bcz 20210515
static inline void pie_save_element(unsigned char *content, unsigned char len, int rssi, 
    const unsigned char *sa, int frame_type, int band)
{
    if (td_save_element_cb) {
        td_save_element_cb(content, len, rssi, sa, frame_type, band);
    }
    return;
}
#else
static inline void pie_save_element(unsigned char *content, unsigned char len)
{
    if (td_save_element_cb) {
        td_save_element_cb(content, len);
    }
    return;
}
#endif //end changed by bcz 20210515



#ifdef BCM //begain changed by bcz 20210515
static inline unsigned int pie_calc_element(struct net_device *dev)
{
    if (!td_calc_element_cb) {
        return 0;
    }

    return td_calc_element_cb(dev);
}
#endif //end changed by bcz 20210515

#endif
