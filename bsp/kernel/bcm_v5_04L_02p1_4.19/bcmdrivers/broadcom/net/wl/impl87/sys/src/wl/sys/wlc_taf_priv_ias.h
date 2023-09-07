/*
 * wlc_taf_oriv_ias.h
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
 * $Id: wlc_taf_priv_ias.h 808142 2022-02-10 01:30:35Z $
 *
 */
#ifndef _wlc_taf_priv_ias_h_
#define _wlc_taf_priv_ias_h_

#if TAF_ENABLE_TIMER
#include <wl_export.h>
#endif

#define TAF_COEFF_IAS_DEF	4032
#define TAF_COEFF_IAS_FACTOR	12 /* (1<<12) is 4096, approx 4ms */
#define TAF_COEFF_IAS_MAX_BITS  12
#define TAF_COEFF_IAS_MAX	(1 << TAF_COEFF_IAS_MAX_BITS) /* coefficient for normalisation */
#define TAF_COEFF_IAS_MAX_CFG	(((1 << TAF_COEFF_IAS_MAX_BITS) * 255) / 256)
#define TAF_SCORE_IAS_MAX	(TAF_SCORE_MAX / TAF_COEFF_IAS_MAX) /* max val prior norm */
#define TAF_PKTCOUNT_SCALE_SHIFT	5
#define TAF_APKTCOUNT_SCALE_SHIFT	8

#define TAF_TIME_HIGH_MAX	16000
#define TAF_TIME_LOW_MAX	16000
#define TAF_TIME_HIGH_DEFAULT	5400
#define TAF_TIME_LOW_DEFAULT	(TAF_TIME_HIGH_DEFAULT - 1500)
#define TAF_TIME_ATOS_HIGH_DEFAULT	TAF_TIME_HIGH_DEFAULT
#define TAF_TIME_ATOS_LOW_DEFAULT	TAF_TIME_LOW_DEFAULT
#define TAF_TIME_ATOS2_HIGH_DEFAULT	(TAF_TIME_ATOS_HIGH_DEFAULT + 400)
#define TAF_TIME_ATOS2_LOW_DEFAULT	TAF_TIME_LOW_DEFAULT

/* to save computational effort, assume fix durations - packet overhead times in us */
#define TAF_NAR_OVERHEAD           330 /* biased a little high for adjusted 'fairness' */
#define TAF_AMPDU_NO_RTS_OVERHEAD  180
#define TAF_AMPDU_RTS_OVERHEAD     260
#define TAF_RTS_OVERHEAD           (TAF_AMPDU_RTS_OVERHEAD - TAF_AMPDU_NO_RTS_OVERHEAD)

#define TAF_IAS_RETRIG_TIME_NORM   505
#define TAF_IAS_RETRIG_TIME_UL     ((TAF_TIME_LOW_DEFAULT + 500) / 1000)

#define TAF_OPT_EXIT_EARLY         0
#define TAF_OPT_TID_RE_SORT        1
#define TAF_OPT_PKT_NAR_OVERHEAD   5
#define TAF_OPT_PKT_AMPDU_OVERHEAD 6
#define TAF_OPT_ULTID_IGNRE        12

#define TAF_OPT(m, f)              ((ias_cfg(m).options & (1 << TAF_OPT_##f)) != 0)

#define TAF_OPT_IAS_IMMED_LOOSE    0
#define TAF_OPT_IAS_TOOMUCH        1
#define TAF_OPT_IAS_TOOMUCH_CUMUL  2
#define TAF_OPT_IAS_TOOMUCH_LOOSE  3
#define TAF_OPT_IAS_SFAIL_LOOSE    4
#define TAF_OPT_IAS_AUTOBW20_SUPER 5
#define TAF_OPT_IAS_AUTOBW40_SUPER 6
#define TAF_OPT_IAS_RETRIGGER      8
#define TAF_OPT_IAS_FREE_RELEASE   9
#define TAF_OPT_IAS_FLUSH_PAIRING  10
#define TAF_OPT_IAS_FLUSH_FREE_REL 11
#define TAF_OPT_IAS_DEFER_UL       12
#define TAF_OPT_IAS_DEFER_SUPER    13
#define TAF_IAS_OPT(i, f)          (((i)->options & (1 << TAF_OPT_IAS_##f)) != 0)

#if TAF_ENABLE_MU_TX
#define TAF_OPT_MIMO_MEAN          0
#define TAF_OPT_MIMO_MIN           1
#define TAF_OPT_MIMO_MEDIAN        2
#define TAF_OPT_MIMO_MAX           3
#define TAF_OPT_MIMO_PAIR          4
#define TAF_OPT_MIMO_SWEEP         5
#define TAF_OPT_MIMO_AGG_HOLD      6
#define TAF_OPT_MIMO_PAIR_MAXMU    8
#define TAF_OPT_MIMO_MAX_STREAMS   9
#define TAF_OPT_MIMO_BRIDGE_PAIR_DL 17
#define TAF_MIMO_OPT(m, f)         ((ias_cfg(m).mu_mimo_opt & (1 << TAF_OPT_MIMO_##f)) != 0)

