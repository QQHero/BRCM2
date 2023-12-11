/*
 * Timer functions used by EMFL. These Functions can be moved to
 * shared/linux_osl.c, include/linux_osl.h
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
 * $Id: osl_linux.c 803551 2021-09-29 11:01:55Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include "osl_linux.h"

static void
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
igs_osl_timer(struct timer_list *data)
#else
igs_osl_timer(ulong data)
#endif
{
	igs_osl_timer_t *t;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	t = from_timer(t, data, timer);
#else
	t = (igs_osl_timer_t *)data;
#endif

	ASSERT(t->set);

	if (t->periodic) {
#if defined(BCMJTAG) || defined(BCMSLTGT)
		t->timer.expires = jiffies + t->ms*HZ/1000*htclkratio;
#else
		t->timer.expires = jiffies + t->ms*HZ/1000;
#endif /* defined(BCMJTAG) || defined(BCMSLTGT) */
		add_timer(&t->timer);
		t->set = TRUE;
		t->fn(t->arg);
	} else {
		t->set = FALSE;
		t->fn(t->arg);
#ifdef BCMDBG
		if (t->name) {
			MFREE(NULL, t->name, strlen(t->name) + 1);
		}
#endif
		MFREE(NULL, t, sizeof(igs_osl_timer_t));
	}

	return;
}

igs_osl_timer_t *
igs_osl_timer_init(const char *name, void (*fn)(void *arg), void *arg)
{
	igs_osl_timer_t *t;

	if ((t = MALLOC(NULL, sizeof(igs_osl_timer_t))) == NULL) {
		printk(KERN_ERR "igs_osl_timer_init: out of memory, malloced %zu bytes\n",
		       sizeof(igs_osl_timer_t));
		return (NULL);
	}

	bzero(t, sizeof(igs_osl_timer_t));

	t->fn = fn;
	t->arg = arg;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	t->timer.data = (ulong)t;
#endif
	t->timer.function = igs_osl_timer;
#ifdef BCMDBG
	if ((t->name = MALLOC(NULL, strlen(name) + 1)) != NULL) {
		strcpy(t->name, name);
	}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	timer_setup(&t->timer, igs_osl_timer, 0);
#else
	init_timer(&t->timer);
#endif

	return (t);
}

void
igs_osl_timer_add(igs_osl_timer_t *t, uint32 ms, bool periodic)
{
	ASSERT(!t->set);

	t->set = TRUE;
	t->ms = ms;
	t->periodic = periodic;
#if defined(BCMJTAG) || defined(BCMSLTGT)
	t->timer.expires = jiffies + ms*HZ/1000*htclkratio;
#else
	t->timer.expires = jiffies + ms*HZ/1000;
#endif /* defined(BCMJTAG) || defined(BCMSLTGT) */

	add_timer(&t->timer);

	return;
}

void
igs_osl_timer_update(igs_osl_timer_t *t, uint32 ms, bool periodic)
{
	ASSERT(t->set);

	t->ms = ms;
	t->periodic = periodic;
	t->set = TRUE;
#if defined(BCMJTAG) || defined(BCMSLTGT)
	t->timer.expires = jiffies + ms*HZ/1000*htclkratio;
#else
	t->timer.expires = jiffies + ms*HZ/1000;
#endif /* defined(BCMJTAG) || defined(BCMSLTGT) */

	mod_timer(&t->timer, t->timer.expires);

	return;
}

/*
 * Return TRUE if timer successfully deleted, FALSE if still pending
 */
bool
igs_osl_timer_del(igs_osl_timer_t *t)
{
	if (t->set) {
		t->set = FALSE;
		if (!del_timer(&t->timer)) {
			printk(KERN_INFO "igs_osl_timer_del: Failed to delete timer\n");
			return (FALSE);
		}
#ifdef BCMDBG
		if (t->name) {
			MFREE(NULL, t->name, strlen(t->name) + 1);
		}
#endif
		MFREE(NULL, t, sizeof(igs_osl_timer_t));
	}

	return (TRUE);
}
