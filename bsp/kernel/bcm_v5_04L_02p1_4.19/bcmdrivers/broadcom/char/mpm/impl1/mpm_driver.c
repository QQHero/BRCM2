/*
  <:copyright-BRCM:2020:proprietary:standard

  Copyright (c) 2020 Broadcom 
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
*
* File Name  : mpm_driver.c
*
* Description: Broadcom Memory Pool Manager (MPM) Driver
*
*******************************************************************************
*/

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/nbuff.h>
#include <linux/bcm_skb_defines.h>

#include "board.h"
#include "bcm_rsvmem.h"
#include "bcm_pkt_lengths.h"

#include "mpm_local.h"
#include "mpm.h"

#if defined(CONFIG_BCM94912) || defined(CONFIG_BCM96813) || (CONFIG_BCM963146)
#if defined(CC_MPM_RUNNER)
#define MPM_POOL_0_BUFFER_SIZE  0
#define MPM_POOL_1_BUFFER_SIZE  0
#define MPM_POOL_2_BUFFER_SIZE  0
#define MPM_POOL_3_BUFFER_SIZE  0
#define MPM_DQM_BUFFER_SIZE     2048
#else
#define MPM_POOL_0_BUFFER_SIZE  MPM_SKB_SIZE
#define MPM_POOL_1_BUFFER_SIZE  BCM_PKTBUF_SIZE
#define MPM_POOL_2_BUFFER_SIZE  0
#define MPM_POOL_3_BUFFER_SIZE  0
#endif
#elif IS_ENABLED(CONFIG_BCM_ARCHER)
#define MPM_POOL_0_BUFFER_SIZE  MPM_SKB_SIZE
#define MPM_POOL_1_BUFFER_SIZE  BCM_PKTBUF_SIZE
#define MPM_POOL_2_BUFFER_SIZE  0
#define MPM_POOL_3_BUFFER_SIZE  0
#else
#error "Unknown SoC"
#endif

#undef MPM_DECL
#define MPM_DECL(x) #x,

static const char *mpm_ioctl_cmd_name[] =
{
    MPM_DECL(MPM_IOC_STATUS)
    MPM_DECL(MPM_IOC_STATS)
    MPM_DECL(MPM_IOC_DEBUG)
    MPM_DECL(MPM_IOC_DUMP)
    MPM_DECL(MPM_IOC_UT)
    MPM_DECL(MPM_IOC_MAX)
};

typedef struct {
    mpm_config_t config;
    struct device *dummy_dev;
} mpm_driver_t;

static mpm_driver_t mpm_driver_g;

/*******************************************************************************
 *
 * Helper Functions
 *
 *******************************************************************************/

int mpm_driver_coherent_mem_alloc(mpm_mem_t *mem_p, int size, const char *name)
{
    mem_p->size = size + BCM_DCACHE_LINE_LEN;

    mem_p->host_unaligned_p = (volatile void *)
        dma_alloc_coherent(mpm_driver_g.dummy_dev, mem_p->size,
                           &mem_p->phys_unaligned_addr, GFP_KERNEL);

    if(!mem_p->host_unaligned_p)
    {
        __logError("Could not dma_alloc_coherent: size %d (%d)\n", size, mem_p->size);

        return -ENOMEM;
    }

    memset((void *)mem_p->host_unaligned_p, 0, mem_p->size);

    mem_p->host_p = (volatile void *)BCM_DCACHE_ALIGN((uintptr_t)(mem_p->host_unaligned_p));
    mem_p->phys_addr = BCM_DCACHE_ALIGN(mem_p->phys_unaligned_addr);

    bcm_print("%s: Host 0x%px (0x%px), Phys 0x%08x (0x%08x), Size %u (%u) bytes\n", name,
              mem_p->host_p, mem_p->host_unaligned_p,
              mem_p->phys_addr, mem_p->phys_unaligned_addr,
              size, mem_p->size);

    return 0;
}

void mpm_driver_coherent_mem_free(mpm_mem_t *mem_p)
{
    dma_free_coherent(mpm_driver_g.dummy_dev, mem_p->size,
                      (void *)mem_p->host_unaligned_p,
                      mem_p->phys_unaligned_addr);
}

static int mpm_dummy_device_alloc(void)
{
    if(mpm_driver_g.dummy_dev == NULL)
    {
        mpm_driver_g.dummy_dev = kzalloc(sizeof(struct device), GFP_KERNEL);

        if(mpm_driver_g.dummy_dev == NULL)
        {
            __logError("Could not allocate dummy_dev");

            return -1;
        }
#ifdef CONFIG_BCM_GLB_COHERENCY
        arch_setup_dma_ops(mpm_driver_g.dummy_dev, 0, 0, NULL, true);
#else
        arch_setup_dma_ops(mpm_driver_g.dummy_dev, 0, 0, NULL, false);
#endif
        /* according to spec, we should be able to do 40-bits */
        dma_coerce_mask_and_coherent(mpm_driver_g.dummy_dev, DMA_BIT_MASK(32));
    }

    return 0;
}

