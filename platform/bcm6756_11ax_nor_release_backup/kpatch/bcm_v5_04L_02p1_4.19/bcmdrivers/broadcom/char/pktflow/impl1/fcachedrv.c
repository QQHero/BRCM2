/*
<:copyright-BRCM:2007:proprietary:standard

   Copyright (c) 2007 Broadcom 
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
 * File Name  : fcachedrv.c
 *
 * Description: This file contains Linux character device driver entry points
 *              for the Flow Cache Driver.
 *******************************************************************************
 */

/*----- Includes -----*/
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/notifier.h>
#include <linux/bcm_log.h>
#include <linux/vmalloc.h>
#include <linux/nbuff.h>
#include <linux/kthread.h>
#include <linux/hashtable.h>
#if IS_ENABLED(CONFIG_BCM_INGQOS)
#include <linux/iqos.h>
#endif
#include "fcache.h"
#include "flwstats.h"
#include "pathstats.h"
#if defined(CONFIG_BCM_FHW)
#include "fcachehw.h"
#endif
#include "fcachedrv_config.h"

/*----- Defines -----*/

/* Override global debug and/or assert compilation for Driver layer */
#define CLRsys              CLRy
#define DBGsys              "FCACHE:DRV"
#if !defined(CC_CONFIG_FCACHE_DRV_DBGLVL)
#undef PKT_DBG_SUPPORTED
#endif
#if defined(PKT_DBG_SUPPORTED)
static int pktDbgLvl = CC_CONFIG_FCACHE_DRV_DBGLVL;
#endif
#if !defined(CC_FCACHE_DRV_ASSERT)
#undef PKT_ASSERT_SUPPORTED
#endif
#if defined(CC_CONFIG_FCACHE_COLOR)
#define PKT_DBG_COLOR_SUPPORTED
#endif
#include "pktDbg.h"
#if defined(CC_CONFIG_FCACHE_DEBUG)    /* Runtime debug level setting */
int fcacheDrvDebug(int lvl) { dbg_config( lvl ); return lvl; }
#endif

#if defined(CONFIG_BCM_FHW)
extern uint32_t fhw_is_hw_cap_enabled(uint32_t cap_mask);
extern int fhw_get_hw_accel_avail(void);
extern uint32_t fhw_set_hw_accel(uint32_t enable);
extern uint32_t fhw_get_hw_accel(void);
extern int  fhw_construct(void);
extern void fhw_destruct(void);
extern int fhw_get_hw_host_mac_mgmt_avail(void);
extern int fhw_stats_print(struct seq_file *m);
#endif

/*----- Exported callbacks from fcache.c ONLY to be invoked by fcachedrv */
extern void fc_timeout_slice( uint32_t slice_ix );
extern void fcache_timeout_uncached( uint32_t slice_ix );
extern void fcache_slice(uint32_t slice_ix);
extern void fcache_interval_sec(uint32_t intv_sec, uint32_t num_slices_per_sec);
extern int  fcache_print_get_next(enumFcacheDrvProcType eProcType, int index);
extern int  fcache_print_by_idx(struct seq_file *s, enumFcacheDrvProcType eProcType, int index);
extern int  fcache_host_dev_mac_print(struct seq_file *m, int index);
extern int fcache_error_stats_print(struct seq_file *m);
extern int fcache_evict_stats_print(struct seq_file *m);
extern int fcache_notify_evt_stats_print(struct seq_file *m);
extern int fcache_query_evt_stats_print(struct seq_file *m);
extern int fcache_slow_path_stats_print(struct seq_file *m);
extern int fcache_flw_bitmap_print(struct seq_file *m);
extern int fcache_mcast_group_info_get_next(int *mcast_grp, int *cm_idx, int *is_client);
extern int fcache_mcast_group_info_print(struct seq_file *s, int mcast_grp, int cm_idx, int is_client);
extern int fcache_max_ent(void);
extern int fcache_set_ucast_max_ent(int maxUcastFlowEntries);
extern int fcache_set_mcast_max_ent(int maxMcastFlowEntries);
extern int fcache_set_mcast_max_groups(int mcast_max_groups);
extern int fcache_set_mcast_max_clients_per_group(int mcast_max_clients_per_group);
extern int fcache_set_mcast_max_clients(int mcast_max_clients);

extern int fcache_npe_max_ent(void);
extern int fcache_set_npe_max_ent(int npe_max_entries);
extern int fcache_npe_print_by_idx(struct seq_file *s, int index);
extern int fcache_npe_print_get_next(int index);

extern int fcache_mcast_print_by_idx(struct seq_file *s, int index);
extern int fcache_mcast_print_get_next(int *index);

extern int  fcache_fdb_print(struct seq_file *s, int index);
extern int  fcache_fdb_max_ent(void);
extern int  fcache_set_fdb_max_ent(int maxFdbEntries);

extern int  fcache_tcpack_thresh(void);
extern int  fcache_set_tcpack_thresh(int tcpack_max_cnt);

extern int fcache_flush_flow(int flowid);
extern void fcache_dump_flow_info(uint32_t flowid);
extern void fcache_dump_flow_blog_info(uint32_t flowid);
extern void fcache_dump_mcast_bitmap_info(int num, int startIdx, int InUseOnly);
extern int  fcache_reset_stats(void);
extern int  fcache_construct(void);
extern void fcache_destruct(void);
extern int  flwStatsInit(void);
extern int fc_flush_params( blog_notify_api_t notify_api, BlogFlushParams_t * params_p, 
        blog_notify_async_cb_fn_t notify_cb_fn, void *notify_cb_data_p);

extern int flwStatsGetQueryNum(void);
extern int flwStatsDumpToStruct(FlwStatsDumpInfo_t *flwStDumpInfo_p, FlwStatsDumpEntryInfo_t *flwStDumpEntry_p);
extern int flwStatsCreateQuery(FlwStatsQueryInfo_t *newQuery);
extern int flwStatsGetQuery(FlwStatsQueryInfo_t *newQuery);
extern int flwStatsDeleteQuery(FlwStatsQueryInfo_t *newQuery);
extern int flwStatsClearQuery(FlwStatsQueryInfo_t *newQuery);
extern int flwStatsGetPollParams(FlwStatsPollParams_t *pollParams);
extern int flwStatsSetPollParams(FlwStatsPollParams_t *pollParams);

extern int path_drv_construct(int procfs);
extern int path_drv_remove_proc_entries(void);
extern void pathstat_slice(uint32_t slice_ix);
extern void pathstat_calc_slice_step(uint32_t slice_period_jiffies);

#if defined(CONFIG_BCM_OVS)
extern int fcache_options_construct(void *log);
extern int fcache_options_destruct(void);
#endif

extern uint32_t iqos_enable_g;
extern uint32_t iqos_cpu_cong_g;

/*----- Forward declarations -----*/
static long fcacheDrvIoctl(struct file *filep, unsigned int command, 
                           unsigned long arg);

static int  fcacheDrvOpen(struct inode *inode, struct file *filp);


static int  fcacheDrvNetDevNotifier(struct notifier_block *this,
                                    unsigned long event, void *ptr);


/*----- Globals -----*/
typedef struct {
    uint32_t slice_ix;          /* slice number processed */
    uint32_t num_slices;        /* total number of slice timers */
    uint32_t slice_period_msec; /* timer period of each slice timer in msec */
    uint16_t num_slice_ent;     /* number of entries per slice */
    int16_t slice_drift_jiffies; /* Every refresh cycle causing +/- drift (in jiffies) */

    struct timer_list timer;

    struct file_operations fops;
    struct notifier_block netdev_notifier;

    int     interval_msec;      /* Refresh cycle total interval in msec */
    int     proc_fs_created;    /* Singleton proc file system initialization */
    int     index;              /* Index */
    int     fdb_index;          /* Index into fdblist */
    uint32_t max_flow_ent;
    int      fc_evt_thread_running; 
    uint32_t notify_evt_count;   /* count of notify events received/processed */
    uint32_t notify_evt_list_extends;   /* #of times notify_evt_extend called */
    uint32_t notify_evt_list_extend_fail;  /* number of times notify_evt_extend failed */
    uint32_t notify_evt_list_extend_total; /* total #of notify_evt_extend entry allocated */
    wq_info_t fc_thread;        /* Timer Thread data */
    wq_info_t fc_evt_thread;    /* Event Thread data */
} ____cacheline_aligned FcacheDrv_t;


static FcacheDrv_t fcacheDrv_g = {
    .fops = {
        .unlocked_ioctl = fcacheDrvIoctl,
#if defined(CONFIG_COMPAT)
        .compat_ioctl = fcacheDrvIoctl,
#endif
        .open           = fcacheDrvOpen
    },

    .netdev_notifier = {
        .notifier_call  = fcacheDrvNetDevNotifier,
    },

    .interval_msec      = FCACHE_REFRESH_MSEC,
    .proc_fs_created    = 0,
    .index              = 0,
    .fdb_index          = FDB_IX_INVALID,
    .max_flow_ent       = 0,
    .fc_evt_thread_running = 0, 
    .notify_evt_count = 0,
    .notify_evt_list_extends = 0,
    .notify_evt_list_extend_fail = 0,
    .notify_evt_list_extend_total = 0,
};


#undef FCACHE_DECL
#define FCACHE_DECL(x) #x,

const char * fcacheDrvIoctlName[] =
{
    FCACHE_DECL(FCACHE_IOCTL_STATUS)
    FCACHE_DECL(FCACHE_IOCTL_ENABLE)
    FCACHE_DECL(FCACHE_IOCTL_UNUSED)
    FCACHE_DECL(FCACHE_IOCTL_DISABLE)
    FCACHE_DECL(FCACHE_IOCTL_FLUSH)
    FCACHE_DECL(FCACHE_IOCTL_DEFER)
    FCACHE_DECL(FCACHE_IOCTL_MCAST)
    FCACHE_DECL(FCACHE_IOCTL_IPV6)
    FCACHE_DECL(FCACHE_IOCTL_RESET_STATS)
    FCACHE_DECL(FCACHE_IOCTL_MONITOR)
    FCACHE_DECL(FCACHE_IOCTL_TIMER)
    FCACHE_DECL(FCACHE_IOCTL_CREATE_FLWSTATS)
    FCACHE_DECL(FCACHE_IOCTL_GET_FLWSTATS)
    FCACHE_DECL(FCACHE_IOCTL_DELETE_FLWSTATS)
    FCACHE_DECL(FCACHE_IOCTL_CLEAR_FLWSTATS)
    FCACHE_DECL(FCACHE_IOCTL_GET_FLWSTATS_NUM)
    FCACHE_DECL(FCACHE_IOCTL_DUMP_FLWSTATS)
    FCACHE_DECL(FCACHE_IOCTL_GET_FLOWSTATS_POLL_PARAMS)
    FCACHE_DECL(FCACHE_IOCTL_SET_FLOWSTATS_POLL_PARAMS)
    FCACHE_DECL(FCACHE_IOCTL_GRE)
    FCACHE_DECL(FCACHE_IOCTL_L2TP)
    FCACHE_DECL(FCACHE_IOCTL_DEBUG)
    FCACHE_DECL(FCACHE_IOCTL_ACCEL_MODE)
    FCACHE_DECL(FCACHE_IOCTL_DUMP_DEBUG_INFO)
    FCACHE_DECL(FCACHE_IOCTL_TCP_ACK_MFLOWS)
    FCACHE_DECL(FCACHE_IOCTL_TOS_MFLOWS)
    FCACHE_DECL(FCACHE_IOCTL_SET_HW_ACCEL)
    FCACHE_DECL(FCACHE_IOCTL_LOW_PKT_RATE)
    FCACHE_DECL(FCACHE_IOCTL_SET_NOTIFY_PROC_MODE)
    FCACHE_DECL(FCACHE_IOCTL_SW_DEFER)
    FCACHE_DECL(FCACHE_IOCTL_4O6_FRAG)
    FCACHE_DECL(FCACHE_IOCTL_INVALID)
};

#define FCACHE_DRV_IOCTL_NAME(cmd) (fcacheDrvIoctlName[(cmd) - FCACHE_IOCTL_DUMMY - 1])

/* Validate the configured parameters */
#if ((FCACHE_CONFIG_MAX_UCAST_FLOW_ENTRIES < FCACHE_MIN_UCAST_FLOW_ENTRIES) || \
     (FCACHE_CONFIG_MAX_UCAST_FLOW_ENTRIES > FCACHE_MAX_UCAST_FLOW_ENTRIES))
#error "Invalid number of Flow Cache Ucast flow entries"
#endif

#if ((FCACHE_CONFIG_MAX_MCAST_GROUPS < FCACHE_MIN_MCAST_GROUPS) || \
     (FCACHE_CONFIG_MAX_MCAST_GROUPS > FCACHE_MAX_MCAST_GROUPS))
#error "Invalid number of Flow Cache Mcast Groups"
#endif

#if ((FCACHE_CONFIG_MAX_MCAST_CLIENTS_PER_GROUP < FCACHE_MIN_MCAST_CLIENTS_PER_GROUP) || \
     (FCACHE_CONFIG_MAX_MCAST_CLIENTS_PER_GROUP > FCACHE_MAX_MCAST_CLIENTS_PER_GROUP))
#error "Invalid number of Flow Cache Clients per Mcast Group"
#endif

/* Assumptions: 
 * (max_mcast_clients >= max_mcast_groups) &&
 * (max_mcast_clients >= max_mcast_clients_per_group) */
#if ((FCACHE_CONFIG_MAX_MCAST_CLIENTS < FCACHE_MIN_MCAST_CLIENTS) || \
     (FCACHE_CONFIG_MAX_MCAST_CLIENTS > FCACHE_MAX_MCAST_CLIENTS) || \
     (FCACHE_CONFIG_MAX_MCAST_CLIENTS < FCACHE_CONFIG_MAX_MCAST_GROUPS) || \
     (FCACHE_CONFIG_MAX_MCAST_CLIENTS < FCACHE_CONFIG_MAX_MCAST_CLIENTS_PER_GROUP))
#error "Invalid number of Flow Cache Clients system wide"
#endif

#if ((FCACHE_CONFIG_MAX_MCAST_FLOW_ENTRIES < FCACHE_MIN_MCAST_FLOW_ENTRIES) || \
     (FCACHE_CONFIG_MAX_MCAST_FLOW_ENTRIES > FCACHE_MAX_MCAST_FLOW_ENTRIES))
#error "Invalid number of Flow Cache mcast flow entries"
#endif

#if ((FCACHE_CONFIG_MAX_FDB_ENTRIES < FCACHE_MIN_FDB_ENTRIES) || \
     (FCACHE_CONFIG_MAX_FDB_ENTRIES > FCACHE_MAX_FDB_ENTRIES))
#error "Invalid number of Flow Cache FDB entries"
#endif

#if ((FCACHE_CONFIG_MAX_HOST_DEV_ENTRIES < FCACHE_MIN_HOST_DEV_ENTRIES) || \
     (FCACHE_CONFIG_MAX_HOST_DEV_ENTRIES > FCACHE_MAX_HOST_DEV_ENTRIES))
#error "Invalid number of Flow Cache Host Dev entries"
#endif

#if ((FCACHE_CONFIG_MAX_HOST_MAC_ENTRIES < FCACHE_MIN_HOST_MAC_ENTRIES) || \
     (FCACHE_CONFIG_MAX_HOST_MAC_ENTRIES > FCACHE_MAX_HOST_MAC_ENTRIES))
#error "Invalid number of Flow Cache Host MAC entries"
#endif


/*
 *------------------------------------------------------------------------------
 * Function Name: fc_mcast_get_first_client_id
 * Description  : Returns the first mcast client id.
 *              : The first few client ids are reserved for non-WLAN clients
 *              : in case of DSL RDP (mainly for Ethernet clients).
 *------------------------------------------------------------------------------
 */
