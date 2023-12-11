/*
*  Copyright 2011, Broadcom Corporation
*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

//**************************************************************************
// File Name  : mac_drv_ae.c
// This file mainly used to implement MAC driver for AE mode.
// Description: Broadcom EPON Active Ethernet Interface Driver
//**************************************************************************
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "EponTypes.h"
#include "board.h"

#include "rdpa_api.h"
#include <bcmsfp_i2c.h>
#include "mac_drv.h"
#include "phy_drv.h"
#include "wan_drv.h"

#if defined (CONFIG_BCM96846)
#undef READ32
#include "shared_utils.h"
#include "drivers_epon_ag.h"
#endif

#include "EponMac.h"
#include "Lif.h"
#ifdef CONFIG_EPON_10G_SUPPORT
#include "Xif.h"
#endif
#include "AeDriver.h"
#include "drv_epon_lif_ag.h"
#include "phy_drv_int_ae.h"

#ifdef CONFIG_BCM_PON
extern int bcm_i2c_legacy_pon_optics_sd_get(int bus, unsigned char * sig_det);
#endif

static uint64_t aeStatsLink[EponLinkHwStatCount];
static uint64_t aeStatsLinkSwStats[SwStatIdNum];

#define AeTimerMs  (1000)
extern void enet_pon_drv_link_change(int link);
extern void enet_pon_drv_speed_change(phy_speed_t speed, phy_duplex_t duplex);
extern void WanPortRunnerTrafficSet(BOOL enable);

static DEFINE_MUTEX(epon_ae_mutex);
static uint8_t is_ext_phy = 0;
static BOOL ae_mac_enabled = FALSE;
static void AeHandle1sTimer(struct work_struct *work_arg);
static DECLARE_DELAYED_WORK(dwork_poll, AeHandle1sTimer);
static uint8_t is_fix_mode = 0;

phy_speed_t laser2phy[LaserRate10G+1] = {PHY_SPEED_UNKNOWN, PHY_SPEED_1000, PHY_SPEED_2500, PHY_SPEED_10000};


void ae_link_create(void)
{
    /* GBE egress_tm, only channel 0 is enabled */
#ifdef CONFIG_EPON_10G_SUPPORT
    XifCreateLink(0, 0x5555);
#endif   
    LifCreateLink(0, 0x5555);
    AeStartLinks(1UL << 0);
    AeSetBurstCap(0, NULL);
    
    if (is_ext_phy || ae_int_phy_enabled())
    {
        AeLaserTxModeSet(LaserTxModeContinuous);
        AeLaserRxPowerSet(1);
    }
    
    EponEnableRx(ae_mac_enabled);
    
    LifLocked();
#ifdef CONFIG_EPON_10G_SUPPORT
    XifLocked();
#endif 
}
        
static int epon_ae_mac_enable(mac_dev_t *mac_dev)
{
    EponEnableRx(TRUE);
    ae_mac_enabled  = TRUE;
    
    return 0;
}

static int epon_ae_mac_disable(mac_dev_t *mac_dev)
{
    EponEnableRx(FALSE);
    ae_mac_enabled = FALSE;
    
    return 0;
}

static int epon_ae_mac_cfg_get(mac_dev_t *mac_dev, mac_cfg_t *mac_cfg)
{
    memset(mac_cfg, 0, sizeof(mac_cfg_t));
    
    mac_cfg->duplex = MAC_DUPLEX_FULL;

    switch (AeGetRate())
    {
        case LaserRate1G:
            mac_cfg->speed = MAC_SPEED_1000;
            break;
        case LaserRate2G:
            mac_cfg->speed = MAC_SPEED_2500;
            break;
        case LaserRate10G:
            mac_cfg->speed = MAC_SPEED_10000;
            break;
        default:
            mac_cfg->speed = MAC_SPEED_UNKNOWN;
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "unknown AE MAC speed\n");
            return -1;
    }
    
    return 0;
}

static void ae_rdpa_port_speed_set(rdpa_speed_type speed)
{
    bdmf_error_t rc = 0;
    bdmf_object_handle port_obj = NULL;
    rdpa_if port = rdpa_wan_type_to_if(rdpa_wan_epon);
    
    rc = rdpa_port_get(port, &port_obj);
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_AE, "failed to get wan port. rc=%d\n", rc);
        return;
    }
	
    rdpa_port_speed_set(port_obj, speed);
    
    bdmf_put(port_obj);
    return;
}

