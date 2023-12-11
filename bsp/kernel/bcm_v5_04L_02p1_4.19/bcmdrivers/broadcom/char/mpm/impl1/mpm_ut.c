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
*
* File Name  : mpm_ut.c
*
* Description: Broadcom Memory Pool Manager (MPM) Unit Test
*
*******************************************************************************
*/

#include <linux/module.h>
#include <linux/nbuff.h>
#include <linux/bcm_skb_defines.h>
#include <linux/gbpm.h>

#include "bcm_pkt_lengths.h"

#include "mpm_local.h"
#include "mpm.h"

#if IS_ENABLED(CONFIG_BCM_BPM) && !defined(CONFIG_BCM_MPM_OVER_BPM)
#define CC_MPM_UT_BUFFER_CHECK
#endif
//#define CC_MPM_UT_BUFFER_DUMP
#define CC_MPM_UT_BPM_RECYCLE_HOOK

#define MPM_UT_DATA_BUFFER_SIZE      BCM_PKTBUF_SIZE
#define MPM_UT_DATA_LENGTH           (MPM_UT_DATA_BUFFER_SIZE / 2)

#define MPM_UT_ALLOC_RING_SIZE_LOG2  7 // 128
#define MPM_UT_FREE_RING_SIZE_LOG2   7 // 128
#define MPM_UT_RECYCLE_CONTEXT       0xAAAA

#define MPM_UT_ALLOC_NBR_OF_BUFFERS  (1 << 20)
#define MPM_UT_ALLOC_LIST_SIZE       64
#define MPM_UT_HOLD_SIZE             (1 << 13)

#define MPM_UT_RETRY_MAX             1000

#define MPM_UT_SHINFO_CHECK_SIZE ( offsetof(struct skb_shared_info, dataref) + \
                                   MPM_MEMBER_SIZEOF(struct skb_shared_info, dataref) )

#define MPM_UT_BUFFER_VALID(_buffer)                                    \
    ( (void *)(_buffer) >= mpm_ut_g.virt_base &&                        \
      (void *)(_buffer) < mpm_ut_g.virt_end )

#define MPM_UT_POINTER_VALID(_ptr)                      \
    ( (_ptr) && (uintptr_t)(_ptr) != (uintptr_t)(-1) )

#define skb1_head skb1->head
#define skb2_head skb2->head
#define skb1_data skb1->data
#define skb2_data skb2->data
#define skb1_tail skb1->tail
#define skb2_tail skb2->tail
#define skb1_end skb1->end
#define skb2_end skb2->end
#define skb1_next skb1->next
#define skb2_next skb2->next
#define skb1_prev skb1->prev
#define skb2_prev skb2->prev
#define skb1_users skb1->users
#define skb2_users skb2->users
#define skb1_truesize skb1->truesize
#define skb2_truesize skb2->truesize

