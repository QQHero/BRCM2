/*
* <:copyright-BRCM:2021:proprietary:standard
* 
*    Copyright (c) 2021 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/
/*
*******************************************************************************
* File Name  : archer_dhd.h
*
* Description: Archer DHD Offload definitions
*
*******************************************************************************
*/

#ifndef __ARCHER_DHD_H__
#define __ARCHER_DHD_H__

#if IS_ENABLED(CONFIG_BCM_DHD_ARCHER)

#include "archer_dhd_helper.h"
#include "bcm_async_queue.h"
#include "bcm_timer.h"

#define CC_ARCHER_DHD_BACKUP_QUEUE

//#define CC_ARCHER_DHD_DEBUG
#define CC_ARCHER_DHD_STATS

#define ARCHER_DHD_BACKUP_QUEUE_POOL_SIZE    21760 // entries per radio

#define ARCHER_DHD_RADIO_MAX                 RDPA_MAX_RADIOS
#define ARCHER_DHD_FLOW_RING_MAX             1024
#define ARCHER_DHD_FLOW_RING_GROUPS_MAX      16
#define LAST_IDMA_FRG_IDX                    15
#define DHD_RX_POST_FLOW_RING_SIZE           1024
#define DHD_RX_COMPLETE_FLOW_RING_SIZE       1024
#define DHD_TX_COMPLETE_FLOW_RING_SIZE       1024

#define ARCHER_DHD_RX_COMPLETE_QUEUE_SIZE    RDPA_DHD_HELPER_CPU_QUEUE_SIZE
#define ARCHER_DHD_TX_POST_QUEUE_SIZE        1024
#define ARCHER_DHD_CPU_MSG_QUEUE_SIZE        32

#define DHD_MSG_TYPE_FLOW_RING_FLUSH         0
#define DHD_MSG_TYPE_FLOW_RING_SET_DISABLED  1
#define DHD_MSG_TYPE_FLOW_RING_SET_ENABLED   2
#define DHD_MSG_TYPE_MAX                     3

#define DMA_TYPE_IDMA                        1
#define DMA_TYPE_HWA_RXPOST                  5
#define DMA_TYPE_HWA_TXCPL                   6
#define DMA_TYPE_HWA_RXCPL                   7

#define DHD_MSG_TYPE_TX_POST                 0xF
#define DHD_MSG_TYPE_RX_POST                 0x11

#define DHD_TX_POST_SKB_BUFFER_VALUE                0   /* 00: possible value in tx complete only */
#define DHD_TX_POST_HOST_BUFFER_VALUE               1   /* 01: possible value in tx post and tx complete */
#define DHD_TX_POST_BPM_BUFFER_VALUE                2   /* 10: possible value in tx post and tx complete */
#define DHD_TX_POST_FKB_BUFFER_VALUE                3   /* 11: possible value in tx complete only */

#define ARCHER_DHD_RADIO_IS_ENABLED(_radio_ptr) (_radio_ptr)->cpu_cfg.rx_isr

#define BCM_GBF(val, NAME)           (((val) & NAME##_MASK) >> NAME##_SHIFT)
#define BCM_SBF(val, NAME)           (((val) << NAME##_SHIFT) & NAME##_MASK)

/*
 * Ch2_HostToDevXDoorbell0 Register (Offset #160)
 * Name		Bits	Description
 * FRG_ID	3:0	Flow Ring Group for dma transfer [0-15].
 * 			Valid only when DMA TYPE = 0001
 * DMA_TYPE	7:4	Specify type of DMA transfer is requested.
 *			DMA_TYPE	MODE	Description
 *			0000	NO_IDMA	Do not initiate an IDMA transfer.
 *				Interrupt ARM.
 *			0001	IDMA	Transfer complete frame for Specified
 *				FRG_ID.
 *			0010	UNDEFINED	NO OP
 *			0011	UNDEFINED	NO OP
 *			0100	UNDEFINED	NO OP (reserved for  TXPOST)
 *			0101	HWA_RXPOST	Directly update RXPOST write index
 *						with "INDEX_VAL".
 *			0110	HWA_TXCPL	Directly update TXCPL read index
 *						with "INDEX_VAL".
 *			0111	HWA_RXCPL	Directly update RXCPL read index
 *						with "INDEX_VAL".
 *			1xxx	Reserved	NO OP
 * INDEX_NUM	15:8	Specifies the Index number to Update (used only  if there
 * 			are more than one Index in a ring)
 * INDEX_VAL	31:16	Index value to update in the HWA block.
 */

