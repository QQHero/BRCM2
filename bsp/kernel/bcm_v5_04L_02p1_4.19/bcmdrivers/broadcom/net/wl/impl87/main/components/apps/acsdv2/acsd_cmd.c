/*
 * ACS deamon command module (Linux)
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
 * $Id: acsd_cmd.c 809799 2022-03-23 21:05:18Z $
 */

#include "acsd_svr.h"

extern void acs_cleanup_scan_entry(acs_chaninfo_t *c_info);

#define CHANIM_GET(field, unit)						\
	{ \
		if (!strcmp(param, #field)) { \
			chanim_info_t* ch_info = c_info->chanim_info; \
			if (ch_info) \
				*r_size = sprintf(buf, "%d %s", \
					chanim_config(ch_info).field, unit); \
			else \
				*r_size = sprintf(buf, "ERR: Requested info not available"); \
			goto done; \
		} \
	}

#define CHANIM_SET(field, unit, type)				\
	{ \
		if (!strcmp(param, #field)) { \
			chanim_info_t* ch_info = c_info->chanim_info; \
			if (!ch_info) { \
				*r_size = sprintf(buf, "ERR: set action not successful"); \
				goto done; \
			} \
			if ((setval > (type)chanim_range(ch_info).field.max_val) || \
				(setval < (type)chanim_range(ch_info).field.min_val)) { \
				*r_size = sprintf(buf, "Value out of range (min: %d, max: %d)\n", \
					chanim_range(ch_info).field.min_val, \
					chanim_range(ch_info).field.max_val); \
				goto done; \
			} \
			chanim_config(ch_info).field = setval; \
			*r_size = sprintf(buf, "%d %s", chanim_config(ch_info).field, unit); \
			goto done; \
		} \
	}

#define ACS_CMD_TEST_DFSR		"test_dfsr"
#define ACS_CMD_TEST_PRECLEAR		"test_preclear"
#define ACS_CMD_ZDFS_5G_MOVE		"zdfs_5g_move"
#define ACS_CMD_ZDFS_5G_PRECLEAR	"zdfs_5g_preclear"
#define ACS_CMD_ZDFS_2G_MOVE		"zdfs_2g_move"
#define ACS_CMD_ZDFS_2G_PRECLEAR	"zdfs_2g_preclear"
#define ACS_CMD_TEST_CEVENT		"test_cevent"

static int
acsd_extract_token_val(char* data, const char *token, char *output, int len)
{
	char *p, *c;
	char copydata[1024];
	char *val;

	if (data == NULL)
		goto err;

	strncpy(copydata, data, sizeof(copydata));
	copydata[sizeof(copydata) - 1] = '\0';

	ACSD_DEBUG("copydata: %s\n", copydata);

	p = strstr(copydata, token);
	if (!p)
		goto err;

	if ((c = strchr(p, '&')))
		*c++ = '\0';

	val = strchr(p, '=');
	if (!val)
		goto err;

	val += 1;
	ACSD_DEBUG("token_val: %s\n", val);

	strncpy(output, val, len);
	output[len - 1] = '\0';

	return strlen(output);

err:
	return -1;
}

static int
acsd_pass_candi(ch_candidate_t * candi, int count, char** buf, uint* r_size)
{
	int totsize = 0;
	int i, j;
	ch_candidate_t *cptr;

	for (i = 0; i < count; i++) {

		ACSD_DEBUG("candi->chspec: 0x%x\n", candi->chspec);

		cptr = (ch_candidate_t *)*buf;

		/* Copy over all fields, converting to network byte order if needed */
		cptr->chspec = htons(candi->chspec);
		cptr->valid = candi->valid;
		cptr->is_dfs = candi->is_dfs;
		cptr->reason = htons(candi->reason);

		if (candi->valid || (candi->reason & ACS_INVALID_NOISE)) {
			for (j = 0; j < CH_SCORE_MAX; j++) {
				cptr->chscore[j].score = htonl(candi->chscore[j].score);
				cptr->chscore[j].weight = htonl(candi->chscore[j].weight);
				ACSD_DEBUG(" htonl score is %d\n", cptr->chscore[j].score);
			}
		}

		*r_size += sizeof(ch_candidate_t);
		*buf += sizeof(ch_candidate_t);
		totsize += sizeof(ch_candidate_t);
		candi ++;
	}

	return totsize;
}

static const char *
acs_policy_name(acs_policy_index i)
{
	switch (i) {
		case ACS_POLICY_DEFAULT2G:	return "Default2g";
		case ACS_POLICY_DEFAULT5G:	return "Default5g";
		case ACS_POLICY_DEFAULT6G:	return "Default6g";
		case ACS_POLICY_USER:		return "User";
		/* No default so compiler will complain if new definitions are missing */
	}
	return "(unknown)";
}

/* processes DFS move/preclear related commands including
 *	test_dfsr, test_preclear,
 *	zdfs_5g_move, zdfs_5g_preclear,
 *	zdfs_2g_move, zdfs_2g_preclear,
 */
static int
acsd_dfs_cmd(acs_chaninfo_t *c_info, char *buf, char *param, char *val, uint *r_size,
		bool move, acs_cac_mode_t cac_mode)
{
	int ret = BCME_OK;
	acs_chaninfo_t *zdfs_2g_ci = NULL;
	chanspec_t chspec = wf_chspec_aton(val);

	/* request must come from a 5GHz interface only */
	if (!BAND_5G(c_info->rs_info.band_type)) {
		*r_size = sprintf(buf, "ERR: not a 5Hz interface");
		return BCME_BADBAND;
	}
	if (!move && !c_info->country_is_edcrs_eu) {
		*r_size = sprintf(buf, "ERR: Country code does not support preclearance");
		return BCME_UNSUPPORTED;
	}
	if (!move || cac_mode == ACS_CAC_MODE_ZDFS_5G_ONLY ||
			cac_mode == ACS_CAC_MODE_ZDFS_2G_ONLY) {
		/* ensure bgdfs is enabled */
		if (!c_info->acs_bgdfs) {
			*r_size = sprintf(buf, "ERR: ZDFS not enabled");
			return BCME_NOTENABLED;
		}
	}
	if (cac_mode == ACS_CAC_MODE_ZDFS_5G_ONLY &&
			c_info->acs_bgdfs->state != BGDFS_STATE_IDLE) {
		/* avoid ZDFS if the 5Hz interface is busy with ZDFS */
		if (c_info->acs_bgdfs->state != BGDFS_STATE_IDLE) {
			*r_size = sprintf(buf, "ERR: ZDFS_5G is not idle on this interface");
			return BCME_BUSY;
		}
	}
	if (ACS_CAC_MODE_ZDFS_2G_ONLY == cac_mode) {
		if ((zdfs_2g_ci = acs_get_zdfs_2g_ci()) == NULL || !zdfs_2g_ci->acs_bgdfs) {
			*r_size = sprintf(buf, "ERR: ZDFS_2G is not available");
			return BCME_NOTENABLED;
		}
		if (zdfs_2g_ci->acs_bgdfs->state != BGDFS_STATE_IDLE) {
			*r_size = sprintf(buf, "ERR: ZDFS_2G is not idle");
			return BCME_BUSY;
		}
	}

	c_info->cac_mode = cac_mode;
	if (move) {
		c_info->selected_chspec = chspec;
		acs_set_chspec(c_info, FALSE, WL_CHAN_REASON_DFS_AP_MOVE_START);
		c_info->switch_reason_type = ACS_DFSREENTRY;
	} else { /* preclear */
		c_info->acs_bgdfs->next_scan_chan = chspec;
		ret = acs_bgdfs_ahead_trigger_scan(c_info);
	}

	*r_size = sprintf(buf, "%s chanspec: %7s (status:%d)", param, val, ret);

	return ret;
}

/* buf should be null terminated. rcount doesn;t include the terminuating null */
int
acsd_proc_cmd(acsd_wksp_t* d_info, char* buf, uint rcount, uint* r_size)
{
	char *c, *data;
	int err = 0, ret;
	char ifname[IFNAMSIZ];

	/* Check if we have command and data in the expected order */
	if (!(c = strchr(buf, '&'))) {
		ACSD_ERROR("Missing Command\n");
		err = -1;
		goto done;
	}
	*c++ = '\0';
	data = c;

	if (!strcmp(buf, "info")) {
		time_t ltime;
		int i;
		const char *mode_str[] = {"disabled", "monitor", "auto", "coex", "11h",
		       "fixchspec"};
		d_info->stats.valid_cmds++;

		time(&ltime);
		*r_size = sprintf(buf, "time: %s \n", ctime(&ltime));
		*r_size += sprintf(buf+ *r_size, "acsd version: %d\n", d_info->version);
		*r_size += sprintf(buf+ *r_size, "acsd ticks: %d\n", d_info->ticks);
		*r_size += sprintf(buf+ *r_size, "acsd poll_interval: %d seconds\n",
			d_info->poll_interval);

		for (i = 0; i < ACS_MAX_IF_NUM; i++) {
			acs_chaninfo_t *c_info = NULL;

			c_info = d_info->acs_info->chan_info[i];

			if (!c_info)
				continue;

			*r_size += sprintf(buf+ *r_size, "ifname: %s, mode: %s\n",
				c_info->name, mode_str[c_info->mode]);
		}

		goto done;
	}

	if (!strcmp(buf, "csscan") || (!strcmp(buf, "autochannel"))) {
		acs_chaninfo_t *c_info = NULL;
		int index;
		bool pick = FALSE;
		bool allow_uncleared_dfs_ch = FALSE;

		d_info->stats.valid_cmds++;
		pick = !strcmp(buf, "autochannel");

		if ((ret = acsd_extract_token_val(data, "ifname", ifname, sizeof(ifname))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing ifname");
			goto done;
		}

		ACSD_DEBUG("cmd: %s, data: %s, ifname: %s\n",
			buf, data, ifname);

		index = acs_idx_from_map(ifname);
		if (index != -1) {
			c_info = d_info->acs_info->chan_info[index];
		}

		if (!c_info) {
			*r_size = sprintf(buf, "Request not permitted: "
				"Interface was not intialized properly");
			goto done;
		}

		if (!AUTOCHANNEL(c_info)) {
			*r_size = sprintf(buf, "Request not permitted: "
				"Interface is not in autochannel mode");
			goto done;
		}

		if (c_info->acs_bgdfs != NULL && c_info->acs_bgdfs->state != BGDFS_STATE_IDLE) {
			*r_size = sprintf(buf, "Request not permitted: "
				"BGDFS in progress");
			goto done;
		}

		/* If current channel is DFS, temporarily set to non_DFS channel
		 * (when in FCC country codes) in order for CS scan to be successful.
		 */
		if (!c_info->country_is_edcrs_eu) {
			c_info->acsd_scs_dfs_scan = 1;
			err = acs_change_from_dfs_to_nondfs(c_info);
			c_info->acsd_scs_dfs_scan = 0;
		}
		if (err) {
			ACSD_ERROR("ifname: %s acs_change_from_dfs_to_non_dfs failed due to: %d\n",
				c_info->name, err);
			return err;
		}

		if (!c_info->chanim_info) {
			err = acsd_chanim_init(c_info);
			ACS_ERR(ret, "chanim init failed\n");
		}

		err = acs_run_cs_scan(c_info);
		if (err) {
			ACSD_ERROR("ifname: %s scan is failed due to: %d\n", c_info->name, err);
			return err;
		}

		acs_cleanup_scan_entry(c_info);
#ifdef ACSD_SEGMENT_CHANIM
		acs_segment_allocate(c_info);
#endif /* ACSD_SEGMENT_CHANIM */
		err = acs_request_data(c_info);

		if (pick) {
			/* Modified per customer requirement to avoid acsd2 kill and restart
			 * "acs_cli autochannel" command is modified to handle driver
			 * config/nvram changes in channel mode/bw/regulatory, etc.
			 */
			char prefix[32], tmp[100];

			/* Get updated wlX_reg_mode */
			acs_snprintf(prefix, sizeof(prefix), "%s_", ifname);

			acs_runtime_retrieve_config(c_info, prefix);

			/* Get updated status such as chanspec, bw, etc. */
			acs_get_rs_info(c_info, ifname);
			if (BAND_5G(c_info->rs_info.band_type) &&
				(nvram_match(strcat_r(prefix, "reg_mode", tmp), "h") ||
				nvram_match(strcat_r(prefix, "reg_mode", tmp), "strict_h"))) {
				c_info->rs_info.reg_11h = TRUE;
			} else {
				c_info->rs_info.reg_11h = FALSE;
			}
			c_info->autochannel_through_cli = TRUE;
			if (!c_info->txpwr_params) {
				acs_get_txpwr_for_allchans(c_info);
			}
			if (!c_info->reg_txpwr_params) {
				acs_get_reg_txpwr_for_allchans(c_info);
			}
			acs_select_chspec(c_info);
			if (c_info->selected_chspec == c_info->cur_chspec) {
				c_info->autochannel_through_cli = FALSE;
				goto done;
			}

			allow_uncleared_dfs_ch =
				c_info->acs_boot_only && c_info->acs_allow_uncleared_dfs_ch;
			if (!acs_is_initial_selection(c_info) && !c_info->acs_initial_autochannel &&
				ACS_CHINFO_IS_UNCLEAR(acs_get_chanspec_info(c_info,
					c_info->selected_chspec)) &&
					!allow_uncleared_dfs_ch) {
				ACSD_INFO("%s: If selected dfs channel(0x%04X) is not cleared then"
						" don't allow acsd to change the channel\n",
						c_info->name, c_info->selected_chspec);
				goto done;
			}

			if (c_info->acs_use_csa) {
				int csa_count = ACS_CSA_COUNT * 100 + 100;
				err = acs_csa_handle_request(c_info);
				if (!err) {
					sleep_ms(csa_count);
				}
			} else {
				err = acs_set_chanspec(c_info, c_info->selected_chspec);
				if (!err) {
					err = acs_update_driver(c_info);
					if (err) {
						ACSD_ERROR("%s: update driver failed\n",
								c_info->name);
					}
				}
			}
			c_info->autochannel_through_cli = FALSE;
			if (!err) {
				chanim_upd_acs_record(c_info->name, c_info->chanim_info,
						c_info->selected_chspec, ACS_IOCTL,
						d_info->ticks);
				if (c_info->acs_boot_only) {
					/* Update status here as acsd_watchdog() does not update
					 * if ACS_MODE_DISABLE
					 */
					acs_update_status(c_info);
				}
			}
		}

		*r_size = sprintf(buf, "Request finished");
		goto done;
	}

	if (!strcmp(buf, "acs_restart")) {
		acs_chaninfo_t *c_info = NULL;
		int index, cur_chspec = 0, i;
		char prefix[32], tmp[100];
		bool cur_is_blacklisted = FALSE;

		d_info->stats.valid_cmds++;

		if ((ret = acsd_extract_token_val(data, "ifname", ifname, sizeof(ifname))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing ifname");
			goto done;
		}

		ACSD_DEBUG("cmd: %s, data: %s, ifname: %s\n",
			buf, data, ifname);

		index = acs_idx_from_map(ifname);
		if (index != -1) {
			c_info = d_info->acs_info->chan_info[index];
		}

		if (!c_info) {
			*r_size = sprintf(buf, "Request not permitted: "
				"Interface was not intialized properly");
			goto done;
		}

		if (!AUTOCHANNEL(c_info)) {
			*r_size = sprintf(buf, "Request not permitted: "
				"Interface is not in autochannel mode");
			goto done;
		}
		c_info->acs_restart = TRUE;

		/* Get updated wlX_reg_mode */
		acs_snprintf(prefix, sizeof(prefix), "%s_", ifname);

		acs_runtime_retrieve_config(c_info, prefix);

		/* By default scan and channel change is allowed without any conditions unless"
		 * acs_skip_scan_on_restart nvram is set. If nvram is set and current channel
		 * is included in the excluded/blacklisted list then channel change is allowed
		 * else channel change will not be allowed.
		 */
		if (c_info->acs_skip_scan_on_restart) {
			if ((acs_get_chanspec(c_info, &cur_chspec)) != ACSD_OK) {
				goto done;
			}

			for (i = 0; i < c_info->excl_chans.count; i++) {
				if (c_info->excl_chans.clist[i] == cur_chspec) {
					cur_is_blacklisted = TRUE;
					break;
				}
			}

			if (!cur_is_blacklisted) {
				ACSD_INFO("%s: Don't initiate channel change if current channel is"
						"(0x%04x) not blacklisted/excluded\n",
						c_info->name, cur_chspec);
				goto done;
			}
		}

		if (c_info->acs_is_in_ap_mode) {
			acs_set_initial_chanspec(c_info);
		}

		err = acs_run_cs_scan(c_info);
		if (err) {
			ACSD_ERROR("ifname: %s scan is failed due to: %d\n", c_info->name, err);
			c_info->acs_restart = FALSE;
			return err;
		}

		acs_cleanup_scan_entry(c_info);
		err = acs_request_data(c_info);

		/* Get updated status such as chanspec, bw, etc. */
		acs_get_rs_info(c_info, ifname);

		if (BAND_5G(c_info->rs_info.band_type) &&
				(nvram_match(strcat_r(prefix, "reg_mode", tmp), "h") ||
				nvram_match(strcat_r(prefix, "reg_mode", tmp), "strict_h"))) {
			c_info->rs_info.reg_11h = TRUE;
		} else {
			c_info->rs_info.reg_11h = FALSE;
		}
		acs_select_chspec(c_info);
		c_info->acs_restart = FALSE;
		if ((acs_get_chanspec(c_info, &cur_chspec)) != ACSD_OK) {
			goto done;
		}
		if (cur_chspec == c_info->selected_chspec) {
			ACSD_PRINT("%s: Don't change channel if current channel is (0x%04x) same"
					"as selected chspec (0x%04x)\n", c_info->name,
					cur_chspec, c_info->selected_chspec);
			goto done;
		}
		if (!c_info->acs_use_csa ||
				(ret = acs_csa_handle_request(c_info)) != ACSD_OK) {
			acs_set_chspec(c_info, TRUE, ACSD_USE_DEF_METHOD);

			ret = acs_update_driver(c_info);
			if (ret)
				ACSD_ERROR("%s: update driver failed\n", ifname);
		}
		if (ret == ACSD_OK) {
			int csa_count = ACS_CSA_COUNT * 100 + 100;
			sleep_ms(csa_count);
			chanim_upd_acs_record(c_info->name, c_info->chanim_info,
					c_info->selected_chspec, ACS_IOCTL,
					d_info->ticks);
		}
		*r_size = sprintf(buf, "Request finished");
		goto done;
	}

	if (!strcmp(buf, "dump")) {
		char param[128];
		int index;
		acs_chaninfo_t *c_info = NULL;

		if ((ret = acsd_extract_token_val(data, "param", param, sizeof(param))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing param");
			goto done;
		}

		if ((ret = acsd_extract_token_val(data, "ifname", ifname, sizeof(ifname))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing ifname");
			goto done;
		}

		ACSD_DEBUG("cmd: %s, data: %s, param: %s, ifname: %s\n",
			buf, data, param, ifname);

		index = acs_idx_from_map(ifname);

		ACSD_DEBUG("index is : %d\n", index);
		if (index != -1)
			c_info = d_info->acs_info->chan_info[index];

		ACSD_DEBUG("c_info: %p\n", c_info);

		if (!c_info) {
			*r_size = sprintf(buf, "ERR: Requested info not available");
			goto done;
		}

		d_info->stats.valid_cmds++;
		if (!strcmp(param, "help")) {
			*r_size = sprintf(buf,
				"dump: acs_record acsd_stats bss candidate chanim cscore"
				" dfsr scanresults");
		}
		else if (!strcmp(param, "dfsr")) {
			*r_size = acs_dfsr_dump(ACS_DFSR_CTX(c_info), buf, ACSD_BUFSIZE_DEFAULT);
		}
		else if (!strcmp(param, "chanim")) {
			wl_chanim_stats_t * chanim_stats = c_info->chanim_stats;
			wl_chanim_stats_t tmp_stats = {0};
			int count;
			int i;
			chanim_stats_t *stats;
			chanim_stats_v2_t *statsv2;
			chanim_stats_t loc;
			chanim_stats_v2_t loc2;

			if (!chanim_stats) {
				*r_size = sprintf(buf, "ERR: Requested info not available");
				goto done;
			}

			count = chanim_stats->count;
			tmp_stats.version = htonl(chanim_stats->version);
			tmp_stats.buflen = htonl(chanim_stats->buflen);
			tmp_stats.count = htonl(chanim_stats->count);

			memcpy((void*)buf, (void*)&tmp_stats, WL_CHANIM_STATS_FIXED_LEN);
			*r_size = WL_CHANIM_STATS_FIXED_LEN;
			buf += *r_size;
			if (chanim_stats->version == WL_CHANIM_STATS_V2) {
				statsv2 = (chanim_stats_v2_t *)chanim_stats->stats;
				for (i = 0; i < count; i++) {
					memcpy((void*)&loc2, (void*)statsv2,
						sizeof(chanim_stats_v2_t));
					loc2.glitchcnt = htonl(statsv2->glitchcnt);
					loc2.badplcp = htonl(statsv2->badplcp);
					loc2.chanspec = htons(statsv2->chanspec);
					loc2.timestamp = htonl(statsv2->timestamp);

					memcpy((void*) buf, (void*)&loc2,
						sizeof(chanim_stats_v2_t));
					*r_size += sizeof(chanim_stats_v2_t);
					buf += sizeof(chanim_stats_v2_t);
					statsv2++;
				}
			} else if (chanim_stats->version <= WL_CHANIM_STATS_VERSION) {
				uint8 *nstats;
				uint elmt_size = 0;

				if (chanim_stats->version == WL_CHANIM_STATS_VERSION_3) {
					elmt_size = sizeof(chanim_stats_v3_t);
				} else if (chanim_stats->version == WL_CHANIM_STATS_VERSION_4) {
					elmt_size = sizeof(chanim_stats_t);
				} else {
					ACSD_ERROR("Unsupported version : %d\n", chanim_stats->version);
					goto done;
				}

				nstats = (uint8 *)chanim_stats->stats;
				stats = chanim_stats->stats;

				for (i = 0; i < count; i++) {
					memcpy((void*)&loc, (void*)stats,
						elmt_size);
					loc.glitchcnt = htonl(stats->glitchcnt);
					loc.badplcp = htonl(stats->badplcp);
					loc.chanspec = htons(stats->chanspec);
					loc.timestamp = htonl(stats->timestamp);

					memcpy((void*) buf, (void*)&loc,
						elmt_size);
					*r_size += elmt_size;
					buf += elmt_size;

					/* move to the next element in the list */
					nstats += elmt_size;
					stats = (chanim_stats_t *)nstats;
				}
			}
		} else if (!strcmp(param, "candidate") || !strcmp(param, "cscore")) {
			ch_candidate_t * candi[ACS_BW_MAX];
			int i;
			bool is_null = TRUE;

			for (i = ACS_BW_20; i < ACS_BW_MAX; i++) {
				candi[i] = c_info->candidate[i];

				if (!candi[i]) {
					continue;
				}

				if (i == ACS_BW_160) {
					ACSD_INFO("160 MHz candidates: count %d\n",
							c_info->c_count[i]);
				} else if (i == ACS_BW_8080) {
					ACSD_INFO("80p80 MHz candidates: count %d\n",
							c_info->c_count[i]);
				} else if (i == ACS_BW_80) {
					ACSD_INFO("80 MHz candidates: count %d\n",
							c_info->c_count[i]);
				} else if (i == ACS_BW_40) {
					ACSD_INFO("40 MHz candidates: count %d\n",
							c_info->c_count[i]);
				} else {
					ACSD_INFO("20 MHz candidates: count %d\n",
							c_info->c_count[i]);
				}

				is_null = FALSE;
				acsd_pass_candi(candi[i], c_info->c_count[i], &buf, r_size);
			}

			if (is_null) {
				*r_size = sprintf(buf, "ERR: Requested info not available");
				goto done;
			}

		} else if (!strcmp(param, "bss")) {
			acs_chan_bssinfo_t *bssinfo = c_info->ch_bssinfo;

			if (!bssinfo) {
				*r_size = sprintf(buf, "ERR: Requested info not available");
				goto done;
			}

			memcpy((void*)buf, (void*)bssinfo, sizeof(acs_chan_bssinfo_t) *
				c_info->scan_chspec_list.count);
			*r_size = sizeof(acs_chan_bssinfo_t) * c_info->scan_chspec_list.count;

		} else if (!strcmp(param, "acs_record")) {
			chanim_info_t * ch_info = c_info->chanim_info;
			acs_record_t record;
			uint8 idx;
			int i, count = 0;

			if (!ch_info) {
				*r_size = sprintf(buf, "ERR: Requested info not available");
				goto done;
			}

			idx = chanim_mark(ch_info).record_idx;

			for (i = 0; i < ACS_MAX_RECORD; i++) {
				if (ch_info->record[idx].valid) {
					bcopy(&ch_info->record[idx], &record,
						sizeof(acs_record_t));
					record.selected_chspc =
						htons(record.selected_chspc);
					record.glitch_cnt =
						htonl(record.glitch_cnt);
					record.timestamp =
						htonl(record.timestamp);
					record.ticks =
						htonl(record.ticks);
					memcpy((void*) buf, (void*)&record,
						sizeof(acs_record_t));
					*r_size += sizeof(acs_record_t);
					buf += sizeof(acs_record_t);

					count++;
					ACSD_DEBUG("count: %d trigger: %d\n",
						count, record.trigger);
				}
				idx = (idx + 1) % ACS_MAX_RECORD;
			}

			ACSD_DEBUG("rsize: %d, sizeof: %zd\n", *r_size,
					sizeof(acs_record_t));

		} else if (!strcmp(param, "acsd_stats")) {
			acsd_stats_t * d_stats = &d_info->stats;

			if (!d_stats) {
				*r_size = sprintf(buf, "ERR: Requested info not available");
				goto done;
			}

			*r_size = sprintf(buf, "ACSD stats:\n");
			*r_size += sprintf(buf + *r_size, "Total cmd: %d, Valid cmd: %d\n",
				d_stats->num_cmds, d_stats->valid_cmds);
			*r_size += sprintf(buf + *r_size, "Total events: %d, Valid events: %d\n",
				d_stats->num_events, d_stats->valid_events);

			goto done;

		} else if (!strcmp(param, "scanresults")) {
			acs_bss_info_entry_t *curptr = c_info->acs_bss_info_q;
			int len, bss_count = 0;
			len = sizeof(acs_bss_info_sm_t);
			while (curptr) {
				if ((*r_size + len) > (ACSD_BUFSIZE_DEFAULT - 1)) {
					break;
				}
				memcpy((void*)buf, curptr, len);
				*r_size += len;
				buf = buf + len;
				curptr = curptr->next;
				bss_count++;
			}
		} else {
			*r_size = sprintf(buf, "Unsupported dump command (try \"dump help\")");
		}
		goto done;
	}

	if (!strcmp(buf, "get")) {
		char param[128];
		int index;
		acs_chaninfo_t *c_info = NULL;

		if ((ret = acsd_extract_token_val(data, "param", param, sizeof(param))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing param");
			goto done;
		}

		if ((ret = acsd_extract_token_val(data, "ifname", ifname, sizeof(ifname))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing ifname");
			goto done;
		}

		ACSD_DEBUG("cmd: %s, data: %s, param: %s, ifname: %s\n",
			buf, data, param, ifname);

		index = acs_idx_from_map(ifname);

		ACSD_DEBUG("index is : %d\n", index);
		if (index != -1)
			c_info = d_info->acs_info->chan_info[index];

		ACSD_DEBUG("c_info: %p\n", c_info);

		if (!c_info) {
			*r_size = sprintf(buf, "ERR: Requested info not available");
			goto done;
		}

		d_info->stats.valid_cmds++;
		if (!strcmp(param, "msglevel")) {
			*r_size = sprintf(buf, "%d", acsd_debug_level);
			goto done;
		}

		if (!strcmp(param, "mode")) {
			const char *mode_str[] = {"disabled", "monitor", "select", "coex",
				"11h", "fixchspec"};
			int acs_mode = c_info->mode;
			*r_size = sprintf(buf, "%d: %s", acs_mode, mode_str[acs_mode]);
			goto done;
		}

		if (!strcmp(param, "acs_cs_scan_timer")) {
			if (c_info->acs_cs_scan_timer)
				*r_size = sprintf(buf, "%d sec", c_info->acs_cs_scan_timer);
			else
				*r_size = sprintf(buf, "OFF");
			goto done;
		}

		if (!strcmp(param, "acs_policy")) {
			*r_size = sprintf(buf, "index: %d : %s", c_info->policy_index,
				acs_policy_name(c_info->policy_index));
			goto done;
		}

		if (!strcmp(param, "acs_flags")) {
			*r_size = sprintf(buf, "0x%x", c_info->flags);
			goto done;
		}

		if (!strcmp(param, "chanim_flags")) {
			chanim_info_t* ch_info = c_info->chanim_info;
			if (ch_info)
				*r_size = sprintf(buf, "0x%x", chanim_config(ch_info).flags);
			else
				*r_size = sprintf(buf, "ERR: Requested info not available");
			goto done;
		}

		if (!strcmp(param, "acs_tx_idle_cnt")) {
			*r_size = sprintf(buf, "%d tx packets", c_info->acs_tx_idle_cnt);
			goto done;
		}

		if (!strcmp(param, "acs_ci_scan_timeout")) {
			*r_size = sprintf(buf, "%d sec", c_info->acs_ci_scan_timeout);
			goto done;
		}

		if (!strcmp(param, "acs_far_sta_rssi")) {
			*r_size = sprintf(buf, "%d", c_info->acs_far_sta_rssi);
			goto done;
		}
		if (!strcmp(param, "acs_nofcs_least_rssi")) {
			*r_size = sprintf(buf, "%d", c_info->acs_nofcs_least_rssi);
			goto done;
		}
		if (!strcmp(param, "acs_scan_chanim_stats")) {
			*r_size = sprintf(buf, "%d", c_info->acs_scan_chanim_stats);
			goto done;
		}
		if (!strcmp(param, "acs_ci_scan_chanim_stats")) {
			*r_size = sprintf(buf, "%d", c_info->acs_ci_scan_chanim_stats);
			goto done;
		}
		if (!strcmp(param, "acs_chan_dwell_time")) {
			*r_size = sprintf(buf, "%d", c_info->acs_chan_dwell_time);
			goto done;
		}
		if (!strcmp(param, "acs_chan_flop_period")) {
			*r_size = sprintf(buf, "%d", c_info->acs_chan_flop_period);
			goto done;
		}
		if (!strcmp(param, "acs_dfs")) {
			*r_size = sprintf(buf, "%d", c_info->acs_dfs);
			goto done;
		}
		if (!strcmp(param, "acs_txdelay_period")) {
			*r_size = sprintf(buf, "%d", c_info->acs_txdelay_period);
			goto done;
		}
		if (!strcmp(param, "acs_txdelay_cnt")) {
			*r_size = sprintf(buf, "%d", c_info->acs_txdelay_cnt);
			goto done;
		}
		if (!strcmp(param, "acs_txdelay_ratio")) {
			*r_size = sprintf(buf, "%d", c_info->acs_txdelay_ratio);
			goto done;
		}
		if (!strcmp(param, ACS_CMD_TEST_DFSR) || !strcmp(param, ACS_CMD_ZDFS_2G_MOVE) ||
				!strcmp(param, ACS_CMD_ZDFS_5G_MOVE)) {
			char ch_str[CHANSPEC_STR_LEN];
			*r_size = sprintf(buf, "chanspec: %7s (0x%04x) for dfsr (CAC mode:%d)",
					wf_chspec_ntoa(c_info->selected_chspec, ch_str),
					c_info->selected_chspec, c_info->cac_mode);
			goto done;
		}
		if ((!strcmp(param, ACS_CMD_TEST_PRECLEAR) ||
				!strcmp(param, ACS_CMD_ZDFS_2G_PRECLEAR) ||
				!strcmp(param, ACS_CMD_ZDFS_5G_PRECLEAR)) &&
				c_info->acs_bgdfs != NULL) {
			char ch_str[CHANSPEC_STR_LEN];
			*r_size = sprintf(buf, "chanspec: %7s (0x%04x) to preclear (CAC mode:%d)",
					wf_chspec_ntoa(c_info->acs_bgdfs->next_scan_chan, ch_str),
					c_info->acs_bgdfs->next_scan_chan, c_info->cac_mode);
			goto done;
		}

		if (!strcmp(param, "acs_switch_score_thresh")) {
			*r_size = sprintf(buf, "%d", c_info->acs_switch_score_thresh);
			goto done;
		}

		if (!strcmp(param, "acs_switch_score_thresh_hi")) {
			*r_size = sprintf(buf, "%d", c_info->acs_switch_score_thresh_hi);
			goto done;
		}

		if (!strcmp(param, "acs_txop_limit_hi")) {
			*r_size = sprintf(buf, "%d", c_info->acs_txop_limit_hi);
			goto done;
		}

		if (!strcmp(param, "acs_ci_scan_timer")) {
			*r_size = sprintf(buf, "%d sec", c_info->acs_ci_scan_timer);
			goto done;
		}

		if (!strcmp(param, "bw_upgradable")) {
			*r_size = sprintf(buf, "%d", c_info->bw_upgradable);
			goto done;
		}

		if (!strcmp(param, "fallback_to_primary")) {
			*r_size = sprintf(buf, "%d", c_info->fallback_to_primary);
			goto done;
		}

		if (!strcmp(param, "ci_scan_txop_limit")) {
			*r_size = sprintf(buf, "%d", c_info->ci_scan_txop_limit);
			goto done;
		}

		if (!strcmp(param, "acs_txop_limit")) {
			*r_size = sprintf(buf, "%d", c_info->acs_txop_limit);
			goto done;
		}

		if (!strcmp(param, "acs_bgdfs_idle_interval") && c_info->acs_bgdfs) {
			*r_size = sprintf(buf, "%d", c_info->acs_bgdfs->idle_interval);
			goto done;
		}

		if (!strcmp(param, "acs_use_escan")) {
			*r_size = sprintf(buf, "%d", c_info->acs_escan->acs_use_escan);
			goto done;
		}

		if (!strcmp(param, "acs_chanim_tx_avail")) {
			*r_size = sprintf(buf, "%d", c_info->acs_chanim_tx_avail);
			goto done;
		}

		if (!strcmp(param, "acs_enable_dfsr_on_highpwr")) {
			*r_size = sprintf(buf, "%d", c_info->acs_enable_dfsr_on_highpwr);
			goto done;
		}

		if (!strcmp(param, "acs_nonwifi_enable")) {
			*r_size = sprintf(buf, "%d", c_info->acs_nonwifi_enable);
			goto done;
		}

		if (!strcmp(param, "acs_chanim_glitch_thresh")) {
			*r_size = sprintf(buf, "%d", c_info->acs_chanim_glitch_thresh);
			goto done;
		}

		if (!strcmp(param, "acs_pref_6g_psc_pri")) {
			*r_size = sprintf(buf, "%d", c_info->acs_pref_6g_psc_pri);
			goto done;
		}

		CHANIM_GET(max_acs, "");
		CHANIM_GET(lockout_period, "sec");
		CHANIM_GET(sample_period, "sec");
		CHANIM_GET(threshold_time, "sample period");
		CHANIM_GET(acs_trigger_var, "dBm");

#ifdef BCM_CEVENT
		if (!strcmp(param, ACS_CMD_TEST_CEVENT)) {
			CE_SEND_CEVENT_A2C(c_info->name, CEVENT_ST_ACSD, CEVENT_A2C_MT_ACS_CH_SW,
					&c_info->selected_chspec, sizeof(chanspec_t));
			goto done;
		}
#endif /* BCM_CEVENT */

		if (!strcmp(param, "acs_dfs_move_back")) {
			if (BAND_5G(c_info->rs_info.band_type)) {
				*r_size = sprintf(buf, "%d", c_info->acs_dfs_move_back);
				goto done;
			}
		}

		if (!strcmp(param, "last_dfs_chspec")) {
			if (BAND_5G(c_info->rs_info.band_type)) {
				*r_size = sprintf(buf, "0x%x", c_info->last_dfs_chspec);
				goto done;
			}
		}

		if (!strcmp(param, "acs_channel_weights")) {
			char ch_wt_buf[ACS_MAX_LIST_LEN * 2 + 2];
			char ch_or_wt_str[8];
			int i, cwlen;

			if (!c_info->channel_weights.count) {
				*r_size = sprintf(buf, "No channel weights specified");
				goto done;
			}
			memset(ch_wt_buf, 0, sizeof(ch_wt_buf));
			memset(ch_or_wt_str, 0, sizeof(ch_or_wt_str));
			for (i = 0; i < c_info->channel_weights.count; i++) {
				snprintf(ch_or_wt_str, sizeof(ch_or_wt_str), "%d,", c_info->channel_weights.ch_list[i]);
				strcat(ch_wt_buf, ch_or_wt_str);
				snprintf(ch_or_wt_str, sizeof(ch_or_wt_str), "%d,", c_info->channel_weights.wt_list[i]);
				strcat(ch_wt_buf, ch_or_wt_str);
			}
			cwlen = strlen(ch_wt_buf);
			ch_wt_buf[cwlen - 1] = '\0';

			*r_size = sprintf(buf, "%s\nChannel weights count:%d",
				ch_wt_buf, c_info->channel_weights.count);
			goto done;
		}

		if (!strcmp(param, "acs_excl_chans")) {
			char excl_chans_buf[ACS_MAX_VECTOR_LEN + 2];
			char ch_str[CHANSPEC_STR_LEN];
			int i, cwlen;

			if (!c_info->excl_chans.count) {
				*r_size = sprintf(buf, "No exclude chans specified");
				goto done;
			}
			memset(ch_str, 0, sizeof(ch_str));
			memset(excl_chans_buf, 0, sizeof(excl_chans_buf));
			for (i = 0; i < c_info->excl_chans.count; i++) {
				snprintf(ch_str, sizeof(ch_str), "0x%x,", c_info->excl_chans.clist[i]);
				strcat(excl_chans_buf, ch_str);
			}
			cwlen = strlen(excl_chans_buf);
			excl_chans_buf[cwlen - 1] = '\0';

			*r_size = sprintf(buf, "%s\nexcl_chans count:%d",
				excl_chans_buf, c_info->excl_chans.count);
			goto done;
		}

		if (!strcmp(param, "acs_chanim_poll")) {
			*r_size = sprintf(buf, "%d", c_info->acs_chanim_poll);
			goto done;
		}

		if (!strcmp(param, "acs_chanim_intf_force_cs_scan")) {
			*r_size = sprintf(buf, "%d", c_info->acs_chanim_intf_force_cs_scan);
			goto done;
		}

		if (!strcmp(param, "acs_bgdfs")) {
			*r_size = sprintf(buf, "%d",
				((c_info->acs_bgdfs != NULL) &&
				(BAND_5G(c_info->rs_info.band_type))) ? 1 : 0);
			goto done;
		}

		if (!strcmp(param, "acs_bgdfs_preclear_etsi")) {
			*r_size = sprintf(buf, "%d",
				((c_info->acs_bgdfs != NULL) &&
				(BAND_5G(c_info->rs_info.band_type)) &&
				(c_info->country_is_edcrs_eu)) ?
				c_info->acs_bgdfs->preclear_etsi : 0);
			goto done;
		}

		if (!strcmp(param, "acs_allow_uncleared_dfs_ch")) {
			*r_size = sprintf(buf, "%d",
				(BAND_5G(c_info->rs_info.band_type)) ?
				c_info->acs_allow_uncleared_dfs_ch : 0);
			goto done;
		}

		*r_size = sprintf(buf, "GET: Unknown variable \"%s\".", param);
		err = -1;
		goto done;
	}

	if (!strcmp(buf, "set")) {
		char param[128];
		char val[ACS_MAX_VECTOR_LEN + 2];
		int setval = 0;
		int index;
		acs_chaninfo_t *c_info = NULL, *zdfs_2g_ci = NULL;

		if ((ret = acsd_extract_token_val(data, "param", param, sizeof(param))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing param");
			goto done;
		}

		if ((ret = acsd_extract_token_val(data, "val", val, sizeof(val))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing val");
			goto done;
		}

		setval = atoi(val);

		if ((ret = acsd_extract_token_val(data, "ifname", ifname, sizeof(ifname))) < 0) {
			*r_size = sprintf(buf, "Request failed: missing ifname");
			goto done;
		}

		index = acs_idx_from_map(ifname);

		ACSD_DEBUG("index is : %d\n", index);
		if (index != -1)
			c_info = d_info->acs_info->chan_info[index];

		ACSD_DEBUG("c_info: %p\n", c_info);

		if (!c_info) {
			*r_size = sprintf(buf, "ERR: Requested ifname not available");
			goto done;
		}

		ACSD_DEBUG("cmd: %s, data: %s, param: %s val: %d, ifname: %s\n",
			buf, data, param, setval, ifname);

		d_info->stats.valid_cmds++;

		if ((zdfs_2g_ci = acs_get_zdfs_2g_ci()) != NULL && zdfs_2g_ci->acs_bgdfs != NULL &&
			zdfs_2g_ci->acs_bgdfs->state != BGDFS_STATE_IDLE &&
			(strcmp(param, "chanspec"))) {
			*r_size = sprintf(buf, "Request not permitted: "
				"BGDFS 2G in progress");
			goto done;
		}

		if (c_info->acs_bgdfs != NULL && c_info->acs_bgdfs->state != BGDFS_STATE_IDLE &&
			(strcmp(param, "chanspec"))) {
			*r_size = sprintf(buf, "Request not permitted: "
				"BGDFS in progress");
			goto done;
		}

		if (!strcmp(param, "msglevel")) {
			acsd_debug_level = setval;
			*r_size = sprintf(buf, "%d", acsd_debug_level);
			goto done;
		}

		if (!strcmp(param, "mode")) {
			const char *mode_str[] = {"disabled", "monitor", "select", "coex",
				"11h", "fixchspec"};

			if (setval < ACS_MODE_DISABLE || setval > ACS_MODE_FIXCHSPEC) {
				*r_size = sprintf(buf, "Out of range");
				goto done;
			}

			/* Update status before switching acs mode */
			acs_update_status(c_info);

			c_info->mode = setval;
			*r_size = sprintf(buf, "%d: %s", setval, mode_str[setval]);
			ACSD_DEBUG("Setting ACSD mode = %d: %s\n", setval, mode_str[setval]);
			goto done;
		}

		if (!strcmp(param, "acs_cs_scan_timer")) {
			if (setval != 0 && setval < ACS_CS_SCAN_TIMER_MIN) {
				*r_size = sprintf(buf, "Out of range");
				goto done;
			}

			c_info->acs_cs_scan_timer = setval;

			if (setval)
				*r_size = sprintf(buf, "%d sec", c_info->acs_cs_scan_timer);
			else
				*r_size = sprintf(buf, "OFF");
			goto done;
		}

		if (!strcmp(param, "acs_policy")) {
			if (setval > ACS_POLICY_MAX || setval < 0)  {
				*r_size = sprintf(buf, "Out of range");
				goto done;
			}

			c_info->policy_index = setval;

			if (setval != ACS_POLICY_USER)
				acs_default_policy(&c_info->acs_policy, setval);

			*r_size = sprintf(buf, "index: %d : %s", setval, acs_policy_name(setval));
			goto done;
		}

		if (!strcmp(param, "acs_flags")) {

			c_info->flags = setval;

			*r_size = sprintf(buf, "flags: 0x%x", c_info->flags);
			goto done;
		}

		if (!strcmp(param, "chanim_flags")) {
			chanim_info_t* ch_info = c_info->chanim_info;
			if (!ch_info) {
				*r_size = sprintf(buf, "ERR: set action not successful");
				goto done;
			}
			chanim_config(ch_info).flags = setval;
			*r_size = sprintf(buf, "0x%x", chanim_config(ch_info).flags);
			goto done;

		}

		if (!strcmp(param, "acs_tx_idle_cnt")) {
			c_info->acs_tx_idle_cnt = setval;
			*r_size = sprintf(buf, "%d tx packets", c_info->acs_tx_idle_cnt);
			goto done;
		}

		if (!strcmp(param, "acs_ci_scan_timeout")) {
			c_info->acs_ci_scan_timeout = setval;
			*r_size = sprintf(buf, "%d sec", c_info->acs_ci_scan_timeout);
			goto done;
		}

		if (!strcmp(param, "acs_far_sta_rssi")) {
			c_info->acs_far_sta_rssi = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_far_sta_rssi);
			goto done;
		}
		if (!strcmp(param, "acs_nofcs_least_rssi")) {
			c_info->acs_nofcs_least_rssi = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_nofcs_least_rssi);
			goto done;
		}
		if (!strcmp(param, "acs_scan_chanim_stats")) {
			c_info->acs_scan_chanim_stats = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_scan_chanim_stats);
			goto done;
		}
		if (!strcmp(param, "acs_ci_scan_chanim_stats")) {
			c_info->acs_ci_scan_chanim_stats = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_ci_scan_chanim_stats);
			goto done;
		}
		if (!strcmp(param, "acs_chan_dwell_time")) {
			c_info->acs_chan_dwell_time = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_chan_dwell_time);
			goto done;
		}
		if (!strcmp(param, "acs_chan_flop_period")) {
			c_info->acs_chan_flop_period = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_chan_flop_period);
			goto done;
		}
		if (!strcmp(param, "acs_dfs")) {
			c_info->acs_dfs = setval;
			acs_dfsr_enable(ACS_DFSR_CTX(c_info), (setval == ACS_DFS_REENTRY));
			*r_size = sprintf(buf, "%d", c_info->acs_dfs);
			goto done;
		}
		if (!strcmp(param, "acs_txdelay_period")) {
			c_info->acs_txdelay_period = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_txdelay_period);
			goto done;
		}
		if (!strcmp(param, "acs_txdelay_cnt")) {
			c_info->acs_txdelay_cnt = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_txdelay_cnt);
			goto done;
		}
		if (!strcmp(param, "acs_txdelay_ratio")) {
			c_info->acs_txdelay_ratio = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_txdelay_ratio);
			goto done;
		}
		if (!strcmp(param, "acs_ci_scan_timer")) {
			c_info->acs_ci_scan_timer = setval;
			*r_size = sprintf(buf, "%d sec", c_info->acs_ci_scan_timer);
			goto done;
		}

		if (!strcmp(param, "ci_scan_txop_limit")) {
			c_info->ci_scan_txop_limit = setval;
			*r_size = sprintf(buf, "%d", c_info->ci_scan_txop_limit);
			goto done;
		}

		if (!strcmp(param, "acs_txop_limit")) {
			c_info->acs_txop_limit = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_txop_limit);
			goto done;
		}

		if (!strcmp(param, "acs_switch_score_thresh")) {
			c_info->acs_switch_score_thresh = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_switch_score_thresh);
			goto done;
		}

		if (!strcmp(param, "acs_switch_score_thresh_hi")) {
			c_info->acs_switch_score_thresh_hi = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_switch_score_thresh_hi);
			goto done;
		}

		if (!strcmp(param, "acs_txop_limit_hi")) {
			c_info->acs_txop_limit_hi = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_txop_limit_hi);
			goto done;
		}

		if (!strcmp(param, ACS_CMD_TEST_DFSR)) {
			acsd_dfs_cmd(c_info, buf, param, val, r_size, TRUE,
					ACS_CAC_MODE_AUTO);
			goto done;
		}
		if (!strcmp(param, ACS_CMD_TEST_PRECLEAR)) {
			acsd_dfs_cmd(c_info, buf, param, val, r_size, FALSE,
					ACS_CAC_MODE_AUTO);
			goto done;
		}
		if (!strcmp(param, ACS_CMD_ZDFS_5G_MOVE)) {
			acsd_dfs_cmd(c_info, buf, param, val, r_size, TRUE,
					ACS_CAC_MODE_ZDFS_5G_ONLY);
			goto done;
		}
		if (!strcmp(param, ACS_CMD_ZDFS_2G_MOVE)) {
			acsd_dfs_cmd(c_info, buf, param, val, r_size, TRUE,
					ACS_CAC_MODE_ZDFS_2G_ONLY);
			goto done;
		}
		if (!strcmp(param, ACS_CMD_ZDFS_5G_PRECLEAR)) {
			acsd_dfs_cmd(c_info, buf, param, val, r_size, FALSE,
					ACS_CAC_MODE_ZDFS_5G_ONLY);
			goto done;
		}
		if (!strcmp(param, ACS_CMD_ZDFS_2G_PRECLEAR)) {
			acsd_dfs_cmd(c_info, buf, param, val, r_size, FALSE,
					ACS_CAC_MODE_ZDFS_2G_ONLY);
			goto done;
		}

		if (!strcmp(param, "acs_chanim_tx_avail")) {
			c_info->acs_chanim_tx_avail = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_chanim_tx_avail);
			goto done;
		}

		if (!strcmp(param, "acs_enable_dfsr_on_highpwr")) {
			c_info->acs_enable_dfsr_on_highpwr = setval;
			*r_size = sprintf(buf, "%d", c_info->acs_enable_dfsr_on_highpwr);
			goto done;
		}

		if (!strcmp(param, "chanspec")) {
			unsigned int tmp;
			char option[16] = {0};
			int do_dfs_ap_move = FALSE;

			if ((ret = acsd_extract_token_val(data, "option", option,
				sizeof(option))) < 0) {
				*r_size = sprintf(buf, "Request failed: missing option");
				goto done;
			}
			sscanf(val, "%d", &tmp);
			sscanf(option, "%d", &do_dfs_ap_move);
			acs_process_cmd(c_info, (chanspec_t)tmp, do_dfs_ap_move);
			goto done;
		}
		/* ACSD CLI command format: controller forwards repeater stats to acsd for
		 * improved channel selection.
		 acs_cli2 -i wl<x> "channelscanrpt&value=%d&devid=%s&radiomac=%s&chspec20=%d
		 &nctrl=%d&next20=%d&next40=%d&next80=%d&util=%d&noise=%d" set
		 */
		if (!strcmp(param, "channelscanrpt")) {
			int nctrl_val = 0, next20_val = 0, next40_val = 0, next80_val = 0;
			int noise_val = 0, util_val = 0, i = 0, cli_ver = 0;
			char devid[20] = {0}, radiomac[20] = {0}, chspec20[16] = {0};
			char nctrl[8] = {0}, next20[8] = {0}, next40[8] = {0}, next80[8] = {0};
			char util[8] = {0}, noise[8] = {0};
			char device_mac_string[32] = {0}, radio_mac_string[32] = {0};
			chanspec_t chanspec;
			struct ether_addr src_dev_mac, src_radio_mac;
			uint8 ether_bcast[ETHER_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

			if (!c_info->perchan_stats) {
				ACSD_DEBUG("ifname: %s memory is not allocated for perchan"
						"stats structure\n", c_info->name);
				goto done;
			}

			if (!c_info->acs_chan_change_through_rep_stats) {
				ACSD_DEBUG("ifname: %s Don't collect stats when chan change"
						" feature is disabled\n", c_info->name);
				goto done;
			}

			if ((ret = acsd_extract_token_val(data, "chspec20", chspec20,
				sizeof(chspec20))) < 0) {
				*r_size = sprintf(buf, "Request failed: missing option");
				goto done;
			}
			sscanf(chspec20, "%u", (unsigned int *)&chanspec);

			/* Copy repeater stats only if rep_chanspec matches with local stats */
			for (i = 0; i < c_info->perchan_count; i++) {
				if ((c_info->perchan_stats[i].chanspec) != chanspec) {
					continue;
				}

				/* Fetch cli ver */
				sscanf(val, "%d", &cli_ver);
				ACSD_DEBUG("cli version is %d\n", cli_ver);

				/* Fetch dev mac address */
				if (strstr(data, "devid") && (c_info->device_count == 0)) {
					if ((ret = acsd_extract_token_val(data, "devid", devid,
							sizeof(devid))) < 0) {
						*r_size = sprintf(buf, "Request failed:"
								"missing option");
						goto done;
					}
					sscanf(devid, "%s", device_mac_string);
					bcm_ether_atoe(device_mac_string, &src_dev_mac);
					memcpy(&(c_info->last_device_mac), &src_dev_mac,
							ETHER_ADDR_LEN);
					ACSD_DEBUG("last device add is [%x:%x:%x:%x:%x:%x]\n",
							MAC2STRDBG(c_info->last_device_mac));
					c_info->device_count ++;
				}

				/* Fetch radio mac address */
				if (strstr(data, "radiomac")) {
					if ((ret = acsd_extract_token_val(data, "radiomac",
							radiomac, sizeof(radiomac))) < 0) {
						*r_size = sprintf(buf, "Request failed: "
								"missing option");
						goto done;
					}
					sscanf(radiomac, "%s", radio_mac_string);
					bcm_ether_atoe(radio_mac_string, &src_radio_mac);
					memcpy(&(c_info->radio_mac), &src_radio_mac,
							ETHER_ADDR_LEN);
					ACSD_DEBUG("ifname: %s radio mac is"
							"[%x:%x:%x:%x:%x:%x]\n", c_info->name,
							MAC2STRDBG(c_info->radio_mac));
					if (!memcmp(&ether_bcast, &src_radio_mac, ETHER_ADDR_LEN)) {
						c_info->end_repeater_stats = TRUE;
						ACSD_DEBUG("ifname: %s end of repeater stats\n",
								c_info->name);
					}

				}

				/* Fetch number of Bss control channel */
				if ((ret = acsd_extract_token_val(data,	"nctrl", nctrl,
						sizeof(nctrl))) < 0) {
					*r_size = sprintf(buf, "Request failed: "
							"missing option");
					goto done;
				}
				sscanf(nctrl, "%d", &nctrl_val);
				if (!c_info->scanrpt_change &&
						ABS(c_info->perchan_stats[i].rep_nbr_nctrl -
						nctrl_val) > c_info->bss_sign_change) {
					c_info->scanrpt_change = TRUE;
				}
				c_info->perchan_stats[i].rep_nbr_nctrl = nctrl_val;
				ACSD_DEBUG("ifname: %s nctrl is %d\n",
						c_info->name,
						c_info->perchan_stats[i].rep_nbr_nctrl);

				/* Fetch number of neighbors in next 20Mhz */
				if ((ret = acsd_extract_token_val(data,	"next20", next20,
						sizeof(next20))) < 0) {
					*r_size = sprintf(buf, "Request failed: "
							"missing option");
					goto done;
				}
				sscanf(next20, "%d", &next20_val);
				if (!c_info->scanrpt_change &&
						ABS(c_info->perchan_stats[i].rep_nbr_next20 -
						next20_val) > c_info->bss_sign_change) {
					c_info->scanrpt_change = TRUE;
				}
				c_info->perchan_stats[i].rep_nbr_next20 = next20_val;
				ACSD_DEBUG("ifname: %s next20 is %d\n",	c_info->name,
						c_info->perchan_stats[i].rep_nbr_next20);

				/* Fetch number of neighbors in next 40Mhz */
				if ((ret = acsd_extract_token_val(data, "next40", next40,
						sizeof(next40))) < 0) {
					*r_size = sprintf(buf, "Request failed: "
							"missing option");
					goto done;
				}
				sscanf(next40, "%d", &next40_val);
				if (!c_info->scanrpt_change &&
						ABS(c_info->perchan_stats[i].rep_nbr_next40 -
						next40_val) > c_info->bss_sign_change) {
					c_info->scanrpt_change = TRUE;
				}
				c_info->perchan_stats[i].rep_nbr_next40 = next40_val;
				ACSD_DEBUG("ifname: %s next40 is %d\n",	c_info->name,
						c_info->perchan_stats[i].rep_nbr_next40);

				/* Fetch number of neighbors in next 80Mhz */
				if ((ret = acsd_extract_token_val(data,	"next80", next80,
						sizeof(next80))) < 0) {
					*r_size = sprintf(buf, "Request failed: "
							"missing option");
					goto done;
				}
				sscanf(next80, "%d", &next80_val);
				if (!c_info->scanrpt_change &&
						ABS(c_info->perchan_stats[i].rep_nbr_next80 -
						next80_val) > c_info->bss_sign_change) {
					c_info->scanrpt_change = TRUE;
				}
				c_info->perchan_stats[i].rep_nbr_next80 = next80_val;
				ACSD_DEBUG("ifname: %s next80 is %d\n",	c_info->name,
						c_info->perchan_stats[i].rep_nbr_next80);

				/* Fetch channel utilization */
				if ((ret = acsd_extract_token_val(data, "util", util,
						sizeof(util))) < 0) {
					*r_size = sprintf(buf, "Request failed: "
							"missing option");
					goto done;
				}
				sscanf(util, "%d", &util_val);
				util_val = ((util_val * 100) / 255);
				if (!c_info->scanrpt_change &&
						ABS(c_info->perchan_stats[i].rep_nbr_util -
						util_val) > c_info->chan_util_sign_change) {
					c_info->scanrpt_change = TRUE;
				}
				c_info->perchan_stats[i].rep_nbr_util =	util_val;
				ACSD_DEBUG(" ifname: %s util is %d\n", c_info->name,
						c_info->perchan_stats[i].rep_nbr_util);

				/* Fetch bgnoise */
				if ((ret = acsd_extract_token_val(data, "noise", noise,
						sizeof(noise))) < 0) {
					*r_size = sprintf(buf, "Request failed: "
							"missing option");
					goto done;
				}
				sscanf(noise, "%d", &noise_val);
				if (!c_info->scanrpt_change &&
						ABS(c_info->perchan_stats[i].rep_nbr_noise -
						noise_val) > c_info->acs_rep_noise) {
					c_info->scanrpt_change = TRUE;
				}
				c_info->perchan_stats[i].rep_nbr_noise = noise_val;
				ACSD_DEBUG(" ifname: %s noise is %d\n", c_info->name,
						c_info->perchan_stats[i].rep_nbr_noise);
			}
			goto done;
		}
		/* ACSD CLI command "excludechspeclist" : for SmartMesh Controller to forward
		 * All Agent's Non-operable Chaspecs to ACSD for improved Channel Selection.
		 */
		if (!strcmp(param, "excludechspeclist")) {
			char excludelist[ACS_MAX_VECTOR_LEN + 2] = {0};
			char nexclude[16] = {0}, excludelist_val[ACS_MAX_VECTOR_LEN + 2] = {0};
			int cli_ver = 0;
			int nexclude_val = 0;

			if (!c_info->acs_chan_change_through_rep_stats) {
				ACSD_DEBUG("ifname: %s Don't collect stats when chan change"
						" feature is disabled\n", c_info->name);
				goto done;
			}

			/* Fetch CLI Version */
			sscanf(val, "%d", &cli_ver);
			ACSD_DEBUG("IFR[%s] cli_ver[%d] ", c_info->name, cli_ver);

			/* Fetch Number of Non-operable Chaspecs */
			ret = acsd_extract_token_val(data, "nexclude", nexclude, sizeof(nexclude));
			if (ret < 0) {
				*r_size = sprintf(buf, "Request failed: missing option");
				goto done;
			}
			sscanf(nexclude, "%d", &nexclude_val);
			ACSD_DEBUG("nexclude_val[%d] ", nexclude_val);

			/* Non-operable Chaspecs String of Hex Chanspecs separated by comma */
			ret = acsd_extract_token_val(data, "excludelist", excludelist,
					nexclude_val*7);
			if (ret < 0) {
				*r_size = sprintf(buf, "Request failed: missing option");
				goto done;
			}
			sscanf(excludelist, "%s", excludelist_val);
			ACSD_DEBUG("excludelist_val[%s] ", excludelist_val);

			/* Reset Previous Exclude List of Repeaters */
			memset(c_info->excl_chans.rep_clist, 0, sizeof(c_info->excl_chans.rep_clist));
			c_info->excl_chans.rep_excl_count = 0;

			/* Set New Exclude List of Repeaters */
			c_info->excl_chans.rep_excl_count = acs_set_chan_table(excludelist_val,
					c_info->excl_chans.rep_clist,
					ARRAYSIZE(c_info->excl_chans.rep_clist));
			if (c_info->excl_chans.rep_excl_count != nexclude_val) {
				ACSD_ERROR("ifname:%s WBD excl list count:%d is not matching with"
						" local excl list count:%d\n", c_info->name,
						nexclude_val, c_info->excl_chans.rep_excl_count);
			}
			goto done;
		}

		CHANIM_SET(max_acs, "", uint8);
		CHANIM_SET(acs_trigger_var, "dBm", int8);
		CHANIM_SET(lockout_period, "sec", uint32);
		CHANIM_SET(sample_period, "sec", uint8);
		CHANIM_SET(threshold_time, "sample period", uint8);

#ifdef BCM_CEVENT
		if (!strcmp(param, ACS_CMD_TEST_CEVENT)) {
			if (setval == CEVENT_A2C_MT_ACS_CH_SW) {
				CE_SEND_CEVENT_A2C(c_info->name, CEVENT_ST_ACSD, setval,
						&c_info->selected_chspec, sizeof(chanspec_t));
			} else {
				CE_SEND_CEVENT_A2C(c_info->name, CEVENT_ST_ACSD, setval, NULL, 0);
			}
			goto done;
		}
#endif /* BCM_CEVENT */

		if (!strcmp(param, "acs_dfs_move_back")) {
			if (BAND_5G(c_info->rs_info.band_type)) {
				c_info->acs_dfs_move_back = setval;
				*r_size = sprintf(buf, "%d", c_info->acs_dfs_move_back);
				goto done;
			}
		}

		if (!strcmp(param, "acs_channel_weights")) {
			acs_set_chan_weight_table(c_info, val,
				ARRAYSIZE(c_info->channel_weights.ch_list),
				c_info->rs_info.band_type);
			*r_size = sprintf(buf, "%s", val);
			goto done;
		}

		if (!strcmp(param, "acs_excl_chans")) {
			c_info->excl_chans.count = acs_set_chan_table(val, c_info->excl_chans.clist, ACS_MAX_LIST_LEN);
			*r_size = sprintf(buf, "%s", val);
			goto done;
		}

		if (!strcmp(param, "acs_bgdfs")) {
			char prefix[32], conf_word[128], tmp[200];
			int acs_bgdfs_enab = 0;
			bool bgdfs_cap = FALSE;

			*r_size = sprintf(buf, "%s", val);
			acs_snprintf(prefix, sizeof(prefix), "%s_", ifname);
			if (!BAND_5G(c_info->rs_info.band_type)) {
				goto done;
			}
			if (setval) {
				/* Set acs_bgdfs = enable */
				if (c_info->acs_bgdfs != NULL) {
					goto done;
				} else if (c_info->acs_bgdfs_saved != NULL) {
					c_info->acs_bgdfs = c_info->acs_bgdfs_saved;
					c_info->acs_allow_immediate_dfsr =
						ACS_IMMEDIATE_DFSR_DISABLE;
					goto done;
				}
				if (c_info->acs_bgdfs == NULL && c_info->acs_bgdfs_saved == NULL) {
					/* Assume that acs_bgdfs_enab nvram is enbled;
					 * read back nvram to verify.
					 */
					acs_safe_get_conf(conf_word, sizeof(conf_word),
						strcat_r(prefix, "acs_bgdfs_enab", tmp));
					if (!strcmp(conf_word, "")) {
						ACSD_INFO("%s: No acs_bgdfs_enab is set.\n",
							c_info->name);
					} else {
						char *endptr = NULL;
						acs_bgdfs_enab = strtoul(conf_word, &endptr, 0);
						ACSD_DEBUG("%s: acs_bgdfs_enab: 0x%x\n",
							c_info->name, acs_bgdfs_enab);
					}
					acs_bgdfs_enab = ACS_BGDFS_ENAB;

					/* To enable acs_bgdfs, bgdfs_cap must be enabled */
					bgdfs_cap = acs_bgdfs_capable(c_info);
					if (acs_bgdfs_enab && bgdfs_cap) {
						/* allocate core data structure for bgdfs */
						c_info->acs_bgdfs =
							(acs_bgdfs_info_t *)acsd_malloc(
								sizeof(*(c_info->acs_bgdfs)));
						c_info->acs_bgdfs_saved = c_info->acs_bgdfs;

						acs_retrieve_config_bgdfs(c_info->acs_bgdfs, prefix);
					} else {
						ACSD_ERROR("%s: Cannot enable acs_bgdfs, "
							"bgdfs_cap:%d\n", ifname, bgdfs_cap);
						goto done;
					}

					/* Allow acsd to change channels using immediate dfs if
					 * current channel is bad or on significant change;
					 * assume acs_allow_immediate_dfsr nvram is enabled;
					 * read back nvram to verify.
					 */
					acs_safe_get_conf(conf_word, sizeof(conf_word),
						strcat_r(prefix, "acs_allow_immediate_dfsr", tmp));
					if (!strcmp(conf_word, "")) {
						ACSD_INFO("%s: No acs_allow_immediate_dfsr set.\n",
							c_info->name);
					} else {
						char *endptr = NULL;
						uint8 acs_allow_immediate_dfsr =
							strtoul(conf_word, &endptr, 0);
						ACSD_DEBUG("%s: acs_allow_immediate_dfsr: %d\n",
							c_info->name, acs_allow_immediate_dfsr);
					}
					c_info->acs_allow_immediate_dfsr =
						ACS_IMMEDIATE_DFSR_DISABLE;
				}
			} else {
				/* Set acs_bgdfs = disable */
				if (c_info->acs_bgdfs == NULL) {
					goto done;
				} else {
					c_info->acs_bgdfs_saved = c_info->acs_bgdfs;
					c_info->acs_bgdfs = NULL;
					c_info->acs_allow_immediate_dfsr = TRUE;
				}

			}
			goto done;
		}

		if (!strcmp(param, "acs_bgdfs_preclear_etsi")) {
			if (c_info->acs_bgdfs != NULL && BAND_5G(c_info->rs_info.band_type) &&
				c_info->country_is_edcrs_eu) {
				c_info->acs_bgdfs->preclear_etsi = (setval) ? 1 : 0;
				*r_size = sprintf(buf, "%d", c_info->acs_bgdfs->preclear_etsi);
			} else {
				*r_size = sprintf(buf, "%d", 0);
			}
			goto done;
		}

		if (!strcmp(param, "acs_allow_uncleared_dfs_ch")) {
			if (BAND_5G(c_info->rs_info.band_type)) {
				c_info->acs_allow_uncleared_dfs_ch = (setval) ? 1 : 0;
				*r_size = sprintf(buf, "%d", c_info->acs_allow_uncleared_dfs_ch);
			} else {
				*r_size = sprintf(buf, "%d", 0);
			}
			goto done;
		}

		*r_size = sprintf(buf, "SET: Unknown variable \"%s\".", param);
		err = -1;
		goto done;
	}
done:
	return err;
}