#define MPM_UT_SKB_DATA_COMPARE(_field)                                 \
    do {                                                                \
        uintptr_t _offset1 = (uintptr_t)skb1_##_field - (uintptr_t)fkb1; \
        uintptr_t _offset2 = (uintptr_t)skb2_##_field - (uintptr_t)fkb2; \
        if(_offset1 != _offset2)                                        \
        {                                                               \
            __logError("skb->%s: %d != %d", #_field, _offset1, _offset2); \
            fail_count++;                                               \
        }                                                               \
    } while(0)

#define MPM_UT_SKB_VAL_COMPARE(_field)                                  \
    do {                                                                \
        typeof(skb1_##_field) _val1 = skb1_##_field;                    \
        typeof(skb2_##_field) _val2 = skb2_##_field;                    \
        if(_val1 != _val2)                                              \
        {                                                               \
            __logError("skb->%s: 0x%x != 0x%x", #_field, _val1, _val2); \
            fail_count++;                                               \
        }                                                               \
    } while(0)

#if defined(CC_MPM_UT_BUFFER_CHECK)
typedef struct {
    struct sk_buff *skb;
    FkBuff_t *fkb;
} mpm_ut_ref_t;
#endif

typedef struct {
#if defined(CC_MPM_UT_BUFFER_CHECK)
    mpm_ut_ref_t ref;
#endif
    uint8_t data[MPM_UT_DATA_BUFFER_SIZE];
    struct sk_buff *skb_hold[MPM_UT_HOLD_SIZE];
    mpm_ring_index_t alloc_ring_index[MPM_BUF_MODE_MAX];
    mpm_ring_index_t free_ring_index;
    Blog_t blog;
    void *virt_base;
    unsigned int mem_size;
    void *virt_end;
} mpm_ut_t;

static mpm_ut_t mpm_ut_g;

static void mpm_ut_mem_dump(void *p, int length)
{
    uint8_t *mem_p = p;
    int i;

    bcm_print("Address 0x%px, Length %d\n", mem_p, length);

    for(i=0; i<length; ++i)
    {
        if(!(i % 16))
        {
            if(i)
            {
                bcm_print("\n");
            }

            bcm_print("%06x :", i);
        }

        bcm_print(" %02X", *(mem_p + i));
    }

    bcm_print("\n");
}

void mpm_ut_skb_header_dump(struct sk_buff *skb)
{
    int dataref;

    if(MPM_UT_POINTER_VALID(skb->head) && MPM_UT_POINTER_VALID(skb->end))
    {
        dataref = skb_shinfo(skb)->dataref.counter;
    }
    else
    {
        dataref = -1;
    }

    bcm_print("SKB (0x%px, %d): head 0x%px, data 0x%px, tail 0x%px, end 0x%px, "
              "next 0x%px, prev 0x%px, users %d, dataref %d\n",
              skb, sizeof(struct sk_buff), skb->head, skb->data, skb->tail, skb->end,
              skb->next, skb->prev, skb->users, dataref);

    mpm_ut_mem_dump(skb, sizeof(struct sk_buff));

    bcm_print("\n");
}

#if defined(CC_MPM_UT_BUFFER_CHECK)
#define mpm_ut_pdata_shinfo_check(_pData)                               \
    mpm_ut_pdata_shinfo_compare(_pData, PFKBUFF_TO_PDATA(mpm_ut_g.data, BCM_PKT_HEADROOM))

int mpm_ut_pdata_shinfo_compare(uint8_t *pData1, uint8_t *pData2)
{
    FkBuff_t *fkb1 = PDATA_TO_PFKBUFF(pData1, BCM_PKT_HEADROOM);
    FkBuff_t *fkb2 = PDATA_TO_PFKBUFF(pData2, BCM_PKT_HEADROOM);
    struct skb_shared_info *skb_shinfo_1 = (struct skb_shared_info *)
        ((uintptr_t)(fkb1) + MPM_DATA_SHINFO_OFFSET);
    struct skb_shared_info *skb_shinfo_2 = (struct skb_shared_info *)
        ((uintptr_t)(fkb2) + MPM_DATA_SHINFO_OFFSET);
    uintptr_t skb_shinfo_offset_1 = (uintptr_t)skb_shinfo_1 - (uintptr_t)fkb1;
    uintptr_t skb_shinfo_offset_2 = (uintptr_t)skb_shinfo_2 - (uintptr_t)fkb2;
    int fail_count = 0;

    if(skb_shinfo_offset_1 == skb_shinfo_offset_2)
    {
        if(memcmp(skb_shinfo_1, skb_shinfo_2, MPM_UT_SHINFO_CHECK_SIZE))
        {
            __logError("skb_shinfo differs");

            mpm_ut_mem_dump(skb_shinfo_1, MPM_UT_SHINFO_CHECK_SIZE);
            mpm_ut_mem_dump(skb_shinfo_2, MPM_UT_SHINFO_CHECK_SIZE);

            fail_count++;
        }
    }
    else
    {
        __logError("skb_shinfo offset mismatch: offset_1 %u, offset_2 %u",
                   skb_shinfo_offset_1, skb_shinfo_offset_2);

        fail_count++;
    }

    return fail_count;
}

#define mpm_ut_skb_check(_skb, _linked) mpm_ut_skb_compare(_skb, mpm_ut_g.ref.skb, _linked)

int mpm_ut_skb_compare(struct sk_buff *skb1, struct sk_buff *skb2, int linked)
{
    FkBuff_t *fkb1 = PDATA_TO_PFKBUFF(skb1->data, BCM_PKT_HEADROOM);
    FkBuff_t *fkb2 = PDATA_TO_PFKBUFF(skb2->data, BCM_PKT_HEADROOM);
    struct skb_shared_info *skb_shinfo_1 = skb_shinfo(skb1);
    struct skb_shared_info *skb_shinfo_2 = skb_shinfo(skb2);
    uintptr_t skb_shinfo_offset_1 = (uintptr_t)skb_shinfo_1 - (uintptr_t)fkb1;
    uintptr_t skb_shinfo_offset_2 = (uintptr_t)skb_shinfo_2 - (uintptr_t)fkb2;
    int fail_count = 0;

    MPM_UT_SKB_DATA_COMPARE(head);
    MPM_UT_SKB_DATA_COMPARE(data);
#ifdef NET_SKBUFF_DATA_USES_OFFSET
    MPM_UT_SKB_VAL_COMPARE(tail);
    MPM_UT_SKB_VAL_COMPARE(end);
#else
    MPM_UT_SKB_DATA_COMPARE(tail);
    MPM_UT_SKB_DATA_COMPARE(end);
#endif
    if(!linked)
    {
        MPM_UT_SKB_VAL_COMPARE(next);
        MPM_UT_SKB_VAL_COMPARE(prev);
    }
    MPM_UT_SKB_VAL_COMPARE(truesize);
    MPM_UT_SKB_VAL_COMPARE(users.refs.counter);

    if(skb_shinfo_offset_1 == skb_shinfo_offset_2)
    {
        if(memcmp(skb_shinfo_1, skb_shinfo_2, MPM_UT_SHINFO_CHECK_SIZE))
        {
            __logError("skb_shinfo differs");

            mpm_ut_mem_dump(skb_shinfo_1, MPM_UT_SHINFO_CHECK_SIZE);
            mpm_ut_mem_dump(skb_shinfo_2, MPM_UT_SHINFO_CHECK_SIZE);

            fail_count++;
        }
    }
    else
    {
        __logError("skb_shinfo offset mismatch: offset_1 %u, offset_2 %u",
                   skb_shinfo_offset_1, skb_shinfo_offset_2);

        fail_count++;
    }

    return fail_count;
}

#define mpm_ut_fkb_check(_fkb, _linked) mpm_ut_fkb_compare(_fkb, mpm_ut_g.ref.fkb, _linked)

int mpm_ut_fkb_compare(FkBuff_t *fkb1, FkBuff_t *fkb2, int linked)
{
    int fail_count = 0;
    int offset1;
    int offset2;

    if(fkb1->len != fkb2->len)
    {
        __logError("fkb->len mismatch: %d != %d", fkb1->len, fkb2->len);

        fail_count++;
    }

    if(!linked)
    {
        if(fkb1->users.counter != fkb2->users.counter)
        {
            __logError("fkb->users mismatch: %ld != %ld", fkb1->users, fkb2->users);

            fail_count++;
        }
    }

    offset1 = (uintptr_t)fkb1->data - (uintptr_t)fkb1;
    offset2 = (uintptr_t)fkb2->data - (uintptr_t)fkb2;
    if(offset1 != offset2)
    {
        __logError("fkb->data mismatch: %d != %d", offset1, offset2);

        fail_count++;
    }

    return fail_count;
}
#else
#define mpm_ut_pdata_shinfo_check(_pData)  0
#define mpm_ut_skb_check(_skb, _linked)  0
#define mpm_ut_fkb_check(_fkb, _linked)  0
#endif /* CC_MPM_UT_BUFFER_CHECK */

int mpm_ut_skb_and_data_list_audit(struct sk_buff *list_head, int buffer_count)
{
    int dump_max = buffer_count - (MPM_UT_ALLOC_LIST_SIZE / 2);
    int dump_min = (MPM_UT_ALLOC_LIST_SIZE / 2);
    struct sk_buff *skb = list_head;
    int list_index = 0;

    while(skb && list_index < buffer_count)
    {
        if(!MPM_UT_BUFFER_VALID(skb))
        {
            __logError("LIST[%d][%d]: Invalid skb 0x%px", skb);

            return -1;
        }

        if(!MPM_UT_BUFFER_VALID(skb->data))
        {
            __logError("LIST[%d][%d]: Invalid skb->data 0x%px", skb->data);

            return -1;
        }

        if(!list_index)
        {
            bcm_print("prev <- curr (head) -> next\n");
        }

        if(list_index < dump_min || list_index >= dump_max)
        {
            bcm_print("[%06u] 0x%px <- 0x%px (0x%px) -> 0x%px\n",
                      list_index, skb->prev, skb, skb->head, skb->next);
        }

        if(list_index < 4)
        {
            bcm_print("\tskb->next (0x%x)\n", (uintptr_t)(&skb->next) - (uintptr_t)(skb));
            bcm_print("\tskb->head (0x%x) 0x%px\n", (uintptr_t)(&skb->head) - (uintptr_t)(skb), skb->head);
            bcm_print("\tskb->data (0x%x) 0x%px\n", (uintptr_t)(&skb->data) - (uintptr_t)(skb), skb->data);
            mpm_ut_skb_header_dump(skb);
        }

        skb = skb->next;

        list_index++;
    }

    if(skb)
    {
        __logError("SKB Linked-List is not NULL terminated");

        return -1;
    }

    bcm_print("SKB Linked-List Audit: skb 0x%xp, list_size %d/%d\n", skb, list_index, buffer_count);

    return 0;
}

static void mpm_ut_recycle_handler(void *nbuff_p, unsigned long context, uint32_t flags)
{
    bcm_print("mpm_ut_recycle_handler\n");
}

typedef struct timespec64 mpm_ut_time_t;

static inline void mpm_ut_get_time(mpm_ut_time_t *time_p)
{
    ktime_get_real_ts64(time_p);
}

static inline uint64_t mpm_ut_runtime_msec(mpm_ut_time_t start_time)
{
    mpm_ut_time_t end_time;
    mpm_ut_time_t runtime;
    uint64_t runtime_msec;

    mpm_ut_get_time(&end_time);

    runtime = timespec64_sub(end_time, start_time);

    runtime_msec = (runtime.tv_sec * MSEC_PER_SEC) + (runtime.tv_nsec / NSEC_PER_MSEC);

    return runtime_msec;
}

#if defined(CONFIG_ARM64)
static inline void mpm_ut_test_results(mpm_ut_time_t start_time, int buffer_count, int fail_count)
{
    uint64_t runtime_msec = mpm_ut_runtime_msec(start_time);
    int64_t bps = -1;

    if(runtime_msec)
    {
        bps = (uint64_t)(buffer_count * MSEC_PER_SEC) / runtime_msec;
    }

    bcm_print("\tResults: %d buffers, %d failures, runtime %d msec, %d buffers/s\n",
              buffer_count, fail_count, runtime_msec, bps);
}
#else
static inline void mpm_ut_test_results(mpm_ut_time_t start_time, int buffer_count, int fail_count)
{
    uint32_t runtime_msec = mpm_ut_runtime_msec(start_time);
    int32_t bps = -1;

    if(runtime_msec)
    {
        bps = (buffer_count / runtime_msec) * (uint32_t)MSEC_PER_SEC;
    }

    bcm_print("\tResults: %d buffers, %d failures, runtime %d msec, %d buffers/s\n",
              buffer_count, fail_count, runtime_msec, bps);
}
#endif

int mpm_ut_test_fkb_alloc_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_FKB;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    mpm_ut_time_t start_time;
    FkBuff_t *fkb;
    int fail_count = 0;
    int ret = 0;
    int i;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d\n",
              __FUNCTION__, buf_mode, ring_index);

    mpm_ut_get_time(&start_time);

    for(i=0; i<MPM_UT_ALLOC_NBR_OF_BUFFERS; ++i)
    {
        int retry_count = MPM_UT_RETRY_MAX;

        while(!(fkb = mpm_alloc_fkb(ring_index)) && --retry_count);
        if(!fkb)
        {
            __logError("Could not mpm_alloc_fkb");

            ret = -1;
            break;
        }

        mpm_fkb_init(fkb, MPM_UT_DATA_LENGTH);

#if defined(CC_MPM_UT_BUFFER_DUMP)
        bcm_print("FKB (0x%px, %d): data 0x%px, len %d, users %d\n",
                  fkb, MPM_FKB_SIZE, fkb->data, fkb->len, fkb->users);

        mpm_ut_mem_dump(fkb, MPM_FKB_SIZE);
        bcm_print("\n");
#endif
        fail_count += mpm_ut_fkb_check(fkb, 0);

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_fkb(mpm_ut_g.free_ring_index, fkb)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_fkb");

            ret = -1;
            break;
        }
    }

    mpm_ut_test_results(start_time, i, fail_count);

    return ret;
}

int mpm_ut_test_fkb_list_alloc_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_FKB_LIST;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    int nbr_of_lists = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_ALLOC_LIST_SIZE;
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int list_index;
    int ret = 0;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, nbr_of_lists %d, list_size %d\n",
              __FUNCTION__, buf_mode, ring_index, nbr_of_lists, MPM_UT_ALLOC_LIST_SIZE);

    mpm_ut_get_time(&start_time);

    for(list_index=0; list_index<nbr_of_lists; ++list_index)
    {
        FkBuff_t *head_fkb;
        FkBuff_t *tail_fkb;
        FkBuff_t *curr_fkb;
        int fkb_index = 0;
        int retry_count = MPM_UT_RETRY_MAX;

        while(!(head_fkb = mpm_alloc_fkb_list(ring_index, MPM_UT_ALLOC_LIST_SIZE, &tail_fkb)) && --retry_count);
        if(!head_fkb)
        {
            __logError("Could not mpm_alloc_fkb_list");

            ret = -1;
            break;
        }

        curr_fkb = head_fkb;

        while(1)
        {
            mpm_fkb_init(curr_fkb, MPM_UT_DATA_LENGTH);

#if defined(CC_MPM_UT_BUFFER_DUMP)
            bcm_print("FKB (0x%px, %d): data 0x%px, len %d, users %d\n",
                      fkb, MPM_FKB_SIZE, fkb->data, fkb->len, fkb->users);

            mpm_ut_mem_dump(fkb, MPM_FKB_SIZE);
            bcm_print("\n");
#endif
            fail_count += mpm_ut_fkb_check(curr_fkb, 1);

#if defined(CC_MPM_UT_BUFFER_CHECK)
            if(fkb_index == MPM_UT_ALLOC_LIST_SIZE - 1)
            {
                if(curr_fkb != tail_fkb)
                {
                    __logError("LIST[%d][%d]: curr_fkb 0x%px != tail_fkb 0x%px",
                               list_index, fkb_index, curr_fkb, tail_fkb);

                    return -1;
                }

                break;
            }
            else
            {
                if(curr_fkb == tail_fkb)
                {
                    __logError("LIST[%d][%d]: curr_fkb == tail_fkb (0x%px)",
                               list_index, fkb_index, curr_fkb);
                    return -1;
                }
            }
#else
            if(fkb_index == MPM_UT_ALLOC_LIST_SIZE - 1)
            {
                break;
            }
#endif
            curr_fkb = curr_fkb->list;

            fkb_index++;
        }

        // Must be NULL terminated linked-list in order to free
        tail_fkb->list = NULL;

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_fkb_list(mpm_ut_g.free_ring_index, head_fkb)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_fkb_list");

            ret = -1;
            break;
        }
    }

    mpm_ut_test_results(start_time, list_index * MPM_UT_ALLOC_LIST_SIZE, fail_count);

    return ret;
}

