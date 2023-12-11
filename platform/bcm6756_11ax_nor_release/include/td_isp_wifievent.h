#ifndef _TD_ISP_WIFIEVENT_H_
#define _TD_ISP_WIFIEVENT_H_

#ifdef __KERNEL__
#include <linux/kernel.h>

#ifdef TD_ISP_BROADCOM
#ifndef wifi_ev_osif
typedef wlc_bsscfg_t wifi_ev_osif;
#endif
#endif

#else /* __KERNEL__ */
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

#endif /* __KERNEL__ */

#define WIFIEVENT_NETLINK_GROUP 1
#define WIFIEVENT_NETLINK 90

#define WIFIEVENT_MSG_MAGIC_LEN     13
#define WIFIEVENT_STA_MAXCNT        128
#define WIFIEVENT_PID               9527
#define WIFIEVENT_EXPIRE_TIME       180
#define WIFIEVENT_MSG_SIZE          512
#define WIFIEVENT_MAX_BEACON_RPT    32
#define WIFIEVENT_MAX_INTERVALCNT   32
#define WIFIEVENT_MSG_MAGIC "isp_wifievent"
#define WIFIEVENT_VERSION "ISP_WIFIEVENT_V1"

#define BSS_BAND2G 0
#define BSS_BAND5G 1

typedef enum wifi_ev {
    WIFI_EV_PING,
    WIFI_EV_CONFIG,
    WIFI_EV_STAINFO,
    WIFI_EV_STA_MONITOR_SET,
    WIFI_EV_STA_MONITOR_GET,

    WIFI_EV_JOIN,
    WIFI_EV_LEAVE,
    WIFI_EV_PASSERR,
    WIFI_EV_BLACKREJ,
    WIFI_EV_MAXCNTREJ,

    WIFI_EV_SET_SSID,
    WIFI_EV_SET_CHANNEL,

    WIFI_EV_BEACREP,
    WIFI_EV_BSSTRANS,
    WIFI_EV_INTERVAL,
    WIFI_EV_IF_STATIS,
    WIFI_EV_RADIO_STATIS,
    WIFI_EV_CHAN_UTIL,
}wifi_ev_e;

struct wifi_ev_config {
    int enable;
    int threshrssi;
    int startrssi;
    unsigned long starttime;
    unsigned long intertime;
};

struct wifi_ev_msg_head {
    char magic[WIFIEVENT_MSG_MAGIC_LEN];
    u_int8_t type;
    u_int8_t msg[0];
};

struct wifi_ev_sta {
    u_int8_t ether_addr_octet[6];
    int associated;
    int band;
    int rssi;
    int sup11k;
    int sup11v;
    unsigned long last_time;
    char ifname[16];
};

struct wifi_ev_interval {
    int count;
    struct wifi_ev_sta sta[WIFIEVENT_MAX_INTERVALCNT];
};

struct wifi_ev_stamon_msg {
    u_int8_t ether_addr_octet[6];
    int rssi;
};

struct wifi_ev_trans_msg {
    u_int8_t sta_mac[6];
    u_int8_t bssid[6];
    unsigned int channel;
    u_int8_t status;
};

struct wifi_ev_beacon_rep_cmd {
    u_int8_t sta_mac[6];
    char ssid[33];
    unsigned int channel;
};

struct wifi_ev_beacon_rep {
    int8_t rcpi;
    u_int8_t bssid[6];
};

struct wifi_ev_beacon_rep_msg {
    u_int8_t sta_mac[6];
    u_int8_t count;
    struct wifi_ev_beacon_rep reports[WIFIEVENT_MAX_BEACON_RPT];
};

struct wifi_ev_if_statistics {
    char ifname[16];
    uint64_t txframe;
    uint64_t txbytes;
    uint64_t txerror;
    uint64_t txdropd;
    uint64_t rxframe;
    uint64_t rxbytes;
    uint64_t rxerror;
    uint64_t rxdropd;
};

struct wifi_ev_ssid_cfg {
    int band;
    char ifname[16];
    unsigned char ssid[33];
    unsigned int  ssid_len;
};

struct wifi_ev_channel_cfg {
    int band;
    unsigned int channel;
};

struct wifi_ev_channel_util {
    int band;
    unsigned char chan_util;
};

#ifdef __KERNEL__
#define wifi_ev_err(fmt, args...) \
printk("isp_wifievent: ERROR %s(%d), "fmt"\n", __func__, __LINE__, ##args)

int wifi_ev_ioctl_handler(void *data, size_t len, wifi_ev_osif *osif);
void wifi_ev_sta_join_handler(u_int8_t *ether_addr_octet, char *dev_name, int rssi, int band, int sup11k, int sup11v);
void wifi_ev_sta_leave_handler(u_int8_t *ether_addr_octet);
void wifi_ev_sta_passerr_handler(u_int8_t *ether_addr_octet);
void wifi_ev_sta_blackrej_handler(u_int8_t *ether_addr_octet);
void wifi_ev_sta_maxcntrej_handler(u_int8_t *ether_addr_octet);

void wifi_ev_set_ssid_handler(int band, char *ifname, unsigned char *ssid, unsigned int ssid_len);
void wifi_ev_set_channel_handler(int band, unsigned int channel);

void wifi_ev_beacon_rep_handler(u_int8_t *ether_addr_octet, void *data);
void wifi_ev_bss_trans_res_handler(u_int8_t *ether_addr_octet, unsigned char status);

void wifi_ev_sta_update_rssi(u_int8_t *ether_addr_octet, int rssi);
void wifi_ev_update_chan_util(int band, unsigned char chan_util);

unsigned int wifi_ev_spec_to_chan(unsigned short chspec);

void wifi_ev_init(void);
void wifi_ev_exit(void);
#endif

#endif