static void mpm_dummy_device_free(void)
{
    if(mpm_driver_g.dummy_dev)
    {
        kfree(mpm_driver_g.dummy_dev);
    }
}

int mpm_skb_size(void)
{
    return sizeof(struct sk_buff);
}

int mpm_skb_users_offset(void)
{
    return offsetof(struct sk_buff, users);
}

int mpm_skb_users_size(void)
{
    return MPM_MEMBER_SIZEOF(struct sk_buff, users);
}

int mpm_skb_next_offset(void)
{
    return offsetof(struct sk_buff, next);
}

int mpm_skb_prev_offset(void)
{
    return offsetof(struct sk_buff, prev);
}

int mpm_skb_head_offset(void)
{
    return offsetof(struct sk_buff, head);
}

int mpm_skb_data_offset(void)
{
    return offsetof(struct sk_buff, data);
}

int mpm_skb_tail_offset(void)
{
    return offsetof(struct sk_buff, tail);
}

int mpm_skb_end_offset(void)
{
    return offsetof(struct sk_buff, end);
}

int mpm_skb_data_size(void)
{
    return sizeof(sk_buff_data_t);
}

int mpm_bcm_pkt_headroom(void)
{
    return BCM_PKT_HEADROOM;
}

int mpm_bcm_max_pkt_len(void)
{
    return BCM_MAX_PKT_LEN;
}

int mpm_bcm_skb_tailroom(void)
{
    return BCM_SKB_TAILROOM;
}

int mpm_bcm_skb_sharedinfo(void)
{
    return BCM_SKB_SHAREDINFO;
}

int mpm_data_shinfo_dataref_offset(void)
{
    return MPM_DATA_SHINFO_OFFSET + offsetof(struct skb_shared_info, dataref);
}

int mpm_data_shinfo_dataref_size(void)
{
    return MPM_MEMBER_SIZEOF(struct skb_shared_info, dataref);
}

/*******************************************************************************
 *
 * IOCTL
 *
 *******************************************************************************/

static long mpm_ioctl(struct file *filep, unsigned int command, unsigned long arg)
{
    mpm_ioctl_cmd_t cmd = (command >= MPM_IOC_MAX) ?
        MPM_IOC_MAX : (mpm_ioctl_cmd_t)command;
    int ret = 0;

    __logDebug("cmd %s (%d), arg<0x%08lX>",
               mpm_ioctl_cmd_name[cmd - MPM_IOC_STATUS], cmd, arg);

    switch(cmd)
    {
        case MPM_IOC_STATUS:
            bcm_print("MPM Status:\n");
            break;

        case MPM_IOC_STATS:
            mpm_stats_dump();
            break;

        case MPM_IOC_DEBUG:
            bcmLog_setLogLevel(BCM_LOG_ID_MPM, arg);
            break;

        case MPM_IOC_DUMP:
            mpm_reg_dump();
            break;

        case MPM_IOC_UT:
            ret = mpm_ut_run(arg, (void *)mpm_driver_g.config.virt_base_addr,
                             mpm_driver_g.config.total_mem_size);
            break;

        default:
            __logError("Invalid Command [%u]", command);
            ret = -1;
    }

    return ret;
}

/*******************************************************************************
 *
 * Initialization
 *
 *******************************************************************************/

static int mpm_open(struct inode *inode, struct file *filp)
{
    __logDebug("MPM Open");

    return 0;
}

static struct file_operations mpm_fops =
{
    .unlocked_ioctl = mpm_ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = mpm_ioctl,
#endif
    .open = mpm_open,
};