int mpm_ut_test_fkb_array_alloc_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_FKB;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    int nbr_of_arrays = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_ALLOC_LIST_SIZE;
    FkBuff_t *fkb_array[MPM_UT_ALLOC_LIST_SIZE];
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int array_index;
    int ret = 0;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, nbr_of_arrays %d, array_size %d\n",
              __FUNCTION__, buf_mode, ring_index, nbr_of_arrays, MPM_UT_ALLOC_LIST_SIZE);

    mpm_ut_get_time(&start_time);

    for(array_index=0; array_index<nbr_of_arrays; ++array_index)
    {
        int retry_count = MPM_UT_RETRY_MAX;
        int fkb_index;

        while((ret = mpm_alloc_fkb_array(ring_index, MPM_UT_ALLOC_LIST_SIZE, fkb_array)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_alloc_fkb_array");

            break;
        }

        for(fkb_index=0; fkb_index<MPM_UT_ALLOC_LIST_SIZE; ++fkb_index)
        {
            FkBuff_t *curr_fkb = fkb_array[fkb_index];

            mpm_fkb_init(curr_fkb, MPM_UT_DATA_LENGTH);

#if defined(CC_MPM_UT_BUFFER_DUMP)
            bcm_print("FKB (0x%px, %d): data 0x%px, len %d, users %d\n",
                      curr_fkb, MPM_FKB_SIZE, curr_fkb->data, curr_fkb->len, curr_fkb->users);

            mpm_ut_mem_dump(curr_fkb, MPM_FKB_SIZE);
            bcm_print("\n");
#endif
            fail_count += mpm_ut_fkb_check(curr_fkb, 0);
        }

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_fkb_array(mpm_ut_g.free_ring_index, MPM_UT_ALLOC_LIST_SIZE, fkb_array)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_fkb_array");

            break;
        }
    }

    mpm_ut_test_results(start_time, array_index * MPM_UT_ALLOC_LIST_SIZE, fail_count);

    return ret;
}

int mpm_ut_test_pdata_alloc_free(mpm_buf_mode_t buf_mode)
{
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int ret = 0;
    int i;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d\n",
              __FUNCTION__, buf_mode, ring_index);

    mpm_ut_get_time(&start_time);

    for(i=0; i<MPM_UT_ALLOC_NBR_OF_BUFFERS; ++i)
    {
        int retry_count = MPM_UT_RETRY_MAX;
        uint8_t *pData;

        while(!(pData = mpm_alloc_pdata(ring_index)) && --retry_count);
        if(!pData)
        {
            __logError("Could not mpm_alloc_pData");

            ret = -1;
            break;
        }

        if(MPM_BUF_MODE_PDATA_SHINFO == buf_mode)
        {
            fail_count += mpm_ut_pdata_shinfo_check(pData);
        }

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_pdata(mpm_ut_g.free_ring_index, pData)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_pData");

            ret = -1;
            break;
        }
    }

    mpm_ut_test_results(start_time, i, fail_count);

    return ret;
}