typedef uint32_t archer_dhd_doorbell_t;

#define FRG_ID_SHIFT            0
#define DMA_TYPE_SHIFT          4
#define INDEX_NUM_SHIFT         8
#define INDEX_VAL_SHIFT         16

#define DMA_TYPE_IDMA		1
#define DMA_TYPE_HWA_RXPOST	5
#define DMA_TYPE_HWA_TXCPL	6
#define DMA_TYPE_HWA_RXCPL	7

typedef enum {
    FR_FORMAT_FIRST        = 0,
    FR_FORMAT_WI_WI64      = 0,
    FR_FORMAT_WI_CWI32     = 1,
    FR_FORMAT_WI_CWI64     = 2,
    FR_FORMAT_WI_ACWI32    = 3,
    FR_FORMAT_WI_ACWI64    = 4,
    FR_FORMAT_LAST         = 4
} archer_dhd_flow_ring_format_t;

typedef struct {
    uint32_t rx_complete_ring_size; // OK
    uint32_t tx_complete_ring_size; // OK
    uint32_t rx_post_ring_size; // OK
    volatile void *rx_complete_fr_base_ptr; // OK
    volatile uint16_t *rx_complete_fr_rd_idx_ptr; // OK
    volatile uint16_t *rx_complete_fr_wr_idx_ptr; // OK
    volatile void *tx_complete_fr_base_ptr; // OK
    volatile uint16_t *tx_complete_fr_rd_idx_ptr; // OK
    volatile uint16_t *tx_complete_fr_wr_idx_ptr; // OK
    volatile void *rx_post_fr_base_ptr; // OK
    volatile uint16_t *rx_post_fr_rd_idx_ptr; // OK
    volatile uint16_t *rx_post_fr_wr_idx_ptr; // OK
    volatile archer_dhd_doorbell_t *dhd_doorbell_ptr; // OK
    uint16_t	rx_complete_rd_idx; // OK
    uint16_t	rx_complete_wr_idx; // OK
    uint16_t	tx_complete_rd_idx; // OK
    uint16_t	tx_complete_wr_idx; // OK
    uint16_t	rx_post_wr_idx; // OK
    archer_dhd_flow_ring_format_t flow_ring_format; // OK
    int	idma_active; // OK
    int idma_group_shift; // OK
    int idma_last_group_fr; // OK
    int fr_ptrs_size_shift; // OK
} archer_dhd_complete_common_t;

typedef struct {
    archer_dhd_flow_ring_format_t flow_ring_format; // OK
    volatile uint8_t *tx_post_fr_rd_idx_base_ptr; // OK
    volatile uint8_t *tx_post_fr_wr_idx_base_ptr; // OK
    int add_llcsnap_header; // OK
    archer_dhd_doorbell_t tx_post_doorbell_value[ARCHER_DHD_FLOW_RING_GROUPS_MAX]; // OK
    volatile archer_dhd_doorbell_t *dhd_doorbell_ptr; // OK
    int doorbell_counters[ARCHER_DHD_FLOW_RING_GROUPS_MAX];
    // FIXME: Duplicate in archer_dhd_complete_common_t
    int idma_active; // OK
    int idma_group_shift; // OK
    int idma_last_group_fr; // OK
    int fr_ptrs_size_shift; // OK
} archer_dhd_post_common_t;

