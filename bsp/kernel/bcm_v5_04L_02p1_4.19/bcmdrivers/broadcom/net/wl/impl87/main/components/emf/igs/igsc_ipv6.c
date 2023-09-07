/*
 * IGMP Snooping Layer: IGMP Snooping module runs at layer 2. IGMP
 * Snooping layer uses the multicast information in the IGMP messages
 * exchanged between the participating hosts and multicast routers to
 * update the multicast forwarding database. This file contains the
 * common code routines of IGS module.
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
 * $Id: igsc_ipv6.c 768025 2018-10-03 06:47:52Z $
 */
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmendian.h>
#include <bcmip.h>
#include <bcmipv6.h>
#include <osl.h>
#include <bcmnvram.h>
#include <clist.h>
#if defined(linux)
#include <osl_linux.h>
#else /* defined(osl_xx) */
#error "Unsupported osl"
#endif /* defined(osl_xx) */
#include "igs_cfg.h"
#include "emfc_export.h"
#include "igs_export.h"
#include "igsc_export.h"
#include "igsc.h"
#include "igs_linux.h"
#include "igsc_sdb.h"
#include <bcm_mcast.h>
extern struct notifier_block mcast_snooping_notifier;
extern void *getprivInf(char *name, int port_no);
extern void *emfc_wmf_get_igsc(int ifindex);
extern void *emfc_wmf_scbfind(int ifindex, unsigned char *mac);

extern int igsc_mcast_snooping_event(unsigned long event, void *ptr);

int mcast_snooping_event(struct notifier_block *unused, unsigned long event, void *ptr)
{
	return igsc_mcast_snooping_event(event, ptr);
}
struct notifier_block mcast_snooping_notifier = {
	.notifier_call = mcast_snooping_event
};

int igsc_mcast_snooping_event(unsigned long event, void *ptr)
{
	t_BCM_MCAST_NOTIFY *notify = (t_BCM_MCAST_NOTIFY *)ptr;
	igsc_info_t *igsc_inf;
	void *scb = NULL;
	igsc_inf = emfc_wmf_get_igsc(notify->ifindex);
	if (igsc_inf && notify->proto == BCM_MCAST_PROTO_IPV6 &&
			(scb = emfc_wmf_scbfind(notify->ifindex, notify->repMac))) {
		switch (event) {
			case BCM_MCAST_EVT_SNOOP_ADD:
				if (igsc_sdb_member_add_ipv6(igsc_inf, scb,
						*((struct ipv6_addr *)&notify->ipv6grp),
						*((struct ipv6_addr *)&notify->ipv6rep)))
					printk("Failed add entry %pI6 \r\n", &notify->ipv6rep);
				break;
			case BCM_MCAST_EVT_SNOOP_DEL:
				if (igsc_sdb_member_del_ipv6(igsc_inf, scb,
						*((struct ipv6_addr *)&notify->ipv6grp),
						*((struct ipv6_addr *)&notify->ipv6rep)))
					printk("Failed delete entry %pI6 \r\n", &notify->ipv6rep);
				break;
		}
		IGS_IGSDB("From station: %pI6\n", notify->ipv6rep.s6_addr32);
		IGS_IGSDB("Rep Mac:0x:%pM\n", notify->repMac);
	}
	return 0;
}

uint8 igsc_instance_count = 0;

void *
igsc_init_ipv6(igsc_info_t *igsc_info, osl_t *osh)
{

	igsc_info->sdb_lock_ipv6 = OSL_LOCK_CREATE("SDB6 Lock");
	if (igsc_info->sdb_lock_ipv6 == NULL)
	{
		igsc_sdb_clear_ipv6(igsc_info);
		MFREE(osh, igsc_info, sizeof(igsc_info_t));
		return (NULL);
	}
	if (++igsc_instance_count == 1)
		bcm_mcast_notify_register(&mcast_snooping_notifier);
	IGS_IGSDB("Initialized IGSDB\n");

	return (igsc_info);
}

void
igsc_exit_ipv6(igsc_info_t *igsc_info)
{
	if (--igsc_instance_count == 0)
		bcm_mcast_notify_unregister(&mcast_snooping_notifier);
	igsc_sdb_clear_ipv6(igsc_info);
	OSL_LOCK_DESTROY(igsc_info->sdb_lock_ipv6);
return;
}
