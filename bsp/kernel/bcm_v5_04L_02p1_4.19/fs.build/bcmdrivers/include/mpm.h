/*
   Copyright (c) 2020 Broadcom Corporation
   All Rights Reserved

<:label-BRCM:2020:DUAL/GPL:standard

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
*/

/*
 *******************************************************************************
 * File Name  : mpm.h
 *
 * Description: This file contains the specification of some common definitions
 *      and interfaces to other modules. This file may be included by Kernel
 *      modules only.
 *
 *******************************************************************************
 */

#ifndef __MPM_H_INCLUDED__
#define __MPM_H_INCLUDED__

/*******************************************************************************
 *
 * Typical MPM usage:
 *
 * 1) User allocates one Buffer Allocation Ring for each Buffer Mode needed.
 *    The supported buffer modes are defined in mpm_buf_mode_t. Each Buffer
 *    Allocation Ring only supports one Buffer Mode.
 *
 * 2) User allocates one Buffer Free Ring, which can be used to free buffers
 *    of any Buffer Mode.
 *
 * 3) User allocates one or more buffers using the MPM Buffer Allocation API
 *    correspoding the the Buffer Mode of each Ring.
 *
 * 4) For each allocated buffer, user calls the corresponding MPM Buffer
 *    Initialization API.
 *
 * 5) User frees one or more buffers using the MPM Buffer Free API corresponding
 *    to the mode of each buffer.
 *
 * Notes:
 *
 * 1) Multiple rings are available to ensure multiple users can operate in
 *    parallel without the need of global synchronization. There are no
 *    dependencies between different Buffer Allocation Rings and
 *    Buffer Free Rings.
 *
 * 2) The Buffer Allocation API and Buffer Free API is non re-entrant and
 *    not protected against concurrent accesses. It is up to the user to
 *    manage the need for spinlocks, etc. In most cases synchronization
 *    should not be needed as multiple rings can be allocated to allow
 *    concurrency.
 *
 * 3) The number of Buffer Rings is implementation dependent.
 *
 *******************************************************************************/

/*******************************************************************************
 *
 * Layout of an MPM buffer
 *
 * FkBuff_t|BCM_PKT_HEADROOM|BCM_MAX_PKT_LEN|BCM_SKB_TAILROOM|BCM_SKB_SHAREDINFO
 *
 * ^       ^                ^               ^                ^
 * |       |                |               |                |
 * fkb     |              pData             |                |
 *         |                |               |                |
 *         |                |               |                |
 *      skb:head         skb:data        skb:tail         skb:end
 *
 * Note: The data pointer may move anywhere between head and end.
 *
 * DMA transfer to a buffer may only occur between pData and BCM_MAX_PKT_LEN.
 *
 *******************************************************************************/

#define MPM_FREE_OPCODE_MC_CNT_INC          0
#define MPM_FREE_OPCODE_MC_CNT_SET          1
#define MPM_FREE_OPCODE_BUFF_FREE           2
#define MPM_FREE_OPCODE_SKB_LIST_FREE       3
#define MPM_FREE_OPCODE_SKB_DATA_LIST_FREE  4
#define MPM_FREE_OPCODE_FKB_LIST_FREE       5

typedef unsigned int mpm_ring_index_t;

typedef enum {
    MPM_BUF_MODE_FKB = 0,           // FKB Buffer: Reset FKB Header, Set fkb->data, fkb->users = 1
    MPM_BUF_MODE_FKB_LIST,          // FKB Linked-list: Reset FKB Header, Set fkb->data, fkb->list
    MPM_BUF_MODE_PDATA,             // Data Buffer: Uninitialized
    MPM_BUF_MODE_PDATA_SHINFO,      // Data Buffer: Reset shinfo, Set shinfo.dataref = 1
    MPM_BUF_MODE_SKB_HEADER,        // SKB Header only: Reset SKB Header, Set skb->users = 1
    MPM_BUF_MODE_SKB_HEADER_LIST,   // Linked-List of MPM_BUF_MODE_SKB_HEADER (via skb->next, skb->prev)
    MPM_BUF_MODE_SKB_AND_DATA,      // SKB Header & Data Buffer Pair: Reset SKB Header, Set skb->users = 1,
                                    // Set skb->head, skb->data, skb->tail, skb->end,
                                    // Reset shinfo, Set shinfo.dataref = 1
    MPM_BUF_MODE_SKB_AND_DATA_LIST, // Linked-list of MPM_BUF_MODE_SKB_AND_DATA (via skb->next, skb->prev)
    MPM_BUF_MODE_MAX
} mpm_buf_mode_t;

