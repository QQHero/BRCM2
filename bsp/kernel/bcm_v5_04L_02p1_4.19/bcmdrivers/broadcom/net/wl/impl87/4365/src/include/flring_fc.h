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
* $Id: flring_fc.h jaganlv $
*
*/
#ifndef __flring_fc_h__
#define __flring_fc_h__

typedef struct flowring_op_param {
	uint32 weight;
	uint32 lfrag_max;
	uint32 phyrate;
	uint32 mumimo;
#ifdef WLATF_PERC
	uint32 atm_perc;
#endif /* WLATF_PERC */
	uint32 dwds;
} flowring_op_param_t;

typedef struct flowring_op_data {
	uint16	flowid;
	uint8	handle;
	uint8	tid;
	uint8	ifindex;
	uint8	maxpkts;
	uint8	minpkts;
	uint8	addr[ETHER_ADDR_LEN];
	/* pointer to extra params is there are any */
	void*   extra_params;
} flowring_op_data_t;

#endif /* __flring_fc_h__ */
