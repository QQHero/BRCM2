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
 * $Id: igs_export.h 803553 2021-09-29 11:42:23Z $
 */

#ifndef _IGS_EXPORT_H_
#define _IGS_EXPORT_H_

struct igs_info;

/*
 * Description: This function is called by IGS Common code when it wants
 *              to send a multicast packet on to all the LAN ports. It
 *              allocates the native OS packet buffer, adds mac header and
 *              forwards a copy of frame on to LAN ports.
 *
 * Input:       igs_info - IGS instance information.
 *              ip       - Pointer to the buffer containing the frame to
 *                         send. Frame starts with IP header.
 *              length   - Length of the buffer.
 *              mgrp_ip  - Multicast destination address.
 *
 * Return:      SUCCESS or FAILURE
 */
extern int32 igs_broadcast(struct igs_info *igs_info, uint8 *ip, uint32 length, uint32 mgrp_ip);

#endif /* _IGS_EXPORT_H_ */
