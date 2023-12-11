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
// File Name  : phy_drv_int_ae.c
// This file mainly used to implement internal PHY driver for AE mode.
// Description: Broadcom EPON Active Ethernet Interface Driver            
//**************************************************************************
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <asm/uaccess.h>

#include "EponTypes.h"
#include "Laser.h"
#include "rdpa_api.h"
#include "phy_drv.h"

#if defined (CONFIG_BCM96846)
#undef READ32
#include "shared_utils.h"
#include "drivers_epon_ag.h"
#endif

#include "drv_epon_lif_ag.h"
#include "phy_drv_int_ae.h"
#include "mac_drv.h"
#include "phy_drv.h"
#include "mac_drv_ae.h"
#include "AeDriver.h"

extern phy_drv_t phy_drv_pon;
static uint32_t int_phy_enabled = 1;

#define AUTONEG_COMPLETE_COUNT  1000
#if defined (CONFIG_BCM96856)
int ae_int_phy_autoneg_restart(phy_dev_t *phy_dev, uint32_t caps)
{
    uint32_t autoneg_countdown = AUTONEG_COMPLETE_COUNT;
    uint16_t cf_autoneg_linktimer;
    uint8_t cf_autoneg_mode_sel, cf_autoneg_restart, cf_autoneg_en;
    uint8_t an_lp_remote_fault, an_sync_status, an_complete;
    
    ag_drv_lif_p2p_autoneg_control_get(&cf_autoneg_linktimer, &cf_autoneg_mode_sel,
                                                    &cf_autoneg_restart, &cf_autoneg_en);
    if (!cf_autoneg_en)
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_AE, "autoneg disabled\n");
        return -1;
    }
    
    do 
    {
        ag_drv_lif_p2p_autoneg_status_get(&an_lp_remote_fault, &an_sync_status, &an_complete);
        autoneg_countdown--;
    }while (!an_complete && autoneg_countdown);
    
    if (an_complete)
    {
        if (an_sync_status)
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "autoneg sucessfully\n");
        else
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "autoneg failed\n");
    }
        
    return 0;
}

void ae_int_phy_caps_set_hw(phy_dev_t *phy_dev, uint32_t caps)
{
    uint16_t v0;
    uint8_t  v1, v2, cf_autoneg_en;
    
    BCM_LOG_DEBUG(BCM_LOG_ID_AE, "INT PHY rate %d, caps 0x%x\n", AeGetRate(), caps);
    
    if (AeGetRate() != LaserRate1G)
        return;
    
    ag_drv_lif_p2p_autoneg_control_get(&v0, &v1, &v2, &cf_autoneg_en);
    cf_autoneg_en = (caps & PHY_CAP_AUTONEG) ? 1 : 0;
    ag_drv_lif_p2p_autoneg_control_set(v0, v1, v2, cf_autoneg_en);
    
    if (cf_autoneg_en)
        ae_int_phy_autoneg_restart(phy_dev, caps);
}

void ae_int_phy_caps_get_hw(phy_dev_t *phy_dev, uint32_t *caps)
{
    uint16_t v0;
    uint8_t  v1, v2, cf_autoneg_en;
    
    if (AeGetRate() != LaserRate1G)
        return;
    
    ag_drv_lif_p2p_autoneg_control_get(&v0, &v1, &v2, &cf_autoneg_en);
    if (cf_autoneg_en)
        *caps |= PHY_CAP_AUTONEG;
}
#elif defined (CONFIG_BCM96846)
int ae_int_phy_autoneg_restart(phy_dev_t *phy_dev, uint32_t caps)
{
    uint8_t rxsyncsqq, rxcorcs, fecrxoutcs;
    uint32_t debug_fec_sm = 0;
    uint32_t autoneg_countdown = AUTONEG_COMPLETE_COUNT;

    RU_REG_READ(0, LIF, DEBUG_FEC_SM, debug_fec_sm);
    rxsyncsqq = RU_FIELD_GET(0, LIF, DEBUG_FEC_SM, RXSYNCSQQ, debug_fec_sm);
    if (!(rxsyncsqq & 0x1))
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_AE, "autoneg disabled\n");
        return -1;
    }

    rxcorcs = RU_FIELD_GET(0, LIF, DEBUG_FEC_SM, RXCORCS, debug_fec_sm);
    /* restart autoneg */
    rxcorcs |= 0x1;
    debug_fec_sm = RU_FIELD_SET(0, LIF, DEBUG_FEC_SM, RXCORCS, debug_fec_sm, rxcorcs);
    RU_REG_WRITE(0, LIF, DEBUG_FEC_SM, debug_fec_sm);
    
    do 
    {
        ag_drv_lif_debug_fec_sm_get(&rxsyncsqq, &rxcorcs, &fecrxoutcs);
        autoneg_countdown--;
    }while (!(fecrxoutcs & 0x1) && autoneg_countdown);
    
    if (fecrxoutcs & 0x1)
    {
        if (fecrxoutcs & 0x2)
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "autoneg sucessfully\n");
        else
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "autoneg failed\n");
    }
    
    return 0;
}

