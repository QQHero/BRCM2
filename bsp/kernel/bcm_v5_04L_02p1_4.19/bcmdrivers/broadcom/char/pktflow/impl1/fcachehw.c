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
 * File Name  : fcachehw.c
 * This file is fcache interface to HW Accelerators.
 *******************************************************************************
 */
#include <bcmenet_common.h>
#include <linux/nbuff.h>
#include <linux/blog.h>
#include <linux/blog_net.h>
#include "fcache.h"
#include "fcachehw.h"
#include "idx_pool_util.h"

extern int  fcache_max_ent(void);

/*----- Forward declaration -----*/

/*----- APIs exported to ONLY fcachedrv.c fhw_xyz() -----*/
/*----- helper functions in fcachedrv.c -----*/

/*
 *------------------------------------------------------------------------------
 * Flow cache design debugging.
 *------------------------------------------------------------------------------
 */
#undef PKT_DBG_SUPPORTED
#define CLRsys              CLRb
#define DBGsys              "[FHW] "
#if defined(CC_CONFIG_FHW_DBGLVL)
#define PKT_DBG_SUPPORTED 
#define PKT_ASSERT_SUPPORTED
static int pktDbgLvl = CC_CONFIG_FHW_DBGLVL;
#endif
#if defined(CC_CONFIG_FHW_COLOR)
#define PKT_DBG_COLOR_SUPPORTED
#endif
#include "pktDbg.h"
#include <linux/bcm_log.h>

#if defined(CC_CONFIG_FCACHE_DEBUG)    /* Runtime debug level setting */
int fcacheFhwDebug(int lvl) { dbg_config( lvl ); return lvl; }
#endif

#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE)
#include <rdpa_api.h>
#include <bdmf_interface.h>
#include <rdpa_types.h>
#include <rdpa_cpu.h>
#include <rdpa_ag_cpu.h>
#endif

/*----- Globals -----*/

static uint32_t g_fhw_hw_accel_enabled = 1; /* Global control to enable/disable HW acceleration */

typedef struct {
    /*
     * hooks initialized by HW Packet Accelerators (Runner, ARCHER, etc.) 
     * FHW makes upcalls into HW Packet Accelerators via theses hooks
     */
    FhwBindHwHooks_t hwHooks;
    IdxPool_t      freeIdxPool;
    HwEnt_t        *hwtbl;
} HwAcc_t;

typedef struct {
    uint32_t    actFail;        /* Flow Activation Failure */
    uint32_t    actSuccess;     /* Flow Activation Success */
    uint32_t    deactSuccess;   /* Flow Deactivation Success */
    uint32_t    updSuccess;     /* Flow Update Success */
    uint32_t    refreshFail;    /* Flow Refresh Failure */
    uint32_t    refreshSuccess; /* Flow Refresh Success */
    uint32_t    macAddFail;     /* Host MAC Add Failure */
    uint32_t    macAddSuccess;  /* Host MAC Add Success */
    uint32_t    macDelFail;     /* Host MAC Delete Failure */
    uint32_t    macDelSuccess;  /* Host MAC Delete Success */
    struct {
        uint32_t disable;       /* HW Acceleration Disable */
        uint32_t T6in4;         /* 6in4 Tunnel */
        uint32_t sdnat;         /* SDNAT */
        uint32_t wltx;          /* WLAN TX */
        uint32_t wlrx;          /* WLAN RX */
        uint32_t mapt;          /* MAPT */
        uint32_t localTcp;      /* Local TCP Flow */
        uint32_t l2tp;          /* L2TP */
        uint32_t pptp;          /* PPTP */
        uint32_t esp;           /* ESP */
        uint32_t hwPort;        /* RX HW Port */
        uint32_t xoa;           /* XoA */
        uint32_t usb;           /* USB */
        uint32_t lte;           /* LTE */
        uint32_t l2l;           /* LAN-2-LAN */
        uint32_t mcRtpSeq;      /* Multicast RTP Sequence */
        uint32_t gre;           /* GRE */
        uint32_t w2w;           /* WAN-2-WAN */
        uint32_t llcsnap;       /* LLCSNAP */
        uint32_t vxlan;         /* VxLAN */
        uint32_t vtagNum;       /* Number of VLAN Tags */
        uint32_t spdTst;        /* Speed Test */
        uint32_t mapt_gre;      /* MAPT and LAN GRE */
        uint32_t t4in6_gre;     /* T4in6 and LAN GRE */
        uint32_t t6in4_gre;     /* T6in4 and LAN GRE */
    }hwSup;
} FhwStats_t;


typedef struct {
    uint32_t    bindCount;
    int         status;           
    int         unused;           
    FhwHwAccPrio_t cap2HwAccMap[HW_CAP_MAX][FHW_PRIO_MAX];
    HwAcc_t     hwAcc[FHW_PRIO_MAX] _FCALIGN_;
    FhwStats_t  stats;
} __attribute__((aligned(16))) Fhw_t;


Fhw_t fhw; /* Global FHW context */
static FC_CLEAR_HOOK fhw_fc_clear_hook = (FC_CLEAR_HOOK)NULL;

uint32_t inline calcU32RollOverDiff(uint32_t currVal, uint32_t prevVal, uint32_t maxVal)
{
    uint32_t diffVal;

    if (currVal >= prevVal) 
    {
       diffVal = currVal - prevVal;
    }
    else
    {
       diffVal = (maxVal - prevVal) + currVal;
    }
    return diffVal;
}

#if defined(CONFIG_BCM_XRDP)
#define HW_NUM_OF_BITS_IN_HIT_COUNT 28
#define HW_NUM_MAX_PKT_HIT_COUNT ((1U<< HW_NUM_OF_BITS_IN_HIT_COUNT ) - 1)
#else
#define HW_NUM_OF_BITS_IN_HIT_COUNT 32
#define HW_NUM_MAX_PKT_HIT_COUNT U32_MAX
#endif

#define HW_NUM_MAX_BYTE_HIT_COUNT U32_MAX

uint32_t inline calcHwPktHitDiff(uint32_t currVal, uint32_t prevVal)
{
    return calcU32RollOverDiff(currVal, prevVal, HW_NUM_MAX_PKT_HIT_COUNT);
}

uint32_t inline calcHwByteHitDiff(uint32_t currVal, uint32_t prevVal)
{
    return calcU32RollOverDiff(currVal, prevVal, HW_NUM_MAX_BYTE_HIT_COUNT);
}

/*
 *------------------------------------------------------------------------------
 * Function     : fhw_alloc_hwtbl
 * Description  : Allocates the HW entries table for the HWACC
 *------------------------------------------------------------------------------
 */
static int fhw_alloc_hwtbl(FhwHwAccPrio_t hwAccIx, int max_ent)
{
    int idx;
    HwEnt_t *hwEnt_p = NULL;
    char fhw_name[16];

    dbgl_print( DBG_EXTIF, "hwAccIx<%d> max_ent<%d>", hwAccIx, max_ent );

    /* one extra entry to cache line align (16B) */
    hwEnt_p = (HwEnt_t *) 
        ( ( (uintptr_t) kmalloc( (sizeof(HwEnt_t) * (max_ent+1) ),
        GFP_ATOMIC) ) & ~0x0F);

    if (hwEnt_p == NULL)
    {
        fc_error("Out of memory for HW Table" );   
        return FHW_ERROR;
    }
    memset( (void*)hwEnt_p, 0, (sizeof(HwEnt_t) * max_ent) );
    fhw.hwAcc[hwAccIx].hwtbl = hwEnt_p; 

    /* Initialize each HW entry */
    for (idx=0; idx < max_ent; idx++)
    {
        hwEnt_p = &fhw.hwAcc[hwAccIx].hwtbl[idx]; 
        hwEnt_p->hw_key = FHW_TUPLE_INVALID;
    }
    /* Initialize the index pool for HW Table (hwtbl) */
    snprintf(fhw_name, sizeof(fhw_name), "FHW[%d]",hwAccIx);

    if ( idx_pool_init(&fhw.hwAcc[hwAccIx].freeIdxPool, max_ent, fhw_name) )
    {
        return FHW_ERROR;
    }
    return FHW_SUCCESS;
}


/*
 *------------------------------------------------------------------------------
 * Function     : fhw_free_hwtbl
 * Description  : Frees all the HW entries for the HWACC
 *------------------------------------------------------------------------------
 */
static int fhw_free_hwtbl(FhwHwAccPrio_t hwAccIx)
{
    /* Release the index pool for hwtbl */
    idx_pool_exit(&fhw.hwAcc[hwAccIx].freeIdxPool);

    if (fhw.hwAcc[hwAccIx].hwtbl == NULL)
        return FHW_SUCCESS;

    kfree(fhw.hwAcc[hwAccIx].hwtbl);

    fhw.hwAcc[hwAccIx].hwtbl = NULL;

    return FHW_SUCCESS;
}

static int fhw_clear( uint32_t key, const FlowScope_t scope );

