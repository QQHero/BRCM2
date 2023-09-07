/*
 * wlc_taf_ias.c
 *
 * This file implements the WL driver infrastructure for the TAF module.
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
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_taf_ias.c 810531 2022-04-08 13:41:16Z $
 *
 */
/*
 * Include files.
 */

#include <wlc_taf_cmn.h>

#ifdef WLTAF_IAS
#include <wlc_taf_priv_ias.h>
#include <wlc_stf.h>

#if TAF_ENABLE_MU_TX
#include <wlc_fifo.h>
#endif

#include <wlc_apps.h>

const taf_scheduler_def_t taf_ias_scheduler =
{
	taf_ias_method_attach,
	taf_ias_method_detach,
	taf_ias_up,
	taf_ias_down,
	taf_ias_get_list_head_ptr
};

/* scoring weights according AC (via tid), normalised to 256;
 * lower score gives higher air access ratio
 */
/* XXX currently this is only working between different SCB, it has no effect relative to
 * flows of different priority to the same SCB
 */
static uint8 taf_tid_score_weights[NUMPRIO] = {
	 80,	/* 0	AC_BE	Best Effort */
	 80,	/* 1	AC_BK	Background */
	 80,	/* 2	AC_BK	Background */
	 80,	/* 3	AC_BE	Best Effort */
	 80,	/* 4	AC_VI	Video */
	 80,	/* 5	AC_VI	Video */
	 80,	/* 6	AC_VO	Voice */
	 80	/* 7	AC_VO	Voice */
};

#if TAF_ENABLE_MU_TX
static uint16 taf_mu_pair[TAF_MAX_MU] = {
	0,
	360,  /* 2 */
	360,  /* 3 */
	360,  /* 4 */
	360,  /* 5 */
	360,  /* 6 */
	360,  /* 7 */
	360   /* 8 */
};
static uint16 taf_mu_mimo_rel_limit[TAF_MAX_MU_MIMO] = {
	TAF_TIME_HIGH_DEFAULT,
	5000, /* 2 */
	5000, /* 3 */
	5000, /* 4 */
	/* avoid flooding FIFO too much as MU users go > 4 by reducing release amount per user */
	(5000 * 4) / 5,  /* 5 */
	(5000 * 4) / 6,  /* 6 */
	(5000 * 4) / 7,  /* 7 */
	(5000 * 4) / 8,  /* 8 */
};
static uint16 taf_dl_ofdma_rel_limit[TAF_MAX_MU_OFDMA] = {
	TAF_TIME_HIGH_DEFAULT,      /* 1 */
	TAF_TIME_HIGH_DEFAULT,      /* 2 */
	TAF_TIME_HIGH_DEFAULT,      /* 3 */
	TAF_TIME_HIGH_DEFAULT,      /* 4 */
	TAF_TIME_HIGH_DEFAULT,      /* 5 */
	TAF_TIME_HIGH_DEFAULT,      /* 6 */
	TAF_TIME_HIGH_DEFAULT,      /* 7 */
	TAF_TIME_HIGH_DEFAULT       /* 8 */
};
#if TAF_ENABLE_UL
static uint16 taf_ul_ofdma_rel_limit[TAF_MAX_MU_OFDMA] = {
	5300,      /* 1 */
	5300 / 2,  /* 2 */
	4200 / 3,  /* 3 */
	5300 / 4,  /* 4 */
	5300 / 6,  /* 5 */
	5300 / 6,  /* 6 */
	5300 / 8,  /* 7 */
	5300 / 8,  /* 8 */
};
#endif /* TAF_ENABLE_UL */
#endif /* TAF_ENABLE_MU_TX */

static void BCMFASTPATH taf_ias_list_recombine_sort(taf_method_info_t *method)
{
	TAF_ASSERT(method->ordering == TAF_LIST_SCORE_MINIMUM);
	WL_TAFM2(method, "recombine active to main list and full sort\n");

	wlc_taf_list_append(&method->list, method->active_list);
	method->active_list = NULL;
	wlc_taf_sort_list(&method->list, method->ordering, TAF_IAS_NOWTIME(method));
}

static taf_list_t ** taf_ias_get_list_head_ptr(void * context)
{
	taf_method_info_t *method = (taf_method_info_t *) context;

	if (method->active_list) {
		taf_ias_list_recombine_sort(method);
	}
	return &method->list;
}

static INLINE taf_list_t * taf_ias_get_list_head(taf_method_info_t * method)
{
	return *taf_ias_get_list_head_ptr(method);
}

static taf_rspec_index_t BCMFASTPATH
taf_ias_get_rindex(taf_scb_cubby_t *scb_taf, taf_list_type_t type, taf_tech_type_t tech)
{
	uint8 ratespec_mask = scb_taf->info.scb_stats.global.rdata.ratespec_mask;

#if TAF_ENABLE_UL
	if (type == TAF_TYPE_UL) {
		if (ratespec_mask & (1 << TAF_RSPEC_UL)) {
			return TAF_RSPEC_UL;
		}
		goto error;
	}
#endif /* TAF_ENABLE_UL */

#if TAF_ENABLE_MU_TX
	if (!scb_taf->info.ps_mode && TAF_TECH_MASK_IS_MUMIMO(1 << tech)) {
#if TAF_ENABLE_MU_BOOST
		if (ratespec_mask & (1 << TAF_RSPEC_MU_DL_BOOST)) {
			return TAF_RSPEC_MU_DL_BOOST;
		}
#endif
		if (ratespec_mask & (1 << TAF_RSPEC_MU_DL_INSTANT)) {
			return TAF_RSPEC_MU_DL_INSTANT;
		}
		goto error;
	}
	if (!scb_taf->info.ps_mode && tech == TAF_TECH_DL_OFDMA) {
	}
#endif /* TAF_ENABLE_MU_TX */

	if (ratespec_mask & (1 << TAF_RSPEC_SU_DL_INSTANT)) {
		return TAF_RSPEC_SU_DL_INSTANT;
	}

	if (ratespec_mask & (1 << TAF_RSPEC_SU_DL)) {
		return TAF_RSPEC_SU_DL;
	}
#if TAF_ENABLE_MU_TX
error:
#endif
	WL_ERROR(("wl%u %s: "MACF"%s tech %d, ratemask 0x%x, tech_en mask 0x%x\n",
		WLCWLUNIT(TAF_WLCC(scb_taf)), __FUNCTION__, TAF_ETHERC(scb_taf), TAF_TYPE(type),
		tech, ratespec_mask, scb_taf->info.tech_enable_mask));
	TAF_ASSERT(0);
	return TAF_RSPEC_SU_DL;
}

static INLINE void BCMFASTPATH taf_ias_get_pktrate(taf_scb_cubby_t *scb_taf, int tid, uint32 aggsf,
	taf_rspec_index_t rindex, taf_source_type_t s_idx, uint32* pkt_rate, uint32* byte_rate)
{
	taf_scheduler_scb_stats_t * scb_stats = &scb_taf->info.scb_stats;

	if (aggsf == 0) {
		aggsf = 1;
		/* Temporarily bypass until we understand why this is firing */
		//TAF_ASSERT(TAF_SOURCE_IS_UL(s_idx));
	}

#if TAF_ENABLE_NAR
	if (taf_ias_is_nar_traffic(scb_taf, tid, s_idx)) {
		*pkt_rate = scb_stats->ias.data.nar_pkt_rate;
		*byte_rate = scb_stats->ias.data.nar_byte_rate;

		if (aggsf > 1) {
			*pkt_rate /= aggsf;
		}
	} else
#endif /* TAF_ENABLE_NAR */
#if TAF_ENABLE_UL
	if (TAF_SOURCE_IS_UL(s_idx)) {
		*byte_rate = scb_stats->ias.data.byte_rate[rindex];
		*pkt_rate = 0;
	} else
#endif /* TAF_ENABLE_UL */
	{
		*pkt_rate = scb_stats->ias.data.pkt_rate[rindex];

		if (TAF_OPT(scb_taf->method, PKT_AMPDU_OVERHEAD) &&
			(scb_taf->info.max_pdu * aggsf) >= 1) {
			uint32 adjust = (scb_stats->ias.data.overhead_without_rts <<
				TAF_PKTBYTES_COEFF_BITS) / (scb_taf->info.max_pdu * aggsf);

			*pkt_rate += adjust;
		}
		*byte_rate = scb_stats->ias.data.byte_rate[rindex];
	}
	WL_TAFM3(scb_taf->method, MACF" tid %u, %s aggsf %u, byte_rate %u, pkt_rate %u\n",
		TAF_ETHERC(scb_taf), tid, TAF_SOURCE_NAME(s_idx), aggsf,
		*byte_rate, *pkt_rate);
}

static INLINE uint32 BCMFASTPATH
taf_ias_est_release_time(taf_scb_cubby_t *scb_taf, int tid, taf_ias_sched_data_t* ias_sched,
	int len, taf_rspec_index_t rindex, taf_source_type_t s_idx, taf_list_type_t type)
{
	uint16 est_pkt_size;
	uint32 pktbytes;
	uint32 pkt_rate = 0;
	uint32 byte_rate = 0;

	if (ias_sched == NULL) {
		TAF_ASSERT(0);
		return 0;
	}

	est_pkt_size = ias_sched->rel_stats.data.pkt_size_mean[type];

	if (est_pkt_size == 0) {
		est_pkt_size = (type == TAF_TYPE_DL) ?
			TAF_PKT_SIZE_DEFAULT_DL : TAF_PKT_SIZE_INIT_UL(&ias_sched->rel_stats);
	}
	taf_ias_get_pktrate(scb_taf, tid, TAF_GET_AGG(ias_sched->aggsf), rindex, s_idx, &pkt_rate,
		&byte_rate);

	pktbytes = TAF_PKTBYTES_TO_UNITS(est_pkt_size, pkt_rate, byte_rate);

	if (pktbytes < ias_cfg(scb_taf->method).min_mpdu_dur[type]) {
		pktbytes = ias_cfg(scb_taf->method).min_mpdu_dur[type];
	}

	WL_TAFM3(scb_taf->method,
		MACF" tid %u, ridx %u, pktsize %u, pkt_rate %u, byte rate %u, pktbytes %u\n",
		TAF_ETHERC(scb_taf), tid, rindex, est_pkt_size, pkt_rate, byte_rate, pktbytes);

	return pktbytes * len;
}

static INLINE uint32 BCMFASTPATH
taf_ias_get_new_traffic_estimate(taf_scb_cubby_t *scb_taf, int tid, taf_rspec_index_t rindex,
	taf_list_type_t type)
{
	uint32 est_release_total = 0;
	uint32 total_pkt_count = 0;
	taf_source_type_t s_idx;

#if TAF_ENABLE_UL
	/* UL not handled here yet */
	TAF_ASSERT(type != TAF_TYPE_UL);
#endif /* TAF_ENABLE_UL */

	for (s_idx = TAF_FIRST_REAL_SOURCE; s_idx < TAF_NUM_SCHED_SOURCES; s_idx ++) {
		if (type != TAF_SRC_TO_TYPE(s_idx)) {
			continue;
		}
		total_pkt_count += scb_taf->info.traffic.count[s_idx][tid];
	}
	if (total_pkt_count > 0) {
		est_release_total =
			taf_ias_est_release_time(scb_taf, tid, TAF_IAS_TID_STATE(scb_taf, tid),
				total_pkt_count, rindex, TAF_SOURCE_UNDEFINED, type);
	}

	return est_release_total;
}

static INLINE void BCMFASTPATH
taf_ias_upd_item_stat(taf_method_info_t *method, taf_scb_cubby_t *scb_taf)
{
	wlc_taf_info_t* taf_info = method->taf_info;
	taf_ias_uni_state_t* uni_state = taf_get_uni_state(taf_info);
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);
	int tid_index;

	for (tid_index = 0; tid_index < TAF_MAXPRIO; tid_index++) {
		taf_ias_sched_data_t* ias_sched;
		taf_source_type_t s_idx;
		uint32 total_pkt_count[TAF_NUM_LIST_TYPES];
		int tid = tid_service_order[tid_index];

		if ((ias_sched = TAF_IAS_TID_STATE(scb_taf, tid)) == NULL) {
			continue;
		}

		ias_sched->source_updated = 0;
		memset(&total_pkt_count, 0, sizeof(total_pkt_count));
		ias_sched->release_config[TAF_TYPE_DL].whole = 0;
#if TAF_ENABLE_UL
		ias_sched->release_config[TAF_TYPE_UL].whole = 0;

		taf_ias_decay_stats(&ias_sched->rel_stats, taf_ias_coeff_p(method),
			TAF_TYPE_UL, TAF_IAS_NOWTIME(method));

#endif
		ias_info->ncount_flow +=
			ias_sched->rel_stats.data.total_scaled_ncount >> TAF_PKTCOUNT_SCALE_SHIFT;

		/* this loops over AMPDU, NAR (and SQS, UL if used) */
		for (s_idx = TAF_FIRST_REAL_SOURCE; s_idx < TAF_NUM_SCHED_SOURCES; s_idx ++) {
			taf_sched_handle_t* handle = &ias_sched->handle[s_idx];
			void * srch;
			taf_list_type_t type;

			if (!TAF_IAS_SOURCE_ENABLED(ias_sched, s_idx)) {
				continue;
			}
			type = TAF_SRC_TO_TYPE(s_idx);

			if (TAF_PRIO_SUSPENDED(scb_taf, tid, type)) {
				WL_TAFM1(method, MACF" tid %u %s suspended\n",
					TAF_ETHERC(scb_taf), tid, TAF_SOURCE_NAME(s_idx));
				continue;
			}

			srch = *(taf_info->source_handle_p[s_idx]);

			if (taf_info->funcs[s_idx].update_fn) {
				bool updated =
					taf_info->funcs[s_idx].update_fn(srch, handle->scbh,
					handle->tidh, TAF_IAS_NOWTIME(method));

				if (updated) {
					ias_sched->source_updated |= (1 << s_idx);
#if defined(TAF_DBG) && TAF_ENABLE_UL
					if (TAF_SOURCE_IS_UL(s_idx)) {
						ias_sched->rel_stats.data.ul.update_count +=
							(1 << TAF_APKTCOUNT_SCALE_SHIFT);
					}
#endif
					WL_TAFM2(method, MACF"%s tid %u updated\n",
						TAF_ETHERC(scb_taf), TAF_TYPE(type), tid);
				}
			}
#if TAF_ENABLE_UL
			if (TAF_SOURCE_IS_UL(s_idx)) {
				ias_sched->rel_stats.data.pkt_size_mean[TAF_TYPE_UL] =
					taf_ias_rx_pkt_size(scb_taf, tid);
			}
#endif
			if ((scb_taf->info.traffic.count[s_idx][tid] =
				taf_info->funcs[s_idx].pktqlen_fn(srch, handle->scbh, handle->tidh,
					TAF_IAS_NOWTIME(method)))) {

				scb_taf->info.traffic.map[s_idx] |= (1 << tid);
				total_pkt_count[type] += scb_taf->info.traffic.count[s_idx][tid];

				WL_TAFM1(method, MACF" tid %u qlen %8s: %4u%s\n",
					TAF_ETHERC(scb_taf), tid, TAF_SOURCE_NAME(s_idx),
					scb_taf->info.traffic.count[s_idx][tid],
				        scb_taf->info.ps_mode ? " PS":"");
			}
		}
#if TAF_ENABLE_UL
		if (total_pkt_count[TAF_TYPE_UL] > 0) {

			TAF_SET_AGG(ias_sched->rx_aggn, taf_ias_rx_aggn(scb_taf, tid));
			TAF_SET_AGG(ias_sched->rx_max_aggn, taf_ias_rx_max_aggn(scb_taf, tid));

			scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid] =
				taf_ias_est_release_time(scb_taf, tid, ias_sched,
				total_pkt_count[TAF_TYPE_UL], TAF_RSPEC_UL, TAF_SOURCE_UL,
				TAF_TYPE_UL);
#ifdef TAF_DEBUG_VERBOSE
			if (ias_cfg(method).bodge > 0) {
				scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid] +=
					scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid] /
					ias_cfg(method).bodge;
			}
#endif

			uni_state->est_release_units +=
				scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid];

			scb_taf->info.traffic.available[TAF_TYPE_UL] |= (1 << tid);

			if (!scb_taf->info.ps_mode) {
				ias_cfg(method).active_mu_count[TAF_TECH_UL_OFDMA][tid]++;
			}

			WL_TAFM1(method, MACF"%s estimated UL %5u units, pkt size %u, aggn %u/%u, "
				"rel_score %4u%s\n",
				TAF_ETHERC(scb_taf), TAF_TYPE(TAF_TYPE_UL),
				scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid],
				ias_sched->rel_stats.data.pkt_size_mean[TAF_TYPE_UL],
				TAF_GET_AGG(ias_sched->rx_aggn),
				TAF_GET_AGG(ias_sched->rx_max_aggn),
				scb_taf->info.scb_stats.ias.data.relative_score[TAF_TYPE_UL],
				scb_taf->info.ps_mode ? " PS":"");
		}
#endif /* TAF_ENABLE_UL */

		if (total_pkt_count[TAF_TYPE_DL] > 0) {
			taf_rspec_index_t rindex =
				scb_taf->info.scb_stats.ias.data.ridx_used[TAF_TYPE_DL];

			if ((scb_taf->info.scb_stats.global.rdata.ratespec_mask &
					(1 << rindex)) == 0) {
				TAF_ASSERT(rindex != TAF_RSPEC_SU_DL);
				rindex = TAF_RSPEC_SU_DL;
				scb_taf->info.scb_stats.ias.data.ridx_used[TAF_TYPE_DL] =
					TAF_RSPEC_SU_DL;
			}

			TAF_SET_AGG(ias_sched->aggsf, taf_ias_aggsf(scb_taf, tid));

			scb_taf->info.traffic.est_units[TAF_TYPE_DL][tid] =
				taf_ias_est_release_time(scb_taf, tid, ias_sched,
				total_pkt_count[TAF_TYPE_DL], rindex, TAF_SOURCE_UNDEFINED,
				TAF_TYPE_DL);

			uni_state->est_release_units +=
				scb_taf->info.traffic.est_units[TAF_TYPE_DL][tid];

			scb_taf->info.traffic.available[TAF_TYPE_DL] |= (1 << tid);

#if TAF_ENABLE_MU_TX
			if (!scb_taf->info.ps_mode && scb_taf->info.tech_enable_mask &&
				(ias_sched->used & (1 << TAF_SOURCE_AMPDU))) {

				taf_tech_type_t tech = TAF_TECH_DL_HEMUMIMO;

				for (; tech <= TAF_TECH_DL_OFDMA; tech++) {
					if (scb_taf->info.mu_tech_en[tech] & (1 << tid)) {
						ias_cfg(method).active_mu_count[tech][tid]++;
					}
				}
			}
#endif
			WL_TAFM2(method, MACF"%s estimated %sU %5u units, pkt size %u, aggsf %u, "
				"rel_score %4u (in flight %3u)%s\n",
				TAF_ETHERC(scb_taf), TAF_TYPE(TAF_TYPE_DL),
				(rindex == TAF_RSPEC_SU_DL || rindex == TAF_RSPEC_SU_DL_INSTANT) ?
				"S" : "M", scb_taf->info.traffic.est_units[TAF_TYPE_DL][tid],
				ias_sched->rel_stats.data.pkt_size_mean[TAF_TYPE_DL],
				TAF_GET_AGG(ias_sched->aggsf),
				scb_taf->info.scb_stats.ias.data.relative_score[TAF_TYPE_DL],
				SCB_PKTS_INFLT_FIFOCNT_VAL(scb_taf->scb, tid),
				scb_taf->info.ps_mode ? " PS":"");
		}

		if ((scb_taf->info.traffic.available[TAF_TYPE_DL] & (1 << tid)) ||
#if TAF_ENABLE_UL
			(scb_taf->info.traffic.available[TAF_TYPE_UL] & (1 << tid))) {
#else
			FALSE) {
#endif
			TAF_DPSTATS_LOG_ADD(&ias_sched->rel_stats, ready, 1);
		} else {
			scb_taf->info.force &= ~(1 << tid);
		}
	}

	ias_cfg(method).sched_active |= scb_taf->info.traffic.available[TAF_TYPE_DL];
#if TAF_ENABLE_UL
	ias_cfg(method).sched_active |= scb_taf->info.traffic.available[TAF_TYPE_UL];
#endif
}

static uint32 BCMFASTPATH taf_ias_units_to_score(taf_method_info_t* method,
	taf_scb_cubby_t *scb_taf, int tid, taf_tech_type_t tech, uint32 units)
{
	uint8* score_weights = ias_cfg(method).score_weights;
	uint32 score = ((units * score_weights[tid]) + 128) >> 8;

#if TAF_ENABLE_MU_BOOST
	if (method->ordering == TAF_LIST_SCORE_MINIMUM && TAF_TECH_MASK_IS_MUMIMO(1 << tech)) {
		wlc_taf_info_t* taf_info = method->taf_info;

		if ((taf_info->mu_boost == TAF_MUBOOST_FACTOR ||
				taf_info->mu_boost == TAF_MUBOOST_RATE_FACTOR) &&
			(scb_taf->info.scb_stats.global.rdata.mu.clients_count > 0)) {

			uint8 boost_factor = taf_ias_boost_factor(scb_taf);

			if (boost_factor >= 1) {
				score /= boost_factor;
			} else {
				TAF_ASSERT(0);
			}
		}
	}
#endif /* TAF_ENABLE_MU_BOOST */

#if defined(WLATM_PERC)
	if (scb_taf->scb->sched_staperc) {
		/* If ATM percentages are applied, normalize the scores here */
		score = score / scb_taf->scb->sched_staperc;
	}
#endif /* WLATM_PERC */
	if (score == 0 && units > 0) {
		score = 1;
	}

	return score;
} /* taf_ias_units_to_score */

static bool BCMFASTPATH
taf_get_rate(wlc_taf_info_t *taf_info, taf_scb_cubby_t* scb_taf, taf_rspec_index_t rindex,
	taf_rate *result)
{
	wlc_info_t* wlc = TAF_WLCT(taf_info);
	struct scb * scb = scb_taf->scb;
	bool ret = TRUE;
#if TAF_ENABLE_MU_BOOST
	uint8 boost_factor;
#endif

	switch (rindex) {
		case TAF_RSPEC_SU_DL:
			result->rspec = wlc_scb_ratesel_get_primary(wlc, scb, NULL);
			result->rate = 0;
			TAF_ASSERT(result->rspec != 0);
			break;
		case TAF_RSPEC_SU_DL_INSTANT:
#if TAF_ENABLE_MU_TX
		case TAF_RSPEC_MU_DL_INSTANT:
#endif
			result->rspec = wlc_scb_ratesel_get_opstats(wlc->wrsi, scb, AC_BE,
				RTSTAT_GET_TXS | ((rindex == TAF_RSPEC_SU_DL_INSTANT ?
				RTSTAT_GET_TYPE_SU : RTSTAT_GET_TYPE_MU) << RTSTAT_GET_TYPE_SHIFT));
			result->rate = 0;
			break;

#if TAF_ENABLE_MU_BOOST
	case TAF_RSPEC_MU_DL_BOOST:
		result->rate = wlc_scb_ratesel_get_muboost(wlc->wrsi, scb, AC_BE,
			&scb_taf->info.scb_stats.global.rdata.mu.clients_count);
		result->rspec = 0;
		boost_factor = taf_ias_boost_factor(scb_taf);

		if (result->rate == 0 || boost_factor == 0 ||
			scb_taf->info.scb_stats.global.rdata.mu.clients_count == 0) {

			WL_ERROR(("wl%u %s: "MACF" null mu boost (%u: rate %u, clients %u, "
				"factor %u)\n", WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__,
				TAF_ETHERC(scb_taf), rindex, result->rate,
				scb_taf->info.scb_stats.global.rdata.mu.clients_count,
				boost_factor));
			return FALSE;
		}
		result->rate /= boost_factor;
		break;
#endif /* TAF_ENABLE_MU_BOOST */

#if TAF_ENABLE_UL
		case TAF_RSPEC_UL:
			result->rspec = wlc_scb_ratesel_get_ulrt_rspec(wlc->wrsi, scb, 0);
			result->rate = 0;
			if (result->rspec == ULMU_RSPEC_INVD) {
				result->rspec = wlc_scb_ratesel_get_primary(wlc, scb, NULL);
				TAF_ASSERT(result->rspec != 0);
				break;
			}
			break;
#endif /* TAF_ENABLE_UL */
		default:
			ret = FALSE;
			result->rspec = 0;
			result->rate = 0;
			TAF_ASSERT(0);
	}

#if TAF_ENABLE_MU_BOOST
	if (rindex == TAF_RSPEC_MU_DL_BOOST) {
		WL_TAFT2(taf_info, MACF" boost rate is %u (type %u), mu clients %u\n",
			TAF_ETHERS(scb), result->rate, rindex,
			scb_taf->info.scb_stats.global.rdata.mu.clients_count);
	} else
#endif
	{
		WL_TAFT4(taf_info, MACF" rspec is 0x%8x (type %u)\n", TAF_ETHERS(scb),
			result->rspec, rindex);
	}

	return ret;
}

static INLINE void BCMFASTPATH taf_ias_pkt_overhead(wlc_taf_info_t *taf_info,
	taf_scb_cubby_t* scb_taf, taf_scheduler_scb_stats_t* scb_stats, taf_rspec_index_t rindex)
{
	taf_method_info_t* method = scb_taf->method;

	scb_stats->ias.data.pkt_rate[rindex] = (scb_stats->ias.data.byte_rate[rindex] *
		wlc_airtime_dot11hdrsize(scb_taf->scb->wsec));

#if TAF_ENABLE_NAR
	if (rindex == TAF_RSPEC_SU_DL && scb_taf->info.scb_stats.ias.data.use[TAF_SOURCE_NAR]) {

		scb_stats->ias.data.nar_byte_rate = scb_stats->ias.data.byte_rate[rindex];

		if (TAF_OPT(method, PKT_NAR_OVERHEAD)) {
			scb_stats->ias.data.nar_pkt_rate =
				TAF_MICROSEC_TO_UNITS(TAF_NAR_OVERHEAD) << TAF_PKTBYTES_COEFF_BITS;

			WL_TAFM2(method, MACF" %s overhead %uus\n", TAF_ETHERC(scb_taf),
				TAF_SOURCE_NAME(TAF_SOURCE_NAR), TAF_NAR_OVERHEAD);
		} else {
			scb_stats->ias.data.nar_pkt_rate = scb_stats->ias.data.pkt_rate[rindex];
		}
	}
#endif /* TAF_ENABLE_NAR */
	if (TAF_OPT(method, PKT_AMPDU_OVERHEAD) && rindex == TAF_RSPEC_SU_DL &&
		scb_stats->ias.data.use[TAF_SOURCE_AMPDU]) {

		WL_TAFT2(taf_info, MACF" %s with rts %uus, no rts %uus, rts %uus\n",
			TAF_ETHERC(scb_taf), TAF_SOURCE_NAME(TAF_SOURCE_AMPDU),
			TAF_AMPDU_RTS_OVERHEAD,
			TAF_AMPDU_NO_RTS_OVERHEAD,
			TAF_RTS_OVERHEAD);

		scb_stats->ias.data.overhead_with_rts =
				TAF_MICROSEC_TO_UNITS(TAF_AMPDU_RTS_OVERHEAD);

		scb_stats->ias.data.overhead_without_rts =
				TAF_MICROSEC_TO_UNITS(TAF_AMPDU_NO_RTS_OVERHEAD);

		scb_stats->ias.data.overhead_rts =
				TAF_MICROSEC_TO_UNITS(TAF_RTS_OVERHEAD);
	}
}

static INLINE void BCMFASTPATH
taf_ias_rate_to_units(wlc_taf_info_t *taf_info, taf_scb_cubby_t* scb_taf, uint32 tech)
{
	taf_rspec_index_t rindex;
	taf_rate rate = {0, 0};
	taf_scheduler_scb_stats_t* scb_stats = &scb_taf->info.scb_stats;
	uint32 rate_sel = taf_info->use_sampled_rate_sel;
#if TAF_ENABLE_MU_BOOST
	uint8 boost = taf_info->mu_boost;
	bool boost_rate = (boost == TAF_MUBOOST_RATE || boost == TAF_MUBOOST_RATE_FACTOR);
#endif

#if TAF_ENABLE_MU_TX
	rate_sel &= (taf_info->mu | TAF_TECH_DL_SU_MASK);
#endif
	rate_sel |= TAF_TECH_UL_OFDMA_MASK;
	tech &= rate_sel;

	scb_stats->global.rdata.ratespec_mask = 0;

	for (rindex = TAF_RSPEC_SU_DL; rindex < NUM_TAF_RATES; rindex++) {
		if (rindex == TAF_RSPEC_SU_DL_INSTANT && ((tech & TAF_TECH_DL_SU_MASK) == 0)) {
			continue;
		}

		if (TAF_RIDX_IS_UL(rindex) && !TAF_TECH_MASK_IS_UL(tech)) {
			continue;
		}

		if (TAF_RIDX_IS_MUMIMO(rindex) && !TAF_TECH_MASK_IS_MUMIMO(tech)) {
			continue;
		}
#if TAF_ENABLE_MU_BOOST
		if (TAF_RIDX_IS_BOOST(rindex) && !boost_rate) {
			continue;
		}
#endif
		if (!taf_get_rate(taf_info, scb_taf, rindex, &rate)) {
			continue;
		}

		if (!TAF_RIDX_IS_BOOST(rindex) && rate.rspec == 0) {
			WL_TAFT2(taf_info, "ridx %u rspec is 0, use SU\n", rindex);
			TAF_ASSERT(rindex > TAF_RSPEC_SU_DL);
			rate.rspec = scb_stats->global.rdata.rate[TAF_RSPEC_SU_DL].rspec;
		}

		scb_stats->global.rdata.ratespec_mask |= (1 << rindex);

		WL_TAFT4(taf_info, MACF" ratespec_mask 0x%x\n", TAF_ETHERC(scb_taf),
			scb_stats->global.rdata.ratespec_mask);

		if ((!TAF_RIDX_IS_BOOST(rindex) &&
			scb_stats->global.rdata.rate[rindex].rspec != rate.rspec) ||
			(TAF_RIDX_IS_BOOST(rindex) &&
			scb_stats->global.rdata.rate[rindex].rate != rate.rate)) {

			uint32 byte_rate;

			WL_TAFT3(taf_info, MACF" updating ridx %d\n", TAF_ETHERC(scb_taf), rindex);

			if (rindex == TAF_RSPEC_SU_DL) {
				uint32 max_bw = wlc_ratespec_bw(rate.rspec);

				int idx = D11_REV128_BW_20MHZ;

				switch (max_bw) {
					case 20: break;
					case 40: idx = D11_REV128_BW_40MHZ; break;
					case 80: idx = D11_REV128_BW_80MHZ; break;
					case 160: idx = D11_REV128_BW_160MHZ; break;
					default:
						WL_ERROR(("wl%u %s: "MACF" invalid bw %u (0x%x)\n",
							WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__,
							TAF_ETHERC(scb_taf), max_bw, rate.rspec));
						TAF_ASSERT(0);
				}
				scb_stats->global.rdata.bw_idx = idx;
				scb_stats->global.rdata.max_nss = wlc_ratespec_nss(rate.rspec);
#ifdef TAF_DBG
				scb_stats->global.rdata.mcs = wlc_ratespec_mcs(rate.rspec);
				scb_stats->global.rdata.encode =
					(rate.rspec & WL_RSPEC_ENCODING_MASK) >>
					WL_RSPEC_ENCODING_SHIFT;
#endif
			}

#if TAF_ENABLE_MU_TX
			if (rindex == TAF_RSPEC_MU_DL_INSTANT) {
#ifdef TAF_DBG
				uint8 prev_nss = scb_stats->global.rdata.mu.nss;

				scb_stats->global.rdata.mu.mcs = wlc_ratespec_mcs(rate.rspec);
				scb_stats->global.rdata.mu.encode =
					(rate.rspec & WL_RSPEC_ENCODING_MASK) >>
					WL_RSPEC_ENCODING_SHIFT;

				if (prev_nss && prev_nss != scb_stats->global.rdata.mu.nss) {
					WL_TAFT2(taf_info, MACF" (%u) NSS switch ! %u --> %u\n",
						TAF_ETHERC(scb_taf), rindex,
						prev_nss, scb_stats->global.rdata.mu.nss);
				}
#endif
				scb_stats->global.rdata.mu.nss = wlc_ratespec_nss(rate.rspec);
			}
#endif /* TAF_ENABLE_MU_TX */
			scb_stats->global.rdata.rate[rindex].rspec = rate.rspec;

			if (rate.rspec != 0 && rate.rate == 0) {
				scb_stats->global.rdata.rate[rindex].rate =
					wf_rspec_to_rate(rate.rspec);
			} else if (TAF_RIDX_IS_BOOST(rindex)) {
				scb_stats->global.rdata.rate[rindex].rate = rate.rate;
			}

			if (scb_stats->global.rdata.rate[rindex].rate == 0) {
				WL_ERROR(("wl%u %s: "MACF" zero rate (type %u, rspec 0x%x, "
					"rate %u)\n", WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__,
					TAF_ETHERC(scb_taf), rindex, rate.rspec, rate.rate));
			}
			TAF_ASSERT(scb_stats->global.rdata.rate[rindex].rate != 0);

			byte_rate =  wlc_airtime_payload_time_us_rate(0,
				scb_stats->global.rdata.rate[rindex].rate,
				TAF_MICROSEC_TO_UNITS(TAF_PKTBYTES_COEFF));

			scb_stats->ias.data.byte_rate[rindex] = byte_rate;

			if (scb_stats->ias.data.byte_rate[rindex] == 0) {
				WL_ERROR(("wl%u %s: "MACF" zero byte rate (type %u, rate %u, "
					"rspec 0x%x)\n",
					WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__,
					TAF_ETHERC(scb_taf), rindex,
					scb_stats->global.rdata.rate[rindex].rate, rate.rspec));
				TAF_ASSERT(scb_stats->ias.data.byte_rate[rindex]);
			}
			taf_ias_pkt_overhead(taf_info, scb_taf, scb_stats, rindex);
		}
	}
}

static void taf_ias_flush_rspec(taf_scb_cubby_t *scb_taf)
{
	taf_scheduler_scb_stats_t* scb_stats = &scb_taf->info.scb_stats;
	wlc_taf_info_t* taf_info = scb_taf->method->taf_info;

	memset(&scb_stats->global.rdata, 0, sizeof(scb_stats->global.rdata));

	if (scb_taf->info.tid_enabled != 0 && wlc_taf_scheduler_blocked(taf_info)) {
		/* normally rate info is re-filled at start of scheduler cycle;
		 * in case rate info is flushed mid-cycle, it needs to be specifically
		 * refreshed now as it may be required for use
		 */
		taf_ias_rate_to_units(taf_info, scb_taf,
			scb_taf->info.tech_enable_mask | TAF_TECH_DL_SU_MASK);
	}
}

static void taf_ias_clean_all_rspecs(taf_method_info_t *method)
{
	taf_list_t* list = taf_ias_get_list_head(method);

	ias_cfg(method).min_mpdu_dur[TAF_TYPE_DL] =
		TAF_MPDU_DENS_TO_UNITS(method->taf_info->mpdu_dens[TAF_TYPE_DL]);
#if TAF_ENABLE_UL
	ias_cfg(method).min_mpdu_dur[TAF_TYPE_UL] =
		TAF_MPDU_DENS_TO_UNITS(method->taf_info->mpdu_dens[TAF_TYPE_UL]);
#endif
	while (list) {
		taf_scb_cubby_t *scb_taf = list->scb_taf;

		taf_ias_flush_rspec(scb_taf);
		list = list->next;
	}
}

static INLINE void BCMFASTPATH taf_ias_upd_item(taf_method_info_t *method, taf_scb_cubby_t* scb_taf)
{
	wlc_taf_info_t *taf_info = method->taf_info;
	taf_list_type_t type;

	/* Assume rate info is the same throughout all scheduling interval. */
	taf_ias_rate_to_units(taf_info, scb_taf,
		scb_taf->info.tech_enable_mask | TAF_TECH_DL_SU_MASK);

	for (type = TAF_TYPE_DL; type < TAF_NUM_LIST_TYPES; type++) {
		if (ias_cfg(method).total_score) { /* avoid divide by 0 */
			scb_taf->info.scb_stats.ias.data.relative_score[type] =
				(TAF_TYPE_SCORE(scb_taf, type) << TAF_COEFF_IAS_MAX_BITS) /
				ias_cfg(method).total_score;
		} else {
			scb_taf->info.scb_stats.ias.data.relative_score[type] = 0;
		}
	}

	memset(&scb_taf->info.traffic, 0, sizeof(scb_taf->info.traffic));
	memset(scb_taf->info.tech_type, TAF_TECH_UNASSIGNED, sizeof(scb_taf->info.tech_type));
#if TAF_ENABLE_SQS_PULL
	scb_taf->info.pkt_pull_dequeue = 0;
#endif
}

static void taf_ias_set_coeff(uint32 coeff, taf_ias_coeff_t* decay_coeff)
{
	uint32 index = 1;
	uint32 value = coeff << 6;
	uint64 normalise;

	decay_coeff->coeff1 = coeff;

	while (index < 10) {
		index++;
		value *= coeff;
		value >>= TAF_COEFF_IAS_MAX_BITS;
	}
	coeff = (value + (1 << 5)) >> 6;
	decay_coeff->coeff10 = coeff;

	while (index < 100) {
		index += 10;
		value *= coeff;
		value >>= TAF_COEFF_IAS_MAX_BITS;
	}
	coeff = (value + (1 << 5)) >> 6;
	decay_coeff->coeff100 = coeff;

	/* use Pade approximation to calculate (1 << time_shift) / ln(TAF_COEFF_IAS_MAX / coeff) */
	coeff = decay_coeff->coeff1;
	value = TAF_COEFF_IAS_MAX - coeff;

	normalise = ((6 * (uint64)coeff) + (4 * (uint64)value));
	normalise *= (uint64)(coeff) * (uint64)(1 << decay_coeff->time_shift);

	value = (6 * coeff + value) * value;

	decay_coeff->normalise = taf_div64(normalise, value);
}

static INLINE uint32 BCMFASTPATH
taf_ias_decay_score(uint32 initial, taf_ias_coeff_t* decay_coeff, uint32 elapsed, int32* correct)
{
	/* The decay coeff, is how much fractional reduction to occur per time interval
	 * of elapsed time. This is an exponential model.
	 */
	uint32 value = initial;
	uint32 counter = elapsed >> decay_coeff->time_shift;

	if (correct) {
		*correct = (int32)(elapsed) - (int32)(counter << decay_coeff->time_shift);
	}
	if (counter > 1000) {
		/* if it is a long time, just reset the score */
		value = 0;
	} else {
		while (value && (counter >= 100)) {
			value = value * decay_coeff->coeff100;
			value >>= TAF_COEFF_IAS_MAX_BITS; /* normalise coeff */
			counter -= 100;
		}
		while (value && (counter >= 10)) {
			value = value * decay_coeff->coeff10;
			value >>= TAF_COEFF_IAS_MAX_BITS; /* normalise coeff */
			counter -= 10;
		}
		while (value && counter) {
			value = value * decay_coeff->coeff1;
			value >>= TAF_COEFF_IAS_MAX_BITS; /* normalise coeff */
			counter--;
		}
	}
	return value;
}

