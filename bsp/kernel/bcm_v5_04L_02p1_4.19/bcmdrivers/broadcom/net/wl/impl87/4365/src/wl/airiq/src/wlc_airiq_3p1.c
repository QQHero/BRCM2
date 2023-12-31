/*
 * @file
 * @brief
 *
 *  Air-IQ 3+1 mode
 *
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_airiq_3p1.c 766110 2018-07-24 22:40:18Z $
 *
 */

#include <wlc_airiq.h>

#include <wlc_phy_ac.h>
#include <phy_utils_reg.h>
#include <wlc_phyreg_ac.h>

#ifdef BCMDBG
static const char *airiq_modesw_state_str[] = {
	"AIRIQ_MODESW_IDLE",
	"AIRIQ_MODESW_DOWNGRADE_IN_PROGRESS",
	"AIRIQ_MODESW_DOWNGRADE_FINISHED",
	"AIRIQ_MODESW_UPGRADE_IN_PROGRESS",
	"AIRIQ_MODESW_PHY_UPGRADED"
};
#endif /* BCMDBG */

/* Downgrades PHY to enable +1 (scan) core on Air-IQ scan channels;
 * disables watchdog to avoid periodic calibration
 */

void
wlc_airiq_3p1_downgrade_phy(airiq_info_t *airiqh, chanspec_t scan_chanspec)
{
	wlc_info_t *wlc = airiqh->wlc;

	wlc_suspend_mac_and_wait(wlc);

	wlc_airiq_scan_set_chanspec_3p1(airiqh, (phy_info_t*)WLC_PI(wlc),
		wlc->home_chanspec, scan_chanspec);

	airiqh->upgrade_pending = TRUE;

	wlc_enable_mac(wlc);

	WL_AIRIQ(("wl%d: %s: downgraded phy to 3+1\n", wlc->pub->unit, __FUNCTION__));
	ASSERT(airiqh->phy_mode == PHYMODE_3x3_1x1);
}

/* Upgrade PHY to disable scan core and move to full MIMO (4x4) mode; enables watchdog */
void
wlc_airiq_3p1_upgrade_phy(airiq_info_t *airiqh)
{
	wlc_info_t *wlc = airiqh->wlc;

	if (D11REV_IS(airiqh->wlc->pub->corerev, 65))
		MOD_PHYREG((phy_info_t*)WLC_PI(wlc), RxFeCtrl1, swap_iq3, 1);
	else
		MOD_PHYREG((phy_info_t*)WLC_PI(wlc), RxFeCtrl1, swap_iq3, 0);

	wlc_suspend_mac_and_wait(wlc);

	wlc_phy_set_val_phymode((phy_info_t*)WLC_PI(wlc), 0);

	wlc_enable_mac(wlc);

	airiqh->upgrade_pending = FALSE;

	WL_AIRIQ(("wl%d: %s: upgraded phy to 4x4\n", wlc->pub->unit, __FUNCTION__));
	ASSERT(airiqh->phy_mode == 0);
}

#ifdef WL_MODESW
void
wlc_airiq_modeswitch_state_upd(airiq_info_t *airiqh, uint new_airiq_modesw_state)
{
#ifdef BCMDBG
	if (new_airiq_modesw_state < AIRIQ_MODESW_STATE_CNT) {
		WL_AIRIQ(("wl%d %s: [%s] -> [%s]\n", airiqh->wlc->pub->unit,
			__FUNCTION__, airiq_modesw_state_str[airiqh->modeswitch_state],
			airiq_modesw_state_str[new_airiq_modesw_state]));
	} else {
		WL_ERROR(("wl%d %s:  invalid new modesw state: %d (curr state: %s\n",
			airiqh->wlc->pub->unit, __FUNCTION__, new_airiq_modesw_state,
			airiq_modesw_state_str[airiqh->modeswitch_state]));
	}
#endif
	airiqh->modeswitch_state = new_airiq_modesw_state;
}

