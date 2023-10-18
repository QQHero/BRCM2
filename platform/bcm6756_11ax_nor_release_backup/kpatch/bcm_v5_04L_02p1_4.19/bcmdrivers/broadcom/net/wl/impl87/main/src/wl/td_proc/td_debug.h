#ifndef _td_debug_h_
#define _td_debug_h_

#define TBIT(x) (1 << (x))

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (unsigned)(sizeof(x)/sizeof((x)[0]))
#endif

#ifndef IFNSIZE
#define IFNSIZE 16
#endif

typedef enum td_dbg_bit {
    TD_DEBUG_BIT_ERROR =    0,
    TD_DEBUG_BIT_INFO =     1,
    TD_DEBUG_BIT_DEBUG =    2,
    TD_DEBUG_BIT_ASSOC =    3,
    TD_DEBUG_BIT_STEER =    4,
    TD_DEBUG_BIT_ARP =      5,
    TD_DEBUG_BIT_ICMP =     6,
    TD_DEBUG_BIT_DHCP =     7,
    
    TD_DEBUG_BIT_MAX =      31  // uint32 max allow 32bit
}td_dbg_bit_e;

typedef enum td_dbg_type {
    TD_DEBUG_OFF = 0,
    TD_DEBUG_ERROR =    TBIT(TD_DEBUG_BIT_ERROR),
    TD_DEBUG_INFO =     TBIT(TD_DEBUG_BIT_INFO),
    TD_DEBUG_DEBUG =    TBIT(TD_DEBUG_BIT_DEBUG),
    TD_DEBUG_ASSOC =    TBIT(TD_DEBUG_BIT_ASSOC),
    TD_DEBUG_STEER =    TBIT(TD_DEBUG_BIT_STEER),
    TD_DEBUG_ARP =    TBIT(TD_DEBUG_BIT_ARP),
    TD_DEBUG_ICMP =    TBIT(TD_DEBUG_BIT_ICMP),
    TD_DEBUG_DHCP =    TBIT(TD_DEBUG_BIT_DHCP),
}td_dbg_type_e;

#define NULL_MAC "\x00\x00\x00\x00\x00\x00"
#define TD_IS_NULL_MAC(mac) ( 0 == (((unsigned char *)(mac))[0] | ((unsigned char *)(mac))[1] | \
    ((unsigned char *)(mac))[2] | ((unsigned char *)(mac))[3] | ((unsigned char *)(mac))[4] | \
    ((unsigned char *)(mac))[5]) )

#define TDP_ERROR(fmt, args...)  do {       \
    if (g_td_debug & TD_DEBUG_ERROR) {          \
        printk(fmt" [%s(%d)] ERROR\n", ##args, __FUNCTION__, __LINE__);  \
    }                                                    \
} while(0)

#define TDP_INFO(fmt, args...)  do {       \
    if (g_td_debug & TD_DEBUG_INFO) {          \
       printk(fmt" [%s(%d)] INFO\n", ##args, __FUNCTION__, __LINE__);  \
    }                                                    \
} while(0)

#define TDP_DEBUG(fmt, args...)  do {       \
    if (g_td_debug & TD_DEBUG_DEBUG) {          \
        printk(fmt" [%s(%d)] DEBUG\n", ##args, __FUNCTION__, __LINE__);  \
    }                                                    \
} while(0)

#define TDP_ASSOC(fmt, args...)  do {       \
    if (g_td_debug & TD_DEBUG_ASSOC) {          \
        printk(fmt" [%s(%d)] ASSOC\n", ##args, __FUNCTION__, __LINE__);  \
    }                                                    \
} while(0)

#define TDP_STEER(fmt, args...)  do {       \
    if (g_td_debug & TD_DEBUG_STEER) {          \
        printk(fmt" [%s(%d)] STEER\n", ##args, __FUNCTION__, __LINE__);  \
    }                                                    \
} while(0)

extern unsigned int g_td_debug;
extern struct ether_addr g_print_mac;
extern char g_print_dev[IFNSIZE];

void td_skb_debug(void *p, bool is_tx, const char *func, int line);
#endif /* _td_debug_h_ */