static INLINE void BCMFASTPATH taf_ias_pre_update_list(taf_method_info_t *method)
{
	taf_list_t *item;
	uint32 total_score = 0;
	uint32 now_time = TAF_IAS_NOWTIME(method);

	TAF_ASSERT(method->active_list == NULL);

	for (item = method->list; item; item = item->next) {
		int32  time_correction = 0;
		uint32 elapsed;
		uint32 score;
		taf_scb_cubby_t* scb_taf = item->scb_taf;

#if TAF_ENABLE_SQS_PULL
		scb_taf->pscore = 0;
#endif
		if (SCB_MARKED_DEL(scb_taf->scb) || SCB_DEL_IN_PROGRESS(scb_taf->scb)) {
			scb_taf->info.force = 0;
			/* max score means least priority for IAS */
			TAF_TYPE_SCORE(scb_taf, item->type) = TAF_SCORE_MAX;
			WL_TAFM1(method, MACF"%s will be deleted\n", TAF_ETHERC(scb_taf),
				TAF_TYPE(item->type));
			continue;
		}
		if (scb_taf->info.tid_enabled == 0) {
			WL_TAFM4(method, MACF"%s not active\n", TAF_ETHERC(scb_taf),
				TAF_TYPE(item->type));
			continue;
		}

		switch (method->type) {
			case TAF_EBOS:
				break;

			case TAF_ATOS:
			case TAF_ATOS2:
				elapsed = now_time -
					scb_taf->info.scb_stats.ias.data.dcaytimestamp[item->type];
				score = taf_ias_decay_score(TAF_TYPE_SCORE(scb_taf, item->type),
					taf_ias_coeff_p(method), elapsed, &time_correction);
				if ((score != TAF_TYPE_SCORE(scb_taf, item->type)) ||
					(score == 0)) {
					TAF_TYPE_SCORE(scb_taf, item->type) = score;
					scb_taf->info.scb_stats.ias.data.dcaytimestamp[item->type] =
						now_time - time_correction;
				}
				WL_TAFM4(method, MACF" type %d score %d\n",
					TAF_ETHERC(scb_taf), item->type,
					TAF_TYPE_SCORE(scb_taf, item->type));
				break;
			default:
				TAF_ASSERT(0);
				break;
		}
		total_score += TAF_TYPE_SCORE(scb_taf, item->type);
	}
	ias_cfg(method).total_score = total_score;
}

static INLINE void BCMFASTPATH taf_ias_post_update_list(taf_method_info_t *method)
{
	taf_list_t *item;

	TAF_ASSERT(method->active_list == NULL);

#if TAF_ENABLE_MU_TX
	memset(&ias_cfg(method).active_mu_count, 0, sizeof(ias_cfg(method).active_mu_count));
#endif
	for (item = method->list; item; item = item->next) {
		taf_scb_cubby_t* scb_taf = item->scb_taf;

		if ((scb_taf->info.tid_enabled == 0) || SCB_MARKED_DEL(scb_taf->scb) ||
			SCB_DEL_IN_PROGRESS(scb_taf->scb) || (item->type != TAF_TYPE_DL)) {
			continue;
		}
		taf_ias_upd_item(method, scb_taf);
		taf_ias_upd_item_stat(method, scb_taf);
	}
}

#if TAF_ENABLE_TIMER
static void taf_ias_agghold_stat(wlc_taf_info_t * taf_info, taf_ias_group_info_t* ias_info,
	uint32 time)
{
	uint32 elapsed = 0;

	if  (ias_info->agg_hold_start_time != 0) {

		elapsed = time - ias_info->agg_hold_start_time;
		if (ias_info->stall_prevent_timer) {
			++ias_info->debug.agg_retrigger_count;
		} else {
			++ias_info->debug.agg_hold_count;
		}
		ias_info->agg_hold_start_time = 0;

		WL_TAFT2(taf_info, "%s ended, %uus elapsed\n",
			ias_info->stall_prevent_timer ? "retrigger" : "aggregation hold",
			elapsed);
	} else {
		TAF_ASSERT(0);
	}

	if (!ias_info->stall_prevent_timer && elapsed > ias_info->debug.agg_hold_max_time) {
		ias_info->debug.agg_hold_max_time = elapsed;
		WL_TAFT4(taf_info, "new max hold %u\n", ias_info->debug.agg_hold_max_time);
	}
	if (!ias_info->stall_prevent_timer) {
		ias_info->debug.agg_hold_total_time += (uint64)elapsed;
	}
}

static void taf_ias_aggh_tmr_exp(void *arg)
{
	wlc_taf_info_t * taf_info = (wlc_taf_info_t *) arg;
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);

	TAF_ASSERT(ias_info);

	ias_info->is_agg_hold_timer_running = FALSE;
	ias_info->agg_hold_expired = TRUE;
#if !TAF_ENABLE_SQS_PULL
	ias_info->agg_hold_exit_pending = FALSE;
#endif
	ias_info->now_time = taf_timestamp(TAF_WLCT(taf_info));

	WL_TAFT2(taf_info, "%s timer expired\n",
		ias_info->stall_prevent_timer ? "retrigger" : "aggregation hold");
	taf_ias_agghold_stat(taf_info, ias_info, ias_info->now_time);

	ias_info->stall_prevent_timer = FALSE;

	if (!wlc_taf_scheduler_blocked(taf_info)) {
#if TAF_ENABLE_SQS_PULL
		ias_info->agg_hold_exit_pending = FALSE;
#endif
		wlc_taf_schedule(taf_info, TAF_DEFAULT_UNIFIED_TID, NULL, FALSE);
	}
}

static int taf_ias_agghold_exp(taf_ias_group_info_t* ias_info)
{
	return ias_info->agg_hold_expired;
}

static int taf_ias_agghold_start(wlc_taf_info_t * taf_info, int ms, uint32 time)
{
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);

	if (ias_info->agg_hold_prevent) {
		return BCME_NOTREADY;
	}
	if (ias_info->is_agg_hold_timer_running) {
		return BCME_BUSY;
	}
	TAF_ASSERT(ias_info->agg_hold_start_time == 0);
	wl_add_timer(TAF_WLCT(taf_info)->wl, ias_info->agg_hold_timer, ms, FALSE);

	ias_info->is_agg_hold_timer_running = TRUE;
	ias_info->agg_hold_exit_pending = TRUE;
	ias_info->agg_hold_start_time = time;
	ias_info->stall_prevent_timer = FALSE;

	WL_TAFT2(taf_info, "agg hold timer started %ums\n", ms);
	return BCME_OK;
}

static bool taf_ias_agghold_stop(wlc_taf_info_t * taf_info, taf_ias_group_info_t* ias_info,
	uint32 time)
{
	if (ias_info->is_agg_hold_timer_running) {
		if (wl_del_timer(TAF_WLCT(taf_info)->wl, ias_info->agg_hold_timer)) {
			taf_ias_agghold_stat(taf_info, ias_info, time);

			ias_info->is_agg_hold_timer_running = FALSE;
		}
	}
	ias_info->agg_hold_exit_pending = FALSE;
	WL_TAFT3(taf_info, "%u\n", ias_info->is_agg_hold_timer_running);

	ias_info->stall_prevent_timer = FALSE;

	return !ias_info->is_agg_hold_timer_running;
}

static INLINE bool taf_ias_agghold_is_active(taf_ias_group_info_t* ias_info)
{
	TAF_ASSERT(!ias_info->agg_hold_expired || !ias_info->is_agg_hold_timer_running);
	return ias_info->is_agg_hold_timer_running || ias_info->agg_hold_exit_pending;
}

static INLINE bool taf_ias_agghold_was_active(taf_ias_group_info_t* ias_info)
{
	return ias_info->agg_hold_expired;
}

static INLINE void taf_ias_agghold_clear(taf_ias_group_info_t* ias_info)
{
	ias_info->agg_hold_expired = FALSE;
	ias_info->agg_hold_prevent = FALSE;
	ias_info->stall_prevent_timer = FALSE;
}

static INLINE void taf_ias_agghold_prevent(taf_ias_group_info_t* ias_info)
{
	ias_info->agg_hold_prevent = TRUE;
}

static INLINE bool taf_ias_agghold_reset(taf_method_info_t * method)
{
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
	taf_ias_agghold_clear(ias_info);
	return taf_ias_agghold_stop(method->taf_info, ias_info, TAF_IAS_NOWTIME(method));
}

static int taf_ias_retrig_start(wlc_taf_info_t * taf_info, uint32 time)
{
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);
	uint32 ms = ias_info->retrigger_timer[TAF_TYPE_DL];

	if (ias_info->is_agg_hold_timer_running) {
		return BCME_BUSY;
	}

	TAF_ASSERT(!ias_info->agg_hold_prevent);
	TAF_ASSERT(ias_info->agg_hold_start_time == 0);
	TAF_ASSERT(!ias_info->agg_hold_exit_pending);

#if TAF_ENABLE_UL
	if (wlc_taf_uladmit_count(taf_info, TRUE) > 0) {
		ms = ias_info->retrigger_timer[TAF_TYPE_UL];
	}
#endif
	wl_add_timer(TAF_WLCT(taf_info)->wl, ias_info->agg_hold_timer, ms, FALSE);

	ias_info->is_agg_hold_timer_running = TRUE;
	ias_info->agg_hold_start_time = time;
	ias_info->stall_prevent_timer = TRUE;

	WL_TAFT2(taf_info, "retrigger timer started %ums\n", ms);
	return BCME_OK;
}

static INLINE bool taf_ias_retrig_is_active(taf_ias_group_info_t* ias_info)
{
	TAF_ASSERT(!ias_info->agg_hold_expired || !ias_info->is_agg_hold_timer_running);
	return ias_info->is_agg_hold_timer_running && ias_info->stall_prevent_timer;
}

static bool taf_ias_retrig_stop(wlc_taf_info_t * taf_info, taf_ias_group_info_t* ias_info,
	uint32 time)
{
	return taf_ias_agghold_stop(taf_info, ias_info, time);
}
#endif /* TAF_ENABLE_TIMER */

static void taf_ias_up(void * context)
{
	taf_method_info_t *method = (taf_method_info_t *) context;

	BCM_REFERENCE(method);
}

static int taf_ias_down(void * context)
{
	taf_method_info_t *method = (taf_method_info_t *) context;
	int callbacks = 0;

#if TAF_ENABLE_TIMER
	if (!taf_ias_agghold_reset(method)) {
		callbacks++;
	}
#else
	BCM_REFERENCE(method);
#endif /* TAF_ENABLE_TIMER */
	return callbacks;
}

#if TAF_ENABLE_MU_TX

static INLINE bool BCMFASTPATH taf_ias_fifo_overlap(taf_method_info_t* method,
	taf_scb_cubby_t **scb_taf_p, taf_scb_cubby_t *scb_taf_test, uint8 count, int tid)
{
	uint32 ac = WME_PRIO2AC(tid);
	uint32 test_fifo = FIFO_INDEX_GET_AC(scb_taf_test->scb, ac);

	while (count--) {
		TAF_ASSERT(*scb_taf_p);
		if (FIFO_INDEX_GET_AC((*scb_taf_p)->scb, ac) == test_fifo) {
			WL_TAFM2(method, MACF" tid/ac %u/%u fifo overlap (%u) with "MACF"\n",
				TAF_ETHERC(scb_taf_test), tid, ac, test_fifo,
				TAF_ETHERC(*scb_taf_p));
			return TRUE;
		}
		++scb_taf_p;
	}
	WL_TAFM4(method, MACF" tid/ac %u/%u fifo %u not overlapped\n",
		TAF_ETHERC(scb_taf_test), tid, ac, test_fifo);
	return FALSE;
}

static bool BCMFASTPATH taf_ias_bridge_list(taf_method_info_t* method, taf_list_t* paired,
	taf_list_t* bridge, int tid)
{
	taf_list_t* insert_next = paired;
	taf_list_t* bridge_next;

	/* need to move all unassigned bridged list items to come after new bridge paired item */
	while (bridge && bridge != paired) {
		if (bridge->scb_taf->info.tech_type[bridge->type][tid] != TAF_TECH_UNASSIGNED) {
			WL_TAFM2(method, "cannot bridge "MACF"%s tid %u %s\n",
				TAF_ETHERC(bridge->scb_taf),
				TAF_TYPE(bridge->type), tid,
				TAF_TECH_NAME(bridge->scb_taf->info.tech_type[bridge->type][tid]));
			return FALSE;
		}
		bridge_next = bridge->next;

		WL_TAFM2(method, "bridging inter "MACF"%s\n",
			TAF_ETHERC(bridge->scb_taf),
			TAF_TYPE(bridge->type));
		wlc_taf_list_remove(&method->list, bridge);
		wlc_taf_list_insert(insert_next, bridge);
		bridge->bridged |= (1 << tid);
		insert_next = bridge;
		bridge = bridge_next;
	}
	return TRUE;
}

static INLINE uint32 BCMFASTPATH taf_ias_max_bridge_score(taf_method_info_t* method,
	taf_scb_cubby_t* scb_taf, taf_list_type_t type, int mu_user_count)
{
	uint32 pair_limit = ias_cfg(method).mu_pair[mu_user_count];

	/* enforce tighter pairing if we release out of list order */
	pair_limit = (3 * pair_limit) / 4;
	pair_limit += TAF_COEFF_IAS_MAX;
	return (TAF_SCORE(scb_taf, type) * pair_limit) >> TAF_COEFF_IAS_MAX_BITS;
}

static bool BCMFASTPATH taf_ias_mu_pair(taf_method_info_t* method, taf_scb_cubby_t* scb_taf,
	taf_scb_cubby_t* scb_taf_next, taf_list_type_t type, int mu_user_count, bool bridge)
{
	uint32 pair_limit = ias_cfg(method).mu_pair[mu_user_count];
	uint32 diff;
	uint32 pro_limit;

	if (bridge) {
		/* enforce tighter pairing if we release out of list order */
		pair_limit = (3 * pair_limit) / 4;
	}

	pro_limit = (TAF_SCORE(scb_taf, type) * pair_limit) >> TAF_COEFF_IAS_MAX_BITS;

	if (pro_limit == 0) {
		pro_limit = pair_limit / 4;
	}
	diff = TAF_SCORE(scb_taf_next, type) - TAF_SCORE(scb_taf, type);

	if (diff > pro_limit) {
		WL_TAFM2(method, "score diff %u (%u/%u) "MACF"%s is too large "
			"(limit %u/%u)%s\n", diff,
			TAF_SCORE(scb_taf_next, type), TAF_SCORE(scb_taf, type),
			TAF_ETHERC(scb_taf_next), TAF_TYPE(type),
			pro_limit, pair_limit,
			bridge ? " (bridge)" : "");
		return FALSE;
	}
	return TRUE;
}

static uint32 BCMFASTPATH taf_ias_tech_alloc(taf_method_info_t* method, taf_scb_cubby_t **scb_taf_p,
	uint8 mu_user_count, uint32 rel_limit_units, taf_tech_type_t tech_idx, uint16 pairing_id,
	bool force, int tid, taf_list_type_t type)
{
	int index;
	uint32 total_mu_traffic = 0;

	TAF_ASSERT(rel_limit_units <= TAF_IAS_MAX_UNITS);
	TAF_ASSERT(pairing_id <= TAF_IAS_MAX_PAIRING);
	TAF_ASSERT(mu_user_count >= 1 && mu_user_count <= TAF_IAS_MAX_MU);

	for (index = 0; index < mu_user_count; index++) {
		taf_scb_cubby_t * scb_taf = *scb_taf_p++;

		scb_taf->info.tech_type[type][tid] = tech_idx;

		if (tech_idx < TAF_NUM_TECH_TYPES) {
			taf_ias_sched_data_t* ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);

			ias_sched->release_config[type].units = rel_limit_units;
			ias_sched->release_config[type].pairing_id = pairing_id;
			ias_sched->release_config[type].force = force ? 1 : 0;
			TAF_BASE1_SET(ias_sched->release_config[type].mu, mu_user_count);

			if (force) {
				total_mu_traffic += rel_limit_units;
			} else {
				total_mu_traffic += MIN(rel_limit_units,
					scb_taf->info.traffic.est_units[type][tid]);
			}

			WL_TAFM2(method, "set rel limit %u to "MACF"%s tid %u %s (pairing %u)\n",
				ias_sched->release_config[type].units, TAF_ETHERC(scb_taf),
				TAF_TYPE(type), tid, TAF_TECH_NAME(tech_idx), pairing_id);
		}
	}
	return total_mu_traffic;
}

static uint32 BCMFASTPATH taf_ias_check_upd_rate(taf_scb_cubby_t *scb_taf,
	taf_tech_type_t tech_idx, int tid, taf_ias_uni_state_t *uni_state, taf_list_type_t type)
{
	taf_rspec_index_t r_idx = taf_ias_get_rindex(scb_taf, type, tech_idx);

	if (r_idx != scb_taf->info.scb_stats.ias.data.ridx_used[type] &&
			scb_taf->info.scb_stats.global.rdata.ratespec_mask & (1 << r_idx)) {

		if (uni_state->est_release_units > scb_taf->info.traffic.est_units[type][tid]) {
			uni_state->est_release_units  -= scb_taf->info.traffic.est_units[type][tid];
		} else {
			uni_state->est_release_units = 0;
		}
		scb_taf->info.traffic.est_units[type][tid] =
			taf_ias_get_new_traffic_estimate(scb_taf, tid, r_idx, type);

		uni_state->est_release_units += scb_taf->info.traffic.est_units[type][tid];
		scb_taf->info.scb_stats.ias.data.ridx_used[type] = r_idx;
	}
	return scb_taf->info.traffic.est_units[type][tid];
}

static INLINE bool BCMFASTPATH taf_ias_dl_ofdma_suitable(taf_scb_cubby_t *scb_taf, int tid,
	taf_tech_type_t tech_idx)
{
	if ((scb_taf->info.mu_tech_en[tech_idx] & scb_taf->info.tid_enabled & (1 << tid)) == 0) {
		return FALSE;
	}
	if (scb_taf->info.ps_mode) {
		return FALSE;
	}
	if (scb_taf->info.tech_type[TAF_TYPE_DL][tid] < TAF_TECH_UNASSIGNED) {
		return FALSE;
	}
	if (SCB_MARKED_DEL(scb_taf->scb) || SCB_DEL_IN_PROGRESS(scb_taf->scb)) {
		return FALSE;
	}
#if TAF_ENABLE_PSQ_PULL && TAF_ENABLE_SQS_PULL
	/* for SQS platform, don't MU pair stations having PSQ packets available */
	if (scb_taf->info.traffic.count[TAF_SOURCE_PSQ][tid] != 0) {
		return FALSE;
	}
#endif
	if (!scb_taf->info.scb_stats.ias.data.use[TAF_SOURCE_AMPDU] ||
		scb_taf->info.linkstate[TAF_SOURCE_AMPDU][tid] != TAF_LINKSTATE_ACTIVE) {
		return FALSE;
	}
	if (scb_taf->info.traffic.est_units[TAF_TYPE_DL][tid] == 0) {
		return FALSE;
	}
	if (RSPEC_ISCCK(scb_taf->info.scb_stats.global.rdata.rate[TAF_RSPEC_SU_DL].rspec)) {
		return FALSE;
	}
	if (scb_taf->method->taf_info->dlofdma_maxn[taf_bw_idx(scb_taf)] == 0) {
		return FALSE;
	}
	return TRUE;
}