/** HWA-2.0 compatible RxPost Compact 32bit host addressing format */
#define HWA_RXPOST_CWI32_WORDS      2
#define HWA_RXPOST_CWI32_BYTES      (HWA_RXPOST_CWI32_WORDS * 4)

typedef union hwa_rxpost_cwi32
{
    uint8_t   u8[HWA_RXPOST_CWI32_BYTES];       //  8 Bytes
    uint32_t u32[HWA_RXPOST_CWI32_WORDS];       //  2 words

    struct {                                  //
        uint32_t host_pktid;                    // +----- word#0
        uint32_t data_buf_haddr32;              // +----- word#1
    };
} hwa_rxpost_cwi32_t;

/** HWA-2.0 compatible RxCple Compact format */
#define HWA_RXCPLE_CWI_WORDS        2
#define HWA_RXCPLE_CWI_BYTES        (HWA_RXCPLE_CWI_WORDS * 4)

typedef union hwa_rxcple_cwi
{
    uint8_t   u8[HWA_RXCPLE_CWI_BYTES];         //  8 Bytes
    uint32_t u32[HWA_RXCPLE_CWI_WORDS];         //  2 Words

    struct {
        uint8_t ifid                  :  5;     // rx packet interface id
        uint8_t flags                 :  3;     // packet flags
        uint8_t data_offset;                    // start of packet data
        uint16_t data_len;                      // length of packet data
        uint32_t host_pktid;                    // host pktid
    };
} hwa_rxcple_cwi_t;

#define HWA_RXCPLE_IFID_SHIFT       0
#define HWA_RXCPLE_IFID_MASK        (0x1F << HWA_RXCPLE_IFID_SHIFT)
#define HWA_RXCPLE_FLAGS_SHIFT      5
#define HWA_RXCPLE_FLAGS_MASK       (0x7 << HWA_RXCPLE_FLAGS_SHIFT)
#define HWA_RXCPLE_DATA_OFFSET_SHIFT 8
#define HWA_RXCPLE_DATA_OFFSET_MASK  (0xFF << HWA_RXCPLE_DATA_OFFSET_SHIFT)
#define HWA_RXCPLE_DATA_LEN_SHIFT   16
#define HWA_RXCPLE_DATA_LEN_MASK    (0xFFFF << HWA_RXCPLE_DATA_LEN_SHIFT)

typedef union {
    uint32_t word_32[1];
    union {
        struct {
            uint32_t buffer_type : 2;
            uint32_t reserved    : 13;
            uint32_t drop        : 1;
            uint32_t free_index  : 16; // see archer_dhd_free_index_t
        };
        // FIXME: Won't work in ARM64
        pNBuff_t pNBuff;
        uint8_t *data_ptr;
    };
} archer_dhd_request_id_t;

typedef struct {
    uint32_t low;
    uint32_t high;
} dma64addr_t;

typedef dma64addr_t haddr64_t; /* No 64bit alignment requirement */

#define HWA_ETHER_ADDR_LEN  6 /* Length of a 802.3 Ethernet Address */

typedef union hwa_txpost_eth_sada
{
    uint8_t   u8[HWA_ETHER_ADDR_LEN * 2];
    uint32_t u32[(HWA_ETHER_ADDR_LEN * 2) / 4];
} hwa_txpost_eth_sada_t;

/** HWA-2.0 compatible TxPost Compact 64bit host addressing format */
#define HWA_TXPOST_CWI64_WORDS      8
#define HWA_TXPOST_CWI64_BYTES      (HWA_TXPOST_CWI64_WORDS * 4)