typedef union {
    struct {
        uint32_t opcode : 3;
        uint32_t arg0   : 5;
        uint32_t arg1   : 24;
    };
    uint32_t u32;
} mpm_free_cmd_t;


/*******************************************************************************
 *
 * MPM Ring API
 *
 *******************************************************************************/
/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_ring_alloc
 * Description: Allocate a Buffer Allocation Ring for a specific buf_mode
 *              defined in mpm_buf_mode_t. The ring size is specified in log2
 *              format (e.g.: For 64 entries, ring_size_log2 = 6).
 *              When successful, a ring_index is returned. The ring_index
 *              should be passed to the Buffer Allocation API corresponding
 *              to the specified buf_mode.
 *------------------------------------------------------------------------------
 */
int mpm_alloc_ring_alloc(mpm_buf_mode_t buf_mode, int ring_size_log2,
                         mpm_ring_index_t *ring_index_p);

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_ring_alloc
 * Description: Allocate a Buffer Free Ring for the specified ring size in log2
 *              format (e.g.: For 64 entries, ring_size_log2 = 6).
 *              A Buffer Free Ring can handle all Buffer Modes specified
 *              in mpm_buf_mode_t.
 *              When successful, a ring_index is returned. The ring_index should
 *              to be passed when calling any of the Buffer Free API functions.
 *------------------------------------------------------------------------------
 */
int mpm_free_ring_alloc(int ring_size_log2, mpm_ring_index_t *ring_index_p);

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_ring_xx_read
 * Description: Read one or more entries from a Buffer Allocation Ring.
 *              These functions should not be called directly. Instead, use the
 *              Buffer Allocation API function corresponding to the Ring's
 *              buf_mode.
 *------------------------------------------------------------------------------
 */
void *mpm_alloc_ring_single_read(mpm_ring_index_t ring_index);
void *mpm_alloc_ring_list_read(mpm_ring_index_t ring_index, int count, void **tail_buffer_pp);
int mpm_alloc_ring_array_read(mpm_ring_index_t ring_index, int count, void **buffer_array, int offset);

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_ring_write
 * Description: Write one or more entries to a Buffer Free Ring.
 *              This function should not be called directly. Instead, use one of
 *              Buffer Free API functions.
 *------------------------------------------------------------------------------
 */
int mpm_free_ring_write(mpm_ring_index_t ring_index, void *buffer_p, mpm_free_cmd_t free_cmd);
int mpm_free_ring_array_write(mpm_ring_index_t ring_index, int count, void **buffer_array,
                              int offset, mpm_free_cmd_t free_cmd);


/*******************************************************************************
 *
 * Buffer Allocation API
 *
 *******************************************************************************/
/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_fkb
 * Description: Allocate one MPM_BUF_MODE_FKB buffer.
 *              Success: Return the allocated buffer pointer.
 *              Failure: Return a NULL pointer.
 *------------------------------------------------------------------------------
 */
