/*
<:copyright-BRCM:2013:proprietary:standard

   Copyright (c) 2013 Broadcom 
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:> 
*/

/*
 *******************************************************************************
 * File Name  : pktrunner_driver.c
 *
 * Description: This file contains Linux character device driver entry points
 *              for the Runner Blog Driver.
 *******************************************************************************
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/bcm_log.h>
#include <linux/sysrq.h>

#include <linux/blog.h>

#include <rdpa_api.h>
#include "pktrunner_proto.h"
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <fcachehw.h>

/* Character Device information */

#define PKT_RUNNER_VERSION               "0.1"
#define PKT_RUNNER_VER_STR               "v" PKT_RUNNER_VERSION
#define PKT_RUNNER_MODNAME               "Broadcom Runner Blog Driver"

#define PKT_RUNNER_DRV_NAME              "pktrunner"
#define PKT_RUNNER_DRV_MAJOR             309
#define PKT_RUNNER_DRV_DEVICE_NAME       "/dev/" PKT_RUNNER_DRV_NAME

#define PKT_RUNNER_PROCFS_DIR_PATH       PKT_RUNNER_DRV_NAME   /* dir: /procfs/pktrunner    */

/*
 *------------------------------------------------------------------------------
 * Function Name: pktRunner_ioctl
 * Description  : Main entry point to handle user applications IOCTL requests.
 * Returns      : 0 - success or error
 *------------------------------------------------------------------------------
 */
