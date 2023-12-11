#ifndef __BPM_TRACK_H_INCLUDED__
#define __BPM_TRACK_H_INCLUDED__

/*
<:copyright-BRCM:2020:proprietary:standard

   Copyright (c) 2020 Broadcom
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
	constitutes the valuable trade secrets of Broadcom, and you shall use
	all reasonable efforts to protect the confidentiality thereof, and to
	use this information only in connection with your use of Broadcom
	integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
	AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
	WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
	RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
	ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
	FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
	COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
	TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
	PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
	ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
	INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
	WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
	IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
	OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
	SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
	SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
	LIMITED REMEDY.
:>
*/

/*
 *******************************************************************************
 * File Name  : bpm_track.h
 *
 *******************************************************************************
 */

/* Common data structures and APIs for BPM Tracking */
struct bpm_track_data {
    void **sbuf_pool;
    void **tbuf_pool;
    uint32_t tbuf_pool_ix;
    uint32_t tbuf_pool_mem;
    uint32_t marked_cnt;
};

typedef struct bpm_track_data bpm_track_data_t;

int __init bpm_track_init(void);
int bpm_track_init_static_pool(int max, int top, void **pool_ptr);
int bpm_track_enable(uint32_t trail_len);
int bpm_track_disable(void);
int bpm_track_ioctl(bpmctl_track_t *trk_p);

/* BPM Common APIs from different implementations */
/* Given a buffer pointer, return the pointer to the very beginning of
 * this buffer. return NULL, if given buffer pointer does not belong
 * to the pool. */
void *bpm_get_buffer_base(void *buf_p);

/* Return the last index of the current active pool. */
int bpm_get_last_idx(void);

#endif /*  __BPM_TRACK_H_INCLUDED__ */

