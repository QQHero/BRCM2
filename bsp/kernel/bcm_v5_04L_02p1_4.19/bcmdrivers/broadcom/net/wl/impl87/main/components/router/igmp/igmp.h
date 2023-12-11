/*
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
 * $Id: igmp.h 710512 2017-07-13 03:54:07Z $
 */

#define	IGMP_FMODE_INCLUDE	1
#define	IGMP_FMODE_EXCLUDE	0

#define	IGMP_TIMER_SCALE	10
#define IGMP_DEF_QI	        125
#define IGMP_DEF_QRI		10
#define IGMP_DEF_RV	       	2
#define IGMP_OQPI		((IGMP_DEF_RV * IGMP_DEF_QI) + IGMP_DEF_QRI/2)
#define IGMP_GMI		((IGMP_DEF_RV * IGMP_DEF_QI) + IGMP_DEF_QRI)

// eddie added
#define IGMP_V2_MEMBERSHIP_LEAVE   0x17
#define IGMP_MEMBERSHIP_QUERY	   0x11
#define IGMP_V1_MEMBERSHIP_REPORT   0x12
#define IGMP_V2_MEMBERSHIP_REPORT   0x16
#define IGMP_V3_MEMBERSHIP_REPORT   0x22

#define IGMP_VERSION_1		0x12
#define IGMP_VERSION_2		0x16
#define IGMP_VERSION_3		0x22

#define	IGMP_MINLEN	       	8

#define	IGMP_ALL_ROUTERS	"224.0.0.2"
#define	IGMP_ALL_ROUTERS_V3	"224.0.0.22"

#define IGMP_SRSP(x)		((x)->igmpq_misc & (0x08))

/*
 * IGMP source filter type
 */
typedef struct _igmp_src_t {
	struct in_addr  igmps_source;
	int             igmps_timer;
	/*
	 * add a flag to indicate the forwarding state
	 */
	int             igmps_fstate;
	struct igmp_src_t *igmps_next;
} igmp_src_t;

/*
 * IGMP member type
 */
typedef struct _igmp_rep_t {
	struct in_addr  igmpr_addr;
	struct igmp_rep_t *igmpr_next;
} igmp_rep_t;

/*
 * IGMP group type
 */
typedef struct _igmp_group_t {
	struct in_addr  igmpg_addr; /* group address in network order */
	int             igmpg_timer;
	int             igmpg_fmode;
	int             igmpg_version;
#ifdef HND_FIX
	struct in_addr  igmpg_source;
#endif				/* HND_FIX */
	igmp_rep_t     *igmpg_members;
	igmp_src_t     *igmpg_sources;
	struct _igmp_group_t *igmpg_next;
} igmp_group_t;

/*
 * Scheduled group specifiq query type
 */
typedef struct _sch_query_t {
	struct in_addr  gp_addr;
	int             igmp_retnum;
	int             numsrc;
	struct in_addr  sources[1];
	struct _sch_query_t *sch_next;

} sch_query_t;
/*
 * IGMP V3 query format
 */
typedef struct _igmpv3q_t {
	u_char          igmpq_type;	/* version & type of IGMP message */
	u_char          igmpq_code;	/* subtype for routing msgs */
	u_short         igmpq_cksum;	/* IP-style checksum */
	struct in_addr  igmpq_group;	/* group address being reported */
	u_char          igmpq_misc;	/* reserved, supress and robustness */
	u_char          igmpq_qqi;	/* querier's query interval */
	u_short         igmpq_numsrc;	/* number of sources */
	struct in_addr  igmpq_sources[1];	/* source addresses */
} igmpv3q_t;

/*
 * IGMP V2 query format
 */
typedef struct _igmpv2q_t {
	u_char          igmpq_type;	/* version & type of IGMP message */
	u_char          igmpq_code;	/* subtype for routing msgs */
	u_short         igmpq_cksum;	/* IP-style checksum */
	struct in_addr  igmpq_group;	/* group address being reported */
} igmpv2q_t;
/*
 * IGMPv1/IGMPv2 report format
 */
typedef struct _igmpr_t {
	u_char          igmpr_type;	/* version & type */
	u_char          igmpr_code;	/* unused,max resp time for V2 */
	u_short         igmpr_cksum;	/* IP-style checksum for IP
					 * payload */
	struct in_addr  igmpr_group;	/* group address being reported */
} igmpr_t;

/*
 * IGMPv3 group record format
 */
typedef struct _igmp_grouprec_t {
	u_char          igmpg_type;	/* record type */
	u_char          igmpg_datalen;	/* amount of aux data */
	u_short         igmpg_numsrc;	/* number of sources */
	struct in_addr  igmpg_group;	/* the group being reported */
	struct in_addr  igmpg_sources[1];	/* source addresses */
} igmp_grouprec_t;

/*
 * IGMPv3 report format
 */
typedef struct _igmp_report_t {
	u_char          igmpr_type;	/* version & type of IGMP message */
	u_char          igmpr_code;	/* subtype for routing msgs */
	u_short         igmpr_cksum;	/* IP-style checksum */
	u_short         igmpr_rsv;	/* reserved */
	u_short         igmpr_numgrps;	/* number of groups */
	igmp_grouprec_t igmpr_group[1];	/* group records */
} igmp_report_t;

extern unsigned int igmp_debug_level;

#define IGMP_DBG_ERR           0x1
#define IGMP_DBG_DBG           0x2
#define IGMP_DBG_PRINT_TABLE   0x4
#define IGMP_DBG(fmt, arg...) \
     do { if (igmp_debug_level & IGMP_DBG_DBG) \
         printf("IGMP >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); } while (0)

#define IGMP_ERR(fmt, arg...) \
     do { if (igmp_debug_level & IGMP_DBG_ERR) \
         printf("IGMP >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); } while (0)

/* FILE-CSTYLED */