typedef union hwa_txpost_cwi64
{
    uint8_t   u8[HWA_TXPOST_CWI64_BYTES];       // 32 Bytes
    uint32_t u32[HWA_TXPOST_CWI64_WORDS];       //  8 Words

    struct {                                  //
        uint32_t host_pktid;                  // +----- word#0
        union {                               // +----- word#1
            struct {                          // BF Access
                uint32_t ifid             :  5; // interface id
                uint32_t prio             :  3; // packet priority
                uint32_t copy             :  1; // COPY as-is bit
                uint32_t flags            :  7; // flags
                uint32_t RSVD_FIELD_0     : 16; // ref: CD[data_buf_hlen]
            };                                //
            struct {                          // CD Access
                uint16_t RSVD_FIELD_1;        // ref: BF[ifid..flags]
                uint16_t data_buf_hlen;       // data buffer length
            };                                //
        }; // u32[1]
        haddr64_t data_buf_haddr64;           // +----- word#2 #3
        hwa_txpost_eth_sada_t eth_sada;       // +----- word#4 #5 #6
        union {                               // +----- word#7
            struct {                          // BF Access
                uint32_t RSVD_FIELD_2     : 16; // ref: CD[eth_type]
                uint32_t info             :  4; // Host field - future use
                uint32_t flowid_override  : 12; // Host flowid override
            };                                //
            struct {                          // CD Access
                uint16_t eth_type;            // packet Ether-Type
                uint16_t RSVD_FIELD_3;        // ref: BF[info..flowid_override]
            };                                //
        }; // u32[7]
    };
} hwa_txpost_cwi64_t;

#define HWA_TXPOST_IFID_SHIFT       0 /* word1 bits 0..4 : ifid */
#define HWA_TXPOST_IFID_MASK        (0x1F << HWA_TXPOST_IFID_SHIFT)
#define HWA_TXPOST_PRIO_SHIFT       5 /* word1 bits 5..7 : prio */
#define HWA_TXPOST_PRIO_MASK        (0x7 << HWA_TXPOST_PRIO_SHIFT)
#define HWA_TXPOST_COPY_SHIFT       8 /* word1 bit 8 : copy */
#define HWA_TXPOST_COPY_MASK        (0x1 << HWA_TXPOST_COPY_SHIFT)
#define HWA_TXPOST_FLAGS_SHIFT      9 /* word1 bit 9..15 : flags */
#define HWA_TXPOST_FLAGS_MASK       (0x7F << HWA_TXPOST_FLAGS_SHIFT)
#define HWA_TXPOST_DATA_BUF_HLEN_SHIFT (16)
#define HWA_TXPOST_DATA_BUF_HLEN_MASK (0xFFFF << HWA_TXPOST_DATA_BUF_HLEN_SHIFT)

#define HWA_TXPOST_INFO_SHIFT       16 /* word7 bits 16..19 : info */
#define HWA_TXPOST_INFO_MASK        (0xF << HWA_TXPOST_INFO_SHIFT)
#define HWA_TXPOST_FLOWOVRD_SHIFT   20 /* word7 bits 20..31 : flowid_override */
#define HWA_TXPOST_FLOWOVRD_MASK    (0xFFF << HWA_TXPOST_FLOWOVRD_SHIFT)

/** HWA-2.0 compatible TxCple Compact format */
#define HWA_TXCPLE_CWI_WORDS        2
#define HWA_TXCPLE_CWI_BYTES        (HWA_TXCPLE_CWI_WORDS * 4)

typedef union hwa_txcple_cwi
{
    uint8_t   u8[HWA_TXCPLE_CWI_BYTES];         //  8 Bytes
    uint32_t u32[HWA_TXCPLE_CWI_WORDS];         //  2 Words

    struct {
        uint32_t host_pktid;                    // host pktid
        union {                                   // +----- word#1
            uint32_t word1;
            struct {                              // word#1
                uint16_t flow_ring_id;
                uint8_t if_id;
                uint8_t reserved1;
            };
        }; // u32[1]
    };
} hwa_txcple_cwi_t;

typedef struct {
    uint8_t type;
    uint8_t radio_idx;
    uint8_t read_idx_valid;
    uint8_t reserved;
    uint16_t read_idx;
    uint16_t flow_ring_id;
} archer_dhd_cpu_msg_queue_t;

typedef rdpa_cpu_rx_info_t archer_dhd_rx_complete_queue_t;

