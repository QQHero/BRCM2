/*
 * SW probe response module source file
 * disable ucode sending probe response,
 * driver will decide whether send probe response,
 * after check the received probe request.
 * Copyright 2022 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_probresp.c 810737 2022-04-13 06:04:10Z $
 */

#include <wlc_cfg.h>

#ifdef WLPROBRESP_SW

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_tpc.h>
#include <wlc_csa.h>
#include <wlc_quiet.h>
#include <wlc_bmac.h>
#include <wlc_probresp.h>
#include <wlc_probresp_mac_filter.h>
#include <wlc_pcb.h>
#include <wlc_assoc.h>
#include <wlc_scan.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_vs.h>
#include <wlc_bsscfg_viel.h>

#if defined(TD_EASYMESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN) && !defined(CONFIG_UGW_BCM_DONGLE)
#include <td_easymesh.h>
#else
#if defined(TD_EASYMESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
#include "td_easymesh_common.h"
#include <wlc_td_extend.h>
#endif
#endif

#if defined(CONFIG_TENDA_PRIVATE_WLAN)
#include "td_wl_core_symb.h"
#endif

#ifdef AP
#include <wlc_ap.h>
#endif /* AP */
#ifdef WL_OCE
#include <wlc_oce.h>
#endif

#ifdef WL_MBSSID
#include <wlc_mbss.h>
#endif /* TESTBED_AP_11AX */

#if BAND6G || defined(WL_OCE_AP)
#include <wlc_rrm.h>
#endif /* BAND6G || WL_OCE_AP */

#if defined(TD_PRIVATE_IE) && defined(CONFIG_TENDA_PRIVATE_WLAN)
#if (!defined(CONFIG_UGW_BCM_DONGLE))
#include <td_ie_manage.h>
#else
#include <td_private_ie_common.h>
#endif
#endif

#define WLC_PROBRESP_MAXFILTERS        3    /* max filter functions */
#define WLC_PROBRESP_INVALID_INDEX    -1
#define WLC_PROBERSP_LIFETIME        35    /* msec (station leaves channel after 40ms) */

#define PROBRESP_BLOCK_SEC_ENABLED(mprobresp)    ((mprobresp)->probresp_block_sec)

#define WL_PROBRESP_LEN            2048

typedef    struct probreqcb {
    void *hdl;
    probreq_filter_fn_t filter_fn;
} probreqcb_t;

/* IOVar table */
/* No ordering is imposed */
enum {
    IOV_PROBRESP_SW = 0, /* SW probe response enable/disable */
    IOV_PRS_RTX_LIMIT = 1, /* Probe response retry limit */
    IOV_PROBRESP_BLOCK_SEC = 2,    /* Block Probe Resp. for Probe Req. received on secondary
                     * sub bands enable/disable
                     */
    IOV_LAST
};

static const bcm_iovar_t wlc_probresp_iovars[] = {
    {"probresp_sw", IOV_PROBRESP_SW, (0), 0, IOVT_BOOL, 0},
    {"prslimit", IOV_PRS_RTX_LIMIT, (0), 0, IOVT_UINT8, 0},
    {"probresp_block_sec", IOV_PROBRESP_BLOCK_SEC,
    (IOVF_BSSCFG_AP_ONLY), 0, IOVT_BOOL, 0},
    {NULL, 0, 0, 0, 0, 0},
};

/* SW probe response module info */
struct wlc_probresp_info {
    wlc_info_t *wlc;
    probreqcb_t probreq_filters[WLC_PROBRESP_MAXFILTERS];
    int p2p_index;
    bool probresp_block_sec;
    uint8 * vndr_ie_ss_active;
};

/* local functions */
/* module */
static int wlc_probresp_doiovar(void *ctx, uint32 actionid,
    void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif);
#ifdef WLPROBRESP_INTRANSIT_FILTER
static void wlc_proberesp_cb(wlc_info_t *wlc, void *pkt, uint txs);
#endif /* WLPROBRESP_INTRANSIT_FILTER */

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

static uint
wlc_probresp_calc_vs_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
    int ielen = 0;
    wlc_probresp_info_t *mprobresp = (wlc_probresp_info_t *)ctx;
    vndr_ie_t *vndr_ie_data = NULL;

    if (mprobresp && mprobresp->vndr_ie_ss_active) {
        vndr_ie_data = (vndr_ie_t *)mprobresp->vndr_ie_ss_active;
        ielen = vndr_ie_data->len + VNDR_IE_HDR_LEN;
    }

    return ielen;
}

