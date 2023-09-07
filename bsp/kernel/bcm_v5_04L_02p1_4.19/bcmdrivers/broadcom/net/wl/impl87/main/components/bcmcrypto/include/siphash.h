/*
 * SipHash reference C implementation
 *
 * Copyright 2022 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifndef _siphash_h_
#define _siphash_h_

#include <typedefs.h>

#define HALFSIPHASH
// #define OUTLEN8

void sip_hash_key_init(uint32 *siphashkey);

/*
 * Arguments to the siphash function are:
 * in: input data to be hashed
 * inlen: length of input data
 * k: pointer to 128-bit key to be used by hash algorithm
 * out: where to put the hash
 * outlen: length of hash in bytes - must be 8 or 16
 */
int siphash(const uint8 *in, const uint32 inlen, const uint8 *k, uint8 *out, const uint32 outlen);

/*
 * Optimised version for mem_t and mem_use_t headers, which are fixed size and 32-bit aligned
 * Arguments to the siphash function are:
 * in: input data to be hashed - must be 32-bit aligned
 * inlen: length of input data - must be 8 or 12
 * k: key to be used by hash algorithm
 * out: where to put the hash - assumed to be 8 bytes
 */
int siphash_memhdr_opt(const uint8 *in, const uint32 inlen, const uint64 *k, uint64 *out);

/*
 * Arguments to the hsiphash function are:
 * in: input data to be hashed
 * inlen: length of input data
 * k: pointer to 64-bit key to be used by hash algorithm
 * out: where to put the hash
 * outlen: length of hash in bytes - must be 4 or 8
 */
int halfsiphash(const uint8 *in, const uint32 inlen, const uint32 *k, uint8 *out,
	const uint32 outlen);

/* HalfSipHash implementation optimised for heap headers */

/*
 * We can gain cycles and space by making the following assumptions:
 * - the key is correctly aligned
 * - the input data is correctly aligned
 * - the input data is either 8 or 12 bytes long
 * - the required output size is 4 bytes (although we can compile for 8)
 *
 * Execution time is reduced by TBA%.
 * Size is reduced by TBA%.
 */
int hsiphash_memhdr_opt(const uint8 *in, const uint32 inlen, const uint32 *k, uint32 *out);

#endif /* _siphash_h_ */
