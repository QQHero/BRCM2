/*
 * Copyright (C) 2022, Broadcom. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: igsc.h 679290 2017-01-13 07:39:40Z $
 */

#ifndef _IGSC_H_
#define _IGSC_H_

#define IGSDB_HASHT_SIZE                        8

#define IGMPV2_TYPE_OFFSET                      0
#define IGMPV2_MAXRSP_TIME_OFFSET               1
#define IGMPV2_CHECKSUM_OFFSET                  2
#define IGMPV2_GRP_ADDR_OFFSET                  4

#define IGMPV2_HOST_MEMBERSHIP_QUERY            0x11
#define IGMPV2_HOST_MEMBERSHIP_REPORT           0x12
#define IGMPV2_HOST_NEW_MEMBERSHIP_REPORT       0x16
#define IGMPV2_LEAVE_GROUP_MESSAGE              0x17

#define IGMPV2_QUERY_INTV                       130000
#define IGMPV2_MAXRSP_TIME                      10000
#define IGMPV2_ROBUSTNESS_VAR                   2
#define IGMPV2_GRP_MEM_INTV                     ((IGMPV2_ROBUSTNESS_VAR * \
	                                         IGMPV2_QUERY_INTV) + IGMPV2_MAXRSP_TIME)

#define IPV4_MIN_HLEN                           20
#define IGMP_HLEN                               8
#define IPV4_LEN_OFFSET                         2
#define IPV4_ID_OFFSET                          4
#define IPV4_FRAG_OFFSET                        6
#define IPV4_TTL_OFFSET                         8
#define IPV4_MCADDR_ALLHOSTS                    (0xe0000001U)
#define IP_ISMULTI(addr)                        (((addr) & 0xf0000000) == 0xe0000000)

#define IGSC_STATS_INCR(igsc, member)           (((igsc)->stats.member)++)
#define IGSC_STATS_DECR(igsc, member)           (((igsc)->stats.member)--)

#if defined(BCM_NBUFF_WLMCAST_IPV6)
#define IGSC_STATS_IPV6_INCR(igsc, member)           (((igsc)->stats_ipv6.member)++)
#define IGSC_STATS_IPV6_DECR(igsc, member)           (((igsc)->stats_ipv6.member)--)
#endif

/*
 * IGMP Snooping Layer data
 */
typedef struct igsc_info
{
	                                        /* IGMP Snooping database */
	clist_head_t     mgrp_sdb[IGSDB_HASHT_SIZE];
	osl_lock_t       sdb_lock;              /* Lock for IGSDB access */
	struct emfc_info *emf_handle;           /* Handle for EMF interfaces */
	uint32           grp_mem_intv;          /* Group Membership interval */
	uint32           query_intv;            /* Query interval */
	void             *osh;                  /* OS Layer handle */
	void             *igs_info;             /* OS Specific IGS data */
	emfc_snooper_t   snooper;               /* Snooper data */
	igsc_wrapper_t   wrapper;               /* Wrapper data */
	clist_head_t     rtlist_head;           /* Router port list head */
	osl_lock_t       rtlist_lock;           /* Lock for router port list */
	igs_stats_t      stats;                 /* Multicast frames statistics */

#if defined(BCM_NBUFF_WLMCAST_IPV6)
	clist_head_t     mgrp_sdb_ipv6[IGSDB_HASHT_SIZE];
	osl_lock_t       sdb_lock_ipv6;              /* Lock for IGSDB access */
	igs_stats_t      stats_ipv6;                 /* Multicast frames statistics */
	int8             inst_id[16];                /* IGSC instance identifier */
#endif

} igsc_info_t;

#define IGSC_INFO(s) ((igsc_info_t *)((char *)(s)-(unsigned long)(&((igsc_info_t *)0)->snooper)))

/*
 * Router port list entry
 */
typedef struct igsc_rtlist
{
	clist_head_t     list;                  /* Router port list prev and next */
	void             *ifp;                  /* Interface on which mcast router
	                                         * is seen.
	                                         */
	uint32           mr_ip;                 /* IP address of mcast router */
	igs_osl_timer_t      *rtlist_timer;         /* Timer for router entry */
	igsc_info_t      *igsc_info;            /* IGSC Instance data */
} igsc_rtlist_t;

typedef struct mc_grp_spl
{
	uint32	addr;
	uint32	mask;
} mc_grp_spl_t;

#define IGMPV3_HOST_MEMBERSHIP_REPORT	0x22	/* V3 version of 0x11 */

#define IGMPV3_MODE_IS_INCLUDE		1
#define IGMPV3_MODE_IS_EXCLUDE		2
#define IGMPV3_CHANGE_TO_INCLUDE	3
#define IGMPV3_CHANGE_TO_EXCLUDE	4
#define IGMPV3_ALLOW_NEW_SOURCES	5
#define IGMPV3_BLOCK_OLD_SOURCES	6

typedef struct igmpv3_report {
	uint8 type;
	uint8 reserved1;
	uint16 checksum;
	uint16 reserved2;
	uint16 group_num;
} igmpv3_report_t;

typedef struct igmpv3_group {
	uint8 type;
	uint8 aux_len;
	uint16 src_num;
	uint32 mcast_addr;
} igmpv3_group_t;

#define IGMPV3_SRC_ADDR_LEN	4

#ifdef BCM_NBUFF_WLMCAST_IPV6
void *igsc_init_ipv6(igsc_info_t *igs_info, osl_t *osh);
void igsc_exit_ipv6(igsc_info_t *igsc_info);
#endif

#endif /* _IGSC_H_ */