static int
wlc_probresp_write_vs_ie(void *ctx, wlc_iem_build_data_t *data)
{
    wlc_probresp_info_t *mprobresp = (wlc_probresp_info_t *)ctx;
    vndr_ie_t *vndr_ie_data = NULL;

    if (mprobresp && mprobresp->vndr_ie_ss_active) {
        vndr_ie_data = (vndr_ie_t *)mprobresp->vndr_ie_ss_active;
        bcm_write_tlv_safe(vndr_ie_data->id, vndr_ie_data->oui, vndr_ie_data->len,
            data->buf, data->buf_len);
    }
    return BCME_OK;
}

wlc_probresp_info_t *
BCMATTACHFN(wlc_probresp_attach)(wlc_info_t *wlc)
{
    wlc_probresp_info_t *mprobresp;

    if (!wlc)
        return NULL;

    mprobresp = MALLOCZ(wlc->osh, sizeof(wlc_probresp_info_t));
    if (mprobresp == NULL) {
        WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
            wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
        goto fail;
    }
    mprobresp->wlc = wlc;
    mprobresp->p2p_index = WLC_PROBRESP_INVALID_INDEX;
    mprobresp->probresp_block_sec = TRUE;

    /* keep the module registration the last other add module unregistration
     * in the error handling code below...
     */
    if (wlc_module_register(wlc->pub, wlc_probresp_iovars, "probresp", mprobresp,
        wlc_probresp_doiovar, NULL, NULL, NULL) != BCME_OK) {
        WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
                  wlc->pub->unit, __FUNCTION__));
        goto fail;
    };

#ifdef WLPROBRESP_INTRANSIT_FILTER
    if (wlc_pcb_fn_set(wlc->pcb, 0, WLF2_PCB1_PROBRESP, wlc_proberesp_cb) != BCME_OK) {
        WL_ERROR(("wl%d: %s wlc_pcb_fn_set() failed\n", wlc->pub->unit, __FUNCTION__));
        goto fail;
    }
#endif /* WLPROBRESP_INTRANSIT_FILTER */

    wlc_enable_probe_req(wlc, PROBE_REQ_PROBRESP_MASK, PROBE_REQ_PROBRESP_MASK);
    wlc_disable_probe_resp(wlc, PROBE_RESP_SW_MASK, PROBE_RESP_SW_MASK);
    wlc->pub->_probresp_sw = TRUE;

    if (wlc_iem_vs_add_build_fn_mft(wlc->iemi, FT2BMP(FC_PROBE_RESP), WLC_IEM_VS_IE_PRIO_VNDR,
          wlc_probresp_calc_vs_ie_len, wlc_probresp_write_vs_ie, mprobresp) != BCME_OK) {
        WL_ERROR(("wl%d: %s wlc_iem_vs_add_build_fn_mft() failed\n",
                  wlc->pub->unit, __FUNCTION__));
        goto fail;
    }

    return mprobresp;

    /* error handling */
fail:
    wlc_probresp_detach(mprobresp);

    return NULL;
}

void
BCMATTACHFN(wlc_probresp_detach)(wlc_probresp_info_t *mprobresp)
{
    wlc_info_t *wlc;

    if (!mprobresp)
        return;
    wlc = mprobresp->wlc;
    wlc_module_unregister(wlc->pub, "probresp", mprobresp);
#ifdef WLPROBRESP_INTRANSIT_FILTER
    (void) wlc_pcb_fn_set(wlc->pcb, 0, WLF2_PCB1_PROBRESP, NULL);
#endif /* WLPROBRESP_INTRANSIT_FILTER */
    MFREE(wlc->osh, mprobresp, sizeof(wlc_probresp_info_t));
}