int mpm_ut_test_pdata_array_alloc_free(mpm_buf_mode_t buf_mode)
{
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    int nbr_of_arrays = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_ALLOC_LIST_SIZE;
    uint8_t *pdata_array[MPM_UT_ALLOC_LIST_SIZE];
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int array_index;
    int ret = 0;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, nbr_of_arrays %d, array_size %d\n",
              __FUNCTION__, buf_mode, ring_index, nbr_of_arrays, MPM_UT_ALLOC_LIST_SIZE);

    mpm_ut_get_time(&start_time);

    for(array_index=0; array_index<nbr_of_arrays; ++array_index)
    {
        int retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_alloc_pdata_array(ring_index, MPM_UT_ALLOC_LIST_SIZE, pdata_array)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_alloc_pdata_array");

            break;
        }

#if defined(CC_MPM_UT_BUFFER_CHECK)
        if(MPM_BUF_MODE_PDATA_SHINFO == buf_mode)
        {
            int pdata_index;

            for(pdata_index=0; pdata_index<MPM_UT_ALLOC_LIST_SIZE; ++pdata_index)
            {
                uint8_t *pData = pdata_array[pdata_index];

                fail_count += mpm_ut_pdata_shinfo_check(pData);
            }
        }
#endif
        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_pdata_array(mpm_ut_g.free_ring_index, MPM_UT_ALLOC_LIST_SIZE, pdata_array)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_pdata_array");

            break;
        }
    }

    mpm_ut_test_results(start_time, array_index * MPM_UT_ALLOC_LIST_SIZE, fail_count);

    return ret;
}

int bpm_ut_test_pdata_array_alloc_free(void)
{
    int nbr_of_arrays = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_ALLOC_LIST_SIZE;
    void *pdata_array[MPM_UT_ALLOC_LIST_SIZE];
    mpm_ut_time_t start_time;
    int array_index;
    int ret;

    bcm_print("\n%s\n", __FUNCTION__);

    mpm_ut_get_time(&start_time);

    for(array_index=0; array_index<nbr_of_arrays; ++array_index)
    {
        ret = gbpm_alloc_mult_buf(MPM_UT_ALLOC_LIST_SIZE, pdata_array);
        if(ret)
        {
            __logError("Could not gbpm_alloc_mult_buf");

            break;
        }

        gbpm_free_mult_buf(MPM_UT_ALLOC_LIST_SIZE, pdata_array);
    }

    mpm_ut_test_results(start_time, array_index * MPM_UT_ALLOC_LIST_SIZE, 0);

    return 0;
}

int mpm_ut_test_skb_header_alloc_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_SKB_HEADER;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    uint8_t *pData = PFKBUFF_TO_PDATA(mpm_ut_g.data, BCM_PKT_HEADROOM);
    mpm_ut_time_t start_time;
    struct sk_buff *skb;
    int fail_count = 0;
    int ret = 0;
    int i;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d\n",
              __FUNCTION__, buf_mode, ring_index);

    mpm_ut_get_time(&start_time);

    for(i=0; i<MPM_UT_ALLOC_NBR_OF_BUFFERS; ++i)
    {
        int retry_count = MPM_UT_RETRY_MAX;

        while(!(skb = mpm_alloc_skb_header(ring_index)) && --retry_count);
        if(!skb)
        {
            __logError("Could not mpm_alloc_skb_header");

            ret = -1;
            break;
        }

        mpm_skb_header_and_shinfo_init(skb, pData, MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                                       MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);
#if defined(CC_MPM_UT_BUFFER_DUMP)
        mpm_ut_skb_header_dump(skb);
#endif
        fail_count += mpm_ut_skb_check(skb, 0);

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_skb_header(mpm_ut_g.free_ring_index, skb)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_skb_header");

            ret = -1;
            break;
        }
    }

    mpm_ut_test_results(start_time, i, fail_count);

    return ret;
}

int bpm_ut_test_skb_header_alloc_free(void)
{
    uint8_t *pData = PFKBUFF_TO_PDATA(mpm_ut_g.data, BCM_PKT_HEADROOM);
    mpm_ut_time_t start_time;
    struct sk_buff *skb;
    int i;

    bcm_print("\n%s\n", __FUNCTION__);

    mpm_ut_get_time(&start_time);

    for(i=0; i<MPM_UT_ALLOC_NBR_OF_BUFFERS; ++i)
    {
        skb = gbpm_alloc_skb();
        if(!skb)
        {
            __logError("Could not gbpm_alloc_buf_skb_attach");

            break;
        }

        gbpm_attach_skb(skb, pData, MPM_UT_DATA_LENGTH);

#if defined(CC_MPM_UT_BPM_RECYCLE_HOOK)
        skb->recycle_flags &= ~SKB_BPM_PRISTINE;
        skb->recycle_hook(skb, 0, SKB_RECYCLE);
#else
        gbpm_free_skb(skb);
#endif
    }

    mpm_ut_test_results(start_time, i, 0);

    return 0;
}

int mpm_ut_test_skb_header_alloc_hold_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_SKB_HEADER;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    uint8_t *pData = PFKBUFF_TO_PDATA(mpm_ut_g.data, BCM_PKT_HEADROOM);
    int hold_count = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_HOLD_SIZE;
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int hold_index;
    int ret = 0;
    int i;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, hold %u\n",
              __FUNCTION__, buf_mode, ring_index, MPM_UT_HOLD_SIZE);

    mpm_ut_get_time(&start_time);

    for(hold_index=0; hold_index<hold_count; ++hold_index)
    {
        for(i=0; i<MPM_UT_HOLD_SIZE; ++i)
        {
            int retry_count = MPM_UT_RETRY_MAX;

            while(!(mpm_ut_g.skb_hold[i] = mpm_alloc_skb_header(ring_index)) && --retry_count);
            if(!mpm_ut_g.skb_hold[i])
            {
                __logError("Could not mpm_alloc_skb_header");

                ret = -1;
                break;
            }

            mpm_skb_header_and_shinfo_init(mpm_ut_g.skb_hold[i], pData, MPM_UT_DATA_LENGTH,
                                           mpm_ut_recycle_handler, MPM_UT_RECYCLE_CONTEXT,
                                           &mpm_ut_g.blog);
#if defined(CC_MPM_UT_BUFFER_DUMP)
            mpm_ut_skb_header_dump(mpm_ut_g.skb_hold[i]);
#endif
            /* if(MPM_UT_POINTER_VALID(mpm_ut_g.skb_hold[i]->data)) */
            /* { */
            /*     mpm_ut_mem_dump(PDATA_TO_PFKBUFF(mpm_ut_g.skb_hold[i]->data, BCM_PKT_HEADROOM), 4096); */
            /* } */

            fail_count += mpm_ut_skb_check(mpm_ut_g.skb_hold[i], 0);
        }

        for(i=0; i<MPM_UT_HOLD_SIZE; ++i)
        {
            int retry_count = MPM_UT_RETRY_MAX;

            while((ret = mpm_free_skb_header(mpm_ut_g.free_ring_index, mpm_ut_g.skb_hold[i])) && --retry_count);
            if(ret)
            {
                __logError("Could not mpm_free_skb_header");

                ret = -1;
                break;
            }
        }
    }

    mpm_ut_test_results(start_time, hold_index * MPM_UT_HOLD_SIZE, fail_count);

    return ret;
}