static INLINE bool BCMFASTPATH taf_ias_dl_ofdma(taf_method_info_t* method, taf_tech_type_t tech_idx,
	taf_list_t *list, taf_ias_uni_state_t *uni_state, int tid)
{
	wlc_taf_info_t* taf_info = method->taf_info;
	taf_scb_cubby_t *items[TAF_MAX_MU_OFDMA];
	taf_scb_cubby_t *scb_taf = list->scb_taf;
	taf_scb_cubby_t *scb_taf_next = NULL;
	uint32 mu_user_count = 1;
	uint32 min_mu_traffic;
	uint32 time_limit_units;
	uint32 user_time_limit_units;
	uint32 total_mu_traffic = 0;
	int bw_idx = taf_bw_idx(scb_taf);
	taf_list_t* bridge = NULL;
	bool inter_traffic = FALSE;
	uint32 admit_count = ias_cfg(method).active_mu_count[tech_idx][tid];

	if (list->type != TAF_TYPE_DL) {
		return FALSE;
	}

	TAF_ASSERT(tech_idx == TAF_TECH_DL_OFDMA);
	TAF_ASSERT(ias_cfg(method).mu_user_rel_limit[TAF_TECH_DL_OFDMA]);

	time_limit_units = taf_ias_window_remain(method, uni_state, TAF_TYPE_DL);

	if (time_limit_units == 0) {
		return FALSE;
	}

	if (!taf_ias_dl_ofdma_suitable(scb_taf, tid, TAF_TECH_DL_OFDMA)) {
		return FALSE;
	}

	items[0] = scb_taf;

	WL_TAFM2(method, "this %s item is "MACF"%s having %u units tid %u and score %u, "
		"rel score %u, bw_idx %u (opt %u)\n",
		TAF_TECH_NAME(TAF_TECH_DL_OFDMA), TAF_ETHERC(scb_taf), TAF_TYPE(TAF_TYPE_DL),
		scb_taf->info.traffic.est_units[TAF_TYPE_DL][tid], tid, scb_taf->score[TAF_TYPE_DL],
		scb_taf->info.scb_stats.ias.data.relative_score[TAF_TYPE_DL], bw_idx,
		ias_cfg(method).mu_ofdma_opt);

	min_mu_traffic = scb_taf->info.traffic.est_units[TAF_TYPE_DL][tid];
	total_mu_traffic = min_mu_traffic;

	while (list && (mu_user_count < admit_count)) {
		uint32 fraction;
		uint32 next_traffic;
		uint32 max_bridge_score = 0;
		bool inter_traffic_pending = FALSE;

		for (list = list->next; list && (mu_user_count < admit_count); list = list->next) {
			scb_taf_next = list->scb_taf;

			if (scb_taf_next->info.ps_mode) {
				WL_TAFM3(method, "skipping "MACF" in ps mode\n",
					TAF_ETHERC(scb_taf_next));
				continue;
			}

			if ((list->type == TAF_TYPE_DL) &&
				taf_ias_dl_ofdma_suitable(scb_taf_next, tid, tech_idx)) {

				if (bridge && (taf_ias_check_upd_rate(scb_taf_next, tech_idx, tid,
					uni_state, TAF_TYPE_DL) <
					((3 * total_mu_traffic) / (5 * mu_user_count)))) {

					WL_TAFM2(method, "bridge broke "MACF"%s insufficient "
						"traffic\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(TAF_TYPE_DL));
					bridge = NULL;
					max_bridge_score = 0;
					inter_traffic_pending = TRUE;
				}
				if (bridge) {
					WL_TAFM2(method, "bridge end "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(TAF_TYPE_DL));
				}
				inter_traffic = inter_traffic_pending;
				break;
			}

			if ((scb_taf_next->info.tech_type[list->type][tid] ==
				TAF_TECH_DONT_ASSIGN) ||
				(scb_taf_next->info.traffic.est_units[list->type][tid] == 0)) {

				continue;
			}

			if (bridge) {
				if (TAF_SCORE(scb_taf_next, list->type) > max_bridge_score) {
					WL_TAFM2(method, "bridge broke "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
					bridge = NULL;
					max_bridge_score = 0;
					inter_traffic = TRUE;
					break;
				} else {
					WL_TAFM2(method, "bridge contu "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
					continue;
				}
			}

			if (!inter_traffic_pending && TAF_OFDMA_OPT(method, BRIDGE_PAIR_DL) &&
				(mu_user_count < taf_info->dlofdma_maxn[bw_idx]) &&
				((max_bridge_score = taf_ias_max_bridge_score(method, scb_taf,
				TAF_TYPE_DL, mu_user_count)) >
				TAF_SCORE(scb_taf_next, list->type))) {

				bridge = list;
				WL_TAFM2(method, "bridge start "MACF"%s\n",
					TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
				continue;
			}

			WL_TAFM2(method, "inter traffic "MACF"%s\n",
				TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
			inter_traffic = TRUE;
			break;
		}

		if (!list || !scb_taf_next) {
			bridge = NULL;
			WL_TAFM3(method, "no next %u %s item found\n", mu_user_count + 1,
				TAF_TECH_NAME(TAF_TECH_DL_OFDMA));
			if (bridge) {
				WL_TAFM2(method, "bridge terminated\n");
				bridge = NULL;
			}
			break;
		}

		if (inter_traffic) {
			WL_TAFM2(method, "intermediate traffic ends %s pairing\n",
				TAF_TECH_NAME(tech_idx));
			inter_traffic = FALSE;
			break;
		}

		TAF_ASSERT(list->type == TAF_TYPE_DL);

		if (taf_ias_fifo_overlap(method, items, scb_taf_next, mu_user_count, tid)) {
			WL_TAFM2(method, "skipping "MACF" tid %u due to fifo overlap%s\n",
				TAF_ETHERC(scb_taf_next), tid, bridge ? " (bridging)" : "");
			scb_taf_next->info.tech_type[TAF_TYPE_DL][tid] = TAF_TECH_DONT_ASSIGN;
			continue;
		}

		if (bridge || TAF_OFDMA_OPT(method, PAIR)) {
			if (!taf_ias_mu_pair(method, scb_taf, scb_taf_next, TAF_TYPE_DL,
				mu_user_count, bridge != NULL)) {
				bridge = NULL;
				break;
			}
		}

		next_traffic = scb_taf_next->info.traffic.est_units[TAF_TYPE_DL][tid];

		fraction = (mu_user_count + 1) * next_traffic;

		if (TAF_OFDMA_OPT(method, TOO_SMALL) &&
			(fraction < time_limit_units) && (fraction < min_mu_traffic)) {

			bridge = NULL;
			WL_TAFM2(method, "traffic available is too small (%u/%u/%u)\n",
				fraction, time_limit_units, min_mu_traffic);
			break;
		}
		if (TAF_OFDMA_OPT(method, TOO_LARGE) &&
			(((mu_user_count + 1) * min_mu_traffic) < time_limit_units) &&
			(next_traffic > time_limit_units)) {

			bridge = NULL;
			WL_TAFM2(method, "traffic available too large for pairing "
				"(%u/%u)\n",  time_limit_units, min_mu_traffic);
			break;
		}

		if (bridge) {
			bool bridged = taf_ias_bridge_list(method, list, bridge, tid);

			if (!bridged) {
				WL_TAFM2(method, "failed to bridge intermediate traffic\n");
				bridge = NULL;
				break;
			}
		}

		WL_TAFM2(method, "next %s item %sis "MACF"%s having %u units tid %u and score "
			"%u, rel_score %u\n",
			TAF_TECH_NAME(TAF_TECH_DL_OFDMA), bridge ? "(bridged) " : "",
			TAF_ETHERC(scb_taf_next),
			TAF_TYPE(list->type), next_traffic, tid,
			scb_taf_next->score[TAF_TYPE_DL],
			scb_taf_next->info.scb_stats.ias.data.relative_score[TAF_TYPE_DL]);

		items[mu_user_count] = scb_taf_next;

		if (next_traffic < min_mu_traffic) {
			min_mu_traffic = next_traffic;
		}
		bw_idx = MAX(bw_idx, taf_bw_idx(scb_taf));
		mu_user_count++;
		total_mu_traffic = min_mu_traffic * mu_user_count;

		if ((bridge || TAF_OFDMA_OPT(method, MAXN_REL_DL)) &&
			(mu_user_count >= taf_info->dlofdma_maxn[bw_idx])) {
			bridge = NULL;
			WL_TAFM2(method, "ending due to maxN mu_user_count (%u)\n", mu_user_count);
			break;
		}

		bridge = NULL;
		if (mu_user_count >= TAF_MAX_MU_OFDMA || (mu_user_count >= admit_count)) {
			WL_TAFM2(method, "ending due to mu_user_count limit (%u)\n", mu_user_count);
			break;
		}
	}

	if (mu_user_count == 1) {
		WL_TAFM2(method, "only 1 %s user possible\n", TAF_TECH_NAME(TAF_TECH_DL_OFDMA));
		return FALSE;
	}

	TAF_ASSERT(ias_cfg(method).active_mu_count[TAF_TECH_DL_OFDMA][tid] >= mu_user_count);
	ias_cfg(method).active_mu_count[TAF_TECH_DL_OFDMA][tid] -= mu_user_count;

	if (!inter_traffic) {
		uint16* rel_limit_p = ias_cfg(method).mu_user_rel_limit[TAF_TECH_DL_OFDMA];
		uint32 user_rel_limit = TAF_MICROSEC_TO_UNITS(rel_limit_p[mu_user_count - 1]);

		user_time_limit_units =
			time_limit_units / MIN(mu_user_count, taf_info->dlofdma_maxn[bw_idx]);
		user_time_limit_units = MIN(user_time_limit_units, min_mu_traffic);

		if (user_rel_limit && user_time_limit_units > user_rel_limit) {
			WL_TAFM2(method, "%s %u limiting to %uus (%u)\n",
				TAF_TECH_NAME(TAF_TECH_DL_OFDMA), mu_user_count,
				TAF_UNITS_TO_MICROSEC(user_rel_limit), user_rel_limit);

			user_time_limit_units = user_rel_limit;
		}

		if (ias_cfg(method).release_limit > 0) {
			uint32 global_rel_limit =
				TAF_MICROSEC_TO_UNITS(ias_cfg(method).release_limit);
			user_time_limit_units = MIN(user_time_limit_units, global_rel_limit);
		}

		WL_TAFM1(method, "final %s possible: %u users, %u traffic possible per user, "
			"bw_idx %u - maxN %u\n",
			TAF_TECH_NAME(TAF_TECH_DL_OFDMA), mu_user_count, user_time_limit_units,
			bw_idx, taf_info->dlofdma_maxn[bw_idx]);
	} else {
		WL_TAFM4(method, "intermediate traffic so sweep this %s release\n",
			TAF_TECH_NAME(tech_idx));
		tech_idx = TAF_TECH_DONT_ASSIGN;
		user_time_limit_units = 0;
	}

	total_mu_traffic = taf_ias_tech_alloc(method, items, mu_user_count, user_time_limit_units,
		tech_idx, ++uni_state->pairing_id, FALSE, tid, TAF_TYPE_DL);

	if (total_mu_traffic > time_limit_units) {
		uni_state->extend.window_dur_units += total_mu_traffic - time_limit_units;

		WL_TAFM2(method, "%s extend units %u\n", TAF_TECH_NAME(tech_idx),
			total_mu_traffic - time_limit_units);
	}

	if (uni_state->last_paired[TAF_TYPE_DL] == 0) {
		uni_state->last_paired[TAF_TYPE_DL] = uni_state->pairing_id;
	}

	return TRUE;
}

#if TAF_ENABLE_UL
static INLINE bool BCMFASTPATH taf_ias_ul_ofdma_suitable(taf_scb_cubby_t *scb_taf, int tid,
	int bw_idx)
{
	if ((scb_taf->info.mu_tech_en[TAF_TECH_UL_OFDMA] & scb_taf->info.tid_enabled &
		(1 << tid)) == 0) {
		return FALSE;
	}
	if (scb_taf->info.ps_mode) {
		return FALSE;
	}
	if (scb_taf->info.tech_type[TAF_TYPE_UL][tid] < TAF_TECH_UNASSIGNED) {
		return FALSE;
	}
	if (SCB_MARKED_DEL(scb_taf->scb) || SCB_DEL_IN_PROGRESS(scb_taf->scb)) {
		return FALSE;
	}
	if (!scb_taf->info.scb_stats.ias.data.use[TAF_SOURCE_UL] ||
		scb_taf->info.linkstate[TAF_SOURCE_UL][tid] != TAF_LINKSTATE_ACTIVE) {
		return FALSE;
	}
	if (scb_taf->info.tech_type[TAF_TYPE_UL][tid] < TAF_TECH_UNASSIGNED) {
		return FALSE;
	}
	if (scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid] == 0) {
		return FALSE;
	}
	if (taf_bw_idx(scb_taf) != bw_idx) {
		return FALSE;
	}
	if (scb_taf->method->taf_info->ulofdma_maxn[bw_idx] == 0) {
		return FALSE;
	}
	return TRUE;
}

static INLINE bool BCMFASTPATH taf_ias_ul_ofdma(taf_method_info_t* method, taf_tech_type_t tech_idx,
	taf_list_t *list, taf_ias_uni_state_t *uni_state, int tid)
{
	wlc_taf_info_t* taf_info = method->taf_info;
	taf_scb_cubby_t *items[TAF_MAX_MU_OFDMA];
	taf_scb_cubby_t *scb_taf = list->scb_taf;
	taf_scb_cubby_t *scb_taf_next = NULL;
	uint32 mu_user_count = 1;
	uint32 min_mu_traffic;
	uint32 bump_mu_traffic;
	uint32 time_limit_units;
	uint32 user_time_limit_units;
	uint32 total_mu_traffic = 0;
	int bw_idx = taf_bw_idx(scb_taf);
	taf_list_t* bridge = NULL;
	bool inter_traffic = FALSE;
	uint32 admit_count = ias_cfg(method).active_mu_count[tech_idx][tid];

	if (list->type != TAF_TYPE_UL || admit_count == 0) {
		return FALSE;
	}

	TAF_ASSERT(tech_idx == TAF_TECH_UL_OFDMA);
	TAF_ASSERT(ias_cfg(method).mu_user_rel_limit[TAF_TECH_UL_OFDMA]);
	TAF_ASSERT(tid == TAF_UL_PRIO);

	time_limit_units = taf_ias_window_remain(method, uni_state, TAF_TYPE_UL);

	if (time_limit_units == 0) {
		return FALSE;
	}

	if (!taf_ias_ul_ofdma_suitable(scb_taf, tid, bw_idx)) {
		return FALSE;
	}

	items[0] = scb_taf;

	WL_TAFM2(method, "this %s item is "MACF"%s having %u units tid %u and score %u, "
		"rel score %u, bw_idx %u (opt %u)%s\n",
		TAF_TECH_NAME(TAF_TECH_UL_OFDMA), TAF_ETHERC(scb_taf), TAF_TYPE(TAF_TYPE_UL),
		scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid], tid, scb_taf->score[TAF_TYPE_UL],
		scb_taf->info.scb_stats.ias.data.relative_score[TAF_TYPE_UL], bw_idx,
		ias_cfg(method).mu_ofdma_opt, TAF_IAS_UL_WAS_BUMPED(scb_taf, tid) ?
		" was bumped" : "");

	min_mu_traffic = scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid];
	total_mu_traffic = min_mu_traffic;

	if (TAF_IAS_UL_WAS_BUMPED(scb_taf, tid) && TAF_OFDMA_OPT(method, MAX_BUMP_UL)) {
		bump_mu_traffic = min_mu_traffic;
	} else {
		bump_mu_traffic = 0;
	}

	while (list && (mu_user_count < admit_count)) {
		uint32 fraction;
		uint32 next_traffic;
		uint32 max_bridge_score = 0;
		bool inter_traffic_pending = FALSE;

		for (list = list->next; list && (mu_user_count < admit_count); list = list->next) {
			scb_taf_next = list->scb_taf;

			if (scb_taf_next->info.ps_mode) {
				WL_TAFM3(method, "skipping "MACF" in ps mode\n",
					TAF_ETHERC(scb_taf_next));
				continue;
			}

			if ((list->type == TAF_TYPE_UL) &&
					taf_ias_ul_ofdma_suitable(scb_taf_next, tid, bw_idx)) {

				if (bridge &&
					(scb_taf_next->info.traffic.est_units[TAF_TYPE_UL][tid] <
					((3 * total_mu_traffic) / (5 * mu_user_count)))) {

					WL_TAFM2(method, "bridge broke "MACF"%s insufficient "
						"traffic\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(TAF_TYPE_DL));
					bridge = NULL;
					max_bridge_score = 0;
					inter_traffic_pending = TRUE;
				}
				if (bridge) {
					WL_TAFM2(method, "bridge end "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(TAF_TYPE_UL));
				}
				inter_traffic = inter_traffic_pending;
				break;
			}

			if ((scb_taf_next->info.tech_type[list->type][tid] ==
				TAF_TECH_DONT_ASSIGN) ||
				(scb_taf_next->info.traffic.est_units[list->type][tid] == 0)) {

				continue;
			}

			if (bridge) {
				if (TAF_SCORE(scb_taf_next, list->type) > max_bridge_score) {
					WL_TAFM2(method, "bridge broke "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
					bridge = NULL;
					max_bridge_score = 0;
					inter_traffic = TRUE;
					break;
				} else {
					WL_TAFM2(method, "bridge contu "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
					continue;
				}
			}

			if (!inter_traffic_pending && TAF_OFDMA_OPT(method, BRIDGE_PAIR_UL) &&
				(mu_user_count < taf_info->ulofdma_maxn[bw_idx]) &&
				((max_bridge_score = taf_ias_max_bridge_score(method, scb_taf,
				TAF_TYPE_UL, mu_user_count)) >
				TAF_SCORE(scb_taf_next, list->type))) {

				bridge = list;
				WL_TAFM2(method, "bridge start "MACF"%s\n",
					TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
				continue;
			}

			WL_TAFM2(method, "inter traffic "MACF"%s\n",
				TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
			inter_traffic = TRUE;
			break;
		}

		if (!list || !scb_taf_next) {
			bridge = NULL;
			WL_TAFM3(method, "no next %u %s item found\n", mu_user_count + 1,
				TAF_TECH_NAME(TAF_TECH_UL_OFDMA));
			if (bridge) {
				WL_TAFM2(method, "bridge terminated\n");
				bridge = NULL;
			}
			break;
		}

		if (inter_traffic) {
			WL_TAFM1(method, "intermediate traffic ends %s pairing\n",
				TAF_TECH_NAME(tech_idx));
			inter_traffic = FALSE;
			break;
		}

		TAF_ASSERT(list->type == TAF_TYPE_UL);
		next_traffic = scb_taf_next->info.traffic.est_units[TAF_TYPE_UL][tid];

		if (bridge || TAF_OFDMA_OPT(method, PAIR)) {
			if (!taf_ias_mu_pair(method, scb_taf, scb_taf_next, TAF_TYPE_UL,
				mu_user_count, bridge != NULL)) {

				if (next_traffic <= bump_mu_traffic &&
					(mu_user_count + 1) * bump_mu_traffic <= time_limit_units) {

					WL_TAFM2(method, MACF" tid %u pairable due to traffic "
						"within range of previous bumped stations\n",
						TAF_ETHERC(scb_taf_next), tid);

				} else if (TAF_OFDMA_OPT(method, MAX_BUMP_UL) &&
					TAF_IAS_UL_WAS_BUMPED(scb_taf_next, tid) &&
					((mu_user_count + 1) * next_traffic) <= time_limit_units) {

					WL_TAFM2(method, MACF" tid %u pairable due to station "
						"bump\n", TAF_ETHERC(scb_taf_next), tid);
				} else {
					bridge = NULL;
					break;
				}
			}
		}

		fraction = (mu_user_count + 1) * next_traffic;

		if (TAF_OFDMA_OPT(method, TOO_SMALLUL) &&
			(next_traffic > bump_mu_traffic) &&
			(fraction < time_limit_units) && (fraction < (min_mu_traffic >> 1))) {

			bridge = NULL;
			WL_TAFM2(method, MACF"tid %u traffic available is too small (%u/%u/%u)\n",
				TAF_ETHERC(scb_taf_next), tid, fraction, time_limit_units,
				min_mu_traffic);
			break;
		}
		if (TAF_OFDMA_OPT(method, TOO_LARGEUL) &&
			(((mu_user_count + 1) * min_mu_traffic) < time_limit_units) &&
			(next_traffic > time_limit_units)) {

			bridge = NULL;
			WL_TAFM2(method, MACF"tid %u traffic available too large for pairing "
				"(%u/%u)\n",
				TAF_ETHERC(scb_taf_next), tid, time_limit_units, min_mu_traffic);
			break;
		}

		if (bridge) {
			bool bridged = taf_ias_bridge_list(method, list, bridge, tid);

			if (!bridged) {
				WL_TAFM2(method, "failed to bridge intermediate traffic\n");
				bridge = NULL;
				break;
			}
		}

		WL_TAFM2(method, "next %s item %sis "MACF"%s having %u units tid %u and score "
			"%u, rel_score %u%s\n",
			TAF_TECH_NAME(TAF_TECH_UL_OFDMA), bridge ? "(bridged) " : "",
			TAF_ETHERC(scb_taf_next),
			TAF_TYPE(list->type), next_traffic, tid,
			scb_taf_next->score[TAF_TYPE_UL],
			scb_taf_next->info.scb_stats.ias.data.relative_score[TAF_TYPE_UL],
			TAF_IAS_UL_WAS_BUMPED(scb_taf_next, tid) ? " was bumped" : "");

		items[mu_user_count] = scb_taf_next;

		if (next_traffic < min_mu_traffic) {
			min_mu_traffic = next_traffic;
		}

		if (TAF_OFDMA_OPT(method, MAX_BUMP_UL) &&
			TAF_IAS_UL_WAS_BUMPED(scb_taf_next, tid) &&
			(next_traffic > bump_mu_traffic)) {

			bump_mu_traffic = next_traffic;
		}
		bw_idx = MAX(bw_idx, taf_bw_idx(scb_taf));
		mu_user_count++;

		total_mu_traffic = MAX(min_mu_traffic, bump_mu_traffic) * mu_user_count;

		if ((bridge || TAF_OFDMA_OPT(method, MAXN_REL_UL)) &&
			(mu_user_count >= taf_info->ulofdma_maxn[bw_idx])) {
			bridge = NULL;
			WL_TAFM2(method, "ending due to maxN mu_user_count (%u)\n", mu_user_count);
			break;
		}

		bridge = NULL;
		if (mu_user_count >= TAF_MAX_MU_OFDMA || (mu_user_count >= admit_count)) {
			WL_TAFM2(method, "ending due to mu_user_count limit (%u)\n", mu_user_count);
			break;
		}
	}

	if (mu_user_count == 1) {
		WL_TAFM2(method, "only 1 %s user possible\n", TAF_TECH_NAME(TAF_TECH_UL_OFDMA));
		return FALSE;
	}

	if (TAF_OFDMA_OPT(method, UL_ODD) && !uni_state->free_release &&
			TAF_ODD_USERS(mu_user_count)) {

		WL_TAFM2(method, "%u odd-user optimisation\n", mu_user_count);
		do {
			mu_user_count--;
			ias_cfg(method).active_mu_count[TAF_TECH_UL_OFDMA][tid]--;
			items[mu_user_count]->info.tech_type[TAF_TYPE_UL][tid] =
				TAF_TECH_DONT_ASSIGN;
		} while (TAF_ODD_USERS(mu_user_count));

		TAF_ASSERT(mu_user_count >= 2);
	}

	TAF_ASSERT(ias_cfg(method).active_mu_count[TAF_TECH_UL_OFDMA][tid] >= mu_user_count);
	ias_cfg(method).active_mu_count[TAF_TECH_UL_OFDMA][tid] -= mu_user_count;

	if (!inter_traffic) {
		uint16* rel_limit_p = ias_cfg(method).mu_user_rel_limit[TAF_TECH_UL_OFDMA];
		uint32 user_rel_limit = TAF_MICROSEC_TO_UNITS(rel_limit_p[mu_user_count - 1]);

		user_time_limit_units =
			time_limit_units / MIN(mu_user_count, taf_info->ulofdma_maxn[bw_idx]);
		user_time_limit_units = MIN(user_time_limit_units, min_mu_traffic);

		if (user_rel_limit && user_time_limit_units > user_rel_limit) {
			WL_TAFM2(method, "%s %u limiting to %uus (%u)\n",
				TAF_TECH_NAME(TAF_TECH_UL_OFDMA), mu_user_count,
				TAF_UNITS_TO_MICROSEC(user_rel_limit), user_rel_limit);

			user_time_limit_units = user_rel_limit;
		}

		if (ias_cfg(method).release_limit > 0) {
			uint32 global_rel_limit =
				TAF_MICROSEC_TO_UNITS(ias_cfg(method).release_limit);
			user_time_limit_units = MIN(user_time_limit_units, global_rel_limit);
		}

		WL_TAFM1(method, "final %s possible: %u users, %u traffic possible per user, "
			"bw_idx %u - maxN %u%s\n",
			TAF_TECH_NAME(TAF_TECH_UL_OFDMA), mu_user_count, user_time_limit_units,
			bw_idx, taf_info->ulofdma_maxn[bw_idx],
			(bump_mu_traffic > 0) ? " max bumped" : "");
	} else {
		WL_TAFM4(method, "intermediate traffic so sweep this %s release\n",
			TAF_TECH_NAME(TAF_TECH_UL_OFDMA));
		tech_idx = TAF_TECH_DONT_ASSIGN;
		user_time_limit_units = 0;
	}

	total_mu_traffic = taf_ias_tech_alloc(method, items, mu_user_count, user_time_limit_units,
		tech_idx, ++uni_state->pairing_id, bump_mu_traffic > 0, tid, TAF_TYPE_UL);

	if (total_mu_traffic > time_limit_units) {
		uni_state->extend.window_dur_units += total_mu_traffic - time_limit_units;

		WL_TAFM2(method, "%s extend units %u\n", TAF_TECH_NAME(tech_idx),
			total_mu_traffic - time_limit_units);
	}

	if (uni_state->last_paired[TAF_TYPE_UL] == 0) {
		uni_state->last_paired[TAF_TYPE_UL] = uni_state->pairing_id;
	}
	return TRUE;
}
#endif /* TAF_ENABLE_UL */

static INLINE bool BCMFASTPATH taf_ias_dl_mimo_suitable(taf_scb_cubby_t *scb_taf, int bw_idx,
	uint32 streams_max, int tid, taf_tech_type_t tech_idx)
{
	uint32 streams;
	uint8 ratespec_mask;

	if ((scb_taf->info.mu_tech_en[tech_idx] & scb_taf->info.tid_enabled & (1 << tid)) == 0) {
		return FALSE;
	}
	if (scb_taf->info.ps_mode) {
		return FALSE;
	}
	if (scb_taf->info.tech_type[TAF_TYPE_DL][tid] < TAF_TECH_UNASSIGNED) {
		return FALSE;
	}
	if (SCB_MARKED_DEL(scb_taf->scb) || SCB_DEL_IN_PROGRESS(scb_taf->scb)) {
		return FALSE;
	}
#if TAF_ENABLE_PSQ_PULL && TAF_ENABLE_SQS_PULL
	/* for SQS platform, don't MU pair stations having PSQ packets available */
	if (scb_taf->info.traffic.count[TAF_SOURCE_PSQ][tid] != 0) {
		return FALSE;
	}
#endif
	if (!scb_taf->info.scb_stats.ias.data.use[TAF_SOURCE_AMPDU] ||
		scb_taf->info.linkstate[TAF_SOURCE_AMPDU][tid] != TAF_LINKSTATE_ACTIVE) {
		return FALSE;
	}
	if (scb_taf->info.traffic.est_units[TAF_TYPE_DL][tid] == 0) {
		return FALSE;
	}
	if (taf_bw_idx(scb_taf) != bw_idx) {
		return FALSE;
	}
	if ((streams = taf_nss(scb_taf)) >= streams_max || (streams_max == 4 && streams == 3)) {
		return FALSE;
	}
	ratespec_mask = (1 << TAF_RSPEC_MU_DL_INSTANT);
#if TAF_ENABLE_MU_BOOST
	ratespec_mask |= (1 << TAF_RSPEC_MU_DL_BOOST);
#endif
	if ((ratespec_mask & scb_taf->info.scb_stats.global.rdata.ratespec_mask) == 0) {
		WL_TAFM2(scb_taf->method, MACF" no rate info\n", TAF_ETHERC(scb_taf));
		return FALSE;
	}
	return TRUE;
}

static INLINE bool BCMFASTPATH taf_ias_dl_mimo(taf_method_info_t* method, taf_tech_type_t tech_idx,
	taf_list_t *list, taf_ias_uni_state_t *uni_state, int tid)
{
	taf_scb_cubby_t *items[TAF_MAX_MU_MIMO];
	uint32 traffic_estimate[TAF_MAX_MU_MIMO];
	taf_scb_cubby_t *scb_taf = list->scb_taf;
	taf_scb_cubby_t *scb_taf_next = NULL;
	uint32 mu_user_count = 1;
	uint32 time_limit_units;
	uint32 streams = 0;
	uint32 streams_max = TAF_WLCM(method)->stf->op_txstreams;
	uint32 min_mu_traffic;
	uint32 max_mu_traffc;
	int  bw_idx;
	uint32 orig_release;
	uint32 total_mu_traffic;
	uint32 release_len;
	taf_list_t* bridge = NULL;
	bool inter_traffic = FALSE;
	uint32 admit_count = ias_cfg(method).active_mu_count[tech_idx][tid];

	if (list->type != TAF_TYPE_DL || streams_max < 2) {
		return FALSE;
	}

	TAF_ASSERT(tech_idx == TAF_TECH_DL_HEMUMIMO || tech_idx == TAF_TECH_DL_VHMUMIMO);
	TAF_ASSERT(ias_cfg(method).mu_user_rel_limit[tech_idx]);

	time_limit_units = taf_ias_window_remain(method, uni_state, TAF_TYPE_DL);

	if (time_limit_units == 0) {
		return FALSE;
	}

	bw_idx = taf_bw_idx(scb_taf);

	if (!taf_ias_dl_mimo_suitable(scb_taf, bw_idx, streams_max, tid, tech_idx)) {
		return FALSE;
	}

	/* need to re-evaluate traffic according to MU rate info */
	traffic_estimate[0] =
		taf_ias_check_upd_rate(scb_taf, tech_idx, tid, uni_state, TAF_TYPE_DL);

	WL_TAFM2(method, "this %s item is "MACF"%s having MU %6u units tid %u, "
		"rel score %4u, mu rate %ux%2u(%u), su rate %ux%2u(%u)  (opt %u)\n",
		taf_mutech_text[tech_idx], TAF_ETHERC(scb_taf), TAF_TYPE(TAF_TYPE_DL),
		traffic_estimate[0], tid,
		scb_taf->info.scb_stats.ias.data.relative_score[TAF_TYPE_DL], taf_nss_mu(scb_taf),
		taf_mcs_mu(scb_taf), taf_encode_mu(scb_taf), taf_nss(scb_taf), taf_mcs(scb_taf),
		taf_encode(scb_taf), ias_cfg(method).mu_mimo_opt);

	streams = 1;
	items[0] = scb_taf;
	orig_release = traffic_estimate[0];

	min_mu_traffic = orig_release;
	max_mu_traffc = orig_release;
	total_mu_traffic = orig_release;

	while (list && (mu_user_count < admit_count)) {
		uint32 max_bridge_score = 0;
		bool inter_traffic_pending = FALSE;
		bool sweep = FALSE;

		for (list = list->next; list && (mu_user_count < admit_count); list = list->next) {
			scb_taf_next = list->scb_taf;

			if (scb_taf_next->info.ps_mode) {
				WL_TAFM3(method, "skipping "MACF" in ps mode\n",
					TAF_ETHERC(scb_taf_next));
				continue;
			}

			if ((list->type == TAF_TYPE_DL) &&
				taf_ias_dl_mimo_suitable(scb_taf_next, bw_idx, streams_max,
					tid, tech_idx)) {

				if ((sweep || bridge) &&
					(taf_ias_check_upd_rate(scb_taf_next, tech_idx, tid,
					uni_state, TAF_TYPE_DL) <
					((3 * total_mu_traffic) / (5 * mu_user_count)))) {

					WL_TAFM2(method, "%s broke "MACF"%s insufficient traffic\n",
						bridge ? "bridge" : "sweep",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(TAF_TYPE_DL));
					bridge = NULL;
					max_bridge_score = 0;
					sweep = FALSE;
					inter_traffic_pending = TRUE;
				}
				if (bridge) {
					WL_TAFM2(method, "bridge ended "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(TAF_TYPE_DL));
				}
				inter_traffic = inter_traffic_pending;
				break;
			}

			if ((scb_taf_next->info.tech_type[list->type][tid] ==
				TAF_TECH_DONT_ASSIGN) ||
				(scb_taf_next->info.traffic.est_units[list->type][tid] == 0)) {

				continue;
			}

			if (bridge) {
				if (TAF_SCORE(scb_taf_next, list->type) > max_bridge_score) {
					WL_TAFM2(method, "bridge broke "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
					bridge = NULL;
					max_bridge_score = 0;

					if ((mu_user_count < (streams_max - 1)) &&
						(mu_user_count < admit_count) &&
						TAF_MIMO_OPT(method, SWEEP)) {

						WL_TAFM2(method, "try sweeping\n");
						inter_traffic_pending = TRUE;
						sweep = TRUE;
						continue;
					}
					inter_traffic = TRUE;
					break;
				} else {
					WL_TAFM2(method, "bridge contu "MACF"%s\n",
						TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
					continue;
				}
			}

			if (!inter_traffic_pending && TAF_MIMO_OPT(method, BRIDGE_PAIR_DL) &&
				(mu_user_count < streams_max) &&
				((max_bridge_score = taf_ias_max_bridge_score(method, scb_taf,
				TAF_TYPE_DL, mu_user_count)) >
				TAF_SCORE(scb_taf_next, list->type))) {

				bridge = list;
				WL_TAFM2(method, "bridge start "MACF"%s\n",
					TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));
				continue;
			}

			WL_TAFM2(method, "inter traffic "MACF"%s\n",
				TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type));

			if (mu_user_count >= streams_max) {
				inter_traffic = TRUE;
				break;
			}

			if (TAF_MIMO_OPT(method, SWEEP)) {
				if (!sweep) {
					inter_traffic_pending = TRUE;
					sweep = TRUE;
					WL_TAFM2(method, "try sweeping\n");
				}
				continue;
			}
			inter_traffic = TRUE;
			break;

		}

		if (!list || !scb_taf_next) {
			WL_TAFM3(method, "no next %u %s item found\n", mu_user_count + 1,
				TAF_TECH_NAME(tech_idx));
			if (bridge) {
				WL_TAFM2(method, "bridge terminated\n");
				bridge = NULL;
			}
			break;
		}

		if (inter_traffic && !sweep) {
			WL_TAFM1(method, "intermediate traffic ends %s pairing\n",
				TAF_TECH_NAME(tech_idx));
			inter_traffic = FALSE;
			break;
		}

		TAF_ASSERT(list->type == TAF_TYPE_DL);
		TAF_ASSERT(!bridge || !sweep);

		if (!sweep &&
			taf_ias_fifo_overlap(method, items, scb_taf_next, mu_user_count, tid)) {

			WL_TAFM2(method, "skipping "MACF" tid %u due to fifo overlap%s\n",
				TAF_ETHERC(scb_taf_next), tid, bridge ? " (bridging)" : "");
			scb_taf_next->info.tech_type[TAF_TYPE_DL][tid] =
				TAF_TECH_DONT_ASSIGN;
			continue;
		}

		if (bridge || TAF_MIMO_OPT(method, PAIR) ||
			((mu_user_count >= streams_max) && TAF_MIMO_OPT(method, PAIR_MAXMU))) {

			if (!taf_ias_mu_pair(method, scb_taf, scb_taf_next, TAF_TYPE_DL,
				mu_user_count, bridge != NULL)) {
				bridge = NULL;
				break;
			}
		}

		if (bridge) {
			bool bridged = taf_ias_bridge_list(method, list, bridge, tid);

			if (!bridged) {
				WL_TAFM2(method, "failed to bridge intermediate traffic\n");
				bridge = NULL;
				break;
			}
		}
		items[mu_user_count] = scb_taf_next;

		traffic_estimate[mu_user_count] =
			taf_ias_check_upd_rate(scb_taf_next, tech_idx, tid, uni_state, TAF_TYPE_DL);

		++streams;

		total_mu_traffic += traffic_estimate[mu_user_count];

		max_mu_traffc = MAX(max_mu_traffc, traffic_estimate[mu_user_count]);
		min_mu_traffic = MIN(min_mu_traffic, traffic_estimate[mu_user_count]);

		WL_TAFM2(method, "next %s item %sis "MACF"%s having MU %6u units tid %u, "
			"rel_score %4u, mu rate %ux%2u(%u), su rate %ux%2u(%u) %s\n",
			TAF_TECH_NAME(tech_idx), bridge ? "(bridged) " : "",
			TAF_ETHERC(scb_taf_next), TAF_TYPE(list->type),
			traffic_estimate[mu_user_count], tid,
			scb_taf_next->info.scb_stats.ias.data.relative_score[TAF_TYPE_DL],
			taf_nss_mu(scb_taf_next), taf_mcs_mu(scb_taf_next), taf_encode_mu(scb_taf),
			taf_nss(scb_taf_next), taf_mcs(scb_taf_next), taf_encode(scb_taf),
			inter_traffic ? "(inter)" : "");

		mu_user_count++;

		if (mu_user_count >= TAF_MAX_MU_MIMO || (mu_user_count >= admit_count) ||
			((mu_user_count >= streams_max) &&
			(bridge || TAF_MIMO_OPT(method, MAX_STREAMS)))) {

			bridge = NULL;

			WL_TAFM2(method, "ending %s due to mu_user_count of %u\n",
				TAF_TECH_NAME(tech_idx), mu_user_count);
			break;
		}
		bridge = NULL;
	}

	if (mu_user_count == 1) {
		WL_TAFM2(method, "only 1 %s user possible\n", TAF_TECH_NAME(tech_idx));
		return FALSE;
	}

	TAF_ASSERT(ias_cfg(method).active_mu_count[tech_idx][tid] >= mu_user_count);
	ias_cfg(method).active_mu_count[tech_idx][tid] -= mu_user_count;

	if (!inter_traffic) {
		uint16* rel_limit_p = ias_cfg(method).mu_user_rel_limit[tech_idx];
		uint32 user_rel_limit = rel_limit_p ?
			TAF_MICROSEC_TO_UNITS(rel_limit_p[mu_user_count - 1]) : 0;

		TAF_ASSERT(bridge == NULL);

		if (TAF_MIMO_OPT(method, MEAN)) {
			release_len = total_mu_traffic / mu_user_count;

		} else if (TAF_MIMO_OPT(method, MIN)) {
			release_len = min_mu_traffic;

		} else if (TAF_MIMO_OPT(method, MEDIAN)) {
			release_len = (min_mu_traffic + max_mu_traffc) / 2;

		} else {
			release_len = max_mu_traffc;
		}

		if (user_rel_limit && release_len > user_rel_limit) {
			WL_TAFM2(method, "%s release %u limiting to %uus (%u)\n",
				TAF_TECH_NAME(tech_idx), mu_user_count,
				TAF_UNITS_TO_MICROSEC(user_rel_limit), user_rel_limit);

			release_len = user_rel_limit;
		}

		if (release_len > time_limit_units) {
			WL_TAFM2(method, "mu-mimo release limiting to remaining window %uus (%u)\n",
				TAF_UNITS_TO_MICROSEC(time_limit_units), time_limit_units);
			release_len = time_limit_units;
		}

		if (ias_cfg(method).release_limit > 0 &&
			release_len > ias_cfg(method).release_limit) {

			uint32 global_rel_limit =
				TAF_MICROSEC_TO_UNITS(ias_cfg(method).release_limit);

			WL_TAFM2(method, "mu-mimo release limiting to global limit %uus (%u)\n",
				ias_cfg(method).release_limit, global_rel_limit);
			release_len = global_rel_limit;
		}

		WL_TAFM1(method, "final %s possible: %u users, %u/%u min/max "
			"traffic, %u possible per user\n", TAF_TECH_NAME(tech_idx), mu_user_count,
			min_mu_traffic, max_mu_traffc, release_len);
	} else {
		WL_TAFM1(method, "intermediate traffic so sweep this %s release\n",
			TAF_TECH_NAME(tech_idx));
		tech_idx = TAF_TECH_DONT_ASSIGN;
		release_len = 0;
	}

	total_mu_traffic = taf_ias_tech_alloc(method, items, mu_user_count, release_len, tech_idx,
		++uni_state->pairing_id, FALSE, tid, TAF_TYPE_DL);

	if (total_mu_traffic > time_limit_units) {
		uni_state->extend.window_dur_units += total_mu_traffic - time_limit_units;

		WL_TAFM2(method, "%s extend units %u\n", TAF_TECH_NAME(tech_idx),
			total_mu_traffic - time_limit_units);
	}
	return TRUE;
}
#endif /* TAF_ENABLE_MU_TX */

static INLINE void BCMFASTPATH taf_ias_analyse_mu(taf_method_info_t* method,
	taf_list_t *list, taf_ias_uni_state_t *uni_state, int tid)
{
#if TAF_ENABLE_MU_TX
	taf_tech_type_t tech_idx = 0;
	taf_scb_cubby_t *scb_taf = list->scb_taf;

	TAF_ASSERT(tid >= 0 && tid < NUMPRIO);

	if (method->type == TAF_EBOS) {
		return;
	}

	if (scb_taf->info.ps_mode) {
		return;
	}

	if (uni_state->free_release) {
		WL_TAFM2(method, "free release\n");
		return;
	}

	if (scb_taf->info.tech_type[list->type][tid] < TAF_TECH_UNASSIGNED) {
		WL_TAFM4(method, MACF"%s tid %u is already allocated for %s (%d units)\n",
			TAF_ETHERC(scb_taf), TAF_TYPE(list->type), tid,
			TAF_TECH_NAME(scb_taf->info.tech_type[list->type][tid]),
			(TAF_IAS_TID_STATE(scb_taf, tid))->release_config[list->type].units);
		return;
	}

#if TAF_ENABLE_UL
	if (list->type == TAF_TYPE_UL) {
		tech_idx = TAF_TECH_UL_OFDMA;
	}
#endif
	for (; scb_taf->info.tech_type[list->type][tid] == TAF_TECH_UNASSIGNED &&
			tech_idx < TAF_NUM_MU_TECH_TYPES;
			tech_idx++) {
		wlc_taf_info_t* taf_info = method->taf_info;

		if (!(taf_info->mu & (1 << tech_idx))) {
			continue;
		}
		if (!(scb_taf->info.mu_tech_en[tech_idx] &
			taf_info->mu_g_enable_mask[tech_idx] & (1 << tid))) {
			continue;
		}

		if (list->type == TAF_TYPE_DL &&
			ias_cfg(method).active_mu_count[tech_idx][tid] < 2) {
			/* need at least 2 active stations for downlink MU */
			continue;
		}
		switch (tech_idx) {
			case TAF_TECH_DL_HEMUMIMO:
			case TAF_TECH_DL_VHMUMIMO:
				taf_ias_dl_mimo(method, tech_idx, list, uni_state, tid);
				continue;
			case TAF_TECH_DL_OFDMA:
				taf_ias_dl_ofdma(method, tech_idx, list, uni_state, tid);
				break;
#if TAF_ENABLE_UL
			case TAF_TECH_UL_OFDMA:
				taf_ias_ul_ofdma(method, tech_idx, list, uni_state, tid);
				continue;
#endif /* TAF_ENABLE_UL */
			default:
				TAF_ASSERT(0);
		}
		break;
	}
#else
	BCM_REFERENCE(method);
	BCM_REFERENCE(list);
	BCM_REFERENCE(uni_state);
	BCM_REFERENCE(tid);
#endif /* TAF_ENABLE_MU_TX */
}

static INLINE void BCMFASTPATH taf_ias_sched_active_list(taf_method_info_t *method)
{
	taf_list_t* item = method->list;
	taf_list_t* local_list_end = NULL;

	TAF_ASSERT(method->active_list == NULL);

	while (item) {
		taf_list_t* next = item->next;

		if (item->used || item->bridged) {
			item->used = 0;
			item->bridged = 0;
			wlc_taf_list_remove(&method->list, item);

			item->prev = local_list_end;

			if (local_list_end != NULL) {
				local_list_end->next = item;
			} else if (method->active_list == NULL) {
				method->active_list = item;
			}
			local_list_end = item;
		}
		item = next;
	}
}

static INLINE void BCMFASTPATH taf_ias_list_separate_sort(taf_method_info_t *method)
{
	TAF_ASSERT(method->ordering == TAF_LIST_SCORE_MINIMUM);

	taf_ias_sched_active_list(method);

	if (method->active_list) {
		WL_TAFM3(method, "sort active list\n");
		wlc_taf_sort_list(&method->active_list, TAF_LIST_SCORE_MINIMUM,
			TAF_IAS_NOWTIME(method));

		WL_TAFM3(method, "merge active and idle lists\n");
		method->list = wlc_taf_merge_list(&method->active_list, &method->list,
			TAF_LIST_SCORE_MINIMUM, TAF_IAS_NOWTIME(method));
	}
}

static INLINE void BCMFASTPATH taf_ias_prepare_list(taf_method_info_t *method)
{
	if (method->list == NULL) {
		WL_TAFM4(method, "empty list\n");
		return;
	}

	taf_ias_pre_update_list(method);

	if (method->ordering == TAF_LIST_SCORE_MINIMUM) {
		taf_ias_list_separate_sort(method);
	}

	TAF_ASSERT(method->active_list == NULL);

	taf_ias_post_update_list(method);
}

static INLINE void BCMFASTPATH
taf_ias_optimise_agg(taf_scb_cubby_t *scb_taf, int tid, taf_ias_uni_state_t* uni_state,
	taf_schedule_state_t op_state, taf_release_context_t *context)
{
	/* aggregation optimisation */

	/* hold_for_aggs = AMSDU optimise */
	bool hold_for_aggs = (uni_state->trigger != TAF_TRIGGER_STAR_THRESHOLD);
	/* hold_for_aggp = AMPDU optimise */
	bool hold_for_aggp;
	uint32 more_traffic = 0;
	taf_method_info_t* scb_method = scb_taf->method;
	uint32 wme_ac = SCB_WME(scb_taf->scb) ? WME_PRIO2AC(tid) : AC_BE;
	int32 in_flight = SCB_PKTS_INFLT_FIFOCNT_VAL(scb_taf->scb, tid);
#if TAF_ENABLE_MU_TX
	bool mu_agg_hold = TRUE;

	if (scb_taf->info.tech_type[TAF_TYPE_DL][tid] == TAF_TECH_DL_OFDMA &&
		!TAF_OFDMA_OPT(scb_method, AGG_HOLD)) {

		mu_agg_hold = FALSE;

	} else if (scb_taf->info.tech_type[TAF_TYPE_DL][tid] <= TAF_TECH_DL_VHMUMIMO &&
		!TAF_MIMO_OPT(scb_method, AGG_HOLD)) {

		mu_agg_hold = FALSE;
	}
#else
	const bool mu_agg_hold = TRUE;
#endif /* TAF_ENABLE_MU_TX */

	if (scb_taf->info.ps_mode) {
		context->public.ias.opt_aggs = FALSE;
		context->public.ias.opt_aggp = FALSE;
		return;
	}

	/* This controls aggregation optimisation for real packet release - release
	 * function can use to optimised (hold back) packets for aggregation.
	 * Aggregation optmisation is only done in case of immediate triggering;
	 * if normal IAS TX window is working, do not do this normally.
	 */

	if ((scb_method->type == TAF_EBOS) || (wme_ac == AC_VO)) {
		/* traffic is prioritised, do not hold back for aggregation */
		hold_for_aggs = FALSE;
	}

	/* check traffic in-transit, if 0 do not hold for aggregation as left over
	* aggregation packets might never get sent without follow-up
	* new traffic which is not guaranteed to come
	*/
	hold_for_aggs = hold_for_aggs && (in_flight > 0) && mu_agg_hold;

	/* if already plenty of in flight traffic, do not hold AMPDU as AQM can be fed */
	hold_for_aggp = hold_for_aggs && (in_flight < scb_taf->info.max_pdu) && mu_agg_hold;

#if TAF_ENABLE_SQS_PULL
	/* Is there is more traffic on the way very soon...? Aggregation can be held as
	 * forthcoming traffic will add to packets already available. Leftover aggregation
	 * packets will ripple forward into the new arriving data where this decision will
	 * be made again next time.
	 */
	more_traffic = (op_state == TAF_SCHEDULE_VIRTUAL_PACKETS) ?
		scb_taf->info.traffic.count[TAF_SOURCE_HOST_SQS][tid] :
		scb_taf->info.pkt_pull_request[tid];
#endif /* TAF_ENABLE_SQS_PULL */

	hold_for_aggs = hold_for_aggs || (more_traffic > 0);
	hold_for_aggp = hold_for_aggp || (more_traffic > 0);

	/* flag to enable aggregation optimisation or not */
	context->public.ias.opt_aggs = hold_for_aggs;
	context->public.ias.opt_aggp = hold_for_aggp;

	/* pdu number threshold for AMPDU holding */
	context->public.ias.opt_aggp_limit =
		ias_cfg(scb_method).opt_aggp_limit < scb_taf->info.max_pdu ?
		ias_cfg(scb_method).opt_aggp_limit : scb_taf->info.max_pdu;
}

static bool BCMFASTPATH
taf_ias_send_source(taf_method_info_t *method, taf_scb_cubby_t *scb_taf,
	int tid, taf_source_type_t s_idx, taf_ias_uni_state_t* uni_state,
	taf_ias_sched_data_t* ias_sched, taf_release_context_t *context, taf_list_type_t type)
{
	wlc_taf_info_t* taf_info;
	uint32 actual_release = 0;
#if TAF_ENABLE_SQS_PULL
	uint32 virtual_release = 0;
	uint32 pending_release = 0;
#else
#ifdef TAF_DBG
	const uint32 virtual_release = 0;
	const uint32 pending_release = 0;
#endif /* TAF_DBG */
#endif /* TAF_ENABLE_SQS_PULL */
	uint32 extend_units = 0;
	uint32 released_units = 0;
	uint32 released_bytes = 0;
	uint32 time_limit_units;
	taf_ias_sched_rel_stats_t* rel_stats;
	bool force;
	uint32 pre_qlen = 0;
	uint32 post_qlen = 0;
	uint32 release_n = 0;

	TAF_ASSERT(ias_sched);

#ifdef TAF_DBG
	BCM_REFERENCE(virtual_release);
	BCM_REFERENCE(pending_release);
#endif
	BCM_REFERENCE(rel_stats);

	if (!TAF_IAS_SOURCE_ENABLED(ias_sched, s_idx)) {
		return TRUE;
	}

	taf_info = method->taf_info;
	time_limit_units = taf_ias_window_total(method, uni_state, type);
	force = (scb_taf->info.force & (1 << tid)) ? TRUE : FALSE;
	context->public.ias.was_emptied = FALSE;
	context->public.ias.is_ps_mode = scb_taf->info.ps_mode;
	context->public.ias.traffic_count_available = scb_taf->info.traffic.count[s_idx][tid];
	context->public.ias.min_mpdu_dur_units = ias_cfg(method).min_mpdu_dur[type];
	context->public.ias.updated = (ias_sched->source_updated & (1 << s_idx)) != 0;

#if TAF_ENABLE_SQS_PULL
	if (TAF_SOURCE_IS_VIRTUAL(s_idx)) {
		if (scb_taf->info.pkt_pull_request[tid]) {
			/* left-over unfulfilled request */
			WL_ERROR(("wl%u %s: (%x) "MACF" tid %u pkt_pull_rqst %u map 0x%x "
				"(sqs fault %u)\n",
				WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__,
				taf_info->scheduler_index,
				TAF_ETHERC(scb_taf),
				tid, scb_taf->info.pkt_pull_request[tid],
				scb_taf->info.pkt_pull_map,
				taf_info->sqs_state_fault));
			scb_taf->info.pkt_pull_request[tid] = 0;
			scb_taf->info.pkt_pull_map &= ~(1 << tid);
		}
		/* margin here is used to ask SQS for extra packets beyond nominal */
		context->public.ias.margin = ias_cfg(method).margin;
	}
#endif /* TAF_ENABLE_SQS_PULL */
	if (TAF_SOURCE_IS_REAL(s_idx) && TAF_SOURCE_IS_DL(s_idx)) {
		/* Record exact level of queue before; this is required because AMSDU
		* in AMPDU is variable according to various configs and situations, and it
		* is necessary to measure this again after release to see what actually went
		* for accurate packet size estimation
		*/
		pre_qlen = taf_info->funcs[s_idx].pktqlen_fn(*(taf_info->source_handle_p[s_idx]),
			ias_sched->handle[s_idx].scbh, ias_sched->handle[s_idx].tidh,
			TAF_IAS_NOWTIME(method));
	}

	context->public.complete = TAF_REL_COMPLETE_TIME_LIMIT;
	context->public.ias.time_limit_units = time_limit_units;

	if (type == TAF_TYPE_DL && !scb_taf->info.ps_mode) {
		if (TAF_SOURCE_IS_AMPDU(s_idx)) {
			/* aggregation optimisation */
			taf_ias_optimise_agg(scb_taf, tid, uni_state, taf_info->op_state, context);
		}
	} else {
		context->public.ias.opt_aggs = 0;
		context->public.ias.opt_aggp = FALSE;
	}

	WL_TAFM4(method, "time_limit_units = %u, released_units = %u\n",
		time_limit_units, uni_state->total.released_units);

	/* for accounting, real or virtual both get counted here */
	while ((uni_state->total.released_units + released_units) < time_limit_units) {

		bool result;

		context->public.ias.total.released_units = uni_state->total.released_units +
				released_units;

		context->public.mode = TAF_SOURCE_IS_REAL(s_idx) ?
#ifdef WLCFP
			TAF_RELEASE_MODE_REAL_FAST :
#else
			TAF_RELEASE_MODE_REAL :
#endif /* WLCFP */
			TAF_RELEASE_MODE_VIRTUAL;

		context->public.complete = TAF_REL_COMPLETE_NULL;

		result = taf_info->funcs[s_idx].release_fn(*taf_info->source_handle_p[s_idx],
			ias_sched->handle[s_idx].scbh, ias_sched->handle[s_idx].tidh, force,
			&context->public);

#if TAF_ENABLE_SQS_PULL
		if (TAF_SOURCE_IS_VIRTUAL(s_idx)) {
			TAF_ASSERT(context->public.ias.actual.release == 0);

			virtual_release += context->public.ias.virtual.release;
			context->public.ias.virtual.release = 0;

			pending_release += context->public.ias.pending.release;
			context->public.ias.pending.release = 0;

			released_units += context->public.ias.virtual.released_units;
			context->public.ias.virtual.released_units = 0;

			released_units += context->public.ias.pending.released_units;
			context->public.ias.pending.released_units = 0;
		} else
#endif /* TAF_ENABLE_SQS_PULL */
		{
			actual_release += context->public.ias.actual.release;
			context->public.ias.actual.release = 0;

			released_units += context->public.ias.actual.released_units;
			context->public.ias.actual.released_units = 0;

			released_bytes += context->public.ias.actual.released_bytes;
			context->public.ias.actual.released_bytes = 0;
		}

		extend_units += context->public.ias.extend.units;
		context->public.ias.extend.units = 0;

		if (context->public.ias.was_emptied) {
			break;
		}
#if TAF_ENABLE_SQS_PULL
		if ((TAF_SOURCE_IS_REAL(s_idx) && context->public.ias.actual.release == 0) ||
			(TAF_SOURCE_IS_VIRTUAL(s_idx) &&
			context->public.ias.virtual.release == 0) ||
			!result)
#else
		if (context->public.ias.actual.release == 0 || !result)
#endif /* TAF_ENABLE_SQS_PULL */
		{
			break;
		}

		if (force && released_units >= TAF_MICROSEC_TO_UNITS(taf_info->force_time)) {
			scb_taf->info.force &= ~(1 << tid);
			break;
		}
	}

	if (released_units) {
		if (TAF_SOURCE_IS_REAL(s_idx) && TAF_SOURCE_IS_DL(s_idx) && (pre_qlen > 0)) {
			post_qlen = taf_info->funcs[s_idx].pktqlen_fn(
				*(taf_info->source_handle_p[s_idx]),
				ias_sched->handle[s_idx].scbh,
				ias_sched->handle[s_idx].tidh, TAF_IAS_NOWTIME(method));

			release_n = pre_qlen - post_qlen;
		}

#if TAF_ENABLE_SQS_PULL && !TAF_LOGL4
		if (method->type != TAF_VIRTUAL_MARKUP)
#endif
		{
			WL_TAFM1(method, MACF"%s tid %u %s: tot_units %6u, rel %5u, ext %3u, "
				"%3u real (%3un) / %3u virt / %2u pend, ridx %u, %u:%s%s\n",
				TAF_ETHERC(scb_taf), TAF_TYPE(type), tid,
				TAF_SOURCE_NAME(s_idx), uni_state->total.released_units,
				released_units, extend_units,
				actual_release, release_n, virtual_release, pending_release,
#if TAF_ENABLE_MU_TX
				scb_taf->info.scb_stats.ias.data.ridx_used[type],
#else
				0,
#endif
				context->public.complete,
				TAF_REL_COMPLETE_TXT(context->public.complete),
				context->public.ias.was_emptied ? " E":"");
		}
	} else {
#if TAF_ENABLE_SQS_PULL && !TAF_LOGL3
		if (method->type != TAF_VIRTUAL_MARKUP ||
			context->public.complete >= TAF_REL_COMPLETE_ERR)
#endif
		{
			WL_TAFM1(method, MACF"%s %s: no release (%u/%u) reason %02u:%s\n",
			TAF_ETHERC(scb_taf), TAF_TYPE(type), TAF_SOURCE_NAME(s_idx),
			uni_state->total.released_units, time_limit_units,
			context->public.complete, TAF_REL_COMPLETE_TXT(context->public.complete));

		}
	}

	/* the following is statistics across the TID state which is global across all SCB using
	 * that TID
	 */
	uni_state->total.released_units += released_units;
	uni_state->extend.rounding_units += extend_units;

#if TAF_ENABLE_SQS_PULL
	if (virtual_release) {
		TAF_ASSERT(TAF_SOURCE_IS_VIRTUAL(s_idx));
		context->virtual_release += virtual_release;
		scb_taf->info.pkt_pull_request[tid] = virtual_release;
		scb_taf->info.pkt_pull_map |= (1 << tid);
		taf_info->total_pull_requests++;
		uni_state->data.virtual_units += released_units;

		WL_TAFT3(taf_info, "total_pull_requests %u\n", taf_info->total_pull_requests);
	}
	context->pending_release += pending_release;
#endif /* TAF_ENABLE_SQS_PULL */

	if (actual_release) {
#if TAF_ENABLE_SQS_PULL
		TAF_ASSERT(TAF_SOURCE_IS_REAL(s_idx));
#endif /* TAF_ENABLE_SQS_PULL */

		uni_state->real[type].released_units += released_units;
		uni_state->total_units_in_transit[type] += released_units;

		/* the following statistics are temporary prior to update into SCB,TID link */
		uni_state->data.release_pcount += actual_release;
		uni_state->data.release_units += released_units;
		uni_state->data.release_bytes += released_bytes;

		if (type == TAF_TYPE_DL) {
			uni_state->data.release_ncount += release_n;
		}

		/* this is global across the whole scheduling interval */
		context->actual_release += actual_release;
		context->actual_release_n += release_n;

	} else if (TAF_SOURCE_IS_AMPDU(s_idx) &&
		context->public.complete == TAF_REL_COMPLETE_NOTHING_AGG &&
		taf_info->op_state != TAF_SCHEDULE_VIRTUAL_PACKETS) {

		TAF_DPSTATS_LOG_ADD(rel_stats, held_z, 1);
	}
	switch (context->public.complete) {
		case TAF_REL_COMPLETE_RESTRICTED:
			TAF_DPSTATS_LOG_ADD(rel_stats, restricted, 1);
			/* fall through */
		case TAF_REL_COMPLETE_FULL:
		case TAF_REL_COMPLETE_BLOCKED:
		case TAF_REL_COMPLETE_NO_BUF:
		case TAF_REL_COMPLETE_ERR:
			TAF_DPSTATS_LOG_ADD(rel_stats, error, 1);
			context->status = TAF_CYCLE_FAILURE;
			break;

		case TAF_REL_COMPLETE_REL_LIMIT:
			return TRUE;
			break;
		default:
			break;
	}

	return context->public.ias.was_emptied;
}

static void BCMFASTPATH taf_ias_decay_stats(taf_ias_sched_rel_stats_t* rel_stats,
	taf_ias_coeff_t* decay, taf_list_type_t type, uint32 now_time)
{
	uint32 elapsed = 0;
	int32 time_correction = 0;
	uint32 coeff;

	if (type == TAF_TYPE_DL) {
		elapsed = now_time - rel_stats->data.calc_timestamp;
	}
#if TAF_ENABLE_UL
	else {
		elapsed = now_time - rel_stats->data.rx.calc_timestamp;
	}
#endif
	coeff = taf_ias_decay_score(TAF_COEFF_IAS_MAX, decay, elapsed, &time_correction);

	if (coeff < TAF_COEFF_IAS_MAX) {
		if (type == TAF_TYPE_DL) {
			rel_stats->data.calc_timestamp = now_time - time_correction;

			TAF_IAS_DECAY64(coeff, rel_stats->data.total_scaled_ncount);
			TAF_IAS_DECAY64(coeff, rel_stats->data.total_scaled_pcount);
			TAF_IAS_DECAY64(coeff, rel_stats->data.total_bytes);
			TAF_IAS_DECAY64(coeff, rel_stats->data.total_release_units);
		}
#if TAF_ENABLE_UL
		else {
			rel_stats->data.rx.calc_timestamp = now_time - time_correction;

			TAF_IAS_DECAY64(coeff, rel_stats->data.rx.bytes);
			TAF_IAS_DECAY64(coeff, rel_stats->data.rx.time);
			TAF_IAS_DECAY64(coeff, rel_stats->data.rx.scaled_pcount);
			TAF_IAS_DECAY64(coeff, rel_stats->data.rx.scaled_acount);
			TAF_IAS_DECAY64(coeff, rel_stats->data.rx.max_pcount);
			TAF_IAS_DECAY64(coeff, rel_stats->data.rx.scaled_qncount);

			if (now_time - rel_stats->data.rx.prev_timestamp >
				TAF_IAS_RXMON_IDLE_LIMIT) {

				rel_stats->data.rx.time +=
					now_time - rel_stats->data.rx.prev_timestamp;
				rel_stats->data.rx.prev_timestamp = now_time;
			}
		}
#endif /* TAF_ENABLE_UL */
	}
}

static INLINE void BCMFASTPATH
taf_ias_mean_pktlen(taf_scb_cubby_t *scb_taf,  taf_ias_uni_state_t *uni_state,
	taf_ias_sched_rel_stats_t* rel_stats, uint32 timestamp, taf_ias_coeff_t* decay)
{
	/*
	* This calculates the average packet size from what has been released. It is going to be
	* used under SQS, to forward predict how many virtual packets to be converted.
	*/

	uint32 mean;
	uint32 ncount = uni_state->data.release_ncount;
	uint32 pcount = uni_state->data.release_pcount;
	uint32 release_units = uni_state->data.release_units;
	uint32 release_bytes = uni_state->data.release_bytes;

	if (ncount == 0) {
		return;
	}

	taf_ias_decay_stats(rel_stats, decay, TAF_TYPE_DL, timestamp);

	/*
	* packet counts are scaled here, as at low data rates, the discrete low packet count
	* loses precision when doing exponential decay; the scaling gives a
	* notional fractional pkt count to help the accuracy
	*/
	rel_stats->data.total_scaled_ncount += (ncount << TAF_PKTCOUNT_SCALE_SHIFT);
	rel_stats->data.total_scaled_pcount += (pcount << TAF_PKTCOUNT_SCALE_SHIFT);
	rel_stats->data.total_bytes += release_bytes;
	rel_stats->data.total_release_units += release_units;

	TAF_ASSERT(rel_stats->data.total_scaled_ncount);

	if (rel_stats->data.total_bytes < (1 << (32 - TAF_PKTCOUNT_SCALE_SHIFT))) {
		TAF_ASSERT(rel_stats->data.total_scaled_ncount);
		mean = rel_stats->data.total_bytes << TAF_PKTCOUNT_SCALE_SHIFT;
		mean /= (rel_stats->data.total_scaled_ncount);
	} else {
		uint64 temp = (uint64)(rel_stats->data.total_bytes);

		temp = temp << TAF_PKTCOUNT_SCALE_SHIFT;
		mean = taf_div64(temp, rel_stats->data.total_scaled_ncount);
	}
	rel_stats->data.pkt_size_mean[TAF_TYPE_DL] = mean;
	TAF_DPSTATS_LOG_SET(rel_stats, pkt_size_mean, mean);
}

#if TAF_ENABLE_UL
static INLINE void BCMFASTPATH
taf_ias_ul_sched_upd(taf_scb_cubby_t *scb_taf, taf_ias_uni_state_t *uni_state,
	taf_ias_sched_rel_stats_t* rel_stats, uint32 timestamp, taf_ias_coeff_t* decay)
{
	uint32 release_units = uni_state->data.release_units;
	uint32 release_bytes = uni_state->data.release_bytes;
	uint32 elapsed = 0;
	int32 time_correction = 0;
	uint32 coeff;

	if (release_units == 0) {
		return;
	}

	elapsed = timestamp - rel_stats->data.ul.calc_timestamp;
	coeff = taf_ias_decay_score(TAF_COEFF_IAS_MAX, decay, elapsed, &time_correction);

	if (coeff < TAF_COEFF_IAS_MAX) {
		rel_stats->data.ul.calc_timestamp = timestamp - time_correction;

		TAF_IAS_DECAY64(coeff, rel_stats->data.ul.total_units);
		TAF_IAS_DECAY64(coeff, rel_stats->data.ul.total_bytes);
		TAF_IAS_DECAY64(coeff, rel_stats->data.ul.sched_count);
#ifdef TAF_DBG
		TAF_IAS_DECAY64(coeff, rel_stats->data.ul.update_count);
#endif
	}

	rel_stats->data.ul.total_bytes += release_bytes;
	rel_stats->data.ul.total_units += release_units;
	rel_stats->data.ul.sched_count += (1 << TAF_APKTCOUNT_SCALE_SHIFT);
}
#endif /* TAF_ENABLE_UL */

static INLINE bool BCMFASTPATH
taf_ias_sched_update(taf_method_info_t *method, taf_schedule_state_t op_state,
	taf_scb_cubby_t *scb_taf, int tid, taf_ias_uni_state_t *uni_state, taf_list_type_t type)
{
	taf_ias_sched_data_t* ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);
	taf_ias_sched_rel_stats_t* rel_stats = &ias_sched->rel_stats;
	uint32 new_score = 0;
#if TAF_ENABLE_SQS_PULL
	uint32 new_pscore = 0;
#endif

	/* clear the force status */
	scb_taf->info.force &= ~(1 << tid);

#if TAF_ENABLE_SQS_PULL
	if (uni_state->data.virtual_units > 0) {
	} else
#endif
	if (uni_state->data.release_units > 0) {
	} else {
		return FALSE;
	}

	if (uni_state->data.release_units > 0 &&
		method->taf_info->scheduler_index != rel_stats->index[type]) {
		TAF_DPSTATS_LOG_ADD(rel_stats, release_frcount, 1);
		rel_stats->index[type] = method->taf_info->scheduler_index;
		/* record the release time */
		scb_taf->timestamp[type] = TAF_IAS_NOWTIME(method);
	}

	/* These following very few lines of code, encompass the entire difference between
	 * EBOS, ATOS and ATOS2. The concept expressed here is critical to the essential behaviour,
	 * and nothing much else is different in any way.
	 * In every other respect, EBOS, ATOS and ATOS2 are identical. ATOS2 behaviour happens just
	 * as a consequence of coming after ATOS.
	 */

	/* NOTE: this score is global across all tid */
	switch (method->type) {
		case TAF_EBOS:
			break;

		case TAF_ATOS:
		case TAF_ATOS2:
			if (uni_state->data.release_units) {
				/* this is scheduled airtime scoring */
				new_score = taf_ias_units_to_score(method, scb_taf, tid,
					scb_taf->info.tech_type[type][tid],
					uni_state->data.release_units);

				scb_taf->score[type] =
					MIN(TAF_TYPE_SCORE(scb_taf, type) + new_score,
					TAF_SCORE_IAS_MAX);
			}
#if TAF_ENABLE_SQS_PULL
			if (uni_state->data.virtual_units) {
				/* this is scheduled airtime scoring */
				new_pscore = taf_ias_units_to_score(method, scb_taf, tid,
					scb_taf->info.tech_type[type][tid],
					uni_state->data.virtual_units);

				scb_taf->pscore =
					MIN(scb_taf->pscore + new_pscore, TAF_SCORE_IAS_MAX);
				WL_TAFM3(method, MACF" tid %u, score = %u, pscore = %u\n",
					TAF_ETHERC(scb_taf), tid,
					scb_taf->score[TAF_TYPE_DL], scb_taf->pscore);
			}
#endif
			break;

		case TAF_PSEUDO_RR:
			/* not supported */
		default:
			TAF_ASSERT(0);
	}

	if (uni_state->data.release_units) {
		if (type == TAF_TYPE_DL) {
			taf_ias_mean_pktlen(scb_taf, uni_state, rel_stats,
				scb_taf->timestamp[TAF_TYPE_DL], taf_ias_coeff_p(method));
		}
#if TAF_ENABLE_UL
		else {
			taf_ias_ul_sched_upd(scb_taf, uni_state, rel_stats,
				scb_taf->timestamp[TAF_TYPE_UL], taf_ias_coeff_p(method));
		}
#endif
		TAF_DPSTATS_LOG_ADD(rel_stats, release_pcount, uni_state->data.release_pcount);
		TAF_DPSTATS_LOG_ADD(rel_stats, release_ncount, uni_state->data.release_ncount);
		TAF_DPSTATS_LOG_ADD(rel_stats, release_time,
			(uint64)TAF_UNITS_TO_MICROSEC(uni_state->data.release_units));
		TAF_DPSTATS_LOG_ADD(rel_stats, release_bytes,
			(uint64)uni_state->data.release_bytes);

		WL_TAFM2(method, MACF"%s tid %u rate %7u, rel %3u pkts (%3u npkts) %4u us, "
			"total %5u us\n",
			TAF_ETHERC(scb_taf), TAF_TYPE(type), tid,
#if TAF_ENABLE_MU_TX
			scb_taf->info.scb_stats.global.rdata.rate
			[scb_taf->info.scb_stats.ias.data.ridx_used[type]].rate,
#else
			scb_taf->info.scb_stats.global.rdata.rate[TAF_RSPEC_SU_DL].rate,
#endif
			uni_state->data.release_pcount, uni_state->data.release_ncount,
			TAF_UNITS_TO_MICROSEC(uni_state->data.release_units),
			TAF_UNITS_TO_MICROSEC(uni_state->total.released_units));
	}

	/* reset before next scheduler cycle */
	memset(&uni_state->data, 0, sizeof(uni_state->data));

	return TRUE;
}

static INLINE void BCMFASTPATH taf_ias_sched_rate(taf_scb_cubby_t *scb_taf, int tid,
	taf_rspec_index_t rindex, taf_source_type_t s_idx, taf_ias_sched_data_t* ias_sched,
	taf_release_context_t *context)
{
	taf_ias_get_pktrate(scb_taf, tid, TAF_GET_AGG(ias_sched->aggsf), rindex, s_idx,
		&context->public.ias.pkt_rate, &context->public.ias.byte_rate);
}

static INLINE void BCMFASTPATH taf_ias_sched_clear(taf_release_context_t *context)
{
	context->public.how = TAF_RELEASE_LIKE_IAS;

#if TAF_ENABLE_SQS_PULL
	context->public.ias.virtual.released_units = 0;
	context->public.ias.virtual.release = 0;
#endif /* TAF_ENABLE_SQS_PULL */
	context->public.ias.actual.released_units = 0;
	context->public.ias.actual.released_bytes = 0;
	context->public.ias.actual.release = 0;

	context->public.ias.timestamp = context->now_time;
}

static INLINE bool BCMFASTPATH
taf_ias_tracking_end(taf_method_info_t *method, taf_scb_cubby_t *scb_taf, taf_list_type_t type,
	taf_ias_sched_data_t* ias_sched)
{
	bool limited = FALSE;
#ifdef TAF_LOGL2
	char * msg = "";

	BCM_REFERENCE(msg);
#endif

	if (!limited && ias_sched->release_config[type].done) {
#ifdef TAF_LOGL2
		msg = "release";
#endif
		limited = TRUE;
	}

	if (limited) {
		WL_TAFM2(method, MACF"%s %s limit reached\n",
			TAF_ETHERC(scb_taf), TAF_TYPE(type), msg);
	}

	return limited;
}

static INLINE void BCMFASTPATH
taf_ias_init_tracking(taf_method_info_t *method, taf_scb_cubby_t *scb_taf,
	taf_ias_sched_data_t* ias_sched, taf_schedule_state_t op_state, bool force,
	taf_list_type_t type, taf_release_context_t *context)
{
	wlc_taf_info_t* taf_info = method->taf_info;
#if TAF_ENABLE_SQS_PULL
	const bool init_tracking = (op_state == TAF_SCHEDULE_VIRTUAL_PACKETS);
#else
	const bool init_tracking = TRUE;
#endif

	/* init the scb tracking for release limiting */
	if (init_tracking) {
		if (ias_sched->release_config[type].units == 0) {

			if (force && type == TAF_TYPE_DL) {
				ias_sched->release_config[type].units =
					TAF_MICROSEC_TO_UNITS(taf_info->force_time);
			} else {
				ias_sched->release_config[type].units =
					TAF_MICROSEC_TO_UNITS(ias_cfg(method).release_limit);
			}
		}
	}
	context->public.ias.released_units_limit = ias_sched->release_config[type].units;
	context->public.ias.pairing_id = ias_sched->release_config[type].pairing_id;
	context->public.ias.mu_pair_count = TAF_BASE1_GET(ias_sched->release_config[type].mu);

	WL_TAFM3(method, MACF"%s tracking init %u\n", TAF_ETHERC(scb_taf), TAF_TYPE(type),
		context->public.ias.released_units_limit);
}

static INLINE void BCMFASTPATH
taf_ias_upd_tracking(taf_method_info_t *method, taf_scb_cubby_t *scb_taf,
	taf_ias_sched_data_t* ias_sched, taf_ias_uni_state_t* uni_state, taf_list_type_t type)
{
	if (ias_sched->release_config[type].units > 0) {
		if (ias_sched->release_config[type].units > uni_state->data.release_units) {
			ias_sched->release_config[type].units -=
				uni_state->data.release_units;
		} else {
			/* flag release limit already reached */
			ias_sched->release_config[type].done = 1;
			ias_sched->release_config[type].units = 0;
		}
	}
}

static INLINE uint32 BCMFASTPATH taf_ias_real_packet_count(taf_scb_cubby_t *scb_taf, int tid,
	taf_list_type_t type)
{
	taf_source_type_t s_idx;
	uint32 count = 0;

	for (s_idx = TAF_FIRST_REAL_SOURCE; s_idx < TAF_NUM_REAL_SOURCES; s_idx++) {
		TAF_ASSERT(TAF_SOURCE_IS_REAL(s_idx));

		if (!taf_src_type_match(s_idx, type)) {
			continue;
		}
		count += scb_taf->info.traffic.count[s_idx][tid];
	}
	return count;
}

static INLINE bool BCMFASTPATH
taf_ias_sched_send(taf_method_info_t *method, taf_scb_cubby_t *scb_taf, int tid,
	taf_ias_uni_state_t *uni_state, taf_ias_sched_data_t* ias_sched,
	taf_release_context_t *context, taf_schedule_state_t op_state, taf_list_type_t type,
        bool agg_hold)
{
	wlc_taf_info_t* taf_info = method->taf_info;
#if TAF_ENABLE_SQS_PULL
	const taf_source_type_t start_source = TAF_FIRST_REAL_SOURCE;
	const taf_source_type_t end_source = (op_state == TAF_SCHEDULE_VIRTUAL_PACKETS) ?
		TAF_NUM_SCHED_SOURCES : TAF_NUM_REAL_SOURCES;
	uint16 v_avail = scb_taf->info.traffic.count[TAF_SOURCE_HOST_SQS][tid];
	uint32 v_prior_released = 0;
	uint32 real_prior_released = 0;
#else /* TAF_ENABLE_SQS_PULL */
	const taf_source_type_t start_source = TAF_FIRST_REAL_SOURCE;
	const taf_source_type_t end_source = TAF_NUM_REAL_SOURCES;
#endif /* TAF_ENABLE_SQS_PULL */
	const bool force = (scb_taf->info.force & (1 << tid)) ? (taf_info->force_time > 0) : FALSE;
	taf_source_type_t s_idx;
	taf_rspec_index_t rindex;
	taf_ias_sched_rel_stats_t* rel_stats = &ias_sched->rel_stats;

	if (taf_ias_tracking_end(method, scb_taf,  type, ias_sched)) {
		/* have to return TRUE here (as if emptied) so IAS continues with next station */
		return TRUE;
	}
	taf_ias_sched_clear(context);

	rindex = taf_ias_get_rindex(scb_taf, type, scb_taf->info.tech_type[type][tid]);

	context->public.type = taf_tech_to_mutype(scb_taf->info.tech_type[type][tid]);
	uni_state->release_type_present |= (1 << scb_taf->info.tech_type[type][tid]);
	uni_state->bw_type_present |= (1 << taf_bw_idx(scb_taf));

	scb_taf->info.scb_stats.ias.data.ridx_used[type] = rindex;

	context->public.ias.estimated_pkt_size_mean = rel_stats->data.pkt_size_mean[type];

	if (context->public.ias.estimated_pkt_size_mean == 0) {
		context->public.ias.estimated_pkt_size_mean = (type == TAF_TYPE_DL) ?
			TAF_PKT_SIZE_DEFAULT_DL : TAF_PKT_SIZE_INIT_UL(rel_stats);
	}

	if (!agg_hold) {
		taf_ias_init_tracking(method, scb_taf, ias_sched, op_state, force, type, context);
	}

	for (s_idx = start_source; s_idx < end_source; s_idx++) {
		bool emptied;
		uint32 pending;

		TAF_ASSERT(s_idx >= 0);

		if (!(scb_taf->info.scb_stats.ias.data.use[s_idx])) {
			continue;
		}
		if (scb_taf->info.linkstate[s_idx][tid] != TAF_LINKSTATE_ACTIVE) {
			continue;
		}
		if (!taf_src_type_match(s_idx, type)) {
			continue;
		}

#if TAF_ENABLE_SQS_PULL
		if (op_state == TAF_SCHEDULE_VIRTUAL_PACKETS) {
			const bool is_mu = scb_taf->info.tech_type[type][tid] <
				TAF_NUM_MU_TECH_TYPES;

			if ((pending = scb_taf->info.traffic.count[s_idx][tid]) == 0) {
				continue;
			}

			/* multi-stage if to make it less complex to handle real packets
			 * already available during virtual phase
			 */
			if (TAF_SOURCE_IS_SQS(s_idx)) {
				v_prior_released = context->virtual_release;
				/* do release */
			} else if (agg_hold) {
				/* For IAS purpose, consider this is emptied */
				scb_taf->info.traffic.map[s_idx] &= ~(1 << tid);
				continue;
			} else if (TAF_SOURCE_IS_UL(s_idx)) {
				/* do release */
			} else if (TAF_SOURCE_IS_NAR(s_idx) || TAF_SOURCE_IS_PSQ(s_idx)) {
				/* do release */
			} else if (scb_taf->info.ps_mode || force || (v_avail == 0) ||
				((pending >= ias_cfg(method).pre_rel_limit[s_idx]) && !is_mu)) {
				/* do release */
			} else if (SCB_PKTS_INFLT_FIFOCNT_VAL(scb_taf->scb, tid) == 0 && !is_mu) {
				/* do release */
			} else {
				WL_TAFM3(method, "skip real vphase rel to "MACF" tid %u %s "
					"only %u pending (thresh %u), v_avail %u, ps %u, mu %u\n",
					TAF_ETHERC(scb_taf), tid, TAF_SOURCE_NAME(s_idx),
					pending, MIN(ias_cfg(method).pre_rel_limit[s_idx],
					scb_taf->info.max_pdu), v_avail,
					scb_taf->info.ps_mode, is_mu);

				/* For IAS purpose, consider this is emptied */
				scb_taf->info.traffic.map[s_idx] &= ~(1 << tid);
				continue;
			}

			if (TAF_SOURCE_IS_REAL(s_idx) && TAF_SOURCE_IS_DL(s_idx)) {
				if (real_prior_released == 0) {
					real_prior_released = context->actual_release;
				}
				WL_TAFM2(method, "try real vphase rel to "MACF"%s tid %u %s "
					"%u pending, v_avail %u, ps %u "
					"force %u tx-in-transit %u, mu %u\n",
					TAF_ETHERC(scb_taf), TAF_TYPE(type), tid,
					TAF_SOURCE_NAME(s_idx), pending, v_avail,
					scb_taf->info.ps_mode, force,
					SCB_PKTS_INFLT_FIFOCNT_VAL(scb_taf->scb, tid), is_mu);
			}
		}
#else
		if ((pending = scb_taf->info.traffic.count[s_idx][tid]) == 0) {
			WL_TAFM2(method, MACF"%s tid %u %s pending 0\n",
				TAF_ETHERC(scb_taf), TAF_TYPE(type), tid, TAF_SOURCE_NAME(s_idx));
			continue;
		}
#endif /* TAF_ENABLE_SQS_PULL */

		taf_ias_sched_rate(scb_taf, tid, rindex, s_idx, ias_sched, context);

		emptied = taf_ias_send_source(method, scb_taf, tid, s_idx, uni_state, ias_sched,
			context, type);

#if TAF_ENABLE_SQS_PULL
		/* handle the case that no virtual packets can be pulled and we skipped existing
		 * real packets available; if so, go back to release the real packets
		 */
		if (TAF_SOURCE_IS_SQS(s_idx) && (v_avail > 0) &&
			(context->virtual_release - v_prior_released == 0) &&
			(context->actual_release - real_prior_released == 0) &&
			(pending = taf_ias_real_packet_count(scb_taf, tid, TAF_TYPE_DL)) != 0 &&
			!scb_taf->info.ps_mode && !agg_hold &&
			(SCB_PKTS_INFLT_FIFOCNT_VAL(scb_taf->scb, tid) == 0)) {

				/* no virtual traffic released & no real traffic was pre-released */
				WL_TAFM1(method, MACF"%s tid %u pre-release not completed and "
					"without virtual rel (virtual avail %u, real avail %u); "
					"try real packets once again....\n",
					TAF_ETHERC(scb_taf), TAF_TYPE(type), tid, v_avail,
					pending);

				scb_taf->info.traffic.count[s_idx][tid] = 0;
				scb_taf->info.traffic.map[s_idx] &= ~(1 << tid);
				v_avail = 0;
				/* go around again... NB: loop counter s_idx will be ++ */
				s_idx = start_source - 1;
				continue;
			}
#endif /* TAF_ENABLE_SQS_PULL */

		if (emptied) {
			scb_taf->info.traffic.map[s_idx] &= ~(1 << tid);
		}

		if (TAF_SOURCE_IS_REAL(s_idx)) {
			taf_ias_upd_tracking(method, scb_taf, ias_sched, uni_state, type);

			if (taf_ias_tracking_end(method, scb_taf, type, ias_sched)) {
				/* release tracking completed */
				return TRUE;
			}
		}
	}
	scb_taf->info.traffic.available[type] = 0;
	for (s_idx = TAF_FIRST_REAL_SOURCE; s_idx < TAF_NUM_SCHED_SOURCES; s_idx++) {
		if (!taf_src_type_match(s_idx, type)) {
			continue;
		}
		scb_taf->info.traffic.available[TAF_SRC_TO_TYPE(s_idx)] |=
			scb_taf->info.traffic.map[s_idx];
	}
	/* return whether SCB, TID was emptied (TRUE) across all sources (or not) */
	return (scb_taf->info.traffic.available[type] & (1 << tid)) == 0;
}

static INLINE uint32 BCMFASTPATH
taf_ias_psq_pend(taf_scb_cubby_t *scb_taf)
{
	return wlc_apps_get_taf_scb_pktq_tot(TAF_WLCC(scb_taf), scb_taf->scb);
}

static INLINE taf_ias_sched_data_t* BCMFASTPATH
taf_ias_sched_valid(taf_scb_cubby_t *scb_taf, taf_list_type_t type, int tid)
{
	taf_ias_sched_data_t* ias_sched = NULL;

	if (TAF_PRIO_SUSPENDED(scb_taf, tid, type)) {
		WL_TAFM2(scb_taf->method, MACF"%s tid %u, suspended\n",
			TAF_ETHERC(scb_taf), TAF_TYPE(type), tid);
		goto valid_end;
	}

	if (SCB_MARKED_DEL(scb_taf->scb) || SCB_DEL_IN_PROGRESS(scb_taf->scb)) {
		WL_TAFM4(scb_taf->method, MACF"%s tid %u, deletion\n",
			TAF_ETHERC(scb_taf), TAF_TYPE(type), tid);
		goto valid_end;
	}

	if (!(scb_taf->info.traffic.available[type] & (1 << tid))) {
		WL_TAFM4(scb_taf->method, MACF"%s tid %u, no traffic\n",
			TAF_ETHERC(scb_taf), TAF_TYPE(type), tid);
		goto valid_end;
	}

	if (!(ias_sched = TAF_IAS_TID_STATE(scb_taf, tid))) {
		WL_TAFM4(scb_taf->method, MACF"%s tid %u, no context\n",
			TAF_ETHERC(scb_taf), TAF_TYPE(type), tid);
	}

	if (scb_taf->info.ps_mode) {
		uint32 count;

		BCM_REFERENCE(count);
		TAF_DPSTATS_LOG_ADD(&ias_sched->rel_stats, pwr_save, 1);

		if (type == TAF_TYPE_DL && (count = taf_ias_psq_pend(scb_taf)) > 0) {
			wlc_info_t *wlc = TAF_WLCM(scb_taf->method);
			struct scb* scb = scb_taf->scb;

			/* Check packet AC is delivery-enabled or legacy. */
			if (AC_BITMAP_TST(scb->apsd.ac_delv, prio2ac[tid])) {
				/* If APSD Unscheduled Service Period in progress,
				 * max packets in psq should be apsd_cnt to
				 * satisfy the apsd trigger process.
				 */
				if (wlc_apps_psq_delv_count(wlc, scb) >=
					wlc_apps_scb_apsd_cnt(wlc, scb_taf->scb)) {
					ias_sched = NULL;
				}
			} else {
				if (wlc_apps_psq_ndelv_count(wlc, scb)) {
					ias_sched = NULL;
				}
			}

			if (!ias_sched) {
				WL_TAFM2(scb_taf->method, MACF"%s tid %u, power save %u pending\n",
					TAF_ETHERC(scb_taf), TAF_TYPE(type), tid, count);
			}
		} else {
			WL_TAFM4(scb_taf->method, MACF"%s tid %u, power save\n",
				TAF_ETHERC(scb_taf), TAF_TYPE(type), tid);
		}
	}

valid_end:
	if (!ias_sched || scb_taf->info.ps_mode) {
		/* cannot force so clear the force status */
		scb_taf->info.force &= ~(1 << tid);
	}
	return ias_sched;
}

static bool BCMFASTPATH
taf_ias_sched_scb(taf_method_info_t *method, taf_ias_uni_state_t *uni_state,
	int tid, taf_release_context_t *context, taf_schedule_state_t op_state)
{
	uint32 highunits = TAF_MICROSEC_TO_UNITS(uni_state->high[IAS_INDEXM(method)]);
	uint32 lowunits = TAF_MICROSEC_TO_UNITS(uni_state->low[IAS_INDEXM(method)]);
	taf_list_t *list;
#if TAF_ENABLE_SQS_PULL && TAF_ENABLE_TIMER
	const bool agg_hold = taf_ias_agghold_is_active(TAF_IAS_GROUP_INFO(method->taf_info));
#else
	const bool agg_hold = FALSE;

#endif
#if defined(TAF_DBG) && TAF_ENABLE_MU_TX
	taf_tech_type_t tech = TAF_TECH_DL_HEMUMIMO;

	for (tech = TAF_TECH_DL_HEMUMIMO; tech < TAF_NUM_MU_TECH_TYPES; tech++) {
		uint8 active = ias_cfg(method).active_mu_count[tech][tid];

		if (active) {
			WL_TAFM2(method, "%s tid %u: %u active stations\n",
				TAF_TECH_NAME(tech), tid, active);
		}
	}
#endif
	for (list = method->list; list; list = list->next) {
		taf_scb_cubby_t *scb_taf = list->scb_taf;
		taf_list_type_t type = list->type;
		taf_ias_sched_data_t* ias_sched;
		bool emptied;
		bool unassigned = TRUE;

		TAF_ASSERT((list->used & (1 << tid)) == 0);

		if (!(ias_sched = taf_ias_sched_valid(scb_taf, type, tid))) {
			continue;
		}

#if TAF_ENABLE_MU_TX
		unassigned = (scb_taf->info.tech_type[type][tid] == TAF_TECH_UNASSIGNED);
#endif
		if (!agg_hold && TAF_OPT(method, EXIT_EARLY) && method->type == TAF_ATOS &&
			unassigned &&
			uni_state->total.released_units > (lowunits + TAF_IAS_EXTEND(uni_state))) {

			uint32 traffic_next = scb_taf->info.traffic.est_units[type][tid];
			uint32 limit = taf_ias_window_remain(method, uni_state, type);

			if (traffic_next > limit) {

				WL_TAFM1(method, MACF"%s tid %u has %u traffic, space remaining %u,"
					" exit cycle now\n", TAF_ETHERC(scb_taf),
					TAF_TYPE(type), tid, traffic_next, limit);
				return TRUE;
			}
		}

#if TAF_ENABLE_MU_TX
		if (!agg_hold) {
			taf_ias_analyse_mu(method, list, uni_state, tid);

			if (scb_taf->info.tech_type[type][tid] == TAF_TECH_DONT_ASSIGN) {
				WL_TAFM1(method,
					"skipping "MACF"%s tid %u due to MU optimisation\n",
					TAF_ETHERC(scb_taf), TAF_TYPE(type), tid);
				continue;
			}
		}
#endif /* TAF_ENABLE_MU_TX */
		if (scb_taf->info.tech_type[type][tid] == TAF_TECH_UNASSIGNED) {
			if (type == TAF_TYPE_DL) {
#if TAF_ENABLE_MU_TX
				/* MU station is being sent as SU, update mu status count */
				if (!scb_taf->info.ps_mode && scb_taf->info.tech_enable_mask &&
					(ias_sched->used & (1 << TAF_SOURCE_AMPDU))) {

					taf_tech_type_t tt;

					for (tt = 0; tt <= TAF_TECH_DL_OFDMA; tt++) {
						if ((scb_taf->info.mu_tech_en[tt] & (1 << tid)) &&
							ias_cfg(method).active_mu_count[tt][tid]) {
							--ias_cfg(method).active_mu_count[tt][tid];
						}
					}
				}
#endif
				scb_taf->info.tech_type[TAF_TYPE_DL][tid] = TAF_TECH_DL_SU;

				WL_TAFM3(method, MACF"%s tid %u to be released like SU\n",
					TAF_ETHERC(scb_taf), TAF_TYPE(TAF_TYPE_DL), tid);
			}
#if TAF_ENABLE_UL
			if (type == TAF_TYPE_UL && !agg_hold) {
				scb_taf->info.tech_type[TAF_TYPE_UL][tid] = TAF_TECH_UL_OFDMA;
				ias_sched->release_config[TAF_TYPE_UL].pairing_id =
					++uni_state->pairing_id;

				TAF_BASE1_SET(ias_sched->release_config[TAF_TYPE_UL].mu, 1);

				if (uni_state->last_paired[TAF_TYPE_UL] == 0) {
					uni_state->last_paired[TAF_TYPE_UL] =
						uni_state->pairing_id;
				}
				if (ias_cfg(method).active_mu_count[TAF_TECH_UL_OFDMA][tid]) {
					--ias_cfg(method).active_mu_count[TAF_TECH_UL_OFDMA][tid];
				}

				WL_TAFM3(method, MACF"%s tid %u released like 1-MU "
					"(pairing %u)\n",
					TAF_ETHERC(scb_taf), TAF_TYPE(TAF_TYPE_UL), tid,
					ias_sched->release_config[TAF_TYPE_UL].pairing_id);
			}
#endif /* TAF_ENABLE_UL */
		}

#if TAF_ENABLE_UL
		if ((type == TAF_TYPE_UL) &&
			(ias_sched->release_config[TAF_TYPE_UL].pairing_id != 0) &&
			(ias_sched->release_config[TAF_TYPE_UL].pairing_id !=
			uni_state->last_paired[TAF_TYPE_UL])) {

			taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
			bool flush = FALSE;

			WL_TAFM4(method, MACF" tid %u pairing %u, last pairing %u\n",
				TAF_ETHERC(scb_taf), tid,
				ias_sched->release_config[TAF_TYPE_UL].pairing_id,
				uni_state->last_paired[TAF_TYPE_UL]);

			if (uni_state->free_release) {
				if (TAF_IAS_OPT(ias_info, FLUSH_FREE_REL)) {
					flush = TRUE;
				}
			} else {
				if (TAF_IAS_OPT(ias_info, FLUSH_PAIRING)) {
					flush = TRUE;
				}
			}
			if (flush) {
				taf_flush_source(method->taf_info, TAF_SOURCE_UL, tid, TRUE);

				if (TAF_BASE1_GET(ias_sched->release_config[TAF_TYPE_UL].mu) == 1) {
					/* remember timestamp for last MU 1 release */
					ias_sched->last_isolate = TAF_IAS_NOWTIME(method);
				}
			}
			uni_state->last_paired[TAF_TYPE_UL] =
				ias_sched->release_config[TAF_TYPE_UL].pairing_id;
		}
#endif /* TAF_ENABLE_UL */

		if (!agg_hold ||
#if TAF_ENABLE_SQS_PULL
			/* pull virtual packets whilst agg hold wait */
			type == TAF_TYPE_DL) {
#else
			FALSE) {
#endif
			emptied = taf_ias_sched_send(method, scb_taf, tid, uni_state, ias_sched,
				context, op_state, type, agg_hold);
		}

		if (!agg_hold &&
			taf_ias_sched_update(method, op_state, scb_taf, tid, uni_state, type)) {

			if (method->ordering == TAF_LIST_SCORE_MINIMUM) {
				list->used |= (1 << tid);
				uni_state->intermediate_sort = (1 << tid);
			}
		}

		if (!emptied) {
			WL_TAFM3(method, MACF"%s tid %u not emptied\n",
				TAF_ETHERC(scb_taf), TAF_TYPE(type), tid);
			return TRUE;
		}
		if (uni_state->total.released_units >= (highunits + TAF_IAS_EXTEND(uni_state))) {
			WL_TAFM3(method, "release window completed (%u, %u)\n",
				uni_state->total.released_units,
				(highunits + TAF_IAS_EXTEND(uni_state)));
			return TRUE;
		}
	}
	return FALSE;
}

static bool BCMFASTPATH
taf_ias_sched_all_scb(taf_method_info_t *method, taf_ias_uni_state_t *uni_state,
	int tid, taf_release_context_t *context)
{
	taf_schedule_state_t op_state = context->op_state;
	wlc_taf_info_t* taf_info = method->taf_info;

	if (taf_info->bulk_commit) {
		taf_open_all_sources(taf_info, tid);
	}

	return taf_ias_sched_scb(method, uni_state, tid, context, op_state);
}

static void taf_ias_time_settings_sync(taf_method_info_t* method)
{
	wlc_taf_info_t* taf_info = method->taf_info;
	taf_ias_uni_state_t* uni_state = taf_get_uni_state(taf_info);

	WL_TAFM1(method, "high = %us, low = %uus\n", ias_cfg(method).high, ias_cfg(method).low);

	uni_state->high[IAS_INDEXM(method)] = ias_cfg(method).high;
	uni_state->low[IAS_INDEXM(method)] = ias_cfg(method).low;
}

static INLINE bool BCMFASTPATH
taf_ias_schedule_all(taf_method_info_t* method, taf_ias_uni_state_t *uni_state,
	int tid_index_start, int tid_index_end, taf_release_context_t* context)
{
	bool finished = FALSE;
	int tid_index = tid_index_start;

	uni_state->intermediate_sort = 0;

	WL_TAFM2(method, "total est units %u (total in transit "
#if TAF_ENABLE_UL
		"%u + %u = "
#endif
		"%u)\n",
		uni_state->est_release_units,
#if TAF_ENABLE_UL
		TAF_IAS_TUIT(uni_state, TAF_TYPE_DL), TAF_IAS_TUIT(uni_state, TAF_TYPE_UL),
#endif
		TAF_IAS_TUIT(uni_state, TAF_TYPE_ALL));

	do {
		int tid = tid_service_order[tid_index];

		if (ias_cfg(method).sched_active & (1 << tid)) {
			if (uni_state->intermediate_sort && TAF_OPT(method, TID_RE_SORT)) {

				WL_TAFM2(method, "intermediate list sort now tid %d, prev tid %d\n",
					tid, taf_bitpos(uni_state->intermediate_sort));
				taf_ias_list_separate_sort(method);
			}
			uni_state->intermediate_sort = 0;
			finished = taf_ias_sched_all_scb(method, uni_state, tid, context);
		}
		if (!finished) {
			tid_index++;
		}
	} while (!finished && tid_index <= tid_index_end);

	return finished;
}

static INLINE BCMFASTPATH bool
taf_ias_super_resched(wlc_taf_info_t* taf_info, taf_ias_uni_state_t *uni_state,
	uint32 low_units, uint32 high_units, taf_release_context_t *context)
{
	bool extra_cycle = FALSE;
	uint32 real_release;
#if TAF_ENABLE_MU_TX
	bool super = (taf_info->super & uni_state->release_type_present) != 0;
#else
	bool super = taf_info->super;
#endif

	if (!super &&
		((uni_state->bw_type_present &
		((1 << D11_REV128_BW_160MHZ) | (1 << D11_REV128_BW_80MHZ))) == 0)) {

		taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);

		if ((uni_state->bw_type_present & (1 << D11_REV128_BW_40MHZ)) &&
			TAF_IAS_OPT(ias_info, AUTOBW40_SUPER)) {

			super = TRUE;
		} else if ((uni_state->bw_type_present & (1 << D11_REV128_BW_20MHZ)) &&
			TAF_IAS_OPT(ias_info, AUTOBW20_SUPER)) {

			super = TRUE;
		}
	}

	if (taf_info->super_active) {
		if (super) {
			uni_state->super_sched_last = context->now_time;

		} else if (uni_state->cycle_next == 0 && uni_state->cycle_now == 0) {
			taf_info->super_active = FALSE;
			uni_state->super_sched_last = 0;

		} else if (uni_state->super_sched_last != 0) {
			uint32 elapsed = context->now_time - uni_state->super_sched_last;
			uint32 limit = TAF_UNITS_TO_MICROSEC(high_units) * TAF_MAX_NUM_WINDOWS;

			if (elapsed > limit) {
				taf_info->super_active = FALSE;
				uni_state->super_sched_last = 0;
				WL_TAFT2(taf_info, "super scheduling to end due to %uus elapsed "
					"with ineligible traffic\n", elapsed);
			}
		} else {
			taf_info->super_active = FALSE;
		}
	}
	if (super && uni_state->cycle_next == 0 && uni_state->cycle_now == 0 &&
		(real_release = TAF_IAS_REAL(uni_state, TAF_TYPE_ALL)) >= low_units &&
		uni_state->barrier_req == 0 && context->status == TAF_CYCLE_INCOMPLETE) {

		uint32 est_traffic = uni_state->est_release_units;

		if (est_traffic > real_release) {
			est_traffic -= real_release;
		} else {
			est_traffic = 0;
		}

		if (est_traffic > high_units) {
#if TAF_ENABLE_MU_TX
			WL_TAFT2(taf_info, "do super reschedule (0x%x, 0x%x, %u)\n",
				taf_info->super, uni_state->release_type_present, est_traffic);
#else
			WL_TAFT2(taf_info, "do super reschedule (%u)\n", est_traffic);
#endif
			/* insert an extra cycle */
			extra_cycle = TRUE;
			taf_info->super_active = TRUE;
			uni_state->super_sched_last = context->now_time;
			uni_state->cycle_ready = 0;
		}
	}
	return extra_cycle;
}

static INLINE BCMFASTPATH bool
taf_ias_completed(taf_method_info_t *method, taf_release_context_t *context,
	taf_ias_uni_state_t *uni_state, bool finished, taf_schedule_state_t op_state)
{
	wlc_taf_info_t* taf_info = method->taf_info;
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);
	uint32 duration;
	uint32 real_release = TAF_IAS_REAL(uni_state, TAF_TYPE_ALL);
	uint32 tuit = TAF_IAS_TUIT(uni_state, TAF_TYPE_ALL);
#if TAF_ENABLE_SQS_PULL
	bool virtual_scheduling = (op_state == TAF_SCHEDULE_VIRTUAL_PACKETS &&
		method->type != TAF_VIRTUAL_MARKUP);
	bool v2r_pending = (virtual_scheduling && (context->virtual_release > 0));
#endif /* TAF_ENABLE_SQS_PULL */

	if (!finished) {
		finished = (method->type == LAST_IAS_SCHEDULER);
	}

#if TAF_ENABLE_SQS_PULL
	if (virtual_scheduling) {
		if (finished) {
			if (v2r_pending) {
				taf_method_info_t *vmarkup = taf_get_method_info(method->taf_info,
					TAF_VIRTUAL_MARKUP);
				*vmarkup->ready_to_schedule = ~0;

				WL_TAFT1(taf_info, "vsched phase exit %uus real, %uus "
					"virtual (%u rpkts / %u vpkts / %u ppkts), "
					"TAF_VIRTUAL_MARKUP pulls %u\n",
					TAF_UNITS_TO_MICROSEC(real_release),
					TAF_UNITS_TO_MICROSEC(uni_state->total.released_units -
						real_release),
					context->actual_release, context->virtual_release,
					context->pending_release,
					method->taf_info->total_pull_requests);
			}
			uni_state->total.released_units = real_release;
			uni_state->extend.rounding_units = 0;
		}
		if (v2r_pending) {
#if TAF_ENABLE_UL
			taf_flush_source(taf_info, TAF_SOURCE_UL, TAF_UL_PRIO, FALSE);
#endif
			return finished;
		}
	} else if (method->type == TAF_VIRTUAL_MARKUP && finished) {
		TAF_ASSERT(!v2r_pending);
		*method->ready_to_schedule = 0;
	}
#endif /* TAF_ENABLE_SQS_PULL */

#if TAF_ENABLE_TIMER
	if (finished) {
		if (taf_ias_agghold_is_active(ias_info) && taf_ias_agghold_exp(ias_info)) {

			TAF_ASSERT(real_release == 0);

			taf_ias_agghold_stop(method->taf_info, ias_info, TAF_IAS_NOWTIME(method));
#if TAF_ENABLE_SQS_PULL
			uni_state->total.released_units = 0;
			memset(&uni_state->extend, 0, sizeof(uni_state->extend));
			finished = FALSE;
			context->op_state = taf_info->op_state;

			WL_TAFM1(method, "scheduling following aggregation timeout\n");
#endif /* TAF_ENABLE_SQS_PULL */
		} else {
			taf_ias_agghold_clear(ias_info);
		}
	}
#endif /* TAF_ENABLE_TIMER */

	if (finished) {
		duration = ias_info->sched_start != 0 ?
			(TAF_IAS_NOWTIME(method) - ias_info->sched_start) : 0;

		ias_info->debug.sched_duration += duration;

		if (duration > ias_info->debug.max_duration) {
			ias_info->debug.max_duration = duration;
		}
		ias_info->sched_start = 0;

		if (taf_info->bulk_commit) {
			taf_close_all_sources(taf_info, ALLPRIO);
		}
	}

	if (finished && (real_release > 0)) {
		uint32 low_units = TAF_MICROSEC_TO_UNITS(uni_state->low[IAS_INDEXM(method)]);
		uint32 high_units = TAF_MICROSEC_TO_UNITS(uni_state->high[IAS_INDEXM(method)]);
		uint8 cnext = uni_state->cycle_next;
		uint8 cnow = uni_state->cycle_now;
#if !TAF_ENABLE_SQS_PULL
		taf_method_info_t *scheduler = method;
#else
		taf_method_info_t *scheduler = taf_get_method_info(method->taf_info,
			TAF_SCHEDULER_START);
#endif /* !TAF_ENABLE_SQS_PULL */

		if (TAF_IAS_EXTEND(uni_state) > 0) {
			if (real_release > high_units) {
				high_units = MIN(real_release,
					high_units + TAF_IAS_EXTEND(uni_state));
			}
			if (real_release > low_units) {
				low_units += high_units -
					TAF_MICROSEC_TO_UNITS(uni_state->high[IAS_INDEXM(method)]);
			}
		}

		WL_TAFT3(taf_info, "high %uus, low %uus, extend %uus\n",
			TAF_UNITS_TO_MICROSEC(high_units),
			TAF_UNITS_TO_MICROSEC(low_units),
			TAF_UNITS_TO_MICROSEC(TAF_IAS_EXTEND(uni_state)));

		WL_TAFT2(taf_info, "tuit "
#if TAF_ENABLE_UL
			"%u + %u = "
#endif
			"%u\n",
#if TAF_ENABLE_UL
			TAF_IAS_TUIT(uni_state, TAF_TYPE_DL), TAF_IAS_TUIT(uni_state, TAF_TYPE_UL),
#endif
			tuit);

		if (real_release < high_units) {
			ias_info->data.uflow_high++;
			ias_info->debug.uflow_high++;
		} else {
			ias_info->data.target_high++;
			ias_info->debug.target_high++;
		}

		ias_info->debug.average_high += real_release;

		ias_info->data.time_delta =
			TAF_IAS_NOWTIME(method) - ias_info->data.prev_release_time;
		ias_info->data.prev_release_time = TAF_IAS_NOWTIME(method);

		if (TAF_IAS_OPT(ias_info, TOOMUCH) && (low_units > 0) &&
			(real_release < low_units) &&
			(uni_state->trigger == TAF_TRIGGER_IMMEDIATE) &&
			(tuit > (low_units + high_units)) &&
			(cnext == 0) && (cnow == 0)) {

			/* scheduler triggering runaway prevention */
			WL_TAFT1(taf_info, "too much immediate trig traffic in transit "
				"%uus, %s barrier (%u)\n",
				TAF_UNITS_TO_MICROSEC(tuit),
				uni_state->barrier_req == 0 ? "request" : "insert",
				uni_state->barrier_req);

			if (uni_state->barrier_req == 0) {
				uni_state->barrier_req = 1;
			} else {
				low_units = TAF_IAS_OPT(ias_info, TOOMUCH_LOOSE) ?
					real_release - 1 : 0;
				ias_info->data.uflow_low++;
				ias_info->debug.uflow_low++;
				memset(uni_state->total_sched_units, 0,
				       sizeof(uni_state->total_sched_units));
			}
		} else {
			uni_state->barrier_req = 0;
		}

		if (TAF_IAS_REAL(uni_state, TAF_TYPE_ALL) > low_units) {
			uni_state->resched_units[cnext] =
				real_release - low_units;
			if (uni_state->barrier_req == 0) {
				ias_info->data.target_low++;
				ias_info->debug.target_low++;
			} else {
				uni_state->barrier_req = 0;
			}
			if (cnext == 1) {
				ias_info->data.super_sched++;
				ias_info->debug.super_sched++;
			}
		} else {
			/* TX window underflow */
			ias_info->data.uflow_low++;
			ias_info->debug.uflow_low++;

			if (cnext == 1) {
				WL_TAFT1(taf_info, "cycle_next is 1 and low underflow, wait for "
					"all traffic to clear\n");
				/* this will wait to clear out entire data path to allow
				 * super-scheduling to resume again- prevent run-away
				 */
				uni_state->cycle_next = cnext = 0;
				uni_state->resched_units[0] = TAF_IAS_OPT(ias_info, SFAIL_LOOSE) ?
					1 : real_release;
				uni_state->barrier_req = 0;
				ias_info->data.super_sched_collapse++;
				ias_info->debug.super_sched_collapse++;
				ias_info->debug.immed_star++;
				ias_info->data.immed_star++;
				context->status = TAF_CYCLE_FAILURE;
			} else if (cnow == 1) {
				WL_TAFT1(taf_info, "cycle_now is 1 and low underflow\n");
				uni_state->resched_units[0] = 1;
			} else if (ias_info->ncount_flow < ias_info->immed) {
				/* infrequent, small volume data */
				uni_state->resched_units[0] = TAF_IAS_OPT(ias_info, IMMED_LOOSE) ?
					1 : real_release;
				WL_TAFT2(taf_info, "star pkt instead of immed trigger "
					"(%u, %u, %u)\n", ias_info->ncount_flow, ias_info->immed,
					uni_state->resched_units[0]);
				ias_info->debug.immed_star++;
				ias_info->data.immed_star++;
				uni_state->trigger = TAF_TRIGGER_IMMEDIATE_STAR;
			} else {
				/* immediate re-trigger mode */
				uni_state->resched_units[0] = 0;
				ias_info->debug.immed_trig++;
				ias_info->data.immed_trig++;
			}
		}

		uni_state->resched_index[cnext] = context->public.ias.index;

		if (uni_state->resched_units[cnext] > 0) {
			bool star_pkt_seen;
			uint32 cumul_units =
				TAF_IAS_CUMUL(uni_state, uni_state->resched_index[cnext],
				TAF_TYPE_ALL);

			star_pkt_seen = (cumul_units >= uni_state->resched_units[cnext]);

			if (taf_info->super_active && cnext && uni_state->cycle_ready == 0) {
				uni_state->cycle_ready = 1;
			}

			if (star_pkt_seen) {
				WL_TAFT1(taf_info, "final exit %uus released, trigger %u:%u "
					"already seen (prev rel is %u, in transit %u), sched dur "
					"%uus, r_t_s 0x%x\n\n",
					TAF_UNITS_TO_MICROSEC(real_release),
					uni_state->resched_index[cnext],
					uni_state->resched_units[cnext],
					ias_info->data.time_delta,
					TXPKTPENDTOT(TAF_WLCT(taf_info)),
					duration, *scheduler->ready_to_schedule);
				uni_state->trigger = TAF_TRIGGER_NONE;
			} else {
				bool extra_cycle =
					taf_ias_super_resched(taf_info, uni_state, low_units,
					high_units, context);

				*scheduler->ready_to_schedule = extra_cycle ? ~0 : 0;

				WL_TAFT1(taf_info, "final exit %uus released "
					"%s trig %u:%uus (prev rel is %u, in transit "
					"%u), sched dur %uus, r_t_s 0x%x%s\n\n",
					TAF_UNITS_TO_MICROSEC(real_release),
					cnext ? "next" : "now",
					uni_state->resched_index[cnext],
					TAF_UNITS_TO_MICROSEC(uni_state->resched_units[cnext]),
					ias_info->data.time_delta,
					TXPKTPENDTOT(TAF_WLCT(taf_info)),
					duration, *scheduler->ready_to_schedule,
					extra_cycle ? " RESCHED" : "");

				if (uni_state->trigger != TAF_TRIGGER_IMMEDIATE_STAR) {
					uni_state->trigger = extra_cycle ?
						TAF_TRIGGER_IMMEDIATE_SUPER_RESCHED :
						TAF_TRIGGER_STAR_THRESHOLD;
				}

				if (extra_cycle) {
					uni_state->cycle_next = cnext = 1;
				} else if (taf_info->super_active && cnext == 1 && cnow == 1) {
					uni_state->cycle_now = cnow = 0;
				}
			}
			/* mask index because the pkt tag only has few bits */
			ias_info->tag_star_index =
				(ias_info->tag_star_index + 1) & TAF_MAX_PKT_INDEX;
		} else {
			WL_TAFT1(taf_info, "final exit %uus released, immed "
				"retrig (%u) (prev rel is %u, in transit %u), "
				"sched dur %uus, r_t_s 0x%x\n\n",
				TAF_UNITS_TO_MICROSEC(real_release),
				uni_state->resched_index[cnext],
				ias_info->data.time_delta, TXPKTPENDTOT(TAF_WLCT(taf_info)),
				duration, *scheduler->ready_to_schedule);
			uni_state->trigger = TAF_TRIGGER_IMMEDIATE;

			if (uni_state->barrier_req == 1) {
				/* mask index because the pkt tag only has few bits */
				ias_info->tag_star_index =
					(ias_info->tag_star_index + 1) & TAF_MAX_PKT_INDEX;

				WL_TAFT1(taf_info, "increment index to %u due to barrier req\n",
					ias_info->tag_star_index);
				++ias_info->debug.barrier_req;
			}
		}
		uni_state->total_sched_units[context->public.ias.index][TAF_TYPE_DL] =
			TAF_IAS_REAL(uni_state, TAF_TYPE_DL);
#if TAF_ENABLE_UL
		uni_state->total_sched_units[context->public.ias.index][TAF_TYPE_UL] =
			TAF_IAS_REAL(uni_state, TAF_TYPE_UL);
#endif
		taf_info->release_count ++;
	} else if (finished) {
#if TAF_ENABLE_TIMER
		if (tuit == 0 && TAF_IAS_OPT(ias_info, RETRIGGER) &&
			taf_ias_retrig_start(taf_info, TAF_IAS_NOWTIME(method)) == BCME_OK) {

			uni_state->trigger = TAF_TRIGGER_TIMER;

			WL_TAFT1(taf_info, "final exit nothing released, start retrigger timer; "
				"retrig (%u) (in transit %u)\n\n",
				uni_state->resched_index[uni_state->cycle_next],
				TXPKTPENDTOT(TAF_WLCT(taf_info)));
		} else
#endif
		{
			uni_state->trigger = TAF_TRIGGER_IMMEDIATE;

			WL_TAFT1(taf_info, "final exit nothing released, immed "
				"retrig (%u) (in transit %u)\n\n",
				uni_state->resched_index[uni_state->cycle_next],
				TXPKTPENDTOT(TAF_WLCT(taf_info)));
		}
	}

	if (finished) {
		uni_state->real[TAF_TYPE_DL].released_units = 0;
#if TAF_ENABLE_UL
		uni_state->real[TAF_TYPE_UL].released_units = 0;
#endif
		uni_state->total.released_units = 0;
		memset(&uni_state->extend, 0, sizeof(uni_state->extend));
		uni_state->est_release_units = 0;
		uni_state->waiting_schedule = 0;
		uni_state->release_type_present = 0;
		uni_state->bw_type_present = 0;
		uni_state->pairing_id = 0;
		uni_state->last_paired[TAF_TYPE_DL] = 0;
#if TAF_ENABLE_UL
		uni_state->last_paired[TAF_TYPE_UL] = 0;
#endif

		ias_info->ncount_flow_last = ias_info->ncount_flow;
		ias_info->ncount_flow = 0;

		if (taf_info->super_active) {
			WL_TAFT2(taf_info, "super sched: wait %u/%u, cycle now %u; cycle next "
				"%u (%u/%u), ready %u, wait sched %u\n\n",
				uni_state->resched_index[0],
				TAF_UNITS_TO_MICROSEC(uni_state->resched_units[0]),
				uni_state->cycle_now, uni_state->cycle_next,
				uni_state->resched_index[1],
				TAF_UNITS_TO_MICROSEC(uni_state->resched_units[1]),
				uni_state->cycle_ready,
				uni_state->waiting_schedule);
		}
		uni_state->star_packet_received = FALSE;

		context->status = TAF_CYCLE_COMPLETE;
		ias_info->ias_state = TAF_IAS_CYCLE_COMPLETE;
	}
	return finished;
}

static bool taf_ias_data_block(taf_method_info_t *method)
{
#ifdef TAF_DBG
	wlc_taf_info_t *taf_info = method->taf_info;
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);
#endif /* TAF_DBG */

	if (ias_cfg(method).data_block) {
#ifdef TAF_DBG
		if (ias_info->data_block_start == 0) {
			ias_info->data_block_start = TAF_IAS_NOWTIME(method);
			ias_info->data_block_prev_in_transit =
				TXPKTPENDTOT(TAF_WLCT(taf_info));
			WL_TAFM1(method, "exit due to data_block (START)\n");
		}
		if (TXPKTPENDTOT(TAF_WLCT(taf_info)) !=
				ias_info->data_block_prev_in_transit) {
			ias_info->data_block_prev_in_transit =
				TXPKTPENDTOT(TAF_WLCT(taf_info));
			WL_TAFM1(method, "exit due to data_block (%uus)\n",
				(TAF_IAS_NOWTIME(method) - ias_info->data_block_start));
		}
#endif /* TAF_DBG */
		return TRUE;
	}
#ifdef TAF_DBG
	else if (ias_info->data_block_start) {
		ias_info->data_block_total += TAF_IAS_NOWTIME(method) - ias_info->data_block_start;
		ias_info->data_block_start = 0;
	}
#endif /* TAF_DBG */
	return FALSE;
}

#if TAF_ENABLE_SQS_PULL
/* this is to PUSH all the virtual packets previously PULLED which have become real;
 * this is a 'hidden' IAS scheduler dedicated to v2r completion processing
 */
static bool BCMFASTPATH taf_ias_markup(wlc_taf_info_t *taf_info, taf_release_context_t *context,
	void *scheduler_context)
{
	taf_method_info_t *method = scheduler_context;
	int tid = context->tid;
	bool v2r_event = (context->tid == TAF_SQS_V2R_COMPLETE_TID);
	uint32 prev_time;
	taf_ias_uni_state_t *uni_state;
	taf_ias_group_info_t* ias_info;
	taf_scb_cubby_t* scb_taf = v2r_event ? NULL : *SCB_TAF_CUBBY_PTR(taf_info, context->scb);

	prev_time = context->now_time;
	TAF_IAS_NOWTIME_SYNC(method, prev_time);

	if ((taf_info->op_state != TAF_MARKUP_REAL_PACKETS) ||
			(context->op_state != TAF_MARKUP_REAL_PACKETS)) {
		WL_ERROR(("wl%u %s: NOT IN MARKUP PHASE\n",
			WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__));
		TAF_ASSERT(0);
		return FALSE;
	}
	if (!v2r_event && scb_taf == NULL) {
		TAF_ASSERT(0);
		return FALSE;
	}

	if (!v2r_event && (scb_taf->info.pkt_pull_dequeue == 0)) {

		WL_TAFM4(method, MACF" tid %u got no new dequeued "
			"packets (%u) %s\n",
			TAF_ETHERC(scb_taf), tid,
			scb_taf->info.pkt_pull_dequeue,
			scb_taf->info.ps_mode ? "in PS mode":"- exit");
		if (!scb_taf->info.ps_mode) {
			return TRUE;
		}
	}

	if (!v2r_event && !(scb_taf->info.tid_enabled & (1 << tid))) {
		/* this is an assertfail level problem */
		WL_ERROR(("wl%u %s: tid %u not enabled (0x%x/0x%x/%u/%u)\n",
			WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__,
			tid, scb_taf->info.tid_enabled,
			scb_taf->info.pkt_pull_map,
			scb_taf->info.pkt_pull_dequeue,
			scb_taf->info.ps_mode));
		TAF_ASSERT(v2r_event || (scb_taf->info.tid_enabled & (1 << tid)));
		return TRUE;
	}

	if (!v2r_event && (SCB_MARKED_DEL(scb_taf->scb) || SCB_DEL_IN_PROGRESS(scb_taf->scb))) {
		WL_TAFM2(method, MACF" delete%s (mark 0x%x)\n",
			TAF_ETHERC(scb_taf), scb_taf->info.ps_mode ? " in PS mode" : "",
		        (scb_taf->scb->mark));
		return TRUE;
	}

	uni_state = taf_get_uni_state(taf_info);
	ias_info = TAF_IAS_GROUP_INFO(taf_info);
	context->public.ias.index = ias_info->tag_star_index;

	if (!v2r_event) {
		taf_ias_sched_data_t* ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);
#if TAF_ENABLE_TIMER
		const bool agg_hold = taf_ias_agghold_is_active(ias_info);
#else
		const bool agg_hold = FALSE;
#endif
		TAF_ASSERT(ias_sched);

		if (!taf_ias_data_block(method) && !agg_hold && !taf_info->scheduler_inhibit &&
			!TAF_PRIO_SUSPENDED(scb_taf, tid, TAF_TYPE_DL)) {

			if (scb_taf->info.ps_mode) {
				/* unsolicited ps poll */
				scb_taf->info.tech_type[TAF_TYPE_DL][tid] = TAF_TECH_DL_SU;

				if (!(scb_taf->info.scb_stats.global.rdata.ratespec_mask &
					(1 << TAF_RSPEC_SU_DL))) {

					taf_ias_rate_to_units(taf_info, scb_taf,
						TAF_TECH_DL_SU_MASK);
				}
			}
			taf_ias_sched_send(method, scb_taf, tid, uni_state,
				ias_sched, context,
				TAF_SCHEDULE_REAL_PACKETS, TAF_TYPE_DL, agg_hold);

			taf_ias_sched_update(scb_taf->method, TAF_SCHEDULE_REAL_PACKETS,
				scb_taf, tid, uni_state, TAF_TYPE_DL);
		} else {
			WL_TAFM2(method, MACF" tid %u release withheld (%d/%d/%d/%d)\n",
				TAF_ETHERC(scb_taf), tid,
			        taf_ias_data_block(method), agg_hold, taf_info->scheduler_inhibit,
				TAF_PRIO_SUSPENDED(scb_taf, tid, TAF_TYPE_DL));
		}
		scb_taf->info.pkt_pull_dequeue = 0;

		if (scb_taf->info.pkt_pull_map == 0) {
			/* reset release limit tracking after pull cycle has completed */
			ias_sched->release_config[TAF_TYPE_DL].units = 0;
		}
		WL_TAFM3(method, "total_pull_requests %u\n", taf_info->total_pull_requests);
	} else {
		WL_TAFM3(method, "v2r completion\n");
	}

	context->now_time = taf_timestamp(TAF_WLCT(taf_info));
	TAF_IAS_NOWTIME_SYNC(method, context->now_time);

	ias_info->cpu_time += (context->now_time - prev_time);

	if (wlc_taf_marked_up(taf_info)) {
		return taf_ias_completed(method, context, uni_state, TRUE,
			TAF_SCHEDULE_REAL_PACKETS);
	}
	return TRUE;
}
#endif /* TAF_ENABLE_SQS_PULL */

static bool BCMFASTPATH
taf_ias_schedule(wlc_taf_info_t *taf_info, taf_release_context_t *context,
	void *scheduler_context)
{
	taf_method_info_t *method = scheduler_context;
	bool finished = FALSE;
	taf_ias_group_info_t* ias_info;
	taf_ias_uni_state_t *uni_state;
	uint32 prev_time;
	uint32 prev_est_units;
	uint32 real_release;

	prev_time = context->now_time;
	TAF_IAS_NOWTIME_SYNC(method, prev_time);

	WL_TAFM4(method, "start\n");

#if TAF_ENABLE_SQS_PULL
	/* packet markup is handle only by taf_ias_markup */
	TAF_ASSERT(taf_info->op_state != TAF_MARKUP_REAL_PACKETS);
	TAF_ASSERT(context->op_state != TAF_MARKUP_REAL_PACKETS);

	if (context->op_state == TAF_SCHEDULE_VIRTUAL_PACKETS) {
		TAF_ASSERT(taf_info->op_state == TAF_SCHEDULE_VIRTUAL_PACKETS);
	}
#endif /* TAF_ENABLE_SQS_PULL */

	if (context->op_state == TAF_SCHEDULE_REAL_PACKETS) {
		TAF_ASSERT(context->tid >= 0 && context->tid < TAF_MAXPRIO);
	}

	if (!ias_cfg(method).tid_active && method->type != LAST_IAS_SCHEDULER) {
		WL_TAFM4(method, "not active\n");
		return FALSE;
	}

	ias_info = TAF_IAS_GROUP_INFO(taf_info);

#if TAF_ENABLE_TIMER
	if (taf_ias_retrig_is_active(ias_info)) {

		taf_ias_retrig_stop(taf_info, ias_info, context->now_time);
		WL_TAFM2(method, "retriggered without timer\n");
	}
#endif

	if (taf_ias_data_block(method)) {
		return TRUE;
	}

	uni_state = taf_get_uni_state(taf_info);
	uni_state->was_reset = 0;

#if TAF_ENABLE_UL
	if (TAF_IAS_OPT(ias_info, DEFER_UL) && (ias_info->ias_state == TAF_IAS_CYCLE_COMPLETE) &&
		(uni_state->trigger != TAF_TRIGGER_STAR_THRESHOLD) &&
		(uni_state->trigger != TAF_TRIGGER_IMMEDIATE_SUPER_RESCHED) &&
		(TAF_IAS_TUIT(uni_state, TAF_TYPE_DL) == 0) &&
		(TAF_IAS_TUIT(uni_state, TAF_TYPE_UL) != 0)) {

		WL_TAFM2(method, "defer scheduler with %u ul units in transit (%u)\n",
			TAF_IAS_TUIT(uni_state, TAF_TYPE_UL), uni_state->trigger);

		goto ias_exit;
	}
#endif

	if (TAF_IAS_OPT(ias_info, DEFER_SUPER) && (ias_info->ias_state == TAF_IAS_CYCLE_COMPLETE) &&
		(uni_state->cycle_now == 1) && (uni_state->cycle_next == 1) &&
		(uni_state->trigger != TAF_TRIGGER_IMMEDIATE_SUPER_RESCHED) &&
		(TAF_IAS_TUIT(uni_state, TAF_TYPE_ALL) > 0)) {

		uint32 prev_rel = context->now_time - ias_info->data.prev_release_time;
		uint32 min_interval =
			uni_state->high[IAS_INDEXM(method)] - uni_state->low[IAS_INDEXM(method)];

		if (prev_rel < min_interval) {
			WL_TAFM2(method, "defer super-scheduler, insufficient interval (%u/%u), "
				"%u tuit\n",
				prev_rel, min_interval, TAF_IAS_TUIT(uni_state, TAF_TYPE_ALL));
			return TRUE;
		}
	}

	if (ias_info->ias_state == TAF_IAS_CYCLE_COMPLETE) {
		ias_info->ias_state = TAF_IAS_CYCLE_START;
	}

	real_release = TAF_IAS_REAL(uni_state, TAF_TYPE_ALL);

	if (ias_info->sched_start == 0) {
		ias_info->sched_start = context->now_time;
	}

	ias_cfg(method).sched_active = 0;
	method->counter++;

	if (uni_state->total.released_units > 0) {
		context->state[TAF_SCHED_INDEX(method->type)] |= (1 << TAF_STATE_HIGHER_PEND);
	}

	prev_est_units = uni_state->est_release_units;
	uni_state->intermediate_sort = 0;
	taf_ias_prepare_list(method);

	if (uni_state->est_release_units > prev_est_units) {

		uint32 new_traffic = uni_state->est_release_units - prev_est_units;

		WL_TAFM2(method, "total est units %u (total in transit %u, total released %u)\n",
			new_traffic, TAF_IAS_TUIT(uni_state, TAF_TYPE_ALL), real_release);

#if TAF_ENABLE_TIMER
		if ((uni_state->total.released_units == 0) && (ias_info->agg_hold > 0)) {
			if (new_traffic < ias_info->agg_hold_threshold) {
				if (!taf_ias_agghold_was_active(ias_info) &&
						!taf_ias_agghold_is_active(ias_info)) {

					taf_ias_agghold_start(taf_info, ias_info->agg_hold,
						context->now_time);
				}
#if !TAF_ENABLE_SQS_PULL
				if (taf_ias_agghold_is_active(ias_info)) {
					ias_cfg(method).sched_active = 0;
					WL_TAFM2(method, "aggregation hold active\n");
				}
#endif /* !TAF_ENABLE_SQS_PULL */
			} else {
				if (taf_ias_agghold_is_active(ias_info)) {
					WL_TAFM1(method, "enough traffic to end agg_hold\n");
					taf_ias_agghold_stop(taf_info, ias_info, context->now_time);
					taf_ias_agghold_clear(ias_info);
				}
			}
		}
#else
		BCM_REFERENCE(new_traffic);
#endif /* TAF_ENABLE_TIMER */
	}

	if ((ias_info->tag_star_index == ias_info->prev_tag_star_index) &&
		(uni_state->trigger != TAF_TRIGGER_STAR_THRESHOLD) && (real_release == 0) &&
		(uni_state->est_release_units >
			TAF_MICROSEC_TO_UNITS(uni_state->low[IAS_INDEXM(method)]))) {

		memset(uni_state->total_sched_units, 0,  sizeof(uni_state->total_sched_units));
		ias_info->tag_star_index = (ias_info->tag_star_index + 1) & TAF_MAX_PKT_INDEX;
		WL_TAFM1(method, "incremented tag index now %u (%u, %u)\n",
			ias_info->tag_star_index, uni_state->est_release_units,
			TAF_MICROSEC_TO_UNITS(uni_state->low[IAS_INDEXM(method)]));
	}

	if (uni_state->est_release_units <
			TAF_MICROSEC_TO_UNITS(uni_state->low[IAS_INDEXM(method)])) {
		uni_state->free_release = (uni_state->trigger != TAF_TRIGGER_STAR_THRESHOLD) &&
			TAF_IAS_OPT(ias_info, FREE_RELEASE);
	} else {
		uni_state->free_release = FALSE;
		WL_TAFM4(method, "no free release %u, %u, %u\n", uni_state->est_release_units,
			TAF_MICROSEC_TO_UNITS(uni_state->low[IAS_INDEXM(method)]),
			uni_state->trigger);
	}

	context->public.how = TAF_RELEASE_LIKE_IAS;
	context->public.ias.index = ias_info->tag_star_index;
	uni_state->cumul_units[ias_info->tag_star_index][TAF_TYPE_DL] = 0;
#if TAF_ENABLE_UL
	uni_state->cumul_units[ias_info->tag_star_index][TAF_TYPE_UL] = 0;
#endif

	if (ias_cfg(method).sched_active) {
		finished = taf_ias_schedule_all(method, uni_state, 0, NUMPRIO - 1,
			context);
	}

#if TAF_ENABLE_UL
ias_exit:
#endif
	context->now_time = taf_timestamp(TAF_WLCT(taf_info));
	TAF_IAS_NOWTIME_SYNC(method, context->now_time);

	ias_info->cpu_time += (context->now_time - prev_time);

	return taf_ias_completed(method, context, uni_state, finished, taf_info->op_state);
}

static int BCMATTACHFN(taf_ias_method_detach)(void* context)
{
	taf_ias_container_t* container = context;
	taf_method_info_t* method = container ? &container->method : NULL;

	TAF_ASSERT((void*)method == (void*)container);

	if (container) {
		wlc_taf_info_t* taf_info = method->taf_info;

		TAF_ASSERT(taf_info->group_use_count[method->group] > 0);
		taf_info->group_use_count[method->group]--;

		if (taf_info->group_use_count[method->group] == 0) {
			taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);

			if (ias_info) {
#if TAF_ENABLE_TIMER
				if (ias_info->agg_hold_timer) {
					wl_free_timer(TAF_WLCT(taf_info)->wl,
						ias_info->agg_hold_timer);
				}
#endif
				MFREE(TAF_WLCT(taf_info)->pub->osh, ias_info, sizeof(*ias_info));
			}
			taf_info->group_context[method->group] = NULL;
		}
		MFREE(TAF_WLCT(taf_info)->pub->osh, container, sizeof(*container));
	} else {
		TAF_ASSERT(0);
	}
	return BCME_OK;
}

static int taf_ias_watchdog(void* handle)
{
	taf_method_info_t* method = handle;
	wlc_taf_info_t* taf_info = method->taf_info;
#ifdef TAF_DEBUG_VERBOSE
	bool do_dump = FALSE;
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);
#endif
	wlc_info_t *wlc = TAF_WLCT(taf_info);
	taf_ias_uni_state_t* uni_state = taf_get_uni_state(taf_info);
	uint32 tuit;

#if TAF_ENABLE_SQS_PULL
	TAF_ASSERT(method->type != TAF_VIRTUAL_MARKUP);
#endif
	if (method->list == NULL && method->active_list == NULL) {
		ias_cfg(method).tid_active = 0;
		WL_TAFM4(method, "no links active\n");
		return BCME_OK;
	}

#ifdef TAF_DEBUG_VERBOSE
	if (ias_info->debug.barrier_start != 0) {
		int8 last_sched = uni_state->prev_resched_index - (ias_cfg(method).barrier - 1);
		uint32 elapsed;
		uint32 now_time = taf_timestamp(TAF_WLCT(taf_info));

		if (last_sched < 0) {
			last_sched += TAF_MAX_NUM_WINDOWS;
		}
		TAF_IAS_NOWTIME_SYNC(method, now_time);

		elapsed = now_time - ias_info->debug.barrier_start;

		WL_TAFM1(method, "waiting barrier (prev window %d, current delay %uus)\n",
			last_sched, elapsed);
	}
#endif /* TAF_DEBUG_VERBOSE */

	if (!wlc_taf_scheduler_blocked(taf_info)) {
		taf_traffic_map_t enabled = 0;
		taf_list_t* list = taf_ias_get_list_head(method);

		while (list) {
			taf_scb_cubby_t *scb_taf;

			scb_taf = list->scb_taf;
			enabled |= scb_taf->info.tid_enabled;

			list = list->next;
		}
		if (ias_cfg(method).tid_active != enabled) {
			WL_TAFM1(method, "current enable 0x%x, checked enable 0x%x\n",
				ias_cfg(method).tid_active, enabled);
			ias_cfg(method).tid_active = enabled;
		}
	}

	if (TXPKTPENDTOT(wlc)) {
		/* if not idle, just exit this housekeeping now */
		return BCME_OK;
	}

	/* Check for active transmission from AMPDU/NAR paths */
#if TAF_ENABLE_NAR
	if (wlc_nar_tx_in_tansit(wlc->nar_handle)) {
		return BCME_OK;
	}
#endif

	if (wlc_ampdu_tx_intransit_get(wlc)) {
		return BCME_OK;
	}

#if TAF_ENABLE_UL
	if (ULTRIGPENDTOT(wlc)) {
		/* if not idle, just exit this housekeeping now */
		return BCME_OK;
	}
#endif /* TAF_ENABLE_UL */

	tuit = TAF_IAS_TUIT(uni_state, TAF_TYPE_ALL);

	/* total packets pending is zero (as watchdog exits earlier if not) */
	if (tuit > 0) {
		WL_ERROR(("wl%u %s: (%x) "
#if TAF_ENABLE_UL
			"%u + %u = "
#endif
			"%u units in transit (expect 0)\n",
			WLCWLUNIT(wlc), __FUNCTION__,
			taf_info->scheduler_index,
#if TAF_ENABLE_UL
			TAF_IAS_TUIT(uni_state, TAF_TYPE_DL), TAF_IAS_TUIT(uni_state, TAF_TYPE_UL),
#endif
			tuit));
#ifdef TAF_DEBUG_VERBOSE
		do_dump  = TRUE;
#endif
		taf_ias_unified_reset(method);
	}

	if (*method->ready_to_schedule == 0) {
		WL_ERROR(("wl%u %s: (%x) (%s) not ready to schedule (%x)\n",
			WLCWLUNIT(wlc), __FUNCTION__,
			taf_info->scheduler_index, TAF_SCHED_NAME(method),
			ias_cfg(method).tid_active));
#ifdef TAF_DEBUG_VERBOSE
		do_dump  = TRUE;
#endif
	}

#ifdef TAF_DEBUG_VERBOSE
	if (do_dump) {
		taf_memtrace_dump(taf_info);
	}
#endif

	return BCME_OK;
}

