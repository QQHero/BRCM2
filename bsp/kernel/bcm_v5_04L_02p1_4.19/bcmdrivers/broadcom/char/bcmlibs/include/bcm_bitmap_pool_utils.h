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
 * File Name  : bcm_bitmap_pool_utils.h
 * This file provides interface for bitmap pool utitilies.
 *******************************************************************************
 */

#ifndef _BCM_BITMAP_POOL_UTILS_H
#define _BCM_BITMAP_POOL_UTILS_H

/* Interface functions */

void *bcm_bitmap_pool_init(uint32_t num_bitmaps, uint32_t bitmap_size);
int bcm_bitmap_pool_alloc_bitmap(void *p);
int bcm_bitmap_pool_free_bitmap(void *p, uint16_t bitmap_idx);
int bcm_bitmap_pool_alloc_bitmap_next(void *p, uint16_t start_idx);
uint32_t* bcm_bitmap_pool_get_bitmap_ptr(void *p, uint32_t bitmap_idx);
int bcm_bitmap_pool_copy_bitmap(void *p, uint32_t bitmap_idx, uint32_t *dst_p, 
                                uint32_t dst_size_words);
void bcm_bitmap_pool_dump(void *p, int num, int bitmap_idx, int in_use_only);
void bcm_bitmap_pool_dump_one(void *p, int bitmap_idx);

/* Wrapper functions around bcm_bitmap_utils */
int bcm_bitmap_pool_set_bit(void *p, uint32_t bitmap_idx, uint32_t bit);
int bcm_bitmap_pool_clear_bit(void *p, uint32_t bitmap_idx, uint32_t bit);
int bcm_bitmap_pool_return_bit_index(void *p, uint32_t bitmap_idx, uint32_t bit);
int bcm_bitmap_pool_get_next_bit_index(void *p, uint32_t bitmap_idx, uint32_t bit);
#endif /* _BCM_BITMAP_POOL_UTILS_H */
