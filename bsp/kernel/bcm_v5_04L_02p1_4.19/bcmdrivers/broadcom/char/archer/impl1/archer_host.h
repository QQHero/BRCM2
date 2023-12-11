/*
  <:copyright-BRCM:2017:proprietary:standard

  Copyright (c) 2017 Broadcom 
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
*
* File Name  : archer_host.h
*
* Description: Management of Host MAC Addresses
*
*******************************************************************************
*/

#ifndef __ARCHER_HOST_H_INCLUDED__
#define __ARCHER_HOST_H_INCLUDED__

typedef enum {
    ARCHER_HOST_OK                                        = 0,
    ARCHER_HOST_ERROR_NOENT                               = 1,
    ARCHER_HOST_ERROR_NORES                               = 2,
    ARCHER_HOST_ERROR_ENTRY_EXISTS                        = 3,
    ARCHER_HOST_ERROR_HOST_MAC_INVALID                    = 4,
    ARCHER_HOST_ERROR_HOST_MAC_ADDR_TABLE_INDEX_INVALID   = 5,
    ARCHER_HOST_ERROR_HOST_DEV_TABLE_INDEX_INVALID        = 6,
    ARCHER_HOST_ERROR_MAX
} ARCHER_HOST_ERROR_t;

#define ARCHER_HOST_DEV_TABLE_SIZE           16
#define ARCHER_HOST_MAC_ADDR_TABLE_SIZE      16

#define ARCHER_HOST_HASH_TABLE_BUCKET_MAX    (1 << 8) // Log2
#define ARCHER_HOST_HASH_TABLE_ENTRY_MAX     4
#define ARCHER_HOST_HASH_TABLE_BUCKET_MASK   (ARCHER_HOST_HASH_TABLE_BUCKET_MAX - 1)

typedef struct {
    uint16_t bucket_index;
    uint16_t entry_index;
} archer_host_hash_table_key_t;

typedef union {
    struct {
        uint16_t valid;
        BlogEthAddr_t addr;
    };
    uint64_t u64;
} archer_host_hash_table_entry_t;

typedef struct {
    archer_host_hash_table_entry_t entry[ARCHER_HOST_HASH_TABLE_ENTRY_MAX];
} archer_host_hash_table_bucket_t;

typedef struct {
    archer_host_hash_table_bucket_t bucket[ARCHER_HOST_HASH_TABLE_BUCKET_MAX];
    int enabled;
} archer_host_hash_table_t;

extern archer_host_hash_table_t archer_host_hash_table_g;
extern archer_mode_t archer_mode_g;

static inline int archer_host_hash_table_bucket_index(BlogEthAddr_t *addr_p)
{
    int bucket_index;

    bucket_index = addr_p->u16[0] + addr_p->u16[1] + addr_p->u16[2];

    bucket_index ^= ( bucket_index >> 16 );
    bucket_index ^= ( bucket_index >>  8 );
    bucket_index ^= ( bucket_index >>  3 );

    return (bucket_index & ARCHER_HOST_HASH_TABLE_BUCKET_MASK);
}

static inline int archer_host_hash_table_match(BlogEthAddr_t *addr_p)
{
    archer_host_hash_table_entry_t entry = { .valid = 1, .addr = *addr_p };
    int bucket_index = archer_host_hash_table_bucket_index(addr_p);
    archer_host_hash_table_bucket_t *bucket_p =
        &archer_host_hash_table_g.bucket[bucket_index];
    int entry_index;

    for(entry_index=0; entry_index<ARCHER_HOST_HASH_TABLE_ENTRY_MAX; ++entry_index)
    {
        if(entry.u64 == bucket_p->entry[entry_index].u64)
        {
            return 1;
        }
    }

    return 0;
}

int archer_host_mac_table_match(uint8_t *packet_p);

static inline int archer_host_mac_address_match(uint8_t *packet_p)
{
    if(likely(archer_mode_g == ARCHER_MODE_L2_L3))
    {
        if(likely(archer_host_hash_table_g.enabled))
        {
            return archer_host_hash_table_match((BlogEthAddr_t *)packet_p);
        }
        else
        {
            return archer_host_mac_table_match(packet_p);
        }
    }

    return 1;
}

ARCHER_HOST_ERROR_t archer_host_mac_addr_table_get(uint32_t xi_table_index,
                                                   BlogEthAddr_t *xo_host_mac_addr,
                                                   uint16_t *xo_ref_count);

static inline int archer_host_mac_address_get_all(BlogEthAddr_t *addr_array, int *count_p)
{
    int ret;
    int i;

    *count_p = 0;

    for(i=0; i<ARCHER_HOST_MAC_ADDR_TABLE_SIZE; ++i)
    {
        BlogEthAddr_t addr;
        uint16_t ref_count;

        if((ret = archer_host_mac_addr_table_get(i, &addr, &ref_count)))
        {
            break;
        }

        if(ref_count)
        {
            addr_array[(*count_p)++] = addr;
        }
    }

    return ret;
}

#endif  /* defined(__ARCHER_HOST_H_INCLUDED__) */
