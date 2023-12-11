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

/*
 *  SipHash reference C implementation
 *
 *  Copyright (c) 2012-2016 Jean-Philippe Aumasson
 *  jeanphilippe.aumasson@gmail.com>
 *  Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
 *
 *  To the extent possible under law, the author(s) have dedicated all copyright
 *  and related and neighboring rights to this software to the public domain
 *  worldwide. This software is distributed without any warranty.
 *
 *  You should have received a copy of the CC0 Public Domain Dedication along
 *  with
 *  this software. If not, see
 *  http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include <typedefs.h>
#include <bcmendian.h>
#include <osl.h>
#include <siphash.h>

typedef struct vee_vars {
	uint64 v0;
	uint64 v1;
	uint64 v2;
	uint64 v3;
} sip_var_t;

/* default: SipHash-2-4 */
#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x, b) (uint32)(((x) << (b)) | ((x) >> (32u - (b))))

#define ROTL64(x, b) (uint64)(((x) << (b)) | ((x) >> (64u - (b))))

#ifdef DEBUG
#define TRACE									\
	do {										\
		printf("(%3d) v0 %08x %08x\n", (int)inlen, (uint32)(v0 >> 32),	\
			   (uint32)v0);						\
		printf("(%3d) v1 %08x %08x\n", (int)inlen, (uint32)(v1 >> 32),	\
			   (uint32)v1);						\
		printf("(%3d) v2 %08x %08x\n", (int)inlen, (uint32)(v2 >> 32),	\
			   (uint32)v2);						\
		printf("(%3d) v3 %08x %08x\n", (int)inlen, (uint32)(v3 >> 32),	\
			   (uint32)v3);						\
	} while (0)
#else
#define TRACE
#endif

void
sip_hash_key_init(uint32 *siphashkey)
{
	siphashkey[0] = OSL_RAND();
	siphashkey[1] = OSL_RAND();
	siphashkey[2] = OSL_RAND();
	siphashkey[3] = OSL_RAND();
}

static void sipround(sip_var_t *sipvars)
{
	sipvars->v0 += sipvars->v1;
	sipvars->v1 = ROTL64(sipvars->v1, 13);
	sipvars->v1 ^= sipvars->v0;
	sipvars->v0 = ROTL64(sipvars->v0, 32);
	sipvars->v2 += sipvars->v3;
	sipvars->v3 = ROTL64(sipvars->v3, 16);
	sipvars->v3 ^= sipvars->v2;
	sipvars->v0 += sipvars->v3;
	sipvars->v3 = ROTL64(sipvars->v3, 21);
	sipvars->v3 ^= sipvars->v0;
	sipvars->v2 += sipvars->v1;
	sipvars->v1 = ROTL64(sipvars->v1, 17);
	sipvars->v1 ^= sipvars->v2;
	sipvars->v2 = ROTL64(sipvars->v2, 32);
}

int
siphash(const uint8 *in, const uint32 inlen, const uint8 *k, uint8 *out, const uint32 outlen)
{
	sip_var_t sipvars = {0x736f6d6570736575ULL,
		0x646f72616e646f6dULL,
		0x6c7967656e657261ULL,
		0x7465646279746573ULL};
	uint64 k0 = load64_ua(k);
	uint64 k1 = load64_ua(k + 8);
	uint64 m;
	int i;
	const uint8 *end = in + inlen - (inlen % sizeof(uint64));
	int left = inlen & 7;
	uint64 b = ((uint64)inlen) << 56u;

	ASSERT((outlen == 8u) || (outlen == 16u));

	sipvars.v3 ^= k1;
	sipvars.v2 ^= k0;
	sipvars.v1 ^= k1;
	sipvars.v0 ^= k0;

	if (outlen == 16)
		sipvars.v1 ^= 0xee;

	for (; in != end; in += 8u) {
		m = load64_ua(in);
		sipvars.v3 ^= m;
		TRACE;
		for (i = 0; i < cROUNDS; ++i)
			sipround(&sipvars);

		sipvars.v0 ^= m;
	}

	while (left-- > 0) {
		b |= (((uint64)in[left]) << ((left) << 3u));
	}

	sipvars.v3 ^= b;

	TRACE;
	for (i = 0; i < cROUNDS; ++i) {
		sipround(&sipvars);
	}

	sipvars.v0 ^= b;

	if (outlen == 16) {
		sipvars.v2 ^= 0xee;
	} else {
		sipvars.v2 ^= 0xff;
	}

	TRACE;
	for (i = 0; i < dROUNDS; ++i) {
		sipround(&sipvars);
	}

	b = sipvars.v0 ^ sipvars.v1 ^ sipvars.v2 ^ sipvars.v3;
	store64_ua(out, b);

	if (outlen == 8) {
		return 0;
	}

	sipvars.v1 ^= 0xdd;

	TRACE;
	for (i = 0; i < dROUNDS; ++i) {
		sipround(&sipvars);
	}

	b = sipvars.v0 ^ sipvars.v1 ^ sipvars.v2 ^ sipvars.v3;
	store64_ua(out + 8, b);

	return 0;
}

