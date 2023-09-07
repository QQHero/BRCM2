/*
 * Network configuration layer (Linux)
 *
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
 * $Id: ipt.h $
 *
 * This header file is the part of netconf utility which will contain all
 * corresponding kernel interface structures and header files which are
 * necessary for netconf utility.
 */
#ifndef _IPT_H_
#define _IPT_H_

#include <linux/netfilter/xt_mac.h>
#include <linux/netfilter/xt_state.h>
#include <linux/netfilter/xt_TCPMSS.h>
#include <linux/netfilter/xt_time.h>
#include <linux/netfilter_ipv4/ipt_LOG.h>

#define ipt_tcpmss_info		xt_tcpmss_info
#define IPT_TCPMSS_CLAMP_PMTU	XT_TCPMSS_CLAMP_PMTU

#define IPT_STATE_BIT		XT_STATE_BIT
#define IPT_STATE_INVALID	XT_STATE_INVALID

#define IPT_STATE_UNTRACKED	XT_STATE_UNTRACKED

#define ipt_state_info		xt_state_info

#define ipt_mac_info xt_mac_info

/* ip_autofw releated stuff */
#define AUTOFW_MASTER_TIMEOUT 600	/* 600 secs */

struct ip_autofw_info {
	u_int16_t proto;	/* Related protocol */
	u_int16_t dport[2];	/* Related destination port range */
	u_int16_t to[2];	/* Port range to map related destination port range to */
};

struct ip_autofw_expect {
	u_int16_t dport[2];	/* Related destination port range */
	u_int16_t to[2];	/* Port range to map related destination port range to */
};

#define IP_NAT_RANGE_MAP_IPS NF_NAT_RANGE_MAP_IPS
#define IP_NAT_RANGE_PROTO_SPECIFIED NF_NAT_RANGE_PROTO_SPECIFIED
#define IP_NAT_RANGE_PROTO_RANDOM NF_NAT_RANGE_PROTO_RANDOM
#define IP_NAT_RANGE_PERSISTENT NF_NAT_RANGE_PERSISTENT

#define nf_nat_range nf_nat_ipv4_range
#define nf_nat_multi_range nf_nat_ipv4_multi_range_compat

#endif /* _IPT_H_ */