static const taf_scheduler_fn_t taf_ias_funcs = {
	taf_ias_schedule,       /* scheduler_fn     */
	taf_ias_watchdog,       /* watchdog_fn      */
	taf_ias_dump,           /* dump_fn          */
	taf_ias_link_state,     /* linkstate_fn     */
	taf_ias_scb_state,      /* scbstate_fn      */
	NULL,                   /* bssstate_fn      */
	taf_ias_rate_override,  /* rateoverride_fn  */
	taf_ias_iovar,          /* iovar_fn         */
	taf_ias_tx_status,      /* txstat_fn        */
	taf_ias_rx_status,      /* rxstat_fn        */
	taf_ias_sched_state,    /* schedstate_fn    */
	taf_ias_traffic_stats,  /* traffic_stats_fn */
#ifdef TAF_PKTQ_LOG
	taf_ias_dpstats_dump,   /* dpstats_log_fn   */
#endif
};

#if TAF_ENABLE_SQS_PULL
static const taf_scheduler_fn_t taf_ias_markup_funcs = {
	taf_ias_markup,         /* scheduler_fn     */
	NULL,                   /* watchdog_fn      */
	NULL,                   /* dump_fn          */
	NULL,                   /* linkstate_fn     */
	NULL,                   /* scbstate_fn      */
	NULL,                   /* bssstate_fn      */
	NULL,                   /* rateoverride_fn  */
	taf_ias_iovar,          /* iovar_fn         */
	NULL,                   /* txstat_fn        */
	NULL,                   /* rxstat_fn        */
	taf_iasm_sched_state,   /* schedstate_fn    */
	NULL,                   /* traffic_stats_fn */
#ifdef TAF_PKTQ_LOG
	NULL,                    /* dpstats_log_fn  */
#endif
};
#endif /* TAF_ENABLE_SQS_PULL */