typedef union {
    struct {
        archer_dhd_request_id_t request_id;
        union {
            struct {
                uint16_t status;
                struct {
                    uint16_t flow_ring_id : 15;
                    uint16_t drop : 1;
                };
            };
            uint32_t info;
        };
    };
    uint64_t u64;
} archer_dhd_tx_complete_queue_t;

typedef struct {
    uint8_t *data_ptr;
    archer_dhd_request_id_t request_id;
    uint16_t flow_ring_id;
    uint16_t data_len;
    uint16_t priority;
    uint16_t reserved;
} archer_dhd_tx_post_queue_t;

#define swap32(_x) (_x)
#define swap16(_x) (_x)

typedef struct {
    uint32_t packets;
    uint32_t discards;
} archer_dhd_complete_stats_t;

typedef struct {
    archer_dhd_complete_stats_t stats;
    bcm_async_queue_t queue;
    archer_task_t task;
} archer_dhd_rx_complete_t;

typedef struct {
    archer_dhd_complete_stats_t stats;
    bcm_async_queue_t queue;
    archer_task_t task;
} archer_dhd_tx_complete_t;

typedef struct {
    bcm_async_queue_t queue;
    archer_task_t task;
    archer_task_t coalescing_task;
    bcm_timer_user_t timer;
    int timer_running;
    struct {
        uint32_t packets;
        uint32_t free_index_discards;
        uint32_t flow_ring_discards;
        uint32_t transfers;
        uint32_t expirations;
    } stats;
} archer_dhd_tx_post_t;

typedef struct {
    bcm_async_queue_t queue;
    archer_task_t task;
} archer_dhd_cpu_msg_t;

typedef struct {
    uint32_t tx_buffers_in_use;
} archer_dhd_radio_stats_t;

typedef struct {
    archer_dhd_backup_queue_entry_t *free_ptr;
    archer_dhd_backup_queue_entry_t *array;
} archer_dhd_backup_queue_t;

typedef struct {
    rdpa_dhd_flring_cache_t *flring_cache; // OK
    archer_dhd_backup_queue_t backup_queue;
    archer_dhd_complete_common_t complete;
    archer_dhd_post_common_t post;
    archer_dhd_rx_complete_t rx_complete;
    archer_dhd_tx_complete_t tx_complete;
    archer_dhd_tx_post_t tx_post;
    archer_dhd_radio_stats_t stats;
    archer_dhd_cpu_msg_t cpu_msg;
    rdpa_cpu_rxq_cfg_t cpu_cfg;
    uint32_t nbr_of_flow_rings;
    uint32_t radio_idx;
} archer_dhd_radio_t;

typedef uint16_t archer_dhd_free_index_t; // see archer_dhd_request_id_t

typedef struct {
    volatile archer_dhd_free_index_t *alloc_p;
    volatile uint16_t write;
    volatile uint16_t read;
} archer_dhd_index_pool_t;

typedef struct {
    archer_dhd_index_pool_t pool;
    pNBuff_t *table;
    uint32_t writes;
    uint32_t reads;
} archer_dhd_buffer_table_t;

typedef struct {
    archer_dhd_radio_t radio[ARCHER_DHD_RADIO_MAX];
    archer_dhd_buffer_table_t buffer_table;
    uint32_t irq_set_phys_addr;
} archer_dhd_t;

extern archer_dhd_t archer_dhd_g;

#if defined(CC_ARCHER_DHD_STATS)
#define ARCHER_DHD_STATS_INCR(_counter) ( (_counter)++ )
#define ARCHER_DHD_STATS_DECR(_counter) ( (_counter)-- )
#else
#define ARCHER_DHD_STATS_INCR(_counter)
#define ARCHER_DHD_STATS_DECR(_counter)
#endif

#define ARCHER_DHD_PHYS_ADDR(_buffer_ptr)  VIRT_TO_PHYS((_buffer_ptr))

