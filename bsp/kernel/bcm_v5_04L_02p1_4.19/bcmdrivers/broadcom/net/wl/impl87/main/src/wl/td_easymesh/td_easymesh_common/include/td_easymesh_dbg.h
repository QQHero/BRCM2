#ifndef _TD_EASYMESH_DBG_H_
#define _TD_EASYMESH_DBG_H_

#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>

extern unsigned int g_td_easymesh_dbg;

struct td_em_fake_data
{
    int channel_prefer_rson_code;
    int bh_steer_result_code;
    int bh_steer_error_code;
    int chnl_select_rsp_code;
    int is_btm_support;
    int btm_status;
    int ap_ability;
    spinlock_t td_em_fakedata_lock;
};
extern const struct td_em_fake_data g_td_em_fake_data;

/* 调试等级类型定义 */
#define TD_DBG_LEVEL_RET_ERR           (0x1)
#define TD_DBG_LEVEL_PARAM_ERR         (0x2)
#define TD_DBG_LEVEL_TRACE             (0x4)
#define TD_DBG_LEVEL_MSG               (0x8)
#define TD_DBG_LEVEL_WARN              (0x10)
#define TD_DBG_LEVEL_DANGER            (0x20)

#define TD_DBG_PRINT_CNT_MAX           (100)

/* 外部函数接口 */
#define TD_EM_DBG_TRACE(fmt, args...) do {        \
    if (g_td_easymesh_dbg & TD_DBG_LEVEL_TRACE) {          \
        printk("[%s][%d]"fmt, __FUNCTION__, __LINE__, ##args);  \
    }                                               \
} while(0)

#define TD_EM_DBG_RET_ERR(fmt, args...) do {       \
    if (g_td_easymesh_dbg & TD_DBG_LEVEL_RET_ERR) {          \
        printk("[%s][%d]"fmt, __FUNCTION__, __LINE__, ##args);  \
    }                                                    \
} while(0)

#define TD_EM_DBG_PARAM_ERR(fmt, args...) do {       \
    if (g_td_easymesh_dbg & TD_DBG_LEVEL_PARAM_ERR) {          \
        printk("[%s][%d]"fmt, __FUNCTION__, __LINE__, ##args);  \
    }                                                    \
} while(0)

#define TD_EM_DBG_WARN(fmt, args...) do {       \
    if (g_td_easymesh_dbg & TD_DBG_LEVEL_WARN) {          \
        printk("[%s][%d]"fmt, __FUNCTION__, __LINE__, ##args);  \
    }                                                    \
} while(0)

#define TD_EM_DBG_MSG(fmt, args...) do {       \
    if (g_td_easymesh_dbg & TD_DBG_LEVEL_MSG) {          \
        printk("[%s][%d]"fmt, __FUNCTION__, __LINE__, ##args);  \
    }                                                    \
} while(0)

#define TD_EM_DBG_DANGER(fmt, args...) do {       \
    if (g_td_easymesh_dbg & TD_DBG_LEVEL_DANGER) {          \
        printk("[%s][%d]"fmt, __FUNCTION__, __LINE__, ##args);  \
    }                                                    \
} while(0)


    
#define TD_EM_SPIN_LOCK(x) \
        do { \
            spin_lock_bh(x); \
        } while (0)
    
#define TD_EM_SPIN_UNLOCK(x) \
        do { \
            spin_unlock_bh(x); \
        } while (0)
    

/* 外部宏定义 */
#define TD_SDK_DBG_IS_CLOSE (0 == g_td_easymesh_dbg)
#define TD_SDK_DBG_IS_MSG_PRINT (g_td_easymesh_dbg & TD_DBG_LEVEL_MSG)
#define TD_SDK_DBG_IS_TRACE_PRINT (g_td_easymesh_dbg & TD_DBG_LEVEL_TRACE)
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#else
extern int td_emdbg_read_proc(struct seq_file *s, void *data);
extern int td_emdbg_write_proc(struct file *file, const char *buffer,
                                                    unsigned long count, void *data);

extern int td_em_write_fake_proc(struct file *file, const char *buffer,
                                                    unsigned long count, void *data);

extern int td_em_read_fake_proc(struct seq_file *s, void *data);

#endif

extern void td_em_dbg_init(void);

#endif