#define TAFNAME  "taf -"

static const char* taf_ias_name[NUM_IAS_SCHEDULERS] = {
#if TAF_ENABLE_SQS_PULL
	TAFNAME"markup",
#endif /* TAF_ENABLE_SQS_PULL */
	TAFNAME"ebos", TAFNAME"prr", TAFNAME"atos", TAFNAME"atos2"};

static void* BCMATTACHFN(taf_ias_method_attach)(wlc_taf_info_t *taf_info, taf_scheduler_kind type)
{
	taf_ias_container_t* container;
	taf_method_info_t* method;
	taf_ias_group_info_t* ias_info = NULL;
	taf_source_type_t source;
	taf_ias_method_info_t* cfg_ias;

	TAF_ASSERT(TAF_TYPE_IS_IAS(type));

	if (type == TAF_PSEUDO_RR) {
		/* not supported any more */
		return NULL;
	}

	container = (taf_ias_container_t*) MALLOCZ(TAF_WLCT(taf_info)->pub->osh,
		sizeof(*container));

	if (container == NULL) {
		goto exitfail;
	}
	method = &container->method;
	TAF_ASSERT((void*)method == (void*)container);

	cfg_ias = &ias_cfg(method);

	method->taf_info = taf_info;
	method->type = type;
	method->ordering = TAF_LIST_SCORE_MINIMUM;
	method->scheme = TAF_ORDERED_PULL_SCHEDULING;

	method->group = TAF_SCHEDULER_IAS_METHOD;
#if TAF_ENABLE_SQS_PULL
	if (type == TAF_VIRTUAL_MARKUP) {
		method->funcs = taf_ias_markup_funcs;
	} else
#endif /* TAF_ENABLE_SQS_PULL */
	{
		method->funcs = taf_ias_funcs;
	}
	method->name = taf_ias_name[IAS_INDEX(type)] + sizeof(TAFNAME) - 1;

	method->score_init = 0;

#ifdef TAF_DBG
	if (method->funcs.dump_fn) {
		method->dump_name = taf_ias_name[IAS_INDEX(type)];
	}
#endif

	/*
	 * done once only (first time) even though this function might be called several times
	 * for each scheduler sub-type (EBOS, PRR, ATOS, ATOS2....)
	 */

	if (IAS_INDEX(type) == 0) {
		ias_info = (taf_ias_group_info_t*)MALLOCZ(TAF_WLCT(taf_info)->pub->osh,
			sizeof(*ias_info));
		if (ias_info == NULL) {
			goto exitfail;
		}
		taf_info->group_context[TAF_SCHEDULER_IAS_METHOD] = ias_info;
		ias_info->immed = 24000;
		ias_info->now_time = taf_timestamp(TAF_WLCT(taf_info));
		ias_info->options =
			(1 << TAF_OPT_IAS_TOOMUCH) |
			(1 << TAF_OPT_IAS_TOOMUCH_LOOSE) |
			(1 << TAF_OPT_IAS_SFAIL_LOOSE) |
			(1 << TAF_OPT_IAS_IMMED_LOOSE) |
			(1 << TAF_OPT_IAS_AUTOBW20_SUPER) |
			(1 << TAF_OPT_IAS_FREE_RELEASE) |
			(1 << TAF_OPT_IAS_DEFER_SUPER) |
			(1 << TAF_OPT_IAS_FLUSH_PAIRING) |
			(1 << TAF_OPT_IAS_DEFER_UL);
#if TAF_ENABLE_TIMER
		ias_info->options |= (1 << TAF_OPT_IAS_RETRIGGER);
		ias_info->agg_hold_timer =
			wl_init_timer(TAF_WLCT(taf_info)->wl, taf_ias_aggh_tmr_exp,
				taf_info, "TAF ias_agg_hold");
		if (ias_info->agg_hold_timer == NULL) {
			WL_ERROR(("wl%d: wl_init_timer for TAF ias_agg_hold failed\n",
				TAF_WLCT(taf_info)->pub->unit));
			goto exitfail;
		}
		ias_info->retrigger_timer[TAF_TYPE_DL] = TAF_IAS_RETRIG_TIME_NORM;
#if TAF_ENABLE_UL
		ias_info->retrigger_timer[TAF_TYPE_UL] = TAF_IAS_RETRIG_TIME_UL;
#endif
#endif /* TAF_ENABLE_TIMER */
	} else {
		ias_info = TAF_IAS_GROUP_INFO(taf_info);
		TAF_ASSERT(ias_info);
	}
	taf_info->group_use_count[TAF_SCHEDULER_IAS_METHOD] ++;
	method->now_time_p = &ias_info->now_time;

	cfg_ias->coeff.time_shift = TAF_COEFF_IAS_FACTOR;
	taf_ias_set_coeff(TAF_COEFF_IAS_DEF, &cfg_ias->coeff);

#if TAF_ENABLE_SQS_PULL
#if TAF_ENABLE_NAR
	cfg_ias->pre_rel_limit[TAF_SOURCE_NAR] = 1;
#endif /* TAF_ENABLE_NAR */

	cfg_ias->pre_rel_limit[TAF_SOURCE_AMPDU] = 32;

#if TAF_ENABLE_UL
	cfg_ias->pre_rel_limit[TAF_SOURCE_UL] = 65535;
#endif
	cfg_ias->pre_rel_limit[TAF_SOURCE_HOST_SQS] = 0;
	/* with SQS, margin is the amount of extra pkts to request beyond estimate */
	cfg_ias->margin = 20;

#if TAF_ENABLE_PSQ_PULL
	cfg_ias->pre_rel_limit[TAF_SOURCE_PSQ] = 0;
#endif
#endif /* TAF_ENABLE_SQS_PULL */

	cfg_ias->release_limit = 0;
	cfg_ias->barrier = 2;

	switch (type) {
#if TAF_ENABLE_SQS_PULL
		case TAF_VIRTUAL_MARKUP:
			/* fall through */
#endif /* TAF_ENABLE_SQS_PULL */
		case TAF_EBOS :
			method->ordering = TAF_LIST_SCORE_FIXED_INIT_MINIMUM;
			method->score_init = 1;
			cfg_ias->high = TAF_TIME_HIGH_DEFAULT;
			cfg_ias->low = TAF_TIME_LOW_DEFAULT;
			cfg_ias->opt_aggp_limit = 0;
			cfg_ias->score_weights = NULL;
			cfg_ias->barrier = 1;
#if TAF_ENABLE_MU_TX
			cfg_ias->mu_pair = NULL;
			cfg_ias->mu_mimo_opt = 0;
			cfg_ias->mu_ofdma_opt = 0;
#endif /* TAF_ENABLE_MU_TX */
			cfg_ias->options =
				(1 << TAF_OPT_PKT_NAR_OVERHEAD) |
				(1 << TAF_OPT_PKT_AMPDU_OVERHEAD) |
				(1 << TAF_OPT_ULTID_IGNRE);
			break;

		case TAF_ATOS:
			cfg_ias->high = TAF_TIME_ATOS_HIGH_DEFAULT;
			cfg_ias->low = TAF_TIME_ATOS_LOW_DEFAULT;
			cfg_ias->opt_aggp_limit = 63;
			cfg_ias->score_weights = taf_tid_score_weights;
#if TAF_ENABLE_UL
			cfg_ias->mu_user_rel_limit[TAF_TECH_UL_OFDMA] = taf_ul_ofdma_rel_limit;
#endif
#if TAF_ENABLE_MU_TX
			cfg_ias->mu_pair = taf_mu_pair;
			cfg_ias->mu_user_rel_limit[TAF_TECH_DL_VHMUMIMO] = taf_mu_mimo_rel_limit;
			cfg_ias->mu_user_rel_limit[TAF_TECH_DL_HEMUMIMO] = taf_mu_mimo_rel_limit;
			cfg_ias->mu_user_rel_limit[TAF_TECH_DL_OFDMA] = taf_dl_ofdma_rel_limit;
			cfg_ias->mu_mimo_opt =
				(1 << TAF_OPT_MIMO_MEAN) |
				(1 << TAF_OPT_MIMO_SWEEP) |
				(1 << TAF_OPT_MIMO_BRIDGE_PAIR_DL) |
				(1 << TAF_OPT_MIMO_PAIR_MAXMU);

			cfg_ias->mu_ofdma_opt =
				(1 << TAF_OPT_OFDMA_TOO_SMALL) |
				(1 << TAF_OPT_OFDMA_TOO_SMALLUL) |
				(1 << TAF_OPT_OFDMA_TOO_LARGE) |
				(1 << TAF_OPT_OFDMA_TOO_LARGEUL) |
				(1 << TAF_OPT_OFDMA_PAIR) |
				(1 << TAF_OPT_OFDMA_ULTID_IGNRE) |
				(1 << TAF_OPT_OFDMA_BRIDGE_PAIR_DL) |
				(1 << TAF_OPT_OFDMA_BRIDGE_PAIR_UL);
#endif /* TAF_ENABLE_MU_TX */
			cfg_ias->options =
				(1 << TAF_OPT_EXIT_EARLY) |
				(1 << TAF_OPT_PKT_NAR_OVERHEAD) |
				(1 << TAF_OPT_PKT_AMPDU_OVERHEAD) |
				(1 << TAF_OPT_ULTID_IGNRE) |
				(1 << TAF_OPT_TID_RE_SORT);
			break;

		case TAF_ATOS2:
			cfg_ias->high = TAF_TIME_ATOS2_HIGH_DEFAULT;
			cfg_ias->low = TAF_TIME_ATOS2_LOW_DEFAULT;
			cfg_ias->opt_aggp_limit = 31;
			cfg_ias->score_weights = taf_tid_score_weights;
#if TAF_ENABLE_UL
			cfg_ias->mu_user_rel_limit[TAF_TECH_UL_OFDMA] = taf_ul_ofdma_rel_limit;
#endif
#if TAF_ENABLE_MU_TX
			cfg_ias->mu_pair = taf_mu_pair;
			cfg_ias->mu_user_rel_limit[TAF_TECH_DL_VHMUMIMO] = taf_mu_mimo_rel_limit;
			cfg_ias->mu_user_rel_limit[TAF_TECH_DL_HEMUMIMO] = taf_mu_mimo_rel_limit;
			cfg_ias->mu_user_rel_limit[TAF_TECH_DL_OFDMA] = taf_dl_ofdma_rel_limit;

			cfg_ias->mu_mimo_opt =
				(1 << TAF_OPT_MIMO_MEAN) |
				(1 << TAF_OPT_MIMO_SWEEP) |
				(1 << TAF_OPT_MIMO_BRIDGE_PAIR_DL) |
				(1 << TAF_OPT_MIMO_PAIR_MAXMU);

			cfg_ias->mu_ofdma_opt =
				(1 << TAF_OPT_OFDMA_TOO_SMALL) |
				(1 << TAF_OPT_OFDMA_TOO_SMALLUL) |
				(1 << TAF_OPT_OFDMA_TOO_LARGE) |
				(1 << TAF_OPT_OFDMA_TOO_LARGEUL) |
				(1 << TAF_OPT_OFDMA_PAIR) |
				(1 << TAF_OPT_OFDMA_ULTID_IGNRE) |
				(1 << TAF_OPT_OFDMA_BRIDGE_PAIR_DL) |
				(1 << TAF_OPT_OFDMA_BRIDGE_PAIR_UL);
#endif /* TAF_ENABLE_MU_TX */
			cfg_ias->options =
				(1 << TAF_OPT_EXIT_EARLY) |
				(1 << TAF_OPT_PKT_NAR_OVERHEAD) |
				(1 << TAF_OPT_PKT_AMPDU_OVERHEAD) |
				(1 << TAF_OPT_ULTID_IGNRE) |
				(1 << TAF_OPT_TID_RE_SORT);
			break;

		case TAF_PSEUDO_RR:
			/* not supported */
		default:
			MFREE(TAF_WLCT(taf_info)->pub->osh, container, sizeof(*container));
			WL_ERROR(("wl%u %s: unknown taf scheduler type %u\n",
				WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__, type));
			TAF_ASSERT(0);
			return NULL;
	}
	taf_ias_time_settings_sync(method);

	if (method->type == taf_info->default_scheduler) {
		taf_info->default_score = method->score_init;
	}
#if TAF_ENABLE_SQS_PULL
	if (type == TAF_VIRTUAL_MARKUP) {
		method->ready_to_schedule = &ias_info->ias_ready_to_schedule[IAS_INDEX(type)];
		*method->ready_to_schedule = 0;
	} else
#endif /* TAF_ENABLE_SQS_PULL */
	{
		method->ready_to_schedule =
			&ias_info->ias_ready_to_schedule[IAS_INDEX(LAST_IAS_SCHEDULER)];
		*method->ready_to_schedule = ~0;
	}

	for (source = 0; source < TAF_NUM_SCHED_SOURCES; source ++) {
		TAF_ASSERT(taf_info->source_handle_p[source]);
		TAF_ASSERT(taf_info->funcs[source].scb_h_fn);
		TAF_ASSERT(taf_info->funcs[source].tid_h_fn);
		TAF_ASSERT(taf_info->funcs[source].pktqlen_fn);
	}
	return container;
exitfail:
	if (container) {
		MFREE(TAF_WLCT(taf_info)->pub->osh, container, sizeof(*container));
	}
	WL_ERROR(("wl%u %s: memory alloc fail\n", WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__));
	return NULL;
}

