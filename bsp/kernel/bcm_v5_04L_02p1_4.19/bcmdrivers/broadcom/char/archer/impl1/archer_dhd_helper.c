/*
* <:copyright-BRCM:2021:proprietary:standard
* 
*    Copyright (c) 2021 Broadcom 
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

/*
*******************************************************************************
*
* File Name  : archer_dhd_helper.c
*
* Description: Archer DHD API
*
*******************************************************************************
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/bcm_log.h>
#include <linux/nbuff.h>

#include "sysport_rsb.h"
#include "sysport_classifier.h"

#include "archer.h"
#include "archer_driver.h"
#include "archer_thread.h"
#include "archer_dhd.h"

// FIXME
int bdmf_global_trace_level = 0;
EXPORT_SYMBOL(bdmf_global_trace_level);

int bdmf_new_and_set(int unused, void *unused_ptr, bdmf_object_handle mo_, bdmf_object_handle *radio_handle)
{
    archer_dhd_helper_attr_t *attr_p = (archer_dhd_helper_attr_t *)mo_;
    uint32_t radio_idx = attr_p->radio_idx;

    if(radio_idx >= ARCHER_DHD_RADIO_MAX)
    {
        __logError("Invalid radio_idx: %u", radio_idx);

        return -1;
    }

    *radio_handle = (bdmf_object_handle)&archer_dhd_g.radio[radio_idx];

    bcm_print("bdmf_new_and_set: radio_idx %u, radio_handle 0x%px\n", radio_idx, *radio_handle);

    return 0;
}
EXPORT_SYMBOL(bdmf_new_and_set);

int bdmf_destroy(bdmf_object_handle mo)
{
//    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    // FIXME

    return 0;
}
EXPORT_SYMBOL(bdmf_destroy);

static void dhd_init_cfg_dump(rdpa_dhd_init_cfg_t *init_cfg)
{
    bcm_print("Initial configuration\n");
    bcm_print("=================================\n");
    bcm_print("\trx_post_flow_ring_base_addr : %px\n", init_cfg->rx_post_flow_ring_base_addr);
    bcm_print("\ttx_post_flow_ring_base_addr : %px\n", init_cfg->tx_post_flow_ring_base_addr);
    bcm_print("\trx_complete_flow_ring_base_addr : %px\n", init_cfg->rx_complete_flow_ring_base_addr);
    bcm_print("\ttx_complete_flow_ring_base_addr : %px\n", init_cfg->tx_complete_flow_ring_base_addr);
    bcm_print("\n");
    bcm_print("\tr2d_wr_arr_base_addr : %px, phys_addr : %px\n", init_cfg->r2d_wr_arr_base_addr, (void *)(uintptr_t)init_cfg->r2d_wr_arr_base_phys_addr);
    bcm_print("\td2r_rd_arr_base_addr : %px, phys_addr : %px\n", init_cfg->d2r_rd_arr_base_addr, (void *)(uintptr_t)init_cfg->d2r_rd_arr_base_phys_addr);
    bcm_print("\tr2d_rd_arr_base_addr : %px, phys_addr : %px\n", init_cfg->r2d_rd_arr_base_addr, (void *)(uintptr_t)init_cfg->r2d_rd_arr_base_phys_addr);
    bcm_print("\td2r_wr_arr_base_addr : %px, phys_addr : %px\n", init_cfg->d2r_wr_arr_base_addr, (void *)(uintptr_t)init_cfg->d2r_wr_arr_base_phys_addr);
    bcm_print("\ttx_post_mgmt_arr_base_addr : %px, phys_addr : %px\n", init_cfg->tx_post_mgmt_arr_base_addr, (void *)(uintptr_t)init_cfg->tx_post_mgmt_arr_base_phys_addr);
    bcm_print("\n");

    bcm_print("\tDoorbell Post Wakeup register : phy_addr: 0x%x, virt_addr: %px\n", init_cfg->dongle_wakeup_register, init_cfg->dongle_wakeup_register_virt);
    bcm_print("\tDoorbell Complete Wakeup register : phy_addr: 0x%x, virt_addr: %px\n", init_cfg->dongle_wakeup_register_2, init_cfg->dongle_wakeup_register_2_virt);
    bcm_print("\tDoorbell ISR : %px\n", init_cfg->doorbell_isr);
    bcm_print("\tDoorbell CTX : %px\n", init_cfg->doorbell_ctx);

    bcm_print("\tadd_llcsnap_header : %d\n", init_cfg->add_llcsnap_header);
    bcm_print("\tflow_ring_format : %d\n", init_cfg->flow_ring_format);
    bcm_print("\tidma_active : %d\n", init_cfg->dongle_wakeup_hwa);
    bcm_print("\thbqd mode : %d\n", init_cfg->hbqd_mode);
}

int rdpa_dhd_helper_init_cfg_set(bdmf_object_handle mo_, rdpa_dhd_init_cfg_t * init_cfg)
{
    archer_dhd_helper_attr_t *attr_p = (archer_dhd_helper_attr_t *)mo_;
    uint32_t radio_idx = attr_p->radio_idx;
    int rc;

    if(radio_idx >= ARCHER_DHD_RADIO_MAX)
    {
        __logError("Invalid radio_idx: %u", radio_idx);

        return -1;
    }

    dhd_init_cfg_dump(init_cfg);

    rc = rdd_dhd_hlp_cfg(radio_idx, init_cfg, 1);
    if(rc)
    {
        __logError("Failed to initialize Radio %u, rc %d\n", radio_idx, rc);
    }

    return rc;
}
EXPORT_SYMBOL(rdpa_dhd_helper_init_cfg_set);

/* "flush" attribute "write" callback */
int rdpa_dhd_helper_flush_set(bdmf_object_handle mo_, uint32_t flush)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    return rdp_drv_dhd_helper_flow_ring_flush(radio_ptr->radio_idx, flush);
}
EXPORT_SYMBOL(rdpa_dhd_helper_flush_set);

