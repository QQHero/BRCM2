/*****************************************************************************
 文件名    : kwb_common.h
 命名风格  : 
 文件功能说明  ：定义公共层接口和结构体，wifibase驱动内部使用，不能对外使用。
 修改历史  :
*****************************************************************************/
#ifndef __KWB_COMMON_H__
#define __KWB_COMMON_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include "kwb_ioctl.h"

#ifndef ARRY_NUM
#define ARRY_NUM(a) (sizeof (a) / sizeof (a[0]))
#endif

#ifndef ZEROMAC
#define ZEROMAC "\x00\x00\x00\x00\x00\x00"
#endif

#ifndef TENDA_WDS_OUI
#define TENDA_WDS_OUI "\xc8\x53\x57"
#endif

#ifndef DEV_IS_UP
#define DEV_IS_UP(_dev) (_dev ? (((_dev)->flags & (IFF_RUNNING|IFF_UP)) == (IFF_RUNNING|IFF_UP)) : false)
#endif

#ifndef WIFI_BSSIDX_X
#define WIFI_BSSIDX_X 1000      /* 解决bcm获取osifname设计 */
#endif

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#define WIFIBASE_GET_BUF_LEN(len1, len2)  (((len1) > (len2)) ? (len1) : (len2))

#define NOMAL_BSS     "nomal bss"
#define FRONTHAUL_BSS "fronthaul bss"
#define BACKHAUL_BSS  "backhaul bss"
#define BACKHAUL_STA  "backhaul sta"

typedef int (*wifibase_common_fn)(void *, void *, unsigned int);

typedef struct wb_fn_handle
{
    kwb_cmd_e cmd;
    wifibase_common_fn fn;
    bool need_check_down_up;
} wb_fn_handle_t;

typedef struct wb_fn_ctx
{
    wb_fn_handle_t *handle;
    int num;
}wb_fn_ctx_t;

typedef struct kwb_ifmap_s {
    char *radio_ifname;
    char *bss_ifname;
}kwb_ifmap_t;

typedef struct wb_common_hook
{
    wb_fn_ctx_t user_handle;
    struct mutex user_handle_lock;
    wb_fn_ctx_t kernel_handle;
    spinlock_t kernel_handle_lock;
}wb_common_hook_t;

typedef enum wifi_bssidx
{
    WIFI_BSSIDX_NONE = -1,
    WIFI_BSSIDX_0 = 0,
    WIFI_BSSIDX_1,
    WIFI_BSSIDX_2,
    WIFI_BSSIDX_3,
    WIFI_BSSIDX_4,
    WIFI_BSSIDX_5,
    WIFI_BSSIDX_6,
    WIFI_BSSIDX_7,
    WIFI_BSSIDX_VXD,
    WIFI_BSSIDX_MAX
}wifi_bssidx_e;

typedef enum wifi_rfidx
{
    WIFI_RFIDX_NONE = -1,
    WIFI_RFIDX_0 = 0,
    WIFI_RFIDX_1,
    WIFI_RFIDX_2,
    WIFI_RFIDX_MAX
}wifi_rfidx_e;

typedef struct wifi_idx
{
    wifi_rfidx_e idx;   /* RF index, 1,2,3... */
    wifi_bssidx_e vidx; /* BSS index, -1,0,1,2,3...(-1 means root interface) */
}wifi_idx_t;


/*内联函数写在头文件，提高编译效率。wifibase接口可能用在可能用在进程上下文*/
static inline void *kwb_malloc(int size)
{
    return kzalloc(size, GFP_ATOMIC);
}

static inline void kwb_free(void *ptr)
{
    if (ptr) {
        kfree(ptr);
    }
    return;
}

int kwb_common_register_func(wb_common_hook_t *);
int kwb_common_init(void);
void kwb_common_deinit(void);
int kwb_common_ioctl(const char *mib_name, kwb_cmd_e cmd, void *data, unsigned int data_len);
long kwb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int kwb_sys_ioctl(const char *dev_name, unsigned int cmd, void *data);
struct net_device *kwb_get_dev_by_name(const char *dev_name);

int kwb_ifname_map(unsigned int cmd, const char *in_ifname, char *out_ifname);
#endif