static BOOL is_wanport_enabled(void)    
{
    bdmf_error_t rc = 0;
    bdmf_object_handle port_obj = NULL;
    rdpa_if port = rdpa_wan_type_to_if(rdpa_wan_epon);
    bdmf_boolean previous_enable_status = 0;

    rc = rdpa_port_get(port, &port_obj);
    if (rc)  
    {
        BCM_LOG_ERROR(BCM_LOG_ID_AE, "Failed to get rdpa wan port. rc=%d\n", rc);
        return 0;
    }

    rc = rdpa_port_enable_get(port_obj, &previous_enable_status);
    if (rc)  
    {
        BCM_LOG_ERROR(BCM_LOG_ID_AE, "Failed to get rdpa wan port enable status. rc=%d\n", rc);
    }

    bdmf_put(port_obj);
    return previous_enable_status;
}


static int ae_switching = 0;
int epon_ae_mac_cfg_set(mac_dev_t *mac_dev, mac_cfg_t *mac_cfg)
{
    LaserRate rate = LaserRateOff;
    serdes_wan_type_t serdes_wan_type;
    rdpa_speed_type speed = rdpa_speed_1g;
    BOOL wan_port_enabled = FALSE;
    int rc = 0;

    switch (mac_cfg->speed)
    {
#if defined(CONFIG_BCM96855)
        case MAC_SPEED_10:
        case MAC_SPEED_100:
            rate = LaserRate1G;
            serdes_wan_type = SERDES_WAN_TYPE_AE;
            speed = rdpa_speed_100m;
            break;
#endif
        case MAC_SPEED_1000:
            rate = LaserRate1G;
            serdes_wan_type = SERDES_WAN_TYPE_AE;
            speed = rdpa_speed_1g;
            break;
        case MAC_SPEED_2500:
            /* 2.5G Base-X */
            rate = LaserRate2G;
            serdes_wan_type = SERDES_WAN_TYPE_AE_2_5G;
            speed = rdpa_speed_2_5g;
   
#ifdef CONFIG_EPON_10G_SUPPORT
#if defined(CONFIG_BCM96856) || defined(CONFIG_BCM96855)
            {
                /* 2.5G Base-R */
                phy_dev_t *phy_dev = (phy_dev_t *)mac_dev->priv;

                if (phy_dev && phy_dev->phy_drv && (phy_dev->phy_drv->phy_type != PHY_TYPE_PON))
                {
                    if (phy_dev->inter_phy_types & INTER_PHY_TYPE_2P5GBASE_R_M)
                    {
                        serdes_wan_type = SERDES_WAN_TYPE_AE_2_5G_R;
                        rate = LaserRate10G;
                    }
                }
            }
#endif
#endif
            break;
#ifdef CONFIG_EPON_10G_SUPPORT
#if defined(CONFIG_BCM96856) || defined(CONFIG_BCM96855)
        case MAC_SPEED_5000:
            /* 5G Base-R */
            rate = LaserRate10G;
            serdes_wan_type = SERDES_WAN_TYPE_AE_5G;
            speed = rdpa_speed_5g;
            break;
#endif
        case MAC_SPEED_10000:
            rate = LaserRate10G;
            serdes_wan_type = SERDES_WAN_TYPE_AE_10G;
            speed = rdpa_speed_10g;
            break;
#endif
        default:
            BCM_LOG_DEBUG(BCM_LOG_ID_AE, "speed %d is not supported by AE MAC\n", mac_cfg->speed);
            return -1;
    }
	
    BCM_LOG_DEBUG(BCM_LOG_ID_AE, "Switch to rate %d and serdes_wan_type %d\n\n", rate, serdes_wan_type);
    if ((rate == AeGetRate() && wan_serdes_type_get() == serdes_wan_type) || is_fix_mode || ae_switching)
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_AE, "No need to switch!\n\n");
        return 0;
    }
    
    mutex_lock(&epon_ae_mutex);
    ae_switching = 1;
    /* store the port enabled value before switching */
    wan_port_enabled = is_wanport_enabled();

    AeSetRunnerPortTraffic(FALSE);
    AeStopLinks(1UL << 0);
    if (!AeCheckRunnerEmpty())
    {
        BCM_LOG_ERROR(BCM_LOG_ID_AE, "AE MAC flush not done\n\n");
        goto switching_abort;
    }
    ae_rdpa_port_speed_set(speed);

    rc = wan_serdes_config(serdes_wan_type);
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_AE, "WAN serdes reconfig fails errno rc=%d\n", rc);
        goto switching_abort;
    }
    mdelay(5);
    AeSetRate(rate);
    AeMacInit(rate);
    mdelay(1);
    ae_link_create();
