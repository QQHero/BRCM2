/*
  <:copyright-BRCM:2019:proprietary:standard

  Copyright (c) 2019 Broadcom 
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
* File Name  : sysport_wol.c
*
* Description: This file implements WOL / ACPI feature for Archer.
*              (note WOL is specific to BCM47622 for now)
*
*******************************************************************************
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/bcm_log.h>
#include <linux/blog.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "bcm_map_part.h"
#include "bcm_intr.h"

#include "sysport_rsb.h"
#include "sysport_classifier.h"
#include "archer.h"
#include "archer_driver.h"
#include "sysport_driver.h"

#if defined(CONFIG_BCM963178) || defined(CONFIG_BCM947622) || defined(CONFIG_BCM96756)
typedef struct {
    struct net_device   *dev;
    int intf_index;
} sysport_dev_dma_intf_t;

static sysport_dev_dma_intf_t sysport_dev_dma_intf_g[BCM_ENET_SYSPORT_BLOG_CHNL_MAX];

#if defined(CONFIG_BCM963178) || defined(CONFIG_BCM96756)
#define ARCHER_SYSPORT_INTF     1 //BCM63178 and BCM6756 has 1 systemport
#elif defined (CONFIG_BCM947622)
#define ARCHER_SYSPORT_INTF     2 //BCM47622 has 2 systemports
#else
#error "unsupported SOC"
#endif

volatile sysport *sysport_return_regp (int intf_index)
{
    volatile sysport *sysport_p;

    switch (intf_index)
    {
        case 0:
            sysport_p = SYSPORT(0);
            break;
#if defined (CONFIG_BCM947622)
        case 1:
            sysport_p = SYSPORT(1);
            break;
#endif
        default:
            __logError ("intf_index %d not supported\n", intf_index);
            sysport_p = NULL;
            break;
    }
    return sysport_p;
}

/*
*******************************************************************************
* Function   : sysport_device_to_dma_intf
* Description: return sysport DMA interface based on network device name
*******************************************************************************
*/
static int sysport_device_to_dma_intf (char *intf_name, int *intf_index)
{
    int i, ret = -1;
    struct net_device *dev;

    /* look for the name of the root device */
    dev = dev_get_by_name(&init_net, intf_name);
    if (!dev)
    {
        __logError("invalid device requested %s\n", intf_name);
        return -1;
    }
    while(1)
    {
        if(netdev_path_is_root(dev))
            break;

        dev_put(dev);
        dev = netdev_path_next_dev(dev);
        dev_hold(dev);
    }
    dev_put(dev);

    /* root device found, look for the systemport interface */
    for (i=0; i < BCM_ENET_SYSPORT_BLOG_CHNL_MAX; i++)
    {
        if (dev->name == sysport_dev_dma_intf_g[i].dev->name)
        {
            *intf_index = sysport_dev_dma_intf_g[i].intf_index;
            ret = 0;

            __logDebug("DMA interface found, root device %s dma index %d\n", dev->name, sysport_dev_dma_intf_g[i].intf_index);
            break;
        }
    }
    return ret;
}

/*
*******************************************************************************
* Function   : sysport_wol_dev_mapping
* Description: keep track of network device and sysport interface
*******************************************************************************
*/
void sysport_wol_intf_dev_mapping (int port_idx, bcmSysport_BlogChnl_t *blog_chnl)
{
    sysport_dev_dma_intf_g[port_idx].dev = blog_chnl->dev;
    sysport_dev_dma_intf_g[port_idx].intf_index = blog_chnl->sysport;
}

