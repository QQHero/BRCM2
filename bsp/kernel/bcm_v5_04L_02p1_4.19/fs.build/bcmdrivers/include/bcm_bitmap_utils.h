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
 * File Name  : bcm_bitmap_utils.h
 * This file provides interface for bitmap utitilies.
 *******************************************************************************
 */

#ifndef _BCM_BITMAP_UTILS_H
#define _BCM_BITMAP_UTILS_H

/*
 *------------------------------------------------------------------------------
 *
 * Function     : bcm_bitmap_xxxxx() APIs
 * Description  :
 *
 * A bitmap[n] is made of n 32-bit words, where n >=1.
 * bitmap[0] bits 31:00
 * bitmap[1] bits 63:32
 * ...
 *
 * Init: The bitmap[n] array is initialized as zeros, and means all the bit indices are free.
 *
 * The least-significant bit is position 0, the most-significant is 31
 *
 *  E.g.                         Higher order   -------  Lower order
 *  for a bitmap 0x0F0FF080 = 0b 00001111 00001111 11110000 10000000
 *                               ^                          ^      ^
 *                              31                          7      0
 *       7 = bcm_bitmap_get/find_next_one(0x0F0FF080, 1, 0)
 *       7 = bcm_bitmap_get/find_next_one(0x0F0FF080, 1, 7)
 *      12 = bcm_bitmap_get/find_next_one(0x0F0FF080, 1, 8)
 *       0 = bcm_bitmap_get/find_next_zero(0x0F0FF080, 1, 0)
 *       8 = bcm_bitmap_get/find_next_zero(0x0F0FF080, 1, 7)
 *
 *     bcm_bitmap_set/clear_bit(): Sets/clears the given bit position in the bitmap.
 *
 *     bcm_bitmap_get_bit_index(): Allocates the next free bit index 
 *          starting at index=0 from the bitmap.
 * 
 *     bcm_bitmap_return_bit_index(): Frees the bit index into the bitmap.
 * 
 *     bcm_bitmap_find_next_zero/one(): finds the next matching bit position in bitmap,
 *          starting from the given bit position (does not wrap around)
 * 
 *     bcm_bitmap_get_next_zero/one(): finds and inverts the next matching bit 
 *          position in bitmap starting from the given bit position. (does not wrap around)
 *
 *     bcm_bitmap_get_next_bit_index(): Allocates the next free bit index 
 *          starting at given index from the bitmap. (does not wrap around)
 *------------------------------------------------------------------------------
 */

/* defines */

#define BCM_BYTES_PER_WORD  (sizeof(uint32_t))
#define BCM_BITS_PER_WORD   (BCM_BYTES_PER_WORD * 8)
#define BCM_GET_MASK(b)	    (1ULL<<(b))

int bcm_bitmap_set_bit(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_num);
int bcm_bitmap_clear_bit(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_num);
int bcm_bitmap_is_bit_set(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_num);
int bcm_bitmap_get_bit_index(uint32_t *bitmap_p, uint32_t num_words);
int bcm_bitmap_get_next_bit_index(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_idx);
int bcm_bitmap_return_bit_index(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_idx);
int bcm_bitmap_find_next_one(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_idx);
int bcm_bitmap_get_next_one(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_idx);
int bcm_bitmap_find_next_zero(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_idx);
int bcm_bitmap_get_next_zero(uint32_t *bitmap_p, uint32_t num_words, uint32_t bit_idx);
void bcm_bitmap_dump(uint32_t *bitmap_p, uint32_t num_words);

#endif /* _BCM_BITMAP_UTILS_H */
