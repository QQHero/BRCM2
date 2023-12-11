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
 * $Id: igsc_sdb.h 679290 2017-01-13 07:39:40Z $
 */

#ifndef _IGSC_SDB_H_
#define _IGSC_SDB_H_
#if defined(BCM_NBUFF_WLMCAST_IPV6)
#include <bcmipv6.h>
#include <typedefs.h>
#define IGSDB_MGRP_HASH_IPV6(m) ((((m).s6_addr32[0])+((m).s6_addr32[1])+((m).s6_addr32[2])+\
			((m).s6_addr32[3])) &7)
#endif
#define IGSDB_MGRP_HASH(m)     ((((m) >> 24) + ((m) >> 16) + \
				 ((m) >> 8) + ((m) & 0xff)) & 7)

/*
 * Group entry of IGSDB
 */
typedef struct igsc_mgrp
{
	clist_head_t   mgrp_hlist;   /* Multicast Groups hash list */
#if defined(BCM_NBUFF_WLMCAST_IPV6)
	struct ipv6_addr mgrp_ipv6;   /* Multicast Group IPv6 address */
#endif
	uint32         mgrp_ip;      /* Multicast Group IP Address */
	clist_head_t   mh_head;      /* List head of group members */
	clist_head_t   mi_head;      /* List head of interfaces */
	igsc_info_t    *igsc_info;   /* IGSC instance data */
} igsc_mgrp_t;

/*
 * Interface entry of IGSDB
 */
typedef struct igsc_mi
{
	clist_head_t   mi_list;      /* Multicast i/f list prev and next */
	void           *mi_ifp;      /* Interface pointer */
	int32          mi_ref;       /* Ref count of hosts on the i/f */
} igsc_mi_t;

/*
 * Host entry of IGSDB
 */
typedef struct igsc_mh
{
	clist_head_t   mh_list;      /* Group members list prev and next */
#if defined(BCM_NBUFF_WLMCAST_IPV6)
	struct ipv6_addr mh_ipv6;
#endif

	uint32         mh_ip;        /* Unicast IP address of host */
	igsc_mgrp_t    *mh_mgrp;     /* Multicast forwarding entry for the
	                              * group
				      */
	igs_osl_timer_t    *mgrp_timer;  /* Group Membership Interval timer */
	igsc_mi_t      *mh_mi;       /* Interface connected to host */
	int		missed_report_cnt;	/* No of membership report it missied */
} igsc_mh_t;

/*
 * Prototypes
 */
int32 igsc_sdb_member_add(igsc_info_t *igsc_info, void *ifp, uint32 mgrp_ip,
                          uint32 mh_ip);
int32 igsc_sdb_member_del(igsc_info_t *igsc_info, void *ifp, uint32 mgrp_ip,
                          uint32 mh_ip);
void igsc_sdb_init(igsc_info_t *igsc_info);
#ifdef BCM_NBUFF_WLMCAST
int32 igsc_sdb_sta_del(igsc_info_t *igsc_info, void *ifp, uint32 mh_ip);
#endif /* BCM_NBUFF_WLMCAST */
#if defined(BCM_NBUFF_WLMCAST_IPV6)
void igsc_sdb_clear_ipv6(igsc_info_t *igsc_info);
int32 igsc_sdb_clear_group_ipv6(igsc_info_t *igsc_info, struct ipv6_addr *grp);
int32 igsc_sdb_member_add_ipv6(igsc_info_t *igsc_info, void *ifp,
		struct ipv6_addr mgrp_ip, struct ipv6_addr mh_ip);
int32 igsc_sdb_member_del_ipv6(igsc_info_t *igsc_info, void *ifp,
		struct ipv6_addr mgrp_ip, struct ipv6_addr mh_ip);
int32 igsc_sdb_interface_del_ipv6(igsc_info_t *igsc_info, void *ifp);
#endif
#endif /* _IGSC_SDB_H_ */