void ae_int_phy_caps_set_hw(phy_dev_t *phy_dev, uint32_t caps)
{
    uint8_t rxsyncsqq;
    uint32_t debug_fec_sm = 0;
    
    BCM_LOG_DEBUG(BCM_LOG_ID_AE, "chip rev 0x%x, caps 0x%x\n", UtilGetChipRev(), caps); 
    
    if (UtilGetChipRev() == 0xA0)
        return;
    
    RU_REG_READ(0, LIF, DEBUG_FEC_SM, debug_fec_sm);
    rxsyncsqq = RU_FIELD_GET(0, LIF, DEBUG_FEC_SM, RXSYNCSQQ, debug_fec_sm);
    if (caps & PHY_CAP_AUTONEG)
        rxsyncsqq |= 0x1;
    else
        rxsyncsqq &= ~0x1;
    debug_fec_sm = RU_FIELD_SET(0, LIF, DEBUG_FEC_SM, RXSYNCSQQ, debug_fec_sm, rxsyncsqq);
    RU_REG_WRITE(0, LIF, DEBUG_FEC_SM, debug_fec_sm);
    
    if (caps & PHY_CAP_AUTONEG)
        ae_int_phy_autoneg_restart(phy_dev, caps);
}

void ae_int_phy_caps_get_hw(phy_dev_t *phy_dev, uint32_t *caps)
{
    uint8_t rxsyncsqq, v0, v1;
    
    if (UtilGetChipRev() == 0xA0)
        return;
    
    ag_drv_lif_debug_fec_sm_get(&rxsyncsqq, &v0, &v1);
    if (rxsyncsqq & 0x1)
        *caps |= PHY_CAP_AUTONEG;
}
#else
#define ae_int_phy_caps_set_hw(...)
#define ae_int_phy_caps_get_hw(...)
#endif

#if defined (CONFIG_BCM96855)
int ae_int_phy_pause_set(phy_dev_t *phy_dev, uint32_t caps)
{
    int pause_enable = 0;
	
    BCM_LOG_DEBUG(BCM_LOG_ID_AE, "INT PHY pause set %d, caps 0x%x\n", AeGetRate(), caps);

    if (caps & PHY_CAP_PAUSE)
        pause_enable = 1;
    else
        pause_enable = 0;
	
    if (epon_ae_mac_pause_set(NULL, pause_enable, pause_enable, NULL) == 0)
    {
        phy_dev->pause_rx = pause_enable; 
        phy_dev->pause_tx = pause_enable;
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_AE, "epon_ae_mac_pause_set fail \n");
    }
    
    return 0;
}

int ae_int_phy_pause_get(phy_dev_t *phy_dev, uint32_t *caps)
{
    int rx_enable = 0;
    int tx_enable = 0;
	
    if (epon_ae_mac_pause_get(NULL, &rx_enable, &tx_enable) == 0)
    {
        if (rx_enable == 1 && tx_enable == 1)
        {
            *caps |= PHY_CAP_PAUSE;
            phy_dev->pause_rx = 1;
            phy_dev->pause_tx = 1;
        }
		else
        {
            *caps &= (~PHY_CAP_PAUSE);
            phy_dev->pause_rx = 0;
            phy_dev->pause_tx = 0;
        }
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_AE, "epon_ae_mac_pause_get fail \n");
    }
    
    return 0;
}
#else
#define ae_int_phy_pause_set(...)
#define ae_int_phy_pause_get(...)
#endif