#define TAF_OPT_OFDMA_PAIR         4
#define TAF_OPT_OFDMA_SWEEP        5
#define TAF_OPT_OFDMA_AGG_HOLD     6
#define TAF_OPT_OFDMA_INT_EXTEND   7
#define TAF_OPT_OFDMA_TOO_SMALL    8
#define TAF_OPT_OFDMA_TOO_LARGE    9
#define TAF_OPT_OFDMA_MAXN_REL_DL 10
#define TAF_OPT_OFDMA_MAXN_REL_UL 11
#define TAF_OPT_OFDMA_ULTID_IGNRE 12
#define TAF_OPT_OFDMA_FORCE_PAIR  13
#define TAF_OPT_OFDMA_TOO_SMALLUL 14
#define TAF_OPT_OFDMA_TOO_LARGEUL 15
#define TAF_OPT_OFDMA_SPLIT_UL    16
#define TAF_OPT_OFDMA_BRIDGE_PAIR_DL 17
#define TAF_OPT_OFDMA_BRIDGE_PAIR_UL 18
#define TAF_OPT_OFDMA_MAX_BUMP_UL 19
#define TAF_OPT_OFDMA_UL_TRIGG_ST 20
#define TAF_OPT_OFDMA_UL_ODD      21

#define TAF_OFDMA_OPT(m, f)        ((ias_cfg(m).mu_ofdma_opt & (1 << TAF_OPT_OFDMA_##f)) != 0)
#endif /* TAF_ENABLE_MU_TX */

#define NUM_IAS_SCHEDULERS         (LAST_IAS_SCHEDULER - FIRST_IAS_SCHEDULER + 1)
#define IAS_INDEX(type)            ((type) - FIRST_IAS_SCHEDULER)
#define IAS_INDEXM(M)              IAS_INDEX((M)->type)

#define TAF_STATE_HIGHER_PEND      0

#if TAF_ENABLE_UL
#define TAF_IAS_RXMON_IDLE_LIMIT   30000
#define TAF_IAS_RXMON_HOLD_SMALL   400000
#define TAF_IAS_RXMON_HOLD_LARGE   2500000

#define TAF_PKT_SIZE_INIT_UL(rs)   MAX(TAF_PKT_SIZE_DEFAULT_UL, \
				   (rs)->data.pkt_size_max[TAF_TYPE_UL])

#define TAF_PKT_TAG_INDEX(p, type) \
	((type == TAF_TYPE_DL) ? TAF_GET_TAG_IDX(WLPKTTAG(p)) : WLULPKTTAG(p)->index)
#define TAF_PKT_TAG_UNITS(p, type) \
	((type == TAF_TYPE_DL) ? TAF_GET_TAG_UNITS(WLPKTTAG(p)) : WLULPKTTAG(p)->units)
#else
#define TAF_PKT_TAG_INDEX(p, type)	(WLPKTTAG(p)->taf.ias.index)
#define TAF_PKT_TAG_UNITS(p, type)	(WLPKTTAG(p)->pktinfo.taf.ias.units)
#define TAF_PKT_SIZE_INIT_UL(rs)   0
#endif /* TAF_ENABLE_UL */

typedef enum {
	TAF_TRIGGER_NONE,
	TAF_TRIGGER_IMMEDIATE,
	TAF_TRIGGER_IMMEDIATE_STAR,
	TAF_TRIGGER_IMMEDIATE_SUPER_RESCHED,
	TAF_TRIGGER_STAR_THRESHOLD,
	TAF_TRIGGER_TIMER,
	TAF_NUM_TRIGGERS
} taf_trigger_cause_t;

typedef struct {
	struct {
		uint32 released_units;
	} real[TAF_NUM_LIST_TYPES];
	struct {
		uint32 released_units;
	} total;
	struct {
		uint32 window_dur_units;
		uint32 rounding_units;
	} extend;
	struct {
		uint16  release_pcount;
		uint16  release_ncount;
		uint32  release_bytes;
		uint32  release_units;
#if TAF_ENABLE_SQS_PULL
		uint32  virtual_units;
#endif
	} data;
	uint32 total_units_in_transit[TAF_NUM_LIST_TYPES];
	uint32 cumul_units[TAF_MAX_NUM_WINDOWS][TAF_NUM_LIST_TYPES];
	uint32 total_sched_units[TAF_MAX_NUM_WINDOWS][TAF_NUM_LIST_TYPES];
	uint32 resched_units[2];
	uint16 high[NUM_IAS_SCHEDULERS];
	uint16 low[NUM_IAS_SCHEDULERS];
	uint16 release_type_present;
	uint16 bw_type_present;
	uint16 pairing_id;
	uint16 last_paired[TAF_NUM_LIST_TYPES];
	uint16 flow_release_count[TAF_NUM_LIST_TYPES];
	uint8  resched_index[2];
	int8   prev_resched_index;
	uint8  cycle_now;
	uint8  cycle_next;
	uint8  cycle_ready;
	uint8  was_reset;
	bool   star_packet_received;
	bool   waiting_schedule;
	bool   free_release;
	bool   fullsort;
	taf_traffic_map_t intermediate_sort;
	uint8  barrier_req;
	taf_trigger_cause_t trigger;
	uint32 est_release_units;
	uint32 super_sched_last;
} taf_ias_uni_state_t;

#ifdef TAF_DEBUG_VERBOSE
typedef struct {
	uint32     cumul_units[TAF_MAX_NUM_WINDOWS][TAF_NUM_LIST_TYPES];
	uint32     pending_units[TAF_MAX_NUM_WINDOWS][TAF_NUM_LIST_TYPES];
} taf_ias_stall_debug_t;
#endif

#if TAF_ENABLE_UL
#define TAF_IAS_TUIT(uni, type)		(((type) == TAF_TYPE_ALL) ? \
					((uni)->total_units_in_transit[TAF_TYPE_DL] + \
					(uni)->total_units_in_transit[TAF_TYPE_UL]) : \
					(uni)->total_units_in_transit[type])

