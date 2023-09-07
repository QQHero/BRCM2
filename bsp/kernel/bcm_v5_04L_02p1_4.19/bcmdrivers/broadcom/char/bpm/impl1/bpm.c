/*
<:copyright-BRCM:2009:proprietary:standard

   Copyright (c) 2009 Broadcom
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
 * File Name  : bpm.c
 *
 *******************************************************************************
 */
/* -----------------------------------------------------------------------------
 *                      Global Buffer Pool Manager (BPM)
 * -----------------------------------------------------------------------------
 * When the system boots up all the buffers are owned by BPM.
 *
 * Interface Initialization:
 * ------------------------
 * When an interface is initialized, the interface assigns buffers to
 * descriptors in it's private RX ring by requesting buffer allocation
 * from BPM (APIs: gbpm_alloc_mult_buf() or gbpm_alloc_buf()),
 * and also informs BPM how many buffers were assigned to RX ring
 * (gbpm_resv_rx_buf()).
 *
 * Similarly, when an interface is uninitialized, the interface frees
 * buffers from descriptors in it's private RX ring by requesting buffer
 * free to BPM (APIs: gbpm_free_mult_buf() or gbpm_free_buf()),
 * and also informs BPM how many buffers were freed from RX ring
 * (gbpm_unresv_rx_buf()).
 *
 * Knowing the size of RX ring allows BPM to keep track of the reserved
 * buffers (assigned to rings) and hence find out how many dynamic buffers
 * are available, which can be shared between the interfaces in the system.
 *
 * Buffer Allocation:
 * ------------------
 * When a packet is received, the buffer is not immediately replenished
 * into RX ring, rather a count is incremented, to keep track of how many
 * buffer allocation requests are pending. This is done to delay as much
 * as possible going to BPM because of overheads (locking, cycles, etc.),
 * and it likely that the RX ring will be soon replenished with a recycled
 * buffer.
 *
 * When an interface's RX ring buffers usage (RX DMA queue depth) exceeds
 * the configured threshold (because the earlier used buffers are not yet
 * recycled to RX ring), the interface requests BPM for allocation of
 * more buffers. The buffer request is fullfilled from the available
 * dynamic buffers in BPM. After one or two such buffer allocation requests
 * equilibirium is established where the newly used buffers are replenished
 * with the recycled buffers.
 *
 * Buffer Free/Recycle:
 * --------------------
 *  After a packet is transmitted or dropped, the recycle function of
 *  RX interface is invoked. Recycle function first checks whether there
 *  is a space in the RX ring. If yes, the buffer is recycled to ring.
 *  If not, buffer is freed to BPM.
 *
 *
 * FkBuff_t:
 * Even though a Buffer has a FkBuff_t at the start of a buffer, this FkBuff_t
 * structure is not initialized as in fkb_preinit(). The recycle_hook needs to
 * be appropriately setup. An FkBuff_t structure does not have a recycle_flags
 * and the recycle_context field was re-purposed. Need to undo this and use the
 * recycle context to save the &bpm_g.
 *
 *
 * TX Queue Threhsolds:
 * --------------------
 * TX Queue Thresholds are configured for the slower interfaces like XTM,
 * MoCA, so that a lot of buffers are not held up in the TX queues of
 * these interfaces.
 *
 * There are two TX Q thresholds per queue of an interface: low and high.
 * Only one of the low or high threshold is used to compare against the
 * current queue depth. If the TX Q depth is lower than the compared
 * threshold, the packet is enqueued, else the packet is dropped.
 *
 * The low or high threshold to be used is decided based on the current
 * level of dynamic buffers available in the system
 * (API gbpm_get_dyn_buf_lvl() ). If the current dynamic buffer
 * level is less than the configured threshold, then the low threshold
 * is used else the high threshold is used.
 *
 *
 * Pre-allocated sk_buff Pool
 * --------------------------
 * BPM also supports a preallocated pool of sk_buffs. BPM managed sk_buffs,
 * avoid the need for a kmem skb_head_cache alloc, and reduced memsets of the
 * struct sk_buff and the skb_shared_info. Unlike skb_header_init(), where a
 * skb_shared_info may be relocated, soon after the tail, given a specified
 * tailroom, in a BPM sk_buff, the tailroom will always be beyond the DMA-able
 * region, on a cacheline aligned boundary. BPM sk_buffs may only be attached to
 * BPM buffers (or buffers that follow the bcm_pkt_lengths.h formats).
 *
 *
 * Note: API prototypes are given in gbpm.h file.
 */
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/nbuff.h>
#include <linux/fs.h>
#include <linux/bcm_log_mod.h>
#include <linux/bcm_log.h>
#include <linux/export.h>
#include <board.h>
#include <linux/gbpm.h>
#include <bpmctl_common.h>
#include <bcm_pkt_lengths.h>
#include <bcmPktDma_defines.h>
#include <bpm.h>
#include "bcm_prefetch.h"
#include <linux/kthread.h>
#include "bpm_common.h"
#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
#include "bpm_track.h"
#endif