int fc_mcast_get_first_client_id(BlogPhy_t phy_type)
{
    int first_id = BLOG_MCAST_FIRST_CLIENT_ID;
#if (defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM94908))
    if (!((phy_type == BLOG_ENETPHY) || (phy_type == BLOG_XTMPHY)))
        first_id += BLOG_MCAST_ENET_CLIENT_RESERVED_IDS;
#endif

    return first_id;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_update_slice_drift
 * Description  : Update the slice_drift based on the current interval & slices
 *------------------------------------------------------------------------------
 */
static void fcachedrv_update_slice_drift(void)
{
    int drift_msec;
    drift_msec = fcacheDrv_g.interval_msec - 
                    (fcacheDrv_g.slice_period_msec * fcacheDrv_g.num_slices);

    if (drift_msec >= 0) /* +ive drift */
    {
        fcacheDrv_g.slice_drift_jiffies = msecs_to_jiffies(drift_msec); /* +ve drift */
    }
    else
    {
        fcacheDrv_g.slice_drift_jiffies = -msecs_to_jiffies(-drift_msec); /* -ive drift */
    }
    return;
}
/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_get_slice_period
 * Description  : Calculate the slice period given the interval & num_slices
 *------------------------------------------------------------------------------
 */
static int fcachedrv_get_slice_period(const uint32_t intvl_msec, const uint32_t num_slices, uint32_t *period_msec_p)
{
    uint32_t granularity_msec = jiffies_to_msecs(1);
    uint32_t start_msec = FCACHE_SLICE_PERIOD_MIN_MSEC;
    uint32_t period_msec = 0;
    uint32_t found = 0;
    uint32_t drift_msec;
    uint32_t last_drift_msec = ~0;

    /* Make sure start period is not less than granularity */
    if (start_msec < granularity_msec)
    {
        start_msec = granularity_msec;
    }

    /* Find the lowest possible slice period that can fit the num_slices */
    for (period_msec = start_msec; period_msec < intvl_msec; period_msec += granularity_msec)
    {   
        /* Stay below total interval and have some waste/drift if we can fit */     
        if (intvl_msec >= (period_msec*num_slices))
        {
            drift_msec = intvl_msec - (period_msec*num_slices);
        }
        else
        {
            drift_msec = (period_msec*num_slices) - intvl_msec;
            /* Don't overshoot more than one period - can't accomodate in drift adjustment */
            if (drift_msec > period_msec)
            {
                break;
            }
        }
        /* Also try to keep minimum drift for better distribution */
        if (last_drift_msec > drift_msec )
        {
            found = 1;
            last_drift_msec = drift_msec;
            *period_msec_p = period_msec;
        }
    }
    return !found;
}
/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_get_slice_data
 * Description  : Called at boot time to get slice data (period, slices, entries)
 *------------------------------------------------------------------------------
 */
static int __init fcachedrv_get_slice_data(const uint32_t max_ent, const uint32_t intvl_msec,
                                    uint32_t *slice_period_msec_p, uint32_t *num_slices_p, uint32_t *num_slice_ent_p)
{
    uint32_t granularity_msec = jiffies_to_msecs(1);
    uint32_t start_msec = FCACHE_SLICE_PERIOD_MIN_MSEC;
    uint32_t num_ent = 0;
    uint32_t num_slices = 0;
    uint32_t period_msec = 0;
    uint32_t found = 0;

    /* Make sure start period is not less than granularity */
    if (start_msec < granularity_msec)
    {
        start_msec = granularity_msec;
    }

    /* Find the lowest possible slice period that can keep num entries between min/max */
    for (period_msec = start_msec; period_msec < intvl_msec && !found; period_msec += granularity_msec)
    {
        /* Get the max possible slices */
        num_slices = intvl_msec / period_msec;
        /* Adjust slices if some interval is left over */
        if (intvl_msec % period_msec)
        {
            num_slices++;
        }
        /* Get the max possible entries per slice */
        num_ent = max_ent/num_slices;
        /* distribute the left over entries over max possible slices */
        if (max_ent % num_slices)
        {
            num_ent++;
        }
        /* Make sure we stay in range */
        if (num_ent >= FCACHE_SLICE_MIN_ENT && num_ent <= FCACHE_SLICE_MAX_ENT)
        {
            found = 1;
            break;
        }
    }

    if (found)
    {
        *slice_period_msec_p = period_msec;
        *num_slices_p = num_slices;
        *num_slice_ent_p = num_ent;
        print("max_ent = %u intvl_msec = %u num_slices = %u num_ent = %u period_msec = %u \n", 
              max_ent, intvl_msec, num_slices, num_ent, period_msec);
    }
    else
    {
        print("Invalid : max_ent = %u intvl_msec = %u \n", max_ent, intvl_msec);
        return FCACHE_ERROR;
    }
    return FCACHE_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheIntervalMsec
 * Description  : Exported fcache API to reset the timeout interval
 *------------------------------------------------------------------------------
 */
int fcacheIntervalMsec(int interval_msec)
{
    unsigned long newtime;
    uint32_t period_msec;

    if ( interval_msec < FCACHE_REFRESH_MIN_MSEC || interval_msec > FCACHE_REFRESH_MAX_MSEC)
    {
       fc_error( "invalid timer value %d ; Range <%u:%u>", interval_msec, 
                 FCACHE_REFRESH_MIN_MSEC, FCACHE_REFRESH_MAX_MSEC);
       return FCACHE_ERROR;
    }

    /* Number of slices and entries per slice remain constant as set at boot-time */
    if ( fcachedrv_get_slice_period(interval_msec, fcacheDrv_g.num_slices, &period_msec) )
    {
        fc_error( "invalid timer value %d to fit num_slices %u ", fcacheDrv_g.num_slices);
        return FCACHE_ERROR;
    }
    /* Update the interval, slice period and notify fcache */
    fcacheDrv_g.interval_msec = interval_msec;
    fcacheDrv_g.slice_period_msec = period_msec;
    fcache_interval_sec( interval_msec / 1000, 1000/period_msec);
    /* Update the new drift value */
    fcachedrv_update_slice_drift();


    pathstat_calc_slice_step( msecs_to_jiffies(fcacheDrv_g.slice_period_msec) );

    /* restart the timer */
    newtime = jiffies + msecs_to_jiffies(fcacheDrv_g.slice_period_msec);
    mod_timer( &fcacheDrv_g.timer, newtime );        /* kick start timer */
    return FCACHE_SUCCESS;
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_callback
 * Description  : Callback function for fcachedrv events.
 *------------------------------------------------------------------------------
 */
static void fcachedrv_callback(void *data)
{
    BLOG_WAKEUP_WORKER_THREAD_NO_PREEMPT((wq_info_t *)data, BLOG_WORK_AVAIL);
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_flush_params
 * Description  : notify the event and wait for the event to complete
 *------------------------------------------------------------------------------
 */
static int fcachedrv_flush_params(BlogFlushParams_t *blogFlushParams_p)
{
    int count = 0;

    if (blog_preemptible_task() && fcacheDrv_g.fc_evt_thread_running && 
            blog_notify_proc_mode_g == BLOG_NOTIFY_PROC_MODE_HYBRID)
    {
        wq_info_t   fcachedrv_thread; /* IOCTL calling thread */

        fcachedrv_thread.work_avail = 0;
        fcachedrv_thread.wakeup_done = false;
        spin_lock_init(&fcachedrv_thread.wakeup_lock);
        init_waitqueue_head(&fcachedrv_thread.wqh);

        blog_lock();
        count = fc_flush_params(BLOG_NOTIFY_API_ASYNC, blogFlushParams_p, fcachedrv_callback, 
                &fcachedrv_thread);
        blog_unlock();

        dbgl_print(DBG_CTLFL, "Waiting in blogFlushParams_p<%px>...\n", 
                blogFlushParams_p);
        do 
        {
            wait_event_interruptible(fcachedrv_thread.wqh, 
                fcachedrv_thread.work_avail);
        } while (!(fcachedrv_thread.work_avail & BLOG_WORK_AVAIL));

        /* here spinlock ensures no race condition on wait_event, 
         * in between setting work_avail and wakeup
         */
        spin_lock_bh(&fcachedrv_thread.wakeup_lock);
        if(fcachedrv_thread.wakeup_done == false)
            WARN(1, "fcachedrv_flush_params improrper wakeup\n");
        spin_unlock_bh(&fcachedrv_thread.wakeup_lock);

        dbgl_print(DBG_CTLFL, "Done. blogFlushParams ");
    } 
    else
    {
        blog_lock();
        count = fc_flush_params(BLOG_NOTIFY_API_SYNC, blogFlushParams_p, NULL, NULL);
        blog_unlock();
    }
    return count;
}


#define BLOG_LOCK_REQUIRED(cmd) \
((cmd != FCACHE_IOCTL_GET_FLWSTATS_NUM) && \
 (cmd != FCACHE_IOCTL_DUMP_FLWSTATS) && \
 (cmd != FCACHE_IOCTL_CREATE_FLWSTATS) && \
 (cmd != FCACHE_IOCTL_GET_FLWSTATS) && \
 (cmd != FCACHE_IOCTL_DELETE_FLWSTATS) && \
 (cmd != FCACHE_IOCTL_CLEAR_FLWSTATS))

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvIoctl
 * Description  : Main entry point to handle user applications IOCTL requests
 *                Flow Cache Utility.
 * Returns      : 0 - success or error
 *------------------------------------------------------------------------------
 */
static long fcacheDrvIoctl(struct file *filep, unsigned int command, 
                           unsigned long arg)
{
    FcacheIoctl_t cmd;
    int ret = FCACHE_SUCCESS;

    if ( command > FCACHE_IOCTL_INVALID )
        cmd = FCACHE_IOCTL_INVALID;
    else
        cmd = (FcacheIoctl_t)command;

    dbgl_print( DBG_EXTIF, "cmd<%d> %s arg<%lu>",
                command, FCACHE_DRV_IOCTL_NAME(cmd), arg );

    /* protect the fc linked lists by disabling all interrupts */
    switch ( cmd )
    {
        case FCACHE_IOCTL_STATUS :
        {
            FcStatusInfo_t fcStatusInfo;

            /* Copy statusInfo structure from user space to kernel space */
            copy_from_user((void*) &fcStatusInfo, (void*) arg, sizeof(FcStatusInfo_t));

            fcStatusInfo.interval = fcacheDrv_g.interval_msec;

            /* Get statusInfo from fc */
            blog_lock();
            fc_status_ioctl(&fcStatusInfo);
            blog_unlock();

            /* Copy structure back to user space */
            copy_to_user((void*) arg, (void*) &fcStatusInfo, sizeof(FcStatusInfo_t));    
      
            break;
        }

        case FCACHE_IOCTL_ENABLE :
        {
            blog_lock();
            fc_bind_blog( 1 );
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_DISABLE :
        {
            blog_lock();
            fc_bind_blog( 0 );
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_FLUSH:
        {
            FcFlushParams_t fcFlushParams = {};
            BlogFlushParams_t blogFlushParams = {};
            int count;

            /* Copy flushParams structure from user space to kernel space */
            copy_from_user((void*) &fcFlushParams, (void*) arg, sizeof(fcFlushParams));

            /* Copy from fcFlush to blogFlush - no memcpy */
            blogFlushParams.flush_all = (fcFlushParams.flags & FCACHE_FLUSH_ALL) ? 1 : 0;
            blogFlushParams.flush_flow = (fcFlushParams.flags & FCACHE_FLUSH_FLOW) ? 1 : 0;
            blogFlushParams.flush_dev = (fcFlushParams.flags & FCACHE_FLUSH_DEV) ? 1 : 0;
            blogFlushParams.flush_dstmac = (fcFlushParams.flags & FCACHE_FLUSH_DSTMAC) ? 1 : 0;
            blogFlushParams.flush_srcmac = (fcFlushParams.flags & FCACHE_FLUSH_SRCMAC) ? 1 : 0;
            blogFlushParams.flush_hw = (fcFlushParams.flags & FCACHE_FLUSH_HW) ? 1 : 0;
            memcpy(blogFlushParams.mac, fcFlushParams.mac, sizeof(blogFlushParams.mac));
            blogFlushParams.devid = fcFlushParams.devid;
            blogFlushParams.flowid = fcFlushParams.flowid;

            count = fcachedrv_flush_params(&blogFlushParams);

            dbgl_print( DBG_EXTIF, "Flow Cache flushed %d items\n", count );

            break;
        }

        case FCACHE_IOCTL_DEFER:
        {
            blog_lock();
            ret = fcacheDefer( (uint16_t) arg );
            blog_unlock();
            dbgl_print( DBG_EXTIF,
                 "Packet Hw flow activates after %d hits in Sw Fcache\n",
                 ret );
            break;
        }

        case FCACHE_IOCTL_SW_DEFER:
        {
            blog_lock();
            ret = fcache_set_sw_defer_count( (uint16_t) arg );
            blog_unlock();
            dbgl_print( DBG_EXTIF,
                 "Packet Sw flow activates after %d hits in Linux\n",
                 ret );
            break;
        }

        case FCACHE_IOCTL_LOW_PKT_RATE:
        {
            blog_lock();
            ret = fcache_set_low_pkt_rate( (int) arg );
            blog_unlock();
            dbgl_print( DBG_EXTIF,
                 "Flow low pkt rate %d hits\n",
                 ret );
            break;
        }
 
        case FCACHE_IOCTL_MCAST:
        {
            blog_lock();
            blog_support_mcast( arg );
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_ACCEL_MODE:
        {
#if defined(CONFIG_BRIDGE_NETFILTER)
            if (arg == BLOG_ACCEL_MODE_L23)
            {
                printk( CLRbold2 "WARNING: L2 flows not supported with CONFIG_BRIDGE_NETFILTER" CLRnl );
                ret = FCACHE_ERROR;
            }
#else
            blog_lock();
            blog_support_accel_mode( arg );

#if defined(CONFIG_BCM_FHW)
            if (arg == BLOG_ACCEL_MODE_L23)
            {
                uint32_t cap_mask = (1<<HW_CAP_L2_UCAST);

                if (!fhw_is_hw_cap_enabled(cap_mask))
                    printk( CLRbold2 "WARNING: No HW acceleration support for L2 flows" CLRnl );
            }
#endif
            blog_unlock();
#endif
            break;
        }

        case FCACHE_IOCTL_SET_HW_ACCEL:
        {
#if defined(CONFIG_BCM_FHW)
            int hw_disable = arg ? 0 : 1;
            blog_lock();
            fhw_set_hw_accel( arg );
            blog_unlock();

            if (hw_disable) /* Disable HW accel */
            {
                BlogFlushParams_t blogFlushParams = {};
                blogFlushParams.flush_hw = 1;
                fcachedrv_flush_params(&blogFlushParams);
            }
            /* Update hw support for active flows if hw_accel is getting re-enabled 
             * During hw_disable, flush_hw process will update the hw_support for
             * for each flow as it deactivates the flow. */
            if (!hw_disable)
            {
                blog_lock();
                fc_update_hw_support();
                blog_unlock();
            }
#else
            printk( CLRbold2 "WARNING: No HW acceleration support builtin" CLRnl );
#endif
            break;
        }

        case FCACHE_IOCTL_IPV6:
        {
            blog_lock();
            blog_support_ipv6( arg );
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_RESET_STATS:
        {
            blog_lock();
            ret = fcache_reset_stats();
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_MONITOR:
        {
            blog_lock();
            ret = fcacheMonitor( (arg) ? 1 : 0 );
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_TIMER:
        {
            blog_lock();
            ret = fcacheIntervalMsec( (int) arg );
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_GET_FLWSTATS_NUM:
        {
            /* get queries entry number */
            ret = flwStatsGetQueryNum();

            break;
        }

        case FCACHE_IOCTL_DUMP_FLWSTATS:
        {
            FlwStatsDumpInfo_t flwStDumpInfo;
            FlwStatsDumpEntryInfo_t *flwStDumpEntry;

            /* Copy dump structure from user space to kernel space */
            copy_from_user((void*) &flwStDumpInfo, (void *) arg, sizeof(FlwStatsDumpInfo_t));

            if(is_compat_task())
            {
                BCM_IOC_PTR_ZERO_EXT(flwStDumpInfo.FlwStDumpEntry);
            }

            flwStDumpEntry = kmalloc((sizeof(FlwStatsDumpEntryInfo_t)*(flwStDumpInfo.num_entries)), GFP_KERNEL);

            if(flwStDumpEntry)
            {
                /* fill dump info structure */
                ret = flwStatsDumpToStruct(&flwStDumpInfo, flwStDumpEntry);

                if(ret == 0)/* Copy structure back to user space */        
                {
                    copy_to_user((void*) arg, (void*) &flwStDumpInfo, sizeof(FlwStatsDumpInfo_t));    
                    copy_to_user((void *)(flwStDumpInfo.FlwStDumpEntry), (void *)flwStDumpEntry, (sizeof(FlwStatsDumpEntryInfo_t)*(flwStDumpInfo.num_entries)));
                }
                kfree(flwStDumpEntry);
            }
            else
            {
                ret = -2;
            }
            break;
        }

        case FCACHE_IOCTL_CREATE_FLWSTATS:
        {
            FlwStatsQueryInfo_t query;

            /* Copy query structure from user space to kernel space */
            copy_from_user((void*) &query, (void*) arg,
                sizeof(FlwStatsQueryInfo_t));

            /* Create query */
            ret = flwStatsCreateQuery(&query);

            /* Copy structure back to user space */
            copy_to_user((void*) arg, (void*) &query,
                sizeof(FlwStatsQueryInfo_t));    
            break;
        }

        case FCACHE_IOCTL_GET_FLWSTATS:
        {
            FlwStatsQueryInfo_t queryinfo;

            /* Copy query structure from user space to kernel space */
            copy_from_user((void*) &queryinfo, (void*) arg,
                sizeof(FlwStatsQueryInfo_t));

            /* Run query */
            ret = flwStatsGetQuery(&queryinfo);

            /* Copy structure back to user space */
            copy_to_user((void*) arg, (void*) &queryinfo,
                sizeof(FlwStatsQueryInfo_t));    
            break;
        }

        case FCACHE_IOCTL_DELETE_FLWSTATS:
        {
            FlwStatsQueryInfo_t queryinfo;

            /* Copy query structure from user space to kernel space */
            copy_from_user((void*) &queryinfo, (void*) arg,
                sizeof(FlwStatsQueryInfo_t));

            /* Run delete. No need to copy back structure - we only 
               return an error code */
            ret = flwStatsDeleteQuery(&queryinfo);
            break;
        }

        case FCACHE_IOCTL_CLEAR_FLWSTATS:
        {
            FlwStatsQueryInfo_t queryinfo;

            /* Copy query structure from user space to kernel space */
            copy_from_user((void*) &queryinfo, (void*) arg,
                sizeof(FlwStatsQueryInfo_t));

            /* Clear counters. No need to copy back structure - we only 
               return an error code */
            ret = flwStatsClearQuery(&queryinfo);
            break;
        }

        case FCACHE_IOCTL_GET_FLOWSTATS_POLL_PARAMS:
        {
            FlwStatsPollParams_t pollparams = {};

            if (!(ret = flwStatsGetPollParams(&pollparams)))
            {
                copy_to_user((void *)arg, (void *)&pollparams,
                        sizeof(FlwStatsPollParams_t));
            }
            break;
        }

        case FCACHE_IOCTL_SET_FLOWSTATS_POLL_PARAMS:
        {
            FlwStatsPollParams_t pollparams;

            copy_from_user((void *)&pollparams, (void *)arg,
                    sizeof(FlwStatsPollParams_t));

            ret = flwStatsSetPollParams(&pollparams);
            break;
        }

        case FCACHE_IOCTL_GRE:
        {
#if defined(CONFIG_BLOG_GRE)
            blog_lock();
            blog_support_gre( arg );
            blog_unlock();
#else
            printk( CLRbold2 "WARNING: GRE acceleration support disabled" CLRnl );
#endif
            break;
        }

        case FCACHE_IOCTL_L2TP:
        {
            blog_lock();
            blog_support_l2tp( arg );
            blog_unlock();
            break;
        }
    
        case FCACHE_IOCTL_DEBUG :
        {
#if defined(CC_CONFIG_FCACHE_DEBUG)
            int layer = (arg>>8) & 0xFF;
            int level = arg & 0xFF;

            blog_lock();
            switch ( layer )
            {
                case FCACHE_DBG_DRV_LAYER: 
                    ret = fcacheDrvDebug( level );
                    break;

                case FCACHE_DBG_FC_LAYER:  
                    ret = fcacheDebug( level );
                    break;
                    
                case FCACHE_DBG_PATHSTAT_LAYER:  
                    ret = fcachePathstatDebug( level );
                    break;                    
                    
#if defined(CONFIG_BCM_FHW)
                case FCACHE_DBG_FHW_LAYER:  
                    ret = fcacheFhwDebug( level );
                    break;
#endif                    

                default: 
                    ret = FCACHE_ERROR;
            }
            blog_unlock();
#else
            fc_error( "CC_CONFIG_FCACHE_DEBUG not defined");
            ret = FCACHE_ERROR;
#endif
            break;
        }

        case FCACHE_IOCTL_DUMP_DEBUG_INFO:
        {
            FcDumpInfoParams_t dumpParams;

            copy_from_user((void *)&dumpParams, (void *)arg,
                           sizeof(dumpParams));
            switch (dumpParams.op)
            {
                case FCACHE_DUMP_OPT_FLOW_INFO:
                {
                    blog_lock();
                    fcache_dump_flow_info( dumpParams.flow.arg );
                    blog_unlock();
                    break;
                }

                case FCACHE_DUMP_OPT_BLOG_INFO:
                {
                    blog_lock();
                    fcache_dump_flow_blog_info( dumpParams.blog.arg );
                    blog_unlock();
                    break;
                }

                case FCACHE_DUMP_OPT_MCAST_BM_INFO:
                {
                    blog_lock();
                    fcache_dump_mcast_bitmap_info(dumpParams.mcast_bm.num_of_bm,
                                                  dumpParams.mcast_bm.bm_start_idx,
                                                  dumpParams.mcast_bm.only_in_use_bm);
                    blog_unlock();
                    break;
                }

                default:
                {
                    fc_error( "Invalid Dump Opt [%u]", dumpParams.op );
                    ret = FCACHE_ERROR;
                    break;
                }
            }
            break;
        }

        case FCACHE_IOCTL_TCP_ACK_MFLOWS:
        {
            blog_lock();
            blog_support_set_tcp_ack_mflows( arg );
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_SET_NOTIFY_PROC_MODE:
        {
            blog_lock();
            blog_set_notify_proc_mode( arg );
            blog_unlock();
            break;
        }
 
        case FCACHE_IOCTL_4O6_FRAG:
        {
            blog_lock();
            blog_support_4o6_frag( arg );
            blog_unlock();
            break;
        }

        case FCACHE_IOCTL_TOS_MFLOWS:
        {
            blog_lock();
            blog_support_set_tos_mflows( arg );
            blog_unlock();
            break;
        }

        default:
        {
            fc_error( "Invalid cmd[%u]", command );
            ret = FCACHE_ERROR;
        }
    }

    return ret;

} /* fcacheDrvIoctl */

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvOpen
 * Description  : Called when a user application opens this device.
 * Returns      : 0 - success
 *------------------------------------------------------------------------------
 */
int fcacheDrvOpen(struct inode *inode, struct file *filp)
{
    dbgl_print( DBG_EXTIF, "Access Flow Cache Char Device" );
    return FCACHE_SUCCESS;
} /* fcacheDrvOpen */

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvTimer
 * Description  : Periodic slice timer callback. Passes timeout to fcache.o
 * Flow Cache timer interval is divided into a number of slice intervals.
 * The distribution of fcache entries to a slice interval is based on slice
 * mask, which is number of slices minus 1. The number of slices is max 
 * flow cache entries divided by number of entries served in one slice timer.
 *------------------------------------------------------------------------------
 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
void fcacheDrvTimer(unsigned long data)
#else
void fcacheDrvTimer(struct timer_list *timer_p)
#endif
{
    BLOG_WAKEUP_WORKER_THREAD(&fcacheDrv_g.fc_thread, BLOG_WORK_AVAIL);
}

static void fcacheDrvHandleTimerSlice(void)
{
    /* Storing the next expiry will take care of time we spend 
       executing this function. */
    unsigned long next_expiry = jiffies + msecs_to_jiffies(fcacheDrv_g.slice_period_msec);

    blog_lock();
    fcache_slice(fcacheDrv_g.slice_ix);
    blog_unlock();
    /* Idea is to not hold the blog_lock (& bh_disable) for too long;
       This will give other tasks execution window (Voice & packet processing);
       Hopefully - compiler does not optimize unlock/lock back-2-back */
    blog_lock();
    fc_timeout_slice(fcacheDrv_g.slice_ix);
    blog_unlock();

    blog_lock();
    fcache_timeout_uncached(fcacheDrv_g.slice_ix);
    blog_unlock();

    blog_lock();
    pathstat_slice(fcacheDrv_g.slice_ix);
    blog_unlock();

    fcacheDrv_g.slice_ix++;
    if (fcacheDrv_g.slice_ix >= fcacheDrv_g.num_slices)
    {
        fcacheDrv_g.slice_ix = 0;
        next_expiry += fcacheDrv_g.slice_drift_jiffies;
    }
    mod_timer( &fcacheDrv_g.timer, next_expiry );        /* kick start timer */
}
/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvThreadFunc
 * Description  : Flow-cache driver task to handle events.
 *------------------------------------------------------------------------------
 */
static int fcacheDrvThreadFunc(void *thread_data)
{
    while (1) {
        wait_event_interruptible(fcacheDrv_g.fc_thread.wqh,
                                 fcacheDrv_g.fc_thread.work_avail || 
                                 kthread_should_stop());
        if (kthread_should_stop())
        {
            printk(KERN_INFO "kthread_should_stop detected in FC timer thread\n");
            break;
        }
        /* Handle Slice Timeout */
        if (fcacheDrv_g.fc_thread.work_avail & BLOG_WORK_AVAIL)
        {
            fcacheDrv_g.fc_thread.work_avail &= (~BLOG_WORK_AVAIL);
            fcacheDrvHandleTimerSlice();
        }
        /* Handle other work this thread can do -- if any */
    }
    return 0;
}

/* max # of flows allowed to be checked for eviction in one scheduling.
 * BLOG_NOTIFY_EVT_FLOW_QUOTA is a tunable parameter and defines how long
 * the fc_evt_thread can hold the cpu when some event occurs.*/
#define BLOG_NOTIFY_EVT_FLOW_QUOTA      8
#define BLOG_NOTIFY_EVT_QUEUE_SIZE      256

/* extend entries one PAGE_SIZE at a time */
#define BLOG_NOTIFY_EVT_QUEUE_EXTEND_SIZE   (PAGE_SIZE/sizeof(blog_notify_evt_t))

Dll_t           notify_evt_list;           /* List of all the queued events */
Dll_t           notify_evt_free_list;      /* List of free events */
blog_notify_evt_t    evt_table[BLOG_NOTIFY_EVT_QUEUE_SIZE];



extern void fcache_evt_flush_fdb(void *net_fdb_p,
        uint32_t flush_quota, uint32_t *done_p);
extern void fcache_evt_flush_npe(blog_npe_t *npe_p, uint32_t key_orig, 
        uint32_t key_reply, uint32_t flush_quota, uint32_t *done_p);
extern void fcache_evt_flush_arp(uint32_t ip, char *mac, uint32_t start_ix, 
        uint32_t flush_quota, uint32_t *done_p);
extern int fcache_evt_flush_flows(uint32_t start_ix, uint32_t flush_quota, uint32_t *done_p);
extern int fcache_evt_flush_hw(uint32_t start_ix, uint32_t flush_quota, uint32_t *done_p);
extern void fcache_evt_flush_flow_dev(void *dev_p, uint32_t start_ix, 
        uint32_t flush_quota, uint32_t *done_p);
extern int fcache_evt_flush_params(void *parm_p, uint32_t start_ix, 
        uint32_t flush_quota, uint32_t *done_p);
extern void fcache_evt_fetch_netif_stats_excl(void *net_p, unsigned long param1, 
        unsigned long param2, uint32_t start_ix, uint32_t flush_quota, uint32_t *done_p);
extern void fcache_evt_fetch_netif_stats_path(void *net_p, unsigned long param1, 
        unsigned long param2, uint32_t *done_p);
extern void fcache_evt_fetch_netif_stats_sw(void *net_p, unsigned long param1, 
        unsigned long param2, uint32_t *start_ix_p, uint32_t flush_quota, uint32_t *done_p);
extern void fcache_evt_fetch_netif_stats_hw(void *net_p, unsigned long param1, 
        unsigned long param2, uint32_t *start_ix_p, uint32_t flush_quota, uint32_t *done_p);
extern void fcache_evt_clear_netif_stats(void *net_p, unsigned long param1, 
        unsigned long param2, uint32_t start_ix, uint32_t flush_quota);

/*
 *------------------------------------------------------------------------------
 * Function   : fcachedrv_extend_notify_evt_list
 * Description: When the notify_evt_free_list is empty this function may be 
 *              invoked to extend the notify_evt_free_list size.
 * Parameters :
 *   num      : Number of blog_notify_evt_t entries to be allocated.
 * Returns    : Number of blog_notify_evt_t entries allocated in pool.
 *------------------------------------------------------------------------------
 */
uint32_t fcachedrv_extend_notify_evt_list( void )
{
    int id;
    blog_notify_evt_t *evt_list_p;
    blog_notify_evt_t *evt_p; 
    int num = BLOG_NOTIFY_EVT_QUEUE_EXTEND_SIZE;

    if (fcacheDrv_g.notify_evt_list_extend_total < fcacheDrv_g.max_flow_ent)
    {
        evt_list_p = (blog_notify_evt_t *) kmalloc(num * 
                sizeof(blog_notify_evt_t), GFP_ATOMIC);
        if ( evt_list_p == NULL )
        {
            fcacheDrv_g.notify_evt_list_extend_fail++;
            printk( "%s: Fatal Error: Failure to allocate %d blog_notify_evt_t\n", 
                    __FUNCTION__, num );
            return 0;
        }

        /* Initialize each event entry and append to free list */
        for ( id=0; id < num; id++ )
        {
            evt_p = evt_list_p + id;
            dll_append( &notify_evt_free_list, &evt_p->node ); /* Append to free list */
        }

        fcacheDrv_g.notify_evt_list_extends++;
        fcacheDrv_g.notify_evt_list_extend_total += num;
    }
    else
    {
        printk( "%s: Fatal Error: Too many blog_notify_evt_t allocations: %d\n", 
                __FUNCTION__, fcacheDrv_g.notify_evt_list_extend_total );
        return 0;
    }

    return num;
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_notify_evt_enqueue
 * Description  : Prepare and enqueue event work
 *------------------------------------------------------------------------------
 */
int fcachedrv_notify_evt_enqueue(blog_notify_evt_type_t evt_type, void *net_p, 
        unsigned long param1, unsigned long param2,
        blog_notify_async_cb_fn_t notify_cb_fn, void *notify_cb_data_p)
{
    blog_notify_evt_t *evt_p;

    /* if notify_evt_list is full dynamically allocate more entries */
    if (dll_empty(&notify_evt_free_list))
        if (!fcachedrv_extend_notify_evt_list())
        {
            return 0;
        }

    /* prepare the event */
    evt_p = (blog_notify_evt_t*)dll_head_p( &notify_evt_free_list );
    dll_delete( &evt_p->node );
    evt_p->evt_type = evt_type;
    evt_p->net_p = net_p;
    evt_p->param1 = param1;
    evt_p->param2 = param2;
    evt_p->notify_cb_fn = notify_cb_fn;
    evt_p->notify_cb_data_p = notify_cb_data_p;

    dbgl_print( DBG_CTLFL, "Adding evt_p<%px> evt_type<%d> net_p<%px> \n",
            evt_p, evt_p->evt_type, evt_p->net_p); 

    /* enqueue the event and wakeup the fc_evt_thread */
    dll_append( &notify_evt_list, &evt_p->node ); /* Append to evt list */
    BLOG_WAKEUP_WORKER_THREAD(&fcacheDrv_g.fc_evt_thread, BLOG_WORK_AVAIL);

    return 1;
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_evt_fetch_netif_stats
 * Description  : Handle fetch stats for an interface
 *------------------------------------------------------------------------------
 */
static void fcachedrv_evt_fetch_netif_stats(blog_notify_evt_t *evt_p)
{
    uint32_t flow_ix;
    uint32_t done = 0;

    for (flow_ix = FLOW_IX_INVALID+1; !done && 
            flow_ix < fcacheDrv_g.max_flow_ent; flow_ix += BLOG_NOTIFY_EVT_FLOW_QUOTA)
    {
        blog_lock();
        fcache_evt_fetch_netif_stats_excl(evt_p->net_p, evt_p->param1, evt_p->param2,
                flow_ix, BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
        blog_unlock();
        schedule();
    }


    done =0;

    blog_lock();
    fcache_evt_fetch_netif_stats_path(evt_p->net_p, evt_p->param1, evt_p->param2, &done);
    blog_unlock();
    schedule();


    for (flow_ix = FLOW_IX_INVALID+1; !done && 
            flow_ix < fcacheDrv_g.max_flow_ent; )
    {
        blog_lock();
        fcache_evt_fetch_netif_stats_sw(evt_p->net_p, evt_p->param1, 
                evt_p->param2, &flow_ix, BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
        blog_unlock();
        schedule();
    }

    for (flow_ix = FLOW_IX_INVALID+1; !done && 
            flow_ix < fcacheDrv_g.max_flow_ent; )
    {
        blog_lock();
        fcache_evt_fetch_netif_stats_hw(evt_p->net_p, evt_p->param1, 
                evt_p->param2, &flow_ix, BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
        blog_unlock();
        schedule();
    }
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_evt_flush_lists
 * Description  : Handle flush fdb, npe event for which lists are maintained
 *------------------------------------------------------------------------------
 */
static void fcachedrv_evt_flush_lists(blog_notify_evt_t *evt_p)
{
    uint32_t done = 0;

    while (!done) {
        blog_lock();
        switch(evt_p->evt_type) {
        case BLOG_NOTIFY_EVT_FLUSH_FDB:
            fcache_evt_flush_fdb(evt_p->net_p, BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
            break;

        case BLOG_NOTIFY_EVT_FLUSH_NPE:
            fcache_evt_flush_npe(evt_p->net_p, evt_p->param1, evt_p->param2, 
                BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
            break;

        default:
            break;
        }
        blog_unlock();

        schedule();
    }
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_evt_flush_non_lists
 * Description  : Handle the events for which lists are not maintained
 *------------------------------------------------------------------------------
 */
static void fcachedrv_evt_flush_non_lists(blog_notify_evt_t *evt_p)
{
    uint32_t flow_ix;
    uint32_t done = 0;

    for (flow_ix = FLOW_IX_INVALID+1; !done && flow_ix < fcacheDrv_g.max_flow_ent; 
            flow_ix += BLOG_NOTIFY_EVT_FLOW_QUOTA) {
        blog_lock();
        switch(evt_p->evt_type) {
        case BLOG_NOTIFY_EVT_FLUSH:
            fcache_evt_flush_flows(flow_ix, BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
            break;

        case BLOG_NOTIFY_EVT_FLUSH_HW:
            fcache_evt_flush_hw(flow_ix, BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
            break;

        case BLOG_NOTIFY_EVT_FLUSH_DEV:
            fcache_evt_flush_flow_dev(evt_p->net_p, flow_ix, BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
            break;

        case BLOG_NOTIFY_EVT_FLUSH_PARAMS:
            fcache_evt_flush_params(evt_p->net_p, flow_ix, BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
            break;

        case BLOG_NOTIFY_EVT_FLUSH_ARP:
            fcache_evt_flush_arp(evt_p->param1, (char *)evt_p->param2, flow_ix, 
                    BLOG_NOTIFY_EVT_FLOW_QUOTA, &done);
            break;

        case BLOG_NOTIFY_EVT_CLEAR_NETIF_STATS:
            fcache_evt_clear_netif_stats(evt_p->net_p, evt_p->param1, 
                    evt_p->param2, flow_ix, BLOG_NOTIFY_EVT_FLOW_QUOTA);
            break;

        default:
            break;
        }
        blog_unlock();

        schedule();
    }
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_evt_thread_func
 * Description  : Flow-cache driver task to handle notify events.
 *------------------------------------------------------------------------------
 */
static int fcachedrv_evt_thread_func(void *thread_data)
{
    blog_notify_evt_t *evt_p;

    while (1) {
        wait_event_interruptible(fcacheDrv_g.fc_evt_thread.wqh,
                                 fcacheDrv_g.fc_evt_thread.work_avail || 
                                 kthread_should_stop());
        if (kthread_should_stop())
        {
            printk(KERN_INFO "kthread_should_stop detected in fc_evt task\n");
            blog_lock();
            blog_bind_notify_evt_enqueue(NULL);
            blog_unlock();
            fcacheDrv_g.fc_evt_thread_running = 0; 
            break;
        }
    
        fcacheDrv_g.notify_evt_count++;
        blog_lock();
        fcacheDrv_g.fc_evt_thread.work_avail &= (~BLOG_WORK_AVAIL);
        while ( !dll_empty( &notify_evt_list ) )
        {
            evt_p = (blog_notify_evt_t*)dll_head_p( &notify_evt_list );
            dll_delete( &evt_p->node );
            blog_unlock();
            dbgl_print( DBG_CTLFL, "evt_p<%px> evt_type<%d> net_p<%px> processing... \n",
                    evt_p, evt_p->evt_type, evt_p->net_p); 

            switch(evt_p->evt_type) {
            case BLOG_NOTIFY_EVT_FLUSH_FDB:
            case BLOG_NOTIFY_EVT_FLUSH_NPE:
                fcachedrv_evt_flush_lists(evt_p);
                break;

            case BLOG_NOTIFY_EVT_FLUSH:
            case BLOG_NOTIFY_EVT_FLUSH_HW:
            case BLOG_NOTIFY_EVT_FLUSH_DEV:
            case BLOG_NOTIFY_EVT_FLUSH_PARAMS:
            case BLOG_NOTIFY_EVT_FLUSH_ARP:
            case BLOG_NOTIFY_EVT_CLEAR_NETIF_STATS:
                fcachedrv_evt_flush_non_lists(evt_p);
                break;

            case BLOG_NOTIFY_EVT_FETCH_NETIF_STATS:
                fcachedrv_evt_fetch_netif_stats(evt_p);
                break;

            case BLOG_NOTIFY_EVT_NONE:
                break;

            default:
                break;
            }

            /* NOTE:- notify_cb_fn is invoked without taking blog_lock */
            if (evt_p->notify_cb_fn)
                evt_p->notify_cb_fn(evt_p->notify_cb_data_p);

            dbgl_print( DBG_CTLFL, 
                    "%s evt_p<%px> evt_type<%d> net_p<%px> freed \n",
                    evt_p, evt_p->evt_type, evt_p->net_p); 

            blog_lock();
            dll_append( &notify_evt_free_list, &evt_p->node );
        }
        blog_unlock();
    }
    return 0;
}

#if defined(PKT_DBG_SUPPORTED)
const char * strNetdevEvent[] = 
{ 
    "", 
    "NETDEV_UP", 
    "NETDEV_DOWN", 
    "NETDEV_REBOOT", 
    "NETDEV_CHANGE",            //0x04 
    "NETDEV_REGISTER",               
    "NETDEV_UNREG", 
    "NETDEV_CHANGEMTU", 
    "NETDEV_CHANGEADDR",        //0x08 
    "NETDEV_GOING_DOWN", 
    "NETDEV_CHANGENAME", 
    "NETDEV_FEAT_CHANGE", 
    "NETDEV_BONDING_FAILOVER",  //0x0C
    "NETDEV_PRE_UP", 
    "NETDEV_PRE_TYPE_CHANGE", 
    "NETDEV_POST_TYPE_CHANGE", 
    "NETDEV_POST_INIT",         //0x10 
    "NETDEV_UNREGISTER_FINAL", 
    "NETDEV_RELEASE", 
    "NETDEV_NOTIFY_PEERS", 
    "NETDEV_JOIN",              //0x14 
    "NETDEV_CHANGEUPPER",
    "NETDEV_RESEND_IGMP", 
    "NETDEV_PRECHANGEMTU", 
    "NETDEV_CHANGEINFODATA",    //0x18 
    "NETDEV_BONDING_INFO"
};
#endif

typedef struct {

    struct hlist_node hnode; /* hash entry */
    struct list_head lnode; /* list entry */

    union{
        struct net_device *netdev_p;
        uintptr_t key;
    };

} fc_netdev_t;


#define FCACHE_NETDEV_DEFAULT_ENTRIES 128

struct {
    DECLARE_HASHTABLE(hash, 7); /*128 hash buckets */
    struct list_head free_list;
    void *objmem_start;
    void *objmem_end;
    uint16_t total;
    uint16_t inuse;
} fc_ndev_db_g;

void fc_netdev_db_init(int count)
{
    int i;
    fc_netdev_t *objmem, *fc_ndev_p;

    hash_init(fc_ndev_db_g.hash);
    INIT_LIST_HEAD(&fc_ndev_db_g.free_list);
    

    /* Allocate memory for debug objects */
    objmem = kmalloc(count * sizeof(fc_netdev_t), GFP_ATOMIC);

    if(objmem == NULL)
    {
        printk(KERN_ERR "%s:%d : failed to allocated memory for fc_netdev objects\n",
            __func__, __LINE__);
        return;
    }

    fc_ndev_db_g.objmem_start = (void *)objmem;
    fc_ndev_db_g.objmem_end = ((void *)objmem) + (count * sizeof(fc_netdev_t));

    for (i=0; i<count; i++)
    {
        fc_ndev_p = objmem + i;
        INIT_LIST_HEAD(&fc_ndev_p->lnode);
        list_add(&fc_ndev_p->lnode, &fc_ndev_db_g.free_list);
    }

    fc_ndev_db_g.total = count;
    fc_ndev_db_g.inuse = 0;
}


static inline fc_netdev_t *fc_netdev_alloc( void )
{
    fc_netdev_t * fc_ndev_p;
    /*check if list is empty */
    fc_ndev_p = list_first_entry_or_null(&fc_ndev_db_g.free_list, fc_netdev_t , lnode);
    if(fc_ndev_p)
        list_del_init(&fc_ndev_p->lnode);
    else
    {
        fc_ndev_p = kmalloc(sizeof(fc_netdev_t), GFP_ATOMIC);

        if(fc_ndev_p)
        {
            INIT_LIST_HEAD(&fc_ndev_p->lnode);
            fc_ndev_db_g.total++;
        }
        else
            printk(KERN_ERR "%s:%d : mem alloc failure for fc_netdev objects\n",
                    __func__, __LINE__);
    }

    return fc_ndev_p;
}

static inline void fc_netdev_free(fc_netdev_t * fc_ndev_p)
{
    INIT_LIST_HEAD(&fc_ndev_p->lnode);
    list_add(&fc_ndev_p->lnode, &fc_ndev_db_g.free_list);
}

int fc_netdev_register(struct net_device *dev_p)
{
    fc_netdev_t * fc_ndev_p;

    if(dev_p == NULL)
    {
        printk(KERN_ERR "%s:%d : Null netdev pointer\n",
            __func__, __LINE__);
        
        return -1;
    }

    fc_ndev_p = fc_netdev_alloc();
    if(fc_ndev_p == NULL)
    {
        return -1;
    }

    fc_ndev_p->netdev_p = dev_p;

    dev_hold(dev_p);
    /*add to netdev hash */
    INIT_HLIST_NODE(&fc_ndev_p->hnode);
    hash_add(fc_ndev_db_g.hash, &fc_ndev_p->hnode, fc_ndev_p->key);
    fc_ndev_db_g.inuse++;
    return 0;
}


int fc_netdev_unregister(struct net_device *dev_p)
{
    fc_netdev_t * fc_ndev_p;
    struct hlist_node *tmpnode;

    if(dev_p == NULL)
    {
        printk(KERN_ERR "%s:%d : Null Netdev pointer\n",
                __func__, __LINE__);
        return -1;
    }

    /* remove netdev from hash*/
    hash_for_each_possible_safe(fc_ndev_db_g.hash, fc_ndev_p, tmpnode, hnode, (uintptr_t)dev_p){
        if(fc_ndev_p->netdev_p == dev_p)
        {
            hash_del(&fc_ndev_p->hnode);
            fc_netdev_free(fc_ndev_p);
            dev_put(dev_p);
            fc_ndev_db_g.inuse--;
            return 0;
        }
    }

    /* error if obj does not exist */
    printk(KERN_ERR "%s:%d Netdev %px, name=%s not found \n", __func__, __LINE__, dev_p, dev_p->name);
    return -1;
}

/*api to register dummy netdevices not registered with linux */
int fc_netdev_register_dummy(void *dev_p)
{
    int ret;

    blog_lock();
    ret=fc_netdev_register(dev_p);
    blog_unlock();
    return ret;
}
/*api to unregister dummy netdevices not registered with linux */
int fc_netdev_unregister_dummy(void *dev_p)
{
    int ret;

    blog_lock();
    /* perform a synchronous flush */
    blog_notify(DESTROY_NETDEVICE, (void*)dev_p, 0, 0);
    ret=fc_netdev_unregister(dev_p);
    blog_unlock();

    return ret;
}

static inline int __fc_netdev_is_valid(struct net_device *dev_p)
{
    fc_netdev_t * fc_ndev_p;

    hash_for_each_possible(fc_ndev_db_g.hash, fc_ndev_p, hnode, (uintptr_t)dev_p){
        if(fc_ndev_p->netdev_p == dev_p)
        {
            return 1;
        }
    }

    return 0;
}
int fc_netdev_is_valid(struct net_device *dev_p)
{
    return __fc_netdev_is_valid(dev_p);
}


/*check if all netdevices in blog are valid */

int fc_blog_check_netdevs( Blog_t *blog_p )
{
    int i;
    void *dev_p;
    /*check RX device */
    if(blog_p->rx_dev_p)    
        if(!__fc_netdev_is_valid(blog_p->rx_dev_p)) 
            return 0;

    if(blog_p->tx_dev_p) { 
        if(!__fc_netdev_is_valid(blog_p->tx_dev_p)) {
            return 0;
        }
    }
        for (i=0; i<MAX_VIRT_DEV; i++)
        {
            dev_p = DEVP_DETACH_DIR(blog_p->virt_dev_info[i].dev_p);

            if(dev_p)
            {
                if(!__fc_netdev_is_valid(dev_p)) 
                    return 0;
            }
            else
                break; /*here the assumption of first NULL device is end of list*/
        }

        return 1;
}

void fc_netdev_db_exit(void)
{
    struct list_head *cur, *tmp;
    fc_netdev_t *fc_ndev_p;
    struct hlist_node *tmpnode;
    int bucket;

    hash_for_each_safe(fc_ndev_db_g.hash, bucket, tmpnode, fc_ndev_p, hnode)
    {
        hash_del(&fc_ndev_p->hnode);
        dev_put(fc_ndev_p->netdev_p);
        fc_netdev_free(fc_ndev_p);
    }

    list_for_each_safe(cur, tmp, &fc_ndev_db_g.free_list) {

        list_del_init(cur);
        fc_ndev_p = list_entry(cur, fc_netdev_t , lnode);

        /* if allocated outisde of intial pool use kfree */
        if(((uintptr_t)fc_ndev_p < (uintptr_t)fc_ndev_db_g.objmem_start) ||
                    ((uintptr_t)fc_ndev_p > (uintptr_t)fc_ndev_db_g.objmem_end))
            kfree(fc_ndev_p);
    }
    /* free the pool mem*/
    kfree(fc_ndev_db_g.objmem_start);
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvNetDevNotifier
 * Description  : Receive notifications of link state changes and device down
 *                and forward them to fcache.o via blog_notify()
 *------------------------------------------------------------------------------
 */
int fcacheDrvNetDevNotifier(struct notifier_block *this,
                            unsigned long event, void *dev_ptr)
{
    struct net_device *dev_p = NETDEV_NOTIFIER_GET_DEV(dev_ptr);
    char *dev_addr;
    int event_can_be_handled = 1; /* events handled by fcachedrv */
    BlogNotify_t blog_notify_event;

    dev_addr = (char *) blog_request( NETDEV_ADDR, dev_p, 0, 0 );
    dbgl_print( DBG_INTIF, "dev<%s> dev_p<%px> event<%u: %s> <%pM>\n", 
            ((struct net_device *) dev_p)->name, dev_p, 
            event, strNetdevEvent[event],
            ((struct net_device *) dev_p)->dev_addr); 

    switch (event) {
        case NETDEV_UP:
            blog_notify_event = UP_NETDEVICE;
            break;

        case NETDEV_CHANGE:
            blog_lock();
            if ( blog_request( LINK_NOCARRIER, dev_p, 0, 0) )
                blog_notify_event = DN_NETDEVICE;
            else
                blog_notify_event = UP_NETDEVICE;
            blog_unlock();
            break;

        case NETDEV_GOING_DOWN:
            blog_notify_event = DESTROY_NETDEVICE;
            break;

        case NETDEV_DOWN:
            blog_notify_event = DN_NETDEVICE;
            break;

        case NETDEV_CHANGEMTU:
            blog_notify_event = UPDATE_NETDEVICE;
            break;

        case NETDEV_CHANGEADDR:
            blog_notify_event = CHANGE_ADDR;
            break;

        case NETDEV_REGISTER:
            blog_lock();
            fc_netdev_register(dev_p);
            blog_unlock();
            event_can_be_handled = 0;
            break;

        case NETDEV_UNREGISTER:
            blog_lock();
            /* perform a synchronous flush */
            blog_notify(DESTROY_NETDEVICE, (void*)dev_p, (unsigned long)dev_addr, 0);
            fc_netdev_unregister(dev_p);
            blog_unlock();
            event_can_be_handled = 0;
            break;

        default:
            event_can_be_handled = 0;
            break;
    }

    if (event_can_be_handled)
    {
        blog_notify_async_wait(blog_notify_event, (void*)dev_p, 
                (unsigned long) dev_addr, 0);
    }

    return NOTIFY_DONE;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_isRootDevBondMaster
 * Description  : Check if dev is a bonding master device
 *------------------------------------------------------------------------------
 */
int fc_isRootDevBondMaster(void *dev)
{
    return netif_is_bond_master(netdev_path_get_root(dev));
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_isDevInSlavePath
 * Description  : Returns 1 if "dev" is in the slave path of bond_dev.
 *                0 Otherwise.
 *------------------------------------------------------------------------------
 */
int fc_isDevInSlavePath(void *dev, void **bond_dev)
{
    bcmFun_t *bcmFun = bcmFun_get(BCM_FUN_ID_ENET_IS_DEV_IN_SLAVE_PATH);

    if (bcmFun) 
    {
        BCM_BondDevInfo params;
        params.slave_dev = dev;
        params.bond_dev  = (struct net_device **)bond_dev;
        return bcmFun(&params);
    }
    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_isDevBonded
 * Description  : Check if dev is part of bonding group(master/slave)
 *------------------------------------------------------------------------------
 */
int fc_isDevBonded(void *dev)
{
    void *master_dev=NULL;
    return (fc_isRootDevBondMaster(dev) ||
            fc_isDevInSlavePath(dev, &master_dev));
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_isIcmpV6Supported
 * Description  : Check if ICMPV6 traffic should be accelerated
 *------------------------------------------------------------------------------
 */
int fc_isIcmpV6Supported(void)
{
/* Ideally - flow-cache behavior should be same for all platform
 * but :
 * Excluding non-PON platforms to keep old behavior 
 * this is a specific request from PON customers
 *  PON runner also supports 3-tuple flow acceleration
 *      which is required for ICMPv6
 *  Later for non-PON platforms, this check should be moved
 *      into fcachehw.c */ 
#if defined(CONFIG_BCM_PON_XRDP)
    return 1;
#else
    return 0;
#endif
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvHostNetdevShow
 * Description  : Handler to list Host Device 
 *------------------------------------------------------------------------------
 */
static int fcacheDrvHostNetdevShow(struct seq_file *m, void *v)
{
    int index = 0;
    int bucket=0;
    fc_netdev_t * fc_ndev_p;

    bcm_seq_printf( m, PKTFLOW_MODNAME PKTFLOW_VER_STR "\n" );
    bcm_seq_printf( m, " idx        fc_ndev      netdev        name \n" );

    blog_lock();


    hash_for_each(fc_ndev_db_g.hash, bucket, fc_ndev_p, hnode)
    {

        bcm_seq_printf( m, "%3u %px %px %s \n",
                (int)index, fc_ndev_p, fc_ndev_p->netdev_p,
                fc_ndev_p->netdev_p->name);
        index++;
    }

    blog_unlock();

    return 0;
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvSlowPathStatsShow
 * Description  : Handler to print the slow path stats
 *------------------------------------------------------------------------------
 */
static int fcacheDrvSlowPathStatsShow(struct seq_file *m, void *v)
{
    blog_lock();

    // MOD_INC_USE_COUNT;

    fcache_slow_path_stats_print( m );

    // MOD_DEC_USE_COUNT;

    blog_unlock();

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvFlwBitmapShow
 * Description  : Handler to print the flow bitmap 
 *------------------------------------------------------------------------------
 */
static int fcacheDrvFlwBitmapShow(struct seq_file *m, void *v)
{
    blog_lock();

    // MOD_INC_USE_COUNT;

    fcache_flw_bitmap_print( m );

    // MOD_DEC_USE_COUNT;

    blog_unlock();

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvNotifyEvtStatsShow
 * Description  : Handler to print the notify event stats
 *------------------------------------------------------------------------------
 */
static int fcacheDrvNotifyEvtStatsShow(struct seq_file *m, void *v)
{
    blog_lock();

    // MOD_INC_USE_COUNT;

    fcache_notify_evt_stats_print( m );

    // MOD_DEC_USE_COUNT;

    blog_unlock();

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvQueryEvtStatsShow
 * Description  : Handler to print the query event stats
 *------------------------------------------------------------------------------
 */
static int fcacheDrvQueryEvtStatsShow(struct seq_file *m, void *v)
{
    blog_lock();

    // MOD_INC_USE_COUNT;

    fcache_query_evt_stats_print( m );

    // MOD_DEC_USE_COUNT;

    blog_unlock();

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvEvictStatsShow
 * Description  : Handler to print the flow evict stats
 *------------------------------------------------------------------------------
 */
static int fcacheDrvEvictStatsShow(struct seq_file *m, void *v)
{
    blog_lock();

    // MOD_INC_USE_COUNT;

    fcache_evict_stats_print( m );

    // MOD_DEC_USE_COUNT;

    blog_unlock();

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvErrorStatsShow
 * Description  : Handler to print the errors stats
 *------------------------------------------------------------------------------
 */
static int fcacheDrvErrorStatsShow(struct seq_file *m, void *v)
{
    blog_lock();

    // MOD_INC_USE_COUNT;

    fcache_error_stats_print( m );

    // MOD_DEC_USE_COUNT;

    blog_unlock();

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function     : fcachedrv_evt_list_info_print
 * Description  : Dump the error counters to a proc fs file.
 * Design Note  : Invoked by fcacheDrvErrorStatsProcfs() in fcachedrv.c
 *------------------------------------------------------------------------------
 */
static inline int fcachedrv_evt_list_info_print(struct seq_file *m)
{
    blog_notify_evt_t *evt_p = (blog_notify_evt_t*)dll_head_p( &notify_evt_list );

    bcm_seq_printf( m, "\nFcache Notify Event List Info:\n" );
    bcm_seq_printf( m, 
            "\tevt_list_size=%u  evt_list_extends=%u  "
            "evt_list_extend_fail=%u  evt_list_extend_total=%u\n",
            BLOG_NOTIFY_EVT_QUEUE_SIZE, fcacheDrv_g.notify_evt_list_extends,
            fcacheDrv_g.notify_evt_list_extend_fail,
            fcacheDrv_g.notify_evt_list_extend_total );

    bcm_seq_printf( m, 
            "\tevt_count=%u evt_list_head_p=0x%px\n",
            fcacheDrv_g.notify_evt_count, evt_p );
    bcm_seq_printf( m, "\n" );

    return 0;
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_evt_list_info_show
 * Description  : Handler to print the event list info
 *------------------------------------------------------------------------------
 */
static int fcachedrv_evt_list_info_show(struct seq_file *m, void *v)
{
    blog_lock();

    // MOD_INC_USE_COUNT;

    fcachedrv_evt_list_info_print( m );

    // MOD_DEC_USE_COUNT;

    blog_unlock();

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_slice_info_show
 * Description  : Handler to print the slice info
 *------------------------------------------------------------------------------
 */
static int fcachedrv_slice_info_show(struct seq_file *sf, void *v)
{
    bcm_seq_printf(sf, "num_slices             : %u\n", fcacheDrv_g.num_slices);
    bcm_seq_printf(sf, "slice_period_msec      : %u\n", fcacheDrv_g.slice_period_msec);
    bcm_seq_printf(sf, "num_slice_ent          : %u\n", fcacheDrv_g.num_slice_ent);
    bcm_seq_printf(sf, "slice_drift_jiffies    : %d\n", fcacheDrv_g.slice_drift_jiffies);
    bcm_seq_printf(sf, "slice_ix               : %u\n", fcacheDrv_g.slice_ix);
    return 0;
}

#if defined(CONFIG_BCM_FHW)
/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvFhwStatsShow
 * Description  : Handler to print the FHW stats
 *------------------------------------------------------------------------------
 */
static int fcacheDrvFhwStatsShow(struct seq_file *m, void *v)
{
    blog_lock();

    // MOD_INC_USE_COUNT;

    fhw_stats_print( m );

    // MOD_DEC_USE_COUNT;

    blog_unlock();

    return 0;
}
#endif

/*
 *------------------------------------------------------------------------------
 * Function Name: fcache_drv_npe_seq_xxx
 * Description  : Handler to print the npe in seq
 *------------------------------------------------------------------------------
 */
static void *fcache_drv_npe_seq_start(struct seq_file *s, loff_t *pos)
{
    return (*pos >= fcache_npe_max_ent()) ? NULL : pos;
}

static void *fcache_drv_npe_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos) = (loff_t) fcache_npe_print_get_next((int)(*pos));
    return (*pos >= fcache_npe_max_ent()) ? NULL : pos;
}

static void fcache_drv_npe_seq_stop(struct seq_file *s, void *v)
{
    return;
}

static int fcache_drv_npe_seq_show(struct seq_file *s, void *v)
{
    loff_t *pos = v;
    int ret = 0;

    blog_lock();

    ret = fcache_npe_print_by_idx(s, (int) *pos);

    blog_unlock();

    return ret;
}

static struct seq_operations fcache_drv_npe_seq_ops = {
    .start = fcache_drv_npe_seq_start,
    .next  = fcache_drv_npe_seq_next,
    .stop  = fcache_drv_npe_seq_stop,
    .show  = fcache_drv_npe_seq_show,
};

static int fcache_drv_npe_procfs_open(struct inode *inode, struct file *file)
{
    struct seq_file *s;
    int ret;

    if ((ret = seq_open(file, &fcache_drv_npe_seq_ops)) >= 0)
    {
        s = file->private_data;
        s->private = file;
    }
    return ret;
}

static struct file_operations fcache_drv_npe_procfs_proc = {
        .open = fcache_drv_npe_procfs_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = seq_release,
};

#ifdef FCACHE_PROC_INFO
#include "fcache.h"
const char *strStructName[max_size_idx] =
{
    "fkbuff",
    "sk_buff",
    "Flow_t",
    "Blog_t",
    "FcStatusInfo_t",
    "FcFlushParams_t",
    "FcBindFhwHooks_t",
    "FcBindPathStatHooks_t",
    "FcacheMcastXmitInfo_t",
    "MdnatKey_t",
    "RtpSeqErr_t",
    "RtpSeqEnt_t",
    "RtpSeqGroupEnt_t",
    "MdnatEnt_t",
    "FlowKey_t",
    "SliceEnt_t",
    "FdbKey_t",
    "FcMac_t",
    "FdbEnt_t",
    "npe_flow_t",
    "BlogKeyFc_t",
    "BlogKeyMc_t",
    "BlogActivateKey_t",
    "blog_npe_t",
    "BlogInfo_t",
    "BlogTuple_t",
    "BlogTupleV6_t",
    "BlogGreFlags_t",
    "BlogGre_t",
    "BlogL2tpFlags_t",
    "BlogL2tp_t",
    "BlogEsp_t",
    "BlogHeader_t",
    "BlogHash_t",
    "BlogWfd_t",
    "BlogRnr_t",
};
extern unsigned int fcache_get_data_structure_size(enumStructureSizeIdx_t idx);
extern int fcache_info_print(char *p);
static unsigned int fcacheDrv_get_data_structure_size(enumStructureSizeIdx_t idx)
{
    unsigned int sz = 0;
    switch (idx) {
        case fkbuff_size_idx:
            sz = sizeof(struct fkbuff); break;
        case sk_buff_size_idx:
            sz = sizeof(struct sk_buff); break;
        case Flow_t_size_idx:
            sz = sizeof(Flow_t); break;
        case Blog_t_size_idx:
            sz = sizeof(Blog_t); break;
        case FcStatusInfo_t_size_idx:
            sz = sizeof(FcStatusInfo_t); break;
        case FcFlushParams_t_size_idx:
            sz = sizeof(FcFlushParams_t); break;
        case FcBindFhwHooks_t_size_idx:
            sz = sizeof(FcBindFhwHooks_t); break;
        case FcBindPathStatHooks_t_size_idx:
            sz = sizeof(FcBindPathStatHooks_t); break;
        case FcacheMcastXmitInfo_t_size_idx:
            sz = sizeof(FcacheMcastXmitInfo_t); break;
        case MdnatKey_t_size_idx:
            sz = sizeof(MdnatKey_t); break;
        case RtpSeqErr_t_size_idx:
            sz = sizeof(RtpSeqErr_t); break;
        case RtpSeqEnt_t_size_idx:
            sz = sizeof(RtpSeqEnt_t); break;
        case RtpSeqGroupEnt_t_size_idx:
            sz = sizeof(RtpSeqGroupEnt_t); break;
        case MdnatEnt_t_size_idx:
            sz = sizeof(MdnatEnt_t); break;
        case FlowKey_t_size_idx:
            sz = sizeof(FlowKey_t); break;
        case SliceEnt_t_size_idx:
            sz = sizeof(SliceEnt_t); break;
        case FdbKey_t_size_idx:
            sz = sizeof(FdbKey_t); break;
        case FcMac_t_size_idx:
            sz = sizeof(FcMac_t); break;
        case FdbEnt_t_size_idx:
            sz = sizeof(FdbEnt_t); break;
        case npe_flow_t_size_idx:
            sz = sizeof(npe_flow_t); break;
        case BlogKeyFc_t_size_idx:
            sz = sizeof(BlogKeyFc_t); break;
        case BlogKeyMc_t_size_idx:
            sz = sizeof(BlogKeyMc_t); break;
        case BlogActivateKey_t_size_idx:
            sz = sizeof(BlogActivateKey_t); break;
        case blog_npe_t_size_idx:
            sz = sizeof(blog_npe_t); break;
        case BlogInfo_t_size_idx:
            sz = sizeof(BlogInfo_t); break;
        case BlogTuple_t_size_idx:
            sz = sizeof(BlogTuple_t); break;
        case BlogTupleV6_t_size_idx:
            sz = sizeof(BlogTupleV6_t); break;
        case BlogGreFlags_t_size_idx:
            sz = sizeof(BlogGreFlags_t); break;
        case BlogGre_t_size_idx:
            sz = sizeof(BlogGre_t); break;
        case BlogL2tpFlags_t_size_idx:
            sz = sizeof(BlogL2tpFlags_t); break;
        case BlogL2tp_t_size_idx:
            sz = sizeof(BlogL2tp_t); break;
        case BlogEsp_t_size_idx:
            sz = sizeof(BlogEsp_t); break;
        case BlogHeader_t_size_idx:
            sz = sizeof(BlogHeader_t); break;
        case BlogHash_t_size_idx:
            sz = sizeof(BlogHash_t); break;
        case BlogWfd_t_size_idx:
            sz = sizeof(BlogWfd_t); break;
        case BlogRnr_t_size_idx:
            sz = sizeof(BlogRnr_t); break;
        default:
            break;
    }
    return sz;
}
void fc_data_size_check(void)
{
    int i;
    unsigned int fcacheDrv_sz, fcache_sz;
    printk("\n\n\n============== %s start ==============\n", __FUNCTION__);
    for (i = 0; i < max_size_idx; i++) {
        fcacheDrv_sz = fcacheDrv_get_data_structure_size(i);
        fcache_sz = fcache_get_data_structure_size(i);
        if (fcacheDrv_sz == fcache_sz)
            printk("%s\t %d %d matched\n", strStructName[i], fcacheDrv_sz, fcache_sz);
        else
            printk("%s\t %d %d mismatched!!!!!!!!!!!!!!\n", strStructName[i], fcacheDrv_sz, fcache_sz);
    }
    printk("\n============== %s end ==============\n\n\n", __FUNCTION__);
}
static ssize_t fcacheDrvInfoProcfs(struct file *file, char __user *page,
        size_t len, loff_t *offset)
{
    int bytes = 0;
    if ( *offset == 0 )
    {
        bytes = fcache_info_print( page);
        *offset += bytes;
    }
    return bytes;
}
static struct file_operations fcachedrv_info_proc = {
        .read = fcacheDrvInfoProcfs,
};
#endif /* FCACHE_PROC_INFO */



/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrv_seq_xxx
 * Description  : Handler to print the fcache in seq
 *------------------------------------------------------------------------------
 */
static void *fcacheDrv_seq_start(struct seq_file *s, loff_t *pos)
{
    return (*pos >= fcacheDrv_g.max_flow_ent) ? NULL : pos; 
}

static void *fcacheDrv_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    struct file *file = (struct file *)(s->private);
    enumFcacheDrvProcType eProcType = (enumFcacheDrvProcType)PDE_DATA(file_inode(file));

    (*pos) = (loff_t) fcache_print_get_next(eProcType, (int)(*pos));
    return (*pos >= fcacheDrv_g.max_flow_ent) ? NULL : pos; 
}

static void fcacheDrv_seq_stop(struct seq_file *s, void *v)
{
    return;
}

static int fcacheDrv_seq_show(struct seq_file *s, void *v)
{
    struct file *file = (struct file *)(s->private);
    enumFcacheDrvProcType eProcType = (enumFcacheDrvProcType)PDE_DATA(file_inode(file));
    loff_t *pos = v;
    int ret = 0;

    blog_lock();

    ret = fcache_print_by_idx(s, eProcType, (int)*pos);

    blog_unlock();

    return ret;
}


static struct seq_operations fcacheDrv_seq_ops = {
    .start = fcacheDrv_seq_start,
    .next  = fcacheDrv_seq_next,
    .stop  = fcacheDrv_seq_stop,
    .show  = fcacheDrv_seq_show,
};

static int fcacheDrv_procfs_open(struct inode *inode, struct file *file)
{
    struct seq_file *s;
    int ret;

    if ((ret = seq_open(file, &fcacheDrv_seq_ops)) >= 0)
    {
        s = file->private_data;
        s->private = file;
    }
    return ret;
}

static struct file_operations fcacheDrvProcfs_proc = {
        .open = fcacheDrv_procfs_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = seq_release,
};

/*
 *------------------------------------------------------------------------------
 * Function Name: fcache_drv_mcast_seq_xxx
 * Description  : Handler to print the mcastlist in seq
 *------------------------------------------------------------------------------
 */
static void *fcache_drv_mcast_seq_start(struct seq_file *s, loff_t *pos)
{
    int flow = *pos;
    return (flow >= fcacheDrv_g.max_flow_ent) ? NULL : pos;    
}

static void *fcache_drv_mcast_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    int flow = *pos;

    blog_lock();
    fcache_mcast_print_get_next(&flow);
    blog_unlock();

    *pos = flow;
    return (flow >= fcacheDrv_g.max_flow_ent) ? NULL : pos;    
}

static void fcache_drv_mcast_seq_stop(struct seq_file *s, void *v)
{
    return;
}

static int fcache_drv_mcast_seq_show(struct seq_file *s, void *v)
{
    int flow = *(int *)v;
    int ret = 0;

    blog_lock();
    ret = fcache_mcast_print_by_idx(s, flow);
    blog_unlock();

    return ret;
}

static struct seq_operations fcache_drv_mcast_seq_ops = {
    .start = fcache_drv_mcast_seq_start,
    .next  = fcache_drv_mcast_seq_next,
    .stop  = fcache_drv_mcast_seq_stop,
    .show  = fcache_drv_mcast_seq_show,
};

static int fcache_drv_mcast_procfs_open(struct inode *inode, struct file *file)
{
    struct seq_file *s;
    int ret;

    if ((ret = seq_open(file, &fcache_drv_mcast_seq_ops)) >= 0)
    {
        s = file->private_data;
        s->private = file;
    }
    return ret;
}

static struct file_operations fcache_drv_mcast_procfs_proc = {
        .open = fcache_drv_mcast_procfs_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = seq_release,
};

/*
   seq_file position encoding:
        bit31-21: multicast group index
        bit20   : is_client
        bit19- 0: client index  if is_client bit is set
                  master flow index  if is_client bit is clear 
*/
#define POS_MGRP_OFFSET         20
#define POS_MGRP_CLIENT_FLAG    (1<<(POS_MGRP_OFFSET-1))

#define POS2MGRP(p)         (int)((p) >> POS_MGRP_OFFSET)
#define POS2CM(p)           (int)((p) & ((1<<(POS_MGRP_OFFSET-1))-1))
#define MGRPCM2POS(g,cm)    (loff_t)(((g) << POS_MGRP_OFFSET)|(cm))

#if (((1<<POS_MGRP_OFFSET) < FCACHE_MAX_FLOW_ENTRIES) || ((1<<(31-POS_MGRP_OFFSET)) < FCACHE_MAX_MCAST_GROUPS))
#error "POS_MGRP_OFFSET needs to be changed"
#endif

#if ((1<<(31-POS_MGRP_OFFSET)) < FCACHE_MAX_MCAST_GROUPS)
#error "POS_MGRP_OFFSET needs to be changed"
#endif

/*
 *------------------------------------------------------------------------------
 * Function Name: fcache_drv_mcast_group_info_seq_xxx
 * Description  : Handler to print the mcastlist in seq
 *------------------------------------------------------------------------------
 */
static void *fcache_drv_mcast_group_info_seq_start(struct seq_file *s, loff_t *pos)
{
    int mcast_grp = POS2MGRP(*pos);
    int cm_idx = 0;
    int is_client = 1;

    *pos = MGRPCM2POS(mcast_grp, cm_idx | (is_client ? POS_MGRP_CLIENT_FLAG : 0));
    return (mcast_grp >= FCACHE_CONFIG_MAX_MCAST_GROUPS) ? NULL : pos;
}

static void *fcache_drv_mcast_group_info_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    int mcast_grp = POS2MGRP(*pos);
    int cm_idx = POS2CM(*pos);
    int is_client = (*pos & POS_MGRP_CLIENT_FLAG) ? 1 : 0;

    blog_lock();
    fcache_mcast_group_info_get_next(&mcast_grp, &cm_idx, &is_client);
    blog_unlock();

    *pos = MGRPCM2POS(mcast_grp, cm_idx | (is_client ? POS_MGRP_CLIENT_FLAG : 0));
    return (mcast_grp >= FCACHE_CONFIG_MAX_MCAST_GROUPS) ? NULL : pos;
}

static void fcache_drv_mcast_group_info_seq_stop(struct seq_file *s, void *v)
{
    return;
}

static int fcache_drv_mcast_group_info_seq_show(struct seq_file *s, void *v)
{
    loff_t *pos = v;
    int mcast_grp = POS2MGRP(*pos);
    int cm_idx = POS2CM(*pos);
    int is_client = (*pos & POS_MGRP_CLIENT_FLAG) ? 1 : 0;
    int ret = 0;

    blog_lock();
    ret = fcache_mcast_group_info_print(s, mcast_grp, cm_idx, is_client);
    blog_unlock();

    return ret;
}

static struct seq_operations fcache_drv_mcast_group_info_seq_ops = {
    .start = fcache_drv_mcast_group_info_seq_start,
    .next  = fcache_drv_mcast_group_info_seq_next,
    .stop  = fcache_drv_mcast_group_info_seq_stop,
    .show  = fcache_drv_mcast_group_info_seq_show,
};

static int fcache_drv_mcast_group_info_procfs_open(struct inode *inode, struct file *file)
{
    struct seq_file *s;
    int ret;

    if ((ret = seq_open(file, &fcache_drv_mcast_group_info_seq_ops)) >= 0)
    {
        s = file->private_data;
        s->private = file;
    }
    return ret;
}

static struct file_operations fcache_drv_mcast_group_info_procfs_proc = {
        .open = fcache_drv_mcast_group_info_procfs_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = seq_release,
};

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvFdbProcfs_seq_xxx
 * Description  : Handler to print the fdb in seq
 *------------------------------------------------------------------------------
 */
static void *fcacheDrvFdbProcfs_seq_start(struct seq_file *s, loff_t *pos)
{
    return (*pos >= fcache_fdb_max_ent()) ? NULL : pos;
}

static void *fcacheDrvFdbProcfs_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    return (*pos >= fcache_fdb_max_ent()) ? NULL : pos;
}

static void fcacheDrvFdbProcfs_seq_stop(struct seq_file *s, void *v)
{
    return;
}

static int fcacheDrvFdbProcfs_seq_show(struct seq_file *s, void *v)
{
    loff_t *pos = v;
    int ret = 0;

    blog_lock();

    ret = fcache_fdb_print(s, (int) *pos);

    blog_unlock();

    return ret;
}

static struct seq_operations fcacheDrvFdbProcfs_seq_ops = {
    .start = fcacheDrvFdbProcfs_seq_start,
    .next  = fcacheDrvFdbProcfs_seq_next,
    .stop  = fcacheDrvFdbProcfs_seq_stop,
    .show  = fcacheDrvFdbProcfs_seq_show,
};

static int fcacheDrvFdbProcfs_open(struct inode *inode, struct file *file)
{
    struct seq_file *s;
    int ret;

    if ((ret = seq_open(file, &fcacheDrvFdbProcfs_seq_ops)) >= 0)
    {
        s = file->private_data;
        s->private = file;
    }
    return ret;
}

static struct file_operations fcacheDrvFdbprocfs_proc = {
        .open = fcacheDrvFdbProcfs_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = seq_release,
};


/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvHostDevMacprocfs_seq_xxx
 * Description  : Handler to print host device mac in seq
 *------------------------------------------------------------------------------
 */
static void *fcacheDrvHostDevMacprocfs_seq_start(struct seq_file *s, loff_t *pos)
{
    return (*pos >= FCACHE_CONFIG_MAX_HOST_DEV_ENTRIES) ? NULL : pos;
}

static void *fcacheDrvHostDevMacprocfs_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    return (*pos >= FCACHE_CONFIG_MAX_HOST_DEV_ENTRIES) ? NULL : pos;
}

static void fcacheDrvHostDevMacprocfs_seq_stop(struct seq_file *s, void *v)
{
    return;
}

static int fcacheDrvHostDevMacprocfs_seq_show(struct seq_file *s, void *v)
{
    loff_t *pos = v;
    int ret = 0;

    blog_lock();

    ret = fcache_host_dev_mac_print(s, (int) *pos);

    blog_unlock();

    return ret;
}

static struct seq_operations fcacheDrvHostDevMacprocfs_seq_ops = {
    .start = fcacheDrvHostDevMacprocfs_seq_start,
    .next  = fcacheDrvHostDevMacprocfs_seq_next,
    .stop  = fcacheDrvHostDevMacprocfs_seq_stop,
    .show  = fcacheDrvHostDevMacprocfs_seq_show,
};

static int fcacheDrvHostDevMacProcfs_open(struct inode *inode, struct file *file)
{
    struct seq_file *s;
    int ret;

    if ((ret = seq_open(file, &fcacheDrvHostDevMacprocfs_seq_ops)) >= 0)
    {
        s = file->private_data;
        s->private = file;
    }
    return ret;
}

static struct file_operations fcacheDrvHostDevMacprocfs_proc = {
        .open = fcacheDrvHostDevMacProcfs_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = seq_release,
};



/*
 *------------------------------------------------------------------------------
 * Function Name: pktflow_construct
 * Description  : Initial function that is called at system startup that
 *                registers this device.
 * Returns      : Error or FCACHE_DRV_MAJOR
 *------------------------------------------------------------------------------
 */

static int __init pktflow_construct(void)
{
    uint16_t deferral;       /* HW activation deferral */
    int npe_max_entries;
    int hwFlowMonitor;  /* Monitor and age-out HW Accelerator flows */
    int low_pkt_rate;
    int id;
    int err;
    uint16_t sw_defer_count;  /* SW flow activation defer count, after learning */

    dbg_config( CC_CONFIG_FCACHE_DRV_DBGLVL );

    dbgl_func( DBG_BASIC );

    /* Flow cache acceleration mode */
#if !defined(CONFIG_BRIDGE_NETFILTER) && !defined(G9991)
    blog_support_accel_mode( BLOG_ACCEL_MODE_L23 );
#else
    blog_support_accel_mode( BLOG_ACCEL_MODE_L3 );
#endif


/* Sets how many SW FC flows are supported. Used to dynamically allocate SW FC
 * DDR according to the number of flows. 
 */  
    /* Enable HW flow monitoring only for low flow system */
#if defined(CC_FCACHE_HIGH_FLOW_SYS)
    hwFlowMonitor = 0;
#else
    hwFlowMonitor = 1;
#endif

    /* one nf_conntrack npe for each flow */
    npe_max_entries = FCACHE_CONFIG_MAX_FLOW_ENTRIES;

#if defined(CONFIG_BCM_KF_MAP) && IS_ENABLED(CONFIG_BCM_MAP)
    /* one MAP-T binding npe for each flow */
    npe_max_entries += FCACHE_CONFIG_MAX_FLOW_ENTRIES;
#endif

    printk( CLRbold PKTFLOW_MODNAME CLRnl );
    printk( "FCACHE_CONFIG_MAX_UCAST_FLOW_ENTRIES<%d>\n", FCACHE_CONFIG_MAX_UCAST_FLOW_ENTRIES);
    printk( "FCACHE_CONFIG_MAX_MCAST_GROUPS<%d>\n", FCACHE_CONFIG_MAX_MCAST_GROUPS);
    printk( "FCACHE_CONFIG_MAX_MCAST_CLIENTS_PER_GROUP<%d>\n", FCACHE_CONFIG_MAX_MCAST_CLIENTS_PER_GROUP);
    printk( "FCACHE_CONFIG_MAX_MCAST_CLIENTS<%d>\n", FCACHE_CONFIG_MAX_MCAST_CLIENTS);
    printk( "FCACHE_CONFIG_MAX_FDB_ENTRIES<%d>\n", FCACHE_CONFIG_MAX_FDB_ENTRIES);
    printk( "FCACHE_CONFIG_MAX_HOST_DEV_ENTRIES<%d>\n", FCACHE_CONFIG_MAX_HOST_DEV_ENTRIES);
    printk( "FCACHE_CONFIG_MAX_HOST_MAC_ENTRIES<%d>\n", FCACHE_CONFIG_MAX_HOST_MAC_ENTRIES);
    printk( "npe_max_entries<%d>\n",npe_max_entries);

    if (npe_max_entries > FCACHE_MAX_NPE_ENTRIES)
    {
        print( CLRerr "Invalid values npe_max_entries<%d>" CLRnl,
                      npe_max_entries);
        return FCACHE_ERROR;
    }

    fcache_set_mcast_max_groups(FCACHE_CONFIG_MAX_MCAST_GROUPS);
    fcache_set_mcast_max_clients(FCACHE_CONFIG_MAX_MCAST_CLIENTS);
    fcache_set_mcast_max_clients_per_group(FCACHE_CONFIG_MAX_MCAST_CLIENTS_PER_GROUP);
    fcache_set_ucast_max_ent(FCACHE_CONFIG_MAX_UCAST_FLOW_ENTRIES);
    fcache_set_mcast_max_ent(FCACHE_CONFIG_MAX_MCAST_FLOW_ENTRIES);
    fcache_set_npe_max_ent(npe_max_entries);
    fcache_set_fdb_max_ent(FCACHE_CONFIG_MAX_FDB_ENTRIES);

    fcache_set_tcpack_thresh(FCACHE_TCPACK_DEF_THRESHOLD);

    /* Init notify event free list */
    dll_init(&notify_evt_free_list);

    /* notify Initialize event list */
    dll_init(&notify_evt_list);

    /* Initialize each event entry and insert into free list */
    for ( id=0; id < BLOG_NOTIFY_EVT_QUEUE_SIZE; id++ )
    {
        blog_notify_evt_t *evt_p = &evt_table[id];
        evt_p->evt_type = BLOG_NOTIFY_EVT_NONE;
        evt_p->net_p = NULL;
        evt_p->param1 = 0;
        evt_p->param2 = 0;
        evt_p->notify_cb_fn = NULL;
        evt_p->notify_cb_data_p = NULL;
        dll_append( &notify_evt_free_list, &evt_p->node ); /* Append to free list */
    }

    /* Fcache Event Thread - Start */
    {
        struct task_struct *tsk;

        fcacheDrv_g.fc_evt_thread_running = 0; 
        fcacheDrv_g.fc_evt_thread.work_avail = 0;
        init_waitqueue_head(&fcacheDrv_g.fc_evt_thread.wqh);

        tsk = kthread_create(fcachedrv_evt_thread_func, NULL, "fc_evt");

        if (IS_ERR(tsk)) {
            printk("fc_evt task creation failed\n");
            return FCACHE_ERROR;
        }

        fcacheDrv_g.fc_evt_thread_running = 1; 
        blog_lock();
        blog_bind_notify_evt_enqueue(fcachedrv_notify_evt_enqueue);
        blog_unlock();

        wake_up_process(tsk);

        printk("fc_evt task created successfully\n");
    }
    /* Fcache Thread - End */

    /* Setup slice info before fcache_construct */
    fcacheDrv_g.max_flow_ent = FCACHE_CONFIG_MAX_FLOW_ENTRIES; /* used below */
    {
        uint32_t slice_period_msec;
        uint32_t num_slices;
        uint32_t num_slice_ent;
        err = fcachedrv_get_slice_data(fcacheDrv_g.max_flow_ent, fcacheDrv_g.interval_msec,
                                       &slice_period_msec, &num_slices, &num_slice_ent);

        if ( err )
        {
            return FCACHE_ERROR;
        }
        fcacheDrv_g.slice_period_msec = slice_period_msec;
        fcacheDrv_g.num_slices = num_slices;
        fcacheDrv_g.num_slice_ent = num_slice_ent;
        fcache_interval_sec(fcacheDrv_g.interval_msec / 1000, 1000/fcacheDrv_g.slice_period_msec);
        /* Update the new drift value */
        fcachedrv_update_slice_drift();
    }

    /*
     * ========================
     * Initialize fcache state
     * ========================
     */
    if ( fcache_construct() == FCACHE_ERROR )
        return FCACHE_ERROR;

#if defined(CONFIG_BCM_OVS)
    if (fcache_options_construct(bcmLog_logIsEnabled(BCM_LOG_ID_LOG, BCM_LOG_LEVEL_ERROR)) == FCACHE_ERROR)
        return FCACHE_ERROR;
#endif

    fcacheMonitor(hwFlowMonitor);

    /* Register a character device for Ioctl handling */
    if ( register_chrdev(FCACHE_DRV_MAJOR, FCACHE_DRV_NAME, &fcacheDrv_g.fops) )
    {
        print( CLRerr "%s Unable to get major number <%d>" CLRnl,
                  __FUNCTION__, FCACHE_DRV_MAJOR);
        return FCACHE_ERROR;
    }

    print( CLRbold PKTFLOW_MODNAME " Char Driver " PKTFLOW_VER_STR
                   " Registered<%d>" CLRnl, FCACHE_DRV_MAJOR );
    /*
     * sw_defer_count = n means, after a flow has been learnt, send 
     * next n-1 packets belonging to this flow to Linux network stack
     * (instead of accelerating). n+1 packet onwards flow cache will 
     * start accelerating all the packets of the flow.
     */
    sw_defer_count = 0;
    fcache_set_sw_defer_count(sw_defer_count);

     /* Fcache Thread - Start */
    {
        struct task_struct *tsk;

        fcacheDrv_g.fc_thread.work_avail = 0;
        init_waitqueue_head(&fcacheDrv_g.fc_thread.wqh);

        tsk = kthread_create(fcacheDrvThreadFunc, NULL, "fc_timer");

        if (IS_ERR(tsk)) {
            printk("fc_task creation failed\n");
            return FCACHE_ERROR;
        }

        wake_up_process(tsk);

        printk("fc_timer_task created successfully\n");
    }
    /* Fcache Thread - End */

    fcache_set_sw_defer_count(sw_defer_count);
#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE) || \
    defined(CONFIG_BCM_ARCHER) || defined(CONFIG_BCM_ARCHER_MODULE)
    deferral = 1;

    /* low_pkt_rate (within the refresh interval) */
    low_pkt_rate = 10; /* 1 pps */
#else
    deferral = FCACHE_REACTIVATE;

    /* low_pkt_rate (within the refresh interval) */
    low_pkt_rate = 1; /* 1 pps */
#endif

    fcacheDefer(deferral);

    /* 
     *
     * A flow with packet rate <= low_pkt_rate (within the refresh time)
     * is candidate for eviction (after all idle flows for the new flow of 
     * that priority has been already evicted).
     *
     * The eviction of the flows to make an entry available for a new flow 
     * happens only when flow table is full.
     *
     * The idea behind low_pkt_rate is to make an entry of a flow with 
     * lower packet rate available to flow which has higher rate when the 
     * flow table becomes full.
     */
    fcache_set_low_pkt_rate(low_pkt_rate);


    /* Start a periodic OS refresh timer  */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
    init_timer( &fcacheDrv_g.timer );       /* initialize periodic timer */
    fcacheDrv_g.timer.function = fcacheDrvTimer;
#else
    timer_setup(&fcacheDrv_g.timer, fcacheDrvTimer, 0);
#endif

    fcacheDrv_g.slice_ix = 0;

    fcacheDrv_g.timer.expires = jiffies + msecs_to_jiffies(fcacheDrv_g.slice_period_msec);

    add_timer( &fcacheDrv_g.timer );        /* kick start timer */

#if defined(CC_CONFIG_FCACHE_PROCFS)
    /*
     * Save returned value of proc_mkdir() create_proc_read_entry() and set
     * owner field of struct proc_dir_entry, to support dynamic unloading of
     * pktcmf module. See MOD_INC_USE_COUNT/MOD_DEC_USE_COUNT in each read_proc
     * handler function, to avoid unload while userspace is accessing proc file.
     */
    if ( fcacheDrv_g.proc_fs_created == 0 )
    {
        struct proc_dir_entry *entry;
        proc_mkdir( FCACHE_PROCFS_DIR_PATH, NULL );
        proc_mkdir( FCACHE_MISC_PROCFS_DIR_PATH, NULL );

        entry = proc_create_data(FCACHE_PROCFS_DIR_PATH "/nflist", 
            S_IRUGO, NULL, &fcacheDrvProcfs_proc, (void*)FCACHE_DRV_PROC_TYPE_NF);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for nflist" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_data(FCACHE_PROCFS_DIR_PATH "/brlist", 
            S_IRUGO, NULL, &fcacheDrvProcfs_proc, (void*)FCACHE_DRV_PROC_TYPE_BR);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for brlist" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_data(FCACHE_PROCFS_DIR_PATH "/l2list", 
            S_IRUGO, NULL, &fcacheDrvProcfs_proc, (void*)FCACHE_DRV_PROC_TYPE_L2);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for l2list" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

		#ifdef FCACHE_PROC_INFO
				entry = proc_create_data(FCACHE_PROCFS_DIR_PATH "/info", S_IRUGO,
					NULL, &fcachedrv_info_proc, NULL);
				if (!entry)
				{
				   print( CLRerr "%s Unable to create proc entry for info" CLRnl,
						 __FUNCTION__);
				   return FCACHE_ERROR;
				}
		#endif /* FCACHE_PROC_INFO */

        entry = proc_create_data(FCACHE_MISC_PROCFS_DIR_PATH "/mcastlist", 
            S_IRUGO, NULL, &fcache_drv_mcast_procfs_proc, (void*)FCACHE_DRV_PROC_TYPE_MCAST);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for mcastlist" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_data(FCACHE_MISC_PROCFS_DIR_PATH "/mcast_group_info",
            S_IRUGO, NULL, &fcache_drv_mcast_group_info_procfs_proc,
            (void*)&fcacheDrv_g.index);

        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for mcast group" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_data(FCACHE_MISC_PROCFS_DIR_PATH "/mcastdnatlist", 
            S_IRUGO, NULL, &fcacheDrvProcfs_proc, (void*)FCACHE_DRV_PROC_TYPE_MDNAT);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for mcastdnatlist" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_data(FCACHE_MISC_PROCFS_DIR_PATH "/rtpseqlist", S_IRUGO,
            NULL, &fcacheDrvProcfs_proc, (void*)FCACHE_DRV_PROC_TYPE_RTP_SEQ);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for rtpseqlist" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_data(FCACHE_MISC_PROCFS_DIR_PATH "/host_dev_mac", S_IRUGO,
            NULL,&fcacheDrvHostDevMacprocfs_proc, NULL);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for host_dev_mac" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_single(FCACHE_MISC_PROCFS_DIR_PATH "/host_netdev", S_IRUGO,
            NULL, fcacheDrvHostNetdevShow);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for host_netdev" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_data(FCACHE_MISC_PROCFS_DIR_PATH "/fdblist", S_IRUGO,
            NULL, &fcacheDrvFdbprocfs_proc, NULL);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for fdblist" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_data(FCACHE_MISC_PROCFS_DIR_PATH "/npelist", S_IRUGO,
            NULL, &fcache_drv_npe_procfs_proc, NULL);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for npelist" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_single(FCACHE_MISC_PROCFS_DIR_PATH "/evt_list_info", S_IRUGO,
            NULL, fcachedrv_evt_list_info_show);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for event list info" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }


        entry = proc_create_single(FCACHE_MISC_PROCFS_DIR_PATH "/slice_info", S_IRUGO,
            NULL, fcachedrv_slice_info_show);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for slice info" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        proc_mkdir( FCACHE_STATS_PROCFS_DIR_PATH, NULL );

        entry = proc_create_single(FCACHE_STATS_PROCFS_DIR_PATH "/errors",
            S_IRUGO, NULL, fcacheDrvErrorStatsShow);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for errors" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_single(FCACHE_STATS_PROCFS_DIR_PATH "/evict",
            S_IRUGO, NULL, fcacheDrvEvictStatsShow);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for evict" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_single(FCACHE_STATS_PROCFS_DIR_PATH "/notify",
            S_IRUGO, NULL, fcacheDrvNotifyEvtStatsShow);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for notify" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_single(FCACHE_STATS_PROCFS_DIR_PATH "/query",
            S_IRUGO, NULL, fcacheDrvQueryEvtStatsShow);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for query" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_single(FCACHE_STATS_PROCFS_DIR_PATH "/slow_path",
            S_IRUGO, NULL, fcacheDrvSlowPathStatsShow);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for slow path" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

        entry = proc_create_single(FCACHE_STATS_PROCFS_DIR_PATH "/flow_bmap",
            S_IRUGO, NULL, fcacheDrvFlwBitmapShow);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for flow bitmap" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }

#if defined(CONFIG_BCM_FHW)
        entry = proc_create_single(FCACHE_STATS_PROCFS_DIR_PATH "/fhw",
            S_IRUGO, NULL, fcacheDrvFhwStatsShow);
        if (!entry)
        {
           print( CLRerr "%s Unable to create proc entry for fhw" CLRnl,
                 __FUNCTION__);
           return FCACHE_ERROR;
        }
#endif
        fcacheDrv_g.proc_fs_created = 1;
        printk( "Created Proc FS /procfs/" FCACHE_PROCFS_DIR_PATH "\n");
    }
#endif
    /*allocate netdev database */
    fc_netdev_db_init(FCACHE_NETDEV_DEFAULT_ENTRIES);
    blog_netdev_register_dummy_fn = fc_netdev_register_dummy;
    blog_netdev_unregister_dummy_fn = fc_netdev_unregister_dummy;
    /* Register handler for network device notifications */
    register_netdevice_notifier( &fcacheDrv_g.netdev_notifier );
    printk( CLRbold PKTFLOW_MODNAME "registered with netdev chain" CLRnl );

    fc_bind_blog( 1 );              /* Blogging enabled on module loading */
    printk( CLRbold PKTFLOW_MODNAME "learning via BLOG enabled." CLRnl );

#if defined(CONFIG_BCM_FHW)
    fhw_construct();
#endif

    /* Create fcStatsThread and Initialize */
    flwStatsInit();

    if ( path_drv_construct(fcacheDrv_g.proc_fs_created) != FCACHE_SUCCESS )
    {
        printk( CLRbold "Construct Failed!! " PKTFLOW_MODNAME PKTFLOW_VER_STR CLRnl );
        return -1;
    }

    /* Start pathstat periodic refresh */
    pathstat_calc_slice_step( msecs_to_jiffies(fcacheDrv_g.slice_period_msec) );

    printk( CLRbold "Constructed " PKTFLOW_MODNAME PKTFLOW_VER_STR CLRnl );
	
	#ifdef FCACHE_PROC_INFO
		fc_data_size_check();
	#endif /* FCACHE_PROC_INFO */
	
    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: pktflow_destruct
 * Description  : Initial function that is called at system startup that
 *                registers this device.
 * Returns      : None
 *
 * CAUTION      : MODULE UNLOADING ONLY FOR LAB !!!
 *
 *------------------------------------------------------------------------------
 */
void pktflow_destruct(void)
{
    dbgl_func( DBG_BASIC );

    fc_bind_blog( 0 );              /* Blogging disabled on module unloading */
    printk( CLRbold PKTFLOW_MODNAME "learning via BLOG disabled." CLRnl );


    del_timer( &fcacheDrv_g.timer );/* Delete timer */

#if defined(CONFIG_BCM_FHW)
    fhw_destruct();
#endif

    fcache_destruct();              /* fcache.o: reset all flow state */

    /* Un register for network device notifications */
    unregister_netdevice_notifier( &fcacheDrv_g.netdev_notifier );
    blog_netdev_register_dummy_fn = NULL;
    blog_netdev_unregister_dummy_fn = NULL;
    fc_netdev_db_exit();
    printk( CLRbold2 PKTFLOW_MODNAME "unregistered with netdev chain" CLRnl );

#if defined(CC_CONFIG_FCACHE_PROCFS)
    /* Delete proc filesystem entries */
    if ( fcacheDrv_g.proc_fs_created == 1 )
    {
        remove_proc_entry( FCACHE_STATS_PROCFS_DIR_PATH "/slow_path", NULL );
        remove_proc_entry( FCACHE_STATS_PROCFS_DIR_PATH "/query", NULL );
        remove_proc_entry( FCACHE_STATS_PROCFS_DIR_PATH "/notify", NULL );
        remove_proc_entry( FCACHE_STATS_PROCFS_DIR_PATH "/evict", NULL );
        remove_proc_entry( FCACHE_STATS_PROCFS_DIR_PATH "/errors", NULL );
        remove_proc_entry( FCACHE_STATS_PROCFS_DIR_PATH "/flow_bmap", NULL );
#if defined(CONFIG_BCM_FHW)
        remove_proc_entry( FCACHE_STATS_PROCFS_DIR_PATH "/fhw", NULL );
#endif
        path_drv_remove_proc_entries();

        remove_proc_entry( FCACHE_STATS_PROCFS_DIR_PATH, NULL );

        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/evt_list_info", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/npelist", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/fdblist", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/host_dev_mac", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/host_netdev", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/rtpseqlist", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/mcastdnatlist", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/mcastlist", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH "/mcast_group_info", NULL );
        remove_proc_entry( FCACHE_MISC_PROCFS_DIR_PATH , NULL );

        remove_proc_entry( FCACHE_PROCFS_DIR_PATH "/l2list", NULL );
        remove_proc_entry( FCACHE_PROCFS_DIR_PATH "/nflist", NULL );
        remove_proc_entry( FCACHE_PROCFS_DIR_PATH "/brlist", NULL );
		
		#ifdef FCACHE_PROC_INFO
        remove_proc_entry( FCACHE_PROCFS_DIR_PATH "/info", NULL );
		#endif /* FCACHE_PROC_INFO */

        remove_proc_entry( FCACHE_PROCFS_DIR_PATH, NULL );
        printk( "Deleted Proc FS /procfs/" FCACHE_PROCFS_DIR_PATH "\n");
        fcacheDrv_g.proc_fs_created = 0;
    }
#endif

    /* Un register character device */
    unregister_chrdev( FCACHE_DRV_MAJOR, FCACHE_DRV_NAME );

    printk( CLRbold2 PKTFLOW_MODNAME " Char Driver " PKTFLOW_VER_STR
                    " Unregistered<%d>" CLRnl, FCACHE_DRV_MAJOR );

    printk( CLRbold2 "Destructed " PKTFLOW_MODNAME PKTFLOW_VER_STR CLRnl );
    /*----- OK, Safe to unload now -----*/
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcacheDrvFlushAll
 * Description  : flushes all the dynamic flows.
 * Returns      : 0 - success or error
 *------------------------------------------------------------------------------
 */
int fcacheDrvFlushAll( void )
{
    int count;
    BlogFlushParams_t blogFlushParams = {};

    blogFlushParams.flush_all = 1 ;
    count = fcachedrv_flush_params(&blogFlushParams);

    dbgl_print( DBG_EXTIF, "Flow Cache flushed %d items\n", count );
    return FCACHE_SUCCESS;
}



/*
 *------------------------------------------------------------------------------
 * Function Name: fc_kmalloc
 * Description  : wrapper function for kmalloc, when successfully allocating
 *                memory, the memory should be cache line aligned.
 *                There are configurations in the kernel that can provide
 *                cache aligned allocation.
 *------------------------------------------------------------------------------
 */
void *fc_kmalloc(size_t size, unsigned int flags)
{
    gfp_t kflags;

    if(flags == FCACHE_ALLOC_TYPE_ATOMIC)
    {
        kflags = GFP_ATOMIC;
    }
    else if(flags == FCACHE_ALLOC_TYPE_KERNEL)
    {
        kflags = GFP_KERNEL;
    }
    else
    {
        print( CLRerr "%s Unknown alloc flags <%u>" CLRnl,
                  __FUNCTION__, flags);
        return NULL;
    }

    return kmalloc(size, kflags);
}

void fc_kfree(const void *p)
{
    kfree(p);
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_vmalloc
 * Description  : wrapper function for vmalloc. Use it for memory allocations 
 *                larger than 4MB.
 *------------------------------------------------------------------------------
 */
void *fc_vmalloc(size_t size)
{
    return vmalloc(size);
}

void fc_vfree(const void *p)
{
    vfree(p);
}

uint32_t fc_get_hw_accel(void)
{
#if defined(CONFIG_BCM_FHW)
    return fhw_get_hw_accel();
#else
    return 0;
#endif
}

int fc_get_hw_accel_avail(void)
{
#if defined(CONFIG_BCM_FHW)
    return fhw_get_hw_accel_avail();
#else
    return 0;
#endif
}

int fc_get_hw_host_mac_mgmt_avail(void)
{
#if defined(CONFIG_BCM_FHW)
    return fhw_get_hw_host_mac_mgmt_avail();
#else
    return 0;
#endif
}

#define fc_skb_check_flags(f, k)  \
    if(f == FCACHE_ALLOC_TYPE_ATOMIC) \
        k = GFP_ATOMIC; \
    else if(f == FCACHE_ALLOC_TYPE_KERNEL) \
        k = GFP_KERNEL; \
    else \
    { \
        print( CLRerr "%s Unknown alloc flags <%u>" CLRnl, \
                  __FUNCTION__, f); \
        return NULL; \
    }
 
/*
 *------------------------------------------------------------------------------
 * Function Name: fc_skb_clone
 * Description  : wrapper function for skb_clone
 *------------------------------------------------------------------------------
 */
struct sk_buff *fc_skb_clone(struct sk_buff *skb, unsigned int flags)
{
    gfp_t kflags;

    fc_skb_check_flags(flags, kflags);
    return skb_clone(skb, kflags);
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_skb_unshare
 * Description  : wrapper function for skb_unshare
 *------------------------------------------------------------------------------
 */
struct sk_buff *fc_skb_unshare(struct sk_buff *skb, unsigned int flags)
{
    gfp_t kflags;

    fc_skb_check_flags(flags, kflags);
    return skb_unshare(skb, kflags);
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_xlate_fkb_to_skb
 * Description  : wrapper function to translate fkb to skb 
 *------------------------------------------------------------------------------
 */
struct sk_buff * fc_xlate_fkb_to_skb(struct fkbuff * fkb_p, uint8_t *dirty_p)
{
    return (skb_xlate_dp(fkb_p, dirty_p));
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_skb_frag_xmit4
 * Description  : fragmnet and xmit an ipv4 packet
 *------------------------------------------------------------------------------
 */
void fc_skb_frag_xmit4(struct sk_buff *origskb, void *txdev,
                     uint32_t is_pppoe, uint32_t minMtu, void *ipp)
{
    blog_unlock();
    dev_hold(txdev);
    origskb->bcm_ext.skb_fc_accel = 1;
    skb_frag_xmit4(origskb, txdev, is_pppoe, minMtu, ipp);
    dev_put(txdev);
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_skb_frag_xmit6
 * Description  : fragmnet and xmit an ipv6 packet
 *------------------------------------------------------------------------------
 */
void fc_skb_frag_xmit6(struct sk_buff *origskb, void *txdev,
                     uint32_t is_pppoe, uint32_t minMtu, void *ipp)
{
    blog_unlock();
    dev_hold(txdev);
    origskb->bcm_ext.skb_fc_accel = 1;
    skb_frag_xmit6(origskb, txdev, is_pppoe, minMtu, ipp);
    dev_put(txdev);
}

/*
 *----------------------------------------------------------------------------
 * Function Name: fc_check_update_chksum
 * Description  : Checks whether to update udp checksum for a given skb
 *----------------------------------------------------------------------------
 */
int fc_check_update_chksum(void *ptr)
{
    int ret = FCACHE_SUCCESS;
    struct sk_buff *skb_p = (struct sk_buff *)ptr;
    if((skb_p->ip_summed == CHECKSUM_PARTIAL))
    {
        if ( skb_checksum_help(skb_p) != 0 )
            ret = FCACHE_ERROR;
    }
    return ret;
}
/*
 *------------------------------------------------------------------------------
 * Function Name: fc_dev_xmit
 * Description  : invoke by fcache to transmit a packet via the device's
 *                HardStartXmit function
 *------------------------------------------------------------------------------
 */
void fc_dev_xmit(void *txdev, unsigned long xmit_fn, pNBuff_t nbuff_p)
{
    blog_unlock();
    dev_hold(txdev);

    if (IS_SKBUFF_PTR(nbuff_p) )
        PNBUFF_2_SKBUFF(nbuff_p)->bcm_ext.skb_fc_accel = 1;

    ((HardStartXmitFuncP)xmit_fn)(nbuff_p, txdev);
    dev_put(txdev);
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_mc_queue_skb, fc_mc_queue_fkb
 * Description  : save the given cloned multicast skb/fkb and the required info
 *                for transmitting it at a later time into the given 
 *                FcacheMcastXmitInfo_t structure
 *------------------------------------------------------------------------------
 */
void fc_mc_queue_skb(Blog_t *blog_p,
                     struct sk_buff *skb_p,
                     FcacheMcastXmitInfo_t *info)
{
    skb_p->fkb_mark = blog_p->mark;
    skb_p->priority = blog_p->priority;
    skb_p->bcm_ext.skb_fc_accel = 1;
    info->txdev = (struct net_device *)blog_p->tx_dev_p;
    info->xmit_fn = (HardStartXmitFuncP)blog_p->dev_xmit;
    info->nbuff_p = (void*)CAST_REAL_TO_VIRT_PNBUFF(skb_p,SKBUFF_PTR);
    dev_hold(info->txdev);
}
void fc_mc_queue_fkb(Blog_t *blog_p,
                     FkBuff_t *fkb_p,
                     FcacheMcastXmitInfo_t *info)
{
    fkb_p->mark = blog_p->mark;
    fkb_p->priority = blog_p->priority;
    info->txdev = (struct net_device *)blog_p->tx_dev_p;
    info->xmit_fn = (HardStartXmitFuncP)blog_p->dev_xmit;
    info->nbuff_p = (void*)CAST_REAL_TO_VIRT_PNBUFF(fkb_p,FKBUFF_PTR);
    dev_hold(info->txdev);
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_mc_queue_free
 * Description  : Free the multicast buffers that were put aside for trasmtting
 *                at a later time in the given FcacheMcastXmitInfo_t list.
 *------------------------------------------------------------------------------
 */
void fc_mc_queue_free(FcacheMcastXmitInfo_t *list, uint16_t mc_cnt)
{
    int mc_idx;

    if (list != NULL)
    {
        for (mc_idx = 0; mc_idx < mc_cnt; mc_idx++)
        {
            nbuff_free(list[mc_idx].nbuff_p);
            dev_put(list[mc_idx].txdev);
        }
        kfree(list);
    }
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_mc_xmit
 * Description  : Transmit all the cloned multicast packets that was saved in
 *                in the given FcacheMcastXmitInfo_t list.  The content in the
 *                list was previously populated vi fc_mc_queue_skb/fkb.
 *------------------------------------------------------------------------------
 */
void fc_mc_xmit(FcacheMcastXmitInfo_t *list, uint16_t mc_cnt)
{
    int mc_idx;

    blog_unlock();
    for (mc_idx = 0; mc_idx < mc_cnt; mc_idx++)
    {
        ((HardStartXmitFuncP)list[mc_idx].xmit_fn)(list[mc_idx].nbuff_p,
                                                   list[mc_idx].txdev);
        dev_put(list[mc_idx].txdev);
    }
    kfree(list);
}

void fc_queue_skb(Blog_t *blog_p,
                     struct sk_buff *skb_p,
                     FcacheMcastXmitInfo_t *info)
{
    if (info)
        fc_mc_queue_skb(blog_p, skb_p, info);
    else
        fc_dev_xmit(blog_p->tx_dev_p, blog_p->dev_xmit,
                    (void*)CAST_REAL_TO_VIRT_PNBUFF(skb_p,SKBUFF_PTR));
}

void fc_queue_fkb(Blog_t *blog_p,
                     FkBuff_t *fkb_p,
                     FcacheMcastXmitInfo_t *info)
{
    if (info)
        fc_mc_queue_fkb(blog_p, fkb_p, info);
    else
        fc_dev_xmit(blog_p->tx_dev_p, blog_p->dev_xmit,
                    (void*)CAST_REAL_TO_VIRT_PNBUFF(fkb_p,FKBUFF_PTR));
}


/*
 *------------------------------------------------------------------------------
 * Function Name: fc_get_host_dev_max_ent
 * Description  : returns max number of host device entries
 *------------------------------------------------------------------------------
 */
int fc_get_host_dev_max_ent(void)
{
    return FCACHE_CONFIG_MAX_HOST_DEV_ENTRIES;
}
/*
 *------------------------------------------------------------------------------
 * Function Name: fc_get_host_mac_max_ent
 * Description  : returns max number of host MAC entries
 *------------------------------------------------------------------------------
 */
int fc_get_host_mac_max_ent(void)
{
    return FCACHE_CONFIG_MAX_HOST_MAC_ENTRIES;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_get_num_slices
 * Description  : returns max number of slices
 *------------------------------------------------------------------------------
 */
int fc_get_num_slices(void)
{
    return fcacheDrv_g.num_slices;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_blog_lock/unlock
 * Description  : wrapper for blog_lock and blog_unlock
 *------------------------------------------------------------------------------
 */
void fc_blog_lock(void)
{
    blog_lock();
}
void fc_blog_unlock(void)
{
    blog_unlock();
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_bh_disable/enable
 * Description  : wrapper for local_bh_enable/disable
 *------------------------------------------------------------------------------
 */
void fc_bh_disable(void)
{
    local_bh_disable();
}
void fc_bh_enable(void)
{
    local_bh_enable();
}

int fc_check_wlan_stats_compatibility(void *dev_p, uint8_t phyHdrType)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)) 
     return 1;
#else
#if defined(CONFIG_BCM_WLAN_MODULE) 
    /*back compatible implemenation for wifi counter, if put_stats not set
     *it use new counter implementation, return 1 */
    return  ((phyHdrType != BLOG_WLANPHY)|| !(((struct net_device *)dev_p)->put_stats));
#else
    return 1;
#endif
#endif
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fc_dev_get_by_index
 * Description  : wrapper for dev_get_by_index
 *------------------------------------------------------------------------------
 */
void *fc_dev_get_by_index(int ifindex)
{
    return dev_get_by_index(&init_net, ifindex);
}
/*
 *------------------------------------------------------------------------------
 * Function Name: fc_dev_put
 * Description  : wrapper for dev_put
 *------------------------------------------------------------------------------
 */
void fc_dev_put(void *dev)
{
    dev_put(dev);
}

EXPORT_SYMBOL(fcacheDrvFlushAll);

/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_iqos_cpu_cong
 * Description  : wrapper for iqos_cpu_cong_g
 *------------------------------------------------------------------------------
 */
uint32_t fcachedrv_iqos_cpu_cong(void)
{
#if IS_ENABLED(CONFIG_BCM_INGQOS)
    return iqos_cpu_cong_g;
#else
    return 0;
#endif
}
/*
 *------------------------------------------------------------------------------
 * Function Name: fcachedrv_iqos_prio
 * Description  : wrapper for IQOS API call
 *------------------------------------------------------------------------------
 */
#define HOST_VTAG_TPID(vtag)   ((vtag & 0xFFFF0000) >> 16)
#define HOST_VTAG_PBITS(vtag)  ((vtag & 0x0000E000) >> 13)
#define HOST_VTAG_VID(vtag)    (vtag & 0x00000FFF)
uint32_t fcachedrv_iqos_prio(Blog_t *blog_p)
{
#if defined(CONFIG_BCM_PON)
    /* For PON, L2 flows should always be high priority */
    if (blog_p->rx.info.bmap.PLD_L2)
        return BLOG_IQ_PRIO_HIGH;
#endif

    /* Multicast flows should always be high priority */
    if (unlikely(blog_p->rx.multicast))
        return BLOG_IQ_PRIO_HIGH;

#if (defined(CONFIG_BCM_INGQOS) || defined(CONFIG_BCM_INGQOS_MODULE))
    {
        int rc;
        iqos_param_t param;
        uint8_t dscp;

        if (!iqos_enable_g)
            return BLOG_IQ_PRIO_LOW;

        iqos_key_param_start(&param);

        dscp = BLOG_IPTOS2DSCP(blog_p->rx.tuple.tos);
        iqos_key_param_field_set(&param, IQOS_FIELD_DSCP, (uint32_t *)&dscp, 1);

        if (blog_p->rx.info.bmap.PLD_L2)
        {
            uint16_t eth_type = ntohs(blog_p->eth_type);

            iqos_key_param_field_set(&param, IQOS_FIELD_SRC_MAC,
                                     (uint32_t *)&blog_p->rx.l2hdr[ETH_ALEN], ETH_ALEN);
            iqos_key_param_field_set(&param, IQOS_FIELD_DST_MAC,
                                     (uint32_t *)&blog_p->rx.l2hdr, ETH_ALEN);
            iqos_key_param_field_set(&param, IQOS_FIELD_ETHER_TYPE,
                                     (uint32_t *)&eth_type, 2);

            if ( blog_p->vtag_num )
            {
                uint32_t vtag = ntohl(blog_p->vtag[0]);
                uint16_t vid = HOST_VTAG_VID(vtag);
                uint8_t pbits = HOST_VTAG_PBITS(vtag);
                iqos_key_param_field_set(&param, IQOS_FIELD_OUTER_VID,
                                         (uint32_t *)&vid, 2);
                iqos_key_param_field_set(&param, IQOS_FIELD_OUTER_PBIT,
                                         (uint32_t *)&pbits, 1);
                if ( blog_p->vtag_num > 1 )
                {
                    vtag = ntohl(blog_p->vtag[1]);
                    vid = HOST_VTAG_VID(vtag);
                    pbits = HOST_VTAG_PBITS(vtag);
                    iqos_key_param_field_set(&param, IQOS_FIELD_INNER_VID,
                                             (uint32_t *)&vid, 2);
                    iqos_key_param_field_set(&param, IQOS_FIELD_INNER_PBIT,
                                             (uint32_t *)&pbits, 1);
                }
            }
        }
        else if (blog_p->rx.info.bmap.PLD_IPv4)
        {
            BlogTuple_t *tuple_p  = &blog_p->rx.tuple;
            uint16_t l3_proto = (blog_p->rx.info.bmap.PPP_1661) ?
                BLOG_PPP_IPV4 : BLOG_ETH_P_IPV4;
            uint16_t src_port = ntohs(tuple_p->port.source);
            uint16_t dst_port = ntohs(tuple_p->port.dest);
            uint32_t src_ip = ntohl(tuple_p->saddr);
            uint32_t dst_ip = ntohl(tuple_p->daddr);

            iqos_key_param_field_set(&param, IQOS_FIELD_L3_PROTO,
                                     (uint32_t *)&l3_proto, 2);
            iqos_key_param_field_set(&param, IQOS_FIELD_IP_PROTO,
                                     (uint32_t *)&blog_p->key.protocol, 1);
            iqos_key_param_field_set(&param, IQOS_FIELD_SRC_IP,
                                     (uint32_t *)&src_ip, 4);
            iqos_key_param_field_set(&param, IQOS_FIELD_DST_IP,
                                     (uint32_t *)&dst_ip, 4);
            iqos_key_param_field_set(&param, IQOS_FIELD_SRC_PORT,
                                     (uint32_t *)&src_port, 2);
            iqos_key_param_field_set(&param, IQOS_FIELD_DST_PORT,
                                     (uint32_t *)&dst_port, 2);
        }
        else if (blog_p->rx.info.bmap.PLD_IPv6)
        {
            int i;
            BlogTupleV6_t *tupleV6_p  = &blog_p->tupleV6;
            uint32_t flowLabel = ntohl(blog_p->tupleV6.word0) & 0x000FFFFF;
            uint16_t l3_proto = (blog_p->rx.info.bmap.PPP_1661) ?
                BLOG_PPP_IPV6 : BLOG_ETH_P_IPV6;
            uint16_t src_port = ntohs(tupleV6_p->port.source);
            uint16_t dst_port = ntohs(tupleV6_p->port.dest);
            uint32_t src_ip[4];
            uint32_t dst_ip[4];

            for (i = 0; i < 4; i++)
            {
                src_ip[i] = ntohl(tupleV6_p->saddr.p32[i]);
                dst_ip[i] = ntohl(tupleV6_p->daddr.p32[i]);
            }

            iqos_key_param_field_set(&param, IQOS_FIELD_L3_PROTO,
                                     (uint32_t *)&l3_proto, 2);
            iqos_key_param_field_set(&param, IQOS_FIELD_IP_PROTO,
                                     (uint32_t *)&blog_p->key.protocol, 1);
            iqos_key_param_field_set(&param, IQOS_FIELD_IPV6_FLOW_LABEL,
                                     (uint32_t *)&flowLabel, 4);
            iqos_key_param_field_set(&param, IQOS_FIELD_SRC_IP,
                                     (uint32_t *)&src_ip, 16);
            iqos_key_param_field_set(&param, IQOS_FIELD_DST_IP,
                                     (uint32_t *)&dst_ip, 16);
            iqos_key_param_field_set(&param, IQOS_FIELD_SRC_PORT,
                                     (uint32_t *)&src_port, 2);
            iqos_key_param_field_set(&param, IQOS_FIELD_DST_PORT,
                                     (uint32_t *)&dst_port, 2);
        }

        rc = iqos_key_commit_and_get(&param);

        if (rc == 0 && param.action == IQOS_ACTION_PRIO &&
            param.action_value == IQOS_PRIO_HIGH)
            return BLOG_IQ_PRIO_HIGH;
    }
#endif  /* CONFIG_BCM_INGQOS */

    return BLOG_IQ_PRIO_LOW;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fcache_udp/tcp_port_no_accel
 * Description  : check if given tcp/udp src, dest port pair in the not accelerate list
 *------------------------------------------------------------------------------
 */
typedef enum {
    MATCH_NONE,
    MATCH_DEST,
    MATCH_SRC,
    MATCH_BOTH,
    MATCH_EITHER
} FcMatch_t;

typedef struct {
    uint16_t    dst;
    uint16_t    src;
    FcMatch_t   op;
} L4PortMatch_t;

L4PortMatch_t udp_port_array[] = 
    {
        {htons(BLOG_DHCP_SERVER_PORT), 0, MATCH_DEST},
        {htons(BLOG_DHCP_CLIENT_PORT), 0, MATCH_DEST},
        {htons(BLOG_DNS_SERVER_PORT), htons(BLOG_DNS_SERVER_PORT), MATCH_EITHER},
        {htons(BLOG_TEREDO_SERVER_PORT), htons(BLOG_TEREDO_SERVER_PORT), MATCH_EITHER},
#ifdef CONFIG_TENDA_PRIVATE_KM

#define TD_XMESH_PORT 9801

        {htons(TD_XMESH_PORT), htons(TD_XMESH_PORT), MATCH_BOTH},
#endif
        {0, 0, MATCH_NONE},                                                             /* last NULL entry */
    };

L4PortMatch_t tcp_port_array[] = 
    {
        {htons(BLOG_DNS_SERVER_PORT), htons(BLOG_DNS_SERVER_PORT), MATCH_EITHER},
        {0, 0, MATCH_NONE},                                                             /* last NULL entry */
    };

static int _fcache_l4_port_match(uint16_t dport, uint16_t sport, L4PortMatch_t *entry)
{
    for (; entry->op != MATCH_NONE; entry++)
    {
        switch (entry->op) {
        case MATCH_DEST:    if (dport == entry->dst) return 1; break;
        case MATCH_SRC:     if (sport == entry->src) return 1; break;
        case MATCH_BOTH:    if ((dport == entry->dst) && (sport == entry->src)) return 1; break;
        case MATCH_EITHER:  if ((dport == entry->dst) || (sport == entry->src)) return 1; break;
        default:            break;
        }
    }
    return 0;
}

int fcache_udp_port_no_accel(uint16_t dport, uint16_t sport)
{
    return _fcache_l4_port_match(dport, sport, udp_port_array);
}

int fcache_tcp_port_no_accel(uint16_t dport, uint16_t sport)
{
    return _fcache_l4_port_match(dport, sport, tcp_port_array);
}


int fc_dpi_wlan_enabled (void)
{
#if defined(CONFIG_BCM_DPI_WLAN_QOS)
  return 1;
#endif
  return 0;
}

module_init(pktflow_construct);
module_exit(pktflow_destruct);

MODULE_DESCRIPTION(PKTFLOW_MODNAME);
MODULE_VERSION(PKTFLOW_VERSION);

MODULE_LICENSE("Proprietary");
