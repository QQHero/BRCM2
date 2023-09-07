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
 * $Id: emf_export.h 803553 2021-09-29 11:42:23Z $
 */

#ifndef _EMF_EXPORT_H_
#define _EMF_EXPORT_H_

struct emf_info;

/*
 * Description: This function allows EMFL to start receiving frames for
 *              its processing and/or forwarding. It registers OS specific
 *              hook functions.  These functions are called from the
 *              bridge or ip forwarding path when packets reach respective
 *              bridge pre or ip post hook points. This function can be
 *              called when multicast forwarding is enabled by user.
 *
 * Input:       emfi - EMFL instance handle.
 *
 * Return:      Registration status, SUCCESS or FAILURE.
 */
extern int32 emf_hooks_register(struct emf_info *emfi);

/*
 * Description: This function causes EMFL to stop receiving frames. It
 *              unregisters the OS specific hook functions. This function
 *              can be called when multicast forwarding is disabled by
 *              user.
 *
 * Input:       emfi - Global EMF instance handle.
 */
extern void emf_hooks_unregister(struct emf_info *emfi);

/*
 * Description: This function is called by EMFL common code when it wants
 *              to forward the packet on to a specific port. It adds the
 *              MAC header to the frame if not present and queues the
 *              frame to the destination interface indicated by the input
 *              paramter (txif).
 *
 * Input:       emfi    - EMFL instance handle
 *              sdu     - Pointer to the packet buffer.
 *              mgrp_ip - Multicast destination address.
 *              txif    - Interface to send the frame on.
 *              rt_port - TRUE when the packet is received from IP stack
 *                        (router port), FALSE otherwise.
 *
 * Return:      SUCCESS or FAILURE.
 */
extern int32 emf_forward(struct emf_info *emfi, void *sdu, uint32 mgrp_ip,
                         void *txif, int rt_port);

/*
 * Description: This function is called to send the packet buffer up
 *              to the IP stack.
 *
 * Input:       emfi - EMF instance information
 *              sdu  - Pointer to the packet buffer.
 */
extern void emf_sendup(struct emf_info *emfi, void *sdu);

#endif /* _EMF_EXPORT_H_ */