static void
wlc_airiq_handle_modeswitch(airiq_info_t *airiqh, uint new_state)
{
	switch (airiqh->modeswitch_state) {
	case AIRIQ_MODESW_IDLE:
		WL_AIRIQ(("wl%d %s: new state %d in idle state\n", airiqh->wlc->pub->unit,
			__FUNCTION__, new_state));
		break;

	case AIRIQ_MODESW_DOWNGRADE_IN_PROGRESS:
		if (new_state == MODESW_DN_AP_COMPLETE) {
			wlc_airiq_modeswitch_state_upd(airiqh, AIRIQ_MODESW_DOWNGRADE_FINISHED);
			WL_AIRIQ(("wl%d %s: downgrade completed %d\n", airiqh->wlc->pub->unit,
				__FUNCTION__, __LINE__));

			/* downgrade complete. continue the scan */
			wlc_airiq_start_scan_phase2(airiqh);
		}
		break;

	case AIRIQ_MODESW_PHY_UPGRADED:
		if (new_state == MODESW_UP_AP_COMPLETE ||
		    new_state == MODESW_DN_AP_COMPLETE) {
			wlc_airiq_modeswitch_state_upd(airiqh, AIRIQ_MODESW_IDLE);
			WL_AIRIQ(("wl%d %s: upgrade completed %d\n", airiqh->wlc->pub->unit,
				__FUNCTION__, __LINE__));

			/* check for pending Air-IQ phy cal */
			if (airiqh->scan.run_phycal)
				wlc_airiq_start_scan(airiqh, 1, 0);
		}
		break;

	default:
		if (new_state == MODESW_DN_AP_COMPLETE)
			WL_AIRIQ(("wl%d %s: [%d] downgrade completed\n",
				airiqh->wlc->pub->unit,
				__FUNCTION__, airiqh->modeswitch_state));
		else if (new_state == MODESW_UP_AP_COMPLETE)
			WL_AIRIQ(("wl%d %s: [%d] upgrade completed\n",
				airiqh->wlc->pub->unit,
				__FUNCTION__, airiqh->modeswitch_state));
		else
			WL_AIRIQ(("wl%d %s: [%d] unhandled new state (%d)\n",
				airiqh->wlc->pub->unit,
				__FUNCTION__, airiqh->modeswitch_state, new_state));
		break;
	}
}
/* callback registered to mode switch module; called on upgrade/downgrade */
void
wlc_airiq_opmode_change_cb(void *ctx, wlc_modesw_notif_cb_data_t *notif_data)
{
	wlc_info_t *wlc = (wlc_info_t*)ctx;
	airiq_info_t *airiqh;

#if defined(BCMDBG) || defined(WLMSG_MODESW)
	wlc_bsscfg_t *bsscfg = NULL;
#endif /* BCMDBG || WLMSG_MODESW */
	ASSERT(wlc);
	ASSERT(notif_data);
	if (wlc->airiq == NULL)
		return;
	airiqh = wlc->airiq;

	if (airiqh->phy_mode != PHYMODE_3x3_1x1 &&
	    airiqh->phy_mode != 0) {
		WL_AIRIQ(("%s airiq mode not enabled, exiting.\n", __FUNCTION__));
		WL_MODE_SWITCH(("%s MODESW CB status = %d oper_mode = %x signal = %d\n",
			__FUNCTION__, notif_data->status, notif_data->opmode,
			notif_data->signal));
		return;
	}

	WL_MODE_SWITCH(("%s MODESW Callback status = %d oper_mode = %x signal = %d\n",
			__FUNCTION__,
			notif_data->status, notif_data->opmode, notif_data->signal));
#if defined(BCMDBG) || defined(WLMSG_MODESW)
	bsscfg = notif_data->cfg;
#endif /* BCMDBG || WLMSG_MODESW */
	switch (notif_data->signal) {
	case MODESW_PHY_DN_COMPLETE:
	case MODESW_PHY_UP_COMPLETE:
	{
		int airiq_modesw_state = airiqh->modeswitch_state;
#ifdef BCMDBG
		WL_AIRIQ(("wl%d: %s: signal=%d airiq modesw_state = %s\n",
			WLCWLUNIT(wlc), __FUNCTION__, notif_data->signal,
			airiq_modesw_state_str[airiqh->modeswitch_state]));
#endif
		if (airiq_modesw_state == AIRIQ_MODESW_DOWNGRADE_IN_PROGRESS)
			/* downgrade and set to first chanspec */
			wlc_airiq_3p1_downgrade_phy(airiqh, airiqh->scan.chanspec_list[0]);
		else if (airiq_modesw_state == AIRIQ_MODESW_UPGRADE_IN_PROGRESS) {
			wlc_airiq_3p1_upgrade_phy(airiqh);
			wlc_airiq_modeswitch_state_upd(airiqh, AIRIQ_MODESW_PHY_UPGRADED);
		}
		WL_MODE_SWITCH(("wl%d: %s: Changed phy mode to (%d) by cfg = %d\n",
			WLCWLUNIT(wlc), __FUNCTION__, airiqh->phy_mode, bsscfg->_idx));
	}
	break;
	case MODESW_PHY_UP_START:
		WL_MODE_SWITCH(("wl%d: %s: Changed chip phy mode to (%d) by cfg = %d\n",
			WLCWLUNIT(wlc), __FUNCTION__, PHYMODE_MIMO, bsscfg->_idx));
		break;
	}

	wlc_airiq_handle_modeswitch(airiqh, notif_data->signal);

	return;
}