#if defined(CONFIG_BCM96855)
    AeSetSgmiiSpeed(mac_cfg->speed);
#endif
    /* restore the port enabled after switching */
    if (wan_port_enabled)
    {
        AeSetRunnerPortTraffic(TRUE);
    }

    ae_switching = 0;
    mutex_unlock(&epon_ae_mutex);
    
    return 0;
    
switching_abort:
    mutex_unlock(&epon_ae_mutex);
    ae_switching = 0;
    return -1;
}

static int epon_ae_mac_stats_get(mac_dev_t *mac_dev, mac_stats_t *mac_stats)
{
    uint8_t stat;
    uint64_t dbuf;

    memset(mac_stats, 0, sizeof(mac_stats_t));
    
    if (!mutex_trylock(&epon_ae_mutex))
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_AE, "AE MAC is protected\n");
        return 0;
    }
    
    for (stat = EponBiDnTotalBytesRx; stat <= EponBiDnUndersizedFramesRx; ++stat)
    {
        EponReadLinkStat(0, (EponLinkStatId)stat, &dbuf, FALSE);
        if ((stat >= EponBiDnBroadcastFramesRx) && (stat <= EponBiDnUnicastFramesRx))
        {
            U64AddEqU64(&aeStatsLinkSwStats[SwStatIdRxTotalFrames],&dbuf);
        }
        U64AddEqU64(&aeStatsLink[stat],&dbuf);
    }

    for (stat = EponBiUpTotalBytesTx; stat <= EponBiUpUnicastFramesTx; ++stat)
    {
        EponReadLinkStat(0, (EponLinkStatId)stat, &dbuf, FALSE);
        if ((stat >= EponBiUpBroadcastFramesTx) && (stat <= EponBiUpUnicastFramesTx))
        {
            U64AddEqU64(&aeStatsLinkSwStats[SwStatIdTxTotalFrames],&dbuf);
        }
        U64AddEqU64(&aeStatsLink[stat],&dbuf);
    }
        
    mutex_unlock(&epon_ae_mutex);
    
    mac_stats->rx_byte = aeStatsLink[EponBiDnTotalBytesRx];
    mac_stats->rx_packet = aeStatsLinkSwStats[SwStatIdRxTotalFrames];
    mac_stats->rx_frame_64 = aeStatsLink[EponBiDn64ByteFramesRx];
    mac_stats->rx_frame_65_127 = aeStatsLink[EponBiDn65to127ByteFramesRx];
    mac_stats->rx_frame_128_255 = aeStatsLink[EponBiDn128to255ByteFramesRx];
    mac_stats->rx_frame_256_511 = aeStatsLink[EponBiDn256to511ByteFramesRx];
    mac_stats->rx_frame_512_1023 = aeStatsLink[EponBiDn512to1023ByteFramesRx];
    mac_stats->rx_frame_1024_1518 = aeStatsLink[EponBiDn1024to1518ByteFramesRx];
    mac_stats->rx_frame_1519_mtu = aeStatsLink[EponBiDn1518PlusByteFramesRx];
    mac_stats->rx_multicast_packet = aeStatsLink[EponBiDnMulticastFramesRx];
    mac_stats->rx_broadcast_packet = aeStatsLink[EponBiDnBroadcastFramesRx];
    mac_stats->rx_unicast_packet = aeStatsLink[EponBiDnUnicastFramesRx];
    mac_stats->rx_fcs_error = aeStatsLink[EponBiDnFcsErrors];
    mac_stats->rx_undersize_packet = aeStatsLink[EponBiDnUndersizedFramesRx];
    mac_stats->rx_oversize_packet = aeStatsLink[EponBiDnOversizedFramesRx];
    mac_stats->tx_byte = aeStatsLink[EponBiUpTotalBytesTx];
    mac_stats->tx_packet = aeStatsLinkSwStats[SwStatIdTxTotalFrames];
    mac_stats->tx_frame_64 = aeStatsLink[EponBiUp64ByteFramesTx];
    mac_stats->tx_frame_65_127 = aeStatsLink[EponBiUp65To127ByteFramesTx];
    mac_stats->tx_frame_128_255 = aeStatsLink[EponBiUp128To255ByteFramesTx];
    mac_stats->tx_frame_256_511 = aeStatsLink[EponBiUp256To511ByteFramesTx];
    mac_stats->tx_frame_512_1023 = aeStatsLink[EponBiUp512To1023ByteFamesTx];
    mac_stats->tx_frame_1024_1518 = aeStatsLink[EponBiUp1024To1518ByteFramesTx];
    mac_stats->tx_frame_1519_mtu = aeStatsLink[EponBiUp1518PlusByteFramesTx];
    mac_stats->tx_multicast_packet = aeStatsLink[EponBiUpMulticastFramesTx];
    mac_stats->tx_broadcast_packet = aeStatsLink[EponBiUpBroadcastFramesTx];
    mac_stats->tx_unicast_packet = aeStatsLink[EponBiUpUnicastFramesTx];
    
    return 0;
}