/*
*******************************************************************************
* Function   : sysport_wol_mpd_cfg
* Description: configure magic packet system port interface would wake up to
*******************************************************************************
*/
void sysport_wol_mpd_cfg (archer_mpd_cfg_t *mpd_cfg)
{
    uint32_t mac_h, mac_l;
    volatile sysport *sysport_p;
    unsigned char *mac_addr;
    int intf_index;

    if (sysport_device_to_dma_intf(mpd_cfg->intf_name, &intf_index) == 0)
    {
        if (mpd_cfg->mode == ARCHER_MPD_INTF)
        {
            struct net_device *dev;

            dev = dev_get_by_name (&init_net, mpd_cfg->intf_name);
            mac_addr = dev->dev_addr;
        }
        else
        {
            mac_addr = mpd_cfg->mac_addr;
        }

        sysport_p = sysport_return_regp(intf_index);

        /* configure the mac address for MPD */
        mac_h = (uint32_t)mac_addr[0] << 24 | (uint32_t)mac_addr[1] << 16 |
            (uint32_t)mac_addr[2] << 8  | mac_addr[3];
        mac_l = (uint32_t)mac_addr[4] << 8  | mac_addr[5];

#if defined (CONFIG_BCM947622)
        sysport_p->SYSTEMPORT_UNIMAC.SYSTEMPORT_UMAC_MAC0 = mac_h;
        sysport_p->SYSTEMPORT_UNIMAC.SYSTEMPORT_UMAC_MAC1 = mac_l;
#elif defined(CONFIG_BCM963178) || defined(CONFIG_BCM96756)
        sysport_p->SYSTEMPORT_GIB.SYSTEMPORT_GIB_MAC_DA_0 = mac_h;
        sysport_p->SYSTEMPORT_GIB.SYSTEMPORT_GIB_MAC_DA_1 = mac_l;
#endif
    }
}

/*
*******************************************************************************
* WOL Proc entries implementation Function 
*******************************************************************************
*/
#define PROC_DIR        "driver/archer"
#define WOL_PROC_FILE   "/wol_status"

static struct proc_dir_entry *proc_dir;

static int archer_wol_status_show(struct seq_file *m, void *v)
{
    int i;
    uint32_t v32;
    volatile sysport *sysport_p;

    seq_printf(m, "number of sysport interfaces: %d\n", ARCHER_SYSPORT_INTF);

    for (i=0; i < ARCHER_SYSPORT_INTF; i++)
    {
        sysport_p = sysport_return_regp(i);

        v32 = sysport_p->SYSTEMPORT_RBUF.SYSTEMPORT_RBUF_RBUF_STATUS;

        seq_printf(m, "interface %d in %s mode\n", i, 
                          (v32 & SYSPORT_RBUF_STATUS_WOL_M)? "WOL" : "Active");
    }

    return 0;
}

int sysport_wol_proc_init(void)
{
    int ret = 0;
    proc_dir = proc_mkdir(PROC_DIR, NULL);
    if (!proc_dir)
    {
        __logError("Could not create Archer WOL PROC directory\n");
        ret = -1;
    }

    if (!proc_create_single(PROC_DIR WOL_PROC_FILE, S_IRUGO, NULL, archer_wol_status_show))
    {
        __logError("failed to create wol status entry\n");
        ret = -1;
    }
    return ret;
}

/*
*******************************************************************************
* WOL Interrupt routine 
*******************************************************************************
*/
#define _STATIC_INLINE_ static inline

#if defined(SYSTEMPORT_X_INTRL2_PHY)

/* routines when WOL is using the stand along WOL insterrupt */
#define SYSPORT_WOL_INTR_M = 0x10000;
_STATIC_INLINE_ void sysport_clear_wol_intr (volatile sysport *sysport_p)
{
    volatile SYSTEMPORT_X_INTRL2_PHY *phy_intrl2_p = &sysport_p->SYSTEMPORT_1_INTRL2_PHY;

    phy_intrl2_p->SYSTEMPORT_INTRL2_PHY_CPU_CLEAR = SYSPORT_WOL_INTR_M;
}

_STATIC_INLINE_ void sysport_disable_wol_intr (volatile sysport *sysport_p)
{
    volatile SYSTEMPORT_X_INTRL2_PHY *phy_intrl2_p = &sysport_p->SYSTEMPORT_1_INTRL2_PHY;

    phy_intrl2_p->SYSTEMPORT_INTRL2_PHY_CPU_MASK_SET = SYSPORT_WOL_INTR_M;
}

_STATIC_INLINE_ void sysport_enable_wol_intr (volatile sysport *sysport_p)
{
    volatile SYSTEMPORT_X_INTRL2_PHY *phy_intrl2_p = &sysport_p->SYSTEMPORT_1_INTRL2_PHY;

    phy_intrl2_p->SYSTEMPORT_INTRL2_PHY_CPU_MASK_CLEAR = SYSPORT_WOL_INTR_M;
}

int sysport_wol_get_intr_id (int intf)
{
    int isr = SYSPORT_WOL_INTERRUPT_ID(0);

#if ARCHER_SYSPORT_INTF > 1
    if (intf == 1)
        isr = SYSPORT_WOL_INTERRUPT_ID(1);
#endif
    return isr;
}