#define TAF_IAS_REAL(uni, type)		(((type) == TAF_TYPE_ALL) ? \
					((uni)->real[TAF_TYPE_DL].released_units + \
					(uni)->real[TAF_TYPE_UL].released_units) : \
					(uni)->real[type].released_units)

#define TAF_IAS_CUMUL(uni, idx, type)    \
					(((type) == TAF_TYPE_ALL) ? \
					((uni)->cumul_units[idx][TAF_TYPE_DL] + \
					(uni)->cumul_units[idx][TAF_TYPE_UL]) : \
					(uni)->cumul_units[idx][type])

#define TAF_IAS_TOTAL_SCHED(uni, idx, type)    \
					(((type) == TAF_TYPE_ALL) ? \
					((uni)->total_sched_units[idx][TAF_TYPE_DL] + \
					(uni)->total_sched_units[idx][TAF_TYPE_UL]) : \
					(uni)->total_sched_units[idx][type])
#else
#define TAF_IAS_TUIT(uni, type)		(uni)->total_units_in_transit[TAF_TYPE_DL]
#define TAF_IAS_REAL(uni, type)		(uni)->real[TAF_TYPE_DL].released_units
#define TAF_IAS_CUMUL(uni, idx, type)	(uni)->cumul_units[idx][TAF_TYPE_DL]
#define TAF_IAS_TOTAL_SCHED(uni, idx, type)	\
					(uni)->total_sched_units[idx][TAF_TYPE_DL]

#endif /* TAF_ENABLE_UL */

#define TAF_IAS_RESCHED_UNITS(ts)  ((ts)->resched_units[(ts)->cycle_next])
#define TAF_IAS_RESCHED_IDX(ts)    ((ts)->resched_index[(ts)->cycle_next])
#define TAF_IAS_EXTEND(ts)         ((ts)->extend.window_dur_units + (ts)->extend.rounding_units)

#ifdef TAF_PKTQ_LOG
typedef struct {
	uint32  time_lo;
	uint32  time_hi;
	uint32  emptied;
	uint32  max_did_rel_delta;
	uint32  forced;
	uint32  ready;
	uint32  pwr_save;
	uint32  did_release;
	uint32  restricted;
	uint32  limited;
	uint32  held_z;
	uint32  release_pcount;
	uint32  release_ncount;
	uint32  pkt_size_mean;
	uint32  release_frcount;
	uint64  release_time;
	uint64  release_bytes;
	uint32  did_rel_delta;
	uint32  did_rel_timestamp;
} taf_ias_dpstats_counters_t;

#define TAF_DPSTATS_LOG_ADD(stats, var, val)	do { if ((stats)->dpstats_log) { \
							(stats)->dpstats_log->var += (val); \
						} } while (0)

#define TAF_DPSTATS_LOG_SET(stats, var, val)	do { if ((stats)->dpstats_log) { \
							(stats)->dpstats_log->var = (val); \
						} } while (0)

#else
#define TAF_DPSTATS_LOG_ADD(stats, var, val)	do {} while (0)
#define TAF_DPSTATS_LOG_SET(stats, var, val)	do {} while (0)
#endif /* TAF_PKTQ_LOG */

typedef struct {
	uint32 index[TAF_NUM_LIST_TYPES];
	/* the following is used for tracking average packet size */
	struct {
		uint16  pkt_size_mean[TAF_NUM_LIST_TYPES];
		uint16  pkt_size_max[TAF_NUM_LIST_TYPES];
		uint16  pkt_size_min[TAF_NUM_LIST_TYPES];
		uint32  total_scaled_ncount;
		uint32  total_scaled_pcount;
		uint32  total_bytes;
		uint32  total_release_units;
		uint32  calc_timestamp;
#if TAF_ENABLE_UL
		struct {
			uint32  total_units;
			uint32  total_bytes;
			uint32  calc_timestamp;
			uint32  sched_count;
#ifdef TAF_DBG
			uint32  update_count;
#endif
		} ul;
		struct {
			uint32  bytes;
			uint32  max_aggn_timestamp;
			uint32  max_pcount;
			uint32  scaled_pcount;
			uint32  scaled_acount;
			uint32  calc_timestamp;
			uint32  prev_timestamp;
			uint32  time;
			uint32  max_plen_timestamp;
			uint32  min_plen_timestamp;
			uint32  scaled_qncount;
		} rx;
#endif /* TAF_ENABLE_UL */
	} data;
#ifdef TAF_PKTQ_LOG
	taf_ias_dpstats_counters_t* dpstats_log;
#endif /* TAF_PKTQ_LOG */
} taf_ias_sched_rel_stats_t;

#define TAF_BASE1_SET(var, val)    do { uint32 _base1 = (val); TAF_ASSERT(_base1 > 0); \
					var = (_base1 - 1); } while (0)

#define TAF_BASE1_GET(var)         ((var) + 1)

#include <packed_section_start.h>

#define TAF_IAS_MAX_PAIRING_BITS   9
#define TAF_IAS_MAX_PAIRING        ((1 << TAF_IAS_MAX_PAIRING_BITS) - 1)

#define TAF_IAS_MAX_UNITS_BITS     17
#define TAF_IAS_MAX_UNITS          ((1 << TAF_IAS_MAX_UNITS_BITS) - 1)

#define TAF_IAS_MAX_MU_BITS        4
#define TAF_IAS_MAX_MU             TAF_BASE1_GET((1 << TAF_IAS_MAX_MU_BITS) - 1)

#if TAF_ENABLE_MU_TX
#if TAF_MAX_MU > TAF_IAS_MAX_MU
#error TAF_IAS_MAX_MU
#endif
#endif /* TAF_ENABLE_MU_TX */

