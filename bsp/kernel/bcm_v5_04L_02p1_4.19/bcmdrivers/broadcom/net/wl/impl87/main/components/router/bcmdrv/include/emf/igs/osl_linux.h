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
 * $Id: osl_linux.h 679290 2017-01-13 07:39:40Z $
 */

#ifndef _OSL_LINUX_H_
#define _OSL_LINUX_H_

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>

struct osl_lock
{
	spinlock_t slock;       /* Spin lock */
	uint8      name[16];    /* Name of the lock */
};

typedef struct osl_lock *osl_lock_t;

static inline osl_lock_t
OSL_LOCK_CREATE(uint8 *name)
{
	osl_lock_t lock;

	lock = MALLOC(NULL, sizeof(struct osl_lock));

	if (lock == NULL)
	{
		printf("Memory alloc for lock object failed\n");
		return (NULL);
	}

	strncpy(lock->name, name, sizeof(lock->name)-1);
	lock->name[ sizeof(lock->name)-1 ] = '\0';
	spin_lock_init(&lock->slock);

	return (lock);
}

static inline void
OSL_LOCK_DESTROY(osl_lock_t lock)
{
	MFREE(NULL, lock, sizeof(struct osl_lock));
	return;
}

#define OSL_LOCK(lock)          spin_lock_bh(&((lock)->slock))
#define OSL_UNLOCK(lock)        spin_unlock_bh(&((lock)->slock))

#define DEV_IFNAME(dev)         (((struct net_device *)dev)->name)

typedef struct igs_osl_timer {
	struct timer_list timer;
	void   (*fn)(void *);
	void   *arg;
	uint   ms;
	bool   periodic;
	bool   set;
#ifdef BCMDBG
	char    *name;          /* Desription of the timer */
#endif
} igs_osl_timer_t;

extern igs_osl_timer_t *igs_osl_timer_init(const char *name, void (*fn)(void *arg), void *arg);
extern void igs_osl_timer_add(igs_osl_timer_t *t, uint32 ms, bool periodic);
extern void igs_osl_timer_update(igs_osl_timer_t *t, uint32 ms, bool periodic);
extern bool igs_osl_timer_del(igs_osl_timer_t *t);
extern osl_lock_t OSL_LOCK_CREATE(uint8 *name);
extern void OSL_LOCK_DESTROY(osl_lock_t lock);
#endif /* _OSL_LINUX_H_ */
