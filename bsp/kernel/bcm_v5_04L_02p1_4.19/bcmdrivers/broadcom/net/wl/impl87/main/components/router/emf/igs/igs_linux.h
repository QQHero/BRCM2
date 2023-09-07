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
 * $Id: igs_linux.h 803553 2021-09-29 11:42:23Z $
 */

#ifndef _IGS_LINUX_H_
#define _IGS_LINUX_H_

#define IGS_MAX_INST  16

typedef struct igs_info
{
	struct igs_info    *next;          /* Next pointer */
	int8               inst_id[IFNAMSIZ]; /* IGS instance identifier */
	osl_t              *osh;           /* OS layer handle */
	void               *igsc_info;     /* IGSC Global data handle */
	struct net_device  *br_dev;        /* Bridge device for the instance */
} igs_info_t;

typedef struct igs_struct
{
	struct sock        *nl_sk;         /* Netlink socket */
	igs_info_t         *list_head;     /* IGS instance list head */
	osl_lock_t         lock;           /* IGS locking */
	int32              inst_count;     /* IGS instance count */
} igs_struct_t;

#if (defined(BCM_NBUFF_WLMCAST_IPV6) && defined(BCM_WMF_MCAST_DBG))
igs_info_t *igs_instance_add(int8 *inst_id, struct net_device *br_ptr, void *igsc_info);
int32 igs_instance_del(igs_info_t *igs_info, void *igsc);
igs_info_t * igs_instance_find(int8 *inst_id);
#endif
#endif /* _IGS_LINUX_H_ */