#ifdef VASIP_HW_SUPPORT
/* Initiates upgrade of wlc to full MIMO (4x4) using modesw module. */
int
wlc_airiq_3p1_upgrade_wlc(wlc_info_t *wlc)
{
	int idx = 0;
	wlc_bsscfg_t *bsscfg;
	uint8 new_oper_mode = 0, curr_oper_mode, bw = 0, nss;
	bool mode_switch_sched = FALSE;
	int err = BCME_UNSUPPORTED;

	if (wlc->stf->core3_p1c) {
		wlc_airiq_3p1_upgrade_phy(wlc->airiq);
#if defined(WL_MODESW)
		wlc_airiq_modeswitch_state_upd(wlc->airiq, AIRIQ_MODESW_IDLE);
#endif
		return BCME_OK;
	}
	/* announce upgrade */
	FOREACH_UP_AP(wlc, idx, bsscfg) {
		if (WLC_BSS_CONNECTED(bsscfg)) {
			curr_oper_mode = bsscfg->oper_mode;
			bw = DOT11_OPER_MODE_CHANNEL_WIDTH(curr_oper_mode);
			nss = DOT11_OPER_MODE_RXNSS(curr_oper_mode);
			/* Try next link if this one's already at max possible chains */
			if (nss >= WLC_BITSCNT(wlc->stf->valid_rxchain_mask))
				continue;
			/* Else fall through to announce we're returning the scan chain
			 * back to the data path
			 */
		} else {
			/* DFS code arbitrarily sets to 80MHz. We will at least try to
			 * base this on the current chanspec
			 */
			if (CHSPEC_IS8080(wlc->chanspec))
				bw = DOT11_OPER_MODE_8080MHZ;
			else if (CHSPEC_IS80(wlc->chanspec))
				bw = DOT11_OPER_MODE_80MHZ;
			else if (CHSPEC_IS40(wlc->chanspec))
				bw = DOT11_OPER_MODE_40MHZ;
			else if (CHSPEC_IS20(wlc->chanspec))
				bw = DOT11_OPER_MODE_20MHZ;
			else
				ASSERT(FALSE);
		}
		/* Assume new mode uses all the chains in the device */
		new_oper_mode =
			DOT11_OPER_MODE(0, WLC_BITSCNT(wlc->stf->valid_rxchain_mask), bw);
		err = wlc_modesw_handle_oper_mode_notif_request(wlc->modesw, bsscfg,
			new_oper_mode, TRUE, MODESW_CTRL_OPMODE_IE_REQD_OVERRIDE);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: failed to request modesw %d\n", wlc->pub->unit, err));
			break;
		}
		mode_switch_sched = TRUE;
	}

	WL_AIRIQ(("wl%d: %s: mode switch up scheduled = %d opmode: 0x%02x, bw: 0x%02x\n",
		wlc->pub->unit, __FUNCTION__, mode_switch_sched, new_oper_mode, bw));

	if (mode_switch_sched)
		return BCME_BUSY;
	else
		return BCME_OK;
}