static char *taf_ias_dump_rate_str(char *s, uint32 kbits)
{
	if (kbits >= 7999) {
		uint32 units = kbits / 1000;
		uint32 frac = kbits % 1000;

		frac = (frac + 50) / 100;
		if (frac >= 10) {
			frac -= 10;
			++units;
		}
		sprintf(s, "%u.%01u Mbits", units, frac);
	} else {
		sprintf(s, "%u Kbits", kbits);
	}
	return s;
}

static char *taf_ias_dump_percent_str(char *s, uint32 number)
{
	sprintf(s, "%u.%u%%", number / 10, number % 10);
	return s;
}

int taf_ias_dump(void *handle, struct bcmstrbuf *b)
{
	taf_method_info_t* method = handle;
	wlc_taf_info_t *taf_info = method->taf_info;
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(taf_info);
	uint32 prev_time = ias_info->cpu_elapsed_time;
	uint32 cpu_norm = 0;
	taf_list_t*  list = taf_ias_get_list_head(method);
	taf_ias_uni_state_t* uni_state = taf_get_uni_state(taf_info);
	uint32 total_count;
	uint32 ave_high = 0;
	uint32 ave_dur = 0;
	uint32 now_time = taf_timestamp(TAF_WLCT(taf_info));
	wlc_info_t *wlc = TAF_WLCM(method);
	taf_traffic_stats_t *tid_stats = NULL;
#if TAF_ENABLE_TIMER
	uint32 ave_agg_hold = 0;
#endif
#if TAF_ENABLE_UL
	bool ul_tid_ignore;
#endif

	BCM_REFERENCE(wlc);
	if (!taf_info->enabled) {
		bcm_bprintf(b, "taf must be enabled first\n");
		return BCME_OK;
	}
	TAF_ASSERT(TAF_TYPE_IS_IAS(method->type));

	tid_stats = (taf_traffic_stats_t*) MALLOCZ(
		TAF_WLCT(taf_info)->pub->osh, NUMPRIO * sizeof(*tid_stats));
	if (tid_stats == NULL) {
		WL_ERROR(("wl%u %s: malloc fail (%u)\n", WLCWLUNIT(wlc), __FUNCTION__,
			(uint32)sizeof(*tid_stats)));
		return BCME_NOMEM;
	}

	TAF_IAS_NOWTIME_SYNC(method, now_time);
	ias_info->cpu_elapsed_time = now_time;

	if (prev_time != 0) {
		uint32 elapsed = ias_info->cpu_elapsed_time - prev_time;
		elapsed /= 10000;
		if (elapsed) {
			cpu_norm = (ias_info->cpu_time + (elapsed >> 1)) / elapsed;
		}
	}
	bcm_bprintf(b, "%s count %d, high = %uus, low = %uus, cpu time = %uus, "
		"cpu_norm = %u/10000, r_t_s 0x%x\n",
		TAF_SCHED_NAME(method), method->counter, ias_cfg(method).high, ias_cfg(method).low,
		ias_info->cpu_time, cpu_norm, *method->ready_to_schedule);

	ias_info->cpu_time = 0;

	bcm_bprintf(b, "ncount flow %u\n", ias_info->ncount_flow_last);
	method->counter = 0;

#if TAF_ENABLE_UL
	ul_tid_ignore = TAF_OFDMA_OPT(method, ULTID_IGNRE);
#endif
#if TAF_ENABLE_TIMER
	if (ias_info->debug.agg_hold_count > 0) {
		ave_agg_hold = taf_div64(ias_info->debug.agg_hold_total_time,
			ias_info->debug.agg_hold_count);
	}
#endif
	total_count = ias_info->debug.target_high + ias_info->debug.uflow_high;

	if (total_count > 0) {
		ave_high = ias_info->debug.average_high / total_count;
		ave_dur = ias_info->debug.sched_duration / total_count;
	}

	bcm_bprintf(b,
		"unified\n"
		"target high  %7u, uflow high  %7u,    ave_high    %7uus\n"
		"target low   %7u, uflow low   %7u\n"
		"immed trigg  %7u, immed star  %7u\n"
		"super_sched  %7u, sschedfail  %7u,    barrier req  %7u\n"
#if TAF_ENABLE_UL
		"tuit dl      %7u, tuit ul     %7u,    tuit all     %7u\n"
#else
		"tuit        %7u\n"
#endif
		"overflow     %7u\n"
#if TAF_ENABLE_TIMER
		"agg hold     %7u, max hold    %7uus,  ave hold    %7uus\n"
		"retrigger    %7u\n"
#endif
		"ave sched duration %5uus, max sched duration %5uus\n",

		ias_info->debug.target_high, ias_info->debug.uflow_high,
		TAF_UNITS_TO_MICROSEC(ave_high),
		ias_info->debug.target_low, ias_info->debug.uflow_low,
		ias_info->debug.immed_trig, ias_info->debug.immed_star,
		ias_info->debug.super_sched, ias_info->debug.super_sched_collapse,
		ias_info->debug.barrier_req,
#if TAF_ENABLE_UL
		TAF_IAS_TUIT(uni_state, TAF_TYPE_DL), TAF_IAS_TUIT(uni_state, TAF_TYPE_UL),
#endif
		TAF_IAS_TUIT(uni_state, TAF_TYPE_ALL),
		ias_info->debug.traffic_stat_overflow,
#if TAF_ENABLE_TIMER
		ias_info->debug.agg_hold_count, ias_info->debug.agg_hold_max_time,
		ave_agg_hold, ias_info->debug.agg_retrigger_count,
#endif
		ave_dur, ias_info->debug.max_duration);

	memset(&ias_info->debug, 0, sizeof(ias_info->debug));

	while (list) {
		taf_scb_cubby_t *scb_taf = list->scb_taf;
		taf_scheduler_scb_stats_t* scb_stats = &scb_taf->info.scb_stats;
		taf_traffic_stats_t *stats;
		int tid;
		bool output = FALSE;
		taf_traffic_map_t has_stats = 0;
		char tput[20], pcent[20];
#ifdef TAF_DBG
#if TAF_LOGL3
		taf_source_type_t s_idx;
#endif
		taf_rspec_index_t rindex;
#endif /* TAF_DBG */

		bcm_bprintf(b, "---\n"MACF"%s score %4u %s\n", TAF_ETHERC(scb_taf),
			TAF_TYPE(list->type), scb_stats->ias.data.relative_score[list->type],
			scb_taf->info.ps_mode ? " PS":"");

		/* Calculate stats for each tid first */
		for (tid = 0; tid < NUMPRIO; tid ++) {
			if (taf_ias_calc_trfstats(method, scb_taf,
				tid, &now_time, &tid_stats[tid]) == BCME_OK) {

				has_stats |= (1 << tid);
			}
		}
		/* Display downlink first */
		for (tid = 0; list->type == TAF_TYPE_DL && tid < NUMPRIO; tid ++) {
			if ((has_stats & (1 << tid)) == 0) {
				continue;
			}
			stats = &tid_stats[tid];
			if (((now_time - scb_taf->timestamp[TAF_TYPE_DL]) > (1 << 22)) &&
				(stats->tx_per_second.npkts == 0)) {

				continue;
			}
			if (output == FALSE) {
				bcm_bprintf(b, "\n    tx tid: packet len, sched airtime, "
					"rel npackets/s, rel ppackets/s, "
					"aggsf achvd/max,     rel tput/s\n");
				output = TRUE;
			}
			bcm_bprintf(b, "         %u: %10u,  %12s,  %13u,  %13u,  %9u / %2u,  "
				"%13s\n",
				tid, stats->tx_release.npacket_len,
				taf_ias_dump_percent_str(pcent, stats->tx_per_second.airtime),
				stats->tx_per_second.npkts,
				stats->tx_per_second.ppkts,
				stats->achvd_aggsf, stats->max_aggsf,
				taf_ias_dump_rate_str(tput, stats->tx_per_second.Kbits));
		}

#if TAF_ENABLE_UL
		/* Display uplink information */
		output = FALSE;
		for (tid = 0; list->type == TAF_TYPE_UL && tid < NUMPRIO; tid ++) {
			int32 deci_pkts;
			uint32 deci_rel_pkts;

			if ((has_stats & (1 << tid)) == 0) {
				continue;
			}
			if (ul_tid_ignore && tid != TAF_UL_PRIO) {
				continue;
			}
			stats = &tid_stats[tid];

			if (((now_time - scb_taf->timestamp[TAF_TYPE_UL]) > (1 << 22)) &&
				(stats->rx_mon.Kbits == 0) && (stats->rx_release.units == 0)) {

				continue;
			}

			/* For taf scheduled stations, display also scheduled target */
			if (output == FALSE) {
				bcm_bprintf(b, "\n    ul tid: ");
#if defined(TAF_DBG) && TAF_ENABLE_UL
				bcm_bprintf(b, "bump interval, ");
#endif
				bcm_bprintf(b, "sched intvl/freq, sched release,  "
					"sched bytes / mpdus, sched packets/s, over dpkts, "
					"sched airtime,  sched tput/s\n");
				output = TRUE;
			}

			bcm_bprintf(b, "         %u: ", tid);
#if defined(TAF_DBG) && TAF_ENABLE_UL
			bcm_bprintf(b, "%11ums, ",
				(stats->rx_per_second.update_interval + 500) / 1000);
#endif
			deci_pkts = taf_ias_sched_rx_deci_overpkts(scb_taf, tid);

			deci_rel_pkts =
				(10 * stats->rx_release.bytes + (stats->rx_mon.packet_len >> 1)) /
				stats->rx_mon.packet_len;

			bcm_bprintf(b, "%8uus /%4u,  %10uus,    "
				"%9u /%4u.%1u, %15u,    %s%6u, %13s,  %12s\n",
				stats->rx_per_second.interval, stats->rx_per_second.count,
				TAF_UNITS_TO_MICROSEC(stats->rx_release.units),
				stats->rx_release.bytes, deci_rel_pkts / 10, deci_rel_pkts % 10,
				stats->rx_per_second.ppkts,
				deci_pkts < 0 ? "-" : " ", ABS(deci_pkts),
				taf_ias_dump_percent_str(pcent, stats->rx_per_second.airtime),
				taf_ias_dump_rate_str(tput, stats->rx_per_second.Kbits));
		}

		output = FALSE;
		for (tid = 0; tid < NUMPRIO; tid ++) {
			bool ul_nosched_output = FALSE;

			if ((has_stats & (1 << tid)) == 0) {
				continue;
			}
			if (ul_tid_ignore && tid != TAF_UL_PRIO) {
				continue;
			}
			stats = &tid_stats[tid];

			/* For non-taf scheduled stations, display only actually received data */
			if (list->type == TAF_TYPE_DL) {
				if (scb_taf->info.mu_tech_en[TAF_TECH_UL_OFDMA] & (1 << tid)) {
					continue;
				}
				if (stats->rx_mon.Kbits == 0) {
					continue;
				}
				ul_nosched_output = TRUE;
			}

			if (output == FALSE) {
				if (ul_nosched_output) {
					bcm_bprintf(b, "\nnon uplink admitted/non scheduled "
						"uplink traffic%s",
						ul_tid_ignore ? " (unified tid)" : "");
				}
				bcm_bprintf(b, "\n    rx tid: pkt len min /  avg /  max,  "
					"rx packets/s, rx agg packets/s, rx agg mean/max,");
#if TAF_ENABLE_UL
				bcm_bprintf(b, " qosnl/s,");
#endif
				bcm_bprintf(b, "     rx tput/s\n");
				output = TRUE;
			}
			bcm_bprintf(b, "         %u:       %5u /%5u /%5u,  %12u,  %15u,  %9u /%3u,",
				tid, stats->rx_mon.min_packet_len,
				stats->rx_mon.packet_len, stats->rx_mon.max_packet_len,
				stats->rx_mon.ppkts, stats->rx_mon.apkts,
				stats->rx_mon.aggn, stats->rx_mon.aggn_max);
#if TAF_ENABLE_UL
			bcm_bprintf(b, " %5u.%1u,", stats->rx_per_second.qosnull_dcount / 10,
				stats->rx_per_second.qosnull_dcount % 10);
#endif
			bcm_bprintf(b, "  %12s\n",
				taf_ias_dump_rate_str(tput, stats->rx_mon.Kbits));
		}
#endif /* TAF_ENABLE_UL */

#ifdef TAF_DBG
		bcm_bprintf(b, "\n");

		for (rindex = TAF_RSPEC_SU_DL; rindex < NUM_TAF_RATES; rindex++) {
			ratespec_t rspec = scb_stats->global.rdata.rate[rindex].rspec;
			uint32 rate = scb_stats->global.rdata.rate[rindex].rate;
			bool sgi = FALSE;

			if ((rspec == 0 && !TAF_RIDX_IS_BOOST(rindex)) ||
				((scb_stats->global.rdata.ratespec_mask & (1 << rindex)) == 0)) {
				continue;
			}
#if TAF_ENABLE_UL
			if (list->type == TAF_TYPE_UL && rindex != TAF_RSPEC_UL) {
				continue;
			}
			if (list->type == TAF_TYPE_DL && rindex == TAF_RSPEC_UL) {
				continue;
			}
#endif
			if (!TAF_RIDX_IS_BOOST(rindex)) {
				switch ((rspec & WL_RSPEC_ENCODING_MASK) >>
					WL_RSPEC_ENCODING_SHIFT) {

					case WL_RSPEC_ENCODE_HE >> WL_RSPEC_ENCODING_SHIFT:
						sgi = HE_IS_GI_0_8us(RSPEC_HE_LTF_GI(rspec));
						break;
					case WL_RSPEC_ENCODE_VHT >> WL_RSPEC_ENCODING_SHIFT:
					case WL_RSPEC_ENCODE_HT >> WL_RSPEC_ENCODING_SHIFT:
						sgi = RSPEC_ISSGI(rspec);
					default:
						break;
				}
			}
#if TAF_ENABLE_MU_BOOST
			if (TAF_RIDX_IS_BOOST(rindex) &&
				(taf_info->mu_boost == TAF_MUBOOST_FACTOR ||
				taf_info->mu_boost == TAF_MUBOOST_RATE_FACTOR)) {

				bcm_bprintf(b, "ridx %u:                   mu clients %u, "
					"boost factor %u,        rate %7u\n",
					rindex,
					scb_taf->info.scb_stats.global.rdata.mu.clients_count,
					taf_ias_boost_factor(scb_taf), rate);
			} else
#endif /* TAF_ENABLE_MU_BOOST */
			{
				bcm_bprintf(b, "ridx %u: rspec 0x%08x, nss %u, bw %3u, mcs %2u, "
					"phy %u, sgi %u, rate %7u\n",
					rindex, rspec, wlc_ratespec_nss(rspec),
					wlc_ratespec_bw(rspec), wlc_ratespec_mcs(rspec),
					(rspec & WL_RSPEC_ENCODING_MASK) >> WL_RSPEC_ENCODING_SHIFT,
					sgi, rate);
			}
		}
		bcm_bprintf(b, "\n");

#if TAF_LOGL3
		for (s_idx = 0; s_idx < TAF_NUM_SCHED_SOURCES; s_idx ++) {
			bool enabled = scb_taf->info.scb_stats.ias.data.use[s_idx];

			if (!taf_src_type_match(s_idx, list->type)) {
				continue;
			}

			bcm_bprintf(b, "%s %sabled\n", TAF_SOURCE_NAME(s_idx),
				enabled ? "en" : "dis");

			if (!enabled) {
				continue;
			}
			for (tid = 0; tid < NUMPRIO; tid++) {
				taf_link_state_t st = scb_taf->info.linkstate[s_idx][tid];
				char * msg = "";
				taf_ias_sched_data_t *ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);

				const char * st_txt = taf_link_states_text[st];

				if (st == TAF_LINKSTATE_NONE || !ias_sched) {
					continue;
				}
#if TAF_ENABLE_SQS_PULL
				if (TAF_SOURCE_IS_SQS(s_idx)) {
					msg = "sqs cap";
				}
#endif /* TAF_ENABLE_SQS_PULL */
				if (TAF_SOURCE_IS_AMPDU(s_idx)) {
					uint32 flow;
					if (!SCB_AMPDU(scb_taf->scb)) {
						msg = "ampdu incap";
					}
					flow = ias_sched->rel_stats.data.total_scaled_ncount;

					bcm_bprintf(b, "%u: [%u] %s (%u/%u/%u) %s\n", tid, st,
						st_txt, flow >> TAF_PKTCOUNT_SCALE_SHIFT,
						SCB_PKTS_INFLT_FIFOCNT_VAL(scb_taf->scb, tid),
						scb_taf->info.traffic.count[s_idx][tid], msg);
				} else {
					bcm_bprintf(b, "%u: [%u] %s (%u) %s\n", tid, st, st_txt,
						scb_taf->info.traffic.count[s_idx][tid], msg);
				}
			}
		}
#endif /* TAF_LOGL3 */
#endif /* TAF_DBG */
		list = list->next;
	}
	MFREE(TAF_WLCT(taf_info)->pub->osh, tid_stats, NUMPRIO * sizeof(*tid_stats));
	return BCME_OK;
}

static void taf_ias_unified_reset(taf_method_info_t* method)
{
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
	taf_ias_uni_state_t* uni_state;

#if TAF_ENABLE_TIMER
	taf_ias_agghold_reset(method);
#endif
	uni_state = taf_get_uni_state(method->taf_info);

	if (uni_state->was_reset) {
		return;
	}

	WL_TAFM1(method, "\n");

	uni_state->trigger = TAF_TRIGGER_NONE;
	uni_state->star_packet_received = FALSE;
	uni_state->super_sched_last = 0;

	uni_state->pairing_id = 0;
	uni_state->last_paired[TAF_TYPE_DL] = 0;
	uni_state->total_units_in_transit[TAF_TYPE_DL] = 0;
#if TAF_ENABLE_UL
	uni_state->last_paired[TAF_TYPE_UL] = 0;
	uni_state->total_units_in_transit[TAF_TYPE_UL] = 0;
#endif
	uni_state->prev_resched_index = -1;
	uni_state->resched_units[0] = 0;
	uni_state->resched_units[1] = 0;
	uni_state->resched_index[0] = ias_info->tag_star_index;
	uni_state->cycle_now = 0;
	uni_state->cycle_next = 0;
	uni_state->cycle_ready = 0;
	uni_state->barrier_req = 0;

	uni_state->high[IAS_INDEXM(method)] = ias_cfg(method).high;
	uni_state->low[IAS_INDEXM(method)] = ias_cfg(method).low;

	memset(&ias_info->data, 0, sizeof(ias_info->data));
	memset(&ias_info->debug, 0, sizeof(ias_info->debug));
	memset(&uni_state->real, 0, sizeof(uni_state->real));
	memset(&uni_state->data, 0, sizeof(uni_state->data));
	memset(&uni_state->total, 0, sizeof(uni_state->total));
	memset(&uni_state->extend, 0, sizeof(uni_state->extend));
	memset(&uni_state->cumul_units, 0, sizeof(uni_state->cumul_units));
	memset(&uni_state->total_sched_units, 0, sizeof(uni_state->total_sched_units));

	uni_state->was_reset ++;

	*method->ready_to_schedule = ~(0);

	WL_TAFM1(method, "r_t_s 0x%x\n", *method->ready_to_schedule);
}

static INLINE void taf_ias_upd_ts2_scb_flag(taf_scb_cubby_t *scb_taf)
{
	struct scb* scb = scb_taf->scb;

	scb->flags3 &= ~(SCB3_TS_MASK);

	switch (scb_taf->method->type) {
		case TAF_EBOS:
			scb->flags3 |= SCB3_TS_EBOS;
			break;
		case TAF_ATOS:
			scb->flags3 |= SCB3_TS_ATOS;
			break;
		case TAF_ATOS2:
			scb->flags3 |= SCB3_TS_ATOS2;
			break;
		default:
			break;
	}
}

static int taf_ias_sched_reset(taf_method_info_t* method, taf_scb_cubby_t *scb_taf, int tid,
	taf_source_type_t s_idx)
{
	wlc_taf_info_t* taf_info = method->taf_info;
	taf_ias_sched_data_t *ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);

	TAF_ASSERT(ias_sched);

	if (!ias_sched) {
		return BCME_ERROR;
	}

	if (TAF_IAS_SOURCE_ENABLED(ias_sched, s_idx)) {
		void* src_h = *(taf_info->source_handle_p[s_idx]);
		TAF_ASSERT(src_h);

		WL_TAFM1(method, "reset %s for "MACF"%s tid %u\n", TAF_SOURCE_NAME(s_idx),
			TAF_ETHERC(scb_taf), TAF_TYPE(TAF_SRC_TO_TYPE(s_idx)), tid);

		ias_sched->handle[s_idx].scbh =
			taf_info->funcs[s_idx].scb_h_fn(src_h, scb_taf->scb);
		ias_sched->handle[s_idx].tidh =
			taf_info->funcs[s_idx].tid_h_fn(ias_sched->handle[s_idx].scbh, tid);

		ias_cfg(method).tid_active |= (1 << tid);
		scb_taf->info.traffic.map[s_idx] |= (1 << tid);

		if (TAF_SOURCE_IS_DL(s_idx)) {
			ias_sched->rel_stats.data.calc_timestamp =
				scb_taf->timestamp[TAF_TYPE_DL];
			/* initial bias in packet averaging initialisation */
			ias_sched->rel_stats.data.pkt_size_mean[TAF_TYPE_DL] =
				TAF_PKT_SIZE_DEFAULT_DL;
			ias_sched->rel_stats.data.total_scaled_ncount =
				(4 << TAF_PKTCOUNT_SCALE_SHIFT);
		}

		taf_ias_flush_rspec(scb_taf);

		WL_TAFM1(method, "ias_sched for "MACF"%s tid %u source %s reset "
			"r_t_s 0x%x\n",  TAF_ETHERC(scb_taf), TAF_TYPE(TAF_SRC_TO_TYPE(s_idx)), tid,
			TAF_SOURCE_NAME(s_idx), *method->ready_to_schedule);
	} else {
		ias_sched->handle[s_idx].scbh = NULL;
		ias_sched->handle[s_idx].tidh = NULL;

		WL_TAFM1(method, "ias_sched for "MACF"%s tid %u source %s unused r_t_s "
			"0x%x\n", TAF_ETHERC(scb_taf), TAF_TYPE(TAF_SRC_TO_TYPE(s_idx)), tid,
			TAF_SOURCE_NAME(s_idx), *method->ready_to_schedule);
	}
	return BCME_OK;
}

static int taf_ias_tid_remove(taf_method_info_t* method, taf_scb_cubby_t *scb_taf, int tid)
{
	taf_ias_sched_data_t *ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);

	TAF_ASSERT(ias_sched);
	if (ias_sched) {
#if TAF_IAS_STATIC_CONTEXT
		WL_TAFM2(method, "ias_sched for "MACF" tid %u reset NULL (in flight count %u)\n",
			TAF_ETHERC(scb_taf), tid, SCB_PKTS_INFLT_FIFOCNT_VAL(scb_taf->scb, tid));
#else
		MFREE(TAF_WLCM(method)->pub->osh, ias_sched, sizeof(*ias_sched));
		WL_TAFM2(method, "ias_sched for "MACF" tid %u destroyed (in flight count %u)\n",
			TAF_ETHERC(scb_taf), tid, SCB_PKTS_INFLT_FIFOCNT_VAL(scb_taf->scb, tid));
#endif
		TAF_CUBBY_TIDINFO(scb_taf, tid) = NULL;
	}
	scb_taf->info.tid_enabled &= ~(1 << tid);
	scb_taf->info.traffic.est_units[TAF_TYPE_DL][tid] = 0;
	scb_taf->info.traffic.available[TAF_TYPE_DL] &= ~(1 << tid);
#if TAF_ENABLE_UL
	scb_taf->info.traffic.est_units[TAF_TYPE_UL][tid] = 0;
	scb_taf->info.traffic.available[TAF_TYPE_UL] &= ~(1 << tid);
#endif
#if TAF_ENABLE_SQS_PULL
	if (scb_taf->info.pkt_pull_request[tid] > 0) {
		WL_TAFM1(method, "pkt pull req for "MACF" tid %u was %u, total pull count %u, "
			"reset\n", TAF_ETHERC(scb_taf), tid, scb_taf->info.pkt_pull_request[tid],
			method->taf_info->total_pull_requests);

		--method->taf_info->total_pull_requests;
		WL_TAFT1(method->taf_info, "total_pull_requests %u\n",
			method->taf_info->total_pull_requests);
		scb_taf->info.pkt_pull_request[tid] = 0;
		scb_taf->info.pkt_pull_map &= ~(1 << tid);
	}
#endif /* TAF_ENABLE_SQS_PULL */
	return BCME_OK;
}

static int taf_ias_sched_init(taf_method_info_t* method, taf_scb_cubby_t *scb_taf, int tid)
{
	wlc_info_t *wlc = TAF_WLCM(method);
	taf_ias_sched_data_t *ias_sched;

	TAF_ASSERT(TAF_CUBBY_TIDINFO(scb_taf, tid) == NULL);

#if TAF_IAS_STATIC_CONTEXT
	BCM_REFERENCE(wlc);

	ias_sched = (taf_ias_sched_data_t*) &scb_taf->info.static_context[tid][0];

	WL_TAFM2(method, "ias_sched for "MACF" tid %u static assigned\n", TAF_ETHERC(scb_taf), tid);
#else
	ias_sched = (taf_ias_sched_data_t*) MALLOCZ(wlc->pub->osh, sizeof(*ias_sched));

	if (!ias_sched) {
		WL_ERROR(("wl%u %s: malloc fail (%u)\n", WLCWLUNIT(wlc), __FUNCTION__,
			(uint32)sizeof(*ias_sched)));
		return BCME_NOMEM;
	}
		WL_TAFM2(method, "ias_sched for "MACF" tid %u created\n", TAF_ETHERC(scb_taf), tid);
#endif

	TAF_CUBBY_TIDINFO(scb_taf, tid) = ias_sched;

	scb_taf->info.tid_enabled |= (1 << tid);

	return BCME_OK;
}