/* SipHash implementation optimised for heap headers */

/*
 * We can gain cycles and space by making the following assumptions:
 * - the key is correctly aligned
 * - the input data is correctly aligned
 * - the input data is either 8 or 12 bytes long
 * - the required output size is 8 bytes
 *
 * Execution time is reduced by 22%.
 * Size is reduced by 74%.
 */

int
siphash_memhdr_opt(const uint8 *in, const uint32 inlen, const uint64 *k, uint64 *out)
{
	uint64 k0 = k[0];
	uint64 k1 = k[1];
	uint64 m;
	int i;
	uint64 b = ((uint64)inlen) << 56u;
	sip_var_t sipvars = {0x736f6d6570736575ULL,
		0x646f72616e646f6dULL,
		0x6c7967656e657261ULL,
		0x7465646279746573ULL};

	sipvars.v3 ^= k1;
	sipvars.v2 ^= k0;
	sipvars.v1 ^= k1;
	sipvars.v0 ^= k0;

	m = *(const uint64*)in;
	sipvars.v3 ^= m;

	TRACE;
	for (i = 0; i < cROUNDS; ++i) {
		sipround(&sipvars);
	}

	sipvars.v0 ^= m;
	if (inlen != 8) {
		b |= ((uint64)in[11]) << 24u;
		b |= ((uint64)in[10]) << 16u;
		b |= ((uint64)in[9]) << 8u;
		b |= ((uint64)in[8]);
	}

	sipvars.v3 ^= b;

	TRACE;
	for (i = 0; i < cROUNDS; ++i) {
		sipround(&sipvars);
	}

	sipvars.v0 ^= b;

	sipvars.v2 ^= 0xff;

	TRACE;
	for (i = 0; i < dROUNDS; ++i) {
		sipround(&sipvars);
	}

	b = sipvars.v0 ^ sipvars.v1 ^ sipvars.v2 ^ sipvars.v3;
	*out = b;

	return 0;
}

/* HalfSipHash implementation optimised for heap headers */

/*
 * We can gain cycles and space by making the following assumptions:
 * - the key is correctly aligned
 * - the input data is correctly aligned
 * - the input data is either 8 or 12 bytes long
 * - the required output size is 4 bytes
 *
 * Execution time is reduced by TBA%.
 * Size is reduced by TBA%.
 */
typedef struct hvee_vars {
	uint32 v0;
	uint32 v1;
	uint32 v2;
	uint32 v3;
} hsip_var_t;

static void hsipround(hsip_var_t *sipvars)
{
	sipvars->v0 += sipvars->v1;
	sipvars->v1 = ROTL(sipvars->v1, 5);
	sipvars->v1 ^= sipvars->v0;
	sipvars->v0 = ROTL(sipvars->v0, 16);
	sipvars->v2 += sipvars->v3;
	sipvars->v3 = ROTL(sipvars->v3, 8);
	sipvars->v3 ^= sipvars->v2;
	sipvars->v0 += sipvars->v3;
	sipvars->v3 = ROTL(sipvars->v3, 7);
	sipvars->v3 ^= sipvars->v0;
	sipvars->v2 += sipvars->v1;
	sipvars->v1 = ROTL(sipvars->v1, 13);
	sipvars->v1 ^= sipvars->v2;
	sipvars->v2 = ROTL(sipvars->v2, 16);
}