/* "flow_ring_enable" attribute "write" callback */
int rdpa_dhd_helper_flow_ring_enable_set(bdmf_object_handle mo_, uint32_t flow_ring_idx, int enable)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    if(flow_ring_idx < 2)
    {
        __logError("flow_ring_idx (%u) < 2", flow_ring_idx);

        return BDMF_ERR_PARM;
    }

    return rdp_drv_dhd_helper_flow_ring_enable_set(radio_ptr->radio_idx, flow_ring_idx, enable);
}
EXPORT_SYMBOL(rdpa_dhd_helper_flow_ring_enable_set);

/* "rx_post_init" attribute "write" callback */
int rdpa_dhd_helper_rx_post_init(bdmf_object_handle mo_)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    return rdp_drv_rx_post_init(radio_ptr->radio_idx, DHD_RX_POST_FLOW_RING_SIZE-1) ? BDMF_ERR_NOMEM : 0;
}
EXPORT_SYMBOL(rdpa_dhd_helper_rx_post_init);

/* "rx_post_uninit" attribute "write" callback */
int rdpa_dhd_helper_rx_post_uninit(bdmf_object_handle mo_)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;
    uint32_t num_items;

    return rdp_drv_dhd_rx_post_uninit(radio_ptr->radio_idx, &num_items) ? BDMF_ERR_NOMEM : 0;
}
EXPORT_SYMBOL(rdpa_dhd_helper_rx_post_uninit);

/* "rx_post_reinit" attribute "write" callback */
int rdpa_dhd_helper_rx_post_reinit(bdmf_object_handle mo_)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    return rdp_drv_dhd_rx_post_reinit(radio_ptr->radio_idx) ? BDMF_ERR_NOMEM : 0;
}
EXPORT_SYMBOL(rdpa_dhd_helper_rx_post_reinit);

int rdpa_dhd_helper_tx_complete_send2host_set(bdmf_object_handle mo_, int tx_complete_send2host)
{
    /* not supported in Archer -return 0 in order not to fail upper driver*/
    return 0;
}
EXPORT_SYMBOL(rdpa_dhd_helper_tx_complete_send2host_set);

/* "ssid_tx_dropped_packets" attribute "read" callback */
int rdpa_dhd_helper_ssid_tx_dropped_packets_get(bdmf_object_handle mo_, uint32_t ai_, uint32_t *ssid_tx_dropped_packets)
{
//    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;
//    uint32_t ssid = ai_;

    // FIXME
    *ssid_tx_dropped_packets = 0;

    return 0;
}
EXPORT_SYMBOL(rdpa_dhd_helper_ssid_tx_dropped_packets_get);

/* "int_connect" attribute "write" callback */
int rdpa_dhd_helper_int_connect_set(bdmf_object_handle mo_, int int_connect)
{
    /* This callback not required, as we are awaking dongle directly */

    return 0;
}
EXPORT_SYMBOL(rdpa_dhd_helper_int_connect_set);

#if defined(RDPA_DHD_HELPER_FEATURE_LBRAGGR_SUPPORT)
int rdpa_dhd_helper_aggregation_size_set(bdmf_object_handle mo_, int access_category, int aggregation_size)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    return rdd_dhd_helper_aggregation_size_set(radio_ptr, access_category, aggregation_size);
}
EXPORT_SYMBOL(rdpa_dhd_helper_aggregation_size_set);