static long pktRunner_ioctl(struct file *filep, unsigned int command, unsigned long arg)
{
    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: pktRunner_open
 * Description  : Called when an user application opens this device.
 * Returns      : 0 - success
 *------------------------------------------------------------------------------
 */
static int pktRunner_open(struct inode *inode, struct file *filp)
{
    return 0;
}

/* Global file ops */
static struct file_operations pktRunner_fops_g =
{
    .unlocked_ioctl  = pktRunner_ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = pktRunner_ioctl,
#endif
    .open   = pktRunner_open,
};

#if defined(CC_PKTRUNNER_PROCFS)
/*
 *  PKT RUNNER Prof File System 
 */

typedef enum {
    ePKTRNR_PROC_FS_TYPE_FIRST,
    ePKTRNR_PROC_FS_TYPE_STATS = ePKTRNR_PROC_FS_TYPE_FIRST,
    ePKTRNR_PROC_FS_TYPE_FLOWS_L3,
    ePKTRNR_PROC_FS_TYPE_FLOWS_L2,
    ePKTRNR_PROC_FS_TYPE_FLOWS_MC,
    ePKTRNR_PROC_FS_TYPE_MAX,
} pktRunnerProcFsTypes_t;

typedef struct
{
    uint64_t accel:2;       /* Accelerator index */
    uint64_t procfs_type:4; /* ProcFS Type */
    uint64_t disp_cnt:18;   /* Last display count during seq_file - gets reset during start */
    uint64_t flw_idx:18;    /* Next flow-index to scan during seq_file - does NOT get reset during start */
    uint64_t flw_cnt:18;    /* Total flow-indexes scanned during seq_file */
    uint64_t done:1;        /* Everything displayed? */
    uint64_t seq_buf_full:1;/* Is current seq buffer full? */
    uint64_t unused1:2;
    uint64_t valid_flows:18;/* Total flows with valid RDPA-Key */
    uint64_t match_flows:18;/* Total matching flows with valid RDPA-Key */
    uint64_t unused2:28;
} pktRunnerProcFsData_t;

pktRunnerProcFsData_t pktRunner_proc_fs_data[PKTRNR_MAX_FHW_ACCEL][ePKTRNR_PROC_FS_TYPE_MAX];
uint32_t disp_cnt_g = 128;

static int pktRunnerDumpVariable(struct seq_file *sf, void *v)
{
    seq_printf(sf, "Max Display = %u \n", disp_cnt_g);
    return 0;
}
static ssize_t pktRunnerVarWrite(struct file *file, const char *buf, size_t count, loff_t *offp)
{
    char *msg = kmalloc(count+1, GFP_KERNEL);
    int32_t val;
    copy_from_user(msg, buf, count);
    msg[count] = '\0';
    if (kstrtoint(msg, 0, &val))
    {
        printk("\n Invalid %s \n",msg);
    }
    kfree(msg);
    printk("\nMax Display <Old:%d> <New:%d>\n", disp_cnt_g, val);
    disp_cnt_g = val;
    return count;
}
static int pktRunnerVarOpen(struct inode *inode, struct file *file)
{
    return single_open(file, pktRunnerDumpVariable, NULL);
}

static struct file_operations pktRunnerVarProcfs_proc = {
    .open = pktRunnerVarOpen,
    .write = pktRunnerVarWrite,
    .read = seq_read,
};

#define pktRunner_seq_write(sf, sf_buf, availabe, bytes, fmt, arg...)   \
{                                                                       \
    bytes = sprintf(sf_buf, fmt, ##arg);                                \
    seq_commit(sf, bytes);                                              \
    available -= bytes;                                                 \
    sf_buf += bytes;                                                    \
}

static uint32_t pktRunnerDumpFlows(struct seq_file *sf, pktRunnerProcFsData_t *data_p, e_PKTRNR_FLOW_TYPE flow_type)
{
    uint32_t max_flows;
    uint32_t idx; 
    uint32_t v32;
    uint32_t accel = data_p->accel;
    //char buf[128];
    char *sf_buf;
    uint32_t bytes;
    int available; /* Intentionally not using size_t to do -ve math */

    max_flows = PKTRUNNER_DATA(accel).max_flow_idxs;
    idx = data_p->flw_idx;
    available = seq_get_buf(sf, &sf_buf);
    if (available < 128 || sf_buf == NULL)
    {
        data_p->seq_buf_full = 1;
        return 1;
    }
    available -= 128; /* Leave this much room and keep single print < 128 */

    while (data_p->flw_cnt < max_flows)
    {
        if ( PKTRUNNER_RDPA_KEY(accel, idx) != FHW_TUPLE_INVALID )
        {
            v32 = __pktRunnerFlowTypeGet(&PKTRUNNER_DATA(accel), idx);
            if (v32 == flow_type) /* Type match */
            {
                if (data_p->disp_cnt < disp_cnt_g)
                {
                    v32 = __pktRunnerBuildKey(accel, v32, idx);
                    if (data_p->disp_cnt == 0)
                    {
                        pktRunner_seq_write(sf,sf_buf,available, bytes,"\n[Index : FHW-Key   ] -> [RDPA-Key  ]\n\n");
                        if (available <= 0)
                        {
                            data_p->seq_buf_full = 1;
                            break;
                        }
                    }
                    data_p->flw_idx = idx; /* Record the last index displayed */
                    data_p->flw_idx += 1;
                    data_p->disp_cnt++;
                    if (data_p->flw_idx >= max_flows)
                    {
                        data_p->flw_idx = 0;
                    }
                    pktRunner_seq_write(sf,sf_buf,available, bytes,"[%5u : 0x%08X] -> [0x%08X]\n", idx, v32, PKTRUNNER_RDPA_KEY(accel, idx));
                    if (available <= 0)
                    {
                        data_p->seq_buf_full = 1;
                        break;
                    }
                }
                data_p->match_flows++;
            }
            data_p->valid_flows++; /* Valid flow found */
        }
        data_p->flw_cnt++;
        idx++;
        if (idx >= max_flows)
        {
            idx = 0;
        }
    }


    if (!data_p->seq_buf_full) /* Done printing and still have space for summary */
    {
        if (available < 128)
        {
            data_p->seq_buf_full = 1;
            return 1;
        }
        /* Keep this print below 128 Bytes */
        pktRunner_seq_write(sf,sf_buf,available, bytes,"\nTotal Flows with valid RDPA Key = %d \n"
                       "Total matching flows = %d \n"
                       "Total displayed = %d\n", 
                        data_p->valid_flows, data_p->match_flows, data_p->disp_cnt);
        data_p->done = 1;
    }
    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: pktRunnerProcfsDirGet
 * Description  : Returns the Directory based on type
 *------------------------------------------------------------------------------
 */
static int pktRunnerProcfsDirGet(pktRunnerProcFsTypes_t types, char *sub_dir_p)
{
    switch ( types )
    {
        case ePKTRNR_PROC_FS_TYPE_STATS: 
            strcpy(sub_dir_p, "/stats");
            break;
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L3: 
            strcpy(sub_dir_p, "/flows/L3");
            break;
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L2: 
            strcpy(sub_dir_p, "/flows/L2");
            break;
        case ePKTRNR_PROC_FS_TYPE_FLOWS_MC: 
            strcpy(sub_dir_p, "/flows/mcast");
            break;
        default:
            return 0;
    }
    return 1;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: pktRunnerStatsProcfs
 * Description  : Handler to display pktRunner Stats into the ProcFs
 *------------------------------------------------------------------------------
 */
static void *pktRunner_seq_start(struct seq_file *s, loff_t *pos)
{
    struct file *file = (struct file *)(s->private);
    pktRunnerProcFsData_t *data_p = (pktRunnerProcFsData_t *)PDE_DATA(file_inode(file));

    if (*pos != 0 && data_p->done)
    {
        return NULL; /* This will stop the sequence */
    }
    switch (data_p->procfs_type)
    {
        case ePKTRNR_PROC_FS_TYPE_STATS: 
            break;
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L3: 
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L2: 
        case ePKTRNR_PROC_FS_TYPE_FLOWS_MC: 
            if (*pos == 0)
            {
                data_p->done = 0;
                data_p->flw_cnt = 0;
                data_p->disp_cnt = 0;
                data_p->valid_flows = 0;
                data_p->match_flows = 0;
            }
            data_p->seq_buf_full = 0;
            break;
        default:
            return NULL;
    }

    return data_p;
}

static void *pktRunner_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    struct file *file = (struct file *)(s->private);
    pktRunnerProcFsData_t *data_p = (pktRunnerProcFsData_t *)PDE_DATA(file_inode(file));

    if (data_p->done || data_p->seq_buf_full)
    {
        return NULL; /* This will move to stop->start */
    }

    switch (data_p->procfs_type)
    {
        case ePKTRNR_PROC_FS_TYPE_STATS: 
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L3: 
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L2: 
        case ePKTRNR_PROC_FS_TYPE_FLOWS_MC: 
            break;
        default:
            return NULL;
    }

    return data_p;
}

static void pktRunner_seq_stop(struct seq_file *s, void *v)
{
    return;
}

static e_PKTRNR_FLOW_TYPE pktRunner_proc_type_to_flow_type(pktRunnerProcFsTypes_t procfs_type)
{
    switch (procfs_type)
    {
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L3: 
            return PKTRNR_FLOW_TYPE_L3;
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L2: 
            return PKTRNR_FLOW_TYPE_L2;
        case ePKTRNR_PROC_FS_TYPE_FLOWS_MC:
            return PKTRNR_FLOW_TYPE_MC;
        default:
            break;
    }
    return PKTRNR_FLOW_TYPE_INV;
}
static int pktRunner_seq_show(struct seq_file *s, void *v)
{
    struct file *file = (struct file *)(s->private);
    pktRunnerProcFsData_t *data_p = (pktRunnerProcFsData_t *)PDE_DATA(file_inode(file));

    switch (data_p->procfs_type)
    {
        case ePKTRNR_PROC_FS_TYPE_STATS: 
            pktRunnerGetState(s, data_p->accel);
            data_p->done = 1;
            break;
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L3: 
        case ePKTRNR_PROC_FS_TYPE_FLOWS_L2: 
        case ePKTRNR_PROC_FS_TYPE_FLOWS_MC:
            pktRunnerDumpFlows(s, data_p, pktRunner_proc_type_to_flow_type(data_p->procfs_type));
            /* done is set by DumpFlows */
            break;
        default:
            data_p->done = 1;
            break;
    }
    return 0;
}


static struct seq_operations pktRunner_seq_ops = {
    .start = pktRunner_seq_start,
    .next  = pktRunner_seq_next,
    .stop  = pktRunner_seq_stop,
    .show  = pktRunner_seq_show,
};

static int pktRunnerOpen(struct inode *inode, struct file *file)
{
    struct seq_file *s;
    int ret;

    if ((ret = seq_open(file, &pktRunner_seq_ops)) >= 0)
    {
        s = file->private_data;
        s->private = file;
    }
    return ret;
}

static struct file_operations pktRunnerDataProcfs_proc = {
    .open = pktRunnerOpen,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

static void pktRunner_proc_fs_build_dir(char *buf, int accel, char *sub_dir)
{
    int bytes;
    bytes = sprintf(buf, PKT_RUNNER_PROCFS_DIR_PATH "/accel%d%s", accel, sub_dir);
}
int __init pktRunner_proc_fs_construct(void)
{
    struct proc_dir_entry *entry;
    int accel, types;
    char dir_name[128];
    char sub_dir[128];
    pktRunnerProcFsData_t *data_p;

    /* Create Top Level Proc Dir */
    proc_mkdir( PKT_RUNNER_PROCFS_DIR_PATH, NULL );

    for (accel=0; accel < PKTRNR_MAX_FHW_ACCEL; accel++)
    {
        /* Create dir per accelerator */
        pktRunner_proc_fs_build_dir(dir_name, accel, "\0");
        proc_mkdir( dir_name, NULL );
        /* Create flows dir per accelerator */
        pktRunner_proc_fs_build_dir(dir_name, accel, "/flows");
        proc_mkdir( dir_name, NULL );
        for (types = ePKTRNR_PROC_FS_TYPE_FIRST; types < ePKTRNR_PROC_FS_TYPE_MAX; types++)
        {
            data_p = &pktRunner_proc_fs_data[accel][types];
            data_p->accel = accel;
            data_p->procfs_type = types;

            if ( pktRunnerProcfsDirGet(types, sub_dir ) )
            {
                pktRunner_proc_fs_build_dir(dir_name, accel, sub_dir);
                entry = proc_create_data(dir_name , S_IRUGO, NULL, &pktRunnerDataProcfs_proc, data_p);
                if (!entry)
                {
                   bcm_print( CLRerr "%s Unable to create proc entry %s" CLRnl, __FUNCTION__, dir_name);
                   return -1;
                }
            }
            else
            {
                bcm_print( CLRerr "%s cannot get procfs dir for type %d" CLRnl, __FUNCTION__, types);
            }
        }
    }

    /* ProcFs to read/write the variable */
    proc_mkdir( PKT_RUNNER_PROCFS_DIR_PATH "/var", NULL );
    entry = proc_create_data(PKT_RUNNER_PROCFS_DIR_PATH "/var/max_disp" , S_IRUGO, NULL, &pktRunnerVarProcfs_proc, NULL);
    if (!entry)
    {
       bcm_print( CLRerr "%s Unable to create proc entry /var/max_disp" CLRnl, __FUNCTION__);
       return -1;
    }

    return 0;
}

void __exit pktRunner_proc_fs_destruct(void)
{
    int accel;
    char dir_name[128];
    char sub_dir[128];
    pktRunnerProcFsTypes_t types;

    for (accel=0; accel < PKTRNR_MAX_FHW_ACCEL; accel++)
    {
        /* Remove dir for types */
        for (types = ePKTRNR_PROC_FS_TYPE_FIRST; types < ePKTRNR_PROC_FS_TYPE_MAX; types++)
        {
            if ( pktRunnerProcfsDirGet(types, sub_dir ) )
            {
                pktRunner_proc_fs_build_dir(dir_name, accel, sub_dir);
                remove_proc_entry( dir_name, NULL );
            }
            else
            {
                bcm_print( CLRerr "%s cannot get procfs dir for type %d" CLRnl, __FUNCTION__, types);
            }
        }
        pktRunner_proc_fs_build_dir(dir_name, accel, "\0");
        remove_proc_entry( dir_name, NULL );
    }
    
    remove_proc_entry(PKT_RUNNER_PROCFS_DIR_PATH, NULL);
}
#else
int __init pktRunner_proc_fs_construct(void)
{
    return 0;
}
void __exit pktRunner_proc_fs_destruct(void)
{
}
#endif /* CC_PKTRUNNER_PROCFS */

#if !defined(RDP_SIM) && (defined(BCM63146) || defined(CONFIG_BCM963146) || \
                          defined(BCM4912) || defined(CONFIG_BCM94912) || \
                          defined(BCM6813) || defined(CONFIG_BCM96813) || \
                          defined(BCM6855) || defined(CONFIG_BCM96855))
extern int pktrunner_read_init_cfg(void);
#else
#define pktrunner_read_init_cfg()
#endif

/*
 *------------------------------------------------------------------------------
 * Function Name: pktRunner_construct
 * Description  : Initial function that is called at system startup that
 *                registers this device.
 * Returns      : None.
 *------------------------------------------------------------------------------
 */
int __init pktRunner_construct(void)
{
    int ret;

    bcmLog_setLogLevel(BCM_LOG_ID_PKTRUNNER, BCM_LOG_LEVEL_ERROR);

    ret = runnerProto_construct();
    if(ret != 0)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "Could not runnerProto_construct");

        goto out;
    }

    if(register_chrdev(PKT_RUNNER_DRV_MAJOR, PKT_RUNNER_DRV_NAME, &pktRunner_fops_g))
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PKTRUNNER, "Unable to get major number <%d>", PKT_RUNNER_DRV_MAJOR);

        ret = -1;

        goto out;
    }

    pktRunner_proc_fs_construct();

    pktrunner_read_init_cfg();

    bcm_print(PKT_RUNNER_MODNAME " Char Driver " PKT_RUNNER_VER_STR " Registered <%d>\n", PKT_RUNNER_DRV_MAJOR);