int bpm_ut_test_skb_header_alloc_hold_free(void)
{
    mpm_ut_time_t start_time;
    uint8_t *pData = PFKBUFF_TO_PDATA(mpm_ut_g.data, BCM_PKT_HEADROOM);
    int hold_count = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_HOLD_SIZE;
    int hold_index;
    int i;

    bcm_print("\n%s: hold %u\n", __FUNCTION__, MPM_UT_HOLD_SIZE);

    mpm_ut_get_time(&start_time);

    for(hold_index=0; hold_index<hold_count; ++hold_index)
    {
        for(i=0; i<MPM_UT_HOLD_SIZE; ++i)
        {
            mpm_ut_g.skb_hold[i] = gbpm_alloc_skb();
            if(!mpm_ut_g.skb_hold[i])
            {
                __logError("Could not gbpm_alloc_buf_skb_attach");

                break;
            }

            gbpm_attach_skb(mpm_ut_g.skb_hold[i], pData, MPM_UT_DATA_LENGTH);
        }

        for(i=0; i<MPM_UT_HOLD_SIZE; ++i)
        {
#if defined(CC_MPM_UT_BPM_RECYCLE_HOOK)
            mpm_ut_g.skb_hold[i]->recycle_flags &= ~SKB_BPM_PRISTINE;
            mpm_ut_g.skb_hold[i]->recycle_hook(mpm_ut_g.skb_hold[i], 0, SKB_RECYCLE);
#else
            gbpm_free_skb(mpm_ut_g.skb_hold[i]);
#endif
        }
    }

    mpm_ut_test_results(start_time, hold_index * MPM_UT_HOLD_SIZE, 0);

    return 0;
}

int mpm_ut_test_skb_header_list_alloc_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_SKB_HEADER_LIST;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    int nbr_of_lists = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_ALLOC_LIST_SIZE;
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int list_index;
    int ret = 0;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, nbr_of_lists %d, list_size %d\n",
              __FUNCTION__, buf_mode, ring_index, nbr_of_lists, MPM_UT_ALLOC_LIST_SIZE);

    mpm_ut_get_time(&start_time);

    for(list_index=0; list_index<nbr_of_lists; ++list_index)
    {
        struct sk_buff *head_skb;
        struct sk_buff *tail_skb;
        struct sk_buff *curr_skb;
#if defined(CC_MPM_UT_BUFFER_CHECK)
        struct sk_buff *prev_skb = NULL;
#endif
        int skb_index = 0;
        int retry_count = MPM_UT_RETRY_MAX;

        while(!(head_skb = mpm_alloc_skb_header_list(ring_index, MPM_UT_ALLOC_LIST_SIZE, &tail_skb)) && --retry_count);
        if(!head_skb)
        {
            __logError("Could not mpm_alloc_skb_header_list");

            ret = -1;
            break;
        }

        curr_skb = head_skb;

        while(1)
        {
            mpm_skb_header_and_shinfo_init(curr_skb, mpm_ut_g.data, MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                                           MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);
#if defined(CC_MPM_UT_BUFFER_DUMP)
            mpm_ut_skb_header_dump(curr_skb);
#endif
            /* if(MPM_UT_POINTER_VALID(skb->data)) */
            /* { */
            /*     mpm_ut_mem_dump(PDATA_TO_PFKBUFF(skb->data, BCM_PKT_HEADROOM), 4096); */
            /* } */

            fail_count += mpm_ut_skb_check(curr_skb, 1);

#if defined(CC_MPM_UT_BUFFER_CHECK)
            if(prev_skb && curr_skb->prev != prev_skb)
            {
                __logError("LIST[%d][%d]: curr_skb->prev 0x%px != prev_skb 0x%px",
                           list_index, skb_index, curr_skb->prev, prev_skb);

                return -1;
            }

            if(skb_index == MPM_UT_ALLOC_LIST_SIZE - 1)
            {
                if(curr_skb != tail_skb)
                {
                    __logError("LIST[%d][%d]: curr_skb 0x%px != tail_skb 0x%px",
                               list_index, skb_index, curr_skb, tail_skb);

                    return -1;
                }

                break;
            }
            else
            {
                if(curr_skb == tail_skb)
                {
                    __logError("LIST[%d][%d]: curr_skb == tail_skb (0x%px)",
                               list_index, skb_index, curr_skb);
                    return -1;
                }
            }

            prev_skb = curr_skb;
#else
            if(skb_index == MPM_UT_ALLOC_LIST_SIZE - 1)
            {
                break;
            }
#endif
            curr_skb = curr_skb->next;

            skb_index++;
        }

        // Must be NULL terminated linked-list in order to free
        tail_skb->next = NULL;

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_skb_header_list(mpm_ut_g.free_ring_index, head_skb)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_skb_header_list");

            ret = -1;
            break;
        }
    }

    mpm_ut_test_results(start_time, list_index * MPM_UT_ALLOC_LIST_SIZE, fail_count);

    return ret;
}

int mpm_ut_test_skb_header_array_alloc_free(mpm_buf_mode_t buf_mode)
{
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    int nbr_of_arrays = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_ALLOC_LIST_SIZE;
    struct sk_buff *skb_array[MPM_UT_ALLOC_LIST_SIZE];
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int array_index;
    int ret = 0;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, nbr_of_arrays %d, array_size %d\n",
              __FUNCTION__, buf_mode, ring_index, nbr_of_arrays, MPM_UT_ALLOC_LIST_SIZE);

    mpm_ut_get_time(&start_time);

    for(array_index=0; array_index<nbr_of_arrays; ++array_index)
    {
        int retry_count = MPM_UT_RETRY_MAX;
        int skb_index;

        while((ret = mpm_alloc_skb_header_array(ring_index, MPM_UT_ALLOC_LIST_SIZE, skb_array)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_alloc_skb_header_array");

            break;
        }

        for(skb_index=0; skb_index<MPM_UT_ALLOC_LIST_SIZE; ++skb_index)
        {
            struct sk_buff *curr_skb = skb_array[skb_index];

            mpm_skb_header_and_shinfo_init(curr_skb, mpm_ut_g.data, MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                                           MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);
#if defined(CC_MPM_UT_BUFFER_DUMP)
            mpm_ut_skb_header_dump(curr_skb);
#endif
            /* if(MPM_UT_POINTER_VALID(skb->data)) */
            /* { */
            /*     mpm_ut_mem_dump(PDATA_TO_PFKBUFF(skb->data, BCM_PKT_HEADROOM), 4096); */
            /* } */

            fail_count += mpm_ut_skb_check(curr_skb, 1);
        }

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_skb_header_array(mpm_ut_g.free_ring_index, MPM_UT_ALLOC_LIST_SIZE, skb_array)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_skb_header_array");

            break;
        }
    }

    mpm_ut_test_results(start_time, array_index * MPM_UT_ALLOC_LIST_SIZE, fail_count);

    return ret;
}

int mpm_ut_test_skb_and_data_alloc_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_SKB_AND_DATA;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    mpm_ut_time_t start_time;
    struct sk_buff *skb;
    int fail_count = 0;
    int ret = 0;
    int i;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d\n",
              __FUNCTION__, buf_mode, ring_index);

    mpm_ut_get_time(&start_time);

    for(i=0; i<MPM_UT_ALLOC_NBR_OF_BUFFERS; ++i)
    {
        int retry_count = MPM_UT_RETRY_MAX;

        while(!(skb = mpm_alloc_skb_and_data(ring_index)) && --retry_count);
        if(!skb)
        {
            __logError("Could not mpm_alloc_skb_and_data");

            ret = -1;
            break;
        }

        mpm_skb_and_data_init(skb, MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                              MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);

#if defined(CC_MPM_UT_BUFFER_DUMP)
        mpm_ut_skb_header_dump(skb);
#endif
        /* if(MPM_UT_POINTER_VALID(skb->data)) */
        /* { */
        /*     mpm_ut_mem_dump(PDATA_TO_PFKBUFF(skb->data, BCM_PKT_HEADROOM), 4096); */
        /* } */

        fail_count += mpm_ut_skb_check(skb, 0);

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_skb_and_data(mpm_ut_g.free_ring_index, skb)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_skb_and_data");

            ret = -1;
            break;
        }
    }

    mpm_ut_test_results(start_time, i, fail_count);

    return ret;
}