#if defined(CC_MPM_RUNNER)
static int mpm_pool_size_set(int pool_nbr, int buffer_size, int ebuf_size, int *alloc_buffer_size_p)
{
    int alloc_pool_size;
    int pool_size;

    switch(pool_nbr)
    {
        case 0:
            alloc_pool_size = 1;
            pool_size = MPM_BUF_SIZE_1;
            break;

        case 1:
            alloc_pool_size = 2;
            pool_size = MPM_BUF_SIZE_2;
            break;

        case 2:
            alloc_pool_size = 4;
            pool_size = MPM_BUF_SIZE_4;
            break;

        case 3:
            alloc_pool_size = 8;
            pool_size = MPM_BUF_SIZE_8;
            break;

        default:
            BCM_ASSERT(0);
    }

    *alloc_buffer_size_p = ebuf_size * alloc_pool_size;

    bcm_print("Pool[%d]: alloc %d EBUFs (%d bytes)\n",
              pool_nbr, alloc_pool_size, *alloc_buffer_size_p);

    return pool_size;
}
#else
static int mpm_pool_size_set(int pool_nbr, int buffer_size, int ebuf_size, int *alloc_buffer_size_p)
{
    int min_pool_size;

    if(!buffer_size)
    {
        bcm_print("Pool[%d]: Disabled\n", pool_nbr);

        *alloc_buffer_size_p = -1;

        return MPM_BUF_SIZE_DISABLED;
    }

    min_pool_size = buffer_size / ebuf_size;
    if(buffer_size % ebuf_size)
    {
        // roundup
        min_pool_size++;
    }

    bcm_print("Pool[%d]: buffer_size %d bytes, min %d, ", pool_nbr, buffer_size, min_pool_size);

#if (MPM_BUF_SIZE_IMPL == 1)
    if(min_pool_size <= 5)
    {
        *alloc_buffer_size_p = ebuf_size * min_pool_size;

        bcm_print("alloc %d EBUFs (%d bytes)\n", min_pool_size, *alloc_buffer_size_p);

        return min_pool_size;
    }
    else if(min_pool_size <= 8)
    {
        *alloc_buffer_size_p = ebuf_size * 8;

        bcm_print("alloc 8 EBUFs (%d bytes)\n", *alloc_buffer_size_p);

        return MPM_BUF_SIZE_8;
    }
    else if(min_pool_size <= 10)
    {
        *alloc_buffer_size_p = ebuf_size * 10;

        bcm_print("alloc 10 EBUFs (%d bytes)\n", *alloc_buffer_size_p);

        return MPM_BUF_SIZE_10;
    }
    else if(min_pool_size <= 20)
    {
        *alloc_buffer_size_p = ebuf_size * 20;

        bcm_print("alloc 20 EBUFs (%d bytes)\n", *alloc_buffer_size_p);

        return MPM_BUF_SIZE_20;
    }
    else if(min_pool_size <= 40)
    {
        *alloc_buffer_size_p = ebuf_size * 40;

        bcm_print("alloc 40 EBUFs (%d bytes)\n", *alloc_buffer_size_p);

        return MPM_BUF_SIZE_40;
    }
#else /* if (MPM_BUF_SIZE_IMPL == 2) */
    if(min_pool_size <= 8)
    {
        *alloc_buffer_size_p = ebuf_size * min_pool_size;

        bcm_print("alloc %d EBUFs (%d bytes)\n", min_pool_size, *alloc_buffer_size_p);

        return min_pool_size;
    }
    else if(min_pool_size <= 10)
    {
        *alloc_buffer_size_p = ebuf_size * 10;

        bcm_print("alloc 10 EBUFs (%d bytes)\n", *alloc_buffer_size_p);

        return MPM_BUF_SIZE_10;
    }
    else if(min_pool_size <= 20)
    {
        *alloc_buffer_size_p = ebuf_size * 20;

        bcm_print("alloc 20 EBUFs (%d bytes)\n", *alloc_buffer_size_p);

        return MPM_BUF_SIZE_20;
    }
#endif

    __logError("Pool[%d]: Invalid buffer_size %d, ebuf_size %d", pool_nbr, buffer_size, ebuf_size);

    return -1;
}
#endif

static int mpm_pool_nbr_set(int *alloc_buffer_size_p, int buffer_size)
{
    int pool_index;

    for(pool_index=0; pool_index<MPM_NBR_OF_POOLS; ++pool_index)
    {
        if(alloc_buffer_size_p[pool_index] >= buffer_size)
        {
            return pool_index;
        }
    }

    __logError("Invalid buffer_size %d", buffer_size);

    return -1;
}

