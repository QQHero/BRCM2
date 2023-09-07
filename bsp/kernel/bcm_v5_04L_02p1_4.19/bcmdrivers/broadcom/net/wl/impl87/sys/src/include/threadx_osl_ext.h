/*
 * Threadx OS Support Extension Layer
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
 * $Id: threadx_osl_ext.h 791379 2020-09-24 16:49:51Z $
 */

#ifndef _threadx_osl_ext_h_
#define _threadx_osl_ext_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <tx_api.h>
#include <typedefs.h>

/**
 * Interrupt control.
 *
 * Usage:
 *
 *   OSL_INTERRUPT_SAVE_AREA
 *   OSL_DISABLE
 *   ...critical section...
 *   OSL_RESTORE
*/
#define OSL_INTERRUPT_SAVE_AREA		osl_ext_interrupt_state_t interrupt_save;
#define OSL_DISABLE			interrupt_save = osl_ext_interrupt_disable();
#define OSL_RESTORE			osl_ext_interrupt_restore(interrupt_save);

/**
 * Preemption control.
 *
 * Usage:
 *
 *   OSL_PREEMPT_SAVE_AREA
 *   OSL_PREEMPT_DISABLE
 *   ...critical section...
 *   OSL_PREEMPT_RESTORE
 */
#define OSL_PREEMPT_SAVE_AREA		UINT preempt_save;
#define OSL_PREEMPT_DISABLE		osl_ext_preempt_disable(&preempt_save);
#define OSL_PREEMPT_RESTORE		osl_ext_preempt_restore(preempt_save);

/* Interrupt state checking */
#define ASSERT_IN_ISR()			ASSERT(osl_ext_in_isr());
#define ASSERT_NOT_ISR()		ASSERT(!osl_ext_in_isr());
#define OSL_IN_ISR()			osl_ext_in_isr()

/* Stack checking */
#define OSL_CHECK_OS_STACK()		hnd_thread_check_os_stack();

/**
 * Reentrancy checking.
 *
 * Usage:
 *   OSL_REENTRANCY_CHECK_SAVE_AREA
 *   OSL_REENTRANCY_CHECK_START
 *   ...critical section...
 *   OSL_REENTRANCY_CHECK_END
 */
#ifdef BCMDBG
#define OSL_REENTRANCY_CHECK_SAVE_AREA	static uint reentrancy_save = 0;
#define OSL_REENTRANCY_CHECK_START	osl_ext_reentrancy_check_start(&reentrancy_save, \
					__FILE__, __LINE__);
#define OSL_REENTRANCY_CHECK_END	osl_ext_reentrancy_check_end(&reentrancy_save);
#else
#define OSL_REENTRANCY_CHECK_SAVE_AREA
#define OSL_REENTRANCY_CHECK_START
#define OSL_REENTRANCY_CHECK_END
#endif /* BCMDBG */

/* Thread checking */
#ifdef BCMDBG_ASSERT
#define ASSERT_THREAD(name)		ASSERT((name) == CURRENT_THREAD_NAMEPTR)
#else
#define ASSERT_THREAD(name)
#endif /* BCMDBG_ASSERT */

/**
 * Critical sections.
 *
 * Protection of critical sections is done through an @see osl_ext_lock_t instance and the
 * osl_ext_lock_* API. The locking mechanism used depends on system setup:
 *
 * - In multiprocessor environments (OSL_CONFIG_SMP defined), critical sections are protected
 *   using ThreadX mutexes.
 * - In uniprocessor/multithreaded environments, critical sections are protected by
 *   controlling ThreadX preemption directly, which is faster than using mutexes.
 * - On uniprocessor/singlethreaded environments, even though no locking is required the same
 *   approach as for uniprocessor/multithreaded environments is used because of the low overhead
 *   of ThreadX preemption control.
 *
 * Locks may only be created and manipulated outside interrupt context.
 *
 * @see osl_ext_lock_create
 * @see osl_ext_lock_delete
 * @see osl_ext_lock_acquire
 * @see osl_ext_lock_release
 */
#if defined(OSL_CONFIG_SMP)
typedef TX_MUTEX osl_ext_lock_t;	/* Multiprocessor system */
#else
typedef UINT osl_ext_lock_t;		/* Uniprocessor system */
#endif /* OSL_CONFIG_SMP */

/* Debugging helpers */
#define THREAD_NAMEPTR(t)		((t) ? ((TX_THREAD*)(t))->tx_thread_name : NULL)
#define CURRENT_THREAD_NAMEPTR		THREAD_NAMEPTR(osl_ext_task_current())
#define THREAD_ID(t)			((t) ? ((TX_THREAD*)(t))->tx_thread_id : NULL)
#define THREAD_PRIO(t)			((t) ? ((TX_THREAD*)(t))->tx_thread_priority : NULL)
#define CURRENT_THREAD_ID		THREAD_ID(osl_ext_task_current())
#define THREAD_CPUUTIL(t)		((t) ? ((TX_THREAD*)(t))->cpuutil : NULL)
#define CURRENT_THREAD_CPUUTIL		THREAD_CPUUTIL(osl_ext_task_current())
#define THREAD_WORKLET(t)		((t) ? ((TX_THREAD*)(t))->current_worklet : NULL)
#define CURRENT_THREAD_WORKLET()	THREAD_WORKLET(osl_ext_task_current())

/* Tick conversion */
#define OSL_MSEC_TO_TICKS(msec)		((msec) * 1000)
#define OSL_TICKS_TO_MSEC(ticks)	((ticks) / 1000)
#define OSL_USEC_TO_TICKS(usec)		(usec)
#define OSL_TICKS_TO_USEC(ticks)	(ticks)

/* Semaphore. */
typedef TX_SEMAPHORE osl_ext_sem_t;
#define OSL_EXT_SEM_DECL(sem)		osl_ext_sem_t sem;

/* Mutex. */
typedef TX_MUTEX osl_ext_mutex_t;
#define OSL_EXT_MUTEX_DECL(mutex)	osl_ext_mutex_t mutex;

/* Timer. */
typedef TX_TIMER osl_ext_timer_t;
#define OSL_EXT_TIMER_DECL(timer)	osl_ext_timer_t timer;

/* Task. */
typedef TX_THREAD osl_ext_task_t;
#define OSL_EXT_TASK_DECL(task)		osl_ext_task_t task;

/* Queue. */
typedef TX_QUEUE osl_ext_queue_t;
#define OSL_EXT_QUEUE_DECL(queue)	osl_ext_queue_t queue;

/* Event. */
typedef TX_EVENT_FLAGS_GROUP osl_ext_event_t;
#define OSL_EXT_EVENT_DECL(event)	osl_ext_event_t event;

/* XXX: Hack invocation hnd_thread_cpuutil_alloc() of binding a cpuutil context
 * to a task created via osl_ext_task_create_ex().
 * rte_priv.h is not exposed to threadx_osl_ext.h and dngl_mthe.c directly
 * invokes osl_ext_task_create() instead of hnd_task_create().
 *
 * Better soln? Prefer not exposing all of rte_priv.h hnd_cpuutil here.
 */
void *hnd_thread_cpuutil_bind(unsigned int thread_func);

#ifdef __cplusplus
	}
#endif

#endif  /* _threadx_osl_ext_h_  */