int bpm_ut_test_skb_and_data_alloc_free(void)
{
    mpm_ut_time_t start_time;
    struct sk_buff *skb;
    int i;

    bcm_print("\n%s\n", __FUNCTION__);

    mpm_ut_get_time(&start_time);

    for(i=0; i<MPM_UT_ALLOC_NBR_OF_BUFFERS; ++i)
    {
        skb = gbpm_alloc_buf_skb_attach(MPM_UT_DATA_LENGTH);
        if(!skb)
        {
            __logError("Could not gbpm_alloc_buf_skb_attach");

            break;
        }

#if defined(CC_MPM_UT_BPM_RECYCLE_HOOK)
        skb->recycle_flags &= ~SKB_BPM_PRISTINE;
        skb->recycle_hook(skb, 0, SKB_DATA_RECYCLE);
        skb->recycle_hook(skb, 0, SKB_RECYCLE);
#else
        gbpm_free_buf(skb->data);
        gbpm_free_skb(skb);
#endif
    }

    mpm_ut_test_results(start_time, i, 0);

    return 0;
}

int mpm_ut_test_skb_and_data_alloc_hold_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_SKB_AND_DATA;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    mpm_ut_time_t start_time;
    int hold_count = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_HOLD_SIZE;
    int fail_count = 0;
    int hold_index;
    int ret = 0;
    int i;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, hold %u\n",
              __FUNCTION__, buf_mode, ring_index, MPM_UT_HOLD_SIZE);

    mpm_ut_get_time(&start_time);

    for(hold_index=0; hold_index<hold_count; ++hold_index)
    {
        for(i=0; i<MPM_UT_HOLD_SIZE; ++i)
        {
            int retry_count = MPM_UT_RETRY_MAX;

            while(!(mpm_ut_g.skb_hold[i] = mpm_alloc_skb_and_data(ring_index)) && --retry_count);
            if(!mpm_ut_g.skb_hold[i])
            {
                __logError("Could not mpm_alloc_skb_and_data");

                ret = -1;
                break;
            }

            mpm_skb_and_data_init(mpm_ut_g.skb_hold[i], MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                                  MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);

#if defined(CC_MPM_UT_BUFFER_DUMP)
            mpm_ut_skb_header_dump(mpm_ut_g.skb_hold[i]);
#endif
            /* if(MPM_UT_POINTER_VALID(mpm_ut_g.skb_hold[i]->data)) */
            /* { */
            /*     mpm_ut_mem_dump(PDATA_TO_PFKBUFF(mpm_ut_g.skb_hold[i]->data, BCM_PKT_HEADROOM), 4096); */
            /* } */

            fail_count += mpm_ut_skb_check(mpm_ut_g.skb_hold[i], 0);
        }

        for(i=0; i<MPM_UT_HOLD_SIZE; ++i)
        {
            int retry_count = MPM_UT_RETRY_MAX;

            while((ret = mpm_free_skb_and_data(mpm_ut_g.free_ring_index, mpm_ut_g.skb_hold[i])) && --retry_count);
            if(ret)
            {
                __logError("Could not mpm_free_skb_and_data");

                ret = -1;
                break;
            }
        }
    }

    mpm_ut_test_results(start_time, hold_index * MPM_UT_HOLD_SIZE, fail_count);

    return ret;
}

int bpm_ut_test_skb_and_data_alloc_hold_free(void)
{
    mpm_ut_time_t start_time;
    int hold_count = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_HOLD_SIZE;
    int hold_index;
    int i;

    bcm_print("\n%s: hold %u\n", __FUNCTION__, MPM_UT_HOLD_SIZE);

    mpm_ut_get_time(&start_time);

    for(hold_index=0; hold_index<hold_count; ++hold_index)
    {
        for(i=0; i<MPM_UT_HOLD_SIZE; ++i)
        {
            mpm_ut_g.skb_hold[i] = gbpm_alloc_buf_skb_attach(MPM_UT_DATA_LENGTH);
            if(!mpm_ut_g.skb_hold[i])
            {
                __logError("Could not gbpm_alloc_buf_skb_attach");

                break;
            }
        }

        for(i=0; i<MPM_UT_HOLD_SIZE; ++i)
        {
#if defined(CC_MPM_UT_BPM_RECYCLE_HOOK)
            mpm_ut_g.skb_hold[i]->recycle_flags &= ~SKB_BPM_PRISTINE;
            mpm_ut_g.skb_hold[i]->recycle_hook(mpm_ut_g.skb_hold[i], 0, SKB_DATA_RECYCLE);
            mpm_ut_g.skb_hold[i]->recycle_hook(mpm_ut_g.skb_hold[i], 0, SKB_RECYCLE);
#else
            gbpm_free_buf(mpm_ut_g.skb_hold[i]->data);
            gbpm_free_skb(mpm_ut_g.skb_hold[i]);
#endif
        }
    }

    mpm_ut_test_results(start_time, hold_index * MPM_UT_HOLD_SIZE, 0);

    return 0;
}

int mpm_ut_test_skb_and_data_list_alloc_free(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_SKB_AND_DATA_LIST;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    int nbr_of_lists = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_ALLOC_LIST_SIZE;
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int list_index;
    int ret = 0;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, nbr_of_lists %d, list_size %d\n",
              __FUNCTION__, buf_mode, ring_index, nbr_of_lists, MPM_UT_ALLOC_LIST_SIZE);

    mpm_ut_get_time(&start_time);

    for(list_index=0; list_index<nbr_of_lists; ++list_index)
    {
        struct sk_buff *head_skb;
        struct sk_buff *tail_skb;
        struct sk_buff *curr_skb;
#if defined(CC_MPM_UT_BUFFER_CHECK)
        struct sk_buff *prev_skb = NULL;
#endif
        int skb_index = 0;
        int retry_count = MPM_UT_RETRY_MAX;

        while(!(head_skb = mpm_alloc_skb_and_data_list(ring_index, MPM_UT_ALLOC_LIST_SIZE, &tail_skb)) && --retry_count);
        if(!head_skb)
        {
            __logError("Could not mpm_alloc_skb_and_data_list");

            ret = -1;
            break;
        }

        curr_skb = head_skb;

        while(1)
        {
            mpm_skb_and_data_init(curr_skb, MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                                  MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);

#if defined(CC_MPM_UT_BUFFER_DUMP)
            mpm_ut_skb_header_dump(curr_skb);
#endif
            /* if(MPM_UT_POINTER_VALID(skb->data)) */
            /* { */
            /*     mpm_ut_mem_dump(PDATA_TO_PFKBUFF(skb->data, BCM_PKT_HEADROOM), 4096); */
            /* } */

            fail_count += mpm_ut_skb_check(curr_skb, 1);

#if defined(CC_MPM_UT_BUFFER_CHECK)
            if(prev_skb && curr_skb->prev != prev_skb)
            {
                __logError("LIST[%d][%d]: curr_skb->prev 0x%px != prev_skb 0x%px",
                           list_index, skb_index, curr_skb->prev, prev_skb);

                return -1;
            }

            if(skb_index == MPM_UT_ALLOC_LIST_SIZE - 1)
            {
                if(curr_skb != tail_skb)
                {
                    __logError("LIST[%d][%d]: curr_skb 0x%px != tail_skb 0x%px",
                               list_index, skb_index, curr_skb, tail_skb);

                    return -1;
                }

                break;
            }
            else
            {
                if(curr_skb == tail_skb)
                {
                    __logError("LIST[%d][%d]: curr_skb == tail_skb (0x%px)",
                               list_index, skb_index, curr_skb);
                    return -1;
                }
            }

            prev_skb = curr_skb;
#else
            if(skb_index == MPM_UT_ALLOC_LIST_SIZE - 1)
            {
                break;
            }
#endif
            curr_skb = curr_skb->next;

            skb_index++;
        }

        // Must be NULL terminated linked-list in order to free
        tail_skb->next = NULL;

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_skb_and_data_list(mpm_ut_g.free_ring_index, head_skb)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_skb_and_data_list");

            ret = -1;
            break;
        }
    }

    mpm_ut_test_results(start_time, list_index * MPM_UT_ALLOC_LIST_SIZE, fail_count);

    return ret;
}