static int
wlc_probresp_doiovar(void *ctx, uint32 actionid,
    void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
    wlc_probresp_info_t *mprobresp = (wlc_probresp_info_t *)ctx;
    wlc_info_t *wlc = mprobresp->wlc;
    wlc_bsscfg_t *bsscfg;
    int err = 0;
    int32 int_val = 0;
    int32 *ret_int_ptr;
    bool bool_val;

    /* update bsscfg w/provided interface context */
    bsscfg = wlc_bsscfg_find_by_wlcif(wlc, wlcif);
    ASSERT(bsscfg != NULL);

    /* convenience int and bool vals for first 8 bytes of buffer */
    if (p_len >= (int)sizeof(int_val))
        bcopy(params, &int_val, sizeof(int_val));

    /* convenience int ptr for 4-byte gets (requires int aligned arg) */
    ret_int_ptr = (int32 *)arg;

    bool_val = (int_val != 0);

    /* update wlcif pointer */
    if (wlcif == NULL)
        wlcif = bsscfg->wlcif;
    ASSERT(wlcif != NULL);

    /* Do the actual parameter implementation */
    switch (actionid) {
    case IOV_GVAL(IOV_PROBRESP_SW):
        *ret_int_ptr = (int32)wlc->pub->_probresp_sw;
        break;
    case IOV_SVAL(IOV_PROBRESP_SW):
        if (UCPRS_DISABLED(wlc->pub)) {
            err = BCME_UNSUPPORTED;
        }
        if (wlc->pub->_probresp_sw != bool_val) {
            wlc->pub->_probresp_sw = bool_val;

            if (wlc->pub->up && AP_ENAB(wlc->pub)) {
                /* ensure templates are ready */
                wlc_update_probe_resp(wlc, TRUE);
            }

            wlc_enable_probe_req(wlc, PROBE_REQ_PROBRESP_MASK,
                bool_val ? PROBE_REQ_PROBRESP_MASK : 0);

            wlc_disable_probe_resp(wlc, PROBE_RESP_SW_MASK,
                bool_val ? PROBE_RESP_SW_MASK : 0);
        }
        break;

    case IOV_GVAL(IOV_PRS_RTX_LIMIT):
        *ret_int_ptr = wlc->probe_rtx_limit;
        break;

    case IOV_SVAL(IOV_PRS_RTX_LIMIT):
        if (D11REV_LT(wlc->pub->corerev, 40)) {
            err = BCME_UNSUPPORTED;
            break;
        }
        if (int_val >= 0 && int_val <= MAX_PRS_RTX) {
            wlc->probe_rtx_limit = int_val;
            if (wlc->pub->up) {
                wlc_write_shm(wlc, M_PRS_RETRY_THR(wlc), wlc->probe_rtx_limit);
            }
        }
        else
            err = BCME_RANGE;
        break;
    case IOV_SVAL(IOV_PROBRESP_BLOCK_SEC):
        if (bool_val)
            mprobresp->probresp_block_sec = TRUE;
        else
            mprobresp->probresp_block_sec = FALSE;
        break;

    case IOV_GVAL(IOV_PROBRESP_BLOCK_SEC):
        *ret_int_ptr = mprobresp->probresp_block_sec;
        break;

    default:
        err = BCME_UNSUPPORTED;
        break;
    }

    return err;
}

static void
wlc_probresp_send_probe_resp(wlc_probresp_info_t *mprobresp, wlc_bsscfg_t *bsscfg,
    struct ether_addr *da, ratespec_t rate_override, wlc_prb_rqst_param_t *req_param)
{
    wlc_info_t *wlc;
    void *p;
    uint8 *pbody;
    int len;

    wlc = mprobresp->wlc;

    ASSERT(bsscfg != NULL);

    ASSERT(wlc->pub->up);