int rdpa_dhd_helper_aggregation_timer_set(bdmf_object_handle mo_, int aggregation_timer)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;
    int access_category;
    int ret = 0;

    for(access_category=0; access_category<RDPA_MAX_AC; ++access_category)
    {
        ret |= rdd_dhd_helper_aggregation_timeout_set(radio_ptr, access_category, aggregation_timer);
    }

    return ret;
}
EXPORT_SYMBOL(rdpa_dhd_helper_aggregation_timer_set);
#endif /* RDPA_DHD_HELPER_FEATURE_LBRAGGR_SUPPORT */

int rdpa_cpu_get(uint32_t radio_idx, bdmf_object_handle *radio_handle)
{
    if(radio_idx >= ARCHER_DHD_RADIO_MAX)
    {
        __logError("Invalid radio_idx: %u", radio_idx);

        return BDMF_ERR_NOENT;
    }

    *radio_handle = (bdmf_object_handle)&archer_dhd_g.radio[radio_idx];

    return 0;
}
EXPORT_SYMBOL(rdpa_cpu_get);

int rdpa_cpu_num_queues_get(bdmf_object_handle mo_, bdmf_number *num_queues)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    *num_queues = radio_ptr->radio_idx + 1;

    return 0;
}
EXPORT_SYMBOL(rdpa_cpu_num_queues_get);

int rdpa_cpu_rxq_cfg_set(bdmf_object_handle mo_, uint32_t ai_, const rdpa_cpu_rxq_cfg_t *rxq_cfg)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    radio_ptr->cpu_cfg = *rxq_cfg;

    return 0;
}
EXPORT_SYMBOL(rdpa_cpu_rxq_cfg_set);

int rdpa_cpu_index_get(bdmf_object_handle mo_, rdpa_cpu_port *index_ptr)
{
    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    *index_ptr = radio_ptr->radio_idx;

    return 0;
}
EXPORT_SYMBOL(rdpa_cpu_index_get);

void rdpa_cpu_int_enable(uint32_t radio_idx, int queue)
{
    archer_dhd_radio_t *radio_ptr = &archer_dhd_g.radio[radio_idx];
    rdpa_cpu_rxq_cfg_t *cpu_ptr = &radio_ptr->cpu_cfg;

    cpu_ptr->irq_enable = 1;

    if(cpu_ptr->irq_status)
    {
        archer_dhd_cpu_queue_notify(radio_idx);
    }
}
EXPORT_SYMBOL(rdpa_cpu_int_enable);

void rdpa_cpu_int_disable(uint32_t radio_idx, int queue)
{
    rdpa_cpu_rxq_cfg_t *cpu_ptr = &archer_dhd_g.radio[radio_idx].cpu_cfg;

    cpu_ptr->irq_enable = 0;
}
EXPORT_SYMBOL(rdpa_cpu_int_disable);

void rdpa_cpu_int_clear(uint32_t radio_idx, int queue)
{
    rdpa_cpu_rxq_cfg_t *cpu_ptr = &archer_dhd_g.radio[radio_idx].cpu_cfg;

    cpu_ptr->irq_status = 0;
}
EXPORT_SYMBOL(rdpa_cpu_int_clear);

int rdpa_cpu_rxq_flush_set(bdmf_object_handle mo_, uint32_t ai_, int rxq_flush)
{
    // FIXME

//    archer_dhd_radio_t *radio_ptr = (archer_dhd_radio_t *)mo_;

    return 0;
}
EXPORT_SYMBOL(rdpa_cpu_rxq_flush_set);

/* get wakeup information to propagate it dongle ( used for dingle to wake up runner directly */
void rdpa_dhd_helper_wakeup_information_get(rdpa_dhd_wakeup_info_t *wakeup_info)
{
    rdp_drv_dhd_helper_wakeup_information_get(wakeup_info);
}
EXPORT_SYMBOL(rdpa_dhd_helper_wakeup_information_get);

/* creates DHD Tx complete Ring with given size */
int rdpa_dhd_helper_dhd_complete_ring_create(uint32_t radio_idx, uint32_t ring_size)
{
    return rdp_drv_dhd_helper_dhd_complete_ring_create(radio_idx, ring_size);
}
EXPORT_SYMBOL(rdpa_dhd_helper_dhd_complete_ring_create);

/* destroys DHD Tx complete Ring with given size */
int rdpa_dhd_helper_dhd_complete_ring_destroy(uint32_t radio_idx, uint32_t ring_size)
{
    return rdp_drv_dhd_helper_dhd_complete_ring_destroy(radio_idx, ring_size);
}
EXPORT_SYMBOL(rdpa_dhd_helper_dhd_complete_ring_destroy);

EXPORT_SYMBOL(rdp_drv_dhd_cpu_tx);
EXPORT_SYMBOL(rdp_drv_dhd_cpu_rx);
EXPORT_SYMBOL(rdp_drv_dhd_helper_dhd_complete_message_get);
EXPORT_SYMBOL(rdp_drv_dhd_complete_wakeup);
EXPORT_SYMBOL(archer_dhd_recycle);