int mpm_ut_test_skb_and_data_array_alloc_free(mpm_buf_mode_t buf_mode)
{
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    int nbr_of_arrays = MPM_UT_ALLOC_NBR_OF_BUFFERS / MPM_UT_ALLOC_LIST_SIZE;
    struct sk_buff *skb_array[MPM_UT_ALLOC_LIST_SIZE];
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int array_index;
    int ret = 0;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, nbr_of_arrays %d, array_size %d\n",
              __FUNCTION__, buf_mode, ring_index, nbr_of_arrays, MPM_UT_ALLOC_LIST_SIZE);

    mpm_ut_get_time(&start_time);

    for(array_index=0; array_index<nbr_of_arrays; ++array_index)
    {
        int skb_index;
        int retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_alloc_skb_and_data_array(ring_index, MPM_UT_ALLOC_LIST_SIZE, skb_array)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_alloc_skb_and_data_array");

            break;
        }

        for(skb_index=0; skb_index<MPM_UT_ALLOC_LIST_SIZE; ++skb_index)
        {
            struct sk_buff *curr_skb = skb_array[skb_index];

            mpm_skb_and_data_init(curr_skb, MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                                  MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);

#if defined(CC_MPM_UT_BUFFER_DUMP)
            mpm_ut_skb_header_dump(curr_skb);
#endif
            /* if(MPM_UT_POINTER_VALID(skb->data)) */
            /* { */
            /*     mpm_ut_mem_dump(PDATA_TO_PFKBUFF(skb->data, BCM_PKT_HEADROOM), 4096); */
            /* } */

            fail_count += mpm_ut_skb_check(curr_skb, 1);
        }

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_skb_and_data_array(mpm_ut_g.free_ring_index, MPM_UT_ALLOC_LIST_SIZE, skb_array)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_skb_and_data_array");

            break;
        }
    }

    mpm_ut_test_results(start_time, array_index * MPM_UT_ALLOC_LIST_SIZE, fail_count);

    return ret;
}

int mpm_ut_test_skb_combo_alloc_free(void)
{
    mpm_buf_mode_t skb_buf_mode = MPM_BUF_MODE_SKB_HEADER;
    mpm_buf_mode_t pdata_buf_mode = MPM_BUF_MODE_PDATA_SHINFO;
    mpm_ring_index_t skb_ring_index = mpm_ut_g.alloc_ring_index[skb_buf_mode];
    mpm_ring_index_t pdata_ring_index = mpm_ut_g.alloc_ring_index[pdata_buf_mode];
    mpm_ut_time_t start_time;
    int fail_count = 0;
    int ret = 0;
    int i;

    bcm_print("\n%s: skb (buf_mode %d -> ring_index %d), pdata (buf_mode %d -> ring_index %d)\n",
              __FUNCTION__, skb_buf_mode, skb_ring_index, pdata_buf_mode, pdata_ring_index);

    mpm_ut_get_time(&start_time);

    for(i=0; i<MPM_UT_ALLOC_NBR_OF_BUFFERS; ++i)
    {
        struct sk_buff *skb;
        uint8_t *pData;
        int retry_count = MPM_UT_RETRY_MAX;

        while(!(skb = mpm_alloc_skb_header(skb_ring_index)) && --retry_count);
        if(!skb)
        {
            __logError("Could not mpm_alloc_skb_header");

            ret = -1;
            break;
        }

        retry_count = MPM_UT_RETRY_MAX;

        while(!(pData = mpm_alloc_pdata(pdata_ring_index)) && --retry_count);
        if(!pData)
        {
            __logError("Could not mpm_alloc_pData");

            ret = -1;
            break;
        }

        mpm_skb_header_init(skb, pData, MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                            MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);

#if defined(CC_MPM_UT_BUFFER_DUMP)
        mpm_ut_skb_header_dump(skb);
#endif
        fail_count += mpm_ut_skb_check(skb, 0);

        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_skb_and_data(mpm_ut_g.free_ring_index, skb)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_skb_header");

            ret = -1;
            break;
        }
    }

    mpm_ut_test_results(start_time, i, fail_count);

    return ret;
}

int mpm_ut_test_skb_and_data_list_exhaustion(void)
{
    mpm_buf_mode_t buf_mode = MPM_BUF_MODE_SKB_AND_DATA_LIST;
    mpm_ring_index_t ring_index = mpm_ut_g.alloc_ring_index[buf_mode];
    struct sk_buff *list_head = NULL;
    struct sk_buff *list_tail = NULL;
    mpm_ut_time_t start_time;
    int list_count = 0;
    int buffer_count = 0;
    int fail_count = 0;
    int retry_count;
    int ret = 0;

    bcm_print("\n%s: buf_mode %d -> alloc_ring_index %d, list_size %d\n",
              __FUNCTION__, buf_mode, ring_index, MPM_UT_ALLOC_LIST_SIZE);

    mpm_ut_get_time(&start_time);

    while(1)
    {
        struct sk_buff *head_skb;
        struct sk_buff *tail_skb;

        retry_count = MPM_UT_RETRY_MAX;

        while(!(head_skb = mpm_alloc_skb_and_data_list(ring_index, MPM_UT_ALLOC_LIST_SIZE, &tail_skb)) && --retry_count);
        if(!head_skb)
        {
//            bcm_print("MPM Empty\n");

            break;
        }

        list_count++;

        if(!list_head)
        {
            list_head = head_skb;
        }

        if(list_tail)
        {
            list_tail->next = head_skb;
        }

        list_tail = tail_skb;
    }

    buffer_count = list_count * MPM_UT_ALLOC_LIST_SIZE;

    if(list_tail)
    {
        // NULL terminated linked-list
        list_tail->next = NULL;
    }

    if(list_head)
    {
        struct sk_buff *curr_skb = list_head;
#if defined(CC_MPM_UT_BUFFER_CHECK)
        struct sk_buff *prev_skb = NULL;
        int list_index;
#endif
        int skb_index = 0;

        while(1)
        {
#if defined(CC_MPM_UT_BUFFER_CHECK)
            if(!MPM_UT_BUFFER_VALID(curr_skb))
            {
                __logError("LIST[%d][%d]: Invalid skb 0x%px", curr_skb);

                return -1;
            }

            if(!MPM_UT_BUFFER_VALID(curr_skb->data))
            {
                __logError("LIST[%d][%d]: Invalid skb->data 0x%px", curr_skb->data);

                return -1;
            }
#endif
            mpm_skb_and_data_init(curr_skb, MPM_UT_DATA_LENGTH, mpm_ut_recycle_handler,
                                  MPM_UT_RECYCLE_CONTEXT, &mpm_ut_g.blog);

#if defined(CC_MPM_UT_BUFFER_DUMP)
            mpm_ut_skb_header_dump(curr_skb);
#endif
            /* if(MPM_UT_POINTER_VALID(skb->data)) */
            /* { */
            /*     mpm_ut_mem_dump(PDATA_TO_PFKBUFF(skb->data, BCM_PKT_HEADROOM), 4096); */
            /* } */

            fail_count += mpm_ut_skb_check(curr_skb, 1);

#if defined(CC_MPM_UT_BUFFER_CHECK)
            list_index = skb_index / MPM_UT_ALLOC_LIST_SIZE;

            if(prev_skb && curr_skb->prev != prev_skb)
            {
                __logError("LIST[%d][%d]: curr_skb->prev 0x%px != prev_skb 0x%px",
                           list_index, skb_index, curr_skb->prev, prev_skb);
                return -1;
            }

            if(skb_index == buffer_count - 1)
            {
                if(curr_skb != list_tail)
                {
                    __logError("LIST[%d][%d]: curr_skb 0x%px != list_tail 0x%px",
                               list_index, skb_index, curr_skb, list_tail);
                    return -1;
                }

                if(curr_skb->next)
                {
                    __logError("LIST[%d][%d]: Tail SKB is not NULL");

                    return -1;
                }

                break;
            }
            else
            {
                if(curr_skb == list_tail)
                {
                    __logError("LIST[%d][%d]: curr_skb == list_tail (0x%px)",
                               list_index, skb_index, curr_skb);
                    return -1;
                }
            }

            prev_skb = curr_skb;
#else
            if(skb_index == buffer_count - 1)
            {
                break;
            }
#endif
            curr_skb = curr_skb->next;

            skb_index++;
        }

//        bcm_print("MPM Replenish\n");

        /* ret = mpm_ut_skb_and_data_list_audit(list_head, buffer_count); */
        /* if(ret) */
        /* { */
        /*     __logError("Could not mpm_ut_skb_and_data_list_audit"); */

        /*     return ret; */
        /* } */
        retry_count = MPM_UT_RETRY_MAX;

        while((ret = mpm_free_skb_and_data_list(mpm_ut_g.free_ring_index, list_head)) && --retry_count);
        if(ret)
        {
            __logError("Could not mpm_free_skb_and_data_list");
        }
    }

    mpm_ut_test_results(start_time, buffer_count, fail_count);

    return ret;
}