int fhw_unbind_hw(FhwHwAccPrio_t hwAccIx)
{
    dbg_assertr( (hwAccIx < FHW_PRIO_MAX), FHW_ERROR );
    dbgl_print( DBG_EXTIF, "hwAccIx<%d>", hwAccIx );

    if (hwAccIx >= FHW_PRIO_MAX)
        return FHW_ERROR;

    if (fhw.bindCount > 0 && fhw.hwAcc[hwAccIx].hwtbl != NULL)
    {
        fhw_free_hwtbl(hwAccIx); 
        memset(&fhw.hwAcc[hwAccIx].hwHooks, 0, sizeof(FhwBindHwHooks_t));

        fhw.bindCount--;
        fhw_bind_fc(fhw.bindCount);
    }
    else
    {
        printk( CLRbold2 PKTFLOW_MODNAME "HW acceleration already disabled." CLRnl );
    }

    return FHW_SUCCESS;
}


/*
 *------------------------------------------------------------------------------
 * Function     : fhw_bind_hw
 * Description  : Binds the HW hooks to activate, deactivate and refresh / refresh_pathstat.
 *                Passes a pointer to fhw_clear() functions.
 *------------------------------------------------------------------------------
 */
int fhw_bind_hw(FhwHwAccPrio_t hwAccIx, FhwBindHwHooks_t *hwHooks_p)
{
    dbg_assertr( (hwAccIx < FHW_PRIO_MAX), FHW_ERROR );

    if (hwAccIx >= FHW_PRIO_MAX)
    {        
        fc_error("fhw_bind_hw invalid parameter hwAccIx=%d",hwAccIx );   
        goto bind_hw_error;
    }

    if ( hwHooks_p->activate_fn == (HOOKP32) NULL )
    {
        *hwHooks_p->fhw_clear_fn = (FC_CLEAR_HOOK)NULL;
        return fhw_unbind_hw(hwAccIx);
    }

    if (fhw.hwAcc[hwAccIx].hwtbl == NULL)
    {
        if (fhw_alloc_hwtbl(hwAccIx, hwHooks_p->max_ent) != FHW_SUCCESS)
        {
            fc_error("fhw_alloc_hwtbl invalid parameter max_ent=%d",hwHooks_p->max_ent );  
            goto bind_hw_error;
        }

        memcpy(&fhw.hwAcc[hwAccIx].hwHooks, hwHooks_p, sizeof(FhwBindHwHooks_t));

        if ( hwHooks_p->activate_fn != (HOOKP32) NULL )
            *hwHooks_p->fhw_clear_fn = fhw_clear;    /* downcall hook from HWACC */
        else
            *hwHooks_p->fhw_clear_fn = (FC_CLEAR_HOOK)NULL;

        fhw.bindCount++;
        fhw_bind_fc(fhw.bindCount);

        /* Update hw support for active flows
           This is necessary when disabling and reenabling hw acceleration */
        fc_update_hw_support();
    }
    else
    {
        printk( CLRbold PKTFLOW_MODNAME "Hardware acceleration already enabled." CLRnl );
    }

    return FHW_SUCCESS;

bind_hw_error:
    fc_error("fhw_bind_hw invalid parameter" );   
    return FHW_ERROR;
}


/*
 *------------------------------------------------------------------------------
 * Function   : fhw_cur_stats_hw
 * Description: Determine the HW hits from previous invocation
 * Returns    : Number of hits in HW hardware.
 *------------------------------------------------------------------------------
 */
uint32_t fhw_cur_stats_hw(FhwKey_t key, uint32_t *hw_hits_p, 
                          unsigned long long *hw_bytes_p)
{
    FhwHwAccPrio_t hwAccIx = key.id.accl;
    uint32_t entIx;
    HwEnt_t *hwEnt_p = NULL;

    dbgl_print( DBG_CTLFL, "fhw<0x%8x>", key.id.fhw);
    dbg_assertr( (hw_hits_p != NULL), 0 );
    dbg_assertr( (hw_bytes_p != NULL), 0 );

    if ( key.word == FHW_TUPLE_INVALID )     /* caller checks already */
        return 0;

    dbg_assertr( (hwAccIx < FHW_PRIO_MAX), 0);

    entIx = key.id.fhw;
    hwEnt_p = &fhw.hwAcc[hwAccIx].hwtbl[entIx]; 

    dbgl_print( DBG_INTIF, "fhw<0x%8x> hwAccIx<%d>, entIx<%d> hwEnt_p<%px>", 
                           key.id.fhw, hwAccIx, entIx, hwEnt_p);

    *hw_hits_p = hwEnt_p->hw_hits_cumm;
    *hw_bytes_p = hwEnt_p->hw_bytes_cumm;

    dbgl_print( DBG_INTIF, "fhw<0x%8x> hw_hits<%u>, hw_bytes<%u>", 
                           key.id.fhw, 
                           *(uint32_t *)hw_hits_p, *(uint32_t *)hw_bytes_p);
    return (*hw_hits_p);
}


#define FHW_IS_RX_DEV_WAN(blog_p) ( is_netdev_wan((struct net_device *)(blog_p)->rx_dev_p) )
#define FHW_IS_TX_DEV_WAN(blog_p) ( is_netdev_wan((struct net_device *)(blog_p)->tx_dev_p) )
#define FHW_IS_ANY_DEV_WAN(blog_p) ( FHW_IS_RX_DEV_WAN(blog_p) || FHW_IS_TX_DEV_WAN(blog_p) )

/*
 *------------------------------------------------------------------------------
 * Function     : fhw_get_gre_flow_support
 * Description  : Check if the HW support various type of GRE acceleration
 *------------------------------------------------------------------------------
 */