typedef struct {
	union {
		struct {
			uint32  units:TAF_IAS_MAX_UNITS_BITS;
			uint32  done:1;
			uint32  force:1;
			uint32  pairing_id:TAF_IAS_MAX_PAIRING_BITS;
			uint32  mu:TAF_IAS_MAX_MU_BITS;
		};
		uint32 whole;
	};
} taf_ias_rel_config_t;

TAF_COMPILE_ASSERT(taf_ias_rel_config_t, sizeof(taf_ias_rel_config_t) <= sizeof(uint32));

#define TAF_SET_AGG(var, val)      TAF_BASE1_SET(var, val)
#define TAF_GET_AGG(var)           TAF_BASE1_GET(var)

#define TAF_IAS_MAX_AGGN_BITS      8
#define TAF_IAS_MAX_AGGN           TAF_GET_AGG(((1 << TAF_IAS_MAX_AGGN_BITS) - 1))

#if TAF_IAS_MAX_AGGN < AMPDU_BA_MAX_WSIZE
#error TAF_IAS_MAX_AGGN too small
#endif

#define TAF_IAS_MAX_AGGSF_BITS     3
#define TAF_IAS_MAX_AGGSF          TAF_GET_AGG(((1 << TAF_IAS_MAX_AGGSF_BITS) - 1))

typedef struct BWL_PRE_PACKED_STRUCT taf_ias_sched_data {
	taf_sched_handle_t handle[TAF_NUM_SCHED_SOURCES];
	taf_ias_sched_rel_stats_t rel_stats;
	taf_ias_rel_config_t  release_config[TAF_NUM_LIST_TYPES];
	uint32             used:TAF_NUM_SCHED_SOURCES;
	uint32             source_updated:TAF_NUM_SCHED_SOURCES;
	uint32             aggsf:TAF_IAS_MAX_AGGSF_BITS;
#if TAF_ENABLE_UL
	uint32             rx_aggn:TAF_IAS_MAX_AGGN_BITS;
	uint32             rx_max_aggn:TAF_IAS_MAX_AGGN_BITS;
	uint32             last_isolate;
#endif
} BWL_POST_PACKED_STRUCT taf_ias_sched_data_t;
#include <packed_section_end.h>

#define TAF_IAS_STATIC_CONTEXT     TAF_STATIC_CONTEXT

#if TAF_IAS_STATIC_CONTEXT
/* size check */
TAF_COMPILE_ASSERT(taf_ias_sched_data_t, sizeof(taf_ias_sched_data_t) <= TAF_STATIC_CONTEXT_SIZE);
#endif

typedef struct taf_ias_coeff {
	uint8              time_shift;
	uint16             coeff1;
	uint16             coeff10;
	uint16             coeff100;
	uint32             normalise;
} taf_ias_coeff_t;

typedef struct taf_ias_method_info {
	uint32             high;
	uint32             low;
	uint32             total_score;
	taf_ias_coeff_t    coeff;
	uint8*             score_weights;
	uint32             release_limit;
	uint32             barrier;
	uint32             opt_aggp_limit;
	bool               data_block;
	taf_traffic_map_t  tid_active;
	taf_traffic_map_t  sched_active;
	uint32             options;
#if TAF_ENABLE_MU_TX
	uint16*            mu_pair;
	uint16*            mu_user_rel_limit[TAF_NUM_MU_TECH_TYPES];
	uint32             mu_mimo_opt;
	uint32             mu_ofdma_opt;
	uint8              active_mu_count[TAF_NUM_MU_TECH_TYPES][TAF_MAXPRIO];
#endif /* TAF_ENABLE_MU_TX */
	uint16             min_mpdu_dur[TAF_NUM_LIST_TYPES];
#if TAF_ENABLE_SQS_PULL
	uint16             pre_rel_limit[TAF_NUM_SCHED_SOURCES];
	uint32             margin;
#endif /* TAF_ENABLE_SQS_PULL */
#ifdef TAF_DEBUG_VERBOSE
	uint32             bodge;
#endif
} taf_ias_method_info_t;

typedef enum {
	TAF_IAS_CYCLE_START,
	TAF_IAS_CYCLE_COMPLETE
} taf_ias_state_t;