/* Initiates downgrade of wlc to reduced MIMO plus scan core (3+1) using modesw module. */
int
wlc_airiq_3p1_downgrade_wlc(wlc_info_t *wlc)
{
	int idx = 0;
	wlc_bsscfg_t *bsscfg;
	uint8 new_oper_mode = 0, curr_oper_mode, bw = 0, nss;
	bool mode_switch_sched = FALSE;
	int err = BCME_UNSUPPORTED;

	if (wlc->stf->core3_p1c) {
		return BCME_OK;
	}

	FOREACH_UP_AP(wlc, idx, bsscfg) {
		if (WLC_BSS_CONNECTED(bsscfg)) {
			curr_oper_mode = wlc_modesw_derive_opermode(wlc->modesw,
					bsscfg->current_bss->chanspec, bsscfg,
					wlc->stf->op_rxstreams);
			bw = DOT11_OPER_MODE_CHANNEL_WIDTH(curr_oper_mode);
			nss = DOT11_OPER_MODE_RXNSS(curr_oper_mode);
			/* Try next link if this one already has free chains */
			if (nss < WLC_BITSCNT(wlc->stf->valid_rxchain_mask))
				continue;
			/* Else fall through to announce we're taking a chain from
			 * the data path (for custom scanning)
			 */
		} else {
			/* XXX DFS code arbitrarily sets to 80MHz. We will at least try to
			 * base this on something
			 */
			if (CHSPEC_IS8080(wlc->chanspec))
				bw = DOT11_OPER_MODE_8080MHZ;
			else if (CHSPEC_IS80(wlc->chanspec))
				bw = DOT11_OPER_MODE_80MHZ;
			else if (CHSPEC_IS40(wlc->chanspec))
				bw = DOT11_OPER_MODE_40MHZ;
			else if (CHSPEC_IS20(wlc->chanspec))
				bw = DOT11_OPER_MODE_20MHZ;
			else
				ASSERT(FALSE);
		}
		/* New mode is one chain less than supported */
		new_oper_mode =
			DOT11_OPER_MODE(0, WLC_BITSCNT(wlc->stf->valid_rxchain_mask) - 1, bw);
		err = wlc_modesw_handle_oper_mode_notif_request(wlc->modesw, bsscfg,
				new_oper_mode, TRUE, MODESW_CTRL_OPMODE_IE_REQD_OVERRIDE);
		if (err != BCME_OK) {
			WL_ERROR(("wl%d: failed to request modesw %d\n", wlc->pub->unit, err));
			break;
		}
		mode_switch_sched = TRUE;
	}

	WL_AIRIQ(("wl%d:%s:  mode switch down scheduled = %d opmode: 0x%02x, bw: 0x%02x\n",
		wlc->pub->unit, __FUNCTION__, mode_switch_sched, new_oper_mode, bw));

	if (mode_switch_sched)
		return BCME_BUSY;
	else
		return BCME_OK;
}
#endif /* WL_MODESW */

#endif /* VASIP_HW_SUPPORT */

bool wlc_airiq_phymode_3p1(wlc_info_t *wlc)
{
	airiq_info_t *airiqh;

	airiqh = wlc->airiq;

	if (!airiqh)
		return FALSE;

	return airiqh->phy_mode == PHYMODE_3x3_1x1;
}