bool taf_ias_link_state(void *handle, struct scb* scb, int tid, taf_source_type_t s_idx,
	taf_link_state_t state)
{
	taf_method_info_t* method = handle;
	taf_scb_cubby_t* scb_taf = *SCB_TAF_CUBBY_PTR(method->taf_info, scb);
	int status = BCME_OK;
	taf_ias_sched_data_t* ias_sched;

	TAF_ASSERT(scb_taf || (state == TAF_LINKSTATE_NOT_ACTIVE) ||
		(state == TAF_LINKSTATE_REMOVE));

	if (!scb_taf) {
		return FALSE;
	}

	if (s_idx >= TAF_SOURCE_UNDEFINED) {
		if (state == TAF_LINKSTATE_HARD_RESET) {
			taf_ias_unified_reset(method);
		}
		return FALSE;
	}
	ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);

	switch (state) {
		case TAF_LINKSTATE_INIT:
			if (!ias_sched) {
				status = taf_ias_sched_init(method, scb_taf, tid);
				if (status == BCME_OK) {
					ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);
				}
			}
			if (ias_sched) {
				scb_taf->info.linkstate[s_idx][tid] = state;
			}
			break;

		case TAF_LINKSTATE_ACTIVE:
			if (scb_taf->info.linkstate[s_idx][tid] == TAF_LINKSTATE_NONE) {
				if (!ias_sched) {
					status = taf_ias_sched_init(method, scb_taf, tid);
					if (status == BCME_OK) {
						ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);
					}
				}
			}
			if (ias_sched) {
				ias_sched->used |= (1 << s_idx);
				status = taf_ias_sched_reset(method, scb_taf, tid, s_idx);
				WL_TAFM2(method, "ias_sched for "MACF" tid %u %s active\n",
					TAF_ETHERS(scb), tid, TAF_SOURCE_NAME(s_idx));
				scb_taf->info.linkstate[s_idx][tid] = state;
			}
			break;

		case TAF_LINKSTATE_NOT_ACTIVE:
			if (ias_sched) {
				ias_sched->used &= ~(1 << s_idx);
				status = taf_ias_sched_reset(method, scb_taf, tid, s_idx);
			}
			scb_taf->info.linkstate[s_idx][tid]  = state;
			WL_TAFM2(method, "ias_sched for "MACF" tid %u %s now inactive\n",
				TAF_ETHERS(scb), tid, TAF_SOURCE_NAME(s_idx));
			break;

		case TAF_LINKSTATE_NONE:
		case TAF_LINKSTATE_REMOVE:
			scb_taf->info.linkstate[s_idx][tid] = TAF_LINKSTATE_NONE;
			scb_taf->info.traffic.count[s_idx][tid] = 0;
			scb_taf->info.traffic.map[s_idx] &= ~(1 << tid);

			if (ias_sched) {
				taf_source_type_t _idx;
				bool allfree = TRUE;

				ias_sched->used &= ~(1 << s_idx);
				for (_idx = 0; _idx < TAF_NUM_SCHED_SOURCES; _idx++) {

					if (scb_taf->info.linkstate[_idx][tid] !=
							TAF_LINKSTATE_NONE) {
						allfree = FALSE;
						break;
					}
				}
				WL_TAFM2(method, "%s for %s "MACF" tid %u%s\n",
					TAF_LINK_STATE_TXT(state), TAF_SOURCE_NAME(s_idx),
					TAF_ETHERS(scb), tid, allfree ? " all free" : "");

				if (allfree) {
					status = taf_ias_tid_remove(method, scb_taf, tid);
				}
			}
			break;

		case TAF_LINKSTATE_HARD_RESET:
			if (ias_sched) {
				taf_ias_sched_reset(method, scb_taf, tid, s_idx);
			}
			taf_ias_unified_reset(method);
			break;

		case TAF_LINKSTATE_SOFT_RESET:
			if (ias_sched) {
				taf_ias_sched_reset(method, scb_taf, tid, s_idx);
			}
			break;

		default:
			status = BCME_ERROR;
			TAF_ASSERT(0);
	}
	/*
	 * return TRUE if we handled it
	 */
	if (status == BCME_OK) {
		return TRUE;
	}
	return FALSE;
}

static int taf_ias_set_source(taf_method_info_t* method, taf_scb_cubby_t* scb_taf,
	taf_source_type_t s_idx, bool set)
{
	if (scb_taf) {
		if (set && !scb_taf->info.scb_stats.ias.data.use[s_idx]) {
			/* flush rspec in order to calculate rate info again */
			taf_ias_flush_rspec(scb_taf);
		}
		scb_taf->info.scb_stats.ias.data.use[s_idx] = set;
		if (!set) {
			scb_taf->info.traffic.map[s_idx] = 0;
		}

		if (TAF_SOURCE_IS_AMPDU(s_idx) && set) {
			wlc_taf_info_t* taf_info = method->taf_info;
			void ** source_h = taf_info->source_handle_p[s_idx];
			void * scb_h = taf_info->funcs[s_idx].scb_h_fn(*source_h, scb_taf->scb);
			scb_taf->info.max_pdu = wlc_ampdu_get_taf_max_pdu(scb_h);
		} else {
			scb_taf->info.max_pdu = 0;
		}
		return BCME_OK;
	}
	return BCME_ERROR;
}

static uint32 taf_ias_dltraffic_wait(taf_method_info_t* method, struct scb* scb)
{
	wlc_taf_info_t* taf_info = method->taf_info;
	taf_scb_cubby_t* scb_taf = *SCB_TAF_CUBBY_PTR(taf_info, scb);
	uint32 pending = 0;
	int tid;
	taf_source_type_t s_idx;

	for (s_idx = 0; s_idx < TAF_NUM_SCHED_SOURCES; s_idx ++) {
		uint16 (*pktqlen_fn) (void *, void *, void *, uint32) =
			taf_info->funcs[s_idx].pktqlen_fn;
		void * srch = *(taf_info->source_handle_p[s_idx]);

		if (!TAF_SOURCE_IS_DL(s_idx)) {
			/* this counts DL traffic only */
			continue;
		}
		for (tid = 0; tid < TAF_NUMPRIO(s_idx); tid++) {
			taf_ias_sched_data_t* ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);

			scb_taf->info.traffic.map[s_idx] &= ~(1 << tid);

			if (TAF_PRIO_SUSPENDED(scb_taf, tid, TAF_TYPE_DL)) {
				continue;
			}

			if (ias_sched && TAF_IAS_SOURCE_ENABLED(ias_sched, s_idx) && pktqlen_fn) {
				uint16 pktq_len;
				pktq_len = pktqlen_fn(srch, ias_sched->handle[s_idx].scbh,
					ias_sched->handle[s_idx].tidh, TAF_IAS_NOWTIME(method));
				scb_taf->info.traffic.count[s_idx][tid] = pktq_len;
				pending += pktq_len;
				if (pktq_len) {
					scb_taf->info.traffic.map[s_idx] |= (1 << tid);
					scb_taf->info.traffic.available[TAF_SRC_TO_TYPE(s_idx)] |=
						(1 << tid);
				}
			}
		}
	}
	WL_TAFM1(method, MACF" - %u\n", TAF_ETHERS(scb), pending);
	return pending;
}

static INLINE void taf_ias_scb_ps_mode(taf_method_info_t* method, taf_scb_cubby_t* scb_taf)
{
	wlc_taf_info_t* taf_info = method->taf_info;
#if TAF_ENABLE_SQS_PULL
	int tid;
#endif

#ifdef TAF_DBG
#if TAF_ENABLE_SQS_PULL
	char debug[48];
	char * taf_debug = debug;
#endif
	uint32 elapsed = 0;

	if (!scb_taf) {
		return;
	}

	TAF_IAS_NOWTIME_SYNC(method, taf_timestamp(TAF_WLCT(taf_info)));

	if (scb_taf->info.ps_mode) {
		scb_taf->info.scb_stats.ias.debug.ps_enter = TAF_IAS_NOWTIME(method);
		scb_taf->info.scb_stats.ias.debug.ps_enter_index = taf_info->scheduler_index;
	} else {
		elapsed = TAF_IAS_NOWTIME(method) - scb_taf->info.scb_stats.ias.debug.ps_enter;
	}

#if TAF_ENABLE_SQS_PULL
	if (scb_taf->info.pkt_pull_map) {
		snprintf(debug, sizeof(debug), ": %u,%u,%u,%u,%u,%u,%u,%u",
			scb_taf->info.pkt_pull_request[0],
			scb_taf->info.pkt_pull_request[1],
			scb_taf->info.pkt_pull_request[2],
			scb_taf->info.pkt_pull_request[3],
			scb_taf->info.pkt_pull_request[4],
			scb_taf->info.pkt_pull_request[5],
			scb_taf->info.pkt_pull_request[6],
			scb_taf->info.pkt_pull_request[7]);
	} else {
		taf_debug = "";
	}
#endif

	WL_TAFT1(taf_info, MACF" PS mode O%s "
#if TAF_ENABLE_SQS_PULL
		"(pkt_pull_map 0x%02x%s), "
#endif
		"op state %d, "
		"elapsed %u, %x/%x (%d)\n",
		TAF_ETHERC(scb_taf), scb_taf->info.ps_mode ? "N" : "FF",
#if TAF_ENABLE_SQS_PULL
		scb_taf->info.pkt_pull_map, taf_debug,
#endif
		taf_info->op_state, elapsed,
		taf_info->scheduler_index,
		scb_taf->info.scb_stats.ias.debug.ps_enter_index,
		taf_info->scheduler_index - scb_taf->info.scb_stats.ias.debug.ps_enter_index);
#endif /* TAF_DBG */

#if TAF_ENABLE_SQS_PULL
	if (scb_taf->info.ps_mode == 0 && scb_taf->info.pkt_pull_map) {
		for (tid = 0; tid < NUMPRIO; tid ++) {
			if (scb_taf->info.pkt_pull_request[tid]) {
				TAF_ASSERT(scb_taf->info.pkt_pull_map & (1 << tid));
				WL_TAFT2(taf_info, MACF" tid %u clearing pull req %u\n",
					TAF_ETHERC(scb_taf), tid,
					scb_taf->info.pkt_pull_request[tid]);
				scb_taf->info.pkt_pull_request[tid] = 0;
			}
		}
		scb_taf->info.pkt_pull_map = 0;
	}
#endif /* TAF_ENABLE_SQS_PULL */

#if TAF_ENABLE_TIMER
	if (!scb_taf->info.ps_mode) {
		WL_TAFM2(method, "prevent agg hold due to power save exit\n");
		taf_ias_agghold_prevent(TAF_IAS_GROUP_INFO(taf_info));
	}
#endif
	BCM_REFERENCE(taf_info);
}

#if TAF_ENABLE_MU_TX
static INLINE
void taf_ias_mu_state_upd(wlc_taf_info_t* taf_info, taf_scb_cubby_t* scb_taf, taf_tech_type_t idx,
	taf_traffic_map_t new_state)
{
	/* XXX for now assume all TID are active/not active together.
	* This should be move to "linkstate" handler once external
	* system is controlling this at TID level and not SCB level
	*/
	taf_traffic_map_t prev_state = scb_taf->info.mu_tech_en[idx];
#if TAF_ENABLE_SQS_PULL
	taf_traffic_map_t state_upd;
#endif

	scb_taf->info.mu_tech_en[idx] = new_state;

	if (new_state && prev_state == 0) {
		taf_info->mu_admit_count[idx]++;

	} else if (prev_state && new_state == 0) {
		TAF_ASSERT(taf_info->mu_admit_count[idx]);
		if (taf_info->mu_admit_count[idx]) {
			taf_info->mu_admit_count[idx]--;
		}
	}

	if (scb_taf->info.mu_tech_en[idx]) {
		scb_taf->info.tech_enable_mask |= (1 << idx);
	} else {
		scb_taf->info.tech_enable_mask &= ~(1 << idx);
	}

#if TAF_ENABLE_MU_BOOST
	taf_info->mu_boost_factor[idx] = MIN(TAF_WLCT(taf_info)->stf->op_txstreams,
		taf_info->mu_admit_count[idx]);
#endif
	if (!TAF_TECH_MASK_IS_MUMIMO(scb_taf->info.tech_enable_mask)) {
		memset(&scb_taf->info.scb_stats.global.rdata.mu,
		       0, sizeof(scb_taf->info.scb_stats.global.rdata.mu));
	}

	WL_TAFM1(scb_taf->method, MACF" set %s to %sable (admit count %u)\n", TAF_ETHERC(scb_taf),
		TAF_TECH_NAME(idx), scb_taf->info.mu_tech_en[idx] ? "en" : "dis",
		taf_info->mu_admit_count[idx]);

#if TAF_ENABLE_UL
	if (idx == TAF_TECH_UL_OFDMA) {
		/* UL transitions are benign here, return */
		return;
	}
#endif
#if TAF_ENABLE_SQS_PULL
	if (taf_info->op_state == TAF_MARKUP_REAL_PACKETS &&
		((state_upd = (new_state ^ prev_state) & scb_taf->info.pkt_pull_map) != 0)) {
		int tid;

		WL_ERROR(("wl%u %s: "MACF" %s state change (0x%x -> 0x%x) while waiting for vpkts "
			"(map 0x%x)\n", WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__,
			TAF_ETHERC(scb_taf), TAF_TECH_NAME(idx), prev_state,
			scb_taf->info.mu_tech_en[idx], scb_taf->info.pkt_pull_map));

		for (tid = 0; tid < NUMPRIO; tid++) {
			taf_ias_sched_data_t* ias_sched;

			if (!(state_upd & (1 << tid))) {
				continue;
			}

			if ((ias_sched = TAF_IAS_TID_STATE(scb_taf, tid))) {
				scb_taf->info.tech_type[TAF_TYPE_DL][tid] = TAF_TECH_DL_SU;

				if (ias_sched->release_config[TAF_TYPE_DL].units >= 0) {
					/* set release limit to 1 (minimum release)  */
					ias_sched->release_config[TAF_TYPE_DL].units = 1;
				}
			}
		}
	}
#endif /* TAF_ENABLE_SQS_PULL */
}
#endif /* TAF_ENABLE_MU_TX */

int
taf_ias_scb_state(void *handle, struct scb* scb, taf_source_type_t s_idx,
	void* data, taf_scb_state_t state)
{
	taf_method_info_t* method = handle;
	taf_scb_cubby_t* scb_taf = *SCB_TAF_CUBBY_PTR(method->taf_info, scb);
	int status = BCME_OK;
	int tid;
	bool active;

	switch (state) {

		case TAF_SCBSTATE_INIT:
			WL_TAFM2(method, MACF" SCB INIT\n", TAF_ETHERS(scb));
			taf_ias_upd_ts2_scb_flag(scb_taf);
			TAF_IAS_NOWTIME_SYNC(method, taf_timestamp(TAF_WLCM(method)));
			scb_taf->timestamp[TAF_TYPE_DL] = TAF_IAS_NOWTIME(method);
#if TAF_ENABLE_UL
			scb_taf->timestamp[TAF_TYPE_UL] = TAF_IAS_NOWTIME(method);
#endif
			break;

		case TAF_SCBSTATE_EXIT:
			WL_TAFM2(method, MACF" SCB EXIT\n", TAF_ETHERS(scb));
			break;

		case TAF_SCBSTATE_RESET:
			if (scb_taf) {
				taf_list_scoring_order_t order = method->ordering;

				taf_ias_flush_rspec(scb_taf);

				for (tid = 0; tid < TAF_MAXPRIO; tid++) {
					taf_ias_sched_data_t* ias_sched =
						TAF_IAS_TID_STATE(scb_taf, tid);
#if TAF_ENABLE_SQS_PULL
					scb_taf->info.pkt_pull_request[tid] = 0;
#endif /* TAF_ENABLE_SQS_PULL */
					for (s_idx = TAF_FIRST_REAL_SOURCE;
						s_idx < TAF_NUM_SCHED_SOURCES; s_idx++) {

						scb_taf->info.traffic.map[s_idx] = 0;
						scb_taf->info.traffic.count[s_idx][tid] = 0;
					}
					if (ias_sched) {
						ias_sched->release_config[TAF_TYPE_DL].whole = 0;
#if TAF_ENABLE_UL
						ias_sched->release_config[TAF_TYPE_UL].whole = 0;
						taf_ias_rx_mon_clear(method, &ias_sched->rel_stats,
							TAF_IAS_NOWTIME(method));
#endif
					}
				}
				scb_taf->info.traffic.available[TAF_TYPE_DL] = 0;
				scb_taf->timestamp[TAF_TYPE_DL] = TAF_IAS_NOWTIME(method);
				scb_taf->info.list[TAF_TYPE_DL].used = 0;
				scb_taf->info.list[TAF_TYPE_DL].bridged = 0;
#if TAF_ENABLE_UL
				scb_taf->info.traffic.available[TAF_TYPE_UL] = 0;
				scb_taf->timestamp[TAF_TYPE_UL] = scb_taf->timestamp[TAF_TYPE_DL];
				scb_taf->info.list[TAF_TYPE_UL].used = 0;
				scb_taf->info.list[TAF_TYPE_DL].bridged = 0;
#endif
				scb_taf->info.ps_mode = SCB_PS(scb);
				scb_taf->info.force = 0;

				if (order == TAF_LIST_SCORE_MINIMUM ||
					order == TAF_LIST_SCORE_MAXIMUM) {
					scb_taf->score[TAF_TYPE_DL] = method->score_init;
#if TAF_ENABLE_UL
					scb_taf->score[TAF_TYPE_UL] = method->score_init;
#endif
				}
#if TAF_ENABLE_SQS_PULL
				scb_taf->info.pkt_pull_map = 0;
				scb_taf->info.pkt_pull_dequeue = 0;
#endif /* TAF_ENABLE_SQS_PULL */
				memset(&scb_taf->info.suspend, 0, sizeof(scb_taf->info.suspend));
#ifdef TAF_DBG
				memset(&scb_taf->info.scb_stats.ias.debug, 0,
				       sizeof(scb_taf->info.scb_stats.ias.debug));
#endif
				WL_TAFM1(method, MACF" SCB RESET\n", TAF_ETHERS(scb));
			}
			break;

		case TAF_SCBSTATE_SOURCE_ENABLE:
		case TAF_SCBSTATE_SOURCE_DISABLE:
			if (s_idx != TAF_SOURCE_UNDEFINED) {
				active = (state != TAF_SCBSTATE_SOURCE_DISABLE) ? TRUE : FALSE;
				status = taf_ias_set_source(method, scb_taf, s_idx, active);
				WL_TAFM1(method, MACF" %s %s\n", TAF_ETHERS(scb),
					TAF_SOURCE_NAME(s_idx), active ? "ON" : "OFF");
			}
			break;

		case TAF_SCBSTATE_SOURCE_UPDATE:
			active = scb_taf->info.scb_stats.ias.data.use[s_idx];
			status = taf_ias_set_source(method, scb_taf, s_idx, active);
			WL_TAFM3(method, MACF" %s update\n", TAF_ETHERS(scb),
				TAF_SOURCE_NAME(s_idx));
			break;

		case TAF_SCBSTATE_POWER_SAVE:
			taf_ias_scb_ps_mode(method, scb_taf);
			break;

		case TAF_SCBSTATE_NONE:
			break;

		case TAF_SCBSTATE_GET_TRAFFIC_ACTIVE:
			/* this is a GET query, with "update" being a uint32 pointer to return
			 * pending traffic; typically called from wlc_apps for pvb update
			 */
			TAF_ASSERT(data);
			* (uint32 *) data = taf_ias_dltraffic_wait(method, scb);
			break;

		case TAF_SCBSTATE_MU_DL_VHMIMO:
		case TAF_SCBSTATE_MU_DL_HEMIMO:
		case TAF_SCBSTATE_MU_DL_OFDMA:
		case TAF_SCBSTATE_MU_UL_OFDMA:
#if TAF_ENABLE_MU_TX
			if (scb_taf) {
				taf_tech_type_t idx;

				switch (state) {
					case TAF_SCBSTATE_MU_DL_VHMIMO:
						idx = TAF_TECH_DL_VHMUMIMO;
						break;
					case TAF_SCBSTATE_MU_DL_HEMIMO:
						idx = TAF_TECH_DL_HEMUMIMO;
						break;
					case TAF_SCBSTATE_MU_DL_OFDMA:
						idx = TAF_TECH_DL_OFDMA;
						break;
#if TAF_ENABLE_UL
					case TAF_SCBSTATE_MU_UL_OFDMA:
						if (!taf_ul_enabled(method->taf_info)) {
							return BCME_ERROR;
						}
						idx = TAF_TECH_UL_OFDMA;
						wlc_taf_handle_ul_transition(scb_taf,
							data ? TRUE : FALSE);
						break;
#endif /* TAF_ENABLE_UL */
					default:
						return BCME_ERROR;
				}

				taf_ias_mu_state_upd(method->taf_info, scb_taf, idx,
					(taf_traffic_map_t) ((data) ? ~0 : 0));
			}
#endif /* TAF_ENABLE_MU_TX */
			break;

		case TAF_SCBSTATE_UPDATE_BSSCFG:
		case TAF_SCBSTATE_DWDS:
		case TAF_SCBSTATE_WDS:
		case TAF_SCBSTATE_OFF_CHANNEL:
		case TAF_SCBSTATE_DATA_BLOCK_OTHER:
			WL_TAFM4(method, MACF" TO DO - %s (%u)\n", TAF_ETHERS(scb),
				taf_scb_states_text[state], state);

			break;
		default:
			TAF_ASSERT(0);
	}
	return status;
}

/* Wrapper function to enable/disable rate overide for the entire driver,
 * called by rate override code
 */
static bool taf_ias_rate_override(void* handle, ratespec_t rspec, wlcband_t *band)
{
	taf_method_info_t* method = handle;
	taf_list_t* list = taf_ias_get_list_head(method);

	WL_TAFM3(method, "0x%x\n", rspec);

	while (list) {
		taf_scb_cubby_t *scb_taf = list->scb_taf;

		taf_ias_flush_rspec(scb_taf);
		list = list->next;
	}
	return BCME_OK;
}

static bool BCMFASTPATH
taf_ias_handle_star(taf_method_info_t* method, int tid, uint32 pkttag,
	uint8 index, taf_list_type_t type)
{
	wlc_taf_info_t* taf_info = method->taf_info;
	taf_ias_uni_state_t* uni_state = taf_get_uni_state(taf_info);
	taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
	bool prev_complete = TRUE;
	int8 last_sched = -1;
	uint32 pkttag_units;
	uint32 now_time = 0;
	uint32 cumul_units;

	if (type == TAF_TYPE_DL) {
		uni_state->cumul_units[index][type] += (pkttag_units = TAF_PKTTAG_TO_UNITS(pkttag));
	}
#if TAF_ENABLE_UL
	else {
		uni_state->cumul_units[index][type] += (pkttag_units = pkttag);
	}
#endif /* TAF_ENABLE_UL */

	if (uni_state->total_units_in_transit[type] >= pkttag_units) {
		uni_state->total_units_in_transit[type] -= pkttag_units;
	} else {
		WL_ERROR(("wl%u %s: (%x) overflow (%u)%s %u/%u/%u/%u\n",
			WLCWLUNIT(TAF_WLCT(taf_info)), __FUNCTION__, taf_info->scheduler_index,
			index, TAF_TYPE(type), pkttag_units,
			TAF_IAS_CUMUL(uni_state, index, type),
			TAF_IAS_TOTAL_SCHED(uni_state, index, type),
			uni_state->total_units_in_transit[type]));
		++ias_info->debug.traffic_stat_overflow;
#ifdef TAF_DEBUG_VERBOSE
		taf_memtrace_dump(taf_info);
#endif
		uni_state->total_units_in_transit[type] = 0;
	}

	if (ias_cfg(method).barrier && (last_sched = uni_state->prev_resched_index) >= 0) {
		last_sched -= (ias_cfg(method).barrier - 1);
		if (last_sched < 0) {
			last_sched += TAF_MAX_NUM_WINDOWS;
		}
		if (TAF_IAS_CUMUL(uni_state, last_sched, TAF_TYPE_ALL) <
				TAF_IAS_TOTAL_SCHED(uni_state, last_sched, TAF_TYPE_ALL)) {
			prev_complete = FALSE;
		}
	}

	cumul_units = TAF_IAS_CUMUL(uni_state, uni_state->resched_index[0], TAF_TYPE_ALL);

#ifdef TAF_DEBUG_VERBOSE
	if (cumul_units >= uni_state->resched_units[0]) {

		if (uni_state->waiting_schedule) {
			if (ias_info->debug.wait_start == 0) {
				WL_TAFT2(taf_info, "waiting for super scheduler\n");
				ias_info->debug.wait_start =
					taf_nowtime(TAF_WLCT(taf_info), &now_time);
			}
		} else {
			if (ias_info->debug.wait_start != 0) {
				uint32 elapsed = taf_nowtime(TAF_WLCT(taf_info), &now_time) -
					ias_info->debug.wait_start;

				ias_info->debug.wait_start = 0;
				WL_TAFT2(taf_info, "supersched wait time %u us\n", elapsed);
			}
			if (!prev_complete && ias_info->debug.barrier_start == 0) {
				ias_info->debug.barrier_start =
					taf_nowtime(TAF_WLCT(taf_info), &now_time);
				WL_TAFT2(taf_info, "waiting barrier (prev window %d)\n",
					last_sched);
			}
			if (prev_complete && ias_info->debug.barrier_start != 0) {
				uint32 elapsed = taf_nowtime(TAF_WLCT(taf_info), &now_time) -
					ias_info->debug.barrier_start;

				ias_info->debug.barrier_start = 0;

				WL_TAFT2(taf_info, "barrier total elapsed (waiting window %d) "
					"%u us\n", last_sched, elapsed);
			}
		}
	}
#endif /* TAF_DEBUG_VERBOSE */

	if (cumul_units >= uni_state->resched_units[0] && prev_complete &&
		!uni_state->waiting_schedule) {

		if (uni_state->star_packet_received) {
			return TRUE;
		}
		uni_state->star_packet_received = TRUE;

		*method->ready_to_schedule = ~(0);

		WL_TAFT1(taf_info, "cumulative %d/%d  reschedule=%d, complete\n\n",
			uni_state->resched_index[0],
			TAF_UNITS_TO_MICROSEC(cumul_units),
			TAF_UNITS_TO_MICROSEC(uni_state->resched_units[0]));

		uni_state->prev_resched_index = uni_state->resched_index[0];

		if (uni_state->cycle_next == 1 && uni_state->cycle_now == 0) {
			if (uni_state->cycle_ready) {
				uni_state->resched_index[0]  = uni_state->resched_index[1];
				uni_state->resched_units[0]  = uni_state->resched_units[1];
				uni_state->star_packet_received = FALSE;
				uni_state->cycle_now = 1;
				uni_state->waiting_schedule = 1;
			} else  {
				WL_TAFT2(taf_info, "super sched not ready\n");
			}
		} else if (uni_state->cycle_next == 0 && uni_state->cycle_now == 1) {
			uni_state->cycle_now = 0;
		} else {
			uni_state->resched_index[0] = ias_info->tag_star_index;
			uni_state->cycle_now = 0;
			uni_state->cycle_next = 0;
		}
		if (taf_info->super_active) {
			WL_TAFT1(taf_info, "super sched: wait %u/%u, cycle now %u; cycle next "
				"%u, prev index %u, ready %u, wait sched %u\n\n",
				uni_state->resched_index[0],
				TAF_UNITS_TO_MICROSEC(uni_state->resched_units[0]),
				uni_state->cycle_now, uni_state->cycle_next,
				uni_state->prev_resched_index, uni_state->cycle_ready,
				uni_state->waiting_schedule);
		}
	}

	if (now_time != 0) {
		TAF_IAS_NOWTIME_SYNC(method, now_time);
	}

	return TRUE;
}

/* function has to be 'safe' in case scb_taf is NULL */
static bool BCMFASTPATH taf_ias_pkt_lost(taf_method_info_t* method, taf_scb_cubby_t* scb_taf,
	int tid, void * p, taf_txpkt_state_t status, taf_list_type_t type)
{
	taf_ias_uni_state_t* uni_state = taf_get_uni_state(method->taf_info);
	uint16 pkttag_index = TAF_PKT_TAG_INDEX(p, type);
	uint16 pkttag_units = TAF_PKT_TAG_UNITS(p, type);
	uint32 taf_pkt_time_units = TAF_PKTTAG_TO_UNITS(pkttag_units);

	BCM_REFERENCE(uni_state);
	WL_TAFT3(method->taf_info, MACF"/%u, %u - %u/%u (%u) %s (tuit %u)\n",
		TAF_SAFE_ETHERC(scb_taf), tid, (uint32)TAF_PKTTAG_TO_MICROSEC(pkttag_units),
		pkttag_index, TAF_UNITS_TO_MICROSEC(TAF_IAS_TUIT(uni_state, pkttag_index, type)),
		uni_state->resched_index[0], taf_txpkt_status_text[status],
		TAF_IAS_TUIT(uni_state, type));

	switch (method->type) {
		case TAF_EBOS:
			break;

		case TAF_ATOS:
		case TAF_ATOS2:
		if (scb_taf) {
			uint32 taf_pkt_time_score = taf_ias_units_to_score(method, scb_taf, tid,
				scb_taf->info.tech_type[type][tid], taf_pkt_time_units);

			if (TAF_TYPE_SCORE(scb_taf, type) > taf_pkt_time_score) {
				TAF_TYPE_SCORE(scb_taf, type) -= taf_pkt_time_score;
			} else {
				TAF_TYPE_SCORE(scb_taf, type) = 0;
			}
		}
			break;

		case TAF_PSEUDO_RR:
			/* not supported */
		default:
			TAF_ASSERT(0);
			break;
	}
	return taf_ias_handle_star(method, tid, pkttag_units, pkttag_index, type);
}

#if TAF_ENABLE_UL
static void taf_ias_rx_mon_clear(taf_method_info_t* method, taf_ias_sched_rel_stats_t* rel_stats,
	uint32 now_time)
{
	WL_TAFM2(method, "\n");

	memset(&rel_stats->data.ul, 0, sizeof(rel_stats->data.ul));
	memset(&rel_stats->data.rx, 0, sizeof(rel_stats->data.rx));

	rel_stats->data.rx.max_aggn_timestamp =
		rel_stats->data.rx.max_plen_timestamp =
		rel_stats->data.rx.min_plen_timestamp =
		rel_stats->data.rx.prev_timestamp =
		rel_stats->data.rx.calc_timestamp = now_time - TAF_IAS_RXMON_HOLD_LARGE;

	rel_stats->data.pkt_size_mean[TAF_TYPE_UL] =
		rel_stats->data.pkt_size_min[TAF_TYPE_UL] =
		rel_stats->data.pkt_size_max[TAF_TYPE_UL] = TAF_PKT_SIZE_DEFAULT_UL;
}

static INLINE void BCMFASTPATH taf_ias_rx_mon_bytes(taf_scb_cubby_t* scb_taf,
	taf_ias_sched_rel_stats_t* rel_stats, taf_rxpkt_stats_t * stats, uint32 now_time)
{
	rel_stats->data.rx.time += now_time - rel_stats->data.rx.prev_timestamp;
	rel_stats->data.rx.prev_timestamp = now_time;

	rel_stats->data.rx.bytes += stats->total_agglen;

	if (rel_stats->data.pkt_size_mean[TAF_TYPE_UL] == 0) {
		rel_stats->data.pkt_size_mean[TAF_TYPE_UL] =
			taf_ias_rel_stats_rx_pkt_size(rel_stats);
	}
}

static INLINE void BCMFASTPATH taf_ias_rx_mon_agg(taf_scb_cubby_t* scb_taf,
	taf_ias_sched_rel_stats_t* rel_stats, taf_rxpkt_stats_t * stats, uint32 now_time)
{
	uint32 max_aggn = taf_ias_rel_stats_rx_max_aggn(rel_stats);
	uint32 aggn = MAX(stats->rxstat_aggn, stats->aggn);

	if (aggn > AMPDU_BA_MAX_WSIZE) {
		WL_ERROR(("wl%u %s: "MACF" AGGN %u\n", WLCWLUNIT(TAF_WLCC(scb_taf)),
			__FUNCTION__, TAF_ETHERC(scb_taf), aggn));
		TAF_ASSERT(0);
		return;
	}
	if (aggn == 0) {
		WL_TAFM2(scb_taf->method, MACF" aggn 0 (qn = %u)\n",
			TAF_ETHERC(scb_taf), stats->qos_nulls);
		return;
	}

	rel_stats->data.rx.scaled_pcount += aggn << TAF_PKTCOUNT_SCALE_SHIFT;
	rel_stats->data.rx.scaled_acount += 1 << TAF_APKTCOUNT_SCALE_SHIFT;

	if ((aggn < max_aggn) &&
		(now_time - rel_stats->data.rx.max_aggn_timestamp > TAF_IAS_RXMON_HOLD_SMALL)) {

		rel_stats->data.rx.max_pcount +=
			(3 * max_aggn + aggn - 3) << (TAF_PKTCOUNT_SCALE_SHIFT - 2);
		return;
	}

	if (aggn >= max_aggn) {
		if (aggn > max_aggn) {
			WL_TAFM2(scb_taf->method, MACF" new max aggn %u -> %u\n",
				TAF_ETHERC(scb_taf), max_aggn, aggn);
		}
		rel_stats->data.rx.max_aggn_timestamp = now_time;
		max_aggn = aggn;
	}

	rel_stats->data.rx.max_pcount =
		((max_aggn << TAF_PKTCOUNT_SCALE_SHIFT) * rel_stats->data.rx.scaled_acount) >>
		TAF_APKTCOUNT_SCALE_SHIFT;
}

static INLINE void BCMFASTPATH taf_ias_rx_mon_plen(taf_scb_cubby_t* scb_taf,
	taf_ias_sched_rel_stats_t* rel_stats, taf_rxpkt_stats_t * stats, uint32 now_time)
{
	uint32 max_plen = rel_stats->data.pkt_size_max[TAF_TYPE_UL];
	uint32 min_plen = rel_stats->data.pkt_size_min[TAF_TYPE_UL];
	uint32 plen;

	plen = stats->max_agglen;

	if ((plen < max_plen) &&
		((now_time - rel_stats->data.rx.max_plen_timestamp) < TAF_IAS_RXMON_HOLD_LARGE)) {

	} else if (plen >= max_plen) {
		if (plen > max_plen) {
			WL_TAFM2(scb_taf->method, MACF" new max plen %u -> %u\n",
				TAF_ETHERC(scb_taf), max_plen, plen);
		}
		rel_stats->data.rx.max_plen_timestamp = now_time;
		rel_stats->data.pkt_size_max[TAF_TYPE_UL] = plen;

	} else {
		WL_TAFM2(scb_taf->method, MACF" reset max plen %u -> %u\n",
			TAF_ETHERC(scb_taf), max_plen, plen);
		rel_stats->data.pkt_size_max[TAF_TYPE_UL] = plen;
	}

	plen = stats->min_agglen;

	if ((plen > min_plen) &&
		((now_time - rel_stats->data.rx.min_plen_timestamp) < TAF_IAS_RXMON_HOLD_LARGE)) {

	} else if (plen <= min_plen) {
		if (plen < min_plen) {
			WL_TAFM2(scb_taf->method, MACF" new min plen %u -> %u\n",
				TAF_ETHERC(scb_taf), min_plen, plen);
		}
		rel_stats->data.rx.min_plen_timestamp = now_time;
		rel_stats->data.pkt_size_min[TAF_TYPE_UL] = plen;

	} else {
		WL_TAFM2(scb_taf->method, MACF" reset min plen %u -> %u\n",
			TAF_ETHERC(scb_taf), min_plen, plen);
		rel_stats->data.pkt_size_min[TAF_TYPE_UL] = plen;
	}

	WL_TAFM3(scb_taf->method, MACF" min = %u, max = %u, mean = %u\n", TAF_ETHERC(scb_taf),
		rel_stats->data.pkt_size_min[TAF_TYPE_UL],
		rel_stats->data.pkt_size_max[TAF_TYPE_UL],
		rel_stats->data.pkt_size_mean[TAF_TYPE_UL]);
}

static INLINE void BCMFASTPATH taf_ias_rx_mon_qn(taf_scb_cubby_t* scb_taf,
	taf_ias_sched_rel_stats_t* rel_stats, taf_rxpkt_stats_t * stats, uint32 now_time)
{

	if (rel_stats->data.rx.bytes > stats->total_qos_null_len) {
		rel_stats->data.rx.bytes -= stats->total_qos_null_len;
	} else {
		WL_TAFM1(scb_taf->method, MACF" qn byte adjust underflow! %u / %u\n",
			TAF_ETHERC(scb_taf), rel_stats->data.rx.bytes, stats->total_qos_null_len);
		rel_stats->data.rx.bytes = 0;
	}

	rel_stats->data.rx.scaled_qncount += stats->qos_nulls << TAF_PKTCOUNT_SCALE_SHIFT;
}
#endif /* TAF_ENABLE_UL */