/* taf_ias_info is GLOBAL shared across all different EBOS/PRR/ATOS/ATOS2 */
typedef struct taf_ias_group_info {
	taf_ias_uni_state_t unified_state;
	/* ias_ready_to_schedule is here so all EBOS/ATOS etc schedulers can peek status */
	taf_traffic_map_t  ias_ready_to_schedule[NUM_IAS_SCHEDULERS];
	taf_ias_state_t    ias_state;
#if TAF_ENABLE_TIMER
	struct wl_timer *  agg_hold_timer;
	uint32             agg_hold_start_time;
	uint32             agg_hold_threshold;
	bool               is_agg_hold_timer_running;
	bool               agg_hold_exit_pending;
	bool               agg_hold_expired;
	bool               agg_hold_prevent;
	bool               stall_prevent_timer;
	uint8              agg_hold;
	uint16             retrigger_timer[TAF_NUM_LIST_TYPES];
#endif /* TAF_ENABLE_TIMER */
	uint8              prev_tag_star_index; /* used to separate scheduler periods */
	uint8              tag_star_index;     /* used to separate scheduler periods */
	uint32             immed;
	uint32             options;
	uint32             now_time;
	uint32             cpu_time;
	uint32             cpu_elapsed_time;
#ifdef TAF_DBG
	uint32             data_block_start;
	uint32             data_block_total;
	uint32             data_block_prev_in_transit;
#endif
	uint32             ncount_flow_last;
	uint32             ncount_flow;
	uint32             sched_start;
	struct {
		uint32 uflow_low;
		uint32 uflow_high;
		uint32 target_high;
		uint32 target_low;
		uint32 immed_trig;
		uint32 cumulative_immed_trig;
		uint32 immed_star;
		uint32 prev_release_time;
		uint32 time_delta;
		uint32 super_sched;
		uint32 super_sched_collapse;
	} data;
	struct {
		uint32 uflow_low;
		uint32 uflow_high;
		uint32 target_high;
		uint32 target_low;
		uint32 nothing_released;
		uint32 immed_trig;
		uint32 immed_star;
		uint32 max_cumulative_immed_trig;
		uint32 average_high;
		uint32 sched_duration;
		uint32 max_duration;
		uint32 super_sched;
		uint32 super_sched_collapse;
		uint32 barrier_req;
		uint32 traffic_stat_overflow;
#if TAF_ENABLE_TIMER
		uint32 agg_hold_count;
		uint32 agg_retrigger_count;
		uint32 agg_hold_max_time;
		uint64 agg_hold_total_time;
#endif
#ifdef TAF_DEBUG_VERBOSE
		uint32 barrier_start;
		uint32 wait_start;
#endif /* TAF_DEBUG_VERBOSE */
	} debug;
} taf_ias_group_info_t;

typedef struct {
	taf_method_info_t       method;
	taf_ias_method_info_t   ias;
} taf_ias_container_t;

#define TAF_IAS_GROUP_INFO(_t) \
			((taf_ias_group_info_t*)((_t)->group_context[TAF_SCHEDULER_IAS_METHOD]))

#define TAF_IAS_METHOD_GROUP_INFO(_m)   (taf_ias_group_info_t*)TAF_METHOD_GROUP_INFO(_m)

#define TAF_IAS_TID_STATE(scb_taf, tid) (taf_ias_sched_data_t*)(TAF_CUBBY_TIDINFO(scb_taf, tid))

#define TAF_IAS_TID_STATS(_s_t, _t)     &((TAF_IAS_TID_STATE(_s_t, _t))->rel_stats)

#define TAF_IAS_NOWTIME(_m)             *((_m)->now_time_p)
#define TAF_IAS_INFO_NOWTIME(_i)        (_i)->now_time

#define TAF_IAS_NOWTIME_SYNC(_m, _ts)   do { \
						*((_m)->now_time_p) = (_ts); \
					} while (0)

#define taf_get_uni_state(_taf)         &(TAF_IAS_GROUP_INFO(_taf)->unified_state)

#define ias_cfg(method)                 ((taf_ias_container_t*)method)->ias

#define TAF_IAS_SOURCE_ENABLED(_i, _s)   ((_i)->used & (1 << (_s)))

#define TAF_IAS_DECAY64(coeff, param)   do { \
						uint64 _lcalc; \
						_lcalc = (uint64)(param) * (uint64)(coeff); \
						_lcalc >>= TAF_COEFF_IAS_MAX_BITS; \
						param = _lcalc; \
					} while (0)

#define taf_ias_coeff_p(_m)             (&(ias_cfg(_m).coeff))
#define taf_ias_norm(_m)                (ias_cfg(_m).coeff.normalise)

#define TAF_IAS_SRC_UPDATED(_c, _t, _s) (((TAF_IAS_TID_STATE(_c, _t))->source_updated & \
						(1 << (_s))) != 0)
#if TAF_ENABLE_UL
#define TAF_IAS_UL_WAS_BUMPED(_c, _t)   TAF_IAS_SRC_UPDATED(_c, _t, TAF_SOURCE_UL)
#endif

static void* BCMATTACHFN(taf_ias_method_attach)(wlc_taf_info_t *taf_info, taf_scheduler_kind type);
static int   BCMATTACHFN(taf_ias_method_detach)(void* context);

static void taf_ias_up(void* context);
static int  taf_ias_down(void* context);
static taf_list_t ** taf_ias_get_list_head_ptr(void * context);

static int  taf_ias_dump(void *handle, struct bcmstrbuf *b);
static bool taf_ias_rate_override(void* handle, ratespec_t rspec, wlcband_t *band);

static bool taf_ias_link_state(void *handle, struct scb* scb, int tid, taf_source_type_t s_idx,
	taf_link_state_t state);
static int taf_ias_scb_state(void *handle, struct scb* scb, taf_source_type_t s_idx,
	void* update, taf_scb_state_t state);

static bool taf_ias_tx_status(void * handle, taf_scb_cubby_t* scb_taf, int tid, void * p,
	taf_txpkt_state_t status);

static bool taf_ias_rx_status(void * handle, taf_scb_cubby_t* scb_taf, int tid, int count,
	void * data, taf_rxpkt_state_t status);

static int taf_ias_iovar(void *handle, taf_scb_cubby_t * scb_taf, const char* cmd,
	wl_taf_define_t* result, struct bcmstrbuf* b);

static void taf_ias_sched_state(void *, taf_scb_cubby_t *, int tid, int count,
	taf_source_type_t s_idx, taf_sched_state_t);

static int taf_ias_traffic_stats(void *, taf_scb_cubby_t *, int, taf_traffic_stats_t *);
static int taf_ias_calc_trfstats(taf_method_info_t* method, taf_scb_cubby_t * scb_taf, int tid,
	uint32* now_time, taf_traffic_stats_t * stat);

static void taf_ias_unified_reset(taf_method_info_t* method);

