/*****************************************************************************
Copyright (C), 吉祥腾达，保留所有版权
File name ：td_easymesh_dbg.c
Description : easymesh调试
Author ：qinke@tenda.cn
Version ：v1.0
Date ：2020.4.10
*****************************************************************************/

#include <linux/kernel.h> 
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include "td_easymesh_dbg.h"

unsigned int g_td_easymesh_dbg = TD_DBG_LEVEL_RET_ERR | TD_DBG_LEVEL_DANGER;
const struct td_em_fake_data g_td_em_fake_data = {0};//内部调试使用
typedef struct td_em_dbg_key_value
{
    const char *key;
    int offset;
}td_em_dbg_key_value_t;//假数据暂时只处理int类型

#define TD_EM_FAKE_DATA_OFFECT(field) ((int)(long *)&(((struct td_em_fake_data *)0)->field))

const static td_em_dbg_key_value_t td_em_fakedata_indx[] = { {"channel_prefer_rson_code", TD_EM_FAKE_DATA_OFFECT(channel_prefer_rson_code)},
                                                             {"bh_steer_result_code", TD_EM_FAKE_DATA_OFFECT(bh_steer_result_code)},
                                                             {"bh_steer_error_code", TD_EM_FAKE_DATA_OFFECT(bh_steer_error_code)},
                                                             {"chnl_select_rsp_code", TD_EM_FAKE_DATA_OFFECT(chnl_select_rsp_code)},
                                                             {"is_btm_support", TD_EM_FAKE_DATA_OFFECT(is_btm_support)},
                                                             {"btm_status", TD_EM_FAKE_DATA_OFFECT(btm_status)},
                                                             {"ap_ability", TD_EM_FAKE_DATA_OFFECT(ap_ability)},
                                                            };

void td_em_dbg_init(void)
{
    struct td_em_fake_data *fake_data = (struct td_em_fake_data *)&g_td_em_fake_data;
    spin_lock_init(&fake_data->td_em_fakedata_lock);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)

#else
/*****************************************************************************
 函 数 名  : td_rasymesh_dbg_level
 功能描述  : 打印调试等级
 输入参数  :
 输出参数  : struct seq_file *s 
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static void td_rasymesh_dbg_level(struct seq_file *s)
{
    if (!s) {
        TD_EM_DBG_PARAM_ERR("seq_file is null\n");
        return;
    }

    seq_printf(s,"debug level name and mask: sugguest:use 0x1f.\n");
    seq_printf(s,"1. RET ERROR    -------->  [0x01]\n");
    seq_printf(s,"2. PARAM ERROR  -------->  [0x02]\n");
    seq_printf(s,"3. TRACE        -------->  [0x04]\n");
    seq_printf(s,"4. MSG          -------->  [0x08]\n");
    seq_printf(s,"5. WARN         -------->  [0x10]\n");
    seq_printf(s,"6. DANGER       -------->  [0x20]\n");
    seq_printf(s,"7. ALL          -------->  [0xff]\n");

    return;
}

/*****************************************************************************
 函 数 名  : td_emdbg_write_proc
 功能描述  : 设置调试等级
 输入参数  : struct file *file    
             const char *buffer   
             unsigned long count  
             void *data           
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_emdbg_write_proc(struct file *file, const char *buffer,
                                                    unsigned long count, void *data)
{
    char tmp[32] = {0};

    if (count > sizeof(tmp)) {
        TD_EM_DBG_PARAM_ERR("count:%lu\n", count);
        return -EINVAL;
    }

    if (buffer && !copy_from_user(tmp, buffer, count)) {
        tmp[count-1] = '\0';
        g_td_easymesh_dbg = simple_strtol(tmp, NULL, 0);
    }

    return count;
}

/*****************************************************************************
 函 数 名  : td_emdbg_read_proc
 功能描述  : 查看调试等级
 输入参数  : struct seq_file *s  
             void *data          
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_emdbg_read_proc(struct seq_file *s, void *data)
{
    if (!s) {
        TD_EM_DBG_PARAM_ERR("seq_file is null\n");
        return -EINVAL;
    }

    seq_printf(s, "0X%X\n", g_td_easymesh_dbg);

    td_rasymesh_dbg_level(s);

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_value
 功能描述  : 假数据写入(内部调试使用)
 输入参数  : char *tmp
 输出参数  : value
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月27日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static void td_em_get_value(char *tmp, int *value)
{
    if (!tmp || !value) {
        TD_EM_DBG_PARAM_ERR("null ptr!! tmp :%p, value : %p\n", tmp, value);
        return;
    }

    while ('=' != *tmp && '\0' != *tmp) {//找到" = "
        tmp++;
    }

    if ('=' == *tmp) {
        while (('0' > *tmp || '9' < *tmp) && '\0' != *tmp) { //找到数据部分
            tmp++;
        }
        if ('\0' != *tmp) {
            *value = simple_strtol(tmp, NULL, 0);
        }
    } else {
        TD_EM_DBG_WARN("input error\n");
        return;
    }

    return;
}
/*****************************************************************************
 函 数 名  : td_emdbg_write_proc
 功能描述  : 假数据写入(内部调试使用)
 输入参数  : struct file *file    
             const char *buffer   
             unsigned long count  
             void *data           
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月27日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_write_fake_proc(struct file *file, const char *buffer,
                                                    unsigned long count, void *data)
{
    char tmp[1024] = {0};
    struct td_em_fake_data *fake_data = (struct td_em_fake_data *)&g_td_em_fake_data;
    int i, len;

    if (count > sizeof(tmp)) {
        TD_EM_DBG_PARAM_ERR("count:%lu\n", count);
        return -EINVAL;
    }

    memset(tmp, 0, sizeof(tmp));
    TD_EM_SPIN_LOCK(&fake_data->td_em_fakedata_lock);

    if (buffer && !copy_from_user(tmp, buffer, count)) {
        tmp[count-1] = '\0';
        for (i = 0; i < sizeof(td_em_fakedata_indx) / sizeof(td_em_fakedata_indx[0]); i++) {
            len = strlen(td_em_fakedata_indx[i].key);
            if (!memcmp(tmp, td_em_fakedata_indx[i].key, len) && ((' ' == tmp[len]) || ('=' == tmp[len]))) {
                TD_EM_DBG_MSG("%s,%s\n", tmp, td_em_fakedata_indx[i].key);
                td_em_get_value(tmp, (int *)((char *)fake_data + td_em_fakedata_indx[i].offset));
            }
        }
    }

    TD_EM_SPIN_UNLOCK(&fake_data->td_em_fakedata_lock);

    return count;
}


/*****************************************************************************
 函 数 名  : td_em_read_fake_data
 功能描述  : 假数据读取(内部调试使用)
 输入参数  : void *data          
 
 输出参数  : struct seq_file *s  
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月27日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_read_fake_proc(struct seq_file *s, void *data)
{
    int i = 0;
    struct td_em_fake_data *fake_data = (struct td_em_fake_data *)&g_td_em_fake_data;

    if (!s) {
        TD_EM_DBG_PARAM_ERR("seq_file is null\n");
        return -EINVAL;
    }

    TD_EM_SPIN_LOCK(&fake_data->td_em_fakedata_lock);

    for (i = 0; i < sizeof(td_em_fakedata_indx)/sizeof(td_em_fakedata_indx[0]); i++) {
        seq_printf(s, "%s = %d\n", td_em_fakedata_indx[i].key, *((int *)((char * )fake_data + td_em_fakedata_indx[i].offset)));
    }

    TD_EM_SPIN_UNLOCK(&fake_data->td_em_fakedata_lock);
    return 0;
}
#endif