int ae_int_phy_caps_set(phy_dev_t *phy_dev, uint32_t caps)
{
    LaserRate rate = AeGetRate();
    
    ae_int_phy_pause_set(phy_dev, caps);

    switch (rate)
    {
        case LaserRate1G:   /* Only 1G mode support autoneg */
#if defined (CONFIG_BCM96855)
            if (caps & ~(PHY_CAP_AUTONEG|PHY_CAP_1000_FULL|PHY_CAP_PAUSE))
#else
            if (caps & ~(PHY_CAP_AUTONEG|PHY_CAP_1000_FULL))
#endif
            {
                BCM_LOG_NOTICE(BCM_LOG_ID_AE, "ignore unsupported autoneg capabilities\n");
            }
            ae_int_phy_caps_set_hw(phy_dev, caps);
            break;
        default:
#if defined (CONFIG_BCM96855)
            if (caps & ~(PHY_CAP_PAUSE))
#endif
            BCM_LOG_NOTICE(BCM_LOG_ID_AE, "ignore autoneg capabilities\n");
            break;
    }
    
    return 0;
}

int ae_int_phy_caps_get(phy_dev_t *phy_dev, int caps_type, uint32_t *caps)
{
    LaserRate rate = AeGetRate();
    
    *caps = 0;
    if ((caps_type != CAPS_TYPE_ADVERTISE) && (caps_type != CAPS_TYPE_SUPPORTED))
        return 0;

    ae_int_phy_pause_get(phy_dev, caps);

    switch (rate)
    {
        case LaserRate10G:
            *caps |= PHY_CAP_10000;
            break;
        case LaserRate2G:
            *caps |= PHY_CAP_2500;
            break;
        case LaserRate1G:   /* Only 1G mode support autoneg */
            ae_int_phy_caps_get_hw(phy_dev, caps);
            
            if (caps_type == CAPS_TYPE_SUPPORTED)
            {
                *caps |= PHY_CAP_AUTONEG | PHY_CAP_1000_FULL;
            }
            else
            {
                *caps |= PHY_CAP_1000_FULL;
            }
            break;
        default:
            break;
    }
    
    return 0;
}

int ae_int_phy_speed_set(phy_dev_t *phy_dev, phy_speed_t speed, phy_duplex_t duplex)
{
    mac_cfg_t mac_cfg;
    
    BCM_LOG_DEBUG(BCM_LOG_ID_AE, "PHY speed %d, duplex %d\n", speed, duplex);
    
    switch(speed)
    {
        case PHY_SPEED_1000:
            mac_cfg.speed = MAC_SPEED_1000;
            ae_int_phy_caps_set_hw(phy_dev, 0);
            break;
        case PHY_SPEED_2500:
            mac_cfg.speed = MAC_SPEED_2500;
            break;
#if defined (CONFIG_BCM96856) || defined(CONFIG_BCM96855)
        case PHY_SPEED_5000:
            mac_cfg.speed = MAC_SPEED_5000;
            break;
#endif
        case PHY_SPEED_10000:
            mac_cfg.speed = MAC_SPEED_10000;
            break;
        case PHY_SPEED_AUTO:
            ae_int_phy_caps_set_hw(phy_dev, PHY_CAP_AUTONEG);
            return 0;
        default:
            BCM_LOG_ERROR(BCM_LOG_ID_AE, "speed can't be supported by AE internal phy\n");
            return -1;
    }
    return epon_ae_mac_cfg_set(NULL, &mac_cfg);
}


static int ae_int_phy_power_set(phy_dev_t *phy_dev, int enable)
{
    int_phy_enabled = enable;
    
    if (enable)
    {
        AeLaserTxModeSet(LaserTxModeContinuous);
        AeLaserRxPowerSet(1);
    }
    else
    {
        AeLaserTxModeSet(LaserTxModeOff);
        AeLaserRxPowerSet(0);
    }

    return 0;
}

static int ae_int_phy_power_get(phy_dev_t *phy_dev, int *enable)
{
    *enable = ae_int_phy_enabled();
    
    return 0;
}

int ae_int_phy_enabled()
{

    return int_phy_enabled;
}

int ae_int_phy_drv_set()
{
    phy_drv_pon.caps_set = ae_int_phy_caps_set;
    phy_drv_pon.caps_get = ae_int_phy_caps_get;
    phy_drv_pon.speed_set = ae_int_phy_speed_set;
    phy_drv_pon.power_set = ae_int_phy_power_set;
    phy_drv_pon.power_get = ae_int_phy_power_get;

    return 0;
}