int rdd_dhd_hlp_cfg(uint32_t radio_idx, rdpa_dhd_init_cfg_t *init_cfg, int enable);
int rdp_drv_dhd_helper_flow_ring_flush(uint32_t radio_idx, uint32_t read_idx_flow_ring_id);
int rdp_drv_dhd_helper_flow_ring_enable_set(uint32_t radio_idx, uint32_t flow_ring_idx, int enable);
int rdp_drv_rx_post_init(uint32_t radio_idx, uint32_t num_items);
int rdp_drv_dhd_rx_post_uninit(uint32_t radio_idx, uint32_t *num_items);
int rdp_drv_dhd_rx_post_reinit(uint32_t radio_idx);
int rdp_drv_dhd_helper_dhd_complete_ring_create(uint32_t radio_idx, uint32_t ring_size);
int rdp_drv_dhd_helper_dhd_complete_ring_destroy(uint32_t radio_idx, uint32_t ring_size);
void rdp_drv_dhd_helper_wakeup_information_get(rdpa_dhd_wakeup_info_t *wakeup_info);

int archer_dhd_buffer_table_try_write(pNBuff_t pNBuff, archer_dhd_free_index_t *free_index_p);
int archer_dhd_tx_complete_task_handler(void *arg_p);
int archer_dhd_tx_post_task_handler(void *arg_p);
int archer_dhd_tx_coalescing_task_handler(void *arg_p);
void archer_dhd_tx_coalescing_timer_handler(void *arg_p);
int archer_dhd_rx_complete_task_handler(void *arg_p);
int archer_dhd_cpu_msg_task_handler(void *arg_p);
int archer_dhd_backup_queue_construct(archer_dhd_radio_t *radio_ptr);
void archer_dhd_processing_construct(void);
void archer_dhd_flow_ring_dump(uint32_t radio_idx, uint32_t flow_ring_id);
void archer_dhd_stats(void);
void archer_dhd_thread_wakeup(uint32_t radio_idx);
int archer_dhd_construct(void);

/* Copy 14B ethernet header: 32bit aligned source and destination. */
#define edasacopy32(s, d) \
do { \
    ((uint32_t *)(d))[0] = ((const uint32_t *)(s))[0]; \
    ((uint32_t *)(d))[1] = ((const uint32_t *)(s))[1]; \
    ((uint32_t *)(d))[2] = ((const uint32_t *)(s))[2]; \
} while (0)

#define RDD_LAYER2_HEADER_MINIMUM_LENGTH  14
#define LLCSNAP_HEADER_LENGTH             8
#define ETHER_TYPE_WORD                   6
/** LLCSNAP: OUI[2] setting for Bridge Tunnel (Apple ARP and Novell IPX) */
#define ETHER_TYPE_APPLE_ARP              0x80f3 /* Apple Address Resolution Protocol */
#define ETHER_TYPE_NOVELL_IPX             0x8137 /* Novel IPX Protocol */

#define BRIDGE_TUNNEL_OUI2                0xf8 /* OUI[2] value for Bridge Tunnel */

#define IS_BRIDGE_TUNNEL(et)                                            \
    (((et) == ETHER_TYPE_APPLE_ARP) || ((et) == ETHER_TYPE_NOVELL_IPX))

static inline uint8_t *archer_dhd_insert_llcsnap_header(uint8_t *data, uint32_t *pkt_length_p)
{
    const union {
        uint32_t u32;
        uint16_t u16[2];
        uint8_t u8[4];
    } ctl_oui3 = { .u8 = {0x00, 0x00, 0x00, 0x03} };
    uint32_t pkt_length = *pkt_length_p;
    uint8_t *source = data;
    uint8_t *dest;
    uint16_t eth_type;

    dest = source - LLCSNAP_HEADER_LENGTH;
    pkt_length += LLCSNAP_HEADER_LENGTH;
    eth_type = ((uint16_t *)(source))[ETHER_TYPE_WORD];

    edasacopy32(source, dest);

    /* default oui */
    ((uint32_t *)(dest))[4] = htonl(ctl_oui3.u32);

    /* ethernet payload length: 2B */
    ((uint16_t *)(dest))[6] = htons(pkt_length - RDD_LAYER2_HEADER_MINIMUM_LENGTH);

    /* dsap = 0xaa ssap = 0xaa: 2B copy */
    ((uint16_t *)(dest))[7] = (uint16_t)0xAAAA;

    /* Set OUI[2] for Bridge Tunnel */
    if (IS_BRIDGE_TUNNEL(eth_type))
    {
        ((uint8_t *)(dest))[19] = BRIDGE_TUNNEL_OUI2;
    }

    *pkt_length_p = pkt_length;

    return dest;
}