static int mpm_ut_init(void *virt_base, unsigned int mem_size)

{
    static int initialized = 0;
    mpm_buf_mode_t buf_mode;
#if defined(CC_MPM_UT_BUFFER_CHECK)
    uint8_t *pData;
#endif
    int ret;

    if(!initialized)
    {
        initialized = 1;

        memset(&mpm_ut_g, 0, sizeof(mpm_ut_t));

        for(buf_mode=0; buf_mode<MPM_BUF_MODE_MAX; ++buf_mode)
        {
#if !defined(CC_MPM_DATA_SHINFO_INIT)
            if(buf_mode != MPM_BUF_MODE_PDATA_SHINFO)
#endif
            {
                ret = mpm_alloc_ring_alloc(buf_mode, MPM_UT_ALLOC_RING_SIZE_LOG2,
                                           &mpm_ut_g.alloc_ring_index[buf_mode]);
                if(ret)
                {
                    return ret;
                }
            }
        }

        ret = mpm_free_ring_alloc(MPM_UT_FREE_RING_SIZE_LOG2, &mpm_ut_g.free_ring_index);
        if(ret)
        {
            return ret;
        }
    }

    mpm_ut_g.virt_base = virt_base;
    mpm_ut_g.mem_size = mem_size;
    mpm_ut_g.virt_end = (uint8_t *)(virt_base) + mem_size;

    bcm_print("virt_base: 0x%px, mem_size: %u bytes, virt_end: 0x%px\n",
              mpm_ut_g.virt_base, mpm_ut_g.mem_size, mpm_ut_g.virt_end);

#if defined(CC_MPM_UT_BUFFER_CHECK)
    mpm_ut_g.ref.skb = gbpm_alloc_buf_skb_attach(MPM_UT_DATA_LENGTH);
    if(!mpm_ut_g.ref.skb)
    {
        __logError("Could not gbpm_alloc_buf_skb_attach");

        return -1;
    }

    memset(PDATA_TO_PFKBUFF(mpm_ut_g.ref.skb->data, BCM_PKT_HEADROOM),
           0xFF, MPM_DATA_SHINFO_OFFSET);
#endif

    memset(mpm_ut_g.data, 0xFF, MPM_UT_DATA_BUFFER_SIZE);

#if defined(CC_MPM_UT_BUFFER_CHECK)
    pData = PFKBUFF_TO_PDATA(mpm_ut_g.data, BCM_PKT_HEADROOM);
    mpm_ut_g.ref.fkb = fkb_init(pData, BCM_PKT_HEADROOM, pData, MPM_UT_DATA_LENGTH);
    BCM_ASSERT((void *)mpm_ut_g.ref.fkb == (void *)mpm_ut_g.data);
#endif

    skb_shinforeset((struct skb_shared_info *)
                    (mpm_ut_g.data + MPM_DATA_SHINFO_OFFSET));

    return 0;
}

static void mpm_ut_exit(void)
{
#if defined(CC_MPM_UT_BUFFER_CHECK)
    kfree_skb(mpm_ut_g.ref.skb);
#endif
}

static int mpm_ut_mpm(void)
{
    int ret;

    ret = mpm_ut_test_fkb_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_fkb_list_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_fkb_array_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_pdata_alloc_free(MPM_BUF_MODE_PDATA);
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_pdata_alloc_free(MPM_BUF_MODE_PDATA_SHINFO);
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_pdata_array_alloc_free(MPM_BUF_MODE_PDATA);
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_pdata_array_alloc_free(MPM_BUF_MODE_PDATA_SHINFO);
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_header_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_header_alloc_hold_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_header_list_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_header_array_alloc_free(MPM_BUF_MODE_SKB_HEADER);
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_header_array_alloc_free(MPM_BUF_MODE_SKB_HEADER_LIST);
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_and_data_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_and_data_alloc_hold_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_and_data_list_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_and_data_array_alloc_free(MPM_BUF_MODE_SKB_AND_DATA);
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_and_data_array_alloc_free(MPM_BUF_MODE_SKB_AND_DATA_LIST);
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_combo_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_and_data_list_exhaustion();
    if(ret)
    {
        return ret;
    }

    return 0;
}

static int mpm_ut_mpm_vs_bpm(void)
{
    int ret;

    ret = mpm_ut_test_pdata_array_alloc_free(MPM_BUF_MODE_PDATA);
    if(ret)
    {
        return ret;
    }

    ret = bpm_ut_test_pdata_array_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_header_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = bpm_ut_test_skb_header_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_header_alloc_hold_free();
    if(ret)
    {
        return ret;
    }

    ret = bpm_ut_test_skb_header_alloc_hold_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_and_data_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = bpm_ut_test_skb_and_data_alloc_free();
    if(ret)
    {
        return ret;
    }

    ret = mpm_ut_test_skb_and_data_alloc_hold_free();
    if(ret)
    {
        return ret;
    }

    ret = bpm_ut_test_skb_and_data_alloc_hold_free();
    if(ret)
    {
        return ret;
    }

    return 0;
}

int mpm_ut_run(mpm_test_t test, void *virt_base, unsigned int mem_size)
{
#if defined(CC_MPM_UT_BUFFER_CHECK)
    char buffer_check[] = "ENABLED";
#else
    char buffer_check[] = "DISABLED";
#endif
    int ret;

    bcm_print("Buffer Check: %s\n\n", buffer_check);

    ret = mpm_ut_init(virt_base, mem_size);
    if(ret)
    {
        return ret;
    }

    switch(test)
    {
        case MPM_UT_TEST_MPM:
            ret = mpm_ut_mpm();
            break;

        case MPM_UT_TEST_MPM_VS_BPM:
            ret = mpm_ut_mpm_vs_bpm();
            break;

        default:
            __logError("Invalid test: %d", test);
            ret = -1;
    }

    mpm_ut_exit();

    return ret;
}