static inline FkBuff_t *mpm_alloc_fkb(mpm_ring_index_t ring_index)
{
    return mpm_alloc_ring_single_read(ring_index);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_fkb_list
 * Description: Allocate a linked-list of MPM_BUF_MODE_FKB_LIST buffers.
 *              The linked-list is not NULL Terminated
 *              Success: Return the Head buffer pointer.
 *              Failure: Return a NULL pointer.
 *------------------------------------------------------------------------------
 */
static inline FkBuff_t *mpm_alloc_fkb_list(mpm_ring_index_t ring_index, int count,
                                           FkBuff_t **tail_fkb_p)
{
    return mpm_alloc_ring_list_read(ring_index, count, (void **)tail_fkb_p);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_fkb_array
 * Description: Allocate multiple MPM_BUF_MODE_FKB or MPM_BUF_MODE_FKB_LIST buffers.
 *              FKB pointers returned through the provided FKB array.
 *              Success: Return  0
 *              Failure: Return -1, no buffer pointers are returned
 *------------------------------------------------------------------------------
 */
static inline int mpm_alloc_fkb_array(mpm_ring_index_t ring_index, int count,
                                      FkBuff_t **fkb_array)
{
    return mpm_alloc_ring_array_read(ring_index, count, (void **)fkb_array, 0);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_pdata
 * Description: Allocate one MPM_BUF_MODE_PDATA or MPM_BUF_MODE_PDATA_SHINFO buffer.
 *              Success: Return the allocated buffer pointer.
 *              Failure: Return a NULL pointer.
 *------------------------------------------------------------------------------
 */
static inline uint8_t *mpm_alloc_pdata(mpm_ring_index_t ring_index)
{
    void *buffer_p = mpm_alloc_ring_single_read(ring_index);

    if(likely(buffer_p))
    {
        buffer_p = PFKBUFF_TO_PDATA(buffer_p, BCM_PKT_HEADROOM);
    }

    return buffer_p;
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_pdata_array
 * Description: Allocate multiple MPM_BUF_MODE_PDATA or MPM_BUF_MODE_PDATA_SHINFO
 *              buffers.
 *              pData pointers returned through the provided pData array.
 *              Success: Return  0
 *              Failure: Return -1, no buffer pointers are returned
 *------------------------------------------------------------------------------
 */
static inline int mpm_alloc_pdata_array(mpm_ring_index_t ring_index, int count,
                                        uint8_t **pdata_array)
{
    const uintptr_t offset = (uintptr_t)PFKBUFF_TO_PDATA(NULL, BCM_PKT_HEADROOM);

    return mpm_alloc_ring_array_read(ring_index, count, (void **)pdata_array, offset);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_skb_header
 * Description: Allocate one MPM_BUF_MODE_SKB_HEADER buffer.
 *              Success: Return the allocated buffer pointer.
 *              Failure: Return a NULL pointer.
 *------------------------------------------------------------------------------
 */
static inline struct sk_buff *mpm_alloc_skb_header(mpm_ring_index_t ring_index)
{
    return mpm_alloc_ring_single_read(ring_index);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_skb_header_list
 * Description: Allocate a linked-list of MPM_BUF_MODE_SKB_HEADER_LIST buffers.
 *              The linked-list is not NULL Terminated
 *              Success: Return the Head buffer pointer.
 *              Failure: Return a NULL pointer.
 *------------------------------------------------------------------------------
 */
static inline struct sk_buff *mpm_alloc_skb_header_list(mpm_ring_index_t ring_index, int count,
                                                        struct sk_buff **tail_skb_p)
{
    return mpm_alloc_ring_list_read(ring_index, count, (void **)tail_skb_p);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_skb_header_array
 * Description: Allocate multiple MPM_BUF_MODE_SKB_HEADER or
 *              MPM_BUF_MODE_SKB_HEADER_LIST buffers.
 *              SKB pointers returned through the provided SKB array.
 *              Success: Return  0
 *              Failure: Return -1, no buffer pointers are returned
 *------------------------------------------------------------------------------
 */
static inline int mpm_alloc_skb_header_array(mpm_ring_index_t ring_index, int count,
                                             struct sk_buff **skb_array)
{
    return mpm_alloc_ring_array_read(ring_index, count, (void **)skb_array, 0);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_skb_and_data
 * Description: Allocate one MPM_BUF_MODE_SKB_AND_DATA Pair.
 *              Success: Return the allocated buffer pointer.
 *              Failure: Return a NULL pointer.
 *------------------------------------------------------------------------------
 */
static inline struct sk_buff *mpm_alloc_skb_and_data(mpm_ring_index_t ring_index)
{
    return mpm_alloc_ring_single_read(ring_index);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_skb_and_data_list
 * Description: Allocate a linked-list of MPM_BUF_MODE_SKB_AND_DATA_LIST Pairs.
 *              The linked-list is not NULL Terminated
 *              Success: Return the Head buffer pointer.
 *              Failure: Return a NULL pointer.
 *------------------------------------------------------------------------------
 */
static inline struct sk_buff *mpm_alloc_skb_and_data_list(mpm_ring_index_t ring_index, int count,
                                                          struct sk_buff **tail_skb_p)
{
    return mpm_alloc_ring_list_read(ring_index, count, (void **)tail_skb_p);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_alloc_skb_and_data_array
 * Description: Allocate multiple MPM_BUF_MODE_SKB_AND_DATA or
 *              MPM_BUF_MODE_SKB_AND_DATA_LIST buffers.
 *              SKB pointers returned through the provided SKB array.
 *              Success: Return  0
 *              Failure: Return -1, no buffer pointers are returned
 *------------------------------------------------------------------------------
 */
static inline int mpm_alloc_skb_and_data_array(mpm_ring_index_t ring_index, int count,
                                               struct sk_buff **skb_array)
{
    return mpm_alloc_ring_array_read(ring_index, count, (void **)skb_array, 0);
}


/*******************************************************************************
 *
 * Buffer Initialization API
 *
 *******************************************************************************/
/*
 *------------------------------------------------------------------------------
 * Function   : mpm_fkb_init
 * Description: Completes the FKB initialization process by initializing fields
 *              that are not supported by the MPM HW Engine.
 *              This function should be called for all FKBs allocated from MPM
 *              (MPM_BUF_MODE_FKB or MPM_BUF_MODE_FKB_LIST).
 *------------------------------------------------------------------------------
 */
static inline void mpm_fkb_init(FkBuff_t *fkb, int data_len)
{
    fkb->len = data_len;
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_skb_and_data_init
 * Description: Completes the SKB initialization process by initializing fields
 *              that are not supported by the MPM HW Engine.
 *              This function should be called for all SKB and Data pairs
 *              allocated from MPM (MPM_BUF_MODE_SKB_AND_DATA or
 *              MPM_BUF_MODE_SKB_AND_DATA_LIST).
 *------------------------------------------------------------------------------
 */
static inline void mpm_skb_and_data_init(struct sk_buff *skb, int data_len,
                                         RecycleFuncP recycle_hook,
                                         unsigned long recycle_context,
                                         Blog_t *blog_p)
{
    skb->truesize = data_len + sizeof(struct sk_buff);

    skb->len = data_len;

#ifdef NET_SKBUFF_DATA_USES_OFFSET
    skb->tail = (sk_buff_data_t)(BCM_PKT_HEADROOM + data_len);
#else
    skb->tail = (sk_buff_data_t)((uintptr_t)(skb->data) + data_len);
#endif

    skb->recycle_hook = recycle_hook;
    skb->recycle_context = recycle_context;
    skb->recycle_flags = SKB_RECYCLE | SKB_DATA_RECYCLE;

#if defined(CONFIG_BLOG)
    if(blog_p)
    {
        skb->blog_p = blog_p;
        blog_p->skb_p = skb;
    }
#endif
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_skb_header_init
 * Description: Completes the SKB initialization process by initializing fields
 *              that are not supported by the MPM HW Engine.
 *              This function should be called for all SKB Headers allocated
 *              from MPM (MPM_BUF_MODE_SKB_HEADER or MPM_BUF_MODE_SKB_HEADER_LIST)
 *              that are attached later to a pre-initialized MPM pData buffer
 *              (MPM_BUF_MODE_PDATA_SHINFO).
 *------------------------------------------------------------------------------
 */
static inline void mpm_skb_header_init(struct sk_buff *skb, uint8_t *pData,
                                       int data_len, RecycleFuncP recycle_hook,
                                       unsigned long recycle_context,
                                       Blog_t *blog_p)
{
    skb->head = pData - BCM_PKT_HEADROOM;
    skb->data = pData;

#ifdef NET_SKBUFF_DATA_USES_OFFSET
    skb->end = BCM_PKT_HEADROOM + BCM_MAX_PKT_LEN + BCM_SKB_TAILROOM;
#else
    skb->end = pData + BCM_MAX_PKT_LEN + BCM_SKB_TAILROOM;
#endif

    mpm_skb_and_data_init(skb, data_len, recycle_hook, recycle_context, blog_p);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_skb_header_and_shinfo_init
 * Description: Completes the SKB initialization process by initializing fields
 *              that are not supported by the MPM HW Engine.
 *              This function should be called for all SKB Headers allocated
 *              from MPM (MPM_BUF_MODE_SKB_HEADER or MPM_BUF_MODE_SKB_HEADER_LIST)
 *              that are attached later to an uninitialized MPM pData buffer
 *              (MPM_BUF_MODE_PDATA).
 *------------------------------------------------------------------------------
 */
static inline void mpm_skb_header_and_shinfo_init(struct sk_buff *skb, uint8_t *pData,
                                                  int data_len, RecycleFuncP recycle_hook,
                                                  unsigned long recycle_context,
                                                  Blog_t *blog_p)
{
    mpm_skb_header_init(skb, pData, data_len, recycle_hook, recycle_context, blog_p);

    skb_shinforeset(skb_shinfo(skb));
}


/*******************************************************************************
 *
 * Buffer Free API
 *
 *******************************************************************************/
/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_fkb
 * Description: Free one MPM_BUF_MODE_FKB buffer.
 *              Success: Return  0
 *              Failure: Return -1, could not free buffer
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_fkb(mpm_ring_index_t ring_index, FkBuff_t *fkb)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_BUFF_FREE;

    return mpm_free_ring_write(ring_index, fkb, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_fkb_list
 * Description: Free a NULL Terminated Linked-List of MPM_BUF_MODE_FKB_LIST buffers.
 *              Success: Return  0
 *              Failure: Return -1, could not free any of the provided buffers
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_fkb_list(mpm_ring_index_t ring_index, FkBuff_t *fkb)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_FKB_LIST_FREE;

    return mpm_free_ring_write(ring_index, fkb, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_fkb_array
 * Description: Free multiple MPM_BUF_MODE_FKB or MPM_BUF_MODE_FKB_LIST buffers.
 *              FKB pointers obtained from the user provided FKB array.
 *              Success: Return  0
 *              Failure: Return -1, could not free any of the provided buffers
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_fkb_array(mpm_ring_index_t ring_index, int count,
                                     FkBuff_t **fkb_array)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_BUFF_FREE;

    return mpm_free_ring_array_write(ring_index, count, (void **)fkb_array,
                                     0, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_pdata
 * Description: Free one MPM_BUF_MODE_PDATA or MPM_BUF_MODE_PDATA_SHINFO buffer.
 *              Success: Return  0
 *              Failure: Return -1, could not free buffer
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_pdata(mpm_ring_index_t ring_index, uint8_t *pData)
{
    void *buffer = PDATA_TO_PFKBUFF(pData, BCM_PKT_HEADROOM);
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_BUFF_FREE;

    return mpm_free_ring_write(ring_index, buffer, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_pdata_array
 * Description: Free multiple MPM_BUF_MODE_PDATA or MPM_BUF_MODE_PDATA_SHINFO
 *              buffers.
 *              Data Buffer pointers obtained from the user provided pData array.
 *              Success: Return  0
 *              Failure: Return -1, could not free any of the provided buffers
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_pdata_array(mpm_ring_index_t ring_index, int count,
                                       uint8_t **pData_array)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };
    const uintptr_t offset = (uintptr_t)PFKBUFF_TO_PDATA(NULL, BCM_PKT_HEADROOM);

    free_cmd.opcode = MPM_FREE_OPCODE_BUFF_FREE;

    return mpm_free_ring_array_write(ring_index, count, (void **)pData_array,
                                     offset, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_skb_header
 * Description: Free one MPM_BUF_MODE_SKB_HEADER buffer.
 *              Success: Return  0
 *              Failure: Return -1, could not free buffer
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_skb_header(mpm_ring_index_t ring_index, struct sk_buff *skb)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_BUFF_FREE;

    return mpm_free_ring_write(ring_index, skb, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_skb_header_list
 * Description: Free a NULL Terminated Linked-List of MPM_BUF_MODE_SKB_HEADER_LIST
 *              buffers.
 *              Success: Return  0
 *              Failure: Return -1, could not free any of the provided buffers
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_skb_header_list(mpm_ring_index_t ring_index, struct sk_buff *skb)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_SKB_LIST_FREE;

    return mpm_free_ring_write(ring_index, skb, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_skb_header_array
 * Description: Free multiple MPM_BUF_MODE_SKB_HEADER or
 *              MPM_BUF_MODE_SKB_HEADER_LIST buffers.
 *              SKB pointers obtained from the user provided SKB array.
 *              Success: Return  0
 *              Failure: Return -1, could not free any of the provided buffers
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_skb_header_array(mpm_ring_index_t ring_index, int count,
                                            struct sk_buff **skb_array)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_BUFF_FREE;

    return mpm_free_ring_array_write(ring_index, count, (void **)skb_array,
                                     0, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_skb_and_data
 * Description: Free one MPM_BUF_MODE_SKB_AND_DATA Pair.
 *              Success: Return  0
 *              Failure: Return -1, could not free buffer
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_skb_and_data(mpm_ring_index_t ring_index, struct sk_buff *skb)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_SKB_DATA_LIST_FREE;
    free_cmd.arg0 = 1;

    return mpm_free_ring_write(ring_index, skb, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_skb_and_data_list
 * Description: Free a NULL Terminated Linked-List of
 *              MPM_BUF_MODE_SKB_AND_DATA_LIST Pairs.
 *              Success: Return  0
 *              Failure: Return -1, could not free any of the provided buffers
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_skb_and_data_list(mpm_ring_index_t ring_index, struct sk_buff *skb)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_SKB_DATA_LIST_FREE;

    return mpm_free_ring_write(ring_index, skb, free_cmd);
}

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_free_skb_and_data_array
 * Description: Free multiple MPM_BUF_MODE_SKB_AND_DATA or
 *              MPM_BUF_MODE_SKB_AND_DATA_LIST buffers.
 *              SKB pointers obtained from the user provided SKB array.
 *              Success: Return  0
 *              Failure: Return -1, could not free any of the provided buffers
 *------------------------------------------------------------------------------
 */
static inline int mpm_free_skb_and_data_array(mpm_ring_index_t ring_index, int count,
                                              struct sk_buff **skb_array)
{
    mpm_free_cmd_t free_cmd = { .u32 = 0 };

    free_cmd.opcode = MPM_FREE_OPCODE_SKB_DATA_LIST_FREE;
    free_cmd.arg0 = 1;

    return mpm_free_ring_array_write(ring_index, count, (void **)skb_array,
                                     0, free_cmd);
}

/*******************************************************************************
 *
 * Buffer Info API
 *
 *******************************************************************************/
/*
 *------------------------------------------------------------------------------
 * Function   : mpm_virt_to_idx
 * Description: given pointer to the start of the buffer,
 *              convert to a 17-bit buff_index
 *              Success: Return the buffer index
 *              Failure: Return 0xffffffff
 *------------------------------------------------------------------------------
 */
uint32_t mpm_virt_to_idx(void *buffer_ptr);

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_idx_to_virt
 * Description: With 17-bit buff_index, convert to the buffe pointer
 *              points to the beginning of the buffer
 *              Success: Return the pointer to the start of the buffer
 *              Failure: Return NULL
 *------------------------------------------------------------------------------
 */
void *mpm_idx_to_virt(uint32_t buffer_idx);

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_runner_xon_xoff_thld_get
 * Description: Obtain the current configured xon and xoff threshold for Runner.
 *              The unit for the threshold is the number of token.
 *              Success: Return 0
 *              Failure: Return -1
 *------------------------------------------------------------------------------
 */
int mpm_runner_xon_xoff_thld_get(int *xon_thld, int *xoff_thld);

/*
 *------------------------------------------------------------------------------
 * Function   : mpm_runner_xon_xoff_thld_set
 * Description: Configure xon and xoff threshold for Runner.
 *              The unit for the threshold is the number of token.
 *              Success: Return 0
 *              Failure: Return -1
 *------------------------------------------------------------------------------
 */
int mpm_runner_xon_xoff_thld_set(int xon_thld, int xoff_thld);
#endif  /* __MPM_H_INCLUDED__ */