static inline void archer_dhd_tx_post_task_schedule(archer_dhd_radio_t *radio_ptr)
{
    if(archer_task_ref_count(&radio_ptr->tx_post.task) < 2)
    {
        archer_task_schedule(&radio_ptr->tx_post.task, ARCHER_TASK_PRIORITY_LOW);
    }
}

static inline void archer_dhd_tx_complete_task_schedule(archer_dhd_radio_t *radio_ptr)
{
    if(archer_task_ref_count(&radio_ptr->tx_complete.task) < 2)
    {
        archer_task_schedule(&radio_ptr->tx_complete.task, ARCHER_TASK_PRIORITY_LOW);
    }
}

static inline void archer_dhd_tx_coalescing_task_schedule(archer_dhd_radio_t *radio_ptr)
{
    if(archer_task_ref_count(&radio_ptr->tx_post.coalescing_task) < 2)
    {
        archer_task_schedule(&radio_ptr->tx_post.coalescing_task, ARCHER_TASK_PRIORITY_MEDIUM);
    }
}

static inline void archer_dhd_rx_complete_task_schedule(archer_dhd_radio_t *radio_ptr)
{
//    printk("rx_complete.task ref_count %d\n", archer_task_ref_count(&radio_ptr->rx_complete.task));

    if(archer_task_ref_count(&radio_ptr->rx_complete.task) < 2)
    {
        archer_task_schedule(&radio_ptr->rx_complete.task, ARCHER_TASK_PRIORITY_LOW);
    }
}

static inline void archer_dhd_cpu_msg_task_schedule(archer_dhd_radio_t *radio_ptr)
{
    if(archer_task_ref_count(&radio_ptr->cpu_msg.task) < 2)
    {
        archer_task_schedule(&radio_ptr->cpu_msg.task, ARCHER_TASK_PRIORITY_HIGH);
    }
}

static inline void archer_dhd_cpu_queue_notify(uint32_t radio_idx)
{
    archer_dhd_thread_wakeup(radio_idx);
}

#if defined(CC_ARCHER_DHD_DEBUG)
extern uint32_t archer_dhd_debug_g;

#define archer_dhd_debug(fmt, arg...)                   \
    if(archer_dhd_debug_g) bcm_print(fmt "\n", ##arg)

static inline void archer_dhd_packet_dump(void *packet_p, int packet_length, int has_brcmtag)
{
    if(archer_dhd_debug_g)
    {
        archer_dhd_debug_g--;

        sysport_classifier_packet_dump(packet_p, packet_length, has_brcmtag);
    }
}

static inline void archer_dhd_rsb_dump(sysport_rsb_t *rsb_p, int mlt_enable)
{
    if(archer_dhd_debug_g)
    {
        sysport_rsb_dump(rsb_p, NULL, mlt_enable);
    }
}
#else
#define archer_dhd_debug(fmt, arg...)
#define archer_dhd_packet_dump(packet_p, packet_length, has_brcmtag)
#define archer_dhd_rsb_dump(rsb_p, mlt_enable)
#endif

#else /* CONFIG_BCM_DHD_ARCHER */

#define archer_dhd_stats()

#define archer_dhd_construct()  0

#endif /* CONFIG_BCM_DHD_ARCHER */

#endif /* __ARCHER_DHD_H__ */
