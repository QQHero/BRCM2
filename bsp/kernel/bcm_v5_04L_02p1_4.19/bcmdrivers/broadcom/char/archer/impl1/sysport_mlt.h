/*
<:copyright-BRCM:2018:proprietary:standard

   Copyright (c) 2018 Broadcom 
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
* File Name  : sysport_mlt.h
*
* Description: This file contains the System Port MLT implementation
*
*******************************************************************************
*/

#ifndef __SYSPORT_MLT_H__
#define __SYSPORT_MLT_H__

#if defined(CC_SYSPORT_MLT_V2)
#define SYSPORT_MLT_HASH_WIDTH         12
#else
#define SYSPORT_MLT_HASH_WIDTH         14
#endif

#define SYSPORT_MLT_HASH_SIZE_LOG2     ( SYSPORT_MLT_HASH_WIDTH / 2 )
#define SYSPORT_MLT_HASH_SIZE          ( 1 << SYSPORT_MLT_HASH_SIZE_LOG2 )

#define SYSPORT_MLT_BUCKET_SIZE_LOG2   2
#define SYSPORT_MLT_BUCKET_SIZE        (1 << SYSPORT_MLT_BUCKET_SIZE_LOG2)

#define SYSPORT_MLT_HASH_TABLE_SIZE    ( SYSPORT_MLT_HASH_SIZE * SYSPORT_MLT_BUCKET_SIZE ) // entries

#define SYSPORT_MLT_CAM_SIZE_LOG2      5
#define SYSPORT_MLT_CAM_SIZE           (1 << SYSPORT_MLT_CAM_SIZE_LOG2)

#define SYSPORT_MLT_MAC_ADDR_SIZE      6

/* MLT Return Status */
// Write and CAM Write Commands
#define SYSPORT_MLT_STATUS_MISS_FULL   0
#define SYSPORT_MLT_STATUS_MISS_WRITE  1
#define SYSPORT_MLT_STATUS_ERROR       2
#define SYSPORT_MLT_STATUS_HIT_WRITE   3
// Read Command
#define SYSPORT_MLT_STATUS_READ        0
// Search and Invalidate Commands
#define SYSPORT_MLT_STATUS_MISS        0
#define SYSPORT_MLT_STATUS_HIT         3

#define SYSPORT_MLT_HASH_TABLE_INST_LOG2  1

typedef enum {
    SYSPORT_MLT_HASH_TABLE_INST_LOW = 0,
    SYSPORT_MLT_HASH_TABLE_INST_HIGH,
    SYSPORT_MLT_HASH_TABLE_INST_MAX
} sysport_mlt_hash_table_inst_t;

typedef enum {
    SYSPORT_MLT_LOCATION_HASH = 0,
    SYSPORT_MLT_LOCATION_CAM,
    SYSPORT_MLT_LOCATION_MAX
} sysport_mlt_location_t;

#define SYSPORT_MLT_INDEX_HASH_TABLE_BITS (SYSPORT_MLT_HASH_TABLE_INST_LOG2 + \
                                           SYSPORT_MLT_HASH_SIZE_LOG2 + \
                                           SYSPORT_MLT_BUCKET_SIZE_LOG2)

#define SYSPORT_MLT_INDEX_RESERVED_BITS   (14 - SYSPORT_MLT_INDEX_HASH_TABLE_BITS)

typedef union {
    struct {
        uint16_t entry_index  : SYSPORT_MLT_BUCKET_SIZE_LOG2;
        uint16_t hash_index   : SYSPORT_MLT_HASH_SIZE_LOG2;
        uint16_t table_inst   : SYSPORT_MLT_HASH_TABLE_INST_LOG2;
        uint16_t location     : 1; // sysport_mlt_location_t
        uint16_t valid        : 1;
        uint16_t reserved     : SYSPORT_MLT_INDEX_RESERVED_BITS;
    };
    uint16_t u16;
} sysport_mlt_index_hash_table_t;

#define SYSPORT_MLT_INDEX_CAM_ZEROS (SYSPORT_MLT_INDEX_HASH_TABLE_BITS - \
                                     SYSPORT_MLT_CAM_SIZE_LOG2);
typedef union {
    struct {
        uint16_t cam_index : SYSPORT_MLT_CAM_SIZE_LOG2;
        uint16_t zeros     : SYSPORT_MLT_INDEX_CAM_ZEROS;
        uint16_t location  : 1; // sysport_mlt_location_t
        uint16_t valid     : 1;
        uint16_t reserved  : SYSPORT_MLT_INDEX_RESERVED_BITS;
    };
    uint16_t u16;
} sysport_mlt_index_cam_t;

typedef union {
    sysport_mlt_index_hash_table_t hash_table;
    sysport_mlt_index_cam_t cam;
    struct {
        uint16_t zeros     : 14 - SYSPORT_MLT_INDEX_RESERVED_BITS;
        uint16_t location  : 1; // sysport_mlt_location_t
        uint16_t valid     : 1;
        uint16_t reserved  : SYSPORT_MLT_INDEX_RESERVED_BITS;
    };
    uint16_t u16;
} sysport_mlt_index_t;

typedef enum {
    SYSPORT_MLT_MAC_ADDR_TYPE_STA = 0,
    SYSPORT_MLT_MAC_ADDR_TYPE_HOST,
    SYSPORT_MLT_MAC_ADDR_TYPE_MAX
} sysport_mlt_mac_addr_type_t;

typedef union {
    struct {
        uint32_t mac_addr_5_2;
        uint16_t mac_addr_1_0;
    };
    uint8_t u8[SYSPORT_MLT_MAC_ADDR_SIZE];
} sysport_mlt_key_t;

typedef union {
    struct {
        uint16_t rxq_index     : 3;
        uint16_t mac_addr_type : 1; // sysport_mlt_mac_addr_type_t
        uint16_t reserved      : 11;
        uint16_t valid         : 1;
    };
    uint16_t u16;
} sysport_mlt_info_t;

int sysport_mlt_search(sysport_mlt_key_t *key_p,
                       sysport_mlt_info_t *info_p,
                       sysport_mlt_index_t *mlt_index_p);

int sysport_mlt_write(sysport_mlt_key_t *key_p,
                      sysport_mlt_info_t *info_p,
                      sysport_mlt_index_t *mlt_index_p);

int sysport_mlt_cam_write(sysport_mlt_key_t *key_p,
                          sysport_mlt_info_t *info_p,
                          sysport_mlt_index_t *mlt_index_p);

int sysport_mlt_read(sysport_mlt_index_t mlt_index,
                     sysport_mlt_key_t *key_p,
                     sysport_mlt_info_t *info_p,
                     int *hash_index_p);

int sysport_mlt_invalidate_by_key(sysport_mlt_key_t *key_p,
                                  sysport_mlt_index_t *mlt_index_p);

void sysport_mlt_invalidate_by_index(sysport_mlt_index_t mlt_index);

void sysport_mlt_clear_hash_table(void);

void sysport_mlt_clear_cam(void);

void sysport_mlt_dump(sysport_mlt_index_t mlt_index);

void sysport_mlt_init(void);

#endif /* __SYSPORT_MLT_H__ */
