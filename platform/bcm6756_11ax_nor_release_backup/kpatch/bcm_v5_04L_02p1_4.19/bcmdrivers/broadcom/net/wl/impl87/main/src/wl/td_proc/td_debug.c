#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <asm/memory.h>
#include <ethernet.h>

#include "td_debug.h"
#include "td_wl_core_symb.h"

typedef struct td_debug_level {
    td_dbg_type_e id;
    char *str;
} td_debug_level_t;

static td_debug_level_t td_debug_levels[] = {
{TD_DEBUG_OFF, "off"},
{TD_DEBUG_ERROR, "error"},
{TD_DEBUG_INFO, "info"},
{TD_DEBUG_DEBUG, "debug"},
{TD_DEBUG_ASSOC, "assoc"},
{TD_DEBUG_STEER, "steer"},
{TD_DEBUG_ARP, "arp"},
{TD_DEBUG_ICMP, "icmp"},
{TD_DEBUG_DHCP, "dhcp"},
};
#ifdef TD_STEER
extern int maps_set_dbg(void *msg, size_t msg_len);
#endif
/* 设置调试等级 */
unsigned int g_td_debug = TD_DEBUG_ERROR | TD_DEBUG_INFO;

/* 指定打印dev */
char g_print_dev[IFNSIZE] = {0};

/* 指定打印MAC, 00:00:00:00:00:00代表所有MAC */
struct ether_addr g_print_mac = {NULL_MAC};

static int td_parse_mac(char *mac_str, struct ether_addr *n)
{
    char *p = mac_str;
    char buf[3] = {0};
    int i = 0;
    int count = 0;
    
    while (*p && count < ETHER_ADDR_LEN) {
        buf[i++] = *p;      
        p++;
        
        if (*p == ':' || i == 2) {
            buf[2] = '\0';
            n->octet[count++] = (unsigned char)simple_strtol(buf, NULL, 16);
            
            memset(buf, 0, sizeof(buf));
            i = 0;
            p++;
        }
    }

    return count;
}

static void td_debug_show(struct seq_file *s)
{
    int i;
    seq_printf(s, "\n avaliable debug levels: \n");

    for (i = 0; i < ARRAYSIZE(td_debug_levels); i++) {
        seq_printf(s, "%#x \t -- %s \n", td_debug_levels[i].id, td_debug_levels[i].str);
    }
    
}

static char *td_debug_parse_string(unsigned int levels, char *buf, int len)
{
    char *p = buf;
    int i;

    if (!buf) {
        TDP_ERROR("null pointer!\n");
        return NULL;
    }

    memset(buf, 0, len);

    for (i = 0; i < ARRAYSIZE(td_debug_levels); i++) {
        if (levels & td_debug_levels[i].id || levels == td_debug_levels[i].id) {
            snprintf(p, len - strlen(td_debug_levels[i].str) - 1, "%s ", 
                td_debug_levels[i].str);
            p += strlen(td_debug_levels[i].str) + 1;
        }
    }

    return buf;
}

int td_debug_read_proc(struct seq_file *s, void *data)
{
    char buf[256] = {0};
    
    if (!s) {
        printk("seq_file is null\n");
        return -EINVAL;
    }

    seq_printf(s, "g_td_debug=%#x (%s)\n", g_td_debug, 
        td_debug_parse_string(g_td_debug, buf, sizeof(buf)));

    td_debug_show(s);

    return 0;
}

int td_debug_write_proc(struct file *file, const char *buffer,
                                                    unsigned long count, void *data)
{
    char tmp[32] = {0};
    int value = 0;

    if (count > sizeof(tmp)) {
        printk("overflow: %lu > %d\n", count, sizeof(tmp));
        return -EINVAL;
    }

    if (buffer && !copy_from_user(tmp, buffer, count)) {
        tmp[count-1] = '\0';
        g_td_debug = simple_strtol(tmp,NULL,0);

        printk("\n set debug=%#x\n", g_td_debug);
    }

    value = (g_td_debug & TD_DEBUG_STEER)? 1: 0;
#ifdef TD_STEER
    TDCORE_FUNC_IF(maps_set_dbg)
        TDCORE_FUNC(maps_set_dbg)(&value, sizeof(value));
#endif
    return count;
}

static void td_debug_filter_show(void)
{
    printk("\ng_print_dev=%s\n", g_print_dev);
    printk("g_print_mac=%pM\n", g_print_mac.octet);
}

static void td_debug_filter_clear(void)
{
    memcpy(&g_print_mac, NULL_MAC, ETHER_ADDR_LEN);
    memset(g_print_dev, 0, sizeof(g_print_dev));
}

static int td_debug_filter_set(char *in)
{
    struct ether_addr addr;
    char *p = in;
    int len;
    int ret = 0;

    do {
        if ('-' == *p && '\0' != *++p) {
            if ('i' == *p ) {
                /* jump over -i and space */
                p += strcspn(p, " ");
                p += strspn(p, " ");
                
                len =  strcspn(p, " ");
                if (len >= sizeof(g_print_dev)) {
                    printk("dev length %d out of range!", len);
                    ret = -1;
                    continue;
                }
                if (0 == strncmp(p, "none", strlen("none"))) {
                    memset(g_print_dev, 0, sizeof(g_print_dev));
                } else {
                    if (strncmp(p, "wl", strlen("wl"))) {
                        printk("bad dev\n");
                        ret = -1;
                        continue;
                    }
                    strncpy(g_print_dev, p, len);
                }
                p += len;
            } else if ('m' == *p) {
                /* jump over -m and space */
                p += strcspn(p, " ");
                p += strspn(p, " ");
                
                if (0 == strncmp(p, "none", strlen("none"))){
                    memcpy(&g_print_mac, NULL_MAC, ETHER_ADDR_LEN);
                } else {
                    memset(&addr, 0, sizeof(addr));
                    if (td_parse_mac(p, &addr) != ETHER_ADDR_LEN) {
                        printk("bad mac\n");
                        ret = -1;
                        continue;
                    }
                    memcpy(&g_print_mac, &addr, ETHER_ADDR_LEN);
                }
                p += strcspn(p, " ");
            } else {
                ret = -1;
                printk("unsupport key %c\n", *p);
            }
        }
    } while (*p++);

    return ret;
}

int td_debug_filter_read_proc(struct seq_file *s, void *data)
{

    if (!s) {
        printk("seq_file is null\n");
        return -EINVAL;
    }

    td_debug_filter_show();

    seq_printf(s, "\n\n avaliable filter: \"-i <dev> -m <mac>\", \"none\" means clear filter\n");

    return 0;
}

int td_debug_filter_write_proc(struct file *file, const char *buffer,
                                                    unsigned long count, void *data)
{
    char tmp[128] = {0};

    if (count > sizeof(tmp)) {
        printk("overflow: %lu > %d\n", count, sizeof(tmp));
        return -EINVAL;
    }

    if (buffer && !copy_from_user(tmp, buffer, count)) {
        tmp[count-1] = '\0';

        if ('\0' == tmp[0] || 0 == strncmp(tmp, "none", strlen("none"))) {
            td_debug_filter_clear();
        } else {
            if (td_debug_filter_set(tmp)) {
                return -EINVAL;
            }
        }

        td_debug_filter_show();
    }

    return count;
}
