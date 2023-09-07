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
 * $Id: clist.h 523133 2014-12-27 05:50:30Z $
 */

#ifndef _CLIST_H_
#define _CLIST_H_

typedef struct clist_head
{
	struct clist_head *next, *prev;
} clist_head_t;

#define CLIST_DECL_INIT(head) clist_head_t head = { &(head), &(head) }

static inline void
clist_init_head(clist_head_t *head)
{
	head->next = head->prev = head;
}

static inline void
clist_add_head(clist_head_t *head, clist_head_t *item)
{
	head->next->prev = item;
	item->next = head->next;
	item->prev = head;
	head->next = item;

	return;
}

static inline void
clist_add_tail(clist_head_t *head, clist_head_t *item)
{
	item->next = head;
	item->prev = head->prev;
	head->prev->next = item;
	head->prev = item;

	return;
}

static inline void
clist_delete(clist_head_t *item)
{
	item->prev->next = item->next;
	item->next->prev = item->prev;

	return;
}

static inline void
clist_walk(clist_head_t *head, void (*fn)(clist_head_t *, void *), void *arg)
{
	clist_head_t *ptr;

	ptr = head;

	while (ptr->next != head)
	{
		fn(ptr, arg);
		ptr = ptr->next;
	}

	return;
}

#define clist_empty(h) ((h)->next == (h))

#define clist_entry(p, type, member) \
	    ((type *)((char *)(p)-(unsigned long)(&((type *)0)->member)))

#endif /* _CLIST_H_ */