#else
/* routines when WOL is handled as part of RX_MISC interrupts */
#define SYSPORT_RX_MISC_ARP_INTR        3
#define SYSPORT_RX_MISC_ARP_INTR_M      (1 << SYSPORT_RX_MISC_ARP_INTR)
#define SYSPORT_RX_MISC_MPD_INTR        2
#define SYSPORT_RX_MISC_MPD_INTR_M      (1 << SYSPORT_RX_MISC_MPD_INTR)

/* map RX_MISC to use the last available interrupt controller (4 or 5) */
#if defined(INTERRUPT_ID_SYSPORT5)
#define SYSPORT_WOL_INTC    5
#else
#define SYSPORT_WOL_INTC    4
#endif
#define SYSPORT_WOL_INTR_M  0x2 /* map rx_misc_intr to INT1 */


_STATIC_INLINE_ void sysport_clear_wol_intr (volatile sysport *sysport_p)
{
    volatile SYSTEMPORT_INTRL2 *intrl2_p = &sysport_p->SYSTEMPORT_INTRL2[SYSPORT_WOL_INTC];

    intrl2_p->SYSTEMPORT_INTR_CPU_CLEAR = SYSPORT_WOL_INTR_M;

    /* clear ARP and MPD interrupt in RX_MISC interrupt */
    intrl2_p = &sysport_p->SYSTEMPORT_INTRL2_MISC_RX;
    intrl2_p->SYSTEMPORT_INTR_CPU_CLEAR = (SYSPORT_RX_MISC_MPD_INTR_M | SYSPORT_RX_MISC_ARP_INTR_M);

}

_STATIC_INLINE_ void sysport_disable_wol_intr (volatile sysport *sysport_p)
{
    volatile SYSTEMPORT_INTRL2 *intrl2_p = &sysport_p->SYSTEMPORT_INTRL2[SYSPORT_WOL_INTC];

    intrl2_p->SYSTEMPORT_INTR_CPU_MASK_SET = SYSPORT_WOL_INTR_M;

    /* enable ARP and MPD interrupt in RX_MISC interrupt */
    intrl2_p = &sysport_p->SYSTEMPORT_INTRL2_MISC_RX;
    intrl2_p->SYSTEMPORT_INTR_CPU_MASK_SET = (SYSPORT_RX_MISC_MPD_INTR_M | SYSPORT_RX_MISC_ARP_INTR_M);
}

_STATIC_INLINE_ void sysport_enable_wol_intr (volatile sysport *sysport_p)
{
    volatile SYSTEMPORT_INTRL2 *intrl2_p = &sysport_p->SYSTEMPORT_INTRL2[SYSPORT_WOL_INTC];

    /* enable RX_MISC interrupt as sysport interrupt source */
    intrl2_p->SYSTEMPORT_INTR_CPU_MASK_CLEAR = SYSPORT_WOL_INTR_M;

    /* enable ARP and MPD interrupt in RX_MISC interrupt */
    intrl2_p = &sysport_p->SYSTEMPORT_INTRL2_MISC_RX;
    intrl2_p->SYSTEMPORT_INTR_CPU_MASK_CLEAR = (SYSPORT_RX_MISC_MPD_INTR_M | SYSPORT_RX_MISC_ARP_INTR_M);
}

int sysport_wol_get_intr_id (int intf)
{
    volatile sysport *sysport_p = sysport_return_regp(intf);
    volatile SYSTEMPORT_INTC *intc_p;
    int isr = SYSPORT_INTERRUPT_ID(0, 0) + SYSPORT_WOL_INTC;

#if ARCHER_SYSPORT_INTF > 1
    if (intf == 1)
        isr = SYSPORT_INTERRUPT_ID(1, 0) + SYSPORT_WOL_INTC;
#endif
    /* when WOL isr is handled as part of the RX_MISC interrupt */
    /* map interrupt to one of the controllers */
    intc_p = &sysport_p->SYSTEMPORT_INTC[SYSPORT_WOL_INTC];

#if defined(CONFIG_BCM96756)
    intc_p->SYSTEMPORT_INTC_INTC_MAPPING_7_0 = (SYSPORT_INTC_MAPPING_RX_MISC_INTR <<
                                                SYSPORT_INTC_MAPPING_INT_SEL_S(1));
#else
    intc_p->SYSTEMPORT_INTC_INTC_MAPPING_0 = (SYSPORT_INTC_MAPPING_RX_MISC_INTR <<
                                              SYSPORT_INTC_MAPPING_INT_SEL_S(1));
#endif

    return isr;
}