    len = WL_PROBRESP_LEN;

#ifdef BCMPCIEDEV
     /* for full dongle operation, do not queue Probe response packets
      * while channel switch is in progress
      */
    if (BCMPCIEDEV_ENAB() && (wlc_bmac_tx_fifo_suspended(wlc->hw, TX_CTL_FIFO)))
        return;
#endif /* BCMPCIEDEV */
    /* build response and send */
    if ((p = wlc_frame_get_mgmt(wlc, FC_PROBE_RESP, da, &bsscfg->cur_etheraddr,
                                &bsscfg->BSSID, len, &pbody)) == NULL) {
        WL_ERROR(("wl%d.%d: %s: wlc_frame_get_mgmt failed\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
    }
    else {
#ifdef WLPROBRESP_INTRANSIT_FILTER
        if (!wlc_probresp_intransit_filter_add_da(wlc->mprobresp_mac_filter, bsscfg, da)) {
            WL_ERROR(("wl%d.%d: %s: no mem for intransit filter!\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
        }
        WLF2_PCB1_REG(p, WLF2_PCB1_PROBRESP);
#endif /* WLPROBRESP_INTRANSIT_FILTER */

        mprobresp->vndr_ie_ss_active = wlc_vndr_ie_find_by_sta(wlc->vieli,
            bsscfg, da, VNDR_IE_PRBRSP_FLAG);

        /* Generate probe response body */
        wlc_bcn_prb_body(wlc, FC_PROBE_RESP, bsscfg, pbody, &len, (void*)req_param);
        PKTSETLEN(wlc->osh, p, len + DOT11_MGMT_HDR_LEN);

        /* Configure the lifetime of proberesponse. Keep our own station in mind and
         * use a value that makes sense. There is no use to send a proberesponse after
         * a long time.
         */
        wlc_lifetime_set(wlc, p, WLC_PROBERSP_LIFETIME * 1000);

        /* Ensure that pkt is not re-enqueued to FIFO after suppress */
        WLPKTTAG(p)->flags3 |= WLF3_TXQ_SHORT_LIFETIME;
        /* let bcast probe resp packet avoid BCMC PSQ */
        if (ETHER_ISMULTI(da)) {
            wlc_queue_80211_frag(wlc, p, bsscfg->wlcif->qi, wlc->band->hwrs_scb,
                bsscfg, FALSE, NULL, rate_override);
        } else {
            wlc_queue_80211_frag(wlc, p, bsscfg->wlcif->qi, NULL, bsscfg, FALSE, NULL,
            rate_override);
        }
    }
}

/*
 * reply to probe request. First apply filters to determine if a probe response should be sent
 * at all. If all filters "pass" then send probe response.
 */
static void
wlc_probresp_filter_and_reply(wlc_probresp_info_t *mprobresp, wlc_bsscfg_t *cfg,
    wlc_d11rxhdr_t *wrxh, uint8 *plcp, struct dot11_management_header *hdr,
    uint8 *body, int body_len)
{
    wlc_info_t *wlc = cfg->wlc;
    int i;
    bool sendProbeResp = TRUE;
#if defined(WL_OCE) && defined(WL_OCE_AP)
    bool oce_prb_rqst = FALSE;
#endif /* WL_OCE && WL_OCE_AP */
    ratespec_t rate_override = 0;
    bcm_tlv_t *ssid;
    wlc_bsscfg_t *bsscfg = NULL;
    he_short_ssid_list_ie_t *s_ssid = NULL;
    uint32 local_short_ssid = 0;
    wlc_prb_rqst_param_t prb_rqst_param;
    bool ssid_match = FALSE;
    bool short_ssid_match = FALSE;
#if defined(TD_PRIVATE_IE) && defined(CONFIG_TENDA_PRIVATE_WLAN)
    bcm_tlv_t *ie = NULL; 
    int ie_body_len = body_len;
#endif

#ifdef AP
    if (BSSCFG_AP(cfg) && !wlc_ap_on_chan(wlc->ap, cfg)) {
        WL_ERROR(("wl%d.%d: %s: AP not ON channel. Not processing further\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(cfg), __FUNCTION__));
        return;
    }
#endif /* AP */
#if defined(TD_EASYMESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
    /* if agent mode , should deal loop, hongguiyang */
    if (!(g_em_mesh_cfg & EM_CONTROLLER_BIT) 
            && ((g_em_mesh_cfg & EM_XMESH_BIT) || (cfg->map_attr & MAP_EXT_ATTR_BACKHAUL_BSS))) {
#ifdef CONFIG_UGW_BCM_DONGLE
        if(!TDCORE_FUNC_IS_NULL(td_em_is_extend_intf_assoc)){
            if (!TDCORE_FUNC(td_em_is_extend_intf_assoc)(cfg)) {
                return;
            }
        }
#else
        if (!td_em_is_extend_intf_assoc(cfg)) {
            return;
        }
#endif
    }
    /* if soft set prio assoc flag, other ap need dont assoc 5s */
    if ((g_em_mesh_cfg & EM_PRIO_ASSOC_BIT) 
            && ((g_em_mesh_cfg & EM_XMESH_BIT) || (cfg->map_attr & MAP_EXT_ATTR_BACKHAUL_BSS))) {
        static long unsigned int prio_assoc_old_time = 0;
        static int flag = 0;
        if (!flag) {
#ifdef CONFIG_UGW_BCM_DONGLE
            prio_assoc_old_time = tdextend_jiffies;
#else
            prio_assoc_old_time = jiffies;
            printk("%s, first time(%lu).\n", __func__, jiffies);
#endif
            flag = 1;
        }
#ifdef CONFIG_UGW_BCM_DONGLE
        if(tdextend_jiffies > prio_assoc_old_time + 20) {
            g_em_mesh_cfg &= (~EM_PRIO_ASSOC_BIT);
            flag = 0;
        }
#else
        if (time_after(jiffies, (prio_assoc_old_time + 20*HZ))) {
            g_em_mesh_cfg &= (~EM_PRIO_ASSOC_BIT);
            flag = 0;
            printk("%s, mesh_cfg(%x) 20s old_time(%lu).\n", __func__, g_em_mesh_cfg, jiffies);
        }
#endif
        return;
    }
#endif

#if defined(TD_PRIVATE_IE) && defined(CONFIG_TENDA_PRIVATE_WLAN) //begain add by bcz
    //遍历221字段，并解析td私有ie字段
    for(ie = bcm_parse_tlvs(body, body_len, DOT11_MNG_VS_ID);ie != NULL; (ie = bcm_next_tlv(ie, &ie_body_len))) {
        if((DOT11_MNG_VS_ID == ie->id)) {
            TDCORE_FUNC_IF(td_recv_ie) {
                TDCORE_FUNC(td_recv_ie)((td_element_t *)ie, cfg, mprobresp->wlc, hdr->sa.octet, TD_PE_FRAME_TYPE_PROBE_REQ, wrxh->rssi);
            }
        }
    }
#endif // end add by bcz

    /*
     * Block Probe Response for Probe Request received on secondary sub-band to prevent
     * exhaustive retries of Probe Responses sent on primary sub-band.
     *
     * The use of dot11acphy register drop20sCtrl1 and obss_param_extra to abort packets
     * received on secondary sub-bands will make this SW-filter obsolete.
     *
     * WAR BCAWLAN-214668: BCM43217 CoreRev 30 falsely marks Probe Request received on primary,
     * as being received on secondary sub-band.
     */
    if (!BAND_6G(wlc->band->bandtype) && PROBRESP_BLOCK_SEC_ENABLED(mprobresp) &&
        D11REV_GT(wlc->pub->corerev, 30)) {
        /* compare sub-band of received frame against primary channel */
        chanspec_t chspec_prb = wlc_recv_mgmt_rx_chspec_get(wlc, wrxh);
        chanspec_t chspec_pri = wf_ctlchspec20_from_chspec(cfg->current_bss->chanspec);

        if (chspec_prb != chspec_pri) {
            WL_INFORM(("wl%d: Probe Req. rxch 0x%x not on primary sub-band ch 0x%x\n",
                        wlc->pub->unit,
                        CHSPEC_CHANNEL(chspec_prb),
                        CHSPEC_CHANNEL(chspec_pri)));
            return;
        }
    }

    /* Check for existence of SSID tlv */
    ssid = bcm_find_tlv(body, body_len, DOT11_MNG_SSID_ID);

    s_ssid = (he_short_ssid_list_ie_t*)bcm_find_tlv_ext(body, body_len, DOT11_MNG_ID_EXT_ID,
            EXT_MNG_SHORT_SSID_ID);
    /* if network type is closed then don't send prob response unless the request contains
     * our ssid (hidden ssid probe response). CLOSED_NET configuration in line with ucode
     * handling.
     */
    if (cfg->closednet || cfg->nobcnssid) {
        /* Check if the ssid in the probe request matches our bsscfg->SSID */
        if (ssid && (ssid->len == cfg->SSID_len) &&
            (bcmp(ssid->data, cfg->SSID, ssid->len) == 0)) {
            ssid_match = TRUE;
        }

        if (s_ssid) {
            local_short_ssid = ~hndcrc32(cfg->SSID, cfg->SSID_len, CRC32_INIT_VALUE);
            for (i = 0; i < s_ssid->len / sizeof(s_ssid->short_ssid[0]); i++) {
                if (s_ssid->short_ssid[i] == local_short_ssid) {
                    short_ssid_match = TRUE;
                    break;
                }
            }
        }
        if (!ssid_match && !short_ssid_match) {
            WL_INFORM(("wl%d:%d No probe response, mismatch both ssid and short"
                " ssid for hidden network\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(cfg)));
            return;
        }
    }

    for (i = 0; i < WLC_PROBRESP_MAXFILTERS; i ++) {
        if ((i != mprobresp->p2p_index) && mprobresp->probreq_filters[i].hdl) {
            if (!(mprobresp->probreq_filters[i].filter_fn(
                mprobresp->probreq_filters[i].hdl, cfg,
                wrxh, plcp, hdr, body, body_len, NULL)))
                sendProbeResp = FALSE;
        }
    }

    /* call p2p filter function last */
    if (mprobresp->p2p_index != WLC_PROBRESP_INVALID_INDEX) {
        sendProbeResp = mprobresp->probreq_filters[mprobresp->p2p_index].filter_fn(
            mprobresp->probreq_filters[mprobresp->p2p_index].hdl, cfg,
            wrxh, plcp, hdr, body, body_len, &sendProbeResp);
    }

    if (!sendProbeResp) {
        return;
    }
#ifdef WL11AX
    if (BSSCFG_IS_BLOCK_HE_MAC(cfg)) {
        if (wlc_isblocked_hemac(cfg, &hdr->sa)) {
            WL_INFORM((" wl%d.%d: probe response blocked \n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(cfg)));
            return;
        }
    }
#endif /* WL11AX */
#ifdef WL11AX
    if (BSSCFG_BLOCK_HE_ENABLED(cfg)) {
        if (bcm_find_tlv_ext(body, body_len, 255,
                EXT_MNG_HE_CAP_ID) != NULL) {
            if (BSSCFG_IS_BLOCK_HE_MAC(cfg)) {
                wlc_addto_heblocklist(cfg, &hdr->sa);
            }
            WL_INFORM(("wl%d.%d: probe response blocked \n",
                    wlc->pub->unit,
                    WLC_BSSCFG_IDX(cfg)));
            return;
        }
    }
#endif /* WL11AX */

#if defined(WL_OCE) && defined(WL_OCE_AP)
    if (BSS_OCE_ENAB(wlc, cfg) &&
        isset(&(wlc->nbr_discovery_cap), WLC_NBR_DISC_CAP_PRB_RSP_CANCEL) &&
        !wlc_bsscfg_is_probe_response_required(cfg, body, body_len)) {

        WL_INFORM(("%s: wl%d: skip probe response to ["MACF"] as Max channel time"
            " is greater than next Beacon TBTT, and Beacon replaces"
            " Probe response\n", __FUNCTION__, wlc->pub->unit,
            ETHERP_TO_MACF(&hdr->sa)));
        return;
    }
    if (BSS_OCE_ENAB(wlc, cfg) && wlc_oce_find_cap_ind_attr(body, body_len)) {
        wifi_oce_probe_suppress_bssid_attr_t *oce_bssid_attr = NULL;
        wifi_oce_probe_suppress_ssid_attr_t *oce_ssid_attr = NULL;
        uint8 *p;

        oce_prb_rqst = TRUE;
        if ((oce_bssid_attr = wlc_oce_get_prb_suppr_bssid_attr(body, body_len))) {
            if ((oce_bssid_attr->len == 0)) {
                sendProbeResp = FALSE;
                goto fail_oce_check;
            }
            /* iterate thru bssid list for a match */
            p = (uint8 *)&oce_bssid_attr->bssid_list;
            while (oce_bssid_attr->len >= ETHER_ADDR_LEN) {
                if ((eacmp(p, &cfg->BSSID)) == 0) {
                    sendProbeResp = FALSE;
                    goto fail_oce_check;
                }
                oce_bssid_attr->len -= ETHER_ADDR_LEN;
                p += ETHER_ADDR_LEN;
            }
        }

        if ((oce_ssid_attr = wlc_oce_get_prb_suppr_ssid_attr(body, body_len))) {
            /* iterate thru ssid list for a match */
            uint32 bsscfg_short_ssid =
                ~hndcrc32(cfg->SSID, cfg->SSID_len, CRC32_INIT_VALUE);

            p = (uint8 *)&oce_ssid_attr->ssid_list;

            while (oce_ssid_attr->len >= SHORT_SSID_LEN) {
                if (*(uint32 *)p == bsscfg_short_ssid) {
                    sendProbeResp = FALSE;
                    goto fail_oce_check;
                }
                oce_ssid_attr->len -= SHORT_SSID_LEN;
                p += SHORT_SSID_LEN;
            }
        }

fail_oce_check:
        if (!sendProbeResp) {
            WL_OCE_INFO(("wl%d: %s:%d OCE PRQ Suppr BSSID/SSID attr set, skip PRS\n",
                wlc->pub->unit, __FUNCTION__, __LINE__));
            return;
        }

        /* Respond to PRQ at the received rate */
        rate_override = wlc_recv_compute_rspec(wlc->pub->corerev, &wrxh->rxhdr, plcp);
    }
#endif /* WL_OCE && WL_OCE_AP */
    if (!(cfg->closednet) &&
        ((CHSPEC_BANDTYPE(cfg->current_bss->chanspec) == WLC_BAND_6G) ||
#if defined(WL_OCE) && defined(WL_OCE_AP)
        oce_prb_rqst ||
#endif /* WL_OCE && WL_OCE_AP */
        FALSE)) {
        /* If bcast prq then respond with bcast prs. */
        if (((eacmp(&hdr->da, &ether_bcast)) == 0)) {
            bcopy((const char*)&ether_bcast, (char*)&hdr->sa, ETHER_ADDR_LEN);
#if defined(WL_OCE) && defined(WL_OCE_AP)
            if (oce_prb_rqst) {
                /*
                 * For a broadcast PRQ respond at 6Mbps for better
                 * sensitivity. Spec recommends min of 5.5Mbps.
                 */
                rate_override = OFDM_RSPEC(WLC_RATE_6M);
            }
#endif /* WL_OCE && WL_OCE_AP */
        }
    }

    memset(&prb_rqst_param, 0, sizeof(prb_rqst_param));
    memcpy(prb_rqst_param.ssid.SSID, ssid->data, ssid->len);
    prb_rqst_param.ssid.SSID_len = ssid->len;

    if (s_ssid && s_ssid->len) {
        prb_rqst_param.short_ssid_len = s_ssid->len;
        prb_rqst_param.short_ssid = (uint32*)MALLOCZ(wlc->osh,
            (prb_rqst_param.short_ssid_len));

        if (prb_rqst_param.short_ssid) {
            memcpy(prb_rqst_param.short_ssid, &s_ssid->short_ssid[0],
                prb_rqst_param.short_ssid_len);
        } else {
            WL_ERROR(("wl%d:%d MALLOC ERROR for short ssid bytes[%d]\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(cfg),
                prb_rqst_param.short_ssid_len));
            return;
        }

    }
    bsscfg = cfg;
#ifdef WL_MBSSID
    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
        if (!BSSCFG_IS_MAIN_AP(cfg)) {
            bsscfg = wlc->main_ap_bsscfg;
        }
    }
#endif /* WL_MBSSID */
    wlc_probresp_send_probe_resp(mprobresp, bsscfg, &hdr->sa, rate_override, &prb_rqst_param);
    if (prb_rqst_param.short_ssid) {
        MFREE(wlc->osh, prb_rqst_param.short_ssid, prb_rqst_param.short_ssid_len);
    }
} /* wlc_probresp_filter_and_reply */

#ifdef WLPROBRESP_INTRANSIT_FILTER
/* Called on free of a probe response packet */
static void
wlc_proberesp_cb(wlc_info_t *wlc, void *p, uint txs)
{
    struct dot11_management_header *hdr;
    wlc_bsscfg_t *bsscfg;
    int err = 0;

    /* get the bsscfg from the pkttag */
    bsscfg = wlc_bsscfg_find(wlc, WLPKTTAGBSSCFGGET(p), &err);

    hdr = (struct dot11_management_header*)wlc_pkt_get_d11_hdr(wlc, p);

    /* in case bsscfg is freed before this callback is invoked */
    if (bsscfg == NULL) {
        return;
    }

    wlc_probresp_intransit_filter_rem_da(wlc->mprobresp_mac_filter, bsscfg, &hdr->da);
}
#endif /* WLPROBRESP_INTRANSIT_FILTER */

/* register filter function */
int
BCMATTACHFN(wlc_probresp_register)(wlc_probresp_info_t *mprobresp, void *hdl,
    probreq_filter_fn_t filter_fn, bool p2p)
{
    int i;
    if (!mprobresp || !hdl || !filter_fn)
        return BCME_BADARG;

    /* find an empty entry and just add, no duplication check! */
    for (i = 0; i < WLC_PROBRESP_MAXFILTERS; i ++) {
        if (!mprobresp->probreq_filters[i].hdl) {
            mprobresp->probreq_filters[i].hdl = hdl;
            mprobresp->probreq_filters[i].filter_fn = filter_fn;
            if (p2p)
                mprobresp->p2p_index = i;
            return BCME_OK;
        }
    }

    /* it is time to increase the capacity */
    ASSERT(i < WLC_PROBRESP_MAXFILTERS);
    return BCME_NORESOURCE;
}
/* register filter function */
int
BCMATTACHFN(wlc_probresp_unregister)(wlc_probresp_info_t *mprobresp, void *hdl)
{
    int i;
    if (!mprobresp || !hdl)
        return BCME_BADARG;

    for (i = 0; i < WLC_PROBRESP_MAXFILTERS; i ++) {
        if (mprobresp->probreq_filters[i].hdl == hdl) {
            bzero(&mprobresp->probreq_filters[i],
                sizeof(mprobresp->probreq_filters[i]));
            return BCME_OK;
        }
    }

    /* table entry not found! */
    return BCME_NOTFOUND;
}

/**
 * Either the ssid element or the short-ssid element contains information that should be compared
 * against the local node.
 */
static bool
wlc_prbreq_matches(wlc_bsscfg_t *cfg, bcm_tlv_t *ssid, he_short_ssid_list_ie_t *s_ssid)
{
#if BAND6G || defined(WL_OCE_AP)
    wlc_ssid_t tmp_ssid = {0};
#endif /* BAND6G || WL_OCE_AP */
    if (ssid != NULL) {
        if (ssid->len == 0 ||
            (ssid->len == cfg->SSID_len && bcmp(ssid->data, cfg->SSID, ssid->len) == 0)) {
            return TRUE;
        }
    }

    if (s_ssid != NULL) {
        int i;
        uint32 local_short_ssid = ~hndcrc32(cfg->SSID, cfg->SSID_len, CRC32_INIT_VALUE);

        for (i = 0; i < s_ssid->len / sizeof(s_ssid->short_ssid[0]); i++) {
            if (s_ssid->short_ssid[i] == local_short_ssid) {
                return TRUE;
            }
        }
    }
#if BAND6G || defined(WL_OCE_AP)
    if (ssid != NULL) {
        tmp_ssid.SSID_len = MIN(sizeof(tmp_ssid.SSID), ssid->len);
        memcpy(&tmp_ssid.SSID, ssid->data, tmp_ssid.SSID_len);
    }
    if (wlc_rrm_ssid_is_in_nbr_list(cfg, &tmp_ssid, (s_ssid ? s_ssid->short_ssid : NULL),
            (s_ssid ? s_ssid->len : 0))) {
        return TRUE;
    }
#endif /* BAND6G || WL_OCE_AP */
    return FALSE;
} /* wlc_prbreq_matches */

/** process received probe request frame */
void
wlc_probresp_recv_process_prbreq(wlc_probresp_info_t *mprobresp, wlc_d11rxhdr_t *wrxh,
    uint8 *plcp, struct dot11_management_header *hdr, uint8 *body, int body_len)
{
    wlc_info_t *wlc;
    wlc_bsscfg_t *bsscfg;
    wlc_bsscfg_t *bsscfg_hwaddr;
    bcm_tlv_t *ssid;
    he_short_ssid_list_ie_t *short_ssid = NULL;
    bool wildcard_ssid = FALSE; /**< TRUE if no SSID was specified in the probe req message */
#ifndef WL_MBSSID
    BCM_REFERENCE(wildcard_ssid);
#endif /* WL_MBSSID */

    if (!mprobresp)
        return;

    wlc = mprobresp->wlc;

    if (!WLPROBRESP_SW_ENAB(wlc) || SCAN_IN_PROGRESS(wlc->scan))
        return;

    if ((ssid = bcm_find_tlv(body, body_len, DOT11_MNG_SSID_ID)) != NULL) {
        wildcard_ssid = (ssid->len == 0);
    }

    if ((short_ssid = (he_short_ssid_list_ie_t*)bcm_find_tlv_ext(body, body_len,
        DOT11_MNG_ID_EXT_ID, EXT_MNG_SHORT_SSID_ID)) != NULL) {

        WL_INFORM(("wl%d: short ssid IE is present with short ssid[%x]\n",
            wlc->pub->unit,    short_ssid->short_ssid[0]));
    }

#if BAND6G || defined(WL_OCE_AP)
    if (wlc->band->bandunit == BAND_6G_INDEX && wildcard_ssid == TRUE &&
        ETHER_ISBCAST(&hdr->da) && ETHER_ISBCAST(&hdr->bssid) &&
        isset(&(wlc->nbr_discovery_cap), WLC_NBR_DISC_CAP_NO_WILDCARD_PRBREQ)) {
        return; /* discourages excessive probing behavior by STA in 6GHz band */
    }
#endif /* BAND6G || WL_OCE_AP */

    if (ssid != NULL || short_ssid != NULL) {
        wlc_bsscfg_t *cfg;
        int idx;
        bsscfg_hwaddr = wlc_bsscfg_find_by_hwaddr(wlc, &hdr->da);
        bsscfg = wlc_bsscfg_find_by_bssid(wlc, &hdr->bssid);
        if (bsscfg || bsscfg_hwaddr) {
            cfg = bsscfg ? bsscfg : bsscfg_hwaddr;
            if (BSSCFG_AP(cfg) && cfg->up && cfg->enable &&
                ((bsscfg == bsscfg_hwaddr) || ETHER_ISBCAST(&hdr->da) ||
                 ETHER_ISBCAST(&hdr->bssid)) &&
                wlc_prbreq_matches(cfg, ssid, short_ssid)) {
#ifdef WL_MBSSID
                if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
                    wlc_probresp_filter_and_reply(mprobresp,
                        cfg, wrxh, plcp, hdr, body, body_len);
                    return;
                }
#endif /* WL_MBSSID */
                wlc_probresp_filter_and_reply(mprobresp, cfg,
                    wrxh, plcp, hdr, body, body_len);
            }
        } else if (ETHER_ISBCAST(&hdr->da) && ETHER_ISBCAST(&hdr->bssid)) {
            FOREACH_UP_AP(wlc, idx, cfg) {
                if (cfg->enable && wlc_prbreq_matches(cfg, ssid, short_ssid)) {
#ifdef WL_MBSSID
                    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) &&
                            wildcard_ssid == FALSE) {
                        /* reply with main BSSID when probe for vif ssid */
                        wlc_probresp_filter_and_reply(mprobresp,
                            cfg, wrxh, plcp, hdr, body, body_len);
                        return;
                    }
                    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) &&
                            wildcard_ssid == TRUE &&
                        !BSSCFG_IS_MAIN_AP(cfg)) {
                        /* Only reply on main BSSID if probe is bcast */
                        continue;
                    }
#endif /* WL_MBSSID */
                    wlc_probresp_filter_and_reply(mprobresp, cfg,
                        wrxh, plcp, hdr, body, body_len);
                }
            }
        }
    }
} /* wlc_probresp_recv_process_prbreq */

#endif /* WLPROBRESP_SW */