static void taf_ias_decay_stats(taf_ias_sched_rel_stats_t* rel_stats, taf_ias_coeff_t* decay,
	taf_list_type_t type, uint32 now_time);

#if TAF_ENABLE_UL
static void taf_ias_rx_mon_clear(taf_method_info_t* method, taf_ias_sched_rel_stats_t* rel_stats,
	uint32 now_time);
#endif /* TAF_ENABLE_UL */

static bool taf_ias_data_block(taf_method_info_t *method);

static bool BCMFASTPATH
taf_get_rate(wlc_taf_info_t *taf_info, taf_scb_cubby_t* scb_taf, taf_rspec_index_t rindex,
	taf_rate *result);

static uint32 taf_ias_dltraffic_wait(taf_method_info_t* method, struct scb* scb);

#ifdef TAF_PKTQ_LOG
static uint32 taf_ias_dpstats_dump(void * handle, taf_scb_cubby_t* scb_taf,
	mac_log_counters_v06_t* mac_log, bool clear, uint32 timelo, uint32 timehi,
	uint32 prec_mask);
#endif

#if TAF_ENABLE_SQS_PULL
static void taf_iasm_sched_state(void *, taf_scb_cubby_t *, int tid, int count,
	taf_source_type_t s_idx, taf_sched_state_t);
#endif /* TAF_ENABLE_SQS_PULL */