static int fhw_get_gre_flow_support(Blog_t * blog_p ) 
{
#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE)
#if defined(CONFIG_BCM_PON_XRDP)
    /* 
        Support GRE termination:
        the transmit should be different from recieve for perfrom termination
    */
    /* L2 acceleration GRE pass thru case should be accelerated */
    if ((blog_p->rx.info.bmap.PLD_L2 == 1) && (blog_p->rx.info.bmap.GRE == 1) && (blog_p->tx.info.bmap.GRE == 1))
        return 1;

    if (blog_p->grerx.gre_flags.ver == 1 && blog_p->gretx.gre_flags.ver == 1)
        return 1;

    /* GRE terminatio LAN<->LAN/WLAN HW acceleration is not supported */
    if (!FHW_IS_ANY_DEV_WAN(blog_p))
        return 0;

    /* HW Accelertion is supported only if the GRE termination is defined on the WAN */
    if ((RX_GRE(blog_p) && !FHW_IS_RX_DEV_WAN(blog_p)) || (TX_GRE(blog_p) && !FHW_IS_TX_DEV_WAN(blog_p)))
        return 0;

#if defined(PKTRUNNER_IMPL2)
    /* L3GRE_4in4 */
    if (TG4in4DN(blog_p) || TG4in4UP(blog_p))
        return 1;

    /* L2GRE_4in4 */
    if (TG2in4DN(blog_p) || (TG2in4UP(blog_p) && blog_p->l2_ipv4))
        return 1;
#else
    /* L2GRE_4in4 / L3GRE_4in4 */
    if (TG24in4DN(blog_p) || TG24in4UP(blog_p))
        return 1;
#endif    
    /* GRE_tunnel_in_tunnel_out */
    if (TOTG4(blog_p))
        return 1;

    /* L3GRE_4in6 */
    if ((TG4in6DN(blog_p) || TG4in6UP(blog_p)) && !RX_GRE_ETH(blog_p))
        return 1;    

    /* L3GRE_6in4 */
    if ((TG6in4DN(blog_p) || TG6in4UP(blog_p)) && !RX_GRE_ETH(blog_p))
        return 1;     

#else  /* CONFIG_BCM_DSL_XRDP and CONFIG_BCM_DSL_RDP */
    /* 63138/148, 4908, 63158, 63146, 4912, 6813 */
    
    /* L2 acceleration GRE pass thru case should be accelerated */
    if ((blog_p->rx.info.bmap.PLD_L2 == 1) && (blog_p->rx.info.bmap.GRE == 1) && (blog_p->tx.info.bmap.GRE == 1))
        return 1;

    /* GRE Acceleration. No support for GRE flags (C, K, S, etc.) */
    if ((RX_GRE(blog_p) && !TX_GRE(blog_p) && blog_p->grerx.gre_flags.u16) || 
        (!RX_GRE(blog_p) && TX_GRE(blog_p) && blog_p->gretx.gre_flags.u16) ||
        ((TOTG4(blog_p) || TOTG6(blog_p)) && (blog_p->grerx.gre_flags.u16 || blog_p->gretx.gre_flags.u16)))
        return 0;

    /* White list */
    /* Runner support L3GRE_4in4 acceleration */
    if (TG4in4DN(blog_p) || TG4in4UP(blog_p))
        return 1;
    
    /* GRE_tunnel_in_tunnel_out */
    if (TOTG4(blog_p))
        return 1;

    /* L3GRE_4in6 */
    if ((TG4in6DN(blog_p) || TG4in6UP(blog_p)) && !RX_GRE_ETH(blog_p))
        return 1;    

#if defined(CONFIG_BCM_DSL_XRDP)
    /* 63158, 63146, 4912, 6813 */
    /* L3GRE_6in4 */
    if ((TG6in4DN(blog_p) || TG6in4UP(blog_p)) && !RX_GRE_ETH(blog_p))
        return 1;

    /* L2GRE_4in4 */
    if (TG2in4DN(blog_p) || (TG2in4UP(blog_p) && blog_p->l2_ipv4))
        return 1;

#elif defined(CONFIG_BCM_DSL_RDP)
    /* 63138, 63148, 4908 */
    /* L2GRE_4in4 */
    if (TG2in4DN(blog_p) || (TG2in4UP(blog_p) && blog_p->l2_ipv4))
    {
        /* non-TCP protocol */
        if (blog_p->key.protocol != BLOG_IPPROTO_TCP)
            return 1;
        
        /* TCP protocol && TCP_PureACK disabled */
        else if (blog_support_tcp_ack_mflows_g == 0)
            return 1;
        
        /* L2GRE_4in4 TCP && TCP_PureACK enabled */
        else
            return 0;
    }
#endif

#if defined(CONFIG_BCM963146) || defined(CONFIG_BCM94912) || defined(CONFIG_BCM96813)    
    /* L2/L3GRE_6in6 origination can be supported due to two-flow natc lookup extension */
    /* L2/L3GRE_6in6 termination can NOT be supported due to packet-buffer size limitation */
    if (TG6in4UP(blog_p) || TG4in6UP(blog_p) || TG6in6UP(blog_p))
        return 1;
#endif    
    
#endif
#endif

#if defined(CONFIG_BCM_ARCHER) || defined(CONFIG_BCM_ARCHER_MODULE)
    /* L2 acceleration GRE pass thru case should be accelerated */
    if ((blog_p->rx.info.bmap.PLD_L2 == 1) && (blog_p->rx.info.bmap.GRE == 1) && (blog_p->tx.info.bmap.GRE == 1))
        return 1;

    /* GRE Acceleration. No support for GRE flags (C, K, S, etc.) */
    if ((RX_GRE(blog_p) && !TX_GRE(blog_p) && blog_p->grerx.gre_flags.u16) || 
        (!RX_GRE(blog_p) && TX_GRE(blog_p) && blog_p->gretx.gre_flags.u16) ||
        ((TOTG4(blog_p) || TOTG6(blog_p)) && (blog_p->grerx.gre_flags.u16 || blog_p->gretx.gre_flags.u16)))
        return 0;

    /* White list */
    /* Runner support L3GRE_4in4 acceleration */
    if (TG4in4DN(blog_p) || TG4in4UP(blog_p))
        return 1;
    
    /* L2GRE_4in4 */
    if (TG2in4DN(blog_p) || (TG2in4UP(blog_p) && blog_p->l2_ipv4))
        return 1;
    
    /* GRE_tunnel_in_tunnel_out */
    if (TOTG4(blog_p))
        return 1;

    /* L2GRE_4in6 and L3GRE_4in6 */
    if (TG4in6DN(blog_p) || TG4in6UP(blog_p))
        return 1;

    /* L2GRE_6in4 and L3GRE_6in4 */
    if (TG6in4DN(blog_p) || TG6in4UP(blog_p))
        return 1;

    /* L2/L3GRE_6in6 */
    if (TG6in6DN(blog_p) || TG6in6UP(blog_p))
        return 1;
#endif

    return 0;
}