out:
    return ret;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: pktRunner_destruct
 * Description  : Final function that is called when the module is unloaded.
 * Returns      : None.
 *------------------------------------------------------------------------------
 */
void __exit pktRunner_destruct(void)
{
    bcm_print("\n\n");
    bcm_print("*********************** WARNING ********************************\n");
    bcm_print("Removing pktrunner module could lead to inconsistencies\n");
    bcm_print("between host and Runner, leading to data-path related issues.\n");
    bcm_print("use \"fcctl config --hw-accel\" to disable flow-acceleration by runner\n");
    bcm_print("****************************************************************\n\n");

    runnerProto_destruct();

    unregister_chrdev(PKT_RUNNER_DRV_MAJOR, PKT_RUNNER_DRV_NAME);

    pktRunner_proc_fs_destruct();

    BCM_LOG_NOTICE(BCM_LOG_ID_PKTRUNNER, PKT_RUNNER_MODNAME " Char Driver " PKT_RUNNER_VER_STR
                   " Unregistered<%d>", PKT_RUNNER_DRV_MAJOR);
}

module_init(pktRunner_construct);
module_exit(pktRunner_destruct);

//EXPORT_SYMBOL();

MODULE_DESCRIPTION(PKT_RUNNER_MODNAME);
MODULE_VERSION(PKT_RUNNER_VERSION);
MODULE_LICENSE("Proprietary");