static bool BCMFASTPATH taf_ias_rx_status(void * handle, taf_scb_cubby_t* scb_taf, int tid,
	int count, void * data, taf_rxpkt_state_t status)
{
	bool ret = TRUE;
#if TAF_ENABLE_UL
	taf_method_info_t* method = handle;
	taf_ias_sched_data_t* ias_sched;
	taf_ias_sched_rel_stats_t* rel_stats;
	int stats_tid;
	bool ul_admitted = taf_list_find(scb_taf, TAF_TYPE_UL) != NULL;
	bool byte_update = FALSE;
	bool aggn_update = FALSE;
	bool plen_update = FALSE;
	bool qnull_update = FALSE;

	if (status == TAF_RXPKT_STATUS_UPDATE_BYTE_PKT_RATE) {
		if (ul_admitted && (count == 0)) {
			/* received a non-ampdu packet from UL admitted station */
			WL_TAFM2(method, MACF" tid %u received non AMPDU pkt size %u from "
				"%s station\n", TAF_ETHERC(scb_taf), tid,
				((taf_rxpkt_stats_t*)data)->total_agglen,
				TAF_TECH_NAME(TAF_TECH_UL_OFDMA));
		}
		byte_update = (count >= 0);
		plen_update = TRUE;
		aggn_update = TRUE;

	} else if (status == TAF_RXPKT_STATUS_UPDATE_BYTE_PKT_RATE_TRIG_STATUS) {
		byte_update = (count < 0);
		qnull_update = ul_admitted;

	} else if (status == TAF_RXPKT_STATUS_UPDATE_QOS_NULL) {
		qnull_update = ul_admitted;
	}

	if (byte_update || aggn_update || plen_update || qnull_update) {
		TAF_ASSERT(data);

		if (data == NULL) {
			return FALSE;
		}
	}

	if (ul_admitted && TAF_OFDMA_OPT(method, ULTID_IGNRE)) {
		stats_tid = TAF_UL_PRIO;
	} else if (TAF_OPT(method, ULTID_IGNRE))  {
		stats_tid = TAF_UL_PRIO;
	} else {
		stats_tid = tid;
	}

	ias_sched = TAF_IAS_TID_STATE(scb_taf, stats_tid);

	if (ias_sched == NULL) {
		taf_ias_link_state(method, scb_taf->scb, stats_tid, TAF_SOURCE_UL,
			TAF_LINKSTATE_INIT);
		ias_sched = TAF_IAS_TID_STATE(scb_taf, stats_tid);
	}

	if (ias_sched == NULL) {
		WL_TAFM3(method, MACF" tid %u no ias_sched\n", TAF_ETHERC(scb_taf), stats_tid);
		return FALSE;
	}
	rel_stats = &ias_sched->rel_stats;

	if (byte_update) {
		uint32 now_time = taf_timestamp(TAF_WLCM(method));

		TAF_IAS_NOWTIME_SYNC(method, now_time);
		taf_ias_decay_stats(rel_stats, taf_ias_coeff_p(method), TAF_TYPE_UL, now_time);
	}

	switch (status) {
		case TAF_RXPKT_STATUS_UPDATE_BYTE_PKT_RATE_TRIG_STATUS:
		case TAF_RXPKT_STATUS_UPDATE_BYTE_PKT_RATE:
			if (count < 0) {
				return ret;
			}

			if (byte_update) {
				taf_rxpkt_stats_t * stats = data;

				taf_ias_rx_mon_bytes(scb_taf, rel_stats,
					stats, TAF_IAS_NOWTIME(method));

				WL_TAFM3(method, MACF" tid %u/%u, byte count %u, rate %u Kbits/s\n",
					TAF_ETHERC(scb_taf), tid, stats_tid,
					stats->total_agglen,
					taf_ias_rx_Kbits_flow(scb_taf, stats_tid));

			}

			if (aggn_update) {
				taf_rxpkt_stats_t * stats = data;

				taf_ias_rx_mon_agg(scb_taf, rel_stats,
					stats, TAF_IAS_NOWTIME(method));
				WL_TAFM3(method, MACF" tid %u/%u, aggn = %3u, rx_stat aggn = "
					"%3u\n", TAF_ETHERC(scb_taf), tid, stats_tid, stats->aggn,
					stats->rxstat_aggn);
			}

			if (plen_update) {
				taf_rxpkt_stats_t * stats = data;

				taf_ias_rx_mon_plen(scb_taf, rel_stats,
					stats, TAF_IAS_NOWTIME(method));
				WL_TAFM3(method, MACF" tid %u/%u, min = %5u, max = %5u\n",
					TAF_ETHERC(scb_taf), tid, stats_tid, stats->min_agglen,
					stats->max_agglen);
			}

			/* fall through */
		case  TAF_RXPKT_STATUS_UPDATE_QOS_NULL:
			if (qnull_update) {
				taf_rxpkt_stats_t * stats = data;

				taf_ias_rx_mon_qn(scb_taf, rel_stats,
					stats, TAF_IAS_NOWTIME(method));

				if (stats->qos_nulls > 0) {
					WL_TAFM3(method, MACF" tid %u/%u, qosnull bytes = %u, "
						"qosnull = %u, qosnull_only = %u\n",
						TAF_ETHERC(scb_taf), tid, stats_tid,
						stats->total_qos_null_len, stats->qos_nulls,
						stats->qos_null_only);
				}
			}
			break;

		case TAF_RXPKT_STATUS_GET_BYTE_RATE:
			/* data contains a pointer to uint32 of Kbits/sec */
			if (data == NULL) {
				return FALSE;
			}
			*((uint32*)data) = taf_ias_rx_Kbits_flow(scb_taf, stats_tid);
			break;

		case TAF_RXPKT_STATUS_GET_PHY_RATE:
			/* data contains a pointer to uint32 of Kbits/sec */
			if (data == NULL) {
				return FALSE;
			}
			*((uint32*)data) = taf_ias_rx_Kbits_phy(scb_taf);

			if (*((uint32*)data) == 0) {
				return taf_ias_rx_Kbits_flow(scb_taf, stats_tid);
			}
			break;

		case TAF_RXPKT_STATUS_GET_PKT_INFO:
			/* data contains a pointer to taf_rxpkt_info_t */
			if (data != NULL) {
				taf_rxpkt_info_t * pkt_info = (taf_rxpkt_info_t *) data;

				pkt_info->size.average = rel_stats->data.pkt_size_mean[TAF_TYPE_UL];
				if (pkt_info->size.average == 0) {
					pkt_info->size.average =
						rel_stats->data.pkt_size_mean[TAF_TYPE_UL] =
						taf_ias_rx_pkt_size(scb_taf, stats_tid);
				}
				if (pkt_info->size.average == 0) {
					pkt_info->size.average = TAF_PKT_SIZE_INIT_UL(rel_stats);
				}

				pkt_info->size.peak = rel_stats->data.pkt_size_max[TAF_TYPE_UL];
				if (pkt_info->size.peak == 0) {
					pkt_info->size.peak = TAF_PKT_SIZE_INIT_UL(rel_stats);
				}

				pkt_info->aggn.average = TAF_GET_AGG(ias_sched->rx_aggn);

				if (pkt_info->aggn.average == 0) {
					TAF_ASSERT(0);
					pkt_info->aggn.average = 16;
				}

				pkt_info->aggn.peak = TAF_GET_AGG(ias_sched->rx_max_aggn);

				if (pkt_info->aggn.peak == 0) {
					TAF_ASSERT(0);
					pkt_info->aggn.peak = 16;
				}
			} else {
				ret = FALSE;
			}
			break;

		case TAF_RXPKT_STATUS_GET_RX_PKT_RATE:
			/* data contains a pointer to uint32 of avg packet rate */
			if (data == NULL) {
				return FALSE;
			}
			*((uint32*)data) = taf_ias_rx_pcount_flow(scb_taf, stats_tid);
			break;

		case TAF_RXPKT_STATUS_GET_SCHED_RATE:
			/* data contains a pointer to uint32 of avg packet rate */
			if (data == NULL) {
				return FALSE;
			}
			*((uint32*)data) = taf_ias_sched_rx_Kbits_flow(scb_taf, stats_tid);
			break;

		case TAF_RXPKT_STATUS_GET_RX_AGG_PKT_RATE:
			/* data contains a pointer to uint32 of avg ampdu rate */
			if (data == NULL) {
				return FALSE;
			}
			*((uint32*)data) = taf_ias_rx_acount_flow(scb_taf, stats_tid);
			break;

		case TAF_RXPKT_STATUS_GET_RX_AGGREGATION:
			/* data contains a pointer to uint32 of aggregation */
			if (data == NULL) {
				return FALSE;
			}
			*((uint32*)data) = TAF_GET_AGG(ias_sched->rx_aggn);
			break;

		case TAF_RXPKT_STATUS_GET_MAX_RX_AGGREGATION:
			/* data contains a pointer to uint32 of aggregation */
			if (data == NULL) {
				return FALSE;
			}
			*((uint32*)data) = TAF_GET_AGG(ias_sched->rx_max_aggn);
			break;

		case TAF_RXPKT_STATUS_GET_SCHED_DPKT_OVER:
			/* data contains a pointer to int32 of deci-pkts */
			if (data == NULL) {
				return FALSE;
			}
			*((int32*)data) = taf_ias_sched_rx_deci_overpkts(scb_taf, stats_tid);
			break;

		case TAF_RXPKT_STATUS_GET_QNULL_DECI_RATE:
			/* data contains a pointer to uint32 of qosnull * 16, per second */
			if (data == NULL) {
				return FALSE;
			}
			*((uint32*)data) = taf_ias_rx_qn10_rate(scb_taf, stats_tid);
			break;

		default:
			ret = FALSE;
			break;
	}
#endif /* TAF_ENABLE_UL */
	return ret;
}

/* function has to be 'safe' in case scb_taf is NULL */
bool BCMFASTPATH taf_ias_tx_status(void * handle, taf_scb_cubby_t* scb_taf, int tid, void * p,
	taf_txpkt_state_t status)
{
	taf_method_info_t* method = handle;
	bool ret = FALSE;
	taf_list_type_t type = TAF_TYPE_DL;

	if (method->taf_info->scheduler_nest > 0) {
		WL_TAFM2(method, MACF" tid %u scheduler active %s\n",
			TAF_SAFE_ETHERC(scb_taf), tid, taf_txpkt_status_text[status]);
	}

	switch (status) {

		case TAF_TXPKT_STATUS_UPDATE_RETRY_COUNT:
		case TAF_TXPKT_STATUS_UPDATE_PACKET_COUNT:
		case TAF_TXPKT_STATUS_NONE:
		case TAF_TXPKT_STATUS_UPDATE_RATE:
			ret = TRUE;
			break;

		case TAF_TXPKT_STATUS_PKTFREE_RESET:
		case TAF_TXPKT_STATUS_REQUEUED:
			if (TAF_IS_TAGGED(WLPKTTAG(p))) {

				WL_ERROR(("wl%u %s: "MACF" tid %u %s\n",
					WLCWLUNIT(TAF_WLCM(method)), __FUNCTION__,
					TAF_SAFE_ETHERC(scb_taf), tid,
					TAF_TXPKT_STAT_TEXT(status)));
			}
			/* fall through */
		case TAF_TXPKT_STATUS_PKTFREE_DROP:
		case TAF_TXPKT_STATUS_SUPPRESSED_FREE:
			if (TAF_IS_TAGGED(WLPKTTAG(p)) &&
					TAF_PKT_TAG_UNITS(p, TAF_TYPE_DL) <= TAF_PKTTAG_RESERVED) {

				ret = taf_ias_pkt_lost(method, scb_taf, tid, p,
					status, TAF_TYPE_DL);

				if (status == TAF_TXPKT_STATUS_REQUEUED) {
					WLPKTTAG(p)->pktinfo.taf.ias.units = 0;
					TAF_UNSET_TAG(WLPKTTAG(p));
				} else {
					WLPKTTAG(p)->pktinfo.taf.ias.units = TAF_PKTTAG_PROCESSED;
				}
			}
			break;

#if TAF_ENABLE_UL
		case TAF_TXPKT_STATUS_UL_SUPPRESSED:
			if (TAF_IS_TAGGED(WLULPKTTAG(p))) {

				ret = taf_ias_pkt_lost(method, scb_taf, tid,
					p, status, TAF_TYPE_UL);
				WLULPKTTAG(p)->units = TAF_PKTTAG_PROCESSED;
			}
			break;
#else
		case TAF_TXPKT_STATUS_UL_SUPPRESSED:
			TAF_ASSERT(0);
			break;
#endif /* TAF_ENABLE_UL */

		case TAF_TXPKT_STATUS_SUPPRESSED:
		case TAF_TXPKT_STATUS_PS_QUEUED:
#if TAF_ENABLE_PSQ_PULL
			if (TAF_IS_TAGGED(WLPKTTAG(p))) {
				if (TAF_PKT_TAG_UNITS(p, TAF_TYPE_DL) <= TAF_PKTTAG_RESERVED) {
					ret = taf_ias_pkt_lost(method, scb_taf, tid, p,
						status, TAF_TYPE_DL);
				}
				WLPKTTAG(p)->pktinfo.taf.ias.units = 0;
				TAF_UNSET_TAG(WLPKTTAG(p));
			}
			break;
#endif /* TAF_ENABLE_SQS_PULL */
		case TAF_TXPKT_STATUS_REGMPDU:
		case TAF_TXPKT_STATUS_PKTFREE:
			if (TAF_IS_TAGGED(WLPKTTAG(p)) &&
					TAF_PKT_TAG_UNITS(p, TAF_TYPE_DL) <= TAF_PKTTAG_RESERVED) {

				ret = taf_ias_handle_star(method, tid,
					TAF_PKT_TAG_UNITS(p, TAF_TYPE_DL),
					TAF_PKT_TAG_INDEX(p, TAF_TYPE_DL), TAF_TYPE_DL);
				WLPKTTAG(p)->pktinfo.taf.ias.units = TAF_PKTTAG_PROCESSED;
			}
			break;
#if TAF_ENABLE_UL
		case TAF_TXPKT_STATUS_TRIGGER_COMPLETE:
			type = TAF_TYPE_UL;
			if (TAF_IS_TAGGED(WLULPKTTAG(p))) {
				ret = taf_ias_handle_star(method, tid,
					TAF_PKT_TAG_UNITS(p, TAF_TYPE_UL),
					TAF_PKT_TAG_INDEX(p, TAF_TYPE_UL), TAF_TYPE_UL);
				WLULPKTTAG(p)->units = TAF_PKTTAG_PROCESSED;
			}
			break;
#else
		case TAF_TXPKT_STATUS_TRIGGER_COMPLETE:
			TAF_ASSERT(0);
			break;
#endif /* TAF_ENABLE_UL */
		case TAF_NUM_TXPKT_STATUS_TYPES:
			TAF_ASSERT(0);
			break;
	}
	if (!ret &&
		(((type == TAF_TYPE_DL) && TAF_IS_TAGGED(WLPKTTAG(p)) &&
			(TAF_PKT_TAG_UNITS(p, TAF_TYPE_DL) == TAF_PKTTAG_PROCESSED)) ||
#if TAF_ENABLE_UL
			((type == TAF_TYPE_UL) && TAF_IS_TAGGED(WLULPKTTAG(p)) &&
			(TAF_PKT_TAG_UNITS(p, TAF_TYPE_UL) == TAF_PKTTAG_PROCESSED)))) {
#else
			FALSE)) {
#endif /* TAF_ENABLE_UL */
		WL_TAFT3(method->taf_info, MACF"%s tid %u, pkt already processed\n",
			TAF_SAFE_ETHERC(scb_taf), TAF_TYPE(type), tid);
		ret = TRUE;
	}
	return ret;
}

#if TAF_ENABLE_SQS_PULL
static void taf_iasm_sched_state(void * handle, taf_scb_cubby_t * scb_taf, int tid, int count,
	taf_source_type_t s_idx, taf_sched_state_t state)
{
	taf_method_info_t* method = handle;

	switch (state) {
		case TAF_SCHED_STATE_DATA_BLOCK_FIFO:
			ias_cfg(method).data_block = count ? TRUE : FALSE;
			WL_TAFM1(method, "data block %u\n", ias_cfg(method).data_block);
			break;

		case TAF_SCHED_STATE_RESET:
			WL_TAFM1(method, "RESET\n");
			*method->ready_to_schedule = 0;
			break;

		default:
			break;
	}
}
#endif /* TAF_ENABLE_SQS_PULL */

static void taf_ias_sched_state(void * handle, taf_scb_cubby_t * scb_taf, int tid, int count,
	taf_source_type_t s_idx, taf_sched_state_t state)
{
	taf_method_info_t* method = handle;

	switch (state) {
		case TAF_SCHED_STATE_DATA_BLOCK_FIFO:
			ias_cfg(method).data_block = count ? TRUE : FALSE;
			WL_TAFM1(method, "data block %u\n", ias_cfg(method).data_block);
			break;

		case TAF_SCHED_STATE_REWIND:
			if (scb_taf) {
				WL_TAFM2(method, MACF" tid %u rewind %u pkts\n",
					TAF_ETHERC(scb_taf), tid, count);
			}
			break;

		case TAF_SCHED_STATE_RESET:
			WL_TAFM1(method, "RESET\n");
#if TAF_ENABLE_MU_TX
			memset(&ias_cfg(method).active_mu_count, 0,
			       sizeof(ias_cfg(method).active_mu_count));
#endif
			taf_ias_clean_all_rspecs(method);
			*method->ready_to_schedule = ~0;
			taf_ias_unified_reset(method);
			TAF_IAS_NOWTIME_SYNC(method, taf_timestamp(TAF_WLCM(method)));

			/* the following will re-sync lists if they were split */
			(void)taf_ias_get_list_head_ptr(method);
			break;

		case TAF_SCHED_STATE_TX_MPDU_DENSITY:
			ias_cfg(method).min_mpdu_dur[TAF_TYPE_DL] = TAF_MPDU_DENS_TO_UNITS(count);
			break;
		case TAF_SCHED_STATE_RX_MPDU_DENSITY:
#if TAF_ENABLE_UL
			ias_cfg(method).min_mpdu_dur[TAF_TYPE_UL] = TAF_MPDU_DENS_TO_UNITS(count);
#endif
			break;

		default:
			break;
	}
}

static int taf_ias_calc_trfstats(taf_method_info_t* method, taf_scb_cubby_t * scb_taf, int tid,
	uint32* now_time, taf_traffic_stats_t * stat)
{
	taf_ias_sched_rel_stats_t* rel_stats;
	uint32 time_ms;
	uint32 normalise = taf_ias_norm(method);
	taf_ias_sched_data_t* ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);

	if (!ias_sched) {
		return BCME_NOTFOUND;
	}
	if (stat == NULL) {
		return BCME_ERROR;
	}

	memset(stat, 0, sizeof(*stat));
	rel_stats = &ias_sched->rel_stats;

	(void)taf_nowtime(TAF_WLCM(method), now_time);

	taf_ias_decay_stats(rel_stats, taf_ias_coeff_p(method), TAF_TYPE_DL, *now_time);

	stat->max_aggsf = taf_aggsf(scb_taf, tid);
	stat->achvd_aggsf = taf_ias_achvd_aggsf(scb_taf, tid);

	stat->tx_release.npacket_len = rel_stats->data.pkt_size_mean[TAF_TYPE_DL];

	time_ms = (normalise + 500) / 1000;

	if (time_ms == 0) {
		WL_ERROR(("wl%u %s: "MACF" normalise %u, time_ms %u\n",
			WLCWLUNIT(TAF_WLCM(method)), __FUNCTION__,
			TAF_ETHERC(scb_taf), normalise, time_ms));
		TAF_ASSERT(normalise > 0 && time_ms > 0);
		return BCME_ERROR;
	}

	stat->tx_per_second.bytes = (rel_stats->data.total_bytes * 1000) / time_ms;

	stat->tx_per_second.Kbits = (rel_stats->data.total_bytes * 8) / time_ms;

	stat->tx_per_second.Mbits =
		(rel_stats->data.total_bytes * 8) / normalise;

	stat->tx_per_second.units = (rel_stats->data.total_release_units  * 1000) / time_ms;

	stat->tx_per_second.airtime =
		(rel_stats->data.total_release_units * 1000) / TAF_MICROSEC_TO_UNITS(normalise);

	stat->tx_per_second.npkts =
		(rel_stats->data.total_scaled_ncount * (10000 >> TAF_PKTCOUNT_SCALE_SHIFT)) /
		(normalise / 100);

	stat->tx_per_second.ppkts =
		(rel_stats->data.total_scaled_pcount * (10000 >> TAF_PKTCOUNT_SCALE_SHIFT)) /
		(normalise / 100);

#if TAF_ENABLE_UL
	taf_ias_decay_stats(rel_stats, taf_ias_coeff_p(method), TAF_TYPE_UL, *now_time);

	WL_TAFM4(scb_taf->method, MACF"tid %u %u/%u\n", TAF_ETHERC(scb_taf), tid,
		rel_stats->data.rx.bytes, rel_stats->data.rx.time);

	stat->rx_mon.packet_len = rel_stats->data.pkt_size_mean[TAF_TYPE_UL] =
		taf_ias_rx_pkt_size(scb_taf, tid);
	stat->rx_mon.max_packet_len =  rel_stats->data.pkt_size_max[TAF_TYPE_UL];
	stat->rx_mon.min_packet_len =  rel_stats->data.pkt_size_min[TAF_TYPE_UL];
	stat->rx_mon.aggn = taf_ias_rx_aggn(scb_taf, tid);
	stat->rx_mon.aggn_max = taf_ias_rx_max_aggn(scb_taf, tid);

	if (rel_stats->data.rx.time >= 500) {
		time_ms = (rel_stats->data.rx.time + 500) / 1000;

		stat->rx_mon.bytes = (rel_stats->data.rx.bytes * 1000) / time_ms;
		stat->rx_mon.Kbits = (rel_stats->data.rx.bytes * 8) / time_ms;
		stat->rx_mon.Mbits =
			(rel_stats->data.rx.bytes * 8) / rel_stats->data.rx.time;

		stat->rx_mon.ppkts =
			(rel_stats->data.rx.scaled_pcount * (10000 >> TAF_PKTCOUNT_SCALE_SHIFT)) /
			(rel_stats->data.rx.time / 100);

		stat->rx_mon.apkts =
			(rel_stats->data.rx.scaled_acount * (10000 >> TAF_APKTCOUNT_SCALE_SHIFT)) /
			(rel_stats->data.rx.time / 100);
	} else {
		WL_ERROR(("wl%u %s: "MACF" rx.time %u\n", WLCWLUNIT(TAF_WLCM(method)),
			__FUNCTION__, TAF_ETHERC(scb_taf), rel_stats->data.rx.time));
	}

	time_ms = (normalise + 500) / 1000;

	if (rel_stats->data.ul.sched_count > 0) {
		stat->rx_per_second.interval =  normalise << TAF_APKTCOUNT_SCALE_SHIFT;
		stat->rx_per_second.interval /= rel_stats->data.ul.sched_count;
	} else {
		stat->rx_per_second.interval = 0;
	}
	stat->rx_per_second.count =
		((rel_stats->data.ul.sched_count * 1000) >> TAF_APKTCOUNT_SCALE_SHIFT) / time_ms;

	stat->rx_per_second.qosnull_dcount = taf_ias_rx_qn10_rate(scb_taf, tid);

#ifdef TAF_DBG
	if (rel_stats->data.ul.update_count > 0) {
		stat->rx_per_second.update_interval =  normalise << TAF_APKTCOUNT_SCALE_SHIFT;
		stat->rx_per_second.update_interval /= rel_stats->data.ul.update_count;
	} else {
		stat->rx_per_second.update_interval = 0;
	}
	stat->rx_per_second.update_count =
		((rel_stats->data.ul.update_count * 1000) >> TAF_APKTCOUNT_SCALE_SHIFT) / time_ms;
#endif /* TAF_DBG */
	stat->rx_per_second.airtime =
		(rel_stats->data.ul.total_units * 1000) / TAF_MICROSEC_TO_UNITS(normalise);

	stat->rx_per_second.Kbits = (rel_stats->data.ul.total_bytes * 8) / time_ms;

	stat->rx_per_second.Mbits = (rel_stats->data.ul.total_bytes * 8) / normalise;

	stat->rx_per_second.ppkts = taf_div64(((uint64)rel_stats->data.ul.total_bytes) * 1000,
		stat->rx_mon.packet_len * time_ms);

	if (rel_stats->data.ul.sched_count > 0) {
		stat->rx_release.units =
			(rel_stats->data.ul.total_units << TAF_APKTCOUNT_SCALE_SHIFT) /
			rel_stats->data.ul.sched_count;

		stat->rx_release.bytes = taf_div64(
			((uint64)rel_stats->data.ul.total_bytes) << TAF_APKTCOUNT_SCALE_SHIFT,
			rel_stats->data.ul.sched_count);
	} else {
		stat->rx_release.units = 0;
		stat->rx_release.bytes = 0;
	}
#endif /* TAF_ENABLE_UL */
	return BCME_OK;
}

static int taf_ias_traffic_stats(void * handle, taf_scb_cubby_t * scb_taf, int tid,
	taf_traffic_stats_t * stat)
{
	taf_method_info_t* method = (taf_method_info_t*)handle;
	uint32 now_time = taf_timestamp(TAF_WLCM(method));

	TAF_IAS_NOWTIME_SYNC(method, now_time);

	return taf_ias_calc_trfstats(method, scb_taf, tid, &now_time, stat);
}

int
taf_ias_iovar(void *handle, taf_scb_cubby_t * scb_taf, const char* cmd, wl_taf_define_t* result,
	struct bcmstrbuf* b)
{
	taf_method_info_t* method = handle;
	int status = BCME_UNSUPPORTED;
	const char* local_cmd = cmd;

	if (!TAF_TYPE_IS_IAS(method->type)) {
		return BCME_UNSUPPORTED;
	}
	if (!cmd) {
		return BCME_UNSUPPORTED;
	}
	if (scb_taf) {
		WL_TAFM1(method, MACF" %s\n", TAF_ETHERC(scb_taf), cmd);
	} else {
		WL_TAFM1(method, "%s\n", cmd);
	}

	if (!strcmp(cmd, "coeff") && (method->type != TAF_EBOS)) {
		int err;
		uint32 coeff = ias_cfg(method).coeff.coeff1;
		err = wlc_taf_param(&local_cmd, &coeff, 0, TAF_COEFF_IAS_MAX_CFG, b);
		if (err == TAF_IOVAR_OK_SET) {
			taf_ias_set_coeff(coeff, &ias_cfg(method).coeff);
		}
		result->misc = ias_cfg(method).coeff.coeff1;
		return err;
	}
	if (!strcmp(cmd, "coeff_factor") && (method->type != TAF_EBOS)) {
		int err;
		uint32 factor = ias_cfg(method).coeff.time_shift;
		err = wlc_taf_param(&local_cmd, &factor, 6, 18, b);
		if (err == TAF_IOVAR_OK_SET) {
			ias_cfg(method).coeff.time_shift = factor;
			taf_ias_set_coeff(ias_cfg(method).coeff.coeff1, &ias_cfg(method).coeff);
		}
		result->misc = ias_cfg(method).coeff.time_shift;
		return err;
	}
#if TAF_ENABLE_TIMER
	if (!strcmp(cmd, "agg_hold")) {
		taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
		uint32 agg_hold = ias_info->agg_hold;
		int err = wlc_taf_param(&local_cmd, &agg_hold, 0, 15, b);
		if (err == TAF_IOVAR_OK_SET) {
			ias_info->agg_hold = agg_hold;
		}
		result->misc = agg_hold;
		return err;
	}
	if (!strcmp(cmd, "agg_hold_threshold")) {
		taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
		int err = wlc_taf_param(&local_cmd, &ias_info->agg_hold_threshold, 0,
			ias_cfg(method).high >= 800 ?
			TAF_MICROSEC_TO_UNITS(ias_cfg(method).high - 800) : 0, b);
		result->misc = ias_info->agg_hold_threshold;
		return err;
	}
	if (!strcmp(cmd, "retrigger")) {
		taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
		uint32 retrigger = ias_info->retrigger_timer[TAF_TYPE_DL];

		int err = wlc_taf_param(&local_cmd, &retrigger, 0, BCM_UINT16_MAX, b);

		if (err == TAF_IOVAR_OK_SET) {
			ias_info->retrigger_timer[TAF_TYPE_DL] = retrigger;
		}
		result->misc = ias_info->retrigger_timer[TAF_TYPE_DL];
		return err;
	}
#if TAF_ENABLE_UL
	if (!strcmp(cmd, "retrigger_ul")) {
		taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
		uint32 retrigger = ias_info->retrigger_timer[TAF_TYPE_UL];

		int err = wlc_taf_param(&local_cmd, &retrigger, 0, BCM_UINT16_MAX, b);

		if (err == TAF_IOVAR_OK_SET) {
			ias_info->retrigger_timer[TAF_TYPE_UL] = retrigger;
		}
		result->misc = ias_info->retrigger_timer[TAF_TYPE_UL];
		return err;
	}
#endif /* TAF_ENABLE_UL */
#endif /* TAF_ENABLE_TIMER */
	if (!strcmp(cmd, "low")) {
		int err = wlc_taf_param(&local_cmd, &ias_cfg(method).low,
			0, ias_cfg(method).high, b);
		if (err == TAF_IOVAR_OK_SET) {
			taf_ias_time_settings_sync(method);
		}
		if (ias_cfg(method).low < 1000 && TAF_OPT(method, EXIT_EARLY)) {
			bcm_bprintf(b, "warning: exit early option set\n");
		}
		result->misc = ias_cfg(method).low;
		return err;
	}
	if (!strcmp(cmd, "high")) {
		int err = wlc_taf_param(&local_cmd, &ias_cfg(method).high, ias_cfg(method).low,
			TAF_WINDOW_MAX, b);
		if (err == TAF_IOVAR_OK_SET) {
			taf_ias_time_settings_sync(method);
		}
		result->misc = ias_cfg(method).high;
		return err;
	}
	if (!strcmp(cmd, "immed")) {
		taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
		uint32 immed = ias_info->immed;
		int err = wlc_taf_param(&local_cmd, &immed, 0, 0xFFFFF, b);
		if (err == TAF_IOVAR_OK_SET) {
			ias_info->immed = immed;
		}
		result->misc = immed;
		return err;
	}
#if TAF_ENABLE_MU_TX
	if (!strcmp(cmd, "mu_pair")) {
		int err = BCME_BADARG;

		if (ias_cfg(method).mu_pair == NULL) {
			return err;
		}
		cmd += strlen(cmd) + 1;

		if (*cmd) {
			int idx = bcm_strtoul(cmd, NULL, 0);

			if (idx >= 2 && idx <= TAF_MAX_MU) {
				uint32 score = ias_cfg(method).mu_pair[idx - 1];

				err = wlc_taf_param(&cmd, &score, 0,
					(1 << TAF_COEFF_IAS_MAX_BITS) -1, b);
				if (err == TAF_IOVAR_OK_SET) {
					ias_cfg(method).mu_pair[idx - 1] = score;
				}
				result->misc = ias_cfg(method).mu_pair[idx - 1];
			}
		} else {
			int idx;
			bcm_bprintf(b, "idx: pair(0-4095)\n");

			for (idx = 1; idx < TAF_MAX_MU; idx++) {
				bcm_bprintf(b, " %2u:   %4u\n", idx + 1,
					ias_cfg(method).mu_pair[idx]);
			}
			err = BCME_OK;
		}
		return err;
	}
	if (!strcmp(cmd, "ofdma_limit")) {
		int err = BCME_UNSUPPORTED;
		uint16* rel_limit_p = ias_cfg(method).mu_user_rel_limit[TAF_TECH_DL_OFDMA];

		if (rel_limit_p == NULL) {
			return err;
		}
		cmd += strlen(cmd) + 1;

		if (*cmd) {
			int idx = bcm_strtoul(cmd, NULL, 0);

			if (idx >= 1 && idx <= TAF_MAX_MU_MIMO) {
				uint32 score = rel_limit_p[idx - 1];

				err = wlc_taf_param(&cmd, &score, 0, TAF_MICROSEC_MAX, b);
				if (err == TAF_IOVAR_OK_SET) {
					rel_limit_p[idx - 1] = score;
				}
				result->misc = rel_limit_p[idx - 1];
			}
		} else {
			int idx;
			bcm_bprintf(b, "idx: dl-ofdma rel limit us (0-%u)\n", TAF_MICROSEC_MAX);

			for (idx = 0; idx < TAF_MAX_MU_MIMO; idx++) {
				bcm_bprintf(b, " %2u:   %4u\n", idx + 1, rel_limit_p[idx]);
			}
			err = BCME_OK;
		}
		return err;
	}
#if TAF_ENABLE_UL
	if (!strcmp(cmd, "ul_ofdma_limit")) {
		int err = BCME_UNSUPPORTED;
		uint16* rel_limit_p = ias_cfg(method).mu_user_rel_limit[TAF_TECH_UL_OFDMA];

		if (rel_limit_p == NULL) {
			return err;
		}
		cmd += strlen(cmd) + 1;

		if (*cmd) {
			int idx = bcm_strtoul(cmd, NULL, 0);

			if (idx >= 1 && idx <= TAF_MAX_MU_MIMO) {
				uint32 score = rel_limit_p[idx - 1];

				err = wlc_taf_param(&cmd, &score, 0, TAF_MICROSEC_MAX, b);
				if (err == TAF_IOVAR_OK_SET) {
					rel_limit_p[idx - 1] = score;
				}
				result->misc = rel_limit_p[idx - 1];
			}
		} else {
			int idx;
			bcm_bprintf(b, "idx: ul-ofdma rel limit us (0-%u)\n", TAF_MICROSEC_MAX);

			for (idx = 0; idx < TAF_MAX_MU_MIMO; idx++) {
				bcm_bprintf(b, " %2u:   %4u\n", idx + 1, rel_limit_p[idx]);
			}
			err = BCME_OK;
		}
		return err;
	}
#endif /* TAF_ENABLE_MU_TX */
	if (!strcmp(cmd, "mu_mimo_limit")) {
		int err = BCME_UNSUPPORTED;
		uint16* rel_limit_p = ias_cfg(method).mu_user_rel_limit[TAF_TECH_DL_VHMUMIMO];

		if (rel_limit_p == NULL ||
			rel_limit_p != ias_cfg(method).mu_user_rel_limit[TAF_TECH_DL_HEMUMIMO]) {
			return err;
		}
		cmd += strlen(cmd) + 1;

		if (*cmd) {
			int idx = bcm_strtoul(cmd, NULL, 0);

			if (idx >= 2 && idx <= TAF_MAX_MU_MIMO) {
				uint32 score = rel_limit_p[idx - 1];

				err = wlc_taf_param(&cmd, &score, 0, TAF_MICROSEC_MAX, b);
				if (err == TAF_IOVAR_OK_SET) {
					rel_limit_p[idx - 1] = score;
				}
				result->misc = rel_limit_p[idx - 1];
			}
		} else {
			int idx;
			bcm_bprintf(b, "idx: mimo rel limit us (0-%u)\n", TAF_MICROSEC_MAX);

			for (idx = 1; idx < TAF_MAX_MU_MIMO; idx++) {
				bcm_bprintf(b, " %2u:   %4u\n", idx + 1, rel_limit_p[idx]);
			}
			err = BCME_OK;
		}
		return err;
	}
	if (!strcmp(cmd, "mu_mimo_opt")) {
		int err = wlc_taf_param(&local_cmd, &ias_cfg(method).mu_mimo_opt,
			0, (uint32)(-1), b);

		result->misc = ias_cfg(method).mu_mimo_opt;
		return err;
	}
	if (!strcmp(cmd, "mu_ofdma_opt")) {
		int err = wlc_taf_param(&local_cmd, &ias_cfg(method).mu_ofdma_opt,
			0, (uint32)(-1), b);

		result->misc = ias_cfg(method).mu_ofdma_opt;
		return err;
	}
#endif /* TAF_ENABLE_MU_TX */
	if (!strcmp(cmd, "opt")) {
		int err = wlc_taf_param(&local_cmd, &ias_cfg(method).options, 0, (uint32)(-1), b);

		result->misc = ias_cfg(method).options;
		return err;
	}
	if (!strcmp(cmd, "ias_opt")) {
		taf_ias_group_info_t* ias_info = TAF_IAS_GROUP_INFO(method->taf_info);
		int err = wlc_taf_param(&local_cmd, &ias_info->options, 0, (uint32)(-1), b);

		result->misc = ias_info->options;
		return err;
	}
	if (!strcmp(cmd, "aggp")) {
		uint32 opt_aggp_limit = ias_cfg(method).opt_aggp_limit;
		int err = wlc_taf_param(&local_cmd, &opt_aggp_limit, 0, 255, b);

		if (err == TAF_IOVAR_OK_SET) {
			ias_cfg(method).opt_aggp_limit = opt_aggp_limit;
		}
		result->misc = ias_cfg(method).opt_aggp_limit;
		return err;
	}
	if (!strcmp(cmd, "barrier")) {
		uint32 barrier = ias_cfg(method).barrier;
		int err = wlc_taf_param(&local_cmd, &barrier, 0, (TAF_MAX_NUM_WINDOWS - 2), b);

		if (err == TAF_IOVAR_OK_SET) {
			ias_cfg(method).barrier = barrier;
		}
		result->misc = ias_cfg(method).barrier;
		return err;
	}
	if (!strcmp(cmd, "tid_weight")) {
		int err = BCME_BADARG;

		if (ias_cfg(method).score_weights == NULL) {
			return err;
		}
		cmd += strlen(cmd) + 1;

		if (*cmd) {
			int tid = bcm_strtoul(cmd, NULL, 0);

			if (tid >= 0 && tid < NUMPRIO) {
				uint32 weight = ias_cfg(method).score_weights[tid];

				err = wlc_taf_param(&cmd, &weight, 0, BCM_UINT8_MAX, b);
				if (err == TAF_IOVAR_OK_SET) {
					ias_cfg(method).score_weights[tid] = weight;
				}
				result->misc = ias_cfg(method).score_weights[tid];
			}
		} else {
			int tid;
			const char* ac[AC_COUNT] = {"BE", "BK", "VI", "VO"};
			bcm_bprintf(b, "tid: (AC) weight(0-255)\n");

			for (tid = 0; tid < NUMPRIO; tid++) {
				bcm_bprintf(b, "  %d: (%s)    %3u\n", tid, ac[WME_PRIO2AC(tid)],
					ias_cfg(method).score_weights[tid]);
			}
			err = BCME_OK;
		}
		return err;
	}
	if (!strcmp(cmd, "rel_limit")) {
		int err = wlc_taf_param(&local_cmd, &ias_cfg(method).release_limit, 0,
			ias_cfg(method).high, b);
		result->misc = ias_cfg(method).release_limit;
		return err;
	}
#if TAF_ENABLE_SQS_PULL
	if (!strcmp(cmd, "margin")) {
		int err = wlc_taf_param(&local_cmd, &ias_cfg(method).margin, 0, 255, b);
		result->misc = ias_cfg(method).margin;
		return err;
	}
#endif /* TAF_ENABLE_SQS_PULL */

#ifdef TAF_DEBUG_VERBOSE
	if (!strcmp(cmd, "bodge")) {
		int err = wlc_taf_param(&local_cmd, &ias_cfg(method).bodge, 0, UINT32_MAX, b);
		result->misc = ias_cfg(method).bodge;
		return err;
	}
#endif
	if (!strcmp(cmd, "reset")) {
		taf_ias_unified_reset(method);
		return BCME_OK;
	}
	return status;
}

#ifdef TAF_PKTQ_LOG
static bool taf_ias_dpstats_counters(taf_scb_cubby_t* scb_taf,
	mac_log_counters_v06_t* mac_log, int tid, bool clear)
{
	taf_log_counters_v06_t*  taflog;
	taf_ias_sched_data_t* ias_sched = scb_taf ? TAF_IAS_TID_STATE(scb_taf, tid) : NULL;
	taf_ias_dpstats_counters_t* counters = ias_sched ?
		ias_sched->rel_stats.dpstats_log : NULL;

	if (mac_log == NULL || (counters == NULL && ias_sched == NULL)) {
		return FALSE;
	}
	if (counters == NULL) {
		counters = (taf_ias_dpstats_counters_t*)MALLOCZ(TAF_WLCM(scb_taf->method)->pub->osh,
			sizeof(*counters));

		ias_sched->rel_stats.dpstats_log = counters;
		return FALSE;
	}

	taflog = &mac_log->taf[tid];

	taflog->ias.emptied               = counters->emptied;
	taflog->ias.pwr_save              = counters->pwr_save;
	taflog->ias.release_pcount        = counters->release_pcount;
	taflog->ias.release_ncount        = counters->release_ncount;
	taflog->ias.ready                 = counters->ready;
	taflog->ias.pkt_size_mean         = counters->pkt_size_mean;
	taflog->ias.release_frcount       = counters->release_frcount;
	taflog->ias.restricted            = counters->restricted;
	taflog->ias.limited               = counters->limited;
	taflog->ias.held_z                = counters->held_z;
	taflog->ias.release_time          = counters->release_time;
	taflog->ias.release_bytes         = counters->release_bytes;

	if (clear) {
		memset(counters, 0, sizeof(*counters));
	}
	return TRUE;
}

static uint32 taf_ias_dpstats_dump(void* handle, taf_scb_cubby_t* scb_taf,
	mac_log_counters_v06_t* mac_log, bool clear, uint32 timelo, uint32 timehi, uint32 mask)
{
	taf_method_info_t* method = handle;
	int tid;

	/* free stats memory ? */
	if ((mac_log == NULL) && (scb_taf != NULL)) {
		for (tid = 0; tid < NUMPRIO; tid++) {
			taf_ias_sched_data_t* ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);
			taf_ias_dpstats_counters_t* counters = ias_sched ?
				ias_sched->rel_stats.dpstats_log : NULL;

			if (counters) {
				MFREE(TAF_WLCM(method)->pub->osh, counters,
					sizeof(*counters));
				ias_sched->rel_stats.dpstats_log  = NULL;
			}
		}
		return 0;
	};

	/* this is the 'auto' setting */
	if (mask & PKTQ_LOG_AUTO) {
		if (mask & PKTQ_LOG_DEF_PREC) {
			/* if was default (all TID) and AUTO mode is set,
			 * we can remove the "all TID"
			 */
			mask &= ~0xFFFF;
		} else if (mask & 0xFFFF) {
			/* this was not default, there is actual specified
			 * mask, so use it rather than automatic mode
			 */
			mask &= ~PKTQ_LOG_AUTO;
		}
	}
	mask |= PKTQ_LOG_DEVELOPMENT_VERSION;

	for (tid = 0; tid < NUMPRIO; tid++) {
		if (!(mask & (1 << tid)) && !(mask & PKTQ_LOG_AUTO)) {
			continue;
		}
		if (taf_ias_dpstats_counters(scb_taf, mac_log, tid, clear)) {
			mask |= (1 << tid);
		} else {
			mask &= ~(1 << tid);
		}
	}
	return mask;
}
#endif /* TAF_PKTQ_LOG */
#endif /* WLTAF_IAS */