int
hsiphash_memhdr_opt(const uint8 *in, const uint32 inlen, const uint32 *k, uint32 *out)
{
	hsip_var_t sipvars = {0,
		0,
		0x6c796765U,
		0x74656462U};
	uint32 k0 = k[0];
	uint32 k1 = k[1];
	uint32 m;
	int i;
	uint32 b = ((uint32)inlen) << 24;

	sipvars.v3 ^= k1;
	sipvars.v2 ^= k0;
	sipvars.v1 ^= k1;
	sipvars.v0 ^= k0;

#ifdef OUTLEN8
	sipvars.v1 ^= 0xee;
#endif

	m = *(const uint32*)in;
	sipvars.v3 ^= m;

	TRACE;
	for (i = 0; i < cROUNDS; ++i) {
		hsipround(&sipvars);
	}
	sipvars.v0 ^= m;

	m = *(const uint32*)&in[4];
	sipvars.v3 ^= m;

	TRACE;
	for (i = 0; i < cROUNDS; ++i) {
		hsipround(&sipvars);
	}
	sipvars.v0 ^= m;

	if (inlen > 8) {
		m = *(const uint32*)&in[8];
		sipvars.v3 ^= m;

		TRACE;
		for (i = 0; i < cROUNDS; ++i)
			hsipround(&sipvars);
		sipvars.v0 ^= m;
	}

	sipvars.v3 ^= b;

	TRACE;
	for (i = 0; i < cROUNDS; ++i) {
		hsipround(&sipvars);
	}

	sipvars.v0 ^= b;

#ifdef OUTLEN8
	sipvars.v2 ^= 0xee;
#else
	sipvars.v2 ^= 0xff;
#endif
	TRACE;
	for (i = 0; i < dROUNDS; ++i) {
		hsipround(&sipvars);
	}

	b = sipvars.v1 ^ sipvars.v3;
	*out = b;

#ifdef OUTLEN8

	sipvars.v1 ^= 0xdd;

	TRACE;
	for (i = 0; i < dROUNDS; ++i) {
		hsipround(&sipvars);
	}

	b = sipvars.v1 ^ sipvars.v3;
	*++out = b;
#endif

	return 0;
}

int halfsiphash(const uint8 *in, const uint32 inlen, const uint32 *k, uint8 *out,
	const uint32 outlen)
{
	hsip_var_t sipvars = {0,
		0,
		0x6c796765U,
		0x74656462U};
	uint32 k0 = k[0];
	uint32 k1 = k[1];
	uint32 m;
	int i;
	const uint8 *end = in + inlen - (inlen % sizeof(uint32));
	int left = inlen & 3;
	uint32 b = ((uint32)inlen) << 24;
	sipvars.v3 ^= k1;
	sipvars.v2 ^= k0;
	sipvars.v1 ^= k1;
	sipvars.v0 ^= k0;

	ASSERT((outlen == 4u) || (outlen == 8u));
	if (outlen == 8) {
		sipvars.v1 ^= 0xee;
	}

	for (; in != end; in += 4) {
		m = load32_ua(in);
		sipvars.v3 ^= m;

		TRACE;
		for (i = 0; i < cROUNDS; ++i)
			hsipround(&sipvars);

		sipvars.v0 ^= m;
	}

	while (left-- > 0) {
		b |= (((uint64)in[left]) << ((left) << 3u));
	}

	sipvars.v3 ^= b;

	TRACE;
	for (i = 0; i < cROUNDS; ++i) {
		hsipround(&sipvars);
	}

	sipvars.v0 ^= b;

	if (outlen == 8) {
		sipvars.v2 ^= 0xee;
	} else {
		sipvars.v2 ^= 0xff;
	}

	TRACE;
	for (i = 0; i < dROUNDS; ++i) {
		hsipround(&sipvars);
	}

	b = sipvars.v1 ^ sipvars.v3;
	store32_ua(out, b);

	if (outlen == 4) {
		return 0;
	}

	sipvars.v1 ^= 0xdd;

	TRACE;
	for (i = 0; i < dROUNDS; ++i) {
		hsipround(&sipvars);
	}

	b = sipvars.v1 ^ sipvars.v3;
	store32_ua(out + 4, b);

	return 0;
}