static int fhw_is_L2L_supported( void )
{
#if (defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE))
    /* On some Runner based platforms Ethernet LAN-to-LAN unicast flows are not accelerated in runner. */
#if defined(CONFIG_BCM_PON) || defined(CONFIG_BCM963146) || defined(CONFIG_BCM94912) || defined(CONFIG_BCM96813)
     /* Depends on port configuration. We promote this flow to runner in any case.
        Runner will check on runtime the dst or src port configuration and decide to use FC or bridge logic */
     return 1;
#else
     /* 63138/63148 - Runner FW does not support and expects integration L2 switch will take care */
     return 0;
#endif
#endif
     return 1;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fhw_is_xoa_flw
 * Description  : Return true if this is a ATM flow. HW acceleration is not
 *                possible currently for XoA flows on Runner platforms.
 *                Examples are PPPoA, IPoA.
 *------------------------------------------------------------------------------
 */
static inline int fhw_is_xoa_flw(Blog_t *blog_p)
{
#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE)
     struct net_device *xtm_dev_p = NULL;

     if (blog_p->rx.info.bmap.BCM_XPHY)
     {
         xtm_dev_p = blog_p->rx_dev_p;
     }
     else if (blog_p->tx.info.bmap.BCM_XPHY)
     {
         xtm_dev_p = blog_p->tx_dev_p;
     }
     if (xtm_dev_p)
     {
         uint32_t phyType = netdev_path_get_hw_port_type(xtm_dev_p); 

         phyType = BLOG_GET_HW_ACT(phyType);
         if ( (phyType == VC_MUX_PPPOA) ||
              (phyType == VC_MUX_IPOA) ||
              (phyType == LLC_SNAP_ROUTE_IP) ||
              (phyType == LLC_ENCAPS_PPP) )
         {
             return 1;
         }
     }
#endif
     return 0;
}


/* Below compile flag disables the Multicast Acceleration of WLAN traffic
 * This flag is introduced to keep the backward compatibility with earlier
 * releases (where WLAN multicast was never accelerated in HW on DSL platforms). */
#if 0
#define BCM_DISABLE_WLAN_MCAST_HW_ACCELERATION
#endif

//static void inline fhw_set_hybrid(uint32_t *hybrid_p)
void inline fhw_set_hybrid(uint32_t *hybrid_p)
{
#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)
#if !defined(CONFIG_BCM_PON)
    *hybrid_p = 1;
#endif
#endif
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fhw_chk_support_hw_hybrid
 * Description  : Exported fcache API to determine whether "Hybrid" HW acceleration
 *                is supported for this flow. "Hybrid" HW acceleration means
 *                that HW is only performing the flow classification but
 *                modification & forwarding is done by flow-cache.
 *------------------------------------------------------------------------------
 */
int fhw_chk_support_hw_hybrid( Blog_t * blog_p )
{
#if defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS)

    /* Add specific check for each traffic scenario that are tested and add */

#endif /* defined(CONFIG_BCM_FCACHE_CLASSIFICATION_BYPASS) */
    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fhw_chk_support_hw
 * Description  : Exported fcache API to determine whether HW acceleration
 *                is supported.
 *------------------------------------------------------------------------------
 */
int fhw_chk_support_hw( Blog_t * blog_p, uint32_t *out_hybrid_p )
{

    *out_hybrid_p = 0;
    /* No HW Accelerator platforms */

    if (!g_fhw_hw_accel_enabled)
    {
        fhw.stats.hwSup.disable++;
        return 0;
    }

    /* Force disable WLAN multicast acceleration - local #define */
#if defined(BCM_DISABLE_WLAN_MCAST_HW_ACCELERATION)
    if (blog_p->rx.multicast && blog_p->tx.info.phyHdrType == BLOG_WLANPHY)
    {
        fhw.stats.hwSup.disable++;
        return 0;
    }
#endif

#if defined(CONFIG_BCM_PON)

    /* IPv6 over IPv4 tunnel pass through is not supported by NP PON FW */  
    if ( RX_IP6in4(blog_p) )
    {
        fhw.stats.hwSup.T6in4++;
        return 0;
    }

    /* sdnat is not supported by NP PON FW */ 
    if (blog_p->rx.tuple.saddr != blog_p->tx.tuple.saddr &&
        blog_p->rx.tuple.daddr != blog_p->tx.tuple.daddr && !(MAPT(blog_p)) && !(RX_GRE(blog_p) || TX_GRE(blog_p)))
    {
        fhw.stats.hwSup.sdnat++;
        return 0;
    }

    /* dnat between two LAN  subnets is not supported by NP PON FW */
    if (!FHW_IS_RX_DEV_WAN(blog_p) &&
        blog_p->rx.tuple.daddr != blog_p->tx.tuple.daddr && !(MAPT(blog_p)) && !(RX_GRE(blog_p) || TX_GRE(blog_p)))
    {
        fhw.stats.hwSup.sdnat++;
        return 0;
    }
#endif

    /* MAP-T and GRE on lan/WLAN side combination flows not HW accelerated */
    if (MAPT(blog_p) && (RX_GIP4in4(blog_p) || TX_GIP4in4(blog_p)))
    {
        fhw.stats.hwSup.mapt_gre++;
        return 0;
    }

    /* T4in6 and GREv4/6 on lan/WLAN side combination flows not HW accelerated */
    if (CHK4in6(blog_p) && (RX_GIP4in4(blog_p) || TX_GIP4in4(blog_p) || RX_GIP4in6(blog_p) || TX_GIP4in6(blog_p)))
    {
        fhw.stats.hwSup.t4in6_gre++;
        return 0;
    }

    /* T6in4 and GREv4/6 on lan/WLAN side combination flows not HW accelerated */
    if (CHK6in4(blog_p) && (RX_GIP6in4(blog_p) || TX_GIP6in4(blog_p) || RX_GIP6in4(blog_p) || TX_GIP6in6(blog_p)))
    {
        fhw.stats.hwSup.t6in4_gre++;
        return 0;
    }

    /* Any flow to/from WLAN should only be HW accelerated
       if "is_xx_hw_acc_en" is set for corresponding interface */
    if ( BLOG_IS_TX_HWACC_ENABLED_WLAN_PHY(blog_p->tx.info.phyHdrType) && !blog_p->wl_hw_support.is_tx_hw_acc_en )         
    {
        /* TODO : Hybrid possible when TX is not offloaded as well */
        fhw.stats.hwSup.wltx++;
        return 0;
    }
    if ( blog_p->rx.info.phyHdrType == BLOG_WLANPHY && !blog_p->wl_hw_support.is_rx_hw_acc_en ) 
    {
        /* TODO : Hybrid possible when TX is not offloaded as well */
        fhw.stats.hwSup.wlrx++;
        return 0;
    }

    /* Local terminated TCP flows */
    if (blog_p->tx.info.phyHdrType == BLOG_TCP4_LOCALPHY) 
    {
        /* TODO : Hybrid possible; need adjustments in pktrunner */
        fhw.stats.hwSup.localTcp++;
        return 0;
    }

    /* WLAN EXTRA PHYs*/
    if ( BLOG_IS_HWACC_DISABLED_WLAN_EXTRAPHY(blog_p->rx.info.phyHdrType,blog_p->tx.info.phyHdrType)) 
    {
        fhw.stats.hwSup.wltx++;
        return 0;
    }

    /* L2TP don't activate flow in HW */
    if ( TX_L2TP(blog_p) || RX_L2TP(blog_p) ) 
    {
        /* TODO : Hybrid possible when TX_L2TP and !RX_L2TP */
        fhw.stats.hwSup.l2tp++;
        return 0;
    }

    /* PPTP don't activate flow in HW */
    if( TX_PPTP(blog_p) || RX_PPTP(blog_p) )
    {
        /* TODO : Hybrid? What is PPTP? */
        fhw.stats.hwSup.pptp++;
        return 0;
    }

    if( TX_ESP(blog_p) || RX_ESP(blog_p) )
    {
        /* if the packet is originated from or destined to a BLOG_SPU device,
           it's not an ESP passthru packet and therefore skip fcache in hw. */
        if ( (blog_p->rx.info.phyHdrType == BLOG_SPU_DS) ||
             (blog_p->rx.info.phyHdrType == BLOG_SPU_US) ||
             (blog_p->tx.info.phyHdrType == BLOG_SPU_DS) ||
             (blog_p->tx.info.phyHdrType == BLOG_SPU_US) )
        {
            /* TODO : Hybrid possible when TX_ESP and !RX_ESP */
            fhw.stats.hwSup.esp++;
            return 0;
        }
        else 
        {
#if defined(CONFIG_BCM_PON)
            return 1;
#else
            /* For DSL platforms, accelerate L2 IPSEC ESP pass-through flow only */
            if ((blog_p->rx.info.bmap.PLD_L2 == 1) && TX_ESP(blog_p) && RX_ESP(blog_p) )
                return 1;
            else
            {
                fhw.stats.hwSup.esp++;
                return 0;
            }
#endif
        }
    }
    /* HW acceleration is possible only for RX port directly connected to accelerators (Runner) */
    if ( blog_p->rx.info.bmap.BCM_SWC == 0   &&     /* Not a Switch/Runner port : ENET, GPON, EPON */
         blog_p->rx.info.bmap.BCM_XPHY == 0         /* Not a XTM Port */
#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE) /* Only Runner can receive WLAN packet directly */
         && (blog_p->rx.info.phyHdrType != BLOG_WLANPHY)        /* Dongle-Offload and WLAN-RX-ACCELERATION */
         && (blog_p->rx.info.phyHdrType != BLOG_NETXLPHY)        /* Dongle-Offload and WLAN-RX-ACCELERATION */
#endif
#if defined(CONFIG_BCM_ARCHER_WLAN) || IS_ENABLED(CONFIG_BCM_DHD_ARCHER)
         && (blog_p->rx.info.phyHdrType != BLOG_WLANPHY)        /* Archer WLAN-RX-ACCELERATION */
#endif
        )
    {
        fhw.stats.hwSup.hwPort++;
        return 0;
    }

    if ((blog_p->rx.info.phyHdrType == BLOG_SPDTST || blog_p->tx.info.phyHdrType == BLOG_SPDTST) && 
            !blog_p->spdtst_bits.is_hw)
    {
        fhw.stats.hwSup.spdTst++;
        return 0;
    }

    /* Runner acceleration of ATM flows is not supported */
    if (fhw_is_xoa_flw(blog_p))
    {
        fhw.stats.hwSup.xoa++;
        return 0;
    }

    /* HW acceleration of flows to USB not supported; RX already covered in above condition */
    if ( blog_p->tx.info.phyHdrType == BLOG_USBPHY )
    {
        /* TODO : Hybrid? What does USBPHY mean? */
        fhw.stats.hwSup.usb++;
        return 0;
    }

    /* HW acceleration of flows from/to LTE WAN not supported */
    if ( (blog_p->rx.info.phyHdrType == BLOG_LTEPHY) || (blog_p->tx.info.phyHdrType == BLOG_LTEPHY) )
    {
        /* TODO : TX_LTE and !RX_LTE */
        fhw.stats.hwSup.lte++;
        return 0;
    }

    if ( !blog_p->rx.multicast ) /* Unicast flows */
    {
        /* Ethernet LAN-to-LAN flows */
        if ( (blog_p->rx.info.phyHdrType == BLOG_ENETPHY && blog_p->tx.info.phyHdrType == BLOG_ENETPHY) && /* ENET-to-ENET */
             !FHW_IS_ANY_DEV_WAN(blog_p) )  /* None is WAN i.e. both are LAN */
        {

            if ( !fhw_is_L2L_supported() )
            {
                /* TODO: Need change in PKT_RUNNER */
                fhw.stats.hwSup.l2l++;
                return 0;
            }
        }
    }
    else
    {
        if (blog_p->rtp_seq_chk)
        {
            /* TODO : Hybrid possible when multicast-hybrid gets supported */
            fhw.stats.hwSup.mcRtpSeq++;
            return 0;
        }

        if (TX_GRE(blog_p) || RX_GRE(blog_p))
        {
            /* Multicast GRE Hardware acceleration not supported */
            fhw.stats.hwSup.gre++;
            return 0;
        }

#if !IS_ENABLED(CONFIG_BCM_ARCHER)
        if ( !FHW_IS_RX_DEV_WAN(blog_p) ) /* Multicast RX from non-WAN Port */
        {
#if defined(CONFIG_BCM_DSL_RDP)
            /* Roll back to no LAN/WLAN to LAN/WLAN multicast support in RDP as in previous releases.
             * Due to JIRA SWBCACPE-43532 and SWBCACPE-43599 */
            fhw.stats.hwSup.l2l++;
            return 0;
#endif /*CONFIG_BCM_DSL_RDP */

            /* XRDP platforms must NOT have any limitations; similar to Archer but for now... */
            /* All XRDP platforms currently do not support multicast from WLAN (RX packet in DDR)
             * DSL is mostly WAN, except DPU; No requirement to support DSL to GPON Mcast for DPU
             * GPON/EPON are always WAN -- no requirement to support upstream towards xPON Multicast */
#if defined(CONFIG_BCM_XRDP)
            /* Don't allow from non-Ethernet LAN interfaces for now */            
            if (blog_p->rx.info.phyHdrType != BLOG_ENETPHY)
            {
                fhw.stats.hwSup.l2l++;
                return 0;
            }
#endif
        }
#endif
    }
    /* GRE Flows -- check platform specific capabilities */
    if ( ((blog_p->rx.info.bmap.GRE == 1) || (blog_p->tx.info.bmap.GRE == 1)) && !fhw_get_gre_flow_support(blog_p) )
    {
        /* fhw_set_hybrid(out_hybrid_p); */
        /* TODO - Enable to test GRE-hybrid */
        fhw.stats.hwSup.gre++;
        return 0;
    }

#if !defined(CONFIG_BCM963158) && !IS_ENABLED(CONFIG_BCM_ARCHER)
    /* WAN to WAN flows */
    if ( FHW_IS_RX_DEV_WAN(blog_p) &&  FHW_IS_TX_DEV_WAN(blog_p) )
    {
        /* WAN to WAN hardware acceleration not supported on other platforms */
        if (blog_p->tx.info.phyHdrType != BLOG_SPDTST)
        {
            fhw.stats.hwSup.w2w++;
            return 0;
        }
    }
#endif

#if ((!defined(CONFIG_BCM_XRDP) || !defined(CONFIG_BCM_PON)) && !defined(RDP_UFC)) 
    if ((blog_p->rx.info.bmap.LLC_SNAP == 1) || (blog_p->tx.info.bmap.LLC_SNAP == 1)) 
    {
        fhw.stats.hwSup.llcsnap++;
        return 0;
    }
#else
    if ((blog_p->rx.info.bmap.LLC_SNAP != blog_p->tx.info.bmap.LLC_SNAP))
    {
        fhw.stats.hwSup.llcsnap++;
        return 0; /* rx.llc_snap != tx.llc_snap  case */
    }

    if ((blog_p->rx.info.bmap.LLC_SNAP && blog_p->tx.info.bmap.LLC_SNAP) &&
       (blog_p->tx.llc_snap.len_delta || blog_p->tx.info.phyHdrType == BLOG_WLANPHY))
    {
        fhw.stats.hwSup.llcsnap++;
        return 0; /* not LLC/SNAP passthrough case (frame length are different) or TX to WLAN */
    }
#endif

#if (!defined(CONFIG_BCM_XRDP) || !defined(CONFIG_BCM_PON))
    if (VXLAN(blog_p))
    {
        fhw.stats.hwSup.vxlan++;
        return 0;
    }
#endif

#if (defined(CONFIG_BCM_DSL_RDP))
    /* Passthru 3rd/4th VLAN tags not supported */
    if (blog_p->vtag_num > 2 || blog_p->vtag_tx_num > 2)
    {
        fhw.stats.hwSup.vtagNum++;
        return 0;
    }
#endif

    return 1;
}

static uint32_t fhw_flow_type(Blog_t *blog_p)
{
    BlogInfo_t * bInfo_p;
    uint32_t flow_type = HW_CAP_NONE;
    
    bInfo_p = &blog_p->rx.info;

    if ((bInfo_p->bmap.PLD_L2 == 1) && (blog_p->rx.multicast != 1))
        flow_type = HW_CAP_L2_UCAST;
    else if (blog_p->tupleV6.tunnel == 1)
        flow_type = HW_CAP_IPV6_TUNNEL;
    else if (bInfo_p->bmap.PLD_IPv4 == 1)
    {
        if (blog_p->rx.multicast == 1)
            flow_type = HW_CAP_IPV4_MCAST;
        else
            flow_type = HW_CAP_IPV4_UCAST;
    }
    else if (bInfo_p->bmap.PLD_IPv6 == 1)
    {
        if (blog_p->rx.multicast == 1)
            flow_type = HW_CAP_IPV6_MCAST;
        else
            flow_type = HW_CAP_IPV6_UCAST;
    }

    return flow_type;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_is_hw_cap_enabled
 * Description: Checks if all the HW capabilities in mask are enabled (subset)
 *              for the HW accelerators.
 * Return Val : 1 if the requested HW cap are subset of the HW Accelerators cap.
 *            : 0 otherwise.
 *------------------------------------------------------------------------------
 */
uint32_t fhw_is_hw_cap_enabled(uint32_t cap_mask)
{
    FhwHwAccPrio_t hwIx;

    if (!cap_mask)
        return 0;

    for (hwIx = FHW_PRIO_0; hwIx < FHW_PRIO_MAX; hwIx++) 
    {        
        if ((fhw.hwAcc[hwIx].hwHooks.cap & cap_mask) == cap_mask)
            return 1;
    }

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_get_hw_accel_avail
 * Description: Get whether HW acceleration is enabled and hooks bound to fcache.
 * Return Val : 0(No), 1(Yes)
 *------------------------------------------------------------------------------
 */
int fhw_get_hw_accel_avail(void)
{
    return fhw.bindCount && g_fhw_hw_accel_enabled;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_set_hw_accel
 * Description: Enable/Disable acceleration of flows by HW.
 * Return Val : Always 1
 *------------------------------------------------------------------------------
 */
uint32_t fhw_set_hw_accel(uint32_t enable)
{
    g_fhw_hw_accel_enabled = enable ? 1 : 0;
    return 1;
}
/*
 *------------------------------------------------------------------------------
 * Function   : fhw_get_hw_accel
 * Description: Get current status of Enable/Disable acceleration of flows by HW.
 * Return Val : 0(Disable), 1(Enable)
 *------------------------------------------------------------------------------
 */
uint32_t fhw_get_hw_accel(void)
{
    return g_fhw_hw_accel_enabled;
}

static uint32_t _fhw_activate_hw(uint32_t flow_type, uint32_t flowIx, unsigned long blog,
                                 unsigned long new_flow, unsigned long key_in)
{
    FhwHwAccPrio_t hwIx, hwAccIx;
    Blog_t *blog_p = (Blog_t *) blog; 
    FhwKey_t fhwKey_in = *(FhwKey_t*)(&key_in);

    dbgl_print( DBG_EXTIF, "flowIx<%d> blog_p<0x%px>", flowIx, blog_p );

    for (hwIx = FHW_PRIO_0; hwIx < FHW_PRIO_MAX; hwIx++) 
    {        
        hwAccIx = fhw.cap2HwAccMap[flow_type][hwIx];
        if (hwAccIx >= FHW_PRIO_MAX)
            break;

        /*
         * HW accelerator which cannot accelerate mcast traffic 
         * returns 0xFFFFFFFF for mcast configuration
         */
        if ( likely(fhw.hwAcc[hwAccIx].hwHooks.activate_fn != (HOOKP32)NULL) )
        {
            uint32_t tuple = FHW_TUPLE_INVALID;
            uint32_t tuple_in = FHW_TUPLE_INVALID;
            int hwTblIdx = 0; /* signed int to catch the error code */

            if ( !(fhw.hwAcc[hwAccIx].hwHooks.cap & (1<<flow_type) ) )
                continue;

            /* First allocate an index to store the mapping table - only for new_flow
             * when multicast client is added, new_flow is not set */

            if (fhwKey_in.word == FHW_TUPLE_INVALID)
            {
                hwTblIdx = idx_pool_get_index(&fhw.hwAcc[hwAccIx].freeIdxPool);
                if (hwTblIdx < 0)
                {
                    /* No available HW flows available */
                    fhw.stats.actFail++;
                    return FHW_TUPLE_INVALID;
                }
            }
            else
            {
                fc_error("Unsupported mode -- can't reuse FHW key; Will remove key_in parameter to this function\n");
                fhw.stats.actFail++;
                return FHW_TUPLE_INVALID;
            }
            /* Do upcall into HW to activate a learnt connection */
            tuple = fhw.hwAcc[hwAccIx].hwHooks.activate_fn( (void*)blog_p, tuple_in );

            if ( tuple != FHW_TUPLE_INVALID )
            {
                FhwKey_t key = {};
                HwEnt_t *hwEnt_p;

                if (tuple_in != FHW_TUPLE_INVALID && tuple_in != tuple)
                {
                    fc_error("tuple_in <0x%08x> tuple <0x%08x> not same\n",tuple_in, tuple);
                }
                hwEnt_p = &fhw.hwAcc[hwAccIx].hwtbl[hwTblIdx];

                /* hwEnt_p stores the actual HW-Flow-Key */
                hwEnt_p->hw_key = tuple;
                hwEnt_p->hw_hits_curr = 0;
                hwEnt_p->hw_hits_cumm = 0;
                hwEnt_p->hw_bytes_curr = 0;
                hwEnt_p->hw_bytes_cumm = 0;
                hwEnt_p->flow_type = flow_type;
                /* Build the FHW key for flow-cache flow */
                key.id.accl = hwAccIx;
                key.id.fhw = hwTblIdx;
                dbgl_print( DBG_EXTIF, "new flowIx<%d> fhw<%u:%u> hw<0x%8x>", flowIx, key.id.accl, key.id.fhw, tuple);
                fhw.stats.actSuccess++;
                return key.word;
            }
            /* HW Flow activation failed - return the index back to pool */
            idx_pool_return_index(&fhw.hwAcc[hwAccIx].freeIdxPool, hwTblIdx);
        }
    }

    fhw.stats.actFail++;
    return FHW_TUPLE_INVALID;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_activate_hw
 * Description: Activates a flow in hardware HW via activate_fn.
 *------------------------------------------------------------------------------
 */
static uint32_t fhw_activate_hw(uint32_t flowIx, unsigned long blog, unsigned long new_flow,
                                unsigned long key_in)
{
    Blog_t *blog_p = (Blog_t *) blog; 
    uint32_t flow_type = fhw_flow_type(blog_p);

    dbgl_print( DBG_EXTIF, "flowIx<%d> blog_p<0x%px>", flowIx, blog_p );
    return _fhw_activate_hw(flow_type, flowIx, blog, new_flow, key_in);
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_deactivate_hw
 * Description: Deactivates a flow in hardware HW via deactivate_fn.
 *------------------------------------------------------------------------------
 */
static int fhw_deactivate_hw(uint32_t flowIx, FhwKey_t key, unsigned long blog)
{
    FhwHwAccPrio_t hwAccIx = key.id.accl;
    struct blog_t * blog_p = (struct blog_t *) blog;
    int active = 0;

    dbgl_print( DBG_EXTIF, "flowIx<%d> fhw<0x%8x> blog_p<0x%px>", 
        flowIx, key.id.fhw, blog_p );

    if ( unlikely(key.word == FHW_TUPLE_INVALID) )
        return active;

    dbg_assertr( (hwAccIx < FHW_PRIO_MAX), 0 );

    if ( likely(fhw.hwAcc[hwAccIx].hwHooks.deactivate_fn != (HOOK4PARM)NULL) )
    {
        const uint32_t entIx = key.id.fhw;
        HwEnt_t *hwEnt_p = &fhw.hwAcc[hwAccIx].hwtbl[entIx]; 
        uint32_t prev_octets;
        uint32_t prev_hits;

        prev_octets = hwEnt_p->hw_bytes_curr;
        prev_hits = hwEnt_p->hw_hits_curr;

        /* Do upcall into HW to deactivate a learnt connection */
        /*
         * For HW multicast acceleration feature, hwHooks.deactivate_fn
         * must return the number of active ports remaining in HW entry.
         */
        dbgl_print( DBG_EXTIF, "remove flowIx<%d> fhw<%u:%u> hw<0x%8x>", flowIx, key.id.accl, key.id.fhw, hwEnt_p->hw_key);
        active = fhw.hwAcc[hwAccIx].hwHooks.deactivate_fn( 
                                                hwEnt_p->hw_key,
                                                (unsigned long)(&hwEnt_p->hw_hits_curr),
                                                (unsigned long)(&hwEnt_p->hw_bytes_curr),
                                                (unsigned long)blog_p );

        hwEnt_p->hw_hits_cumm += calcHwPktHitDiff(hwEnt_p->hw_hits_curr, prev_hits);
        hwEnt_p->hw_bytes_cumm += calcHwByteHitDiff(hwEnt_p->hw_bytes_curr, prev_octets);

        idx_pool_return_index(&fhw.hwAcc[hwAccIx].freeIdxPool, key.id.fhw);
        hwEnt_p->hw_key = FHW_TUPLE_INVALID;
    }

    dbgl_print( DBG_EXTIF, "flowIx<%d> active<%d>", flowIx, active );
    fhw.stats.deactSuccess++;
    return active;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_update_hw
 * Description: Updates a flow in hardware via update_fn.
 *------------------------------------------------------------------------------
 */
static int fhw_update_hw(uint32_t update, FhwKey_t key, unsigned long blog)
{
    FhwHwAccPrio_t hwAccIx = key.id.accl;

    dbgl_print( DBG_EXTIF, "update<%d> fhw<0x%8x> blog_p<0x%px>", 
                update, key.id.fhw, (struct blog_t *)blog );

    dbg_assertr( (key.word != FHW_TUPLE_INVALID), 0 );
    dbg_assertr( (hwAccIx < FHW_PRIO_MAX), 0 );

    if ( likely(fhw.hwAcc[hwAccIx].hwHooks.update_fn != (HOOK3PARM)NULL) )
    {
        /* Do upcall into HW to update a learnt connection */
        fhw.hwAcc[hwAccIx].hwHooks.update_fn((uint32_t)update, fhw.hwAcc[hwAccIx].hwtbl[key.id.fhw].hw_key, blog);
    }

    fhw.stats.updSuccess++;
    return FHW_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_refresh_hw
 * Description: Determine the HW hits from previous invocation
 * Returns    : Number of hits in HW hardware.
 *------------------------------------------------------------------------------
 */
static uint32_t fhw_refresh_hw(FhwKey_t key, unsigned long hw_hits, 
                               unsigned long hw_bytes)
{
    FhwHwAccPrio_t hwAccIx = key.id.accl;
    dbg_assertr( (hwAccIx < FHW_PRIO_MAX), 0);

    if ( likely(fhw.hwAcc[hwAccIx].hwHooks.refresh_fn != (HOOK3PARM)NULL) )
    {
        uint32_t *hw_hits_p = (uint32_t *) hw_hits; 
        uint32_t *hw_bytes_p = (uint32_t *) hw_bytes; 
        uint32_t prev_hits;
        uint32_t prev_octets;
        uint32_t entIx = key.id.fhw;
        HwEnt_t *hwEnt_p = &fhw.hwAcc[hwAccIx].hwtbl[entIx]; 

        prev_hits = hwEnt_p->hw_hits_curr;
        prev_octets = hwEnt_p->hw_bytes_curr;

        if (hwEnt_p->hw_key == FHW_TUPLE_INVALID)
        {
            fc_error("invalid hw_key: key<0%08x> accel<%d> entIx<%d> hw_key<0x%8x>", 
                    key.word, hwAccIx, entIx, hwEnt_p->hw_key);
            fhw.stats.refreshFail++;
            return 0;
        }

        /* Do upcall into HW to fetch hit count of a learnt connection */
        fhw.hwAcc[hwAccIx].hwHooks.refresh_fn(hwEnt_p->hw_key,
                                    (unsigned long)(&hwEnt_p->hw_hits_curr),
                                    (unsigned long)(&hwEnt_p->hw_bytes_curr) );

        *hw_hits_p = calcHwPktHitDiff(hwEnt_p->hw_hits_curr, prev_hits);
        *hw_bytes_p = calcHwByteHitDiff(hwEnt_p->hw_bytes_curr, prev_octets);
        hwEnt_p->hw_bytes_cumm += *hw_bytes_p;
        hwEnt_p->hw_hits_cumm += *hw_hits_p;
        fhw.stats.refreshSuccess++;
        return *hw_hits_p;
    }
    else
    {
        fhw.stats.refreshFail++;
        return 0;
    }
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_refresh_pathstat_hw
 * Description: Determine the HW hits from previous invocation
 * Returns    : Number of hits in HW hardware.
 *------------------------------------------------------------------------------
 */
static void fhw_refresh_pathstat_hw(uint8_t pathstat_idx, uint32_t *hw_hits_p, 
    uint32_t *hw_bytes_p)
{
    *hw_hits_p = 0;
    *hw_bytes_p = 0;    

    if ( likely(fhw.hwAcc[FHW_PRIO_0].hwHooks.refresh_pathstat_fn != (HOOK3PARM)NULL) )
    {
        /* Do upcall into HW to fetch hit count of a learnt connection */
        fhw.hwAcc[FHW_PRIO_0].hwHooks.refresh_pathstat_fn( pathstat_idx, 
                               (uintptr_t)(hw_hits_p), (uintptr_t)(hw_bytes_p) );
    }
    return;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_get_path_num
 * Description: Report number of hw pathstat resource available in hwacc
 * Returns    : Number of pathstat counters in HW hardware.
 *------------------------------------------------------------------------------
 */
static uint32_t fhw_get_path_num(void)
{
    return fhw.hwAcc[FHW_PRIO_0].hwHooks.max_hw_pathstat;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_stats_hw
 * Description: Determine the cumulative HW hits
 * Returns    : Number of hits in HW hardware.
 *------------------------------------------------------------------------------
 */
uint32_t fhw_stats_hw(uint32_t flowIx, FhwKey_t key, unsigned long hw_hits_p, 
    unsigned long long *hw_bytes_p)
{
    fhw_cur_stats_hw(key, (uint32_t *)hw_hits_p, 
                          (unsigned long long *)hw_bytes_p);

    dbgl_print( DBG_EXTIF, "flowIx<%d> fhw<0x%8x> "
                           "tot_hw_hits<%u>, tot_hw_bytes<%u>", 
                           flowIx, key.id.fhw, 
                           *(uint32_t *)hw_hits_p, *(uint32_t *)hw_bytes_p);
    return (*(uint32_t *) hw_hits_p);
}


/*
 *------------------------------------------------------------------------------
 * Function   : fhw_reset_stats_hw
 * Description: Reset the stats of the flow
 * Returns    : 
 *------------------------------------------------------------------------------
 */
uint32_t fhw_reset_stats_hw(uint32_t flowIx, FhwKey_t key)
{
    FhwHwAccPrio_t hwAccIx = key.id.accl;

    if ( key.word == FHW_TUPLE_INVALID )     /* caller checks already */
        return 0;

    dbg_assertr( (hwAccIx < FHW_PRIO_MAX), 0);

    if ( likely(fhw.hwAcc[hwAccIx].hwHooks.reset_stats_fn != (HOOK32)NULL) )
    {
        uint32_t entIx = key.id.fhw;
        HwEnt_t *hwEnt_p = &fhw.hwAcc[hwAccIx].hwtbl[entIx]; 

        /* Do upcall into HW to fetch hit count of a learnt connection */
        fhw.hwAcc[hwAccIx].hwHooks.reset_stats_fn(hwEnt_p->hw_key);

        hwEnt_p->hw_hits_cumm = 0;
        hwEnt_p->hw_hits_curr = 0;
        hwEnt_p->hw_bytes_curr = 0;
        hwEnt_p->hw_bytes_cumm = 0;
    }

    return 0;
}


/*
 *------------------------------------------------------------------------------
 * Function   : fhw_clear_hw
 * Description: Clears a flow in hardware HW via hwHooks.clear_fn.
 *------------------------------------------------------------------------------
 */
static void fhw_clear_hw(FhwKey_t key)
{
    FhwHwAccPrio_t hwAccIx = key.id.accl;

    if ( key.word == FHW_TUPLE_INVALID )     /* caller checks already */
        return;

    dbg_assertv( (hwAccIx < FHW_PRIO_MAX) );

    if ( likely(fhw.hwAcc[hwAccIx].hwHooks.clear_fn != (HOOK32)NULL) )
    {
        /* Do upcall into HW to clear an evicted connection */
        fhw.hwAcc[hwAccIx].hwHooks.clear_fn(fhw.hwAcc[hwAccIx].hwtbl[key.id.fhw].hw_key);
    }
    return;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_get_hw_host_mac_mgmt_avail
 * Description: Get whether HW host MAC managment hooks bound to fcache.
 * Return Val : 0(No), 1(Yes)
 *------------------------------------------------------------------------------
 */
int fhw_get_hw_host_mac_mgmt_avail(void)
{
    return fhw.bindCount && fhw.hwAcc[FHW_PRIO_0].hwHooks.add_host_mac_fn != (HOOKP)NULL &&
        fhw.hwAcc[FHW_PRIO_0].hwHooks.del_host_mac_fn != (HOOKP)NULL;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_add_host_mac_hw
 * Description: Adds a host MAC address in hardware HW via add_host_mac_fn.
 * Design Note  : Assuming hook is bound to first HW accelerator.
 *------------------------------------------------------------------------------
 */
static int fhw_add_host_mac_hw(char *mac_p)
{
    /* Do upcall into HW to add host MAC address */
    if ( fhw.hwAcc[FHW_PRIO_0].hwHooks.add_host_mac_fn == (HOOKP)NULL ||
         fhw.hwAcc[FHW_PRIO_0].hwHooks.add_host_mac_fn( mac_p ) != 0 )
    {
        fhw.stats.macAddFail++;
        return FCACHE_ERROR;
    }

    fhw.stats.macAddSuccess++;
    return FCACHE_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_del_host_mac_hw
 * Description: Deletes a host MAC address in hardware HW via del_host_mac_fn.
 * Design Note  : Assuming hook is bound to first HW accelerator.
 *------------------------------------------------------------------------------
 */
static int fhw_del_host_mac_hw(char *mac_p)
{
    /* Do upcall into HW to delete host MAC address */
    if ( fhw.hwAcc[FHW_PRIO_0].hwHooks.del_host_mac_fn == (HOOKP)NULL ||
         fhw.hwAcc[FHW_PRIO_0].hwHooks.del_host_mac_fn( mac_p ) != 0 )
    {
        fhw.stats.macDelFail++;
        return FCACHE_ERROR;
    }

    fhw.stats.macDelSuccess++;
    return FCACHE_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * Function   : fhw_get_hw_tuple
 * Description: returns the hw_tuple corresponding to the Fhw_Key.
 *------------------------------------------------------------------------------
 */
static uint32_t fhw_get_hw_tuple(FhwKey_t key)
{
    FhwHwAccPrio_t hwAccIx = key.id.accl;

    if ( unlikely(key.word == FHW_TUPLE_INVALID) )     /* caller checks already */
        return FHW_TUPLE_INVALID;

    dbg_assertr( (hwAccIx < FHW_PRIO_MAX) , FHW_TUPLE_INVALID );

    if ( likely(fhw.status) )
    {
        /* Do upcall into HW to clear an evicted connection */
        return fhw.hwAcc[hwAccIx].hwtbl[key.id.fhw].hw_key;
    }
    return FHW_TUPLE_INVALID;
}


/*
 *------------------------------------------------------------------------------
 * Function   : fhw_mcast_whitelist_add_hw
 * Description: Add a mcast_whiteliset entry in hardware HW via activate_fn.
 *------------------------------------------------------------------------------
 */
static uint32_t fhw_mcast_whitelist_add_hw(uint32_t flowIx, unsigned long blog,
                                           unsigned long new_flow,
                                           unsigned long key_in)
{
    Blog_t *blog_p = (Blog_t *) blog; 
    uint32_t flow_type = fhw_flow_type(blog_p);

    if ((flow_type == HW_CAP_IPV4_MCAST) || (flow_type == HW_CAP_IPV6_MCAST))
        flow_type = HW_CAP_MCAST_WHITELIST;
    else
        return FHW_TUPLE_INVALID;

    dbgl_print( DBG_EXTIF, "flowIx<%d> blog_p<0x%px>", flowIx, blog_p );
    return _fhw_activate_hw(flow_type, flowIx, blog, new_flow, key_in);
}

/*
 *------------------------------------------------------------------------------
 * Function     : fhw_bind_fc
 * Description  : Permits manual enabling|disabling of bloging for FlowCache.
 * Parameter    :
 *                To enable:    enable_flag > 0
 *                To disable:   enable_flag = 0
 *------------------------------------------------------------------------------
 */
void fhw_bind_fc(int enable_flag)
{
    FcBindFhwHooks_t fhw_hooks, *fhw_hooks_p = &fhw_hooks;

    if ( enable_flag )
    {
        /* Bind HWACC handlers to fc*/
        fhw_hooks_p->activate_fn       = (HOOK4PARM) fhw_activate_hw;
        fhw_hooks_p->deactivate_fn     = (HOOK3PARM) fhw_deactivate_hw;
        fhw_hooks_p->update_fn         = (HOOK3PARM) fhw_update_hw;
        fhw_hooks_p->refresh_fn        = (HOOK3PARM) fhw_refresh_hw; 
        fhw_hooks_p->refresh_pathstat_fn  = (HOOK3PARM) fhw_refresh_pathstat_hw;
        fhw_hooks_p->reset_stats_fn    = (HOOK2PARM) fhw_reset_stats_hw; 
        fhw_hooks_p->add_host_mac_fn   = (HOOKP) fhw_add_host_mac_hw;
        fhw_hooks_p->del_host_mac_fn   = (HOOKP) fhw_del_host_mac_hw;
        fhw_hooks_p->clear_fn          = (HOOK32) fhw_clear_hw;
        fhw_hooks_p->fc_clear_fn       = &fhw_fc_clear_hook;
        fhw_hooks_p->stats_fn          = (HOOK4PARM) fhw_stats_hw;
        fhw_hooks_p->hwsupport_fn      = (HOOKP2)fhw_chk_support_hw;
        fhw_hooks_p->hybrid_hwsupport_fn      = (HOOKP)fhw_chk_support_hw_hybrid;
        fhw_hooks_p->get_path_num_fn   = (HOOKV)fhw_get_path_num; 
        fhw_hooks_p->get_hw_tuple_fn   = (HOOK32)fhw_get_hw_tuple; 
        fhw_hooks_p->mcast_whitelist_add_fn = (HOOK4PARM)fhw_mcast_whitelist_add_hw;
        fhw_hooks_p->mcast_whitelist_rem_fn = (HOOK3PARM)fhw_deactivate_hw;
        fc_bind_fhw( fhw_hooks_p );

        printk( CLRbold PKTFLOW_MODNAME "HW acceleration enabled." CLRnl );
    }
    else
    {
        fhw_hooks_p->activate_fn       = (HOOK4PARM) NULL;
        fhw_hooks_p->deactivate_fn     = (HOOK3PARM) NULL;
        fhw_hooks_p->update_fn         = (HOOK3PARM) NULL;
        fhw_hooks_p->refresh_fn        = (HOOK3PARM) NULL;
        fhw_hooks_p->refresh_pathstat_fn  = (HOOK3PARM) NULL; 
        fhw_hooks_p->reset_stats_fn    = (HOOK2PARM) NULL; 
        fhw_hooks_p->add_host_mac_fn   = (HOOKP) NULL;
        fhw_hooks_p->del_host_mac_fn   = (HOOKP) NULL;
        fhw_hooks_p->clear_fn          = (HOOK32) NULL;
        fhw_hooks_p->fc_clear_fn       = &fhw_fc_clear_hook;
        fhw_hooks_p->stats_fn          = (HOOK4PARM) NULL;
        fhw_hooks_p->hwsupport_fn      = (HOOKP2)fhw_chk_support_hw;
        fhw_hooks_p->hybrid_hwsupport_fn      = (HOOKP)fhw_chk_support_hw_hybrid;
        fhw_hooks_p->get_path_num_fn   = (HOOKV)NULL;
        fhw_hooks_p->get_hw_tuple_fn   = (HOOK32)NULL; 
        fhw_hooks_p->mcast_whitelist_add_fn = (HOOK4PARM)NULL;
        fhw_hooks_p->mcast_whitelist_rem_fn = (HOOK3PARM)NULL;

        /* Unbind hooks with fc */
        fc_bind_fhw( fhw_hooks_p );
        printk( CLRbold2 PKTFLOW_MODNAME "HW acceleration disabled." CLRnl );
    }

    fhw.status = enable_flag;
}

static int fhw_clear( uint32_t key, uint32_t scope )
{
    if (fhw_fc_clear_hook != (FC_CLEAR_HOOK)NULL)
            fhw_fc_clear_hook(key, scope);

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function     : fhw_set_cap2HwAcc
 * Description  : Sets the cap to HwAccIx Map
 *                If the default cap to HW Accelerator mapping is not
 *                acceptable, the new mappings can be set here.
 *------------------------------------------------------------------------------
 */
static void fhw_set_cap2HwAcc(HwCap_t cap, 
        FhwHwAccPrio_t hwAcc0, FhwHwAccPrio_t hwAcc1)
{
    dbgl_print( DBG_EXTIF, "cap<%d> hwAcc0<%d> hwAcc1<%d>", 
                cap, hwAcc0, hwAcc1 ); 

    if ( (hwAcc0 >= FHW_PRIO_MAX) || (hwAcc1 >= FHW_PRIO_MAX) )
        fc_error("out of range hwAccIx" );   

    fhw.cap2HwAccMap[cap][0] = hwAcc0;
    fhw.cap2HwAccMap[cap][1] = hwAcc1;
}

/*
 *------------------------------------------------------------------------------
 * Function     : fhw_init_cap2HwAccMap
 * Description  : Initializes the default cap to HwAccIx Map
 *                By default, all the cap (flow types) will be checked for
 *                the first accelerator and then second in case of multiple
 *                HW accelerators.
 *------------------------------------------------------------------------------
 */
static void fhw_init_cap2HwAccMap( void )
{
    HwCap_t cap;

    for (cap = HW_CAP_NONE; cap < HW_CAP_MAX; cap++)
    {
        fhw_set_cap2HwAcc(cap, FHW_PRIO_0, FHW_PRIO_0);
        fhw_set_cap2HwAcc(cap, FHW_PRIO_0, FHW_PRIO_1);
    }
}

/*
 *------------------------------------------------------------------------------
 * Function     : fhw_stats_print
 * Description  : Dump the stats to a proc fs file.
 * Design Note  : Invoked by fcacheDrvProcfs() in fcachedrv.c
 *------------------------------------------------------------------------------
 */
int fhw_stats_print(struct seq_file *m)
{
    FhwStats_t *stats = &fhw.stats;

    bcm_seq_printf( m, "\nFHW Global Stats:\n\n");
    bcm_seq_printf( m, "Flow Activation Success    =%u\n", stats->actSuccess);
    bcm_seq_printf( m, "Flow Activation Failure    =%u\n", stats->actFail);
    bcm_seq_printf( m, "Flow Deactivation Success  =%u\n", stats->deactSuccess);
    bcm_seq_printf( m, "Flow Update Success        =%u\n", stats->updSuccess);
    bcm_seq_printf( m, "Flow Refresh Success       =%u\n", stats->refreshSuccess);
    bcm_seq_printf( m, "Flow Refresh Failure       =%u\n", stats->refreshFail);
    bcm_seq_printf( m, "Host MAC Add Success       =%u\n", stats->macAddSuccess);
    bcm_seq_printf( m, "Host MAC Add Failure       =%u\n", stats->macAddFail);
    bcm_seq_printf( m, "Host MAC Delete Success    =%u\n", stats->macDelSuccess);
    bcm_seq_printf( m, "Host MAC Delete Failure    =%u\n", stats->macDelFail);

    bcm_seq_printf( m, "\nHW Acceleration not supported reason stats:\n\n");

    if (stats->hwSup.disable)   bcm_seq_printf( m, "HW Acceleration Disable    =%u\n", stats->hwSup.disable);
    if (stats->hwSup.wltx)      bcm_seq_printf( m, "WLAN TX                    =%u\n", stats->hwSup.wltx);
    if (stats->hwSup.wlrx)      bcm_seq_printf( m, "WLAN RX                    =%u\n", stats->hwSup.wlrx);
    if (stats->hwSup.T6in4)     bcm_seq_printf( m, "6in4 Tunnel                =%u\n", stats->hwSup.T6in4);
    if (stats->hwSup.sdnat)     bcm_seq_printf( m, "SDNAT                      =%u\n", stats->hwSup.sdnat);
    if (stats->hwSup.mapt)      bcm_seq_printf( m, "MAPT                       =%u\n", stats->hwSup.mapt);
    if (stats->hwSup.localTcp)  bcm_seq_printf( m, "Local TCP Flow             =%u\n", stats->hwSup.localTcp);
    if (stats->hwSup.l2tp)      bcm_seq_printf( m, "L2TP                       =%u\n", stats->hwSup.l2tp);
    if (stats->hwSup.pptp)      bcm_seq_printf( m, "PPTP                       =%u\n", stats->hwSup.pptp);
    if (stats->hwSup.esp)       bcm_seq_printf( m, "ESP                        =%u\n", stats->hwSup.esp);
    if (stats->hwSup.hwPort)    bcm_seq_printf( m, "RX HW Port                 =%u\n", stats->hwSup.hwPort);
    if (stats->hwSup.xoa)       bcm_seq_printf( m, "XoA                        =%u\n", stats->hwSup.xoa);
    if (stats->hwSup.usb)       bcm_seq_printf( m, "USB                        =%u\n", stats->hwSup.usb);
    if (stats->hwSup.lte)       bcm_seq_printf( m, "LTE                        =%u\n", stats->hwSup.lte);
    if (stats->hwSup.l2l)       bcm_seq_printf( m, "LAN-2-LAN                  =%u\n", stats->hwSup.l2l);
    if (stats->hwSup.mcRtpSeq)  bcm_seq_printf( m, "Multicast RTP Sequence     =%u\n", stats->hwSup.mcRtpSeq);
    if (stats->hwSup.gre)       bcm_seq_printf( m, "GRE                        =%u\n", stats->hwSup.gre);
    if (stats->hwSup.w2w)       bcm_seq_printf( m, "WAN-2-WAN                  =%u\n", stats->hwSup.w2w);
    if (stats->hwSup.llcsnap)   bcm_seq_printf( m, "LLCSNAP                    =%u\n", stats->hwSup.llcsnap);
    if (stats->hwSup.vxlan)     bcm_seq_printf( m, "VxLAN                      =%u\n", stats->hwSup.vxlan);
    if (stats->hwSup.vtagNum)   bcm_seq_printf( m, "Number of VLAN Tags        =%u\n", stats->hwSup.vtagNum);
    if (stats->hwSup.spdTst)    bcm_seq_printf( m, "Speed Test                 =%u\n", stats->hwSup.spdTst);
    if (stats->hwSup.mapt_gre)  bcm_seq_printf( m, "MAPT & LAN GRE             =%u\n", stats->hwSup.mapt_gre);
    if (stats->hwSup.t4in6_gre) bcm_seq_printf( m, "4in6 Tunnel & LAN GRE      =%u\n", stats->hwSup.t4in6_gre);
    if (stats->hwSup.t6in4_gre) bcm_seq_printf( m, "6in4 Tunnel & LAN GRE      =%u\n", stats->hwSup.t6in4_gre);

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function     : fhw_construct
 * Description  : Construction of HW accelerator subsystem.
 * Design Note  : 
 *------------------------------------------------------------------------------
 */
int fhw_construct(void)
{
    dbg_config( CC_CONFIG_FHW_DBGLVL );
    dbgl_func( DBG_BASIC );

    memset( (void*)&fhw, 0, sizeof(Fhw_t) );
    fhw_init_cap2HwAccMap();

    printk( CLRbold "Initialized Fcache HW accelerator layer state" CLRnl );
    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function     : fhw_destruct
 * Description  : Destruction of flow cache subsystem.
 * Design Note  : Invoked by pktflow_destruct() in fcachedrv.c
 *------------------------------------------------------------------------------
 */
void fhw_destruct(void)
{
    dbgl_func( DBG_BASIC );

    fhw_bind_fc( 0 );

    printk( CLRbold2 PKTFLOW_MODNAME "Reset HW acceleration state" CLRnl );
}

EXPORT_SYMBOL(fhw_bind_hw);