/*----- Globals -----*/
extern gbpm_t gbpm_g;

#define GBPM_NOOP()                 do { /* noop */ } while (0)

/* GBPM USER Hook Invocations by BPM */

#define GBPM_USER_INVOKE( HOOKNAME, ARG... )                                   \
({                                                                             \
    if (likely(gbpm_g.HOOKNAME != (gbpm_ ## HOOKNAME ## _hook_t)NULL) )        \
        (gbpm_g.HOOKNAME)( ARG );                                              \
})

#define GBPM_USER_ENET_STATUS()         GBPM_USER_INVOKE(enet_status)

#define GBPM_USER_ENET_THRESH()         GBPM_NOOP()

#if defined(GBPM_XTM_SUPPORT)
#define GBPM_USER_XTM_STATUS()          GBPM_USER_INVOKE(xtm_status)
#define GBPM_USER_XTM_THRESH()          GBPM_USER_INVOKE(xtm_thresh)
#else  /* ! GBPM_XTM_SUPPORT */
#define GBPM_USER_XTM_STATUS()          GBPM_NOOP()
#define GBPM_USER_XTM_THRESH()          GBPM_NOOP()
#endif /* ! GBPM_XTM_SUPPORT */

#define GBPM_USER_STATUS()                                                     \
({ GBPM_USER_ENET_STATUS(); GBPM_USER_XTM_STATUS(); })

#define GBPM_USER_THRESH()                                                     \
({ GBPM_USER_XTM_THRESH(); })

#undef BPM_DECL
#define BPM_DECL(x) #x,

const char *bpmctl_ioctl_name[] =
{
    BPM_DECL(BPMCTL_IOCTL_SYS)
    BPM_DECL(BPMCTL_IOCTL_MAX)
};

const char *bpmctl_subsys_name[] =
{
    BPM_DECL(BPMCTL_SUBSYS_STATUS)
    BPM_DECL(BPMCTL_SUBSYS_THRESH)
    BPM_DECL(BPMCTL_SUBSYS_BUFFERS)
    BPM_DECL(BPMCTL_SUBSYS_SKBUFFS)
#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
    BPM_DECL(BPMCTL_SUBSYS_TRACK)
#endif
    BPM_DECL(BPMCTL_SUBSYS_MAX)
};

const char *bpmctl_op_name[] =
{
    BPM_DECL(BPMCTL_OP_SET)
    BPM_DECL(BPMCTL_OP_GET)
    BPM_DECL(BPMCTL_OP_ADD)
    BPM_DECL(BPMCTL_OP_REM)
    BPM_DECL(BPMCTL_OP_DUMP)
    BPM_DECL(BPMCTL_OP_MAX)
};

#if !defined(CONFIG_BCM_MPM_OVER_BPM)
static void __bpm_dump_status(void)
{
    bpm_dump_status();
    GBPM_USER_STATUS();
}

static void __bpm_dump_thresh(void)
{
    bpm_dump_thresh();

    printk("\n---------------------------------------\n");
    printk("        dev  txq loThr hiThr    dropped\n");
    printk("------ ---- ---- ----- ----- ----------\n");

    GBPM_USER_THRESH();
    GBPM_USER_ENET_THRESH();
}
#endif

/*
 *------------------------------------------------------------------------------
 * Function Name: bpm_drv_ioctl
 * Description  : Main entry point to handle user applications IOCTL requests
 *                from BPM Control Utility.
 * Returns      : 0 - success or error
 *------------------------------------------------------------------------------
 */
static int bpm_drv_ioctl(struct inode *inode, struct file *filep,
                         unsigned int command, unsigned long arg)
{
    bpmctl_ioctl_t cmd;
    bpmctl_data_t bpm;
    bpmctl_data_t *bpm_p = &bpm;
    int ret = BPM_SUCCESS;

    if (command > BPMCTL_IOCTL_MAX)
        cmd = BPMCTL_IOCTL_MAX;
    else
        cmd = (bpmctl_ioctl_t)command;

    copy_from_user(bpm_p, (uint8_t *)arg, sizeof(bpm));

    BCM_LOG_DEBUG(BCM_LOG_ID_BPM,
                  "cmd<%d> %s subsys<%d> %s op<%d> %s arg<0x%lx>",
                  command, bpmctl_ioctl_name[command - BPMCTL_IOCTL_SYS],
                  bpm_p->subsys, bpmctl_subsys_name[bpm_p->subsys],
                  bpm_p->op, bpmctl_op_name[bpm_p->op], arg);

    switch (cmd) {
#if !defined(CONFIG_BCM_MPM_OVER_BPM)
        case BPMCTL_IOCTL_SYS:
            switch (bpm_p->subsys) {
                case BPMCTL_SUBSYS_STATUS:
                    switch (bpm_p->op) {
                        case BPMCTL_OP_DUMP:
                            __bpm_dump_status();
                            break;

                        default:
                            BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Invalid op[%u]", bpm_p->op);
                    }
                    break;

                case BPMCTL_SUBSYS_THRESH:
                    switch (bpm_p->op) {
                        case BPMCTL_OP_DUMP:
                            __bpm_dump_thresh();
                            break;

                        default:
                            BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Invalid op[%u]", bpm_p->op);
                    }
                    break;

                case BPMCTL_SUBSYS_BUFFERS:
                    switch (bpm_p->op) {
                        case BPMCTL_OP_DUMP:
                            bpm_dump_buffers();
                            break;

                        default:
                            BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Invalid op[%u]", bpm_p->op);
                    }
                    break;

#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
                case BPMCTL_SUBSYS_TRACK:
                    ret = bpm_track_ioctl(&(bpm_p->track));
                    break;
#endif

                case BPMCTL_SUBSYS_SKBUFFS:
                    switch (bpm_p->op) {
                        case BPMCTL_OP_DUMP:
                            bpm_dump_skbuffs();
                            break;

                        default:
                            BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Invalid op[%u]", bpm_p->op);
                    }
                    break;

                default:
                    BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Invalid subsys[%u]", bpm_p->subsys);
                    break;
            }
            break;
#endif
        default:
            BCM_LOG_ERROR(BCM_LOG_ID_BPM, "Invalid cmd[%u]", command );
            ret = BPM_ERROR;
    }

    return ret;
} /* bpm_drv_ioctl */


static DEFINE_MUTEX(bpmIoctlMutex);

static long bpm_drv_unlocked_ioctl(struct file *filep,
    unsigned int cmd, unsigned long arg)
{
    struct inode *inode;
    long rt;

    inode = file_inode(filep);

    mutex_lock(&bpmIoctlMutex);
    rt = bpm_drv_ioctl( inode, filep, cmd, arg );
    mutex_unlock(&bpmIoctlMutex);

    return rt;
}


/*
 *------------------------------------------------------------------------------
 * Function Name: bpm_drv_open
 * Description  : Called when a user application opens this device.
 * Returns      : 0 - success
 *------------------------------------------------------------------------------
 */
static int bpm_drv_open(struct inode *inode, struct file *filp)
{
    BCM_LOG_DEBUG( BCM_LOG_ID_BPM, "Access BPM Char Device" );
    return BPM_SUCCESS;
} /* bpm_drv_open */


/* Global file ops */
static struct file_operations bpm_fops =
{
    .unlocked_ioctl = bpm_drv_unlocked_ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = bpm_drv_unlocked_ioctl,
#endif
    .open   = bpm_drv_open,
};


/*
 *------------------------------------------------------------------------------
 * Function Name: bpm_drv_construct
 * Description  : Initial function that is called at system startup that
 *                registers this device.
 * Returns      : BPM_ERROR or BPM_DRV_MAJOR.
 *------------------------------------------------------------------------------
 */
static int bpm_drv_construct(void)
{
    if ( register_chrdev( BPM_DRV_MAJOR, BPM_DRV_NAME, &bpm_fops ) )
    {
        BCM_LOG_ERROR( BCM_LOG_ID_BPM,
                "%s Unable to get major number <%d>" CLRnl,
                  __FUNCTION__, BPM_DRV_MAJOR);
        return BPM_ERROR;
    }

    printk( BPM_MODNAME " Char Driver " BPM_VER_STR " Registered<%d>"
                                                    CLRnl, BPM_DRV_MAJOR );

    return BPM_DRV_MAJOR;
}

static int __init bpm_module_init( void )
{
    bcmLog_setLogLevel(BCM_LOG_ID_BPM, BCM_LOG_LEVEL_NOTICE);

    bpm_drv_construct();

#if 0
    BCM_LOG_DEBUG( BCM_LOG_ID_BPM, "%s: bpm_pg<0x%px>", __FUNCTION__,
            bpm_pg );
#endif

    if (bpm_pool_manager_init())
        return BPM_ERROR;

#if !defined(CONFIG_BCM_MPM_OVER_BPM)
#if defined(CONFIG_BCM_BPM_BUF_TRACKING)
    /* TODO! see how to revert the initialization when fail, (if it is needed) */
    if (bpm_track_init())
        return BPM_ERROR;
#endif
#endif
    return 0;
}

module_init( bpm_module_init );

#if 0
/*
 *------------------------------------------------------------------------------
 * Function Name: bpm_drv_destruct
 * Description  : Final function that is called when the module is unloaded.
 * Returns      : None.
 *------------------------------------------------------------------------------
 */
static void bpm_drv_destruct(void)
{
    unregister_chrdev( BPM_DRV_MAJOR, BPM_DRV_NAME );

    printk( BPM_MODNAME " Char Driver " BPM_VER_STR " Unregistered<%d>"
                                                    CLRnl, BPM_DRV_MAJOR);
}


/*
 * Cannot remove BPM module because buffers allocated by BPM are already
 * in use with all the RX rings.
 */
static void bpm_module_exit( void )
{
    BCM_LOG_ERROR( BCM_LOG_ID_BPM, "Cannot remove BPM Module !!!" );

    gbpm_unbind();
#if !defined(CONFIG_BCM_MPM_OVER_BPM)
#if defined(CC_BPM_SKB_POOL_BUILD)
    bpm_fini_skb_pool();
#endif
    bpm_fini_buf_pool();
#endif
    BCM_LOG_NOTICE( BCM_LOG_ID_BPM, "BPM Module Exit" );
    bpm_drv_destruct();
}
module_exit( bpm_module_exit );
#endif