#if TAF_ENABLE_UL
static INLINE uint32 BCMFASTPATH taf_ias_rx_pcount_flow(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_rel_stats_t* rel_stats = TAF_IAS_TID_STATS(scb_taf, tid);

	return (rel_stats->data.rx.scaled_pcount * (10000 >> TAF_PKTCOUNT_SCALE_SHIFT)) /
	       (rel_stats->data.rx.time / 100);
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_acount_flow(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_rel_stats_t* rel_stats = TAF_IAS_TID_STATS(scb_taf, tid);

	return (rel_stats->data.rx.scaled_acount * (10000 >> TAF_APKTCOUNT_SCALE_SHIFT)) /
	       (rel_stats->data.rx.time  / 100);
}

static INLINE uint32 BCMFASTPATH taf_ias_rel_stats_rx_aggn(taf_ias_sched_rel_stats_t* rel_stats)
{
	uint32 aggn = 1;

	if (rel_stats->data.rx.scaled_acount > (1 << TAF_APKTCOUNT_SCALE_SHIFT)) {
		aggn = (rel_stats->data.rx.scaled_pcount << TAF_APKTCOUNT_SCALE_SHIFT) /
		       rel_stats->data.rx.scaled_acount;

		aggn += (1 << TAF_PKTCOUNT_SCALE_SHIFT) >> 1;
		aggn >>= TAF_PKTCOUNT_SCALE_SHIFT;
	}
	if (aggn == 0 || aggn > AMPDU_BA_MAX_WSIZE) {
		WL_TAF(("%s: aggn=%u, scaled_pcount=%u, scaled_acount=%u\n", __FUNCTION__, aggn,
			rel_stats->data.rx.scaled_pcount, rel_stats->data.rx.scaled_acount));
		aggn = (aggn == 0) ? 1 : AMPDU_BA_MAX_WSIZE;
	}
	return aggn;
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_aggn(taf_scb_cubby_t * scb_taf, int tid)
{
	uint32 result = taf_ias_rel_stats_rx_aggn(TAF_IAS_TID_STATS(scb_taf, tid));
	TAF_ASSERT(result <= TAF_IAS_MAX_AGGN);
	return result;
}

static INLINE uint32 BCMFASTPATH taf_ias_rel_stats_rx_max_aggn(taf_ias_sched_rel_stats_t* rel_stats)
{
	uint32 aggn = 16;

	if (rel_stats->data.rx.scaled_acount > (1 << TAF_APKTCOUNT_SCALE_SHIFT)) {
		aggn = (rel_stats->data.rx.max_pcount << TAF_APKTCOUNT_SCALE_SHIFT) /
		       rel_stats->data.rx.scaled_acount;

		aggn += (15 << TAF_PKTCOUNT_SCALE_SHIFT) >> 4;
		aggn >>= TAF_PKTCOUNT_SCALE_SHIFT;
	}
	if (aggn == 0 || aggn > AMPDU_BA_MAX_WSIZE) {
		WL_TAF(("%s: aggn=%u, max_pcount=%u, scaled_acount=%u\n", __FUNCTION__, aggn,
			rel_stats->data.rx.max_pcount, rel_stats->data.rx.scaled_acount));
		aggn = (aggn == 0) ? 16 : AMPDU_BA_MAX_WSIZE;
	}
	return aggn;
}

static INLINE uint32 BCMFASTPATH
taf_ias_rel_stats_rx_max_pkt_size(taf_ias_sched_rel_stats_t* rel_stats)
{
	return rel_stats->data.pkt_size_max[TAF_TYPE_UL];
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_max_pkt_size(taf_scb_cubby_t * scb_taf, int tid)
{
	uint32 result = taf_ias_rel_stats_rx_max_pkt_size(TAF_IAS_TID_STATS(scb_taf, tid));

	return result;
}

static INLINE uint32 BCMFASTPATH taf_ias_rel_stats_qn10_rate(taf_ias_sched_rel_stats_t* rel_stats)
{
	uint32 qn_count = 0;

	if (rel_stats->data.rx.time >= 10) {
		qn_count = (10 * 100000) >> TAF_PKTCOUNT_SCALE_SHIFT;
		qn_count *= rel_stats->data.rx.scaled_qncount;
		qn_count /= (rel_stats->data.rx.time / 10);
	}
	return qn_count;
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_qn10_rate(taf_scb_cubby_t * scb_taf, int tid)
{
	uint32 result = taf_ias_rel_stats_qn10_rate(TAF_IAS_TID_STATS(scb_taf, tid));
	return result;
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_max_aggn(taf_scb_cubby_t * scb_taf, int tid)
{
	uint32 result = taf_ias_rel_stats_rx_max_aggn(TAF_IAS_TID_STATS(scb_taf, tid));
	TAF_ASSERT(result <= TAF_IAS_MAX_AGGN);
	return result;
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_Mbits_flow(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_rel_stats_t* rel_stats = TAF_IAS_TID_STATS(scb_taf, tid);

	return (rel_stats->data.rx.bytes * 8) / rel_stats->data.rx.time;
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_Kbits_flow(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_rel_stats_t* rel_stats = TAF_IAS_TID_STATS(scb_taf, tid);

	return (rel_stats->data.rx.bytes * 8) /
	       ((rel_stats->data.rx.time + 500) / 1000);
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_Kbits_phy(taf_scb_cubby_t * scb_taf)
{
	ratespec_t ratespec =  scb_taf->info.scb_stats.global.rdata.rate[TAF_RSPEC_UL].rspec;

	if (ratespec == 0) {
		taf_rate result;
		if (taf_get_rate(scb_taf->method->taf_info, scb_taf, TAF_RSPEC_UL, &result)) {
			scb_taf->info.scb_stats.global.rdata.rate[TAF_RSPEC_UL].rspec =
				ratespec = result.rspec;
			scb_taf->info.scb_stats.global.rdata.rate[TAF_RSPEC_UL].rate =
				wf_rspec_to_rate(ratespec);
		}
	}
	if (ratespec != 0) {
		return scb_taf->info.scb_stats.global.rdata.rate[TAF_RSPEC_UL].rate;
	}
	return 0;
}

static INLINE uint32 BCMFASTPATH taf_ias_sched_rx_Kbits_flow(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_rel_stats_t* rel_stats = TAF_IAS_TID_STATS(scb_taf, tid);

	return (rel_stats->data.ul.total_bytes * 8) /
	       ((taf_ias_norm(scb_taf->method) + 500) / 1000);
}

static INLINE int32 BCMFASTPATH taf_ias_sched_rx_deci_overpkts(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_rel_stats_t* rel_stats = TAF_IAS_TID_STATS(scb_taf, tid);

	uint32 norm = taf_ias_norm(scb_taf->method) + (1 << TAF_PKTCOUNT_SCALE_SHIFT);

	uint32 sched_pkts = (10 * rel_stats->data.ul.total_bytes) /
		rel_stats->data.pkt_size_mean[TAF_TYPE_UL];

	uint32 rx_pkts =
		(rel_stats->data.rx.scaled_pcount * (norm >> (TAF_PKTCOUNT_SCALE_SHIFT + 1))) /
		((rel_stats->data.rx.time + 10) / 20);

	int32 deci_pkts = (sched_pkts - rx_pkts);

	deci_pkts *= (int32)(1 << TAF_APKTCOUNT_SCALE_SHIFT);
	deci_pkts /= (int32)rel_stats->data.ul.sched_count;

	return deci_pkts;
}

static INLINE uint32 BCMFASTPATH taf_ias_rel_stats_rx_pkt_size(taf_ias_sched_rel_stats_t* rel_stats)
{
	uint32 mean = TAF_PKT_SIZE_INIT_UL(rel_stats);

	if (rel_stats->data.rx.scaled_pcount > 0) {
		TAF_ASSERT(rel_stats->data.rx.bytes < (1 << (32 - TAF_PKTCOUNT_SCALE_SHIFT)));

		mean = rel_stats->data.rx.bytes << TAF_PKTCOUNT_SCALE_SHIFT;
		mean /= (rel_stats->data.rx.scaled_pcount);
	}
	return mean;
}

static INLINE uint32 BCMFASTPATH taf_ias_rx_pkt_size(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_rel_stats_t* rel_stats = TAF_IAS_TID_STATS(scb_taf, tid);
	uint32 mean = taf_ias_rel_stats_rx_pkt_size(rel_stats);

	if (mean < 32) {
		WL_TAFM2(scb_taf->method, MACF" tid %u pktsize %u (%u, %u)\n",
			TAF_ETHERC(scb_taf), tid, mean,
			rel_stats->data.rx.bytes, rel_stats->data.rx.scaled_pcount);
		/* ensure pkt size is not less than 32 bytes */
		mean = 32;
	}

	if (rel_stats->data.pkt_size_max[TAF_TYPE_UL] > 0 &&
		mean > rel_stats->data.pkt_size_max[TAF_TYPE_UL]) {

		if (rel_stats->data.rx.scaled_pcount > (16 << TAF_PKTCOUNT_SCALE_SHIFT)) {
			uint32 err = mean - rel_stats->data.pkt_size_max[TAF_TYPE_UL];

			if (err > 1) {
				WL_TAFM1(scb_taf->method, MACF" tid %u, mean (%u) > max (%u)\n",
					TAF_ETHERC(scb_taf), tid, mean,
					rel_stats->data.pkt_size_max[TAF_TYPE_UL]);
			}
		}
		mean = rel_stats->data.pkt_size_max[TAF_TYPE_UL];
	}
	if (rel_stats->data.pkt_size_min[TAF_TYPE_UL] > 0 &&
		mean < rel_stats->data.pkt_size_min[TAF_TYPE_UL]) {

		if (rel_stats->data.rx.scaled_pcount > (16 << TAF_PKTCOUNT_SCALE_SHIFT)) {
			uint32 err = rel_stats->data.pkt_size_min[TAF_TYPE_UL] - mean;

			if (err > 1) {
				WL_TAFM1(scb_taf->method, MACF" tid %u, mean (%u) < min (%u)\n",
					TAF_ETHERC(scb_taf), tid, mean,
					rel_stats->data.pkt_size_min[TAF_TYPE_UL]);
			}
		}
		mean = rel_stats->data.pkt_size_min[TAF_TYPE_UL];
	}
	return mean;
}

static INLINE uint32 BCMFASTPATH taf_ias_ul_agglen(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_data_t* ias_sched = TAF_IAS_TID_STATE(scb_taf, tid);
	uint32 agglen = 0;

	if (ias_sched) {
		agglen = TAF_GET_AGG(ias_sched->rx_aggn) *
			ias_sched->rel_stats.data.pkt_size_mean[TAF_TYPE_UL];
	}
	return agglen;
}
#endif /* TAF_ENABLE_UL */

static INLINE uint8 BCMFASTPATH taf_ias_achvd_aggsf(taf_scb_cubby_t * scb_taf, int tid)
{
	taf_ias_sched_rel_stats_t* rel_stats = TAF_IAS_TID_STATS(scb_taf, tid);
	uint32 ppkts = rel_stats->data.total_scaled_pcount;

	if (ppkts > (20 << TAF_PKTCOUNT_SCALE_SHIFT)) {
		uint32 aggsf = (rel_stats->data.total_scaled_ncount + (ppkts >> 1)) / ppkts;

		return MAX(aggsf, 1);
	}
	return 1;
}

static INLINE uint8 BCMFASTPATH taf_ias_aggsf(taf_scb_cubby_t *scb_taf, int tid)
{
	uint8 achieved = taf_ias_achvd_aggsf(scb_taf, tid);
	uint8 limit = taf_aggsf(scb_taf, tid);
	uint8 result = MIN(achieved, limit);

	TAF_ASSERT(result <= TAF_IAS_MAX_AGGSF);
	return result;
}

static INLINE uint32 BCMFASTPATH taf_ias_mpdus(taf_scb_cubby_t *scb_taf, int tid, uint32 len)
{
	uint8 aggsf = taf_ias_aggsf(scb_taf, tid);

	if (aggsf > 0) {
		return len / aggsf;
	}
	return 0;
}
static INLINE bool BCMFASTPATH taf_ias_is_nar_traffic(taf_scb_cubby_t * scb_taf, int tid,
	taf_source_type_t s_idx)
{
#if TAF_ENABLE_NAR
	bool is_nar = TAF_SOURCE_IS_NAR(s_idx);

	if (!is_nar && TAF_SOURCE_IS_DL(s_idx) &&
		scb_taf->info.linkstate[TAF_SOURCE_AMPDU][tid] != TAF_LINKSTATE_ACTIVE &&
		scb_taf->info.scb_stats.ias.data.use[TAF_SOURCE_NAR]) {

		is_nar = (scb_taf->info.linkstate[TAF_SOURCE_NAR][tid] == TAF_LINKSTATE_ACTIVE);
	}
	return is_nar;
#else
	return FALSE;
#endif /* TAF_ENABLE_NAR */
}

#if TAF_ENABLE_MU_BOOST
static INLINE uint8 BCMFASTPATH taf_ias_boost_factor(taf_scb_cubby_t * scb_taf)
{
	wlc_taf_info_t* taf_info = scb_taf->method->taf_info;

	if (scb_taf->info.mu_tech_en[TAF_TECH_DL_VHMUMIMO]) {
		return taf_info->mu_boost_factor[TAF_TECH_DL_VHMUMIMO];

	} else if (scb_taf->info.mu_tech_en[TAF_TECH_DL_HEMUMIMO]) {
		return taf_info->mu_boost_factor[TAF_TECH_DL_HEMUMIMO];
	}
	return 0;
}
#endif /* TAF_ENABLE_MU_BOOST */

static INLINE uint32 BCMFASTPATH taf_ias_window_total(taf_method_info_t* method,
		taf_ias_uni_state_t* uni_state,  taf_list_type_t type)
{
	uint32 time_limit_units = TAF_MICROSEC_TO_UNITS(uni_state->high[IAS_INDEXM(method)]);

	time_limit_units += TAF_IAS_EXTEND(uni_state);

	WL_TAFM4(method, "high = %u, extend = %u/%u, total = %u\n",
		TAF_MICROSEC_TO_UNITS(uni_state->high[IAS_INDEXM(method)]),
		uni_state->extend.window_dur_units, uni_state->extend.rounding_units,
		time_limit_units);

	return time_limit_units;
}

static INLINE uint32 BCMFASTPATH taf_ias_window_remain(taf_method_info_t* method,
	taf_ias_uni_state_t* uni_state,  taf_list_type_t type)
{
	uint32 time_limit_units = taf_ias_window_total(method, uni_state, type);
	uint32 remain;

	if (time_limit_units > uni_state->total.released_units) {
		remain = time_limit_units - uni_state->total.released_units;
	} else {
		remain = 0;
	}
	WL_TAFM4(method, "total = %u, released = %u, remain = %u\n",
		time_limit_units, uni_state->total.released_units, remain);

	return remain;
}

#define TAF_ODD_USERS(usercount)   ((usercount) <= 1) ? FALSE : ((usercount & (usercount - 1)) != 0)

#endif /* _wlc_taf_priv_ias_h_ */