static int mpm_memory_init(void)
{
    void *virt_base;
    phys_addr_t phy_base;
    unsigned int total_mem_size;
    int alloc_buffer_size[MPM_NBR_OF_POOLS];
    int ebuf_size;
    int ebuf_total;
    int ret;

    ret = BcmMemReserveGetByName(BUFMEM_BASE_ADDR_STR, &virt_base, &phy_base, &total_mem_size);
    if(ret)
    {
        return ret;
    }

    ebuf_size = 256;
    ebuf_total = total_mem_size / ebuf_size;
    if(ebuf_total > MPM_EBUF_TOTAL_MAX)
    {
        ebuf_size = 512;
        ebuf_total = total_mem_size / ebuf_size;
        if(ebuf_total > MPM_EBUF_TOTAL_MAX)
        {
            ebuf_size = 1024;
            ebuf_total = total_mem_size / ebuf_size;
            if(ebuf_total > MPM_EBUF_TOTAL_MAX)
            {
                ebuf_size = 2048;
                ebuf_total = total_mem_size / ebuf_size;
                if(ebuf_total > MPM_EBUF_TOTAL_MAX)
                {
                    ebuf_total = MPM_EBUF_TOTAL_MAX;
                }
            }
        }
    }

    bcm_print("MPM: virt_base = 0x%px, phy_base = 0x%llx, total_mem_size = %d\n",
              virt_base, phy_base, total_mem_size);

    bcm_print("MPM: ebuf_size = %u, ebuf_total = %u/%u\n", ebuf_size, ebuf_total, MPM_EBUF_TOTAL_MAX);

    mpm_driver_g.config.phys_base_addr = (uintptr_t)phy_base;
    mpm_driver_g.config.virt_base_addr = (uintptr_t)virt_base;
    mpm_driver_g.config.total_mem_size = total_mem_size;
    mpm_driver_g.config.ebuf_size = ebuf_size;
    mpm_driver_g.config.ebuf_total = ebuf_total;
    mpm_driver_g.config.pool_size[0] = mpm_pool_size_set(0, MPM_POOL_0_BUFFER_SIZE,
                                                         ebuf_size, &alloc_buffer_size[0]);
    mpm_driver_g.config.pool_size[1] = mpm_pool_size_set(1, MPM_POOL_1_BUFFER_SIZE,
                                                         ebuf_size, &alloc_buffer_size[1]);
    mpm_driver_g.config.pool_size[2] = mpm_pool_size_set(2, MPM_POOL_2_BUFFER_SIZE,
                                                         ebuf_size, &alloc_buffer_size[2]);
    mpm_driver_g.config.pool_size[3] = mpm_pool_size_set(3, MPM_POOL_3_BUFFER_SIZE,
                                                         ebuf_size, &alloc_buffer_size[3]);
    mpm_driver_g.config.skb_pool_nbr = mpm_pool_nbr_set(alloc_buffer_size, MPM_SKB_SIZE);
    mpm_driver_g.config.data_pool_nbr = mpm_pool_nbr_set(alloc_buffer_size, BCM_PKTBUF_SIZE);
#if defined(CC_MPM_RUNNER)
    mpm_driver_g.config.dqm_pool_nbr = mpm_pool_nbr_set(alloc_buffer_size, MPM_DQM_BUFFER_SIZE);
    if (mpm_driver_g.config.data_pool_nbr == -1)
        mpm_driver_g.config.data_pool_nbr = mpm_driver_g.config.dqm_pool_nbr;

    bcm_print("MPM Pool Numbers: SKB = %d, Data = %d, DQM = %d\n",
              mpm_driver_g.config.skb_pool_nbr, mpm_driver_g.config.data_pool_nbr,
              mpm_driver_g.config.dqm_pool_nbr);
#else
    bcm_print("MPM Pool Numbers: SKB = %d, Data = %d\n",
              mpm_driver_g.config.skb_pool_nbr, mpm_driver_g.config.data_pool_nbr);
#endif

    return 0;
}

int __init mpm_construct(void)
{
    int ret;

    bcm_print(CLRcb "Broadcom MPM Driver Intializing" CLRnl);

    bcmLog_setLogLevel(BCM_LOG_ID_MPM, BCM_LOG_LEVEL_ERROR);

    memset(&mpm_driver_g, 0, sizeof(mpm_driver_t));

    ret = mpm_memory_init();
    if(ret)
    {
        return ret;
    }

    ret = mpm_dummy_device_alloc();
    if(ret)
    {
        return ret;
    }

    ret = mpm_init(&mpm_driver_g.config);
    if(ret)
    {
        mpm_dummy_device_free();

        return ret;
    }

    ret = register_chrdev(MPM_DRV_MAJOR, MPM_DRV_NAME, &mpm_fops);
    if(ret)
    {
        __logError("Unable to get major number <%d>", MPM_DRV_MAJOR);

        return ret;
    }

    bcm_print(CLRcb MPM_MODNAME " Char Driver " MPM_VER_STR " Registered <%d>" CLRnl, MPM_DRV_MAJOR);

    return 0;
}

void __exit mpm_destruct(void)
{
    unregister_chrdev(MPM_DRV_MAJOR, MPM_DRV_NAME);

    mpm_dummy_device_free();
}

module_init(mpm_construct);
module_exit(mpm_destruct);

MODULE_DESCRIPTION(MPM_MODNAME);
MODULE_VERSION(MPM_VERSION);
MODULE_LICENSE("Proprietary");