#endif

static FN_HANDLER_RT sysport_wol_isr(int irq, void *param)
{

    uint32_t v32, rbuf_status;
    int intf = (int)param;
    volatile sysport *sysport_p = sysport_return_regp(intf);

    rbuf_status = sysport_p->SYSTEMPORT_RBUF.SYSTEMPORT_RBUF_RBUF_STATUS;

    sysport_disable_wol_intr(sysport_p);

    // disable ACPI (WOL) mode
    v32 = sysport_p->SYSTEMPORT_RBUF.SYSTEMPORT_RBUF_RBUF_CONTROL;
    v32 &= ~SYSPORT_RBUF_CTRL_ACPI_EN_M;
    sysport_p->SYSTEMPORT_RBUF.SYSTEMPORT_RBUF_RBUF_CONTROL = v32;

    // disable MPD
    v32 = sysport_p->SYSTEMPORT_MPD.SYSTEMPORT_MPD_CTRL;
    v32 &= ~(0x1); // MPD disable
    sysport_p->SYSTEMPORT_MPD.SYSTEMPORT_MPD_CTRL = v32;

    // disable WOL interrupt on ARP
    v32 = sysport_p->SYSTEMPORT_RXCHK.SYSTEMPORT_RXCHK_CONTROL;
    v32 &= ~SYSPORT_RXCHK_CONTROL_ARP_INTR_EN_M;
    v32 &= ~SYSPORT_RXCHK_CONTROL_BRCM_TAG_MATCH_EN_M;
    sysport_p->SYSTEMPORT_RXCHK.SYSTEMPORT_RXCHK_CONTROL = v32;

    sysport_clear_wol_intr(sysport_p);

    bcm_print("\n* WOL IRQ %d ** rbuf status 0x%x\n", irq, rbuf_status);

    return BCM_IRQ_HANDLED;
}

/*
*******************************************************************************
* WOL initialization 
*******************************************************************************
*/
int sysport_wol_init(void)
{
    int ret, wol_interrupt_id;
    volatile sysport *sysport_p = SYSPORT(0);

    wol_interrupt_id = sysport_wol_get_intr_id (0);
    sysport_clear_wol_intr(sysport_p);

    BcmHalMapInterrupt(sysport_wol_isr, 0, wol_interrupt_id);

    bcm_print("Sysport 0 WOL IRQ %d\n", wol_interrupt_id);

#if ARCHER_SYSPORT_INTF > 1
    sysport_p = SYSPORT(1);
    wol_interrupt_id = sysport_wol_get_intr_id (1);
    sysport_clear_wol_intr(sysport_p);

    BcmHalMapInterrupt(sysport_wol_isr, (void *)1, wol_interrupt_id);

    bcm_print("Sysport 1 WOL IRQ %d\n", wol_interrupt_id);
#endif

    memset(&sysport_dev_dma_intf_g, 0, sizeof(sysport_dev_dma_intf_g));

    ret = sysport_wol_proc_init();

    return ret;
}

/*
*******************************************************************************
* Function   : sysport_wol_enter
* Description: set specific system port to WOL mode 
this function is called assuming MPD MAC addresses has been programmed
*******************************************************************************
*/
void sysport_wol_enter(char *dev_name)
{
    uint32_t v32;
    int intf_index;
    volatile sysport *sysport_p;
    
    if (sysport_device_to_dma_intf(dev_name, &intf_index) == 0)
    {
        sysport_p = sysport_return_regp(intf_index);

        // enable both ARP and MPD
        v32 = sysport_p->SYSTEMPORT_RXCHK.SYSTEMPORT_RXCHK_CONTROL;
        v32 |= SYSPORT_RXCHK_CONTROL_ARP_INTR_EN_M;
        sysport_p->SYSTEMPORT_RXCHK.SYSTEMPORT_RXCHK_CONTROL = v32;

        v32 = sysport_p->SYSTEMPORT_MPD.SYSTEMPORT_MPD_CTRL;
        v32 |= SYSPORT_MPD_CTRL_MPD_EN;
        sysport_p->SYSTEMPORT_MPD.SYSTEMPORT_MPD_CTRL = v32;
        sysport_clear_wol_intr(sysport_p);
        sysport_enable_wol_intr (sysport_p);
    }
}

#endif