static int epon_ae_mac_stats_clear(mac_dev_t *mac_dev)
{
    memset(aeStatsLink, 0, sizeof(aeStatsLink));
    memset(aeStatsLinkSwStats, 0, sizeof(aeStatsLinkSwStats));

    return 0;
}

static void AeHandle1sTimer(struct work_struct *work_arg)
{
    int up = 0;
    static int up_prev = 0;
    LaserRate rate = AeGetRate();
    BOOL sig_det = TRUE;

    /* To avoid race condition between link up/down event and ae switching */
    if (!mutex_trylock(&epon_ae_mutex))
    {
        BCM_LOG_ERROR(BCM_LOG_ID_AE, "AE MAC is protected\n");
        goto exit;
    }

#ifndef CONFIG_BCM_PON
    bcm_i2c_pon_optics_sd_get(&sig_det);
#else
    bcm_i2c_legacy_pon_optics_sd_get(wantop_bus_get(), &sig_det);
#endif

    switch (rate)
    {
#ifdef CONFIG_EPON_10G_SUPPORT
        case LaserRate10G:
            up = XifLocked() && sig_det;
            break;
#endif
        case LaserRate1G:
        case LaserRate2G:
            up = LifLocked() && sig_det;
            break;
        default:
            goto exit_withlock;
    }
    
    /* External PHY dev */
    if (is_ext_phy)
    {
        if (!up)
        {
            BCM_LOG_DEBUG(BCM_LOG_ID_AE, "AE MAC is out of sync\n");
            kerSysLedCtrl(kLedEpon, kLedStateOff);
        }
        else if (up && !up_prev)
        {
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "AE MAC is synced\n");
            kerSysLedCtrl(kLedEpon, kLedStateOn);
        }
    }
    /* Internal serdes PHY */
    else
    {   
        /* overide up status */
        if (!ae_int_phy_enabled())
            up = 0;

        if (up != up_prev)
        {
            if (up)
            {
                enet_pon_drv_speed_change(laser2phy[rate],PHY_DUPLEX_FULL);
                kerSysLedCtrl(kLedEpon, kLedStateOn);
            }
            else
                kerSysLedCtrl(kLedEpon, kLedStateOff);

            enet_pon_drv_link_change(up);
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "link status %d\n", up);
        }
    }

    up_prev = up;

exit_withlock:
    mutex_unlock(&epon_ae_mutex);

exit:
    schedule_delayed_work(&dwork_poll, msecs_to_jiffies(AeTimerMs));
	
}

#define PROC_CMD_MAX_LEN 64
static ssize_t proc_set_ae_mode(struct file *file, const char *buff, size_t len, loff_t *offset)
{
    char input[PROC_CMD_MAX_LEN];
    uint32_t val;
    int ret=0;
    
    if (len > PROC_CMD_MAX_LEN)
        len = PROC_CMD_MAX_LEN;
    
    if (copy_from_user(input, buff, len) != 0)
        return -EFAULT;
    
    ret = sscanf(input, "%d", &val);
    if (ret >= 1)
    {
        mac_cfg_t mac_cfg;
        if (val == 1)
        {
            mac_cfg.speed = MAC_SPEED_1000;
        }
        else if(val == 25)
        {
            mac_cfg.speed = MAC_SPEED_2500;
        }
        else
        {
            mac_cfg.speed = MAC_SPEED_10000;
        }
        epon_ae_mac_cfg_set(NULL, &mac_cfg);
    }
    else
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_AE, "error format!");
        return -EFAULT;
    }
    
    return len;
};

static struct file_operations set_ae_mode_proc =
{
    .write = proc_set_ae_mode,
};

