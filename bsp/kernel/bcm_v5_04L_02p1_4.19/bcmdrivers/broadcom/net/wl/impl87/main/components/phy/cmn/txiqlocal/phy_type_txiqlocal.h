/*
 * TXIQLO CAL module internal interface (to PHY specific implementations).
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
 * $Id: phy_type_txiqlocal.h 788934 2020-07-14 19:47:39Z $
 */

#ifndef _phy_type_txiqlocal_h_
#define _phy_type_txiqlocal_h_

#include <typedefs.h>
#include <bcmutils.h>
#include <phy_txiqlocal.h>

typedef struct phy_txiqlocal_priv_info phy_txiqlocal_priv_info_t;

typedef struct phy_txiqlocal_data {
	int8	txiqcalidx2g;
	int8	txiqcalidx5g;
} phy_txiqlocal_data_t;

struct phy_txiqlocal_info {
	phy_txiqlocal_priv_info_t *priv;
	phy_txiqlocal_data_t *data;
};

/*
 * PHY type implementation interface.
 *
 * Each PHY type implements the following functionality and registers the functions
 * via a vtbl/ftbl defined below, along with a context 'ctx' pointer.
 */
typedef void phy_type_txiqlocal_ctx_t;

typedef int (*phy_type_txiqlocal_init_fn_t)(phy_type_txiqlocal_ctx_t *ctx);
typedef int (*phy_type_txiqlocal_dump_fn_t)(phy_type_txiqlocal_ctx_t *ctx, struct bcmstrbuf *b);
typedef void (*phy_type_txiqlocal_txiqccget_fn_t)(phy_type_txiqlocal_ctx_t *ctx, void *a);
typedef void (*phy_type_txiqlocal_txiqccset_fn_t)(phy_type_txiqlocal_ctx_t *ctx, void *b);
typedef void (*phy_type_txiqlocal_txloccget_fn_t)(phy_type_txiqlocal_ctx_t *ctx, void *a);
typedef void (*phy_type_txiqlocal_txloccset_fn_t)(phy_type_txiqlocal_ctx_t *ctx, void *b);
typedef void (*phy_type_txiqlocal_scanroam_cache_fn_t)(phy_type_txiqlocal_ctx_t *ctx, bool set);
typedef int (*phy_type_txiqlocal_get_var_fn_t) (phy_type_txiqlocal_ctx_t *ctx, int32 *var);
typedef int (*phy_type_txiqlocal_set_int_fn_t) (phy_type_txiqlocal_ctx_t *ctx, int32 var);
typedef struct {
	phy_type_txiqlocal_txiqccget_fn_t	txiqccget;
	phy_type_txiqlocal_txiqccset_fn_t	txiqccset;
	phy_type_txiqlocal_txloccget_fn_t	txloccget;
	phy_type_txiqlocal_txloccset_fn_t	txloccset;
	phy_type_txiqlocal_scanroam_cache_fn_t	scanroam_cache;
	phy_type_txiqlocal_ctx_t		*ctx;
	phy_type_txiqlocal_get_var_fn_t		get_calidx;
	phy_type_txiqlocal_set_int_fn_t		set_calidx;
	phy_type_txiqlocal_get_var_fn_t		get_target_tssi;
	phy_type_txiqlocal_set_int_fn_t		set_target_tssi;
	phy_type_txiqlocal_get_var_fn_t		get_tssi_search_enable;
	phy_type_txiqlocal_set_int_fn_t		set_tssi_search_enable;
} phy_type_txiqlocal_fns_t;

/*
 * Register/unregister PHY type implementation to the txiqlocal module.
 * It returns BCME_XXXX.
 */
int phy_txiqlocal_register_impl(phy_txiqlocal_info_t *cmn_info, phy_type_txiqlocal_fns_t *fns);
void phy_txiqlocal_unregister_impl(phy_txiqlocal_info_t *cmn_info);

#endif /* _phy_type_txiqlocal_h_ */