static int epon_ae_mac_init(mac_dev_t *mac_dev)
{
    phy_dev_t *phy_dev = (phy_dev_t *)mac_dev->priv;
    
    memset(aeStatsLink, 0, sizeof(aeStatsLink));
    memset(aeStatsLinkSwStats, 0, sizeof(aeStatsLinkSwStats));

    if (phy_dev && phy_dev->phy_drv && (phy_dev->phy_drv->phy_type != PHY_TYPE_PON))
    {
        is_ext_phy = TRUE;
    }
    else
    {
        is_ext_phy = FALSE;
    }
    
    if (!is_ext_phy)
    {   
        ae_int_phy_drv_set();
    }

#ifndef CONFIG_BCM_PON
    bcm_i2c_optics_tx_control(BCM_I2C_OPTICS_ENABLE);
#endif
    schedule_delayed_work(&dwork_poll, 0);

    return 0;
}

static int epon_ae_mac_drv_init(mac_drv_t *mac_drv)
{
    struct proc_dir_entry *p0, *p1;

    ae_link_create();
    p0 = proc_mkdir("aemode", NULL);
    if (!p0)
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_AE, "failed to create /proc/aemode !");
    }
    else
    {
        p1 = proc_create("switch", S_IWUSR | S_IRUSR, p0, &set_ae_mode_proc);
        if (!p1)
        {
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "failed to create /proc/aemode/switch !");
        }
    }
    
    mac_drv->initialized = 1;
    
    return 0;
}

static int epon_ae_mac_mtu_set(mac_dev_t *mac_dev, int mtu)
{
    EponSetMaxFrameSize(mtu);

    return 0;
}

#if defined(CONFIG_BCM96855)
int epon_ae_mac_pause_set(mac_dev_t *mac_dev, int rx_enable, int tx_enable, char *src_addr)
{
    epn_onu_pause_pfc_cfg onu_pause_pfc_cfg;
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U8 default_pause_da[] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x01};
    
    EponSetMac (0, (MacAddr *)&default_pause_da[0]);

    memset(&onu_pause_pfc_cfg, 0, sizeof(onu_pause_pfc_cfg));

    drv_error += ag_drv_epn_onu_pause_pfc_cfg_get(&onu_pause_pfc_cfg);

    onu_pause_pfc_cfg.cfgupftype = 1;
    onu_pause_pfc_cfg.cfgupfen = tx_enable;
    onu_pause_pfc_cfg.cfgdpfenable = rx_enable;
    onu_pause_pfc_cfg.cfgdpfoperatingmode = 1;
    onu_pause_pfc_cfg.cfgdpfasyncbypassen = 0;
    onu_pause_pfc_cfg.cfgupfasyncbypassen = 0;
    onu_pause_pfc_cfg.cfgupfoveride = 1;
    onu_pause_pfc_cfg.cfgupfgengo = 0;
    

    drv_error += ag_drv_epn_onu_pause_pfc_cfg_set(&onu_pause_pfc_cfg);

    return drv_error;
}

int epon_ae_mac_pause_get(mac_dev_t *mac_dev, int *rx_enable, int *tx_enable)
{
    epn_onu_pause_pfc_cfg onu_pause_pfc_cfg;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    memset(&onu_pause_pfc_cfg, 0, sizeof(onu_pause_pfc_cfg));

    drv_error += ag_drv_epn_onu_pause_pfc_cfg_get(&onu_pause_pfc_cfg);
    *rx_enable = onu_pause_pfc_cfg.cfgdpfenable;
    *tx_enable = onu_pause_pfc_cfg.cfgupfen;
    return drv_error;
}
#else
#define epon_ae_mac_pause_set NULL
#define epon_ae_mac_pause_get NULL
#endif

mac_drv_t mac_drv_epon_ae =
{
    .mac_type = MAC_TYPE_EPON_AE,
    .name = "EPONAE",
    .init = epon_ae_mac_init,
    .enable = epon_ae_mac_enable,
    .disable = epon_ae_mac_disable,
    .cfg_get = epon_ae_mac_cfg_get,
    .cfg_set = epon_ae_mac_cfg_set,
    .stats_get = epon_ae_mac_stats_get,
    .stats_clear = epon_ae_mac_stats_clear,
    .mtu_set = epon_ae_mac_mtu_set,
    .drv_init = epon_ae_mac_drv_init,
    .pause_get = epon_ae_mac_pause_get,
    .pause_set = epon_ae_mac_pause_set,
};
